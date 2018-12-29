//****************************************************************************
//
//	Copyright (C) 2013 Shenzhen exceedspace
//
//	Author	ZhuoYongHong
//
//	File name: xm_uvc_socket.h
//
//	Revision history
//
//		2013.5.23 ZhuoYongHong Initial version
//
//****************************************************************************
#include <hardware.h>
#include "RTOS.h"		// OSͷ�ļ�
#include "fs.h"
#include <stdio.h>
#include <string.h>
#include "xm_core.h"
#include "xm_uvc_socket.h"
#include "xm_systemupdate.h"
#include <assert.h>
#include "xm_app_menudata.h"
#include "xm_videoitem.h"
#include "xm_h264_codec.h"
#include "xm_file_manager_task.h"
#include "xm_user.h"
#include "CdrRequest.h"
#include "xm_dev.h"
#include "xm_hybrid_stream.h"
#include "xm_video_task.h"

int XMSYS_CdrSendResponse (unsigned int packet_type, unsigned char command_id, 
									unsigned char *command_packet_buffer, unsigned int command_packet_length);
extern void XMSYS_CameraSetOutputImageSize (XMSIZE size);

//#define	MESSAGE_DEBUG

#ifdef MESSAGE_DEBUG
#define	MESSAGE_PRINT(fmt, args...)		printf(fmt, ## args)	
#else
#define	MESSAGE_PRINT(fmt, args...)	
#endif

#define	MAX_UVC_RESPONSE_PACKET_SIZE		(65536)

static volatile int uvc_socket_inited = 0;

static unsigned char uvc_socket_message_buffer[MAX_UVC_RESPONSE_PACKET_SIZE];

static OS_RSEMA uvc_message_socket_access_sema;		// socket�ڲ����ݻ�������ź���

static int uvc_socket_enabled = 1;

#define	ACK		0		// �������ִ��
#define	NAK		1		// ��������ܾ�ִ��
#define	TIMEOUT	2		// �������ʱ

static const char * resp_code[] = {"ACK", "NAK", "TIMEOUT"};
static const char * type_code[] = {"VIDEO", "IMAGE"};
static const char * ch_code[] = {"FORE", "REAR"};

void  XMSYS_UvcSocketInit (void)
{
	uvc_socket_inited = 0;
	OS_CREATERSEMA (&uvc_message_socket_access_sema);
}

void  XMSYS_UvcSocketExit (void)
{
	OS_DeleteRSema (&uvc_message_socket_access_sema);

	uvc_socket_inited = 0;
}

// ���socketͨ���Ƿ��ѽ���
int  XMSYS_UvcSocketCheckConnection (void)
{
	return uvc_socket_inited;
}

void XMSYS_UvcSocketSetEnable (int enable)
{
	uvc_socket_enabled = enable;
	MESSAGE_PRINT ("\nUvcSocket %s\n", enable ? "enabled" : "disabled"); 
}

static const char* aysn_code[] = {
	/*  0 */	"NID_IMAGE_OUTPUT",
	/*  1 */	"NID_PLAYBACK_FINISH",
	/*  2 */	"NID_SDCARD_CAPACITY",
	/*  3 */	"NID_SDCARD_FORMAT_FINISH",
	/*  4 */	"NID_FILE_DELETE_FINISH",
	/*  5 */	"NID_SDCARD_PLUGOUT",
	/*  6 */	"NID_SDCARD_INSERT",
	/*  7 */	"NID_SDCARD_DISKFULL",
	/*  8 */	"NID_SDCARD_DAMAGE",
	/*  9 */	"NID_FSERROR",
	/* 10 */	"NID_SDCARD_INVALID",
	/* 11 */	"NID_FILE_LIST_UPDATE",
	/* 12 */	"NID_SYSTEM_UPDATE_PACKAGE",
	/* 13 */	"NID_SYSTEM_UPDATE_STEP",
	/* 14 */	"NID_TOO_MORE_LOCKED_RESOURCE",
	/* 15 */	"NID_PLAYBACK_PROGRESS",
	/* 16 */ 	"NID_FILELIST",
	/* 17 */ 	"NID_DOWNLOAD",
	/* 18 */ 	"NID_LOW_PREFORMANCE",
	""
};


// ����"ͬ��Ӧ���"����"�첽��Ϣ��"
int  XMSYS_UvcSocketTransferResponsePacket (unsigned int socket_type,  
												  				unsigned char command_id,			// �����ID
												  				unsigned char *command_packet_buffer,
												  				unsigned int command_packet_length
															)
{
	unsigned short packet_type;
	int ret = -1;
	int loop;
		
	// ���������ź���, ��ֹ��ͬ��Ӧ����������롰�첽��Ϣ�������͵Ĳ�������
	OS_Use (&uvc_message_socket_access_sema); /* Make sure nobody else uses */
	
	if(socket_type == XMSYS_UVC_SOCKET_TYPE_ASYC)
	{
		MESSAGE_PRINT ("%s\n", aysn_code[command_id]);
	}

	do
	{
		if(socket_type >= XMSYS_UVC_SOCKET_TYPE_COUNT)
		{
			MESSAGE_PRINT ("illegal message socket type(%d)\n", socket_type);
			break;
		}
		
		if(socket_type == XMSYS_UVC_SOCKET_TYPE_SYNC)
			packet_type = XMSYS_UVC_SOCKET_TYPE_ID_SYNC;
		else
			packet_type = XMSYS_UVC_SOCKET_TYPE_ID_ASYC;
		
		ret = XMSYS_CdrSendResponse (packet_type, command_id, command_packet_buffer, command_packet_length);
		
	}	while (0);
	
	// ����������
	OS_Unuse (&uvc_message_socket_access_sema);
	
	return ret;
}

int PostMediaFileListMessage (int ch, int type)
{
	int ret = -1;
	char *message = NULL;
	char *buffer = NULL;
	int length = 0;
	int size;
	do
	{
		// ���CDR(UVC)�Ƿ��Ѵ���
		if(XMSYS_CameraIsReady() == 0)
			break;
		
		length = 0x20000;
		message = kernel_malloc (length);
		if(message == NULL)
		{
			length /= 2;
			message = kernel_malloc (length);
			if(message == NULL)
			{
				XM_printf ("PostMediaFileListMessage failed, memory busy\n");
				break;
			}
		}
		
		buffer = message;
	
		// ��ȡ��Ƶ�б�
		if(type == 0 || type == 0x07)
		{
			// ͨ��0����Ƶ�б�
			if(ch == 0 || ch == 0x0F)
			{
				size = XM_VideoItemGetVideoItemList (0, 0, 1024, buffer, length);
				if(size > 0)
				{
					buffer += size;
					length -= size;
				}
			}
			// ͨ��1����Ƶ�б�
			if(ch == 1 || ch == 0x0F)
			{
				size = XM_VideoItemGetVideoItemList (1, 0, 1024, buffer, length);
				if(size > 0)
				{
					buffer += size;
					length -= size;
				}
			}
			
			//size = XM_VideoItemGetVideoItemList (ch, 0, 1024, message, 0x10000);
		}
		
		// ��ȡ��Ƭ�б�
		if(type == 1 || type == 0x07)
		{
			// ͨ��0����Ƭ�б�
			if(ch == 0 || ch == 0x0F)
			{
				size = XM_VideoItemGetImageItemList (0, 0, 1024, buffer, length);
				if(size > 0)
				{
					buffer += size;
					length -= size;					
				}
			}
			// ͨ��1����Ƭ�б�
			if(ch == 1 || ch == 0x0F)
			{
				size = XM_VideoItemGetImageItemList (1, 0, 1024, buffer, length);
				if(size > 0)
				{
					buffer += size;
					length -= size;					
				}
			}
			
			// size = XM_VideoItemGetImageItemList (ch, 0, 1024, message, 0x10000);
		}
		
		// ����Ƿ������Ч�б���Ҫ����
		if(buffer != message)
		{
			ret = XMSYS_CdrInsertAsycMessage (XMSYS_UVC_NID_FILELIST, message, buffer - message);
			if(ret == 0)
			{
				XM_printf ("FILELIST Prepared\n");
			}
		}
	} while (0);
	
	if(message)
		kernel_free (message);
	
	return ret;
}

// ��ȡ��״̬��
// һ�ֽڳ��ȣ�
// 0x01 --> SD���γ�
// 0x02 --> SD������
// 0x03 --> SD���ļ�ϵͳ����
// 0x04 --> SD���޷�ʶ��
int XMSYS_UvcSocketGetCardState (void)
{
	int ret;
	unsigned int card_state = XM_GetFmlDeviceCap (DEVCAP_SDCARDSTATE);
	if(card_state == DEVCAP_SDCARDSTATE_UNPLUG)
		ret = 1;
	else if(card_state == DEVCAP_SDCARDSTATE_INSERT)
		ret = 2;
	else if(card_state == DEVCAP_SDCARDSTATE_FS_ERROR)
		ret = 3;
	else //if(card_state == DEVCAP_SDCARDSTATE_INVALID)
		ret = 4;
	return ret;
}

// �������յ�����������������������������������
// 0 	�ɹ�
// -1 ʧ��
int  XMSYS_UvcSocketReceiveCommandPacket (
															 unsigned char command_id,					// �������ʶ
															 unsigned char *command_packet_buffer,
															 unsigned int command_packet_length
															)
{
	int i;
	char file_name[64];		// ����ý���ļ���ȫ·���ļ���
	int ret_code;
		
	unsigned char ret;
	
	if(uvc_socket_enabled == 0)
	{
		MESSAGE_PRINT ("\nSocket Disabled, Reject command (%d)\n", command_id);
		return -1;
	}
	
	//MESSAGE_PRINT ("\n%d, receive command (%d), ", XM_GetTickCount(), command_id);
	for (i = 0; i < command_packet_length; i++)
	{
		//MESSAGE_PRINT ("%02x ", command_packet_buffer[i]);
	}
	//MESSAGE_PRINT ("\n");
	
	

	
	ret = NAK;		// NAK
	switch (command_id)
	{
		case XMSYS_UVC_CID_GET_SYSTEM_VERSION:		// ��ȡϵͳ�汾��
		{
			int ret_code;
			unsigned int version;
			MESSAGE_PRINT ("XMSYS_UVC_CID_GET_SYSTEM_VERSION\n");
			version = 0xFFFFFFFF;
			XMSYS_GetSystemSoftwareVersion ((unsigned char *)&version);
			MESSAGE_PRINT ("version=0x%08x\n", version);
			ret_code =  XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_SYNC,
																		command_id, 
																		(unsigned char *)&version,
																		4
																		);
			MESSAGE_PRINT ("XMSYS_UVC_CID_GET_SYSTEM_VERSION %d\n", ret_code);
			return ret_code;
		}
		
		case XMSYS_UVC_CID_SHUTDOWN:
		{
			int loop = 3;
			MESSAGE_PRINT ("XMSYS_UVC_CID_SHUTDOWN\n");
			//if(command_packet_length != 1)
			{
				//MESSAGE_PRINT ("XMSYS_UVC_CID_SHUTDOWN's command packet length(%d) != 1\n", command_packet_length);
				//break;
			}
			unsigned char cmd = *command_packet_buffer;
			// ֹͣ¼��򲥷�
			MESSAGE_PRINT ("%d, CodecStop\n", XM_GetTickCount());
			XMSYS_H264CodecStop ();
			MESSAGE_PRINT ("%d, xm_close_volume_service\n", XM_GetTickCount());
			xm_close_volume_service ("mmc:0:");
			
			ret = ACK; 
			XMSYS_FileManagerSystemShutDown (cmd);
			break;	
		}
			
		case XMSYS_UVC_CID_DATETIME_SET:		// ʱ������
		{
			// 4�ֽڳ��� ��ʱ���ʾ��
			//
			//	YY --> 0 ~ 99  (��ȥ2000)
			//	MM --> 1 ~ 12
			//	DD --> 1 ~ 31
			//	hh --> 0 ~ 23
			//	mm --> 0 ~ 59
			//	ss --> 0 ~ 59			
			//	
			XMSYSTEMTIME local_time;
			MESSAGE_PRINT ("CID_DATETIME_SET\n");
			do
			{
				if(command_packet_length != 6)
				{
					MESSAGE_PRINT ("CID_DATETIME_SET's command packet length(%d) != 6\n", command_packet_length);
					break;
				}
				memset (&local_time, 0, sizeof(local_time));
				local_time.bSecond = command_packet_buffer[5];
				if(local_time.bSecond >= 60)
				{
					MESSAGE_PRINT ("Second (%d) >= 60\n", local_time.bSecond);
					break;
				}
				local_time.bMinute = command_packet_buffer[4];
				if(local_time.bMinute >= 60)
				{
					MESSAGE_PRINT ("Minute (%d) >= 60\n", local_time.bMinute);
					break;
				}
				local_time.bHour = command_packet_buffer[3];
				if(local_time.bHour >= 24)
				{
					MESSAGE_PRINT ("Hour (%d) >= 24\n", local_time.bHour);
					break;
				}
				local_time.bDay = command_packet_buffer[2];
				if(local_time.bDay < 1 || local_time.bDay > 31)
				{
					MESSAGE_PRINT ("Day (%d) <> 1 ~ 31\n", local_time.bDay);
					break;
				}
				local_time.bMonth = command_packet_buffer[1];
				if(local_time.bMonth < 1 || local_time.bMonth > 12)
				{
					MESSAGE_PRINT ("Month (%d) <> 1 ~ 12\n", local_time.bMonth);
					break;
				}
				local_time.wYear = command_packet_buffer[0];
				local_time.wYear = (unsigned short)(local_time.wYear + 2000);
				if(local_time.wYear > 2050)
				{
					MESSAGE_PRINT ("Year (%d) <= 2050\n", local_time.wYear);
					break;
				}
				ret = ACK;
				XM_SetLocalTime (&local_time);
			} while (0);
			
			MESSAGE_PRINT ("DATETIME_SET %s\n", resp_code[ret]);
			break;
		}
		
		// ¼��ֶ�ʱ�䳤������
		case XMSYS_UVC_CID_VIDEO_SEGMENT_LENGTH_SET:
		{
			// LL ��1�ֽڳ��ȣ�
			//	LL --> 0x02��2���ӣ�, 
			//	LL --> 0x03��3���ӣ�, 
			//	LL --> 0x05��5���ӣ�,
			//	����ȡֵ�Ƿ�
			unsigned char ll = 0xFF;
			MESSAGE_PRINT ("CID_VIDEO_SEGMENT_LENGTH_SET\n");
			do
			{
				if(command_packet_length != 1)
				{
					MESSAGE_PRINT ("CID_VIDEO_SEGMENT_LENGTH_SET's command packet length(%d) != 1\n", command_packet_length);
					break;
				}
				ll = *command_packet_buffer;
				if(ll != 1 && ll != 2 && ll != 3 && ll != 5)
				{
					MESSAGE_PRINT ("time length(%d) != {1, 2, 3, 5}\n", ll);
					break;
				}
				ret = ACK;
				AP_SetMenuItem (APPMENUITEM_VIDEO_TIME_SIZE, ll);
			} while (0);
			MESSAGE_PRINT ("VIDEO_SEGMENT_LENGTH(%d) %s\n", ll, resp_code[ret]);
			break;
		}
		
		// ¼���������ر�
		case XMSYS_UVC_CID_MIC_SET:
		{
			// MM ��1�ֽڳ��ȣ�
			// MM --> 0x00(¼���ر�)��
			// MM --> 0x01(¼������),
			// ����ȡֵ�Ƿ�
			const char *mic_code[] = {"OFF", "ON"};
			unsigned char MM = 0xFF;
			MESSAGE_PRINT ("CID_MIC_SET\n");
			do
			{
				if(command_packet_length != 1)
				{
					MESSAGE_PRINT ("XMSYS_UVC_CID_MIC_SET's command packet length(%d) != 1\n", command_packet_length);
					break;
				}
				MM = *command_packet_buffer;
				if(MM >= 2)
				{
					MESSAGE_PRINT ("MIC setting(%d) != {0, 1}\n", MM); 
					break;
				}
				ret = ACK;
				AP_SetMenuItem (APPMENUITEM_MIC, MM);
			} while (0);
			if(MM <= 1)
			{
				MESSAGE_PRINT ("MIC_SET(%s) (%s)\n", mic_code[MM], resp_code[ret]);
			}
			else
			{
				MESSAGE_PRINT ("MIC_SET(%d) (%s)\n", MM, resp_code[ret]);
			}
			break;
		}
		
		// ��ȡ¼������״̬
		case XMSYS_UVC_CID_MIC_GET:
		{
			const char *mic_code[] = {"OFF", "ON"};
			MESSAGE_PRINT ("CID_MIC_GET\n");
			// ���Ƿ���¼��״̬
			ret = (char)AP_GetMenuItem(APPMENUITEM_MIC);
			if(ret <= 1)
			{
				MESSAGE_PRINT ("MIC_GET(%s)\n", mic_code[ret]);
			}
			else
			{
				MESSAGE_PRINT ("MIC_GET(%d)\n", ret);
			}
			break;
		}
		
		// ��Ƶ����������ر�
		// 0 --> �ر�
		// 1 --> ����
		case XMSYS_UVC_CID_VIDEO_OUTPUT_SET_ONOFF:
		{
			unsigned char onoff = 0xFF;
			MESSAGE_PRINT ("XMSYS_UVC_CID_VIDEO_OUTPUT_SET_ONOFF\n");
			do
			{
				if(command_packet_length != 1)
				{
					MESSAGE_PRINT ("XMSYS_UVC_CID_VIDEO_OUTPUT_SET_ONOFF's command packet length(%d) != 1\n", command_packet_length);
					break;
				}
				onoff = *command_packet_buffer;
				if(onoff >= 2)
				{
					MESSAGE_PRINT ("ONOFF setting(%d) != {0, 1}\n", onoff); 
					break;
				}
				ret = ACK;
			} while (0);
			MESSAGE_PRINT ("XMSYS_UVC_CID_VIDEO_OUTPUT_SET_ONOFF(%d) (%s)\n", onoff, resp_code[ret]);
			break;
		}
		
		// ������������
		// "MUTE ��1�ֽڳ��ȣ�
		// MUTE --> 0x01(������������)��MUTE --> 0x00(���������ر�), ����ȡֵ�Ƿ�"
		case XMSYS_UVC_CID_AUDIO_MUTE:
		{
			extern void XM_WaveSetDacMute (int Mute);

			unsigned char audio_mute = 0xFF;
			MESSAGE_PRINT ("XMSYS_UVC_CID_AUDIO_MUTE\n");
			do
			{
				if(command_packet_length != 1)
				{
					MESSAGE_PRINT ("XMSYS_UVC_CID_AUDIO_MUTE's command packet length(%d) != 1\n", command_packet_length);
					break;
				}
				audio_mute = *command_packet_buffer;
				if(audio_mute >= 2)
				{
					MESSAGE_PRINT ("AUDIO_MUTE setting(%d) != {0, 1}\n", audio_mute); 
					break;
				}
				ret = ACK;
				XM_WaveSetDacMute (audio_mute);
			} while (0);
			MESSAGE_PRINT ("XMSYS_UVC_CID_AUDIO_MUTE(%d) (%s)\n", audio_mute, resp_code[ret]);
			break;			
		}
		
		case XMSYS_UVC_CID_AUDIO_VOLUME:
		{
			extern void XM_WaveSetDacVolume (unsigned int volume);
			unsigned char audio_vol = 20;
			MESSAGE_PRINT ("XMSYS_UVC_CID_AUDIO_VOLUME\n");
			do
			{
				if(command_packet_length != 1)
				{
					MESSAGE_PRINT ("XMSYS_UVC_CID_AUDIO_VOLUME's command packet length(%d) != 1\n", command_packet_length);
					break;
				}
				audio_vol = *command_packet_buffer;
				if(audio_vol > 20)
				{
					MESSAGE_PRINT ("AUDIO_VOLUME setting(%d) != {0 ~ 20}\n", audio_vol); 
					break;
				}
				ret = ACK;
				XM_WaveSetDacVolume (audio_vol);
			} while (0);
			MESSAGE_PRINT ("XMSYS_UVC_CID_AUDIO_VOLUME(%d) (%s)\n", audio_vol, resp_code[ret]);
			break;			
		}
		
		case XMSYS_UVC_CID_OUTPUT_IMAGE_SIZE:
			// ���ͼ������
			// WIDTH(2�ֽ�),HEIGHT(2�ֽ�)
			// WIDTH  --> 320 ~ 1920
			// HEIGHT --> 240 ~ 1080
			
			MESSAGE_PRINT ("XMSYS_UVC_CID_OUTPUT_IMAGE_SIZE\n");
			do
			{
				XMSIZE size;
				if(command_packet_length != 4)
				{
					MESSAGE_PRINT ("XMSYS_UVC_CID_OUTPUT_IMAGE_SIZE's command packet length(%d) != 4\n", command_packet_length);
					break;
				}
				size.cx = *(unsigned short *)(command_packet_buffer + 0);
				size.cy = *(unsigned short *)(command_packet_buffer + 2);
				if(size.cx < 320 || size.cx > 1920)
				{
					MESSAGE_PRINT ("XMSYS_UVC_CID_OUTPUT_IMAGE_SIZE's cx(%d) illegal\n", size.cx);
					break;
				}
				if(size.cy < 240 || size.cy > 1080)
				{
					MESSAGE_PRINT ("XMSYS_UVC_CID_OUTPUT_IMAGE_SIZE's cy(%d) illegal\n", size.cy);
					break;
				}
				
				XM_printf ("width = %d, height = %d\n", size.cx, size.cy);
				// �رյ�ǰ��UVC��Դ
				//XMSYS_JpegSetupMode (-1, NULL);
				
				ret = ACK;
				
				XMSYS_CameraSetOutputImageSize (size);
				
				// �л���ʵʱ��
				//XMSYS_JpegSetupMode (XMSYS_JPEG_MODE_ISP_SCALAR, NULL);
				
			} while (0);
			MESSAGE_PRINT ("XMSYS_UVC_CID_OUTPUT_IMAGE_SIZE (%s)\n", resp_code[ret]);

			break;
		
		// ¼����Ƶ��ʽ���� (1080p30fps, 720p30fps, 720p60fps)
		case XMSYS_UVC_CID_VIDEO_FORMAT:	
		{
			int codec_ret;
			unsigned int codec_mode = XMSYS_H264CodecGetMode ();
			// ARKN141_VIDEO_FORMAT_1080P_30 = 0,
			// ARKN141_VIDEO_FORMAT_720P_30,
			// ARKN141_VIDEO_FORMAT_720P_60,
			unsigned char video_format = ARKN141_VIDEO_FORMAT_1080P_30;
			MESSAGE_PRINT ("XMSYS_UVC_CID_VIDEO_FORMAT\n");
			
#if TULV_USB
			// ��֧��1080P��ʽ
			ret = NAK;
#else
			do
			{
				if(command_packet_length != 1)
				{
					MESSAGE_PRINT ("XMSYS_UVC_CID_VIDEO_FORMAT's command packet length(%d) != 1\n", command_packet_length);
					break;
				}
				video_format = *command_packet_buffer;
				if(video_format >= ARKN141_VIDEO_FORMAT_COUNT)
				{
					MESSAGE_PRINT ("XMSYS_UVC_CID_VIDEO_FORMAT format(%d) invalid\n", video_format); 
					break;
				}
				
				// ֹͣ�����
				//codec_ret = XMSYS_H264CodecStop();
				
				AP_SetMenuItem (APPMENUITEM_VIDEO_RESOLUTION, (BYTE)video_format);
				// ����˵����õ�����洢�豸
				if(AP_SaveMenuData (&AppMenuData))
				{
					if(XMSYS_H264CodecSetVideoFormat (video_format) < 0)
						ret = NAK;
					else
						ret = ACK;
				}
				else
					ret = NAK;
				
			} while (0);
#endif
			
			MESSAGE_PRINT ("XMSYS_UVC_CID_VIDEO_FORMAT(%d) (%s)\n", video_format, resp_code[ret]);
			break;
		}
		
		// ¼������
		case XMSYS_UVC_CID_RECORD_START:
		{
			int codec_ret = -100;
			MESSAGE_PRINT ("CID_RECORD_START\n");
			do
			{
				XM_printf(">>>>>uvc start recorder..........\r\n");
				codec_ret = XMSYS_H264CodecRecorderStart ();
				if(codec_ret == 0)
					ret = ACK;
				else if(codec_ret == -1)
					ret = NAK;
				else
					ret = TIMEOUT;
			} while (0);
			if(ret == ACK)
			{
				XMSYS_JpegSetupMode(XMSYS_JPEG_MODE_ISP_SCALAR, 0);
			}
			else
			{
				XMSYS_JpegSetupMode(XMSYS_JPEG_MODE_DUMMY, 0);
			}
			
			MESSAGE_PRINT ("RECORD_START(%d), (%s)\n", codec_ret, resp_code[ret]);
			break;
		}

		// ¼��ֹͣ
		case XMSYS_UVC_CID_RECORD_STOP:
		{
			int codec_ret = -100;
			MESSAGE_PRINT ("CID_RECORD_STOP\n");
			do
			{
				codec_ret = XMSYS_H264CodecRecorderStop ();
				if(codec_ret == 0)
					ret = ACK;
				else if(codec_ret == -1)
					ret = NAK;
				else
					ret = TIMEOUT;
			} while (0);
			if(ret == ACK)
			{
				// ¼����ֹͣ, �л���DUMMYģʽ
				XMSYS_JpegSetupMode(XMSYS_JPEG_MODE_DUMMY, 0);		
			}
			
			MESSAGE_PRINT ("RECORD_STOP(%d), (%s)\n", codec_ret, resp_code[ret]);
			break;
		}
		
		// ¼��ͨ���л�
		case XMSYS_UVC_CID_RECORD_CHANNEL_SWITCH:
		{
			// XMSYS_UVC_CID_RECORD_CHANNEL_SWITCH
			unsigned int ch;
			MESSAGE_PRINT ("XMSYS_UVC_CID_RECORD_CHANNEL_SWITCH\n");
			if(command_packet_length != 1)
			{
				MESSAGE_PRINT ("XMSYS_UVC_CID_RECORD_CHANNEL_SWITCH's command packet length(%d) != 1\n", command_packet_length);
				break;
			}
			ch = *command_packet_buffer & 0x0F;
			if(ch >= 2)
			{
				MESSAGE_PRINT ("XMSYS_UVC_CID_RECORD_CHANNEL_SWITCH's ch(%d) != {0, 1}\n", ch);
				break;
			}
			if(ch == 0)
			{
				if(XMSYS_VideoSetImageAssemblyMode (XMSYS_ASSEMBLY_MODE_FRONT_ONLY) == 0)
					ret = ACK;
			}
			else if(ch == 1)
			{
				if(XMSYS_VideoSetImageAssemblyMode (XMSYS_ASSEMBLY_MODE_REAL_ONLY) == 0)
					ret = ACK;
			}
			MESSAGE_PRINT ("XMSYS_UVC_CID_RECORD_CHANNEL_SWITCH %s", ret == ACK ? "ACK" : "NAK");
			break;
		}
		
		// ����ǿ�Ʒ�¼��ģʽ
		case XMSYS_UVC_CID_SET_FORCED_NON_RECORDING_MODE:
		{
			int codec_ret = -100;
			MESSAGE_PRINT ("CID_SET_FORCED_NON_RECORDING_MODE\n");
			do
			{
				if(command_packet_length != 1)
				{
					MESSAGE_PRINT ("XMSYS_UVC_CID_SET_FORCED_NON_RECORDING_MODE's command packet length(%d) != 1\n", command_packet_length);
					break;
				}
				XMSYS_H264CodecSetForcedNonRecordingMode (*command_packet_buffer);
				ret = ACK;
			} while (0);
			
			MESSAGE_PRINT ("SET_FORCED_NON_RECORDING_MODE %d, %s\n", *command_packet_buffer, ret == ACK ? "OK" : "NG");
			break;
		}
		
		case XMSYS_UVC_CID_GET_FORCED_NON_RECORDING_MODE:
		{
			ret = XMSYS_H264CodecGetForcedNonRecordingMode();
			MESSAGE_PRINT ("UVC_CID_GET_FORCED_NON_RECORDING_MODE (%d)\n", ret);
			break;		
		}
		
		// һ������
		case XMSYS_UVC_CID_RECORD_ONE_KEY_PHOTOGRAPH:
		{
			int codec_ret = -100;
			MESSAGE_PRINT ("CID_RECORD_ONE_KEY_PHOTOGRAPH\n");
			do
			{
				codec_ret = XMSYS_JpegCodecOneKeyPhotograph ();
				if(codec_ret == 0)
					ret = ACK;
				else if(codec_ret == -1)
					ret = NAK;
				else
					ret = TIMEOUT;
			} while (0);
			MESSAGE_PRINT ("ONE_KEY_PHOTOGRAPH (%d), (%s)\n", codec_ret, resp_code[ret]);
			break;
		}
		
		// ��������ͷ
		case XMSYS_UVC_CID_SENSOR_START:
		{
			MESSAGE_PRINT ("XMSYS_UVC_CID_SENSOR_START\n");
			do
			{
				XMSYS_SensorCaptureStart ();
				ret = ACK;
			} while (0);
			MESSAGE_PRINT ("XMSYS_UVC_CID_SENSOR_START ACK");
			break;
		}
		
		// ��ȡ¼��/��Ƭ�ļ��б�
		// ��������ݳ���Ϊ1��CH(ͨ�����ֽڵĵ�4λ)��TYPE(���ͣ��ֽڵĸ�4λ)��
		// CH --> 0x00  ǰ������ͷ
		// CH --> 0x01  ��������ͷ		
		// CH --> 0x0F  ȫ������ͷ
		// TYPE --> 0x00 ��Ƶ
		// TYPE --> 0x01 ��Ƭ
		// TYPE --> 0x07 ȫ����ʽ
		case XMSYS_UVC_CID_GET_MEDIA_FILE_LIST:
		{
			unsigned char ch, type;
			unsigned short count;
			int codec_ret = -100;
			ch = 0;
			type = 0;
			//MESSAGE_PRINT ("CID_GET_MEDIA_FILE_LIST\n");
			do 
			{
				if(command_packet_length != 1)
				{
					MESSAGE_PRINT ("GET_MEDIA_FILE_LIST's command packet length(%d) != 1\n", command_packet_length);
					break;
				}
				ch = *command_packet_buffer & 0x0F;
				if(ch >= 2 && ch != 0x0F)	// 0x0F��ʾȫ��ͨ��
				{
					MESSAGE_PRINT ("GET_MEDIA_FILE_LIST's ch(%d) != {0, 1}\n", ch);
					break;
				}
				type = *(command_packet_buffer) >> 4;
				if(type >= 2 && type != 0x07)	// 0x0F��ʾȫ������
				{
					MESSAGE_PRINT ("CID_GET_MEDIA_FILE_LIST's type(%d) != {0, 1}\n", type);
					break;
				}
				// ����ִ����Ϻ�����������"�ļ�Ŀ¼"�첽֪ͨ��Ϣ
				// Ͷ���첽��Ϣ��"�ļ�Ŀ¼"
				if(type == 0x00 || type == 0x01 || type == 0x07)
				{
					if(PostMediaFileListMessage (ch, type) == 0)
						ret = ACK;
					else
						ret = NAK;
				}
				else
					ret = NAK;
			} while (0);
			MESSAGE_PRINT ("XMSYS_UVC_CID_GET_MEDIA_FILE_LIST %s\n", ret == ACK ? "ACK" : "NAK");
			break;
		}
		
		// �ط�����
		// ���������9���ֽ�, CH(ͨ������һ���ֽ����ݵĵ�4λ)��TYPE(���ͣ���һ���ֽ����ݵĸ�4λ)���ļ���(8���ֽ�, XXXXXXXX)
		// CH --> 0x00  ǰ������ͷ
		// CH --> 0x01  ��������ͷ
		// ����ȡֵ�Ƿ�
		//
		// TYPE --> 0x00 ��Ƶ
		// TYPE --> 0x01 ��Ƭ
		case XMSYS_UVC_CID_PLAYBACK_START:
		{
			int codec_ret = -100;
			unsigned char ch, type;
			MESSAGE_PRINT ("CID_PLAYBACK_START\n");
			do 
			{
				if(command_packet_length != 9)
				{
					MESSAGE_PRINT ("PLAYBACK_START's command packet length(%d) != 9\n", command_packet_length);
					break;
				}
				ch = *command_packet_buffer & 0x0F;
				if(ch >= 2)
				{
					MESSAGE_PRINT ("PLAYBACK_START's ch(%d) != {0, 1}\n", ch);
					break;
				}
				type = *(command_packet_buffer) >> 4;
				if(type >= 2)
				{
					MESSAGE_PRINT ("PLAYBACK_START's type(%d) != {0, 1}\n", type);
					break;
				}
				memset (file_name, 0, sizeof(file_name));
				if(type == 0)	// ��Ƶ
				{
					if(XMSYS_H264CodecStop () == 0)
					{
						combine_fullpath_media_filename ((unsigned char *)file_name, sizeof(file_name), ch, type, command_packet_buffer + 1);
						codec_ret = XMSYS_JpegSetupMode (XMSYS_JPEG_MODE_VIDEO, NULL);
						if(codec_ret == 0)
						{
							codec_ret = XMSYS_H264CodecPlayerStart (file_name, XM_VIDEO_PLAYBACK_MODE_NORMAL, 0);
							if(codec_ret < 0)
							{
								XMSYS_JpegSetupMode (XMSYS_JPEG_MODE_DUMMY, NULL);
							}
						}
						if(codec_ret == 0)
							ret = ACK;
						else if(codec_ret == -1)
							ret = NAK;
						else
							ret = TIMEOUT;
					}
					else
					{
						MESSAGE_PRINT ("XMSYS_H264CodecStop failed\n");
						ret = NAK;	
					}
				}
				else // ��Ƭ
				{				
					char file[9];
					memset (file, 0, sizeof(file));
					memcpy (file, (char *)(command_packet_buffer + 1), 8);
					MESSAGE_PRINT ("PLAYBACK_START photo(%s)\n", file);
					
					if(XMSYS_H264CodecStop () == 0)
					{
						// ֱ�ӽ���ƬǶ�뵽UVC���з���
						combine_fullpath_media_filename ((unsigned char *)file_name, sizeof(file_name), ch, type, command_packet_buffer + 1);
						codec_ret = XMSYS_JpegSetupMode (XMSYS_JPEG_MODE_IMAGE, file_name);		// ���ò��л���UVC��IMAGEģʽ
						if(codec_ret == 0)
							ret = ACK;
						else if(codec_ret == -1)
							ret = NAK;
						else
							ret = TIMEOUT;		
					}
					else
					{
						MESSAGE_PRINT ("XMSYS_H264CodecStop failed\n");
						ret = NAK;
					}
				}
			} while (0);
			MESSAGE_PRINT ("PLAYBACK_START %d, (%s)\n", codec_ret, resp_code[ret]);
			break;			
		}
		
		// �ط�ֹͣ
		case XMSYS_UVC_CID_PLAYBACK_STOP:
		{
			int codec_ret = -100;
			MESSAGE_PRINT ("CID_PLAYBACK_STOP\n");
			do
			{
				/*
				if(command_packet_length != 0)
				{
					MESSAGE_PRINT ("PLAYBACK_STOP's command packet length(%d) should be 0\n", command_packet_length);
					break;
				}
				*/
				codec_ret = XMSYS_H264CodecPlayerStop ();
				if(codec_ret == 0)
					ret = ACK;
				else if(codec_ret == -1)
					ret = NAK;
				else
					ret = TIMEOUT;
			} while (0);
			MESSAGE_PRINT ("PLAYBACK_STOP %d, (%s)\n", codec_ret, resp_code[ret]);
			if(ret == ACK)
			{
				// �ط���ֹͣ, �л���DUMMYģʽ
				XMSYS_JpegSetupMode(XMSYS_JPEG_MODE_DUMMY, 0);	
			}
			break;			
		}

		// �ط���ͣ
		case XMSYS_UVC_CID_PLAYBACK_PAUSE:
		{
			int codec_ret = -100;
			MESSAGE_PRINT ("CID_PLAYBACK_PAUSE\n");
			do 
			{
				/*
				if(command_packet_length != 0)
				{
					MESSAGE_PRINT ("PLAYBACK_PAUSE's command packet length(%d) should be 0\n", command_packet_length);
					break;
				}
				*/
				codec_ret = XMSYS_H264CodecPlayerPause ();
				if(codec_ret == 0)
					ret = ACK;
				else if(codec_ret == -1)
					ret = NAK;
				else
					ret = TIMEOUT;
			} while (0);
			MESSAGE_PRINT ("PLAYBACK_PAUSE %d, (%s)\n", codec_ret, resp_code[ret]);
			break;			
		}		

		// �طſ��
		case XMSYS_UVC_CID_PLAYBACK_FORWARD:
		{
			int codec_ret = -100;
			MESSAGE_PRINT ("CID_PLAYBACK_FORWARD\n");
			do
			{
				/*
				if(command_packet_length != 0)
				{
					MESSAGE_PRINT ("PLAYBACK_FORWARD's command packet length(%d) should be 0\n", command_packet_length);
					break;
				}
				*/
				codec_ret = XMSYS_H264CodecPlayerForward ();
				if(codec_ret == 0)
					ret = ACK;
				else if(codec_ret == -1)
					ret = NAK;
				else
					ret = TIMEOUT;
			} while (0);
			MESSAGE_PRINT ("PLAYBACK_FORWARD %d, (%s)\n", codec_ret, resp_code[ret]);
			break;			
		}		

		// �طſ���
		case XMSYS_UVC_CID_PLAYBACK_BACKWARD:
		{
			int codec_ret = -100;
			MESSAGE_PRINT ("CID_PLAYBACK_BACKWARD\n");
			do 
			{
				/*
				if(command_packet_length != 0)
				{
					MESSAGE_PRINT ("PLAYBACK_BACKWARD's command packet length(%d) should be 0\n", command_packet_length);
					break;
				}
				*/
				codec_ret = XMSYS_H264CodecPlayerBackward ();
				if(codec_ret == 0)
					ret = ACK;
				else if(codec_ret == -1)
					ret = NAK;
				else
					ret = TIMEOUT;
			} while (0);
			MESSAGE_PRINT ("PLAYBACK_BACKWARD %d, (%s)\n", codec_ret, resp_code[ret]);
			break;			
		}		

		// �طŶ�λ
		case XMSYS_UVC_CID_PLAYBACK_SEEK:
		{
			int codec_ret = -100;
			unsigned int seek_time;
			MESSAGE_PRINT ("CID_PLAYBACK_SEEK\n");
			do 
			{
				if(command_packet_length != 2)
				{
					MESSAGE_PRINT ("PLAYBACK_SEEK's command packet length(%d) should be 2\n", command_packet_length);
					break;
				}
				seek_time = command_packet_buffer[0] | (command_packet_buffer[1] << 8);
				codec_ret = XMSYS_H264CodecPlayerSeek (seek_time);
				if(codec_ret == 0)
					ret = ACK;
				else if(codec_ret == -1)
					ret = NAK;
				else
					ret = TIMEOUT;
			} while (0);
			MESSAGE_PRINT ("PLAYBACK_SEEK %d, (%s)\n", codec_ret, resp_code[ret]);			
			break;
		}
		
		case XMSYS_UVC_CID_SDCARD_GET_STATE:
		{
			// 0x01 --> SD���γ�
			// 0x02 --> SD������
			// 0x03 --> SD���ļ�ϵͳ����
			// 0x04 --> SD���޷�ʶ��
			static const char* sd_state_code[] = {
				"card plugout",
				"card insert",
				"card fserror",
				"card invalid"
			};
			MESSAGE_PRINT ("CID_SDCARD_GET_STATE\n");
			// ��ʹ���������Ҳ����״̬
			ret =  XMSYS_UvcSocketGetCardState ();
			MESSAGE_PRINT ("SDCARD_GET_STATE's command response=%d(%s)\n", ret, sd_state_code[ret - 1]);
			break;
		}
		
		// ��״̬���
		case XMSYS_UVC_CID_SDCARD_CLEAR_STATE:
		{
			// �������д�������� (��дУ��ʧ��)״̬������״̬�޷������
			// ������ѡ���������״̬��д�������𻵣�ʱ��Ӧ�������������״̬���֡�
			MESSAGE_PRINT ("XMSYS_UVC_CID_SDCARD_CLEAR_STATE\n");	
			ret = ACK;
			MESSAGE_PRINT ("XMSYS_UVC_CID_SDCARD_CLEAR_STATE's command response=%s\n", resp_code[ret]);
			break;
		}			

		// ��ȡ������
		case XMSYS_UVC_CID_SDCARD_GET_CAPACITY:
		{
			int codec_ret = -100;
			MESSAGE_PRINT ("CID_SDCARD_GET_CAPACITY\n");
			do
			{
				codec_ret = XMSYS_FileManagerGetCardCapacity ();
				if(codec_ret == 0)
					ret = ACK;
				else if(codec_ret == -1)
					ret = NAK;
				else
					ret = TIMEOUT;
			} while (0);
			MESSAGE_PRINT ("SDCARD_GET_CAPACITY %d, (%s)\n", codec_ret, resp_code[ret]);
			break;
		}

		// ����ʽ��
		case XMSYS_UVC_CID_SDCARD_FORMAT:
		{
			int fm_ret = -100;
			MESSAGE_PRINT ("CID_SDCARD_FORMAT\n");
			do
			{
				// ����¼������и�ʽ��SD��
				/*
				// �����Ƶ��¼�򲥷��Ƿ������
				if(XMSYS_H264CodecGetState() != XMSYS_UVC_H264_CODEC_STATE_STOP)
				{
					MESSAGE_PRINT ("SDCARD_FORMAT NG, CODEC STOP state required to format\n");
					break;
				}
				*/
				
				fm_ret = XMSYS_FileManagerCardFormat ();
				if(fm_ret == 0)
					ret = ACK;
				else if(fm_ret == -1)
					ret = NAK;
				else
					ret = TIMEOUT;
			} while (0);
			MESSAGE_PRINT ("SDCARD_FORMAT %d, (%s)\n", fm_ret, resp_code[ret]);
			break;
		}

		// �ļ���� (����ļ��Ƿ���ڡ�)
		case XMSYS_UVC_CID_MEDIA_FILE_CHECK:
		{
			unsigned char ch = 0xFF;
			unsigned char type = 0xFF;
			void *fp;
			memset (file_name, 0, sizeof(file_name));
			MESSAGE_PRINT ("CID_MEDIA_FILE_CHECK\n");
			do
			{
				if(command_packet_length != 9)
				{
					MESSAGE_PRINT ("FILE_CHECK's command packet length(%d) != 9\n", command_packet_length);
					break;
				}
				ch = *(command_packet_buffer + 0) & 0x0f;
				type = *(command_packet_buffer + 0) >> 4;
				if(ch >= 2)
				{
					MESSAGE_PRINT ("illegal channel (%d) != {0, 1}\n", ch);
					break;
				}
				if(type >= 2)
				{
					MESSAGE_PRINT ("illegal type (%d) != {0, 1}\n", type);
					break;
				}
				combine_fullpath_media_filename ((unsigned char *)file_name, sizeof(file_name), ch, type, command_packet_buffer + 1);
				fp = XM_fopen (file_name, "rb");
				if(fp)
				{
					XM_fclose (fp);
					ret = 0x00;
				}
				else
				{
					ret = 0x01;
				}
			} while (0);
			MESSAGE_PRINT ("FileCheck ch=%d, type=%d, (%s) resp=%s\n", ch, type, file_name, resp_code[ret]);
			break;
		}
		
		// �ļ�ɾ��
		// ɾ��һ���ļ�(��Ƶ�ļ�����Ƭ�ļ�)��ɾ�������󣬷���֪ͨ��Ϣ���ļ�ɾ����ɡ���������
		case XMSYS_UVC_CID_MEDIA_FILE_DELETE:
		{
			// ��������ݳ���Ϊ10��CH(ͨ����1�ֽ�)��TYPE(���ͣ�1�ֽ�), XXXXXXXX(�ļ���8���ֽ�)
			//	CH --> 0x00 ǰ������ͷ
			//	CH --> 0x01 ��������ͷ
			//	����ȡֵ�Ƿ�
			//	TYPE --> 0x00 ��Ƶ
			//	TYPE --> 0x01 ��Ƭ
			//	ɾ��һ���ļ�(��Ƶ�ļ�����Ƭ�ļ�)(�����ļ�����������׺)��
			//	ɾ�������󣬷���֪ͨ��Ϣ���ļ�ɾ����ɡ���������
			unsigned char ch = 0xFF;
			unsigned char type = 0xFF;
			int fm_ret = -100;
			memset (file_name, 0, sizeof(file_name));
			MESSAGE_PRINT ("CID_MEDIA_FILE_DELETE\n");
			do
			{
				if(command_packet_length != 9)
				{
					MESSAGE_PRINT ("FILE_DELETE's command packet length(%d) != 9\n", command_packet_length);
					break;
				}
				ch = *(command_packet_buffer + 0) & 0x0F;
				type = *(command_packet_buffer + 0) >> 4;
				if(ch >= 2)
				{
					MESSAGE_PRINT ("illegal channel (%d) != {0, 1}\n", ch);
					break;
				}
				if(type >= 2)
				{
					MESSAGE_PRINT ("illegal type (%d) != {0, 1}\n", type);
					break;
				}
				
				combine_fullpath_media_filename ((unsigned char *)file_name, sizeof(file_name), ch, type, command_packet_buffer + 1);
				fm_ret = XMSYS_FileManagerFileDelete (ch, type, command_packet_buffer + 1);
				if(fm_ret == 0)
					ret = ACK;
				else if(fm_ret == -1)
					ret = NAK;
				else
					ret = TIMEOUT;
			} while (0);
			MESSAGE_PRINT ("FileDelete %d, ch=%d, type=%d, (%s) resp=%s\n", fm_ret, ch, type, file_name, resp_code[ret]);
			break;
		}
		
		// ��Ƶ�ļ�����/����
		case XMSYS_UVC_CID_VIDEO_LOCK:
		{
			// ��������ݳ���Ϊ10��CH(ͨ������һ���ֽ����ݵĵ�4λ)��TYPE(���ͣ���һ���ֽڵĸ�4λ)��COMMAND(���1�ֽڣ�, XXXXXXXX(�ļ���8���ֽ�)
			unsigned char ch = 0xFF;
			unsigned char type = 0xFF;
			MESSAGE_PRINT ("CID_VIDEO_LOCK\n");
			do 
			{
				if(command_packet_length != 10)
				{
					MESSAGE_PRINT ("VIDEO_LOCK's packet length(%d) != 10\n", command_packet_length);
					break;
				}
				ch = *(command_packet_buffer + 0) & 0x0F;
				type = (*(command_packet_buffer + 0) >> 4) & 0x0F;
				if(ch >= 2)
				{
					MESSAGE_PRINT ("illegal channel (%d) != {0, 1}\n", ch);
					break;
				}
				if(type >= 2)
				{
					MESSAGE_PRINT ("illegal type (%d) != {0, 1}\n", type);
					break;
				}
				if(type == 1)
				{
					// ��֧����������
					MESSAGE_PRINT ("illegal image type (%d)\n", type);
					break;
				}
				
				combine_fullpath_media_filename ((unsigned char *)file_name, sizeof(file_name), ch, type, command_packet_buffer + 2);
				// MESSAGE_PRINT ("file_name=%s\n", file_name);
				// COMMAND --> 0 ����
				// COMMAND --> 1 ����
				if(command_packet_buffer[1])
				{
					// ����
					ret = !AP_VideoItemLockVideoFileFromFileName ((char *)file_name);
				}
				else
				{
					// ����
					ret = !AP_VideoItemUnlockVideoFileFromFileName ((char *)file_name);
				}
			} while (0);
			MESSAGE_PRINT ("FileLock ch=%d, type=%d, resp=%s\n", ch, type, resp_code[ret]);
			
			break;
		}
		
		// ϵͳ����
		case XMSYS_UVC_CID_SYSTEM_UPDATE:
		{
			// "ϵͳ����"����
			MESSAGE_PRINT ("XMSYS_UVC_CID_SYSTEM_UPDATE\n");
			XMSYS_H264CodecStop ();
			XMSYS_JpegSetupMode(XMSYS_JPEG_MODE_DUMMY, 0);
			// �رվ����
			xm_close_volume_service ("mmc:0:");
			
			int r = XMSYS_FileManagerSystemUpdate ();
			char resp;
			if(r == 0)
				resp = 0;
			else
				resp = 1;
			ret_code =  XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_SYNC,
																		command_id, 
																		(unsigned char *)&resp,
																		1
																		);
			return ret_code;
		}
			
		// ���ؿ���, ����Ƶ�ļ����ص�����, ����fopen	
		case XMSYS_UVC_CID_DOWNLOAD_OPEN:
		{
			// ���ļ�
			// "����Ƶ�ļ����ص�����, ����fopen
			//	��������ݳ���Ϊ9��
			//	CH(ͨ������һ���ֽ����ݵĵ�4λ)��
			// TYPE(���ͣ���һ���ֽ����ݵĸ�4λ)��
			// XXXXXXXX(�ļ���8���ֽ�)
			// CH --> 0x00 ǰ������ͷ
			//	CH --> 0x01 ��������ͷ
			//	����ȡֵ�Ƿ�
			// TYPE --> 0x00 ��Ƶ
			// TYPE --> 0x01 ��Ƭ"
			unsigned char ch = 0xFF;
			unsigned char type = 0xFF;
			int size = -1;
			MESSAGE_PRINT ("XMSYS_UVC_CID_DOWNLOAD_OPEN\n");
			do
			{
				if(command_packet_length != 9)
				{
					MESSAGE_PRINT ("XMSYS_UVC_CID_DOWNLOAD_OPEN's packet length(%d) != 9\n", command_packet_length);
					break;
				}
				ch = *command_packet_buffer & 0x0F;
				type = *command_packet_buffer >> 4;
				if(ch >= 2)
				{
					MESSAGE_PRINT ("illegal channel (%d) != {0, 1}\n", ch);
					break;
				}
				if(type >= 2)
				{
					MESSAGE_PRINT ("illegal type (%d) != {0, 1}\n", type);
					break;
				}
				
				size = XMSYS_CdrOpenStream (ch, type, (const char *)(command_packet_buffer + 1));
			} while (0);
			ret_code =  XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_SYNC,
																		command_id, 
																		(unsigned char *)&size,
																		4
																		);
			MESSAGE_PRINT ("XMSYS_UVC_CID_DOWNLOAD_OPEN %d\n", ret_code);
			return ret_code;
		}
	
		// ���عر�, ���������Ϊ�ա�����fclose
		// �رյ�ǰ���ڽ��е���Ƶ����.
		case XMSYS_UVC_CID_DOWNLOAD_CLOSE:
			MESSAGE_PRINT ("XMSYS_UVC_CID_DOWNLOAD_CLOSE\n");
			XMSYS_CdrCloseStream ();
			ret = ACK;
			break;
		
		// ���ض�λ	0x42	"��չ����"	"��λ���ص�λ��, ����fseek

		//	��������ݳ���Ϊ4�ֽ�, offset(4�ֽ�, ���ļ���ʼ����ƫ��)."	
		// ����ֵ "һ�ֽڳ��ȣ�
		//	0x00 --> ACK, ����ִ�гɹ�
		// 0x01 --> NAK, ����ִ��ʧ��"
		case XMSYS_UVC_CID_DOWNLOAD_SEEK:
			if(command_packet_length != 4)
			{
				MESSAGE_PRINT ("XMSYS_UVC_CID_DOWNLOAD_SEEK's command packet length(%d) != 4\n", command_packet_length);
				break;
			}
			MESSAGE_PRINT ("XMSYS_UVC_CID_DOWNLOAD_SEEK 0x%08x\n", get_long(command_packet_buffer));
			if(XMSYS_CdrSeekStream (get_long(command_packet_buffer)) == 0)
				ret = ACK;
			else
				ret = NAK;
			break;
			
		// ���ض�ȡ	0x43	"��չ����"	
		//	"��ȡ��������, ����fread
		//	��������ݳ���Ϊ8�ֽ�, ƫ��(4�ֽ�), ��ȡ�ֽ�size(4�ֽ�, �����ȡ�İ�����).
		//	Ӧ����е����ݳ���LENGTH <= ��ȡ�ֽ�size, �������ݴ�������Ӧ��������ݳ���."
		case XMSYS_UVC_CID_DOWNLOAD_READ:
			if(command_packet_length != 8)
			{
				MESSAGE_PRINT ("XMSYS_UVC_CID_DOWNLOAD_READ's command packet length(%d) != 8\n", command_packet_length);
				break;
			}
			//MESSAGE_PRINT ("XMSYS_UVC_CID_DOWNLOAD_READ offset=0x%08x, length=0x%08x\n", get_long(command_packet_buffer), get_long(command_packet_buffer+4));
			if(XMSYS_CdrReadStream (get_long(command_packet_buffer), get_long(command_packet_buffer + 4)) == 0)
				ret = ACK;
			else
				ret = NAK;
			return 0;		// no response
			
		// �Զ�����
		case XMSYS_UVC_CID_AUTOTEST_KEY:	
		{
			u16_t key, mod;
			if(command_packet_length != 4)
			{
				MESSAGE_PRINT ("XMSYS_UVC_CID_AUTOTEST_KEY's command packet length(%d) != 4\n", command_packet_length);
				break;
			}
			key = get_word(command_packet_buffer);
			mod = get_word(command_packet_buffer + 2);
			MESSAGE_PRINT ("XMSYS_UVC_CID_AUTOTEST_KEY key=0x%04x, mod=0x%04x\n", key, mod);
			if(XM_KeyEventProc (key, mod) == 1)
				ret = ACK;		// �����¼���Ͷ��
			else
				ret = NAK;
			break;
		}
		
		// ���֡
		case XMSYS_UVC_CID_HYBRID_START:
		{
			XM_HybridStreamStart ();
			ret = ACK;
			break;
		}
		case XMSYS_UVC_CID_HYBRID_STOP:
		{
			XM_HybridStreamStop ();
			ret = ACK;
			break;
		}
		case XMSYS_UVC_CID_HYBRID_FORCE_IFRAME:
		{
			// ��Ӧ���
			XM_HybridStreamForceIFrame ();
			return 0;
		}
		
		case XMSYS_UVC_CID_LOCK_CURRENT_VIDEO:
		{
			MESSAGE_PRINT ("XMSYS_UVC_CID_LOCK_CURRENT_VIDEO\n");
			XMSYS_H264CodecLockCurrentVideo ();
			ret = ACK;
			break;
		}
		case XMSYS_UVC_CID_UNLOCK_CURRENT_VIDEO:
		{
			MESSAGE_PRINT ("XMSYS_UVC_CID_UNLOCK_CURRENT_VIDEO\n");
			XMSYS_H264CodecUnLockCurrentVideo ();
			ret = ACK;
			break;
		}
		case XMSYS_UVC_CID_GET_VIDEO_LOCK_STATE:
		{
			ret = XMSYS_H264CodecGetVideoLockState ();
			MESSAGE_PRINT ("XMSYS_UVC_CID_GET_VIDEO_LOCK_STATE ret=%d\n", ret);
			break;
		}
			
		default:
			MESSAGE_PRINT ("unknown command 0x%x\n", command_id);
			return -1;	
	}
		
	// �˴������䵥�ֽڵ�Ӧ���
	return XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_SYNC,
																command_id, 
																&ret,
																1
																);
	
}

// �ļ��б����
void	XMSYS_UvcSocketFileListUpdate (unsigned char channel,
													  unsigned char type,
													  unsigned char file_name[8],
													  unsigned int create_time,		// �ļ�����ʱ��
													  unsigned int record_time,		// ��Ƶ¼��ʱ��(��)
													  unsigned short code)
{
	PostMediaFileListMessage (channel, type);
	/*
	//	��Ϣ������12���ֽ�
	//	�ļ���(XXXXXXXX,8�ֽ�), CH(ͨ����1�ֽ�), TYPE(���ͣ�1�ֽ�), CODE(�����룬2�ֽڣ���
	
	//	CH --> 0x00 ǰ������ͷ
	//	CH --> 0x01 ��������ͷ
	
	//	TYPE --> 0x00 ��Ƶ
	//	TYPE --> 0x01 ��Ƭ
	
	//	CODE --> 0 (ɾ��)
	//	CODE --> 1 (����)	
	unsigned char notification[20];
	char name[9];
	memset (name, 0, sizeof(name));
	memcpy (name, file_name, 8);
	memcpy (notification,  file_name, 8);
	memcpy (notification + 8,  &create_time, 4);
	memcpy (notification + 12, &record_time, 4);
	
	notification[16] = channel;
	notification[17] = type;
	memcpy (notification + 18, &code, 2);
	
	
	//MESSAGE_PRINT ("FILE LIST Update ch(%d), type(%d), %s, code(%d, %s)\n", channel, type, name, code, code == 0 ? "REMOVE" : "INSERT");
	// Ͷ���첽��Ϣ
	XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_ASYC,
															XMSYS_UVC_NID_FILE_LIST_UPDATE,
															notification,
															20
															);
	*/
}

// ��Ƶ���ĵ�һ֡����JPEGͼ���ѽ��벢�������������������Ƶ�������ʾ��
int XMSYS_UvcSocketImageOutputReady (unsigned short width, unsigned short height)
{
	return XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_ASYC,
															XMSYS_UVC_NID_IMAGE_OUTPUT,
															0,
															0
															);
	
}

// �ط���� (��ǰ¼����Ƶ�ļ��ط����)
void XMSYS_UvcSocketPlaybackFinish (unsigned char type)
{
	XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_ASYC,
															XMSYS_UVC_NID_PLAYBACK_FINISH,
															&type,
															1
															);
	
}

// �طŽ���ָʾ
// ��Ϣ������2���ֽ� ��ǰ��Ƶ����λ��(2�ֽڣ��뵥λ)
void XMSYS_UvcSocketPlaybackProgress (unsigned int playback_progress)
{
	unsigned short pos = (unsigned short)playback_progress;
	XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_ASYC,
															XMSYS_UVC_NID_PLAYBACK_PROGRESS,
															(unsigned char *)&pos,
															2
															);
	
}

// SD���γ�	0x05
// SD������	0x06
// SD��д��	0x07
// SD����	0x08
// SD���ļ�ϵͳ�Ƿ�	0x09
// SD���޷�ʶ�� 0x0A
//
// ��UI�㱨��SD����״̬
// sdcard_SystemEvent��SD����ص�ϵͳ�¼�
void XMSYS_UvcSocketReportSDCardSystemEvent (unsigned int sdcard_SystemEvent)
{
	char NID = (char)(-1);
	switch (sdcard_SystemEvent)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:	// SD���γ��¼�
			NID = XMSYS_UVC_NID_SDCARD_WITHDRAW;
			break;
			
		case SYSTEM_EVENT_CARD_INSERT:	// SD������
			NID = XMSYS_UVC_NID_SDCARD_INSERT;
			break;
			
		case SYSTEM_EVENT_CARD_DISK_FULL:
			NID = XMSYS_UVC_NID_SDCARD_FULL;
			break;
			
		case SYSTEM_EVENT_CARD_VERIFY_ERROR:
			NID = XMSYS_UVC_NID_SDCARD_DAMAGE;
			break;
			
		case SYSTEM_EVENT_CARD_INVALID:	
			NID = XMSYS_UVC_NID_SDCARD_INVALID;
			break;
			
		case SYSTEM_EVENT_CARD_FS_ERROR:
			NID = XMSYS_UVC_NID_FSERROR;
			break;
			
			
		case SYSTEM_EVENT_VIDEOITEM_RECYCLE_CONSUMED:	
			// ������̫��
			NID = XMSYS_UVC_NID_TOO_MORE_LOCKED_RESOURCE;
			break;
			
		case SYSTEM_EVENT_VIDEOITEM_LOW_PREFORMANCE:	
			NID = XMSYS_UVC_NID_LOW_PREFORMANCE;
			break;
			
		default:
			return;
	}
	XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_ASYC,
															NID,
															0,
															0
															);
}



