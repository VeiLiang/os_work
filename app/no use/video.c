//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_videoview.c
//	  视频回放窗口
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

static unsigned char h264codec_frame_rate = 30;	// 每秒25帧缺省
static volatile unsigned int video_time_size = 1*60;	// 缺省3分钟

#define	CODEC_TIMEOUT	400	// 100毫秒命令超时

static OS_EVENT	h264_scalar_event;
static OS_EVENT	h264_encode_event;

static OS_RSEMA	h264_scalar_sema;	// 互斥访问信号量，保护Scalar硬件

static OS_TASK TCB_H264CodecTask;
static OS_STACKPTR int StackH264CodecTask[XMSYS_H264CODEC_TASK_STACK_SIZE/4];          /* Task stacks */

// h264 Codec 工作模式
static int volatile h264codec_mode;
// H264 Codec状态(编码与解码器共用)
// 编码器具有STOP及WORK两种状态
// 解码器具有STOP/WORK/PAUSE三种状态
static int volatile h264codec_state;

static const char* h264codec_mode_code[] = {"IDLE", "RECORD", "PLAYBACK"};
static const char* h264codec_state_code[] = {"STOP", "WORK", "PAUSE"};

// 只保留一个命令字
static volatile H264CODEC_COMMAND_PARAM h264codec_command_param;

static OS_RSEMA h264codec_sema;			// 互斥保护
static OS_MAILBOX h264codec_mailbox;		// 一字节邮箱。
static char h264codec_mailbuf;					// 一个字节的邮箱


// codec返回应答码给服务请求者
static void reply_response_message (char response_code)
{
	// 清除信箱的内容，防止阻塞
	OS_ClearMB (&h264codec_mailbox);
	OS_PutMail1 (&h264codec_mailbox, &response_code);
}

// 清除缓存的应答消息
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

// 仅支持一个摄像头工作
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

// 返回“视频记录分段时间长度”选项的时间值(秒)
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
		// 记录达到分段时限
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

// 返回值
// 0		-->	继续播放
//	1		-->	播放退出
//	2		-->	前进
//	3		-->	后退
// 5     -->   回放定位(秒)
int XMSYS_H264DecoderCheckEvent (struct H264DecodeStream *stream)
{
	char		file_name[XMSYS_H264_CODEC_MAX_FILE_NAME + 1];
	u8_t event;
	int ret = 0;
	stream->curr_ticket += 33;
		
	// 检查是否存在外部事件
	event = OS_GetEventsOccurred (&TCB_H264CodecTask);
	
	// 检查一键保护事件
	if(event & XMSYS_H264_CODEC_EVENT_ONE_KEY_PROTECT)
	{
		// 一键保护请求
		char response_code = (char)(-1);
		// 过滤事件
		OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_ONE_KEY_PROTECT);
		// 一键保护请求 (允许回放中将某个视频锁定)
		// 文件保存使能
		if(stream->file_lock == 0)
		{
			int lock = AP_VideoItemLockVideoFileFromFileName (stream->file_name[0]);	// 当前只使用一个通道
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
			// 文件已保护
			response_code = 0;
		}
		// 应答客户端
		reply_response_message (response_code);
	}
	
	if(event & XMSYS_H264_CODEC_EVENT_COMMAND)
	{
		int command;
		H264CODEC_COMMAND_PARAM command_param;
		// 获取命令
		OS_Use(&h264codec_sema); /* Make sure nobody else uses */
		command = h264codec_command_param.command;
		memcpy (&command_param, (void *)&h264codec_command_param, sizeof(h264codec_command_param));
		OS_Unuse(&h264codec_sema);
		
		// 命令解析
		if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_START)
		{
			// 播放开始命令
			// 检查新的播放文件是否与当前播放文件一致
			if(!strcmp (stream->file_name[0], command_param.file_name))
			{
				// 相同文件名
				// 应答客户端
				reply_response_message (0);
				clear_codec_command ();
				OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
				ret = 0;
			}
			else
			{
				// 不同文件名
				XM_printf ("playback end to play different video\n");
				// 退出播放过程
				ret = 1;
			}		
		}
		else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_STOP)
		{
			// 回放结束
			stream->command_player_stop = 1;
		command_player_stop:
			XM_printf ("playback end due to PLAYER_STOP\n");
			ret = 1;
		}
		else if(command == XMSYS_H264_CDOEC_COMMAND_PLAYER_FORWARD)
		{
			// 回放快进
		command_player_forword:
			stream->system_ticket = (unsigned int)(-1);
			// 清除命令event
			clear_codec_command ();
			reply_response_message (0);
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
			XM_printf ("PLAYER_FORWARD\n");
			ret = 2;
		}
		else if(command == XMSYS_H264_CDOEC_COMMAND_PLAYER_BACKWARD)
		{
			// 回放快退
		command_player_back:
			stream->system_ticket = (unsigned int)(-1);
			// 清除命令event
			clear_codec_command ();
			reply_response_message (0);
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
			XM_printf ("PLAYER_BACKWARD\n");
			ret = 3;
		}
		else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_SEEK)
		{
			// 回放定位
		command_player_seek:
			stream->system_ticket = (unsigned int)(-1);
			stream->seek_time = command_param.seek_time;
			// 清除命令事件
			clear_codec_command ();
			// 应答
			reply_response_message (0);
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
			
			XM_printf ("PLAYER_SEEK %d\n", stream->seek_time);
			ret = 5;		// 标记SEEK操作
		}
		else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_PAUSE)
		{
			// 暂停命令
			
			stream->system_ticket = (unsigned int)(-1);
			stream->playback_mode = XM_VIDEO_PLAYBACK_MODE_NORMAL;
			
			// 清除命令事件
			clear_codec_command ();
			reply_response_message (0);
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
			
			// 应答
			XM_printf ("PLAYER_PAUSE\n");
			
			// 等待其他命令事件
			while(1)
			{
				event = OS_GetEventsOccurred (&TCB_H264CodecTask);
				// 检查是否存在其他命令
				if(event & XMSYS_H264_CODEC_EVENT_COMMAND)
				{
					OS_Use(&h264codec_sema); /* Make sure nobody else uses */
					command = h264codec_command_param.command;
					strcpy (file_name, (const char *)h264codec_command_param.file_name);
					OS_Unuse(&h264codec_sema);
					if(command == XMSYS_H264_CODEC_COMMAND_RECORDER_START)
					{
						// 录像启动命令
						// 取消暂停，退出播放
						goto command_player_stop;
					}
					else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_STOP)
					{
						// 播放停止命令
						stream->command_player_stop = 1;
						// 取消暂停，退出播放
						goto command_player_stop;
					}
					else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_START)
					{
						// 播放继续命令
						// 判断播放命令的文件名是否与当前的视频文件名一致
						if(strcmp (file_name, stream->file_name[0]))
						{
							// 不同的文件名
							// 播放其他的文件
							// 退出
							goto command_player_stop;
						}
						else
						{
							// 相同的文件名，表示恢复播放
							// 取消暂停，继续播放
							// 清除命令event
							clear_codec_command ();
							reply_response_message (0);
							OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
							ret = 4;		// 表示播放恢复
							break;
						}
					}
					else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_PAUSE)
					{
						// 恢复播放
						stream->system_ticket = (unsigned int)(-1);
						clear_codec_command ();
						reply_response_message (0);
						OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
						ret = 4;		// 表示播放恢复
						break;
					}
					else if(command == XMSYS_H264_CDOEC_COMMAND_PLAYER_FORWARD)
					{
						// 回放前进
						goto command_player_forword;
					}
					else if(command == XMSYS_H264_CDOEC_COMMAND_PLAYER_BACKWARD)
					{
						// 回放后退
						goto command_player_back;
					}
					else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_SEEK)
					{
						// 回放定位
						goto command_player_seek;
					}
					else
					{
						// 非法命令，过滤
						clear_codec_command ();
						OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
					}
				}
				
				OS_Delay (2);
			}			
		}
		else
		{
			// 非法命令，过滤
			ret = 0;
			clear_codec_command ();
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
		}
		
		return ret;
	}
	// 等待直到下一次定时器事件
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
	

	// 记录启动
	while(1)
	{
		XM_GetLocalTime (&LocalTime);
		memset (&EncodeStream, 0, sizeof(EncodeStream));
			XM_printf("memset EncodeStream11\n");
			
		memset (h264name_f, 0, sizeof(h264name_f));
		memset (h264name_r, 0, sizeof(h264name_r));
		memset (h264name, 0, sizeof(h264name));
		
		hVideoItem = NULL;
		
		// 注意，现在的XM_VideoItemCreateVideoItemHandle 存在Bug，没有释放的代码
		//XM_printf ("Please resolve bugs within XM_VideoItemCreateVideoItemHandle\n");
		// 获取一个新的视频项文件名
		channel = XMSYS_SensorGetCameraChannel ();
		// 检查系统RTC时间是否已设置。ARK1930的后视镜主板没有RTC，需要外部设置RTC时间。
		if(!XM_GetLocalTime(&CurrTime))
		{
			// 若未设置，执行录像命令但不保存
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
			// 恢复初始状态
			// h264codec_mode = XMSYS_H264_CODEC_MODE_IDLE;
			// h264codec_state = XMSYS_H264_CODEC_STATE_STOP;
			// 向UI线程投递异常消息
			// 20141020 ZhuoYongHong
			// 创建临时视频文件，用于记录视频。
			// 当存储卡第一次插入时，某些情况下获取卡文件系统容量会持续较长时间(10秒~30秒)，导致视频项数据库暂时无法使用，
			// 而导致无法记录视频。
			// 为解决上述问题，创建一个临时视频记录文件，用于保存实时视频。
			if(XM_VideoItemCreateVideoTempFile (channel, h264name_f, sizeof(h264name_f)))
			{
				// 创建一个临时视频文件成功
				strcpy (EncodeStream.file_name[0], h264name_f);
				EncodeStream.file_save = 1;
				//EncodeStream.file_temp = 1;	// 标记是临时视频项文件
			}
			else
			{
				// 创建一个逻辑视频文件(无法写入)
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
				// 保存文件名
				strcpy (EncodeStream.file_name[0], h264name_f);
				EncodeStream.file_save = 1;
			}
			else
				goto create_video_item_failed;
		}

		// 设置当前使用的分辨率
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
		// 设置视频记录时间长度
		//XM_printf ("VideoTimeSize=%d\n", AP_GetVideoTimeSize());
		if(EncodeStream.file_save)
			EncodeStream.stop_ticket = XM_GetTickCount() + 1000 * AP_GetVideoTimeSize();	
		else
			EncodeStream.stop_ticket = XM_GetTickCount() + 1000 * 30;	
		//XM_printf ("h264_encoder_double_stream start_ticket=%d, stop_ticket=%d\n", EncodeStream.curr_ticket, EncodeStream.stop_ticket);

		//XM_printf ("%d Enter %s\n",  XM_GetTickCount(), h264name_f);
		
		// 启动H264编码，等待返回(外部命令、记录超时等事件)
	//	h264_encode_avi_stream (h264name_f, NULL, &EncodeStream, NULL, NULL);
		XM_printf ("%d Leave\n",  XM_GetTickCount());
		
		// 检查是否是临时视频项文件
//		if(EncodeStream.file_temp)
		{
			// 临时视频项
		}
				
		
		// 检查退出原因
		
	//	if(event & XMSYS_H264_CODEC_EVENT_COMMAND)
		{
			// 外部命令
			XM_printf ("Exit Codec Loop\n");
			
			// 等待视频句柄关闭
			if(hVideoItem)
				XM_VideoItemWaitingForUpdate (EncodeStream.file_name[0]);
			break;
		}
					
		// 记录文件超时退出，开始下一个录像记录
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


// H264编解码任务
void XMSYS_H264CodecTask (void)
{
	OS_U8 h264_event;		// 外部事件

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
			h264codec_command_param.command = 0;		// 清除命令
			// 清除命令
			OS_Unuse(&h264codec_sema); /* Make sure nobody else uses */

			switch (command.command)
			{
				case XMSYS_H264_CODEC_COMMAND_RECORDER_START:
					XM_printf ("XMSYS_H264_CODEC_COMMAND_RECORDER_START\n");
					OS_Use(&h264codec_sema); /* Make sure nobody else uses */
					h264codec_mode = XMSYS_H264_CODEC_MODE_RECORD;
					h264codec_state = XMSYS_H264_CODEC_STATE_WORK;
					OS_Unuse(&h264codec_sema); 
					// 应答UI线程
					ret = 0;
					OS_PutMail1 (&h264codec_mailbox, &ret);	
					clear_codec_command ();
					XMSYS_SensorCaptureStart ();

					// 启动h264 codec过程
					h264codec_RecorderStart ();
					break;

				case XMSYS_H264_CODEC_COMMAND_RECORDER_STOP:

					XM_printf ("XMSYS_H264_CODEC_COMMAND_RECORDER_STOP\n");
					//XMSYS_SensorCaptureStop ();
					OS_Use(&h264codec_sema); /* Make sure nobody else uses */
					h264codec_mode = XMSYS_H264_CODEC_MODE_IDLE;
					h264codec_state = XMSYS_H264_CODEC_STATE_STOP;
					OS_Unuse(&h264codec_sema); 


					// 应答UI线程
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
					// 当前仅支持一个流同步解码
					strcpy (dec_stream.file_name[0], (char *)command.file_name);
					// 应答UI线程
					ret = 0;
					OS_PutMail1 (&h264codec_mailbox, &ret);	//

					// 启动播放
					XM_printf ("decode_avi %s\n", file_name);

					// 打开音频
					waveFormat.wChannels = 1;
					waveFormat.wBitsPerSample = 16;
					waveFormat.dwSamplesPerSec = 8000;

					// 将指定文件名的视频标记使用中，防止删除操作将视频项删除
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

					// 向UI线程发送播放结束消息
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

					// 应答UI线程
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
			// 切换录像通道
		}
	}

}


void XMSYS_H264CodecInit (void)
{
	// H264 Codec Cache文件初始化
	//XMSYS_H264FileInit ();
	
	h264codec_mode = XMSYS_H264_CODEC_MODE_IDLE;
	h264codec_state = XMSYS_H264_CODEC_STATE_STOP;
	
	OS_CREATERSEMA(&h264codec_sema); /* Creates resource semaphore */
	
	OS_CREATEMB (&h264codec_mailbox, 1, 1, &h264codec_mailbuf);	
	
	// 中断事件
	//irq_unmask(T18XX_H264_INT);
	OS_EVENT_Create (&h264_scalar_event);
	// 专用于H264 编码器
	OS_EVENT_Create (&h264_encode_event);
	
	// Scalar互斥访问
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

// 获取当前H264 Codec的工作模式
int XMSYS_H264CodecGetMode (void)
{
	return h264codec_mode;
}

const char* XMSYS_H264CodecGetModeCode (void)
{
	return h264codec_mode_code[h264codec_mode];
}

// 获取H264 Codec当前的工作状态
int XMSYS_H264CodecGetState (void)
{
	return h264codec_state;
}

const char* XMSYS_H264CodecGetStateCode (void)
{
	return h264codec_state_code[h264codec_state];
}

// 停止录像记录
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
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
		// 启动记录
		h264codec_command_param.command = XMSYS_H264_CODEC_COMMAND_RECORDER_STOP;
		OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* 通知事件 */
		OS_Unuse(&h264codec_sema);
		
		// 等待H264 Codec线程应答
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, 1000) == 0)
		{
			// codec已应答，获得消息
			ret = (int)response_code;
			XM_printf ("RecorderStop resp=%d\n", ret);
			
			// 延时1秒，等待codec停止
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
			// 超时
			XM_printf ("RecorderStop timeout\n");
			ret = -2;
		}
	}
	else
	{
		// 空闲状态模式 
		// (重复的RECORDER STOP命令会进入到该状态)
		OS_Unuse(&h264codec_sema);
		XM_printf ("RecorderStop, idle\n");
		// 应答成功
		ret = 0;
	}
	return ret;
}

// 切换摄像头通道
// 返回值
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecRecorderSwitchChannel  (unsigned int camera_channel)
{
	int ret = -1;
	char response_code = -1;
	
	if(camera_channel >= 2)
	//if(camera_channel >= 1)		// 当前版本仅支持前置摄像头录像
	{
		XM_printf ("Can't switch to camera channel (%d)\n",  camera_channel);
		return ret;
	}
	
	clear_response_message ();
	
	// 检查摄像头通道是否与现在一致
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
		// 设置新的摄像头通道
		XMSYS_SensorSetCameraChannel (camera_channel);
		// 触发事件
		OS_SignalEvent(XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL, &TCB_H264CodecTask); /* 通知事件 */
		OS_Unuse(&h264codec_sema);
		
		// 需要等待codec的应答
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
		{
			// codec已应答，获得消息
			ret = (int)response_code;
			XM_printf ("SwitchChannel resp=%d\n", ret);
		}
		else
		{
			// 超时
			XM_printf ("SwitchChannel timeout\n");
			ret = -2;
		}
	}
	else if(h264codec_mode == XMSYS_H264_CODEC_MODE_IDLE)
	{
		// 空闲状态
		// 停止当前通道的sensor采集
		XMSYS_SensorCaptureStop ();
		
		// 设置新的摄像头通道
		XMSYS_SensorSetCameraChannel (camera_channel);
		
		// 启动当前通道的sensor采集
		XMSYS_SensorCaptureStart ();

		OS_Unuse(&h264codec_sema);
		ret = 0;
	}
	else
	{
		// 录像回放模式或空闲模式
		OS_Unuse(&h264codec_sema);
		XM_printf ("playback or idle mode reject switch channel command\n");
		ret = -1;
	}	
	
	return ret;
}

// 播放指定的录像记录
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
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
		// 正在播放中
	}
	else if(h264codec_mode == XMSYS_H264_CODEC_MODE_RECORD)
	{
		// 正在记录中。终止当前记录
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

	OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* 通知事件 */
	OS_Unuse(&h264codec_sema);
	
	if(OS_GetMailTimed (&h264codec_mailbox, &response_code, 600) == 0)
	{
		// codec已应答，获得消息
		ret = (int)response_code;		
		XM_printf ("PlayerStart resp=%d\n", ret);
	}
	else
	{
		// 超时
		XM_printf ("PlayerStart timeout\n");
		ret = -2;
	}
	return ret;
}
#endif
// 停止“视频播放”
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
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
		// 视频录像模式，异常模式
		OS_Unuse(&h264codec_sema);
		XM_printf ("MODE_RECORD reject PLAYER_STOP command\n");
	}
	else if(h264codec_mode == XMSYS_H264_CODEC_MODE_PLAY)
	{
		// 视频播放模式
		XM_printf ("PLAYER_STOP\n");
		
		h264codec_command_param.command = XMSYS_H264_CODEC_COMMAND_PLAYER_STOP;
		OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* 通知事件 */
		OS_Unuse(&h264codec_sema);
		
		// 等待H264 Codec线程应答
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, 600) == 0)
		{
			// codec已应答
			ret = response_code;
			XM_printf ("PlayerStop resp=%d\n", ret);
		}
		else
		{
			XM_printf ("PlayerStop timeout\n");
			ret = -2;	// 超时错误码
		}
		
	}
	else
	{
		// 空闲状态模式 
		// (重复的PLAYER STOP命令会进入到该状态)
		OS_Unuse(&h264codec_sema);
		// 应答成功
		ret = 0;
	}
	
	return ret;
}

// 暂停视频播放
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
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
		// 录像模式或空闲模式
		OS_Unuse(&h264codec_sema);
		XM_printf ("mode %s, Player Pause reject\n", XMSYS_H264CodecGetModeCode());
	}
	else // if (h264codec_mode == XMSYS_H264_CODEC_MODE_PLAY)
	{
		XM_printf ("COMMAND_PLAYER_PAUSE\n");
		// 启动记录
		h264codec_command_param.command = XMSYS_H264_CODEC_COMMAND_PLAYER_PAUSE;
		OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* 通知事件 */
		OS_Unuse(&h264codec_sema);
				
		// 等待H264 Codec线程应答
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
		{
			// codec已应答
			ret = response_code;
			XM_printf ("PlayerPause resp=%d\n", ret);
		}
		else
		{
			XM_printf ("PlayerPause timeout\n");
			ret = -2;	// 超时错误码
		}		
	}
	return ret;
}


// 启动快进播放
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
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
		// 启动记录
		
		h264codec_command_param.command = XMSYS_H264_CDOEC_COMMAND_PLAYER_FORWARD;
		OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* 通知事件 */
		OS_Unuse(&h264codec_sema);
				
		// 等待H264 Codec线程应答
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
		{
			// codec已应答
			ret = response_code;
			XM_printf ("PlayerForward resp=%d\n", ret);
		}
		else
		{
			XM_printf ("PlayerForward timeout\n");
			ret = -2;	// 超时错误码
		}		
	}
	return ret;
}

// 启动快退播放
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
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
		// 启动记录
		h264codec_command_param.command = XMSYS_H264_CDOEC_COMMAND_PLAYER_BACKWARD;
		OS_SignalEvent (XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* 通知事件 */
		OS_Unuse(&h264codec_sema);
				
		// 等待H264 Codec线程应答
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
		{
			// codec已应答
			ret = response_code;
			XM_printf ("PlayerBackward resp=%d\n", ret);
		}
		else
		{
			XM_printf ("PlayerBackward timeout\n");
			ret = -2;	// 超时错误码
		}
	}
	return ret;
}

// 定位到特定时间位置(time_to_play, 单位秒)
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
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
		// 启动记录
		h264codec_command_param.command = XMSYS_H264_CODEC_COMMAND_PLAYER_SEEK;
		h264codec_command_param.seek_time = time_to_play;
		OS_SignalEvent (XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* 通知事件 */
		OS_Unuse(&h264codec_sema);
				
		// 等待H264 Codec线程应答
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
		{
			// codec已应答
			ret = response_code;
			XM_printf ("PlayerSeek resp=%d\n", ret);
		}
		else
		{
			XM_printf ("PlayerSeek timeout\n");
			ret = -2;	// 超时错误码
		}
	}
	return ret;	
}

// 一键保护
// 暂时未完整实现，此处仅简单实现
int XMSYS_H264CodecOneKeyProtect (void)
{
	int ret = -1;
	char response_code;
	unsigned int ticket = XM_GetTickCount ();	
	
	// 检查是否是空闲状态
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	if(h264codec_mode == XMSYS_H264_CODEC_MODE_IDLE)
	{
		// 空闲状态
		OS_Unuse(&h264codec_sema);
		XM_printf ("OneKeyProtect can't start in IDLE state\n");
		return -1;
	}
	if(h264codec_state == XMSYS_H264_CODEC_STATE_STOP)
	{
		// 停止状态
		OS_Unuse(&h264codec_sema);
		XM_printf ("OneKeyProtect can't start in STOP state\n");
		return -1;
	}
	OS_Unuse(&h264codec_sema);
	OS_SignalEvent (XMSYS_H264_CODEC_EVENT_ONE_KEY_PROTECT, &TCB_H264CodecTask); /* 通知事件 */
	// 等待H264 Codec线程应答
	if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
	{
		// codec已应答
		ret = response_code;
		XM_printf ("OneKeyProtect resp=%d\n", ret);
	}
	else
	{
		XM_printf ("OneKeyProtect timeout\n");
		ret = -2;	// 超时错误码
	}
	
	return 0;
}