//****************************************************************************
//
//	Copyright (C) 2012~2016 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_power.h
//	  power device driver
//
//	Revision history
//
//		2012.10.02	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_POWER_H_
#define _XM_POWER_H_

// 上电锁定
void xm_power_lock (void);

// 上电解锁,关机
void xm_power_unlock (void);

void xm_powerkey_init (void);

// 检测开关机按键的状态，返回0x8000表示按键按下，0表示按键释放
int xm_powerkey_test (void);

// 检测ACC是否已上电
//	返回值
//		1		Acc已上电
//		0		Acc未上电
int xm_power_check_acc_on (void);

// 检查ACC是否安全电压
// 返回值
//		1		Acc供电安全
//		0		Acc供电不安全
int xm_power_check_acc_safe_or_not (void);

// 检查USB是否供电
// 返回值
//		1		USB供电
//		0		USB未供电
int xm_power_check_usb_on (void);
void xm_board_power_ctl(u8 state);

#endif	// _XM_POWER_H_