//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_card.c
// “SD卡”窗口定制属性, 暂定5种
//		1  SD卡正常格式化定制属性 DEFAULT (设置界面)
//		2	检测到卡文件系统空闲容量低时 LOWSPACE，提示用户进行格式化的定制属性。（仅在开机及卡插入时提示）
//		3	卡未插入时 NOCARD ，提示用户插入卡的定制属性
//		4	卡写保护 WRITEPROTECT，提示用户解除卡写保护
//		5	卡文件系统检测失败 FSERROR (如文件系统格式错误，路径无法访问或创建等)，提示用户进行格式化的定制属性
//
//	Revision history
//
//		2012.09.19	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"

#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"
#include "xm_app_menudata.h"
#include "system_check.h"


#define	CARDVIEW_COMMAND_FORMAT		0
#define	CARDVIEW_COMMAND_RETURN		1
#define	CARDVIEW_COMMAND_CANCEL		2
#define	CARDVIEW_COMMAND_DEMOMODE	3

static const DWORD cardTitleID[APP_CARDVIEW_CUSTOM_COUNT] = {
	AP_ID_CARD_TITLE_FORMAT,
	AP_ID_CARD_TITLE_FORMAT,
	AP_ID_CARD_TITLE_CARDCHECKING,
	AP_ID_CARD_TITLE_CARDCHECKING,
	AP_ID_CARD_TITLE_FSERROR,
	AP_ID_CARD_TITLE_CARDDAMAGE
};

// CARDVIEW按键定义
#define	CARDVIEWBTNCOUNT		2
static const XMBUTTONINFO cardBtn[APP_CARDVIEW_CUSTOM_COUNT][CARDVIEWBTNCOUNT] = {
	// 1  SD卡正常格式化定制属性 DEFAULT (设置界面)
	{
		{	
			VK_AP_MENU,		CARDVIEW_COMMAND_FORMAT,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
		},
		{	
			VK_AP_MODE,		CARDVIEW_COMMAND_CANCEL,	AP_ID_COMMON_MODE,	AP_ID_BUTTON_CANCEL
		},
	},
	// 2	检测到卡文件系统空闲容量低时 LOWSPACE，提示用户进行格式化的定制属性。（仅在开机时提示）
	{
		{	
			VK_AP_MENU,		CARDVIEW_COMMAND_FORMAT,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
		},
		{	
			VK_AP_MODE,		CARDVIEW_COMMAND_CANCEL,	AP_ID_COMMON_MODE,	AP_ID_BUTTON_CANCEL
		},
	},
	// 3	卡未插入时 NOCARDVIEW ，提示用户插入卡的定制属性
	{
		{	
			VK_AP_MENU,		CARDVIEW_COMMAND_RETURN,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
		},
		{	
			VK_AP_MODE,		CARDVIEW_COMMAND_DEMOMODE,	AP_ID_COMMON_MODE,	AP_ID_CARD_BUTTON_DEMO
		},
	},
	// 4	卡写保护 ，提示用户解除卡写保护
	{
		{	
			VK_AP_MENU,		CARDVIEW_COMMAND_RETURN,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
		},
		{	
			VK_AP_MODE,		CARDVIEW_COMMAND_DEMOMODE,	AP_ID_COMMON_MODE,	AP_ID_CARD_BUTTON_DEMO
		},
	},
	//	5	卡文件系统检测失败 FSERROR (如文件系统格式错误，路径无法访问或创建等)，提示用户进行格式化的定制属性
	{
		{	
			VK_AP_MENU,		CARDVIEW_COMMAND_FORMAT,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
		},
		{	
			VK_AP_MODE,		CARDVIEW_COMMAND_CANCEL,	AP_ID_COMMON_MODE,	AP_ID_BUTTON_CANCEL
		},
	},
	//	6	卡读写校验失败，卡已损坏 CARD_DAMAGE ，提示用户更换数据卡或者进行格式化的定制属性
	{
		{	
			VK_AP_MENU,		CARDVIEW_COMMAND_FORMAT,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
		},
		{	
			VK_AP_MODE,		CARDVIEW_COMMAND_CANCEL,	AP_ID_COMMON_MODE,	AP_ID_BUTTON_CANCEL
		},
	}
};



typedef struct tagCARDVIEWDATA {
	DWORD					dwCustomOption;		// 卡格式化窗口定制属性

	XMBUTTONCONTROL	btnControl;

} CARDVIEWDATA;


VOID CardViewOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
		// 窗口未建立，第一次进入
		CARDVIEWDATA *cardData;

		// 检查卡窗口定制属性
		if(msg->lp > APP_CARDVIEW_CUSTOM_CARDDAMAGE)
			return;
	
		// 分配私有数据句柄
		cardData = XM_calloc (sizeof(CARDVIEWDATA));
		if(cardData == NULL)
		{
			XM_printf ("cardData XM_calloc failed\n");
			// 失败返回到调用者窗口
			XM_PullWindow (0);
			return;
		}

		cardData->dwCustomOption = msg->lp;

		// 按钮控件初始化
		AP_ButtonControlInit (&cardData->btnControl, CARDVIEWBTNCOUNT, 
			XMHWND_HANDLE(CardView), &cardBtn[msg->lp][0]);

		
		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(CardView), cardData);
	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("CardView Pull\n");
	}
}

VOID CardViewOnLeave (XMMSG *msg)
{
	if (msg->wp == 0)
	{
		// 窗口退出，彻底摧毁。
		// 获取私有数据句柄
		CARDVIEWDATA *cardData = (CARDVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(CardView));
		// 释放所有分配的资源
		if(cardData)
		{
			// 按钮控件退出过程
			AP_ButtonControlExit (&cardData->btnControl);
			// 释放私有数据句柄
			XM_free (cardData);
		}
		XM_printf ("CardView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("CardView Push\n");
	}
}



VOID CardViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
	APPROMRES *AppRes;
	XMSIZE error_size, info_size;
	int sx, sy;
	DWORD eID, iID;
	int card_state;
	unsigned int old_alpha;

	CARDVIEWDATA *cardData;

	cardData = (CARDVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(CardView));
	if(cardData == NULL)
		return;

	XM_GetDesktopRect (&rc);
	XM_FillRect (XMHWND_HANDLE(CardView), rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_WINDOW));

	old_alpha = XM_GetWindowAlpha (XMHWND_HANDLE(CardView));
	XM_SetWindowAlpha (XMHWND_HANDLE(CardView), 255);

	// --------------------------------------
	//
	// ********* 1 显示标题栏区信息 *********
	//
	// --------------------------------------
	AP_DrawTitlebarControl (XMHWND_HANDLE(CardView), 
				AP_NULLID, 
				cardTitleID[cardData->dwCustomOption] - APP_CARDVIEW_CUSTOM_DEFAULT);


	// 根据卡格式化窗口的定制属性显示提示信息
	switch (cardData->dwCustomOption)
	{
		case APP_CARDVIEW_CUSTOM_DEFAULT:
			// 居中显示
			rc.top = (XMCOORD)(rc.top + APP_TITLEBAR_HEIGHT);
			rc.bottom = (XMCOORD)(rc.bottom - APP_BUTTON_HEIGHT);
			// 根据卡的三种状态 “卡拔出状态”“卡读写状态”“卡写保护”，提示不同的信息
			card_state = APSYS_CardChecking ();
			if(card_state == APP_SYSTEMCHECK_CARD_NOCARD)
			{
				eID = AP_ID_CARD_INFO_CARDINSERT;
			//	AppRes = AP_AppRes2RomRes (AP_ID_CARD_INFO_CARDINSERT);
			}
			else if(card_state == APP_SYSTEMCHECK_CARD_WRITEPROTECT)
			{
				eID = AP_ID_CARD_INFO_WRITEPROTECT;
			//	AppRes = AP_AppRes2RomRes (AP_ID_CARD_INFO_WRITEPROTECT);
			}
			else
			{
				eID = AP_ID_CARD_INFO_FORMAT;
			//	AppRes = AP_AppRes2RomRes (AP_ID_CARD_INFO_FORMAT);
			}
			//XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, 
			//	XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
			//AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, 
			//	XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
			AP_RomImageDrawByMenuID (eID, XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
			break;

		case APP_CARDVIEW_CUSTOM_WRITEPROTECT:
			// “移去卡写保护”信息
			rc.top = (XMCOORD)(rc.top + APP_TITLEBAR_HEIGHT);
			rc.bottom = (XMCOORD)(rc.bottom - APP_BUTTON_HEIGHT);
			// 居中显示
		//	AppRes = AP_AppRes2RomRes (AP_ID_CARD_INFO_WRITEPROTECT);
			//XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, 
			//	XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
		//	AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, 
		//		XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
			AP_RomImageDrawByMenuID (AP_ID_CARD_INFO_WRITEPROTECT, XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
			break;

		case APP_CARDVIEW_CUSTOM_CARDDAMAGE:
			// “数据卡已损坏，请更换”信息
			rc.top = (XMCOORD)(rc.top + APP_TITLEBAR_HEIGHT);
			rc.bottom = (XMCOORD)(rc.bottom - APP_BUTTON_HEIGHT);
			// 居中显示
		//	AppRes = AP_AppRes2RomRes (AP_ID_CARD_INFO_CARDDAMAGE);
		//	XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, 
		//		XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
		//	AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, 
		//		XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
			AP_RomImageDrawByMenuID (AP_ID_CARD_INFO_CARDDAMAGE, XMHWND_HANDLE(CardView), &rc, XMGIF_DRAW_POS_CENTER);
			break;

		case APP_CARDVIEW_CUSTOM_LOWSPACE:
		case APP_CARDVIEW_CUSTOM_NOCARD:
		case APP_CARDVIEW_CUSTOM_FSERROR:
			rc.top = (XMCOORD)(rc.top + APP_TITLEBAR_HEIGHT);
			rc.bottom = (XMCOORD)(rc.bottom - APP_BUTTON_HEIGHT);
			if(cardData->dwCustomOption == APP_CARDVIEW_CUSTOM_LOWSPACE)
			{
				eID = AP_ID_CARD_INFO_LOWSPACE;
				iID = AP_ID_CARD_INFO_FORMAT;
			}
			else if(cardData->dwCustomOption == APP_CARDVIEW_CUSTOM_NOCARD)
			{
				eID = AP_ID_CARD_INFO_NOCARD;
				iID = AP_ID_CARD_INFO_CARDINSERT;
			}
			else if(cardData->dwCustomOption == APP_CARDVIEW_CUSTOM_FSERROR)
			{
				eID = AP_ID_CARD_INFO_FSERROR;
				iID = AP_ID_CARD_INFO_FORMAT;
			}
			else
				break;
			// 获取错误信息及开始格式化提示信息的尺寸
			AppRes = AP_AppRes2RomRes (eID);
			XM_GetGifImageSize (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, &error_size);
			AppRes = AP_AppRes2RomRes (iID);
			XM_GetGifImageSize (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, &info_size);
			// 计算信息的显示位置
			sy = ((rc.bottom - rc.top + 1) - (error_size.cy + info_size.cy + 24)) / 2;
			sx = ((rc.right - rc.left + 1) - error_size.cx) / 2;

			// 显示卡错误信息
			rect.top = (XMCOORD)(rc.top + sy);
			rect.left = (XMCOORD)(sx);
			rect.right = rect.left;
			rect.bottom = rect.top;
			AppRes = AP_AppRes2RomRes (eID);
			XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(CardView), &rect, XMGIF_DRAW_POS_LEFTTOP);
	//		AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, XMHWND_HANDLE(CardView), &rect, XMGIF_DRAW_POS_LEFTTOP);
		
			// 显示开始格式化信息
			rect.top = (XMCOORD)(rect.top + error_size.cy + 12);
			sx = ((rc.right - rc.left + 1) - info_size.cx) / 2;
			rect.left = (XMCOORD)(sx);
			rect.right = rect.left;
			rect.bottom = rect.top;
			AppRes = AP_AppRes2RomRes (iID);
			XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(CardView), &rect, XMGIF_DRAW_POS_LEFTTOP);
		//	AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, XMHWND_HANDLE(CardView), &rect, XMGIF_DRAW_POS_LEFTTOP);
	}

	// 处理按钮控件的显示。
	// 若存在按钮控件，必须调用AP_ButtonControlMessageHandler执行按钮控件显示
	AP_ButtonControlMessageHandler (&cardData->btnControl, msg);

	XM_SetWindowAlpha (XMHWND_HANDLE(CardView), (unsigned char)old_alpha);
}



VOID CardViewOnKeyDown (XMMSG *msg)
{
	CARDVIEWDATA *cardData;
	cardData = (CARDVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(CardView));
	if(cardData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// 菜单键
		case VK_AP_MODE:
			// 在"卡"窗口中，MENU及MODE键被定义为按钮操作。
			// 此处将这两个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&cardData->btnControl, msg);
			break;

		case VK_AP_UP:		// 紧急录像键
			AP_ButtonControlMessageHandler (&cardData->btnControl, msg);
			break;

		case VK_AP_DOWN:
			AP_ButtonControlMessageHandler (&cardData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&cardData->btnControl, msg);
			break;
	}

}

VOID CardViewOnKeyUp (XMMSG *msg)
{
	CARDVIEWDATA *cardData;
	cardData = (CARDVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(CardView));
	if(cardData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// 菜单键
		case VK_AP_MODE:		// 模式键
			// 在"卡格式化"窗口中，MENU及MODE键被定义为按钮操作。
			// 此处将这两个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&cardData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&cardData->btnControl, msg);
			break;
	}
}

VOID CardViewOnCommand (XMMSG *msg)
{
	CARDVIEWDATA *cardData;

	cardData = (CARDVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(CardView));
	if(cardData == NULL)
		return;

	switch(msg->wp)
	{
		case CARDVIEW_COMMAND_FORMAT:		// 格式化命令
			// 执行格式化操作
			AP_OpenFormatView (0);
			break;

		case CARDVIEW_COMMAND_RETURN:		// 返回
			// 返回到桌面
			XM_PullWindow (0);
			break;

		case CARDVIEW_COMMAND_CANCEL:		// 取消
			XM_PullWindow (0);
			break;

		case CARDVIEW_COMMAND_DEMOMODE:		// 无卡
			// 设置"无卡操作模式",即演示模式
			AppSetCardOperationMode (APP_DEMO_OPERATION_MODE);
			XM_PullWindow (0);
			break;
	}
}

// 卡插拔事件处理
// #define	XM_CARDVIEW			0x10	// SD卡插拔事件
										// lp保留为0
										// wp = 0, SD卡拔出事件
										// wp = 1, SD卡插入(写保护)
										// wp = 2, SD卡插入(读写允许)

VOID CardViewOnSystemEvent (XMMSG *msg)
{
	CARDVIEWDATA *cardData;
	cardData = (CARDVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(CardView));
	if(cardData == NULL)
		return;
	switch (cardData->dwCustomOption)
	{
		case APP_CARDVIEW_CUSTOM_NOCARD:			// “卡未插入”定制属性
			if(msg->wp == SYSTEM_EVENT_CARD_INSERT_WRITE_PROTECT || msg->wp == SYSTEM_EVENT_CARD_INSERT)
			{
				// 检测到卡插入事件
				// 返回到调用者窗口
				XM_BreakSystemEventDefaultProcess (msg);
				XM_PullWindow (0);
				return;
			}
			break;

		case APP_CARDVIEW_CUSTOM_WRITEPROTECT:	// “卡写保护”定制属性
			if(msg->wp == SYSTEM_EVENT_CARD_UNPLUG)
			{
				// 检测到卡拔出
				// 返回到调用者窗口
				XM_BreakSystemEventDefaultProcess (msg);
				XM_PullWindow (0);
				return;
			}
			break;

		case APP_CARDVIEW_CUSTOM_FSERROR:			// “文件系统错误”定制模式
		case APP_CARDVIEW_CUSTOM_CARDDAMAGE:		// "卡读写校验失败，卡已损坏"	
			// 1) 检查是否是同类卡事件。若是，直接过滤该同类事件
			if(	msg->wp == SYSTEM_EVENT_CARD_FS_ERROR
				||	msg->wp == SYSTEM_EVENT_CARD_VERIFY_ERROR)
			{
				// 过滤
				XM_BreakSystemEventDefaultProcess (msg);
				return;
			}
			// 2) 检查是否是其他卡事件。若是，退出并相应该事件
			else if(msg->wp == SYSTEM_EVENT_CARD_DETECT
				||	msg->wp == SYSTEM_EVENT_CARD_UNPLUG
				||	msg->wp == SYSTEM_EVENT_CARD_INSERT_WRITE_PROTECT
				||	msg->wp == SYSTEM_EVENT_CARD_INSERT
				||	msg->wp == SYSTEM_EVENT_CARD_INVALID)
			{
				XM_BreakSystemEventDefaultProcess (msg);
				XM_PullWindow (0);
				AP_PostSystemEvent (msg->wp);
				return;				
			}
			// 3) 其他类型事件，有缺省系统事件处理过程处理
			break;

		case APP_CARDVIEW_CUSTOM_LOWSPACE:			// “循环记录空间低”定制模式
			if(msg->wp == SYSTEM_EVENT_CARD_UNPLUG)
			{
				// 检测到卡拔出
				// 返回到调用者窗口
				XM_BreakSystemEventDefaultProcess (msg);
				XM_PullWindow (0);
				return;
			}
			break;

		case APP_CARDVIEW_CUSTOM_DEFAULT:			// “格式化”定制模式
			// 不对卡插拔事件处理，仅刷新显示，提示不同的信息
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;
	}		
}


// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (CardView)
	XM_ON_MESSAGE (XM_PAINT, CardViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, CardViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, CardViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, CardViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, CardViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, CardViewOnCommand)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, CardViewOnSystemEvent)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, CardView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_EVENT)

