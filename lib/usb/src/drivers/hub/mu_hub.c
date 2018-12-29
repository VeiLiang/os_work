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
 * Hub feature discovery, configuration, interrupt IRP submission,
 * power and TT management functionality.
 * $Revision: 1.2 $
 */

#include "mu_bits.h"
#include "mu_cdi.h"
#include "mu_descs.h"
#include "mu_diag.h"
#include "mu_mem.h"

#include "mu_hdef.h"
#include "mu_hub.h"
#include "mu_hcore.h"
#include "mu_hcreq.h"

#include "mu_hapi.h"

static const char* MGC_aDisconnectHub = "HubError: Please disconnect hub";
static char Ecount = 0;
/*******************************************************************
 *                 MGC_HubConnectHandler ()                        *
 *******************************************************************/

uint32_t
MGC_HubConnectHandler(MUSB_Hub *pHubDevice,MUSB_Device *pUsbDevice, 
                      uint8_t bBusAddress, MUSB_Port *pPort)
{

	MUSB_ControlIrp     *pControlIrp;    
	MUSB_DeviceRequest  *pSetupPacket;
	uint8_t              bConfigurationNumber;  
	uint32_t             dwStatus;

	pControlIrp      = &(pHubDevice->ctrlIrp);
	pSetupPacket     = &(pHubDevice->setupPacket);

	MUSB_DIAG_STRING(2,"MGC_HubConnectHandler() >>");
	/* Hub can have only one configuration descriptor so always that configuration
	* is active configuration Number. 
	*/
	bConfigurationNumber = pUsbDevice->apConfigDescriptors[0]->bConfigurationValue;

	/* Prepare control request for SET_CONFIG */
	MUSB_HubPrepareSetupPkt (pSetupPacket,
						(MUSB_DIR_OUT | MUSB_TYPE_STANDARD | MUSB_RECIP_DEVICE),
						MUSB_REQ_SET_CONFIGURATION,
						bConfigurationNumber,
						0x0000,
						MUSB_HUB_NO_DATA_PHASE);

	/* Initialize control IRP. Now onward whenewer driver wants to use
	* control IRP then it has to update only pInBuffer, pOutBuffer, 
	* dwInLength, dwOutLength and ofcource Callback as per requrement. 
	*/
	MUSB_FILL_CTRL_IRP(pControlIrp, bBusAddress, 
						pSetupPacket, 
						MUSB_HUB_SETUP_PACKET_LENGTH, 
						NULL, MUSB_HUB_NO_IN_DATA_TRANSFER, 
						MGC_HubInitializeCallback, 
						(void *) pHubDevice);
						pControlIrp->pDevice = pUsbDevice;

	/* Start control Transfer.  */
	dwStatus = MUSB_StartControlTransfer (pPort, pControlIrp);
	if (dwStatus)
	{
		/* Print the status and then come aback.*/ 
		MUSB_DIAG_STRING(1,"HubError: MGC_HubConnectHandler()");
		MUSB_DIAG_STRING(1,MGC_aDisconnectHub);
		return (MUSB_FAILURE);
	}

	MUSB_DIAG_STRING(3,"HubSuccess: MGC_HubConnectHandler() <<");
	return (MUSB_SUCCESS);
}   /* End of function MGC_HubConnectHandler () */


/*******************************************************************
 *                 MGC_HubInitializeCallback ()                    *
 *******************************************************************/

void
MGC_HubInitializeCallback (void *pContext, MUSB_ControlIrp *pControlIrp)
{      

	MUSB_Hub        *pHubDevice;
	MUSB_Device     *pUsbDevice;
	MUSB_HubDescriptor    *pHubDscr;
	uint8_t         *pIrpData;
	uint32_t        dwStatus;
	uint8_t         bNumOfPort;
	uint32_t        dwBytesForPorts;
	uint32_t        dwNumOfBytes;

	pHubDevice   = (MUSB_Hub *)pContext;
	pUsbDevice   = pHubDevice->pUsbDevice;

	if (pControlIrp->dwStatus)
	{
		/*  IRP is not executed properly. So can not go ahead. */
		MUSB_DIAG1(1,"HubError: MGC_HubInitializeCallback(): pControlIrp->dwStatus=", pControlIrp->dwStatus, 16, 0);
		MUSB_DIAG1(1,"HubState: ",pHubDevice->bState, 16, 0x00);
		return;
	}

	pUsbDevice->pCurrentConfiguration = pUsbDevice->apConfigDescriptors[0];

	/* Since IRP is executed properly hence whatever was present in the bNextState 
	* before IRP execution will become bState. 
	*/
	pHubDevice->bState   =  pHubDevice->bNextState;

	switch (pHubDevice->bState)
	{
		case MUSB_HUB_STATE_CONFIGURED:
		{
			/* Control Comes here it means SetConfig Command is executed properly
			* Now time to prepare RESET_TT if hub is high speed otherwise prepare
			* command to get Hub Descriptor. 
			*/
			MUSB_DIAG_STRING(2, "Hub: MUSB_HUB_STATE_CONFIGURED");

			pHubDevice->bActivePortNum = MUSB_HUB_STARTING_PORT_NUMBER;

			dwStatus = MGC_HubValidateUpdateTtInfo(pHubDevice);
			if(MUSB_SUCCESS != dwStatus) 
			{
				/* There is some problem in interface descriptor hence cannot
				* proceed.
				*/
				MUSB_DIAG_STRING(1, "HubError: Descriptor parsing failed");
				MUSB_DIAG_STRING(1, MGC_aDisconnectHub);
				return;
			}

			if(MUSB_HUB_NO_TT == pHubDevice->bTtType & MUSB_HUB_CURRENT_TT_TYPE_BM)
			{
				/* In case of full as well as low speed hub there is no need to send
				* resetTT command. So call same callback again with the bNextState
				* complete.
				*/
				pHubDevice->bNextState = MUSB_HUB_STATE_RESET_TT_COMPLETE;
				MGC_HubInitializeCallback (pContext,pControlIrp);
			}
			else
			{
				/* For single TT arrangement, only once RESET_TT command required to
				* but for Multiple TT hub RESET_TT command need to send to all 
				* the ports one by one.
				*/            
				if(MUSB_HUB_SINGLE_TT == pHubDevice->bTtType & MUSB_HUB_CURRENT_TT_TYPE_BM)
				{
					MUSB_DIAG_STRING(2, "Hub: SingleTT");
					pHubDevice->bNextState = MUSB_HUB_STATE_RESET_TT_COMPLETE;
				}
				else 
				{
					pHubDevice->bNextState = MUSB_HUB_STATE_RESET_TT;
				}

				dwStatus = MGC_HubInitAndResetTt(pUsbDevice, MGC_HubInitializeCallback, pHubDevice);
				if(MUSB_SUCCESS != dwStatus)
				{
					MUSB_DIAG_STRING(1,"HubError: InitAndResetTt() failed: please disconnect hub");
					return;
				}
			}
			break;
		}   /* End of case   MUSB_HUB_STATE_CONFIGURED  */

		case MUSB_HUB_STATE_RESET_TT:
		{
			/* Control comes here it means RESET_TT sent for previous port is sent properly
			* now time to sent RESET_TT for next port. If next port is the Last port of
			* the Hub then bNextState will be  MUSB_HUB_STATE_RESET_TT_COMPLETE otherwise
			* it will be same as MUSB_HUB_STATE_RESET_TT.
			*/
			MUSB_DIAG_STRING(2, "Hub: MUSB_HUB_STATE_RESET_TT");
			pHubDevice->bActivePortNum++;

			if (pHubDevice->bActivePortNum < pHubDevice->hubDscr.bNbrPorts)
			{
				pHubDevice->bNextState = MUSB_HUB_STATE_RESET_TT;                
			}
			else
			{
				pHubDevice->bNextState = MUSB_HUB_STATE_RESET_TT_COMPLETE;
			}

			dwStatus = MUSB_HubResetTt(pUsbDevice, MGC_HubInitializeCallback, 
			pHubDevice, pHubDevice->bActivePortNum);

			if(MUSB_SUCCESS != dwStatus)
			{
				MUSB_DIAG_STRING(1,"HubError: ResetTT failed: please disconnect hub");
				return;
			}
			break;
		}   /* End of case   MUSB_HUB_STATE_RESET_TT  */

		case MUSB_HUB_STATE_RESET_TT_COMPLETE:
		{
			/* Control comes here it means ResetTT command is executed
			* properly for all the ports. Now its time to ask for Hub 
			* descriptor by sending control request to the device.
			*/
			MUSB_DIAG_STRING(2, "Hub: MUSB_HUB_STATE_RESET_TT_COMPLETE");

			MUSB_HubPrepareSetupPkt (&(pHubDevice->setupPacket), 
								(MUSB_DIR_IN | MUSB_TYPE_CLASS | MUSB_RECIP_DEVICE),
								MUSB_REQ_GET_DESCRIPTOR, (uint16_t)(MUSB_DT_HUB << 8), 
								0x00, MUSB_HUB_MAX_HUB_DSCR_SIZE);

			/* Update required field of control IRP for getting hub descriptor. */
			pControlIrp->pInBuffer         =  pHubDevice->aHubRawDscr; 
			pControlIrp->dwInLength        =  MUSB_HUB_MAX_HUB_DSCR_SIZE;

			pHubDevice->bNextState = MUSB_HUB_STATE_GOT_HUB_DSCR;        
			dwStatus = MUSB_StartControlTransfer (pUsbDevice->pPort, pControlIrp);

			if (dwStatus)
			{
				/* Not able to submit control IRP. You are gone. */ 
				MUSB_DIAG_STRING(1,"HubError: MUSB_HUB_STATE_RESET_TT_COMPLETE: StartControlTransfer()");
				MUSB_DIAG_STRING(1, MGC_aDisconnectHub);
			}
			break;
		}   /* End of case  MUSB_HUB_STATE_RESET_TT_COMPLETE */


		case MUSB_HUB_STATE_GOT_HUB_DSCR : 
		{
			/* If control comes here it means hub descriptor has come. Store it in proper
			* format and Enable all ports of HUB through setFeature(PORT_POWER) command.
			*/

			MUSB_DIAG_STRING(2, "Hub: MUSB_HUB_STATE_GOT_HUB_DSCR");
			pHubDscr = &(pHubDevice->hubDscr);
			pIrpData = pControlIrp->pInBuffer;

			/* Store hub descriptor in proper data structure.Number of port this
			* hub support is available as third byte in incoming data.
			* It may possible that number of ports could be 0x00 but that does 
			* not mean HUB is not working. So Dont worry and go ahead.
			*/
			bNumOfPort = pIrpData[2];

			dwBytesForPorts = (uint32_t)((bNumOfPort >> 3) + 1);

			/* Copy from bDscrLenth to aDeviceRemovable.  */
			dwNumOfBytes = 0x07 + dwBytesForPorts;
			MUSB_MemCopy ((void *)pHubDscr, (void *)pIrpData, dwNumOfBytes);

			pIrpData += 0x07;
			MUSB_MemCopy ((void *)pHubDscr->aDeviceRemovable, (void *)pIrpData, dwBytesForPorts);
			pIrpData++;
			MUSB_MemCopy ((void *)pHubDscr->aPortPwrCtrlMask, (void *)pIrpData, dwBytesForPorts);

			/* swap wHubCharacteristics */
			pHubDevice->hubDscr.wHubCharacteristics = pControlIrp->pInBuffer[3] | (pControlIrp->pInBuffer[4] << 8);

			/* If bNumOfPorts are  more then 0x00 then go ahead otherwise 
			* submit Intr Irp for hub. IntrIrp will report change of Hub
			* status only.
			*/
			if ( ((pHubDevice->hubDscr.wHubCharacteristics & MUSB_HUB_COMPOUND_DEVICE) == MUSB_HUB_COMPOUND_DEVICE)
				&& (pHubDevice->bDepthInHubChain == MUSB_MAX_HUB_CHAIN))
			{
				MUSB_DIAG_STRING(1,"Compound device; cannot handle at this level");
				MUSB_DIAG_STRING(1,MGC_aDisconnectHub);
				pHubDevice->bNextState = MUSB_HUB_STATE_DISCONNECT_DEVICE;
				return;
			} 
			if (bNumOfPort != 0x00)
			{
				/* As per 11.11 if hub supports per port Power switching then SET_FEATURE
				* request send to every port. If hub supports GANG power switching then
				* power of all ports belongs to gang can be switched on by making power
				* on for one ports.
				*/       
				MUSB_DIAG1(2, "NumberOfPorts = ", bNumOfPort, 10, 0);
				pHubDevice->bActivePortNum =  MUSB_HUB_STARTING_PORT_NUMBER;
				/* Send Command for SetFeature (PORT_POWER). */
				/* Set power for first port and then every port into a gang would be 
				* POWER ON state.  
				*/

				//Nicholas Xu
				#if 0
				if ( MUSB_HUB_GANGED_POWER_SWITCHING ==
					(pHubDevice->hubDscr.wHubCharacteristics & MUSB_HUB_LOGICAL_POWER_SWITCH_MODE_BM))
				{
					/* bActivePort make it to MUSB_HUB_INVALID_PORT_NUMBER because
					* request for one port is sufficient in case ganged power. 
					*/
					MUSB_DIAG_STRING(2,"HubGangedPowerSwitched");
					pHubDevice->bNextState     = MUSB_HUB_STATE_POWER_ON_COMPLETE;                
				}
				else
				{
					/* need to send request for every port so state will be
					* POWER_ON not POWER_ON_COMPLETE. Till IRP is not executed
					* properly, don't make bActivePortNum to next port number.
					*/
					MUSB_DIAG_STRING(2,"HubPerPortPowerSwitched");
					if (pHubDevice->hubDscr.bNbrPorts == 0x01)
					{
						pHubDevice->bNextState     = MUSB_HUB_STATE_POWER_ON_COMPLETE;                         
					}
					else
					{
						pHubDevice->bNextState = MUSB_HUB_STATE_POWER_ON;                
					}
				}
				#else
				/* need to send request for every port so state will be
				* POWER_ON not POWER_ON_COMPLETE. Till IRP is not executed
				* properly, don't make bActivePortNum to next port number.
				*/
				MUSB_DIAG_STRING(2,"HubPerPortPowerSwitched");
				if (pHubDevice->hubDscr.bNbrPorts == 0x01)
				{
					pHubDevice->bNextState     = MUSB_HUB_STATE_POWER_ON_COMPLETE;                         
				}
				else
				{
					pHubDevice->bNextState = MUSB_HUB_STATE_POWER_ON;                
				}
				#endif
				
				dwStatus = MUSB_HubSetPortFeature(pHubDevice, 
										MUSB_HUB_PORT_POWER_FEATURE,
										MGC_HubInitializeCallback,
										pHubDevice->bActivePortNum);
				if (MUSB_SUCCESS == dwStatus)
				{
					/* If Power On is comeplete then make activePort Number to
					* Invalid port Number.
					*/
					if (MUSB_HUB_STATE_POWER_ON_COMPLETE == pHubDevice->bNextState)
					{
						MUSB_DIAG_STRING(2,"bNextSatate POWER_ON_COMPLETE");
						pHubDevice->bActivePortNum = MUSB_HUB_INVALID_PORT_NUMBER;
					}
				}
				else
				{
					MUSB_DIAG_STRING(1,"HubError: While Calling MUSB_HubSetPortFeature()");
					MUSB_DIAG_STRING(1,MGC_aDisconnectHub);
				}
			}
			else
			{
				/* Control comes here it means number of port is 0x00 in the
				* hub. Hub is not useful so display messge that there is no use
				* of hub and put hub state INIT.
				*/
				MUSB_DIAG_STRING(1,"HubError: 0 ports; please disconnect the hub");
				pHubDevice->bNextState = MUSB_HUB_STATE_DISCONNECT_DEVICE;
			}
			break;
		}   /* End of case MUSB_HUB_STATE_GOT_HUB_DSCR */


		case MUSB_HUB_STATE_POWER_ON : 
		{
			/* If control comes here it means PORT_POWER_FEATURE for port
			* reperesented by bActivePortNum is set and do same thing for 
			* next port. Hub will remain in this state till 
			* pHubDevice->bActivePortNum == bNumOfPort.
			*/
			MUSB_DIAG_STRING(2, "Hub: MUSB_HUB_STATE_POWER_ON");
			pHubDevice->bActivePortNum++;

			if (pHubDevice->bActivePortNum < pHubDevice->hubDscr.bNbrPorts)
			{
				pHubDevice->bNextState = MUSB_HUB_STATE_POWER_ON;                
			}
			else
			{
				pHubDevice->bNextState = MUSB_HUB_STATE_POWER_ON_COMPLETE;
			}

			dwStatus = MUSB_HubSetPortFeature(pHubDevice, 
									MUSB_HUB_PORT_POWER_FEATURE,
									MGC_HubInitializeCallback,
									pHubDevice->bActivePortNum);
			if (MUSB_SUCCESS != dwStatus)
			{
				MUSB_DIAG_STRING(1,"HubError: while calling MUSB_HubSetPortFeature()");
				MUSB_DIAG_STRING(1,MGC_aDisconnectHub);
			}
			break;
		}   /* End of case  MUSB_HUB_STATE_POWER_ON */

		case MUSB_HUB_STATE_POWER_ON_COMPLETE:
		{
			/* Control Comes here it means All the ports are in POWER ON state.
			* Now its time to  Get Hub Status for Power Validation.
			*/
			MUSB_DIAG_STRING(2, "Hub: MUSB_HUB_STATE_POWER_ON_COMPLETE");

			pHubDevice->bNextState = MUSB_HUB_STATE_GOT_HUB_STATUS; 

			dwStatus = MUSB_HubGetHubStatus (pHubDevice, MGC_HubInitializeCallback); 

			if(MUSB_SUCCESS != dwStatus)
			{
				MUSB_DIAG1(1, "HubError: MUSB_HubGetHubStatus() returned ", dwStatus, 16, 0);
				MUSB_DIAG_STRING(1, MGC_aDisconnectHub);
				pHubDevice->bNextState = MUSB_HUB_STATE_DISCONNECT_DEVICE;
			}
			break;
		}   /* End of case  MUSB_HUB_STATE_POWER_ON_COMPLETE */

		case MUSB_HUB_STATE_GOT_HUB_STATUS : 
		{
			MUSB_DIAG_STRING(2, "Hub: MUSB_HUB_STATE_GOT_HUB_STATUS"); 
			MGC_HubStoreStatus (pHubDevice->aHubPortStatus, &(pHubDevice->hubStatus));  

			/* Now Its time to validate Power */
			dwStatus = MGC_HubPowerValidateInit(pHubDevice);
			if(MUSB_SUCCESS != dwStatus)
			{
				MUSB_DIAG1(1, "HubError: MGC_HubPowerValidateInit() returned ", dwStatus, 16, 0);

				MUSB_DIAG_STRING(1, MGC_aDisconnectHub);
				pHubDevice->bNextState = MUSB_HUB_STATE_DISCONNECT_DEVICE;
				break;
			}

			/* For all the ports, SETPORT Feature (PORT_POWER) is sent. Now 
			* time to submit Interrupt IRP. This Interrupt IRP will report
			* any change in HUB as well port status.
			*/
			dwStatus = MGC_HubFillAndSubmitIntrIrp (pHubDevice, pUsbDevice->bBusAddress);

			if (MUSB_SUCCESS != dwStatus)
			{
				if (NULL != pHubDevice->intrInPipe)
				{
					dwStatus = MUSB_ClosePipe(pHubDevice->intrInPipe);
				}
				MUSB_DIAG_STRING(1,"HubError: While calling MGC_HubFillAndSubmitIntrIrp()");
				pHubDevice->bNextState = MUSB_HUB_STATE_DISCONNECT_DEVICE;
				break;
			}
			MUSB_DIAG_STRING(3, "HubSuccess: Hub is operating");
			pHubDevice->bState     = MUSB_HUB_STATE_INIT;
			pHubDevice->bNextState = MUSB_HUB_STATE_INIT;
			break;
		}   /* End of case  MUSB_HUB_STATE_GOT_HUB_STATUS*/

	}    /* End of Switch Statement. */  
}   /* End of function MGC_HubInitializeCallback () */


/*******************************************************************
 *                 MGC_HubFillAndSubmitIntrIrp ()                  *
 *******************************************************************/

uint32_t 
MGC_HubFillAndSubmitIntrIrp (MUSB_Hub *pHubDevice, uint8_t bBusAddress)
{
	const MUSB_EndpointDescriptor        *pEndpDscr;
	MUSB_DeviceEndpoint                  devEndp;
	MUSB_Irp                             *pIntrInIrp; 
	uint32_t                             dwIntrIrpDataLength;
	uint16_t                             wNakLimit = 0xFFFF; /* Disable the NAK Limit */
	uint32_t                             dwStatus = MUSB_FAILURE;

	pEndpDscr = pHubDevice->pEndpDscr;
	pIntrInIrp  = &(pHubDevice->intrInIrp); 

	MUSB_DIAG_STRING(3,"MGC_HubFillAndSubmitIntrIrp() >>");

	/* One bit for each port's status change and one bit for hub status change.
	* Let us assume 4 ports are there then to report status change of 4 ports,
	* 4 bit is required and 1 bit for hub status change bit hence 5 bits are
	* required. Data transfered happens in Bytes SO 1 byte is required to report above
	* things.
	*/
	dwIntrIrpDataLength = (uint32_t) ((pHubDevice->hubDscr.bNbrPorts >> 3) + 1);

	if (MUSB_ENDPOINT_XFER_INT == (pEndpDscr->bmAttributes & MUSB_ENDPOINT_XFERTYPE_MASK))
	{  
		if ( MUSB_DIR_IN == (pEndpDscr->bEndpointAddress & MUSB_ENDPOINT_DIR_MASK)) 
		{
			MUSB_MemCopy((void *)(&(devEndp.UsbDescriptor)), (void *)pEndpDscr, 0x07);
			devEndp.wNakLimit		= wNakLimit;
			devEndp.pDevice			= pHubDevice->pUsbDevice;
			
			//Nicholas Xu
		#if 1
			if(!pHubDevice->busHandle)
				return dwStatus;
		#endif
			
			pHubDevice->intrInPipe	= MUSB_OpenPipe(pHubDevice->busHandle, &devEndp, NULL);
			if (NULL == pHubDevice->intrInPipe)
			{
				MUSB_DIAG1(1, "HubError: OpenPipe status=", dwStatus, 16, 0);
				return dwStatus;
			}
			/* Fill the Interrupt IRP */
			pIntrInIrp->hPipe = pHubDevice->intrInPipe;
			pIntrInIrp->bAllowShortTransfer = FALSE;
			pIntrInIrp->pBuffer = pHubDevice->aHubPortStatusChangeBuffer;
			pIntrInIrp->dwLength = dwIntrIrpDataLength;
			pIntrInIrp->pfIrpComplete = MGC_HubIntrIrpCallback;
			pIntrInIrp->pCompleteParam = (void *)pHubDevice;

			MUSB_DIAG_STRING(2,"Hub: Submitting interrupt IRP");
			dwStatus = MUSB_StartTransfer(pIntrInIrp);
			if (dwStatus)
			{
				MUSB_DIAG1(3,"HubError: MUSB_StartTransfer() status=", dwStatus, 16, 0);
				return dwStatus;
			}

			MUSB_DIAG_STRING(3,"HubSuccess: MGC_HubFillAndSubmitIntrIrp() <<");
			dwStatus = MUSB_SUCCESS;
		}
		else
		{
			MUSB_DIAG_STRING(1,"HubError: EndpDirOutButExpectedIn");
			MUSB_DIAG_STRING(1,"HubError: MGC_HubFillAndSubmitIntrIrp() <<");		
		}
	}
	else
	{
		MUSB_DIAG_STRING(1,"HubError: InterruptEndpNotAvailable");
		MUSB_DIAG_STRING(1,"HubError: MGC_HubFillAndSubmitIntrIrp() <<");   
	}

	return (dwStatus);
}   /* End of function  MGC_HubFillAndSubmitIntrIrp () */


/*******************************************************************
 *                 MGC_HubIntrIrpCallback ()                       *
 *******************************************************************/
void
MGC_HubIntrIrpCallback (void *pContext, MUSB_Irp *pIntrIrp)
{      
	MUSB_Hub    *pHubDevice;

	pHubDevice = (MUSB_Hub *)pContext;
	MUSB_DIAG_STRING(2,"MGC_HubIntrIrpCallback() >> ");

	if (pIntrIrp->dwStatus)
	{
		if (pHubDevice->bIntrIrpExecutionErrorCount <= MUSB_HUB_MAX_ERROR_COUNT_FOR_INTR_IRP_EXECUTION)
		{ 
			/*  IRP is not executed properly. So can not go ahead. return from here itself
			*/
			pHubDevice->bIntrIrpExecutionErrorCount++;
			MUSB_DIAG_STRING(1,"HubError: pIntrIrp->dwStatus in MGC_HubIntrIrpCallback()");
		}
		else
		{
			/* There is no use of continue with dump device. Display messge that disconnect
			* the HUb.
			*/
			pHubDevice->bState = MUSB_HUB_STATE_DISCONNECT_DEVICE;
			pHubDevice->bNextState = MUSB_HUB_STATE_DISCONNECT_DEVICE;
			MUSB_DIAG_STRING(1,"HubError: MGC_HubIntrIrpCallback () error count more then expected.");
			MUSB_DIAG_STRING(1,MGC_aDisconnectHub);
		}
		return;
	}

	/* Reset Error count. */
	pHubDevice->bIntrIrpExecutionErrorCount    = 0x00;
	MGC_HubIntrIrpHandler(pHubDevice, pIntrIrp);
	MUSB_DIAG_STRING(2,"MGC_HubIntrIrpCallback() <<");
	return;
}   /* End of function MGC_HubCtrlReqComplete () */


/*******************************************************************
 *                 MGC_HubIntrIrpHandler ()                        *
 *******************************************************************/
void                   
MGC_HubIntrIrpHandler(MUSB_Hub *pHubDevice, MUSB_Irp *pIntrIrp)
{
	OS_ERR err;
	
	MUSB_DIAG_STRING(2,"MGC_HubIntrIrpHandler() >>");
	
	/* Get status information in the uint32 field so that now onward operation
	* would be easy.
	*/   
	MUSB_BitsGet(pIntrIrp->pBuffer, 0, 32, &(pHubDevice->dwHubPortStatusChange));

	if ((pHubDevice->dwHubPortStatusChange & 0x01) && 
		(MUSB_HUB_STATE_GOT_HUB_STATUS != pHubDevice->bNextState))
	{
		MUSB_DIAG_STRING(2," Hub's Status Changed");
		/* If Hub Status Has changed then call function related to 
		* Hub Status otherwise call function related to Port status change
		*/
		MUSB_HubStatusChange (pHubDevice);
		Ecount = 0;
	}
	else if(MUSB_HUB_PORT_STATE_DEFAULT == pHubDevice->bPortNextState)
	/*
	else if((MUSB_HUB_PORT_STATE_GOT_STATUS != pHubDevice->bPortNextState) &&
	(MUSB_HUB_INVALID_PORT_NUMBER == pHubDevice->bActivePortNum))
	*/
	{
		/* call funtion which handle change in port status.
		*/
		pHubDevice->bActivePortNum = MUSB_HUB_STARTING_PORT_NUMBER;
		MGC_HubPortStatusChange(pHubDevice);
	}
	else
	{
		Ecount++;
		if(Ecount == 5)
		{
			int bPortNumber;
			
			MUSB_PRINTK("MUSB_HUB_PORT_ERROR\n"); 

			MGC_HubUpdatePower(pHubDevice, MUSB_HUB_PORT_DISCONNECTED);
			pHubDevice->bPortNextState = MUSB_HUB_PORT_STATE_DISCONNECT; 

			MUSB_HubClearPortFeature(pHubDevice, MUSB_HUB_C_PORT_CONNECTION_FEATURE, 
										MGC_HubGetPortStatusCallback, pHubDevice->bActivePortNum);
				
			for( bPortNumber = (uint8_t)MUSB_HUB_STARTING_PORT_NUMBER; bPortNumber <= pHubDevice->hubDscr.bNbrPorts; bPortNumber++)
			{
				MUSB_HubClearPortFeature(pHubDevice, MUSB_HUB_C_PORT_CONNECTION_FEATURE, 
										MGC_HubGetPortStatusCallback, bPortNumber);
				//OSTimeDly(10, OS_OPT_TIME_DLY, &err);
				OSTimeDly(10);
			}

			Ecount = 0;
		}
	}
	MUSB_DIAG_STRING(2,"MGC_HubIntrIrpHandler() <<");
	return;
}   /* End of function MGC_HubIntrIrpHandler () */


/*******************************************************************
 *                 MGC_HubDisconnectHandler ()                     *
 *******************************************************************/

void
MGC_HubDisconnectHandler(MUSB_Hub *pHubDevice, uint32_t dwHubType,
                         MUSB_HubDriverContext *pDriverContext)
{
	uint32_t dwStatus;

	MUSB_DIAG_STRING(2,"Hub: MGC_HubDisconnectHandler () >> ");
	if (0x01 == pDriverContext->bNumOfHubActive)
	{
		MUSB_DIAG_STRING(2,"Hub: RootHub is disconnected");
		/* When Root hub is disconnected then directly hub disconnected function 
		* will be called so first de-enumerate hub then release memory related to hub.
		*/
		MGC_DeEnumerateHub(pHubDevice, dwHubType);
	}

	/* Control comes here when hub is already Deenumerated. */
	if (pHubDevice->intrInPipe)
	{
		dwStatus = MUSB_ClosePipe (pHubDevice->intrInPipe);
	}

	pHubDevice->pUsbDevice = NULL;

	MUSB_DIAG_STRING(2,"Hub: MGC_HubDisconnectHandler () Complete");

	return;
}   /* End of function MGC_HubDisconnectHandler () */
 
 
/*******************************************************************
 *                 MGC_HubFindFreeHubDeviceIndex ()                *
 *******************************************************************/
uint32_t 
MGC_HubFindFreeHubDeviceIndex(MUSB_HubDriverContext *pDriverContext, uint8_t *pIndex)
{
	uint8_t bIndex;

	for (bIndex = 0x00; bIndex < MUSB_HUB_MAX_HUB_SUPPORTED; bIndex++)
	{
		if (pDriverContext->hubDeviceList[bIndex].bState == MUSB_HUB_STATE_FREE)
		{
			MUSB_DIAG_STRING(3, "HubSuccess: FreeHubDeviceIsAvailable");
			*pIndex = bIndex;
			return (MUSB_SUCCESS);
		}
	}   /*   End of function   MGC_HubFindFreeHubDeviceIndex () */

	MUSB_DIAG_STRING(1, "HubSuccess: FreeHubDeviceIsNotAvailable");
	return (MUSB_FAILURE);
}   /* End of function MGC_HubFindFreeHubDeviceIndex () */


/*******************************************************************
 *                 MGC_HubFindHubDevice ()                     *
 *******************************************************************/
uint32_t 
MGC_HubFindHubDevice (MUSB_Device *pUsbDevice, MUSB_Hub **pHubDevice)
{
	uint8_t                bIndex;
	MUSB_HubDriverContext *pDriverContext = (MUSB_HubDriverContext *) pUsbDevice->pDriverPrivateData;
	MUSB_Hub              *pHubDeviceList = pDriverContext->hubDeviceList;

	for ( bIndex = 0x00; bIndex < MUSB_HUB_MAX_HUB_SUPPORTED; bIndex++ )
	{
		if (pHubDeviceList[bIndex].pUsbDevice == pUsbDevice)
		{
			*pHubDevice = &pHubDeviceList[bIndex];
			return (MUSB_SUCCESS);
		}
	}
	return (MUSB_FAILURE);
}   /* End of function  MGC_HubFindHubDevice() */



/*******************************************************************
 *                 MGC_HubPowerValidateInit ()                     *
 *******************************************************************/
/*
 * It does the verification against the power requirements (usb2.0 spec, 11.13)
 * and initialize the parameters related to power management.
 */
uint32_t MGC_HubPowerValidateInit(MUSB_Hub *pHubDevice)
{
	uint8_t                            bConfigSelfPower;
	uint8_t                            bMaxPower;
	uint8_t                            bHubStatusLocalPower;
	MUSB_ConfigurationDescriptor      *pConfig;
	MUSB_Hub                          *pParentHubDevice;
	int32_t                            dwStatus = MUSB_SUCCESS;

	bHubStatusLocalPower =   (uint8_t) (pHubDevice->hubStatus.wStatus & MUSB_HUB_STATUS_LOCAL_POWER_BM);

	/*
	* Extract configuration,then extract self-power bit(6th bit-0x40) of the 
	* bmAttributes and bMaxPower field of configuration descriptor
	*/
	pConfig             = pHubDevice->pUsbDevice->pCurrentConfiguration;
	bConfigSelfPower    = (uint8_t)(pConfig->bmAttributes & MUSB_HUB_CONFIG_SELF_POWER_BM);
	bMaxPower           = pConfig->bMaxPower;

	if(pHubDevice->pUsbDevice->pParentUsbDevice)
	{
		dwStatus = MGC_HubFindHubDevice(pHubDevice->pUsbDevice->pParentUsbDevice, &pParentHubDevice);
		if(MUSB_SUCCESS != dwStatus)
		{
			MUSB_DIAG_STRING(1, "HubError: Parent is not a Hub");
			return(MUSB_FAILURE);
		}
		else
		{
			if( (MUSB_HUB_BUS_POWERED == pParentHubDevice->bSelfPower) && (!bConfigSelfPower) )
			{
				/* Its paraent hub is Bus powered and it is also bus powered
				it cannot driver any devices. So, bAllocatedPower is initialised to MAX*/
				MUSB_DIAG_STRING(1,"HubError: Parent and child both are bus-powered so they cannot work");
				pHubDevice->bAllocatedPower    = MUSB_HUB_BUS_POWER_MAX_UNITS;
			}
			else
			{
				pHubDevice->bAllocatedPower    = 0; /* Initialise to zero */
			}
		}
	}

	pHubDevice->bAllocatedPower    = 0; /* Initialise to zero */

	if(bConfigSelfPower) /* Self-Powered Hub */
	{
		if( (!bHubStatusLocalPower) && (!bMaxPower) )
		{
			/* 
			* A device which is only self-powered, but doesn't have local power
			* cannot connect to the bus and communicate
			*/

			MUSB_DIAG_STRING(1, "HubError: Self-powered hub status local power & max power=0");
			dwStatus = MUSB_FAILURE;
		}
		else if( (bHubStatusLocalPower) && (!bMaxPower) )
		{
			/*
			* Self-Powered only hub and local power supply is good. 
			* Hub status also indicates local power good.
			*/
			pHubDevice->bSelfPower         = MUSB_HUB_SELF_POWERED;
			pHubDevice->bLocalPower        = MUSB_HUB_LOCAL_POWERED;
		}
		else if( (!bHubStatusLocalPower) && (bMaxPower) )
		{
			/*
			* This hub is capable of both self- and bus- power operating modes.
			* it is currently available as a bus-Powered hub.
			*/
			pHubDevice->bSelfPower         = MUSB_HUB_SELF_AND_BUS_POWERED;
			pHubDevice->bLocalPower        = MUSB_HUB_NOT_LOCAL_POWERED;
		}
		else /* bHubStatusLocalPower = 1 bMaxPower > 0 */
		{
			/*
			* This hub is capable of both self- and bus- power operating modes.
			* it is currently available as a self-Powered hub.
			*/
			pHubDevice->bSelfPower         = MUSB_HUB_SELF_AND_BUS_POWERED;
			pHubDevice->bLocalPower        = MUSB_HUB_LOCAL_POWERED;
		}
	} /* End of if(bConfigSelfPower) */
	else /* Bus-Powered Hub */
	{
		if(!bMaxPower)
		{
			/* This is illegal set of information, hub cannot work with this */
			MUSB_DIAG_STRING(1,"HubError: Bus-powered max power=0");
			dwStatus = MUSB_FAILURE;
		}
		else
		{
			/* 
			* Hub device status reporting Self-Powered is meaning less 
			* in combination of a zeroed "bmAttributes.Self-Powered"
			* So, no need to check hub status 
			*/
			pHubDevice->bSelfPower         = MUSB_HUB_BUS_POWERED;
			pHubDevice->bLocalPower        = MUSB_HUB_NOT_LOCAL_POWERED;
		}
	}
	
	return(dwStatus);
} /* End of MGC_HubPowerValidateInit() */


/*******************************************************************
 *                 MGC_HubValidateUpdateTtInfo ()                  *
 *******************************************************************/
/*
 * This interprets the Operating speed of the Hub and validates the 
 * descriptors fields accordingly. It also Finds the whether it supports 
 * multiple TT or Single TT or Invalid device and stores the 
 * TT information at HUB device information
 */
uint32_t 
MGC_HubValidateUpdateTtInfo(MUSB_Hub *pHubDevice)
{
	/*
	* The value of the "bDeviceProtocol" field of the USB device descriptor 
	* and the "bInterfaceProtocol" field in the USB interface descriptor 
	* indicate whether a hub is single-TT or multi-TT: 
	* Single-TT. bDeviceProtocol == 0x01 and bInterfaceProtocol == 0
	* Multi-TT.  bDeviceProtocol == 0x02
	*    It can have two alternate settings
	*        The first interface descriptor, bInterfaceProtocol == 1. 
	*        The second interface descriptor, bInterfaceProtocol == 2.
	*  When the hub is configured with an interface protocol of one(1), 
	*   it will operate as a single TT organized hub. 
	*  When the hub is configured with an interface protocol of two(2), 
	*   it will operate as a multiple TT organized hub. 
	*/

	MUSB_InterfaceDescriptor       *pInterfaceDescriptor;
	uint8_t                         bDeviceProtocol;
	uint8_t                         bInterfaceProtocol;
	MUSB_Device                    *pUsbDevice      = pHubDevice->pUsbDevice;

	pInterfaceDescriptor = (MUSB_InterfaceDescriptor *)
	            MUSB_FindInterfaceDescriptor(
	                            pUsbDevice->pCurrentConfiguration,
	                            0, /* Interface number */
	                            pHubDevice->bAlternateSetting);
	if(!pInterfaceDescriptor)
	{
		MUSB_DIAG_STRING(1,"HubError: UpdateTTInfo-FindInfDscrFail");
		return(MUSB_FAILURE);
	}

	bDeviceProtocol        = pUsbDevice->DeviceDescriptor.bDeviceProtocol;
	bInterfaceProtocol     = pInterfaceDescriptor->bInterfaceProtocol;

	if( (0x00 == bDeviceProtocol) && (0x00 == bInterfaceProtocol) )
	{
		/* Full-/Low-Speed Operating Hub (usb2.0, 11.23.3)*/
		pHubDevice->bTtType    = MUSB_HUB_NO_TT;
		MUSB_DIAG_STRING(1,"Hub:Operating At Full-/Low-Speed-NoTT");
	}
	else if( (0x02 == bDeviceProtocol) && (0x01 == bInterfaceProtocol) )
	{
		/* 
		* Multiple-TT Hub, But Presently Hub configured as Single-TT,
		* First Bit(0) of Lower Nibble of "bTtType" represents Hub TT-Type
		* First Bit(4) of Upper Nibble of "bTtType" Presenty Hub TT configured,
		*/
		pHubDevice->bTtType = ((MUSB_HUB_MULTIPLE_TT<<4) | MUSB_HUB_SINGLE_TT);
	}
	else if( (0x02 == bDeviceProtocol) && (0x02 == bInterfaceProtocol) )
	{
		/* Multiple-TT Hub, But Presently Hub configured as Multiple-TT */
		pHubDevice->bTtType = ((MUSB_HUB_MULTIPLE_TT<<4) | MUSB_HUB_MULTIPLE_TT);
	}
	else if( (0x01 == bDeviceProtocol) && (0x00 == bInterfaceProtocol) )
	{
		/* Single-TT Hub */
		pHubDevice->bTtType    = (MUSB_HUB_SINGLE_TT<<4) | MUSB_HUB_SINGLE_TT;
	}
	else /* Undefined values for Hub */
	{
		MUSB_DIAG_STRING(1,"HubError:UpdateTTInfo-UndefinedDescriptor:");
		MUSB_DIAG1(1,"bDeviceProtocol:",bDeviceProtocol, 16, 0x00);
		MUSB_DIAG1(1,"bInfProtocol:",bInterfaceProtocol, 16, 0x00);

		pHubDevice->bTtType    = (int8_t)MUSB_HUB_INVALID_TT;
		return(MUSB_FAILURE);
	}
	return(MUSB_SUCCESS);
} /* End of MGC_HubValidateUpdateTtInfo() */



/*******************************************************************
 *                 MGC_HubUpdatePower ()                           *
 *******************************************************************/
/*
 * Assumption: Bus powered Hub always gets 5 units of load from upstream port 
 * and Bus Powered hub cannot be connected to a Bus powered hub.
 *
 * Power scheduling is required for Bus-Powered hubs only since 
 * there are some limits on the available power from upstream and number 
 * of devices to be connected.
 * Self-Powered hubs can drive any number of ports, so no need to 
 * schedule the power.
 *
 * It updates the available power and checks the newly connected device
 * working possibility
 */
uint32_t 
MGC_HubUpdatePower(MUSB_Hub        *pHubDevice,
                            uint8_t          bPowerUpdateType)
{
	uint32_t            dwStatus = MUSB_SUCCESS;

	if( (MUSB_HUB_SELF_POWERED     == pHubDevice->bSelfPower) ||
		(MUSB_HUB_LOCAL_POWERED    == pHubDevice->bLocalPower) )
	{
		/* For Self-Powered hub, Power management is not required */
	}
	else
	{
		/* Bus Powered Hub */
		switch(bPowerUpdateType)
		{
			case MUSB_HUB_PORT_CONNECTED    :
			{
				if(pHubDevice->bAllocatedPower < MUSB_HUB_BUS_POWER_MAX_UNITS)
				{
					/* Deduct one unit power from available power */
					pHubDevice->bAllocatedPower++;
				}
				else
				{
					MUSB_DIAG_STRING(1,"Hub:WarCannotSupplyLoad");
					dwStatus = MUSB_FAILURE;
				}
				break;
			}
			case MUSB_HUB_PORT_DISCONNECTED    :
			{
				if(pHubDevice->bAllocatedPower)
				{
					/* Add one unit power to available power */
					pHubDevice->bAllocatedPower--;
				}
				break;
			}
			case MUSB_HUB_PORT_SUSPENDED    :
			{
				/* 
				* usb2.0 spec 7.2.3, 500uA for low-Powered devices
				* 2.5mA for High-powred devices with remote wakeup
				* FIXME: Should we add one unit to bAllocatedPower
				*/
				break;
			}
			case MUSB_HUB_PORT_RESUMED        :
			{
				/* FIXME: This is depends on the actions taken at 
				MUSB_HUB_PORT_SUSPENDED case */
				break;
			}
			default                            :
			{
				MUSB_DIAG_STRING(1,"Hub:ErrorUnknownPowerUpdateType");
				dwStatus = MUSB_FAILURE;
				break;
			}
		} /* End of Switch */
	} /* End of else(MUSB_HUB_SELF_POWERED == pHubDevice->bSelfPower) */

	return(dwStatus);
} /* End of MGC_HubUpdatePower() */



/*******************************************************************
 *                 MGC_HubInitAndResetTt ()                        *
 *******************************************************************/
/*
 * This Initializes the TT information of the hub and
 * Reset the TT of the Hub to keep the TT in known state
 */
uint32_t 
MGC_HubInitAndResetTt(MUSB_Device  *pUsbDevice,
                               MUSB_pfControlIrpComplete ControlIrpComplete,
                               MUSB_Hub     *pHubDevice)
{
	uint32_t    dwStatus    = MUSB_FAILURE_UNSIGNED;

	/*
	* Reset the TT, to keep TT in a known state 
	* (If the Hub operating at High speed)
	*/
	dwStatus = (uint32_t)MUSB_HubResetTt(pUsbDevice, ControlIrpComplete, 
			(void *)pHubDevice, 0x01); /*First Port's RESET TT Request */
	
	if(dwStatus != TRUE)
	{
		MUSB_DIAG_STRING(1, "Hub:ErrorResetTT");
		return(MUSB_FAILURE_UNSIGNED);
	}

	return(MUSB_SUCCESS);
} /* End of MGC_HubInitAndResetTt() */


/*******************************************************************
 *                 MGC_HubIsAnyPortNotSupportsRemoteWakeup ()      *
 *******************************************************************/
/*
 * It retunrs TRUE if all the devices connected to the given hub are supporting
 * Remote wakeup. In other cases it returns FALSE
 */
uint8_t 
MGC_HubIsAnyPortNotSupportsRemoteWakeup(MUSB_Device *pUsbDevice)
{
	MUSB_Hub				*pHubDevice;
	MUSB_Device				*pChildUsbDevice;
	int32_t					dwStatus;
	uint8_t					bPortNumber;
	uint8_t					bChildIndex;

	dwStatus    = MGC_HubFindHubDevice(pUsbDevice, &pHubDevice);
	if(MUSB_SUCCESS != dwStatus)
	{
		MUSB_DIAG_STRING(1, "HubError: This Is Not A Hub Device/NULL");
		return(FALSE);
	}

	/*
	* Check all the devices connected to the Hub for remote wakeup feature
	* If any device does not support remote wakeup, return FALSE.
	* If all the devices support remote wakeup, return TRUE
	*/
	for( bPortNumber = (uint8_t)MUSB_HUB_STARTING_PORT_NUMBER; bPortNumber <= pHubDevice->hubDscr.bNbrPorts; bPortNumber++)
	{
		dwStatus = MGC_HubGetChildIndexForGivenPort(pHubDevice, bPortNumber, &bChildIndex);
		if(MUSB_HUB_CHILD_PRESENT != dwStatus) 
		{
			/* 
			* No device is connected at this port or 
			* The port is more than the supported Poert numbers(MUSB_HUB_MAX_CHILD). 
			*/
			MUSB_DIAG_STRING(1,"HubWarning: No Device connected at the Port/ Port Number>MUSB_HUB_MAX_PORTS");
		}
		else
		{
			pChildUsbDevice = pHubDevice->pChild[bChildIndex];
			if(MUSB_CLASS_HUB == pChildUsbDevice->DeviceDescriptor.bDeviceClass)
			{
				/* Hub is connected at this port number */
				if(FALSE ==  MGC_HubIsAnyPortNotSupportsRemoteWakeup(pChildUsbDevice) )
				{
					/* Some device is not supporting remote wakeup */
					break;
				}
			}
			else
			{
				if( !(pChildUsbDevice->pCurrentConfiguration->bmAttributes & MUSB_HUB_REMOTE_WAKEUP_BM) )
				{
					/*
					* The connected at the this port number is not supporting remote wakeup
					* Then No need to check remaining ports. 
					*/
					break;
				}
			}
		} /* End of if(MUSB_HUB_CHILD_PRESENT) */
	} /* End of for()*/

	if(bPortNumber > pHubDevice->hubDscr.bNbrPorts)
	{
		/*
		* All the devices in the tree are supporting Remote Wakeup 
		*/
		return(TRUE);
	}
	/* Found a device which doesn't support remote wakeup facility */
	return(FALSE);
} /* MGC_HubIsAnyPortNotSupportsRemoteWakeup() */

