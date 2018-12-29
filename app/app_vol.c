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
#define	VOL_COMMAND_MODIFY		0
#define	VOL_COMMAND_RETURN		1

// “菜单选项”窗口按钮控件定义
#define	VOLVIEWBTNCOUNT	2
static const XMBUTTONINFO menuOptionBtn[VOLVIEWBTNCOUNT] = {
	{	
		VK_AP_SWITCH,		VOL_COMMAND_MODIFY,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_MODIFY
	},
	{	
		VK_AP_MODE,		VOL_COMMAND_RETURN,	AP_ID_COMMON_OK,	AP_ID_BUTTON_RETURN
	},
};


int Timer_Count = 0;


VOID VolViewOnEnter (XMMSG *msg)
{
    //XM_DisableViewAnimate (XMHWND_HANDLE(VolView));
    //创建定时器 500ms 用于关闭界面
    Timer_Count = 0; //清零动作
    XM_SetTimer (XMTIMER_SETTINGVIEW, 200);
}

VOID VolViewOnLeave (XMMSG *msg)
{
	XM_KillTimer(XMTIMER_SETTINGVIEW);
    printf("11111111 VolViewOnLeave \n");
}


#define GuageImageNum 20

VOID VolViewOnPaint (XMMSG *msg)
{
	XMRECT rc,rect;
	unsigned int old_alpha;
    XMCOLOR bkg_clolor;
    int StepNum = 0;
    int i;
    char String[32];
    XMSIZE size;

	HANDLE hwnd = XMHWND_HANDLE(VolView);
	// 显示标题栏
	XM_GetDesktopRect (&rc); 
    bkg_clolor = XM_GetSysColor(XM_COLOR_DESKTOP);
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, bkg_clolor);
	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);

    rect.left = 380;
    rect.top =  540;
    #if 0
    sprintf(String,"%s","VOL");
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString (hwnd, rect.left, rect.top, String, strlen(String));
    #endif
    AP_RomImageDrawByMenuID (AP_ID_VOL_SELECT, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

    StepNum = AP_GetAudo_PWM();
    rect = rc;
    rect.left = 450;
    rect.top = 550;
    for(i = 0; i < GuageImageNum; i ++)
    {
        rect.left +=8;
        if(i <  StepNum) {
            AP_RomImageDrawByMenuID (AP_ID_SETTING_SELECT, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
        }else if(i >= StepNum){
            rect.top = 560;
            AP_RomImageDrawByMenuID (AP_ID_SETTING_NO_SELECT, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
        }
    }

    //显示数值
    rect.left = 620;
    rect.top =  540;
    sprintf(String,"%d",StepNum);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString (hwnd, rect.left, rect.top, String, strlen(String));
	XM_SetWindowAlpha (hwnd, old_alpha);
}

VOID VolViewOnKeyRepeat(XMMSG *msg)
{
    int Data_Vol = 0;
    Data_Vol = AP_GetAudo_PWM();
    switch(msg->wp)
    {
        case REMOTE_KEY_LEFT:
        case VK_AP_UP:
            Timer_Count = 0;
            if(Data_Vol < 20) {
                AP_SetAudo_PWM(++ Data_Vol);
                //HW_Auido_SetLevel(Data_Vol);
                APP_SaveMenuData();
                XM_InvalidateWindow ();
    		    XM_UpdateWindow ();
            }
            break;
       // case REMOTE_KEY_RIGHT:
        case VK_AP_DOWN:
            Timer_Count = 0;
            if(Data_Vol > 0) {
                AP_SetAudo_PWM(-- Data_Vol);
                //HW_Auido_SetLevel(Data_Vol);
                APP_SaveMenuData();
                XM_InvalidateWindow ();
    		    XM_UpdateWindow ();
            }
            break;
    }
}

extern BOOL Auto_Menu;
VOID VolViewOnKeyDown (XMMSG *msg)
{
    int Data_Vol = 0;
    Data_Vol = AP_GetAudo_PWM();
    switch(msg->wp)
    {
        case REMOTE_KEY_RIGHT:
        case VK_AP_UP:
            Timer_Count = 0;
            if(Data_Vol < 20) {
                AP_SetAudo_PWM(++ Data_Vol);
                //HW_Auido_SetLevel(Data_Vol);
                APP_SaveMenuData();
                XM_InvalidateWindow ();
    		    XM_UpdateWindow ();
            }
            break;
        case REMOTE_KEY_LEFT:
        case VK_AP_DOWN:
            Timer_Count = 0;
            if(Data_Vol > 0) {
                AP_SetAudo_PWM(-- Data_Vol);
                //HW_Auido_SetLevel(Data_Vol);
                APP_SaveMenuData();
                XM_InvalidateWindow ();
    		    XM_UpdateWindow ();
            }
            break;
        case VK_AP_MENU:
        case REMOTE_KEY_MENU:
             XM_PushWindow (XMHWND_HANDLE(SettingView));
		    if(AHD_Guide_Start())
				Auto_Menu = TRUE;
            break;
    }
}

VOID VolViewOnKeyUp (XMMSG *msg)
{
	printf("ColorViewOnKeyUp \n");
}

VOID VolViewOnCommand (XMMSG *msg)
{
	
}

VOID VolViewOnTouchDown (XMMSG *msg)
{
	printf("ColorViewOnTouchDown \n");
}

VOID VolViewOnTouchUp (XMMSG *msg)
{
    printf("ColorViewOnTouchUp \n");
}

VOID VolViewOnTouchMove (XMMSG *msg)
{
    printf("ColorViewOnTouchMove \n");
}
VOID VolViewOnTimer (XMMSG *msg)
{
    Timer_Count ++;
    if(Timer_Count > 10) {
        Timer_Count = 0;
        XM_PullWindow (0);
    }
}

// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (VolView)
	XM_ON_MESSAGE (XM_PAINT, VolViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, VolViewOnKeyDown)
	XM_ON_MESSAGE (XMKEY_REPEAT, VolViewOnKeyRepeat)
	XM_ON_MESSAGE (XM_KEYUP, VolViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, VolViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, VolViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, VolViewOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, VolViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, VolViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, VolViewOnTouchUp)
	XM_ON_MESSAGE (XM_TOUCHMOVE, VolViewOnTouchMove)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, VolView, 0, 0, 0, 10, HWND_VIEW)

