//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_okcancel_view.c
//	  ��ʾ��Ϣ�Ӵ�
//		(����ʾ��ʾ��Ϣ���ɶ�����ֹ��ʱ���Զ��رգ�һ��ȷ�ϰ�ť)
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"

#define	OKCANCEL_MAX_BUTTON	2		// ���2�������˵�		

#define	OKCANCEL_V_SPACE		15		// ��ֱ����߾�
#define	OKCANCEL_H_SPACE		10		// ˮƽ����߾�

#define	OKCANCEL_BACKGROUND_COLOR		0xFFFFFFFF

typedef struct tagOKCANCELVIEWDATA {
	DWORD				dwTextInfoID;					// �ı���Ϣ��ԴID
	DWORD				dwImageID;						// ͼƬ��Ϣ��ԴID
	float				fAutoCloseTime;				// �Ӵ���ʱ���Զ��ر�ʱ�䡣0��ʾ��ֹ�Զ��رչ���
															//		�Զ��ر�Ĭ��ѡ�� XM_COMMAND_OK
	DWORD				dwButtonCount;					// ��ť����
	DWORD				dwButtonNormalID[OKCANCEL_MAX_BUTTON];		//	��ť��ԴID
	DWORD				dwButtonPressedID[OKCANCEL_MAX_BUTTON];	//	��ť������ԴID
	float				fViewAlpha;						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
	DWORD				dwAlignOption;					//	��ͼ����ʾ����������Ϣ
	DWORD 				dwOption;

	DWORD				dwBkgColor;						// ����ɫ

	FPOKCANCELCB		okcancelcb;						// �û��ṩ�İ����˵��ص�����

	// �ڲ�״̬
	XMSIZE				sizeTextInfo;	
	XMSIZE				sizeImage;	
	XMSIZE				sizeButton[OKCANCEL_MAX_BUTTON];
	XMSIZE				sizeClient;
	DWORD				dwTimeout;						// ����������ʱ����
	int					btnPressed[OKCANCEL_MAX_BUTTON];
} OKCANCELVIEWDATA;

// OKCANCEL��Ϣ��ʾ�Ӵ�
//
//	----- ��ͼ����ģʽ֧�� ( XM_VIEW_ALIGN_CENTRE �� XM_VIEW_ALIGN_BOTTOM ) -----
//
//	1) ��ֱ����/ˮƽ������ʾ XM_VIEW_ALIGN_CENTRE
//	2) �ײ�����/ˮƽ������ʾ XM_VIEW_ALIGN_BOTTOM
//
//  ---- UIԪ�ز���(Layout) ----
//
// 1) ��ʾ��������Ϣ���ݡ���ͼƬ��Ϣ���ݡ�������ť��
//		�հ���
//		������Ϣ����
//		�հ���
//		ͼƬ��Ϣ����
//		�հ���
//		��ť OK
//		�հ���
//		��ť Cancel (������)
//		�հ���
//

// ��ʾ��Ϣ��ͼ
// 0 ��ͼ����������ͼ��ʾʧ��
// 1 ��ͼ��������ʾ
XMBOOL XM_OpenOkCancelView ( 
								 DWORD dwInfoTextID,				// ��Ϣ�ı���ԴID
																		//		��0ֵ��ָ����ʾ������Ϣ����ԴID
								 DWORD dwImageID,					// ͼƬ��Ϣ��ԴID
																		//		��0ֵ��ָ����ʾͼƬ��Ϣ����ԴID	

								 DWORD dwButtonCount,			// ��ť������1����2
																		//		������һ����ťʱ��
																		//		OK��ť��ʹ��VK_F1(Menu)
																		//		Cancel������ʹ��VK_F2(Mode)
								 DWORD dwButtonNormalID[],		//	��ť��ԴID
																		//		0 
																		//			��ʾû�ж���Button��OkCancel��Ϊһ����Ϣ��ʾ���ڡ�
																		//			���������ʱ���Զ��ر�
																		//		����ֵ
																		//			��ʾButton������Ϣ����ԴID��OkCancel��Ҫ��ʾһ����ť
								 DWORD dwButtonPressedID[],	//	��ť������ԴID
																		//		0
																		//			��ʾû�ж������Դ��
																		//			��ť����ʱʹ��dwButtonNormalID��Դ
																		//			��û���ṩ��ť����Ч����ͼƬ������ͼƬ���ȣ��䰵�������ɰ���Ч��
																		//		����ֵ
																		//			��ʾButton����ʱ������Ϣ����ԴID
								 DWORD dwBackgroundColor,		// ����ɫ
																		//		0
																		//			��ʾʹ��ȱʡ����ɫ
																		//		����ֵ
																		//			��ʾ��Ч��䱳��ɫ������ɫ��ָ��Alpha����
								 float fAutoCloseTime,			//	ָ���Զ��ر�ʱ�� (�뵥λ)��
																		//		0.0	��ʾ��ֹ�Զ��ر�
								 float fViewAlpha,				// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
																		//		0.0	��ʾȫ͸
																		//		1.0	��ʾȫ����
								 FPOKCANCELCB okcancelcb,				// �����ص�����

								 DWORD dwAlignOption,			//	��ͼ����ʾ����������Ϣ
																		//		���OSD��ʾ��ԭ��(OSD��Ч��������Ͻ�)
																		//		XM_VIEW_ALIGN_CENTRE	
																		//			�Ӵ����ж���(���OSD��ʾ����)
																		//		XM_VIEW_ALIGN_BOTTOM
																		//			�Ӵ��ײ����ж���(���OSD��ʾ����)
								 DWORD dwOption					// OKCANCEL��ͼ�Ŀ���ѡ��
																		//		XM_OKCANCEL_OPTION_ENABLE_POPTOPVIEW
																		//			����ջ���Ӵ�ʹ��
								 )
{
	OKCANCELVIEWDATA *viewData;

	// �������

	// ������������Ϊ1����2
	if(dwButtonCount != 1 && dwButtonCount != 2)
	{
		XM_printf ("XM_OpenOkCancelView NG, dwButtonCount(%d) must be 1 or 2\n", dwButtonCount);
		return 0;
	}

	if(dwInfoTextID == 0)
	{
		XM_printf ("XM_OpenOkCancelView NG, dwInfoTextID(%d) must be non-zero\n", dwInfoTextID);
		return 0;
	}
	if(dwAlignOption != XM_VIEW_ALIGN_CENTRE && dwAlignOption != XM_VIEW_ALIGN_BOTTOM)
	{
		XM_printf ("XM_OpenOkCancelView NG, illegal dwAlignOption(%d)\n", dwAlignOption);
		return 0;
	}
	if(fViewAlpha < 0.0 || fViewAlpha > 1.0)
	{
		XM_printf ("XM_OpenOkCancelView NG, illegal fViewAlpha(%f), should be 0.0 ~ 1.0\n", fViewAlpha);
		return 0;
	}
	// �Զ��ر�ʱ�估������Դ����ͬʱΪ0
	if(fAutoCloseTime == 0.0 && dwButtonNormalID == 0)
	{
		XM_printf ("XM_OpenOkCancelView NG, both dwAutoCloseTime & dwButtonNormalID set to 0\n");
		return 0;
	}


	if(dwButtonCount > OKCANCEL_MAX_BUTTON)
	{
		XM_printf ("XM_OpenOkCancelView NG, dwButtonCount(%d) exceed maximum button count(%d)\n", 
						dwButtonCount,
						OKCANCEL_MAX_BUTTON);
		return 0;
	}
	else if(dwButtonCount)
	{
		unsigned int i;
		if(dwButtonNormalID == NULL)
		{
			XM_printf ("XM_OpenOkCancelView NG, dwButtonNormalID is NULL\n");
			return 0;
		}
		for (i = 0; i < dwButtonCount; i++)
		{
			if(dwButtonNormalID[i] == 0)
			{
				XM_printf ("XM_OpenOkCancelView NG, dwButtonNormalID[%d] is NULL\n", i);
				return 0;
			}
		}
	}
	
	viewData = XM_calloc (sizeof(OKCANCELVIEWDATA));
	if(viewData == NULL)
		return 0;
	viewData->dwTextInfoID = dwInfoTextID;
	viewData->dwImageID = dwImageID;
	viewData->fAutoCloseTime = fAutoCloseTime;
	viewData->dwButtonCount = dwButtonCount;
	memcpy (viewData->dwButtonNormalID, dwButtonNormalID, dwButtonCount * sizeof(DWORD));
	if(dwButtonPressedID)
	{
		memcpy (viewData->dwButtonPressedID, dwButtonPressedID, dwButtonCount * sizeof(DWORD));
	}
	viewData->dwBkgColor = dwBackgroundColor;
	viewData->fViewAlpha = fViewAlpha;
	viewData->okcancelcb = okcancelcb;
	viewData->dwAlignOption = dwAlignOption;
	viewData->dwOption = dwOption;
	memset (viewData->btnPressed, 0, sizeof(viewData->btnPressed));

	if(dwOption & XM_OKCANCEL_OPTION_ENABLE_POPTOPVIEW)
	{
		// ����ջ����ͼ
		if(XM_JumpWindowEx (XMHWND_HANDLE(OkCancelView), (DWORD)viewData, XM_JUMP_POPTOPVIEW) == 0)
		{
			XM_free (viewData);
			return 0;
		}
	}
	else
	{
		if(XM_PushWindowEx (XMHWND_HANDLE(OkCancelView), (DWORD)viewData) == 0)
		{
			XM_free (viewData);
			return 0;
		}
	}
	return 1;
}

VOID OkCancelViewOnEnter (XMMSG *msg)
{
	OKCANCELVIEWDATA *viewData;
	if(msg->wp == 0)
	{
		int x, y, w, h;
		APPROMRES *AppRes;
		XMSIZE sizeInfo;
		XMSIZE sizeImage;
		XMSIZE sizeButton[OKCANCEL_MAX_BUTTON];
		XMRECT rectDesktop;
		// ��һ�ν���(����׼������)
		viewData = (OKCANCELVIEWDATA *)msg->lp;

		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(OkCancelView), viewData);

		// �����Ӵ���alphaֵ
		XM_SetWindowAlpha (XMHWND_HANDLE(OkCancelView), (unsigned char)(viewData->fViewAlpha * 255));

		// ���㴰�ڵ�λ����Ϣ (����UIԪ�ش�С������ģʽ����)
		XM_GetDesktopRect (&rectDesktop);
		w = 0;
		h = 0;
		sizeInfo.cx = 0;
		sizeInfo.cy = 0;
		sizeImage.cx = 0;
		sizeImage.cy = 0;
		memset (sizeButton, 0, sizeof(sizeButton));
		//sizeButton.cx = 0;
		//sizeButton.cy = 0;
		//	1) ������Ϣ�ı�����ʾ�����С
		AppRes = AP_AppRes2RomRes (viewData->dwTextInfoID);
		if(AppRes)
		{
			XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &sizeInfo);
		}
		//	2) ����ͼƬ��Ϣ����ʾ�����С
		if(viewData->dwImageID)
		{
			AppRes = AP_AppRes2RomRes (viewData->dwImageID);
			if(AppRes)
			{
				XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &sizeImage);
			}
		}
		// 3) ����Button����ʾ�����С
		if(viewData->dwButtonCount)
		{
			// ��Ҫ��ʾButton
			unsigned int i;
			for (i = 0; i < viewData->dwButtonCount; i ++)
			{
				AppRes = AP_AppRes2RomRes (viewData->dwButtonNormalID[i]);
				if(AppRes)
				{
					XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &sizeButton[i]);
				}
			}
		}

		viewData->sizeTextInfo = sizeInfo;
		viewData->sizeImage = sizeImage;
		memcpy (viewData->sizeButton, sizeButton, sizeof(sizeButton));

		if(viewData->dwAlignOption == XM_VIEW_ALIGN_CENTRE)
		{
			// view��ֱ����/ˮƽ������ʾ XM_VIEW_ALIGN_CENTRE
			h = OKCANCEL_V_SPACE;
			w = 0;
			// �ۼơ��ı���Ϣ���ݡ�
			h += sizeInfo.cy + OKCANCEL_V_SPACE;
			if(w < sizeInfo.cx)
				w = sizeInfo.cx;
			// �ۼơ�ͼƬ��Ϣ���ݡ�
			h += sizeImage.cy + OKCANCEL_V_SPACE;
			if(w < sizeImage.cx)
				w = sizeImage.cx;
			// �ۼ� "��ť"
			if(viewData->dwButtonCount)
			{
				unsigned int i;
				for (i = 0; i < viewData->dwButtonCount; i ++)
				{
					h += sizeButton[i].cy + OKCANCEL_V_SPACE;
					if(w < sizeButton[i].cx)
						w = sizeButton[i].cx;
				}
			}
			w += OKCANCEL_H_SPACE + OKCANCEL_H_SPACE;

			/*
			if(viewData->dwButtonNormalID)
			{
				// ��ʾ����Ϣ���ݡ�������ť��
				//	�Զ�����UIԪ�ع���
				//	(�հ���+����Ϣ���ݡ�+ �հ��� + "����ʱ" + �հ�������ť�� + �հ���)
				h = OKCANCEL_V_SPACE + sizeInfo.cy + OKCANCEL_V_SPACE + sizeButton.cy + OKCANCEL_V_SPACE;
				w = sizeInfo.cx;
				if(sizeInfo.cx < sizeButton.cx)
					w = sizeButton.cx;
				w += OKCANCEL_H_SPACE + OKCANCEL_H_SPACE;
			}
			else
			{
				// ����ʾ������Ϣ
				//	�Զ�����UIԪ�ع���
				// (�հ���+����Ϣ���ݡ�+ �հ���)
				h = OKCANCEL_V_SPACE + sizeInfo.cy + OKCANCEL_V_SPACE;
				w = OKCANCEL_H_SPACE + sizeInfo.cx + OKCANCEL_H_SPACE;
			}*/
			
			if(h > (rectDesktop.bottom - rectDesktop.top + 1))
				h = (rectDesktop.bottom - rectDesktop.top + 1);
			if(w > (rectDesktop.right - rectDesktop.left + 1))
				w = (rectDesktop.right - rectDesktop.left + 1);

			x = (rectDesktop.right - rectDesktop.left + 1 - w) / 2;
			y = (rectDesktop.bottom - rectDesktop.top + 1 - h) / 2;
		}
		else //if(viewData->dwAlignOption == XM_VIEW_ALIGN_BOTTOM)
		{
			/*
			// view�ײ�����/ˮƽ������ʾ XM_VIEW_ALIGN_BOTTOM
			if(viewData->dwButtonNormalID)
			{
				// ��ʾ����Ϣ���ݡ�������ť��
				//	�Զ�����UIԪ�ع���
				//	(�հ���+����Ϣ���ݡ�+ �հ��� + ����ť�� + �հ���)
				h = OKCANCEL_V_SPACE + sizeInfo.cy + OKCANCEL_V_SPACE + sizeButton.cy + OKCANCEL_V_SPACE;
			}
			else
			{
				// ����ʾ������Ϣ
				//	�Զ�����UIԪ�ع���
				// (�հ���+����Ϣ���ݡ�+ �հ���)
				h = OKCANCEL_V_SPACE + sizeInfo.cy + OKCANCEL_V_SPACE;
			}*/

			h = OKCANCEL_V_SPACE;
			w = 0;
			// �ۼơ�������Ϣ���ݡ�
			h += sizeInfo.cy + OKCANCEL_V_SPACE;
			if(w < sizeInfo.cx)
				w = sizeInfo.cx;
			// �ۼơ�ͼƬ��Ϣ���ݡ�
			h += sizeImage.cy + OKCANCEL_V_SPACE;
			if(w < sizeImage.cx)
				w = sizeImage.cx;
			// �ۼ� "��ť"
			if(viewData->dwButtonCount)
			{
				unsigned int i;
				for (i = 0; i < viewData->dwButtonCount; i ++)
				{
					h += sizeButton[i].cy + OKCANCEL_V_SPACE;
					if(w < sizeButton[i].cx)
						w = sizeButton[i].cx;
				}
			}

			if(h > (rectDesktop.bottom - rectDesktop.top + 1))
				h = (rectDesktop.bottom - rectDesktop.top + 1);

			x = 0;
			w = (rectDesktop.right - rectDesktop.left + 1);
			y = rectDesktop.bottom - h + 1;
		}

		viewData->sizeClient.cx = w;
		viewData->sizeClient.cy = h;
			
		//XM_SetWindowPos (XMHWND_HANDLE(OkCancelView), x, y, w, h);
	}
	else
	{
		viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(OkCancelView));
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("OkCancelView Pull\n");
	}

	// ������ʱ��
	if(viewData->fAutoCloseTime != 0.0)
	{
		XM_SetTimer (XMTIMER_MESSAGEVIEW, 100);	// ����100ms�Ķ�ʱ��
		viewData->dwTimeout = (DWORD)(viewData->fAutoCloseTime * 10.0 + 0.5);
	}
}

VOID OkCancelViewOnLeave (XMMSG *msg)
{
	OKCANCELVIEWDATA *viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(OkCancelView));
	// ɾ����ʱ��
	XM_KillTimer (XMTIMER_MESSAGEVIEW);

	if (msg->wp == 0)
	{
		if(viewData)
		{
			// �ͷ�˽�����ݾ��
			XM_free (viewData);
			
		}
		XM_printf ("OkCancelView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("OkCancelView Push\n");
	}
}



VOID OkCancelViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
	APPROMRES *AppRes;
	XMCOLOR bkg_clolor;
	BYTE old_alpha;
	HANDLE hwnd = XMHWND_HANDLE(OkCancelView);
	OKCANCELVIEWDATA *viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(viewData == NULL)
		return;

	// ��ȡ�Ӵ�λ����Ϣ
	XM_GetWindowRect (XMHWND_HANDLE(OkCancelView), &rc);
	if(viewData->dwBkgColor)
		bkg_clolor = viewData->dwBkgColor;
	else
		bkg_clolor = OKCANCEL_BACKGROUND_COLOR;	// ʹ���뱳��ˢ����ͬ����ɫ��Alphaֵ
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, bkg_clolor);

	rect = rc;
	rc.top = rc.top + (rc.bottom - rc.top + 1 - viewData->sizeClient.cy) / 2;
	rc.bottom = rect.bottom - (rect.bottom - rect.top + 1 - viewData->sizeClient.cy) / 2;


	// ��ʾOkCancel��Ϣ
	rect = rc;
	rect.bottom = rect.top + OKCANCEL_V_SPACE + viewData->sizeTextInfo.cy + OKCANCEL_V_SPACE - 1 ;
	AppRes = AP_AppRes2RomRes (viewData->dwTextInfoID);
	if(AppRes)
	{
		// ���������ʾ
		old_alpha = XM_GetWindowAlpha (hwnd);
		XM_SetWindowAlpha (hwnd, 255);

		XM_RomImageDraw (AppRes->rom_offset, 
							AppRes->res_length,
							hwnd, &rect, XMIMG_DRAW_POS_CENTER);
		XM_SetWindowAlpha (hwnd, old_alpha);
	}

	// ��ʾͼƬ��Ϣ
	if(viewData->sizeImage.cy)
	{
		// ����ͼƬ��Ϣ
		rect = rc;
		rect.top += OKCANCEL_V_SPACE + viewData->sizeTextInfo.cy + OKCANCEL_V_SPACE;
		rect.bottom = rect.top + viewData->sizeImage.cy;
		AppRes = AP_AppRes2RomRes (viewData->dwImageID);
		if(AppRes)
		{
			// ���������ʾ
			old_alpha = XM_GetWindowAlpha (hwnd);
			XM_SetWindowAlpha (hwnd, 255);			

			XM_RomImageDraw (AppRes->rom_offset, 
									AppRes->res_length,
									hwnd, &rect, XMIMG_DRAW_POS_CENTER);
			XM_SetWindowAlpha (hwnd, old_alpha);
		}
	}


	// ��ť��ʾ
	if(viewData->dwButtonCount)
	{
		// ��ʾ��ť����
		unsigned int i;
		unsigned char layer_brightness;
		old_alpha = XM_GetWindowAlpha (hwnd);
		XM_SetWindowAlpha (hwnd, 255);			
		rect = rc;
		//rect.top = rect.bottom - (OKCANCEL_H_SPACE + viewData->sizeButton[0].cy + OKCANCEL_H_SPACE) + 1;
		//rect.top = rect.bottom - (OKCANCEL_H_SPACE) + 1;
		rect.top = rect.bottom;
		for (i = 0; i < viewData->dwButtonCount; i ++)
		{
			rect.top -= OKCANCEL_V_SPACE + viewData->sizeButton[i].cy;
			//rect.bottom -= OKCANCEL_H_SPACE + viewData->sizeButton[i].cy;
		}
		
		for (i = 0; i < viewData->dwButtonCount; i ++)
		{
			rect.bottom = rect.top + viewData->sizeButton[i].cy - 1;
			if(viewData->btnPressed[i])
			{
				// ��ʾ���°�ťЧ��
				if(viewData->dwButtonPressedID[i] == 0)
				{
					// û���ṩ��ť����Ч����ͼƬ��ʹ�õ���ͼƬ���ȣ��䰵�������ɰ���Ч��
					AppRes = AP_AppRes2RomRes (viewData->dwButtonNormalID[i]);
					layer_brightness = 128;
				}
				else
				{
					AppRes = AP_AppRes2RomRes (viewData->dwButtonPressedID[i]);
					layer_brightness = 255;
				}
			}
			else
			{
				// ��ʾ�ͷŰ�ťЧ��
				AppRes = AP_AppRes2RomRes (viewData->dwButtonNormalID[i]);
				layer_brightness = 255;
			}
			if(AppRes)
			{
				if(layer_brightness == 255)
				{
					XM_RomImageDraw (AppRes->rom_offset, AppRes->res_length,
									hwnd, &rect, XMIMG_DRAW_POS_CENTER);
				}
				else
				{
					float brightness_ratio = (float)layer_brightness / 255.0f;
					XM_IMAGE *image, *dark_image;
					xm_osd_framebuffer_t framebuffer = XM_GetWindowFrameBuffer (hwnd);
					unsigned int x, y;
					if(framebuffer)
					{
						dark_image = NULL;
						image = XM_RomImageCreate (AppRes->rom_offset,
													AppRes->res_length,
													XM_OSD_LAYER_FORMAT_ARGB888);
						if(image)
						{
							dark_image = XM_ImageCloneWithBrightness (image, brightness_ratio);
							XM_ImageDelete (image);
						}
						if(dark_image)
						{
							x = rect.left + (rect.right - rect.left + 1 - dark_image->width) / 2;
							y = rect.top  + (rect.bottom - rect.top + 1 - dark_image->height) / 2;
							XM_lcdc_copy_raw_image (
										framebuffer->address,
										framebuffer->format,
										framebuffer->width, framebuffer->height,
										framebuffer->stride,
										x, y,
										(unsigned char *)dark_image->image,
										dark_image->width, dark_image->height,
										dark_image->stride,
										0, 0,
										dark_image->width,
										dark_image->height,
										XM_GetWindowAlpha (hwnd)
										);
							XM_ImageDelete (dark_image);
						}
					}
				}
			}

			rect.top += OKCANCEL_V_SPACE + viewData->sizeButton[i].cy;
			//rect.bottom += OKCANCEL_H_SPACE + viewData->sizeButton[i].cy;

		}


		XM_SetWindowAlpha (hwnd, old_alpha);

	}
}


VOID OkCancelViewOnKeyDown (XMMSG *msg)
{
	OKCANCELVIEWDATA *viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(OkCancelView));
	if(viewData == NULL)
		return;
// ������
	XM_Beep (XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
			viewData->btnPressed[0] = 1;
			if(viewData->dwButtonCount > 0)
			{
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			break;

		case VK_AP_MODE:		// �л������г���¼��״̬
			viewData->btnPressed[1] = 1;
			if(viewData->dwButtonCount > 1)
			{
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			break;

		default:
			// �˴������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			break;

	}
}

VOID OkCancelViewOnKeyUp (XMMSG *msg)
{
	OKCANCELVIEWDATA *viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(OkCancelView));
	if(viewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
			if(viewData->dwButtonCount > 0 && viewData->btnPressed[0])
			{
				viewData->btnPressed[0] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();

				XM_PostMessage (XM_COMMAND, XM_COMMAND_OK, 0);
			}
			break;

		case VK_AP_MODE:		// �л������г���¼��״̬
			if(viewData->dwButtonCount > 1 && viewData->btnPressed[1])
			{
				viewData->btnPressed[1] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();

				XM_PostMessage (XM_COMMAND, XM_COMMAND_CANCEL, 0);
			}
			break;


		default:
			// �˴������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			break;
	}
}

VOID OkCancelViewOnCommand (XMMSG *msg)
{
	OKCANCELVIEWDATA *viewData;

	viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(OkCancelView));
	if(viewData == NULL)
		return;

	XM_printf ("OkCancel %d\n", msg->wp);

	// �����ڰ�������ص��������ɻص�����������
	(*viewData->okcancelcb) (msg->wp);
}

VOID OkCancelViewOnTimer (XMMSG *msg)
{
	OKCANCELVIEWDATA *viewData;
	viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(OkCancelView));
	if(viewData == NULL)
		return;
	if(viewData->dwTimeout > 0)
	{
		viewData->dwTimeout --;
	}
	if(viewData->dwTimeout == 0)
	{
		// ���ص��������Ӵ�
		(*viewData->okcancelcb) (XM_COMMAND_OK);
	}
}

// ���������޷�����ϵͳ��Ϣ�¼����˳�������ϵͳȱʡ����
VOID OkCancelViewOnSystemEvent (XMMSG *msg)
{
	// ���ص��������Ӵ�
	OKCANCELVIEWDATA *viewData;
	viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(OkCancelView));
	if(viewData == NULL)
		return;
	if(viewData->dwOption & XM_OKCANCEL_OPTION_SYSTEM_MODEL)
	{
		// ���Բ��������е�ϵͳ�¼�(���β�/��ص�)
		XM_BreakSystemEventDefaultProcess (msg);
		return;
	}
	// ����Ϣ����Ͷ�ݵ�ϵͳ, ��������ǰ�Ӵ�
	XM_BreakSystemEventDefaultProcess (msg);
	XM_PostMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
	XM_PullWindow (0);
}

// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (OkCancelView)
	XM_ON_MESSAGE (XM_PAINT, OkCancelViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, OkCancelViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, OkCancelViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, OkCancelViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, OkCancelViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, OkCancelViewOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, OkCancelViewOnCommand)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, OkCancelViewOnSystemEvent)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, OkCancelView, 1, 0, 0, 255, HWND_VIEW)
