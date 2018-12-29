#ifndef _SYS_SOFT_RESET_H__
#define _SYS_SOFT_RESET_H__

#ifdef __cplusplus
extern "C" {
#endif
	
// CLK_ID∂®“Â
enum {
	softreset_card1 = 0,
	softreset_ddr_ahb_reset,
	softreset_zctrl,
	softreset_h2xusb,
	softreset_h2xdma,
	softreset_usbphy,
	softreset_dma,
	softreset_usb,
	softreset_deinter,
	softreset_mae2_core,
	softreset_nand,
	softreset_itu,
	softreset_adc,
	softreset_lcd,
	softreset_ddr,
	softreset_i2s,
	softreset_ssp,
	softreset_mac,
	softreset_card,
	softreset_i2c,
	softreset_ddr_cfg,
	softreset_ddrphy,
	softreset_ddr1,
	softreset_mae2_axi,
	softreset_rcrt,
	softreset_rtc,
	softreset_icu,
	softreset_dac,
	softreset_saradc,
	softreset_imc,
	softreset_isp_scale,
	softreset_isp,
	softreset_scale,
	softreset_i2s2,
	softreset_i2s1,
	softreset_uart0,
	softreset_uart1,
	softreset_uart2,
	softreset_uart3,
	softreset_gpio,
	softreset_timer,
	softreset_pwm,
	softreset_count
};


void sys_soft_reset (int reset_dev);


#ifdef __cplusplus
}
#endif

#endif	// _SYS_SOFT_RESET_H__

