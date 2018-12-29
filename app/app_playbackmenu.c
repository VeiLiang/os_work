//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_FormatSetting.c
//	  通用Menu选项列表选择窗口(只能选择一个选项)
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuid.h"
#include "app_menuoptionview.h"
#include "types.h"

#define TYPE_VIDEO		0
#define TYPE_PIC		1

#define REPLAYMENU_ITEM0_TYPE	0
#define REPLAYMENU_ITEM1_CH		1
#define REPLAYMENU_ITEM2_PAGE	2
#define REPLAYMENU_ITEM3_DEL	3
#define REPLAYMENU_ITEM3_DELALL	4

struct _tagVIDEOLIST{
	int					nPageNum;					// 当前page
	int					nCurItem;					// 当前选择的菜单项,0-5

	int					nItemCount;					// 菜单项个数,总数
	int					nVisualCount;				// 视窗可显示的项目数量
	int					nTotalPage;					// 总page;
	
	BYTE				videotype;					//类型,视频或者图片
	
    HANDLE				hMoniorSwitchControl;		//停车监控
	char				fileName[XM_MAX_FILEFIND_NAME];		// 保存文件查找过程的文件名
	XMFILEFIND			fileFind;
};

typedef struct tagPLAYBACKMENUDATA{
	int				nCurItem;					//回放菜单,列表位置
	int				nItemCount;					//菜单项个数
	int				videoCurItem;				//videolist当前选中的item
	
	struct _tagVIDEOLIST *videolist;
	
    u8_t            type;                //类型
	u8_t            curch;            	 //当前显示通道
	u32_t			curpage;			 //当前page;
	u32_t 			totalpage;			 //总page;
}PLAYBACKMENUDATA;

static u8_t Video_First_Enter_Type;
static int Photo_curpage_num = 1;//图片页数
static u8_t replay_menu_timeout;
extern int get_curch_total_page(void);
extern int get_curch_page_num(void);
extern void set_video_curch_page_num(int val);
extern void set_photo_curch_page_num(int val);
extern void APP_SaveMenuData(void);


void reset_replay_menu_timeout(void)
{
	replay_menu_timeout = 0;
}

int get_photo_curpage_num(void)
{
	return Photo_curpage_num;
}

static void Video_DeleteOneFileCallback(VOID *UserPrivate, UINT uKeyPressed)
{
	PLAYBACKMENUDATA *ReplayMenuData = (PLAYBACKMENUDATA *)UserPrivate;
	HANDLE hVideoItem;
	XMVIDEOITEM *pVideoItem;
	unsigned int replay_ch;
	unsigned int nItemCount;
	
	//当调整page的时候需要判断item是否有效
	DWORD item = (ReplayMenuData->curpage-1)*6 + ReplayMenuData->videolist->nCurItem;//index
	if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
	{
		replay_ch = XM_VIDEO_CHANNEL_0;
	}
	else
	{
		replay_ch = XM_VIDEO_CHANNEL_1;
	}
	nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_VIDEO,XM_VIDEOITEM_CLASS_BITMASK_ALL);

	XM_printf(">>>>>>>>>Video_DeleteOneFileCallback,nItemCount:%d\r\n", nItemCount);
	XM_printf(">>>>>>>>>Video_DeleteOneFileCallback,item:%d\r\n", item);
		
	if(item>nItemCount)
	{//需要删除的文件,超过了总文件数,直接弹出
		XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0); //解决从图片预览返回图片列表，OSD_LAYER_0仍有图片数据
		// 将alert弹出
		XM_PullWindow(0);
	}
	else
	{
		do
		{
			if(uKeyPressed==VK_AP_MENU)
			{
				hVideoItem = AP_VideoItemGetVideoItemHandleEx(ReplayMenuData->videolist->videotype, item, ReplayMenuData->curch);
				if(hVideoItem == NULL)
				{
					XM_printf(">>>>del error2 ..............\r\n");
					break;
				}

				pVideoItem = AP_VideoItemGetVideoItemFromHandle(hVideoItem);
				if(pVideoItem == NULL)
				{
					XM_printf(">>>>del error1 ..............\r\n");
					break;
				}
				XM_printf(">>>>>>>>>pVideoItem->video_name:%s\r\n", pVideoItem->video_name);
				int ret = XM_VideoItemDeleteVideoItemHandleFromFileName((const char *)pVideoItem->video_name);
				XM_printf(">>>>>>>>>.ret:%d\r\n", ret);
				if(ret==1)
				{
					XM_printf(">>>>>>>>>>>>>del one file ok............\r\n");
				}
				else
				{
					XM_printf(">>>>>>>>>>>>>del one file error ............\r\n");
				}
			}
		}while(0);

		//删除完成后,需要重新计算当前page和item

		nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_VIDEO,XM_VIDEOITEM_CLASS_BITMASK_ALL);
		XM_printf(">>>>>>>>>>>>>>after del one file, nItemCount:%d\r\n", nItemCount);
		XM_printf(">>>>>>>>>item:%d\r\n", item);
		XM_printf(">>>>>>>>>ReplayMenuData->curpage:%d\r\n", ReplayMenuData->curpage);
		XM_printf(">>>>>>>>>ReplayMenuData->videolist->nCurItem:%d\r\n", ReplayMenuData->videolist->nCurItem);
		
		if(item>=nItemCount)
		{
			if(ReplayMenuData->videolist->nCurItem==0)
			{
				ReplayMenuData->curpage--;
			}
			else
			{
				ReplayMenuData->videolist->nCurItem--;
				if(ReplayMenuData->videolist->nCurItem==0)
				{
					ReplayMenuData->curpage--;
				}
			}
		}
		XM_printf(">>>>>>>>>1,ReplayMenuData->curpage:%d\r\n", ReplayMenuData->curpage);
		XM_printf(">>>>>>>>>1,ReplayMenuData->videolist->nCurItem:%d\r\n", ReplayMenuData->videolist->nCurItem);
		
		XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0); //解决从图片预览返回图片列表，OSD_LAYER_0仍有图片数据
		// 将alert弹出
		XM_PullWindow(0);
	}

}


static void Video_DeleteAllFileCallback(VOID *UserPrivate, UINT uKeyPressed)
{
	PLAYBACKMENUDATA *ReplayMenuData = (PLAYBACKMENUDATA *)UserPrivate;

	XM_printf(">>>>>>ReplayMenuData->curch:%d\r\n", ReplayMenuData->curch);
	XM_printf(">>>>>>ReplayMenuData->type:%d\r\n", ReplayMenuData->type);
	do
	{
		if(uKeyPressed==VK_AP_MENU)
		{
			XM_printf(">>>>>>>>>>>>>del all file............\r\n");
			int ret = XM_VideoItemDeleteVideoItemList(0,ReplayMenuData->curch, VIDEOITEM_CIRCULAR_VIDEO_MODE, XM_VIDEOITEM_CLASS_BITMASK_ALL, NULL, NULL);
			if(ret==0)
			{
				XM_printf(">>>>>>>>>>>>>del all file ok............\r\n");
			}
			else
			{
				XM_printf(">>>>>>>>>>>>>del all file error ............\r\n");
			}
		}
	}while(0);

	XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0); //解决从图片预览返回图片列表，OSD_LAYER_0仍有图片数据

	// 将alert弹出
	XM_PullWindow(0);
}

VOID PlayBackMenuOnEnter(XMMSG *msg)
{	
	unsigned int replay_ch;
	unsigned int nItemCount;
	
	XM_printf(">>>>>PlayBackMenuOnEnter..........\r\n");
	if(msg->wp == 0)
	{
		u32_t VP_CONTRAST = 0;
		Video_First_Enter_Type = TYPE_VIDEO;
		
		struct _tagVIDEOLIST *videolist = (struct _tagVIDEOLIST *)msg->lp;
	
		PLAYBACKMENUDATA *ReplayMenuData = XM_calloc(sizeof(PLAYBACKMENUDATA));
		
		if(ReplayMenuData == NULL)
		{
			XM_printf ("menuOptionViewData XM_calloc failed\n");
			// 失败返回到桌面
			XM_PullWindow (0);
			return;
		}
		reset_replay_menu_timeout();
		XM_printf(">>>>page num:%d\r\n", videolist->nPageNum);
		XM_printf(">>>>cur item:%d\r\n", videolist->nCurItem);
		XM_printf(">>>>total page num:%d\r\n", videolist->nTotalPage);//videolist传过来的总page

		//根据通道计算总page
		if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
		{
			replay_ch = XM_VIDEO_CHANNEL_0;
		}
		else
		{
			replay_ch = XM_VIDEO_CHANNEL_1;
		}
		nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_VIDEO,XM_VIDEOITEM_CLASS_BITMASK_ALL);

		ReplayMenuData->totalpage = nItemCount/6;
		if(nItemCount%6!=0)
		{
			ReplayMenuData->totalpage += 1;
		}	
		XM_printf(">>>>>>>>>>>>>>ReplayMenuData->totalpage:%d\r\n", ReplayMenuData->totalpage);

		ReplayMenuData->nCurItem = 0;
		ReplayMenuData->type = TYPE_VIDEO;
		ReplayMenuData->videoCurItem = videolist->nCurItem;//videolist中选中的文件index
		ReplayMenuData->curch = AP_GetMenuItem(APPMENUITEM_REPLAY_CH);
		ReplayMenuData->curpage = videolist->nPageNum;//videolist中选中的文件index
		ReplayMenuData->videolist = videolist;
		
		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData(XMHWND_HANDLE(PlayBackMenu), ReplayMenuData);
	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf(">>>>>>>>>>>>>>>>>play back menu Pull\n");
	}
    XM_SetTimer(XMTIMER_SETTINGVIEW, 500); //开启定时器
}

VOID PlayBackMenuOnLeave(XMMSG *msg)
{
	XM_printf(">>>>>PlayBackMenuOnLeave..........\r\n");
    XM_KillTimer (XMTIMER_SETTINGVIEW);
	if (msg->wp == 0)
	{
		PLAYBACKMENUDATA *ReplayMenuData = (PLAYBACKMENUDATA *)XM_GetWindowPrivateData(XMHWND_HANDLE(PlayBackMenu));
		if(ReplayMenuData)
		{
			// 释放私有数据句柄
			XM_free (ReplayMenuData);
		}
		XM_printf(">>>>>play back menu Exit.......\r\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf(">>>>>play back menu Push......\r\n");
	}
}


VOID PlayBackMenuOnPaint(XMMSG *msg)
{
	XMRECT rc,rect,rcArrow;
	unsigned int old_alpha;
    char String[32];
    XMSIZE size;
    int nVisualCount,i,nItem;
	unsigned int replay_ch;
	unsigned int nItemCount;
	
	XM_printf(">>>>>>>>>>>PlayBackMenuOnPaint, msg->wp:%d\r\n", msg->wp);
	HANDLE hwnd = XMHWND_HANDLE(PlayBackMenu);
	PLAYBACKMENUDATA *ReplayMenuData = (PLAYBACKMENUDATA *)XM_GetWindowPrivateData (hwnd);
	if(ReplayMenuData == NULL)
		return;

	// 显示标题栏
	XM_GetDesktopRect (&rc);
	XM_FillRect(hwnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	old_alpha = XM_GetWindowAlpha (hwnd);

	XM_SetWindowAlpha(hwnd, 255);

	#if 0
	if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
	{
		replay_ch = XM_VIDEO_CHANNEL_0;
	}
	else
	{
		replay_ch = XM_VIDEO_CHANNEL_1;
	}
	XM_printf(">>>>>>>>>>>ReplayMenuData->curpage:%d\r\n", ReplayMenuData->curpage);
	XM_printf(">>>>>>>>>>>>ReplayMenuData->type:%d\r\n", ReplayMenuData->type);
	if(ReplayMenuData->type==TYPE_VIDEO)
	{
		XM_printf(">>>>>>>>>>>>replay_ch:%d\r\n", replay_ch);
		nItemCount = (WORD)AP_VideoItemGetVideoFileCountEx(VIDEOITEM_CIRCULAR_VIDEO_MODE, replay_ch);
		ReplayMenuData->videolist->nItemCount = nItemCount;
		ReplayMenuData->totalpage = nItemCount/6;
		if(nItemCount%6!=0)
		{
			ReplayMenuData->totalpage += 1;
		}	
	}
	else
	{
		nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_ALL);
		ReplayMenuData->videolist->nItemCount = nItemCount;
		ReplayMenuData->totalpage = nItemCount/6;

		if(nItemCount%6!=0)
		{
			ReplayMenuData->totalpage += 1;
		}
	}

	XM_printf(">>>>>>>>>>>ReplayMenuData->curpage:%d\r\n", ReplayMenuData->curpage);
	XM_printf(">>>>>>>>>>>ReplayMenuData->totalpage:%d\r\n", ReplayMenuData->totalpage);
	if(ReplayMenuData->curpage>ReplayMenuData->totalpage)
	{
		ReplayMenuData->curpage = 1;
	}
	#endif
	
	/*********画title***********/
	rect = rc;rect.bottom = 42;
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUTITLEBARBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title 背景
	rect = rc;rect.bottom = 42;
	AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_REPLAYMENU, hwnd, &rect, XMGIF_DRAW_POS_CENTER);//title 名字
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title分割线

	//item选中背景
    rect = rc;
	rect.left = 0;
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (ReplayMenuData->nCurItem)  * APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//选中背景色
	
	//类型
    rect = rc;
	rect.top = 42+13;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_TYPE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//类型
	
    //显示视频/照片
    rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
	if(ReplayMenuData->type==TYPE_VIDEO)
	{
		AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_VIDEO, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//默认视频
	}
	else
	{
		AP_RomImageDrawByMenuID(AP_ID_VIDEO_TITLE_ALBUM, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//默认照片
	}
	
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(42+APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title分割线
	
	//通道
    rect = rc;
	rect.top = 42+62+13;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_CH, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//通道
    //显示CH1/CH2
    rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
	DWORD MenuID;
	if(ReplayMenuData->curch==CH_AHD1)
	{
		MenuID = AP_ID_VIDEO_BUTTON_CH1;
	}
	else if(ReplayMenuData->curch==CH_AHD2)
	{
		MenuID = AP_ID_VIDEO_BUTTON_CH2;
	}
	rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
	AP_RomImageDrawByMenuID(MenuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(42+APP_POS_ITEM5_LINEHEIGHT*2);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title分割线
	
	//页数
    rect = rc;
	rect.top = 42+62+62+13;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_PAGE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//页数
    //显示当前页数/总页数
    rect.left = APP_POS_ITEM5_MENUFLAG_X*3;

	//根据通道及类型，计算总page
	if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
	{
		replay_ch = XM_VIDEO_CHANNEL_0;
	}
	else
	{
		replay_ch = XM_VIDEO_CHANNEL_1;
	}
	if(ReplayMenuData->type==TYPE_VIDEO)
	{
		nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_VIDEO,XM_VIDEOITEM_CLASS_BITMASK_ALL);
		ReplayMenuData->totalpage = nItemCount/6;
		if(nItemCount%6!=0)
		{
			ReplayMenuData->totalpage += 1;
		}	
	}
	else
	{
		nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_ALL);
		ReplayMenuData->totalpage = nItemCount/6;
		if(nItemCount%6!=0)
		{
			ReplayMenuData->totalpage += 1;
		}
	}
	
    sprintf(String,"%d%s%d", ReplayMenuData->curpage, "/", ReplayMenuData->totalpage);
    AP_TextGetStringSize(String,sizeof(String),&size);
    AP_TextOutDataTimeString(hwnd, rect.left, rect.top, String, strlen(String));
	
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(42+APP_POS_ITEM5_LINEHEIGHT*3);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title分割线
	
	//删除
    rect = rc;
	rect.top = 42+62+62+62+18;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_DELETE_ONE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    //显示CH1/CH2
    rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
	AP_RomImageDrawByMenuID(AP_ID_COMMON_RIGHT_ARROW, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(42+APP_POS_ITEM5_LINEHEIGHT*4);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title分割线
	
	//删除全部
    rect = rc;
	rect.top = 42+62+62+62+62+18;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_DELETE_ALL, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    //显示CH1/CH2
    rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
	AP_RomImageDrawByMenuID(AP_ID_COMMON_RIGHT_ARROW, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(42+APP_POS_ITEM5_LINEHEIGHT*4);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title分割线

    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(42+APP_POS_ITEM5_LINEHEIGHT*5);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title分割线
	
	XM_SetWindowAlpha(hwnd, (unsigned char)old_alpha);
}

void AdjustReplayMenuDat(PLAYBACKMENUDATA *ReplayMenuDat, int nItem, int diff)
{
    switch(nItem) 
	{
		case REPLAYMENU_ITEM0_TYPE:
			if(ReplayMenuDat->type==TYPE_VIDEO)
			{
				ReplayMenuDat->type = TYPE_PIC;
			}
			else if(ReplayMenuDat->type==TYPE_PIC)
			{
				ReplayMenuDat->type = TYPE_VIDEO;
			}	
			ReplayMenuDat->curpage = 1;//切换类型，默认调到PAGE 1
			XM_printf(">>>>>>>>>ReplayMenuDat->type:%d\r\n", ReplayMenuDat->type);
			break;
			
	    case REPLAYMENU_ITEM1_CH://通道选择
	        if(diff == 1) 
			{
				if(ReplayMenuDat->curch==CH_AHD1)
				{
					ReplayMenuDat->curch = CH_AHD2;
				}
				else if(ReplayMenuDat->curch==CH_AHD2)
				{
					ReplayMenuDat->curch = CH_AHD1;
				}
				AP_SetMenuItem(APPMENUITEM_REPLAY_CH, ReplayMenuDat->curch);
				ReplayMenuDat->curpage = 1;//切换通道，默认调到PAGE 1
	        }
			else if(diff == -1) 
			{
				if(ReplayMenuDat->curch==CH_AHD1)
				{
					ReplayMenuDat->curch = CH_AHD2;
				}
				else if(ReplayMenuDat->curch==CH_AHD2)
				{
					ReplayMenuDat->curch = CH_AHD1;
				}
				AP_SetMenuItem(APPMENUITEM_REPLAY_CH, ReplayMenuDat->curch);
				ReplayMenuDat->curpage = 1;//切换通道，默认调到PAGE 1
			}
	        break;

		case REPLAYMENU_ITEM2_PAGE:
			if(diff==1)
			{
				if(ReplayMenuDat->curpage<=ReplayMenuDat->totalpage)
				{
					ReplayMenuDat->curpage++;
				}
				
				if(ReplayMenuDat->curpage>ReplayMenuDat->totalpage)
				{
					ReplayMenuDat->curpage = 1;
				}
			}
			else if(diff==-1)
			{
				if(ReplayMenuDat->curpage>0)
				{
					ReplayMenuDat->curpage--;
					if(ReplayMenuDat->curpage==0)
					{
						ReplayMenuDat->curpage = 1;
					}
				}
			}
			break;

		case REPLAYMENU_ITEM3_DEL://删除选中文件

			break;

		case REPLAYMENU_ITEM3_DELALL://删除所有文件

			break;
			
		default:
			break;
	}
}


VOID PlayBackMenuOnKeyDown(XMMSG *msg)
{
	XMRECT rc;
	
	PLAYBACKMENUDATA *ReplayMenuData = (PLAYBACKMENUDATA *)XM_GetWindowPrivateData(XMHWND_HANDLE(PlayBackMenu));
	if(ReplayMenuData == NULL)
		return;

	XM_printf(">>>>>>>>>>>PlayBackMenuOnKeyDown, msg->wp:%d\r\n", msg->wp);
    switch(msg->wp)
    {
        case REMOTE_KEY_MENU:
		case VK_AP_MENU://进入或退出回放菜单
			XM_printf(">>>VK_AP_MENU...\r\n");
			XM_printf(">>>>>>>>ReplayMenuData->curpage:%d\r\n", ReplayMenuData->curpage);

			if( (ReplayMenuData->type==TYPE_VIDEO) && (Video_First_Enter_Type==TYPE_VIDEO) )
			{//视频选项
				ReplayMenuData->videolist->nPageNum = ReplayMenuData->curpage;
				XM_PullWindow(0);
			}
			else if( (ReplayMenuData->type==TYPE_PIC) && (Video_First_Enter_Type==TYPE_VIDEO) )
			{
				XM_printf(">>>type other photo...\r\n");
				XM_printf(">>>>>>>>jump album list view..........\r\n");
				Photo_curpage_num = ReplayMenuData->curpage;//跳到图片回放模式,传输这两个参数
				XM_JumpWindow(XMHWND_HANDLE(AlbumListView));//把栈中UI全部弹出,跳到指定UI
			}
			APP_SaveMenuData();
			break;

		case REMOTE_KEY_DOWN:
        case VK_AP_SWITCH://确认键，调节键
			XM_printf(">>>VK_AP_SWITCH...\r\n");
			reset_replay_menu_timeout();

			if(ReplayMenuData->nCurItem==REPLAYMENU_ITEM3_DEL)
			{//跳到确认框(videoListViewListData->nPageNum-1)*6 + i
				XM_printf(">>>>>>>>>nItemCount:%d\r\n", ReplayMenuData->videolist->nItemCount);
				XM_printf(">>>>>>>>>ReplayMenuData->videoCurItem:%d\r\n", ReplayMenuData->videoCurItem);

				if(ReplayMenuData->videolist->nItemCount>0)
				{//有file
					if(ReplayMenuData->videolist->videotype==VIDEOITEM_CIRCULAR_VIDEO_MODE)
					{//视频选项
						DWORD dwButtonNormalTextID[2];
						DWORD dwButtonPressedTextID[2];
						dwButtonNormalTextID[0] = AP_ID_VIDEO_BUTTON_DELETE_YES;
						dwButtonPressedTextID[0] = AP_ID_VIDEO_BUTTON_DELETE_YES;
						dwButtonNormalTextID[1] = AP_ID_VIDEO_BUTTON_CANCEL_NO;
						dwButtonPressedTextID[1] = AP_ID_VIDEO_BUTTON_CANCEL_NO;

						XM_OpenAlertView(AP_ID_VIDEO_INFO_TO_DELETE_ONE_VIDEO,
										AP_NULLID,
										2,
										dwButtonNormalTextID,
										dwButtonPressedTextID,
										APP_ALERT_BKGCOLOR,
										(float)20.0,		// 自动关闭时间
										APP_ALERT_BKGALPHA,
										Video_DeleteOneFileCallback,	// 按键回调函数
										ReplayMenuData,
										XM_VIEW_ALIGN_CENTRE,		// 居中对齐
										XM_ALERTVIEW_OPTION_ENABLE_CALLBACK		// ALERTVIEW视图的控制选项
										);
					}
				}
			}
			else if(ReplayMenuData->nCurItem==REPLAYMENU_ITEM3_DELALL)
			{//跳到确认框
				XM_printf(">>>>>>>>>nItemCount:%d\r\n", ReplayMenuData->videolist->nItemCount);
				if(ReplayMenuData->videolist->nItemCount>0)
				{//有file
					if(ReplayMenuData->videolist->videotype==VIDEOITEM_CIRCULAR_VIDEO_MODE)
					{//视频选项
						DWORD dwButtonNormalTextID[2];
						DWORD dwButtonPressedTextID[2];
						dwButtonNormalTextID[0] = AP_ID_VIDEO_BUTTON_DELETE_YES;
						dwButtonPressedTextID[0] = AP_ID_VIDEO_BUTTON_DELETE_YES;
						dwButtonNormalTextID[1] = AP_ID_VIDEO_BUTTON_CANCEL_NO;
						dwButtonPressedTextID[1] = AP_ID_VIDEO_BUTTON_CANCEL_NO;

						XM_OpenAlertView(AP_ID_VIDEO_INFO_TO_DELETE_ALL_VIDEO,
										AP_NULLID,
										2,
										dwButtonNormalTextID,
										dwButtonPressedTextID,
										APP_ALERT_BKGCOLOR,
										(float)20.0,		// 自动关闭时间
										APP_ALERT_BKGALPHA,
										Video_DeleteAllFileCallback,	// 按键回调函数
										ReplayMenuData,
										XM_VIEW_ALIGN_CENTRE,		// 居中对齐
										XM_ALERTVIEW_OPTION_ENABLE_CALLBACK		// ALERTVIEW视图的控制选项
										);
					}				
				}
			}
			else
			{
				AdjustReplayMenuDat(ReplayMenuData, ReplayMenuData->nCurItem, 1);
				XM_printf(">>>>>>>>>>>ReplayMenuData->curpage:%d\r\n", ReplayMenuData->curpage);
				//刷新
				XM_InvalidateWindow();
				XM_UpdateWindow();
			}
            break;

		case REMOTE_KEY_UP:
        case VK_AP_FONT_BACK_SWITCH:
			XM_printf(">>>VK_AP_FONT_BACK_SWITCH...\r\n");
			if( (ReplayMenuData->type==TYPE_VIDEO) && (Video_First_Enter_Type==TYPE_VIDEO) )
			{//视频选项
				ReplayMenuData->videolist->nPageNum = ReplayMenuData->curpage;
				XM_PullWindow(0);
			}
            break;

        case REMOTE_KEY_RIGHT:
        case VK_AP_UP://选中下一个item
			XM_printf(">>>VK_AP_UP...\r\n");
			reset_replay_menu_timeout();
			if(ReplayMenuData->nCurItem==4)
			{
				ReplayMenuData->nCurItem = 0;
			}
			else
			{
				ReplayMenuData->nCurItem ++;
			}
    		XM_InvalidateWindow();
    		XM_UpdateWindow();
            break;

		case REMOTE_KEY_LEFT:
		case VK_AP_DOWN://选中上一个item
			XM_printf(">>>VK_AP_DOWN...\r\n");
			reset_replay_menu_timeout();
			if(ReplayMenuData->nCurItem==0)
			{
				ReplayMenuData->nCurItem = 4;
			}
			else
			{
				ReplayMenuData->nCurItem --;
			}
    		XM_InvalidateWindow();
    		XM_UpdateWindow();
            break;

		default:
			break;
    }

}

VOID PlayBackMenuOnKeyRepeat(XMMSG *msg)
{
	XMRECT rc;
	
	PLAYBACKMENUDATA *ReplayMenuData = (PLAYBACKMENUDATA *)XM_GetWindowPrivateData(XMHWND_HANDLE(PlayBackMenu));
	if(ReplayMenuData == NULL)
		return;

	XM_printf(">>>>>>>>>>>PlayBackMenuOnKeyRepeat, msg->wp:%d\r\n", msg->wp);
    switch(msg->wp)
    {
        case VK_AP_SWITCH:
			XM_printf(">>>VK_AP_SWITCH...\r\n");
			reset_replay_menu_timeout();
			AdjustReplayMenuDat(ReplayMenuData, ReplayMenuData->nCurItem, 1);
			//刷新
			XM_InvalidateWindow();
			XM_UpdateWindow();
            break;

		default:
			break;
    }
}


VOID PlayBackMenuOnKeyUp (XMMSG *msg)
{
	XM_printf(">>>>>>>>PlayBackMenuOnKeyUp \n");
}

VOID PlayBackMenuOnTimer (XMMSG *msg)
{
	XM_printf(">>>>>>>>PlayBackMenuOnTimer \n");
	replay_menu_timeout++;
	if(replay_menu_timeout>=20)
	{
		XM_PullWindow(0);
	}
	else if(get_parking_trig_status() != 0 && AP_GetMenuItem(APPMENUITEM_PARKING_LINE))
	{
	    XM_PullWindow(0);//返回桌面显示倒车界面
	}
}

static VOID PlayBackMenuOnSystemEvent(XMMSG *msg)
{
	XM_printf(">>>>>>>>PlayBackMenuOnSystemEvent, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:			// SD卡拔出事件
			XM_printf(">>>>>PlayBackMenuOnSystemEvent,1\r\n");
			XM_BreakSystemEventDefaultProcess (msg);		// 不进行缺省处理

			#if 0
			// 将该消息重新插入到消息队列队首
			XM_InsertMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
			XM_JumpWindowEx (XMHWND_HANDLE(Desktop), HWND_CUSTOM_DEFAULT, XM_JUMP_POPDEFAULT);
			//XM_JumpWindow (XMHWND_HANDLE(Desktop));	// 跳转到桌面
			#endif
			//XM_PullWindow(0);					   // 弹出当前的列表视窗
			XM_printf(">>>>>PlayBackMenuOnSystemEvent,2\r\n");
			XM_JumpWindow(XMHWND_HANDLE(Desktop));
			break;

		default:
			break;
	}
}

//消息处理函数定义
XM_MESSAGE_MAP_BEGIN(PlayBackMenu)
	XM_ON_MESSAGE (XM_PAINT, PlayBackMenuOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, PlayBackMenuOnKeyDown)
	XM_ON_MESSAGE (XMKEY_REPEAT, PlayBackMenuOnKeyRepeat)
	XM_ON_MESSAGE (XM_KEYUP, PlayBackMenuOnKeyUp)
	XM_ON_MESSAGE (XM_TIMER, PlayBackMenuOnTimer)
	XM_ON_MESSAGE (XM_ENTER, PlayBackMenuOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, PlayBackMenuOnLeave)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, PlayBackMenuOnSystemEvent)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(10, 20, 30, 40, PlayBackMenu, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
