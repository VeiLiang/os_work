//****************************************************************************
//
//	Copyright (C) 2004-2005 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: calender.c
//	  This is julian calender implementation toolbox
//
//	Revision history
//
//		2005.2.13	ZhuoYongHong add julian calender toolbox
//
//****************************************************************************
#include <stdio.h>
#include <time.h>

// Word 名词解释

// Julian Calendar 儒略历,儒略历是公历的前身 
// Gregorian Calendar 表示公历
//							公历识别两种纪元：B.C.(公元前)和 A.D.(公元后) 或 C.E.(基督纪元).
//							公历中的闰年定义为：除可被 100 整除的年份外，可以被 4 整除的年份是闰年；
//							在可被 100 整除的年份中，可以被 400 整除的年份是闰年。
//							例如，1900 年不是闰年，但 2000 年是闰年。平年有 365 天，闰年有 366 天。
//							公历有 12 个月，每个月 28 到 31 天不等：
//							1 月(31 天)、2 月(28 或 29 天)、3 月(31 天)、4 月(30 天)、5 月(31 天)、6 月(30 天)、
//							7 月(31 天)、8 月(31 天)、9 月(30 天)、10 月(31 天)、11 月(30 天)和 12 月(31 天)。
//							2 月在闰年中为 29 天，在平年中为 28 天。
// Time Zone 时区 地球分为24个时区,一个基准时区,格林威治标准平时(Greenwich Mediation Time)

// DST: Daylight Savings Time 日光节约时间, 即夏时制
// UTC: Universal Time, Coordinated 协调世界时，又称世界标准时间，为格林威治标准时间的新名 
// GMT: Greenwich Mean Time 格林威治标准时间 

#define JULIAN_1970					2440587

long mdy_to_julian (unsigned char month, unsigned char day, int year)
{
	int a, b = 0;
	int work_month = month, work_day = day, work_year = year;
	long julian;

	// correct for negative year
	if (work_year < 0)
		work_year++;

	if (work_month <= 2)
	{
		work_year--;
		work_month +=12;
	}

	// deal with Gregorian calendar
	if (work_year*10000. + work_month*100. + work_day >= 15821015.)
	{
		a = (int)(work_year/100.);
		b = 2 - a + a/4;
	}

	julian = (long) (365.25*work_year) +
			 (long) (30.6001 * (work_month+1))  +  work_day + 1720994L + b;

	return julian;
}

void julian_to_mdy (long julian, unsigned char *day, unsigned char *month, int *year)
{
	long a,b,c,d,e,z,alpha;
	z = julian+1;

	// dealing with Gregorian calendar reform

	if (z < 2299161L)
		a = z;
	else
	{
		alpha = (long) ((z-1867216.25) / 36524.25);
		a = z + 1 + alpha - alpha/4;
	}

	b = ( a > 1721423 ? a + 1524 : a + 1158 );
	c = (long) ((b - 122.1) / 365.25);
	d = (long) (365.25 * c);
	e = (long) ((b - d) / 30.6001);

	*day = (unsigned char)(b - d - (long)(30.6001 * e));
	*month = (unsigned char)((e < 13.5) ? e - 1 : e - 13);
	*year = (int)((*month > 2.5 ) ? (c - 4716) : c - 4715);
}


unsigned char julian_to_wday (long julian)
{
	unsigned char day_of_week;
	day_of_week = (unsigned char) ((julian + 2) % 7 + 1);

	return day_of_week;
}

int is_leap_year( int year )
{
	return  ( (year >= 1582) ?
		  (year % 4 == 0  &&  year % 100 != 0  ||  year % 400 == 0 ):
		  (year % 4 == 0) );
}

/* 将天数(1970.1.1)转换为年月日 */
void time_to_mdy (time_t days, unsigned char *day, unsigned char *month, int *year)
{
	days += 	JULIAN_1970;
	julian_to_mdy (days, day, month, year);
}