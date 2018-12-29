#ifndef _APP_SUBMENU_MOT_H_
#define _APP_SUBMENU_MOT_H_
#include "app_settingview.h"
#include "types.h"

void initMotView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData);
void updateMot(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData);
void motViewKeyOkClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData);

#endif

