//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: avi_codec_task.c
//	  AVI�����������
//
//	Revision history
//
//		2012.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#include <stdio.h>
#include "xm_core.h"
#include "RTOS.h"		// OSͷ�ļ�
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
static void open_recycle_task (void);		// ����ѭ����������
static void close_recycle_task (void);		// �ر�ѭ����������

extern  int ShowLogo (UINT32 LogoAddr,UINT32 LogoSize);
extern int PowerOnShowLogo;
extern unsigned int rendervideo_start_ticket;	//��ǰ��Ƶ�ļ��ѻط�ʱ��(��ȥ��ʼʱ��)

extern unsigned int XMSYS_GetCacheSystemBusyLevel (void);

#define	CODEC_TIMEOUT				2000	// 2000�������ʱ

#define CVBS_NTSC					0x00
#define CVBS_PAL					0x01
#define _720P_25FPS				0x02
#define _720P_30FPS				0x03
#define _720P_50FPS				0x04
#define _720P_60FPS				0x05
#define _1080P_25FPS				0x06
#define _1080P_30FPS				0x07

// 20180516 ������ʱ¼Ӱģʽ
static volatile unsigned int time_lapse_photography_mode = 0;	// 1 ��ʱ¼Ӱģʽ 0 ��ͨ¼Ӱģʽ
static unsigned int current_video_start_ticket;		//��ǰ��Ƶ�ļ���¼��ʱ��(��ȥ��ʼʱ��)
static volatile int forced_iframe;			// 1 ǿ��IFrame����
static volatile int forced_encoder_stop;	// 1 ǿ�Ƶ�ǰ�������˳�
static volatile unsigned int video_lock_state;		// ��ǰ����¼�Ƶ���Ƶ�ļ��Ƿ������� 1 ������ 0 δ����
static volatile unsigned int video_lock_command;	// 0x01 --> ����, 0x02 --> ����
static volatile int forced_non_recording_mode=0;		// ǿ�ƷǼ�¼ģʽ 1 ǿ�ƷǼ�¼ģʽ 0 ��ͨ��¼ģʽ
static volatile int forced_switch_to_recording_mode;		// ��Ǵ�"ǿ�ƷǼ�¼ģʽ"�л���"��ͨ��¼ģʽ"
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

static volatile H264CODEC_COMMAND_PARAM h264codec_command_param;

static OS_RSEMA h264codec_sema;			// ���Ᵽ��
static OS_MAILBOX h264codec_mailbox;		// һ�ֽ����䡣
static char h264codec_mailbuf;					// һ���ֽڵ�����

static unsigned char h264codec_frame_rate = 30;	// ÿ��30֡ȱʡ

#define	MASK_LAST_TICKET		1000		// ֡���γ���ʱ��
static volatile unsigned int h264_mask_mod = 0;
static volatile unsigned int mask_stop_ticket;			// ��ǵ�ǰ֡���ν�����ʱ��
static  unsigned int frame_mask[] = {
	  0,								// ��ɾ
	  1<<0 | 1<<15,				// ɾȥ2֡
	  1<<0 | 1<<6 | 1<<12 | 1<<18 | 1<<24,						// ɾȥ5֡
	  1<<0 | 1<<3 | 1<<6 | 1<<9 | 1<<12 | 1<<15 | 1<<18 | 1<<21 | 1<<24 | 1<<27,			// ɾȥ10֡
	(unsigned int)~(1<<0 | 1<<3 | 1<<6 | 1<<9 | 1<<12 | 1<<15 | 1<<18 | 1<<21 | 1<<24 | 1<<27),			// ɾȥ20֡
	(unsigned int)~(1<<0 | 1<<6 | 1<<12 | 1<<18 | 1<<24),						// ɾȥ25֡
};

#pragma data_alignment=64
__no_init static unsigned char		audio_fifo[4096*4];
#pragma data_alignment=64
__no_init static unsigned char		audio_data[0x8000];


// ��ȡ��ǰ����ʱ¼Ӱģʽ
// ����ֵ
//		1		��ʱ¼Ӱģʽ
//		0		��ͨ¼Ӱģʽ
unsigned int XMSYS_H264CodecGetTimeLapsePhotographyMode (void)
{
	return time_lapse_photography_mode;
}

// ������ʱ¼Ӱģʽ
// mode	1	��ʱ¼Ӱģʽ
//       0	��ͨ¼Ӱģʽ
void XMSYS_H264CodecSetTimeLapsePhotographyMode (unsigned int mode)
{
	time_lapse_photography_mode = mode;
}

// ���ø���IFRAME��־,��ǿ����һ֡����I֡
void XMSYS_H264CodecForceIFrame (void)
{
	forced_iframe = 1;
}

// ��ȡ��ǰ��Ƶ��¼��ʱ��
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

// ����ǰ¼����Ƶ����
void XMSYS_H264CodecLockCurrentVideo (void)
{
	video_lock_command = 0x01;
}

// ����ǰ¼����Ƶ����
void XMSYS_H264CodecUnLockCurrentVideo (void)
{
	video_lock_command = 0x02;
}

// ��ȡ��ǰ¼����Ƶ������״̬
// 1 ������
// 0 δ����
unsigned int XMSYS_H264CodecGetVideoLockState (void)
{
	return video_lock_state;
}

// ���IFRAME��־
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

// ��ȡAVI����ʹ�õ���Ƶ��ʽ
int XMSYS_H264CodecGetVideoFormat (void)
{
	return AP_GetMenuItem (APPMENUITEM_VIDEO_RESOLUTION);
}

int XMSYS_H264CodecSetVideoFormat (int video_format)
{
	int ret = -1;
	OS_EnterRegion();		// ��ֹ�����л�
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
		// ����ISP��Ƶ����
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
			// ȱʡ1080P 30fps
			XM_lock ();
			isp_set_video_width (1920);
			isp_set_video_height (1080);			
			isp_set_sensor_bit (ARKN141_ISP_SENSOR_BIT_12);
			XM_unlock ();
			XM_printf ("video format (1080P_30)\n");
		}		
#else
		// ����ISP��Ƶ����
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
			// ȱʡ1080P 30fps
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
	//h264codec_command_param.id = 0;
	h264codec_command_param.command = 0;
	OS_Unuse(&h264codec_sema);
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

// ����ֵ
// 0    ��ʾ����һ����Ƶ֡����H264����
// 1    ��ʾ������һ����Ƶ֡��������H264����
// 2	��ʾ��ֹ��¼���˳�
// 3 	��ʾ��¼ʱ�䳤���Ѵﵽ���
// 4    ��ʾͨ���л�
int XMSYS_H264CodecCheckEvent (struct H264EncodeStream *stream)
{
	int ret = 0;
	u8_t event;
	int64_t clock;
	//unsigned int mask = h264_mask_mod;
	int mask = 0;		// �Ƿ����ε�ǰ֡(��������H264���룬ֻ����scalar����)

	// ����ⲿACC��Դ״��
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// XM_printf ("codec stop, acc bad\n");
		// �ǰ�ȫ��ѹ
		// ACC�ϵ� --> ACC����, ׼���ػ�
		return 2;		
	}
	
	if(forced_encoder_stop)
	{
		// ���¼�Ϊһ�����¼�
		forced_encoder_stop = 0;
		if(stream->file_save)
		{
			XM_printf ("codec stop, forced encoder stop\n");
			// ��ر�ǿ�Ʊ������˳��¼�
			return 2;
		}
	}
	
	/*
	if(stream->frame_number == 0)
	{
		// ��һ֡
		// ���ݵ�ǰϵͳʱ�䣬����֡��
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
	
	// ����ʱ�䣬���㵱ǰ��֡��
	stream->frame_number = ((XM_GetTickCount() - stream->start_ticket) * h264codec_frame_rate + 500) / 1000;
	
	if(h264_mask_mod == 0)
	{
		// ��֡����ģʽ
		unsigned int busy_level = XMSYS_GetCacheSystemBusyLevel ();
		// Խ��æ, ����֡Խ��
		switch(busy_level)
		{
#if 1
			// 20180410 zhuoyonghong �ر�֡���Σ�����ʹ�ö�̬���ʵ���
			// ��������������5֡
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
			mask_stop_ticket = XM_GetTickCount() + MASK_LAST_TICKET;		// ����´μ��ʱ��
			XM_printf ("mask_mod=0x%08x\n", h264_mask_mod);
		}
		else if(XM_VideoItemCheckDiskFreeSpaceBusy())
		{
			// 20180410 zhuoyonghong �ر�֡���Σ�����ʹ�ö�̬���ʵ���
			// ����ʣ��ռ��޷�����ʵʱ¼������, ����ʵʱ����֡��
			// mask_stop_ticket = XM_GetTickCount() + MASK_LAST_TICKET;		// ����´μ��ʱ��
			//h264_mask_mod = frame_mask[3];
			//h264_mask_mod = frame_mask[2];
			//XM_printf ("free space busy, mask_mod=0x%08x\n", h264_mask_mod);
		}
	}
	
	// ���֡����ģʽ.������, ����mask pattern����֡
	if(h264_mask_mod)
	{
		int shift = stream->frame_number % h264codec_frame_rate;
		if( h264_mask_mod & (1 << shift))
		{
			// ��ǰ֡��Ҫ����
			// �ۼ�������Ҫ���ε�֡���������ӳ�ʱ��
			mask = 1;
		}
		else
		{
			// ��ǰ֡��Ҫ��¼
			// ����Ƿ��������ټ�¼
			if(h264_mask_mod == frame_mask[5])
			{
				// ǿ��I֡
				XMSYS_H264CodecForceIFrame ();
			}
		}
		/*
		if( (stream->frame_number % mask) == 0 )
		{
			stream->frame_number ++;	// ������ʱ���Ӷ�����֡��
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
	// ���SD��		
	if(stream->file_save)
	{
		// �ļ�����ģʽ, ���Ա�����Ƶ
		
		// ��SD���γ�����ʱ��ֹ��Ƶ��¼����
		if(!IsCardExist(0))
		{
			// ���γ�����ֹ��¼����
			XM_printf ("switch to saveless mode\n");
			return 2;			
		}
	}
	else
	{
		// Ԥ��ģʽ���޷�������Ƶ
		
		// �����Ƶ������Ƿ��ѿ���	
		XMSYSTEMTIME CurrTime;
		// �����Ƶ������Ƿ��ѿ���, ��鿨�Ƿ����, ���RTCʱ���Ƿ�������
		if(	XM_VideoItemIsBasicServiceOpened() 
				// ��鿨�Ƿ����
				&& IsCardExist(0)
				// ���RTCʱ���Ƿ�������. ��RTCʱ��δ����, ��ִ��¼�����.	
				&& XM_GetLocalTime(&CurrTime)
			)	
		{
			// ��Ƶ������ѿ���, RTCʱ�������ã���ֹ��ǰ���ļ�����ļ�¼����, ����, ׼��ִ���ļ��������Ƶ��¼����
			XM_printf ("switch to save mode\n");
			return 2;			
		}
	}
	#endif
		// ���SD��		
	if(stream->file_save)
	{
		// �ļ�����ģʽ, ���Ա�����Ƶ
		
		// ����Ƿ�������"ǿ�ƷǼ�¼ģʽ"
		if(forced_non_recording_mode)
		{
			// ��ֹ��ǰ�Ŀ���¼ģʽ
			return 2;
		}
		
		// ��SD���γ�����ʱ��ֹ��Ƶ��¼����
		if(!IsCardExist(0))
		{
			// ���γ�����ֹ��¼����
			return 2;			
		}
	}
	else
	{
		// Ԥ��ģʽ���޷�������Ƶ
		
		// ����Ƿ���� ǿ�ƷǼ�¼ģʽ --> ��ͨ��¼ģʽ 
		if(forced_switch_to_recording_mode)
		{
			// ��ֹ�Ǽ�¼ģʽ
			return 2;
		}
		
		// �����Ƶ������Ƿ��ѿ���	
		XMSYSTEMTIME CurrTime;
		// �����Ƶ������Ƿ��ѿ���, ��鿨�Ƿ����, ���RTCʱ���Ƿ�������
		if(	XM_VideoItemIsBasicServiceOpened() 
				// ��鿨�Ƿ����
				&& IsCardExist(0)
				// ���RTCʱ���Ƿ�������. ��RTCʱ��δ����, ��ִ��¼�����.	
				&& XM_GetLocalTime(&CurrTime)
				// ����Ƿ�"��ͨ��¼ģʽ"
				&& !forced_non_recording_mode
			)	
		{
			// ��Ƶ������ѿ���, RTCʱ�������ã���ֹ��ǰ���ļ�����ļ�¼����, ����, ׼��ִ���ļ��������Ƶ��¼����
			XM_printf ("switch to save mode\n");
			return 2;			
		}
	}
	
	// �����ײ
	{
		if(stream->file_save)
		{
			// SD������������д��	
			if(stream->file_lock == 0)
			{
				if(XMSYS_GSensorCheckAndClearCollisionEvent())
				{
					// ������ײ�¼�
					// ��Ƶ��δ����
					// ִ����������
					XM_printf ("video (%s) lock\n", stream->file_name[0]);
					int locked = AP_VideoItemLockVideoFileFromFileName (stream->file_name[0]);	// ��ǰֻʹ��һ��ͨ��
					if(locked)
					{
					    video_lock_state = 1;//eason 20170920��������������Ƶ״̬
						stream->file_lock = 1;
						//stream->stop_ticket += 1 * 60 * 1000;	// ¼���н���ǰ¼�Ƶ���Ƶ������������¼�����ӳ�1���ӡ�
					}
					else
					{
						XM_printf ("lock (%s) NG\n", stream->file_name[0]);
					}
				}
			}
			else
			{
				// ��Ƶ�ѱ�����, ��������
			}
		}
		else
		{
			// SD��������
			//SD�����ڣ�����ֹͣ¼�񣬼����ײ
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
						
						// ��ֹ��ǰ�Ŀ���¼ģʽ
						return 2;
				    }
				}
			}
		}		
	}
	
	// ����Ƿ�����������������
	switch(video_lock_command)
	{
		case 0x01:	// ����
			if(stream->file_save)
			{
				if(stream->file_lock == 0)
				{
					// ��Ƶ��δ����
					// ִ����������
					/*
					int ret = AP_VideoItemLockVideoFileFromFileName (stream->file_name[0]);	// ��ǰֻʹ��һ��ͨ��
					if(ret)
					{
						stream->file_lock = 1;
						video_lock_state = 1;
					}
					XM_printf ("lock (%s) %s\n", stream->file_name[0], ret ? "OK" : "NG");
					*/
					
					#if 1  //���ǰ��·ͬʱ����������
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
						int ret = AP_VideoItemLockVideoFileFromFileName (stream->file_name[0]);	// ��ǰֻʹ��һ��ͨ��
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
					// ������
				}
			}
			break;
			
		case 0x02:	// ����
			if(stream->file_save)
			{
				if(stream->file_lock == 1)
				{
					// ��Ƶ�ѱ���
					// ִ�н�������
					/*
					int ret = AP_VideoItemUnlockVideoFileFromFileName (stream->file_name[0]);	// ��ǰֻʹ��һ��ͨ��
					if(ret)
					{
						stream->file_lock = 0;
						video_lock_state = 0;
					}
					XM_printf ("unlock (%s) %s\n", stream->file_name[0], ret ? "OK" : "NG");
					*/
					
					#if 1  //���ǰ��·ͬʱ����������
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
						int ret = AP_VideoItemUnlockVideoFileFromFileName (stream->file_name[0]);	// ��ǰֻʹ��һ��ͨ��
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
					// �ѽ���
				}
			}
			break;
	}
	// �������/��������
	video_lock_command = 0;
	
	// ����Ƿ�����ⲿ�¼�
	event = OS_GetEventsOccurred (&TCB_H264CodecTask);
	
#if 0
	// ���һ�������¼�
	if(event & XMSYS_H264_CODEC_EVENT_ONE_KEY_PROTECT)
	{
		// һ����������
		char response_code = (char)(-1);
		// �����¼�
		OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_ONE_KEY_PROTECT);
		if(stream->file_save)
		{
			// SD������������д��			
			if(stream->file_lock == 0)
			{
				// ��Ƶ��δ����
				// ִ����������
				int locked = AP_VideoItemLockVideoFileFromFileName (stream->file_name[0]);	// ��ǰֻʹ��һ��ͨ��
				if(locked)
				{
					stream->file_lock = 1;
					stream->stop_ticket += 3 * 60 * 1000;	// ¼���� ����ǰ¼�Ƶ���Ƶ������������¼�����ӳ�3���ӡ�
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
				// ��Ƶ�ѱ�����
				response_code = 0;
			}
		}
		else
		{
			// SD��������
			
		}
		// Ӧ��ͻ���
		reply_response_message (response_code);			
	}
#endif
	
	// �ⲿ����(ֹͣ���ط�)������codec����
	if(event & XMSYS_H264_CODEC_EVENT_COMMAND)
	{
		int command;
		// ��ȡ����
		OS_Use(&h264codec_sema); /* Make sure nobody else uses */
		command = h264codec_command_param.command;
		OS_Unuse(&h264codec_sema);
		
		// �������
		if(command == XMSYS_H264_CODEC_COMMAND_RECORDER_STOP)
		{
			// ¼��ֹͣ����
			XM_printf ("%d, Stoped due to RECORDER_STOP\n", XM_GetTickCount());
			// ����2��ʾ��ֹ��ǰH264 Codec����
			ret = 2;
			// Ӧ���������¼��ֹͣ����Ӧ���һ��ᳬʱ(�ļ��رյ���)��
			// ��Ӧ������͵������˴�����¼��ֹͣ��������ִ��
			reply_response_message(0);	
			return ret;
		}
		else if(command == XMSYS_H264_CODEC_COMMAND_PLAYER_START)
		{
			// ������������
			XM_printf ("%d, Stoped due to PLAYER_START\n", XM_GetTickCount());
			// ����2��ʾ��ֹ��ǰH264 Codec����
			ret = 2;
			return ret;
		}
		else
		{
			// �������ȫ�����˲����������
			clear_codec_command ();
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_COMMAND);
		}	
	}
	
	// 20170929
	// �ر���֮ǰ2������ǰ��ֹ�����ӳ��ļ�ɾ������, �����ļ�ϵͳ�������µĶ�֡����
	if( (XM_GetTickCount() + 2000) >= stream->stop_ticket )
	{
		XM_VideoItemSetDelayCutOff ();		// �ӳ��ļ�ɾ������, �����ļ�ϵͳ�������µĶ�֡����		
	}
	
	// ����Ƿ��¼�ѳ�ʱ(��Ƶ�ֶ�)
	if(XM_GetTickCount() >= stream->stop_ticket)
	{
		// ��ֹ�����˳�
		XM_printf ("%d, Expired\n", XM_GetTickCount());
		XM_AppInitStartupTicket();//�ֶ���Ƶ��ʼ¼���¼���ʱ��
		return 3;
	}
	
	// �л�¼��ͨ��(ǰ�ӡ�����)
	if(event & XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL)
	{
		// ��ֹ�����˳�
		XM_printf ("%d, Switch\n", XM_GetTickCount());
		return 4;
	}
	
	// �ȴ�ֱ����һ�ζ�ʱ���¼�
	// OS_DelayUntil (stream->curr_ticket);
	OS_Delay (1);
	return mask;
}

extern void *os_fopen (const char *filename, const char *mode);

void XMSYS_H264CodecOpenAudioRecord (void)
{
	XMWAVEFORMAT waveFormat;
	// ����MIC
	waveFormat.wChannels = 1;			// ������
	waveFormat.wBitsPerSample = 16;
	waveFormat.dwSamplesPerSec = 8000;
	XM_WaveOpen (XMMCIDEVICE_RECORD, &waveFormat);
}

void XMSYS_H264CodecCloseAudioRecord (void)
{
	XM_WaveClose (XMMCIDEVICE_RECORD);
}

// 20170606 
// �޸�video_index����;, ��Ϊͨ����
int XMSYS_H264CodecGetAudioFrame (short *samples, int frame_size, int nb_channels, unsigned int video_index)
{
	int  readcnt ;
	// mono
	if(XM_WaveRead (XMMCIDEVICE_RECORD, (WORD *)samples, frame_size*nb_channels) <= 0)
		return 0;
	
	readcnt = frame_size*nb_channels;
	#if 0 //�طŵ�ʱ����ʱĬ�ϴ�MIC
	if(AP_GetMenuItem(APPMENUITEM_MIC) == AP_SETTING_MIC_OFF)	// MIC�ر�
	{
		memset (samples, 0, frame_size*nb_channels*2);
	}
	#endif
	return readcnt  ;
}



extern 	int h264_encode_avi_stream (char *filename, char *filename2, struct H264EncodeStream *H264Stream,
				const char **argv, int argc);
extern int XMSYS_WaitingForCacheSystemFlush (unsigned int timeout_ms);

// ����ֵ	����˳�ԭ��
//	0		�ⲿ���������˳�
// 	-1		�ڲ��쳣�����˳�
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
	int rec_ret = 0;		// ����ֵ

	int voice_startup = 0;	// ���"¼������"�����Ƿ��Ѳ���
	
	unsigned int video_format = AP_GetMenuItem (APPMENUITEM_VIDEO_RESOLUTION);
	
	// ����¼��֡��
	if(video_format == AP_SETTING_VIDEORESOLUTION_720P_30)
	{
		h264codec_frame_rate = 30;
	}
#if HONGJING_CVBS
	// ������ʽ��Ϊ30֡
#else
	else if(video_format == AP_SETTING_VIDEORESOLUTION_720P_60)
	{
		h264codec_frame_rate = 60;
	}
#endif
	else //if(video_format == AP_SETTING_VIDEORESOLUTION_1080P_30)
	{
		// ȱʡ1080P 30fps
		h264codec_frame_rate = 30;
	}

	
#if HIGH_VIDEO_QUALITY
	//XM_printf ("HIGH_VIDEO_QUALITY\n");
#elif NORMAL_VIDEO_QUALITY
	//XM_printf ("NORMAL_VIDEO_QUALITY\n");
#else
	//XM_printf ("GOOD_VIDEO_QUALITY\n");
#endif
	
	// ���ACC�Ƿ�ȫ����
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// �ػ����ȴ�ACC�ϵ�
		XM_ShutDownSystem (SDS_POWERACC);
	}

	open_recycle_task ();
	
	// ����MIC
	XMSYS_H264CodecOpenAudioRecord ();	
	
	video_lock_state = 0;
	video_lock_command = 0;
	
	forced_encoder_stop = 0;

	memset (&LastTime, 0, sizeof(LastTime));

	
	// ��¼����
	while(1)
	{
		memset (&EncodeStream, 0, sizeof(EncodeStream));
					
		memset (h264name_f, 0, sizeof(h264name_f));
		memset (h264name_r, 0, sizeof(h264name_r));
		memset (h264name, 0, sizeof(h264name));
		
		// �µ���Ƶ�ļ�,ȱʡ������
		video_lock_state = 0;
		
		hVideoItem = NULL;
		
		// ע�⣬���ڵ�XM_VideoItemCreateVideoItemHandle ����Bug��û���ͷŵĴ���
		//XM_printf ("Please resolve bugs within XM_VideoItemCreateVideoItemHandle\n");
		// ��ȡһ���µ���Ƶ���ļ���
		channel = XMSYS_SensorGetCameraChannel ();
		
		// ǿ�ƷǼ�¼ģʽ
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
			// ���ٵ�¼����/�رջ�����ͬ��ʱ����(ͬһ����)����2��¼����, ������ͬ��¼���ļ�����, ǰһ��ͬ���ļ��ᱻɾ��.
			// ��Ϊǰһ��ͬ�����ļ���δ��ȫ�ر�(�������̴߳����رղ���), �ᵼ��ɾ��ʧ��.
			// ��Ҫ�ܿ������
			// ���ʱ���Ƿ�һ��. ��һ��, ��ʱ, 
			if(!check_same_video_time (&CurrTime, &LastTime))
				break;
			OS_Delay (10);
			delay_count ++;
			// XM_printf ("Delay %d\n", delay_count);
		} while (delay_count < 100);		// �����ʱ1��
		LastTime = CurrTime;
		
		// ���ϵͳRTCʱ���Ƿ������á�ARK1930�ĺ��Ӿ�����û��RTC����Ҫ�ⲿ����RTCʱ�䡣
		//if(!XM_GetLocalTime(&CurrTime))
		{
			// ��δ���ã�ִ��¼�����������
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
			// ������ʱ��Ƶ�ļ������ڼ�¼��Ƶ��
			// 	���洢����һ�β���ʱ��ĳЩ����»�ȡ���ļ�ϵͳ����������ϳ�ʱ��(10��~30��)��������Ƶ�����ݿ���ʱ�޷�ʹ�ã�
			// 	�������޷���¼��Ƶ��
			// 	Ϊ����������⣬����һ����ʱ��Ƶ��¼�ļ������ڱ���ʵʱ��Ƶ��
			/*if(XM_VideoItemCreateVideoTempFile (channel, h264name_f, sizeof(h264name_f)))
			{
				// ����һ����ʱ��Ƶ�ļ��ɹ�
				strcpy (EncodeStream.file_name[0], h264name_f);
				EncodeStream.file_save = 1;
				//EncodeStream.file_temp = 1;	// �������ʱ��Ƶ���ļ�
			}
			else*/
			{
				// ����һ���߼���Ƶ�ļ�(�޷�д��)
				//XM_KeyEventProc (VK_AP_VIDEOSTOP, (unsigned char)3);
				sprintf (h264name_f, "\\TEST.AVI");
				EncodeStream.file_save = 0;
				strcpy (EncodeStream.file_name[0], h264name_f);
			}
		}
		else
		{
			// ��Ƶ��������ɹ�
			pVideoItem = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
			if(pVideoItem == NULL)
			{
				hVideoItem = NULL;
				goto create_video_item_failed;
			}
			// ��ȡ��Ƶ���·����
			if(XM_VideoItemGetVideoFilePath (pVideoItem, channel, h264name_f, sizeof(h264name_f)))
			{
				// �����ļ���
				strcpy (EncodeStream.file_name[0], h264name_f);
				EncodeStream.file_save = 1;
			}
			else
				goto create_video_item_failed;
		}

	prepare_to_record:
		// ���õ�ǰʹ�õķֱ���
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
		
		// ������Ƶ��¼ʱ�䳤��
		XM_printf(">>>>>>>>>>>>>>>>>>>>VideoTimeSize=%d\n", AP_GetVideoTimeSize());
		if(EncodeStream.file_save)
			EncodeStream.stop_ticket = OS_GetTime() + 1000 * AP_GetVideoTimeSize() + 250;		// 250 ����AVI�ļ�ʱ�䳤�ȵ����
		else
		{
			EncodeStream.stop_ticket = OS_GetTime() + 1000 * 25;	
			// ÿ��25������
			if(!XMSYS_H264CodecGetForcedNonRecordingMode())
			{
				// ��δ����/����Ч/���ļ�ϵͳ����ʱ, ÿ��25��������UI����
				if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
					XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM);
			}
			voice_startup = 0;
		}
		
		// ����Ƿ񲥷�"¼������"����
		if(voice_startup == 0 && EncodeStream.file_save)
		{
			// ����"¼������"����
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				XM_voice_prompts_insert_voice (XM_VOICE_ID_STARTUP);
			voice_startup = 1;
		}

		//XM_printf ("%d Enter %s\n",  XM_GetTickCount(), h264name_f);
		forced_switch_to_recording_mode = 0;
		
		// ����H264���룬�ȴ�����(�ⲿ�����¼��ʱ���¼�)
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
			// ��0ֵ��ʾ�쳣
			XM_printf ("Codec stop due to exception, code = %d\n", ret);
			
			// �ȴ���Ƶ����ر�
			//if(hVideoItem)
			//	XM_VideoItemWaitingForUpdate (EncodeStream.file_name[0]);
			//XMSYS_WaitingForCacheSystemFlush (500);
			
			//break;
			// 20170602 codec�����쳣/�ο��Ȼᵼ���˳�. ��ʱ���˳�ѭ��, �ȴ��ⲿ����
			//if(IsCardExist(0))
			//{
				// Ͷ���쳣��Ϣ
				// XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_VERIFY_ERROR);
			//}
		}
		
		// ����Ƿ�ͣ���������
		if(XMSYS_GSensorCheckParkingCollisionStartup())
		{
			// ����Ƿ��Ǵ�Ԥ��ģʽ�л�������ģʽ
			if(EncodeStream.file_save == 0 && IsCardExist(0))
			{
				OS_Delay (100);
				if(XM_VideoItemIsBasicServiceOpened())
				{
					XM_printf ("g-sensor, switch to save mode\n");
					goto switch_to_save_state;
				}
			}
			// ͣ�����������ʽ
			XM_printf ("Parking Collision Recording Close\n");
			
			XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 0);

			close_recycle_task ();
			
			// �ȴ���Ƶ�������ر�	
			// ������Ƶ��
			AP_VideoItemLockVideoFileFromFileName (EncodeStream.file_name[0]);
			// �ȴ���Ƶ��ر�
			XM_VideoItemWaitingForUpdate (EncodeStream.file_name[0]);
			XM_printf ("%d, XM_VideoItemWaitingForUpdate\n", XM_GetTickCount());

			// �ȴ�Cache�ļ�ϵͳ������д�뵽SD��
			XMSYS_WaitingForCacheSystemFlush (5000);			
			
			// �ػ����ȴ�ACC�ϵ�
			XM_ShutDownSystem (SDS_POWERACC);
		}
		
		// ���ACC�Ƿ��ACC�ϵ�ģʽ�л���ACC�ض�ģʽ
		if(xm_power_check_acc_safe_or_not() == 0)
		{
			// ACC�ϵ�-->ACC�µ�
			XM_printf ("ACC Power Down\n");
			
			XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 0);
			
			close_recycle_task ();
			
			// �ȴ���Ƶ�������ر�	
			XM_VideoItemWaitingForUpdate (EncodeStream.file_name[0]);
			XM_printf ("%d, XM_VideoItemWaitingForUpdate\n", XM_GetTickCount());

			// �ȴ�Cache�ļ�ϵͳ������д�뵽SD��
			XMSYS_WaitingForCacheSystemFlush (5000);			
			
			// �ػ����ȴ�ACC�ϵ�
			XM_ShutDownSystem (SDS_POWERACC);
		}
		
	switch_to_save_state:
		// ����˳�ԭ��
		event = OS_GetEventsOccurred (&TCB_H264CodecTask);
		
		if(event & XMSYS_H264_CODEC_EVENT_COMMAND)
		{
			// �ⲿ����
			XM_printf ("%d, Exit Codec Loop\n", XM_GetTickCount());
			
			// �ȴ���Ƶ����ر�
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
			// ͨ���л�����
			// ���¼�����
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL);
			// Ӧ��ͻ���
			reply_response_message (0);
			XM_printf ("switch channel\n");
			// ֹͣ��ǰͨ����sensor�ɼ�
			XMSYS_SensorCaptureStop ();			
			
			// �����µ�ͨ������Ƶ�ɼ�����
			XMSYS_SensorCaptureStart ();
		}
					
		// ��¼�ļ���ʱ�˳�����ʼ��һ��¼���¼
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
				const char **argv_fore, int argc_fore,		// ǰ����Ƶ�������
				const char **argv_back, int argc_back		// ������Ƶ�������
				);

// ˫·¼��(ǰ1080p + ����cvbs)
// ����ֵ	����˳�ԭ��
//	0		�ⲿ���������˳�
// 	-1		�ڲ��쳣�����˳�
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
	int rec_ret = 0;		// ����ֵ
	
	int voice_startup = 0;	// ���"¼������"�����Ƿ��Ѳ���
	unsigned int camera_format = 2;//camera_get_video_format();
	unsigned int video_format = AP_GetMenuItem (APPMENUITEM_VIDEO_RESOLUTION);

	XM_printf(">>>>>>>video_format:%d\r\n", video_format);

	#if 0
	// ����¼��֡��
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
		// ȱʡ1080P 30fps
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
	//�ο�IAR�������õĺ궨��
	#if HIGH_VIDEO_QUALITY
	//XM_printf ("HIGH_VIDEO_QUALITY\n");
	#elif NORMAL_VIDEO_QUALITY
	XM_printf("NORMAL_VIDEO_QUALITY\n");
	#else
	//XM_printf ("GOOD_VIDEO_QUALITY\n");
	#endif
	
	// ���ACC�Ƿ�ȫ����
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// �ػ����ȴ�ACC�ϵ�
		XM_ShutDownSystem (SDS_POWERACC);
	}
		
	open_recycle_task ();
	
	// ����MIC
	XMSYS_H264CodecOpenAudioRecord();	
	
	video_lock_state = 0;
	video_lock_command = 0;
	forced_encoder_stop = 0;
	
	memset (hVideoItem, 0, sizeof(hVideoItem));
	memset (pVideoItem, 0, sizeof(pVideoItem));
	memset (&LastTime, 0, sizeof(LastTime));
		
	// ��¼����
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
			// ���ٵ�¼����/�رջ�����ͬ��ʱ����(ͬһ����)����2��¼����, ������ͬ��¼���ļ�����, ǰһ��ͬ���ļ��ᱻɾ��.
			// ��Ϊǰһ��ͬ�����ļ���δ��ȫ�ر�(�������̴߳����رղ���), �ᵼ��ɾ��ʧ��.
			// ��Ҫ�ܿ������
			// ���ʱ���Ƿ�һ��. ��һ��, ��ʱ, 
			if(!check_same_video_time(&CurrTime, &LastTime))
				break;
			OS_Delay (10);
			delay_count ++;
			// XM_printf ("Delay %d\n", delay_count);
		} while (delay_count < 100);		// �����ʱ1��
		LastTime = CurrTime;
		
		// �µ���Ƶ�ļ�,ȱʡ������
		video_lock_state = 0;
		
		//ǿ�ƷǼ�¼ģʽ
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
					
		// ����, ��֤���ͨ��������һ����(ȷ��ͬʱʹ����ʱ�ļ���������Ƶ�ļ���)
		XM_VideoItemLock ();
		for (channel = 0; channel < channel_count; channel ++)
		{
			// ���õ�ǰʹ�õķֱ���
			unsigned int width, height;
			width = XMSYS_SensorGetCameraChannelWidth (channel);
			height = XMSYS_SensorGetCameraChannelHeight (channel);	
			if(width == 0 || height == 0)
			{
				// ͨ��0�ĳߴ粻��Ϊ0
				if(channel == 0)
				{
					XM_printf ("Channel 0's width(%d) or height(%d) illegal\n", width, height);
					break;
				}
				continue;
			}
			
			EncodeStream.width[channel] = width;
			EncodeStream.height[channel] = height;
						
			// ���ݵ�ǰʱ��������Ƶ�ļ���
			// ����Ƿ�ͣ���������
			#if 0//���ε�gsensor����
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
				// ������ʱ��Ƶ�ļ������ڼ�¼��Ƶ��
				// 	���洢����һ�β���ʱ��ĳЩ����»�ȡ���ļ�ϵͳ����������ϳ�ʱ��(10��~30��)��������Ƶ�����ݿ���ʱ�޷�ʹ�ã�
				// 	�������޷���¼��Ƶ��
				// 	Ϊ����������⣬����һ����ʱ��Ƶ��¼�ļ������ڱ���ʵʱ��Ƶ��
				/*if( XM_VideoItemCreateVideoTempFile (channel, EncodeStream.file_name[channel], sizeof(EncodeStream.file_name[channel])))
				{
					// ����һ����ʱ��Ƶ�ļ��ɹ�
					EncodeStream.file_save = 1;
				}
				else*/
				{
					// ����һ���߼���Ƶ�ļ�(�޷�д��)
					EncodeStream.file_save = 0;
					break;
				}
			}
			else
			{
				// ��Ƶ��������ɹ�
				pVideoItem[channel] = AP_VideoItemGetVideoItemFromHandle (hVideoItem[channel]);
				if(pVideoItem[channel] == NULL)
				{
					hVideoItem[channel] = NULL;
					goto create_video_item_failed;
				}
				// ��ȡ��Ƶ���·����
				if(XM_VideoItemGetVideoFilePath (pVideoItem[channel], channel, EncodeStream.file_name[channel], sizeof(EncodeStream.file_name[channel])))
				{
					// �����ļ���
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
		// ���ͨ��0�Ƿ���Ч
		if(EncodeStream.width[0] == 0 || EncodeStream.height[0] == 0)
		{
			XM_printf ("h264codec_RecorderStart failed, illegal size(w=%d, h=%d) of channel 0\n", EncodeStream.width[0], EncodeStream.height[0]);
			rec_ret = -1;
			break;
		}
		*/
		
		if(EncodeStream.file_save == 0)
		{
			// Ԥ��ģʽ
			// ��ͨ��0�����ļ���, ����ͨ�����ļ���Ϊ��
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
		
		//������Ƶ��¼ʱ�䳤��
		//XM_printf("VideoTimeSize=%d\n", AP_GetVideoTimeSize());
		XM_printf(">>>>EncodeStream.file_save=%d\n", EncodeStream.file_save);
		if(EncodeStream.file_save)
		{
			EncodeStream.stop_ticket = OS_GetTime() + 1000 * AP_GetVideoTimeSize() + 300;		// 300 ����AVI�ļ�ʱ�䳤�ȵ����
		}
		else
		{
			EncodeStream.stop_ticket = OS_GetTime() + 1000 * 25;	
			// ÿ��25������
			//if(!XMSYS_H264CodecGetForcedNonRecordingMode())
			if(!XMSYS_H264CodecGetForcedNonRecordingMode() && PowerOnShowLogo)
			{
				// ��δ����/����Ч/���ļ�ϵͳ����ʱ, ÿ��25��������UI����
				if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
					XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM);
			}
			voice_startup = 0;
		}
		
		//����Ƿ񲥷�"¼������"����
		if(voice_startup == 0 && EncodeStream.file_save)
		{
			// ����"¼������"����
			if(AP_GetMenuItem(APPMENUITEM_VOICE_PROMPTS))
				XM_voice_prompts_insert_voice(XM_VOICE_ID_STARTUP);
			voice_startup = 1;
		}
		
		forced_switch_to_recording_mode = 0;
		
		video_format = SensorData_Get_FormatInfo();
		XM_printf(">>>>2 video_format:%d\r\n", video_format);
		XM_printf(">>>>2 h264codec_frame_rate:%d\r\n", h264codec_frame_rate);
		// ����H264���룬�ȴ�����(�ⲿ�����¼��ʱ���¼�)
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
			// ��0ֵ��ʾ�쳣
			XM_printf ("Codec stop due to exception, code = %d\n", ret);
			
			//break;
			// 20170602 codec�����쳣/�ο��Ȼᵼ���˳�. ��ʱ���˳�ѭ��, �ȴ��ⲿ����
			//if(IsCardExist(0))
			//{
				// Ͷ���쳣��Ϣ
				//XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_VERIFY_ERROR);
			//}
		}
		
		// ����Ƿ�ͣ���������
		if(XMSYS_GSensorCheckParkingCollisionStartup())
		{
			// ����Ƿ��Ǵ�Ԥ��ģʽ�л�������ģʽ
			if(EncodeStream.file_save == 0 && IsCardExist(0))
			{
				OS_Delay (100);
				if(XM_VideoItemIsBasicServiceOpened())
				{
					XM_printf ("g-sensor, switch to save mode\n");
					goto switch_to_save_state;
				}
			}
			
			// ͣ�����������ʽ
			XM_printf ("Parking Collision Recording Close\n");
			
			//XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 0);
			
			close_recycle_task ();
			
			// �ȴ���Ƶ����رղ�����
			if(EncodeStream.file_save)
			{
				XM_printf ("waiting for video Lock & close ...\n");
				for (channel = 0; channel < channel_count; channel ++)
				{
					// ������Ƶ��
					AP_VideoItemLockVideoFileFromFileName (EncodeStream.file_name[channel]);
					// �ȴ���Ƶ��ر�
					XM_VideoItemWaitingForUpdate (EncodeStream.file_name[channel]);
				}
				XM_printf ("waiting for video Lock & close end\n");
			}
			
			// �ȴ�Cache�ļ�ϵͳ������д�뵽SD��
			XMSYS_WaitingForCacheSystemFlush (5000);	

			//tony.yue 20180305 ��ӹػ���ʾLOGO
			#if 0
			{
				//OS_EnterRegion ();	// ��ֹ�����л�
				PowerOnShowLogo=0;
                if(AP_GetLogo())
                    ShowLogo (ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG,ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG_SIZE);
	            OS_Delay (2000);
				//OS_LeaveRegion();
			}
			#endif
			
			// �ػ����ȴ�ACC�ϵ�
			XM_ShutDownSystem (SDS_POWERACC);
		}

		// ���ACC�Ƿ��ACC�ϵ�ģʽ�л���ACC�ض�ģʽ
		if(xm_power_check_acc_safe_or_not() == 0)
		{
			// ACC�ϵ�-->ACC�µ�
			XM_printf ("ACC Power Down\n");
			
			//XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 0);
			
			close_recycle_task ();
			
			// �ȴ���Ƶ����رղ�����
			if(EncodeStream.file_save)
			{
				XM_printf ("waiting for video close ...\n");
				for (channel = 0; channel < channel_count; channel ++)
				{
					// �ȴ���Ƶ��ر�
					XM_VideoItemWaitingForUpdate (EncodeStream.file_name[channel]);
				}
				XM_printf ("waiting for video close end\n");
			}
			
			// �ȴ�Cache�ļ�ϵͳ������д�뵽SD��
			XMSYS_WaitingForCacheSystemFlush (5000);			

			//tony.yue 20180305 ��ӹػ���ʾLOGO
			{
				//OS_EnterRegion ();	// ��ֹ�����л�
				PowerOnShowLogo=0;
                //ShowLogo (ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG,ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG_SIZE);
	            OS_Delay (2000);
				//OS_LeaveRegion();
			}
			
			// 20180414 zhuoyonghong
			// �ر��ļ�ϵͳ����
			xm_close_volume_service ("mmc:0:");
			
			// �ػ����ȴ�ACC�ϵ�
			XM_ShutDownSystem (SDS_POWERACC);
		}		
		
	switch_to_save_state:	
		// ����˳�ԭ��
		event = OS_GetEventsOccurred (&TCB_H264CodecTask);
		
		if(event & XMSYS_H264_CODEC_EVENT_COMMAND)
		{
			// �ⲿ����
			XM_printf ("%d, Exit Codec Loop\n", XM_GetTickCount());
			
			// �ȴ���Ƶ����ر�
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
			// ͨ���л�����
			// ���¼�����
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL);
			// Ӧ��ͻ���
			reply_response_message (0);
			XM_printf ("switch channel\n");
			// ֹͣ��ǰͨ����sensor�ɼ�
			XMSYS_SensorCaptureStop ();			
			
			// �����µ�ͨ������Ƶ�ɼ�����
			XMSYS_SensorCaptureStart ();
		}	
		// ��¼�ļ���ʱ�˳�����ʼ��һ��¼���¼
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
											const char **x264_argv2, int x264_argc2,		// ������Ƶ�������
											const char **x264_argv3, int x264_argc3		// ������Ƶ�������
											);
// ��·¼��(ǰ��720P + USB����ͷ720P)
// ����ֵ	����˳�ԭ��
//	0		�ⲿ���������˳�
// 	-1		�ڲ��쳣�����˳�
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
	int rec_ret = 0;		// ����ֵ
	
	int voice_startup = 0;	// ���"¼������"�����Ƿ��Ѳ���
	unsigned int camera_format = 2;//camera_get_video_format();
	unsigned int video_format = AP_SETTING_VIDEORESOLUTION_720P_30;//AP_GetMenuItem (APPMENUITEM_VIDEO_RESOLUTION);
	// ����¼��֡��
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
		// ȱʡ1080P 30fps
		h264codec_frame_rate = 30;
	}

	
#if HIGH_VIDEO_QUALITY
	//XM_printf ("HIGH_VIDEO_QUALITY\n");
#elif NORMAL_VIDEO_QUALITY
	//XM_printf ("NORMAL_VIDEO_QUALITY\n");
#else
	//XM_printf ("GOOD_VIDEO_QUALITY\n");
#endif
	
	// ���ACC�Ƿ�ȫ����
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// �ػ����ȴ�ACC�ϵ�
		XM_ShutDownSystem (SDS_POWERACC);
	}
		
	open_recycle_task ();
	
	// ����MIC
	XMSYS_H264CodecOpenAudioRecord();	
	
	video_lock_state = 0;
	video_lock_command = 0;
	forced_encoder_stop = 0;
	
	memset (hVideoItem, 0, sizeof(hVideoItem));
	memset (pVideoItem, 0, sizeof(pVideoItem));
	memset (&LastTime, 0, sizeof(LastTime));
		
	// ��¼����
	while(1)
	{
		memset (&EncodeStream, 0, sizeof(EncodeStream));
		memset (hVideoItem, 0, sizeof(hVideoItem));
		memset (pVideoItem, 0, sizeof(pVideoItem));
		//���˵�һͨ����1080P
		if(Compare_CH0_1080P_Status())
			channel_count = 2;
		else 
			channel_count = 3;
		// XM_GetLocalTime(&CurrTime);
		delay_count = 0;
		do
		{
			XM_GetLocalTime(&CurrTime);
			// ���ٵ�¼����/�رջ�����ͬ��ʱ����(ͬһ����)����2��¼����, ������ͬ��¼���ļ�����, ǰһ��ͬ���ļ��ᱻɾ��.
			// ��Ϊǰһ��ͬ�����ļ���δ��ȫ�ر�(�������̴߳����رղ���), �ᵼ��ɾ��ʧ��.
			// ��Ҫ�ܿ������
			// ���ʱ���Ƿ�һ��. ��һ��, ��ʱ, 
			if(!check_same_video_time (&CurrTime, &LastTime))
				break;
			OS_Delay (10);
			delay_count ++;
			// XM_printf ("Delay %d\n", delay_count);
		} while (delay_count < 100);		// �����ʱ1��
		LastTime = CurrTime;
		
		// �µ���Ƶ�ļ�,ȱʡ������
		video_lock_state = 0;
		
		// ǿ�ƷǼ�¼ģʽ
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
					
		// ����, ��֤���ͨ��������һ����(ȷ��ͬʱʹ����ʱ�ļ���������Ƶ�ļ���)
		XM_VideoItemLock ();
		for (channel = 0; channel < channel_count; channel ++)
		{
			// ���õ�ǰʹ�õķֱ���
			unsigned int width, height;
			width = XMSYS_SensorGetCameraChannelWidth (channel);
			height = XMSYS_SensorGetCameraChannelHeight (channel);
			if(width == 0 || height == 0)
			{
				// ͨ��0�ĳߴ粻��Ϊ0
				if(channel == 0)
				{
					XM_printf ("Channel 0's width(%d) or height(%d) illegal\n", width, height);
					break;
				}
				continue;
			}
			
			EncodeStream.width[channel] = width;
			EncodeStream.height[channel] = height;
			// ���ݵ�ǰʱ��������Ƶ�ļ���
			// ����Ƿ�ͣ���������
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
				// ������ʱ��Ƶ�ļ������ڼ�¼��Ƶ��
				// 	���洢����һ�β���ʱ��ĳЩ����»�ȡ���ļ�ϵͳ����������ϳ�ʱ��(10��~30��)��������Ƶ�����ݿ���ʱ�޷�ʹ�ã�
				// 	�������޷���¼��Ƶ��
				// 	Ϊ����������⣬����һ����ʱ��Ƶ��¼�ļ������ڱ���ʵʱ��Ƶ��
				/*if( XM_VideoItemCreateVideoTempFile (channel, EncodeStream.file_name[channel], sizeof(EncodeStream.file_name[channel])))
				{
					// ����һ����ʱ��Ƶ�ļ��ɹ�
					EncodeStream.file_save = 1;
				}
				else*/
				{
					// ����һ���߼���Ƶ�ļ�(�޷�д��)
					EncodeStream.file_save = 0;
					break;
				}
			}
			else
			{
				// ��Ƶ��������ɹ�
				pVideoItem[channel] = AP_VideoItemGetVideoItemFromHandle (hVideoItem[channel]);
				if(pVideoItem[channel] == NULL)
				{
					hVideoItem[channel] = NULL;
					goto create_video_item_failed;
				}
				// ��ȡ��Ƶ���·����
				if(XM_VideoItemGetVideoFilePath (pVideoItem[channel], channel, EncodeStream.file_name[channel], sizeof(EncodeStream.file_name[channel])))
				{
					// �����ļ���
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
		// ���ͨ��0�Ƿ���Ч
		if(EncodeStream.width[0] == 0 || EncodeStream.height[0] == 0)
		{
			XM_printf ("h264codec_RecorderStart failed, illegal size(w=%d, h=%d) of channel 0\n", EncodeStream.width[0], EncodeStream.height[0]);
			rec_ret = -1;
			break;
		}
		*/
		
		if(EncodeStream.file_save == 0)
		{
			// Ԥ��ģʽ
			// ��ͨ��0�����ļ���, ����ͨ�����ļ���Ϊ��
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
		
		// ������Ƶ��¼ʱ�䳤��
		//XM_printf ("VideoTimeSize=%d\n", AP_GetVideoTimeSize());
		if(EncodeStream.file_save)
		{
			EncodeStream.stop_ticket = OS_GetTime() + 1000 * AP_GetVideoTimeSize() + 300;		// 100 ����AVI�ļ�ʱ�䳤�ȵ����
		}
		else
		{
			EncodeStream.stop_ticket = OS_GetTime() + 1000 * 25;	
			// ÿ��25������
			//if(!XMSYS_H264CodecGetForcedNonRecordingMode())
			if(!XMSYS_H264CodecGetForcedNonRecordingMode() && PowerOnShowLogo)
			{
				// ��δ����/����Ч/���ļ�ϵͳ����ʱ, ÿ��25��������UI����
				if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
					XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM);
			}
			voice_startup = 0;
		}
		
		// ����Ƿ񲥷�"¼������"����
		if(voice_startup == 0 && EncodeStream.file_save)
		{
			// ����"¼������"����
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				XM_voice_prompts_insert_voice (XM_VOICE_ID_STARTUP);
			voice_startup = 1;
		}
		
		forced_switch_to_recording_mode = 0;
		
		ret = 0;
		// ����H264���룬�ȴ�����(�ⲿ�����¼��ʱ���¼�)
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
											argv_720P, sizeof(argv_720P)/sizeof(char *),		// ������Ƶ�������
											argv_720P, sizeof(argv_720P)/sizeof(char *)		// ������Ƶ�������
											);
		}else if(video_format == AP_SETTING_VIDEORESOLUTION_720P_NSTL_30)
		{
			ret = h264_encode_avi_stream_three_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], EncodeStream.file_name[2],
											&EncodeStream,
											argv_720P, sizeof(argv_720P)/sizeof(char *),
											argv_cvbs_ntsc, sizeof(argv_cvbs_ntsc)/sizeof(char *),		// ������Ƶ�������
											argv_720P, sizeof(argv_720P)/sizeof(char *)		// ������Ƶ�������
											);
		}else if(video_format == AP_SETTING_VIDEORESOLUTION_720P_PAL_30)
		{
			ret = h264_encode_avi_stream_three_channel (EncodeStream.file_name[0], EncodeStream.file_name[1], EncodeStream.file_name[2],
											&EncodeStream,
											argv_720P, sizeof(argv_720P)/sizeof(char *),
											argv_cvbs_pal, sizeof(argv_cvbs_pal)/sizeof(char *),		// ������Ƶ�������
											argv_720P, sizeof(argv_720P)/sizeof(char *)		// ������Ƶ�������
											);
		}
		#if 0
		if(video_format == AP_SETTING_VIDEORESOLUTION_1080P_30)
		{
		  //ÿ��¼�µ��ļ��ж���û�к�������,���û�оͲ�¼����
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
										// 7116ʹ��VGA�ֱ������
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
											argv_720P, sizeof(argv_720P)/sizeof(char *),		// ������Ƶ�������
											argv_720P, sizeof(argv_720P)/sizeof(char *)		// ������Ƶ�������
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
			// ��0ֵ��ʾ�쳣
			XM_printf ("Codec stop due to exception, code = %d\n", ret);
			
			//break;
			// 20170602 codec�����쳣/�ο��Ȼᵼ���˳�. ��ʱ���˳�ѭ��, �ȴ��ⲿ����
			//if(IsCardExist(0))
			//{
				// Ͷ���쳣��Ϣ
				//XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_VERIFY_ERROR);
			//}
		}
		
		// ����Ƿ�ͣ���������
		if(XMSYS_GSensorCheckParkingCollisionStartup())
		{
			// ����Ƿ��Ǵ�Ԥ��ģʽ�л�������ģʽ
			if(EncodeStream.file_save == 0 && IsCardExist(0))
			{
				OS_Delay (100);
				if(XM_VideoItemIsBasicServiceOpened())
				{
					XM_printf ("g-sensor, switch to save mode\n");
					goto switch_to_save_state;
				}
			}
			
			// ͣ�����������ʽ
			XM_printf ("Parking Collision Recording Close\n");
			
			//XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 0);
			
			close_recycle_task ();
			
			// �ȴ���Ƶ����رղ�����
			if(EncodeStream.file_save)
			{
				XM_printf ("waiting for video Lock & close ...\n");
				for (channel = 0; channel < channel_count; channel ++)
				{
					// ������Ƶ��
					AP_VideoItemLockVideoFileFromFileName (EncodeStream.file_name[channel]);
					// �ȴ���Ƶ��ر�
					XM_VideoItemWaitingForUpdate (EncodeStream.file_name[channel]);
				}
				XM_printf ("waiting for video Lock & close end\n");
			}
			
			// �ȴ�Cache�ļ�ϵͳ������д�뵽SD��
			XMSYS_WaitingForCacheSystemFlush (5000);	

			//tony.yue 20180305 ��ӹػ���ʾLOGO
			{
				//OS_EnterRegion ();	// ��ֹ�����л�
				PowerOnShowLogo=0;
                //if(AP_GetLogo())
                //    ShowLogo (ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG,ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG_SIZE);
	            OS_Delay (2000);
				//OS_LeaveRegion();
			}
						
			// �ػ����ȴ�ACC�ϵ�
			XM_ShutDownSystem (SDS_POWERACC);
		}

		// ���ACC�Ƿ��ACC�ϵ�ģʽ�л���ACC�ض�ģʽ
		if(xm_power_check_acc_safe_or_not() == 0)
		{
			// ACC�ϵ�-->ACC�µ�
			XM_printf ("ACC Power Down\n");
			
			//XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 0);
			
			close_recycle_task ();
			
			// �ȴ���Ƶ����رղ�����
			if(EncodeStream.file_save)
			{
				XM_printf ("waiting for video close ...\n");
				for (channel = 0; channel < channel_count; channel ++)
				{
					// �ȴ���Ƶ��ر�
					XM_VideoItemWaitingForUpdate (EncodeStream.file_name[channel]);
				}
				XM_printf ("waiting for video close end\n");
			}
			
			// �ȴ�Cache�ļ�ϵͳ������д�뵽SD��
			XMSYS_WaitingForCacheSystemFlush (5000);			

			//tony.yue 20180305 ��ӹػ���ʾLOGO
			{
				//OS_EnterRegion ();	// ��ֹ�����л�
				PowerOnShowLogo=0;
                //ShowLogo (ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG,ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG_SIZE);
	            //OS_Delay (2000);
				//OS_LeaveRegion();
			}
			
			// 20180414 zhuoyonghong
			// �ر��ļ�ϵͳ����
			xm_close_volume_service ("mmc:0:");
			
			// �ػ����ȴ�ACC�ϵ�
			XM_ShutDownSystem (SDS_POWERACC);
		}		
		
	switch_to_save_state:	
		// ����˳�ԭ��
		event = OS_GetEventsOccurred (&TCB_H264CodecTask);
		
		if(event & XMSYS_H264_CODEC_EVENT_COMMAND)
		{
			// �ⲿ����
			XM_printf ("%d, Exit Codec Loop\n", XM_GetTickCount());
			
			// �ȴ���Ƶ����ر�
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
			// ͨ���л�����
			// ���¼�����
			OS_WaitSingleEvent (XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL);
			// Ӧ��ͻ���
			reply_response_message (0);
			XM_printf ("switch channel\n");
			// ֹͣ��ǰͨ����sensor�ɼ�
			XMSYS_SensorCaptureStop ();			
			
			// �����µ�ͨ������Ƶ�ɼ�����
			XMSYS_SensorCaptureStart ();
		}	
		// ��¼�ļ���ʱ�˳�����ʼ��һ��¼���¼
		//XM_printf ("h264_encoder_double_stream end\n");	
	}
	
	// XM_WaveClose (XMMCIDEVICE_RECORD);
	XMSYS_H264CodecCloseAudioRecord ();
		
	close_recycle_task ();
	
	return rec_ret;
}


// ����ֵ
// 0		-->	��������
//	1		-->	�����˳�
//	2		-->	ǰ��
//	3		-->	����
// 5     -->   �طŶ�λ(��)
int XMSYS_H264DecoderCheckEvent (struct H264DecodeStream *stream)
{
	char file_name[XMSYS_H264_CODEC_MAX_FILE_NAME + 1];
	u8_t event;
	int ret = 0;
	stream->curr_ticket += 33;
	
	// ����ⲿACC��Դ״��
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// XM_printf ("codec stop, acc bad\n");
		// �ǰ�ȫ��ѹ
		// ACC�ϵ� --> ACC����, ׼���ػ�
		return 1;		
	}
		
	// ����Ƿ�����ⲿ�¼�
	event = OS_GetEventsOccurred(&TCB_H264CodecTask);

	//XM_printf(">>>>>>>>>>>XMSYS_H264DecoderCheckEvent, event:%d\r\n", event);
	//XM_printf(">>>>>>>>>>>XMSYS_H264DecoderCheckEvent, stream->system_ticket:%d\r\n", stream->system_ticket);
	//XM_printf(">>>>>>>>>>>XMSYS_H264DecoderCheckEvent, stream->curr_ticket:%d\r\n", stream->curr_ticket);

	
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
			XM_printf ("playback end due to PLAYER_STOP, %d\n", XM_GetTickCount());
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
						OS_WaitSingleEvent(XMSYS_H264_CODEC_EVENT_COMMAND);
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
	OS_Delay (1);
	// �ȴ�ֱ����һ�ζ�ʱ���¼�
	return 0;
	
}
UCHAR Play_Next_Video_keyflag= FALSE;
UCHAR Play_Next_Video_keyflag_time= 0;
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
			h264codec_command_param.command = 0;		// �������
			// �������
			OS_Unuse(&h264codec_sema); /* Make sure nobody else uses */

			switch (command.command)
			{
				case XMSYS_H264_CODEC_COMMAND_RECORDER_START:
					XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>XMSYS_H264_CODEC_COMMAND_RECORDER_START\r\n");
					OS_Use(&h264codec_sema); /* Make sure nobody else uses */
					h264codec_mode = XMSYS_H264_CODEC_MODE_RECORD;
					h264codec_state = XMSYS_H264_CODEC_STATE_WORK;

					OS_Unuse(&h264codec_sema); 
					
					// Ӧ��UI�߳�
					ret = 0;
					OS_PutMail1 (&h264codec_mailbox, &ret);	
					clear_codec_command();
					XMSYS_SensorCaptureStart();
					
					// ����h264 codec����
					#if _XM_PROJ_ == _XM_PROJ_2_SENSOR_1080P_CVBS 
					//codec_ret = h264codec_RecorderStart_two_channel();
					codec_ret = h264codec_RecorderStart_two_channel();
					#else
					codec_ret = h264codec_RecorderStart();
					#endif
					if(codec_ret < 0)
					{
						// ��Ƶ¼��δ����ȷ����, ����codec״̬, ����״̬����
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

					// Ӧ��UI�߳�
					//ret = 0;
					//OS_PutMail1 (&h264codec_mailbox, &ret);	

					clear_codec_command ();
					break;

				case XMSYS_H264_CODEC_COMMAND_PLAYER_START://�������
					// ֹͣSensor
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
					// ��ǰ��֧��һ����ͬ������
					strcpy (dec_stream.file_name[0], (char *)command.file_name);
					// Ӧ��UI�߳�
					ret = 0;
					OS_PutMail1(&h264codec_mailbox, &ret);	//

					// ��������
					XM_printf(">>>>>>decode_avi %s\n", file_name);

					// ����Ƶ
					waveFormat.wChannels = 1;
					waveFormat.wBitsPerSample = 16;
					waveFormat.dwSamplesPerSec = 8000;

					// ��ָ���ļ�������Ƶ���ʹ���У���ֹɾ����������Ƶ��ɾ��
					video_usage = XM_VideoItemMarkVideoFileUsage(file_name);
					
					result = XM_WaveOpen(XMMCIDEVICE_PLAY, &waveFormat);
					(void)(result); // remove compiler warning
					
					{
						// ����LCD����, ��ֹ�Զ�����
						unsigned int old_lcd_setting = AP_GetMenuItem (APPMENUITEM_LCD);
						AP_SetMenuItem(APPMENUITEM_LCD, AP_SETTING_LCD_NEVERCLOSE);
						
						codec_ret = h264_decode_avi_stream(file_name,	NULL,	&dec_stream);//����API�ӿ�
						
						// �ָ�LCD����
						AP_SetMenuItem (APPMENUITEM_LCD, old_lcd_setting);
					}
					
					XM_WaveClose (XMMCIDEVICE_PLAY);
					
					// ����ⲿACC��Դ״��
					if(xm_power_check_acc_safe_or_not() == 0)
					{
						XM_printf ("video play stop, acc bad\n");
						// �ǰ�ȫ��ѹ
						// �ػ����ȴ�ACC�ϵ�
						XM_ShutDownSystem (SDS_POWERACC);
					}
					
					if(video_usage)
						XM_VideoItemMarkVideoFileUnuse (file_name);
					
					//XM_printf ("close Output\n");
					//XMSYS_VideoSetImageOutputReady (0, 0, 0);

					XM_printf(">>>>>>>>>>>>>>>>check other file if not player.......\r\n");
					XM_printf(">>>>>>>h264codec_command_param.command:%d\r\n", h264codec_command_param.command);
					
					// ����Ƿ���������Ĳ����ļ�
					playback_again = 0;
					OS_Use(&h264codec_sema);
					// ����Ƿ񲥷��µĲ����ļ�
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
					// ��UI�̷߳��Ͳ��Ž�����Ϣ

					
					if(codec_ret == 0)
						//XM_printf(">>>>>>>>>>>play pause...........\r\n");
						XM_KeyEventProc (VK_AP_VIDEOSTOP, (SHORT)(AP_VIDEOEXITCODE_FINISH | (h264codec_state << 8)) );//���Ͳ������״̬
					else
						XM_KeyEventProc(VK_AP_VIDEOSTOP, AP_VIDEOEXITCODE_STREAMERROR);
					break;

				case XMSYS_H264_CODEC_COMMAND_PLAYER_STOP://ֹͣ����
					XM_printf(">>>>>>>>>>>>>>>>XMSYS_H264_CODEC_COMMAND_PLAYER_STOP %d\n", XM_GetTickCount());
					OS_Use(&h264codec_sema); /* Make sure nobody else uses */
					h264codec_mode = XMSYS_H264_CODEC_MODE_IDLE;
					h264codec_state = XMSYS_H264_CODEC_STATE_STOP;
					OS_Unuse(&h264codec_sema); 
					
					clear_codec_command ();
					
					//Ӧ��UI�߳�
					ret = 0;
					OS_PutMail1(&h264codec_mailbox, &ret);	//

					// ����Sensor
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
			// �л�¼��ͨ��
		}
	}

}


void XMSYS_H264CodecInit (void)
{
	//H264 Codec Cache�ļ���ʼ��
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


// ������Ƶ¼��
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
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
		
	// ��������Ӧ����Ϣ
	clear_response_message ();
	
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	
	if(h264codec_command_param.command)
	{
		XM_printf ("command %d busy\n", h264codec_command_param.command);
		OS_Unuse(&h264codec_sema);
		return -1;
	}
	
	// ��鵱ǰģʽ
	if(h264codec_mode == XMSYS_H264_CODEC_MODE_RECORD)
	{
		// �Ѵ��ڼ�¼ģʽ
		XM_printf ("Warning��RecorderStart reject, repeat command\n");
		OS_Unuse(&h264codec_sema);
		return 0;
	}
	
	OS_Unuse(&h264codec_sema);
	
	if(h264codec_mode == XMSYS_H264_CODEC_MODE_PLAY)//�ط�ģʽ
	{
		// ��ǰ���ڲ���ģʽ
		// ��ֹ��ǰ����Ƶ����
		ret = XMSYS_H264CodecPlayerStop ();
		if(ret)
		{
			XM_printf ("RecorderStart reject\n");
			return ret;
		}
	}
	
	// ��鵱ǰ��״̬
	if(XMSYS_H264CodecGetMode() != XMSYS_H264_CODEC_MODE_IDLE)
	{//�쳣
		XM_printf(">>>>Mode %s, state %s reject RecorderStart command\n", XMSYS_H264CodecGetModeCode(), XMSYS_H264CodecGetStateCode());
		return -1;
	}
	
	XM_printf(">>>>RECORDER_START...............\r\n\n");
	XM_AppInitStartupTicket();
	OS_Use(&h264codec_sema); /* Make sure nobody else uses */
	
	//h264codec_command_param.id = XMSYS_H264_CODEC_COMMAND_ID;
	h264codec_command_param.command = XMSYS_H264_CODEC_COMMAND_RECORDER_START;
	OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* ֪ͨ�¼� */
	OS_Unuse(&h264codec_sema);
	
	// �ȴ�H264 Codec�߳�Ӧ��
	if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
	{
		// codec��Ӧ�𣬻����Ϣ
		ret = (int)response_code;
		printf(">>>>XMSYS_VideoOpen \n");
		XMSYS_VideoOpen();
	}
	else
	{
		// ��ʱ
		XM_printf (">>>>RecorderStart timeout\n");
		ret = -2;
	}
	XM_printf(">>>>fun XMSYS_H264CodecRecorderStart end......\r\n");
	return ret;
}


// ���H264������Ƿ�æ
// 1 busy
// 0 idle
int XMSYS_H264CodecCheckBusy (void)
{
	if(h264codec_command_param.command)
		return 1;
	else
		return 0;
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
		XM_printf ("%d,RECORDER_STOP\n",XM_GetTickCount());
		
		// �ر�H264������
		h264codec_command_param.command = XMSYS_H264_CODEC_COMMAND_RECORDER_STOP;
		OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* ֪ͨ�¼� */
		OS_Unuse(&h264codec_sema);
		
		// �ȴ�H264 Codec�߳�Ӧ��
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
		{
			// codec��Ӧ�𣬻����Ϣ
			ret = (int)response_code;
			XM_printf ("%d,RecorderStop resp=%d\n", XM_GetTickCount(),ret);
			
			// ��ʱ1�룬�ȴ�codecֹͣ
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
			#if 1  //������ٽ���¼���б������Ƴ�¼���б���ʱ�����ֹͣ¼�����������¼����棬���º���
			XM_SetFmlDeviceCap (DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_STOP);
			#endif
		}
		else
		{
			// ��ʱ
			XM_printf ("RecorderStop timeout\n");
			ret = -2;
		}
		
		// ֹͣ��¼
		// �ر���Ƶ���
		XMSYS_VideoClose ();
		printf(" XMSYS_VideoClose \n");
		// ֹͣsensor
		XM_printf ("%d,XMSYS_SensorCaptureStop\n",XM_GetTickCount());
		XMSYS_SensorCaptureStop ();

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
	}
	*/
		
	XM_printf ("SwitchChannel %d\n", camera_channel);
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
	
	if(OS_GetMailTimed(&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
	{
		// codec��Ӧ�𣬻����Ϣ
		ret = (int)response_code;
		rendervideo_start_ticket = OS_GetTime();

		XM_printf(">>>>>>rendervideo_start_ticket:%d\r\n", rendervideo_start_ticket);
		XM_printf(">>>>>>>>PlayerStart resp=%d\r\n", ret);
	}
	else
	{
		// ��ʱ
		XM_printf("PlayerStart timeout\n");
		ret = -2;
	}
	return ret;
}

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
		XM_printf ("PLAYER_STOP %d\n", XM_GetTickCount());
		
		h264codec_command_param.command = XMSYS_H264_CODEC_COMMAND_PLAYER_STOP;
		OS_SignalEvent(XMSYS_H264_CODEC_EVENT_COMMAND, &TCB_H264CodecTask); /* ֪ͨ�¼� */
		OS_Unuse(&h264codec_sema);
		
		// �ȴ�H264 Codec�߳�Ӧ��
		if(OS_GetMailTimed (&h264codec_mailbox, &response_code, CODEC_TIMEOUT) == 0)
		{
			// codec��Ӧ��
			ret = response_code;
			rendervideo_start_ticket = OS_GetTime();
			XM_printf ("PlayerStop resp=%d\n", ret);
		}
		else
		{
			XM_printf ("PlayerStop timeout\n");
			ret = -2;	// ��ʱ������
		}
		
		// �ȴ���Ƶ��ʾ�߳�ˢ�½�������һ֡ͼ��
		//OS_Delay (100);
		
		// �ر���Ƶ���(OSD 0��ر�)
		//XM_osd_framebuffer_release (0, XM_OSD_LAYER_0);
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
			rendervideo_start_ticket = OS_GetTime();
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
			rendervideo_start_ticket = OS_GetTime();
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
			rendervideo_start_ticket = OS_GetTime();
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
			rendervideo_start_ticket = OS_GetTime();
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

// ֹͣ��Ƶ�������
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
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
static volatile int do_recycle;		// ����Ƿ�����ִ�л��չ��� 1 ����ִ��

static void XMSYS_RecycleTask (void)
{
	int count = 0;
	OS_Delay (2000);		// 2���ʼ
	while (1)
	{
		if(recycle_task_stop)
		{
			OS_Delay (100);
			continue;
		}
		
		if(count % 200 == 0 || do_recycle)	// ÿ20��ɨ����̽��л��ջ����ⲿ��������ִ�л��չ���
		{
			do_recycle = 0;
			XM_VideoItemMonitorAndRecycleGarbage(0);
		}
		OS_Delay (100);	
		count ++;
	} 
}

static void open_recycle_task (void)		// ����ѭ����������
{
	// �����������
	recycle_task_stop = 0;
}

static void close_recycle_task (void)		// �ر�ѭ����������
{
	recycle_task_stop = 1;
}

// ��������ִ�л��չ���
void XMSYS_DoRecycle (void)
{
	do_recycle = 1;
}

void XMSYS_RecycleTaskInit (void)
{
	recycle_task_stop = 1;
	OS_CREATETASK(&TCB_RecycleTask, "RecycleTask", XMSYS_RecycleTask, XMSYS_RECYCLE_TASK_PRIORITY, StackRecycleTask);
}

// ����ǿ�ƷǼ�¼ģʽ
// forced_non_recording
//		1			ǿ�ƷǼ�¼ģʽ (��ģʽ�²���¼��, ���������ģʽ)
//		0			��ͨ��¼ģʽ 
void XMSYS_H264CodecSetForcedNonRecordingMode(int forced_non_recording)
{
	int start_record = 0;

	XM_printf(">>>>forced_non_recording_mode:%d\r\n", forced_non_recording_mode);
	
	XM_lock ();
	if( (forced_non_recording_mode == 1) && (forced_non_recording == 0))
	{
		// ǿ�ƷǼ�¼ģʽ --> ��ͨ��¼ģʽ 
		// ��������¼��
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


// ��鵱ǰģʽ�Ƿ���ǿ�ƷǼ�¼ģʽ
// ����ֵ
// 	1			ǿ�ƷǼ�¼ģʽ (��ģʽ�²���¼��, ���������ģʽ)
//	0			��ͨ��¼ģʽ 
int XMSYS_H264CodecGetForcedNonRecordingMode (void)
{
	return forced_non_recording_mode;
}

#if 1
// ��鵱ǰģʽ�Ƿ��Ѿ��ɹ��˳�¼��ģʽ
// ����ֵ
// 	-1			�ǳɹ��˳�¼��ģʽ
//	0			�ɹ��˳�¼��ģʽ
int  CheckExitRecord(void)
{
	if(!XMSYS_H264CodecGetForcedNonRecordingMode())
	{
		if(XM_GetFmlDeviceCap (DEVCAP_VIDEO_REC) == DEVCAP_VIDEO_REC_STOP)
		{
		   	// ��ʱ4�룬�ȴ�codecֹͣ
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

