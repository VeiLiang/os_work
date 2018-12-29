//****************************************************************************
//
//	Copyright (C) 2018 HX
//
//	Author	Maxd
//
//	File name: app_submenu_color.c
//	  设置窗口
//
//	Revision history
//
//		2018.12.27	Maxd Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuid.h"
#include "app_submenu_color.h"
#include "rxchip.h"

#define COLOR_VIEW_BG           0xff293040
#define COLOR_SLIDER_UP_COLOR   0xff0c0c0c
#define COLOR_SLIDER_DOWN_COLOR_A   (XM_RGB(235, 182, 24))
#define COLOR_SLIDER_DOWN_COLOR_N   (XM_RGB(3, 75, 157))

#define SLIDER_WIDTH	10
#define SLIDER_MAX      100
#define COLOR_ITEM_MAX_VAL  SLIDER_MAX

#define COLOR_VIEW_X_OFFSET     100
#define COLOR_VIEW_TOP_OFFSET   50

#define COLOR_VIEW_ITEM_HEIGHT  300

#define COLOR_SLIDER_HEIGHT     240

#define COLOR_ITEM_TXT_HEIGHT   25
#define COLOR_ITEM_VAL_TXT_HEIGHT   25
#define COLOR_SPLIT_LINE_WIDTH  12

#define ITEM_WIDTH          174
#define ITEM_HEIGHT         62

static void drawVSlider(HANDLE hWnd,XMRECT *rc,unsigned int upBgcolor,unsigned int downBgcolor,unsigned int thum,int curValue)
{
	unsigned int val_height = (curValue * (rc->bottom - rc->top))/SLIDER_MAX;
	XMRECT slider_rc;
	slider_rc.left = rc->left + (rc->right - rc->left - SLIDER_WIDTH) / 2;
	slider_rc.right = slider_rc.left + SLIDER_WIDTH;
	slider_rc.top = rc->top;
	slider_rc.bottom = rc->bottom;
	
	XM_FillRect(hWnd, slider_rc.left, slider_rc.bottom - val_height, slider_rc.right, slider_rc.bottom, downBgcolor);
	XM_FillRect(hWnd, slider_rc.left, slider_rc.top, slider_rc.right, slider_rc.bottom-val_height, upBgcolor);
	
	XMRECT rect;
	unsigned int val_height_thum = (curValue * (rc->bottom - rc->top - 40))/SLIDER_MAX;
	rect.left = slider_rc.left - 15;
	rect.top = rc->bottom - val_height_thum - 40;
	rect.right = slider_rc.right + 15;
	rect.bottom = rect.top + 40;
	AP_RomImageDrawByMenuID(thum,hWnd,&rect,XMGIF_DRAW_POS_CENTER);
}

static void drawHSlider(HANDLE hWnd,XMRECT *rc,unsigned int leftBgcolor,unsigned int rightBgcolor,unsigned int thum,int curValue)
{
    unsigned int val_height = (curValue%SLIDER_MAX) * ((rc->bottom - rc->top)/SLIDER_MAX);
	XMRECT slider_rc;

    slider_rc.left = rc->left + 10;
    slider_rc.right = rc->right - 10;
    slider_rc.top = rc->top + (rc->bottom - rc->top - SLIDER_WIDTH) / 2;
    slider_rc.bottom = slider_rc.top + SLIDER_WIDTH;

	XM_FillRect(hWnd, slider_rc.left, slider_rc.top, slider_rc.left + val_height, slider_rc.bottom, leftBgcolor);
	XM_FillRect(hWnd, slider_rc.left + val_height, slider_rc.top, slider_rc.right, slider_rc.bottom, rightBgcolor);
	
	XMRECT rect;
	rect.left = slider_rc.left + val_height - 10;
	rect.top = slider_rc.top - 5;   //thumb width is 20;
	rect.right = rect.left + 10;
	rect.bottom = rect.top + 20;
	
	AP_RomImageDrawByMenuID(thum,hWnd,&rect,XMGIF_DRAW_POS_CENTER);
}

static void getColorItemRect(XMRECT *item_rc,XMRECT *menu_rc,int cur_index)
{
    XMRECT rc;
    rc.left = menu_rc->left + COLOR_VIEW_X_OFFSET;
    rc.right = menu_rc->right - COLOR_VIEW_X_OFFSET;
    rc.top = menu_rc->top + COLOR_VIEW_TOP_OFFSET;
    rc.bottom = rc.top + COLOR_VIEW_ITEM_HEIGHT;

    int item_width = (rc.right - rc.left - 5 * COLOR_SPLIT_LINE_WIDTH)/4;
    
    item_rc->top = rc.top;
    item_rc->bottom = rc.bottom;
    item_rc->left = rc.left + item_width*cur_index + (cur_index + 1) * COLOR_SPLIT_LINE_WIDTH;  //除去分割线
    item_rc->right = item_rc->left + item_width;
}

static void drawColorViewItem(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData,int cur_index)
{
    XMRECT item_rc,slider_rc;
    getColorItemRect(&item_rc,menu_rc,cur_index);
    XM_FillRect(hWnd,item_rc.left,item_rc.top - 10,item_rc.right,item_rc.bottom,COLOR_VIEW_BG);
    unsigned int color_val;
    char color_val_str[4];
    switch(cur_index)
    {
        case 0:
            color_val = AP_GetColor_Tone();
            break;
        case 1:
            color_val = AP_GetColor_Brightness();
            break;
        case 2:
            color_val = AP_GetColor_Contrast();
            break;
        case 3:
            color_val = AP_GetColor_Saturation();
            break;
        default:
            break;
    }
    
    sprintf(color_val_str,"%03d",color_val);
    
    AP_TextOutNumString(hWnd,item_rc.left + 20,item_rc.top - 10,16, color_val_str,3, 2);
    slider_rc.left = item_rc.left;
    slider_rc.right = item_rc.right;
    slider_rc.top = item_rc.top + COLOR_ITEM_VAL_TXT_HEIGHT;
    slider_rc.bottom = slider_rc.top+ COLOR_SLIDER_HEIGHT;
    
    if(cur_index == settingViewData->curSecSubmenuIndex)
    {
        drawVSlider(hWnd,&slider_rc,COLOR_SLIDER_UP_COLOR,COLOR_SLIDER_DOWN_COLOR_A,AP_ID_SLIDER_THUMB_N,color_val);
        item_rc.top = item_rc.bottom - COLOR_ITEM_TXT_HEIGHT;
        AP_RomImageDrawByMenuID(AP_ID_SETTING_COLOR_A + cur_index,hWnd,&item_rc,XMGIF_DRAW_POS_CENTER);
    }
    else
    {
        drawVSlider(hWnd,&slider_rc,COLOR_SLIDER_UP_COLOR,COLOR_SLIDER_DOWN_COLOR_N,AP_ID_SLIDER_THUMB_N,color_val);
        item_rc.top = item_rc.bottom - COLOR_ITEM_TXT_HEIGHT;
        AP_RomImageDrawByMenuID(AP_ID_SETTING_COLOR + cur_index,hWnd,&item_rc,XMGIF_DRAW_POS_CENTER);
    }
}

void updateColorView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData)
{
    drawColorViewItem(hWnd,menu_rc,settingViewData,settingViewData->lastSecSubmenuIndex);
    drawColorViewItem(hWnd,menu_rc,settingViewData,settingViewData->curSecSubmenuIndex);
}

void drawColorView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData)
{
    int item_index = 0;
    for(item_index = 0; item_index < settingViewData->secSubmenuItemCount; item_index ++)
    {
        drawColorViewItem(hWnd,menu_rc,settingViewData,item_index);
    }
}

void initColorView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData)
{
    XMRECT txt_rect,rect; 
    
    settingViewData->secSubmenuItemCount = 4;
    XM_FillRect(hWnd,menu_rc->left,menu_rc->top,menu_rc->right,menu_rc->bottom,0xff293040);
    drawColorView(hWnd,menu_rc,settingViewData);
}

////key event handler
static void rxchip_setting_hue(u8_t val)
{
    RXCHIPCMD cmd;
    cmd.cmd = CMD_HUE;
    cmd.dat = val;
    rxchip_setting(cmd);
}

static void rxchip_setting_brightness(u8_t val)
{
    RXCHIPCMD cmd;
    cmd.cmd = CMD_BRIGHTNESS;
    cmd.dat = val;
    rxchip_setting(cmd);
}

static void rxchip_setting_contrast(u8_t val)
{
    RXCHIPCMD cmd;
    cmd.cmd = CMD_CONTRAST;
    cmd.dat = val;
    rxchip_setting(cmd);
}

static void rxchip_setting_saturation(u8_t val)
{
    RXCHIPCMD cmd;
    cmd.cmd = CMD_SATURATION;
    cmd.dat = val;
    rxchip_setting(cmd);
}

//real set when exit menu
BOOL colorViewAdjust(XMMSG *msg,SETTINGVIEWDATA *settingViewData,u8_t dir)
{
	u8_t colorVal;
	BOOL res = FALSE;
    
	XM_printf(">>>>colorViewAdjust, settingViewData->curSecSubmenuIndex:%d\r\n", settingViewData->curSecSubmenuIndex);
	switch(settingViewData->curSecSubmenuIndex )
	{
	    case 0:
			colorVal = AP_GetColor_Tone();
			colorVal += dir;
			if(colorVal == 255)
			{
				colorVal = 0;
			}
			else if(colorVal > COLOR_ITEM_MAX_VAL)
			{
				colorVal = COLOR_ITEM_MAX_VAL;
			}
			else
			{
    			AP_SetColor_Tone(colorVal);
    			res = TRUE;
			}
			break;
	
		case 1:
			colorVal = AP_GetColor_Brightness();
			colorVal += dir;
			if(colorVal == 255)
			{
				colorVal = 0;

			}
			else if(colorVal > COLOR_ITEM_MAX_VAL)
			{
				colorVal = COLOR_ITEM_MAX_VAL;
			}
			else
			{
			    AP_SetColor_Brightness(colorVal);
			    res = TRUE;
			}
			break;
			
		case 2:
			colorVal = AP_GetColor_Contrast();
			colorVal += dir;
			if(colorVal == 255)
			{
				colorVal = 0;

			}
			else if(colorVal > COLOR_ITEM_MAX_VAL)
			{
				colorVal = COLOR_ITEM_MAX_VAL;
			}
			else
			{
			    AP_SetColor_Contrast(colorVal);
			    res = TRUE;
			}
			break;
			
		case 3:
			colorVal = AP_GetColor_Saturation();
			colorVal += dir;
			if(colorVal == 255)
			{
				colorVal = 0;
			}
			else if(colorVal > COLOR_ITEM_MAX_VAL)
			{
				colorVal = COLOR_ITEM_MAX_VAL;
			}
			else
			{
    			AP_SetColor_Saturation(colorVal);
    			res = TRUE;
			}
			break;

		default:
			break;
	}
	
	return res;
}

void colorViewKeyOkClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData)
{
    submenuNextCyc(msg,settingViewData);
}



