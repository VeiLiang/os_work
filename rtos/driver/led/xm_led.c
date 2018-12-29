//****************************************************************************
//
//	Copyright (C) 2012~2016 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_led.c
//	  led device driver
//
//	Revision history
//
//		2015.12.02	ZhuoYongHong Initial version
//	
//
//****************************************************************************
#include "hardware.h"
#include "gpio.h"
#include "xm_led.h"

// LED初始化
void xm_led_init (void)
{
	unsigned int val;
	
#if CZS_USB_01
	// LED_EN_1 D2 BLUE_LED (LCD_D14/GPIO66)
	// LED_EN_2 D3 RED_LED  (LCD_D15/GPIO67)
	XM_lock ();
	val = rSYS_PAD_CTRL06 ;
	val &= ~((7 << 12) | (7 << 15));
	rSYS_PAD_CTRL06 = val;	
	SetGPIOPadDirection (GPIO66, euOutputPad);
	SetGPIOPadDirection (GPIO67, euOutputPad);
	XM_unlock ();
#endif
	
#if 0
	// ITUB_D8 REC_LED
	// pad_ctl2
	//	[20:18]	itu_b_din8	itu_b_din8_pad	GPIO26	itu_b_din[8]
	XM_lock ();
	val = rSYS_PAD_CTRL02 ;
	val &= ~( (7 << 18) );
	rSYS_PAD_CTRL02 = val;	
	XM_unlock ();
	SetGPIOPadDirection (GPIO26, euOutputPad);
#endif
}

// led开启/关闭
// led	led序号
// on_off	1 开启 0 关闭
void xm_led_on (int led, int on_off)
{
    return ;
#if CZS_USB_01
	if(led == XM_LED_1)	// LED_EN_1 D2 BLUE_LED (LCD_D14/GPIO66)
	{
		if(on_off)
			SetGPIOPadData (GPIO66, euDataHigh);
		else
			SetGPIOPadData (GPIO66, euDataLow);
	}
	else if(led == XM_LED_2)	// LED_EN_2 D3 RED_LED  (LCD_D15/GPIO67)
	{
		if(on_off)
			SetGPIOPadData (GPIO67, euDataHigh);
		else
			SetGPIOPadData (GPIO67, euDataLow);
	}
#endif
	
#if TULV
	if(led == XM_LED_1)	
	{
		if(on_off)
			SetGPIOPadData (GPIO26, euDataHigh);
		else
			SetGPIOPadData (GPIO26, euDataLow);
	}

#endif	
	
}
