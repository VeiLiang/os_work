//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_videoview.c
//	  ��Ƶ�طŴ���
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include <string.h>
#include <stdlib.h>
#include <xm_user.h>
#include <xm_base.h>
#include <xm_gdi.h>
#include <xm_key.h>
#include <stdio.h>
#include <common_wstring.h>
#include <xm_malloc.h>
#include <xm_file.h>
#include <xm_rom.h>
#include <xm_assert.h>
#include <xm_dev.h>
#include <xm_printf.h>
#include <xm_image.h>
#include <rom.h>
#include <xm_videoitem.h>

#include "xm_core.h"
#include <xm_mci.h>

#include "rtos.h"



#pragma data_alignment=64
static unsigned char		audio_fifo[4096*4];
#pragma data_alignment=64
static unsigned char		audio_data[0x8000];
extern 	int h264_encode_avi_stream (char *filename, char *filename2, struct H264EncodeStream *H264Stream,
				const char **argv, int argc);

static unsigned char h264codec_frame_rate = 30;	// ÿ��25֡ȱʡ
static volatile unsigned int video_time_size = 1*60;	// ȱʡ3����

#define	CODEC_TIMEOUT	400	// 100�������ʱ

static OS_EVENT	h264_scalar_event;
static OS_EVENT	h264_encode_event;

static OS_RSEMA	h264_scalar_sema;	// ��������ź���������ScalarӲ��

static OS_TASK TCB_H264CodecTask;
static OS_STACKPTR int StackH264CodecTask[XMSYS_H264CODEC_TASK_STACK_SIZE/4];          /* Task stacks */

// h264 Codec ����ģʽ
static int volatile h264codec_mode;
// H264 Codec״̬(���������������)
// ����������STOP��WORK����״̬
// ����������STOP/WORK/PAUSE����״̬
static int volatile h264codec_state;

static const char* h264codec_mode_code[] = {"IDLE", "RECORD", "PLAYBACK"};
static const char* h264codec_state_code[] = {"STOP", "WORK", "PAUSE"};

// ֻ����һ��������
static volatile H264CODEC_COMMAND_PARAM h264codec_command_param;

static OS_RSEMA h264codec_sema;			// ���Ᵽ��
static OS_MAILBOX h264codec_mailbox;		// һ�ֽ����䡣
static char h264codec_mailbuf;					// һ���ֽڵ�����


// codec����Ӧ���������������
static void reply_response_message (char response_code)
{
	// �����������ݣ���ֹ����
	OS_ClearMB (&h264codec_mailbox);
	OS_PutMail1 (&h264codec_mailbox, &response_code);
}

// ��������Ӧ����Ϣ
static void clear_response_message (void)
{
	OS_ClearMB (&h264codec_mailbox);
}

static void clear_codec_command (void)
{
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	h264codec_command_param.command = 0;
	OS_Unuse(&h264codec_sema);
}
unsigned int XMSYS_SensorGetCameraChannel (void)
{
	return 0;
}

// ��֧��һ������ͷ����
unsigned int XMSYS_SensorGetCameraChannelWidth (unsigned int channel)
{
	return 1280;
}

unsigned int XMSYS_SensorGetCameraChannelHeight (unsigned int channel)
{
	return 720;
}



void XMSYS_SensorSetCameraChannel (unsigned int channel)
{
}

// ���ء���Ƶ��¼�ֶ�ʱ�䳤�ȡ�ѡ���ʱ��ֵ(��)
extern unsigned int	AP_GetVideoTimeSize (void);

int _ReadPic(unsigned char * image, int size, int nro, char *name, int width, int height,
            int format)
{
    int ret = 0;
    void *readFile = NULL;
	 
	 void  *yuvFile = NULL;
	 static unsigned int file_size;
	 
	 XM_printf ("_ReadPic index=%d\n", nro);

    if(yuvFile == NULL)
    {
        yuvFile = XM_fopen(name, "rb");

        if(yuvFile == NULL)
        {
            XM_printf("\nUnable to open YUV file: %s\n", name);
            ret = -1;
            goto end;
        }

        XM_fseek(yuvFile, 0, SEEK_END);
        file_size = XM_filesize (yuvFile);
    }

    readFile = yuvFile;

    /* Stop if over last frame of the file */
    if((unsigned int)size * (nro + 1) > file_size)
    {
        XM_printf("\nCan't read frame, EOF\n");
        ret = -1;
        goto end;
    }

    if(XM_fseek(readFile, (unsigned int)size * nro, SEEK_SET) != 0)
    {
        XM_printf("\nI can't seek frame no: %i from file: %s\n",
                nro, name);
        ret = -1;
        goto end;
    }

    if((width & 0x0f) == 0)
        XM_fread(image, 1, size, readFile);
    else
    {
        int i;
        unsigned char *buf = image;
        int scan = (width + 15) & (~0x0f);

        /* Read the frame so that scan (=stride) is multiple of 16 pixels */

        if(format == 0) /* YUV 4:2:0 planar */
        {
            /* Y */
            for(i = 0; i < height; i++)
            {
                XM_fread(buf, 1, width, readFile);
                buf += scan;
            }
            /* Cb */
            for(i = 0; i < (height / 2); i++)
            {
                XM_fread(buf, 1, width / 2, readFile);
                buf += scan / 2;
            }
            /* Cr */
            for(i = 0; i < (height / 2); i++)
            {
                XM_fread(buf, 1, width / 2, readFile);
                buf += scan / 2;
            }
        }
        else if(format == 1)    /* YUV 4:2:0 semiplanar */
        {
            /* Y */
            for(i = 0; i < height; i++)
            {
                XM_fread(buf, 1, width, readFile);
                buf += scan;
            }
            /* CbCr */
            for(i = 0; i < (height / 2); i++)
            {
                XM_fread(buf, 1, width, readFile);
                buf += scan;
            }
        }
        else if(format <= 9)   /* YUV 4:2:2 interleaved or 16-bit RGB */
        {
            for(i = 0; i < height; i++)
            {
                XM_fread(buf, 1, width * 2, readFile);
                buf += scan * 2;
            }
        }
        else    /* 32-bit RGB */
        {
            for(i = 0; i < height; i++)
            {
                XM_fread(buf, 1, width * 4, readFile);
                buf += scan * 4;
            }
        }

    }

  end:

    return ret;
}

char pictureMem_bus_address[1920*1080*3/2];
XMSYSSENSORPACKET packet;
static int pic_no = 0;
static const char yuv_file[] = "\\Y_UV420\\12800720.YUV";

XMSYSSENSORPACKET * XMSYS_SensorCreatePacket (int channel, int user)
{
	if(pic_no >= 100)
		return NULL;
	if(_ReadPic((unsigned char *) pictureMem_bus_address,
                    1280*720*3/2, pic_no,
                    (char *)yuv_file,
                   1280, 720, 0) != 0)
		return NULL;
	
	pic_no ++;
	memset (&packet, 0, sizeof(packet));
	packet.width = 1280;
	packet.height = 720;
	packet.size = 1280*720*3/2;
	packet.buffer = (unsigned char *) pictureMem_bus_address;
	return &packet;
}

void XMSYS_SensorDeletePacket (int channel, int user, XMSYSSENSORPACKET *packet)
{

}

int XMSYS_H264CodecCheckIFrame (void)
{
	return 0;
}

int XMSYS_H264CodecCheckEvent (struct H264EncodeStream *stream)
{
	//if(XM_GetTickCount() >= stream->stop_ticket)
	if(pic_no >= 100)
	{
		// ��¼�ﵽ�ֶ�ʱ��
		XM_printf ("%d, Expired\n", XM_GetTickCount());
		return 3;
	}
	return 0;
}

static void *fp_raw_pcm = NULL;
static int pcm_index = 0;
static unsigned int audio_index = 0;
int XMSYS_H264CodecGetAudioFrame (short *samples, int frame_size, int nb_channels, unsigned int stream_ticket)
{
	int ret;
	int j;
	//XM_printf ("frame_size=%d, nb_channels=%d\n", frame_size, nb_channels);
	if(fp_raw_pcm == NULL)
	{
		fp_raw_pcm = XM_fopen ("\\test.pcm", "rb");
		XM_printf ("open test pcm %s\n", fp_raw_pcm ? "OK" : "NG");
	}
	if(fp_raw_pcm == NULL)
		return 0;
	
	{
		float audio_ticket = (float)(audio_index * frame_size ) / 8.0f;	
		//float audio_ticket = (float)(audio_index * frame_size * 1000 ) / (8000);	
		if(audio_ticket > (float)stream_ticket)
			return 0;
		XM_printf ("stream_ticket = %f, audio_ticket=%f\n", stream_ticket, audio_ticket);
	}
	
	audio_index ++;
	
	if(pcm_index >= 100)
	{
		XM_printf ("\n\nseek\n\n");
		XM_fseek (fp_raw_pcm, 0, SEEK_SET); 
		pcm_index = 0;
	}
	
	//XM_printf ("stream_ticket=%d, audio_index=%d\n", video_index, audio_index);
	
	//XM_printf ("index=%d, frame_size=%d, channels=%d\n", pcm_index, frame_size, nb_channels);
	
	ret = XM_fread (samples, frame_size * nb_channels * 2, 1, fp_raw_pcm);
	
	pcm_index ++;
		
	return frame_size * nb_channels;
}

void 	XMSYS_H264CodecReset (void)
{
}
void XMSYS_H264CodecConfigScale (struct H264EncodeStream *stream)
{
	stream->do_scale[0] = 0;
	stream->do_scale[1] = 0;
	stream->do_scale[2] = 0;
	stream->do_scale[3] = 0;
}

// ����ֵ
// 0		-->	��������
//	1		-->	�����˳�
//	2		-->	ǰ��
//	3		-->	����
// 5     -->   �طŶ�λ(��)
int XMSYS_H264DecoderCheckEvent (struct H264DecodeStream *stream)
{
	char		file_name[XMSYS_H264_CODEC_MAX_FILE_NAME + 1];
	u8_t event;
	int ret = 0;
	stream->curr_ticket += 33;
		
	// ����Ƿ�����ⲿ�¼�
	event = OS_GetEventsOccurred (&TCB_H264CodecTask);
	
	// ���һ�������¼�
	if(event & XMSYS_H264_CODEC_EVENT_ONE_KEY_PROTECT)
	{
		// һ����������
		char response_code = (char)(-1);
		// �����¼�
		OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_ONE_KEY_PROTECT);
		// һ���������� (����ط��н�ĳ����Ƶ����)
		// �ļ�����ʹ��
		if(stream->file_lock == 0)
		{
			int lock = AP_VideoItemLockVideoFileFromFileName (stream->file_name[0]);	// ��ǰֻʹ��һ��ͨ��
			if(lock)
			{
				stream->file_lock = 1;
				response_code = 0;
				//XM_printf ("playback lock (%s) OK\n", stream->file_name[0]);
			}
			else
			{
				//XM_printf ("playback lock (%s) NG\n", stream->file_name[0]);
			}
		}
		else
		{
			// �ļ��ѱ���
			response_code = 0;
		}
		// Ӧ��ͻ���
		reply_response_message (response_code);
	}
	
	if(event & XMSYS_H264_CODEC_EVENT_COMMAND)
	{
		int command;
		H264CODEC_COMMAND_PARAM command_param;
		// ��ȡ����
		OS_Use(&h264codec_sema); /* Make sure nobody else uses */
		command = h264codec_command_param.command;
		memcpy (&command_param, (void *)&h264codec_command_param, sizeof(h264codec_command_param));
		OS_Unuse(&h264codec_sema);
		
		// �������
		if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_START)
		{
			// ���ſ�ʼ����
			// ����µĲ����ļ��Ƿ��뵱ǰ�����ļ�һ��
			if(!strcmp (stream->file_name[0], command_param.file_name))
			{
				// ��ͬ�ļ���
				// Ӧ��ͻ���
				reply_response_message (0);
				clear_codec_command ();
				OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
				ret = 0;
			}
			else
			{
				// ��ͬ�ļ���
				XM_printf ("playback end to play different video\n");
				// �˳����Ź���
				ret = 1;
			}		
		}
		else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_STOP)
		{
			// �طŽ���
			stream->command_player_stop = 1;
		command_player_stop:
			XM_printf ("playback end due to PLAYER_STOP\n");
			ret = 1;
		}
		else if(command == XMSYS_H264_CDOEC_COMMAND_PLAYER_FORWARD)
		{
			// �طſ��
		command_player_forword:
			stream->system_ticket = (unsigned int)(-1);
			// �������event
			clear_codec_command ();
			reply_response_message (0);
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
			XM_printf ("PLAYER_FORWARD\n");
			ret = 2;
		}
		else if(command == XMSYS_H264_CDOEC_COMMAND_PLAYER_BACKWARD)
		{
			// �طſ���
		command_player_back:
			stream->system_ticket = (unsigned int)(-1);
			// �������event
			clear_codec_command ();
			reply_response_message (0);
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
			XM_printf ("PLAYER_BACKWARD\n");
			ret = 3;
		}
		else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_SEEK)
		{
			// �طŶ�λ
		command_player_seek:
			stream->system_ticket = (unsigned int)(-1);
			stream->seek_time = command_param.seek_time;
			// ��������¼�
			clear_codec_command ();
			// Ӧ��
			reply_response_message (0);
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
			
			XM_printf ("PLAYER_SEEK %d\n", stream->seek_time);
			ret = 5;		// ���SEEK����
		}
		else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_PAUSE)
		{
			// ��ͣ����
			
			stream->system_ticket = (unsigned int)(-1);
			stream->playback_mode = XM_VIDEO_PLAYBACK_MODE_NORMAL;
			
			// ��������¼�
			clear_codec_command ();
			reply_response_message (0);
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
			
			// Ӧ��
			XM_printf ("PLAYER_PAUSE\n");
			
			// �ȴ����������¼�
			while(1)
			{
				event = OS_GetEventsOccurred (&TCB_H264CodecTask);
				// ����Ƿ������������
				if(event & XMSYS_H264_CODEC_EVENT_COMMAND)
				{
					OS_Use(&h264codec_sema); /* Make sure nobody else uses */
					command = h264codec_command_param.command;
					strcpy (file_name, (const char *)h264codec_command_param.file_name);
					OS_Unuse(&h264codec_sema);
					if(command == XMSYS_H264_CODEC_COMMAND_RECORDER_START)
					{
						// ¼����������
						// ȡ����ͣ���˳�����
						goto command_player_stop;
					}
					else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_STOP)
					{
						// ����ֹͣ����
						stream->command_player_stop = 1;
						// ȡ����ͣ���˳�����
						goto command_player_stop;
					}
					else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_START)
					{
						// ���ż�������
						// �жϲ���������ļ����Ƿ��뵱ǰ����Ƶ�ļ���һ��
						if(strcmp (file_name, stream->file_name[0]))
						{
							// ��ͬ���ļ���
							// �����������ļ�
							// �˳�
							goto command_player_stop;
						}
						else
						{
							// ��ͬ���ļ�������ʾ�ָ�����
							// ȡ����ͣ����������
							// �������event
							clear_codec_command ();
							reply_response_message (0);
							OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
							ret = 4;		// ��ʾ���Żָ�
							break;
						}
					}
					else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_PAUSE)
					{
						// �ָ�����
						stream->system_ticket = (unsigned int)(-1);
						clear_codec_command ();
						reply_response_message (0);
						OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
						ret = 4;		// ��ʾ���Żָ�
						break;
					}
					else if(command == XMSYS_H264_CDOEC_COMMAND_PLAYER_FORWARD)
					{
						// �ط�ǰ��
						goto command_player_forword;
					}
					else if(command == XMSYS_H264_CDOEC_COMMAND_PLAYER_BACKWARD)
					{
						// �طź���
						goto command_player_back;
					}
					else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_SEEK)
					{
						// �طŶ�λ
						goto command_player_seek;
					}
					else
					{
						// �Ƿ��������
						clear_codec_command ();
						OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
					}
				}
				
				OS_Delay (2);
			}			
		}
		else
		{
			// �Ƿ��������
			ret = 0;
			clear_codec_command ();
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
		}
		
		return ret;
	}
	// �ȴ�ֱ����һ�ζ�ʱ���¼�
	return 0;
}

VOID	XMSYS_VideoScaleCopyVideoImageIntoFrameBuffer (struct H264DecodeStream *decode_stream,
																	  unsigned char *buffer,
																	  unsigned char *u,
																	  unsigned char *v)
{
}

void XMSYS_AHB_Lock (void)
{
}

void XMSYS_AHB_Unlock (void)
{
}

void h264_decode_reset (void)
{
}

void *h264_fopen (const char *filename, const char *mode)
{
	return XM_fopen (filename, mode);
}
int h264_fclose (void *stream)
{
	return XM_fclose (stream);
}

int h264_fseek (void *stream, long offset, int mode)
{
	return XM_fseek (stream, offset, mode);
}
size_t h264_fread (void *ptr, size_t size, size_t nelem, void *stream)
{
	return XM_fread (ptr, size, nelem, stream);
}
size_t h264_fwrite (void *ptr, size_t size, size_t nelem, void *stream)
{
	int ret = XM_fwrite (ptr, size, nelem, stream);
	return ret;
}

int h264_filelength (void *stream)
{
	int ret = XM_filesize (stream);
	return ret;

}

void *os_fopen (const char *filename, const char *mode)
{
	return XM_fopen (filename, mode);
}

int os_fclose (void *stream)
{
	return XM_fclose (stream);
}

int os_fseek (void *stream, long offset, int mode)
{
	return XM_fseek (stream, offset, mode);
}

size_t os_fread (void *ptr, size_t size, size_t nelem, void *stream)
{
	return XM_fread (ptr, size, nelem, stream);
}

size_t os_fwrite (void *ptr, size_t size, size_t nelem, void *stream)
{
	return XM_fwrite (ptr, size, nelem, stream);
}


void h264codec_RecorderStart (void)
{
	char h264name[32];
	char h264name_f[32];
	char h264name_r[32];
	
	XMSYSTEMTIME LocalTime;
	struct H264EncodeStream EncodeStream;
	HANDLE hVideoItem;
	XMVIDEOITEM *pVideoItem;
	u8_t event;
	//XMWAVEFORMAT waveFormat;
	unsigned int channel;
	XMSYSTEMTIME CurrTime;
	

	// ��¼����
	while(1)
	{
		XM_GetLocalTime (&LocalTime);
		memset (&EncodeStream, 0, sizeof(EncodeStream));
			XM_printf("memset EncodeStream11\n");
			
		memset (h264name_f, 0, sizeof(h264name_f));
		memset (h264name_r, 0, sizeof(h264name_r));
		memset (h264name, 0, sizeof(h264name));
		
		hVideoItem = NULL;
		
		// ע�⣬���ڵ�XM_VideoItemCreateVideoItemHandle ����Bug��û���ͷŵĴ���
		//XM_printf ("Please resolve bugs within XM_VideoItemCreateVideoItemHandle\n");
		// ��ȡһ���µ���Ƶ���ļ���
		channel = XMSYS_SensorGetCameraChannel ();
		// ���ϵͳRTCʱ���Ƿ������á�ARK1930�ĺ��Ӿ�����û��RTC����Ҫ�ⲿ����RTCʱ�䡣
		if(!XM_GetLocalTime(&CurrTime))
		{
			// ��δ���ã�ִ��¼�����������
			// XM_printf ("RTC not setup, save NG\n");
			hVideoItem = NULL;
		}
		else
		{
			hVideoItem = XM_VideoItemCreateVideoItemHandle (channel, &CurrTime);
		}
		if(hVideoItem == NULL)
		{
		create_video_item_failed:
			//XM_printf ("XM_VideoItemCreateVideoItemHandle failed\n");
			// �ָ���ʼ״̬
			// h264codec_mode = XMSYS_H264_CODEC_MODE_IDLE;
			// h264codec_state = XMSYS_H264_CODEC_STATE_STOP;
			// ��UI�߳�Ͷ���쳣��Ϣ
			// 20141020 ZhuoYongHong
			// ������ʱ��Ƶ�ļ������ڼ�¼��Ƶ��
			// ���洢����һ�β���ʱ��ĳЩ����»�ȡ���ļ�ϵͳ����������ϳ�ʱ��(10��~30��)��������Ƶ�����ݿ���ʱ�޷�ʹ�ã�
			// �������޷���¼��Ƶ��
			// Ϊ����������⣬����һ����ʱ��Ƶ��¼�ļ������ڱ���ʵʱ��Ƶ��
			if(XM_VideoItemCreateVideoTempFile (channel, h264name_f, sizeof(h264name_f)))
			{
				// ����һ����ʱ��Ƶ�ļ��ɹ�
				strcpy (EncodeStream.file_name[0], h264name_f);
				EncodeStream.file_save = 1;
				//EncodeStream.file_temp = 1;	// �������ʱ��Ƶ���ļ�
			}
			else
			{
				// ����һ���߼���Ƶ�ļ�(�޷�д��)
				XM_KeyEventProc (VK_AP_VIDEOSTOP, (unsigned char)3);
				EncodeStream.file_save = 1;
				sprintf (h264name_f, "\\TEST.AVI");
				strcpy (EncodeStream.file_name[0], h264name_f);
			}
		}
		else
		{
			pVideoItem = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
			if(pVideoItem == NULL)
			{
				hVideoItem = NULL;
				goto create_video_item_failed;
			}
			if(XM_VideoItemGetVideoFilePath (pVideoItem, channel, h264name_f, sizeof(h264name_f)))
			{
				// �����ļ���
				strcpy (EncodeStream.file_name[0], h264name_f);
				EncodeStream.file_save = 1;
			}
			else
				goto create_video_item_failed;
		}

		// ���õ�ǰʹ�õķֱ���
		//EncodeStream.width[0] = XMSYS_CH0_SENSOR_FRAME_WIDTH;
		//EncodeStream.height[0] = XMSYS_CH0_SENSOR_FRAME_HEIGHT;	
		EncodeStream.width[0] = XMSYS_SensorGetCameraChannelWidth (channel);
		EncodeStream.height[0] = XMSYS_SensorGetCameraChannelHeight (channel);	
		
		XM_printf ("codec %s, w=%d, h=%d, %s\n", EncodeStream.file_name[0],
				  EncodeStream.width[0],
				  EncodeStream.height[0],
				  EncodeStream.file_save ? "save" : "look");		
		
		EncodeStream.type = 1;
		
		EncodeStream.audio_fifo = audio_fifo;
		EncodeStream.audio_data = audio_data;
			
		EncodeStream.frame_rate = h264codec_frame_rate;
		EncodeStream.frame_number = 0;
		EncodeStream.start_ticket = XM_GetTickCount();
		EncodeStream.curr_ticket = EncodeStream.start_ticket;
		// ������Ƶ��¼ʱ�䳤��
		//XM_printf ("VideoTimeSize=%d\n", AP_GetVideoTimeSize());
		if(EncodeStream.file_save)
			EncodeStream.stop_ticket = XM_GetTickCount() + 1000 * AP_GetVideoTimeSize();	
		else
			EncodeStream.stop_ticket = XM_GetTickCount() + 1000 * 30;	
		//XM_printf ("h264_encoder_double_stream start_ticket=%d, stop_ticket=%d\n", EncodeStream.curr_ticket, EncodeStream.stop_ticket);

		//XM_printf ("%d Enter %s\n",  XM_GetTickCount(), h264name_f);
		
		// ����H264���룬�ȴ�����(�ⲿ�����¼��ʱ���¼�)
	//	h264_encode_avi_stream (h264name_f, NULL, &EncodeStream, NULL, NULL);
		XM_printf ("%d Leave\n",  XM_GetTickCount());
		
		// ����Ƿ�����ʱ��Ƶ���ļ�
//		if(EncodeStream.file_temp)
		{
			// ��ʱ��Ƶ��
		}
				
		
		// ����˳�ԭ��
		
	//	if(event & XMSYS_H264_CODEC_EVENT_COMMAND)
		{
			// �ⲿ����
			XM_printf ("Exit Codec Loop\n");
			
			// �ȴ���Ƶ����ر�
			if(hVideoItem)
				XM_VideoItemWaitingForUpdate (EncodeStream.file_name[0]);
			break;
		}
					
		// ��¼�ļ���ʱ�˳�����ʼ��һ��¼���¼
		//XM_printf ("h264_encoder_double_stream end\n");	
	}
	
}



void XMSYS_H264CodecOpenAudioRecord (void)
{

}

void XMSYS_H264CodecCloseAudioRecord (void)
{
	
}

extern 	int  h264_decode_avi_stream (const char *filename, const char *filename2, struct H264DecodeStream *stream);


// H264���������
void XMSYS_H264CodecTask (void)
{
	OS_U8 h264_event;		// �ⲿ�¼�

	H264CODEC_COMMAND_PARAM command;
	
	char ret;
	int loop = 0;
	struct H264DecodeStream dec_stream;
	char file_name[XMSYS_H264_CODEC_MAX_FILE_NAME+1];
	int codec_ret;
	int video_usage;
	
	XMWAVEFORMAT waveFormat;
	LONG result;
		
	//XM_printf ("XMSYS_H264CodecTask\n");	
	
	while(1)
	{
		h264_event = OS_WaitEvent(XMSYS_H264_CODEC_EVENT_COMMAND|XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL);
		
		if(h264_event & XMSYS_H264_CODEC_EVENT_COMMAND)
		{
			OS_Use(&h264codec_sema); /* Make sure nobody else uses */
			memcpy (&command, (const void *)&h264codec_command_param, sizeof(H264CODEC_COMMAND_PARAM));
			h264codec_command_param.command = 0;		// �������
			// �������
			OS_Unuse(&h264codec_sema); /* Make sure nobody else uses */

			switch (command.command)
			{
				case XMSYS_H264_CODEC_COMMAND_RECORDER_START:
					XM_printf ("XMSYS_H264_CODEC_COMMAND_RECORDER_START\n");
					OS_Use(&h264codec_sema); /* Make sure nobody else uses */
					h264codec_mode = XMSYS_H264_CODEC_MODE_RECORD;
					h264codec_state = XMSYS_H264_CODEC_STATE_WORK;
					OS_Unuse(&h264codec_sema); 
					// Ӧ��UI�߳�
					ret = 0;
					OS_PutMail1 (&h264codec_mailbox, &ret);	
					clear_codec_command ();
					XMSYS_SensorCaptureStart ();

					// ����h264 codec����
					h264codec_RecorderStart ();
					break;

				case XMSYS_H264_CODEC_COMMAND_RECORDER_STOP:

					XM_printf ("XMSYS_H264_CODEC_COMMAND_RECORDER_STOP\n");
					//XMSYS_SensorCaptureStop ();
					OS_Use(&h264codec_sema); /* Make sure nobody else uses */
					h264codec_mode = XMSYS_H264_CODEC_MODE_IDLE;
					h264codec_state = XMSYS_H264_CODEC_STATE_STOP;
					OS_Unuse(&h264codec_sema); 


					// Ӧ��UI�߳�
					//ret = 0;
					//OS_PutMail1 (&h264codec_mailbox, &ret);	//

					clear_codec_command ();
					break;

				case XMSYS_H264_CODEC_COMMAND_PLAYER_START:
					XM_printf ("XMSYS_H264_CODEC_COMMAND_PLAYER_START\n");

					OS_Use(&h264codec_sema); /* Make sure nobody else uses */
					h264codec_mode = XMSYS_H264_CODEC_MODE_PLAY;
					h264codec_state = XMSYS_H264_CODEC_STATE_WORK;
					OS_Unuse(&h264codec_sema); 

					memset (&dec_stream, 0, sizeof(dec_stream));
					dec_stream.curr_ticket = OS_GetTime();
					dec_stream.playback_mode = command.playback_mode;
					dec_stream.seek_time = command.seek_time;
					memset (file_name, 0, sizeof(file_name));
					strcpy (file_name, (char *)command.file_name);
					// ��ǰ��֧��һ����ͬ������
					strcpy (dec_stream.file_name[0], (char *)command.file_name);
					// Ӧ��UI�߳�
					ret = 0;
					OS_PutMail1 (&h264codec_mailbox, &ret);	//

					// ��������
					XM_printf ("decode_avi %s\n", file_name);

					// ����Ƶ
					waveFormat.wChannels = 1;
					waveFormat.wBitsPerSample = 16;
					waveFormat.dwSamplesPerSec = 8000;

					// ��ָ���ļ�������Ƶ���ʹ���У���ֹɾ����������Ƶ��ɾ��
					video_usage = XM_VideoItemMarkVideoFileUsage (file_name);

					//	result = XM_WaveOpen (XMMCIDEVICE_PLAY, &waveFormat);
					codec_ret = h264_decode_avi_stream (file_name,	NULL,	&dec_stream);
					//	XM_WaveClose (XMMCIDEVICE_PLAY);

					if(video_usage)
						XM_VideoItemMarkVideoFileUnuse (file_name);

					//XM_printf ("close Output\n");
					//			XMSYS_VideoSetImageOutputReady (0, 0, 0);

					OS_Use(&h264codec_sema); /* Make sure nobody else uses */
					h264codec_mode = XMSYS_H264_CODEC_MODE_IDLE;
					h264codec_state = XMSYS_H264_CODEC_STATE_STOP;
					OS_Unuse(&h264codec_sema); 

					// ��UI�̷߳��Ͳ��Ž�����Ϣ
					if(codec_ret == 0)
						XM_KeyEventProc (VK_AP_VIDEOSTOP, (SHORT)(AP_VIDEOEXITCODE_FINISH | (dec_stream.playback_mode << 8)) );
					else
						XM_KeyEventProc (VK_AP_VIDEOSTOP, AP_VIDEOEXITCODE_STREAMERROR);
					break;

				case XMSYS_H264_CODEC_COMMAND_PLAYER_STOP:
					XM_printf ("XMSYS_H264_CODEC_COMMAND_PLAYER_STOP\n");
					OS_Use(&h264codec_sema); /* Make sure nobody else uses */
					h264codec_mode = XMSYS_H264_CODEC_MODE_IDLE;
					h264codec_state = XMSYS_H264_CODEC_STATE_STOP;
					OS_Unuse(&h264codec_sema); 

					// Ӧ��UI�߳�
					ret = 0;
					OS_PutMail1 (&h264codec_mailbox, &ret);	//

					break;

				default:
					XM_printf ("clar codec command (%d)\n", command.command);
					clear_codec_command ();
					break;

			}


		}
		
		if(h264_event & XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL)
		{
			// �л�¼��ͨ��
		}
	}

}


void XMSYS_H264CodecInit (void)
{
	// H264 Codec Cache�ļ���ʼ��
	//XMSYS_H264FileInit ();
	
	h264codec_mode = XMSYS_H264_CODEC_MODE_IDLE;
	h264codec_state = XMSYS_H264_CODEC_STATE_STOP;
	
	OS_CREATERSEMA(&h264codec_sema); /* Creates resource semaphore */
	
	OS_CREATEMB (&h264codec_mailbox, 1, 1, &h264codec_mailbuf);	
	
	// �ж��¼�
	//irq_unmask(T18XX_H264_INT);
	OS_EVENT_Create (&h264_scalar_event);
	// ר����H264 ������
	OS_EVENT_Create (&h264_encode_event);
	
	// Scalar�������
	OS_CREATERSEMA (&h264_scalar_sema);
	
	OS_CREATETASK(&TCB_H264CodecTask, "H264CodecTask", XMSYS_H264CodecTask, XMSYS_H264CODEC_TASK_PRIORITY, StackH264CodecTask);
}

void XMSYS_H264CodecExit (void)
{
	OS_Terminate(&TCB_H264CodecTask);

	OS_DeleteRSema (&h264_scalar_sema);

	OS_EVENT_Delete (&h264_encode_event);

	OS_EVENT_Delete (&h264_scalar_event);

	OS_DeleteMB (&h264codec_mailbox);

	OS_DeleteRSema (&h264codec_sema);
}

// ��ȡ��ǰH264 Codec�Ĺ���ģʽ
int XMSYS_H264CodecGetMode (void)
{
	return h264codec_mode;
}

const char* XMSYS_H264CodecGetModeCode (void)
{
	return h264codec_mode_code[h264codec_mode];
}

// ��ȡH264 Codec��ǰ�Ĺ���״̬
int XMSYS_H264CodecGetState (void)
{
	return h264codec_state;
}

const char* XMSYS_H264CodecGetStateCode (void)
{
	return h264codec_state_code[h264codec_state];
}

// ֹͣ¼���¼
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecRecorderStop  (void)
{
	int ret = -1;
	char response_code = -1;
	
	clear_response_message ();
	
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	if(h264codec_command_param.command)
	{
		XM_printf ("command %d busy\n", h264codec_command_param.command);
		OS_Unuse(&h264codec_sema);
		return -1;
	}

	if(h264codec_mode == XMSYS_H264_CODEC_MODE_PLAY)
	{
		OS_Unuse(&h264codec_sema);
		XM_printf ("MODE_PLAY do nothing to do RECORDER STOP command\n");
		//return XMSYS_H264CodecPlayerStop ();
		ret = 0;
	}
	else if(h264codec_mode == XMSYS_H264_CODEC_MODE_RECORD)
	{
		XM_printf ("RECORDER_STOP\n");
		// ������¼
		h264codec_command_param.command = XMSYS_H264_CODEC_COMMAND_RECORDER_STOP;
		OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* ֪ͨ�¼� */
		OS_Unuse(&h264codec_sema);
		
		// �ȴ�H264 Codec�߳�Ӧ��
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, 1000) == 0)
		{
			// codec��Ӧ�𣬻����Ϣ
			ret = (int)response_code;
			XM_printf ("RecorderStop resp=%d\n", ret);
			
			// ��ʱ1�룬�ȴ�codecֹͣ
			if(ret == 0)
			{
				unsigned int ticket = XM_GetTickCount ();
				while(h264codec_mode != XMSYS_H264_CODEC_MODE_IDLE)
				{
					if( abs(XM_GetTickCount() - ticket) > 2000 )
						break;
					OS_Delay (1);
				}
				if(h264codec_mode == XMSYS_H264_CODEC_MODE_IDLE)
					ret = 0;
				else
					ret = -2;
			}
		}
		else
		{
			// ��ʱ
			XM_printf ("RecorderStop timeout\n");
			ret = -2;
		}
	}
	else
	{
		// ����״̬ģʽ 
		// (�ظ���RECORDER STOP�������뵽��״̬)
		OS_Unuse(&h264codec_sema);
		XM_printf ("RecorderStop, idle\n");
		// Ӧ��ɹ�
		ret = 0;
	}
	return ret;
}

// �л�����ͷͨ��
// ����ֵ
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecRecorderSwitchChannel  (unsigned int camera_channel)
{
	int ret = -1;
	char response_code = -1;
	
	if(camera_channel >= 2)
	//if(camera_channel >= 1)		// ��ǰ�汾��֧��ǰ������ͷ¼��
	{
		XM_printf ("Can't switch to camera channel (%d)\n",  camera_channel);
		return ret;
	}
	
	clear_response_message ();
	
	// �������ͷͨ���Ƿ�������һ��
	if(XMSYS_SensorGetCameraChannel() == camera_channel)
	{
		XM_printf ("same channel (%d)\n", camera_channel);
		return 0;
	}
	
	/*
	ret = XMSYS_H264CodecRecorderStop();
	if(ret)
	{
		return ret;
	}*/
		
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	if(h264codec_mode == XMSYS_H264_CODEC_MODE_RECORD)
	{
		// �����µ�����ͷͨ��
		XMSYS_SensorSetCameraChannel (camera_channel);
		// �����¼�
		OS_SignalEvent(XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL, &TCB_H264CodecTask); /* ֪ͨ�¼� */
		OS_Unuse(&h264codec_sema);
		
		// ��Ҫ�ȴ�codec��Ӧ��
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
		{
			// codec��Ӧ�𣬻����Ϣ
			ret = (int)response_code;
			XM_printf ("SwitchChannel resp=%d\n", ret);
		}
		else
		{
			// ��ʱ
			XM_printf ("SwitchChannel timeout\n");
			ret = -2;
		}
	}
	else if(h264codec_mode == XMSYS_H264_CODEC_MODE_IDLE)
	{
		// ����״̬
		// ֹͣ��ǰͨ����sensor�ɼ�
		XMSYS_SensorCaptureStop ();
		
		// �����µ�����ͷͨ��
		XMSYS_SensorSetCameraChannel (camera_channel);
		
		// ������ǰͨ����sensor�ɼ�
		XMSYS_SensorCaptureStart ();

		OS_Unuse(&h264codec_sema);
		ret = 0;
	}
	else
	{
		// ¼��ط�ģʽ�����ģʽ
		OS_Unuse(&h264codec_sema);
		XM_printf ("playback or idle mode reject switch channel command\n");
		ret = -1;
	}	
	
	return ret;
}

// ����ָ����¼���¼
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
#if 0
int XMSYS_H264CodecPlayerStart (const char *lpVideoFile, unsigned int playback_mode, unsigned int start_point)
{
	char response_code;
	int ret = -1;
	
	XM_printf ("PlayerStart\n");
	if(lpVideoFile)
	{
		if(*lpVideoFile == 0)
		{
			XM_printf ("PlayerStart NG, null file\n");
			return -1;
		}
		
		if(strlen(lpVideoFile) > XMSYS_H264_CODEC_MAX_FILE_NAME)
		{
			XM_printf ("Video File Name (%s) too long\n", lpVideoFile);
			return -1;
		}
	}
	
	clear_response_message ();
	
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	if(h264codec_command_param.command)
	{
		XM_printf ("command %d busy\n", h264codec_command_param.command);
		OS_Unuse(&h264codec_sema);
		return -1;
	}
	OS_Unuse(&h264codec_sema);
	

	if(h264codec_mode == XMSYS_H264_CODEC_MODE_PLAY)
	{
		// ���ڲ�����
	}
	else if(h264codec_mode == XMSYS_H264_CODEC_MODE_RECORD)
	{
		// ���ڼ�¼�С���ֹ��ǰ��¼
		if(XMSYS_H264CodecRecorderStop () != 0)
		{
			XM_printf ("can't stop recorder, PlayerStart reject\n");
			return -1;
		}
		
		if (h264codec_mode != XMSYS_H264_CODEC_MODE_IDLE)
		{
			XM_printf ("h264codec_mode = %s, PlayerStart reject\n", XMSYS_H264CodecGetModeCode());
			return -1;
		}
		//OS_Delay (500);
	}
	
	
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	memset ((void *)&h264codec_command_param, 0, sizeof(h264codec_command_param));
	h264codec_command_param.command = XMSYS_H264_CODEC_COMMAND_PLAYER_START;
	if(lpVideoFile)
		strcpy ((char *)h264codec_command_param.file_name, lpVideoFile);
	h264codec_command_param.seek_time = start_point;
	h264codec_command_param.playback_mode = playback_mode;

	OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* ֪ͨ�¼� */
	OS_Unuse(&h264codec_sema);
	
	if(OS_GetMailTimed (&h264codec_mailbox, &response_code, 600) == 0)
	{
		// codec��Ӧ�𣬻����Ϣ
		ret = (int)response_code;		
		XM_printf ("PlayerStart resp=%d\n", ret);
	}
	else
	{
		// ��ʱ
		XM_printf ("PlayerStart timeout\n");
		ret = -2;
	}
	return ret;
}
#endif
// ֹͣ����Ƶ���š�
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecPlayerStop (void)
{
	int ret = -1;
	char response_code;
	clear_response_message ();
	
	XM_printf ("PlayerStop\n");
	
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	if(h264codec_command_param.command)
	{
		XM_printf ("command %d busy\n", h264codec_command_param.command);
		OS_Unuse(&h264codec_sema);
		return -1;
	}
	if(h264codec_mode == XMSYS_H264_CODEC_MODE_RECORD)
	{
		// ��Ƶ¼��ģʽ���쳣ģʽ
		OS_Unuse(&h264codec_sema);
		XM_printf ("MODE_RECORD reject PLAYER_STOP command\n");
	}
	else if(h264codec_mode == XMSYS_H264_CODEC_MODE_PLAY)
	{
		// ��Ƶ����ģʽ
		XM_printf ("PLAYER_STOP\n");
		
		h264codec_command_param.command = XMSYS_H264_CODEC_COMMAND_PLAYER_STOP;
		OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* ֪ͨ�¼� */
		OS_Unuse(&h264codec_sema);
		
		// �ȴ�H264 Codec�߳�Ӧ��
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, 600) == 0)
		{
			// codec��Ӧ��
			ret = response_code;
			XM_printf ("PlayerStop resp=%d\n", ret);
		}
		else
		{
			XM_printf ("PlayerStop timeout\n");
			ret = -2;	// ��ʱ������
		}
		
	}
	else
	{
		// ����״̬ģʽ 
		// (�ظ���PLAYER STOP�������뵽��״̬)
		OS_Unuse(&h264codec_sema);
		// Ӧ��ɹ�
		ret = 0;
	}
	
	return ret;
}

// ��ͣ��Ƶ����
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecPlayerPause (void)
{
	int ret = -1;
	char response_code;
	unsigned int ticket = XM_GetTickCount ();		
	
	clear_response_message ();
	
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	if(h264codec_command_param.command)
	{
		XM_printf ("command %d busy\n", h264codec_command_param.command);
		OS_Unuse(&h264codec_sema);
		return -1;
	}
	if(h264codec_mode != XMSYS_H264_CODEC_MODE_PLAY)
	{
		// ¼��ģʽ�����ģʽ
		OS_Unuse(&h264codec_sema);
		XM_printf ("mode %s, Player Pause reject\n", XMSYS_H264CodecGetModeCode());
	}
	else // if (h264codec_mode == XMSYS_H264_CODEC_MODE_PLAY)
	{
		XM_printf ("COMMAND_PLAYER_PAUSE\n");
		// ������¼
		h264codec_command_param.command = XMSYS_H264_CODEC_COMMAND_PLAYER_PAUSE;
		OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* ֪ͨ�¼� */
		OS_Unuse(&h264codec_sema);
				
		// �ȴ�H264 Codec�߳�Ӧ��
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
		{
			// codec��Ӧ��
			ret = response_code;
			XM_printf ("PlayerPause resp=%d\n", ret);
		}
		else
		{
			XM_printf ("PlayerPause timeout\n");
			ret = -2;	// ��ʱ������
		}		
	}
	return ret;
}


// �����������
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecPlayerForward (void)
{
	int ret = -1;
	char response_code;
	unsigned int ticket = XM_GetTickCount ();	
	
	clear_response_message ();
	
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	if(h264codec_command_param.command)
	{
		XM_printf ("command %d busy\n", h264codec_command_param.command);
		OS_Unuse(&h264codec_sema);
		return -1;
	}
	if(h264codec_mode != XMSYS_H264_CODEC_MODE_PLAY)
	{
		OS_Unuse(&h264codec_sema);
		XM_printf ("mode %s, PLAYER_FORWARD reject\n", XMSYS_H264CodecGetModeCode());
	}
	else //if(h264codec_mode == XMSYS_H264_CODEC_MODE_PLAY)
	{
		XM_printf ("PLAYER_FORWARD\n");
		// ������¼
		
		h264codec_command_param.command = XMSYS_H264_CDOEC_COMMAND_PLAYER_FORWARD;
		OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* ֪ͨ�¼� */
		OS_Unuse(&h264codec_sema);
				
		// �ȴ�H264 Codec�߳�Ӧ��
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
		{
			// codec��Ӧ��
			ret = response_code;
			XM_printf ("PlayerForward resp=%d\n", ret);
		}
		else
		{
			XM_printf ("PlayerForward timeout\n");
			ret = -2;	// ��ʱ������
		}		
	}
	return ret;
}

// �������˲���
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecPlayerBackward (void)
{
	int ret = -1;
	char response_code;
	unsigned int ticket = XM_GetTickCount ();	

	clear_response_message ();
	
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	if(h264codec_command_param.command)
	{
		XM_printf ("command %d busy\n", h264codec_command_param.command);
		OS_Unuse(&h264codec_sema);
		return -1;
	}
	if(h264codec_mode != XMSYS_H264_CODEC_MODE_PLAY)
	{
		XM_printf ("mode %s, PLAYER_BACKWARD reject\n", XMSYS_H264CodecGetModeCode());
		OS_Unuse(&h264codec_sema);
	}
	else //if(h264codec_mode == XMSYS_H264_CODEC_MODE_PLAY)
	{
		XM_printf ("PLAYER_BACKWARD\n");
		// ������¼
		h264codec_command_param.command = XMSYS_H264_CDOEC_COMMAND_PLAYER_BACKWARD;
		OS_SignalEvent (XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* ֪ͨ�¼� */
		OS_Unuse(&h264codec_sema);
				
		// �ȴ�H264 Codec�߳�Ӧ��
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
		{
			// codec��Ӧ��
			ret = response_code;
			XM_printf ("PlayerBackward resp=%d\n", ret);
		}
		else
		{
			XM_printf ("PlayerBackward timeout\n");
			ret = -2;	// ��ʱ������
		}
	}
	return ret;
}

// ��λ���ض�ʱ��λ��(time_to_play, ��λ��)
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecPlayerSeek (unsigned int time_to_play)
{
	int ret = -1;
	char response_code;
	
	clear_response_message ();
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	if(h264codec_command_param.command)
	{
		XM_printf ("command %d busy\n", h264codec_command_param.command);
		OS_Unuse(&h264codec_sema);
		return -1;
	}
	if(h264codec_mode != XMSYS_H264_CODEC_MODE_PLAY)
	{
		XM_printf ("mode %s, PLAYER_SEEK reject\n", XMSYS_H264CodecGetModeCode());
		OS_Unuse(&h264codec_sema);
	}
	else //if(h264codec_mode == XMSYS_H264_CODEC_MODE_PLAY)
	{
		XM_printf ("PLAYER_SEEK\n");
		// ������¼
		h264codec_command_param.command = XMSYS_H264_CODEC_COMMAND_PLAYER_SEEK;
		h264codec_command_param.seek_time = time_to_play;
		OS_SignalEvent (XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* ֪ͨ�¼� */
		OS_Unuse(&h264codec_sema);
				
		// �ȴ�H264 Codec�߳�Ӧ��
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
		{
			// codec��Ӧ��
			ret = response_code;
			XM_printf ("PlayerSeek resp=%d\n", ret);
		}
		else
		{
			XM_printf ("PlayerSeek timeout\n");
			ret = -2;	// ��ʱ������
		}
	}
	return ret;	
}

// һ������
// ��ʱδ����ʵ�֣��˴�����ʵ��
int XMSYS_H264CodecOneKeyProtect (void)
{
	int ret = -1;
	char response_code;
	unsigned int ticket = XM_GetTickCount ();	
	
	// ����Ƿ��ǿ���״̬
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	if(h264codec_mode == XMSYS_H264_CODEC_MODE_IDLE)
	{
		// ����״̬
		OS_Unuse(&h264codec_sema);
		XM_printf ("OneKeyProtect can't start in IDLE state\n");
		return -1;
	}
	if(h264codec_state == XMSYS_H264_CODEC_STATE_STOP)
	{
		// ֹͣ״̬
		OS_Unuse(&h264codec_sema);
		XM_printf ("OneKeyProtect can't start in STOP state\n");
		return -1;
	}
	OS_Unuse(&h264codec_sema);
	OS_SignalEvent (XMSYS_H264_CODEC_EVENT_ONE_KEY_PROTECT, &TCB_H264CodecTask); /* ֪ͨ�¼� */
	// �ȴ�H264 Codec�߳�Ӧ��
	if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
	{
		// codec��Ӧ��
		ret = response_code;
		XM_printf ("OneKeyProtect resp=%d\n", ret);
	}
	else
	{
		XM_printf ("OneKeyProtect timeout\n");
		ret = -2;	// ��ʱ������
	}
	
	return 0;
}