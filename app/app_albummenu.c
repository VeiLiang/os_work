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

#define PHOTOMENU_ITEM0_TYPE	0
#define PHOTOMENU_ITEM1_CH		1
#define PHOTOMENU_ITEM2_PAGE	2
#define PHOTOMENU_ITEM3_DEL		3
#define PHOTOMENU_ITEM3_DELALL	4

struct _tagPHOTOLIST{
	int					nPageNum;					// 第一个可视的菜单项
	int					nCurItem;					// 当前选择的菜单项

	int					nItemCount;					// 菜单项个数
	int					nVisualCount;				// 视窗可显示的项目数量
	int					nTotalPage;					//总page;

	BYTE				videotype;

    HANDLE				hMoniorSwitchControl;		//停车监控
	char				fileName[XM_MAX_FILEFIND_NAME];		// 保存文件查找过程的文件名
	XMFILEFIND			fileFind;
};

typedef struct tagPHOTOMENUDATA{
	int				nCurItem;					//回放菜单,列表位置
	int				nItemCount;					//菜单项个数
	int				videoCurItem;				//videolist当前选中的item
	
	struct _tagPHOTOLIST *photolist;
	
    u8_t            type;                //类型
	u8_t            curch;            	 //当前显示通道
	u32_t			curpage;			 //当前page;
	u32_t 			totalpage;			 //总page;
}PHOTOMENUDATA;

static u8_t Photo_First_Enter_Type;
static u8_t photo_menu_timeout;
static u8_t Video_curpage_num = 1;

void reset_photo_menu_timeout(void)
{
	photo_menu_timeout = 0;
}

u8_t get_video_curpage_num(void)
{
	return Video_curpage_num;
}

static void Photo_DeleteOneFileCallback(VOID *UserPrivate, UINT uKeyPressed)
{
	PHOTOMENUDATA *PhotoMenuData = (PHOTOMENUDATA *)UserPrivate;
	HANDLE hVideoItem;
	XMVIDEOITEM *pVideoItem;
	unsigned int replay_ch;
	unsigned int nItemCount;
	
	//当调整page的时候需要判断item是否有效
	DWORD item = (PhotoMenuData->curpage-1)*6 + PhotoMenuData->photolist->nCurItem;//index
	if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
	{
		replay_ch = XM_VIDEO_CHANNEL_0;
	}
	else
	{
		replay_ch = XM_VIDEO_CHANNEL_1;
	}
	nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_ALL);

	XM_printf(">>>>>>>>>Photo_DeleteOneFileCallback,nItemCount:%d\r\n", nItemCount);
	XM_printf(">>>>>>>>>Photo_DeleteOneFileCallback,item:%d\r\n", item);
		
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
				hVideoItem = XM_VideoItemGetVideoItemHandleEx(0,PhotoMenuData->curch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL,item,1);
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
		nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_ALL);
		XM_printf(">>>>>>>>>>>>>>after del one file, nItemCount:%d\r\n", nItemCount);
		XM_printf(">>>>>>>>>item:%d\r\n", item);
		XM_printf(">>>>>>>>>PhotoMenuData->curpage:%d\r\n", PhotoMenuData->curpage);
		XM_printf(">>>>>>>>>PhotoMenuData->photolist->nCurItem:%d\r\n", PhotoMenuData->photolist->nCurItem);
		
		if(item>=nItemCount)
		{
			if(PhotoMenuData->photolist->nCurItem==0)
			{
				PhotoMenuData->curpage--;
			}
			else
			{
				PhotoMenuData->photolist->nCurItem--;
				if(PhotoMenuData->photolist->nCurItem==0)
				{
					PhotoMenuData->curpage--;
				}
			}
		}
		XM_printf(">>>>>>>>>1,PhotoMenuData->curpage:%d\r\n", PhotoMenuData->curpage);
		XM_printf(">>>>>>>>>1,PhotoMenuData->photolist->nCurItem:%d\r\n", PhotoMenuData->photolist->nCurItem);
		
		XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0); //解决从图片预览返回图片列表，OSD_LAYER_0仍有图片数据
		// 将alert弹出
		XM_PullWindow(0);
	}
}


static void Photo_DeleteAllFileCallback(VOID *UserPrivate, UINT uKeyPressed)
{
	PHOTOMENUDATA *PhotoMenuData = (PHOTOMENUDATA *)UserPrivate;

	XM_printf(">>>>>>PhotoMenuData->curch:%d\r\n", PhotoMenuData->curch);
	XM_printf(">>>>>>PhotoMenuData->type:%d\r\n", PhotoMenuData->type);
	do
	{
		if(uKeyPressed==VK_AP_MENU)
		{
			XM_printf(">>>>>>>>>>>>>del all file............\r\n");
			int ret = XM_VideoItemDeleteVideoItemList(0,PhotoMenuData->curch, PhotoMenuData->type, XM_VIDEOITEM_CLASS_BITMASK_ALL, NULL, NULL);
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

	XM_osd_framebuffer_release(XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0); //解决从图片预览返回图片列表，OSD_LAYER_0仍有图片数据
	// 将alert弹出
	XM_PullWindow(0);
}

VOID PhotoMenuOnEnter(XMMSG *msg)
{	
	unsigned int replay_ch;
	unsigned int nItemCount;
	
	XM_printf(">>>>>PhotoMenuOnEnter..........\r\n");
	if(msg->wp == 0)
	{
		u32_t VP_CONTRAST = 0;
		Photo_First_Enter_Type = TYPE_PIC;
		
		struct _tagPHOTOLIST *photolist = (struct _tagPHOTOLIST *)msg->lp;
	
		PHOTOMENUDATA *PhotoMenuData = XM_calloc(sizeof(PHOTOMENUDATA));
		
		if(PhotoMenuData == NULL)
		{
			XM_printf ("menuOptionViewData XM_calloc failed\n");
			// 失败返回到桌面
			XM_PullWindow (0);
			return;
		}
		reset_photo_menu_timeout();
		XM_printf(">>>>page num:%d\r\n", photolist->nPageNum);
		XM_printf(">>>>cur item:%d\r\n", photolist->nCurItem);
		XM_printf(">>>>total page num:%d\r\n", photolist->nTotalPage);//videolist传过来的总page

		//根据通道计算总page
		if(AP_GetMenuItem(APPMENUITEM_REPLAY_CH)==CH_AHD1)
		{
			replay_ch = XM_VIDEO_CHANNEL_0;
		}
		else
		{
			replay_ch = XM_VIDEO_CHANNEL_1;
		}

		nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_ALL);
		PhotoMenuData->totalpage = nItemCount/6;

		if(nItemCount%6!=0)
		{
			PhotoMenuData->totalpage += 1;
		}
		XM_printf(">>>>>>>>>>>>>>PhotoMenuData->totalpage:%d\r\n", PhotoMenuData->totalpage);

		PhotoMenuData->nCurItem = 0;
		PhotoMenuData->type = TYPE_PIC;
		PhotoMenuData->videoCurItem = photolist->nCurItem;//videolist中选中的文件index
		PhotoMenuData->curch = AP_GetMenuItem(APPMENUITEM_REPLAY_CH);
		PhotoMenuData->curpage = photolist->nPageNum;//videolist中选中的文件index
		PhotoMenuData->photolist = photolist;
	
		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData(XMHWND_HANDLE(PhotoMenu), PhotoMenuData);
	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf(">>>>>>>>>>>>>>>>>play back menu Pull\n");
	}
    XM_SetTimer(XMTIMER_SETTINGVIEW, 1000); //开启定时器
}

VOID PhotoMenuOnLeave(XMMSG *msg)
{
	XM_printf(">>>>>PhotoMenuOnLeave..........\r\n");
    XM_KillTimer (XMTIMER_SETTINGVIEW);
	if (msg->wp == 0)
	{
		PHOTOMENUDATA *PhotoMenuData = (PHOTOMENUDATA *)XM_GetWindowPrivateData(XMHWND_HANDLE(PhotoMenu));
		if(PhotoMenuData)
		{
			// 释放私有数据句柄
			XM_free (PhotoMenuData);
		}
		XM_printf(">>>>>photo menu Exit.....\r\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf(">>>>>photo menu Push......\r\n");
	}
}


VOID PhotoMenuOnPaint(XMMSG *msg)
{
	XMRECT rc,rect,rcArrow;
	unsigned int old_alpha;
    char String[32];
    XMSIZE size;
    int nVisualCount,i,nItem;
	unsigned int replay_ch;
	unsigned int nItemCount;
	
	XM_printf(">>>>>>>>>>>PhotoMenuOnPaint, msg->wp:%d\r\n", msg->wp);
	HANDLE hwnd = XMHWND_HANDLE(PhotoMenu);
	PHOTOMENUDATA *PhotoMenuData = (PHOTOMENUDATA *)XM_GetWindowPrivateData (hwnd);
	if(PhotoMenuData == NULL)
		return;

	// 显示标题栏
	XM_GetDesktopRect (&rc);
	XM_FillRect(hwnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	old_alpha = XM_GetWindowAlpha (hwnd);

	XM_SetWindowAlpha(hwnd, 255);
	
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
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (PhotoMenuData->nCurItem)  * APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//选中背景色
	
	//类型
    rect = rc;
	rect.top = 42+13;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_TYPE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//类型
	
    //显示视频/照片
    rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
	if(PhotoMenuData->type==TYPE_VIDEO)
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
	if(PhotoMenuData->curch==CH_AHD1)
	{
		MenuID = AP_ID_VIDEO_BUTTON_CH1;
	}
	else if(PhotoMenuData->curch==CH_AHD2)
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
	if(PhotoMenuData->type==TYPE_VIDEO)
	{
		nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_VIDEO,XM_VIDEOITEM_CLASS_BITMASK_ALL);
		PhotoMenuData->totalpage = nItemCount/6;
		if(nItemCount%6!=0)
		{
			PhotoMenuData->totalpage += 1;
		}	
	}
	else
	{
		nItemCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,replay_ch,XM_FILE_TYPE_PHOTO,XM_VIDEOITEM_CLASS_BITMASK_ALL);
		PhotoMenuData->totalpage = nItemCount/6;
		if(nItemCount%6!=0)
		{
			PhotoMenuData->totalpage += 1;
		}
	}
    sprintf(String,"%d%s%d", PhotoMenuData->curpage, "/", PhotoMenuData->totalpage);
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

void AdjustPhotoMenuDat(PHOTOMENUDATA *PhotoMenuData, int nItem, int diff)
{
    switch(nItem) 
	{
		case PHOTOMENU_ITEM0_TYPE:
			if(PhotoMenuData->type==TYPE_VIDEO)
			{
				PhotoMenuData->type = TYPE_PIC;
			}
			else if(PhotoMenuData->type==TYPE_PIC)
			{
				PhotoMenuData->type = TYPE_VIDEO;
			}	
			PhotoMenuData->curpage = 1;//切换类型，默认调到PAGE 1
			XM_printf(">>>>>>>>>PhotoMenuData->type:%d\r\n", PhotoMenuData->type);
			break;
			
	    case PHOTOMENU_ITEM1_CH://通道选择
	        if(diff == 1) 
			{
				if(PhotoMenuData->curch==CH_AHD1)
				{
					PhotoMenuData->curch = CH_AHD2;
				}
				else if(PhotoMenuData->curch==CH_AHD2)
				{
					PhotoMenuData->curch = CH_AHD1;
				}
				AP_SetMenuItem(APPMENUITEM_REPLAY_CH, PhotoMenuData->curch);
				PhotoMenuData->curpage = 1;//切换通道，默认调到PAGE 1
	        }
			else if(diff == -1) 
			{
				if(PhotoMenuData->curch==CH_AHD1)
				{
					PhotoMenuData->curch = CH_AHD2;
				}
				else if(PhotoMenuData->curch==CH_AHD2)
				{
					PhotoMenuData->curch = CH_AHD1;
				}
				AP_SetMenuItem(APPMENUITEM_REPLAY_CH, PhotoMenuData->curch);
				PhotoMenuData->curpage = 1;//切换通道，默认调到PAGE 1
			}
	        break;

		case PHOTOMENU_ITEM2_PAGE:
			if(diff==1)
			{
				if(PhotoMenuData->curpage<=PhotoMenuData->totalpage)
				{
					PhotoMenuData->curpage++;
				}
				
				if(PhotoMenuData->curpage>PhotoMenuData->totalpage)
				{
					PhotoMenuData->curpage = 1;
				}
			}
			else if(diff==-1)
			{
				if(PhotoMenuData->curpage>0)
				{
					PhotoMenuData->curpage--;
					if(PhotoMenuData->curpage==0)
					{
						PhotoMenuData->curpage = 1;
					}
				}
			}
			break;

		case PHOTOMENU_ITEM3_DEL://删除选中文件

			break;

		case PHOTOMENU_ITEM3_DELALL://删除所有文件

			break;
			
		default:
			break;
	}
}


VOID PhotoMenuOnKeyDown(XMMSG *msg)
{
	XMRECT rc;
	
	PHOTOMENUDATA *PhotoMenuData = (PHOTOMENUDATA *)XM_GetWindowPrivateData(XMHWND_HANDLE(PhotoMenu));
	if(PhotoMenuData == NULL)
		return;

	XM_printf(">>>>>>>>>>>PhotoMenuOnKeyDown, msg->wp:%d\r\n", msg->wp);
    switch(msg->wp)
    {
    	case VK_AP_FONT_BACK_SWITCH:
		case VK_AP_MENU://进入或退出回放菜单
			XM_printf(">>>VK_AP_MENU...\r\n");
			XM_printf(">>>>>>>>PhotoMenuData->curpage:%d\r\n", PhotoMenuData->curpage);

			if( (PhotoMenuData->type==TYPE_PIC) && (Photo_First_Enter_Type==TYPE_PIC) )
			{//视频选项
				PhotoMenuData->photolist->nPageNum = PhotoMenuData->curpage;
				XM_PullWindow(0);
			}
			else if( (PhotoMenuData->type==TYPE_VIDEO) && (Photo_First_Enter_Type==TYPE_PIC) )
			{
				XM_printf(">>>type other video...\r\n");
				XM_printf(">>>>>>>>jump video list view..........\r\n");
				Video_curpage_num = PhotoMenuData->curpage;//跳到图片回放模式,传输这两个参数
				XM_JumpWindow(XMHWND_HANDLE(VideoListView));//把栈中UI全部弹出,跳到指定UI
			}
			APP_SaveMenuData();
			break;
			
        case VK_AP_SWITCH://确认键，调节键
			XM_printf(">>>VK_AP_SWITCH...\r\n");
			reset_photo_menu_timeout();

			if(PhotoMenuData->nCurItem==PHOTOMENU_ITEM3_DEL)
			{//跳到确认框(videoListViewListData->nPageNum-1)*6 + i
				XM_printf(">>>>>>>>>nItemCount:%d\r\n", PhotoMenuData->photolist->nItemCount);
				if(PhotoMenuData->photolist->nItemCount>0)
				{//有file
					{//图片选项
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
										Photo_DeleteOneFileCallback,	// 按键回调函数
										PhotoMenuData,
										XM_VIEW_ALIGN_CENTRE,		// 居中对齐
										XM_ALERTVIEW_OPTION_ENABLE_CALLBACK		// ALERTVIEW视图的控制选项
										);
					}
				}
			}
			else if(PhotoMenuData->nCurItem==PHOTOMENU_ITEM3_DELALL)
			{//跳到确认框
				XM_printf(">>>>>>>>>nItemCount:%d\r\n", PhotoMenuData->photolist->nItemCount);
				if(PhotoMenuData->photolist->nItemCount>0)
				{//有file					
					{//图片选项
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
										Photo_DeleteAllFileCallback,	// 按键回调函数
										PhotoMenuData,
										XM_VIEW_ALIGN_CENTRE,		// 居中对齐
										XM_ALERTVIEW_OPTION_ENABLE_CALLBACK		// ALERTVIEW视图的控制选项
										);
					}
				}
			}
			else
			{
				AdjustPhotoMenuDat(PhotoMenuData, PhotoMenuData->nCurItem, 1);
				XM_printf(">>>>>>>>>>>PhotoMenuData->curpage:%d\r\n", PhotoMenuData->curpage);
				//刷新
				XM_InvalidateWindow();
				XM_UpdateWindow();
			}
            break;

        case VK_AP_UP://选中下一个item
			XM_printf(">>>VK_AP_UP...\r\n");
			reset_photo_menu_timeout();
			if(PhotoMenuData->nCurItem==4)
			{
				PhotoMenuData->nCurItem = 0;
			}
			else
			{
				PhotoMenuData->nCurItem ++;
			}
    		XM_InvalidateWindow();
    		XM_UpdateWindow();
            break;
			
		case VK_AP_DOWN://选中上一个item
			XM_printf(">>>VK_AP_DOWN...\r\n");
			reset_photo_menu_timeout();
			if(PhotoMenuData->nCurItem==0)
			{
				PhotoMenuData->nCurItem = 4;
			}
			else
			{
				PhotoMenuData->nCurItem --;
			}
    		XM_InvalidateWindow();
    		XM_UpdateWindow();
            break;

		default:
			break;
    }

}

VOID PhotoMenuOnKeyRepeat(XMMSG *msg)
{
	XMRECT rc;
	
	PHOTOMENUDATA *PhotoMenuData = (PHOTOMENUDATA *)XM_GetWindowPrivateData(XMHWND_HANDLE(PhotoMenu));
	if(PhotoMenuData == NULL)
		return;

	XM_printf(">>>>>>>>>>>PhotoMenuOnKeyRepeat, msg->wp:%d\r\n", msg->wp);
    switch(msg->wp)
    {
        case VK_AP_SWITCH:
			XM_printf(">>>VK_AP_SWITCH...\r\n");
			reset_photo_menu_timeout();
			AdjustPhotoMenuDat(PhotoMenuData, PhotoMenuData->nCurItem, 1);
			//刷新
			XM_InvalidateWindow();
			XM_UpdateWindow();
            break;

		default:
			break;
    }
}


VOID PhotoMenuOnKeyUp (XMMSG *msg)
{
	XM_printf(">>>>>>>>PhotoMenuOnKeyUp \n");
}

VOID PhotoMenuOnTimer (XMMSG *msg)
{
	XM_printf(">>>>>>>>PhotoMenuOnTimer \n");
	photo_menu_timeout++;
	if(photo_menu_timeout>=10)
	{
		XM_PullWindow(0);
	}
}


static VOID PhotoMenuOnSystemEvent(XMMSG *msg)
{
	XM_printf(">>>>>>>>PhotoMenuOnSystemEvent, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:			// SD卡拔出事件
			XM_printf(">>>>>PhotoMenuOnSystemEvent,1\r\n");
			XM_BreakSystemEventDefaultProcess (msg);		// 不进行缺省处理

			#if 0
			// 将该消息重新插入到消息队列队首
			XM_InsertMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
			XM_JumpWindowEx (XMHWND_HANDLE(Desktop), HWND_CUSTOM_DEFAULT, XM_JUMP_POPDEFAULT);
			//XM_JumpWindow (XMHWND_HANDLE(Desktop));	// 跳转到桌面
			#endif
			//XM_PullWindow(0);					   // 弹出当前的列表视窗
			XM_printf(">>>>>PhotoMenuOnSystemEvent,2\r\n");
			XM_JumpWindow(XMHWND_HANDLE(Desktop));
			break;

		default:
			break;
	}
}
//消息处理函数定义
XM_MESSAGE_MAP_BEGIN(PhotoMenu)
	XM_ON_MESSAGE (XM_PAINT, PhotoMenuOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, PhotoMenuOnKeyDown)
	XM_ON_MESSAGE (XMKEY_REPEAT, PhotoMenuOnKeyRepeat)
	XM_ON_MESSAGE (XM_KEYUP, PhotoMenuOnKeyUp)
	XM_ON_MESSAGE (XM_TIMER, PhotoMenuOnTimer)
	XM_ON_MESSAGE (XM_ENTER, PhotoMenuOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, PhotoMenuOnLeave)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, PhotoMenuOnSystemEvent)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(10, 20, 30, 40, PhotoMenu, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
