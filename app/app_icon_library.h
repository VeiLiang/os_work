//****************************************************************************
//
//	Copyright (C) 2010~2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_icon_library.c
//	  状态ICON的资源管理
//
//	Revision history
//
//		2012.09.01	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _APP_ICON_LIBRARY_H_
#define _APP_ICON_LIBRARY_H_

#include <xm_type.h>
#include "app_menuid.h"
#include "xm_app_menudata.h"

#if defined (__cplusplus)
	extern "C"{
#endif

// ICON类型定义
enum {
	AP_ICON_SIZE_16 = 0,		// 16点阵
	AP_ICON_SIZE_24,			// 24点阵
	AP_ICON_SIZE_32,			// 32点阵
	AP_ICON_SIZE_COUNT
};

enum {
	// ***** 桌面ICON资源 ******
	// BATTERY
	AP_ID_ICON_BATTERY_CHARGE = 0,	//1
	AP_ID_ICON_BATTERY_WORST,		//2
	AP_ID_ICON_BATTERY_BAD,			//3
	AP_ID_ICON_BATTERY_NORMAL,		//4
	AP_ID_ICON_BATTERY_GOOD,		//5
	// MIC
	AP_ID_ICON_MIC_OFF,				//6
	AP_ID_ICON_MIC_ON,				//7
	// VOLUME
	AP_ID_ICON_VOLUME_MUTE,			//8
	AP_ID_ICON_VOLUME_ON,			//9
	// USB
	AP_ID_ICON_USB_DISCONNECTED,	//10
	AP_ID_ICON_USB_CONNECTED,		//11
	AP_ID_ICON_USB_DISK,			//12
	// SD
	AP_ID_ICON_SDCARD,				//13
	AP_ID_ICON_SDCARD_PLUGIN,		//14
	// VIDEO REC
	AP_ID_ICON_VIDEO_REC_RED_DOT,	//15
	// DAY/NIGHT
	AP_ID_ICON_DAY,					//16
	AP_ID_ICON_NIGHT,				//17
	// GPS
	AP_ID_ICON_GPS_DISCONNECTED,	//18
	AP_ID_ICON_GPS_SEARCHING,		//19
	AP_ID_ICON_GPS_RECEIVING,		//20
    //PHOTO
    AP_ID_ICON_PHOTO,				//21
    AP_ID_ICON_PHOTO_PRESS,			//22
    //RETURN
    AP_ID_ICON_HOME,				//23
    //VIDEO_LOCK
    AP_ID_ICON_UNLOCK,				//24
    AP_ID_ICON_LOCK,				//25
    //SYS_SETTING
    AP_ID_ICON_SYS_SETTING,			//26
    AP_ID_ICON_SYS_SETTING_PRESS,	//27
    //VIDEOSWITCH
    AP_ID_ICON_VIDEO_SWITCH,		//28
    AP_ID_ICON_VIDEO_SWITCH_PRESS,	//29
    //PHOTOGRAPH
    AP_ID_ICON_PHOTOGRAPH,			//30
    AP_ID_ICON_PHOTOGRA_PRESS,		//31
    //RECORD
    AP_ID_ICON_RECORD,				//32
    AP_ID_ICON_RECORD_PRESS,		//33
    //RECORDLOCK
    AP_ID_ICON_RECORD_LOCK,			//34
    AP_ID_ICON_RECORD_LOCK_PRESS,	//35
    //RECORDLIST
    AP_ID_ICON_RECORD_LIST,			//36
    AP_ID_ICON_RECORD_LIST_PRESS,	//37
    
    //标识
    AP_ID_ICON_BIAOSHI,				//38
    //摄像头
    AP_ID_ICON_CAMERA1,				//39
    AP_ID_ICON_CAMERA2,				//40
    AP_ID_ICON_CAMERA3,				//41
    AP_ID_ICON_CAMERA4,				//42
    AP_ID_ICON_V_LINE,				//43
    AP_ID_ICON_H_LINE,				//44
		
    AP_ID_ICON_NOSIGNED,			//45
    AP_ID_ICON_NOSIGNED_C,			//46

	AP_ID_ICON_MODE,				//47,	录像拍照模式图标
	AP_ID_ICON_TAKEPHOTO,			//48
	AP_ID_ICON_COUNT				//49
};

// 根据ICON ID获取对应的ROM资源
APPROMRES*	AP_AppGetIconResource (UINT IconType, UINT IconID);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _APP_ICON_LIBRARY_H_