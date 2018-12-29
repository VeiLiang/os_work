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
extern int		iRecordMode;
#define DEMO_MODE			0			// ��ʾģʽ(�޿����롢��д��������)
#define WAITING_MODE		1			// �ȴ�ģʽ(��ʱ��¼�ȴ�)
#define RECORD_MODE		2			// ��¼ģʽ



// ˽�������

#define	MAIN_COMMAND_PHOTO		      0
#define	MAIN_COMMAND_RETURN		             1
#define	MAIN_COMMAND_VEDIO_PLAY		2
#define	MAIN_COMMAND_MENU		       3
// �����á����ڰ�ť�ؼ�����
#define	MAINBTNCOUNT	4
static const TPBUTTONINFO MianTpBtn[MAINBTNCOUNT] = {
	
	{	
		20,150,220,439,VK_AP_SWITCH,	MAIN_COMMAND_RETURN,	AP_ID_MAIN_VIDEO_UP,	AP_ID_MAIN_VIDEO_DOWN
	},
        {	
		220,150,420,439,VK_AP_MENU,		MAIN_COMMAND_PHOTO,	AP_ID_MAIN_PHOTO_UP,	AP_ID_MAIN_PHOTO_DOWN
	},
	{	
		420,150,620,439,VK_AP_MODE,     MAIN_COMMAND_VEDIO_PLAY,	AP_ID_MAIN_REPLAY_UP,	AP_ID_MAIN_REPLAY_DOWN
	},
	{	
		620,150,820,439,VK_AP_MENU,     MAIN_COMMAND_MENU,	AP_ID_MAIN_SET_UP,	AP_ID_MAIN_SET_DOWN
	},
	
	
};



typedef struct tagMAINVIEWDATA {
	int				nTopItem;					// ��һ�����ӵĲ˵���
	int				nCurItem;					// ��ǰѡ��Ĳ˵���
	int				nItemCount;					// �˵������
	int				nTouchItem;					// ��ǰ������, -1 ��ʾû��
       XMSYSTEMTIME		dateTime;	
	APPMENUOPTION	menuOption;					// �˵�ѡ��

	XMBUTTONCONTROL	btnControl;				// ��ť�ؼ�

	TPBUTTONCONTROL	TPbtnControl;				// ��ť�ؼ�


//	BYTE				bTimer;						// ��ʱ��

} MAINVIEWDATA;


VOID MainViewOnEnter (XMMSG *msg)
{
	MAINVIEWDATA *settingViewData;
	XMPOINT pt;
	XM_printf ("MainViewOnEnter\n");
	if(msg->wp == 0)
	{
		// ����δ��������һ�ν���
	
		// ����˽�����ݾ��
		settingViewData = XM_calloc (sizeof(MAINVIEWDATA));
		if(settingViewData == NULL)
		{
			XM_printf ("settingViewData XM_calloc failed\n");
			// ʧ�ܷ��ص�����
			XM_PullWindow (0);
			return;
		}

		// ��ʼ��˽������
		/*settingViewData->nCurItem = 0;
		settingViewData->nItemCount = SETTING_MENU_COUNT;
		settingViewData->nTouchItem = -1;*/

		pt.x = 0;
		pt.y = 0;
		
	
		// ��ȡ��ǰϵͳʱ����Ϊʱ�����õĳ�ʼֵ����ʱ���Ѷ�ʧ(RTC����)��ϵͳ���Զ�����Ϊĳ��ȱʡʱ�䡣��20130101
		XM_GetLocalTime (&settingViewData->dateTime);

		// ��ť�ؼ���ʼ��
		AP_TpButtonControlInit (&settingViewData->TPbtnControl, MAINBTNCOUNT, XMHWND_HANDLE(MainView), MianTpBtn);
	
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(MainView), settingViewData);

	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("Setting Pull\n");
	}
	
	// ������ʱ��������ʱ��ˢ��
	// ����1��Ķ�ʱ��
	XM_SetTimer (XMTIMER_SETTINGVIEW, 1000);
}

VOID MainViewOnLeave (XMMSG *msg)
{
	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		MAINVIEWDATA *settingViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MainView));
		// �ͷ����з������Դ
		if(settingViewData)
		{
		      	// ��ť�ؼ��˳�����
			AP_TpButtonControlExit (&settingViewData->TPbtnControl);
			
			// ��ť�ؼ��˳�����
			//AP_ButtonControlExit (&settingViewData->btnControl);
		

			// �ͷ�˽�����ݾ��
			XM_free (settingViewData);
		}
		// ���ô��ڵ�˽�����ݾ��Ϊ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(MainView), NULL);
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


VOID MainViewOnPaint (XMMSG *msg)
{
     
	XMCOORD x, y,x2,y2;

	unsigned int old_alpha;
	char text[32];
	char text2[32];
	XMSIZE size;
	XMRECT rc, rect, rcArrow;

	unsigned int ticket = XM_GetTickCount();
	HANDLE hWnd = XMHWND_HANDLE(MainView);
	XMCOLOR bkg_color = XM_GetSysColor(XM_COLOR_WINDOWTEXT);
	MAINVIEWDATA *MainViewData;
	MainViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (hWnd);
	if(MainViewData == NULL)
		return;
	old_alpha = XM_GetWindowAlpha (hWnd);
	XM_GetDesktopRect (&rc);
	XM_FillRect (hWnd, rc.left, rc.top, rc.right, rc.bottom, bkg_color);
	old_alpha = XM_GetWindowAlpha (hWnd);
	XM_SetWindowAlpha (hWnd, 255);

//////////////
        XM_GetLocalTime (&MainViewData->dateTime);

	// ��ʽ2�� ������� "2012 / 09 / 16"
	AP_FormatDataTime (&MainViewData->dateTime, APP_DATETIME_FORMAT_3, text, sizeof(text));
	AP_TextGetStringSize (text, strlen(text), &size);
	x = 400;
	y = 20;
	// ��ʽ4�� ���ʱ���� "10 : 22 : 20"
	AP_FormatDataTime (&MainViewData->dateTime, APP_DATETIME_FORMAT_7, text2, sizeof(text2));
	AP_TextGetStringSize (text2, strlen(text2), &size);
	x2 = 350;
	y2 =60;
	// ��ʽ2�� ������� "2012 / 09 / 16"
	AP_TextOutDataTimeString (XMHWND_HANDLE(MainView), x, y, text, strlen(text));
	// ��ʽ4�� ���ʱ���� "10 : 22 : 20"
	AP_TextOutDataTimeString (XMHWND_HANDLE(MainView), x2, y2, text2, strlen(text2));
////////////

	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
	AP_TpButtonControlMessageHandler (&MainViewData->TPbtnControl, msg);

	XM_SetWindowAlpha (hWnd, (unsigned char)old_alpha);
     
	
}






VOID MainViewOnKeyDown (XMMSG *msg)
{
	MAINVIEWDATA *settingViewData;
	int nVisualCount;
	XMRECT rc;

	settingViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MainView));
	if(settingViewData == NULL)
		return;

	XM_GetWindowRect (XMHWND_HANDLE(MainView), &rc);
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
// ������
	XM_Beep (XM_BEEP_KEYBOARD);
	if(msg->message == XM_KEYDOWN)
	{
		switch(msg->wp)
		{
		    #if 0
			// ���ϼ�
			case VK_AP_UP:
				
				AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
				settingViewData->nCurItem --;
				if(settingViewData->nCurItem < 0)
				{
					// �۽������һ��
					settingViewData->nCurItem = (WORD)(settingViewData->nItemCount - 1);
					settingViewData->nTopItem = 0;
					while ( (settingViewData->nCurItem - settingViewData->nTopItem) >= nVisualCount )
					{
						settingViewData->nTopItem ++;
					}
				}
				else
				{
					if(settingViewData->nTopItem > settingViewData->nCurItem)
						settingViewData->nTopItem = settingViewData->nCurItem;
				}
				// ˢ��
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
				break;

			// ���¼�
			case VK_AP_DOWN:
				AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
				settingViewData->nCurItem ++;	
				if(settingViewData->nCurItem >= settingViewData->nItemCount)
				{
					// �۽�����һ��
					settingViewData->nTopItem = 0;
					settingViewData->nCurItem = 0;
				}
				else
				{
					while ( (settingViewData->nCurItem - settingViewData->nTopItem) >= nVisualCount )
					{
						settingViewData->nTopItem ++;
					}
				}
				// ˢ��
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
				break;
                          #endif
			case VK_AP_MENU:
			case VK_AP_SWITCH:
			case VK_AP_MODE:
				// ��"����"�����У�MENU��SWITCH��MODE��������Ϊ��ť������
				// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
				AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
				break;
		}
	}
}

VOID MainViewOnKeyUp (XMMSG *msg)
{
	MAINVIEWDATA *settingViewData;

	settingViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MainView));
	if(settingViewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:
		case VK_AP_SWITCH:
		case VK_AP_MODE:
			// ��"����"�����У�MENU��SWITCH��MODE��������Ϊ��ť������
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;
	}
}

VOID MainViewOnTimer (XMMSG *msg)
{
     static BYTE PreMinute=0;
    XMSYSTEMTIME Currenttime;
     XM_GetLocalTime (&Currenttime);
	 //Currenttime.bDayOfWeek 
      if(PreMinute!=Currenttime.bMinute)
      	{
           PreMinute=Currenttime.bMinute;
	    XM_InvalidateWindow ();
	    XM_UpdateWindow ();
	 }

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




VOID MainViewOnCommand (XMMSG *msg)
{
	MAINVIEWDATA *settingViewData;
	settingViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MainView));
	if(settingViewData == NULL)
		return;
	switch(msg->wp)
	
	{   case MAIN_COMMAND_RETURN:
	           XM_PullWindow (0);
				break;
		case MAIN_COMMAND_VEDIO_PLAY:
		        	// �л���¼��ط�ģʽ
			if(iRecordMode == DEMO_MODE)
			{
				// �޷��л�����ƵԤ��ģʽ��������Ϣ���ڣ���ʾԭ��
				unsigned int card_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
				if(card_state == DEVCAP_SDCARDSTATE_UNPLUG)
				{
					// ���Ѱγ�
					XM_OpenAlertView (	
						AP_ID_CARD_MESSAGE_CARD_WITHDRAW,	// ��Ϣ�ı���ԴID
						AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
						0,
						0,												// ��ť������ԴID
						0,												// ��ť����������ԴID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10�����Զ��ر�
						APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
						NULL,											// �����ص�����
						NULL,											// �û��ص�����˽�в���
						XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
						//XM_VIEW_ALIGN_CENTRE,
						0												// ALERTVIEW��ͼ�Ŀ���ѡ��
						);
				}
				else if(card_state == DEVCAP_SDCARDSTATE_FS_ERROR)
				{
					DWORD dwButtonNormalTextID[2];
					DWORD dwButtonPressedTextID[2];
					dwButtonNormalTextID[0] = AP_ID_ALERT_FORMAT_NORMAL;
					dwButtonNormalTextID[1] = AP_ID_ALERT_CANCEL_NORMAL;
					dwButtonPressedTextID[0] = AP_ID_ALERT_FORMAT_PRESSED;
					dwButtonPressedTextID[1] = AP_ID_ALERT_CANCEL_PRESSED;

					// ���ļ�ϵͳ����
					XM_OpenAlertView (	
						AP_ID_CARD_MESSAGE_CARD_FSERROR,		// ��Ϣ�ı���ԴID
						AP_ID_CARD_ICON_SDCARD,												// ͼƬ��Ϣ��ԴID
						2,												// ��ť����
						dwButtonNormalTextID,					// ��ť������ԴID
						dwButtonPressedTextID,					// ��ť����������ԴID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10�����Զ��ر�
						APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
						fs_access_error_alert_cb_main,							// �����ص�����
						NULL,											// �û��ص�����˽�в���
						XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
						//XM_VIEW_ALIGN_CENTRE,
						XM_ALERTVIEW_OPTION_ENABLE_CALLBACK	// ALERTVIEW��ͼ�Ŀ���ѡ��
						);
				}
				else if(card_state == DEVCAP_SDCARDSTATE_INVALID)
				{
					// ����Ч(���޷�ʶ��)
					XM_OpenAlertView (	
						AP_ID_CARD_MESSAGE_CARD_INVALID,	// ��Ϣ�ı���ԴID
						AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
						0,
						0,												// ��ť������ԴID
						0,												// ��ť����������ԴID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10�����Զ��ر�
						APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
						NULL,											// �����ص�����
						NULL,											// �û��ص�����˽�в���
						XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
						//XM_VIEW_ALIGN_CENTRE,
						0												// ALERTVIEW��ͼ�Ŀ���ѡ��
						);
				}	
				else 
				{
					// �����Ƶ���б��Ƿ�����ɨ����.
					// ������ɨ��, ����һPOP�Ӵ���ʾ�û�"����ɨ����Ƶ�ļ�,���Ե�"
					if(XM_VideoItemIsBasicServiceOpened() && !XM_VideoItemIsServiceOpened())
					{
						XM_OpenAlertView (	
							AP_ID_VIDEO_VIEW_SCAN_VIDEO_FILE,	// ��Ϣ�ı���ԴID
							AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
							0,
							0,												// ��ť������ԴID
							0,												// ��ť����������ԴID
							APP_ALERT_BKGCOLOR,
							60.0,											// 60�����Զ��ر�
							APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
							NULL,											// �����ص�����
							NULL,											// �û��ص�����˽�в���
							XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW��ͼ�Ŀ���ѡ��
							);
						
					}
					else
					{
						// ������(��дģʽ��д����ģʽ)
						APPMarkCardChecking (1);		// ��ֹ��Ƶͨ��
	
						XM_PushWindow (XMHWND_HANDLE(VideoListView));
					}
				}
			}
			else
			{
				// �����Ƶ���б��Ƿ�����ɨ����.
				// ������ɨ��, ����һPOP�Ӵ���ʾ�û�"����ɨ����Ƶ�ļ�,���Ե�"
				if(XM_VideoItemIsBasicServiceOpened() && !XM_VideoItemIsServiceOpened())
				{
					XM_OpenAlertView (	
							AP_ID_VIDEO_VIEW_SCAN_VIDEO_FILE,	// ��Ϣ�ı���ԴID
							AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
							0,
							0,												// ��ť������ԴID
							0,												// ��ť����������ԴID
							APP_ALERT_BKGCOLOR,
							60.0,											// 60�����Զ��ر�
							APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
							NULL,											// �����ص�����
							NULL,											// �û��ص�����˽�в���
							XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW��ͼ�Ŀ���ѡ��
							);
						
				}
				else
				{
					APPMarkCardChecking (1);		// ��ֹ��Ƶͨ��

					//XM_PushWindow (XMHWND_HANDLE(VideoListView));
					XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP);
			              XM_JumpWindowEx (XMHWND_HANDLE(VideoListView), 0, XM_JUMP_POPDEFAULT);
				}
			}

			break;
			

		case MAIN_COMMAND_PHOTO:
			// �л�����¼�����á�����
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP);
			//XM_JumpWindowEx (XMHWND_HANDLE(Photo), 0, XM_JUMP_POPDEFAULT);
			break;
               case MAIN_COMMAND_MENU:
			   	// �л��������á�����
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP);
			XM_JumpWindowEx (XMHWND_HANDLE(SettingView), 0, XM_JUMP_POPDEFAULT);
			break;
			   	
	/*	case SETTING_COMMAND_RETURN:
			// ���ص�����
			XM_PullWindow (0);
			break;*/
	}
}

VOID MainViewOnTouchDown (XMMSG *msg)
{
	int index;
	unsigned int x, y;
	HANDLE hWnd = XMHWND_HANDLE(MainView);
	MAINVIEWDATA *settingViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MainView));
   
	if(settingViewData == NULL)
		return;

	if(AP_TpButtonControlMessageHandler (&settingViewData->TPbtnControl, msg))
		return;
    
    #if 1
	index = AppLocateItem (XMHWND_HANDLE(MainView), settingViewData->nItemCount, APP_POS_ITEM5_LINEHEIGHT, settingViewData->nTopItem, LOWORD(msg->lp), HIWORD(msg->lp));
	if(index < 0)
		return;
	settingViewData->nTouchItem = index;
	if(settingViewData->nCurItem != index)
	{
		settingViewData->nCurItem = index;
		XM_InvalidateWindow ();
		XM_UpdateWindow ();
	}
    #endif
}

VOID MainViewOnTouchUp (XMMSG *msg)
{
	MAINVIEWDATA *settingViewData;
	settingViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MainView));
	if(settingViewData == NULL)
		return;

	if(AP_TpButtonControlMessageHandler (&settingViewData->TPbtnControl, msg))
		return;
	
	//XM_PostMessage (XM_COMMAND, settingViewData->TPbtnControl.pBtnInfo[0].wCommand, settingViewData->nCurItem);
}

VOID MainViewOnTouchMove (XMMSG *msg)
{
	MAINVIEWDATA *settingViewData;
	settingViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemSettingView));
	if(settingViewData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg))
		return;

}

// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (MainView)
	XM_ON_MESSAGE (XM_PAINT, MainViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, MainViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, MainViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, MainViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, MainViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, MainViewOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, MainViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, MainViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, MainViewOnTouchUp)
	XM_ON_MESSAGE (XM_TOUCHMOVE, MainViewOnTouchMove)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, MainView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)


