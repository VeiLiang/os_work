//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_systemsetting.c
//	  系统设置
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

// 系统的菜单项及顺序定义, 0表示第一个项目(最顶部)
static typedef enum  {
	SYSTEM_MENU_SDCARDVOLUME = 0,					// "SDCARD容量"
	SYSTEM_MENU_SOFTWAREVERSION,					// "软件版本"
	SYSTEM_MENU_FORMAT,								// "格式化"
	SYSTEM_MENU_RESTORESETTING,					// "恢复出厂设置"
#ifdef _XM_SYSTEM_UPDATE_ENABLE_
	SYSTEM_MENU_SYSTEMUPDATE,						// "系统升级"
#endif
	SYSTEM_MENU_COUNT									// 系统项菜单个数
} SYSTEMSETTING_MENU_DEFINE;


// 私有命令定义
#define	SYSTEMSETTING_COMMAND_MODIFY		0
#define	SYSTEMSETTING_COMMAND_SETTING	1
#define	SYSTEMSETTING_COMMAND_RETURN		2

// “设置”窗口按钮控件定义
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
	int					nTopItem;					// 第一个可视的菜单项
	int					nCurItem;					// 当前选择的菜单项
	int					nItemCount;					// 菜单项个数
	int					nTouchItem;					// 当前触摸项, -1 表示没有
	AP_PROCESSINFO		ProcessInfo;

	XMBUTTONCONTROL	btnControl;
	XMTITLEBARCONTROL titleControl;				// 标题控件

} SYSTEMSETTINGVIEWDATA;

// 动态改变按钮控件
static void UpdateButtonControl (void)
{
	/*
	SYSTEMSETTINGVIEWDATA *settingViewData = (SYSTEMSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemSettingView));
	// 关闭当前按钮控件
	AP_ButtonControlExit (&settingViewData->btnControl);
	// 重置按钮控件
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
	// 刷新
	*/
	XM_InvalidateWindow ();
	XM_UpdateWindow ();
}


VOID SystemSettingViewOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
		// 窗口未建立，第一次进入
		SYSTEMSETTINGVIEWDATA *settingViewData;

		// 分配私有数据句柄
		settingViewData = XM_calloc (sizeof(SYSTEMSETTINGVIEWDATA));
		if(settingViewData == NULL)
		{
			XM_printf ("settingViewData XM_calloc failed\n");
			// 失败返回到调用者窗口
			XM_PullWindow (0);
			return;
		}
		
		settingViewData->nCurItem = 0;
		settingViewData->nItemCount = SYSTEM_MENU_COUNT;
		settingViewData->nTouchItem = -1;
		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(SystemSettingView), settingViewData);

		// 按钮控件初始化
		AP_ButtonControlInit (&settingViewData->btnControl, SYSTEMSETTINGBTNCOUNT, 
			XMHWND_HANDLE(SystemSettingView), systemSettingBtn_1);
		// 标题控件初始化
	//	AP_TitleBarControlInit (&settingViewData->titleControl, XMHWND_HANDLE(SystemSettingView), 
	//													AP_NULLID, AP_ID_SYSTEMSETTING_TITLE);

	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("SystemSettingView Pull\n");
	}

	// 创建定时器，用于菜单的隐藏
	// 创建x秒的定时器
	XM_SetTimer (XMTIMER_SYSTEMSETTING, OSD_AUTO_HIDE_TIMEOUT);
}

VOID SystemSettingViewOnLeave (XMMSG *msg)
{
	// 删除定时器
	XM_KillTimer (XMTIMER_SYSTEMSETTING);
	
	if (msg->wp == 0)
	{
		// 窗口退出，彻底摧毁。
		// 获取私有数据句柄
		SYSTEMSETTINGVIEWDATA *settingViewData = (SYSTEMSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemSettingView));
		// 释放所有分配的资源
		if(settingViewData)
		{
			// 按钮控件退出过程
			AP_ButtonControlExit (&settingViewData->btnControl);
			// 标题控件退出过程
		//	AP_TitleBarControlExit (&settingViewData->titleControl);
			// 释放私有数据句柄
			XM_free (settingViewData);
		}
		// 设置窗口的私有数据句柄为空
		XM_SetWindowPrivateData (XMHWND_HANDLE(SystemSettingView), NULL);
		
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("SystemSettingView Push\n");
	}
}



VOID SystemSettingViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect, rcArrow;
	int i;
	float scale_factor;		// 水平缩放因子
	XMCOORD menu_name_x, menu_data_x, menu_flag_x;	// 菜单项标题、数值、标识的x坐标
	unsigned int old_alpha;
	HANDLE hwnd = XMHWND_HANDLE(SystemSettingView);
	SYSTEMSETTINGVIEWDATA *settingViewData = (SYSTEMSETTINGVIEWDATA *)XM_GetWindowPrivateData (hwnd);

	XM_printf ("SystemSettingViewOnPaint \n");
	if(settingViewData == NULL)
		return;

	XM_GetDesktopRect (&rc);
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	// 计算水平缩放因子(UI按320X240规格设计)
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);
	menu_name_x = (XMCOORD)(APP_POS_ITEM5_MENUNAME_X * scale_factor);
	menu_data_x = (XMCOORD)(APP_POS_ITEM5_MENUDATA_X * scale_factor);
	menu_flag_x = (XMCOORD)(APP_POS_ITEM5_MENUFLAG_X * scale_factor);

	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);
	
	// --------------------------------------
	//
	// ********* 1 显示标题栏区信息 *********
	//
	// --------------------------------------
	AP_DrawTitlebarControl (hwnd, AP_NULLID, AP_ID_SYSTEMSETTING_TITLE);
	// 处理标题控件的显示。
	// 若存在标题控件，必须调用AP_TitleControlMessageHandler执行标题控件显示
	//AP_TitleBarControlMessageHandler (&settingViewData->titleControl, msg);

	// 显示菜单项水平分割线
	rect = rc;
	for (i = 0; i < 6; i++)
	{
		rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * scale_factor);
		rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + i * APP_POS_ITEM5_LINEHEIGHT);
		AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	}

	// 填充当前选择项背景
	rect = rc;
	rect.left = 0;
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + settingViewData->nCurItem * APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	
	// --------------------------------------
	//
	// ********* 2 显示菜单项区 *************
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
			case 0:	// "管理副摄像头”
				AppRes = AP_AppRes2RomRes (AP_ID_SYSTEMSETTING_MANAGECCD);
				XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// 显示选项
				break;
			*/

			case SYSTEM_MENU_SDCARDVOLUME:	// "SDCARD容量"
				AP_RomImageDrawByMenuID (AP_ID_SYSTEMSETTING_SDCARDVOLUME, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// 显示选项
				break;

			case SYSTEM_MENU_FORMAT:	// "格式化"
				AP_RomImageDrawByMenuID (AP_ID_SYSTEMSETTING_FORMAT, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// 显示选项
				break;

			case SYSTEM_MENU_RESTORESETTING:	// "恢复出厂设置"
				AP_RomImageDrawByMenuID (AP_ID_SYSTEMSETTING_RESTORESETTING, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// 显示选项
				break;
	
#ifdef _XM_SYSTEM_UPDATE_ENABLE_
			case SYSTEM_MENU_SYSTEMUPDATE:	//"系统升级"
				AP_RomImageDrawByMenuID (AP_ID_SYSTEMSETTING_SYSTEMUPDATE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				break;
#endif

			case SYSTEM_MENU_SOFTWAREVERSION:	// "软件版本"
				AP_RomImageDrawByMenuID (AP_ID_SYSTEMSETTING_SOFTWAREVERSION, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				break;
				
		}
		// 显示向右的标记
		rcArrow = rect;
		rcArrow.left = menu_flag_x;
		AP_RomImageDrawByMenuID (AP_ID_COMMON_RIGHT_ARROW, hwnd, &rcArrow, XMGIF_DRAW_POS_LEFTTOP);
		
		rect.top += APP_POS_ITEM5_LINEHEIGHT;
	}
	// 处理按钮控件的显示。
	// 若存在按钮控件，必须调用AP_ButtonControlMessageHandler执行按钮控件显示
	AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);

	XM_SetWindowAlpha (hwnd, (unsigned char)old_alpha);
}


VOID SystemSettingViewOnKeyDown (XMMSG *msg)
{
	SYSTEMSETTINGVIEWDATA *settingViewData = (SYSTEMSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemSettingView));
	if(settingViewData == NULL)
		return;
	// 按键音
	XM_Beep (XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{     
	    case VK_AP_SWITCH:	// 画面切换键
	         break;
		case VK_AP_MENU:		// 菜单键
		case VK_AP_MODE:		// 切换到“行车记录”状态
			// 在"录像浏览"窗口中，MENU、Power、Switch及MODE键被定义为按钮操作。
			// 此处将这四个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			XM_SetTimer (XMTIMER_SYSTEMSETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
			
        #if 1
		case VK_AP_UP:		// 紧急录像键
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
	    case VK_AP_SWITCH:	// 画面切换键
	       XM_PostMessage (XM_COMMAND, SYSTEMSETTING_COMMAND_MODIFY, settingViewData->nCurItem);

	           break;
		case VK_AP_MENU:		// 菜单键
		case VK_AP_MODE:		// 切换到“行车记录”状态
		//case VK_AP_SWITCH:
			// 在"时间设置"窗口中，MENU及MODE键被定义为按钮操作。
			// 此处将这两个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;
	}
}

// 恢复系统设置的系统事件回调函数
static void restore_setting_view_system_event_handler (XMMSG *msg, int process_state)
{
	// 不处理任何事件
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:					// SD卡拔出事件
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
			if(settingViewData->nCurItem == SYSTEM_MENU_SDCARDVOLUME) // "SD卡容量"
			{
				XM_OpenRecycleVideoAlertView (
					AP_ID_SYSTEMSETTING_SDCARDVOLUME_TITLE,		// 循环录像时间低于设定值
					APP_ALERT_BKGCOLOR,						// 背景色
					40.0,											// 10秒钟自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					XM_VIEW_ALIGN_BOTTOM,
					0
					);
				    settingViewData->nCurItem=-1;
			}
			else if(settingViewData->nCurItem == SYSTEM_MENU_FORMAT) // "格式化"
			{
			    //settingViewData->nCurItem=-1;
				AP_OpenFormatView (1);
			}
			else if(settingViewData->nCurItem == SYSTEM_MENU_RESTORESETTING)// "恢复出厂设置"
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
			else if(settingViewData->nCurItem == SYSTEM_MENU_SYSTEMUPDATE)//"系统升级"
			{
				AP_OpenSystemUpdateView (1);
			}
#endif	// #ifdef _XM_SYSTEM_UPDATE_ENABLE_
			else // "软件版本"
			{
				//AP_OpenMessageViewEx (AP_ID_SYSTEMSETTING_VERSION_TITLE, AP_ID_SYSTEMSETTING_VERSION, 30, 0);
				XM_PushWindowEx (XMHWND_HANDLE(SystemVersionView), (DWORD)0);
				// settingViewData->nCurItem=-1;
			}
			break;

		case SYSTEMSETTING_COMMAND_RETURN:
			// 返回到桌面
			XM_PullWindow (0);
			break;

		case SYSTEMSETTING_COMMAND_SETTING:
			// 切换到“设置”窗口
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
			XM_JumpWindowEx (XMHWND_HANDLE(SettingView), 0, XM_JUMP_POPDEFAULT);
			break;

	}
}

int AppLocateItem (HANDLE hwnd, int item_count, int item_height, int top_item, int x, int y)
{
	XMRECT rc;
	int index;

	// 列表区域的上限/下限
	int top = APP_POS_ITEM5_MENUNAME_Y;
	int bottom = APP_POS_ITEM5_MENUNAME_Y + item_count  * item_height;

	if((y < top )|| (y >= bottom))
		return -1;
	// 列表区域的左右边
	if((x < 430)|| (x >= 1160))
		return -1;

	XM_GetDesktopRect (&rc);

	index = (y - top) / item_height;//eason
   
	if(index >= item_count )
		return -1;

    XM_Beep (XM_BEEP_KEYBOARD);//eason
      
	// 索引项
	return index + top_item;
}

#define TP_LEFT     		500
#define TP_WIDTH           600

int AppLocateItem1 (HANDLE hwnd, int item_count, int item_height, int top_item, int x, int y)
{
	static XMRECT rcbuffer[4]={{26,52,290,188},{316,52,580,188},{26,194,290,330},{316,194,580,330}};
	int index;
	
	// 列表区域的上限/下限
	int top = 52;
	int bottom = 330;

	if((y < top )|| (y >= bottom))
		return -1;
	
	// 列表区域的左右边
	if((x < TP_LEFT)|| (x >= TP_LEFT+TP_WIDTH))
		return -1;

    for(index =0;index<4;index++)
    {
       	if (((x-TP_LEFT) >= rcbuffer[index].left) && ((x-TP_LEFT) < rcbuffer[index].right) && (y >rcbuffer[index].top) && (y < rcbuffer[index].bottom))
		{
	         // 索引项
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

	// 返回到桌面
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

// 消息处理函数定义
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

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, SystemSettingView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)

