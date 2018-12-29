//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_card.c
// ��SD�������ڶ�������, �ݶ�5��
//		1  SD��������ʽ���������� DEFAULT (���ý���)
//		2	��⵽���ļ�ϵͳ����������ʱ LOWSPACE����ʾ�û����и�ʽ���Ķ������ԡ������ڿ�����������ʱ��ʾ��
//		3	��δ����ʱ NOCARD ����ʾ�û����뿨�Ķ�������
//		4	��д���� WRITEPROTECT����ʾ�û������д����
//		5	���ļ�ϵͳ���ʧ�� FSERROR (���ļ�ϵͳ��ʽ����·���޷����ʻ򴴽���)����ʾ�û����и�ʽ���Ķ�������
//
//	Revision history
//
//		2012.09.19	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"

#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"
#include "xm_app_menudata.h"
#include "system_check.h"


#define	CARDVIEW_COMMAND_FORMAT		0
#define	CARDVIEW_COMMAND_RETURN		1
#define	CARDVIEW_COMMAND_CANCEL		2
#define	CARDVIEW_COMMAND_DEMOMODE	3

static const DWORD cardTitleID[APP_CARDVIEW_CUSTOM_COUNT] = {
	AP_ID_CARD_TITLE_FORMAT,
	AP_ID_CARD_TITLE_FORMAT,
	AP_ID_CARD_TITLE_CARDCHECKING,
	AP_ID_CARD_TITLE_CARDCHECKING,
	AP_ID_CARD_TITLE_FSERROR,
	AP_ID_CARD_TITLE_CARDDAMAGE
};

// CARDVIEW��������
#define	CARDVIEWBTNCOUNT		2
static const XMBUTTONINFO cardBtn[APP_CARDVIEW_CUSTOM_COUNT][CARDVIEWBTNCOUNT] = {
	// 1  SD��������ʽ���������� DEFAULT (���ý���)
	{
		{	
			VK_AP_MENU,		CARDVIEW_COMMAND_FORMAT,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
		},
		{	
			VK_AP_MODE,		CARDVIEW_COMMAND_CANCEL,	AP_ID_COMMON_MODE,	AP_ID_BUTTON_CANCEL
		},
	},
	// 2	��⵽���ļ�ϵͳ����������ʱ LOWSPACE����ʾ�û����и�ʽ���Ķ������ԡ������ڿ���ʱ��ʾ��
	{
		{	
			VK_AP_MENU,		CARDVIEW_COMMAND_FORMAT,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
		},
		{	
			VK_AP_MODE,		CARDVIEW_COMMAND_CANCEL,	AP_ID_COMMON_MODE,	AP_ID_BUTTON_CANCEL
		},
	},
	// 3	��δ����ʱ NOCARDVIEW ����ʾ�û����뿨�Ķ�������
	{
		{	
			VK_AP_MENU,		CARDVIEW_COMMAND_RETURN,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
		},
		{	
			VK_AP_MODE,		CARDVIEW_COMMAND_DEMOMODE,	AP_ID_COMMON_MODE,	AP_ID_CARD_BUTTON_DEMO
		},
	},
	// 4	��д���� ����ʾ�û������д����
	{
		{	
			VK_AP_MENU,		CARDVIEW_COMMAND_RETURN,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
		},
		{	
			VK_AP_MODE,		CARDVIEW_COMMAND_DEMOMODE,	AP_ID_COMMON_MODE,	AP_ID_CARD_BUTTON_DEMO
		},
	},
	//	5	���ļ�ϵͳ���ʧ�� FSERROR (���ļ�ϵͳ��ʽ����·���޷����ʻ򴴽���)����ʾ�û����и�ʽ���Ķ�������
	{
		{	
			VK_AP_MENU,		CARDVIEW_COMMAND_FORMAT,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
		},
		{	
			VK_AP_MODE,		CARDVIEW_COMMAND_CANCEL,	AP_ID_COMMON_MODE,	AP_ID_BUTTON_CANCEL
		},
	},
	//	6	����дУ��ʧ�ܣ������� CARD_DAMAGE ����ʾ�û��������ݿ����߽��и�ʽ���Ķ�������
	{
		{	
			VK_AP_MENU,		CARDVIEW_COMMAND_FORMAT,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
		},
		{	
			VK_AP_MODE,		CARDVIEW_COMMAND_CANCEL,	AP_ID_COMMON_MODE,	AP_ID_BUTTON_CANCEL
		},
	}
};



typedef struct tagCARDVIEWDATA {
	DWORD					dwCustomOption;		// ����ʽ�����ڶ�������

	XMBUTTONCONTROL	btnControl;

} CARDVIEWDATA;


VOID CardViewOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
		// ����δ��������һ�ν���
		CARDVIEWDATA *cardData;

		// ��鿨���ڶ�������
		if(msg->lp > APP_CARDVIEW_CUSTOM_CARDDAMAGE)
			return;
	
		// ����˽�����ݾ��
		cardData = XM_calloc (sizeof(CARDVIEWDATA));
		if(cardData == NULL)
		{
			XM_printf ("cardData XM_calloc failed\n");
			// ʧ�ܷ��ص������ߴ���
			XM_PullWindow (0);
			return;
		}

		cardData->dwCustomOption = msg->lp;

		// ��ť�ؼ���ʼ��
		AP_ButtonControlInit (&cardData->btnControl, CARDVIEWBTNCOUNT, 
			XMHWND_HANDLE(CardView), &cardBtn[msg->lp][0]);

		
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(CardView), cardData);
	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("CardView Pull\n");
	}
}

VOID CardViewOnLeave (XMMSG *msg)
{
	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		CARDVIEWDATA *cardData = (CARDVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(CardView));
		// �ͷ����з������Դ
		if(cardData)
		{
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&cardData->btnControl);
			// �ͷ�˽�����ݾ��
			XM_free (cardData);
		}
		XM_printf ("CardView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("CardView Push\n");
	}
}



VOID CardViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
	APPROMRES *AppRes;
	XMSIZE error_size, info_size;
	int sx, sy;
	DWORD eID, iID;
	int card_state;
	unsigned int old_alpha;

	CARDVIEWDATA *cardData;

	cardData = (CARDVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(CardView));
	if(cardData == NULL)
		return;

	XM_GetDesktopRect (&rc);
	XM_FillRect (XMHWND_HANDLE(CardView), rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_WINDOW));

	old_alpha = XM_GetWindowAlpha (XMHWND_HANDLE(CardView));
	XM_SetWindowAlpha (XMHWND_HANDLE(CardView), 255);

	// --------------------------------------
	//
	// ********* 1 ��ʾ����������Ϣ *********
	//
	// --------------------------------------
	AP_DrawTitlebarControl (XMHWND_HANDLE(CardView), 
				AP_NULLID, 
				cardTitleID[cardData->dwCustomOption] - APP_CARDVIEW_CUSTOM_DEFAULT);


	// ���ݿ���ʽ�����ڵĶ���������ʾ��ʾ��Ϣ
	switch (cardData->dwCustomOption)
	{
		case APP_CARDVIEW_CUSTOM_DEFAULT:
			// ������ʾ
			rc.top = (XMCOORD)(rc.top + APP_TITLEBAR_HEIGHT);
			rc.bottom = (XMCOORD)(rc.bottom - APP_BUTTON_HEIGHT);
			// ���ݿ�������״̬ �����γ�״̬��������д״̬������д����������ʾ��ͬ����Ϣ
			card_state = APSYS_CardChecking ();
			if(card_state == APP_SYSTEMCHECK_CARD_NOCARD)
			{
				eID = AP_ID_CARD_INFO_CARDINSERT;
			//	AppRes = AP_AppRes2RomRes (AP_ID_CARD_INFO_CARDINSERT);
			}
			else if(card_state == APP_SYSTEMCHECK_CARD_WRITEPROTECT)
			{
				eID = AP_ID_CARD_INFO_WRITEPROTECT;
			//	AppRes = AP_AppRes2RomRes (AP_ID_CARD_INFO_WRITEPROTECT);
			}
			else
			{
				eID = AP_ID_CARD_INFO_FORMAT;
			//	AppRes = AP_AppRes2RomRes (AP_ID_CARD_INFO_FORMAT);
			}
			//XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, 
			//	XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
			//AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, 
			//	XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
			AP_RomImageDrawByMenuID (eID, XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
			break;

		case APP_CARDVIEW_CUSTOM_WRITEPROTECT:
			// ����ȥ��д��������Ϣ
			rc.top = (XMCOORD)(rc.top + APP_TITLEBAR_HEIGHT);
			rc.bottom = (XMCOORD)(rc.bottom - APP_BUTTON_HEIGHT);
			// ������ʾ
		//	AppRes = AP_AppRes2RomRes (AP_ID_CARD_INFO_WRITEPROTECT);
			//XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, 
			//	XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
		//	AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, 
		//		XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
			AP_RomImageDrawByMenuID (AP_ID_CARD_INFO_WRITEPROTECT, XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
			break;

		case APP_CARDVIEW_CUSTOM_CARDDAMAGE:
			// �����ݿ����𻵣����������Ϣ
			rc.top = (XMCOORD)(rc.top + APP_TITLEBAR_HEIGHT);
			rc.bottom = (XMCOORD)(rc.bottom - APP_BUTTON_HEIGHT);
			// ������ʾ
		//	AppRes = AP_AppRes2RomRes (AP_ID_CARD_INFO_CARDDAMAGE);
		//	XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, 
		//		XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
		//	AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, 
		//		XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
			AP_RomImageDrawByMenuID (AP_ID_CARD_INFO_CARDDAMAGE, XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
			break;

		case APP_CARDVIEW_CUSTOM_LOWSPACE:
		case APP_CARDVIEW_CUSTOM_NOCARD:
		case APP_CARDVIEW_CUSTOM_FSERROR:
			rc.top = (XMCOORD)(rc.top + APP_TITLEBAR_HEIGHT);
			rc.bottom = (XMCOORD)(rc.bottom - APP_BUTTON_HEIGHT);
			if(cardData->dwCustomOption == APP_CARDVIEW_CUSTOM_LOWSPACE)
			{
				eID = AP_ID_CARD_INFO_LOWSPACE;
				iID = AP_ID_CARD_INFO_FORMAT;
			}
			else if(cardData->dwCustomOption == APP_CARDVIEW_CUSTOM_NOCARD)
			{
				eID = AP_ID_CARD_INFO_NOCARD;
				iID = AP_ID_CARD_INFO_CARDINSERT;
			}
			else if(cardData->dwCustomOption == APP_CARDVIEW_CUSTOM_FSERROR)
			{
				eID = AP_ID_CARD_INFO_FSERROR;
				iID = AP_ID_CARD_INFO_FORMAT;
			}
			else
				break;
			// ��ȡ������Ϣ����ʼ��ʽ����ʾ��Ϣ�ĳߴ�
			AppRes = AP_AppRes2RomRes (eID);
			XM_GetGifImageSize (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, &error_size);
			AppRes = AP_AppRes2RomRes (iID);
			XM_GetGifImageSize (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, &info_size);
			// ������Ϣ����ʾλ��
			sy = ((rc.bottom - rc.top + 1) - (error_size.cy + info_size.cy + 24)) / 2;
			sx = ((rc.right - rc.left + 1) - error_size.cx) / 2;

			// ��ʾ��������Ϣ
			rect.top = (XMCOORD)(rc.top + sy);
			rect.left = (XMCOORD)(sx);
			rect.right = rect.left;
			rect.bottom = rect.top;
			AppRes = AP_AppRes2RomRes (eID);
			XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(CardView), &rect, XMGIF_DRAW_POS_LEFTTOP);
	//		AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, XMHWND_HANDLE(CardView), &rect, XMGIF_DRAW_POS_LEFTTOP);
		
			// ��ʾ��ʼ��ʽ����Ϣ
			rect.top = (XMCOORD)(rect.top + error_size.cy + 12);
			sx = ((rc.right - rc.left + 1) - info_size.cx) / 2;
			rect.left = (XMCOORD)(sx);
			rect.right = rect.left;
			rect.bottom = rect.top;
			AppRes = AP_AppRes2RomRes (iID);
			XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(CardView), &rect, XMGIF_DRAW_POS_LEFTTOP);
		//	AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, XMHWND_HANDLE(CardView), &rect, XMGIF_DRAW_POS_LEFTTOP);
	}

	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
	AP_ButtonControlMessageHandler (&cardData->btnControl, msg);

	XM_SetWindowAlpha (XMHWND_HANDLE(CardView), (unsigned char)old_alpha);
}



VOID CardViewOnKeyDown (XMMSG *msg)
{
	CARDVIEWDATA *cardData;
	cardData = (CARDVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(CardView));
	if(cardData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:
			// ��"��"�����У�MENU��MODE��������Ϊ��ť������
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&cardData->btnControl, msg);
			break;

		case VK_AP_UP:		// ����¼���
			AP_ButtonControlMessageHandler (&cardData->btnControl, msg);
			break;

		case VK_AP_DOWN:
			AP_ButtonControlMessageHandler (&cardData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&cardData->btnControl, msg);
			break;
	}

}

VOID CardViewOnKeyUp (XMMSG *msg)
{
	CARDVIEWDATA *cardData;
	cardData = (CARDVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(CardView));
	if(cardData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// ģʽ��
			// ��"����ʽ��"�����У�MENU��MODE��������Ϊ��ť������
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&cardData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&cardData->btnControl, msg);
			break;
	}
}

VOID CardViewOnCommand (XMMSG *msg)
{
	CARDVIEWDATA *cardData;

	cardData = (CARDVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(CardView));
	if(cardData == NULL)
		return;

	switch(msg->wp)
	{
		case CARDVIEW_COMMAND_FORMAT:		// ��ʽ������
			// ִ�и�ʽ������
			AP_OpenFormatView (0);
			break;

		case CARDVIEW_COMMAND_RETURN:		// ����
			// ���ص�����
			XM_PullWindow (0);
			break;

		case CARDVIEW_COMMAND_CANCEL:		// ȡ��
			XM_PullWindow (0);
			break;

		case CARDVIEW_COMMAND_DEMOMODE:		// �޿�
			// ����"�޿�����ģʽ",����ʾģʽ
			AppSetCardOperationMode (APP_DEMO_OPERATION_MODE);
			XM_PullWindow (0);
			break;
	}
}

// ������¼�����
// #define	XM_CARDVIEW			0x10	// SD������¼�
										// lp����Ϊ0
										// wp = 0, SD���γ��¼�
										// wp = 1, SD������(д����)
										// wp = 2, SD������(��д����)

VOID CardViewOnSystemEvent (XMMSG *msg)
{
	CARDVIEWDATA *cardData;
	cardData = (CARDVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(CardView));
	if(cardData == NULL)
		return;
	switch (cardData->dwCustomOption)
	{
		case APP_CARDVIEW_CUSTOM_NOCARD:			// ����δ���롱��������
			if(msg->wp == SYSTEM_EVENT_CARD_INSERT_WRITE_PROTECT || msg->wp == SYSTEM_EVENT_CARD_INSERT)
			{
				// ��⵽�������¼�
				// ���ص������ߴ���
				XM_BreakSystemEventDefaultProcess (msg);
				XM_PullWindow (0);
				return;
			}
			break;

		case APP_CARDVIEW_CUSTOM_WRITEPROTECT:	// ����д��������������
			if(msg->wp == SYSTEM_EVENT_CARD_UNPLUG)
			{
				// ��⵽���γ�
				// ���ص������ߴ���
				XM_BreakSystemEventDefaultProcess (msg);
				XM_PullWindow (0);
				return;
			}
			break;

		case APP_CARDVIEW_CUSTOM_FSERROR:			// ���ļ�ϵͳ���󡱶���ģʽ
		case APP_CARDVIEW_CUSTOM_CARDDAMAGE:		// "����дУ��ʧ�ܣ�������"	
			// 1) ����Ƿ���ͬ�࿨�¼������ǣ�ֱ�ӹ��˸�ͬ���¼�
			if(	msg->wp == SYSTEM_EVENT_CARD_FS_ERROR
				||	msg->wp == SYSTEM_EVENT_CARD_VERIFY_ERROR)
			{
				// ����
				XM_BreakSystemEventDefaultProcess (msg);
				return;
			}
			// 2) ����Ƿ����������¼������ǣ��˳�����Ӧ���¼�
			else if(msg->wp == SYSTEM_EVENT_CARD_DETECT
				||	msg->wp == SYSTEM_EVENT_CARD_UNPLUG
				||	msg->wp == SYSTEM_EVENT_CARD_INSERT_WRITE_PROTECT
				||	msg->wp == SYSTEM_EVENT_CARD_INSERT
				||	msg->wp == SYSTEM_EVENT_CARD_INVALID)
			{
				XM_BreakSystemEventDefaultProcess (msg);
				XM_PullWindow (0);
				AP_PostSystemEvent (msg->wp);
				return;				
			}
			// 3) ���������¼�����ȱʡϵͳ�¼�������̴���
			break;

		case APP_CARDVIEW_CUSTOM_LOWSPACE:			// ��ѭ����¼�ռ�͡�����ģʽ
			if(msg->wp == SYSTEM_EVENT_CARD_UNPLUG)
			{
				// ��⵽���γ�
				// ���ص������ߴ���
				XM_BreakSystemEventDefaultProcess (msg);
				XM_PullWindow (0);
				return;
			}
			break;

		case APP_CARDVIEW_CUSTOM_DEFAULT:			// ����ʽ��������ģʽ
			// ���Կ�����¼�������ˢ����ʾ����ʾ��ͬ����Ϣ
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;
	}		
}


// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (CardView)
	XM_ON_MESSAGE (XM_PAINT, CardViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, CardViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, CardViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, CardViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, CardViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, CardViewOnCommand)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, CardViewOnSystemEvent)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, CardView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_EVENT)

