//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_videoplayview.c
//	  ��Ƶ���Ŵ���
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
// *************   ��Ƶ������   ******************
//
// -----------------------------------------------
// ��Ƶ�������Ĺ���
//	1) UI��ʾ(����������)
// 2) ��������(AP_VideoSendCommand)������Ƶ�����̵Ĳ��š���ͣ��������ֹͣ��
//
// ��Ƶ�����������
// 1) ��Ƶ������UI�ı���ɫΪ��ɫ��
//		��ʾ�����Զ�ʹ��alpha���ӽ�����Ƶ������UI���롰��Ƶ�������л�ϲ���ʾ
// 2) ����Ƶ����������Ƶ������UI����Ϊȫ����ʾ
// 3) ����������Ϊ͸��ɫͼ�꣬��5��ͼ��
//		��������ť������֧�ּ���/��ͣ��ǰһ������һ�����˳�(ģʽ�л�)�����л��л�(��ͼ�л�)
//		������ť�ڰ����޲���10����Զ����ء�
//		������ť���غ󣬰��κμ���������ʾ������ť
// 4) ��Ƶ��������VideoListView���롣�˳�ʱ���ص�VideoListView
// 5) ��Ƶ���Ź����У�����⵽���γ���ֱ����תJump������(���渺�𿨲�ε����д���)
// 6) ���Ž�������Ƶ�����̷���XM_VIDEOSTOP��Ϣ��֪ͨ���Ž�����
//		�˳����ж��������ԭ��(app_video.h)

#define	TIMETOHIDETOOLBAR		10*2		// 10��

// ��Ƶ������״̬����
enum {
	VIDEOPLAYSTATE_STOP = 0,			// ֹͣ״̬
	VIDEOPLAYSTATE_PLAY,					// ����״̬
	VIDEOPLAYSTATE_PAUSE,				// ��ͣ״̬
};

typedef struct tagVIDEOPLAYVIEWDATA {
	BYTE		mode;							// ѭ��ģʽ���Ǳ���ģʽ
	DWORD		wVideoFileIndex;			// ���ڲ�����Ƶ�ļ����������
	BYTE		bVideoPlayState;			// ��Ƶ������״̬
	XMBOOL	bToolbarShow;				//	1 ��ʾ������������ʾ
												// 0 ��ʾ��������������
	BYTE		bTimeToHideToolbar;		// ��������������ʾ��ʣ��ʱ��(��Ϊ��λ)
												//		ÿ�ζ�ʱ���¼�����ʱ����������һ��
												//		Ϊ0ʱ���ظ���������
} VIDEOPLAYVIEWDATA;

// ����Ƶ������ͼ
VOID AP_OpenVideoPlayView (BYTE mode, WORD wVideoFileIndex)
{
	XM_PushWindowEx (XMHWND_HANDLE(VideoPlayView), (mode << 24) | wVideoFileIndex);
}

// ��ʾ������
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
	// �����˳���Ϣ
	XM_PeekMessage (&msg, XM_VIDEOSTOP, XM_VIDEOSTOP);
	return 1;
}


static XMBOOL PlayVideo (VIDEOPLAYVIEWDATA *videoMenuData)
{
	// ��ֹ��ǰ��Ƶ����
	HANDLE hVideoItem;
	XMVIDEOITEM *pVideoItem;
	VIDEO_PLAY_PARAM playParam;
	char videoPath[MAX_APP_VIDEOPATH]; 
	char videoPath2[MAX_APP_VIDEOPATH]; 
	XMBOOL ret = 0;

	if(videoMenuData->bVideoPlayState == VIDEOPLAYSTATE_STOP)
	{
		// ֹͣ״̬
		// ��ȡ��Ƶ�ļ�ȫ·����
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
		// ��ͣ״̬
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
		// ����δ��������һ�ν���

		// ����˽�����ݾ��
		videoPlayViewListData = XM_calloc (sizeof(VIDEOPLAYVIEWDATA));
		if(videoPlayViewListData == NULL)
		{
			XM_printf ("videoPlayViewListData XM_calloc failed\n");
			// ʧ�ܷ��ص������ߴ���
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
			// �����ʧ��
			XM_free (videoPlayViewListData);
			XM_printf ("AP_VideoSendCommand AP_VIDEOCOMMAND_PLAY failed\n");
			// ʧ�ܷ��ص������ߴ���
			XM_PullWindow (0);
			return;
		}

		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(VideoPlayView), videoPlayViewListData);
	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�		
		XM_printf ("VideoPlayView Pull\n");
	}

	// ������ʱ��������ʱ��ˢ��
	// ����0.5��Ķ�ʱ��
	XM_SetTimer (XMTIMER_VIDEOPLAYVIEW, 500);

}

VOID VideoPlayViewOnLeave (XMMSG *msg)
{
	XM_printf ("VideoPlayViewOnLeave %d\n", msg->wp);
	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		VIDEOPLAYVIEWDATA *videoPlayViewListData = (VIDEOPLAYVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoPlayView));
		// �ͷ����з������Դ
		if(videoPlayViewListData)
		{
			// �ͷ�˽�����ݾ��
			XM_free (videoPlayViewListData);
		}
		XM_printf ("VideoPlayView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
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

	// ����Ƿ���ʾ���Ź�����
	if(videoMenuData->bToolbarShow)
	{
		rect = rc;
		rect.top = APP_POS_VIDEOPLAY_TOOLBAR_Y;
		AppRes = AP_AppRes2RomRes (AP_ID_COMMON_VIDEOTOOLBARBACKGROUND);
		XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(VideoPlayView), &rect, XMGIF_DRAW_POS_LEFTTOP);

		// ��ʽ������ļ���
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
			// ˢ��
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
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_POWER:
			if(	videoMenuData->bVideoPlayState == VIDEOPLAYSTATE_STOP
				||	videoMenuData->bVideoPlayState == VIDEOPLAYSTATE_PAUSE)
			{
				// ��ͣ��ֹͣ
				PlayVideo (videoMenuData);
			}
			else
			{
				// ����״̬, �ָ�Ϊֹͣ״̬
				AP_VideoSendCommand (AP_VIDEOCOMMAND_STOP, 0);
				videoMenuData->bVideoPlayState = VIDEOPLAYSTATE_STOP;
			}
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_MODE:		
			if(	videoMenuData->bVideoPlayState == VIDEOPLAYSTATE_STOP
				||	videoMenuData->bVideoPlayState == VIDEOPLAYSTATE_PAUSE)
			{
				// �˳�
				XM_PullWindow (0);
			}
			else
			{
				StopVideo (videoMenuData);
				// ˢ��
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			break;

		case VK_AP_SWITCH:
			// ��ʱδ����������ͼ�л�
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

	// ��⿨״̬
	if(APSYS_CardChecking() == APP_SYSTEMCHECK_CARD_NOCARD)
	{
		// ���γ���ֱ�������ص�����(���渺�𿨲�ε����д���), �˴�ǿ���������³�ʼ��
		StopVideo (videoMenuData);
		XM_JumpWindow (XMHWND_HANDLE(Desktop));
		return;
	}
	// ��鲥��״̬
	if(videoMenuData->bVideoPlayState == VIDEOPLAYSTATE_PLAY)
	{
		if(videoMenuData->bToolbarShow)
		{
			// ��鹤�����Ƿ����
			videoMenuData->bTimeToHideToolbar --;
			if(videoMenuData->bTimeToHideToolbar == 0)
			{
				videoMenuData->bToolbarShow = 0;

				// ˢ��
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
		}
	}
	else if(videoMenuData->bToolbarShow == 0)
	{
		// �ǲ���״̬�£�Ӧ��ʾ������
		videoMenuData->bToolbarShow = 1;
		// ˢ��
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
	// ˢ��
	XM_InvalidateWindow ();
	XM_UpdateWindow ();
}

static VOID VideoPlayViewOnSystemEvent (XMMSG *msg)
{
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:			// SD���γ��¼�
			XM_BreakSystemEventDefaultProcess (msg);		// ������ȱʡ����
			// ������Ϣ���²��뵽��Ϣ���ж���
			XM_InsertMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
			XM_JumpWindowEx (XMHWND_HANDLE(Desktop), HWND_CUSTOM_DEFAULT, XM_JUMP_POPDEFAULT);
		//	XM_JumpWindow (XMHWND_HANDLE(Desktop));		// ��ת������
			//XM_PullWindow (0);					// ������ǰ���б��Ӵ�
			break;
	}
}

// ��Ϣ����������
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

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, VideoPlayView, 0, 0, 0, 255, HWND_VIEW)
