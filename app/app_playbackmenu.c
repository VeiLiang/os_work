//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_FormatSetting.c
//	  ͨ��Menuѡ���б�ѡ�񴰿�(ֻ��ѡ��һ��ѡ��)
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
	int					nPageNum;					// ��ǰpage
	int					nCurItem;					// ��ǰѡ��Ĳ˵���,0-5

	int					nItemCount;					// �˵������,����
	int					nVisualCount;				// �Ӵ�����ʾ����Ŀ����
	int					nTotalPage;					// ��page;
	
	BYTE				videotype;					//����,��Ƶ����ͼƬ
	
    HANDLE				hMoniorSwitchControl;		//ͣ�����
	char				fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	XMFILEFIND			fileFind;
};

typedef struct tagPLAYBACKMENUDATA{
	int				nCurItem;					//�طŲ˵�,�б�λ��
	int				nItemCount;					//�˵������
	int				videoCurItem;				//videolist��ǰѡ�е�item
	
	struct _tagVIDEOLIST *videolist;
	
    u8_t            type;                //����
	u8_t            curch;            	 //��ǰ��ʾͨ��
	u32_t			curpage;			 //��ǰpage;
	u32_t 			totalpage;			 //��page;
}PLAYBACKMENUDATA;

static u8_t Video_First_Enter_Type;
static int Photo_curpage_num = 1;//ͼƬҳ��
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
	
	//������page��ʱ����Ҫ�ж�item�Ƿ���Ч
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
	{//��Ҫɾ�����ļ�,���������ļ���,ֱ�ӵ���
		XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0); //�����ͼƬԤ������ͼƬ�б�OSD_LAYER_0����ͼƬ����
		// ��alert����
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

		//ɾ����ɺ�,��Ҫ���¼��㵱ǰpage��item

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
		
		XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0); //�����ͼƬԤ������ͼƬ�б�OSD_LAYER_0����ͼƬ����
		// ��alert����
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

	XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0); //�����ͼƬԤ������ͼƬ�б�OSD_LAYER_0����ͼƬ����

	// ��alert����
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
			// ʧ�ܷ��ص�����
			XM_PullWindow (0);
			return;
		}
		reset_replay_menu_timeout();
		XM_printf(">>>>page num:%d\r\n", videolist->nPageNum);
		XM_printf(">>>>cur item:%d\r\n", videolist->nCurItem);
		XM_printf(">>>>total page num:%d\r\n", videolist->nTotalPage);//videolist����������page

		//����ͨ��������page
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
		ReplayMenuData->videoCurItem = videolist->nCurItem;//videolist��ѡ�е��ļ�index
		ReplayMenuData->curch = AP_GetMenuItem(APPMENUITEM_REPLAY_CH);
		ReplayMenuData->curpage = videolist->nPageNum;//videolist��ѡ�е��ļ�index
		ReplayMenuData->videolist = videolist;
		
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData(XMHWND_HANDLE(PlayBackMenu), ReplayMenuData);
	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf(">>>>>>>>>>>>>>>>>play back menu Pull\n");
	}
    XM_SetTimer(XMTIMER_SETTINGVIEW, 500); //������ʱ��
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
			// �ͷ�˽�����ݾ��
			XM_free (ReplayMenuData);
		}
		XM_printf(">>>>>play back menu Exit.......\r\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
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

	// ��ʾ������
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
	
	/*********��title***********/
	rect = rc;rect.bottom = 42;
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUTITLEBARBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title ����
	rect = rc;rect.bottom = 42;
	AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_REPLAYMENU, hwnd, &rect, XMGIF_DRAW_POS_CENTER);//title ����
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title�ָ���

	//itemѡ�б���
    rect = rc;
	rect.left = 0;
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (ReplayMenuData->nCurItem)  * APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//ѡ�б���ɫ
	
	//����
    rect = rc;
	rect.top = 42+13;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_TYPE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//����
	
    //��ʾ��Ƶ/��Ƭ
    rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
	if(ReplayMenuData->type==TYPE_VIDEO)
	{
		AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_VIDEO, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//Ĭ����Ƶ
	}
	else
	{
		AP_RomImageDrawByMenuID(AP_ID_VIDEO_TITLE_ALBUM, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//Ĭ����Ƭ
	}
	
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(42+APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title�ָ���
	
	//ͨ��
    rect = rc;
	rect.top = 42+62+13;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_CH, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//ͨ��
    //��ʾCH1/CH2
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
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title�ָ���
	
	//ҳ��
    rect = rc;
	rect.top = 42+62+62+13;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_PAGE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//ҳ��
    //��ʾ��ǰҳ��/��ҳ��
    rect.left = APP_POS_ITEM5_MENUFLAG_X*3;

	//����ͨ�������ͣ�������page
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
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title�ָ���
	
	//ɾ��
    rect = rc;
	rect.top = 42+62+62+62+18;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_DELETE_ONE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    //��ʾCH1/CH2
    rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
	AP_RomImageDrawByMenuID(AP_ID_COMMON_RIGHT_ARROW, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(42+APP_POS_ITEM5_LINEHEIGHT*4);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title�ָ���
	
	//ɾ��ȫ��
    rect = rc;
	rect.top = 42+62+62+62+62+18;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_DELETE_ALL, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    //��ʾCH1/CH2
    rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
	AP_RomImageDrawByMenuID(AP_ID_COMMON_RIGHT_ARROW, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(42+APP_POS_ITEM5_LINEHEIGHT*4);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title�ָ���

    rect = rc;
	rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * 1);
	rect.top = (XMCOORD)(42+APP_POS_ITEM5_LINEHEIGHT*5);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//title�ָ���
	
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
			ReplayMenuDat->curpage = 1;//�л����ͣ�Ĭ�ϵ���PAGE 1
			XM_printf(">>>>>>>>>ReplayMenuDat->type:%d\r\n", ReplayMenuDat->type);
			break;
			
	    case REPLAYMENU_ITEM1_CH://ͨ��ѡ��
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
				ReplayMenuDat->curpage = 1;//�л�ͨ����Ĭ�ϵ���PAGE 1
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
				ReplayMenuDat->curpage = 1;//�л�ͨ����Ĭ�ϵ���PAGE 1
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

		case REPLAYMENU_ITEM3_DEL://ɾ��ѡ���ļ�

			break;

		case REPLAYMENU_ITEM3_DELALL://ɾ�������ļ�

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
		case VK_AP_MENU://������˳��طŲ˵�
			XM_printf(">>>VK_AP_MENU...\r\n");
			XM_printf(">>>>>>>>ReplayMenuData->curpage:%d\r\n", ReplayMenuData->curpage);

			if( (ReplayMenuData->type==TYPE_VIDEO) && (Video_First_Enter_Type==TYPE_VIDEO) )
			{//��Ƶѡ��
				ReplayMenuData->videolist->nPageNum = ReplayMenuData->curpage;
				XM_PullWindow(0);
			}
			else if( (ReplayMenuData->type==TYPE_PIC) && (Video_First_Enter_Type==TYPE_VIDEO) )
			{
				XM_printf(">>>type other photo...\r\n");
				XM_printf(">>>>>>>>jump album list view..........\r\n");
				Photo_curpage_num = ReplayMenuData->curpage;//����ͼƬ�ط�ģʽ,��������������
				XM_JumpWindow(XMHWND_HANDLE(AlbumListView));//��ջ��UIȫ������,����ָ��UI
			}
			APP_SaveMenuData();
			break;

		case REMOTE_KEY_DOWN:
        case VK_AP_SWITCH://ȷ�ϼ������ڼ�
			XM_printf(">>>VK_AP_SWITCH...\r\n");
			reset_replay_menu_timeout();

			if(ReplayMenuData->nCurItem==REPLAYMENU_ITEM3_DEL)
			{//����ȷ�Ͽ�(videoListViewListData->nPageNum-1)*6 + i
				XM_printf(">>>>>>>>>nItemCount:%d\r\n", ReplayMenuData->videolist->nItemCount);
				XM_printf(">>>>>>>>>ReplayMenuData->videoCurItem:%d\r\n", ReplayMenuData->videoCurItem);

				if(ReplayMenuData->videolist->nItemCount>0)
				{//��file
					if(ReplayMenuData->videolist->videotype==VIDEOITEM_CIRCULAR_VIDEO_MODE)
					{//��Ƶѡ��
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
										(float)20.0,		// �Զ��ر�ʱ��
										APP_ALERT_BKGALPHA,
										Video_DeleteOneFileCallback,	// �����ص�����
										ReplayMenuData,
										XM_VIEW_ALIGN_CENTRE,		// ���ж���
										XM_ALERTVIEW_OPTION_ENABLE_CALLBACK		// ALERTVIEW��ͼ�Ŀ���ѡ��
										);
					}
				}
			}
			else if(ReplayMenuData->nCurItem==REPLAYMENU_ITEM3_DELALL)
			{//����ȷ�Ͽ�
				XM_printf(">>>>>>>>>nItemCount:%d\r\n", ReplayMenuData->videolist->nItemCount);
				if(ReplayMenuData->videolist->nItemCount>0)
				{//��file
					if(ReplayMenuData->videolist->videotype==VIDEOITEM_CIRCULAR_VIDEO_MODE)
					{//��Ƶѡ��
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
										(float)20.0,		// �Զ��ر�ʱ��
										APP_ALERT_BKGALPHA,
										Video_DeleteAllFileCallback,	// �����ص�����
										ReplayMenuData,
										XM_VIEW_ALIGN_CENTRE,		// ���ж���
										XM_ALERTVIEW_OPTION_ENABLE_CALLBACK		// ALERTVIEW��ͼ�Ŀ���ѡ��
										);
					}				
				}
			}
			else
			{
				AdjustReplayMenuDat(ReplayMenuData, ReplayMenuData->nCurItem, 1);
				XM_printf(">>>>>>>>>>>ReplayMenuData->curpage:%d\r\n", ReplayMenuData->curpage);
				//ˢ��
				XM_InvalidateWindow();
				XM_UpdateWindow();
			}
            break;

		case REMOTE_KEY_UP:
        case VK_AP_FONT_BACK_SWITCH:
			XM_printf(">>>VK_AP_FONT_BACK_SWITCH...\r\n");
			if( (ReplayMenuData->type==TYPE_VIDEO) && (Video_First_Enter_Type==TYPE_VIDEO) )
			{//��Ƶѡ��
				ReplayMenuData->videolist->nPageNum = ReplayMenuData->curpage;
				XM_PullWindow(0);
			}
            break;

        case REMOTE_KEY_RIGHT:
        case VK_AP_UP://ѡ����һ��item
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
		case VK_AP_DOWN://ѡ����һ��item
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
			//ˢ��
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
	    XM_PullWindow(0);//����������ʾ��������
	}
}

static VOID PlayBackMenuOnSystemEvent(XMMSG *msg)
{
	XM_printf(">>>>>>>>PlayBackMenuOnSystemEvent, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:			// SD���γ��¼�
			XM_printf(">>>>>PlayBackMenuOnSystemEvent,1\r\n");
			XM_BreakSystemEventDefaultProcess (msg);		// ������ȱʡ����

			#if 0
			// ������Ϣ���²��뵽��Ϣ���ж���
			XM_InsertMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
			XM_JumpWindowEx (XMHWND_HANDLE(Desktop), HWND_CUSTOM_DEFAULT, XM_JUMP_POPDEFAULT);
			//XM_JumpWindow (XMHWND_HANDLE(Desktop));	// ��ת������
			#endif
			//XM_PullWindow(0);					   // ������ǰ���б��Ӵ�
			XM_printf(">>>>>PlayBackMenuOnSystemEvent,2\r\n");
			XM_JumpWindow(XMHWND_HANDLE(Desktop));
			break;

		default:
			break;
	}
}

//��Ϣ����������
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

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(10, 20, 30, 40, PlayBackMenu, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
