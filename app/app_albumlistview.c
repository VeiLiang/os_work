//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_albumlistview.c
//	  ����б��������
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************

#include "app.h"
#include "app_videolist.h"
#include "xm_core.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"
//#include "app_album.h"
#include "system_check.h"
#include "arkn141_codec.h"
#include "xm_isp_scalar.h"
#include "xm_file.h"
#include "arkn141_isp.h"

extern int get_photo_curpage_num(void);

#define Photo_Thumb_width 304
#define Photo_Thumb_height 208
#define Photo_Thumb_Hor_Interval 28
#define Photo_Thumb_Vert_Interval 28

static int curch_photo_total_page;
static int curch_photo_page_num;//��ǰͨ��,�ڼ���item
static u8_t photo_page_setting_flag = FALSE;

static const int photo_file_error_offset[] = {ROM_T18_LANG_CHN_VIDEO_FILE_ERROR_PNG};
static const int photo_file_error_length[] = {ROM_T18_LANG_CHN_VIDEO_FILE_ERROR_PNG_SIZE};
static XM_IMAGE *photo_file_error_image[1];
static u8_t photo_file_error_flag[6] = {0,0,0,0,0,0};//�ļ��𻵱�־

typedef struct tagALBUMLISTVIEWLISTDATA {
	int					nPageNum;					// ��һ�����ӵĲ˵���
	int					nCurItem;					// ��ǰѡ��Ĳ˵���

	int					nItemCount;					// �˵������
	int					nVisualCount;				// �Ӵ�����ʾ����Ŀ����
	int					nTotalPage;					//��page;

	BYTE				videotype;

    HANDLE				hMoniorSwitchControl;		//ͣ�����
	char				fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	XMFILEFIND			fileFind;
} ALBUMLISTVIEWLISTDATA;

int get_curch_photo_page_num(void)
{
	return curch_photo_page_num;
}

void set_curch_photo_page_num(int val)
{
	curch_photo_page_num = val;
}


VOID AlbumListViewOnEnter (XMMSG *msg)
{
	ALBUMLISTVIEWLISTDATA *albumListViewListData;
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlbumListViewOnEnter, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

	if(msg->wp == 0)
	{
	  	XMRECT rc;
		int nVisualCount;
		int photo_count;
		unsigned int replay_ch;
		
		// ����δ��������һ�ν���
		// ����˽�����ݾ��
		albumListViewListData = XM_calloc (sizeof(ALBUMLISTVIEWLISTDATA));
		if(albumListViewListData == NULL)
		{
			XM_printf ("albumListViewListData XM_calloc failed\n");
			// ʧ�ܷ��ص������ߴ���
			XM_PullWindow (0);
			return;
		}

		XMSYS_H264CodecRecorderStop();//�ر�����ͨ��
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(AlbumListView), albumListViewListData);

		// ��ʼɨ�����з�����������Ƶ�ļ�
		albumListViewListData->nPageNum = get_photo_curpage_num();
		albumListViewListData->nCurItem = 0;
		albumListViewListData->videotype = XM_FILE_TYPE_PHOTO;
		if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
		{
			replay_ch = XM_VIDEO_CHANNEL_0;
		}
		else
		{
			replay_ch = XM_VIDEO_CHANNEL_1;
		}
		albumListViewListData->nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_ALL);
		albumListViewListData->nTotalPage = albumListViewListData->nItemCount/6;

		if(albumListViewListData->nItemCount%6!=0)
		{
			albumListViewListData->nTotalPage += 1;
		}
		XM_printf(">>>>>albumListViewListData->nItemCount:%d\r\n", albumListViewListData->nItemCount);
		XM_printf(">>>>>albumListViewListData->nPageNum:%d\r\n", albumListViewListData->nPageNum);
		
		// ���������ʾ����Ŀ����
		XM_GetDesktopRect(&rc);
		nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
		nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
        nVisualCount -= 1;

		if(albumListViewListData->nItemCount>=nVisualCount)
		{
			albumListViewListData->nVisualCount = (SHORT)nVisualCount;
		}
		else
		{
            albumListViewListData->nVisualCount = albumListViewListData->nItemCount;
		}
		photo_file_error_image[0] = XM_RomImageCreate(photo_file_error_offset[0], photo_file_error_length[0], XM_OSD_LAYER_FORMAT_ARGB888);
	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�		
		albumListViewListData = (ALBUMLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumListView));
		if(albumListViewListData == NULL)
			return;
		
		albumListViewListData->nCurItem = 0;
		XM_printf(">>>>>albumListViewListData->nPageNum:%d\r\n", albumListViewListData->nPageNum);

		XM_printf ("AlbumListView Pull\n");
	}
	// ������ʱ�������ڿ��γ����
	// ����0.5��Ķ�ʱ��
	//XM_SetTimer (XMTIMER_ALBUMLISTVIEW, 500);
}

VOID AlbumListViewOnLeave (XMMSG *msg)
{
	// ɾ����ʱ��
	XM_printf ("AlbumListViewOnLeave %d\n", msg->wp);
	XM_KillTimer (XMTIMER_ALBUMLISTVIEW);

	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		ALBUMLISTVIEWLISTDATA *albumListViewListData = (ALBUMLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumListView));
		// �ͷ����з������Դ
		if(albumListViewListData)
		{
			// �ͷ�˽�����ݾ��
			XM_free (albumListViewListData);
		}
		// ���ô��ڵ�˽�����ݾ��Ϊ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(AlbumListView), NULL);
		XMSYS_H264CodecRecorderStart();
		XM_printf ("AlbumListView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("AlbumListView Push\n");
	}
}

extern int zoom_decode_jpeg_file (const char *file_name,unsigned char *zoombuffer,unsigned int w,unsigned int h);

VOID AlbumListViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
	int i, item;
	XMCOORD x, y;
	unsigned int old_alpha;
	HANDLE hWnd;
	int photo_count;
	int nVisualCount;
	int diff_val;
	char text[64];
	char String[32];
	XMSIZE size;
	XMSYSTEMTIME filetime;
	unsigned int replay_ch;
	DWORD MenuID;

	ALBUMLISTVIEWLISTDATA *albumListViewListData = (ALBUMLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumListView));
	if(albumListViewListData == NULL)
		return;

	hWnd = XMHWND_HANDLE(AlbumListView);

	XM_GetDesktopRect (&rc);
	XM_FillRect (hWnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));
	old_alpha =  XM_GetWindowAlpha (hWnd);
	XM_SetWindowAlpha (hWnd, 255);
	
    //��ʾ��ǰҳ��/��ҳ��
	//����ͨ�����벻ͬ����,��ȡͼƬ����
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
	albumListViewListData->nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_ALL);
	albumListViewListData->nTotalPage = albumListViewListData->nItemCount/6;

	if(albumListViewListData->nItemCount%6!=0)
	{
		albumListViewListData->nTotalPage += 1;
	}
	
	// --------------------------------------
	//
	// ********* 1 ��ʾ����������Ϣ *********
	//
	// --------------------------------------
	rect.left = 0;rect.top = 0;rect.right = 1024;rect.bottom = 42;
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUTITLEBARBACKGROUND, hWnd, &rc, XMGIF_DRAW_POS_LEFTTOP);

	rect.left = 20;rect.top = 3;rect.right = 65;rect.bottom = 36;
	AP_RomImageDrawByMenuID(AP_ID_VIDEO_TITLE_ALBUM, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//����:��Ƶ/ͼƬ
	
	rect.left = 450;rect.top = 3;rect.right = 65;rect.bottom = 36;
	AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_CH, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//ͨ��
	rect.left = 515;rect.top = 3;rect.right = 65;rect.bottom = 36;
	AP_RomImageDrawByMenuID(MenuID, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//CH1/CH2

	rect.left = 930;rect.top = 3;rect.right = 65;rect.bottom = 36;
    sprintf(String,"%d%s%d", albumListViewListData->nPageNum, "/", albumListViewListData->nTotalPage);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString(hWnd, rect.left, rect.top, String, strlen(String));

	// ��5����ʾ��Ƶ�б�
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
	
	//��ʾѡ���
	diff_val = albumListViewListData->nCurItem;
	if(albumListViewListData->nCurItem!=-1 && albumListViewListData->nItemCount >0)
	{   
		if(diff_val == 0)
		{
			rect.left = Photo_Thumb_Hor_Interval-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y);
		}
		else if(diff_val == 1)
		{
		    rect.left = Photo_Thumb_Hor_Interval+Photo_Thumb_Hor_Interval+Photo_Thumb_width-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y);
		}
		else if(diff_val == 2)
		{
			rect.left = Photo_Thumb_Hor_Interval+Photo_Thumb_Hor_Interval+Photo_Thumb_width+Photo_Thumb_Hor_Interval+Photo_Thumb_width-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y);
		}
		if(diff_val == 3)
		{
			rect.left = Photo_Thumb_Hor_Interval-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + Photo_Thumb_height+Photo_Thumb_Vert_Interval+50);
		}
		else if(diff_val == 4)
		{
		    rect.left = Photo_Thumb_Hor_Interval+Photo_Thumb_Hor_Interval+Photo_Thumb_width-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + Photo_Thumb_height+Photo_Thumb_Vert_Interval+50);
		}
		else if(diff_val == 5)
		{
			rect.left = Photo_Thumb_Hor_Interval+Photo_Thumb_Hor_Interval+Photo_Thumb_width+Photo_Thumb_Hor_Interval+Photo_Thumb_width-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + Photo_Thumb_height+Photo_Thumb_Vert_Interval+50);
		}
		rect.right = rect.left+Photo_Thumb_width+8+10;
		rect.bottom = rect.top +Photo_Thumb_height+8+10;
		XM_FillRect(hWnd,rect.left,rect.top,rect.right,rect.bottom,XM_RGB(0,255,0));
	}
	
	// ��ʾĿ¼��
	x = Photo_Thumb_Hor_Interval;
	y = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 10);
	for(i = 0; i < 6; i++)
	{	
		unsigned char ch;
		HANDLE hPhotoItem;
		XMVIDEOITEM *pPhotoItem;

	    item = (albumListViewListData->nPageNum-1)*6 + i;

		XM_printf(">>>>>item:%d\r\n", item);
		if(item >= albumListViewListData->nItemCount)
			break;
		
		if(albumListViewListData->nItemCount <= 0)
			break;

		hPhotoItem = XM_VideoItemGetVideoItemHandleEx(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL,item,1);
		if(hPhotoItem == NULL)
			break;
		
		pPhotoItem = AP_VideoItemGetVideoItemFromHandle(hPhotoItem);
		if(pPhotoItem == NULL)
			break;
		
		// ��ȡ��Ƭ���ļ���
		if(XM_VideoItemGetVideoFilePath(pPhotoItem, replay_ch, text, sizeof(text)))
		{
			unsigned int w = Photo_Thumb_width;
			unsigned int h = Photo_Thumb_height;
			unsigned int yuvstride = Photo_Thumb_width;
			unsigned int imagestride;
			unsigned char *jpeg_image;
			XM_IMAGE *lpImage = NULL;

			XM_printf(">>>>>>>>>>>>text:%s\r\n", text);
			imagestride = w;
			imagestride *= 4;
			lpImage = (XM_IMAGE *)kernel_malloc (imagestride * h + sizeof(XM_IMAGE) - 4);
			jpeg_image = (unsigned char *)kernel_malloc (yuvstride * h * 3/2);

			if(jpeg_image)
			{
				if(zoom_decode_jpeg_file(text, jpeg_image,w, h) == 0)
				{
				    XM_ImageConvert_Y_UV420_to_ARGB(jpeg_image,(unsigned char *)lpImage->image,w,h);
					lpImage->id = XM_IMAGE_ID;
					lpImage->format = XM_OSD_LAYER_FORMAT_ARGB888;
					lpImage->stride = (unsigned short)imagestride;
					lpImage->width = w;
					lpImage->height = h;
					lpImage->ayuv = NULL;
					
					XM_ImageDisplay (lpImage, hWnd, x, y);
				}
				else
				{
					photo_file_error_flag[i] = 1;
					XM_ImageDisplay(photo_file_error_image[0], hWnd, x, y);
					XM_printf(">>>>>file error............\r\n");
				}	
				
				//��ʾʱ���ļ���
				XM_GetDateTimeFromFileName ((char *)pPhotoItem->video_name, &filetime);
				sprintf(text, "%04d/%02d/%02d %02d:%02d:%02d", filetime.wYear, 
																filetime.bMonth,
																filetime.bDay,
																filetime.bHour,
																filetime.bMinute,
																filetime.bSecond);

				AP_TextGetStringSize (text, strlen(text), &size);
				AP_TextOutDataTimeString(hWnd, x, y + Photo_Thumb_height +10, text, strlen(text));

				kernel_free (jpeg_image);
				kernel_free (lpImage);
			}
		}

		x = (XMCOORD)(x + Photo_Thumb_width + Photo_Thumb_Hor_Interval);		
		if(i==2)
		{
		   	x = Photo_Thumb_Hor_Interval;
			y = (XMCOORD)(y + Photo_Thumb_height+Photo_Thumb_Vert_Interval+50);
		}
	}

	//����ť�ؼ�����ʾ��
	//�����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
	XM_SetWindowAlpha (hWnd, (unsigned char)old_alpha);
}


VOID AlbumListViewOnKeyDown(XMMSG *msg)
{
	int photo_count;
	ALBUMLISTVIEWLISTDATA *albumListViewListData = (ALBUMLISTVIEWLISTDATA *)XM_GetWindowPrivateData(XMHWND_HANDLE(AlbumListView));
	if(albumListViewListData == NULL)
		return;

	XM_printf(">>>>>>>>AlbumListViewOnKeyDown, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
	// ������
	XM_Beep (XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
		case VK_AP_MENU://MENU��,����ط�ģʽ�˵�
			XM_printf(">>>>>>>VK_AP_MENU\r\n");
            XM_PushWindowEx(XMHWND_HANDLE(PhotoMenu),(DWORD)albumListViewListData);//�طŲ˵�
			break;

		case VK_AP_SWITCH://OK��
			XM_printf(">>>>>>>VK_AP_SWITCH\r\n");
			if(!photo_file_error_flag[albumListViewListData->nCurItem])
			{
				if(albumListViewListData->nItemCount > 0)
				{	
					// ����
					XM_PushWindowEx(XMHWND_HANDLE(AlbumView), (albumListViewListData->videotype<< 24) | (WORD)((albumListViewListData->nPageNum-1)*6+albumListViewListData->nCurItem));
				}
			}
			break;
		
		case VK_AP_DOWN://-��		
			XM_printf(">>>>>>>VK_AP_DOWN\r\n");
			if(albumListViewListData->nItemCount <= 0)
				return;

			if(albumListViewListData->nCurItem==0)
			{
				if(albumListViewListData->nPageNum==1)
				{
					albumListViewListData->nPageNum = albumListViewListData->nTotalPage;
		
					if(albumListViewListData->nItemCount%6==0)
					{
						albumListViewListData->nCurItem = 5;
					}
					else
					{
						albumListViewListData->nCurItem = albumListViewListData->nItemCount%6-1;
					}
				}
				else
				{
					albumListViewListData->nPageNum--;
					albumListViewListData->nCurItem = 5;
				}
			}
			else
			{
				albumListViewListData->nCurItem--;
			}		
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_UP:////+��	ѡ����һ����
			XM_printf(">>>>>>>VK_AP_UP\r\n");
			if(albumListViewListData->nItemCount <= 0)
				return;

			albumListViewListData->nCurItem ++;
			if(((albumListViewListData->nPageNum-1)*6+albumListViewListData->nCurItem) < albumListViewListData->nItemCount)
			{
				if(albumListViewListData->nCurItem==albumListViewListData->nVisualCount)
				{
					albumListViewListData->nCurItem = 0;
					albumListViewListData->nPageNum++;
				}
			}
			else
			{
				albumListViewListData->nPageNum = 1;
				albumListViewListData->nCurItem = 0;
			}
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

        case VK_AP_FONT_BACK_SWITCH://AV�����˳��ط�ģʽ
			XM_printf(">>>>>>>VK_AP_FONT_BACK_SWITCH\r\n");
			XM_JumpWindow(XMHWND_HANDLE(Desktop));
			if(albumListViewListData->nItemCount <= 0)
				return;
            break;
			
		default:
			break;
	}
}

VOID AlbumListViewOnKeyUp (XMMSG *msg)
{
	XM_printf(">>>>>>>>AlbumListViewOnKeyUp, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
}

VOID AlbumListViewOnTimer (XMMSG *msg)
{
	// ��⿨״̬
	#if 0
	if(APSYS_CardChecking() == APP_SYSTEMCHECK_CARD_NOCARD)
	{
		// ���γ���ֱ�������ص�����(���渺�𿨲�ε����д���), �˴�ǿ���������³�ʼ��
		XM_JumpWindow (XMHWND_HANDLE(Desktop));
		return;
	}
	#endif

}

VOID AlbumListViewOnSystemEvent (XMMSG *msg)
{
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:			// SD���γ��¼�
			XM_BreakSystemEventDefaultProcess (msg);		// ������ȱʡ����
			// ������Ϣ���²��뵽��Ϣ���ж���
			XM_InsertMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
			XM_JumpWindowEx (XMHWND_HANDLE(Desktop), HWND_CUSTOM_DEFAULT, XM_JUMP_POPDEFAULT);
			//XM_JumpWindow (XMHWND_HANDLE(Desktop));	// ��ת������
			//XM_PullWindow (0);					   // ������ǰ���б��Ӵ�
			break;
	}
}
// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (AlbumListView)
	XM_ON_MESSAGE (XM_PAINT, AlbumListViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, AlbumListViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, AlbumListViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, AlbumListViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, AlbumListViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, AlbumListViewOnTimer)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, AlbumListViewOnSystemEvent)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, AlbumListView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
