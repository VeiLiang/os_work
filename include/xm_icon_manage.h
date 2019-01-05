//****************************************************************************
//
//	Copyright (C) 2010~2014 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_icon_manage.h
//	  状态ICON的管理、显示、隐藏
//
//	Revision history
//
//		2012.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
/*
*/
#ifndef _XM_ICON_MANAGE_H_
#define _XM_ICON_MANAGE_H_

#include "xm_osd_framebuffer.h"
#include "xm_app_menudata.h"

#if defined (__cplusplus)
	extern "C"{
#endif


enum {	
	AP_ICON_REC_OFF	=	0,		// 录像关闭
	AP_ICON_REC_ON,				// 录像开启
	AP_ICON_REC_COUNT
};

enum
{
	AP_ICON_DISPLAY_OFF = 0,
	AP_ICON_DISPLAY_ON,
};


// ICON类型定义
enum {
	AP_ICON_MIC,						// MIC标志(开启、关闭)
											//		AP_SETTING_MIC_OFF
											//		AP_SETTING_MIC_ON
	
	AP_ICON_VOICE,						// 语音助手开关标志(开启、关闭)
											//		AP_SETTING_VOICE_PROMPTS_OFF
											//		AP_SETTING_VOICE_PROMPTS_ON
	
	AP_ICON_REC_RED_DOT,				//录像红点
											
											
	
	AP_ICON_VIDEORESOLUTION,		// 录像类型(1080P 60、1080P 30、720P 60、720P 30等)
											//		AP_SETTING_VIDEORESOLUTION_1080P_30
											//		AP_SETTING_VIDEORESOLUTION_1080P_60
											//		AP_SETTING_VIDEORESOLUTION_720P_30
											//		AP_SETTING_VIDEORESOLUTION_720P_60
	
	AP_ICON_CARD,						// 卡状态 (卡拔出、卡损坏、卡写保护、卡无法识别、卡正常读写)
											//		DEVCAP_SDCARDSTATE_UNPLUG
											//		DEVCAP_SDCARDSTATE_WRITEPROTECT
											//		DEVCAP_SDCARDSTATE_INSERT
											//		DEVCAP_SDCARDSTATE_VERIFYFAILED
	
	AP_ICON_USB,						// USB连接断开、USB充电、U盘
											//		DEVCAP_USB_DISCONNECT
											//		DEVCAP_USB_CONNECT_CHARGE
											//		DEVCAP_USB_CONNECT_UDISK
											//		DEVCAP_USB_CONNECT_CAMERA

	AP_ICON_MAIN_BATTERY,			// 主电池状态
	AP_ICON_BACKUP_BATTERY,			// 备份电池状态
											//		DEVCAP_BATTERYVOLTAGE_WORST
											//		DEVCAP_BATTERYVOLTAGE_BAD
											//		DEVCAP_BATTERYVOLTAGE_NORMAL
											//		DEVCAP_BATTERYVOLTAGE_BEST
	

	AP_ICON_GPS,						//  GPS状态
										// 未连接
										// 已连接
										// 已定位
    AP_ICON_PHOTO,	
    AP_ICON_HOME ,
    AP_ICON_LOCK ,
    AP_ICON_DAY_NIGHT_MODE,
    AP_ICON_SYS_SETTING,
    AP_ICON_VIDEO_SWITCH,
    AP_ICON_PHOTOGRAPH,
    AP_ICON_RECORD_SWITCH,
    AP_ICON_RECORD_LOCK,
    AP_ICON_RECORD_LIST,
    AP_ICON_V_LINE,
    AP_ICON_H_LINE,
    AP_ICON_BIAOZHI,
    AP_ICON_CH_AHD1,
    AP_ICON_CH_AHD2,
    AP_ICON_NO_SIGNED,
    AP_ICON_MODE,
	AP_ICON_TAKEPHOTO,
	
	AP_ICON_COUNT
};

// ICON管理初始化
void XM_IconInit (void);

// ICON管理关闭
void XM_IconExit (void);

void XM_VideoIconInit (void);

void XM_VideoIconExit (void);




// 设置ICON的状态
void XM_IconSetState (unsigned int dwIcon, unsigned int dwIconState);

// 读取ICON的状态
unsigned int XM_IconGetState (unsigned int dwIcon);

// 将可视的ICON显示在视频层帧缓冲区
void XM_IconDisplay (xm_osd_framebuffer_t framebuffer);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif	// #ifndef _XM_ICON_MANAGE_H_




