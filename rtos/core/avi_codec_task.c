//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: avi_codec_task.c
//	  AVI流编解码任务
//
//	Revision history
//
//		2012.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#include <stdio.h>
#include "xm_core.h"
#include "RTOS.h"		// OS头文件
#include <string.h>
#include <stdlib.h>
#include "hardware.h"
#include <xm_type.h>
#include <xm_dev.h>
#include <xm_key.h>
#include <xm_queue.h>
#include <assert.h>
#include <xm_base.h>
#include <xm_printf.h>
#include <xm_mci.h>
#include <xm_user.h>
#include "xm_core.h"
#include <xm_h264_codec.h>
#include "xm_h264_cache_file.h"
#include "xm_videoitem.h"
#include <xm_app_menudata.h>
#include "arkn141_isp.h"
#include "xm_systemupdate.h"
#include "rom.h"
#include "avi_codec_video_setting.h"
#include "xm_power.h"
#include "xm_voice_prompts.h"
#include "xm_videoitem.h"
#include "app_head.h"
#include "rxchip.h"

typedef unsigned int i32;
#if TULV
extern int ITU656_in;
#endif
extern INT32 IsCardExist(UINT32 ulCardID);
static void open_recycle_task (void);		// 开启循环回收任务
static void close_recycle_task (void);		// 关闭循环回收任务

extern  int ShowLogo (UINT32 LogoAddr,UINT32 LogoSize);
extern int PowerOnShowLogo;
extern unsigned int rendervideo_start_ticket;	//当前视频文件已回放时间(减去开始时间)

extern unsigned int XMSYS_GetCacheSystemBusyLevel (void);

#define	CODEC_TIMEOUT				2000	// 2000毫秒命令超时

#define CVBS_NTSC					0x00
#define CVBS_PAL					0x01
#define _720P_25FPS				0x02
#define _720P_30FPS				0x03
#define _720P_50FPS				0x04
#define _720P_60FPS				0x05
#define _1080P_25FPS				0x06
#define _1080P_30FPS				0x07

// 20180516 增加缩时录影模式
static volatile unsigned int time_lapse_photography_mode = 0;	// 1 缩时录影模式 0 普通录影模式
static unsigned int current_video_start_ticket;		//当前视频文件已录制时间(减去开始时间)
static volatile int forced_iframe;			// 1 强制IFrame生成
static volatile int forced_encoder_stop;	// 1 强制当前编码器退出
static volatile unsigned int video_lock_state;		// 当前正在录制的视频文件是否已锁定 1 锁定中 0 未锁定
static volatile unsigned int video_lock_command;	// 0x01 --> 加锁, 0x02 --> 解锁
static volatile int forced_non_recording_mode=0;		// 强制非记录模式 1 强制非记录模式 0 普通记录模式
static volatile int forced_switch_to_recording_mode;		// 标记从"强制非记录模式"切换到"普通记录模式"
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

static volatile H264CODEC_COMMAND_PARAM h264codec_command_param;

static OS_RSEMA h264codec_sema;			// 互斥保护
static OS_MAILBOX h264codec_mailbox;		// 一字节邮箱。
static char h264codec_mailbuf;					// 一个字节的邮箱

static unsigned char h264codec_frame_rate = 30;	// 每秒30帧缺省

#define	MASK_LAST_TICKET		1000		// 帧屏蔽持续时间
static volatile unsigned int h264_mask_mod = 0;
static volatile unsigned int mask_stop_ticket;			// 标记当前帧屏蔽结束的时间
static  unsigned int frame_mask[] = {
	  0,								// 不删
	  1<<0 | 1<<15,				// 删去2帧
	  1<<0 | 1<<6 | 1<<12 | 1<<18 | 1<<24,						// 删去5帧
	  1<<0 | 1<<3 | 1<<6 | 1<<9 | 1<<12 | 1<<15 | 1<<18 | 1<<21 | 1<<24 | 1<<27,			// 删去10帧
	(unsigned int)~(1<<0 | 1<<3 | 1<<6 | 1<<9 | 1<<12 | 1<<15 | 1<<18 | 1<<21 | 1<<24 | 1<<27),			// 删去20帧
	(unsigned int)~(1<<0 | 1<<6 | 1<<12 | 1<<18 | 1<<24),						// 删去25帧
};

#pragma data_alignment=64
__no_init static unsigned char		audio_fifo[4096*4];
#pragma data_alignment=64
__no_init static unsigned char		audio_data[0x8000];


// 读取当前的缩时录影模式
// 返回值
//		1		缩时录影模式
//		0		普通录影模式
unsigned int XMSYS_H264CodecGetTimeLapsePhotographyMode (void)
{
	return time_lapse_photography_mode;
}

// 设置缩时录影模式
// mode	1	缩时录影模式
//       0	普通录影模式
void XMSYS_H264CodecSetTimeLapsePhotographyMode (unsigned int mode)
{
	time_lapse_photography_mode = mode;
}

// 设置更新IFRAME标志,即强制下一帧编码I帧
void XMSYS_H264CodecForceIFrame (void)
{
	forced_iframe = 1;
}

// 获取当前视频已录制时间
unsigned int XMSYS_H264CodecGetCurrentVideoRecoringTime (void)
{
	int recoding_ticket;
	XM_lock ();
	recoding_ticket = XM_GetTickCount() - current_video_start_ticket;
	if(recoding_ticket < 0)
		recoding_ticket = 0;
	XM_unlock ();
	
	return recoding_ticket;
}

// 将当前录制视频锁定
void XMSYS_H264CodecLockCurrentVideo (void)
{
	video_lock_command = 0x01;
}

// 将当前录制视频解锁
void XMSYS_H264CodecUnLockCurrentVideo (void)
{
	video_lock_command = 0x02;
}

// 获取当前录像视频的锁定状态
// 1 锁定中
// 0 未锁定
unsigned int XMSYS_H264CodecGetVideoLockState (void)
{
	return video_lock_state;
}

// 检查IFRAME标志
int XMSYS_H264CodecCheckIFrame (void)
{
	int flag = forced_iframe;
	forced_iframe = 0;
	return flag;
}

void XMSYS_H264CodecForcedEncoderStop (void)
{
	forced_encoder_stop = 1;
}

// 获取AVI编码使用的视频格式
int XMSYS_H264CodecGetVideoFormat (void)
{
	return AP_GetMenuItem (APPMENUITEM_VIDEO_RESOLUTION);
}

int XMSYS_H264CodecSetVideoFormat (int video_format)
{
	int ret = -1;
	OS_EnterRegion();		// 禁止任务切换
	do
	{
		if(video_format < 0 || video_format >= ARKN141_VIDEO_FORMAT_COUNT)
		{
			XM_printf ("XMSYS_H264CodecSetVideoFormat failed, illegal format (%d)\n", video_format);
			// break;
			video_format = AP_SETTING_VIDEORESOLUTION_1080P_30;
		}
		if(h264codec_state != XMSYS_H264_CODEC_STATE_STOP)
		{
			XM_printf ("XMSYS_H264CodecSetVideoFormat failed, only STOP state support set action\n");
			break;			
		}
		
#if HONGJING_CVBS
		// 配置ISP视频参数
		if(video_format == AP_SETTING_VIDEORESOLUTION_720P_30)
		{
			XM_lock ();
			isp_set_video_width (1280);
			isp_set_video_height (720);
			isp_set_sensor_bit (ARKN141_ISP_SENSOR_BIT_12);
			XM_unlock ();
			XM_printf ("video format (720P_30)\n");
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_720X480P)
		{
			XM_lock ();
			isp_set_video_width (720);
			isp_set_video_height (480);
			isp_set_sensor_bit (ARKN141_ISP_SENSOR_BIT_12);
			XM_unlock ();
			XM_printf ("video format (720x480P)\n");
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_640X480P)
		{
			XM_lock ();
			isp_set_video_width (640);
			isp_set_video_height (480);
			isp_set_sensor_bit (ARKN141_ISP_SENSOR_BIT_12);
			XM_unlock ();
			XM_printf ("video format (640X480P)\n");
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_320X240P)
		{
			XM_lock ();
			isp_set_video_width (320);
			isp_set_video_height (240);
			isp_set_sensor_bit (ARKN141_ISP_SENSOR_BIT_12);
			XM_unlock ();
			XM_printf ("video format (320X240P)\n");
		}
		else //if(video_format == AP_SETTING_VIDEORESOLUTION_1080P_30)
		{
			// 缺省1080P 30fps
			XM_lock ();
			isp_set_video_width (1920);
			isp_set_video_height (1080);			
			isp_set_sensor_bit (ARKN141_ISP_SENSOR_BIT_12);
			XM_unlock ();
			XM_printf ("video format (1080P_30)\n");
		}		
#else
		// 配置ISP视频参数
		if(video_format == AP_SETTING_VIDEORESOLUTION_720P_30)
		{
			XM_lock ();
			isp_set_video_width (1280);
			isp_set_video_height (720);
			isp_set_sensor_bit (ARKN141_ISP_SENSOR_BIT_10);
			XM_unlock ();
			XM_printf ("video format (720P_30)\n");
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_720P_60)
		{
			XM_lock ();
			isp_set_video_width (1280);
			isp_set_video_height (720);
			isp_set_sensor_bit (ARKN141_ISP_SENSOR_BIT_10);
			XM_unlock ();
			XM_printf ("video format (720P_60)\n");
		}
		else //if(video_format == AP_SETTING_VIDEORESOLUTION_1080P_30)
		{
			// 缺省1080P 30fps
			XM_lock ();
			isp_set_video_width (1920);
			isp_set_video_height (1080);			
			isp_set_sensor_bit (ARKN141_ISP_SENSOR_BIT_12);
			XM_unlock ();
			XM_printf ("video format (1080P_30)\n");
		}		
#endif
		ret = 0;
	} while (0);
	OS_LeaveRegion();
	
	return ret;
}


void XMSYS_H264CodecSetFrameRate (unsigned char fps)
{
	h264codec_frame_rate = fps;
}

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
	//h264codec_command_param.id = 0;
	h264codec_command_param.command = 0;
	OS_Unuse(&h264codec_sema);
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

static int check_same_video_time (XMSYSTEMTIME *LastTime, XMSYSTEMTIME *CurrTime)
{
	if(LastTime == 0 || CurrTime == 0)
		return 0;
	if(	LastTime->wYear == CurrTime->wYear
		&&	LastTime->bDay == CurrTime->bDay
		&& LastTime->bMonth == CurrTime->bMonth
		&& LastTime->bHour == CurrTime->bHour
		&& LastTime->bMinute == CurrTime->bMinute
		&& LastTime->bSecond == CurrTime->bSecond )
		return 1;
	return 0;
}

// 返回值
// 0    表示对下一个视频帧进行H264编码
// 1    表示屏蔽下一个视频帧，不进行H264编码
// 2	表示终止记录并退出
// 3 	表示记录时间长度已达到最大，
// 4    表示通道切换
int XMSYS_H264CodecCheckEvent (struct H264EncodeStream *stream)
{
	int ret = 0;
	u8_t event;
	int64_t clock;
	//unsigned int mask = h264_mask_mod;
	int mask = 0;		// 是否屏蔽当前帧(即不进行H264编码，只进行scalar操作)

	// 检查外部ACC电源状况
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// XM_printf ("codec stop, acc bad\n");
		// 非安全电压
		// ACC上电 --> ACC掉电, 准备关机
		return 2;		
	}
	
	if(forced_encoder_stop)
	{
		// 该事件为一次性事件
		forced_encoder_stop = 0;
		if(stream->file_save)
		{
			XM_printf ("codec stop, forced encoder stop\n");
			// 卷关闭强制编码器退出事件
			return 2;
		}
	}
	
	/*
	if(stream->frame_number == 0)
	{
		// 第一帧
		// 根据当前系统时间，修正帧号
		//float x;
		//x * 1000.0 / h264codec_frame_rate =  XM_GetTickCount()
		stream->frame_number = (XM_GetTickCount() - stream->start_ticket * h264codec_frame_rate + 500) / 1000;
		stream->frame_number ++;
	}
	else
	{
		stream->frame_number ++;
	}
	*/
	
	// 根据时间，计算当前的帧号
	stream->frame_number = ((XM_GetTickCount() - stream->start_ticket) * h264codec_frame_rate + 500) / 1000;
	
	if(h264_mask_mod == 0)
	{
		// 非帧屏蔽模式
		unsigned int busy_level = XMSYS_GetCacheSystemBusyLevel ();
		// 越繁忙, 屏蔽帧越多
		switch(busy_level)
		{
#if 1
			// 20180410 zhuoyonghong 关闭帧屏蔽，首先使用动态码率调整
			// 最恶劣情况下屏蔽5帧
			case 8:		h264_mask_mod = frame_mask[2];	break;
			case 9:		h264_mask_mod = frame_mask[2];	break;
#else
			case 4:		h264_mask_mod = frame_mask[1];	break;
			case 5:		h264_mask_mod = frame_mask[1];	break;
			case 6:		h264_mask_mod = frame_mask[2];	break;
			case 7:		h264_mask_mod = frame_mask[3];	break;
			case 8:		h264_mask_mod = frame_mask[4];	break;
			case 9:		h264_mask_mod = frame_mask[5];	break;
#endif
			default:	h264_mask_mod = 0;				break;
		}
		if(h264_mask_mod)
		{
			mask_stop_ticket = XM_GetTickCount() + MASK_LAST_TICKET;		// 标记下次检查时间
			XM_printf ("mask_mod=0x%08x\n", h264_mask_mod);
		}
		else if(XM_VideoItemCheckDiskFreeSpaceBusy())
		{
			// 20180410 zhuoyonghong 关闭帧屏蔽，首先使用动态码率调整
			// 磁盘剩余空间无法满足实时录像需求, 降低实时编码帧率
			// mask_stop_ticket = XM_GetTickCount() + MASK_LAST_TICKET;		// 标记下次检查时间
			//h264_mask_mod = frame_mask[3];
			//h264_mask_mod = frame_mask[2];
			//XM_printf ("free space busy, mask_mod=0x%08x\n", h264_mask_mod);
		}
	}
	
	// 检查帧屏蔽模式.若存在, 按照mask pattern屏蔽帧
	if(h264_mask_mod)
	{
		int shift = stream->frame_number % h264codec_frame_rate;
		if( h264_mask_mod & (1 << shift))
		{
			// 当前帧需要屏蔽
			// 累加连续需要屏蔽的帧个数，即延迟时间
			mask = 1;
		}
		else
		{
			// 当前帧需要记录
			// 检查是否是最慢速记录
			if(h264_mask_mod == frame_mask[5])
			{
				// 强制I帧
				XMSYS_H264CodecForceIFrame ();
			}
		}
		/*
		if( (stream->frame_number % mask) == 0 )
		{
			stream->frame_number ++;	// 增加延时，从而降低帧率
		}
		*/
		
		if(XM_GetTickCount() >= mask_stop_ticket)
		{
			h264_mask_mod = 0;
		}
	}
	
	
	if(stream->file_save == 0)
	{
		mask = 1;
	}
	
	//XM_printf ("XMSYS_H264CodecCheckEvent\n");
	#if 0
	// 检查SD卡		
	if(stream->file_save)
	{
		// 文件保存模式, 可以保存视频
		
		// 若SD卡拔出，即时终止视频记录过程
		if(!IsCardExist(0))
		{
			// 卡拔出，终止记录过程
			XM_printf ("switch to saveless mode\n");
			return 2;			
		}
	}
	else
	{
		// 预览模式，无法保存视频
		
		// 检查视频项服务是否已开启	
		XMSYSTEMTIME CurrTime;
		// 检查视频项服务是否已开启, 检查卡是否插入, 检查RTC时间是否已设置
		if(	XM_VideoItemIsBasicServiceOpened() 
				// 检查卡是否插入
				&& IsCardExist(0)
				// 检查RTC时间是否已设置. 若RTC时间未设置, 不执行录像操作.	
				&& XM_GetLocalTime(&CurrTime)
			)	
		{
			// 视频项服务已开启, RTC时间已设置，终止当前无文件保存的记录过程, 返回, 准备执行文件保存的视频记录过程
			XM_printf ("switch to save mode\n");
			return 2;			
		}
	}
	#endif
		// 检查SD卡		
	if(stream->file_save)
	{
		// 文件保存模式, 可以保存视频
		
		// 检查是否已设置"强制非记录模式"
		if(forced_non_recording_mode)
		{
			// 终止当前的卡记录模式
			return 2;
		}
		
		// 若SD卡拔出，即时终止视频记录过程
		if(!IsCardExist(0))
		{
			// 卡拔出，终止记录过程
			return 2;			
		}
	}
	else
	{
		// 预览模式，无法保存视频
		
		// 检查是否存在 强制非记录模式 --> 普通记录模式 
		if(forced_switch_to_recording_mode)
		{
			// 终止非记录模式
			return 2;
		}
		
		// 检查视频项服务是否已开启	
		XMSYSTEMTIME CurrTime;
		// 检查视频项服务是否已开启, 检查卡是否插入, 检查RTC时间是否已设置
		if(	XM_VideoItemIsBasicServiceOpened() 
				// 检查卡是否插入
				&& IsCardExist(0)
				// 检查RTC时间是否已设置. 若RTC时间未设置, 不执行录像操作.	
				&& XM_GetLocalTime(&CurrTime)
				// 检查是否"普通记录模式"
				&& !forced_non_recording_mode
			)	
		{
			// 视频项服务已开启, RTC时间已设置，终止当前无文件保存的记录过程, 返回, 准备执行文件保存的视频记录过程
			XM_printf ("switch to save mode\n");
			return 2;			
		}
	}
	
	// 检查碰撞
	{
		if(stream->file_save)
		{
			// SD卡存在且允许写入	
			if(stream->file_lock == 0)
			{
				if(XMSYS_GSensorCheckAndClearCollisionEvent())
				{
					// 发生碰撞事件
					// 视频尚未保护
					// 执行锁定命令
					XM_printf ("video (%s) lock\n", stream->file_name[0]);
					int locked = AP_VideoItemLockVideoFileFromFileName (stream->file_name[0]);	// 当前只使用一个通道
					if(locked)
					{
					    video_lock_state = 1;//eason 20170920更新桌面锁定视频状态
						stream->file_lock = 1;
						//stream->stop_ticket += 1 * 60 * 1000;	// 录像中将当前录制的视频加锁，并将记录长度延长1分钟。
					}
					else
					{
						XM_printf ("lock (%s) NG\n", stream->file_name[0]);
					}
				}
			}
			else
			{
				// 视频已被保护, 不做处理
			}
		}
		else
		{
			// SD卡不存在
			//SD卡存在，但是停止录像，检查碰撞
			if(XMSYS_GSensorCheckAndClearCollisionEvent())
			{
				if(IsCardExist(0))
				{
					printf("1111111111 XMSYS_H264CodecGetForcedNonRecordingMode(): %d \n",XMSYS_H264CodecGetForcedNonRecordingMode());
				    if(XMSYS_H264CodecGetForcedNonRecordingMode())
				    {
				       	XMSYS_H264CodecSetForcedNonRecordingMode(0);
						XM_SetFmlDeviceCap (DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_START);
						XMSYS_H264CodecLockCurrentVideo();
						
						// 终止当前的卡记录模式
						return 2;
				    }
				}
			}
		}		
	}
	
	// 检查是否存在锁定或清除请求
	switch(video_lock_command)
	{
		case 0x01:	// 加锁
			if(stream->file_save)
			{
				if(stream->file_lock == 0)
				{
					// 视频尚未保护
					// 执行锁定命令
					/*
					int ret = AP_VideoItemLockVideoFileFromFileName (stream->file_name[0]);	// 当前只使用一个通道
					if(ret)
					{
						stream->file_lock = 1;
						video_lock_state = 1;
					}
					XM_printf ("lock (%s) %s\n", stream->file_name[0], ret ? "OK" : "NG");
					*/
					
					#if 1  //解决前后路同时加锁的问题
					if(ITU656_in == 1)
					{
						int ret = AP_VideoItemLockVideoFileFromFileName (stream->file_name[0])
								  &&AP_VideoItemLockVideoFileFromFileName (stream->file_name[1]);
						if(ret)
						{
							stream->file_lock = 1;
							video_lock_state = 1;
						}
						XM_printf ("lock (%s) (%s) %s\n", stream->file_name[0],stream->file_name[1], ret ? "OK" : "NG");
					}
					else if(ITU656_in == 0)
					{
						int ret = AP_VideoItemLockVideoFileFromFileName (stream->file_name[0]);	// 当前只使用一个通道
						if(ret)
						{
							stream->file_lock = 1;
							video_lock_state = 1;
						}
						XM_printf ("lock (%s) %s\n", stream->file_name[0], ret ? "OK" : "NG");
					}
					#endif
				}
				else
				{
					// 已锁定
				}
			}
			break;
			
		case 0x02:	// 解锁
			if(stream->file_save)
			{
				if(stream->file_lock == 1)
				{
					// 视频已保护
					// 执行解锁命令
					/*
					int ret = AP_VideoItemUnlockVideoFileFromFileName (stream->file_name[0]);	// 当前只使用一个通道
					if(ret)
					{
						stream->file_lock = 0;
						video_lock_state = 0;
					}
					XM_printf ("unlock (%s) %s\n", stream->file_name[0], ret ? "OK" : "NG");
					*/
					
					#if 1  //解决前后路同时解锁的问题
					if(ITU656_in == 1)
					{
						int ret = AP_VideoItemUnlockVideoFileFromFileName (stream->file_name[0])
								  &&AP_VideoItemUnlockVideoFileFromFileName (stream->file_name[1]);
						if(ret)
						{
							stream->file_lock = 0;
							video_lock_state = 0;
						}
						XM_printf ("unlock (%s) (%s) %s\n", stream->file_name[0],stream->file_name[1], ret ? "OK" : "NG");
					}
					else if(ITU656_in == 0)
					{
						int ret = AP_VideoItemUnlockVideoFileFromFileName (stream->file_name[0]);	// 当前只使用一个通道
						if(ret)
						{
							stream->file_lock = 0;
							video_lock_state = 0;
						}
						XM_printf ("unlock (%s) %s\n", stream->file_name[0], ret ? "OK" : "NG");
					}
					#endif
				}
				else
				{
					// 已解锁
				}
			}
			break;
	}
	// 清除锁定/解锁命令
	video_lock_command = 0;
	
	// 检查是否存在外部事件
	event = OS_GetEventsOccurred (&TCB_H264CodecTask);
	
#if 0
	// 检查一键保护事件
	if(event & XMSYS_H264_CODEC_EVENT_ONE_KEY_PROTECT)
	{
		// 一键保护请求
		char response_code = (char)(-1);
		// 过滤事件
		OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_ONE_KEY_PROTECT);
		if(stream->file_save)
		{
			// SD卡存在且允许写入			
			if(stream->file_lock == 0)
			{
				// 视频尚未保护
				// 执行锁定命令
				int locked = AP_VideoItemLockVideoFileFromFileName (stream->file_name[0]);	// 当前只使用一个通道
				if(locked)
				{
					stream->file_lock = 1;
					stream->stop_ticket += 3 * 60 * 1000;	// 录像中 将当前录制的视频加锁，并将记录长度延长3分钟。
					//XM_printf ("lock (%s) OK\n", stream->file_name[0]);
					response_code = 0;
				}
				else
				{
					
					XM_printf ("lock (%s) NG\n", stream->file_name[0]);
				}
			}
			else
			{
				// 视频已被保护
				response_code = 0;
			}
		}
		else
		{
			// SD卡不存在
			
		}
		// 应答客户端
		reply_response_message (response_code);			
	}
#endif
	
	// 外部命令(停止、回放)，触发codec结束
	if(event & XMSYS_H264_CODEC_EVENT_COMMAND)
	{
		int command;
		// 获取命令
		OS_Use(&h264codec_sema); /* Make sure nobody else uses */
		command = h264codec_command_param.command;
		OS_Unuse(&h264codec_sema);
		
		// 命令解析
		if(command == XMSYS_H264_CODEC_COMMAND_RECORDER_STOP)
		{
			// 录像停止命令
			XM_printf ("%d, Stoped due to RECORDER_STOP\n", XM_GetTickCount());
			// 返回2表示终止当前H264 Codec过程
			ret = 2;
			// 应答给主机“录像停止”的应答包一般会超时(文件关闭导致)。
			// 将应答包发送调整到此处，“录像停止”命令已执行
			reply_response_message(0);	
			return ret;
		}
		else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_START)
		{
			// 播放启动命令
			XM_printf ("%d, Stoped due to PLAYER_START\n", XM_GetTickCount());
			// 返回2表示终止当前H264 Codec过程
			ret = 2;
			return ret;
		}
		else
		{
			// 其他命令，全部过滤并将命令清除
			clear_codec_command ();
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
		}	
	}
	
	// 20170929
	// 关闭流之前2秒钟提前禁止或者延迟文件删除操作, 避免文件系统锁定导致的丢帧现象
	if( (XM_GetTickCount() + 2000) >= stream->stop_ticket )
	{
		XM_VideoItemSetDelayCutOff ();		// 延迟文件删除操作, 避免文件系统阻塞导致的丢帧现象		
	}
	
	// 检查是否记录已超时(视频分段)
	if(XM_GetTickCount() >= stream->stop_ticket)
	{
		// 终止编码退出
		XM_printf ("%d, Expired\n", XM_GetTickCount());
		XM_AppInitStartupTicket();//分段视频开始录重新计算时间
		return 3;
	}
	
	// 切换录像通道(前视、后视)
	if(event & XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL)
	{
		// 终止编码退出
		XM_printf ("%d, Switch\n", XM_GetTickCount());
		return 4;
	}
	
	// 等待直到下一次定时器事件
	// OS_DelayUntil (stream->curr_ticket);
	OS_Delay (1);
	return mask;
}

extern void *os_fopen (const char *filename, const char *mode);

void XMSYS_H264CodecOpenAudioRecord (void)
{
	XMWAVEFORMAT waveFormat;
	// 开启MIC
	waveFormat.wChannels = 1;			// 单声道
	waveFormat.wBitsPerSample = 16;
	waveFormat.dwSamplesPerSec = 8000;
	XM_WaveOpen (XMMCIDEVICE_RECORD, &waveFormat);
}

void XMSYS_H264CodecCloseAudioRecord (void)
{
	XM_WaveClose (XMMCIDEVICE_RECORD);
}

// 20170606 
// 修改video_index的用途, 改为通道号
int XMSYS_H264CodecGetAudioFrame (short *samples, int frame_size, int nb_channels, unsigned int video_index)
{
	int  readcnt ;
	// mono
	if(XM_WaveRead (XMMCIDEVICE_RECORD, (WORD *)samples, frame_size*nb_channels) <= 0)
		return 0;
	
	readcnt = frame_size*nb_channels;
	#if 0 //回放的时候暂时默认打开MIC
	if(AP_GetMenuItem(APPMENUITEM_MIC) == AP_SETTING_MIC_OFF)	// MIC关闭
	{
		memset (samples, 0, frame_size*nb_channels*2);
	}
	#endif
	return readcnt  ;
}



extern 	int h264_encode_avi_stream (char *filename, char *filename2, struct H264EncodeStream *H264Stream,
				const char **argv, int argc);
extern int XMSYS_WaitingForCacheSystemFlush (unsigned int timeout_ms);

// 返回值	标记退出原因
//	0		外部命令请求退出
// 	-1		内部异常导致退出
static int h264codec_RecorderStart (void)
{
	char h264name[VIDEOITEM_MAX_FILE_NAME];
	char h264name_f[VIDEOITEM_MAX_FILE_NAME];
	char h264name_r[VIDEOITEM_MAX_FILE_NAME];
	
	struct H264EncodeStream EncodeStream;
	HANDLE hVideoItem;
	XMVIDEOITEM *pVideoItem;
	u8_t event;
	//XMWAVEFORMAT waveFormat;
	unsigned int channel;
	XMSYSTEMTIME CurrTime;
	XMSYSTEMTIME LastTime;
	unsigned int delay_count;
	int ret;
	int rec_ret = 0;		// 返回值

	int voice_startup = 0;	// 标记"录像启动"声音是否已播放
	
	unsigned int video_format = AP_GetMenuItem (APPMENUITEM_VIDEO_RESOLUTION);
	
	// 配置录像帧率
	if(video_format == AP_SETTING_VIDEORESOLUTION_720P_30)
	{
		h264codec_frame_rate = 30;
	}
#if HONGJING_CVBS
	// 其他格式均为30帧
#else
	else if(video_format == AP_SETTING_VIDEORESOLUTION_720P_60)
	{
		h264codec_frame_rate = 60;
	}
#endif
	else //if(video_format == AP_SETTING_VIDEORESOLUTION_1080P_30)
	{
		// 缺省1080P 30fps
		h264codec_frame_rate = 30;
	}

	
#if HIGH_VIDEO_QUALITY
	//XM_printf ("HIGH_VIDEO_QUALITY\n");
#elif NORMAL_VIDEO_QUALITY
	//XM_printf ("NORMAL_VIDEO_QUALITY\n");
#else
	//XM_printf ("GOOD_VIDEO_QUALITY\n");
#endif
	
	// 检查ACC是否安全供电
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// 关机并等待ACC上电
		XM_ShutDownSystem (SDS_POWERACC);
	}

	open_recycle_task ();
	
	// 开启MIC
	XMSYS_H264CodecOpenAudioRecord ();	
	
	video_lock_state = 0;
	video_lock_command = 0;
	
	forced_encoder_stop = 0;

	memset (&LastTime, 0, sizeof(LastTime));

	
	// 记录启动
	while(1)
	{
		memset (&EncodeStream, 0, sizeof(EncodeStream));
					
		memset (h264name_f, 0, sizeof(h264name_f));
		memset (h264name_r, 0, sizeof(h264name_r));
		memset (h264name, 0, sizeof(h264name));
		
		// 新的视频文件,缺省非锁定
		video_lock_state = 0;
		
		hVideoItem = NULL;
		
		// 注意，现在的XM_VideoItemCreateVideoItemHandle 存在Bug，没有释放的代码
		//XM_printf ("Please resolve bugs within XM_VideoItemCreateVideoItemHandle\n");
		// 获取一个新的视频项文件名
		channel = XMSYS_SensorGetCameraChannel ();
		
		// 强制非记录模式
		if(forced_non_recording_mode)
		{
			hVideoItem = NULL;
			sprintf (h264name_f, "\\TEST.AVI");
			strcpy (EncodeStream.file_name[0], h264name_f);
			EncodeStream.file_save = 0;
			goto prepare_to_record;
		}
		
		// XM_GetLocalTime(&CurrTime);
		delay_count = 0;
		do
		{
			XM_GetLocalTime(&CurrTime);
			// 快速的录像开启/关闭会在相同的时间内(同一秒内)产生2次录像动作, 导致相同的录像文件命名, 前一个同名文件会被删除.
			// 因为前一个同名流文件尚未完全关闭(由其他线程触发关闭操作), 会导致删除失败.
			// 需要避开上情况
			// 检查时间是否一致. 若一致, 延时, 
			if(!check_same_video_time (&CurrTime, &LastTime))
				break;
			OS_Delay (10);
			delay_count ++;
			// XM_printf ("Delay %d\n", delay_count);
		} while (delay_count < 100);		// 最大延时1秒
		LastTime = CurrTime;
		
		// 检查系统RTC时间是否已设置。ARK1930的后视镜主板没有RTC，需要外部设置RTC时间。
		//if(!XM_GetLocalTime(&CurrTime))
		{
			// 若未设置，执行录像命令但不保存
			// XM_printf ("RTC not setup, save NG\n");
		//	hVideoItem = NULL;
		}
		//else
		{
			hVideoItem = XM_VideoItemCreateVideoItemHandle (channel, &CurrTime);
		}
		if(hVideoItem == NULL)
		{
		create_video_item_failed:
			//XM_printf ("XM_VideoItemCreateVideoItemHandle failed\n");
			// 创建临时视频文件，用于记录视频。
			// 	当存储卡第一次插入时，某些情况下获取卡文件系统容量会持续较长时间(10秒~30秒)，导致视频项数据库暂时无法使用，
			// 	而导致无法记录视频。
			// 	为解决上述问题，创建一个临时视频记录文件，用于保存实时视频。
			/*if(XM_VideoItemCreateVideoTempFile (channel, h264name_f, sizeof(h264name_f)))
			{
				// 创建一个临时视频文件成功
				strcpy (EncodeStream.file_name[0], h264name_f);
				EncodeStream.file_save = 1;
				//EncodeStream.file_temp = 1;	// 标记是临时视频项文件
			}
			else*/
			{
				// 创建一个逻辑视频文件(无法写入)
				//XM_KeyEventProc (VK_AP_VIDEOSTOP, (unsigned char)3);
				sprintf (h264name_f, "\\TEST.AVI");
				EncodeStream.file_save = 0;
				strcpy (EncodeStream.file_name[0], h264name_f);
			}
		}
		else
		{
			// 视频句柄创建成功
			pVideoItem = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
			if(pVideoItem == NULL)
			{
				hVideoItem = NULL;
				goto create_video_item_failed;
			}
			// 获取视频项的路径名
			if(XM_VideoItemGetVideoFilePath (pVideoItem, channel, h264name_f, sizeof(h264name_f)))
			{
				// 保存文件名
				strcpy (EncodeStream.file_name[0], h264name_f);
				EncodeStream.file_save = 1;
			}
			else
				goto create_video_item_failed;
		}

	prepare_to_record:
		// 设置当前使用的分辨率
		EncodeStream.width[0] = XMSYS_SensorGetCameraChannelWidth (channel);
		EncodeStream.height[0] = XMSYS_SensorGetCameraChannelHeight (channel);	
		
		XM_printf ("codec %s, w=%d, h=%d, %s\n", EncodeStream.file_name[0],
				  EncodeStream.width[0],
				  EncodeStream.height[0],
				  EncodeStream.file_save ? "save" : "look");		
		
		EncodeStream.type = 1;
		
		EncodeStream.audio_fifo = audio_fifo;
		EncodeStream.audio_data = audio_data;
			
		EncodeStream.frame_rate[0] = h264codec_frame_rate;
		EncodeStream.frame_rate[1] = h264codec_frame_rate;
		EncodeStream.frame_rate[2] = h264codec_frame_rate;
		EncodeStream.frame_number = 0;
		EncodeStream.start_ticket = OS_GetTime();
		EncodeStream.curr_ticket = EncodeStream.start_ticket;
		
		// 设置视频记录时间长度
		XM_printf(">>>>>>>>>>>>>>>>>>>>VideoTimeSize=%d\n", AP_GetVideoTimeSize());
		if(EncodeStream.file_save)
			EncodeStream.stop_ticket = OS_GetTime() + 1000 * AP_GetVideoTimeSize() + 250;		// 250 修正AVI文件时间长度的误差
		else
		{
			EncodeStream.stop_ticket = OS_GetTime() + 1000 * 25;	
			// 每隔25秒提醒
			if(!XMSYS_H264CodecGetForcedNonRecordingMode())
			{
				// 卡未插入/卡无效/卡文件系统错误时, 每隔25秒语音及UI提醒
				if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
					XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM);
			}
			voice_startup = 0;
		}
		
		// 检查是否播放"录像启动"声音
		if(voice_startup == 0 && EncodeStream.file_save)
		{
			// 播放"录像启动"声音
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				XM_voice_prompts_insert_voice (XM_VOICE_ID_STARTUP);
			voice_startup = 1;
		}

		//XM_printf ("%d Enter %s\n",  XM_GetTickCount(), h264name_f);
		forced_switch_to_recording_mode = 0;
		
		// 启动H264编码，等待返回(外部命令、记录超时等事件)
		if(video_format == AP_SETTING_VIDEORESOLUTION_1080P_30)
		{
			unsigned int video_quality = AP_GetMenuItem (APPMENU_IMAGE_QUALITY);
			if(video_quality == AP_SETTING_VIDEO_QUALITY_NORMAL)
			{
				ret = h264_encode_avi_stream (h264name_f, NULL, 
										&EncodeStream,
										argv_1080P_normal_video_quality,
										sizeof(argv_1080P_normal_video_quality)/sizeof(char *));
			}
			else if(video_quality == AP_SETTING_VIDEO_QUALITY_GOOD)
			{
				ret = h264_encode_avi_stream (h264name_f, NULL, 
										&EncodeStream,
										argv_1080P_good_video_quality,
										sizeof(argv_1080P_good_video_quality)/sizeof(char *));	
			}
			else
			{
				ret = h264_encode_avi_stream (h264name_f, NULL, 
										&EncodeStream,
										argv_1080P_high_video_quality,
										sizeof(argv_1080P_high_video_quality)/sizeof(char *));
				
			}
		}
#if HONGJING_CVBS
		else if(video_format == AP_SETTING_VIDEORESOLUTION_720P_30)
		{
			ret = h264_encode_avi_stream (h264name_f, NULL, 
										&EncodeStream,
										argv_720P,
										sizeof(argv_720P)/sizeof(char *));
			
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_720X480P)
		{
			ret = h264_encode_avi_stream (h264name_f, NULL, 
										&EncodeStream,
										argv_720X480P,
										sizeof(argv_720X480P)/sizeof(char *));
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_640X480P)
		{
			ret = h264_encode_avi_stream (h264name_f, NULL, 
										&EncodeStream,
										argv_640X480P,
										sizeof(argv_640X480P)/sizeof(char *));
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_320X240P)
		{
			ret = h264_encode_avi_stream (h264name_f, NULL, 
										&EncodeStream,
										argv_320X240P,
										sizeof(argv_320X240P)/sizeof(char *));
		}
		
#else
		else if(video_format == AP_SETTING_VIDEORESOLUTION_720P_30
				  ||video_format == AP_SETTING_VIDEORESOLUTION_720P_60)
		{
			ret = h264_encode_avi_stream (h264name_f, NULL, 
										&EncodeStream,
										argv_720P,
										sizeof(argv_720P)/sizeof(char *));
			
		}
#endif
		else
		{
			XM_printf ("h264codec_RecorderStart failed, illegal video format(%d)\n", video_format);
			rec_ret = -1;
			break;
		}

		//XM_printf ("%d Leave, ret=%d\n",  XM_GetTickCount(), ret);
						
		if(ret != 0)
		{
			// 非0值表示异常
			XM_printf ("Codec stop due to exception, code = %d\n", ret);
			
			// 等待视频句柄关闭
			//if(hVideoItem)
			//	XM_VideoItemWaitingForUpdate (EncodeStream.file_name[0]);
			//XMSYS_WaitingForCacheSystemFlush (500);
			
			//break;
			// 20170602 codec编码异常/拔卡等会导致退出. 此时不退出循环, 等待外部命令
			//if(IsCardExist(0))
			//{
				// 投递异常信息
				// XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_VERIFY_ERROR);
			//}
		}
		
		// 检查是否停车监控启动
		if(XMSYS_GSensorCheckParkingCollisionStartup())
		{
			// 检查是否是从预览模式切换到保存模式
			if(EncodeStream.file_save == 0 && IsCardExist(0))
			{
				OS_Delay (100);
				if(XM_VideoItemIsBasicServiceOpened())
				{
					XM_printf ("g-sensor, switch to save mode\n");
					goto switch_to_save_state;
				}
			}
			// 停车监控启动方式
			XM_printf ("Parking Collision Recording Close\n");
			
			XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 0);

			close_recycle_task ();
			
			// 等待视频锁定并关闭	
			// 锁定视频项
			AP_VideoItemLockVideoFileFromFileName (EncodeStream.file_name[0]);
			// 等待视频项关闭
			XM_VideoItemWaitingForUpdate (EncodeStream.file_name[0]);
			XM_printf ("%d, XM_VideoItemWaitingForUpdate\n", XM_GetTickCount());

			// 等待Cache文件系统将数据写入到SD卡
			XMSYS_WaitingForCacheSystemFlush (5000);			
			
			// 关机并等待ACC上电
			XM_ShutDownSystem (SDS_POWERACC);
		}
		
		// 检查ACC是否从ACC上电模式切换到ACC关断模式
		if(xm_power_check_acc_safe_or_not() == 0)
		{
			// ACC上电-->ACC下电
			XM_printf ("ACC Power Down\n");
			
			XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 0);
			
			close_recycle_task ();
			
			// 等待视频锁定并关闭	
			XM_VideoItemWaitingForUpdate (EncodeStream.file_name[0]);
			XM_printf ("%d, XM_VideoItemWaitingForUpdate\n", XM_GetTickCount());

			// 等待Cache文件系统将数据写入到SD卡
			XMSYS_WaitingForCacheSystemFlush (5000);			
			
			// 关机并等待ACC上电
			XM_ShutDownSystem (SDS_POWERACC);
		}
		
	switch_to_save_state:
		// 检查退出原因
		event = OS_GetEventsOccurred (&TCB_H264CodecTask);
		
		if(event & XMSYS_H264_CODEC_EVENT_COMMAND)
		{
			// 外部命令
			XM_printf ("%d, Exit Codec Loop\n", XM_GetTickCount());
			
			// 等待视频句柄关闭
			if(EncodeStream.file_save)
			{
				XM_VideoItemWaitingForUpdate (EncodeStream.file_name[0]);
				XM_printf ("%d, XM_VideoItemWaitingForUpdate\n", XM_GetTickCount());
			}
			XMSYS_WaitingForCacheSystemFlush (500);
			rec_ret = 0;
			break;
		}
		else if(event & XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL)
		{
			// 通道切换命令
			// 将事件过滤
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL);
			// 应答客户端
			reply_response_message (0);
			XM_printf ("switch channel\n");
			// 停止当前通道的sensor采集
			XMSYS_SensorCaptureStop ();			
			
			// 启动新的通道的视频采集过程
			XMSYS_SensorCaptureStart ();
		}
					
		// 记录文件超时退出，开始下一个录像记录
		//XM_printf ("h264_encoder_double_stream end\n");	
	}
	
	// XM_WaveClose (XMMCIDEVICE_RECORD);
	XMSYS_H264CodecCloseAudioRecord ();
		
	close_recycle_task ();
	
	return rec_ret;
}

extern int Sensor1_In_Flag;
extern int Sensor2_In_Flag;
extern unsigned int SensorData_CH0_Format;
extern unsigned int SensorData_CH1_Format;
extern unsigned int SensorData_CH2_Format;

extern 	int h264_encode_avi_stream_two_channel (char *filename, char *filename2, struct H264EncodeStream *H264Stream,
				const char **argv_fore, int argc_fore,		// 前置视频编码参数
				const char **argv_back, int argc_back		// 后置视频编码参数
				);

// 双路录像(前1080p + 后拉cvbs)
// 返回值	标记退出原因
//	0		外部命令请求退出
// 	-1		内部异常导致退出
static int h264codec_RecorderStart_two_channel (void)
{	
	struct H264EncodeStream EncodeStream;
	HANDLE hVideoItem[XMSYS_SENSOR_COUNT];
	XMVIDEOITEM *pVideoItem[XMSYS_SENSOR_COUNT];
	u8_t event;
	//XMWAVEFORMAT waveFormat;
	unsigned int channel;
	unsigned int channel_count;

	XMSYSTEMTIME CurrTime;
	XMSYSTEMTIME LastTime;
	unsigned int delay_count;
	int ret;
	int rec_ret = 0;		// 返回值
	
	int voice_startup = 0;	// 标记"录像启动"声音是否已播放
	unsigned int camera_format = 2;//camera_get_video_format();
	unsigned int video_format = AP_GetMenuItem (APPMENUITEM_VIDEO_RESOLUTION);

	XM_printf(">>>>>>>video_format:%d\r\n", video_format);

	#if 0
	// 配置录像帧率
	if(video_format == AP_SETTING_VIDEORESOLUTION_720P_30)
	{
		h264codec_frame_rate = 30;
	}
#if HONGJING_CVBS
	
#else
	else if(video_format == AP_SETTING_VIDEORESOLUTION_720P_60)
	{
		h264codec_frame_rate = 60;
	}
#endif
	else //if(video_format == AP_SETTING_VIDEORESOLUTION_1080P_30)
	{
		// 缺省1080P 30fps
		h264codec_frame_rate = 30;
	}
	#endif

	if( (SensorData_CH0_Format==AP_DSTRESOURCE_720P_25) && (SensorData_CH1_Format==AP_DSTRESOURCE_720P_25) )
	{
		h264codec_frame_rate = 25;
	}
	else if( (SensorData_CH0_Format==AP_DSTRESOURCE_720P_30) && (SensorData_CH1_Format==AP_DSTRESOURCE_720P_30) )
	{
		h264codec_frame_rate = 30;
	}
	else
	{
		h264codec_frame_rate = 25;
	}
	
	XM_printf(">>>>1 h264codec_frame_rate:%d\r\n", h264codec_frame_rate);
	//参考IAR工具设置的宏定义
	#if HIGH_VIDEO_QUALITY
	//XM_printf ("HIGH_VIDEO_QUALITY\n");
	#elif NORMAL_VIDEO_QUALITY
	XM_printf("NORMAL_VIDEO_QUALITY\n");
	#else
	//XM_printf ("GOOD_VIDEO_QUALITY\n");
	#endif
	
	// 检查ACC是否安全供电
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// 关机并等待ACC上电
		XM_ShutDownSystem (SDS_POWERACC);
	}
		
	open_recycle_task ();
	
	// 开启MIC
	XMSYS_H264CodecOpenAudioRecord();	
	
	video_lock_state = 0;
	video_lock_command = 0;
	forced_encoder_stop = 0;
	
	memset (hVideoItem, 0, sizeof(hVideoItem));
	memset (pVideoItem, 0, sizeof(pVideoItem));
	memset (&LastTime, 0, sizeof(LastTime));
		
	// 记录启动
	while(1)
	{
		memset (&EncodeStream, 0, sizeof(EncodeStream));
		memset (hVideoItem, 0, sizeof(hVideoItem));
		memset (pVideoItem, 0, sizeof(pVideoItem));
		
		#if 0
		if(ITU656_in == 1)
			channel_count = 2;
		else
			channel_count = 1;
		#else
		//if(Sensor1_In_Flag==1 && Sensor2_In_Flag==1)
		channel_count = 2;
		//if((Sensor1_In_Flag==1 && Sensor2_In_Flag==0)||(Sensor1_In_Flag==0 && Sensor2_In_Flag==1))
		//channel_count = 1;
		//if(Sensor1_In_Flag==0 && Sensor2_In_Flag==0)
		//channel_count = 0;
		#endif

		// XM_GetLocalTime(&CurrTime);
		delay_count = 0;
		do
		{
			XM_GetLocalTime(&CurrTime);
			// 快速的录像开启/关闭会在相同的时间内(同一秒内)产生2次录像动作, 导致相同的录像文件命名, 前一个同名文件会被删除.
			// 因为前一个同名流文件尚未完全关闭(由其他线程触发关闭操作), 会导致删除失败.
			// 需要避开上情况
			// 检查时间是否一致. 若一致, 延时, 
			if(!check_same_video_time(&CurrTime, &LastTime))
				break;
			OS_Delay (10);
			delay_count ++;
			// XM_printf ("Delay %d\n", delay_count);
		} while (delay_count < 100);		// 最大延时1秒
		LastTime = CurrTime;
		
		// 新的视频文件,缺省非锁定
		video_lock_state = 0;
		
		//强制非记录模式
		if(forced_non_recording_mode)
		{
			unsigned int width, height;
			width = XMSYS_SensorGetCameraChannelWidth(0);
			height = XMSYS_SensorGetCameraChannelHeight(0);	
			if(width == 0 || height == 0)
				break;
			
			EncodeStream.width[0] = width;
			EncodeStream.height[0] = height;
			
			strcpy (EncodeStream.file_name[0], "\\TEST.AVI");
			EncodeStream.file_save = 0;
			goto prepare_to_record;
		}
					
		// 锁定, 保证多个通道命名的一致性(确保同时使用临时文件名或者视频文件名)
		XM_VideoItemLock ();
		for (channel = 0; channel < channel_count; channel ++)
		{
			// 设置当前使用的分辨率
			unsigned int width, height;
			width = XMSYS_SensorGetCameraChannelWidth (channel);
			height = XMSYS_SensorGetCameraChannelHeight (channel);	
			if(width == 0 || height == 0)
			{
				// 通道0的尺寸不能为0
				if(channel == 0)
				{
					XM_printf ("Channel 0's width(%d) or height(%d) illegal\n", width, height);
					break;
				}
				continue;
			}
			
			EncodeStream.width[channel] = width;
			EncodeStream.height[channel] = height;
						
			// 根据当前时间命名视频文件名
			// 检查是否停车监控启动
			#if 0//屏蔽掉gsensor类型
			if(XMSYS_GSensorCheckParkingCollisionStartup())
			{
			   	hVideoItem[channel] = XM_VideoItemCreateVideoItemHandleEx (channel, &CurrTime,XM_FILE_TYPE_VIDEO,XM_VIDEOITEM_CLASS_URGENT,XM_VIDEOITEM_VOLUME_0);
			}
			else
			{
				hVideoItem[channel] = XM_VideoItemCreateVideoItemHandle (channel, &CurrTime);
			}
			#endif
			
			hVideoItem[channel] = XM_VideoItemCreateVideoItemHandle (channel, &CurrTime);
			
			if(hVideoItem[channel] == NULL)
			{
				create_video_item_failed:
				XM_printf(">>>>XM_VideoItemCreateVideoItemHandle failed\r\n");
				// 创建临时视频文件，用于记录视频。
				// 	当存储卡第一次插入时，某些情况下获取卡文件系统容量会持续较长时间(10秒~30秒)，导致视频项数据库暂时无法使用，
				// 	而导致无法记录视频。
				// 	为解决上述问题，创建一个临时视频记录文件，用于保存实时视频。
				/*if( XM_VideoItemCreateVideoTempFile (channel, EncodeStream.file_name[channel], sizeof(EncodeStream.file_name[channel])))
				{
					// 创建一个临时视频文件成功
					EncodeStream.file_save = 1;
				}
				else*/
				{
					// 创建一个逻辑视频文件(无法写入)
					EncodeStream.file_save = 0;
					break;
				}
			}
			else
			{
				// 视频句柄创建成功
				pVideoItem[channel] = AP_VideoItemGetVideoItemFromHandle (hVideoItem[channel]);
				if(pVideoItem[channel] == NULL)
				{
					hVideoItem[channel] = NULL;
					goto create_video_item_failed;
				}
				// 获取视频项的路径名
				if(XM_VideoItemGetVideoFilePath (pVideoItem[channel], channel, EncodeStream.file_name[channel], sizeof(EncodeStream.file_name[channel])))
				{
					// 保存文件名
					EncodeStream.file_save = 1;
				}
				else
				{
					goto create_video_item_failed;
				}
			}	
		}
		
		XM_VideoItemUnlock ();

		/*
		// 检查通道0是否有效
		if(EncodeStream.width[0] == 0 || EncodeStream.height[0] == 0)
		{
			XM_printf ("h264codec_RecorderStart failed, illegal size(w=%d, h=%d) of channel 0\n", EncodeStream.width[0], EncodeStream.height[0]);
			rec_ret = -1;
			break;
		}
		*/
		
		if(EncodeStream.file_save == 0)
		{
			// 预览模式
			// 仅通道0给出文件名, 其他通道的文件名为空
			for (channel = 0; channel < channel_count; channel ++)
			{
				if(channel == 0)
					sprintf (EncodeStream.file_name[0], "\\TEST.AVI");
				else
					memset (EncodeStream.file_name[channel], 0, sizeof(EncodeStream.file_name[channel]));
			}
		}
		
	prepare_to_record:
		XM_printf(">>>>>>prepare_to_record......\r\n");
		EncodeStream.type = 1;
		
		EncodeStream.audio_fifo = audio_fifo;
		EncodeStream.audio_data = audio_data;
			
		EncodeStream.frame_rate[0] = h264codec_frame_rate;
		EncodeStream.frame_rate[1] = h264codec_frame_rate;
		EncodeStream.frame_number = 0;
		EncodeStream.start_ticket = OS_GetTime();
		EncodeStream.curr_ticket = EncodeStream.start_ticket;
		current_video_start_ticket = OS_GetTime();
		
		//设置视频记录时间长度
		//XM_printf("VideoTimeSize=%d\n", AP_GetVideoTimeSize());
		XM_printf(">>>>EncodeStream.file_save=%d\n", EncodeStream.file_save);
		if(EncodeStream.file_save)
		{
			EncodeStream.stop_ticket = OS_GetTime() + 1000 * AP_GetVideoTimeSize() + 300;		// 300 修正AVI文件时间长度的误差
		}
		else
		{
			EncodeStream.stop_ticket = OS_GetTime() + 1000 * 25;	
			// 每隔25秒提醒
			//if(!XMSYS_H264CodecGetForcedNonRecordingMode())
			if(!XMSYS_H264CodecGetForcedNonRecordingMode() && PowerOnShowLogo)
			{
				// 卡未插入/卡无效/卡文件系统错误时, 每隔25秒语音及UI提醒
				if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
					XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM);
			}
			voice_startup = 0;
		}
		
		//检查是否播放"录像启动"声音
		if(voice_startup == 0 && EncodeStream.file_save)
		{
			// 播放"录像启动"声音
			if(AP_GetMenuItem(APPMENUITEM_VOICE_PROMPTS))
				XM_voice_prompts_insert_voice(XM_VOICE_ID_STARTUP);
			voice_startup = 1;
		}
		
		forced_switch_to_recording_mode = 0;
		
		video_format = SensorData_Get_FormatInfo();
		XM_printf(">>>>2 video_format:%d\r\n", video_format);
		XM_printf(">>>>2 h264codec_frame_rate:%d\r\n", h264codec_frame_rate);
		// 启动H264编码，等待返回(外部命令、记录超时等事件)
		#if 0
		if(video_format == AP_SETTING_VIDEORESOLUTION_1080P_720P_30)
		{
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_1080P,
										sizeof(argv_1080P)/sizeof(char *),
										argv_720P,
										sizeof(argv_720P)/sizeof(char *)
										);
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_1080P_NSTL_30)
		{
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_1080P,
										sizeof(argv_1080P)/sizeof(char *),
										argv_cvbs_ntsc,
										sizeof(argv_cvbs_ntsc)/sizeof(char *)
										);
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_1080P_PAL_30)
		{
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_1080P,
										sizeof(argv_1080P)/sizeof(char *),
										argv_cvbs_pal,
										sizeof(argv_cvbs_pal)/sizeof(char *)
										);
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_720P_720P_30)
		{
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_720P,
										sizeof(argv_720P)/sizeof(char *),
										argv_720P,
										sizeof(argv_720P)/sizeof(char *)
										);
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_NSTL_720P_30)
		{
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_cvbs_ntsc,
										sizeof(argv_cvbs_ntsc)/sizeof(char *),
										argv_720P,
										sizeof(argv_720P)/sizeof(char *)
										);
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_NSTL_NSTL_30)
		{
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_cvbs_ntsc,
										sizeof(argv_cvbs_ntsc)/sizeof(char *),
										argv_cvbs_ntsc,
										sizeof(argv_cvbs_ntsc)/sizeof(char *)
										);
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_NSTL_PAL_30)
		{
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_cvbs_ntsc,
										sizeof(argv_cvbs_ntsc)/sizeof(char *),
										argv_cvbs_pal,
										sizeof(argv_cvbs_pal)/sizeof(char *)
										);
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_PAL_720P_30)
		{
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_cvbs_pal,
										sizeof(argv_cvbs_pal)/sizeof(char *),
										argv_720P,
										sizeof(argv_720P)/sizeof(char *)
										);
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_PAL_NSTL_30)
		{
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_cvbs_pal,
										sizeof(argv_cvbs_pal)/sizeof(char *),
										argv_cvbs_ntsc,
										sizeof(argv_cvbs_ntsc)/sizeof(char *)
										);
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_PAL_PAL_30)
		{
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_cvbs_pal,
										sizeof(argv_cvbs_pal)/sizeof(char *),
										argv_cvbs_pal,
										sizeof(argv_cvbs_pal)/sizeof(char *)
										);
		}
		else 
		{
			XM_printf ("h264codec_RecorderStart failed, illegal video format(%d)\n", video_format);
			rec_ret = -1;
			break;
		}
		#endif
		if( (video_format == AP_SETTING_VIDEORESOLUTION_720P_720P_25) || (video_format == AP_SETTING_VIDEORESOLUTION_720P_720P_30) )
		{
			#ifdef EN_RN6752M1080N_CHIP
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_1080n,
										sizeof(argv_1080n)/sizeof(char *),
										argv_1080n,
										sizeof(argv_1080n)/sizeof(char *));	
			#else
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_720P,
										sizeof(argv_720P)/sizeof(char *),
										argv_720P,
										sizeof(argv_720P)/sizeof(char *));
			#endif
		}
		else 
		{
			XM_printf(">>>>h264codec_RecorderStart failed, illegal video format(%d)\n", video_format);
			rec_ret = -1;
			break;
		}
		
		if(ret != 0)
		{
			// 非0值表示异常
			XM_printf ("Codec stop due to exception, code = %d\n", ret);
			
			//break;
			// 20170602 codec编码异常/拔卡等会导致退出. 此时不退出循环, 等待外部命令
			//if(IsCardExist(0))
			//{
				// 投递异常信息
				//XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_VERIFY_ERROR);
			//}
		}
		
		// 检查是否停车监控启动
		if(XMSYS_GSensorCheckParkingCollisionStartup())
		{
			// 检查是否是从预览模式切换到保存模式
			if(EncodeStream.file_save == 0 && IsCardExist(0))
			{
				OS_Delay (100);
				if(XM_VideoItemIsBasicServiceOpened())
				{
					XM_printf ("g-sensor, switch to save mode\n");
					goto switch_to_save_state;
				}
			}
			
			// 停车监控启动方式
			XM_printf ("Parking Collision Recording Close\n");
			
			//XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 0);
			
			close_recycle_task ();
			
			// 等待视频句柄关闭并锁定
			if(EncodeStream.file_save)
			{
				XM_printf ("waiting for video Lock & close ...\n");
				for (channel = 0; channel < channel_count; channel ++)
				{
					// 锁定视频项
					AP_VideoItemLockVideoFileFromFileName (EncodeStream.file_name[channel]);
					// 等待视频项关闭
					XM_VideoItemWaitingForUpdate (EncodeStream.file_name[channel]);
				}
				XM_printf ("waiting for video Lock & close end\n");
			}
			
			// 等待Cache文件系统将数据写入到SD卡
			XMSYS_WaitingForCacheSystemFlush (5000);	

			//tony.yue 20180305 添加关机显示LOGO
			#if 0
			{
				//OS_EnterRegion ();	// 禁止任务切换
				PowerOnShowLogo=0;
                if(AP_GetLogo())
                    ShowLogo (ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG,ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG_SIZE);
	            OS_Delay (2000);
				//OS_LeaveRegion();
			}
			#endif
			
			// 关机并等待ACC上电
			XM_ShutDownSystem (SDS_POWERACC);
		}

		// 检查ACC是否从ACC上电模式切换到ACC关断模式
		if(xm_power_check_acc_safe_or_not() == 0)
		{
			// ACC上电-->ACC下电
			XM_printf ("ACC Power Down\n");
			
			//XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 0);
			
			close_recycle_task ();
			
			// 等待视频句柄关闭并锁定
			if(EncodeStream.file_save)
			{
				XM_printf ("waiting for video close ...\n");
				for (channel = 0; channel < channel_count; channel ++)
				{
					// 等待视频项关闭
					XM_VideoItemWaitingForUpdate (EncodeStream.file_name[channel]);
				}
				XM_printf ("waiting for video close end\n");
			}
			
			// 等待Cache文件系统将数据写入到SD卡
			XMSYS_WaitingForCacheSystemFlush (5000);			

			//tony.yue 20180305 添加关机显示LOGO
			{
				//OS_EnterRegion ();	// 禁止任务切换
				PowerOnShowLogo=0;
                //ShowLogo (ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG,ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG_SIZE);
	            OS_Delay (2000);
				//OS_LeaveRegion();
			}
			
			// 20180414 zhuoyonghong
			// 关闭文件系统服务
			xm_close_volume_service ("mmc:0:");
			
			// 关机并等待ACC上电
			XM_ShutDownSystem (SDS_POWERACC);
		}		
		
	switch_to_save_state:	
		// 检查退出原因
		event = OS_GetEventsOccurred (&TCB_H264CodecTask);
		
		if(event & XMSYS_H264_CODEC_EVENT_COMMAND)
		{
			// 外部命令
			XM_printf ("%d, Exit Codec Loop\n", XM_GetTickCount());
			
			// 等待视频句柄关闭
			if(EncodeStream.file_save)
			{
				XM_printf ("%d,waiting for video close ...\n",XM_GetTickCount());
				for (channel = 0; channel < channel_count; channel ++)
				{
					XM_VideoItemWaitingForUpdate (EncodeStream.file_name[channel]);
				}
				XM_printf ("%d,waiting for video close end\n",XM_GetTickCount());
				XMSYS_WaitingForCacheSystemFlush (500); 
			}
			rec_ret = 0;
			break;
		}
		else if(event & XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL)
		{
			// 通道切换命令
			// 将事件过滤
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL);
			// 应答客户端
			reply_response_message (0);
			XM_printf ("switch channel\n");
			// 停止当前通道的sensor采集
			XMSYS_SensorCaptureStop ();			
			
			// 启动新的通道的视频采集过程
			XMSYS_SensorCaptureStart ();
		}	
		// 记录文件超时退出，开始下一个录像记录
		//XM_printf ("h264_encoder_double_stream end\n");	
	}
	
	// XM_WaveClose (XMMCIDEVICE_RECORD);
	XMSYS_H264CodecCloseAudioRecord ();
		
	close_recycle_task ();
	
	return rec_ret;
}

extern  int h264_encode_avi_stream_three_channel (char *filename, char *filename2, char *filename3,
											struct H264EncodeStream *H264Stream,
											const char **x264_argv, int x264_argc,
											const char **x264_argv2, int x264_argc2,		// 后置视频编码参数
											const char **x264_argv3, int x264_argc3		// 后置视频编码参数
											);
// 三路录像(前后720P + USB摄像头720P)
// 返回值	标记退出原因
//	0		外部命令请求退出
// 	-1		内部异常导致退出
static int h264codec_RecorderStart_Three_channel (void)
{	
	struct H264EncodeStream EncodeStream;
	HANDLE hVideoItem[XMSYS_SENSOR_COUNT];
	XMVIDEOITEM *pVideoItem[XMSYS_SENSOR_COUNT];
	u8_t event;
	//XMWAVEFORMAT waveFormat;
	unsigned int channel;
	unsigned int channel_count;

	XMSYSTEMTIME CurrTime;
	XMSYSTEMTIME LastTime;
	unsigned int delay_count;
	int ret;
	int rec_ret = 0;		// 返回值
	
	int voice_startup = 0;	// 标记"录像启动"声音是否已播放
	unsigned int camera_format = 2;//camera_get_video_format();
	unsigned int video_format = AP_SETTING_VIDEORESOLUTION_720P_30;//AP_GetMenuItem (APPMENUITEM_VIDEO_RESOLUTION);
	// 配置录像帧率
	if(video_format == AP_SETTING_VIDEORESOLUTION_720P_30)
	{
		h264codec_frame_rate = 30;
	}
#if HONGJING_CVBS
	
#else
	else if(video_format == AP_SETTING_VIDEORESOLUTION_720P_60)
	{
		h264codec_frame_rate = 60;
	}
#endif
	else //if(video_format == AP_SETTING_VIDEORESOLUTION_1080P_30)
	{
		// 缺省1080P 30fps
		h264codec_frame_rate = 30;
	}

	
#if HIGH_VIDEO_QUALITY
	//XM_printf ("HIGH_VIDEO_QUALITY\n");
#elif NORMAL_VIDEO_QUALITY
	//XM_printf ("NORMAL_VIDEO_QUALITY\n");
#else
	//XM_printf ("GOOD_VIDEO_QUALITY\n");
#endif
	
	// 检查ACC是否安全供电
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// 关机并等待ACC上电
		XM_ShutDownSystem (SDS_POWERACC);
	}
		
	open_recycle_task ();
	
	// 开启MIC
	XMSYS_H264CodecOpenAudioRecord();	
	
	video_lock_state = 0;
	video_lock_command = 0;
	forced_encoder_stop = 0;
	
	memset (hVideoItem, 0, sizeof(hVideoItem));
	memset (pVideoItem, 0, sizeof(pVideoItem));
	memset (&LastTime, 0, sizeof(LastTime));
		
	// 记录启动
	while(1)
	{
		memset (&EncodeStream, 0, sizeof(EncodeStream));
		memset (hVideoItem, 0, sizeof(hVideoItem));
		memset (pVideoItem, 0, sizeof(pVideoItem));
		//过滤第一通道是1080P
		if(Compare_CH0_1080P_Status())
			channel_count = 2;
		else 
			channel_count = 3;
		// XM_GetLocalTime(&CurrTime);
		delay_count = 0;
		do
		{
			XM_GetLocalTime(&CurrTime);
			// 快速的录像开启/关闭会在相同的时间内(同一秒内)产生2次录像动作, 导致相同的录像文件命名, 前一个同名文件会被删除.
			// 因为前一个同名流文件尚未完全关闭(由其他线程触发关闭操作), 会导致删除失败.
			// 需要避开上情况
			// 检查时间是否一致. 若一致, 延时, 
			if(!check_same_video_time (&CurrTime, &LastTime))
				break;
			OS_Delay (10);
			delay_count ++;
			// XM_printf ("Delay %d\n", delay_count);
		} while (delay_count < 100);		// 最大延时1秒
		LastTime = CurrTime;
		
		// 新的视频文件,缺省非锁定
		video_lock_state = 0;
		
		// 强制非记录模式
		if(forced_non_recording_mode)
		{
			unsigned int width, height;
			width = XMSYS_SensorGetCameraChannelWidth(0);
			height = XMSYS_SensorGetCameraChannelHeight(0);	
			if(width == 0 || height == 0)
				break;
			EncodeStream.width[0] = width;
			EncodeStream.height[0] = height;
			XM_SetFmlDeviceCap (DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_STOP);
			strcpy (EncodeStream.file_name[0], "\\TEST.AVI");
			EncodeStream.file_save = 0;
			goto prepare_to_record;
		}
					
		// 锁定, 保证多个通道命名的一致性(确保同时使用临时文件名或者视频文件名)
		XM_VideoItemLock ();
		for (channel = 0; channel < channel_count; channel ++)
		{
			// 设置当前使用的分辨率
			unsigned int width, height;
			width = XMSYS_SensorGetCameraChannelWidth (channel);
			height = XMSYS_SensorGetCameraChannelHeight (channel);
			if(width == 0 || height == 0)
			{
				// 通道0的尺寸不能为0
				if(channel == 0)
				{
					XM_printf ("Channel 0's width(%d) or height(%d) illegal\n", width, height);
					break;
				}
				continue;
			}
			
			EncodeStream.width[channel] = width;
			EncodeStream.height[channel] = height;
			// 根据当前时间命名视频文件名
			// 检查是否停车监控启动
			if(XMSYS_GSensorCheckParkingCollisionStartup())
			{
				hVideoItem[channel] = XM_VideoItemCreateVideoItemHandleEx (channel, &CurrTime, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_URGENT, XM_VIDEOITEM_VOLUME_0);
			}
			else
			{
				hVideoItem[channel] = XM_VideoItemCreateVideoItemHandle (channel, &CurrTime);
			}
			
			if(hVideoItem[channel] == NULL)
			{
			create_video_item_failed:
				//XM_printf ("XM_VideoItemCreateVideoItemHandle failed\n");
				// 创建临时视频文件，用于记录视频。
				// 	当存储卡第一次插入时，某些情况下获取卡文件系统容量会持续较长时间(10秒~30秒)，导致视频项数据库暂时无法使用，
				// 	而导致无法记录视频。
				// 	为解决上述问题，创建一个临时视频记录文件，用于保存实时视频。
				/*if( XM_VideoItemCreateVideoTempFile (channel, EncodeStream.file_name[channel], sizeof(EncodeStream.file_name[channel])))
				{
					// 创建一个临时视频文件成功
					EncodeStream.file_save = 1;
				}
				else*/
				{
					// 创建一个逻辑视频文件(无法写入)
					EncodeStream.file_save = 0;
					break;
				}
			}
			else
			{
				// 视频句柄创建成功
				pVideoItem[channel] = AP_VideoItemGetVideoItemFromHandle (hVideoItem[channel]);
				if(pVideoItem[channel] == NULL)
				{
					hVideoItem[channel] = NULL;
					goto create_video_item_failed;
				}
				// 获取视频项的路径名
				if(XM_VideoItemGetVideoFilePath (pVideoItem[channel], channel, EncodeStream.file_name[channel], sizeof(EncodeStream.file_name[channel])))
				{
					// 保存文件名
					EncodeStream.file_save = 1;
				}
				else
				{
					goto create_video_item_failed;
				}          
			}	
		}
		
		XM_VideoItemUnlock ();
		
		/*
		// 检查通道0是否有效
		if(EncodeStream.width[0] == 0 || EncodeStream.height[0] == 0)
		{
			XM_printf ("h264codec_RecorderStart failed, illegal size(w=%d, h=%d) of channel 0\n", EncodeStream.width[0], EncodeStream.height[0]);
			rec_ret = -1;
			break;
		}
		*/
		
		if(EncodeStream.file_save == 0)
		{
			// 预览模式
			// 仅通道0给出文件名, 其他通道的文件名为空
			for (channel = 0; channel < channel_count; channel ++)
			{
				if(channel == 0)
					sprintf (EncodeStream.file_name[0], "\\TEST.AVI");
				else
					memset (EncodeStream.file_name[channel], 0, sizeof(EncodeStream.file_name[channel]));
			}
		}
		
	prepare_to_record:
		EncodeStream.type = 1;
		
		EncodeStream.audio_fifo = audio_fifo;
		EncodeStream.audio_data = audio_data;
			
		EncodeStream.frame_rate[0] = h264codec_frame_rate;
		EncodeStream.frame_rate[1] = h264codec_frame_rate;
		EncodeStream.frame_rate[2] = h264codec_frame_rate;
		EncodeStream.frame_number = 0;
		EncodeStream.start_ticket = OS_GetTime();
		EncodeStream.curr_ticket = EncodeStream.start_ticket;
		current_video_start_ticket = OS_GetTime();
		
		// 设置视频记录时间长度
		//XM_printf ("VideoTimeSize=%d\n", AP_GetVideoTimeSize());
		if(EncodeStream.file_save)
		{
			EncodeStream.stop_ticket = OS_GetTime() + 1000 * AP_GetVideoTimeSize() + 300;		// 100 修正AVI文件时间长度的误差
		}
		else
		{
			EncodeStream.stop_ticket = OS_GetTime() + 1000 * 25;	
			// 每隔25秒提醒
			//if(!XMSYS_H264CodecGetForcedNonRecordingMode())
			if(!XMSYS_H264CodecGetForcedNonRecordingMode() && PowerOnShowLogo)
			{
				// 卡未插入/卡无效/卡文件系统错误时, 每隔25秒语音及UI提醒
				if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
					XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM);
			}
			voice_startup = 0;
		}
		
		// 检查是否播放"录像启动"声音
		if(voice_startup == 0 && EncodeStream.file_save)
		{
			// 播放"录像启动"声音
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				XM_voice_prompts_insert_voice (XM_VOICE_ID_STARTUP);
			voice_startup = 1;
		}
		
		forced_switch_to_recording_mode = 0;
		
		ret = 0;
		// 启动H264编码，等待返回(外部命令、记录超时等事件)
		video_format = SensorData_Get_FormatInfo();
		//printf("video_format: %d \n",video_format);
		if(video_format == AP_SETTING_VIDEORESOLUTION_1080P_720P_30)
		{
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_1080P,
										sizeof(argv_1080P)/sizeof(char *),
										argv_720P,
										sizeof(argv_720P)/sizeof(char *)
										);
		}else if(video_format == AP_SETTING_VIDEORESOLUTION_1080P_NSTL_30)
		{
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_1080P,
										sizeof(argv_1080P)/sizeof(char *),
										argv_cvbs_ntsc,
										sizeof(argv_cvbs_ntsc)/sizeof(char *)
										);
		}else if(video_format == AP_SETTING_VIDEORESOLUTION_1080P_PAL_30)
		{
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_1080P,
										sizeof(argv_1080P)/sizeof(char *),
										argv_cvbs_pal,
										sizeof(argv_cvbs_pal)/sizeof(char *)
										);
		}else if(video_format == AP_SETTING_VIDEORESOLUTION_720P_720P_30) 
		{
			ret = h264_encode_avi_stream_three_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], EncodeStream.file_name[2],
											&EncodeStream,
											argv_720P, sizeof(argv_720P)/sizeof(char *),
											argv_720P, sizeof(argv_720P)/sizeof(char *),		// 后置视频编码参数
											argv_720P, sizeof(argv_720P)/sizeof(char *)		// 后置视频编码参数
											);
		}else if(video_format == AP_SETTING_VIDEORESOLUTION_720P_NSTL_30)
		{
			ret = h264_encode_avi_stream_three_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], EncodeStream.file_name[2],
											&EncodeStream,
											argv_720P, sizeof(argv_720P)/sizeof(char *),
											argv_cvbs_ntsc, sizeof(argv_cvbs_ntsc)/sizeof(char *),		// 后置视频编码参数
											argv_720P, sizeof(argv_720P)/sizeof(char *)		// 后置视频编码参数
											);
		}else if(video_format == AP_SETTING_VIDEORESOLUTION_720P_PAL_30)
		{
			ret = h264_encode_avi_stream_three_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], EncodeStream.file_name[2],
											&EncodeStream,
											argv_720P, sizeof(argv_720P)/sizeof(char *),
											argv_cvbs_pal, sizeof(argv_cvbs_pal)/sizeof(char *),		// 后置视频编码参数
											argv_720P, sizeof(argv_720P)/sizeof(char *)		// 后置视频编码参数
											);
		}
		#if 0
		if(video_format == AP_SETTING_VIDEORESOLUTION_1080P_30)
		{
		  //每次录新的文件判断有没有后拉输入,如果没有就不录后拉
		   if(channel_count == 2)
		   {
			ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], 
										&EncodeStream,
										argv_1080P,
										sizeof(argv_1080P)/sizeof(char *),
										argv_720P,
										sizeof(argv_720P)/sizeof(char *)
										);
		   }
		   else
		   {
            ret = h264_encode_avi_stream_two_channel (EncodeStream.file_name[0], 0, 
										&EncodeStream,
										argv_720P,
										sizeof(argv_720P)/sizeof(char *),
										// 7116使用VGA分辨率输出
										argv_720P,
										sizeof(argv_720P)/sizeof(char *)
										//argv_cvbs_ntsc,
										//sizeof(argv_cvbs_ntsc)/sizeof(char *)
										);
		   }
		}
		else if(video_format == AP_SETTING_VIDEORESOLUTION_720P_30
				||video_format == AP_SETTING_VIDEORESOLUTION_720P_60)
		{

            ret = h264_encode_avi_stream_three_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], EncodeStream.file_name[2],
											&EncodeStream,
											argv_1080P, sizeof(argv_1080P)/sizeof(char *),
											argv_720P, sizeof(argv_720P)/sizeof(char *),		// 后置视频编码参数
											argv_720P, sizeof(argv_720P)/sizeof(char *)		// 后置视频编码参数
											);
						

		}
		else
		{
			XM_printf ("h264codec_RecorderStart failed, illegal video format(%d)\n", video_format);
			rec_ret = -1;
			break;
		}
		#endif
		//XM_printf ("%d Leave, ret=%d\n",  XM_GetTickCount(), ret);
		if(ret != 0)
		{
			// 非0值表示异常
			XM_printf ("Codec stop due to exception, code = %d\n", ret);
			
			//break;
			// 20170602 codec编码异常/拔卡等会导致退出. 此时不退出循环, 等待外部命令
			//if(IsCardExist(0))
			//{
				// 投递异常信息
				//XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_VERIFY_ERROR);
			//}
		}
		
		// 检查是否停车监控启动
		if(XMSYS_GSensorCheckParkingCollisionStartup())
		{
			// 检查是否是从预览模式切换到保存模式
			if(EncodeStream.file_save == 0 && IsCardExist(0))
			{
				OS_Delay (100);
				if(XM_VideoItemIsBasicServiceOpened())
				{
					XM_printf ("g-sensor, switch to save mode\n");
					goto switch_to_save_state;
				}
			}
			
			// 停车监控启动方式
			XM_printf ("Parking Collision Recording Close\n");
			
			//XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 0);
			
			close_recycle_task ();
			
			// 等待视频句柄关闭并锁定
			if(EncodeStream.file_save)
			{
				XM_printf ("waiting for video Lock & close ...\n");
				for (channel = 0; channel < channel_count; channel ++)
				{
					// 锁定视频项
					AP_VideoItemLockVideoFileFromFileName (EncodeStream.file_name[channel]);
					// 等待视频项关闭
					XM_VideoItemWaitingForUpdate (EncodeStream.file_name[channel]);
				}
				XM_printf ("waiting for video Lock & close end\n");
			}
			
			// 等待Cache文件系统将数据写入到SD卡
			XMSYS_WaitingForCacheSystemFlush (5000);	

			//tony.yue 20180305 添加关机显示LOGO
			{
				//OS_EnterRegion ();	// 禁止任务切换
				PowerOnShowLogo=0;
                //if(AP_GetLogo())
                //    ShowLogo (ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG,ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG_SIZE);
	            OS_Delay (2000);
				//OS_LeaveRegion();
			}
						
			// 关机并等待ACC上电
			XM_ShutDownSystem (SDS_POWERACC);
		}

		// 检查ACC是否从ACC上电模式切换到ACC关断模式
		if(xm_power_check_acc_safe_or_not() == 0)
		{
			// ACC上电-->ACC下电
			XM_printf ("ACC Power Down\n");
			
			//XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 0);
			
			close_recycle_task ();
			
			// 等待视频句柄关闭并锁定
			if(EncodeStream.file_save)
			{
				XM_printf ("waiting for video close ...\n");
				for (channel = 0; channel < channel_count; channel ++)
				{
					// 等待视频项关闭
					XM_VideoItemWaitingForUpdate (EncodeStream.file_name[channel]);
				}
				XM_printf ("waiting for video close end\n");
			}
			
			// 等待Cache文件系统将数据写入到SD卡
			XMSYS_WaitingForCacheSystemFlush (5000);			

			//tony.yue 20180305 添加关机显示LOGO
			{
				//OS_EnterRegion ();	// 禁止任务切换
				PowerOnShowLogo=0;
                //ShowLogo (ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG,ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG_SIZE);
	            //OS_Delay (2000);
				//OS_LeaveRegion();
			}
			
			// 20180414 zhuoyonghong
			// 关闭文件系统服务
			xm_close_volume_service ("mmc:0:");
			
			// 关机并等待ACC上电
			XM_ShutDownSystem (SDS_POWERACC);
		}		
		
	switch_to_save_state:	
		// 检查退出原因
		event = OS_GetEventsOccurred (&TCB_H264CodecTask);
		
		if(event & XMSYS_H264_CODEC_EVENT_COMMAND)
		{
			// 外部命令
			XM_printf ("%d, Exit Codec Loop\n", XM_GetTickCount());
			
			// 等待视频句柄关闭
			if(EncodeStream.file_save)
			{
				XM_printf ("%d,waiting for video close ...\n",XM_GetTickCount());
				for (channel = 0; channel < channel_count; channel ++)
				{
					XM_VideoItemWaitingForUpdate (EncodeStream.file_name[channel]);
				}
				XM_printf ("%d,waiting for video close end\n",XM_GetTickCount());
				XMSYS_WaitingForCacheSystemFlush (500); 
			}
			rec_ret = 0;
			break;
		}
		else if(event & XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL)
		{
			// 通道切换命令
			// 将事件过滤
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL);
			// 应答客户端
			reply_response_message (0);
			XM_printf ("switch channel\n");
			// 停止当前通道的sensor采集
			XMSYS_SensorCaptureStop ();			
			
			// 启动新的通道的视频采集过程
			XMSYS_SensorCaptureStart ();
		}	
		// 记录文件超时退出，开始下一个录像记录
		//XM_printf ("h264_encoder_double_stream end\n");	
	}
	
	// XM_WaveClose (XMMCIDEVICE_RECORD);
	XMSYS_H264CodecCloseAudioRecord ();
		
	close_recycle_task ();
	
	return rec_ret;
}


// 返回值
// 0		-->	继续播放
//	1		-->	播放退出
//	2		-->	前进
//	3		-->	后退
// 5     -->   回放定位(秒)
int XMSYS_H264DecoderCheckEvent (struct H264DecodeStream *stream)
{
	char file_name[XMSYS_H264_CODEC_MAX_FILE_NAME + 1];
	u8_t event;
	int ret = 0;
	stream->curr_ticket += 33;
	
	// 检查外部ACC电源状况
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// XM_printf ("codec stop, acc bad\n");
		// 非安全电压
		// ACC上电 --> ACC掉电, 准备关机
		return 1;		
	}
		
	// 检查是否存在外部事件
	event = OS_GetEventsOccurred(&TCB_H264CodecTask);

	//XM_printf(">>>>>>>>>>>XMSYS_H264DecoderCheckEvent, event:%d\r\n", event);
	//XM_printf(">>>>>>>>>>>XMSYS_H264DecoderCheckEvent, stream->system_ticket:%d\r\n", stream->system_ticket);
	//XM_printf(">>>>>>>>>>>XMSYS_H264DecoderCheckEvent, stream->curr_ticket:%d\r\n", stream->curr_ticket);

	
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
			XM_printf ("playback end due to PLAYER_STOP, %d\n", XM_GetTickCount());
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
						OS_WaitSingleEvent(XMSYS_H264_CODEC_EVENT_COMMAND);
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
	OS_Delay (1);
	// 等待直到下一次定时器事件
	return 0;
	
}
UCHAR Play_Next_Video_keyflag= FALSE;
UCHAR Play_Next_Video_keyflag_time= 0;
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
	char file_name2[XMSYS_H264_CODEC_MAX_FILE_NAME+1];
	int codec_ret=-1;

	int playback_again;
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
					XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>XMSYS_H264_CODEC_COMMAND_RECORDER_START\r\n");
					OS_Use(&h264codec_sema); /* Make sure nobody else uses */
					h264codec_mode = XMSYS_H264_CODEC_MODE_RECORD;
					h264codec_state = XMSYS_H264_CODEC_STATE_WORK;

					OS_Unuse(&h264codec_sema); 
					
					// 应答UI线程
					ret = 0;
					OS_PutMail1 (&h264codec_mailbox, &ret);	
					clear_codec_command();
					XMSYS_SensorCaptureStart();
					
					// 启动h264 codec过程
					#if _XM_PROJ_ == _XM_PROJ_2_SENSOR_1080P_CVBS 
					//codec_ret = h264codec_RecorderStart_two_channel();
					codec_ret = h264codec_RecorderStart_two_channel();
					#else
					codec_ret = h264codec_RecorderStart();
					#endif
					if(codec_ret < 0)
					{
						// 视频录像未能正确启动, 重置codec状态, 避免状态锁死
						OS_Use(&h264codec_sema); /* Make sure nobody else uses */
						h264codec_mode = XMSYS_H264_CODEC_MODE_IDLE;
						h264codec_state = XMSYS_H264_CODEC_STATE_STOP;
						OS_Unuse(&h264codec_sema); 
						
						clear_codec_command ();
					}
					break;

				case XMSYS_H264_CODEC_COMMAND_RECORDER_STOP:

					XM_printf ("%d,XMSYS_H264_CODEC_COMMAND_RECORDER_STOP\n",XM_GetTickCount());
					//XMSYS_SensorCaptureStop ();
					OS_Use(&h264codec_sema); /* Make sure nobody else uses */
					h264codec_mode = XMSYS_H264_CODEC_MODE_IDLE;
					h264codec_state = XMSYS_H264_CODEC_STATE_STOP;
					OS_Unuse(&h264codec_sema); 

					// 应答UI线程
					//ret = 0;
					//OS_PutMail1 (&h264codec_mailbox, &ret);	

					clear_codec_command ();
					break;

				case XMSYS_H264_CODEC_COMMAND_PLAYER_START://解码操作
					// 停止Sensor
					XMSYS_SensorCaptureStop ();
					
					XM_printf(">>>>>>>>>>>>>>>>>>>>XMSYS_H264_CODEC_COMMAND_PLAYER_START............\r\n");

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
					OS_PutMail1(&h264codec_mailbox, &ret);	//

					// 启动播放
					XM_printf(">>>>>>decode_avi %s\n", file_name);

					// 打开音频
					waveFormat.wChannels = 1;
					waveFormat.wBitsPerSample = 16;
					waveFormat.dwSamplesPerSec = 8000;

					// 将指定文件名的视频标记使用中，防止删除操作将视频项删除
					video_usage = XM_VideoItemMarkVideoFileUsage(file_name);
					
					result = XM_WaveOpen(XMMCIDEVICE_PLAY, &waveFormat);
					(void)(result); // remove compiler warning
					
					{
						// 保存LCD设置, 禁止自动关屏
						unsigned int old_lcd_setting = AP_GetMenuItem (APPMENUITEM_LCD);
						AP_SetMenuItem(APPMENUITEM_LCD, AP_SETTING_LCD_NEVERCLOSE);
						
						codec_ret = h264_decode_avi_stream(file_name,	NULL,	&dec_stream);//解码API接口
						
						// 恢复LCD设置
						AP_SetMenuItem (APPMENUITEM_LCD, old_lcd_setting);
					}
					
					XM_WaveClose (XMMCIDEVICE_PLAY);
					
					// 检查外部ACC电源状况
					if(xm_power_check_acc_safe_or_not() == 0)
					{
						XM_printf ("video play stop, acc bad\n");
						// 非安全电压
						// 关机并等待ACC上电
						XM_ShutDownSystem (SDS_POWERACC);
					}
					
					if(video_usage)
						XM_VideoItemMarkVideoFileUnuse (file_name);
					
					//XM_printf ("close Output\n");
					//XMSYS_VideoSetImageOutputReady (0, 0, 0);

					XM_printf(">>>>>>>>>>>>>>>>check other file if not player.......\r\n");
					XM_printf(">>>>>>>h264codec_command_param.command:%d\r\n", h264codec_command_param.command);
					
					// 检查是否存在其他的播放文件
					playback_again = 0;
					OS_Use(&h264codec_sema);
					// 检查是否播放新的播放文件
					if(h264codec_command_param.command == XMSYS_H264_CODEC_COMMAND_PLAYER_START)
					{
						playback_again = 1;
					}					
					OS_Unuse(&h264codec_sema);
					
					//if(!playback_again && !dec_stream.command_player_stop)
					//	XMSYS_MessageSocketPlaybackFinish (0);
					(void)(playback_again);		// remove compiler warning
					
					OS_Use(&h264codec_sema); /* Make sure nobody else uses */
					h264codec_mode = XMSYS_H264_CODEC_MODE_IDLE;
					h264codec_state = XMSYS_H264_CODEC_STATE_STOP;
					OS_Unuse(&h264codec_sema); 

					XM_printf(">>>>>codec_ret:%d\r\n", codec_ret);
					// 向UI线程发送播放结束消息

					
					if(codec_ret == 0)
						//XM_printf(">>>>>>>>>>>play pause...........\r\n");
						XM_KeyEventProc (VK_AP_VIDEOSTOP, (SHORT)(AP_VIDEOEXITCODE_FINISH | (h264codec_state << 8)) );//发送播放完成状态
					else
						XM_KeyEventProc(VK_AP_VIDEOSTOP, AP_VIDEOEXITCODE_STREAMERROR);
					break;

				case XMSYS_H264_CODEC_COMMAND_PLAYER_STOP://停止解码
					XM_printf(">>>>>>>>>>>>>>>>XMSYS_H264_CODEC_COMMAND_PLAYER_STOP %d\n", XM_GetTickCount());
					OS_Use(&h264codec_sema); /* Make sure nobody else uses */
					h264codec_mode = XMSYS_H264_CODEC_MODE_IDLE;
					h264codec_state = XMSYS_H264_CODEC_STATE_STOP;
					OS_Unuse(&h264codec_sema); 
					
					clear_codec_command ();
					
					//应答UI线程
					ret = 0;
					OS_PutMail1(&h264codec_mailbox, &ret);	//

					// 开启Sensor
					//XMSYS_SensorCaptureStart ();
					
					break;

				default:
					XM_printf ("unknown codec command (%d)\n", command.command);
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
	//H264 Codec Cache文件初始化
	//XMSYS_H264FileInit ();
	
	h264codec_mode = XMSYS_H264_CODEC_MODE_IDLE;
	h264codec_state = XMSYS_H264_CODEC_STATE_STOP;
	
	OS_CREATERSEMA(&h264codec_sema); /* Creates resource semaphore */
	
	OS_CREATEMB(&h264codec_mailbox, 1, 1, &h264codec_mailbuf);	
	
	OS_CREATETASK(&TCB_H264CodecTask, "H264CodecTask", XMSYS_H264CodecTask, XMSYS_H264CODEC_TASK_PRIORITY, StackH264CodecTask);
}

void XMSYS_H264CodecExit (void)
{
	OS_Terminate(&TCB_H264CodecTask);
	
	OS_DeleteMB (&h264codec_mailbox);
	
	OS_DeleteRSema (&h264codec_sema);
}


// 启动视频录像
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecRecorderStart (void)
{
	int ret = -1;
	char response_code;

	XM_printf(">>>>XMSYS_H264CodecRecorderStart......\r\n");
	if(XMSYS_system_update_busy_state())
	{
		XM_printf ("recorder startup rejected because system busy to do system update\n");
		return -1;
	}
		
	// 清除缓存的应答消息
	clear_response_message ();
	
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	
	if(h264codec_command_param.command)
	{
		XM_printf ("command %d busy\n", h264codec_command_param.command);
		OS_Unuse(&h264codec_sema);
		return -1;
	}
	
	// 检查当前模式
	if(h264codec_mode == XMSYS_H264_CODEC_MODE_RECORD)
	{
		// 已处于记录模式
		XM_printf ("Warning，RecorderStart reject, repeat command\n");
		OS_Unuse(&h264codec_sema);
		return 0;
	}
	
	OS_Unuse(&h264codec_sema);
	
	if(h264codec_mode == XMSYS_H264_CODEC_MODE_PLAY)//回放模式
	{
		// 当前处于播放模式
		// 终止当前的视频播放
		ret = XMSYS_H264CodecPlayerStop ();
		if(ret)
		{
			XM_printf ("RecorderStart reject\n");
			return ret;
		}
	}
	
	// 检查当前的状态
	if(XMSYS_H264CodecGetMode() != XMSYS_H264_CODEC_MODE_IDLE)
	{//异常
		XM_printf(">>>>Mode %s, state %s reject RecorderStart command\n", XMSYS_H264CodecGetModeCode(), XMSYS_H264CodecGetStateCode());
		return -1;
	}
	
	XM_printf(">>>>RECORDER_START...............\r\n\n");
	XM_AppInitStartupTicket();
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	
	//h264codec_command_param.id = XMSYS_H264_CODEC_COMMAND_ID;
	h264codec_command_param.command = XMSYS_H264_CODEC_COMMAND_RECORDER_START;
	OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* 通知事件 */
	OS_Unuse(&h264codec_sema);
	
	// 等待H264 Codec线程应答
	if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
	{
		// codec已应答，获得消息
		ret = (int)response_code;
		printf(">>>>XMSYS_VideoOpen \n");
		XMSYS_VideoOpen();
	}
	else
	{
		// 超时
		XM_printf (">>>>RecorderStart timeout\n");
		ret = -2;
	}
	XM_printf(">>>>fun XMSYS_H264CodecRecorderStart end......\r\n");
	return ret;
}


// 检查H264编解码是否忙
// 1 busy
// 0 idle
int XMSYS_H264CodecCheckBusy (void)
{
	if(h264codec_command_param.command)
		return 1;
	else
		return 0;
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
		XM_printf ("%d,RECORDER_STOP\n",XM_GetTickCount());
		
		// 关闭H264编码器
		h264codec_command_param.command = XMSYS_H264_CODEC_COMMAND_RECORDER_STOP;
		OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* 通知事件 */
		OS_Unuse(&h264codec_sema);
		
		// 等待H264 Codec线程应答
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
		{
			// codec已应答，获得消息
			ret = (int)response_code;
			XM_printf ("%d,RecorderStop resp=%d\n", XM_GetTickCount(),ret);
			
			// 延时1秒，等待codec停止
			if(ret == 0)
			{
				unsigned int ticket = XM_GetTickCount ();
				while(h264codec_mode != XMSYS_H264_CODEC_MODE_IDLE)
				{
					if(abs(XM_GetTickCount() - ticket) > 2000 ) 
						break;
					OS_Delay (1);
				}
				if(h264codec_mode == XMSYS_H264_CODEC_MODE_IDLE)
					ret = 0;
				else
					ret = -2;
			}
			#if 1  //解决快速进入录像列表，快速推出录像列表，有时候出现停止录像出现在启动录像后面，导致黑屏
			XM_SetFmlDeviceCap (DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_STOP);
			#endif
		}
		else
		{
			// 超时
			XM_printf ("RecorderStop timeout\n");
			ret = -2;
		}
		
		// 停止记录
		// 关闭视频输出
		XMSYS_VideoClose ();
		printf(" XMSYS_VideoClose \n");
		// 停止sensor
		XM_printf ("%d,XMSYS_SensorCaptureStop\n",XM_GetTickCount());
		XMSYS_SensorCaptureStop ();

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
	}
	*/
		
	XM_printf ("SwitchChannel %d\n", camera_channel);
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
		XM_printf("command %d busy\n", h264codec_command_param.command);
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
	
	if(OS_GetMailTimed(&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
	{
		// codec已应答，获得消息
		ret = (int)response_code;
		rendervideo_start_ticket = OS_GetTime();

		XM_printf(">>>>>>rendervideo_start_ticket:%d\r\n", rendervideo_start_ticket);
		XM_printf(">>>>>>>>PlayerStart resp=%d\r\n", ret);
	}
	else
	{
		// 超时
		XM_printf("PlayerStart timeout\n");
		ret = -2;
	}
	return ret;
}

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
		XM_printf ("PLAYER_STOP %d\n", XM_GetTickCount());
		
		h264codec_command_param.command = XMSYS_H264_CODEC_COMMAND_PLAYER_STOP;
		OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* 通知事件 */
		OS_Unuse(&h264codec_sema);
		
		// 等待H264 Codec线程应答
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
		{
			// codec已应答
			ret = response_code;
			rendervideo_start_ticket = OS_GetTime();
			XM_printf ("PlayerStop resp=%d\n", ret);
		}
		else
		{
			XM_printf ("PlayerStop timeout\n");
			ret = -2;	// 超时错误码
		}
		
		// 等待视频显示线程刷新解码的最后一帧图像
		//OS_Delay (100);
		
		// 关闭视频输出(OSD 0层关闭)
		//XM_osd_framebuffer_release (0, XM_OSD_LAYER_0);
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
			rendervideo_start_ticket = OS_GetTime();
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
			rendervideo_start_ticket = OS_GetTime();
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
			rendervideo_start_ticket = OS_GetTime();
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
			rendervideo_start_ticket = OS_GetTime();
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

// 停止视频编解码器
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecStop (void)
{
	int ret = -1;
	
	do
	{
		OS_Use(&h264codec_sema); /* Make sure nobody else uses */
		clear_response_message ();
	
		XM_printf ("mode=%s state=%s\n", XMSYS_H264CodecGetModeCode(), XMSYS_H264CodecGetStateCode());
		if(h264codec_state == XMSYS_H264_CODEC_STATE_STOP)
		{
			OS_Unuse(&h264codec_sema);
			ret = 0;
		}
		else if(h264codec_mode == XMSYS_H264_CODEC_MODE_PLAY)	
		{
			// playback
			OS_Unuse(&h264codec_sema);
			ret = XMSYS_H264CodecPlayerStop ();
		}
		else if(h264codec_mode == XMSYS_H264_CODEC_MODE_RECORD)	
		{
			// record
			OS_Unuse(&h264codec_sema);
			ret = XMSYS_H264CodecRecorderStop ();
		}
		else
		{
			XM_printf ("XMSYS_H264CodecStop failed, illegal mode(%d)\n", h264codec_mode);
			OS_Unuse(&h264codec_sema);
			ret = -1;
		}
	} while (0);
	return ret;
}

#include <xm_videoitem.h>
#pragma data_alignment=32
static OS_TASK TCB_RecycleTask;
#pragma data_alignment=32
static OS_STACKPTR int StackRecycleTask[XMSYS_RECYCLE_TASK_STACK_SIZE/4];          /* Task stacks */
static volatile int recycle_task_stop;
static volatile int do_recycle;		// 标记是否立刻执行回收过程 1 马上执行

static void XMSYS_RecycleTask (void)
{
	int count = 0;
	OS_Delay (2000);		// 2秒后开始
	while (1)
	{
		if(recycle_task_stop)
		{
			OS_Delay (100);
			continue;
		}
		
		if(count % 200 == 0 || do_recycle)	// 每20秒扫描磁盘进行回收或者外部请求立刻执行回收过程
		{
			do_recycle = 0;
			XM_VideoItemMonitorAndRecycleGarbage(0);
		}
		OS_Delay (100);	
		count ++;
	} 
}

static void open_recycle_task (void)		// 开启循环回收任务
{
	// 允许回收任务
	recycle_task_stop = 0;
}

static void close_recycle_task (void)		// 关闭循环回收任务
{
	recycle_task_stop = 1;
}

// 请求立刻执行回收过程
void XMSYS_DoRecycle (void)
{
	do_recycle = 1;
}

void XMSYS_RecycleTaskInit (void)
{
	recycle_task_stop = 1;
	OS_CREATETASK(&TCB_RecycleTask, "RecycleTask", XMSYS_RecycleTask, XMSYS_RECYCLE_TASK_PRIORITY, StackRecycleTask);
}

// 设置强制非记录模式
// forced_non_recording
//		1			强制非记录模式 (该模式下不再录像, 除非清除该模式)
//		0			普通记录模式 
void XMSYS_H264CodecSetForcedNonRecordingMode(int forced_non_recording)
{
	int start_record = 0;

	XM_printf(">>>>forced_non_recording_mode:%d\r\n", forced_non_recording_mode);
	
	XM_lock ();
	if( (forced_non_recording_mode == 1) && (forced_non_recording == 0))
	{
		// 强制非记录模式 --> 普通记录模式 
		// 重新启动录像
		forced_switch_to_recording_mode = 1;
		start_record = 1;
	}
	forced_non_recording_mode = forced_non_recording;
	XM_unlock ();
	
	if(start_record)
	{
		XM_AppInitStartupTicket();
		XM_printf(">>>>>XMSYS_H264CodecSetForcedNonRecordingMode start recorder..........\r\n");
		XMSYS_H264CodecRecorderStart ();
	}
}


// 检查当前模式是否是强制非记录模式
// 返回值
// 	1			强制非记录模式 (该模式下不再录像, 除非清除该模式)
//	0			普通记录模式 
int XMSYS_H264CodecGetForcedNonRecordingMode (void)
{
	return forced_non_recording_mode;
}

#if 1
// 检查当前模式是否已经成功退出录像模式
// 返回值
// 	-1			非成功退出录像模式
//	0			成功退出录像模式
int  CheckExitRecord(void)
{
	if(!XMSYS_H264CodecGetForcedNonRecordingMode())
	{
		if(XM_GetFmlDeviceCap (DEVCAP_VIDEO_REC) == DEVCAP_VIDEO_REC_STOP)
		{
		   	// 延时4秒，等待codec停止
			unsigned int ticket = XM_GetTickCount ();
			while(h264codec_mode != XMSYS_H264_CODEC_MODE_IDLE)
			{
				if(abs(XM_GetTickCount() - ticket) > 4000 )
					break;
				OS_Delay (1);
			}
			if(h264codec_mode != XMSYS_H264_CODEC_MODE_IDLE)
			{
			    XM_printf("before start record and check record stop timeout\n");
				return -1;
			}
			else
			{
				return 0;
			}
		}
		else
		{
			 return 0;
		}
	}
}
#endif

