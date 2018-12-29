//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_settingview.c
//	  设置窗口
//
//	Revision history
//
//		2012.09.10	ZhuoYongHong Initial version
//
//****************************************************************************


#include "app.h"
#include "app_menuid.h"
#include "app_menuoptionview.h"

extern int	iRecordMode;
extern int  CurrentPhotoMode;
#define DEMO_MODE			0			// 演示模式(无卡插入、卡写保护或卡损坏)
#define WAITING_MODE		1			// 等待模式(延时纪录等待)
#define RECORD_MODE		2			// 记录模式



// 私有命令定义

#define	MAIN_COMMAND_RECORD		      0
#define	MAIN_COMMAND_PHOTO	             1
#define	MAIN_COMMAND_VEDIO_PLAY		2
#define	MAIN_COMMAND_EXIT		       3
// “设置”窗口按钮控件定义

#define	PHOTOBTNCOUNT	2
static const TPBUTTONINFO PHOTOTpBtn[PHOTOBTNCOUNT] = {

    {	
		780,200,848,300,MAIN_COMMAND_PHOTO,	MAIN_COMMAND_PHOTO,	AP_ID_PHOTO,	AP_ID_PHOTO
	},
	
	{	
		783,360,848,480,MAIN_COMMAND_EXIT,	MAIN_COMMAND_EXIT,	AP_ID_BUTTON_RETURN,	AP_ID_BUTTON_RETURN
	},
	
};



typedef struct tagPHOTODATA {
	int				nTopItem;					// 第一个可视的菜单项
	int				nCurItem;					// 当前选择的菜单项
	int				nItemCount;					// 菜单项个数
	int				nTouchItem;					// 当前触摸项, -1 表示没有

	APPMENUOPTION	menuOption;					// 菜单选项

	XMBUTTONCONTROL	btnControl;				// 按钮控件

	TPBUTTONCONTROL	TPbtnControl;				// 按钮控件

	//BYTE				bTimer;				  // 定时器

} PHOTODATA;


VOID PhotoOnEnter (XMMSG *msg)
{
	PHOTODATA *photoData;
	XMPOINT pt;
	XM_printf ("MainViewOnEnter\n");
	CurrentPhotoMode=1;
	if(msg->wp == 0)
	{
		// 窗口未建立，第一次进入
	
		// 分配私有数据句柄
		photoData = XM_calloc (sizeof(PHOTODATA));
		if(photoData == NULL)
		{
			XM_printf ("settingViewData XM_calloc failed\n");
			// 失败返回到桌面
			XM_PullWindow (0);
			return;
		}

		pt.x = 0;
		pt.y = 0;
		
		// 按钮控件初始化
		AP_TpButtonControlInit (&photoData->TPbtnControl, PHOTOBTNCOUNT, XMHWND_HANDLE(Photo), PHOTOTpBtn);
	
		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(Photo), photoData);

	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("Setting Pull\n");
	}
	
	// 创建定时器，用于时间刷新
	// 创建1秒的定时器
//	XM_SetTimer (XMTIMER_SETTINGVIEW, 1000);
}

VOID PhotoOnLeave (XMMSG *msg)
{
	if (msg->wp == 0)
	{
		// 窗口退出，彻底摧毁。
		// 获取私有数据句柄
		PHOTODATA *photoData = (PHOTODATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(Photo));
		// 释放所有分配的资源
		if(photoData)
		{
		      	// 按钮控件退出过程
			AP_TpButtonControlExit (&photoData->TPbtnControl);
			
			// 按钮控件退出过程
			AP_ButtonControlExit (&photoData->btnControl);
		

			// 释放私有数据句柄
			XM_free (photoData);
		}
		// 设置窗口的私有数据句柄为空
		XM_SetWindowPrivateData (XMHWND_HANDLE(Photo), NULL);
		XM_printf ("Main Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("Main Push\n");
	}

	XM_KillTimer (XMTIMER_SETTINGVIEW);
}


VOID PhotoOnPaint (XMMSG *msg)
{
	int nItem, nLastItem;
	XMRECT rc, rect, rcArrow;
	int i;
	int nVisualCount;
	unsigned int ticket = XM_GetTickCount();
	HANDLE hWnd = XMHWND_HANDLE(Photo);
//	XM_IMAGE *image;

	float scale_factor;		// 水平缩放因子
//	APPROMRES *AppRes;
	//XMSYSTEMTIME SystemTime;
	XMCOLOR bkg_color = XM_GetSysColor(XM_COLOR_DESKTOP);
	unsigned int old_alpha;
	DWORD menuID;

	PHOTODATA *photoData;
	
	XMCOORD menu_name_x, menu_data_x, menu_flag_x;	// 菜单项标题、数值、标识的x坐标

	photoData = (PHOTODATA *)XM_GetWindowPrivateData (hWnd);

	if(photoData == NULL)
		return;
	old_alpha = XM_GetWindowAlpha (hWnd);
	//XM_SetWindowAlpha (hWnd, 255);
		XM_GetDesktopRect (&rc);
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;

	// 计算水平缩放因子(UI按320X240规格设计)
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);
      XM_SetWindowAlpha (hWnd, 0);
	XM_FillRect (hWnd, rc.left, rc.top, rc.right, rc.bottom, bkg_color);

	old_alpha = XM_GetWindowAlpha (hWnd);
	XM_SetWindowAlpha (hWnd, 255);

	// 处理按钮控件的显示。
	// 若存在按钮控件，必须调用AP_ButtonControlMessageHandler执行按钮控件显示
	AP_TpButtonControlMessageHandler (&photoData->TPbtnControl, msg);

	XM_SetWindowAlpha (hWnd, (unsigned char)old_alpha);
}






VOID PhotoOnKeyDown (XMMSG *msg)
{
	PHOTODATA *photoData;
	int nVisualCount;
	XMRECT rc;

	photoData = (PHOTODATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(Photo));
	if(photoData == NULL)
		return;

	XM_GetWindowRect (XMHWND_HANDLE(Photo), &rc);
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;

	if(msg->message == XM_KEYDOWN)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_SWITCH:
			case VK_AP_MODE:
				// 在"设置"窗口中，MENU、SWITCH及MODE键被定义为按钮操作。
				// 此处将这三个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
				AP_ButtonControlMessageHandler (&photoData->btnControl, msg);
				break;
		}
	}
}

VOID PhotoOnKeyUp (XMMSG *msg)
{
	PHOTODATA *photoData;

	photoData = (PHOTODATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(Photo));
	if(photoData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:
		case VK_AP_SWITCH:
		case VK_AP_MODE:
			// 在"设置"窗口中，MENU、SWITCH及MODE键被定义为按钮操作。
			// 此处将这三个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&photoData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&photoData->btnControl, msg);
			break;
	}
}

VOID PhotoOnTimer (XMMSG *msg)
{
	XM_PostMessage (XM_KEYDOWN, VK_AP_DOWN, 0);
	//SettingDisplayDateTime ();
}

// 文件系统无法访问错误回调函数
static void fs_access_error_alert_cb_main (void *userPrivate, unsigned int uKeyPressed)
{
	if(uKeyPressed == VK_AP_MENU)		// “格式化”按键
	{
		// 格式化操作
		AP_OpenFormatView (0);
	}
	else					// “取消”按键
	{
		// 返回到调用者窗口
		XM_PullWindow (0);
	}
}




VOID PhotoOnCommand (XMMSG *msg)
{
	PHOTODATA *photoData;
	photoData = (PHOTODATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(Photo));
	if(photoData == NULL)
		return;
	
	switch(msg->wp)
	
	{   case MAIN_COMMAND_PHOTO:
	         //  if(iRecordMode == RECORD_MODE)
			{
			  
				AP_OnekeyPhotograph ();
			}
			break;
	

		case MAIN_COMMAND_RECORD:
			// 切换到“录像设置”窗口
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP);
			XM_JumpWindowEx (XMHWND_HANDLE(VideoSettingView), 0, XM_JUMP_POPDEFAULT);
			break;
			
        case MAIN_COMMAND_EXIT:
			// 切换到“DESKTOP”窗口
			// 切换到“录像设置”窗口
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP);
			XM_JumpWindowEx (XMHWND_HANDLE(MainView), 0, XM_JUMP_POPDEFAULT);
			break;
			   	
		/*	
		case SETTING_COMMAND_RETURN:
			// 返回到桌面
			XM_PullWindow (0);
			break;
		*/
	}
}

VOID PhotoOnTouchDown (XMMSG *msg)
{
	int index;
	unsigned int x, y;
	HANDLE hWnd = XMHWND_HANDLE(Photo);
	PHOTODATA *photoData = (PHOTODATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(Photo));
   
	if(photoData == NULL)
		return;

	if(AP_TpButtonControlMessageHandler (&photoData->TPbtnControl, msg))
		return;
}

VOID PhotoOnTouchUp (XMMSG *msg)
{
    unsigned int x, y;
	PHOTODATA *photoData;
	photoData = (PHOTODATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(Photo));
	if(photoData == NULL)
		return;
	
	x = LOWORD(msg->lp);
	y = HIWORD(msg->lp);

	if(AP_TpButtonControlMessageHandler (&photoData->TPbtnControl, msg))
		return;
}



// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (Photo)
	XM_ON_MESSAGE (XM_PAINT, PhotoOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, PhotoOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, PhotoOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, PhotoOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, PhotoOnLeave)
	XM_ON_MESSAGE (XM_TIMER, PhotoOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, PhotoOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, PhotoOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, PhotoOnTouchUp)

XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, Photo, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)


