//****************************************************************************
//
//	Copyright (C) 2012~2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_switch_button.c
//	  通用SWITCH按钮控件
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"

#define	SWITCHBUTTON_ID		0x54425753

// SWITCH按钮控件
typedef struct _tagXMSWITCHBUTTON {
	DWORD						id;							// "SWBT"
	HANDLE					hWnd;							// switch开关所属窗口
	XMPOINT					ptBtn;						// 开关控件位置
	XMRECT					rcBtn;
	DWORD						dwSwitchPos;				// switch开关位置
	DWORD						dwSwitchState;				// 开关值
	DWORD						dwClickTimeout;			// 按下时间

	DWORD						mBtnWidth;
	DWORD						mMaskWidth;
	float						mBtnOffPos;
	float						mBtnOnPos;
	float						mBtnPos;
	float						mRealPos;

	int						mAnimating;
	float						mAnimatedVelocity;
	float						mAnimationPosition;
	float						mVelocity;

	VOID						*private_data;				// 私有参数
	VOID (*cb_switch_state) (VOID *private_data, unsigned int state);	// 开关状态变化时的回调函数

	XM_CANVAS *				canvas;
	XM_IMAGE *				mask_image;
	XM_IMAGE *				bottom_image;
	XM_IMAGE *				frame_image;
	XM_IMAGE *				btn_pressed_image;
	XM_IMAGE *				btn_unpress_image;

	char *					on_image;
	char *					off_image;

} XMSWITCHBUTTON;


// 初始化一个开关切换控件
HANDLE AP_SwitchButtonControlInit (HANDLE hWnd,			// 开关切换控件所属的窗口句柄
									 XMPOINT *lpButtonPoint,		// 开关切换控件显示的位置
									 XMBOOL bInitialState,		// 开关切换控件的初始状态，0 关闭(禁止) 1 开启(使能)
									 VOID (*lpSwitchStateCallback) (VOID *lpPrivateData, unsigned int state),
																		// 状态切换时调用的回调函数
									 VOID *lpPrivateData)
{
	XMSWITCHBUTTON *lpSwitchButton;
	if(hWnd == NULL || lpButtonPoint == NULL)
		return NULL;

	lpSwitchButton = XM_calloc (sizeof(XMSWITCHBUTTON));
	if(lpSwitchButton)
	{
		lpSwitchButton->id = SWITCHBUTTON_ID;
		lpSwitchButton->hWnd = hWnd;
		lpSwitchButton->ptBtn = *lpButtonPoint;

		lpSwitchButton->dwSwitchState = bInitialState;
		lpSwitchButton->cb_switch_state = lpSwitchStateCallback;
		lpSwitchButton->private_data = lpPrivateData;

		lpSwitchButton->bottom_image = XM_RomImageOpen (
				(ROM_T18_COMMON_SWITCH_BUTTON_BOTTOM_PNG),
				ROM_T18_COMMON_SWITCH_BUTTON_BOTTOM_PNG_SIZE);
		lpSwitchButton->mask_image = XM_RomImageOpen (
				(ROM_T18_COMMON_SWITCH_BUTTON_MASK_PNG),
				ROM_T18_COMMON_SWITCH_BUTTON_MASK_PNG_SIZE);
		lpSwitchButton->btn_pressed_image = XM_RomImageOpen (
				(ROM_T18_COMMON_SWITCH_BUTTON_BTN_PRESSED_PNG),
				ROM_T18_COMMON_SWITCH_BUTTON_BTN_PRESSED_PNG_SIZE
				);
		lpSwitchButton->btn_unpress_image = XM_RomImageOpen (
				(ROM_T18_COMMON_SWITCH_BUTTON_BTN_UNPRESSED_PNG),
				ROM_T18_COMMON_SWITCH_BUTTON_BTN_UNPRESSED_PNG_SIZE
				);
		lpSwitchButton->frame_image = XM_RomImageOpen (
				(ROM_T18_COMMON_SWITCH_BUTTON_FRAME_PNG),
				ROM_T18_COMMON_SWITCH_BUTTON_FRAME_PNG_SIZE
				);
		
		lpSwitchButton->canvas = XM_CanvasCreateFromImage (lpSwitchButton->mask_image);

		// 检查资源是否全部OK
		if(	lpSwitchButton->bottom_image == NULL
			||	lpSwitchButton->mask_image == NULL
			||	lpSwitchButton->btn_pressed_image == NULL
			||	lpSwitchButton->btn_unpress_image == NULL
			||	lpSwitchButton->frame_image == NULL
			||	lpSwitchButton->canvas == NULL)
		{
			if(lpSwitchButton->canvas)
			{
				XM_CanvasDelete (lpSwitchButton->canvas);
				lpSwitchButton->canvas = NULL;
			}
			if(lpSwitchButton->frame_image)
			{
			//	XM_ImageDelete (lpSwitchButton->frame_image);
				lpSwitchButton->frame_image = NULL;
			}
			if(lpSwitchButton->btn_unpress_image)
			{
			//	XM_ImageDelete (lpSwitchButton->btn_unpress_image);
				lpSwitchButton->btn_unpress_image = NULL;
			}
			if(lpSwitchButton->btn_pressed_image)
			{
			//	XM_ImageDelete (lpSwitchButton->btn_pressed_image);
				lpSwitchButton->btn_pressed_image = NULL;
			}
			if(lpSwitchButton->mask_image)
			{
		//		XM_ImageDelete (lpSwitchButton->mask_image);
				lpSwitchButton->mask_image = NULL;
			}
			if(lpSwitchButton->bottom_image)
			{
		//		XM_ImageDelete (lpSwitchButton->bottom_image);
				lpSwitchButton->bottom_image = NULL;
			}
			lpSwitchButton->id = 0;
			XM_free (lpSwitchButton);
			return NULL;
		}

		lpSwitchButton->mBtnWidth = lpSwitchButton->btn_pressed_image->width;
		lpSwitchButton->mMaskWidth = lpSwitchButton->mask_image->width;
		lpSwitchButton->mBtnOnPos = (float)(lpSwitchButton->mBtnWidth / 2.0);
		lpSwitchButton->mBtnOffPos = (float)(lpSwitchButton->mMaskWidth - lpSwitchButton->mBtnWidth / 2.0);

		lpSwitchButton->mBtnPos = lpSwitchButton->dwSwitchState ? lpSwitchButton->mBtnOnPos : lpSwitchButton->mBtnOffPos;

		lpSwitchButton->mRealPos = lpSwitchButton->mBtnPos - lpSwitchButton->mBtnWidth / 2;
		
		lpSwitchButton->mVelocity = (float)((lpSwitchButton->mBtnOnPos - lpSwitchButton->mBtnOffPos) / 5.0);

		lpSwitchButton->rcBtn.left = lpSwitchButton->ptBtn.x;
		lpSwitchButton->rcBtn.top = lpSwitchButton->ptBtn.y;
		lpSwitchButton->rcBtn.right = lpSwitchButton->rcBtn.left + lpSwitchButton->canvas->width - 1;
		lpSwitchButton->rcBtn.bottom = lpSwitchButton->rcBtn.top + lpSwitchButton->canvas->height - 1;
	}
	return lpSwitchButton;
}

// 关闭开关切换控件
XMBOOL AP_SwitchButtonControlExit (HANDLE hSwitchControl)
{
	XMSWITCHBUTTON* lpSwitchButton = (XMSWITCHBUTTON *)hSwitchControl;
	if(lpSwitchButton == NULL || lpSwitchButton->id != SWITCHBUTTON_ID)
	{
		XM_printf ("AP_SwitchButtonControlExit, illeral handle\n");
		return 0;
	}

	if(lpSwitchButton->canvas)
	{
		XM_CanvasDelete (lpSwitchButton->canvas);
		lpSwitchButton->canvas = NULL;
	}
	if(lpSwitchButton->frame_image)
	{
	//	XM_ImageDelete (lpSwitchButton->frame_image);
		lpSwitchButton->frame_image = NULL;
	}
	if(lpSwitchButton->btn_unpress_image)
	{
	//	XM_ImageDelete (lpSwitchButton->btn_unpress_image);
		lpSwitchButton->btn_unpress_image = NULL;
	}
	if(lpSwitchButton->btn_pressed_image)
	{
	//	XM_ImageDelete (lpSwitchButton->btn_pressed_image);
		lpSwitchButton->btn_pressed_image = NULL;
	}
	if(lpSwitchButton->mask_image)
	{
	//	XM_ImageDelete (lpSwitchButton->mask_image);
		lpSwitchButton->mask_image = NULL;
	}
	if(lpSwitchButton->bottom_image)
	{
	//	XM_ImageDelete (lpSwitchButton->bottom_image);
		lpSwitchButton->bottom_image = NULL;
	}

	if(lpSwitchButton->off_image)
		XM_free (lpSwitchButton->off_image);
	if(lpSwitchButton->on_image)
		XM_free (lpSwitchButton->on_image);

	lpSwitchButton->id = 0;
	XM_free (hSwitchControl);
	return 1;
}

unsigned int AP_SwitchButtonControlGetState (HANDLE hSwitchControl)
{
	XMSWITCHBUTTON* lpSwitchButton = (XMSWITCHBUTTON *)hSwitchControl;
	if(lpSwitchButton == NULL || lpSwitchButton->id != SWITCHBUTTON_ID)
	{
		XM_printf ("AP_SwitchButtonControlExit, illeral handle\n");
		return 0;
	}
	return lpSwitchButton->dwSwitchState;
}

XMBOOL AP_SwitchButtonControlSetState (HANDLE hSwitchControl, XMBOOL bState)
{
	XMSWITCHBUTTON* lpSwitchButton = (XMSWITCHBUTTON *)hSwitchControl;
	if(lpSwitchButton == NULL || lpSwitchButton->id != SWITCHBUTTON_ID)
	{
		XM_printf ("AP_SwitchButtonControlExit, illeral handle\n");
		return 0;
	}
	lpSwitchButton->dwSwitchState = bState;
	lpSwitchButton->mBtnPos = lpSwitchButton->dwSwitchState ? lpSwitchButton->mBtnOnPos : lpSwitchButton->mBtnOffPos;

	lpSwitchButton->mRealPos = lpSwitchButton->mBtnPos - lpSwitchButton->mBtnWidth / 2;
	return 1;
}


// 移动开关切换控件
VOID AP_SwitchButtonControlMove (HANDLE hSwitchControl, XMCOORD x, XMCOORD y)
{
	XMSWITCHBUTTON* lpSwitchButton = (XMSWITCHBUTTON *)hSwitchControl;
	if(lpSwitchButton == NULL || lpSwitchButton->id != SWITCHBUTTON_ID)
	{
		XM_printf ("AP_SwitchButtonControlMove, illeral handle\n");
		return;
	}

	lpSwitchButton->ptBtn.x = x;
	lpSwitchButton->ptBtn.y = y;
}


VOID AP_SwitchButtonControlPaint (XMSWITCHBUTTON* lpSwitchButton, xm_osd_framebuffer_t framebuffer, int x, int y)
{
	
	XM_CANVAS *canvas;
	if(lpSwitchButton == NULL || lpSwitchButton->id != SWITCHBUTTON_ID)
	{
		XM_printf ("AP_SwitchButtonControlPaint, illeral handle\n");
		return;
	}

	canvas = lpSwitchButton->canvas;
	if(canvas == NULL)
		return;

	if(lpSwitchButton->dwSwitchState && lpSwitchButton->on_image)
	{
		XM_lcdc_copy_raw_image (
				framebuffer->address,
				framebuffer->format,
				framebuffer->width, framebuffer->height,
				framebuffer->stride,
				x, y,
				(unsigned char *)lpSwitchButton->on_image,
				canvas->width, canvas->height,
				canvas->stride,
				0, 0,
				canvas->width,
				canvas->height,
				XM_GetWindowAlpha (lpSwitchButton->hWnd)
				);
		return;
	}
	else if(!lpSwitchButton->dwSwitchState && lpSwitchButton->off_image)
	{
		XM_lcdc_copy_raw_image (
				framebuffer->address,
				framebuffer->format,
				framebuffer->width, framebuffer->height,
				framebuffer->stride,
				x, y,
				(unsigned char *)lpSwitchButton->off_image,
				canvas->width, canvas->height,
				canvas->stride,
				0, 0,
				canvas->width,
				canvas->height,
				XM_GetWindowAlpha (lpSwitchButton->hWnd)
				);
		return;

	}

	XM_CanvasSetXferMode (canvas, XM_CANVAS_XFERMODE_DST);
	XM_CanvasDrawImage (canvas, NULL, 0, 0);
	XM_CanvasSetXferMode (canvas, XM_CANVAS_XFERMODE_SRC_IN);

	lpSwitchButton->bottom_image = XM_RomImageOpen (
				(ROM_T18_COMMON_SWITCH_BUTTON_BOTTOM_PNG),
				ROM_T18_COMMON_SWITCH_BUTTON_BOTTOM_PNG_SIZE);
	XM_CanvasDrawImage (canvas, lpSwitchButton->bottom_image, (int)lpSwitchButton->mRealPos, 0);
	XM_CanvasSetXferMode (canvas, XM_CANVAS_XFERMODE_SRC_OVER);

	lpSwitchButton->frame_image = XM_RomImageOpen (
				(ROM_T18_COMMON_SWITCH_BUTTON_FRAME_PNG),
				ROM_T18_COMMON_SWITCH_BUTTON_FRAME_PNG_SIZE
				);
	XM_CanvasDrawImage (canvas, lpSwitchButton->frame_image, 0, 0);

	if(lpSwitchButton->dwSwitchState)
	{
		lpSwitchButton->btn_pressed_image = XM_RomImageOpen (
				(ROM_T18_COMMON_SWITCH_BUTTON_BTN_PRESSED_PNG),
				ROM_T18_COMMON_SWITCH_BUTTON_BTN_PRESSED_PNG_SIZE
				);
		XM_CanvasDrawImage (canvas, lpSwitchButton->btn_pressed_image, (int)lpSwitchButton->mRealPos, 0);
		if(lpSwitchButton->on_image == NULL)
		{
			lpSwitchButton->on_image = XM_malloc (canvas->stride * canvas->height);
			if(lpSwitchButton->on_image)
				memcpy (lpSwitchButton->on_image, canvas->image, canvas->stride * canvas->height);
		}
	}
	else
	{
		lpSwitchButton->btn_unpress_image = XM_RomImageOpen (
				(ROM_T18_COMMON_SWITCH_BUTTON_BTN_UNPRESSED_PNG),
				ROM_T18_COMMON_SWITCH_BUTTON_BTN_UNPRESSED_PNG_SIZE
				);
		XM_CanvasDrawImage (canvas, lpSwitchButton->btn_unpress_image, (int)lpSwitchButton->mRealPos, 0);
		if(lpSwitchButton->off_image == NULL)
		{
			lpSwitchButton->off_image = XM_malloc (canvas->stride * canvas->height);
			if(lpSwitchButton->off_image)
				memcpy (lpSwitchButton->off_image, canvas->image, canvas->stride * canvas->height);
		}
	}

	XM_lcdc_copy_raw_image (
				framebuffer->address,
				framebuffer->format,
				framebuffer->width, framebuffer->height,
				framebuffer->stride,
				x, y,
				(unsigned char *)canvas->image,
				canvas->width, canvas->height,
				canvas->stride,
				0, 0,
				canvas->width,
				canvas->height,
				XM_GetWindowAlpha (lpSwitchButton->hWnd)
				);

}

void AP_SwitchButtonControlStartAnimation (XMSWITCHBUTTON *lpSwitchButton, unsigned int turnOn)
{
	lpSwitchButton->mAnimating = 1;
	lpSwitchButton->mAnimatedVelocity = turnOn ? lpSwitchButton->mVelocity : -lpSwitchButton->mVelocity;

	if(lpSwitchButton->dwSwitchState)
		lpSwitchButton->mAnimationPosition = lpSwitchButton->mBtnOnPos;
	else
		lpSwitchButton->mAnimationPosition = lpSwitchButton->mBtnOffPos;
	//lpSwitchButton->mAnimationPosition = (float)lpSwitchButton->mBtnPos;
}

void AP_SwitchButtonControlStopAnimation (XMSWITCHBUTTON *lpSwitchButton) 
{
	lpSwitchButton->mAnimating = 0;
}

#if 0
void AP_SwitchButtonControlRunAnimation (XMSWITCHBUTTON *lpSwitchButton)
{
	xm_osd_framebuffer_t framebuffer;
	//int osd_x, osd_y;
	XMHWND *lpWnd;

	int loop = 5;
	while(loop > 0)
	{
		lpSwitchButton->mAnimationPosition += lpSwitchButton->mAnimatedVelocity;
		if (lpSwitchButton->mAnimationPosition <= lpSwitchButton->mBtnOffPos) {
			AP_SwitchButtonControlStopAnimation (lpSwitchButton);
			lpSwitchButton->mBtnPos = lpSwitchButton->mBtnOffPos;
			lpSwitchButton->mRealPos = (float)(lpSwitchButton->mBtnPos - lpSwitchButton->mBtnWidth / 2.0);
		} 
		else if (lpSwitchButton->mAnimationPosition >= lpSwitchButton->mBtnOnPos) {
			AP_SwitchButtonControlStopAnimation (lpSwitchButton);
			lpSwitchButton->mAnimationPosition = (float)lpSwitchButton->mBtnOnPos;
			lpSwitchButton->mBtnPos = lpSwitchButton->mBtnOnPos;
			lpSwitchButton->mRealPos = (float)(lpSwitchButton->mBtnPos - lpSwitchButton->mBtnWidth / 2.0);
		}
		else
		{
			lpSwitchButton->mBtnPos = lpSwitchButton->mAnimationPosition;
			lpSwitchButton->mRealPos = lpSwitchButton->mBtnPos - (float)(lpSwitchButton->mBtnWidth / 2.0);
		}


		// 此处尚未完善
		// 1)	创建一个与视窗关联的framebuffer对象，XM_osd_framebuffer_create
		// 将坐标转换为LCD显示坐标
		lpWnd = (XMHWND *)lpSwitchButton->hWnd;

		/*
		framebuffer = XM_osd_framebuffer_create (
									XM_LCDC_CHANNEL_0, 
									XM_OSD_LAYER_2,	// ALERTVIEW位于OSD 2层
									XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),	// 获取UI层的显示格式
									//lpWnd->view_cx,
									//lpWnd->view_cy,
									lpSwitchButton->canvas->width,
									lpSwitchButton->canvas->height,
									lpSwitchButton->hWnd,
									0			// 复制最近一次相同视窗的framebuffer的视频缓冲区数据,
									0
									);
		osd_x = (HW_lcdc_get_xdots(XM_LCDC_CHANNEL_0)  - lpWnd->view_cx)/2;
		osd_y = (HW_lcdc_get_ydots(XM_LCDC_CHANNEL_0)  - lpWnd->view_cy)/2;
		osd_x = XM_lcdc_osd_horz_align (XM_LCDC_CHANNEL_0, osd_x);
		osd_y = XM_lcdc_osd_vert_align (XM_LCDC_CHANNEL_0, osd_y);

		framebuffer->offset_x = osd_x + lpSwitchButton->ptBtn.x;
		framebuffer->offset_y = osd_y + lpSwitchButton->ptBtn.y;
		*/

		framebuffer = XM_osd_framebuffer_open (
									XM_LCDC_CHANNEL_0,
									XM_OSD_LAYER_1,
									XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),	// 获取UI层的显示格式
									lpWnd->view_cx,
									lpWnd->view_cy);


	//	framebuffer->offset_x = lpSwitchButton->ptBtn.x;
	//	framebuffer->offset_y = lpSwitchButton->ptBtn.y;
		AP_SwitchButtonControlPaint (lpSwitchButton, framebuffer, lpSwitchButton->ptBtn.x, lpSwitchButton->ptBtn.y);

	//	AP_SwitchButtonControlPaint (lpSwitchButton, framebuffer, 0, 0);

		XM_osd_framebuffer_close (framebuffer, 0);

		loop --;
		XM_Sleep (10);
	}

	lpSwitchButton->dwSwitchState = !lpSwitchButton->dwSwitchState;
}
#endif

// 开关切换控件消息处理
VOID AP_SwitchButtonControlMessageHandler (HANDLE hSwitchControl, XMMSG *msg)
{
	xm_osd_framebuffer_t framebuffer;
	XMSWITCHBUTTON* lpSwitchButton = (XMSWITCHBUTTON *)hSwitchControl;
	if(lpSwitchButton == NULL || lpSwitchButton->id != SWITCHBUTTON_ID)
	{
		XM_printf ("AP_SwitchButtonControlMessageHandler, illeral handle\n");
		return;
	}

	switch (msg->message)
	{
		case XM_PAINT:
			framebuffer = XM_GetWindowFrameBuffer (lpSwitchButton->hWnd);
			if(framebuffer == NULL)
				return;
			AP_SwitchButtonControlPaint (lpSwitchButton, framebuffer, lpSwitchButton->ptBtn.x, lpSwitchButton->ptBtn.y);
			break;

		case XM_KEYDOWN:
			if(msg->wp == VK_AP_MENU)
			{
			//	AP_SwitchButtonControlStartAnimation (lpSwitchButton, !lpSwitchButton->dwSwitchState);
			//	AP_SwitchButtonControlRunAnimation (lpSwitchButton);
			//	return;
				
				if(lpSwitchButton->dwSwitchState)
					lpSwitchButton->dwSwitchState = 0;
				else
					lpSwitchButton->dwSwitchState = 1;
				
				lpSwitchButton->mBtnPos = lpSwitchButton->dwSwitchState ? lpSwitchButton->mBtnOnPos : lpSwitchButton->mBtnOffPos;

				lpSwitchButton->mRealPos = lpSwitchButton->mBtnPos - lpSwitchButton->mBtnWidth / 2;

				if(lpSwitchButton->cb_switch_state)
				{
					(*lpSwitchButton->cb_switch_state) (lpSwitchButton->private_data, lpSwitchButton->dwSwitchState);
				}

				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			break;
	}

}



