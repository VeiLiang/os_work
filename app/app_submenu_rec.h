#ifndef _APP_SUBMENU_REC_H_
#define _APP_SUBMENU_REC_H_

#include "app_settingview.h"
#include "types.h"

typedef enum
{
	CYC_REC_ITEM=0,
	TIME_STAMP_ITEM,
	CARBACK_LINE_ITEM,
	CARBACK_DELAY_TIME_ITEM,
}SUB_REC_MENU_ITEM;


void updateVideoView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData);
void initVideoView(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData);
void videoViewKeyUpClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData);
void videoViewKeyDownClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData);
void videoViewKeyOkClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData);

#endif
