
/***********************************************************************
*Copyright (c)2012  Arkmicro Technologies Inc. 
*All Rights Reserved 
*
*Filename:    clock.c
*Version :    1.0 
*Date    :    2009.05.12 
*Author  :    Carl 
*Abstract:    Ark1660 MPW PLL init&test code file. 
*History :     
* 
*Version :    2.0 
*Date    :    2012.02.27
*Author  :     
*Abstract:    ark1660  Ver1.0 MPW driver remove waring
*History :    1.0

************************************************************************/
 

#include "hardware.h"

void I2C_ClkEnable(void)
{
	sys_clk_enable (i2s_pclk_enable);
	sys_clk_enable (i2c_clk_enable);
}

void I2C_ClkDisable(void)
{
	sys_clk_disable (i2s_pclk_enable);
	sys_clk_disable (i2c_clk_enable);
}

/*
**************************************************************************************
*                       LCD时钟使能
*
* 说明: 使能LCD工作时钟
*			
*
* 参数: 无
*
*
* 返回值: 无
***************************************************************************************
*/
void Lcd_ClkEnable(void)
{
	sys_clk_enable (lcd_xclk_enable);
	sys_clk_enable (lcd_out_clk_enable);
	sys_clk_enable (lcd_clk_enable);
}

/*
**************************************************************************************
*                       关闭LCD时钟
*
* 说明: 关闭LCD工作时钟
*			
*
* 参数: 无
*
*
* 返回值: 无
***************************************************************************************
*/
void Lcd_ClkDisable(void)
{
	sys_clk_disable (lcd_xclk_enable);
	sys_clk_disable (lcd_out_clk_enable);
	sys_clk_disable (lcd_clk_enable);
}

void Spi_ClkEnable(void)
{
	sys_clk_enable (ssp_pclk_enable);
	sys_clk_enable (spi_clk_enable);
}

void Spi_ClkDisable(void)
{
	sys_clk_disable (ssp_pclk_enable);
	sys_clk_disable (spi_clk_enable);
}

void ITU656in_ClkEnable(void)
{
}

void ITU656in_ClkDisable(void)
{
}

void PreScaler_ClkEnable(void)
{
}

void PreScaler_ClkDisable(void)
{
}

void VideoScaler_ClkEnable(void)
{
}

void VideoScaler_ClkDisable(void)
{
}

void TVoutScreen_ClkEnable(void)
{
}

void TVoutScreen_ClkDisable(void)
{
}

void TVoutYpbpr_ClkEnable(void)
{
}

void TVoutYpbpr_ClkDisable(void)
{
}

void TVoutCVBS_ClkEnable(void)
{
}

void TVoutCVBS_ClkDisable(void)
{
}



void AudioCodec_ClkEnable(void)
{
	sys_clk_enable (i2s_pclk_enable);
	sys_clk_enable (i2s1_pclk_enable);
	sys_clk_enable (i2s2_pclk_enable);
}

void AudioCodec_ClkDisable(void)
{
	sys_clk_disable (i2s_pclk_enable);
	sys_clk_disable (i2s1_pclk_enable);
	sys_clk_disable (i2s2_pclk_enable);
}

void SDHC0_ClkEnable(void)
{
	sys_clk_enable (sdc_hclk_enable);
}

void SDHC0_ClkDisable(void)
{
	sys_clk_disable (sdc_hclk_enable);
}

void SDHC1_ClkEnable(void)
{
	sys_clk_enable (sdc1_hclk_enable);
}

void SDHC1_ClkDisable(void)
{
	sys_clk_disable (sdc1_hclk_enable);
}

void Nand_ClkEnable(void)
{
	sys_clk_enable (nand_hclk_enable);
}

void Nand_ClkDisable(void)
{
	sys_clk_disable (nand_hclk_enable);
}


void Uart0_ClkEnable(void)
{
	sys_clk_enable (uart0_pclk_enable);
}

void Uart0_ClkDisable(void)
{
	sys_clk_disable (uart0_pclk_enable);
}

void Uart1_ClkEnable(void)
{
	sys_clk_enable (uart1_pclk_enable);
}

void Uart1_ClkDisable(void)
{
	sys_clk_disable (uart1_pclk_enable);
}

void Uart2_ClkEnable(void)
{
	sys_clk_enable (uart2_pclk_enable);
}

void Uart2_ClkDisable(void)
{
	sys_clk_disable (uart2_pclk_enable);
}

void Uart3_ClkEnable(void)
{
	sys_clk_enable (uart3_pclk_enable);
}

void Uart3_ClkDisable(void)
{
	sys_clk_disable (uart3_pclk_enable);
}

void PWM_ClkEnable(void)
{
	sys_clk_enable (pwm_clk_enable);
}

void PWM_ClkDisable(void)
{
	sys_clk_disable (pwm_clk_enable);
}

void ADC_ClkEnable(void)
{
	sys_clk_enable (adc_pclk_enable);
	sys_clk_enable (adc_clk_enable);
}

void ADC_ClkDisable(void)
{
	sys_clk_disable (adc_pclk_enable);
	sys_clk_disable (adc_clk_enable);
}

void Rcrt_ClkEnable(void)
{
	sys_clk_enable (rcrt_pclk_enable);
}

void Rcrt_ClkDisable(void)
{
	sys_clk_disable (rcrt_pclk_enable);
}

void Wdt_ClkEnable(void)
{
	sys_clk_enable (wdt_pclk_enable);
}

void Wdt_ClkDisable(void)
{
	sys_clk_disable (wdt_pclk_enable);
}

void RTC_ClkEnable(void)
{
	sys_clk_enable (rtc_pclk_enable);
}

void RTC_ClkDisable(void)
{
	sys_clk_disable (rtc_pclk_enable);
}

void GPIO_ClkEnable(void)
{
	sys_clk_enable (gpio_pclk_enable);
}

void GPIO_ClkDisable(void)
{
	sys_clk_disable (gpio_pclk_enable);
}



