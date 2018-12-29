//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: app_gps_alarm.c
//	  ���ӹ�
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//		2017.05.23  �����ڴ�ΰҵԤ����֧��
//
//****************************************************************************
#include "app.h"
#include "app_menuid.h"
#include "xm_gps_alarm.h"

// 1.������ֵ�������ϰ����ʾ��ͨ��ʾ��ͼ�꣬�°����ʾ����ֵ��ֻ��4�����ͣ��ֱ�Ϊ�����(2)���̶�����(3)����������(4)���������(18);
// 2.û������ֵ�ģ�Ҳ��������ֵΪ0�ģ���ʾ18��������ͨ��ʾ��ͼ�ꣻ
// 3.�ر�ע�⣬����˵����4������㣬�п���û������ֵ�����Ե�2,3,4,18������ֵΪ0��ʱ��Ҫ��ʾ������ͨ��ʾ��
// 4.ͨ�������۵�ʱ�򣬼ǵ������ЩICON��
// 5.�����ڵ������ϵ������룬ʵʱ���ٵȵȣ�
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
		// 1.������ֵ�������ϰ����ʾ��ͨ��ʾ��ͼ�꣬�°����ʾ����ֵ��ֻ��4�����ͣ��ֱ�Ϊ�����(2)���̶�����(3)����������(4)���������(18);
	}
	else
	{
		// 2.û������ֵ�ģ�Ҳ��������ֵΪ0�ģ���ʾ18��������ͨ��ʾ��ͼ�ꣻ
		// 3.�ر�ע�⣬����˵����4������㣬�п���û������ֵ�����Ե�2,3,4,18������ֵΪ0��ʱ��Ҫ��ʾ������ͨ��ʾ��
	}
	
}
