//****************************************************************************
//
//	Copyright (C) 2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_soundvolumeview.c
//	  ���������Ӵ�
//
//	Revision history
//
//		2014.03.23	ZhuoYongHong Initial version
//
//****************************************************************************

#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"

// ����ɫ����
#define	SOUNDVOLUME_BACKGROUND_COLOR		0x50404040

#define	WAIT_TIMEOUT	3000	// ms�������ȴ�����
#define	EXIT_TIMEOUT	1000	// ms���˳���������

// ������

// ������MICͼƬ

// ����ָʾ���� 14X14����ɫ�����4
#define	INDICATOR_SIZE			14
#define	INDICATOR_SPACE		4

#define	HORZ_SPACE_SIZE		10		// ˮƽ����հ�����Ĵ�С
#define	VERT_SPACE_SIZE		14		// ��ֱ����հ׼�UIԪ��֮��ļ��

// �Ӵ������Ͷ���
#define	MIC_TYPE				0		// MIC��������
#define	BELL_TYPE				1		// ������������

typedef struct tagSOUNDVOLUMEVIEWDATA {
	DWORD				dwTitleID;						// ����/MIC��Ϣ�ı���ԴID
	DWORD				dwImage_ON_ID;					// ����ͼƬID(����״̬)
	DWORD				dwImage_Off_ID;				// ����ͼƬID(�ر�״̬)
	UCHAR				type;								// MIC/BELL�Ӵ�������
	UCHAR				ucSoundVolume;					// ����/¼������ֵ
	UCHAR				ucOldSoundVolume;				// �ϵ�����/¼������ֵ
	UCHAR				rev;

	// �ڲ�״̬
	XMSIZE			sizeTitle;						// ��������С
	XMSIZE			sizeImage;						// ͼƬ����С
	XMSIZE			sizeIndicator;					// ����ָʾ����С

	USHORT			dwWaitTimeout;					// �ȴ�������
	USHORT			dwExitTimeout;					// �رռ�����������Alphaֵ���˳�		
} SOUNDVOLUMEVIEWDATA;

XMBOOL XM_OpenSoundVolumeView (DWORD dwTitleID, UCHAR type)
{
	SOUNDVOLUMEVIEWDATA *viewData;
	XMRECT rc;

	XM_GetDesktopRect (&rc);

	// �������
	if(dwTitleID == 0)
	{
		XM_printf ("XM_OpenSoundVolumeView NG, dwInfoTextID(%d) must be non-zero\n", dwTitleID);
		return 0;
	}

	if(type > BELL_TYPE)
	{
		XM_printf ("XM_OpenSoundVolumeView NG, type(%d) illegal\n", type);
		return 0;
	}
	
	viewData = XM_calloc (sizeof(SOUNDVOLUMEVIEWDATA));
	if(viewData == NULL)
		return 0;
	viewData->type = type;
	viewData->dwTitleID = dwTitleID;
	if(type == MIC_TYPE)
	{
		viewData->ucSoundVolume = (unsigned char)AP_GetMenuItem (APPMENUITEM_MIC_VOLUME);
		if( (rc.bottom - rc.top + 1) >= 320)
			viewData->dwImage_ON_ID = AP_ID_SETTING_VOLUME_MIC_IMAGE;
		else
			viewData->dwImage_ON_ID = AP_ID_SETTING_VOLUME_MIC_IMAGE_SMALL;
	}
	else
	{
		viewData->ucSoundVolume = (unsigned char)AP_GetMenuItem (APPMENUITEM_BELL_VOLUME);
		if( (rc.bottom - rc.top + 1) >= 320)
			viewData->dwImage_ON_ID = AP_ID_SETTING_VOLUME_SOUND_IMAGE;
		else
			viewData->dwImage_ON_ID = AP_ID_SETTING_VOLUME_MIC_IMAGE_SMALL;
	}
	viewData->ucOldSoundVolume = viewData->ucSoundVolume;
	if(XM_PushWindowEx (XMHWND_HANDLE(SoundVolumeView), (DWORD)viewData) == 0)
	{
		XM_free (viewData);
		return 0;
	}
	return 1;
}

// ���������������Ӵ�
XMBOOL XM_OpenBellSoundVolumeSettingView (VOID)
{
	return XM_OpenSoundVolumeView (AP_ID_SETTING_VOLUME_SOUND_TITLE_BELL, BELL_TYPE);
}

// ��MIC¼�����������Ӵ�
XMBOOL XM_OpenMicSoundVolumeSettingView (VOID)
{
	return XM_OpenSoundVolumeView (AP_ID_SETTING_VOLUME_MIC_TITLE_BELL, MIC_TYPE);
}


VOID SoundVolumeViewOnEnter (XMMSG *msg)
{
	SOUNDVOLUMEVIEWDATA *viewData;
	HANDLE hWnd;
	hWnd = XMHWND_HANDLE(SoundVolumeView);
	if(msg->wp == 0)
	{
		int x, y, w, h;
		APPROMRES *AppRes;
		XMSIZE sizeTitle;
		XMSIZE sizeImage;
		XMSIZE sizeIndicator;
		XMRECT rectDesktop;
		// ��һ�ν���(����׼������)
		viewData = (SOUNDVOLUMEVIEWDATA *)msg->lp;

		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (hWnd, viewData);

		// ���㴰�ڵ�λ����Ϣ (��������UIԪ�ش�С)
		XM_GetDesktopRect (&rectDesktop);
		w = 0;
		h = 0;
		sizeTitle.cx = 0;
		sizeTitle.cy = 0;
		sizeImage.cx = 0;
		sizeImage.cy = 0;
		sizeIndicator.cx = INDICATOR_SIZE * (AP_SETTING_SOUNDVOLUME_COUNT - 1) + INDICATOR_SPACE * (AP_SETTING_SOUNDVOLUME_COUNT - 2);
		sizeIndicator.cy = INDICATOR_SIZE;
		//	1) ������Ϣ�ı�����ʾ�����С
		AppRes = AP_AppRes2RomRes (viewData->dwTitleID);
		if(AppRes)
		{
			XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &sizeTitle);
		}

		// 2) ���㱳��ͼƬ����ʾ�����С
		if(viewData->dwImage_ON_ID)
		{
			AppRes = AP_AppRes2RomRes (viewData->dwImage_ON_ID);
			if(AppRes)
			{
				XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &sizeImage);
			}
		}

		viewData->sizeTitle = sizeTitle;
		viewData->sizeImage = sizeImage;
		viewData->sizeIndicator = sizeIndicator;

		w = sizeTitle.cx;
		if(w < sizeImage.cx)
			w = sizeImage.cx;
		if(w < sizeIndicator.cx)
			w = sizeIndicator.cx;
		w += HORZ_SPACE_SIZE * 2;	// ���߿հ�

		h = VERT_SPACE_SIZE + sizeTitle.cy + VERT_SPACE_SIZE + sizeIndicator.cy + VERT_SPACE_SIZE;
		if(sizeImage.cy)
		{
			h +=  VERT_SPACE_SIZE + sizeImage.cy;
		}
		
		// ��Ļ������ʾ
		x = ((rectDesktop.right - rectDesktop.left + 1) - w) / 2;
		y = ((rectDesktop.bottom - rectDesktop.top + 1) - h) / 2;
			
		// ���µ����Ӵ���λ�ü���С
		XM_SetWindowPos (hWnd, x, y, w, h);

		viewData->dwWaitTimeout = WAIT_TIMEOUT / 100;
		viewData->dwExitTimeout = EXIT_TIMEOUT / 100;

		// ��ֹ�Ӵ�����/�ر�ʱʹ�ö���Ч��
		XM_DisableViewAnimate (hWnd);
	}
	else
	{
		viewData = (SOUNDVOLUMEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SoundVolumeView));
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("SoundVolumeView Pull\n");
	}

	// ������ʱ��
	XM_SetTimer (XMTIMER_MESSAGEVIEW, 100);	// ����100ms�Ķ�ʱ��
}

VOID SoundVolumeViewOnLeave (XMMSG *msg)
{
	SOUNDVOLUMEVIEWDATA *viewData = (SOUNDVOLUMEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SoundVolumeView));
	// ɾ����ʱ��
	XM_KillTimer (XMTIMER_MESSAGEVIEW);

	if (msg->wp == 0)
	{
		// ��������ֵ���õ��ڲ��洢��
		if(viewData && viewData->ucOldSoundVolume != viewData->ucSoundVolume)
		{
			AP_SaveMenuData (&AppMenuData);
		}

		if(viewData)
		{
			// �ͷ�˽�����ݾ��
			XM_free (viewData);
			
		}
		XM_printf ("SoundVolumeView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("SoundVolumeView Push\n");
	}
}


VOID SoundVolumeViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
//	APPROMRES *AppRes;
	XMCOLOR bkg_clolor;
	int i, count;
	HANDLE hWnd = XMHWND_HANDLE(SoundVolumeView);
	SOUNDVOLUMEVIEWDATA *viewData = (SOUNDVOLUMEVIEWDATA *)XM_GetWindowPrivateData (hWnd);
	if(viewData == NULL)
		return;

	// ��ȡ�Ӵ�λ����Ϣ
	XM_GetWindowRect (hWnd, &rc);

	// ��䱳��(ʹ�þ���Alphaֵ�ı���ɫ)
	bkg_clolor = SOUNDVOLUME_BACKGROUND_COLOR;
	XM_FillRect (hWnd, rc.left, rc.top, rc.right, rc.bottom, bkg_clolor);

	// ��ʾ����
	rect = rc;
	rect.top += VERT_SPACE_SIZE;
	rect.bottom = rect.top + viewData->sizeTitle.cy - 1;
//	AppRes = AP_AppRes2RomRes (viewData->dwTitleID);
	// XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
//	AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
	AP_RomImageDrawByMenuID (viewData->dwTitleID, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
	rect.top += viewData->sizeTitle.cy;

	// ��ʾBELL/MICͼƬ
	if(viewData->sizeImage.cy)
	{
		rect.top += VERT_SPACE_SIZE;
		rect.bottom = rect.top + viewData->sizeImage.cy - 1;
	//	AppRes = AP_AppRes2RomRes (viewData->dwImage_ON_ID);
		//XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
	//	AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		AP_RomImageDrawByMenuID (viewData->dwImage_ON_ID, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		rect.top += viewData->sizeImage.cy;
	}

	// ��ʾ����ָʾ
	rect.top += VERT_SPACE_SIZE;
	rect.bottom = rect.top + viewData->sizeIndicator.cy - 1;
	rect.left = rect.left + ((rect.right - rect.left + 1) - viewData->sizeIndicator.cx) / 2;
	rect.right = rect.left + INDICATOR_SIZE - 1;
	count = viewData->ucSoundVolume;
	if(count > (AP_SETTING_SOUNDVOLUME_COUNT - 1))
		count = AP_SETTING_SOUNDVOLUME_COUNT - 1;
	bkg_clolor = 0xFFFFFF | (XM_GetWindowAlpha (hWnd) << 24);	// ��ɫ+Alpha
	for (i = 0; i < count; i ++)
	{
		XM_FillRect (hWnd, rect.left, rect.top, rect.right, rect.bottom, bkg_clolor);
		rect.left += INDICATOR_SIZE + INDICATOR_SPACE;
		rect.right += INDICATOR_SIZE + INDICATOR_SPACE;
	}
}


VOID SoundVolumeViewOnKeyDown (XMMSG *msg)
{
	HANDLE hWnd = XMHWND_HANDLE(SoundVolumeView);
	SOUNDVOLUMEVIEWDATA *viewData = (SOUNDVOLUMEVIEWDATA *)XM_GetWindowPrivateData (hWnd);
	if(viewData == NULL)
		return;
	
	switch(msg->wp)
	{
	       #if 0
		case VK_AP_UP:
			if(viewData->ucSoundVolume < (AP_SETTING_SOUNDVOLUME_COUNT - 1))
			{
				viewData->ucSoundVolume ++;
				if(viewData->type == MIC_TYPE)
				{
					// MIC
					AP_SetMenuItem (APPMENUITEM_MIC_VOLUME, viewData->ucSoundVolume);
					if(viewData->ucSoundVolume == 1)
					{
						// ����MIC
						AP_SetMenuItem (APPMENUITEM_MIC, 1);
					}
				}
				else
				{
					// BELL
					AP_SetMenuItem (APPMENUITEM_BELL_VOLUME, viewData->ucSoundVolume);
				}
				XM_InvalidateWindow ();
			}
			// �ָ��Ӵ���AlphaΪ��͸��
			XM_SetWindowAlpha (hWnd, (UCHAR)(255));
			// ���ü�����
			viewData->dwWaitTimeout = WAIT_TIMEOUT / 100;
			viewData->dwExitTimeout = EXIT_TIMEOUT / 100;
			
			break;

		case VK_AP_DOWN:
			if(viewData->ucSoundVolume > 0)
			{
				viewData->ucSoundVolume --;
				if(viewData->type == MIC_TYPE)
				{
					AP_SetMenuItem (APPMENUITEM_MIC_VOLUME, viewData->ucSoundVolume);
					if(viewData->ucSoundVolume == 0)
					{
						// �ر�MIC¼��
						AP_SetMenuItem (APPMENUITEM_MIC, 0);
					}
				}
				else
				{
					AP_SetMenuItem (APPMENUITEM_BELL_VOLUME, viewData->ucSoundVolume);
					if(viewData->ucSoundVolume == 0)
					{
						// �ر�������ʾ
						AP_SetMenuItem (APPMENUITEM_VOICE_PROMPTS, 0);
					}
				}
				XM_InvalidateWindow ();
			}
			// �ָ��Ӵ���AlphaΪ��͸��
			XM_SetWindowAlpha (hWnd, (UCHAR)(255));
			// ���ü�����
			viewData->dwWaitTimeout = WAIT_TIMEOUT / 100;
			viewData->dwExitTimeout = EXIT_TIMEOUT / 100;
			break;
                 #endif
		default:
			// �˴������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			break;

	}
}

VOID SoundVolumeViewOnTimer (XMMSG *msg)
{
	SOUNDVOLUMEVIEWDATA *viewData;
	float alpha;
	HANDLE hWnd = XMHWND_HANDLE(SoundVolumeView);
	viewData = (SOUNDVOLUMEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SoundVolumeView));
	if(viewData == NULL)
		return;

	if(viewData->dwWaitTimeout > 0)
	{
		viewData->dwWaitTimeout --;
	}
	else if(viewData->dwExitTimeout > 0)
	{
		// ʵ���˳������Ӵ�����Ч��(ͨ����Сview��alphaֵ)
		viewData->dwExitTimeout --;
		alpha = XM_GetWindowAlpha (hWnd);
		alpha = (float)(alpha * viewData->dwExitTimeout / 10.0);
		XM_printf ("alpha=%d\n", (UCHAR)(alpha));
		// �����Ӵ���alphaֵ
		XM_SetWindowAlpha (hWnd, (UCHAR)(alpha));

		XM_InvalidateWindow ();
	}
	else
	{
		// ���ص��������Ӵ�
		XM_PullWindow (0);
	}
}

// ���������޷�����ϵͳ��Ϣ�¼����˳�������ϵͳȱʡ����
VOID SoundVolumeViewOnSystemEvent (XMMSG *msg)
{
	// ���ص��������Ӵ�
	XM_BreakSystemEventDefaultProcess (msg);
	XM_PostMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
	XM_PullWindow (0);
}


// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (SoundVolumeView)
	XM_ON_MESSAGE (XM_PAINT, SoundVolumeViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, SoundVolumeViewOnKeyDown)
	XM_ON_MESSAGE (XM_ENTER, SoundVolumeViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, SoundVolumeViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, SoundVolumeViewOnTimer)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, SoundVolumeViewOnSystemEvent)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, LCD_YDOTS - 120, LCD_XDOTS, 120, SoundVolumeView, 1, 0, 0, 255, HWND_ALERT)
