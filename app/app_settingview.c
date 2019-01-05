//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_settingview.c
//	  ���ô���
//
//	Revision history
//
//		2012.09.10	ZhuoYongHong Initial version
//
//****************************************************************************


#include "app.h"
#include "app_menuid.h"
#include "app_menuoptionview.h"
#include "types.h"
#include "xm_video_task.h"
#include "ark1960.h"
#include "rtc.h"
#include "rxchip.h"
#include "xm_app_menudata.h"
#include "app_settingview.h"

#include "app_submenu_color.h"
#include "app_submenu_datetime.h"

#define MAIN_MENU_STATUS	0
#define SUB_MENU_STATUS		1

extern XMCOORD AP_TextOutNumString(HANDLE hWnd, XMCOORD x, XMCOORD y, u8_t num_width,char *text, int size,u8_t type);
extern void AP_OpenDefaultParamView (XMBOOL bPushView);
extern u8 get_parking_trig_status(void);
static void drawMenuBar(HANDLE hWnd,XMRECT *menu_rc);

// �����á����ڰ�ť�ؼ�����
#define	SETTINGBTNCOUNT	2
static const XMBUTTONINFO settingBtn[SETTINGBTNCOUNT] = {
	 /*
	 {
		VK_AP_MENU,		SETTING_COMMAND_MODIFY,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_MODIFY
	 },
	 */
	{
		VK_AP_MENU,	SETTING_COMMAND_RECORD,	AP_ID_COMMON_MODE,		AP_ID_SETTING_RECORD
	},
	{
		VK_AP_MODE,	SETTING_COMMAND_RETURN,	AP_ID_COMMON_OK,	AP_ID_BUTTON_RETURN
	},
};


static MENU_ICON menu_icon_array[] = 
{
    {AP_ID_MENU_ICON_DATE,  AP_ID_MENU_NORMAL,  AP_ID_MENU_ACTIVE,  AP_ID_TXT_DATE},
    {AP_ID_MENU_ICON_MOT,   AP_ID_MENU_NORMAL,  AP_ID_MENU_ACTIVE,  AP_ID_TXT_MOT},
    {AP_ID_MENU_ICON_DEC,   AP_ID_MENU_NORMAL,  AP_ID_MENU_ACTIVE,  AP_ID_TXT_DEC},
    {AP_ID_MENU_ICON_COLOR, AP_ID_MENU_NORMAL,  AP_ID_MENU_ACTIVE,  AP_ID_TXT_COLOR},
    {AP_ID_MENU_ICON_REC,   AP_ID_MENU_NORMAL,  AP_ID_MENU_ACTIVE,  AP_ID_TXT_REC},
    {AP_ID_MENU_ICON_SYS,   AP_ID_MENU_NORMAL,  AP_ID_MENU_ACTIVE,  AP_ID_TXT_SYS},
    {0,                     AP_ID_MENU_ICON_HOME,          AP_ID_MENU_ICON_HOME_ACTIVE,    0},
};


//static BOOL update_time = FALSE;    //���ڶ�ʱˢ�²˵�����ʱ��

// ���ؿؼ��Ļص�����
static void setting_switch_callback (void *private_data, unsigned int state)
{
	unsigned int type = (unsigned int)private_data;
	if(type == APPMENUITEM_VOICE_PROMPTS)
	{
		if(state == 1)
		{
			AP_SetMenuItem (type, state);
			// ����˵����õ�����洢�豸
			AP_SaveMenuData (&AppMenuData);
		}
		else
		{
			AP_SetMenuItem (type, state);
			// ����˵����õ�����洢�豸
			AP_SaveMenuData (&AppMenuData);
		}
	}
	else if(type == APPMENUITEM_MIC)
	{
		AP_SetMenuItem (type, state);
		// ����˵����õ�����洢�豸
		AP_SaveMenuData (&AppMenuData);

	}
	else
	{
		AP_SetMenuItem (type, state);
		AP_SaveMenuData (&AppMenuData);
	}
}

u8_t Delay_Close_View = 0;
static int first_paint = TRUE;
static int submenu_first_paint = TRUE;
VOID SettingViewOnEnter (XMMSG *msg)
{
	SETTINGVIEWDATA *settingViewData;
	//XMPOINT pt ;
	if(msg->wp == 0)
	{
		// ����δ��������һ�ν���

		// ����˽�����ݾ��
		settingViewData = XM_calloc (sizeof(SETTINGVIEWDATA));
        SETTINGMENUDATA *SettingData = settingViewData->SettingData;

		if(settingViewData == NULL)
		{
			XM_printf ("settingViewData XM_calloc failed\n");
			// ʧ�ܷ��ص�����
			XM_PullWindow (0);
			return;
		}

		// ��ʼ��˽������
		//SettingData->nVolOnOff = AP_GetAudo_PWM();
		settingViewData->nItemCount = 6;
		settingViewData->nCurItem = 0;
		settingViewData->nTouchItem = -1;


		// Switch���ؿؼ���ʼ�� (��������)
		#ifdef _XM_VOICE_PROMPTS_ENABLE_
		pt.x = 0;
		pt.y = 0;
		settingViewData->hVoiceSwitchControl = AP_SwitchButtonControlInit (
			XMHWND_HANDLE(SettingView),
			&pt,
			AppMenuData.voice_prompts,
			setting_switch_callback,
			(void *)APPMENUITEM_VOICE_PROMPTS);
		#endif

		#ifdef _XM_MIC_SWITCH_ENABLE
		// Switch���ؿؼ���ʼ�� (¼��)
		settingViewData->hMicSwitchControl = AP_SwitchButtonControlInit (
			XMHWND_HANDLE(SettingView),
			&pt,
			AppMenuData.mic,
			setting_switch_callback,
			(void *)APPMENUITEM_MIC);
		#endif

		#ifdef _XM_KEY_BEEP_ENABLE_
		settingViewData->hKeySwitchControl = AP_SwitchButtonControlInit (
			XMHWND_HANDLE(SettingView),
			&pt,
			AppMenuData.key,
			setting_switch_callback,
			(void *)APPMENUITEM_KEY);
		#endif

		// ��ť�ؼ���ʼ��
		//AP_ButtonControlInit (&settingViewData->btnControl, SETTINGBTNCOUNT, XMHWND_HANDLE(SettingView), settingBtn);

		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(SettingView), settingViewData);
		XM_SetWindowAlpha (XMHWND_HANDLE(SettingView), 255);
	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("Setting Pull\n");
	}

	// ������ʱ�������ڲ˵�������
	// ����x��Ķ�ʱ��
	XM_SetTimer (XMTIMER_SETTINGVIEW, 500);
}

static VOID SettingViewOnLeave (XMMSG *msg)
{
	// ɾ����ʱ��
    XM_KillTimer(XMTIMER_SETTINGVIEW);
    
    //update_time = FALSE;
    first_paint = TRUE;

	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		SETTINGVIEWDATA *settingViewData = (SETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SettingView));
		// �ͷ����з������Դ
		if(settingViewData)
		{
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&settingViewData->btnControl);

			#ifdef _XM_VOICE_PROMPTS_ENABLE_
			if(settingViewData->hVoiceSwitchControl)
				AP_SwitchButtonControlExit (settingViewData->hVoiceSwitchControl);
			#endif

			#ifdef _XM_MIC_SWITCH_ENABLE
			if(settingViewData->hMicSwitchControl)
				AP_SwitchButtonControlExit (settingViewData->hMicSwitchControl);
			#endif

			#ifdef _XM_KEY_BEEP_ENABLE_
			if(settingViewData->hKeySwitchControl)
				AP_SwitchButtonControlExit (settingViewData->hKeySwitchControl);
			#endif

			// �ͷ�˽�����ݾ��
			XM_free (settingViewData);
		}
		// ���ô��ڵ�˽�����ݾ��Ϊ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(SettingView), NULL);
		XM_printf ("Setting Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("Setting Push\n");
	}
}

extern u8_t ACC_Select;
unsigned char  MenuOption_VOL_Close = FALSE;//������������,��Ҫ�Ե�ǰѡ����м�һ

static void GetMenuItemPos(XMRECT *rc,SETTINGVIEWDATA *settingViewData,int item_index)
{
    if(item_index >= settingViewData->nItemCount )
    {
        return;
    }
    
    if(item_index == SETTING_MENU_COLOR)
    {
    	rc->left = 0;
    	rc->top = 600 - 100;
    	rc->right = 174;
    	rc->bottom = 600;
    	return ;
    }

    rc->left = (item_index - 1) * 170 + 174;
    rc->top = 500;
    rc->right = rc->left + 170;
    rc->bottom = 600;
}

void getListViewItemRect(const XMRECT *rect, XMRECT *item_rect,int item_index )
{
    item_rect->left = rect->left;
    item_rect->right = rect->right;
    item_rect->top = rect->top + LISTVIEW_TOP_OFFSET + item_index * (LISTVIEW_ITEM_HEIGHT + LISTVIEW_ITEM_SPACE);
    item_rect->bottom = item_rect->top + LISTVIEW_ITEM_HEIGHT;
}

static void drawListViewItem(HANDLE hWnd,XMRECT *menu_rc,int item_index, unsigned int icon_id,unsigned int text_id,unsigned value_id,SETTINGVIEWDATA *settingViewData)
{
    XMRECT rect,item_rect;
    getListViewItemRect(menu_rc,&item_rect,item_index);
    rect.left = item_rect.left;
    rect.top = item_rect.top;
    rect.right = rect.left + 65;
    rect.bottom = item_rect.bottom;
    if(settingViewData->curSecSubmenuIndex == item_index)
    {
        AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_ITEM_A, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
    }
    else
    {
        AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_ITEM_N, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
    }
    
    rect.left = item_rect.left + 65;
    rect.right = rect.left + 50;
    AP_RomImageDrawByMenuID(icon_id, hWnd, &rect, XMGIF_DRAW_POS_CENTER);

    rect.left = rect.right + 15;
    rect.right = rect.left + 200;
    AP_RomImageDrawByMenuID(text_id, hWnd, &rect, XMGIF_DRAW_POS_CENTER);

    
    rect.right = menu_rc->right;
    rect.left = rect.right - 100;
    rect.top = rect.top + 8;
    
    AP_RomImageDrawByMenuID(value_id, hWnd, &rect, XMGIF_DRAW_POS_RIGHT);
}

static void drawMenuBar(HANDLE hWnd,XMRECT *menu_rc)
{
    #if 0
    XMRECT rect,menu_bar_rect;
    menu_bar_rect.top  = menu_rc->top;
    menu_bar_rect.left = menu_rc->left;
    menu_bar_rect.right = menu_rc->right;
    menu_bar_rect.bottom = menu_bar_rect.top + 50;
    AP_RomImageDrawByMenuID(AP_ID_MENU_TOP_BAR,hWnd, &menu_bar_rect,XMGIF_DRAW_POS_CENTER);
    
    menu_bar_rect.bottom = menu_rc->bottom;
    menu_bar_rect.top  = menu_bar_rect.bottom - 56;
    menu_bar_rect.left = menu_rc->left;
    menu_bar_rect.right = menu_rc->right;
    AP_RomImageDrawByMenuID(AP_ID_MENU_BOTTOM_BAR,hWnd, &menu_bar_rect,XMGIF_DRAW_POS_CENTER);
    #endif
}

static void drawMenuBg(HANDLE hWnd,XMRECT *menu_rc)
{
    XMRECT rect;
    //AP_RomImageDrawByMenuID(AP_ID_MENU_BG, hWnd,menu_rc,XMGIF_DRAW_POS_CENTER);0xff293040
    //XM_FillRect(hWnd,menu_rc->left,menu_rc->top,menu_rc->right,menu_rc->bottom,0xff293040);
    //drawMenuBar(hWnd,menu_rc);
}

static void initSubMenu(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData)
{
    printf("--------------------------> initSubMenu --------------------------->\n");
    XMRECT rect,menu_bar_rect;
    XM_GetDesktopRect(&rect);
    XM_FillRect(hWnd,rect.left,rect.top,rect.right,rect.bottom,0x00000000);

	switch(settingViewData->nCurItem)
	{
		case SETTING_MENU_TIME:
		    drawMenuBg(hWnd,menu_rc);
			initDateView(hWnd,menu_rc,settingViewData);
			break;
		case SETTING_MENU_MOT:
		    drawMenuBg(hWnd,menu_rc);
		    initMotView(hWnd,menu_rc,settingViewData);
			break;
		case SETTING_MENU_DEC:
			break;
		case SETTING_MENU_COLOR:
		    drawMenuBar(hWnd,menu_rc);
			initColorView(hWnd,menu_rc,settingViewData);
			break;
		case SETTING_MENU_REC:
		    drawMenuBg(hWnd,menu_rc);
		    initVideoView(hWnd,menu_rc,settingViewData);
			break;
		case SETTING_MENU_SYS:
		    drawMenuBg(hWnd,menu_rc);
		    initSysMenu(hWnd,menu_rc,settingViewData);
			break;
		default:
			break;
	}
}

static void updateSubMenu(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData)
{
    printf("----------------------> updateSubmenu \n");
      
	switch(settingViewData->nCurItem)
	{
	    case SETTING_MENU_TIME:
			updateDateView(hWnd,menu_rc,settingViewData);
			break;
			
		case SETTING_MENU_MOT:
		    updateMot(hWnd,menu_rc,settingViewData);
			break;
			
		case SETTING_MENU_DEC:
			break;
			
		case SETTING_MENU_COLOR:
			updateColorView(hWnd,menu_rc,settingViewData);
			break;
			
		case SETTING_MENU_REC:
		    updateVideoView(hWnd,menu_rc,settingViewData);
			break;
			
		case SETTING_MENU_SYS:
		    updateSysMenu(hWnd,menu_rc,settingViewData);
			break;
			
		default:
			break;
	}
}

static void drawSubMenu(HANDLE hWnd,XMMSG *msg,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData)
{
    printf("-------------------------------> drawSubMenu -----------------------> \n");
    if(submenu_first_paint)
    {
        submenu_first_paint = FALSE; 
        //submenu_index = 0;
        initSubMenu(hWnd,menu_rc,settingViewData);
    }
    else
    {
        updateSubMenu(hWnd,menu_rc,settingViewData);
    }
}

//first item_index is 0
static void GetMenuItemRect(const XMRECT *rect, XMRECT *item_rect,int item_index,SETTINGVIEWDATA *settingViewData)
{
    //unsigned int top_offset = 80;
    //unsigned int y_offset = 60;
    //unsigned int bottom_offset = 50;
    
    unsigned int top_offset = 80;
    unsigned int y_offset = 40;
    unsigned int bottom_offset = 30;
    
    unsigned int item_count = 6;
    unsigned int item_columns = 3;
    unsigned int item_rows = 2;
    unsigned int txt_offset_height = 90;
#if 0
    if(item_index == 6)
    {
        item_rect->left = rect->left; 
        item_rect->right = rect->right;
        item_rect->bottom = rect->bottom;
        item_rect->top = item_rect->bottom - 60;
    }
    else
    #endif
    {
        unsigned int item_width = (rect->right - rect->left - 2*y_offset) / item_columns;
        unsigned int item_height = (rect->bottom - rect->top - top_offset - bottom_offset) / item_rows;

        item_rect->top = rect->top + top_offset + item_height * (item_index /item_columns)  ;
        item_rect->left = rect->left + y_offset + item_width * (item_index % item_columns);
        item_rect->right = item_rect->left + item_width;
        item_rect->bottom = item_rect->top + item_height - txt_offset_height;
    }
}

static void drawMenuItems(const XMRECT *rect,SETTINGVIEWDATA *settingViewData,HANDLE hWnd)
{
    XMRECT rc,item_rect;
    unsigned int txt_height = 65;

    int i;
    for(i = 0;i < settingViewData->nItemCount ; i++)
    {
        if( i == SETTING_MENU_HOME)
        {
            item_rect.left = rect->left;
            item_rect.right = rect->right;
            item_rect.bottom = rect->bottom;
            item_rect.top = item_rect.bottom - 54;
            AP_RomImageDrawByMenuID(AP_ID_MENU_BOTTOM_BAR, hWnd, &item_rect, XMGIF_DRAW_POS_CENTER);
            GetMenuItemRect(rect,&item_rect ,i,settingViewData);
            AP_RomImageDrawByMenuID(menu_icon_array[i].nor_bg,hWnd,&item_rect,XMGIF_DRAW_POS_CENTER);
        }
        else
        {
            GetMenuItemRect(rect,&item_rect,i,settingViewData);
            //XM_FillRect(hWnd,rect->left,rect->top,rect->right,rect->bottom,0xff000080);
            AP_RomImageDrawByMenuID(menu_icon_array[i].nor_bg, hWnd, &item_rect, XMGIF_DRAW_POS_CENTER);
            AP_RomImageDrawByMenuID(menu_icon_array[i].icon, hWnd, &item_rect, XMGIF_DRAW_POS_CENTER);
            item_rect.top = item_rect.bottom;
            item_rect.bottom = item_rect.top + txt_height;
            AP_RomImageDrawByMenuID(menu_icon_array[i].txt, hWnd, &item_rect, XMGIF_DRAW_POS_CENTER);
        }
    }
}

static void updateMenuHightLight(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData)
{
    XMRECT item_rect;

    unsigned int txt_height = 65;

    if(settingViewData->nLastItem <= settingViewData->nItemCount - 1)
    {
        if(settingViewData->nLastItem == 6)
        {
            item_rect.left = menu_rc->left;
            item_rect.right = menu_rc->right;
            item_rect.bottom = menu_rc->bottom;
            item_rect.top = item_rect.bottom - 54;
            AP_RomImageDrawByMenuID(AP_ID_MENU_BOTTOM_BAR, hWnd, &item_rect, XMGIF_DRAW_POS_CENTER);
            GetMenuItemRect(menu_rc,&item_rect ,settingViewData->nLastItem,settingViewData);
            AP_RomImageDrawByMenuID(menu_icon_array[settingViewData->nLastItem].nor_bg,hWnd,&item_rect,XMGIF_DRAW_POS_CENTER);
        }
        else
        {
            GetMenuItemRect(menu_rc,&item_rect ,settingViewData->nLastItem,settingViewData);
            AP_RomImageDrawByMenuID(menu_icon_array[settingViewData->nLastItem].nor_bg,hWnd,&item_rect,XMGIF_DRAW_POS_CENTER);
            AP_RomImageDrawByMenuID(menu_icon_array[settingViewData->nLastItem].icon,hWnd,&item_rect,XMGIF_DRAW_POS_CENTER);   
            
            item_rect.top = item_rect.bottom;
            item_rect.bottom = item_rect.top + txt_height;
            AP_RomImageDrawByMenuID(menu_icon_array[settingViewData->nLastItem].txt, hWnd, &item_rect, XMGIF_DRAW_POS_CENTER);
        }
    }
    
    if(settingViewData->nCurItem <= settingViewData->nItemCount - 1)
    {
        if(settingViewData->nCurItem == 6)
        {
            item_rect.left = menu_rc->left;
            item_rect.right = menu_rc->right;
            item_rect.bottom = menu_rc->bottom;
            item_rect.top = item_rect.bottom - 54;
            AP_RomImageDrawByMenuID(AP_ID_MENU_BOTTOM_BAR_A, hWnd, &item_rect, XMGIF_DRAW_POS_CENTER);
            GetMenuItemRect(menu_rc,&item_rect ,settingViewData->nCurItem,settingViewData);
            AP_RomImageDrawByMenuID(menu_icon_array[settingViewData->nCurItem].act_bg,hWnd,&item_rect,XMGIF_DRAW_POS_CENTER);
        }
        else
        {
            GetMenuItemRect(menu_rc,&item_rect ,settingViewData->nCurItem,settingViewData);
            AP_RomImageDrawByMenuID(menu_icon_array[settingViewData->nCurItem].act_bg,hWnd,&item_rect,XMGIF_DRAW_POS_CENTER);
            AP_RomImageDrawByMenuID(menu_icon_array[settingViewData->nCurItem].icon,hWnd,&item_rect,XMGIF_DRAW_POS_CENTER);   
            
            item_rect.top = item_rect.bottom;
            item_rect.bottom = item_rect.top + txt_height;
            AP_RomImageDrawByMenuID(menu_icon_array[settingViewData->nCurItem].txt, hWnd, &item_rect, XMGIF_DRAW_POS_CENTER);
        }
    }
}

static void initMenu(HANDLE hWnd,XMRECT *desk_rc,XMRECT *rect,SETTINGVIEWDATA *settingViewData)
{
    XMRECT menu_bar_rect;
    settingViewData->curSecSubmenuIndex = 0;
    settingViewData->lastSecSubmenuIndex = 0;
    XM_FillRect(hWnd,desk_rc->left,desk_rc->top,desk_rc->right,desk_rc->bottom,0x00000000);
    XM_FillRect(hWnd,rect->left,rect->top,rect->right,rect->bottom,0xff293040);
}

static VOID SettingViewOnPaint (XMMSG *msg)
{
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>[SettingViewOnPaint]\r\n");
	
	SETTINGVIEWDATA *settingViewData;
	HANDLE hWnd = XMHWND_HANDLE(SettingView);

	settingViewData = (SETTINGVIEWDATA *)XM_GetWindowPrivateData (hWnd);

	XMRECT rc;
	XMRECT menu_rect;
	XMRECT sub_menu_rect;
    XM_GetDesktopRect (&rc);
    menu_rect.left = (rc.right - rc.left - 600)/2;
    menu_rect.top = (rc.bottom - rc.top - 400)/2;
    menu_rect.right = menu_rect.left + 600;
    menu_rect.bottom = menu_rect.top + 400;
    
    sub_menu_rect.left = (rc.right - rc.left - 800)/2;
    sub_menu_rect.top = (rc.bottom - rc.top - 480)/2;
    sub_menu_rect.right = sub_menu_rect.left + 800;
    sub_menu_rect.bottom = sub_menu_rect.top + 480;
    
    if(settingViewData->nmenuStatus == MAIN_MENU_STATUS)
    {
        if(first_paint)
        {
            initMenu(hWnd,&rc,&menu_rect,settingViewData);
			
            drawMenuItems(&menu_rect,settingViewData,hWnd); 
            first_paint = FALSE;
            submenu_first_paint = TRUE;
        }
        updateMenuHightLight(hWnd,&menu_rect,settingViewData);
    }
    else
    {
        first_paint = TRUE;
        if(settingViewData->nCurItem == SETTING_MENU_COLOR)
        {
            drawSubMenu(hWnd,msg,&menu_rect,settingViewData);
        }
        else
        {
            drawSubMenu(hWnd,msg,&sub_menu_rect,settingViewData);
        }
    }
}

static const DWORD dwLcdMenuOption[] = {
	AP_ID_SETTING_LCD_NEVERCLOSE,
	AP_ID_SETTING_LCD_S60,
	AP_ID_SETTING_LCD_S120,
	AP_ID_SETTING_LCD_S300,
};

static void LcdMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
#if 0
    if(dMenuOptionSelect == AP_ID_SETTING_LCD_NEVERCLOSE) {
        dMenuOptionSelect = AP_ID_SETTING_LCD_NEVERCLOSE;
    }else if(dMenuOptionSelect == 1) {
        dMenuOptionSelect = AP_SETTING_LCD_60S;
    }else if(dMenuOptionSelect == 2) {
        dMenuOptionSelect = AP_SETTING_LCD_120S;
    }else{
        dMenuOptionSelect = AP_SETTING_LCD_300S;
    }
	AP_SetMenuItem (APPMENUITEM_LCD, (BYTE)dMenuOptionSelect);
	AppMenuData.lcd = (BYTE)dMenuOptionSelect;
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
	#endif
}

static const DWORD dwShowMenuOption[] = {
	AP_ID_SETTING_Show_Select_0,
	AP_ID_SETTING_Show_Select_1,
	AP_ID_SETTING_Show_Select_2,
	AP_ID_SETTING_Show_Select_Double,
	AP_ID_SETTING_Show_Select_Left,
	AP_ID_SETTING_Show_Select_Right,
	AP_ID_SETTING_Show_Select_Up,
	AP_ID_SETTING_Show_Select_Vert,
};
static void ShowMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	if(dMenuOptionSelect == 0){
		XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_ISP601_ONLY);
	}else if(dMenuOptionSelect == 1) {
		XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_ITU656_ONLY);
	}else if(dMenuOptionSelect == 2) {
		XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_USB_ONLY);
	}else if(dMenuOptionSelect == 3) {
		XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_ISP601_ITU656);
	}else if(dMenuOptionSelect == 4) {
		XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_Left);
	}else if(dMenuOptionSelect == 5) {
		XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_Right);
	}else if(dMenuOptionSelect == 6) {
		XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_UP);
	}else if(dMenuOptionSelect == 7) {
		XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_Vert);
	}
}


static const DWORD dwLangMenuOption[] = {
	AP_ID_SETTING_LANG_CHINESE_SIMPLIFIED,
	AP_ID_SETTING_LANG_ENGLISH,
	//AP_ID_SETTING_LANG_RUSSIAN,
	//AP_ID_SETTING_LANG_CHINESE_TRADITIONAL,
	//AP_ID_SETTING_LANG_FRENCH,
	//AP_ID_SETTING_LANG_JAPANESE,
	//AP_ID_SETTING_LANG_KOREAN,
	//AP_ID_SETTING_LANG_GERMAN
};

static void LangMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	/*
	if(dMenuOptionSelect != AP_SETTING_LANG_CHINESE_SIMPLIFIED)
	{
		// DEMO�汾������������֧��
		XM_printf ("AP_SetMenuItem APPMENUITEM_LANG value=%d, only AP_SETTING_LANG_CHINESE_SIMPLIFIED supported(DEMO)\n",
			dMenuOptionSelect);
		return;
	}
	*/
	AppMenuData.lang = (BYTE)dMenuOptionSelect;
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
}

static const DWORD dwV1V2MenuOption[] = {
	AP_ID_SETTING_CLOSE,
	AP_ID_SETTING_OPEN,

};
static void V1V2MenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{

	AppMenuData.AUTO_Switch_OnOff = (BYTE)dMenuOptionSelect;
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
}


static const DWORD dwMicMenuOption[] = {
	AP_ID_SETTING_CLOSE,
	AP_ID_SETTING_OPEN,
};

static void MicMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	AP_SetMenuItem (APPMENUITEM_MIC, (BYTE)dMenuOptionSelect);
	//AppMenuData.lcd = (BYTE)dMenuOptionSelect;
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
}

static const DWORD dwDefaultMenuOption[] = {
	AP_ID_SETTING_CLOSE,
	AP_ID_SETTING_OPEN,

};
static void DefaultMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	 if(dMenuOptionSelect == 1) {
        AP_RestoreMenuData();
    }
}

static const DWORD dwNoSignedMenuOption[] = {
	AP_ID_SETTING_SINGED_BLUE,
	AP_ID_SETTING_SINGED_BLACK,

};

static void NoSignedMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
    AppMenuData.NoSigned = dMenuOptionSelect;
    AP_SaveMenuData (&AppMenuData);
}
static const DWORD dwKeyMenuOption[] = {
	AP_ID_SETTING_CLOSE,
	AP_ID_SETTING_OPEN,

};

static void KeyMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	AP_SetMenuItem (APPMENUITEM_KEY, (BYTE)dMenuOptionSelect);
	//AppMenuData.lcd = (BYTE)dMenuOptionSelect;
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
}

static const DWORD dwVoicePromptsMenuOption[] = {
	AP_ID_SETTING_CLOSE,
	AP_ID_SETTING_OPEN,

};

static void VoicePromptsMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	AP_SetMenuItem (APPMENUITEM_VOICE_PROMPTS, (BYTE)dMenuOptionSelect);
	//AppMenuData.lcd = (BYTE)dMenuOptionSelect;
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
}


void submenuNextCyc(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
    settingViewData->lastSecSubmenuIndex = settingViewData->curSecSubmenuIndex;
	if(settingViewData->curSecSubmenuIndex < settingViewData->secSubmenuItemCount - 1)
	{
		settingViewData->curSecSubmenuIndex  += 1;
	}
	else
	{
		settingViewData->curSecSubmenuIndex  = 0;
	}
}

void submenuPrivCyc(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
    settingViewData->lastSecSubmenuIndex = settingViewData->curSecSubmenuIndex;
	if(settingViewData->curSecSubmenuIndex == 0)
	{
		settingViewData->curSecSubmenuIndex = settingViewData->secSubmenuItemCount - 1;
	}
	else
	{
		settingViewData->curSecSubmenuIndex -= 1;
	}
}


#define ITEM_ADJUST_LIMIT_TIME   200

static void SubMenuKeyUpClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
    UINT32 cur_time = 0;
	static UINT32 last_time = 0;
    cur_time = XM_GetTickCount();

    if( cur_time - last_time < ITEM_ADJUST_LIMIT_TIME)
    {
        return;
    }
    last_time = cur_time;
	switch(settingViewData->nCurItem)
	{
		case SETTING_MENU_TIME:
	        dateViewAdjust(msg,settingViewData,1);
		    XM_InvalidateWindow();
		    XM_UpdateWindow();
			break;
			
		case SETTING_MENU_MOT://����ѡ��
 			motViewKeyOkClick(msg,settingViewData);
			XM_InvalidateWindow();
			XM_UpdateWindow();
			break;
			
		case SETTING_MENU_DEC:								// LCD
			break;
			
		case SETTING_MENU_COLOR:
			if(colorViewAdjust(msg,settingViewData,1))
			{
			    //AP_SaveMenuData(&AppMenuData);    //�˵������˳�ʱ����
			    XM_InvalidateWindow();
			    XM_UpdateWindow();
			}
			break;
			
		case SETTING_MENU_REC:
	        videoViewKeyUpClick(msg,settingViewData);
		    XM_InvalidateWindow();
		    XM_UpdateWindow();
			break;
			
		case SETTING_MENU_SYS:
	        sysMenuKeyUpClick(msg,settingViewData);
		    XM_InvalidateWindow();
		    XM_UpdateWindow();
			break;
			
		default:
			break;
	}
}


static void SubMenuKeyDownClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
    printf("--------------------> SubMenuKeyDownClick\n");
	static UINT32 last_time = 0;
	UINT32 cur_time = rRTC_CNTL;
	
    cur_time = XM_GetTickCount();
    printf("--------------------> SubMenuKeyDownClick cur_time : %d\n",cur_time);
    if( cur_time - last_time < ITEM_ADJUST_LIMIT_TIME)
    {
        return;
    }
    last_time = cur_time;
    switch(settingViewData->nCurItem)
	{
		case SETTING_MENU_TIME:
	        dateViewAdjust(msg,settingViewData,-1);
		    XM_InvalidateWindow();
		    XM_UpdateWindow();
			break;
			
		case SETTING_MENU_MOT://����ѡ��
 			motViewKeyOkClick(msg,settingViewData);
			XM_InvalidateWindow();
			XM_UpdateWindow();
			break;
			
		case SETTING_MENU_DEC:
			break;
			
		case SETTING_MENU_COLOR:
			if( colorViewAdjust(msg,settingViewData,-1))
			{
			    XM_InvalidateWindow();
			    XM_UpdateWindow();
			}
			break;
			
		case SETTING_MENU_REC:
	        videoViewKeyDownClick(msg,settingViewData);
		    XM_InvalidateWindow();
		    XM_UpdateWindow();
			break;
			
		case SETTING_MENU_SYS:
	        sysMenuKeyDownClick(msg,settingViewData);
		    XM_InvalidateWindow();
		    XM_UpdateWindow();
			break;
			
		default:
			break;
	}
}

static void sysmenu_alert_callback(VOID *UserPrivate, UINT uKeyPressed)
{
    printf("--------------------> sysmenu_alert_callback \n");

	XM_printf ("FS_Format ...\n");

	XM_KeyEventProc(XM_SYSTEMEVENT, 0);	
	FS_FORMAT_INFO FormatInfo;
	FormatInfo.SectorsPerCluster = 64;
	FormatInfo.NumRootDirEntries = 512;
	FormatInfo.pDevInfo = NULL;
	//ret = FS_Format ("mmc:0:", NULL);
	int ret = FS_Format("mmc:0:", &FormatInfo);
	XM_printf("FS_Format ret=%d\n", ret);

	if(ret == 0)
	{//��ʽ���ɹ�
		FS_CACHE_Clean ("");
		if(FS_Mount ("mmc:0:") >= 0)
		{
			XM_printf(">>>>>>>>fs mount success.....\r\n");
			//xm_open_volume_service("mmc:0:", 0);
		}
	}
	XM_KeyEventProc(XM_SYSTEMEVENT, 0x55);	
	//XM_PullWindow (0);
}

static void show_alert(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
    DWORD sysmenu_alert_normal_txt_id[2];
    DWORD sysmenu_alert_pressed_txt_id[2];
    sysmenu_alert_normal_txt_id[0] = AP_ID_SYS_ALERT_OK;
    sysmenu_alert_pressed_txt_id[0] = AP_ID_SYS_ALERT_OK;
    
    sysmenu_alert_normal_txt_id[1] = AP_ID_SYS_ALERT_CANCEL;
    sysmenu_alert_pressed_txt_id[1] = AP_ID_SYS_ALERT_CANCEL;
    int msg_id;
    int title_id;
    int btn_count;
    DWORD align_type;
    switch(settingViewData->curSecSubmenuIndex)
    {
        case 3:
            align_type = XM_VIEW_ALIGN_CENTRE;
            btn_count = 2;
            title_id = AP_ID_SYS_MENU_TITLE_FORMATE;
            msg_id = AP_ID_SYS_MENU_TXT_SD_FORMAT;
            break;
        case 5:
            align_type = XM_VIEW_ALIGN_CENTRE;
            btn_count = 2;
            title_id = AP_ID_SYS_MENU_TITLE_RESET;
            msg_id = AP_ID_SYS_MENU_TXT_RESET;
            break;
        case 7:
            align_type = XM_VIEW_ALIGN_BOTTOM;
            btn_count = 1;
            title_id = AP_ID_SYS_MENU_TITLE_VERSION;
            msg_id = AP_ID_SYS_MENU_TXT_RESET;
            break;
        default:
            break;
    }
    XM_OpenAlertView(title_id, 
                msg_id,
                btn_count, 
                sysmenu_alert_normal_txt_id,
                sysmenu_alert_pressed_txt_id,
                0xB4040404,
                0,
                1.0f,
                sysmenu_alert_callback,
    			NULL,
    			align_type,	
    			XM_ALERTVIEW_OPTION_ENABLE_CALLBACK
    			);
}

static void SubMenuKeyOkClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
	switch(settingViewData->nCurItem)
	{
		case SETTING_MENU_TIME://ʱ�����ò˵�
			dateViewKeyOkClick(msg,settingViewData);
			XM_InvalidateWindow();
			XM_UpdateWindow();
			break;
			
		case SETTING_MENU_MOT://�����˵�
		    motViewKeyOkClick(msg,settingViewData);
		    AP_SaveMenuData (&AppMenuData);
			XM_InvalidateWindow();
			XM_UpdateWindow();
			break;

		case SETTING_MENU_DEC://�طŽ���
			break;

		case SETTING_MENU_COLOR://��ʾ���ò˵�
			colorViewKeyOkClick(msg,settingViewData);
			XM_InvalidateWindow();
			XM_UpdateWindow();
			break;

		case SETTING_MENU_REC://¼�����ò˵�
		    videoViewKeyOkClick(msg,settingViewData);
        	XM_InvalidateWindow();
        	XM_UpdateWindow();
			break;

		case SETTING_MENU_SYS://ϵͳ���ò˵�
		    sysMenuKeyOkClick(msg, settingViewData);
			AP_SaveMenuData (&AppMenuData);
		    if(settingViewData->curSecSubmenuIndex != 3 &&
		       settingViewData->curSecSubmenuIndex != 5 &&
		       settingViewData->curSecSubmenuIndex != 7 )
		    {
        	    XM_InvalidateWindow();
        	    XM_UpdateWindow();
            }
			break;
			
	    case SETTING_MENU_HOME://��ҳ�˵�
	        settingViewData->nmenuStatus = 0;
        	//XM_InvalidateWindow();
        	//XM_UpdateWindow();
		    XM_PullWindow (XMHWND_HANDLE(Desktop));
	        break;
			
		default:
			break;
	}
}

static void SettingMenuKeyOkClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
	int nCurItem = settingViewData->nCurItem;
	#if 0
	if((nCurItem >= SETTING_MENU_VOL) &&(!AppMenuData.VolOnOff || (ACC_Select == 4)))
    {
        nCurItem ++;
    }
    
    if((nCurItem >= SETTING_MENU_DELAY) &&!AppMenuData.carbackdelay)
    {
        nCurItem ++;
    }
    
    if((nCurItem == SETTING_MENU_VOL) &&(AppMenuData.VolOnOff)&& (ACC_Select != 4)) {
        if(AppMenuData.Audio_PWM < 20)
        {
            AppMenuData.Audio_PWM ++;
        }
        HW_Auido_SetLevel(AppMenuData.Audio_PWM);
        XM_InvalidateWindow ();
	    XM_UpdateWindow ();
    }
    else 
    #endif
    if(settingViewData->nCurItem == SETTING_MENU_DEC)
    {
    	APPMarkCardChecking (1);		// ��ֹ��Ƶͨ��

		if(XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE) == DEVCAP_SDCARDSTATE_INSERT)
		{
    		XM_PushWindow (XMHWND_HANDLE(VideoListView));
		}
    	return;
    }
    else if(settingViewData->nCurItem == SETTING_MENU_HOME)
    {
        AP_SaveMenuData(&AppMenuData);
	    XM_PullWindow (0);
    }
    else
    {
        if(settingViewData->nmenuStatus == MAIN_MENU_STATUS)
        {
        	settingViewData->nmenuStatus = SUB_MENU_STATUS;// ��¼�˵�״̬Ϊ�Ӳ˵�״̬;
        }
        XM_InvalidateWindow();
        XM_UpdateWindow();
    }
}

static void SettingMenuKeyUpClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData,int nVisualCount)
{
//    AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
	settingViewData->nLastItem = settingViewData->nCurItem;
	settingViewData->nCurItem ++;


    if(settingViewData->nCurItem >= settingViewData->nItemCount)
	{
		// �۽�����һ��
		settingViewData->nCurItem = 0;
	}
}

static void SettingMenuKeyDownClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData,int nVisualCount)
{
//	AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);

	settingViewData->nLastItem = settingViewData->nCurItem;
	settingViewData->nCurItem --;

	if(settingViewData->nCurItem < 0)
	{
		// �۽������һ��
		settingViewData->nCurItem = settingViewData->nItemCount - 1;
	}
	printf("---------------------------> settingViewData->nCurItem : %d \n",settingViewData->nCurItem);
}

VOID SettingViewOnKeyDown (XMMSG *msg)
{
    if(AP_GetMenuItem(APPMENUITEM_POWER_STATE) == POWER_STATE_OFF)
    {
        return;
    }
	SETTINGVIEWDATA *settingViewData;
	XMRECT rc;

	settingViewData = (SETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SettingView));
	if(settingViewData == NULL)
		return;
    SETTINGMENUDATA *SettingData = settingViewData->SettingData;
	XM_GetWindowRect (XMHWND_HANDLE(SettingView), &rc);
    #if 0
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
    #endif
    Delay_Close_View = 0;
	
	// ������
	XM_Beep (XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
		// ���ϼ�
		case VK_AP_UP://+��
        case REMOTE_KEY_RIGHT:
			XM_printf(">>>>>>>>>>VK_AP_UP,settingViewData->nmenuStatus:%d\r\n", settingViewData->nmenuStatus);
			if(settingViewData->nmenuStatus == MAIN_MENU_STATUS)
			{
				SettingMenuKeyUpClick(msg,settingViewData,settingViewData->nItemCount);
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			else
			{
				SubMenuKeyUpClick(msg,settingViewData);
			}
			// ˢ��
			break;
		// ���¼�
		case VK_AP_AV:
        case REMOTE_KEY_LEFT:
	    case VK_AP_DOWN://-��
			XM_printf(">>>>>>>>>>VK_AP_DOWN,settingViewData->nmenuStatus:%d\r\n", settingViewData->nmenuStatus);
			if(settingViewData->nmenuStatus == MAIN_MENU_STATUS)
			{
			    SettingMenuKeyDownClick(msg,settingViewData,settingViewData->nItemCount);
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			else
			{
			    SubMenuKeyDownClick(msg,settingViewData);
			}
        	printf("----------------------------> VK_AP_DOWN 2\r\n");
			break;
		case VK_AP_FONT_BACK_SWITCH:
        case VK_AP_MENU://MENU�˵�
        case REMOTE_KEY_POWER:
        case REMOTE_KEY_MENU:
			XM_printf(">>>>>>>>>>VK_AP_MENU,settingViewData->nmenuStatus:%d\r\n", settingViewData->nmenuStatus);
        	if(settingViewData->nmenuStatus == SUB_MENU_STATUS)
        	{
        		settingViewData->nmenuStatus = MAIN_MENU_STATUS;
				//���²���
				switch(settingViewData->nCurItem)
				{
					case SETTING_MENU_TIME://����ʱ��
						{
							XM_printf(">>>>>>>update sys time.......\r\n");
							XMSYSTEMTIME *settingdate = getDateViewData();
   							XM_SetLocalTime(settingdate);
						}	
						break;
						
					case SETTING_MENU_MOT://����ѡ��

						break;
						
					case SETTING_MENU_DEC:								// LCD
						break;
						
					case SETTING_MENU_COLOR:
						{
						    XM_RxchipDisplayInit();
						}
						break;
						
					case SETTING_MENU_REC:

						break;
						
					case SETTING_MENU_SYS:
						break;
						
					default:
						break;
				}
				
        		XM_InvalidateWindow();
        		XM_UpdateWindow();
        	}
        	else
        	{
            	AP_SaveMenuData(&AppMenuData);
				//���ص�����
				XM_PullWindow (0);
			}
			break;
			
        case VK_AP_SWITCH://ȷ�ϼ�
        case REMOTE_KEY_DOWN:
			XM_printf(">>>>>>>>>>VK_AP_SWITCH,settingViewData->nmenuStatus:%d\r\n", settingViewData->nmenuStatus);
            if(settingViewData->nmenuStatus == MAIN_MENU_STATUS)
            {
            	SettingMenuKeyOkClick(msg,settingViewData);
            }
            else
            {
            	SubMenuKeyOkClick(msg,settingViewData);
            }
            break;
			
       //case VK_AP_SWITCH:
        case REMOTE_KEY_UP:
             
            #if 0
            else if(nCurItem == SETTING_MENU_V1V2){
                if(AppMenuData.AUTO_Switch_OnOff)
                    AppMenuData.AUTO_Switch_OnOff = 0;
                else
                    AppMenuData.AUTO_Switch_OnOff = 1;
                AP_SaveMenuData (&AppMenuData);
                XM_InvalidateWindow ();
			    XM_UpdateWindow ();
            }
            #endif
            break;
        #if 0
		case VK_AP_MODE:
			// ��"����"�����У�MENU��SWITCH��MODE��������Ϊ��ť������
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			XM_SetTimer (XMTIMER_SETTINGVIEW, OSD_AUTO_HIDE_TIMEOUT);
			break;
        #endif
	}
}

/********************************************************************************
1.������ظ���ת��Ϊ�̰���Ϣ,ֻ���������°�����Ϣ�Ĳ�ת,����������Ӧ.
2.������ϢҪ��Ϊ�ظ�������,�� {VK_AP_DOWN,				XMKEY_REPEAT},
3.Ҫ���ٳ�������Ӧ�ٶ�,�����޸��ظ�����delay_timeʱ��,�����ܱȶ̰�С
*******************************************************************************/
VOID SettingViewOnKeyRepeatDown(XMMSG *msg)
{
    if(AP_GetMenuItem(APPMENUITEM_POWER_STATE) == POWER_STATE_OFF)
    {
        return;
    }
	switch(msg->wp)
	{
          	
		case VK_AP_UP://+��
        case REMOTE_KEY_RIGHT:
        case REMOTE_KEY_LEFT:
	    case VK_AP_DOWN://-��
             SettingViewOnKeyDown(msg);
        break;

        default:
        break;          
    }
#if 0
    SETTINGVIEWDATA *settingViewData;
	XMRECT rc;

	settingViewData = (SETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SettingView));
	if(settingViewData == NULL)
		return;
	
    SETTINGMENUDATA *SettingData = settingViewData->SettingData;
	XM_GetWindowRect (XMHWND_HANDLE(SettingView), &rc);
    Delay_Close_View = 0;
    switch(msg->wp)
    {
        case VK_AP_UP:
        case REMOTE_KEY_RIGHT:
            #if 0
            if((nCurItem == SETTING_MENU_VOL) &&(AppMenuData.VolOnOff)&&(ACC_Select != 4)) {
                if(AppMenuData.Audio_PWM < 20)
                    AppMenuData.Audio_PWM ++;
                //AppMenuData.Audio_PWM = SettingData->nVolOnOff;
                HW_Auido_SetLevel(AppMenuData.Audio_PWM);
                XM_InvalidateWindow ();
			    XM_UpdateWindow ();
            }
            #endif
            break;
        case VK_AP_DOWN:
        case REMOTE_KEY_LEFT:
            #if 0
            if((nCurItem == SETTING_MENU_VOL) &&(AppMenuData.VolOnOff)&&(ACC_Select != 4)) {
                if(AppMenuData.Audio_PWM > 0)
                    AppMenuData.Audio_PWM --;
                //AppMenuData.Audio_PWM = SettingData->nVolOnOff;
                HW_Auido_SetLevel(AppMenuData.Audio_PWM);
                XM_InvalidateWindow ();
			    XM_UpdateWindow ();
            }
            #endif
            break;

    }
#endif
}

VOID SettingViewOnKeyUp (XMMSG *msg)
{
    if(AP_GetMenuItem(APPMENUITEM_POWER_STATE) == POWER_STATE_OFF)
    {
        return;
    }
	SETTINGVIEWDATA *settingViewData;

	settingViewData = (SETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SettingView));
	if(settingViewData == NULL)
		return;
    Delay_Close_View = 0;
	switch(msg->wp)
	{
	    case VK_AP_SWITCH:
		   XM_PostMessage (XM_COMMAND, SETTING_COMMAND_MODIFY, settingViewData->nCurItem);//settingViewData->btnControl.pBtnInfo[0].wCommand
			break;

		case VK_AP_MENU:
		    printf("------------------------> KeyUp VK_AP_MENU\r\n");
		case VK_AP_MODE:
			// ��"����"�����У�MENU��SWITCH��MODE��������Ϊ��ť������
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;
	}
}
extern BOOL AHD_Guide_Start(void);
extern BOOL Auto_Menu;
VOID SettingViewOnTimer (XMMSG *msg)
{
    //10s ��û���κβ����Զ��˳�
    Delay_Close_View ++;
	#if 0
    if(AHD_Guide_Start() || (Delay_Close_View>20)&& !Auto_Menu) {
        Delay_Close_View = 0;
       XM_PullWindow (0);
    }
	#endif
	if(Auto_Menu == TRUE) {
		if(Delay_Close_View>20) {
			Delay_Close_View = 0;
            AP_SaveMenuData(&AppMenuData);
    	    XM_PullWindow (0);
		}
	}else {
		if(AHD_Guide_Start() || (Delay_Close_View>20)) {
			Delay_Close_View = 0;
            AP_SaveMenuData(&AppMenuData);
    	    XM_PullWindow (0);
		}
	}
	if(get_parking_trig_status() != 0 && AP_GetMenuItem(APPMENUITEM_PARKING_LINE))
	{
	    XM_PullWindow(0);//����������ʾ��������
	}
	#if 0
	if(Delay_Close_View % 2 == 0)
	{
		update_time = TRUE;
	}
	#endif
}

VOID SettingViewOnCommand (XMMSG *msg)
{
	SETTINGVIEWDATA *settingViewData;
	settingViewData = (SETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(SettingView));
	if(settingViewData == NULL)
		return;
	switch(msg->wp)
	{
		case SETTING_COMMAND_MODIFY:
            switch(msg->lp)
            {
                case SETTING_MENU_COLOR:
            		XM_InvalidateWindow();
                    XM_UpdateWindow();
                    break;
		    case SETTING_MENU_TIME:
				break;

            }
			break;

		case SETTING_COMMAND_RECORD:
			// �л�����¼�����á�����
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
			//XM_JumpWindowEx (XMHWND_HANDLE(VideoSettingView), 0, XM_JUMP_POPDEFAULT);
			break;

		case SETTING_COMMAND_RETURN:
			// ���ص�����
			XM_PullWindow (0);
			break;
	}
}



// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (SettingView)
	XM_ON_MESSAGE (XM_PAINT, SettingViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, SettingViewOnKeyDown)
	XM_ON_MESSAGE (XMKEY_REPEAT, SettingViewOnKeyRepeatDown)
	XM_ON_MESSAGE (XM_KEYUP, SettingViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, SettingViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, SettingViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, SettingViewOnTimer)
	XM_ON_MESSAGE (XM_COMMAND, SettingViewOnCommand)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, SettingView, 0, 0, 0, 255, HWND_VIEW)


