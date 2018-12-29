#include <stdio.h>
#include "RTOS.h"		// OSͷ�ļ�
#include <assert.h>

extern void RTOS_OS_DoRR (void);
extern void OS_MakeTaskReady (OS_TASK *pTask);
extern void RTOS_CheckDelays (OS_GLOBAL *OS_global);

OS_GLOBAL *RTOS_ChangeTask (OS_GLOBAL *OS_global);
void _ChangeTask_jump_ (void);

extern OS_TASK TCB_Isp;                        /* Task-control-blocks */


void _ChangeTask_jump (void)
{
	asm ("ldr	r1,	lable\n");
	asm ("b 		lable2\n");
	asm ("lable: DCD	 RTOS_ChangeTask\n");
	asm ("lable2: bx 	r1\n");
	//asm ("bx 	r1\n");
	//asm ("blx 	r1\n");
	//asm ("ldr  	pc,	RTOS_ChangeTask\n");
}

void _ChangeTask_jump_ (void)
{
	//asm ("mov	lr,	r2\n");
	asm ("ldr  	pc,	RTOS_ChangeTask\n");
}

OS_GLOBAL *RTOS_ChangeTask (OS_GLOBAL *OS_global)
{
	unsigned char *addr;
	OS_TASK *pTask;
	OS_TASK *pNext;
	OS_WAIT_LIST *pWaitList;
	OS_RSEMA *pResm;
	OS_Global.Counters.Cnt.DI = 0;
	if(OS_Global.Counters.Cnt.Region != 0)
		goto OS_ChangeTask_1;
	
	OS_Error (118);

OS_ChangeTask_1:
	// ���ϵͳջ��������, ���ջ�ײ���ֵΪ0xcD
	addr = (unsigned char *)OS_GetSysStackBase ();
	if(*addr != 0xcd)
	{
		OS_Error (125);
	}
	
OS_ChangeTask_2:
	// ���Intջ��������
	addr = (unsigned char *)OS_GetIntStackBase ();
	if(*addr != 0xcd)
	{
		OS_Error (126);
	}

OS_ChangeTask_3:
	// ���浱ǰ�����״̬ (pExtendContext )

OS_ChangeTask_4:
	if(OS_Global.TimeSliceAtStart == 0)
		goto OS_ChangeTask_6;

	// ���浱ǰ����δ�����ʱ��Ƭ��Ϣ 
	OS_Global.pCurrentTask->TimeSliceRem = OS_Global.TimeSlice;

	// ����Ƿ������ѯ����
	if(OS_Global.Pending.Flag.RoundRobin == 0)
		goto OS_ChangeTask_6;
	// ִ����ѯ����
	RTOS_OS_DoRR ();

OS_ChangeTask_6:
	// ͳ�Ƶ�ǰ���� pCurrentTask ����ռ�Ĵ���
	if(OS_Global.pCurrentTask == 0)
		goto OS_ChangeTask_7;
	if(OS_Global.pCurrentTask->Stat == 0)
	{
		OS_Global.pCurrentTask->NumPreemptions ++;
	}

OS_ChangeTask_7:
	if(OS_Global.pActiveTask == 0)
		goto OS_ChangeTask_8;
	goto OS_ChangeTask_9;

OS_ChangeTask_loop: 
	pNext = OS_Global.pActiveTask->pNext;
	OS_Global.pActiveTask = pNext;

	// �����ж�, Supervisorģʽ
	asm ("MSR       CPSR_c, #19\n");

OS_ChangeTask_9:
	// ��һ���������� pActiveTask ����

	// ��ֹ�ж�, �л���Supervisorģʽ
	asm ("MSR       CPSR_c, #147\n");
	// ;  ��鼤������ pActiveTask�Ƿ����. �������, �жϼ��������Statλ
	if(OS_Global.pActiveTask)
	{
		if( OS_Global.pActiveTask->Stat  == 0)
			goto OS_ChangeTask_11;
	}
	else
		goto OS_ChangeTask_11;

	// ; ���������Stat��Ϊ0
	// ; ���Stat��bit2�Ƿ�Ϊ0, ���Ƿ�ȴ���ʱ
	if( (OS_Global.pActiveTask->Stat & 0x04) == 0 )
		goto OS_ChangeTask_12;

	// Stat��bit2��Ϊ0, ����ȴ�ĳ����ʱʱ�� OS_Delay 
   	// �Ƚ�����ĳ�ʱʱ�� Timeout �Ƿ�С�ڻ���� ��ǰϵͳʱ�� (OS_Global.Time).
	if( (OS_Global.pActiveTask->Timeout - OS_Global.Time) >= 1 )
		goto OS_ChangeTask_13;		// δ��ʱ

	// ; �ѳ�ʱ, ִ�� OS_MakeTaskReady (OS_Global.pActiveTask)����������Ϊready
	OS_MakeTaskReady ((OS_TASK *)OS_Global.pActiveTask);
	
	if(OS_Global.pActiveTask->Stat != 0)
		goto OS_ChangeTask_12;
	// ; 	���������StatΪ0, ��ready״̬	
	goto OS_ChangeTask_8;

OS_ChangeTask_13: 
	if( (OS_Global.pActiveTask->Timeout - OS_Global.TimeDex) < 0)
		OS_Global.TimeDex = OS_Global.pActiveTask->Timeout;

OS_ChangeTask_12: 
	// ;	��������Ƿ�ȴ�����(ͨ��pWaitList����)  (OS_RSEMA)
	if( (OS_Global.pActiveTask->Stat & 0xFB) != 0x10) 
		goto OS_ChangeTask_loop;

	// ; ����ͨ��pWaitList�ȴ������¼����� (OS_RSEMA)
	pWaitList = OS_Global.pActiveTask->pWaitList;
	pResm = (OS_RSEMA *)OS_Global.pActiveTask->pWaitList->pWaitObj;
	pTask = pResm->pTask;
	if(pTask->Stat != 0)
		goto OS_ChangeTask_loop;

OS_ChangeTask_8:
	asm ("MSR       CPSR_c, #147\n");		// ; ���ж�,�л���Supervisorģʽ

OS_ChangeTask_11: 
	// ; ����Ƿ�Preempion��Ϊ0
	if(OS_Global.Pending.Flag.Preemption != 0)
		goto OS_ChangeTask_20;

OS_ChangeTask_task_40:
	if( (OS_TickStep & 1) == 0)
		goto OS_ChangeTask_21;
	
	assert (0);
	

OS_ChangeTask_20:
	// ; ѭ��ֱ�� OS_Global.Preemption == 0
	RTOS_CheckDelays (&OS_Global);
	if(OS_Global.Pending.Flag.Preemption != 0)
		goto OS_ChangeTask_20;

	goto OS_ChangeTask_task_40;

 OS_ChangeTask_21: 
	OS_Global.Pending.Flag.TaskSwitch = 0;
	if(OS_Global.pActiveTask == NULL)
		goto OS_ChangeTask_22;
	if(OS_Global.pActiveTask->Stat != 0)
	{
		OS_RSEMA *pResm = (OS_RSEMA *)OS_Global.pActiveTask->pWaitList->pWaitObj;
		pTask = pResm->pTask;
	}
	else
	{
		pTask = (OS_TASK *)OS_Global.pActiveTask;
	}

	// �������Bug, ��DCacheû�й�ϵ(����DCache�������ǽ�ֹ)
	// ISP������ĳ��״̬���޷���������, ��ʱISP������Ч, Ready, �����ȼ����.
	//
	// ;���õ�ǰ���� pCurrentTask	
	// ���ISP�����Ƿ���Ч, ready, �Ҳ��ǵ�ǰ����.
	// ������, ����TCB_IspΪ��ǰ����
	if(pTask != &TCB_Isp && TCB_Isp.Stat == 0)
	{
		if(OS_Global.pTask == &TCB_Isp)
		{
			static int count = 0;
			count ++;
			//printf ("\nisp task re-schedule %d\n\n", count);
			pTask = &TCB_Isp;
		}
	}
	OS_Global.pCurrentTask = pTask;
	if( OS_Global.pCurrentTask->Id == 0x64 )
		goto OS_ChangeTask_id_ok;

	OS_Error(128);

OS_ChangeTask_id_ok:
	// ; ���pCurrentTask��ջ, (ջ�ײ���ֵΪ0xcd)
	if(OS_Global.pCurrentTask->pStackBot[0] == 0xcd)
		goto OS_ChangeTask_task_stack_ok;

	OS_Error(120);

OS_ChangeTask_task_stack_ok: 
	// ���pCurrentTask��pExtendContext�Ƿ����(pExtendContextδʹ��, �̶�Ϊ0)

OS_ChangeTask_task_30:
	// ; ��pCurrentTask��ǰ�����pTLS����Ϊϵͳ��OS_pTLS

	// ����ǰ�����ʱ��Ƭ����
	OS_Global.TimeSlice = OS_Global.pCurrentTask->TimeSliceRem;
	OS_Global.TimeSliceAtStart = OS_Global.pCurrentTask->TimeSliceRem;

	// �ۼӵ�ǰ����Ļ�Ծ���� + 1
	OS_Global.pCurrentTask->NumActivations ++;
	goto OS_ChangeTask_exit;
	
 OS_ChangeTask_22: 
	 //if(TCB_Isp.Stat == 0 && OS_Global.pTask == &TCB_Isp)
	 {
		 // printf ("task error\n");
	 }
	// ; û���������, ����OS_Idle
	OS_Global.TimeSliceAtStart = 0;
	OS_Global.TimeSlice = 0;
	OS_Global.pCurrentTask = 0;
	OS_Global.Counters.Cnt.Region = 0;
	
	asm ("MSR       CPSR_c, #19 \n");		//						���ж�, Supervisor
	OS_Idle ();
	OS_Error (119);

 OS_ChangeTask_exit: 
	return &OS_Global;
}

void RTOS_OS_DoRR (void)
{
	OS_TASK *pTask, *pNext;
	//				; RoundRobin��ѯʽ�����㷨, ����OS_Global.RoundRobin == 1������µ���
	//				;
	//				; 	�㷨���� : ��ѯ����ѡ���뵱ǰ�������ȼ���ͬ����һ������Ԫ���е���, ������ǰ�������ӵ����һ���������ȼ���ͬ������ĺ���. (���ȼ���ͬ��ͬ������ѯ����)
	//				;
		//			; ���OS_Global.pCurrentTask�Ƿ����. 
	//				;	������, �������б���pCurrentTask����һ������Ԫ(pNext)��ʼ����, ���ҵ�һ��"���ȼ���pCurrentTask�����ȼ�����ͬ"������Ԫ
	asm ("MSR       CPSR_c, #147            ; 0x93								���ж�\n");
	
	// ; �����ѯʽ���ȱ�־	OS_Global.RoundRobin = 0
	OS_Global.Pending.Flag.RoundRobin = 0;
	
	asm ("MSR       CPSR_c, #19             ; 0x13								���ж�\n");
	
   //				; ��鵱ǰ�����Ƿ����  OS_Global.pCurrentTask != NULL, ��������, ����.
   //				; ����ǰ�������, ��鵱ǰ�������һ������(���������������ӵ�˳��)�Ƿ����. ��������, ����
	if(OS_Global.pCurrentTask != NULL)
	{
		pTask = (OS_TASK *)OS_Global.pCurrentTask->pNext;
		if(pTask == NULL)
			goto _OS_DoRR_exit;
	}
	else
		goto _OS_DoRR_exit;
	
	// ; �����һ������Ԫ(r2)�����ȼ��Ƿ��뵱ǰ������ͬ. ������ͬ, ����	
	if( OS_Global.pCurrentTask->Priority  != pTask->Priority)
		goto _OS_DoRR_exit;
	goto _OS_DoRR_1;	
	
  	// 				; ���������б�, ֱ���������һ������ΪNULL������һ����������ȼ��뵱ǰ��������ȼ���ͬ
   //				; ���������б��в������һ�����ȼ��뵱����������ȼ���ͬ������Ԫ, ����λ�ôӵ�ǰ�������һ������ʼ
_OS_DoRR_loop:   
	pTask = pNext;
	
_OS_DoRR_1:
	pNext = 	pTask->pNext;
	if(pNext == NULL)
		goto _OS_DoRR_2;
	if(pNext->Priority == OS_Global.pCurrentTask->Priority)
		goto _OS_DoRR_loop;
	
_OS_DoRR_2:    
//				; �����뵱ǰ����pCurrentTask���ȼ���ͬ������, 
//				; pTask(r2)ָ�������Ԫ�����ȼ���pCurrentTask�����ȼ���ͬ, �����һ����pCurrentTask�����ȼ���ͬ������Ԫ
	asm ("MSR       CPSR_c, #147            ; 0x93						���ж�\n");
	
	// ��ѯ����ѡ���뵱ǰ�������ȼ���ͬ����һ������Ԫ���е���
	if(OS_Global.pActiveTask == OS_Global.pCurrentTask)
		OS_Global.pActiveTask = OS_Global.pCurrentTask->pNext;
	
	// ; ����ǰ����Ԫ���ӵ����һ���������ȼ���ͬ������Ԫ�ĺ���.
	OS_Global.pCurrentTask->pPrev->pNext = OS_Global.pCurrentTask->pNext;
	OS_Global.pCurrentTask->pNext->pPrev = OS_Global.pCurrentTask->pPrev;
	OS_Global.pCurrentTask->pPrev = pTask;		// (pTask �����һ���������ȼ���ͬ������)
	OS_Global.pCurrentTask->pNext = pTask->pNext;
	if(pTask->pNext != NULL)
		pTask->pNext->pPrev = 	OS_Global.pCurrentTask;
	pTask->pNext = OS_Global.pCurrentTask;		
	
	pTask->TimeSliceRem = pTask->TimeSliceReload;
	
_OS_DoRR_exit:	
	return;
}

void RTOS_CheckDelays(OS_GLOBAL *OS_global)
{
	OS_TIMER *pTimer = NULL;
	OS_TIMER *pNext;
	OS_TASK *pTask = NULL;
	// ; ��ռʽ����
	// ; ��� ��ռʽ���ȱ�־ OS_Global.Preemption = 0
	OS_Global.Pending.Flag.Preemption = 0;
	
	// ; ������һ����ռ�ȵ��ȵ�ʱ�� OS_Global.TimeDex = OS_Global.Time + 4000;		// 4��
	OS_Global.TimeDex = OS_Global.Time + 4000;
	goto _CheckDelays_1;
	
 _CheckDelays_10:	
	// ; pTimer�ѳ�ʱ, ��pTimer��TIMER�������Ƴ�, ��Hook����, ����Hook����
	OS_Global.pCurrentTimer = pTimer; 
	OS_Global.pTimer = pTimer->pNext;
	pTimer->Active = 0;			// ��Timer��Ϊ����Ծ
	OS_Global.pCurrentTimer->Time = 0;
	
	asm ("MSR       CPSR_c, #19             ; 0x13					���ж�\n");
	
	OS_InTimer ++;
	pTimer  = OS_Global.pCurrentTimer;
	(*pTimer->Hook)();
	OS_InTimer --;
	
	asm ("MSR       CPSR_c, #147            ; 0x93					���ж�\n");
	
_CheckDelays_1: 
	//									; ����ÿ��ϵͳ��ʱ��, ����䳬ʱʱ���Ƿ����ϵͳʱ��
	//									; ���ϵͳ��ʱ�� OS_Global.pTimer �Ƿ�Ϊ��. 
	pTimer = OS_Global.pTimer;	
	if(pTimer == NULL)
		goto _CheckDelays_2;

	// ��鶨ʱ�� pTimer �Ƿ�ʱ(����ϵͳʱ��)
	// ���pTimer�Ƿ�ʱ (pTimer->Time  <= OS_Global.Time)
	if( (pTimer->Time - OS_Global.Time) < 1 )
		goto _CheckDelays_10;	
	
  	//								; ��ʱ���ĳ�ʱʱ����ڵ�ǰ��ϵͳʱ��
	// 									; ȡ����ʱ���ĳ�ʱʱ�̲�����Ϊ��һ�ε���ռʽ����ʱ��
	OS_Global.TimeDex = pTimer->Time;

_CheckDelays_2:  
	asm ("MSR       CPSR_c, #19             ; 0x13					CPSR_c = 0x13, ���ж�, supervisor\n");
	
	// ; ��TASK����ĵ�һ����Ԫ��ʼ�����������
	pTask = OS_Global.pTask;
	goto _CheckDelays_3;

_CheckDelays_loop:
	
	asm ("MSR       CPSR_c, #147            ; 0x93					CPSR_c = 0x93, ���ж�, supervisor	\n");
	
	// ; ����Ƿ���ڻ�Ծ������ OS_Global.pActiveTask
	if(OS_Global.pActiveTask == 0)
		goto _CheckDelays_4;
	
  //					; �Ƚ�pTask�����ȼ���OS_Global.pActiveTask�����ȼ�
  //					; ��pTask�����ȼ� < pActiveTask�����ȼ�, ��������, ����
	if(pTask->Priority < OS_Global.pActiveTask->Priority)
		goto _CheckDelays_exit;	// ��������, ����

_CheckDelays_4: 
	// ; pTask�����ȼ� >= pActiveTask�����ȼ�	
	// ; ���pTask�Ƿ�ȴ���ʱ (Stat��bit2Ϊ1)
	if( (pTask->Stat & 0x04) == 0)
		goto _CheckDelays_5;	
	
//   						; ���pTask�Ƿ��ѵȴ���ʱ. ����, ������Ϊready( OS_MakeTaskReady )
//   						; pTask�ĳ�ʱʱ�� pTask->Timeout �Ƿ�С�ڻ����ϵͳʱ�� OS_Global.Time
	if( (pTask->Timeout - OS_Global.Time) >= 1 )	
		goto _CheckDelays_6;
	
  // 						; pTask�ѳ�ʱ
  // 						; ������Ϊready( OS_MakeTaskReady )		
	OS_MakeTaskReady (pTask);

	asm ("MSR       CPSR_c, #147            ; 0x93					���ж�\n");
	
	// ; ��������Ƿ���׼�� (ready). ��������Ready, 
	if(pTask->Stat == 0)
		goto _CheckDelays_exit;
	
_CheckDelays_6:
	// ; ������ĳ�ʱʱ�� < ϵͳ�ĳ�ʱʱ��(OS_Global.TimeDex), ����ϵͳ����һ�γ�ʱʱ��. 
	if( pTask->Timeout < OS_Global.TimeDex )
		OS_Global.TimeDex = pTask->Timeout;
	
_CheckDelays_5: 	
	asm ("MSR       CPSR_c, #19             ; 0x13					���ж�\n");
	
	// ; ȡ�������������һ������Ԫ
	pTask = pTask->pNext;
	
_CheckDelays_3:   
	if(pTask != NULL)
		goto _CheckDelays_loop;
	
	asm ("MSR       CPSR_c, #147            ; 0x93\n");		// ���ж�
_CheckDelays_exit:  
	return;	
}

#include "hardware.h"
static volatile int change_task_do = 1;

void xm_do_change_task (void)
{
	unsigned int change_task_addr;
	//OS_IncDI();
	if(change_task_do)
	{
		extern void OS_ChangeTask(void);
		change_task_do = 0;
		change_task_addr = (unsigned int)OS_ChangeTask;
		*(unsigned int *)(change_task_addr +  0) = 0xe59f1000;
		*(unsigned int *)(change_task_addr +  4) = 0xea000000;
		*(unsigned int *)(change_task_addr +  8) = (unsigned int) RTOS_ChangeTask;	//0x0030014c;
		*(unsigned int *)(change_task_addr + 12) = 0xe12fff11;
			
		dma_clean_range ((unsigned int)OS_ChangeTask, 16 + (unsigned int)OS_ChangeTask);
		__DSB();
		__ISB();
		CP15_DisableIcache();
		CP15_EnableIcache();
	}
	//OS_DecRI();
}