#ifndef CPUCLKCOUNTER_H__
#define CPUCLKCOUNTER_H__

#ifdef __cplusplus
extern "C" {
#endif
	
// CLK_ID∂®“Â
enum {
	ARKN141_CLK_CPUPLL = 0,
	ARKN141_CLK_SYSPLL,
	ARKN141_CLK_AUDPLL,
	ARKN141_CLK_24M,
	ARKN141_CLK_240M,
	ARKN141_CLK_DDR,
	ARKN141_CLK_CPU,
	ARKN141_CLK_AXI,
	ARKN141_CLK_AHB,
	ARKN141_CLK_APB,
	ARKN141_CLK_ISP,
	ARKN141_CLK_ISP_SCALAR,
	ARKN141_CLK_SCALAR,
	ARKN141_CLK_H264_ENCODER,
	ARKN141_CLK_H264_DECODER,
	ARKN141_CLK_SENSOR_MCLK,
	ARKN141_CLK_LCD_CLK,
	ARKN141_CLK_COUNT
};




UINT32 GetCPUPLLFrequency(void);

UINT32 GetSYSPLLFrequency(void);

UINT32 GetAUDPLLFrequency(void);

UINT32 GetEXT24ExcryptFrequency(void);

UINT32 GetEXTRTCFrequency(void);

UINT32 GetAHB_CLK(void);

UINT32 GetAPB_CLK(void);

UINT32 GetCPU_CLK(void);

UINT32 GetXCLK(void);

UINT32 GetDDRIICLK(void);

UINT32 GetLCDFPS(void);

UINT32 GetCLK240MFrequency (void);

UINT32 arkn141_get_clks (UINT32 clk_id);

void ShowHWFreqInfo(void);

enum {
	zctrl_clk_enable = 0,
	Mae2_enc_enable,
	Mae2_dec_enable,
	Scale_clk_enable,
	Isp_clk_enable,
	Isp_xclk_enable,
	Scale_xclk_enable,
	h2xdma_xclk_enable,
	h2xusb_xclk_enable,
	imc_xclk_enable,			// Internal RAM clock
	ddr_xclk_enable,
	lcd_xclk_enable,
	itu656_xclk_enable,
	deinterlace_xclk_enable ,
	mae_xclk_enable,
	Isp_hclk_enable,
	ddrctl_hclk_enable,
	MFC_hclk_enable,
	h2xdma_hclk_enable,
	h2xusb_hclk_enable,
	Isp_Scale_clk_enable,
	mac_hclk_enable,
	sdc1_hclk_enable,
	sdc_hclk_enable,
	nand_hclk_enable,
	dma_hclk_enable,
	usb_hclk_enable,
	i2s2_pclk_enable,
	i2s1_pclk_enable,
	icu_pclk_enable,
	pwm_pclk_enable,
	rcrt_pclk_enable,
	timer_pclk_enable,
	uart3_pclk_enable,
	uart2_pclk_enable,
	uart1_pclk_enable,
	uart0_pclk_enable,
	rtc_pclk_enable,
	gpio_pclk_enable,
	ssp_pclk_enable,
	adc_pclk_enable,
	i2c_pclk_enable,
	i2s_pclk_enable,
	wdt_pclk_enable,
	Tv_out_lcd_pixel_clk_enable,
	lcd_out_clk_enable,
	Sensor_mclk_out_enable,
	Mac_rx_clk_enable,
	Mac_tx_clk_enable,
	Itu_b_clk_enable,		// itu656_clk_in
	Itu_a_clk_enable,		// isp_clk_in
	Uart3_clk_enable,
	Uart2_clk_enable,
	Uart1_clk_enable,
	Uart0_clk_enable,
	Gpio_debounce_clk_enable,
	rcrt_clk_out_enable,
	adc_clk_enable,
	i2c_clk_enable,
	pwm_clk_enable,
	i2s2_mclk_enable,
	i2s2_bitclk_enable,
	i2s1_mclk_enable,
	i2s1_bitclk_enable,
	sadc_clk_enable,
	i2s_mclk_enable,
	i2s_bitclk_enable,
	spi_clk_enable,
	Spi1_clk_enable,
	USB_PHY_clk_enable,
	usb_12m_enable,
	lcd_clk_enable,
	ddr_phyclk_enable
};

void sys_clk_enable (unsigned int clk);

void sys_clk_disable (unsigned int clk);

#ifdef __cplusplus
}
#endif

#endif

