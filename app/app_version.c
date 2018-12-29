//软件版本号
#include "rtos.h"		// 任务相关函数
#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"
#include "systemapi.h"
#include "app_processview.h"
#include "xm_h264_codec.h"
#include <xm_gif.h>
#include <xm_semaphore.h>
#include "xm_autotest.h"
#include "fs.h"
#include "xm_proj_define.h"
#include "xm_flash_space_define.h"

static u8 exit_timeout;

VOID VersionViewOnEnter(XMMSG *msg)
{
	if(msg->wp == 0)
	{
		// 窗口未建立，第一次进入
		XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>VersionViewOnEnter, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

		// 标记卡重新检测并关闭视频播放
		//APPMarkCardChecking (1);
		exit_timeout = 0;
		// 设置视窗的alpha值
		XM_SetWindowAlpha(XMHWND_HANDLE(VersionView), (unsigned char)(0));
		XM_SetWindowPos(XMHWND_HANDLE(VersionView), 287, 169, 441, 258);
	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf(">>>>>>>>>>>>>>VersionView Pull......................\n");
	}
	
	//创建定时器，用于菜单的隐藏
	//创建x秒的定时器
	XM_SetTimer(XMTIMER_VERSION, 1000);
}


VOID VersionViewOnLeave (XMMSG *msg)
{
	// 删除定时器
	XM_KillTimer (XMTIMER_VERSION);
	
	if (msg->wp == 0)
	{
		XM_printf ("VersionView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("VersionView Push\n");
	}
}

VOID VersionViewOnPaint (XMMSG *msg)
{
	char text[64];
	char String[32];
	XMSIZE size;
	XMRECT rc, rect;
	unsigned int old_alpha;
	HANDLE hwnd = XMHWND_HANDLE(VersionView);

	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>VersionViewOnPaint, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

    XM_GetWindowRect(XMHWND_HANDLE(Desktop),&rc);
	XM_FillRect(hwnd, rc.left, rc.top, rc.right, rc.bottom, 0x00000000);
	
	// 获取视窗位置信息
	XM_GetWindowRect(XMHWND_HANDLE(VersionView), &rc);
	XM_FillRect(XMHWND_HANDLE(VersionView), rc.left, rc.top, rc.right, rc.bottom, 0xB0404040);

	old_alpha = XM_GetWindowAlpha(hwnd);
	XM_SetWindowAlpha (hwnd, 255);

	//背景图片
	rect = rc;
	AP_RomImageDrawByMenuID(AP_ID_ALERT_BG_PIC, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	//软件版本号
	rect = rc;
	rect.left = rect.left + 174; rect.top = rect.top + 4;
	AP_RomImageDrawByMenuID(AP_ID_SYS_MENU_TXT_VERSION, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	rect = rc;
	rect.left = rect.left + 140; rect.top = rect.top + 100;	
    sprintf(String,"%s", "VER:181219");
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString(hwnd, rect.left, rect.top, String, strlen(String));

	XM_SetWindowAlpha(hwnd, (unsigned char)old_alpha);
}

VOID VersionViewOnKeyDown (XMMSG *msg)
{
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>VersionViewOnKeyDown, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

	//按键音
	XM_Beep(XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
		case VK_AP_DOWN:
		case VK_AP_UP:
			XM_PullWindow(0);
			break;

		case VK_AP_SWITCH:
			XM_PullWindow(0);
			break;
			
		default:
			break;
	}

}


VOID VersionViewOnTimer (XMMSG *msg)
{
	XM_printf(">>>>>>>>>>>>>>>>>VersionViewOnTimer..............\r\n");
	exit_timeout++;
	if(exit_timeout>=3)
	{
		XM_PullWindow(0);
	}
}

// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (VersionView)
	XM_ON_MESSAGE (XM_PAINT, VersionViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, VersionViewOnKeyDown)
	XM_ON_MESSAGE (XM_ENTER, VersionViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, VersionViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, VersionViewOnTimer)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, VersionView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_ALERT)

