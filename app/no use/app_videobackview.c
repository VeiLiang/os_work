//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_videoview.c
//	  ��Ƶ�طŴ���
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

extern XMSYSTEMTIME rendervideotime;              //��ǰ��Ƶ�ļ�¼��ʱ��
extern DWORD dispyear,dispmonth,dispday,disphour, dispminute, dispsecond; //��ǰ��Ƶ�ļ���ʾʱ��

#define	ALBUM_BUTTON_AUTO_HIDE_TIMEOUT	5000	// ��ť/��Ƶ�����Զ�����ʱ�� 8000����

// ˽�������
#define	VIDEOVIEW_COMMAND_PAUSE_PLAY		0
#define	VIDEOVIEW_COMMAND_LOCK_UNLOCK		1
#define	VIDEOVIEW_COMMAND_RETURN			2
#define	VIDEOVIEW_COMMAND_PAINT				3

#define	VIDEO_PLAY_STATE		1
#define	VIDEO_PAUSE_STATE		0


// �����á����ڰ�ť�ؼ�����
#define	VIDEOVIEWBTNCOUNT	3


// �ط�ģʽ�µİ�ť�ؼ����� (3����ť)
static const XMBUTTONINFO videoViewBtn_0[VIDEOVIEWBTNCOUNT] = {
	{	
		// ����/��ͣ
		VK_AP_MENU,		VIDEOVIEW_COMMAND_PAUSE_PLAY,	AP_ID_COMMON_MENU,	AP_ID_VIDEO_BUTTON_PAUSE
	},
	{	
		// "����"��"����"
		VK_AP_SWITCH,	VIDEOVIEW_COMMAND_LOCK_UNLOCK,	AP_ID_COMMON_MODE,	AP_ID_VIDEO_VIEW_BUTTON_LOCK
	},
	{	
		// ���ص�"��Ƶ�б�"�Ӵ�
		VK_AP_MODE,		VIDEOVIEW_COMMAND_RETURN,	AP_ID_COMMON_OK,	AP_ID_BUTTON_RETURN
	},
};



typedef struct tagVIDEOVIEWDATA {
	int					nCurItem;					// ��ǰѡ��Ĳ˵���
	XMVIDEOITEM	*		video_item;

	BYTE					delete_one_confirm;
	BYTE					mode;							// ��ͨģʽ���߱���ģʽ
	BYTE					playing;						// 1 ���ڲ��� 0 ������ͣ
	BYTE					channel;						// ��Ƶ���ڵ�ͨ��(��0��ʼ)

	BYTE					button_hide;				// ��ť����ʾ/����     1 ���� 0 ��ʾ
	BYTE					video_title_hide;			// ��Ƶ�������ʾ/���� 1 ���� 0 ��ʾ
	BYTE					video_damaged;				// ��Ƶ�������
	BYTE					playback_mode;				// ��������/�������/���˲���

	XMBUTTONCONTROL	btnControl;

} VIDEOVIEWDATA;

// ����Ƶ������ͼ
VOID AP_OpenVideoBackView (BYTE mode, WORD wVideoFileIndex)
{
	XM_PushWindowEx (XMHWND_HANDLE(VideoBackView), (mode << 24) | wVideoFileIndex);
}

static VOID AP_ReturnToVideoListView (VIDEOVIEWDATA *videoViewData)
{
	// ����ǰ���ڲ��ŵ���Ƶ������Ϊ"��Ƶ�б��Ӵ�"�ĵ�ǰѡ����(������)
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
	videoViewData->video_title_hide = 0;	// ��ʾ��Ƶ����
		
	videoViewData->button_hide = 0;
	// ʹ���Ӵ��İ�ť��ʾ
	AP_ButtonControlSetFlag (&videoViewData->btnControl, 0);
	XM_InvalidateWindow ();
	XM_SetTimer (XMTIMER_VIDEOVIEW, ALBUM_BUTTON_AUTO_HIDE_TIMEOUT);
}

// ������Ƶ������/����״̬, ���õڶ�����ť����ۼ���Ϊ
static void modify_lock_unlock_button (VIDEOVIEWDATA *videoViewData)
{
	XMBUTTONINFO buttonSetting;
	buttonSetting.wCommand = VIDEOVIEW_COMMAND_LOCK_UNLOCK;
	buttonSetting.wKey = VK_AP_SWITCH; 
	buttonSetting.dwLogoId = AP_ID_COMMON_MODE;
	if(videoViewData->video_item->video_lock)
		buttonSetting.dwTextId = AP_ID_VIDEO_VIEW_BUTTON_UNLOCK;
	else
		buttonSetting.dwTextId = AP_ID_VIDEO_VIEW_BUTTON_LOCK;	// ����
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
		if(start_point == 0)		// �ļ�û�������ر�
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

	// �رչ���ANIMATION
	XM_DisableViewAnimate (XMHWND_HANDLE(VideoBackView));

	if(msg->wp == 0)
	{
		// ����δ��������һ�ν���
		DWORD lp = msg->lp;

		// ����˽�����ݾ��
		videoViewData = (VIDEOVIEWDATA *)XM_calloc (sizeof(VIDEOVIEWDATA));
		if(videoViewData == NULL)
		{
			XM_printf ("videoViewData XM_calloc failed\n");
			// ʧ�ܷ��ص������ߴ���
			XM_PullWindow (0);
			return;
		}
		
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(VideoBackView), videoViewData);

		if(get_video_info (lp >> 24, lp & 0xFFFF, videoViewData) < 0)
		{
			XM_printf ("videoViewData get_video_info failed\n");
			XM_SetWindowPrivateData (XMHWND_HANDLE(VideoBackView), NULL);
			XM_free (videoViewData);
			// ʧ�ܷ��ص������ߴ���
			XM_PullWindow (0);
			return;
		}

		#if 1
        //��ȡ�ļ�����ʱ��
        if(AP_VideoItemFormatVideoFileName (videoViewData->video_item, text, sizeof(text)) == NULL)
			return;
		
		// ����Ƶ���ļ����ƻ�ȡ¼��Ĵ���������ʱ��
		XM_GetDateTimeFromFileName ((char *)text, &filetime);

		rendervideotime.wYear = filetime.wYear;
		rendervideotime.bMonth = filetime.bMonth;
		rendervideotime.bDay = filetime.bDay;
		rendervideotime.bHour = filetime.bHour;
		rendervideotime.bMinute = filetime.bMinute;
		rendervideotime.bSecond = filetime.bSecond;
		#endif
		
		// ��ʼɨ�����з�����������Ƶ�ļ�
		videoViewData->button_hide = 0;
		videoViewData->video_title_hide = 0;

		videoViewData->playing = VIDEO_PLAY_STATE;

		// ��ť�ؼ���ʼ��
		AP_ButtonControlInit (&videoViewData->btnControl, VIDEOVIEWBTNCOUNT, 
			XMHWND_HANDLE(VideoBackView), videoViewBtn_0);

		// ���ذ�ť���Ķ���ˮƽ�ָ��߼�ÿ����ť֮��Ĵ�ֱ�ָ���
		AP_ButtonControlSetFlag (&videoViewData->btnControl,
			XMBUTTON_FLAG_HIDE_HORZ_SPLIT|XMBUTTON_FLAG_HIDE_VERT_SPLIT);

		modify_lock_unlock_button (videoViewData);

		if(decode_video (videoViewData) >= 0)
		{
			videoViewData->playing = 1;
		}
		else
		{
			// ����������һ��
			//set_video_damaged_state (videoViewData);
			XM_KeyEventProc (VK_AP_VIDEOSTOP, (SHORT)(AP_VIDEOEXITCODE_FINISH|(XM_VIDEO_PLAYBACK_MODE_NORMAL << 8)) );
		}
	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�		
		videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackView));
		if(videoViewData == NULL)
			return;

		XM_printf ("VideoBackView Pull\n");
	}

	// ������ʱ�������ڰ�ť����Ƭ���������
	// ����x��Ķ�ʱ��
	XM_SetTimer (XMTIMER_VIDEOVIEW, ALBUM_BUTTON_AUTO_HIDE_TIMEOUT);
}

static VOID VideoBackViewOnLeave (XMMSG *msg)
{
	XM_printf ("VideoViewOnLeave %d\n", msg->wp);
	// ɾ����ʱ��
	XM_KillTimer (XMTIMER_VIDEOVIEW);

	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		VIDEOVIEWDATA *videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackView));
		XMSYS_H264CodecPlayerStop ();
		// �ͷ����з������Դ
		if(videoViewData)
		{
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&videoViewData->btnControl);
			// �ͷ�˽�����ݾ��
			XM_free (videoViewData);
		}
		// �����Ӵ���˽�����ݾ��Ϊ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(VideoBackView), NULL);
             OS_Delay (100);
		
		// �ر���Ƶ���(OSD 0��ر�)
		XM_osd_framebuffer_release (0, XM_OSD_LAYER_0);
		XM_printf ("VideoBackView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
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
	// ����Ƶ���ļ����ƻ�ȡ¼��Ĵ���������ʱ��
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

	// ��ʾ��Ƶ��¼�ƿ�ʼʱ��, �Ӵ������Ͻ�(10, 10)																	
	AP_TextOutDataTimeString (XMHWND_HANDLE(VideoBackView), x, y, text, strlen(text));
}

static void clear_video_layer (void)
{
	xm_osd_framebuffer_t yuv_framebuffer;
	unsigned int w, h;
	// ��ȡ��Ƶ��ĳߴ���Ϣ
	w = XM_lcdc_osd_get_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	h = XM_lcdc_osd_get_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	// ����һ���µ���Ƶ���֡������
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
	// ���framebuffer
	XM_osd_framebuffer_clear (yuv_framebuffer, 0, 255, 255, 255);
	// ��framebuffer�ر�, �������Ϊ��ǰ����Ƶ����, ����OSD 0����ʾ
	XM_osd_framebuffer_close (yuv_framebuffer, 0);

}

static VOID VideoBackViewOnPaint (XMMSG *msg)
{
	XMRECT rc;
	unsigned int old_alpha;
	HANDLE hWnd;

	float scale_factor;		// ˮƽ��������

	// ��ȡ���Ӵ�������˽������
	VIDEOVIEWDATA *videoViewData = (VIDEOVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackView));
	if(videoViewData == NULL)
		return;

	// ��ȡ���ڵľ��
	hWnd = XMHWND_HANDLE(VideoBackView);

	// VideoView����ʱ���Ӵ�Alpha����Ϊ0 (�ο��ļ�����ײ�����)
	// ���������Ϊȫ͸������(AlphaΪ0)
	XM_GetDesktopRect (&rc);
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);
	XM_FillRect (hWnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	// �����Ӵ���Alpha����Ϊ255
	old_alpha =  XM_GetWindowAlpha (hWnd);
	XM_SetWindowAlpha (hWnd, 255);

	// ������Ƶ�������ʾ 
	// ��Ƶ�ļ���ʱ��Ҫ��ʾ��Ƶ������ʾ�û�
	if(!videoViewData->video_title_hide || videoViewData->video_damaged)
	{
		// ��ʾ��Ƶ�ı���
		ShowVideoTime (videoViewData);
	}

	if(videoViewData->video_item->video_lock)
	{
		// ��ʾ�������
		XMRECT rect = rc;
		rect.left = 24;
		rect.top = 24;
		AP_RomImageDrawByMenuID (AP_ID_COMMON_LOCK_32, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	}

    if(!(videoViewData->playing))
	{
		// ��ʾ��ͣ
		XMRECT rectstop = rc;
		rectstop.left = 90;
		rectstop.top = 24;
		AP_RomImageDrawByMenuID (AP_ID_COMMON_PAUSE, hWnd, &rectstop, XMGIF_DRAW_POS_LEFTTOP);
	}
	
	// ��Ƶ�������
	if(videoViewData->video_damaged)
	{
		// ��ʾ������Ϣ
		AP_RomImageDrawByMenuID (AP_ID_VIDEO_INFO_PHOTO_DAMAGE, hWnd, &rc, XMGIF_DRAW_POS_CENTER);
		clear_video_layer ();
	}

	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
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
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
		case VK_AP_SWITCH:	// �����л���
		case VK_AP_POWER:		// ��Դ��
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
			// ��"¼�����"�����У�MENU��Power��Switch��MODE��������Ϊ��ť������
			// �˴������ĸ����¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&videoViewData->btnControl, msg);
			break;

		case VK_AP_UP:		
			// ���
			if(videoViewData->video_damaged)
				return;
			XMSYS_H264CodecPlayerBackward ();
			break;

		case VK_AP_DOWN:	// 
			// ����
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
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
		case VK_AP_POWER:
		case VK_AP_SWITCH:
			// ��"ʱ������"�����У�MENU��MODE��������Ϊ��ť������
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
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
			// ���»���
			XM_InvalidateWindow ();
			break;
		}
		
		case VIDEOVIEW_COMMAND_PAUSE_PLAY:
		{
			XMBUTTONINFO buttonSetting;
			// ��ͣ/����
			// ���ݵ�ǰ���ŵ�״̬,�޸ĵ�һ����ť����ʾ����Ϊ
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
			// �޸ĵ�һ����ť����Ϣ
			buttonSetting.wCommand = VIDEOVIEW_COMMAND_PAUSE_PLAY;
			buttonSetting.wKey = VK_AP_MENU;
			buttonSetting.dwLogoId = AP_ID_COMMON_MENU;
			if(videoViewData->playing)
				buttonSetting.dwTextId = AP_ID_VIDEO_BUTTON_PAUSE;
			else
				buttonSetting.dwTextId = AP_ID_VIDEO_BUTTON_PLAY;
			AP_ButtonControlModify (&videoViewData->btnControl, 0, &buttonSetting);
			// ���»���
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
	
	// ������һ����Ƶ
	if(playback_mode == XM_VIDEO_PLAYBACK_MODE_NORMAL || playback_mode == XM_VIDEO_PLAYBACK_MODE_FORWARD)
	{
		// Ѱ����֮ʱ�����ڵ���һ����Ƶ����
		next_item = AP_VideoItemGetNextVideoIndex (videoViewData->mode, videoViewData->nCurItem);
		if(next_item == (DWORD)(-1) || get_video_info (videoViewData->mode, next_item, videoViewData) < 0)
		{
			// ��һ����Ƶ������
			AP_ReturnToVideoListView (videoViewData);
		}
		else
		{
			// ��һ����Ƶ���ҵ�
			videoViewData->playback_mode = playback_mode;
			
			#if 1
	        //��ȡ�ļ�����ʱ��
	        if(AP_VideoItemFormatVideoFileName (videoViewData->video_item, text, sizeof(text)) == NULL)
				return;
			
			// ����Ƶ���ļ����ƻ�ȡ¼��Ĵ���������ʱ��
			XM_GetDateTimeFromFileName ((char *)text, &filetime);

			rendervideotime.wYear = filetime.wYear;
			rendervideotime.bMonth = filetime.bMonth;
			rendervideotime.bDay = filetime.bDay;
			rendervideotime.bHour = filetime.bHour;
			rendervideotime.bMinute = filetime.bMinute;
			rendervideotime.bSecond = filetime.bSecond;
			#endif

			// ����
			if(decode_video (videoViewData) < 0)
			{
				// ����ʧ��
				// set_video_damaged_state (videoViewData);
				XM_KeyEventProc (VK_AP_VIDEOSTOP, (SHORT)(AP_VIDEOEXITCODE_FINISH|(playback_mode << 8)) );
			}
			// eason 20170712 ������һ����Ƶʱ���¿ؼ�״̬
			modify_lock_unlock_button (videoViewData);
			// ���»���
			XM_InvalidateWindow ();
		}
	}
	else if(playback_mode == XM_VIDEO_PLAYBACK_MODE_BACKWARD)
	{
		// Ѱ����֮ʱ�����ڵ�ǰһ����Ƶ����
		next_item = AP_VideoItemGetPrevVideoIndex (videoViewData->mode, videoViewData->nCurItem);
		if(next_item == (DWORD)(-1) || get_video_info (videoViewData->mode, next_item, videoViewData) < 0)
		{
			// ��һ����Ƶ������
			AP_ReturnToVideoListView (videoViewData);
		}
		else
		{
			// ��һ����Ƶ���ҵ�
			videoViewData->playback_mode = playback_mode;
			// ����
			if(decode_video (videoViewData) < 0)
			{
				// ����ʧ��
				// set_video_damaged_state (videoViewData);
				XM_KeyEventProc (VK_AP_VIDEOSTOP, (SHORT)(AP_VIDEOEXITCODE_FINISH|(playback_mode << 8)) );
			}

		}

	}
}

// �����޲���������������, ȱʡΪ3��
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
		videoViewData->video_title_hide = 1;		// ������Ƭ�ı���
		XM_InvalidateWindow ();
	}
}

static VOID VideoBackViewOnSystemEvent (XMMSG *msg)
{
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:			// SD���γ��¼�
			XM_BreakSystemEventDefaultProcess (msg);		// ������ȱʡ����
			// ������Ϣ���²��뵽��Ϣ���ж���
			XM_InsertMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
			XM_JumpWindowEx (XMHWND_HANDLE(Desktop), HWND_CUSTOM_DEFAULT, XM_JUMP_POPDEFAULT);
			//XM_JumpWindow (XMHWND_HANDLE(Desktop));		// ��ת������
			//XM_PullWindow (0);					// ������ǰ���б��Ӵ�
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


// ��Ϣ����������
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

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, VideoBackView, 0, 0, 0, 0, HWND_VIEW)
