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
#include "xm_base.h"

//��ǰͨ��
enum {
	CH_AHD1 =	0,
	CH_AHD2,
	CH_V_AHD12,//�����з֣���CH1,��CH2
	CH_V_AHD21,//�����з֣���CH2,��CH1
	CH_H_AHD12,//�����з֣���CH1,��CH2
	CH_H_AHD21,//�����з֣���CH2,��CH1
	CH_COUNT
};

// MIC����ѡ��
enum {
	AP_SETTING_MIC_OFF =	0,
	AP_SETTING_MIC_ON,
	AP_SETTING_MIC_OPTION_COUNT
};

// LCD�ر�ʱ������ѡ��
enum {
	AP_SETTING_LCD_NEVERCLOSE	=	0,
	AP_SETTING_LCD_1MIN,
	AP_SETTING_LCD_3MIN,
	AP_SETTING_LCD_5MIN,
	#if 0
	AP_SETTING_LCD_20S,
	AP_SETTING_LCD_40S,
	AP_SETTING_LCD_60S,
	AP_SETTING_LCD_120S,
	AP_SETTING_LCD_300S,
	#endif
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
#if 0
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
#endif
	AP_SETTING_LANG_OPTION_COUNT	//	11
};

#define	AP_LANG_COUNT		AP_SETTING_LANG_OPTION_COUNT

// ������ʾ����ѡ��
enum {
	AP_SETTING_VOICE_PROMPTS_OFF = 0,
	AP_SETTING_VOICE_PROMPTS_ON,
	AP_SETTING_VOICE_PROMPTS_OPTION_COUNT
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
	AP_SETTING_RECORDTIME_1M = 0,	//1����
	AP_SETTING_RECORDTIME_3M = 1,	//3����
	AP_SETTING_RECORDTIME_5M,		//5����
	AP_SETTING_RECORDTIME_10M,		//10����
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

// ���ڱ�ǩ����ѡ��
enum {
	AP_SETTING_DATE_LABEL_OFF = 0,
	AP_SETTING_DATE_LABEL_ON,
	AP_SETTING_DATE_LABEL_OPTION_COUNT
};

// �عⲹ������ѡ��
enum {
	AP_SETTING_EXPOSURE_COMPENSATION_POSITIVE_1 = 0,
	AP_SETTING_EXPOSURE_COMPENSATION_POSITIVE_2_3,
	AP_SETTING_EXPOSURE_COMPENSATION_POSITIVE_1_3,
	AP_SETTING_EXPOSURE_COMPENSATION_POSITIVE_0,
	AP_SETTING_EXPOSURE_COMPENSATION_NEGATIVE_1_3,
	AP_SETTING_EXPOSURE_COMPENSATION_NEGATIVE_2_3,
	AP_SETTING_EXPOSURE_COMPENSATION_NEGATIVE_1,
	AP_SETTING_EXPOSURE_COMPENSATION_OPTION_COUNT
};


// ͣ���������ѡ��
enum {
	AP_SETTING_PARK_MONITOR_OFF = 0,
	AP_SETTING_PARK_MONITOR_ON,
	AP_SETTING_PARK_MONITOR_OPTION_COUNT
};

//¼Ӱ�ֱ���
enum {
	AP_SETTING_VIDEORESOLUTION_1080P_30 = 0,
	AP_SETTING_VIDEORESOLUTION_720P_30,
	AP_SETTING_VIDEORESOLUTION_720P_60,
	AP_SETTING_VIDEORESOLUTION_1080P_720P_30,	// 1080P 30֡
	AP_SETTING_VIDEORESOLUTION_1080P_NSTL_30,
	AP_SETTING_VIDEORESOLUTION_1080P_PAL_30,
	AP_SETTING_VIDEORESOLUTION_720P_720P_25,
	AP_SETTING_VIDEORESOLUTION_720P_720P_30,
	AP_SETTING_VIDEORESOLUTION_720P_NSTL_30,
	AP_SETTING_VIDEORESOLUTION_720P_PAL_30,
	AP_SETTING_VIDEORESOLUTION_NSTL_720P_30,
	AP_SETTING_VIDEORESOLUTION_NSTL_NSTL_30,
	AP_SETTING_VIDEORESOLUTION_NSTL_PAL_30,
	AP_SETTING_VIDEORESOLUTION_PAL_720P_30,
	AP_SETTING_VIDEORESOLUTION_PAL_NSTL_30,
	AP_SETTING_VIDEORESOLUTION_PAL_PAL_30,
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
	AP_SETTING_DAY_MODE	=	0,
	AP_SETTING_NIGHT_MODE,	// 1
	AP_SETTING_DAY_NIGHT_MODE_OPTION_COUNT
};
enum {
	AP_SETTING_PHOTO_PRESS	=	0,
	AP_SETTING_PHOTO,	//					1
	AP_SETTING_PHOTO_OPTION_COUNT
};
enum {
	AP_SETTING_VIDEOLOCK_OFF	=	0,
	AP_SETTING_VIDEOLOCK_ON,	// 1
	AP_SETTING_VIDEOLOCK_OPTION_COUNT
};

enum {
	AP_SYS_SETTING_PRESS	=	0,
	AP_SYS_SETTING,	// 1
	AP_SYS_SETTING_OPTION_COUNT
};

enum {
	AP_VIDEO_SWITCH_PRESS	=	0,
	AP_VIDEO_SWITCH,	// 1
	AP_VIDEO_SWITCH_OPTION_COUNT
};

enum {
	AP_PHOTOGRAPH_PRESS	=	0,
	AP_PHOTOGRAPH,	// 1
	AP_PHOTOGRAPH_OPTION_COUNT
};

enum {
	AP_RECORD_SWITCH_PRESS	=	0,
	AP_RECORD_SWITCH,	// 1
	AP_RECORD_SWITCH_OPTION_COUNT
};

enum {
	AP_RECORD_LOCK_PRESS	=	0,
	AP_RECORD_LOCK,	// 1
	AP_RECORD_LOCK_OPTION_COUNT
};


enum {
	AP_RECORD_LIST_PRESS	=	0,
	AP_RECORD_LIST,	// 1
	AP_RECORD_LIST_OPTION_COUNT
};

enum {
	AP_MIC_SWITCH_PRESS	=	0,
	AP_MIC_SWITCH,	// 1
	AP_MIC_SWITCH_OPTION_COUNT
};

enum {
	AP_AHD_ChannelNum_1	= 1,
	AP_AHD_ChannelNum_2,
	AP_AHD_ChannelNum_3,
	AP_AHD_ChannelNum_4,
	AP_AHD_ChannelNum_OPTION_COUNT
};

enum {
	AP_Logo_Enable_OFF	=	0,
	AP_Logo_Enable_ON,	// 1
	AP_Logo_Enable_OPTION_COUNT
};

enum {
	AP_BRIGHT_ATUO_OFF	=	0,
    AP_BRIGHT_ATUO_MEMORY,
	AP_BRIGHT_ATUO_ON,	// 1
	AP_BRIGHT_ATUO_OPTION_COUNT
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

// cvbs��ʽѡ��
enum {
	AP_SETTING_CVBS_NTSC = 0,
	AP_SETTING_CVBS_PAL,
	AP_SETTING_CVBS_COUNT
};

// �عⲹ��ֵ���� ( -2 -1 0 +1 +2 )
enum {
	AP_SETTING_EV_MINUS_2,			// -2
	AP_SETTING_EV_MINUS_1,			// -1
	AP_SETTING_EV_0,					// 0
	AP_SETTING_EV_1,					// 1
	AP_SETTING_EV_2,					// 2

	AP_SETTING_EV_COUNT
};
// ��ԴƵ��
enum {
	AP_SETTING_LIGHTFREQ_CLOSE,
	AP_SETTING_LIGHT_FREQ_50HZ,
	AP_SETTING_LIGHT_FREQ_60HZ,
	AP_SETTING_LIGHTFREQ_COUNT
};
// ��ǿ��
enum {
	AP_SETTING_SHARPENLING_MINOR = 0,	// ����΢
	AP_SETTING_SHARPENLING_SOFT,			// �����
	AP_SETTING_SHARPENLING_STRONG,		// ��ǿ��
	AP_SETTING_SHARPENLING_COUNT
};

// ��Ƶͼ������
enum {
	AP_SETTING_VIDEO_QUALITY_NORMAL = 0,	// ��ͨ��Ƶ��������
	AP_SETTING_VIDEO_QUALITY_GOOD,			// �Ϻ���Ƶ��������
	AP_SETTING_VIDEO_QUALITY_HIGH,			// ������Ƶ��������
	AP_SETTING_VIDEO_QUALITY_COUNT
};

enum{ 
	AP_SETTING_LCD_FLIP_NORMAL, 
	AP_SETTING_LCD_FLIP_MIRROR, 
};

//��Դ״̬
enum {
    POWER_OFF,
    POWER_ON,

};

#define CH1_MIRROR      0x01
#define CH2_MIRROR      0x02

#define MOT_ON          1
#define MOT_OFF         0

#define ACC_ON      0
#define ACC_OFF     1

#define	AP_MENU_DATA_CHECKSUM			0xA5A5A5A5
#define	AP_MENU_DATA_ID					0x554E454D		// "MENU"
//#define	AP_MENU_DATA_VERSION_01010101	0x01010101		// ��ǰ�汾
//#define	AP_MENU_DATA_VERSION_01010102	0x01010102		// ��ǰ�汾, ����CVBS
//#define	AP_MENU_DATA_VERSION_01010103	0x01010103		// ��ǰ�汾, �����عⲹ��,��,ͼ������
#define	AP_MENU_DATA_VERSION	0x20181227

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
	APPMENUITEM_DATE_LABEL,			// ���ڱ�ǩ
	APPMENU_IMAGE_QUALITY,          //ͼ������
	APPMENUITEM_MIC_VOLUME,						// ¼����������
	APPMENUITEM_BELL_VOLUME,					// ������������
    APPMENUITEM_DAY_NIGHT_MODE,                       // ��������ģʽʹ��
	APPMENUITEM_LOCK_MODE,
	APPMENUITEM_PHOTO,
	APPMENUITEM_SYS_SETTING,
	APPMENUITEM_VIDEO_SWITCH,
	APPMENUITEM_PHOTOGRAPH,
	APPMENUITEM_RECORD_SWITCH,
	APPMENUITEM_RECORD_LIST,
	APPMENUITEM_MIC_SWITCH,
    APPMENUITEM_PARKMONITOR,

	// �����������Ͷ��壬ÿһ��������ʾ�ɷֱ�����ر�
	APPMENUITEM_VOICEASSISTANT_DRIVE,					// ��ʻ������ʾ (ƣ�ͼ�ʻ)
	APPMENUITEM_VOICEASSISTANT_BATTERY_ALARM,			// ��ص���������ʾ
	APPMENUITEM_VOICEASSISTANT_CARD_STATUS,			// ���ݿ�״̬������ʾ
	APPMENUITEM_VOICEASSISTANT_MIC_ONOFF,				// ¼��������״̬������ʾ
	APPMENUITEM_VOICEASSISTANT_REC_ONOFF,				// ¼��״̬������ʾ (¼�񿪡��ء�¼��ģʽ)
	APPMENUITEM_VOICEASSISTANT_URGENT_RECORD,			// ����¼���������ر�������ʾ
	APPMENUITEM_VOICEASSISTANT_STORAGE_SPACE,			// ѭ��¼�ƿռ��
	APPMENUITEM_VOICEASSISTANT_NAVIGATE_STATUS,		// ����״̬������ʾ

	APPMENUITEM_CVBS_TYPE,									// CVBS NTSC/PAL��ʽ����
	APPMENUITEM_EV,											// �عⲹ������
    APPMENUITEM_LIGHT_FREQ,
	APPMENUITEM_SHARPENING,									// ��ǿ��
	APPMENUITEM_VIDEO_QUALITY,								// ͼ������
	
    APPMENUITEM_PARKING_LINE,                   //parking line switch
    APPMENUITEM_PARKING_DELAY,                  //parking delay switch
    APPMENUITEM_MIRROR,                         //mirror setting
    
	APPMENUITEM_CH,//��ǰͨ��
	APPMENUITEM_REPLAY_CH,//�ط�ͨ��
	APPMENUITEM_PRE_CH,//��ǰͨ��
	APPMENUITEM_CH1_DELAY,//ͨ��1��ʱ,��λs,1��ʾ1s
	APPMENUITEM_CH2_DELAY,//ͨ��2��ʱ,��λs
	APPMENUITEM_CH3_DELAY,//ͨ��3��ʱ,��λs
	APPMENUITEM_CH4_DELAY,//ͨ��4��ʱ,��λs
	APPMENUITEM_POWER_STATE,//��Դ״̬

	
	APPMENUITEM_COUNT,  //���ֵ,�����ں������,Ҫ��ӵ�ǰ��   

};

typedef struct _tagAPPMENUDATA {
	u32_t		menu_id;				// ID��ʶ
	u32_t		menu_version;			// �˵����ø�ʽ�汾
	u32_t		menu_size;				// ��ǰ�汾���ݽṹ���ֽڴ�С
	u32_t		menu_checksum;			//	��ǰ�汾��������(��ȥ����)�����ۼ�CheckSum + menu_checksum = 0xA5A5A5A5

	// ϵͳ���ò˵���ֵ
	u8_t		mic;						// ¼������/�ر� 1 ���� 0 �ر�
	u8_t		lcd;						// lcd�ر�
	u8_t		key;						// ������
	u8_t		lang;						// ����
	u8_t        day_night;					// ��������ģʽʹ��  1 ʹ�� 0 ��ֹ
	u8_t		video_lock;
	u8_t        photo;

	u8_t        sys_setting;
	u8_t        video_switch;
	u8_t        photograph;
	u8_t        record_switch;
	u8_t        record_list;
	u8_t        mic_switch;

	// ¼�����ò˵���
	u8_t		voice_prompts;			// ������ʾ 1 ���� 0 �ر�
	u8_t		recordtime_alarm;		// ��¼ʱ�䱨������
	u8_t		video_time_size;		// ¼��ֶ�ʱ��
	
	// ��ײ������
	u8_t		collision_sensitivity;
	
	//ͣ�����
	u8_t       parkmonitor;
	
	// ʱ��ˮӡ
	u8_t		time_stamp;				// ʱ���ˮӡ����/�ر�	1 ���� 0 �ر�
	u8_t		flag_stamp;				// ��־ˮӡ 1 ���� 0 �ر�
	u8_t		flag_stamp_update;		// ��־ˮӡ�޸ı�־ 1 �޸� 0 δ�޸�
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

    u8_t		date_label;			// ���ڱ�ǩ

	u8_t		voiceassistant_drive;					// ��ʻ������ʾ (ƣ�ͼ�ʻ)
	u8_t		voiceassistant_battery_alarm;			// ��ص���������ʾ
	u8_t		voiceassistant_card_status;			// ���ݿ�״̬������ʾ
	u8_t		voiceassistant_mic_onoff;				// ¼��������״̬������ʾ
	u8_t		voiceassistant_rec_onoff;				// ¼��״̬������ʾ (¼�񿪡��ء�¼��ģʽ)
	u8_t		voiceassistant_urgent_record;			// ����¼���������ر�������ʾ
	u8_t		voiceassistant_storage_space;			// ѭ��¼�ƿռ��������ʾ
	u8_t		voiceassistant_navigate_status;		// ����״̬������ʾ

	u8_t		cvbs_type;							// cvbs��ʽѡ��
	u8_t		exposure_compensation;			// Ev �عⲹ��, -3,~ + 3
	u8_t        light_freq;
	u8_t		sharpening;							// ��ǿ��ֵ
	u8_t		video_quality;						// ��Ƶ��������(����, �Ϻ�, ��)
    u8_t        NoSigned;
    u8_t        LCD_Rotate;
    u8_t        auto_brightness;
    u8_t        blue_show;
    u8_t        guide_camer1;
    u8_t        guide_camer2;
    u8_t        guide_camer3;
    u8_t        guide_camer4;
    u8_t        camera1;
    u8_t        camera2;
    u8_t        camera3;
    u8_t        camera4;
    u8_t        camera_mot;

    u8_t        carbackdelay;
    u8_t        poweron_brightness;
    u8_t        AHD_ChannelNum;
    u8_t        VolOnOff;
    u8_t        LogoEnable;
    u8_t        AutoDimOnOff;
    u8_t        Middle_RED_LIne;
    u8_t        Audio_PWM;
    u8_t        VCOM_PWM;
    u8_t        Color_Brightness;
    u8_t        Color_Contrast;
    u8_t        Color_Saturation;
    u8_t        Color_Tone;
    u8_t        AHD_Select;
    int         Red_Location;
    u8_t        AUTO_Switch_OnOff;
    u8_t        PowerOn_Memory;
    u8_t        PowerOff_Restore;

    u8_t        parking_line;
    u8_t        parking_delay;
    u8_t        mirror;

	u8_t curch;//��ǰͨ��
	u8_t replay_ch;//�ط�ͨ��
    u8_t prech;//��ǰͨ��
    u8_t ch1_delay;//ͨ��1��ʱ
    u8_t ch2_delay;//ͨ��2��ʱ
    u8_t ch3_delay;//ͨ��3��ʱ
    u8_t ch4_delay;//ͨ��4��ʱ
	u8_t power_state;//��Դ״̬
	XMSYSTEMTIME buildtime;
	u8_t update_date_falg;//ʱ����±�־
	
	u32_t crc32_checksum;//�������һ��
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

// �ָ�������ʼ����
// ����ֵ
//   0    �ɹ�
//   < 0  ʧ��
int AP_RestoreMenuData (void);


extern u8_t APP_GetMemory(VOID);
extern u8_t APP_GetPowerOff_Restore(VOID);
extern VOID APP_SetPowerOff_Restore(u8_t PowerOff_Restore);
extern VOID APP_SetPowerOn_Memory(u8_t PowerOn_Memory);
extern u8_t APP_GetPowerOn_Memory(VOID);
extern u8_t APP_GetAuto_DimOnOff(VOID);
extern u8_t APP_GetVOl_OnOff(VOID);
extern VOID APP_SetAuto_Switch(u8_t Auto_switch);



extern int	AP_VerifyMenuData (APPMENUDATA *lpMenuData);



// ��Flash����˵�����
// 1 ���سɹ�
// 0 ����ʧ��
extern int	AP_LoadMenuData (APPMENUDATA *lpMenuData);


// ���˵�����д��Flash
// 1 ����ɹ�
//	0 ����ʧ��
extern int	AP_SaveMenuData (APPMENUDATA *lpMenuData);



extern void APP_SaveMenuData(void);


extern int AP_RestoreMenuData (void);


extern unsigned int AP_GetMenuItem (int menu_item);



// ���ò˵�ѡ��
extern void AP_SetMenuItem (int menu_item, unsigned int item_value);



extern unsigned int AP_GetVideoTimeSize (void);


// ����"��ʱ��¼"ѡ���ʱ��ֵ(��)
extern unsigned int AP_GetRecordDelayTime (void);


//��ȡ�Զ����ȿ���
extern unsigned int AP_GetAutoBrightNess(void);

//Ĭ��Ϊ��ɫ
extern unsigned int AP_GetBlue(void);

//����ͷ����
extern unsigned int AP_GetAHD_ChannelNum(void);

//����ͷ ��ʱ1
extern unsigned int AP_GetDelay_Camera1(void);

//����ͷ ��ʱ2
extern unsigned int AP_GetDelay_Camera2(void);

//����ͷ ��ʱ3
extern unsigned int AP_GetDelay_Camera3(void);

//����ͷ ��ʱ4
extern unsigned int AP_GetDelay_Camera4(void);


//������������
extern unsigned int AP_GetBright_Switch(void);


extern VOID AP_SetBright_Switch(u8_t Bright_Switch);



//���ؿ���logo
extern unsigned int AP_GetLogo(void);


//���غ���������ʾ
extern unsigned int AP_GetMidle_RED_Line(void);


//����ͷ ������ʶ1
extern unsigned int AP_GetGuide_Camera1(void);


//����ͷ ������ʶ2
extern unsigned int AP_GetGuide_Camera2(void);


//����ͷ ������ʶ3
extern unsigned int AP_GetGuide_Camera3(void);


extern unsigned int AP_GetCamera_Mot(void);


//����ͷ ������ʶ4
extern unsigned int AP_GetGuide_Camera4(void);

//��ȡ����
extern unsigned int AP_GetAudo_PWM();

extern VOID AP_SetAudo_PWM(int Data_PWM);


extern unsigned int AP_GetVCOM_PWM();

extern void AP_SetVCOM_PWM(u8_t Data_PWM);


extern unsigned int AP_GetLCD_Rotate(void);


extern void AP_SetLCD_Rotate(u8_t Lcd_Rotate);


extern unsigned int AP_GetColor_Brightness(void);

extern void AP_SetColor_Brightness(u8_t Brightness);


extern VOID AP_SetCamera_Mot(u8_t mot);


unsigned int AP_GetColor_Contrast(void);

extern VOID AP_SetColor_Contrast(u8_t Contrast);


extern unsigned int AP_GetColor_Saturation(void);

extern VOID AP_SetColor_Saturation(u8_t Struation);


extern unsigned int AP_GetColor_Tone(void);


extern void AP_SetColor_Tone(u8_t Tone);

extern VOID Select_Background_Color(VOID);


extern unsigned int AP_GetLang();


extern unsigned int AP_GetAHD_Select();


extern VOID AP_SetAHD_Select(u8_t select);


extern unsigned int AP_GetRed_Location();


extern VOID AP_SetRed_Location(int Location);


extern u8_t APP_GetAudio_Sound(VOID);


extern VOID APP_SetAudio_Sound(u8_t Audio_Sound);


extern u8_t APP_GetAuto_Switch(VOID);




#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_APP_MENUDATA_H_
