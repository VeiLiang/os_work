//****************************************************************************
//
//	Copyright (C) 2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_voiceassistantview.c
//	  语音助手设置窗口
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

// 私有命令定义
#define	SETTING_COMMAND_MODIFY		0
#define	SETTING_COMMAND_RETURN		1

// “语音助手设置”窗口按钮控件定义
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
	int				nTopItem;					// 第一个可视的菜单项
	int				nCurItem;					// 当前选择的菜单项
	int				nItemCount;					// 菜单项个数

	APPMENUOPTION	menuOption;					// 菜单选项

	XMBUTTONCONTROL	btnControl;				// 按钮控件

	// 开关控件
	HANDLE			hSwitchControl[OPTION_COUNT];

	BYTE				bTimer;						// 定时器
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
			// 保存菜单设置到物理存储设备
			AP_SaveMenuData (&AppMenuData);
            if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
			    XM_voice_prompts_insert_voice (XM_VOICE_ID_VOICE_PROMPTS_ON);
		}
		else
		{
			AP_SetMenuItem (type, state);
			AP_SetMenuItem (APPMENUITEM_BELL_VOLUME, 0);
			// 保存菜单设置到物理存储设备
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
		// 窗口未建立，第一次进入
	
		// 分配私有数据句柄
		settingViewData = XM_calloc (sizeof(VOICEASSISTANTVIEWDATA));
		if(settingViewData == NULL)
		{
			XM_printf ("settingViewData XM_calloc failed\n");
			// 失败返回到桌面
			XM_PullWindow (0);
			return;
		}

		// 初始化私有数据
		settingViewData->nCurItem = 0;
		settingViewData->nItemCount = OPTION_COUNT + 1;

		pt.x = 0;
		pt.y = 0;
		for (i = 0; i < OPTION_COUNT; i++)
		{
			// Switch开关控件初始化
			settingViewData->hSwitchControl[i] = AP_SwitchButtonControlInit (
				XMHWND_HANDLE(VoiceAssistantView),
				&pt,
				(XMBOOL)AP_GetMenuItem(switch_type[i]),
				setting_switch,
				(void *)(unsigned int)(switch_type[i]));
		}

		// 按钮控件初始化
		AP_ButtonControlInit (&settingViewData->btnControl, SETTINGBTNCOUNT, XMHWND_HANDLE(VoiceAssistantView), settingBtn);

		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(VoiceAssistantView), settingViewData);
	}
	else
	{
		settingViewData = (VOICEASSISTANTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VoiceAssistantView));
		// 窗口已建立，当前窗口从栈中恢复
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
		// 窗口退出，彻底摧毁。
		// 获取私有数据句柄
		VOICEASSISTANTVIEWDATA *settingViewData = (VOICEASSISTANTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VoiceAssistantView));
		// 释放所有分配的资源
		if(settingViewData)
		{
			int i;
			// 按钮控件退出过程
			AP_ButtonControlExit (&settingViewData->btnControl);
			
			for (i = 0; i < OPTION_COUNT; i++)
			{
				if(settingViewData->hSwitchControl[i])
				{
					AP_SwitchButtonControlExit (settingViewData->hSwitchControl[i]);
					settingViewData->hSwitchControl[i] = NULL;
				}
			}

			// 释放私有数据句柄
			XM_free (settingViewData);
		}
		XM_printf ("Setting Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
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
	float scale_factor;		// 水平缩放因子
	APPROMRES *AppRes;
	//XMSYSTEMTIME SystemTime;
	XMCOLOR bkg_color = XM_GetSysColor(XM_COLOR_WINDOW);
	HANDLE hwnd = XMHWND_HANDLE(VoiceAssistantView);
	//XMCOLOR bkg_color = (XM_GetSysColor(XM_COLOR_WINDOW) & ~0xFF000000) | 0x70000000;
	unsigned int old_alpha;

	VOICEASSISTANTVIEWDATA *settingViewData;
	
	XMCOORD menu_name_x, menu_data_x, menu_flag_x;	// 菜单项标题、数值、标识的x坐标

	settingViewData = (VOICEASSISTANTVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(settingViewData == NULL)
		return;

	XM_GetDesktopRect (&rc);

	// 计算水平缩放因子(UI按320X240规格设计)
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);

//	XM_FillRect (XMHWND_HANDLE(VoiceAssistantView), rc.left, rc.top, rc.right, rc.bottom, COLOR2RGB565(RGB(255,0,0)));
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, bkg_color);

	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);


	// --------------------------------------
	//
	// ********* 1 显示标题栏区信息 *********
	//
	// --------------------------------------
	AP_DrawTitlebarControl (hwnd, AP_NULLID, AP_ID_VOICEASSISTANT_TITLE);

	// 显示菜单项水平分割线
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

	// 填充当前选择项背景
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
	// ********* 2 显示菜单项区 *************
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
			// 音量调节
			AppRes = AP_AppRes2RomRes (AP_ID_SETTING_VOLUME_ADJUST);
			XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
			
			rcArrow = rect;
			rcArrow.left = menu_flag_x;
			AppRes = AP_AppRes2RomRes (AP_ID_COMMON_RIGHT_ARROW);
			XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rcArrow, XMGIF_DRAW_POS_LEFTTOP);
		}
		else
		{
			// 显示项目标题
			int i = nItem - 1;
			AppRes = AP_AppRes2RomRes (switch_menu[i]);
			XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
			
			// 显示项目选项
			rect.left = menu_data_x;
			if(AP_GetMenuItem(switch_type[i]))
				AppRes = AP_AppRes2RomRes (AP_ID_SETTING_OPEN);
			else
				AppRes = AP_AppRes2RomRes (AP_ID_SETTING_CLOSE);
			XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

			// 显示选项开关状态
			AP_SwitchButtonControlMove (settingViewData->hSwitchControl[i], menu_flag_x - 51, rect.top - 5);
			AP_SwitchButtonControlMessageHandler (settingViewData->hSwitchControl[i], msg);
		}
		rect.top += APP_POS_ITEM5_LINEHEIGHT;
	}

	// 处理按钮控件的显示。
	// 若存在按钮控件，必须调用AP_ButtonControlMessageHandler执行按钮控件显示
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
			// 向上键
			case VK_AP_UP:
				AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
				settingViewData->nCurItem --;
				if(settingViewData->nCurItem < 0)
				{
					// 聚焦到最后一个
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
				// 刷新
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
				break;

			// 向下键
			case VK_AP_DOWN:
				AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
				settingViewData->nCurItem ++;	
				if(settingViewData->nCurItem >= settingViewData->nItemCount)
				{
					// 聚焦到第一个
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
				// 刷新
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
				break;

			case VK_AP_MENU:
			case VK_AP_SWITCH:
			case VK_AP_MODE:
				// 在"设置"窗口中，MENU、SWITCH及MODE键被定义为按钮操作。
				// 此处将这三个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
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
			// 在"设置"窗口中，MENU、SWITCH及MODE键被定义为按钮操作。
			// 此处将这三个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
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
				// 调节音量
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
			// 返回
			// 返回到桌面
			XM_PullWindow (0);
			break;
	}
}



// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (VoiceAssistantView)
	XM_ON_MESSAGE (XM_PAINT, VoiceAssistantViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, VoiceAssistantViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, VoiceAssistantViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, VoiceAssistantViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, VoiceAssistantViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, VoiceAssistantViewOnCommand)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, VoiceAssistantView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)


