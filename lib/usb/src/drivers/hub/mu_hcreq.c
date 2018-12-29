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
 * Implementation of hub-class-specific requests
 * $Revision: 1.1 $
 */
    
#include "mu_cdi.h"
#include "mu_diag.h"
#include "mu_bits.h"
#include "mu_mem.h"
#include "mu_descs.h"
#include "mu_hdef.h"
#include "mu_hub.h"
#include "mu_hcore.h"
#include "mu_hcreq.h"


/*******************************************************************
 *                 MUSB_HubGetStatus ()                            *
 *******************************************************************/
uint32_t 
MUSB_HubGetStatus (MUSB_Hub *pHubDevice, MUSB_pfControlIrpComplete complete, 
                   uint8_t bPortNum, uint8_t bmRequest)
{
	MUSB_ControlIrp     *pCtrlIrp       = &(pHubDevice->ctrlIrp);

	uint32_t            dwStatus     = MUSB_FAILURE_UNSIGNED;
	uint16_t            wLength      = (uint16_t)MUSB_HUB_PORT_STATUS_LENGTH;

	MUSB_DIAG_STRING(3,"MUSB_HubGetStatus() >>");

	/* Port number always starts from one. If it is 0x00 it means request
	* ment for HUB not for port.
	*/
	MUSB_HubPrepareSetupPkt (&(pHubDevice->setupPacket), 
						bmRequest, MUSB_REQ_GET_STATUS, 
						0x00, (uint16_t)bPortNum, wLength);

	pCtrlIrp->pInBuffer         =  pHubDevice->aHubPortStatus;
	pCtrlIrp->dwInLength        =  MUSB_HUB_PORT_STATUS_LENGTH;
	pCtrlIrp->dwOutLength       =  MUSB_HUB_SETUP_PACKET_LENGTH;
	pCtrlIrp->pfIrpComplete     =  complete;

	dwStatus = MUSB_StartControlTransfer (pHubDevice->pUsbDevice->pPort,pCtrlIrp);

	if (dwStatus)
	{
		/* Print the status and then come aback.*/ 
		MUSB_DIAG_STRING(1,"HubError: MUSB_HubGetStatus() <<"); 
		return (MUSB_FAILURE_UNSIGNED);
	}

	MUSB_DIAG_STRING(1,"HubSuccwaa: MUSB_HubGetStatus() <<");
	return (MUSB_SUCCESS);
}   /* End of function  MUSB_HubGetStatus ()  */


/*******************************************************************
 *                 MUSB_HubFeatureRequest ()                       *
 *******************************************************************/
uint32_t 
MUSB_HubFeatureRequest (MUSB_Hub *pHubDevice, uint16_t wFeatureSelector,
                        MUSB_pfControlIrpComplete complete, uint8_t bPortNum, 
                        uint8_t bmRequest, uint8_t bRequest)
{
	MUSB_ControlIrp     *pCtrlIrp    = &(pHubDevice->ctrlIrp);
	uint16_t            wLength      = MUSB_HUB_NO_DATA_PHASE;
	uint32_t            dwStatus; 

	MUSB_DIAG_STRING(3,"MUSB_HubFeatureRequest() >>"); 
	/* Port number always starts from 0x01. If it is 0x00 it means request
	* ment for HUB not for port.
	*/
	MUSB_HubPrepareSetupPkt (&(pHubDevice->setupPacket), 
						bmRequest, bRequest, 
						wFeatureSelector, (uint16_t)bPortNum, 
						wLength);

	pCtrlIrp->pInBuffer         =  NULL;
	pCtrlIrp->dwInLength        =  MUSB_HUB_NO_DATA_PHASE;
	pCtrlIrp->dwOutLength       =  MUSB_HUB_SETUP_PACKET_LENGTH;
	pCtrlIrp->pfIrpComplete     =  complete;

	dwStatus = MUSB_StartControlTransfer (pHubDevice->pUsbDevice->pPort, pCtrlIrp);

	if (dwStatus)
	{
		/* Print the status and then come aback.*/
		MUSB_DIAG_STRING(1,"HubError: MUSB_HubFeatureRequest() <<"); 
		return (MUSB_FAILURE);
	}
	MUSB_DIAG_STRING(3,"HubSuccess: MUSB_HubFeatureRequest(): <<");
	return (MUSB_SUCCESS);
}   /* End of function  MUSB_HubFeatureRequest () */



