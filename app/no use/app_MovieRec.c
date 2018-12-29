
#include "app.h"
#include "app_menuid.h"
#include "app_menuoptionview.h"
#include "types.h"

VOID MovieViewOnEnter (XMMSG *msg)
{
    XM_DisableViewAnimate (XMHWND_HANDLE(MovieRec));
    //创建定时器 500ms 用于关闭界面
    XM_SetTimer (XMTIMER_DESKTOPVIEW, 500);
}

VOID MovieViewOnLeave (XMMSG *msg)
{
	XM_KillTimer(XMTIMER_DESKTOPVIEW);
}

VOID MovieViewOnPaint (XMMSG *msg)
{
    char String[32];
    XMRECT rc,rect;
	unsigned int old_alpha;
    XMCOLOR bkg_clolor;
    XMSIZE size;
    HANDLE hwnd = XMHWND_HANDLE(MovieRec);

    // 显示标题栏
	XM_GetDesktopRect (&rc); 
    bkg_clolor = XM_GetSysColor(XM_COLOR_DESKTOP);
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, 0xffffffff);
	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);

    

    #if 1 //时间水印
    XMSYSTEMTIME local_time;
    XM_GetLocalTime (&local_time);
    rect = rc;
    rect.left = 20;
    rect.top =  rect.bottom - 40;
    sprintf(String,"%04d:%02d:%02d %02d:%02d:%02d",local_time.wYear,local_time.bMonth,local_time.bDay,local_time.bHour,local_time.bMinute,local_time.bSecond);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString (hwnd, rect.left, rect.top, String, strlen(String));
    #endif
    XM_SetWindowAlpha (hwnd, old_alpha);
}


VOID MovieViewOnKeyDown (XMMSG *msg)
{
    switch(msg->wp)
    {
    }
}

VOID MovieViewOnKeyUp (XMMSG *msg)
{
	printf("ColorViewOnKeyUp \n");
}

VOID MovieViewOnCommand (XMMSG *msg)
{
	
}

VOID MovieViewOnTouchDown (XMMSG *msg)
{
	printf("ColorViewOnTouchDown \n");
}

VOID MovieViewOnTouchUp (XMMSG *msg)
{
    printf("ColorViewOnTouchUp \n");
}

VOID MovieViewOnTouchMove (XMMSG *msg)
{
    printf("ColorViewOnTouchMove \n");
}
VOID MovieViewOnTimer (XMMSG *msg)
{
    XM_InvalidateWindow ();
    XM_UpdateWindow ();
}

// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (MovieRec)
	XM_ON_MESSAGE (XM_PAINT, MovieViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, MovieViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, MovieViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, MovieViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, MovieViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, MovieViewOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, MovieViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, MovieViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, MovieViewOnTouchUp)
	XM_ON_MESSAGE (XM_TOUCHMOVE, MovieViewOnTouchMove)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, MovieRec, 0, 0, 0, 0, HWND_EVENT)
