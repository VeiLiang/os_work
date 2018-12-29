#ifndef _ARK1960_IRQS_H_
#define _ARK1960_IRQS_H_

/* interrupt vector definition */
#define LCD_INT               			0		// LCD interrupt
//#define SATA_INT								1		// SATA interrupt	
#define CODEC_ENCODER_INT					1		// codec encoder interrupt(H264/JPEG) 
#define ITU656_INT            			2	
#define USB_INT               			3		// USB interrupt
#define USBDMA_INT            			4		// USB DMA interrupt
#define DMA_INT								5		// AHB DMA interrupt
#define CODEC_DECODER_INT					6		// codec decoder interrupt(H264/JPEG)
#define DEINTERLACE_INT						7
#define NANDFLASH_INT						8
#define SDHC0_INT								9		// SDMMC interrupt
#define MAC_INT								10
#define SDHC1_INT								11		// SDMMC 1 interrupt
#define L2CC_INT               			12		// L2CC interrupt
#define PWM_INT	            			13		// PWM interrupt
#define I2S_INT               			14	
#define SSP_INT								15	
#define ADC_INT               			16
#define WDT_INT               			17		// Watchdog interrupt
#define TIMER0_INT            			18
#define TIMER12345_INT            		19

#define ISP_SCALE_INT            		20		// 
#define SPI1_INT               			21		// 
#define RTC_INT								22		// RTC period interrupt / RTC alarm interrupt
#define RCRT_INT								23
#define GPIO_INT								24
#define UART0_INT								25		// UART0 interrupt
//#define UART1_INT								26		// UART1 interrupt	
#define UART123_INT							26		// UART123 interrupt	
#define ISP_INT							   27		// ISP_INT interrupt
#define SCALE_INT								28		//UART3_INT UART3 interrupt
#define I2C_INT               			29		// I2C interrupt
#define I2S1_INT								30		// I2S1 interrupt
#define I2S2_INT								31		// I2S2 interrupt

#define RTC_PERIOD_INT						RTC_INT
#define SSI_INT								SSP_INT

#define PRIORITY_ZERO			0		//highest
#define PRIORITY_ONE				1
#define PRIORITY_TWO				2
#define PRIORITY_THREE			3
#define PRIORITY_FOUR			4
#define PRIORITY_FIVE			5
#define PRIORITY_SIX				6
#define PRIORITY_SEVEN			7
#define PRIORITY_EIGHT			8
#define PRIORITY_NINE			9
#define PRIORITY_TEN				10
#define PRIORITY_ELEVEN			11
#define PRIORITY_TWELEVE		12
#define PRIORITY_THIRTEEN		13
#define PRIORITY_FOURTEEN		14
#define PRIORITY_FIFTEEN		15		//lowest

#define	INTR_HANDLER_DECLARE(intr)	\
extern void ark1960_##intr##_handler (void);

#define	INTR_HANDLER_DEFINE(intr)	\
void 	ark1960_##intr##_handler (void)

INTR_HANDLER_DECLARE(_LCD_INT)
INTR_HANDLER_DECLARE(_SATA_INT)
INTR_HANDLER_DECLARE(_ITU656_INT)
INTR_HANDLER_DECLARE(_USB_INT)             
INTR_HANDLER_DECLARE(_USBDMA_INT)          
INTR_HANDLER_DECLARE(_DMA_INT)				
INTR_HANDLER_DECLARE(_MAE2_INT)			
INTR_HANDLER_DECLARE(_DEINTERLACE_INT)		
INTR_HANDLER_DECLARE(_NANDFLASH_INT)		
INTR_HANDLER_DECLARE(_SDHC0_INT)			
INTR_HANDLER_DECLARE(_MAC_INT)				
INTR_HANDLER_DECLARE(_SDHC1_INT)			
INTR_HANDLER_DECLARE(_USB1_INT)            
INTR_HANDLER_DECLARE(_USB1DMA_INT)         
INTR_HANDLER_DECLARE(_I2S_INT)             
INTR_HANDLER_DECLARE(_SSP_INT)				
INTR_HANDLER_DECLARE(_ADC_INT)             
INTR_HANDLER_DECLARE(_WDT_INT)             
INTR_HANDLER_DECLARE(_TIMER0_INT)          
INTR_HANDLER_DECLARE(_TIMER1_INT)          
INTR_HANDLER_DECLARE(_TIMER2_INT)          
INTR_HANDLER_DECLARE(_TIMER3_INT)          
INTR_HANDLER_DECLARE(_RTC_INT)				
INTR_HANDLER_DECLARE(_RCRT_INT)			
INTR_HANDLER_DECLARE(_GPIO_INT)			
INTR_HANDLER_DECLARE(_UART0_INT)			
INTR_HANDLER_DECLARE(_UART1_INT)			
INTR_HANDLER_DECLARE(_UART2_INT)			
INTR_HANDLER_DECLARE(_UART3_INT)
INTR_HANDLER_DECLARE(_I2C_INT)             
INTR_HANDLER_DECLARE(_I2S1_INT)			
INTR_HANDLER_DECLARE(_I2S2_INT)			

#define	INTR_HANDLER_PROCESS(intr)	\
ark1960_##intr##_handler()


extern void irq_mask   (unsigned int irqno);
extern void irq_unmask (unsigned int irqno);

typedef  void (*pfnIRQ)(void);

typedef  void (*pfnIRQUSER)(void *);


void irq_disable(unsigned char irq);

void irq_enable(unsigned char irq);

int request_irq(unsigned char irq, unsigned char priority, pfnIRQ handler);


#endif	// _ARK1960_IRQS_H_
