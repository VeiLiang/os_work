//****************************************************************************
//
//	Copyright (C) 2018 HX
//
//	Author	Maxd
//
//	File name: app_submenu_datetime.c
//	  设置窗口
//
//	Revision history
//
//		2018.12.27	Maxd Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuid.h"
#include "app_submenu_datetime.h"
#include "rxchip.h"

static void drawTime()
{
	XMCOORD x,y;
	XMSIZE size;
	
	HANDLE hWnd = XMHWND_HANDLE(SettingView);
	XMSYSTEMTIME systemTime;
	
	XM_GetLocalTime(&systemTime);
	XMRECT rc;
	XM_GetDesktopRect(&rc);

	char text[32];
	char text2[32];
	//格式4： 输入时分秒
	AP_FormatDataTime (&systemTime, APP_DATETIME_FORMAT_3, text, sizeof(text));
	AP_TextGetStringSize (text, strlen(text), &size);

	x = (rc.right - 180)/2;
	y = 10;

	XM_FillRect(hWnd, x, y, x + 180, 50, XM_GetSysColor(XM_COLOR_DESKTOP));
	// 格式2： 输出年月日"

	AP_TextOutDataTimeString (hWnd, (rc.right-size.cx)/2, y, text, strlen(text));
}

#define DATE_SPACE_WIDTH        10
#define DATE_VIEW_TOP_OFFSET    100
#define DATE_VIEW_BOTTOM_OFFSET 100
#define DATE_VIEW_Y_OFFSET      120
#define DATE_VIEW_ROWS          2
#define DATE_VIEW_COLUMNS       3

#define DATE_VIEW_ITEM_WIDTH    140
#define DATE_VIEW_ITEM_HEIGHT   100
#define DATE_VIEW_ITEM_SPACE    27

static void getCurDateSettingItem(XMRECT *rect,XMRECT *menu_rc,unsigned index)
{
    unsigned int item_width = DATE_VIEW_ITEM_WIDTH;
    unsigned int item_height = DATE_VIEW_ITEM_HEIGHT;

    unsigned int menu_width = menu_rc->right - menu_rc->left;
    unsigned int date_panel_width = (item_width + DATE_VIEW_ITEM_SPACE) * (DATE_VIEW_COLUMNS + 1) - DATE_VIEW_ITEM_SPACE;

    unsigned int y_offset = (menu_width - date_panel_width)/2;

    unsigned int date_panel_start = menu_rc->left + y_offset;

    rect->left = menu_rc->left + y_offset + (item_width + DATE_VIEW_ITEM_SPACE) * (index % DATE_VIEW_COLUMNS + 1);
    rect->right = rect->left + item_width ;

    rect->top = menu_rc->top + DATE_VIEW_TOP_OFFSET + (index / DATE_VIEW_COLUMNS) * item_height ;
    if(index / DATE_VIEW_COLUMNS >= 1)
    {
        rect->top += DATE_VIEW_ITEM_SPACE;
    }
    rect->bottom = rect->top + item_height;
}

static XMSYSTEMTIME m_dateSetTime;

#define DATE_SETTING_YEAR   0
#define DATE_SETTING_MONTH  1
#define DATE_SETTING_DAY    2
#define DATE_SETTING_HOUR   3
#define DATE_SETTING_MIN    4
#define DATE_SETTING_SEC    5

static void drawDateItemView(HANDLE hWnd,XMRECT *rect,int item_index,u8_t type)
{
    char time[5];
    memset(time,0,sizeof(time));
    const int num_witch = 27;
    switch(item_index)
    {
        case DATE_SETTING_YEAR:
            sprintf(time,"%04d",m_dateSetTime.wYear);
            AP_TextOutNumString(hWnd,rect->left + 15,rect->top + 30,num_witch,time,4,type);
            break;
        case DATE_SETTING_MONTH:
            sprintf(time,"%02d",m_dateSetTime.bMonth);
            AP_TextOutNumString(hWnd,rect->left + 37,rect->top + 30,num_witch,time,2,type);
            rect->left = rect->right + DATE_VIEW_ITEM_SPACE;
            rect->right = rect->left + DATE_VIEW_ITEM_WIDTH;
            type = 0;
            XM_FillRect(hWnd, rect->left, rect->top, rect->right-1, rect->bottom-1, 0xff34352c);
            AP_RomImageDrawByMenuID(AP_ID_DATE_MENU_ICON_N, hWnd, rect, XMGIF_DRAW_POS_CENTER);
            //break;
        case DATE_SETTING_DAY:
            sprintf(time,"%02d",m_dateSetTime.bDay);
            AP_TextOutNumString(hWnd,rect->left + 37,rect->top + 30,num_witch,time,2,type);
            break;
        case DATE_SETTING_HOUR:
            sprintf(time,"%02d",m_dateSetTime.bHour);
            AP_TextOutNumString(hWnd,rect->left + 37,rect->top + 30,num_witch,time,2,type);
            break;
        case DATE_SETTING_MIN:
            sprintf(time,"%02d",m_dateSetTime.bMinute);
            AP_TextOutNumString(hWnd,rect->left + 37,rect->top + 30,num_witch,time,2,type);
            break;
        case DATE_SETTING_SEC:
            sprintf(time,"%02d",m_dateSetTime.bSecond);
            AP_TextOutNumString(hWnd,rect->left + 37,rect->top + 30,num_witch,time,2,type);
            break;
        default:
            break;
    }
}

void updateDateView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData)
{
    XMRECT rect;
    getCurDateSettingItem(&rect,menu_rc,settingViewData->lastSecSubmenuIndex);
    XM_FillRect(hWnd, rect.left, rect.top, rect.right-1, rect.bottom-1, 0xff34352c);
    AP_RomImageDrawByMenuID(AP_ID_DATE_MENU_ICON_N, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
    drawDateItemView(hWnd, &rect, settingViewData->lastSecSubmenuIndex, 0);
    
    getCurDateSettingItem(&rect,menu_rc,settingViewData->curSecSubmenuIndex);
    XM_FillRect(hWnd, rect.left, rect.top, rect.right-1, rect.bottom-1, 0xff34352c);
    AP_RomImageDrawByMenuID(AP_ID_DATE_MENU_ICON_A, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
    drawDateItemView(hWnd, &rect, settingViewData->curSecSubmenuIndex, 1);
}

void initDateView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData)
{
    printf("----------------------> drawDateView ----------------------------->\n");
    u8_t date_set_rows = 2;
    u8_t date_set_columns = DATE_VIEW_COLUMNS;
    
    unsigned int item_width = DATE_VIEW_ITEM_WIDTH;
    unsigned int item_height = DATE_VIEW_ITEM_HEIGHT;
    unsigned int space_width = DATE_VIEW_ITEM_SPACE;

    unsigned int x_offset = DATE_VIEW_TOP_OFFSET;
    unsigned int bottom_offset = DATE_VIEW_BOTTOM_OFFSET;
    unsigned int y_offset = ((menu_rc->right - menu_rc->left) - ((item_width + space_width) * (date_set_columns + 1) - space_width))/2;
    
    XMRECT menu_bar_rect,rect;
    
    settingViewData->secSubmenuItemCount = 6;
    settingViewData->curSecSubmenuIndex = 0;
    settingViewData->lastSecSubmenuIndex = 0;
    
    rect.left = menu_rc->left + y_offset;
    rect.top = menu_rc->top + x_offset;
    rect.right = rect.left + item_width;
    rect.bottom = rect.top + item_height;

    AP_RomImageDrawByMenuID(AP_ID_DATE_MENU_ICON_DATE, hWnd, &rect,XMGIF_DRAW_POS_CENTER);
    rect.top = rect.bottom + DATE_VIEW_ITEM_SPACE;
    rect.bottom = rect.top + item_height;
    AP_RomImageDrawByMenuID(AP_ID_DATE_MENU_ICON_TIME, hWnd, &rect,XMGIF_DRAW_POS_CENTER);

    rect.left = menu_rc->left;
    rect.right = menu_rc->right;
    rect.bottom = menu_rc->bottom;
    rect.top = rect.bottom - 60;
 
    u8_t row_item = 0;
    u8_t index = 0;
	
	XM_GetLocalTime(&m_dateSetTime);
    
	u8_t date_count = 0;
    for(index = 0; index < settingViewData->secSubmenuItemCount; index ++)
    {
        getCurDateSettingItem(&rect,menu_rc,index);
        XM_FillRect(hWnd,rect.left,rect.top,rect.right - 1,rect.bottom - 1,0xff34352c);
        if(settingViewData->curSecSubmenuIndex == index)
        {
            AP_RomImageDrawByMenuID(AP_ID_DATE_MENU_ICON_A, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
        }
        else
        {
            AP_RomImageDrawByMenuID(AP_ID_DATE_MENU_ICON_N, hWnd, &rect, XMGIF_DRAW_POS_CENTER);
        }
        drawDateItemView(hWnd, &rect, index, settingViewData->curSecSubmenuIndex == index);
        
        if( (index + 1)% DATE_VIEW_COLUMNS != 0)
        {
            rect.left = rect.right + 8;
            rect.right = rect.left + DATE_VIEW_ITEM_SPACE;
            if(index / DATE_VIEW_COLUMNS > 0)
            {
                //AP_TextOutDataTimeString(hWnd,rect.left,rect.top + 20,":",2);
            }
            else
            {
                //AP_TextOutDataTimeString(hWnd,rect.left,rect.top + 20,"/",2);
            }
        }
    }
}

//key event handler
static u16 leap_month_table[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void dateViewAdjust(XMMSG *msg,SETTINGVIEWDATA *settingViewData,u8_t dir)
{
    switch(settingViewData->curSecSubmenuIndex)
    {
        case 0:
            if(dir == 1)
            {
                m_dateSetTime.wYear += dir;
            }
            else
            {
                m_dateSetTime.wYear -= (u8_t)((~dir) + 1);
            }
            break;
        case 1:
            m_dateSetTime.bMonth += dir;
            if( m_dateSetTime.bMonth > 12)
            {
                m_dateSetTime.bMonth = 1;
            }
            else if(m_dateSetTime.bMonth < 1)
            {
                m_dateSetTime.bMonth = 12;
            }
            if(m_dateSetTime.bDay > leap_month_table[m_dateSetTime.bMonth - 1])
            {
                m_dateSetTime.bDay = leap_month_table[m_dateSetTime.bMonth - 1];
            }
            break;
        case 2:
            m_dateSetTime.bDay += dir;
            if(m_dateSetTime.bDay > leap_month_table[m_dateSetTime.bMonth - 1])
            {
                m_dateSetTime.bDay = 1;
            }
            else if(m_dateSetTime.bDay < 1 )
            {
                m_dateSetTime.bDay = leap_month_table[m_dateSetTime.bMonth - 1];
            }
            break;
        case 3:
            m_dateSetTime.bHour += dir;
            
            if(m_dateSetTime.bHour == 255)
            {
                m_dateSetTime.bHour = 23;
            }
            else if( m_dateSetTime.bHour > 23)
            {
                m_dateSetTime.bHour = 0;
            }
            break;
        case 4:
            m_dateSetTime.bMinute += dir;
            if(m_dateSetTime.bMinute == 255)
            {
                m_dateSetTime.bMinute = 59;
            }
            else if( m_dateSetTime.bMinute > 59)
            {
                m_dateSetTime.bMinute = 0;
            }
            break;
        case 5:
            m_dateSetTime.bSecond += dir;
            if(m_dateSetTime.bSecond == 255)
            {
                m_dateSetTime.bSecond = 59;
            }
            else if(m_dateSetTime.bSecond > 59)
            {
                m_dateSetTime.bSecond = 0;
            }
            break;
    }
}


void dateViewKeyOkClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
    submenuNextCyc(msg,settingViewData);
}

XMSYSTEMTIME* getDateViewData()
{
    return &m_dateSetTime;
}


