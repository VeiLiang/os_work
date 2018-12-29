//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	SongXing
//
//	File name: app_process.c
//	格式化SD 卡
//
//	Revision history
//
//		2012.09.01	SongXing Initial version
//
//****************************************************************************
/*
*/
#include "app.h"
#include "app_menuid.h"
#include "systemapi.h"
#include "app_processview.h"



// 必须为静态变量或动态分配
static AP_PROCESSINFO DefaultParamProcessInfo;

// 恢复系统设置的系统事件回调函数
static void restore_setting_view_system_event_handler(XMMSG *msg, int process_state)
{
	// 不处理任何事件
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:					// SD卡拔出事件
		case SYSTEM_EVENT_CARD_INSERT:
		case SYSTEM_EVENT_CARD_FS_ERROR:	//
		case SYSTEM_EVENT_CARD_VERIFY_ERROR:
			break;

		default:
			break;
	}
}

// 打开格式化视窗
VOID AP_OpenDefaultParamView (XMBOOL bPushView)
{
	//恢复系统参数
	memset (&DefaultParamProcessInfo, 0, sizeof(DefaultParamProcessInfo));
	DefaultParamProcessInfo.type = AP_PROCESS_VIEW_RESTORESETTING;
	DefaultParamProcessInfo.Title = AP_ID_RESET_TITLE;
	DefaultParamProcessInfo.DispItem[0] = AP_ID_CARD_INFO_RESTORE_INITIAL;
	DefaultParamProcessInfo.DispItem[1] = AP_ID_CARD_INFO_RESTORE_CONFIRM;
	DefaultParamProcessInfo.DispItem[2] = AP_ID_CARD_INFO_RESTORE_WORKING;
	DefaultParamProcessInfo.DispItem[3] = AP_ID_CARD_INFO_RESTORE_SUCCESS;
	DefaultParamProcessInfo.DispItem[4] = AP_ID_CARD_INFO_RESTORE_FAILURE;
	DefaultParamProcessInfo.nDispItemNum = 5;
	DefaultParamProcessInfo.nMaxProgress = 1;
	DefaultParamProcessInfo.fpStartProcess = StartRestoreProgress;
	DefaultParamProcessInfo.fpQueryProgress = QueryRestoreProgress;
	DefaultParamProcessInfo.fpSystemEventCb = restore_setting_view_system_event_handler;
	DefaultParamProcessInfo.fpEndProcess = EndDefaultParamProgress;

	if(bPushView)
		XM_PushWindowEx(XMHWND_HANDLE(ProcessView), (DWORD)(&DefaultParamProcessInfo));
	else
		XM_JumpWindowEx(XMHWND_HANDLE(ProcessView), (DWORD)(&DefaultParamProcessInfo), XM_JUMP_POPDEFAULT);
}


