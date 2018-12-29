#ifndef _APP_SETTINGVIEW_H_
#define _APP_SETTINGVIEW_H_

#include "app_menuoptionview.h"

#define	OSD_AUTO_HIDE_TIMEOUT			10000

// ���õ����ò˵��˳����, 0��ʾ��һ����Ŀ(���)
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
	SETTING_MENU_COUNT							// ���������
} SETTING_MENU_DEFINE;


// ˽�������
#define	SETTING_COMMAND_MODIFY		0
#define	SETTING_COMMAND_RECORD		1
#define	SETTING_COMMAND_RETURN		2

typedef struct _tagSETTINGMENUDATA {
    u8_t            nVolOnOff;
}SETTINGMENUDATA;


typedef struct tagSETTINGVIEWDATA {
	int				nTopItem;					// ��һ�����ӵĲ˵���
	int				nCurItem;					// ��ǰѡ��Ĳ˵���
	int             nLastItem;
	u8_t			nmenuStatus;				//��ǰ�˵�״̬���Ӳ˵�ģʽ ���˵�ģʽ
	int				nItemCount;					// �˵������
	int				nTouchItem;					// ��ǰ������, -1 ��ʾû��

    int    curSecSubmenuIndex;         //
    int    lastSecSubmenuIndex;        //
    int    secSubmenuItemCount;
    
	APPMENUOPTION	menuOption;					// �˵�ѡ��
    SETTINGMENUDATA *SettingData;
	XMBUTTONCONTROL	btnControl;				    // ��ť�ؼ�

	// ���ؿؼ�
	#ifdef _XM_VOICE_PROMPTS_ENABLE_
	HANDLE			hVoiceSwitchControl;		// ������ʾ�ļ򵥿��ؿؼ�ʵ��
	#endif

	#ifdef _XM_MIC_SWITCH_ENABLE
	HANDLE			hMicSwitchControl;		// ¼��
	#endif

#ifdef _XM_KEY_BEEP_ENABLE_
	HANDLE			hKeySwitchControl;		// ������
#endif

//	BYTE				bTimer;						// ��ʱ��

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

