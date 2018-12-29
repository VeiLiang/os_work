#ifndef CLOCK_H__
#define CLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

void I2C_ClkEnable(void);

void I2C_ClkDisable(void);

void Lcd_ClkEnable(void);

void Lcd_ClkDisable(void);

void Spi_ClkEnable(void);

void Spi_ClkDisable(void);

void ITU656in_ClkEnable(void);

void ITU656in_ClkDisable(void);

void PreScaler_ClkEnable(void);

void PreScaler_ClkDisable(void);

void VideoScaler_ClkEnable(void);

void VideoScaler_ClkDisable(void);

void TVoutScreen_ClkEnable(void);

void TVoutScreen_ClkDisable(void);

void TVoutYpbpr_ClkEnable(void);

void TVoutYpbpr_ClkDisable(void);

void TVoutCVBS_ClkEnable(void);

void TVoutCVBS_ClkDisable(void);

void M2MDma_ClkEnable(void);

void M2MDma_ClkDisable(void);

void MFC_ClkEnable(void);

void MFC_ClkDisable(void);

void Jpeg_ClkEnable(void);

void Jpeg_ClkDisable(void);

void MailBox_ClkEnable(void);

void MailBox_ClkDisable(void);

void AudioCodec_ClkEnable(void);

void AudioCodec_ClkDisable(void);

void SDHC0_ClkEnable(void);

void SDHC0_ClkDisable(void);

void SDHC1_ClkEnable(void);

void SDHC1_ClkDisable(void);

void SDHC2_ClkEnable(void);

void SDHC2_ClkDisable(void);

void Nand_ClkEnable(void);

void Nand_ClkDisable(void);

void TSDemux_ClkEnable(void);

void TSDemux_ClkDisable(void);

void HSUart0_ClkEnable(void);

void HSUart0_ClkDisable(void);

void HSUart1_ClkEnable(void);

void HSUart1_ClkDisable(void);

void Uart0_ClkEnable(void);

void Uart0_ClkDisable(void);

void Uart1_ClkEnable(void);

void Uart1_ClkDisable(void);

void Uart2_ClkEnable(void);

void Uart2_ClkDisable(void);

void Uart3_ClkEnable(void);

void Uart3_ClkDisable(void);

void PWM_ClkEnable(void);

void PWM_ClkDisable(void);

void ADC_ClkEnable(void);

void ADC_ClkDisable(void);

void Rcrt_ClkEnable(void);

void Rcrt_ClkDisable(void);

void Wdt_ClkEnable(void);

void Wdt_ClkDisable(void);

void RTC_ClkEnable(void);

void RTC_ClkDisable(void);

void GPIO_ClkEnable(void);

void GPIO_ClkDisable(void);


#ifdef __cplusplus
}
#endif

#endif

