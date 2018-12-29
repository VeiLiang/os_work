//****************************************************************************
//
//	Copyright (C) 2011 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_device.h
//	  基本设备接口 (按键、定时器、MCI)
//
//	Revision history
//
//		2011.05.31	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_DEVICE_H_
#define _XM_DEVICE_H_

#include <xm_type.h>
#include <xm_dev.h>
#include "xm_config.h"

// 按键驱动
XMBOOL XM_KeyDriverGetEvent (WORD *key, WORD *modifier);
void XM_KeyDriverOpen (void);
void XM_KeyDriverClose (void);
XMBOOL XM_KeyDriverPoll(void);

XMBOOL XM_TouchDriverGetEvent (DWORD *point, DWORD *type);
void XM_TouchDriverOpen (void);
void XM_TouchDriverClose (void);
XMBOOL XM_TouchDriverPoll(void);

// 定时器驱动
XMBOOL XM_TimerDriverDispatchEvent (void);
void XM_TimerDriverOpen (void);
void XM_TimerDriverClose (void);
XMBOOL XM_TimerDriverPoll (void);

// UART驱动
XMBOOL XM_UartDriverGetEvent (WORD *key, WORD *modifier);
void XM_UartDriverOpen (void);
void XM_UartDriverClose (void);
XMBOOL XM_UartDriverPoll(void);

// MCI驱动
void XM_MciDriverOpen (void);
void XM_MciDriverClose (void);
XMBOOL XM_MciDriverPoll (void);
XMBOOL XM_MciDriverDispatchEvent (void);


// USB驱动打开
void XM_UsbDriverOpen (void);
// USB驱动关闭
void XM_UsbDriverClose (void);
// 读取当前的USB事件
unsigned char XM_UsbDriverGetEvent (void);
//=========================
// 查询是否存在未读的USB事件
XMBOOL XM_UsbDriverPoll(void);




void winuser_init (void);
void wingdi_init (void);
void winmem_init (void);


DWORD XM_MainLoop (VOID);

#endif	// #ifndef _GDI_DEVICE_H_

