#ifndef _APP_SETTINGVIEW_H_
#define _APP_SETTINGVIEW_H_

#include "app_menuoptionview.h"

#define	OSD_AUTO_HIDE_TIMEOUT			10000

// 设置的设置菜单项及顺序定义, 0表示第一个项目(最顶部)
typedef enum  {
    #ifdef _XM_MIC_SWITCH_ENABLE
	SETTING_MENU_MIC = 0,						// MIC
	#endif

	SETTING_MENU_TIME,
	SETTING_MENU_MOT,
	SETTING_MENU_DEC,
	SETTING_MENU_COLOR,
	SETTING_MENU_REC,
	SETTING_MENU_SYS,
	SETTING_MENU_HOME,
	SETTING_MENU_COUNT							// 设置项个数
} SETTING_MENU_DEFINE;


// 私有命令定义
#define	SETTING_COMMAND_MODIFY		0
#define	SETTING_COMMAND_RECORD		1
#define	SETTING_COMMAND_RETURN		2

typedef struct _tagSETTINGMENUDATA {
    u8_t            nVolOnOff;
}SETTINGMENUDATA;


typedef struct tagSETTINGVIEWDATA {
	int				nTopItem;					// 第一个可视的菜单项
	int				nCurItem;					// 当前选择的菜单项
	int             nLastItem;
	u8_t			nmenuStatus;				//当前菜单状态，子菜单模式 主菜单模式
	int				nItemCount;					// 菜单项个数
	int				nTouchItem;					// 当前触摸项, -1 表示没有

    int    curSecSubmenuIndex;         //
    int    lastSecSubmenuIndex;        //
    int    secSubmenuItemCount;
    
	APPMENUOPTION	menuOption;					// 菜单选项
    SETTINGMENUDATA *SettingData;
	XMBUTTONCONTROL	btnControl;				    // 按钮控件

	// 开关控件
	#ifdef _XM_VOICE_PROMPTS_ENABLE_
	HANDLE			hVoiceSwitchControl;		// 语音提示的简单开关控件实现
	#endif

	#ifdef _XM_MIC_SWITCH_ENABLE
	HANDLE			hMicSwitchControl;		// 录音
	#endif

#ifdef _XM_KEY_BEEP_ENABLE_
	HANDLE			hKeySwitchControl;		// 按键音
#endif

//	BYTE				bTimer;						// 定时器

} SETTINGVIEWDATA;

typedef struct menu_icon{
    unsigned int icon;
    unsigned int nor_bg;
    unsigned int act_bg;
    unsigned int txt;
}MENU_ICON;

//
#define LISTVIEW_ITEM_HEIGHT    60
#define LISTVIEW_ITEM_SPACE     15
#define LISTVIEW_TOP_OFFSET     80
#define LISTVIEW_MAX_VISIBLE_COUNT  4


void submenuNextCyc(XMMSG *msg,SETTINGVIEWDATA *settingViewData);
void submenuPrivCyc(XMMSG *msg,SETTINGVIEWDATA *settingViewData);
void getListViewItemRect(const XMRECT *rect, XMRECT *item_rect,int item_index );

#endif

