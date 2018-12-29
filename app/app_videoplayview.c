//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_videoplayview.c
//	  视频播放窗口
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************

#include "app.h"
#include "app_videolist.h"
#include "app_video.h"


#include "app_menuid.h"
#include "xm_app_menudata.h"
#include "system_check.h"
// -----------------------------------------------
//
// *************   视频播放器   ******************
//
// -----------------------------------------------
// 视频播放器的功能
//	1) UI显示(浮动工具栏)
// 2) 发送命令(AP_VideoSendCommand)控制视频流进程的播放、暂停、继续、停止。
//
// 视频播放器的设计
// 1) 视频播放器UI的背景色为黑色。
//		显示驱动自动使用alpha因子将“视频播放器UI”与“视频流”进行混合并显示
// 2) “视频流”及“视频播放器UI”均为全屏显示
// 3) 浮动工具栏为透明色图标，共5个图标
//		允许浮动按钮弹出，支持继续/暂停、前一个、后一个、退出(模式切换)、画中画切换(视图切换)
//		浮动按钮在按键无操作10秒后自动隐藏。
//		浮动按钮隐藏后，按任何键将重新显示浮动按钮
// 4) 视频播放器从VideoListView进入。退出时返回到VideoListView
// 5) 视频播放过程中，若检测到卡拔出，直接跳转Jump到桌面(桌面负责卡插拔的所有处理)
// 6) 播放结束后，视频流进程发送XM_VIDEOSTOP消息，通知播放结束。
//		退出码中定义结束的原因(app_video.h)

#define	TIMETOHIDETOOLBAR		10*2		// 10秒

// 视频播放器状态定义
enum {
	VIDEOPLAYSTATE_STOP = 0,			// 停止状态
	VIDEOPLAYSTATE_PLAY,					// 播放状态
	VIDEOPLAYSTATE_PAUSE,				// 暂停状态
};

typedef struct tagVIDEOPLAYVIEWDATA {
	BYTE		mode;							// 循环模式还是保护模式
	DWORD		wVideoFileIndex;			// 正在播放视频文件的索引序号
	BYTE		bVideoPlayState;			// 视频播放器状态
	XMBOOL	bToolbarShow;				//	1 表示浮动工具栏显示
												// 0 表示浮动工具栏隐藏
	BYTE		bTimeToHideToolbar;		// 浮动工具栏可显示的剩余时间(秒为单位)
												//		每次定时器事件到达时将计数器减一。
												//		为0时隐藏浮动工具栏
} VIDEOPLAYVIEWDATA;

// 打开视频播放视图
VOID AP_OpenVideoPlayView (BYTE mode, WORD wVideoFileIndex)
{
	XM_PushWindowEx (XMHWND_HANDLE(VideoPlayView), (mode << 24) | wVideoFileIndex);
}

// 显示工具栏
static void ShowToolbar (VIDEOPLAYVIEWDATA *videoMenuData)
{
	videoMenuData->bToolbarShow = 1;
	videoMenuData->bTimeToHideToolbar = TIMETOHIDETOOLBAR;
}

static XMBOOL StopVideo (VIDEOPLAYVIEWDATA *videoMenuData)
{
	XMMSG msg;
	AP_VideoSendCommand (AP_VIDEOCOMMAND_STOP, 0);
	videoMenuData->bVideoPlayState = VIDEOPLAYSTATE_STOP;
	// 处理退出消息
	XM_PeekMessage (&msg, XM_VIDEOSTOP, XM_VIDEOSTOP);
	return 1;
}


static XMBOOL PlayVideo (VIDEOPLAYVIEWDATA *videoMenuData)
{
	// 终止当前视频播放
	HANDLE hVideoItem;
	XMVIDEOITEM *pVideoItem;
	VIDEO_PLAY_PARAM playParam;
	char videoPath[MAX_APP_VIDEOPATH]; 
	char videoPath2[MAX_APP_VIDEOPATH]; 
	XMBOOL ret = 0;

	if(videoMenuData->bVideoPlayState == VIDEOPLAYSTATE_STOP)
	{
		// 停止状态
		// 获取视频文件全路径名
		hVideoItem = AP_VideoItemGetVideoItemHandle (videoMenuData->mode, videoMenuData->wVideoFileIndex);
		pVideoItem = AP_VideoItemGetVideoItemFromHandle (hVideoItem);

		memset (&videoPath, 0, sizeof(videoPath));
		memset (&videoPath2, 0, sizeof(videoPath2));
		XM_VideoItemGetVideoFilePath (pVideoItem, 0, videoPath, MAX_APP_VIDEOPATH);
		XM_VideoItemGetVideoFilePath (pVideoItem, 1, videoPath2, MAX_APP_VIDEOPATH);
		if(videoPath[0] == 0 && videoPath2[0] == 0)
		{
			XM_printf ("PlayVideo XM_VideoItemGetVideoFilePath failed\n");
			return 0;

		}

		XM_printf ("XM_VideoItemGetVideoFilePath videoPath=%s\n", videoPath);
		if(videoPath2[0])
			XM_printf ("XM_VideoItemGetVideoFilePath videoPath2=%s\n", videoPath2);
		playParam.pMainViewVideoFilePath = videoPath;
		playParam.pMainViewVideoFilePath2 = videoPath2;


		ret = AP_VideoSendCommand (AP_VIDEOCOMMAND_PLAY, (DWORD)(&playParam));
		if(ret)
		{
			videoMenuData->bVideoPlayState = VIDEOPLAYSTATE_PLAY;
		}
	}
	else if(videoMenuData->bVideoPlayState == VIDEOPLAYSTATE_PAUSE)
	{
		// 暂停状态
		ret = AP_VideoSendCommand (AP_VIDEOCOMMAND_CONTINUE, 0);
	}
	return ret;
}

VOID VideoPlayViewOnEnter (XMMSG *msg)
{
	VIDEOPLAYVIEWDATA *videoPlayViewListData;

	XM_printf ("VideoPlayViewOnEnter %d\n", msg->wp);
	if(msg->wp == 0)
	{
		// 窗口未建立，第一次进入

		// 分配私有数据句柄
		videoPlayViewListData = XM_calloc (sizeof(VIDEOPLAYVIEWDATA));
		if(videoPlayViewListData == NULL)
		{
			XM_printf ("videoPlayViewListData XM_calloc failed\n");
			// 失败返回到调用者窗口
			XM_PullWindow (0);
			return;
		}
		
		videoPlayViewListData->mode = (BYTE)(msg->lp >> 24);
		videoPlayViewListData->wVideoFileIndex = (WORD)(msg->lp);
		videoPlayViewListData->bVideoPlayState = VIDEOPLAYSTATE_STOP;
		videoPlayViewListData->bTimeToHideToolbar = TIMETOHIDETOOLBAR;
		videoPlayViewListData->bToolbarShow = 1;

		if(!PlayVideo (videoPlayViewListData))
		{
			// 命令发送失败
			XM_free (videoPlayViewListData);
			XM_printf ("AP_VideoSendCommand AP_VIDEOCOMMAND_PLAY failed\n");
			// 失败返回到调用者窗口
			XM_PullWindow (0);
			return;
		}

		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(VideoPlayView), videoPlayViewListData);
	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复		
		XM_printf ("VideoPlayView Pull\n");
	}

	// 创建定时器，用于时间刷新
	// 创建0.5秒的定时器
	XM_SetTimer (XMTIMER_VIDEOPLAYVIEW, 500);

}

VOID VideoPlayViewOnLeave (XMMSG *msg)
{
	XM_printf ("VideoPlayViewOnLeave %d\n", msg->wp);
	if (msg->wp == 0)
	{
		// 窗口退出，彻底摧毁。
		// 获取私有数据句柄
		VIDEOPLAYVIEWDATA *videoPlayViewListData = (VIDEOPLAYVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoPlayView));
		// 释放所有分配的资源
		if(videoPlayViewListData)
		{
			// 释放私有数据句柄
			XM_free (videoPlayViewListData);
		}
		XM_printf ("VideoPlayView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("VideoPlayView Push\n");
	}
	XM_KillTimer (XMTIMER_VIDEOPLAYVIEW);

}



VOID VideoPlayViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
	APPROMRES *AppRes;
	char text[32];
	HANDLE hVideoItem;
	XMVIDEOITEM *pVideoItem;

	VIDEOPLAYVIEWDATA *videoMenuData = (VIDEOPLAYVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoPlayView));
	if(videoMenuData == NULL)
		return;

	XM_GetDesktopRect (&rc);
	XM_FillRect (XMHWND_HANDLE(VideoPlayView), rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_WINDOWTEXT));

	// 检查是否显示播放工具栏
	if(videoMenuData->bToolbarShow)
	{
		rect = rc;
		rect.top = APP_POS_VIDEOPLAY_TOOLBAR_Y;
		AppRes = AP_AppRes2RomRes (AP_ID_COMMON_VIDEOTOOLBARBACKGROUND);
		XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(VideoPlayView), &rect, XMGIF_DRAW_POS_LEFTTOP);

		// 格式化输出文件名
		// AP_VideoListFormatVideoFileName (videoMenuData->wVideoFileIndex, text, sizeof(text));
		hVideoItem = AP_VideoItemGetVideoItemHandle (videoMenuData->mode, videoMenuData->wVideoFileIndex);
		pVideoItem = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
		memset (text, 0, sizeof(text));
		AP_VideoItemFormatVideoFileName (pVideoItem, text, sizeof(text));
		AP_TextOutDataTimeString (XMHWND_HANDLE(VideoPlayView), 
					APP_POS_VIDEOPLAY_TIME_X, APP_POS_VIDEOPLAY_TIME_y, 
					text, 20);

		AppRes = AP_AppRes2RomRes (AP_ID_COMMON_VIDEO_PREV_BTN);
		rect.left = APP_POS_VIDEOPLAY_PREV_BTN_X;
		rect.right = rect.left;
		rect.top = APP_POS_VIDEOPLAY_BTN_Y;
		rect.bottom = rect.top;
		XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(VideoPlayView), &rect, XMGIF_DRAW_POS_CENTER);

		if(	videoMenuData->bVideoPlayState == VIDEOPLAYSTATE_PLAY)
			AppRes = AP_AppRes2RomRes (AP_ID_COMMON_VIDEO_PAUSE_BTN);
		else if(	videoMenuData->bVideoPlayState == VIDEOPLAYSTATE_STOP)
			AppRes = AP_AppRes2RomRes (AP_ID_COMMON_VIDEO_PLAY_BTN);
		else
			AppRes = AP_AppRes2RomRes (AP_ID_COMMON_VIDEO_PLAY_BTN);
		rect.left = APP_POS_VIDEOPLAY_PLAY_BTN_X;
		rect.right = rect.left;
		rect.top = APP_POS_VIDEOPLAY_BTN_Y;
		rect.bottom = rect.top;
		XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(VideoPlayView), &rect, XMGIF_DRAW_POS_CENTER);

		AppRes = AP_AppRes2RomRes (AP_ID_COMMON_VIDEO_NEXT_BTN);
		rect.left = APP_POS_VIDEOPLAY_NEXT_BTN_X;
		rect.right = rect.left;
		rect.top = APP_POS_VIDEOPLAY_BTN_Y;
		rect.bottom = rect.top;
		XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(VideoPlayView), &rect, XMGIF_DRAW_POS_CENTER);

	}
}



VOID VideoPlayViewOnKeyDown (XMMSG *msg)
{
	DWORD dwItem;
	VIDEOPLAYVIEWDATA *videoMenuData = (VIDEOPLAYVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoPlayView));
	if(videoMenuData == NULL)
		return;
	ShowToolbar (videoMenuData);

	XM_printf ("VideoPlayViewOnKeyDown %d\n", msg->wp);

	switch(msg->wp)
	{
		case VK_LEFT:
			// dwItem = AP_VideoListGetPrevVideoIndex (videoMenuData->wVideoFileIndex);
			dwItem = AP_VideoItemGetPrevVideoIndex (videoMenuData->mode, videoMenuData->wVideoFileIndex);
			if(dwItem == (DWORD)(-1))
				return;
			videoMenuData->wVideoFileIndex = dwItem;
			StopVideo (videoMenuData);
			PlayVideo (videoMenuData);
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_RIGHT:
			// dwItem = AP_VideoListGetNextVideoIndex (videoMenuData->wVideoFileIndex);
			dwItem = AP_VideoItemGetNextVideoIndex (videoMenuData->mode, videoMenuData->wVideoFileIndex);
			if(dwItem == (DWORD)(-1))
				return;
			videoMenuData->wVideoFileIndex = dwItem;
			StopVideo (videoMenuData);
			PlayVideo (videoMenuData);
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_POWER:
			if(	videoMenuData->bVideoPlayState == VIDEOPLAYSTATE_STOP
				||	videoMenuData->bVideoPlayState == VIDEOPLAYSTATE_PAUSE)
			{
				// 暂停或停止
				PlayVideo (videoMenuData);
			}
			else
			{
				// 播放状态, 恢复为停止状态
				AP_VideoSendCommand (AP_VIDEOCOMMAND_STOP, 0);
				videoMenuData->bVideoPlayState = VIDEOPLAYSTATE_STOP;
			}
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_MODE:		
			if(	videoMenuData->bVideoPlayState == VIDEOPLAYSTATE_STOP
				||	videoMenuData->bVideoPlayState == VIDEOPLAYSTATE_PAUSE)
			{
				// 退出
				XM_PullWindow (0);
			}
			else
			{
				StopVideo (videoMenuData);
				// 刷新
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			break;

		case VK_AP_SWITCH:
			// 暂时未考虑主副视图切换
			XM_printf ("VK_AP_SWITCH\n");

#ifdef PCVER
#else
			XMSYS_VideoSetCameraMode (-1);	
#endif
			break;

		default:
			break;
	}
}

VOID VideoPlayViewOnKeyUp (XMMSG *msg)
{
//	VIDEOPLAYVIEWDATA *videoMenuData = (VIDEOPLAYVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoPlayView));
}

VOID VideoPlayViewOnTimer (XMMSG *msg)
{
	VIDEOPLAYVIEWDATA *videoMenuData = (VIDEOPLAYVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoPlayView));
	if(videoMenuData == NULL)
		return;

	// 检测卡状态
	if(APSYS_CardChecking() == APP_SYSTEMCHECK_CARD_NOCARD)
	{
		// 卡拔出，直接跳出回到桌面(桌面负责卡插拔的所有处理), 此处强制桌面重新初始化
		StopVideo (videoMenuData);
		XM_JumpWindow (XMHWND_HANDLE(Desktop));
		return;
	}
	// 检查播放状态
	if(videoMenuData->bVideoPlayState == VIDEOPLAYSTATE_PLAY)
	{
		if(videoMenuData->bToolbarShow)
		{
			// 检查工具栏是否可视
			videoMenuData->bTimeToHideToolbar --;
			if(videoMenuData->bTimeToHideToolbar == 0)
			{
				videoMenuData->bToolbarShow = 0;

				// 刷新
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
		}
	}
	else if(videoMenuData->bToolbarShow == 0)
	{
		// 非播放状态下，应显示工具栏
		videoMenuData->bToolbarShow = 1;
		// 刷新
		XM_InvalidateWindow ();
		XM_UpdateWindow ();
	}	
}

VOID VideoPlayViewOnVideoStop (XMMSG *msg)
{
	VIDEOPLAYVIEWDATA *videoMenuData = (VIDEOPLAYVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoPlayView));
	if(videoMenuData == NULL)
		return;
	videoMenuData->bVideoPlayState = VIDEOPLAYSTATE_STOP;
	XM_printf ("VideoPlayViewOnVideoStop\n");
	// 刷新
	XM_InvalidateWindow ();
	XM_UpdateWindow ();
}

static VOID VideoPlayViewOnSystemEvent (XMMSG *msg)
{
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:			// SD卡拔出事件
			XM_BreakSystemEventDefaultProcess (msg);		// 不进行缺省处理
			// 将该消息重新插入到消息队列队首
			XM_InsertMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
			XM_JumpWindowEx (XMHWND_HANDLE(Desktop), HWND_CUSTOM_DEFAULT, XM_JUMP_POPDEFAULT);
		//	XM_JumpWindow (XMHWND_HANDLE(Desktop));		// 跳转到桌面
			//XM_PullWindow (0);					// 弹出当前的列表视窗
			break;
	}
}

// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (VideoPlayView)
	XM_ON_MESSAGE (XM_PAINT, VideoPlayViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, VideoPlayViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, VideoPlayViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, VideoPlayViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, VideoPlayViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, VideoPlayViewOnTimer)
	XM_ON_MESSAGE (XM_VIDEOSTOP, VideoPlayViewOnVideoStop)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, VideoPlayViewOnSystemEvent)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, VideoPlayView, 0, 0, 0, 255, HWND_VIEW)
