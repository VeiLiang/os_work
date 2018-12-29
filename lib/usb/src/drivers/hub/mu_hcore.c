/*****************************************************************************
 *                                                                           *
 *      Copyright Mentor Graphics Corporation 2003-2005                      *
 *                                                                           *
 *                All Rights Reserved.                                       *
 *                                                                           *
 *    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION            *
 *  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS              *
 *  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                               *
 *                                                                           *
 ****************************************************************************/

/*
 * Hub and port status change handling
 * $Revision: 1.2 $
 */

#include "mu_bits.h"
#include "mu_cdi.h"
#include "mu_descs.h"
#include "mu_diag.h"
#include "mu_mem.h"

#include "mu_hub.h"

#include "mu_hdef.h"
#include "mu_hcore.h"
#include "mu_hcreq.h"

/*******************************************************************
 *                 MUSB_HubStatusChange ()                         *
 *******************************************************************/
void 
MUSB_HubStatusChange (MUSB_Hub *pHubDevice)
{
	uint32_t    dwStatus;

	MUSB_PRINTK("MUSB_HubStatusChange() >>\n");
	/* Get hub status from the connected Hub. */
	pHubDevice->bNextState = MUSB_HUB_STATE_GOT_HUB_STATUS;
	dwStatus = MUSB_HubGetHubStatus (pHubDevice, MGC_HubGetHubStatusCallback);  
	return;
}   /* End of function MUSB_HubStatusChange () */

 

/*******************************************************************                        
 *                 MGC_HubGetHubStatusCallback ()                  *
 *******************************************************************/
void
MGC_HubGetHubStatusCallback (void *pContext, MUSB_ControlIrp *pControlIrp)
{
	MUSB_Hub        *pHubDevice;
	uint32_t        dwStatus;

	pHubDevice   = (MUSB_Hub *)pContext;

	if (MUSB_SUCCESS != pControlIrp->dwStatus  )
	{
		/* This is error condition so check which error has come and 
		* do the things as per requirement.
		*/
		MUSB_DIAG_STRING(1,"HubError: pControlIrp->dwStatus in MGC_HubGetHubStatusCallback()");
		MUSB_DIAG1(2,"HubState:", pHubDevice->bState, 16, 0x00); 
		MUSB_DIAG1(1,"IrpStatus:", pControlIrp->dwStatus, 16, 0x00); 
		return;
	}

	/* Since IRP is executed properly hence update hub state to its next state.
	*/
	pHubDevice->bState  =  pHubDevice->bNextState;

	MUSB_DIAG1(2,"HubState:",pHubDevice->bState, 16, 0x00); 

	if ( MUSB_HUB_STATE_GOT_HUB_STATUS == pHubDevice->bState)
	{
		/* If control comes here it means Status is availble. Store it into proper
		* place. 
		*/
		MGC_HubStoreStatus (pHubDevice->aHubPortStatus, &(pHubDevice->hubStatus));  

		if (pHubDevice->hubStatus.wStatusChange & MUSB_HUB_LOCAL_POWER_STATUS_CHANGED_BM)
		{
			/* This function sends clear feature requests and make 
			* pHubNextState to MUSB_HUB_STATE_C_LOCAL_POWER_COMPLETED.
			*/
			MUSB_DIAG_STRING(2, "HUB: LocalPowerStatusChange");
			pHubDevice->bNextState = MUSB_HUB_STATE_C_LOCAL_POWER_COMPLETED;   
			dwStatus = MUSB_HubClearHubFeature (pHubDevice, MUSB_C_HUB_LOCAL_POWER_FEATURE, 
				MGC_HubGetHubStatusCallback);
			return;
		}
		else
		{
			/* Since there is no change in local power supply bm hence go ahead and
			* check for over current condition by making present state 
			* MUSB_HUB_STATE_C_LOCAL_POWER_COMPELETED.
			*/
			pHubDevice->bState = MUSB_HUB_STATE_C_LOCAL_POWER_COMPLETED;
		}
	}   /* End of if(MUSB_HUB_STATE_GOT_HUB_STATUS) */

	if ( MUSB_HUB_STATE_C_LOCAL_POWER_COMPLETED == pHubDevice->bState)
	{
		/* Control comes here it means check as well as remedy for local
		* power supply has been done. Now time to check over current condition.
		*/
		if (pHubDevice->hubStatus.wStatusChange & MUSB_HUB_OVER_CURRENT_STATUS_CHANGED_BM)
		{
			/* Host recovery action for an over current event: As per 11.12.5 USB
			* 2.0 Specification.
			*/
			MUSB_DIAG_STRING(1, "HUB: OverCurrent status change");
			pHubDevice->bNextState = MUSB_HUB_STATE_CHECK_OVER_CURRENT_EXISTANCE;
			/* First clear the over current status change bit. */
			dwStatus = MUSB_HubClearHubFeature (pHubDevice, MUSB_C_HUB_OVER_CURRENT_FEATURE, 
				MGC_HubGetHubStatusCallback); 
		}
		else
		{
			/* If over current status has not changed then checking status of 
			* HUB is over go ahead check status for all ports one by one.
			*/
			MUSB_DIAG_STRING(2, "HUB: Hub status change processing finished");
			pHubDevice->bState     = MUSB_HUB_STATE_INIT;
			pHubDevice->bNextState = MUSB_HUB_STATE_INIT;
			pHubDevice->bActivePortNum = MUSB_HUB_STARTING_PORT_NUMBER;
			MGC_HubPortStatusChange(pHubDevice);
		return;
		}   /* End of else  */
	}   /* End of if(MUSB_HUB_STATE_C_LOCAL_POWER_COMPLETED) */

	if ( MUSB_HUB_STATE_CHECK_OVER_CURRENT_EXISTANCE == pHubDevice->bState)
	{
		if (pHubDevice->hubStatus.wStatus & MUSB_HUB_OVER_CURRENT_EXISTS_BM)
		{
			/* Wait for 400 ms */
			/* This function call system timer function with proper callback.
			*/
			MUSB_DIAG_STRING(1, "HUB: OverCurrent exists");
			pHubDevice->bNextState = MUSB_HUB_STATE_C_HUB_OVER_CURRENT;
			MGC_HubHandleHubOverCurrentTimer (pHubDevice, 400);
			return;
		}
		else
		{    
			MUSB_DIAG_STRING(2, "HUB: OverCurrent doesn't exist");
			pHubDevice->bState     = MUSB_HUB_STATE_INIT;
			pHubDevice->bNextState = MUSB_HUB_STATE_INIT;
			pHubDevice->bActivePortNum = MUSB_HUB_STARTING_PORT_NUMBER;
			MGC_HubPortStatusChange(pHubDevice);
			return;
		}
	}

	if (MUSB_HUB_STATE_C_HUB_OVER_CURRENT  == pHubDevice->bState)
	{
		/* Since over current exist and clear feature for overcurrent 
		* is already done. Now its time to power on all the ports
		* again after waiting for few ms.
		*/
		/* Start from port 0x01  */
		pHubDevice->bActivePortNum = MUSB_HUB_STARTING_PORT_NUMBER;
		MGC_HubHandlePowerOnPorts(pHubDevice, &(pHubDevice->bNextState));
		return;
	}

	if ( MUSB_HUB_STATE_POWER_ON == pHubDevice->bState)
	{
		/* If control comes here it means PORT_POWER_FEATURE for port
		* reperesented by bActivePortNum is set and do same thing for 
		* next port. Hub will remain in this state till 
		* pHubDevice->bActivePortNum == bNumOfPort.
		*/
		pHubDevice->bActivePortNum++;

		if (pHubDevice->bActivePortNum < pHubDevice->hubDscr.bNbrPorts)
		{
			pHubDevice->bNextState = MUSB_HUB_STATE_POWER_ON;                
		}
		else
		{
			/* This request is going for last port in the hub. */
			pHubDevice->bNextState = MUSB_HUB_STATE_POWER_ON_COMPLETE;
		}

		dwStatus = MUSB_HubSetPortFeature(pHubDevice, 
						MUSB_HUB_PORT_POWER_FEATURE,
						MGC_HubGetHubStatusCallback,
						pHubDevice->bActivePortNum);
		return;
	}

	if (MUSB_HUB_STATE_POWER_ON_COMPLETE == pHubDevice->bState)
	{
		/* Call callbackWait for (bPwrOn2PwrGood *2) msecond  so that power on ports will 
		* be good. As per 11.23.2.1 bPwrOn2PwrGood
		*/
		pHubDevice->bState = MUSB_HUB_STATE_INIT;
		pHubDevice->bNextState = MUSB_HUB_STATE_INIT;
		MGC_HubPowerOnGoodTimer (pHubDevice, (uint32_t)((pHubDevice->hubDscr.bPwrOn2PwrGood) * 2));
		return;
	}

	if (MUSB_HUB_STATE_INIT == pHubDevice->bState)
	{
		/* Hub Status Has changed and its time to go for port status. 
		*/
		MUSB_DIAG_STRING(2,"HUB: Hub status change processing finish 1");
		pHubDevice->bActivePortNum = MUSB_HUB_STARTING_PORT_NUMBER;
		MGC_HubPortStatusChange(pHubDevice);
	}

	return;
}   /* End of function  MGC_HubGetHubStatusCallback() */



/*******************************************************************************
 *              MGC_HubHandleHubOverCurrentTimer ()                            *
 *******************************************************************************/

void 
MGC_HubHandleHubOverCurrentTimer (MUSB_Hub *pHubDevice, uint32_t dwWaitingTime)
{
	uint32_t dwStatus; 
	MUSB_DIAG_STRING(2,"HUB: MGC_HubHandleHubOverCurrentTimer()"); 

	dwStatus = MUSB_ArmTimer(pHubDevice->busHandle, 
					pHubDevice->pDriver, 
					pHubDevice->bIndexAtHubDeviceList,
					dwWaitingTime, 
					MGC_HubOverCurrentTimerCallback, 
					(void *)pHubDevice);

	return;
}   /* End of function  MGC_HubHandleHubOverCurrentTimer() */


/*****************************************************************************
 *              MGC_HubPortDebounceTimerCallback ()                          *
 *****************************************************************************/
void 
MGC_HubOverCurrentTimerCallback (void *pParam, MUSB_BusHandle hBus)
{
	MUSB_Hub                *pHubDevice;

	pHubDevice = (MUSB_Hub *)pParam;

	MUSB_DIAG_STRING(2,"HUB: MGC_HubOverCurrentTimerCallback()"); 

	/* Now againt control should go to the HubStatusChangeCallBack  */
	MGC_HubGetHubStatusCallback (pParam, &(pHubDevice->ctrlIrp));
	return;
}   /* End of function  MGC_HubOverCurrentTimerCallback()  */


/*******************************************************************************
 *              MGC_HubPowerOnGoodTimer ()                                    *
 *******************************************************************************/

void 
MGC_HubPowerOnGoodTimer (MUSB_Hub *pHubDevice, uint32_t dwWaitingTime)
{
	uint32_t dwStatus; 

	MUSB_DIAG_STRING(2,"HUB: MGC_HubPowerOnGoodTimer()"); 
	dwStatus = MUSB_ArmTimer(pHubDevice->busHandle, 
					pHubDevice->pDriver, 
					pHubDevice->bIndexAtHubDeviceList,
					dwWaitingTime, 
					MGC_HubPowerOnGoodTimerCallback, 
					(void *)pHubDevice);
	return;
}   /* End of function MGC_HubPowerOnGoodtTimer () */



/*****************************************************************************
 *              MGC_HubPowerOnGoodTimerCallback ()                           *
 *****************************************************************************/
void 
MGC_HubPowerOnGoodTimerCallback (void *pParam, MUSB_BusHandle hBus)
{
	MUSB_Hub                *pHubDevice;

	pHubDevice = (MUSB_Hub *)pParam;

	MUSB_DIAG_STRING(2,"HUB: MGC_HubPowerOnGoodTimerCallback()"); 
	pHubDevice->bActivePortNum = MUSB_HUB_STARTING_PORT_NUMBER;
	MGC_HubPortStatusChange(pHubDevice);
	return;
}   /* End of function  MGC_HubPowerOnGoodTimerCallback() */


/*******************************************************************
 *                 MGC_HubHandlePowerOnPorts ()                    *
 *******************************************************************/

void 
MGC_HubHandlePowerOnPorts(MUSB_Hub *pHubDevice, uint8_t *pHubNextState)
{
	uint32_t dwStatus;
	uint8_t  bPortNumber = pHubDevice->bActivePortNum;

	//Nicholas Xu
#if 0
	if ( MUSB_HUB_GANGED_POWER_SWITCHING ==
		(pHubDevice->hubDscr.wHubCharacteristics & MUSB_HUB_LOGICAL_POWER_SWITCH_MODE_BM))
	{
		/* bActivePort make it to MUSB_HUB_INVALID_PORT_NUMBER because
		* request for one port is sufficient in case ganged power. 
		* so make port number for next request is Invalid.
		*/
		*pHubNextState = MUSB_HUB_STATE_POWER_ON_COMPLETE;                
		pHubDevice->bActivePortNum = MUSB_HUB_INVALID_PORT_NUMBER;
	}
	else
	{    
		/* Next state will remain POWER_ON till request is not sent to last port. */
		*pHubNextState = MUSB_HUB_STATE_POWER_ON;                
	}
#else
	/* Next state will remain POWER_ON till request is not sent to last port. */
	*pHubNextState = MUSB_HUB_STATE_POWER_ON;     
#endif

	dwStatus = MUSB_HubSetPortFeature(pHubDevice, 
				MUSB_HUB_PORT_POWER_FEATURE,
				MGC_HubGetHubStatusCallback,
				bPortNumber);


	if (MUSB_FAILURE == dwStatus)
	{
		MUSB_DIAG_STRING(1, "HubError: MGC_HubHandlePowerOnPorts() ");
		MUSB_DIAG1(2, "HubState: ",pHubDevice->bState, 16, 0x00);
	}
	return;
}  /* End of function  MGC_HubHandlePowerOnPorts () */


/**********************Function related to PORT status start. ****************/


/*****************************************************************************
 *              MGC_HubPortStatusChange ()                                   *
 *****************************************************************************/
void 
MGC_HubPortStatusChange(MUSB_Hub *pHubDevice)
{
	uint8_t  bNumOfPorts = pHubDevice->hubDscr.bNbrPorts;
	uint32_t dwStatus;

	/* This function expected that bActivePortNum should have  proper value
	* either initialized or increamented before calling the function.
	*/ 
	MUSB_DIAG2(2, "Hub: Port status change: PortState=", pHubDevice->bPortState, " / Port=", 
		pHubDevice->bActivePortNum, 16, 0x00); 

	while (pHubDevice->bActivePortNum <= bNumOfPorts) 
	{
		if (pHubDevice->dwHubPortStatusChange & (1 << pHubDevice->bActivePortNum))
		{
			/* If ports status changed then go ahead and ask for ports status. */
			pHubDevice->bPortNextState =  MUSB_HUB_PORT_STATE_GOT_STATUS;
			dwStatus = MUSB_HubGetPortStatus (pHubDevice, MGC_HubGetPortStatusCallback,
								pHubDevice->bActivePortNum);
			if (MUSB_SUCCESS != dwStatus)
			{
				/* Todo : What to do other then displaying error message. 
				*/
				MUSB_DIAG_STRING(1, "HubError: MGC_HubPortStatusChange() ");
				MUSB_DIAG1(2,"PortState: ",pHubDevice->bPortState, 16, 0x00);
			}
			/* Either error or success return from here because getting status for
			* this port is called. If it returns success then comeout from the loop
			* even error comes then no point of calling same function for other ports
			* so return from here.
			*/
			return;
		}
		else
		{
			/* since ports status has not changed hence go to next port by increamenting
			* one.
			*/
			pHubDevice->bActivePortNum++;
		}
	}   /* End of while loop */
	return;
}   /* End of function MGC_HubPortStatusChange () */



/**************************************************************************
 *              MGC_HubGetPortStatusCallback ()                           *
 **************************************************************************/

void
MGC_HubGetPortStatusCallback (void *pContext, MUSB_ControlIrp *pControlIrp)
{
	MUSB_Hub            *pHubDevice;
	uint8_t             *pPortNextState;
	MUSB_HubPortStatus  *pPortStatus;
	uint32_t             dwStatus;

	pHubDevice      =  (MUSB_Hub *)pContext;
	pPortStatus     =  &(pHubDevice->portStatus);

	if (pControlIrp->dwStatus  != MUSB_SUCCESS)
	{
		MUSB_DIAG_STRING(1,"HubError: pControlIrp->dwStatus in MGC_HubGetPortStatusCallback()");
		MUSB_DIAG1(2,"PortState: ",pHubDevice->bPortState, 16, 0x00);
		MUSB_DIAG1(1,"ControlIrpStatus: ", pControlIrp->dwStatus, 16, 0x00); 
		return;
	}

	/* Since IRP is executed properly hence update port state to its next port state.
	*/
	pHubDevice->bPortState = pHubDevice->bPortNextState;
	pPortNextState = &(pHubDevice->bPortNextState);

	/* If bPortStatus is MUSB_HUB_PORT_STATE_GOT_STATUS then 
	* 1. Store the Port status in proper format. 
	* 2. Check Connect status has changed or not. If connect status
	*    changed then check device is connected or disconnected and
	*    take action acording to that.
	* 3. If connection status bit not changed then make port state
	*    to MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE so that next 
	*    bit of status change can be addressed.
	*/
	if (MUSB_HUB_PORT_STATE_GOT_STATUS == pHubDevice->bPortState)
	{
		MGC_HubStoreStatus (pHubDevice->aHubPortStatus, pPortStatus);

		if (pPortStatus->wStatusChange & MUSB_HUB_C_PORT_CONNECTION_BM)
		{
			MUSB_DIAG_STRING(2,"HubPort: ConnectionStatusChanged");
			if ( pPortStatus->wStatus & MUSB_HUB_PORT_CONNECTION_BM)
			{
				/*
				* Device is connected to the port so handle that device. 
				*/
				MUSB_DIAG_STRING(2,"HubPort: NewDeviceConnected");
				MGC_HubHandlePortConnect (pHubDevice, pPortNextState, pHubDevice->bActivePortNum);
				return;
			}
			else
			{
				/*
				* Device is Disconnected from the port so handle device disconnect.
				*/
				/*
				* Update the power availabe and 
				* it always returns success in case of diconnection
				*/
				MUSB_DIAG_STRING(2,"HubPort: DeviceDisconnected");
				dwStatus = MGC_HubUpdatePower(pHubDevice, MUSB_HUB_PORT_DISCONNECTED);
				*pPortNextState = MUSB_HUB_PORT_STATE_DISCONNECT;    

				dwStatus = MUSB_HubClearPortFeature(pHubDevice, 
									MUSB_HUB_C_PORT_CONNECTION_FEATURE, 
									MGC_HubGetPortStatusCallback, 
									pHubDevice->bActivePortNum);
				return;
			}
		}
		else
		{
			/* Note: Here Don't return  let control fall to next if statement. */
			pHubDevice->bPortState = MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE;
		}
	} /* End of if (MUSB_HUB_PORT_STATE_GOT_STATUS)*/


	if ( MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE  == pHubDevice->bPortState)
	{
		if (pPortStatus->wStatusChange & MUSB_HUB_C_PORT_ENABLE_BM)
		{
			MUSB_DIAG_STRING(2,"HubPort: EnableStatusChanged");
			MGC_HubHandleEnablePort (pHubDevice, pPortNextState, pHubDevice->bActivePortNum);
			return;
		}
		else
		{
			pHubDevice->bPortState = MUSB_HUB_PORT_STATE_C_ENABLE_COMPLETE;
			MUSB_DIAG1(2,"PortState: ",pHubDevice->bPortState, 16, 0x00);
		}
	}   /* End of if (MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE)*/


	if ( MUSB_HUB_PORT_STATE_C_ENABLE_COMPLETE == pHubDevice->bPortState)
	{
		if (pPortStatus->wStatusChange & MUSB_HUB_C_PORT_SUSPEND_BM)
		{
			MUSB_DIAG_STRING(2,"HubPort: SuspendStatusChanged");
			if ( pPortStatus->wStatus & MUSB_HUB_PORT_SUSPEND_BM)
			{
				/* Port Suspended, Update the power availabe and 
				* it always returns success in case of resuming
				*/
				dwStatus = MGC_HubUpdatePower(pHubDevice, MUSB_HUB_PORT_SUSPENDED);   
			}
			else
			{
				/* Port resumed, Update the power availabe and 
				* it always returns success in case of resuming
				*/
				dwStatus = MGC_HubUpdatePower(pHubDevice, MUSB_HUB_PORT_RESUMED);
			}

			MGC_HubHandleSuspendPort (pHubDevice, pPortNextState, pHubDevice->bActivePortNum);
			return;
		}
		else                     
		{
			pHubDevice->bPortState = MUSB_HUB_PORT_STATE_C_SUSPEND_COMPLETE;
		}
	}   /* End of if (MUSB_HUB_PORT_STATE_C_ENABLE_COMPLETE)*/
	

	if ( MUSB_HUB_PORT_STATE_C_SUSPEND_COMPLETE == pHubDevice->bPortState)
	{
		if (pPortStatus->wStatusChange & MUSB_HUB_C_PORT_OVER_CURRENT_BM)
		{
			MUSB_DIAG_STRING(2,"HubPort: OverCurrent status changed");
			if (pPortStatus->wStatus & MUSB_HUB_PORT_OVER_CURRENT_BM)
			{
				MUSB_DIAG_STRING(2,"HubPort: OverCurrent exists");
				*pPortNextState = MUSB_HUB_PORT_STATE_C_OVERCURRENT; 
			}
			else
			{   
				MUSB_DIAG_STRING(2,"HubPort: OverCurrent doesn't exist"); 
				*pPortNextState = MUSB_HUB_PORT_STATE_C_OVERCURRENT_COMPLETE; 
			}

			dwStatus = MUSB_HubClearPortFeature(pHubDevice, MUSB_HUB_C_PORT_OVER_CURRENT_FEATURE, 
									MGC_HubGetPortStatusCallback, pHubDevice->bActivePortNum);
			return;
		}
		else
		{
			/* Since over current condition is not there hence make state as complete
			* and go ahead to check next condition.
			*/
			pHubDevice->bPortState = MUSB_HUB_PORT_STATE_C_OVERCURRENT_COMPLETE;
		}
	}   /* End of if (MUSB_HUB_PORT_STATE_C_SUSPEND_COMPLETE)*/


	if ( MUSB_HUB_PORT_STATE_C_OVERCURRENT == pHubDevice->bPortState)
	{
		/* Doing POWER ON for the state. */
		*pPortNextState = MUSB_HUB_PORT_STATE_C_OVERCURRENT_CONTINUE;
		dwStatus = MUSB_HubSetPortFeature (pHubDevice, 
							MUSB_HUB_PORT_POWER_FEATURE,
							MGC_HubGetPortStatusCallback, 
							pHubDevice->bActivePortNum);
		return;
	}   /* End of if (MUSB_HUB_PORT_STATE_C_OVERCURRENT)*/



	if ( MUSB_HUB_PORT_STATE_C_OVERCURRENT_CONTINUE == pHubDevice->bPortState)
	{
		/* */
		MGC_HubOverCurrentPortTimer (pHubDevice,(uint32_t)((pHubDevice->hubDscr.bPwrOn2PwrGood) * 2));
		return;
	}  /* End of if (MUSB_HUB_PORT_STATE_C_OVERCURRENT)*/


	if ( MUSB_HUB_PORT_STATE_C_OVERCURRENT_COMPLETE == pHubDevice->bPortState)
	{
		if (pPortStatus->wStatusChange & MUSB_HUB_C_PORT_RESET_BM)
		{
			MUSB_DIAG_STRING(2,"HubPort: Reset state changed");
			*pPortNextState = MUSB_HUB_PORT_STATE_C_RESET_COMPLETE;

			dwStatus = MUSB_HubClearPortFeature(pHubDevice, MUSB_HUB_C_PORT_RESET_FEATURE, 
								MGC_HubGetPortStatusCallback, pHubDevice->bActivePortNum);
			return;
		}
		else
		{
			pHubDevice->bPortState  = MUSB_HUB_PORT_STATE_C_RESET_COMPLETE;
		}
	}   /* End of if (MUSB_HUB_PORT_STATE_C_OVERCURRENT_COMPLETE) */


	if ( MUSB_HUB_PORT_STATE_C_RESET_COMPLETE == pHubDevice->bPortState)
	{
		/* All the status changes are taken care and now time to go for
		* next port.
		*/
		pHubDevice->bPortState     = MUSB_HUB_PORT_STATE_DEFAULT;
		*pPortNextState = MUSB_HUB_PORT_STATE_DEFAULT;

		pHubDevice->bActivePortNum++;
		MGC_HubPortStatusChange (pHubDevice);
		return;
	}   /* End of if (MUSB_HUB_PORT_STATE_C_RESET_COMPLETE) */


	if ( MUSB_HUB_PORT_STATE_DISCONNECT == pHubDevice->bPortState)
	{
		/* All the status changes are taken care and now time to go for
		* next port.
		*/
		pHubDevice->bPortState     = MUSB_HUB_PORT_STATE_DEFAULT;
		*pPortNextState = MUSB_HUB_PORT_STATE_DEFAULT;
		MGC_HubHandlePortDisconnect (pHubDevice, pHubDevice->bActivePortNum);

		pHubDevice->bPortState     = MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE;
		pHubDevice->bPortNextState = MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE;
		pHubDevice->ctrlIrp.dwStatus = 0x00;
		MGC_HubGetPortStatusCallback ((void *)pHubDevice, &(pHubDevice->ctrlIrp));
		return;
	}

	return;
}   /* End of function  MGC_HubGetPortStatusCallback ()  */


/*******************************************************************
 *                 MGC_HubHandlePortConnect ()                     *
 *******************************************************************/
void
MGC_HubHandlePortConnect (MUSB_Hub *pHubDevice, uint8_t *pPortNextState, uint8_t bPortNum)
{
	uint32_t dwStatus;

	pHubDevice->debounceParam.dwWaitingTime  = MUSB_HUB_MIN_DEBOUNCE_TIME;
	pHubDevice->debounceParam.bErrorCount    = 0x00;

	MUSB_DIAG_STRING(1, "MGC_HubHandlePortConnect()");
	MUSB_PRINTK("===MGC_HubHandlePortConnect()===\n");
	
	if( pHubDevice->bAllocatedPower >= MUSB_HUB_BUS_POWER_MAX_UNITS )
	{
		/* Power is not available */
		MUSB_DIAG_STRING(1, "HubError: Cannot supply load to device");
		MUSB_DIAG_STRING(1, "HubError: Please disconnect device");
		*pPortNextState = MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE;
	}
	else
	{
		*pPortNextState = MUSB_HUB_PORT_STATE_WAIT_FOR_DEBOUNCE;   
	}

	dwStatus = MUSB_HubClearPortFeature(pHubDevice, MUSB_HUB_C_PORT_CONNECTION_FEATURE, 
								MGC_HubPortDebounceCallback, bPortNum);

	if (dwStatus != MUSB_SUCCESS)
	{
		MUSB_DIAG_STRING(1,"HubError: MGC_HubHandlePortConnect()");
		MUSB_DIAG1(2,"PortState: ",pHubDevice->bPortState, 16, 0x00);
	}
}   /* End of function MGC_HubHandleConnectPort () */


/*******************************************************************************
 *              MGC_HubPortDebounceCallback ()                                 *
 *******************************************************************************/

void 
MGC_HubPortDebounceCallback (void *pContext, MUSB_ControlIrp *pControlIrp)
{
	uint32_t dwStatus;
	MUSB_Hub                *pHubDevice;
	MUSB_HubPortStatus      *pDebouncePortStatus;
	MUSB_PortDebounceParam  *pDebounceParam;

	pHubDevice          =  (MUSB_Hub *)pContext;
	pDebounceParam      = &(pHubDevice->debounceParam);
	pDebouncePortStatus = &(pDebounceParam->debouncePortStatus);

	if (pControlIrp->dwStatus != MUSB_SUCCESS)
	{
		MUSB_DIAG_STRING(1,"HubError: pControlIrp->dwStatus in MGC_HubPortDebounceCallback()");
		MUSB_DIAG1(2,"PortState: ",pHubDevice->bPortState, 16, 0x00);
		MUSB_DIAG1(1,"ControlIrpStatus: ", pControlIrp->dwStatus, 16, 0x00);  
		return;
	}

	/* Since IRP is executed properly hence update port state to its next port state.
	*/
	pHubDevice->bPortState = pHubDevice->bPortNextState;


	if (MUSB_HUB_PORT_STATE_WAIT_FOR_DEBOUNCE == pHubDevice->bPortState)
	{
		pHubDevice->bPortNextState = MUSB_HUB_PORT_STATE_DEBOUNCE_GOT_STATUS;
		MGC_HubHandlePortDebounceTimer (pHubDevice, pDebounceParam->dwWaitingTime);
		return;
	}

	if (MUSB_HUB_PORT_STATE_DEBOUNCE_GOT_STATUS == pHubDevice->bPortState)
	{
		/* Store Status into proper format. */
		MGC_HubStoreStatus (pHubDevice->aHubPortStatus, pDebouncePortStatus);

		if (pDebouncePortStatus->wStatus & MUSB_HUB_PORT_CONNECTION_BM)
		{  
			if (pDebounceParam->dwWaitingTime <= MUSB_HUB_MAX_DEBOUNCE_TIME)
			{
				/* Connection is still there. Then wait for some more time
				*/
				pHubDevice->bPortNextState = MUSB_HUB_PORT_STATE_DEBOUNCE_GOT_STATUS;
				MGC_HubHandlePortDebounceTimer (pHubDevice, pDebounceParam->dwWaitingTime);
				return;
			}
			else
			{
				/* Debounce waiting time is over and still connection is there
				* hence go ahead and start RESET by calling reset callback. 
				*/
				MUSB_DIAG_STRING(3,"HubSuccess: Debounce complete");
				pHubDevice->bPortNextState = MUSB_HUB_PORT_STATE_START_RESET;
				pHubDevice->ctrlIrp.dwStatus = 0x00;
				pHubDevice->ctrlIrp.pfIrpComplete = MGC_HubPortResetCallback;
				MUSB_HubPrepareSetupPkt (&(pHubDevice->setupPacket), 
									(uint8_t)(MUSB_TYPE_CLASS | MUSB_RECIP_OTHER), MUSB_REQ_SET_FEATURE, 
									MUSB_HUB_PORT_RESET_FEATURE, (uint16_t)pHubDevice->bActivePortNum, 
									0);
				pHubDevice->ctrlIrp.pInBuffer = NULL;
				pHubDevice->ctrlIrp.dwInLength = MUSB_HUB_NO_DATA_PHASE;
				dwStatus = MUSB_StartControlTransfer (pHubDevice->pUsbDevice->pPort, &(pHubDevice->ctrlIrp));
				MUSB_PRINTK("HubSuccess: Debounce complete bActivePortNum is 0x%x\n", pHubDevice->bActivePortNum);
				return;
			}
		}
		else
		{
			MUSB_DIAG_STRING(2,"HubError: During debounce device disconnected");
			/* Disconnect got hence restart delay. */
			pDebounceParam->dwWaitingTime = MUSB_HUB_MIN_DEBOUNCE_TIME;
			pDebounceParam->bErrorCount++;
			if (pDebounceParam->bErrorCount > MUSB_HUB_DEBOUNCE_MAX_ERROR_COUNT)
			{
				/* Go to main loop with connection complete and display error
				* message that can not newly connected device is not responding.
				*/
				/* just return; With error messagge and put state proper. Call callback
				* to handle other port status change.
				*/
				MUSB_DIAG_STRING(2,"HubError: Cannot work with newly-connected device at the port");
				pHubDevice->bPortState     = MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE;
				pHubDevice->bPortNextState = MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE;
				pHubDevice->ctrlIrp.dwStatus = 0x00;
				MGC_HubGetPortStatusCallback ((void *)pHubDevice, &(pHubDevice->ctrlIrp));
				return;
			}   /* End of if (MUSB_HUB_DEBOUNCE_MAX_ERROR_COUNT) s */
			else
			{
				/* Again Start Timer. */
				pHubDevice->bPortNextState = MUSB_HUB_PORT_STATE_DEBOUNCE_GOT_STATUS;
				MGC_HubHandlePortDebounceTimer (pHubDevice, pDebounceParam->dwWaitingTime);
			}
		}
	}   /* End of if (MUSB_HUB_PORT_STATE_DEBOUNCE_GOT_STATUS) */

	return;
}  /* End of function  */


/*******************************************************************************
 *              MGC_HubHandlePortDebounceTimer ()                               *
 *******************************************************************************/
void 
MGC_HubHandlePortDebounceTimer (MUSB_Hub *pHubDevice, uint32_t dwWaitingTime)
{
	uint32_t dwStatus; 
	dwStatus = MUSB_ArmTimer(pHubDevice->busHandle, pHubDevice->pDriver, 
						pHubDevice->bIndexAtHubDeviceList, dwWaitingTime, 
						MGC_HubPortDebounceTimerCallback, (void *)pHubDevice);

	return;
}   /* End of function MGC_HubHandlePortDebounceTimer ()   */


/*****************************************************************************
 *              MGC_HubPortDebounceTimerCallback ()                          *
 *****************************************************************************/

void 
MGC_HubPortDebounceTimerCallback (void *pParam, MUSB_BusHandle hBus)
{
	MUSB_Hub                *pHubDevice;
	uint32_t                dwStatus;

	pHubDevice = (MUSB_Hub *)pParam;

	MUSB_DIAG_STRING(2,"Hub:WithIn MGC_HubPortDebounceTimerCallback()");    
	/* Since timer has expired then update the waiting time. */
	pHubDevice->debounceParam.dwWaitingTime += MUSB_HUB_INCREMENT_DEBOUNCE_TIME;

	pHubDevice->bPortNextState = MUSB_HUB_PORT_STATE_DEBOUNCE_GOT_STATUS;

	dwStatus = MUSB_HubGetPortStatus (pHubDevice, MGC_HubPortDebounceCallback, pHubDevice->bActivePortNum);

	return;
}  /* End of function MGC_HubPortDebounceTimerCallback ()   */


/*******************************************************************************
 *              MGC_HubPortResetCallback()                                      *
 *******************************************************************************/

void 
MGC_HubPortResetCallback (void *pContext, MUSB_ControlIrp *pControlIrp)
{
	MUSB_Hub                *pHubDevice;
	MUSB_Device             *pUsbDevice;

	MUSB_HubPortStatus      *pResetPortStatus;
	MUSB_PortResetParam      *pPortResetParam;

	pHubDevice      =  (MUSB_Hub *)pContext;
	pUsbDevice      =   pHubDevice->pUsbDevice;

	pPortResetParam = &(pHubDevice->resetParam);
	pResetPortStatus = &(pPortResetParam->resetPortStatus);

	if (pControlIrp->dwStatus != MUSB_SUCCESS)
	{
		MUSB_DIAG_STRING(1,"HubError: pControlIrp->dwStatus in MGC_HubPortResetCallback()");
		MUSB_DIAG1(2,"PortState:",pHubDevice->bPortState, 16, 0x00);
		MUSB_DIAG1(1,"ControlIrpStatus:", pControlIrp->dwStatus, 16, 0x00); 
		return;
	}

	/* Since IRP is executed properly hence update port state to its next port state.
	*/
	pHubDevice->bPortState = pHubDevice->bPortNextState;

	MUSB_DIAG_STRING(2,"Hub: MGC_HubPortResetCallback()");

	if (MUSB_HUB_PORT_STATE_START_RESET == pHubDevice->bPortState)
	{
		pHubDevice->bPortNextState = MUSB_HUB_PORT_STATE_RESET_GOT_STATUS;
		MGC_HubHandlePortResetTimer (pHubDevice, pPortResetParam->dwWaitingTime);
		return;
	}

	if (MUSB_HUB_PORT_STATE_RESET_GOT_STATUS == pHubDevice->bPortState)
	{
		MGC_HubStoreStatus (pHubDevice->aHubPortStatus, pResetPortStatus);

		if (!(pResetPortStatus->wStatus & MUSB_HUB_PORT_CONNECTION_BM))
		{
			/*  Make State proper and comeout from loop. No need to do reset
			* Prnt message that can not work with device. It is disconnected.
			*/
			/* Go to main loop with connection complete and display error
			* message that can not newly connected device is not responding.
			*/
			/* just return; With error messagge and put state proper. Call callback
			* to handle other port status change.
			*/
			MUSB_DIAG_STRING(2,"HubError: During Reset DeviceDisconnected");
			pHubDevice->bPortState     = MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE;
			pHubDevice->bPortNextState = MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE;
			MGC_HubGetPortStatusCallback ((void *)pHubDevice, &(pHubDevice->ctrlIrp));
			return;
		}

		if ((pResetPortStatus->wStatus & MUSB_HUB_PORT_CONNECTION_BM) &&
				(pResetPortStatus->wStatus & MUSB_HUB_PORT_ENABLE_BM))
		{
			/* RESET is successful and so get information about speed and return
			* as RESET SUCCESSFUL.
			*/
			MUSB_DIAG_STRING(2,"Hub: RESET COMPLETED");
			if (pResetPortStatus->wStatus & MUSB_HUB_PORT_LOW_SPEED_BM)
			{
				MUSB_DIAG_STRING(2,"Hub: Low-speed device connected");
				pHubDevice->bSpeed = MUSB_DEVICE_SPEED_LOW;
			}
			else if (pResetPortStatus->wStatus & MUSB_HUB_PORT_HIGH_SPEED_BM)
			{
				MUSB_DIAG_STRING(2,"Hub: High-speed device connected");
				pHubDevice->bSpeed = MUSB_DEVICE_SPEED_HIGH;
			}
			else
			{
				MUSB_DIAG_STRING(2,"Hub: Full-speed device connected");
				pHubDevice->bSpeed = MUSB_DEVICE_SPEED_FULL;
			}

			/* Reset signalling is complete */
			pHubDevice->bPortNextState = MUSB_HUB_PORT_STATE_RESET_COMPLETE;
			MUSB_HubClearPortFeature (pHubDevice, (uint16_t)MUSB_HUB_C_PORT_RESET_FEATURE, 
								MGC_HubPortConnectCallback, pHubDevice->bActivePortNum);
			return;
		}

		/* Check total reset time as well as number of tries.
		* If both are over then rise error message and comeout
		* else go for one more try.
		*/
		if (pPortResetParam->dwNumOfTry < MUSB_HUB_MAX_RESET_TRIES)
		{
			if (pPortResetParam->dwWaitingTime >= MUSB_HUB_MAX_RESET_TIME)
			{
				/* If waiting time is already more than MAX reset time but number
				* of tries less then MAX TRY then again Start RESET from begining.
				*/
				pPortResetParam->dwNumOfTry++;
				pPortResetParam->dwWaitingTime = MUSB_HUB_MIN_RESET_TIME;
			}
			/* Till dwNumOfTry < MAX TRY then go ahead and call timer. */
			pHubDevice->bPortNextState = MUSB_HUB_PORT_STATE_RESET_GOT_STATUS;
			MGC_HubHandlePortResetTimer (pHubDevice, pPortResetParam->dwWaitingTime);
			return;
		}
		else
		{
			/* Hub Has tried to RESET the device but couldn't Succeed.
			* So go for forget about device and go and address other
			* port status change.
			*/
			/* Return with pPortState and pPortState to 
			* "MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE" so 
			* other status of same port can be address.
			*/
			MUSB_DIAG_STRING(2,"Hub: Unable to complete reset");
			pHubDevice->bPortState     = MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE;
			pHubDevice->bPortNextState  = MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE;
			pHubDevice->ctrlIrp.dwStatus = 0x00;
			MGC_HubGetPortStatusCallback ((void *)pHubDevice, &(pHubDevice->ctrlIrp));
			return;
		}
		return;
	}
}   /* End of function  MGC_HubPortResetCallback () */


/*******************************************************************************
 *              MGC_HubHandlePortResetTimer ()                                 *
 *******************************************************************************/
void 
MGC_HubHandlePortResetTimer (MUSB_Hub *pHubDevice, uint32_t dwWaitingTime)
{
	uint32_t dwStatus; 
	dwStatus = MUSB_ArmTimer(pHubDevice->busHandle, pHubDevice->pDriver, 
						pHubDevice->bIndexAtHubDeviceList, dwWaitingTime, 
						MGC_HubPortResetTimerCallback, (void *)pHubDevice);

	return;
}   /* End of function MGC_HubHandlePortResetTimer () */



/*******************************************************************************
 *              MGC_HubPortResetTimerCallback ()                               *
 *******************************************************************************/

void 
MGC_HubPortResetTimerCallback (void *pParam, MUSB_BusHandle hBus)
{
	MUSB_Hub                *pHubDevice;
	uint32_t                dwStatus;

	pHubDevice = (MUSB_Hub *)pParam;

	MUSB_DIAG_STRING(2,"Hub: MGC_HubPortResetTimerCallback()");    
	/* Since timer has expired then update the waiting time. */
	pHubDevice->resetParam.dwWaitingTime += MUSB_HUB_INCREMENT_RESET_TIME;

	pHubDevice->bPortNextState = MUSB_HUB_PORT_STATE_RESET_GOT_STATUS;

	dwStatus = MUSB_HubGetPortStatus (pHubDevice, MGC_HubPortResetCallback, pHubDevice->bActivePortNum);

	return;
}   /* End of function MGC_HubPortResetTimerCallback () */


/*******************************************************************************
 *              MGC_HubPortConnectCallback()                                   *
 *******************************************************************************/

void
MGC_HubPortConnectCallback (void *pContext, MUSB_ControlIrp *pControlIrp)
{
	MUSB_Hub                *pHubDevice;
	uint32_t                dwStatus;

	pHubDevice      =  (MUSB_Hub *)pContext;
	if (pControlIrp->dwStatus != MUSB_SUCCESS)
	{
		MUSB_DIAG_STRING(1,"HubError: pControlIrp->dwStatus in MGC_HubPortConnectCallback()");
		MUSB_DIAG1(2,"PortState: ",pHubDevice->bPortState, 16, 0x00);
		MUSB_DIAG1(1,"ControlIrpStatus: ", pControlIrp->dwStatus, 16, 0x00); 
		return;
	}

	/* Since IRP is executed properly hence update port state to its next port state.
	*/
	pHubDevice->bPortState = pHubDevice->bPortNextState;

	if (MUSB_HUB_PORT_STATE_RESET_COMPLETE == pHubDevice->bPortState)
	{
		/*  1. First find out relation between bPortNum and bChildIndex if
		*     any device is present at that port. If any device is present at 
		*     given port then first deenumerate the device then enumerate
		*     newly connected device.
		*  2. If at given port no device is present then find first free 
		*     childIndex at which new device can register. 
		*/
		dwStatus = MGC_HubGetChildIndexForGivenPort(pHubDevice, 
							pHubDevice->bActivePortNum, 
							&(pHubDevice->bCurrentChildIndex));
		if ((MUSB_SUCCESS == dwStatus) && (pHubDevice->bDepthInHubChain <= MUSB_MAX_HUB_CHAIN))
		{
			/* Now everything is fine and go ahead and call enumeration function */
			pHubDevice->bPortNextState     = MUSB_HUB_PORT_WAIT_FOR_ENUMERATE;
			MUSB_EnumerateDevice(pHubDevice->pUsbDevice, pHubDevice->bActivePortNum,
			pHubDevice->bSpeed, MUSB_HubDeviceEnumerateCallback);
		}
		else
		{
			/* There is not free port is available so either increase the
			* limit of MUSB_HUB_MAX_CHILD or remove unwanted device from 
			* hub's port. 
			*/
			MUSB_DIAG2(1,"HubError: Failure Return By MGC_HubGetChildIndexForGivenPort(",
						(uint32_t)pHubDevice, ", ", pHubDevice->bActivePortNum, 16, 0);
						MUSB_DIAG1(2,"bDepthInHubChain: ",pHubDevice->bDepthInHubChain, 16, 0x00);

			/* Connected device cannot enumerated so go and handle other status
			* changes of the port. 
			*/
			pHubDevice->bPortState         = MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE;
			pHubDevice->bPortNextState     = MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE;
			pHubDevice->ctrlIrp.dwStatus = 0x00;
			MGC_HubGetPortStatusCallback ((void *)pHubDevice, &(pHubDevice->ctrlIrp));
		}
	}
	return;
}   /* End of function  MGC_HubPortConnectCallback () */

/*******************************************************************************
 *              MUSB_HubDeviceEnumerateCallback()                                   *
 *******************************************************************************/

void MUSB_HubDeviceEnumerateCallback (MUSB_Device *pHubUsbDevice, MUSB_Device *pChildUsbDevice)
{
	uint32_t dwStatus;
	MUSB_Hub *pHubDevice;

	dwStatus    = MGC_HubFindHubDevice(pHubUsbDevice, &pHubDevice);

	if (MUSB_SUCCESS == dwStatus )
	{
		if(MUSB_CLASS_HUB == pChildUsbDevice->DeviceDescriptor.bDeviceClass)
		{
			/*
			* Bus Powered bus cannot drive bus powered hub,
			* Newly connected hus can draw 1 unit of load atmost.
			* So, only the hub can servive(It can't supply load to the its ports)
			* The newly connected hub doesn't have power to drive the ports,
			* so, during connect function of this hub, make bAllocatedPower to
			* MUSB_HUB_BUS_POWER_MAX_UNITS
			*/
			if( (MUSB_HUB_BUS_POWERED == pHubDevice->bSelfPower)
				&& (!(pChildUsbDevice->apConfigDescriptors[0]->bmAttributes&MUSB_HUB_CONFIG_SELF_POWER_BM)) )
			{
				MUSB_DIAG_STRING(1,"HubError: bus-powered hub cannot drive another bus-powered hub");
				MUSB_DIAG_STRING(1,"Please disconnect newly connected device");
			}
		}

		dwStatus = MGC_HubUpdatePower(pHubDevice, MUSB_HUB_PORT_CONNECTED);
		pChildUsbDevice->pParentUsbDevice = pHubDevice->pUsbDevice; 
		pHubDevice->pChild[pHubDevice->bCurrentChildIndex] = pChildUsbDevice;
		pHubDevice->aHubChildPortMap[pHubDevice->bCurrentChildIndex].bStatus  = MUSB_HUB_CHILD_PRESENT;
		pHubDevice->aHubChildPortMap[pHubDevice->bCurrentChildIndex].bPortNum = pHubDevice->bActivePortNum;

		/* Everything is handled related to connect. Go and check for other
		* status of the port.
		*/
		pHubDevice->bPortState         = MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE;
		pHubDevice->bPortNextState     = MUSB_HUB_PORT_STATE_C_CONNECT_COMPLETE;
		pHubDevice->ctrlIrp.dwStatus = 0x00;
		MGC_HubGetPortStatusCallback ((void *)pHubDevice, &(pHubDevice->ctrlIrp));
	}   /* End of if  */

	return;
}  


/*******************************************************************
 *                 MGC_HubOverCurrentPortTimerCallback ()          *
 *******************************************************************/
void 
MGC_HubOverCurrentPortTimerCallback (void *pParam, MUSB_BusHandle hBus)
{
	MUSB_Hub                *pHubDevice;

	pHubDevice = (MUSB_Hub *)pParam;

	pHubDevice->bPortNextState = MUSB_HUB_PORT_STATE_C_OVERCURRENT_COMPLETE;
	MGC_HubGetPortStatusCallback(pParam, &(pHubDevice->ctrlIrp));

	return;
}


/*******************************************************************************
 *              MGC_HubOverCurrentPortTimer ()                                 *
 *******************************************************************************/

void 
MGC_HubOverCurrentPortTimer (MUSB_Hub *pHubDevice, uint32_t dwWaitingTime)
{
	uint32_t dwStatus; 
	dwStatus = MUSB_ArmTimer(pHubDevice->busHandle, pHubDevice->pDriver, 
					pHubDevice->bIndexAtHubDeviceList, dwWaitingTime, 
					MGC_HubOverCurrentPortTimerCallback, (void *)pHubDevice);

	return;
}   /* End of function MGC_HubOverCurrentPortTimer () */


/*******************************************************************
 *                 MGC_HubHandlePortDisconnect ()                  *
 *******************************************************************/
void 
MGC_HubHandlePortDisconnect (MUSB_Hub *pHubDevice, uint8_t bPortNum)
{
	MUSB_Device   *pChildUsbDevice;
	MUSB_Hub      *pChildHubDevice;
	uint8_t       bChildIndex;
	uint32_t      dwStatus;

	MUSB_PRINTK("===MGC_HubHandlePortDisconnect()===\n");
	/* Get the USB device pointer of the port where device is disconnect.
	*/

	/* 1. Get Index of the device disconnected from bPortNum.
	* 2. Extract USBDevicePointer of disconnected device.
	* 3. Check That Disconnected device is Hub of normal USB device.
	* 4. If it is hub then Call De-Enumeate Hub otherwise call De-Enumerate device.
	* 5. Make pHubDevice->aHubChildPortMap[bChildIndex].bStatus  = MUSB_HUB_CHILD_FREE;
	*/

	/* Get Child index. */
	dwStatus = MGC_HubGetChildIndexForGivenPort(pHubDevice, bPortNum, &bChildIndex);
	MUSB_PRINTK("dwStatus is 0x%x bChildIndex is 0x%x \n", dwStatus, bChildIndex);
	
	if ( MUSB_HUB_CHILD_PRESENT == dwStatus)
	{
		MUSB_DIAG_STRING(2,"GivenPortDeviceIsPresent");
		/* Control comes here it means device is allready there 
		* hence de-enumerate it. 
		*/
		pChildUsbDevice  = pHubDevice->pChild[bChildIndex];

		MUSB_PRINTK("pChildUsbDevice is 0x%x \n", pChildUsbDevice);
		
		if(MUSB_CLASS_HUB != pChildUsbDevice->DeviceDescriptor.bDeviceClass)
		{
			/* This device is Normal USB device hence de-enumerate it.*/
			MUSB_DIAG_STRING(2,"DeviceIsNotAHub");
			MUSB_DeviceDisconnected (pChildUsbDevice);
		}
		else
		{
			/* the device was a hub */
			MUSB_DIAG_STRING(2,"DeviceIsHub");
			dwStatus = MGC_HubFindHubDevice(pChildUsbDevice, &pChildHubDevice);
			MGC_DeEnumerateHub (pChildHubDevice, MUSB_NORMAL_HUB);
		}
		/* Disconnected device is handled properly so Its time to update "childPortMap" */    
		pHubDevice->aHubChildPortMap[bChildIndex].bStatus  = MUSB_HUB_CHILD_FREE;
		pHubDevice->pChild[bChildIndex] = NULL;
	}

	return;
}   /* End of function MGC_HubHandlePortDisconnect() */



/*******************************************************************
 *                 MGC_DeEnumerateHub ()                           *
 *******************************************************************/
void 
MGC_DeEnumerateHub (MUSB_Hub *pHubDevice , uint32_t dwHubType)
{
	MUSB_Hub        *pChildHubDevice;
	MUSB_Device     *pChildUsbDevice;
	uint8_t         bI;
	uint32_t        dwStatus;

	MUSB_PRINTK("HUB: MGC_DeEnumerateHub() >>\n");
	MUSB_PRINTK("HubType: 0x%x\n", dwHubType);

	/* 1. Find out any USB device is attached with disconnected hub's port or not.
	* 2. If any Usb device is conectd then find out is it Hub device or normal USB device.
	* 3. If it is hub device then Call De-EnumerateHub again otherwise call De-Enumerate device.
	* 4. Once all the child Handled then it is time to "De-Enumerate Hub as Device" if it is
	*    not root hub.
	*/
	for ( bI =0x00; bI < MUSB_HUB_MAX_PORTS; bI++)
	{
		pChildUsbDevice = pHubDevice->pChild[bI];

		if (pChildUsbDevice)
		{
			/* Get Hub device pointer if given USB device pointer belongs to Hub device. */
			dwStatus = MGC_HubFindHubDevice(pChildUsbDevice, &pChildHubDevice); 

			/* Probability of getting MUSB_FAILURE is 60-80% i.e.
			* given device is not hub.
			*/
			/*
			if (dwStatus == MUSB_FAILURE_UNSIGNED)
			*/
			if(pChildUsbDevice->pDriverPrivateData != pHubDevice->pUsbDevice->pDriverPrivateData)
			{
				MUSB_DIAG_STRING(2,"HUB: Child is not a hub");
				/* This device is not a hub device. */
				MUSB_DeviceDisconnected (pChildUsbDevice);
			}
			else
			{
				MUSB_DIAG_STRING(2,"HUB: Child is also a hub");
				/* This is a hub DEVICE hence Deenumerate HUB */
				MGC_DeEnumerateHub (pChildHubDevice, MUSB_NORMAL_HUB);
			}
			pHubDevice->aHubChildPortMap[bI].bStatus = MUSB_HUB_CHILD_FREE;
			pHubDevice->pChild[bI] = NULL;
		}   /* End of if (pChildUsbDevice) */
	}   /* End of for (bI)      */

	/* All ports of the hub is cleaned so De-Enumerate current Hub as device. */
	if (MUSB_NORMAL_HUB == dwHubType)
	{
		/* If it is normal hub then De-Enumerate device call needed. If it is 
		* Root hub then De-Enumerate device is allready called  by disconnect 
		* interrupt.
		*/
		MUSB_DeviceDisconnected (pHubDevice->pUsbDevice);
	}

	MUSB_DIAG_STRING(3,"HUB: MGC_DeEnumerateHub() <<");
	return;
}   /* End of function MGC_DeEnumerateHub()  */


/*******************************************************************
 *                 MGC_HubHandleEnablePort ()                      *
 *******************************************************************/

void
MGC_HubHandleEnablePort (MUSB_Hub *pHubDevice, uint8_t *pPortNextState, uint8_t bPortNum)
{
	uint32_t  dwStatus;

	MUSB_PRINTK("HUB: MGC_HubHandleEnablePort()\n");

	*pPortNextState = MUSB_HUB_PORT_STATE_C_ENABLE_COMPLETE;   
	dwStatus = MUSB_HubClearPortFeature(pHubDevice, MUSB_HUB_C_PORT_ENABLE_FEATURE, 
	                                   	 MGC_HubGetPortStatusCallback, bPortNum);
}   /* End of function MGC_HubHandleEnablePort () */


/*******************************************************************
 *                 MGC_HubHandleSuspendPort()                      *
 *******************************************************************/
void
MGC_HubHandleSuspendPort (MUSB_Hub *pHubDevice, uint8_t *pPortNextState, uint8_t bPortNum)
{
	uint32_t  dwStatus;

	MUSB_PRINTK("HUB: MGC_HubHandleSuspendPort()\n");

	*pPortNextState = MUSB_HUB_PORT_STATE_C_SUSPEND_COMPLETE;   
	dwStatus = MUSB_HubClearPortFeature(pHubDevice, MUSB_HUB_C_PORT_SUSPEND_FEATURE, 
	                                    	MGC_HubGetPortStatusCallback, bPortNum);
}   /* End of function MGC_HubHandleSuspendPort () */

/*******************************************************************
 *                 MGC_HubStoreStatus ()                           *
 *******************************************************************/
void 
MGC_HubStoreStatus (uint8_t *pStatusData, MUSB_HubPortStatus *pHubStatus)
{   
	uint32_t dwTempVar;
	/* Once Get Bits are used then no need to Swap the fields */
	MUSB_BitsGet (pStatusData, 0x00, 16, &dwTempVar);
	pHubStatus->wStatus =   (uint16_t)  dwTempVar;

	pStatusData = pStatusData + 2;
	MUSB_BitsGet (pStatusData, 0x00, 16, &dwTempVar);  
	pHubStatus->wStatusChange = (uint16_t)dwTempVar;
}   /* End of function  MGC_HubStoreStatus() */


/**************************************************************************
 *Function    : MUSB_HubPrepareSetupPkt()
 *Purpose     : This function prepars Setup packet. 
***************************************************************************/
/* TBD we have to take decision that prepare SETUP packet should be
 * MACRO or Function.
 */
void
MUSB_HubPrepareSetupPkt (MUSB_DeviceRequest *pSetup, uint8_t bmRequestType, 
                       uint8_t bRequest, uint16_t wValue, 
                       uint16_t wIndex, uint16_t wLength)
{ 
	pSetup->bmRequestType = bmRequestType;
	pSetup->bRequest      = bRequest;
	pSetup->wValue        = wValue;
	pSetup->wIndex        = wIndex;
	pSetup->wLength       = wLength;

	/*
	 * Swap double byte fields of Setup packet
	 */
	pSetup->wValue        = (uint16_t)MUSB_SWAP16(pSetup->wValue);
	pSetup->wIndex        = (uint16_t)MUSB_SWAP16(pSetup->wIndex);
	pSetup->wLength       = (uint16_t)MUSB_SWAP16(pSetup->wLength);
}   /* End of function MUSB_PrepareSetupPkt ()*/


/*******************************************************************
 *                 MGC_HubGetChildIndexForGivenPort ()             *
 *******************************************************************/

uint32_t 
MGC_HubGetChildIndexForGivenPort (MUSB_Hub *pHubDevice, uint8_t bPortNum, uint8_t *pChildIndex)
{
	MUSB_HubChildPortMap *pHubChildPortMap = pHubDevice->aHubChildPortMap;
	int16_t  wFreeChildIndex = -1;
	uint8_t  bIndex;

	/* 1. Findout at given port any device is present or not along with 
	*    first free entry. 
	* 2. Once No device is connected at given port then pChildIndex update 
	*    with bFreeChild Index.
	*/
	for (bIndex = 0x00; bIndex < MUSB_HUB_MAX_PORTS; bIndex++)
	{
		if ((pHubChildPortMap[bIndex].bStatus == MUSB_HUB_CHILD_PRESENT)
			&& (pHubChildPortMap[bIndex].bPortNum == bPortNum))
		{
			*pChildIndex = bIndex;
			return (MUSB_HUB_CHILD_PRESENT);
		}

		if ((pHubChildPortMap[bIndex].bStatus == MUSB_HUB_CHILD_FREE) 
			&& (wFreeChildIndex == -1))
		{
			wFreeChildIndex = (int16_t)bIndex;
		}       
	}

	if (wFreeChildIndex != -1)
	{
		*pChildIndex = (uint8_t) wFreeChildIndex;
		return MUSB_HUB_CHILD_FREE;
	}
	return (MUSB_FAILURE);
}   /* End of function  MGC_HubGetChildIndexForGivenPort () */


