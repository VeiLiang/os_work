#include <xm_proj_define.h>
#include <intrinsics.h>
#include <stdlib.h>
#include <stdio.h>
#include "hardware.h"
#include "xm_type.h"
#include "gpio.h"
#include "xm_app_menudata.h"

//硬件配置文件

extern void XM_lock(void);
extern void XM_unlock(void);

//ir
//采用固定GPIO115,PIN133


//AES
//采用固定GPIO28,GPIO29

//SD
//采用固定PIN136-PIN142


//LCD
#define BIAS_PIN	GPIO33
#define BL_PIN		GPIO34
#define H_ROTATE 	GPIO80//左右
#define V_ROTATE 	GPIO81//上下
#define LCD_VCOMDC	GPIO31//PWM1
#define LCD_PWM1_CH	1


void lcd_ctrl_pin_init(void)
{
    unsigned int val;

	//BIAS
	XM_lock();
	val = rSYS_PAD_CTRL03;
	val &= ~(0x07 << 9);
	rSYS_PAD_CTRL03 = val;
	XM_unlock();

	SetGPIOPadDirection(BIAS_PIN, euOutputPad);
	SetGPIOPadData(BIAS_PIN, euDataLow);
	
	//BL
	XM_lock();
    val = rSYS_PAD_CTRL03;
    val &= ~(0x07 << 12);
    rSYS_PAD_CTRL03 = val;
	XM_unlock();
	
	SetGPIOPadDirection(BL_PIN, euOutputPad);
	SetGPIOPadData(BL_PIN, euDataLow);	

	//LR
    XM_lock();
	val = rSYS_PAD_CTRL08;
	val &= ~(0x03 << 0);
	rSYS_PAD_CTRL08 = val;
	XM_unlock();
	SetGPIOPadDirection(H_ROTATE, euOutputPad);

	//UD
    XM_lock();
	val = rSYS_PAD_CTRL08;
	val &= ~(0x03 << 2);
	rSYS_PAD_CTRL08 = val;
	XM_unlock();
    SetGPIOPadDirection(V_ROTATE, euOutputPad);
}

void set_lcd_bias_pin_value(u8_t val)
{
	if(val==0)
	{
		SetGPIOPadData(BIAS_PIN, euDataLow);
	}
	else
	{
		SetGPIOPadData(BIAS_PIN, euDataHigh);
	}
}

void set_lcd_bl_pin_value(u8_t val)
{
	if(val==0)
	{
		SetGPIOPadData(BL_PIN, euDataLow);
	}
	else
	{
		SetGPIOPadData(BL_PIN, euDataHigh);
	}
}

void HW_LCD_ROTATE(int select)
{
	if(select == AP_SETTING_LCD_FLIP_NORMAL)
	{
		SetGPIOPadData (H_ROTATE, euDataHigh);
		SetGPIOPadData (V_ROTATE, euDataLow);
	}
	else if(select == AP_SETTING_LCD_FLIP_MIRROR)
	{
		SetGPIOPadData (H_ROTATE, euDataLow);
		SetGPIOPadData (V_ROTATE, euDataHigh);
	}
}

void set_pwm_duty(u8_t vcomdc)
{
	int div = arkn141_get_clks(ARKN141_CLK_APB) / 1000000;
	int duty = div * vcomdc / 100  ;	// 50% duty AP_GetVCOM_PWM
	int cycle = 0xFFFFFFFF;
	int pwm_ena = 1;
	int polarity = 0;
	int intr_en = 0;		// 不产生中断
	int pwm_activate = 1;
	int cmd = ( pwm_ena)|( pwm_activate << 2 )|( intr_en << 3)|( polarity << 4);
	
	PWM_CFG(1, cmd, div, duty, cycle);
}

void lcd_vcom_init(void)
{
	unsigned int val;

	//GPIO31,pwm1
	val = rSYS_PAD_CTRL03;
	val &= ~(0x07 << 3);
	val |=  (0x03 << 3);
	rSYS_PAD_CTRL03 = val;
	
	// 13	R/W	1	pwm_pclk enable
	//				1: enable
	//				0: disable
	rSYS_APB_CLK_EN |= (1 << 13);

	// 14	R/W	1	pwm_clk enable
	//					1: enable
	//					0: disable
	rSYS_PER_CLK_EN |= (1 << 14);

	set_pwm_duty(AP_GetVCOM_PWM());
	
	PWM_OnOff(1,1);//通道1
}


//sensor
//IIC采用固定接口,参照gpioi2c.c文件
#define RXCHIP_SENSOR0_RESET_PIN 	GPIO6
#define RXCHIP_SENSOR1_RESET_PIN 	GPIO26
#define RXCHIP_SDA_PIN				GPIO103
#define RXCHIP_SCL_PIN				GPIO104


void rxchip_iic_pin_init(void)
{
	unsigned int val;

	// 第三组GPIO I2C
	// 7116-SDA SD1_D2 ([9]	 sd1_d2	sd1_d2_pad	GPIO103	sd1_data2)
	// 7116-SCL SD1_D3 ([10] sd1_d3	sd1_d3_pad	GPIO104	sd1_data3)
	// pad_ctl9	
	XM_lock();
	val = rSYS_PAD_CTRL0A;
	val &= ~( (0x03 << 0) | (0x03 << 2) );
	rSYS_PAD_CTRL0A= val;
	XM_unlock();

	SetGPIOPadDirection (RXCHIP_SDA_PIN, euOutputPad);
	SetGPIOPadDirection (RXCHIP_SCL_PIN, euOutputPad);	
}

void rxchip_reset_pin_init(void)
{
	unsigned int val;

	//sensor 0,GPIO6
	XM_lock();
	val = rSYS_PAD_CTRL00;
	val &= ~(0x07 << 18);
	rSYS_PAD_CTRL00 = val;
	XM_unlock();

	SetGPIOPadDirection (RXCHIP_SENSOR0_RESET_PIN, euOutputPad);

	//sensor 1,GPIO26
	XM_lock();
	val = rSYS_PAD_CTRL02;
	val &= ~(0x07 << 18);
	rSYS_PAD_CTRL02 = val;
	XM_unlock();
	
	SetGPIOPadDirection (RXCHIP_SENSOR1_RESET_PIN, euOutputPad);
}

void set_sensor0_reset_pin_value(u8_t val)
{
	XM_lock ();	
	if(val==0)
	{
		SetGPIOPadData(RXCHIP_SENSOR0_RESET_PIN, euDataLow);
	}
	else
	{
		SetGPIOPadData(RXCHIP_SENSOR0_RESET_PIN, euDataHigh);
	}
	XM_unlock();
}

void set_sensor1_reset_pin_value(u8_t val)
{
	XM_lock ();	
	if(val==0)
	{
		SetGPIOPadData(RXCHIP_SENSOR1_RESET_PIN, euDataLow);
	}
	else
	{
		SetGPIOPadData(RXCHIP_SENSOR1_RESET_PIN, euDataHigh);
	}
	XM_unlock();
}

//LED
#define LED_CTRL_PIN GPIO93
void led_ctrl_pin_init(void)
{
	unsigned int val;

	//KEY LED 
    XM_lock();
	val = rSYS_PAD_CTRL08;
	val &= ~(0x03 << 26);
	rSYS_PAD_CTRL08 = val;
	XM_unlock();
	SetGPIOPadDirection(LED_CTRL_PIN, euOutputPad);
}

void set_led_ctrl_pin_value(u8_t val)
{
	XM_lock ();	
	if(val==0)
	{
		SetGPIOPadData(LED_CTRL_PIN, euDataLow);
	}
	else
	{
		SetGPIOPadData(LED_CTRL_PIN, euDataHigh);
	}
	XM_unlock();
}


//5V电源控制
#define POWER5V_CTRL_PIN GPIO82
void power5v_ctrl_pin_init(void)
{
	unsigned int val;

	//power 5v 控制
    XM_lock();
	val = rSYS_PAD_CTRL08;
	val &= ~(0x03 << 4);
	rSYS_PAD_CTRL08 = val;
	XM_unlock();
	SetGPIOPadDirection(POWER5V_CTRL_PIN, euOutputPad);
}

void set_power5v_ctrl_pin_value(u8_t val)
{
	XM_lock ();	
	if(val==0)
	{
		SetGPIOPadData(POWER5V_CTRL_PIN, euDataLow);
	}
	else
	{
		SetGPIOPadData(POWER5V_CTRL_PIN, euDataHigh);
	}
	XM_unlock();
}


//ACC
#define ACC_DET_PIN	GPIO84
void acc_det_pin_init(void)
{
	unsigned int val;

	//acc det
    XM_lock();
	val = rSYS_PAD_CTRL08;
	val &= ~(0x03 << 8);
	rSYS_PAD_CTRL08 = val;
	XM_unlock();
	
	SetGPIOPadDirection(ACC_DET_PIN, euInputPad);
}


EU_GPIO_Data get_acc_det_pin_status(void)
{
	return GetGPIOPadData(ACC_DET_PIN);
}


//按键ADC
#define KEY_ADC_PIN	GPIO36

//POWER电压检测,KEY0
#define POWER_DET_PIN 


//倒车触发
#define TRG_DET_PIN GPIO85
void trig_det_pin_init(void)
{
	unsigned int val;

	//acc det
    XM_lock();
	val = rSYS_PAD_CTRL08;
	val &= ~(0x03 << 10);
	rSYS_PAD_CTRL08 = val;
	XM_unlock();
	SetGPIOPadDirection(TRG_DET_PIN, euInputPad);
}

EU_GPIO_Data get_trig_det_pin_status(void)
{
	return GetGPIOPadData(TRG_DET_PIN);
}


//功放
#define PA_PIN GPIO32

void pa_set_d_class(void)
{
    unsigned int val;
    XM_lock();
    val = rSYS_PAD_CTRL03;
    val &= ~(0x07 << 6);//
    rSYS_PAD_CTRL03 = val;
    XM_unlock();
    SetGPIOPadDirection (PA_PIN, euOutputPad);
	SetGPIOPadData (PA_PIN, euDataLow);	
	delay(100);
	SetGPIOPadData (PA_PIN, euDataHigh);
	delay(100);
	SetGPIOPadData (PA_PIN, euDataLow);	
	delay(100);
	SetGPIOPadData (PA_PIN, euDataHigh);

}

void pa_close(void)
{
    unsigned int val;
    XM_lock();
    val = rSYS_PAD_CTRL03;
    val &= ~(0x07 << 6);//
    rSYS_PAD_CTRL03 = val;
    XM_unlock();
    SetGPIOPadDirection (PA_PIN, euOutputPad);
	SetGPIOPadData (PA_PIN, euDataLow);//

}


