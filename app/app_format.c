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
static AP_PROCESSINFO			FormatProcessInfo;

// 格式化过程视窗的系统事件处理回调函数
static void format_view_system_event_handler (XMMSG *msg, int process_state)
{
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_DETECT:
		case SYSTEM_EVENT_CARD_UNPLUG:					// SD卡拔出事件
		case SYSTEM_EVENT_CARD_INSERT:
		case SYSTEM_EVENT_CARD_INVALID:
			if(	process_state == PROCESS_STATE_INITIAL
				||	process_state == PROCESS_STATE_CONFIRM)
			{
				XM_BreakSystemEventDefaultProcess (msg);
				XM_PullWindow (0);

				XM_lock ();
				XM_KeyEventProc ((unsigned short)VK_AP_SYSTEM_EVENT, (unsigned short)(msg->wp));
				XM_unlock ();

			}
			break;

		case SYSTEM_EVENT_CARD_FS_ERROR:	//
		case SYSTEM_EVENT_CARD_VERIFY_ERROR:		// 读写校验检查错误(卡损坏)
			XM_BreakSystemEventDefaultProcess (msg);
			break;

		case SYSTEM_EVENT_MAIN_BATTERY:	// 主电池电压变化事件
		{
			XM_BreakSystemEventDefaultProcess (msg);
			// 读取主电池的当前状态
			if(	process_state == PROCESS_STATE_INITIAL
				||	process_state == PROCESS_STATE_CONFIRM)
			{
				if(XM_GetFmlDeviceCap (DEVCAP_MAINBATTERYVOLTAGE) == DEVCAP_BATTERYVOLTAGE_WORST)
				{
					XM_PullWindow (0);
					// 电池电量极低
					AP_PostSystemEvent (SYSTEM_EVENT_MAIN_BATTERY);
				}
			}
			break;
		}

		default:
			break;
	}
}

// 打开格式化视窗
VOID AP_OpenFormatView (XMBOOL bPushView)
{
	// 检查卡状态 (卡拔出、卡写保护禁止执行操作)
	DWORD CardStatus = XM_GetFmlDeviceCap (DEVCAP_SDCARDSTATE);
	
	if (CardStatus == DEVCAP_SDCARDSTATE_UNPLUG)
	{
		// 卡已拔出
		AP_PostSystemEvent (SYSTEM_EVENT_CARD_UNPLUG);
		return;
	}
	
	/*
	else if (CardStatus == DEVCAP_SDCARDSTATE_INVALID)
	{
		// 卡无效事件
		AP_PostSystemEvent (SYSTEM_EVENT_CARD_INVALID);
		return;
	}
	*/
	
	// 检查主电池电压
	//if(XM_GetFmlDeviceCap (DEVCAP_MAINBATTERYVOLTAGE) == DEVCAP_BATTERYVOLTAGE_WORST)
	//{
		// 电池电量极低
	//	AP_PostSystemEvent (SYSTEM_EVENT_MAIN_BATTERY);
	//	return;
	//}
	//else
	{	
		// 格式化检查Pass
		memset (&FormatProcessInfo, 0, sizeof(FormatProcessInfo));
		FormatProcessInfo.type = AP_PROCESS_VIEW_FORMAT;
		FormatProcessInfo.Title = AP_ID_ALERT_SD_FORMAT_PIC;
		FormatProcessInfo.DispItem[0] = AP_ID_SD_FORMAT_INITIAL;
		FormatProcessInfo.DispItem[1] = AP_ID_SD_FORMATTING;
		FormatProcessInfo.DispItem[2] = AP_ID_SD_FORMATTING;
		FormatProcessInfo.DispItem[3] = AP_ID_SD_FORMATSUCCESS;
		FormatProcessInfo.DispItem[4] = AP_ID_CARD_INFO_FORMATFAILURE;
		FormatProcessInfo.nDispItemNum = 5;
		FormatProcessInfo.nMaxProgress = 10;
		FormatProcessInfo.fpStartProcess = StartFormatProgress;
		FormatProcessInfo.fpQueryProgress = QueryFormatProgress;
		FormatProcessInfo.fpSystemEventCb = format_view_system_event_handler;
		
		if(bPushView)
			XM_PushWindowEx (XMHWND_HANDLE(ProcessView), (DWORD)(&FormatProcessInfo));
		else
			XM_JumpWindowEx (XMHWND_HANDLE(ProcessView), (DWORD)(&FormatProcessInfo), XM_JUMP_POPDEFAULT);
	}	
}


