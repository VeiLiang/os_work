//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_FormatSetting.c
//	  ͨ��Menuѡ���б�ѡ�񴰿�(ֻ��ѡ��һ��ѡ��)
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuid.h"
#include "app_menuoptionview.h"

// ˽�������
#define	GUIDE_COMMAND_MODIFY		0
#define	GUIDE_COMMAND_RETURN		1

// ���˵�ѡ����ڰ�ť�ؼ�����
#define	GUIDEVIEWBTNCOUNT	2
static const XMBUTTONINFO menuOptionBtn[GUIDEVIEWBTNCOUNT] = {
	{
		VK_AP_SWITCH,		GUIDE_COMMAND_MODIFY,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_MODIFY
	},
	{
		VK_AP_MODE,		GUIDE_COMMAND_RETURN,	AP_ID_COMMON_OK,	AP_ID_BUTTON_RETURN
	},
};

typedef struct _tagGUIDEMENUDATA {
    u8_t            nGuide_Camer1;
	u8_t            nGuide_Camer2;
	u8_t            nGuide_Camer3;
    u8_t            nGuide_Camer4;
}GUIDEMENUDATA;

typedef struct tagGUIDEMENUDAT {
	int				nTopItem;					// ��һ�����ӵĲ˵���
	int				nCurItem;					// ��ǰѡ��Ĳ˵���
	int				nItemCount;					// �˵������
	APPMENUOPTION	*lpguideView;
    GUIDEMENUDATA *guideData;
	XMBUTTONCONTROL	btnControl;				// ��ť�ؼ�
	XMTITLEBARCONTROL	titleControl;			// ����ؼ�

} GUIDEMENUDAT;

VOID GuideViewOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
	    GUIDEMENUDAT *GuideViewData = XM_calloc (sizeof(GUIDEMENUDAT));
            APPMENUOPTION *lpFunctionView = (APPMENUOPTION *)msg->lp;
            GUIDEMENUDATA *GuideData = GuideViewData->guideData;
		if(GuideViewData == NULL)
		{
			XM_printf ("menuOptionViewData XM_calloc failed\n");
			// ʧ�ܷ��ص�����
			XM_PullWindow (0);
			return;
		}
        GuideData->nGuide_Camer1 = AppMenuData.guide_camer1;
        GuideData->nGuide_Camer2 = AppMenuData.guide_camer2;
        GuideData->nGuide_Camer3 = AppMenuData.guide_camer3;
        GuideData->nGuide_Camer4 = AppMenuData.guide_camer4;

		//�����ʼ��
		AP_TitleBarControlInit (&GuideViewData->titleControl, XMHWND_HANDLE(GuideView),
														lpFunctionView->dwWindowIconId, lpFunctionView->dwWindowTextId);
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(GuideView), GuideViewData);

	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("FormatSettingView Pull\n");
	}
    XM_SetTimer (XMTIMER_SETTINGVIEW, 200); //������ʱ��
}

VOID GuideViewOnLeave (XMMSG *msg)
{
    XM_KillTimer (XMTIMER_SETTINGVIEW);
	if (msg->wp == 0)
	{
//		int i;
		GUIDEMENUDAT *GuideViewData = (GUIDEMENUDAT *)XM_GetWindowPrivateData (XMHWND_HANDLE(GuideView));
		if(GuideViewData)
		{
			// ����ؼ��˳�����
			AP_TitleBarControlExit (&GuideViewData->titleControl);
			// �ͷ�˽�����ݾ��
			XM_free (GuideViewData);
		}
		XM_printf ("FormatSettingView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("FormatSettingView Push\n");
	}
}


VOID GuideViewOnPaint (XMMSG *msg)
{
	XMRECT rc,rect,rcArrow;
	unsigned int old_alpha;
    char String[32];
    XMSIZE size;
    XMCOORD menu_name_x;
    int nVisualCount;
    int i;
    int nItem, nLastItem;
    DWORD menuID;
    int Middle_Five = 0;
    int Middle_Data = 0;
    int Middle_String = 0;
	HANDLE hwnd = XMHWND_HANDLE(GuideView);
	GUIDEMENUDAT *GuideViewData = (GUIDEMENUDAT *)XM_GetWindowPrivateData (hwnd);
	if(GuideViewData == NULL)
		return;

	// ��ʾ������
	XM_GetDesktopRect (&rc);
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);
    GUIDEMENUDATA *GuideData =  GuideViewData->guideData;
    menu_name_x = (XMCOORD)(APP_POS_ITEM5_MENUNAME_X * 1);

	// ********* 1 ��ʾ����������Ϣ *********
	AP_TitleBarControlMessageHandler (&GuideViewData->titleControl, msg);
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    rect = rc;
	rect.left = 0;
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (GuideViewData->nCurItem - GuideViewData->nTopItem)  * APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    //ѡ��
    rect = rc;
	rect.top = APP_POS_ITEM5_MENUNAME_Y;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_CAMERA1, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
     //��ʾ����
    rect.left = APP_POS_ITEM5_MENUFLAG_X*2;
    sprintf(String,"%d",GuideData->nGuide_Camer1);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString (XMHWND_HANDLE(GuideView), rect.left, rect.top, String, strlen(String));
    // ��ʾ "s"
    rect.left = APP_POS_ITEM5_MENUFLAG_X*2+ 50;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_SECOND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 *APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    //contrast
    rect = rc;
    rect.top = APP_POS_ITEM5_MENUNAME_Y *2;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_CAMERA2, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    rect.left = APP_POS_ITEM5_MENUFLAG_X*2;
    sprintf(String,"%d",GuideData->nGuide_Camer2);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString (XMHWND_HANDLE(GuideView), rect.left, rect.top, String, strlen(String));
    // ��ʾ "s"
    rect.left = APP_POS_ITEM5_MENUFLAG_X*2+ 50;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_SECOND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 2 *APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
#if 0
    //Saturation
    rect = rc;
    rect.top = APP_POS_ITEM5_MENUNAME_Y *3;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_CAMERA3, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    rect.left = APP_POS_ITEM5_MENUFLAG_X*2;
    sprintf(String,"%d",GuideData->nGuide_Camer3);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString (XMHWND_HANDLE(GuideView), rect.left, rect.top, String, strlen(String));
    // ��ʾ "s"
    rect.left = APP_POS_ITEM5_MENUFLAG_X*2+ 50;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_SECOND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 3 *APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    //Saturation
    rect = rc;
    rect.top = APP_POS_ITEM5_MENUNAME_Y *4;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_CAMERA4, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    rect.left = APP_POS_ITEM5_MENUFLAG_X*2;
    sprintf(String,"%d",GuideData->nGuide_Camer4);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString (XMHWND_HANDLE(GuideView), rect.left, rect.top, String, strlen(String));
    // ��ʾ "s"
    rect.left = APP_POS_ITEM5_MENUFLAG_X*2+ 50;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_SECOND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 4 *APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
#endif

    AP_ButtonControlMessageHandler (&GuideViewData->btnControl, msg);
	XM_SetWindowAlpha (hwnd, (unsigned char)old_alpha);
}

VOID GuideViewOnKeyDown (XMMSG *msg)
{
    int nVisualCount;
	XMRECT rc;
	GUIDEMENUDAT *GuideViewData = (GUIDEMENUDAT *)XM_GetWindowPrivateData (XMHWND_HANDLE(GuideView));
    GUIDEMENUDATA *GuideData =  GuideViewData->guideData;
	if(GuideViewData == NULL)
		return;
    nVisualCount = GuideViewData->nItemCount;
    switch(msg->wp)
    {
        case VK_AP_MENU:
        case REMOTE_KEY_MENU:
        case REMOTE_KEY_POWER:
            AppMenuData.guide_camer1 = GuideData->nGuide_Camer1;
            AppMenuData.guide_camer2 = GuideData->nGuide_Camer2;
            //AppMenuData.guide_camer3 = GuideData->nGuide_Camer3;
            //AppMenuData.guide_camer4 = GuideData->nGuide_Camer4;
            AP_SaveMenuData (&AppMenuData);
            XM_PullWindow (0);
            break;
        case VK_AP_SWITCH:
        case REMOTE_KEY_RIGHT:
			// ˢ��
            AP_ButtonControlMessageHandler (&GuideViewData->btnControl, msg);
            if(GuideViewData->nCurItem == 0) {
                if(GuideData->nGuide_Camer1 == 15)
                    GuideData->nGuide_Camer1 = 0;
                else
                    GuideData->nGuide_Camer1 ++;
            }else if(GuideViewData->nCurItem == 1) {
                if(GuideData->nGuide_Camer2 == 15)
                    GuideData->nGuide_Camer2 = 0;
                else
                    GuideData->nGuide_Camer2 ++;
            }else if(GuideViewData->nCurItem == 2) {
                if(GuideData->nGuide_Camer3 == 15)
                    GuideData->nGuide_Camer3 = 0;
                else
                    GuideData->nGuide_Camer3 ++;
            }else if(GuideViewData->nCurItem == 3) {
                if(GuideData->nGuide_Camer4 == 15)
                    GuideData->nGuide_Camer4 = 0;
                else
                    GuideData->nGuide_Camer4 ++;
            }
            XM_InvalidateWindow ();
            XM_UpdateWindow ();
            break;
        case VK_AP_FONT_BACK_SWITCH:
        case REMOTE_KEY_LEFT:
            AP_ButtonControlMessageHandler (&GuideViewData->btnControl, msg);
			// ˢ��
			if(GuideViewData->nCurItem == 0) {
                if(GuideData->nGuide_Camer1 == 0)
                    GuideData->nGuide_Camer1 = 15;
                else
                    GuideData->nGuide_Camer1 --;
            }else if(GuideViewData->nCurItem == 1) {
                if(GuideData->nGuide_Camer2 == 0)
                    GuideData->nGuide_Camer2 = 15;
                else
                    GuideData->nGuide_Camer2 --;
            }else if(GuideViewData->nCurItem == 2) {
                if(GuideData->nGuide_Camer3 == 0)
                    GuideData->nGuide_Camer3 = 15;
                else
                    GuideData->nGuide_Camer3 --;
            }else if(GuideViewData->nCurItem == 3) {
                if(GuideData->nGuide_Camer4 == 0)
                    GuideData->nGuide_Camer4 = 15;
                else
                    GuideData->nGuide_Camer4 --;
            }
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
            break;
        //case VK_AP_AV:
        case VK_AP_DOWN:
        case REMOTE_KEY_DOWN:
            GuideViewData->nCurItem ++;
            if(GuideViewData->nCurItem > 1) {
               GuideViewData->nCurItem  = 0;
            }
    		XM_InvalidateWindow ();
    		XM_UpdateWindow ();
            break;
	case VK_AP_UP:
        case REMOTE_KEY_UP:
            GuideViewData->nCurItem --;
            if(GuideViewData->nCurItem < 0) {
               GuideViewData->nCurItem  = 1;
            }
    		XM_InvalidateWindow ();
    		XM_UpdateWindow ();
            break;
    }

}

VOID GuideViewOnKeyUp (XMMSG *msg)
{
	printf("ColorViewOnKeyUp \n");
}

VOID GuideViewOnCommand (XMMSG *msg)
{
	GUIDEMENUDAT *GuideViewData;
	GuideViewData = (GUIDEMENUDAT *)XM_GetWindowPrivateData (XMHWND_HANDLE(GuideView));
	if(GuideViewData == NULL)
		return;
	switch(msg->wp)
	{
		case GUIDE_COMMAND_MODIFY:

            #if 0
			XM_printf ("MENUOPTIONVIEW_COMMAND_MODIFY\n");
			// ȷ�ϲ�����
			if(FormatViewData->lpFormatView && FormatViewData->lpFormatView->fpMenuOptionCB)
			{
				APPMENUOPTION	*lpMenuOptionView = FormatViewData->lpFormatView;
				(*lpMenuOptionView->fpMenuOptionCB) (lpMenuOptionView->lpUserData, FormatViewData->nCurItem);
			}
            #endif
			XM_PullWindow (NULL);
			break;

		case GUIDE_COMMAND_RETURN:
			// ���ص���һ��
			XM_PullWindow (0);
			break;
	}
}

VOID GuideViewOnTouchDown (XMMSG *msg)
{
	printf("ColorViewOnTouchDown \n");
}

VOID GuideViewOnTouchUp (XMMSG *msg)
{
    printf("ColorViewOnTouchUp \n");
}

VOID GuideViewOnTouchMove (XMMSG *msg)
{
    printf("ColorViewOnTouchMove \n");
}
#include "types.h"
extern BOOL  Auto_Menu;
VOID GuideViewOnTimer (XMMSG *msg)
{
	if(!Auto_Menu) {
	    if(AHD_Guide_Start()) {
	        //XM_JumpWindow (XMHWND_HANDLE(Desktop));
	        XM_PullWindow(XMHWND_HANDLE(Desktop));
	    }
	}
}

// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (GuideView)
	XM_ON_MESSAGE (XM_PAINT, GuideViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, GuideViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, GuideViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, GuideViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, GuideViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, GuideViewOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, GuideViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, GuideViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, GuideViewOnTouchUp)
	XM_ON_MESSAGE (XM_TOUCHMOVE, GuideViewOnTouchMove)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, GuideView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
