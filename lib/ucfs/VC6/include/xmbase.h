//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xmbase.h
//	  constant，macro & basic typedef definition of kernel service
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_BASE_H_
#define _XM_BASE_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// macro definition

// 系统重启类型定义
#define	SDS_REBOOT				0		// re-booting 系统重新引导
#define	SDS_POWEROFF			1		// system power off 系统关机,唤醒后重新引导
#define	SDS_RESUME				2		// system power off 系统关机,唤醒后恢复上次的现场

// 电池状态事件
#define	DEVCAP_VOLTAGE_WORST		0
#define	DEVCAP_VOLTAGE_BAD		1
#define	DEVCAP_VOLTAGE_NORMAL	2
#define	DEVCAP_VOLTAGE_GOOD		3


// bDevCapIndex定义
#define	DEVCAP_AUTOPOWEROFFTIME			0x01
			// 设置或读取自动关机时间
// wValue = 0表示禁止自动关机，非0值表示秒数 (1 ~ 120)

#define	DEVCAP_VIDEOCONTRASTLEVEL		0x02
			// 设置或读取对比度 (0 ~ 15级)

#define	DEVCAP_KEYBOARDTICKON			0x03
			// 设置按键声音是否开启 （0 关闭， 1 开启）

#define	DEVCAP_MAINBATTERYVOLTAGE		0x04
			// 读取电压状态
			// 0  Worst (很差)
			// 1  bad	(较差)
			// 2	normal(正常)
			// 3  best	(良好)

#define	DEVCAP_USB							0x05
			// 读取USB的状态
	#define	DEVCAP_USB_DISCONNECT			0		// USB连接断开
	#define	DEVCAP_USB_CONNECT_CHARGE		1		// USB已插入，作为充电线使用
	// USB设备功能
	#define	DEVCAP_USB_CONNECT_UDISK		2		// USB已插入，主机作为其他设备的U盘使用
	#define	DEVCAP_USB_CONNECT_CAMERA		3		// USB已插入，主机作为其他设备的摄像头

#define	DEVCAP_BACKLIGHT					0x06
			// 启动或关闭背光
			// 1  开启背光
			// 0	关闭背光

#define	DEVCAP_SDCARDSTATE				0x07	
			// SD卡状态检测
			// 0  SD卡准备中(正在检测SD卡)
			//	1	SD卡未插入
			// 2	SD卡写保护
			//	3	SD卡插入，且读写允许
			//	4	SD卡插入，卡文件系统已安装 (无法创建保存视频的目录)
			// 5	SD卡插入，且读写允许，文件校验已失败(可能是坏卡)
			// 6	SD卡插入，数据卡不能识别，需更换
			// 7	SD卡插入，且读写允许，数据卡已写满
			// 8	SD卡插入，无法支持的文件系统 exFAT或者NTFS
			//		"卡文件系统异常"及"卡文件校验失败"提示用户执行格式化操作
	#define	DEVCAP_SDCARDSTATE_DETECT						0	// SD卡准备中(正在检测SD卡)
	#define	DEVCAP_SDCARDSTATE_UNPLUG						1	// 数据卡拔出
	#define	DEVCAP_SDCARDSTATE_WRITEPROTECT				2	// 数据卡写保护
	#define	DEVCAP_SDCARDSTATE_INSERT						3	// 数据卡读写模式(正常模式)
	#define	DEVCAP_SDCARDSTATE_FSERROR						4	// 数据卡已识别(读写模式)，卡文件系统已安装
																			//		无法在数据卡上创建保存视频记录的目录， 
																			//		存在与目录同名的文件时，将其重命名(自动修复)
																			//		无法自动修复时需要提示"格式化或者换卡"
															
	#define	DEVCAP_SDCARDSTATE_VERIFYFAILED				5	// 文件读写校验失败 (数据卡存在失效的坏扇区，可能是坏卡)
	#define	DEVCAP_SDCARDSTATE_INVALID						6	//	数据卡不能识别，需更换(FS_MOUNT失败)
	#define	DEVCAP_SDCARDSTATE_DISKFULL					7	// 数据卡已写满
	#define	DEVCAP_SDCARDSTATE_FSUNSUPPORT				8	// 无法支持的文件系统 (exFAT或者NTFS)

#define	DEVCAP_BACKUPBATTERYVOLTAGE	0x08
			// 备份电池电压状态
	#define	DEVCAP_BATTERYVOLTAGE_WORST		0
	#define	DEVCAP_BATTERYVOLTAGE_BAD			1
	#define	DEVCAP_BATTERYVOLTAGE_NORMAL		2
	#define	DEVCAP_BATTERYVOLTAGE_BEST			3
			// 0  Worst (很差)
			// 1  bad	(较差)
			// 2	normal(正常)
			// 3  best	(良好)

#define	DEVCAP_MIC							0x09
			// MIC状态读取及设置
			//	0	关闭
			// 1	开启
	#define	DEVCAP_MIC_OFF		0
	#define	DEVCAP_MIC_ON		1

#define	DEVCAP_TIMESETTING				0x0A
			// 读取系统时间设置状态
			// 0	系统时间尚未设置
			// 1	系统时间已设置
	#define	DEVCAP_TIMESETTING_NG	0
	#define	DEVCAP_TIMESETTING_OK	1	

#define	DEVCAP_AVOUT						0x0B
			// 读取系统AVOUT的插入状态
			// 0	AVOUT拔出
			// 1	AVOUT插入
	#define	DEVCAP_AVOUT_PLUGOUT		0	
	#define	DEVCAP_AVOUT_PLUGIN		1

#define	DEVCAP_AVOUT_TYPE					0x0C
			// 读取系统AVOUT的类型定义 (由产品定义)
			// 取值参考 hw_osd_layer.h 中的LCDC输出通道类型定义的AVOUT定义

#define	DEVCAP_HDMI							0x0D
			// 读取系统HDMI的插入状态
			// 0	HDMI拔出
			// 1	HDMI插入
	#define	DEVCAP_HDMI_PLUGOUT		0	
	#define	DEVCAP_HDMI_PLUGIN		1

#define	DEVCAP_HDMI_TYPE					0x0E
			// 读取系统定义的HDMI的类型 (由产品定义)
			// 取值参考 hw_osd_layer.h 中的LCDC输出通道类型定义的HDMI定义


#define	DEVCAP_LCD							0x0F	
			// 读取LCD屏的连接状态(有屏产品或无屏产品)
			// 0	无屏产品
			// 1	有屏产品
	#define	DEVCAP_LCD_DISCONNECT	0	
	#define	DEVCAP_LCD_CONNECT		1	

#define	DEVCAP_SCREEN						0x10
			// 读取显示屏的连接状态
			// 0	显示屏未连接(LCD、AVOUT、HDMI等显示外设均未连接)
			// 1	显示屏已连接(至少LCD、AVOUT、HDMI其中一种显示外设已连接)
	#define	DEVCAP_SCREEN_DISCONNECT	0
	#define	DEVCAP_SCREEN_CONNECT		1

#define	DEVCAP_GPSBD					0x11
			// 读取GPSBD的连接状态 
			// 0	GPSBD已断开(未连接)
			//	1	GPSBD已连接(天线未连接)
			// 2	GPSBD已连接(天线短路)
			// 3	GPSBD已连接(天线已连接)
			// 4	GPSBD已连接(定位成功)
	#define	DEVCAP_GPSBD_DISCONNECT						0
	#define	DEVCAP_GPSBD_CONNECT_ANTENNA_OPEN		1	
	#define	DEVCAP_GPSBD_CONNECT_ANTENNA_SHORT		2	
	#define	DEVCAP_GPSBD_CONNECT_ANTENNA_OK			3	
	#define	DEVCAP_GPSBD_CONNECT_LOCATE_OK			4	

#define	DEVCAP_VIDEO_REC				0x12		// 录像状态(停止、录像中)
	#define	DEVCAP_VIDEO_REC_STOP						0
	#define	DEVCAP_VIDEO_REC_START						1


#define	DEVCAP_VIDEOITEM				0x13		// 视频项状态
															//	0 表示视频项数据正常，无错误
	#define	DEVCAP_VIDEOITEM_NO_DATA					0x00000001		// 无视频项数据库
																						//		卡不存在、卡写保护、卡损坏
	#define	DEVCAP_VIDEOITEM_TOO_MANY_ITEM			0x00000002		// 视频项太多，超出系统定义
																						//		需要格式化或自动删除
	#define	DEVCAP_VIDEOITEM_LOWEST_RECYCLE_SPACE	0x00000004		// 可循环使用空间太低，无法录制一个完整的视频
																						//		需要格式化或者自动删除
	
	

#define	DEVCAP_SENSOR						0x20
#define	DEVCAP_SENSOR_1					0x21
#define	DEVCAP_SENSOR_2					0x22
#define	DEVCAP_SENSOR_3					0x23
#define	DEVCAP_SENSOR_4					0x24
#define	DEVCAP_SENSOR_5					0x25
#define	DEVCAP_SENSOR_6					0x26
#define	DEVCAP_SENSOR_7					0x27
			// 读取摄像头的状态
			// 0	摄像头无法识别，未连接
			// 1	摄像头已识别，成功连接
	#define	DEVCAP_SENSOR_DISCONNECT	0
	#define	DEVCAP_SENSOR_CONNECT		1


#define	DEVCAP_DAYNIGHT					0x28		// 白天/晚上状态
	#define	DEVCAP_NIGHT					0			// 晚上
	#define	DEVCAP_DAY						1			// 白天

#ifndef _XMBOOL_DEFINED_
#define _XMBOOL_DEFINED_
typedef unsigned char		XMBOOL;		// BOOL类型
#endif

// 系统参数设置
XMBOOL	XM_SetFmlDeviceCap  (BYTE bDevCapIndex, DWORD wValue);
DWORD  	XM_GetFmlDeviceCap  (BYTE bDevCapIndex);

// typedef definition

// structure definition

// 系统时间
typedef struct tagXMSYSTEMTIME {
	WORD		wYear;			
	BYTE		bDay;				// 1 ~ 31
	BYTE		bMonth;			// 1 ~ 12
	BYTE		bDayOfWeek;		// 0 ~ 6
	BYTE		bHour;			// 0 ~ 23
	BYTE		bMinute;			// 0 ~ 59
	BYTE		bSecond;			// 0 ~ 59
	WORD		wMilliSecond;	// 0 ~ 999
} XMSYSTEMTIME, *PXMSYSTEMTIME;

// 闹铃
typedef struct tagXMSYSTEMALARM {
	UCHAR		AlarmHour;		// 0 ~ 23
	UCHAR		AlarmMinute;	// 0 ~ 59
	WORD		AlarmDay;		// since 2010.1.1						
} XMSYSTEMALARM, *PXMSYSTEMALARM;

// function protocol type
VOID XM_InitRTC (VOID);

// 时间设置/读取函数
//		返回1表示系统时间已设置
//		返回0表示系统时间未设置，系统返回缺省定义时间
XMBOOL	XM_GetLocalTime	(XMSYSTEMTIME* pSystemTime);
VOID		XM_SetLocalTime	(const XMSYSTEMTIME *pSystemTime);

// 获取机器启动后的滴答计数 （1ms为一滴答计数单位）
DWORD		XM_GetTickCount	(VOID);

// 获取机器启动后高精度的滴答计数，以微秒为计数单位
XMINT64 		XM_GetHighResolutionTickCount (void);


VOID		XM_Sleep (DWORD dwMilliseconds);		// 延时，毫秒单位

// 闹铃设置
XMBOOL	XM_SetAlarm		(const XMSYSTEMALARM *pSystemAlarm);
XMBOOL	XM_KillAlarm	(VOID);

// 系统重启,非法重启方式等同于 SDS_REBOOT
VOID		XM_ShutDownSystem  (BYTE bShutDownType);


XMBOOL	XM_LoadUSBModule		(VOID);
XMBOOL	XM_ExitUSBModule		(VOID);

XMBOOL	XM_UsbDetect			(VOID);

VOID		XM_EnableUsb			(XMBOOL bEnableUsb);

short int XM_GetKeyState		(int vKey);

// 按键驱动初始化
void 		XM_KeyInit				(void);

#define	XM_BEEP_KEYBOARD			1

// Beep发音(如按键音)
XMBOOL XM_Beep (DWORD dwBeepID);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_BASE_H_
