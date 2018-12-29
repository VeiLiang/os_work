//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_menuoption.c
//	  ͨ��Menuѡ���б�ѡ�񴰿�(ֻ��ѡ��һ��ѡ��)
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"

#define	OSD_AUTO_HIDE_TIMEOUT			10000	

// ˽�������
#define	MENUOPTIONVIEW_COMMAND_MODIFY		0
#define	MENUOPTIONVIEW_COMMAND_RETURN		1

// ���˵�ѡ����ڰ�ť�ؼ�����
#define	MENUOPTIONVIEWBTNCOUNT	2
static const XMBUTTONINFO menuOptionBtn[MENUOPTIONVIEWBTNCOUNT] = {
	{	
		VK_AP_SWITCH,		MENUOPTIONVIEW_COMMAND_MODIFY,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_MODIFY
	},
	{	
		VK_AP_MODE,		MENUOPTIONVIEW_COMMAND_RETURN,	AP_ID_COMMON_OK,	AP_ID_BUTTON_RETURN
	},
};

typedef struct tagMENUOPTIONVIEWDATA {
	int				nTopItem;					// ��һ�����ӵĲ˵���
	int				nCurItem;					// ��ǰѡ��Ĳ˵���
	int				nItemCount;					// �˵������
	int				nTouchItem;					// ��ǰ������, -1 ��ʾû��

	APPMENUOPTION	*lpMenuOptionView;

	XMBUTTONCONTROL	btnControl;				// ��ť�ؼ�
	XMTITLEBARCONTROL	titleControl;			// ����ؼ�

#if 0
	// Ԥ����ͼ����Դ
	XM_IMAGE *		pImgMenuItemSplitLine;	// �ָ���
	XM_IMAGE *		pImageMenuItemBackground;

	XM_IMAGE **		pImageMenuOption;
#endif

} MENUOPTIONVIEWDATA;


VOID AP_OpenMenuOptionView (APPMENUOPTION *lpAppMenuOption)
{
	if(lpAppMenuOption == NULL)
		return;

	XM_PushWindowEx (XMHWND_HANDLE(MenuOptionView), (DWORD)lpAppMenuOption);
}



VOID MenuOptionViewOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
		//APPROMRES *AppRes;
		MENUOPTIONVIEWDATA *menuOptionViewData = XM_calloc (sizeof(MENUOPTIONVIEWDATA));
		APPMENUOPTION *lpAppMenuOption = (APPMENUOPTION *)msg->lp;
		if(menuOptionViewData == NULL)
		{
			XM_printf ("menuOptionViewData XM_calloc failed\n");
			// ʧ�ܷ��ص�����
			XM_PullWindow (0);
			return;
		}
		
		menuOptionViewData->lpMenuOptionView = lpAppMenuOption;
		menuOptionViewData->nItemCount = lpAppMenuOption->wMenuOptionCount;
		menuOptionViewData->nCurItem = lpAppMenuOption->wMenuOptionSelect;
		menuOptionViewData->nTouchItem = -1;
		// �����ʼѡ���λ��
		if(lpAppMenuOption->wMenuOptionCount > APP_MENUOPTIONVIEW_ITEM_COUNT)
		{
			// ѡ�������������������ʾ���б������
			menuOptionViewData->nTopItem = 0;
			while ( (menuOptionViewData->nCurItem - menuOptionViewData->nTopItem) >= APP_MENUOPTIONVIEW_ITEM_COUNT )
			{
				menuOptionViewData->nTopItem ++;
			}
		}
		else
		{
			menuOptionViewData->nTopItem = 0;
		}

		// ��ť�ؼ���ʼ��
		//AP_ButtonControlInit (&menuOptionViewData->btnControl, MENUOPTIONVIEWBTNCOUNT, XMHWND_HANDLE(MenuOptionView), menuOptionBtn);
		// ����ؼ���ʼ��
		AP_TitleBarControlInit (&menuOptionViewData->titleControl, XMHWND_HANDLE(MenuOptionView), 
														lpAppMenuOption->dwWindowIconId, lpAppMenuOption->dwWindowTextId);

#if 0
		AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMSPLITLINE);
		menuOptionViewData->pImgMenuItemSplitLine = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
		AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMBACKGROUND);
		menuOptionViewData->pImageMenuItemBackground = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);

		menuOptionViewData->pImageMenuOption = XM_calloc (sizeof(XM_IMAGE *) * menuOptionViewData->nItemCount);
		if(menuOptionViewData->pImageMenuOption == NULL)
		{
			XM_printf ("menuOptionViewData->pImageMenuOption XM_calloc failed\n");
		}
#endif


		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(MenuOptionView), menuOptionViewData);

	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("MenuOptionView Pull\n");
	}

	// ������ʱ�������ڲ˵�������
	// ����x��Ķ�ʱ��
	XM_SetTimer (XMTIMER_OPTIONVIEW, 200);
}

VOID MenuOptionViewOnLeave (XMMSG *msg)
{
	// ɾ����ʱ��
	XM_KillTimer (XMTIMER_OPTIONVIEW);
	
	if (msg->wp == 0)
	{
		//int i;
		MENUOPTIONVIEWDATA *menuOptionViewData = (MENUOPTIONVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MenuOptionView));
		if(menuOptionViewData)
		{
			#if 0
			XM_ImageDelete (menuOptionViewData->pImgMenuItemSplitLine);
			XM_ImageDelete (menuOptionViewData->pImageMenuItemBackground);
			if(menuOptionViewData->pImageMenuOption)
			{
				for (i = 0; i < menuOptionViewData->nItemCount; i ++)
				{
					XM_ImageDelete (menuOptionViewData->pImageMenuOption[i]);
				}
			}
			#endif
			
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&menuOptionViewData->btnControl);
			// ����ؼ��˳�����
			AP_TitleBarControlExit (&menuOptionViewData->titleControl);

			// �ͷ�˽�����ݾ��
			XM_free (menuOptionViewData);
			
		}
		XM_printf ("MenuOptionView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("MenuOptionView Push\n");
	}
}



VOID MenuOptionViewOnPaint (XMMSG *msg)
{
	int nItem, nLastItem;
	XMRECT rc, rect;
	//APPROMRES *AppRes;
	int i;
	int nVisualCount;
	float scale_factor;		// ˮƽ��������
	unsigned int old_alpha;
	APPMENUOPTION	*lpMenuOptionView;
	HANDLE hwnd = XMHWND_HANDLE(MenuOptionView);
	MENUOPTIONVIEWDATA *menuOptionViewData = (MENUOPTIONVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(menuOptionViewData == NULL)
		return;

	lpMenuOptionView = menuOptionViewData->lpMenuOptionView;
	
	// ��ʾ������
	XM_GetDesktopRect (&rc);
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
	
	// ����ˮƽ��������(UI��320X240������)
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);
    
	// --------------------------------------
	//
	// ********* 1 ��ʾ����������Ϣ *********
	//
	// --------------------------------------
	//AP_DrawTitlebarControl (hwnd, lpMenuOptionView->dwWindowIconId, lpMenuOptionView->dwWindowTextId);
	// �������ؼ�����ʾ��
	// �����ڱ���ؼ����������AP_TitleControlMessageHandlerִ�б���ؼ���ʾ
	AP_TitleBarControlMessageHandler (&menuOptionViewData->titleControl, msg);
    
	// ��ʾ�˵���ˮƽ�ָ���
	//AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMSPLITLINE);
	rect = rc;
	for (i = 0; i < nVisualCount; i++)
	{
		rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * scale_factor);
		rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + (i+1) * APP_POS_ITEM5_LINEHEIGHT);
		//XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
		//XM_ImageDisplay (menuOptionViewData->pImgMenuItemSplitLine, hwnd, rect.left, rect.top);
		AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	}

	// ��䵱ǰѡ�����
	//AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMBACKGROUND);
	rect = rc;
	rect.left = 0;
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (menuOptionViewData->nCurItem - menuOptionViewData->nTopItem) * APP_POS_ITEM5_LINEHEIGHT);
	//XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	//XM_ImageDisplay (menuOptionViewData->pImageMenuItemBackground, hwnd, rect.left, rect.top);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	// ��ʾ�˵���Ŀ 
	rect = rc;
	rect.top = APP_POS_ITEM5_SPLITLINE_Y + 1;
	rect.bottom = APP_POS_ITEM5_SPLITLINE_Y + APP_POS_ITEM5_LINEHEIGHT;
	nLastItem = menuOptionViewData->nItemCount;
	if(nLastItem > (menuOptionViewData->nTopItem + nVisualCount))
		nLastItem = (menuOptionViewData->nTopItem + nVisualCount);
	for (nItem = menuOptionViewData->nTopItem; nItem < nLastItem; nItem ++)
	{
#if 0
		XM_IMAGE *imageOption;
		XMCOORD x, y;

		if(menuOptionViewData->pImageMenuOption)
		{
			imageOption = menuOptionViewData->pImageMenuOption[nItem];
			if(imageOption == NULL)
			{
				AppRes = AP_AppRes2RomRes (lpMenuOptionView->lpdwMenuOptionId[nItem]);
				if(AppRes)
				{
					menuOptionViewData->pImageMenuOption[nItem] = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), 
								AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
					imageOption = menuOptionViewData->pImageMenuOption[nItem];
				}
			}
			//else
			//{
			// AppRes = AP_AppRes2RomRes (lpMenuOptionView->lpdwMenuOptionId[nItem]);
			//	XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_CENTER);	
			//}

			if(imageOption)
			{
				// ������ʾ
				x = rect.left + (rect.right - rect.left + 1 - imageOption->width) / 2;
				y = rect.top + (rect.bottom - rect.top + 1 - imageOption->height) / 2;
				XM_ImageDisplay (imageOption, hwnd, x, y);
			}
		}
#endif

		AP_RomImageDrawByMenuID (lpMenuOptionView->lpdwMenuOptionId[nItem], hwnd, &rect, XMGIF_DRAW_POS_CENTER);
		XM_OffsetRect (&rect, 0, APP_POS_ITEM5_LINEHEIGHT);
	}
	
	// --------------------------------------
	//
	// ********* 3 ��ʾ��ť�� ***************
	//
	// --------------------------------------
	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
	AP_ButtonControlMessageHandler (&menuOptionViewData->btnControl, msg);
    
	XM_SetWindowAlpha (hwnd, (unsigned char)old_alpha);

}


VOID MenuOptionViewOnKeyDown (XMMSG *msg)
{
	int nVisualCount;
	XMRECT rc;
	MENUOPTIONVIEWDATA *menuOptionViewData = (MENUOPTIONVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MenuOptionView));
	if(menuOptionViewData == NULL)
		return;

	XM_GetWindowRect (XMHWND_HANDLE(MenuOptionView), &rc);
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
// ������
	XM_Beep (XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
		// ���ϼ�
		case VK_AP_UP:
        case REMOTE_KEY_UP:
			AP_ButtonControlMessageHandler (&menuOptionViewData->btnControl, msg);
			menuOptionViewData->nCurItem --;
			if(menuOptionViewData->nCurItem < 0)
			{
				// �۽������һ��
				menuOptionViewData->nCurItem = (WORD)(menuOptionViewData->nItemCount - 1);
				menuOptionViewData->nTopItem = 0;
				while ( (menuOptionViewData->nCurItem - menuOptionViewData->nTopItem) >= nVisualCount )
				{
					menuOptionViewData->nTopItem ++;
				}
			}
			else
			{
				if(menuOptionViewData->nTopItem > menuOptionViewData->nCurItem)
					menuOptionViewData->nTopItem = menuOptionViewData->nCurItem;
			}
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_OPTIONVIEW, OSD_AUTO_HIDE_TIMEOUT);
			break;

		// ���¼�
		//case VK_AP_AV:
	case VK_AP_DOWN:
        case REMOTE_KEY_DOWN:
			AP_ButtonControlMessageHandler (&menuOptionViewData->btnControl, msg);
			menuOptionViewData->nCurItem ++;	
			if(menuOptionViewData->nCurItem >= menuOptionViewData->nItemCount)
			{
				// �۽�����һ��
				menuOptionViewData->nTopItem = 0;
				menuOptionViewData->nCurItem = 0;
			}
			else
			{
				while ( (menuOptionViewData->nCurItem - menuOptionViewData->nTopItem) >= nVisualCount )
				{
					menuOptionViewData->nTopItem ++;
				}
			}

			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_OPTIONVIEW, OSD_AUTO_HIDE_TIMEOUT);
			break;
        case VK_AP_MENU:
        case REMOTE_KEY_POWER:
            XM_PullWindow (0);
            break;
        case VK_AP_SWITCH:
       // case VK_AP_DOWN:
        case REMOTE_KEY_MENU:
            XM_PostMessage (XM_COMMAND, MENUOPTIONVIEW_COMMAND_MODIFY, menuOptionViewData->nCurItem);
            break;

	}
}

VOID MenuOptionViewOnKeyUp (XMMSG *msg)
{
	MENUOPTIONVIEWDATA *menuOptionViewData = (MENUOPTIONVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MenuOptionView));
	if(menuOptionViewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_SWITCH:
		case VK_AP_MODE:
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&menuOptionViewData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&menuOptionViewData->btnControl, msg);
			break;

	}
}

VOID MenuOptionViewOnCommand (XMMSG *msg)
{
	MENUOPTIONVIEWDATA *menuOptionViewData;
	menuOptionViewData = (MENUOPTIONVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MenuOptionView));
	if(menuOptionViewData == NULL)
		return;
	switch(msg->wp)
	{
		case MENUOPTIONVIEW_COMMAND_MODIFY:
			// ȷ�ϲ�����
			if(menuOptionViewData->lpMenuOptionView && menuOptionViewData->lpMenuOptionView->fpMenuOptionCB)
			{
				APPMENUOPTION	*lpMenuOptionView = menuOptionViewData->lpMenuOptionView;
				(*lpMenuOptionView->fpMenuOptionCB) (lpMenuOptionView->lpUserData, menuOptionViewData->nCurItem);
			}
			XM_PullWindow (NULL);

			break;

		case MENUOPTIONVIEW_COMMAND_RETURN:
			// ����
			// ���ص�����
			XM_PullWindow (0);
			break;
	}
}


static VOID MenuOptionViewOnTimer (XMMSG *msg)
{
	//if(AHD_Guide_Start())
	    //XM_PullWindow(XMHWND_HANDLE(Desktop));
}


VOID MenuOptionViewOnTouchDown (XMMSG *msg)
{
	int index;
	MENUOPTIONVIEWDATA *menuOptionViewData = (MENUOPTIONVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MenuOptionView));
	if(menuOptionViewData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&menuOptionViewData->btnControl, msg))
		return;

	index = AppLocateItem (XMHWND_HANDLE(MenuOptionView), menuOptionViewData->nItemCount,APP_POS_ITEM5_LINEHEIGHT, menuOptionViewData->nTopItem, LOWORD(msg->lp), HIWORD(msg->lp));
	if(index < 0)
		return;
	menuOptionViewData->nTouchItem = index;
	if(menuOptionViewData->nCurItem != index)
	{
		menuOptionViewData->nCurItem = index;
		XM_InvalidateWindow ();
		XM_UpdateWindow ();
		XM_SetTimer (XMTIMER_OPTIONVIEW, OSD_AUTO_HIDE_TIMEOUT);
	}
}

VOID MenuOptionViewOnTouchUp (XMMSG *msg)
{
	MENUOPTIONVIEWDATA *menuOptionViewData;
	menuOptionViewData = (MENUOPTIONVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MenuOptionView));
	if(menuOptionViewData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&menuOptionViewData->btnControl, msg))
		return;
	if(menuOptionViewData->nTouchItem == -1)
		return;
	menuOptionViewData->nTouchItem = -1;

	XM_PostMessage (XM_COMMAND, menuOptionViewData->btnControl.pBtnInfo[0].wCommand, menuOptionViewData->nCurItem);
}

VOID MenuOptionViewOnTouchMove (XMMSG *msg)
{
	MENUOPTIONVIEWDATA *menuOptionViewData;
	menuOptionViewData = (MENUOPTIONVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(MenuOptionView));
	if(menuOptionViewData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&menuOptionViewData->btnControl, msg))
		return;

}

// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (MenuOptionView)
	XM_ON_MESSAGE (XM_PAINT, MenuOptionViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, MenuOptionViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, MenuOptionViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, MenuOptionViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, MenuOptionViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, MenuOptionViewOnCommand)
	XM_ON_MESSAGE (XM_TIMER, MenuOptionViewOnTimer)
	XM_ON_MESSAGE (XM_TOUCHDOWN, MenuOptionViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, MenuOptionViewOnTouchUp)
	XM_ON_MESSAGE (XM_TOUCHMOVE, MenuOptionViewOnTouchMove)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, MenuOptionView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
