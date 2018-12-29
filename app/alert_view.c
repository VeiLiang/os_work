//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: alert_view.c
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

#define	ALERT_MAX_BUTTON	3		// ���3�������˵�		

#define	ALERT_V_SPACE		30//15		// ��ֱ����߾�
#define	ALERT_H_SPACE		10		// ˮƽ����߾�

#define	ALERT_BACKGROUND_COLOR		0xFFFFFFFF

typedef struct tagALERTVIEWDATA {
	DWORD				dwTextInfoID;					// �ı���Ϣ��ԴID
	DWORD				dwImageID;						// ͼƬ��Ϣ��ԴID
	float				fAutoCloseTime;				// �Ӵ���ʱ���Զ��ر�ʱ�䡣0��ʾ��ֹ�Զ��رչ���
	DWORD				dwButtonCount;					// ��ť����
	DWORD				dwButtonNormalTextID[ALERT_MAX_BUTTON];	//��ť������ԴID
	DWORD				dwButtonPressedTextID[ALERT_MAX_BUTTON];	//��ť����������ԴID
	XMRECT				rectButton[ALERT_MAX_BUTTON];	// ��ť����
	float				fViewAlpha;					// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
	DWORD				dwAlignOption;					//	��ͼ����ʾ����������Ϣ
	DWORD				dwOption;						//	ALERTVIEW��ͼ�Ŀ���ѡ��

	DWORD				dwBkgColor;

	FPALERTCB			alertcb;						// �û��ṩ�İ����˵��ص�����
	VOID *				UserPrivate;					// �û�˽������

	// �ڲ�״̬
	XMSIZE				sizeTextInfo;	
	XMSIZE				sizeImage;	
	XMSIZE				sizeButton[4];
	XMSIZE				sizeCountDown;
	int			        cbCountDownString;	
	DWORD				dwTimeout;					// ����������ʱ����
	int			        btnPressed[4];
} ALERTVIEWDATA;

// ALERT��Ϣ��ʾ�Ӵ�
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
//		����ʱ��
//		�հ���
//		��ť
//		�հ���
//
// 2) ����ʾ����Ϣ���ݡ�
//		�հ���
//		��Ϣ����
//		�հ���

// ��ʾ��Ϣ��ͼ
// 0 ��ͼ����������ͼ��ʾʧ��
// 1 ��ͼ��������ʾ
XMBOOL XM_OpenAlertView ( 
								 DWORD dwInfoTextID,				// ��Ϣ�ı���ԴID
																		//		��0ֵ��ָ����ʾ������Ϣ����ԴID
								 DWORD dwImageID,					// ͼƬ��Ϣ��ԴID
																		//		��0ֵ��ָ����ʾͼƬ��Ϣ����ԴID	

								 DWORD dwButtonCount,			// ��ť����
																		//		������һ����ťʱ��
																		//		��һ����ť��ʹ��VK_F1(Menu)
																		//		�ڶ���������ʹ��VK_F2(Mode)
																		//		������������ʹ��VK_F3(Switch)
								 DWORD dwButtonNormalTextID[],	//	��ť������ԴID
																		//		0 
																		//			��ʾû�ж���Button��Alert��Ϊһ����Ϣ��ʾ���ڡ�
																		//			���������ʱ���Զ��ر�
																		//		����ֵ
																		//			��ʾButton������Ϣ����ԴID��Alert��Ҫ��ʾһ����ť
								 DWORD dwButtonPressedTextID[],	//	��ť����������ԴID
																		//		0
																		//			��ʾû�ж������Դ��
																		//			��ť����ʱʹ��dwButtonNormalTextID��Դ
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
								 FPALERTCB alertcb,				// �����ص�����
								 VOID *UserPrivate,				// �û��������

								 DWORD dwAlignOption,			//	��ͼ����ʾ����������Ϣ
																		//		���OSD��ʾ��ԭ��(OSD��Ч��������Ͻ�)
																		//		XM_VIEW_ALIGN_CENTRE	
																		//			�Ӵ����ж���(���OSD��ʾ����)
																		//		XM_VIEW_ALIGN_BOTTOM
																		//			�Ӵ��ײ����ж���(���OSD��ʾ����)
								 DWORD dwOption					// ALERTVIEW��ͼ�Ŀ���ѡ��
																		//		XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN
																		//			����ʱ��ʾʹ��
																		//		XM_ALERTVIEW_OPTION_ENABLE_KEYDISABLE
																		//			"��ֹ��������"ʹ��
																		//		XM_ALERTVIEW_OPTION_ENABLE_CALLBACK
																		//			ʹ�ܰ����¼��ص�����
								 )
{
	ALERTVIEWDATA *viewData;

	// �������
	if(dwInfoTextID == 0)
	{
		XM_printf ("XM_OpenAlertView NG, dwInfoTextID(%d) must be non-zero\n", dwInfoTextID);
		return 0;
	}
	if(dwAlignOption != XM_VIEW_ALIGN_CENTRE && dwAlignOption != XM_VIEW_ALIGN_BOTTOM)
	{
		XM_printf ("XM_OpenAlertView NG, illegal dwAlignOption(%d)\n", dwAlignOption);
		return 0;
	}
	if(fViewAlpha < 0.0 || fViewAlpha > 1.0)
	{
		XM_printf ("XM_OpenAlertView NG, illegal fViewAlpha(%f), should be 0.0 ~ 1.0\n", fViewAlpha);
		return 0;
	}
	// �Զ��ر�ʱ�估������Դ����ͬʱΪ0
	if(fAutoCloseTime == 0.0 && dwButtonNormalTextID == 0)
	{
		XM_printf ("XM_OpenAlertView NG, both dwAutoCloseTime & dwButtonNormalTextID set to 0\n");
		return 0;
	}
	// XM_ALERTVIEW_OPTION_ENABLE_KEYDISABLE ʹ��ʱ��鶨ʱ������
	if(dwOption & XM_ALERTVIEW_OPTION_ENABLE_KEYDISABLE)
	{
		if(fAutoCloseTime == 0.0)
		{
			XM_printf ("XM_OpenAlertView NG, dwAutoCloseTime can't set to 0.0 when XM_ALERTVIEW_OPTION_ENABLE_KEYDISABLE enable\n");
			return 0;
		}
	}
	// XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN ʹ��ʱ��鶨ʱ������
	if(dwOption & XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN)
	{
		if(fAutoCloseTime == 0.0)
		{
			XM_printf ("XM_OpenAlertView NG, dwAutoCloseTime can't set to 0.0 when XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN enable\n");
			return 0;
		}
	}

	if(dwOption & XM_ALERTVIEW_OPTION_ADJUST_COUNTDOWN)
	{
		if(!(dwOption & XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN))
		{
			XM_printf ("XM_OpenAlertView NG, XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN should enable when XM_ALERTVIEW_OPTION_ADJUST_COUNTDOWN enable\n");
			return 0;
		}
	}

	if((dwOption & XM_ALERTVIEW_OPTION_ENABLE_CALLBACK) && alertcb == NULL)
	{
		XM_printf ("XM_OpenAlertView NG, alertcb can't set to NULL when XM_ALERTVIEW_OPTION_ENABLE_CALLBACK enable\n");
		return 0;
	}

	if(dwButtonCount > ALERT_MAX_BUTTON)
	{
		XM_printf ("XM_OpenAlertView NG, dwButtonCount(%d) exceed maximum button count(%d)\n", 
						dwButtonCount,
						ALERT_MAX_BUTTON);
		return 0;
	}
	else if(dwButtonCount)
	{
		unsigned int i;
		if(dwButtonNormalTextID == NULL)
		{
			XM_printf ("XM_OpenAlertView NG, dwButtonNormalTextID is NULL\n");
			return 0;
		}
		for (i = 0; i < dwButtonCount; i++)
		{
			if(dwButtonNormalTextID[i] == 0)
			{
				XM_printf ("XM_OpenAlertView NG, dwButtonNormalTextID[%d] is NULL\n", i);
				return 0;
			}
		}
	}
	
	viewData = XM_calloc(sizeof(ALERTVIEWDATA));
	if(viewData == NULL)
		return 0;
	viewData->dwTextInfoID = dwInfoTextID;
	viewData->dwImageID = dwImageID;
	viewData->fAutoCloseTime = fAutoCloseTime;
	viewData->dwButtonCount = dwButtonCount;
	memcpy (viewData->dwButtonNormalTextID, dwButtonNormalTextID, dwButtonCount * sizeof(DWORD));
	memcpy (viewData->dwButtonPressedTextID, dwButtonPressedTextID, dwButtonCount * sizeof(DWORD));
	viewData->dwBkgColor = dwBackgroundColor;
	viewData->fViewAlpha = fViewAlpha;
	viewData->alertcb = alertcb;
	viewData->UserPrivate = UserPrivate;
	viewData->dwAlignOption = dwAlignOption;
	memset (viewData->btnPressed, 0, sizeof(viewData->btnPressed));
	viewData->dwOption = dwOption;
	if(XM_PushWindowEx (XMHWND_HANDLE(AlertView), (DWORD)viewData) == 0)
	{
		XM_free (viewData);
		return 0;
	}
	return 1;
}

VOID AlertViewOnEnter (XMMSG *msg)
{
	ALERTVIEWDATA *viewData;

	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnEnter, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);
	if(msg->wp == 0)
	{
		int x, y, w, h;
		APPROMRES *AppRes;
		XMSIZE sizeInfo;
		XMSIZE sizeImage;
		XMSIZE sizeButton[4];
		XMSIZE sizeCountDown;
		XMRECT rectDesktop;
		// ��һ�ν���(����׼������)
		viewData = (ALERTVIEWDATA *)msg->lp;

		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(AlertView), viewData);

		// �����Ӵ���alphaֵ
		XM_SetWindowAlpha (XMHWND_HANDLE(AlertView), (unsigned char)(viewData->fViewAlpha * 255));

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
		sizeCountDown.cx = 0;
		sizeCountDown.cy = 0;
		//	1) ������Ϣ�ı�����ʾ�����С
		AppRes = AP_AppRes2RomRes (viewData->dwTextInfoID);
		if(AppRes)
		{
			XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &sizeInfo);
		}
		//	2) ����ͼƬ��Ϣ����ʾ�����С
		if(viewData->dwImageID && viewData->dwImageID != AP_NULLID)
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
				AppRes = AP_AppRes2RomRes (AP_ID_ALERT_BUTTON_NORMAL);
				if(AppRes)
				{
					XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &sizeButton[i]);
				}
			}
		}
		// 4) ���㵹��ʱ����ʾ�����С
		if(viewData->dwOption & XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN)
		{
			// ����ʱʹ��
			char string[32];
			DWORD count = (DWORD)(viewData->fAutoCloseTime * 10.0 + 0.5);
			sprintf (string, "%02d:%02d", count);
			viewData->cbCountDownString = strlen(string);
			AP_TextGetStringSize (string, strlen(string), &sizeCountDown);
		}

		viewData->sizeTextInfo = sizeInfo;
		viewData->sizeImage = sizeImage;
		viewData->sizeCountDown = sizeCountDown;
		memcpy (viewData->sizeButton, sizeButton, sizeof(sizeButton));

		if(viewData->dwAlignOption == XM_VIEW_ALIGN_CENTRE)
		{
			// view��ֱ����/ˮƽ������ʾ XM_VIEW_ALIGN_CENTRE
			h = ALERT_V_SPACE;
			w = 0;
			// �ۼơ��ı���Ϣ���ݡ�
			h += sizeInfo.cy + ALERT_V_SPACE;
			if(w < sizeInfo.cx)
				w = sizeInfo.cx;
			// �ۼơ�ͼƬ��Ϣ���ݡ�
			h += sizeImage.cy + ALERT_V_SPACE;
			if(w < sizeImage.cx)
				w = sizeImage.cx;
			// �ۼ� "����ʱ"
			if(viewData->dwOption & XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN)
			{
				h += sizeCountDown.cy + ALERT_V_SPACE;
				if(w < sizeCountDown.cx)
					w = sizeCountDown.cx;
			}
			// �ۼ� "��ť"
			if(viewData->dwButtonCount)
			{
				unsigned int i;
				for (i = 0; i < viewData->dwButtonCount; i ++)
				{
					h += sizeButton[i].cy + ALERT_V_SPACE;
					if(w < sizeButton[i].cx)
						w = sizeButton[i].cx;
				}
			}
			w += ALERT_H_SPACE + ALERT_H_SPACE;

			/*
			if(viewData->dwButtonNormalTextID)
			{
				// ��ʾ����Ϣ���ݡ�������ť��
				//	�Զ�����UIԪ�ع���
				//	(�հ���+����Ϣ���ݡ�+ �հ��� + "����ʱ" + �հ�������ť�� + �հ���)
				h = ALERT_V_SPACE + sizeInfo.cy + ALERT_V_SPACE + sizeButton.cy + ALERT_V_SPACE;
				w = sizeInfo.cx;
				if(sizeInfo.cx < sizeButton.cx)
					w = sizeButton.cx;
				w += ALERT_H_SPACE + ALERT_H_SPACE;
			}
			else
			{
				// ����ʾ������Ϣ
				//	�Զ�����UIԪ�ع���
				// (�հ���+����Ϣ���ݡ�+ �հ���)
				h = ALERT_V_SPACE + sizeInfo.cy + ALERT_V_SPACE;
				w = ALERT_H_SPACE + sizeInfo.cx + ALERT_H_SPACE;
			}
			*/
			
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
			if(viewData->dwButtonNormalTextID)
			{
				// ��ʾ����Ϣ���ݡ�������ť��
				//	�Զ�����UIԪ�ع���
				//	(�հ���+����Ϣ���ݡ�+ �հ��� + ����ť�� + �հ���)
				h = ALERT_V_SPACE + sizeInfo.cy + ALERT_V_SPACE + sizeButton.cy + ALERT_V_SPACE;
			}
			else
			{
				// ����ʾ������Ϣ
				//	�Զ�����UIԪ�ع���
				// (�հ���+����Ϣ���ݡ�+ �հ���)
				h = ALERT_V_SPACE + sizeInfo.cy + ALERT_V_SPACE;
			}
			*/

			h = ALERT_V_SPACE;
			w = 0;
			// �ۼơ�������Ϣ���ݡ�
			h += sizeInfo.cy + ALERT_V_SPACE;
			if(w < sizeInfo.cx)
				w = sizeInfo.cx;
			// �ۼơ�ͼƬ��Ϣ���ݡ�
			h += sizeImage.cy + ALERT_V_SPACE;
			if(w < sizeImage.cx)
				w = sizeImage.cx;
			// �ۼ� "����ʱ"
			if(viewData->dwOption & XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN)
			{
				h += sizeCountDown.cy + ALERT_V_SPACE;
				if(w < sizeCountDown.cx)
					w = sizeCountDown.cx;
			}
			// �ۼ� "��ť"
			if(viewData->dwButtonCount)
			{
				unsigned int i;
				for (i = 0; i < viewData->dwButtonCount; i ++)
				{
					h += sizeButton[i].cy + ALERT_V_SPACE;
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
			
		XM_SetWindowPos (XMHWND_HANDLE(AlertView), x, y, w, h);
	}
	else
	{
		viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("AlertView Pull\n");
	}

	// ������ʱ��
	if(viewData->fAutoCloseTime != 0.0)
	{
		XM_SetTimer (XMTIMER_MESSAGEVIEW, 100);	// ����100ms�Ķ�ʱ��
		viewData->dwTimeout = (DWORD)(viewData->fAutoCloseTime * 10.0 + 0.5);
	}
}

VOID AlertViewOnLeave (XMMSG *msg)
{
	ALERTVIEWDATA *viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));

	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnLeave, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);
	// ɾ����ʱ��
	XM_KillTimer (XMTIMER_MESSAGEVIEW);

	if (msg->wp == 0)
	{
		if(viewData)
		{
			// �ͷ�˽�����ݾ��
			XM_free (viewData);
			
		}
		// ���ô��ڵ�˽�����ݾ��Ϊ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(AlertView), NULL);
		XM_printf ("AlertView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("AlertView Push\n");
	}
}

// ��ˢ��CountDown�����ڷ�XM_PAINT��Ϣ���������е���
static void CountDownPaint (void)
{
	XMRECT rc, rect;
	XMRECT rectDesktop;
	XMSIZE sizeCountDown;
	char string[32];
	//APPROMRES *AppRes;
	int x, y;
	int minute, second;
	XMCOLOR bkg_clolor;
	ALERTVIEWDATA *viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));
	if(viewData == NULL)
		return;
	
	//XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>CountDownPaint, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

	XM_GetWindowRect (XMHWND_HANDLE(AlertView), &rc);
	XM_GetDesktopRect (&rectDesktop);

	if(viewData->dwOption & XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN)
	{
		xm_osd_framebuffer_t framebuffer;

		// �˴���δ����
		// 1)	����һ�����Ӵ�������framebuffer����XM_osd_framebuffer_create
		framebuffer = XM_osd_framebuffer_create (
									XM_LCDC_CHANNEL_0, 
									XM_OSD_LAYER_2,	// ALERTVIEWλ��OSD 2��
									XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),	// ��ȡUI�����ʾ��ʽ
									rectDesktop.right - rectDesktop.left + 1,
									rectDesktop.bottom - rectDesktop.top + 1,
									XMHWND_HANDLE(AlertView),
									1,			// �������һ����ͬ�Ӵ���framebuffer����Ƶ����������,
									0
									);
		if(framebuffer == NULL)
			return;

		//	2)	����framebuffer�����Ӵ����� XM_SetWindowFrameBuffer
		XM_SetWindowFrameBuffer (XMHWND_HANDLE(AlertView), framebuffer);

		// 3) ִ����ʾ����
		y = rc.top;
		y += ALERT_V_SPACE + viewData->sizeTextInfo.cy + ALERT_V_SPACE;
		// ����Ƿ���Ҫ��ʾͼƬ��Ϣ
		if(viewData->sizeImage.cy)
			y += viewData->sizeImage.cy + ALERT_V_SPACE;
		x = rc.left + (rc.right - rc.left + 1 - viewData->sizeCountDown.cx) / 2;
		// ˢ����Ҫ��ʾ�����򱳾�
		rect.left = x;
		rect.top = y;
		rect.right = x + viewData->sizeCountDown.cx - 1;
		rect.bottom = y + viewData->sizeCountDown.cy - 1;
		if(viewData->dwBkgColor)
			bkg_clolor = viewData->dwBkgColor;
		else
			bkg_clolor = ALERT_BACKGROUND_COLOR;	// ʹ���뱳��ˢ����ͬ����ɫ��Alphaֵ
		XM_FillRect (XMHWND_HANDLE(AlertView), rect.left, rect.top, rect.right, rect.bottom, bkg_clolor);
		// ���µ���ʱ��������ʾ
		minute = (viewData->dwTimeout / 10) / 60;
		second = (viewData->dwTimeout / 10) % 60;
		sprintf (string, "%02d:%02d", minute,  second);
		AP_TextGetStringSize (string, strlen(string), &sizeCountDown);
		x = rc.left + (rc.right - rc.left + 1 - sizeCountDown.cx) / 2;
		// AP_TextOutDataTimeString (XMHWND_HANDLE(AlertView), x, y, string, strlen(string));
		AP_TextOutWhiteDataTimeString (XMHWND_HANDLE(AlertView), x, y, string, strlen(string));

		// 4) ����NULL framebuffer�����Ӵ����� XM_SetWindowFrameBuffer
		XM_SetWindowFrameBuffer (XMHWND_HANDLE(AlertView), NULL);

		// 5)	�ر�framebuffer,���޸�ˢ�µ���ʾ�豸 XM_osd_framebuffer_close
		XM_osd_framebuffer_close (framebuffer, 0);
	}
}

VOID AlertViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
	XMCOLOR bkg_clolor;
	BYTE old_alpha;
	DWORD menuID;
	HANDLE hWnd = XMHWND_HANDLE(AlertView);
	ALERTVIEWDATA *viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (hWnd);
	if(viewData == NULL)
		return;
	
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnPaint, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

	// ��ȡ�Ӵ�λ����Ϣ
	XM_GetWindowRect (XMHWND_HANDLE(AlertView), &rc);
	if(viewData->dwBkgColor)
		bkg_clolor = viewData->dwBkgColor;
	else
		bkg_clolor = ALERT_BACKGROUND_COLOR;	// ʹ���뱳��ˢ����ͬ����ɫ��Alphaֵ
		
	XM_FillRect(XMHWND_HANDLE(AlertView), rc.left, rc.top, rc.right, rc.bottom, bkg_clolor);

	// ��ʾAlert��Ϣ
	rect = rc;
	rect.bottom = rect.top + ALERT_V_SPACE + viewData->sizeTextInfo.cy + ALERT_V_SPACE - 1 ;

	// ���������ʾ
	old_alpha = XM_GetWindowAlpha (XMHWND_HANDLE(AlertView));
	XM_SetWindowAlpha (XMHWND_HANDLE(AlertView), 255);
	AP_RomImageDrawByMenuID (viewData->dwTextInfoID, XMHWND_HANDLE(AlertView), &rect, XMIMG_DRAW_POS_CENTER);
	XM_SetWindowAlpha (XMHWND_HANDLE(AlertView), old_alpha);
	
	// ��ʾͼƬ��Ϣ
	if(viewData->sizeImage.cy)
	{
		// ����ͼƬ��Ϣ
		rect = rc;
		rect.top += ALERT_V_SPACE + viewData->sizeTextInfo.cy + ALERT_V_SPACE;
		rect.bottom = rect.top + viewData->sizeImage.cy;

		// ���������ʾ
		old_alpha = XM_GetWindowAlpha (XMHWND_HANDLE(AlertView));
		XM_SetWindowAlpha (XMHWND_HANDLE(AlertView), 255);			
		AP_RomImageDrawByMenuID (viewData->dwImageID, XMHWND_HANDLE(AlertView), &rect, XMIMG_DRAW_POS_CENTER);
		XM_SetWindowAlpha (XMHWND_HANDLE(AlertView), old_alpha);
	}

	// ����ʱ��ʾ
	if(viewData->dwOption & XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN)
	{
		int x, y;
		int minute, second;
		XMSIZE sizeCountDown;
		char string[32];
		y = rc.top;
		y += ALERT_V_SPACE + viewData->sizeTextInfo.cy + ALERT_V_SPACE;
		if(viewData->sizeImage.cy)
			y += viewData->sizeImage.cy + ALERT_V_SPACE;

		minute = (viewData->dwTimeout / 10) / 60;
		second = (viewData->dwTimeout / 10) % 60;
		sprintf (string, "%02d:%02d", minute,  second);
		AP_TextGetStringSize (string, strlen(string), &sizeCountDown);
		x = rc.left + (rc.right - rc.left + 1 - sizeCountDown.cx) / 2;
		// AP_TextOutDataTimeString (XMHWND_HANDLE(AlertView), x, y, string, strlen(string));
		AP_TextOutWhiteDataTimeString (XMHWND_HANDLE(AlertView), x, y, string, strlen(string));
	}

	// ��ť��ʾ
	if(viewData->dwButtonCount)
	{
		// ��ʾ��ť����
		unsigned int i;
		old_alpha = XM_GetWindowAlpha (XMHWND_HANDLE(AlertView));
		XM_SetWindowAlpha (XMHWND_HANDLE(AlertView), 255);			
		rect = rc;
		//rect.top = rect.bottom - (ALERT_H_SPACE + viewData->sizeButton[0].cy + ALERT_H_SPACE) + 1;
		//rect.top = rect.bottom - (ALERT_H_SPACE) + 1;
		rect.top = rect.bottom;
		for (i = 0; i < viewData->dwButtonCount; i ++)
		{
			rect.top -= ALERT_V_SPACE + viewData->sizeButton[i].cy;
			//rect.bottom -= ALERT_H_SPACE + viewData->sizeButton[i].cy;
		}
		
		for (i = 0; i < viewData->dwButtonCount; i ++)
		{
			XM_IMAGE *imageButtonTxt, *imageButtonKey;
			imageButtonTxt = NULL;
			imageButtonKey = NULL;
			rect.bottom = rect.top + viewData->sizeButton[i].cy - 1;
			if(viewData->btnPressed[i])
			{
				menuID = AP_ID_ALERT_BUTTON_PRESSED;
			}
			else
			{
				menuID = AP_ID_ALERT_BUTTON_NORMAL;
			}

			viewData->rectButton[i] = rect;
			AP_RomImageDrawByMenuID (menuID, XMHWND_HANDLE(AlertView), &rect, XMIMG_DRAW_POS_CENTER);

			// ��ʾ��ť����
			if(viewData->btnPressed[i])
			{
				//AppRes = AP_AppRes2RomRes (viewData->dwButtonPressedTextID[i]);
				menuID = viewData->dwButtonPressedTextID[i];
			}
			else
			{
				//AppRes = AP_AppRes2RomRes (viewData->dwButtonNormalTextID[i]);
				menuID = viewData->dwButtonNormalTextID[i];
			}
			
#if 0
			// ���� "Button����" �� "Button��ݼ�" �ĳߴ�, ���ж���
			{
			int image_w, image_h;
			int pos_x, pos_y;
			AppRes = AP_AppRes2RomRes (menuID);
			if(AppRes)
				//imageButtonTxt = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
				imageButtonTxt = XM_RomImageCreate (AppRes->rom_offset, AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
			if(i == 0)
				AppRes = AP_AppGetKeyClueRes (VK_AP_MENU);
			else if(i == 1)
				AppRes = AP_AppGetKeyClueRes (VK_AP_MODE);
			else
				AppRes = AP_AppGetKeyClueRes (VK_AP_SWITCH);
			if(AppRes)
				//imageButtonKey = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
				imageButtonKey = XM_RomImageCreate (AppRes->rom_offset, AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);

			image_w = imageButtonTxt->width + 8 + imageButtonKey->width;		// 8 λͼ֮��Ŀհ�
			image_h = imageButtonTxt->height;
			if(image_h < imageButtonKey->height)
				image_h = imageButtonKey->height;

			// ˮƽ����
			pos_x = ((rect.right - rect.left + 1) - image_w) / 2 + rect.left;
			pos_y = ((rect.bottom - rect.top + 1) - imageButtonTxt->height) / 2 + rect.top;
			XM_ImageDisplay (imageButtonTxt, hWnd, pos_x, pos_y);
			pos_x += imageButtonTxt->width + 8;
			XM_ImageDelete (imageButtonTxt);

			pos_y = ((rect.bottom - rect.top + 1) - imageButtonKey->height) / 2 + rect.top;
			XM_ImageDisplay (imageButtonKey, hWnd, pos_x, pos_y);
			XM_ImageDelete (imageButtonKey);
			}
#else
			AP_RomImageDrawByMenuID (menuID, XMHWND_HANDLE(AlertView), &rect, XMIMG_DRAW_POS_CENTER);
#endif

			rect.top += ALERT_V_SPACE + viewData->sizeButton[i].cy;
			//rect.bottom += ALERT_H_SPACE + viewData->sizeButton[i].cy;
		}
		XM_SetWindowAlpha (XMHWND_HANDLE(AlertView), old_alpha);
	}
}


VOID AlertViewOnKeyDown (XMMSG *msg)
{
	ALERTVIEWDATA *viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));
	if(viewData == NULL)
		return;

	
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnKeyDown, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);
	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
		case VK_AP_DOWN:
			printf(">>>VK_AP_DOWN, viewData->btnPressed[0]: %d viewData->btnPressed[1]: %d \r\n",viewData->btnPressed[0],viewData->btnPressed[1]);
			viewData->btnPressed[0] = 0;
			viewData->btnPressed[1] = 1;
			if(viewData->dwButtonCount > 0)
			{
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			break;

		case VK_AP_MODE:		// �л������г���¼��״̬
		case VK_AP_UP:
			printf(">>>VK_AP_UP, viewData->btnPressed[0]: %d viewData->btnPressed[1]: %d \r\n",viewData->btnPressed[0],viewData->btnPressed[1]);
			viewData->btnPressed[1] = 0;
			viewData->btnPressed[0] = 1;
			if(viewData->dwButtonCount > 1)
			{
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			break;

		case VK_AP_SWITCH:
			printf(">>>VK_AP_SWITCH, viewData->btnPressed[0]: %d viewData->btnPressed[1]: %d \r\n",viewData->btnPressed[0],viewData->btnPressed[1]);
			if(viewData->btnPressed[0] == 1)
				XM_PostMessage(XM_COMMAND, VK_AP_MENU, 0);
			else 
				XM_PullWindow (0);
				
			
			#if 0
			viewData->btnPressed[2] = 1;
			if(viewData->dwButtonCount > 2)
			{
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			#endif
			break;
		#if 0
		case VK_AP_UP:
			printf("11111111111111111 VK_AP_UP \n");
			/*
			if(viewData->dwOption & (XM_ALERTVIEW_OPTION_ADJUST_COUNTDOWN))
			{
				if(viewData->dwTimeout > 0)
				{
					int timeout = viewData->dwTimeout;
					timeout -= 100;
					if(timeout < 0)
						timeout = 0;
					viewData->dwTimeout = timeout;
					XM_InvalidateWindow ();
					XM_UpdateWindow ();
				}
			}
			*/
			break;

		case VK_AP_DOWN:			// һ������
		printf("11111111111111111 VK_AP_DOWN \n");
			/*
			if(viewData->dwOption & (XM_ALERTVIEW_OPTION_ADJUST_COUNTDOWN))
			{
				viewData->dwTimeout += 100;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			*/
			//AP_OnekeyPhotograph ();
			break;
		#endif
		default:
			// �˴������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			break;

	}
}

VOID AlertViewOnKeyUp (XMMSG *msg)
{
	ALERTVIEWDATA *viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));
	if(viewData == NULL)
		return;
	
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnKeyUp, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
			if(viewData->dwButtonCount > 0 && viewData->btnPressed[0])
			{
				viewData->btnPressed[0] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();

				XM_PostMessage (XM_COMMAND, msg->wp, 0);
			}
			else if(viewData->dwButtonCount == 0 && viewData->btnPressed[0])
			{
				XM_PostMessage (XM_COMMAND, msg->wp, 0);
			}
			break;

		case VK_AP_MODE:		// �л������г���¼��״̬
			if(viewData->dwButtonCount > 1 && viewData->btnPressed[1])
			{
				viewData->btnPressed[1] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();

				XM_PostMessage (XM_COMMAND, msg->wp, 1);
			}
			else if(viewData->dwButtonCount == 0 && viewData->btnPressed[1])
			{
				XM_PostMessage (XM_COMMAND, msg->wp, 0);
			}
			break;

		case VK_AP_SWITCH:
			if(viewData->dwButtonCount > 2 && viewData->btnPressed[2])
			{
				viewData->btnPressed[2] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();

				XM_PostMessage (XM_COMMAND, msg->wp, 2);
			}
			else if(viewData->dwButtonCount == 0 && viewData->btnPressed[2])
			{
				XM_PostMessage (XM_COMMAND, msg->wp, 0);
			}
			break;

		default:
			// �˴������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			break;
	}
}

VOID AlertViewOnCommand (XMMSG *msg)
{
	ALERTVIEWDATA *viewData;

	viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));
	if(viewData == NULL)
		return;
	
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnCommand, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);
	
	// �����ڰ�������ص��������ɻص�����������
	// ����ȱʡ�ر��Ӵ������ص�������
	if(viewData->dwOption & XM_ALERTVIEW_OPTION_ENABLE_CALLBACK && viewData->alertcb)
	{
		(*viewData->alertcb) (viewData->UserPrivate, msg->wp);
	}
	else
	{
		// ���ص������ߴ���
		XM_PullWindow (0);
	}

}

VOID AlertViewOnTimer (XMMSG *msg)
{
	ALERTVIEWDATA *viewData;
	viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));
	if(viewData == NULL)
		return;

	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnTimer, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);
	
	if(viewData->dwTimeout > 0)
	{
		viewData->dwTimeout --;
	}
	if(viewData->dwTimeout == 0)
	{
		// ���ص��������Ӵ�
		if(viewData->dwOption & XM_ALERTVIEW_OPTION_ENABLE_CALLBACK && viewData->alertcb)
		{
			XM_PostMessage (XM_COMMAND, 0, 0);
		}
		else
		{
			XM_PullWindow (0);
		}
	}
	else if((viewData->dwTimeout % 10) == 0)
	{
	      
		// XM_InvalidateWindow ();	
		CountDownPaint ();
	}
}

// ���������޷�����ϵͳ��Ϣ�¼����˳�������ϵͳȱʡ����
VOID AlertViewOnSystemEvent(XMMSG *msg)
{
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnSystemEvent, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

#if 0
	// һ�������¼�
	if(msg->wp == SYSTEM_EVENT_ONE_KEY_PHOTOGRAPH)
	{
		// ϵͳȱʡ����
		return;
	}

	// ���ص��������Ӵ�
	// ��ֹ������Ϣ���͸�ϵͳȱʡ����
	XM_BreakSystemEventDefaultProcess (msg);
	// ������Ϣ���²��뵽��Ϣ���ж���
	XM_InsertMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
	// ������ǰ��alert�Ӵ�
	XM_PullWindow (0);
#endif
}

// TouchDown
VOID AlertViewOnTouchDown (XMMSG *msg)
{
	int index;
	int x, y;
	ALERTVIEWDATA *viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));
	if(viewData == NULL)
		return;
	
	if(viewData->dwButtonCount == 0)
		return;

	x = LOWORD(msg->lp);
	x = x - 640;
	y = HIWORD(msg->lp);
	
	for (index = 0; index < viewData->dwButtonCount; index ++)
	{
		if(	x >= viewData->rectButton[index].left 
			&& x < viewData->rectButton[index].right
			&& y >= viewData->rectButton[index].top
			&& y < viewData->rectButton[index].bottom
			)
		{
			break;
		}
	}

	if(index == viewData->dwButtonCount)
		return;

	viewData->btnPressed[index] = 1;
	XM_InvalidateWindow ();
	XM_UpdateWindow ();


}

// TouchUp
VOID AlertViewOnTouchUp (XMMSG *msg)
{
	int index;
	ALERTVIEWDATA *viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));

	if(viewData == NULL)
		return;
	if(viewData->dwButtonCount == 0)
	{
		// ������ǰ��alert�Ӵ�
		XM_PullWindow (0);
		return;
	}
	for (index = 0; index < viewData->dwButtonCount; index ++)
	{
		if(viewData->btnPressed[index])
			break;
	}
	if(index == viewData->dwButtonCount)
		return;
	
	viewData->btnPressed[index] = 0;
	XM_InvalidateWindow ();
	XM_UpdateWindow ();

	XM_PostMessage (XM_COMMAND, (WORD)(VK_AP_MENU + index), index);
}

// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (AlertView)
	XM_ON_MESSAGE (XM_PAINT, AlertViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, AlertViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, AlertViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, AlertViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, AlertViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, AlertViewOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, AlertViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, AlertViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, AlertViewOnTouchUp)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, AlertViewOnSystemEvent)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, LCD_YDOTS - 120, LCD_XDOTS, 120, AlertView, 1, 0, 0, 255, HWND_ALERT)
