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
extern unsigned int get_videodecoder_times(void);
extern void clear_videodecoder_times(void);

unsigned char replaytime = 0;
#define	VIDEO_PLAY_STATE					1
#define	VIDEO_PAUSE_STATE					0


typedef struct tagVIDEOVIEWDATA {
	int					nCurItem;					// 当前选择的菜单项
	XMVIDEOITEM	*		video_item;

	//BYTE              delete_one_confirm;
	BYTE				mode;							// 普通模式或者保护模式
	BYTE				playing;						// 1 正在播放 0 播放暂停
	BYTE				channel;						// 视频所在的通道(从0开始)

	BYTE				Prev_Or_Next_video;			//0:上一个视频,1:下一个视频
	BYTE				button_hide;				// 按钮的显示/隐藏     1 隐藏 0 显示
	BYTE				video_title_hide;			// 视频标题的显示/隐藏 1 隐藏 0 显示
	BYTE				video_damaged;				// 视频解码错误
	BYTE				playback_mode;				// 正常播放/快进播放/快退播放

	XMBUTTONCONTROL		btnControl;

} VIDEOVIEWDATA;


static void rePlay_DrawVSlider(HANDLE hWnd,XMRECT *rc,unsigned int upBgcolor,unsigned int leftBgcolor,unsigned int thum,int curValue, int totaltime)
{
	unsigned int slider_val;
	XMRECT slider_rc;

	if(curValue==totaltime)
	{
		slider_val = 700;
	}
	else
	{
		slider_val = 700*curValue/totaltime;
	}
	slider_rc.left = rc->left;
	slider_rc.top = rc->top;
	slider_rc.right = rc->left+700;
	slider_rc.bottom = rc->top + 4;

	//XM_printf(">>>>>slider_rc.left:%d, slider_val:%d, slider_rc.left+slider_val:%d\r\n", slider_rc.left, slider_val, slider_rc.left+slider_val);

	
	XM_FillRect(hWnd, slider_rc.left, slider_rc.top, slider_rc.right, slider_rc.bottom, upBgcolor);
	XM_FillRect(hWnd, slider_rc.left, slider_rc.top, slider_rc.left+slider_val, slider_rc.bottom, leftBgcolor);

	#if 0
	XMRECT rect;
	rect.left = slider_rc.left - 5;
	rect.top = rc->bottom - val_height - 10;
	rect.right = slider_rc.right + 5;
	rect.bottom = rect.top + 20;
	
	AP_RomImageDrawByMenuID(thum,hWnd,&rect,XMGIF_DRAW_POS_CENTER);
	#endif
}


// 打开视频播放视图
VOID AP_OpenVideoView(BYTE mode, WORD wVideoFileIndex)
{
	XM_JumpWindowEx(XMHWND_HANDLE(VideoView), (mode << 24) | wVideoFileIndex, XM_JUMP_POPDESKTOP);
}


static VOID AP_ReturnToVideoListView (VIDEOVIEWDATA *videoViewData)
{
	// 将当前正在播放的视频项设置为"视频列表视窗"的当前选择项(焦点项)
	XM_PullWindow (0);

	if(videoViewData && videoViewData->nCurItem >= 0)
	{
		AP_VideoListViewSetFocusItem (videoViewData->nCurItem);
	}
}

static int get_video_info(unsigned int mode, unsigned int item, VIDEOVIEWDATA *videoViewData)
{
	HANDLE hVideoItem;
	XMVIDEOITEM *pVideoItem;
	unsigned int replay_ch;
	if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
	{
		replay_ch = XM_VIDEO_CHANNEL_0;
	}
	else
	{
		replay_ch = XM_VIDEO_CHANNEL_1;
	}	
	XM_printf(">>>>get_video_info,item:%d\r\n", item);
	//hVideoItem = AP_VideoItemGetVideoItemHandleEx((BYTE)mode, item, replay_ch);
	hVideoItem = XM_VideoItemGetVideoItemHandleEx(0,replay_ch,XM_FILE_TYPE_VIDEO,XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL,item,1);
	
	if(hVideoItem == NULL)
		return -1;
	
	pVideoItem = AP_VideoItemGetVideoItemFromHandle(hVideoItem);
	if(pVideoItem == NULL)
		return -1;

	videoViewData->video_item = pVideoItem;
	
	videoViewData->nCurItem = item;
	videoViewData->mode = (BYTE)mode;
	videoViewData->channel = (BYTE)XM_VideoItemGetVideoChannel(hVideoItem);

	return 0;
}

#ifdef PCVER
void isp_set_avi_filename (const char *filename);
#endif

static int decode_video(VIDEOVIEWDATA *videoViewData)//解码播放时候,获取录像文件时长
{
	char filename[64];
	unsigned int start_point = 0;
	XMVIDEOITEM *pVideoItem = videoViewData->video_item;

	XM_printf(">>>>>>decode_video....................\r\n");
	XM_printf(">>>>>>videoViewData->channel:%d....................\r\n", videoViewData->channel);
	XM_printf(">>>>>>pVideoItem->video_channel:%d....................\r\n", pVideoItem->video_channel);
	if(XM_VideoItemGetVideoFilePath(pVideoItem, videoViewData->channel, filename, sizeof(filename)) == 0)
	{
		return -1;
	}
	
	XM_printf(">>>>>>videoViewData->playback_mode:%d\r\n", videoViewData->playback_mode);
	if( videoViewData->playback_mode == XM_VIDEO_PLAYBACK_MODE_BACKWARD )
	{
		start_point = AP_VideoItemGetVideoStreamSize(pVideoItem, videoViewData->channel);
		if(start_point == 0)		// 文件没有正常关闭
		{

		}
	}
	
	AP_VideoItemGetVideoStreamSize(pVideoItem, videoViewData->channel); //0通道

#ifdef PCVER
	isp_set_avi_filename (pVideoItem->video_name);
#endif

	XM_printf(">>>>>>>>>>>>>>>>>filename:%sf\r\n", filename);
	if(XMSYS_H264CodecPlayerStart(filename, videoViewData->playback_mode, start_point) < 0)
	{
		return -1;
	}
	else
	{
		XM_InvalidateWindow ();
	}
	return 0;
}


static VOID VideoViewOnEnter (XMMSG *msg)
{
	XMSYSTEMTIME filetime;
	char text[32];
	
	VIDEOVIEWDATA *videoViewData;
	XM_printf(">>>>>>>>>>>>>>>VideoViewOnEnter, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	// 关闭过程ANIMATION
	XM_DisableViewAnimate (XMHWND_HANDLE(VideoView));

	if(msg->wp == 0)
	{
		//窗口未建立，第一次进入
		DWORD lp = msg->lp;

		// 分配私有数据句柄
		videoViewData = (VIDEOVIEWDATA *)XM_calloc(sizeof(VIDEOVIEWDATA));
		if(videoViewData == NULL)
		{
			XM_printf ("videoViewData XM_calloc failed\n");
			// 失败返回到调用者窗口
			XM_PullWindow (0);
			return;
		}
		
		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData(XMHWND_HANDLE(VideoView), videoViewData);

		if(get_video_info(lp >> 24, lp & 0xFFFF, videoViewData) < 0)
		{
			XM_printf ("videoViewData get_video_info failed\n");
			XM_SetWindowPrivateData (XMHWND_HANDLE(VideoView), NULL);
			XM_free (videoViewData);
			// 失败返回到调用者窗口
			XM_PullWindow (0);
			return;
		}

		#if 1
        //获取文件创建时间
        if(AP_VideoItemFormatVideoFileName(videoViewData->video_item, text, sizeof(text)) == NULL)
			return;
		
		// 从视频的文件名称获取录像的创建日期与时间
		XM_GetDateTimeFromFileName((char *)text, &filetime);

		rendervideotime.wYear = filetime.wYear;
		rendervideotime.bMonth = filetime.bMonth;
		rendervideotime.bDay = filetime.bDay;
		rendervideotime.bHour = filetime.bHour;
		rendervideotime.bMinute = filetime.bMinute;
		rendervideotime.bSecond = filetime.bSecond;
		#endif
		
		unsigned int replay_ch;
		if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
		{
			replay_ch = XM_VIDEO_CHANNEL_0;
		}
		else
		{
			replay_ch = XM_VIDEO_CHANNEL_1;
		}	

		videoViewData->channel = replay_ch;
		// 开始扫描所有符合条件的视频文件
		videoViewData->button_hide = 0;
		videoViewData->video_title_hide = 0;
		videoViewData->playing = VIDEO_PLAY_STATE;

		if(decode_video(videoViewData) >= 0)
		{
			videoViewData->playing = VIDEO_PLAY_STATE;
		}
		else
		{// 继续播放下一个
			XM_KeyEventProc(VK_AP_VIDEOSTOP, (SHORT)(AP_VIDEOEXITCODE_FINISH|(XM_VIDEO_PLAYBACK_MODE_NORMAL << 8)) );
		}
	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复		
		videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData(XMHWND_HANDLE(VideoView));
		if(videoViewData == NULL)
			return;

		XM_printf ("VideoView Pull\n");
	}

	// 创建x秒的定时器
	XM_SetTimer(XMTIMER_VIDEOVIEW, 450);
}


static VOID VideoViewOnLeave (XMMSG *msg)
{
	XM_printf(">>>>>>>>>>>>>>>VideoViewOnLeave, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
	
	XM_KillTimer (XMTIMER_VIDEOVIEW);//删除定时器
	if (msg->wp == 0)
	{
		XMSYS_H264CodecRecorderStart();
		// 窗口退出，彻底摧毁。
		// 获取私有数据句柄
		VIDEOVIEWDATA *videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData(XMHWND_HANDLE(VideoView));
		// 释放所有分配的资源
		if(videoViewData)
		{
			// 释放私有数据句柄
			XM_free(videoViewData);
		}
		// 设置视窗的私有数据句柄为空
		XM_SetWindowPrivateData (XMHWND_HANDLE(VideoView), NULL);
        OS_Delay(100);
		
		// 关闭视频输出(OSD 0层关闭)
		XM_osd_framebuffer_release (0, XM_OSD_LAYER_0);
		
		XM_printf ("VideoView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("VideoView Push\n");
	}
}


static void ShowVideoTime(VIDEOVIEWDATA *videoViewData)
{
	XMSYSTEMTIME filetime;
	char text[32];
	XMRECT rc;
	XMSIZE size;
	XMCOORD x, y;

	if(AP_VideoItemFormatVideoFileName(videoViewData->video_item, text, sizeof(text)) == NULL)
		return;
	
	// 从视频的文件名称获取录像的创建日期与时间
	XM_GetDateTimeFromFileName ((char *)text, &filetime);
	sprintf(text, "%04d/%02d/%02d %02d:%02d:%02d",  filetime.wYear, 
													filetime.bMonth,
													filetime.bDay,
													filetime.bHour,
													filetime.bMinute,
													filetime.bSecond);
	
	AP_TextGetStringSize(text, strlen(text), &size);

	XM_GetDesktopRect(&rc);
	x = (rc.right - rc.left + 1 - size.cx) / 2;
	y = 20;

	// 显示视频的录制开始时间, 视窗的左上角(10, 10)																	
	AP_TextOutDataTimeString(XMHWND_HANDLE(VideoView), x, y, text, strlen(text));
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


static VOID VideoViewOnPaint(XMMSG *msg)
{
	XMRECT rc, rect;
	unsigned int old_alpha;
	HANDLE hWnd;
	int offset;
	
	float scale_factor;	//水平缩放因子

	//XM_printf(">>>>>>>>>>>>>>>VideoViewOnPaint, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	// 获取与视窗关联的私有数据
	VIDEOVIEWDATA *videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoView));
	if(videoViewData == NULL)
		return;

	// 获取窗口的句柄
	hWnd = XMHWND_HANDLE(VideoView);

	// VideoView定义时的视窗Alpha因子为0 (参考文件的最底部定义)
	// 将背景填充为全透明背景(Alpha为0)
	XM_GetDesktopRect(&rc);
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);
	XM_FillRect(hWnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	// 设置视窗的Alpha因子为255
	old_alpha =  XM_GetWindowAlpha (hWnd);
	XM_SetWindowAlpha(hWnd, 255);

	rect.left = 0;rect.top = 600-42;rect.right = 1024;rect.bottom = 42;
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUTITLEBARBACKGROUND, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//底部背景色

	// 处理视频标题的显示 
	// 视频文件损坏时需要显示视频标题提示用户
	if(!videoViewData->video_title_hide || videoViewData->video_damaged)
	{
		ShowVideoTime(videoViewData);// 显示视频的标题
	}

	if(videoViewData->video_item->video_lock)
	{
		XMRECT rect = rc;
		rect.left = 24;
		rect.top = 24;
		AP_RomImageDrawByMenuID (AP_ID_COMMON_LOCK_32, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//显示锁定标记
	}

	//XM_printf(">>>>>>>>>>>>>>>>>>>>>>videoViewData->playing:%d\r\n", videoViewData->playing);
    if(!(videoViewData->playing))
	{//暂停播放
		rect.left = 10;rect.top = 600-42+2;rect.right = 1024;rect.bottom = 42;
		AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_PLAY_ICON, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//显示播放
	}
	else
	{//正在播放
		rect.left = 10;rect.top = 600-42+2;rect.right = 1024;rect.bottom = 42;
		AP_RomImageDrawByMenuID(AP_ID_COMMON_PAUSE, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//显示暂停
	}
	
	//视频解码错误
	if(videoViewData->video_damaged)
	{
		AP_RomImageDrawByMenuID (AP_ID_VIDEO_INFO_PHOTO_DAMAGE, hWnd, &rc, XMGIF_DRAW_POS_CENTER);//显示错误信息
		clear_video_layer ();
	}
	
	unsigned int replay_ch;
	if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
	{
		replay_ch = XM_VIDEO_CHANNEL_0;
	}
	else
	{
		replay_ch = XM_VIDEO_CHANNEL_1;
	}	

	videoViewData->channel = replay_ch;
	
	//获取视频文件时长
	{
		DWORD dwVideoTime, runtime, totaltime;
		DWORD h, m, s, run_h, run_m, run_s;
		char text[64];
		XMCOORD x,y;

		runtime = get_videodecoder_times();
		if(videoViewData->video_item->stream_length_valid)
		{
			dwVideoTime = videoViewData->video_item->stream_length;
		}
		totaltime = dwVideoTime/1000;//秒
		//dwVideoTime = AP_VideoItemGetVideoStreamSize(videoViewData->video_item, 0);		// 0通道
		//h = dwVideoTime / (1000 * 3600);
		//m = ((dwVideoTime / 1000) / 60) % 60;
		//s = (dwVideoTime / 1000) % 60;
		h = totaltime / 3600;
		m = totaltime / 60;
		s = totaltime % 60;
		
		run_h = runtime / 3600;
		run_m = runtime / 60;
		run_s = runtime % 60;

		sprintf(text, "%02d:%02d:%02d%s%02d:%02d:%02d", run_h, run_m, run_s, "/", h, m, s);

		x = 1024-(12*15)-(4*11)-14-30; y=600-42+3;//数字15宽度,冒号11宽度,斜杠14宽度
		AP_TextOutDataTimeString(hWnd, x, y, text, strlen(text));

		//XM_printf(">>>>>video file times:%s\r\n", text);

		rect.left = 50;rect.top = 600-42+19;rect.right = 1024;rect.bottom = 42;
		rePlay_DrawVSlider(hWnd, &rect, 0xffffffff, XM_RGB(255, 0, 0), AP_ID_SLIDER_THUMB_N, runtime, totaltime);
	}

	XM_SetWindowAlpha(hWnd, (unsigned char)old_alpha);
}

extern UCHAR Play_Next_Video_keyflag;
static VOID VideoViewOnKeyDown(XMMSG *msg)
{
	XM_printf(">>>>>>>>>>>>>>>VideoViewOnKeyDown, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	VIDEOVIEWDATA *videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoView));
	if(videoViewData == NULL)
		return;

	switch(msg->wp)
	{
		//case VK_AP_FONT_BACK_SWITCH://AV键

		//	break;
		
		case VK_AP_SWITCH://OK键	
			XM_printf(">>>>>>>>>>>>VK_AP_SWITCH...........\r\n");
			// 暂停/继续
			// 根据当前播放的状态,修改第一个按钮的显示及行为
			if(videoViewData->video_damaged)
			{
				return;
	        }
			if(videoViewData->playing==VIDEO_PLAY_STATE)
			{
				if(XMSYS_H264CodecPlayerPause () < 0)
				{
					return;
				}
				rendervideotime.wYear = dispyear;
				rendervideotime.bMonth = dispmonth;
				rendervideotime.bDay = dispday;
				rendervideotime.bHour = disphour;
				rendervideotime.bMinute = dispminute;
				rendervideotime.bSecond = dispsecond;
			}
			else
			{
				if(decode_video(videoViewData) < 0)
				{
					XM_KeyEventProc (VK_AP_VIDEOSTOP, (SHORT)(AP_VIDEO_PRE_NEXT|(XM_VIDEO_PLAYBACK_MODE_NORMAL << 8)) );
					return;
				}
			}

			videoViewData->playing = (BYTE)(!videoViewData->playing);

			// 重新绘制
			XM_InvalidateWindow();
			XM_UpdateWindow();
			break;

		case VK_AP_UP://+键	
			XM_printf(">>>>>>>>>>>>VK_AP_UP...........\r\n");
			
			if(videoViewData->video_damaged)
			{
				return;
			}
			//XMSYS_H264CodecPlayerBackward();
			XM_KeyEventProc(VK_AP_VIDEOSTOP, (SHORT)(AP_VIDEO_PRE_NEXT|(XM_VIDEO_PLAYBACK_MODE_NORMAL << 8)) );
			videoViewData->Prev_Or_Next_video = 1;//下一个视频标志		
			break;

		case VK_AP_DOWN://-键	
			XM_printf(">>>>>>>>>>>>VK_AP_DOWN...........\r\n");
			
			if(videoViewData->video_damaged)
			{
				return;
			}
			//XMSYS_H264CodecPlayerForward();
			XM_KeyEventProc(VK_AP_VIDEOSTOP, (SHORT)(AP_VIDEO_PRE_NEXT|(XM_VIDEO_PLAYBACK_MODE_NORMAL << 8)) );
			videoViewData->Prev_Or_Next_video = 0;//上一个视频标志
			break;
			
		case VK_AP_FONT_BACK_SWITCH:
		case VK_AP_MENU://MENU键,退出播放页面
			XM_printf(">>>>>>>>>>>>VK_AP_MENU...........\r\n");

			if(videoViewData->video_damaged)
			{
				return;
			}
		    XMSYS_H264CodecPlayerStop();
            //XM_PullWindow(0);
            XM_JumpWindowEx(XMHWND_HANDLE(VideoListView), 0, 0);
			break;

		default:
			break;
	}
}


static VOID VideoViewOnKeyUp (XMMSG *msg)
{
	XM_printf(">>>>>>>>>>>>>>>VideoViewOnKeyUp, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	VIDEOVIEWDATA *videoViewData;
	videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoView));
	if(videoViewData == NULL)
		return;
	
}


static void continue_playback(VIDEOVIEWDATA *videoViewData, BYTE playback_mode)
{
	XMSYSTEMTIME filetime;
	char text[32];
	DWORD next_item;

	XM_printf(">>>>>>>continue_playback, playback_mode:%d\r\n", playback_mode);
	XM_printf(">>>>>>>videoViewData->Prev_Or_Next_video:%d\r\n", videoViewData->Prev_Or_Next_video);
	
	// 播放下一个视频
	if(playback_mode == XM_VIDEO_PLAYBACK_MODE_NORMAL)//正常播放模式
	{
		// 寻找与之时间相邻的下一个视频播放
		if(videoViewData->Prev_Or_Next_video == 1)
			next_item = AP_VideoItemGetNextVideoIndex (videoViewData->mode, videoViewData->nCurItem);
		else
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
			
			#if 1
	        //获取文件创建时间
	        if(AP_VideoItemFormatVideoFileName(videoViewData->video_item, text, sizeof(text)) == NULL)
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
			if(decode_video(videoViewData) < 0)
			{//解码失败,播放下一个视频
				XM_KeyEventProc(VK_AP_VIDEOSTOP, (SHORT)(AP_VIDEOEXITCODE_FINISH|(playback_mode << 8)) );
			}

			// 重新绘制
			XM_InvalidateWindow ();
		}
	}
}

// 用于无操作后将主按键隐藏, 缺省为3秒
static VOID VideoViewOnTimer (XMMSG *msg)
{
	VIDEOVIEWDATA *videoViewData;
	videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoView));
	if(videoViewData == NULL)
		return;

	if(videoViewData->video_damaged)
	{
		XM_KillTimer (XMTIMER_VIDEOVIEW);

		continue_playback(videoViewData, videoViewData->playback_mode);
		return;
	}

	replaytime++;
	// 重新绘制
	XM_InvalidateWindow();
	XM_UpdateWindow();
}


VOID VideoViewOnLongTimeDown(XMMSG *msg)
{
	XM_printf(">>>>>>>>VideoViewOnLongTimeDown, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
	
	switch(msg->wp)
	{
		case VK_AP_UP:	
			XM_printf(">>>>>>>>>>>>>>VK_AP_UP.............\r\n");
			break;

		case VK_AP_DOWN:	
			XM_printf(">>>>>>>>>>>>VK_AP_DOWN...........\r\n");
			break;
			
		default:
			break;
	}
}


static VOID VideoViewOnSystemEvent (XMMSG *msg)
{
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:			// SD卡拔出事件
			XM_printf(">>>VideoViewOnSystemEvent, SYSTEM_EVENT_CARD_UNPLUG\r\n");
			XM_BreakSystemEventDefaultProcess (msg);		// 不进行缺省处理
			XM_JumpWindowEx(XMHWND_HANDLE(Desktop), 0, 0);
			break;

		default:
			break;
	}
}



static VOID VideoViewOnVideoStop (XMMSG *msg)
{
	VIDEOVIEWDATA *videoViewData;
	DWORD next_item;
	BYTE stop_code = LOBYTE(msg->wp);//AP_VIDEOEXITCODE_FINISH
	BYTE ext_info = HIBYTE(msg->wp);//videoViewData->playback_mode

	XM_printf(">>>>>>>>>>>>>>>VideoViewOnVideoStop, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoView));
	if(videoViewData == NULL)
		return;
	
	if( (stop_code==AP_VIDEO_PRE_NEXT) && (ext_info==XM_VIDEO_PLAYBACK_MODE_NORMAL) )
	{//上下一个文件播放
		videoViewData->playing = VIDEO_PLAY_STATE;//显示暂停图标
		continue_playback(videoViewData, ext_info);
	}
	else if( (stop_code==AP_VIDEOEXITCODE_FINISH) && (ext_info==XMSYS_H264_CODEC_STATE_STOP) )
	{
		XM_printf(">>>>>>>>>>>>>play compleate ok,,,,,,,,,,,,,\r\n");
		videoViewData->playing = VIDEO_PAUSE_STATE;//显示播放图标
		clear_videodecoder_times();
		XM_printf(">>>>>>>>>>>>>videoViewData->playing:%d\r\n", videoViewData->playing);
		XM_InvalidateWindow();
		XM_UpdateWindow();
	}
}


// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (VideoView)
	XM_ON_MESSAGE (XM_PAINT, VideoViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, VideoViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, VideoViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, VideoViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, VideoViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, VideoViewOnTimer)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, VideoViewOnSystemEvent)
	XM_ON_MESSAGE (XM_VIDEOSTOP, VideoViewOnVideoStop)
	XM_ON_MESSAGE (XMKEY_LONGTIME, VideoViewOnLongTimeDown)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, VideoView, 0, 0, 0, 0, HWND_VIEW)

