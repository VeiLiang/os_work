//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: app_systemversionview.c
//	  系统版本信息视窗
//
//	Revision history
//
//		2010.03.11	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include <xm_type.h>
#include "app_menuid.h"

#define	OSD_AUTO_HIDE_TIMEOUT			10000	

// 私有命令定义
#define	SYSTEMVERSIONVIEW_COMMAND_OK				0
#define	SYSTEMVERSIONVIEW_COMMAND_RETURN		1

// “菜单选项”窗口按钮控件定义
#define	SYSTEMVERSIONVIEWBTNCOUNT	2
static const XMBUTTONINFO systemVersionBtn[SYSTEMVERSIONVIEWBTNCOUNT] = {
	{	
		VK_AP_MENU,		SYSTEMVERSIONVIEW_COMMAND_OK,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
	},
	{	
		VK_AP_MODE,		SYSTEMVERSIONVIEW_COMMAND_RETURN,	AP_ID_COMMON_OK,	AP_ID_BUTTON_RETURN
	},
};

typedef struct tagSYSTEMVERSIONVIEWDATA {

	XMBUTTONCONTROL	btnControl;				// 按钮控件
	XMTITLEBARCONTROL	titleControl;			// 标题控件

} SYSTEMVERSIONVIEWDATA;


VOID AP_OpenSystemVersionView (void)
{
	XM_PushWindowEx (XMHWND_HANDLE(SystemVersionView), (DWORD)0);
}



VOID SystemVersionViewOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
		//APPROMRES *AppRes;
		SYSTEMVERSIONVIEWDATA *systemVersionViewData = XM_calloc (sizeof(SYSTEMVERSIONVIEWDATA));
		if(systemVersionViewData == NULL)
		{
			XM_printf ("systemVersionViewData XM_calloc failed\n");
			// 失败返回到桌面
			XM_PullWindow (0);
			return;
		}
		

		// 按钮控件初始化
		AP_ButtonControlInit (&systemVersionViewData->btnControl, SYSTEMVERSIONVIEWBTNCOUNT, XMHWND_HANDLE(SystemVersionView), systemVersionBtn);
		// 标题控件初始化
		AP_TitleBarControlInit (&systemVersionViewData->titleControl, XMHWND_HANDLE(SystemVersionView), 
														AP_NULLID, AP_ID_SYSTEMSETTING_VERSION_TITLE);

		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(SystemVersionView), systemVersionViewData);

	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("SystemVersionView Pull\n");
	}

	// 创建定时器，用于菜单的隐藏
	// 创建x秒的定时器
	XM_SetTimer (XMTIMER_VERSIONVIEW, OSD_AUTO_HIDE_TIMEOUT);
}

VOID SystemVersionViewOnLeave (XMMSG *msg)
{
	// 删除定时器
	XM_KillTimer (XMTIMER_VERSIONVIEW);
	
	if (msg->wp == 0)
	{
		//int i;
		SYSTEMVERSIONVIEWDATA *systemVersionViewData = (SYSTEMVERSIONVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemVersionView));
		if(systemVersionViewData)
		{
			// 按钮控件退出过程
			AP_ButtonControlExit (&systemVersionViewData->btnControl);
			// 标题控件退出过程
			AP_TitleBarControlExit (&systemVersionViewData->titleControl);

			// 释放私有数据句柄
			XM_free (systemVersionViewData);
			
		}
		XM_printf ("SystemVersionView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("SystemVersionView Push\n");
	}
}

int XMSYS_GetSystemSoftwareVersion (unsigned char sw_version[4]);

VOID SystemVersionViewOnPaint (XMMSG *msg)
{
	XMRECT rc;
	unsigned int old_alpha;
	unsigned char software_version[4];
	XMSIZE size;
	XMCOORD x, y;

	char text[32]="ARK V 1.2  20180324";
	HANDLE hwnd = XMHWND_HANDLE(SystemVersionView);
	SYSTEMVERSIONVIEWDATA *systemVersionViewData = (SYSTEMVERSIONVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(systemVersionViewData == NULL)
		return;

	// 显示标题栏
	XM_GetDesktopRect (&rc); 
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);

	// --------------------------------------
	//
	// ********* 1 显示标题栏区信息 *********
	//
	// --------------------------------------
	//AP_DrawTitlebarControl (hwnd, lpSystemVersionView->dwWindowIconId, lpSystemVersionView->dwWindowTextId);
	// 处理标题控件的显示。
	// 若存在标题控件，必须调用AP_TitleControlMessageHandler执行标题控件显示
	AP_TitleBarControlMessageHandler (&systemVersionViewData->titleControl, msg);


	//memset (software_version, 0, sizeof(software_version));
	//XMSYS_GetSystemSoftwareVersion (software_version);

	//sprintf (text, "%02x%02x%02x%02x", software_version[3], software_version[2], software_version[1], software_version[0]);
	AP_TextGetStringSize (text, strlen(text), &size);

	x = (XMCOORD)( (rc.left + (rc.right - rc.left + 1 - size.cx) / 2) );
	y = (XMCOORD)( (rc.top + (rc.bottom - rc.top + 1 - size.cy) / 2) );
	AP_TextOutDataTimeString (XMHWND_HANDLE(SystemVersionView), x, y, text, strlen(text));
	
	// --------------------------------------
	//
	// ********* 3 显示按钮区 ***************
	//
	// --------------------------------------
	// 处理按钮控件的显示。
	// 若存在按钮控件，必须调用AP_ButtonControlMessageHandler执行按钮控件显示
	AP_ButtonControlMessageHandler (&systemVersionViewData->btnControl, msg);

	XM_SetWindowAlpha (hwnd, (unsigned char)old_alpha);

}


VOID SystemVersionViewOnKeyDown (XMMSG *msg)
{
	int nVisualCount;
	XMRECT rc;
	SYSTEMVERSIONVIEWDATA *systemVersionViewData = (SYSTEMVERSIONVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemVersionView));
	if(systemVersionViewData == NULL)
		return;

	XM_GetWindowRect (XMHWND_HANDLE(SystemVersionView), &rc);
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
// 按键音
	XM_Beep (XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
		// 向上键
		case VK_AP_UP:
			break;

		// 向下键
		case VK_AP_DOWN:
			break;

		case VK_AP_MENU:
		case VK_AP_MODE:
			// 此处将这三个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&systemVersionViewData->btnControl, msg);
			XM_SetTimer (XMTIMER_VERSIONVIEW, OSD_AUTO_HIDE_TIMEOUT);
			break;

	}
}

VOID SystemVersionViewOnKeyUp (XMMSG *msg)
{
	SYSTEMVERSIONVIEWDATA *systemVersionViewData = (SYSTEMVERSIONVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemVersionView));
	if(systemVersionViewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:
		case VK_AP_MODE:
			// 此处将这三个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&systemVersionViewData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&systemVersionViewData->btnControl, msg);
			break;

	}
}

VOID SystemVersionViewOnCommand (XMMSG *msg)
{
	SYSTEMVERSIONVIEWDATA *systemVersionViewData;
	systemVersionViewData = (SYSTEMVERSIONVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemVersionView));
	if(systemVersionViewData == NULL)
		return;
	switch(msg->wp)
	{
		case SYSTEMVERSIONVIEW_COMMAND_OK:
			// 确认并返回
			XM_PullWindow (NULL);

			break;

		case SYSTEMVERSIONVIEW_COMMAND_RETURN:
			// 返回
			// 返回到桌面
			XM_PullWindow (0);
			break;
	}
}


static VOID SystemVersionViewOnTimer (XMMSG *msg)
{
	SYSTEMVERSIONVIEWDATA *systemVersionViewData;
	systemVersionViewData = (SYSTEMVERSIONVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemVersionView));
	if(systemVersionViewData == NULL)
	   return;
	
	// 返回到桌面
	XM_PullWindow(XMHWND_HANDLE(Desktop));
}


VOID SystemVersionViewOnTouchDown (XMMSG *msg)
{
	int index;
      SYSTEMVERSIONVIEWDATA *systemVersionViewData;
	systemVersionViewData = (SYSTEMVERSIONVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemVersionView));
	if(systemVersionViewData == NULL)
		return;
	
	if(AP_ButtonControlMessageHandler (&systemVersionViewData->btnControl, msg))
		return;

	XM_SetTimer (XMTIMER_VERSIONVIEW, OSD_AUTO_HIDE_TIMEOUT);
	
    #if 0
	index = AppLocateItem (XMHWND_HANDLE(systemVersionViewData), systemVersionViewData->nItemCount, APP_POS_ITEM5_LINEHEIGHT, settingViewData->nTopItem, LOWORD(msg->lp), HIWORD(msg->lp));
	if(index < 0)
		return;
	systemVersionViewData->nTouchItem = index;
	if(systemVersionViewData->nCurItem != index)
	{
		systemVersionViewData->nCurItem = index;
	
		XM_InvalidateWindow ();
		XM_UpdateWindow ();
	}
    #endif
}

VOID SystemVersionViewOnTouchUp (XMMSG *msg)
{
	   SYSTEMVERSIONVIEWDATA *systemVersionViewData;
	systemVersionViewData = (SYSTEMVERSIONVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemVersionView));
	if(systemVersionViewData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&systemVersionViewData->btnControl, msg))
		return;
	
	#if 0
	if(systemVersionViewData->nTouchItem == -1)
		return;
	systemVersionViewData->nTouchItem = -1;
	XM_PostMessage (XM_COMMAND, systemVersionViewData->btnControl.pBtnInfo[0].wCommand, systemVersionViewData->nCurItem);
    #endif  
}

// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (SystemVersionView)
	XM_ON_MESSAGE (XM_PAINT, SystemVersionViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, SystemVersionViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, SystemVersionViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, SystemVersionViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, SystemVersionViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, SystemVersionViewOnCommand)
	XM_ON_MESSAGE (XM_TIMER, SystemVersionViewOnTimer)
	XM_ON_MESSAGE (XM_TOUCHDOWN, SystemVersionViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, SystemVersionViewOnTouchUp)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, SystemVersionView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
