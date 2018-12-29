#ifndef _APP_SUBMENU_COLOR_H_
#define _APP_SUBMENU_COLOR_H_

#include "app_settingview.h"
#include "types.h"

void initColorView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData);
void drawColorView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData);
void updateColorView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData);
BOOL colorViewAdjust(XMMSG *msg,SETTINGVIEWDATA *settingViewData,u8_t dir);
void colorViewKeyOkClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData);

#endif
