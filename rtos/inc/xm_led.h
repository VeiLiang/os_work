//****************************************************************************
//
//	Copyright (C) 2012~2016 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_led.h
//	  led device driver
//
//	Revision history
//
//		2013.07.02	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef XM_LED_H_
#define XM_LED_H_

enum {
	XM_LED_1 = 0,
	XM_LED_2,
	XM_LED_3,
	XM_LED_4,
	XM_LED_COUNT
};

void xm_led_init (void);

// led����/�ر�
// led	led���
// on_off	1 ���� 0 �ر�
void xm_led_on (int led, int on_off);


#endif /* XM_LED_H_ */
