//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_init.c
//	  Ӧ�ó�ʼ��/�˳����̼�ȫ�ֱ���/ȫ�ֺ�������
//
//	Revision history
//
//		2012.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_voice.h"
#include "xm_h264_codec.h"
#include "rtc.h"

static DWORD	startup_ticket;			// ��¼ϵͳ����ʱ��


/******************************************************************
 - ������������һ���ַ���תΪ32λ�ı���������"1234"תΪ1234
 - ����ģ�飺��������ģ��
 - �������ԣ��ⲿ���û��ɵ���
 - ����˵����str:ָ���ת�����ַ���           
 - ����˵����ת�������ֵ
 ******************************************************************/
unsigned long strtou32(unsigned char *str, unsigned char len) 
{
	unsigned long temp =0;
	unsigned long fact= 1;
	unsigned char i;
	
	for(i=len;i>0;i--)
	{
		temp+=((str[i-1]-0x30)*fact);
		fact*=10;
	}
	return temp;
}

/******************************************************************
 - ������������һ��32λ�ı���datתΪ�ַ����������1234תΪ"1234"
 - ����ģ�飺��������ģ��
 - �������ԣ��ⲿ���û��ɵ���
 - ����˵����dat:��ת��long�͵ı���
             str:ָ���ַ������ָ�룬ת������ֽڴ���������           
 - ����˵������
 ******************************************************************/
void u32tostr(unsigned long dat,char *str) 
{
	char temp[20];
	unsigned char i=0,j=0;
	
	i=0;
	while(dat)
	{
		temp[i]=dat%10+0x30;
		i++;
		dat/=10;
	}
	
	j=i;
	for(i=0;i<j;i++)
	{
		str[i]=temp[j-i-1];
	}
	
	if(!i) 
	{
		str[i++]='0';
	}
	str[i]=0;
}

XMSYSTEMTIME get_default_time(void)
{
	unsigned char cRevisionDate[12] = __DATE__;
	unsigned char cRevisionTime[8] = __TIME__;
	XMSYSTEMTIME buildtime;

	XM_printf(">>>>date:%s, time:%s\r\n", __DATE__, __TIME__);
	XM_printf(">>>>XM_AppInit\n");
    if( (cRevisionDate[0]=='J') && (cRevisionDate[1]=='a') && (cRevisionDate[2]=='n') && (cRevisionDate[3]==' ') )    
	{//1		
		buildtime.bMonth = 1;    
	}    
	else if( (cRevisionDate[0]=='F') && (cRevisionDate[1]=='e') && (cRevisionDate[2]=='b') && (cRevisionDate[3]==' ') )    
	{//2		
		buildtime.bMonth = 2;    
	}    
	else if( (cRevisionDate[0]=='M') && (cRevisionDate[1]=='a') && (cRevisionDate[2]=='r') && (cRevisionDate[3]==' ') )    
	{//3		
		buildtime.bMonth = 3;   
	}    
	else if( (cRevisionDate[0]=='A') && (cRevisionDate[1]=='p') && (cRevisionDate[2]=='r') && (cRevisionDate[3]==' ') )    
	{//4		
		buildtime.bMonth = 4;    
	}    
	else if( (cRevisionDate[0]=='M') && (cRevisionDate[1]=='a') && (cRevisionDate[2]=='y') && (cRevisionDate[3]==' ') )    
	{//5		
		buildtime.bMonth = 5;    
	}    
	else if( (cRevisionDate[0]=='J') && (cRevisionDate[1]=='u') && (cRevisionDate[2]=='n') && (cRevisionDate[3]==' ') )    
	{//6		
		buildtime.bMonth = 6;    
	}    
	else if( (cRevisionDate[0]=='J') && (cRevisionDate[1]=='u') && (cRevisionDate[2]=='l') && (cRevisionDate[3]==' ') )    
	{//7		
		buildtime.bMonth = 7;    
	}    
	else if( (cRevisionDate[0]=='A') && (cRevisionDate[1]=='u') && (cRevisionDate[2]=='g') && (cRevisionDate[3]==' ') )    
	{//8		
		buildtime.bMonth = 8;   
	}    
	else if( (cRevisionDate[0]=='S') && (cRevisionDate[1]=='e') && (cRevisionDate[2]=='p') && (cRevisionDate[3]=='t') )    
	{//9		
		buildtime.bMonth = 9;    
	}    
	else if( (cRevisionDate[0]=='O') && (cRevisionDate[1]=='c') && (cRevisionDate[2]=='t') && (cRevisionDate[3]==' ') )    
	{//10		
		buildtime.bMonth = 10;    
	}    
	else if( (cRevisionDate[0]=='N') && (cRevisionDate[1]=='o') && (cRevisionDate[2]=='v') && (cRevisionDate[3]==' ') )    
	{//11		
		buildtime.bMonth = 11;    
	}    
	else if( (cRevisionDate[0]=='D') && (cRevisionDate[1]=='e') && (cRevisionDate[2]=='c') && (cRevisionDate[3]==' ') )    
	{//12		
		buildtime.bMonth = 12;    
	}	

	buildtime.wYear = strtou32(&(*(cRevisionDate+7)), 4);
	buildtime.bDay = strtou32(&(*(cRevisionDate+4)), 2);
	buildtime.bHour = strtou32(&(*(cRevisionTime+0)), 2);
	buildtime.bMinute = strtou32(&(*(cRevisionTime+3)), 2);
	buildtime.bSecond = strtou32(&(*(cRevisionTime+6)), 2);	
	
	printf(">>>build_time.year:%d\r\n", buildtime.wYear);	
	printf(">>>build_time.month:%d\r\n", buildtime.bMonth);	
	printf(">>>build_time.day:%d\r\n", buildtime.bDay);	
	printf(">>>build_time.hour:%d\r\n", buildtime.bHour);	
	printf(">>>build_time.min:%d\r\n", buildtime.bMinute);	
	printf(">>>build_time.sec:%d\r\n", buildtime.bSecond);

	return buildtime;
}

// ϵͳ����ʱ��ϵͳ������ã�ֻ����һ��
void XM_AppInit (void)
{
	XMSYSTEMTIME defaulttime;

	defaulttime = get_default_time();
	XM_AppInitStartupTicket();

	// ���ز����������Ϣ
	AP_LoadMenuData(&AppMenuData);
	AP_VerifyMenuData(&AppMenuData);
	
	// ��������
	AP_SetMenuItem (APPMENUITEM_LCD, AppMenuData.lcd);
	
	// ����¼��ķֱ���/֡��
	XMSYS_H264CodecSetVideoFormat (AppMenuData.video_resolution);

	if(AppMenuData.update_date_falg==0x0)//δ����
	{
		AppMenuData.update_date_falg = 1;
		XM_SetLocalTime(&defaulttime);
	}

	//XM_printf ("XM_AppInit Success\n");
}

// ϵͳ����ʱ��ϵͳ������ã�ֻ����һ��
void XM_AppExit (void)
{

}
void XM_AppInitStartupTicket (void)
{
      startup_ticket = XM_GetTickCount();
}
// ��ȡAPP����ʱ��
DWORD AppGetStartupTicket (void)
{
      
	return startup_ticket;
}

// Ͷ��ϵͳ�¼�����Ϣ����
VOID AP_PostSystemEvent (unsigned int event)
{
	XM_lock ();
	XM_KeyEventProc ((unsigned short)VK_AP_SYSTEM_EVENT, (unsigned short)event);
	XM_unlock ();
}





