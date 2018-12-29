#include <stdio.h>
#include <stdlib.h>
#include "hardware.h"
#include "xm_core.h"
#include "xm_printf.h"
#include "target_ucos_ii.h"
#include "xm_dev.h"

extern void HardwareTimerTigger(void);
// 使用软件TICKET模拟定时器硬件过程
__no_init static OS_STACKPTR int StackHardTimerTask[XMSYS_HARD_TIMER_STASK_SIZE/4];          /* Task stacks */
static OS_TASK TCBHardTimer;                        /* Task-control-blocks */

//#define	OS_ENTER_CRITICAL		OS_EnterRegion
//#define	OS_EXIT_CRITICAL		OS_LeaveRegion

#define	OS_ENTER_CRITICAL		XM_lock
#define	OS_EXIT_CRITICAL		XM_unlock


typedef struct stHardwareTimerNode
{
	HardwareTimer stTimer;
	struct stHardwareTimerNode *pNext;
}HardwareTimerNode;

#define MAX_HARDWARENODES		32

#define HARDWARE_TIMER_STATUS_STOP			0
#define HARDWARE_TIMER_STATUS_RUN			1


static HardwareTimerNode lg_aHardwareTimerNodes[MAX_HARDWARENODES];
static HardwareTimerNode *lg_pFreeHardwareTimerNodeList = NULL;
static HardwareTimerNode *lg_pHardwareTimerNodeList = NULL;

static void InitHardwareTimers(void)
{
	int i;

#ifdef SYS_UCOSII	
	OS_CPU_SR cpu_sr;
#endif
	
	OS_ENTER_CRITICAL();
	memset(lg_aHardwareTimerNodes, 0, sizeof(lg_aHardwareTimerNodes));
	for(i=0;i<MAX_HARDWARENODES-1;i++)
	{
		lg_aHardwareTimerNodes[i].pNext= &lg_aHardwareTimerNodes[i+1];
	}
	lg_aHardwareTimerNodes[i].pNext = NULL;
	lg_pFreeHardwareTimerNodeList = &lg_aHardwareTimerNodes[0];
	lg_pHardwareTimerNodeList = NULL;
	OS_EXIT_CRITICAL();
}

static HardwareTimerNode *GetFreeHarewareTimerNode(void)
{
	HardwareTimerNode *pFreeNode;

	pFreeNode = lg_pFreeHardwareTimerNodeList;
	if(lg_pFreeHardwareTimerNodeList)
		lg_pFreeHardwareTimerNodeList = lg_pFreeHardwareTimerNodeList->pNext;

	return pFreeNode;
}

static void AddFreeHardwareTimerNode(HardwareTimerNode *pFreeNode)
{
	HardwareTimerNode *pNode;

	pFreeNode->pNext = NULL;
	pNode = lg_pFreeHardwareTimerNodeList;
	if(pNode)
	{
		while(pNode->pNext)
			pNode = pNode->pNext;

		pNode->pNext = pFreeNode;
	}
	else
		lg_pFreeHardwareTimerNodeList = pFreeNode;
}

void HardwareTimerTigger(void)
{
	HardwareTimerNode *pNode;
	HardwareTimer *pTimer;

	OS_ENTER_CRITICAL();
	pNode = lg_pHardwareTimerNodeList;
	while(pNode)
	{
		pTimer = &pNode->stTimer;
		if(pTimer->ulStatus == HARDWARE_TIMER_STATUS_RUN)
		{
			pTimer->ulCurTick --;
			if(pTimer->ulCurTick == 0)
			{
				if(pTimer->opt == OS_TMR_OPT_ONE_SHOT)
					StopHardwareTimer(pTimer);

				pTimer->ulCurTick = pTimer->ulIntervalTick;
				
				if(pTimer->pfnHandler)
				{
					OS_EXIT_CRITICAL();
					pTimer->pfnHandler(pTimer->pdata, 0);
					OS_ENTER_CRITICAL();
				}
				// pTimer->ulCurTick = pTimer->ulIntervalTick;
			}
		}
		pNode = pNode->pNext;
	}
	OS_EXIT_CRITICAL();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
HardwareTimer* NewHardwareTimer(UINT32 ulIntervalTick, void *pdata, void (*pfnTimeOut)(void *, UINT16))
{
	HardwareTimerNode *pNewNode;
	HardwareTimer *pNewTimer = NULL;
	
	OS_ENTER_CRITICAL();
	
	pNewNode = GetFreeHarewareTimerNode();
	if(pNewNode)
	{
		pNewTimer = &pNewNode->stTimer;
		pNewTimer->ulIntervalTick = ulIntervalTick;
		pNewTimer->ulCurTick = ulIntervalTick;
		pNewTimer->ulStatus = HARDWARE_TIMER_STATUS_STOP;
		pNewTimer->pfnHandler = pfnTimeOut;
		pNewTimer->pdata = pdata;
	}

	OS_EXIT_CRITICAL();
	
	return pNewTimer;
}

void AddHardwareTimer(HardwareTimer *pTimer)
{
	HardwareTimerNode *pNode;
	HardwareTimerNode *pNewNode;
#ifdef SYS_UCOSII	
	OS_CPU_SR cpu_sr;
#endif
		
	OS_ENTER_CRITICAL();
	
	if(pTimer)
	{	
		pNewNode = (HardwareTimerNode *)pTimer;
		pNewNode->pNext = NULL;

		pNode = lg_pHardwareTimerNodeList;
		if(pNode)
		{
			while(pNode->pNext)
				pNode = pNode->pNext;

			pNode->pNext = pNewNode;
		}
		else
			lg_pHardwareTimerNodeList = pNewNode;
	}

	OS_EXIT_CRITICAL();
}

void StartHardwareTimer(HardwareTimer *pTimer)
{
#ifdef SYS_UCOSII	
	OS_CPU_SR cpu_sr;
#endif
	
	OS_ENTER_CRITICAL();
	if(pTimer)
	{
		pTimer->ulStatus = HARDWARE_TIMER_STATUS_RUN;
		pTimer->ulCurTick = pTimer->ulIntervalTick;
	}
	OS_EXIT_CRITICAL();
}

void StopHardwareTimer(HardwareTimer *pTimer)
{
#ifdef SYS_UCOSII	
	OS_CPU_SR cpu_sr;
#endif
		
	OS_ENTER_CRITICAL();
	if(pTimer)
		pTimer->ulStatus = HARDWARE_TIMER_STATUS_STOP;
	OS_EXIT_CRITICAL();
}

void DestroyHardwareTimer(HardwareTimer *pTimer)
{
	HardwareTimerNode *pNode;
	HardwareTimerNode *pCurTimerNode;
	HardwareTimerNode *pLastNode;
#ifdef SYS_UCOSII	
	OS_CPU_SR cpu_sr;
#endif
	
	OS_ENTER_CRITICAL();
	
	if(pTimer)
	{
		pTimer->ulStatus = HARDWARE_TIMER_STATUS_STOP;

		pLastNode = NULL;
		pNode = lg_pHardwareTimerNodeList;
		pCurTimerNode = (HardwareTimerNode*)pTimer;
		while(pNode)
		{
			if(pNode == pCurTimerNode)
			{
				if(pLastNode == NULL)
				{
					lg_pHardwareTimerNodeList = pNode->pNext;
				}
				else
				{
					pLastNode->pNext = pNode->pNext;
				}
				AddFreeHardwareTimerNode(pNode);
				break;
			}

			pLastNode = pNode;
			pNode = pNode->pNext; 
		}
	}
	OS_EXIT_CRITICAL();
}


static void XMSYS_HardTimerTask (void)
{
	//XM_printf ("XMSYS_HardTimerTask start\n");
	int t = OS_GetTime();
	while (1)
	{
		// 模拟10ms时钟
		t += 10;
		OS_DelayUntil (t);
		HardwareTimerTigger ();
	}
}

// 初始化并创建模拟硬件的定时器（定时器线程具有最高线程优先级）
void XMSYS_HardTimerInit (void)
{
	InitHardwareTimers ();
	OS_CREATETASK(&TCBHardTimer, "HARDTIMER", XMSYS_HardTimerTask, XMSYS_HARD_TIMER_TASK_PRIORITY, StackHardTimerTask);
}

void XMSYS_HardTimerExit (void)
{
}

