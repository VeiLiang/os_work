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

#define PHOTOMENU_ITEM0_TYPE	0
#define PHOTOMENU_ITEM1_CH		1
#define PHOTOMENU_ITEM2_PAGE	2
#define PHOTOMENU_ITEM3_DEL		3
#define PHOTOMENU_ITEM3_DELALL	4

struct _tagPHOTOLIST{
	int					nPageNum;					// ��һ�����ӵĲ˵���
	int					nCurItem;					// ��ǰѡ��Ĳ˵���

	int					nItemCount;					// �˵������
	int					nVisualCount;				// �Ӵ�����ʾ����Ŀ����
	int					nTotalPage;					//��page;

	BYTE				videotype;

    HANDLE				hMoniorSwitchControl;		//ͣ�����
	char				fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	XMFILEFIND			fileFind;
};

typedef struct tagPHOTOMENUDATA{
	int				nCurItem;					//�طŲ˵�,�б�λ��
	int				nItemCount;					//�˵������
	int				videoCurItem;				//videolist��ǰѡ�е�item
	
	struct _tagPHOTOLIST *photolist;
	
    u8_t            type;                //����
	u8_t            curch;            	 //��ǰ��ʾͨ��
	u32_t			curpage;			 //��ǰpage;
	u32_t 			totalpage;			 //��page;
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
	
	//������page��ʱ����Ҫ�ж�item�Ƿ���Ч
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

		//ɾ����ɺ�,��Ҫ���¼��㵱ǰpage��item
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
		
		XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0); //�����ͼƬԤ������ͼƬ�б�OSD_LAYER_0����ͼƬ����
		// ��alert����
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

	XM_osd_framebuffer_release(XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0); //�����ͼƬԤ������ͼƬ�б�OSD_LAYER_0����ͼƬ����
	// ��alert����
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
			// ʧ�ܷ��ص�����
			XM_PullWindow (0);
			return;
		}
		reset_photo_menu_timeout();
		XM_printf(">>>>page num:%d\r\n", photolist->nPageNum);
		XM_printf(">>>>cur item:%d\r\n", photolist->nCurItem);
		XM_printf(">>>>total page num:%d\r\n", photolist->nTotalPage);//videolist����������page

		//����ͨ��������page
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
		PhotoMenuData->videoCurItem = photolist->nCurItem;//videolist��ѡ�е��ļ�index
		PhotoMenuData->curch = AP_GetMenuItem(APPMENUITEM_REPLAY_CH);
		PhotoMenuData->curpage = photolist->nPageNum;//videolist��ѡ�е��ļ�index
		PhotoMenuData->photolist = photolist;
	
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData(XMHWND_HANDLE(PhotoMenu), PhotoMenuData);
	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf(">>>>>>>>>>>>>>>>>play back menu Pull\n");
	}
    XM_SetTimer(XMTIMER_SETTINGVIEW, 1000); //������ʱ��
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
			// �ͷ�˽�����ݾ��
			XM_free (PhotoMenuData);
		}
		XM_printf(">>>>>photo menu Exit.....\r\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
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

	// ��ʾ������
	XM_GetDesktopRect (&rc);
	XM_FillRect(hwnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	old_alpha = XM_GetWindowAlpha (hwnd);

	XM_SetWindowAlpha(hwnd, 255);
	
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
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (PhotoMenuData->nCurItem)  * APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID(AP_ID_COMMON_MENUITEMBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//ѡ�б���ɫ
	
	//����
    rect = rc;
	rect.top = 42+13;
    rect.left = APP_POS_ITEM5_MENUNAME_X;
    AP_RomImageDrawByMenuID(AP_ID_VIDEO_BUTTON_TYPE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);//����
	
    //��ʾ��Ƶ/��Ƭ
    rect.left = APP_POS_ITEM5_MENUFLAG_X*3;
	if(PhotoMenuData->type==TYPE_VIDEO)
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
			PhotoMenuData->curpage = 1;//�л����ͣ�Ĭ�ϵ���PAGE 1
			XM_printf(">>>>>>>>>PhotoMenuData->type:%d\r\n", PhotoMenuData->type);
			break;
			
	    case PHOTOMENU_ITEM1_CH://ͨ��ѡ��
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
				PhotoMenuData->curpage = 1;//�л�ͨ����Ĭ�ϵ���PAGE 1
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
				PhotoMenuData->curpage = 1;//�л�ͨ����Ĭ�ϵ���PAGE 1
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

		case PHOTOMENU_ITEM3_DEL://ɾ��ѡ���ļ�

			break;

		case PHOTOMENU_ITEM3_DELALL://ɾ�������ļ�

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
		case VK_AP_MENU://������˳��طŲ˵�
			XM_printf(">>>VK_AP_MENU...\r\n");
			XM_printf(">>>>>>>>PhotoMenuData->curpage:%d\r\n", PhotoMenuData->curpage);

			if( (PhotoMenuData->type==TYPE_PIC) && (Photo_First_Enter_Type==TYPE_PIC) )
			{//��Ƶѡ��
				PhotoMenuData->photolist->nPageNum = PhotoMenuData->curpage;
				XM_PullWindow(0);
			}
			else if( (PhotoMenuData->type==TYPE_VIDEO) && (Photo_First_Enter_Type==TYPE_PIC) )
			{
				XM_printf(">>>type other video...\r\n");
				XM_printf(">>>>>>>>jump video list view..........\r\n");
				Video_curpage_num = PhotoMenuData->curpage;//����ͼƬ�ط�ģʽ,��������������
				XM_JumpWindow(XMHWND_HANDLE(VideoListView));//��ջ��UIȫ������,����ָ��UI
			}
			APP_SaveMenuData();
			break;
			
        case VK_AP_SWITCH://ȷ�ϼ������ڼ�
			XM_printf(">>>VK_AP_SWITCH...\r\n");
			reset_photo_menu_timeout();

			if(PhotoMenuData->nCurItem==PHOTOMENU_ITEM3_DEL)
			{//����ȷ�Ͽ�(videoListViewListData->nPageNum-1)*6 + i
				XM_printf(">>>>>>>>>nItemCount:%d\r\n", PhotoMenuData->photolist->nItemCount);
				if(PhotoMenuData->photolist->nItemCount>0)
				{//��file
					{//ͼƬѡ��
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
										Photo_DeleteOneFileCallback,	// �����ص�����
										PhotoMenuData,
										XM_VIEW_ALIGN_CENTRE,		// ���ж���
										XM_ALERTVIEW_OPTION_ENABLE_CALLBACK		// ALERTVIEW��ͼ�Ŀ���ѡ��
										);
					}
				}
			}
			else if(PhotoMenuData->nCurItem==PHOTOMENU_ITEM3_DELALL)
			{//����ȷ�Ͽ�
				XM_printf(">>>>>>>>>nItemCount:%d\r\n", PhotoMenuData->photolist->nItemCount);
				if(PhotoMenuData->photolist->nItemCount>0)
				{//��file					
					{//ͼƬѡ��
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
										Photo_DeleteAllFileCallback,	// �����ص�����
										PhotoMenuData,
										XM_VIEW_ALIGN_CENTRE,		// ���ж���
										XM_ALERTVIEW_OPTION_ENABLE_CALLBACK		// ALERTVIEW��ͼ�Ŀ���ѡ��
										);
					}
				}
			}
			else
			{
				AdjustPhotoMenuDat(PhotoMenuData, PhotoMenuData->nCurItem, 1);
				XM_printf(">>>>>>>>>>>PhotoMenuData->curpage:%d\r\n", PhotoMenuData->curpage);
				//ˢ��
				XM_InvalidateWindow();
				XM_UpdateWindow();
			}
            break;

        case VK_AP_UP://ѡ����һ��item
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
			
		case VK_AP_DOWN://ѡ����һ��item
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
			//ˢ��
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
		case SYSTEM_EVENT_CARD_UNPLUG:			// SD���γ��¼�
			XM_printf(">>>>>PhotoMenuOnSystemEvent,1\r\n");
			XM_BreakSystemEventDefaultProcess (msg);		// ������ȱʡ����

			#if 0
			// ������Ϣ���²��뵽��Ϣ���ж���
			XM_InsertMessage (XM_SYSTEMEVENT, msg->wp, msg->lp);
			XM_JumpWindowEx (XMHWND_HANDLE(Desktop), HWND_CUSTOM_DEFAULT, XM_JUMP_POPDEFAULT);
			//XM_JumpWindow (XMHWND_HANDLE(Desktop));	// ��ת������
			#endif
			//XM_PullWindow(0);					   // ������ǰ���б��Ӵ�
			XM_printf(">>>>>PhotoMenuOnSystemEvent,2\r\n");
			XM_JumpWindow(XMHWND_HANDLE(Desktop));
			break;

		default:
			break;
	}
}
//��Ϣ����������
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

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(10, 20, 30, 40, PhotoMenu, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
