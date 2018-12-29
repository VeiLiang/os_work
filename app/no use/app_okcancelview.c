//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_okcancel_view.c
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

#define	OKCANCEL_MAX_BUTTON	2		// 最多2个按键菜单		

#define	OKCANCEL_V_SPACE		15		// 垂直方向边距
#define	OKCANCEL_H_SPACE		10		// 水平方向边距

#define	OKCANCEL_BACKGROUND_COLOR		0xFFFFFFFF

typedef struct tagOKCANCELVIEWDATA {
	DWORD				dwTextInfoID;					// 文本信息资源ID
	DWORD				dwImageID;						// 图片信息资源ID
	float				fAutoCloseTime;				// 视窗定时器自动关闭时间。0表示禁止自动关闭功能
															//		自动关闭默认选择 XM_COMMAND_OK
	DWORD				dwButtonCount;					// 按钮个数
	DWORD				dwButtonNormalID[OKCANCEL_MAX_BUTTON];		//	按钮资源ID
	DWORD				dwButtonPressedID[OKCANCEL_MAX_BUTTON];	//	按钮按下资源ID
	float				fViewAlpha;						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
	DWORD				dwAlignOption;					//	视图的显示对齐设置信息
	DWORD 				dwOption;

	DWORD				dwBkgColor;						// 背景色

	FPOKCANCELCB		okcancelcb;						// 用户提供的按键菜单回调函数

	// 内部状态
	XMSIZE				sizeTextInfo;	
	XMSIZE				sizeImage;	
	XMSIZE				sizeButton[OKCANCEL_MAX_BUTTON];
	XMSIZE				sizeClient;
	DWORD				dwTimeout;						// 计数器倒计时计数
	int					btnPressed[OKCANCEL_MAX_BUTTON];
} OKCANCELVIEWDATA;

// OKCANCEL信息显示视窗
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
//		按钮 OK
//		空白区
//		按钮 Cancel (若存在)
//		空白区
//

// 显示信息视图
// 0 视图参数错误，视图显示失败
// 1 视图创建并显示
XMBOOL XM_OpenOkCancelView ( 
								 DWORD dwInfoTextID,				// 信息文本资源ID
																		//		非0值，指定显示文字信息的资源ID
								 DWORD dwImageID,					// 图片信息资源ID
																		//		非0值，指定显示图片信息的资源ID	

								 DWORD dwButtonCount,			// 按钮个数，1或者2
																		//		当超过一个按钮时，
																		//		OK按钮，使用VK_F1(Menu)
																		//		Cancel按键，使用VK_F2(Mode)
								 DWORD dwButtonNormalID[],		//	按钮资源ID
																		//		0 
																		//			表示没有定义Button，OkCancel仅为一个信息提示窗口。
																		//			按任意键或定时器自动关闭
																		//		其他值
																		//			表示Button文字信息的资源ID，OkCancel需要显示一个按钮
								 DWORD dwButtonPressedID[],	//	按钮按下资源ID
																		//		0
																		//			表示没有定义该资源。
																		//			按钮按下时使用dwButtonNormalID资源
																		//			若没有提供按钮按下效果的图片，调节图片亮度（变暗）来生成按下效果
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
								 FPOKCANCELCB okcancelcb,				// 按键回调函数

								 DWORD dwAlignOption,			//	视图的显示对齐设置信息
																		//		相对OSD显示的原点(OSD有效区域的左上角)
																		//		XM_VIEW_ALIGN_CENTRE	
																		//			视窗居中对齐(相对OSD显示区域)
																		//		XM_VIEW_ALIGN_BOTTOM
																		//			视窗底部居中对齐(相对OSD显示区域)
								 DWORD dwOption					// OKCANCEL视图的控制选项
																		//		XM_OKCANCEL_OPTION_ENABLE_POPTOPVIEW
																		//			弹出栈顶视窗使能
								 )
{
	OKCANCELVIEWDATA *viewData;

	// 参数检查

	// 按键个数必须为1或者2
	if(dwButtonCount != 1 && dwButtonCount != 2)
	{
		XM_printf ("XM_OpenOkCancelView NG, dwButtonCount(%d) must be 1 or 2\n", dwButtonCount);
		return 0;
	}

	if(dwInfoTextID == 0)
	{
		XM_printf ("XM_OpenOkCancelView NG, dwInfoTextID(%d) must be non-zero\n", dwInfoTextID);
		return 0;
	}
	if(dwAlignOption != XM_VIEW_ALIGN_CENTRE && dwAlignOption != XM_VIEW_ALIGN_BOTTOM)
	{
		XM_printf ("XM_OpenOkCancelView NG, illegal dwAlignOption(%d)\n", dwAlignOption);
		return 0;
	}
	if(fViewAlpha < 0.0 || fViewAlpha > 1.0)
	{
		XM_printf ("XM_OpenOkCancelView NG, illegal fViewAlpha(%f), should be 0.0 ~ 1.0\n", fViewAlpha);
		return 0;
	}
	// 自动关闭时间及按键资源不能同时为0
	if(fAutoCloseTime == 0.0 && dwButtonNormalID == 0)
	{
		XM_printf ("XM_OpenOkCancelView NG, both dwAutoCloseTime & dwButtonNormalID set to 0\n");
		return 0;
	}


	if(dwButtonCount > OKCANCEL_MAX_BUTTON)
	{
		XM_printf ("XM_OpenOkCancelView NG, dwButtonCount(%d) exceed maximum button count(%d)\n", 
						dwButtonCount,
						OKCANCEL_MAX_BUTTON);
		return 0;
	}
	else if(dwButtonCount)
	{
		unsigned int i;
		if(dwButtonNormalID == NULL)
		{
			XM_printf ("XM_OpenOkCancelView NG, dwButtonNormalID is NULL\n");
			return 0;
		}
		for (i = 0; i < dwButtonCount; i++)
		{
			if(dwButtonNormalID[i] == 0)
			{
				XM_printf ("XM_OpenOkCancelView NG, dwButtonNormalID[%d] is NULL\n", i);
				return 0;
			}
		}
	}
	
	viewData = XM_calloc (sizeof(OKCANCELVIEWDATA));
	if(viewData == NULL)
		return 0;
	viewData->dwTextInfoID = dwInfoTextID;
	viewData->dwImageID = dwImageID;
	viewData->fAutoCloseTime = fAutoCloseTime;
	viewData->dwButtonCount = dwButtonCount;
	memcpy (viewData->dwButtonNormalID, dwButtonNormalID, dwButtonCount * sizeof(DWORD));
	if(dwButtonPressedID)
	{
		memcpy (viewData->dwButtonPressedID, dwButtonPressedID, dwButtonCount * sizeof(DWORD));
	}
	viewData->dwBkgColor = dwBackgroundColor;
	viewData->fViewAlpha = fViewAlpha;
	viewData->okcancelcb = okcancelcb;
	viewData->dwAlignOption = dwAlignOption;
	viewData->dwOption = dwOption;
	memset (viewData->btnPressed, 0, sizeof(viewData->btnPressed));

	if(dwOption & XM_OKCANCEL_OPTION_ENABLE_POPTOPVIEW)
	{
		// 弹出栈顶视图
		if(XM_JumpWindowEx (XMHWND_HANDLE(OkCancelView), (DWORD)viewData, XM_JUMP_POPTOPVIEW) == 0)
		{
			XM_free (viewData);
			return 0;
		}
	}
	else
	{
		if(XM_PushWindowEx (XMHWND_HANDLE(OkCancelView), (DWORD)viewData) == 0)
		{
			XM_free (viewData);
			return 0;
		}
	}
	return 1;
}

VOID OkCancelViewOnEnter (XMMSG *msg)
{
	OKCANCELVIEWDATA *viewData;
	if(msg->wp == 0)
	{
		int x, y, w, h;
		APPROMRES *AppRes;
		XMSIZE sizeInfo;
		XMSIZE sizeImage;
		XMSIZE sizeButton[OKCANCEL_MAX_BUTTON];
		XMRECT rectDesktop;
		// 第一次进入(窗口准备创建)
		viewData = (OKCANCELVIEWDATA *)msg->lp;

		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(OkCancelView), viewData);

		// 设置视窗的alpha值
		XM_SetWindowAlpha (XMHWND_HANDLE(OkCancelView), (unsigned char)(viewData->fViewAlpha * 255));

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
		// 3) 计算Button的显示区域大小
		if(viewData->dwButtonCount)
		{
			// 需要显示Button
			unsigned int i;
			for (i = 0; i < viewData->dwButtonCount; i ++)
			{
				AppRes = AP_AppRes2RomRes (viewData->dwButtonNormalID[i]);
				if(AppRes)
				{
					XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &sizeButton[i]);
				}
			}
		}

		viewData->sizeTextInfo = sizeInfo;
		viewData->sizeImage = sizeImage;
		memcpy (viewData->sizeButton, sizeButton, sizeof(sizeButton));

		if(viewData->dwAlignOption == XM_VIEW_ALIGN_CENTRE)
		{
			// view垂直居中/水平居中显示 XM_VIEW_ALIGN_CENTRE
			h = OKCANCEL_V_SPACE;
			w = 0;
			// 累计“文本信息内容”
			h += sizeInfo.cy + OKCANCEL_V_SPACE;
			if(w < sizeInfo.cx)
				w = sizeInfo.cx;
			// 累计“图片信息内容”
			h += sizeImage.cy + OKCANCEL_V_SPACE;
			if(w < sizeImage.cx)
				w = sizeImage.cx;
			// 累加 "按钮"
			if(viewData->dwButtonCount)
			{
				unsigned int i;
				for (i = 0; i < viewData->dwButtonCount; i ++)
				{
					h += sizeButton[i].cy + OKCANCEL_V_SPACE;
					if(w < sizeButton[i].cx)
						w = sizeButton[i].cx;
				}
			}
			w += OKCANCEL_H_SPACE + OKCANCEL_H_SPACE;

			/*
			if(viewData->dwButtonNormalID)
			{
				// 显示“信息内容”及“按钮”
				//	自顶而下UI元素构成
				//	(空白区+“信息内容”+ 空白区 + "倒计时" + 空白区“按钮” + 空白区)
				h = OKCANCEL_V_SPACE + sizeInfo.cy + OKCANCEL_V_SPACE + sizeButton.cy + OKCANCEL_V_SPACE;
				w = sizeInfo.cx;
				if(sizeInfo.cx < sizeButton.cx)
					w = sizeButton.cx;
				w += OKCANCEL_H_SPACE + OKCANCEL_H_SPACE;
			}
			else
			{
				// 仅显示文字信息
				//	自顶而下UI元素构成
				// (空白区+“信息内容”+ 空白区)
				h = OKCANCEL_V_SPACE + sizeInfo.cy + OKCANCEL_V_SPACE;
				w = OKCANCEL_H_SPACE + sizeInfo.cx + OKCANCEL_H_SPACE;
			}*/
			
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
			if(viewData->dwButtonNormalID)
			{
				// 显示“信息内容”及“按钮”
				//	自顶而下UI元素构成
				//	(空白区+“信息内容”+ 空白区 + “按钮” + 空白区)
				h = OKCANCEL_V_SPACE + sizeInfo.cy + OKCANCEL_V_SPACE + sizeButton.cy + OKCANCEL_V_SPACE;
			}
			else
			{
				// 仅显示文字信息
				//	自顶而下UI元素构成
				// (空白区+“信息内容”+ 空白区)
				h = OKCANCEL_V_SPACE + sizeInfo.cy + OKCANCEL_V_SPACE;
			}*/

			h = OKCANCEL_V_SPACE;
			w = 0;
			// 累计“文字信息内容”
			h += sizeInfo.cy + OKCANCEL_V_SPACE;
			if(w < sizeInfo.cx)
				w = sizeInfo.cx;
			// 累计“图片信息内容”
			h += sizeImage.cy + OKCANCEL_V_SPACE;
			if(w < sizeImage.cx)
				w = sizeImage.cx;
			// 累加 "按钮"
			if(viewData->dwButtonCount)
			{
				unsigned int i;
				for (i = 0; i < viewData->dwButtonCount; i ++)
				{
					h += sizeButton[i].cy + OKCANCEL_V_SPACE;
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

		viewData->sizeClient.cx = w;
		viewData->sizeClient.cy = h;
			
		//XM_SetWindowPos (XMHWND_HANDLE(OkCancelView), x, y, w, h);
	}
	else
	{
		viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(OkCancelView));
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("OkCancelView Pull\n");
	}

	// 创建定时器
	if(viewData->fAutoCloseTime != 0.0)
	{
		XM_SetTimer (XMTIMER_MESSAGEVIEW, 100);	// 创建100ms的定时器
		viewData->dwTimeout = (DWORD)(viewData->fAutoCloseTime * 10.0 + 0.5);
	}
}

VOID OkCancelViewOnLeave (XMMSG *msg)
{
	OKCANCELVIEWDATA *viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(OkCancelView));
	// 删除定时器
	XM_KillTimer (XMTIMER_MESSAGEVIEW);

	if (msg->wp == 0)
	{
		if(viewData)
		{
			// 释放私有数据句柄
			XM_free (viewData);
			
		}
		XM_printf ("OkCancelView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("OkCancelView Push\n");
	}
}



VOID OkCancelViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
	APPROMRES *AppRes;
	XMCOLOR bkg_clolor;
	BYTE old_alpha;
	HANDLE hwnd = XMHWND_HANDLE(OkCancelView);
	OKCANCELVIEWDATA *viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(viewData == NULL)
		return;

	// 获取视窗位置信息
	XM_GetWindowRect (XMHWND_HANDLE(OkCancelView), &rc);
	if(viewData->dwBkgColor)
		bkg_clolor = viewData->dwBkgColor;
	else
		bkg_clolor = OKCANCEL_BACKGROUND_COLOR;	// 使用与背景刷新相同的颜色及Alpha值
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, bkg_clolor);

	rect = rc;
	rc.top = rc.top + (rc.bottom - rc.top + 1 - viewData->sizeClient.cy) / 2;
	rc.bottom = rect.bottom - (rect.bottom - rect.top + 1 - viewData->sizeClient.cy) / 2;


	// 显示OkCancel信息
	rect = rc;
	rect.bottom = rect.top + OKCANCEL_V_SPACE + viewData->sizeTextInfo.cy + OKCANCEL_V_SPACE - 1 ;
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
		rect = rc;
		rect.top += OKCANCEL_V_SPACE + viewData->sizeTextInfo.cy + OKCANCEL_V_SPACE;
		rect.bottom = rect.top + viewData->sizeImage.cy;
		AppRes = AP_AppRes2RomRes (viewData->dwImageID);
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
	}


	// 按钮显示
	if(viewData->dwButtonCount)
	{
		// 显示按钮背景
		unsigned int i;
		unsigned char layer_brightness;
		old_alpha = XM_GetWindowAlpha (hwnd);
		XM_SetWindowAlpha (hwnd, 255);			
		rect = rc;
		//rect.top = rect.bottom - (OKCANCEL_H_SPACE + viewData->sizeButton[0].cy + OKCANCEL_H_SPACE) + 1;
		//rect.top = rect.bottom - (OKCANCEL_H_SPACE) + 1;
		rect.top = rect.bottom;
		for (i = 0; i < viewData->dwButtonCount; i ++)
		{
			rect.top -= OKCANCEL_V_SPACE + viewData->sizeButton[i].cy;
			//rect.bottom -= OKCANCEL_H_SPACE + viewData->sizeButton[i].cy;
		}
		
		for (i = 0; i < viewData->dwButtonCount; i ++)
		{
			rect.bottom = rect.top + viewData->sizeButton[i].cy - 1;
			if(viewData->btnPressed[i])
			{
				// 显示按下按钮效果
				if(viewData->dwButtonPressedID[i] == 0)
				{
					// 没有提供按钮按下效果的图片，使用调节图片亮度（变暗）来生成按下效果
					AppRes = AP_AppRes2RomRes (viewData->dwButtonNormalID[i]);
					layer_brightness = 128;
				}
				else
				{
					AppRes = AP_AppRes2RomRes (viewData->dwButtonPressedID[i]);
					layer_brightness = 255;
				}
			}
			else
			{
				// 显示释放按钮效果
				AppRes = AP_AppRes2RomRes (viewData->dwButtonNormalID[i]);
				layer_brightness = 255;
			}
			if(AppRes)
			{
				if(layer_brightness == 255)
				{
					XM_RomImageDraw (AppRes->rom_offset, AppRes->res_length,
									hwnd, &rect, XMIMG_DRAW_POS_CENTER);
				}
				else
				{
					float brightness_ratio = (float)layer_brightness / 255.0f;
					XM_IMAGE *image, *dark_image;
					xm_osd_framebuffer_t framebuffer = XM_GetWindowFrameBuffer (hwnd);
					unsigned int x, y;
					if(framebuffer)
					{
						dark_image = NULL;
						image = XM_RomImageCreate (AppRes->rom_offset,
													AppRes->res_length,
													XM_OSD_LAYER_FORMAT_ARGB888);
						if(image)
						{
							dark_image = XM_ImageCloneWithBrightness (image, brightness_ratio);
							XM_ImageDelete (image);
						}
						if(dark_image)
						{
							x = rect.left + (rect.right - rect.left + 1 - dark_image->width) / 2;
							y = rect.top  + (rect.bottom - rect.top + 1 - dark_image->height) / 2;
							XM_lcdc_copy_raw_image (
										framebuffer->address,
										framebuffer->format,
										framebuffer->width, framebuffer->height,
										framebuffer->stride,
										x, y,
										(unsigned char *)dark_image->image,
										dark_image->width, dark_image->height,
										dark_image->stride,
										0, 0,
										dark_image->width,
										dark_image->height,
										XM_GetWindowAlpha (hwnd)
										);
							XM_ImageDelete (dark_image);
						}
					}
				}
			}

			rect.top += OKCANCEL_V_SPACE + viewData->sizeButton[i].cy;
			//rect.bottom += OKCANCEL_H_SPACE + viewData->sizeButton[i].cy;

		}


		XM_SetWindowAlpha (hwnd, old_alpha);

	}
}


VOID OkCancelViewOnKeyDown (XMMSG *msg)
{
	OKCANCELVIEWDATA *viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(OkCancelView));
	if(viewData == NULL)
		return;
// 按键音
	XM_Beep (XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
		case VK_AP_MENU:		// 菜单键
			viewData->btnPressed[0] = 1;
			if(viewData->dwButtonCount > 0)
			{
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			break;

		case VK_AP_MODE:		// 切换到“行车记录”状态
			viewData->btnPressed[1] = 1;
			if(viewData->dwButtonCount > 1)
			{
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			break;

		default:
			// 此处将键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			break;

	}
}

VOID OkCancelViewOnKeyUp (XMMSG *msg)
{
	OKCANCELVIEWDATA *viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(OkCancelView));
	if(viewData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// 菜单键
			if(viewData->dwButtonCount > 0 && viewData->btnPressed[0])
			{
				viewData->btnPressed[0] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();

				XM_PostMessage (XM_COMMAND, XM_COMMAND_OK, 0);
			}
			break;

		case VK_AP_MODE:		// 切换到“行车记录”状态
			if(viewData->dwButtonCount > 1 && viewData->btnPressed[1])
			{
				viewData->btnPressed[1] = 0;
				XM_InvalidateWindow ();
				XM_UpdateWindow ();

				XM_PostMessage (XM_COMMAND, XM_COMMAND_CANCEL, 0);
			}
			break;


		default:
			// 此处将键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			break;
	}
}

VOID OkCancelViewOnCommand (XMMSG *msg)
{
	OKCANCELVIEWDATA *viewData;

	viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(OkCancelView));
	if(viewData == NULL)
		return;

	XM_printf ("OkCancel %d\n", msg->wp);

	// 若存在按键处理回调函数，由回调函数负责处理。
	(*viewData->okcancelcb) (msg->wp);
}

VOID OkCancelViewOnTimer (XMMSG *msg)
{
	OKCANCELVIEWDATA *viewData;
	viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(OkCancelView));
	if(viewData == NULL)
		return;
	if(viewData->dwTimeout > 0)
	{
		viewData->dwTimeout --;
	}
	if(viewData->dwTimeout == 0)
	{
		// 返回到调用者视窗
		(*viewData->okcancelcb) (XM_COMMAND_OK);
	}
}

// 弹出窗口无法处理系统消息事件，退出并交给系统缺省处理
VOID OkCancelViewOnSystemEvent (XMMSG *msg)
{
	// 返回到调用者视窗
	OKCANCELVIEWDATA *viewData;
	viewData = (OKCANCELVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(OkCancelView));
	if(viewData == NULL)
		return;
	if(viewData->dwOption & XM_OKCANCEL_OPTION_SYSTEM_MODEL)
	{
		// 忽略并过滤所有的系统事件(卡拔插/电池等)
		XM_BreakSystemEventDefaultProcess (msg);
		return;
	}
	// 将消息重新投递到系统, 并弹出当前视窗
	XM_BreakSystemEventDefaultProcess (msg);
	XM_PostMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
	XM_PullWindow (0);
}

// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (OkCancelView)
	XM_ON_MESSAGE (XM_PAINT, OkCancelViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, OkCancelViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, OkCancelViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, OkCancelViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, OkCancelViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, OkCancelViewOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, OkCancelViewOnCommand)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, OkCancelViewOnSystemEvent)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, OkCancelView, 1, 0, 0, 255, HWND_VIEW)
