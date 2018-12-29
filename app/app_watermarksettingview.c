//****************************************************************************
//
//	Copyright (C) 2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_watersettingsettingview.c
//	  视频水印设置
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

// 私有命令定义
#define	WATERMARKSETTINGVIEW_COMMAND_MODIFY		0
#define	WATERMARKSETTINGVIEW_COMMAND_SYSTEM		1
#define	WATERMARKSETTINGVIEW_COMMAND_RETURN		2

// “设置”窗口按钮控件定义
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
	int					nTopItem;					// 第一个可视的菜单项
	int					nCurItem;					// 当前选择的菜单项
	int					nItemCount;					// 菜单项个数

	APPMENUOPTION	menuOption;					// 菜单选项
	XMBUTTONCONTROL	btnControl;

	// 开关控件
	HANDLE			hTimeMarkSwitchControl;		// 时间水印开关
	HANDLE			hNaviMarkSwitchControl;		// 导航水印开关
	HANDLE			hFlagMarkSwitchControl;		// 标志水印开关


} WATERMARKSETTINGVIEWDATA;

static void watermark_setting_switch (void *private_data, unsigned int state)
{
	*(u8_t *)private_data = (u8_t)state;
	// 保存菜单设置到物理存储设备
	AP_SaveMenuData (&AppMenuData);
}

VOID WaterMarkSettingViewOnEnter (XMMSG *msg)
{
	XMPOINT pt;
	if(msg->wp == 0)
	{
		// 窗口未建立，第一次进入
		WATERMARKSETTINGVIEWDATA *settingViewData;

		// 分配私有数据句柄
		settingViewData = XM_calloc (sizeof(WATERMARKSETTINGVIEWDATA));
		if(settingViewData == NULL)
		{
			XM_printf ("settingViewData XM_calloc failed\n");
			// 失败返回到调用者窗口
			XM_PullWindow (0);
			return;
		}
		
		// 初始化私有数据
		settingViewData->nCurItem = 0;
		settingViewData->nTopItem = 0;

		settingViewData->nItemCount = APP_WATERMARKSETTING_ITEM_COUNT;

		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(WaterMarkSettingView), settingViewData);

		pt.x = 0;
		pt.y = 0;

		// Switch开关控件初始化 (时间水印)
		settingViewData->hTimeMarkSwitchControl = AP_SwitchButtonControlInit (
			XMHWND_HANDLE(WaterMarkSettingView),
			&pt,
			AppMenuData.time_stamp,
			watermark_setting_switch,
			&AppMenuData.time_stamp);

		// Switch开关控件初始化 (导航水印)
		settingViewData->hNaviMarkSwitchControl = AP_SwitchButtonControlInit (
			XMHWND_HANDLE(WaterMarkSettingView),
			&pt,
			AppMenuData.navi_stamp,
			watermark_setting_switch,
			&AppMenuData.navi_stamp);

		// Switch开关控件初始化 (标志水印)
		settingViewData->hFlagMarkSwitchControl = AP_SwitchButtonControlInit (
			XMHWND_HANDLE(WaterMarkSettingView),
			&pt,
			AppMenuData.flag_stamp,
			watermark_setting_switch,
			&AppMenuData.flag_stamp);

		// 按钮控件初始化
		AP_ButtonControlInit (&settingViewData->btnControl, WATERMARKSETTINGVIEWBTNCOUNT, 
			XMHWND_HANDLE(WaterMarkSettingView), videoRecordBtn);

	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("WaterMarkSettingView Pull\n");
	}
}

VOID WaterMarkSettingViewOnLeave (XMMSG *msg)
{
	if (msg->wp == 0)
	{
		// 窗口退出，彻底摧毁。
		// 获取私有数据句柄
		WATERMARKSETTINGVIEWDATA *settingViewData = (WATERMARKSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(WaterMarkSettingView));
		// 释放所有分配的资源
		if(settingViewData)
		{
			// 按钮控件退出过程
			AP_ButtonControlExit (&settingViewData->btnControl);

			if(settingViewData->hTimeMarkSwitchControl)
				AP_SwitchButtonControlExit (settingViewData->hTimeMarkSwitchControl);
			if(settingViewData->hNaviMarkSwitchControl)
				AP_SwitchButtonControlExit (settingViewData->hNaviMarkSwitchControl);
			if(settingViewData->hFlagMarkSwitchControl)
				AP_SwitchButtonControlExit (settingViewData->hFlagMarkSwitchControl);

			// 释放私有数据句柄
			XM_free (settingViewData);
		}
		XM_printf ("WaterMarkSettingView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("WaterMarkSettingView Push\n");
	}
}



VOID WaterMarkSettingViewOnPaint (XMMSG *msg)
{
	int nItem, nLastItem;
	XMRECT rc, rect, rcArrow;
	int i;
	APPROMRES *AppRes;
	float scale_factor;		// 水平缩放因子
	unsigned int old_alpha;

	XMCOORD menu_name_x, menu_data_x, menu_flag_x;	// 菜单项标题、数值、标识的x坐标
	HANDLE hwnd = XMHWND_HANDLE(WaterMarkSettingView);

	WATERMARKSETTINGVIEWDATA *settingViewData = (WATERMARKSETTINGVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(settingViewData == NULL)
		return;

	XM_GetDesktopRect (&rc);

	// 计算水平缩放因子(UI按320X240规格设计)
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);

	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_WINDOW));
	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);

	menu_name_x = (XMCOORD)(APP_POS_ITEM5_MENUNAME_X * scale_factor);
	menu_data_x = (XMCOORD)(APP_POS_ITEM5_MENUDATA_X * scale_factor);
	menu_flag_x = (XMCOORD)(APP_POS_ITEM5_MENUFLAG_X * scale_factor);
	// --------------------------------------
	//
	// ********* 1 显示标题栏区信息 *********
	//
	// --------------------------------------
	AP_DrawTitlebarControl (hwnd, AP_NULLID, AP_ID_WATERMARKSETTING_TITLE);

	// 显示菜单项水平分割线
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMSPLITLINE);
	rect = rc;
	for (i = 0; i < 5; i++)
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

	// --------------------------------------
	//
	// ********* 2 显示菜单项区 *************
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
			case 0:	// "时间水印"
				AppRes = AP_AppRes2RomRes (AP_ID_WATERMARKSETTING_TIME);
				XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// 显示选项
				rect.left = menu_data_x;
				if(AppMenuData.time_stamp== 1)
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_OPEN);
				else
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_CLOSE);

				XM_DrawPngImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

				AP_SwitchButtonControlMove (settingViewData->hTimeMarkSwitchControl, menu_flag_x - 51, rect.top - 5);
				AP_SwitchButtonControlMessageHandler (settingViewData->hTimeMarkSwitchControl, msg);

				break;

			case 1:	// "GPS水印"
				AppRes = AP_AppRes2RomRes (AP_ID_WATERMARKSETTING_GPS);
				XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// 显示选项
				rect.left = menu_data_x;
				if(AppMenuData.navi_stamp == 1)
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_OPEN);
				else
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_CLOSE);

				XM_DrawPngImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

				AP_SwitchButtonControlMove (settingViewData->hNaviMarkSwitchControl, menu_flag_x - 51, rect.top - 5);
				AP_SwitchButtonControlMessageHandler (settingViewData->hNaviMarkSwitchControl, msg);
				break;

			case 2:	// "LOGO水印"
				AppRes = AP_AppRes2RomRes (AP_ID_WATERMARKSETTING_LOGO);
				XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// 显示选项
				rect.left = menu_data_x;
				if(AppMenuData.flag_stamp == 1)
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_OPEN);
				else
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_CLOSE);

				XM_DrawPngImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

				AP_SwitchButtonControlMove (settingViewData->hFlagMarkSwitchControl, menu_flag_x - 51, rect.top - 5);
				AP_SwitchButtonControlMessageHandler (settingViewData->hFlagMarkSwitchControl, msg);
				break;
	
			case 3:	// "LOGO水印载入"
				AppRes = AP_AppRes2RomRes (AP_ID_WATERMARKSETTING_LOGO_LOAD);
				XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// 显示选项
				rect.left = menu_data_x;
				if(AppMenuData.time_stamp== 1)
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_OPEN);
				else
					AppRes = AP_AppRes2RomRes (AP_ID_SETTING_CLOSE);

				//XM_DrawPngImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(WaterMarkSettingView), &rect, XMGIF_DRAW_POS_LEFTTOP);
				
				// 显示向右的标记
				rcArrow = rect;
				rcArrow.left = menu_flag_x;
				AppRes = AP_AppRes2RomRes (AP_ID_COMMON_RIGHT_ARROW);
				XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rcArrow, XMGIF_DRAW_POS_LEFTTOP);

				break;
				
		}
			
		
		rect.top += APP_POS_ITEM5_LINEHEIGHT;
	}
	// 处理按钮控件的显示。
	// 若存在按钮控件，必须调用AP_ButtonControlMessageHandler执行按钮控件显示
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
		case VK_AP_MENU:		// 菜单键
		case VK_AP_MODE:		// 切换到“行车记录”状态
		case VK_AP_SWITCH:	// 画面切换键
			// 在"录像浏览"窗口中，MENU、Power、Switch及MODE键被定义为按钮操作。
			// 此处将这四个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;

		case VK_AP_UP:		// 紧急录像键
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			settingViewData->nCurItem --;
			if(settingViewData->nCurItem < 0)
			{
				// 聚焦到最后一个
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

			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_DOWN:	// 
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
				while ( (settingViewData->nCurItem - settingViewData->nTopItem) >= APP_MENUOPTIONVIEW_ITEM_COUNT )
				{
					settingViewData->nTopItem ++;
				}
			}
			
			// 刷新
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

				// 刷新
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
		case VK_AP_MENU:		// 菜单键
		case VK_AP_MODE:		// 切换到“行车记录”状态
		case VK_AP_SWITCH:
			// 在"时间设置"窗口中，MENU及MODE键被定义为按钮操作。
			// 此处将这两个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;
	}
}

// MIC开启关闭
static const DWORD dwOnOffMenuOption[2] = {
	AP_ID_SETTING_CLOSE,
	AP_ID_SETTING_OPEN
};


// "时间水印"
/*
static void TimeStampMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	// AppMenuData.time_stamp = (BYTE)dMenuOptionSelect;
	if(lpUserData)
	{
		*(BYTE *)lpUserData = (BYTE)dMenuOptionSelect;
		// 保存菜单设置到物理存储设备
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
				// "时间水印"
				XMMSG key_msg;
				key_msg.message = XM_KEYDOWN;
				key_msg.wp = VK_AP_MENU;
				key_msg.lp = 0;

				AP_SwitchButtonControlMessageHandler (settingViewData->hTimeMarkSwitchControl, &key_msg);
			}
			else if(SelectedItem == 1)
			{
				// "GPS/北斗导航水印"
				XMMSG key_msg;
				key_msg.message = XM_KEYDOWN;
				key_msg.wp = VK_AP_MENU;
				key_msg.lp = 0;

				AP_SwitchButtonControlMessageHandler (settingViewData->hNaviMarkSwitchControl, &key_msg);
			}
			else if(SelectedItem == 2)
			{
				// "标志水印"
				XMMSG key_msg;
				key_msg.message = XM_KEYDOWN;
				key_msg.wp = VK_AP_MENU;
				key_msg.lp = 0;

				AP_SwitchButtonControlMessageHandler (settingViewData->hFlagMarkSwitchControl, &key_msg);
			}
			else if(SelectedItem == 3)
			{
				// “标志水印”载入
				XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				// 切换到“系统”窗口
				XM_PushWindowEx (XMHWND_HANDLE(WaterMarkLogoLoadView), 0);
			}

			
			break;


		case WATERMARKSETTINGVIEW_COMMAND_RETURN:
			// 返回到桌面
			XM_PullWindow (0);
			break;

		case WATERMARKSETTINGVIEW_COMMAND_SYSTEM:
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
			// 切换到“系统”窗口
			XM_JumpWindowEx (XMHWND_HANDLE(SystemSettingView), 0, XM_JUMP_POPDEFAULT);
			break;

	}
}

// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (WaterMarkSettingView)
	XM_ON_MESSAGE (XM_PAINT, WaterMarkSettingViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, WaterMarkSettingViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, WaterMarkSettingViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, WaterMarkSettingViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, WaterMarkSettingViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, WaterMarkSettingViewOnCommand)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, WaterMarkSettingView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
