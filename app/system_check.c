//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: system_check.c
//	  ϵͳ�Լ�
//
//	Revision history
//
//		2012.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "system_check.h"
#include "app_videolist.h"


int APSYS_CardChecking (void)
{
	// ����Ӳ���ӿ�ֱ�ӻ�ȡ
	DWORD state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
	if(state != DEVCAP_SDCARDSTATE_INSERT)
		return APP_SYSTEMCHECK_CARD_NOCARD;
	return APP_SYSTEMCHECK_SUCCESS;
}



int APSYS_TimeSettingChecking (void)
{
	DWORD state = XM_GetFmlDeviceCap (DEVCAP_TIMESETTING);
	if(state == 0)
		return APP_SYSTEMCHECK_TIMESETTING_INVALID;
	else
		return APP_SYSTEMCHECK_SUCCESS;
}

int APSYS_BackupBatteryChecking (void)
{
	DWORD state = XM_GetFmlDeviceCap (DEVCAP_BACKUPBATTERYVOLTAGE);
	if(state == 0)
		return APP_SYSTEMCHECK_BACKUPBATTERY_LOWVOLTAGE;
	else
		return APP_SYSTEMCHECK_SUCCESS;
}