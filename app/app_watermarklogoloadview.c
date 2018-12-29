//****************************************************************************
//
//	Copyright (C) 2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_watermarklogoloadview.c
//	  ��־ˮӡ����
//
//	Revision history
//
//		2014.03.17	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include <common_string.h>
#include <stdio.h>
#include <xm_osd_framebuffer.h>
#include <xm_heap_malloc.h>

#include "app_menuoptionview.h"
#include "app_menuid.h"
#include "xm_app_menudata.h"

// ˽�������
#define	WATERMARKLOGOLOADVIEW_COMMAND_MODIFY		0
#define	WATERMARKLOGOLOADVIEW_COMMAND_RETURN		1

// �����á����ڰ�ť�ؼ�����
#define	WATERMARKLOGOLOADVIEWBTNCOUNT	2
static const XMBUTTONINFO videoRecordBtn[WATERMARKLOGOLOADVIEWBTNCOUNT] = {
	{	
		VK_AP_MENU,		WATERMARKLOGOLOADVIEW_COMMAND_MODIFY,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
	},
	{	
		VK_AP_MODE,		WATERMARKLOGOLOADVIEW_COMMAND_RETURN,	AP_ID_COMMON_MODE,	AP_ID_BUTTON_RETURN
	},
};

#define	MAX_LOGO_PNG		32		// ���֧��32��PNG�ļ�
#define	MAX_LOGO_NAME		32		// �ļ����ȫ·��������

typedef struct tagWATERMARKLOGOLOADVIEWDATA {
	int					nTopItem;					// ��һ�����ӵĲ˵���
	int					nCurItem;					// ��ǰѡ��Ĳ˵���
	int					nItemCount;					// �˵������

	XMBUTTONCONTROL	btnControl;

	char		chPngImageFile[MAX_LOGO_PNG][MAX_LOGO_NAME];

} WATERMARKLOGOLOADVIEWDATA;


static const char * png_path[] = {
#ifdef _WINDOWS
	"g:\\",
	"g:\\logo\\"
#else
	"\\",
	"\\logo\\"
#endif
};

static void append_png_file (WATERMARKLOGOLOADVIEWDATA *settingViewData, char *filename)
{
	if(strlen(filename) >= MAX_LOGO_NAME)
		return;

	if(settingViewData->nItemCount < MAX_LOGO_PNG)
	{
		strcpy (settingViewData->chPngImageFile[settingViewData->nItemCount], filename);
		settingViewData->nItemCount ++;
	}
}

// ɨ���Ŀ¼/logoĿ¼
static void scan_logo_png_image (WATERMARKLOGOLOADVIEWDATA *settingViewData)
{
	char path[32];
	char filename[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	char png_pathname[XM_MAX_FILEFIND_NAME];
	XMFILEFIND fileFind;
	int ch = 0;
	for (ch = 0; ch < sizeof(png_path)/sizeof(char *); ch ++)
	{
	#ifdef _WINDOWS
		sprintf (path, "%s*.*", png_path[ch]);
	#else		
		sprintf (path, "%s", png_path[ch]);
	#endif
		if(XM_FindFirstFile (path, &fileFind, filename, XM_MAX_FILEFIND_NAME))
		{
			do
			{
				// ���Ŀ¼������
				DWORD dwFileAttributes = fileFind.dwFileAttributes;
				// ����Ŀ¼���Ƿ����ļ�����
				if((dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == 0x00)
				{
					// ��ͨ�ļ�
					char *ext = strchr (filename, '.');
					if(ext && *(ext+1))
					{
						if(xm_stricmp (ext + 1, "png") == 0)
						{
							sprintf (png_pathname, "%s%s", png_path[ch], filename);
							append_png_file (settingViewData, png_pathname);
						}
					}
				}
			} while(XM_FindNextFile (&fileFind));
			XM_FindClose (&fileFind);
		}
		
		// ɨ����һ����Ƶͨ��·��
	}
}

VOID WaterMarkLogoLoadViewOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
		// ����δ��������һ�ν���
		WATERMARKLOGOLOADVIEWDATA *settingViewData;

		// ����˽�����ݾ��
		settingViewData = XM_calloc (sizeof(WATERMARKLOGOLOADVIEWDATA));
		if(settingViewData == NULL)
		{
			XM_printf ("settingViewData XM_calloc failed\n");
			// ʧ�ܷ��ص������ߴ���
			XM_PullWindow (0);
			return;
		}
		
		// ��ʼ��˽������
		settingViewData->nCurItem = 0;
		settingViewData->nTopItem = 0;

		settingViewData->nItemCount = 0;

		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(WaterMarkLogoLoadView), settingViewData);

		// ��ť�ؼ���ʼ��
		AP_ButtonControlInit (&settingViewData->btnControl, WATERMARKLOGOLOADVIEWBTNCOUNT, 
			XMHWND_HANDLE(WaterMarkLogoLoadView), videoRecordBtn);

		scan_logo_png_image (settingViewData);
	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("WaterMarkLogoLoadView Pull\n");
	}
}

VOID WaterMarkLogoLoadViewOnLeave (XMMSG *msg)
{
	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		WATERMARKLOGOLOADVIEWDATA *settingViewData = (WATERMARKLOGOLOADVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(WaterMarkLogoLoadView));
		// �ͷ����з������Դ
		if(settingViewData)
		{
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&settingViewData->btnControl);
			// �ͷ�˽�����ݾ��
			XM_free (settingViewData);
		}
		XM_printf ("WaterMarkLogoLoadView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("WaterMarkLogoLoadView Push\n");
	}
}



VOID WaterMarkLogoLoadViewOnPaint (XMMSG *msg)
{
	int nItem, nLastItem;
	XMRECT rc, rect;
	int i;
	int x, y;
	int image_x, image_y;	// ͼƬ��ʾ���������
	unsigned int width, height;		// PNG��ʾ����ĳߴ���Ϣ
	APPROMRES *AppRes;
	float scale_factor;		// ˮƽ��������
	HANDLE hWnd = XMHWND_HANDLE(WaterMarkLogoLoadView);

	XMCOORD menu_name_x, menu_data_x, menu_flag_x;	// �˵�����⡢��ֵ����ʶ��x����

	WATERMARKLOGOLOADVIEWDATA *settingViewData = (WATERMARKLOGOLOADVIEWDATA *)XM_GetWindowPrivateData (hWnd);
	if(settingViewData == NULL)
		return;

	XM_GetDesktopRect (&rc);

	// ����ˮƽ��������(UI��320X240������)
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);

	XM_FillRect (hWnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_WINDOW));
	menu_name_x = (XMCOORD)(APP_POS_ITEM5_MENUNAME_X * scale_factor);
	menu_data_x = (XMCOORD)(APP_POS_ITEM5_MENUDATA_X * scale_factor);
	menu_flag_x = (XMCOORD)(APP_POS_ITEM5_MENUFLAG_X * scale_factor);
	// --------------------------------------
	//
	// ********* 1 ��ʾ����������Ϣ *********
	//
	// --------------------------------------
	AP_DrawTitlebarControl (hWnd, AP_NULLID, AP_ID_WATERMARKSETTING_LOGO_LOGOPNGLOAD_TITLE);

	// ��ʾ�ָ���
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMSPLITLINE);
	rect = rc;
	for (i = 0; i < 3; i++)
	{
		rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * scale_factor);
		rect.top = (XMCOORD)(APP_POS_MENU_Y + (i + 1) * APP_POS_ITEM6_LINEHEIGHT);
		XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), 
			AppRes->res_length, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	}

	// ��ʾѡ�����
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMBACKGROUND6);
	rect = rc;
	rect.left = 0;
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (settingViewData->nCurItem - settingViewData->nTopItem) * APP_POS_ITEM6_LINEHEIGHT);
	XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	// ��ʾĿ¼��
	rect = rc;
	rect.top = APP_POS_ITEM5_MENUNAME_Y;
	x = (XMCOORD)(menu_name_x + 14 * 8 * scale_factor);
	y = (XMCOORD)(APP_POS_MENU_Y + 5);

	image_x = 0;
	image_y = y + APP_POS_ITEM6_LINEHEIGHT * 3;

	width = (XMCOORD)(320.0 * scale_factor - image_x);
	height = APP_POS_ITEM6_LINEHEIGHT * 3; 

	nLastItem = settingViewData->nItemCount;
	if(nLastItem > (settingViewData->nTopItem + 3))
		nLastItem = (settingViewData->nTopItem + 3);
	for (nItem = settingViewData->nTopItem; nItem < nLastItem; nItem ++)
	{
		char title[32];
		char *name;
		XM_IMAGE *lpImage = NULL;
		XM_IMAGE *lpScaleImage = NULL;
		UINT ScaleRatio;

		strcpy (title, settingViewData->chPngImageFile[nItem]);
		name = strchr (title, '\\');
		name ++;

		AP_TextOutDataTimeString (hWnd, menu_name_x, y, name, strlen(name));

		// ����PNG����ʾ
		if(nItem == settingViewData->nCurItem)
		{
			lpImage = XM_ImageCreateFromFile (settingViewData->chPngImageFile[nItem], XM_OSD_LAYER_FORMAT_ARGB888);
			if(lpImage)
			{
				ScaleRatio = (lpImage->width + width - 1) / width;
				if( ((lpImage->height + height - 1) / height) > ScaleRatio)
					ScaleRatio = (lpImage->height + height - 1) / height;
				lpScaleImage = XM_ImageScale (lpImage, ScaleRatio);

				if(lpScaleImage)
				{

					XM_ImageDisplay (lpScaleImage, 
										hWnd,
										image_x + (width - lpScaleImage->width)/2 , 
										image_y + (height - lpScaleImage->height)/2
										);
				}
			}
		}

		y = (XMCOORD)(y + APP_POS_ITEM6_LINEHEIGHT);

		if(lpScaleImage)
			XM_ImageDelete (lpScaleImage);
		if(lpImage)
			XM_ImageDelete (lpImage);
	}

	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
	AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);

}

#define	PNG_FILE_ITEM_COUNT		3

VOID WaterMarkLogoLoadViewOnKeyDown (XMMSG *msg)
{
	WATERMARKLOGOLOADVIEWDATA *settingViewData = (WATERMARKLOGOLOADVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(WaterMarkLogoLoadView));
	if(settingViewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
		case VK_AP_SWITCH:	// �����л���
			// ��"¼�����"�����У�MENU��Power��Switch��MODE��������Ϊ��ť������
			// �˴������ĸ����¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;

		case VK_AP_UP:		// ����¼���
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			settingViewData->nCurItem --;
			if(settingViewData->nCurItem < 0)
			{
				// �۽������һ��
				settingViewData->nCurItem = (WORD)(settingViewData->nItemCount - 1);
				settingViewData->nTopItem = 0;
				while ( (settingViewData->nCurItem - settingViewData->nTopItem) >= PNG_FILE_ITEM_COUNT )
				{
					settingViewData->nTopItem ++;
				}
			}
			else
			{
				if(settingViewData->nTopItem > settingViewData->nCurItem)
					settingViewData->nTopItem = settingViewData->nCurItem;
			}

			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_DOWN:	// 
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);

			settingViewData->nCurItem ++;	
			if(settingViewData->nCurItem >= settingViewData->nItemCount)
			{
				// �۽�����һ��
				settingViewData->nTopItem = 0;
				settingViewData->nCurItem = 0;
			}
			else
			{
				while ( (settingViewData->nCurItem - settingViewData->nTopItem) >= PNG_FILE_ITEM_COUNT )
				{
					settingViewData->nTopItem ++;
				}
			}
			
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();

			break;
	}

}

VOID WaterMarkLogoLoadViewOnKeyUp (XMMSG *msg)
{
	WATERMARKLOGOLOADVIEWDATA *settingViewData;
	settingViewData = (WATERMARKLOGOLOADVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(WaterMarkLogoLoadView));
	if(settingViewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
		case VK_AP_SWITCH:
			// ��"ʱ������"�����У�MENU��MODE��������Ϊ��ť������
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;
	}
}


VOID WaterMarkLogoLoadViewOnCommand (XMMSG *msg)
{
	WATERMARKLOGOLOADVIEWDATA *settingViewData;
	int	SelectedItem;
	void *fp;
	char *lpPngBuffer;
	int cbPngBuffer;
	int ret;
	settingViewData = (WATERMARKLOGOLOADVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(WaterMarkLogoLoadView));
	if(settingViewData == NULL)
		return;

	SelectedItem = settingViewData->nCurItem;// + settingViewData->nTopItem;

	switch(msg->wp)
	{
		case WATERMARKLOGOLOADVIEW_COMMAND_MODIFY:

			// ��PNGͼƬд�뵽�ڲ��洢��
			fp = NULL;
			lpPngBuffer = NULL;
			cbPngBuffer = 0;
			ret = -1;
			do
			{
				fp = XM_fopen (settingViewData->chPngImageFile[settingViewData->nCurItem], "rb");
				if(fp == NULL)
				{
					XM_printf ("Logo PNG SAVE NG, XM_fopen(%s) failed\n", 
						settingViewData->chPngImageFile[settingViewData->nCurItem]);
					break;
				}

				cbPngBuffer = XM_filesize (fp);
				if(cbPngBuffer == 0 || cbPngBuffer > MAX_FLAG_PNG_SIZE)
				{
					XM_printf ("Logo PNG SAVE NG, file size(%d) illegal\n", cbPngBuffer);
					break;
				}

				lpPngBuffer = XM_heap_malloc (cbPngBuffer);
				if(lpPngBuffer == NULL)
				{
					XM_printf ("Logo PNG SAVE NG, xm_heap_malloc (%d bytes) failed\n", cbPngBuffer);
					break;
				}

				if( (int)XM_fread (lpPngBuffer, 1, cbPngBuffer, fp) != cbPngBuffer)
				{
					XM_printf ("Logo PNG SAVE NG, XM_fwrite failed\n");
					break;
				}

				if(XM_FlagWaterMarkPngImageSave (lpPngBuffer, cbPngBuffer) < 0)
				{
					XM_printf ("Logo PNG SAVE NG, XM_FlagPngSave failed\n");
					break;
				}
				XM_printf ("Logo PNG SAVE OK\n");
				ret = 0;
				break;
			} while(fp);

			if(fp)
			{
				XM_fclose (fp);
				fp = NULL;
			}
			if(lpPngBuffer)
			{
				XM_heap_free (lpPngBuffer);
				lpPngBuffer = NULL;
			}
			cbPngBuffer = 0;

			if(ret == 0)
			{
				AP_SetMenuItem (APPMENUITEM_FLAG_STAMP_UPDATE, 1);
#if 0
				//
				unsigned int osd_width = XM_lcdc_osd_get_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
				unsigned int osd_height = XM_lcdc_osd_get_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
				// ����һ��YUV���OSD framebuffer����
				xm_osd_framebuffer_t yuv_framebuffer = XM_osd_framebuffer_create (
					XM_LCDC_CHANNEL_0,
					XM_OSD_LAYER_0,
					XM_OSD_LAYER_FORMAT_YUV420,
					osd_width, osd_height,
					0,
					0,		// copy_last_framebuffer_context
					0
					);
				if(yuv_framebuffer)
				{
					// ����־ˮӡ��ϵ�framebuffer����
					XM_BlendFlagWaterMarkImage (
						yuv_framebuffer->address,
						yuv_framebuffer->width,
						yuv_framebuffer->height,
						yuv_framebuffer->stride);

					// �ر�YUV framebuffer����ʹ����ʾ
					XM_osd_framebuffer_close (yuv_framebuffer, 0);

				}
#endif
			}

			break;


		case WATERMARKLOGOLOADVIEW_COMMAND_RETURN:
			// ���ص�����
			XM_PullWindow (0);
			break;

	}
}

// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (WaterMarkLogoLoadView)
	XM_ON_MESSAGE (XM_PAINT, WaterMarkLogoLoadViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, WaterMarkLogoLoadViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, WaterMarkLogoLoadViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, WaterMarkLogoLoadViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, WaterMarkLogoLoadViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, WaterMarkLogoLoadViewOnCommand)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, WaterMarkLogoLoadView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
