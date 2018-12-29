/*
**********************************************************************
Copyright (c)2009 Arkmicro Technologies Inc.  All Rights Reserved
Filename: ark1620.h
Version : 1.0
Date    : 2008.01.10
Author  : Salem
Abstract: ark1620 SoC register definition
History :
          This file is derived from ark1600.h
          ark1600.h is derived from soc.h
;2009.03.16:Carl: ver 1.2 Modified for MIPS
***********************************************************************
*/

#ifndef _ARK1960_H_
#define _ARK1960_H_

#include <xm_proj_define.h>


#define VECTOR_ENABLE		0	//if you want to change this value, please
								//change the Assemble logic valuable VECTOR_ENABLE
								//in file Boot.s


typedef enum
{
	SPI_BOOTER,
	NAND_BOOTER,
	UART_BOOTER,
	USB_BOOTER,
	SDHC_BOOTER,
	BIST_BOOTER,
	UNKNOWN_BOOTER
}EU_BOOT_SEL;

#define BOOT_FROM_NAND				0x0 	//nand chip0
#define BOOT_FROM_SPI				0x1 	//spi chip 0
#define BOOT_FROM_UART				0x3 	//uart0
#define BOOT_FROM_SDHC_CHIP0		0x4 	//sdhc chip 0
#define BOOT_FROM_SDHC_CHIP1		0x5 	//sdhc chip 1

#define BOOT_FROM_USB2_BY_INT		0x8	//USB 2.0 INT
#define BOOT_FROM_USB2_BY_INQ		0x9	//USB 2.0 INQ
#define BOOT_FROM_USB11_BY_INT	0xA 	//USB 1.1 INT
#define BOOT_FROM_USB11_BY_INQ	0xB 	//USB 1.1 INQ
#define BOOT_FROM_BIST				0xF	//BIST TEST MODE

/***************************************************************
    System Memory Base

****************************************************************/
//#define IRAM_BASE			0xC0000000
#define IRAM_BASE			0x100000
#define IROM_BASE			0x00000000		//before remap

/* External SDRAM */
#define DDRII_MEM_BASE	0x80000000



// ******** AHB_CTL ***************

#define NAND_BASE					0x50000000			// rs_nand	32' h5000_0000, 256M
#define SDHC0_BASE				0x60000000			// sdmmc		32' h6000_0000, 128M
#define SDHC1_BASE				0x68000000			// sdmmc1	32' h6800_0000, 128M

#define DMA_BASE					0x70020000			// ahb_dma	32' h7002_0000, 64K
#define MAC_BASE					0x70050000			// ahb_mac	32' h7005_0000, 64K
#define USB_BASE					0x70060000			// ahb_usb	32' h7006_0000, 64K
#define USB1_BASE					0x70070000			// ahb_usb	32' h7007_0000, 64K
#define MEMCTL_BASE				0x70080000			//	ahb_memctl	32' h7008_0000, 64K
#define AES_BASE					0x70090000			//	ahb_aes	32' h7009_0000, 64K
#define ISP_BASE              0x70090000   
#define ISP_SCALE_BASE        0X70098000        // Scale for isp or 601
#define DEINTERLACE_BASE		0x70130000			//	ahb_deinterlace	32'h7013_0000,64K
#define ITU656_BASE				0x70180000			//	ahb_656		32'h7018_0000,64K
#define LCD_BASE					0x70190000			//	ahb_lcd		32'h7019_0000,64K
#define SCALE_BASE     		   0x701A0000
#define MAE2_BASE					0x70200000			// ahb_mae2		32'h7020_0000,2M

// ****** AHB2APB0 *******
#define ICU_BASE					0x40400000			// apb_icu		32'h4040_0000,4K
//#define I2S_BASE					0x40401000			//	apb_i2s0		32'h4040_1000,4K
#define I2S_BASE					0x40800000			//	apb_i2s1		32'h4040_1000,4K

#define SPI_BASE					0x40402000			//	apb_spi		32'h4040_2000,4K
#define SSI_BASE					SPI_BASE
#define ADC_BASE					0x40403000			//	apb_adc		32'h4040_3000,4K
#define WDT_BASE					0x40404000			// apb_wdt		32'h4040_4000,4K
#define TIMER_BASE				0x40405000			// apb_timer	32'h4040_5000,4K
#define RTC_BASE					0x40406000			// apb_rtc		32'h4040_6000,4K
#define RCRT_BASE					0x40407000			// apb_rcrt		32'h4040_7000,4K
#define RC_BASE					RCRT_BASE
#define SYS_BASE					0x40408000			// apb_sys		32'h4040_8000,4K
#define GPIO_BASE					0x40409000			// apb_gpio		32'h4040_9000,4K
#define UART0_BASE				0x4040A000			// apb_uart0	32'h4040_A000,4K
#define UART1_BASE				0x4040B000			// apb_uart1	32'h4040_B000,4K
#define UART2_BASE				0x4040C000			//	apb_uart2	32'h4040_C000,4K
#define UART3_BASE				0x4040D000			// apb_uart3	32'h4040_D000,4K
#define PWM_BASE					0x4040E000			// apb_pwm		32'h4040_E000,4K
#define IIC_BASE					0x4040F000			// apb_i2c		32'h4040_F000,4K


#define I2S_ADC_BASE          0x40401000		// I2S ADC 0
#define I2S_DAC_BASE          0x40800000
#define I2S1_BASE					0x40800000			//	apb_i2s1		32'h4080_0000,4K
#define I2S2_BASE					0x40801000			//	apb_i2s2		32'h4080_1000,4K

//spi1 base address: apb_spi132'h4080_2000,4k
#define SSI1_BASE             0x40802000


#define USE_UART0										1
#define USE_UART1										0
#define USE_UART2										0
#define USE_UART3										0


#if USE_UART0
#define UART_BASE										UART0_BASE
#define UART_INT										UART0_INT
#elif USE_UART1
#define UART_BASE										UART1_BASE
#define UART_INT										UART1_INT
#elif USE_UART2
#define UART_BASE										UART2_BASE
#define UART_INT										UART2_INT
#elif USE_UART3
#define UART_BASE										UART3_BASE
#define UART_INT										UART3_INT
#endif

//uart
#define rUART_DR(UARTx_BASE)						(*(volatile unsigned int *)(UARTx_BASE + 0x00))
#define rUART_RSR(UARTx_BASE)						(*(volatile unsigned int *)(UARTx_BASE + 0x04))
#define rUART_FR(UARTx_BASE)						(*(volatile unsigned int *)(UARTx_BASE + 0x18))
#define rUART_ILPR(UARTx_BASE)					(*(volatile unsigned int *)(UARTx_BASE + 0x20))
#define rUART_IBRD(UARTx_BASE)					(*(volatile unsigned int *)(UARTx_BASE + 0x24))
#define rUART_FBRD(UARTx_BASE)					(*(volatile unsigned int *)(UARTx_BASE + 0x28))
#define rUART_LCR_H(UARTx_BASE)					(*(volatile unsigned int *)(UARTx_BASE + 0x2C))
#define rUART_CR(UARTx_BASE)						(*(volatile unsigned int *)(UARTx_BASE + 0x30))
#define rUART_IFLS(UARTx_BASE)					(*(volatile unsigned int *)(UARTx_BASE + 0x34))
#define rUART_IMSC(UARTx_BASE)					(*(volatile unsigned int *)(UARTx_BASE + 0x38))
#define rUART_RIS(UARTx_BASE)						(*(volatile unsigned int *)(UARTx_BASE + 0x3C))
#define rUART_MIS(UARTx_BASE)						(*(volatile unsigned int *)(UARTx_BASE + 0x40))
#define rUART_ICR(UARTx_BASE)						(*(volatile unsigned int *)(UARTx_BASE + 0x44))
#define rUART_DMACR(UARTx_BASE)					(*(volatile unsigned int *)(UARTx_BASE + 0x48))




/***************************************************************
      AHB slave interface registers definition
****************************************************************/

/* AHB system */
#define rSYS_BOOT_SAMPLE								*((volatile unsigned int *)(SYS_BASE+0x0))
#define rSYS_CLK_SEL									*((volatile unsigned int *)(SYS_BASE+0x40))
#define rSYS_AHB_CLK_EN								*((volatile unsigned int *)(SYS_BASE+0x44))
#define rSYS_APB_CLK_EN									*((volatile unsigned int *)(SYS_BASE+0x48))
#define rSYS_AXI_CLK_EN									*((volatile unsigned int *)(SYS_BASE+0x4c))
#define rSYS_PER_CLK_EN									*((volatile unsigned int *)(SYS_BASE+0x50))
#define rSYS_LCD_CLK_CFG							*((volatile unsigned int *)(SYS_BASE+0x54))
#define rSYS_SD_CLK_CFG									*((volatile unsigned int *)(SYS_BASE+0x58))
#define rSYS_SD1_CLK_CFG								*((volatile unsigned int *)(SYS_BASE+0x5c))
#define rSYS_DEVICE_CLK_CFG0								*((volatile unsigned int *)(SYS_BASE+0x60))
#define rSYS_DEVICE_CLK_CFG1							*((volatile unsigned int *)(SYS_BASE+0x64))
#define rSYS_DEVICE_CLK_CFG2							*((volatile unsigned int *)(SYS_BASE+0x68)) 
#define rSYS_DEVICE_CLK_CFG3							*((volatile unsigned int *)(SYS_BASE+0x6c)) 
#define rSYS_CLK_DLY_REG							*((volatile unsigned int *)(SYS_BASE+0x70))
#define rSYS_SOFT_RSTNA							*((volatile unsigned int *)(SYS_BASE+0x74))
#define rSYS_SOFT_RSTNB							*((volatile unsigned int *)(SYS_BASE+0x78))
#define rSYS_SD2_CLK_CFG							*((volatile unsigned int *)(SYS_BASE+0x7c))
#define rSYS_ANALOG_REG0								*((volatile unsigned int *)(SYS_BASE+0x140))	
#define rSYS_ANALOG_REG1								*((volatile unsigned int *)(SYS_BASE+0x144))	
#define rSYS_DDR2_PHY_REG								*((volatile unsigned int *)(SYS_BASE+0x148))	 
#define rSYS_PLLRFCK_CTL								*((volatile unsigned int *)(SYS_BASE+0x14c))	
#define rSYS_CPUPLL_CFG								*((volatile unsigned int *)(SYS_BASE+0x150))	
#define rSYS_SYSPLL_CFG								*((volatile unsigned int *)(SYS_BASE+0x154))	
#define rSYS_AUDPLL_CFG								*((volatile unsigned int *)(SYS_BASE+0x158))	
#define rSYS_I2S_NCO_CFG							*((volatile unsigned int *)(SYS_BASE+0x15C))
#define rSYS_I2S1_NCO_CFG							*((volatile unsigned int *)(SYS_BASE+0x16C))
#define rSYS_I2S2_NCO_CFG							*((volatile unsigned int *)(SYS_BASE+0x170))


#define rSYS_PAD_CTRL00							*((volatile unsigned int *)(SYS_BASE+0x1c0))
#define rSYS_PAD_CTRL01							*((volatile unsigned int *)(SYS_BASE+0x1c4))
#define rSYS_PAD_CTRL02							*((volatile unsigned int *)(SYS_BASE+0x1c8))
#define rSYS_PAD_CTRL03							*((volatile unsigned int *)(SYS_BASE+0x1cc))
#define rSYS_PAD_CTRL04							*((volatile unsigned int *)(SYS_BASE+0x1d0))
#define rSYS_PAD_CTRL05							*((volatile unsigned int *)(SYS_BASE+0x1d4))
#define rSYS_PAD_CTRL06							*((volatile unsigned int *)(SYS_BASE+0x1d8))
#define rSYS_PAD_CTRL07							*((volatile unsigned int *)(SYS_BASE+0x1dc))
#define rSYS_PAD_CTRL08							*((volatile unsigned int *)(SYS_BASE+0x1e0))
#define rSYS_PAD_CTRL09							*((volatile unsigned int *)(SYS_BASE+0x1e4))
#define rSYS_PAD_CTRL0A							*((volatile unsigned int *)(SYS_BASE+0x1e8))
#define rSYS_PAD_CTRL0B                             		*((volatile unsigned int *)(SYS_BASE+0x1ec))
#define rSYS_PAD_CTRL0C                             		*((volatile unsigned int *)(SYS_BASE+0x1f0))
#define rSYS_PAD_CTRL0F                   *((volatile unsigned int *)(SYS_BASE+0x180))

#define rSYS_IO_DRIVER01                             				*((volatile unsigned int *)(SYS_BASE+0x1f4))
#define rSYS_IO_DRIVER02                             				*((volatile unsigned int *)(SYS_BASE+0x1f8))

/* DDRII_Controller */
#define rDDR_CCR										*((volatile unsigned int  *) (DDRIII_BASE + 0x00))
#define rDDR_DCR										*((volatile unsigned int  *) (DDRIII_BASE + 0x04))
#define rDDR_IOCR										*((volatile unsigned int  *) (DDRIII_BASE + 0x08))
#define rDDR_CSR										*((volatile unsigned int  *) (DDRIII_BASE + 0x0C))
#define rDDR_DRR										*((volatile unsigned int  *) (DDRIII_BASE + 0x10))
#define rDDR_TPR0										*((volatile unsigned int  *) (DDRIII_BASE + 0x14))
#define rDDR_TPR1										*((volatile unsigned int  *) (DDRIII_BASE + 0x18))
#define rDDR_TPR2										*((volatile unsigned int  *) (DDRIII_BASE + 0x1C))
#define rDDR_DLLCR										*((volatile unsigned int  *) (DDRIII_BASE + 0x20))
#define rDDR_DLLCR0										*((volatile unsigned int  *) (DDRIII_BASE + 0x24))
#define rDDR_DLLCR1										*((volatile unsigned int  *) (DDRIII_BASE + 0x28))
#define rDDR_DLLCR2										*((volatile unsigned int  *) (DDRIII_BASE + 0x2C))
#define rDDR_DLLCR3										*((volatile unsigned int  *) (DDRIII_BASE + 0x30))
#define rDDR_DLLCR4										*((volatile unsigned int  *) (DDRIII_BASE + 0x34))
#define rDDR_DLLCR5										*((volatile unsigned int  *) (DDRIII_BASE + 0x38))
#define rDDR_DLLCR6										*((volatile unsigned int  *) (DDRIII_BASE + 0x3C))
#define rDDR_DLLCR7										*((volatile unsigned int  *) (DDRIII_BASE + 0x40))
#define rDDR_DLLCR8										*((volatile unsigned int  *) (DDRIII_BASE + 0x44))
#define rDDR_DLLCR9										*((volatile unsigned int  *) (DDRIII_BASE + 0x48))
#define rDDR_RSLR0										*((volatile unsigned int  *) (DDRIII_BASE + 0x4C))
#define rDDR_RSLR1										*((volatile unsigned int  *) (DDRIII_BASE + 0x50))
#define rDDR_RSLR2										*((volatile unsigned int  *) (DDRIII_BASE + 0x54))
#define rDDR_RSLR3										*((volatile unsigned int  *) (DDRIII_BASE + 0x58))
#define rDDR_RDGR0										*((volatile unsigned int  *) (DDRIII_BASE + 0x5C))
#define rDDR_RDGR1										*((volatile unsigned int  *) (DDRIII_BASE + 0x60))
#define rDDR_RDGR2										*((volatile unsigned int  *) (DDRIII_BASE + 0x64))
#define rDDR_RDGR3										*((volatile unsigned int  *) (DDRIII_BASE + 0x68))
#define rDDR_DQTR0										*((volatile unsigned int  *) (DDRIII_BASE + 0x6C))
#define rDDR_DQTR1										*((volatile unsigned int  *) (DDRIII_BASE + 0x70))
#define rDDR_DQTR2										*((volatile unsigned int  *) (DDRIII_BASE + 0x74))
#define rDDR_DQTR3										*((volatile unsigned int  *) (DDRIII_BASE + 0x78))
#define rDDR_DQTR4										*((volatile unsigned int  *) (DDRIII_BASE + 0x7C))
#define rDDR_DQTR5										*((volatile unsigned int  *) (DDRIII_BASE + 0x80))
#define rDDR_DQTR6										*((volatile unsigned int  *) (DDRIII_BASE + 0x84))
#define rDDR_DQTR7										*((volatile unsigned int  *) (DDRIII_BASE + 0x88))
#define rDDR_DQTR8										*((volatile unsigned int  *) (DDRIII_BASE + 0x8C))
#define rDDR_DQSTR										*((volatile unsigned int  *) (DDRIII_BASE + 0x90))
#define rDDR_DQSBTR										*((volatile unsigned int  *) (DDRIII_BASE + 0x94))
#define rDDR_ODTCR										*((volatile unsigned int  *) (DDRIII_BASE + 0x98))
#define rDDR_DTR0										*((volatile unsigned int  *) (DDRIII_BASE + 0x9C))
#define rDDR_DTR1										*((volatile unsigned int  *) (DDRIII_BASE + 0xA0))
#define rDDR_DTAR										*((volatile unsigned int  *) (DDRIII_BASE + 0xA4))
#define rDDR_ZQCR0										*((volatile unsigned int  *) (DDRIII_BASE + 0xA8))
#define rDDR_ZQCR1										*((volatile unsigned int  *) (DDRIII_BASE + 0xAC))
#define rDDR_ZQCR2										*((volatile unsigned int  *) (DDRIII_BASE + 0xB0))
#define rDDR_ZQSR										*((volatile unsigned int  *) (DDRIII_BASE + 0xB4))
#define rDDR_TPR3										*((volatile unsigned int  *) (DDRIII_BASE + 0xB8))
#define rDDR_ALPMR										*((volatile unsigned int  *) (DDRIII_BASE + 0xBC))
#define rDDR_MR											*((volatile unsigned int  *) (DDRIII_BASE + 0x1F0))
#define rDDR_EMR										*((volatile unsigned int  *) (DDRIII_BASE + 0x1F4))
#define rDDR_EMR2										*((volatile unsigned int  *) (DDRIII_BASE + 0x1F8))
#define rDDR_EMR3										*((volatile unsigned int  *) (DDRIII_BASE + 0x1FC))

#define rDDR_HPCR0										*((volatile unsigned int  *) (DDRIII_BASE + 0x200))
#define rDDR_HPCR1										*((volatile unsigned int  *) (DDRIII_BASE + 0x204))
#define rDDR_HPCR2										*((volatile unsigned int  *) (DDRIII_BASE + 0x208))
#define rDDR_HPCR3										*((volatile unsigned int  *) (DDRIII_BASE + 0x20C))
#define rDDR_HPCR4										*((volatile unsigned int  *) (DDRIII_BASE + 0x210))
#define rDDR_HPCR5										*((volatile unsigned int  *) (DDRIII_BASE + 0x214))
#define rDDR_HPCR6										*((volatile unsigned int  *) (DDRIII_BASE + 0x218))
#define rDDR_HPCR7										*((volatile unsigned int  *) (DDRIII_BASE + 0x21C))
#define rDDR_HPCR8										*((volatile unsigned int  *) (DDRIII_BASE + 0x220))
#define rDDR_HPCR9										*((volatile unsigned int  *) (DDRIII_BASE + 0x224))
#define rDDR_HPCR10										*((volatile unsigned int  *) (DDRIII_BASE + 0x228))
#define rDDR_HPCR11										*((volatile unsigned int  *) (DDRIII_BASE + 0x22C))
#define rDDR_HPCR12										*((volatile unsigned int  *) (DDRIII_BASE + 0x230))
#define rDDR_HPCR13										*((volatile unsigned int  *) (DDRIII_BASE + 0x234))
#define rDDR_HPCR14										*((volatile unsigned int  *) (DDRIII_BASE + 0x238))
#define rDDR_HPCR15										*((volatile unsigned int  *) (DDRIII_BASE + 0x23C))
#define rDDR_HPCR16										*((volatile unsigned int  *) (DDRIII_BASE + 0x240))
#define rDDR_HPCR17										*((volatile unsigned int  *) (DDRIII_BASE + 0x244))
#define rDDR_HPCR18										*((volatile unsigned int  *) (DDRIII_BASE + 0x248))
#define rDDR_HPCR19										*((volatile unsigned int  *) (DDRIII_BASE + 0x24C))
#define rDDR_HPCR20										*((volatile unsigned int  *) (DDRIII_BASE + 0x250))
#define rDDR_HPCR21										*((volatile unsigned int  *) (DDRIII_BASE + 0x254))
#define rDDR_HPCR22										*((volatile unsigned int  *) (DDRIII_BASE + 0x258))
#define rDDR_HPCR23										*((volatile unsigned int  *) (DDRIII_BASE + 0x25C))
#define rDDR_HPCR24										*((volatile unsigned int  *) (DDRIII_BASE + 0x260))
#define rDDR_HPCR25										*((volatile unsigned int  *) (DDRIII_BASE + 0x264))
#define rDDR_HPCR26										*((volatile unsigned int  *) (DDRIII_BASE + 0x268))
#define rDDR_HPCR27										*((volatile unsigned int  *) (DDRIII_BASE + 0x26C))
#define rDDR_HPCR28										*((volatile unsigned int  *) (DDRIII_BASE + 0x270))
#define rDDR_HPCR29										*((volatile unsigned int  *) (DDRIII_BASE + 0x274))
#define rDDR_HPCR30										*((volatile unsigned int  *) (DDRIII_BASE + 0x278))
#define rDDR_HPCR31										*((volatile unsigned int  *) (DDRIII_BASE + 0x27C))
#define rDDR_PQCR0										*((volatile unsigned int  *) (DDRIII_BASE + 0x280))
#define rDDR_PQCR1										*((volatile unsigned int  *) (DDRIII_BASE + 0x284))
#define rDDR_PQCR2										*((volatile unsigned int  *) (DDRIII_BASE + 0x288))
#define rDDR_PQCR3										*((volatile unsigned int  *) (DDRIII_BASE + 0x28C))
#define rDDR_PQCR4										*((volatile unsigned int  *) (DDRIII_BASE + 0x290))
#define rDDR_PQCR5										*((volatile unsigned int  *) (DDRIII_BASE + 0x294))
#define rDDR_PQCR6										*((volatile unsigned int  *) (DDRIII_BASE + 0x298))
#define rDDR_PQCR7										*((volatile unsigned int  *) (DDRIII_BASE + 0x29C))
#define rDDR_MMGCR										*((volatile unsigned int  *) (DDRIII_BASE + 0x2A0))




// DMA
// DW DMA 外设通道号（Assigns a hardware handshaking interface）
#define I2S_RX_FIFO_DMACBREQ		0 
#define I2S_TX_FIFO_DMACBREQ		1 
#define NAND_RXBREQ					2 
#define NAND_TXBREQ					3 
#define UART0_UARTRXDMABREQ		4 
#define UART0_UARTTXDMABREQ		5 
#define SDMMC_DW_DMA_REQ			6 
#define SDMMC1_DW_DMA_REQ			7 
#define SSP_RXDMABREQ				8 
#define SSP_TXDMABREQ				9 
#define UART1_UARTRXDMABREQ		10 
#define UART1_UARTTXDMABREQ		11 
#define UART2_UARTRXDMABREQ		12 
#define UART2_UARTTXDMABREQ		13 
#define UART3_UARTRXDMABREQ		14 
#define UART3_UARTTXDMABREQ		15 
#define I2C_RX_BREQ					16 
#define I2C_TX_BREQ					17 
#define I2S1_RX_FIFO_DMACBREQ		18 
#define I2S1_TX_FIFO_DMACBREQ		19 
#define I2S2_RX_FIFO_DMACBREQ		20 
#define I2S2_TX_FIFO_DMACBREQ		21 
#define SPI1_RXDMABREQ				22 
#define SPI1_TXDMABREQ				23 

#define REQ_NAND_TX					NAND_TXBREQ
#define REQ_NAND_RX					NAND_RXBREQ
#define REQ_SDMMC0					SDMMC_DW_DMA_REQ
#define REQ_SDMMC1					SDMMC1_DW_DMA_REQ
#define REQ_SSI_RX					SSP_RXDMABREQ
#define REQ_SSI_TX					SSP_TXDMABREQ
#define REQ_I2C_TX					I2C_TX_BREQ
#define REQ_I2C_RX					I2C_RX_BREQ

/*Source Address Register*/
#define rDMA_SAR0_L       		*((volatile unsigned int *)(DMA_BASE + 0x000))
#define rDMA_SAR0_H      		*((volatile unsigned int *)(DMA_BASE + 0x004))
/*Destination Address Register*/
#define rDMA_DAR0_L			*((volatile unsigned int *)(DMA_BASE + 0x008))
#define rDMA_DAR0_H		*((volatile unsigned int *)(DMA_BASE + 0x00c))
/*Linked List Pointer Register*/
#define rDMA_LLP0_L			*((volatile unsigned int*)(DMA_BASE + 0x010))
#define rDMA_LLP0_H			*((volatile unsigned int *)(DMA_BASE + 0x014))
/*Control Register*/
#define rDMA_CTL0_L			*((volatile unsigned int*)(DMA_BASE + 0x018))
#define rDMA_CTL0_H			*((volatile unsigned int*)(DMA_BASE + 0x01c))
/*Source Status Register*/
#define rDMA_SSTAT0_L		*((volatile unsigned int *)(DMA_BASE + 0x020))
#define rDMA_SSTAT0_H		*((volatile unsigned int *)(DMA_BASE + 0x024))
/*Destination Status Register*/
#define rDMA_DSTAT0_L		*((volatile unsigned int *)(DMA_BASE + 0x028))
#define rDMA_DSTAT0_H		*((volatile unsigned int *)(DMA_BASE + 0x02c))
/*Source Status Address Register*/
#define rDMA_SSTATAR0_L		*((volatile unsigned int *)(DMA_BASE + 0x030))
#define rDMA_SSTATAR0_H		*((volatile unsigned int *)(DMA_BASE + 0x034))
/*Destination Status Address Register */
#define rDMA_DSTATAR0_L		*((volatile unsigned int *)(DMA_BASE + 0x038))
#define rDMA_DSTATAR0_H		*((volatile unsigned int *)(DMA_BASE + 0x03c))
/*Configuration Register*/
#define rDMA_CFG0_L			*((volatile unsigned int *)(DMA_BASE + 0x040))
#define rDMA_CFG0_H			*((volatile unsigned int *)(DMA_BASE + 0x044))
/*Source Gather Register*/
#define rDMA_SGR0_L			*((volatile unsigned int *)(DMA_BASE + 0x048))
#define rDMA_SGR0_H			*((volatile unsigned int *)(DMA_BASE + 0x04c))
/*Destination Scatter Register*/
#define rDMA_DSR0_L			*((volatile unsigned int *)(DMA_BASE + 0x050))
#define rDMA_DSR0_H			*((volatile unsigned int *)(DMA_BASE + 0x054))


/*Source Address Register*/
#define rDMA_SAR1_L       		*((volatile unsigned int *)(DMA_BASE + 0x058))
#define rDMA_SAR1_H      		*((volatile unsigned int *)(DMA_BASE + 0x05c))
/*Destination Address Register*/
#define rDMA_DAR1_L			*((volatile unsigned int *)(DMA_BASE + 0x060))
#define rDMA_DAR1_H		*((volatile unsigned int *)(DMA_BASE + 0x064))
/*Linked List Pointer Register*/
#define rDMA_LLP1_L			*((volatile unsigned int*)(DMA_BASE + 0x068))
#define rDMA_LLP1_H			*((volatile unsigned int *)(DMA_BASE + 0x06c))
/*Control Register*/
#define rDMA_CTL1_L			*((volatile unsigned int*)(DMA_BASE + 0x070))
#define rDMA_CTL1_H			*((volatile unsigned int*)(DMA_BASE + 0x074))
/*Source Status Register*/
#define rDMA_SSTAT1_L		*((volatile unsigned int *)(DMA_BASE + 0x078))
#define rDMA_SSTAT1_H		*((volatile unsigned int *)(DMA_BASE + 0x07c))
/*Destination Status Register*/
#define rDMA_DSTAT1_L		*((volatile unsigned int *)(DMA_BASE + 0x080))
#define rDMA_DSTAT1_H		*((volatile unsigned int *)(DMA_BASE + 0x084))
/*Source Status Address Register*/
#define rDMA_SSTATAR1_L		*((volatile unsigned int *)(DMA_BASE + 0x088))
#define rDMA_SSTATAR1_H		*((volatile unsigned int *)(DMA_BASE + 0x08c))
/*Destination Status Address Register */
#define rDMA_DSTATAR1_L		*((volatile unsigned int *)(DMA_BASE + 0x090))
#define rDMA_DSTATAR1_H		*((volatile unsigned int *)(DMA_BASE + 0x094))
/*Configuration Register*/
#define rDMA_CFG1_L			*((volatile unsigned int *)(DMA_BASE + 0x098))
#define rDMA_CFG1_H			*((volatile unsigned int *)(DMA_BASE + 0x09c))
/*Source Gather Register*/
#define rDMA_SGR1_L			*((volatile unsigned int *)(DMA_BASE + 0xa0))
#define rDMA_SGR1_H			*((volatile unsigned int *)(DMA_BASE + 0x0a4))
/*Destination Scatter Register*/
#define rDMA_DSR1_L			*((volatile unsigned int *)(DMA_BASE + 0x0a8))
#define rDMA_DSR1_H			*((volatile unsigned int *)(DMA_BASE + 0x0ac))

/*Source Address Register*/
#define rDMA_SAR2_L       		*((volatile unsigned int *)(DMA_BASE + 0x0b0))
#define rDMA_SAR2_H      		*((volatile unsigned int *)(DMA_BASE + 0x0b4))
/*Destination Address Register*/
#define rDMA_DAR2_L			*((volatile unsigned int *)(DMA_BASE + 0x0b8))
#define rDMA_DAR2_H		*((volatile unsigned int *)(DMA_BASE + 0x0bc))
/*Linked List Pointer Register*/
#define rDMA_LLP2_L			*((volatile unsigned int*)(DMA_BASE + 0x0c0))
#define rDMA_LLP2_H			*((volatile unsigned int *)(DMA_BASE + 0x0c4))
/*Control Register*/
#define rDMA_CTL2_L			*((volatile unsigned int*)(DMA_BASE + 0x0c8))
#define rDMA_CTL2_H			*((volatile unsigned int*)(DMA_BASE + 0x0cc))
/*Source Status Register*/
#define rDMA_SSTAT2_L		*((volatile unsigned int *)(DMA_BASE + 0x0d0))
#define rDMA_SSTAT2_H		*((volatile unsigned int *)(DMA_BASE + 0x0d4))
/*Destination Status Register*/
#define rDMA_DSTAT2_L		*((volatile unsigned int *)(DMA_BASE + 0x0d8))
#define rDMA_DSTAT2_H		*((volatile unsigned int *)(DMA_BASE + 0x0dc))
/*Source Status Address Register*/
#define rDMA_SSTATAR2_L		*((volatile unsigned int *)(DMA_BASE + 0x0e0))
#define rDMA_SSTATAR2_H		*((volatile unsigned int *)(DMA_BASE + 0x0e4))
/*Destination Status Address Register */
#define rDMA_DSTATAR2_L		*((volatile unsigned int *)(DMA_BASE + 0x0e8))
#define rDMA_DSTATAR2_H		*((volatile unsigned int *)(DMA_BASE + 0x0ec))
/*Configuration Register*/
#define rDMA_CFG2_L			*((volatile unsigned int *)(DMA_BASE + 0x0f0))
#define rDMA_CFG2_H			*((volatile unsigned int *)(DMA_BASE + 0x0f4))
/*Source Gather Register*/
#define rDMA_SGR2_L			*((volatile unsigned int *)(DMA_BASE + 0xf8))
#define rDMA_SGR2_H			*((volatile unsigned int *)(DMA_BASE + 0x0fc))
/*Destination Scatter Register*/
#define rDMA_DSR2_L			*((volatile unsigned int *)(DMA_BASE + 0x100))
#define rDMA_DSR2_H			*((volatile unsigned int *)(DMA_BASE + 0x104))



/*Source Address Register*/
#define rDMA_SAR3_L       		*((volatile unsigned int *)(DMA_BASE + 0x108))
#define rDMA_SAR3_H      		*((volatile unsigned int *)(DMA_BASE + 0x10c))
/*Destination Address Register*/
#define rDMA_DAR3_L			*((volatile unsigned int *)(DMA_BASE + 0x110))
#define rDMA_DAR3_H		*((volatile unsigned int *)(DMA_BASE + 0x114))
/*Linked List Pointer Register*/
#define rDMA_LLP3_L			*((volatile unsigned int*)(DMA_BASE + 0x118))
#define rDMA_LLP3_H			*((volatile unsigned int *)(DMA_BASE + 0x11c))
/*Control Register*/
#define rDMA_CTL3_L			*((volatile unsigned int*)(DMA_BASE + 0x120))
#define rDMA_CTL3_H			*((volatile unsigned int*)(DMA_BASE + 0x124))
/*Source Status Register*/
#define rDMA_SSTAT3_L		*((volatile unsigned int *)(DMA_BASE + 0x128))
#define rDMA_SSTAT3_H		*((volatile unsigned int *)(DMA_BASE + 0x12c))
/*Destination Status Register*/
#define rDMA_DSTAT3_L		*((volatile unsigned int *)(DMA_BASE + 0x130))
#define rDMA_DSTAT3_H		*((volatile unsigned int *)(DMA_BASE + 0x134))
/*Source Status Address Register*/
#define rDMA_SSTATAR3_L		*((volatile unsigned int *)(DMA_BASE + 0x138))
#define rDMA_SSTATAR3_H		*((volatile unsigned int *)(DMA_BASE + 0x13c))
/*Destination Status Address Register */
#define rDMA_DSTATAR3_L		*((volatile unsigned int *)(DMA_BASE + 0x140))
#define rDMA_DSTATAR3_H		*((volatile unsigned int *)(DMA_BASE + 0x144))
/*Configuration Register*/
#define rDMA_CFG3_L			*((volatile unsigned int *)(DMA_BASE + 0x148))
#define rDMA_CFG3_H			*((volatile unsigned int *)(DMA_BASE + 0x14c))
/*Source Gather Register*/
#define rDMA_SGR3_L			*((volatile unsigned int *)(DMA_BASE + 0x150))
#define rDMA_SGR3_H			*((volatile unsigned int *)(DMA_BASE + 0x154))
/*Destination Scatter Register*/
#define rDMA_DSR3_L			*((volatile unsigned int *)(DMA_BASE + 0x158))
#define rDMA_DSR3_H			*((volatile unsigned int *)(DMA_BASE + 0x15c))

/*Source Address Register*/
#define rDMA_SARx_L(ch)   		*((volatile unsigned int *)(DMA_BASE + 0x000 + 0x58*ch))
#define rDMA_SARx_H(ch)   		*((volatile unsigned int *)(DMA_BASE + 0x004 + 0x58*ch))
/*Destination Address Register*/
#define rDMA_DARx_L(ch)			*((volatile unsigned int *)(DMA_BASE + 0x008 + 0x58*ch))
#define rDMA_DARx_H(ch)		*((volatile unsigned int *)(DMA_BASE + 0x00c + 0x58*ch))
/*Linked List Pointer Register*/
#define rDMA_LLPx_L(ch)			*((volatile unsigned int*)(DMA_BASE + 0x010 + 0x58*ch))
#define rDMA_LLPx_H(ch)			*((volatile unsigned int *)(DMA_BASE + 0x014 + 0x58*ch))
/*Control Register*/
#define rDMA_CTLx_L(ch)			*((volatile unsigned int*)(DMA_BASE + 0x018 + 0x58*ch))
#define rDMA_CTLx_H(ch)			*((volatile unsigned int*)(DMA_BASE + 0x01c + 0x58*ch))
/*Source Status Register*/
#define rDMA_SSTATx_L(ch)		*((volatile unsigned int *)(DMA_BASE + 0x020 + 0x58*ch))
#define rDMA_SSTATx_H(ch)		*((volatile unsigned int *)(DMA_BASE + 0x024 + 0x58*ch))
/*Destination Status Register*/
#define rDMA_DSTATx_L(ch)		*((volatile unsigned int *)(DMA_BASE + 0x028 + 0x58*ch))
#define rDMA_DSTATx_H(ch)		*((volatile unsigned int *)(DMA_BASE + 0x02c + 0x58*ch))
/*Source Status Address Register*/
#define rDMA_SSTATARx_L(ch)		*((volatile unsigned int *)(DMA_BASE + 0x030 + 0x58*ch))
#define rDMA_SSTATARx_H(ch)		*((volatile unsigned int *)(DMA_BASE + 0x034 + 0x58*ch))
/*Destination Status Address Register */
#define rDMA_DSTATARx_L(ch)		*((volatile unsigned int *)(DMA_BASE + 0x038 + 0x58*ch))
#define rDMA_DSTATARx_H(ch)		*((volatile unsigned int *)(DMA_BASE + 0x03c + 0x58*ch))
/*Configuration Register*/
#define rDMA_CFGx_L(ch)			*((volatile unsigned int *)(DMA_BASE + 0x040 + 0x58*ch))
#define rDMA_CFGx_H(ch)			*((volatile unsigned int *)(DMA_BASE + 0x044 + 0x58*ch))
/*Source Gather Register*/
#define rDMA_SGRx_L(ch)			*((volatile unsigned int *)(DMA_BASE + 0x048 + 0x58*ch))
#define rDMA_SGRx_H(ch)			*((volatile unsigned int *)(DMA_BASE + 0x04c + 0x58*ch))
/*Destination Scatter Register*/
#define rDMA_DSRx_L(ch)			*((volatile unsigned int *)(DMA_BASE + 0x050 + 0x58*ch))
#define rDMA_DSRx_H(ch)			*((volatile unsigned int *)(DMA_BASE + 0x054 + 0x58*ch))

/*DMA Transfer Complete Interrupt Register*/
#define rDMA_RAW_TRF_L			*((volatile unsigned int *)(DMA_BASE + 0x2c0))
#define rDMA_RAW_TRF_H		*((volatile unsigned int *)(DMA_BASE + 0x2c4))
/*Block Transfer Complete Interrupt Register*/
#define rDMA_RAW_BLOCK_L			*((volatile unsigned int *)(DMA_BASE + 0x2c8))
#define rDMA_RAW_BLOCK	_H		*((volatile unsigned int *)(DMA_BASE + 0x2cc))
/*Source Transaction Complete Interrupt Register*/
#define rDMA_RAW_SRC_TRAN_L		*((volatile unsigned int *)(DMA_BASE + 0x2d0))
#define rDMA_RAW_SRC_TRAN_H		*((volatile unsigned int *)(DMA_BASE + 0x2d4))
/*Destination Transaction Complete Interrupt Register*/
#define rDMA_RAW_DST_TRAN_L		*((volatile unsigned int *)(DMA_BASE + 0x2d8))
#define rDMA_RAW_DST_TRAN_H		*((volatile unsigned int *)(DMA_BASE + 0x2dc))
/*Error Interrupt Register*/
#define rDMA_RAW_ERR_L		*((volatile unsigned int *)(DMA_BASE + 0x2e0))
#define rDMA_RAW_ERR_H		*((volatile unsigned int *)(DMA_BASE + 0x2e4))

/*DMA Transfer Complete Interrupt Status Register*/
#define rDMA_STATUS_TRF_L		*((volatile unsigned int *)(DMA_BASE + 0x2e8))
#define rDMA_STATUS_TRF_H		*((volatile unsigned int *)(DMA_BASE + 0x2ec))
/*Block Transfer Complete Interrupt  Status Register*/
#define rDMA_STATUS_BLOCK_L		*((volatile unsigned int *)(DMA_BASE + 0x2f0))
#define rDMA_STATUS_BLOCK_H		*((volatile unsigned int *)(DMA_BASE + 0x2f4))
/*Source Transaction Complete Interrupt Status Register*/
#define rDMA_STATUS_SRC_TRAN_L		*((volatile unsigned int *)(DMA_BASE + 0x2f8))
#define rDMA_STATUS_SRC_TRAN_H		*((volatile unsigned int *)(DMA_BASE + 0x2fc))
/*Destination Transaction Complete Interrupt  Status Register*/
#define rDMA_STATUS_DST_TRAN_L		*((volatile unsigned int *)(DMA_BASE + 0x300))
#define rDMA_STATUS_DST_TRAN_H		*((volatile unsigned int *)(DMA_BASE + 0x304))
/*Error Interrupt Register*/
#define rDMA_STATUS_ERR_L		*((volatile unsigned int *)(DMA_BASE + 0x308))
#define rDMA_STATUS_ERR_H		*((volatile unsigned int *)(DMA_BASE + 0x30c))

/*DMA Transfer Complete Interrupt Mask Register*/
#define rDMA_MASK_TRF_L		*((volatile unsigned int *)(DMA_BASE + 0x310))
#define rDMA_MASK_TRF_H		*((volatile unsigned int *)(DMA_BASE + 0x314))
/*Block Transfer Complete Interrupt Mask Register*/
#define rDMA_MASK_BLOCK_L		*((volatile unsigned int *)(DMA_BASE + 0x318))
#define rDMA_MASK_BLOCK_H		*((volatile unsigned int *)(DMA_BASE + 0x31c))
/*Source Transaction Complete Interrupt Mask Register*/
#define rDMA_MASK_SRC_TRAN_L		*((volatile unsigned int *)(DMA_BASE + 0x320))
#define rDMA_MASK_SRC_TRAN_H		*((volatile unsigned int *)(DMA_BASE + 0x324))
/*Destination Transaction Complete Interrupt Mask Register*/
#define rDMA_MASK_DST_TRAN_L		*((volatile unsigned int *)(DMA_BASE + 0x328))
#define rDMA_MASK_DST_TRAN_H		*((volatile unsigned int *)(DMA_BASE + 0x32c))
/*Error Interrupt  Mask Register*/
#define rDMA_MASK_ERR_L		*((volatile unsigned int *)(DMA_BASE + 0x330))
#define rDMA_MASK_ERR_H		*((volatile unsigned int *)(DMA_BASE + 0x334))

/*DMA Transfer Complete Interrupt Clear Register*/
#define rDMA_CLEAR_TRF_L		*((volatile unsigned int *)(DMA_BASE + 0x338))
#define rDMA_CLEAR_TRF_H		*((volatile unsigned int *)(DMA_BASE + 0x33c))
/*Block Transfer Complete Interrupt Clear Register*/
#define rDMA_CLEAR_BLOCK_L		*((volatile unsigned int *)(DMA_BASE + 0x340))
#define rDMA_CLEAR_BLOCK_H		*((volatile unsigned int *)(DMA_BASE + 0x344))
/*Source Transaction Complete Interrupt Clear Register*/
#define rDMA_CLEAR_SRC_TRAN_L		*((volatile unsigned int *)(DMA_BASE + 0x348))
#define rDMA_CLEAR_SRC_TRAN_H		*((volatile unsigned int *)(DMA_BASE + 0x34c))
/*Destination Transaction Complete Interrupt Clear Register*/
#define rDMA_CLEAR_DST_TRAN_L		*((volatile unsigned int *)(DMA_BASE + 0x350))
#define rDMA_CLEAR_DST_TRAN_H		*((volatile unsigned int *)(DMA_BASE + 0x354))
/*Error Interrupt  Clear Register*/
#define rDMA_CLEAR_ERR_L		*((volatile unsigned int *)(DMA_BASE + 0x358))
#define rDMA_CLEAR_ERR_H		*((volatile unsigned int *)(DMA_BASE + 0x35c))

/*Combined Interrupt Status Register*/
#define rDMA_STATUS_INT_L      *((volatile unsigned int *)(DMA_BASE + 0x360))
#define rDMA_STATUS_INT_H      *((volatile unsigned int *)(DMA_BASE + 0x364))

/*Software Transaction Request Register*/
#define rDMA_REQ_SRC_L		 *((volatile unsigned int *)(DMA_BASE + 0x368))
#define rDMA_REQ_SRC_H		 *((volatile unsigned int *)(DMA_BASE + 0x36c))

/*Destination Software Transaction Request Register*/
#define rDMA_REQ_DST_L       	*((volatile unsigned int *)(DMA_BASE + 0x370))
#define rDMA_REQ_DST_H       	*((volatile unsigned int *)(DMA_BASE + 0x374))

/*Single Source Transaction Request Register*/
#define rDMA_SGL_REQ_SRC_L		*((volatile unsigned int *)(DMA_BASE + 0x378))
#define rDMA_SGL_REQ_SRC_H		*((volatile unsigned int *)(DMA_BASE + 0x37c))

/*Single Destination Transaction Request Register*/
#define rDMA_SGL_REQ_DST_L		*((volatile unsigned int *)(DMA_BASE + 0x380))
#define rDMA_SGL_REQ_DST_H		*((volatile unsigned int *)(DMA_BASE + 0x384))

/*Last Source Transaction Request Register*/
#define rDMA_LST_SRC_REQ_L		*((volatile unsigned int *)(DMA_BASE + 0x388))
#define rDMA_LST_SRC_REQ_H		*((volatile unsigned int *)(DMA_BASE + 0x38c))

/*Last Destination Transaction Request Register*/
#define rDMA_LST_DST_REQ_L		*((volatile unsigned int *)(DMA_BASE + 0x390))
#define rDMA_LST_DST_REQ_H		*((volatile unsigned int *)(DMA_BASE + 0x394))


/*DW_ahb_dmac Configuration Register*/
#define rDMA_CFG_L                    *((volatile unsigned int *)(DMA_BASE + 0x398))	
#define rDMA_CFG_H                    *((volatile unsigned int *)(DMA_BASE + 0x39c))
/*DW_ahb_dmac Channel Enable Register*/
#define rDMA_CHEN_L            	*((volatile unsigned int *)(DMA_BASE + 0x3a0))
#define rDMA_CHEN_H            	*((volatile unsigned int *)(DMA_BASE + 0x3a4))


/*DW_ahb_dmac ID Register*/
#define rDMA_ID_L					*((volatile unsigned int *)(DMA_BASE + 0x3a8))
#define rDMA_ID_H					*((volatile unsigned int *)(DMA_BASE + 0x3ac))

/*DW_ahb_dmac Test Register*/
#define rDMA_TEST_L				*((volatile unsigned int *)(DMA_BASE + 0x3b0))
#define rDMA_TEST_H				*((volatile unsigned int *)(DMA_BASE + 0x3b4))

/*DW_ahb_dmac Component Parameters Register 6*/
#define rDMA_COMP_PARAMS_6_L	*((volatile unsigned int *)(DMA_BASE + 0x3c8))
#define rDMA_COMP_PARAMS_6_H	*((volatile unsigned int  *)(DMA_BASE + 0x3cc))

/*DW_ahb_dmac Component Parameters Register 5*/
#define rDMA_COMP_PARAMS_5_L	*((volatile unsigned int  *)(DMA_BASE + 0x3d0))
#define rDMA_COMP_PARAMS_5_H	*((volatile unsigned int *)(DMA_BASE + 0x3d4))
#define rDMA_COMP_PARAMS_4_L	*((volatile unsigned int *)(DMA_BASE + 0x3d8))
#define rDMA_COMP_PARAMS_4_H	*((volatile unsigned int *)(DMA_BASE + 0x3dc))
#define rDMA_COMP_PARAMS_3_L	*((volatile unsigned int *)(DMA_BASE + 0x3e0))
#define rDMA_COMP_PARAMS_3_H	*((volatile unsigned int  *)(DMA_BASE + 0x3e4))
#define rDMA_COMP_PARAMS_2_L	*((volatile unsigned int  *)(DMA_BASE + 0x3e8))
#define rDMA_COMP_PARAMS_2_H	*((volatile unsigned int *)(DMA_BASE + 0x3ec))
#define rDMA_COMP_PARAMS_1_L	*((volatile unsigned int  *)(DMA_BASE + 0x3f0))

#define rDMA_COMP_PARAMS_1_H	*((volatile unsigned int *)(DMA_BASE + 0x3f4))


/////////////////////////////////////////////////////
/* MAC */
#define rMAC1					*((volatile unsigned int *)(MAC_BASE + 0x000))    
#define rMAC2					*((volatile unsigned int *)(MAC_BASE + 0x004))    
#define rIPGT					*((volatile unsigned int *)(MAC_BASE + 0x008))    
#define rIPGR					*((volatile unsigned int *)(MAC_BASE + 0x00c))    
#define rCLRT					*((volatile unsigned int *)(MAC_BASE + 0x010))
#define rMAXF					*((volatile unsigned int *)(MAC_BASE + 0x014))
#define rSUPP					*((volatile unsigned int *)(MAC_BASE + 0x018))    
#define rTEST					*((volatile unsigned int *)(MAC_BASE + 0x01c))
#define rMCFG					*((volatile unsigned int *)(MAC_BASE + 0x020))
#define rMCMD					*((volatile unsigned int *)(MAC_BASE + 0x024))
#define rMADR					*((volatile unsigned int *)(MAC_BASE + 0x028))
#define rMWTD					*((volatile unsigned int *)(MAC_BASE + 0x02c))
#define rMRDD					*((volatile unsigned int *)(MAC_BASE + 0x030))    
#define rMIND					*((volatile unsigned int *)(MAC_BASE + 0x034))
#define rMACFIFOCFG0			*((volatile unsigned int *)(MAC_BASE + 0x03c))
#define rMACADDR1				*((volatile unsigned int *)(MAC_BASE + 0x040))    
#define rMACADDR2				*((volatile unsigned int *)(MAC_BASE + 0x044))    
#define rMACADDR3				*((volatile unsigned int *)(MAC_BASE + 0x048))    
#define rMACFIFOCFG1			*((volatile unsigned int *)(MAC_BASE + 0x04c))
#define rMACFIFOCFG2			*((volatile unsigned int *)(MAC_BASE + 0x050))
#define rMACFIFOCFG3			*((volatile unsigned int *)(MAC_BASE + 0x054))
#define rMACFIFOCFG4			*((volatile unsigned int *)(MAC_BASE + 0x058))
#define rMACFIFOCFG5			*((volatile unsigned int *)(MAC_BASE + 0x05c))
#define rMACFIFORAM0			*((volatile unsigned int *)(MAC_BASE + 0x060))
#define rMACFIFORAM1			*((volatile unsigned int *)(MAC_BASE + 0x064))
#define rMACFIFORAM2			*((volatile unsigned int *)(MAC_BASE + 0x068))
#define rMACFIFORAM3			*((volatile unsigned int *)(MAC_BASE + 0x06c))
#define rMACFIFORAM4			*((volatile unsigned int *)(MAC_BASE + 0x070))
#define rMACFIFORAM5			*((volatile unsigned int *)(MAC_BASE + 0x074))
#define rMACFIFORAM6			*((volatile unsigned int *)(MAC_BASE + 0x078))
#define rMACFIFORAM7			*((volatile unsigned int *)(MAC_BASE + 0x07c))
#define rDMATxCtrl				*((volatile unsigned int *)(MAC_BASE + 0x180))    
#define rDMATxDescriptor		*((volatile unsigned int *)(MAC_BASE + 0x184))
#define rDMATxStatus			*((volatile unsigned int *)(MAC_BASE + 0x188))
#define rDMARxCtrl				*((volatile unsigned int *)(MAC_BASE + 0x18c))
#define rDMARxDescriptor		*((volatile unsigned int *)(MAC_BASE + 0x190))
#define rDMARxStatus   			*((volatile unsigned int *)(MAC_BASE + 0x194))
#define rDMAIntrMask   			*((volatile unsigned int *)(MAC_BASE + 0x198))
#define rDMAInterrupt   		*((volatile unsigned int *)(MAC_BASE + 0x19c))

/* LCD */
#define  LCD_TVOUT_PARAM0_reg      (*(volatile unsigned *)(LCD_BASE + 60 * 4))
#define  LCD_TVOUT_PARAM1_reg      (*(volatile unsigned *)(LCD_BASE + 61 * 4))
#define  LCD_TVOUT_PARAM2_reg      (*(volatile unsigned *)(LCD_BASE + 62 * 4))
#define  LCD_TVOUT_PARAM3_reg      (*(volatile unsigned *)(LCD_BASE + 63 * 4))
#define  LCD_TVOUT_PARAM4_reg      (*(volatile unsigned *)(LCD_BASE + 64 * 4))
#define  LCD_TVOUT_PARAM5_reg      (*(volatile unsigned *)(LCD_BASE + 65 * 4))
#define  LCD_TVOUT_PARAM6_reg      (*(volatile unsigned *)(LCD_BASE + 66 * 4))
#define  LCD_TVOUT_PARAM7_reg      (*(volatile unsigned *)(LCD_BASE + 67 * 4))
#define  LCD_TVOUT_PARAM8_reg      (*(volatile unsigned *)(LCD_BASE + 68 * 4))
#define  LCD_TVOUT_PARAM9_reg      (*(volatile unsigned *)(LCD_BASE + 69 * 4))
#define  LCD_TVOUT_PARAM10_reg     (*(volatile unsigned *)(LCD_BASE + 70 * 4))
#define  LCD_PARAM0_reg            (*(volatile unsigned *)(LCD_BASE + 0 * 4))
#define  LCD_PARAM1_reg            (*(volatile unsigned *)(LCD_BASE + 1 * 4))
#define  LCD_PARAM2_reg            (*(volatile unsigned *)(LCD_BASE + 2 * 4))
#define  LCD_PARAM3_reg            (*(volatile unsigned *)(LCD_BASE + 3 * 4))
#define  LCD_PARAM4_reg            (*(volatile unsigned *)(LCD_BASE + 4 * 4))
#define  LCD_PARAM5_reg            (*(volatile unsigned *)(LCD_BASE + 5 * 4))
#define  LCD_PARAM6_reg            (*(volatile unsigned *)(LCD_BASE + 6 * 4))
#define  LCD_PARAM7_reg            (*(volatile unsigned *)(LCD_BASE + 7 * 4))
#define  LCD_PARAM8_reg            (*(volatile unsigned *)(LCD_BASE + 8 * 4))
#define  LCD_PARAM9_reg            (*(volatile unsigned *)(LCD_BASE + 9 * 4))
#define  LCD_PARAM10_reg           (*(volatile unsigned *)(LCD_BASE + 10 * 4))
#define  LCD_PARAM11_reg           (*(volatile unsigned *)(LCD_BASE + 11 * 4))
#define  LCD_PARAM12_reg           (*(volatile unsigned *)(LCD_BASE + 12 * 4))
#define  LCD_PARAM13_reg           (*(volatile unsigned *)(LCD_BASE + 13 * 4))
#define  LCD_PARAM14_reg           (*(volatile unsigned *)(LCD_BASE + 14 * 4))
#define  LCD_PARAM15_reg           (*(volatile unsigned *)(LCD_BASE + 15 * 4))
#define  LCD_PARAM16_reg           (*(volatile unsigned *)(LCD_BASE + 16 * 4))
#define  LCD_PARAM17_reg           (*(volatile unsigned *)(LCD_BASE + 17 * 4))
#define  LCD_PARAM18_reg           (*(volatile unsigned *)(LCD_BASE + 18 * 4))
#define  LCD_PARAM19_reg           (*(volatile unsigned *)(LCD_BASE + 19 * 4))
#define  LCD_PARAM20_reg           (*(volatile unsigned *)(LCD_BASE + 20 * 4))
#define  LCD_PARAM21_reg           (*(volatile unsigned *)(LCD_BASE + 21 * 4))
#define  LCD_PARAM22_reg           (*(volatile unsigned *)(LCD_BASE + 22 * 4))

//OSD_0
#define  LCD_OSD_BASE               (LCD_BASE + 23 * 4)
#define  LCD_OSD_0_PARAM0_reg      (*(volatile unsigned *)(LCD_BASE + 23 * 4))//0x5c
#define  LCD_OSD_0_PARAM1_reg      (*(volatile unsigned *)(LCD_BASE + 24 * 4))
//#define  LCD_OSD_ADDR               (LCD_BASE + 25 * 4)
#define  LCD_OSD_0_PARAM2_reg      (*(volatile unsigned *)(LCD_BASE + 25 * 4))
#define  LCD_OSD_0_PARAM3_reg      (*(volatile unsigned *)(LCD_BASE + 26 * 4))
#define  LCD_OSD_0_PARAM4_reg      (*(volatile unsigned *)(LCD_BASE + 27 * 4))
#define  LCD_OSD_0_PARAM5_reg      (*(volatile unsigned *)(LCD_BASE + 28 * 4))//add
//OSD_1
#define  LCD_OSD_1_PARAM0_reg      (*(volatile unsigned *)(LCD_BASE + 29 * 4))//0x74
#define  LCD_OSD_1_PARAM1_reg      (*(volatile unsigned *)(LCD_BASE + 30 * 4))
#define  LCD_OSD_1_PARAM2_reg      (*(volatile unsigned *)(LCD_BASE + 31 * 4))
#define  LCD_OSD_1_PARAM3_reg      (*(volatile unsigned *)(LCD_BASE + 32 * 4))
#define  LCD_OSD_1_PARAM4_reg      (*(volatile unsigned *)(LCD_BASE + 33 * 4))
#define  LCD_OSD_1_PARAM5_reg      (*(volatile unsigned *)(LCD_BASE + 34 * 4))
//OSD_2
#define  LCD_OSD_2_PARAM0_reg      (*(volatile unsigned *)(LCD_BASE + 35 * 4))//0x8c
#define  LCD_OSD_2_PARAM1_reg      (*(volatile unsigned *)(LCD_BASE + 36 * 4))
#define  LCD_OSD_2_PARAM2_reg      (*(volatile unsigned *)(LCD_BASE + 37 * 4))
#define  LCD_OSD_2_PARAM3_reg      (*(volatile unsigned *)(LCD_BASE + 38 * 4))
#define  LCD_OSD_2_PARAM4_reg      (*(volatile unsigned *)(LCD_BASE + 39 * 4))
#define  LCD_OSD_2_PARAM5_reg      (*(volatile unsigned *)(LCD_BASE + 40 * 4))

#define	OSD_0_PARAM0_rd			  (*(volatile unsigned *)(LCD_BASE + 0xA4))	// Osd_0_y_addr_rd 当前读取的Y地址。
#define	OSD_0_PARAM1_rd			  (*(volatile unsigned *)(LCD_BASE + 0xA8))	// Osd_0_u_addr_rd 当前读取的U地址
#define	OSD_0_PARAM2_rd			  (*(volatile unsigned *)(LCD_BASE + 0xAC))	// Osd_0_v_addr_rd 当前读取的V地址。

#define	OSD_1_PARAM0_rd			  (*(volatile unsigned *)(LCD_BASE + 0xB0))	// Osd_1_y_addr_rd 当前读取的Y地址。
#define	OSD_1_PARAM1_rd			  (*(volatile unsigned *)(LCD_BASE + 0xB4))	// Osd_1_u_addr_rd 当前读取的U地址
#define	OSD_1_PARAM2_rd			  (*(volatile unsigned *)(LCD_BASE + 0xB8))	// Osd_1_v_addr_rd 当前读取的V地址。

#define	OSD_2_PARAM0_rd			  (*(volatile unsigned *)(LCD_BASE + 0xBC))	// Osd_2_y_addr_rd 当前读取的Y地址。
#define	OSD_2_PARAM1_rd			  (*(volatile unsigned *)(LCD_BASE + 0xC0))	// Osd_2_u_addr_rd 当前读取的U地址
#define	OSD_2_PARAM2_rd			  (*(volatile unsigned *)(LCD_BASE + 0xC4))	// Osd_2_v_addr_rd 当前读取的V地址。

#define  LCD_INTR_CLR_reg               (*(volatile unsigned *)(LCD_BASE + 50 * 4))// 0xc8
#define  LCD_STATUS_reg                 (*(volatile unsigned *)(LCD_BASE + 51 * 4))// 0xcc



#define  LCD_TV_PARAM0_reg         (*(volatile unsigned *)(LCD_BASE + 54 * 4))
#define  LCD_TV_PARAM1_reg         (*(volatile unsigned *)(LCD_BASE + 55 * 4))
#define  LCD_TV_PARAM2_reg         (*(volatile unsigned *)(LCD_BASE + 56 * 4))
#define  LCD_TV_PARAM3_reg         (*(volatile unsigned *)(LCD_BASE + 57 * 4))
#define  LCD_TV_PARAM4_reg         (*(volatile unsigned *)(LCD_BASE + 58 * 4))
#define  LCD_TV_PARAM5_reg         (*(volatile unsigned *)(LCD_BASE + 59 * 4))
#define  LCD_TV_PARAM6_reg         (*(volatile unsigned *)(LCD_BASE + 60 * 4))
#define  LCD_TV_PARAM7_reg         (*(volatile unsigned *)(LCD_BASE + 61 * 4))
#define  LCD_TV_PARAM8_reg         (*(volatile unsigned *)(LCD_BASE + 62 * 4))
#define  LCD_TV_PARAM9_reg         (*(volatile unsigned *)(LCD_BASE + 63 * 4))
#define  LCD_TV_PARAM10_reg        (*(volatile unsigned *)(LCD_BASE + 64 * 4))
#define  LCD_TV_PARAM11_reg        (*(volatile unsigned *)(LCD_BASE + 65 * 4))
#define  LCD_TV_PARAM12_reg        (*(volatile unsigned *)(LCD_BASE + 66 * 4))
#define  LCD_TV_PARAM13_reg        (*(volatile unsigned *)(LCD_BASE + 67 * 4))
#define  LCD_TV_PARAM14_reg        (*(volatile unsigned *)(LCD_BASE + 68 * 4))
#define  LCD_TV_PARAM15_reg        (*(volatile unsigned *)(LCD_BASE + 69 * 4))
#define  LCD_TV_PARAM16_reg        (*(volatile unsigned *)(LCD_BASE + 70 * 4))
#define  LCD_TV_PARAM17_reg        (*(volatile unsigned *)(LCD_BASE + 71 * 4))
#define  LCD_TV_PARAM18_reg        (*(volatile unsigned *)(LCD_BASE + 72 * 4))
#define  LCD_TV_PARAM19_reg        (*(volatile unsigned *)(LCD_BASE + 73 * 4))
#define  LCD_TV_PARAM20_reg        (*(volatile unsigned *)(LCD_BASE + 74 * 4))
#define  LCD_TV_PARAM21_reg        (*(volatile unsigned *)(LCD_BASE + 75 * 4))

#define  LCD_R_BIT_ORDER           (*(volatile unsigned *)(LCD_BASE + 76 * 4))
#define  LCD_G_BIT_ORDER           (*(volatile unsigned *)(LCD_BASE + 77 * 4))
#define  LCD_B_BIT_ORDER           (*(volatile unsigned *)(LCD_BASE + 78 * 4))
#define	LCD_GAMMA_REG_0           (*(volatile unsigned *)(LCD_BASE + 79 * 4))			
#define	LCD_GAMMA_REG_1           (*(volatile unsigned *)(LCD_BASE + 80 * 4))			
#define	LCD_GAMMA_REG_2           (*(volatile unsigned *)(LCD_BASE + 81 * 4))			
#define	LCD_GAMMA_REG_3           (*(volatile unsigned *)(LCD_BASE + 82 * 4))			
#define	LCD_GAMMA_REG_4           (*(volatile unsigned *)(LCD_BASE + 83 * 4))			
#define	LCD_GAMMA_REG_5           (*(volatile unsigned *)(LCD_BASE + 84 * 4))			
#define	LCD_GAMMA_REG_6           (*(volatile unsigned *)(LCD_BASE + 85 * 4))			
#define	LCD_GAMMA_REG_7           (*(volatile unsigned *)(LCD_BASE + 86 * 4))			
#define	LCD_GAMMA_REG_8           (*(volatile unsigned *)(LCD_BASE + 87 * 4))			
#define	LCD_GAMMA_REG_9           (*(volatile unsigned *)(LCD_BASE + 88 * 4))			
#define	LCD_GAMMA_REG_10           (*(volatile unsigned *)(LCD_BASE + 89 * 4))			
#define	LCD_GAMMA_REG_11           (*(volatile unsigned *)(LCD_BASE + 90 * 4))			
#define	LCD_GAMMA_REG_12           (*(volatile unsigned *)(LCD_BASE + 91 * 4))			
#define	LCD_GAMMA_REG_13           (*(volatile unsigned *)(LCD_BASE + 92 * 4))			
#define	LCD_GAMMA_REG_14           (*(volatile unsigned *)(LCD_BASE + 93 * 4))			
#define	LCD_GAMMA_REG_15           (*(volatile unsigned *)(LCD_BASE + 94 * 4))			
#define	LCD_GAMMA_REG_16           (*(volatile unsigned *)(LCD_BASE + 95 * 4))			
#define	LCD_GAMMA_REG_17           (*(volatile unsigned *)(LCD_BASE + 96 * 4))			
#define	LCD_GAMMA_REG_18           (*(volatile unsigned *)(LCD_BASE + 97 * 4))			
#define	LCD_GAMMA_REG_19           (*(volatile unsigned *)(LCD_BASE + 98 * 4))			
#define	LCD_GAMMA_REG_20           (*(volatile unsigned *)(LCD_BASE + 99 * 4))			
#define	LCD_GAMMA_REG_21           (*(volatile unsigned *)(LCD_BASE + 100 * 4))			
#define	LCD_GAMMA_REG_22           (*(volatile unsigned *)(LCD_BASE + 101 * 4))			
#define	LCD_GAMMA_REG_23           (*(volatile unsigned *)(LCD_BASE + 102 * 4))			
#define	LCD_GAMMA_REG_24           (*(volatile unsigned *)(LCD_BASE + 103 * 4))			
#define	LCD_GAMMA_REG_25           (*(volatile unsigned *)(LCD_BASE + 104 * 4))			
#define	LCD_GAMMA_REG_26           (*(volatile unsigned *)(LCD_BASE + 105 * 4))			
#define	LCD_GAMMA_REG_27           (*(volatile unsigned *)(LCD_BASE + 106 * 4))			
#define	LCD_GAMMA_REG_28           (*(volatile unsigned *)(LCD_BASE + 107 * 4))			
#define	LCD_GAMMA_REG_29           (*(volatile unsigned *)(LCD_BASE + 108 * 4))			
#define	LCD_GAMMA_REG_30           (*(volatile unsigned *)(LCD_BASE + 109 * 4))			
#define	LCD_GAMMA_REG_31           (*(volatile unsigned *)(LCD_BASE + 110 * 4))			
#define	LCD_GAMMA_REG_32           (*(volatile unsigned *)(LCD_BASE + 111 * 4))			
#define	LCD_GAMMA_REG_33           (*(volatile unsigned *)(LCD_BASE + 112 * 4))			
#define	LCD_GAMMA_REG_34           (*(volatile unsigned *)(LCD_BASE + 113 * 4))			
#define	LCD_GAMMA_REG_35           (*(volatile unsigned *)(LCD_BASE + 114 * 4))			
#define	LCD_GAMMA_REG_36           (*(volatile unsigned *)(LCD_BASE + 115 * 4))			
#define	LCD_GAMMA_REG_37           (*(volatile unsigned *)(LCD_BASE + 116 * 4))			
#define	LCD_GAMMA_REG_38           (*(volatile unsigned *)(LCD_BASE + 117 * 4))			
#define	LCD_GAMMA_REG_39           (*(volatile unsigned *)(LCD_BASE + 118 * 4))			
#define	LCD_GAMMA_REG_40           (*(volatile unsigned *)(LCD_BASE + 119 * 4))			
#define	LCD_GAMMA_REG_41           (*(volatile unsigned *)(LCD_BASE + 120 * 4))			
#define	LCD_GAMMA_REG_42           (*(volatile unsigned *)(LCD_BASE + 121 * 4))			
#define	LCD_GAMMA_REG_43           (*(volatile unsigned *)(LCD_BASE + 122 * 4))			
#define	LCD_GAMMA_REG_44           (*(volatile unsigned *)(LCD_BASE + 123 * 4))			
#define	LCD_GAMMA_REG_45           (*(volatile unsigned *)(LCD_BASE + 124 * 4))			
#define	LCD_GAMMA_REG_46           (*(volatile unsigned *)(LCD_BASE + 125 * 4))			
#define	LCD_GAMMA_REG_47           (*(volatile unsigned *)(LCD_BASE + 126 * 4))			
#define	LCD_GAMMA_REG_48           (*(volatile unsigned *)(LCD_BASE + 127 * 4))			// 0x1FC

#define  VP_RGB2YCBCR_COEF0_reg    (*(volatile unsigned *)(LCD_BASE + 128 * 4))//0x200=512
#define  VP_RGB2YCBCR_COEF1_reg    (*(volatile unsigned *)(LCD_BASE + 129 * 4))
#define  VP_RGB2YCBCR_COEF2_reg    (*(volatile unsigned *)(LCD_BASE + 130 * 4))
#define  VP_ADJUSTMent_reg    	(*(volatile unsigned *)(LCD_BASE + 131 * 4))//0x200=512
#define  VP_CONTROL_reg            (*(volatile unsigned *)(LCD_BASE + 132 * 4))//0x210=528
#define  LCD_dithing_reg           (*(volatile unsigned *)(LCD_BASE + 133 * 4))//0x214=532
#define  LCD_srgb_cfg_reg          (*(volatile unsigned *)(LCD_BASE + 134 * 4))
#define  rSRGB_CFG  						(*(volatile unsigned *)(LCD_BASE + 134 * 4))
#define  rCPU_SCR_SOFT_REG             (*(volatile unsigned *)(LCD_BASE + 0x021C ))
#define  rCPU_SCR_CTRL_REG             (*(volatile unsigned *)(LCD_BASE + 0x0220 ))
#define  rCPU_SCR_DATA_REGBASE        ( LCD_BASE + 0x0224 )

#define  LCD_cpu_screen_cfg_reg    (*(volatile unsigned *)(LCD_BASE + 136 * 4))
#define  LCD_cpu_screen_clr0_reg   (*(volatile unsigned *)(LCD_BASE + 137 * 4))
#define  LCD_cpu_screen_clr1_reg    (*(volatile unsigned *)(LCD_BASE + 138 * 4))
#define  LCD_cpu_screen_clr2_reg    (*(volatile unsigned *)(LCD_BASE + 139 * 4))
#define  LCD_cpu_screen_clr3_reg    (*(volatile unsigned *)(LCD_BASE + 140 * 4))
#define  LCD_cpu_screen_clr4_reg    (*(volatile unsigned *)(LCD_BASE + 141 * 4))
#define  LCD_cpu_screen_clr5_reg    (*(volatile unsigned *)(LCD_BASE + 142 * 4))
#define  LCD_cpu_screen_clr6_reg    (*(volatile unsigned *)(LCD_BASE + 143 * 4))
#define  LCD_cpu_screen_clr7_reg    (*(volatile unsigned *)(LCD_BASE + 144 * 4))
#define  LCD_cpu_screen_clr8_reg    (*(volatile unsigned *)(LCD_BASE + 145 * 4))
#define  LCD_cpu_screen_clr9_reg    (*(volatile unsigned *)(LCD_BASE + 146 * 4))
#define  LCD_cpu_screen_clra_reg    (*(volatile unsigned *)(LCD_BASE + 147 * 4))
#define  LCD_cpu_screen_clrb_reg    (*(volatile unsigned *)(LCD_BASE + 148 * 4))
#define  LCD_cpu_screen_clrc_reg    (*(volatile unsigned *)(LCD_BASE + 149 * 4))
#define  LCD_cpu_screen_clrd_reg    (*(volatile unsigned *)(LCD_BASE + 150 * 4))
#define  LCD_cpu_screen_clre_reg    (*(volatile unsigned *)(LCD_BASE + 151 * 4))
#define  LCD_cpu_screen_clrf_reg    (*(volatile unsigned *)(LCD_BASE + 152 * 4))

/*配置YUV420数据是UV分开还是交织在一起  ；RGB顺序的排列，在做blending之前  */
#define  LCD_OSD_YUV420_config_reg   (*(volatile unsigned *)(LCD_BASE + 154 * 4)) // 0x268: 616 =154*4

// 最低位写入1, 在下一帧开始刷新时(场同步信号到达时)，
// LCDC将OSD控制寄存器的所有内容刷新到内部的OSD控制寄存器
#define  LCD_cpu_osd_coef_syn_reg   (*(volatile unsigned *)(LCD_BASE + 155 * 4))//0x26c
#define  LCD_vsyn_pos_num           (*(volatile unsigned *)(LCD_BASE + 156 * 4))//0x270




#define rAXI_SCALE_EN     					*((volatile unsigned int *)(SCALE_BASE + 0x0))
#define rAXI_SCALE_WB_START				*((volatile unsigned int *)(SCALE_BASE + 0x4))
#define rAXI_SCALE_CONTROL	      		*((volatile unsigned int *)(SCALE_BASE + 0x8))

#define rAXI_SCALE_VIDEO_ADDR1			*((volatile unsigned int *)(SCALE_BASE + 0xC))
#define rAXI_SCALE_VIDEO_ADDR2	      *((volatile unsigned int *)(SCALE_BASE + 0x10))
#define rAXI_SCALE_VIDEO_ADDR3	      *((volatile unsigned int *)(SCALE_BASE + 0x14))
#define rAXI_SCALE_VIDEO_SOURCE_SIZE   *((volatile unsigned int *)(SCALE_BASE + 0x18))
#define rAXI_SCALE_VIDEO_WINDOW_POINT  *((volatile unsigned int *)(SCALE_BASE + 0x1c))
#define rAXI_SCALE_VIDEO_WINDOW_SIZE   *((volatile unsigned int *)(SCALE_BASE + 0x20))

#define rAXI_SCALE_VIDEO_SIZE				*((volatile unsigned int *)(SCALE_BASE + 0x24))
#define rAXI_SCALE_SCALE_CTL				*((volatile unsigned int *)(SCALE_BASE + 0x28))
#define rAXI_SCALE_SCALE_VXMOD			*((volatile unsigned int *)(SCALE_BASE + 0x2c))
#define rAXI_SCALE_SCALE_CTL0		      *((volatile unsigned int *)(SCALE_BASE + 0x30))
#define rAXI_SCALE_SCALE_CTL1          *((volatile unsigned int *)(SCALE_BASE + 0x34))

#define rAXI_SCALE_RIGHT_BOTTOM_CUT_NUM	*((volatile unsigned int *)(SCALE_BASE + 0x38))
#define rAXI_SCALE_SCALE_CTL2				*((volatile unsigned int *)(SCALE_BASE + 0x3c))

#define rAXI_SCALE_SCALE_CTL3				*((volatile unsigned int *)(SCALE_BASE + 0x40))
#define rAXI_SCALE_SCALE_CTL4				*((volatile unsigned int *)(SCALE_BASE + 0x44))
#define rAXI_SCALE_HSCAL_COS_VALUE		*((volatile unsigned int *)(SCALE_BASE + 0x48))
#define rAXI_SCALE_WB_DEST_YADDR			*((volatile unsigned int *)(SCALE_BASE + 0x4c))

#define rAXI_SCALE_WB_DEST_UADDR			*((volatile unsigned int *)(SCALE_BASE + 0x50))
#define rAXI_SCALE_WB_DEST_VADDR			*((volatile unsigned int *)(SCALE_BASE + 0x54))
#define rAXI_SCALE_WB_DATA_HSIZE_RAM	*((volatile unsigned int *)(SCALE_BASE + 0x58))
#define rAXI_SCALE_RESERVED				*((volatile unsigned int *)(SCALE_BASE + 0x5c))

#define rAXI_SCALE_INT_CTL					*((volatile unsigned int *)(SCALE_BASE + 0x60))
#define rAXI_SCALE_INT_STATUS				*((volatile unsigned int *)(SCALE_BASE + 0x64))
#define rAXI_SCALE_CLCD_INT_CLR			*((volatile unsigned int *)(SCALE_BASE + 0x68))
#define rAXI_SCALE_WR_Y_VCNT				*((volatile unsigned int *)(SCALE_BASE + 0x6C))
//目前只支持Y-UV420 UV数据分开
//#define rAXI_SCALE_WR_UV_VCNT				*((volatile unsigned int *)(SCALE_BASE + 0x70))

/* USB OTG */
//------Common USB Registers------//
#define USB_FADDR						*((BYTE	 volatile *)(USB_BASE + 0x00))	//peripheral mode only
#define USB_POWER						*((BYTE  volatile *)(USB_BASE + 0x01))	//[iso_update][soft_conn][hs_enab][hs_mode][reset][resume][suspend_mode][en_suspendm]
#define USB_INTRTX						*((HWORD volatile *)(USB_BASE + 0x02))	//[ep15:ep0]
#define USB_INTRRX						*((HWORD volatile *)(USB_BASE + 0x04))	//[ep15:ep1][0]
#define USB_INTRTXE						*((HWORD volatile *)(USB_BASE + 0x06))	//[ep15:ep0]
#define USB_INTRRXE						*((HWORD volatile *)(USB_BASE + 0x08))	//[ep15:ep1][0]
#define USB_INTRUSB						*((BYTE  volatile *)(USB_BASE + 0x0a))	//[vbus_error][sess_req][discon][conn][sof][reset(babble)][resume][suspend]
#define USB_INTRUSBE					*((BYTE  volatile *)(USB_BASE + 0x0b))	//[vbus_error][sess_req][discon][conn][sof][reset(babble)][resume][suspend] usb interrupton enable
#define USB_FRAME						*((HWORD volatile *)(USB_BASE + 0x0c))	//[15:11][frame_number10:0]
#define USB_INDEX						*((BYTE  volatile *)(USB_BASE + 0x0e))	//[7:4][selected_endpoint3:0]  status and control register in which endpoint will be accessed
#define USB_TESTMODE					*((BYTE  volatile *)(USB_BASE + 0x0f))	//[force_host][fifo_access][force_fs][force_hs][test_packet][test_k][test_j][test_se0_nak]

//------Indexed Registers------//
#define USB_TXMAXP						*((HWORD volatile *)(USB_BASE + 0x10))	//[m-1][max_payload/transaction10:0]
#define USB_TXCSR						*((HWORD volatile *)(USB_BASE + 0x12))	//HostMode:[AutoSet][14][Mode][M2MDMAReqEn][FrcDataTog][DMAReqMode][DataTogWrEn][DataTog]
					  	//[NAKTimeout][ClrDataTog][RxStall][SetupPkt][FlushFIFO][Error][FIFONotEmpty][TxPktRdy]
#define USB_RXMAXP						*((HWORD volatile *)(USB_BASE + 0x14))	//[m-1][MaxPayload/Transaction10:0]
#define USB_RXCSR						*((HWORD volatile *)(USB_BASE + 0x16))	//HostMode:[AutoClear][AutoReq][DMAReqEn][DisNyet][DMAReqMode][DataTogWrEn][DataTog][IncompRx]
					   	//[ClrDataTog][RxStall][ReqPkt][FlushFIFO][DataError/NAKTimeout][Error][FIFOFull][RxPktRdy]
#define USB_RXCOUNT						*((HWORD volatile *)(USB_BASE + 0x18))	//[15:13][EndpointRxCount12:0]
#define USB_TXTYPE						*((BYTE  volatile *)(USB_BASE + 0x1a))	//Only used by HostMode[Speed1:0][Protocol1:0][TargeEndpointNumber3:0]
#define USB_TXINTERVAL					*((BYTE  volatile *)(USB_BASE + 0x1b))	//Only used by HostMode[TxPollingInterval/NAKLimit]
#define USB_RXTYPE						*((BYTE  volatile *)(USB_BASE + 0x1c))	//Only used by HostMode[Speed1:0][Protocol1:0][TargetEndpointNumber3:0]
#define USB_RXINTERVAL					*((BYTE  volatile *)(USB_BASE + 0x1d))	//Only used by HostMode[RxPollingInterval/NAKLimit]
#define USB_FIFOSIZE					*((BYTE  volatile *)(USB_BASE + 0x1f))	//[RxFIFOSize3:0][TxFIFOSize3:0]
/***************************************************/
//the  registers used by Endpoint 0 (USB_INDEX = 0)

#define USB_CSR0						*((HWORD volatile *)(USB_BASE + 0x12))	//hostmode:[15:11][datatog_wr_en][data_tog][flushfifo]
							//[naktimeout][statuspkt][reqpkt][error][setuppkt][rxstall][txpktrdy][rxpktrdy]
#define USB_COUNT0						*((BYTE  volatile *)(USB_BASE + 0x18))	//[7][endpoint0_rx_count6:0]
#define USB_TYPE0						*((BYTE  volatile *)(USB_BASE + 0x1a))	//Only used by HostMode[speed1:0][5:0]
#define USB_NAKLIMIT0					*((BYTE  volatile *)(USB_BASE + 0x1b))	//Only used by HostMode[7:5][endpoint0_nak_limit4:0]
#define USB_CONFIGDATA					*((BYTE  volatile *)(USB_BASE + 0x1f))	//[mprxe][mptxe][bigendian][hbrxe][hbtxe][dynfifo_sizing][softcone][utmi_datawidth]
/***************************************************/

//------FIFO Address of Endpoints------//
#define USB_EP0FIFO						*((BYTE  volatile *)(USB_BASE + 0x20))
#define USB_EP1FIFO						*((BYTE  volatile *)(USB_BASE + 0x24))
//#define USB_EP2FIFO					*((BYTE  volatile *)(USB_BASE + 0x28))

//------OTG,Dynamic FIFO,Version & Vendor Registers------//
#define USB_DEVCTL						*((BYTE  volatile *)(USB_BASE + 0x60))	//[b_device][fsdev][lsdev][vbus1][vbus0][host_mode][host_req][session]
//#define USB_TXFIFOSZ					*((BYTE  volatile *)(USB_BASE + 0x62))
//#define USB_RXFIFOSZ					*((BYTE  volatile *)(USB_BASE + 0x63))
//#define USB_TXFIFOADD					*((HWORD volatile *)(USB_BASE + 0x64))
//#define USB_RXFIFOADD					*((HWORD volatile *)(USB_BASE + 0x66))
//#define USB_UTMI_PHY					*((WORD  volatile *)(USB_BASE + 0x68))
//#define USB_HWVERS					*((HWORD volatile *)(USB_BASE + 0x6c))


//------Target Address Registers------//
//#define USB_TXFUNCADDR_EP0			*((BYTE  volatile *)(USB_BASE + 0x80))//[7][AddressofTargetFunction6:0]
//#define USB_TXHUBADDR_EP0				*((BYTE  volatile *)(USB_BASE + 0x82))//[MultipleTranslators][HubAddress6:0]
//#define USB_TXHUBPORT_EP0				*((BYTE  volatile *)(USB_BASE + 0x83))//[7][HubPort6:0]
//#define USB_RXFUNCADDR_EP0			*((BYTE  volatile *)(USB_BASE + 0x84))//[7][AddressofTargetFunction6:0]
//#define USB_RXHUBADDR_EP0				*((BYTE  volatile *)(USB_BASE + 0x86))//[MultipleTranslators][HubAddress6:0]
//#define USB_RXHUBPORT_EP0				*((BYTE  volatile *)(USB_BASE + 0x87))//[7][HubPort6:0]

//#define USB_TXFUNCADDR_EP1			*((BYTE  volatile *)(USB_BASE + 0x80+8))//[7][AddressofTargetFunction6:0]
//#define USB_TXHUBADDR_EP1				*((BYTE  volatile *)(USB_BASE + 0x82+8))//[MultipleTranslators][HubAddress6:0]
//#define USB_TXHUBPORT_EP1				*((BYTE  volatile *)(USB_BASE + 0x83+8))//[7][HubPort6:0]
//#define USB_RXFUNCADDR_EP1			*((BYTE  volatile *)(USB_BASE + 0x84+8))//[7][AddressofTargetFunction6:0]
//#define USB_RXHUBADDR_EP1				*((BYTE  volatile *)(USB_BASE + 0x86+8))//[MultipleTranslators][HubAddress6:0]
//#define USB_RXHUBPORT_EP1				*((BYTE  volatile *)(USB_BASE + 0x87+8))//[7][HubPort6:0]

//#define USB_TXFUNCADDR_EP2			*((BYTE  volatile *)(USB_BASE + 0x80+16))//[7][AddressofTargetFunction6:0]
//#define USB_TXHUBADDR_EP2				*((BYTE  volatile *)(USB_BASE + 0x82+16))//[MultipleTranslators][HubAddress6:0]
//#define USB_TXHUBPORT_EP2				*((BYTE  volatile *)(USB_BASE + 0x83+16))//[7][HubPort6:0]
//#define USB_RXFUNCADDR_EP2			*((BYTE  volatile *)(USB_BASE + 0x84+16))//[7][AddressofTargetFunction6:0]
//#define USB_RXHUBADDR_EP2				*((BYTE  volatile *)(USB_BASE + 0x86+16))//[MultipleTranslators][HubAddress6:0]
//#define USB_RXHUBPORT_EP2				*((BYTE  volatile *)(USB_BASE + 0x87+16))//[7][HubPort6:0]

/*isp register  ISP_BASE=0x70090000*/
// #define rISP_SYS  (0x000)
#define rISP_SYS							      (*(volatile unsigned int  *) (ISP_BASE + 0x0))

// #define GEM_STS_BASE  (0x158)
#define rISP_INT_STATUS							(*(volatile unsigned int  *) (ISP_BASE + 0x1a8))

//#define GEM_MSK_BASE  (0x05c)



/*isp scale use and 601in use */
#define rISP_SCALE_EN     					*((volatile unsigned int *)(ISP_SCALE_BASE + 0x0))
#define rISP_SCALE_WB_START				*((volatile unsigned int *)(ISP_SCALE_BASE + 0x4))
#define rISP_SCALE_CONTROL	      		*((volatile unsigned int *)(ISP_SCALE_BASE + 0x8))
#define rISP_SCALE_VIDEO_ADDR1			*((volatile unsigned int *)(ISP_SCALE_BASE + 0xC))
#define rISP_SCALE_VIDEO_ADDR2	      *((volatile unsigned int *)(ISP_SCALE_BASE + 0x10))
#define rISP_SCALE_VIDEO_ADDR3	      *((volatile unsigned int *)(ISP_SCALE_BASE + 0x14))
#define rISP_SCALE_VIDEO_SOURCE_SIZE   *((volatile unsigned int *)(ISP_SCALE_BASE + 0x18))
#define rISP_SCALE_VIDEO_WINDOW_POINT  *((volatile unsigned int *)(ISP_SCALE_BASE + 0x1c))
#define rISP_SCALE_VIDEO_WINDOW_SIZE   *((volatile unsigned int *)(ISP_SCALE_BASE + 0x20))
#define rISP_SCALE_VIDEO_SIZE				*((volatile unsigned int *)(ISP_SCALE_BASE + 0x24))
#define rISP_SCALE_SCALE_CTL				*((volatile unsigned int *)(ISP_SCALE_BASE + 0x28))
#define rISP_SCALE_SCALE_VXMOD			*((volatile unsigned int *)(ISP_SCALE_BASE + 0x2c))
#define rISP_SCALE_SCALE_CTL0		      *((volatile unsigned int *)(ISP_SCALE_BASE + 0x30))
#define rISP_SCALE_SCALE_CTL1          *((volatile unsigned int *)(ISP_SCALE_BASE + 0x34))
#define rISP_SCALE_RIGHT_BOTTOM_CUT_NUM	*((volatile unsigned int *)(ISP_SCALE_BASE + 0x38))
#define rISP_SCALE_SCALE_CTL2				*((volatile unsigned int *)(ISP_SCALE_BASE + 0x3c))
#define rISP_SCALE_SCALE_CTL3				*((volatile unsigned int *)(ISP_SCALE_BASE + 0x40))
#define rISP_SCALE_SCALE_CTL4				*((volatile unsigned int *)(ISP_SCALE_BASE + 0x44))
#define rISP_SCALE_HSCAL_COS_VALUE		*((volatile unsigned int *)(ISP_SCALE_BASE + 0x48))
#define rISP_SCALE_WB_DEST_YADDR			*((volatile unsigned int *)(ISP_SCALE_BASE + 0x4c))
#define rISP_SCALE_WB_DEST_UADDR			*((volatile unsigned int *)(ISP_SCALE_BASE + 0x50))
#define rISP_SCALE_WB_DEST_VADDR			*((volatile unsigned int *)(ISP_SCALE_BASE + 0x54))
#define rISP_SCALE_WB_DATA_HSIZE_RAM	*((volatile unsigned int *)(ISP_SCALE_BASE + 0x58))
#define rISP_SCALE_RESERVED				*((volatile unsigned int *)(ISP_SCALE_BASE + 0x5c))
#define rISP_SCALE_INT_CTL					*((volatile unsigned int *)(ISP_SCALE_BASE + 0x60))
#define rISP_SCALE_INT_STATUS				*((volatile unsigned int *)(ISP_SCALE_BASE + 0x64))
#define rISP_SCALE_CLCD_INT_CLR			*((volatile unsigned int *)(ISP_SCALE_BASE + 0x68))
#define rISP_SCALE_WR_Y_VCNT				*((volatile unsigned int *)(ISP_SCALE_BASE + 0x6C))
#define rISP_SCALE_WB_STATUS                  (*(volatile unsigned int  *) (ISP_SCALE_BASE +  0x70 ))
#define rISP_SCALE_WB_CTL                     (*(volatile unsigned int  *) (ISP_SCALE_BASE +  0x74 ))
#define rISP_SCALE_WB_FINISH_YADDR			*((volatile unsigned int *)(ISP_SCALE_BASE + 0x78))
#define rISP_SCALE_WB_FINISH_UADDR			*((volatile unsigned int *)(ISP_SCALE_BASE + 0x78))
#define rISP_SCALE_Frame_Mask    			*((volatile unsigned int *)(ISP_SCALE_BASE + 0x7c))


/* NAND Flash Controller */
#define rNAND_CR							(*(volatile unsigned int  *) (NAND_BASE + 0x00))
#define rNAND_CLE_WR						(*(volatile unsigned char *) (NAND_BASE + 0x04))
#define rNAND_ALE_WR						(*(volatile unsigned char *) (NAND_BASE + 0x08))
#define rNAND_ID_RD							(*(volatile unsigned char *) (NAND_BASE + 0x0c))
#define rNAND_STATUS_RD					(*(volatile unsigned char *) (NAND_BASE + 0x10))
#define rNAND_DATA							(*(volatile unsigned int  *) (NAND_BASE + 0x14))
#define rNAND_TX_FIFO						(*(volatile unsigned int  *) (NAND_BASE + 0x18))
#define rNAND_RX_FIFO						(*(volatile unsigned int  *) (NAND_BASE + 0x1c))
#define rNAND_WRBLK_START					(*(volatile unsigned int  *) (NAND_BASE + 0x20))
#define rNAND_RDBLK_START					(*(volatile unsigned int  *) (NAND_BASE + 0x24))
#define rBCH_ENCODE_RESULT0_GROUP1		(*(volatile unsigned int  *) (NAND_BASE + 0x28))
#define rBCH_ENCODE_RESULT1_GROUP1		(*(volatile unsigned int  *) (NAND_BASE + 0x2c))
#define rBCH_ENCODE_RESULT2_GROUP1		(*(volatile unsigned int  *) (NAND_BASE + 0x30))
#define rBCH_ENCODE_RESULT3_GROUP1		(*(volatile unsigned int  *) (NAND_BASE + 0x34))
#define rBCH_ENCODE_RESULT4_GROUP1		(*(volatile unsigned int  *) (NAND_BASE + 0x38))
#define rBCH_ENCODE_RESULT5_GROUP1		(*(volatile unsigned int  *) (NAND_BASE + 0x3c))

#define rBCH_ENCODE_RESULT0_GROUP2		(*(volatile unsigned int  *) (NAND_BASE + 0x40))
#define rBCH_ENCODE_RESULT1_GROUP2		(*(volatile unsigned int  *) (NAND_BASE + 0x44))
#define rBCH_ENCODE_RESULT2_GROUP2		(*(volatile unsigned int  *) (NAND_BASE + 0x48))
#define rBCH_ENCODE_RESULT3_GROUP2		(*(volatile unsigned int  *) (NAND_BASE + 0x4c))
#define rBCH_ENCODE_RESULT4_GROUP2		(*(volatile unsigned int  *) (NAND_BASE + 0x50))
#define rBCH_ENCODE_RESULT5_GROUP2		(*(volatile unsigned int  *) (NAND_BASE + 0x54))

#define rBCH_ENCODE_RESULT0_GROUP3		(*(volatile unsigned int  *) (NAND_BASE + 0x58))
#define rBCH_ENCODE_RESULT1_GROUP3		(*(volatile unsigned int  *) (NAND_BASE + 0x5c))
#define rBCH_ENCODE_RESULT2_GROUP3		(*(volatile unsigned int  *) (NAND_BASE + 0x60))
#define rBCH_ENCODE_RESULT3_GROUP3		(*(volatile unsigned int  *) (NAND_BASE + 0x64))
#define rBCH_ENCODE_RESULT4_GROUP3		(*(volatile unsigned int  *) (NAND_BASE + 0x68))
#define rBCH_ENCODE_RESULT5_GROUP3		(*(volatile unsigned int  *) (NAND_BASE + 0x6c))

#define rBCH_ENCODE_RESULT0_GROUP4		(*(volatile unsigned int  *) (NAND_BASE + 0x70))
#define rBCH_ENCODE_RESULT1_GROUP4		(*(volatile unsigned int  *) (NAND_BASE + 0x74))
#define rBCH_ENCODE_RESULT2_GROUP4		(*(volatile unsigned int  *) (NAND_BASE + 0x78))
#define rBCH_ENCODE_RESULT3_GROUP4		(*(volatile unsigned int  *) (NAND_BASE + 0x7c))
#define rBCH_ENCODE_RESULT4_GROUP4		(*(volatile unsigned int  *) (NAND_BASE + 0x80))
#define rBCH_ENCODE_RESULT5_GROUP4		(*(volatile unsigned int  *) (NAND_BASE + 0x84))

#define rBCH_ENCODE_RESULT0_GROUP5		(*(volatile unsigned int  *) (NAND_BASE + 0x88))
#define rBCH_ENCODE_RESULT1_GROUP5		(*(volatile unsigned int  *) (NAND_BASE + 0x8c))
#define rBCH_ENCODE_RESULT2_GROUP5		(*(volatile unsigned int  *) (NAND_BASE + 0x90))
#define rBCH_ENCODE_RESULT3_GROUP5		(*(volatile unsigned int  *) (NAND_BASE + 0x94))
#define rBCH_ENCODE_RESULT4_GROUP5		(*(volatile unsigned int  *) (NAND_BASE + 0x98))
#define rBCH_ENCODE_RESULT5_GROUP5		(*(volatile unsigned int  *) (NAND_BASE + 0x9c))

#define rBCH_ENCODE_RESULT0_GROUP6		(*(volatile unsigned int  *) (NAND_BASE + 0xa0))
#define rBCH_ENCODE_RESULT1_GROUP6		(*(volatile unsigned int  *) (NAND_BASE + 0xa4))
#define rBCH_ENCODE_RESULT2_GROUP6		(*(volatile unsigned int  *) (NAND_BASE + 0xa8))
#define rBCH_ENCODE_RESULT3_GROUP6		(*(volatile unsigned int  *) (NAND_BASE + 0xac))
#define rBCH_ENCODE_RESULT4_GROUP6		(*(volatile unsigned int  *) (NAND_BASE + 0xb0))
#define rBCH_ENCODE_RESULT5_GROUP6		(*(volatile unsigned int  *) (NAND_BASE + 0xb4))

#define rBCH_ENCODE_RESULT0_GROUP7		(*(volatile unsigned int  *) (NAND_BASE + 0xb8))
#define rBCH_ENCODE_RESULT1_GROUP7		(*(volatile unsigned int  *) (NAND_BASE + 0xbc))
#define rBCH_ENCODE_RESULT2_GROUP7		(*(volatile unsigned int  *) (NAND_BASE + 0xc0))
#define rBCH_ENCODE_RESULT3_GROUP7		(*(volatile unsigned int  *) (NAND_BASE + 0xc4))
#define rBCH_ENCODE_RESULT4_GROUP7		(*(volatile unsigned int  *) (NAND_BASE + 0xc8))
#define rBCH_ENCODE_RESULT5_GROUP7		(*(volatile unsigned int  *) (NAND_BASE + 0xcc))

#define rBCH_ENCODE_RESULT0_GROUP8		(*(volatile unsigned int  *) (NAND_BASE + 0xd0))
#define rBCH_ENCODE_RESULT1_GROUP8		(*(volatile unsigned int  *) (NAND_BASE + 0xd4))
#define rBCH_ENCODE_RESULT2_GROUP8		(*(volatile unsigned int  *) (NAND_BASE + 0xd8))
#define rBCH_ENCODE_RESULT3_GROUP8		(*(volatile unsigned int  *) (NAND_BASE + 0xdc))
#define rBCH_ENCODE_RESULT4_GROUP8		(*(volatile unsigned int  *) (NAND_BASE + 0xe0))
#define rBCH_ENCODE_RESULT5_GROUP8		(*(volatile unsigned int  *) (NAND_BASE + 0xe4))


#define rBCH_DECODE_RESULT0_GROUP1		(*(volatile unsigned int  *) (NAND_BASE + 0xe8))
#define rBCH_DECODE_RESULT1_GROUP1		(*(volatile unsigned int  *) (NAND_BASE + 0xec))
#define rBCH_DECODE_RESULT2_GROUP1		(*(volatile unsigned int  *) (NAND_BASE + 0xf0))
#define rBCH_DECODE_RESULT3_GROUP1		(*(volatile unsigned int  *) (NAND_BASE + 0xf4))
#define rBCH_DECODE_RESULT4_GROUP1		(*(volatile unsigned int  *) (NAND_BASE + 0xf8))
#define rBCH_DECODE_RESULT5_GROUP1		(*(volatile unsigned int  *) (NAND_BASE + 0xfc))
#define rBCH_DECODE_RESULT6_GROUP1		(*(volatile unsigned int  *) (NAND_BASE + 0x100))

#define rBCH_DECODE_RESULT0_GROUP2		(*(volatile unsigned int  *) (NAND_BASE + 0x104))
#define rBCH_DECODE_RESULT1_GROUP2		(*(volatile unsigned int  *) (NAND_BASE + 0x108))
#define rBCH_DECODE_RESULT2_GROUP2		(*(volatile unsigned int  *) (NAND_BASE + 0x10c))
#define rBCH_DECODE_RESULT3_GROUP2		(*(volatile unsigned int  *) (NAND_BASE + 0x110))
#define rBCH_DECODE_RESULT4_GROUP2		(*(volatile unsigned int  *) (NAND_BASE + 0x114))
#define rBCH_DECODE_RESULT5_GROUP2		(*(volatile unsigned int  *) (NAND_BASE + 0x118))
#define rBCH_DECODE_RESULT6_GROUP2		(*(volatile unsigned int  *) (NAND_BASE + 0x11c))

#define rBCH_DECODE_RESULT0_GROUP3		(*(volatile unsigned int  *) (NAND_BASE + 0x120))
#define rBCH_DECODE_RESULT1_GROUP3		(*(volatile unsigned int  *) (NAND_BASE + 0x124))
#define rBCH_DECODE_RESULT2_GROUP3		(*(volatile unsigned int  *) (NAND_BASE + 0x128))
#define rBCH_DECODE_RESULT3_GROUP3		(*(volatile unsigned int  *) (NAND_BASE + 0x12c))
#define rBCH_DECODE_RESULT4_GROUP3		(*(volatile unsigned int  *) (NAND_BASE + 0x130))
#define rBCH_DECODE_RESULT5_GROUP3		(*(volatile unsigned int  *) (NAND_BASE + 0x134))
#define rBCH_DECODE_RESULT6_GROUP3		(*(volatile unsigned int  *) (NAND_BASE + 0x138))

#define rBCH_DECODE_RESULT0_GROUP4		(*(volatile unsigned int  *) (NAND_BASE + 0x13c))
#define rBCH_DECODE_RESULT1_GROUP4		(*(volatile unsigned int  *) (NAND_BASE + 0x140))
#define rBCH_DECODE_RESULT2_GROUP4		(*(volatile unsigned int  *) (NAND_BASE + 0x144))
#define rBCH_DECODE_RESULT3_GROUP4		(*(volatile unsigned int  *) (NAND_BASE + 0x148))
#define rBCH_DECODE_RESULT4_GROUP4		(*(volatile unsigned int  *) (NAND_BASE + 0x14c))
#define rBCH_DECODE_RESULT5_GROUP4		(*(volatile unsigned int  *) (NAND_BASE + 0x150))
#define rBCH_DECODE_RESULT6_GROUP4		(*(volatile unsigned int  *) (NAND_BASE + 0x154))

#define rBCH_DECODE_RESULT0_GROUP5		(*(volatile unsigned int  *) (NAND_BASE + 0x158))
#define rBCH_DECODE_RESULT1_GROUP5		(*(volatile unsigned int  *) (NAND_BASE + 0x15c))
#define rBCH_DECODE_RESULT2_GROUP5		(*(volatile unsigned int  *) (NAND_BASE + 0x160))
#define rBCH_DECODE_RESULT3_GROUP5		(*(volatile unsigned int  *) (NAND_BASE + 0x164))
#define rBCH_DECODE_RESULT4_GROUP5		(*(volatile unsigned int  *) (NAND_BASE + 0x168))
#define rBCH_DECODE_RESULT5_GROUP5		(*(volatile unsigned int  *) (NAND_BASE + 0x16c))
#define rBCH_DECODE_RESULT6_GROUP5		(*(volatile unsigned int  *) (NAND_BASE + 0x170))

#define rBCH_DECODE_RESULT0_GROUP6		(*(volatile unsigned int  *) (NAND_BASE + 0x174))
#define rBCH_DECODE_RESULT1_GROUP6		(*(volatile unsigned int  *) (NAND_BASE + 0x178))
#define rBCH_DECODE_RESULT2_GROUP6		(*(volatile unsigned int  *) (NAND_BASE + 0x17c))
#define rBCH_DECODE_RESULT3_GROUP6		(*(volatile unsigned int  *) (NAND_BASE + 0x180))
#define rBCH_DECODE_RESULT4_GROUP6		(*(volatile unsigned int  *) (NAND_BASE + 0x184))
#define rBCH_DECODE_RESULT5_GROUP6		(*(volatile unsigned int  *) (NAND_BASE + 0x188))
#define rBCH_DECODE_RESULT6_GROUP6		(*(volatile unsigned int  *) (NAND_BASE + 0x18c))

#define rBCH_DECODE_RESULT0_GROUP7		(*(volatile unsigned int  *) (NAND_BASE + 0x190))
#define rBCH_DECODE_RESULT1_GROUP7		(*(volatile unsigned int  *) (NAND_BASE + 0x194))
#define rBCH_DECODE_RESULT2_GROUP7		(*(volatile unsigned int  *) (NAND_BASE + 0x198))
#define rBCH_DECODE_RESULT3_GROUP7		(*(volatile unsigned int  *) (NAND_BASE + 0x19c))
#define rBCH_DECODE_RESULT4_GROUP7		(*(volatile unsigned int  *) (NAND_BASE + 0x1a0))
#define rBCH_DECODE_RESULT5_GROUP7		(*(volatile unsigned int  *) (NAND_BASE + 0x1a4))
#define rBCH_DECODE_RESULT6_GROUP7		(*(volatile unsigned int  *) (NAND_BASE + 0x1a8))

#define rBCH_DECODE_RESULT0_GROUP8		(*(volatile unsigned int  *) (NAND_BASE + 0x1ac))
#define rBCH_DECODE_RESULT1_GROUP8		(*(volatile unsigned int  *) (NAND_BASE + 0x1b0))
#define rBCH_DECODE_RESULT2_GROUP8		(*(volatile unsigned int  *) (NAND_BASE + 0x1b4))
#define rBCH_DECODE_RESULT3_GROUP8		(*(volatile unsigned int  *) (NAND_BASE + 0x1b8))
#define rBCH_DECODE_RESULT4_GROUP8		(*(volatile unsigned int  *) (NAND_BASE + 0x1bc))
#define rBCH_DECODE_RESULT5_GROUP8		(*(volatile unsigned int  *) (NAND_BASE + 0x1c0))
#define rBCH_DECODE_RESULT6_GROUP8		(*(volatile unsigned int  *) (NAND_BASE + 0x1c4))

#define rEX_BCH_ENCODE_SOURCE				(*(volatile unsigned int  *) (NAND_BASE + 0x1c8))
#define rEX_BCH_DECODE_SOURCE				(*(volatile unsigned int  *) (NAND_BASE + 0x1cc))

#define rEX_BCH_ENCODE_RESULT0			(*(volatile unsigned int  *) (NAND_BASE + 0x1d0))
#define rEX_BCH_ENCODE_RESULT1			(*(volatile unsigned int  *) (NAND_BASE + 0x1d4))
#define rEX_BCH_ENCODE_RESULT2			(*(volatile unsigned int  *) (NAND_BASE + 0x1d8))
#define rEX_BCH_ENCODE_RESULT3			(*(volatile unsigned int  *) (NAND_BASE + 0x1dc))
#define rEX_BCH_ENCODE_RESULT4			(*(volatile unsigned int  *) (NAND_BASE + 0x1e0))
#define rEX_BCH_ENCODE_RESULT5			(*(volatile unsigned int  *) (NAND_BASE + 0x1e4))
#define rEX_BCH_ENCODE_RESULT6			(*(volatile unsigned int  *) (NAND_BASE + 0x1e8))
#define rEX_BCH_ENCODE_RESULT7			(*(volatile unsigned int  *) (NAND_BASE + 0x1ec))
#define rEX_BCH_ENCODE_RESULT8			(*(volatile unsigned int  *) (NAND_BASE + 0x1f0))
#define rEX_BCH_ENCODE_RESULT9			(*(volatile unsigned int  *) (NAND_BASE + 0x1f4))
#define rEX_BCH_ENCODE_RESULT10			(*(volatile unsigned int  *) (NAND_BASE + 0x1f8))
#define rEX_BCH_ENCODE_RESULT11			(*(volatile unsigned int  *) (NAND_BASE + 0x1fc))
#define rEX_BCH_ENCODE_RESULT12			(*(volatile unsigned int  *) (NAND_BASE + 0x200))
#define rEX_BCH_ENCODE_RESULT13			(*(volatile unsigned int  *) (NAND_BASE + 0x204))
#define rEX_BCH_ENCODE_RESULT14			(*(volatile unsigned int  *) (NAND_BASE + 0x208))
#define rEX_BCH_ENCODE_RESULT15			(*(volatile unsigned int  *) (NAND_BASE + 0x20c))
#define rEX_BCH_ENCODE_RESULT16			(*(volatile unsigned int  *) (NAND_BASE + 0x210))
#define rEX_BCH_ENCODE_RESULT17			(*(volatile unsigned int  *) (NAND_BASE + 0x214))
#define rEX_BCH_ENCODE_RESULT18			(*(volatile unsigned int  *) (NAND_BASE + 0x218))
#define rEX_BCH_ENCODE_RESULT19			(*(volatile unsigned int  *) (NAND_BASE + 0x21c))
#define rEX_BCH_ENCODE_RESULT20			(*(volatile unsigned int  *) (NAND_BASE + 0x220))

#define rEX_BCH_ENCODE_STATUS			(*(volatile unsigned int  *) (NAND_BASE + 0x274))
#define rEX_BCH_DECODE_STATUS			(*(volatile unsigned int  *) (NAND_BASE + 0x278))
#define rBCH_CR							(*(volatile unsigned int  *) (NAND_BASE + 0x27c))
#define rBCH_NAND_STATUS				(*(volatile unsigned int  *) (NAND_BASE + 0x280))
#define rBCH_DECODE_STATUS				(*(volatile unsigned int  *) (NAND_BASE + 0x284))
#define rBCH_INT						(*(volatile unsigned int  *) (NAND_BASE + 0x288))
#define rBCH_INT_MASK					(*(volatile unsigned int  *) (NAND_BASE + 0x28c))
#define rNAND_DMA_CTRL					(*(volatile unsigned int  *) (NAND_BASE + 0x290))
#define rNAND_GLOBAL_CTL				(*(volatile unsigned int  *) (NAND_BASE + 0x294))
#define rNAND_JUMP_CTL					(*(volatile unsigned int  *) (NAND_BASE + 0x298))

#define rEX_BCH_DECODE_RESULT0			(*(volatile unsigned int  *) (NAND_BASE + 0x29c))
#define rEX_BCH_DECODE_RESULT1			(*(volatile unsigned int  *) (NAND_BASE + 0x268))
#define rEX_BCH_DECODE_RESULT2			(*(volatile unsigned int  *) (NAND_BASE + 0x26c))
#define rEX_BCH_DECODE_RESULT3			(*(volatile unsigned int  *) (NAND_BASE + 0x270))
#define rEX_BCH_DECODE_RESULT4			(*(volatile unsigned int  *) (NAND_BASE + 0x264))
#define rEX_BCH_DECODE_RESULT5			(*(volatile unsigned int  *) (NAND_BASE + 0x268))
#define rEX_BCH_DECODE_RESULT6			(*(volatile unsigned int  *) (NAND_BASE + 0x26c))
#define rEX_BCH_DECODE_RESULT7			(*(volatile unsigned int  *) (NAND_BASE + 0x270))
#define rEX_BCH_DECODE_RESULT8			(*(volatile unsigned int  *) (NAND_BASE + 0x264))
#define rEX_BCH_DECODE_RESULT9			(*(volatile unsigned int  *) (NAND_BASE + 0x268))
#define rEX_BCH_DECODE_RESULT10			(*(volatile unsigned int  *) (NAND_BASE + 0x26c))
#define rEX_BCH_DECODE_RESULT11			(*(volatile unsigned int  *) (NAND_BASE + 0x270))
#define rEX_BCH_DECODE_RESULT12			(*(volatile unsigned int  *) (NAND_BASE + 0x264))
#define rEX_BCH_DECODE_RESULT13			(*(volatile unsigned int  *) (NAND_BASE + 0x268))
#define rEX_BCH_DECODE_RESULT14			(*(volatile unsigned int  *) (NAND_BASE + 0x26c))
#define rEX_BCH_DECODE_RESULT15			(*(volatile unsigned int  *) (NAND_BASE + 0x270))
#define rEX_BCH_DECODE_RESULT16			(*(volatile unsigned int  *) (NAND_BASE + 0x264))
#define rEX_BCH_DECODE_RESULT17			(*(volatile unsigned int  *) (NAND_BASE + 0x268))
#define rEX_BCH_DECODE_RESULT18			(*(volatile unsigned int  *) (NAND_BASE + 0x26c))
#define rEX_BCH_DECODE_RESULT19			(*(volatile unsigned int  *) (NAND_BASE + 0x270))
#define rEX_BCH_DECODE_RESULT20			(*(volatile unsigned int  *) (NAND_BASE + 0x264))
#define rEX_BCH_DECODE_RESULT21			(*(volatile unsigned int  *) (NAND_BASE + 0x268))
#define rEX_BCH_DECODE_RESULT22			(*(volatile unsigned int  *) (NAND_BASE + 0x26c))
#define rEX_BCH_DECODE_RESULT23			(*(volatile unsigned int  *) (NAND_BASE + 0x270))



#define ECCCODET0ADDR	volatile UINT32*)((NAND_BASE + 0x28)
#define ECCCODET1ADDR	volatile UINT32*)((NAND_BASE + 0x2c)
#define ECCCODET2ADDR	volatile UINT32*)((NAND_BASE + 0x30)
#define ECCCODET3ADDR	volatile UINT32*)((NAND_BASE + 0x34)
#define ECCCODET4ADDR	volatile UINT32*)((NAND_BASE + 0x38)
#define ECCCODET5ADDR	volatile UINT32*)((NAND_BASE + 0x3c)

//#define ECCRESULTADDR	volatile UINT32*)((NAND_BASE + 0xe8)
#define EX_BCH_ENCODE_RESULT_ADDR	volatile UINT32*) ((NAND_BASE + 0x1d0)
#define EX_BCH_DECODE_RESULT_ADDR	volatile UINT32*)((NAND_BASE + 0x29c)

enum {
	NAND_DEV_0 = 0,
	NAND_DEV_COUNT		// NANDFLASH支持的设备个数
};

#define rITU656IN_ModuleEn         (*(volatile unsigned int *)(ITU656_BASE + 0x00))
#define rITU656IN_IMR              (*(volatile unsigned int *)(ITU656_BASE + 0x124))
#define rITU656IN_ICR              (*(volatile unsigned int *)(ITU656_BASE + 0x128))
#define rITU656IN_ISR              (*(volatile unsigned int *)(ITU656_BASE + 0x12c))
#define rITU656IN_LINE_NUM_PER_FIELD (*(volatile unsigned int *)(ITU656_BASE + 0x8f4))
#define rITU656IN_PIX_NUM_PER_LINE (*(volatile unsigned int *)(ITU656_BASE + 0x8f8))
#define rITU656IN_DELTA_NUM        (*(volatile unsigned int *)(ITU656_BASE + 0x8fc))
#define rITU656IN_INPUT_SEL        (*(volatile unsigned int *)(ITU656_BASE + 0x900))
#define rITU656IN_INPUT_CTL        (*(volatile unsigned int *)(ITU656_BASE + 0x904))
#define rITU656IN_ENABLE_REG       (*(volatile unsigned int *)(ITU656_BASE + 0x930))
#define rITU656IN_MODULE_STATUS    (*(volatile unsigned int *)(ITU656_BASE + 0x934))
#define rITU656IN_SIZE            (*(volatile unsigned int *)(ITU656_BASE + 0x938))
#define rITU656IN_SLICE_PIXEL_NUM (*(volatile unsigned int *)(ITU656_BASE + 0x94c))
#define rITU656IN_DRAM_Y_ADDR     (*(volatile unsigned int *)(ITU656_BASE + 0x950))
#define rITU656IN_DRAM_CBCR_ADDR  (*(volatile unsigned int *)(ITU656_BASE + 0x954))
#define rITU656IN_TOTAL_PIX       (*(volatile unsigned int *)(ITU656_BASE + 0x958))
#define rITU656IN_DATA_OUT_LINE_NUM_PER_FIELD (*(volatile unsigned int *)(ITU656_BASE + 0x95c))
#define rITU656IN_H_CUT          (*(volatile unsigned int *)(ITU656_BASE + 0x960))
#define rITU656IN_V_CUT          (*(volatile unsigned int *)(ITU656_BASE + 0x964))



#define ITU_A_BASE                ITU656_BASE				// ahb_656	32'h7018_0000,64K
#if 1
#define ITU_A_ENABLE              (*(volatile unsigned int *)(ITU_A_BASE + 0x00*4))
#define ITU_A_CTL                 (*(volatile unsigned int *)(ITU_A_BASE + 0x01*4))
#define ITU_A_ADDR00              (*(volatile unsigned int *)(ITU_A_BASE + 0x02*4))
#define ITU_A_ADDR01              (*(volatile unsigned int *)(ITU_A_BASE + 0x03*4))
#define ITU_A_ADDR02              (*(volatile unsigned int *)(ITU_A_BASE + 0x04*4))
#define ITU_A_ADDR10              (*(volatile unsigned int *)(ITU_A_BASE + 0x05*4))
#define ITU_A_ADDR11              (*(volatile unsigned int *)(ITU_A_BASE + 0x06*4))
#define ITU_A_ADDR12              (*(volatile unsigned int *)(ITU_A_BASE + 0x07*4))
#define ITU_A_INT_CTL             (*(volatile unsigned int *)(ITU_A_BASE + 0x08*4))
#define ITU_A_STATUS              (*(volatile unsigned int *)(ITU_A_BASE + 0x09*4))
#define ITU_A_INPUT_SIZE          (*(volatile unsigned int *)(ITU_A_BASE + 0x0a*4))
#define ITU_A_INPUT_WIDE_OFFSET   (*(volatile unsigned int *)(ITU_A_BASE + 0x0b*4))
#define ITU_A_STORE_CTL1          (*(volatile unsigned int *)(ITU_A_BASE + 0x0c*4))
#define ITU_A_GP_CTL              (*(volatile unsigned int *)(ITU_A_BASE + 0x0d*4))
#define ITU_A_DVB_PACK_CNT        (*(volatile unsigned int *)(ITU_A_BASE + 0x0e*4))
#define ITU_A_H_CROP              (*(volatile unsigned int *)(ITU_A_BASE + 0x14*4))
#define ITU_A_V_CROP              (*(volatile unsigned int *)(ITU_A_BASE + 0x15*4))
#define ITU_A_G_THRESH            (*(volatile unsigned int *)(ITU_A_BASE + 0x16*4))
#define ITU_A_B_THRESH            (*(volatile unsigned int *)(ITU_A_BASE + 0x17*4))
#define ITU_A_R_THRESH            (*(volatile unsigned int *)(ITU_A_BASE + 0x18*4))
#define ITU_A_AWB_RB              (*(volatile unsigned int *)(ITU_A_BASE + 0x1b*4))
#define ITU_A_AWB_G               (*(volatile unsigned int *)(ITU_A_BASE + 0x1c*4))
#define ITU_A_AEC_GAIN            (*(volatile unsigned int *)(ITU_A_BASE + 0x1d*4))
#define ITU_A_FRAME_SEL           (*(volatile unsigned int *)(ITU_A_BASE + 0x1e*4))
#define ITU_A_PID_CMP0            (*(volatile unsigned int *)(ITU_A_BASE + 0x1f*4))
#define ITU_A_PID_CMP1            (*(volatile unsigned int *)(ITU_A_BASE + 0x20*4))
#define ITU_A_PID_CMP2            (*(volatile unsigned int *)(ITU_A_BASE + 0x21*4))
#define ITU_A_PID_CMP3            (*(volatile unsigned int *)(ITU_A_BASE + 0x22*4))
#define ITU_A_PID_CMP4            (*(volatile unsigned int *)(ITU_A_BASE + 0x23*4))
#define ITU_A_PID_CMP5            (*(volatile unsigned int *)(ITU_A_BASE + 0x24*4))
#define ITU_A_PID_FLAG            (*(volatile unsigned int *)(ITU_A_BASE + 0x25*4))
#define ITU_A_DVB_SIZE            (*(volatile unsigned int *)(ITU_A_BASE + 0x26*4))
#define ITU_A_DMA_OUT_STEP        (*(volatile unsigned int *)(ITU_A_BASE + 0x27*4))       
#define ITU_A_SYS_FIFO_0_ADDR     (*(volatile unsigned int *)(ITU_A_BASE + 0x29*4))       
#define ITU_A_SYS_FIFO_1_ADDR     (*(volatile unsigned int *)(ITU_A_BASE + 0x2A*4))       
#define ITU_A_FINISH_FIFO_0_ADDR  (*(volatile unsigned int *)(ITU_A_BASE + 0x2B*4))       
#define ITU_A_FIFO_STATUS         (*(volatile unsigned int *)(ITU_A_BASE + 0x2D*4))       

#define ITU_A_FINISH_Y_ADDR       (*(volatile unsigned int *)(ITU_A_BASE + 0x30*4))      
#define ITU_A_FINISH_U_ADDR       (*(volatile unsigned int *)(ITU_A_BASE + 0x31*4))      
#define ITU_A_FINISH_V_ADDR       (*(volatile unsigned int *)(ITU_A_BASE + 0x32*4))      
#define ITU_A_FINISH_SUB_Y_ADDR   (*(volatile unsigned int *)(ITU_A_BASE + 0x33*4))      
#define ITU_A_FINISH_SUB_U_ADDR   (*(volatile unsigned int *)(ITU_A_BASE + 0x34*4))      
#define ITU_A_FINISH_SUB_V_ADDR   (*(volatile unsigned int *)(ITU_A_BASE + 0x35*4))      

#define ITU_A_ISP_PARAM0_ADDR     (*(volatile unsigned int *)(ITU_A_BASE + 0x44*4))      
#define ITU_A_ISP_PARAM1_ADDR     (*(volatile unsigned int *)(ITU_A_BASE + 0x45*4))      
#define ITU_A_ISP_PARAM2_ADDR     (*(volatile unsigned int *)(ITU_A_BASE + 0x46*4))      
#define ITU_A_ISP_PARAM3_ADDR     (*(volatile unsigned int *)(ITU_A_BASE + 0x47*4))      
#define ITU_A_ISP_PARAM4_ADDR     (*(volatile unsigned int *)(ITU_A_BASE + 0x48*4))      
#define ITU_A_ISP_PARAM5_ADDR     (*(volatile unsigned int *)(ITU_A_BASE + 0x49*4))      
#define ITU_A_ISP_PARAM6_ADDR     (*(volatile unsigned int *)(ITU_A_BASE + 0x4a*4))      
#define ITU_A_ISP_PARAM7_ADDR     (*(volatile unsigned int *)(ITU_A_BASE + 0x4b*4))      
#define ITU_A_ISP_PARAM8_ADDR     (*(volatile unsigned int *)(ITU_A_BASE + 0x4c*4))      
#define ITU_A_ISP_PARAM9_ADDR     (*(volatile unsigned int *)(ITU_A_BASE + 0x4d*4))      
#define ITU_A_ISP_PARAM10_ADDR    (*(volatile unsigned int *)(ITU_A_BASE + 0x4e*4))      
#define ITU_A_ISP_PARAM11_ADDR    (*(volatile unsigned int *)(ITU_A_BASE + 0x4f*4))      
#define ITU_A_ISP_PARAM12_ADDR    (*(volatile unsigned int *)(ITU_A_BASE + 0x50*4))      
#define ITU_A_ISP_PARAM13_ADDR    (*(volatile unsigned int *)(ITU_A_BASE + 0x51*4))      
#define ITU_A_ISP_PARAM14_ADDR    (*(volatile unsigned int *)(ITU_A_BASE + 0x52*4))     
#define ITU_A_ISP_PARAM15_ADDR    (*(volatile unsigned int *)(ITU_A_BASE + 0x53*4))       
                                                         
#define ITU_A_SOFT_VALUE          (*(volatile unsigned int *)(ITU_A_BASE + 0xf0))
#define ITU_A_FINISH              (*(volatile unsigned int *)(ITU_A_BASE + 0xfc))

#define ITU_A_GAMMA_RAM(addr)     (*(volatile unsigned int *)(ITU_A_BASE+4*1024 + addr*4))
#define ITU_A_LENS_RAM(addr)      (*(volatile unsigned int *)(ITU_A_BASE+4*2048 + addr*4))
#endif

/***************************************************************
      APB slave interface registers definition
****************************************************************/


/* Timer */
#define rTIMER_CTL0						(*(volatile unsigned int *)(TIMER_BASE + 0x00))
#define rTIMER_CTL1						(*(volatile unsigned int *)(TIMER_BASE + 0x04))
#define rTIMER_CTL2						(*(volatile unsigned int *)(TIMER_BASE + 0x08))
#define rTIMER_PRS0						(*(volatile unsigned int *)(TIMER_BASE + 0x0c))
#define rTIMER_PRS1						(*(volatile unsigned int *)(TIMER_BASE + 0x10))
#define rTIMER_PRS2						(*(volatile unsigned int *)(TIMER_BASE + 0x14))
#define rTIMER_MOD0						(*(volatile unsigned int *)(TIMER_BASE + 0x18))
#define rTIMER_MOD1						(*(volatile unsigned int *)(TIMER_BASE + 0x1c))
#define rTIMER_MOD2						(*(volatile unsigned int *)(TIMER_BASE + 0x20))
#define rTIMER_CNT0						(*(volatile unsigned int *)(TIMER_BASE + 0x24))
#define rTIMER_CNT1						(*(volatile unsigned int *)(TIMER_BASE + 0x28))
#define rTIMER_CNT2						(*(volatile unsigned int *)(TIMER_BASE + 0x2c))
#define rTIMER_CTL3						(*(volatile unsigned int *)(TIMER_BASE + 0x30))
#define rTIMER_CTL4						(*(volatile unsigned int *)(TIMER_BASE + 0x34))
#define rTIMER_CTL5						(*(volatile unsigned int *)(TIMER_BASE + 0x38))
#define rTIMER_PRS3						(*(volatile unsigned int *)(TIMER_BASE + 0x3c))
#define rTIMER_PRS4						(*(volatile unsigned int *)(TIMER_BASE + 0x40))
#define rTIMER_PRS5						(*(volatile unsigned int *)(TIMER_BASE + 0x44))
#define rTIMER_MOD3						(*(volatile unsigned int *)(TIMER_BASE + 0x48))
#define rTIMER_MOD4						(*(volatile unsigned int *)(TIMER_BASE + 0x4c))
#define rTIMER_MOD5						(*(volatile unsigned int *)(TIMER_BASE + 0x50))
#define rTIMER_CNT3						(*(volatile unsigned int *)(TIMER_BASE + 0x54))
#define rTIMER_CNT4						(*(volatile unsigned int *)(TIMER_BASE + 0x58))
#define rTIMER_CNT5						(*(volatile unsigned int *)(TIMER_BASE + 0x5c))
/*the list bit description is following:
 0: time3
 1: time0
 2: time1
 3: time2
 4: time3|time4|time5
 5: time4
 6: time5
ps: write  1 of this resiter to clr corresponding bit.*/
#define rTIMER_INT_STATUS           (*(volatile unsigned int *)(TIMER_BASE + 0x7c))

// TCTL0: Timer0 Control Register
//	Bit	Type  Reset		Description 
// 4		电平中断位 写0清中断			
// 3           0沿中断 1 电平
//	2		R/W	0			Interrupt enable. 
//								Interrupt enable or not. When HIGH enabled. When LOW disabled. 
//	1		R/W	0			Periodic running mode 
//								Periodic running mode chooses. 
//								When HIGH chosen. When LOW one-shot mode chosen. 
//	0		R/W	0			Timer enable 
//								Timer enables or disables. 
//								When HIGH enable timer. When LOW disable timer. 
#define rTCTL0				*((volatile unsigned int *)(TIMER_BASE + 0x00))

// TPRS0: Timer0 Prescaler Register 
//		TPRS0 is the prescaler register of timer0 
//	Bit	Type  Reset		Description 
//	7-0	R/W	0			Prescaler value 
//								This register provides a prescale value for timer0
#define rTPRS0				*((volatile unsigned int *)(TIMER_BASE + 0x0c))

// TMOD0: Timer0 Mode Register 
//		TMOD0 is the initial count value register of timer0 
//	Bit	Type  Reset		Description 
//	24-0	R/W	0 			Initial count value 
//								This register provides a initialize value to decrease for timer0. 
#define rTMOD0				*((volatile unsigned int *)(TIMER_BASE + 0x18))

// TCNT0: Timer0 Current Count Value Register
//		TCNT0 is the current count value register of timer0 
//	Bit	Type  Reset		Description 
//	24-0	R/W	0			Current count value 
//								This register provides a current count value of timer0. 
#define rTCNT0				*((volatile unsigned int *)(TIMER_BASE + 0x24))

// TSTA0: Timer0 Status Register 
//		TSTA0 is the status register of timer0 
//	Bit	Type  Reset		Description 
//	1		R/W	0			Interrupt maintenance 
//								Maintain the latest interrupt or not. 
//								When HIGH maintained. When LOW only sustain one cycle. 	
// 0		R/W	0			Interrupt status flag 
//								Latest interrupt status flag. 
#define rTSTA0				*((volatile unsigned int *)(TIMER_BASE + 0x40))

#define rTCTL1				*((volatile unsigned int *)(TIMER_BASE + 0x04))
#define rTPRS1				*((volatile unsigned int *)(TIMER_BASE + 0x10))
#define rTMOD1				*((volatile unsigned int *)(TIMER_BASE + 0x1c))
#define rTCNT1				*((volatile unsigned int *)(TIMER_BASE + 0x28))
#define rTCTL2				*((volatile unsigned int *)(TIMER_BASE + 0x08))
#define rTPRS2				*((volatile unsigned int *)(TIMER_BASE + 0x14))
#define rTMOD2				*((volatile unsigned int *)(TIMER_BASE + 0x20))
#define rTCNT2				*((volatile unsigned int *)(TIMER_BASE + 0x2c))

/* WDT */
#define rWDT_CR							(*(volatile unsigned int *)(WDT_BASE + 0x00))
#define rWDT_PSR							(*(volatile unsigned int *)(WDT_BASE + 0x04))
#define rWDT_LDR							(*(volatile unsigned int *)(WDT_BASE + 0x08))
#define rWDT_VLR							(*(volatile unsigned int *)(WDT_BASE + 0x0C))
#define rWDT_ISR							(*(volatile unsigned int *)(WDT_BASE + 0x10))
#define rWDT_RCR							(*(volatile unsigned int *)(WDT_BASE + 0x14))
#define rWDT_TMR							(*(volatile unsigned int *)(WDT_BASE + 0x18))
#define rWDT_TCR							(*(volatile unsigned int *)(WDT_BASE + 0x1C))

#define WDTCR		0x0
#define WDTPSR		0x4
#define WDTLDR		0x8
#define WDTVLR		0xc
#define WDTISR		0x10
#define WDTRVR		0x14
#define WDTTMR		0x80

typedef struct t18xx_wdt {
	unsigned int volatile wdtcr;
	unsigned int volatile wdtpsr;
	unsigned int volatile wdtldr;
	unsigned int volatile wdtvlr;
	unsigned int volatile wdtisr;
	unsigned int volatile wdtrvr;
} t18xx_wdt_t;

/* RTC */
#define rRTC_CTL	   						(*(volatile unsigned int *)(RTC_BASE + 0x00)) /*control register*/
#define rRTC_ANAWEN	   				(*(volatile unsigned int *)(RTC_BASE + 0x04)) /*analog block write enable register*/
#define rRTC_ANACTL	   					(*(volatile unsigned int *)(RTC_BASE + 0x08)) /*analog block control register*/
#define rRTC_IM	   						(*(volatile unsigned int *)(RTC_BASE + 0x0C)) /*interrupt mode register*/
#define rRTC_STA	   						(*(volatile unsigned int *)(RTC_BASE + 0x10)) /*rtc status register*/
#define rRTC_ALMDAT	   					(*(volatile unsigned int *)(RTC_BASE + 0x14)) /*alarm data register*/
#define rRTC_DONT	   					(*(volatile unsigned int *)(RTC_BASE + 0x18)) /*delay on timer register*/
#define rRTC_RAM	    					(*(volatile unsigned int *)(RTC_BASE + 0x1C)) /*ram bit register*/
#define rRTC_CNTL     						(*(volatile unsigned int *)(RTC_BASE + 0x20)) /*rtc counter register*/
#define rRTC_CNTH 	  					(*(volatile unsigned int *)(RTC_BASE + 0x24)) /*rtc sec counter register*/


/* UART0 */
#define rUART0_DR						(*(volatile unsigned int *)(UART0_BASE + 0x00))
#define rUART0_RSR						(*(volatile unsigned int *)(UART0_BASE + 0x04))
#define rUART0_FR						(*(volatile unsigned int *)(UART0_BASE + 0x18))
#define rUART0_ILPR						(*(volatile unsigned int *)(UART0_BASE + 0x20))
#define rUART0_IBRD						(*(volatile unsigned int *)(UART0_BASE + 0x24))
#define rUART0_FBRD						(*(volatile unsigned int *)(UART0_BASE + 0x28))
#define rUART0_LCR_H					(*(volatile unsigned int *)(UART0_BASE + 0x2C))
#define rUART0_CR						(*(volatile unsigned int *)(UART0_BASE + 0x30))
#define rUART0_IFLS						(*(volatile unsigned int *)(UART0_BASE + 0x34))
#define rUART0_IMSC						(*(volatile unsigned int *)(UART0_BASE + 0x38))
#define rUART0_RIS						(*(volatile unsigned int *)(UART0_BASE + 0x3C))
#define rUART0_MIS						(*(volatile unsigned int *)(UART0_BASE + 0x40))
#define rUART0_ICR						(*(volatile unsigned int *)(UART0_BASE + 0x44))
#define rUART0_DMACR					(*(volatile unsigned int *)(UART0_BASE + 0x48))


/* SSI */
#define rSPI_CONTROLREG             (*(volatile unsigned int *)(SSI_BASE + 0x08))
#define rSPI_CONFIGREG              (*(volatile unsigned int *)(SSI_BASE + 0x0C))
#define rSPI_INTREG                 (*(volatile unsigned int *)(SSI_BASE + 0x10))
#define rSPI_DMAREG                 (*(volatile unsigned int *)(SSI_BASE + 0x14))
#define rSPI_STATUSREG              (*(volatile unsigned int *)(SSI_BASE + 0x18))
#define rSPI_PERIODREG              (*(volatile unsigned int *)(SSI_BASE + 0x1C))
#define rSPI_TESTREG                (*(volatile unsigned int *)(SSI_BASE + 0x20))
#define rSPI_MSGREG                 (*(volatile unsigned int *)(SSI_BASE + 0x40))
#define rSPI_RXDATA                 (*(volatile unsigned int *)(SSI_BASE + 0x50))
#define rSPI_TXDATA                 (*(volatile unsigned int *)(SSI_BASE + 0x460))
#define rSPI_TXFIFO                   (SSI_BASE + 0x460)
#define rSPI_RXFIFO                   (SSI_BASE + 0x50) 

/* SSI0 */
#define rSPI0_CONTROLREG             (*(volatile unsigned int *)(SSI_BASE + 0x08))
#define rSPI0_CONFIGREG              (*(volatile unsigned int *)(SSI_BASE + 0x0C))
#define rSPI0_INTREG                 (*(volatile unsigned int *)(SSI_BASE + 0x10))
#define rSPI0_DMAREG                 (*(volatile unsigned int *)(SSI_BASE + 0x14))
#define rSPI0_STATUSREG              (*(volatile unsigned int *)(SSI_BASE + 0x18))
#define rSPI0_PERIODREG              (*(volatile unsigned int *)(SSI_BASE + 0x1C))
#define rSPI0_TESTREG                (*(volatile unsigned int *)(SSI_BASE + 0x20))
#define rSPI0_MSGREG                 (*(volatile unsigned int *)(SSI_BASE + 0x40))
#define rSPI0_RXDATA                 (*(volatile unsigned int *)(SSI_BASE + 0x50))
#define rSPI0_TXDATA                 (*(volatile unsigned int *)(SSI_BASE + 0x460))
#define rSPI0_TXFIFO                   (SSI_BASE + 0x460)
#define rSPI0_RXFIFO                   (SSI_BASE + 0x50) 
/* SSI1 */
#define rSPI1_CONTROLREG             (*(volatile unsigned int *)(SSI1_BASE + 0x08))
#define rSPI1_CONFIGREG              (*(volatile unsigned int *)(SSI1_BASE + 0x0C))
#define rSPI1_INTREG                 (*(volatile unsigned int *)(SSI1_BASE + 0x10))
#define rSPI1_DMAREG                 (*(volatile unsigned int *)(SSI1_BASE + 0x14))
#define rSPI1_STATUSREG              (*(volatile unsigned int *)(SSI1_BASE + 0x18))
#define rSPI1_PERIODREG              (*(volatile unsigned int *)(SSI1_BASE + 0x1C))
#define rSPI1_TESTREG                (*(volatile unsigned int *)(SSI1_BASE + 0x20))
#define rSPI1_MSGREG                 (*(volatile unsigned int *)(SSI1_BASE + 0x40))
#define rSPI1_RXDATA                 (*(volatile unsigned int *)(SSI1_BASE + 0x50))
#define rSPI1_TXDATA                 (*(volatile unsigned int *)(SSI1_BASE + 0x460))
#define rSPI1_TXFIFO                   (SSI1_BASE + 0x460)
#define rSPI1_RXFIFO                   (SSI1_BASE + 0x50) 


/* IIC */
#define rIIC_CON							*((volatile unsigned int *)(IIC_BASE + 0x000))
#define rIIC_TAR							*((volatile unsigned int *)(IIC_BASE + 0x004))
#define rIIC_SAR							*((volatile unsigned int *)(IIC_BASE + 0x008))
#define rIIC_HS_MADDR					*((volatile unsigned int *)(IIC_BASE + 0x00C))
#define rIIC_DATA_CMD					*((volatile unsigned int *)(IIC_BASE + 0x010))
#define rIIC_SS_SCL_HCNT					*((volatile unsigned int *)(IIC_BASE + 0x014))
#define rIIC_SS_SCL_LCNT					*((volatile unsigned int *)(IIC_BASE + 0x018))
#define rIIC_FS_SCL_HCNT					*((volatile unsigned int *)(IIC_BASE + 0x01C))
#define rIIC_FS_SCL_LCNT					*((volatile unsigned int *)(IIC_BASE + 0x020))
#define rIIC_HS_SCL_HCNT					*((volatile unsigned int *)(IIC_BASE + 0x024))
#define rIIC_HS_SCL_LCNT					*((volatile unsigned int *)(IIC_BASE + 0x028))
#define rIIC_INTR_STAT					*((volatile unsigned int *)(IIC_BASE + 0x02C))
#define rIIC_INTR_MASK					*((volatile unsigned int *)(IIC_BASE + 0x030))
#define rIIC_RAW_INTR_STAT				*((volatile unsigned int *)(IIC_BASE + 0x034))
#define rIIC_RX_TL						*((volatile unsigned int *)(IIC_BASE + 0x038))
#define rIIC_TX_TL						*((volatile unsigned int *)(IIC_BASE + 0x03C))
#define rIIC_CLR_INTR					*((volatile unsigned int *)(IIC_BASE + 0x040))
#define rIIC_CLR_RX_UNDER				*((volatile unsigned int *)(IIC_BASE + 0x044))
#define rIIC_CLR_RX_OVER					*((volatile unsigned int *)(IIC_BASE + 0x048))
#define rIIC_CLR_TX_OVER					*((volatile unsigned int *)(IIC_BASE + 0x04C))
#define rIIC_CLR_RD_REQ					*((volatile unsigned int *)(IIC_BASE + 0x050))
#define rIIC_CLR_TX_ABRT					*((volatile unsigned int *)(IIC_BASE + 0x054))
#define rIIC_CLR_RX_DONE					*((volatile unsigned int *)(IIC_BASE + 0x058))
#define rIIC_CLR_ACTIVITY				*((volatile unsigned int *)(IIC_BASE + 0x05C))
#define rIIC_CLR_STOP_DET				*((volatile unsigned int *)(IIC_BASE + 0x060))
#define rIIC_CLR_START_DET				*((volatile unsigned int *)(IIC_BASE + 0x064))
#define rIIC_CLR_GEN_CALL				*((volatile unsigned int *)(IIC_BASE + 0x068))
#define rIIC_ENABLE						*((volatile unsigned int *)(IIC_BASE + 0x06C))
#define rIIC_STATUS						*((volatile unsigned int *)(IIC_BASE + 0x070))
#define rIIC_TXFLR						*((volatile unsigned int *)(IIC_BASE + 0x074))
#define rIIC_RXFLR						*((volatile unsigned int *)(IIC_BASE + 0x078))
#define rIIC_TX_ABRT_SOURCE				*((volatile unsigned int *)(IIC_BASE + 0x080))
#define rIIC_DMA_CR						*((volatile unsigned int *)(IIC_BASE + 0x088))
#define rIIC_DMA_TDLR					*((volatile unsigned int *)(IIC_BASE + 0x08C))
#define rIIC_DMA_RDLR					*((volatile unsigned int *)(IIC_BASE + 0x090))
#define rIIC_ACK_GENERAL_CALL			*((volatile unsigned int *)(IIC_BASE + 0x098))
#define rIIC_IC_DENOISE					*((volatile unsigned int *)(IIC_BASE + 0x0A0))
#define rIIC_CLR_ACTIVITY_DONE		*((volatile unsigned int *)(IIC_BASE + 0x0A4))
#define rIIC_COMP_PARAM_1				*((volatile unsigned int *)(IIC_BASE + 0x0F4))
#define rIIC_COMP_VERSION				*((volatile unsigned int *)(IIC_BASE + 0x0F8))
#define rIIC_COMP_TYPE					*((volatile unsigned int *)(IIC_BASE + 0x0FC))

/* IIS */
#define rIIS_SACR0							(*(volatile unsigned int *) (I2S_BASE + 0x00))
#define rIIS_SACR1							(*(volatile unsigned int *) (I2S_BASE + 0x04))
#define rIIS_DACR0							(*(volatile unsigned int *) (I2S_BASE + 0x08))
#define rIIS_DACR							   (*(volatile unsigned int *) (I2S_BASE + 0x08))
#define rIIS_DACR1							(*(volatile unsigned int *) (I2S_BASE + 0x10))
#define rIIS_SASR0							(*(volatile unsigned int *) (I2S_BASE + 0x0c))   //read-only
#define rIIS_SAIMR							(*(volatile unsigned int *) (I2S_BASE + 0x14))
#define rIIS_SAICR							(*(volatile unsigned int *) (I2S_BASE + 0x18))   //write-only
#define rIIS_ADCR0							(*(volatile unsigned int *) (I2S_BASE + 0x1c))
#define rIIS_SADR							(*(volatile unsigned int *) (I2S_BASE + 0x80))
#define rIIS_SATR							(*(volatile unsigned int *) (I2S_BASE + 0x80))
#define I2S_DATA_FIFO					(I2S_BASE + 0x80)

/* GPIO */
#define rGPIO_PA_MOD							(*(volatile unsigned int *)(GPIO_BASE + 0x00))
#define rGPIO_PA_RDATA						(*(volatile unsigned int *)(GPIO_BASE + 0x04))
#define rGPIO_PA_INTEN							(*(volatile unsigned int *)(GPIO_BASE + 0x08))
#define rGPIO_PA_LEVEL							(*(volatile unsigned int *)(GPIO_BASE + 0x0C))
#define rGPIO_PA_PEND							(*(volatile unsigned int *)(GPIO_BASE + 0x10))

#define rGPIO_PB_MOD							(*(volatile unsigned int *)(GPIO_BASE + 0x20))
#define rGPIO_PB_RDATA						(*(volatile unsigned int *)(GPIO_BASE + 0x24))
#define rGPIO_PB_INTEN						(*(volatile unsigned int *)(GPIO_BASE + 0x28))
#define rGPIO_PB_LEVEL							(*(volatile unsigned int *)(GPIO_BASE + 0x2C))
#define rGPIO_PB_PEND							(*(volatile unsigned int *)(GPIO_BASE + 0x30))

#define rGPIO_PC_MOD							(*(volatile unsigned int *)(GPIO_BASE + 0x40))
#define rGPIO_PC_RDATA						(*(volatile unsigned int *)(GPIO_BASE + 0x44))
#define rGPIO_PC_INTEN							(*(volatile unsigned int *)(GPIO_BASE + 0x48))
#define rGPIO_PC_LEVEL							(*(volatile unsigned int *)(GPIO_BASE + 0x4C))
#define rGPIO_PC_PEND							(*(volatile unsigned int *)(GPIO_BASE + 0x50))

#define rGPIO_PD_MOD							(*(volatile unsigned int *)(GPIO_BASE + 0x60))
#define rGPIO_PD_RDATA						(*(volatile unsigned int *)(GPIO_BASE + 0x64))
#define rGPIO_PD_INTEN							(*(volatile unsigned int *)(GPIO_BASE + 0x68))
#define rGPIO_PD_LEVEL							(*(volatile unsigned int *)(GPIO_BASE + 0x6C))
#define rGPIO_PD_PEND							(*(volatile unsigned int *)(GPIO_BASE + 0x70))

// GPIO_ debounce_enable
//	Bit	Type  Reset		Description 
// 7-0 	R/W 	0  		debounce_0-7 enable:   
//								bit0 is GPIO0   
//								bit1 is GPIO1 
//								bit2 is GPIO2
//								… 
//								bit7 is GPIO7 
//								1:enable   0:disable 
#define rGPIO_DEBOUNCE_ENABLE					(*(volatile unsigned int *)(GPIO_BASE + 0xA0))

// GPIO_ debounce_cnt_0
// Debounce counter for eliminating bounce . 
// 	After the time of DBNCNT, we’ll think the value of the signal is stable and valid. 
//	Bit	Type  Reset				Description 
// 23-0 	R/W 	24’hffffffff  debounce_cnt_0 for gpio 0 
#define rGPIO_DEBOUNCE_CNT_0					(*(volatile unsigned int *)(GPIO_BASE + 0x80))

// GPIO_ debounce_cnt_1
// Debounce counter for eliminating bounce . 
// 	After the time of DBNCNT, we’ll think the value of the signal is stable and valid. 
//	Bit	Type  Reset				Description 
// 23-0 	R/W 	24’hffffffff  debounce_cnt_0 for gpio 1 
#define rGPIO_DEBOUNCE_CNT_1					(*(volatile unsigned int *)(GPIO_BASE + 0x84))

// GPIO_ debounce_cnt_2
// Debounce counter for eliminating bounce . 
// 	After the time of DBNCNT, we’ll think the value of the signal is stable and valid. 
//	Bit	Type  Reset				Description 
// 23-0 	R/W 	24’hffffffff  debounce_cnt_0 for gpio 2 
#define rGPIO_DEBOUNCE_CNT_2					(*(volatile unsigned int *)(GPIO_BASE + 0x88))

// GPIO_ debounce_cnt_3
// Debounce counter for eliminating bounce . 
// 	After the time of DBNCNT, we’ll think the value of the signal is stable and valid. 
//	Bit	Type  Reset				Description 
// 23-0 	R/W 	24’hffffffff  debounce_cnt_0 for gpio 3
#define rGPIO_DEBOUNCE_CNT_3					(*(volatile unsigned int *)(GPIO_BASE + 0x8c))

// GPIO_ debounce_cnt_4
// Debounce counter for eliminating bounce . 
// 	After the time of DBNCNT, we’ll think the value of the signal is stable and valid. 
//	Bit	Type  Reset				Description 
// 23-0 	R/W 	24’hffffffff  debounce_cnt_0 for gpio 4
#define rGPIO_DEBOUNCE_CNT_4					(*(volatile unsigned int *)(GPIO_BASE + 0x90))

// GPIO_ debounce_cnt_5
// Debounce counter for eliminating bounce . 
// 	After the time of DBNCNT, we’ll think the value of the signal is stable and valid. 
//	Bit	Type  Reset				Description 
// 23-0 	R/W 	24’hffffffff  debounce_cnt_0 for gpio 5
#define rGPIO_DEBOUNCE_CNT_5					(*(volatile unsigned int *)(GPIO_BASE + 0x94))

// GPIO_ debounce_cnt_6
// Debounce counter for eliminating bounce . 
// 	After the time of DBNCNT, we’ll think the value of the signal is stable and valid. 
//	Bit	Type  Reset				Description 
// 23-0 	R/W 	24’hffffffff  debounce_cnt_0 for gpio 6
#define rGPIO_DEBOUNCE_CNT_6					(*(volatile unsigned int *)(GPIO_BASE + 0x98))

// GPIO_ debounce_cnt_7
// Debounce counter for eliminating bounce . 
// 	After the time of DBNCNT, we’ll think the value of the signal is stable and valid. 
//	Bit	Type  Reset				Description 
// 23-0 	R/W 	24’hffffffff  debounce_cnt_0 for gpio 7
#define rGPIO_DEBOUNCE_CNT_7					(*(volatile unsigned int *)(GPIO_BASE + 0x9c))

/* Remote */
#define rRC_DATA0						(*(volatile unsigned int *)(RC_BASE + 0x00))
#define rRC_DATA1						(*(volatile unsigned int *)(RC_BASE + 0x04))
#define rRC_DATA2						(*(volatile unsigned int *)(RC_BASE + 0x08))
#define rRC_DATA3						(*(volatile unsigned int *)(RC_BASE + 0x0C))
#define rRC_DATA4						(*(volatile unsigned int *)(RC_BASE + 0x10))
#define rRC_DATA5						(*(volatile unsigned int *)(RC_BASE + 0x14))
#define rRC_DATA6						(*(volatile unsigned int *)(RC_BASE + 0x18))
#define rRC_DATA7						(*(volatile unsigned int *)(RC_BASE + 0x1C))
#define rRC_CODEBUF						(*(volatile unsigned short*)(RC_BASE + 0x20))  /* RC Code Value Buffer*/
#define rRC_CODEVAL						(*(volatile unsigned char *)(RC_BASE + 0x24))  /* RC Code Value */
#define rRC_KEYVAL						(*(volatile unsigned char *)(RC_BASE + 0x28))  /* RC Key Value */
#define rRC_STATUS						(*(volatile unsigned char *)(RC_BASE + 0x2C))
#define rRC_RT_USER_CODE_3_4             (*(volatile unsigned char *)(RC_BASE + 0x30))
#define rRC_RT_INTER_REQ_CLR             (*(volatile unsigned char *)(RC_BASE + 0x34))

/* 12bit ADC */
#define rADC_CTR					(*(volatile unsigned int*)(ADC_BASE + 0x00))
#define rADC_CFG					(*(volatile unsigned int*)(ADC_BASE + 0x04))
#define rADC_IMR					(*(volatile unsigned int*)(ADC_BASE + 0x08))
#define rADC_STA					(*(volatile unsigned int*)(ADC_BASE + 0x0C))
#define rADC_BAT					(*(volatile unsigned int*)(ADC_BASE + 0x10))
#define rADC_AUX0					(*(volatile unsigned int*)(ADC_BASE + 0x14))
#define rADC_AUX1					(*(volatile unsigned int*)(ADC_BASE + 0x18)) 
#define rADC_AUX2					(*(volatile unsigned int*)(ADC_BASE + 0x1C))
#define rADC_AUX3					(*(volatile unsigned int*)(ADC_BASE + 0x20)) 
#define rADC_AUX(ch)				(*(volatile unsigned int*)(ADC_BASE + 0x14 + (ch) * 4)) 
#define rADC_PANXZ1					(*(volatile unsigned int*)(ADC_BASE + 0x24)) 
#define rADC_PANXZ2					(*(volatile unsigned int*)(ADC_BASE + 0x28)) 
#define rADC_DBNCNT					(*(volatile unsigned int*)(ADC_BASE + 0x2C)) 
#define rADC_DETINTER				(*(volatile unsigned int*)(ADC_BASE + 0x30))
#define rADC_SCTR					(*(volatile unsigned int*)(ADC_BASE + 0x34))

#define	SDMMC_DEV_0 	 	0
#define	SDMMC_DEV_1	 		1
//#define	SDMMC_DEV_COUNT	2	// SDMMC支持的设备个数
#define	SDMMC_DEV_COUNT	1

#if 0
enum {
	SDMMC_DEV_0 = 0,
	SDMMC_DEV_1,
	SDMMC_DEV_COUNT		// SDMMC支持的设备个数
};
#endif


#if 0
/* SD/MMC card */
#define rSDMMC_CTRL							(*(volatile unsigned int *)(SDHC0_BASE + 0x00))
#define rSDMMC_PWREN						(*(volatile unsigned int *)(SDHC0_BASE + 0x04))
#define rSDMMC_CLKDIV						(*(volatile unsigned int *)(SDHC0_BASE + 0x08))
#define rSDMMC_CLKSRC						(*(volatile unsigned int *)(SDHC0_BASE + 0x0C))
#define rSDMMC_CLKENA						(*(volatile unsigned int *)(SDHC0_BASE + 0x10))
#define rSDMMC_TMOUT						(*(volatile unsigned int *)(SDHC0_BASE + 0x14))
#define rSDMMC_CTYPE						(*(volatile unsigned int *)(SDHC0_BASE + 0x18))
#define rSDMMC_BLKSIZ						(*(volatile unsigned int *)(SDHC0_BASE + 0x1C))
#define rSDMMC_BYTCNT						(*(volatile unsigned int *)(SDHC0_BASE + 0x20))
#define rSDMMC_INTMASK						(*(volatile unsigned int *)(SDHC0_BASE + 0x24))
#define rSDMMC_CMDARG						(*(volatile unsigned int *)(SDHC0_BASE + 0x28))
#define rSDMMC_CMD							(*(volatile unsigned int *)(SDHC0_BASE + 0x2C))
#define rSDMMC_RESP0						(*(volatile unsigned int *)(SDHC0_BASE + 0x30))
#define rSDMMC_RESP1						(*(volatile unsigned int *)(SDHC0_BASE + 0x34))
#define rSDMMC_RESP2						(*(volatile unsigned int *)(SDHC0_BASE + 0x38))
#define rSDMMC_RESP3						(*(volatile unsigned int *)(SDHC0_BASE + 0x3C))
#define rSDMMC_MINTSTS						(*(volatile unsigned int *)(SDHC0_BASE + 0x40))
#define rSDMMC_RINTSTS						(*(volatile unsigned int *)(SDHC0_BASE + 0x44))
#define rSDMMC_STATUS						(*(volatile unsigned int *)(SDHC0_BASE + 0x48))
#define rSDMMC_FIFOTH						(*(volatile unsigned int *)(SDHC0_BASE + 0x4C))
#define rSDMMC_CDETECT						(*(volatile unsigned int *)(SDHC0_BASE + 0x50))
#define rSDMMC_WRTPRT						(*(volatile unsigned int *)(SDHC0_BASE + 0x54))
#define rSDMMC_GPIO							(*(volatile unsigned int *)(SDHC0_BASE + 0x58))
#define rSDMMC_TCBCNT						(*(volatile unsigned int *)(SDHC0_BASE + 0x5C))
#define rSDMMC_TBBCNT						(*(volatile unsigned int *)(SDHC0_BASE + 0x60))
#define rSDMMC_DEBNCE						(*(volatile unsigned int *)(SDHC0_BASE + 0x64))
#define rSDMMC_USRID						(*(volatile unsigned int *)(SDHC0_BASE + 0x68))
#define rSDMMC_VERID						(*(volatile unsigned int *)(SDHC0_BASE + 0x6C))
#define rSDMMC_HCON						(*(volatile unsigned int *)(SDHC0_BASE + 0x70))
#define rSDMMC_DATA						(*(volatile unsigned int *)(SDHC0_BASE + 0x100))
#define SDMMC_FIFO							(SDHC0_BASE + 0x100)
#endif

/* PWM */
#define rPWM_ENA0						(*(volatile unsigned int *)(PWM_BASE + 0x00))
#define rPWM_DUTY0						(*(volatile unsigned int *)(PWM_BASE + 0x04))
#define rPWM_CNTR0						(*(volatile unsigned int *)(PWM_BASE + 0x08))
#define rPWM_ENA1						(*(volatile unsigned int *)(PWM_BASE + 0x10))
#define rPWM_DUTY1						(*(volatile unsigned int *)(PWM_BASE + 0x14))
#define rPWM_CNTR1						(*(volatile unsigned int *)(PWM_BASE + 0x18))
#define rPWM_ENA2						(*(volatile unsigned int *)(PWM_BASE + 0x20))
#define rPWM_DUTY2						(*(volatile unsigned int *)(PWM_BASE + 0x24))
#define rPWM_CNTR2						(*(volatile unsigned int *)(PWM_BASE + 0x28))
#define rPWM_ENA3						(*(volatile unsigned int *)(PWM_BASE + 0x30))
#define rPWM_DUTY3						(*(volatile unsigned int *)(PWM_BASE + 0x34))
#define rPWM_CNTR3						(*(volatile unsigned int *)(PWM_BASE + 0x38))

/* UART1 */
#define rUART1_DR						(*(volatile unsigned int *)(UART1_BASE + 0x00))
#define rUART1_RSR						(*(volatile unsigned int *)(UART1_BASE + 0x04))
#define rUART1_FR						(*(volatile unsigned int *)(UART1_BASE + 0x18))
#define rUART1_ILPR						(*(volatile unsigned int *)(UART1_BASE + 0x20))
#define rUART1_IBRD						(*(volatile unsigned int *)(UART1_BASE + 0x24))
#define rUART1_FBRD						(*(volatile unsigned int *)(UART1_BASE + 0x28))
#define rUART1_LCR_H					(*(volatile unsigned int *)(UART1_BASE + 0x2C))
#define rUART1_CR						(*(volatile unsigned int *)(UART1_BASE + 0x30))
#define rUART1_IFLS						(*(volatile unsigned int *)(UART1_BASE + 0x34))
#define rUART1_IMSC						(*(volatile unsigned int *)(UART1_BASE + 0x38))
#define rUART1_RIS						(*(volatile unsigned int *)(UART1_BASE + 0x3C))
#define rUART1_MIS						(*(volatile unsigned int *)(UART1_BASE + 0x40))
#define rUART1_ICR						(*(volatile unsigned int *)(UART1_BASE + 0x44))
#define rUART1_DMACR					(*(volatile unsigned int *)(UART1_BASE + 0x48))

/* UART2 */
#define rUART2_DR						(*(volatile unsigned int *)(UART2_BASE + 0x00))
#define rUART2_RSR						(*(volatile unsigned int *)(UART2_BASE + 0x04))
#define rUART2_FR						(*(volatile unsigned int *)(UART2_BASE + 0x18))
#define rUART2_ILPR						(*(volatile unsigned int *)(UART2_BASE + 0x20))
#define rUART2_IBRD						(*(volatile unsigned int *)(UART2_BASE + 0x24))
#define rUART2_FBRD						(*(volatile unsigned int *)(UART2_BASE + 0x28))
#define rUART2_LCR_H					(*(volatile unsigned int *)(UART2_BASE + 0x2C))
#define rUART2_CR						(*(volatile unsigned int *)(UART2_BASE + 0x30))
#define rUART2_IFLS						(*(volatile unsigned int *)(UART2_BASE + 0x34))
#define rUART2_IMSC						(*(volatile unsigned int *)(UART2_BASE + 0x38))
#define rUART2_RIS						(*(volatile unsigned int *)(UART2_BASE + 0x3C))
#define rUART2_MIS						(*(volatile unsigned int *)(UART2_BASE + 0x40))
#define rUART2_ICR						(*(volatile unsigned int *)(UART2_BASE + 0x44))
#define rUART2_DMACR					(*(volatile unsigned int *)(UART2_BASE + 0x48))


/* UART3 */
#define rUART3_DR						(*(volatile unsigned int *)(UART3_BASE + 0x00))
#define rUART3_RSR						(*(volatile unsigned int *)(UART3_BASE + 0x04))
#define rUART3_FR						(*(volatile unsigned int *)(UART3_BASE + 0x18))
#define rUART3_ILPR						(*(volatile unsigned int *)(UART3_BASE + 0x20))
#define rUART3_IBRD						(*(volatile unsigned int *)(UART3_BASE + 0x24))
#define rUART3_FBRD						(*(volatile unsigned int *)(UART3_BASE + 0x28))
#define rUART3_LCR_H					(*(volatile unsigned int *)(UART3_BASE + 0x2C))
#define rUART3_CR						(*(volatile unsigned int *)(UART3_BASE + 0x30))
#define rUART3_IFLS						(*(volatile unsigned int *)(UART3_BASE + 0x34))
#define rUART3_IMSC						(*(volatile unsigned int *)(UART3_BASE + 0x38))
#define rUART3_RIS						(*(volatile unsigned int *)(UART3_BASE + 0x3C))
#define rUART3_MIS						(*(volatile unsigned int *)(UART3_BASE + 0x40))
#define rUART3_ICR						(*(volatile unsigned int *)(UART3_BASE + 0x44))
#define rUART3_DMACR					(*(volatile unsigned int *)(UART3_BASE + 0x48))

/* DE_INTERLACE */
// Deinerlace start//this signal is a pulse to enable the FSM work
//	Bit	Type	Reset		Description
//	31-1	R		0			N/A
//	0		W		0			Deinterlace start enable : when FSM is idle write 1 to start the FSM.
#define rDe_start			(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x00))

//	Bit	Type	Reset		Description
//	31-30	R		0			N/A
//	29		R/W	0			Field_1 : 
//									It indicate the S_addr_1:   1-even field ;0-odd field
//									The deinterlace will use this bit only when pingpong addr fetch mode 
//									enable(Switch_2_group_enable is enable).
//	28		R/W	0			Only_wr_1_field: 
//									only write the deinterlace data to the destination. 
//									When this bit is set. The y destination will change to s_addr_1. 
//									the Data mode should be 420.and the y data will write with skip one line length address from one line to next line.
//	27_20	R/W	0			line_length: 
//									the data of one line(devided by 8)
//	19-11	R/W	0			Total_line : 
//									the number of line one field 
//	10-3	R/W	0 			Data_stride: 
//									for interleaved data stored mode, the length between 2 line.
//									(the length should be devided by 8 )
//	2		R/W	0			Data_mode: 
//									0:420 ; 1:422;
//									When the data mode is 420, the deinterlace will ingore the u/v data. 
//	1		R/W	0	 		Field _0:
//									It indicate the S_addr_1:   1-even field; 0-odd field
//	0		R/W	0			Soft_reset : 
//									1-enable reset   
#define rDe_ctrl0			(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x04))

//	Bit	Type	Reset		Description
//	31-24	R/W	0 	 		VCAL_LEVEL_THD：垂直变化门限
//	23-8	R/W	0	 		VARY_LEVEL_THD：细节变化门限(7dh)
//	7-0	R/W	0			NOISE_LOW_THD：噪声低门限（18h）
#define rDe_ctrl1			(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x08))

//	Bit	Type	Reset		Description
//	31-24	R/W	0			motion_ctrl_reg3：for vary function counter(30h)
//	23-16	R/W	0 			LMOTN_LEVEL_THD : 低运动变化门限() 
//	15-8	R/W	0			EDGE_LEVEL_THD :边缘门限(42h)
//	7-0	R/W	0			HORZ_LEVEL_THD：水平变化门限(30h)
#define rDe_ctrl2			(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x0C))

//	Bit	Type	Reset		Description
//	31-24	R/W	0			motion_ctrl_reg2: MV1 or MV2 select control threshold for detail  
//	23-16	R/W	0			motion_ctrl_reg5：降噪门限(0bh)
//	15		R/W	0			bypass_mode：1 ：bypass denoise  0: denoise
//									this bit should set to 1 when data_mode is 420
//	14		R		0				N/A
//	13		R/W	0			PN
//	12		R/W	0			Filmmode_en
//	11		R/W	0			max_ycbcr_noise_ena: use for control the max noise control
//	10		R/W	0			field_merge_ena:
//									1:场合并 0 ：不合并 
//	9		R/W	0			ela_protect_sel ：
//									use for select ela interpolation mode only
//	8		R/W	0			line_copy_sel : Line copy select
//	7		R/W	0			disply_copy: Display coped video
//	6		R/W	0			disply_vect: Display motion data
//	5		R/W	0			disply_line_intra: Display intra line
//	4		R/W	0			disply_var: Display vary function
//	3		R/W	0			global_cnt_ena : Use for control the global counter
//	2		R/W	0 			max_ycbcr_ena :use for control the max control  
//	1		R/W	0			disply_mv_1: Display motion_value2
//	0		R/W	0			disply_mv_0 : Display motion_value1  
#define rDe_ctrl3			(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x10))

//	Bit	Type	Reset		Description
//	30-22	R/W	0			globalmotion_thd
//	21-15	R/W	0			film_margin：电影模式边缘大小
//	14-12	R/W	0 			film_tol : 电影模式匹配检测门限值2
//	11-8	R/W	0			film_thd_scale : 电影模式 scale 门限2
//	7-0	R/W	0			film_noise_thd：电影模式噪声门限0x10
#define rFilm_mode_ctrl	(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x14))

//	Bit	Type	Reset		Description
//	31-0	R/W	0			S_addr_0_0: Source address field 0 in group0
#define rDe_S_addr_0		(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x18))

//	Bit	Type	Reset		Description
//	31-0	R/W	0			S_addr_1_0: Source address field 1 in group0
#define rDe_S_addr_1		(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x1C))

//	Bit	Type	Reset		Description
//	31-0	R/W	0			S_addr_2_0: Source address field 2 in group0
#define rDe_S_addr_2		(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x20))

//	Bit	Type	Reset		Description
//	31-0	R/W	0			D_addr_y_0: y Destination address in group 0
#define rDe_des_addr_y	(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x24))

//	Bit	Type	Reset		Description
//	31-0	R/W	0			D_addr_u_0: u Destination address in group 0
#define rDe_des_addr_u	(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x28))

//	Bit	Type	Reset		Description
//	31-0	R/W	0			D_addr_v_0: v Destination address in group 0
#define rDe_des_addr_v	(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x2C))

//	Bit	Type	Reset		Description
//	31-0	R/W	0			S_addr_0_1: Source address field 0 in group1
#define rDe_S_addr_0_1	(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x30))

//	Bit	Type	Reset		Description
//	31-0	R/W	0			S_addr_1_1: Source address field 1 in group1
#define rDe_S_addr_1_1	(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x34))

//	Bit	Type	Reset		Description
//	31-0	R/W	0			S_addr_2_1:Source address field 2 in group1
#define rDe_S_addr_2_1	(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x38))

//	Bit	Type	Reset		Description
//	31-0	R/W	0			D_addr_y_0: y Destination address in group 1
#define rDe_des_addr_y_1	(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x3C))

//	Bit	Type	Reset		Description
//	31-0	R/W	0			D_addr_u_1: u Destination address in group 1
#define rDe_des_addr_u_1	(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x40))

//	Bit	Type	Reset		Description
//	31-0	R/W	0			D_addr_v_0: v Destination address in group 1
#define rDe_des_addr_v_1	(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x44))

//	Bit	Type	Reset		Description
//	31-2	R		0			N/A
//	1		R/W	0			AXI_wr error interrupt mask,
//									0: enable mask 1:disable mask
//	0		R/W	0			Field finish interrupt mask,
//									0: enable mask 1:disable mask
#define rDe_Int_mask		(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x48))

//	Bit	Type	Reset		Description
//	31-2	R		0			N/A
//	1		R		0			AXI_wr error interrupt 
//	0		R		0			Field finish interrupt
#define rDe_Raw_Int		(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x4C))

//	Bit	Type	Reset		Description
//	31-2	R		0			N/A
//	1		W		0			AXI_wr error interrupt clr : 
//									1:clear the interrupt
//	0		W		0			Field finish interrupt clr: 
//									1: clear the interrupt 
#define rDe_Int_clr		(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x50))

//	Bit	Type	Reset		Description
//	31-20	R		0			N/A
//	19-16	R		0			Write address state
//	15		R		0			N/A
//	14-12	R		0			Read address state
//	11		R		0			N/A
//	10-8	R		0			Write data state 
//	7-6	R		0			N/A
//	5-0	R		0			Main FSM state
#define rDe_status		(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x54))	

//	Bit	Type	Reset		Description
//	31-3	R		0			N/A
//	2		R/W	0			Switch_2_group_enable: 
//									1:enable ; 0 disbale
//									When this bit is enable,group0/1 addr and field 0/1 can be used, 
//										and when the field finish interrupt generate, the deinterlace will continue to work 
//										without to wait the software to enable the start if there is addr prepared. 
//										And deinterlace will fetch data from group 0 first.  
//									When this bit is disable, only group 0 addr and field_0 will be used. 
//	1		R/W	0			Group_addr_valid_1: 
//									1: the group 1 addr is enable
//	0		R/W	0			Group_Addr_valid_0: 
//									1: the group 0 addr is enable 
#define rAddr_switch_mode	(*(volatile unsigned int *)(DEINTERLACE_BASE + 0x58))	

/////////////////////////////////////////////////////
// apb_icu	32'h4040_0000, 4K
/////////////////////////////////////////////////////
// #define ICU_BASE		(0x40000000 + 0x400000)

#define rICSET			*((volatile unsigned int *)(ICU_BASE + 0x00))
#define rICPEND			*((volatile unsigned int *)(ICU_BASE + 0x04))
#define rICMODE			*((volatile unsigned int *)(ICU_BASE + 0x08))
#define rICMASK			*((volatile unsigned int *)(ICU_BASE + 0x0c))
#define rICLEVEL		*((volatile unsigned int *)(ICU_BASE + 0x10))
#define rIRQISPR		*((volatile unsigned int *)(ICU_BASE + 0x3c))
#define rIRQISPC		*((volatile unsigned int *)(ICU_BASE + 0x40))
#define rIVEC_ADDR      *((volatile unsigned int *)(ICU_BASE + 0x78))

#define DisableIntNum(num)	do { rICMASK |= (  1 << num);  }while(0)
#define EnableIntNum(num)	do { rICMASK &= (~(1 << num)); }while(0)

// DDR2/3-Lite DDR SDRAM Memory Controller
#define rMCTL_CCR				*((volatile unsigned int *)(MEMCTL_BASE + 0x00 * 4))
#define rMCTL_DCR				*((volatile unsigned int *)(MEMCTL_BASE + 0x01 * 4))
#define rMCTL_IOCR			*((volatile unsigned int *)(MEMCTL_BASE + 0x02 * 4))
#define rMCTL_CSR				*((volatile unsigned int *)(MEMCTL_BASE + 0x03 * 4))
#define rMCTL_DRR				*((volatile unsigned int *)(MEMCTL_BASE + 0x04 * 4))
#define rMCTL_TPR0			*((volatile unsigned int *)(MEMCTL_BASE + 0x05 * 4))
#define rMCTL_TPR1			*((volatile unsigned int *)(MEMCTL_BASE + 0x06 * 4))
#define rMCTL_TPR2			*((volatile unsigned int *)(MEMCTL_BASE + 0x07 * 4))
#define rMCTL_TPR3			*((volatile unsigned int *)(MEMCTL_BASE + 0x2E * 4))
#define rMCTL_DLLCR			*((volatile unsigned int *)(MEMCTL_BASE + 0x08 * 4))
#define rMCTL_DLLCR0			*((volatile unsigned int *)(MEMCTL_BASE + 0x09 * 4))
#define rMCTL_DLLCR1			*((volatile unsigned int *)(MEMCTL_BASE + 0x0A * 4))
#define rMCTL_DLLCR2			*((volatile unsigned int *)(MEMCTL_BASE + 0x0B * 4))
#define rMCTL_DLLCR3			*((volatile unsigned int *)(MEMCTL_BASE + 0x0C * 4))
#define rMCTL_DLLCR4			*((volatile unsigned int *)(MEMCTL_BASE + 0x0D * 4))
#define rMCTL_DLLCR5			*((volatile unsigned int *)(MEMCTL_BASE + 0x0E * 4))
#define rMCTL_DLLCR6			*((volatile unsigned int *)(MEMCTL_BASE + 0x0F * 4))
#define rMCTL_DLLCR7			*((volatile unsigned int *)(MEMCTL_BASE + 0x10 * 4))
#define rMCTL_DLLCR8			*((volatile unsigned int *)(MEMCTL_BASE + 0x11 * 4))
#define rMCTL_DLLCR9			*((volatile unsigned int *)(MEMCTL_BASE + 0x12 * 4))
#define rMCTL_RSLR0			*((volatile unsigned int *)(MEMCTL_BASE + 0x13 * 4))
#define rMCTL_RSLR1			*((volatile unsigned int *)(MEMCTL_BASE + 0x14 * 4))
#define rMCTL_RSLR2			*((volatile unsigned int *)(MEMCTL_BASE + 0x15 * 4))
#define rMCTL_RSLR3			*((volatile unsigned int *)(MEMCTL_BASE + 0x16 * 4))
#define rMCTL_RDGR0			*((volatile unsigned int *)(MEMCTL_BASE + 0x17 * 4))
#define rMCTL_RDGR1			*((volatile unsigned int *)(MEMCTL_BASE + 0x18 * 4))
#define rMCTL_RDGR2			*((volatile unsigned int *)(MEMCTL_BASE + 0x19 * 4))
#define rMCTL_RDGR3			*((volatile unsigned int *)(MEMCTL_BASE + 0x1A * 4))
#define rMCTL_DQTR0			*((volatile unsigned int *)(MEMCTL_BASE + 0x1B * 4))
#define rMCTL_DQTR1			*((volatile unsigned int *)(MEMCTL_BASE + 0x1C * 4))
#define rMCTL_DQTR2			*((volatile unsigned int *)(MEMCTL_BASE + 0x1D * 4))
#define rMCTL_DQTR3			*((volatile unsigned int *)(MEMCTL_BASE + 0x1E * 4))
#define rMCTL_DQTR4			*((volatile unsigned int *)(MEMCTL_BASE + 0x1F * 4))
#define rMCTL_DQTR5			*((volatile unsigned int *)(MEMCTL_BASE + 0x20 * 4))
#define rMCTL_DQTR6			*((volatile unsigned int *)(MEMCTL_BASE + 0x21 * 4))
#define rMCTL_DQTR7			*((volatile unsigned int *)(MEMCTL_BASE + 0x22 * 4))
#define rMCTL_DQTR8			*((volatile unsigned int *)(MEMCTL_BASE + 0x23 * 4))
#define rMCTL_DQSTR			*((volatile unsigned int *)(MEMCTL_BASE + 0x24 * 4))
#define rMCTL_DQSBTR			*((volatile unsigned int *)(MEMCTL_BASE + 0x25 * 4))
#define rMCTL_ODTCR			*((volatile unsigned int *)(MEMCTL_BASE + 0x26 * 4))
#define rMCTL_DTR0			*((volatile unsigned int *)(MEMCTL_BASE + 0x27 * 4))
#define rMCTL_DTR1			*((volatile unsigned int *)(MEMCTL_BASE + 0x28 * 4))
#define rMCTL_DTAR			*((volatile unsigned int *)(MEMCTL_BASE + 0x29 * 4))
#define rMCTL_ZQCR0			*((volatile unsigned int *)(MEMCTL_BASE + 0x2A * 4))
#define rMCTL_ZQCR1			*((volatile unsigned int *)(MEMCTL_BASE + 0x2B * 4))
#define rMCTL_ZQCR2			*((volatile unsigned int *)(MEMCTL_BASE + 0x2C * 4))
#define rMCTL_ZQSR			*((volatile unsigned int *)(MEMCTL_BASE + 0x2D * 4))
#define rMCTL_ALPMR			*((volatile unsigned int *)(MEMCTL_BASE + 0x2F * 4))
#define rMCTL_MR0				*((volatile unsigned int *)(MEMCTL_BASE + 0x7C * 4))
#define rMCTL_MR1				*((volatile unsigned int *)(MEMCTL_BASE + 0x7D * 4))
#define rMCTL_MR2				*((volatile unsigned int *)(MEMCTL_BASE + 0x7E * 4))
#define rMCTL_MR3				*((volatile unsigned int *)(MEMCTL_BASE + 0x7F * 4))
#define rMCTL_HPCR0			*((volatile unsigned int *)(MEMCTL_BASE + 0x80 * 4))
// HPCR0-31
#define rMCTL_HPCR(x)		*((volatile unsigned int *)(MEMCTL_BASE + (0x80 + x) * 4))
#define rMCTL_PQCR0			*((volatile unsigned int *)(MEMCTL_BASE + 0xA0 * 4))
#define rMCTL_PQCR1			*((volatile unsigned int *)(MEMCTL_BASE + 0xA1 * 4))
#define rMCTL_PQCR2			*((volatile unsigned int *)(MEMCTL_BASE + 0xA2 * 4))
#define rMCTL_PQCR3			*((volatile unsigned int *)(MEMCTL_BASE + 0xA3 * 4))
#define rMCTL_PQCR4			*((volatile unsigned int *)(MEMCTL_BASE + 0xA4 * 4))
#define rMCTL_PQCR5			*((volatile unsigned int *)(MEMCTL_BASE + 0xA5 * 4))
#define rMCTL_PQCR6			*((volatile unsigned int *)(MEMCTL_BASE + 0xA6 * 4))
#define rMCTL_PQCR7			*((volatile unsigned int *)(MEMCTL_BASE + 0xA7 * 4))
#define rMCTL_MMGCR			*((volatile unsigned int *)(MEMCTL_BASE + 0xA8 * 4))

#endif // _ARK1960_H_































