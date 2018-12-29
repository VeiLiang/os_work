#ifndef __HARDWARE_H
#define __HARDWARE_H
#include <xm_proj_define.h>



/////////////////////////////////////////////////////
/* USB */
#if _USB_DEV_ == _USB_DEV_0_
#define USB_BASE				(0x70060000) 
#elif _USB_DEV_ == _USB_DEV_1_
#define USB_BASE				(0x70070000) 
#endif
                            	
#define USB_FADDR      			(*(volatile unsigned char *)(USB_BASE + 0x000))
#define USB_Power      			(*(volatile unsigned char *)(USB_BASE + 0x001))
                       			                                 
#define USB_IntrTx     			(*(volatile unsigned char *)(USB_BASE + 0x002))
#define USB_IntrRx     			(*(volatile unsigned char *)(USB_BASE + 0x004))
#define USB_IntrTxEn   			(*(volatile unsigned char *)(USB_BASE + 0x006))
#define USB_IntrRxEn   			(*(volatile unsigned char *)(USB_BASE + 0x008))
#define USB_Intr       			(*(volatile unsigned char *)(USB_BASE + 0x00a))
#define USB_IntrEn     			(*(volatile unsigned char *)(USB_BASE + 0x00b))
#define USB_FRAMEL     			(*(volatile unsigned char *)(USB_BASE + 0x00c))
#define USB_FRAMEH     			(*(volatile unsigned char *)(USB_BASE + 0x00d))
#define USB_INDEX      			(*(volatile unsigned char *)(USB_BASE + 0x00e))
#define USB_TESTMODE   			(*(volatile unsigned char *)(USB_BASE + 0x00f))
#define USB_TXMAXPL    			(*(volatile unsigned char *)(USB_BASE + 0x010))
#define USB_TXMAXPH    			(*(volatile unsigned char *)(USB_BASE + 0x011))
#define USB_TXCSRL     			(*(volatile unsigned char *)(USB_BASE + 0x012)) 
#define USB_CSR0L      			(*(volatile unsigned char *)(USB_BASE + 0x012)) 
#define USB_TXCSRH     			(*(volatile unsigned char *)(USB_BASE + 0x013)) 
#define USB_CSR0H      			(*(volatile unsigned char *)(USB_BASE + 0x013)) 
#define USB_RXMAXPL    			(*(volatile unsigned char *)(USB_BASE + 0x014))
#define USB_RXMAXPH    			(*(volatile unsigned char *)(USB_BASE + 0x015))
#define USB_RXCSRL     			(*(volatile unsigned char *)(USB_BASE + 0x016))
#define USB_RXCSRH     			(*(volatile unsigned char *)(USB_BASE + 0x017))
#define USB_CountL     			(*(volatile unsigned char *)(USB_BASE + 0x018))
#define USB_CountH     			(*(volatile unsigned char *)(USB_BASE + 0x019))
#define USB_TxType     			(*(volatile unsigned char *)(USB_BASE + 0x01a))
#define USB_TxInterval 			(*(volatile unsigned char *)(USB_BASE + 0x01b))
#define USB_NAKLIMIT0  			(*(volatile unsigned char *)(USB_BASE + 0x01b))
                       			                                 
#define USB_RxType     			(*(volatile unsigned char *)(USB_BASE + 0x01c))
#define USB_RxInterval 			(*(volatile unsigned char *)(USB_BASE + 0x01d))
                       			                                  
#define USB_ENDP0_FIFO 			(*(volatile unsigned char *)(USB_BASE + 0x020))         
#define USB_ENDP1_FIFO 			(*(volatile unsigned      *)(USB_BASE + 0x024))
#define USB_ENDP2_FIFO 			(*(volatile unsigned      *)(USB_BASE + 0x028))
#define USB_ENDP3_FIFO 			(*(volatile unsigned      *)(USB_BASE + 0x02c))
#define USB_DEVCTL     			(*(volatile unsigned short*)(USB_BASE + 0x060))
                            	
#define USB_TXFIFOSIZE      	(*(volatile unsigned char *)(USB_BASE + 0x062))
#define USB_RXFIFOSIZE      	(*(volatile unsigned char *)(USB_BASE + 0x063))
#define USB_TXFIFOADDR      	(*(volatile unsigned short *)(USB_BASE + 0x064))
#define USB_RXFIFOADDR      	(*(volatile unsigned short *)(USB_BASE + 0x066))
                       			                                 
#define USB_DMAINT     			(*(volatile unsigned      *)(USB_BASE + 0x200))
                       			                                  
#define USB_DMA1CTL    			(*(volatile unsigned      *)(USB_BASE + 0x204))
#define USB_DMA1ADDR   			(*(volatile unsigned      *)(USB_BASE + 0x208))
#define USB_DMA1CNT    			(*(volatile unsigned      *)(USB_BASE + 0x20c))
                       			                                  
#define USB_DMA2CTL    			(*(volatile unsigned      *)(USB_BASE + 0x214))
#define USB_DMA2ADDR   			(*(volatile unsigned      *)(USB_BASE + 0x218))
#define USB_DMA2CNT    			(*(volatile unsigned      *)(USB_BASE + 0x21c))
                       			                                  
#define USB_DMA3CTL    			(*(volatile unsigned      *)(USB_BASE + 0x234))
#define USB_DMA3ADDR   			(*(volatile unsigned      *)(USB_BASE + 0x238))
#define USB_DMA3CNT    			(*(volatile unsigned      *)(USB_BASE + 0x23c))
                       			                                  
#define USB_DMA4CTL    			(*(volatile unsigned      *)(USB_BASE + 0x244))
#define USB_DMA4ADDR   			(*(volatile unsigned      *)(USB_BASE + 0x248))
#define USB_DMA4CNT    			(*(volatile unsigned      *)(USB_BASE + 0x24c))



#endif


