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

// MIC设置选项
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

// LCD关闭时间设置选项
enum {
	AP_SETTING_LCD_NEVERCLOSE	=	0,
	AP_SETTING_LCD_20S,
	AP_SETTING_LCD_40S,
	AP_SETTING_LCD_60S,
	AP_SETTING_LCD_120S,	
	AP_SETTING_LCD_300S,
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
	AP_SETTING_LANG_OPTION_COUNT	//	11
};

#define	AP_LANG_COUNT		AP_SETTING_LANG_OPTION_COUNT

// 语音提示设置选项
enum {
	AP_SETTING_VOICE_PROMPTS_OFF = 0,
	AP_SETTING_VOICE_PROMPTS_ON,
	AP_SETTING_VOICE_PROMPTS_OPTION_COUNT
};
// 停车监控选项
enum {
	AP_VIDEO_SETTING_MONITOR_OFF = 0,
	AP_VIDEO_SETTING_MONITOR_ON,
	AP_VIDEO_SETTING_MONITOR_OPTION_COUNT
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
	AP_SETTING_RECORDTIME_1M = 0,		// 1分钟
	AP_SETTING_RECORDTIME_2M,	//				1		// 2分钟
	AP_SETTING_RECORDTIME_3M,	//				2		// 3分钟
	AP_SETTING_RECORDTIME_5M,	//				3		// 5分钟
	AP_SETTING_RECORDTIME_10M,	//				4		// 10分钟
	AP_SETTING_RECORDTIME_15M,	//				5		// 15分钟
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

// 录像分辨率
enum {
	AP_SETTING_VIDEORESOLUTION_1080P_30 = 0,	// 1080P 30帧
	AP_SETTING_VIDEORESOLUTION_1080P_60,		// 1080P运动 60帧
	AP_SETTING_VIDEORESOLUTION_720P_30,
	AP_SETTING_VIDEORESOLUTION_720P_60,
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
	AP_SETTING_DAYNIGHT_OFF	=	0,
	AP_SETTING_DAYNIGHT_ON,	//					1
	AP_SETTING_DAYNIGHT_OPTION_COUNT
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

#define	AP_MENU_DATA_CHECKSUM			0xA5A5A5A5
#define	AP_MENU_DATA_ID					0x554E454D		// "MENU"
#define	AP_MENU_DATA_VERSION_01010101	0x01010101		// 当前版本

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
	APPMENUITEM_MIC_VOLUME,						// 录音音量设置
	APPMENUITEM_BELL_VOLUME,					// 铃声音量设置

	APPMENUITEM_DAY_NIGHT_MODE,				// 白天晚上模式使能

	// 语音助手类型定义，每一类语音提示可分别开启或关闭
	APPMENUITEM_VOICEASSISTANT_DRIVE,					// 驾驶语音提示 (疲劳驾驶)
	APPMENUITEM_VOICEASSISTANT_BATTERY_ALARM,			// 电池电量语音提示
	APPMENUITEM_VOICEASSISTANT_CARD_STATUS,			// 数据卡状态语音提示
	APPMENUITEM_VOICEASSISTANT_MIC_ONOFF,				// 录音开、关状态语音提示
	APPMENUITEM_VOICEASSISTANT_REC_ONOFF,				// 录像状态语音提示 (录像开、关、录像模式)
	APPMENUITEM_VOICEASSISTANT_URGENT_RECORD,			// 紧急录像启动、关闭语音提示
	APPMENUITEM_VOICEASSISTANT_STORAGE_SPACE,			// 循环录制空间低
	APPMENUITEM_VOICEASSISTANT_NAVIGATE_STATUS,		// 导航状态语音提示

	APPMENUITEM_COUNT
};

typedef struct _tagAPPMENUDATA {
	u32_t		menu_id;					// ID标识
	u32_t		menu_version;			// 菜单设置格式版本
	u32_t		menu_size;				// 当前版本数据结构的字节大小
	u32_t		menu_checksum;			//	当前版本所有数据(除去自身)的字累加CheckSum + menu_checksum = 0xA5A5A5A5

	// 系统设置菜单项值
	u8_t		mic;						// 录音开启/关闭 1 开启 0 关闭
	u8_t		lcd;						// lcd关闭
	u8_t		key;						// 按键音
	u8_t		lang;						// 语言
	u8_t		video_lock;		// 白天晚上模式使能  1 使能 0 禁止

	// 录像设置菜单项
	u8_t		voice_prompts;			// 语音提示 1 开启 0 关闭
	u8_t		recordtime_alarm;		// 可录时间报警语音
	u8_t		video_time_size;		// 录像分段时间
	// 碰撞灵敏度
	u8_t		collision_sensitivity;		
	//停车监控
	u8_t       monitor;
	// 时间水印
	u8_t		time_stamp;				// 时间戳水印开启/关闭	1 开启 0 关闭
	u8_t		flag_stamp;				// 标志水印 1 开启 0 关闭
	u8_t		flag_stamp_update;	// 标志水印修改标志 1 修改 0 未修改
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

	u8_t		voiceassistant_drive;					// 驾驶语音提示 (疲劳驾驶)
	u8_t		voiceassistant_battery_alarm;			// 电池电量语音提示
	u8_t		voiceassistant_card_status;			// 数据卡状态语音提示
	u8_t		voiceassistant_mic_onoff;				// 录音开、关状态语音提示
	u8_t		voiceassistant_rec_onoff;				// 录像状态语音提示 (录像开、关、录像模式)
	u8_t		voiceassistant_urgent_record;			// 紧急录像启动、关闭语音提示
	u8_t		voiceassistant_storage_space;			// 循环录制空间低语音提示
	u8_t		voiceassistant_navigate_status;		// 导航状态语音提示

	//
	u8_t curch;//当前通道
	
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





#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_APP_MENUDATA_H_
