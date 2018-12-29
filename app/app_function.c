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

// 私有命令定义
#define	FUNCTION_COMMAND_MODIFY		0
#define	FUNCTION_COMMAND_RETURN		1

// “菜单选项”窗口按钮控件定义
#define	FUNCTIONVIEWBTNCOUNT	2
static const XMBUTTONINFO menuOptionBtn[FUNCTIONVIEWBTNCOUNT] = {
	{
		VK_AP_SWITCH,		FUNCTION_COMMAND_MODIFY,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_MODIFY
	},
	{
		VK_AP_MODE,		FUNCTION_COMMAND_RETURN,	AP_ID_COMMON_OK,	AP_ID_BUTTON_RETURN
	},
};

typedef struct _tagFUNCTIONMENUDATA {
    u8_t            nRotateData;
	u8_t            nAuto_BrightnessData;
	u8_t            nBlueData;
}FUNCTIONMENUDATA;

typedef struct tagFUNCTIONMENUDAT {
	int				nTopItem;					// 第一个可视的菜单项
	int				nCurItem;					// 当前选择的菜单项
	int				nItemCount;					// 菜单项个数
	APPMENUOPTION	*lpFunctionView;
    FUNCTIONMENUDATA *functionData;
	XMBUTTONCONTROL	btnControl;				// 按钮控件
	XMTITLEBARCONTROL	titleControl;			// 标题控件

} FUNCTIONMENUDAT;

enum  {
    FUNCTION_LCD_ROTATE = 0,
    FUNCTION_AUTO_BRIGHT,
    FUNCTION_BLUEDATA,
    FUNCTION_COUNT
}Function_Define;


VOID FunctionViewOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
	    u32_t VP_CONTRAST = 0;
	    FUNCTIONMENUDAT *FunctionViewData = XM_calloc (sizeof(FUNCTIONMENUDAT));
            APPMENUOPTION *lpFunctionView = (APPMENUOPTION *)msg->lp;
            FUNCTIONMENUDATA *functionData = FunctionViewData->functionData;
		if(FunctionViewData == NULL)
		{
			XM_printf ("menuOptionViewData XM_calloc failed\n");
			// 失败返回到桌面
			XM_PullWindow (0);
			return;
		}
        FunctionViewData->nItemCount = FUNCTION_COUNT;
        functionData->nRotateData = AppMenuData.LCD_Rotate;
        functionData->nAuto_BrightnessData = AppMenuData.auto_brightness;
        functionData->nBlueData = AppMenuData.blue_show;

		//标题初始化
		AP_TitleBarControlInit (&FunctionViewData->titleControl, XMHWND_HANDLE(FunctionView),
														lpFunctionView->dwWindowIconId, lpFunctionView->dwWindowTextId);
		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(FunctionView), FunctionViewData);

	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("FormatSettingView Pull\n");
	}
    XM_SetTimer (XMTIMER_SETTINGVIEW, 200); //开启定时器
}
//extern BOOL Modeify_Param;

VOID FunctionViewOnLeave (XMMSG *msg)
{
    XM_KillTimer (XMTIMER_SETTINGVIEW);
	if (msg->wp == 0)
	{
//		int i;
		FUNCTIONMENUDAT *FunctionViewData = (FUNCTIONMENUDAT *)XM_GetWindowPrivateData (XMHWND_HANDLE(FunctionView));
		if(FunctionViewData)
		{
			// 标题控件退出过程
			AP_TitleBarControlExit (&FunctionViewData->titleControl);
			// 释放私有数据句柄
			XM_free (FunctionViewData);
		}
		XM_printf ("FormatSettingView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("FormatSettingView Push\n");
	}
    //Modeify_Param = TRUE;
}


#define Start_SHOW_MENU 144
VOID FunctionViewOnPaint (XMMSG *msg)
{
	XMRECT rc,rect,rcArrow;
	unsigned int old_alpha;
    char String[32];
    XMSIZE size;
    XMCOORD menu_name_x;
    int nVisualCount = 3;
    int i;
    int nItem = 0, nLastItem;
    DWORD menuID;
    int Middle_Five = 0;
    int Middle_Data = 0;
    int Middle_String = 0;
	HANDLE hwnd = XMHWND_HANDLE(FunctionView);
	FUNCTIONMENUDAT *FunctionViewData = (FUNCTIONMENUDAT *)XM_GetWindowPrivateData (hwnd);
	if(FunctionViewData == NULL)
		return;

	// 显示标题栏
	XM_GetDesktopRect (&rc);
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);
    FUNCTIONMENUDATA *functionData =  FunctionViewData->functionData;
    menu_name_x = (XMCOORD)(APP_POS_ITEM5_MENUNAME_X * 1);

	// ********* 1 显示标题栏区信息 *********
	AP_TitleBarControlMessageHandler (&FunctionViewData->titleControl, msg);
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    rect = rc;
	rect.left = 0;
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (FunctionViewData->nCurItem - FunctionViewData->nTopItem)  * APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    rect = rc;
    rcArrow = rc; //分割线
    rcArrow.top = APP_POS_ITEM5_SPLITLINE_Y;
    rect.top =55;
    nLastItem = FunctionViewData->nItemCount;
    if(nLastItem >(FunctionViewData->nTopItem + nVisualCount))
        nLastItem = FunctionViewData->nTopItem + nVisualCount;
    for(nItem = FunctionViewData->nTopItem; nItem <nLastItem; nItem ++)
    {
        //跳过自动背光
        if(APP_GetAuto_DimOnOff() == 0)
        {
            if(nItem == FUNCTION_AUTO_BRIGHT) {
                nItem ++;
            }
        }

        rect.left = APP_POS_ITEM5_MENUNAME_X;
        switch(nItem)
        {
            case FUNCTION_LCD_ROTATE:
                //字符
                AP_RomImageDrawByMenuID (AP_ID_SETTING_ROTATE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
                //数字
                rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
                sprintf(String,"%d",functionData->nRotateData);
                AP_TextGetStringSize(String,sizeof(String),&size);
                AP_TextOutDataTimeString (hwnd, rect.left, rect.top, String, strlen(String));
                break;
            case FUNCTION_AUTO_BRIGHT:
                AP_RomImageDrawByMenuID (AP_ID_SETTING_AUTO_BIGHTNESS, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
               if(functionData->nAuto_BrightnessData == 0) {
                    menuID = AP_ID_SETTING_CLOSE;
                }else {
                    menuID = AP_ID_SETTING_OPEN;
                }
                rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
                AP_RomImageDrawByMenuID (menuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
                break;
            case FUNCTION_BLUEDATA:
                AP_RomImageDrawByMenuID (AP_ID_SETTING_BLUE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
                if(functionData->nBlueData== 0) {
                    menuID = AP_ID_SETTING_CLOSE;
                }else {
                    menuID = AP_ID_SETTING_OPEN;
                }
                rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
                AP_RomImageDrawByMenuID (menuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
                break;
        }
        rect.top += APP_POS_ITEM5_MENUNAME_Y;
        #if 1
        rcArrow.top += APP_POS_ITEM5_LINEHEIGHT;
        rcArrow.left = 0;
        AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rcArrow, XMGIF_DRAW_POS_LEFTTOP);
        #endif
    }

    #if 0
    //选项
    rect = rc;
	rect.top = APP_POS_ITEM5_MENUNAME_Y;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_ROTATE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
     //显示数字
     #if 0
    if(functionData->nRotateData == 0) {
        menuID = AP_ID_SETTING_CLOSE;
    }else {
        menuID = AP_ID_SETTING_OPEN;
    }
    #endif
    rect.left = APP_POS_ITEM5_MENUFLAG_X*2;
    sprintf(String,"%d",functionData->nRotateData);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString (hwnd, rect.left, rect.top, String, strlen(String));
    //rect.left = APP_POS_ITEM5_MENUFLAG_X*2;
    //AP_RomImageDrawByMenuID (menuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 *APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    //contrast
    rect = rc;
    rect.top = APP_POS_ITEM5_MENUNAME_Y *2;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_AUTO_BIGHTNESS, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
   if(functionData->nAuto_BrightnessData == 0) {
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
    //Saturation
    rect = rc;
    rect.top = APP_POS_ITEM5_MENUNAME_Y *3;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID (AP_ID_SETTING_BLUE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    if(functionData->nBlueData== 0) {
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
    #endif

    AP_ButtonControlMessageHandler (&FunctionViewData->btnControl, msg);
	XM_SetWindowAlpha (hwnd, (unsigned char)old_alpha);
}
VOID FunctionViewOnKeyDown (XMMSG *msg)
{
    int nVisualCount;
    int CurItem;
	XMRECT rc;
	FUNCTIONMENUDAT *FunctionViewData = (FUNCTIONMENUDAT *)XM_GetWindowPrivateData (XMHWND_HANDLE(FunctionView));
    FUNCTIONMENUDATA *functionData =  FunctionViewData->functionData;
	if(FunctionViewData == NULL)
		return;

    CurItem = FunctionViewData->nCurItem;
    nVisualCount = FunctionViewData->nItemCount;
    switch(msg->wp)
    {
        case VK_AP_MENU:
        case REMOTE_KEY_MENU:
        case REMOTE_KEY_POWER:
            AppMenuData.LCD_Rotate = functionData->nRotateData;
            AppMenuData.auto_brightness = functionData->nAuto_BrightnessData;
            AppMenuData.blue_show = functionData->nBlueData;
            AP_SaveMenuData (&AppMenuData);
            XM_PullWindow (0);
            break;
        case VK_AP_FONT_BACK_SWITCH:
        case REMOTE_KEY_RIGHT:
			// 刷新
            AP_ButtonControlMessageHandler (&FunctionViewData->btnControl, msg);
            if((CurItem == FUNCTION_AUTO_BRIGHT) && (APP_GetAuto_DimOnOff() == 0))
                 CurItem ++;
            if(CurItem == 0) {
                #if 0
                if(functionData->nRotateData == 0) {
                    functionData->nRotateData = 1;
                }else {
                    functionData->nRotateData = 0;
                }
                #endif
                functionData->nRotateData ++;
                if(functionData->nRotateData > 4)
                   functionData->nRotateData = 1;
                AppMenuData.LCD_Rotate = functionData->nRotateData;
                HW_LCD_ROTATE(functionData->nRotateData);
            }else if(CurItem == 1) {
                if(functionData->nAuto_BrightnessData == 0) {
                    functionData->nAuto_BrightnessData = 1;
                }else {
                    functionData->nAuto_BrightnessData = 0;
                }
            }else {
                if(functionData->nBlueData == 0) {
                    functionData->nBlueData = 1;
                }else {
                    functionData->nBlueData = 0;
                }
                AppMenuData.blue_show = functionData->nBlueData;
                //Select_Background_Color();
            }
            XM_InvalidateWindow ();
            XM_UpdateWindow ();
            break;
        case VK_AP_SWITCH:
        case REMOTE_KEY_LEFT:
            AP_ButtonControlMessageHandler (&FunctionViewData->btnControl, msg);
            if((CurItem == FUNCTION_AUTO_BRIGHT) && (APP_GetAuto_DimOnOff() == 0))
                 CurItem ++;

			if(CurItem == 0) {
                #if 0
                if(functionData->nRotateData == 0) {
                    functionData->nRotateData = 1;
                }else {
                    functionData->nRotateData = 0;
                }
                #endif
                if(functionData->nRotateData == 1)
                    functionData->nRotateData = 4;
                else
                    functionData->nRotateData --;
                AppMenuData.LCD_Rotate = functionData->nRotateData;
                HW_LCD_ROTATE(functionData->nRotateData);
            }else if(CurItem == 1) {
                if(functionData->nAuto_BrightnessData == 0) {
                    functionData->nAuto_BrightnessData = 1;
                }else {
                    functionData->nAuto_BrightnessData = 0;
                }
            }else {
                if(functionData->nBlueData == 0) {
                    functionData->nBlueData = 1;
                }else {
                    functionData->nBlueData = 0;
                }
                AppMenuData.blue_show = functionData->nBlueData;
                Select_Background_Color();
            }
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
            break;
        case VK_AP_UP:
        case REMOTE_KEY_DOWN:
            FunctionViewData->nCurItem ++;
            if(APP_GetAuto_DimOnOff() == 0)
            {
                nVisualCount --;
            }
            if(FunctionViewData->nCurItem >= nVisualCount) {
               FunctionViewData->nCurItem  = 0;
               FunctionViewData->nTopItem = 0;
            }
    		XM_InvalidateWindow ();
    		XM_UpdateWindow ();
            break;
        case VK_AP_DOWN:
        case REMOTE_KEY_UP:
            FunctionViewData->nCurItem --;
            if(APP_GetAuto_DimOnOff() == 0)
            {
                nVisualCount --;
            }
            //#Edison add End #20180608
			if(FunctionViewData->nCurItem < 0)
			{
				// 聚焦到最后一个
				FunctionViewData->nCurItem = (WORD)(nVisualCount - 1);
				FunctionViewData->nTopItem = 0;
				while ((FunctionViewData->nCurItem - FunctionViewData->nTopItem) >= nVisualCount )
				{
					FunctionViewData->nTopItem ++;
				}
			}
			else
			{
				if(FunctionViewData->nTopItem > nVisualCount)
					FunctionViewData->nTopItem = FunctionViewData->nCurItem;
			}
    		XM_InvalidateWindow ();
    		XM_UpdateWindow ();
            break;
            #if 0
            if(FunctionViewData->nCurItem == 0) {
                AppMenuData.LCD_Rotate = functionData->nRotateData;
                HW_LCD_ROTATE(functionData->nRotateData);
            }else if(FunctionViewData->nCurItem == 1) {
                AppMenuData.auto_brightness = functionData->nAuto_BrightnessData;
            }else {
                AppMenuData.blue_show = functionData->nBlueData;
            }
            #endif
    }

}

VOID FunctionViewOnKeyUp (XMMSG *msg)
{
	printf("ColorViewOnKeyUp \n");
}

VOID FunctionViewOnCommand (XMMSG *msg)
{
	FUNCTIONMENUDAT *FormatViewData;
	FormatViewData = (FUNCTIONMENUDAT *)XM_GetWindowPrivateData (XMHWND_HANDLE(FunctionView));
	if(FormatViewData == NULL)
		return;
	switch(msg->wp)
	{
		case FUNCTION_COMMAND_MODIFY:

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

		case FUNCTION_COMMAND_RETURN:
			// 返回到上一层
			XM_PullWindow (0);
			break;
	}
}

VOID FunctionViewOnTouchDown (XMMSG *msg)
{
	printf("ColorViewOnTouchDown \n");
}

VOID FunctionViewOnTouchUp (XMMSG *msg)
{
    printf("ColorViewOnTouchUp \n");
}

VOID FunctionViewOnTouchMove (XMMSG *msg)
{
    printf("ColorViewOnTouchMove \n");
}
#include "types.h"
extern BOOL Auto_Menu;
VOID FunctionViewOnTimer (XMMSG *msg)
{
	if(!Auto_Menu) {
	    if(AHD_Guide_Start()) {
	        //XM_JumpWindow (XMHWND_HANDLE(Desktop));
	        XM_PullWindow(XMHWND_HANDLE(Desktop));
	    }
	}
}
// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (FunctionView)
	XM_ON_MESSAGE (XM_PAINT, FunctionViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, FunctionViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, FunctionViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, FunctionViewOnEnter)
	XM_ON_MESSAGE (XM_TIMER, FunctionViewOnTimer)
	XM_ON_MESSAGE (XM_LEAVE, FunctionViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, FunctionViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, FunctionViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, FunctionViewOnTouchUp)
	XM_ON_MESSAGE (XM_TOUCHMOVE, FunctionViewOnTouchMove)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, FunctionView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
