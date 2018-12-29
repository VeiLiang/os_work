//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_videoview.c
//	  视频回放窗口
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"
//#include "system_check.h"
#include "xm_h264_codec.h"

extern XMSYSTEMTIME rendervideotime;              //当前视频文件录制时间
extern DWORD dispyear,dispmonth,dispday,disphour, dispminute, dispsecond; //当前视频文件显示时间

#define	ALBUM_BUTTON_AUTO_HIDE_TIMEOUT	5000	// 按钮/视频标题自动隐藏时间 8000毫秒

// 私有命令定义
#define	VIDEOVIEW_COMMAND_PAUSE_PLAY		0
#define	VIDEOVIEW_COMMAND_LOCK_UNLOCK		1
#define	VIDEOVIEW_COMMAND_RETURN			2
#define	VIDEOVIEW_COMMAND_PAINT				3

#define	VIDEO_PLAY_STATE		1
#define	VIDEO_PAUSE_STATE		0


// “设置”窗口按钮控件定义
#define	VIDEOVIEWBTNCOUNT	3


// 回放模式下的按钮控件定义 (3个按钮)
static const XMBUTTONINFO videoViewBtn_0[VIDEOVIEWBTNCOUNT] = {
	{	
		// 播放/暂停
		VK_AP_MENU,		VIDEOVIEW_COMMAND_PAUSE_PLAY,	AP_ID_COMMON_MENU,	AP_ID_VIDEO_BUTTON_PAUSE
	},
	{	
		// "锁定"或"解锁"
		VK_AP_SWITCH,	VIDEOVIEW_COMMAND_LOCK_UNLOCK,	AP_ID_COMMON_MODE,	AP_ID_VIDEO_VIEW_BUTTON_LOCK
	},
	{	
		// 返回到"视频列表"视窗
		VK_AP_MODE,		VIDEOVIEW_COMMAND_RETURN,	AP_ID_COMMON_OK,	AP_ID_BUTTON_RETURN
	},
};



typedef struct tagVIDEOVIEWDATA {
	int					nCurItem;					// 当前选择的菜单项
	XMVIDEOITEM	*		video_item;

	BYTE					delete_one_confirm;
	BYTE					mode;							// 普通模式或者保护模式
	BYTE					playing;						// 1 正在播放 0 播放暂停
	BYTE					channel;						// 视频所在的通道(从0开始)

	BYTE					button_hide;				// 按钮的显示/隐藏     1 隐藏 0 显示
	BYTE					video_title_hide;			// 视频标题的显示/隐藏 1 隐藏 0 显示
	BYTE					video_damaged;				// 视频解码错误
	BYTE					playback_mode;				// 正常播放/快进播放/快退播放

	XMBUTTONCONTROL	btnControl;

} VIDEOVIEWDATA;

// 打开视频播放视图
VOID AP_OpenVideoBackView (BYTE mode, WORD wVideoFileIndex)
{
	XM_PushWindowEx (XMHWND_HANDLE(VideoBackView), (mode << 24) | wVideoFileIndex);
}

static VOID AP_ReturnToVideoListView (VIDEOVIEWDATA *videoViewData)
{
	// 将当前正在播放的视频项设置为"视频列表视窗"的当前选择项(焦点项)
	XM_PullWindow (0);

	if(videoViewData && videoViewData->nCurItem >= 0)
	{
		AP_VideoBackListViewSetFocusItem (videoViewData->nCurItem);
	}
	
	
}

static int get_video_info (unsigned int mode, unsigned int item, VIDEOVIEWDATA *videoViewData)
{
	HANDLE hVideoItem;
	XMVIDEOITEM *pVideoItem;

	hVideoItem = AP_VideoItemGetVideoItemHandleEx ((BYTE)mode, item,XM_VIDEO_CHANNEL_1);
	if(hVideoItem == NULL)
		return -1;
	pVideoItem = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
	if(pVideoItem == NULL)
		return -1;

	videoViewData->video_item = pVideoItem;
	
	videoViewData->nCurItem = item;
	videoViewData->mode = (BYTE)mode;
	videoViewData->channel = (BYTE)XM_VideoItemGetVideoChannel (hVideoItem);

	return 0;
}

static void set_video_damaged_state (VIDEOVIEWDATA *videoViewData)
{
	videoViewData->video_damaged = 1;
	videoViewData->video_title_hide = 0;	// 显示视频标题
		
	videoViewData->button_hide = 0;
	// 使能视窗的按钮显示
	AP_ButtonControlSetFlag (&videoViewData->btnControl, 0);
	XM_InvalidateWindow ();
	XM_SetTimer (XMTIMER_VIDEOVIEW, ALBUM_BUTTON_AUTO_HIDE_TIMEOUT);
}

// 根据视频项锁定/解锁状态, 设置第二个按钮的外观及行为
static void modify_lock_unlock_button (VIDEOVIEWDATA *videoViewData)
{
	XMBUTTONINFO buttonSetting;
	buttonSetting.wCommand = VIDEOVIEW_COMMAND_LOCK_UNLOCK;
	buttonSetting.wKey = VK_AP_SWITCH; 
	buttonSetting.dwLogoId = AP_ID_COMMON_MODE;
	if(videoViewData->video_item->video_lock)
		buttonSetting.dwTextId = AP_ID_VIDEO_VIEW_BUTTON_UNLOCK;
	else
		buttonSetting.dwTextId = AP_ID_VIDEO_VIEW_BUTTON_LOCK;	// 锁定
	AP_ButtonControlModify (&videoViewData->btnControl, 1, &buttonSetting);
}

#ifdef PCVER
void isp_set_avi_filename (const char *filename);
#endif

static int decode_video (VIDEOVIEWDATA *videoViewData)
{
	char filename[64];
	unsigned int start_point = 0;
	XMVIDEOITEM *pVideoItem = videoViewData->video_item;
	if(XM_VideoItemGetVideoFilePath (pVideoItem, videoViewData->channel, filename, sizeof(filename)) == 0)
	{
		return -1;
	}

	if( videoViewData->playback_mode == XM_VIDEO_PLAYBACK_MODE_BACKWARD )
	{
		start_point = AP_VideoItemGetVideoStreamSize (pVideoItem, videoViewData->channel);
		if(start_point == 0)		// 文件没有正常关闭
		{

		}
	}

#ifdef PCVER
	isp_set_avi_filename (pVideoItem->video_name);
#endif

	if(XMSYS_H264CodecPlayerStart (filename, videoViewData->playback_mode, start_point) < 0)
	{
		return -1;
	}
	else
	{
		XM_InvalidateWindow ();
	}
	return 0;
}

static VOID VideoBackViewOnEnter (XMMSG *msg)
{
	XMSYSTEMTIME filetime;
	char text[32];
	
	VIDEOVIEWDATA *videoViewData;
	XM_printf ("VideoViewOnEnter %d\n", msg->wp);

	// 关闭过程ANIMATION
	XM_DisableViewAnimate (XMHWND_HANDLE(VideoBackView));

	if(msg->wp == 0)
	{
		// 窗口未建立，第一次进入
		DWORD lp = msg->lp;

		// 分配私有数据句柄
		videoViewData = (VIDEOVIEWDATA *)XM_calloc (sizeof(VIDEOVIEWDATA));
		if(videoViewData == NULL)
		{
			XM_printf ("videoViewData XM_calloc failed\n");
			// 失败返回到调用者窗口
			XM_PullWindow (0);
			return;
		}
		
		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(VideoBackView), videoViewData);

		if(get_video_info (lp >> 24, lp & 0xFFFF, videoViewData) < 0)
		{
			XM_printf ("videoViewData get_video_info failed\n");
			XM_SetWindowPrivateData (XMHWND_HANDLE(VideoBackView), NULL);
			XM_free (videoViewData);
			// 失败返回到调用者窗口
			XM_PullWindow (0);
			return;
		}

		#if 1
        //获取文件创建时间
        if(AP_VideoItemFormatVideoFileName (videoViewData->video_item, text, sizeof(text)) == NULL)
			return;
		
		// 从视频的文件名称获取录像的创建日期与时间
		XM_GetDateTimeFromFileName ((char *)text, &filetime);

		rendervideotime.wYear = filetime.wYear;
		rendervideotime.bMonth = filetime.bMonth;
		rendervideotime.bDay = filetime.bDay;
		rendervideotime.bHour = filetime.bHour;
		rendervideotime.bMinute = filetime.bMinute;
		rendervideotime.bSecond = filetime.bSecond;
		#endif
		
		// 开始扫描所有符合条件的视频文件
		videoViewData->button_hide = 0;
		videoViewData->video_title_hide = 0;

		videoViewData->playing = VIDEO_PLAY_STATE;

		// 按钮控件初始化
		AP_ButtonControlInit (&videoViewData->btnControl, VIDEOVIEWBTNCOUNT, 
			XMHWND_HANDLE(VideoBackView), videoViewBtn_0);

		// 隐藏按钮区的顶部水平分割线及每个按钮之间的垂直分割线
		AP_ButtonControlSetFlag (&videoViewData->btnControl,
			XMBUTTON_FLAG_HIDE_HORZ_SPLIT|XMBUTTON_FLAG_HIDE_VERT_SPLIT);

		modify_lock_unlock_button (videoViewData);

		if(decode_video (videoViewData) >= 0)
		{
			videoViewData->playing = 1;
		}
		else
		{
			// 继续播放下一个
			//set_video_damaged_state (videoViewData);
			XM_KeyEventProc (VK_AP_VIDEOSTOP, (SHORT)(AP_VIDEOEXITCODE_FINISH|(XM_VIDEO_PLAYBACK_MODE_NORMAL << 8)) );
		}
	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复		
		videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackView));
		if(videoViewData == NULL)
			return;

		XM_printf ("VideoBackView Pull\n");
	}

	// 创建定时器，用于按钮及照片标题的隐藏
	// 创建x秒的定时器
	XM_SetTimer (XMTIMER_VIDEOVIEW, ALBUM_BUTTON_AUTO_HIDE_TIMEOUT);
}

static VOID VideoBackViewOnLeave (XMMSG *msg)
{
	XM_printf ("VideoViewOnLeave %d\n", msg->wp);
	// 删除定时器
	XM_KillTimer (XMTIMER_VIDEOVIEW);

	if (msg->wp == 0)
	{
		// 窗口退出，彻底摧毁。
		// 获取私有数据句柄
		VIDEOVIEWDATA *videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackView));
		XMSYS_H264CodecPlayerStop ();
		// 释放所有分配的资源
		if(videoViewData)
		{
			// 按钮控件退出过程
			AP_ButtonControlExit (&videoViewData->btnControl);
			// 释放私有数据句柄
			XM_free (videoViewData);
		}
		// 设置视窗的私有数据句柄为空
		XM_SetWindowPrivateData (XMHWND_HANDLE(VideoBackView), NULL);
             OS_Delay (100);
		
		// 关闭视频输出(OSD 0层关闭)
		XM_osd_framebuffer_release (0, XM_OSD_LAYER_0);
		XM_printf ("VideoBackView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("VideoBackView Push\n");
	}
}

static void ShowVideoTime (VIDEOVIEWDATA *videoViewData)
{
	XMSYSTEMTIME filetime;
	char text[32];
	XMRECT rc;
	XMSIZE size;
	XMCOORD x, y;

	XM_GetDesktopRect (&rc);
	if(AP_VideoItemFormatVideoFileName (videoViewData->video_item, text, sizeof(text)) == NULL)
		return;

	XM_printf(">>>>>>>>>>>>>videoViewData->video_item->video_name:%s\r\n", videoViewData->video_item->video_name);
	// 从视频的文件名称获取录像的创建日期与时间
	XM_GetDateTimeFromFileName ((char *)(videoViewData->video_item->video_name), &filetime);
	sprintf (text, "%04d/%02d/%02d %02d:%02d:%02d", filetime.wYear, 
																	filetime.bMonth,
																	filetime.bDay,
																	filetime.bHour,
																	filetime.bMinute,
																	filetime.bSecond);
	AP_TextGetStringSize (text, strlen(text), &size);

	XM_printf(">>>>>>>>>>>>>text:%s\r\n", text);

	x = (rc.right - rc.left + 1 - size.cx) / 2;
	y = 40;

	// 显示视频的录制开始时间, 视窗的左上角(10, 10)																	
	AP_TextOutDataTimeString (XMHWND_HANDLE(VideoBackView), x, y, text, strlen(text));
}

static void clear_video_layer (void)
{
	xm_osd_framebuffer_t yuv_framebuffer;
	unsigned int w, h;
	// 获取视频层的尺寸信息
	w = XM_lcdc_osd_get_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	h = XM_lcdc_osd_get_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	// 创建一个新的视频层的帧缓存句柄
	yuv_framebuffer = XM_osd_framebuffer_create (0,
						XM_OSD_LAYER_0,
						XM_OSD_LAYER_FORMAT_Y_UV420,
						w,
						h,
						NULL,
						0,
						0		// clear_framebuffer_context
						);
	if(yuv_framebuffer == NULL)
	{
		XM_printf ("AlbumView failed, XM_osd_framebuffer_create NG\n");
		return;
	}
	// 清除framebuffer
	XM_osd_framebuffer_clear (yuv_framebuffer, 0, 255, 255, 255);
	// 将framebuffer关闭, 将其更新为当前的视频缓冲, 即在OSD 0层显示
	XM_osd_framebuffer_close (yuv_framebuffer, 0);

}

static VOID VideoBackViewOnPaint (XMMSG *msg)
{
	XMRECT rc;
	unsigned int old_alpha;
	HANDLE hWnd;

	float scale_factor;		// 水平缩放因子

	// 获取与视窗关联的私有数据
	VIDEOVIEWDATA *videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackView));
	if(videoViewData == NULL)
		return;

	// 获取窗口的句柄
	hWnd = XMHWND_HANDLE(VideoBackView);

	// VideoView定义时的视窗Alpha因子为0 (参考文件的最底部定义)
	// 将背景填充为全透明背景(Alpha为0)
	XM_GetDesktopRect (&rc);
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);
	XM_FillRect (hWnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	// 设置视窗的Alpha因子为255
	old_alpha =  XM_GetWindowAlpha (hWnd);
	XM_SetWindowAlpha (hWnd, 255);

	// 处理视频标题的显示 
	// 视频文件损坏时需要显示视频标题提示用户
	if(!videoViewData->video_title_hide || videoViewData->video_damaged)
	{
		// 显示视频的标题
		ShowVideoTime (videoViewData);
	}

	if(videoViewData->video_item->video_lock)
	{
		// 显示锁定标记
		XMRECT rect = rc;
		rect.left = 24;
		rect.top = 24;
		AP_RomImageDrawByMenuID (AP_ID_COMMON_LOCK_32, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	}

    if(!(videoViewData->playing))
	{
		// 显示暂停
		XMRECT rectstop = rc;
		rectstop.left = 90;
		rectstop.top = 24;
		AP_RomImageDrawByMenuID (AP_ID_COMMON_PAUSE, hWnd, &rectstop, XMGIF_DRAW_POS_LEFTTOP);
	}
	
	// 视频解码错误
	if(videoViewData->video_damaged)
	{
		// 显示错误信息
		AP_RomImageDrawByMenuID (AP_ID_VIDEO_INFO_PHOTO_DAMAGE, hWnd, &rc, XMGIF_DRAW_POS_CENTER);
		clear_video_layer ();
	}

	// 处理按钮控件的显示。
	// 若存在按钮控件，必须调用AP_ButtonControlMessageHandler执行按钮控件显示
	AP_ButtonControlMessageHandler (&videoViewData->btnControl, msg);

	XM_SetWindowAlpha (hWnd, (unsigned char)old_alpha);

}


static VOID VideoBackViewOnKeyDown (XMMSG *msg)
{
	VIDEOVIEWDATA *videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackView));
	if(videoViewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// 菜单键
		case VK_AP_MODE:		// 切换到“行车记录”状态
		case VK_AP_SWITCH:	// 画面切换键
		case VK_AP_POWER:		// 电源键
			XM_SetTimer (XMTIMER_VIDEOVIEW, ALBUM_BUTTON_AUTO_HIDE_TIMEOUT);
			if(videoViewData->button_hide)
			{
				videoViewData->button_hide = 0;
				videoViewData->video_title_hide = 0;
				
				AP_ButtonControlSetFlag (&videoViewData->btnControl,
					XMBUTTON_FLAG_HIDE_HORZ_SPLIT|XMBUTTON_FLAG_HIDE_VERT_SPLIT);
				XM_InvalidateWindow ();
				return;
			}
			// 在"录像浏览"窗口中，MENU、Power、Switch及MODE键被定义为按钮操作。
			// 此处将这四个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&videoViewData->btnControl, msg);
			break;

		case VK_AP_UP:		
			// 快进
			if(videoViewData->video_damaged)
				return;
			XMSYS_H264CodecPlayerBackward ();
			break;

		case VK_AP_DOWN:	// 
			// 快退
			if(videoViewData->video_damaged)
				return;
			XMSYS_H264CodecPlayerForward ();
			break;
	}

}



static VOID VideoBackViewOnKeyUp (XMMSG *msg)
{
	VIDEOVIEWDATA *videoViewData;
	videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackView));
	if(videoViewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// 菜单键
		case VK_AP_MODE:		// 切换到“行车记录”状态
		case VK_AP_POWER:
		case VK_AP_SWITCH:
			// 在"时间设置"窗口中，MENU及MODE键被定义为按钮操作。
			// 此处将这两个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&videoViewData->btnControl, msg);
			break;

#ifdef _WINDOWS
		case VK_SPACE:
			if(videoViewData->playing)
			{
				XMSYS_H264CodecPlayerFrame ();
			}
			break;
#endif

		default:
			AP_ButtonControlMessageHandler (&videoViewData->btnControl, msg);
			break;
	}
}


static VOID VideoBackViewOnCommand (XMMSG *msg)
{
	VIDEOVIEWDATA *videoViewData;
	videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackView));
	if(videoViewData == NULL)
		return;

	XM_printf ("VideoViewOnCommand msg->wp=%d\n", msg->wp);

	switch (msg->wp)
	{
		case VIDEOVIEW_COMMAND_RETURN:
			
			AP_ReturnToVideoListView (videoViewData);
			break;

		case VIDEOVIEW_COMMAND_LOCK_UNLOCK:
		{
			if(videoViewData->video_item->video_lock)
			{
				AP_VideoItemUnlockVideoFile (videoViewData->video_item);
			}
			else
			{
				AP_VideoItemLockVideoFile (videoViewData->video_item);
			}
			modify_lock_unlock_button (videoViewData);
			// 重新绘制
			XM_InvalidateWindow ();
			break;
		}
		
		case VIDEOVIEW_COMMAND_PAUSE_PLAY:
		{
			XMBUTTONINFO buttonSetting;
			// 暂停/继续
			// 根据当前播放的状态,修改第一个按钮的显示及行为
			if(videoViewData->video_damaged)
				return;

			rendervideotime.wYear = dispyear;
			rendervideotime.bMonth = dispmonth;
			rendervideotime.bDay = dispday;
			rendervideotime.bHour = disphour;
			rendervideotime.bMinute = dispminute;
			rendervideotime.bSecond = dispsecond;
			
			if(videoViewData->playing)
			{
				if(XMSYS_H264CodecPlayerPause () < 0)
					return;
			}
			else
			{
				if(decode_video (videoViewData) < 0)
				{
					// set_video_damaged_state (videoViewData);
					XM_KeyEventProc (VK_AP_VIDEOSTOP, (SHORT)(AP_VIDEOEXITCODE_FINISH|(XM_VIDEO_PLAYBACK_MODE_NORMAL << 8)) );
					return;
				}
			}

			videoViewData->playing = (BYTE)(!videoViewData->playing);
			// 修改第一个按钮的信息
			buttonSetting.wCommand = VIDEOVIEW_COMMAND_PAUSE_PLAY;
			buttonSetting.wKey = VK_AP_MENU;
			buttonSetting.dwLogoId = AP_ID_COMMON_MENU;
			if(videoViewData->playing)
				buttonSetting.dwTextId = AP_ID_VIDEO_BUTTON_PAUSE;
			else
				buttonSetting.dwTextId = AP_ID_VIDEO_BUTTON_PLAY;
			AP_ButtonControlModify (&videoViewData->btnControl, 0, &buttonSetting);
			// 重新绘制
			XM_InvalidateWindow ();
			break;
		}

		default:
			break;
	}
}

static void continue_playback (VIDEOVIEWDATA *videoViewData, BYTE playback_mode)
{
	XMSYSTEMTIME filetime;
	char text[32];
	
	DWORD next_item;
	
	// 播放下一个视频
	if(playback_mode == XM_VIDEO_PLAYBACK_MODE_NORMAL || playback_mode == XM_VIDEO_PLAYBACK_MODE_FORWARD)
	{
		// 寻找与之时间相邻的下一个视频播放
		next_item = AP_VideoItemGetNextVideoIndex (videoViewData->mode, videoViewData->nCurItem);
		if(next_item == (DWORD)(-1) || get_video_info (videoViewData->mode, next_item, videoViewData) < 0)
		{
			// 下一个视频不存在
			AP_ReturnToVideoListView (videoViewData);
		}
		else
		{
			// 下一个视频已找到
			videoViewData->playback_mode = playback_mode;
			
			#if 1
	        //获取文件创建时间
	        if(AP_VideoItemFormatVideoFileName (videoViewData->video_item, text, sizeof(text)) == NULL)
				return;
			
			// 从视频的文件名称获取录像的创建日期与时间
			XM_GetDateTimeFromFileName ((char *)text, &filetime);

			rendervideotime.wYear = filetime.wYear;
			rendervideotime.bMonth = filetime.bMonth;
			rendervideotime.bDay = filetime.bDay;
			rendervideotime.bHour = filetime.bHour;
			rendervideotime.bMinute = filetime.bMinute;
			rendervideotime.bSecond = filetime.bSecond;
			#endif

			// 解码
			if(decode_video (videoViewData) < 0)
			{
				// 解码失败
				// set_video_damaged_state (videoViewData);
				XM_KeyEventProc (VK_AP_VIDEOSTOP, (SHORT)(AP_VIDEOEXITCODE_FINISH|(playback_mode << 8)) );
			}
			// eason 20170712 播放下一个视频时更新控件状态
			modify_lock_unlock_button (videoViewData);
			// 重新绘制
			XM_InvalidateWindow ();
		}
	}
	else if(playback_mode == XM_VIDEO_PLAYBACK_MODE_BACKWARD)
	{
		// 寻找与之时间相邻的前一个视频播放
		next_item = AP_VideoItemGetPrevVideoIndex (videoViewData->mode, videoViewData->nCurItem);
		if(next_item == (DWORD)(-1) || get_video_info (videoViewData->mode, next_item, videoViewData) < 0)
		{
			// 下一个视频不存在
			AP_ReturnToVideoListView (videoViewData);
		}
		else
		{
			// 下一个视频已找到
			videoViewData->playback_mode = playback_mode;
			// 解码
			if(decode_video (videoViewData) < 0)
			{
				// 解码失败
				// set_video_damaged_state (videoViewData);
				XM_KeyEventProc (VK_AP_VIDEOSTOP, (SHORT)(AP_VIDEOEXITCODE_FINISH|(playback_mode << 8)) );
			}

		}

	}
}

// 用于无操作后将主按键隐藏, 缺省为3秒
static VOID VideoBackViewOnTimer (XMMSG *msg)
{
	VIDEOVIEWDATA *videoViewData;
	videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackView));
	if(videoViewData == NULL)
		return;
	if(videoViewData->video_damaged)
	{
		XM_KillTimer (XMTIMER_VIDEOVIEW);

		continue_playback (videoViewData, videoViewData->playback_mode);
		return;
	}

	if(!videoViewData->button_hide)
	{
		XM_KillTimer (XMTIMER_VIDEOVIEW);
		videoViewData->button_hide = 1;
		videoViewData->video_title_hide = 1;

		AP_ButtonControlSetFlag (&videoViewData->btnControl,
					XMBUTTON_FLAG_HIDE);
		XM_InvalidateWindow ();
	}
	else if(!videoViewData->video_title_hide)
	{
		XM_KillTimer (XMTIMER_VIDEOVIEW);
		videoViewData->video_title_hide = 1;		// 隐藏照片的标题
		XM_InvalidateWindow ();
	}
}

static VOID VideoBackViewOnSystemEvent (XMMSG *msg)
{
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:			// SD卡拔出事件
			XM_BreakSystemEventDefaultProcess (msg);		// 不进行缺省处理
			// 将该消息重新插入到消息队列队首
			XM_InsertMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
			XM_JumpWindowEx (XMHWND_HANDLE(Desktop), HWND_CUSTOM_DEFAULT, XM_JUMP_POPDEFAULT);
			//XM_JumpWindow (XMHWND_HANDLE(Desktop));		// 跳转到桌面
			//XM_PullWindow (0);					// 弹出当前的列表视窗
			break;
	}
}



static VOID VideoBackViewOnVideoStop (XMMSG *msg)
{
	VIDEOVIEWDATA *videoViewData;
	DWORD next_item;
	BYTE stop_code = LOBYTE(msg->wp);
	BYTE ext_info = HIBYTE(msg->wp);
	videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackView));
	if(videoViewData == NULL)
		return;
	//if(stop_code  == AP_VIDEOEXITCODE_FINISH)
	{
		continue_playback (videoViewData, ext_info);
	}
	//else
	{
		// stream error
		//set_video_damaged_state (videoViewData);
	}
}

VOID VideoBackViewOnTouchUp (XMMSG *msg)
{
	VIDEOVIEWDATA *VideoViewData;
	VideoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackView));
	if(VideoViewData == NULL)
		return;
     if(!VideoViewData->button_hide)
     {
	  if(AP_ButtonControlMessageHandler (&VideoViewData->btnControl, msg))
		return;
     	}

	
       
}
VOID VideoBackViewOnTouchDown (XMMSG *msg)
{
	int index;
	VIDEOVIEWDATA *VideoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackView));
	if(VideoViewData == NULL)
		return;
	
	if(VideoViewData->button_hide)
	{
        XM_SetTimer (XMTIMER_VIDEOVIEW, ALBUM_BUTTON_AUTO_HIDE_TIMEOUT);

		VideoViewData->button_hide = 0;
		VideoViewData->video_title_hide = 0;
		
		AP_ButtonControlSetFlag (&VideoViewData->btnControl,
			XMBUTTON_FLAG_HIDE_HORZ_SPLIT|XMBUTTON_FLAG_HIDE_VERT_SPLIT);
		XM_InvalidateWindow ();
		return;
	}
    else 
	{
		if(AP_VideoplayingButtonControlMessageHandler (&VideoViewData->btnControl, msg))
			return;
		
		XM_KillTimer (XMTIMER_VIDEOVIEW);
		VideoViewData->button_hide = 1;
		VideoViewData->video_title_hide = 1;

		AP_ButtonControlSetFlag (&VideoViewData->btnControl,
				XMBUTTON_FLAG_HIDE);
		XM_InvalidateWindow ();
	}
	
	//index = AppLocateItem (XMHWND_HANDLE(VideoView), VideoViewData->nItemCount, APP_POS_ITEM5_LINEHEIGHT, VideoViewData->nTopItem, LOWORD(msg->lp), HIWORD(msg->lp));
	//if(index < 0)
		//return;
	//settingViewData->nTouchItem = index;
	/*
	if(VideoViewData->nCurItem != index)
	{
		VideoViewData->nCurItem = index;
	
		XM_InvalidateWindow ();
		XM_UpdateWindow ();
	}
	*/
}


// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (VideoBackView)
	XM_ON_MESSAGE (XM_PAINT, VideoBackViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, VideoBackViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, VideoBackViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, VideoBackViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, VideoBackViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, VideoBackViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, VideoBackViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, VideoBackViewOnTouchUp)
	XM_ON_MESSAGE (XM_TIMER, VideoBackViewOnTimer)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, VideoBackViewOnSystemEvent)
	XM_ON_MESSAGE (XM_VIDEOSTOP, VideoBackViewOnVideoStop)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, VideoBackView, 0, 0, 0, 0, HWND_VIEW)
