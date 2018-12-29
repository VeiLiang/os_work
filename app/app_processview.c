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
#include "rtos.h"		// ������غ���
#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"
#include "systemapi.h"
#include "app_processview.h"
#include "xm_h264_codec.h"
#include <xm_gif.h>
#include <xm_semaphore.h>
#include "xm_autotest.h"
#include "fs.h"
#include "xm_proj_define.h"
#include "xm_flash_space_define.h"


#define	OSD_AUTO_HIDE_TIMEOUT						2000	


#define	XMSYS_PROCESS_TASK_PRIORITY					200	
#define	XMSYS_PROCESS_TASK_STACK_SIZE				0x1000


static OS_TASK TCB_ProcessTask;
static OS_STACKPTR int StackProcessTask[XMSYS_PROCESS_TASK_STACK_SIZE/4];          /* Task stacks */


#define READY_TO_PROCESS	0
#define PROCESSING			1
#define END_OF_PROCESS		2

typedef struct tagPROCESSVIEWDATA {
	AP_PROCESSINFO	ProcessInfo;		// ���̿��Ʋ���

	BYTE 			curitem;//ȷ��,ȡ��index
	BYTE			enablekey;

	void *			process_sync_semaphore;			// ͬ���ź�������
	int				process_state;					// ״̬
	int				result;							// �����ⲿ���̲����ķ���ֵ
} PROCESSVIEWDATA;

static int process_task_exit (PROCESSVIEWDATA *ProcessViewData);


// ��Ƶ����������
void XMSYS_ProcessTask (PROCESSVIEWDATA *ProcessViewData) 
{	
	// Video�����ʼ�����
	//int ret;
	XM_printf(">>>>>>>>>>>>>>>>>XMSYS_ProcessTask start................\r\n");
	ProcessViewData->process_state = PROCESS_STATE_WORKING;
	XM_SignalSemaphore(ProcessViewData->process_sync_semaphore);
	
	// ��ֹ�����
	//ret = XMSYS_H264CodecStop ();
	//if(ret == 0)
	{
		if(ProcessViewData->ProcessInfo.type == AP_PROCESS_VIEW_FORMAT)
		{
			// ��ʽ��
			//xm_close_volume_service ("mmc:0");
			OS_Delay (500);
			
			// ִ�и�ʽ������
			ProcessViewData->result = (*ProcessViewData->ProcessInfo.fpStartProcess)(ProcessViewData->ProcessInfo.lpPrivateData);
			
			if(ProcessViewData->result == 0)
			{
				// ��ʽ���ɹ�
				FS_CACHE_Clean ("");
				if(FS_Mount ("mmc:0:") >= 0)
				{
					xm_open_volume_service("mmc:0:", 0);
				}
				else
				{
					XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_INVALID);
				}
			}
			else
			{
				XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_INVALID);
			}
		}
		else if(ProcessViewData->ProcessInfo.type == AP_PROCESS_VIEW_RESTORESETTING)
		{
			// �ָ���������
			ProcessViewData->result = (*ProcessViewData->ProcessInfo.fpStartProcess)(ProcessViewData->ProcessInfo.lpPrivateData);
			// ��������
			AP_SetMenuItem(APPMENUITEM_LCD, AppMenuData.lcd);
				
			// ����¼��ķֱ���/֡��
			XMSYS_H264CodecSetVideoFormat(AppMenuData.video_resolution);
		}
		else if(ProcessViewData->ProcessInfo.type == AP_PROCESS_VIEW_SYSTEMUPDATE)
		{// ϵͳ����
			ProcessViewData->result = (*ProcessViewData->ProcessInfo.fpStartProcess)(ProcessViewData->ProcessInfo.lpPrivateData);
		}
	}

	if(ProcessViewData->result == 0)
		ProcessViewData->process_state = PROCESS_STATE_SUCCESS;
	else
		ProcessViewData->process_state = PROCESS_STATE_FAILURE;
}



void  XMSYS_ProcessTask_rtos (void *user_data)
{
	PROCESSVIEWDATA * ProcessViewData = (PROCESSVIEWDATA *)user_data;
	
	XMSYS_ProcessTask((PROCESSVIEWDATA *)user_data);

	XM_printf(">>>>>>>>>>>>>>>>>>XMSYS_ProcessTask End..............\r\n");

	XM_printf(">>>>>>>>>>>>XMSYS_ProcessTask_rtos, ProcessViewData->process_state:%d\r\n", ProcessViewData->process_state);
	XM_InvalidateWindow ();
	XM_UpdateWindow ();	
	// ��ֹ��ǰ����
	OS_Terminate (NULL);
}


static int process_task_init (PROCESSVIEWDATA *ProcessViewData)
{
	if(ProcessViewData == NULL)
		return -1;

	// ��������������
	//if(ProcessViewData->process_state != PROCESS_STATE_INITIAL && ProcessViewData->process_state != PROCESS_STATE_CONFIRM)
	//	return 0;

    if(ProcessViewData->process_sync_semaphore != NULL)
		return -1;
	
	ProcessViewData->process_sync_semaphore = XM_CreateSemaphore ("process_sync_semaphore", 0);
	if(!ProcessViewData->process_sync_semaphore)
	{
		return -1;
	}

	// ��ǿ����¼�Ⲣ�ر���Ƶ����
	//APPMarkCardChecking(1);
	
	OS_CREATETASK_EX(&TCB_ProcessTask, "ProcessTask", 
						XMSYS_ProcessTask_rtos, 
						XMSYS_PROCESS_TASK_PRIORITY, 
						StackProcessTask,
						ProcessViewData
					);

	// �ȴ���������
	if(XM_WaitSemaphore(ProcessViewData->process_sync_semaphore) == 0)
	{
		XM_printf(">>>>>>>>>>>>>>>.process_task_init NG, wait semaphore \"process_sync_semaphore\" failed\n");

		process_task_exit (ProcessViewData);
		return -1;
	}

	return 0;
}

static int process_task_exit (PROCESSVIEWDATA *ProcessViewData)
{
	if(ProcessViewData == NULL)
		return -1;

	if(ProcessViewData->process_sync_semaphore)
	{
		// �ر��źŵ�
		XM_CloseSemaphore (ProcessViewData->process_sync_semaphore);
		// ɾ���źŵ�
		XM_DeleteSemaphore (ProcessViewData->process_sync_semaphore);
		ProcessViewData->process_sync_semaphore = NULL;
	}

	return 0;
}


VOID ProcessViewOnEnter(XMMSG *msg)
{
	PROCESSVIEWDATA *ProcessViewData;
	AP_PROCESSINFO* lpProcessInfo;

	if(msg->wp == 0)
	{
		// ����δ��������һ�ν���
		XMRECT rect;
		lpProcessInfo = (AP_PROCESSINFO *)msg->lp;

		XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>ProcessViewOnEnter, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

		if(lpProcessInfo->type==AP_PROCESS_VIEW_SYSTEMUPDATE)
		{
			// ��ǿ����¼�Ⲣ�ر���Ƶ����
			//APPMarkCardChecking (1);
		}

		
		// �������
		if(lpProcessInfo == NULL)
			return;
		if(lpProcessInfo->fpStartProcess == NULL)
			return;

		// ����˽�����ݾ��
		ProcessViewData = XM_calloc (sizeof(PROCESSVIEWDATA));
		if(ProcessViewData == NULL)
		{
			XM_printf ("ProcessViewData XM_calloc failed\n");
			// ʧ�ܷ��ص������ߴ���
			XM_PullWindow (0);
			return;
		}

		ProcessViewData->process_state = PROCESS_STATE_INITIAL;
		ProcessViewData->curitem = 0;
		ProcessViewData->enablekey = 1;
		
		memcpy(&ProcessViewData->ProcessInfo, lpProcessInfo, sizeof(ProcessViewData->ProcessInfo));
		
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData(XMHWND_HANDLE(ProcessView), ProcessViewData);
		// �����Ӵ���alphaֵ
		XM_SetWindowAlpha(XMHWND_HANDLE(ProcessView), (unsigned char)(0));
		XM_SetWindowPos(XMHWND_HANDLE(ProcessView), 287, 169, 441, 258);
	}
	else
	{
		ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ProcessView));

		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf(">>>>>>>>>>>>>>ProcessView Pull......................\n");
	}
	
	// ������ʱ�������ڲ˵�������
	// ����x��Ķ�ʱ��
	//XM_SetTimer(XMTIMER_PROCESSVIEW, OSD_AUTO_HIDE_TIMEOUT);
}


VOID ProcessViewOnLeave (XMMSG *msg)
{
	// ɾ����ʱ��
	XM_KillTimer (XMTIMER_PROCESSVIEW);
	
	PROCESSVIEWDATA *ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ProcessView));
	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		// �ͷ����з������Դ
		if(ProcessViewData)
		{
			// �ͷ�˽�����ݾ��
			XM_free (ProcessViewData);
		}
		XM_printf ("ProcessView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("ProcessView Push\n");
	}
}

static void draw_progress_icon (void)
{
	XMRECT rect;
	XMRECT rc_pic;
	int offset;
	PROCESSVIEWDATA *ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ProcessView));

	if(ProcessViewData == NULL)
		return;

	// ��鶨ʱ���Ƿ��ѿ���
	//if(!is_timer_running(ProcessViewData))
	//	return;

	if(ProcessViewData->process_state == PROCESS_STATE_INITIAL)
		return;

	XM_printf(">>>>>>>>>>>>>>draw_progress_icon, ProcessViewData->process_state:%d\r\n", ProcessViewData->process_state);
	if(	(ProcessViewData->process_state == PROCESS_STATE_SUCCESS) || (ProcessViewData->process_state == PROCESS_STATE_FAILURE) )
	{//ִ����SD����ʽ����,�ͷ�����
		XM_printf(">>>>>>>>>>>>>>All Process End.............\r\n");

		process_task_exit(ProcessViewData);//ɾ���ź���

		// ����Ƿ����"���̽����������". ����, ִ�и�"���̽����������"
		if(ProcessViewData->ProcessInfo.fpEndProcess)
		{
			XM_printf(">>>>>>>>>>>>>>>>process end process.......\r\n");
			(*ProcessViewData->ProcessInfo.fpEndProcess) ((void *)ProcessViewData->result);
		}
		else
		{
			XM_printf(">>>>>>>>>>>>>>>>no end process.......\r\n");
		}
		//XM_InvalidateWindow ();
		//XM_UpdateWindow ();
		return;
	}
	else
	{
		
	}
}

#define MSG_BODY_WIDTH  400

VOID ProcessViewOnPaint (XMMSG *msg)
{
	char text[64];
	char String[32];
	XMSIZE size;
	XMRECT rc, rect;
	unsigned int old_alpha;
	HANDLE hwnd = XMHWND_HANDLE(ProcessView);

	PROCESSVIEWDATA *ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(ProcessViewData == NULL)
		return;

	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>ProcessViewOnPaint, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);
	
    XM_GetWindowRect(XMHWND_HANDLE(Desktop),&rc);
	XM_FillRect(hwnd, rc.left, rc.top, rc.right, rc.bottom, 0x00000000);
	
	// ��ȡ�Ӵ�λ����Ϣ
	XM_GetWindowRect(XMHWND_HANDLE(ProcessView), &rc);
	XM_FillRect(XMHWND_HANDLE(ProcessView), rc.left, rc.top, rc.right, rc.bottom, 0xB0404040);

	old_alpha = XM_GetWindowAlpha(hwnd);
	XM_SetWindowAlpha (hwnd, 255);

	//����ͼƬ
	rect = rc;
	AP_RomImageDrawByMenuID(AP_ID_ALERT_BG_PIC, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	rect = rc;
	rect.left = rect.left + 174; rect.top = rect.top + 2;
	AP_RomImageDrawByMenuID(ProcessViewData->ProcessInfo.Title, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	//����״̬�仯,�л���ͬ������ʾ;
	rect = rc;
	if(ProcessViewData->process_state==PROCESS_STATE_INITIAL)
	{
		//rect.left = rect.left + 113;
		rect.left = rect.left + (rect.right - rect.left - MSG_BODY_WIDTH)/2;
		rect.top = rect.top + 84;
	}
	else if(ProcessViewData->process_state==PROCESS_STATE_WORKING)
	{
		//rect.left = rect.left + 142;
		rect.left = rect.left + (rect.right - rect.left - MSG_BODY_WIDTH)/2;
		rect.top = rect.top + 84;
	}
	else if(ProcessViewData->process_state==PROCESS_STATE_SUCCESS)
	{
		//rect.left = rect.left + 162;
		rect.left = rect.left + (rect.right - rect.left - MSG_BODY_WIDTH)/2;
		rect.top = rect.top + 84;
		rect.right = rect.left + MSG_BODY_WIDTH;
	}
	AP_RomImageDrawByMenuID(ProcessViewData->ProcessInfo.DispItem[ProcessViewData->process_state], hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	//��ťѡ���
	rect = rc;
	if(ProcessViewData->curitem==0)
	{//ȡ��
		rect.left = rect.left + 96; rect.top = rect.top + 193;
	}
	else
	{//ȷ��
		rect.left = rect.left + 296; rect.top = rect.top + 193;
	}
	AP_RomImageDrawByMenuID(AP_ID_SD_BUTTON, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	//ȡ����ť
	rect = rc;
	rect.left = rect.left + 100; rect.top = rect.top + 194;
	AP_RomImageDrawByMenuID(AP_ID_SD_CANCEL, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	//ȷ����ť
	rect = rc;
	rect.left = rect.left + 300; rect.top = rect.top + 194;
	AP_RomImageDrawByMenuID(AP_ID_SD_OK, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	draw_progress_icon();

	if(ProcessViewData->ProcessInfo.type==AP_PROCESS_VIEW_SYSTEMUPDATE)
	{
		rect = rc;
		rect.left = rect.left + 20; rect.top = rect.top + 150;	

		int setp = (*ProcessViewData->ProcessInfo.fpQueryProgress)(ProcessViewData->ProcessInfo.lpPrivateData);
		XM_printf(">>>>>>>>>>>setp:%d\r\n", setp);
	    sprintf(String,"%d", setp);
	    AP_TextGetStringSize(String,sizeof(String),&size);
	    AP_TextOutDataTimeString(hwnd, rect.left, rect.top, String, strlen(String));
	}
	XM_SetWindowAlpha (hwnd, (unsigned char)old_alpha);
}

VOID ProcessViewOnKeyDown (XMMSG *msg)
{
	PROCESSVIEWDATA *ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ProcessView));
	if(ProcessViewData == NULL)
		return;

	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>ProcessViewOnKeyDown, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

	//������
	XM_Beep(XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
	    case REMOTE_KEY_LEFT:
	    case REMOTE_KEY_RIGHT:
		case VK_AP_DOWN:
		case VK_AP_UP:
			if(ProcessViewData->enablekey==1)
			{
				if(ProcessViewData->curitem==0)
				{
					ProcessViewData->curitem = 1;
				}
				else
				{
					ProcessViewData->curitem = 0;
				}
				XM_InvalidateWindow ();
				XM_UpdateWindow ();	
			}
			break;
        case REMOTE_KEY_DOWN:
		case VK_AP_SWITCH:
			if(ProcessViewData->curitem==1)
			{
				ProcessViewData->enablekey = 0;
				//�����߳�
				if(process_task_init(ProcessViewData) < 0)
				{
					ProcessViewData->process_state = PROCESS_STATE_WORKING;
				}

				XM_printf(">>>>>>>>>>>>>qqq ProcessViewData->process_state:%d\r\n", ProcessViewData->process_state);
				if(ProcessViewData->ProcessInfo.type==AP_PROCESS_VIEW_SYSTEMUPDATE)
				{
					XM_SetTimer(XMTIMER_PROCESSVIEW, 500);//��Ҫˢ������,�ӿ�ʱ��
				}
				else
				{
					XM_SetTimer(XMTIMER_PROCESSVIEW, OSD_AUTO_HIDE_TIMEOUT);
				}
				XM_InvalidateWindow();
				XM_UpdateWindow();				
			}
			else if(ProcessViewData->curitem==0)
			{
				XM_PullWindow(0);
			}
			break;
			
		default:
			break;
	}

}

VOID ProcessViewOnKeyUp (XMMSG *msg)
{
	PROCESSVIEWDATA *ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ProcessView));
	if(ProcessViewData == NULL)
		return;
}

VOID ProcessViewOnTimer (XMMSG *msg)
{
	xm_osd_framebuffer_t framebuffer;
	XMRECT rect;
	PROCESSVIEWDATA *ProcessViewData;

	ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ProcessView));
	if(ProcessViewData == NULL)
		return;

	XM_printf(">>>>>>>>>>>>>>>>>ProcessViewOnTimer..............\r\n");
	if((ProcessViewData->process_state==PROCESS_STATE_SUCCESS) || ProcessViewData->process_state==PROCESS_STATE_FAILURE)
	{
		XM_PullWindow(0);
	}

	if(ProcessViewData->ProcessInfo.type==AP_PROCESS_VIEW_SYSTEMUPDATE)
	{
		XM_InvalidateWindow();
		XM_UpdateWindow();	
	}
}

// ϵͳ�¼�����
VOID ProcessViewOnSystemEvent (XMMSG *msg)
{
	PROCESSVIEWDATA *ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ProcessView));
	if(ProcessViewData == NULL)
		return;
	if(ProcessViewData->ProcessInfo.fpSystemEventCb)
	{
		(*ProcessViewData->ProcessInfo.fpSystemEventCb)(msg, ProcessViewData->process_state);
	}
}

// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (ProcessView)
	XM_ON_MESSAGE (XM_PAINT, ProcessViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, ProcessViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, ProcessViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, ProcessViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, ProcessViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, ProcessViewOnTimer)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, ProcessViewOnSystemEvent)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, ProcessView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_ALERT)

