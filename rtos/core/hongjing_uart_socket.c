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
#include "RTOS.h"		// OSͷ�ļ�
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
 	// DATA0��DATA1ΪX���꣬���ֽ���ǰ�����ֽ��ں�
   //  DATA2��DATA3ΪY���꣬���ֽ���ǰ�����ֽ��ں�
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
	// ��ֵ���壺  
	//	0x00: KEY_OK��ȷ��/¼��
	//	0x01: KEY_UP�����ϣ�
	//	0x02: KEY_DOWN�����£�
	//	0x03: KEY_MODE��ģʽ��
	// 	0x04: KEY_MENU���˵���
	// 	0x05: KEY_MUTE��������
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
			// ����MIC¼��
			AP_SetMenuItem (APPMENUITEM_MIC, AP_SETTING_MIC_ON);
		}
		else
		{
			// �ر�MIC
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
	// �̶�ΪYYYYMMDDHHMMSS�ĸ�ʽ����λ�ֽ���ǰ����λ�ֽ��ں�
	// ��������ʱ�������Ƕ�ȡϵͳʱ������ڵ����ػ�ǰ����������½�ϵͳʱ�䷢�͸��г���¼�ǣ���֤�г���¼�Ǽ�¼��ʵʱ�ԣ�
	// ������2015��11��25��16ʱ40��09��Ϊ�������İ�ͷ�����ָ�
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
	// ����	ͨ��Э��֡���ݣ���������֣���
	//  ֡��ʽ���������Ƿ����г���¼�ǣ�(9)
	// 	ͬ���ֽ�	������	���ݳ���	��������	У���
	// 	0xFF+0xAA	0x01	0x04	Data0-data3	�ۼ�У��
	//	��Ӧ����֡��ʽ�����г���¼�Ƿ��������ǣ�       
	//	ͬ���ֽ�	������	���ݳ���	������
	//	0xFF+0xAA	0x01	0x01	�ۼ�У����
	//	�����룺Ϊ�����Ƿ����г���¼�ǵ����������У���
	//
	// �ģ�	ͨ��Э��֡���ݣ���������֣���
	//  ֡��ʽ���������Ƿ����г���¼�ǣ�(6)
	// 	ͬ���ֽ�	������	���ݳ���	��������	У���
	// 	0xFF+0xAA	0x02	0x01	��ֵ	�ۼ�У��
	//  ��Ӧ����֡��ʽ�����г���¼�Ƿ��������ǣ�  
	// 	ͬ���ֽ�	������	���ݳ���	����ֵ
	//	0xFF+0xAA	0x02	0x01	�ۼ�У����
	//	����ֵ��Ϊ�����Ƿ����г���¼�ǰ��������У���
	// 
	// �壬	ͨ��Э��֡���ݣ�ʱ����ʾ����֣���
	//  ֡��ʽ�� (12)
	//	ͬ���ֽ�	������	���ݳ���	��������	У���
	// 	0xFF+0xAA	0x03	0x07	ʱ��	�ۼ�У��
	//	��Ӧ����֡��ʽ�����г���¼�Ƿ��������ǣ�       
	//	ͬ���ֽ�	������	���ݳ���	����ֵ
	//	0xFF+0xAA	0x03	0x01	�ۼ�У����
    //  ����ֵ��Ϊ�����Ƿ����г���¼��ʱ�������У���


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
			// ��ȡ������
			ret = xm_uart_read (hongjing_socket_dev, id + 2, 1, HONGJING_CMD_TIMEOUT);
			if(ret != 1)
				continue;
			// �����Ƿ���Ч
			cmd = id[2];
			// ��ȡ1�ֽ����ݳ���
			ret = xm_uart_read (hongjing_socket_dev, id + 3, 1, HONGJING_CMD_TIMEOUT);
			if(ret != 1)
				continue;
			len = id[3];
			if(len == 0 || len > 7)
			{
				TP_DEBUG ("HONGJING: illegal len (%d)\n", len);
				continue;
			}
			// ��ȡ����+У���
			ret = xm_uart_read (hongjing_socket_dev, id + 4, len + 1, HONGJING_CMD_TIMEOUT);
			if(ret != (len + 1))
				continue;
			crc = 0;
			//  У�鷽ʽ��У���Ϊ�������ֿ�ʼ���ۼӺ�У�飨ȡ���ֽڣ�
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
			
			// ��������
			switch(cmd)
			{
				case 0x01:	// ���������
					ret = tp_command (id + 4, len);
					break;
				
				case 0x02:	// ���������
					ret = key_command (id + 4, len);
					break;
				
				case 0x03:	// ʱ����ʾ�����
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
				// Ӧ�𵼺���
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
	��ʼ���꾰 ���ڽ������ݡ�
********************************************************************************/
void XMSYS_MessageSocketInit (void)
{
#if JLINK_DISABLE
	// UART1 �� JLink����
	int err = XM_UART_ERRCODE_ILLEGAL_PARA;
	unsigned int dev;
	
	dev = XM_UART_DEV_1;		// ʹ��UART 1�복��ͨ��
	
	// �����ʣ�115200��8λ���ݣ�1λֹͣλ������żУ�飬��Ӳ��������
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
											  
											  
	// ����һ�����մ��ڰ�������
	OS_CREATETASK (&TCB_HONGJING_socket_Task, "HONGJING_socket", HONGJING_socket_Task, XMSYS_HONGJING_SOCKET_TASK_PRIORITY, StackHONGJING_socket );	
#else

#endif	
}

#endif