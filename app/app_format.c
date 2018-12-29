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
static AP_PROCESSINFO			FormatProcessInfo;

// ��ʽ�������Ӵ���ϵͳ�¼�����ص�����
static void format_view_system_event_handler (XMMSG *msg, int process_state)
{
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_DETECT:
		case SYSTEM_EVENT_CARD_UNPLUG:					// SD���γ��¼�
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
		case SYSTEM_EVENT_CARD_VERIFY_ERROR:		// ��дУ�������(����)
			XM_BreakSystemEventDefaultProcess (msg);
			break;

		case SYSTEM_EVENT_MAIN_BATTERY:	// ����ص�ѹ�仯�¼�
		{
			XM_BreakSystemEventDefaultProcess (msg);
			// ��ȡ����صĵ�ǰ״̬
			if(	process_state == PROCESS_STATE_INITIAL
				||	process_state == PROCESS_STATE_CONFIRM)
			{
				if(XM_GetFmlDeviceCap (DEVCAP_MAINBATTERYVOLTAGE) == DEVCAP_BATTERYVOLTAGE_WORST)
				{
					XM_PullWindow (0);
					// ��ص�������
					AP_PostSystemEvent (SYSTEM_EVENT_MAIN_BATTERY);
				}
			}
			break;
		}

		default:
			break;
	}
}

// �򿪸�ʽ���Ӵ�
VOID AP_OpenFormatView (XMBOOL bPushView)
{
	// ��鿨״̬ (���γ�����д������ִֹ�в���)
	DWORD CardStatus = XM_GetFmlDeviceCap (DEVCAP_SDCARDSTATE);
	
	if (CardStatus == DEVCAP_SDCARDSTATE_UNPLUG)
	{
		// ���Ѱγ�
		AP_PostSystemEvent (SYSTEM_EVENT_CARD_UNPLUG);
		return;
	}
	
	/*
	else if (CardStatus == DEVCAP_SDCARDSTATE_INVALID)
	{
		// ����Ч�¼�
		AP_PostSystemEvent (SYSTEM_EVENT_CARD_INVALID);
		return;
	}
	*/
	
	// �������ص�ѹ
	//if(XM_GetFmlDeviceCap (DEVCAP_MAINBATTERYVOLTAGE) == DEVCAP_BATTERYVOLTAGE_WORST)
	//{
		// ��ص�������
	//	AP_PostSystemEvent (SYSTEM_EVENT_MAIN_BATTERY);
	//	return;
	//}
	//else
	{	
		// ��ʽ�����Pass
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


