//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	SongXing
//
//	File name: InitSetting.c
//	第一次开机时，进行相关设置：时间、副摄像头
//
//	Revision history
//
//		2012.09.01	SongXing Initial version
//
//****************************************************************************
/*
*/

#include "app.h"
#include "app_voice.h"
#include "app_menuid.h"

#include "system_check.h"
#include "systemapi.h"

#define	XM_USER_SYSINITSETTING		(XM_USER+0)

USHORT	SettingTurn = 0;

VOID InitSettingOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
		SettingTurn = 0;
	}
	else
	{
		SettingTurn++;	
	}

	XM_PostMessage (XM_USER_SYSINITSETTING, 0, 0);
}

VOID InitSettingOnSetting (XMMSG *msg)
{
	if (SettingTurn == 0)
	{
		XM_PushWindowEx (XMHWND_HANDLE(DateTimeSetting), APP_DATETIMESETTING_CUSTOM_FORCED);
	}

	/*
	if (SettingTurn == 1)
	{
		XM_PushWindow (XMHWND_HANDLE(CCDManageView));
	}*/

	if (SettingTurn >= 1)
	{
		XM_PullWindow (0);
	}
}

// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (InitSettingView)
	XM_ON_MESSAGE (XM_ENTER, InitSettingOnEnter)
	XM_ON_MESSAGE (XM_USER_SYSINITSETTING, InitSettingOnSetting)
XM_MESSAGE_MAP_END

// 初始设置视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, InitSettingView, 1, 0, 0, 255, HWND_VIEW)


