#ifndef _APP_SUBMENU_SYS_H_
#define _APP_SUBMENU_SYS_H_
#include "app_settingview.h"
#include "types.h"

typedef enum
{
	SCREEN_TIME_ITEM=0,//����ʱ��
	SYS_LANG_ITEM,//ϵͳ����
	CH1_MIRROR_ITEM,//ͨ��1����
	SD_FORMAT_ITEM,//SD����ʽ��
	LCD_RORATE_ITEM,//����ת
	DEFAULT_PARAM_ITEM,//�ָ�Ĭ��ֵ
	CH2_MIRROR_ITEM,//ͨ��2����
	SW_VERSION_ITEM,//����汾
}SUB_SYS_MENU_ITEM;

void initSysMenu(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData);
void updateSysMenu(HANDLE hWnd,XMRECT *menu_rc,SETTINGVIEWDATA *settingViewData);
void sysMenuKeyOkClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData);
void sysMenuKeyUpClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData);
void sysMenuKeyDownClick(XMMSG *msg,SETTINGVIEWDATA *settingViewData);

#endif
