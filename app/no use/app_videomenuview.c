//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_videomenuview.c
//	  ��Ƶ�˵�
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

// ���˵�ѡ����ڰ�ť�ؼ�����
#define	VIDEOMENUVIEWBTNCOUNT	2
// ˽�������
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

// �˵����
static const int commandItem[VIDEOMENUVIEW_COMMANDITEM_COUNT] = {
	AP_ID_VIDEO_LOCKVIDEO,			// ������Ƶ
	AP_ID_VIDEO_UNLOCKVIDEO,		// ������Ƶ
	AP_ID_VIDEO_PLAY				// ������Ƶ
};

typedef struct tagVIDEOMENUVIEWDATA {
	int				nTopItem;					// ��һ�����ӵĲ˵���
	int				nCurItem;					// ��ǰѡ��Ĳ˵���
	int				nItemCount;					// �˵������

	BYTE				mode;
	WORD				wVideoFileIndex;

	XMBUTTONCONTROL	btnControl;				// ��ť�ؼ�
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
			// ʧ�ܷ��ص�����
			XM_PullWindow (0);
			return;
		}

		videoMenuData->mode = (BYTE)(msg->lp >> 24);
		videoMenuData->wVideoFileIndex = (WORD)msg->lp;
		
		videoMenuData->nItemCount = VIDEOMENUVIEW_COMMANDITEM_COUNT;
		videoMenuData->nCurItem = 0;
		videoMenuData->nTopItem = 0;

		// ��ť�ؼ���ʼ��
		AP_ButtonControlInit (&videoMenuData->btnControl, VIDEOMENUVIEWBTNCOUNT, XMHWND_HANDLE(VideoMenuView), videoMenuViewBtn);

		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(VideoMenuView), videoMenuData);

	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("VideoMenuView Pull\n");
	}
	// ������ʱ�������ڿ��γ����
	// ����0.5��Ķ�ʱ��
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
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&videoMenuData->btnControl);

			// �ͷ�˽�����ݾ��
			XM_free (videoMenuData);
			
		}
		XM_printf ("VideoMenuView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
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

	// ��ʾ������
	XM_GetDesktopRect (&rc);
	XM_FillRect (XMHWND_HANDLE(VideoMenuView), rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_WINDOW));

	// --------------------------------------
	//
	// ********* 1 ��ʾ����������Ϣ *********
	//
	// --------------------------------------
	AP_DrawTitlebarControl (XMHWND_HANDLE(VideoMenuView), AP_NULLID, AP_ID_VIDEO_TITLE_COMMAND);



	// ��ʾ�˵���ˮƽ�ָ���
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMSPLITLINE);
	rect = rc;
	for (i = 0; i < VIDEOMENUVIEW_COMMANDITEM_COUNT; i++)
	{
		rect.left = APP_POS_ITEM5_SPLITLINE_X;
		rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + (i + 1) * APP_POS_ITEM5_LINEHEIGHT);
		XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(VideoMenuView), &rect, XMGIF_DRAW_POS_LEFTTOP);
	}

	// ��䵱ǰѡ�����
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMBACKGROUND);
	rect = rc;
	rect.left = 0;
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (videoMenuData->nCurItem - videoMenuData->nTopItem + 1) * APP_POS_ITEM5_LINEHEIGHT);
	XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XMHWND_HANDLE(VideoMenuView), &rect, XMGIF_DRAW_POS_LEFTTOP);

	// ��ʾ��ǰѡ�����Ƶ�ļ���
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

	// ��ʾ�˵���Ŀ 
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
	// ********* 3 ��ʾ��ť�� ***************
	//
	// --------------------------------------
	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
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
		// ���ϼ�
		case VK_AP_UP:
			AP_ButtonControlMessageHandler (&videoMenuData->btnControl, msg);
			videoMenuData->nCurItem --;
			if(videoMenuData->nCurItem < 0)
			{
				// �۽������һ��
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
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		// ���¼�
		case VK_AP_DOWN:
			AP_ButtonControlMessageHandler (&videoMenuData->btnControl, msg);
			videoMenuData->nCurItem ++;	
			if(videoMenuData->nCurItem >= videoMenuData->nItemCount)
			{
				// �۽�����һ��
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

			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_MENU:
		case VK_AP_MODE:
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
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
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
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
				// ������Ƶ
				// ����Ƿ�д����
				card_state = APSYS_CardChecking();
				if(card_state == APP_SYSTEMCHECK_SUCCESS)
				{
					// AP_VideoListLockVideoFile (videoMenuData->wVideoFileIndex);
					hVideoItem = AP_VideoItemGetVideoItemHandle (videoMenuData->mode, videoMenuData->wVideoFileIndex);
					pVideoItem = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
					AP_VideoItemLockVideoFile (pVideoItem);
					// ȷ�ϲ�����
					XM_PullWindow (NULL);
				}
				else if(card_state == APP_SYSTEMCHECK_CARD_WRITEPROTECT)
				{
					// �л�����Ϣ��ʾ����,��ʾ"��д����"
					// ��Ϣ��ʾ���ڰ������ػ�ʱ���Զ����� 
					AP_OpenMessageView (AP_ID_CARD_INFO_WRITEPROTECT);	// 3���Զ��ر�
				}
			}
			else if(videoMenuData->nCurItem == 1)
			{
				// ������Ƶ
				card_state = APSYS_CardChecking();
				if(card_state == APP_SYSTEMCHECK_SUCCESS)
				{
					// AP_VideoListUnlockVideoFile (videoMenuData->wVideoFileIndex);
					hVideoItem = AP_VideoItemGetVideoItemHandle (videoMenuData->mode, videoMenuData->wVideoFileIndex);
					pVideoItem = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
					AP_VideoItemUnlockVideoFile (pVideoItem);
					// ȷ�ϲ�����
					XM_PullWindow (NULL);
				}
				else if(card_state == APP_SYSTEMCHECK_CARD_WRITEPROTECT)
				{
					// �л�����Ϣ��ʾ����,��ʾ"��д����"
					// ��Ϣ��ʾ���ڰ������ػ�ʱ���Զ����� 
					AP_OpenMessageView (AP_ID_CARD_INFO_WRITEPROTECT);	// 3���Զ��ر�
				}
			}
			else if(videoMenuData->nCurItem == 2)
			{
				// ������Ƶ

				// ȷ�ϲ�����
				XM_PullWindow (NULL);
			}

			break;

		case VIDEOMENUVIEW_COMMAND_CANCEL:
			// ����
			// ���ص�����
			XM_PullWindow (0);
			break;
	}
}

VOID VideoMenuViewOnTimer (XMMSG *msg)
{
	// ��⿨״̬
	if(APSYS_CardChecking() == APP_SYSTEMCHECK_CARD_NOCARD)
	{
		// ���γ���ֱ�������ص�����(���渺�𿨲�ε����д���), �˴�ǿ���������³�ʼ��
		XM_JumpWindow (XMHWND_HANDLE(Desktop));
		return;
	}
}

// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (VideoMenuView)
	XM_ON_MESSAGE (XM_PAINT, VideoMenuViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, VideoMenuViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, VideoMenuViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, VideoMenuViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, VideoMenuViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, VideoMenuViewOnCommand)
	XM_ON_MESSAGE (XM_TIMER, VideoMenuViewOnTimer)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, VideoMenuView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
