//****************************************************************************
//
//	Copyright (C) 2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: recycle_alert_view.c
//	  循环录制空间信息视窗
//
//	Revision history
//
//		2014.05.15	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"


#define	ALERT_V_SPACE		15		// 垂直方向边距
#define	ALERT_H_SPACE		10		// 水平方向边距

#define	ALERT_BACKGROUND_COLOR		0xFFFFFFFF

typedef struct tagRECYCLEVIDEOALERTVIEWDATA {
	DWORD				dwTextInfoID;					// 文本信息资源ID
	DWORD				dwImageID;						// 图片信息资源ID
	float				fAutoCloseTime;				// 视窗定时器自动关闭时间。0表示禁止自动关闭功能
	float				fViewAlpha;						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
	DWORD				dwAlignOption;					//	视图的显示对齐设置信息
	DWORD				dwOption;						//	RECYCLEVIDEOALERTVIEW视图的控制选项

	DWORD				dwBkgColor;

	// 内部状态
	XMSIZE				sizeTextInfo;	
	XMSIZE				sizeImage;
	
	XMSIZE				sizeSDCARD;						// SD卡容量
	XMSIZE				sizeRECYCLE;					// 循环录像
	XMSIZE				sizeLOCKED;						// 锁定录像
	XMSIZE				sizeOTHERS;						// 其他

	XMSIZE				sizeREGION;						// 包含区域大小

	DWORD				dwTimeout;						// 计数器倒计时计数
	int					btnPressed[4];

} RECYCLEVIDEOALERTVIEWDATA;

// 循环录像报警信息显示视窗
//
//	----- 视图对齐模式支持 ( XM_VIEW_ALIGN_CENTRE 及 XM_VIEW_ALIGN_BOTTOM ) -----
//
//	1) 垂直居中/水平居中显示 XM_VIEW_ALIGN_CENTRE
//	2) 底部对齐/水平居中显示 XM_VIEW_ALIGN_BOTTOM
//
//  ---- UI元素布局(Layout) ----
//
// 1) 显示“文字信息内容”“图片信息内容”
//		空白区
//		文字信息内容
//		空白区
//		图片信息内容
//		空白区
//
// 2) 仅显示“信息内容”
//		空白区
//		信息内容
//		空白区

// 显示信息视图
// 0 视图参数错误，视图显示失败
// 1 视图创建并显示
XMBOOL XM_OpenRecycleVideoAlertView ( 
								 DWORD dwTitleID,					// 标题资源ID

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

								 DWORD dwAlignOption,			//	视图的显示对齐设置信息
																		//		相对OSD显示的原点(OSD有效区域的左上角)
																		//		XM_VIEW_ALIGN_CENTRE	
																		//			视窗居中对齐(相对OSD显示区域)
																		//		XM_VIEW_ALIGN_BOTTOM
																		//			视窗底部居中对齐(相对OSD显示区域)
								 DWORD dwOption					// RECYCLEVIDEOALERTVIEW视图的控制选项
								 )
{
	RECYCLEVIDEOALERTVIEWDATA *viewData;

	if(dwTitleID == 0)
	{
		XM_printf ("XM_OpenRecycleVideoAlertView NG, dwTitleID(%d) must be non-zero\n", dwTitleID);
		return 0;
	}
	if(dwAlignOption != XM_VIEW_ALIGN_CENTRE && dwAlignOption != XM_VIEW_ALIGN_BOTTOM)
	{
		XM_printf ("XM_OpenRecycleVideoAlertView NG, illegal dwAlignOption(%d)\n", dwAlignOption);
		return 0;
	}
	if(fViewAlpha < 0.0 || fViewAlpha > 1.0)
	{
		XM_printf ("XM_OpenRecycleVideoAlertView NG, illegal fViewAlpha(%f), should be 0.0 ~ 1.0\n", fViewAlpha);
		return 0;
	}


	viewData = XM_calloc (sizeof(RECYCLEVIDEOALERTVIEWDATA));
	if(viewData == NULL)
		return 0;
	viewData->dwTextInfoID = dwTitleID;
	viewData->fAutoCloseTime = fAutoCloseTime;
	viewData->dwBkgColor = dwBackgroundColor;
	viewData->fViewAlpha = fViewAlpha;
	viewData->dwAlignOption = dwAlignOption;
	viewData->dwOption = dwOption;
	if(XM_PushWindowEx (XMHWND_HANDLE(RecycleVideoAlertView), (DWORD)viewData) == 0)
	{
		XM_free (viewData);
		return 0;
	}
	return 1;
}

VOID RecycleVideoAlertViewOnEnter (XMMSG *msg)
{
	RECYCLEVIDEOALERTVIEWDATA *viewData;
	if(msg->wp == 0)
	{
		int x, y, w, h;
		APPROMRES *AppRes;
		XMSIZE sizeInfo;
		XMSIZE sizeImage;
		XMRECT rectDesktop;
		// 第一次进入(窗口准备创建)
		viewData = (RECYCLEVIDEOALERTVIEWDATA *)msg->lp;

		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView), viewData);

		// 设置视窗的alpha值
		XM_SetWindowAlpha (XMHWND_HANDLE(RecycleVideoAlertView), (unsigned char)(viewData->fViewAlpha * 255));

		// 计算窗口的位置信息 (根据UI元素大小及对齐模式计算)
		XM_GetDesktopRect (&rectDesktop);
		w = 0;
		h = 0;
		sizeInfo.cx = 0;
		sizeInfo.cy = 0;
		sizeImage.cx = 0;
		sizeImage.cy = 0;
		//	1) 计算信息文本的显示区域大小
		AppRes = AP_AppRes2RomRes (viewData->dwTextInfoID);
		if(AppRes)
		{
			XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &sizeInfo);
		}
		//	2) 计算图片信息的显示区域大小
		if(viewData->dwImageID)
		{
			AppRes = AP_AppRes2RomRes (viewData->dwImageID);
			if(AppRes)
			{
				XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &sizeImage);
			}
		}
		if(viewData->dwOption & AP_RECYCLEVIDEOALERTVIEW_OPTION_GRAPH)
		{
			// 饼图输出
		}
		else
		{
			// 表格输出
			XMSIZE size;
			// SD卡容量
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_SDCARD_SPACE);
			if(AppRes)
			{
				XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &size);
				sizeImage.cy += size.cy + ALERT_V_SPACE;
				viewData->sizeSDCARD = size;

				if(size.cx > viewData->sizeREGION.cx)
					viewData->sizeREGION.cx = size.cx;
				if(size.cy > viewData->sizeREGION.cy)
					viewData->sizeREGION.cy = size.cy;
			}
			// 循环录像
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_RECYCLE_SPACE);
			if(AppRes)
			{
				XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &size);
				sizeImage.cy += size.cy + ALERT_V_SPACE;
				viewData->sizeRECYCLE = size;
				if(size.cx > viewData->sizeREGION.cx)
					viewData->sizeREGION.cx = size.cx;
				if(size.cy > viewData->sizeREGION.cy)
					viewData->sizeREGION.cy = size.cy;
			}
			// 锁定录像
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_LOCKED_SPACE);
			if(AppRes)
			{
				XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &size);
				sizeImage.cy += size.cy + ALERT_V_SPACE;
				viewData->sizeLOCKED = size;
				if(size.cx > viewData->sizeREGION.cx)
					viewData->sizeREGION.cx = size.cx;
				if(size.cy > viewData->sizeREGION.cy)
					viewData->sizeREGION.cy = size.cy;
			}
			// 其他空间
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_OTHERS_SPACE);
			if(AppRes)
			{
				XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &size);
				sizeImage.cy += size.cy + ALERT_V_SPACE;
				viewData->sizeOTHERS = size;
				if(size.cx > viewData->sizeREGION.cx)
					viewData->sizeREGION.cx = size.cx;
				if(size.cy > viewData->sizeREGION.cy)
					viewData->sizeREGION.cy = size.cy;
			}
			
		}

		// 3) 计算Button的显示区域大小

		viewData->sizeTextInfo = sizeInfo;
		viewData->sizeImage = sizeImage;


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

			w += ALERT_H_SPACE + ALERT_H_SPACE;
			
			if(h > (rectDesktop.bottom - rectDesktop.top + 1))
				h = (rectDesktop.bottom - rectDesktop.top + 1);
			if(w > (rectDesktop.right - rectDesktop.left + 1))
				w = (rectDesktop.right - rectDesktop.left + 1);

			x = (rectDesktop.right - rectDesktop.left + 1 - w) / 2;
			y = (rectDesktop.bottom - rectDesktop.top + 1 - h) / 2;
		}
		else //if(viewData->dwAlignOption == XM_VIEW_ALIGN_BOTTOM)
		{

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

			if(h > (rectDesktop.bottom - rectDesktop.top + 1))
				h = (rectDesktop.bottom - rectDesktop.top + 1);

			x = 0;
			w = (rectDesktop.right - rectDesktop.left + 1);
			y = rectDesktop.bottom - h + 1;
		}
			
		XM_SetWindowPos (XMHWND_HANDLE(RecycleVideoAlertView), x, y, w, h);
	}
	else
	{
		viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("RecycleVideoAlertView Pull\n");
	}

	// 创建定时器
	if(viewData->fAutoCloseTime != 0.0)
	{
		XM_SetTimer (XMTIMER_MESSAGEVIEW, 100);	// 创建100ms的定时器
		viewData->dwTimeout = (DWORD)(viewData->fAutoCloseTime * 10.0 + 0.5);
	}
}

VOID RecycleVideoAlertViewOnLeave (XMMSG *msg)
{
	RECYCLEVIDEOALERTVIEWDATA *viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
	// 删除定时器
	XM_KillTimer (XMTIMER_MESSAGEVIEW);

	if (msg->wp == 0)
	{
		if(viewData)
		{
			// 释放私有数据句柄
			XM_free (viewData);
			
		}
		XM_printf ("RecycleVideoAlertView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("RecycleVideoAlertView Push\n");
	}
}

extern void recycle_video_graph_draw (u8_t *framebuf, unsigned int linelength, 
										 float recycle_percent,
										 float locked_percent,
										 float others_percent);

// 此处没有考虑多个卷的情况
// 返回值定义
//	0	已获取卷信息
// -1	无法获取卷信息
int XM_VideoItemGetDiskUsage (XMINT64 *TotalSpace, 		// SD卡空间
										XMINT64 *LockedSpace, 		// 已锁定的文件空间
										XMINT64 *RecycleSpace,		// 可循环使用文件 + 空闲空间
										XMINT64 *OtherSpace			// 其他文件(系统)占用空间
									);



VOID RecycleVideoAlertViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
	APPROMRES *AppRes;
	XMCOLOR bkg_clolor;
	BYTE old_alpha;
	HANDLE hwnd = XMHWND_HANDLE(RecycleVideoAlertView);
	RECYCLEVIDEOALERTVIEWDATA *viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(viewData == NULL)
		return;

	// 获取视窗位置信息
	XM_GetWindowRect (hwnd, &rc);
	if(viewData->dwBkgColor)
		bkg_clolor = viewData->dwBkgColor;
	else
		bkg_clolor = ALERT_BACKGROUND_COLOR;	// 使用与背景刷新相同的颜色及Alpha值
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, bkg_clolor);

	// 显示RecycleVideoAlert信息
	rect = rc;
	rect.bottom = rect.top + ALERT_V_SPACE + viewData->sizeTextInfo.cy + ALERT_V_SPACE - 1 ;
	AppRes = AP_AppRes2RomRes (viewData->dwTextInfoID);
	if(AppRes)
	{
		// 区域居中显示
		old_alpha = XM_GetWindowAlpha (hwnd);
		XM_SetWindowAlpha (hwnd, 255);

		XM_RomImageDraw (AppRes->rom_offset, 
							AppRes->res_length,
							hwnd, &rect, XMIMG_DRAW_POS_CENTER);
		XM_SetWindowAlpha (hwnd, old_alpha);
	}

	// 显示图片信息
	if(viewData->sizeImage.cy)
	{
		// 存在图片信息
		float sdcard_volume;
		float recycle_percent, locked_percent, others_percent;
		XMINT64 TotalSpace, LockedSpace, RecycleSpace, OtherSpace;
		unsigned int total_size, locked_size, recycle_size, other_size;

		int w = 0;
		XMSIZE s;
		char txt[16];

		TotalSpace = 0;
		LockedSpace = 0;
		RecycleSpace = 0;
		OtherSpace = 0;
		XM_VideoItemGetDiskUsage (&TotalSpace, &LockedSpace, &RecycleSpace, &OtherSpace);

		total_size = (unsigned int)(TotalSpace / (1024*1024));
		locked_size = (unsigned int)(LockedSpace / (1024*1024));
		recycle_size = (unsigned int)(RecycleSpace / (1024*1024));
		other_size = (unsigned int)(OtherSpace / (1024*1024));

		sprintf (txt, "128.0GB");
		s.cx = 0;
		AP_TextGetStringSize (txt, strlen(txt), &s);

		// DEMO测试代码
		sdcard_volume = ((float)total_size) / (1024.0f);	
		recycle_percent = ((float)recycle_size) / (total_size ? total_size : 1);
		locked_percent = ((float)locked_size) / (total_size ? total_size : 1);
		others_percent = ((float)other_size) / (total_size ? total_size : 1);



		w = ALERT_H_SPACE + 120 + 2 * ALERT_H_SPACE + viewData->sizeREGION.cx + ALERT_H_SPACE + s.cx + ALERT_H_SPACE;
		
		w = (rect.right - rect.left + 1 - w) / 2;
		if(w < 0)
		{
			XM_printf ("RECYCLE VIEW NG, illegal width (%d)\n", w);
			w = 0;
		}


		rect = rc;
		rect.top += ALERT_V_SPACE + viewData->sizeTextInfo.cy + ALERT_V_SPACE;
		rect.left += ALERT_H_SPACE + w+100;

		{
			// SD卡容量
			float space;
			char  buffer[16];
			unsigned int cx = viewData->sizeREGION.cx + ALERT_H_SPACE;
			old_alpha = XM_GetWindowAlpha (hwnd);
			XM_SetWindowAlpha (hwnd, 255);			
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_SDCARD_SPACE);
			if(AppRes)
			{
				// 区域居中显示
				XM_RomImageDraw (AppRes->rom_offset, 
										AppRes->res_length,
										hwnd, &rect, XMIMG_DRAW_POS_LEFTTOP);
				space = sdcard_volume;
				sprintf (buffer, "%.1f GB", space);
				AP_TextOutWhiteDataTimeString (hwnd, rect.left + cx, rect.top, buffer, strlen(buffer));

				rect.top += viewData->sizeSDCARD.cy + ALERT_V_SPACE;
			}

			// 循环录像
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_RECYCLE_SPACE);
			if(AppRes)
			{
				// 区域左上角显示
				XM_FillRect (hwnd, rect.left, rect.top, 
					rect.left + viewData->sizeRECYCLE.cx - 1,
					rect.top + viewData->sizeRECYCLE.cy - 1, XM_ARGB(0xFF, 0x00, 0x80, 0x00) );

				XM_RomImageDraw (AppRes->rom_offset, 
										AppRes->res_length,
										hwnd, &rect, XMIMG_DRAW_POS_LEFTTOP);

				space = sdcard_volume * recycle_percent;
				sprintf (buffer, "%.1f GB", space);
				AP_TextOutWhiteDataTimeString (hwnd, rect.left + cx, rect.top, buffer, strlen(buffer));

				rect.top += viewData->sizeRECYCLE.cy + ALERT_V_SPACE;
			}

			// 锁定录像
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_LOCKED_SPACE);
			if(AppRes)
			{
				XM_FillRect (hwnd, rect.left, rect.top, 
					rect.left + viewData->sizeRECYCLE.cx - 1,
					rect.top + viewData->sizeRECYCLE.cy - 1, XM_ARGB(0xFF, 0x80, 0x00, 0x00) );
				// 区域左上角显示
				XM_RomImageDraw (AppRes->rom_offset, 
										AppRes->res_length,
										hwnd, &rect, XMIMG_DRAW_POS_LEFTTOP);
				space = sdcard_volume * locked_percent;
				sprintf (buffer, "%.1f GB", space);
				AP_TextOutWhiteDataTimeString (hwnd, rect.left + cx, rect.top, buffer, strlen(buffer));

				rect.top += viewData->sizeLOCKED.cy + ALERT_V_SPACE;
			}
			// 其他
			AppRes = AP_AppRes2RomRes (AP_ID_RECYCLEVIDEO_OTHERS_SPACE);
			if(AppRes)
			{
				// 区域居中显示
				XM_FillRect (hwnd, rect.left, rect.top, 
					rect.left + viewData->sizeRECYCLE.cx - 1,
					rect.top + viewData->sizeRECYCLE.cy - 1, XM_ARGB(0xFF, 0x00, 0x00, 0x80) );

				XM_RomImageDraw (AppRes->rom_offset, 
										AppRes->res_length,
										hwnd, &rect, XMIMG_DRAW_POS_LEFTTOP);
				space = sdcard_volume * others_percent;
				sprintf (buffer, "%.1f GB", space);
				AP_TextOutWhiteDataTimeString (hwnd, rect.left + cx, rect.top, buffer, strlen(buffer));

				rect.top += viewData->sizeOTHERS.cy + ALERT_V_SPACE;
			}
			
			XM_SetWindowAlpha (hwnd, old_alpha);
		}

	}


}


VOID RecycleVideoAlertViewOnKeyDown (XMMSG *msg)
{
	RECYCLEVIDEOALERTVIEWDATA *viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
	if(viewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// 菜单键
			viewData->btnPressed[0] = 1;
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_MODE:		// 切换到“行车记录”状态
			viewData->btnPressed[1] = 1;
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_SWITCH:
			viewData->btnPressed[2] = 1;
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		default:
			// 此处将键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			break;

	}
}

VOID RecycleVideoAlertViewOnKeyUp (XMMSG *msg)
{
	RECYCLEVIDEOALERTVIEWDATA *viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
	if(viewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// 菜单键
			if(viewData->btnPressed[0] == 1)
			{
				viewData->btnPressed[0] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
				XM_PostMessage (XM_COMMAND, msg->wp, 0);
			}
			break;

		case VK_AP_MODE:		// 切换到“行车记录”状态
			if(viewData->btnPressed[1] == 1)
			{
				viewData->btnPressed[1] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
				XM_PostMessage (XM_COMMAND, msg->wp, 0);
			}
			break;

		case VK_AP_SWITCH:
			if(viewData->btnPressed[2] == 1)
			{
				viewData->btnPressed[2] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
				XM_PostMessage (XM_COMMAND, msg->wp, 0);
			}
			break;

		default:
			// 此处将键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			break;
	}
}

VOID RecycleVideoAlertViewOnCommand (XMMSG *msg)
{
	RECYCLEVIDEOALERTVIEWDATA *viewData;

	viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
	if(viewData == NULL)
		return;

	XM_printf ("RecycleVideoAlert %d\n", msg->wp);

	// 若存在按键处理回调函数，由回调函数负责处理。
		// 返回到调用者窗口
		XM_PullWindow (0);

}

VOID RecycleVideoAlertViewOnTimer (XMMSG *msg)
{
	RECYCLEVIDEOALERTVIEWDATA *viewData;
	viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
	if(viewData == NULL)
		return;
	if(viewData->dwTimeout > 0)
	{
		viewData->dwTimeout --;
	}

	//XM_InvalidateWindow ();

	if(viewData->dwTimeout == 0)
	{
		// 返回到调用者视窗
		XM_PullWindow (0);
	}
}

// 弹出窗口无法处理系统消息事件，退出并交给系统缺省处理
VOID RecycleVideoAlertViewOnSystemEvent (XMMSG *msg)
{
	// 返回到调用者视窗
	XM_BreakSystemEventDefaultProcess (msg);
	XM_PostMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
	XM_PullWindow (0);
}
VOID RecycleVideoAlertViewOnTouchDown (XMMSG *msg)
{
         RECYCLEVIDEOALERTVIEWDATA *viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
	    if(viewData == NULL)
		return;
        	viewData->btnPressed[0] = 1;
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
	

}

VOID RecycleVideoAlertViewOnTouchUp (XMMSG *msg)
{
    RECYCLEVIDEOALERTVIEWDATA *viewData = (RECYCLEVIDEOALERTVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(RecycleVideoAlertView));
	if(viewData == NULL)
		return;

		if(viewData->btnPressed[0] == 1)
			{
				viewData->btnPressed[0] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
				XM_PostMessage (XM_COMMAND, VK_AP_MENU, 0);
			}
       
}
// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (RecycleVideoAlertView)
	XM_ON_MESSAGE (XM_PAINT, RecycleVideoAlertViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, RecycleVideoAlertViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, RecycleVideoAlertViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, RecycleVideoAlertViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, RecycleVideoAlertViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, RecycleVideoAlertViewOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, RecycleVideoAlertViewOnCommand)
	XM_ON_MESSAGE (XM_TOUCHDOWN, RecycleVideoAlertViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, RecycleVideoAlertViewOnTouchUp)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, RecycleVideoAlertViewOnSystemEvent)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, LCD_YDOTS - 120, LCD_XDOTS, 120, RecycleVideoAlertView, 1, 0, 0, 255, HWND_ALERT)
