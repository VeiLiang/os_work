/******************************************************************
 *                                                                *
 *        Copyright Mentor Graphics Corporation 2004              *
 *                                                                *
 *                All Rights Reserved.                            *
 *                                                                *
 *    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION *
 *  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS   *
 *  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                    *
 *                                                                *
 ******************************************************************/

#ifndef __MUSB_UCOS_PLATFORM_CONFIG_H__
#define __MUSB_UCOS_PLATFORM_CONFIG_H__

/*
 * uC/OS-II platform-specific configuration definitions
 * $Revision: 1.8 $
 */

#include <stdlib.h>
#include "ark1960.h"
#include "timer.h"
#include "rtos.h"
/* include board-specific configuration */
#include "brd_cnf.h"
#include "printk.h"
#include "xm_core.h"

//#define	MUSB_DIAG	2
#define	MUSB_DEBUG

#ifdef	MUSB_DEBUG
#define	MUSB_PRINTK	printk
#else
#define	MUSB_PRINTK	
#endif

#define MUSB_MHDRC							//Nicholas Xu
#define MUSB_HDRC_DMA						//Nicholas Xu
#define MUSB_HUB							//Nicholas Xu
#define MUSB_ISO								//Nicholas Xu
#define MUSB_IPOD							//Nicholas Xu

// 20131220 ZhuoYongHong
//#define	DYNAMIC_FIFO

#ifdef DYNAMIC_FIFO
// 使能动态FIFO定义
// enable DMA channel, the number of DMA channels is 2
//#define MUSB_DMA
//#define MUSB_C_DMA2

// dynamic FIFO Sizing
#define MUSB_C_DYNFIFO_DEF
#define MUSB_C_NUM_EPS			6
#define MUSB_C_RAM_BITS			10

#else

// 禁止动态FIFO定义
#define MUSB_DMA
#define MUSB_C_DMA4

#endif

//#define MUSB_FORCE_FULLSPEED				//Nicholas Xu

#define MAX_SUPPORT_DEVICE		6			//Nicholas Xu	

/* Binary semaphore count value */
#define MGC_UCOS_BINARY_SEM_COUNT	1

/* First 4 and last 4 task priority are reserved for future use,
 * Highest priority is 0 and lowest priority is 63 */
#define MGC_UCOS_HIGHEST_PRIORITY       5

#define MGC_UCOS_LOWEST_PRIORITY        10

#define MGC_UCOS_SUSPEND            	   0

/* Maximum number of interrupts, the system provids */
#define MGC_UCOS_MAX_INTERRUPTS 	4

extern unsigned char lg_Usb_Present;

#endif	/* multiple inclusion protection */
