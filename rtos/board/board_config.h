#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#ifdef  __cplusplus
extern "C" {
#endif  // __cplusplus

#include "xm_type.h"	
#include "gpio.h"
//LCD
extern void lcd_ctrl_pin_init(void);
extern void set_lcd_bias_pin_value(u8_t val);
extern void set_lcd_bl_pin_value(u8_t val);
extern void HW_LCD_ROTATE(int select);
extern void lcd_vcom_init(void);
extern void set_pwm_duty(u8_t vcomdc);

//sensor
extern void rxchip_reset_pin_init(void);
extern void set_sensor0_reset_pin_value(u8_t val);
extern void set_sensor1_reset_pin_value(u8_t val);

//LED
extern void led_ctrl_pin_init(void);
extern void set_led_ctrl_pin_value(u8_t val);

//power 5v
extern void power5v_ctrl_pin_init(void);
extern void set_power5v_ctrl_pin_value(u8_t val);

//´¥·¢
extern void trig_det_pin_init(void);
extern EU_GPIO_Data get_trig_det_pin_status(void);

//ACC
extern void acc_det_pin_init(void);

#ifdef __cplusplus
}
#endif     


#endif	

