//****************************************************************************
//
//	Copyright (C) 2011 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xmdev.h
//	  constant，macro & basic typedef definition of X-Mini System
//
//	Revision history
//
//		2011.05.31	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_DEV_H_
#define _XM_DEV_H_

#if defined (__cplusplus)
	extern "C"{
#endif

		extern unsigned char *XM_DispMap;

// LCD颜色模式定义
#define	LCD_GRY1BPP					0		// 黑白
#define	LCD_GRY2BPP					1		//	4级灰度
#define	LCD_GRY4BPP					2		// 16级灰度
#define	LCD_GRY8BPP					3		//	256级灰度
#define	LCD_RGB4BPP					4		//	16色彩色(EGA)
#define	LCD_RGB8BPP					5		// 256色彩色(调色板)
#define	LCD_RGB16BPP				6		// 65536色彩色(656模式)
#define	LCD_RGB24BPP				7		//	24位真彩
#define	LCD_RGB32BPP				8		// 32位真彩
#define	LCD_GRY2BPP_ST7586S		9		// ST7586S 四级灰度
#define	LCD_ARGB32BPP				10		// ARGB32位

// Modifier属性定义
#define	XMKEY_PRESSED			0x10		/* key pressed */
#define	XMKEY_REPEAT			0x20		/* Key Repeat Status */
#define	XMKEY_STROKE			0x40		/* indicate hardware stroke */

#define	XMEXIT_POWERDOWN			(WORD)(0)		// 正常退出并关机
#define	XMEXIT_ROMERROR			(WORD)(-1)		// ROM无效或错误


// OSD相对显示区(原点位于左上角)偏移
#define	OSD_MAX_H_POSITION	(2048 - 1)		// 最大水平像素偏移(相对左上角)
#define	OSD_MAX_V_POSITION	(2048 - 1)		// 最大垂直像素偏移(相对左上角)

#define	OSD_MAX_H_SIZE			(2048)			// OSD最大像素宽度
#define	OSD_MAX_V_SIZE			(2048)			// OSD最大像素高度







#include "xmconfig.h"

// GUI主函数入口，返回值为XMEXIT_xxx退出码
int XM_Main (void);

// GUI向内核注册按键事件回调函数
// 返回值定义
//		1		事件投递到事件缓冲队列成功
//		0		事件投递到事件缓冲队列失败
unsigned char XM_KeyEventProc (unsigned short key, unsigned short mod);
// GUI向内核注册定时器事件回调函数
unsigned char XM_TimerEventProc (void);
// GUI向内核注册MCI事件回调函数
unsigned char XM_MciEventProc (void);


// USB事件定义
#define	USB_EVENT_NONE						0		// 无USB事件
#define	USB_EVENT_SYSTEM_UPDATE			1		// 系统升级事件

// 投递硬件USB事件, 由外部硬件驱动调用
unsigned char XM_UsbEventProc (unsigned char UsbEvent);

#ifndef _XMBOOL_DEFINED_
#define _XMBOOL_DEFINED_
typedef unsigned char		XMBOOL;		// BOOL类型
#endif

// 向消息队列投递消息
XMBOOL	XM_PostMessage (WORD message, WORD wp, DWORD lp);

// 在消息队列首部插入消息
XMBOOL	XM_InsertMessage (WORD message, WORD wp, DWORD lp);


// 关中断
void XM_lock (void);

// 开中断
void XM_unlock (void);

// 进入待机状态
void XM_idle (void);

// 唤醒XM主任务
void XM_wakeup (void);

// 判断设备是主机、辅机模式
// 1 主机 0 辅机
int XM_IsMasterDevice (void);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_DEV_H_
