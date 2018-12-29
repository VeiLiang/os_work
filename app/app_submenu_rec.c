//****************************************************************************
//
//	Copyright (C) 2018 HX
//
//	Author	Maxd
//
//	File name: app_submenu_rec.c
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
#include "app_submenu_rec.h"
static const int video_cyc_val_array_icon[] = 
{
    AP_ID_VIDEO_CYC_1MIN,
    AP_ID_VIDEO_CYC_3MIN,
    AP_ID_VIDEO_CYC_5MIN,
    AP_ID_VIDEO_CYC_10MIN,
};

static void initVideoViewItem(HANDLE hWnd, XMRECT *item_rect,unsigned item_index,SETTINGVIEWDATA *settingViewData)
{
    XMRECT item_rect_tmp;
    item_rect_tmp.top = item_rect->top;
    item_rect_tmp.bottom = item_rect->bottom;
    unsigned int item_val;
    switch(item_index)
    {
        case 0:
            if(settingViewData->curSecSubmenuIndex == item_index)
            {
                AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_ITEM_A, hWnd, item_rect, XMGIF_DRAW_POS_CENTER);
            }
            else
            {
                AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_ITEM_N, hWnd, item_rect, XMGIF_DRAW_POS_CENTER);
            }
            item_rect_tmp.left = item_rect->left + 65;
            item_rect_tmp.right = item_rect_tmp.left + 40;
            AP_RomImageDrawByMenuID(AP_ID_VIDEO_ICON_CYC, hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            item_rect_tmp.left = item_rect_tmp.right ;
            item_rect_tmp.right = item_rect_tmp.left + 150;
            AP_RomImageDrawByMenuID(AP_ID_VIDEO_TXT_CYC, hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            item_rect_tmp.right = item_rect->right;
            item_rect_tmp.left = item_rect_tmp.right - 150;
            AP_RomImageDrawByMenuID(AP_ID_VIDEO_CYC_VAL_BG, hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            item_val = AP_GetMenuItem(APPMENUITEM_VIDEO_TIME_SIZE);
            AP_RomImageDrawByMenuID(video_cyc_val_array_icon[item_val], hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            break;
        case 1:
            if(settingViewData->curSecSubmenuIndex == item_index)
            {
                AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_ITEM_A, hWnd, item_rect, XMGIF_DRAW_POS_CENTER);
            }
            else
            {
                AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_ITEM_N, hWnd, item_rect, XMGIF_DRAW_POS_CENTER);
            }
            item_rect_tmp.left = item_rect->left + 65;
            item_rect_tmp.right = item_rect_tmp.left + 40;
            AP_RomImageDrawByMenuID(AP_ID_VIDEO_ICON_WATER, hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            item_rect_tmp.left = item_rect_tmp.right ;
            item_rect_tmp.right = item_rect_tmp.left + 150;
            AP_RomImageDrawByMenuID(AP_ID_VIDEO_TXT_WATER, hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            item_rect_tmp.right = item_rect->right;
            item_rect_tmp.left = item_rect_tmp.right - 150;
            item_val = AP_GetMenuItem(APPMENUITEM_TIME_STAMP);
            if(item_val)
            {
                AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_SWITCH_ON, hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            }
            else
            {
                AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_SWITCH_OFF, hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            }
            break;
            
        case 2:
            if(settingViewData->curSecSubmenuIndex == item_index)
            {
                AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_ITEM_A, hWnd, item_rect, XMGIF_DRAW_POS_CENTER);
            }
            else
            {
                AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_ITEM_N, hWnd, item_rect, XMGIF_DRAW_POS_CENTER);
            }
            item_rect_tmp.left = item_rect->left + 65;
            item_rect_tmp.right = item_rect_tmp.left + 40;
            AP_RomImageDrawByMenuID(AP_ID_VIDEO_ICON_PARKING_LINE, hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            item_rect_tmp.left = item_rect_tmp.right ;
            item_rect_tmp.right = item_rect_tmp.left + 150;
            AP_RomImageDrawByMenuID(AP_ID_VIDEO_TXT_PARKING_LINE, hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            item_rect_tmp.right = item_rect->right;
            item_rect_tmp.left = item_rect_tmp.right - 150;
            item_val = AP_GetMenuItem(APPMENUITEM_PARKING_LINE);
            if(item_val)
            {
                AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_SWITCH_ON, hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            }
            else
            {
                AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_SWITCH_OFF, hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            }
            break;
        case 3:
            if(settingViewData->curSecSubmenuIndex == item_index)
            {
                AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_ITEM_A, hWnd, item_rect, XMGIF_DRAW_POS_CENTER);
            }
            else
            {
                AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_ITEM_N, hWnd, item_rect, XMGIF_DRAW_POS_CENTER);
            }
            item_rect_tmp.left = item_rect->left + 65;
            item_rect_tmp.right = item_rect_tmp.left + 40;
            AP_RomImageDrawByMenuID(AP_ID_VIDEO_ICON_PARKING_DELAY, hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            item_rect_tmp.left = item_rect_tmp.right ;
            item_rect_tmp.right = item_rect_tmp.left + 150;
            AP_RomImageDrawByMenuID(AP_ID_VIDEO_TXT_PARKING_DELAY, hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            item_rect_tmp.right = item_rect->right;
            item_rect_tmp.left = item_rect_tmp.right - 150;
            item_val = AP_GetMenuItem(APPMENUITEM_PARKING_DELAY);
            if(item_val)
            {
                AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_SWITCH_ON, hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            }
            else
            {
                AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_SWITCH_OFF, hWnd, &item_rect_tmp, XMGIF_DRAW_POS_CENTER);
            }

            break;
        default:
            break;
    }
}

void updateVideoView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData)
{
    XMRECT item_rect;
    getListViewItemRect(menu_rc, &item_rect, settingViewData->lastSecSubmenuIndex);
    initVideoViewItem(hWnd,&item_rect,settingViewData->lastSecSubmenuIndex,settingViewData);

    getListViewItemRect(menu_rc, &item_rect, settingViewData->curSecSubmenuIndex);
    initVideoViewItem(hWnd,&item_rect,settingViewData->curSecSubmenuIndex,settingViewData);
}

void initVideoView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData)
{
    settingViewData->secSubmenuItemCount = 4;
    settingViewData->curSecSubmenuIndex = 0;
    settingViewData->lastSecSubmenuIndex = 0;
    XMRECT item_rect;
    
    int item_index = 0;
    for( item_index = 0; item_index < settingViewData->secSubmenuItemCount; item_index ++)
    {
        getListViewItemRect(menu_rc, &item_rect, item_index);
        initVideoViewItem(hWnd,&item_rect,item_index,settingViewData);
    }
    XMRECT rect;
    rect.left = menu_rc->left;
    rect.right = menu_rc->right;
    rect.bottom = menu_rc->bottom;
    rect.top = rect.bottom - 60;
}

void videoViewKeyUpClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
    int item_val;
    submenuNextCyc(msg,settingViewData);
}

void videoViewKeyDownClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
	int item_val;
    submenuPrivCyc(msg,settingViewData);
}

void videoViewKeyOkClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
	int item_val;
    switch(settingViewData->curSecSubmenuIndex)
    {
        case CYC_REC_ITEM://循环录像时间
            item_val = AP_GetMenuItem(APPMENUITEM_VIDEO_TIME_SIZE);
            if(item_val == 3)
            {
                item_val = 0;
            }
            else
            {
                item_val ++;
            }
            AP_SetMenuItem(APPMENUITEM_VIDEO_TIME_SIZE,item_val);
            break;
			
        case TIME_STAMP_ITEM://时间水印
            item_val = AP_GetMenuItem(APPMENUITEM_TIME_STAMP);
            item_val = item_val ? 0 : 1;
            AP_SetMenuItem(APPMENUITEM_TIME_STAMP,item_val);
            break;
			
        case CARBACK_LINE_ITEM://倒车线开关
            item_val = AP_GetMenuItem(APPMENUITEM_PARKING_LINE);
            item_val = item_val ? 0 : 1;
            AP_SetMenuItem(APPMENUITEM_PARKING_LINE,item_val);
            break;
			
        case CARBACK_DELAY_TIME_ITEM://倒车返回延迟
            item_val = AP_GetMenuItem(APPMENUITEM_PARKING_DELAY);
            item_val = item_val ? 0 : 1;
            AP_SetMenuItem(APPMENUITEM_PARKING_DELAY,item_val);
            break;
			
        default:
            break;
    }
    
}

