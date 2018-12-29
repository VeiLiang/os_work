#include "Uart_Protocal.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <xm_type.h>
#include <xm_base.h>
#include <xm_key.h>
#include <xm_user.h>
#include <xm_dev.h>
#include "rtos.h"
#include "xm_gps_alarm.h"

long					mdy_to_julian	(unsigned char month, unsigned char day, int year);
void					julian_to_mdy	(long julian, unsigned char *day, unsigned char *month, int *year);

static u8 GPS_DataBuf[GPS_DATA_SIZE];
static u8 Decode_DataBuf[GPS_DATA_SIZE]; //after verify

static FIFO_BUF_STRUCT GpsSave; //GPS Data
static GPS_TIME CurTime;
static u8 FirstGpsTime;
static u8 ElectronEyeFound;

// ICON/������ʾʹ��
static u8 GpsAlarmType;		// ��������
static u8 GpsLimitSpeed;	// ����ֵ
static u16 GpsDistance; 	// ��������
static u8 GpsCurSpeed;		// ��ǰ����

static OS_RSEMA	GpsSema;	


/*
u32 TotalSec_UTC_Time(void)
{
	time_t tmptime;
	struct tm today;	
	
	today.tm_year = CurTime.m_bRMCYear+100;//X+2000-1900,since from 1900	
	today.tm_mon = CurTime.m_btRMCMonth-1; // 0~11
	today.tm_mday = CurTime.m_btRMCDay;
	
	today.tm_hour = CurTime.m_btRMCUTCHour;
	today.tm_min = CurTime.m_btRMCMinute;
	today.tm_sec = CurTime.m_btRMCSecond;
	
	tmptime = mktime(&today);
	
	return ((u32)tmptime);
}

void GetLocalTime(struct tm * day)
{
	time_t tmptime;
	struct tm * tmp;
	
	tmptime = (time_t)TotalSec_UTC_Time();
	
	tmptime += 28800; // 8*60*60=28800
	
	tmp = localtime(&tmptime);
	
	(*day).tm_year = (*tmp).tm_year;	
	(*day).tm_mon = (*tmp).tm_mon;
	(*day).tm_mday = (*tmp).tm_mday;
	(*day).tm_hour = (*tmp).tm_hour;
	(*day).tm_min = (*tmp).tm_min;
	(*day).tm_sec = (*tmp).tm_sec;
	(*day).tm_wday = (*tmp).tm_wday;
	(*day).tm_yday = (*tmp).tm_yday;
	(*day).tm_isdst = (*tmp).tm_isdst;
	
}


//Receive data in interrupt
void uart0_isr(void)
{
    u8 temp = 0;
    if(UT0_CON & BIT(14)) //rpend
    {
		temp = UT0_BUF;
        	UT0_CON |=BIT(12);
		
		GPS_DataBuf[GpsSave.m_dwWrId++]=temp; 
		if(GpsSave.m_dwWrId >= GPS_DATA_SIZE)
		{
			GpsSave.m_dwWrId = 0;
		}
		
    }
}
*/

void GPS_ReceiveOneByte (unsigned char temp)
{
	GPS_DataBuf[GpsSave.m_dwWrId++]=temp; 
	if(GpsSave.m_dwWrId >= GPS_DATA_SIZE)
	{
		GpsSave.m_dwWrId = 0;
	}
	
}

//��һ�㣬ֱ�ӽ���һ������
//len Ϊ�������ݰ��ĳ���
u8 CalCRC(u8 *ptr, u16 len) 
{ 
	u8 crc; 
	u16 i; 
	u16 calLen;
	
	//�������� 5���ֽ�
	if(len < 5)
	{
		return FALSE;
	}
	
	crc = 0; 
	calLen = len-2;
	for(i=1;i<calLen;i++)
	{
		crc += *(ptr+i);
	}
	crc ^= 0x8A;
	
	if(crc == *(ptr+len -2))
	{
		return TRUE;
	}
	else
	{
		return FALSE; 
	}
} 

u8 DataIn_Decode(u8 *pBuff, u16 nMaxCount,u16 *pLen)
{
	u8 ret;
	u16 tryLen;
	
	if((*pBuff) == '|')
	{
		tryLen = ((u16)*(pBuff+1))&0xFF;
		tryLen |= (((u16)*(pBuff+2))<<8)&0xFF00;
		
		tryLen = tryLen+2;

		
		if(tryLen <= nMaxCount)
		{
			if(*(pBuff+tryLen-1) == '~')
			{
				
				ret = CalCRC(pBuff, tryLen);
				if(ret)
				{
					*pLen = tryLen;
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}
void GPS_Command_Analysis(void)
{
	u16 i;
	u8 alarmType;
	u8 limitSpeed;
	u16 distance;
	u8 curSpeed;
	u8 curAngle;
	u32 curLattitude;
	u32 curLongitude;
	u32 curjulian;
	int year;
	unsigned char month, day, hour, minute, second;
	u32 play_alarm, play_speed;
	
	play_alarm = 0;	// ����������
	play_speed = 0;	// ��������
	i = DATA_OFFSET;
	if(Decode_DataBuf[i]=='A') //GPS��λ����������ʹ��
	{
		//��һ�μ�⵽Aʱ������ֱ������DVR��ʱ��
		i++;
		if(Decode_DataBuf[i]>0)
		{
			// ���ֵ����ۣ�ֱ����Ҫ��ʾICON��
			// ����ǵ�һ����0-->X, ������Ҫ������ǰ��XXX����XXX
			if(ElectronEyeFound == 0)
			{
				ElectronEyeFound = 1;
				play_alarm = 1;
			}
			alarmType = Decode_DataBuf[i];
			i++;
			limitSpeed = Decode_DataBuf[i];
			i++;
			
			
			//����ֵ��30��120����֮�䣬��Ҫ��������XX����
			if((limitSpeed>=30)&&(limitSpeed<=120))
			{
				//Play audio
				//��������ID=2(�����),3(�̶�����),4(��������),18(�������)
				if(alarmType == 2 || alarmType == 3 || alarmType == 4 || alarmType == 18)
				{
					play_speed = 1;
					//printf ("GPS-->  ����ֵ = %d\n", limitSpeed);
				}
			}
			else
			{
				//��ȫ�㣬û������ֵ
				//��������ID��1~18,�������д����(ID=2),Ҳ��������ֵΪ0��ʱ��
				//����ֵΪ0���벻Ҫ��������XX����
			}

			//�������룬һ�㶼�ǵݼ��ģ��벻Ҫ���ݵ�������Ϊ0���ж�
			//�Ƿ�ͨ�������ۣ���Ҫ����alarmTypeΪ0���ж�
			memcpy((u8*)&distance,&Decode_DataBuf[i],2);
			i += 2;
			//��ǰ���٣����ᳬ��220
			curSpeed = Decode_DataBuf[i];
			i++;

			//��ǰ���ԽǶȣ�����˳ʱ�뷽������Ϊ1������Ϊ2...
			curAngle = Decode_DataBuf[i];
			i++;

			//��ǰγ�ȣ��Ŵ���10000��������2212345��ʵ������22.12345��
			memcpy((u8*)&curLattitude,&Decode_DataBuf[i],3);
			i += 3;

			//��ǰ���ȣ��Ŵ���10000��������11934567��ʵ������119.34567��
			memcpy((u8*)&curLongitude,&Decode_DataBuf[i],3);
			i += 3;

			//����Ĳ���˵��
			//��һ���ر�ע�⣬�����ʱ���Ǳ�׼��UTCʱ��
			//��Ҫת���ɱ���ʱ�䣬���Ǿ��Բ�����ʹ��UTC+8��
			//��Сʱ���м򵥵ļ�8������Ҫʹ��time.h�����
			//����������86400(8*60*60)���ٻ����Ϊ��׼��ʱ��
			//���磬��ǰUTCʱ����2015.12.31 23:58:59����ȷ��
			//����ʱ��Ӧ����2016.01.01 07:58:59
			// ���������Ϊ0xFFΪ��Ч�Ĳ���������Ч����Ķ���Ϊ���ݼ���2000
			year = Decode_DataBuf[i+0];
			month = Decode_DataBuf[i+1];
			day = Decode_DataBuf[i+2];
			hour = Decode_DataBuf[i+3];
			minute = Decode_DataBuf[i+4];
			second = Decode_DataBuf[i+5];
			if(FirstGpsTime && year != 0xFF && month != 0xFF && day != 0xFF)
			{
				long julian;
				XMSYSTEMTIME localTime;
				
				// ��һ����Чʱ��
				FirstGpsTime = 0;
				
				year = year + 2000;
				if( (hour + 8) >= 24 )
				{
					julian = mdy_to_julian (month, day, year);
					hour = hour + 8 - 24;	// // UTC --> BeiJing
					julian ++;
					julian_to_mdy (julian, &day, &month, &year);
				}
				else
				{
					hour = hour + 8;		// UTC --> BeiJing
				}
				
				localTime.wYear = year;
				localTime.bDay = day;
				localTime.bMonth = month;
				localTime.bHour = hour;
				localTime.bMinute = minute;
				localTime.bSecond = second;
				
				XM_SetLocalTime (&localTime);
				
			}
		}
		else
		{
			//�������X-->0,˵����������ʧ����Ҫ���ICON��ʾ
			ElectronEyeFound = 0;
		}
	}
	
	if(play_alarm)
	{
		OS_Use(&GpsSema); /* Make sure nobody else uses */
		GpsAlarmType = alarmType;
		GpsLimitSpeed = limitSpeed;
		GpsDistance = distance;
		GpsCurSpeed = curSpeed;
		OS_Unuse (&GpsSema);
		
		// ��������������
		if(play_speed)
		{
			// ������������
		}
		
		XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_RADAR_ALARM);
	}
	else
	{
		if(play_speed)
		{
			printf ("no alarm, only limit speed voice\n");
		}
	}
	
}

static u16 gps_data_ver;		// ���ݰ汾��
static u16 gps_code_ver;		// �̼��汾��
static void Version_Command_Analysis(void)
{
	// �汾���� (3 bytes)
	// 1~2 (���ݰ汾��) 3 (�̼��汾��)
	// ������Ϣ�ڿ�����������������ʮ�Σ�ÿ�μ��ʱ���ԼΪ1s�����Է����¼�ǲ鿴��ǰ�İ汾��Ϣ������ʮ��֮���ֹͣ���ͣ� 
	//	ע�⣬�̼��汾�ź����ݰ汾����Ҫ��ʾ����Ӧ�Ĳ˵����߽����У��Ա��ܲ��ġ�
	u16 i;
	i = DATA_OFFSET;
	OS_Use(&GpsSema); /* Make sure nobody else uses */
	gps_data_ver = Decode_DataBuf[i] | (Decode_DataBuf[i+1] << 8);
	gps_code_ver = Decode_DataBuf[i+2];
	OS_Unuse(&GpsSema); /* Make sure nobody else uses */
}

static u16 ErrorType;
static void Error_Command_Analysis(void)
{
	// �쳣��ʾ��Ϣ��Dog-----Car Record��
	// Error Type (1byte)
	// 0x01:��ȡ�����ļ������쳣�������ļ���ɾ��(Ԥ�������в��������д�У��)�� 
	// 0x02:��֤Ԥ��������ʧ�ܣ�������Ҫ��ʽ��U�̣����¸���ȫ���ļ���U����ȥ�� 
	// 0x04:Ԥ�����������ز����������������أ�
	// 0x05:��ȡFLASHʧ�ܻ���U���쳣��
	u16 i;
	i = DATA_OFFSET;
	ErrorType = Decode_DataBuf[i];
}


static void Update_Command_Analysis(void)
{
	// ��������������ʾ��Ϣ��Dog-----Car Record��
	// Link Status (1byte)
	// USBͨѶ��ͨ��mass storage�ķ�ʽʵ�ֵģ��û�������ԣ�����������ߣ��������ݵ�FLASH���渲�ǣ�
	// ��;���ɶϵ磬Ҳ���������������ļ���U������(������Ӱ���´�Ԥ��������������������)��
 	// 0x01��USB���ӳɹ�
}

void Decode_Uart_Data(void)
{
	switch(Decode_DataBuf[3])
	{
   	case 0x81:
		// GPS Ԥ����Ϣ
		GPS_Command_Analysis();
		break;

	case 0x82:
		//�汾������Ϣ
		Version_Command_Analysis();
		break;	

	case 0x83:
		//�쳣��ʾ��Ϣ
		Error_Command_Analysis();
		break;
		
	case 0x84:
		//��������������ʾ��Ϣ
		Update_Command_Analysis();
		break;
		
	default:
		break;
	}
}

void GPS_DecodeUart(void)
{
	u8 bRet;
	u16 curHandLen;
	u16 findLen;
	u16 i;
	 
	if(GpsSave.m_dwRdId < GpsSave.m_dwWrId)
	{
		i = 0;
		curHandLen = GpsSave.m_dwWrId - GpsSave.m_dwRdId; //current left length
		if(curHandLen > (GPS_DATA_SIZE>>1)) //delay long time or make mistake,discard
		{
			curHandLen = 0; //error
			GpsSave.m_dwRdId = 0;
			GpsSave.m_dwWrId = 0;
		}
		while(curHandLen)
		{
			bRet = DataIn_Decode(&GPS_DataBuf[i+GpsSave.m_dwRdId],curHandLen,&findLen);
			if(bRet==TRUE)
			{
				//copy data
				memcpy(Decode_DataBuf,&GPS_DataBuf[i+GpsSave.m_dwRdId],findLen);
				Decode_Uart_Data();
				i += findLen;  //find data,need decode
				GpsSave.m_dwRdId += findLen;
				curHandLen -= findLen;
			}
			else
			{
				i++;
				curHandLen--;
			}
		}
		//simple handle
		if(GpsSave.m_dwRdId == GpsSave.m_dwWrId)
		{
			GpsSave.m_dwRdId = 0;
			GpsSave.m_dwWrId = 0;
		}
	}
}

void GPS_Init (void)
{
	memset (&GpsSave, 0, sizeof(GpsSave));
	memset (&CurTime, 0, sizeof(CurTime));
	memset (GPS_DataBuf, 0, sizeof(GPS_DataBuf));
	memset (Decode_DataBuf, 0, sizeof(Decode_DataBuf));
	
	gps_data_ver = 0;
	gps_code_ver = 0;
	FirstGpsTime = 1;
	ElectronEyeFound = 0;
	
	OS_CREATERSEMA(&GpsSema); /* Creates resource semaphore */
}

// ��ȡ���ӹ��汾��Ϣ
int GPS_GetVersion (unsigned int *code_ver, unsigned int *data_ver)
{
	int ret = 0;
	OS_Use(&GpsSema); /* Make sure nobody else uses */
	*code_ver = gps_code_ver;
	*data_ver = gps_data_ver;
	if(gps_code_ver == 0 || gps_data_ver == 0)
		ret = -1;
	OS_Unuse(&GpsSema); 
	return ret;
}

// ��ȡ���ӹ�������Ϣ
// ����ֵ
// 0	--> OK
// -1 --> NG
int GPS_GetAlarm (GPS_ALARM *alarm)
{
	int ret;
	if(alarm == NULL)
		return -1;
	OS_Use(&GpsSema); /* Make sure nobody else uses */
	if(GpsAlarmType)
	{
		alarm->m_GpsAlarmType = GpsAlarmType;
		alarm->m_GpsLimitSpeed = GpsLimitSpeed;
		alarm->m_GpsDistance = GpsDistance;
		alarm->m_GpsCurSpeed = GpsCurSpeed;
		ret = 0;
	}
	else
	{
		ret = -1;
	}
	OS_Unuse(&GpsSema); 
	return ret;
}
