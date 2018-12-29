/*
*********************************************************************************************************
*                                               uC/OS-II
*                                        The Real-Time Kernel
*
*                         (c) Copyright 1992-2002, Jean J. Labrosse, Weston, FL
*                                          All Rights Reserved
*
*                                           MASTER INCLUDE FILE
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               ARM Port
*
*                 Target           : ARM (Includes ARM7, ARM9)
*                 Ported by        : Michael Anburaj
*                 URL              : http://geocities.com/michaelanburaj/    Email : michaelanburaj@hotmail.com
*
*********************************************************************************************************
*/

#ifndef __INCLUDES_H__
#define __INCLUDES_H__

#include "xm_proj_define.h"
#include "rtos.h"
#include "irqs.h"
#include "uart.h"

typedef unsigned int   OS_STK;                   /* Each stack entry is 32-bit wide                    */

#define	OS_NO_ERR	0

void *OSSemCreate (INT16U cnt);
void  *OSSemDel (void *pevent, INT8U opt, INT8U *err);
INT8U  OSSemPost (void *pevent);

// 20131212 ZhuoYongHong
// OSSemPend 修改为32位超时单位。UCOS II为16位。
// MGC_UcFsDeviceWriteBurst 会出现超出16位的timeout
void  OSSemPend (void *pevent, INT32U timeout, INT8U *err);


void  OSTimeDly (INT16U ticks);
INT32U  OSTimeGet (void);

/*
*********************************************************************************************************
*                            TIMER OPTIONS (see OSTmrStart() and OSTmrStop())
*********************************************************************************************************
*/
#define  OS_TMR_OPT_NONE              0u    /* No option selected                                      */

#define  OS_TMR_OPT_ONE_SHOT          1u    /* Timer will not automatically restart when it expires    */
#define  OS_TMR_OPT_PERIODIC          2u    /* Timer will     automatically restart when it expires    */

#define  OS_TMR_OPT_CALLBACK          3u    /* OSTmrStop() option to call 'callback' w/ timer arg.     */
#define  OS_TMR_OPT_CALLBACK_ARG      4u    /* OSTmrStop() option to call 'callback' w/ new   arg.     */

/*
*********************************************************************************************************
*                                            TIMER STATES
*********************************************************************************************************
*/
#define  OS_TMR_STATE_UNUSED          0u
#define  OS_TMR_STATE_STOPPED         1u
#define  OS_TMR_STATE_COMPLETED       2u
#define  OS_TMR_STATE_RUNNING         3u


void OSIntEnter (void);
void OSIntExit (void);


#endif /*__INCLUDES_H__*/

