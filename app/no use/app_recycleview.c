//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_recycleview.c
//	  ����վ����
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuid.h"
#include "app_album.h"
#include "xm_recycle.h"


// ˽�������
#define	RECYCLEVIEW_COMMAND_RESTORE		1
#define	RECYCLEVIEW_COMMAND_SWITCH		2
#define	RECYCLEVIEW_COMMAND_RETURN		3
#define RECYCLEVIEW_COMMAND_UP			4
#define RECYCLEVIEW_COMMAND_DOWN			5

// �����á����ڰ�ť�ؼ�����
#define	RECYCLEVIEWBTNCOUNT				5



// ����վ�İ�ť�ؼ�����
static const XMBUTTONINFO recycleViewBtn[RECYCLEVIEWBTNCOUNT] = {
    // �Ϸ�
    {	
		VK_AP_UP,		RECYCLEVIEW_COMMAND_UP,			AP_ID_BUTTON_LAST,	AP_ID_BUTTON_LAST
	},
	// �·�
	{	
		VK_AP_DOWN,		RECYCLEVIEW_COMMAND_DOWN,		AP_ID_BUTTON_NEXT,	AP_ID_BUTTON_NEXT
	},
	// ��ԭ
	{	
		VK_AP_SWITCH,	RECYCLEVIEW_COMMAND_RESTORE,	AP_ID_COMMON_MENU,	AP_ID_RECYCLE_BUTTON_RESTORE_ONE
	},
	// �л���"¼���б�"
	{	
		VK_AP_MENU,		RECYCLEVIEW_COMMAND_SWITCH,		AP_ID_COMMON_MODE,	AP_ID_VIDEO_BUTTON_VIDEO
	},
	
	// ����
	{	
		VK_AP_MODE,		RECYCLEVIEW_COMMAND_RETURN,		AP_ID_COMMON_OK,	AP_ID_BUTTON_RETURN
	},
};



typedef struct tagRECYCLEVIEWDATA {
	int					nTopItem;					// ��һ�����ӵĲ˵���
	int					nCurItem;					// ��ǰѡ��Ĳ˵���

	int					nItemCount;					// �˵������
	SHORT				nVisualCount;				// �Ӵ�����ʾ����Ŀ����
	
	BYTE				channel;

	XMBUTTONCONTROL		btnControl;

	char				fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	XMFILEFIND			fileFind;
} RECYCLEVIEWDATA;

static void adjust_position (RECYCLEVIEWDATA *recycleViewListData)
{
	int item_count = XM_RecycleGetItemCount();
	if(item_count == 0)
	{
		recycleViewListData->nItemCount = 0;
		recycleViewListData->nTopItem = 0;
		recycleViewListData->nCurItem = 0;
	}
	else 
	{
		if(recycleViewListData->nTopItem >= item_count)
		{
			recycleViewListData->nTopItem = (item_count - 1);
		}
		if(recycleViewListData->nCurItem >= item_count)
		{
			recycleViewListData->nCurItem = (item_count - 1);
		}
		if(recycleViewListData->nCurItem < recycleViewListData->nTopItem)
			recycleViewListData->nTopItem = recycleViewListData->nCurItem;
	}
	
	// ����Ƿ���Ҫ����������Ŀ���Ա㾡����������ʾ
	if(item_count <= recycleViewListData->nVisualCount)
	{
		recycleViewListData->nTopItem = 0;
	}
	else
	{
		if( (recycleViewListData->nTopItem + recycleViewListData->nVisualCount - 1) < recycleViewListData->nCurItem )
			recycleViewListData->nTopItem = recycleViewListData->nCurItem - recycleViewListData->nVisualCount + 1;
		while( (recycleViewListData->nTopItem + recycleViewListData->nVisualCount) > item_count )
		{
			recycleViewListData->nTopItem --;
		}
		XM_ASSERT ((recycleViewListData->nCurItem - recycleViewListData->nTopItem + 1) <= recycleViewListData->nVisualCount);
	}
}

VOID RecycleViewOnEnter (XMMSG *msg)
{
	int item_count;
	RECYCLEVIEWDATA *recycleViewListData;
	XM_printf ("RecycleViewOnEnter %d\n", msg->wp);

	if(msg->wp == 0)
	{
		// ����δ��������һ�ν���
		XMRECT rc;
		int nVisualCount;

		// ����˽�����ݾ��
		recycleViewListData = XM_calloc (sizeof(RECYCLEVIEWDATA));
		if(recycleViewListData == NULL)
		{
			XM_printf ("recycleViewListData XM_calloc failed\n");
			// ʧ�ܷ��ص������ߴ���
			XM_PullWindow (0);
			return;
		}
		
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(RecycleView), recycleViewListData);

		recycleViewListData->channel = 0;
		// ��ʼɨ�����з�����������Ƶ�ļ�
		recycleViewListData->nTopItem = 0;
		recycleViewListData->nCurItem = 0;
		XM_GetDesktopRect (&rc);
		nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
		nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
		recycleViewListData->nVisualCount = (SHORT)nVisualCount;

		// ��ť�ؼ���ʼ��
		AP_ButtonControlInit (&recycleViewListData->btnControl, RECYCLEVIEWBTNCOUNT, 
			XMHWND_HANDLE(RecycleView), recycleViewBtn);

		XM_RecycleInit ();
		recycleViewListData->nItemCount = XM_RecycleGetItemCount();
	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�		
		recycleViewListData = (RECYCLEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleView));
		if(recycleViewListData == NULL)
			return;

		adjust_position (recycleViewListData);

		XM_printf ("RecycleView Pull\n");
	}
}

VOID RecycleViewOnLeave (XMMSG *msg)
{
	XM_printf ("RecycleViewOnLeave %d\n", msg->wp);

	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		RECYCLEVIEWDATA *recycleViewListData = (RECYCLEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleView));
		// �ͷ����з������Դ
		if(recycleViewListData)
		{
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&recycleViewListData->btnControl);
			// �ͷ�˽�����ݾ��
			XM_free (recycleViewListData);
		}
		// ���ô��ڵ�˽�����ݾ��Ϊ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(RecycleView), NULL);

		XM_RecycleExit ();

		XM_printf ("RecycleView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("RecycleView Push\n");
	}
}

extern int get_recycle_item_name (XM_RECYCLEITEM *item, char *name);
extern int zoom_decode_jpeg_file (const char *file_name,unsigned char *zoombuffer,unsigned int w,unsigned int h);

VOID RecycleViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
	int i;
	XMCOORD x, y;
	unsigned int old_alpha;
	HANDLE hWnd;
	int item_count;
	int nVisualCount;
	int diff_val;
	char text[64];
	float scale_factor;		// ˮƽ��������
	XMCOORD item_icon_x, item_name_x, item_ch_x;	// �����������ͼ��, ����ʱ��, ͨ���ŵ�x����
	unsigned int w = 256;
	unsigned int h = 128;
	unsigned int yuvstride = 256;
	unsigned int imagestride;
	unsigned char *thumb_image;
	unsigned char *jpeg_image;
	XM_IMAGE *lpImage = NULL;
			
	RECYCLEVIEWDATA *recycleViewListData = (RECYCLEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleView));
	if(recycleViewListData == NULL)
		return;
	
	item_count = XM_RecycleGetItemCount ();
	hWnd = XMHWND_HANDLE(RecycleView);
	XM_GetDesktopRect (&rc);

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
	AP_DrawTitlebarControl (hWnd, AP_NULLID, AP_ID_RECYCLE_TITLE);
    
	// ��5����ʾ��Ƭ�б�
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
	rect = rc;
	rect.left = 0;
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (recycleViewListData->nCurItem - recycleViewListData->nTopItem) * APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMBACKGROUND, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	// ��ʾĿ¼��
	x = item_name_x;
	y = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1);
	for (i = 0; i < nVisualCount; i++)
	{
		XM_RECYCLEITEM *item;
		XMSYSTEMTIME filetime;
		XMSIZE size;

		item = XM_RecycleGetItem (recycleViewListData->nTopItem + i);
		if(item == NULL)
			break;

		rect = rc;
		rect.top = y;
		rect.bottom = rect.top + APP_POS_ITEM5_LINEHEIGHT - 1;
		rect.left = item_icon_x;
		rect.right = rect.left + 24;
		if(item->file_type == XM_FILE_TYPE_VIDEO)
		{
			// ��ʾ��Ƶ�ļ����ļ�����ͼ��
			AP_RomImageDrawByMenuID (AP_ID_RECYCLE_ICON_VIDEO_24, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		}
		else
		{
			// ��ʾ��Ƭ�ļ����ļ�����ͼ��
			AP_RomImageDrawByMenuID (AP_ID_RECYCLE_ICON_PHOTO_24, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		}

		// ����Ƭ���ļ����ƻ�ȡ��Ƭ�Ĵ���������ʱ��
		XM_GetDateTimeFromFileName ((char *)item->item_name, &filetime);

		sprintf (text, "%04d/%02d/%02d %02d:%02d:%02d", filetime.wYear, 
																		filetime.bMonth,
																		filetime.bDay,
																		filetime.bHour,
																		filetime.bMinute,
																		filetime.bSecond);
																		
		AP_TextGetStringSize (text, strlen(text), &size);
		AP_TextOutDataTimeString (hWnd, item_name_x, y + (APP_POS_ITEM5_LINEHEIGHT - size.cy)/2, text, strlen(text));

		// ��ʾͨ����Ŷ�Ӧ��ICON
		rect = rc;
		rect.top = y;
		rect.bottom = rect.top + APP_POS_ITEM5_LINEHEIGHT - 1;
		rect.left = item_ch_x;
		rect.right = rect.left + 24;
		if(item->channel == XM_VIDEO_CHANNEL_0)
		{
			// ǰ������ͷ
			AP_RomImageDrawByMenuID (AP_ID_RECYCLE_ICON_CHANNEL_1_16, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		}
		else if(item->channel == XM_VIDEO_CHANNEL_1)
		{
			// ��������ͷ
			AP_RomImageDrawByMenuID (AP_ID_RECYCLE_ICON_CHANNEL_2_16, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		}

		y = (XMCOORD)(y + APP_POS_ITEM5_LINEHEIGHT);
	}
	#else  //��΢ͼ��ʾ

	//��ʾѡ���
	diff_val = recycleViewListData->nCurItem - recycleViewListData->nTopItem;
	if((recycleViewListData->nCurItem!=-1)&&item_count !=0)
	{   
		if(diff_val == 0)
		{
			rect.left = 26;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 10);
		}
		else if(diff_val == 1)
		{
		    rect.left = 316;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 10);
		}
		else if(diff_val == 2)
		{
			rect.left = 26;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 10+128+14);
		}
		else if(diff_val == 3)
		{
			rect.left = 316;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 10+128+14);
		}
		rect.right = rect.left+256+8;
		rect.bottom = rect.top +128+8;
		XM_FillRect(hWnd,rect.left,rect.top,rect.right,rect.bottom,XM_RGB(255,0,0));
	}
	
	// ��ʾĿ¼��
	x = 30;
	y = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 14);

	for (i = 0; i < nVisualCount; i++)
	{
	   	XM_RECYCLEITEM *item;
		XMSYSTEMTIME filetime;
		XMSIZE size;

		item = XM_RecycleGetItem (recycleViewListData->nTopItem + i);
		if(item == NULL)
			break;

		if(item->file_type == XM_FILE_TYPE_VIDEO)
		{
		    if(get_recycle_item_name (item, text) == 0)
		    {
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

						// ��ʾ�ļ�����ͼ��
						rect = rc;
						rect.top = y+10;
						rect.bottom = rect.top + 36;
						rect.left = x+10;
						rect.right = rect.left + 50;
						AP_RomImageDrawByMenuID (AP_ID_RECYCLE_ICON_VIDEO_24, hWnd, &rect, XMGIF_DRAW_POS_CENTER);

						//��ȡ¼��ͨ��
						rect = rc;
						rect.top = y+10;
						rect.bottom = rect.top + 36;
						rect.left = x+48;
						rect.right = rect.left + 50;
						
						if(item->channel == XM_VIDEO_CHANNEL_0)
						{
							// ǰ������ͷ
							AP_RomImageDrawByMenuID (AP_ID_RECYCLE_ICON_CHANNEL_1_16, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
						}
						else if(item->channel == XM_VIDEO_CHANNEL_1)
						{
							// ��������ͷ
							AP_RomImageDrawByMenuID (AP_ID_RECYCLE_ICON_CHANNEL_2_16, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
						}
						kernel_free (thumb_image);
						kernel_free (lpImage);
					}
				}
		    }
		}
		else
		{
		    if(get_recycle_item_name (item, text) == 0)
		    {
                  imagestride = w;
                  imagestride *= 4;
                  lpImage = (XM_IMAGE *)kernel_malloc (imagestride * h + sizeof(XM_IMAGE) - 4);
                  jpeg_image = (unsigned char *)kernel_malloc (yuvstride * h * 3/2);

				  if(jpeg_image)
				  {
				      if(zoom_decode_jpeg_file (text, jpeg_image,w, h) == 0)
				      {
					        XM_ImageConvert_Y_UV420_to_ARGB(jpeg_image,(unsigned char *)lpImage->image,w,h);
							lpImage->id = XM_IMAGE_ID;
							lpImage->format = XM_OSD_LAYER_FORMAT_ARGB888;
							lpImage->stride = (unsigned short)imagestride;
							lpImage->width = w;
							lpImage->height = h;
							lpImage->ayuv = NULL;
							
							XM_ImageDisplay (lpImage, hWnd, x, y);
							
							//��ʾ�ļ�����ͼ��
							rect = rc;
							rect.top = y+10;
							rect.bottom = rect.top + 36;
							rect.left = x+10;
							rect.right = rect.left + 50;
							AP_RomImageDrawByMenuID (AP_ID_RECYCLE_ICON_PHOTO_24, hWnd, &rect, XMGIF_DRAW_POS_CENTER);

							//��ȡ¼��ͨ��
							rect = rc;
							rect.top = y+10;
							rect.bottom = rect.top + 36;
							rect.left = x+48;
							rect.right = rect.left + 50;
							
							if(item->channel == XM_VIDEO_CHANNEL_0)
							{
								// ǰ������ͷ
								AP_RomImageDrawByMenuID (AP_ID_RECYCLE_ICON_CHANNEL_1_16, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
							}
							else if(item->channel == XM_VIDEO_CHANNEL_1)
							{
								// ��������ͷ
								AP_RomImageDrawByMenuID (AP_ID_RECYCLE_ICON_CHANNEL_2_16, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
							}
							kernel_free (jpeg_image);
							kernel_free (lpImage);
				    }
				  }
		    }			
		}
		x = (XMCOORD)(x + 256 + 34);		
		if(i==1)
		{
		   	x=30;
			y=(XMCOORD)(y + 128+14);
		}
	}
	#endif
	
	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
	AP_ButtonControlMessageHandler (&recycleViewListData->btnControl, msg);
	XM_SetWindowAlpha (hWnd, (unsigned char)old_alpha);
}


VOID RecycleViewOnKeyDown (XMMSG *msg)
{
	int item_count;
	RECYCLEVIEWDATA *recycleViewListData = (RECYCLEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleView));
	if(recycleViewListData == NULL)
		return;
	
	// ������
	XM_Beep (XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
		case VK_AP_SWITCH:	// �����л���
		case VK_AP_POWER:		// ��Դ��
			// ��"¼�����"�����У�MENU��Power��Switch��MODE��������Ϊ��ť������
			// �˴������ĸ����¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&recycleViewListData->btnControl, msg);
			break;
			
        #if 1
		case VK_AP_UP:	
			item_count = XM_RecycleGetItemCount ();
			if(item_count <= 0)
				return;

			AP_ButtonControlMessageHandler (&recycleViewListData->btnControl, msg);
			recycleViewListData->nCurItem --;
			if(recycleViewListData->nCurItem < 0)
			{
				// �۽������һ��
				recycleViewListData->nCurItem = (WORD)(item_count - 1);
				recycleViewListData->nTopItem = 0;
				while ( (recycleViewListData->nCurItem - recycleViewListData->nTopItem) >= recycleViewListData->nVisualCount )
				{
					recycleViewListData->nTopItem ++;
				}
			}
			else
			{
				if(recycleViewListData->nTopItem > recycleViewListData->nCurItem)
					recycleViewListData->nTopItem = recycleViewListData->nCurItem;
			}
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_DOWN:	// 
			item_count = XM_RecycleGetItemCount();
			if(item_count <= 0)
				return;

			AP_ButtonControlMessageHandler (&recycleViewListData->btnControl, msg);
			recycleViewListData->nCurItem ++;	
			if(recycleViewListData->nCurItem >= item_count)
			{
				// �۽�����һ��
				recycleViewListData->nTopItem = 0;
				recycleViewListData->nCurItem = 0;
			}
			else
			{
				while ( (recycleViewListData->nCurItem - recycleViewListData->nTopItem) >= recycleViewListData->nVisualCount )
				{
					recycleViewListData->nTopItem ++;
				}
			}

			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;
		#endif
	}
}

VOID RecycleViewOnKeyUp (XMMSG *msg)
{
	RECYCLEVIEWDATA *recycleViewListData;
	recycleViewListData = (RECYCLEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleView));
	if(recycleViewListData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
		case VK_AP_POWER:
		case VK_AP_SWITCH:
			// ��"ʱ������"�����У�MENU��MODE��������Ϊ��ť������
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&recycleViewListData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&recycleViewListData->btnControl, msg);
			break;
	}
}

VOID RecycleViewOnCommand (XMMSG *msg)
{
	RECYCLEVIEWDATA *recycleViewListData;
	recycleViewListData = (RECYCLEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleView));
	if(recycleViewListData == NULL)
		return;

	XM_printf ("RecycleViewOnCommand msg->wp=%d\n", msg->wp);

	switch (msg->wp)
	{
		case RECYCLEVIEW_COMMAND_RETURN:
			// ���ص�����
			XM_PullWindow (0);
			break;

		case RECYCLEVIEW_COMMAND_SWITCH:
			// ����վ --> ¼��
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
			XM_JumpWindowEx (XMHWND_HANDLE(VideoListView), VIDEOLIST_ALL_VIDEO_MODE, 0);		// �л���¼�����ģʽ
			break;
		
		case RECYCLEVIEW_COMMAND_RESTORE:
			if(XM_RecycleGetItemCount())
			{
				// ��������һ���ɻ�����Ŀ
				if(XM_RecycleRestoreFile (recycleViewListData->nCurItem) == 0)
				{	
					adjust_position (recycleViewListData);
					// ˢ��
					XM_InvalidateWindow ();
					XM_UpdateWindow ();
				}
				else
				{
					// ��ʾ�û���ԭʧ��
					XM_OpenAlertView (AP_ID_RECYCLE_INFO_RESTORE_FILE_ERROR,
										AP_NULLID,
										0,
										0,
										0,
										APP_ALERT_BKGCOLOR,
										(float)1.2,		// �Զ��ر�ʱ��
										APP_ALERT_BKGALPHA,
										0,			// �����ص�����
										0,
										XM_VIEW_ALIGN_CENTRE,		// ���ж���
										0		// ALERTVIEW��ͼ�Ŀ���ѡ��
										);
				}
				break;
			}
			break;
			
         case RECYCLEVIEW_COMMAND_UP:
			if(recycleViewListData->nItemCount <= 0)
				return;

			AP_ButtonControlMessageHandler (&recycleViewListData->btnControl, msg);
			recycleViewListData->nCurItem --;
			if(recycleViewListData->nCurItem < 0)
			{
				// �۽������һ��
				recycleViewListData->nCurItem = (WORD)(recycleViewListData->nItemCount - 1);
				recycleViewListData->nTopItem = 0;
				while ((recycleViewListData->nCurItem - recycleViewListData->nTopItem) >= recycleViewListData->nVisualCount )
				{
					recycleViewListData->nTopItem ++;
				}
			}
			else
			{
				if(recycleViewListData->nTopItem > recycleViewListData->nCurItem)
					recycleViewListData->nTopItem = recycleViewListData->nCurItem;
			}
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;
			
		case RECYCLEVIEW_COMMAND_DOWN:
			if(recycleViewListData->nItemCount <= 0)
				return;

			AP_ButtonControlMessageHandler (&recycleViewListData->btnControl, msg);
			recycleViewListData->nCurItem ++;	
			if(recycleViewListData->nCurItem >= recycleViewListData->nItemCount)
			{
				// �۽�����һ��
				recycleViewListData->nTopItem = 0;
				recycleViewListData->nCurItem = 0;
			}
			else
			{
				while ((recycleViewListData->nCurItem - recycleViewListData->nTopItem) >= recycleViewListData->nVisualCount )
				{
					recycleViewListData->nTopItem ++;
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


VOID RecycleViewOnSystemEvent (XMMSG *msg)
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
VOID RecycleViewOnTouchUp (XMMSG *msg)
{
	RECYCLEVIEWDATA *recycleViewListData;
	recycleViewListData = (RECYCLEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleView));
	if(recycleViewListData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&recycleViewListData->btnControl, msg))
		return;
}
VOID RecycleViewOnTouchDown (XMMSG *msg)
{
	int index;
	RECYCLEVIEWDATA *recycleViewListData = (RECYCLEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleView));
	if(recycleViewListData == NULL)
		return;
	
	if(AP_ButtonControlMessageHandler (&recycleViewListData->btnControl, msg))
		return;	
      index = AppLocateItem (XMHWND_HANDLE(RecycleView), recycleViewListData->nItemCount, APP_POS_ITEM5_LINEHEIGHT, recycleViewListData->nTopItem, LOWORD(msg->lp), HIWORD(msg->lp));
	if(index < 0)
		return;
	//recycleViewListData->nTouchItem = index;
	if(recycleViewListData->nCurItem != index)
	{
		recycleViewListData->nCurItem = index;
		XM_InvalidateWindow ();
		XM_UpdateWindow ();
	}
}


// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (RecycleView)
	XM_ON_MESSAGE (XM_PAINT, RecycleViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, RecycleViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, RecycleViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, RecycleViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, RecycleViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, RecycleViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, RecycleViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, RecycleViewOnTouchUp)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, RecycleViewOnSystemEvent)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, RecycleView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
