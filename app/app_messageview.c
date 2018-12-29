//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_messageview.c
//	  ��ʾ��Ϣ�Ӵ�
//		(����ʾ��ʾ��Ϣ���ɶ�����ֹ��ʱ���Զ��رգ�һ��ȷ�ϰ�ť)
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"

#define MESSAGEVIEW_COMMAND_OK			1
#define MESSAGEVIEW_COMMAND_CANCEL		2
#define	MESSAGEVIEW_COMMAND_RETURN		3

// ���˵�ѡ����ڰ�ť�ؼ�����
#define	MESSAGEVIEWBTNCOUNT	1
#define COMFIRMVIEWBTNCOUNT	2

static const XMBUTTONINFO menuOptionReturnBtn[MESSAGEVIEWBTNCOUNT] = {
	{	
		VK_AP_MENU,		MESSAGEVIEW_COMMAND_RETURN,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_RETURN
	}
};

static const XMBUTTONINFO menuOptionComfirmBtn[COMFIRMVIEWBTNCOUNT] = {
	{	
		VK_AP_MENU,		MESSAGEVIEW_COMMAND_OK,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
	},
	{	
		VK_AP_MODE,		MESSAGEVIEW_COMMAND_CANCEL,	AP_ID_COMMON_OK,	AP_ID_BUTTON_CANCEL
	}
};

typedef struct tagMESSAGEVIEWDATA {
	DWORD					dwTitleID;				// �������ı�	
	DWORD					dwMessageID;			// ��Ϣ�ı�
	DWORD					dwAutoCloseTime;		// �Ӵ���ʱ���Զ��ر�ʱ�䡣-1��ʾ
	DWORD					dwMode;					// ��ʾ
	FPMENUOPTIONCB		fpMenuOptionCB;		// �ص��������˴�Ӧ�ã�����������Ϊ��
	DWORD					dwTimeout;				// ������
	XMBUTTONCONTROL	btnControl;				// ��ť�ؼ�
} MESSAGEVIEWDATA;

VOID AP_OpenComfirmView (DWORD dwViewTitleID, DWORD dwViewMessageID, void *fpMenuOptionCB)
{
	MESSAGEVIEWDATA *messageViewData = XM_calloc (sizeof(MESSAGEVIEWDATA));
	if(messageViewData == NULL)
		return;
	messageViewData->dwMessageID = dwViewMessageID;
	messageViewData->dwTitleID = dwViewTitleID;
	messageViewData->dwAutoCloseTime = (DWORD)-1;
	messageViewData->dwMode = COMFIRMVIEWBTNCOUNT;
	messageViewData->fpMenuOptionCB = (FPMENUOPTIONCB)fpMenuOptionCB;
	if(!XM_PushWindowEx (XMHWND_HANDLE(MessageView), (DWORD)messageViewData))
	{
		XM_free (messageViewData);
		XM_printf ("XM_PushWindowEx MessageView NG\n");
	}
}

// ��ʾ��Ϣ��ͼ
// dwViewTitleID ��Ϣ����ID
// dwViewMessageID ��Ϣ�ı�ID
// dwAutoCloseTime ָ���Զ��ر�ʱ��(�뵥λ) (-1) ��ֹ�Զ��ر�
VOID AP_OpenMessageViewEx (DWORD dwViewTitleID, DWORD dwViewMessageID, DWORD dwAutoCloseTime, XMBOOL bPopTopView)
{
	MESSAGEVIEWDATA *messageViewData = XM_calloc (sizeof(MESSAGEVIEWDATA));
	if(messageViewData == NULL)
		return;
	messageViewData->dwMessageID = dwViewMessageID;
	messageViewData->dwTitleID = dwViewTitleID;
	messageViewData->dwAutoCloseTime = dwAutoCloseTime;
	//messageViewData->dwMode = MESSAGEVIEWBTNCOUNT;
	messageViewData->dwMode = COMFIRMVIEWBTNCOUNT;
	messageViewData->fpMenuOptionCB = NULL;
	if(bPopTopView)
	{
		if(!XM_JumpWindowEx (XMHWND_HANDLE(MessageView), (DWORD)messageViewData, XM_JUMP_POPTOPVIEW))
		{
			XM_free (messageViewData);
		}
	}
	else
	{
		if(!XM_PushWindowEx (XMHWND_HANDLE(MessageView), (DWORD)messageViewData))
		{
			XM_free (messageViewData);
		}
	}
}

// ��ʾ��Ϣ��ͼ(ʹ��ȱʡ���⡢3�����Զ��ر�)
VOID AP_OpenMessageView (DWORD dwViewMessageID)
{
	AP_OpenMessageViewEx (AP_ID_MESSAGE_PROMPT_TITLE, dwViewMessageID, 3, 0);
}


VOID MessageViewOnEnter (XMMSG *msg)
{
	MESSAGEVIEWDATA *messageViewData;
	if(msg->wp == 0)
	{
		messageViewData = (MESSAGEVIEWDATA *)msg->lp;

		// ��ť�ؼ���ʼ��
		if (messageViewData->dwMode == MESSAGEVIEWBTNCOUNT)
			AP_ButtonControlInit (&messageViewData->btnControl, MESSAGEVIEWBTNCOUNT, XMHWND_HANDLE(MessageView), menuOptionReturnBtn);
		else
			AP_ButtonControlInit (&messageViewData->btnControl, COMFIRMVIEWBTNCOUNT, XMHWND_HANDLE(MessageView), menuOptionComfirmBtn);

		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(MessageView), messageViewData);
	}
	else
	{
		messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("MessageView Pull\n");
	}

	// ������ʱ��
	if(messageViewData && messageViewData->dwAutoCloseTime != (DWORD)(-1))
	{
		XM_SetTimer (XMTIMER_MESSAGEVIEW, 100);	// ����100ms�Ķ�ʱ��
		messageViewData->dwTimeout = XM_GetTickCount() + messageViewData->dwAutoCloseTime * 1000;
	}
}

VOID MessageViewOnLeave (XMMSG *msg)
{
	MESSAGEVIEWDATA *messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
	// ɾ����ʱ��
	if(messageViewData && messageViewData->dwAutoCloseTime != (DWORD)(-1))
	{
		XM_KillTimer (XMTIMER_MESSAGEVIEW);
	}

	if (msg->wp == 0)
	{
		MESSAGEVIEWDATA *messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
		if(messageViewData)
		{
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&messageViewData->btnControl);

			// �ͷ�˽�����ݾ��
			XM_free (messageViewData);
			
		}
		XM_printf ("MessageView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("MessageView Push\n");
	}
}



VOID MessageViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
	APPROMRES *AppRes;
	unsigned int old_alpha;
	HANDLE hWnd = XMHWND_HANDLE(MessageView);

	MESSAGEVIEWDATA *messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (hWnd);
	if(messageViewData == NULL)
		return;

	// ��ʾ������
	XM_GetDesktopRect (&rc);
	XM_FillRect (hWnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	old_alpha = XM_GetWindowAlpha (hWnd);
	XM_SetWindowAlpha (hWnd, 255);

	// --------------------------------------
	//
	// ********* 1 ��ʾ����������Ϣ *********
	//
	// --------------------------------------
	AP_DrawTitlebarControl (hWnd, AP_NULLID, messageViewData->dwTitleID);

	rect.left = rc.left;
	rect.right = rc.right;
	rect.top = (XMCOORD)(rc.top + APP_TITLEBAR_HEIGHT);
	rect.bottom = (XMCOORD)(rc.bottom - APP_BUTTON_HEIGHT);
	AppRes = AP_AppRes2RomRes (messageViewData->dwMessageID);
	XM_RomImageDraw (AppRes->rom_offset, AppRes->res_length, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
	// --------------------------------------
	//
	// ********* 3 ��ʾ��ť�� ***************
	//
	// --------------------------------------
	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
	AP_ButtonControlMessageHandler (&messageViewData->btnControl, msg);

	XM_SetWindowAlpha (hWnd, (unsigned char)old_alpha);

}


VOID MessageViewOnKeyDown (XMMSG *msg)
{
	MESSAGEVIEWDATA *messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
	if(messageViewData == NULL)
		return;
// ������
	XM_Beep (XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
		case VK_AP_SWITCH:
			AP_ButtonControlMessageHandler (&messageViewData->btnControl, msg);
			break;

		default:
			// �˴������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&messageViewData->btnControl, msg);
			break;

	}
}

VOID MessageViewOnKeyUp (XMMSG *msg)
{
	MESSAGEVIEWDATA *messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
	if(messageViewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
		case VK_AP_SWITCH:
			AP_ButtonControlMessageHandler (&messageViewData->btnControl, msg);
			break;

		default:
			// �˴������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&messageViewData->btnControl, msg);
			break;
	}
}

VOID MessageViewOnCommand (XMMSG *msg)
{
	MESSAGEVIEWDATA *messageViewData;
	messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
	if(messageViewData == NULL)
		return;
	switch(msg->wp)
	{
		case MESSAGEVIEW_COMMAND_OK:
			if(messageViewData->fpMenuOptionCB)
			{
				(*messageViewData->fpMenuOptionCB) ((void*)NULL, (int)NULL);
			}

//			Ŀǰ�Ĵ�����Ԥ�ȴ����õ�һ�����ڣ�������һ��ʵ��������ջ�в���ͬʱ����������ͬ�Ĵ��ڣ�����˽�����ݻᱻ���ǡ�
//			AP_OpenMessageViewEx (messageViewData->dwTitleID, AP_ID_MESSAGE_SUCCESS, 30, 0);
//			else
//				AP_OpenMessageViewEx (messageViewData->dwTitleID, AP_ID_MESSAGE_FAIL, 30, 0);

			XM_PullWindow (0);
			break;
				
		case MESSAGEVIEW_COMMAND_CANCEL:
		case MESSAGEVIEW_COMMAND_RETURN:
			// ���ص��������Ӵ�
			XM_PullWindow (0);
			break;
	}
}

VOID MessageViewOnTimer (XMMSG *msg)
{
	MESSAGEVIEWDATA *messageViewData;
	messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
	if(messageViewData == NULL)
		return;
	if(messageViewData->dwAutoCloseTime != (DWORD)(-1))
	{
		if(messageViewData->dwTimeout <= XM_GetTickCount())
		{
			// ���ص��������Ӵ�
			XM_PullWindow (0);
		}
	}
}
VOID MessageViewOnTouchDown (XMMSG *msg)
{
       int x,y;
	int index;
	MESSAGEVIEWDATA *messageViewData;
	messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
	if(messageViewData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&messageViewData->btnControl, msg))
		return;	

}

VOID MessageViewOnTouchUp (XMMSG *msg)
{
       int x,y;
	   MESSAGEVIEWDATA *messageViewData;
	messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
	if(messageViewData == NULL)
		return;


	if(AP_ButtonControlMessageHandler (&messageViewData->btnControl, msg))
		return;
	
}
// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (MessageView)
	XM_ON_MESSAGE (XM_PAINT, MessageViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, MessageViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, MessageViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, MessageViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, MessageViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, MessageViewOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, MessageViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, MessageViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, MessageViewOnTouchUp)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, MessageView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
