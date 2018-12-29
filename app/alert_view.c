//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: alert_view.c
//	  提示信息视窗
//		(仅显示提示信息，可定义或禁止定时器自动关闭，一个确认按钮)
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"

#define	ALERT_MAX_BUTTON	3		// 最多3个按键菜单		

#define	ALERT_V_SPACE		30//15		// 垂直方向边距
#define	ALERT_H_SPACE		10		// 水平方向边距

#define	ALERT_BACKGROUND_COLOR		0xFFFFFFFF

typedef struct tagALERTVIEWDATA {
	DWORD				dwTextInfoID;					// 文本信息资源ID
	DWORD				dwImageID;						// 图片信息资源ID
	float				fAutoCloseTime;				// 视窗定时器自动关闭时间。0表示禁止自动关闭功能
	DWORD				dwButtonCount;					// 按钮个数
	DWORD				dwButtonNormalTextID[ALERT_MAX_BUTTON];	//按钮文字资源ID
	DWORD				dwButtonPressedTextID[ALERT_MAX_BUTTON];	//按钮按下文字资源ID
	XMRECT				rectButton[ALERT_MAX_BUTTON];	// 按钮坐标
	float				fViewAlpha;					// 信息视图的视窗Alpha因子，0.0 ~ 1.0
	DWORD				dwAlignOption;					//	视图的显示对齐设置信息
	DWORD				dwOption;						//	ALERTVIEW视图的控制选项

	DWORD				dwBkgColor;

	FPALERTCB			alertcb;						// 用户提供的按键菜单回调函数
	VOID *				UserPrivate;					// 用户私有数据

	// 内部状态
	XMSIZE				sizeTextInfo;	
	XMSIZE				sizeImage;	
	XMSIZE				sizeButton[4];
	XMSIZE				sizeCountDown;
	int			        cbCountDownString;	
	DWORD				dwTimeout;					// 计数器倒计时计数
	int			        btnPressed[4];
} ALERTVIEWDATA;

// ALERT信息显示视窗
//
//	----- 视图对齐模式支持 ( XM_VIEW_ALIGN_CENTRE 及 XM_VIEW_ALIGN_BOTTOM ) -----
//
//	1) 垂直居中/水平居中显示 XM_VIEW_ALIGN_CENTRE
//	2) 底部对齐/水平居中显示 XM_VIEW_ALIGN_BOTTOM
//
//  ---- UI元素布局(Layout) ----
//
// 1) 显示“文字信息内容”“图片信息内容”及“按钮”
//		空白区
//		文字信息内容
//		空白区
//		图片信息内容
//		空白区
//		倒计时区
//		空白区
//		按钮
//		空白区
//
// 2) 仅显示“信息内容”
//		空白区
//		信息内容
//		空白区

// 显示信息视图
// 0 视图参数错误，视图显示失败
// 1 视图创建并显示
XMBOOL XM_OpenAlertView ( 
								 DWORD dwInfoTextID,				// 信息文本资源ID
																		//		非0值，指定显示文字信息的资源ID
								 DWORD dwImageID,					// 图片信息资源ID
																		//		非0值，指定显示图片信息的资源ID	

								 DWORD dwButtonCount,			// 按钮个数
																		//		当超过一个按钮时，
																		//		第一个按钮，使用VK_F1(Menu)
																		//		第二个按键，使用VK_F2(Mode)
																		//		第三个按键，使用VK_F3(Switch)
								 DWORD dwButtonNormalTextID[],	//	按钮文字资源ID
																		//		0 
																		//			表示没有定义Button，Alert仅为一个信息提示窗口。
																		//			按任意键或定时器自动关闭
																		//		其他值
																		//			表示Button文字信息的资源ID，Alert需要显示一个按钮
								 DWORD dwButtonPressedTextID[],	//	按钮按下文字资源ID
																		//		0
																		//			表示没有定义该资源。
																		//			按钮按下时使用dwButtonNormalTextID资源
																		//		其他值
																		//			表示Button按下时文字信息的资源ID
								 DWORD dwBackgroundColor,		// 背景色
																		//		0
																		//			表示使用缺省背景色
																		//		其他值
																		//			表示有效填充背景色，背景色可指定Alpha分量
								 float fAutoCloseTime,			//	指定自动关闭时间 (秒单位)，
																		//		0.0	表示禁止自动关闭
								 float fViewAlpha,				// 信息视图的视窗Alpha因子，0.0 ~ 1.0
																		//		0.0	表示全透
																		//		1.0	表示全覆盖
								 FPALERTCB alertcb,				// 按键回调函数
								 VOID *UserPrivate,				// 用户传入参数

								 DWORD dwAlignOption,			//	视图的显示对齐设置信息
																		//		相对OSD显示的原点(OSD有效区域的左上角)
																		//		XM_VIEW_ALIGN_CENTRE	
																		//			视窗居中对齐(相对OSD显示区域)
																		//		XM_VIEW_ALIGN_BOTTOM
																		//			视窗底部居中对齐(相对OSD显示区域)
								 DWORD dwOption					// ALERTVIEW视图的控制选项
																		//		XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN
																		//			倒计时显示使能
																		//		XM_ALERTVIEW_OPTION_ENABLE_KEYDISABLE
																		//			"禁止按键操作"使能
																		//		XM_ALERTVIEW_OPTION_ENABLE_CALLBACK
																		//			使能按键事件回调函数
								 )
{
	ALERTVIEWDATA *viewData;

	// 参数检查
	if(dwInfoTextID == 0)
	{
		XM_printf ("XM_OpenAlertView NG, dwInfoTextID(%d) must be non-zero\n", dwInfoTextID);
		return 0;
	}
	if(dwAlignOption != XM_VIEW_ALIGN_CENTRE && dwAlignOption != XM_VIEW_ALIGN_BOTTOM)
	{
		XM_printf ("XM_OpenAlertView NG, illegal dwAlignOption(%d)\n", dwAlignOption);
		return 0;
	}
	if(fViewAlpha < 0.0 || fViewAlpha > 1.0)
	{
		XM_printf ("XM_OpenAlertView NG, illegal fViewAlpha(%f), should be 0.0 ~ 1.0\n", fViewAlpha);
		return 0;
	}
	// 自动关闭时间及按键资源不能同时为0
	if(fAutoCloseTime == 0.0 && dwButtonNormalTextID == 0)
	{
		XM_printf ("XM_OpenAlertView NG, both dwAutoCloseTime & dwButtonNormalTextID set to 0\n");
		return 0;
	}
	// XM_ALERTVIEW_OPTION_ENABLE_KEYDISABLE 使能时检查定时器开启
	if(dwOption & XM_ALERTVIEW_OPTION_ENABLE_KEYDISABLE)
	{
		if(fAutoCloseTime == 0.0)
		{
			XM_printf ("XM_OpenAlertView NG, dwAutoCloseTime can't set to 0.0 when XM_ALERTVIEW_OPTION_ENABLE_KEYDISABLE enable\n");
			return 0;
		}
	}
	// XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN 使能时检查定时器开启
	if(dwOption & XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN)
	{
		if(fAutoCloseTime == 0.0)
		{
			XM_printf ("XM_OpenAlertView NG, dwAutoCloseTime can't set to 0.0 when XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN enable\n");
			return 0;
		}
	}

	if(dwOption & XM_ALERTVIEW_OPTION_ADJUST_COUNTDOWN)
	{
		if(!(dwOption & XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN))
		{
			XM_printf ("XM_OpenAlertView NG, XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN should enable when XM_ALERTVIEW_OPTION_ADJUST_COUNTDOWN enable\n");
			return 0;
		}
	}

	if((dwOption & XM_ALERTVIEW_OPTION_ENABLE_CALLBACK) && alertcb == NULL)
	{
		XM_printf ("XM_OpenAlertView NG, alertcb can't set to NULL when XM_ALERTVIEW_OPTION_ENABLE_CALLBACK enable\n");
		return 0;
	}

	if(dwButtonCount > ALERT_MAX_BUTTON)
	{
		XM_printf ("XM_OpenAlertView NG, dwButtonCount(%d) exceed maximum button count(%d)\n", 
						dwButtonCount,
						ALERT_MAX_BUTTON);
		return 0;
	}
	else if(dwButtonCount)
	{
		unsigned int i;
		if(dwButtonNormalTextID == NULL)
		{
			XM_printf ("XM_OpenAlertView NG, dwButtonNormalTextID is NULL\n");
			return 0;
		}
		for (i = 0; i < dwButtonCount; i++)
		{
			if(dwButtonNormalTextID[i] == 0)
			{
				XM_printf ("XM_OpenAlertView NG, dwButtonNormalTextID[%d] is NULL\n", i);
				return 0;
			}
		}
	}
	
	viewData = XM_calloc(sizeof(ALERTVIEWDATA));
	if(viewData == NULL)
		return 0;
	viewData->dwTextInfoID = dwInfoTextID;
	viewData->dwImageID = dwImageID;
	viewData->fAutoCloseTime = fAutoCloseTime;
	viewData->dwButtonCount = dwButtonCount;
	memcpy (viewData->dwButtonNormalTextID, dwButtonNormalTextID, dwButtonCount * sizeof(DWORD));
	memcpy (viewData->dwButtonPressedTextID, dwButtonPressedTextID, dwButtonCount * sizeof(DWORD));
	viewData->dwBkgColor = dwBackgroundColor;
	viewData->fViewAlpha = fViewAlpha;
	viewData->alertcb = alertcb;
	viewData->UserPrivate = UserPrivate;
	viewData->dwAlignOption = dwAlignOption;
	memset (viewData->btnPressed, 0, sizeof(viewData->btnPressed));
	viewData->dwOption = dwOption;
	if(XM_PushWindowEx (XMHWND_HANDLE(AlertView), (DWORD)viewData) == 0)
	{
		XM_free (viewData);
		return 0;
	}
	return 1;
}

VOID AlertViewOnEnter (XMMSG *msg)
{
	ALERTVIEWDATA *viewData;

	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnEnter, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);
	if(msg->wp == 0)
	{
		int x, y, w, h;
		APPROMRES *AppRes;
		XMSIZE sizeInfo;
		XMSIZE sizeImage;
		XMSIZE sizeButton[4];
		XMSIZE sizeCountDown;
		XMRECT rectDesktop;
		// 第一次进入(窗口准备创建)
		viewData = (ALERTVIEWDATA *)msg->lp;

		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(AlertView), viewData);

		// 设置视窗的alpha值
		XM_SetWindowAlpha (XMHWND_HANDLE(AlertView), (unsigned char)(viewData->fViewAlpha * 255));

		// 计算窗口的位置信息 (根据UI元素大小及对齐模式计算)
		XM_GetDesktopRect (&rectDesktop);
		w = 0;
		h = 0;
		sizeInfo.cx = 0;
		sizeInfo.cy = 0;
		sizeImage.cx = 0;
		sizeImage.cy = 0;
		memset (sizeButton, 0, sizeof(sizeButton));
		//sizeButton.cx = 0;
		//sizeButton.cy = 0;
		sizeCountDown.cx = 0;
		sizeCountDown.cy = 0;
		//	1) 计算信息文本的显示区域大小
		AppRes = AP_AppRes2RomRes (viewData->dwTextInfoID);
		if(AppRes)
		{
			XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &sizeInfo);
		}
		//	2) 计算图片信息的显示区域大小
		if(viewData->dwImageID && viewData->dwImageID != AP_NULLID)
		{
			AppRes = AP_AppRes2RomRes (viewData->dwImageID);
			if(AppRes)
			{
				XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &sizeImage);
			}
		}
		// 3) 计算Button的显示区域大小
		if(viewData->dwButtonCount)
		{
			// 需要显示Button
			unsigned int i;
			for (i = 0; i < viewData->dwButtonCount; i ++)
			{
				AppRes = AP_AppRes2RomRes (AP_ID_ALERT_BUTTON_NORMAL);
				if(AppRes)
				{
					XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &sizeButton[i]);
				}
			}
		}
		// 4) 计算倒计时的显示区域大小
		if(viewData->dwOption & XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN)
		{
			// 倒计时使能
			char string[32];
			DWORD count = (DWORD)(viewData->fAutoCloseTime * 10.0 + 0.5);
			sprintf (string, "%02d:%02d", count);
			viewData->cbCountDownString = strlen(string);
			AP_TextGetStringSize (string, strlen(string), &sizeCountDown);
		}

		viewData->sizeTextInfo = sizeInfo;
		viewData->sizeImage = sizeImage;
		viewData->sizeCountDown = sizeCountDown;
		memcpy (viewData->sizeButton, sizeButton, sizeof(sizeButton));

		if(viewData->dwAlignOption == XM_VIEW_ALIGN_CENTRE)
		{
			// view垂直居中/水平居中显示 XM_VIEW_ALIGN_CENTRE
			h = ALERT_V_SPACE;
			w = 0;
			// 累计“文本信息内容”
			h += sizeInfo.cy + ALERT_V_SPACE;
			if(w < sizeInfo.cx)
				w = sizeInfo.cx;
			// 累计“图片信息内容”
			h += sizeImage.cy + ALERT_V_SPACE;
			if(w < sizeImage.cx)
				w = sizeImage.cx;
			// 累加 "倒计时"
			if(viewData->dwOption & XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN)
			{
				h += sizeCountDown.cy + ALERT_V_SPACE;
				if(w < sizeCountDown.cx)
					w = sizeCountDown.cx;
			}
			// 累加 "按钮"
			if(viewData->dwButtonCount)
			{
				unsigned int i;
				for (i = 0; i < viewData->dwButtonCount; i ++)
				{
					h += sizeButton[i].cy + ALERT_V_SPACE;
					if(w < sizeButton[i].cx)
						w = sizeButton[i].cx;
				}
			}
			w += ALERT_H_SPACE + ALERT_H_SPACE;

			/*
			if(viewData->dwButtonNormalTextID)
			{
				// 显示“信息内容”及“按钮”
				//	自顶而下UI元素构成
				//	(空白区+“信息内容”+ 空白区 + "倒计时" + 空白区“按钮” + 空白区)
				h = ALERT_V_SPACE + sizeInfo.cy + ALERT_V_SPACE + sizeButton.cy + ALERT_V_SPACE;
				w = sizeInfo.cx;
				if(sizeInfo.cx < sizeButton.cx)
					w = sizeButton.cx;
				w += ALERT_H_SPACE + ALERT_H_SPACE;
			}
			else
			{
				// 仅显示文字信息
				//	自顶而下UI元素构成
				// (空白区+“信息内容”+ 空白区)
				h = ALERT_V_SPACE + sizeInfo.cy + ALERT_V_SPACE;
				w = ALERT_H_SPACE + sizeInfo.cx + ALERT_H_SPACE;
			}
			*/
			
			if(h > (rectDesktop.bottom - rectDesktop.top + 1))
				h = (rectDesktop.bottom - rectDesktop.top + 1);
			if(w > (rectDesktop.right - rectDesktop.left + 1))
				w = (rectDesktop.right - rectDesktop.left + 1);

			x = (rectDesktop.right - rectDesktop.left + 1 - w) / 2;
			y = (rectDesktop.bottom - rectDesktop.top + 1 - h) / 2;
		}
		else //if(viewData->dwAlignOption == XM_VIEW_ALIGN_BOTTOM)
		{
			/*
			// view底部对齐/水平居中显示 XM_VIEW_ALIGN_BOTTOM
			if(viewData->dwButtonNormalTextID)
			{
				// 显示“信息内容”及“按钮”
				//	自顶而下UI元素构成
				//	(空白区+“信息内容”+ 空白区 + “按钮” + 空白区)
				h = ALERT_V_SPACE + sizeInfo.cy + ALERT_V_SPACE + sizeButton.cy + ALERT_V_SPACE;
			}
			else
			{
				// 仅显示文字信息
				//	自顶而下UI元素构成
				// (空白区+“信息内容”+ 空白区)
				h = ALERT_V_SPACE + sizeInfo.cy + ALERT_V_SPACE;
			}
			*/

			h = ALERT_V_SPACE;
			w = 0;
			// 累计“文字信息内容”
			h += sizeInfo.cy + ALERT_V_SPACE;
			if(w < sizeInfo.cx)
				w = sizeInfo.cx;
			// 累计“图片信息内容”
			h += sizeImage.cy + ALERT_V_SPACE;
			if(w < sizeImage.cx)
				w = sizeImage.cx;
			// 累加 "倒计时"
			if(viewData->dwOption & XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN)
			{
				h += sizeCountDown.cy + ALERT_V_SPACE;
				if(w < sizeCountDown.cx)
					w = sizeCountDown.cx;
			}
			// 累加 "按钮"
			if(viewData->dwButtonCount)
			{
				unsigned int i;
				for (i = 0; i < viewData->dwButtonCount; i ++)
				{
					h += sizeButton[i].cy + ALERT_V_SPACE;
					if(w < sizeButton[i].cx)
						w = sizeButton[i].cx;
				}
			}

			if(h > (rectDesktop.bottom - rectDesktop.top + 1))
				h = (rectDesktop.bottom - rectDesktop.top + 1);

			x = 0;
			w = (rectDesktop.right - rectDesktop.left + 1);
			y = rectDesktop.bottom - h + 1;
		}
			
		XM_SetWindowPos (XMHWND_HANDLE(AlertView), x, y, w, h);
	}
	else
	{
		viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("AlertView Pull\n");
	}

	// 创建定时器
	if(viewData->fAutoCloseTime != 0.0)
	{
		XM_SetTimer (XMTIMER_MESSAGEVIEW, 100);	// 创建100ms的定时器
		viewData->dwTimeout = (DWORD)(viewData->fAutoCloseTime * 10.0 + 0.5);
	}
}

VOID AlertViewOnLeave (XMMSG *msg)
{
	ALERTVIEWDATA *viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));

	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnLeave, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);
	// 删除定时器
	XM_KillTimer (XMTIMER_MESSAGEVIEW);

	if (msg->wp == 0)
	{
		if(viewData)
		{
			// 释放私有数据句柄
			XM_free (viewData);
			
		}
		// 设置窗口的私有数据句柄为空
		XM_SetWindowPrivateData (XMHWND_HANDLE(AlertView), NULL);
		XM_printf ("AlertView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("AlertView Push\n");
	}
}

// 仅刷新CountDown区域，在非XM_PAINT消息处理流程中调用
static void CountDownPaint (void)
{
	XMRECT rc, rect;
	XMRECT rectDesktop;
	XMSIZE sizeCountDown;
	char string[32];
	//APPROMRES *AppRes;
	int x, y;
	int minute, second;
	XMCOLOR bkg_clolor;
	ALERTVIEWDATA *viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));
	if(viewData == NULL)
		return;
	
	//XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>CountDownPaint, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

	XM_GetWindowRect (XMHWND_HANDLE(AlertView), &rc);
	XM_GetDesktopRect (&rectDesktop);

	if(viewData->dwOption & XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN)
	{
		xm_osd_framebuffer_t framebuffer;

		// 此处尚未完善
		// 1)	创建一个与视窗关联的framebuffer对象，XM_osd_framebuffer_create
		framebuffer = XM_osd_framebuffer_create (
									XM_LCDC_CHANNEL_0, 
									XM_OSD_LAYER_2,	// ALERTVIEW位于OSD 2层
									XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),	// 获取UI层的显示格式
									rectDesktop.right - rectDesktop.left + 1,
									rectDesktop.bottom - rectDesktop.top + 1,
									XMHWND_HANDLE(AlertView),
									1,			// 复制最近一次相同视窗的framebuffer的视频缓冲区数据,
									0
									);
		if(framebuffer == NULL)
			return;

		//	2)	设置framebuffer对象到视窗对象 XM_SetWindowFrameBuffer
		XM_SetWindowFrameBuffer (XMHWND_HANDLE(AlertView), framebuffer);

		// 3) 执行显示过程
		y = rc.top;
		y += ALERT_V_SPACE + viewData->sizeTextInfo.cy + ALERT_V_SPACE;
		// 检查是否需要显示图片信息
		if(viewData->sizeImage.cy)
			y += viewData->sizeImage.cy + ALERT_V_SPACE;
		x = rc.left + (rc.right - rc.left + 1 - viewData->sizeCountDown.cx) / 2;
		// 刷新需要显示的区域背景
		rect.left = x;
		rect.top = y;
		rect.right = x + viewData->sizeCountDown.cx - 1;
		rect.bottom = y + viewData->sizeCountDown.cy - 1;
		if(viewData->dwBkgColor)
			bkg_clolor = viewData->dwBkgColor;
		else
			bkg_clolor = ALERT_BACKGROUND_COLOR;	// 使用与背景刷新相同的颜色及Alpha值
		XM_FillRect (XMHWND_HANDLE(AlertView), rect.left, rect.top, rect.right, rect.bottom, bkg_clolor);
		// 更新倒计时计数器显示
		minute = (viewData->dwTimeout / 10) / 60;
		second = (viewData->dwTimeout / 10) % 60;
		sprintf (string, "%02d:%02d", minute,  second);
		AP_TextGetStringSize (string, strlen(string), &sizeCountDown);
		x = rc.left + (rc.right - rc.left + 1 - sizeCountDown.cx) / 2;
		// AP_TextOutDataTimeString (XMHWND_HANDLE(AlertView), x, y, string, strlen(string));
		AP_TextOutWhiteDataTimeString (XMHWND_HANDLE(AlertView), x, y, string, strlen(string));

		// 4) 设置NULL framebuffer对象到视窗对象 XM_SetWindowFrameBuffer
		XM_SetWindowFrameBuffer (XMHWND_HANDLE(AlertView), NULL);

		// 5)	关闭framebuffer,将修改刷新到显示设备 XM_osd_framebuffer_close
		XM_osd_framebuffer_close (framebuffer, 0);
	}
}

VOID AlertViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
	XMCOLOR bkg_clolor;
	BYTE old_alpha;
	DWORD menuID;
	HANDLE hWnd = XMHWND_HANDLE(AlertView);
	ALERTVIEWDATA *viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (hWnd);
	if(viewData == NULL)
		return;
	
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnPaint, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

	// 获取视窗位置信息
	XM_GetWindowRect (XMHWND_HANDLE(AlertView), &rc);
	if(viewData->dwBkgColor)
		bkg_clolor = viewData->dwBkgColor;
	else
		bkg_clolor = ALERT_BACKGROUND_COLOR;	// 使用与背景刷新相同的颜色及Alpha值
		
	XM_FillRect(XMHWND_HANDLE(AlertView), rc.left, rc.top, rc.right, rc.bottom, bkg_clolor);

	// 显示Alert信息
	rect = rc;
	rect.bottom = rect.top + ALERT_V_SPACE + viewData->sizeTextInfo.cy + ALERT_V_SPACE - 1 ;

	// 区域居中显示
	old_alpha = XM_GetWindowAlpha (XMHWND_HANDLE(AlertView));
	XM_SetWindowAlpha (XMHWND_HANDLE(AlertView), 255);
	AP_RomImageDrawByMenuID (viewData->dwTextInfoID, XMHWND_HANDLE(AlertView), &rect, XMIMG_DRAW_POS_CENTER);
	XM_SetWindowAlpha (XMHWND_HANDLE(AlertView), old_alpha);
	
	// 显示图片信息
	if(viewData->sizeImage.cy)
	{
		// 存在图片信息
		rect = rc;
		rect.top += ALERT_V_SPACE + viewData->sizeTextInfo.cy + ALERT_V_SPACE;
		rect.bottom = rect.top + viewData->sizeImage.cy;

		// 区域居中显示
		old_alpha = XM_GetWindowAlpha (XMHWND_HANDLE(AlertView));
		XM_SetWindowAlpha (XMHWND_HANDLE(AlertView), 255);			
		AP_RomImageDrawByMenuID (viewData->dwImageID, XMHWND_HANDLE(AlertView), &rect, XMIMG_DRAW_POS_CENTER);
		XM_SetWindowAlpha (XMHWND_HANDLE(AlertView), old_alpha);
	}

	// 倒计时显示
	if(viewData->dwOption & XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN)
	{
		int x, y;
		int minute, second;
		XMSIZE sizeCountDown;
		char string[32];
		y = rc.top;
		y += ALERT_V_SPACE + viewData->sizeTextInfo.cy + ALERT_V_SPACE;
		if(viewData->sizeImage.cy)
			y += viewData->sizeImage.cy + ALERT_V_SPACE;

		minute = (viewData->dwTimeout / 10) / 60;
		second = (viewData->dwTimeout / 10) % 60;
		sprintf (string, "%02d:%02d", minute,  second);
		AP_TextGetStringSize (string, strlen(string), &sizeCountDown);
		x = rc.left + (rc.right - rc.left + 1 - sizeCountDown.cx) / 2;
		// AP_TextOutDataTimeString (XMHWND_HANDLE(AlertView), x, y, string, strlen(string));
		AP_TextOutWhiteDataTimeString (XMHWND_HANDLE(AlertView), x, y, string, strlen(string));
	}

	// 按钮显示
	if(viewData->dwButtonCount)
	{
		// 显示按钮背景
		unsigned int i;
		old_alpha = XM_GetWindowAlpha (XMHWND_HANDLE(AlertView));
		XM_SetWindowAlpha (XMHWND_HANDLE(AlertView), 255);			
		rect = rc;
		//rect.top = rect.bottom - (ALERT_H_SPACE + viewData->sizeButton[0].cy + ALERT_H_SPACE) + 1;
		//rect.top = rect.bottom - (ALERT_H_SPACE) + 1;
		rect.top = rect.bottom;
		for (i = 0; i < viewData->dwButtonCount; i ++)
		{
			rect.top -= ALERT_V_SPACE + viewData->sizeButton[i].cy;
			//rect.bottom -= ALERT_H_SPACE + viewData->sizeButton[i].cy;
		}
		
		for (i = 0; i < viewData->dwButtonCount; i ++)
		{
			XM_IMAGE *imageButtonTxt, *imageButtonKey;
			imageButtonTxt = NULL;
			imageButtonKey = NULL;
			rect.bottom = rect.top + viewData->sizeButton[i].cy - 1;
			if(viewData->btnPressed[i])
			{
				menuID = AP_ID_ALERT_BUTTON_PRESSED;
			}
			else
			{
				menuID = AP_ID_ALERT_BUTTON_NORMAL;
			}

			viewData->rectButton[i] = rect;
			AP_RomImageDrawByMenuID (menuID, XMHWND_HANDLE(AlertView), &rect, XMIMG_DRAW_POS_CENTER);

			// 显示按钮文字
			if(viewData->btnPressed[i])
			{
				//AppRes = AP_AppRes2RomRes (viewData->dwButtonPressedTextID[i]);
				menuID = viewData->dwButtonPressedTextID[i];
			}
			else
			{
				//AppRes = AP_AppRes2RomRes (viewData->dwButtonNormalTextID[i]);
				menuID = viewData->dwButtonNormalTextID[i];
			}
			
#if 0
			// 计算 "Button文字" 及 "Button快捷键" 的尺寸, 居中对齐
			{
			int image_w, image_h;
			int pos_x, pos_y;
			AppRes = AP_AppRes2RomRes (menuID);
			if(AppRes)
				//imageButtonTxt = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
				imageButtonTxt = XM_RomImageCreate (AppRes->rom_offset, AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
			if(i == 0)
				AppRes = AP_AppGetKeyClueRes (VK_AP_MENU);
			else if(i == 1)
				AppRes = AP_AppGetKeyClueRes (VK_AP_MODE);
			else
				AppRes = AP_AppGetKeyClueRes (VK_AP_SWITCH);
			if(AppRes)
				//imageButtonKey = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
				imageButtonKey = XM_RomImageCreate (AppRes->rom_offset, AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);

			image_w = imageButtonTxt->width + 8 + imageButtonKey->width;		// 8 位图之间的空白
			image_h = imageButtonTxt->height;
			if(image_h < imageButtonKey->height)
				image_h = imageButtonKey->height;

			// 水平居中
			pos_x = ((rect.right - rect.left + 1) - image_w) / 2 + rect.left;
			pos_y = ((rect.bottom - rect.top + 1) - imageButtonTxt->height) / 2 + rect.top;
			XM_ImageDisplay (imageButtonTxt, hWnd, pos_x, pos_y);
			pos_x += imageButtonTxt->width + 8;
			XM_ImageDelete (imageButtonTxt);

			pos_y = ((rect.bottom - rect.top + 1) - imageButtonKey->height) / 2 + rect.top;
			XM_ImageDisplay (imageButtonKey, hWnd, pos_x, pos_y);
			XM_ImageDelete (imageButtonKey);
			}
#else
			AP_RomImageDrawByMenuID (menuID, XMHWND_HANDLE(AlertView), &rect, XMIMG_DRAW_POS_CENTER);
#endif

			rect.top += ALERT_V_SPACE + viewData->sizeButton[i].cy;
			//rect.bottom += ALERT_H_SPACE + viewData->sizeButton[i].cy;
		}
		XM_SetWindowAlpha (XMHWND_HANDLE(AlertView), old_alpha);
	}
}


VOID AlertViewOnKeyDown (XMMSG *msg)
{
	ALERTVIEWDATA *viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));
	if(viewData == NULL)
		return;

	
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnKeyDown, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);
	switch(msg->wp)
	{
		case VK_AP_MENU:		// 菜单键
		case VK_AP_DOWN:
			printf(">>>VK_AP_DOWN, viewData->btnPressed[0]: %d viewData->btnPressed[1]: %d \r\n",viewData->btnPressed[0],viewData->btnPressed[1]);
			viewData->btnPressed[0] = 0;
			viewData->btnPressed[1] = 1;
			if(viewData->dwButtonCount > 0)
			{
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			break;

		case VK_AP_MODE:		// 切换到“行车记录”状态
		case VK_AP_UP:
			printf(">>>VK_AP_UP, viewData->btnPressed[0]: %d viewData->btnPressed[1]: %d \r\n",viewData->btnPressed[0],viewData->btnPressed[1]);
			viewData->btnPressed[1] = 0;
			viewData->btnPressed[0] = 1;
			if(viewData->dwButtonCount > 1)
			{
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			break;

		case VK_AP_SWITCH:
			printf(">>>VK_AP_SWITCH, viewData->btnPressed[0]: %d viewData->btnPressed[1]: %d \r\n",viewData->btnPressed[0],viewData->btnPressed[1]);
			if(viewData->btnPressed[0] == 1)
				XM_PostMessage(XM_COMMAND, VK_AP_MENU, 0);
			else 
				XM_PullWindow (0);
				
			
			#if 0
			viewData->btnPressed[2] = 1;
			if(viewData->dwButtonCount > 2)
			{
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			#endif
			break;
		#if 0
		case VK_AP_UP:
			printf("11111111111111111 VK_AP_UP \n");
			/*
			if(viewData->dwOption & (XM_ALERTVIEW_OPTION_ADJUST_COUNTDOWN))
			{
				if(viewData->dwTimeout > 0)
				{
					int timeout = viewData->dwTimeout;
					timeout -= 100;
					if(timeout < 0)
						timeout = 0;
					viewData->dwTimeout = timeout;
					XM_InvalidateWindow ();
					XM_UpdateWindow ();
				}
			}
			*/
			break;

		case VK_AP_DOWN:			// 一键拍照
		printf("11111111111111111 VK_AP_DOWN \n");
			/*
			if(viewData->dwOption & (XM_ALERTVIEW_OPTION_ADJUST_COUNTDOWN))
			{
				viewData->dwTimeout += 100;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			*/
			//AP_OnekeyPhotograph ();
			break;
		#endif
		default:
			// 此处将键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			break;

	}
}

VOID AlertViewOnKeyUp (XMMSG *msg)
{
	ALERTVIEWDATA *viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));
	if(viewData == NULL)
		return;
	
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnKeyUp, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

	switch(msg->wp)
	{
		case VK_AP_MENU:		// 菜单键
			if(viewData->dwButtonCount > 0 && viewData->btnPressed[0])
			{
				viewData->btnPressed[0] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();

				XM_PostMessage (XM_COMMAND, msg->wp, 0);
			}
			else if(viewData->dwButtonCount == 0 && viewData->btnPressed[0])
			{
				XM_PostMessage (XM_COMMAND, msg->wp, 0);
			}
			break;

		case VK_AP_MODE:		// 切换到“行车记录”状态
			if(viewData->dwButtonCount > 1 && viewData->btnPressed[1])
			{
				viewData->btnPressed[1] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();

				XM_PostMessage (XM_COMMAND, msg->wp, 1);
			}
			else if(viewData->dwButtonCount == 0 && viewData->btnPressed[1])
			{
				XM_PostMessage (XM_COMMAND, msg->wp, 0);
			}
			break;

		case VK_AP_SWITCH:
			if(viewData->dwButtonCount > 2 && viewData->btnPressed[2])
			{
				viewData->btnPressed[2] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();

				XM_PostMessage (XM_COMMAND, msg->wp, 2);
			}
			else if(viewData->dwButtonCount == 0 && viewData->btnPressed[2])
			{
				XM_PostMessage (XM_COMMAND, msg->wp, 0);
			}
			break;

		default:
			// 此处将键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			break;
	}
}

VOID AlertViewOnCommand (XMMSG *msg)
{
	ALERTVIEWDATA *viewData;

	viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));
	if(viewData == NULL)
		return;
	
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnCommand, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);
	
	// 若存在按键处理回调函数，由回调函数负责处理。
	// 否则缺省关闭视窗，返回到调用者
	if(viewData->dwOption & XM_ALERTVIEW_OPTION_ENABLE_CALLBACK && viewData->alertcb)
	{
		(*viewData->alertcb) (viewData->UserPrivate, msg->wp);
	}
	else
	{
		// 返回到调用者窗口
		XM_PullWindow (0);
	}

}

VOID AlertViewOnTimer (XMMSG *msg)
{
	ALERTVIEWDATA *viewData;
	viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));
	if(viewData == NULL)
		return;

	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnTimer, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);
	
	if(viewData->dwTimeout > 0)
	{
		viewData->dwTimeout --;
	}
	if(viewData->dwTimeout == 0)
	{
		// 返回到调用者视窗
		if(viewData->dwOption & XM_ALERTVIEW_OPTION_ENABLE_CALLBACK && viewData->alertcb)
		{
			XM_PostMessage (XM_COMMAND, 0, 0);
		}
		else
		{
			XM_PullWindow (0);
		}
	}
	else if((viewData->dwTimeout % 10) == 0)
	{
	      
		// XM_InvalidateWindow ();	
		CountDownPaint ();
	}
}

// 弹出窗口无法处理系统消息事件，退出并交给系统缺省处理
VOID AlertViewOnSystemEvent(XMMSG *msg)
{
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlertViewOnSystemEvent, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

#if 0
	// 一键拍照事件
	if(msg->wp == SYSTEM_EVENT_ONE_KEY_PHOTOGRAPH)
	{
		// 系统缺省处理
		return;
	}

	// 返回到调用者视窗
	// 终止将该消息发送给系统缺省处理
	XM_BreakSystemEventDefaultProcess (msg);
	// 将该消息重新插入到消息队列队首
	XM_InsertMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
	// 弹出当前的alert视窗
	XM_PullWindow (0);
#endif
}

// TouchDown
VOID AlertViewOnTouchDown (XMMSG *msg)
{
	int index;
	int x, y;
	ALERTVIEWDATA *viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));
	if(viewData == NULL)
		return;
	
	if(viewData->dwButtonCount == 0)
		return;

	x = LOWORD(msg->lp);
	x = x - 640;
	y = HIWORD(msg->lp);
	
	for (index = 0; index < viewData->dwButtonCount; index ++)
	{
		if(	x >= viewData->rectButton[index].left 
			&& x < viewData->rectButton[index].right
			&& y >= viewData->rectButton[index].top
			&& y < viewData->rectButton[index].bottom
			)
		{
			break;
		}
	}

	if(index == viewData->dwButtonCount)
		return;

	viewData->btnPressed[index] = 1;
	XM_InvalidateWindow ();
	XM_UpdateWindow ();


}

// TouchUp
VOID AlertViewOnTouchUp (XMMSG *msg)
{
	int index;
	ALERTVIEWDATA *viewData = (ALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlertView));

	if(viewData == NULL)
		return;
	if(viewData->dwButtonCount == 0)
	{
		// 弹出当前的alert视窗
		XM_PullWindow (0);
		return;
	}
	for (index = 0; index < viewData->dwButtonCount; index ++)
	{
		if(viewData->btnPressed[index])
			break;
	}
	if(index == viewData->dwButtonCount)
		return;
	
	viewData->btnPressed[index] = 0;
	XM_InvalidateWindow ();
	XM_UpdateWindow ();

	XM_PostMessage (XM_COMMAND, (WORD)(VK_AP_MENU + index), index);
}

// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (AlertView)
	XM_ON_MESSAGE (XM_PAINT, AlertViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, AlertViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, AlertViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, AlertViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, AlertViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, AlertViewOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, AlertViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, AlertViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, AlertViewOnTouchUp)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, AlertViewOnSystemEvent)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, LCD_YDOTS - 120, LCD_XDOTS, 120, AlertView, 1, 0, 0, 255, HWND_ALERT)
