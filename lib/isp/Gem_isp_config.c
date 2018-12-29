// =============================================================================
// File        : 
// Version     :  
// Author      :  
// Date        :  
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#include  "hardware.h"
#include "arkn141_isp_sensor_cfg.h"
#include  "Gem_isp_sys.h"
#include  "Gem_isp.h"
#include  "RTOS.H"
#include <stdio.h>
#include <stdlib.h>
#include "gpio.h"
#include <xm_dev.h>

/* 	时钟 为FPGA 第三个时钟 */

/*
padlist 配置 ：
I2C 配置 ：
	pad_ctla ： scl sda 
Sensor:
	pad_ctla ：gpio0 
	pad_ctl0 : ituclk_in_a   itu_a_vsync_in  itu_a_hsync_in
	pad_ctl0 : itu_a_din0 - itu_a_din6
	pad_ctl1 : itu_a_din7 - itu_a_din11
	12根数据线 + 行信号 + 场信号 + 2根时钟线(MCLK PCLK)
*/



//#define	SENSOR_RST_IO		GPIO80
//#define	SENSOR_RST_IO		GPIO31

// itu_b_din9	itu_b_din9_pad	GPIO27
#define	SENSOR_RST_IO			GPIO75


void isp_sensor_set_reset_pin_high (void)
{
	SetGPIOPadData (SENSOR_RST_IO, euDataHigh);
}

void isp_sensor_set_reset_pin_low (void)
{
	SetGPIOPadData (SENSOR_RST_IO, euDataLow);
}

void isp_sensor_set_standby_pin_low (void)
{
	// CSI_PWDN (CD3/GP83)
	SetGPIOPadData (GPIO83, euDataLow);
}

void isp_sensor_set_standby_pin_high (void)
{
	SetGPIOPadData (GPIO83, euDataHigh);
}


/*******************************************************************************
void isp_SelPad()
用于isp选择pin脚
********************************************************************************/
void isp_SelPad (void)
{
	unsigned int val;
	XM_lock ();

	// RESET
#if SENSOR_RST_IO == GPIO80
	// pad_ctl8
	// [1:0]	cd0	cd0_pad	GPIO80	nand_data[9]
	val = rSYS_PAD_CTRL08;
	val &= ~(3 << 0);
	rSYS_PAD_CTRL08 = val;
#elif SENSOR_RST_IO == GPIO31
	// pad_ctl3
	// [5:3]	itu_c_vsync_in	itu_c_vsync_in_pad	GPIO31	itu_c_vsync_in	mac_mdio	pwm_out[1]
	val = rSYS_PAD_CTRL03;
	val &= ~(7 << 3);
	rSYS_PAD_CTRL03 = val;
	
#elif SENSOR_RST_IO == GPIO27
	// pad_ctl2
	// [23:21]	itu_b_din9	itu_b_din9_pad	GPIO27
	val = rSYS_PAD_CTRL02;
	val &= ~( (7 << 21) );
	rSYS_PAD_CTRL02 = val;
#elif SENSOR_RST_IO == GPIO75
    val = rSYS_PAD_CTRL07;
	val &= ~( (7 << 9) );
	rSYS_PAD_CTRL07 = val;
#endif
	
	SetGPIOPadDirection (SENSOR_RST_IO, euOutputPad);
	SetGPIOPadData (SENSOR_RST_IO, euDataLow);
	
	XM_printf ("Sensor Reset Pin (GPIO%d)\n", SENSOR_RST_IO);
	
	rSYS_DEVICE_CLK_CFG0 &= ~(0x1<<8);// MCLK时钟相位不取反 

	// ISP的sensor时钟只能从 GPIO1 输出
	val = rSYS_PAD_CTRL0A ;
	val &= ~(3 << 6);
	val |=  (1 << 6);
	rSYS_PAD_CTRL0A = val;	
	// 将GPIO1(sen_clk_out)的驱动能力修改为最大
	rSYS_IO_DRIVER01 = (unsigned int)(rSYS_IO_DRIVER01 | (unsigned int)(0x3 << 30));

	val = rSYS_PAD_CTRL00;
	val &=0xc0000000;
	//     clk sensor输出来的时钟信号
	//     clk =bit.2-0:GPIO127	
	//     clk   | vs   |  hs  | din0 | din1  |  din2 | din3  | din4  |  din5 | din6    
	val |= (1<<0)|(1<<3)|(1<<6)|(1<<9)|(1<<12)|(1<<15)|(1<<18)|(1<<21)|(1<<24)|(1<<27);
	rSYS_PAD_CTRL00 = val;
	
	val = rSYS_PAD_CTRL01 ;
	val &=0xffff8000;
	//     din7 | din8 | din9 | din10| din11
	val |=(1<<0)|(1<<3)|(1<<6)|(1<<9)|(1<<12) ;
	rSYS_PAD_CTRL01 = val;

	// SENA_STANDY
	// pad_ctl8
	// [7:6]	cd3	cd3_pad	GPIO83	nand_data[3]
	val = rSYS_PAD_CTRL08;
	val &= ~(3 << 6);
	rSYS_PAD_CTRL08 = val;
	SetGPIOPadDirection (GPIO83, euOutputPad);
	SetGPIOPadData (GPIO83, euDataHigh);
	

	XM_unlock ();
}

/*******************************************************************************
*	isp_UnSelPad()
*	清除为 GPIO 
********************************************************************************/
void isp_UnSelPad(void)
{
	unsigned int val;
	
	XM_lock ();
	
	// gpio1_pad 设置为 GPIO1, 输出低
	val = rSYS_PAD_CTRL0A ;
	val &= ~(3 << 6);
	rSYS_PAD_CTRL0A = val ;
	SetGPIOPadDirection (GPIO1, euOutputPad);
	SetGPIOPadData (GPIO1, euDataLow);
	
	val = rSYS_PAD_CTRL00;
	val &=0xc0000000;
	//       clk   | vs   |  hs  | din0 | din1  |  din2 | din3  | din4  |  din5 | din6    
	//val |= (1<<0)|(1<<3)|(1<<6)|(1<<9)|(1<<12)|(1<<15)|(1<<18)|(1<<21)|(1<<24)|(1<<27);
	rSYS_PAD_CTRL00 = val;
	
	val = rSYS_PAD_CTRL01 ;
	val &= 0xffff8000;
	//       din7 | din8 | din9 | din10| din11
	//val |=(1<<0)|(1<<3)|(1<<6)|(1<<9)|(1<<12) ;
	rSYS_PAD_CTRL01 = val;
	
	XM_unlock ();
}

extern void isp_clock_stop (void);
void isp_clock_init (void)
{
	unsigned int sen_clk_switch_sel, sen_clk_div, Sen_mclk_sel;
	unsigned int val;
	unsigned int Int_ISP_clk_sel, ISP_clk_div, Itu_clk_a_dly;
	
	
	isp_clock_stop ();
	sys_soft_reset (softreset_isp);
	delay (0x10000);

	// MCLK = 12MHz
	
	//  SYS_DEVICE_CLK_CFG1
	// 23	R/W	0	Sen_mclk_sel:
	// 				1'b0: sen_clk_int.
	// 				1'b1: sen_clk_int_inv
	Sen_mclk_sel = 0;
	
	
	// 22-19	R/W	0	sen_clk_div
	// 					sen_clk_int = sen_clk_switch/(sen_clk_div? sen_clk_div:1)
	sen_clk_div = 2;		// 24/2= 12
	
	// 18-15	R/W	4	sen_clk_switch_sel: mfc_clk_switch
	// 					4'b0000: Clk_240m
	//						4'b0001: Syspll_clk
	//						4'b0010: audpll_clk
	//						4'b0100: Clk_24m
	sen_clk_switch_sel = 4;
	
	
	XM_lock ();
	//rSYS_DEVICE_CLK_CFG0 |= (1 << 8);
	val = rSYS_DEVICE_CLK_CFG1;
	val &= ~((0xF << 15) | (0xF << 19) | (0x1 << 23));
	val |=  (sen_clk_switch_sel << 15) | (sen_clk_div << 19) | (Sen_mclk_sel << 23);
	rSYS_DEVICE_CLK_CFG1 = val;
	XM_unlock ();
	
	// 20180313 99M(AUD396/4)
	// 20180131 ISP时钟配置为134M(402/3，Int_ISP_clk_sel = 2， ISP_clk_div = 3)时，开关机测试较为稳定， 但还是挂机。
	// 20180303 ISP时钟配置为78MHz(390/5, Int_ISP_clk_sel = 2， ISP_clk_div = 5)时, 开关机测试稳定(ISP启动前关闭其他模块的时钟) 
	//				初步怀疑是ISP启动瞬间，1.2V大电流导致某个模块挂死
	// 2-0	R/W	4	Int_ISP_clk_sel
	// 					4'b0000: itu_clk_a
	// 					4'b0001: Syspll_clk
	// 					4'b0010: audpll_clk
	// 					4'b0100: Clk_24m
	Int_ISP_clk_sel = 1;
	//Int_ISP_clk_sel = 2;
	// 5-3	R/W	0	ISP clk div
	//						ISP clk = int_ISP_clk/div
	//ISP_clk_div = 3;		// 354/3 = 118, OKm
	//Int_ISP_clk_sel = 2;
	//ISP_clk_div = 3;		// 402/3 = 134, OK (每15秒软开关机测试24小时OK)
	ISP_clk_div = 4;		// 402/4 = 100.5, W_1600_H_750 NG, W_1500_H_800 OK, 开关机启动ISP出现挂机, 1分钟
	//ISP_clk_div = 5;		// 312/5 = 62.4, W_1500_H_800 ISP挂死， W_1450_H_825 OK, 开关机启动ISP出现挂机， 10分钟
	//ISP_clk_div = 6;		// 312/6 = 52, W_1410_H_850 OK
	//ISP_clk_div = 7;
	
	// 22-16	R/W	0x0	Itu_clk_a_dly	
	//Itu_clk_a_dly = 0x30;	//0x60;
	//Itu_clk_a_dly = 0x60;
	Itu_clk_a_dly = 0;		// 不同的板dly设置存在差异,需要微调
	
	XM_lock ();
	val = rSYS_DEVICE_CLK_CFG3;
	val &= ~((0x7 << 0) | (0x7 << 3) | (0x7F << 16) );
	val |=  ((Int_ISP_clk_sel << 0) | (ISP_clk_div << 3) | (Itu_clk_a_dly << 16) );
	rSYS_DEVICE_CLK_CFG3 = val;
	XM_unlock ();

	sys_clk_enable (Isp_hclk_enable);
	sys_clk_enable (Isp_clk_enable);
	sys_clk_enable (Isp_xclk_enable);
	sys_clk_enable (Sensor_mclk_out_enable);
	
	delay (10);
	
	XM_printf ("sensor_mclk = %d, isp_clk = %d\n", arkn141_get_clks(ARKN141_CLK_SENSOR_MCLK), arkn141_get_clks(ARKN141_CLK_ISP));
			  
}

void isp_clock_stop (void)
{
	sys_clk_disable (Sensor_mclk_out_enable);
	sys_clk_disable (Isp_clk_enable);
	sys_clk_disable (Isp_xclk_enable);
	sys_clk_disable (Isp_hclk_enable);
}

// ISP上电复位
void isp_cold_reset (void)
{
	irq_disable(ISP_INT);
	rISP_SYS &= ~1;
	OS_Delay (1);
   rSYS_SOFT_RSTNB &= ~(1<<11);
	OS_Delay (1);	
	rSYS_SOFT_RSTNB |= (1<<11);
}

void isp_reset (void)
{
   irq_disable(ISP_INT);
	
   rISP_SYS &= ~1;
	OS_Delay (40);	// 等待ISP 至少一帧
   rSYS_SOFT_RSTNB &= ~(1<<11);
   OS_Delay(30);
   rSYS_SOFT_RSTNB |= (1<<11);
   OS_Delay(1);

}


extern void isp_isr (void);

void isp_int_init (void)
{
	request_irq(ISP_INT, PRIORITY_FIFTEEN, isp_isr );	
}

void isp_int_exit (void)
{
	request_irq(ISP_INT, PRIORITY_FIFTEEN, 0 );	
}

