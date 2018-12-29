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


// Modifier属性定义
#define	XMKEY_PRESSED			0x10		/* key pressed */
#define	XMKEY_REPEAT			0x20		/* Key Repeat Status */
#define	XMKEY_STROKE			0x40		/* indicate hardware stroke */
#define	XMKEY_LONGTIME			0x80		/* long time pressed */

#define	XMEXIT_POWERDOWN			(WORD)(0)		// 正常退出并关机
#define	XMEXIT_ROMERROR			(WORD)(-1)		// ROM无效或错误


// OSD相对显示区(原点位于左上角)偏移
#define	OSD_MAX_H_POSITION	(2048 - 1)		// 最大水平像素偏移(相对左上角)
#define	OSD_MAX_V_POSITION	(2048 - 1)		// 最大垂直像素偏移(相对左上角)

#define	OSD_MAX_H_SIZE			(2048)			// OSD最大像素宽度
#define	OSD_MAX_V_SIZE			(2048)			// OSD最大像素高度

// GUI主函数入口，返回值为XMEXIT_xxx退出码
int XM_Main (void);

// 投递按键事件到事件消息队列
// 返回值定义
//		1		事件投递到事件缓冲队列成功
//		0		事件投递到事件缓冲队列失败
unsigned char XM_KeyEventProc (unsigned short key, unsigned short mod);

// 检查并投递硬件事件, 由外部硬件驱动调用
// 如果事件未在硬件消息队列中, 将该事件加入到队尾.
// 若该事件已在硬件消息队列中, 不做任何处理, 返回
// 返回值
//   1        表示该消息已投递到硬件队列
//   0        表示该消息未能投递到硬件队列, 消息投递失败
unsigned char XM_KeyEventPost (unsigned short key, unsigned short mod);


// GUI向内核注册定时器事件回调函数
unsigned char XM_TimerEventProc (void);
// GUI向内核注册MCI事件回调函数
unsigned char XM_MciEventProc (void);


#define	TOUCH_TYPE_DOWN	1	// 触摸按下事件类型
#define	TOUCH_TYPE_UP		2	// 触摸释放事件类型
#define	TOUCH_TYPE_MOVE	3	// 触摸移动事件类型
// 投递触摸事件到消息队列
// point		触摸位置
// type		触摸事件类型
// 返回值定义
//		1		事件投递到事件缓冲队列成功
//		0		事件投递到事件缓冲队列失败
unsigned char XM_TouchEventProc (unsigned long point, unsigned int type);


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
