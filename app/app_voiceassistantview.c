//****************************************************************************
//
//	Copyright (C) 2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_voiceassistantview.c
//	  �����������ô���
//
//	Revision history
//
//		2014.04.24	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "xm_app_menudata.h"
#include "app_menuid.h"
#include "app_menuoptionview.h"

// ˽�������
#define	SETTING_COMMAND_MODIFY		0
#define	SETTING_COMMAND_RETURN		1

// �������������á����ڰ�ť�ؼ�����
#define	SETTINGBTNCOUNT	2
static const XMBUTTONINFO settingBtn[SETTINGBTNCOUNT] = {
	{	
		VK_AP_MENU,		SETTING_COMMAND_MODIFY,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_MODIFY
	},
	{	
		VK_AP_MODE,		SETTING_COMMAND_RETURN,	AP_ID_COMMON_MODE,	AP_ID_BUTTON_RETURN
	},
};


#define	OPTION_COUNT	9

static const char switch_type[OPTION_COUNT] = {
	APPMENUITEM_VOICE_PROMPTS,
	APPMENUITEM_VOICEASSISTANT_DRIVE,
	APPMENUITEM_VOICEASSISTANT_BATTERY_ALARM,
	APPMENUITEM_VOICEASSISTANT_CARD_STATUS,
	APPMENUITEM_VOICEASSISTANT_MIC_ONOFF,
	APPMENUITEM_VOICEASSISTANT_REC_ONOFF,
	APPMENUITEM_VOICEASSISTANT_URGENT_RECORD,
	APPMENUITEM_VOICEASSISTANT_STORAGE_SPACE,
	APPMENUITEM_VOICEASSISTANT_NAVIGATE_STATUS
};

static const APPMENUID switch_menu[OPTION_COUNT] = {
	AP_ID_VIDEOSETTING_VOICEPROMPTS,
	AP_ID_VOICEASSISTANT_DRIVE_ALARM,
	AP_ID_VOICEASSISTANT_BATTERY_ALARM,
	AP_ID_VOICEASSISTANT_CARD_STATUS_ALARM,
	AP_ID_VOICEASSISTANT_MIC_STATE_ALARM,
	AP_ID_VOICEASSISTANT_REC_STATE_ALARM,
	AP_ID_VOICEASSISTANT_URGENT_RECORD_ALARM,
	AP_ID_VOICEASSISTANT_STORAGE_ALARM,
	AP_ID_VOICEASSISTANT_NAVIGATE_STATUS_ALARM,
};

typedef struct tagVOICEASSISTANTVIEWDATA {
	int				nTopItem;					// ��һ�����ӵĲ˵���
	int				nCurItem;					// ��ǰѡ��Ĳ˵���
	int				nItemCount;					// �˵������

	APPMENUOPTION	menuOption;					// �˵�ѡ��

	XMBUTTONCONTROL	btnControl;				// ��ť�ؼ�

	// ���ؿؼ�
	HANDLE			hSwitchControl[OPTION_COUNT];

	BYTE				bTimer;						// ��ʱ��
} VOICEASSISTANTVIEWDATA;

static void setting_switch (void *private_data, unsigned int state)
{
	unsigned int type = (unsigned int)private_data;
	if(type == APPMENUITEM_VOICE_PROMPTS)
	{
		if(state == 1)
		{
			AP_SetMenuItem (type, state);
			if(AP_GetMenuItem (APPMENUITEM_BELL_VOLUME) == 0)
			{
				AP_SetMenuItem (APPMENUITEM_BELL_VOLUME, 1);
			}
			// ����˵����õ�����洢�豸
			AP_SaveMenuData (&AppMenuData);
            if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
			    XM_voice_prompts_insert_voice (XM_VOICE_ID_VOICE_PROMPTS_ON);
		}
		else
		{
			AP_SetMenuItem (type, state);
			AP_SetMenuItem (APPMENUITEM_BELL_VOLUME, 0);
			// ����˵����õ�����洢�豸
			AP_SaveMenuData (&AppMenuData);
		}
	}
	else 
	{
		AP_SetMenuItem (type, state);
		AP_SaveMenuData (&AppMenuData);
	}
}

VOID VoiceAssistantViewOnEnter (XMMSG *msg)
{
	VOICEASSISTANTVIEWDATA *settingViewData;
	XMPOINT pt;
	if(msg->wp == 0)
	{
		int i;
		// ����δ��������һ�ν���
	
		// ����˽�����ݾ��
		settingViewData = XM_calloc (sizeof(VOICEASSISTANTVIEWDATA));
		if(settingViewData == NULL)
		{
			XM_printf ("settingViewData XM_calloc failed\n");
			// ʧ�ܷ��ص�����
			XM_PullWindow (0);
			return;
		}

		// ��ʼ��˽������
		settingViewData->nCurItem = 0;
		settingViewData->nItemCount = OPTION_COUNT + 1;

		pt.x = 0;
		pt.y = 0;
		for (i = 0; i < OPTION_COUNT; i++)
		{
			// Switch���ؿؼ���ʼ��
			settingViewData->hSwitchControl[i] = AP_SwitchButtonControlInit (
				XMHWND_HANDLE(VoiceAssistantView),
				&pt,
				(XMBOOL)AP_GetMenuItem(switch_type[i]),
				setting_switch,
				(void *)(unsigned int)(switch_type[i]));
		}

		// ��ť�ؼ���ʼ��
		AP_ButtonControlInit (&settingViewData->btnControl, SETTINGBTNCOUNT, XMHWND_HANDLE(VoiceAssistantView), settingBtn);

		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(VoiceAssistantView), settingViewData);
	}
	else
	{
		settingViewData = (VOICEASSISTANTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VoiceAssistantView));
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("Setting Pull\n");

		if(settingViewData)
		{
			unsigned int state = AP_GetMenuItem(APPMENUITEM_VOICE_PROMPTS);
			if(AP_SwitchButtonControlGetState(settingViewData->hSwitchControl[0]) != state)
			{
				AP_SwitchButtonControlSetState(settingViewData->hSwitchControl[0], (XMBOOL)state);
			}
		}
	}
}

VOID VoiceAssistantViewOnLeave (XMMSG *msg)
{
	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		VOICEASSISTANTVIEWDATA *settingViewData = (VOICEASSISTANTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VoiceAssistantView));
		// �ͷ����з������Դ
		if(settingViewData)
		{
			int i;
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&settingViewData->btnControl);
			
			for (i = 0; i < OPTION_COUNT; i++)
			{
				if(settingViewData->hSwitchControl[i])
				{
					AP_SwitchButtonControlExit (settingViewData->hSwitchControl[i]);
					settingViewData->hSwitchControl[i] = NULL;
				}
			}

			// �ͷ�˽�����ݾ��
			XM_free (settingViewData);
		}
		XM_printf ("Setting Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("Setting Push\n");
	}

	XM_KillTimer (XMTIMER_SETTINGVIEW);
}


VOID VoiceAssistantViewOnPaint (XMMSG *msg)
{
	int nItem, nLastItem;
	XMRECT rc, rect, rcArrow;
	int nVisualCount;
	int i;
	float scale_factor;		// ˮƽ��������
	APPROMRES *AppRes;
	//XMSYSTEMTIME SystemTime;
	XMCOLOR bkg_color = XM_GetSysColor(XM_COLOR_WINDOW);
	HANDLE hwnd = XMHWND_HANDLE(VoiceAssistantView);
	//XMCOLOR bkg_color = (XM_GetSysColor(XM_COLOR_WINDOW) & ~0xFF000000) | 0x70000000;
	unsigned int old_alpha;

	VOICEASSISTANTVIEWDATA *settingViewData;
	
	XMCOORD menu_name_x, menu_data_x, menu_flag_x;	// �˵�����⡢��ֵ����ʶ��x����

	settingViewData = (VOICEASSISTANTVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(settingViewData == NULL)
		return;

	XM_GetDesktopRect (&rc);

	// ����ˮƽ��������(UI��320X240������)
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);

//	XM_FillRect (XMHWND_HANDLE(VoiceAssistantView), rc.left, rc.top, rc.right, rc.bottom, COLOR2RGB565(RGB(255,0,0)));
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, bkg_color);

	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);


	// --------------------------------------
	//
	// ********* 1 ��ʾ����������Ϣ *********
	//
	// --------------------------------------
	AP_DrawTitlebarControl (hwnd, AP_NULLID, AP_ID_VOICEASSISTANT_TITLE);

	// ��ʾ�˵���ˮƽ�ָ���
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMSPLITLINE);
	rect = rc;
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
	for (i = 0; i < nVisualCount; i++)
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

	menu_name_x = (XMCOORD)(APP_POS_ITEM5_MENUNAME_X * scale_factor);
	menu_data_x = (XMCOORD)(APP_POS_ITEM5_MENUDATA_X * scale_factor);
	menu_flag_x = (XMCOORD)(APP_POS_ITEM5_MENUFLAG_X * scale_factor);
	// --------------------------------------
	//
	// ********* 2 ��ʾ�˵����� *************
	//
	// --------------------------------------
	rect = rc;
	rect.top = APP_POS_ITEM5_MENUNAME_Y;
	rcArrow = rc;
	nLastItem = settingViewData->nItemCount;
	if(nLastItem > (settingViewData->nTopItem + nVisualCount))
		nLastItem = (settingViewData->nTopItem + nVisualCount);
	for (nItem = settingViewData->nTopItem; nItem < nLastItem; nItem ++)
	{
		rect.left = menu_name_x;

		if(nItem == 0)
		{
			// ��������
			AppRes = AP_AppRes2RomRes (AP_ID_SETTING_VOLUME_ADJUST);
			XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
			
			rcArrow = rect;
			rcArrow.left = menu_flag_x;
			AppRes = AP_AppRes2RomRes (AP_ID_COMMON_RIGHT_ARROW);
			XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rcArrow, XMGIF_DRAW_POS_LEFTTOP);
		}
		else
		{
			// ��ʾ��Ŀ����
			int i = nItem - 1;
			AppRes = AP_AppRes2RomRes (switch_menu[i]);
			XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
			
			// ��ʾ��Ŀѡ��
			rect.left = menu_data_x;
			if(AP_GetMenuItem(switch_type[i]))
				AppRes = AP_AppRes2RomRes (AP_ID_SETTING_OPEN);
			else
				AppRes = AP_AppRes2RomRes (AP_ID_SETTING_CLOSE);
			XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

			// ��ʾѡ���״̬
			AP_SwitchButtonControlMove (settingViewData->hSwitchControl[i], menu_flag_x - 51, rect.top - 5);
			AP_SwitchButtonControlMessageHandler (settingViewData->hSwitchControl[i], msg);
		}
		rect.top += APP_POS_ITEM5_LINEHEIGHT;
	}

	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
	AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);

	XM_SetWindowAlpha (hwnd, (unsigned char)old_alpha);

}



VOID VoiceAssistantViewOnKeyDown (XMMSG *msg)
{
	VOICEASSISTANTVIEWDATA *settingViewData;
	int nVisualCount;
	XMRECT rc;

	settingViewData = (VOICEASSISTANTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VoiceAssistantView));
	if(settingViewData == NULL)
		return;

	XM_GetWindowRect (XMHWND_HANDLE(VoiceAssistantView), &rc);
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;

	if(msg->message == XM_KEYDOWN)
	{
		switch(msg->wp)
		{
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

VOID VoiceAssistantViewOnKeyUp (XMMSG *msg)
{
	VOICEASSISTANTVIEWDATA *settingViewData;

	settingViewData = (VOICEASSISTANTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VoiceAssistantView));
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


VOID VoiceAssistantViewOnCommand (XMMSG *msg)
{
	VOICEASSISTANTVIEWDATA *settingViewData;
	XMMSG key_msg;

	settingViewData = (VOICEASSISTANTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VoiceAssistantView));
	if(settingViewData == NULL)
		return;
	switch(msg->wp)
	{
		case SETTING_COMMAND_MODIFY:
			if(settingViewData->nCurItem == 0)
			{
				// ��������
				XM_OpenBellSoundVolumeSettingView ();
			}
			else
			{
				key_msg.message = XM_KEYDOWN;
				key_msg.wp = VK_AP_MENU;
				key_msg.lp = 0;

				AP_SwitchButtonControlMessageHandler (
					settingViewData->hSwitchControl[settingViewData->nCurItem - 1], 
					&key_msg);
			}
			break;

		case SETTING_COMMAND_RETURN:
			// ����
			// ���ص�����
			XM_PullWindow (0);
			break;
	}
}



// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (VoiceAssistantView)
	XM_ON_MESSAGE (XM_PAINT, VoiceAssistantViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, VoiceAssistantViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, VoiceAssistantViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, VoiceAssistantViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, VoiceAssistantViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, VoiceAssistantViewOnCommand)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, VoiceAssistantView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)


