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


extern int XMSYS_FileManagerFileDelete (unsigned char channel, unsigned char type, unsigned char file_name[8]);

#define Thumb_width 304
#define Thumb_height 208
#define Thumb_Hor_Interval 28
#define Thumb_Vert_Interval 28

static const int file_error_offset[] = {ROM_T18_LANG_CHN_VIDEO_FILE_ERROR_PNG};
static const int file_error_length[] = {ROM_T18_LANG_CHN_VIDEO_FILE_ERROR_PNG_SIZE};
static XM_IMAGE *file_error_image[1];
static u8_t file_error_flag[6] = {0,0,0,0,0,0};//�ļ��𻵱�־
static u8_t video_page_setting_flag = FALSE;
static u8_t first_enter = FALSE;

typedef struct tagVIDEOLISTVIEWLISTDATA {
	int					nPageNum;					// ��ǰpage
	int					nCurItem;					// ��ǰѡ��Ĳ˵���,0-5

	int					nItemCount;					// �˵������,����
	int					nVisualCount;				// �Ӵ�����ʾ����Ŀ����
	int					nTotalPage;					// ��page;
	
	BYTE				videotype;					//����,��Ƶ����ͼƬ
	
    HANDLE				hMoniorSwitchControl;		//ͣ�����
	char				fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	XMFILEFIND			fileFind;
} VIDEOLISTVIEWLISTDATA;


void set_video_page_switch_flag(u8_t flg)
{
	video_page_setting_flag = flg;
}


// ����¼���б��Ӵ��ľ۽���
XMBOOL AP_VideoListViewSetFocusItem (UINT uFocusItem)
{
	VIDEOLISTVIEWLISTDATA *videoListViewListData;
	videoListViewListData = (VIDEOLISTVIEWLISTDATA *)XM_GetWindowPrivateData(XMHWND_HANDLE(VideoListView));
	if(videoListViewListData)
	{
		videoListViewListData->nCurItem = (SHORT)uFocusItem;
		return 1;
	}
	return 0;
}

void hw_backlight_set_auto_off_ticket (unsigned int ticket);
static VOID VideoListViewOnEnter(XMMSG *msg)
{
	unsigned int replay_ch;
	VIDEOLISTVIEWLISTDATA *videoListViewListData;
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>VideoListViewOnEnter, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

	if(msg->wp == 0)
	{
		XMRECT rc;
		int nVisualCount;
		// ����δ��������һ�ν���
		BYTE mode = VIDEOLIST_ALL_VIDEO_MODE;

		// ����˽�����ݾ��
		videoListViewListData = XM_calloc(sizeof(VIDEOLISTVIEWLISTDATA));
		if(videoListViewListData == NULL)
		{
			XM_printf("videoListViewListData XM_calloc failed\n");
			// ʧ�ܷ��ص������ߴ���
			XM_PullWindow (0);
			return;
		}

		XMSYS_H264CodecRecorderStop();//�ر�����ͨ��
		
		videoListViewListData->videotype = VIDEOITEM_CIRCULAR_VIDEO_MODE;//ѭ��¼��ģʽ
		
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData(XMHWND_HANDLE(VideoListView), videoListViewListData);

		//��ʼɨ�����з�����������Ƶ�ļ�
		videoListViewListData->nPageNum = get_video_curpage_num();
		videoListViewListData->nCurItem = 0;
		
		//����ͨ��,ʵʱ��ȡ����Ƶ�ļ�������������page����
		if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
		{
			replay_ch = XM_VIDEO_CHANNEL_0;
		}
		else
		{
			replay_ch = XM_VIDEO_CHANNEL_1;
		}
		videoListViewListData->nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_VIDEO,XM_VIDEOITEM_CLASS_BITMASK_ALL);

		videoListViewListData->nTotalPage = videoListViewListData->nItemCount/6;
		
		if(videoListViewListData->nItemCount%6!=0)
		{
			videoListViewListData->nTotalPage += 1;
		}
		XM_printf(">>>>>>>>VideoListViewOnEnter,videoListViewListData->nPageNum:%d\r\n", videoListViewListData->nPageNum);
		XM_printf(">>>>>>>>VideoListViewOnEnter,videoListViewListData->nTotalPage:%d\r\n", videoListViewListData->nTotalPage);
		//���������ʾ����Ŀ����
		XM_GetDesktopRect(&rc);
		nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
		nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
        nVisualCount -= 1;

		if(videoListViewListData->nItemCount>=nVisualCount)
		{
			videoListViewListData->nVisualCount = (SHORT)nVisualCount;
		}
		else
		{
            videoListViewListData->nVisualCount=videoListViewListData->nItemCount;
		}
		file_error_image[0] = XM_RomImageCreate(file_error_offset[0], file_error_length[0], XM_OSD_LAYER_FORMAT_ARGB888);
	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�		
		videoListViewListData = (VIDEOLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoListView));
		if(videoListViewListData == NULL)
			return;

		//�л�pageĬ��ѡ�е�һ��
		videoListViewListData->nCurItem = 0;
		XM_printf(">>>>videoListViewListData->nPageNum:%d\r\n", videoListViewListData->nPageNum);

		XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>VideoListView Pull.............\r\n");
	}
	// ������ʱ�������ڿ��γ����
	// ����0.5��Ķ�ʱ��
	hw_backlight_set_auto_off_ticket(0xFFFFFFFF);//�ر��������ڶԷ�ģʽ�ٴ�
	XM_SetTimer(XMTIMER_VIDEOLISTVIEW, 500);
}

VOID VideoListViewOnLeave (XMMSG *msg)
{
	// ɾ����ʱ��
	XM_printf(">>>>>>VideoListViewOnLeave %d......\r\n", msg->wp);
	XM_KillTimer(XMTIMER_VIDEOLISTVIEW);

	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		VIDEOLISTVIEWLISTDATA *videoListViewListData = (VIDEOLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoListView));
		// �ͷ����з������Դ
		if(videoListViewListData)
		{
			// �ͷ�˽�����ݾ��
			XM_free (videoListViewListData);
			// �����Ӵ���˽�����ݾ��Ϊ��
			XM_SetWindowPrivateData(XMHWND_HANDLE(VideoListView), NULL);
		}
		
		if(file_error_image[0])
		{
			XM_ImageDelete(file_error_image[0]);
			file_error_image[0] = NULL;
		}

		XMSYS_H264CodecRecorderStart();
		XM_printf(">>>>>>VideoListView Exit......\r\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf(">>>>>>VideoListView Push......\r\n");
	}
}

VOID VideoListViewOnPaint(XMMSG *msg)
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
	char String[32];
	XMSIZE size;
	XMSYSTEMTIME filetime;
	unsigned int replay_ch;
	DWORD MenuID;

	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>VideoListViewOnPaint......\r\n");
	VIDEOLISTVIEWLISTDATA *videoListViewListData = (VIDEOLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoListView));
	if(videoListViewListData == NULL)
		return;
	
	hWnd = XMHWND_HANDLE(VideoListView);
	XM_GetDesktopRect (&rc);

	#if 0
	if(videoListViewListData->nItemCount <= 0)
	{
		XM_PullWindow(0);
	}
	#endif
	
	XM_FillRect(hWnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));
	old_alpha = XM_GetWindowAlpha (hWnd);
	XM_SetWindowAlpha(hWnd, 255);

	//����ͨ��,ʵʱ��ȡ����Ƶ�ļ�������������page����
	if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
	{
		MenuID = AP_ID_VIDEO_BUTTON_CH1;
		replay_ch = XM_VIDEO_CHANNEL_0;
	}
	else
	{
		MenuID = AP_ID_VIDEO_BUTTON_CH2;
		replay_ch = XM_VIDEO_CHANNEL_1;
	}
	videoListViewListData->nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_VIDEO,XM_VIDEOITEM_CLASS_BITMASK_ALL);

	videoListViewListData->nTotalPage = videoListViewListData->nItemCount/6;
	if(videoListViewListData->nItemCount%6!=0)
	{
		videoListViewListData->nTotalPage += 1;
	}		
	XM_printf(">>>>>>>>VideoListViewOnPaint,videoListViewListData->nItemCount:%d\r\n", videoListViewListData->nItemCount);
	XM_printf(">>>>>>>>VideoListViewOnPaint,videoListViewListData->nTotalPage:%d\r\n", videoListViewListData->nTotalPage);
	
	// --------------------------------------
	//
	// ********* 1 ��ʾ����������Ϣ *********
	//
	// --------------------------------------
	rect.left = 0;rect.top = 0;rect.right = 1024;rect.bottom = 42;
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUTITLEBARBACKGROUND, hWnd, &rc, XMGIF_DRAW_POS_LEFTTOP);

	rect.left = 20;rect.top = 3;rect.right = 65;rect.bottom = 36;
	AP_RomImageDrawByMenuID(AP_ID_VIDEO_TITLE_VIDEO, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//����:��Ƶ/ͼƬ
	
	rect.left = 450;rect.top = 3;rect.right = 65;rect.bottom = 36;
	AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_CH, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//ͨ��
	rect.left = 515;rect.top = 3;rect.right = 65;rect.bottom = 36;
	AP_RomImageDrawByMenuID(MenuID, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//CH1/CH2

	rect.left = 930;rect.top = 3;rect.right = 65;rect.bottom = 36;
    sprintf(String,"%d%s%d", videoListViewListData->nPageNum, "/", videoListViewListData->nTotalPage);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString(hWnd, rect.left, rect.top, String, strlen(String));

	// ��5����ʾ��Ƶ�б�
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
	
	//��ʾѡ���,���ݵ�ǰnCurItem����ѡ�п�,�൱�ھ۽�λ��
	diff_val = videoListViewListData->nCurItem;
	if(videoListViewListData->nCurItem!=-1 && videoListViewListData->nItemCount >0)
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
		XM_FillRect(hWnd,rect.left,rect.top,rect.right,rect.bottom,XM_RGB(0,255,0));
	}
	
	// ��ʾĿ¼��
	x = Thumb_Hor_Interval;
	y = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 10);

	u8_t startitem, maxitem;

	if(first_enter==FALSE)
	{
		first_enter = TRUE;
		startitem = 0;
		maxitem = 6;
	}
	else
	{
		if(videoListViewListData->nCurItem<=2)
		{
			startitem = 0;
			maxitem = 3;
		}
		else
		{
			startitem = 3;
			maxitem = 6;
		}
	}

	startitem = 0;
	maxitem = 6;
	//����pagenum��ʾpage
	for(i = startitem; i < maxitem; i++)
	{
		HANDLE hVideoItem;
		XMVIDEOITEM *pVideoItem;
		int ch;

	    item = (videoListViewListData->nPageNum-1)*6 + i;

		XM_printf(">>>>>item:%d\r\n", item);
		if(item >= videoListViewListData->nItemCount)
			break;
		
		if(videoListViewListData->nItemCount <= 0)
			break;

		//��ȡ��Ƶ���ļ���
		unsigned int replay_ch;
		if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
		{
			replay_ch = XM_VIDEO_CHANNEL_0;
		}
		else
		{
			replay_ch = XM_VIDEO_CHANNEL_1;
		}
		hVideoItem = XM_VideoItemGetVideoItemHandleEx(0,replay_ch,XM_FILE_TYPE_VIDEO,XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL,item,1);

		//hVideoItem = AP_VideoItemGetVideoItemHandleEx(videoListViewListData->videotype, item, replay_ch);
		if(hVideoItem == NULL)
		{
			XM_printf(">>>>error2 ..............\r\n");
			break;
		}
		
		pVideoItem = AP_VideoItemGetVideoItemFromHandle(hVideoItem);
		if(pVideoItem == NULL)
		{
			XM_printf(">>>>error1 ..............\r\n");
			break;
		}
		
		// ��ƵԤ��ͼƬ��ȡ
		if(XM_VideoItemGetVideoFilePath(pVideoItem, replay_ch, text, sizeof(text)))
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
				if(h264_decode_avi_stream_thumb_frame(text, w, h, yuvstride, image) == 0)
				{
					XM_printf(">>>>>file ok............\r\n");
					XM_ImageConvert_Y_UV420_to_ARGB(thumb_image,(unsigned char *)lpImage->image,w,h);
					lpImage->id = XM_IMAGE_ID;
					lpImage->format = XM_OSD_LAYER_FORMAT_ARGB888;
					lpImage->stride = (unsigned short)imagestride;
					lpImage->width = w;
					lpImage->height = h;
					lpImage->ayuv = NULL;

					XM_ImageDisplay(lpImage, hWnd, x, y);
				}
				else
				{
					file_error_flag[i] = 1;
					XM_ImageDisplay(file_error_image[0], hWnd, x, y);
					XM_printf(">>>>>file error............\r\n");
				}	
				//��ʾʱ��
				XM_GetDateTimeFromFileName ((char *)pVideoItem->video_name, &filetime);
				sprintf (text, "%04d/%02d/%02d %02d:%02d:%02d", filetime.wYear, 
																filetime.bMonth,
																filetime.bDay,
																filetime.bHour,
																filetime.bMinute,
																filetime.bSecond);

				AP_TextGetStringSize (text, strlen(text), &size);
				AP_TextOutDataTimeString (hWnd, x, y + Thumb_height +10, text, strlen(text));

				kernel_free (thumb_image);
				kernel_free (lpImage);
			}
		}
		else
		{
			XM_printf(">>>>error ..............\r\n");
		}

		
		x = (XMCOORD)(x + Thumb_width + Thumb_Hor_Interval);		
		if(i==2)
		{
		   	x=Thumb_Hor_Interval;
			y=(XMCOORD)(y + Thumb_height+Thumb_Vert_Interval+50);
		}
	}

	XM_SetWindowAlpha (hWnd, (unsigned char)old_alpha);
}

VOID VideoListViewOnKeyDown (XMMSG *msg)
{
	VIDEOLISTVIEWLISTDATA *videoListViewListData = (VIDEOLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoListView));
	if(videoListViewListData == NULL)
		return;
	
	XM_printf(">>>>>>>>VideoListViewOnKeyDown, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	// ������
	XM_Beep(XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
	    case REMOTE_KEY_MENU:
		case VK_AP_MENU://MENU��,����ط�ģʽ�˵�
			XM_printf(">>>>>>>VK_AP_MENU\r\n");
            XM_PushWindowEx(XMHWND_HANDLE(PlayBackMenu),(DWORD)videoListViewListData);//�طŲ˵�,ѹջ��ʽ���ݻ�δ�ͷ�
			break;
        case REMOTE_KEY_DOWN:
		case VK_AP_SWITCH://OK��
			XM_printf(">>>>>>>VK_AP_MODE\r\n");
			if(!file_error_flag[videoListViewListData->nCurItem])
			{
				if(videoListViewListData->nItemCount > 0)
				{	
					// ����
					AP_OpenVideoView(videoListViewListData->videotype, (WORD)((videoListViewListData->nPageNum-1)*6+videoListViewListData->nCurItem));
				}
			}
			break;
		case REMOTE_KEY_LEFT:
		case VK_AP_DOWN://-��		
			XM_printf(">>>>>>>VK_AP_DOWN\r\n");
			if(videoListViewListData->nItemCount <= 0)
				return;

			//ÿҳ6����Ƶ�ļ�,ͨ��item����,�ı�page
			if(videoListViewListData->nCurItem==0)
			{
				if(videoListViewListData->nPageNum==1)
				{
					videoListViewListData->nPageNum = videoListViewListData->nTotalPage;
		
					if(videoListViewListData->nItemCount%6==0)
					{
						videoListViewListData->nCurItem = 5;
					}
					else
					{
						videoListViewListData->nCurItem = videoListViewListData->nItemCount%6-1;
					}
				}
				else
				{
					videoListViewListData->nPageNum--;
					videoListViewListData->nCurItem = 5;
				}
			}
			else
			{
				videoListViewListData->nCurItem--;
			}		
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;
		case REMOTE_KEY_RIGHT:
		case VK_AP_UP:////+��	ѡ����һ����
			XM_printf(">>>>>>>VK_AP_UP\r\n");
			if(videoListViewListData->nItemCount <= 0)
				return;

			//ÿҳ6����Ƶ�ļ�,ͨ��item����,�ı�page
			videoListViewListData->nCurItem ++;
			if(((videoListViewListData->nPageNum-1)*6+videoListViewListData->nCurItem) < videoListViewListData->nItemCount)
			{
				if(videoListViewListData->nCurItem==videoListViewListData->nVisualCount)
				{
					videoListViewListData->nCurItem = 0;
					videoListViewListData->nPageNum++;
				}
			}
			else
			{
				videoListViewListData->nPageNum = 1;
				videoListViewListData->nCurItem = 0;
			}
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case REMOTE_KEY_UP:
        case VK_AP_FONT_BACK_SWITCH://AV�����˳��ط�ģʽ
			XM_printf(">>>>>>>VK_AP_FONT_BACK_SWITCH\r\n");
			XM_JumpWindow(XMHWND_HANDLE(Desktop));
			if(videoListViewListData->nItemCount <= 0)
				return;

            break;

		default:
			break;
	}
}


VOID VideoListViewOnKeyUp (XMMSG *msg)
{
	XM_printf(">>>>>>>>VideoListViewOnKeyUp, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
}

VOID VideoListViewOnTimer (XMMSG *msg)
{
	//XM_printf(">>>>>111\r\n");
	if(get_parking_trig_status() != 0 && AP_GetMenuItem(APPMENUITEM_PARKING_LINE))
	{
	    XM_PullWindow(0);//����������ʾ��������
	}
}

static VOID VideoListViewOnSystemEvent (XMMSG *msg)
{
	XM_printf(">>>>>>>>VideoListViewOnSystemEvent, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:			// SD���γ��¼�
			XM_printf(">>>>>VideoListViewOnSystemEvent,1\r\n");
			XM_BreakSystemEventDefaultProcess (msg);		// ������ȱʡ����

			#if 0
			// ������Ϣ���²��뵽��Ϣ���ж���
			XM_InsertMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
			XM_JumpWindowEx (XMHWND_HANDLE(Desktop), HWND_CUSTOM_DEFAULT, XM_JUMP_POPDEFAULT);
			//XM_JumpWindow (XMHWND_HANDLE(Desktop));	// ��ת������
			#endif
			//XM_PullWindow(0);					   // ������ǰ���б��Ӵ�
			XM_printf(">>>>>VideoListViewOnSystemEvent,2\r\n");
			XM_JumpWindow(XMHWND_HANDLE(Desktop));
			break;

		default:
			break;
	}
}

// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (VideoListView)
	XM_ON_MESSAGE (XM_PAINT, VideoListViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, VideoListViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, VideoListViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, VideoListViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, VideoListViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, VideoListViewOnTimer)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, VideoListViewOnSystemEvent)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, VideoListView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
