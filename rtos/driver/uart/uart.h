#ifndef _UART_H_
#define _UART_H_

#if defined (__cplusplus)
	extern "C"{
#endif
		
#include "types.h"
/************************************************************************
      UART 0
*************************************************************************/
//#define UART_CLK 6000000
//#define UART_CLK 10000000   
//#define UART_CLK 15000000
//#define UART_CLK 18000000		
//#define UART_CLK 20000000
#define UART_CLK 24000000
//#define UART_CLK 25000000
//#define UART_CLK 27000000
//#define UART_CLK 30000000
//#define UART_CLK 33000000
//#define UART_CLK 35000000
//#define UART_CLK 50000000		
//#define UART_CLK 54000000		// LCD与NTSC显示模式切换使用27M

/************************************************************************
      UART 1
*************************************************************************/
//#define UART1_CLK          15000000
//#define UART1_CLK        20000000
//#define UART1_CLK        24000000
//#define UART1_CLK 			27000000
#define UART1_CLK 			30000000
//#define UART1_CLK 			50000000
      
      
void uart0_puts(const UINT8 *buf);
void uart0_putc(INT8 ch);
INT8 uart0_getc(void);
void uart0_init(UINT32 baud);
void Uart_GetString(INT8 *string);
INT32 Uart_GetIntNum(INT8 *string);
		
void UART0IntHandler(void);

extern void uart_init (void);

extern void uart0_init(unsigned int baud);

extern void uart0_send_char(char ch);

extern void SendUart0Char( char ch );

extern void SendUart0String(char * buf);

extern char HexToChar(unsigned char value);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif

