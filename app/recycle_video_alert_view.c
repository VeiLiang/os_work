//****************************************************************************
//
//	Copyright (C) 2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: recycle_alert_view.c
//	  ѭ��¼�ƿռ���Ϣ�Ӵ�
//
//	Revision history
//
//		2014.05.15	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"


#define	ALERT_V_SPACE		15		// ��ֱ����߾�
#define	ALERT_H_SPACE		10		// ˮƽ����߾�

#define	ALERT_BACKGROUND_COLOR		0xFFFFFFFF

typedef struct tagRECYCLEVIDEOALERTVIEWDATA {
	DWORD				dwTextInfoID;					// �ı���Ϣ��ԴID
	DWORD				dwImageID;						// ͼƬ��Ϣ��ԴID
	float				fAutoCloseTime;				// �Ӵ���ʱ���Զ��ر�ʱ�䡣0��ʾ��ֹ�Զ��رչ���
	float				fViewAlpha;						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
	DWORD				dwAlignOption;					//	��ͼ����ʾ����������Ϣ
	DWORD				dwOption;						//	RECYCLEVIDEOALERTVIEW��ͼ�Ŀ���ѡ��

	DWORD				dwBkgColor;

	// �ڲ�״̬
	XMSIZE				sizeTextInfo;	
	XMSIZE				sizeImage;
	
	XMSIZE				sizeSDCARD;						// SD������
	XMSIZE				sizeRECYCLE;					// ѭ��¼��
	XMSIZE				sizeLOCKED;						// ����¼��
	XMSIZE				sizeOTHERS;						// ����

	XMSIZE				sizeREGION;						// ���������С

	DWORD				dwTimeout;						// ����������ʱ����
	int					btnPressed[4];

} RECYCLEVIDEOALERTVIEWDATA;

// ѭ��¼�񱨾���Ϣ��ʾ�Ӵ�
//
//	----- ��ͼ����ģʽ֧�� ( XM_VIEW_ALIGN_CENTRE �� XM_VIEW_ALIGN_BOTTOM ) -----
//
//	1) ��ֱ����/ˮƽ������ʾ XM_VIEW_ALIGN_CENTRE
//	2) �ײ�����/ˮƽ������ʾ XM_VIEW_ALIGN_BOTTOM
//
//  ---- UIԪ�ز���(Layout) ----
//
// 1) ��ʾ��������Ϣ���ݡ���ͼƬ��Ϣ���ݡ�
//		�հ���
//		������Ϣ����
//		�հ���
//		ͼƬ��Ϣ����
//		�հ���
//
// 2) ����ʾ����Ϣ���ݡ�
//		�հ���
//		��Ϣ����
//		�հ���

// ��ʾ��Ϣ��ͼ
// 0 ��ͼ����������ͼ��ʾʧ��
// 1 ��ͼ��������ʾ
XMBOOL XM_OpenRecycleVideoAlertView ( 
								 DWORD dwTitleID,					// ������ԴID

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

								 DWORD dwAlignOption,			//	��ͼ����ʾ����������Ϣ
																		//		���OSD��ʾ��ԭ��(OSD��Ч��������Ͻ�)
																		//		XM_VIEW_ALIGN_CENTRE	
																		//			�Ӵ����ж���(���OSD��ʾ����)
																		//		XM_VIEW_ALIGN_BOTTOM
																		//			�Ӵ��ײ����ж���(���OSD��ʾ����)
								 DWORD dwOption					// RECYCLEVIDEOALERTVIEW��ͼ�Ŀ���ѡ��
								 )
{
	RECYCLEVIDEOALERTVIEWDATA *viewData;

	if(dwTitleID == 0)
	{
		XM_printf ("XM_OpenRecycleVideoAlertView NG, dwTitleID(%d) must be non-zero\n", dwTitleID);
		return 0;
	}
	if(dwAlignOption != XM_VIEW_ALIGN_CENTRE && dwAlignOption != XM_VIEW_ALIGN_BOTTOM)
	{
		XM_printf ("XM_OpenRecycleVideoAlertView NG, illegal dwAlignOption(%d)\n", dwAlignOption);
		return 0;
	}
	if(fViewAlpha < 0.0 || fViewAlpha > 1.0)
	{
		XM_printf ("XM_OpenRecycleVideoAlertView NG, illegal fViewAlpha(%f), should be 0.0 ~ 1.0\n", fViewAlpha);
		return 0;
	}


	viewData = XM_calloc (sizeof(RECYCLEVIDEOALERTVIEWDATA));
	if(viewData == NULL)
		return 0;
	viewData->dwTextInfoID = dwTitleID;
	viewData->fAutoCloseTime = fAutoCloseTime;
	viewData->dwBkgColor = dwBackgroundColor;
	viewData->fViewAlpha = fViewAlpha;
	viewData->dwAlignOption = dwAlignOption;
	viewData->dwOption = dwOption;
	if(XM_PushWindowEx (XMHWND_HANDLE(RecycleVideoAlertView), (DWORD)viewData) == 0)
	{
		XM_free (viewData);
		return 0;
	}
	return 1;
}

VOID RecycleVideoAlertViewOnEnter (XMMSG *msg)
{
	RECYCLEVIDEOALERTVIEWDATA *viewData;
	if(msg->wp == 0)
	{
		int x, y, w, h;
		APPROMRES *AppRes;
		XMSIZE sizeInfo;
		XMSIZE sizeImage;
		XMRECT rectDesktop;
		// ��һ�ν���(����׼������)
		viewData = (RECYCLEVIDEOALERTVIEWDATA *)msg->lp;

		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView), viewData);

		// �����Ӵ���alphaֵ
		XM_SetWindowAlpha (XMHWND_HANDLE(RecycleVideoAlertView), (unsigned char)(viewData->fViewAlpha * 255));

		// ���㴰�ڵ�λ����Ϣ (����UIԪ�ش�С������ģʽ����)
		XM_GetDesktopRect (&rectDesktop);
		w = 0;
		h = 0;
		sizeInfo.cx = 0;
		sizeInfo.cy = 0;
		sizeImage.cx = 0;
		sizeImage.cy = 0;
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
		if(viewData->dwOption & AP_RECYCLEVIDEOALERTVIEW_OPTION_GRAPH)
		{
			// ��ͼ���
		}
		else
		{
			// ������
			XMSIZE size;
			// SD������
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_SDCARD_SPACE);
			if(AppRes)
			{
				XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &size);
				sizeImage.cy += size.cy + ALERT_V_SPACE;
				viewData->sizeSDCARD = size;

				if(size.cx > viewData->sizeREGION.cx)
					viewData->sizeREGION.cx = size.cx;
				if(size.cy > viewData->sizeREGION.cy)
					viewData->sizeREGION.cy = size.cy;
			}
			// ѭ��¼��
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_RECYCLE_SPACE);
			if(AppRes)
			{
				XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &size);
				sizeImage.cy += size.cy + ALERT_V_SPACE;
				viewData->sizeRECYCLE = size;
				if(size.cx > viewData->sizeREGION.cx)
					viewData->sizeREGION.cx = size.cx;
				if(size.cy > viewData->sizeREGION.cy)
					viewData->sizeREGION.cy = size.cy;
			}
			// ����¼��
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_LOCKED_SPACE);
			if(AppRes)
			{
				XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &size);
				sizeImage.cy += size.cy + ALERT_V_SPACE;
				viewData->sizeLOCKED = size;
				if(size.cx > viewData->sizeREGION.cx)
					viewData->sizeREGION.cx = size.cx;
				if(size.cy > viewData->sizeREGION.cy)
					viewData->sizeREGION.cy = size.cy;
			}
			// �����ռ�
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_OTHERS_SPACE);
			if(AppRes)
			{
				XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &size);
				sizeImage.cy += size.cy + ALERT_V_SPACE;
				viewData->sizeOTHERS = size;
				if(size.cx > viewData->sizeREGION.cx)
					viewData->sizeREGION.cx = size.cx;
				if(size.cy > viewData->sizeREGION.cy)
					viewData->sizeREGION.cy = size.cy;
			}
			
		}

		// 3) ����Button����ʾ�����С

		viewData->sizeTextInfo = sizeInfo;
		viewData->sizeImage = sizeImage;


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

			w += ALERT_H_SPACE + ALERT_H_SPACE;
			
			if(h > (rectDesktop.bottom - rectDesktop.top + 1))
				h = (rectDesktop.bottom - rectDesktop.top + 1);
			if(w > (rectDesktop.right - rectDesktop.left + 1))
				w = (rectDesktop.right - rectDesktop.left + 1);

			x = (rectDesktop.right - rectDesktop.left + 1 - w) / 2;
			y = (rectDesktop.bottom - rectDesktop.top + 1 - h) / 2;
		}
		else //if(viewData->dwAlignOption == XM_VIEW_ALIGN_BOTTOM)
		{

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

			if(h > (rectDesktop.bottom - rectDesktop.top + 1))
				h = (rectDesktop.bottom - rectDesktop.top + 1);

			x = 0;
			w = (rectDesktop.right - rectDesktop.left + 1);
			y = rectDesktop.bottom - h + 1;
		}
			
		XM_SetWindowPos (XMHWND_HANDLE(RecycleVideoAlertView), x, y, w, h);
	}
	else
	{
		viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("RecycleVideoAlertView Pull\n");
	}

	// ������ʱ��
	if(viewData->fAutoCloseTime != 0.0)
	{
		XM_SetTimer (XMTIMER_MESSAGEVIEW, 100);	// ����100ms�Ķ�ʱ��
		viewData->dwTimeout = (DWORD)(viewData->fAutoCloseTime * 10.0 + 0.5);
	}
}

VOID RecycleVideoAlertViewOnLeave (XMMSG *msg)
{
	RECYCLEVIDEOALERTVIEWDATA *viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
	// ɾ����ʱ��
	XM_KillTimer (XMTIMER_MESSAGEVIEW);

	if (msg->wp == 0)
	{
		if(viewData)
		{
			// �ͷ�˽�����ݾ��
			XM_free (viewData);
			
		}
		XM_printf ("RecycleVideoAlertView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("RecycleVideoAlertView Push\n");
	}
}

extern void recycle_video_graph_draw (u8_t *framebuf, unsigned int linelength, 
										 float recycle_percent,
										 float locked_percent,
										 float others_percent);

// �˴�û�п��Ƕ��������
// ����ֵ����
//	0	�ѻ�ȡ����Ϣ
// -1	�޷���ȡ����Ϣ
int XM_VideoItemGetDiskUsage (XMINT64 *TotalSpace, 		// SD���ռ�
										XMINT64 *LockedSpace, 		// ���������ļ��ռ�
										XMINT64 *RecycleSpace,		// ��ѭ��ʹ���ļ� + ���пռ�
										XMINT64 *OtherSpace			// �����ļ�(ϵͳ)ռ�ÿռ�
									);



VOID RecycleVideoAlertViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
	APPROMRES *AppRes;
	XMCOLOR bkg_clolor;
	BYTE old_alpha;
	HANDLE hwnd = XMHWND_HANDLE(RecycleVideoAlertView);
	RECYCLEVIDEOALERTVIEWDATA *viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(viewData == NULL)
		return;

	// ��ȡ�Ӵ�λ����Ϣ
	XM_GetWindowRect (hwnd, &rc);
	if(viewData->dwBkgColor)
		bkg_clolor = viewData->dwBkgColor;
	else
		bkg_clolor = ALERT_BACKGROUND_COLOR;	// ʹ���뱳��ˢ����ͬ����ɫ��Alphaֵ
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, bkg_clolor);

	// ��ʾRecycleVideoAlert��Ϣ
	rect = rc;
	rect.bottom = rect.top + ALERT_V_SPACE + viewData->sizeTextInfo.cy + ALERT_V_SPACE - 1 ;
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
		float sdcard_volume;
		float recycle_percent, locked_percent, others_percent;
		XMINT64 TotalSpace, LockedSpace, RecycleSpace, OtherSpace;
		unsigned int total_size, locked_size, recycle_size, other_size;

		int w = 0;
		XMSIZE s;
		char txt[16];

		TotalSpace = 0;
		LockedSpace = 0;
		RecycleSpace = 0;
		OtherSpace = 0;
		XM_VideoItemGetDiskUsage (&TotalSpace, &LockedSpace, &RecycleSpace, &OtherSpace);

		total_size = (unsigned int)(TotalSpace / (1024*1024));
		locked_size = (unsigned int)(LockedSpace / (1024*1024));
		recycle_size = (unsigned int)(RecycleSpace / (1024*1024));
		other_size = (unsigned int)(OtherSpace / (1024*1024));

		sprintf (txt, "128.0GB");
		s.cx = 0;
		AP_TextGetStringSize (txt, strlen(txt), &s);

		// DEMO���Դ���
		sdcard_volume = ((float)total_size) / (1024.0f);	
		recycle_percent = ((float)recycle_size) / (total_size ? total_size : 1);
		locked_percent = ((float)locked_size) / (total_size ? total_size : 1);
		others_percent = ((float)other_size) / (total_size ? total_size : 1);



		w = ALERT_H_SPACE + 120 + 2 * ALERT_H_SPACE + viewData->sizeREGION.cx + ALERT_H_SPACE + s.cx + ALERT_H_SPACE;
		
		w = (rect.right - rect.left + 1 - w) / 2;
		if(w < 0)
		{
			XM_printf ("RECYCLE VIEW NG, illegal width (%d)\n", w);
			w = 0;
		}


		rect = rc;
		rect.top += ALERT_V_SPACE + viewData->sizeTextInfo.cy + ALERT_V_SPACE;
		rect.left += ALERT_H_SPACE + w+100;

		{
			// SD������
			float space;
			char  buffer[16];
			unsigned int cx = viewData->sizeREGION.cx + ALERT_H_SPACE;
			old_alpha = XM_GetWindowAlpha (hwnd);
			XM_SetWindowAlpha (hwnd, 255);			
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_SDCARD_SPACE);
			if(AppRes)
			{
				// ���������ʾ
				XM_RomImageDraw (AppRes->rom_offset, 
										AppRes->res_length,
										hwnd, &rect, XMIMG_DRAW_POS_LEFTTOP);
				space = sdcard_volume;
				sprintf (buffer, "%.1f GB", space);
				AP_TextOutWhiteDataTimeString (hwnd, rect.left + cx, rect.top, buffer, strlen(buffer));

				rect.top += viewData->sizeSDCARD.cy + ALERT_V_SPACE;
			}

			// ѭ��¼��
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_RECYCLE_SPACE);
			if(AppRes)
			{
				// �������Ͻ���ʾ
				XM_FillRect (hwnd, rect.left, rect.top, 
					rect.left + viewData->sizeRECYCLE.cx - 1,
					rect.top + viewData->sizeRECYCLE.cy - 1, XM_ARGB(0xFF, 0x00, 0x80, 0x00) );

				XM_RomImageDraw (AppRes->rom_offset, 
										AppRes->res_length,
										hwnd, &rect, XMIMG_DRAW_POS_LEFTTOP);

				space = sdcard_volume * recycle_percent;
				sprintf (buffer, "%.1f GB", space);
				AP_TextOutWhiteDataTimeString (hwnd, rect.left + cx, rect.top, buffer, strlen(buffer));

				rect.top += viewData->sizeRECYCLE.cy + ALERT_V_SPACE;
			}

			// ����¼��
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_LOCKED_SPACE);
			if(AppRes)
			{
				XM_FillRect (hwnd, rect.left, rect.top, 
					rect.left + viewData->sizeRECYCLE.cx - 1,
					rect.top + viewData->sizeRECYCLE.cy - 1, XM_ARGB(0xFF, 0x80, 0x00, 0x00) );
				// �������Ͻ���ʾ
				XM_RomImageDraw (AppRes->rom_offset, 
										AppRes->res_length,
										hwnd, &rect, XMIMG_DRAW_POS_LEFTTOP);
				space = sdcard_volume * locked_percent;
				sprintf (buffer, "%.1f GB", space);
				AP_TextOutWhiteDataTimeString (hwnd, rect.left + cx, rect.top, buffer, strlen(buffer));

				rect.top += viewData->sizeLOCKED.cy + ALERT_V_SPACE;
			}
			// ����
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_OTHERS_SPACE);
			if(AppRes)
			{
				// ���������ʾ
				XM_FillRect (hwnd, rect.left, rect.top, 
					rect.left + viewData->sizeRECYCLE.cx - 1,
					rect.top + viewData->sizeRECYCLE.cy - 1, XM_ARGB(0xFF, 0x00, 0x00, 0x80) );

				XM_RomImageDraw (AppRes->rom_offset, 
										AppRes->res_length,
										hwnd, &rect, XMIMG_DRAW_POS_LEFTTOP);
				space = sdcard_volume * others_percent;
				sprintf (buffer, "%.1f GB", space);
				AP_TextOutWhiteDataTimeString (hwnd, rect.left + cx, rect.top, buffer, strlen(buffer));

				rect.top += viewData->sizeOTHERS.cy + ALERT_V_SPACE;
			}
			
			XM_SetWindowAlpha (hwnd, old_alpha);
		}

	}


}


VOID RecycleVideoAlertViewOnKeyDown (XMMSG *msg)
{
	RECYCLEVIDEOALERTVIEWDATA *viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
	if(viewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
			viewData->btnPressed[0] = 1;
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_MODE:		// �л������г���¼��״̬
			viewData->btnPressed[1] = 1;
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_SWITCH:
			viewData->btnPressed[2] = 1;
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		default:
			// �˴������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			break;

	}
}

VOID RecycleVideoAlertViewOnKeyUp (XMMSG *msg)
{
	RECYCLEVIDEOALERTVIEWDATA *viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
	if(viewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
			if(viewData->btnPressed[0] == 1)
			{
				viewData->btnPressed[0] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
				XM_PostMessage (XM_COMMAND, msg->wp, 0);
			}
			break;

		case VK_AP_MODE:		// �л������г���¼��״̬
			if(viewData->btnPressed[1] == 1)
			{
				viewData->btnPressed[1] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
				XM_PostMessage (XM_COMMAND, msg->wp, 0);
			}
			break;

		case VK_AP_SWITCH:
			if(viewData->btnPressed[2] == 1)
			{
				viewData->btnPressed[2] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
				XM_PostMessage (XM_COMMAND, msg->wp, 0);
			}
			break;

		default:
			// �˴������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			break;
	}
}

VOID RecycleVideoAlertViewOnCommand (XMMSG *msg)
{
	RECYCLEVIDEOALERTVIEWDATA *viewData;

	viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
	if(viewData == NULL)
		return;

	XM_printf ("RecycleVideoAlert %d\n", msg->wp);

	// �����ڰ�������ص��������ɻص�����������
		// ���ص������ߴ���
		XM_PullWindow (0);

}

VOID RecycleVideoAlertViewOnTimer (XMMSG *msg)
{
	RECYCLEVIDEOALERTVIEWDATA *viewData;
	viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
	if(viewData == NULL)
		return;
	if(viewData->dwTimeout > 0)
	{
		viewData->dwTimeout --;
	}

	//XM_InvalidateWindow ();

	if(viewData->dwTimeout == 0)
	{
		// ���ص��������Ӵ�
		XM_PullWindow (0);
	}
}

// ���������޷�����ϵͳ��Ϣ�¼����˳�������ϵͳȱʡ����
VOID RecycleVideoAlertViewOnSystemEvent (XMMSG *msg)
{
	// ���ص��������Ӵ�
	XM_BreakSystemEventDefaultProcess (msg);
	XM_PostMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
	XM_PullWindow (0);
}
VOID RecycleVideoAlertViewOnTouchDown (XMMSG *msg)
{
         RECYCLEVIDEOALERTVIEWDATA *viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
	    if(viewData == NULL)
		return;
        	viewData->btnPressed[0] = 1;
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
	

}

VOID RecycleVideoAlertViewOnTouchUp (XMMSG *msg)
{
    RECYCLEVIDEOALERTVIEWDATA *viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
	if(viewData == NULL)
		return;

		if(viewData->btnPressed[0] == 1)
			{
				viewData->btnPressed[0] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
				XM_PostMessage (XM_COMMAND, VK_AP_MENU, 0);
			}
       
}
// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (RecycleVideoAlertView)
	XM_ON_MESSAGE (XM_PAINT, RecycleVideoAlertViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, RecycleVideoAlertViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, RecycleVideoAlertViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, RecycleVideoAlertViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, RecycleVideoAlertViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, RecycleVideoAlertViewOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, RecycleVideoAlertViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, RecycleVideoAlertViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, RecycleVideoAlertViewOnTouchUp)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, RecycleVideoAlertViewOnSystemEvent)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, LCD_YDOTS - 120, LCD_XDOTS, 120, RecycleVideoAlertView, 1, 0, 0, 255, HWND_ALERT)
