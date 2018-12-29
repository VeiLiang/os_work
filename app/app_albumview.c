//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_albumview.c
//	  ����б��������
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include <xm_jpeg_codec.h>
#include "arkn141_codec.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"
//#include "app_album.h"
#include "system_check.h"
#include "xm_rotate.h"


#define	ALBUM_BUTTON_AUTO_HIDE_TIMEOUT	3000	// ��ť/��Ƭ�����Զ�����ʱ�� 3000����


static int decode_jpeg_file(const char *file_name);
void* kernel_malloc(unsigned int n);
void  kernel_free (void* pMemBlock);

typedef struct tagALBUMVIEWDATA {
	SHORT					nCurItem;					// ��ǰѡ��Ĳ˵���

	int						nItemCount;					// �˵������
	
	BYTE					channel;

	BYTE					delete_one_confirm;
	BYTE					delete_all_confirm;

	BYTE					button_hide;				// ��ť����ʾ/����     1 ���� 0 ��ʾ
	BYTE					photo_title_hide;			// ��Ƭ�������ʾ/���� 1 ���� 0 ��ʾ
	BYTE					photo_damaged;				// ��Ƭ�������

	XMBUTTONCONTROL			btnControl;

	char					fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	XMFILEFIND				fileFind;
} ALBUMVIEWDATA;


static VOID AlbumViewOnEnter (XMMSG *msg)
{
	char text[64];
	ALBUMVIEWDATA *albumViewData;
	HANDLE hPhotoItem;
	XMVIDEOITEM *pPhotoItem;
	unsigned int replay_ch;
	
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlbumViewOnEnter, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

	// �رչ���ANIMATION
	XM_DisableViewAnimate (XMHWND_HANDLE(AlbumView));

	if(msg->wp == 0)
	{
		// ����δ��������һ�ν���
		// ����˽�����ݾ��
		albumViewData = XM_calloc (sizeof(ALBUMVIEWDATA));
		if(albumViewData == NULL)
		{
			XM_printf ("albumViewData XM_calloc failed\n");
			// ʧ�ܷ��ص������ߴ���
			XM_PullWindow (0);
			return;
		}

		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData(XMHWND_HANDLE(AlbumView), albumViewData);

		if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
		{
			replay_ch = XM_VIDEO_CHANNEL_0;
		}
		else
		{
			replay_ch = XM_VIDEO_CHANNEL_1;
		}
		
		albumViewData->channel = 0;
		// ��ʼɨ�����з�����������Ƶ�ļ�
		albumViewData->nCurItem = (msg->lp) & 0xFFFF;
		albumViewData->nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_ALL);
		XM_printf(">>>>>>albumViewData->nCurItem:%d\r\n", albumViewData->nCurItem);
		XM_printf(">>>>>>albumViewData->nItemCount:%d\r\n", albumViewData->nItemCount);

		hPhotoItem = XM_VideoItemGetVideoItemHandleEx(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL,albumViewData->nCurItem,1);
		if(hPhotoItem == NULL)
			return;
		
		pPhotoItem = AP_VideoItemGetVideoItemFromHandle(hPhotoItem);
		if(pPhotoItem == NULL)
			return;
		
		// ��ȡ��Ƭ���ļ���
		if(XM_VideoItemGetVideoFilePath(pPhotoItem, replay_ch, text, sizeof(text)))
		{
			XM_printf(">>>>>>>>>>>>>>text:%s.............\r\n", text);
			// ����Ƭ���벢��ʾ��OSD 0��(��Ƶ��)
			if(decode_jpeg_file(text))
			{
				// �������
				albumViewData->photo_damaged = 1;
			}
			else
			{
				albumViewData->photo_damaged = 0;
			}
		}
		else
		{
			albumViewData->photo_damaged = 1;
		}
	}
	else
	{
		int repaint = 0;		// �Ƿ��ػ洰��
		// �����ѽ�������ǰ���ڴ�ջ�лָ�		
		albumViewData = (ALBUMVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumView));
		if(albumViewData == NULL)
			return;

		XM_printf ("AlbumView Pull\n");
	}

	// ������ʱ�������ڰ�ť����Ƭ���������
	// ����x��Ķ�ʱ��
	XM_SetTimer (XMTIMER_ALBUMLISTVIEW, ALBUM_BUTTON_AUTO_HIDE_TIMEOUT);
}

static VOID AlbumViewOnLeave (XMMSG *msg)
{
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlbumViewOnLeave, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);
	// ɾ����ʱ��
	XM_KillTimer (XMTIMER_ALBUMLISTVIEW);

	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		ALBUMVIEWDATA *albumViewData = (ALBUMVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumView));
		// �ͷ����з������Դ
		if(albumViewData)
		{
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&albumViewData->btnControl);
			// �ͷ�˽�����ݾ��
			XM_free (albumViewData);
		}
		// ���ô��ڵ�˽�����ݾ��Ϊ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(AlbumView), NULL);

		XMSYS_H264CodecRecorderStart();
		XM_printf ("AlbumView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("AlbumView Push\n");
	}
}

extern int xm_jpeg_file_decode( const char *jpeg_data, size_t jpeg_size, 
								unsigned char *yuv,		// Y_UV420�����ַ(NV12��ʽ)
								unsigned int width,		// ������������ؿ��
								unsigned int height		// ������������ظ߶�
								);



// ����Ƭ���벢��ʾ��OSD 0��(��Ƶ��)
// -1 ����ʧ��
//  0 ����ɹ�
static int decode_jpeg_file (const char *file_name)
{
	xm_osd_framebuffer_t yuv_framebuffer;
	unsigned int w, h;
	unsigned int size; 
	void *fp;
	char *jpg = NULL;
	int ret = -1;

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
						1		// clear_framebuffer_context
						);
	if(yuv_framebuffer == NULL)
	{
		XM_printf ("AlbumView failed, XM_osd_framebuffer_create NG\n");
		return -1;
	}

	do
	{
		fp = XM_fopen (file_name, "rb");
		if(fp == NULL)
		{
			XM_printf ("AlbumView failed, open file(%s) NG\n", file_name);
			break;
		}
		size = XM_filesize (fp);
		if(size == 0 || size > 0x200000)
		{
			XM_printf ("AlbumView failed, file(%s)'s size (%d) illegal\n", file_name, size);
			break;
		}

		jpg = kernel_malloc (size);
		if(jpg == NULL)
		{
			XM_printf ("AlbumView failed, malloc (%d) NG\n", size);
			break;
		}

		if(XM_fread (jpg, 1, size, fp) != size)
		{
			XM_printf ("AlbumView failed, XM_fread (%d) NG\n", size);
			break;
		}

		XM_fclose (fp);
		fp = NULL;

#ifdef LCD_ROTATE_90		
		{
			unsigned char *rotate_buffer = kernel_malloc (w * h * 3/2);
			if(rotate_buffer)
			{
				//ret = xm_jpeg_decode ((const char *)jpg, size, (unsigned char *)rotate_buffer, w, h);
				ret = xm_jpeg_file_decode ((const char *)jpg, size, (unsigned char *)rotate_buffer, w, h);
				if(ret == 0)
				{
					u8_t *data[4];
					XM_osd_framebuffer_get_buffer (yuv_framebuffer, data);
					//rotate_degree90 (	rotate_buffer, data[0], w, h, h, w);
					
					xm_rotate_90_by_zoom_opt ((u8_t *)rotate_buffer,
						  (u8_t *)rotate_buffer + w*h,
						  w,	h,	w,
						  0, 0, w, h,
						  data[0],
						  data[1],
						  w, h,
						  0,1);
				}
				kernel_free (rotate_buffer);
			}
			else
			{
				ret = -1;
			}
		}
#else
		// ��JPEG�������뵽�´�����֡����
		dma_inv_range ((unsigned int)yuv_framebuffer->address,(unsigned int)yuv_framebuffer->address + w * h * 3/2);
		ret = xm_jpeg_decode ((const char *)jpg, size, (unsigned char *)yuv_framebuffer->address, w, h);
#endif
		kernel_free (jpg);
		jpg = NULL;


		break;
	} while (jpg);

	if(fp)
		XM_fclose (fp);

	if(jpg)
		kernel_free (jpg);


	if(ret != 0)
	{
		// ����ʧ��
		// ���framebuffer
		XM_osd_framebuffer_clear (yuv_framebuffer, 0, 255, 255, 255);
	}

	// ��framebuffer�ر�, �������Ϊ��ǰ����Ƶ����, ����OSD 0����ʾ
	XM_osd_framebuffer_close (yuv_framebuffer, 0);

	return ret;
}


static void AlbumShowPhotoTime (ALBUMVIEWDATA *albumViewData)
{
	XMSYSTEMTIME filetime;
	char text[64];
	const char *photo_name;
	HANDLE hPhotoItem;
	XMVIDEOITEM *pPhotoItem;
	unsigned int replay_ch;
	
	if(albumViewData->nItemCount <= 0)
		return;
	
	if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
	{
		replay_ch = XM_VIDEO_CHANNEL_0;
	}
	else
	{
		replay_ch = XM_VIDEO_CHANNEL_1;
	}	
		
	hPhotoItem = XM_VideoItemGetVideoItemHandleEx(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL,albumViewData->nCurItem,1);
	if(hPhotoItem == NULL)
		return;
	
	pPhotoItem = AP_VideoItemGetVideoItemFromHandle(hPhotoItem);
	if(pPhotoItem == NULL)
		return;
	
	//��ȡ��Ƭ���ļ���
	XM_VideoItemGetVideoFilePath(pPhotoItem, replay_ch, text, sizeof(text));

	XM_GetDateTimeFromFileName ((char *)pPhotoItem->video_name, &filetime);

	//��ʽ����Ƭ������ʱ��
	sprintf(text, "%04d/%02d/%02d %02d:%02d:%02d", filetime.wYear, 
																		filetime.bMonth,
																		filetime.bDay,
																		filetime.bHour,
																		filetime.bMinute,
																		filetime.bSecond);
	// ��ʾ��Ƭ������ʱ��, �Ӵ������Ͻ�(10, 10)																	
	AP_TextOutDataTimeString(XMHWND_HANDLE(AlbumView), 10, 560, text, strlen(text));
}

static VOID AlbumViewOnPaint (XMMSG *msg)
{
	XMRECT rc,rect;
	unsigned int old_alpha;
	HANDLE hWnd;
	float scale_factor;		// ˮƽ��������

	// ��ȡ���Ӵ�������˽������
	ALBUMVIEWDATA *albumViewData = (ALBUMVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumView));
	if(albumViewData == NULL)
		return;

	// ��ȡ���ڵľ��
	hWnd = XMHWND_HANDLE(AlbumView);

	// AlbumView����ʱ���Ӵ�Alpha����Ϊ0 (�ο��ļ�����ײ�����)
	// ���������Ϊȫ͸������(AlphaΪ0)
	XM_GetDesktopRect (&rc);
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);
	XM_FillRect (hWnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	// �����Ӵ���Alpha����Ϊ255
	old_alpha =  XM_GetWindowAlpha (hWnd);
	XM_SetWindowAlpha(hWnd, 255);

	//�ļ���
	rect.left = 0;rect.top = 600-42;rect.right = 1024;rect.bottom = 42;
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUTITLEBARBACKGROUND, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//�ײ�����ɫ

	//����ͼ��
	rect.left = 500;rect.top = 560;
	AP_RomImageDrawByMenuID(AP_ID_LEFT_ICON, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//�ײ�����ɫ

	rect.left = 600;rect.top = 560;
	AP_RomImageDrawByMenuID(AP_ID_RIGHT_ICON, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//�ײ�����ɫ
	
	//��ʾ��Ƭ�ı���
	AlbumShowPhotoTime(albumViewData);

	// ��Ƭ�������
	if(albumViewData->photo_damaged)
	{
		// ��ʾ������Ϣ
		APPROMRES *AppRes; 
		// ��ȡ��Դ���
		AppRes = AP_AppRes2RomRes (AP_ID_PHOTO_INFO_PHOTO_DAMAGE);
		if(AppRes)
		{
			// ��ʾPNG��Դ, ���Ӵ��Ͼ�����ʾ
			XM_RomImageDraw (AppRes->rom_offset, AppRes->res_length, hWnd, &rc, XMGIF_DRAW_POS_CENTER);
		}
	}

	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
	//AP_ButtonControlMessageHandler (&albumViewData->btnControl, msg);
	XM_SetWindowAlpha (hWnd, (unsigned char)old_alpha);
}


static VOID AlbumViewOnKeyDown (XMMSG *msg)
{
	char text[64];
	HANDLE hPhotoItem;
	XMVIDEOITEM *pPhotoItem;
	unsigned int replay_ch;

	XM_printf(">>>>>>>>>>>>>>>AlbumViewOnKeyDown, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	ALBUMVIEWDATA *albumViewData = (ALBUMVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumView));
	if(albumViewData == NULL)
		return;
	
	// ������
	XM_Beep(XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
		case VK_AP_FONT_BACK_SWITCH://AV��

			break;
			
		case VK_AP_DOWN:	
			XM_printf(">>>>>>>>>>>>VK_AP_DOWN...........\r\n");
			if(albumViewData->nItemCount <= 0)
				return;

			albumViewData->nCurItem --;
			if(albumViewData->nCurItem < 0)
			{// �۽������һ��
				albumViewData->nCurItem = (WORD)(albumViewData->nItemCount - 1);
			}
			
			if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
			{
				replay_ch = XM_VIDEO_CHANNEL_0;
			}
			else
			{
				replay_ch = XM_VIDEO_CHANNEL_1;
			}

			hPhotoItem = XM_VideoItemGetVideoItemHandleEx(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL,albumViewData->nCurItem,1);
			if(hPhotoItem == NULL)
				return;
			
			pPhotoItem = AP_VideoItemGetVideoItemFromHandle(hPhotoItem);
			if(pPhotoItem == NULL)
				return;
			
			// ��ȡ��Ƭ���ļ���
			if(XM_VideoItemGetVideoFilePath(pPhotoItem, replay_ch, text, sizeof(text)))
			{
				XM_printf(">>>>>>>>>>text:%s............\r\n", text);
				// ����Ƭ���벢��ʾ��OSD 0��(��Ƶ��)
				if(decode_jpeg_file(text))
				{
					// �������
					albumViewData->photo_damaged = 1;
				}
				else
				{
					albumViewData->photo_damaged = 0;
				}
			}
		
			XM_SetTimer (XMTIMER_ALBUMLISTVIEW, ALBUM_BUTTON_AUTO_HIDE_TIMEOUT);
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_UP:	
			XM_printf(">>>>>>>>>>>>VK_AP_UP...........\r\n");
			if(albumViewData->nItemCount <= 0)
				return;

			albumViewData->nCurItem ++;	
			if(albumViewData->nCurItem >= albumViewData->nItemCount)
			{
				// �۽�����һ��
				albumViewData->nCurItem = 0;
			}

			if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
			{
				replay_ch = XM_VIDEO_CHANNEL_0;
			}
			else
			{
				replay_ch = XM_VIDEO_CHANNEL_1;
			}
			XM_printf(">>>>>>>>>>>>albumViewData->nCurItem:%d\r\n", albumViewData->nCurItem);
			hPhotoItem = XM_VideoItemGetVideoItemHandleEx(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL,albumViewData->nCurItem,1);
			if(hPhotoItem == NULL)
				return;
			
			pPhotoItem = AP_VideoItemGetVideoItemFromHandle(hPhotoItem);
			if(pPhotoItem == NULL)
				return;
			
			// ��ȡ��Ƭ���ļ���
			if(XM_VideoItemGetVideoFilePath(pPhotoItem, replay_ch, text, sizeof(text)))
			{
				XM_printf(">>>>>>>>>>text:%s............\r\n", text);
				// ����Ƭ���벢��ʾ��OSD 0��(��Ƶ��)
				if(decode_jpeg_file(text))
				{
					// �������
					albumViewData->photo_damaged = 1;
				}
				else
				{
					albumViewData->photo_damaged = 0;
				}
			}
			XM_SetTimer(XMTIMER_ALBUMLISTVIEW, ALBUM_BUTTON_AUTO_HIDE_TIMEOUT);
			// ˢ���봰�ڹ�����OSD��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;
			
		case VK_AP_MENU://MENU��,�˳�����ҳ��
			XM_printf(">>>>>>>>>>>>VK_AP_MENU...........\r\n");

		    XMSYS_H264CodecPlayerStop();
            XM_PullWindow(0);
			break;
			
		default:
			break;
	}

}

static VOID AlbumViewOnKeyUp (XMMSG *msg)
{
	ALBUMVIEWDATA *albumViewData;
	albumViewData = (ALBUMVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumView));
	if(albumViewData == NULL)
		return;
	
	XM_printf(">>>>>>>>>>>>>>>AlbumViewOnKeyUp, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
}

static VOID DeleteOneCallback (VOID *UserPrivate, UINT uKeyPressed)
{
	ALBUMVIEWDATA *albumViewData = UserPrivate;
	if(uKeyPressed == VK_AP_MENU)
	{
		// ȷ��ɾ��
		albumViewData->delete_one_confirm = 1;
	}
	// ��alert����
	XM_PullWindow (0);
}

static VOID DeleteAllCallback (VOID *UserPrivate, UINT uKeyPressed)
{
	ALBUMVIEWDATA *albumViewData = UserPrivate;
	if(uKeyPressed == VK_AP_MENU)
	{
		// ȷ��ɾ��
		albumViewData->delete_all_confirm = 1;
	}
	XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0); //�����ͼƬԤ������ͼƬ�б�OSD_LAYER_0����ͼƬ����
	// ��alert����
	XM_PullWindow (0);
}

static VOID AlbumViewOnCommand (XMMSG *msg)
{
	ALBUMVIEWDATA *albumViewData;
	albumViewData = (ALBUMVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumView));
	if(albumViewData == NULL)
		return;

	XM_printf ("AlbumViewOnCommand msg->wp=%d\n", msg->wp);
#if 0
	switch (msg->wp)
	{
		case ALBUMVIEW_COMMAND_RETURN:
			XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0); //�����ͼƬԤ������ͼƬ�б�OSD_LAYER_0����ͼƬ����
			// ���ص�����
			XM_PullWindow (0);
			break;

		case ALBUMVIEW_COMMAND_DELETE_ALL:
		{
			// ɾ��ȫ��ͼƬ
			DWORD dwButtonNormalTextID[2];
			DWORD dwButtonPressedTextID[2];
			dwButtonNormalTextID[0] = AP_ID_VIDEO_BUTTON_DELETE_YES;
			dwButtonPressedTextID[0] = AP_ID_VIDEO_BUTTON_DELETE_YES;
			dwButtonNormalTextID[1] = AP_ID_VIDEO_BUTTON_CANCEL_NO;
			dwButtonPressedTextID[1] = AP_ID_VIDEO_BUTTON_CANCEL_NO;

			albumViewData->delete_all_confirm = 0;

			XM_OpenAlertView (AP_ID_VIDEO_INFO_TO_DELETE_ALL_PHOTO,
										AP_NULLID,
										2,
										dwButtonNormalTextID,
										dwButtonPressedTextID,
										APP_ALERT_BKGCOLOR,
										(float)20.0,		// �Զ��ر�ʱ��
										APP_ALERT_BKGALPHA,
										DeleteAllCallback,			// �����ص�����
										albumViewData,
										XM_VIEW_ALIGN_CENTRE,		// ���ж���
										XM_ALERTVIEW_OPTION_ENABLE_CALLBACK		// ALERTVIEW��ͼ�Ŀ���ѡ��
										);
			break;
		}
		
		case ALBUMVIEW_COMMAND_DELETE_ONE:
		{
			// ɾ����ǰͼƬ
			DWORD dwButtonNormalTextID[2];
			DWORD dwButtonPressedTextID[2];
			dwButtonNormalTextID[0] = AP_ID_VIDEO_BUTTON_DELETE_YES;
			dwButtonPressedTextID[0] = AP_ID_VIDEO_BUTTON_DELETE_YES;
			dwButtonNormalTextID[1] = AP_ID_VIDEO_BUTTON_CANCEL_NO;
			dwButtonPressedTextID[1] = AP_ID_VIDEO_BUTTON_CANCEL_NO;

			albumViewData->delete_one_confirm = 0;

			XM_OpenAlertView (AP_ID_VIDEO_INFO_TO_DELETE_PHOTO,
										AP_NULLID,
										2,
										dwButtonNormalTextID,
										dwButtonPressedTextID,
										APP_ALERT_BKGCOLOR,
										(float)20.0,		// �Զ��ر�ʱ��
										APP_ALERT_BKGALPHA,
										DeleteOneCallback,			// �����ص�����
										albumViewData,
										XM_VIEW_ALIGN_CENTRE,		// ���ж���
										XM_ALERTVIEW_OPTION_ENABLE_CALLBACK		// ALERTVIEW��ͼ�Ŀ���ѡ��
										);
			break;
		}

		default:
			break;
	}
#endif
}

// �����޲���������������, ȱʡΪ3��
static VOID AlbumViewOnTimer (XMMSG *msg)
{
	ALBUMVIEWDATA *albumViewData;
	albumViewData = (ALBUMVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumView));
	if(albumViewData == NULL)
		return;
	if(!albumViewData->button_hide)
	{
		XM_KillTimer (XMTIMER_ALBUMLISTVIEW);
		albumViewData->button_hide = 1;
		albumViewData->photo_title_hide = 1;

		AP_ButtonControlSetFlag (&albumViewData->btnControl,
					XMBUTTON_FLAG_HIDE);
		XM_InvalidateWindow ();

		//XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_1), (1 << XM_OSD_LAYER_1) );
		//XM_lcdc_osd_set_enable (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1, 0);
		//XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_1), 0);
	}
	else if(!albumViewData->photo_title_hide)
	{
		XM_KillTimer (XMTIMER_ALBUMLISTVIEW);
		albumViewData->photo_title_hide = 1;		// ������Ƭ�ı���
		XM_InvalidateWindow ();
	}
}

static VOID AlbumViewOnSystemEvent (XMMSG *msg)
{
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:			// SD���γ��¼�
			XM_BreakSystemEventDefaultProcess (msg);		// ������ȱʡ����
			XM_JumpWindowEx(XMHWND_HANDLE(Desktop), 0, 0);
			break;
	}
}

// �û�˽����Ϣ����
static VOID AlbumViewOnUserMessage (XMMSG *msg)
{
#if 0
	if(msg->wp == ALBUMVIEW_COMMAND_RETURN)
	{
		// ���ص��������Ӵ�
		XM_PullWindow (0);
	}
	else if(msg->wp == ALBUMVIEW_COMMAND_PAINT)
	{
		XM_InvalidateWindow ();
	}
#endif
}


// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (AlbumView)
	XM_ON_MESSAGE (XM_PAINT, AlbumViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, AlbumViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, AlbumViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, AlbumViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, AlbumViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, AlbumViewOnCommand)
	XM_ON_MESSAGE (XM_TIMER, AlbumViewOnTimer)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, AlbumViewOnSystemEvent)
	XM_ON_MESSAGE (XM_USER, AlbumViewOnUserMessage)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, AlbumView, 0, 0, 0, 0, HWND_VIEW)
