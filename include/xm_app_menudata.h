//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_menudata.h
//	  菜单数据结构定义
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

//当前通道
enum {
	CH_AHD1 =	0,
	CH_AHD2,
	CH_V_AHD12,//屏宽中分，左CH1,右CH2
	CH_V_AHD21,//屏宽中分，左CH2,右CH1
	CH_H_AHD12,//屏高中分，左CH1,右CH2
	CH_H_AHD21,//屏高中分，左CH2,右CH1
	CH_COUNT
};

// MIC设置选项
enum {
	AP_SETTING_MIC_OFF =	0,
	AP_SETTING_MIC_ON,
	AP_SETTING_MIC_OPTION_COUNT
};

// LCD关闭时间设置选项
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

// KEY设置选项
enum {
	AP_SETTING_KEY_OFF	=		0,
	AP_SETTING_KEY_ON	,
	AP_SETTING_KEY_OPTION_COUNT
};

// 语言设置选项
enum {
	AP_SETTING_LANG_CHINESE_SIMPLIFIED = 0,// 简体中文
#if 0
	AP_SETTING_LANG_ENGLISH	= 0,				// 英语
	AP_SETTING_LANG_RUSSIAN,					// 俄语
	AP_SETTING_LANG_CHINESE_TRADITIONAL,	// 繁体中文
	AP_SETTING_LANG_FRENCH,						// 法语
	AP_SETTING_LANG_JAPANESE,					// 日语
	AP_SETTING_LANG_KOREAN,						// 韩语
	AP_SETTING_LANG_GERMAN,	//			8		// 德语
	AP_SETTING_LANG_SPANISH,//			7		// 西班牙语
	AP_SETTING_LANG_ITALIAN,	//		9		// 意大利语
	AP_SETTING_LANG_ARABIC,		//		10		// 阿拉伯语
#endif
	AP_SETTING_LANG_OPTION_COUNT	//	11
};

#define	AP_LANG_COUNT		AP_SETTING_LANG_OPTION_COUNT

// 语音提示设置选项
enum {
	AP_SETTING_VOICE_PROMPTS_OFF = 0,
	AP_SETTING_VOICE_PROMPTS_ON,
	AP_SETTING_VOICE_PROMPTS_OPTION_COUNT
};

// 水印设置选项
enum {
	AP_SETTING_VIDEO_TIMESTAMP_OFF	= 	0,
	AP_SETTING_VIDEO_TIMESTAMP_ON,	//			1
	AP_SETTING_VIDEO_TIMESTAMP_OPTION_COUNT
};

// 演示模式设置选项
enum {
	AP_SETTING_DEMOMODE_OFF	=	0,
	AP_SETTING_DEMOMODE_ON,	//					1
	AP_SETTING_DEMOMODE_OPTION_COUNT
};

// 可录时间报警设置选项
enum {
	AP_SETTING_RECORDALARM_OFF	=	0,
	AP_SETTING_RECORDALARM_10M,	//				1	// 10分钟
	AP_SETTING_RECORDALARM_20M,	//				2
	AP_SETTING_RECORDALARM_40M,	//				3
	AP_SETTING_RECORDALARM_1H,		//				4	// 1小时
	AP_SETTING_RECORDALARM_2H,		//				5
	AP_SETTING_RECORDALARM_4H,		//				6
	AP_SETTING_RECORDALARM_6H,		//				7
	AP_SETTING_RECORDALARM_8H,		//				8
	AP_SETTING_RECORDALARM_10H,	//				9
	AP_SETTING_RECORDALARM_12H,	//				10
	AP_SETTING_RECORDALARM_OPTION_COUNT
};

// 录像延迟时间设置选项
enum {
	AP_SETTING_VIDEO_DELAY_0S	= 0,
	AP_SETTING_VIDEO_DELAY_3S,	//				1
	AP_SETTING_VIDEO_DELAY_5S,	//				2
	AP_SETTING_VIDEO_DELAY_10S,	//			3
	AP_SETTING_VIDEO_DELAY_15S,	//			4
	AP_SETTING_VIDEO_DELAY_OPTION_COUNT
};

// 录像分段时间设置选项
enum {
	AP_SETTING_RECORDTIME_1M = 0,	//1分钟
	AP_SETTING_RECORDTIME_3M = 1,	//3分钟
	AP_SETTING_RECORDTIME_5M,		//5分钟
	AP_SETTING_RECORDTIME_10M,		//10分钟
	AP_SETTING_RECORDTIME_OPTION_COUNT
};

// 碰撞灵敏度选项
enum {
	AP_SETTING_COLLISION_1G = 0,		// 1G加速度
	AP_SETTING_COLLISION_2G,
	AP_SETTING_COLLISION_3G,
	AP_SETTING_COLLISION_4G,
	AP_SETTING_COLLISION_OPTION_COUNT
};

// 紧急录像时间
enum {
	AP_SETTING_URGENTRECORDTIME_5M	=	0,		// 5分钟
	AP_SETTING_URGENTRECORDTIME_10M,	//				1	// 10分钟
	AP_SETTING_URGENTRECORDTIME_15M,	//				2	// 15分钟
	AP_SETTING_URGENTRECORDTIME_20M,	//				3	// 20分钟
	AP_SETTING_URGENTRECORDTIME_COUNT
};

// 日期标签设置选项
enum {
	AP_SETTING_DATE_LABEL_OFF = 0,
	AP_SETTING_DATE_LABEL_ON,
	AP_SETTING_DATE_LABEL_OPTION_COUNT
};

// 曝光补偿设置选项
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


// 停车监控设置选项
enum {
	AP_SETTING_PARK_MONITOR_OFF = 0,
	AP_SETTING_PARK_MONITOR_ON,
	AP_SETTING_PARK_MONITOR_OPTION_COUNT
};

//录影分辨率
enum {
	AP_SETTING_VIDEORESOLUTION_1080P_30 = 0,
	AP_SETTING_VIDEORESOLUTION_720P_30,
	AP_SETTING_VIDEORESOLUTION_720P_60,
	AP_SETTING_VIDEORESOLUTION_1080P_720P_30,	// 1080P 30帧
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

// 音量 10级
enum {
	AP_SETTING_SOUNDVOLUME_0	= 0,		// 关闭
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

// 白天晚上模式设置选项
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
	AP_SETTING_VOICEASSISTANT_BATTERY_ALARM = 0,		// 电池电量语音提示
	AP_SETTING_VOICEASSISTANT_CARD_STATUS,				// 数据卡状态语音提示
	AP_SETTING_VOICEASSISTANT_MIC_ONOFF,				// 录音开、关状态语音提示
	AP_SETTING_VOICEASSISTANT_REC_ONOFF,				// 录像状态语音提示 (录像开、关、录像模式)
	AP_SETTING_VOICEASSISTANT_URGENT_RECORD,			// 紧急录像启动、关闭语音提示
	AP_SETTING_VOICEASSISTANT_STORAGE_SPACE,			// 循环录制空间低
	AP_SETTING_VOICEASSISTANT_NAVIGATE_STATUS,		// 导航状态语音提示
	AP_SETTING_VOICEASSISTANT_DRIVE,						// 驾驶语音提示 (疲劳驾驶)
	AP_SETTING_VOICEASSISTANT_COUNT
};

// cvbs制式选择
enum {
	AP_SETTING_CVBS_NTSC = 0,
	AP_SETTING_CVBS_PAL,
	AP_SETTING_CVBS_COUNT
};

// 曝光补偿值设置 ( -2 -1 0 +1 +2 )
enum {
	AP_SETTING_EV_MINUS_2,			// -2
	AP_SETTING_EV_MINUS_1,			// -1
	AP_SETTING_EV_0,					// 0
	AP_SETTING_EV_1,					// 1
	AP_SETTING_EV_2,					// 2

	AP_SETTING_EV_COUNT
};
// 光源频率
enum {
	AP_SETTING_LIGHTFREQ_CLOSE,
	AP_SETTING_LIGHT_FREQ_50HZ,
	AP_SETTING_LIGHT_FREQ_60HZ,
	AP_SETTING_LIGHTFREQ_COUNT
};
// 锐化强度
enum {
	AP_SETTING_SHARPENLING_MINOR = 0,	// 锐化轻微
	AP_SETTING_SHARPENLING_SOFT,			// 锐化柔和
	AP_SETTING_SHARPENLING_STRONG,		// 锐化强烈
	AP_SETTING_SHARPENLING_COUNT
};

// 视频图像质量
enum {
	AP_SETTING_VIDEO_QUALITY_NORMAL = 0,	// 普通视频编码质量
	AP_SETTING_VIDEO_QUALITY_GOOD,			// 较好视频编码质量
	AP_SETTING_VIDEO_QUALITY_HIGH,			// 高清视频编码质量
	AP_SETTING_VIDEO_QUALITY_COUNT
};

enum{ 
	AP_SETTING_LCD_FLIP_NORMAL, 
	AP_SETTING_LCD_FLIP_MIRROR, 
};

//电源状态
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
//#define	AP_MENU_DATA_VERSION_01010101	0x01010101		// 当前版本
//#define	AP_MENU_DATA_VERSION_01010102	0x01010102		// 当前版本, 增加CVBS
//#define	AP_MENU_DATA_VERSION_01010103	0x01010103		// 当前版本, 增加曝光补偿,锐化,图像质量
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
	APPMENUITEM_TIME_STAMP,						// 时间水印
	APPMENUITEM_NAVI_STAMP,						// 导航水印
	APPMENUITEM_FLAG_STAMP,						// 标志水印
	APPMENUITEM_FLAG_STAMP_UPDATE,			// 标志水印修改标志 1 已被修改
	APPMENUITEM_RECORD_DELAY,
	APPMENUITEM_DEMO_MODE,
	APPMENUITEM_URGENT_RECORD_TIME,
	APPMENUITEM_VIDEO_RESOLUTION,				// 录像分辨率
	APPMENUITEM_DATE_LABEL,			// 日期标签
	APPMENU_IMAGE_QUALITY,          //图像质量
	APPMENUITEM_MIC_VOLUME,						// 录音音量设置
	APPMENUITEM_BELL_VOLUME,					// 铃声音量设置
    APPMENUITEM_DAY_NIGHT_MODE,                       // 白天晚上模式使能
	APPMENUITEM_LOCK_MODE,
	APPMENUITEM_PHOTO,
	APPMENUITEM_SYS_SETTING,
	APPMENUITEM_VIDEO_SWITCH,
	APPMENUITEM_PHOTOGRAPH,
	APPMENUITEM_RECORD_SWITCH,
	APPMENUITEM_RECORD_LIST,
	APPMENUITEM_MIC_SWITCH,
    APPMENUITEM_PARKMONITOR,

	// 语音助手类型定义，每一类语音提示可分别开启或关闭
	APPMENUITEM_VOICEASSISTANT_DRIVE,					// 驾驶语音提示 (疲劳驾驶)
	APPMENUITEM_VOICEASSISTANT_BATTERY_ALARM,			// 电池电量语音提示
	APPMENUITEM_VOICEASSISTANT_CARD_STATUS,			// 数据卡状态语音提示
	APPMENUITEM_VOICEASSISTANT_MIC_ONOFF,				// 录音开、关状态语音提示
	APPMENUITEM_VOICEASSISTANT_REC_ONOFF,				// 录像状态语音提示 (录像开、关、录像模式)
	APPMENUITEM_VOICEASSISTANT_URGENT_RECORD,			// 紧急录像启动、关闭语音提示
	APPMENUITEM_VOICEASSISTANT_STORAGE_SPACE,			// 循环录制空间低
	APPMENUITEM_VOICEASSISTANT_NAVIGATE_STATUS,		// 导航状态语音提示

	APPMENUITEM_CVBS_TYPE,									// CVBS NTSC/PAL制式设置
	APPMENUITEM_EV,											// 曝光补偿设置
    APPMENUITEM_LIGHT_FREQ,
	APPMENUITEM_SHARPENING,									// 锐化强度
	APPMENUITEM_VIDEO_QUALITY,								// 图像质量
	
    APPMENUITEM_PARKING_LINE,                   //parking line switch
    APPMENUITEM_PARKING_DELAY,                  //parking delay switch
    APPMENUITEM_MIRROR,                         //mirror setting
    
	APPMENUITEM_CH,//当前通道
	APPMENUITEM_REPLAY_CH,//回放通道
	APPMENUITEM_PRE_CH,//先前通道
	APPMENUITEM_CH1_DELAY,//通道1延时,单位s,1表示1s
	APPMENUITEM_CH2_DELAY,//通道2延时,单位s
	APPMENUITEM_CH3_DELAY,//通道3延时,单位s
	APPMENUITEM_CH4_DELAY,//通道4延时,单位s
	APPMENUITEM_POWER_STATE,//电源状态

	
	APPMENUITEM_COUNT,  //最大值,请勿在后面添加,要添加到前面   

};

typedef struct _tagAPPMENUDATA {
	u32_t		menu_id;				// ID标识
	u32_t		menu_version;			// 菜单设置格式版本
	u32_t		menu_size;				// 当前版本数据结构的字节大小
	u32_t		menu_checksum;			//	当前版本所有数据(除去自身)的字累加CheckSum + menu_checksum = 0xA5A5A5A5

	// 系统设置菜单项值
	u8_t		mic;						// 录音开启/关闭 1 开启 0 关闭
	u8_t		lcd;						// lcd关闭
	u8_t		key;						// 按键音
	u8_t		lang;						// 语言
	u8_t        day_night;					// 白天晚上模式使能  1 使能 0 禁止
	u8_t		video_lock;
	u8_t        photo;

	u8_t        sys_setting;
	u8_t        video_switch;
	u8_t        photograph;
	u8_t        record_switch;
	u8_t        record_list;
	u8_t        mic_switch;

	// 录像设置菜单项
	u8_t		voice_prompts;			// 语音提示 1 开启 0 关闭
	u8_t		recordtime_alarm;		// 可录时间报警语音
	u8_t		video_time_size;		// 录像分段时间
	
	// 碰撞灵敏度
	u8_t		collision_sensitivity;
	
	//停车监控
	u8_t       parkmonitor;
	
	// 时间水印
	u8_t		time_stamp;				// 时间戳水印开启/关闭	1 开启 0 关闭
	u8_t		flag_stamp;				// 标志水印 1 开启 0 关闭
	u8_t		flag_stamp_update;		// 标志水印修改标志 1 修改 0 未修改
	u8_t		navi_stamp;				// GPS/北斗导航水印 1 开启 0 关闭
	
	// 记录延时
	u8_t		record_delay;			// 记录延时
	
	// 紧急录像时间
	u8_t		urgent_record_time;	// 0 --> 5分钟
											// 1 --> 10分钟
											// 2 --> 15分钟
											// 3 --> 20分钟

	// 系统设置菜单
	u8_t		demo_mode;				// 无卡无时间设置演示模式 1 使能 0 禁止

	u8_t		video_resolution;		// 录像分辨率
	u8_t		mic_volume;				// MIC音量
	u8_t		bell_volume;			// 铃声音量

    u8_t		date_label;			// 日期标签

	u8_t		voiceassistant_drive;					// 驾驶语音提示 (疲劳驾驶)
	u8_t		voiceassistant_battery_alarm;			// 电池电量语音提示
	u8_t		voiceassistant_card_status;			// 数据卡状态语音提示
	u8_t		voiceassistant_mic_onoff;				// 录音开、关状态语音提示
	u8_t		voiceassistant_rec_onoff;				// 录像状态语音提示 (录像开、关、录像模式)
	u8_t		voiceassistant_urgent_record;			// 紧急录像启动、关闭语音提示
	u8_t		voiceassistant_storage_space;			// 循环录制空间低语音提示
	u8_t		voiceassistant_navigate_status;		// 导航状态语音提示

	u8_t		cvbs_type;							// cvbs制式选择
	u8_t		exposure_compensation;			// Ev 曝光补偿, -3,~ + 3
	u8_t        light_freq;
	u8_t		sharpening;							// 锐化强度值
	u8_t		video_quality;						// 视频编码质量(正常, 较好, 高)
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

	u8_t curch;//当前通道
	u8_t replay_ch;//回放通道
    u8_t prech;//当前通道
    u8_t ch1_delay;//通道1延时
    u8_t ch2_delay;//通道2延时
    u8_t ch3_delay;//通道3延时
    u8_t ch4_delay;//通道4延时
	u8_t power_state;//电源状态
	XMSYSTEMTIME buildtime;
	u8_t update_date_falg;//时间更新标志
	
	u32_t crc32_checksum;//必须最后一个
} APPMENUDATA;

extern APPMENUDATA AppMenuData;

// 系统提供的读取菜单数据的接口函数
int		XM_LoadMenuData (void *lpMenuData, int cbMenuData);
int		XM_SaveMenuData (void *lpMenuData, int cbMenuData);

// 对菜单项数据进行检查
int		AP_VerifyMenuData (APPMENUDATA *lpMenuData);
// 菜单项数据读取/写入
int		AP_LoadMenuData (APPMENUDATA *lpMenuData);
int		AP_SaveMenuData (APPMENUDATA *lpMenuData);

// 获取菜单项设置
unsigned int	AP_GetMenuItem (int menu_item);
// 设置菜单项
void				AP_SetMenuItem (int menu_item, unsigned int item_value);

// 返回“视频记录分段时间长度”选项的时间值(秒)
unsigned int	AP_GetVideoTimeSize (void);

// 返回“可录时间报警”选项的时间值 (秒)
unsigned int	AP_GetRecordTimeAlarm (void);

// 返回"延时记录"选项的时间值(秒)
unsigned int AP_GetRecordDelayTime (void);

// 恢复出厂初始设置
// 返回值
//   0    成功
//   < 0  失败
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



// 从Flash载入菜单设置
// 1 加载成功
// 0 加载失败
extern int	AP_LoadMenuData (APPMENUDATA *lpMenuData);


// 将菜单设置写入Flash
// 1 保存成功
//	0 保存失败
extern int	AP_SaveMenuData (APPMENUDATA *lpMenuData);



extern void APP_SaveMenuData(void);


extern int AP_RestoreMenuData (void);


extern unsigned int AP_GetMenuItem (int menu_item);



// 设置菜单选项
extern void AP_SetMenuItem (int menu_item, unsigned int item_value);



extern unsigned int AP_GetVideoTimeSize (void);


// 返回"延时记录"选项的时间值(秒)
extern unsigned int AP_GetRecordDelayTime (void);


//获取自动亮度开关
extern unsigned int AP_GetAutoBrightNess(void);

//默认为蓝色
extern unsigned int AP_GetBlue(void);

//摄像头个数
extern unsigned int AP_GetAHD_ChannelNum(void);

//摄像头 延时1
extern unsigned int AP_GetDelay_Camera1(void);

//摄像头 延时2
extern unsigned int AP_GetDelay_Camera2(void);

//摄像头 延时3
extern unsigned int AP_GetDelay_Camera3(void);

//摄像头 延时4
extern unsigned int AP_GetDelay_Camera4(void);


//开机背光亮度
extern unsigned int AP_GetBright_Switch(void);


extern VOID AP_SetBright_Switch(u8_t Bright_Switch);



//开关开机logo
extern unsigned int AP_GetLogo(void);


//开关红外竖线显示
extern unsigned int AP_GetMidle_RED_Line(void);


//摄像头 倒车标识1
extern unsigned int AP_GetGuide_Camera1(void);


//摄像头 倒车标识2
extern unsigned int AP_GetGuide_Camera2(void);


//摄像头 倒车标识3
extern unsigned int AP_GetGuide_Camera3(void);


extern unsigned int AP_GetCamera_Mot(void);


//摄像头 倒车标识4
extern unsigned int AP_GetGuide_Camera4(void);

//获取声音
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
