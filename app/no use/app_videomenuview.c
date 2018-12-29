//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_videomenuview.c
//	  视频菜单
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_videolist.h"


#include "app_menuoptionview.h"
#include "app_menuid.h"
#include "xm_app_menudata.h"
#include "system_check.h"

// “菜单选项”窗口按钮控件定义
#define	VIDEOMENUVIEWBTNCOUNT	2
// 私有命令定义
#define	VIDEOMENUVIEW_COMMAND_OK				0
#define	VIDEOMENUVIEW_COMMAND_CANCEL		1
static const XMBUTTONINFO videoMenuViewBtn[VIDEOMENUVIEWBTNCOUNT] = {
	{	
		VK_AP_MENU,		VIDEOMENUVIEW_COMMAND_OK,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
	},
	{	
		VK_AP_MODE,		VIDEOMENUVIEW_COMMAND_CANCEL,	AP_ID_COMMON_MODE,	AP_ID_BUTTON_CANCEL
	},
};

#define	VIDEOMENUVIEW_COMMANDITEM_COUNT		3

// 菜单项定义
static const int commandItem[VIDEOMENUVIEW_COMMANDITEM_COUNT] = {
	AP_ID_VIDEO_LOCKVIDEO,			// 锁定视频
	AP_ID_VIDEO_UNLOCKVIDEO,		// 解锁视频
	AP_ID_VIDEO_PLAY				// 播放视频
};

typedef struct tagVIDEOMENUVIEWDATA {
	int				nTopItem;					// 第一个可视的菜单项
	int				nCurItem;					// 当前选择的菜单项
	int				nItemCount;					// 菜单项个数

	BYTE				mode;
	WORD				wVideoFileIndex;

	XMBUTTONCONTROL	btnControl;				// 按钮控件
} VIDEOMENUVIEWDATA;


VOID AP_OpenVideoMenuView (BYTE mode, WORD wVideoFileIndex)
{
	XM_PushWindowEx (XMHWND_HANDLE(VideoMenuView), (mode << 24) | wVideoFileIndex);
}



VOID VideoMenuViewOnEnter (XMMSG *msg)
{
	XM_printf(">>>>>>>>VideoMenuViewOnEnter, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	if(msg->wp == 0)
	{
		VIDEOMENUVIEWDATA *videoMenuData = XM_calloc (sizeof(VIDEOMENUVIEWDATA));
		if(videoMenuData == NULL)
		{
			XM_printf ("videoMenuData XM_calloc failed\n");
			// 失败返回到桌面
			XM_PullWindow (0);
			return;
		}

		videoMenuData->mode = (BYTE)(msg->lp >> 24);
		videoMenuData->wVideoFileIndex = (WORD)msg->lp;
		
		videoMenuData->nItemCount = VIDEOMENUVIEW_COMMANDITEM_COUNT;
		videoMenuData->nCurItem = 0;
		videoMenuData->nTopItem = 0;

		// 按钮控件初始化
		AP_ButtonControlInit (&videoMenuData->btnControl, VIDEOMENUVIEWBTNCOUNT, XMHWND_HANDLE(VideoMenuView), videoMenuViewBtn);

		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(VideoMenuView), videoMenuData);

	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("VideoMenuView Pull\n");
	}
	// 创建定时器，用于卡拔出检测
	// 创建0.5秒的定时器
	XM_SetTimer (XMTIMER_VIDEOMENUVIEW, 500);
}

VOID VideoMenuViewOnLeave (XMMSG *msg)
{
	XM_KillTimer (XMTIMER_VIDEOMENUVIEW);
	if (msg->wp == 0)
	{
		VIDEOMENUVIEWDATA *videoMenuData = (VIDEOMENUVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoMenuView));
		if(videoMenuData)
		{
			// 按钮控件退出过程
			AP_ButtonControlExit (&videoMenuData->btnControl);

			// 释放私有数据句柄
			XM_free (videoMenuData);
			
		}
		XM_printf ("VideoMenuView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("VideoMenuView Push\n");
	}
}



VOID VideoMenuViewOnPaint (XMMSG *msg)
{
	int nItem;
	XMRECT rc, rect;
	APPROMRES *AppRes;
	int i;
	char text[32];
	HANDLE hVideoItem;
	XMVIDEOITEM *pVideoItem;

	XM_printf(">>>>>>>>VideoMenuViewOnPaint, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
	
	VIDEOMENUVIEWDATA *videoMenuData = (VIDEOMENUVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoMenuView));
	if(videoMenuData == NULL)
		return;

	// 显示标题栏
	XM_GetDesktopRect (&rc);
	XM_FillRect (XMHWND_HANDLE(VideoMenuView), rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_WINDOW));

	// --------------------------------------
	//
	// ********* 1 显示标题栏区信息 *********
	//
	// --------------------------------------
	AP_DrawTitlebarControl (XMHWND_HANDLE(VideoMenuView), AP_NULLID, AP_ID_VIDEO_TITLE_COMMAND);



	// 显示菜单项水平分割线
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMSPLITLINE);
	rect = rc;
	for (i = 0; i < VIDEOMENUVIEW_COMMANDITEM_COUNT; i++)
	{
		rect.left = APP_POS_ITEM5_SPLITLINE_X;
		rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + (i + 1) * APP_POS_ITEM5_LINEHEIGHT);
		XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(VideoMenuView), &rect, XMGIF_DRAW_POS_LEFTTOP);
	}

	// 填充当前选择项背景
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMBACKGROUND);
	rect = rc;
	rect.left = 0;
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (videoMenuData->nCurItem - videoMenuData->nTopItem + 1) * APP_POS_ITEM5_LINEHEIGHT);
	XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(VideoMenuView), &rect, XMGIF_DRAW_POS_LEFTTOP);

	// 显示当前选择的视频文件名
	// if(AP_VideoListFormatVideoFileName (videoMenuData->wVideoFileIndex, text, sizeof(text)))
	hVideoItem = AP_VideoItemGetVideoItemHandle (videoMenuData->mode, videoMenuData->wVideoFileIndex);
	pVideoItem = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
	if(AP_VideoItemFormatVideoFileName (pVideoItem, text, sizeof(text)))
	{
		rect.left = APP_POS_VIDEOMENUVIEW_FILENAME_X;
		rect.top = APP_POS_VIDEOMENUVIEW_FILENAME_Y;
		AP_TextOutDataTimeString (XMHWND_HANDLE(VideoMenuView), 
			APP_POS_VIDEOMENUVIEW_FILENAME_X, APP_POS_VIDEOMENUVIEW_FILENAME_Y, text, 20);
	}

	// 显示菜单项目 
	rect.top = APP_POS_ITEM5_SPLITLINE_Y + 1 + APP_POS_ITEM5_LINEHEIGHT;
	rect.bottom = APP_POS_ITEM5_SPLITLINE_Y + APP_POS_ITEM5_LINEHEIGHT + APP_POS_ITEM5_LINEHEIGHT;
	for (nItem = videoMenuData->nTopItem; nItem < videoMenuData->nItemCount; nItem ++)
	{
		AppRes = AP_AppRes2RomRes (commandItem[nItem]);
		XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(VideoMenuView), &rect, XMGIF_DRAW_POS_CENTER);
		XM_OffsetRect (&rect, 0, APP_POS_ITEM5_LINEHEIGHT);
	}

	// --------------------------------------
	//
	// ********* 3 显示按钮区 ***************
	//
	// --------------------------------------
	// 处理按钮控件的显示。
	// 若存在按钮控件，必须调用AP_ButtonControlMessageHandler执行按钮控件显示
	AP_ButtonControlMessageHandler (&videoMenuData->btnControl, msg);

}


VOID VideoMenuViewOnKeyDown (XMMSG *msg)
{
	XM_printf(">>>>>>>>VideoMenuViewOnKeyDown, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	VIDEOMENUVIEWDATA *videoMenuData = (VIDEOMENUVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoMenuView));
	if(videoMenuData == NULL)
		return;

	switch(msg->wp)
	{
		// 向上键
		case VK_AP_UP:
			AP_ButtonControlMessageHandler (&videoMenuData->btnControl, msg);
			videoMenuData->nCurItem --;
			if(videoMenuData->nCurItem < 0)
			{
				// 聚焦到最后一个
				videoMenuData->nCurItem = (WORD)(videoMenuData->nItemCount - 1);
				videoMenuData->nTopItem = 0;
				while ( (videoMenuData->nCurItem - videoMenuData->nTopItem) >= VIDEOMENUVIEW_COMMANDITEM_COUNT )
				{
					videoMenuData->nTopItem ++;
				}
			}
			else
			{
				if(videoMenuData->nTopItem > videoMenuData->nCurItem)
					videoMenuData->nTopItem = videoMenuData->nCurItem;
			}
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		// 向下键
		case VK_AP_DOWN:
			AP_ButtonControlMessageHandler (&videoMenuData->btnControl, msg);
			videoMenuData->nCurItem ++;	
			if(videoMenuData->nCurItem >= videoMenuData->nItemCount)
			{
				// 聚焦到第一个
				videoMenuData->nTopItem = 0;
				videoMenuData->nCurItem = 0;
			}
			else
			{
				while ( (videoMenuData->nCurItem - videoMenuData->nTopItem) >= VIDEOMENUVIEW_COMMANDITEM_COUNT )
				{
					videoMenuData->nTopItem ++;
				}
			}

			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_MENU:
		case VK_AP_MODE:
			// 此处将这三个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&videoMenuData->btnControl, msg);
			break;

	}
}

VOID VideoMenuViewOnKeyUp (XMMSG *msg)
{
	VIDEOMENUVIEWDATA *videoMenuData = (VIDEOMENUVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoMenuView));
	if(videoMenuData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:
		case VK_AP_MODE:
			// 此处将这三个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&videoMenuData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&videoMenuData->btnControl, msg);
			break;

	}
}

VOID VideoMenuViewOnCommand (XMMSG *msg)
{
	VIDEOMENUVIEWDATA *videoMenuData;
	HANDLE hVideoItem;
	XMVIDEOITEM *pVideoItem;
	int card_state;
	//char FullPathName[64];
	videoMenuData = (VIDEOMENUVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoMenuView));
	if(videoMenuData == NULL)
		return;
	switch(msg->wp)
	{
		case VIDEOMENUVIEW_COMMAND_OK:
			if(videoMenuData->nCurItem == 0)
			{
				// 锁定视频
				// 检查是否写保护
				card_state = APSYS_CardChecking();
				if(card_state == APP_SYSTEMCHECK_SUCCESS)
				{
					// AP_VideoListLockVideoFile (videoMenuData->wVideoFileIndex);
					hVideoItem = AP_VideoItemGetVideoItemHandle (videoMenuData->mode, videoMenuData->wVideoFileIndex);
					pVideoItem = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
					AP_VideoItemLockVideoFile (pVideoItem);
					// 确认并返回
					XM_PullWindow (NULL);
				}
				else if(card_state == APP_SYSTEMCHECK_CARD_WRITEPROTECT)
				{
					// 切换到信息提示窗口,提示"卡写保护"
					// 信息提示窗口按键返回或定时器自动返回 
					AP_OpenMessageView (AP_ID_CARD_INFO_WRITEPROTECT);	// 3秒自动关闭
				}
			}
			else if(videoMenuData->nCurItem == 1)
			{
				// 解锁视频
				card_state = APSYS_CardChecking();
				if(card_state == APP_SYSTEMCHECK_SUCCESS)
				{
					// AP_VideoListUnlockVideoFile (videoMenuData->wVideoFileIndex);
					hVideoItem = AP_VideoItemGetVideoItemHandle (videoMenuData->mode, videoMenuData->wVideoFileIndex);
					pVideoItem = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
					AP_VideoItemUnlockVideoFile (pVideoItem);
					// 确认并返回
					XM_PullWindow (NULL);
				}
				else if(card_state == APP_SYSTEMCHECK_CARD_WRITEPROTECT)
				{
					// 切换到信息提示窗口,提示"卡写保护"
					// 信息提示窗口按键返回或定时器自动返回 
					AP_OpenMessageView (AP_ID_CARD_INFO_WRITEPROTECT);	// 3秒自动关闭
				}
			}
			else if(videoMenuData->nCurItem == 2)
			{
				// 播放视频

				// 确认并返回
				XM_PullWindow (NULL);
			}

			break;

		case VIDEOMENUVIEW_COMMAND_CANCEL:
			// 返回
			// 返回到桌面
			XM_PullWindow (0);
			break;
	}
}

VOID VideoMenuViewOnTimer (XMMSG *msg)
{
	// 检测卡状态
	if(APSYS_CardChecking() == APP_SYSTEMCHECK_CARD_NOCARD)
	{
		// 卡拔出，直接跳出回到桌面(桌面负责卡插拔的所有处理), 此处强制桌面重新初始化
		XM_JumpWindow (XMHWND_HANDLE(Desktop));
		return;
	}
}

// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (VideoMenuView)
	XM_ON_MESSAGE (XM_PAINT, VideoMenuViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, VideoMenuViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, VideoMenuViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, VideoMenuViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, VideoMenuViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, VideoMenuViewOnCommand)
	XM_ON_MESSAGE (XM_TIMER, VideoMenuViewOnTimer)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, VideoMenuView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
