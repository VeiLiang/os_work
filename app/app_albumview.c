//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_albumview.c
//	  相册列表浏览窗口
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include <xm_jpeg_codec.h>
#include "arkn141_codec.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"
//#include "app_album.h"
#include "system_check.h"
#include "xm_rotate.h"


#define	ALBUM_BUTTON_AUTO_HIDE_TIMEOUT	3000	// 按钮/照片标题自动隐藏时间 3000毫秒


static int decode_jpeg_file(const char *file_name);
void* kernel_malloc(unsigned int n);
void  kernel_free (void* pMemBlock);

typedef struct tagALBUMVIEWDATA {
	SHORT					nCurItem;					// 当前选择的菜单项

	int						nItemCount;					// 菜单项个数
	
	BYTE					channel;

	BYTE					delete_one_confirm;
	BYTE					delete_all_confirm;

	BYTE					button_hide;				// 按钮的显示/隐藏     1 隐藏 0 显示
	BYTE					photo_title_hide;			// 照片标题的显示/隐藏 1 隐藏 0 显示
	BYTE					photo_damaged;				// 照片解码错误

	XMBUTTONCONTROL			btnControl;

	char					fileName[XM_MAX_FILEFIND_NAME];		// 保存文件查找过程的文件名
	XMFILEFIND				fileFind;
} ALBUMVIEWDATA;


static VOID AlbumViewOnEnter (XMMSG *msg)
{
	char text[64];
	ALBUMVIEWDATA *albumViewData;
	HANDLE hPhotoItem;
	XMVIDEOITEM *pPhotoItem;
	unsigned int replay_ch;
	
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlbumViewOnEnter, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

	// 关闭过程ANIMATION
	XM_DisableViewAnimate (XMHWND_HANDLE(AlbumView));

	if(msg->wp == 0)
	{
		// 窗口未建立，第一次进入
		// 分配私有数据句柄
		albumViewData = XM_calloc (sizeof(ALBUMVIEWDATA));
		if(albumViewData == NULL)
		{
			XM_printf ("albumViewData XM_calloc failed\n");
			// 失败返回到调用者窗口
			XM_PullWindow (0);
			return;
		}

		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData(XMHWND_HANDLE(AlbumView), albumViewData);

		if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
		{
			replay_ch = XM_VIDEO_CHANNEL_0;
		}
		else
		{
			replay_ch = XM_VIDEO_CHANNEL_1;
		}
		
		albumViewData->channel = 0;
		// 开始扫描所有符合条件的视频文件
		albumViewData->nCurItem = (msg->lp) & 0xFFFF;
		albumViewData->nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_ALL);
		XM_printf(">>>>>>albumViewData->nCurItem:%d\r\n", albumViewData->nCurItem);
		XM_printf(">>>>>>albumViewData->nItemCount:%d\r\n", albumViewData->nItemCount);

		hPhotoItem = XM_VideoItemGetVideoItemHandleEx(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL,albumViewData->nCurItem,1);
		if(hPhotoItem == NULL)
			return;
		
		pPhotoItem = AP_VideoItemGetVideoItemFromHandle(hPhotoItem);
		if(pPhotoItem == NULL)
			return;
		
		// 获取照片的文件名
		if(XM_VideoItemGetVideoFilePath(pPhotoItem, replay_ch, text, sizeof(text)))
		{
			XM_printf(">>>>>>>>>>>>>>text:%s.............\r\n", text);
			// 将照片解码并显示在OSD 0层(视频层)
			if(decode_jpeg_file(text))
			{
				// 解码错误
				albumViewData->photo_damaged = 1;
			}
			else
			{
				albumViewData->photo_damaged = 0;
			}
		}
		else
		{
			albumViewData->photo_damaged = 1;
		}
	}
	else
	{
		int repaint = 0;		// 是否重绘窗口
		// 窗口已建立，当前窗口从栈中恢复		
		albumViewData = (ALBUMVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumView));
		if(albumViewData == NULL)
			return;

		XM_printf ("AlbumView Pull\n");
	}

	// 创建定时器，用于按钮及照片标题的隐藏
	// 创建x秒的定时器
	XM_SetTimer (XMTIMER_ALBUMLISTVIEW, ALBUM_BUTTON_AUTO_HIDE_TIMEOUT);
}

static VOID AlbumViewOnLeave (XMMSG *msg)
{
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AlbumViewOnLeave, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);
	// 删除定时器
	XM_KillTimer (XMTIMER_ALBUMLISTVIEW);

	if (msg->wp == 0)
	{
		// 窗口退出，彻底摧毁。
		// 获取私有数据句柄
		ALBUMVIEWDATA *albumViewData = (ALBUMVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumView));
		// 释放所有分配的资源
		if(albumViewData)
		{
			// 按钮控件退出过程
			AP_ButtonControlExit (&albumViewData->btnControl);
			// 释放私有数据句柄
			XM_free (albumViewData);
		}
		// 设置窗口的私有数据句柄为空
		XM_SetWindowPrivateData (XMHWND_HANDLE(AlbumView), NULL);

		XMSYS_H264CodecRecorderStart();
		XM_printf ("AlbumView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("AlbumView Push\n");
	}
}

extern int xm_jpeg_file_decode( const char *jpeg_data, size_t jpeg_size, 
								unsigned char *yuv,		// Y_UV420输出地址(NV12格式)
								unsigned int width,		// 解码输出的像素宽度
								unsigned int height		// 解码输出的像素高度
								);



// 将照片解码并显示在OSD 0层(视频层)
// -1 解码失败
//  0 解码成功
static int decode_jpeg_file (const char *file_name)
{
	xm_osd_framebuffer_t yuv_framebuffer;
	unsigned int w, h;
	unsigned int size; 
	void *fp;
	char *jpg = NULL;
	int ret = -1;

	// 获取视频层的尺寸信息
	w = XM_lcdc_osd_get_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	h = XM_lcdc_osd_get_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	// 创建一个新的视频层的帧缓存句柄
	yuv_framebuffer = XM_osd_framebuffer_create (0,
						XM_OSD_LAYER_0,
						XM_OSD_LAYER_FORMAT_Y_UV420,
						w,
						h,
						NULL,
						0,
						1		// clear_framebuffer_context
						);
	if(yuv_framebuffer == NULL)
	{
		XM_printf ("AlbumView failed, XM_osd_framebuffer_create NG\n");
		return -1;
	}

	do
	{
		fp = XM_fopen (file_name, "rb");
		if(fp == NULL)
		{
			XM_printf ("AlbumView failed, open file(%s) NG\n", file_name);
			break;
		}
		size = XM_filesize (fp);
		if(size == 0 || size > 0x200000)
		{
			XM_printf ("AlbumView failed, file(%s)'s size (%d) illegal\n", file_name, size);
			break;
		}

		jpg = kernel_malloc (size);
		if(jpg == NULL)
		{
			XM_printf ("AlbumView failed, malloc (%d) NG\n", size);
			break;
		}

		if(XM_fread (jpg, 1, size, fp) != size)
		{
			XM_printf ("AlbumView failed, XM_fread (%d) NG\n", size);
			break;
		}

		XM_fclose (fp);
		fp = NULL;

#ifdef LCD_ROTATE_90		
		{
			unsigned char *rotate_buffer = kernel_malloc (w * h * 3/2);
			if(rotate_buffer)
			{
				//ret = xm_jpeg_decode ((const char *)jpg, size, (unsigned char *)rotate_buffer, w, h);
				ret = xm_jpeg_file_decode ((const char *)jpg, size, (unsigned char *)rotate_buffer, w, h);
				if(ret == 0)
				{
					u8_t *data[4];
					XM_osd_framebuffer_get_buffer (yuv_framebuffer, data);
					//rotate_degree90 (	rotate_buffer, data[0], w, h, h, w);
					
					xm_rotate_90_by_zoom_opt ((u8_t *)rotate_buffer,
						  (u8_t *)rotate_buffer + w*h,
						  w,	h,	w,
						  0, 0, w, h,
						  data[0],
						  data[1],
						  w, h,
						  0,1);
				}
				kernel_free (rotate_buffer);
			}
			else
			{
				ret = -1;
			}
		}
#else
		// 将JPEG码流解码到新创建的帧缓存
		dma_inv_range ((unsigned int)yuv_framebuffer->address,(unsigned int)yuv_framebuffer->address + w * h * 3/2);
		ret = xm_jpeg_decode ((const char *)jpg, size, (unsigned char *)yuv_framebuffer->address, w, h);
#endif
		kernel_free (jpg);
		jpg = NULL;


		break;
	} while (jpg);

	if(fp)
		XM_fclose (fp);

	if(jpg)
		kernel_free (jpg);


	if(ret != 0)
	{
		// 解码失败
		// 清除framebuffer
		XM_osd_framebuffer_clear (yuv_framebuffer, 0, 255, 255, 255);
	}

	// 将framebuffer关闭, 将其更新为当前的视频缓冲, 即在OSD 0层显示
	XM_osd_framebuffer_close (yuv_framebuffer, 0);

	return ret;
}


static void AlbumShowPhotoTime (ALBUMVIEWDATA *albumViewData)
{
	XMSYSTEMTIME filetime;
	char text[64];
	const char *photo_name;
	HANDLE hPhotoItem;
	XMVIDEOITEM *pPhotoItem;
	unsigned int replay_ch;
	
	if(albumViewData->nItemCount <= 0)
		return;
	
	if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
	{
		replay_ch = XM_VIDEO_CHANNEL_0;
	}
	else
	{
		replay_ch = XM_VIDEO_CHANNEL_1;
	}	
		
	hPhotoItem = XM_VideoItemGetVideoItemHandleEx(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL,albumViewData->nCurItem,1);
	if(hPhotoItem == NULL)
		return;
	
	pPhotoItem = AP_VideoItemGetVideoItemFromHandle(hPhotoItem);
	if(pPhotoItem == NULL)
		return;
	
	//获取照片的文件名
	XM_VideoItemGetVideoFilePath(pPhotoItem, replay_ch, text, sizeof(text));

	XM_GetDateTimeFromFileName ((char *)pPhotoItem->video_name, &filetime);

	//格式化照片的拍摄时间
	sprintf(text, "%04d/%02d/%02d %02d:%02d:%02d", filetime.wYear, 
																		filetime.bMonth,
																		filetime.bDay,
																		filetime.bHour,
																		filetime.bMinute,
																		filetime.bSecond);
	// 显示照片的拍摄时间, 视窗的左上角(10, 10)																	
	AP_TextOutDataTimeString(XMHWND_HANDLE(AlbumView), 10, 560, text, strlen(text));
}

static VOID AlbumViewOnPaint (XMMSG *msg)
{
	XMRECT rc,rect;
	unsigned int old_alpha;
	HANDLE hWnd;
	float scale_factor;		// 水平缩放因子

	// 获取与视窗关联的私有数据
	ALBUMVIEWDATA *albumViewData = (ALBUMVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumView));
	if(albumViewData == NULL)
		return;

	// 获取窗口的句柄
	hWnd = XMHWND_HANDLE(AlbumView);

	// AlbumView定义时的视窗Alpha因子为0 (参考文件的最底部定义)
	// 将背景填充为全透明背景(Alpha为0)
	XM_GetDesktopRect (&rc);
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);
	XM_FillRect (hWnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	// 设置视窗的Alpha因子为255
	old_alpha =  XM_GetWindowAlpha (hWnd);
	XM_SetWindowAlpha(hWnd, 255);

	//文件名
	rect.left = 0;rect.top = 600-42;rect.right = 1024;rect.bottom = 42;
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUTITLEBARBACKGROUND, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//底部背景色

	//左右图标
	rect.left = 500;rect.top = 560;
	AP_RomImageDrawByMenuID(AP_ID_LEFT_ICON, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//底部背景色

	rect.left = 600;rect.top = 560;
	AP_RomImageDrawByMenuID(AP_ID_RIGHT_ICON, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//底部背景色
	
	//显示照片的标题
	AlbumShowPhotoTime(albumViewData);

	// 照片解码错误
	if(albumViewData->photo_damaged)
	{
		// 显示错误信息
		APPROMRES *AppRes; 
		// 获取资源句柄
		AppRes = AP_AppRes2RomRes (AP_ID_PHOTO_INFO_PHOTO_DAMAGE);
		if(AppRes)
		{
			// 显示PNG资源, 在视窗上居中显示
			XM_RomImageDraw (AppRes->rom_offset, AppRes->res_length, hWnd, &rc, XMGIF_DRAW_POS_CENTER);
		}
	}

	// 处理按钮控件的显示。
	// 若存在按钮控件，必须调用AP_ButtonControlMessageHandler执行按钮控件显示
	//AP_ButtonControlMessageHandler (&albumViewData->btnControl, msg);
	XM_SetWindowAlpha (hWnd, (unsigned char)old_alpha);
}


static VOID AlbumViewOnKeyDown (XMMSG *msg)
{
	char text[64];
	HANDLE hPhotoItem;
	XMVIDEOITEM *pPhotoItem;
	unsigned int replay_ch;

	XM_printf(">>>>>>>>>>>>>>>AlbumViewOnKeyDown, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	ALBUMVIEWDATA *albumViewData = (ALBUMVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumView));
	if(albumViewData == NULL)
		return;
	
	// 按键音
	XM_Beep(XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
		case VK_AP_FONT_BACK_SWITCH://AV键

			break;
			
		case VK_AP_DOWN:	
			XM_printf(">>>>>>>>>>>>VK_AP_DOWN...........\r\n");
			if(albumViewData->nItemCount <= 0)
				return;

			albumViewData->nCurItem --;
			if(albumViewData->nCurItem < 0)
			{// 聚焦到最后一个
				albumViewData->nCurItem = (WORD)(albumViewData->nItemCount - 1);
			}
			
			if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
			{
				replay_ch = XM_VIDEO_CHANNEL_0;
			}
			else
			{
				replay_ch = XM_VIDEO_CHANNEL_1;
			}

			hPhotoItem = XM_VideoItemGetVideoItemHandleEx(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL,albumViewData->nCurItem,1);
			if(hPhotoItem == NULL)
				return;
			
			pPhotoItem = AP_VideoItemGetVideoItemFromHandle(hPhotoItem);
			if(pPhotoItem == NULL)
				return;
			
			// 获取照片的文件名
			if(XM_VideoItemGetVideoFilePath(pPhotoItem, replay_ch, text, sizeof(text)))
			{
				XM_printf(">>>>>>>>>>text:%s............\r\n", text);
				// 将照片解码并显示在OSD 0层(视频层)
				if(decode_jpeg_file(text))
				{
					// 解码错误
					albumViewData->photo_damaged = 1;
				}
				else
				{
					albumViewData->photo_damaged = 0;
				}
			}
		
			XM_SetTimer (XMTIMER_ALBUMLISTVIEW, ALBUM_BUTTON_AUTO_HIDE_TIMEOUT);
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_UP:	
			XM_printf(">>>>>>>>>>>>VK_AP_UP...........\r\n");
			if(albumViewData->nItemCount <= 0)
				return;

			albumViewData->nCurItem ++;	
			if(albumViewData->nCurItem >= albumViewData->nItemCount)
			{
				// 聚焦到第一个
				albumViewData->nCurItem = 0;
			}

			if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
			{
				replay_ch = XM_VIDEO_CHANNEL_0;
			}
			else
			{
				replay_ch = XM_VIDEO_CHANNEL_1;
			}
			XM_printf(">>>>>>>>>>>>albumViewData->nCurItem:%d\r\n", albumViewData->nCurItem);
			hPhotoItem = XM_VideoItemGetVideoItemHandleEx(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL,albumViewData->nCurItem,1);
			if(hPhotoItem == NULL)
				return;
			
			pPhotoItem = AP_VideoItemGetVideoItemFromHandle(hPhotoItem);
			if(pPhotoItem == NULL)
				return;
			
			// 获取照片的文件名
			if(XM_VideoItemGetVideoFilePath(pPhotoItem, replay_ch, text, sizeof(text)))
			{
				XM_printf(">>>>>>>>>>text:%s............\r\n", text);
				// 将照片解码并显示在OSD 0层(视频层)
				if(decode_jpeg_file(text))
				{
					// 解码错误
					albumViewData->photo_damaged = 1;
				}
				else
				{
					albumViewData->photo_damaged = 0;
				}
			}
			XM_SetTimer(XMTIMER_ALBUMLISTVIEW, ALBUM_BUTTON_AUTO_HIDE_TIMEOUT);
			// 刷新与窗口关联的OSD层
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;
			
		case VK_AP_MENU://MENU键,退出播放页面
			XM_printf(">>>>>>>>>>>>VK_AP_MENU...........\r\n");

		    XMSYS_H264CodecPlayerStop();
            XM_PullWindow(0);
			break;
			
		default:
			break;
	}

}

static VOID AlbumViewOnKeyUp (XMMSG *msg)
{
	ALBUMVIEWDATA *albumViewData;
	albumViewData = (ALBUMVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumView));
	if(albumViewData == NULL)
		return;
	
	XM_printf(">>>>>>>>>>>>>>>AlbumViewOnKeyUp, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
}

static VOID DeleteOneCallback (VOID *UserPrivate, UINT uKeyPressed)
{
	ALBUMVIEWDATA *albumViewData = UserPrivate;
	if(uKeyPressed == VK_AP_MENU)
	{
		// 确认删除
		albumViewData->delete_one_confirm = 1;
	}
	// 将alert弹出
	XM_PullWindow (0);
}

static VOID DeleteAllCallback (VOID *UserPrivate, UINT uKeyPressed)
{
	ALBUMVIEWDATA *albumViewData = UserPrivate;
	if(uKeyPressed == VK_AP_MENU)
	{
		// 确认删除
		albumViewData->delete_all_confirm = 1;
	}
	XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0); //解决从图片预览返回图片列表，OSD_LAYER_0仍有图片数据
	// 将alert弹出
	XM_PullWindow (0);
}

static VOID AlbumViewOnCommand (XMMSG *msg)
{
	ALBUMVIEWDATA *albumViewData;
	albumViewData = (ALBUMVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumView));
	if(albumViewData == NULL)
		return;

	XM_printf ("AlbumViewOnCommand msg->wp=%d\n", msg->wp);
#if 0
	switch (msg->wp)
	{
		case ALBUMVIEW_COMMAND_RETURN:
			XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0); //解决从图片预览返回图片列表，OSD_LAYER_0仍有图片数据
			// 返回到桌面
			XM_PullWindow (0);
			break;

		case ALBUMVIEW_COMMAND_DELETE_ALL:
		{
			// 删除全部图片
			DWORD dwButtonNormalTextID[2];
			DWORD dwButtonPressedTextID[2];
			dwButtonNormalTextID[0] = AP_ID_VIDEO_BUTTON_DELETE_YES;
			dwButtonPressedTextID[0] = AP_ID_VIDEO_BUTTON_DELETE_YES;
			dwButtonNormalTextID[1] = AP_ID_VIDEO_BUTTON_CANCEL_NO;
			dwButtonPressedTextID[1] = AP_ID_VIDEO_BUTTON_CANCEL_NO;

			albumViewData->delete_all_confirm = 0;

			XM_OpenAlertView (AP_ID_VIDEO_INFO_TO_DELETE_ALL_PHOTO,
										AP_NULLID,
										2,
										dwButtonNormalTextID,
										dwButtonPressedTextID,
										APP_ALERT_BKGCOLOR,
										(float)20.0,		// 自动关闭时间
										APP_ALERT_BKGALPHA,
										DeleteAllCallback,			// 按键回调函数
										albumViewData,
										XM_VIEW_ALIGN_CENTRE,		// 居中对齐
										XM_ALERTVIEW_OPTION_ENABLE_CALLBACK		// ALERTVIEW视图的控制选项
										);
			break;
		}
		
		case ALBUMVIEW_COMMAND_DELETE_ONE:
		{
			// 删除当前图片
			DWORD dwButtonNormalTextID[2];
			DWORD dwButtonPressedTextID[2];
			dwButtonNormalTextID[0] = AP_ID_VIDEO_BUTTON_DELETE_YES;
			dwButtonPressedTextID[0] = AP_ID_VIDEO_BUTTON_DELETE_YES;
			dwButtonNormalTextID[1] = AP_ID_VIDEO_BUTTON_CANCEL_NO;
			dwButtonPressedTextID[1] = AP_ID_VIDEO_BUTTON_CANCEL_NO;

			albumViewData->delete_one_confirm = 0;

			XM_OpenAlertView (AP_ID_VIDEO_INFO_TO_DELETE_PHOTO,
										AP_NULLID,
										2,
										dwButtonNormalTextID,
										dwButtonPressedTextID,
										APP_ALERT_BKGCOLOR,
										(float)20.0,		// 自动关闭时间
										APP_ALERT_BKGALPHA,
										DeleteOneCallback,			// 按键回调函数
										albumViewData,
										XM_VIEW_ALIGN_CENTRE,		// 居中对齐
										XM_ALERTVIEW_OPTION_ENABLE_CALLBACK		// ALERTVIEW视图的控制选项
										);
			break;
		}

		default:
			break;
	}
#endif
}

// 用于无操作后将主按键隐藏, 缺省为3秒
static VOID AlbumViewOnTimer (XMMSG *msg)
{
	ALBUMVIEWDATA *albumViewData;
	albumViewData = (ALBUMVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(AlbumView));
	if(albumViewData == NULL)
		return;
	if(!albumViewData->button_hide)
	{
		XM_KillTimer (XMTIMER_ALBUMLISTVIEW);
		albumViewData->button_hide = 1;
		albumViewData->photo_title_hide = 1;

		AP_ButtonControlSetFlag (&albumViewData->btnControl,
					XMBUTTON_FLAG_HIDE);
		XM_InvalidateWindow ();

		//XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_1), (1 << XM_OSD_LAYER_1) );
		//XM_lcdc_osd_set_enable (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1, 0);
		//XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_1), 0);
	}
	else if(!albumViewData->photo_title_hide)
	{
		XM_KillTimer (XMTIMER_ALBUMLISTVIEW);
		albumViewData->photo_title_hide = 1;		// 隐藏照片的标题
		XM_InvalidateWindow ();
	}
}

static VOID AlbumViewOnSystemEvent (XMMSG *msg)
{
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:			// SD卡拔出事件
			XM_BreakSystemEventDefaultProcess (msg);		// 不进行缺省处理
			XM_JumpWindowEx(XMHWND_HANDLE(Desktop), 0, 0);
			break;
	}
}

// 用户私有消息处理
static VOID AlbumViewOnUserMessage (XMMSG *msg)
{
#if 0
	if(msg->wp == ALBUMVIEW_COMMAND_RETURN)
	{
		// 返回到调用者视窗
		XM_PullWindow (0);
	}
	else if(msg->wp == ALBUMVIEW_COMMAND_PAINT)
	{
		XM_InvalidateWindow ();
	}
#endif
}


// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (AlbumView)
	XM_ON_MESSAGE (XM_PAINT, AlbumViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, AlbumViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, AlbumViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, AlbumViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, AlbumViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, AlbumViewOnCommand)
	XM_ON_MESSAGE (XM_TIMER, AlbumViewOnTimer)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, AlbumViewOnSystemEvent)
	XM_ON_MESSAGE (XM_USER, AlbumViewOnUserMessage)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, AlbumView, 0, 0, 0, 0, HWND_VIEW)
