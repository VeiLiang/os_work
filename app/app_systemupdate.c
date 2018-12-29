//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_systemupdate.c
//	  ϵͳ����
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"
#include "app_processview.h"
#include "systemapi.h"

#ifdef _WINDOWS
static const char update_file[] = "update.bin";				// ��ǰ����Ŀ¼
#else
static const char update_file[] = "mmc:0:\\update.bin";	// SD��0��Ŀ¼��
#endif

static AP_PROCESSINFO SystemUpdateProcessInfo;

// ϵͳ���������Ӵ���ϵͳ�¼�����ص�����
static void system_update_view_system_event_handler (XMMSG *msg, int process_state)
{
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:					// SD���γ��¼�
		case SYSTEM_EVENT_CARD_INVALID:					// ����Ч
		case SYSTEM_EVENT_CARD_INSERT:					// ������
		case SYSTEM_EVENT_CARD_FS_ERROR:				// ���ļ�ϵͳ����
			XM_printf(">>>>>>>>>>system_update_view_system_event_handler, 1...\r\n");
			if(	process_state == PROCESS_STATE_INITIAL||process_state == PROCESS_STATE_CONFIRM)
			{
				// ��һ��ȷ�ϼ��ڶ���ȷ�Ϲ�����, ���Ͽ��¼�����"ϵͳ��������"ֱ���˳�
				XM_BreakSystemEventDefaultProcess (msg);		// ��ֹ��ϵͳ�¼��ļ�������
				XM_PullWindow (0);		// ����"ϵͳ����"�Ӵ�

				// ��ϵͳ�¼�����ѹ��, �ȴ�ϵͳ����
				XM_lock ();
				XM_KeyEventProc ((unsigned short)0xF0, (unsigned short)(msg->wp));
				XM_unlock ();
			}
			else if(process_state == PROCESS_STATE_WORKING || process_state == PROCESS_STATE_SUCCESS)
			{
				XM_BreakSystemEventDefaultProcess (msg);		// ��ֹ��ϵͳ�¼��ļ�������, �����˸��¼�
			}
			break;

		case SYSTEM_EVENT_CARD_VERIFY_ERROR:
		case SYSTEM_EVENT_CARD_DISK_FULL:	
		case SYSTEM_EVENT_VIDEOITEM_RECYCLE_CONSUMED:
		case SYSTEM_EVENT_VIDEOITEM_LOW_SPACE:
			// ��ʾ�������ȴ��û�����
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

					XM_lock ();
					XM_KeyEventProc ((unsigned short)VK_AP_SYSTEM_EVENT, (unsigned short)(msg->wp));
					XM_unlock ();
				}
			}
			break;
		}
		
		case SYSTEM_EVENT_SYSTEM_UPDATE_FILE_CHECKED:
		{
			// �ظ��յ���ϵͳ������Ϣ, �򵥶���
			XM_BreakSystemEventDefaultProcess (msg);		// ��ֹ��ϵͳ�¼��ļ�������
			break;
		}

		default:
			break;
	}
}

// ��ť����Ļص�����
static void menu_cb (UINT menu)
{
	// �����ɹ���ǿ������
	XM_ShutDownSystem(SDS_REBOOT);
}


// ϵͳ���������ص�����
static int end_update_cb (void * state)
{
	if((int)state == 0)
	{
		// �����ɹ�
		#if 0
		DWORD dwNormalButton[1] = {AP_ID_CARD_INFO_SYSUPDATE_REBOOT};

		XM_OpenOkCancelView (AP_ID_CARD_INFO_SYSUPDATE_INFO_FINISH,	// ��Ϣ�ı���ԴID
									0,												// ͼƬ��Ϣ��ԴID, ��0ֵ��ָ����ʾͼƬ��Ϣ����ԴID
									1,												// ��ť������1����2
									dwNormalButton,							//	��ť��ԴID������
									NULL,											//	��ť������ԴID, ����Ϊ��.
									0x80404040,									// ����ɫ, 0x80404040(ARGB32) ��ʾ��͸��(Alpga=0x80)��ǳ��ɫ(RGB=0x404040)
									5.0f,		// �Զ��ر�ʱ��
									1.0f,			// Alpha͸����, 1.0 ȫ����    0.0 ȫ͸��
													//		Alpha͸���� * 	����ɫ��Alphaֵ ��ʾ ���յ�Alpha����
									menu_cb,		// ��ť����Ļص�����������Ϊ XM_COMMAND_OK ���� XM_COMMAND_CANCEL
									XM_VIEW_ALIGN_CENTRE,
									XM_OKCANCEL_OPTION_ENABLE_POPTOPVIEW|XM_OKCANCEL_OPTION_SYSTEM_MODEL
									);
		#endif
		//�����ɹ���ǿ������
		XM_ShutDownSystem(SDS_REBOOT);
	}
	else
	{
		//����ʧ��
		//AP_OpenMessageViewEx(AP_ID_SYSTEMSETTING_UPDATE_TITLE, AP_ID_CARD_INFO_SYSUPDATE_FAILURE, (DWORD)-1, 1);
	}
	return 0;
}

void app_start_update (int push_view)
{
	DWORD UpdateFileStatus;
	// ���ϵͳ�����ļ��Ƿ����

	// ���ϵͳ�����ļ��Ƿ�Ϊ�Ϸ��������ļ�(�ļ���ʶ��CRC���)
	
	// ���ϵͳ�����ļ��Ƿ��ǵ�ǰ����ʹ�� (��鳧��ID)

	// ���ϵͳ�汾
	//UpdateFileStatus = XM_GetUpdateFileStatus();
	
	//if (UpdateFileStatus == 0)
	//{
	//	AP_OpenMessageView (AP_ID_SYSTEMSETTING_UPDATE_FILE_ABNORMAL);
	//}
	//else
	{
		memset (&SystemUpdateProcessInfo, 0, sizeof(SystemUpdateProcessInfo));
		SystemUpdateProcessInfo.type = AP_PROCESS_VIEW_SYSTEMUPDATE;
		SystemUpdateProcessInfo.Title = AP_ID_SYSTEMSETTING_UPDATE_TITLE;
		SystemUpdateProcessInfo.DispItem[0] = AP_ID_CARD_INFO_SYSUPDATE_INITIAL;
		SystemUpdateProcessInfo.DispItem[1] = AP_ID_CARD_INFO_SYSUPDATE_CONFIRM;
		SystemUpdateProcessInfo.DispItem[2] = AP_ID_CARD_INFO_SYSUPDATE_WORKING;
		SystemUpdateProcessInfo.DispItem[3] = AP_ID_CARD_INFO_SYSUPDATE_SUCCESS;
		SystemUpdateProcessInfo.DispItem[4] = AP_ID_CARD_INFO_SYSUPDATE_FAILURE;
		SystemUpdateProcessInfo.nDispItemNum = 5;
		SystemUpdateProcessInfo.nMaxProgress = 10;
		SystemUpdateProcessInfo.fpStartProcess = StartUpdateProgress;
		SystemUpdateProcessInfo.fpEndProcess = end_update_cb;
		SystemUpdateProcessInfo.fpQueryProgress = QueryUpdateProgress;
		SystemUpdateProcessInfo.fpSystemEventCb = system_update_view_system_event_handler;
		
		// ѹ���ջ���Ӵ�(�ļ�У����ʾ�Ӵ�)��ջ��ѹ��
		if(push_view)
			XM_PushWindowEx (XMHWND_HANDLE(ProcessView), (DWORD)(&SystemUpdateProcessInfo));
		else
			XM_JumpWindowEx (XMHWND_HANDLE(ProcessView), (DWORD)(&SystemUpdateProcessInfo), XM_JUMP_POPTOPVIEW);
	}
}

// �ļ�ϵͳ��дУ�����ص�����
static void fs_verify_error_alert_cb (void *lpUserData, unsigned int uKeyPressed)
{
	app_start_update (0);
}

// ��ϵͳ�����Ӵ�
// push_view_or_pull_view	
//		1	push view, ��ϵͳ�����Ӵ�ѹ�뵽��ǰ�Ӵ�ջ��ջ��
//		0	pull view, ����ǰ�Ӵ�ջ�������Ӵ�����, Ȼ���ٽ�ϵͳ�����Ӵ�ѹ�뵽�Ӵ�ջ��ջ��
VOID AP_OpenSystemUpdateView (unsigned int push_view_or_pull_view)
{
	
	// ��鿨״̬ (���γ�����д������ִֹ�в���)
	DWORD CardStatus = XM_GetFmlDeviceCap (DEVCAP_SDCARDSTATE);

	if (CardStatus == DEVCAP_SDCARDSTATE_UNPLUG)
	{
		// ���Ѱγ�
		AP_PostSystemEvent (SYSTEM_EVENT_CARD_UNPLUG);
		return;
	}
	else if (CardStatus == DEVCAP_SDCARDSTATE_INVALID)
	{
		// ����Ч�¼�
		AP_PostSystemEvent (SYSTEM_EVENT_CARD_INVALID);
		return;
	}
	else if(CardStatus == DEVCAP_SDCARDSTATE_FS_ERROR)
	{
		// ���ļ�ϵͳ����
		XM_OpenAlertView (	
			AP_ID_CARD_MESSAGE_CARD_FSERROR,	// ��Ϣ�ı���ԴID
			0,			// ͼƬ��Ϣ��ԴID
			0,
			0,												// ��ť������ԴID
			0,												// ��ť����������ԴID
			APP_ALERT_BKGCOLOR,
			10.0,											// 5�����Զ��ر�
			APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
			NULL,											// �����ص�����
			NULL,
			XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
			//XM_VIEW_ALIGN_CENTRE,
			0												// ALERTVIEW��ͼ�Ŀ���ѡ��
			);
		return;
	}

	// �������ص�ѹ
	#if 0
	if(XM_GetFmlDeviceCap (DEVCAP_MAINBATTERYVOLTAGE) == DEVCAP_BATTERYVOLTAGE_WORST)
	{
		// ��ص�������
		AP_PostSystemEvent (SYSTEM_EVENT_MAIN_BATTERY);
		return;
	}
    #endif
	
	app_start_update (push_view_or_pull_view);

}