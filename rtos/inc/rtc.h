/*
**********************************************************************
Copyright (c)2009 Arkmicro Technologies Inc.  All Rights Reserved
Filename: rtc.c
Version	: 1.0
Date    : 2009.06.18
Author  : Andy
Abstract: ark1620 RTC driver
History :
***********************************************************************
*/
//#include "types.h"
#ifndef _RTC_H_
#define _RTC_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
	UINT16 day;
	UINT16 sec;
	UINT16 min;
	UINT16 hour;
	UINT16 date;
	UINT16 month;
	UINT16 year;
	UINT16 ms;
}SYSTEM_TIME;

void rtc_init(void);
INT32 rtc_get_time(SYSTEM_TIME *tm);
INT32 rtc_set_time(SYSTEM_TIME *tm);
UINT16 day_month(UINT16 y, UINT16 m);
UINT16 week_day(UINT16 y, UINT16 m, UINT16 d);
void alarm_int_handler(void);
void alarm_on(void);
void alarm_off(void);
INT32 alarm_set(UINT32 data);

// software RTC
extern void XM_TriggerRTC (void);

#ifdef __cplusplus
}
#endif

#endif /* _RTC_H_ */



