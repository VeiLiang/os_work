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

#ifndef __MUSB_NONE_BOARD_CONFIG_H__
#define __MUSB_NONE_BOARD_CONFIG_H__

#define MUSB_VBUS_ERROR_RETRIES 4

#define	MUSB_TS_SESSREQ	    8	/* SRP, WAIT_A_BCON, WAIT_B_ACON */
#define	MUSB_MAX_TRANSITION_TM   250	/* 250 msec for transition coalesce */
#define	MUSB_TB_SRP_FAIL	5

/** Required task stack size for this target */
#define MUSB_UCOS_TASK_STACK_SIZE        4*1024 

/** Tick period on this target */
#define MUSB_UCOS_TICK_PERIOD       10

/** PCI vendor ID for this target's controller(s) */
#define MUSB_PCI_VENDOR_ID	    0x14AB

/** PCI device ID for this target's controller(s) */
#define MUSB_PCI_DEVICE_ID		0x0010 

/* AFS doesn't provide OS delete functions, so define fake ones */
//#define OSSemDel(a, b, c)
//#define OSQDel(a, b, c)
//#define OSMboxDel(a, b, c)

#define OS_DEL_ALWAYS	0xff

#endif	/* multiple inclusion protection */
