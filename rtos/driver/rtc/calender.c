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

// Word ���ʽ���

// Julian Calendar ������,�������ǹ�����ǰ�� 
// Gregorian Calendar ��ʾ����
//							����ʶ�����ּ�Ԫ��B.C.(��Ԫǰ)�� A.D.(��Ԫ��) �� C.E.(������Ԫ).
//							�����е����궨��Ϊ�����ɱ� 100 ����������⣬���Ա� 4 ��������������ꣻ
//							�ڿɱ� 100 ����������У����Ա� 400 ��������������ꡣ
//							���磬1900 �겻�����꣬�� 2000 �������ꡣƽ���� 365 �죬������ 366 �졣
//							������ 12 ���£�ÿ���� 28 �� 31 �첻�ȣ�
//							1 ��(31 ��)��2 ��(28 �� 29 ��)��3 ��(31 ��)��4 ��(30 ��)��5 ��(31 ��)��6 ��(30 ��)��
//							7 ��(31 ��)��8 ��(31 ��)��9 ��(30 ��)��10 ��(31 ��)��11 ��(30 ��)�� 12 ��(31 ��)��
//							2 ����������Ϊ 29 �죬��ƽ����Ϊ 28 �졣
// Time Zone ʱ�� �����Ϊ24��ʱ��,һ����׼ʱ��,�������α�׼ƽʱ(Greenwich Mediation Time)

// DST: Daylight Savings Time �չ��Լʱ��, ����ʱ��
// UTC: Universal Time, Coordinated Э������ʱ���ֳ������׼ʱ�䣬Ϊ�������α�׼ʱ������� 
// GMT: Greenwich Mean Time �������α�׼ʱ�� 

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

/* ������(1970.1.1)ת��Ϊ������ */
void time_to_mdy (time_t days, unsigned char *day, unsigned char *month, int *year)
{
	days += 	JULIAN_1970;
	julian_to_mdy (days, day, month, year);
}