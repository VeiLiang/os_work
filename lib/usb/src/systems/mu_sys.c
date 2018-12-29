/******************************************************************
*                                                                *
*      Copyright Mentor Graphics Corporation 2003-2005           *
*                                                                *
*                All Rights Reserved.                            *
*                                                                *
*    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION *
*  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS   *
*  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                    *
*                                                                *
******************************************************************/

/*
 * uC/OS-II implementation of a system for Controller Driver
 * $Revision: 1.3 $
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "mu_tools.h"

#include "plat_cnf.h"
#include "mu_sys.h"
#include "mu_mem.h"
#include "mu_stdio.h"
#include "target_ucos_ii.h"
/*
 * Fill driver table
 */
#include "mu_cdi.h"
#include "mu_impl.h"
#include "mu_mapi.h"
#include "mu_hapi.h"
#include "mu_uapi.h"
#include "mu_iapi.h"

#ifndef MUSB_MAX_CONTROLLERS
#define MUSB_MAX_CONTROLLERS 1
#endif

#include "includes.h"
#define XMSYS_MUSYS_TASK_STACK_SIZE 0x1000
#pragma data_alignment=32	//add by helen
static OS_TASK TCBMUSYS;                        /* Task-control-blocks */
#pragma data_alignment=32	//add by helen
static OS_STACKPTR int StackMusysTask[XMSYS_MUSYS_TASK_STACK_SIZE/4];          /* Task stacks */
/***************************** TYPES ******************************/

/** Wrapping Ucos Timer */
typedef struct
{
    uint32_t            dwTime;
    uint32_t            dwTimePeriod;
    uint8_t             bPeriodic;
    uint8_t             unused;
    uint16_t 		wIndex;
    MUSB_pfTimerExpired pfExpired;
    void* 		pExpireParam;
} MGC_UcosTimerWrapper;

/* System */
typedef struct _MGC_UcosSystem
{
    MUSB_Controller*	    pController;
    uint8_t*                aQueueMsg;
    uint32_t                dwQueueHead;
    uint32_t                dwQueueTail;
    //OS_SEM*		aLockSem;
    //void**                  aLock;
	OS_EVENT**				aLock;
    MGC_UcosTimerWrapper*   aTimer;
    void*		    pBoardPrivateData;
    uint8_t* 		    pPciIackAddr;
    MUSB_SystemServices	    Services;
    void*       	    pInterrupt;
    uint8_t       	    bThread;
    //OS_SEM		Semaphore;
    //void*             	    pSemaphore;
	OS_EVENT*				pSemaphore;
    CPU_STK                 aBsrStackData[MUSB_UCOS_TASK_STACK_SIZE];
    CPU_STK                 aTimerStackData[MUSB_UCOS_TASK_STACK_SIZE];
    OS_TCB		TaskTCB;
} MGC_UcosSystem;

/**************************** GLOBALS *****************************/

static MGC_UcosSystem* MGC_apUcosSystem[MUSB_MAX_CONTROLLERS];

static uint8_t MGC_bUcosSystemCount = 0;

/////////////Fill driver table//////////////
#pragma data_alignment=32	//add by helen
static uint8_t MGC_aPeripheralList[256];

static MUSB_DeviceDriver MGC_aDeviceDriver[MAX_SUPPORT_DEVICE];
	
static MUSB_HostClient MGC_HostClient = 
{
    MGC_aPeripheralList, 	/* peripheral list */
    0,						/*sizeof(MGC_aPeripheralList),*/ /* peripheral list length */						
    MGC_aDeviceDriver,
    0
};

/*************************** FUNCTIONS ****************************/

/*
 * ISR, arranged to be called by board-specific code.
 */
static uint8_t MGC_UcosControllerIsr(void* pPrivateData)
{
	int isrValue;
	OS_ERR err;
	volatile MGC_UcosSystem* pSystem = (MGC_UcosSystem*)pPrivateData;

	/* call controller ISR */
	isrValue = pSystem->pController->pfIsr(pSystem->pController->pIsrParam);

	/* indicate spurious interrupt if it was */
	if(isrValue < 0)
	{
		return FALSE;
	}

	/* wake BSR task if indicated */
	if(isrValue > 0)
	{
		//OSSemPost(pSystem->pSemaphore, OS_OPT_POST_1, &err);
		if(OSSemPost(pSystem->pSemaphore))
		//if(OS_ERR_NONE != err)
		{
			MUSB_PrintLine(" Semaphore Post error ");
		}
	}
	return TRUE;
}

/*
 * The UCD calls this to arm a timer (periodic or one-shot).
 * Call the board-specific function.
 */
static uint8_t MGC_UcosArmTimer(void*               pPrivate, 
				uint16_t            wIndex, 
				uint32_t            dwTime, 
				uint8_t             bPeriodic, 
				MUSB_pfTimerExpired pfExpireCallback)
{
	MGC_UcosSystem*       pSystem = (MGC_UcosSystem*)pPrivate;

	return MUSB_BoardArmTimer(pSystem->pBoardPrivateData, wIndex, dwTime,
		bPeriodic, pfExpireCallback, pSystem->pController->pPrivateData);
}

/*
 * The UCD calls this to cancel a timer.
 * Call the board-specific function.
 */
static uint8_t MGC_UcosCancelTimer(void*    pPrivate, 
				   uint16_t wIndex)
{
	MGC_UcosSystem* pSystem = (MGC_UcosSystem*)pPrivate;

	return MUSB_BoardCancelTimer(pSystem->pBoardPrivateData, wIndex);
}


/*
* Controller calls this to enqueue a background item
*/
static uint8_t MGC_UcosQueueBackgroundItem(void* pPrivate, const void* pItem)
{
	MGC_UcosSystem* pSystem = (MGC_UcosSystem*)pPrivate;

	/* Post a message to the queue */
	if ( ((pSystem->dwQueueHead + 1) %  pSystem->pController->wQueueLength) != pSystem->dwQueueTail )
	{
		MUSB_MemCopy((void*)((intptr_t)pSystem->aQueueMsg + (pSystem->dwQueueHead * pSystem->pController->wQueueItemSize)), 
			pItem, pSystem->pController->wQueueItemSize);
		pSystem->dwQueueHead = (pSystem->dwQueueHead + 1) % pSystem->pController->wQueueLength;
		return (TRUE);
	}
	return (FALSE);
}

/*
* Controller calls this to dequeue a background item
*/
static uint8_t MGC_UcosDequeueBackgroundItem(void* pPrivate, 
					     void* pItem)
{
	MGC_UcosSystem* pSystem = (MGC_UcosSystem*)pPrivate;

	/* Read a message from Queue */
	if (pSystem->dwQueueTail != pSystem->dwQueueHead)
	{
		MUSB_MemCopy(pItem, (void*)((intptr_t)pSystem->aQueueMsg + (pSystem->dwQueueTail * pSystem->pController->wQueueItemSize)),
			pSystem->pController->wQueueItemSize);

		pSystem->dwQueueTail = (pSystem->dwQueueTail + 1) % pSystem->pController->wQueueLength;

		return (TRUE);
	}
	return (FALSE);
}

static void MGC_UcosFlushBackgroundQueue(void* pPrivate)
{
	MGC_UcosSystem* pSystem = (MGC_UcosSystem*)pPrivate;

	pSystem->dwQueueTail = 0;
	pSystem->dwQueueHead = 0;
	return;
}

/*
* Controller calls this to enter a lock
*/
static uint8_t MGC_UcosLock(void*    pPrivate, 
			    uint16_t wIndex)
{
	//OS_ERR err;
	//CPU_TS ts;
        INT8U err;
	MGC_UcosSystem* pSystem = (MGC_UcosSystem*)pPrivate;

	/* Obtains a Semaphore */
	if(wIndex < pSystem->pController->wLockCount)
	{
		//OSSemPend( (OS_SEM *)pSystem->aLock[wIndex], MGC_UCOS_SUSPEND, OS_OPT_PEND_BLOCKING, &ts, &err );
		OSSemPend(pSystem->aLock[wIndex], MGC_UCOS_SUSPEND, &err );
		if(OS_ERR_NONE == err)
		{
			return (TRUE);
		}
	}
	return(FALSE);
}

/*
* Controller calls this to exit a lock
*/
static uint8_t MGC_UcosUnlock(void*    pPrivate, 
			      uint16_t wIndex)
{
	OS_ERR err;
	MGC_UcosSystem* pSystem = (MGC_UcosSystem*)pPrivate;

	/* Release the semaphore */
	//OSSemPost((OS_SEM *)pSystem->aLock[wIndex], OS_OPT_POST_1, &err);
	if(!OSSemPost(pSystem->aLock[wIndex]))
	//if(OS_ERR_NONE == err)
	{
		return (TRUE);
	}
	return(FALSE);
}

/* Controller will use the follwoing 3 services */
uint8_t MGC_UcosMessageString(char*       pMsg, 
			      uint16_t    wBufSize, 
			      const char* pString)
{
	if((strlen(pMsg) + strlen(pString)) >= wBufSize)
	{
		return (FALSE);
	}
	strcat(pMsg, pString);
	return (TRUE);
}

uint8_t MGC_UcosMessageNumber(char*    pMsg, 
			      uint16_t wBufSize, 
			      uint32_t dwNumber, 
			      uint8_t  bBase, 
			      uint8_t  bJustification)
{
	char type;
	char format[8];
	char fmt[16];
	char number[32];

	switch(bBase)
	{
		case 8:
		{
			type = 'o';
			break;
		}
		case 10:
		{
			type = 'd';
			break;
		}
		case 16:
		{
			type = 'x';
			break;
		}
		default:
		{
			return (FALSE);
		}
	}
	
	if(bJustification)
	{
		sprintf(format, "0%d%c", bJustification, type);
	}
	else
	{
		sprintf(format, "%c", type);
	}
	fmt[0] = '%';
	fmt[1] = (char)0;
	strcat(fmt, format);
	sprintf(number, fmt, dwNumber);

	return (MGC_UcosMessageString(pMsg, wBufSize, number));
}

/* Get the OS time in terms of ticks*/
uint32_t MGC_UcosGetTime()
{
	//OS_ERR err;
	
	//return (OSTimeGet(&err));
	return (OSTimeGet());
}

/*
* Utilities provided to controller
*/
static MUSB_SystemUtils MGC_gUcosUtils = {
    MUSB_SYSTEM_UTILS_VERSION,
    MGC_UcosMessageString,
    MGC_UcosMessageNumber,
    MGC_UcosGetTime
};

/* Back ground service Routine */
static void MGC_UcosBsrThread(void* pParam)
{
	//OS_ERR           err;
	//CPU_TS ts;
        INT8U err;
	MGC_UcosSystem* pSystem = (MGC_UcosSystem*)pParam;

	while(1)
	{
		//OSSemPend((OS_SEM *)(pSystem->pSemaphore), MGC_UCOS_SUSPEND, OS_OPT_PEND_BLOCKING, &ts, &err);
		OSSemPend((pSystem->pSemaphore), MGC_UCOS_SUSPEND, &err);
		if(OS_ERR_NONE == err)
		{
			pSystem->pController->pfBsr(pSystem->pController->pBsrParam);
		}
	}
}

/*
 * Delete all the resources allocated for the given system
 */
static void MGC_UcosDestroyController(MGC_UcosSystem* pSystem)
{
	uint16_t wIndex;
	OS_ERR           err;

	MUSB_StopController(pSystem->pController);

	/* Delete the Semaphores created and release corresponding memory */
	if(pSystem->aLock)
	{ 
		for( wIndex = 0; wIndex < pSystem->pController->wLockCount; wIndex++)
		{
			if(pSystem->aLock[wIndex])
			{
				/* Delete the created locks */	
				//OSSemDel((OS_SEM *)pSystem->aLock[wIndex], OS_OPT_DEL_ALWAYS, &err);
				OSSemDel(pSystem->aLock[wIndex], OS_OPT_DEL_ALWAYS, (INT8U *)&err);
				pSystem->aLock[wIndex] = NULL;
			}
		}

//		if(pSystem->aLockSem)
//		{
//			MUSB_MemFree(pSystem->aLockSem);
//			pSystem->aLockSem = NULL;
//		}
		MUSB_MemFree(pSystem->aLock);
		pSystem->aLock = NULL;
	}

	/* Delete the Queues created and release corresponding memory */
	if(pSystem->aQueueMsg)
	{
		MUSB_MemFree(pSystem->aQueueMsg);
		pSystem->aQueueMsg = NULL;
	}

	/* Delete thread */

	if(pSystem->bThread)
	{
		/* destroy BSR task */
//		OSTaskDel(pSystem->bThread);
//		OSTaskDel(&pSystem->TaskTCB, &err);
		OS_Terminate(&TCBMUSYS);

		pSystem->bThread = 0;
	}


	if(pSystem->pSemaphore)
	{
		/* destroy semaphore for BSR task */
		//OSSemDel((OS_SEM *)pSystem->pSemaphore, OS_OPT_DEL_ALWAYS, &err);
		OSSemDel(pSystem->pSemaphore, OS_OPT_DEL_ALWAYS, (INT8U *)&err);
		pSystem->pSemaphore = NULL;
	}
	MUSB_DestroyController(pSystem->pController);
	MUSB_BoardDestroyController(pSystem->pBoardPrivateData);
	MUSB_MemFree(pSystem);
	pSystem = NULL;		
}

void MGC_HostFillDriverTable(MUSB_Controller*	pController)
{
	MGC_Controller* pImpl = NULL;	
	MUSB_DeviceDriver* pDriver;
	uint8_t* pList;
	uint16_t wCount, wSize, wRemain;
	uint8_t bDriver;
	uint32_t dwCounter = 0;

	if(pController)
	{
		pImpl = (MGC_Controller*)pController->pPrivateData;
	}

	if(pImpl)
	{
		/* fill driver table */
		bDriver = 0;
		wSize = wCount = 0;
		wRemain = (uint16_t)sizeof(MGC_aPeripheralList);
		pList = MGC_aPeripheralList;
#ifdef MUSB_IPOD
		wSize = MGC_FillIpodClassPeripheralList(pList, wRemain);
		if(wSize < wRemain)
		{
			pDriver = MGC_GetIpodClassDriver();
			if(pDriver)
			{
				MUSB_MemCopy(&(MGC_HostClient.aDeviceDriverList[bDriver]), pDriver, sizeof(MUSB_DeviceDriver));
				pList += wSize;
				wCount += wSize + 1;
				wRemain -= wSize;
				*pList++ = bDriver++;
			}
		}	
#endif
		wSize = MGC_FillStorageClassPeripheralList(pList, wRemain);
		if(wSize < wRemain)
		{
			pDriver = MGC_GetStorageClassDriver();
			if(pDriver)
			{
				MUSB_MemCopy(&(MGC_HostClient.aDeviceDriverList[bDriver]), pDriver, sizeof(MUSB_DeviceDriver));
				pList += wSize;
				wCount += wSize + 1;
				wRemain -= wSize;
				*pList++ = bDriver++;
			}
		}

#ifdef MUSB_HUB
		wSize = MGC_FillHubClassPeripheralList(pList, wRemain);
		if(wSize < wRemain)
		{
			pDriver = MGC_GetHubClassDriver();
			if(pDriver)
			{
				MUSB_MemCopy(&(MGC_HostClient.aDeviceDriverList[bDriver]), pDriver, sizeof(MUSB_DeviceDriver));
				pList += wSize;
				wCount += wSize + 1;
				wRemain -= wSize;
				*pList++ = bDriver++;
			}
		}
#endif
#ifdef MUSB_ISO
		wSize = MGC_FillUvcClassPeripheralList(pList, wRemain);
		if(wSize < wRemain)
		{
			pDriver = MGC_GetUvcClassDriver();
			if(pDriver)
			{
				MUSB_MemCopy(&(MGC_HostClient.aDeviceDriverList[bDriver]), pDriver, sizeof(MUSB_DeviceDriver));
				pList += wSize;
				wCount += wSize + 1;
				wRemain -= wSize;
				*pList++ = bDriver++;
			}
		}	  
#endif
		MGC_HostClient.wPeripheralListLength = wCount;	
		MGC_HostClient.bDeviceDriverListLength = bDriver;
		pImpl->pPort->pHostClient = &MGC_HostClient;  
	}
}

/*
* Initialize the system with the services
*/
static MGC_UcosSystem* MGC_UcosInitController(
    const MUSB_UcosController* pControllerInfo, uint8_t bBsrPriority)
{
	MGC_UcosSystem* pSystem;
	uint8_t* pBaseIsr;
	uint8_t* pBaseBsr;
	OS_ERR    	err;
	uint16_t 	wIndex;
	int32_t  	dwStatus;

	pSystem = (MGC_UcosSystem*)MUSB_MemAlloc(sizeof(MGC_UcosSystem));
	if(pSystem)
	{
		pSystem->Services.wVersion                = MUSB_SYSTEM_SERVICES_VERSION;
		pSystem->Services.pPrivateData            = (void*)pSystem;
		pSystem->Services.pfSystemToBusAddress    = MUSB_BoardSystemToBusAddress;
		pSystem->Services.pfQueueBackgroundItem   = MGC_UcosQueueBackgroundItem;
		pSystem->Services.pfDequeueBackgroundItem = MGC_UcosDequeueBackgroundItem;
		pSystem->Services.pfFlushBackgroundQueue  = MGC_UcosFlushBackgroundQueue;
		pSystem->Services.pfArmTimer              = MGC_UcosArmTimer;
		pSystem->Services.pfCancelTimer           = MGC_UcosCancelTimer;
		pSystem->Services.pfLock                  = MGC_UcosLock;
		pSystem->Services.pfUnlock                = MGC_UcosUnlock;
		pSystem->Services.pfPrintDiag             = MUSB_BoardPrintDiag;

		/* for structured error handling: */
		do
		{
			/* try target-specific init */
			pBaseIsr = pBaseBsr = pControllerInfo->pBase;
			pSystem->pBoardPrivateData = MUSB_BoardInitController(pSystem, MGC_UcosControllerIsr, pControllerInfo, 
														&pBaseIsr, &pBaseBsr, &(pSystem->pPciIackAddr));
			if(!pSystem->pBoardPrivateData)
			{
				MUSB_MemFree(pSystem);
				pSystem = NULL;
				break;
			}

			/* try UCD init */
			pSystem->pController = MUSB_NewController(&MGC_gUcosUtils, pControllerInfo->wType, pBaseIsr, pBaseBsr);
			if(!pSystem->pController)
			{
				MUSB_BoardDestroyController(pSystem->pBoardPrivateData);
				MUSB_MemFree(pSystem);
				pSystem = NULL;
				break;
			}

			/* fill driver table */
			MGC_HostFillDriverTable(pSystem->pController);

			/* try target-specific timer init now that we know the requirements */
			if(!MUSB_BoardInitTimers(pSystem->pBoardPrivateData, 
				pSystem->pController->wTimerCount, 
				pSystem->pController->adwTimerResolutions))
			{
				MGC_UcosDestroyController(pSystem);
				MUSB_MemFree(pSystem);
				pSystem = NULL;
				break;
			}

			/* prepare queue */
			pSystem->aQueueMsg = MUSB_MemAlloc(pSystem->pController->wQueueLength * 
						pSystem->pController->wQueueItemSize);
			if(!pSystem->aQueueMsg)
			{
				MGC_UcosDestroyController(pSystem);
				MUSB_MemFree(pSystem);
				pSystem = NULL;
				break;
			}
			MUSB_MemSet(pSystem->aQueueMsg, 0, pSystem->pController->wQueueLength * 
					pSystem->pController->wQueueItemSize);

			/* Initialize queue head and tail to null */
			pSystem->dwQueueTail = 0;
			pSystem->dwQueueHead = 0;

			/* create mailbox to wake the BSR task */
			pSystem->pSemaphore = OSSemCreate(MGC_UCOS_BINARY_SEM_COUNT);
			if(!pSystem->pSemaphore)
			//OSSemCreate(&pSystem->Semaphore, NULL, MGC_UCOS_BINARY_SEM_COUNT, &err);
			//if(OS_ERR_NONE != err)
			{
				MGC_UcosDestroyController(pSystem);
				MUSB_MemFree(pSystem);
				pSystem = NULL;
				break;
			}
			//else
				//pSystem->pSemaphore = &pSystem->Semaphore;

			/* prepare locks */
			pSystem->aLock = MUSB_MemAlloc(pSystem->pController->wLockCount * sizeof(void *));
			if(pSystem->aLock)
			{
				MUSB_MemSet(pSystem->aLock, 0, pSystem->pController->wLockCount * sizeof(void *));
				
//				pSystem->aLockSem = MUSB_MemAlloc(pSystem->pController->wLockCount * sizeof(OS_SEM));
//				if(!pSystem->aLockSem)
//				{
//					MGC_UcosDestroyController(pSystem);
//					break;
//				}
//				MUSB_MemSet(pSystem->aLockSem, 0, pSystem->pController->wLockCount * sizeof(OS_SEM));

				for(wIndex = 0; wIndex < pSystem->pController->wLockCount; wIndex++)
				{
					/* Create a binary semaphore */
					pSystem->aLock[wIndex] = OSSemCreate(MGC_UCOS_BINARY_SEM_COUNT);
					if(!pSystem->aLock[wIndex])
					//OSSemCreate(&pSystem->aLockSem[wIndex], NULL, MGC_UCOS_BINARY_SEM_COUNT, &err);
					//if(OS_ERR_NONE != err)
					{
						break;
					}
					//else
						//pSystem->aLock[wIndex] = &pSystem->aLockSem[wIndex];
				}
				if (wIndex < pSystem->pController->wLockCount)
				{
					MGC_UcosDestroyController(pSystem);
					break;
				}
			}
			else
			{
				MGC_UcosDestroyController(pSystem);
				MUSB_MemFree(pSystem);
				pSystem = NULL;
				break;
			}

			/* start the controller */
			dwStatus = MUSB_StartController(pSystem->pController, &(pSystem->Services));
			if(MUSB_STATUS_OK != dwStatus)
			{
				MGC_UcosDestroyController(pSystem);
				MUSB_MemFree(pSystem);
				pSystem = NULL;
				break;
			}
#if 0
			/* Create the BSR task, Top of the stack has to be passed  */
//			err = OSTaskCreate( (void (*) (void *))MGC_UcosBsrThread, (void *)pSystem, 
//							(OS_STK *)&(pSystem->aBsrStackData[MUSB_UCOS_TASK_STACK_SIZE-sizeof(unsigned long)]), 
//							(INT8U)(bBsrPriority - MGC_bUcosSystemCount));
			OSTaskCreate(   (OS_TCB *)&(pSystem->TaskTCB),
							(CPU_CHAR *)"MGC_UcosBsrThread",
							(OS_TASK_PTR)MGC_UcosBsrThread,
							(void *) pSystem,
							(OS_PRIO)(bBsrPriority - MGC_bUcosSystemCount),
							(CPU_STK *)&(pSystem->aBsrStackData[0]),
							(CPU_STK_SIZE)(MUSB_UCOS_TASK_STACK_SIZE/10),
							(CPU_STK_SIZE)MUSB_UCOS_TASK_STACK_SIZE,
							(OS_MSG_QTY)0,
							(OS_TICK)0,
							(void *)0,
							(OS_OPT)(OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR|OS_OPT_TASK_NO_TLS),
							(OS_ERR *)&err);
			//if(err != OS_ERR_NONE)
				//printk("Create MGC_UcosBsrThread failed\r\n");
	
//			if(OS_NO_ERR != err)
			if(OS_ERR_NONE != err)
			{
				MGC_UcosDestroyController(pSystem);
				MUSB_MemFree(pSystem);
				pSystem = NULL;
				break;
			}
			pSystem->bThread = bBsrPriority - MGC_bUcosSystemCount;
#else
			OS_CREATETASK_EX (&TCBMUSYS, "MGC_UcosBsrThread", 
				MGC_UcosBsrThread, 
				bBsrPriority - MGC_bUcosSystemCount, 
				StackMusysTask,
				(void *) pSystem
				);

			pSystem->bThread = bBsrPriority - MGC_bUcosSystemCount;
#endif
		} while(FALSE);
	}

	return pSystem;
}

/*
 * Startup code calls this
 */
uint8_t MUSB_InitSystem(uint32_t dwBsrPriority)
{
	MGC_UcosSystem* pSystem;
	uint8_t bIndex;

	MGC_bUcosSystemCount = 0;
	MUSB_MemSet(MGC_apUcosSystem, 0, sizeof(MGC_apUcosSystem));

	for(bIndex = 0; bIndex < (uint8_t)MUSB_MAX_CONTROLLERS; bIndex++)
	{
		pSystem = MGC_UcosInitController(&(MUSB_aUcosController[bIndex]), (uint8_t)(dwBsrPriority & 0xff));
		if(pSystem)
		{
			MGC_apUcosSystem[MGC_bUcosSystemCount++] = pSystem;
		}
	}
	if(MGC_bUcosSystemCount < (uint8_t)MUSB_MAX_CONTROLLERS)
	{
		MUSB_DestroySystem();
		return FALSE;
	}

	return TRUE;
}

uint8_t MUSB_DestroySystem(void)
{
	uint8_t bIndex;

	for(bIndex = 0; bIndex < MGC_bUcosSystemCount; bIndex++)
	{
		MGC_UcosDestroyController(MGC_apUcosSystem[bIndex]);
	}
	return TRUE;
}

#ifdef MUSB_SYSTEM_TEST

/*
 * System test instrumentation
 */
 
static volatile uint8_t MGC_bRunTest = FALSE;
static INT8U MGC_hTestTask;
static const uint8_t* MGC_abInterruptCounts;
static CPU_STK MGC_aTestStackData[MUSB_UCOS_TASK_STACK_SIZE];

static void MGC_TestTask(void* pParam)
{
	uint8_t bController;
	uint8_t bIndex, bCount;
	MGC_UcosSystem* pSystem;

	while(MGC_bRunTest)
	{
		/* for each controller... */
		for(bController = 0; bController < MGC_bUcosSystemCount; bController++)
		{
			pSystem = MGC_apUcosSystem[bController];
			bCount = MGC_abInterruptCounts[bController];
			/* for the requested # interrupts... */
			for(bIndex = 0; bIndex < bCount; bIndex++)
			{
				/* call our ISR */
				MGC_UcosControllerIsr(pSystem);
			}
		}
	}
}

static OS_TCB MGC_TestTaskTCB;

uint8_t MUSB_SystemTestStart(const uint8_t* abInterruptCounts, 
    uint8_t bCountArrayLength)
{
	OS_ERR err;
	uint8_t bSuccess = FALSE;

	if(MGC_bUcosSystemCount == bCountArrayLength)
	{
		MGC_abInterruptCounts = abInterruptCounts;
		MGC_bRunTest = TRUE;
		MGC_hTestTask = (INT8U)(MGC_UCOS_HIGHEST_PRIORITY - MGC_bUcosSystemCount);
//		err = OSTaskCreate( (void (*) (void *))MGC_TestTask, NULL, 
//						(OS_STK *)&(MGC_aTestStackData[MUSB_UCOS_TASK_STACK_SIZE-1]), 
//						MGC_hTestTask);
		OSTaskCreate(   (OS_TCB *)&MGC_TestTaskTCB,
						(CPU_CHAR *)"MGC_TestTask",
						(OS_TASK_PTR)MGC_TestTask,
						(void *) 0,
						(OS_PRIO)MGC_hTestTask,
						(CPU_STK *)&MGC_aTestStackData[0],
						(OS_STK_SIZE)(MUSB_UCOS_TASK_STACK_SIZE/10),
						(OS_STK_SIZE)MUSB_UCOS_TASK_STACK_SIZE,
						(OS_MSG_QTY)0,
						(OS_TICK)0,
						(void *)0,
						(OS_OPT)(OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR|OS_OPT_TASK_NO_TLS),
						(OS_ERR *)&err);
		if(err != OS_ERR_NONE)
			printk("Create MGC_TestTask failed\r\n");
			
		if(OS_ERR_NONE == err)
		{
			bSuccess = TRUE;
		}
	}

	return bSuccess;
}

uint8_t MUSB_SystemTestStop()
{
	MGC_bRunTest = FALSE;

	/* TODO: kill test task */

	return TRUE;
}

uint32_t MUSB_Sleep(uint32_t dwTime)
{
	OS_ERR err;
	
	uint16_t wTicks = OS_TICKS_PER_SEC * dwTime / 1000;

	//OSTimeDly(wTicks, OS_OPT_TIME_DLY, &err);
	OSTimeDly(wTicks);

	return 0;
}

#endif
