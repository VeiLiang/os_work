/*
**********************************************************************
Copyright (c)2008 Arkmicro Technologies Inc.  All Rights Reserved 
Filename: uart.c
Version:  1.0
Created:  2006.12.27
By:       Harey
Abstract:
Filename: uart.c
Version:  1.1
Modified:  2008.01.10
By:       Carl
Abstract: 
***********************************************************************
*/

#include "ark1960.h"
#include "uart.h"
#include "irqs.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>



void uart0_init(unsigned int baud)
{
	unsigned int Baud_Rate_Divisor;
	
	rSYS_PAD_CTRL09 &= ~(0x0F << 16);
	rSYS_PAD_CTRL09 |=  (0x05 << 16);

   //irq_disable(UART0_INT);
	OS_ARM_DisableISRSource (UART0_INT);
   rSYS_SOFT_RSTNB &= ~(1<<6);
   __asm ("nop");
   rSYS_SOFT_RSTNB |= (1<<6);
   __asm ("nop");
	__asm ("nop");
 	__asm ("nop");
 	__asm ("nop");
 	//rAHB_SYS_REG29 |= (1<<20);
 	//rAHB_SYS_REG22 &= ~(1<<15);
			
	Baud_Rate_Divisor = ((UART_CLK<<3) + baud)/(baud<<1);
	rUART0_IBRD = Baud_Rate_Divisor >> 6;
	rUART0_FBRD = Baud_Rate_Divisor & 0x3f;
	
	rUART0_LCR_H = 0x70;//data len:8 bit,parity checking disable
	rUART0_IFLS = 0x19;
	rUART0_CR = 0x301;
	rUART0_IMSC = 0x50;
}

void uart0_putc(INT8 ch)
{
	while( (rUART0_FR & 0x20) );

	rUART0_DR = ch;
}

void uart0_puts(const UINT8*buf)
{
	INT32 i = 0;
	while ( buf[i] != 0)
	{
		while( !(rUART0_FR & 0x20) )
		{
			if ( buf[i] == '\n' )
			{
				rUART0_DR = '\r';
				while( (rUART0_FR & 0x20) );
			}

			rUART0_DR = buf[i++];
			if ( buf[i] == 0 )
				return;
		}
		while( (rUART0_FR & 0x20) );
	}
}

INT8 uart0_getc(void)
{
	INT8 rec_ch = 0xFF;
	
	while( (rUART0_FR & 0x10) );
	rec_ch = rUART0_DR;

	return rec_ch;
}

int  uart0_check_char_received (void)
{
	if(rUART0_FR & 0x10)
		return 0;
	else
		return 1;
}

void uart0_send_char( char ch )
{
	while( (rUART0_FR & 0x20) );
	if(ch == '\n')
	{
		rUART0_DR = '\r';
		while( (rUART0_FR & 0x20) );
	}
	rUART0_DR = ch;
}

char HexToChar(unsigned char value)
{
	value &= 0x0f;

	if ( value < 10 )
		return (0x30 + value);
	else
		return (0x60 + value - 9);
}
void SendUart0Char( char ch )
{
	uart0_send_char (ch);
}

void  SendUart0String(char * buf)
{
	int i = 0;
	
	while ( buf[i] != 0)
	{
		SendUart0Char(buf[i++]);
	}
}

void ShortToStr(unsigned short value, char *str)
{
	if ( str == 0 )
		return;

	str[0] = HexToChar(value >> 12);
	str[1] = HexToChar((value >> 8) & 0x0f);
	str[2] = HexToChar((value >> 4) & 0x0f);
	str[3] = HexToChar(value & 0x0f);
	str[4] = 0;
}

void IntToStr(unsigned int value, char *str)
{
	if ( str == 0 )
		return;

	ShortToStr(value >> 16, str);
	ShortToStr(value & 0xffff, str + 4);
	str[8] = 0;
}

#define	uart0_send_string SendUart0String
void PrintVariableValueHex(char * variable, unsigned int value)
{
	char buf[32];

	uart0_send_string(variable);
	uart0_send_string(": 0x");
	IntToStr(value, buf);
	uart0_send_string(buf);
	//uart0_send_string("\r\n");
}

extern XMBOOL XM_UartEventProc (BYTE ch);



void Uart_GetString(INT8 *string)
{
	INT8 c;
	INT8 node = 0;
	//uart0_putc('>');
	while(1)
	{
		c = uart0_getc();
		string[node] = c;

		if((string[node-1]==92)&&(string[node]=='n'))
		{
			node--;
			break;
		}
		else if(string[node]== 0xd)
		{
			uart0_putc('\r');
			uart0_putc('\n');
			break;
		}
		else
			uart0_putc(c);

		node++;
	}
	string[node]='\0';
}

////////////////////////////////////////////////////////
INT32 Uart_GetIntNum(INT8 *string)
{
	INT32 LastIndex;
	INT32 i;
	INT32 Result = 0;
	INT32 Base = 10;

	if( string[0]== 0 )
		string ++;
	if(string[0]=='0' && (string[1]=='x' || string[1]=='X'))
	{
		Base = 16;
		string += 2;
	}

	LastIndex = strlen((char *)string) - 1;
	if(LastIndex < 0)
		return -1;

	if(string[LastIndex]=='h' || string[LastIndex]=='H' )
	{
		Base = 16;
		string[LastIndex] = 0;
		LastIndex--;
	}

	if(Base==10)        //decimalist
	{
		Result = atoi((char *)string);
	}
	else        //Hex
	{
		for(i=0;i<=LastIndex;i++)
		{
			if(isalpha(string[i]))
			{
				if(isupper(string[i]))
					Result = (Result<<4) + string[i] - 'A' + 10;
				else
					Result = (Result<<4) + string[i] - 'a' + 10;
			}
			else
				Result = (Result<<4) + string[i] - '0';
		}
	}

	return Result;
}


void uart_init (void)
{
	//uart0_init(57600);
	//uart0_init(19200);
	//uart0_init (115200);
	//SendUart0String ("Ark1960 SoC Booting . . .\r\n");
	rUART0_LCR_H = 0x60; //disable fifo 
	//rUART0_IFLS = 0x12;   // 
	if( rUART0_RSR )
	{
		char ch;
		while( !(rUART0_FR & 0x10) )//the RXFE bit is set when the receive FIFO is empty.
		{
			ch = rUART0_DR ;
			(void) ch;
		}
		rUART0_RSR=0xffff;
	}

	rUART0_IMSC  |= (0x1<<4);//base+0x38:bit.4:the mask of the RXIM interrupt is set.
	//rUART0_IMSC  |= (0x1<<4)|(1<<5)|(1<<6);//base+0x38:bit.4:the mask of the RXIM interrupt is set.
	
	rUART0_ICR = rUART0_MIS;//查询中断状态，清除中断
	//char ch = rUART0_DR;
	//request_irq(UART0_INT,PRIORITY_FIFTEEN, UART0IntHandler);

}

