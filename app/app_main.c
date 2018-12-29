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
extern int		iRecordMode;
#define DEMO_MODE			0			// 演示模式(无卡插入、卡写保护或卡损坏)
#define WAITING_MODE		1			// 等待模式(延时纪录等待)
#define RECORD_MODE		2			// 记录模式



// 私有命令定义

#define	MAIN_COMMAND_PHOTO		      0
#define	MAIN_COMMAND_RETURN		             1
#define	MAIN_COMMAND_VEDIO_PLAY		2
#define	MAIN_COMMAND_MENU		       3
// “设置”窗口按钮控件定义
#define	MAINBTNCOUNT	4
static const TPBUTTONINFO MianTpBtn[MAINBTNCOUNT] = {
	
	{	
		20,150,220,439,VK_AP_SWITCH,	MAIN_COMMAND_RETURN,	AP_ID_MAIN_VIDEO_UP,	AP_ID_MAIN_VIDEO_DOWN
	},
        {	
		220,150,420,439,VK_AP_MENU,		MAIN_COMMAND_PHOTO,	AP_ID_MAIN_PHOTO_UP,	AP_ID_MAIN_PHOTO_DOWN
	},
	{	
		420,150,620,439,VK_AP_MODE,     MAIN_COMMAND_VEDIO_PLAY,	AP_ID_MAIN_REPLAY_UP,	AP_ID_MAIN_REPLAY_DOWN
	},
	{	
		620,150,820,439,VK_AP_MENU,     MAIN_COMMAND_MENU,	AP_ID_MAIN_SET_UP,	AP_ID_MAIN_SET_DOWN
	},
	
	
};



typedef struct tagMAINVIEWDATA {
	int				nTopItem;					// 第一个可视的菜单项
	int				nCurItem;					// 当前选择的菜单项
	int				nItemCount;					// 菜单项个数
	int				nTouchItem;					// 当前触摸项, -1 表示没有
       XMSYSTEMTIME		dateTime;	
	APPMENUOPTION	menuOption;					// 菜单选项

	XMBUTTONCONTROL	btnControl;				// 按钮控件

	TPBUTTONCONTROL	TPbtnControl;				// 按钮控件


//	BYTE				bTimer;						// 定时器

} MAINVIEWDATA;


VOID MainViewOnEnter (XMMSG *msg)
{
	MAINVIEWDATA *settingViewData;
	XMPOINT pt;
	XM_printf ("MainViewOnEnter\n");
	if(msg->wp == 0)
	{
		// 窗口未建立，第一次进入
	
		// 分配私有数据句柄
		settingViewData = XM_calloc (sizeof(MAINVIEWDATA));
		if(settingViewData == NULL)
		{
			XM_printf ("settingViewData XM_calloc failed\n");
			// 失败返回到桌面
			XM_PullWindow (0);
			return;
		}

		// 初始化私有数据
		/*settingViewData->nCurItem = 0;
		settingViewData->nItemCount = SETTING_MENU_COUNT;
		settingViewData->nTouchItem = -1;*/

		pt.x = 0;
		pt.y = 0;
		
	
		// 获取当前系统时间作为时间设置的初始值。若时间已丢失(RTC掉电)，系统会自动设置为某个缺省时间。如20130101
		XM_GetLocalTime (&settingViewData->dateTime);

		// 按钮控件初始化
		AP_TpButtonControlInit (&settingViewData->TPbtnControl, MAINBTNCOUNT, XMHWND_HANDLE(MainView), MianTpBtn);
	
		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(MainView), settingViewData);

	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("Setting Pull\n");
	}
	
	// 创建定时器，用于时间刷新
	// 创建1秒的定时器
	XM_SetTimer (XMTIMER_SETTINGVIEW, 1000);
}

VOID MainViewOnLeave (XMMSG *msg)
{
	if (msg->wp == 0)
	{
		// 窗口退出，彻底摧毁。
		// 获取私有数据句柄
		MAINVIEWDATA *settingViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MainView));
		// 释放所有分配的资源
		if(settingViewData)
		{
		      	// 按钮控件退出过程
			AP_TpButtonControlExit (&settingViewData->TPbtnControl);
			
			// 按钮控件退出过程
			//AP_ButtonControlExit (&settingViewData->btnControl);
		

			// 释放私有数据句柄
			XM_free (settingViewData);
		}
		// 设置窗口的私有数据句柄为空
		XM_SetWindowPrivateData (XMHWND_HANDLE(MainView), NULL);
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


VOID MainViewOnPaint (XMMSG *msg)
{
     
	XMCOORD x, y,x2,y2;

	unsigned int old_alpha;
	char text[32];
	char text2[32];
	XMSIZE size;
	XMRECT rc, rect, rcArrow;

	unsigned int ticket = XM_GetTickCount();
	HANDLE hWnd = XMHWND_HANDLE(MainView);
	XMCOLOR bkg_color = XM_GetSysColor(XM_COLOR_WINDOWTEXT);
	MAINVIEWDATA *MainViewData;
	MainViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (hWnd);
	if(MainViewData == NULL)
		return;
	old_alpha = XM_GetWindowAlpha (hWnd);
	XM_GetDesktopRect (&rc);
	XM_FillRect (hWnd, rc.left, rc.top, rc.right, rc.bottom, bkg_color);
	old_alpha = XM_GetWindowAlpha (hWnd);
	XM_SetWindowAlpha (hWnd, 255);

//////////////
        XM_GetLocalTime (&MainViewData->dateTime);

	// 格式2： 输出日期 "2012 / 09 / 16"
	AP_FormatDataTime (&MainViewData->dateTime, APP_DATETIME_FORMAT_3, text, sizeof(text));
	AP_TextGetStringSize (text, strlen(text), &size);
	x = 400;
	y = 20;
	// 格式4： 输出时分秒 "10 : 22 : 20"
	AP_FormatDataTime (&MainViewData->dateTime, APP_DATETIME_FORMAT_7, text2, sizeof(text2));
	AP_TextGetStringSize (text2, strlen(text2), &size);
	x2 = 350;
	y2 =60;
	// 格式2： 输出日期 "2012 / 09 / 16"
	AP_TextOutDataTimeString (XMHWND_HANDLE(MainView), x, y, text, strlen(text));
	// 格式4： 输出时分秒 "10 : 22 : 20"
	AP_TextOutDataTimeString (XMHWND_HANDLE(MainView), x2, y2, text2, strlen(text2));
////////////

	// 处理按钮控件的显示。
	// 若存在按钮控件，必须调用AP_ButtonControlMessageHandler执行按钮控件显示
	AP_TpButtonControlMessageHandler (&MainViewData->TPbtnControl, msg);

	XM_SetWindowAlpha (hWnd, (unsigned char)old_alpha);
     
	
}






VOID MainViewOnKeyDown (XMMSG *msg)
{
	MAINVIEWDATA *settingViewData;
	int nVisualCount;
	XMRECT rc;

	settingViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MainView));
	if(settingViewData == NULL)
		return;

	XM_GetWindowRect (XMHWND_HANDLE(MainView), &rc);
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
// 按键音
	XM_Beep (XM_BEEP_KEYBOARD);
	if(msg->message == XM_KEYDOWN)
	{
		switch(msg->wp)
		{
		    #if 0
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
                          #endif
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

VOID MainViewOnKeyUp (XMMSG *msg)
{
	MAINVIEWDATA *settingViewData;

	settingViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MainView));
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

VOID MainViewOnTimer (XMMSG *msg)
{
     static BYTE PreMinute=0;
    XMSYSTEMTIME Currenttime;
     XM_GetLocalTime (&Currenttime);
	 //Currenttime.bDayOfWeek 
      if(PreMinute!=Currenttime.bMinute)
      	{
           PreMinute=Currenttime.bMinute;
	    XM_InvalidateWindow ();
	    XM_UpdateWindow ();
	 }

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




VOID MainViewOnCommand (XMMSG *msg)
{
	MAINVIEWDATA *settingViewData;
	settingViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MainView));
	if(settingViewData == NULL)
		return;
	switch(msg->wp)
	
	{   case MAIN_COMMAND_RETURN:
	           XM_PullWindow (0);
				break;
		case MAIN_COMMAND_VEDIO_PLAY:
		        	// 切换到录像回放模式
			if(iRecordMode == DEMO_MODE)
			{
				// 无法切换到视频预览模式，弹出信息窗口，提示原因
				unsigned int card_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
				if(card_state == DEVCAP_SDCARDSTATE_UNPLUG)
				{
					// 卡已拔出
					XM_OpenAlertView (	
						AP_ID_CARD_MESSAGE_CARD_WITHDRAW,	// 信息文本资源ID
						AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
						0,
						0,												// 按钮文字资源ID
						0,												// 按钮按下文字资源ID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10秒钟自动关闭
						APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
						NULL,											// 按键回调函数
						NULL,											// 用户回调函数私有参数
						XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
						//XM_VIEW_ALIGN_CENTRE,
						0												// ALERTVIEW视图的控制选项
						);
				}
				else if(card_state == DEVCAP_SDCARDSTATE_FS_ERROR)
				{
					DWORD dwButtonNormalTextID[2];
					DWORD dwButtonPressedTextID[2];
					dwButtonNormalTextID[0] = AP_ID_ALERT_FORMAT_NORMAL;
					dwButtonNormalTextID[1] = AP_ID_ALERT_CANCEL_NORMAL;
					dwButtonPressedTextID[0] = AP_ID_ALERT_FORMAT_PRESSED;
					dwButtonPressedTextID[1] = AP_ID_ALERT_CANCEL_PRESSED;

					// 卡文件系统错误
					XM_OpenAlertView (	
						AP_ID_CARD_MESSAGE_CARD_FSERROR,		// 信息文本资源ID
						AP_ID_CARD_ICON_SDCARD,												// 图片信息资源ID
						2,												// 按钮个数
						dwButtonNormalTextID,					// 按钮文字资源ID
						dwButtonPressedTextID,					// 按钮按下文字资源ID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10秒钟自动关闭
						APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
						fs_access_error_alert_cb_main,							// 按键回调函数
						NULL,											// 用户回调函数私有参数
						XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
						//XM_VIEW_ALIGN_CENTRE,
						XM_ALERTVIEW_OPTION_ENABLE_CALLBACK	// ALERTVIEW视图的控制选项
						);
				}
				else if(card_state == DEVCAP_SDCARDSTATE_INVALID)
				{
					// 卡无效(卡无法识别)
					XM_OpenAlertView (	
						AP_ID_CARD_MESSAGE_CARD_INVALID,	// 信息文本资源ID
						AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
						0,
						0,												// 按钮文字资源ID
						0,												// 按钮按下文字资源ID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10秒钟自动关闭
						APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
						NULL,											// 按键回调函数
						NULL,											// 用户回调函数私有参数
						XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
						//XM_VIEW_ALIGN_CENTRE,
						0												// ALERTVIEW视图的控制选项
						);
				}	
				else 
				{
					// 检查视频项列表是否正在扫描中.
					// 若正在扫描, 弹出一POP视窗提示用户"正在扫描视频文件,请稍等"
					if(XM_VideoItemIsBasicServiceOpened() && !XM_VideoItemIsServiceOpened())
					{
						XM_OpenAlertView (	
							AP_ID_VIDEO_VIEW_SCAN_VIDEO_FILE,	// 信息文本资源ID
							AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
							0,
							0,												// 按钮文字资源ID
							0,												// 按钮按下文字资源ID
							APP_ALERT_BKGCOLOR,
							60.0,											// 60秒钟自动关闭
							APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
							NULL,											// 按键回调函数
							NULL,											// 用户回调函数私有参数
							XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW视图的控制选项
							);
						
					}
					else
					{
						// 卡插入(读写模式、写保护模式)
						APPMarkCardChecking (1);		// 终止视频通道
	
						XM_PushWindow (XMHWND_HANDLE(VideoListView));
					}
				}
			}
			else
			{
				// 检查视频项列表是否正在扫描中.
				// 若正在扫描, 弹出一POP视窗提示用户"正在扫描视频文件,请稍等"
				if(XM_VideoItemIsBasicServiceOpened() && !XM_VideoItemIsServiceOpened())
				{
					XM_OpenAlertView (	
							AP_ID_VIDEO_VIEW_SCAN_VIDEO_FILE,	// 信息文本资源ID
							AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
							0,
							0,												// 按钮文字资源ID
							0,												// 按钮按下文字资源ID
							APP_ALERT_BKGCOLOR,
							60.0,											// 60秒钟自动关闭
							APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
							NULL,											// 按键回调函数
							NULL,											// 用户回调函数私有参数
							XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW视图的控制选项
							);
						
				}
				else
				{
					APPMarkCardChecking (1);		// 终止视频通道

					//XM_PushWindow (XMHWND_HANDLE(VideoListView));
					XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP);
			              XM_JumpWindowEx (XMHWND_HANDLE(VideoListView), 0, XM_JUMP_POPDEFAULT);
				}
			}

			break;
			

		case MAIN_COMMAND_PHOTO:
			// 切换到“录像设置”窗口
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP);
			//XM_JumpWindowEx (XMHWND_HANDLE(Photo), 0, XM_JUMP_POPDEFAULT);
			break;
               case MAIN_COMMAND_MENU:
			   	// 切换到“设置”窗口
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP);
			XM_JumpWindowEx (XMHWND_HANDLE(SettingView), 0, XM_JUMP_POPDEFAULT);
			break;
			   	
	/*	case SETTING_COMMAND_RETURN:
			// 返回到桌面
			XM_PullWindow (0);
			break;*/
	}
}

VOID MainViewOnTouchDown (XMMSG *msg)
{
	int index;
	unsigned int x, y;
	HANDLE hWnd = XMHWND_HANDLE(MainView);
	MAINVIEWDATA *settingViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MainView));
   
	if(settingViewData == NULL)
		return;

	if(AP_TpButtonControlMessageHandler (&settingViewData->TPbtnControl, msg))
		return;
    
    #if 1
	index = AppLocateItem (XMHWND_HANDLE(MainView), settingViewData->nItemCount, APP_POS_ITEM5_LINEHEIGHT, settingViewData->nTopItem, LOWORD(msg->lp), HIWORD(msg->lp));
	if(index < 0)
		return;
	settingViewData->nTouchItem = index;
	if(settingViewData->nCurItem != index)
	{
		settingViewData->nCurItem = index;
		XM_InvalidateWindow ();
		XM_UpdateWindow ();
	}
    #endif
}

VOID MainViewOnTouchUp (XMMSG *msg)
{
	MAINVIEWDATA *settingViewData;
	settingViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MainView));
	if(settingViewData == NULL)
		return;

	if(AP_TpButtonControlMessageHandler (&settingViewData->TPbtnControl, msg))
		return;
	
	//XM_PostMessage (XM_COMMAND, settingViewData->TPbtnControl.pBtnInfo[0].wCommand, settingViewData->nCurItem);
}

VOID MainViewOnTouchMove (XMMSG *msg)
{
	MAINVIEWDATA *settingViewData;
	settingViewData = (MAINVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SystemSettingView));
	if(settingViewData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg))
		return;

}

// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (MainView)
	XM_ON_MESSAGE (XM_PAINT, MainViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, MainViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, MainViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, MainViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, MainViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, MainViewOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, MainViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, MainViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, MainViewOnTouchUp)
	XM_ON_MESSAGE (XM_TOUCHMOVE, MainViewOnTouchMove)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, MainView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)


