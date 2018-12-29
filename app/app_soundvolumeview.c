//****************************************************************************
//
//	Copyright (C) 2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_soundvolumeview.c
//	  音量调节视窗
//
//	Revision history
//
//		2014.03.23	ZhuoYongHong Initial version
//
//****************************************************************************

#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"

// 背景色定义
#define	SOUNDVOLUME_BACKGROUND_COLOR		0x50404040

#define	WAIT_TIMEOUT	3000	// ms，按键等待周期
#define	EXIT_TIMEOUT	1000	// ms，退出渐隐周期

// 标题区

// 铃声或MIC图片

// 铃声指示方块 14X14，白色，间隔4
#define	INDICATOR_SIZE			14
#define	INDICATOR_SPACE		4

#define	HORZ_SPACE_SIZE		10		// 水平方向空白区域的大小
#define	VERT_SPACE_SIZE		14		// 垂直方向空白及UI元素之间的间隔

// 视窗子类型定义
#define	MIC_TYPE				0		// MIC音量调节
#define	BELL_TYPE				1		// 铃声音量调节

typedef struct tagSOUNDVOLUMEVIEWDATA {
	DWORD				dwTitleID;						// 铃声/MIC信息文本资源ID
	DWORD				dwImage_ON_ID;					// 背景图片ID(开启状态)
	DWORD				dwImage_Off_ID;				// 背景图片ID(关闭状态)
	UCHAR				type;								// MIC/BELL视窗子类型
	UCHAR				ucSoundVolume;					// 铃声/录音音量值
	UCHAR				ucOldSoundVolume;				// 老的铃声/录音音量值
	UCHAR				rev;

	// 内部状态
	XMSIZE			sizeTitle;						// 标题区大小
	XMSIZE			sizeImage;						// 图片区大小
	XMSIZE			sizeIndicator;					// 音量指示器大小

	USHORT			dwWaitTimeout;					// 等待计数器
	USHORT			dwExitTimeout;					// 关闭计数器，调整Alpha值并退出		
} SOUNDVOLUMEVIEWDATA;

XMBOOL XM_OpenSoundVolumeView (DWORD dwTitleID, UCHAR type)
{
	SOUNDVOLUMEVIEWDATA *viewData;
	XMRECT rc;

	XM_GetDesktopRect (&rc);

	// 参数检查
	if(dwTitleID == 0)
	{
		XM_printf ("XM_OpenSoundVolumeView NG, dwInfoTextID(%d) must be non-zero\n", dwTitleID);
		return 0;
	}

	if(type > BELL_TYPE)
	{
		XM_printf ("XM_OpenSoundVolumeView NG, type(%d) illegal\n", type);
		return 0;
	}
	
	viewData = XM_calloc (sizeof(SOUNDVOLUMEVIEWDATA));
	if(viewData == NULL)
		return 0;
	viewData->type = type;
	viewData->dwTitleID = dwTitleID;
	if(type == MIC_TYPE)
	{
		viewData->ucSoundVolume = (unsigned char)AP_GetMenuItem (APPMENUITEM_MIC_VOLUME);
		if( (rc.bottom - rc.top + 1) >= 320)
			viewData->dwImage_ON_ID = AP_ID_SETTING_VOLUME_MIC_IMAGE;
		else
			viewData->dwImage_ON_ID = AP_ID_SETTING_VOLUME_MIC_IMAGE_SMALL;
	}
	else
	{
		viewData->ucSoundVolume = (unsigned char)AP_GetMenuItem (APPMENUITEM_BELL_VOLUME);
		if( (rc.bottom - rc.top + 1) >= 320)
			viewData->dwImage_ON_ID = AP_ID_SETTING_VOLUME_SOUND_IMAGE;
		else
			viewData->dwImage_ON_ID = AP_ID_SETTING_VOLUME_MIC_IMAGE_SMALL;
	}
	viewData->ucOldSoundVolume = viewData->ucSoundVolume;
	if(XM_PushWindowEx (XMHWND_HANDLE(SoundVolumeView), (DWORD)viewData) == 0)
	{
		XM_free (viewData);
		return 0;
	}
	return 1;
}

// 打开铃声音量调节视窗
XMBOOL XM_OpenBellSoundVolumeSettingView (VOID)
{
	return XM_OpenSoundVolumeView (AP_ID_SETTING_VOLUME_SOUND_TITLE_BELL, BELL_TYPE);
}

// 打开MIC录音音量调节视窗
XMBOOL XM_OpenMicSoundVolumeSettingView (VOID)
{
	return XM_OpenSoundVolumeView (AP_ID_SETTING_VOLUME_MIC_TITLE_BELL, MIC_TYPE);
}


VOID SoundVolumeViewOnEnter (XMMSG *msg)
{
	SOUNDVOLUMEVIEWDATA *viewData;
	HANDLE hWnd;
	hWnd = XMHWND_HANDLE(SoundVolumeView);
	if(msg->wp == 0)
	{
		int x, y, w, h;
		APPROMRES *AppRes;
		XMSIZE sizeTitle;
		XMSIZE sizeImage;
		XMSIZE sizeIndicator;
		XMRECT rectDesktop;
		// 第一次进入(窗口准备创建)
		viewData = (SOUNDVOLUMEVIEWDATA *)msg->lp;

		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (hWnd, viewData);

		// 计算窗口的位置信息 (根据所有UI元素大小)
		XM_GetDesktopRect (&rectDesktop);
		w = 0;
		h = 0;
		sizeTitle.cx = 0;
		sizeTitle.cy = 0;
		sizeImage.cx = 0;
		sizeImage.cy = 0;
		sizeIndicator.cx = INDICATOR_SIZE * (AP_SETTING_SOUNDVOLUME_COUNT - 1) + INDICATOR_SPACE * (AP_SETTING_SOUNDVOLUME_COUNT - 2);
		sizeIndicator.cy = INDICATOR_SIZE;
		//	1) 计算信息文本的显示区域大小
		AppRes = AP_AppRes2RomRes (viewData->dwTitleID);
		if(AppRes)
		{
			XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &sizeTitle);
		}

		// 2) 计算背景图片的显示区域大小
		if(viewData->dwImage_ON_ID)
		{
			AppRes = AP_AppRes2RomRes (viewData->dwImage_ON_ID);
			if(AppRes)
			{
				XM_GetRomImageSize (AppRes->rom_offset, AppRes->res_length, &sizeImage);
			}
		}

		viewData->sizeTitle = sizeTitle;
		viewData->sizeImage = sizeImage;
		viewData->sizeIndicator = sizeIndicator;

		w = sizeTitle.cx;
		if(w < sizeImage.cx)
			w = sizeImage.cx;
		if(w < sizeIndicator.cx)
			w = sizeIndicator.cx;
		w += HORZ_SPACE_SIZE * 2;	// 两边空白

		h = VERT_SPACE_SIZE + sizeTitle.cy + VERT_SPACE_SIZE + sizeIndicator.cy + VERT_SPACE_SIZE;
		if(sizeImage.cy)
		{
			h +=  VERT_SPACE_SIZE + sizeImage.cy;
		}
		
		// 屏幕居中显示
		x = ((rectDesktop.right - rectDesktop.left + 1) - w) / 2;
		y = ((rectDesktop.bottom - rectDesktop.top + 1) - h) / 2;
			
		// 重新调整视窗的位置及大小
		XM_SetWindowPos (hWnd, x, y, w, h);

		viewData->dwWaitTimeout = WAIT_TIMEOUT / 100;
		viewData->dwExitTimeout = EXIT_TIMEOUT / 100;

		// 禁止视窗创建/关闭时使用动画效果
		XM_DisableViewAnimate (hWnd);
	}
	else
	{
		viewData = (SOUNDVOLUMEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SoundVolumeView));
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("SoundVolumeView Pull\n");
	}

	// 创建定时器
	XM_SetTimer (XMTIMER_MESSAGEVIEW, 100);	// 创建100ms的定时器
}

VOID SoundVolumeViewOnLeave (XMMSG *msg)
{
	SOUNDVOLUMEVIEWDATA *viewData = (SOUNDVOLUMEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SoundVolumeView));
	// 删除定时器
	XM_KillTimer (XMTIMER_MESSAGEVIEW);

	if (msg->wp == 0)
	{
		// 保存音量值设置到内部存储区
		if(viewData && viewData->ucOldSoundVolume != viewData->ucSoundVolume)
		{
			AP_SaveMenuData (&AppMenuData);
		}

		if(viewData)
		{
			// 释放私有数据句柄
			XM_free (viewData);
			
		}
		XM_printf ("SoundVolumeView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("SoundVolumeView Push\n");
	}
}


VOID SoundVolumeViewOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
//	APPROMRES *AppRes;
	XMCOLOR bkg_clolor;
	int i, count;
	HANDLE hWnd = XMHWND_HANDLE(SoundVolumeView);
	SOUNDVOLUMEVIEWDATA *viewData = (SOUNDVOLUMEVIEWDATA *)XM_GetWindowPrivateData (hWnd);
	if(viewData == NULL)
		return;

	// 获取视窗位置信息
	XM_GetWindowRect (hWnd, &rc);

	// 填充背景(使用具有Alpha值的背景色)
	bkg_clolor = SOUNDVOLUME_BACKGROUND_COLOR;
	XM_FillRect (hWnd, rc.left, rc.top, rc.right, rc.bottom, bkg_clolor);

	// 显示标题
	rect = rc;
	rect.top += VERT_SPACE_SIZE;
	rect.bottom = rect.top + viewData->sizeTitle.cy - 1;
//	AppRes = AP_AppRes2RomRes (viewData->dwTitleID);
	// XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
//	AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
	AP_RomImageDrawByMenuID (viewData->dwTitleID, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
	rect.top += viewData->sizeTitle.cy;

	// 显示BELL/MIC图片
	if(viewData->sizeImage.cy)
	{
		rect.top += VERT_SPACE_SIZE;
		rect.bottom = rect.top + viewData->sizeImage.cy - 1;
	//	AppRes = AP_AppRes2RomRes (viewData->dwImage_ON_ID);
		//XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
	//	AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		AP_RomImageDrawByMenuID (viewData->dwImage_ON_ID, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
		rect.top += viewData->sizeImage.cy;
	}

	// 显示音量指示
	rect.top += VERT_SPACE_SIZE;
	rect.bottom = rect.top + viewData->sizeIndicator.cy - 1;
	rect.left = rect.left + ((rect.right - rect.left + 1) - viewData->sizeIndicator.cx) / 2;
	rect.right = rect.left + INDICATOR_SIZE - 1;
	count = viewData->ucSoundVolume;
	if(count > (AP_SETTING_SOUNDVOLUME_COUNT - 1))
		count = AP_SETTING_SOUNDVOLUME_COUNT - 1;
	bkg_clolor = 0xFFFFFF | (XM_GetWindowAlpha (hWnd) << 24);	// 白色+Alpha
	for (i = 0; i < count; i ++)
	{
		XM_FillRect (hWnd, rect.left, rect.top, rect.right, rect.bottom, bkg_clolor);
		rect.left += INDICATOR_SIZE + INDICATOR_SPACE;
		rect.right += INDICATOR_SIZE + INDICATOR_SPACE;
	}
}


VOID SoundVolumeViewOnKeyDown (XMMSG *msg)
{
	HANDLE hWnd = XMHWND_HANDLE(SoundVolumeView);
	SOUNDVOLUMEVIEWDATA *viewData = (SOUNDVOLUMEVIEWDATA *)XM_GetWindowPrivateData (hWnd);
	if(viewData == NULL)
		return;
	
	switch(msg->wp)
	{
	       #if 0
		case VK_AP_UP:
			if(viewData->ucSoundVolume < (AP_SETTING_SOUNDVOLUME_COUNT - 1))
			{
				viewData->ucSoundVolume ++;
				if(viewData->type == MIC_TYPE)
				{
					// MIC
					AP_SetMenuItem (APPMENUITEM_MIC_VOLUME, viewData->ucSoundVolume);
					if(viewData->ucSoundVolume == 1)
					{
						// 开启MIC
						AP_SetMenuItem (APPMENUITEM_MIC, 1);
					}
				}
				else
				{
					// BELL
					AP_SetMenuItem (APPMENUITEM_BELL_VOLUME, viewData->ucSoundVolume);
				}
				XM_InvalidateWindow ();
			}
			// 恢复视窗的Alpha为不透明
			XM_SetWindowAlpha (hWnd, (UCHAR)(255));
			// 重置计数器
			viewData->dwWaitTimeout = WAIT_TIMEOUT / 100;
			viewData->dwExitTimeout = EXIT_TIMEOUT / 100;
			
			break;

		case VK_AP_DOWN:
			if(viewData->ucSoundVolume > 0)
			{
				viewData->ucSoundVolume --;
				if(viewData->type == MIC_TYPE)
				{
					AP_SetMenuItem (APPMENUITEM_MIC_VOLUME, viewData->ucSoundVolume);
					if(viewData->ucSoundVolume == 0)
					{
						// 关闭MIC录音
						AP_SetMenuItem (APPMENUITEM_MIC, 0);
					}
				}
				else
				{
					AP_SetMenuItem (APPMENUITEM_BELL_VOLUME, viewData->ucSoundVolume);
					if(viewData->ucSoundVolume == 0)
					{
						// 关闭语音提示
						AP_SetMenuItem (APPMENUITEM_VOICE_PROMPTS, 0);
					}
				}
				XM_InvalidateWindow ();
			}
			// 恢复视窗的Alpha为不透明
			XM_SetWindowAlpha (hWnd, (UCHAR)(255));
			// 重置计数器
			viewData->dwWaitTimeout = WAIT_TIMEOUT / 100;
			viewData->dwExitTimeout = EXIT_TIMEOUT / 100;
			break;
                 #endif
		default:
			// 此处将键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			break;

	}
}

VOID SoundVolumeViewOnTimer (XMMSG *msg)
{
	SOUNDVOLUMEVIEWDATA *viewData;
	float alpha;
	HANDLE hWnd = XMHWND_HANDLE(SoundVolumeView);
	viewData = (SOUNDVOLUMEVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SoundVolumeView));
	if(viewData == NULL)
		return;

	if(viewData->dwWaitTimeout > 0)
	{
		viewData->dwWaitTimeout --;
	}
	else if(viewData->dwExitTimeout > 0)
	{
		// 实现退出过程视窗渐隐效果(通过减小view的alpha值)
		viewData->dwExitTimeout --;
		alpha = XM_GetWindowAlpha (hWnd);
		alpha = (float)(alpha * viewData->dwExitTimeout / 10.0);
		XM_printf ("alpha=%d\n", (UCHAR)(alpha));
		// 调整视窗的alpha值
		XM_SetWindowAlpha (hWnd, (UCHAR)(alpha));

		XM_InvalidateWindow ();
	}
	else
	{
		// 返回到调用者视窗
		XM_PullWindow (0);
	}
}

// 弹出窗口无法处理系统消息事件，退出并交给系统缺省处理
VOID SoundVolumeViewOnSystemEvent (XMMSG *msg)
{
	// 返回到调用者视窗
	XM_BreakSystemEventDefaultProcess (msg);
	XM_PostMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
	XM_PullWindow (0);
}


// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (SoundVolumeView)
	XM_ON_MESSAGE (XM_PAINT, SoundVolumeViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, SoundVolumeViewOnKeyDown)
	XM_ON_MESSAGE (XM_ENTER, SoundVolumeViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, SoundVolumeViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, SoundVolumeViewOnTimer)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, SoundVolumeViewOnSystemEvent)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, LCD_YDOTS - 120, LCD_XDOTS, 120, SoundVolumeView, 1, 0, 0, 255, HWND_ALERT)
