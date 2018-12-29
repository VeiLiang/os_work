#include <xm_proj_define.h>
#include "RTOS.H"
#include "types.h"
#include "common.h"
#include "hardware.h"
#include "irqs.h"
#include "rtc.h"

extern unsigned char XM_TimerEventProc (void);


irq_handler_t itc_handler[_NUM_INT_VECTORS];


/*********************************************************************
*
*       OS_Systick
*
* Function description
*   This is the code that gets called when the processor receives a
*   _SysTick exception. SysTick is used as OS timer tick.
*
* NOTES:
*   (1) It has to be inserted in the interrupt vector table, if RAM
*       vectors are not used. Therefore is is declared public
*/
void OS_Systick(void)
{
	/*
	static int count = 0;
	count ++;
	if((count % 2000) == 0)
		printf ("count=%d\n", count);
	*/
	// 清中断
	rTIMER_CTL0 &= ~0x10;
	   
	// 定义1000Hz的系统TICK时钟
	OS_HandleTickEx();

#ifdef _XMSYS_APP_SUPPORT_
	// UI时钟触发
	XM_TimerEventProc ();
#endif

	// 软件RTC, 仅在RTC损坏或者未安装的情况下应用
	XM_TriggerRTC ();
	
	extern void backlight_check_auto_off (void);
	backlight_check_auto_off ();
#if 0//def DEADLINE_TICKET_ENABLE	
	extern void XMSYS_SensorCheckDeadLineTicket (int channel);
	XMSYS_SensorCheckDeadLineTicket (0);
#endif
}


void delay(unsigned int time)
{
	volatile int i;
	for(i=0; i < time; i++);
}

/*********************************************************************
*
*       Idle loop  (OS_Idle)
*
*       Please note:
*       This is basically the "core" of the idle loop.
*       This core loop can be changed, but:
*       The idle loop does not have a stack of its own, therefore no
*       functionality should be implemented that relies on the stack
*       to be preserved. However, a simple program loop can be programmed
*       (like toggeling an output or incrementing a counter)
*/
void OS_Idle(void) {     // Idle loop: No task is ready to execute
	
	while (1)
	{
		// Entry into Standby mode is performed by executing the Wait For Interrupt (WFI)
		// instruction. To ensure that the entry into the standby mode does not affect the
		// memory system, the WFI instruction automatically performs a Data
		// Synchronization Barrier (DSB) operation. This ensures that all explicit memory
		// accesses occur in program order before the WFI instruction has completed.		
		__asm ("wfi");		// Wait For Interrupt
	}
}



/*********************************************************************
*
*       OS interrupt handler and ISR specific functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _OS_ISR_Undefined
*
*       Is called when an uninstalled interrupt was detected
*       As interrupt pending condition of peripherals has to be reset,
*       program will not continue when interrupt is ignored.
*/
void _OS_ISR_Undefined(void)
{
	printf ("ISR_Undefined\n");
}

/*********************************************************************
*
*       OS_irq_handler
*
*       Detect reason for IRQ and call correspondig service routine.
*       OS_irq_handler is called from OS_IRQ_SERVICE function
*       found in RTOSVect.asm
*/
extern unsigned int rtos_get_irq (void);
extern void rtos_ack_irq (unsigned int pend);

OS_INTERWORK void OS_irq_handler(void)
{
	//static volatile unsigned int pend;	
	//pend = rIVEC_ADDR >> 2;
	//__asm ("mcr	p15, 0, r0, c7, c10, 5	");
	unsigned int pend;// = -1;	
#if 1	
	//pend = rIVEC_ADDR >> 2;
	//__DSB();
	pend = rIVEC_ADDR >> 2;
	//__DSB();
#else	

	int loop = 2;
	//unsigned int old_mask = rICMASK;
	// 屏蔽所有中断，防止因为其他高优先级中断竞争导致中断源向量读取出现0的情况
	//rICMASK = 0xFFFFFFFF;
	while(loop > 0)
	{
		pend = rIVEC_ADDR >> 2;
		//pend = rtos_get_irq ();
		if(pend)
		{
			//if(loop != 10)
			//	printf ("pend=%d, loop=%d\n", pend, 10 - loop);
			break;
		}
		else
		{
			// LCD (bit 0)
			if(rICPEND & (1 << LCD_INT))
			{
				if(rICMASK & (1 << LCD_INT))
				{
					// LCD中断屏蔽
					rIRQISPC = 1 << 0;
				}
				else
				{
					break;
				}
			}
			//printf ("pend=%d, rICPEND=%x\n", pend, rICPEND);
		}
		loop --;
	}
	
	//if(pend == (unsigned int)(-1))
	if(loop == 0)
	{
		printf ("no irq\n");
		return;
	}
	
	//pend = pend >> 2;
#endif
	
	//irq_maskack(pend);
	//rIRQISPC = 1 << pend;
	//__asm ("mcr	p15, 0, r0, c7, c10, 5	");

	//rtos_ack_irq (1 << pend);
	
	if(pend >= _NUM_INT_VECTORS)
	{
		//printf ("itc wrong number(%d)\n", pend);
		return;
	}
		
	OS_EnterInterrupt ();
	
	// 检查中断服务函数是否非空
	if(itc_handler[pend])
	{
		(*itc_handler[pend])();
	}
	
	rIRQISPC = 1 << pend;
	__DSB();
	
	OS_DI();                                   // Disable interrupts and unlock
	OS_LeaveInterrupt ();	
	//rtos_ack_irq (1 << pend);
	
	// 恢复中断屏蔽状态
	//rICMASK = old_mask;
}

/*********************************************************************
*
*       OS_ARM_EnableISRSource
*
*/
void OS_ARM_EnableISRSource(int SourceIndex)
{
	if(SourceIndex >= _NUM_INT_VECTORS)
		return;
	rICMASK &= (~(1 << SourceIndex));
}

void OS_ARM_DisableISRSource(int SourceIndex)
{
	if(SourceIndex >= _NUM_INT_VECTORS)
		return;
	rICMASK |= (1 << SourceIndex);
}

// 中断保护
void irq_disable (unsigned char irq)
{
	if(irq >= _NUM_INT_VECTORS)
		return;
	OS_IncDI();
	rICMASK |= (1 << irq);
	OS_DecRI();
}

// 中断保护
void irq_enable(unsigned char irq)
{
	if(irq >= _NUM_INT_VECTORS)
		return;
	OS_IncDI();
	rICMASK &= (~(1 << irq));
	OS_DecRI();
}