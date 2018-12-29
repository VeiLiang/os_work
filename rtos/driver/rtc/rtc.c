/***********************************************************************
*Copyright (c)2012  Arkmicro Technologies Inc. 
*All Rights Reserved 
*
*Filename:    rtc.c
*Version :    1.0 
*Date    :    2009.06.18
*Author  :    Andy
*Abstract:    ark1620 RTC driver 
*History :     
* 
*Version :    2.0 
*Date    :    2012.02.27
*Author  :     
*Abstract:    ark1660  Ver1.0 MPW driver remove waring
*History :    1.0

************************************************************************/
#include <xm_proj_define.h>
#include <time.h>
 
#include "types.h"
#include "ark1960.h"
#include "rtc.h"
#include "uart.h"
#include "irqs.h"
#include "printk.h"
#include "xm_printf.h"

//RTC_CTL register fields defination
#define CTL_ALM_DATA_WEN						(1<<3)
#define CTL_PERIOD_INT_EN						(1<<2)
#define CTL_ALARM_INT_EN						(1<<1)
#define CTL_RESET								(1<<0)

//RTC_ANAWEN register fields defination
#define ANA_CNT_WEN							(1<<7)
#define ANA_RAM_WEN							(1<<6)
#define ANA_DELAY_TIMER_WEN					(1<<5)
#define ANA_CLR_PWR_DET_WEN					(1<<4)
#define ANA_DELAY_POWER_ON_WEN				(1<<3)
#define ANA_FORCE_POWER_OFF_WEN				(1<<2)
#define ANA_FORCE_POWER_ON_WEN				(1<<1)
#define ANA_RTC_WEN							(1<<0)

//RTC_ANACTL register fields defination
#define ANACTL_CLR_PWR						(1<<4)
#define ANACTL_DELAY_POWER_ON				(1<<3)
#define ANACTL_FORCE_POWER_OFF			(1<<2)
#define ANACTL_FORCE_POWER_ON				(1<<1)
#define ANACTL_COUNTER_EN					(1<<0)

#define STA_PWR_DET						(1<<6)
#define STA_DELAY_ON						(1<<5)
#define STA_FORCE_OFF						(1<<4)
#define STA_FORCE_ON						(1<<3)
#define STA_RCT_BUSY						(1<<2)
#define STA_PERIOD_INT						(1<<1)
#define STA_ALARM_INT						(1<<0)
//#define WAIT_FOR_SYNCHRONIZE_TO_ANA	while(rRTC_STA & STA_RCT_BUSY)
static unsigned int rtc_bad;
static void _WAIT_FOR_SYNCHRONIZE_TO_ANA(void)
{
	unsigned int i;
	if(rtc_bad)
		return;
	i = 0;
	while(rRTC_STA & STA_RCT_BUSY)
	{
		i ++;
		if(i >= 0x100000)
		{
			XM_printf ("\nRTC doesn't working\n");
			rtc_bad = 1;
			break;
		}
	}
}

#define WAIT_FOR_SYNCHRONIZE_TO_ANA _WAIT_FOR_SYNCHRONIZE_TO_ANA()

enum PEROID_VALUE{
        euPEROID_1_p_1024sec = (0<<2), // 1/1024s
        euPEROID_1_p_512sec  = (1<<2),  // 1/512s 
        euPEROID_1_p_256sec  = (2<<2),  // 1/256s
        euPEROID_1_p_128sec  = (3<<2),  // 1/128s
        euPEROID_1_p_64sec = (4<<2),  // 1/64s
        euPEROID_1_p_32sec  = (5<<2),  // 1/32s
        euPEROID_1_p_16sec  = (6<<2),  // 1/16s
        euPEROID_1_p_8sec = (7<<2),  // 1/8s
        euPEROID_1_p_4sec  = (8<<2),  // 1/4s
        euPEROID_1_p_2sec  = (9<<2), // 1/2s
        euPEROID_1sec = (10<<2),  // 1s
        euPEROID_2sec = (11<<2),  // 2s
        euPEROID_4sec = (12<<2),  // 4s
        euPEROID_8sec = (13<<2),  // 8s
        euPEROID_16sec = (14<<2),  // 16s
        euPEROID_32sec = (15<<2),  // 32s
        euPEROID_64sec = (16<<2),  // 64s
        euPEROID_128sec = (17<<2),  // 128s
        euPEROID_256sec = (18<<2),  // 256s
        euPEROID_512sec = (19<<2),  // 512s
        euPEROID_1024sec = (20<<2)  // 1024s
};

void SetAutoPowerUpTime(UINT32 power_off_delay_seconds)
{
	UINT32 cur_sec;
	UINT32 new_delay_timer;
	
	//get current second count;
	cur_sec = rRTC_CNTH;
	
	//calculate new delay power up timer
	new_delay_timer = cur_sec + power_off_delay_seconds;
	
	//write new_delay_timer to delay timer register;
	rRTC_ANAWEN = ANA_DELAY_TIMER_WEN;
	rRTC_DONT = new_delay_timer;
	WAIT_FOR_SYNCHRONIZE_TO_ANA;
	
	//enable delay on
	rRTC_ANAWEN = ANA_DELAY_POWER_ON_WEN;
	rRTC_ANACTL = ANACTL_DELAY_POWER_ON;
	WAIT_FOR_SYNCHRONIZE_TO_ANA; 
}

void PowerOff(void)
{
	//force_off = 0;
	rRTC_ANAWEN  = ANA_FORCE_POWER_OFF_WEN;
	rRTC_ANACTL &= ~(ANACTL_FORCE_POWER_OFF);
	
	//wait rtc_busy;
	WAIT_FOR_SYNCHRONIZE_TO_ANA;

	//force_off = 1;
	rRTC_ANAWEN = ANA_FORCE_POWER_OFF_WEN;
	rRTC_ANACTL = ANACTL_FORCE_POWER_OFF;
	//wait rtc_busy;
	WAIT_FOR_SYNCHRONIZE_TO_ANA;

}

void PowerOn(void)
{
//force_on = 0;

	rRTC_ANAWEN  = ANA_FORCE_POWER_ON_WEN;
	rRTC_ANACTL &= ~(ANACTL_FORCE_POWER_ON);
	//wait rtc_busy;
	WAIT_FOR_SYNCHRONIZE_TO_ANA;
	
//	force_on = 1;
	rRTC_ANAWEN = ANA_FORCE_POWER_ON_WEN;
	rRTC_ANACTL = ANACTL_FORCE_POWER_ON;
	//wait rtc_busy;
	WAIT_FOR_SYNCHRONIZE_TO_ANA;	
}

static void CleanPowerOffFlag(void)
{
//force_off = 0;

	rRTC_ANAWEN  = ANA_FORCE_POWER_OFF_WEN;
	rRTC_ANACTL &= ~(ANACTL_FORCE_POWER_OFF);
	//wait rtc_busy;
	WAIT_FOR_SYNCHRONIZE_TO_ANA;		
}

static void CleanPowerOnFlag(void)
{
	//force_off = 0;

	rRTC_ANAWEN  = ANA_FORCE_POWER_ON_WEN;
	rRTC_ANACTL &= ~(ANACTL_FORCE_POWER_ON);
	//wait rtc_busy;
	WAIT_FOR_SYNCHRONIZE_TO_ANA;	
}

static void CleanPowerDetectFlag(void)
{
	rRTC_ANAWEN = ANA_CLR_PWR_DET_WEN;
	rRTC_ANACTL &= ~(ANACTL_CLR_PWR);
	WAIT_FOR_SYNCHRONIZE_TO_ANA;	
}

static void CleanDlyOnFlag(void)
{
	rRTC_ANAWEN = ANA_DELAY_POWER_ON_WEN;
	rRTC_ANACTL &= ~(ANACTL_DELAY_POWER_ON);
	WAIT_FOR_SYNCHRONIZE_TO_ANA;	
}

void WriteToRam(UINT32 ulNewRam)
{
	rRTC_ANAWEN = ANA_RAM_WEN;
	rRTC_RAM = ulNewRam;
	WAIT_FOR_SYNCHRONIZE_TO_ANA;	
	if(rtc_bad)
		return;
	while(rRTC_RAM != ulNewRam); //after the busy signal is pull down, the analog module is begin to update, so, if you read the register now, it may
										//be not the value you want to set, at least after one 32K RTC clock elapse, it can be writen to analog.
										//Therefor, we have to wait for a moment at this position. Donier 2012-03-22	
}



void UpdateCounter(UINT32 ulNewCounter)
{
    rRTC_ANAWEN = ANA_RTC_WEN;	
	rRTC_ANACTL = ANACTL_COUNTER_EN;
	//wait rtc_busy;
	WAIT_FOR_SYNCHRONIZE_TO_ANA;	

    rRTC_ANAWEN = ANA_CNT_WEN;
	rRTC_CNTH = ulNewCounter;
	//wait rtc_busy;
	WAIT_FOR_SYNCHRONIZE_TO_ANA;		
	if(rtc_bad)
		return;
	while(rRTC_CNTH != ulNewCounter); //after the busy signal is pull down, the analog module is begin to update, so, if you read the register now, it may
										//be not the value you want to set, at least after one 32K RTC clock elapse, it can be writen to analog.
										//Therefor, we have to wait for a moment at this position. Donier 2012-03-22
}



void rtc_init(void)
{
	UINT32 val;

	//rRTC_CTL = (1<<CTL_RESET);
	//WAIT_FOR_SYNCHRONIZE_TO_ANA;

	//测试RTC休眠功能时，第一次必须运行UpdateCounter函数，然后掉电
	//这样RTC电池开始供电，第二次给1680上电运行时，必须注释掉
	//这个函数调用，否则休眠期间的RTC计数会重新被置位，第二次上
	//电后直接运行TestRtcTime，就可以看出秒数是否还在计数中

	val = rRTC_STA;
	//printk("rRTC_STA = 0x%x\n", val);
	if(val & STA_PWR_DET) //如果PWR_DET_FLG标志位被置1了，则表明RTC模拟模块被重新上电复位了，因此，我们需要重新设置rRTC_CNTH，以便RTC模拟模块起振，并开始计数
	{
		//清除RTC模拟部分电源复位检测位
		CleanPowerDetectFlag();
		
		//开启rRTC_CNTH
		UpdateCounter(0);
		
		WriteToRam(0x5555AAAA);
		//这里需要注意的是，这个ram寄存器只是用来保存数据用的，至于保存什么
		//数据，怎么使用这些bit位由各类方案根据实际应用场合来确定，这里只是为了测试
	}
	
	if(val & STA_DELAY_ON) //如果DLYON_ACT标志被置1了，则表明这次CPU CORE上电的原因是RTC_DLY_ON功能，即自动定时唤醒
	{
		CleanDlyOnFlag();
		CleanPowerOnFlag();
		CleanPowerOffFlag();
	}
}






static SYSTEM_TIME time_init = {
	0,
	0,
	0,
	0,
	1,
	1,
	1970
};



static void rtc_time_printf(void)
{     
	SYSTEM_TIME time_out;
	
	rtc_get_time(&time_out);
	
	printk("%d/%d/%d    %d:%d:%d   \n", time_out.year, time_out.month, time_out.date, time_out.hour,time_out.min,time_out.sec);       
}

static void period_int_handler(void)
{
	rRTC_STA &= ~STA_PERIOD_INT;
	printk("period_int_handler\n");
	rtc_time_printf();		
}

static void alarm_int_handler(void)
{
	rRTC_STA &= ~STA_ALARM_INT;
	
	printk("alarm_int_handler\n");
	rtc_time_printf();
}

INT32 rtc_set_time(SYSTEM_TIME *tm)
{
	UINT32 new_counter;
	INT32 i,j;
	UINT16  _month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
	UINT32 y_day,m_day,d_day;
	
	y_day=m_day=d_day=0;
	
	for(i=time_init.year;i<tm->year;i++)
	{
		if(i%4==0   &&   i%100!=0   ||   i%400==0)
			y_day+=366;
		else
			y_day+=365;
	}

	if(tm->year%4==0   &&   tm->year%100!=0   ||   tm->year%400==0)
	{
		_month[1]=29;
	}

	for(j=1;j<tm->month;j++)
	{
		m_day += _month[j-1];
	}

	d_day = tm->date - 1;

	new_counter = (y_day + m_day + d_day) *24*3600 + tm->hour*3600 + tm->min*60 + tm->sec;
	UpdateCounter(new_counter);

	time_init.day = week_day(tm->year, tm->month, tm->date);

	return 0;
}

UINT16 GetDaysInMonth(UINT16 y, UINT16 m)
{
	UINT16  _month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
	
	if(m==2)
	{
		if(y%4==0   &&   y%100!=0   ||   y%400==0)
		{
			_month[1]=29;
		}

		return _month[1];
	}
	else
		return _month[m-1] ;
}



INT32 rtc_get_time(SYSTEM_TIME *tm)
{
	UINT32 sec_con_sec,sec_con_min,sec_con_hour,sec_con_date;
	UINT32 cs,cmin,ch;
	UINT32 datesum,countDays;

	UINT32 sec_con = rRTC_CNTH;
	UINT32 ms_con = rRTC_CNTL;
	
	//printf ("sec=%d\n", sec_con);

	tm->ms = (ms_con * 1000) / (1 << 15);
	
	sec_con_sec   = sec_con % 60;
	sec_con_min   = sec_con /60 %60;
	sec_con_hour  = sec_con /3600 %24;
	sec_con_date = sec_con / 86400;

	tm->sec = (time_init.sec + sec_con_sec) % 60;
	cs = (time_init.sec + sec_con_sec) /60;

	tm->min = (time_init.min + sec_con_min + cs) % 60;
	cmin = (time_init.min + sec_con_min+ cs) /60;

	tm->hour = (time_init.hour + sec_con_hour+cmin) % 24;
	ch = (time_init.hour + sec_con_hour+cmin) /24;

	datesum = time_init.date + sec_con_date + ch;
	countDays = GetDaysInMonth(time_init.year, time_init.month);

        tm->month = time_init.month;
	tm->year = time_init.year;

	while (countDays < datesum)
	{
		datesum -= countDays;
		 if(++(tm->month) > 12)
		{
			tm->month = 1;
			(tm->year)++;
		}

		countDays = GetDaysInMonth(tm->year,tm->month);
	}
	tm->date = datesum;
	tm->day = week_day(tm->year, tm->month, tm->date);
	
	return 0;
}



UINT16 week_day(UINT16  y,UINT16  m,UINT16  d)   
{
	UINT16   _month[12]={31,28,31,30,31,30,31,31, 30,31,30,31};
	UINT16 C=0,i,S;
	
	if(y%4==0   &&   y%100!=0   ||   y%400==0)
	{
		_month[1]=29;
	}
	for(i=0; i<m-1; ++i)
		C+=_month[i];

	C+=d;
	S=y-1+(y-1)/4-(y-1)/100+(y-1)/400+C;

	return   S%7;
}

void alarm_on(void)
{
        rRTC_IM |= 1;
        rRTC_CTL |= CTL_ALARM_INT_EN;
}

void alarm_off(void)
{
        rRTC_CTL &= ~CTL_ALARM_INT_EN;
}

INT32 alarm_set(UINT32 data)
{
	rtc_time_printf();
//	WAIT_FOR_SYNCHRONIZE_TO_ANA;
	rRTC_CTL |= CTL_ALM_DATA_WEN;
	rRTC_ALMDAT=rRTC_CNTH+data;
	
	printk("Set alarm timer : %d s \n",data);
	
	return 0;
}

void period_set(UINT32  peroidNum)
{
	rRTC_IM = peroidNum | (1<<1);
}

void period_on(void)
{
	rRTC_CTL |= CTL_PERIOD_INT_EN;
}

void period_off(void)
{
	rRTC_CTL &= ~CTL_PERIOD_INT_EN;
}

/*parameter unit is second*/
void alarm_data_config(UINT32 data)
{
	/*set alarm time interval*/
	rRTC_CTL |= CTL_ALM_DATA_WEN;
	rRTC_ALMDAT = data;
}

void rtc_reset (void)
{
	rRTC_CTL = CTL_RESET;
	WAIT_FOR_SYNCHRONIZE_TO_ANA;
}

//test period interrupt
/*
*通过设置一个周期中断时间N，查看是否隔时间N会来一个周期中断
*/
void rtc_test_period_int(void)
{
	alarm_off();
	rtc_init();
	period_set (0x28);   // 1s的周期中断
	request_irq(RTC_PERIOD_INT, PRIORITY_FIFTEEN, period_int_handler);
	period_on();
}

#include "xm_core.h"
#include <xm_user.h>
#include <xm_base.h>
#include <xm_dev.h>
#include "rtos.h"
#include "calender.h"


#define	RTC_INIT_FLAG		0x45435452		// "RTCE"

extern BYTE _core_bTimeSetting;			// 系统时间未设置

static volatile long rtc_julian;							// 儒略历计数(自1970年1月1日)
static volatile unsigned char rtc_hour;				// 小时计数
static volatile unsigned char rtc_minute;				// 分钟计数
static volatile unsigned char rtc_second;				// 秒计数 (0 ~ 59)
static volatile unsigned short rtc_millisecond;		// 毫秒计数 (0 ~ 999)


XMBOOL	XM_GetLocalTime	(XMSYSTEMTIME* pSystemTime)
{
	XMBOOL ret = 0;

	if(rtc_bad)
	{
		// 软件模拟RTC
		unsigned char day;
		unsigned char month;
		int year;
		int julian;
	
		// 关中断, 保持数据一致性
		XM_lock ();
		julian = rtc_julian;
		pSystemTime->bHour = rtc_hour;
		pSystemTime->bMinute = rtc_minute;
		pSystemTime->bSecond = rtc_second;
		pSystemTime->wMilliSecond = rtc_millisecond;	
		ret = _core_bTimeSetting;	
		XM_unlock ();
		
		// 软件RTC
		// 将儒略历时间转换为年月日
		julian_to_mdy (julian, &day, &month, &year);
		pSystemTime->wYear = (WORD)(year + 1970);
		pSystemTime->bMonth = month;
		pSystemTime->bDay = day;
		pSystemTime->bDayOfWeek = julian_to_wday (julian);		
	}
	else
	{
		// 硬件RTC
	
		SYSTEM_TIME time;
		// 获取RTC的计数值
		rtc_get_time (&time);
		pSystemTime->wYear = time.year;
		pSystemTime->bMonth = time.month;
		pSystemTime->bDay = time.date;
		pSystemTime->bHour = time.hour;
		pSystemTime->bMinute = time.min;
		pSystemTime->bSecond = time.sec;
		pSystemTime->bDayOfWeek = time.day;
		pSystemTime->wMilliSecond = time.ms;
		
		ret = _core_bTimeSetting;	
	}	
	return ret;
}

VOID XM_SetLocalTime(const XMSYSTEMTIME *pSystemTime)
{
	if(rtc_bad)
	{
		// 软件模拟RTC
		long julian;
		
		julian = mdy_to_julian (pSystemTime->bMonth, pSystemTime->bDay, pSystemTime->wYear - 1970);
		
		// 关中断, 保持数据一致性
		XM_lock ();
		rtc_julian = julian;
		rtc_hour = pSystemTime->bHour;
		rtc_minute = pSystemTime->bMinute;
		rtc_second = pSystemTime->bSecond;
		rtc_millisecond = pSystemTime->wMilliSecond;
		_core_bTimeSetting = 1;
		XM_unlock ();
	}
	else
	{
	
		SYSTEM_TIME time;
		time.year = pSystemTime->wYear;
		time.month = pSystemTime->bMonth;
		time.date = pSystemTime->bDay;
		time.hour = pSystemTime->bHour;
		time.min = pSystemTime->bMinute;
		time.sec = pSystemTime->bSecond;
		time.day = pSystemTime->bDayOfWeek;
		rtc_set_time (&time);
			
		// 设置RTC初始化标志
		WriteToRam (RTC_INIT_FLAG);
		_core_bTimeSetting = 1;
	}
	
}

// RTC初始化
VOID XM_InitRTC (VOID)
{
	unsigned int rtc_flag;
	SYSTEM_TIME TSysTime;
	
	// 初始软件RTC时间为2017、1、1
	rtc_julian = mdy_to_julian (1, 1, 2017 - 1970);
	rtc_hour = 0;
	rtc_minute = 0;
	rtc_second = 0;
	rtc_millisecond = 0;
	
	
	rtc_init ();
	if(rtc_bad)
	{
		// 软件模拟RTC
use_software_rtc:
		
		// 初始时间为2017、1、1
		//rtc_julian = mdy_to_julian (1, 1, 2017 - 1970);
		//rtc_hour = 0;
		//rtc_minute = 0;
		//rtc_second = 0;
		//rtc_millisecond = 0;
		
		_core_bTimeSetting = 1;		// 初始时间自动设置,外部提示RTC未工作
		printf ("SW RTC run\n");
	}
	else
	{
		// 读取"RTC Ram Register", 判断RTC是否已初始化
		rtc_flag = rRTC_RAM;
		if(rtc_flag != RTC_INIT_FLAG)
		{
			//printf ("RTC don't run\n");
			alarm_off();
			rtc_reset();
			// 2017/1/1 00:00:00
			TSysTime.year = 2017;
			TSysTime.month = 1;
			TSysTime.date = 1;
			TSysTime.hour = 0;
			TSysTime.min = 0;
			TSysTime.sec = 0;
			TSysTime.day = 1;
			rtc_set_time (&TSysTime);
			_core_bTimeSetting = 0;
			//printf ("RTC run\n");
			if(rtc_bad)
				goto use_software_rtc;
		}
		else
		{
			_core_bTimeSetting = 1;
			//printf ("RTC already run\n");
		}
		printf ("HW RTC run\n");
	}
	
}

void XM_TriggerRTC (void)
{
	if(rtc_bad == 0)
		return;
	
	// RTC未安装或已坏, 使用软件模拟RTC
	rtc_millisecond ++;
	if(rtc_millisecond >= 1000)
	{
		rtc_millisecond = 0;
		rtc_second ++;
		if(rtc_second >= 60)
		{
			rtc_second = 0;
			rtc_minute ++;
			if(rtc_minute >= 60)
			{
				rtc_minute = 0;
				rtc_hour ++;
				if(rtc_hour >= 24)
				{
					rtc_hour = 0;
					rtc_julian ++;
				}
			}
		}
	}
}

