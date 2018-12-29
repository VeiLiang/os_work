//****************************************************************************
//
//	Copyright (C) 2010~2014 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_kernel_simulate.h
//	  硬件模拟接口
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_KERNEL_SIMULATE_H_
#define _XM_KERNEL_SIMULATE_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// Win32 内核模拟接口，用于MFC模拟硬件修改内核的状态

// -------------------------------------------------------
//
// ********** !!! 以下全局变量仅在Win32下应用 ************
//
// -------------------------------------------------------
#if WIN32 && _WINDOWS

extern unsigned int hw_bSDCardState;					// SD卡插入状态模拟
extern unsigned int hw_bTimeSetting;					// 系统时间设置状态 0 未设置 1 已设置
extern unsigned int hw_bBackupBatteryVoltageLevel;	// 备份电池状态
extern unsigned int hw_bMainBatteryVoltageLevel;	// 主电池状态	
extern unsigned int hw_bLcdPlugIn;						// LCD屏连接(有屏产品)或未接(无屏产品)
extern unsigned int hw_bAvoutPlugIn;					// AVOUT插入/拔出模拟
extern unsigned int hw_uAvoutType;						// AVOUT输出类型
extern unsigned int hw_bHdmiPlugIn;						// HDMI插入/拔出状态
extern unsigned int hw_uHdmiType;						// HDMI类型(产品定义)
extern unsigned int hw_bSensorConnect;					// Sensor 0(摄像头0)识别(连接)状态												
extern unsigned int hw_bUsbConnectState;				// USB连接状态
extern unsigned int hw_bGpsbdConnectState;			// GPSBD连接状态
extern unsigned int hw_bDayNightState;					// 白天、黑夜模拟						

// ******** 视频模拟接口 ********

// 在Win32 HDC设备的指定位置显示lcdc的输出通道
void FML_VideoUpdate (HDC hdc, int x, int y, unsigned int lcdc_channel);
// 创建win32位图clipboard
void FML_CreateBitmapClipboard (HWND hWnd, unsigned int lcd_channel);

// WIN32下模拟触发疲劳驾驶事件触发
void Win32_TriggerLongTimeDrivingEvent (unsigned int event);

void FML_IspIsr (void);

#endif

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_KERNEL_SIMULATE_H_