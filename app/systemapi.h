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

#ifndef _XM_APP_SYSTEM_API_H_
#define _XM_APP_SYSTEM_API_H_

#if defined (__cplusplus)
	extern "C"{
#endif

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

#define LOW_BATTERY_INDICATOR				0x01000000
#define INITIAL_INDICATOR					0x02000000
#define CARD_PROTECTION_ERR_INDICATOR	0x04000000
#define CARD_FILESYSTEM_ERR_INDICATOR	0x08000000
#define NO_CARD_INDICATOR					0x10000000

#define CCD_STATUS_INDICATOR				0x000F0000
#define CCD1_STATUS_INDICATOR				0x00010000
#define CCD2_STATUS_INDICATOR				0x00020000
#define CCD3_STATUS_INDICATOR				0x00040000
#define RCD_CAPACITY_INDICATOR			0x00000FFF

#define RCD_CAPACITY(x)					((x) & RCD_CAPACITY_INDICATOR)
#define CCD_LINK_STATUS(x)					(((x) & CCD_STATUS_INDICATOR)>>16)


DWORD GetSystemStatus ();

enum
	{
	SYSTEM_SETTING_DELAY_RCD = 0,
	SYSTEM_SETTING_MIC,
	SYSTEM_SETTING_CCD_1_POSITION,
	SYSTEM_SETTING_CCD_2_POSITION,
	SYSTEM_SETTING_CCD_3_POSITION
	};

#define SYSTEM_SETTING_MIC_MUTE	0
#define SYSTEM_SETTING_MIC_ON		(!0)

#define CCD_POSITION_REAR			0
#define CCD_POSITION_LEFT			1
#define CCD_POSITION_RIGHT		2
#define CCD_POSITION_INNER		3

/*
	返回指定的设置值
*/
DWORD GetSystemSetting  (unsigned short SettingID);



void SetPlaybackMode (int iPlaybackMode);

// 启动录像
// 返回值
// 0   success
// -1  failure
int StartRecord (void);

// 停止录像
// 返回值
// 0   success
// -1  failure
int StopRecord (void);


/*
ReservePlaybackArea
	指定系统回放时不要覆盖的区域，可提供多个区域（2个是否足够？）
*/

/*
DisplayPlaybackArea
	在回放显示缓冲区显示给定的位图
*/

USHORT GetRcdForecastTimeVoice(int Min);
USHORT GetCCDPositionVoice (unsigned short CCDNumber);
void DisplayRemaindTimer (HANDLE hWnd, int Second);
void CancelDisplayRemainTimer ();
void ReservePlaybackArea (XMRECT *rc);
u32_t GetCCDStatusIcon();

#define START		1
#define STOP		0

void PlaybackControl(int RcdCtrl, int Mode, int PlaybackMode);

// 卡格式化函数
int StartFormatProgress (void *private_data);
int QueryFormatProgress (void *private_data);

// 出厂设置恢复函数
int StartRestoreProgress (void *private_data);
int QueryRestoreProgress (void *private_data);
int EndDefaultParamProgress (void *private_data);

// 系统升级函数
int StartUpdateProgress (void *private_data);
int QueryUpdateProgress (void *private_data);

// 检查系统升级文件
DWORD XM_GetUpdateFileStatus (void);

APPMENUID XM_GetUpdateFileVersion (void);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_APP_MENUDATA_H_

