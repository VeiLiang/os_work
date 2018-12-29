//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_menudata.h
//	  �˵����ݽṹ����
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _XM_APP_MENUDATA_H_
#define _XM_APP_MENUDATA_H_

#if defined (__cplusplus)
	extern "C"{
#endif

#include <xm_type.h>

// MIC����ѡ��
enum {
	AP_SETTING_MIC_OFF =	0,
	AP_SETTING_MIC_ON,
	AP_SETTING_MIC_OPTION_COUNT
};
enum {
	AP_VIDEO_SETTING_MONITOR_OFF = 0,
	AP_VIDEO_SETTING_MONITOR_ON,
	AP_VIDEO_SETTING_MONITOR_OPTION_COUNT
};

// LCD�ر�ʱ������ѡ��
enum {
	AP_SETTING_LCD_NEVERCLOSE	=	0,
	AP_SETTING_LCD_20S,
	AP_SETTING_LCD_40S,
	AP_SETTING_LCD_60S,
	AP_SETTING_LCD_120S,	
	AP_SETTING_LCD_300S,
	AP_SETTING_LCD_OPTION_COUNT
};

// KEY����ѡ��
enum {
	AP_SETTING_KEY_OFF	=		0,
	AP_SETTING_KEY_ON	,
	AP_SETTING_KEY_OPTION_COUNT
};

// ��������ѡ��
enum {
	AP_SETTING_LANG_CHINESE_SIMPLIFIED = 0,// ��������
	AP_SETTING_LANG_ENGLISH	= 0,				// Ӣ��
	AP_SETTING_LANG_RUSSIAN,					// ����
	AP_SETTING_LANG_CHINESE_TRADITIONAL,	// ��������
	AP_SETTING_LANG_FRENCH,						// ����
	AP_SETTING_LANG_JAPANESE,					// ����
	AP_SETTING_LANG_KOREAN,						// ����
	AP_SETTING_LANG_GERMAN,	//			8		// ����
	AP_SETTING_LANG_SPANISH,//			7		// ��������
	AP_SETTING_LANG_ITALIAN,	//		9		// �������
	AP_SETTING_LANG_ARABIC,		//		10		// ��������
	AP_SETTING_LANG_OPTION_COUNT	//	11
};

#define	AP_LANG_COUNT		AP_SETTING_LANG_OPTION_COUNT

// ������ʾ����ѡ��
enum {
	AP_SETTING_VOICE_PROMPTS_OFF = 0,
	AP_SETTING_VOICE_PROMPTS_ON,
	AP_SETTING_VOICE_PROMPTS_OPTION_COUNT
};
// ͣ�����ѡ��
enum {
	AP_VIDEO_SETTING_MONITOR_OFF = 0,
	AP_VIDEO_SETTING_MONITOR_ON,
	AP_VIDEO_SETTING_MONITOR_OPTION_COUNT
};
// ˮӡ����ѡ��
enum {
	AP_SETTING_VIDEO_TIMESTAMP_OFF	= 	0,
	AP_SETTING_VIDEO_TIMESTAMP_ON,	//			1
	AP_SETTING_VIDEO_TIMESTAMP_OPTION_COUNT
};

// ��ʾģʽ����ѡ��
enum {
	AP_SETTING_DEMOMODE_OFF	=	0,
	AP_SETTING_DEMOMODE_ON,	//					1
	AP_SETTING_DEMOMODE_OPTION_COUNT
};

// ��¼ʱ�䱨������ѡ��
enum {
	AP_SETTING_RECORDALARM_OFF	=	0,
	AP_SETTING_RECORDALARM_10M,	//				1	// 10����
	AP_SETTING_RECORDALARM_20M,	//				2
	AP_SETTING_RECORDALARM_40M,	//				3
	AP_SETTING_RECORDALARM_1H,		//				4	// 1Сʱ
	AP_SETTING_RECORDALARM_2H,		//				5
	AP_SETTING_RECORDALARM_4H,		//				6
	AP_SETTING_RECORDALARM_6H,		//				7
	AP_SETTING_RECORDALARM_8H,		//				8
	AP_SETTING_RECORDALARM_10H,	//				9
	AP_SETTING_RECORDALARM_12H,	//				10
	AP_SETTING_RECORDALARM_OPTION_COUNT
};

// ¼���ӳ�ʱ������ѡ��
enum {
	AP_SETTING_VIDEO_DELAY_0S	= 0,
	AP_SETTING_VIDEO_DELAY_3S,	//				1
	AP_SETTING_VIDEO_DELAY_5S,	//				2
	AP_SETTING_VIDEO_DELAY_10S,	//			3
	AP_SETTING_VIDEO_DELAY_15S,	//			4
	AP_SETTING_VIDEO_DELAY_OPTION_COUNT
};

// ¼��ֶ�ʱ������ѡ��
enum {
	AP_SETTING_RECORDTIME_1M = 0,		// 1����
	AP_SETTING_RECORDTIME_2M,	//				1		// 2����
	AP_SETTING_RECORDTIME_3M,	//				2		// 3����
	AP_SETTING_RECORDTIME_5M,	//				3		// 5����
	AP_SETTING_RECORDTIME_10M,	//				4		// 10����
	AP_SETTING_RECORDTIME_15M,	//				5		// 15����
	AP_SETTING_RECORDTIME_OPTION_COUNT
};

// ��ײ������ѡ��
enum {
	AP_SETTING_COLLISION_1G = 0,		// 1G���ٶ�
	AP_SETTING_COLLISION_2G,		
	AP_SETTING_COLLISION_3G,		
	AP_SETTING_COLLISION_4G,
	AP_SETTING_COLLISION_OPTION_COUNT
};

// ����¼��ʱ��
enum {
	AP_SETTING_URGENTRECORDTIME_5M	=	0,		// 5����
	AP_SETTING_URGENTRECORDTIME_10M,	//				1	// 10����
	AP_SETTING_URGENTRECORDTIME_15M,	//				2	// 15����
	AP_SETTING_URGENTRECORDTIME_20M,	//				3	// 20����
	AP_SETTING_URGENTRECORDTIME_COUNT
};

// ¼��ֱ���
enum {
	AP_SETTING_VIDEORESOLUTION_1080P_30 = 0,	// 1080P 30֡
	AP_SETTING_VIDEORESOLUTION_1080P_60,		// 1080P�˶� 60֡
	AP_SETTING_VIDEORESOLUTION_720P_30,
	AP_SETTING_VIDEORESOLUTION_720P_60,
	AP_SETTING_VIDEORESOLUTION_COUNT
};

// ���� 10��
enum {
	AP_SETTING_SOUNDVOLUME_0	= 0,		// �ر�
	AP_SETTING_SOUNDVOLUME_1,
	AP_SETTING_SOUNDVOLUME_2,
	AP_SETTING_SOUNDVOLUME_3,
	AP_SETTING_SOUNDVOLUME_4,
	AP_SETTING_SOUNDVOLUME_5,
	AP_SETTING_SOUNDVOLUME_6,
	AP_SETTING_SOUNDVOLUME_7,
	AP_SETTING_SOUNDVOLUME_8,
	AP_SETTING_SOUNDVOLUME_9,
	AP_SETTING_SOUNDVOLUME_10,
	AP_SETTING_SOUNDVOLUME_11,
	AP_SETTING_SOUNDVOLUME_12,
	AP_SETTING_SOUNDVOLUME_13,
	AP_SETTING_SOUNDVOLUME_14,
	AP_SETTING_SOUNDVOLUME_15,
	AP_SETTING_SOUNDVOLUME_COUNT
};

// ��������ģʽ����ѡ��
enum {
	AP_SETTING_DAYNIGHT_OFF	=	0,
	AP_SETTING_DAYNIGHT_ON,	//					1
	AP_SETTING_DAYNIGHT_OPTION_COUNT
};


enum {
	AP_SETTING_VOICEASSISTANT_BATTERY_ALARM = 0,		// ��ص���������ʾ
	AP_SETTING_VOICEASSISTANT_CARD_STATUS,				// ���ݿ�״̬������ʾ
	AP_SETTING_VOICEASSISTANT_MIC_ONOFF,				// ¼��������״̬������ʾ
	AP_SETTING_VOICEASSISTANT_REC_ONOFF,				// ¼��״̬������ʾ (¼�񿪡��ء�¼��ģʽ)
	AP_SETTING_VOICEASSISTANT_URGENT_RECORD,			// ����¼���������ر�������ʾ
	AP_SETTING_VOICEASSISTANT_STORAGE_SPACE,			// ѭ��¼�ƿռ��
	AP_SETTING_VOICEASSISTANT_NAVIGATE_STATUS,		// ����״̬������ʾ
	AP_SETTING_VOICEASSISTANT_DRIVE,						// ��ʻ������ʾ (ƣ�ͼ�ʻ)
	AP_SETTING_VOICEASSISTANT_COUNT
};

#define	AP_MENU_DATA_CHECKSUM			0xA5A5A5A5
#define	AP_MENU_DATA_ID					0x554E454D		// "MENU"
#define	AP_MENU_DATA_VERSION_01010101	0x01010101		// ��ǰ�汾

enum {
	APPMENUITEM_MIC = 0,
	APPMENUITEM_LCD,
	APPMENUITEM_KEY,
	APPMENUITEM_LANG,
	APPMENUITEM_VOICE_PROMPTS,
	APPMENUITEM_RECORDTIME_ALARM,
	APPMENUITEM_VIDEO_TIME_SIZE,
	APPMENUITEM_COLLISION_SENSITIVITY,
	APPMENUITEM_TIME_STAMP,						// ʱ��ˮӡ
	APPMENUITEM_NAVI_STAMP,						// ����ˮӡ
	APPMENUITEM_FLAG_STAMP,						// ��־ˮӡ
	APPMENUITEM_FLAG_STAMP_UPDATE,			// ��־ˮӡ�޸ı�־ 1 �ѱ��޸�
	APPMENUITEM_RECORD_DELAY,
	APPMENUITEM_DEMO_MODE,
	APPMENUITEM_URGENT_RECORD_TIME,
	APPMENUITEM_VIDEO_RESOLUTION,				// ¼��ֱ���
	APPMENUITEM_MIC_VOLUME,						// ¼����������
	APPMENUITEM_BELL_VOLUME,					// ������������

	APPMENUITEM_DAY_NIGHT_MODE,				// ��������ģʽʹ��

	// �����������Ͷ��壬ÿһ��������ʾ�ɷֱ�����ر�
	APPMENUITEM_VOICEASSISTANT_DRIVE,					// ��ʻ������ʾ (ƣ�ͼ�ʻ)
	APPMENUITEM_VOICEASSISTANT_BATTERY_ALARM,			// ��ص���������ʾ
	APPMENUITEM_VOICEASSISTANT_CARD_STATUS,			// ���ݿ�״̬������ʾ
	APPMENUITEM_VOICEASSISTANT_MIC_ONOFF,				// ¼��������״̬������ʾ
	APPMENUITEM_VOICEASSISTANT_REC_ONOFF,				// ¼��״̬������ʾ (¼�񿪡��ء�¼��ģʽ)
	APPMENUITEM_VOICEASSISTANT_URGENT_RECORD,			// ����¼���������ر�������ʾ
	APPMENUITEM_VOICEASSISTANT_STORAGE_SPACE,			// ѭ��¼�ƿռ��
	APPMENUITEM_VOICEASSISTANT_NAVIGATE_STATUS,		// ����״̬������ʾ

	APPMENUITEM_COUNT
};

typedef struct _tagAPPMENUDATA {
	u32_t		menu_id;					// ID��ʶ
	u32_t		menu_version;			// �˵����ø�ʽ�汾
	u32_t		menu_size;				// ��ǰ�汾���ݽṹ���ֽڴ�С
	u32_t		menu_checksum;			//	��ǰ�汾��������(��ȥ����)�����ۼ�CheckSum + menu_checksum = 0xA5A5A5A5

	// ϵͳ���ò˵���ֵ
	u8_t		mic;						// ¼������/�ر� 1 ���� 0 �ر�
	u8_t		lcd;						// lcd�ر�
	u8_t		key;						// ������
	u8_t		lang;						// ����
	u8_t		video_lock;		// ��������ģʽʹ��  1 ʹ�� 0 ��ֹ

	// ¼�����ò˵���
	u8_t		voice_prompts;			// ������ʾ 1 ���� 0 �ر�
	u8_t		recordtime_alarm;		// ��¼ʱ�䱨������
	u8_t		video_time_size;		// ¼��ֶ�ʱ��
	// ��ײ������
	u8_t		collision_sensitivity;		
	//ͣ�����
	u8_t       monitor;
	// ʱ��ˮӡ
	u8_t		time_stamp;				// ʱ���ˮӡ����/�ر�	1 ���� 0 �ر�
	u8_t		flag_stamp;				// ��־ˮӡ 1 ���� 0 �ر�
	u8_t		flag_stamp_update;	// ��־ˮӡ�޸ı�־ 1 �޸� 0 δ�޸�
	u8_t		navi_stamp;				// GPS/��������ˮӡ 1 ���� 0 �ر�
	// ��¼��ʱ
	u8_t		record_delay;			// ��¼��ʱ
	// ����¼��ʱ��
	u8_t		urgent_record_time;	// 0 --> 5����
											// 1 --> 10����
											// 2 --> 15����
											// 3 --> 20����

	// ϵͳ���ò˵�
	u8_t		demo_mode;				// �޿���ʱ��������ʾģʽ 1 ʹ�� 0 ��ֹ

	u8_t		video_resolution;		// ¼��ֱ���
	u8_t		mic_volume;				// MIC����
	u8_t		bell_volume;			// ��������

	u8_t		voiceassistant_drive;					// ��ʻ������ʾ (ƣ�ͼ�ʻ)
	u8_t		voiceassistant_battery_alarm;			// ��ص���������ʾ
	u8_t		voiceassistant_card_status;			// ���ݿ�״̬������ʾ
	u8_t		voiceassistant_mic_onoff;				// ¼��������״̬������ʾ
	u8_t		voiceassistant_rec_onoff;				// ¼��״̬������ʾ (¼�񿪡��ء�¼��ģʽ)
	u8_t		voiceassistant_urgent_record;			// ����¼���������ر�������ʾ
	u8_t		voiceassistant_storage_space;			// ѭ��¼�ƿռ��������ʾ
	u8_t		voiceassistant_navigate_status;		// ����״̬������ʾ

	//
	u8_t curch;//��ǰͨ��
	
} APPMENUDATA;

extern APPMENUDATA AppMenuData;

// ϵͳ�ṩ�Ķ�ȡ�˵����ݵĽӿں���
int		XM_LoadMenuData (void *lpMenuData, int cbMenuData);
int		XM_SaveMenuData (void *lpMenuData, int cbMenuData);

// �Բ˵������ݽ��м��
int		AP_VerifyMenuData (APPMENUDATA *lpMenuData);
// �˵������ݶ�ȡ/д��
int		AP_LoadMenuData (APPMENUDATA *lpMenuData);
int		AP_SaveMenuData (APPMENUDATA *lpMenuData);

// ��ȡ�˵�������
unsigned int	AP_GetMenuItem (int menu_item);
// ���ò˵���
void				AP_SetMenuItem (int menu_item, unsigned int item_value);

// ���ء���Ƶ��¼�ֶ�ʱ�䳤�ȡ�ѡ���ʱ��ֵ(��)
unsigned int	AP_GetVideoTimeSize (void);

// ���ء���¼ʱ�䱨����ѡ���ʱ��ֵ (��)
unsigned int	AP_GetRecordTimeAlarm (void);

// ����"��ʱ��¼"ѡ���ʱ��ֵ(��)
unsigned int AP_GetRecordDelayTime (void);





#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_APP_MENUDATA_H_
