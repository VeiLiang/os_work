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
#define	CAMERA_COMMAND_MODIFY		0
#define	CAMERA_COMMAND_RETURN		1

// ���˵�ѡ����ڰ�ť�ؼ�����
#define	CAMERAVIEWBTNCOUNT	2
static const XMBUTTONINFO menuOptionBtn[CAMERAVIEWBTNCOUNT] = {
	{
		VK_AP_SWITCH,		CAMERA_COMMAND_MODIFY,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_MODIFY
	},
	{
		VK_AP_MODE,		CAMERA_COMMAND_RETURN,	AP_ID_COMMON_OK,	AP_ID_BUTTON_RETURN
	},
};

typedef struct _tagCAMERAMENUDATA {
    u8_t            nGuideData1;
	u8_t            nGuideData2;
	u8_t            nGuideData3;
    u8_t            nGuideData4;
}CAMERAMENUDATA;

typedef struct tagCAMERAMENUDAT {
	int				nTopItem;					// ��һ�����ӵĲ˵���
	int				nCurItem;					// ��ǰѡ��Ĳ˵���
	int				nItemCount;					// �˵������
	APPMENUOPTION	*lpFunctionView;
    CAMERAMENUDATA *CameraData;
	XMBUTTONCONTROL	btnControl;				// ��ť�ؼ�
	XMTITLEBARCONTROL	titleControl;			// ����ؼ�
} CAMERAMENUDAT;

VOID CameraViewOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
	    u32_t VP_CONTRAST = 0;
	    CAMERAMENUDAT *CameraViewData = XM_calloc (sizeof(CAMERAMENUDAT));
            APPMENUOPTION *lpFunctionView = (APPMENUOPTION *)msg->lp;
            CAMERAMENUDATA *CameraData = CameraViewData->CameraData;
		if(CameraViewData == NULL)
		{
			XM_printf ("menuOptionViewData XM_calloc failed\n");
			// ʧ�ܷ��ص�����
			XM_PullWindow (0);
			return;
		}
        CameraData->nGuideData1 = AppMenuData.camera1;
        CameraData->nGuideData2 = AppMenuData.camera2;
        //CameraData->nGuideData3 = AppMenuData.camera3;
        //CameraData->nGuideData4 = AppMenuData.camera4;

		//�����ʼ��
		AP_TitleBarControlInit (&CameraViewData->titleControl, XMHWND_HANDLE(CameraView),
														lpFunctionView->dwWindowIconId, lpFunctionView->dwWindowTextId);
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(CameraView), CameraViewData);

	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("FormatSettingView Pull\n");
	}

    XM_SetTimer (XMTIMER_SETTINGVIEW, 200); //������ʱ��
}

VOID CameraViewOnLeave (XMMSG *msg)
{
    XM_KillTimer (XMTIMER_SETTINGVIEW);
	if (msg->wp == 0)
	{
		CAMERAMENUDAT *CameraViewData = (CAMERAMENUDAT *)XM_GetWindowPrivateData (XMHWND_HANDLE(CameraView));
		if(CameraViewData)
		{
			// ����ؼ��˳�����
			AP_TitleBarControlExit (&CameraViewData->titleControl);
			// �ͷ�˽�����ݾ��
			XM_free (CameraViewData);
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


VOID CameraViewOnPaint (XMMSG *msg)
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
	HANDLE hwnd = XMHWND_HANDLE(CameraView);
	CAMERAMENUDAT *CameraViewData = (CAMERAMENUDAT *)XM_GetWindowPrivateData (hwnd);
	if(CameraViewData == NULL)
		return;

	// ��ʾ������
	XM_GetDesktopRect (&rc);
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);
    CAMERAMENUDATA *CameraData =  CameraViewData->CameraData;
    menu_name_x = (XMCOORD)(APP_POS_ITEM5_MENUNAME_X * 1);

	// ********* 1 ��ʾ����������Ϣ *********
	AP_TitleBarControlMessageHandler (&CameraViewData->titleControl, msg);
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    rect = rc;
	rect.left = 0;
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (CameraViewData->nCurItem - CameraViewData->nTopItem)  * APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    //ѡ��
    rect = rc;
	rect.top = APP_POS_ITEM5_MENUNAME_Y;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_GUIDE_1, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
     //��ʾ����
    if(CameraData->nGuideData1 == 0) {
        menuID = AP_ID_SETTING_CLOSE;
    }else {
        menuID = AP_ID_SETTING_OPEN;
    }
    rect.left = APP_POS_ITEM5_MENUFLAG_X*2;
    AP_RomImageDrawByMenuID (menuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 *APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    //contrast
    rect = rc;
    rect.top = APP_POS_ITEM5_MENUNAME_Y *2;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_GUIDE_2, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
   if(CameraData->nGuideData2 == 0) {
        menuID = AP_ID_SETTING_CLOSE;
    }else {
        menuID = AP_ID_SETTING_OPEN;
    }
    rect.left = APP_POS_ITEM5_MENUFLAG_X*2;
    AP_RomImageDrawByMenuID (menuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 2 *APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
#if 0
    //Saturation
    rect = rc;
    rect.top = APP_POS_ITEM5_MENUNAME_Y *3;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_GUIDE_3, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    if(CameraData->nGuideData3 == 0) {
        menuID = AP_ID_SETTING_CLOSE;
    }else {
        menuID = AP_ID_SETTING_OPEN;
    }
    rect.left = APP_POS_ITEM5_MENUFLAG_X*2;
    AP_RomImageDrawByMenuID (menuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 3 *APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    rect = rc;
    rect.top = APP_POS_ITEM5_MENUNAME_Y *4;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_GUIDE_4, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    if(CameraData->nGuideData4 == 0) {
        menuID = AP_ID_SETTING_CLOSE;
    }else {
        menuID = AP_ID_SETTING_OPEN;
    }
    rect.left = APP_POS_ITEM5_MENUFLAG_X*2;
    AP_RomImageDrawByMenuID (menuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 4 *APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
#endif

    AP_ButtonControlMessageHandler (&CameraViewData->btnControl, msg);
	XM_SetWindowAlpha (hwnd, (unsigned char)old_alpha);
}

VOID CameraViewOnKeyDown (XMMSG *msg)
{
    int nVisualCount;
	XMRECT rc;
	CAMERAMENUDAT *CameraViewData = (CAMERAMENUDAT *)XM_GetWindowPrivateData (XMHWND_HANDLE(CameraView));
    CAMERAMENUDATA *CameraData =  CameraViewData->CameraData;
	if(CameraViewData == NULL)
		return;

	#if 0
    nVisualCount = CameraViewData->nItemCount;
    switch(msg->wp)
    {
        case VK_AP_MENU:
        case REMOTE_KEY_MENU:
        case REMOTE_KEY_POWER:
            AppMenuData.camera1 = CameraData->nGuideData1;
            AppMenuData.camera2 = CameraData->nGuideData2;
            //AppMenuData.camera3 = CameraData->nGuideData3;
            //AppMenuData.camera4 = CameraData->nGuideData4;
            AP_SaveMenuData (&AppMenuData);
            XM_PullWindow (0);
            break;
        case VK_AP_SWITCH:
        case REMOTE_KEY_LEFT:
			// ˢ��
            AP_ButtonControlMessageHandler (&CameraViewData->btnControl, msg);
            if(CameraViewData->nCurItem == 0) {
                if(CameraData->nGuideData1 == 0) {
                    CameraData->nGuideData1 = 1;
                }else {
                    CameraData->nGuideData1 = 0;
                }
            }else if(CameraViewData->nCurItem == 1) {
                if(CameraData->nGuideData2 == 0) {
                    CameraData->nGuideData2 = 1;
                }else {
                    CameraData->nGuideData2 = 0;
                }
            }else if(CameraViewData->nCurItem == 2) {
                if(CameraData->nGuideData3 == 0) {
                    CameraData->nGuideData3 = 1;
                }else {
                    CameraData->nGuideData3 = 0;
                }
            }else {
                if(CameraData->nGuideData4 == 0) {
                    CameraData->nGuideData4 = 1;
                }else {
                    CameraData->nGuideData4 = 0;
                }
            }
            XM_InvalidateWindow ();
            XM_UpdateWindow ();
            break;
        //case VK_AP_DOWN:
        case REMOTE_KEY_RIGHT:
            AP_ButtonControlMessageHandler (&CameraViewData->btnControl, msg);
			// ˢ��
			if(CameraViewData->nCurItem == 0) {
                if(CameraData->nGuideData1 == 0) {
                    CameraData->nGuideData1 = 1;
                }else {
                    CameraData->nGuideData1 = 0;
                }
            }else if(CameraViewData->nCurItem == 1) {
                if(CameraData->nGuideData2 == 0) {
                    CameraData->nGuideData2 = 1;
                }else {
                    CameraData->nGuideData2 = 0;
                }
            }else if(CameraViewData->nCurItem == 2) {
                if(CameraData->nGuideData3 == 0) {
                    CameraData->nGuideData3 = 1;
                }else {
                    CameraData->nGuideData3 = 0;
                }
            }else {
                if(CameraData->nGuideData4 == 0) {
                    CameraData->nGuideData4 = 1;
                }else {
                    CameraData->nGuideData4 = 0;
                }
            }
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
            break;
        //case VK_AP_AV:
        case VK_AP_DOWN:
        case REMOTE_KEY_DOWN:
            CameraViewData->nCurItem ++;
            if(CameraViewData->nCurItem > 1) {
               CameraViewData->nCurItem  = 0;
            }
    		XM_InvalidateWindow ();
    		XM_UpdateWindow ();
            break;
	 case VK_AP_UP:
        case REMOTE_KEY_UP:
            CameraViewData->nCurItem --;
            if(CameraViewData->nCurItem < 0) {
               CameraViewData->nCurItem  = 1;
            }
    		XM_InvalidateWindow ();
    		XM_UpdateWindow ();
            break;
    }
	#endif
}

VOID CameraViewOnKeyUp (XMMSG *msg)
{
	printf("ColorViewOnKeyUp \n");
}

VOID CameraViewOnCommand (XMMSG *msg)
{
#if 0
	CAMERAMENUDAT *CameraViewData;
	CameraViewData = (CAMERAMENUDAT *)XM_GetWindowPrivateData (XMHWND_HANDLE(CameraView));
	if(CameraViewData == NULL)
		return;
	switch(msg->wp)
	{
		case CAMERA_COMMAND_MODIFY:
			XM_PullWindow (NULL);
			break;

		case CAMERA_COMMAND_RETURN:
			// ���ص���һ��
			XM_PullWindow (0);
			break;
	}
#endif
}

VOID CameraViewOnTouchDown (XMMSG *msg)
{
	printf("ColorViewOnTouchDown \n");
}

VOID CameraViewOnTouchUp (XMMSG *msg)
{
    printf("ColorViewOnTouchUp \n");
}

VOID CameraViewOnTouchMove (XMMSG *msg)
{
    printf("ColorViewOnTouchMove \n");
}
#include "types.h"
extern BOOL  Auto_Menu;

static VOID CameraViewOnTimer (XMMSG *msg)
{
#if 0
	if(!Auto_Menu) {
		if(AHD_Guide_Start())
		    XM_PullWindow(XMHWND_HANDLE(Desktop));
	}
#endif
}
// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (CameraView)
	XM_ON_MESSAGE (XM_PAINT, CameraViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, CameraViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, CameraViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, CameraViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, CameraViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, CameraViewOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, CameraViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, CameraViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, CameraViewOnTouchUp)
	XM_ON_MESSAGE (XM_TOUCHMOVE, CameraViewOnTouchMove)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, CameraView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
