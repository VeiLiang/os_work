#ifndef _APP_SUBMENU_DATETIME_H_
#define _APP_SUBMENU_DATETIME_H_
#include "app_settingview.h"
#include "types.h"

void updateDateView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData);
void initDateView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData);
void dateViewAdjust(XMMSG *msg,SETTINGVIEWDATA *settingViewData,u8_t dir);

void dateViewKeyOkClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData);
XMSYSTEMTIME* getDateViewData();

#endif
