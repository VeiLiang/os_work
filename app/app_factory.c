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
#include "xm_app_menudata.h"

#define ITEM_HEIGHT                 40
#define ITEM_MAX_AVILIBALE_COUNT    6
#define ITEM_WIDTH                  450
#define TITLE_WIDTH                 800
#define ITEM_VALUE_WIDTH            150

#define ITEM_ACTIVE_COLOR           0xFF00A2E8
#define ITEM_NORMAL_COLOR           0xFF808080


typedef struct _tagFACTORYMENUDATA {
    u8_t            nCarback;
	u8_t            nPowerOnMode;
	u8_t            nAHDChannelNum;
    u8_t            VolOnOff;
    u8_t            LogoEnable;
    u8_t            AutoDimOnOff;
    u8_t            MiddleRedLine;
    u8_t            nVCOM;
}FACTORYMENUDATA;

typedef struct tagFACTORYVIEWDATA {
	int				nTopItem;					// 第一个可视的菜单项
	int				nCurItem;					// 当前选择的菜单项
	int             nLastItem;                  //上一个选中的菜单
	int				nItemCount;					// 菜单项个数
    FACTORYMENUDATA  *FactoryData;
	APPMENUOPTION	*lpFactoryView;
	XMBUTTONCONTROL	btnControl;				// 按钮控件
	XMTITLEBARCONTROL	titleControl;			// 标题控件

} FACTORYVIEWDATA;


// 设置的设置菜单项及顺序定义, 0表示第一个项目(最顶部)
enum  {
    FACTORY_VCOM = 0,
    FACTORY_POWERONMODE,
    FACTORY_KEYBOARD,
    FACTORY_USER0,
    FACTORY_USER1,
    FACTORY_EXIT,
    FACTORY_COUNT
} FACTORY_DEFINE;

static int factory_menu_first_show = TRUE;

VOID FactoryViewOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
	    u32_t VP_CONTRAST = 0;
	    FACTORYVIEWDATA *FactoryViewData = XM_calloc(sizeof(FACTORYVIEWDATA));
        FACTORYMENUDATA *factoryData = FactoryViewData->FactoryData;
		if(FactoryViewData == NULL)
		{
			XM_printf ("menuOptionViewData XM_calloc failed\n");
			// 失败返回到桌面
			XM_PullWindow (0);
			return;
		}

        FactoryViewData->nCurItem = 0;
        FactoryViewData->nItemCount = FACTORY_COUNT;

        factoryData->nCarback = AppMenuData.carbackdelay;

        factoryData->nPowerOnMode = AppMenuData.poweron_brightness;
        factoryData->VolOnOff = AppMenuData.VolOnOff;
        factoryData->nAHDChannelNum = AppMenuData.AHD_ChannelNum;
        factoryData->LogoEnable = AppMenuData.LogoEnable;
        factoryData->AutoDimOnOff = AppMenuData.AutoDimOnOff;
        factoryData->MiddleRedLine = AppMenuData.Middle_RED_LIne;
        factoryData->nVCOM = AP_GetVCOM_PWM();

		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData(XMHWND_HANDLE(FactoryView), FactoryViewData);
	    XM_SetWindowAlpha(XMHWND_HANDLE(FactoryView), 255);
        printf("------------------------------> FactoryViewOnEnter \n");
	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("FormatSettingView Pull\n");
	}
    XM_SetTimer(XMTIMER_SETTINGVIEW, 200); //开启定时器
}

VOID FactoryViewOnLeave (XMMSG *msg)
{
    XM_KillTimer (XMTIMER_SETTINGVIEW);
    factory_menu_first_show = TRUE;
	if (msg->wp == 0)
	{
		FACTORYVIEWDATA *FactoryViewData = (FACTORYVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(FactoryView));
		if(FactoryViewData)
		{
			// 标题控件退出过程
			AP_TitleBarControlExit (&FactoryViewData->titleControl);
			// 释放私有数据句柄
			XM_free (FactoryViewData);
		}
		XM_printf ("FormatSettingView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("FormatSettingView Push\n");
	}
}

//offset_index start from 0
static void getCurRect(XMRECT* rect, XMRECT* menu_rc,int offset_index)
{
    int menu_height = ITEM_HEIGHT * ITEM_MAX_AVILIBALE_COUNT;
    rect->bottom = menu_rc->bottom - (ITEM_MAX_AVILIBALE_COUNT - offset_index ) * ITEM_HEIGHT;
    rect->top = rect->bottom - ITEM_HEIGHT;
    rect->left = menu_rc->left;
    rect->right = menu_rc->right;
}

static void drawFactoryMenuItem(HANDLE hWnd,FACTORYVIEWDATA *factoryViewData, XMRECT* menu_rc,int cur_index)
{
    XMRECT rect;
    XMSIZE size;
    char String[32];
    DWORD menuID;

    int offset_index = cur_index - factoryViewData->nTopItem;
    FACTORYMENUDATA *factorynData =  factoryViewData->FactoryData;
    
    getCurRect(&rect,menu_rc,offset_index);
    
    if(factoryViewData->nCurItem == cur_index)
    {
        XM_FillRect(hWnd,rect.left,rect.top,rect.right,rect.bottom,ITEM_ACTIVE_COLOR);
    }
    else
    {
        XM_FillRect(hWnd,rect.left,rect.top,rect.right,rect.bottom,ITEM_NORMAL_COLOR);
    }
    switch(cur_index)
    {
        case FACTORY_VCOM:
            AP_RomImageDrawByMenuID (AP_ID_SETTING_VCOM,hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
            sprintf(String,"%d",factorynData->nVCOM);
            AP_TextGetStringSize(String,sizeof(String),&size);
            rect.left = rect.right - ITEM_VALUE_WIDTH ;
            AP_TextOutDataTimeString (hWnd, rect.left, rect.top, String, strlen(String));
            break;
			
        case FACTORY_POWERONMODE:
            AP_RomImageDrawByMenuID (AP_ID_SETTING_POWERON, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
            if(factorynData->nPowerOnMode == 0) {
                menuID = AP_ID_SETTING_OFF;
            }else if(factorynData->nPowerOnMode == 1){
                menuID = AP_ID_SETTING_MOMERY;
            }else {
                menuID = AP_ID_SETTING_ON;
            }
            rect.left = rect.right - ITEM_VALUE_WIDTH ;
            AP_RomImageDrawByMenuID (menuID, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
            break;
			
        case FACTORY_KEYBOARD:
            AP_RomImageDrawByMenuID (AP_ID_SETTING_KEYBOARD, hWnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
            sprintf(String,"%s","6KEY");
            AP_TextGetStringSize(String,sizeof(String),&size);
            rect.left = rect.right - ITEM_VALUE_WIDTH ;
            AP_TextOutDataTimeString(hWnd, rect.left, rect.top, String, strlen(String));
            break;
			
        case FACTORY_USER0:
            break;
			
        case FACTORY_USER1:
            break;
			
        case FACTORY_EXIT:
            AP_RomImageDrawByMenuID (AP_ID_SETTING_EXIT,hWnd, &rect, XMGIF_DRAW_POS_CENTER);
            break;
			
        default:
            break;
    }
}

static void factoryMenuInit(HANDLE hWnd,XMRECT* menu_rc,FACTORYVIEWDATA *factoryMenuData)
{
    int i = 0;
    factoryMenuData->nLastItem = 0;
    factoryMenuData->nCurItem = 0;
    for(i = 0;i < ITEM_MAX_AVILIBALE_COUNT;i++)
    {
        drawFactoryMenuItem(hWnd,factoryMenuData,menu_rc,i);
    }
}

static void updateFactoryMenuItem(HANDLE hWnd,FACTORYVIEWDATA *factoryViewData,XMRECT* menu_rc)
{
    drawFactoryMenuItem(hWnd,factoryViewData,menu_rc,factoryViewData->nLastItem);
    drawFactoryMenuItem(hWnd,factoryViewData,menu_rc,factoryViewData->nCurItem);
}

VOID FactoryViewOnPaint (XMMSG *msg)
{
	XMRECT rc,rect,rcArrow,menu_rc;
	unsigned int old_alpha;
    char String[32];
	
	HANDLE hwnd = XMHWND_HANDLE(FactoryView);
	FACTORYVIEWDATA *FactoryViewData = (FACTORYVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(FactoryViewData == NULL)
	{
		return;
    }
    printf("------------------------------> FactoryViewOnPaint \n");
	// 显示标题栏

    XM_GetDesktopRect(&rc);
    menu_rc.bottom = rc.bottom;
    menu_rc.top = menu_rc.bottom - ITEM_HEIGHT * ITEM_MAX_AVILIBALE_COUNT;
    menu_rc.left = ((rc.right - rc.left) - ITEM_WIDTH)/2;
    menu_rc.right = menu_rc.left + ITEM_WIDTH;
    
    if(factory_menu_first_show)
    {
        factory_menu_first_show = FALSE;
    	//XM_GetDesktopRect(&rc);
    	XM_FillRect(hwnd, rc.left, rc.top, rc.right, rc.bottom, 0x00000000);
        factoryMenuInit(hwnd,&menu_rc,FactoryViewData);
    }
    else
    {
        updateFactoryMenuItem(hwnd,FactoryViewData,&menu_rc);
    }
}

void AdjustFactoryData(int nItem, int diff)
{
	FACTORYVIEWDATA *FactoryViewData = (FACTORYVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(FactoryView));
	if(FactoryViewData == NULL)
	{
		return;
	}
	FACTORYMENUDATA *factorynData =  FactoryViewData->FactoryData;

    switch(nItem) 
	{
        case FACTORY_POWERONMODE:
            if(diff == 1) {
                if(factorynData->nPowerOnMode >= 2) {
                    factorynData->nPowerOnMode = 0;
                }else {
                    factorynData->nPowerOnMode ++;;
                }
            }else {
                if(factorynData->nPowerOnMode == 0) {
                      factorynData->nPowerOnMode = 2;
                }else {
                    factorynData->nPowerOnMode --;
                }
            }
            break;
			
        case FACTORY_KEYBOARD:
            break;
			
        case FACTORY_VCOM:
            if(diff == 1) 
			{
                if(factorynData->nVCOM < 100)
                    factorynData->nVCOM ++;
                else
                   factorynData->nVCOM =0; //继续增大,就从0开始      
            }
			else 
			{
                if(factorynData->nVCOM > 0)
                    factorynData->nVCOM --;
                else
                    factorynData->nVCOM =100;
            }
            AppMenuData.VCOM_PWM = factorynData->nVCOM;
			XM_printf(">>>>vcom dc:%d\r\n", AppMenuData.VCOM_PWM);
			set_pwm_duty(AP_GetVCOM_PWM());
			break;
			
        case FACTORY_EXIT:
            AppMenuData.Middle_RED_LIne = factorynData->MiddleRedLine;
            AppMenuData.AutoDimOnOff = factorynData->AutoDimOnOff;
            AppMenuData.LogoEnable = factorynData->LogoEnable;
            AppMenuData.VolOnOff = factorynData->VolOnOff;
            AppMenuData.AHD_ChannelNum = factorynData->nAHDChannelNum;
            AppMenuData.poweron_brightness = factorynData->nPowerOnMode;
            AppMenuData.carbackdelay = factorynData->nCarback;
            AppMenuData.VCOM_PWM = factorynData->nVCOM;
            AP_SaveMenuData (&AppMenuData);
            XM_PullWindow (0);
            break;

		default:
			break;
    }
}


VOID FactoryViewOnKeyDown (XMMSG *msg)
{
    int nVisualCount;
	XMRECT rc;
	
	FACTORYVIEWDATA *FactoryViewData = (FACTORYVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(FactoryView));
	if(FactoryViewData == NULL)
		return;
    FACTORYMENUDATA *factorynData =  FactoryViewData->FactoryData;
    nVisualCount = 6;
    switch(msg->wp)
    {
        case VK_AP_MENU:
        case REMOTE_KEY_MENU:
        case REMOTE_KEY_POWER:
            printf("------------------------> REMOTE_KEY_MENU \n");
            AppMenuData.Middle_RED_LIne = factorynData->MiddleRedLine;
            AppMenuData.AutoDimOnOff = factorynData->AutoDimOnOff;
            AppMenuData.LogoEnable = factorynData->LogoEnable;
            AppMenuData.VolOnOff = factorynData->VolOnOff;
            AppMenuData.AHD_ChannelNum = factorynData->nAHDChannelNum;
            AppMenuData.poweron_brightness = factorynData->nPowerOnMode;
            AppMenuData.carbackdelay = factorynData->nCarback;
            AppMenuData.VCOM_PWM = factorynData->nVCOM;
            AP_SaveMenuData(&AppMenuData);
            XM_PullWindow (0);
            break;
			
        case VK_AP_UP:
        case REMOTE_KEY_RIGHT:
            printf("------------------------> REMOTE_KEY_RIGHT \n");
			// 刷新
            AdjustFactoryData(FactoryViewData->nCurItem,1);
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
            break;
			
        case VK_AP_DOWN:
        case REMOTE_KEY_LEFT:
            printf("------------------------> REMOTE_KEY_LEFT \n");
			// 刷新
			AdjustFactoryData(FactoryViewData->nCurItem,-1);
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
            break;
			
        case VK_AP_SWITCH:
        case REMOTE_KEY_DOWN:
            printf("------------------------> REMOTE_KEY_DOWN \n");
            FactoryViewData->nLastItem = FactoryViewData->nCurItem;
            FactoryViewData->nCurItem ++;

            if(FactoryViewData->nCurItem >= FactoryViewData->nItemCount) {
                FactoryViewData->nTopItem = 0;
                FactoryViewData->nCurItem = 0;
            }else {
                while ((FactoryViewData->nCurItem -FactoryViewData->nTopItem) >= nVisualCount)
                {
                    FactoryViewData->nTopItem ++;
                }
            }
    		XM_InvalidateWindow ();
    		XM_UpdateWindow ();
            break;
        case VK_AP_FONT_BACK_SWITCH:
        case REMOTE_KEY_UP:
            printf("------------------------> REMOTE_KEY_UP \n");
            //AP_ButtonControlMessageHandler (&FactoryViewData->btnControl, msg);
            FactoryViewData->nLastItem = FactoryViewData->nCurItem;
            FactoryViewData->nCurItem --;
            if(FactoryViewData->nCurItem < 0)
			{
				// 聚焦到最后一个
				FactoryViewData->nCurItem = (WORD)(FactoryViewData->nItemCount - 1);
				FactoryViewData->nTopItem = 0;
				while ( (FactoryViewData->nCurItem - FactoryViewData->nTopItem) >= nVisualCount )
				{
					FactoryViewData->nTopItem ++;
				}
			}
			else
			{
				if(FactoryViewData->nTopItem > FactoryViewData->nCurItem)
					FactoryViewData->nTopItem = FactoryViewData->nCurItem;
			}
    		XM_InvalidateWindow ();
    		XM_UpdateWindow ();
            break;
			
		default:
			break;
    }

}
VOID FactoryViewRepeatDown (XMMSG *msg)
{
    FactoryViewOnKeyDown (msg);//让VCOM调节 一直按着可以调节

}

VOID FactoryViewOnTimer (XMMSG *msg)
{
    if(get_parking_trig_status()) {
        //XM_JumpWindow (XMHWND_HANDLE(Desktop));
        XM_PullWindow(XMHWND_HANDLE(Desktop));
    }
}
// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (FactoryView)
	XM_ON_MESSAGE (XM_PAINT, FactoryViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, FactoryViewOnKeyDown)
	XM_ON_MESSAGE (XMKEY_REPEAT, FactoryViewRepeatDown)	
	XM_ON_MESSAGE (XM_ENTER, FactoryViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, FactoryViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, FactoryViewOnTimer)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, FactoryView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
