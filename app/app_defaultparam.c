//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	SongXing
//
//	File name: app_process.c
//	��ʽ��SD ��
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



// ����Ϊ��̬������̬����
static AP_PROCESSINFO DefaultParamProcessInfo;

// �ָ�ϵͳ���õ�ϵͳ�¼��ص�����
static void restore_setting_view_system_event_handler(XMMSG *msg, int process_state)
{
	// �������κ��¼�
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:					// SD���γ��¼�
		case SYSTEM_EVENT_CARD_INSERT:
		case SYSTEM_EVENT_CARD_FS_ERROR:	//
		case SYSTEM_EVENT_CARD_VERIFY_ERROR:
			break;

		default:
			break;
	}
}

// �򿪸�ʽ���Ӵ�
VOID AP_OpenDefaultParamView (XMBOOL bPushView)
{
	//�ָ�ϵͳ����
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


