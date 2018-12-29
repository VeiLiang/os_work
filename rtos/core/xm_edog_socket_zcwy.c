/*
**********************************************************************
Copyright (c)2014 SHENZHEN Exceedspace Company.  All Rights Reserved
Filename: 众创伟业电子狗协议
Version : 
Date    : 
Author  : zhuoyonghong
Abstract: 
History :
***********************************************************************
*/
#include <hardware.h>
#include "RTOS.h"		// OS头文件
#include "hw_osd_layer.h"
#include <stdio.h>
#include <string.h>
#include "xm_core.h"
#include "xm_uart.h"
#include "Uart_Protocal.h"

xm_uart_dev_t edog_socket_dev = NULL;
#pragma data_alignment=32
static OS_TASK TCB_EDOG_socket_Task;
#pragma data_alignment=32
static OS_STACKPTR int StackEDOG_socket[0x1000/4];    


#define	XMSYS_EDOG_SOCKET_TASK_PRIORITY	200
#define	EDOG_CMD_TIMEOUT		100		// 100ms
static void EDOG_socket_Task (void)
{
	unsigned char ch;
	int ret;
	
	GPS_Init ();
	
	while (1)
	{
		ret = xm_uart_read (edog_socket_dev, &ch, 1, EDOG_CMD_TIMEOUT);
		if(ret == 1)
		{
			GPS_ReceiveOneByte (ch);
			GPS_DecodeUart ();
		}
	}
}


/******************************************************************************
	uart_edogt_init
	初始化电子狗 串口接收数据。
********************************************************************************/
void XMSYS_EdogSocketInit (void)
{
	int err = XM_UART_ERRCODE_ILLEGAL_PARA;
	unsigned int dev;
	
	dev = XM_UART_DEV_3;		// ASIC使用UART 3调试ISP
	
	// 众创伟业预警仪-行车记录仪
	// 采用UART通信，波特率9600bps，8bit数据位，无奇偶校验，1bit起始位，1bit停止位。
	edog_socket_dev = xm_uart_open (dev, 
											  XM_UART_STOPBIT_1,
											  XM_UART_PARITY_DISABLE,
											  XM_UART_MODE_RX,
											  9600,
											  512,
											  &err);
	if(edog_socket_dev == NULL)
	{
		printf ("open edog socket failed, err_code=%d\n", err);
		return;
	}
											  
											  
	// 创建一个接收串口包的任务。
	OS_CREATETASK (&TCB_EDOG_socket_Task, "EDOG_socket", EDOG_socket_Task, XMSYS_EDOG_SOCKET_TASK_PRIORITY, StackEDOG_socket );	
		
}

