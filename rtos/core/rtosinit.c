/*********************************************************************
*
*   IAR PowerPac
*
*   (c) Copyright IAR Systems 2010.  All rights reserved.
*
**********************************************************************
----------------------------------------------------------------------
File    : RTOSInit.c
Purpose : RTOSInit for STM32F107 (MB784 / STM3210D_EVAL). Initializes
          and handles the hardware for the OS as far as required by
          the OS.
--------  END-OF-HEADER  ---------------------------------------------
*/
#include "hardware.h"

#include "timer.h"
#include "rtc.h"
#include "gpio.h"
#include "xm_adc.h"
#include "xm_power.h"
#ifdef _XMSYS_FS_NANDFLASH_SUPPORT_
#include "nand_base.h"
#endif
#include <xm_dev.h>
#include <xm_base.h>
#include "xm_core.h"
#include "xm_i2c.h"
#include "xm_uart.h"
#include "ssi.h"

//extern void xm_uart_init (void);
extern void OS_Systick(void);
extern void xm_do_change_task (void);




/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/
static void _OS_ISR_Undefined(void);
static void _OS_ISR_Undefined_0(void);


/*********************************************************************
*
*       _Init_AITC()
*
* Function description
*   Clears and disables all interrut vectors in AITC.
*/
static void _Init_AITC(void)
{
	//	Do not mask any Normal interrupts by priority
	int i;
	rICSET = 0x05;
	rICMODE = 0;	
	rICMASK = 0xffffffff;
	
	// ARK1960, all interrupt level trigger
	rICLEVEL = 0x00000000;
   rICLEVEL |= (1 << LCD_INT);	// LCD
	rICLEVEL |= (1 << TIMER0_INT);	// OS系统时钟
	rICLEVEL |= (1 << TIMER12345_INT);	//  for test
	rICLEVEL |= (1 << USB_INT);
	rICLEVEL |= (1 << DMA_INT);
	rICLEVEL |= (1 << RCRT_INT);		// 遥控
	rICLEVEL |= (1 << SDHC0_INT);		// SDMMC0
	rICLEVEL |= (1 << SDHC1_INT);		// SDMMC1
	rICLEVEL |= (1 << RTC_INT);		// RTC
	rICLEVEL |= (1 << I2C_INT);		// I2C
   rICLEVEL |= (1 << ITU656_INT);	// ITU656
   rICLEVEL |= (1 << UART0_INT);		// UART 0
   rICLEVEL |= (1 << UART123_INT);		// UART 1
	rICLEVEL |= (1 << MAC_INT);		// MAC
	rICLEVEL |= (1 << ISP_INT);		// ISP_INT
	rICLEVEL |= (1 << CODEC_ENCODER_INT);// H264/JPEG encoder
	rICLEVEL |= (1 << CODEC_DECODER_INT);// H264/JPEG decoder
	rICLEVEL |= (1 << SCALE_INT);
   rICLEVEL |= (1 << ISP_SCALE_INT);
	rICLEVEL |= (1 << ADC_INT);
	//rICLEVEL = 0x0;
	
	// all interrupts are disable
	
	// all interrupt lines are routed to the normal interrupt

	// All interrupts with lowest prioriry
	
	for (i = 0; i < _NUM_INT_VECTORS; i ++)
	{
		itc_handler[i] = _OS_ISR_Undefined;
	}
	itc_handler[0] = _OS_ISR_Undefined_0;
	
	rIRQISPC = 0xffffffff;
}


#define	VECTOR_TABLE_BASE		0x80000000
/*
*********************************************************************************************************
*                                           EXCEPTION DEFINES
*********************************************************************************************************
*/

                                                 /* ARM exception IDs                                  */
#define  OS_CPU_ARM_EXCEPT_RESET                                                                    0x00
#define  OS_CPU_ARM_EXCEPT_UNDEF_INSTR                                                              0x01
#define  OS_CPU_ARM_EXCEPT_SWI                                                                      0x02
#define  OS_CPU_ARM_EXCEPT_PREFETCH_ABORT                                                           0x03
#define  OS_CPU_ARM_EXCEPT_DATA_ABORT                                                               0x04
#define  OS_CPU_ARM_EXCEPT_ADDR_ABORT                                                               0x05
#define  OS_CPU_ARM_EXCEPT_IRQ                                                                      0x06
#define  OS_CPU_ARM_EXCEPT_FIQ                                                                      0x07
#define  OS_CPU_ARM_EXCEPT_NBR                                                                      0x08
                                                 /* ARM exception vectors addresses                    */
#define  OS_CPU_ARM_EXCEPT_RESET_VECT_ADDR              (OS_CPU_ARM_EXCEPT_RESET          * 0x04 + 0x00 + VECTOR_TABLE_BASE)
#define  OS_CPU_ARM_EXCEPT_UNDEF_INSTR_VECT_ADDR        (OS_CPU_ARM_EXCEPT_UNDEF_INSTR    * 0x04 + 0x00 + VECTOR_TABLE_BASE)
#define  OS_CPU_ARM_EXCEPT_SWI_VECT_ADDR                (OS_CPU_ARM_EXCEPT_SWI            * 0x04 + 0x00 + VECTOR_TABLE_BASE)
#define  OS_CPU_ARM_EXCEPT_PREFETCH_ABORT_VECT_ADDR     (OS_CPU_ARM_EXCEPT_PREFETCH_ABORT * 0x04 + 0x00 + VECTOR_TABLE_BASE)
#define  OS_CPU_ARM_EXCEPT_DATA_ABORT_VECT_ADDR         (OS_CPU_ARM_EXCEPT_DATA_ABORT     * 0x04 + 0x00 + VECTOR_TABLE_BASE)
#define  OS_CPU_ARM_EXCEPT_ADDR_ABORT_VECT_ADDR         (OS_CPU_ARM_EXCEPT_ADDR_ABORT     * 0x04 + 0x00 + VECTOR_TABLE_BASE)
#define  OS_CPU_ARM_EXCEPT_IRQ_VECT_ADDR                (OS_CPU_ARM_EXCEPT_IRQ            * 0x04 + 0x00 + VECTOR_TABLE_BASE)
#define  OS_CPU_ARM_EXCEPT_FIQ_VECT_ADDR                (OS_CPU_ARM_EXCEPT_FIQ            * 0x04 + 0x00 + VECTOR_TABLE_BASE)

                                                 /* ARM exception handlers addresses                   */
#define  OS_CPU_ARM_EXCEPT_RESET_HANDLER_ADDR           (OS_CPU_ARM_EXCEPT_RESET          * 0x04 + 0x20 + VECTOR_TABLE_BASE)
#define  OS_CPU_ARM_EXCEPT_UNDEF_INSTR_HANDLER_ADDR     (OS_CPU_ARM_EXCEPT_UNDEF_INSTR    * 0x04 + 0x20 + VECTOR_TABLE_BASE)
#define  OS_CPU_ARM_EXCEPT_SWI_HANDLER_ADDR             (OS_CPU_ARM_EXCEPT_SWI            * 0x04 + 0x20 + VECTOR_TABLE_BASE)
#define  OS_CPU_ARM_EXCEPT_PREFETCH_ABORT_HANDLER_ADDR  (OS_CPU_ARM_EXCEPT_PREFETCH_ABORT * 0x04 + 0x20 + VECTOR_TABLE_BASE)
#define  OS_CPU_ARM_EXCEPT_DATA_ABORT_HANDLER_ADDR      (OS_CPU_ARM_EXCEPT_DATA_ABORT     * 0x04 + 0x20 + VECTOR_TABLE_BASE)
#define  OS_CPU_ARM_EXCEPT_ADDR_ABORT_HANDLER_ADDR      (OS_CPU_ARM_EXCEPT_ADDR_ABORT     * 0x04 + 0x20 + VECTOR_TABLE_BASE)
#define  OS_CPU_ARM_EXCEPT_IRQ_HANDLER_ADDR             (OS_CPU_ARM_EXCEPT_IRQ            * 0x04 + 0x20 + VECTOR_TABLE_BASE)
#define  OS_CPU_ARM_EXCEPT_FIQ_HANDLER_ADDR             (OS_CPU_ARM_EXCEPT_FIQ            * 0x04 + 0x20 + VECTOR_TABLE_BASE)

                                                 /* ARM "Jump To Self" asm instruction                 */
#define  OS_CPU_ARM_INSTR_JUMP_TO_SELF                   0xEAFFFFFE
                                                 /* ARM "Jump To Exception Handler" asm instruction    */
#define  OS_CPU_ARM_INSTR_JUMP_TO_HANDLER                0xE59FF018

//typedef unsigned int   INT32U;                   /* Unsigned 32 bit quantity                           */

void       OS_CPU_ARM_ExceptUndefInstrHndlr   (void);
void       OS_CPU_ARM_ExceptSwiHndlr          (void);
void       OS_CPU_ARM_ExceptPrefetchAbortHndlr(void);
void       OS_CPU_ARM_ExceptDataAbortHndlr    (void);
void       OS_CPU_ARM_ExceptAddrAbortHndlr    (void);
void       OS_CPU_ARM_ExceptIrqHndlr          (void);
void       OS_CPU_ARM_ExceptFiqHndlr          (void);

extern void Undefined_Handler(void);
extern void SWI_Handler(void);
extern void Prefetch_Handler(void);
extern void Abort_Handler(void);
extern void IRQ_Handler(void);
extern void FIQ_Handler(void);

extern void ExceptionUND(void);		// Undefine
extern void ExceptionPAB(void);		// Prefetch_Handler	
extern void ExceptionDAB(void);		// Abort_Handler
//extern void ExceptionREV(void);		// Reserved_Addr

void  OS_CPU_InitExceptVect (void)
{
    (*(INT32U *)OS_CPU_ARM_EXCEPT_UNDEF_INSTR_VECT_ADDR)       =         OS_CPU_ARM_INSTR_JUMP_TO_HANDLER;
    //(*(INT32U *)OS_CPU_ARM_EXCEPT_UNDEF_INSTR_HANDLER_ADDR)    = (INT32U)Undefined_Handler;
	 (*(INT32U *)OS_CPU_ARM_EXCEPT_UNDEF_INSTR_HANDLER_ADDR)    = (INT32U)ExceptionUND;

    (*(INT32U *)OS_CPU_ARM_EXCEPT_SWI_VECT_ADDR)               =         OS_CPU_ARM_INSTR_JUMP_TO_HANDLER;
    (*(INT32U *)OS_CPU_ARM_EXCEPT_SWI_HANDLER_ADDR)            = (INT32U)SWI_Handler;

    (*(INT32U *)OS_CPU_ARM_EXCEPT_PREFETCH_ABORT_VECT_ADDR)    =         OS_CPU_ARM_INSTR_JUMP_TO_HANDLER;
    //(*(INT32U *)OS_CPU_ARM_EXCEPT_PREFETCH_ABORT_HANDLER_ADDR) = (INT32U)Prefetch_Handler;
	 (*(INT32U *)OS_CPU_ARM_EXCEPT_PREFETCH_ABORT_HANDLER_ADDR) = (INT32U)ExceptionPAB;

    (*(INT32U *)OS_CPU_ARM_EXCEPT_DATA_ABORT_VECT_ADDR)        =         OS_CPU_ARM_INSTR_JUMP_TO_HANDLER;
    //(*(INT32U *)OS_CPU_ARM_EXCEPT_DATA_ABORT_HANDLER_ADDR)     = (INT32U)Abort_Handler;
	 (*(INT32U *)OS_CPU_ARM_EXCEPT_DATA_ABORT_HANDLER_ADDR)     = (INT32U)ExceptionDAB;

    (*(INT32U *)OS_CPU_ARM_EXCEPT_ADDR_ABORT_VECT_ADDR)        =         OS_CPU_ARM_INSTR_JUMP_TO_HANDLER;
    //(*(INT32U *)OS_CPU_ARM_EXCEPT_ADDR_ABORT_HANDLER_ADDR)     = (INT32U)Abort_Handler;
	 (*(INT32U *)OS_CPU_ARM_EXCEPT_ADDR_ABORT_HANDLER_ADDR)     = (INT32U)ExceptionDAB;

    (*(INT32U *)OS_CPU_ARM_EXCEPT_IRQ_VECT_ADDR)               =         OS_CPU_ARM_INSTR_JUMP_TO_HANDLER;
    (*(INT32U *)OS_CPU_ARM_EXCEPT_IRQ_HANDLER_ADDR)            = (INT32U)IRQ_Handler;

    (*(INT32U *)OS_CPU_ARM_EXCEPT_FIQ_VECT_ADDR)               =         OS_CPU_ARM_INSTR_JUMP_TO_HANDLER;
    (*(INT32U *)OS_CPU_ARM_EXCEPT_FIQ_HANDLER_ADDR)            = (INT32U)FIQ_Handler;
	 
	 __ISB();
}
/*********************************************************************
*
*       low_level_init()
*
* Function description
*   Usually this function name is __low_level_init() and is called from startup code.
*   Here however it is called from MX27_startup.s before the stack pointers init.
*   This function uses temp stack in VRAM
*   Used to initialize AIPI, PLL and SDRAM controller as early as possible
*/
OS_INTERWORK int low_level_init(void);       /* Avoid "No prototype" warning */
OS_INTERWORK int low_level_init(void)
{
	uart0_init (115200);
	
	//printf ("low_level_init\n");

	MMU_Init ();
	
	OS_CPU_InitExceptVect ();
	
	return 1;                      // Always initialize segments !
}



/*********************************************************************
*
*       OS_InitHW()
*
*       Initialize the hardware (timer) required for the OS to run.
*       May be modified, if an other timer should be used
*/

void OS_InitHW(void)
{
	// 关中断
	OS_DI();
	
	xm_do_change_task ();
	
	_Init_AITC();                   // Initialize VIC, clear and disable all interrupts
		
	// PLL、CORE时钟初始化
	timer_init ();
	
	// 定义1KHz的系统TICK时钟
	OS_TICK_Config(1, 1);	// 1KHz

	//
	// Initialize OS timer, clock soure = core clock
	//
	// OS定时器初始化代码
	StarSysTimer ();
	
	// RTC初始化
	XM_InitRTC ();
	
	// 软件定时器服务初始化(类似硬件定时器)，可以在定时器回调函数中使用所有系统服务。
	XMSYS_HardTimerInit ();
	
	// DMA初始化
	dma_init ();
	
	// GPIO
	InitGPIO ();
	
	xm_adc_init ();
	
	xm_powerkey_init ();//dc_5v
	
	xm_uart_init ();
	
	// I2C初始化
#if _XMSYS_I2C_ == _XMSYS_I2C_HARDWARE_
	xm_i2c_init ();
#endif
	
#ifdef _XMSYS_FS_NANDFLASH_SUPPORT_	// NANDFLASH文件系统支持
	NandInit(NandChip1);
#endif	
	
	init_SPI();
	
	void arkn141_lcdc_init (void);
	arkn141_lcdc_init ();
	

	/*
#ifdef _XMSYS_FS_SDMMC_SUPPORT_		// SDMMC卡文件系统支持
#if _XMSYS_FS_SDMMC_TYPE_	== _XMSYS_FS_SDMMC_TYPE_ARKMICRO_
	SDCard_Module_Init();
#endif
	
#endif
	*/
	
	//
	// Install Systick Timer Handler and enable timer interrupt
	//
  	OS_ARM_InstallISRHandler(TIMER0_INT, (OS_ISR_HANDLER*)OS_Systick);
	// 设置中断优先级
	//OS_ARM_ISRSetPrio(TIMER0_INT, _OS_ISR_TIMER_PRIO);
	// 使能中断
	OS_ARM_EnableISRSource (TIMER0_INT);	
		
	// 允许中断
	//OS_RestoreI();
}




/*********************************************************************
*
*       OS interrupt handler and ISR specific functions
*
**********************************************************************
*/
static void _OS_ISR_Undefined_0(void)
{
	//printf ("ISR 0\n");
//	while(1);
//	MGC_AfsUdsIsr_usb_0 ();
}

/*********************************************************************
*
*       _OS_ISR_Undefined
*
*       Is called when an uninstalled interrupt was detected
*       As interrupt pending condition of peripherals has to be reset,
*       program will not continue when interrupt is ignored.
*/
//static unsigned int irq_no;
static void _OS_ISR_Undefined(void)
{
	//printf ("ISR_Undefined %d\n", irq_no);
	printf ("ISR_Undefined\n");
}


/*********************************************************************
*
*       OS_ARM_InstallISRHandler
*/
OS_ISR_HANDLER* OS_ARM_InstallISRHandler (int ISRIndex, OS_ISR_HANDLER* pISRHandler)
{
	OS_ISR_HANDLER *old_handler = NULL;
	if(ISRIndex >= _NUM_INT_VECTORS)
	{
		printf ("illegal OS_ARM_InstallISRHandler, ISRIndex=%d, OS_ISR_HANDLER=%x\n", 
				  ISRIndex, pISRHandler);
		return old_handler;
	}
	OS_IncDI();
	old_handler = itc_handler[ISRIndex];
	itc_handler[ISRIndex] = pISRHandler;
	if(pISRHandler == NULL)
	{
		// 关闭中断	
		irq_disable (ISRIndex);
	}
	__DSB();
	OS_DecRI();
	return pISRHandler;
}



/*********************************************************************
*
*       OS_ARM_ISRSetPrio
*/
int OS_ARM_ISRSetPrio(int ISRIndex, int Prio)
{
	return 0;
}


/*****  EOF  ********************************************************/

int request_irq (unsigned char irq, unsigned char priority, pfnIRQ handler)
{
	if(OS_ARM_InstallISRHandler (irq, handler))
	{
		irq_enable (irq);
	}
	return 0;
}

void busy_delay(INT32U time)
{
	while(time--);
}

DWORD XM_GetTickCount (void)
{
	return OS_GetTime();
}


void XM_Sleep (DWORD ms)
{
	OS_Delay (ms);
}

// 关中断
void XM_lock (void)
{
	OS_IncDI();
}

// 开中断
void XM_unlock (void)
{
	OS_DecRI();
}

#include <stdarg.h>
#define	XM_PRINTF_SIZE		2048
void XM_printf (char *fmt, ...)
{
	static char xm_info[XM_PRINTF_SIZE + 4];
	
	va_list ap;
	xm_info[XM_PRINTF_SIZE] = 0;
	va_start(ap, fmt);
	//vsprintf (xm_info, fmt, ap);
	vsnprintf (xm_info, XM_PRINTF_SIZE, fmt, ap);
	va_end(ap);
		
#if defined(HEAP_DEBUG)	|| defined(CORE_HEAP_DEBUG)
	
#else
	printf ("%s", xm_info);
#endif
}