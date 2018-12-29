//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_button.c
//	  通用按钮控件(最多支持4个按钮)
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuid.h"

#define	XMBTNINIT	0x54

// 按钮控件初始化
XMBOOL AP_ButtonControlInit (XMBUTTONCONTROL *pButtonControl, BYTE bBtnCount, HANDLE hWnd, const XMBUTTONINFO* pButtonInfo)
{
	//APPROMRES *AppRes;
	//int i;
	if(pButtonControl == NULL)
		return 0;
	if(pButtonInfo == NULL)
		return 0;
	if(hWnd == NULL)
		return 0;
	// 暂时只支持1, 2, 3, 4个Button样式
	if(bBtnCount < 1 || bBtnCount > XM_MAX_BUTTON_COUNT)
		return 0;

	pButtonControl->bBtnCount = bBtnCount;
	pButtonControl->hWnd = hWnd;
	pButtonControl->dwBtnBackgroundId = (DWORD)-1;
	//pButtonControl->pBtnInfo = pButtonInfo;
	pButtonControl->dwBtnFlag = 0;

	// 标记按钮控件已初始化
	pButtonControl->bBtnInited = XMBTNINIT;
	pButtonControl->bBtnEnable = 1;
	pButtonControl->bBtnClick = (BYTE)(-1);

	memcpy (pButtonControl->pBtnInfo, pButtonInfo, bBtnCount * sizeof(XMBUTTONINFO));

#if 0
	// 预加载图像
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_MENUITEMSPLITLINE);
	if(AppRes)
		pButtonControl->pImageMenuItemSplitLine = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_BUTTONBACKGROUND);
	if(AppRes)
		pButtonControl->pImageButtonBackground = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_BUTTONSPLITLINE);
	if(AppRes)
		pButtonControl->pImageButtonSplitLine = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);

	for (i = 0; i < pButtonControl->bBtnCount; i++)
	{
		AppRes = AP_AppRes2RomRes (pButtonControl->pBtnInfo[i].dwLogoId);
		if(AppRes)
			pButtonControl->pImageLogo[i] = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
		
		AppRes = AP_AppRes2RomRes (pButtonControl->pBtnInfo[i].dwTextId);
		if(AppRes)
			pButtonControl->pImageText[i] = XM_ImageCreate (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, XM_OSD_LAYER_FORMAT_ARGB888);
	}
#endif

	return 1;
}

// 按钮控件初始化
XMBOOL AP_TpButtonControlInit (TPBUTTONCONTROL *TpButtonControl, BYTE bBtnCount, HANDLE hWnd, const TPBUTTONINFO* pButtonInfo)
{
	//APPROMRES *AppRes;
	//int i;

	if(TpButtonControl == NULL)
		return 0;

	if(pButtonInfo == NULL)
		return 0;

	if(hWnd == NULL)
		return 0;

	// 暂时只支持1, 2, 3, 4个Button样式
	if(bBtnCount < 1 || bBtnCount > TP_MAX_BUTTON_COUNT)
		return 0;

	TpButtonControl->bBtnCount = bBtnCount;
	TpButtonControl->hWnd = hWnd;
	TpButtonControl->dwBtnBackgroundId = (DWORD)-1;
	//TpButtonControl->pBtnInfo = pButtonInfo;
	TpButtonControl->dwBtnFlag = 0;

	// 标记按钮控件已初始化
	TpButtonControl->bBtnInited = XMBTNINIT;
	TpButtonControl->bBtnEnable = 1;
	TpButtonControl->bBtnClick = (BYTE)(-1);

	memcpy (TpButtonControl->TpBtnInfo, pButtonInfo, bBtnCount * sizeof(TPBUTTONINFO));

	return 1;
}


// 按钮控件释放
XMBOOL AP_ButtonControlExit (XMBUTTONCONTROL *TpButtonControl)
{
//	int i;
	if(TpButtonControl == NULL)
		return 0;



	TpButtonControl->bBtnInited = (BYTE)0;	
	TpButtonControl->bBtnEnable = 0;
	return 1;
}
// 按钮控件释放
XMBOOL AP_TpButtonControlExit (TPBUTTONCONTROL *TpButtonControl)
{
	//int i;
	if(TpButtonControl == NULL)
		return 0;



	TpButtonControl->bBtnInited = (BYTE)0;	
	TpButtonControl->bBtnEnable = 0;
	return 1;
}

// 修改单个按钮的设置
// pButtonControl		按钮控件
// bBtnIndex			需要修改的按钮的索引序号 (序号从0开始)
// pButtonInfo			新的按钮的设置信息
XMBOOL AP_ButtonControlModify (XMBUTTONCONTROL *pButtonControl, BYTE bBtnIndex, const XMBUTTONINFO* pButtonInfo)
{
	if(pButtonControl == NULL)
		return FALSE;
	if(bBtnIndex >= pButtonControl->bBtnCount)
		return FALSE;
	if(pButtonInfo == NULL)
		return FALSE;
	memcpy (pButtonControl->pBtnInfo + bBtnIndex, pButtonInfo, sizeof(XMBUTTONINFO));
	return TRUE;
}


// 设置按钮的标志状态, 改变按钮的显示
VOID  AP_ButtonControlSetFlag (XMBUTTONCONTROL *pButtonControl, DWORD Flag)
{
	if(pButtonControl == NULL)
		return;
	pButtonControl->dwBtnFlag = Flag;
}

DWORD AP_ButtonControlGetFlag (XMBUTTONCONTROL *pButtonControl)
{
	if(pButtonControl == NULL)
		return 0;
	return pButtonControl->dwBtnFlag;
}


// 绘制按钮控件
static VOID AP_ButtonPaint (XMBUTTONCONTROL *pButtonControl)
{
	XMRECT rc, rect;
	int i, loop;
	XMCOORD x, y;
	int offset;

	// 隐藏按钮区的显示
	if( (pButtonControl->dwBtnFlag & XMBUTTON_FLAG_HIDE) == XMBUTTON_FLAG_HIDE )
		return;

	XM_GetDesktopRect (&rc);
	rect = rc;

	// 计算相对基准屏(320X240)的偏移
	offset = rc.bottom - 240;

	// --------------------------------------
	//
	// ********* 3 显示按钮区 ***************
	//
	// --------------------------------------
	rect.left = APP_POS_BUTTON_X;
	rect.top = offset + APP_POS_BUTTON_Y;
	// 按钮区顶部绘制一条水平分割线
	if(!(pButtonControl->dwBtnFlag & XMBUTTON_FLAG_HIDE_HORZ_SPLIT))
	{
		AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, pButtonControl->hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	}

	rect.top = (XMCOORD)(rect.top + 1);
	// 填充按钮区背景 
	if(!(pButtonControl->dwBtnFlag & XMBUTTON_FLAG_HIDE_BACKGROUND))
	{
		AP_RomImageDrawByMenuID (AP_ID_COMMON_BUTTONBACKGROUND, pButtonControl->hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	}
	
	// 画按钮区的垂直分割线
	if(pButtonControl->bBtnCount == 1)
	{
		// 1个或2个Button，绘制1条分割线
		loop = 1;
	}
	else
	{
		loop = pButtonControl->bBtnCount - 1;
	}
	if(!(pButtonControl->dwBtnFlag & XMBUTTON_FLAG_HIDE_VERT_SPLIT))
	{
		for (i = 0; i < loop; i ++)
		{
			rect.left = (XMCOORD)( (i + 1) * (rc.right - rc.left + 1) / (loop + 1) - 1);
			rect.top = offset + APP_POS_BUTTON_SPLITLINE_Y;
			AP_RomImageDrawByMenuID (AP_ID_COMMON_BUTTONSPLITLINE, pButtonControl->hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
		}
	}

	#if 0
	// 画按钮的LOGO
	x = (XMCOORD)((rc.right - rc.left + 1) / ((loop+1)*2));
	y = offset + APP_POS_BUTTON_LOGO_Y;

	rect.left = x;
	rect.top = y;
	rect.right = x;
	rect.bottom = y;
	for (i = 0; i < pButtonControl->bBtnCount; i++)
	{
		XMRECT rc_backup = rect;
		
		// 检查按钮是否已按下
		if(pButtonControl->bBtnClick == (BYTE)i)
		{
			XM_OffsetRect (&rect, 5, 5); // 1 1
		}

		AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, pButtonControl->hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		AP_RomImageDrawByMenuID (pButtonControl->pBtnInfo[i].dwLogoId, pButtonControl->hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		
		rect = rc_backup;
		XM_OffsetRect (&rect, (XMCOORD)((rc.right - rc.left + 1) / (loop+1)), 0);
	}
    #else
	// 画按钮的文字
	x = (XMCOORD)((rc.right - rc.left + 1) / ((loop+1)*2));
	y = offset + APP_POS_BUTTON_TEXT_Y;

	rect.left = x;
	rect.top = y-20;
	rect.right = x;
	rect.bottom = y;
	for (i = 0; i < pButtonControl->bBtnCount; i++)
	{
		XMRECT rc_backup = rect;
		
		// 检查按钮是否已按下
		if(pButtonControl->bBtnClick == (BYTE)i)
		{
			XM_OffsetRect (&rect, 5, 5);
		}
		AP_RomImageDrawByMenuID (pButtonControl->pBtnInfo[i].dwTextId, pButtonControl->hWnd, &rect, XMGIF_DRAW_POS_CENTER);

		rect = rc_backup;
		XM_OffsetRect (&rect, (XMCOORD)((rc.right - rc.left + 1) / (loop+1)), 0);
	}
	#endif
}


// 绘制按钮控件
static VOID AP_TpButtonPaint (TPBUTTONCONTROL *TpButtonControl)
{
	XMRECT rc, rect;
	int i, loop;
	XMCOORD x, y;
	int offset;

	// 隐藏按钮区的显示
	if( (TpButtonControl->dwBtnFlag & XMBUTTON_FLAG_HIDE) == XMBUTTON_FLAG_HIDE )
		return;

	XM_GetDesktopRect (&rc);
	rect = rc;

	for (i = 0; i < TpButtonControl->bBtnCount; i++)
	{
		XMRECT rc_backup;
		
		rect.left = TpButtonControl->TpBtnInfo[i].left;
	    rect.top = TpButtonControl->TpBtnInfo[i].top;
	    rect.right = TpButtonControl->TpBtnInfo[i].right;
	    rect.bottom = TpButtonControl->TpBtnInfo[i].bottom;
		rc_backup = rect;
		
		// 检查按钮是否已按下，选中位置稍微移动
		if(TpButtonControl->bBtnClick == (BYTE)i)
		{
			//XM_OffsetRect (&rect, 5, 5);
			AP_RomImageDrawByMenuID (TpButtonControl->TpBtnInfo[i].ClickdwLogoId, TpButtonControl->hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		}
		else
		{
            AP_RomImageDrawByMenuID (TpButtonControl->TpBtnInfo[i].dwLogoId, TpButtonControl->hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		}
		rect = rc_backup;
	}
}

// 直接在新创建的framebuffer上进行输出 (显示的过程不可视)
// 1) 创建并复制最近的framebuffer内容
// 2) 在新创建的framebuffer上绘制按钮区域
// 3) 释放新创建的framebuffer (将被用作下一帧的OSD显示输出)
static void AP_ButtonDirectPaint (XMBUTTONCONTROL *pButtonControl)
{
	XM_OSD_FRAMEBUFFER *framebuffer, *old_framebuffer;
	XMRECT rc, rect;
	int offset;
	XMCOLOR bkg_color = XM_GetSysColor(XM_COLOR_DESKTOP);
	unsigned int old_alpha;
					
	XM_GetDesktopRect (&rc);

	// 以下使用2种不同方法进行直接写屏操作
	//
	// 1) 复制当前正在显示的帧缓存并创建一个新的framebuffer对象，然后在这个新的对象上绘制，
	//		绘制完毕后将新创建的framebuffer对象释放，等待显示使用新的帧内容更新。
	//		更新内容的过程隐藏(用户不可视)
	//
	// 2) 直接使用与当前显示的帧缓存关联的framebuffer对象， 然后在这个对象上绘制，
	//		绘制完毕后将framebuffer对象释放
	//		更新内容的过程可视，可能会产生某些闪烁现象(如区域背景擦除，且擦除的时刻该区域正在显示刷新)
	//
#if 1
	// 1) 创建一个新的framebuffer, 并复制当前显示framebuffer的内容
	framebuffer = XM_osd_framebuffer_create (XM_LCDC_CHANNEL_0,
													XM_OSD_LAYER_1, 
													XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
													rc.right - rc.left + 1,
													rc.bottom - rc.top + 1,
													pButtonControl->hWnd,
													1,			// 复制最近的帧内容(从当前正在显示的framebuffer对象的帧缓存复制)
													0			// 不使用背景色清除帧内容
													);
#else
	// 2) 使用最近当前正在显示的framebuffer
	framebuffer = XM_osd_framebuffer_open (XM_LCDC_CHANNEL_0,
													XM_OSD_LAYER_1, 
													XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
													rc.right - rc.left + 1,
													rc.bottom - rc.top + 1);
#endif

	if(framebuffer)
	{
		// 将窗口的framebuffer设置为当前显示的framebuffer
		old_framebuffer = XM_GetWindowFrameBuffer (pButtonControl->hWnd);
		XM_SetWindowFrameBuffer (pButtonControl->hWnd, framebuffer);

		rect = rc;

		// 计算相对基准屏(320X240)的偏移
		offset = rc.bottom - 240;

		// --------------------------------------
		//
		// ********* 3 显示按钮区 ***************
		//
		// --------------------------------------
		// 刷新按钮所在的背景区
		rect.left = APP_POS_BUTTON_X;
		rect.top = offset + APP_POS_BUTTON_Y;
		XM_FillRect (pButtonControl->hWnd, rect.left, rect.top, rect.right, rect.bottom, bkg_color);

		old_alpha = XM_GetWindowAlpha (pButtonControl->hWnd);
		XM_SetWindowAlpha (pButtonControl->hWnd, 255);

		// 绘制按钮区域
		AP_ButtonPaint (pButtonControl);

		XM_SetWindowAlpha (pButtonControl->hWnd, (unsigned char)old_alpha);

		XM_SetWindowFrameBuffer (pButtonControl->hWnd, old_framebuffer);

		// 释放framebuffer，等待显示刷新
		XM_osd_framebuffer_close (framebuffer, 0);
	}
	else
	{
		// 使用全窗口刷新模式
		XM_InvalidateWindow ();
		XM_UpdateWindow ();
	}
}

// 直接在新创建的framebuffer上进行输出 (显示的过程不可视)
// 1) 创建并复制最近的framebuffer内容
// 2) 在新创建的framebuffer上绘制按钮区域
// 3) 释放新创建的framebuffer (将被用作下一帧的OSD显示输出)
static void AP_TpButtonDirectPaint (TPBUTTONCONTROL *TpButtonControl)
{
	XM_OSD_FRAMEBUFFER *framebuffer, *old_framebuffer;
	XMRECT rc, rect;
	int offset;
	XMCOLOR bkg_color = XM_GetSysColor(XM_COLOR_DESKTOP);
	unsigned int old_alpha;
					
	XM_GetDesktopRect (&rc);

	// 以下使用2种不同方法进行直接写屏操作
	//
	// 1) 复制当前正在显示的帧缓存并创建一个新的framebuffer对象，然后在这个新的对象上绘制，
	//		绘制完毕后将新创建的framebuffer对象释放，等待显示使用新的帧内容更新。
	//		更新内容的过程隐藏(用户不可视)
	//
	// 2) 直接使用与当前显示的帧缓存关联的framebuffer对象， 然后在这个对象上绘制，
	//		绘制完毕后将framebuffer对象释放
	//		更新内容的过程可视，可能会产生某些闪烁现象(如区域背景擦除，且擦除的时刻该区域正在显示刷新)
	//
#if 1
	// 1) 创建一个新的framebuffer, 并复制当前显示framebuffer的内容
	framebuffer = XM_osd_framebuffer_create (XM_LCDC_CHANNEL_0,
													XM_OSD_LAYER_1, 
													XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
													rc.right - rc.left + 1,
													rc.bottom - rc.top + 1,
													TpButtonControl->hWnd,
													1,			// 复制最近的帧内容(从当前正在显示的framebuffer对象的帧缓存复制)
													0			// 不使用背景色清除帧内容
													);
#else
	// 2) 使用最近当前正在显示的framebuffer
	framebuffer = XM_osd_framebuffer_open (XM_LCDC_CHANNEL_0,
													XM_OSD_LAYER_1, 
													XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
													rc.right - rc.left + 1,
													rc.bottom - rc.top + 1);
#endif

	if(framebuffer)
	{
		// 将窗口的framebuffer设置为当前显示的framebuffer
		old_framebuffer = XM_GetWindowFrameBuffer (TpButtonControl->hWnd);
		XM_SetWindowFrameBuffer (TpButtonControl->hWnd, framebuffer);

            #if 0
		rect = rc;

		// 计算相对基准屏(320X240)的偏移
		offset = rc.bottom - 240;

		// --------------------------------------
		//
		// ********* 3 显示按钮区 ***************
		//
		// --------------------------------------
		// 刷新按钮所在的背景区
		rect.left = APP_POS_BUTTON_X;
		rect.top = offset + APP_POS_BUTTON_Y;
		#endif
		//XM_FillRect (pButtonControl->hWnd,pButtonControl->pBtnInfo[pButtonControl->bBtnClick].left,pButtonControl->pBtnInfo[pButtonControl->bBtnClick].top, pButtonControl->pBtnInfo[pButtonControl->bBtnClick].right,pButtonControl->pBtnInfo[pButtonControl->bBtnClick].bottom, bkg_color);

		old_alpha = XM_GetWindowAlpha (TpButtonControl->hWnd);
		XM_SetWindowAlpha (TpButtonControl->hWnd, 255);

		// 绘制按钮区域
		AP_TpButtonPaint (TpButtonControl);

		XM_SetWindowAlpha (TpButtonControl->hWnd, (unsigned char)old_alpha);

		XM_SetWindowFrameBuffer (TpButtonControl->hWnd, old_framebuffer);

		// 释放framebuffer，等待显示刷新
		XM_osd_framebuffer_close (framebuffer, 0);
	}
	else
	{
		// 使用全窗口刷新模式
		XM_InvalidateWindow ();
		XM_UpdateWindow ();
	}
}

static int testBtnKey (XMBUTTONCONTROL *pButtonControl, WORD key)
{
	int i;
	for (i = 0; i < pButtonControl->bBtnCount; i++)
	{
		if(pButtonControl->pBtnInfo[i].wKey == key)
			return i;
	}
	return -1;
}
static int testTpBtnKey (TPBUTTONCONTROL *TpButtonControl, WORD key)
{
	int i;
	for (i = 0; i < TpButtonControl->bBtnCount; i++)
	{
		if(TpButtonControl->TpBtnInfo[i].wKey == key)
			return i;
	}
	return -1;
}
// 使能或禁止按钮
XMBOOL AP_ButtonControlSetEnable (XMBUTTONCONTROL *pButtonControl, XMBOOL bEnable)
{
	// 检查按钮控件是否已初始化
	if(pButtonControl->bBtnInited != XMBTNINIT)
		return 0;
	pButtonControl->bBtnEnable = bEnable;
	return 1;
}

#define TP_LEFT     		500
#define TP_WIDTH           600

static int check_button_clicked (XMBUTTONCONTROL *pButtonControl, XMMSG *msg)
{
	XMRECT rc, rect;
	 int x, y,width;
	int button_index = -1;
	 int offset;
	int count;

	if(!pButtonControl->bBtnEnable || !pButtonControl->bBtnInited)
		return -1;

	XM_GetDesktopRect (&rc);
	rect = rc;

	// 计算相对基准屏(320X240)的偏移
	offset = rc.bottom - APP_BUTTON_HEIGHT;
	x = LOWORD(msg->lp);
	y = HIWORD(msg->lp);

	if((y>rc.bottom)||(y < offset))
		return -1;

	count = pButtonControl->bBtnCount;
	if(count == 1)
		count ++;
    
	//button_index = x * count / (rc.right - rc.left + 1);
 	width = TP_WIDTH/count;
    button_index = (unsigned int)(x - TP_LEFT)/width;
	
	if(button_index >= count)
		return -1;
	
	return button_index;
}
static int check_TPbutton_clicked (TPBUTTONCONTROL *TpButtonControl, XMMSG *msg)
{
	#define OFFSET 	10
	
	unsigned int x, y;
	int button_index = -1;
	int count;
	
	if(!TpButtonControl->bBtnEnable || !TpButtonControl->bBtnInited)
		return -1;

	x = LOWORD(msg->lp)-530+48;
	y = HIWORD(msg->lp);

	if((y<100)||(y>300))
		return -1;
	
    for(button_index=0;button_index<TpButtonControl->bBtnCount;button_index++)
    {
	    //if (x >= (TpButtonControl->TpBtnInfo[button_index].left - OFFSET) && x < (TpButtonControl->TpBtnInfo[button_index].right + OFFSET) && y >( TpButtonControl->TpBtnInfo[button_index].top - OFFSET-50) && y < (TpButtonControl->TpBtnInfo[button_index].bottom + OFFSET-100))
	    if (x >= (TpButtonControl->TpBtnInfo[button_index].left - OFFSET) && x < (TpButtonControl->TpBtnInfo[button_index].right + OFFSET) && y >( TpButtonControl->TpBtnInfo[button_index].top - OFFSET) && y < (TpButtonControl->TpBtnInfo[button_index].bottom + OFFSET))
		{
	         //XM_printf("button_index is %d\n",button_index);
	         return button_index;
	    }
    }
    return -1;
}




/*视频播放时触摸控件不要响按键音*/
XMBOOL AP_VideoplayingButtonControlMessageHandler (XMBUTTONCONTROL *pButtonControl, XMMSG *msg)
{
	int i;
	XMBOOL bButtonProcessed = 0;
	
	// 检查按钮控件是否已初始化
	if(pButtonControl->bBtnInited != XMBTNINIT)
		return bButtonProcessed;

	if(msg->message == XM_KEYDOWN && pButtonControl->bBtnEnable)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// 检查该键是否被定义为按钮控件的功能键
				i = testBtnKey (pButtonControl, msg->wp);
				if(i == (-1))
				{
					// 没有被定义为按钮控件的功能键
					// 检查按钮控件的功能键是否存在按下。若是，将其取消
					if(pButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// 存在功能键按下。取消其按下效果显示
					pButtonControl->bBtnClick = (BYTE)(-1);
					// 直接写屏刷新
					AP_ButtonDirectPaint (pButtonControl);
					// 刷新
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					return bButtonProcessed;
				}

				// 检查按钮是否已选择
				if(pButtonControl->bBtnClick == (BYTE)i)
					return bButtonProcessed;
				pButtonControl->bBtnClick = (BYTE)i;

				AP_ButtonDirectPaint (pButtonControl);
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				break;

			default:
				// 检查按钮控件的功能键是否存在按下。若是，将其取消
				if(pButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// 存在功能键按下。取消其按下效果显示
				pButtonControl->bBtnClick = (BYTE)(-1);
				// 刷新
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_ButtonDirectPaint (pButtonControl);
				break;
		}
	}
	else if(msg->message == XM_KEYUP && pButtonControl->bBtnEnable)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// 检查该键是否被定义为按钮控件的功能键
				i = testBtnKey (pButtonControl, msg->wp);
				if(i == (-1))
				{
					// 没有被定义为按钮控件的功能键
					// 检查按钮控件的功能键是否存在按下。若是，将其取消
					if(pButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// 存在功能键按下。取消其按下效果显示
					pButtonControl->bBtnClick = (BYTE)(-1);
					// 刷新
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					AP_ButtonDirectPaint (pButtonControl);
					return bButtonProcessed;
				}
				if(pButtonControl->bBtnClick != (BYTE)i)
					return bButtonProcessed;
				pButtonControl->bBtnClick = (BYTE)(-1);
				XM_PostMessage (XM_COMMAND, pButtonControl->pBtnInfo[i].wCommand, i);

				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_ButtonDirectPaint (pButtonControl);
				break;

			default:
				// 检查按钮控件的功能键是否存在按下。若是，将其取消
				if(pButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// 存在功能键按下。取消其按下效果显示
				pButtonControl->bBtnClick = (BYTE)(-1);
				// 刷新
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_ButtonDirectPaint (pButtonControl);
				break;
		}
	}
	else if(msg->message == XM_PAINT)
	{
		AP_ButtonPaint (pButtonControl);
	}
	else if(msg->message == XM_TOUCHDOWN)
	{
		int button_click_index;
	
		// 异常情况处理
		if(pButtonControl->bBtnClick  != (BYTE)(-1))
		{
			pButtonControl->bBtnClick = (BYTE)(-1);
			AP_ButtonDirectPaint (pButtonControl);
		}
		
		button_click_index = check_button_clicked (pButtonControl, msg);
		
		if(button_click_index < 0)
			return bButtonProcessed;

		bButtonProcessed = 1;
		if(button_click_index < pButtonControl->bBtnCount)
		{
		      
			pButtonControl->bBtnClick = (BYTE)button_click_index;
			AP_ButtonDirectPaint (pButtonControl);
		}
	}
	else if(msg->message == XM_TOUCHUP)
	{
		if(pButtonControl->bBtnClick != (BYTE)(-1))
		{
			int i = pButtonControl->bBtnClick;
			pButtonControl->bBtnClick = (BYTE)(-1);
			AP_ButtonDirectPaint (pButtonControl);
			XM_PostMessage (XM_COMMAND, pButtonControl->pBtnInfo[i].wCommand, i);
			bButtonProcessed = 1;
		}
	}
	else if(msg->message == XM_TOUCHMOVE)
	{
		if(pButtonControl->bBtnClick != (BYTE)(-1))
		{
			bButtonProcessed = 1;
		}
	}
	return bButtonProcessed;
}

XMBOOL AP_ButtonControlMessageHandler(XMBUTTONCONTROL *pButtonControl, XMMSG *msg)
{
	int i;
	XMBOOL bButtonProcessed = 0;
	
	// 检查按钮控件是否已初始化
	if(pButtonControl->bBtnInited != XMBTNINIT)
		return bButtonProcessed;

	XM_printf(">>>>AP_ButtonControlMessageHandler, msg->message:%x\r\n", msg->message);
	XM_printf(">>>>AP_ButtonControlMessageHandler, pButtonControl->bBtnEnable:%x\r\n", pButtonControl->bBtnEnable);
	if(msg->message == XM_KEYDOWN && pButtonControl->bBtnEnable)
	{
		XM_printf(">>>>>>>>>>>msg->wp:%x\r\n", msg->wp);
		switch(msg->wp)
		{
			//case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// 检查该键是否被定义为按钮控件的功能键
				i = testBtnKey (pButtonControl, msg->wp);
				XM_printf(">>>>i:%d\r\n", i);
				if(i == (-1))
				{
					// 没有被定义为按钮控件的功能键

					// 检查按钮控件的功能键是否存在按下。若是，将其取消
					if(pButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// 存在功能键按下。取消其按下效果显示
					pButtonControl->bBtnClick = (BYTE)(-1);
					// 直接写屏刷新
					AP_ButtonDirectPaint (pButtonControl);
					// 刷新
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					return bButtonProcessed;
				}
				XM_printf(">>>>pButtonControl->bBtnClick:%d\r\n", pButtonControl->bBtnClick);

				// 检查按钮是否已选择
				if(pButtonControl->bBtnClick == (BYTE)i)
					return bButtonProcessed;
				pButtonControl->bBtnClick = (BYTE)i;
				
				XM_PostMessage (XM_COMMAND, pButtonControl->pBtnInfo[i].wCommand, i);//luo
				AP_ButtonDirectPaint (pButtonControl);
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				break;

			default:
				// 检查按钮控件的功能键是否存在按下。若是，将其取消
				if(pButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// 存在功能键按下。取消其按下效果显示
				pButtonControl->bBtnClick = (BYTE)(-1);
				// 刷新
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_ButtonDirectPaint (pButtonControl);
				break;
		}
	}
	else if(msg->message == XM_KEYUP && pButtonControl->bBtnEnable)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// 检查该键是否被定义为按钮控件的功能键
				i = testBtnKey (pButtonControl, msg->wp);
				if(i == (-1))
				{
					// 没有被定义为按钮控件的功能键
					// 检查按钮控件的功能键是否存在按下。若是，将其取消
					if(pButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// 存在功能键按下。取消其按下效果显示
					pButtonControl->bBtnClick = (BYTE)(-1);
					// 刷新
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					AP_ButtonDirectPaint (pButtonControl);
					return bButtonProcessed;
				}
				if(pButtonControl->bBtnClick != (BYTE)i)
					return bButtonProcessed;
				pButtonControl->bBtnClick = (BYTE)(-1);
				XM_PostMessage(XM_COMMAND, pButtonControl->pBtnInfo[i].wCommand, i);

				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_ButtonDirectPaint (pButtonControl);
				break;

			default:
				// 检查按钮控件的功能键是否存在按下。若是，将其取消
				if(pButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// 存在功能键按下。取消其按下效果显示
				pButtonControl->bBtnClick = (BYTE)(-1);
				// 刷新
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_ButtonDirectPaint (pButtonControl);
				break;
		}
	}
	else if(msg->message == XM_PAINT)
	{
		AP_ButtonPaint (pButtonControl);
	}
	else if(msg->message == XM_TOUCHDOWN)
	{
		int button_click_index;
	
		// 异常情况处理
		if(pButtonControl->bBtnClick  != (BYTE)(-1))
		{
			pButtonControl->bBtnClick = (BYTE)(-1);
			AP_ButtonDirectPaint (pButtonControl);
		}
		
		button_click_index = check_button_clicked (pButtonControl, msg);
		
		if(button_click_index < 0)
			return bButtonProcessed;

		bButtonProcessed = 1;
		if(button_click_index < pButtonControl->bBtnCount)
		{
		    XM_Beep (XM_BEEP_KEYBOARD);//eason
			pButtonControl->bBtnClick = (BYTE)button_click_index;
			AP_ButtonDirectPaint (pButtonControl);
		}
	}
	else if(msg->message == XM_TOUCHUP)
	{
		if(pButtonControl->bBtnClick != (BYTE)(-1))
		{
			int i = pButtonControl->bBtnClick;
			pButtonControl->bBtnClick = (BYTE)(-1);
			AP_ButtonDirectPaint (pButtonControl);
			XM_PostMessage (XM_COMMAND, pButtonControl->pBtnInfo[i].wCommand, i);
			bButtonProcessed = 1;
		}
	}
	else if(msg->message == XM_TOUCHMOVE)
	{
		if(pButtonControl->bBtnClick != (BYTE)(-1))
		{
			bButtonProcessed = 1;
		}
	}
	return bButtonProcessed;
}


XMBOOL AP_TpButtonControlMessageHandlerDateSet (TPBUTTONCONTROL *TpButtonControl, XMMSG *msg)
{
	int i;
	XMBOOL bButtonProcessed = 0;

	// 检查按钮控件是否已初始化
	if(TpButtonControl->bBtnInited != XMBTNINIT)
		return bButtonProcessed;
	
	if(msg->message == XM_KEYDOWN && TpButtonControl->bBtnEnable)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// 检查该键是否被定义为按钮控件的功能键
				i = testTpBtnKey (TpButtonControl, msg->wp);
				if(i == (-1))
				{
					// 没有被定义为按钮控件的功能键

					// 检查按钮控件的功能键是否存在按下。若是，将其取消
					if(TpButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// 存在功能键按下。取消其按下效果显示
					TpButtonControl->bBtnClick = (BYTE)(-1);
					// 直接写屏刷新
					AP_TpButtonDirectPaint (TpButtonControl);
					// 刷新
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					return bButtonProcessed;
				}

				// 检查按钮是否已选择
				if(TpButtonControl->bBtnClick == (BYTE)i)
					return bButtonProcessed;
				TpButtonControl->bBtnClick = (BYTE)i;

				AP_TpButtonDirectPaint (TpButtonControl);
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				break;

			default:
				// 检查按钮控件的功能键是否存在按下。若是，将其取消
				if(TpButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// 存在功能键按下。取消其按下效果显示
				TpButtonControl->bBtnClick = (BYTE)(-1);
				// 刷新
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_TpButtonDirectPaint (TpButtonControl);
				break;
		}
	}
	else if(msg->message == XM_KEYUP && TpButtonControl->bBtnEnable)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// 检查该键是否被定义为按钮控件的功能键
				i = testTpBtnKey (TpButtonControl, msg->wp);
				if(i == (-1))
				{
					// 没有被定义为按钮控件的功能键
					// 检查按钮控件的功能键是否存在按下。若是，将其取消
					if(TpButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// 存在功能键按下。取消其按下效果显示
					TpButtonControl->bBtnClick = (BYTE)(-1);
					// 刷新
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					AP_TpButtonDirectPaint (TpButtonControl);
					return bButtonProcessed;
				}
				if(TpButtonControl->bBtnClick != (BYTE)i)
					return bButtonProcessed;
				TpButtonControl->bBtnClick = (BYTE)(-1);
				XM_PostMessage (XM_COMMAND, TpButtonControl->TpBtnInfo[i].wCommand, i);
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_TpButtonDirectPaint (TpButtonControl);
				break;

			default:
				// 检查按钮控件的功能键是否存在按下。若是，将其取消
				if(TpButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// 存在功能键按下。取消其按下效果显示
				TpButtonControl->bBtnClick = (BYTE)(-1);
				// 刷新
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_TpButtonDirectPaint (TpButtonControl);
				break;
		}
	}
	else if(msg->message == XM_PAINT)
	{
		AP_TpButtonPaint (TpButtonControl);
	}
	else if(msg->message == XM_TOUCHDOWN)
	{
		int button_click_index;
		
		// 异常情况处理
		if(TpButtonControl->bBtnClick  != (BYTE)(-1))
		{
			TpButtonControl->bBtnClick = (BYTE)(-1);
			AP_TpButtonDirectPaint (TpButtonControl);
		}
		button_click_index = check_TPbutton_clicked (TpButtonControl, msg);
		if(button_click_index < 0)
			return bButtonProcessed;

		bButtonProcessed = 1;
		if(button_click_index < TpButtonControl->bBtnCount)
		{
		    XM_Beep (XM_BEEP_KEYBOARD);//eason
			TpButtonControl->bBtnClick = (BYTE)button_click_index;
			//AP_TpButtonDirectPaint (TpButtonControl);
		}
	}
	else if(msg->message == XM_TOUCHUP)
	{
		if(TpButtonControl->bBtnClick != (BYTE)(-1))
		{
			int i = TpButtonControl->bBtnClick;
			TpButtonControl->bBtnClick = (BYTE)(-1);
			//AP_TpButtonDirectPaint (TpButtonControl);
			XM_PostMessage (XM_COMMAND, TpButtonControl->TpBtnInfo[i].wCommand, i);
			//XM_printf("pButtonControl->pBtnInfo[i].wCommand is%d\n",TpButtonControl->TpBtnInfo[i].wCommand);
			bButtonProcessed = 1;
		}
	}
	else if(msg->message == XM_TOUCHMOVE)
	{
		if(TpButtonControl->bBtnClick != (BYTE)(-1))
		{
			bButtonProcessed = 1;
		}
	}
	return bButtonProcessed;
}


XMBOOL AP_TpButtonControlMessageHandler (TPBUTTONCONTROL *TpButtonControl, XMMSG *msg)
{
	int i;
	XMBOOL bButtonProcessed = 0;
	
	// 检查按钮控件是否已初始化
	if(TpButtonControl->bBtnInited != XMBTNINIT)
		return bButtonProcessed;
	
	if(msg->message == XM_KEYDOWN && TpButtonControl->bBtnEnable)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// 检查该键是否被定义为按钮控件的功能键
				i = testTpBtnKey (TpButtonControl, msg->wp);
				if(i == (-1))
				{
					// 没有被定义为按钮控件的功能键

					// 检查按钮控件的功能键是否存在按下。若是，将其取消
					if(TpButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// 存在功能键按下。取消其按下效果显示
					TpButtonControl->bBtnClick = (BYTE)(-1);
					// 直接写屏刷新
					AP_TpButtonDirectPaint (TpButtonControl);
					// 刷新
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					return bButtonProcessed;
				}

				// 检查按钮是否已选择
				if(TpButtonControl->bBtnClick == (BYTE)i)
					return bButtonProcessed;
				TpButtonControl->bBtnClick = (BYTE)i;

				AP_TpButtonDirectPaint (TpButtonControl);
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				break;

			default:
				// 检查按钮控件的功能键是否存在按下。若是，将其取消
				if(TpButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// 存在功能键按下。取消其按下效果显示
				TpButtonControl->bBtnClick = (BYTE)(-1);
				// 刷新
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_TpButtonDirectPaint (TpButtonControl);
				break;
		}
	}
	else if(msg->message == XM_KEYUP && TpButtonControl->bBtnEnable)
	{
		switch(msg->wp)
		{
			case VK_AP_MENU:
			case VK_AP_MODE:
			case VK_AP_SWITCH:
			case VK_AP_POWER:
				// 检查该键是否被定义为按钮控件的功能键
				i = testTpBtnKey (TpButtonControl, msg->wp);
				if(i == (-1))
				{
					// 没有被定义为按钮控件的功能键

					// 检查按钮控件的功能键是否存在按下。若是，将其取消
					if(TpButtonControl->bBtnClick == (BYTE)(-1))
						return bButtonProcessed;
					
					// 存在功能键按下。取消其按下效果显示
					TpButtonControl->bBtnClick = (BYTE)(-1);
					// 刷新
					//XM_InvalidateWindow ();
					//XM_UpdateWindow ();
					AP_TpButtonDirectPaint (TpButtonControl);
					return bButtonProcessed;
				}
				if(TpButtonControl->bBtnClick != (BYTE)i)
					return bButtonProcessed;
				TpButtonControl->bBtnClick = (BYTE)(-1);
				XM_PostMessage (XM_COMMAND, TpButtonControl->TpBtnInfo[i].wCommand, i);
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_TpButtonDirectPaint (TpButtonControl);
				break;

			default:
				// 检查按钮控件的功能键是否存在按下。若是，将其取消
				if(TpButtonControl->bBtnClick == (BYTE)(-1))
					return bButtonProcessed;
					
				// 存在功能键按下。取消其按下效果显示
				TpButtonControl->bBtnClick = (BYTE)(-1);
				// 刷新
				//XM_InvalidateWindow ();
				//XM_UpdateWindow ();
				AP_TpButtonDirectPaint (TpButtonControl);
				break;
		}
	}
	else if(msg->message == XM_PAINT)
	{
		AP_TpButtonPaint (TpButtonControl);
	}
	else if(msg->message == XM_TOUCHDOWN)
	{
		int button_click_index;
		
		// 异常情况处理
		if(TpButtonControl->bBtnClick  != (BYTE)(-1))
		{
			TpButtonControl->bBtnClick = (BYTE)(-1);
			AP_TpButtonDirectPaint (TpButtonControl);
		}
		button_click_index = check_TPbutton_clicked (TpButtonControl, msg);
		if(button_click_index < 0)
			return bButtonProcessed;

		bButtonProcessed = 1;
		if(button_click_index < TpButtonControl->bBtnCount)
		{
		    XM_Beep (XM_BEEP_KEYBOARD);//eason
			TpButtonControl->bBtnClick = (BYTE)button_click_index;
			AP_TpButtonDirectPaint (TpButtonControl);
		}
	}
	else if(msg->message == XM_TOUCHUP)
	{
		if(TpButtonControl->bBtnClick != (BYTE)(-1))
		{
			int i = TpButtonControl->bBtnClick;
			TpButtonControl->bBtnClick = (BYTE)(-1);
			AP_TpButtonDirectPaint (TpButtonControl);
			XM_PostMessage (XM_COMMAND, TpButtonControl->TpBtnInfo[i].wCommand, i);
			//XM_printf("pButtonControl->pBtnInfo[i].wCommand is%d\n",TpButtonControl->TpBtnInfo[i].wCommand);
			bButtonProcessed = 1;
		}
	}
	else if(msg->message == XM_TOUCHMOVE)
	{
		if(TpButtonControl->bBtnClick != (BYTE)(-1))
		{
			bButtonProcessed = 1;
		}
	}
	return bButtonProcessed;
}

