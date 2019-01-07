//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_videolistview.c
//	  视频列表浏览窗口
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************

#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"
#include "system_check.h"
#include "xm_h264_codec.h"
#include "gpio.h"
#include "app_imageloader.h"

DWORD gcur_ticket;
DWORD glast_ticket;

//计算某段程序运行时间
#define  Start_Calculate do{gcur_ticket = XM_GetTickCount();}while(0)
#define  End_Calculate do{glast_ticket = XM_GetTickCount();XM_printf(">>>cur ticket:%d, last ticket:%d\r\n", gcur_ticket, glast_ticket);}while(0)

extern int XMSYS_FileManagerFileDelete (unsigned char channel, unsigned char type, unsigned char file_name[8]);
extern void rxchip_set_delay_data(u8_t val);

#define Thumb_width 304
#define Thumb_height 208
#define Thumb_Hor_Interval 28
#define Thumb_Vert_Interval 28

u8_t update_all_videolist = FALSE;
u8_t videolist_buffer_vaild_flag[3][6] = { {0,0,0,0,0,0}, {0,0,0,0,0,0}, {0,0,0,0,0,0}};
XMVIDEOITEM *pVideoItem_buffer[3][6] = { {NULL, NULL, NULL, NULL, NULL, NULL}, {NULL, NULL, NULL, NULL, NULL, NULL}, {NULL, NULL, NULL, NULL, NULL, NULL}};
__no_init static unsigned char Page_videolist_buffer[3*6*(Thumb_width*Thumb_height*3/2)];		// 显示临时缓冲区

static const int file_error_offset[] = {ROM_T18_LANG_CHN_VIDEO_FILE_ERROR_PNG};
static const int file_error_length[] = {ROM_T18_LANG_CHN_VIDEO_FILE_ERROR_PNG_SIZE};
static XM_IMAGE *file_error_image[1];
static u8_t file_error_flag[3][6] = { {0,0,0,0,0,0}, {0,0,0,0,0,0}, {0,0,0,0,0,0}};//文件损坏标志
static u8_t video_buffer_pos = 1;
static u8_t video_buffer_pos_pagenum[3] = {0,0,0};
static u8_t load_update_flag = FALSE;

typedef struct tagVIDEOLISTVIEWLISTDATA {
	int					nPageNum;					// 当前page
	int					nCurItem;					// 当前选择的菜单项,0-5

	int					nItemCount;					// 菜单项个数,总数
	int					nVisualCount;				// 视窗可显示的项目数量
	int					nTotalPage;					// 总page;
	
	BYTE				videotype;					//类型,视频或者图片
	
    HANDLE				hMoniorSwitchControl;		//停车监控
	char				fileName[XM_MAX_FILEFIND_NAME];		// 保存文件查找过程的文件名
	XMFILEFIND			fileFind;
} VIDEOLISTVIEWLISTDATA;


void set_update_all_videolist_flag(u8_t flg)
{
	update_all_videolist = flg;
}

// 设置录像列表视窗的聚焦项
XMBOOL AP_VideoListViewSetFocusItem (UINT uFocusItem)
{
	VIDEOLISTVIEWLISTDATA *videoListViewListData;
	videoListViewListData = (VIDEOLISTVIEWLISTDATA *)XM_GetWindowPrivateData(XMHWND_HANDLE(VideoListView));
	if(videoListViewListData)
	{
		videoListViewListData->nCurItem = (SHORT)uFocusItem;
		return 1;
	}
	return 0;
}

void load_page_video(u8_t pos, int curpage)//加载到指定位置page
{
	HANDLE hVideoItem;
	XMVIDEOITEM *pVideoItem;
	int ch,item;
	u8_t i;
	char text[64];
	
	VIDEOLISTVIEWLISTDATA *videoListViewListData = (VIDEOLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoListView));
	if(videoListViewListData == NULL)
		return;	

	//加载指定page
	for(i = 0; i < 6; i++)
	{
		HANDLE hVideoItem;
		XMVIDEOITEM *pVideoItem;
		int ch;

	    item = (curpage-1)*6 + i;

		XM_printf(">>>>>load item:%d\r\n", item);
		if(item >= videoListViewListData->nItemCount)
			break;
		
		if(videoListViewListData->nItemCount <= 0)
			break;

		//获取视频项文件名
		unsigned int replay_ch;
		if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
		{
			replay_ch = XM_VIDEO_CHANNEL_0;
		}
		else
		{
			replay_ch = XM_VIDEO_CHANNEL_1;
		}
		hVideoItem = XM_VideoItemGetVideoItemHandleEx(0,replay_ch,XM_FILE_TYPE_VIDEO,XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL,item,1);

		//hVideoItem = AP_VideoItemGetVideoItemHandleEx(videoListViewListData->videotype, item, replay_ch);
		if(hVideoItem == NULL)
		{
			XM_printf(">>>>error2 ..............\r\n");
			break;
		}
		pVideoItem = AP_VideoItemGetVideoItemFromHandle(hVideoItem);
		if(pVideoItem == NULL)
		{
			XM_printf(">>>>error1 ..............\r\n");
			break;
		}
		pVideoItem_buffer[pos][i] = pVideoItem;
		// 视频预览图片获取
		if(XM_VideoItemGetVideoFilePath(pVideoItem, replay_ch, text, sizeof(text)))
		{
			unsigned int w = Thumb_width;
			unsigned int h = Thumb_height;
			unsigned int yuvstride = Thumb_width;
			unsigned int imagestride;
			unsigned char *thumb_image;
			//XM_IMAGE *lpImage = NULL;

			imagestride = w;
			imagestride *= 4;

			thumb_image = &Page_videolist_buffer[(pos*6*(Thumb_width*Thumb_height*3/2))+i*(Thumb_width*Thumb_height*3/2)];	
			//lpImage = (XM_IMAGE *)kernel_malloc (imagestride * h + sizeof(XM_IMAGE) - 4);
			//thumb_image = (unsigned char *)kernel_malloc (yuvstride * h * 3/2);
			//thumb_image = &(videolist_buffer[pos][i][Thumb_width*Thumb_height*3/2]);
			
            if(thumb_image)
			{
				unsigned char *image[3];
				
				image[0] = thumb_image;
				image[1] = thumb_image+yuvstride * h;
				image[2] = 0;
				if(h264_decode_avi_stream_thumb_frame(text, w, h, yuvstride, image) == 0)//解码把数据放到videolist_buffer中
				{
					XM_printf(">>>>>load file ok............\r\n");
				}
				else
				{
					XM_printf(">>>>>load file error............\r\n");
					file_error_flag[pos][i] = 1;
				}	

				videolist_buffer_vaild_flag[pos][i] = TRUE;

				//kernel_free (thumb_image);
				//kernel_free (lpImage);
			}
		}
		else
		{
			XM_printf(">>>>get video error ..............\r\n");
		}
	}
}

void hw_backlight_set_auto_off_ticket (unsigned int ticket);
static VOID VideoListViewOnEnter(XMMSG *msg)
{
	unsigned int replay_ch;
	VIDEOLISTVIEWLISTDATA *videoListViewListData;
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>VideoListViewOnEnter, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

	if(msg->wp == 0)
	{
		XMRECT rc;
		int nVisualCount;
		// 窗口未建立，第一次进入
		BYTE mode = VIDEOLIST_ALL_VIDEO_MODE;

		// 分配私有数据句柄
		videoListViewListData = XM_calloc(sizeof(VIDEOLISTVIEWLISTDATA));
		if(videoListViewListData == NULL)
		{
			XM_printf("videoListViewListData XM_calloc failed\n");
			// 失败返回到调用者窗口
			XM_PullWindow (0);
			return;
		}

		XMSYS_H264CodecRecorderStop();//关闭数据通道
		update_all_videolist = FALSE;
		videoListViewListData->videotype = VIDEOITEM_CIRCULAR_VIDEO_MODE;//循环录像模式
		
		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData(XMHWND_HANDLE(VideoListView), videoListViewListData);

		//开始扫描所有符合条件的视频文件
		videoListViewListData->nPageNum = get_video_curpage_num();
		videoListViewListData->nCurItem = 0;
		
		//根据通道,实时获取总视频文件数量，计算总page数量
		if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
		{
			replay_ch = XM_VIDEO_CHANNEL_0;
		}
		else
		{
			replay_ch = XM_VIDEO_CHANNEL_1;
		}
		videoListViewListData->nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_VIDEO,XM_VIDEOITEM_CLASS_BITMASK_ALL);

		videoListViewListData->nTotalPage = videoListViewListData->nItemCount/6;
		
		if(videoListViewListData->nItemCount%6!=0)
		{
			videoListViewListData->nTotalPage += 1;
		}
		XM_printf(">>>>>>>>VideoListViewOnEnter,videoListViewListData->nPageNum:%d\r\n", videoListViewListData->nPageNum);
		XM_printf(">>>>>>>>VideoListViewOnEnter,videoListViewListData->nTotalPage:%d\r\n", videoListViewListData->nTotalPage);
		//计算可以显示的项目数量
		XM_GetDesktopRect(&rc);
		nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
		nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
        nVisualCount -= 1;

		if(videoListViewListData->nItemCount>=nVisualCount)
		{
			videoListViewListData->nVisualCount = (SHORT)nVisualCount;
		}
		else
		{
            videoListViewListData->nVisualCount=videoListViewListData->nItemCount;
		}
		file_error_image[0] = XM_RomImageCreate(file_error_offset[0], file_error_length[0], XM_OSD_LAYER_FORMAT_ARGB888);
	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复		
		videoListViewListData = (VIDEOLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoListView));
		if(videoListViewListData == NULL)
			return;

		//切换page默认选中第一个
		videoListViewListData->nCurItem = 0;
		XM_printf(">>>>videoListViewListData->nPageNum:%d\r\n", videoListViewListData->nPageNum);

		XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>VideoListView Pull.............\r\n");
	}
	// 创建定时器，用于卡拔出检测
	// 创建0.5秒的定时器
	hw_backlight_set_auto_off_ticket(0xFFFFFFFF);//关闭屏保，在录像模式再打开
	XM_SetTimer(XMTIMER_VIDEOLISTVIEW, 1500);
}

VOID VideoListViewOnLeave (XMMSG *msg)
{
	// 删除定时器
	XM_printf(">>>>>>VideoListViewOnLeave %d......\r\n", msg->wp);
	XM_KillTimer(XMTIMER_VIDEOLISTVIEW);

	if(msg->wp == 0)
	{
		XMSYS_H264CodecRecorderStart();
		// 窗口退出，彻底摧毁。
		// 获取私有数据句柄
		VIDEOLISTVIEWLISTDATA *videoListViewListData = (VIDEOLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoListView));
		// 释放所有分配的资源
		if(videoListViewListData)
		{
			// 释放私有数据句柄
			XM_free (videoListViewListData);
			// 设置视窗的私有数据句柄为空
			XM_SetWindowPrivateData(XMHWND_HANDLE(VideoListView), NULL);
		}
		
		if(file_error_image[0])
		{
			XM_ImageDelete(file_error_image[0]);
			file_error_image[0] = NULL;
		}
		rxchip_set_delay_data(2);
		//OS_Delay(1000);
		XM_printf(">>>>>>VideoListView Exit......\r\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf(">>>>>>VideoListView Push......\r\n");
	}
}

VOID VideoListViewOnPaint(XMMSG *msg)
{
	XMRECT rc, rect;
	int i, item, page;
	XMCOORD x, y;
	//DWORD dwFileAttribute;
	unsigned int old_alpha;
	HANDLE hWnd;
	int nVisualCount;
	int diff_val;
	char text[64];
	char String[32];
	XMSIZE size;
	XMSYSTEMTIME filetime;
	unsigned int replay_ch;
	DWORD MenuID;

	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>VideoListViewOnPaint......\r\n");
	VIDEOLISTVIEWLISTDATA *videoListViewListData = (VIDEOLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoListView));
	if(videoListViewListData == NULL)
		return;
	
	hWnd = XMHWND_HANDLE(VideoListView);
	XM_GetDesktopRect (&rc);

	#if 0
	if(videoListViewListData->nItemCount <= 0)
	{
		XM_PullWindow(0);
	}
	#endif
	old_alpha = XM_GetWindowAlpha (hWnd);
	XM_SetWindowAlpha(hWnd, 255);

	//根据通道,实时获取总视频文件数量，计算总page数量
	if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
	{
		MenuID = AP_ID_VIDEO_BUTTON_CH1;
		replay_ch = XM_VIDEO_CHANNEL_0;
	}
	else
	{
		MenuID = AP_ID_VIDEO_BUTTON_CH2;
		replay_ch = XM_VIDEO_CHANNEL_1;
	}
	videoListViewListData->nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_VIDEO,XM_VIDEOITEM_CLASS_BITMASK_ALL);

	videoListViewListData->nTotalPage = videoListViewListData->nItemCount/6;
	if(videoListViewListData->nItemCount%6!=0)
	{
		videoListViewListData->nTotalPage += 1;
	}		
	
	XM_FillRect(hWnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));
	if(update_all_videolist==FALSE)//仅在page改变时候才全部更新
	{
		update_all_videolist = TRUE;
		for(page=0; page<3; page++)
		{
			for(i=0; i<6; i++)
			{
				videolist_buffer_vaild_flag[page][i] = FALSE;
				file_error_flag[page][i] = FALSE;
				pVideoItem_buffer[page][i] = NULL;
			}
			video_buffer_pos_pagenum[page] = 0;
		}
		video_buffer_pos = 1;
		load_update_flag = TRUE;
		video_buffer_pos_pagenum[video_buffer_pos]= videoListViewListData->nPageNum;
	}	
	
	// --------------------------------------
	//
	// ********* 1 显示标题栏区信息 *********
	//
	// --------------------------------------
	rect.left = 0;rect.top = 0;rect.right = 1024;rect.bottom = 42;
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUTITLEBARBACKGROUND, hWnd, &rc, XMGIF_DRAW_POS_LEFTTOP);

	rect.left = 20;rect.top = 3;rect.right = 65;rect.bottom = 36;
	AP_RomImageDrawByMenuID(AP_ID_VIDEO_TITLE_VIDEO, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//类型:视频/图片
	
	rect.left = 450;rect.top = 3;rect.right = 65;rect.bottom = 36;
	AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_CH, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//通道
	rect.left = 515;rect.top = 3;rect.right = 65;rect.bottom = 36;
	AP_RomImageDrawByMenuID(MenuID, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//CH1/CH2

	rect.left = 930;rect.top = 3;rect.right = 65;rect.bottom = 36;
    sprintf(String,"%d%s%d", videoListViewListData->nPageNum, "/", videoListViewListData->nTotalPage);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString(hWnd, rect.left, rect.top, String, strlen(String));
		
	// 按5行显示视频列表
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
	
	//显示选择框,根据当前nCurItem调整选中框,相当于聚焦位置
	diff_val = videoListViewListData->nCurItem;
	if(videoListViewListData->nCurItem!=-1 && videoListViewListData->nItemCount >0)
	{   
		if(diff_val == 0)
		{
			rect.left = Thumb_Hor_Interval-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y);
		}
		else if(diff_val == 1)
		{
		    rect.left = Thumb_Hor_Interval+Thumb_Hor_Interval+Thumb_width-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y);
		}
		else if(diff_val == 2)
		{
			rect.left = Thumb_Hor_Interval+Thumb_Hor_Interval+Thumb_width+Thumb_Hor_Interval+Thumb_width-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y);
		}
		if(diff_val == 3)
		{
			rect.left = Thumb_Hor_Interval-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + Thumb_height+Thumb_Vert_Interval+50);
		}
		else if(diff_val == 4)
		{
		    rect.left = Thumb_Hor_Interval+Thumb_Hor_Interval+Thumb_width-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + Thumb_height+Thumb_Vert_Interval+50);
		}
		else if(diff_val == 5)
		{
			rect.left = Thumb_Hor_Interval+Thumb_Hor_Interval+Thumb_width+Thumb_Hor_Interval+Thumb_width-10;
			rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + Thumb_height+Thumb_Vert_Interval+50);
		}
		rect.right = rect.left+Thumb_width+8+10;
		rect.bottom = rect.top +Thumb_height+8+10;
		XM_FillRect(hWnd,rect.left,rect.top,rect.right,rect.bottom,XM_RGB(0,255,0));
	}
	
	// 显示目录项
	x = Thumb_Hor_Interval;
	y = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 10);

	//XM_printf(">>>>video_buffer_pos:%d\r\n", video_buffer_pos);
	//XM_printf(">>>>video_buffer_pos_pagenum[0]:%d\r\n", video_buffer_pos_pagenum[0]);
	//XM_printf(">>>>video_buffer_pos_pagenum[1]:%d\r\n", video_buffer_pos_pagenum[1]);
	//XM_printf(">>>>video_buffer_pos_pagenum[2]:%d\r\n", video_buffer_pos_pagenum[2]);

	
	//根据pagenum显示page
	for(i = 0; i < 6; i++)
	{
		HANDLE hVideoItem;
		XMVIDEOITEM *pVideoItem;
		int ch;

	    item = (videoListViewListData->nPageNum-1)*6 + i;

		//XM_printf(">>>>>item:%d\r\n", item);
		if(item >= videoListViewListData->nItemCount)
			break;
		
		if(videoListViewListData->nItemCount <= 0)
			break;

		if(videolist_buffer_vaild_flag[video_buffer_pos][i]==FALSE)
		{
			//获取视频项文件名
			unsigned int replay_ch;
			if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
			{
				replay_ch = XM_VIDEO_CHANNEL_0;
			}
			else
			{
				replay_ch = XM_VIDEO_CHANNEL_1;
			}
			hVideoItem = XM_VideoItemGetVideoItemHandleEx(0,replay_ch,XM_FILE_TYPE_VIDEO,XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL,item,1);

			//hVideoItem = AP_VideoItemGetVideoItemHandleEx(videoListViewListData->videotype, item, replay_ch);
			if(hVideoItem == NULL)
			{
				XM_printf(">>>>error2 ..............\r\n");
				break;
			}
			
			pVideoItem = AP_VideoItemGetVideoItemFromHandle(hVideoItem);
			if(pVideoItem == NULL)
			{
				XM_printf(">>>>error1 ..............\r\n");
				break;
			}
			pVideoItem_buffer[video_buffer_pos][i] = pVideoItem;
			// 视频预览图片获取
			if(XM_VideoItemGetVideoFilePath(pVideoItem, replay_ch, text, sizeof(text)))
			{
				unsigned int w = Thumb_width;
				unsigned int h = Thumb_height;
				unsigned int yuvstride = Thumb_width;
				unsigned int imagestride;
				unsigned char *thumb_image;
				XM_IMAGE *lpImage = NULL;

				imagestride = w;
				imagestride *= 4;
				
				lpImage = (XM_IMAGE *)kernel_malloc (imagestride * h + sizeof(XM_IMAGE) - 4);
				//thumb_image = (unsigned char *)kernel_malloc (yuvstride * h * 3/2);

				thumb_image = &Page_videolist_buffer[(video_buffer_pos*6*(Thumb_width*Thumb_height*3/2))+i*(Thumb_width*Thumb_height*3/2)];

	            if(thumb_image)
				{
					unsigned char *image[3];
					image[0] = thumb_image;
					image[1] = thumb_image+yuvstride * h;
					image[2] = 0;

					if(h264_decode_avi_stream_thumb_frame(text, w, h, yuvstride, image) == 0)//解码把数据放到videolist_buffer中
					{
						//XM_printf(">>>>>file ok............\r\n");
						XM_ImageConvert_Y_UV420_to_ARGB(thumb_image,(unsigned char *)lpImage->image,w,h);
						lpImage->id = XM_IMAGE_ID;
						lpImage->format = XM_OSD_LAYER_FORMAT_ARGB888;
						lpImage->stride = (unsigned short)imagestride;
						lpImage->width = w;
						lpImage->height = h;
						lpImage->ayuv = NULL;

						XM_ImageDisplay(lpImage, hWnd, x, y);
					}
					else
					{
						file_error_flag[video_buffer_pos][i] = 1;
						XM_ImageDisplay(file_error_image[0], hWnd, x, y);
						//XM_printf(">>>>>file error............\r\n");
					}	

					videolist_buffer_vaild_flag[video_buffer_pos][i] = TRUE;
					//显示时间
					XM_GetDateTimeFromFileName ((char *)pVideoItem->video_name, &filetime);
					sprintf (text, "%04d/%02d/%02d %02d:%02d:%02d", filetime.wYear, 
																	filetime.bMonth,
																	filetime.bDay,
																	filetime.bHour,
																	filetime.bMinute,
																	filetime.bSecond);

					AP_TextGetStringSize (text, strlen(text), &size);
					AP_TextOutDataTimeString (hWnd, x, y + Thumb_height +10, text, strlen(text));

					//kernel_free (thumb_image);
					kernel_free (lpImage);
				}
			}
			else
			{
				XM_printf(">>>>error ..............\r\n");
			}
		}
		else
		{//已解码好数据,直接转换进行显示
			//XM_printf(">>>>>1 file ok............\r\n");
			if(file_error_flag[video_buffer_pos][i]==0)
			{//文件OK
				unsigned int w = Thumb_width;
				unsigned int h = Thumb_height;
				unsigned int yuvstride = Thumb_width;
				unsigned int imagestride;
				unsigned char *thumb_image;
				XM_IMAGE *lpImage = NULL;
				unsigned char *ycbcr;
				
				imagestride = w;
				imagestride *= 4;
				
				lpImage = (XM_IMAGE *)kernel_malloc (imagestride * h + sizeof(XM_IMAGE) - 4);

				ycbcr = &Page_videolist_buffer[(video_buffer_pos*6*(Thumb_width*Thumb_height*3/2))+i*(Thumb_width*Thumb_height*3/2)];

				//DWORD cur_ticket = XM_GetTickCount();
				XM_ImageConvert_Y_UV420_to_ARGB(ycbcr,(unsigned char *)lpImage->image,w,h);//消耗100ms
				//DWORD last_ticket = XM_GetTickCount();

				//XM_printf(">>>>>2 file ok............\r\n");
				lpImage->id = XM_IMAGE_ID;
				lpImage->format = XM_OSD_LAYER_FORMAT_ARGB888;
				lpImage->stride = (unsigned short)imagestride;
				lpImage->width = w;
				lpImage->height = h;
				lpImage->ayuv = NULL;
				
				//Start_Calculate;
				XM_ImageDisplay(lpImage, hWnd, x, y);
				//End_Calculate;

				kernel_free(lpImage);
			}
			else
			{//文件损坏
				XM_ImageDisplay(file_error_image[0], hWnd, x, y);
				XM_printf(">>>>>file error............\r\n");
			}

			//Start_Calculate;
			//显示时间
			XM_GetDateTimeFromFileName ((char *)pVideoItem_buffer[video_buffer_pos][i]->video_name, &filetime);
			sprintf(text, "%04d/%02d/%02d %02d:%02d:%02d", filetime.wYear, 
															filetime.bMonth,
															filetime.bDay,
															filetime.bHour,
															filetime.bMinute,
															filetime.bSecond);

			AP_TextGetStringSize (text, strlen(text), &size);
			AP_TextOutDataTimeString (hWnd, x, y + Thumb_height +10, text, strlen(text));
			//End_Calculate;
		}

		x = (XMCOORD)(x + Thumb_width + Thumb_Hor_Interval);		
		if(i==2)
		{
		   	x=Thumb_Hor_Interval;
			y=(XMCOORD)(y + Thumb_height+Thumb_Vert_Interval+50);
		}
	}
	
	XM_SetWindowAlpha (hWnd, (unsigned char)old_alpha);
}

VOID VideoListViewOnKeyDown (XMMSG *msg)
{
	VIDEOLISTVIEWLISTDATA *videoListViewListData = (VIDEOLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoListView));

    if( AP_GetMenuItem(APPMENUITEM_POWER_STATE) == POWER_STATE_OFF)
    {
        return;
    }

	if(videoListViewListData == NULL)
		return;
	
	XM_printf(">>>>>>>>VideoListViewOnKeyDown, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	// 按键音
	XM_Beep(XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
	    case REMOTE_KEY_MENU:
		case VK_AP_MENU://MENU键,进入回放模式菜单
			XM_printf(">>>>>>>VK_AP_MENU\r\n");
            XM_PushWindowEx(XMHWND_HANDLE(PlayBackMenu),(DWORD)videoListViewListData);//回放菜单,压栈方式数据还未释放
			break;
			
        case REMOTE_KEY_DOWN:
		case VK_AP_SWITCH://OK键
			XM_printf(">>>>>>>VK_AP_MODE\r\n");
			if(!file_error_flag[video_buffer_pos][videoListViewListData->nCurItem])
			{
				if(videoListViewListData->nItemCount > 0)
				{	
					// 播放
					AP_OpenVideoView(videoListViewListData->videotype, (WORD)((videoListViewListData->nPageNum-1)*6+videoListViewListData->nCurItem));
				}
			}
			break;
			
		case REMOTE_KEY_LEFT:
		case VK_AP_DOWN://-键		
			XM_printf(">>>>>>>VK_AP_DOWN\r\n");
			if(videoListViewListData->nItemCount <= 0)
				return;

			//每页6个视频文件,通过item调整,改变page
			if(videoListViewListData->nCurItem==0)
			{
				if(videoListViewListData->nPageNum==1)
				{
					videoListViewListData->nPageNum = videoListViewListData->nTotalPage;
		
					if(videoListViewListData->nItemCount%6==0)
					{
						videoListViewListData->nCurItem = 5;
					}
					else
					{
						videoListViewListData->nCurItem = videoListViewListData->nItemCount%6-1;
					}
					//update_all_videolist = FALSE;
					load_update_flag = TRUE;
				}
				else
				{
					videoListViewListData->nPageNum--;
					videoListViewListData->nCurItem = 5;
					//update_all_videolist = FALSE;
					if(video_buffer_pos==0)
					{
						video_buffer_pos = 2;
					}
					else
					{
						video_buffer_pos--;
					}
					load_update_flag = TRUE;
				}
			}
			else
			{
				videoListViewListData->nCurItem--;
			}		
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;
			
		case REMOTE_KEY_RIGHT:
		case VK_AP_UP:////+键	选中下一个框
			XM_printf(">>>>>>>VK_AP_UP\r\n");
			if(videoListViewListData->nItemCount <= 0)
				return;

			//每页6个视频文件,通过item调整,改变page
			videoListViewListData->nCurItem ++;
			if(((videoListViewListData->nPageNum-1)*6+videoListViewListData->nCurItem) < videoListViewListData->nItemCount)
			{
				if(videoListViewListData->nCurItem==videoListViewListData->nVisualCount)
				{
					videoListViewListData->nCurItem = 0;
					videoListViewListData->nPageNum++;
					//update_all_videolist = FALSE;
					video_buffer_pos++;
					if(video_buffer_pos>2)
					{
						video_buffer_pos= 0;
					}
					load_update_flag = TRUE;
				}
			}
			else
			{
				videoListViewListData->nPageNum = 1;
				videoListViewListData->nCurItem = 0;
				load_update_flag = TRUE;
				//update_all_videolist = FALSE;
			}
			
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case REMOTE_KEY_UP:
        case VK_AP_FONT_BACK_SWITCH://AV键，退出回放模式
			XM_printf(">>>>>>>VK_AP_FONT_BACK_SWITCH\r\n");
			XM_JumpWindow(XMHWND_HANDLE(Desktop));
			if(videoListViewListData->nItemCount <= 0)
				return;

            break;

		default:
			break;
	}
}


VOID VideoListViewOnKeyUp (XMMSG *msg)
{
	XM_printf(">>>>>>>>VideoListViewOnKeyUp, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
}


VOID VideoListViewOnTimer (XMMSG *msg)
{
	u8_t pre_pagenum; 
	u8_t next_pagenum;
	u8_t next_pos;
	u8_t pre_pos;

	VIDEOLISTVIEWLISTDATA *videoListViewListData = (VIDEOLISTVIEWLISTDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoListView));
	if(videoListViewListData == NULL)
		return;

	if(load_update_flag==TRUE)
	{
		load_update_flag = FALSE;
		XM_printf(">>>>video_buffer_pos:%d\r\n", video_buffer_pos);
		XM_printf(">>>>cur page num:%d\r\n", videoListViewListData->nPageNum);
		
		next_pos = video_buffer_pos + 1;
		pre_pos = video_buffer_pos - 1;
		if(next_pos>2) next_pos = 0;
		if(pre_pos>3) pre_pos = 2;
		
		XM_printf(">>>>next pos:%d\r\n", next_pos);
		XM_printf(">>>>video_buffer_pos_pagenum[next_pos]:%d\r\n", video_buffer_pos_pagenum[next_pos]);

		next_pagenum = videoListViewListData->nPageNum+1;
		if(next_pagenum>videoListViewListData->nTotalPage)
		{
			next_pagenum = 1;
		}

		XM_printf(">>>>next page num:%d\r\n", next_pagenum);
		if(video_buffer_pos_pagenum[next_pos]!=next_pagenum)
		{
			load_page_video(next_pos, next_pagenum);
			video_buffer_pos_pagenum[next_pos] = next_pagenum;	
		}	

		XM_printf(">>>>pre pos:%d\r\n", pre_pos);
		XM_printf(">>>>video_buffer_pos_pagenum[pre_pos]:%d\r\n", video_buffer_pos_pagenum[pre_pos]);
		if(videoListViewListData->nPageNum==1)
		{
			pre_pagenum = videoListViewListData->nTotalPage;
		}
		else
		{
			pre_pagenum = videoListViewListData->nPageNum-1;
		}
		XM_printf(">>>>pre page num:%d\r\n", pre_pagenum);
		if(video_buffer_pos_pagenum[pre_pos]!=pre_pagenum)
		{
			load_page_video(pre_pos, pre_pagenum);
			video_buffer_pos_pagenum[pre_pos] = pre_pagenum;	
		}			
	}
}

static VOID VideoListViewOnSystemEvent (XMMSG *msg)
{
	XM_printf(">>>>>>>>VideoListViewOnSystemEvent, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:			// SD卡拔出事件
			XM_printf(">>>>>VideoListViewOnSystemEvent,1\r\n");
			XM_BreakSystemEventDefaultProcess (msg);		// 不进行缺省处理

			#if 0
			// 将该消息重新插入到消息队列队首
			XM_InsertMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
			XM_JumpWindowEx (XMHWND_HANDLE(Desktop), HWND_CUSTOM_DEFAULT, XM_JUMP_POPDEFAULT);
			//XM_JumpWindow (XMHWND_HANDLE(Desktop));	// 跳转到桌面
			#endif
			//XM_PullWindow(0);					   // 弹出当前的列表视窗
			XM_printf(">>>>>VideoListViewOnSystemEvent,2\r\n");
			XM_JumpWindow(XMHWND_HANDLE(Desktop));
			break;

		default:
			break;
	}
}

// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (VideoListView)
	XM_ON_MESSAGE (XM_PAINT, VideoListViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, VideoListViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, VideoListViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, VideoListViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, VideoListViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, VideoListViewOnTimer)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, VideoListViewOnSystemEvent)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, VideoListView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
