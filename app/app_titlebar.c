//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_titlbar.c
//	  �������ؼ�
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuid.h"



// ���ư�ť�ؼ�
VOID AP_DrawTitlebarControl (HANDLE hWnd, DWORD dwIconId, DWORD dwTextID)
{
	XMRECT rc, rcView;
	float scale_factor;		// ˮƽ��������

	XM_GetDesktopRect (&rc);
	XM_GetWindowRect (hWnd, &rcView);

	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);
	// --------------------------------------
	//
	// ********* 1 ��ʾ����������Ϣ *********
	//
	// --------------------------------------
	// ��ʾ�������ı���
	rc.left = (XMCOORD)(APP_POS_TITLEBAR_X * scale_factor);
	rc.top = APP_POS_TITLEBAR_Y;
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUTITLEBARBACKGROUND, hWnd, &rc, XMGIF_DRAW_POS_LEFTTOP);

	// �������ײ�����һ���ָ���
	rc.left = (XMCOORD)(APP_POS_TITLEBAR_X * scale_factor);
	rc.top = APP_TITLEBAR_HEIGHT - 1;
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hWnd, &rc, XMGIF_DRAW_POS_LEFTTOP);

	// ��ʾ��������ͼ��
	if(dwIconId != AP_NULLID)
	{
		rc.left = (XMCOORD)(APP_POS_TITLEBAR_ICON_X * scale_factor);
		rc.top = APP_POS_TITLEBAR_ICON_Y;

		AP_RomImageDrawByMenuID (dwIconId, hWnd, &rc, XMGIF_DRAW_POS_LEFTTOP);
	}
	
	// ��ʾ�������ı���
	if(dwTextID != AP_NULLID)
	{
		rc.left = rcView.left;
		rc.right = rcView.right;
		rc.top = 0;
		rc.bottom = APP_TITLEBAR_HEIGHT - 1;

		AP_RomImageDrawByMenuID (dwTextID, hWnd, &rc, XMGIF_DRAW_POS_CENTER);
	}
	
}

VOID AP_TitleBarControlPaint (XMTITLEBARCONTROL *pTitleBarControl)
{
	XMRECT rc, rcView;
	float scale_factor;		// ˮƽ��������
	HANDLE hWnd; 
	if(pTitleBarControl == NULL)
		return;

	hWnd = pTitleBarControl->hWnd;

	XM_GetDesktopRect (&rc);
	XM_GetWindowRect (hWnd, &rcView);

	// ������������
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);
	// --------------------------------------
	//
	// ********* 1 ��ʾ����������Ϣ *********
	//
	// --------------------------------------
	// ��ʾ�������ı���
	rc.left = (XMCOORD)(APP_POS_TITLEBAR_X * scale_factor);
	rc.top = APP_POS_TITLEBAR_Y;
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUTITLEBARBACKGROUND, hWnd, &rc, XMGIF_DRAW_POS_LEFTTOP);

	// �������ײ�����һ���ָ���
	//rc.left = (XMCOORD)(APP_POS_TITLEBAR_X * scale_factor);
	//rc.top = APP_TITLEBAR_HEIGHT - 1;
	//AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hWnd, &rc, XMGIF_DRAW_POS_LEFTTOP);

	// ��ʾ��������ͼ��
	rc.left = (XMCOORD)(APP_POS_TITLEBAR_ICON_X * scale_factor);
	rc.top = APP_POS_TITLEBAR_ICON_Y;
	if(pTitleBarControl->dwIconID != AP_NULLID)
	{
		AP_RomImageDrawByMenuID (pTitleBarControl->dwIconID, hWnd, &rc, XMGIF_DRAW_POS_LEFTTOP);
	}
	
	// ��ʾ�������ı���
#if 0
	if(pTitleBarControl->pImageText)
	{
		// ������ʾ
		x = rcView.left + (rcView.right - rcView.left + 1 - pTitleBarControl->pImageText->width)/2;
		y = rcView.top + (APP_TITLEBAR_HEIGHT - pTitleBarControl->pImageText->height)/2;
		XM_ImageDisplay (pTitleBarControl->pImageText, hWnd, x, y);
	}
#else
	if(pTitleBarControl->dwTextID != AP_NULLID)
	{
		rc.left = rcView.left;
		rc.right = rcView.right;
		rc.top = 0;
		rc.bottom = APP_TITLEBAR_HEIGHT - 1;
		AP_RomImageDrawByMenuID (pTitleBarControl->dwTextID, hWnd, &rc, XMGIF_DRAW_POS_CENTER);
	}
#endif
}

XMBOOL AP_TitleBarControlInit (XMTITLEBARCONTROL *pTitleBarControl, HANDLE hWnd, DWORD dwIconID, DWORD dwTextID)
{
	if(pTitleBarControl == NULL)
		return 0;
	if(hWnd == NULL)
		return 0;

	pTitleBarControl->hWnd = hWnd;
	pTitleBarControl->dwIconID = dwIconID;
	pTitleBarControl->dwTextID = dwTextID;

	return 1;
}

XMBOOL AP_TitleBarControlExit (XMTITLEBARCONTROL *pTitleBarControl)
{
	if(pTitleBarControl == NULL)
		return 0;
	// �ͷ���Դ
#if 0
	XM_ImageDelete (pTitleBarControl->pImageIcon);
	XM_ImageDelete (pTitleBarControl->pImageText);
	XM_ImageDelete (pTitleBarControl->pImageMenuItemSplitLine);
	XM_ImageDelete (pTitleBarControl->pImageMenuTitleBarBackground);
#endif
	return 1;
}

VOID AP_TitleBarControlMessageHandler (XMTITLEBARCONTROL *pTitleBarControl, XMMSG *msg)
{
	if(pTitleBarControl == NULL || msg == NULL)
		return;
	if(msg->message == XM_PAINT)
	{
		AP_TitleBarControlPaint (pTitleBarControl);
	}
}

