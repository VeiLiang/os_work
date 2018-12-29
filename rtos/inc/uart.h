#ifndef _UART_H_
#define _UART_H_

#if defined (__cplusplus)
	extern "C"{
#endif
		
#include "types.h"
		
void uart0_puts(const UINT8 *buf);
void uart0_putc(INT8 ch);
INT8 uart0_getc(void);
void uart0_init(UINT32 baud);

extern void uart0_init(unsigned int baud);

extern void uart0_send_char(char ch);

extern void SendUart0Char( char ch );

extern void SendUart0String(char * buf);

extern char HexToChar(unsigned char value);

extern int  uart0_check_char_received (void);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif

