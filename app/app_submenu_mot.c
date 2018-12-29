//****************************************************************************
//
//	Copyright (C) 2018 HX
//
//	Author	Maxd
//
//	File name: app_submenu_mot.c
//	  ÉèÖÃ´°¿Ú
//
//	Revision history
//
//		2018.12.27	Maxd Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuid.h"
#include "app_submenu_mot.h"
#include "xm_app_menudata.h"

void updateMot(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData)
{
    XMRECT item_rect;
    getListViewItemRect(menu_rc,&item_rect,settingViewData->lastSecSubmenuIndex);

    item_rect.left = item_rect.right - 150;
    item_rect.top = item_rect.top ;

    if(AP_GetMenuItem(APPMENUITEM_PARKMONITOR))
    {
        AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_SWITCH_ON, hWnd, &item_rect, XMGIF_DRAW_POS_CENTER);
    }
    else
    {
        AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_SWITCH_OFF, hWnd, &item_rect, XMGIF_DRAW_POS_CENTER);
    }
}

void initMotView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData)
{
    settingViewData->secSubmenuItemCount = 1;
    settingViewData->curSecSubmenuIndex = 0;
    settingViewData->lastSecSubmenuIndex = 0;

    XMRECT rect;
    
    rect.left = menu_rc->left;
    rect.top = menu_rc->top + LISTVIEW_TOP_OFFSET;
    rect.right = menu_rc->right;
    rect.bottom = rect.top + LISTVIEW_ITEM_HEIGHT;
    AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_ITEM_A, hWnd, &rect, XMGIF_DRAW_POS_CENTER);

    rect.left = menu_rc->left + 65;
    rect.right = rect.left + 40;
    AP_RomImageDrawByMenuID(AP_ID_MOT_ICON_MOT, hWnd, &rect, XMGIF_DRAW_POS_CENTER);

    rect.left = rect.right ;
    rect.right = rect.left + 150;
    AP_RomImageDrawByMenuID(AP_ID_MOT_TXT_MOT, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
    
    rect.right = menu_rc->right;
    rect.left = rect.right - 150;
    rect.top = rect.top ;
    
    if(AP_GetMenuItem(APPMENUITEM_PARKMONITOR))
    {
        AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_SWITCH_ON, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
    }
    else
    {
        AP_RomImageDrawByMenuID(AP_ID_LISTVIEW_SWITCH_OFF, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
    }
    rect.left = menu_rc->left;
    rect.right = menu_rc->right;
    rect.bottom = menu_rc->bottom;
    rect.top = rect.bottom - 60;
    //AP_RomImageDrawByMenuID(AP_ID_TXT_MOT, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
}

//key event handler
void motViewKeyOkClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
    if(AP_GetMenuItem(APPMENUITEM_PARKMONITOR) == MOT_ON )
    {
        AP_SetMenuItem(APPMENUITEM_PARKMONITOR,MOT_OFF );
    }
    else
    {
        AP_SetMenuItem(APPMENUITEM_PARKMONITOR,MOT_ON );
    }
}


