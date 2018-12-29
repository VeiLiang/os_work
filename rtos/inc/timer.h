/*
**********************************************************************
Copyright (c)2007 Arkmicro Technologies Inc.  All Rights Reserved 
Filename: timer.h
Version : 1.1 
Date    : 2008.01.08
Author  : Salem
Abstract: ark1610 soc timer driver
History :
***********************************************************************
*/

#ifndef _TIMER_H_
#define _TIMER_H_

#include "types.h"
#include "irqs.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    UINT32 wHour; 
    UINT32 wMinute; 
    UINT32 wSecond; 
    UINT32 wMilliseconds; 
}tSystemTime;

typedef struct
{
    UINT32 wYear;
    UINT32 wMonth;
    UINT32 wDay;
    UINT32 wDayOfWeek;
}tSystemDate;


typedef enum 
{
 	euPeriodTrig,
	euOneShotTrig
}EU_INT_TRIG_MODE;

#ifdef SYS_IAR_RTOS
#include "rtos.h"
#endif
 typedef struct stHardwareTimer
{
	UINT32 opt;
	UINT32 ulIntervalTick;
	UINT32 ulCurTick;
	UINT32 ulStatus;
	void *	pdata;
	void (*pfnHandler)(void *, uint16_t );
	
#ifdef SYS_IAR_RTOS
	OS_TIMER_EX	TimerEx;
#endif	
}HardwareTimer;
 

HardwareTimer* NewHardwareTimer(UINT32 ulIntervalTick, void *pdata, void (*pfnTimeOut)(void *, uint16_t));

void AddHardwareTimer(HardwareTimer *pTimer);

void StartHardwareTimer(HardwareTimer *pTimer);

void StopHardwareTimer(HardwareTimer *pTimer);

void DestroyHardwareTimer(HardwareTimer *pTimer);

void HardwareTimerTigger(void);


void TimerInit(void);

// ����1KHz��ϵͳ��ʱ��
void StarSysTimer(void);

enum {
	XM_TIMER_0 = 0,
	XM_TIMER_1,
	XM_TIMER_2,
	XM_TIMER_3,
	XM_TIMER_4,
	XM_TIMER_5,
	XM_TIMER_COUNT
};

int timer_init (void);

void busy_delay(INT32U time);

void delay(unsigned int time);


// timer		��ʱ�����
// ticks_per_second		ÿ��δ���
// user_tick_callback	�û��δ�ص�����
// user_private_data    �û�˽������
int timer_x_start (unsigned int timer, UINT ticks_per_second, pfnIRQUSER user_tick_callback, void* user_private_data);

int timer_x_stop (unsigned int timer);

// ΢�뼶��ʱ
void udelay (unsigned long usec);

// ��ȡ����������߾��ȵĵδ��������΢��Ϊ������λ
XMINT64 		XM_GetHighResolutionTickCount (void);

#ifdef __cplusplus
}
#endif

#endif

