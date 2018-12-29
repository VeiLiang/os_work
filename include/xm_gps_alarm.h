#ifndef _XM_GPS_ALARM_H_
#define _XM_GPS_ALARM_H_

#if defined (__cplusplus)
	extern "C"{
#endif
		

typedef struct {
	unsigned short m_GpsAlarmType;
	unsigned short m_GpsLimitSpeed;
	unsigned short m_GpsDistance;
	unsigned short m_GpsCurSpeed;
} GPS_ALARM;
		

// ��ȡ���ӹ��汾��Ϣ
// ����ֵ
// 0	--> OK
// -1 --> NG
int GPS_GetVersion (unsigned int *code_ver, unsigned int *data_ver);

// ��ȡ���ӹ�������Ϣ
// ����ֵ
// 0	--> OK
// -1 --> NG
int GPS_GetAlarm (GPS_ALARM *alarm);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_GPS_ALARM_H_
