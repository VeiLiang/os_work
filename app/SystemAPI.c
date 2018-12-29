//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: SystemAPI.c
//	  行车记录仪对UI 提供的API
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************

#ifdef _WINDOWS

#include <Windows.h>
#include <stdlib.h>

#else

#include "fs.h"

#endif

#include "app.h"
#include "app_voice.h"
#include "app_menuid.h"

#include "system_check.h"
#include "systemapi.h"
#include <xm_voice_prompts.h>
#include "xm_h264_codec.h"
#include "rtc.h"

extern int XMSYS_H264CodecRecorderStart (void);
extern int XMSYS_H264CodecRecorderStop  (void);
extern int	XMSYS_VideoSetCameraMode (int camera_mode);
extern XMSYSTEMTIME get_default_time(void);



/*
GetSystemStatus 
	返回系统状态，如果系统自检未完成，等待自检完成后再返回
	返回值:
	0x01000000		LOW_BATTERY_INDICATOR
	0x02000000		INITIAL_INDICATOR
	0x04000000		CARD_PROTECTION_ERR_INDICATOR
	0x08000000		CARD_FILESYSTEM_ERR_INDICATOR
	0x10000000		NO_CARD_INDICATOR

	0x000F0000		CCD_STATUS_INDICATOR
	0x00010000		CCD1 Installed, But lost connection
	0x00020000		CCD2 Installed, But lost connection
	0x00040000		CCD3 Installed, But lost connection

	0x00000FFF		RCD_CAPACITY_INDICATOR
*/

DWORD GetSystemSetting (unsigned short SettingID)
{
	return 0;
}




/*
设置回放方式:
enum {
	PLAYBACK_MAIN 	= 0,		// Play back main CCD only
	PLAYBACK_REAR,
//	PLAYBACK_LEFT,
//	PLAYBACK_RIGHT,
//	PLAYBACK_MAIN_AND_REAR,
	PLAYBACK_MAX_MODE
};

*/

void SetPlaybackMode (int iPlaybackMode)
{
#ifdef PCVER
#else
	XMSYS_VideoSetCameraMode (-1);	
#endif
}


extern int CheckExitRecord(void);

/*
开始记录
*/
// 启动录像
// 返回值
// 0   success
// -1  failure
int StartRecord (void)
{
	XM_printf(">>>>>StartRecord...........\r\n");
	
 	//解决快速进入录像列表，快速推出录像列表，有时候出现停止录像出现在启动录像后面，导致黑屏
	#if 1
    if(CheckExitRecord() == (-1))
	  return -1;
	#endif

	// 检查是否已经是录像模式
	if(XMSYS_H264CodecGetMode() == XMSYS_H264_CODEC_MODE_RECORD)
	{
		// 检查codec是否忙(如正在执行退出指令)。若忙， 返回失败
		if( XMSYS_H264CodecCheckBusy() == 1 )
		{
			XM_printf ("StartRecord NG, codec busy\n");
			return -1;
		}
		return 0;
	}
	if( XMSYS_H264CodecRecorderStart() == (-1) )
		return -1;
	
	XM_SetFmlDeviceCap(DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_START);
	
	return 0;
}

// 停止录像
// 返回值
// 0   success
// -1  failure
int StopRecord (void)
{
	int ret = -1;
	if(XMSYS_H264CodecRecorderStop() == 0)
	{
		XM_SetFmlDeviceCap (DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_STOP);
		ret = 0;
	}
	return ret;
}


/*
PlaybackControl
*/

void PlaybackControl(int RcdCtrl, int Mode, int PlaybackMode)
{
}

/*
 StartxxxProgress	开始xxx 过程
 QueryxxxProgress 	查询进度

*/
static int progress;

int StartFormatProgress (void *private_data)
{
	int ret;
#ifdef _WINDOWS
	XM_Sleep (15*1000);
	ret = 0;
#else
	XM_printf ("FS_Format ...\n");
	
	FS_FORMAT_INFO FormatInfo;
	FormatInfo.SectorsPerCluster = 64;
	FormatInfo.NumRootDirEntries = 512;
	FormatInfo.pDevInfo = NULL;
	//ret = FS_Format ("mmc:0:", NULL);
	ret = FS_Format ("mmc:0:", &FormatInfo);
	XM_printf ("FS_Format ret=%d\n", ret);
#endif

	if(ret == 0)
	{
		// 命令执行成功
		// 清除“文件系统错误”或者“读写校验失败”异常

	}
	
	return ret;
}

int QueryFormatProgress (void *private_data)
{
	// 最大进度10
	return 0;
}

extern int AP_RestoreMenuData (void);

// 0  表示成功
// -1 表示失败
int StartRestoreProgress (void *private_data)
{
	u8_t update_date_falg;
	u8_t lcd_vcom;
	
	update_date_falg = AppMenuData.update_date_falg;
	lcd_vcom = AppMenuData.VCOM_PWM;
	
	if(AP_RestoreMenuData () == 1)
	{
		AppMenuData.update_date_falg = update_date_falg;
		AppMenuData.VCOM_PWM = lcd_vcom;
		return 0;
	}
	else
	{
		return -1;
	}
}

int QueryRestoreProgress (void *private_data)
{
	// 最大进度1
	return 1;
}


//复位完成后执行相应操作
int EndDefaultParamProgress (void *private_data)
{
	XMSYSTEMTIME defaulttime;

	XM_printf(">>>>>>>>>>>>EndDefaultParamProgress...........\r\n");
	
	//恢复翻转
	XM_printf(">>>>>>>>>>>>AP_GetLCD_Rotate():%d...........\r\n", AP_GetLCD_Rotate());
	HW_LCD_ROTATE(AP_GetLCD_Rotate());

	//恢复默认时间
	if(AppMenuData.update_date_falg==0x0)//未更新
	{
		defaulttime = get_default_time();
		XM_SetLocalTime(&defaulttime);
		AppMenuData.update_date_falg = 1;
	}

	XM_RxchipDisplayInit();

	set_pwm_duty(AP_GetVCOM_PWM());
	APP_SaveMenuData();
}


static unsigned int system_update_ticket;

int StartUpdateProgress (void *private_data)
{
	int ret = 0;
#ifdef _WINDOWS
	system_update_ticket = XM_GetTickCount();
	XM_Sleep (10*1000);
	ret = 0;
#else
	// 刷新SPI程序区
	extern int XMSYS_system_update (void);
	ret = XMSYS_system_update ();
#endif
	return ret;
}


// 获取系统升级过程当前的阶段, 
// 0 ~ 100		表示升级完成的百分比
// 0xFF			表示升级已失败
unsigned int XMSYS_system_update_get_step (void);

int QueryUpdateProgress (void *private_data)
{
	int step;
	// 最大进度10
#ifdef _WINDOWS
	step = (GetTickCount() - system_update_ticket) * 100 / 10000;
	if(step >= 100)
		step = 100;

#else
	step = XMSYS_system_update_get_step ();
#endif

	return step;
}

/*
 XM_GetUpdateFileStatus		升级文件是否正常(文件存在，CRC  OK)
 QueryxxxProgress 	升级文件的版本(文件内的图片资源)

*/

DWORD XM_GetUpdateFileStatus ()
{
	return 1;
}

