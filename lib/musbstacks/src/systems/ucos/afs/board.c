/******************************************************************
 *                                                                *
 *      Copyright Mentor Graphics Corporation 2003-2004           *
 *                                                                *
 *                All Rights Reserved.                            *
 *                                                                *
 *    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION *
 *  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS   *
 *  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                    *
 *                                                                *
 ******************************************************************/

/*
 * uC/OS target-specific code for any target supported by the ARM Firmware Suite
 * $Revision: 1.2 $
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "mu_diag.h"
#include "mu_mem.h"
#include "mu_sys.h"
#include "brd_cnf.h"
#include "board.h"

#include "timer.h"
#ifdef SYS_UCOSII
#include "ucos_ii.h"
#endif
#ifdef SYS_IAR_RTOS	 
#include "target_ucos_ii.h"
#endif

#define  OS_TMR_LINK_DLY       0
#define  OS_TMR_LINK_PERIODIC  1

HardwareTimer *Musb_Timer;

/* 
 * Define this to log diagnostics to a RAM buffer and upload later with your debugger, etc.
#define MUSB_MSG_BUF
 */

/***************************** TYPES ******************************/

/**
 * @field iVector uHAL's vector for reverse-lookup
 * @field iIndex uHAL's timer index
 * @field pfExpired expiration callback
 * @field pParam expiration callback parameter
 * @field dwTime remaining time, due to uHAL's MAX_PERIOD limitation
 * @field bPeriodic whether currently set for periodic
 */
typedef struct
{
    unsigned int iVector;
    unsigned int iIndex;
    MUSB_pfTimerExpired pfExpired;
    void* pParam;
    uint32_t dwTime;
    uint8_t bPeriodic;
} MGC_AfsTimerWrapper;

/**
 * MGC_AfsUds.
 * Board-specific UDS instance data.
 * @field pfIsr ISR
 * @field pIsrParam parameter to pass controller ISR
 * @field aTimerWrapper timer wrappers
 * @field dwIrq interrupt number
 * @field wTimerCount how many wrappers
 * @field bIndex our index into the global array
 * @field bIsPci TRUE if PCI-based; FALSE otherwise
 */
typedef struct
{
    char aIsrName[8];
    MUSB_pfUcosIsr pfIsr;
    void* pIsrParam;
    MGC_AfsTimerWrapper* aTimerWrapper;
    unsigned int dwIrq;
    uint16_t wTimerCount;
    uint8_t bIndex;
    uint8_t bIsPci;
} MGC_AfsUds;

/*************************** FORWARDS *****************************/

static void MGC_AfsUdsIsr(unsigned int dwInterruptNumber);
static void MGC_AfsTimerExpired(unsigned int iVector);

/**************************** GLOBALS *****************************/

/** since AFS doesn't allow for instance data on ISRs or timer callbacks */
static MGC_AfsUds* MGC_apAfsUds[sizeof(MUSB_aUcosController) / sizeof(MUSB_UcosController)];

/** since AFS doesn't allow for instance data on ISRs or timer callbacks */
static uint8_t MGC_bAfsUdsCount = 0;

static uint8_t MGC_bBoardInitDone = FALSE;

#define uHALr_putchar	uart0_putc
#define uHALr_getchar	uart0_getc

/*************************** FUNCTIONS ****************************/

#define HARDWARE_TIMER_STATUS_STOP			0
#define HARDWARE_TIMER_STATUS_RUN			1

void uHALr_SetTimerState(INT16U iTimerIndex, INT8U OSTmrState, MUSB_pfTimerExpired pfExpired, void* pParam)
{
	Musb_Timer->opt = OS_TMR_OPT_ONE_SHOT;
	Musb_Timer->ulStatus = OSTmrState;
	Musb_Timer->pfnHandler = pfExpired;
	Musb_Timer->pdata = pParam;
}

void uHALr_SetTimerInterval(INT16U iTimerIndex, INT32U Interval)
{
	Musb_Timer->ulIntervalTick = Interval/10;	// Time最低为10ms , Interval 单位为ms
}

INT8U uHALr_GetTimerStatus(INT16U iTimerIndex)
{
	INT8U	iTimerStatus;
	
	iTimerStatus = Musb_Timer->ulStatus;
	return iTimerStatus;
}

void uHALr_EnableTimer(INT16U iTimerIndex)
{
	StartHardwareTimer(Musb_Timer);
}

void uHALr_DisableTimer(INT16U iTimerIndex)
{
	StopHardwareTimer(Musb_Timer);
}

char MUSB_ReadConsole()
{
    char bData;

    bData = uHALr_getchar();
    /* Echo back the data entered by user */

    uHALr_putchar(bData);

    if('\r' == bData)
    {
	uHALr_putchar('\n');
    }
    return bData;
}  
       
void MUSB_WriteConsole(const char bChar) 
{
#ifdef MUSB_MSG_BUF
    MGC_MsgChar(bChar);
#else
    uHALr_putchar(bChar);
#endif
    return;
}

/* Reallocate memory */
void* MGC_AfsMemRealloc(void* pBlock, size_t iSize)
{
    /* no realloc */
    void* pNewBlock = MUSB_MemAlloc(iSize);
    if(pNewBlock)
    {
        MUSB_MemCopy(pNewBlock, pBlock, iSize);
        MUSB_MemFree(pBlock);
	  pBlock = NULL;	
    }
    return (pNewBlock);
}

uint8_t MUSB_BoardMessageString(char* pMsg, uint16_t wBufSize, const char* pString)
{
    if((strlen(pMsg) + strlen(pString)) >= wBufSize)
    {
	return FALSE;
    }
    strcat(pMsg, pString);
    return TRUE;
}

uint8_t MUSB_BoardMessageNumber(char* pMsg, uint16_t wBufSize, uint32_t dwNumber, 
			      uint8_t bBase, uint8_t bJustification)
{
    char type;
    char format[8];
    char fmt[16];
    char number[32];

    switch(bBase)
    {
    case 8:
	type = 'i';
	break;
    case 10:
	type = 'd';
	break;
    case 16:
	type = 'x';
	break;
    default:
	return FALSE;
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

    return MUSB_BoardMessageString(pMsg, wBufSize, number);
}

uint32_t MUSB_BoardGetTime()
{
    return 0L;
}

static void MGC_AfsUdsIsr(unsigned int dwInterruptNumber)
{
    uint8_t bIndex;
    MGC_AfsUds* pUds;
		
    for(bIndex = 0; bIndex < MGC_bAfsUdsCount; bIndex++)
    {
	pUds = MGC_apAfsUds[bIndex];
	if(pUds)
	{
	    /*uHALr_DisableInterrupt(pUds->dwIrq);*/
#ifdef MUSB_MSG_BUF
	    MUSB_DIAG1(3, "[[[ Calling ISR for ", dwInterruptNumber, 10, 0);
#endif
	    OSIntEnter();
	    pUds->pfIsr(pUds->pIsrParam);
	    OSIntExit();
#ifdef MUSB_MSG_BUF
	    MUSB_DIAG_STRING(3, "ISR return ]]]");
#endif
	    /*uHALr_EnableInterrupt(pUds->dwIrq);*/
	    return;
	}
    }
}

// USB 0
static void MGC_AfsUdsIsr_usb_0(void)
{
	MGC_AfsUdsIsr (USB_INT);
}

#ifdef _USB_1_SUPPORT_
static void MGC_AfsUdsIsr_usb_1(void)
{
	MGC_AfsUdsIsr (USB1_INT);
}
#endif

void MUSB_BoardInit()
{
    MUSB_MemSet(MGC_apAfsUds, 0, sizeof(MGC_apAfsUds));

    MUSB_PRINTK("\n\r\n\r MUSBStack-S v2.0 on uC/OS-II from ARM Firmware Suite\n\r");
    MUSB_PRINTK("Mentor Graphics Inventra Division \n\r");

#ifdef MUSB_MSG_BUF
    MGC_pMsgBuf = MUSB_MemAlloc(MGC_iMsgBufSize);
#endif

    MGC_bBoardInitDone = TRUE;
}

void* MUSB_BoardInitController(void* pPrivateData, MUSB_pfUcosIsr pfIsr,
			       const MUSB_UcosController* pControllerInfo,
			       uint8_t** ppBaseIsr, uint8_t** ppBaseBsr,
			       uint8_t** ppPciAck)
{
    MGC_AfsUds* pUds;

    if(!MGC_bBoardInitDone)
    {
	MUSB_BoardInit();
    }

    pUds = (MGC_AfsUds*)MUSB_MemAlloc(sizeof(MGC_AfsUds));
    if(!pUds)
    {
	/* no memory */
	return NULL;
    }
    MUSB_MemSet(pUds, 0, sizeof(MGC_AfsUds));

    pUds->dwIrq = pControllerInfo->dwInfo;
    pUds->pfIsr = pfIsr;
    pUds->pIsrParam = pPrivateData;

#ifndef MUSB_SYSTEM_TEST
    irq_disable(pUds->dwIrq);

    /* assign the interrupt */
    strcpy(pUds->aIsrName, "MUSB-");
    pUds->aIsrName[5] = '0' + MGC_bAfsUdsCount;
    pUds->aIsrName[6] = (char)0;

    //request_irq(pUds->dwIrq, 15, MGC_AfsUdsIsr);
	 if(pUds->dwIrq == USB_INT)
	 {
		 printf ("usb_0 installed\n");
		 request_irq(pUds->dwIrq, 15, MGC_AfsUdsIsr_usb_0);
	 }
#ifdef _USB_1_SUPPORT_
	 else
	 {
		 printf ("usb_1 installed\n");
		 request_irq(pUds->dwIrq, 15, MGC_AfsUdsIsr_usb_1);
	 }
#endif
    irq_enable(pUds->dwIrq);
#endif

    pUds->bIndex = MGC_bAfsUdsCount;
    MGC_apAfsUds[MGC_bAfsUdsCount++] = pUds;
    return pUds;
}

uint8_t MUSB_BoardInitTimers(void* pPrivateData, uint16_t wTimerCount, 
			     const uint32_t* adwTimerResolutions)
{
    
    int iTimerInterval;
    int iTimerState;
    unsigned int iTimerCount;
    unsigned int iTimerIndex;
    int iIndex;
    unsigned int iTimerAvail = 0;
    MGC_AfsUds* pUds = (MGC_AfsUds*)pPrivateData;

    pUds->aTimerWrapper = (MGC_AfsTimerWrapper*)MUSB_MemAlloc(wTimerCount * 
	sizeof(MGC_AfsTimerWrapper));
    if(!pUds->aTimerWrapper)
    {
	/* no memory */
	return FALSE;
    }
	
    /* allocate timers now */
    for(iTimerIndex = 0; iTimerIndex < wTimerCount; iTimerIndex++)
    {
	MGC_AfsTimerWrapper* pWrapper;
	INT8U           perr;
	pWrapper = &(pUds->aTimerWrapper[iTimerIndex]);	

	//这里暂时只处理一个定时器，系统暂时也只需要一个定时器
	Musb_Timer = NewHardwareTimer(pWrapper->dwTime, pWrapper->pParam, pWrapper->pfExpired);

	if(Musb_Timer)
	{
		MUSB_PRINTK("====MUSB_BoardInitTimers Musb_Timer is 0x%x====\n", Musb_Timer);
		AddHardwareTimer(Musb_Timer);
	}
    }

    pUds->wTimerCount = wTimerCount;
    return TRUE;
}

void MUSB_BoardDestroyController(void* pPrivateData)
{
    MGC_AfsUds* pUds = (MGC_AfsUds*)pPrivateData;

#ifndef MUSB_SYSTEM_TEST
    irq_disable(pUds->dwIrq);
#endif

    /* TODO: timers? */

    MGC_apAfsUds[pUds->bIndex] = NULL;
    MUSB_MemFree(pPrivateData);
    pPrivateData = NULL;		
}

#define mSEC_1	1

uint8_t MUSB_BoardArmTimer(void* pPrivateData, uint16_t wIndex, 
			     uint32_t dwTime, uint8_t bPeriodic, 
			     MUSB_pfTimerExpired pfExpireCallback,
			     void* pParam)
{
    int status;
    unsigned int dwInterval;
    MGC_AfsUds* pUds = (MGC_AfsUds*)pPrivateData;
    MGC_AfsTimerWrapper* pWrapper = &(pUds->aTimerWrapper[wIndex]);
    unsigned int iIndex = pWrapper->iIndex;

    pWrapper->pParam = pParam;
    pWrapper->pfExpired = pfExpireCallback;
    pWrapper->dwTime = mSEC_1 * dwTime;
    pWrapper->bPeriodic = bPeriodic;

    /*uHALr_SetTimerState(iIndex, pWrapper->bPeriodic ? T_INTERVAL : T_ONESHOT);*/
    /* T_ONESHOT does NOT work */

    MUSB_PRINTK("====MUSB_BoardArmTimer====dwTime is 0x%x bPeriodic is 0x%x\n", pWrapper->dwTime, pWrapper->bPeriodic);
	 // 20131205 ZhuoYongHong
	 // 首先停止定时器 
	 uHALr_DisableTimer(iIndex);
	
    uHALr_SetTimerState(iIndex, HARDWARE_TIMER_STATUS_STOP, pWrapper->pfExpired, pWrapper->pParam);

    uHALr_SetTimerInterval(iIndex, pWrapper->dwTime);

    uHALr_EnableTimer(iIndex);
    return TRUE;
}

uint8_t MUSB_BoardCancelTimer(void* pPrivate, uint16_t wIndex)
{
    MGC_AfsUds* pUds = (MGC_AfsUds*)pPrivate;
    MGC_AfsTimerWrapper* pWrapper = &(pUds->aTimerWrapper[wIndex]);
    unsigned int iIndex = pWrapper->iIndex;

    uHALr_DisableTimer(iIndex);
    pWrapper->pfExpired = NULL;

    return TRUE;
}

/*
* Controller calls this to print a diagnostic message
*/
uint8_t MUSB_BoardPrintDiag(void* pPrivate, const char* pMessage)
{
    MUSB_PRINTK("%s\n", pMessage);

    return TRUE;
}

/*
* Controller calls this to get a bus address (for DMA) from a system address
*/
void* MUSB_BoardSystemToBusAddress(void* pPrivate, const void* pSysAddr)
{
    MGC_AfsUds* pUds = (MGC_AfsUds*)pPrivate;

    if(pUds->bIsPci)
    {
	return (void*)((uint8_t*)pSysAddr + 0);		//MUSB_PCIMASTER_OFFSET
    }
    else
    {
	return (void*)((uint8_t*)pSysAddr + 0);		//MUSB_AHBMASTER_OFFSET
    }
}

void usb_host_isr (void)
{
	MGC_AfsUdsIsr (USB_INT);
}
