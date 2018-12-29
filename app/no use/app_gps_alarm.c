//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: app_gps_alarm.c
//	  电子狗
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//		2017.05.23  加入众创伟业预警仪支持
//
//****************************************************************************
#include "app.h"
#include "app_menuid.h"
#include "xm_gps_alarm.h"

// 1.有限速值，就在上半截显示交通标示的图标，下半截显示限速值，只有4种类型，分别为闯红灯(2)，固定测速(3)，流动测速(4)，区间测速(18);
// 2.没有限速值的，也就是限速值为0的，显示18种整个交通标示的图标；
// 3.特别注意，上面说到的4种照相点，有可能没有限速值，所以当2,3,4,18的限速值为0的时候，要显示整个交通标示。
// 4.通过电子眼的时候，记得清除这些ICON；
// 5.可以在底下配上倒数距离，实时车速等等；
void AP_ProcessGpsAlarm (void)
{
	GPS_ALARM alarm;
	unsigned int alarmType;
	unsigned int limitSpeed;
	int half_icon_display = 0;		// 
	
	if(GPS_GetAlarm (&alarm) < 0)
		return;
	
	alarmType = alarm.m_GpsAlarmType;
	if(alarmType == 0)
		return;
	
	limitSpeed = alarm.m_GpsLimitSpeed;
	if(alarmType == 2 || alarmType == 3 || alarmType == 4 || alarmType == 18)
	{
		if((limitSpeed >= 30) &&(limitSpeed <= 120))
		{
			half_icon_display = 1;
		}
	}
	
	if(half_icon_display)
	{
		// 1.有限速值，就在上半截显示交通标示的图标，下半截显示限速值，只有4种类型，分别为闯红灯(2)，固定测速(3)，流动测速(4)，区间测速(18);
	}
	else
	{
		// 2.没有限速值的，也就是限速值为0的，显示18种整个交通标示的图标；
		// 3.特别注意，上面说到的4种照相点，有可能没有限速值，所以当2,3,4,18的限速值为0的时候，要显示整个交通标示。
	}
	
}
