#include "hardware.h"
#include <stdio.h>
#include <stdlib.h>
#include <xm_base.h>
#include "timer.h"
#include "rtos.h"
#include "target_ucos_ii.h"
#include "cpuClkCounter.h"

// ��ʱ�����û��δ�ص�����
static pfnIRQUSER timer_user_tick_callback[XM_TIMER_COUNT];
static void*  timer_user_private_data[XM_TIMER_COUNT];


int timer_init (void)
{
   rSYS_SOFT_RSTNB &= ~(1<<1);
   __asm ("nop");
   __asm ("nop");
   rSYS_SOFT_RSTNB |= (1<<1);
   __asm ("nop");
   __asm ("nop");
	// ASIC
	ShowHWFreqInfo ();
		
	memset (timer_user_tick_callback, 0, sizeof(timer_user_tick_callback));
	memset (timer_user_private_data, 0, sizeof(timer_user_private_data));

	return 0;
}



// ����1KHz��ϵͳ��ʱ��
void StarSysTimer(void)
{
	unsigned int clk;
	// 1KHZ
	clk = arkn141_get_clks(ARKN141_CLK_APB) / 1000000;
	rTIMER_PRS0 = clk - 1;	// ��������Ϊ΢��
	//rTIMER_PRS0 = 24-1;
	rTIMER_CNT0 = 0;
	rTIMER_MOD0 = 1000-1;	// ÿ����(1000)�����ж�
	rTIMER_CTL0 = 0x0F;
	//rTIMER_CTL0 |= 0x08;		// ����Ϊ��ƽ�жϴ�����ʽ
	//rTIMER_CTL0 &= ~0x10;		// �����ƽ�ж�
	EnableIntNum (TIMER0_INT);	
}

// ��ȡ����������߾��ȵĵδ��������΢��Ϊ������λ
XMINT64 XM_GetHighResolutionTickCount (void)
{
	int64_t ticket;
	u32_t ms, us1, us2;
	int timer_factor = 0;	// ��ʱ���ж�
	OS_IncDI();
	us1 = 999 - rTIMER_CNT0;
	ms = OS_GetTime();
	timer_factor = rICPEND & (1 << TIMER0_INT);
	us2 = 999 - rTIMER_CNT0;
	OS_DecRI();	
		
	if(us1 > us2 || timer_factor)		// �����������˷�ת
		ms ++;
	ticket = ms;
	ticket *= 1000;
	ticket += us2;
	return ticket;
}

// ΢�뼶��ʱ
void udelay (unsigned long usec)
{
	XMINT64 now_ticket = XM_GetHighResolutionTickCount ();
	XMINT64 end_ticket = now_ticket + usec;
	while(now_ticket < end_ticket)
	{
		now_ticket = XM_GetHighResolutionTickCount ();
	}
}

// timer1/2/3/4/5�жϺϲ���һ���Ӧ�ж�TIMER12345_INT�����ж�TIMER12345_INT��������Ҫ��intr_status_reg��ѯ�ж�Դ��
void timer_12345_intr_hander (void)
{
	UINT32 stat = rTIMER_INT_STATUS;
	//printf ("timer_12345 intr, 0x%x\n", stat);
		
	if( (stat & (1 << 2)) )	// Timer 1�ж�
	{
		rTIMER_CTL1 &= ~0x10;
		if(timer_user_tick_callback[1])
			(*timer_user_tick_callback[1])(timer_user_private_data[1]);
	}
	if( (stat & (1 << 3)) )	// Timer 2�ж�
	{
		rTIMER_CTL2 &= ~0x10;
		if(timer_user_tick_callback[2])
			(*timer_user_tick_callback[2])(timer_user_private_data[2]);
	}
	if( (stat & (1 << 4)) )	// time3|time4|time5
	{
		if( (stat & (1 << 0)) )	// Timer 3�ж�
		{
			rTIMER_CTL3 &= ~0x10;
			if(timer_user_tick_callback[3])
				(*timer_user_tick_callback[3])(timer_user_private_data[3]);
		}
		if( (stat & (1 << 5)) )	// Timer 4�ж�
		{
			rTIMER_CTL4 &= ~0x10;
			if(timer_user_tick_callback[4])
				(*timer_user_tick_callback[4])(timer_user_private_data[4]);
		}
		if( (stat & (1 << 6)) )	// Timer 5�ж�
		{
			rTIMER_CTL5 &= ~0x10;
			if(timer_user_tick_callback[5])
				(*timer_user_tick_callback[5])(timer_user_private_data[5]);
		}
	}
	
	// ���ж�
	rTIMER_INT_STATUS = stat;
}

// timer		��ʱ�����
// ticks_per_second		ÿ��δ���
// user_tick_callback	�û��δ�ص�����
// user_private_data    �û�˽������
int timer_x_start (unsigned int timer, UINT ticks_per_second, pfnIRQUSER user_tick_callback, void* user_private_data)
{
	unsigned int clk;
	
	if(timer == 0 || timer >= XM_TIMER_COUNT)
	{
		printf ("timer (%d), illegal id\n", timer);
		return -1;
	}
	if(ticks_per_second == 0 || ticks_per_second > 1000000)
	{
		printf ("timer (%d), illegal ticks_per_second (%d)\n", timer, ticks_per_second);
		return (-1);
	}
	
	// ��ֹ��������
	OS_IncDI();		// ���ж�
	timer_user_tick_callback[timer] = user_tick_callback;
	timer_user_private_data[timer] = user_private_data;
	OS_DecRI();		
		
	request_irq (TIMER12345_INT, 0, timer_12345_intr_hander);
	clk = arkn141_get_clks(ARKN141_CLK_APB);
	if(timer == XM_TIMER_1)
	{
		rTIMER_INT_STATUS = (1 << 2);
		rTIMER_CNT1 = 0;
		rTIMER_PRS1 = (clk/1000000) - 1;
		rTIMER_MOD1 = 1000000 / ticks_per_second - 1;
		rTIMER_CTL1 = 0x0F;
	//	rTIMER_CTL1 |= 0x08;		// ����Ϊ��ƽ�жϴ�����ʽ
	//	rTIMER_CTL1 &= ~0x10;		// �����ƽ�ж�
	}
	else if(timer == XM_TIMER_2)
	{
		rTIMER_INT_STATUS = (1 << 3);
		rTIMER_CNT2 = 0;
		rTIMER_PRS2 = (clk/1000000) - 1;
		rTIMER_MOD2 = 1000000 / ticks_per_second - 1;
		rTIMER_CTL2 = 0x0F;
	//	rTIMER_CTL2 |= 0x08;		// ����Ϊ��ƽ�жϴ�����ʽ
	//	rTIMER_CTL2 &= ~0x10;		// �����ƽ�ж�
	}
	else if(timer == XM_TIMER_3)
	{
		rTIMER_INT_STATUS = (1 << 0);
		rTIMER_CNT3 = 0;
		rTIMER_PRS3 = (clk/1000000) - 1;
		rTIMER_MOD3 = 1000000 / ticks_per_second - 1;
		rTIMER_CTL3 = 0x0F;
	//	rTIMER_CTL3 |= 0x08;		// ����Ϊ��ƽ�жϴ�����ʽ
	//	rTIMER_CTL3 &= ~0x10;		// �����ƽ�ж�
	}
	else if(timer == XM_TIMER_4)
	{
		rTIMER_INT_STATUS = (1 << 5);
		rTIMER_CNT4 = 0;
		rTIMER_PRS4 = (clk/1000000) - 1;
		rTIMER_MOD4 = 1000000 / ticks_per_second - 1;
		rTIMER_CTL4 = 0x0F;
	//	rTIMER_CTL4 |= 0x08;		// ����Ϊ��ƽ�жϴ�����ʽ
	//	rTIMER_CTL4 &= ~0x10;		// �����ƽ�ж�
	}
	else if(timer == XM_TIMER_5)
	{
		rTIMER_INT_STATUS = (1 << 6);
		rTIMER_CNT5 = 0;
		rTIMER_PRS5 = (clk/1000000) - 1;
		rTIMER_MOD5 = 1000000 / ticks_per_second - 1;
		rTIMER_CTL5 = 0x0F;
	//	rTIMER_CTL5 |= 0x08;		// ����Ϊ��ƽ�жϴ�����ʽ
	//	rTIMER_CTL5 &= ~0x10;		// �����ƽ�ж�
	}
	
	return 0;	
}	

int timer_x_stop (unsigned int timer)
{
	if(timer == 0 || timer >= XM_TIMER_COUNT)
	{
		printf ("timer (%d), illegal id\n", timer);
		return -1;
	}
	
	if(timer == 1)
		rTIMER_CTL1 = 0;
	else if(timer == 2)
		rTIMER_CTL2 = 0;
	else if(timer == 3)
		rTIMER_CTL3 = 0;
	else if(timer == 4)
		rTIMER_CTL4 = 0;
	else if(timer == 5)
		rTIMER_CTL5 = 0;
		
	// ��ֹ��������
	OS_IncDI();
	timer_user_tick_callback[timer] = NULL;
	OS_DecRI();
	
	return 0;
}


static int timer_1_loop = 0;
void timer_1_intr_handler (void *private_data)
{
	
	printf ("timer 1 100Hz, %d, t=%d\n", timer_1_loop, XM_GetTickCount());	
	timer_1_loop ++;
	if(timer_1_loop == 100)
	{
		timer_x_stop (1);
	}
}

static int timer_2_loop = 0;
void user_ticket_timer_2_intr_handler (void *private_data)
{
	printf ("timer 2 10Hz, %d, t=%d\n", timer_2_loop, XM_GetTickCount());	
	timer_2_loop ++;
	if(timer_2_loop == 100)
	{
		timer_x_stop (2);
	}
}

static int timer_3_loop = 0;
void user_ticket_timer_3_intr_handler (void *private_data)
{
	printf ("timer 3 1Hz, %d, t=%d\n", timer_3_loop, XM_GetTickCount());	
	timer_3_loop ++;
	if(timer_3_loop == 10)
	{
		timer_x_stop (3);
	}
}

static int timer_4_loop = 0;
void user_ticket_timer_4_intr_handler (void *private_data)
{
	printf ("timer 4 2Hz, %d, t=%d\n", timer_4_loop, XM_GetTickCount());	
	timer_4_loop ++;
	if(timer_4_loop == 20)
	{
		timer_x_stop (4);
	}
}

static int timer_5_loop = 0;
void user_ticket_timer_5_intr_handler (void *private_data)
{
	printf ("timer 5 5Hz, %d, t=%d\n", timer_5_loop, XM_GetTickCount());	
	timer_5_loop ++;
	if(timer_5_loop == 50)
	{
		timer_x_stop (5);
	}
}

void timer_1_oneticket_handler (void *private_data)
{
	
	printf ("timer 1 10000Hz\n");	
	//timer_1_loop ++;
	//if(timer_1_loop == 100)
	{
		timer_x_stop (1);
	}
}

void xm_timer_test (void)
{
	printf ("xm_timer_test\n");
	int times = 0;
	while(times < 10)
	{
		timer_x_start (1, 10000, timer_1_oneticket_handler, 0);
		OS_Delay (10);
		times ++;
		printf ("times = %d\n", times);
	}
	
	printf ("timer 1, 100 Hz, 1s\n");
	timer_x_start (1, 100, timer_1_intr_handler, 0);
	OS_Delay (2000);
	
	printf ("timer 2, 10 Hz, 10s\n");
	timer_x_start (2, 10, user_ticket_timer_2_intr_handler, 0);
	OS_Delay (12000);
	
	printf ("timer 3, 1 Hz, 10s\n");
	timer_x_start (3, 1, user_ticket_timer_3_intr_handler, 0);
	OS_Delay (12000);
	
	printf ("timer 4, 2 Hz, 10s\n");
	timer_x_start (4, 2, user_ticket_timer_4_intr_handler, 0);
	OS_Delay (12000);	

	printf ("timer 4, 5 Hz, 10s\n");
	timer_x_start (5, 5, user_ticket_timer_5_intr_handler, 0);
	OS_Delay (12000);
}