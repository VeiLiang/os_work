//****************************************************************************
//
//	Copyright (C) 2010~2014 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_uart_drv.c
//	  UART�����¼����� (��SecureCRT����)
//
//	Revision history
//
//		2014.04.01	ZhuoYongHong Initial version
//
//****************************************************************************

#include <xm_type.h>
#include <string.h>
#include <xm_user.h>
#include <xm_dev.h>
#include <xm_base.h>
#include <xm_key.h>
#include <xm_printf.h>
#include "xm_device.h"
#ifndef WIN32
#include "uart.h"
#endif

// ����ESC�������ַ���
static int uart_ch_size;
static unsigned char uart_ch_buff[2];
static int uart_last_key;		// ģ��DOWN��

static unsigned int uart_recv;

static volatile	BYTE		uartInit = 0;		// ��ʶUART�����Ƿ��ѳ�ʼ��


XMBOOL XM_UartEventProc (BYTE ch)
{
	if(!uartInit)
		return 0;
	
	uart_recv = ch;
	XM_wakeup ();
	return 1;
}

// UART����������
void XM_UartDriverOpen (void)
{
	uart_last_key = -1;
	uart_ch_size = 0;
	uart_recv = (unsigned int)(-1);
	memset (uart_ch_buff, 0, sizeof(uart_ch_buff));
	
	uartInit = 1;
}

// UART���������ر�
void XM_UartDriverClose (void)
{
	uartInit = 0;
	
	uart_last_key = -1;
	uart_ch_size = 0;
	uart_recv = (unsigned int)(-1);
}

XMBOOL XM_UartDriverGetEvent (WORD *key, WORD *modifier)
{
	XMBOOL ret = 0;
	
#ifndef WIN32
	if(uart_last_key != -1)
	{
		*key = (BYTE)uart_last_key;
		*modifier = 0;	// ģ�ⰴ���ͷ�
		uart_last_key = -1;
		return 1;
	}
	if(uart_recv != (unsigned int)(-1))
	{
		unsigned char ch = (unsigned char)uart_recv;
		uart_recv = -1;
		//XM_printf ("getc = 0x%02x\n", ch);
		if(uart_ch_size > 2)
			uart_ch_size = 0;
		
		if(ch == 0x1b)		// '\'
		{
			uart_ch_size = 2;
			memset (uart_ch_buff, 0, sizeof(uart_ch_buff));
		}
		else if(uart_ch_size > 0)
		{
			uart_ch_buff[2 - uart_ch_size] = ch;
			uart_ch_size --;
			if(uart_ch_size == 0)
			{
				// ESC������ȫ���������
				// ����
				if(uart_ch_buff[0] == 0x4F)
				{
					// F1 ~ F4
					if(uart_ch_buff[1] >= 0x50 && uart_ch_buff[1] <= 0x53)
					{
						uart_last_key = VK_F1 + uart_ch_buff[1] - 0x50;
					}
				}
				else if(uart_ch_buff[0] == 0x5B)
				{
					// UP/DOWN/RIGHT/LEFT
					if(uart_ch_buff[1] == 0x41)
						uart_last_key = VK_UP;
					else if(uart_ch_buff[1] == 0x42)
						uart_last_key = VK_DOWN;
					else if(uart_ch_buff[1] == 0x43)
						uart_last_key = VK_RIGHT;
					else if(uart_ch_buff[1] == 0x44)
						uart_last_key = VK_LEFT;
				}
			}
		}
		else
		{
			if( ch == '0' )
			{
				// ��ݷ�ʽ ��������
				*key = 0xf0;
				*modifier = SYSTEM_EVENT_ADJUST_BELL_VOLUME;
				ret = 1;
				XM_printf ("SYSTEM_EVENT_ADJUST_BELL_VOLUME\n");
			}
			else if( ch == '1' )
			{
				// ��ݷ�ʽ ¼������
				*key = 0xf0;
				*modifier = SYSTEM_EVENT_ADJUST_MIC_VOLUME;
				ret = 1;
				XM_printf ("SYSTEM_EVENT_ADJUST_MIC_VOLUME\n");
			}
			else if( ch == '2' )
			{
				// ��ݷ�ʽ ����¼��
				*key = 0xf0;
				*modifier = SYSTEM_EVENT_URGENT_RECORD;
				ret = 1;
				XM_printf ("SYSTEM_EVENT_URGENT_RECORD\n");
			}
			else if( ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z')
			{
				//uart_last_key = ch;
				if(ch == 'A' || ch == 'a')
					uart_last_key = VK_F1;
				else if(ch == 'B' || ch == 'b')
					uart_last_key = VK_F2;
				else if(ch == 'C' || ch == 'c')
					uart_last_key = VK_F3;
				else if(ch == 'D' || ch == 'd')
					uart_last_key = VK_F4;
				else if(ch == 'E' || ch == 'e')
					uart_last_key = VK_UP;
				else if(ch == 'F' || ch == 'f')
					uart_last_key = VK_DOWN;
				
				
			}
		}
		
		if(uart_last_key != -1)
		{
			*key = (BYTE)uart_last_key;
			*modifier = XMKEY_PRESSED|XMKEY_STROKE;
			ret = 1;
			
			XM_printf ("VK=0x%02x\n", (BYTE)uart_last_key);
		}
		
	
	}
#endif
	
	return ret;
}

//=========================
// ��ѯ�Ƿ����δ����UART�����¼�
XMBOOL XM_UartDriverPoll(void)
{
#ifdef WIN32
	return 0;
#else
	if(uart_last_key != -1)
	{
		return 1;
	}
	if(uart_recv != -1)
		return 1;
	else
		return 0;
#endif
}

//=====================