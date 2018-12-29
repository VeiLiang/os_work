#ifndef _PWM_H_
#define _PWM_H_

#if defined (__cplusplus)
	extern "C"{
#endif
		
#include "types.h"
		
//  共 八  个 PWM 通道
enum {
	XM_PWM_CHANNEL_0 = 0,		// 通道0
	XM_PWM_CHANNEL_1,				// 通道1
	XM_PWM_CHANNEL_2,
	XM_PWM_CHANNEL_3,
	
	XM_PWM_CHANNEL_4,			   // 通道4
	XM_PWM_CHANNEL_5,				// 通道5
	XM_PWM_CHANNEL_6,
	XM_PWM_CHANNEL_7,
	
	XM_PWM_CHANNEL_COUNT
}; 


enum {
	XM_PWM_OFF = 0,	 // 0
	XM_PWM_ON,			 // 1
}; 


void PWM_Init(void);

void PWM_CFG (int ch, int cmd, int div, int duty, int cycle );

void PWM_config (int ch, int pwm_ena, int fri, float ratio, int polarity , int pwm_activate, int cycle );

void PWM_OnOff (int ch, int OnOff);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _PWM_H_

