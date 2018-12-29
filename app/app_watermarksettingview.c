//****************************************************************************
//
//	Copyright (C) 2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_watersettingsettingview.c
//	  ��Ƶˮӡ����
//
//	Revision history
//
//		2014.03.17	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"
#include "xm_app_menudata.h"

// ˽�������
#define	WATERMARKSETTINGVIEW_COMMAND_MODIFY		0
#define	WATERMARKSETTINGVIEW_COMMAND_SYSTEM		1
#define	WATERMARKSETTINGVIEW_COMMAND_RETURN		2

// �����á����ڰ�ť�ؼ�����
#define	WATERMARKSETTINGVIEWBTNCOUNT	3
static const XMBUTTONINFO videoRecordBtn[WATERMARKSETTINGVIEWBTNCOUNT] = {
	{	
		VK_AP_MENU,		WATERMARKSETTINGVIEW_COMMAND_MODIFY,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_MODIFY
	},
	{	
		VK_AP_SWITCH,	WATERMARKSETTINGVIEW_COMMAND_SYSTEM,	AP_ID_COMMON_SWITCH,	AP_ID_VIDEOSETTING_BUTTON_SYSTEM
	},
	{	
		VK_AP_MODE,		WATERMARKSETTINGVIEW_COMMAND_RETURN,	AP_ID_COMMON_MODE,	AP_ID_BUTTON_RETURN
	},
};

typedef struct tagWATERMARKSETTINGVIEWDATA {
	int					nTopItem;					// ��һ�����ӵĲ˵���
	int					nCurItem;					// ��ǰѡ��Ĳ˵���
	int					nItemCount;					// �˵������

	APPMENUOPTION	menuOption;					// �˵�ѡ��
	XMBUTTONCONTROL	btnControl;

	// ���ؿؼ�
	HANDLE			hTimeMarkSwitchControl;		// ʱ��ˮӡ����
	HANDLE			hNaviMarkSwitchControl;		// ����ˮӡ����
	HANDLE			hFlagMarkSwitchControl;		// ��־ˮӡ����


} WATERMARKSETTINGVIEWDATA;

static void watermark_setting_switch (void *private_data, unsigned int state)
{
	*(u8_t *)private_data = (u8_t)state;
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
}

VOID WaterMarkSettingViewOnEnter (XMMSG *msg)
{
	XMPOINT pt;
	if(msg->wp == 0)
	{
		// ����δ��������һ�ν���
		WATERMARKSETTINGVIEWDATA *settingViewData;

		// ����˽�����ݾ��
		settingViewData = XM_calloc (sizeof(WATERMARKSETTINGVIEWDATA));
		if(settingViewData == NULL)
		{
			XM_printf ("settingViewData XM_calloc failed\n");
			// ʧ�ܷ��ص������ߴ���
			XM_PullWindow (0);
			return;
		}
		
		// ��ʼ��˽������
		settingViewData->nCurItem = 0;
		settingViewData->nTopItem = 0;

		settingViewData->nItemCount = APP_WATERMARKSETTING_ITEM_COUNT;

		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(WaterMarkSettingView), settingViewData);

		pt.x = 0;
		pt.y = 0;

		// Switch���ؿؼ���ʼ�� (ʱ��ˮӡ)
		settingViewData->hTimeMarkSwitchControl = AP_SwitchButtonControlInit (
			XMHWND_HANDLE(WaterMarkSettingView),
			&pt,
			AppMenuData.time_stamp,
			watermark_setting_switch,
			&AppMenuData.time_stamp);

		// Switch���ؿؼ���ʼ�� (����ˮӡ)
		settingViewData->hNaviMarkSwitchControl = AP_SwitchButtonControlInit (
			XMHWND_HANDLE(WaterMarkSettingView),
			&pt,
			AppMenuData.navi_stamp,
			watermark_setting_switch,
			&AppMenuData.navi_stamp);

		// Switch���ؿؼ���ʼ�� (��־ˮӡ)
		settingViewData->hFlagMarkSwitchControl = AP_SwitchButtonControlInit (
			XMHWND_HANDLE(WaterMarkSettingView),
			&pt,
			AppMenuData.flag_stamp,
			watermark_setting_switch,
			&AppMenuData.flag_stamp);

		// ��ť�ؼ���ʼ��
		AP_ButtonControlInit (&settingViewData->btnControl, WATERMARKSETTINGVIEWBTNCOUNT, 
			XMHWND_HANDLE(WaterMarkSettingView), videoRecordBtn);

	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("WaterMarkSettingView Pull\n");
	}
}

VOID WaterMarkSettingViewOnLeave (XMMSG *msg)
{
	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		WATERMARKSETTINGVIEWDATA *settingViewData = (WATERMARKSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(WaterMarkSettingView));
		// �ͷ����з������Դ
		if(settingViewData)
		{
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&settingViewData->btnControl);

			if(settingViewData->hTimeMarkSwitchControl)
				AP_SwitchButtonControlExit (settingViewData->hTimeMarkSwitchControl);
			if(settingViewData->hNaviMarkSwitchControl)
				AP_SwitchButtonControlExit (settingViewData->hNaviMarkSwitchControl);
			if(settingViewData->hFlagMarkSwitchControl)
				AP_SwitchButtonControlExit (settingViewData->hFlagMarkSwitchControl);

			// �ͷ�˽�����ݾ��
			XM_free (settingViewData);
		}
		XM_printf ("WaterMarkSettingView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("WaterMarkSettingView Push\n");
	}
}



VOID WaterMarkSettingViewOnPaint (XMMSG *msg)
{
	int nItem, nLastItem;
	XMRECT rc, rect, rcArrow;
	int i;
	APPROMRES *AppRes;
	float scale_factor;		// ˮƽ��������
	unsigned int old_alpha;

	XMCOORD menu_name_x, menu_data_x, menu_flag_x;	// �˵�����⡢��ֵ����ʶ��x����
	HANDLE hwnd = XMHWND_HANDLE(WaterMarkSettingView);

	WATERMARKSETTINGVIEWDATA *settingViewData = (WATERMARKSETTINGVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(settingViewData == NULL)
		return;

	XM_GetDesktopRect (&rc);

	// ����ˮƽ��������(UI��320X240������)
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);

	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_WINDOW));
	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);

	menu_name_x = (XMCOORD)(APP_POS_ITEM5_MENUNAME_X * scale_factor);
	menu_data_x = (XMCOORD)(APP_POS_ITEM5_MENUDATA_X * scale_factor);
	menu_flag_x = (XMCOORD)(APP_POS_ITEM5_MENUFLAG_X * scale_factor);
	// --------------------------------------
	//
	// ********* 1 ��ʾ����������Ϣ *********
	//
	// --------------------------------------
	AP_DrawTitlebarControl (hwnd, AP_NULLID, AP_ID_WATERMARKSETTING_TITLE);

	// ��ʾ�˵���ˮƽ�ָ���
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMSPLITLINE);
	rect = rc;
	for (i = 0; i < 5; i++)
	{
		rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * scale_factor);
		rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + i * APP_POS_ITEM5_LINEHEIGHT);
		XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	}

	// ��䵱ǰѡ�����
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMBACKGROUND);
	rect = rc;
	rect.left = 0;
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (settingViewData->nCurItem - settingViewData->nTopItem)  * APP_POS_ITEM5_LINEHEIGHT);
	XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	// --------------------------------------
	//
	// ********* 2 ��ʾ�˵����� *************
	//
	// --------------------------------------
	rect = rc;
	rect.top = APP_POS_ITEM5_MENUNAME_Y;
	nLastItem = settingViewData->nItemCount;
	if(nLastItem > (settingViewData->nTopItem + 5))
		nLastItem = (settingViewData->nTopItem + 5);
	for (nItem = settingViewData->nTopItem; nItem < nLastItem; nItem ++)
	{
		rect.left = menu_name_x;
		rcArrow = rect;
		switch (nItem)
		{
			case 0:	// "ʱ��ˮӡ"
				AppRes = AP_AppRes2RomRes (AP_ID_WATERMARKSETTING_TIME);
				XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				rect.left = menu_data_x;
				if(AppMenuData.time_stamp== 1)
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_OPEN);
				else
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_CLOSE);

				XM_DrawPngImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

				AP_SwitchButtonControlMove (settingViewData->hTimeMarkSwitchControl, menu_flag_x - 51, rect.top - 5);
				AP_SwitchButtonControlMessageHandler (settingViewData->hTimeMarkSwitchControl, msg);

				break;

			case 1:	// "GPSˮӡ"
				AppRes = AP_AppRes2RomRes (AP_ID_WATERMARKSETTING_GPS);
				XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				rect.left = menu_data_x;
				if(AppMenuData.navi_stamp == 1)
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_OPEN);
				else
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_CLOSE);

				XM_DrawPngImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

				AP_SwitchButtonControlMove (settingViewData->hNaviMarkSwitchControl, menu_flag_x - 51, rect.top - 5);
				AP_SwitchButtonControlMessageHandler (settingViewData->hNaviMarkSwitchControl, msg);
				break;

			case 2:	// "LOGOˮӡ"
				AppRes = AP_AppRes2RomRes (AP_ID_WATERMARKSETTING_LOGO);
				XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				rect.left = menu_data_x;
				if(AppMenuData.flag_stamp == 1)
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_OPEN);
				else
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_CLOSE);

				XM_DrawPngImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

				AP_SwitchButtonControlMove (settingViewData->hFlagMarkSwitchControl, menu_flag_x - 51, rect.top - 5);
				AP_SwitchButtonControlMessageHandler (settingViewData->hFlagMarkSwitchControl, msg);
				break;
	
			case 3:	// "LOGOˮӡ����"
				AppRes = AP_AppRes2RomRes (AP_ID_WATERMARKSETTING_LOGO_LOAD);
				XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				rect.left = menu_data_x;
				if(AppMenuData.time_stamp== 1)
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_OPEN);
				else
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_CLOSE);

				//XM_DrawPngImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(WaterMarkSettingView), &rect, XMGIF_DRAW_POS_LEFTTOP);
				
				// ��ʾ���ҵı��
				rcArrow = rect;
				rcArrow.left = menu_flag_x;
				AppRes = AP_AppRes2RomRes (AP_ID_COMMON_RIGHT_ARROW);
				XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rcArrow, XMGIF_DRAW_POS_LEFTTOP);

				break;
				
		}
			
		
		rect.top += APP_POS_ITEM5_LINEHEIGHT;
	}
	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
	AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);

	XM_SetWindowAlpha (hwnd, (unsigned char)old_alpha);

}


VOID WaterMarkSettingViewOnKeyDown (XMMSG *msg)
{
	WATERMARKSETTINGVIEWDATA *settingViewData = (WATERMARKSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(WaterMarkSettingView));
	if(settingViewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
		case VK_AP_SWITCH:	// �����л���
			// ��"¼�����"�����У�MENU��Power��Switch��MODE��������Ϊ��ť������
			// �˴������ĸ����¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;

		case VK_AP_UP:		// ����¼���
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			settingViewData->nCurItem --;
			if(settingViewData->nCurItem < 0)
			{
				// �۽������һ��
				settingViewData->nCurItem = (WORD)(settingViewData->nItemCount - 1);
				settingViewData->nTopItem = 0;
				while ( (settingViewData->nCurItem - settingViewData->nTopItem) >= APP_MENUOPTIONVIEW_ITEM_COUNT )
				{
					settingViewData->nTopItem ++;
				}
			}
			else
			{
				if(settingViewData->nTopItem > settingViewData->nCurItem)
					settingViewData->nTopItem = settingViewData->nCurItem;
			}

			/*
			if (settingViewData->nCurItem)
			{
				settingViewData->nCurItem--;
			}
			else if (settingViewData->nTopItem)
			{
				settingViewData->nTopItem--;
			}*/

			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_DOWN:	// 
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
				while ( (settingViewData->nCurItem - settingViewData->nTopItem) >= APP_MENUOPTIONVIEW_ITEM_COUNT )
				{
					settingViewData->nTopItem ++;
				}
			}
			
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();

			/*
			if ((settingViewData->nCurItem + settingViewData->nTopItem) < (settingViewData->nItemCount- 1))
			{
				if (settingViewData->nCurItem < (APP_MENUOPTIONVIEW_ITEM_COUNT-1))
				{
					settingViewData->nCurItem ++;
				}
				else
				{
					settingViewData->nTopItem++;
				}

				// ˢ��
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}*/

			break;
	}

}

VOID WaterMarkSettingViewOnKeyUp (XMMSG *msg)
{
	WATERMARKSETTINGVIEWDATA *settingViewData;
	settingViewData = (WATERMARKSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(WaterMarkSettingView));
	if(settingViewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
		case VK_AP_SWITCH:
			// ��"ʱ������"�����У�MENU��MODE��������Ϊ��ť������
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;
	}
}

// MIC�����ر�
static const DWORD dwOnOffMenuOption[2] = {
	AP_ID_SETTING_CLOSE,
	AP_ID_SETTING_OPEN
};


// "ʱ��ˮӡ"
/*
static void TimeStampMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	// AppMenuData.time_stamp = (BYTE)dMenuOptionSelect;
	if(lpUserData)
	{
		*(BYTE *)lpUserData = (BYTE)dMenuOptionSelect;
		// ����˵����õ�����洢�豸
		AP_SaveMenuData (&AppMenuData);
	}
}*/




VOID WaterMarkSettingViewOnCommand (XMMSG *msg)
{
	WATERMARKSETTINGVIEWDATA *settingViewData;
	int	SelectedItem;
	settingViewData = (WATERMARKSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(WaterMarkSettingView));
	if(settingViewData == NULL)
		return;

	SelectedItem = settingViewData->nCurItem;// + settingViewData->nTopItem;

	switch(msg->wp)
	{
		case WATERMARKSETTINGVIEW_COMMAND_MODIFY:

			if(SelectedItem == 0)
			{
				// "ʱ��ˮӡ"
				XMMSG key_msg;
				key_msg.message = XM_KEYDOWN;
				key_msg.wp = VK_AP_MENU;
				key_msg.lp = 0;

				AP_SwitchButtonControlMessageHandler (settingViewData->hTimeMarkSwitchControl, &key_msg);
			}
			else if(SelectedItem == 1)
			{
				// "GPS/��������ˮӡ"
				XMMSG key_msg;
				key_msg.message = XM_KEYDOWN;
				key_msg.wp = VK_AP_MENU;
				key_msg.lp = 0;

				AP_SwitchButtonControlMessageHandler (settingViewData->hNaviMarkSwitchControl, &key_msg);
			}
			else if(SelectedItem == 2)
			{
				// "��־ˮӡ"
				XMMSG key_msg;
				key_msg.message = XM_KEYDOWN;
				key_msg.wp = VK_AP_MENU;
				key_msg.lp = 0;

				AP_SwitchButtonControlMessageHandler (settingViewData->hFlagMarkSwitchControl, &key_msg);
			}
			else if(SelectedItem == 3)
			{
				// ����־ˮӡ������
				XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				// �л�����ϵͳ������
				XM_PushWindowEx (XMHWND_HANDLE(WaterMarkLogoLoadView), 0);
			}

			
			break;


		case WATERMARKSETTINGVIEW_COMMAND_RETURN:
			// ���ص�����
			XM_PullWindow (0);
			break;

		case WATERMARKSETTINGVIEW_COMMAND_SYSTEM:
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
			// �л�����ϵͳ������
			XM_JumpWindowEx (XMHWND_HANDLE(SystemSettingView), 0, XM_JUMP_POPDEFAULT);
			break;

	}
}

// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (WaterMarkSettingView)
	XM_ON_MESSAGE (XM_PAINT, WaterMarkSettingViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, WaterMarkSettingViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, WaterMarkSettingViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, WaterMarkSettingViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, WaterMarkSettingViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, WaterMarkSettingViewOnCommand)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, WaterMarkSettingView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
