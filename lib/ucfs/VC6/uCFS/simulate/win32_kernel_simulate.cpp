//****************************************************************************
//
//	Copyright (C) 2010~2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: win32_kernel_simulate.c
//	  Win32 系统内核基本服务接口模拟
//
//	Revision history
//
//		2010.08.18	ZhuoYongHong Initial version
//
//****************************************************************************#include <io.h>
#include <stdio.h>
#include <direct.h>
#include <afx.h>
#include <assert.h>
#include <winbase.h>

#pragma warning(disable:4100) // 不显示4100号警告信息 (unreferenced formal parameter)
#pragma warning(disable : 4996)	// warning C4996: 'fopen': This function or variable may be unsafe.


typedef unsigned char		XMBOOL;		// BOOL类型

#include "..\..\include\xm_proj_define.h"
#include "..\..\include\xmtype.h"
#include "..\..\include\xmdev.h"
#include "..\..\include\xmlang.h"
#include "..\..\include\xmbase.h"
#include "..\..\include\xmuser.h"
#include "..\..\include\xmkey.h"

#include "..\..\include\hw_osd_layer.h"



extern "C" {

typedef unsigned char		XMBOOL;		// BOOL类型

volatile unsigned int hw_bSDCardState = SYSTEM_EVENT_CARD_INSERT;			// SD卡已插入
volatile static BYTE bVideoContrastLevel = 7;
volatile static BYTE bBackLight = 0;

volatile unsigned int hw_bSensorConnect;						// Sensor 0(摄像头0)识别(连接)状态												

volatile unsigned int hw_bLcdPlugIn;							// LCD屏连接(有屏产品)或未接(无屏产品)
volatile unsigned int hw_bAvoutPlugIn;							// AVOUT插入/拔出模拟
volatile unsigned int hw_uAvoutType;							// AVOUT输出类型
volatile unsigned int hw_bHdmiPlugIn;							// HDMI插入/拔出状态
volatile unsigned int hw_uHdmiType;								// HDMI类型(产品定义)
volatile unsigned int hw_bTimeSetting = 0;					// 系统时间未设置
volatile unsigned int hw_bBackupBatteryVoltageLevel;		// 备份电池状态
volatile unsigned int hw_bMainBatteryVoltageLevel;			// 主电池状态
volatile unsigned int hw_bUsbConnectState;					// USB连接状态
volatile unsigned int hw_bGpsbdConnectState;					// GPSBD连接状态
volatile unsigned int hw_bVideoRecState;						// 录像状态

volatile unsigned int hw_bDayNightState = 0;					//	0 强制黑夜模拟
																// 1 强制白天模拟
																//	2 自动选择 (根据时间自动选择白天、晚上模式)	


static WORD wOemCodePage	= FML_LANG_CHINESE_SIMPLIFIED;	// 系统本地语言，文件系统使用
static WORD wResCodePage	= FML_LANG_CHINESE_SIMPLIFIED;	// UI(资源)界面语言，界面显示使用


// 从INI文件加载模拟器的系统初始状态
void LoadInitialSetting (void)
{
	// 卡状态
	char str[128];
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"WITHDRAW",
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "DETECT") == 0)
		hw_bSDCardState = SYSTEM_EVENT_CARD_DETECT;						// 卡检测中
	else if(strcmp (str, "UNPLUG") == 0)
		hw_bSDCardState = SYSTEM_EVENT_CARD_UNPLUG;					// 卡拔出
	else if(strcmp (str, "WRITE_PROTECT") == 0)
		hw_bSDCardState = SYSTEM_EVENT_CARD_INSERT_WRITE_PROTECT;	// 卡写保护
	else if(strcmp (str, "INSERT") == 0)
		hw_bSDCardState = SYSTEM_EVENT_CARD_INSERT;						// SD卡插入(读写允许)
	else if(strcmp (str, "FSERROR") == 0)
		hw_bSDCardState = SYSTEM_EVENT_CARD_FS_ERROR;						// SD卡插入(文件系统访问异常, 无法识别的卡等)
	else if(strcmp (str, "VERIFY_ERROR") == 0)
		hw_bSDCardState = SYSTEM_EVENT_CARD_VERIFY_ERROR;				// SD卡读写校验检查失败，SD卡已损害
	else if(strcmp (str, "INVALID") == 0)
		hw_bSDCardState = SYSTEM_EVENT_CARD_INVALID;						// SD卡无法识别
	else
		hw_bSDCardState = SYSTEM_EVENT_CARD_INSERT;						// SD卡插入(读写允许)

	// RTC时间设置
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"RTC",
			"时间未设置",
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "时间未设置") == 0)
		hw_bTimeSetting = 0;					// 时间未设置
	else if(strcmp (str, "时间已设置") == 0)
		hw_bTimeSetting = 1;					// 时间已设置

	// 备份电池
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"BACKUPBATTERY",
			"2",			// 电池状况正常
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "0") == 0)
		hw_bBackupBatteryVoltageLevel = 0;					// 较差
	else if(strcmp (str, "1") == 0)
		hw_bBackupBatteryVoltageLevel = 1;					// 差
	else if(strcmp (str, "2") == 0)
		hw_bBackupBatteryVoltageLevel = 2;					// 正常
	else
		hw_bBackupBatteryVoltageLevel = 3;					// 好

	// 主电池
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"MAINBATTERY",
			"2",			// 电池状况正常
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "0") == 0)
		hw_bMainBatteryVoltageLevel = 0;					// 较差
	else if(strcmp (str, "1") == 0)
		hw_bMainBatteryVoltageLevel = 1;					// 差
	else if(strcmp (str, "2") == 0)
		hw_bMainBatteryVoltageLevel = 2;					// 正常
	else
		hw_bMainBatteryVoltageLevel = 3;					// 好

	// AVOUT 插入或拔出
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT",
			"PLUGIN",			// 插入
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "PLUGIN") == 0)
		hw_bAvoutPlugIn = 1;					// AVOUT插入
	else if(strcmp (str, "PLUGOUT") == 0)
		hw_bAvoutPlugIn = 0;					// AVOUT拔出
	else 
		hw_bAvoutPlugIn = 0;					// AVOUT拔出

	// AVOUT 类型
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT_TYPE",
			"CVBS_NTSC",			
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "CVBS_NTSC") == 0)
		hw_uAvoutType = XM_LCDC_TYPE_CVBS_NTSC;
	else if(strcmp (str, "CVBS_PAL") == 0)
		hw_uAvoutType = XM_LCDC_TYPE_CVBS_PAL;
	else if(strcmp (str, "YPbPr_480i") == 0)
		hw_uAvoutType = XM_LCDC_TYPE_YPbPr_480i;
	else if(strcmp (str, "YPbPr_480p") == 0)
		hw_uAvoutType = XM_LCDC_TYPE_YPbPr_480p;
	else if(strcmp (str, "YPbPr_576i") == 0)
		hw_uAvoutType = XM_LCDC_TYPE_YPbPr_576i;
	else if(strcmp (str, "YPbPr_576p") == 0)
		hw_uAvoutType = XM_LCDC_TYPE_YPbPr_576p;
	else 
		hw_bAvoutPlugIn = 0;					// AVOUT拔出


	// HDMI 插入或拔出
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI",
			"PLUGIN",			// 插入
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "PLUGIN") == 0)
		hw_bHdmiPlugIn = 1;					// HDMI 插入
	else if(strcmp (str, "PLUGOUT") == 0)
		hw_bHdmiPlugIn = 0;					// HDMI 拔出
	else 
		hw_bHdmiPlugIn = 0;					// HDMI 拔出

	// HDMI 类型
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI_TYPE",
			"HDMI_1080p",			
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "HDMI_1080p") == 0)
		hw_uHdmiType = XM_LCDC_TYPE_HDMI_1080p;
	else if(strcmp (str, "HDMI_1080i") == 0)
		hw_uHdmiType = XM_LCDC_TYPE_HDMI_1080i;
	else if(strcmp (str, "HDMI_720p") == 0)
		hw_uHdmiType = XM_LCDC_TYPE_HDMI_720p;
	else if(strcmp (str, "HDMI_720i") == 0)
		hw_uHdmiType = XM_LCDC_TYPE_HDMI_720i;
	else
		hw_uHdmiType = XM_LCDC_TYPE_HDMI_720p;

	// LCD显示屏 连接或未接 (模拟有屏/无屏)
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"LCD",
			"CONNECT",			// 插入
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "CONNECT") == 0)
		hw_bLcdPlugIn = 1;					// LCD屏连接
	else if(strcmp (str, "DISCONNECT") == 0)
		hw_bLcdPlugIn = 0;					// 无屏产品
	else 
		hw_bLcdPlugIn = 0;					// 无屏产品

	// USB连接状态 
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"USB",
			"DISCONNECT",			// 未连接
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "CHARGE") == 0)
		hw_bUsbConnectState = DEVCAP_USB_CONNECT_CHARGE;			// USB已插入，作为充电线使用
	else if(strcmp (str, "UDISK") == 0)
		hw_bUsbConnectState = DEVCAP_USB_CONNECT_UDISK;				// USB已插入，主机作为其他设备的U盘使用
	else if(strcmp (str, "CAMERA") == 0)
		hw_bUsbConnectState = DEVCAP_USB_CONNECT_CAMERA;			// USB已插入，主机作为其他设备的摄像头
	else if(strcmp (str, "DISCONNECT") == 0)
		hw_bUsbConnectState = DEVCAP_USB_DISCONNECT;					// USB已断开
	else
		hw_bUsbConnectState = DEVCAP_USB_DISCONNECT;					// USB已断开

	// Sensor 0 连接或无法识别
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SENSOR",
			"CONNECT",			// 插入
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "CONNECT") == 0)
		hw_bSensorConnect = DEVCAP_SENSOR_CONNECT;					// Sensor已识别，成功连接
	else if(strcmp (str, "DISCONNECT") == 0)
		hw_bSensorConnect = DEVCAP_SENSOR_DISCONNECT;					// Sensor无法识别，未能连接
	else 
		hw_bSensorConnect = DEVCAP_SENSOR_DISCONNECT;					// Sensor无法识别，未能连接

	// GPSBD 连接、断开
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"GPSBD",
			"DISCONNECT",			// GPSBD断开
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "DISCONNECT") == 0)
		hw_bGpsbdConnectState = DEVCAP_GPSBD_DISCONNECT;			// GPSBD已断开
	else if(strcmp (str, "ANTENNA_OPEN") == 0)
		hw_bGpsbdConnectState = DEVCAP_GPSBD_CONNECT_ANTENNA_OPEN;			// GPSBD天线未连接
	else if(strcmp (str, "ANTENNA_SHORT") == 0)
		hw_bGpsbdConnectState = DEVCAP_GPSBD_CONNECT_ANTENNA_SHORT;		// GPSBD天线短路
	else if(strcmp (str, "ANTENNA_OK") == 0)
		hw_bGpsbdConnectState = DEVCAP_GPSBD_CONNECT_ANTENNA_OK;			// GPSBD天线正常
	else if(strcmp (str, "LOCATE_OK") == 0)
		hw_bGpsbdConnectState = DEVCAP_GPSBD_CONNECT_LOCATE_OK;				// GPSBD已定位
	else
		hw_bGpsbdConnectState = DEVCAP_GPSBD_DISCONNECT;			// GPSBD已断开

	// 白天、晚上模式选择
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"DAYTIME",
			"AUTO",			// 缺省自动选择
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "SUN") == 0)
		hw_bDayNightState = 1;			// 强制白天模式
	else if(strcmp (str, "MOON") == 0)
		hw_bDayNightState = 0;			// 强制晚上模式
	else //if(strcmp (str, "AUTO") == 0)
		hw_bDayNightState = 2;			// 自动选择  (根据时间自动选择白天、晚上模式)	
}

// 保存模拟器的系统初始状态到INI文件
void SaveInitialSetting (void)
{
	if(hw_bSDCardState == SYSTEM_EVENT_CARD_DETECT)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"DETECT",
			".\\ark1960.ini"
			);
	else if(hw_bSDCardState == SYSTEM_EVENT_CARD_UNPLUG)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"UNPLUG",
			".\\ark1960.ini"
			);
	else if(hw_bSDCardState == SYSTEM_EVENT_CARD_INSERT_WRITE_PROTECT)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"WRITE_PROTECT",
			".\\ark1960.ini"
			);
	else if(hw_bSDCardState == SYSTEM_EVENT_CARD_INSERT)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"INSERT",
			".\\ark1960.ini"
			);
	else if(hw_bSDCardState == SYSTEM_EVENT_CARD_FS_ERROR)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"FSERROR",
			".\\ark1960.ini"
			);
	else if(hw_bSDCardState == SYSTEM_EVENT_CARD_VERIFY_ERROR)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"VERIFY_ERROR",
			".\\ark1960.ini"
			);
	else if(hw_bSDCardState == SYSTEM_EVENT_CARD_INVALID)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"INVALID",
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"可读写",
			".\\ark1960.ini"
			);

	if(hw_bTimeSetting == 0)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"RTC",
			"时间未设置",
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"RTC",
			"时间已设置",
			".\\ark1960.ini"
			);

	// 备份电池
	if(hw_bBackupBatteryVoltageLevel == 0)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"BACKUPBATTERY",
			"0",			// 电池状况正常
			".\\ark1960.ini"
			);
	else if(hw_bBackupBatteryVoltageLevel == 1)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"BACKUPBATTERY",
			"1",			// 电池状况正常
			".\\ark1960.ini"
			);
	else if(hw_bBackupBatteryVoltageLevel == 2)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"BACKUPBATTERY",
			"2",			// 电池状况正常
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"BACKUPBATTERY",
			"3",			// 电池状况正常
			".\\ark1960.ini"
			);

	// 主电池
	if(hw_bMainBatteryVoltageLevel == 0)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"MAINBATTERY",
			"0",			// 电池状况正常
			".\\ark1960.ini"
			);
	else if(hw_bMainBatteryVoltageLevel == 1)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"MAINBATTERY",
			"1",			// 电池状况正常
			".\\ark1960.ini"
			);
	else if(hw_bMainBatteryVoltageLevel == 2)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"MAINBATTERY",
			"2",			// 电池状况正常
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"MAINBATTERY",
			"3",			// 电池状况正常
			".\\ark1960.ini"
			);


	// AVOUT类型
	if(hw_uAvoutType == XM_LCDC_TYPE_CVBS_NTSC)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT_TYPE",
			"CVBS_NTSC",			// CVBS_NTSC
			".\\ark1960.ini"
			);
	else if(hw_uAvoutType == XM_LCDC_TYPE_CVBS_PAL)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT_TYPE",
			"CVBS_PAL",			// CVBS_PAL
			".\\ark1960.ini"
			);
	else if(hw_uAvoutType == XM_LCDC_TYPE_YPbPr_480i)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT_TYPE",
			"YPbPr_480i",			// YPbPr_480i
			".\\ark1960.ini"
			);
	else if(hw_uAvoutType == XM_LCDC_TYPE_YPbPr_480p)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT_TYPE",
			"YPbPr_480p",			// YPbPr_480p
			".\\ark1960.ini"
			);
	else if(hw_uAvoutType == XM_LCDC_TYPE_YPbPr_576i)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT_TYPE",
			"YPbPr_576i",			// YPbPr_576i
			".\\ark1960.ini"
			);
	else if(hw_uAvoutType == XM_LCDC_TYPE_YPbPr_576p)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT_TYPE",
			"YPbPr_576p",			// YPbPr_576p
			".\\ark1960.ini"
			);

	if(hw_bAvoutPlugIn == 1)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT",
			"PLUGIN",			// AVOUT插入
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT",
			"PLUGOUT",			// AVOUT拔出
			".\\ark1960.ini"
			);


	// HDMI插拔状态
	if(hw_bHdmiPlugIn == 1)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI",
			"PLUGIN",			// HDMI 插入
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI",
			"PLUGOUT",			// HDMI 拔出
			".\\ark1960.ini"
			);

	// HDMI接口类型
	if(hw_uHdmiType == XM_LCDC_TYPE_HDMI_1080p)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI_TYPE",
			"HDMI_1080p",			// HDMI_1080p
			".\\ark1960.ini"
			);
	else if(hw_uHdmiType == XM_LCDC_TYPE_HDMI_1080i)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI_TYPE",
			"HDMI_1080i",			// HDMI_1080i
			".\\ark1960.ini"
			);
	else if(hw_uHdmiType == XM_LCDC_TYPE_HDMI_720p)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI_TYPE",
			"HDMI_720p",			// HDMI_720p
			".\\ark1960.ini"
			);
	else if(hw_uHdmiType == XM_LCDC_TYPE_HDMI_720i)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI_TYPE",
			"HDMI_720i",			// HDMI_720i
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI_TYPE",
			"HDMI_720p",			// HDMI_720p
			".\\ark1960.ini"
			);


	// LCD屏状态
	if(hw_bLcdPlugIn == 1)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"LCD",
			"CONNECT",			// 有LCD屏
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"LCD",
			"DISCONNECT",		// 无LCD屏
			".\\ark1960.ini"
			);

	// USB连接状态 
	if(hw_bUsbConnectState == DEVCAP_USB_CONNECT_CHARGE)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"USB",
			"CHARGE",			// USB已插入，作为充电线使用
			".\\ark1960.ini"
			);
	else if(hw_bUsbConnectState == DEVCAP_USB_CONNECT_UDISK)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"USB",
			"UDISK",			// USB已插入，主机作为其他设备的U盘使用
			".\\ark1960.ini"
			);
	else if(hw_bUsbConnectState == DEVCAP_USB_CONNECT_CAMERA)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"USB",
			"CAMERA",			// USB已插入，主机作为其他设备的摄像头
			".\\ark1960.ini"
			);
	else if(hw_bUsbConnectState == DEVCAP_USB_DISCONNECT)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"USB",
			"DISCONNECT",			// USB已断开
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"USB",
			"DISCONNECT",			// USB已断开
			".\\ark1960.ini"
			);



	// Sensor 0(摄像头0)状态
	if(hw_bSensorConnect == DEVCAP_SENSOR_CONNECT)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SENSOR",
			"CONNECT",			// Sensor已识别，成功连接
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SENSOR",
			"DISCONNECT",		// Sensor无法识别，未能连接
			".\\ark1960.ini"
			);

	if(hw_bGpsbdConnectState == DEVCAP_GPSBD_DISCONNECT)
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"GPSBD",
			"DISCONNECT",		
			".\\ark1960.ini"
			);
	}
	else if(hw_bGpsbdConnectState == DEVCAP_GPSBD_CONNECT_ANTENNA_OPEN)
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"GPSBD",
			"ANTENNA_OPEN",		
			".\\ark1960.ini"
			);
	}
	else if(hw_bGpsbdConnectState == DEVCAP_GPSBD_CONNECT_ANTENNA_SHORT)
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"GPSBD",
			"ANTENNA_SHORT",		
			".\\ark1960.ini"
			);
	}
	else if(hw_bGpsbdConnectState == DEVCAP_GPSBD_CONNECT_ANTENNA_OK)
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"GPSBD",
			"ANTENNA_OK",		
			".\\ark1960.ini"
			);
	}
	else if(hw_bGpsbdConnectState == DEVCAP_GPSBD_CONNECT_LOCATE_OK)
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"GPSBD",
			"LOCATE_OK",		
			".\\ark1960.ini"
			);
	}
	else
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"GPSBD",
			"DISCONNECT",		
			".\\ark1960.ini"
			);
	}

	// 白天、晚上模式
	if(hw_bDayNightState == 0)		// 0 强制黑夜模拟
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"DAYTIME",
			"MOON",		
			".\\ark1960.ini"
			);
	}
	else if(hw_bDayNightState == 1)		// 1 强制白天模拟
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"DAYTIME",
			"SUN",		
			".\\ark1960.ini"
			);
	}
	else
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"DAYTIME",
			"AUTO",		
			".\\ark1960.ini"
			);

	}
}

// 获取系统语言代码页
WORD	XM_GetSystemCodePage		(void)
{
	return wOemCodePage;
}

// 获取资源语言代码页
WORD	XM_GetResourceCodePage	(void)
{
	return wResCodePage;
}

// 设置资源语言代码页，返回值为原来的代码页
WORD	XM_SetResourceCodePage	(WORD wCodePage)
{
	WORD wCode = wResCodePage;
	wResCodePage = wCodePage;
	return wCode;
}


XMBOOL	XM_GetLocalTime	(XMSYSTEMTIME* pSystemTime)
{
	SYSTEMTIME time;
	GetLocalTime (&time);

	pSystemTime->wYear = time.wYear;
	pSystemTime->bDay = (BYTE)time.wDay;
	pSystemTime->bMonth = (BYTE)time.wMonth;
	pSystemTime->bDayOfWeek = (BYTE)time.wDayOfWeek;
	pSystemTime->bHour = (BYTE)time.wHour;
	pSystemTime->bMinute = (BYTE)time.wMinute;
	pSystemTime->bSecond = (BYTE)time.wSecond;
	return (XMBOOL)hw_bTimeSetting;
}

VOID	XM_SetLocalTime	(const XMSYSTEMTIME *pSystemTime)
{
	hw_bTimeSetting = 1;
}


// 设置系统参数
XMBOOL	XM_SetFmlDeviceCap  (BYTE bDevCapIndex, DWORD dwValue)
{
	switch (bDevCapIndex)
	{

		case DEVCAP_VIDEOCONTRASTLEVEL:
			// 设置或读取对比度 (0 ~ 15级)
			if(dwValue >= 15)
				return 0;
			bVideoContrastLevel = (BYTE)dwValue;
			break;

		case DEVCAP_USB:
			return 0;

		case DEVCAP_MAINBATTERYVOLTAGE:
			// 不允许设置电压，只允许读取
			return 0;

		case DEVCAP_BACKLIGHT:
			bBackLight = (BYTE)dwValue;
			break;

		case DEVCAP_AVOUT:
			hw_bAvoutPlugIn = (BYTE)dwValue;
			break;

		case DEVCAP_HDMI:
			hw_bHdmiPlugIn = (BYTE)dwValue;
			break;

		case DEVCAP_VIDEO_REC:
			if(dwValue > DEVCAP_VIDEO_REC_START)
				return 0;
			hw_bVideoRecState = (BYTE)dwValue;
			break;

		default:
			return 0;
	}
	return 1;
}

// 获取系统参数
DWORD  	XM_GetFmlDeviceCap  (BYTE bDevCapIndex)
{
	switch (bDevCapIndex)
	{

		case DEVCAP_VIDEOCONTRASTLEVEL:
			return (WORD)bVideoContrastLevel;

		case DEVCAP_MAINBATTERYVOLTAGE:
			return hw_bMainBatteryVoltageLevel;

		case DEVCAP_USB:
			return hw_bUsbConnectState;

		case DEVCAP_BACKLIGHT:
			return bBackLight;

		case DEVCAP_SDCARDSTATE:	// SD卡状态检测
			return hw_bSDCardState;

		case DEVCAP_TIMESETTING:	// 系统时间是否已设置
			return hw_bTimeSetting;

		case DEVCAP_BACKUPBATTERYVOLTAGE:	// 备份电池
			return hw_bBackupBatteryVoltageLevel;

		case DEVCAP_AVOUT:			// AVOUT插拔状态
			return hw_bAvoutPlugIn;			// 1 插入 0 拔出

		case DEVCAP_AVOUT_TYPE:			// AVOUT类型
			return hw_uAvoutType;			

		case DEVCAP_HDMI:				// HDMI插拔状态
			return hw_bHdmiPlugIn;			// 1 插入 0 拔出

		case DEVCAP_HDMI_TYPE:			// AVOUT类型
			return hw_uHdmiType;			

		case DEVCAP_LCD:					// LCD屏连接状态
			return hw_bLcdPlugIn;

		case DEVCAP_SCREEN:				// 外置显示屏状态
			if(hw_bAvoutPlugIn || hw_bLcdPlugIn || hw_bHdmiPlugIn)
				return 1;
			else
				return 0;

		case DEVCAP_SENSOR:				// 摄像头状态
			return hw_bSensorConnect;

		case DEVCAP_GPSBD:
			return hw_bGpsbdConnectState;

		case DEVCAP_VIDEO_REC:
			return hw_bVideoRecState;

		case DEVCAP_DAYNIGHT:
			if(hw_bDayNightState == 0)		// 晚上模式
				return DEVCAP_NIGHT;
			else if(hw_bDayNightState == 1)		// 白天模式
				return DEVCAP_DAY;
			else if(hw_bDayNightState == 2)		// 自动模式
			{
				XMSYSTEMTIME time;	
				XM_GetLocalTime (&time);
				// 7 ~ 19点
				if(time.bHour >= 7 && time.bHour <= 19)
					return DEVCAP_DAY;
				else
					return DEVCAP_NIGHT;
			}
			else
				return DEVCAP_DAY;

		default:
			return 0;
	}
}



extern void XM_heap_init (void);
extern void XM_heap_exit (void);


CRITICAL_SECTION CriticalSection;
CRITICAL_SECTION CodecCriticalSection;


void XM_SimulatorInit (void)
{
	XM_heap_init ();
	LoadInitialSetting ();
	InitializeCriticalSection (&CriticalSection);

	InitializeCriticalSection (&CodecCriticalSection);
}

void XM_SimulatorExit (void)
{
	XM_heap_exit ();
	SaveInitialSetting ();
	DeleteCriticalSection (&CriticalSection);
}

// 关中断
void XM_lock (void)
{
	EnterCriticalSection (&CriticalSection);
}

// 开中断
void XM_unlock (void)
{
	LeaveCriticalSection (&CriticalSection);
}

VOID XM_idle (VOID)
{

}

VOID XM_wakeup (VOID)
{
}

short int XM_GetKeyState (int vKey)
{
	return GetKeyState (vKey);
}


#define ANSI            /* Comment out for UNIX version     */
#include <stdarg.h>

void __cdecl XM_printf (const char *fmt, ...)
{
	char xm_info[2048];

	va_list ap;
	va_start(ap, fmt); 
	vsprintf (xm_info, fmt, ap);
	// vsprintf_s (xm_info, sizeof(xm_info), fmt, ap);
	::AfxTrace ("%s", xm_info);
	va_end(ap); 
}


int		XM_LoadMenuData (void *lpMenuData, int cbMenuData)
{
	FILE *fp = fopen ("menudata.bin", "rb");
	if(fp == 0)
		return -1;
	if( (int)fread (lpMenuData, 1, cbMenuData, fp) != cbMenuData)
	{
		fclose (fp);
		return -1;
	}
	fclose (fp);
	return 0;

}

int		XM_SaveMenuData (void *lpMenuData, int cbMenuData)
{
	FILE* fp = fopen ("menudata.bin", "wb");
	if(fp == 0)
		return -1;
	if( (int)fwrite (lpMenuData, 1, cbMenuData, fp) != cbMenuData)
	{
		fclose (fp);
		return -1;
	}
	fclose (fp);
	return 0;
}



void* OS_malloc(unsigned int n)
{
	return malloc (n);
}

void  OS_free  (void* pMemBlock)
{
	free (pMemBlock);
}

void* OS_calloc (unsigned int bytes,unsigned int size)
{
	return calloc (bytes, size);
}

void* OS_realloc  (void* pMemBlock, unsigned NewSize)
{
	return realloc (pMemBlock, NewSize);
}

}