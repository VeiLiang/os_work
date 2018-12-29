//****************************************************************************
//
//	Copyright (C) 2012~2016 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_adc.h
//	  adc device driver
//
//	Revision history
//
//		2013.07.02	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef XM_ADC_H_
#define XM_ADC_H_

enum {
	XM_ADC_AUX0 = 0,		// BAT
	XM_ADC_AUX1,			// KEY
	XM_ADC_COUNT
};

void xm_adc_init (void);
void xm_adc_test (void);

// 获取电池的当前电压(毫伏)
unsigned int XM_GetBatteryVoltage (void);


#endif /* XM_ADC_H_ */
