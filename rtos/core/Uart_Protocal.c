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

// ICON/语音显示使用
static u8 GpsAlarmType;		// 报警类型
static u8 GpsLimitSpeed;	// 限速值
static u16 GpsDistance; 	// 倒数距离
static u8 GpsCurSpeed;		// 当前车速

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

//简单一点，直接解析一包数据
//len 为整个数据包的长度
u8 CalCRC(u8 *ptr, u16 len) 
{ 
	u8 crc; 
	u16 i; 
	u16 calLen;
	
	//不能少于 5个字节
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
	
	play_alarm = 0;	// 电子眼语音
	play_speed = 0;	// 限速语音
	i = DATA_OFFSET;
	if(Decode_DataBuf[i]=='A') //GPS定位，可以正常使用
	{
		//第一次检测到A时，可以直接修正DVR的时间
		i++;
		if(Decode_DataBuf[i]>0)
		{
			// 发现电子眼，直接需要显示ICON，
			// 如果是第一次由0-->X, 语音需要播报，前方XXX米有XXX
			if(ElectronEyeFound == 0)
			{
				ElectronEyeFound = 1;
				play_alarm = 1;
			}
			alarmType = Decode_DataBuf[i];
			i++;
			limitSpeed = Decode_DataBuf[i];
			i++;
			
			
			//限速值在30到120公里之间，需要播报限速XX公里
			if((limitSpeed>=30)&&(limitSpeed<=120))
			{
				//Play audio
				//仅仅限于ID=2(闯红灯),3(固定测速),4(流动测速),18(区间测速)
				if(alarmType == 2 || alarmType == 3 || alarmType == 4 || alarmType == 18)
				{
					play_speed = 1;
					//printf ("GPS-->  限速值 = %d\n", limitSpeed);
				}
			}
			else
			{
				//安全点，没有限速值
				//包含所有ID，1~18,例如其中闯红灯(ID=2),也会有限速值为0的时候
				//限速值为0，请不要播报限速XX公里
			}

			//倒数距离，一般都是递减的，请不要根据倒数距离为0来判断
			//是否通过电子眼，需要根据alarmType为0来判断
			memcpy((u8*)&distance,&Decode_DataBuf[i],2);
			i += 2;
			//当前车速，不会超过220
			curSpeed = Decode_DataBuf[i];
			i++;

			//当前粗略角度，按照顺时针方向，正北为1，东北为2...
			curAngle = Decode_DataBuf[i];
			i++;

			//当前纬度，放大了10000倍，例如2212345，实际上是22.12345°
			memcpy((u8*)&curLattitude,&Decode_DataBuf[i],3);
			i += 3;

			//当前经度，放大了10000倍，例如11934567，实际上是119.34567°
			memcpy((u8*)&curLongitude,&Decode_DataBuf[i],3);
			i += 3;

			//后面的不再说明
			//有一点特别注意，后面的时间是标准的UTC时间
			//需要转换成北京时间，但是绝对不可以使用UTC+8，
			//对小时进行简单的加8处理，需要使用time.h里面的
			//函数，加上86400(8*60*60)，再换算成为标准的时间
			//例如，当前UTC时间是2015.12.31 23:58:59，正确的
			//北京时间应该是2016.01.01 07:58:59
			// 年月日输出为0xFF为无效的参数，如有效，年的定义为数据加上2000
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
				
				// 第一次有效时间
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
			//如果是由X-->0,说明电子眼消失，需要清除ICON显示
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
		
		// 播报电子眼语音
		if(play_speed)
		{
			// 播报限速语音
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

static u16 gps_data_ver;		// 数据版本号
static u16 gps_code_ver;		// 固件版本号
static void Version_Command_Analysis(void)
{
	// 版本参数 (3 bytes)
	// 1~2 (数据版本号) 3 (固件版本号)
	// ：此信息在开机后主动连续发送十次（每次间隔时间大约为1s），以方便记录仪查看当前的版本信息，连续十次之后就停止发送； 
	//	注意，固件版本号和数据版本号需要显示在相应的菜单或者界面中，以便能查阅。
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
	// 异常提示信息（Dog-----Car Record）
	// Error Type (1byte)
	// 0x01:读取声音文件数量异常，可能文件被删除(预警仪自行播放声音有此校验)； 
	// 0x02:验证预警仪数据失败，可能需要格式化U盘，重新复制全部文件到U盘中去； 
	// 0x04:预警仪数据下载不完整，请重新下载；
	// 0x05:读取FLASH失败或者U盘异常；
	u16 i;
	i = DATA_OFFSET;
	ErrorType = Decode_DataBuf[i];
}


static void Update_Command_Analysis(void)
{
	// 正在升级数据提示信息（Dog-----Car Record）
	// Link Status (1byte)
	// USB通讯是通过mass storage的方式实现的，用户插入电脑，点击升级工具，下载数据到FLASH上面覆盖；
	// 中途不可断电，也不允许复制其它的文件到U盘里面(后续会影响下次预警仪数据升级的完整性)，
 	// 0x01：USB连接成功
}

void Decode_Uart_Data(void)
{
	switch(Decode_DataBuf[3])
	{
   	case 0x81:
		// GPS 预警信息
		GPS_Command_Analysis();
		break;

	case 0x82:
		//版本参数信息
		Version_Command_Analysis();
		break;	

	case 0x83:
		//异常提示信息
		Error_Command_Analysis();
		break;
		
	case 0x84:
		//正在升级数据提示信息
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

// 读取电子狗版本信息
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

// 读取电子狗报警信息
// 返回值
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
