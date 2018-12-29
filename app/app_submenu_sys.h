#ifndef _APP_SUBMENU_SYS_H_
#define _APP_SUBMENU_SYS_H_
#include "app_settingview.h"
#include "types.h"

typedef enum
{
	SCREEN_TIME_ITEM=0,//屏保时间
	SYS_LANG_ITEM,//系统语言
	CH1_MIRROR_ITEM,//通道1镜像
	SD_FORMAT_ITEM,//SD卡格式化
	LCD_RORATE_ITEM,//屏翻转
	DEFAULT_PARAM_ITEM,//恢复默认值
	CH2_MIRROR_ITEM,//通道2镜像
	SW_VERSION_ITEM,//软件版本
}SUB_SYS_MENU_ITEM;

void initSysMenu(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData);
void updateSysMenu(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData);
void sysMenuKeyOkClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData);
void sysMenuKeyUpClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData);
void sysMenuKeyDownClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData);

#endif
