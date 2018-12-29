//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_button.c
//	  ͨ�ð�ť�ؼ�(���֧��4����ť)
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuid.h"

#define	XMBTNINIT	0x54

// ��ť�ؼ���ʼ��
XMBOOL AP_ButtonControlInit (XMBUTTONCONTROL *pButtonControl, BYTE bBtnCount, HANDLE hWnd, const XMBUTTONINFO* pButtonInfo)
{
	//APPROMRES *AppRes;
	//int i;
	if(pButtonControl == NULL)
		return 0;
	if(pButtonInfo == NULL)
		return 0;
	if(hWnd == NULL)
		return 0;
	// ��ʱֻ֧��1, 2, 3, 4��Button��ʽ
	if(bBtnCount < 1 || bBtnCount > XM_MAX_BUTTON_COUNT)
		return 0;

	pButtonControl->bBtnCount = bBtnCount;
	pButtonControl->hWnd = hWnd;
	pButtonControl->dwBtnBackgroundId = (DWORD)-1;
	//pButtonControl->pBtnInfo = pButtonInfo;
	pButtonControl->dwBtnFlag = 0;

	// ��ǰ�ť�ؼ��ѳ�ʼ��
	pButtonControl->bBtnInited = XMBTNINIT;
	pButtonControl->bBtnEnable = 1;
	pButtonControl->bBtnClick = (BYTE)(-1);

	memcpy (pButtonControl->pBtnInfo, pButtonInfo, bBtnCount * sizeof(XMBUTTONINFO));

#if 0
	// Ԥ����ͼ��
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMSPLITLINE);
	if(AppRes)
		pButtonControl->pImageMenuItemSplitLine = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_BUTTONBACKGROUND);
	if(AppRes)
		pButtonControl->pImageButtonBackground = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_BUTTONSPLITLINE);
	if(AppRes)
		pButtonControl->pImageButtonSplitLine = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);

	for (i = 0; i < pButtonControl->bBtnCount; i++)
	{
		AppRes = AP_AppRes2RomRes (pButtonControl->pBtnInfo[i].dwLogoId);
		if(AppRes)
			pButtonControl->pImageLogo[i] = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
		
		AppRes = AP_AppRes2RomRes (pButtonControl->pBtnInfo[i].dwTextId);
		if(AppRes)
			pButtonControl->pImageText[i] = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
	}
#endif

	return 1;
}

// ��ť�ؼ���ʼ��
XMBOOL AP_TpButtonControlInit (TPBUTTONCONTROL *TpButtonControl, BYTE bBtnCount, HANDLE hWnd, const TPBUTTONINFO* pButtonInfo)
{
	//APPROMRES *AppRes;
	//int i;

	if(TpButtonControl == NULL)
		return 0;

	if(pButtonInfo == NULL)
		return 0;

	if(hWnd == NULL)
		return 0;

	// ��ʱֻ֧��1, 2, 3, 4��Button��ʽ
	if(bBtnCount < 1 || bBtnCount > TP_MAX_BUTTON_COUNT)
		return 0;

	TpButtonControl->bBtnCount = bBtnCount;
	TpButtonControl->hWnd = hWnd;
	TpButtonControl->dwBtnBackgroundId = (DWORD)-1;
	//TpButtonControl->pBtnInfo = pButtonInfo;
	TpButtonControl->dwBtnFlag = 0;

	// ��ǰ�ť�ؼ��ѳ�ʼ��
	TpButtonControl->bBtnInited = XMBTNINIT;
	TpButtonControl->bBtnEnable = 1;
	TpButtonControl->bBtnClick = (BYTE)(-1);

	memcpy (TpButtonControl->TpBtnInfo, pButtonInfo, bBtnCount * sizeof(TPBUTTONINFO));

	return 1;
}


// ��ť�ؼ��ͷ�
XMBOOL AP_ButtonControlExit (XMBUTTONCONTROL *TpButtonControl)
{
//	int i;
	if(TpButtonControl == NULL)
		return 0;



	TpButtonControl->bBtnInited = (BYTE)0;	
	TpButtonControl->bBtnEnable = 0;
	return 1;
}
// ��ť�ؼ��ͷ�
XMBOOL AP_TpButtonControlExit (TPBUTTONCONTROL *TpButtonControl)
{
	//int i;
	if(TpButtonControl == NULL)
		return 0;



	TpButtonControl->bBtnInited = (BYTE)0;	
	TpButtonControl->bBtnEnable = 0;
	return 1;
}

// �޸ĵ�����ť������
// pButtonControl		��ť�ؼ�
// bBtnIndex			��Ҫ�޸ĵİ�ť��������� (��Ŵ�0��ʼ)
// pButtonInfo			�µİ�ť��������Ϣ
XMBOOL AP_ButtonControlModify (XMBUTTONCONTROL *pButtonControl, BYTE bBtnIndex, const XMBUTTONINFO* pButtonInfo)
{
	if(pButtonControl == NULL)
		return FALSE;
	if(bBtnIndex >= pButtonControl->bBtnCount)
		return FALSE;
	if(pButtonInfo == NULL)
		return FALSE;
	memcpy (pButtonControl->pBtnInfo + bBtnIndex, pButtonInfo, sizeof(XMBUTTONINFO));
	return TRUE;
}


// ���ð�ť�ı�־״̬, �ı䰴ť����ʾ
VOID  AP_ButtonControlSetFlag (XMBUTTONCONTROL *pButtonControl, DWORD Flag)
{
	if(pButtonControl == NULL)
		return;
	pButtonControl->dwBtnFlag = Flag;
}

DWORD AP_ButtonControlGetFlag (XMBUTTONCONTROL *pButtonControl)
{
	if(pButtonControl == NULL)
		return 0;
	return pButtonControl->dwBtnFlag;
}


// ���ư�ť�ؼ�
static VOID AP_ButtonPaint (XMBUTTONCONTROL *pButtonControl)
{
	XMRECT rc, rect;
	int i, loop;
	XMCOORD x, y;
	int offset;

	// ���ذ�ť������ʾ
	if( (pButtonControl->dwBtnFlag & XMBUTTON_FLAG_HIDE) == XMBUTTON_FLAG_HIDE )
		return;

	XM_GetDesktopRect (&rc);
	rect = rc;

	// ������Ի�׼��(320X240)��ƫ��
	offset = rc.bottom - 240;

	// --------------------------------------
	//
	// ********* 3 ��ʾ��ť�� ***************
	//
	// --------------------------------------
	rect.left = APP_POS_BUTTON_X;
	rect.top = offset + APP_POS_BUTTON_Y;
	// ��ť����������һ��ˮƽ�ָ���
	if(!(pButtonControl->dwBtnFlag & XMBUTTON_FLAG_HIDE_HORZ_SPLIT))
	{
		AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, pButtonControl->hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	}

	rect.top = (XMCOORD)(rect.top + 1);
	// ��䰴ť������ 
	if(!(pButtonControl->dwBtnFlag & XMBUTTON_FLAG_HIDE_BACKGROUND))
	{
		AP_RomImageDrawByMenuID (AP_ID_COMMON_BUTTONBACKGROUND, pButtonControl->hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	}
	
	// ����ť���Ĵ�ֱ�ָ���
	if(pButtonControl->bBtnCount == 1)
	{
		// 1����2��Button������1���ָ���
		loop = 1;
	}
	else
	{
		loop = pButtonControl->bBtnCount - 1;
	}
	if(!(pButtonControl->dwBtnFlag & XMBUTTON_FLAG_HIDE_VERT_SPLIT))
	{
		for (i = 0; i < loop; i ++)
		{
			rect.left = (XMCOORD)( (i + 1) * (rc.right - rc.left + 1) / (loop + 1) - 1);
			rect.top = offset + APP_POS_BUTTON_SPLITLINE_Y;
			AP_RomImageDrawByMenuID (AP_ID_COMMON_BUTTONSPLITLINE, pButtonControl->hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
		}
	}

	#if 0
	// ����ť��LOGO
	x = (XMCOORD)((rc.right - rc.left + 1) / ((loop+1)*2));
	y = offset + APP_POS_BUTTON_LOGO_Y;

	rect.left = x;
	rect.top = y;
	rect.right = x;
	rect.bottom = y;
	for (i = 0; i < pButtonControl->bBtnCount; i++)
	{
		XMRECT rc_backup = rect;
		
		// ��鰴ť�Ƿ��Ѱ���
		if(pButtonControl->bBtnClick == (BYTE)i)
		{
			XM_OffsetRect (&rect, 5, 5); // 1 1
		}

		AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, pButtonControl->hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		AP_RomImageDrawByMenuID (pButtonControl->pBtnInfo[i].dwLogoId, pButtonControl->hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		
		rect = rc_backup;
		XM_OffsetRect (&rect, (XMCOORD)((rc.right - rc.left + 1) / (loop+1)), 0);
	}
    #else
	// ����ť������
	x = (XMCOORD)((rc.right - rc.left + 1) / ((loop+1)*2));
	y = offset + APP_POS_BUTTON_TEXT_Y;

	rect.left = x;
	rect.top = y-20;
	rect.right = x;
	rect.bottom = y;
	for (i = 0; i < pButtonControl->bBtnCount; i++)
	{
		XMRECT rc_backup = rect;
		
		// ��鰴ť�Ƿ��Ѱ���
		if(pButtonControl->bBtnClick == (BYTE)i)
		{
			XM_OffsetRect (&rect, 5, 5);
		}
		AP_RomImageDrawByMenuID (pButtonControl->pBtnInfo[i].dwTextId, pButtonControl->hWnd, &rect, XMGIF_DRAW_POS_CENTER);

		rect = rc_backup;
		XM_OffsetRect (&rect, (XMCOORD)((rc.right - rc.left + 1) / (loop+1)), 0);
	}
	#endif
}


// ���ư�ť�ؼ�
static VOID AP_TpButtonPaint (TPBUTTONCONTROL *TpButtonControl)
{
	XMRECT rc, rect;
	int i, loop;
	XMCOORD x, y;
	int offset;

	// ���ذ�ť������ʾ
	if( (TpButtonControl->dwBtnFlag & XMBUTTON_FLAG_HIDE) == XMBUTTON_FLAG_HIDE )
		return;

	XM_GetDesktopRect (&rc);
	rect = rc;

	for (i = 0; i < TpButtonControl->bBtnCount; i++)
	{
		XMRECT rc_backup;
		
		rect.left = TpButtonControl->TpBtnInfo[i].left;
	    rect.top = TpButtonControl->TpBtnInfo[i].top;
	    rect.right = TpButtonControl->TpBtnInfo[i].right;
	    rect.bottom = TpButtonControl->TpBtnInfo[i].bottom;
		rc_backup = rect;
		
		// ��鰴ť�Ƿ��Ѱ��£�ѡ��λ����΢�ƶ�
		if(TpButtonControl->bBtnClick == (BYTE)i)
		{
			//XM_OffsetRect (&rect, 5, 5);
			AP_RomImageDrawByMenuID (TpButtonControl->TpBtnInfo[i].ClickdwLogoId, TpButtonControl->hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		}
		else
		{
            AP_RomImageDrawByMenuID (TpButtonControl->TpBtnInfo[i].dwLogoId, TpButtonControl->hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		}
		rect = rc_backup;
	}
}

// ֱ�����´�����framebuffer�Ͻ������ (��ʾ�Ĺ��̲�����)
// 1) ���������������framebuffer����
// 2) ���´�����framebuffer�ϻ��ư�ť����
// 3) �ͷ��´�����framebuffer (����������һ֡��OSD��ʾ���)
static void AP_ButtonDirectPaint (XMBUTTONCONTROL *pButtonControl)
{
	XM_OSD_FRAMEBUFFER *framebuffer, *old_framebuffer;
	XMRECT rc, rect;
	int offset;
	XMCOLOR bkg_color = XM_GetSysColor(XM_COLOR_DESKTOP);
	unsigned int old_alpha;
					
	XM_GetDesktopRect (&rc);

	// ����ʹ��2�ֲ�ͬ��������ֱ��д������
	//
	// 1) ���Ƶ�ǰ������ʾ��֡���沢����һ���µ�framebuffer����Ȼ��������µĶ����ϻ��ƣ�
	//		������Ϻ��´�����framebuffer�����ͷţ��ȴ���ʾʹ���µ�֡���ݸ��¡�
	//		�������ݵĹ�������(�û�������)
	//
	// 2) ֱ��ʹ���뵱ǰ��ʾ��֡���������framebuffer���� Ȼ������������ϻ��ƣ�
	//		������Ϻ�framebuffer�����ͷ�
	//		�������ݵĹ��̿��ӣ����ܻ����ĳЩ��˸����(�����򱳾��������Ҳ�����ʱ�̸�����������ʾˢ��)
	//
#if 1
	// 1) ����һ���µ�framebuffer, �����Ƶ�ǰ��ʾframebuffer������
	framebuffer = XM_osd_framebuffer_create (XM_LCDC_CHANNEL_0,
													XM_OSD_LAYER_1, 
													XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
													rc.right - rc.left + 1,
													rc.bottom - rc.top + 1,
													pButtonControl->hWnd,
													1,			// ���������֡����(�ӵ�ǰ������ʾ��framebuffer�����֡���渴��)
													0			// ��ʹ�ñ���ɫ���֡����
													);
#else
	// 2) ʹ�������ǰ������ʾ��framebuffer
	framebuffer = XM_osd_framebuffer_open (XM_LCDC_CHANNEL_0,
													XM_OSD_LAYER_1, 
													XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
													rc.right - rc.left + 1,
													rc.bottom - rc.top + 1);
#endif

	if(framebuffer)
	{
		// �����ڵ�framebuffer����Ϊ��ǰ��ʾ��framebuffer
		old_framebuffer = XM_GetWindowFrameBuffer (pButtonControl->hWnd);
		XM_SetWindowFrameBuffer (pButtonControl->hWnd, framebuffer);

		rect = rc;

		// ������Ի�׼��(320X240)��ƫ��
		offset = rc.bottom - 240;

		// --------------------------------------
		//
		// ********* 3 ��ʾ��ť�� ***************
		//
		// --------------------------------------
		// ˢ�°�ť���ڵı�����
		rect.left = APP_POS_BUTTON_X;
		rect.top = offset + APP_POS_BUTTON_Y;
		XM_FillRect (pButtonControl->hWnd, rect.left, rect.top, rect.right, rect.bottom, bkg_color);

		old_alpha = XM_GetWindowAlpha (pButtonControl->hWnd);
		XM_SetWindowAlpha (pButtonControl->hWnd, 255);

		// ���ư�ť����
		AP_ButtonPaint (pButtonControl);

		XM_SetWindowAlpha (pButtonControl->hWnd, (unsigned char)old_alpha);

		XM_SetWindowFrameBuffer (pButtonControl->hWnd, old_framebuffer);

		// �ͷ�framebuffer���ȴ���ʾˢ��
		XM_osd_framebuffer_close (framebuffer, 0);
	}
	else
	{
		// ʹ��ȫ����ˢ��ģʽ
		XM_InvalidateWindow ();
		XM_UpdateWindow ();
	}
}

// ֱ�����´�����framebuffer�Ͻ������ (��ʾ�Ĺ��̲�����)
// 1) ���������������framebuffer����
// 2) ���´�����framebuffer�ϻ��ư�ť����
// 3) �ͷ��´�����framebuffer (����������һ֡��OSD��ʾ���)
static void AP_TpButtonDirectPaint (TPBUTTONCONTROL *TpButtonControl)
{
	XM_OSD_FRAMEBUFFER *framebuffer, *old_framebuffer;
	XMRECT rc, rect;
	int offset;
	XMCOLOR bkg_color = XM_GetSysColor(XM_COLOR_DESKTOP);
	unsigned int old_alpha;
					
	XM_GetDesktopRect (&rc);

	// ����ʹ��2�ֲ�ͬ��������ֱ��д������
	//
	// 1) ���Ƶ�ǰ������ʾ��֡���沢����һ���µ�framebuffer����Ȼ��������µĶ����ϻ��ƣ�
	//		������Ϻ��´�����framebuffer�����ͷţ��ȴ���ʾʹ���µ�֡���ݸ��¡�
	//		�������ݵĹ�������(�û�������)
	//
	// 2) ֱ��ʹ���뵱ǰ��ʾ��֡���������framebuffer���� Ȼ������������ϻ��ƣ�
	//		������Ϻ�framebuffer�����ͷ�
	//		�������ݵĹ��̿��ӣ����ܻ����ĳЩ��˸����(�����򱳾��������Ҳ�����ʱ�̸�����������ʾˢ��)
	//
#if 1
	// 1) ����һ���µ�framebuffer, �����Ƶ�ǰ��ʾframebuffer������
	framebuffer = XM_osd_framebuffer_create (XM_LCDC_CHANNEL_0,
													XM_OSD_LAYER_1, 
													XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
													rc.right - rc.left + 1,
													rc.bottom - rc.top + 1,
													TpButtonControl->hWnd,
													1,			// ���������֡����(�ӵ�ǰ������ʾ��framebuffer�����֡���渴��)
													0			// ��ʹ�ñ���ɫ���֡����
													);
#else
	// 2) ʹ�������ǰ������ʾ��framebuffer
	framebuffer = XM_osd_framebuffer_open (XM_LCDC_CHANNEL_0,
													XM_OSD_LAYER_1, 
													XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
													rc.right - rc.left + 1,
													rc.bottom - rc.top + 1);
#endif

	if(framebuffer)
	{
		// �����ڵ�framebuffer����Ϊ��ǰ��ʾ��framebuffer
		old_framebuffer = XM_GetWindowFrameBuffer (TpButtonControl->hWnd);
		XM_SetWindowFrameBuffer (TpButtonControl->hWnd, framebuffer);

            #if 0
		rect = rc;

		// ������Ի�׼��(320X240)��ƫ��
		offset = rc.bottom - 240;

		// --------------------------------------
		//
		// ********* 3 ��ʾ��ť�� ***************
		//
		// --------------------------------------
		// ˢ�°�ť���ڵı�����
		rect.left = APP_POS_BUTTON_X;
		rect.top = offset + APP_POS_BUTTON_Y;
		#endif
		//XM_FillRect (pButtonControl->hWnd,pButtonControl->pBtnInfo[pButtonControl->bBtnClick].left,pButtonControl->pBtnInfo[pButtonControl->bBtnClick].top, pButtonControl->pBtnInfo[pButtonControl->bBtnClick].right,pButtonControl->pBtnInfo[pButtonControl->bBtnClick].bottom, bkg_color);

		old_alpha = XM_GetWindowAlpha (TpButtonControl->hWnd);
		XM_SetWindowAlpha (TpButtonControl->hWnd, 255);

		// ���ư�ť����
		AP_TpButtonPaint (TpButtonControl);

		XM_SetWindowAlpha (TpButtonControl->hWnd, (unsigned char)old_alpha);

		XM_SetWindowFrameBuffer (TpButtonControl->hWnd, old_framebuffer);

		// �ͷ�framebuffer���ȴ���ʾˢ��
		XM_osd_framebuffer_close (framebuffer, 0);
	}
	else
	{
		// ʹ��ȫ����ˢ��ģʽ
		XM_InvalidateWindow ();
		XM_UpdateWindow ();
	}
}

static int testBtnKey (XMBUTTONCONTROL *pButtonControl, WORD key)
{
	int i;
	for (i = 0; i < pButtonControl->bBtnCount; i++)
	{
		if(pButtonControl->pBtnInfo[i].wKey == key)
			return i;
	}
	return -1;
}
static int testTpBtnKey (TPBUTTONCONTROL *TpButtonControl, WORD key)
{
	int i;
	for (i = 0; i < TpButtonControl->bBtnCount; i++)
	{
		if(TpButtonControl->TpBtnInfo[i].wKey == key)
			return i;
	}
	return -1;
}
// ʹ�ܻ��ֹ��ť
XMBOOL AP_ButtonControlSetEnable (XMBUTTONCONTROL *pButtonControl, XMBOOL bEnable)
{
	// ��鰴ť�ؼ��Ƿ��ѳ�ʼ��
	if(pButtonControl->bBtnInited != XMBTNINIT)
		return 0;
	pButtonControl->bBtnEnable = bEnable;
	return 1;
}

#define TP_LEFT     		500
#define TP_WIDTH           600

static int check_button_clicked (XMBUTTONCONTROL *pButtonControl, XMMSG *msg)
{
	XMRECT rc, rect;
	 int x, y,width;
	int button_index = -1;
	 int offset;
	int count;

	if(!pButtonControl->bBtnEnable || !pButtonControl->bBtnInited)
		return -1;

	XM_GetDesktopRect (&rc);
	rect = rc;

	// ������Ի�׼��(320X240)��ƫ��
	offset = rc.bottom - APP_BUTTON_HEIGHT;
	x = LOWORD(msg->lp);
	y = HIWORD(msg->lp);

	if((y>rc.bottom)||(y < offset))
		return -1;

	count = pButtonControl->bBtnCount;
	if(count == 1)
		count ++;
    
	//button_index = x * count / (rc.right - rc.left + 1);
 	width = TP_WIDTH/count;
    button_index = (unsigned int)(x - TP_LEFT)/width;
	
	if(button_index >= count)
		return -1;
	
	return button_index;
}
static int check_TPbutton_clicked (TPBUTTONCONTROL *TpButtonControl, XMMSG *msg)
{
	#define OFFSET 	10
	
	unsigned int x, y;
	int button_index = -1;
	int count;
	
	if(!TpButtonControl->bBtnEnable || !TpButtonControl->bBtnInited)
		return -1;

	x = LOWORD(msg->lp)-530+48;
	y = HIWORD(msg->lp);

	if((y<100)||(y>300))
		return -1;
	
    for(button_index=0;button_index<TpButtonControl->bBtnCount;button_index++)
    {
	    //if (x >= (TpButtonControl->TpBtnInfo[button_index].left - OFFSET) && x < (TpButtonControl->TpBtnInfo[button_index].right + OFFSET) && y >( TpButtonControl->TpBtnInfo[button_index].top - OFFSET-50) && y < (TpButtonControl->TpBtnInfo[button_index].bottom + OFFSET-100))
	    if (x >= (TpButtonControl->TpBtnInfo[button_index].left - OFFSET) && x < (TpButtonControl->TpBtnInfo[button_index].right + OFFSET) && y >( TpButtonControl->TpBtnInfo[button_index].top - OFFSET) && y < (TpButtonControl->TpBtnInfo[button_index].bottom + OFFSET))
		{
	         //XM_printf("button_index is %d\n",button_index);
	         return button_index;
	    }
    }
    return -1;
}




/*��Ƶ����ʱ�����ؼ���Ҫ�찴����*/
XMBOOL AP_VideoplayingButtonControlMessageHandler (XMBUTTONCONTROL *pButtonControl, XMMSG *msg)
{
	int i;
	XMBOOL bButtonProcessed = 0;
	
	// ��鰴ť�ؼ��Ƿ��ѳ�ʼ��
	if(pButtonControl->bBtnInited != XMBTNINIT)
		return bButtonProcessed;

	if(msg->message == XM_KEYDOWN && pButtonControl->bBtnEnable)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// ���ü��Ƿ񱻶���Ϊ��ť�ؼ��Ĺ��ܼ�
				i = testBtnKey (pButtonControl, msg->wp);
				if(i == (-1))
				{
					// û�б�����Ϊ��ť�ؼ��Ĺ��ܼ�
					// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
					if(pButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
					pButtonControl->bBtnClick = (BYTE)(-1);
					// ֱ��д��ˢ��
					AP_ButtonDirectPaint (pButtonControl);
					// ˢ��
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					return bButtonProcessed;
				}

				// ��鰴ť�Ƿ���ѡ��
				if(pButtonControl->bBtnClick == (BYTE)i)
					return bButtonProcessed;
				pButtonControl->bBtnClick = (BYTE)i;

				AP_ButtonDirectPaint (pButtonControl);
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				break;

			default:
				// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
				if(pButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
				pButtonControl->bBtnClick = (BYTE)(-1);
				// ˢ��
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_ButtonDirectPaint (pButtonControl);
				break;
		}
	}
	else if(msg->message == XM_KEYUP && pButtonControl->bBtnEnable)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// ���ü��Ƿ񱻶���Ϊ��ť�ؼ��Ĺ��ܼ�
				i = testBtnKey (pButtonControl, msg->wp);
				if(i == (-1))
				{
					// û�б�����Ϊ��ť�ؼ��Ĺ��ܼ�
					// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
					if(pButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
					pButtonControl->bBtnClick = (BYTE)(-1);
					// ˢ��
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					AP_ButtonDirectPaint (pButtonControl);
					return bButtonProcessed;
				}
				if(pButtonControl->bBtnClick != (BYTE)i)
					return bButtonProcessed;
				pButtonControl->bBtnClick = (BYTE)(-1);
				XM_PostMessage (XM_COMMAND, pButtonControl->pBtnInfo[i].wCommand, i);

				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_ButtonDirectPaint (pButtonControl);
				break;

			default:
				// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
				if(pButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
				pButtonControl->bBtnClick = (BYTE)(-1);
				// ˢ��
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_ButtonDirectPaint (pButtonControl);
				break;
		}
	}
	else if(msg->message == XM_PAINT)
	{
		AP_ButtonPaint (pButtonControl);
	}
	else if(msg->message == XM_TOUCHDOWN)
	{
		int button_click_index;
	
		// �쳣�������
		if(pButtonControl->bBtnClick  != (BYTE)(-1))
		{
			pButtonControl->bBtnClick = (BYTE)(-1);
			AP_ButtonDirectPaint (pButtonControl);
		}
		
		button_click_index = check_button_clicked (pButtonControl, msg);
		
		if(button_click_index < 0)
			return bButtonProcessed;

		bButtonProcessed = 1;
		if(button_click_index < pButtonControl->bBtnCount)
		{
		      
			pButtonControl->bBtnClick = (BYTE)button_click_index;
			AP_ButtonDirectPaint (pButtonControl);
		}
	}
	else if(msg->message == XM_TOUCHUP)
	{
		if(pButtonControl->bBtnClick != (BYTE)(-1))
		{
			int i = pButtonControl->bBtnClick;
			pButtonControl->bBtnClick = (BYTE)(-1);
			AP_ButtonDirectPaint (pButtonControl);
			XM_PostMessage (XM_COMMAND, pButtonControl->pBtnInfo[i].wCommand, i);
			bButtonProcessed = 1;
		}
	}
	else if(msg->message == XM_TOUCHMOVE)
	{
		if(pButtonControl->bBtnClick != (BYTE)(-1))
		{
			bButtonProcessed = 1;
		}
	}
	return bButtonProcessed;
}

XMBOOL AP_ButtonControlMessageHandler(XMBUTTONCONTROL *pButtonControl, XMMSG *msg)
{
	int i;
	XMBOOL bButtonProcessed = 0;
	
	// ��鰴ť�ؼ��Ƿ��ѳ�ʼ��
	if(pButtonControl->bBtnInited != XMBTNINIT)
		return bButtonProcessed;

	XM_printf(">>>>AP_ButtonControlMessageHandler, msg->message:%x\r\n", msg->message);
	XM_printf(">>>>AP_ButtonControlMessageHandler, pButtonControl->bBtnEnable:%x\r\n", pButtonControl->bBtnEnable);
	if(msg->message == XM_KEYDOWN && pButtonControl->bBtnEnable)
	{
		XM_printf(">>>>>>>>>>>msg->wp:%x\r\n", msg->wp);
		switch(msg->wp)
		{
			//case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// ���ü��Ƿ񱻶���Ϊ��ť�ؼ��Ĺ��ܼ�
				i = testBtnKey (pButtonControl, msg->wp);
				XM_printf(">>>>i:%d\r\n", i);
				if(i == (-1))
				{
					// û�б�����Ϊ��ť�ؼ��Ĺ��ܼ�

					// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
					if(pButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
					pButtonControl->bBtnClick = (BYTE)(-1);
					// ֱ��д��ˢ��
					AP_ButtonDirectPaint (pButtonControl);
					// ˢ��
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					return bButtonProcessed;
				}
				XM_printf(">>>>pButtonControl->bBtnClick:%d\r\n", pButtonControl->bBtnClick);

				// ��鰴ť�Ƿ���ѡ��
				if(pButtonControl->bBtnClick == (BYTE)i)
					return bButtonProcessed;
				pButtonControl->bBtnClick = (BYTE)i;
				
				XM_PostMessage (XM_COMMAND, pButtonControl->pBtnInfo[i].wCommand, i);//luo
				AP_ButtonDirectPaint (pButtonControl);
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				break;

			default:
				// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
				if(pButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
				pButtonControl->bBtnClick = (BYTE)(-1);
				// ˢ��
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_ButtonDirectPaint (pButtonControl);
				break;
		}
	}
	else if(msg->message == XM_KEYUP && pButtonControl->bBtnEnable)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// ���ü��Ƿ񱻶���Ϊ��ť�ؼ��Ĺ��ܼ�
				i = testBtnKey (pButtonControl, msg->wp);
				if(i == (-1))
				{
					// û�б�����Ϊ��ť�ؼ��Ĺ��ܼ�
					// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
					if(pButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
					pButtonControl->bBtnClick = (BYTE)(-1);
					// ˢ��
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					AP_ButtonDirectPaint (pButtonControl);
					return bButtonProcessed;
				}
				if(pButtonControl->bBtnClick != (BYTE)i)
					return bButtonProcessed;
				pButtonControl->bBtnClick = (BYTE)(-1);
				XM_PostMessage(XM_COMMAND, pButtonControl->pBtnInfo[i].wCommand, i);

				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_ButtonDirectPaint (pButtonControl);
				break;

			default:
				// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
				if(pButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
				pButtonControl->bBtnClick = (BYTE)(-1);
				// ˢ��
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_ButtonDirectPaint (pButtonControl);
				break;
		}
	}
	else if(msg->message == XM_PAINT)
	{
		AP_ButtonPaint (pButtonControl);
	}
	else if(msg->message == XM_TOUCHDOWN)
	{
		int button_click_index;
	
		// �쳣�������
		if(pButtonControl->bBtnClick  != (BYTE)(-1))
		{
			pButtonControl->bBtnClick = (BYTE)(-1);
			AP_ButtonDirectPaint (pButtonControl);
		}
		
		button_click_index = check_button_clicked (pButtonControl, msg);
		
		if(button_click_index < 0)
			return bButtonProcessed;

		bButtonProcessed = 1;
		if(button_click_index < pButtonControl->bBtnCount)
		{
		    XM_Beep (XM_BEEP_KEYBOARD);//eason
			pButtonControl->bBtnClick = (BYTE)button_click_index;
			AP_ButtonDirectPaint (pButtonControl);
		}
	}
	else if(msg->message == XM_TOUCHUP)
	{
		if(pButtonControl->bBtnClick != (BYTE)(-1))
		{
			int i = pButtonControl->bBtnClick;
			pButtonControl->bBtnClick = (BYTE)(-1);
			AP_ButtonDirectPaint (pButtonControl);
			XM_PostMessage (XM_COMMAND, pButtonControl->pBtnInfo[i].wCommand, i);
			bButtonProcessed = 1;
		}
	}
	else if(msg->message == XM_TOUCHMOVE)
	{
		if(pButtonControl->bBtnClick != (BYTE)(-1))
		{
			bButtonProcessed = 1;
		}
	}
	return bButtonProcessed;
}


XMBOOL AP_TpButtonControlMessageHandlerDateSet (TPBUTTONCONTROL *TpButtonControl, XMMSG *msg)
{
	int i;
	XMBOOL bButtonProcessed = 0;

	// ��鰴ť�ؼ��Ƿ��ѳ�ʼ��
	if(TpButtonControl->bBtnInited != XMBTNINIT)
		return bButtonProcessed;
	
	if(msg->message == XM_KEYDOWN && TpButtonControl->bBtnEnable)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// ���ü��Ƿ񱻶���Ϊ��ť�ؼ��Ĺ��ܼ�
				i = testTpBtnKey (TpButtonControl, msg->wp);
				if(i == (-1))
				{
					// û�б�����Ϊ��ť�ؼ��Ĺ��ܼ�

					// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
					if(TpButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
					TpButtonControl->bBtnClick = (BYTE)(-1);
					// ֱ��д��ˢ��
					AP_TpButtonDirectPaint (TpButtonControl);
					// ˢ��
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					return bButtonProcessed;
				}

				// ��鰴ť�Ƿ���ѡ��
				if(TpButtonControl->bBtnClick == (BYTE)i)
					return bButtonProcessed;
				TpButtonControl->bBtnClick = (BYTE)i;

				AP_TpButtonDirectPaint (TpButtonControl);
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				break;

			default:
				// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
				if(TpButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
				TpButtonControl->bBtnClick = (BYTE)(-1);
				// ˢ��
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_TpButtonDirectPaint (TpButtonControl);
				break;
		}
	}
	else if(msg->message == XM_KEYUP && TpButtonControl->bBtnEnable)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// ���ü��Ƿ񱻶���Ϊ��ť�ؼ��Ĺ��ܼ�
				i = testTpBtnKey (TpButtonControl, msg->wp);
				if(i == (-1))
				{
					// û�б�����Ϊ��ť�ؼ��Ĺ��ܼ�
					// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
					if(TpButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
					TpButtonControl->bBtnClick = (BYTE)(-1);
					// ˢ��
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					AP_TpButtonDirectPaint (TpButtonControl);
					return bButtonProcessed;
				}
				if(TpButtonControl->bBtnClick != (BYTE)i)
					return bButtonProcessed;
				TpButtonControl->bBtnClick = (BYTE)(-1);
				XM_PostMessage (XM_COMMAND, TpButtonControl->TpBtnInfo[i].wCommand, i);
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_TpButtonDirectPaint (TpButtonControl);
				break;

			default:
				// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
				if(TpButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
				TpButtonControl->bBtnClick = (BYTE)(-1);
				// ˢ��
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_TpButtonDirectPaint (TpButtonControl);
				break;
		}
	}
	else if(msg->message == XM_PAINT)
	{
		AP_TpButtonPaint (TpButtonControl);
	}
	else if(msg->message == XM_TOUCHDOWN)
	{
		int button_click_index;
		
		// �쳣�������
		if(TpButtonControl->bBtnClick  != (BYTE)(-1))
		{
			TpButtonControl->bBtnClick = (BYTE)(-1);
			AP_TpButtonDirectPaint (TpButtonControl);
		}
		button_click_index = check_TPbutton_clicked (TpButtonControl, msg);
		if(button_click_index < 0)
			return bButtonProcessed;

		bButtonProcessed = 1;
		if(button_click_index < TpButtonControl->bBtnCount)
		{
		    XM_Beep (XM_BEEP_KEYBOARD);//eason
			TpButtonControl->bBtnClick = (BYTE)button_click_index;
			//AP_TpButtonDirectPaint (TpButtonControl);
		}
	}
	else if(msg->message == XM_TOUCHUP)
	{
		if(TpButtonControl->bBtnClick != (BYTE)(-1))
		{
			int i = TpButtonControl->bBtnClick;
			TpButtonControl->bBtnClick = (BYTE)(-1);
			//AP_TpButtonDirectPaint (TpButtonControl);
			XM_PostMessage (XM_COMMAND, TpButtonControl->TpBtnInfo[i].wCommand, i);
			//XM_printf("pButtonControl->pBtnInfo[i].wCommand is%d\n",TpButtonControl->TpBtnInfo[i].wCommand);
			bButtonProcessed = 1;
		}
	}
	else if(msg->message == XM_TOUCHMOVE)
	{
		if(TpButtonControl->bBtnClick != (BYTE)(-1))
		{
			bButtonProcessed = 1;
		}
	}
	return bButtonProcessed;
}


XMBOOL AP_TpButtonControlMessageHandler (TPBUTTONCONTROL *TpButtonControl, XMMSG *msg)
{
	int i;
	XMBOOL bButtonProcessed = 0;
	
	// ��鰴ť�ؼ��Ƿ��ѳ�ʼ��
	if(TpButtonControl->bBtnInited != XMBTNINIT)
		return bButtonProcessed;
	
	if(msg->message == XM_KEYDOWN && TpButtonControl->bBtnEnable)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// ���ü��Ƿ񱻶���Ϊ��ť�ؼ��Ĺ��ܼ�
				i = testTpBtnKey (TpButtonControl, msg->wp);
				if(i == (-1))
				{
					// û�б�����Ϊ��ť�ؼ��Ĺ��ܼ�

					// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
					if(TpButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
					TpButtonControl->bBtnClick = (BYTE)(-1);
					// ֱ��д��ˢ��
					AP_TpButtonDirectPaint (TpButtonControl);
					// ˢ��
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					return bButtonProcessed;
				}

				// ��鰴ť�Ƿ���ѡ��
				if(TpButtonControl->bBtnClick == (BYTE)i)
					return bButtonProcessed;
				TpButtonControl->bBtnClick = (BYTE)i;

				AP_TpButtonDirectPaint (TpButtonControl);
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				break;

			default:
				// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
				if(TpButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
				TpButtonControl->bBtnClick = (BYTE)(-1);
				// ˢ��
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_TpButtonDirectPaint (TpButtonControl);
				break;
		}
	}
	else if(msg->message == XM_KEYUP && TpButtonControl->bBtnEnable)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// ���ü��Ƿ񱻶���Ϊ��ť�ؼ��Ĺ��ܼ�
				i = testTpBtnKey (TpButtonControl, msg->wp);
				if(i == (-1))
				{
					// û�б�����Ϊ��ť�ؼ��Ĺ��ܼ�

					// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
					if(TpButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
					TpButtonControl->bBtnClick = (BYTE)(-1);
					// ˢ��
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					AP_TpButtonDirectPaint (TpButtonControl);
					return bButtonProcessed;
				}
				if(TpButtonControl->bBtnClick != (BYTE)i)
					return bButtonProcessed;
				TpButtonControl->bBtnClick = (BYTE)(-1);
				XM_PostMessage (XM_COMMAND, TpButtonControl->TpBtnInfo[i].wCommand, i);
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_TpButtonDirectPaint (TpButtonControl);
				break;

			default:
				// ��鰴ť�ؼ��Ĺ��ܼ��Ƿ���ڰ��¡����ǣ�����ȡ��
				if(TpButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// ���ڹ��ܼ����¡�ȡ���䰴��Ч����ʾ
				TpButtonControl->bBtnClick = (BYTE)(-1);
				// ˢ��
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_TpButtonDirectPaint (TpButtonControl);
				break;
		}
	}
	else if(msg->message == XM_PAINT)
	{
		AP_TpButtonPaint (TpButtonControl);
	}
	else if(msg->message == XM_TOUCHDOWN)
	{
		int button_click_index;
		
		// �쳣�������
		if(TpButtonControl->bBtnClick  != (BYTE)(-1))
		{
			TpButtonControl->bBtnClick = (BYTE)(-1);
			AP_TpButtonDirectPaint (TpButtonControl);
		}
		button_click_index = check_TPbutton_clicked (TpButtonControl, msg);
		if(button_click_index < 0)
			return bButtonProcessed;

		bButtonProcessed = 1;
		if(button_click_index < TpButtonControl->bBtnCount)
		{
		    XM_Beep (XM_BEEP_KEYBOARD);//eason
			TpButtonControl->bBtnClick = (BYTE)button_click_index;
			AP_TpButtonDirectPaint (TpButtonControl);
		}
	}
	else if(msg->message == XM_TOUCHUP)
	{
		if(TpButtonControl->bBtnClick != (BYTE)(-1))
		{
			int i = TpButtonControl->bBtnClick;
			TpButtonControl->bBtnClick = (BYTE)(-1);
			AP_TpButtonDirectPaint (TpButtonControl);
			XM_PostMessage (XM_COMMAND, TpButtonControl->TpBtnInfo[i].wCommand, i);
			//XM_printf("pButtonControl->pBtnInfo[i].wCommand is%d\n",TpButtonControl->TpBtnInfo[i].wCommand);
			bButtonProcessed = 1;
		}
	}
	else if(msg->message == XM_TOUCHMOVE)
	{
		if(TpButtonControl->bBtnClick != (BYTE)(-1))
		{
			bButtonProcessed = 1;
		}
	}
	return bButtonProcessed;
}

