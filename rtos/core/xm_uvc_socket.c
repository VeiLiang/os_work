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
#include "RTOS.h"		// OS头文件
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

static OS_RSEMA uvc_message_socket_access_sema;		// socket内部数据互斥访问信号量

static int uvc_socket_enabled = 1;

#define	ACK		0		// 命令包已执行
#define	NAK		1		// 命令包被拒绝执行
#define	TIMEOUT	2		// 命令包超时

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

// 检查socket通信是否已建立
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


// 发送"同步应答包"或者"异步消息包"
int  XMSYS_UvcSocketTransferResponsePacket (unsigned int socket_type,  
												  				unsigned char command_id,			// 命令包ID
												  				unsigned char *command_packet_buffer,
												  				unsigned int command_packet_length
															)
{
	unsigned short packet_type;
	int ret = -1;
	int loop;
		
	// 锁定互斥信号量, 阻止“同步应答包”发送与“异步消息包”发送的并发操作
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
	
	// 解除互斥访问
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
		// 检查CDR(UVC)是否已创建
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
	
		// 获取视频列表
		if(type == 0 || type == 0x07)
		{
			// 通道0的视频列表
			if(ch == 0 || ch == 0x0F)
			{
				size = XM_VideoItemGetVideoItemList (0, 0, 1024, buffer, length);
				if(size > 0)
				{
					buffer += size;
					length -= size;
				}
			}
			// 通道1的视频列表
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
		
		// 获取照片列表
		if(type == 1 || type == 0x07)
		{
			// 通道0的照片列表
			if(ch == 0 || ch == 0x0F)
			{
				size = XM_VideoItemGetImageItemList (0, 0, 1024, buffer, length);
				if(size > 0)
				{
					buffer += size;
					length -= size;					
				}
			}
			// 通道1的照片列表
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
		
		// 检查是否存在有效列表需要传递
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

// 读取卡状态码
// 一字节长度，
// 0x01 --> SD卡拔出
// 0x02 --> SD卡插入
// 0x03 --> SD卡文件系统错误
// 0x04 --> SD卡无法识别
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

// 辅机接收到主机发出的命令包。对命令包解析并处理
// 0 	成功
// -1 失败
int  XMSYS_UvcSocketReceiveCommandPacket (
															 unsigned char command_id,					// 命令包标识
															 unsigned char *command_packet_buffer,
															 unsigned int command_packet_length
															)
{
	int i;
	char file_name[64];		// 保存媒体文件的全路径文件名
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
		case XMSYS_UVC_CID_GET_SYSTEM_VERSION:		// 获取系统版本号
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
			// 停止录像或播放
			MESSAGE_PRINT ("%d, CodecStop\n", XM_GetTickCount());
			XMSYS_H264CodecStop ();
			MESSAGE_PRINT ("%d, xm_close_volume_service\n", XM_GetTickCount());
			xm_close_volume_service ("mmc:0:");
			
			ret = ACK; 
			XMSYS_FileManagerSystemShutDown (cmd);
			break;	
		}
			
		case XMSYS_UVC_CID_DATETIME_SET:		// 时间设置
		{
			// 4字节长度 （时间表示）
			//
			//	YY --> 0 ~ 99  (减去2000)
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
		
		// 录像分段时间长度设置
		case XMSYS_UVC_CID_VIDEO_SEGMENT_LENGTH_SET:
		{
			// LL （1字节长度）
			//	LL --> 0x02（2分钟）, 
			//	LL --> 0x03（3分钟）, 
			//	LL --> 0x05（5分钟）,
			//	其他取值非法
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
		
		// 录音开启、关闭
		case XMSYS_UVC_CID_MIC_SET:
		{
			// MM （1字节长度）
			// MM --> 0x00(录音关闭)，
			// MM --> 0x01(录音开启),
			// 其他取值非法
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
		
		// 读取录音开关状态
		case XMSYS_UVC_CID_MIC_GET:
		{
			const char *mic_code[] = {"OFF", "ON"};
			MESSAGE_PRINT ("CID_MIC_GET\n");
			// 总是返回录音状态
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
		
		// 视频输出开启、关闭
		// 0 --> 关闭
		// 1 --> 开启
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
		
		// 音量静音设置
		// "MUTE （1字节长度）
		// MUTE --> 0x01(音量静音开启)，MUTE --> 0x00(音量静音关闭), 其他取值非法"
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
			// 输出图像设置
			// WIDTH(2字节),HEIGHT(2字节)
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
				// 关闭当前的UVC资源
				//XMSYS_JpegSetupMode (-1, NULL);
				
				ret = ACK;
				
				XMSYS_CameraSetOutputImageSize (size);
				
				// 切换到实时流
				//XMSYS_JpegSetupMode (XMSYS_JPEG_MODE_ISP_SCALAR, NULL);
				
			} while (0);
			MESSAGE_PRINT ("XMSYS_UVC_CID_OUTPUT_IMAGE_SIZE (%s)\n", resp_code[ret]);

			break;
		
		// 录像视频格式设置 (1080p30fps, 720p30fps, 720p60fps)
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
			// 仅支持1080P格式
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
				
				// 停止编解码
				//codec_ret = XMSYS_H264CodecStop();
				
				AP_SetMenuItem (APPMENUITEM_VIDEO_RESOLUTION, (BYTE)video_format);
				// 保存菜单设置到物理存储设备
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
		
		// 录像启动
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

		// 录像停止
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
				// 录像已停止, 切换到DUMMY模式
				XMSYS_JpegSetupMode(XMSYS_JPEG_MODE_DUMMY, 0);		
			}
			
			MESSAGE_PRINT ("RECORD_STOP(%d), (%s)\n", codec_ret, resp_code[ret]);
			break;
		}
		
		// 录像通道切换
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
		
		// 设置强制非录像模式
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
		
		// 一键拍照
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
		
		// 开启摄像头
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
		
		// 读取录像/照片文件列表
		// 命令包数据长度为1，CH(通道，字节的低4位)，TYPE(类型，字节的高4位)，
		// CH --> 0x00  前置摄像头
		// CH --> 0x01  后置摄像头		
		// CH --> 0x0F  全部摄像头
		// TYPE --> 0x00 视频
		// TYPE --> 0x01 照片
		// TYPE --> 0x07 全部格式
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
				if(ch >= 2 && ch != 0x0F)	// 0x0F表示全部通道
				{
					MESSAGE_PRINT ("GET_MEDIA_FILE_LIST's ch(%d) != {0, 1}\n", ch);
					break;
				}
				type = *(command_packet_buffer) >> 4;
				if(type >= 2 && type != 0x07)	// 0x0F表示全部类型
				{
					MESSAGE_PRINT ("CID_GET_MEDIA_FILE_LIST's type(%d) != {0, 1}\n", type);
					break;
				}
				// 辅机执行完毕后，向主机发送"文件目录"异步通知消息
				// 投递异步消息包"文件目录"
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
		
		// 回放启动
		// 命令包数据9个字节, CH(通道，第一个字节数据的低4位)，TYPE(类型，第一个字节数据的高4位)，文件名(8个字节, XXXXXXXX)
		// CH --> 0x00  前置摄像头
		// CH --> 0x01  后置摄像头
		// 其他取值非法
		//
		// TYPE --> 0x00 视频
		// TYPE --> 0x01 照片
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
				if(type == 0)	// 视频
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
				else // 照片
				{				
					char file[9];
					memset (file, 0, sizeof(file));
					memcpy (file, (char *)(command_packet_buffer + 1), 8);
					MESSAGE_PRINT ("PLAYBACK_START photo(%s)\n", file);
					
					if(XMSYS_H264CodecStop () == 0)
					{
						// 直接将照片嵌入到UVC流中返回
						combine_fullpath_media_filename ((unsigned char *)file_name, sizeof(file_name), ch, type, command_packet_buffer + 1);
						codec_ret = XMSYS_JpegSetupMode (XMSYS_JPEG_MODE_IMAGE, file_name);		// 设置并切换到UVC的IMAGE模式
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
		
		// 回放停止
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
				// 回放已停止, 切换到DUMMY模式
				XMSYS_JpegSetupMode(XMSYS_JPEG_MODE_DUMMY, 0);	
			}
			break;			
		}

		// 回放暂停
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

		// 回放快进
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

		// 回放快退
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

		// 回放定位
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
			// 0x01 --> SD卡拔出
			// 0x02 --> SD卡插入
			// 0x03 --> SD卡文件系统错误
			// 0x04 --> SD卡无法识别
			static const char* sd_state_code[] = {
				"card plugout",
				"card insert",
				"card fserror",
				"card invalid"
			};
			MESSAGE_PRINT ("CID_SDCARD_GET_STATE\n");
			// 即使命令包错误，也返回状态
			ret =  XMSYS_UvcSocketGetCardState ();
			MESSAGE_PRINT ("SDCARD_GET_STATE's command response=%d(%s)\n", ret, sd_state_code[ret - 1]);
			break;
		}
		
		// 卡状态清除
		case XMSYS_UVC_CID_SDCARD_CLEAR_STATE:
		{
			// 清除卡的写满、卡损坏 (读写校验失败)状态，其他状态无法清除。
			// 当主机选择忽略上述状态（写满、卡损坏）时，应将其清除，避免状态保持。
			MESSAGE_PRINT ("XMSYS_UVC_CID_SDCARD_CLEAR_STATE\n");	
			ret = ACK;
			MESSAGE_PRINT ("XMSYS_UVC_CID_SDCARD_CLEAR_STATE's command response=%s\n", resp_code[ret]);
			break;
		}			

		// 读取卡容量
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

		// 卡格式化
		case XMSYS_UVC_CID_SDCARD_FORMAT:
		{
			int fm_ret = -100;
			MESSAGE_PRINT ("CID_SDCARD_FORMAT\n");
			do
			{
				// 允许录像过程中格式化SD卡
				/*
				// 检查视频记录或播放是否进行中
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

		// 文件检查 (检查文件是否存在。)
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
		
		// 文件删除
		// 删除一个文件(视频文件或照片文件)。删除结束后，发送通知消息“文件删除完成”给主机。
		case XMSYS_UVC_CID_MEDIA_FILE_DELETE:
		{
			// 命令包数据长度为10，CH(通道，1字节)，TYPE(类型，1字节), XXXXXXXX(文件名8个字节)
			//	CH --> 0x00 前置摄像头
			//	CH --> 0x01 后置摄像头
			//	其他取值非法
			//	TYPE --> 0x00 视频
			//	TYPE --> 0x01 照片
			//	删除一个文件(视频文件或照片文件)(完整文件名，包含后缀)。
			//	删除结束后，发送通知消息“文件删除完成”给主机。
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
		
		// 视频文件锁定/解锁
		case XMSYS_UVC_CID_VIDEO_LOCK:
		{
			// 命令包数据长度为10，CH(通道，第一个字节数据的低4位)，TYPE(类型，第一个字节的高4位)，COMMAND(命令，1字节）, XXXXXXXX(文件名8个字节)
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
					// 不支持拍照锁定
					MESSAGE_PRINT ("illegal image type (%d)\n", type);
					break;
				}
				
				combine_fullpath_media_filename ((unsigned char *)file_name, sizeof(file_name), ch, type, command_packet_buffer + 2);
				// MESSAGE_PRINT ("file_name=%s\n", file_name);
				// COMMAND --> 0 解锁
				// COMMAND --> 1 锁定
				if(command_packet_buffer[1])
				{
					// 锁定
					ret = !AP_VideoItemLockVideoFileFromFileName ((char *)file_name);
				}
				else
				{
					// 解锁
					ret = !AP_VideoItemUnlockVideoFileFromFileName ((char *)file_name);
				}
			} while (0);
			MESSAGE_PRINT ("FileLock ch=%d, type=%d, resp=%s\n", ch, type, resp_code[ret]);
			
			break;
		}
		
		// 系统升级
		case XMSYS_UVC_CID_SYSTEM_UPDATE:
		{
			// "系统升级"命令
			MESSAGE_PRINT ("XMSYS_UVC_CID_SYSTEM_UPDATE\n");
			XMSYS_H264CodecStop ();
			XMSYS_JpegSetupMode(XMSYS_JPEG_MODE_DUMMY, 0);
			// 关闭卷服务
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
			
		// 下载开启, 将视频文件下载到主机, 类似fopen	
		case XMSYS_UVC_CID_DOWNLOAD_OPEN:
		{
			// 流文件
			// "将视频文件下载到主机, 类似fopen
			//	命令包数据长度为9，
			//	CH(通道，第一个字节数据的低4位)，
			// TYPE(类型，第一个字节数据的高4位)，
			// XXXXXXXX(文件名8个字节)
			// CH --> 0x00 前置摄像头
			//	CH --> 0x01 后置摄像头
			//	其他取值非法
			// TYPE --> 0x00 视频
			// TYPE --> 0x01 照片"
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
	
		// 下载关闭, 命令包数据为空。类似fclose
		// 关闭当前正在进行的视频下载.
		case XMSYS_UVC_CID_DOWNLOAD_CLOSE:
			MESSAGE_PRINT ("XMSYS_UVC_CID_DOWNLOAD_CLOSE\n");
			XMSYS_CdrCloseStream ();
			ret = ACK;
			break;
		
		// 下载定位	0x42	"扩展命令"	"定位下载的位置, 类似fseek

		//	命令包数据长度为4字节, offset(4字节, 从文件开始处的偏移)."	
		// 返回值 "一字节长度，
		//	0x00 --> ACK, 命令执行成功
		// 0x01 --> NAK, 命令执行失败"
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
			
		// 下载读取	0x43	"扩展命令"	
		//	"读取下载数据, 类似fread
		//	命令包数据长度为8字节, 偏移(4字节), 读取字节size(4字节, 定义读取的包长度).
		//	应答包中的数据长度LENGTH <= 读取字节size, 辅机根据带宽会调整应答包的数据长度."
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
			
		// 自动测试
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
				ret = ACK;		// 按键事件已投递
			else
				ret = NAK;
			break;
		}
		
		// 混合帧
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
			// 无应答包
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
		
	// 此处仅传输单字节的应答包
	return XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_SYNC,
																command_id, 
																&ret,
																1
																);
	
}

// 文件列表更新
void	XMSYS_UvcSocketFileListUpdate (unsigned char channel,
													  unsigned char type,
													  unsigned char file_name[8],
													  unsigned int create_time,		// 文件创建时间
													  unsigned int record_time,		// 视频录像时间(秒)
													  unsigned short code)
{
	PostMediaFileListMessage (channel, type);
	/*
	//	消息包数据12个字节
	//	文件名(XXXXXXXX,8字节), CH(通道，1字节), TYPE(类型，1字节), CODE(操作码，2字节），
	
	//	CH --> 0x00 前置摄像头
	//	CH --> 0x01 后置摄像头
	
	//	TYPE --> 0x00 视频
	//	TYPE --> 0x01 照片
	
	//	CODE --> 0 (删除)
	//	CODE --> 1 (增加)	
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
	// 投递异步消息
	XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_ASYC,
															XMSYS_UVC_NID_FILE_LIST_UPDATE,
															notification,
															20
															);
	*/
}

// 视频流的第一帧或者JPEG图像已解码并输出，用于主机开启视频输入的显示。
int XMSYS_UvcSocketImageOutputReady (unsigned short width, unsigned short height)
{
	return XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_ASYC,
															XMSYS_UVC_NID_IMAGE_OUTPUT,
															0,
															0
															);
	
}

// 回放完成 (当前录像视频文件回放完成)
void XMSYS_UvcSocketPlaybackFinish (unsigned char type)
{
	XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_ASYC,
															XMSYS_UVC_NID_PLAYBACK_FINISH,
															&type,
															1
															);
	
}

// 回放进度指示
// 消息包数据2个字节 当前视频播放位置(2字节，秒单位)
void XMSYS_UvcSocketPlaybackProgress (unsigned int playback_progress)
{
	unsigned short pos = (unsigned short)playback_progress;
	XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_ASYC,
															XMSYS_UVC_NID_PLAYBACK_PROGRESS,
															(unsigned char *)&pos,
															2
															);
	
}

// SD卡拔出	0x05
// SD卡插入	0x06
// SD卡写满	0x07
// SD卡损坏	0x08
// SD卡文件系统非法	0x09
// SD卡无法识别 0x0A
//
// 向UI层报告SD卡的状态
// sdcard_SystemEvent与SD卡相关的系统事件
void XMSYS_UvcSocketReportSDCardSystemEvent (unsigned int sdcard_SystemEvent)
{
	char NID = (char)(-1);
	switch (sdcard_SystemEvent)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:	// SD卡拔出事件
			NID = XMSYS_UVC_NID_SDCARD_WITHDRAW;
			break;
			
		case SYSTEM_EVENT_CARD_INSERT:	// SD卡插入
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
			// 锁定项太多
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



