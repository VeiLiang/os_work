//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: SystemAPI.c
//	  �г���¼�Ƕ�UI �ṩ��API
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
	����ϵͳ״̬�����ϵͳ�Լ�δ��ɣ��ȴ��Լ���ɺ��ٷ���
	����ֵ:
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
	����ָ��������ֵ
*/
DWORD GetSystemSetting  (unsigned short SettingID);



void SetPlaybackMode (int iPlaybackMode);

// ����¼��
// ����ֵ
// 0   success
// -1  failure
int StartRecord (void);

// ֹͣ¼��
// ����ֵ
// 0   success
// -1  failure
int StopRecord (void);


/*
ReservePlaybackArea
	ָ��ϵͳ�ط�ʱ��Ҫ���ǵ����򣬿��ṩ�������2���Ƿ��㹻����
*/

/*
DisplayPlaybackArea
	�ڻط���ʾ��������ʾ������λͼ
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

// ����ʽ������
int StartFormatProgress (void *private_data);
int QueryFormatProgress (void *private_data);

// �������ûָ�����
int StartRestoreProgress (void *private_data);
int QueryRestoreProgress (void *private_data);
int EndDefaultParamProgress (void *private_data);

// ϵͳ��������
int StartUpdateProgress (void *private_data);
int QueryUpdateProgress (void *private_data);

// ���ϵͳ�����ļ�
DWORD XM_GetUpdateFileStatus (void);

APPMENUID XM_GetUpdateFileVersion (void);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_APP_MENUDATA_H_

