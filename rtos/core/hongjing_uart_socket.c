/*
**********************************************************************
Copyright (c)2014 SHENZHEN Exceedspace Company.  All Rights Reserved
Filename: 
Version : 
Date    : 
Author  : tangchao
Abstract: 
History :
***********************************************************************
*/
#include <hardware.h>
#include "RTOS.h"		// OS头文件
#include <stdio.h>
#include <string.h>
#include "xm_core.h"
#include "xm_uart.h"
#include "xm_key.h"
#include "xm_dev.h"
#include "xm_app_menudata.h"

#if HONGJING_CVBS

xm_uart_dev_t hongjing_socket_dev = NULL;
#pragma data_alignment=32
static OS_TASK TCB_HONGJING_socket_Task;
#pragma data_alignment=32
static OS_STACKPTR int StackHONGJING_socket[0x1000/4];    


//#define	TP_DEBUG		printf

#define	TP_DEBUG		

#define	XMSYS_HONGJING_SOCKET_TASK_PRIORITY		200
#define	HONGJING_CMD_TIMEOUT						100		// 100ms

static int tp_command (u8_t *data, u8_t size)
{
 	// DATA0，DATA1为X坐标，高字节在前，低字节在后；
   //  DATA2，DATA3为Y坐标，高字节在前，低字节在后；
	u32_t x, y;
	
	if(size != 4)
		return -1;
	
	x = (data[0] << 8) | data[1];
	y = (data[2] << 8) | data[3];
	TP_DEBUG ("tp.x = %d, tp.y = %d\n", x, y);
	
	XM_TouchEventProc (MAKELONG(x, y), TOUCH_TYPE_DOWN);
	XM_TouchEventProc (MAKELONG(x, y), TOUCH_TYPE_UP);
	
	return 0;
}

static int key_command (u8_t *data, u8_t size)
{
	// 键值定义：  
	//	0x00: KEY_OK（确认/录像）
	//	0x01: KEY_UP（向上）
	//	0x02: KEY_DOWN（向下）
	//	0x03: KEY_MODE（模式）
	// 	0x04: KEY_MENU（菜单）
	// 	0x05: KEY_MUTE（静音）
	if(size != 1)
		return -1;
	
	if(data[0] == 0)
	{
		TP_DEBUG ("KEY_OK\n");
		XM_KeyEventProc (VK_AP_SWITCH, XMKEY_PRESSED);
		XM_KeyEventProc (VK_AP_SWITCH, 0);
	}
	else if(data[0] == 1)
	{
		TP_DEBUG ("KEY_UP\n");
		XM_KeyEventProc (VK_AP_UP, XMKEY_PRESSED);
		XM_KeyEventProc (VK_AP_UP, 0);
	}
	else if(data[0] == 2)
	{
		TP_DEBUG ("KEY_DOWN\n");
		XM_KeyEventProc (VK_AP_DOWN, XMKEY_PRESSED);
		XM_KeyEventProc (VK_AP_DOWN, 0);
	}
	else if(data[0] == 3)
	{
		TP_DEBUG ("KEY_MODE\n");
		XM_KeyEventProc (VK_AP_MODE, XMKEY_PRESSED);
		XM_KeyEventProc (VK_AP_MODE, 0);
	}
	else if(data[0] == 4)
	{
		TP_DEBUG ("KEY_MENU\n");
		XM_KeyEventProc (VK_AP_MENU, XMKEY_PRESSED);
		XM_KeyEventProc (VK_AP_MENU, 0);
	}
	else if(data[0] == 5)
	{
		TP_DEBUG ("KEY_MUTE\n");
		
		#if 0
		AP_SetMenuItem (APPMENUITEM_MIC, AP_SETTING_MIC_OFF);
		#else //tony.yue 20170626
		if(AP_GetMenuItem (APPMENUITEM_MIC) == 0)
		{
			// 开启MIC录音
			AP_SetMenuItem (APPMENUITEM_MIC, AP_SETTING_MIC_ON);
		}
		else
		{
			// 关闭MIC
			AP_SetMenuItem (APPMENUITEM_MIC, AP_SETTING_MIC_OFF);
		}
		#endif
		
		AP_SaveMenuData (&AppMenuData);
	}
	else
	{
		TP_DEBUG ("KEY INVALID\n");
		return -1;
	}
	
	return 0;
}

static int time_command (u8_t *data, u8_t size)
{
	XMSYSTEMTIME time;
	// 固定为YYYYMMDDHHMMSS的格式，高位字节在前，低位字节在后
	// 开机启动时，导航仪读取系统时间或者在导航关机前这两种情况下将系统时间发送给行车记录仪，保证行车记录仪记录的实时性：
	// 以下以2015年11月25日16时40分09秒为例，核心板就发如下指令：
	// 0xFF  0xAA  0x03  0x07  0x07  0xDF  0x0B  0x19  0x10  0x28  0x09  0x55
	TP_DEBUG ("RTC Time\n");
	if(size != 7)
		return -1;
	
	time.wYear = (WORD)(data[0] << 8) | (data[1] << 0);
	time.bMonth = data[2];
	time.bDay = data[3];
	time.bHour = data[4];
	time.bMinute = data[5];
	time.bSecond = data[6];
	
	XM_SetLocalTime (&time);
	
	return 0;
}


static void HONGJING_socket_Task (void)
{
	// 三，	通信协议帧内容（坐标命令部分）：
	//  帧格式：（导航仪发给行车记录仪）(9)
	// 	同步字节	命令字	数据长度	数据内容	校验和
	// 	0xFF+0xAA	0x01	0x04	Data0-data3	累加校验
	//	响应命令帧格式：（行车记录仪发给导航仪）       
	//	同步字节	命令字	数据长度	返回码
	//	0xFF+0xAA	0x01	0x01	累加校验码
	//	返回码：为导航仪发给行车记录仪的坐标命令的校验和
	//
	// 四，	通信协议帧内容（按键命令部分）：
	//  帧格式：（导航仪发给行车记录仪）(6)
	// 	同步字节	命令字	数据长度	数据内容	校验和
	// 	0xFF+0xAA	0x02	0x01	键值	累加校验
	//  响应命令帧格式：（行车记录仪发给导航仪）  
	// 	同步字节	命令字	数据长度	返回值
	//	0xFF+0xAA	0x02	0x01	累加校验码
	//	返回值：为导航仪发给行车记录仪按键命令的校验和
	// 
	// 五，	通信协议帧内容（时间显示命令部分）：
	//  帧格式： (12)
	//	同步字节	命令字	数据长度	数据内容	校验和
	// 	0xFF+0xAA	0x03	0x07	时间	累加校验
	//	响应命令帧格式：（行车记录仪发给导航仪）       
	//	同步字节	命令字	数据长度	返回值
	//	0xFF+0xAA	0x03	0x01	累加校验码
    //  返回值：为导航仪发给行车记录仪时间命令的校验和


	unsigned char ch;
	int ret;
	u8_t id[16];
	u8_t cmd;
	u8_t len;
	u8_t crc;
	int i;
	
	printf ("HONGJING_socket_TASK start\n");
	
	while (1)
	{
		ret = xm_uart_read (hongjing_socket_dev, id, 1, HONGJING_CMD_TIMEOUT);
		if(ret == 1)
		{
			//TP_DEBUG ("id[0] = 0x%02x\n", id[0]);
			if(id[0] != 0xFF)
				continue;
			
step_scan_AA:			
			ret = xm_uart_read (hongjing_socket_dev, id + 1, 1, HONGJING_CMD_TIMEOUT);
			if(ret != 1)
				continue;
			if(id[1] == 0xFF)
			{
				id[0] = 0xFF;
				goto step_scan_AA;
			}
			if(id[1] != 0xAA)
				continue;
			// 读取命令字
			ret = xm_uart_read (hongjing_socket_dev, id + 2, 1, HONGJING_CMD_TIMEOUT);
			if(ret != 1)
				continue;
			// 命令是否有效
			cmd = id[2];
			// 读取1字节数据长度
			ret = xm_uart_read (hongjing_socket_dev, id + 3, 1, HONGJING_CMD_TIMEOUT);
			if(ret != 1)
				continue;
			len = id[3];
			if(len == 0 || len > 7)
			{
				TP_DEBUG ("HONGJING: illegal len (%d)\n", len);
				continue;
			}
			// 读取数据+校验和
			ret = xm_uart_read (hongjing_socket_dev, id + 4, len + 1, HONGJING_CMD_TIMEOUT);
			if(ret != (len + 1))
				continue;
			crc = 0;
			//  校验方式：校验和为从命令字开始的累加和校验（取低字节）
			for (i = 2; i < (4 + len); i ++)
				crc = (u8_t)(crc + id[i]);
			if(crc != id[4 + len])
			{
				TP_DEBUG ("HONGJING: crc (0x%02x) mismatch (0x%02x)\n", crc, id[4 + len]);
				continue;
			}
			
			TP_DEBUG ("HONGJING: cmd[] = ");
			for (i = 0; i < (5 + len); i ++)
			{
				TP_DEBUG ("%02x ", id[i]);
			}
			TP_DEBUG ("\n");
			
			// 处理命令
			switch(cmd)
			{
				case 0x01:	// 坐标命令部分
					ret = tp_command (id + 4, len);
					break;
				
				case 0x02:	// 按键命令部分
					ret = key_command (id + 4, len);
					break;
				
				case 0x03:	// 时间显示命令部分
					ret = time_command (id + 4, len);
					break;
					
				case 0x05:
					ret = 0;
					break;
					
				default:
					TP_DEBUG ("HONGJING: illegal cmd (%d)\n", cmd);
					ret = -1;
					break;
			}
			
			if(ret == 0)
			{
				// 应答导航仪
				id[0] = 0xFF;
				id[1] = 0xAA;
				id[2] = cmd;
				id[3] = 0x01;
				id[4] = crc;
				if(xm_uart_write (hongjing_socket_dev, id, 5, -1) < 0)
				{
					//TP_DEBUG ("HONGJING: send resp ng\n");
				}
				else
				{
					//TP_DEBUG ("HONGJING: send resp OK\n");
				}
			}
		}
		else
		{
			OS_Delay (1);
		}
	}
}


/******************************************************************************
	XMSYS_MessageSocketInit
	初始化宏景 串口接收数据。
********************************************************************************/
void XMSYS_MessageSocketInit (void)
{
#if JLINK_DISABLE
	// UART1 与 JLink复用
	int err = XM_UART_ERRCODE_ILLEGAL_PARA;
	unsigned int dev;
	
	dev = XM_UART_DEV_1;		// 使用UART 1与车机通信
	
	// 波特率：115200，8位数据，1位停止位，无奇偶校验，无硬件流控制
	hongjing_socket_dev = xm_uart_open (dev, 
											  XM_UART_STOPBIT_1,
											  XM_UART_PARITY_DISABLE,
											  XM_UART_MODE_TR,
											  115200,
											  512,
											  &err);
	if(hongjing_socket_dev == NULL)
	{
		printf ("open hongjing socket failed, err_code=%d\n", err);
		return;
	}
											  
											  
	// 创建一个接收串口包的任务。
	OS_CREATETASK (&TCB_HONGJING_socket_Task, "HONGJING_socket", HONGJING_socket_Task, XMSYS_HONGJING_SOCKET_TASK_PRIORITY, StackHONGJING_socket );	
#else

#endif	
}

#endif