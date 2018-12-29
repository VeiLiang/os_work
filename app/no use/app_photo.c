//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_settingview.c
//	  ���ô���
//
//	Revision history
//
//		2012.09.10	ZhuoYongHong Initial version
//
//****************************************************************************


#include "app.h"
#include "app_menuid.h"
#include "app_menuoptionview.h"

extern int	iRecordMode;
extern int  CurrentPhotoMode;
#define DEMO_MODE			0			// ��ʾģʽ(�޿����롢��д��������)
#define WAITING_MODE		1			// �ȴ�ģʽ(��ʱ��¼�ȴ�)
#define RECORD_MODE		2			// ��¼ģʽ



// ˽�������

#define	MAIN_COMMAND_RECORD		      0
#define	MAIN_COMMAND_PHOTO	             1
#define	MAIN_COMMAND_VEDIO_PLAY		2
#define	MAIN_COMMAND_EXIT		       3
// �����á����ڰ�ť�ؼ�����

#define	PHOTOBTNCOUNT	2
static const TPBUTTONINFO PHOTOTpBtn[PHOTOBTNCOUNT] = {

    {	
		780,200,848,300,MAIN_COMMAND_PHOTO,	MAIN_COMMAND_PHOTO,	AP_ID_PHOTO,	AP_ID_PHOTO
	},
	
	{	
		783,360,848,480,MAIN_COMMAND_EXIT,	MAIN_COMMAND_EXIT,	AP_ID_BUTTON_RETURN,	AP_ID_BUTTON_RETURN
	},
	
};



typedef struct tagPHOTODATA {
	int				nTopItem;					// ��һ�����ӵĲ˵���
	int				nCurItem;					// ��ǰѡ��Ĳ˵���
	int				nItemCount;					// �˵������
	int				nTouchItem;					// ��ǰ������, -1 ��ʾû��

	APPMENUOPTION	menuOption;					// �˵�ѡ��

	XMBUTTONCONTROL	btnControl;				// ��ť�ؼ�

	TPBUTTONCONTROL	TPbtnControl;				// ��ť�ؼ�

	//BYTE				bTimer;				  // ��ʱ��

} PHOTODATA;


VOID PhotoOnEnter (XMMSG *msg)
{
	PHOTODATA *photoData;
	XMPOINT pt;
	XM_printf ("MainViewOnEnter\n");
	CurrentPhotoMode=1;
	if(msg->wp == 0)
	{
		// ����δ��������һ�ν���
	
		// ����˽�����ݾ��
		photoData = XM_calloc (sizeof(PHOTODATA));
		if(photoData == NULL)
		{
			XM_printf ("settingViewData XM_calloc failed\n");
			// ʧ�ܷ��ص�����
			XM_PullWindow (0);
			return;
		}

		pt.x = 0;
		pt.y = 0;
		
		// ��ť�ؼ���ʼ��
		AP_TpButtonControlInit (&photoData->TPbtnControl, PHOTOBTNCOUNT, XMHWND_HANDLE(Photo), PHOTOTpBtn);
	
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(Photo), photoData);

	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("Setting Pull\n");
	}
	
	// ������ʱ��������ʱ��ˢ��
	// ����1��Ķ�ʱ��
//	XM_SetTimer (XMTIMER_SETTINGVIEW, 1000);
}

VOID PhotoOnLeave (XMMSG *msg)
{
	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		PHOTODATA *photoData = (PHOTODATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(Photo));
		// �ͷ����з������Դ
		if(photoData)
		{
		      	// ��ť�ؼ��˳�����
			AP_TpButtonControlExit (&photoData->TPbtnControl);
			
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&photoData->btnControl);
		

			// �ͷ�˽�����ݾ��
			XM_free (photoData);
		}
		// ���ô��ڵ�˽�����ݾ��Ϊ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(Photo), NULL);
		XM_printf ("Main Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("Main Push\n");
	}

	XM_KillTimer (XMTIMER_SETTINGVIEW);
}


VOID PhotoOnPaint (XMMSG *msg)
{
	int nItem, nLastItem;
	XMRECT rc, rect, rcArrow;
	int i;
	int nVisualCount;
	unsigned int ticket = XM_GetTickCount();
	HANDLE hWnd = XMHWND_HANDLE(Photo);
//	XM_IMAGE *image;

	float scale_factor;		// ˮƽ��������
//	APPROMRES *AppRes;
	//XMSYSTEMTIME SystemTime;
	XMCOLOR bkg_color = XM_GetSysColor(XM_COLOR_DESKTOP);
	unsigned int old_alpha;
	DWORD menuID;

	PHOTODATA *photoData;
	
	XMCOORD menu_name_x, menu_data_x, menu_flag_x;	// �˵�����⡢��ֵ����ʶ��x����

	photoData = (PHOTODATA *)XM_GetWindowPrivateData (hWnd);

	if(photoData == NULL)
		return;
	old_alpha = XM_GetWindowAlpha (hWnd);
	//XM_SetWindowAlpha (hWnd, 255);
		XM_GetDesktopRect (&rc);
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;

	// ����ˮƽ��������(UI��320X240������)
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);
      XM_SetWindowAlpha (hWnd, 0);
	XM_FillRect (hWnd, rc.left, rc.top, rc.right, rc.bottom, bkg_color);

	old_alpha = XM_GetWindowAlpha (hWnd);
	XM_SetWindowAlpha (hWnd, 255);

	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
	AP_TpButtonControlMessageHandler (&photoData->TPbtnControl, msg);

	XM_SetWindowAlpha (hWnd, (unsigned char)old_alpha);
}






VOID PhotoOnKeyDown (XMMSG *msg)
{
	PHOTODATA *photoData;
	int nVisualCount;
	XMRECT rc;

	photoData = (PHOTODATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(Photo));
	if(photoData == NULL)
		return;

	XM_GetWindowRect (XMHWND_HANDLE(Photo), &rc);
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;

	if(msg->message == XM_KEYDOWN)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_SWITCH:
			case VK_AP_MODE:
				// ��"����"�����У�MENU��SWITCH��MODE��������Ϊ��ť������
				// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
				AP_ButtonControlMessageHandler (&photoData->btnControl, msg);
				break;
		}
	}
}

VOID PhotoOnKeyUp (XMMSG *msg)
{
	PHOTODATA *photoData;

	photoData = (PHOTODATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(Photo));
	if(photoData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:
		case VK_AP_SWITCH:
		case VK_AP_MODE:
			// ��"����"�����У�MENU��SWITCH��MODE��������Ϊ��ť������
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&photoData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&photoData->btnControl, msg);
			break;
	}
}

VOID PhotoOnTimer (XMMSG *msg)
{
	XM_PostMessage (XM_KEYDOWN, VK_AP_DOWN, 0);
	//SettingDisplayDateTime ();
}

// �ļ�ϵͳ�޷����ʴ���ص�����
static void fs_access_error_alert_cb_main (void *userPrivate, unsigned int uKeyPressed)
{
	if(uKeyPressed == VK_AP_MENU)		// ����ʽ��������
	{
		// ��ʽ������
		AP_OpenFormatView (0);
	}
	else					// ��ȡ��������
	{
		// ���ص������ߴ���
		XM_PullWindow (0);
	}
}




VOID PhotoOnCommand (XMMSG *msg)
{
	PHOTODATA *photoData;
	photoData = (PHOTODATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(Photo));
	if(photoData == NULL)
		return;
	
	switch(msg->wp)
	
	{   case MAIN_COMMAND_PHOTO:
	         //  if(iRecordMode == RECORD_MODE)
			{
			  
				AP_OnekeyPhotograph ();
			}
			break;
	

		case MAIN_COMMAND_RECORD:
			// �л�����¼�����á�����
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP);
			XM_JumpWindowEx (XMHWND_HANDLE(VideoSettingView), 0, XM_JUMP_POPDEFAULT);
			break;
			
        case MAIN_COMMAND_EXIT:
			// �л�����DESKTOP������
			// �л�����¼�����á�����
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP);
			XM_JumpWindowEx (XMHWND_HANDLE(MainView), 0, XM_JUMP_POPDEFAULT);
			break;
			   	
		/*	
		case SETTING_COMMAND_RETURN:
			// ���ص�����
			XM_PullWindow (0);
			break;
		*/
	}
}

VOID PhotoOnTouchDown (XMMSG *msg)
{
	int index;
	unsigned int x, y;
	HANDLE hWnd = XMHWND_HANDLE(Photo);
	PHOTODATA *photoData = (PHOTODATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(Photo));
   
	if(photoData == NULL)
		return;

	if(AP_TpButtonControlMessageHandler (&photoData->TPbtnControl, msg))
		return;
}

VOID PhotoOnTouchUp (XMMSG *msg)
{
    unsigned int x, y;
	PHOTODATA *photoData;
	photoData = (PHOTODATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(Photo));
	if(photoData == NULL)
		return;
	
	x = LOWORD(msg->lp);
	y = HIWORD(msg->lp);

	if(AP_TpButtonControlMessageHandler (&photoData->TPbtnControl, msg))
		return;
}



// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (Photo)
	XM_ON_MESSAGE (XM_PAINT, PhotoOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, PhotoOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, PhotoOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, PhotoOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, PhotoOnLeave)
	XM_ON_MESSAGE (XM_TIMER, PhotoOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, PhotoOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, PhotoOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, PhotoOnTouchUp)

XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, Photo, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)


