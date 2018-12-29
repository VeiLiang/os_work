#include <stdio.h>
#include "RTOS.h"		// OS头文件
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
	// 检查系统栈的完整性, 检查栈底部的值为0xcD
	addr = (unsigned char *)OS_GetSysStackBase ();
	if(*addr != 0xcd)
	{
		OS_Error (125);
	}
	
OS_ChangeTask_2:
	// 检查Int栈的完整性
	addr = (unsigned char *)OS_GetIntStackBase ();
	if(*addr != 0xcd)
	{
		OS_Error (126);
	}

OS_ChangeTask_3:
	// 保存当前任务的状态 (pExtendContext )

OS_ChangeTask_4:
	if(OS_Global.TimeSliceAtStart == 0)
		goto OS_ChangeTask_6;

	// 保存当前任务未用完的时间片信息 
	OS_Global.pCurrentTask->TimeSliceRem = OS_Global.TimeSlice;

	// 检查是否存在轮询调度
	if(OS_Global.Pending.Flag.RoundRobin == 0)
		goto OS_ChangeTask_6;
	// 执行轮询调度
	RTOS_OS_DoRR ();

OS_ChangeTask_6:
	// 统计当前任务 pCurrentTask 被抢占的次数
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

	// 允许中断, Supervisor模式
	asm ("MSR       CPSR_c, #19\n");

OS_ChangeTask_9:
	// 下一个激活任务 pActiveTask 存在

	// 禁止中断, 切换到Supervisor模式
	asm ("MSR       CPSR_c, #147\n");
	// ;  检查激活任务 pActiveTask是否存在. 如果存在, 判断激活任务的Stat位
	if(OS_Global.pActiveTask)
	{
		if( OS_Global.pActiveTask->Stat  == 0)
			goto OS_ChangeTask_11;
	}
	else
		goto OS_ChangeTask_11;

	// ; 激活任务的Stat不为0
	// ; 检查Stat的bit2是否为0, 即是否等待超时
	if( (OS_Global.pActiveTask->Stat & 0x04) == 0 )
		goto OS_ChangeTask_12;

	// Stat的bit2不为0, 任务等待某个超时时刻 OS_Delay 
   	// 比较任务的超时时刻 Timeout 是否小于或等于 当前系统时间 (OS_Global.Time).
	if( (OS_Global.pActiveTask->Timeout - OS_Global.Time) >= 1 )
		goto OS_ChangeTask_13;		// 未超时

	// ; 已超时, 执行 OS_MakeTaskReady (OS_Global.pActiveTask)将该任务置为ready
	OS_MakeTaskReady ((OS_TASK *)OS_Global.pActiveTask);
	
	if(OS_Global.pActiveTask->Stat != 0)
		goto OS_ChangeTask_12;
	// ; 	激活任务的Stat为0, 即ready状态	
	goto OS_ChangeTask_8;

OS_ChangeTask_13: 
	if( (OS_Global.pActiveTask->Timeout - OS_Global.TimeDex) < 0)
		OS_Global.TimeDex = OS_Global.pActiveTask->Timeout;

OS_ChangeTask_12: 
	// ;	检查任务是否等待其他(通过pWaitList访问)  (OS_RSEMA)
	if( (OS_Global.pActiveTask->Stat & 0xFB) != 0x10) 
		goto OS_ChangeTask_loop;

	// ; 任务通过pWaitList等待其他事件对象 (OS_RSEMA)
	pWaitList = OS_Global.pActiveTask->pWaitList;
	pResm = (OS_RSEMA *)OS_Global.pActiveTask->pWaitList->pWaitObj;
	pTask = pResm->pTask;
	if(pTask->Stat != 0)
		goto OS_ChangeTask_loop;

OS_ChangeTask_8:
	asm ("MSR       CPSR_c, #147\n");		// ; 关中断,切换到Supervisor模式

OS_ChangeTask_11: 
	// ; 检查是否Preempion不为0
	if(OS_Global.Pending.Flag.Preemption != 0)
		goto OS_ChangeTask_20;

OS_ChangeTask_task_40:
	if( (OS_TickStep & 1) == 0)
		goto OS_ChangeTask_21;
	
	assert (0);
	

OS_ChangeTask_20:
	// ; 循环直到 OS_Global.Preemption == 0
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

	// 任务调度Bug, 与DCache没有关系(不管DCache开启还是禁止)
	// ISP任务在某种状态下无法继续调度, 此时ISP任务有效, Ready, 且优先级最高.
	//
	// ;设置当前任务 pCurrentTask	
	// 检查ISP任务是否有效, ready, 且不是当前任务.
	// 若满足, 设置TCB_Isp为当前任务
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
	// ; 检查pCurrentTask的栈, (栈底部的值为0xcd)
	if(OS_Global.pCurrentTask->pStackBot[0] == 0xcd)
		goto OS_ChangeTask_task_stack_ok;

	OS_Error(120);

OS_ChangeTask_task_stack_ok: 
	// 检查pCurrentTask的pExtendContext是否存在(pExtendContext未使用, 固定为0)

OS_ChangeTask_task_30:
	// ; 将pCurrentTask当前任务的pTLS设置为系统的OS_pTLS

	// 将当前任务的时间片设置
	OS_Global.TimeSlice = OS_Global.pCurrentTask->TimeSliceRem;
	OS_Global.TimeSliceAtStart = OS_Global.pCurrentTask->TimeSliceRem;

	// 累加当前任务的活跃计数 + 1
	OS_Global.pCurrentTask->NumActivations ++;
	goto OS_ChangeTask_exit;
	
 OS_ChangeTask_22: 
	 //if(TCB_Isp.Stat == 0 && OS_Global.pTask == &TCB_Isp)
	 {
		 // printf ("task error\n");
	 }
	// ; 没有任务调度, 调用OS_Idle
	OS_Global.TimeSliceAtStart = 0;
	OS_Global.TimeSlice = 0;
	OS_Global.pCurrentTask = 0;
	OS_Global.Counters.Cnt.Region = 0;
	
	asm ("MSR       CPSR_c, #19 \n");		//						开中断, Supervisor
	OS_Idle ();
	OS_Error (119);

 OS_ChangeTask_exit: 
	return &OS_Global;
}

void RTOS_OS_DoRR (void)
{
	OS_TASK *pTask, *pNext;
	//				; RoundRobin轮询式调度算法, 仅在OS_Global.RoundRobin == 1的情况下调用
	//				;
	//				; 	算法描述 : 轮询调度选择与当前任务优先级相同的下一个任务单元进行调度, 并将当前任务链接到最后一个与它优先级相同的任务的后面. (优先级相同的同组内轮询调度)
	//				;
		//			; 检查OS_Global.pCurrentTask是否存在. 
	//				;	若存在, 从任务列表中pCurrentTask的下一个任务单元(pNext)开始遍历, 查找第一个"优先级与pCurrentTask的优先级不相同"的任务单元
	asm ("MSR       CPSR_c, #147            ; 0x93								关中断\n");
	
	// ; 清除轮询式调度标志	OS_Global.RoundRobin = 0
	OS_Global.Pending.Flag.RoundRobin = 0;
	
	asm ("MSR       CPSR_c, #19             ; 0x13								开中断\n");
	
   //				; 检查当前任务是否存在  OS_Global.pCurrentTask != NULL, 若不存在, 返回.
   //				; 若当前任务存在, 检查当前任务的下一个任务(按照任务链表链接的顺序)是否存在. 若不存在, 返回
	if(OS_Global.pCurrentTask != NULL)
	{
		pTask = (OS_TASK *)OS_Global.pCurrentTask->pNext;
		if(pTask == NULL)
			goto _OS_DoRR_exit;
	}
	else
		goto _OS_DoRR_exit;
	
	// ; 检查下一个任务单元(r2)的优先级是否与当前任务相同. 若不相同, 返回	
	if( OS_Global.pCurrentTask->Priority  != pTask->Priority)
		goto _OS_DoRR_exit;
	goto _OS_DoRR_1;	
	
  	// 				; 遍历任务列表, 直到任务的下一个链接为NULL或者下一个任务的优先级与当前任务的优先级不同
   //				; 即在任务列表中查找最后一个优先级与当期任务的优先级相同的任务单元, 查找位置从当前任务的下一个任务开始
_OS_DoRR_loop:   
	pTask = pNext;
	
_OS_DoRR_1:
	pNext = 	pTask->pNext;
	if(pNext == NULL)
		goto _OS_DoRR_2;
	if(pNext->Priority == OS_Global.pCurrentTask->Priority)
		goto _OS_DoRR_loop;
	
_OS_DoRR_2:    
//				; 存在与当前任务pCurrentTask优先级相同的任务, 
//				; pTask(r2)指向的任务单元的优先级与pCurrentTask的优先级相同, 是最后一个与pCurrentTask的优先级相同的任务单元
	asm ("MSR       CPSR_c, #147            ; 0x93						关中断\n");
	
	// 轮询调度选择与当前任务优先级相同的下一个任务单元进行调度
	if(OS_Global.pActiveTask == OS_Global.pCurrentTask)
		OS_Global.pActiveTask = OS_Global.pCurrentTask->pNext;
	
	// ; 将当前任务单元链接到最后一个与它优先级相同的任务单元的后面.
	OS_Global.pCurrentTask->pPrev->pNext = OS_Global.pCurrentTask->pNext;
	OS_Global.pCurrentTask->pNext->pPrev = OS_Global.pCurrentTask->pPrev;
	OS_Global.pCurrentTask->pPrev = pTask;		// (pTask 是最后一个与它优先级相同的任务)
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
	// ; 抢占式调度
	// ; 清除 抢占式调度标志 OS_Global.Preemption = 0
	OS_Global.Pending.Flag.Preemption = 0;
	
	// ; 更新下一次抢占度调度的时刻 OS_Global.TimeDex = OS_Global.Time + 4000;		// 4秒
	OS_Global.TimeDex = OS_Global.Time + 4000;
	goto _CheckDelays_1;
	
 _CheckDelays_10:	
	// ; pTimer已超时, 将pTimer从TIMER链表中移除, 若Hook存在, 调用Hook函数
	OS_Global.pCurrentTimer = pTimer; 
	OS_Global.pTimer = pTimer->pNext;
	pTimer->Active = 0;			// 将Timer置为不活跃
	OS_Global.pCurrentTimer->Time = 0;
	
	asm ("MSR       CPSR_c, #19             ; 0x13					开中断\n");
	
	OS_InTimer ++;
	pTimer  = OS_Global.pCurrentTimer;
	(*pTimer->Hook)();
	OS_InTimer --;
	
	asm ("MSR       CPSR_c, #147            ; 0x93					关中断\n");
	
_CheckDelays_1: 
	//									; 遍历每个系统定时器, 检查其超时时间是否大于系统时间
	//									; 检查系统定时器 OS_Global.pTimer 是否为空. 
	pTimer = OS_Global.pTimer;	
	if(pTimer == NULL)
		goto _CheckDelays_2;

	// 检查定时器 pTimer 是否超时(大于系统时间)
	// 检查pTimer是否超时 (pTimer->Time  <= OS_Global.Time)
	if( (pTimer->Time - OS_Global.Time) < 1 )
		goto _CheckDelays_10;	
	
  	//								; 定时器的超时时间大于当前的系统时间
	// 									; 取出定时器的超时时刻并设置为下一次的抢占式调度时刻
	OS_Global.TimeDex = pTimer->Time;

_CheckDelays_2:  
	asm ("MSR       CPSR_c, #19             ; 0x13					CPSR_c = 0x13, 开中断, supervisor\n");
	
	// ; 从TASK链表的第一个单元开始遍历任务队列
	pTask = OS_Global.pTask;
	goto _CheckDelays_3;

_CheckDelays_loop:
	
	asm ("MSR       CPSR_c, #147            ; 0x93					CPSR_c = 0x93, 关中断, supervisor	\n");
	
	// ; 检查是否存在活跃的任务 OS_Global.pActiveTask
	if(OS_Global.pActiveTask == 0)
		goto _CheckDelays_4;
	
  //					; 比较pTask的优先级与OS_Global.pActiveTask的优先级
  //					; 若pTask的优先级 < pActiveTask的优先级, 遍历结束, 返回
	if(pTask->Priority < OS_Global.pActiveTask->Priority)
		goto _CheckDelays_exit;	// 遍历结束, 返回

_CheckDelays_4: 
	// ; pTask的优先级 >= pActiveTask的优先级	
	// ; 检查pTask是否等待超时 (Stat的bit2为1)
	if( (pTask->Stat & 0x04) == 0)
		goto _CheckDelays_5;	
	
//   						; 检查pTask是否已等待超时. 若是, 将其置为ready( OS_MakeTaskReady )
//   						; pTask的超时时间 pTask->Timeout 是否小于或等于系统时间 OS_Global.Time
	if( (pTask->Timeout - OS_Global.Time) >= 1 )	
		goto _CheckDelays_6;
	
  // 						; pTask已超时
  // 						; 将其置为ready( OS_MakeTaskReady )		
	OS_MakeTaskReady (pTask);

	asm ("MSR       CPSR_c, #147            ; 0x93					关中断\n");
	
	// ; 检查任务是否已准备 (ready). 若任务已Ready, 
	if(pTask->Stat == 0)
		goto _CheckDelays_exit;
	
_CheckDelays_6:
	// ; 若任务的超时时间 < 系统的超时时限(OS_Global.TimeDex), 更新系统的下一次超时时限. 
	if( pTask->Timeout < OS_Global.TimeDex )
		OS_Global.TimeDex = pTask->Timeout;
	
_CheckDelays_5: 	
	asm ("MSR       CPSR_c, #19             ; 0x13					开中断\n");
	
	// ; 取出任务链表的下一个任务单元
	pTask = pTask->pNext;
	
_CheckDelays_3:   
	if(pTask != NULL)
		goto _CheckDelays_loop;
	
	asm ("MSR       CPSR_c, #147            ; 0x93\n");		// 关中断
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