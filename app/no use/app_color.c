//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_FormatSetting.c
//	  通用Menu选项列表选择窗口(只能选择一个选项)
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuid.h"
#include "app_menuoptionview.h"
#include "types.h"

// 私有命令定义
#define	COLOR_COMMAND_MODIFY		0
#define	COLOR_COMMAND_RETURN		1

// “菜单选项”窗口按钮控件定义
#define	COLORVIEWBTNCOUNT	2
static const XMBUTTONINFO menuOptionBtn[COLORVIEWBTNCOUNT] = {
	{
		VK_AP_SWITCH,		COLOR_COMMAND_MODIFY,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_MODIFY
	},
	{
		VK_AP_MODE,		COLOR_COMMAND_RETURN,	AP_ID_COMMON_OK,	AP_ID_BUTTON_RETURN
	},
};
typedef struct _tagCOLORMENUDATA {
    u8_t            nBrightData;                //亮度
	u8_t            nSaturationData;                //色度
	u8_t            nContrastData;                //对比度
}COLORMENUDATA;

typedef struct tagCOLORVIEWDATA {
	int				nTopItem;					// 第一个可视的菜单项
	int				nCurItem;					// 当前选择的菜单项
	int				nItemCount;					// 菜单项个数
    COLORMENUDATA  *ColorData;
	APPMENUOPTION	*lpColorView;
	XMBUTTONCONTROL	btnControl;				// 按钮控件
	XMTITLEBARCONTROL	titleControl;			// 标题控件

} COLORVIEWDATA;

BOOL NO_Signed_Fail = TRUE;
VOID ColorViewOnEnter (XMMSG *msg)
{
    NO_Signed_Fail = FALSE;
	if(msg->wp == 0)
	{
	    u32_t VP_CONTRAST = 0;
	    COLORVIEWDATA *ColorViewData = XM_calloc (sizeof(COLORVIEWDATA));
            APPMENUOPTION *lpColorView = (APPMENUOPTION *)msg->lp;
            COLORMENUDATA *ColorData = ColorViewData->ColorData;
		if(ColorViewData == NULL)
		{
			XM_printf ("menuOptionViewData XM_calloc failed\n");
			// 失败返回到桌面
			XM_PullWindow (0);
			return;
		}
        ColorData->nContrastData = AP_GetColor_Contrast();
        ColorData->nSaturationData = AP_GetColor_Saturation();
        ColorData->nBrightData = AP_GetColor_Brightness();
        ColorViewData->lpColorView = lpColorView;
		ColorViewData->nItemCount = lpColorView->wMenuOptionCount;
		ColorViewData->nCurItem = lpColorView->wMenuOptionSelect;
		//标题初始化
		AP_TitleBarControlInit (&ColorViewData->titleControl, XMHWND_HANDLE(ColorView),
														lpColorView->dwWindowIconId, lpColorView->dwWindowTextId);
		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(ColorView), ColorViewData);

	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("FormatSettingView Pull\n");
	}
    XM_SetTimer (XMTIMER_SETTINGVIEW, 200); //开启定时器
}

VOID ColorViewOnLeave (XMMSG *msg)
{
    XM_KillTimer (XMTIMER_SETTINGVIEW);
	if (msg->wp == 0)
	{
//		int i;
		COLORVIEWDATA *ColorViewData = (COLORVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ColorView));
		if(ColorViewData)
		{
			// 标题控件退出过程
			AP_TitleBarControlExit (&ColorViewData->titleControl);

			// 释放私有数据句柄
			XM_free (ColorViewData);

		}
		XM_printf ("FormatSettingView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("FormatSettingView Push\n");
	}
}


#define Start_SHOW_MENU 144
VOID ColorViewOnPaint (XMMSG *msg)
{
	XMRECT rc,rect,rcArrow;
	unsigned int old_alpha;
    char String[32];
    XMSIZE size;
    XMCOORD menu_name_x;
    int nVisualCount;
    int i;
    APPMENUOPTION	*lpFormatView;
    int nItem, nLastItem;
    int Middle_Five = 0;
    int Middle_Data = 0;
    int Middle_String = 0;
	HANDLE hwnd = XMHWND_HANDLE(ColorView);
	COLORVIEWDATA *ColorViewData = (COLORVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(ColorViewData == NULL)
		return;

    COLORMENUDATA *ColorData =  ColorViewData->ColorData;
	// 显示标题栏
	XM_GetDesktopRect (&rc);
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);
    lpFormatView = ColorViewData->lpColorView;

    menu_name_x = (XMCOORD)(APP_POS_ITEM5_MENUNAME_X * 1);

	// ********* 1 显示标题栏区信息 *********
	AP_TitleBarControlMessageHandler (&ColorViewData->titleControl, msg);

    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    rect = rc;
	rect.left = 0;
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (ColorViewData->nCurItem - ColorViewData->nTopItem)  * APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);


    //选项
    rect = rc;
	rect.top = APP_POS_ITEM5_MENUNAME_Y;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_BRIGHT, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    //显示数字
    rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
    sprintf(String,"%d",ColorData->nBrightData);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString (XMHWND_HANDLE(ColorView), rect.left, rect.top, String, strlen(String));

    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 *APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    //contrast
    rect = rc;
    rect.top = APP_POS_ITEM5_MENUNAME_Y *2;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_CONTRAST, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    //显示数字
    rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
    sprintf(String,"%d",ColorData->nContrastData);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString (XMHWND_HANDLE(ColorView), rect.left, rect.top, String, strlen(String));

    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 2 *APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    //Saturation
    rect = rc;
    rect.top = APP_POS_ITEM5_MENUNAME_Y *3;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
   AP_RomImageDrawByMenuID (AP_ID_SETTING_COLOR, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    //显示数字
    rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
    sprintf(String,"%d",ColorData->nSaturationData);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString (XMHWND_HANDLE(ColorView), rect.left, rect.top, String, strlen(String));
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 3 *APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    rect = rc;
    rect.top = APP_POS_ITEM5_MENUNAME_Y *4;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_RESET, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 4 *APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    #if 0
    #if 0
    // ********* 2 警告信息处理 *************
    rect = rc;
    rect.left = 0;//(XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * scale_factor);
	rect.top = -70;
    AP_RomImageDrawByMenuID (APP_ID_SETTING_FORMAT_ALL, hwnd, &rect, XMGIF_DRAW_POS_CENTER);
    //********* 3. 菜单选项 ******************
    nVisualCount = FormatViewData->nItemCount;

    // 填充当前选择项背景
    for(i = 0; i < nVisualCount;i++) {
        rect.left = menu_name_x;
		rect.top = (XMCOORD)(Start_SHOW_MENU + i * APP_POS_ITEM5_LINEHEIGHT);
        AP_RomImageDrawByMenuID (AP_ID_COMMON_BUTTONBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    }

	rect = rc;
	rect.left = menu_name_x;
	rect.top = (XMCOORD)(Start_SHOW_MENU + 1 + (FormatViewData->nCurItem - FormatViewData->nTopItem) * APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    rect.top = Start_SHOW_MENU + 1;
	rect.bottom = Start_SHOW_MENU + APP_POS_ITEM5_LINEHEIGHT;
	nLastItem = FormatViewData->nItemCount;
	if(nLastItem > (FormatViewData->nTopItem + nVisualCount))
		nLastItem = (FormatViewData->nTopItem + nVisualCount);
	for (nItem = FormatViewData->nTopItem; nItem < nLastItem; nItem ++)
    {
        AP_RomImageDrawByMenuID (lpFormatView->lpdwMenuOptionId[nItem], hwnd, &rect, XMGIF_DRAW_POS_CENTER);
		XM_OffsetRect (&rect, 0, APP_POS_ITEM5_LINEHEIGHT);
    }
    #endif
    Middle_Data = (rc.bottom - rc.top +1)*3/6;
    Middle_String = (rc.bottom - rc.top +1)*2/6;
    Middle_Five = (rc.right - rc.left +1) /8;
    //select backgroud
#if 0
    rect.left = Middle_Five * (1 + ColorViewData->nCurItem *2) -15;
    rect.top = Middle_Data - 10;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_DEL_BACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
#endif
    //bright
    rect = rc;
    rect.left = Middle_Five *1;
    rect.top = Middle_String;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_BRIGHT, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    rect.left += 5;
    rect.top = Middle_Data;
    sprintf(String,"%d",ColorData->nBrightData);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString (XMHWND_HANDLE(ColorView), rect.left, rect.top, String, strlen(String));


    //COLOR
    rect = rc;
    rect.left = Middle_Five *3;
    rect.top = Middle_String;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_CHROMA, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    memset (String, 0, sizeof(String));
    rect.left += 5;
    rect.top = Middle_Data;
    sprintf(String,"%d",ColorData->nSaturationData);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString (XMHWND_HANDLE(ColorView), rect.left, rect.top, String, strlen(String));

     //CONSTRAST
    rect = rc;
    rect.left = Middle_Five *5;
    rect.top = Middle_String;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_CONTRAST, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    memset (String, 0, sizeof(String));
    rect.left += 20;
    rect.top = Middle_Data;
    sprintf(String,"%d",ColorData->nContrastData);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString (XMHWND_HANDLE(ColorView), rect.left, rect.top, String, strlen(String));
    #endif
    AP_ButtonControlMessageHandler (&ColorViewData->btnControl, msg);
	XM_SetWindowAlpha (hwnd, (unsigned char)old_alpha);
}

void AdjustColorData(COLORMENUDATA *ColorData, int nItem, int diff)
{
    switch(nItem) {
        case 0:
            if(diff == 1) {
                if(ColorData->nBrightData < 100) {
                   ColorData->nBrightData = ColorData->nBrightData + diff;
                   AP_SetColor_Brightness(ColorData->nBrightData);
                   //Set_Color_Brightness(ColorData->nBrightData);
                }
            }else {
                //if(ColorData->nBrightData > 0) {
                    ColorData->nBrightData = ColorData->nBrightData + diff;
                    AP_SetColor_Brightness(ColorData->nBrightData);
			if(ColorData->nBrightData > 100)
				ColorData->nBrightData = 1;
			if(ColorData->nBrightData <=0)
				ColorData->nBrightData = 100;
                   // Set_Color_Brightness(ColorData->nBrightData);
                //}
            }
            break;
        case 2:
            if(diff == 1) {
                if(ColorData->nSaturationData < 100) {
                   ColorData->nSaturationData = ColorData->nSaturationData + diff;
                   AP_SetColor_Saturation(ColorData->nSaturationData);
                  // Set_Color_Struation(ColorData->nSaturationData);
                }
            }else {
                //if(ColorData->nSaturationData > 0) {
                    ColorData->nSaturationData = ColorData->nSaturationData + diff;
                    AP_SetColor_Saturation(ColorData->nSaturationData);
			if(ColorData->nSaturationData > 100)
				ColorData->nSaturationData = 1;
			if(ColorData->nSaturationData <=0)
				ColorData->nSaturationData = 100;
                   //Set_Color_Struation(ColorData->nSaturationData);
                //}
            }
            break;
        case 1:
            if(diff == 1) {
                if(ColorData->nContrastData < 100) {
                   ColorData->nContrastData = ColorData->nContrastData + diff;
                   AP_SetColor_Contrast(ColorData->nContrastData);
                   //Set_Color_Contrast(ColorData->nContrastData);
                }
            }else {
               // if(ColorData->nContrastData > 0) {
                    ColorData->nContrastData = ColorData->nContrastData + diff;
                    AP_SetColor_Contrast(ColorData->nContrastData);
			if(ColorData->nContrastData > 100)
				ColorData->nContrastData = 1;
			if(ColorData->nContrastData <=0)
				ColorData->nContrastData = 100;
			// Set_Color_Contrast(ColorData->nContrastData);
               // }
            }

            break;
    }
}

VOID Reset_Color_Data(COLORMENUDATA *ColorData)
{
    ColorData->nBrightData = 50;
    ColorData->nSaturationData = 50;
    ColorData->nContrastData = 50;
    //PR2000_SetContract_Data(ColorData->nContrastData);
    //PR2000_2_SetContract_Data(ColorData->nContrastData);
	
    //PR2000_SetStaturation_Data(ColorData->nSaturationData);
    //PR2000_SetStaturation_Data(ColorData->nSaturationData);

    //PR2000_SetBright_Data(ColorData->nBrightData);
    //PR2000_SetBright_Data(ColorData->nBrightData);

}


VOID ColorViewOnKeyRepeatDown (XMMSG *msg)
{
    int nVisualCount;
	XMRECT rc;
	COLORVIEWDATA *ColorViewData = (COLORVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ColorView));
	if(ColorViewData == NULL)
		return;
    COLORMENUDATA *ColorData = ColorViewData->ColorData;
    nVisualCount = ColorViewData->nItemCount;
    switch(msg->wp)
    {
        case VK_AP_SWITCH:
       // case REMOTE_KEY_RIGHT:
			// 刷新
			AP_ButtonControlMessageHandler (&ColorViewData->btnControl, msg);
            if(ColorViewData->nCurItem < 3) {
                AdjustColorData(ColorData,ColorViewData->nCurItem,1);
             }else {
                Reset_Color_Data(ColorData);
             }

			XM_InvalidateWindow ();
			XM_UpdateWindow ();
            break;
        case VK_AP_FONT_BACK_SWITCH:
        //case REMOTE_KEY_LEFT:
            AP_ButtonControlMessageHandler (&ColorViewData->btnControl, msg);
            if(ColorViewData->nCurItem < 3) {
                AdjustColorData(ColorData,ColorViewData->nCurItem,-1);
            }else {
                Reset_Color_Data(ColorData);
            }
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
            break;
    }
}

VOID ColorViewOnKeyDown (XMMSG *msg)
{
    int nVisualCount;
	XMRECT rc;
	COLORVIEWDATA *ColorViewData = (COLORVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ColorView));
	if(ColorViewData == NULL)
		return;
    COLORMENUDATA *ColorData = ColorViewData->ColorData;
    nVisualCount = ColorViewData->nItemCount;
    switch(msg->wp)
    {
        case VK_AP_MENU:
        case REMOTE_KEY_MENU:
        case REMOTE_KEY_POWER:
            APP_SaveMenuData();
            XM_PullWindow (0);
            break;
        case VK_AP_SWITCH:
        case REMOTE_KEY_RIGHT:
			// 刷新
			AP_ButtonControlMessageHandler (&ColorViewData->btnControl, msg);
            if(ColorViewData->nCurItem < 3) {
                AdjustColorData(ColorData,ColorViewData->nCurItem,1);
             }else {
                Reset_Color_Data(ColorData);
             }

			XM_InvalidateWindow ();
			XM_UpdateWindow ();
            break;
        case VK_AP_FONT_BACK_SWITCH:
        case REMOTE_KEY_LEFT:
            AP_ButtonControlMessageHandler (&ColorViewData->btnControl, msg);
            if(ColorViewData->nCurItem < 3) {
                AdjustColorData(ColorData,ColorViewData->nCurItem,-1);
            }else {
                Reset_Color_Data(ColorData);
            }
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
            break;

       // case VK_AP_AV:
        case VK_AP_UP:
        case REMOTE_KEY_DOWN:
            #if 0
            printf("ColorViewData->nCurItem: %d \n",ColorViewData->nCurItem);
            if(ColorViewData->nCurItem == 0) {
                Set_BrightLight_Value(ColorData->nBrightData);
            }else if(ColorViewData->nCurItem == 1) {
                Set_Saturation_Value(ColorData->nSaturationData);
            }else if(ColorViewData->nCurItem == 2) {
                Set_Contrast_Value(ColorData->nContrastData);
                XM_PullWindow (0);
            }
            ColorViewData->nCurItem ++;
            XM_InvalidateWindow ();
    		XM_UpdateWindow ();
            #endif
            #if 1
            if(ColorViewData->nCurItem == 3) {
                    ColorViewData->nCurItem = 0;
            }else {
                 ColorViewData->nCurItem ++;
            }
    		XM_InvalidateWindow ();
    		XM_UpdateWindow ();

            #endif
            break;
	case VK_AP_DOWN:
        case REMOTE_KEY_UP:
            if(ColorViewData->nCurItem <= 0) {
                    ColorViewData->nCurItem = 3;
            }else {
                 ColorViewData->nCurItem --;

            }
    		XM_InvalidateWindow ();
    		XM_UpdateWindow ();
            break;

    }

}


VOID ColorViewOnKeyLongDown (XMMSG *msg)
{
    int nVisualCount;
	XMRECT rc;
	COLORVIEWDATA *ColorViewData = (COLORVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ColorView));
	if(ColorViewData == NULL)
		return;
    COLORMENUDATA *ColorData = ColorViewData->ColorData;
    nVisualCount = ColorViewData->nItemCount;
    switch(msg->wp)
    {
        case VK_AP_SWITCH:
       // case REMOTE_KEY_RIGHT:
			// 刷新
			AP_ButtonControlMessageHandler (&ColorViewData->btnControl, msg);
            if(ColorViewData->nCurItem < 3) {
                AdjustColorData(ColorData,ColorViewData->nCurItem,5);
             }else {
                Reset_Color_Data(ColorData);
             }

			XM_InvalidateWindow ();
			XM_UpdateWindow ();
            break;
        case VK_AP_FONT_BACK_SWITCH:
        //case REMOTE_KEY_LEFT:
            AP_ButtonControlMessageHandler (&ColorViewData->btnControl, msg);
            if(ColorViewData->nCurItem < 3) {
                AdjustColorData(ColorData,ColorViewData->nCurItem,-5);
            }else {
                Reset_Color_Data(ColorData);
            }
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
            break;
    }

}

VOID ColorViewOnKeyUp (XMMSG *msg)
{
	printf("ColorViewOnKeyUp \n");
}

VOID ColorViewOnCommand (XMMSG *msg)
{
	COLORVIEWDATA *FormatViewData;
	FormatViewData = (COLORVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ColorView));
	if(FormatViewData == NULL)
		return;
	switch(msg->wp)
	{
		case COLOR_COMMAND_MODIFY:
            #if 0
			XM_printf ("MENUOPTIONVIEW_COMMAND_MODIFY\n");
			// 确认并返回
			if(FormatViewData->lpFormatView && FormatViewData->lpFormatView->fpMenuOptionCB)
			{
				APPMENUOPTION	*lpMenuOptionView = FormatViewData->lpFormatView;
				(*lpMenuOptionView->fpMenuOptionCB) (lpMenuOptionView->lpUserData, FormatViewData->nCurItem);
			}
            #endif
			XM_PullWindow (NULL);
			break;

		case COLOR_COMMAND_RETURN:
			// 返回到上一层
			XM_PullWindow (0);
			break;
	}
}

VOID ColorViewOnTouchDown (XMMSG *msg)
{
	printf("ColorViewOnTouchDown \n");
}

VOID ColorViewOnTouchUp (XMMSG *msg)
{
    printf("ColorViewOnTouchUp \n");
}

VOID ColorViewOnTouchMove (XMMSG *msg)
{
    printf("ColorViewOnTouchMove \n");
}
//Timer
extern BOOL Auto_Menu;

VOID ColorViewOnTimer (XMMSG *msg)
{
	if(!Auto_Menu) {
	    if(AHD_Guide_Start()) {
	        XM_PullWindow (XMHWND_HANDLE(Desktop));
	        //XM_PullWindow (0);
	    }
	}
}

// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (ColorView)
	XM_ON_MESSAGE (XM_PAINT, ColorViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, ColorViewOnKeyDown)
	//XM_ON_MESSAGE (XMKEY_LONGTIME, ColorViewOnKeyLongDown)
	XM_ON_MESSAGE (XMKEY_REPEAT, ColorViewOnKeyRepeatDown)
	XM_ON_MESSAGE (XM_KEYUP, ColorViewOnKeyUp)
	XM_ON_MESSAGE (XM_TIMER, ColorViewOnTimer)
	XM_ON_MESSAGE (XM_ENTER, ColorViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, ColorViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, ColorViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, ColorViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, ColorViewOnTouchUp)
	XM_ON_MESSAGE (XM_TOUCHMOVE, ColorViewOnTouchMove)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, ColorView, 0, 0, 0, 0, HWND_VIEW)
