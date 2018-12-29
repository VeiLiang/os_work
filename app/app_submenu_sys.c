//****************************************************************************
//
//	Copyright (C) 2018 HX
//
//	Author	Maxd
//
//	File name: app_submenu_sys.c
//	  设置窗口
//
//	Revision history
//
//		2018.12.27	Maxd Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuid.h"
#include "xm_app_menudata.h"
#include "app_submenu_sys.h"

#define SYS_VIEW_ITEM_WIDTH     308
#define SYS_VIEW_ITEM_HIGHT     60

static int lcd_menu_txt_id[] =
{
    AP_ID_VIDEO_CLOSE,
    AP_ID_VIDEO_CYC_1MIN,
    AP_ID_VIDEO_CYC_3MIN,
    AP_ID_VIDEO_CYC_5MIN,
};

static int lang_menu_txt_id[] = 
{
    AP_ID_SYS_MENU_TXT_LANG_CHS,
    AP_ID_SYS_MENU_TXT_LANG_CHT,
    AP_ID_SYS_MENU_TXT_LANG_EN,
};

static int lcd_flip_txt_id[] =
{
    AP_ID_SYS_MENU_TXT_FLIP_NOR,
    AP_ID_SYS_MENU_TXT_FLIP_180,//显示镜像
    AP_ID_SYS_MENU_TXT_FLIP_H,
    AP_ID_SYS_MENU_TXT_FLIP_V,
};

static void getSysMenuItemRect(const XMRECT *rect,XMRECT *item_rect,int item_index, SETTINGVIEWDATA *settingViewData)
{
    int temp_index = item_index % 4;
    int x_offset = ((rect->right - rect->left)/2 - SYS_VIEW_ITEM_WIDTH)/2;
    
    printf("-------------------------------> getSysMenuItemRect\n");
    getListViewItemRect(rect,item_rect,temp_index);
    if(item_index >= settingViewData->secSubmenuItemCount / 2)
    {
        item_rect->right = item_rect->right - x_offset;
        item_rect->left = item_rect->right - SYS_VIEW_ITEM_WIDTH;
    }
    else
    {
        item_rect->left = item_rect->left + x_offset;
        item_rect->right = item_rect->left + SYS_VIEW_ITEM_WIDTH;
    }
}

static void drawSysMenuItem(HANDLE hWnd, const XMRECT *item_rect,unsigned item_index,const SETTINGVIEWDATA *settingViewData)
{
    XMRECT temp_rect;
    int sys_item_val;

    temp_rect.left = item_rect->left;
    temp_rect.right = item_rect->right;
    temp_rect.top = item_rect->top;
    temp_rect.bottom = item_rect->bottom;
    if(settingViewData->curSecSubmenuIndex == item_index)
    {
        AP_RomImageDrawByMenuID(AP_ID_SYS_MENU_ITEM_A, hWnd, &temp_rect, XMGIF_DRAW_POS_CENTER);
    }
    else
    {
        AP_RomImageDrawByMenuID(AP_ID_SYS_MENU_ITEM_N, hWnd, &temp_rect, XMGIF_DRAW_POS_CENTER);
    }

    switch(item_index)
    {
        case SCREEN_TIME_ITEM:
            sys_item_val = 
            temp_rect.left = item_rect->left + 20;
            temp_rect.top = item_rect->top + 10;
            AP_RomImageDrawByMenuID(AP_ID_SYS_MENU_TXT_LCD_PRO,hWnd, &temp_rect,XMGIF_DRAW_POS_LEFTTOP);
            
            temp_rect.top = item_rect->top - 10;
            temp_rect.right = item_rect->right - 10;
            temp_rect.left = item_rect->right - 100;
            sys_item_val = AP_GetMenuItem(APPMENUITEM_LCD);
            AP_RomImageDrawByMenuID(lcd_menu_txt_id[sys_item_val],hWnd, &temp_rect,XMGIF_DRAW_POS_RIGHT);
            break;
			
        case SYS_LANG_ITEM:
            temp_rect.left = item_rect->left + 20;
            temp_rect.top = item_rect->top + 10;
            AP_RomImageDrawByMenuID(AP_ID_SYS_MENU_TXT_LANG,hWnd, &temp_rect,XMGIF_DRAW_POS_LEFTTOP);
            temp_rect.top = item_rect->top - 10;
            temp_rect.right = item_rect->right - 10;
            temp_rect.left = item_rect->right - 100;
            sys_item_val = AP_GetMenuItem(APPMENUITEM_LANG);
            
            AP_RomImageDrawByMenuID(lang_menu_txt_id[sys_item_val],hWnd, &temp_rect,XMGIF_DRAW_POS_RIGHT);
            break;
			
        case CH1_MIRROR_ITEM:
            temp_rect.left = item_rect->left + 20;
            temp_rect.top = item_rect->top + 10;
            AP_RomImageDrawByMenuID(AP_ID_SYS_MENU_TXT_MIRROR_CH1,hWnd, &temp_rect,XMGIF_DRAW_POS_LEFTTOP);
            temp_rect.top = item_rect->top - 10;
            temp_rect.right = item_rect->right - 10;
            temp_rect.left = item_rect->right - 100;
            sys_item_val = AP_GetMenuItem(APPMENUITEM_MIRROR);
            printf("-----------------------> APPMENUITEM_MIRROR 1 : %d \n",sys_item_val);
            if( (sys_item_val & CH1_MIRROR) != 0)
            {
                AP_RomImageDrawByMenuID(AP_ID_SYS_MENU_SWITCH_ON,hWnd, &temp_rect,XMGIF_DRAW_POS_RIGHT);
            }
            else
            {
                AP_RomImageDrawByMenuID(AP_ID_SYS_MENU_SWITCH_OFF,hWnd, &temp_rect,XMGIF_DRAW_POS_RIGHT);
            }
            break;
			
        case SD_FORMAT_ITEM:
            temp_rect.left = item_rect->left + 20;
            temp_rect.top = item_rect->top + 10;
            AP_RomImageDrawByMenuID(AP_ID_SYS_MENU_TXT_SD_FORMAT,hWnd, &temp_rect,XMGIF_DRAW_POS_LEFTTOP);
            break;
			
        case LCD_RORATE_ITEM:
            temp_rect.left = item_rect->left + 20;
            temp_rect.top = item_rect->top + 10;
            AP_RomImageDrawByMenuID(AP_ID_SYS_MENU_TXT_FLIP,hWnd, &temp_rect,XMGIF_DRAW_POS_LEFTTOP);
            temp_rect.top = item_rect->top - 10;
            temp_rect.right = item_rect->right - 10;
            temp_rect.left = item_rect->right - 100;
            sys_item_val = AP_GetLCD_Rotate();
            printf("-----------------------> APPMENUITEM_LCD_FLIP : %d \n",sys_item_val);
            AP_RomImageDrawByMenuID(lcd_flip_txt_id[sys_item_val],hWnd, &temp_rect,XMGIF_DRAW_POS_RIGHT);
            break;
			
        case DEFAULT_PARAM_ITEM:
            temp_rect.left = item_rect->left + 20;
            temp_rect.top = item_rect->top + 10;
            AP_RomImageDrawByMenuID(AP_ID_SYS_MENU_TXT_RESET,hWnd, &temp_rect,XMGIF_DRAW_POS_LEFTTOP);
            break;
			
        case CH2_MIRROR_ITEM:
            temp_rect.left = item_rect->left + 20;
            temp_rect.top = item_rect->top + 10;
            AP_RomImageDrawByMenuID(AP_ID_SYS_MENU_TXT_MIRROR_CH2,hWnd, &temp_rect,XMGIF_DRAW_POS_LEFTTOP);
            temp_rect.top = item_rect->top - 10;
            temp_rect.right = item_rect->right - 10;
            temp_rect.left = item_rect->right - 100;
            sys_item_val = AP_GetMenuItem(APPMENUITEM_MIRROR);
            printf("-----------------------> APPMENUITEM_MIRROR 2 : %d \n",sys_item_val);
            printf("-----------------------> APPMENUITEM_MIRROR 2 res : %d \n",sys_item_val & CH2_MIRROR);
            if( (sys_item_val&CH2_MIRROR) != 0)
            {
                AP_RomImageDrawByMenuID(AP_ID_SYS_MENU_SWITCH_ON,hWnd, &temp_rect,XMGIF_DRAW_POS_RIGHT);
            }
            else
            {
                printf("-----------------------> APPMENUITEM_MIRROR 2 ---: %d \n",sys_item_val);
                AP_RomImageDrawByMenuID(AP_ID_SYS_MENU_SWITCH_OFF,hWnd, &temp_rect,XMGIF_DRAW_POS_RIGHT);
            }
            break;
			
        case SW_VERSION_ITEM:
            temp_rect.left = item_rect->left + 20;
            temp_rect.top = item_rect->top + 10;
            AP_RomImageDrawByMenuID(AP_ID_SYS_MENU_TXT_VERSION,hWnd, &temp_rect,XMGIF_DRAW_POS_LEFTTOP);
            break;
			
        default:
            break;
    }
}

void initSysMenu(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData)
{
    settingViewData->secSubmenuItemCount = 8;
    settingViewData->curSecSubmenuIndex = 0;
    settingViewData->lastSecSubmenuIndex = 0;
    int item_index = 0;
    XMRECT item_rect;
    printf("-------------------------------> initSysMenu\n");
    for(item_index = 0;item_index < settingViewData->secSubmenuItemCount; item_index ++)
    {
        getSysMenuItemRect(menu_rc, &item_rect, item_index, settingViewData);
        drawSysMenuItem(hWnd,&item_rect,item_index,settingViewData);
    }

    XMRECT rect;
    rect.left = menu_rc->left;
    rect.right = menu_rc->right;
    rect.bottom = menu_rc->bottom;
    rect.top = rect.bottom - 60;
}

void updateSysMenu(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData)
{
    XMRECT item_rect;
    getSysMenuItemRect(menu_rc, &item_rect, settingViewData->lastSecSubmenuIndex, settingViewData);
    drawSysMenuItem(hWnd,&item_rect,settingViewData->lastSecSubmenuIndex,settingViewData);

    getSysMenuItemRect(menu_rc, &item_rect, settingViewData->curSecSubmenuIndex, settingViewData);
    drawSysMenuItem(hWnd,&item_rect,settingViewData->curSecSubmenuIndex,settingViewData);
}

void sysMenuKeyOkClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
    u8_t sys_val;
    switch(settingViewData->curSecSubmenuIndex)
    {
        case SCREEN_TIME_ITEM://屏保时捷
            sys_val = AP_GetMenuItem(APPMENUITEM_LCD);
            sys_val += 1;
            if(sys_val >= AP_SETTING_LCD_OPTION_COUNT)
            {
                sys_val = AP_SETTING_LCD_NEVERCLOSE;
            }
            AP_SetMenuItem(APPMENUITEM_LCD,sys_val);
            break;
			
        case SYS_LANG_ITEM://系统语言
            sys_val = AP_GetMenuItem(APPMENUITEM_LANG);
            sys_val += 1;
            if(sys_val >= AP_SETTING_LANG_OPTION_COUNT)
            {
                sys_val = 0;
            }
            AP_SetMenuItem(APPMENUITEM_LANG, sys_val);
            break;
			
        case CH1_MIRROR_ITEM://通道1镜像
            sys_val = AP_GetMenuItem(APPMENUITEM_MIRROR);
            if( (sys_val & CH1_MIRROR) != 0)
            {
                sys_val &= ~CH1_MIRROR;
            }
            else
            {
                sys_val |= CH1_MIRROR;
            }
            AP_SetMenuItem(APPMENUITEM_MIRROR, sys_val);
            break;
			
        case SD_FORMAT_ITEM://SD卡格式化
            {
				AP_OpenFormatView(1);//push
				//show_alert(msg,settingViewData);
            }
            break;
			
        case LCD_RORATE_ITEM://正常/镜像
            sys_val = AP_GetLCD_Rotate();// AP_GetMenuItem(APPMENUITEM_LCD_FLIP);
            if(sys_val==AP_SETTING_LCD_FLIP_NORMAL)
            {
				sys_val = AP_SETTING_LCD_FLIP_MIRROR;
			}
			else if(sys_val==AP_SETTING_LCD_FLIP_MIRROR)
			{
				sys_val = AP_SETTING_LCD_FLIP_NORMAL;
			}
			else    //初始值不再两个值中，则无法进行设置
			{
				sys_val = AP_SETTING_LCD_FLIP_NORMAL;
			}
            AP_SetLCD_Rotate(sys_val);
            break;
			
        case DEFAULT_PARAM_ITEM://恢复默认值
            AP_OpenDefaultParamView(1);
            break;
			
        case CH2_MIRROR_ITEM://CH2通道镜像
            sys_val = AP_GetMenuItem(APPMENUITEM_MIRROR);
            printf("----------------------------> CH2_MIRROR 0 sys_val : %d\n",sys_val);
            if( (sys_val & CH2_MIRROR) != 0)
            {
                printf("----------------------------> CH2_MIRROR 1 sys_val : %d\n",sys_val);
                sys_val &= (~CH2_MIRROR);
            }
            else
            {
                printf("----------------------------> CH2_MIRROR 2 sys_val : %d\n",sys_val);
                sys_val |= CH2_MIRROR;
            }
            AP_SetMenuItem(APPMENUITEM_MIRROR, sys_val);
            break;
			
        case SW_VERSION_ITEM://软件版本
            XM_PushWindow(XMHWND_HANDLE(VersionView));
            break;
        default:
            break;
    }
}

void sysMenuKeyUpClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
    submenuNextCyc(msg,settingViewData);
}

void sysMenuKeyDownClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
    submenuPrivCyc(msg,settingViewData);
}


