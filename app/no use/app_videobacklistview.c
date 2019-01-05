//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_videolistview.c
//	  ��Ƶ�б��������
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************

#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"
#include "system_check.h"
#include "xm_h264_codec.h"
#include "gpio.h"


// ˽�������
#define	VIDEOBACKLISTVIEWLIST_COMMAND_PLAY			1
#define	VIDEOBACKLISTVIEWLIST_COMMAND_SWITCH		2
#define	VIDEOBACKLISTVIEWLIST_COMMAND_RETURN		3
#define VIDEOBACKLISTVIEWLIST_COMMAND_UP            4
#define VIDEOBACKLISTVIEWLIST_COMMAND_DOWN         	5

// �����á����ڰ�ť�ؼ�����
#define	VIDEOBACKLISTVIEWLISTBTNCOUNT	4


// ����¼��ģʽ�µİ�ť�ؼ�����(������˳����)
static const XMBUTTONINFO videoBackListViewBtn_0[VIDEOBACKLISTVIEWLISTBTNCOUNT] = {
       {	
		VK_AP_UP,	VIDEOBACKLISTVIEWLIST_COMMAND_UP,	AP_ID_BUTTON_LAST,	AP_ID_BUTTON_LAST
	},
	{	
		VK_AP_DOWN,	VIDEOBACKLISTVIEWLIST_COMMAND_DOWN,	AP_ID_BUTTON_NEXT,	AP_ID_BUTTON_NEXT
	},
	{	
		//VK_AP_SWITCH,	VIDEOLISTVIEWLIST_COMMAND_SWITCH,	AP_ID_COMMON_SWITCH,	AP_ID_VIDEO_PROTECTEDVIDEO
		VK_AP_MENU,	VIDEOBACKLISTVIEWLIST_COMMAND_SWITCH,	AP_ID_COMMON_MODE,	AP_ID_VIDEO_BUTTON_ALBUM
	},
	{	
		VK_AP_MODE,		VIDEOBACKLISTVIEWLIST_COMMAND_RETURN,	AP_ID_COMMON_OK,	AP_ID_BUTTON_RETURN
	},
};

// ����¼��ģʽ�µİ�ť�ؼ�����(������˳����)
static const XMBUTTONINFO videoBackListViewBtn_1[VIDEOBACKLISTVIEWLISTBTNCOUNT] = {
	   {	
		VK_AP_UP,	VIDEOBACKLISTVIEWLIST_COMMAND_UP,	AP_ID_BUTTON_LAST,	AP_ID_BUTTON_LAST
	},
	{	
		VK_AP_DOWN,	VIDEOBACKLISTVIEWLIST_COMMAND_DOWN,	AP_ID_BUTTON_NEXT,	AP_ID_BUTTON_NEXT
	},
	{	
		//VK_AP_SWITCH,	VIDEOLISTVIEWLIST_COMMAND_SWITCH,	AP_ID_COMMON_SWITCH,	AP_ID_VIDEO_BUTTON_VIDEO
		VK_AP_MENU,	VIDEOBACKLISTVIEWLIST_COMMAND_SWITCH,	AP_ID_COMMON_MODE,	AP_ID_VIDEO_BUTTON_RECYCLE
	},
	{	
		VK_AP_MODE,		VIDEOBACKLISTVIEWLIST_COMMAND_RETURN,	AP_ID_COMMON_OK,	AP_ID_BUTTON_RETURN
	},

};

typedef struct tagVIDEOBACKLISTVIEWLISTDATA {
	int					nTopItem;					// ��һ�����ӵĲ˵���
	int					nCurItem;					// ��ǰѡ��Ĳ˵���

	int					nItemCount;					// �˵������
	int					nVisualCount;				// �Ӵ�����ʾ����Ŀ����
	
	BYTE				mode;

	XMBUTTONCONTROL		btnControl;
    HANDLE				hMoniorSwitchControl;		//ͣ�����
	char				fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	XMFILEFIND			fileFind;
} VIDEOBACKLISTVIEWLISTDATA;


#if 0
static void SwitchMode (VIDEOLISTVIEWLISTDATA *videoListViewListData)
{
	// ��ť�ؼ��˳�����
	AP_ButtonControlExit (&videoListViewListData->btnControl);

	// ��ѭ��¼���롰����¼���б�ģʽ֮���л�
//	videoListViewListData->mode = (BYTE)((videoListViewListData->mode + 1) & 1);

	// �����µİ�ť�ؼ�
	if(videoListViewListData->mode == VIDEOLIST_ALL_VIDEO_MODE)		// ȫ��¼��
		AP_ButtonControlInit (&videoListViewListData->btnControl, VIDEOLISTVIEWLISTBTNCOUNT, 
			XMHWND_HANDLE(VideoListView), videoListViewBtn_0);
	else if(videoListViewListData->mode == VIDEOLIST_PROTECT_VIDEO_MODE)	// ����¼��
		AP_ButtonControlInit (&videoListViewListData->btnControl, VIDEOLISTVIEWLISTBTNCOUNT, 
			XMHWND_HANDLE(VideoListView), videoListViewBtn_1);
	else		// ѭ��¼��
		AP_ButtonControlInit (&videoListViewListData->btnControl, VIDEOLISTVIEWLISTBTNCOUNT, 
			XMHWND_HANDLE(VideoListView), videoListViewBtn_0);


	// ���¹�����Ƶ�ļ��б�
	// ��ʼɨ�����з�����������Ƶ�ļ�
	// AP_VideoListScanVideoFileList (&videoListViewListData->fileFind, videoListViewListData->mode);
	videoListViewListData->nTopItem = 0;
	videoListViewListData->nCurItem = 0;
	// videoListViewListData->nItemCount = AP_VideoListGetVideoFileCount();
	videoListViewListData->nItemCount = (WORD)AP_VideoItemGetVideoFileCount(videoListViewListData->mode);

	// ˢ��
	XM_InvalidateWindow ();
	XM_UpdateWindow ();
}
#endif

static void set_focus_item (VIDEOBACKLISTVIEWLISTDATA *videoBackListViewListData)
{
		// ����ɨ�����з�����������Ƶ�ļ�
		videoBackListViewListData->nItemCount = (WORD)AP_VideoItemGetVideoFileCountEx(videoBackListViewListData->mode,XM_VIDEO_CHANNEL_1);
              
		// ����Ӵ����ӵ���Ŀ�Ƿ���Ҫ����
		if(videoBackListViewListData->nItemCount == 0)
		{
		    
			videoBackListViewListData->nTopItem = 0;
			videoBackListViewListData->nCurItem = 0;
		}
		else
		{
			if(videoBackListViewListData->nTopItem >= videoBackListViewListData->nItemCount)
			{
			    
				videoBackListViewListData->nTopItem = (WORD)(videoBackListViewListData->nItemCount - 1);
			}
			if(videoBackListViewListData->nCurItem >= videoBackListViewListData->nItemCount)
			{
			     
				videoBackListViewListData->nCurItem = (WORD)(videoBackListViewListData->nItemCount - 1);
			}
			if(videoBackListViewListData->nCurItem < videoBackListViewListData->nTopItem)
				videoBackListViewListData->nTopItem = videoBackListViewListData->nCurItem;
		}

		// ����Ƿ���Ҫ����������Ŀ���Ա㾡����������ʾ
		if(videoBackListViewListData->nItemCount <= videoBackListViewListData->nVisualCount)
		{
			videoBackListViewListData->nTopItem = 0;
		}
		else
		{
			if( (videoBackListViewListData->nTopItem + videoBackListViewListData->nVisualCount - 1) < videoBackListViewListData->nCurItem )
				videoBackListViewListData->nTopItem = (short)(videoBackListViewListData->nCurItem - videoBackListViewListData->nVisualCount + 1);

			while( (videoBackListViewListData->nTopItem + videoBackListViewListData->nVisualCount) > videoBackListViewListData->nItemCount )
			{
				videoBackListViewListData->nTopItem --;
			}
			XM_ASSERT ((videoBackListViewListData->nCurItem - videoBackListViewListData->nTopItem + 1) <= videoBackListViewListData->nVisualCount);
		}

}

// ����¼���б��Ӵ��ľ۽���
XMBOOL AP_VideoBackListViewSetFocusItem (UINT uFocusItem)
{
	VIDEOBACKLISTVIEWLISTDATA *videoBackListViewListData;
	videoBackListViewListData = (VIDEOBACKLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackListView));
	if(videoBackListViewListData)
	{
		videoBackListViewListData->nCurItem = (SHORT)uFocusItem;
		
		return 1;
	}
	return 0;
}

static VOID VideoBackListViewOnEnter (XMMSG *msg)
{
	VIDEOBACKLISTVIEWLISTDATA *videoBackListViewListData;
	
	XM_printf(">>>>>>>>>>>>>>>>>>>>VideoBackListViewOnEnter, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	if(msg->wp == 0)
	{
		XMRECT rc;
		int nVisualCount;
		// ����δ��������һ�ν���
		BYTE mode = VIDEOLIST_ALL_VIDEO_MODE;

		// ����˽�����ݾ��
		videoBackListViewListData = XM_calloc (sizeof(VIDEOBACKLISTVIEWLISTDATA));
		if(videoBackListViewListData == NULL)
		{
			XM_printf ("videoListViewListData XM_calloc failed\n");
			// ʧ�ܷ��ص������ߴ���
			XM_PullWindow (0);
			return;
		}
		
		if(msg->lp == VIDEOLIST_PROTECT_VIDEO_MODE || msg->lp == VIDEOLIST_CIRCULAR_VIDEO_MODE)
			mode = (BYTE)msg->lp;		// �ⲿ�����ģʽ
		videoBackListViewListData->mode = mode;

		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(VideoBackListView), videoBackListViewListData);

		// ��ʼɨ�����з�����������Ƶ�ļ�
		videoBackListViewListData->nTopItem = 0;
		videoBackListViewListData->nCurItem = 0;
		videoBackListViewListData->nItemCount = (WORD)AP_VideoItemGetVideoFileCountEx(videoBackListViewListData->mode,XM_VIDEO_CHANNEL_1);
             
		// ���������ʾ����Ŀ����
		XM_GetDesktopRect (&rc);
		nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
		nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
		nVisualCount -= 1;
	    //videoBackListViewListData->nVisualCount = (SHORT)nVisualCount;
               
		if(videoBackListViewListData->nItemCount>=nVisualCount)
		{
			videoBackListViewListData->nVisualCount = (SHORT)nVisualCount;
		}
		else
		{
            videoBackListViewListData->nVisualCount=videoBackListViewListData->nItemCount;
		}
		#if 0
		// ��ť�ؼ���ʼ��
		if(videoBackListViewListData->mode == VIDEOLIST_ALL_VIDEO_MODE)		// ȫ��¼��
			AP_ButtonControlInit (&videoBackListViewListData->btnControl, VIDEOBACKLISTVIEWLISTBTNCOUNT, 
				XMHWND_HANDLE(VideoBackListView), videoBackListViewBtn_0);
		else if(videoBackListViewListData->mode == VIDEOLIST_PROTECT_VIDEO_MODE)	// ����¼��
			AP_ButtonControlInit (&videoBackListViewListData->btnControl, VIDEOBACKLISTVIEWLISTBTNCOUNT, 
				XMHWND_HANDLE(VideoBackListView), videoBackListViewBtn_1);
		else		// ѭ��¼��
			AP_ButtonControlInit (&videoBackListViewListData->btnControl, VIDEOBACKLISTVIEWLISTBTNCOUNT, 
				XMHWND_HANDLE(VideoBackListView), videoBackListViewBtn_0);
		#endif
	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�		
		videoBackListViewListData = (VIDEOBACKLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackListView));
		if(videoBackListViewListData == NULL)
			return;

		set_focus_item (videoBackListViewListData);
           // videoListViewListData->nCurItem=-1;
		XM_printf ("VideoBackListView Pull\n");
	}
	// ������ʱ�������ڿ��γ����
	// ����0.5��Ķ�ʱ��
	XM_SetTimer (XMTIMER_VIDEOLISTVIEW, 500);
}

VOID VideoBackListViewOnLeave (XMMSG *msg)
{
	// ɾ����ʱ��
	XM_printf(">>>>>>>>>>>>>>>>>>>>VideoBackListViewOnLeave, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
	XM_KillTimer (XMTIMER_VIDEOLISTVIEW);

	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		VIDEOBACKLISTVIEWLISTDATA *videoBackListViewListData = (VIDEOBACKLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackListView));
		// �ͷ����з������Դ
		if(videoBackListViewListData)
		{
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&videoBackListViewListData->btnControl);
			// �ͷ�˽�����ݾ��
			XM_free (videoBackListViewListData);
			// �����Ӵ���˽�����ݾ��Ϊ��
			XM_SetWindowPrivateData (XMHWND_HANDLE(VideoBackListView), NULL);
		}

		XM_printf ("VideoBackListView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("VideoBackListView Push\n");
	}
}

#define Thumb_width 304
#define Thumb_height 208
#define Thumb_Hor_Interval 28
#define Thumb_Vert_Interval 28

VOID VideoBackListViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
	int i, item;
	XMCOORD x, y;
	//DWORD dwFileAttribute;
	unsigned int old_alpha;
	HANDLE hWnd;
	int nVisualCount;
	int diff_val;
	char text[64];
	XMSIZE size;
	float scale_factor;		// ˮƽ��������
	XMSYSTEMTIME filetime;
	XMCOORD item_icon_x, item_name_x, item_ch_x;	// �����������ͼ��, ����ʱ��, ͨ���ŵ�x����

	XM_printf(">>>>>>>>>>>>>>>>>>>>VideoBackListViewOnPaint, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	VIDEOBACKLISTVIEWLISTDATA *videoBackListViewListData = (VIDEOBACKLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackListView));
	if(videoBackListViewListData == NULL)
		return;

	hWnd = XMHWND_HANDLE(VideoBackListView);
	XM_GetDesktopRect (&rc);
	if(videoBackListViewListData->nItemCount <= 0)
	{
		XM_PullWindow (0);
	}
	// ����ˮƽ��������(UI��320X240������)
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);
	XM_FillRect (hWnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));
	old_alpha =  XM_GetWindowAlpha (hWnd);
	XM_SetWindowAlpha (hWnd, 255);

	item_icon_x = (XMCOORD)(APP_POS_MEDIA_TYPE_X * scale_factor);
	item_name_x = (XMCOORD)(APP_POS_MEDIA_NAME_X * scale_factor);
	item_ch_x   = (XMCOORD)(APP_POS_MEDIA_CHANNEL_X * scale_factor);
	// --------------------------------------
	//
	// ********* 1 ��ʾ����������Ϣ *********
	//
	// --------------------------------------
	if(videoBackListViewListData->mode == VIDEOLIST_ALL_VIDEO_MODE)
		AP_DrawTitlebarControl (hWnd, AP_NULLID, AP_ID_VIDEO_TITLE_VIDEO);
	else if(videoBackListViewListData->mode == VIDEOLIST_CIRCULAR_VIDEO_MODE)
		AP_DrawTitlebarControl (hWnd, AP_NULLID, AP_ID_VIDEO_TITLE_CIRCULARVIDEO);
	else if(videoBackListViewListData->mode == VIDEOLIST_PROTECT_VIDEO_MODE)
		AP_DrawTitlebarControl (hWnd, AP_NULLID, AP_ID_VIDEO_TITLE_PROTECTEDVIDEO);


	rect.left = 300;rect.top = 0;rect.right = 65;rect.bottom = 36;
	AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_CH, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	rect.left = 365;rect.top = 0;rect.right = 65;rect.bottom = 36;
	AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_CH2, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	
	// ��5����ʾ��Ƶ�б�
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;

  	#if 0
	// ��ʾ�ָ���
	rect = rc;
	for (i = 0; i < nVisualCount; i++)
	{
		rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * scale_factor);
		rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + (i+1) * APP_POS_ITEM5_LINEHEIGHT);
		AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	}
	
	// ��ʾѡ�����
	if(videoBackListViewListData->nCurItem!=-1)
	{
		rect = rc;
		rect.left = 0;
		rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (videoBackListViewListData->nCurItem - videoBackListViewListData->nTopItem) * APP_POS_ITEM5_LINEHEIGHT);
		AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMBACKGROUND, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	}
	// ��ʾĿ¼��
	x = item_name_x;
	y = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1);
	for (i = 0; i < videoBackListViewListData->nVisualCount; i++)
	{
		HANDLE hVideoItem;
		XMVIDEOITEM *pVideoItem;
		DWORD dwVideoTime;
		DWORD h, m, s;
		XMSYSTEMTIME filetime;
		int ch;
		XMSIZE size;

		item = videoBackListViewListData->nTopItem + i;
		if(item >= videoBackListViewListData->nItemCount)
			break;

		// AP_VideoListFormatVideoFileName (item, text, sizeof(text));
		// ��ȡ��Ƶ���ļ���
		hVideoItem = AP_VideoItemGetVideoItemHandleEx (videoBackListViewListData->mode, item,XM_VIDEO_CHANNEL_1);
		if(hVideoItem == NULL)
			break;
		
		pVideoItem = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
		if(pVideoItem == NULL)
			break;

		memset (text, 0, sizeof(text));
		if(AP_VideoItemFormatVideoFileName (pVideoItem, text, sizeof(text)) == NULL)
			break;
		// ����Ƶ���ļ����ƻ�ȡ¼��Ĵ���������ʱ��
		XM_GetDateTimeFromFileName ((char *)text, &filetime);
		sprintf (text, "%04d/%02d/%02d %02d:%02d:%02d", filetime.wYear, 
																		filetime.bMonth,
																		filetime.bDay,
																		filetime.bHour,
																		filetime.bMinute,
																		filetime.bSecond);
		AP_TextGetStringSize (text, strlen(text), &size);
		AP_TextOutDataTimeString (hWnd, item_name_x, y + (APP_POS_ITEM5_LINEHEIGHT - size.cy)/2, text, strlen(text));

		/*
		// ������Ƶ��ʱ�䳤����Ϣ
		dwVideoTime = AP_VideoItemGetVideoStreamSize (pVideoItem, 0);		// 0ͨ��
		h = dwVideoTime / (1000 * 3600);
		m = ((dwVideoTime / 1000) / 60) % 60;
		s = (dwVideoTime / 1000) % 60;
		sprintf (text, "%02d:%02d:%02d", h, m, s);
		AP_TextOutDataTimeString (hWnd, menu_data_x, y, text, strlen(text));
		*/

		// ������ԣ�����ֻ�����ԣ�����������Ƶ��Ӧ�����������
		// dwFileAttribute = AP_VideoListGetVideoFileAttribute (item);
		// if( dwFileAttribute & D_RDONLY)
		if(pVideoItem->video_lock)
		{
			rect = rc;
			rect.top = y;
			rect.bottom = rect.top + APP_POS_ITEM5_LINEHEIGHT - 1;
			rect.left = item_icon_x;
			rect.right = rect.left + 24;
			AP_RomImageDrawByMenuID (AP_ID_COMMON_KEY, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		}

		ch = XM_VideoItemGetVideoChannel (hVideoItem);
		rect = rc;
		rect.top = y;
		rect.bottom = rect.top + APP_POS_ITEM5_LINEHEIGHT - 1;
		rect.left = item_ch_x;
		rect.right = rect.left + 24;
		if(ch == XM_VIDEO_CHANNEL_1)
		{
			// ��������ͷ
			AP_RomImageDrawByMenuID (AP_ID_RECYCLE_ICON_CHANNEL_2_16, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		}
		y = (XMCOORD)(y + APP_POS_ITEM5_LINEHEIGHT);
	}
    #else //��΢ͼ��ʾ
	//��ʾѡ���
	diff_val = videoBackListViewListData->nCurItem - videoBackListViewListData->nTopItem;
	#if 1
	if(videoBackListViewListData->nCurItem!=-1 && videoBackListViewListData->nItemCount >0)
	{   
		if(diff_val == 0)
		{
			rect.left = Thumb_Hor_Interval-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y);
		}
		else if(diff_val == 1)
		{
		    rect.left = Thumb_Hor_Interval+Thumb_Hor_Interval+Thumb_width-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y);
		}
		else if(diff_val == 2)
		{
			rect.left = Thumb_Hor_Interval+Thumb_Hor_Interval+Thumb_width+Thumb_Hor_Interval+Thumb_width-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y);
		}
		if(diff_val == 3)
		{
			rect.left = Thumb_Hor_Interval-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + Thumb_height+Thumb_Vert_Interval+50);
		}
		else if(diff_val == 4)
		{
		    rect.left = Thumb_Hor_Interval+Thumb_Hor_Interval+Thumb_width-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + Thumb_height+Thumb_Vert_Interval+50);
		}
		else if(diff_val == 5)
		{
			rect.left = Thumb_Hor_Interval+Thumb_Hor_Interval+Thumb_width+Thumb_Hor_Interval+Thumb_width-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + Thumb_height+Thumb_Vert_Interval+50);
		}
		rect.right = rect.left+Thumb_width+8+10;
		rect.bottom = rect.top +Thumb_height+8+10;
		XM_FillRect(hWnd,rect.left,rect.top,rect.right,rect.bottom,XM_RGB(255,0,0));
	}
	#endif
	#if 0
	// ��ʾѡ�����
	if(videoListViewListData->nCurItem!=-1)
	{
		rect = rc;
		rect.left = 0;
		rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (videoListViewListData->nCurItem - videoListViewListData->nTopItem) * APP_POS_ITEM5_LINEHEIGHT);
		AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMBACKGROUND, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	}
	#endif
	
	// ��ʾĿ¼��
	x = Thumb_Hor_Interval;
	y = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 10);
		
	for(i = 0; i < 6; i++)
	{
		HANDLE hVideoItem;
		XMVIDEOITEM *pVideoItem;
		int ch;
		
	    item = videoBackListViewListData->nTopItem + i;
		if(item >= videoBackListViewListData->nItemCount)
			break;
		if(videoBackListViewListData->nItemCount <= 0)
			break;

		//��ȡ��Ƶ���ļ���
		hVideoItem = AP_VideoItemGetVideoItemHandleEx(videoBackListViewListData->mode, item,XM_VIDEO_CHANNEL_1);
		if(hVideoItem == NULL)
			break;
		
		pVideoItem = AP_VideoItemGetVideoItemFromHandle(hVideoItem);
		if(pVideoItem == NULL)
			break;

		// ��ƵԤ��ͼƬ��ȡ
		if(XM_VideoItemGetVideoFilePath (pVideoItem, XM_VIDEO_CHANNEL_1, text, sizeof(text)))
		{
			unsigned int w = Thumb_width;
			unsigned int h = Thumb_height;
			unsigned int yuvstride = Thumb_width;
			unsigned int imagestride;
			unsigned char *thumb_image;
			XM_IMAGE *lpImage = NULL;

			imagestride = w;
			imagestride *= 4;
			lpImage = (XM_IMAGE *)kernel_malloc (imagestride * h + sizeof(XM_IMAGE) - 4);
			thumb_image = (unsigned char *)kernel_malloc (yuvstride * h * 3/2);
			
            if(thumb_image)
			{
				unsigned char *image[3];
				image[0] = thumb_image;
				image[1] = thumb_image+yuvstride * h;
				image[2] = 0;
				if(h264_decode_avi_stream_thumb_frame (text, w, h, yuvstride, image) == 0)
				{
					XM_ImageConvert_Y_UV420_to_ARGB(thumb_image,(unsigned char *)lpImage->image,w,h);
					lpImage->id = XM_IMAGE_ID;
					lpImage->format = XM_OSD_LAYER_FORMAT_ARGB888;
					lpImage->stride = (unsigned short)imagestride;
					lpImage->width = w;
					lpImage->height = h;
					lpImage->ayuv = NULL;

					XM_ImageDisplay (lpImage, hWnd, x, y);

					// ������ԣ�����ֻ�����ԣ�����������Ƶ��Ӧ�����������
					#if 0
					if(pVideoItem->video_lock)
					{
						rect = rc;
						rect.top = y+12;
						rect.bottom = rect.top + 36;
						rect.left = x+10;
						rect.right = rect.left + 38;
						AP_RomImageDrawByMenuID (AP_ID_COMMON_KEY, hWnd, &rect, XMGIF_DRAW_POS_CENTER);

						//��ȡ¼��ͨ��
						ch = XM_VideoItemGetVideoChannel (hVideoItem);
						rect = rc;
						rect.top = y+10;
						rect.bottom = rect.top + 36;
						rect.left = x+48;
						rect.right = rect.left + 50;
						
						if(ch == XM_VIDEO_CHANNEL_0)
						{
							// ǰ������ͷ
							AP_RomImageDrawByMenuID (AP_ID_RECYCLE_ICON_CHANNEL_1_16, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
						}
						else if(ch == XM_VIDEO_CHANNEL_1)
						{
							// ��������ͷ
							AP_RomImageDrawByMenuID (AP_ID_RECYCLE_ICON_CHANNEL_2_16, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
						}
					}
					else
					{
						//��ȡ¼��ͨ��
						ch = XM_VideoItemGetVideoChannel (hVideoItem);
						rect = rc;
						rect.top = y+10;
						rect.bottom = rect.top + 36;
						rect.left = x+10;
						rect.right = rect.left + 50;
						
						if(ch == XM_VIDEO_CHANNEL_0)
						{
							// ǰ������ͷ
							AP_RomImageDrawByMenuID (AP_ID_RECYCLE_ICON_CHANNEL_1_16, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
						}
						else if(ch == XM_VIDEO_CHANNEL_1)
						{
							// ��������ͷ
							AP_RomImageDrawByMenuID (AP_ID_RECYCLE_ICON_CHANNEL_2_16, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
						}
					}
					#endif
					//��ʾʱ��
					XM_GetDateTimeFromFileName ((char *)(pVideoItem->video_name), &filetime);
					sprintf (text, "%04d/%02d/%02d %02d:%02d:%02d", filetime.wYear, 
																		filetime.bMonth,
																		filetime.bDay,
																		filetime.bHour,
																		filetime.bMinute,
																		filetime.bSecond);
					AP_TextGetStringSize (text, strlen(text), &size);
					AP_TextOutDataTimeString (hWnd, x, y + Thumb_height +10, text, strlen(text));
					
				}
				kernel_free (thumb_image);
				kernel_free (lpImage);
			}
		}
		x = (XMCOORD)(x + Thumb_width + Thumb_Hor_Interval);		
		if(i==2)
		{
		   	x=Thumb_Hor_Interval;
			y=(XMCOORD)(y + Thumb_height+Thumb_Vert_Interval+50);
		}
	}
	#endif
	
	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
	AP_ButtonControlMessageHandler(&videoBackListViewListData->btnControl, msg);
	XM_SetWindowAlpha(hWnd, (unsigned char)old_alpha);
}


VOID VideoBackListViewOnKeyDown (XMMSG *msg)
{
	VIDEOBACKLISTVIEWLISTDATA *videoBackListViewListData = (VIDEOBACKLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackListView));
	if(videoBackListViewListData == NULL)
		return;

	XM_printf(">>>>>>>>>>>>>>>>>>>>VideoBackListViewOnKeyDown, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
	
	// ������
	XM_Beep (XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
		#if 1
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
		//case VK_AP_SWITCH:	// �����л���
		//case VK_AP_POWER:		// ��Դ��
			// ��"¼�����"�����У�MENU��Power��Switch��MODE��������Ϊ��ť������
			// �˴������ĸ����¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&videoBackListViewListData->btnControl, msg);
			break;
		#endif
		
		case VK_AP_UP:		
			if(videoBackListViewListData->nItemCount <= 0)
				return;

			AP_ButtonControlMessageHandler (&videoBackListViewListData->btnControl, msg);
			videoBackListViewListData->nCurItem --;
			if(videoBackListViewListData->nCurItem < 0)
			{
				// �۽������һ��
				videoBackListViewListData->nCurItem = (WORD)(videoBackListViewListData->nItemCount - 1);
				videoBackListViewListData->nTopItem = 0;
				while ( (videoBackListViewListData->nCurItem - videoBackListViewListData->nTopItem) >= videoBackListViewListData->nVisualCount )
				{
					videoBackListViewListData->nTopItem ++;
				}
			}
			else
			{
				if(videoBackListViewListData->nTopItem > videoBackListViewListData->nCurItem)
					videoBackListViewListData->nTopItem = videoBackListViewListData->nCurItem;
			}
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_DOWN:	// 
			if(videoBackListViewListData->nItemCount <= 0)
				return;

			AP_ButtonControlMessageHandler (&videoBackListViewListData->btnControl, msg);
			videoBackListViewListData->nCurItem ++;	
			if(videoBackListViewListData->nCurItem >= videoBackListViewListData->nItemCount)
			{
				// �۽�����һ��
				videoBackListViewListData->nTopItem = 0;
				videoBackListViewListData->nCurItem = 0;
			}
			else
			{
				while ( (videoBackListViewListData->nCurItem - videoBackListViewListData->nTopItem) >= videoBackListViewListData->nVisualCount )
				{
					videoBackListViewListData->nTopItem ++;
				}
			}

			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;
			
		case VK_AP_SWITCH:
			// ѭ��¼�񡢱���¼���л�
			//SwitchMode (videoListViewListData);
			if(videoBackListViewListData->mode == VIDEOLIST_ALL_VIDEO_MODE)
			{
				XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				//XM_JumpWindowEx (XMHWND_HANDLE(AlbumListView), 0, 0);//VideoBackListView
				XM_JumpWindowEx(XMHWND_HANDLE(VideoListView), 0, 0);
			}
			else
			{
				// ����¼��ģʽ --> ����վ
				XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				XM_JumpWindowEx (XMHWND_HANDLE(RecycleView), 0, 0);		// �л�������¼�����ģʽ

			/*
			// ����¼��ģʽ --> ȫ��¼��ģʽ
				XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				XM_JumpWindowEx (XMHWND_HANDLE(VideoListView), VIDEOLIST_ALL_VIDEO_MODE, 0);		// �л�������¼�����ģʽ
				videoListViewListData->mode = VIDEOLIST_ALL_VIDEO_MODE;
				SwitchMode (videoListViewListData);
			*/
			}

			/*
			// ���Դ���
			{
				XMMSG key;
				key.wp = VK_AP_UP;
				VideoListViewOnKeyDown (&key);
			}
			*/
			break;

		default:
			break;
	}

}

VOID VideoBackListViewOnKeyUp (XMMSG *msg)
{
	VIDEOBACKLISTVIEWLISTDATA *videoBackListViewListData;
	videoBackListViewListData = (VIDEOBACKLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackListView));
	if(videoBackListViewListData == NULL)
		return;

	XM_printf(">>>>>>>>>>>>>>>>>>>>VideoBackListViewOnKeyUp, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
      
	switch(msg->wp)
	{
		case VK_AP_MENU:	// �˵���
	    case VK_AP_MODE:	// �л������г���¼��״̬
		//case VK_AP_POWER:
	       
			// ��"ʱ������"�����У�MENU��MODE��������Ϊ��ť������
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&videoBackListViewListData->btnControl, msg);
			break;
			
		case VK_AP_SWITCH:
			XM_PostMessage (XM_COMMAND, VIDEOBACKLISTVIEWLIST_COMMAND_PLAY, videoBackListViewListData->nCurItem);//settingViewData->btnControl.pBtnInfo[0].wCommand
			break;
         
		default:
			AP_ButtonControlMessageHandler (&videoBackListViewListData->btnControl, msg);
			break;
	}
}

VOID VideoBackListViewOnCommand (XMMSG *msg)
{
	VIDEOBACKLISTVIEWLISTDATA *videoBackListViewListData;
	videoBackListViewListData = (VIDEOBACKLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackListView));
	if(videoBackListViewListData == NULL)
		return;

	XM_printf(">>>>>>>>>>>>>>>>>>>>VideoBackListViewOnCommand, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
    
	switch (msg->wp)
	{
		case VIDEOBACKLISTVIEWLIST_COMMAND_RETURN:
			// ���ص�����
			XM_PullWindow (0);
			break;

		case VIDEOBACKLISTVIEWLIST_COMMAND_SWITCH:
			// ѭ��¼�񡢱���¼���л�
			//SwitchMode (videoListViewListData);
			if(videoBackListViewListData->mode == VIDEOLIST_ALL_VIDEO_MODE)
			{
				XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				XM_JumpWindowEx (XMHWND_HANDLE(AlbumListView), 0, 0);
			}
			else
			{
				// ����¼��ģʽ --> ����վ
				XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				XM_JumpWindowEx (XMHWND_HANDLE(RecycleView), 0, 0);		// �л�������¼�����ģʽ

			/*
			// ����¼��ģʽ --> ȫ��¼��ģʽ
				XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				XM_JumpWindowEx (XMHWND_HANDLE(VideoListView), VIDEOLIST_ALL_VIDEO_MODE, 0);		// �л�������¼�����ģʽ
				videoListViewListData->mode = VIDEOLIST_ALL_VIDEO_MODE;
				SwitchMode (videoListViewListData);
			*/
			}

			/*
			// ���Դ���
			{
				XMMSG key;
				key.wp = VK_AP_UP;
				VideoListViewOnKeyDown (&key);
			}
			*/
			break;
		

		case VIDEOBACKLISTVIEWLIST_COMMAND_PLAY:
			if(videoBackListViewListData->nItemCount > 0)
			{	
				// ����
				AP_OpenVideoBackView (videoBackListViewListData->mode, (WORD)videoBackListViewListData->nCurItem);     
			}
			break;
			
         case VIDEOBACKLISTVIEWLIST_COMMAND_UP:
			 if(videoBackListViewListData->nItemCount <= 0)
				return;

			AP_ButtonControlMessageHandler (&videoBackListViewListData->btnControl, msg);
			videoBackListViewListData->nCurItem --;
			if(videoBackListViewListData->nCurItem < 0)
			{
				// �۽������һ��
				videoBackListViewListData->nCurItem = (WORD)(videoBackListViewListData->nItemCount - 1);
				videoBackListViewListData->nTopItem = 0;
				while ( (videoBackListViewListData->nCurItem - videoBackListViewListData->nTopItem) >= videoBackListViewListData->nVisualCount )
				{
					videoBackListViewListData->nTopItem ++;
				}
			}
			else
			{
				if(videoBackListViewListData->nTopItem > videoBackListViewListData->nCurItem)
					videoBackListViewListData->nTopItem = videoBackListViewListData->nCurItem;
			}
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;
			
		case VIDEOBACKLISTVIEWLIST_COMMAND_DOWN:
			if(videoBackListViewListData->nItemCount <= 0)
				return;

			AP_ButtonControlMessageHandler (&videoBackListViewListData->btnControl, msg);
			videoBackListViewListData->nCurItem ++;	
			if(videoBackListViewListData->nCurItem >= videoBackListViewListData->nItemCount)
			{
				// �۽�����һ��
				videoBackListViewListData->nTopItem = 0;
				videoBackListViewListData->nCurItem = 0;
			}
			else
			{
				while ( (videoBackListViewListData->nCurItem - videoBackListViewListData->nTopItem) >= videoBackListViewListData->nVisualCount )
				{
					videoBackListViewListData->nTopItem ++;
				}
			}

			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;
			
		default:
			break;
	}
}

VOID VideoBackListViewOnTimer (XMMSG *msg)
{
	// ��⿨״̬
	if(APSYS_CardChecking() == APP_SYSTEMCHECK_CARD_NOCARD)
	{
		// ���γ���ֱ�������ص�����(���渺�𿨲�ε����д���), �˴�ǿ���������³�ʼ��
		XM_JumpWindow (XMHWND_HANDLE(Desktop));
		return;
	}
}

static VOID VideoBackListViewOnSystemEvent (XMMSG *msg)
{
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:			// SD���γ��¼�
			XM_BreakSystemEventDefaultProcess (msg);		// ������ȱʡ����
			// ������Ϣ���²��뵽��Ϣ���ж���
			XM_InsertMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
			XM_JumpWindowEx (XMHWND_HANDLE(Desktop), HWND_CUSTOM_DEFAULT, XM_JUMP_POPDEFAULT);
			//XM_JumpWindow (XMHWND_HANDLE(Desktop));// ��ת������
			//XM_PullWindow (0);					// ������ǰ���б��Ӵ�
			break;
	}
}
VOID VideoBackListViewOnTouchDown (XMMSG *msg)
{
	int index;
	VIDEOBACKLISTVIEWLISTDATA *videoBackListViewListData = (VIDEOBACKLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackListView));
	if(videoBackListViewListData == NULL)
		return;
	
	if(AP_ButtonControlMessageHandler (&videoBackListViewListData->btnControl, msg))
		return;
    
	//index = AppLocateItem (XMHWND_HANDLE(VideoBackListView), videoBackListViewListData->nVisualCount, APP_POS_ITEM5_LINEHEIGHT, videoBackListViewListData->nTopItem, LOWORD(msg->lp), HIWORD(msg->lp));
	index = AppLocateItem1 (XMHWND_HANDLE(VideoBackListView), videoBackListViewListData->nVisualCount, APP_POS_ITEM5_LINEHEIGHT, videoBackListViewListData->nTopItem, LOWORD(msg->lp), HIWORD(msg->lp));
    
	if(index < 0)
		return;
	
	if(videoBackListViewListData->nCurItem != index)
	{
		videoBackListViewListData->nCurItem = index;
	
		XM_InvalidateWindow ();
		XM_UpdateWindow ();
	}
}

VOID VideoBackListViewOnTouchUp (XMMSG *msg)
{
       int index;
	VIDEOBACKLISTVIEWLISTDATA *videoBackListViewListData;
	videoBackListViewListData = (VIDEOBACKLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoBackListView));
	if(videoBackListViewListData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&videoBackListViewListData->btnControl, msg))
		return;
	
	//index = AppLocateItem (XMHWND_HANDLE(VideoBackListView), videoBackListViewListData->nVisualCount, APP_POS_ITEM5_LINEHEIGHT, videoBackListViewListData->nTopItem, LOWORD(msg->lp), HIWORD(msg->lp));
	index = AppLocateItem1 (XMHWND_HANDLE(VideoBackListView), videoBackListViewListData->nVisualCount, APP_POS_ITEM5_LINEHEIGHT, videoBackListViewListData->nTopItem, LOWORD(msg->lp), HIWORD(msg->lp));

	if(index < 0)
		return;
	
	videoBackListViewListData->nCurItem = index;

	XM_PostMessage (XM_COMMAND, VIDEOBACKLISTVIEWLIST_COMMAND_PLAY, videoBackListViewListData->nCurItem);//settingViewData->btnControl.pBtnInfo[0].wCommand  
}

// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (VideoBackListView)
	XM_ON_MESSAGE (XM_PAINT, VideoBackListViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, VideoBackListViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, VideoBackListViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, VideoBackListViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, VideoBackListViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, VideoBackListViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, VideoBackListViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, VideoBackListViewOnTouchUp)
	XM_ON_MESSAGE (XM_TIMER, VideoBackListViewOnTimer)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, VideoBackListViewOnSystemEvent)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif

XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, VideoBackListView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
