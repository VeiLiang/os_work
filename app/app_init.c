//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_init.c
//	  应用初始化/退出过程及全局变量/全局函数定义
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

static DWORD	startup_ticket;			// 纪录系统开机时间


/******************************************************************
 - 功能描述：将一个字符串转为32位的变量，比如"1234"转为1234
 - 隶属模块：公开函数模块
 - 函数属性：外部，用户可调用
 - 参数说明：str:指向待转换的字符串           
 - 返回说明：转换后的数值
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
 - 功能描述：将一个32位的变量dat转为字符串，比如把1234转为"1234"
 - 隶属模块：公开函数模块
 - 函数属性：外部，用户可调用
 - 参数说明：dat:带转的long型的变量
             str:指向字符数组的指针，转换后的字节串放在其中           
 - 返回说明：无
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

// 系统启动时由系统负责调用，只调用一次
void XM_AppInit (void)
{
	XMSYSTEMTIME defaulttime;

	defaulttime = get_default_time();
	XM_AppInitStartupTicket();

	// 加载并检查设置信息
	AP_LoadMenuData(&AppMenuData);
	AP_VerifyMenuData(&AppMenuData);
	
	// 开启背光
	AP_SetMenuItem (APPMENUITEM_LCD, AppMenuData.lcd);
	
	// 设置录像的分辨率/帧率
	XMSYS_H264CodecSetVideoFormat (AppMenuData.video_resolution);

	if(AppMenuData.update_date_falg==0x0)//未更新
	{
		AppMenuData.update_date_falg = 1;
		XM_SetLocalTime(&defaulttime);
	}

	//XM_printf ("XM_AppInit Success\n");
}

// 系统启动时由系统负责调用，只调用一次
void XM_AppExit (void)
{

}
void XM_AppInitStartupTicket (void)
{
      startup_ticket = XM_GetTickCount();
}
// 获取APP启动时间
DWORD AppGetStartupTicket (void)
{
      
	return startup_ticket;
}

// 投递系统事件到消息队列
VOID AP_PostSystemEvent (unsigned int event)
{
	XM_lock ();
	XM_KeyEventProc ((unsigned short)VK_AP_SYSTEM_EVENT, (unsigned short)event);
	XM_unlock ();
}





