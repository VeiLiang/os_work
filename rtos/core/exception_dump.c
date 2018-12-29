#include <stdio.h>
#include <string.h>
//#include <common.h>
#include "types.h"
#include "hardware.h"


//#define	_DEBUG_DUMP_

extern void ExceptionUND(void);
extern void ExceptionPAB(void);
extern void ExceptionDAB(void);

#define SendChar(ch)						\
{												\
	while( (rUART0_FR & 0x20) );		\
	if (ch == '\n')           			\
	{											\
		rUART0_DR = '\r';					\
		while( (rUART0_FR & 0x20) );	\
	}											\
	rUART0_DR = ch;							\
}


void ExceptionHandlerInit (void)
{
	
	// 安装异常处理向量
	*(unsigned int *)0x24 = (unsigned int)ExceptionUND;
	*(unsigned int *)0x2C = (unsigned int)ExceptionPAB;
	*(unsigned int *)0x30 = (unsigned int)ExceptionDAB;
}

#if !defined(_DEBUG_DUMP_)
extern void _ExceptionDump_ (unsigned int *stackframe, unsigned int exception_type)
{	
	
	register unsigned int addr;
	if(exception_type == 0)		// 未定义指令异常
	{
		SendChar('U');
		SendChar('N');
		SendChar('D');
	}	
	else if(exception_type == 1)		// 指令预取异常
	{
		SendChar('P');
		SendChar('A');
		SendChar('B');
	}
	else if(exception_type == 2)		// 指令预取异常
	{
		SendChar('D');
		SendChar('A');
		SendChar('B');
	}
	else
	{
		SendChar('R');
		SendChar('E');
		SendChar('V');
	}		
	
	/*dump ((unsigned int)g_fifo.pi16FIFO);
	dump ((unsigned int)g_fifo.pi16FIFOEnd);
	dump ((unsigned int)g_fifo.u32Samples);
	dump ((unsigned int)g_fifo.pi16Head);
	dump ((unsigned int)g_fifo.pi16Tail);
	dump ((unsigned int)g_fifo.u32SamplesInFIFO);*/
}

#else

//#define	DUMP_printf		DrvSIO_SemiPrintf
#define	DUMP_printf		printf
extern void _ExceptionDump_ (unsigned int *stackframe, unsigned int exception_type)
{
	if(exception_type == 0)		// 未定义指令异常
	{
		SendChar('U');
		SendChar('N');
		SendChar('D');
	}	
	else if(exception_type == 1)		// 指令预取异常
	{
		SendChar('P');
		SendChar('A');
		SendChar('B');
	}
	else if(exception_type == 2)		// 指令预取异常
	{
		SendChar('D');
		SendChar('A');
		SendChar('B');
	}
	else
	{
		SendChar('R');
		SendChar('E');
		SendChar('V');
	}		
	
	DUMP_printf ("\nRegister Dump\n");
	DUMP_printf ("SPSR    : %08X\n", *stackframe ++);
	DUMP_printf ("R13(SP) : %08X\n", *stackframe ++);
	DUMP_printf ("R0      : %08X\n", *stackframe ++);
	DUMP_printf ("R1      : %08X\n", *stackframe ++);
	DUMP_printf ("R2      : %08X\n", *stackframe ++);
	DUMP_printf ("R3      : %08X\n", *stackframe ++);
	DUMP_printf ("R4      : %08X\n", *stackframe ++);
	DUMP_printf ("R5      : %08X\n", *stackframe ++);
	DUMP_printf ("R6      : %08X\n", *stackframe ++);
	DUMP_printf ("R7      : %08X\n", *stackframe ++);
	DUMP_printf ("R8      : %08X\n", *stackframe ++);
	DUMP_printf ("R9      : %08X\n", *stackframe ++);
	DUMP_printf ("R10     : %08X\n", *stackframe ++);
	DUMP_printf ("R11     : %08X\n", *stackframe ++);
	DUMP_printf ("R12     : %08X\n", *stackframe ++);
	DUMP_printf ("R14(LR) : %08X\n", *stackframe ++);
}
#endif