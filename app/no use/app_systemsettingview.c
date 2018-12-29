//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_systemsetting.c
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

#define	OSD_AUTO_HIDE_TIMEOUT			10000	

// ϵͳ�Ĳ˵��˳����, 0��ʾ��һ����Ŀ(���)
static typedef enum  {
	SYSTEM_MENU_SDCARDVOLUME = 0,					// "SDCARD����"
	SYSTEM_MENU_SOFTWAREVERSION,					// "����汾"
	SYSTEM_MENU_FORMAT,								// "��ʽ��"
	SYSTEM_MENU_RESTORESETTING,					// "�ָ���������"
#ifdef _XM_SYSTEM_UPDATE_ENABLE_
	SYSTEM_MENU_SYSTEMUPDATE,						// "ϵͳ����"
#endif
	SYSTEM_MENU_COUNT									// ϵͳ��˵�����
} SYSTEMSETTING_MENU_DEFINE;


// ˽�������
#define	SYSTEMSETTING_COMMAND_MODIFY		0
#define	SYSTEMSETTING_COMMAND_SETTING	1
#define	SYSTEMSETTING_COMMAND_RETURN		2

// �����á����ڰ�ť�ؼ�����
#define	SYSTEMSETTINGBTNCOUNT	2
static const XMBUTTONINFO systemSettingBtn_1[SYSTEMSETTINGBTNCOUNT] = {
	 /*
	 {	
		//VK_AP_MENU,		SYSTEMSETTING_COMMAND_MODIFY,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
	 },
	 */
	{	
		VK_AP_MENU,	SYSTEMSETTING_COMMAND_SETTING,	AP_ID_COMMON_MODE,	AP_ID_SYSTEMSETTING_BUTTON_SETTING
	},
	{	
		VK_AP_MODE,		SYSTEMSETTING_COMMAND_RETURN,	AP_ID_COMMON_OK,	AP_ID_BUTTON_RETURN
	},
};




typedef struct tagSYSTEMSETTINGVIEWDATA {
	int					nTopItem;					// ��һ�����ӵĲ˵���
	int					nCurItem;					// ��ǰѡ��Ĳ˵���
	int					nItemCount;					// �˵������
	int					nTouchItem;					// ��ǰ������, -1 ��ʾû��
	AP_PROCESSINFO		ProcessInfo;

	XMBUTTONCONTROL	btnControl;
	XMTITLEBARCONTROL titleControl;				// ����ؼ�

} SYSTEMSETTINGVIEWDATA;

// ��̬�ı䰴ť�ؼ�
static void UpdateButtonControl (void)
{
	/*
	SYSTEMSETTINGVIEWDATA *settingViewData = (SYSTEMSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemSettingView));
	// �رյ�ǰ��ť�ؼ�
	AP_ButtonControlExit (&settingViewData->btnControl);
	// ���ð�ť�ؼ�
	if(settingViewData->nCurItem == 0)
	{
		AP_ButtonControlInit (&settingViewData->btnControl, SYSTEMSETTINGBTNCOUNT, 
			XMHWND_HANDLE(SystemSettingView), systemSettingBtn_1);
	}
	else
	{
		AP_ButtonControlInit (&settingViewData->btnControl, SYSTEMSETTINGBTNCOUNT, 
			XMHWND_HANDLE(SystemSettingView), systemSettingBtn_2);
	}
	// ˢ��
	*/
	XM_InvalidateWindow ();
	XM_UpdateWindow ();
}


VOID SystemSettingViewOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
		// ����δ��������һ�ν���
		SYSTEMSETTINGVIEWDATA *settingViewData;

		// ����˽�����ݾ��
		settingViewData = XM_calloc (sizeof(SYSTEMSETTINGVIEWDATA));
		if(settingViewData == NULL)
		{
			XM_printf ("settingViewData XM_calloc failed\n");
			// ʧ�ܷ��ص������ߴ���
			XM_PullWindow (0);
			return;
		}
		
		settingViewData->nCurItem = 0;
		settingViewData->nItemCount = SYSTEM_MENU_COUNT;
		settingViewData->nTouchItem = -1;
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(SystemSettingView), settingViewData);

		// ��ť�ؼ���ʼ��
		AP_ButtonControlInit (&settingViewData->btnControl, SYSTEMSETTINGBTNCOUNT, 
			XMHWND_HANDLE(SystemSettingView), systemSettingBtn_1);
		// ����ؼ���ʼ��
	//	AP_TitleBarControlInit (&settingViewData->titleControl, XMHWND_HANDLE(SystemSettingView), 
	//													AP_NULLID, AP_ID_SYSTEMSETTING_TITLE);

	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("SystemSettingView Pull\n");
	}

	// ������ʱ�������ڲ˵�������
	// ����x��Ķ�ʱ��
	XM_SetTimer (XMTIMER_SYSTEMSETTING, OSD_AUTO_HIDE_TIMEOUT);
}

VOID SystemSettingViewOnLeave (XMMSG *msg)
{
	// ɾ����ʱ��
	XM_KillTimer (XMTIMER_SYSTEMSETTING);
	
	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		SYSTEMSETTINGVIEWDATA *settingViewData = (SYSTEMSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemSettingView));
		// �ͷ����з������Դ
		if(settingViewData)
		{
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&settingViewData->btnControl);
			// ����ؼ��˳�����
		//	AP_TitleBarControlExit (&settingViewData->titleControl);
			// �ͷ�˽�����ݾ��
			XM_free (settingViewData);
		}
		// ���ô��ڵ�˽�����ݾ��Ϊ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(SystemSettingView), NULL);
		
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("SystemSettingView Push\n");
	}
}



VOID SystemSettingViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect, rcArrow;
	int i;
	float scale_factor;		// ˮƽ��������
	XMCOORD menu_name_x, menu_data_x, menu_flag_x;	// �˵�����⡢��ֵ����ʶ��x����
	unsigned int old_alpha;
	HANDLE hwnd = XMHWND_HANDLE(SystemSettingView);
	SYSTEMSETTINGVIEWDATA *settingViewData = (SYSTEMSETTINGVIEWDATA *)XM_GetWindowPrivateData (hwnd);

	XM_printf ("SystemSettingViewOnPaint \n");
	if(settingViewData == NULL)
		return;

	XM_GetDesktopRect (&rc);
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	// ����ˮƽ��������(UI��320X240������)
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);
	menu_name_x = (XMCOORD)(APP_POS_ITEM5_MENUNAME_X * scale_factor);
	menu_data_x = (XMCOORD)(APP_POS_ITEM5_MENUDATA_X * scale_factor);
	menu_flag_x = (XMCOORD)(APP_POS_ITEM5_MENUFLAG_X * scale_factor);

	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);
	
	// --------------------------------------
	//
	// ********* 1 ��ʾ����������Ϣ *********
	//
	// --------------------------------------
	AP_DrawTitlebarControl (hwnd, AP_NULLID, AP_ID_SYSTEMSETTING_TITLE);
	// �������ؼ�����ʾ��
	// �����ڱ���ؼ����������AP_TitleControlMessageHandlerִ�б���ؼ���ʾ
	//AP_TitleBarControlMessageHandler (&settingViewData->titleControl, msg);

	// ��ʾ�˵���ˮƽ�ָ���
	rect = rc;
	for (i = 0; i < 6; i++)
	{
		rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * scale_factor);
		rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + i * APP_POS_ITEM5_LINEHEIGHT);
		AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	}

	// ��䵱ǰѡ�����
	rect = rc;
	rect.left = 0;
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + settingViewData->nCurItem * APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	
	// --------------------------------------
	//
	// ********* 2 ��ʾ�˵����� *************
	//
	// --------------------------------------
	rect = rc;
	rect.top = APP_POS_ITEM5_MENUNAME_Y;
	for (i = 0; i < SYSTEM_MENU_COUNT; i ++)
	{
		rect.left = menu_name_x;
		switch (i)
		{
			/*
			case 0:	// "��������ͷ��
				AppRes = AP_AppRes2RomRes (AP_ID_SYSTEMSETTING_MANAGECCD);
				XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				break;
			*/

			case SYSTEM_MENU_SDCARDVOLUME:	// "SDCARD����"
				AP_RomImageDrawByMenuID (AP_ID_SYSTEMSETTING_SDCARDVOLUME, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				break;

			case SYSTEM_MENU_FORMAT:	// "��ʽ��"
				AP_RomImageDrawByMenuID (AP_ID_SYSTEMSETTING_FORMAT, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				break;

			case SYSTEM_MENU_RESTORESETTING:	// "�ָ���������"
				AP_RomImageDrawByMenuID (AP_ID_SYSTEMSETTING_RESTORESETTING, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				break;
	
#ifdef _XM_SYSTEM_UPDATE_ENABLE_
			case SYSTEM_MENU_SYSTEMUPDATE:	//"ϵͳ����"
				AP_RomImageDrawByMenuID (AP_ID_SYSTEMSETTING_SYSTEMUPDATE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				break;
#endif

			case SYSTEM_MENU_SOFTWAREVERSION:	// "����汾"
				AP_RomImageDrawByMenuID (AP_ID_SYSTEMSETTING_SOFTWAREVERSION, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				break;
				
		}
		// ��ʾ���ҵı��
		rcArrow = rect;
		rcArrow.left = menu_flag_x;
		AP_RomImageDrawByMenuID (AP_ID_COMMON_RIGHT_ARROW, hwnd, &rcArrow, XMGIF_DRAW_POS_LEFTTOP);
		
		rect.top += APP_POS_ITEM5_LINEHEIGHT;
	}
	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
	AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);

	XM_SetWindowAlpha (hwnd, (unsigned char)old_alpha);
}


VOID SystemSettingViewOnKeyDown (XMMSG *msg)
{
	SYSTEMSETTINGVIEWDATA *settingViewData = (SYSTEMSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemSettingView));
	if(settingViewData == NULL)
		return;
	// ������
	XM_Beep (XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{     
	    case VK_AP_SWITCH:	// �����л���
	         break;
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
			// ��"¼�����"�����У�MENU��Power��Switch��MODE��������Ϊ��ť������
			// �˴������ĸ����¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			XM_SetTimer (XMTIMER_SYSTEMSETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
			
        #if 1
		case VK_AP_UP:		// ����¼���
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			settingViewData->nCurItem --;
			if(settingViewData->nCurItem < 0)
			{
				settingViewData->nCurItem = settingViewData->nItemCount - 1;
			}
			UpdateButtonControl ();
			XM_SetTimer (XMTIMER_SYSTEMSETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;

		case VK_AP_DOWN:	// 
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			settingViewData->nCurItem ++;
			if(settingViewData->nCurItem >= settingViewData->nItemCount)
			{
				settingViewData->nCurItem = 0;
			}
			UpdateButtonControl ();
			XM_SetTimer (XMTIMER_SYSTEMSETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
		#endif
	}

}

VOID SystemSettingViewOnKeyUp (XMMSG *msg)
{
	SYSTEMSETTINGVIEWDATA *settingViewData;
	settingViewData = (SYSTEMSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemSettingView));
	if(settingViewData == NULL)
		return;

	switch(msg->wp)
	{
	    case VK_AP_SWITCH:	// �����л���
	       XM_PostMessage (XM_COMMAND, SYSTEMSETTING_COMMAND_MODIFY, settingViewData->nCurItem);

	           break;
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
		//case VK_AP_SWITCH:
			// ��"ʱ������"�����У�MENU��MODE��������Ϊ��ť������
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;
	}
}

// �ָ�ϵͳ���õ�ϵͳ�¼��ص�����
static void restore_setting_view_system_event_handler (XMMSG *msg, int process_state)
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

VOID SystemSettingViewOnCommand (XMMSG *msg)
{
	SYSTEMSETTINGVIEWDATA *settingViewData;
	settingViewData = (SYSTEMSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemSettingView));
	if(settingViewData == NULL)
		return;

	switch(msg->wp)
	{
		case SYSTEMSETTING_COMMAND_MODIFY:
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
			if(settingViewData->nCurItem == SYSTEM_MENU_SDCARDVOLUME) // "SD������"
			{
				XM_OpenRecycleVideoAlertView (
					AP_ID_SYSTEMSETTING_SDCARDVOLUME_TITLE,		// ѭ��¼��ʱ������趨ֵ
					APP_ALERT_BKGCOLOR,						// ����ɫ
					40.0,											// 10�����Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					XM_VIEW_ALIGN_BOTTOM,
					0
					);
				    settingViewData->nCurItem=-1;
			}
			else if(settingViewData->nCurItem == SYSTEM_MENU_FORMAT) // "��ʽ��"
			{
			    //settingViewData->nCurItem=-1;
				AP_OpenFormatView (1);
			}
			else if(settingViewData->nCurItem == SYSTEM_MENU_RESTORESETTING)// "�ָ���������"
			{
				memset (&settingViewData->ProcessInfo, 0, sizeof(settingViewData->ProcessInfo));
				settingViewData->ProcessInfo.type = AP_PROCESS_VIEW_RESTORESETTING;
				settingViewData->ProcessInfo.Title = AP_ID_SYSTEMSETTING_RESTORESETTING_TITLE;
				settingViewData->ProcessInfo.DispItem[0] = AP_ID_CARD_INFO_RESTORE_INITIAL;
				settingViewData->ProcessInfo.DispItem[1] = AP_ID_CARD_INFO_RESTORE_CONFIRM;
				settingViewData->ProcessInfo.DispItem[2] = AP_ID_CARD_INFO_RESTORE_WORKING;
				settingViewData->ProcessInfo.DispItem[3] = AP_ID_CARD_INFO_RESTORE_SUCCESS;
				settingViewData->ProcessInfo.DispItem[4] = AP_ID_CARD_INFO_RESTORE_FAILURE;
				settingViewData->ProcessInfo.nDispItemNum = 5;
				settingViewData->ProcessInfo.nMaxProgress = 1;
				settingViewData->ProcessInfo.fpStartProcess = StartRestoreProgress;
				settingViewData->ProcessInfo.fpQueryProgress = QueryRestoreProgress;
				settingViewData->ProcessInfo.fpSystemEventCb = restore_setting_view_system_event_handler;
				//settingViewData->nCurItem = -1;	
				XM_PushWindowEx (XMHWND_HANDLE(ProcessView), (DWORD)(&settingViewData->ProcessInfo));
			}
#ifdef _XM_SYSTEM_UPDATE_ENABLE_
			else if(settingViewData->nCurItem == SYSTEM_MENU_SYSTEMUPDATE)//"ϵͳ����"
			{
				AP_OpenSystemUpdateView (1);
			}
#endif	// #ifdef _XM_SYSTEM_UPDATE_ENABLE_
			else // "����汾"
			{
				//AP_OpenMessageViewEx (AP_ID_SYSTEMSETTING_VERSION_TITLE, AP_ID_SYSTEMSETTING_VERSION, 30, 0);
				XM_PushWindowEx (XMHWND_HANDLE(SystemVersionView), (DWORD)0);
				// settingViewData->nCurItem=-1;
			}
			break;

		case SYSTEMSETTING_COMMAND_RETURN:
			// ���ص�����
			XM_PullWindow (0);
			break;

		case SYSTEMSETTING_COMMAND_SETTING:
			// �л��������á�����
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
			XM_JumpWindowEx (XMHWND_HANDLE(SettingView), 0, XM_JUMP_POPDEFAULT);
			break;

	}
}

int AppLocateItem (HANDLE hwnd, int item_count, int item_height, int top_item, int x, int y)
{
	XMRECT rc;
	int index;

	// �б����������/����
	int top = APP_POS_ITEM5_MENUNAME_Y;
	int bottom = APP_POS_ITEM5_MENUNAME_Y + item_count  * item_height;

	if((y < top )|| (y >= bottom))
		return -1;
	// �б���������ұ�
	if((x < 430)|| (x >= 1160))
		return -1;

	XM_GetDesktopRect (&rc);

	index = (y - top) / item_height;//eason
   
	if(index >= item_count )
		return -1;

    XM_Beep (XM_BEEP_KEYBOARD);//eason
      
	// ������
	return index + top_item;
}

#define TP_LEFT     		500
#define TP_WIDTH           600

int AppLocateItem1 (HANDLE hwnd, int item_count, int item_height, int top_item, int x, int y)
{
	static XMRECT rcbuffer[4]={{26,52,290,188},{316,52,580,188},{26,194,290,330},{316,194,580,330}};
	int index;
	
	// �б����������/����
	int top = 52;
	int bottom = 330;

	if((y < top )|| (y >= bottom))
		return -1;
	
	// �б���������ұ�
	if((x < TP_LEFT)|| (x >= TP_LEFT+TP_WIDTH))
		return -1;

    for(index =0;index<4;index++)
    {
       	if (((x-TP_LEFT) >= rcbuffer[index].left) && ((x-TP_LEFT) < rcbuffer[index].right) && (y >rcbuffer[index].top) && (y < rcbuffer[index].bottom))
		{
	         // ������
	         return index + top_item;
	    }
    }
	
    XM_Beep (XM_BEEP_KEYBOARD);
	return -1;
}

static VOID SystemSettingViewOnTimer (XMMSG *msg)
{
	SYSTEMSETTINGVIEWDATA *settingViewData;
	
	settingViewData = (SYSTEMSETTINGVIEWDATA *)XM_GetWindowPrivateData(XMHWND_HANDLE(SystemSettingView));
	if(settingViewData == NULL)
		return;

	// ���ص�����
	XM_PullWindow(XMHWND_HANDLE(Desktop));
}

VOID SystemSettingViewOnTouchDown (XMMSG *msg)
{
	int index;
	SYSTEMSETTINGVIEWDATA *settingViewData = (SYSTEMSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemSettingView));
	if(settingViewData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg))
		return;

	index = AppLocateItem (XMHWND_HANDLE(SystemSettingView), settingViewData->nItemCount, APP_POS_ITEM5_LINEHEIGHT, settingViewData->nTopItem, LOWORD(msg->lp), HIWORD(msg->lp));
	if(index < 0)
		return;

	settingViewData->nTouchItem = index;
	if(settingViewData->nCurItem != index)
	{
		settingViewData->nCurItem = index;
		XM_InvalidateWindow ();
		XM_UpdateWindow ();
		XM_SetTimer (XMTIMER_SYSTEMSETTING, OSD_AUTO_HIDE_TIMEOUT);
	}
}

VOID SystemSettingViewOnTouchUp (XMMSG *msg)
{

	SYSTEMSETTINGVIEWDATA *settingViewData;
	settingViewData = (SYSTEMSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemSettingView));
	if(settingViewData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg))
		return;
	if(settingViewData->nTouchItem == (-1))
		return;
	settingViewData->nTouchItem = -1;
	XM_PostMessage (XM_COMMAND, SYSTEMSETTING_COMMAND_MODIFY, settingViewData->nCurItem);
}

VOID SystemSettingViewOnTouchMove (XMMSG *msg)
{
	SYSTEMSETTINGVIEWDATA *settingViewData;
	
	settingViewData = (SYSTEMSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemSettingView));
	if(settingViewData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg))
		return;

}

// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (SystemSettingView)
	XM_ON_MESSAGE (XM_PAINT, SystemSettingViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, SystemSettingViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, SystemSettingViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, SystemSettingViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, SystemSettingViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, SystemSettingViewOnCommand)
	XM_ON_MESSAGE (XM_TIMER, SystemSettingViewOnTimer)
	XM_ON_MESSAGE (XM_TOUCHDOWN, SystemSettingViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, SystemSettingViewOnTouchUp)
	XM_ON_MESSAGE (XM_TOUCHMOVE, SystemSettingViewOnTouchMove)

XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, SystemSettingView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)

