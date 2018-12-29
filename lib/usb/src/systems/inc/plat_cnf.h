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
 * $Revision: 1.2 $
 */

#include <stdlib.h>
//#include "ark1660.h"
#include "ark1960.h"
#include "timer.h"
/* include board-specific configuration */
#include "brd_cnf.h"
#include "printk.h"
#include "os.h"


//#include <stdarg.h>
void print_null (char *fmt, ...);

//#define	MUSB_DEBUG
#ifdef	MUSB_DEBUG
#define	MUSB_PRINTK	printf	//printk
#else
#define	MUSB_PRINTK print_null
#endif

#define MUSB_MHDRC							//Nicholas Xu
#define MUSB_HDRC_DMA						//Nicholas Xu
#define MUSB_HUB							//Nicholas Xu
#define MUSB_ISO								//Nicholas Xu
//#define MUSB_IPOD							//Nicholas Xu

#define MUSB_DMA							//Nicholas Xu
#define MUSB_C_DMA8							//Nicholas Xu

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
