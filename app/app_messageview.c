//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_messageview.c
//	  提示信息视窗
//		(仅显示提示信息，可定义或禁止定时器自动关闭，一个确认按钮)
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"

#define MESSAGEVIEW_COMMAND_OK			1
#define MESSAGEVIEW_COMMAND_CANCEL		2
#define	MESSAGEVIEW_COMMAND_RETURN		3

// “菜单选项”窗口按钮控件定义
#define	MESSAGEVIEWBTNCOUNT	1
#define COMFIRMVIEWBTNCOUNT	2

static const XMBUTTONINFO menuOptionReturnBtn[MESSAGEVIEWBTNCOUNT] = {
	{	
		VK_AP_MENU,		MESSAGEVIEW_COMMAND_RETURN,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_RETURN
	}
};

static const XMBUTTONINFO menuOptionComfirmBtn[COMFIRMVIEWBTNCOUNT] = {
	{	
		VK_AP_MENU,		MESSAGEVIEW_COMMAND_OK,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
	},
	{	
		VK_AP_MODE,		MESSAGEVIEW_COMMAND_CANCEL,	AP_ID_COMMON_OK,	AP_ID_BUTTON_CANCEL
	}
};

typedef struct tagMESSAGEVIEWDATA {
	DWORD					dwTitleID;				// 标题栏文本	
	DWORD					dwMessageID;			// 信息文本
	DWORD					dwAutoCloseTime;		// 视窗定时器自动关闭时间。-1表示
	DWORD					dwMode;					// 显示
	FPMENUOPTIONCB		fpMenuOptionCB;		// 回调函数，此处应用，两个参数都为空
	DWORD					dwTimeout;				// 计数器
	XMBUTTONCONTROL	btnControl;				// 按钮控件
} MESSAGEVIEWDATA;

VOID AP_OpenComfirmView (DWORD dwViewTitleID, DWORD dwViewMessageID, void *fpMenuOptionCB)
{
	MESSAGEVIEWDATA *messageViewData = XM_calloc (sizeof(MESSAGEVIEWDATA));
	if(messageViewData == NULL)
		return;
	messageViewData->dwMessageID = dwViewMessageID;
	messageViewData->dwTitleID = dwViewTitleID;
	messageViewData->dwAutoCloseTime = (DWORD)-1;
	messageViewData->dwMode = COMFIRMVIEWBTNCOUNT;
	messageViewData->fpMenuOptionCB = (FPMENUOPTIONCB)fpMenuOptionCB;
	if(!XM_PushWindowEx (XMHWND_HANDLE(MessageView), (DWORD)messageViewData))
	{
		XM_free (messageViewData);
		XM_printf ("XM_PushWindowEx MessageView NG\n");
	}
}

// 显示信息视图
// dwViewTitleID 信息标题ID
// dwViewMessageID 信息文本ID
// dwAutoCloseTime 指定自动关闭时间(秒单位) (-1) 禁止自动关闭
VOID AP_OpenMessageViewEx (DWORD dwViewTitleID, DWORD dwViewMessageID, DWORD dwAutoCloseTime, XMBOOL bPopTopView)
{
	MESSAGEVIEWDATA *messageViewData = XM_calloc (sizeof(MESSAGEVIEWDATA));
	if(messageViewData == NULL)
		return;
	messageViewData->dwMessageID = dwViewMessageID;
	messageViewData->dwTitleID = dwViewTitleID;
	messageViewData->dwAutoCloseTime = dwAutoCloseTime;
	//messageViewData->dwMode = MESSAGEVIEWBTNCOUNT;
	messageViewData->dwMode = COMFIRMVIEWBTNCOUNT;
	messageViewData->fpMenuOptionCB = NULL;
	if(bPopTopView)
	{
		if(!XM_JumpWindowEx (XMHWND_HANDLE(MessageView), (DWORD)messageViewData, XM_JUMP_POPTOPVIEW))
		{
			XM_free (messageViewData);
		}
	}
	else
	{
		if(!XM_PushWindowEx (XMHWND_HANDLE(MessageView), (DWORD)messageViewData))
		{
			XM_free (messageViewData);
		}
	}
}

// 显示信息视图(使用缺省标题、3秒钟自动关闭)
VOID AP_OpenMessageView (DWORD dwViewMessageID)
{
	AP_OpenMessageViewEx (AP_ID_MESSAGE_PROMPT_TITLE, dwViewMessageID, 3, 0);
}


VOID MessageViewOnEnter (XMMSG *msg)
{
	MESSAGEVIEWDATA *messageViewData;
	if(msg->wp == 0)
	{
		messageViewData = (MESSAGEVIEWDATA *)msg->lp;

		// 按钮控件初始化
		if (messageViewData->dwMode == MESSAGEVIEWBTNCOUNT)
			AP_ButtonControlInit (&messageViewData->btnControl, MESSAGEVIEWBTNCOUNT, XMHWND_HANDLE(MessageView), menuOptionReturnBtn);
		else
			AP_ButtonControlInit (&messageViewData->btnControl, COMFIRMVIEWBTNCOUNT, XMHWND_HANDLE(MessageView), menuOptionComfirmBtn);

		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(MessageView), messageViewData);
	}
	else
	{
		messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("MessageView Pull\n");
	}

	// 创建定时器
	if(messageViewData && messageViewData->dwAutoCloseTime != (DWORD)(-1))
	{
		XM_SetTimer (XMTIMER_MESSAGEVIEW, 100);	// 创建100ms的定时器
		messageViewData->dwTimeout = XM_GetTickCount() + messageViewData->dwAutoCloseTime * 1000;
	}
}

VOID MessageViewOnLeave (XMMSG *msg)
{
	MESSAGEVIEWDATA *messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
	// 删除定时器
	if(messageViewData && messageViewData->dwAutoCloseTime != (DWORD)(-1))
	{
		XM_KillTimer (XMTIMER_MESSAGEVIEW);
	}

	if (msg->wp == 0)
	{
		MESSAGEVIEWDATA *messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
		if(messageViewData)
		{
			// 按钮控件退出过程
			AP_ButtonControlExit (&messageViewData->btnControl);

			// 释放私有数据句柄
			XM_free (messageViewData);
			
		}
		XM_printf ("MessageView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("MessageView Push\n");
	}
}



VOID MessageViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
	APPROMRES *AppRes;
	unsigned int old_alpha;
	HANDLE hWnd = XMHWND_HANDLE(MessageView);

	MESSAGEVIEWDATA *messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (hWnd);
	if(messageViewData == NULL)
		return;

	// 显示标题栏
	XM_GetDesktopRect (&rc);
	XM_FillRect (hWnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	old_alpha = XM_GetWindowAlpha (hWnd);
	XM_SetWindowAlpha (hWnd, 255);

	// --------------------------------------
	//
	// ********* 1 显示标题栏区信息 *********
	//
	// --------------------------------------
	AP_DrawTitlebarControl (hWnd, AP_NULLID, messageViewData->dwTitleID);

	rect.left = rc.left;
	rect.right = rc.right;
	rect.top = (XMCOORD)(rc.top + APP_TITLEBAR_HEIGHT);
	rect.bottom = (XMCOORD)(rc.bottom - APP_BUTTON_HEIGHT);
	AppRes = AP_AppRes2RomRes (messageViewData->dwMessageID);
	XM_RomImageDraw (AppRes->rom_offset, AppRes->res_length, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
	// --------------------------------------
	//
	// ********* 3 显示按钮区 ***************
	//
	// --------------------------------------
	// 处理按钮控件的显示。
	// 若存在按钮控件，必须调用AP_ButtonControlMessageHandler执行按钮控件显示
	AP_ButtonControlMessageHandler (&messageViewData->btnControl, msg);

	XM_SetWindowAlpha (hWnd, (unsigned char)old_alpha);

}


VOID MessageViewOnKeyDown (XMMSG *msg)
{
	MESSAGEVIEWDATA *messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
	if(messageViewData == NULL)
		return;
// 按键音
	XM_Beep (XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
		case VK_AP_MENU:		// 菜单键
		case VK_AP_MODE:		// 切换到“行车记录”状态
		case VK_AP_SWITCH:
			AP_ButtonControlMessageHandler (&messageViewData->btnControl, msg);
			break;

		default:
			// 此处将键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&messageViewData->btnControl, msg);
			break;

	}
}

VOID MessageViewOnKeyUp (XMMSG *msg)
{
	MESSAGEVIEWDATA *messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
	if(messageViewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// 菜单键
		case VK_AP_MODE:		// 切换到“行车记录”状态
		case VK_AP_SWITCH:
			AP_ButtonControlMessageHandler (&messageViewData->btnControl, msg);
			break;

		default:
			// 此处将键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&messageViewData->btnControl, msg);
			break;
	}
}

VOID MessageViewOnCommand (XMMSG *msg)
{
	MESSAGEVIEWDATA *messageViewData;
	messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
	if(messageViewData == NULL)
		return;
	switch(msg->wp)
	{
		case MESSAGEVIEW_COMMAND_OK:
			if(messageViewData->fpMenuOptionCB)
			{
				(*messageViewData->fpMenuOptionCB) ((void*)NULL, (int)NULL);
			}

//			目前的窗口是预先创建好的一个窗口，仅能有一个实例，窗口栈中不能同时出现两个相同的窗口，否则私有数据会被覆盖。
//			AP_OpenMessageViewEx (messageViewData->dwTitleID, AP_ID_MESSAGE_SUCCESS, 30, 0);
//			else
//				AP_OpenMessageViewEx (messageViewData->dwTitleID, AP_ID_MESSAGE_FAIL, 30, 0);

			XM_PullWindow (0);
			break;
				
		case MESSAGEVIEW_COMMAND_CANCEL:
		case MESSAGEVIEW_COMMAND_RETURN:
			// 返回到调用者视窗
			XM_PullWindow (0);
			break;
	}
}

VOID MessageViewOnTimer (XMMSG *msg)
{
	MESSAGEVIEWDATA *messageViewData;
	messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
	if(messageViewData == NULL)
		return;
	if(messageViewData->dwAutoCloseTime != (DWORD)(-1))
	{
		if(messageViewData->dwTimeout <= XM_GetTickCount())
		{
			// 返回到调用者视窗
			XM_PullWindow (0);
		}
	}
}
VOID MessageViewOnTouchDown (XMMSG *msg)
{
       int x,y;
	int index;
	MESSAGEVIEWDATA *messageViewData;
	messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
	if(messageViewData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&messageViewData->btnControl, msg))
		return;	

}

VOID MessageViewOnTouchUp (XMMSG *msg)
{
       int x,y;
	   MESSAGEVIEWDATA *messageViewData;
	messageViewData = (MESSAGEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MessageView));
	if(messageViewData == NULL)
		return;


	if(AP_ButtonControlMessageHandler (&messageViewData->btnControl, msg))
		return;
	
}
// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (MessageView)
	XM_ON_MESSAGE (XM_PAINT, MessageViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, MessageViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, MessageViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, MessageViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, MessageViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, MessageViewOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, MessageViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, MessageViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, MessageViewOnTouchUp)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, MessageView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
