/******************************************************************
*                                                                *
*        Copyright Mentor Graphics Corporation 2004              *
*                                                                *
*                All Rights Reserved.                            *
*                                                                *
*    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION *
*  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS   *
*  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                    *
*                                                                *
******************************************************************/

/*
 * Extra function-specific functionality.
 * $Revision: 1.1 $
 */

#include "mu_impl.h"
#include "mu_funpr.h"
#include "types.h"

typedef int (*App_Connect_Event)(void *pPrivateData);

static App_Connect_Event	MUSB_Connet_Event=NULL;
static App_Connect_Event	MUSB_Disconnet_Event=NULL;


MUSB_Pipe MUSB_BindFunctionEnd(MUSB_BusHandle hBus, 
			       const MUSB_EndpointDescriptor* pEndDesc)
{
	MGC_Pipe* pPipe;
	MUSB_DeviceEndpoint Endpoint;
	MGC_EndpointResource* pResource;
	MGC_Port* pPort = (MGC_Port*)hBus;

	pPipe = (MGC_Pipe*)MUSB_MemAlloc(sizeof(MGC_Pipe));
	if(pPipe)
	{
		MUSB_MemCopy((void*)&(Endpoint.UsbDescriptor), (void*)pEndDesc, sizeof(MUSB_EndpointDescriptor));
		pResource = pPort->pfBindEndpoint(pPort, &Endpoint, NULL, TRUE);
		if(pResource)
		{
			MGC_FunctionPreparePipe(pPort, pPipe, (MUSB_BusHandle)pPort, pResource, pEndDesc);
		}
		else
		{
			MUSB_MemFree((void*)pPipe);
			pPipe = NULL;
		}
	}

	return (MUSB_Pipe)pPipe;
}

void MUSB_CloseFunctionPipe(MUSB_Pipe hPipe)
{
	MGC_Pipe* pPipe = (MGC_Pipe*)hPipe;
	MGC_Port* pPort = (MGC_Port*)pPipe->hSession;

	pPort->pfProgramFlushEndpoint(pPort, pPipe->pLocalEnd, 
		(pPipe->bmFlags & MGC_PIPEFLAGS_TRANSMIT) ? MUSB_DIR_IN : MUSB_DIR_OUT, FALSE);
	MUSB_MemFree((void*)pPipe);
	pPipe = NULL;		
}


void MUSB_User_App_Register_Connect_Ind(int (*USER_Connect_Event)(void *pPrivateData))
{
	MUSB_Connet_Event = USER_Connect_Event;
}

void MUSB_User_App_Register_Discon_Ind(int (*USER_Discon_Event)(void *pPrivateData))
{
	MUSB_Disconnet_Event = USER_Discon_Event;
}

/*****************************************************************************
 * FUNCTION   : MUSB_User_App_Connect_Ind()
 * PURPOSE    : This is a Registered Function provided by the application
 *              to each client driver to be called when corresponding device
 *              is connected. Using different function for different client,
 *              application writer can have very generic application 
 *              implementation. In current demo, this function is registered
 *              for all clients.
 * PARAMETERS : pPrivateData : Client pPrivateData exp:"MSD_CLIENT"
 *              device being connected
 * RETURNS    : RETURNS    : TRUE or FALSE
 * CALLED BY  : Class Driver Thread to whom it is registered. (Registered
 *              at the time of usb_client structure initialization)
 *****************************************************************************/
uint8_t MUSB_User_App_Connect_Ind(void *pPrivateData)
{
	/* Check whether there is already a device connected or not.
	* If Yes, then without getting a disconnect, second invocation
	* of Connect should not be handled
	*/
	if(MUSB_Connet_Event != NULL)
	{
		MUSB_Connet_Event(pPrivateData);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*****************************************************************************
 * FUNCTION   : MUSB_User_App_Discon_Ind()
 * PURPOSE    : This is a Registered Function provided by the application
 *              to each client driver to be called when corresponding device
 *              is dis-connected. Using different function for different client
 *              application writer can have very generic application 
 *              implementation. In current demo, this function is registered
 *              for all clients.
 * PARAMETERS : PARAMETERS : pPrivateData : Client pPrivateData exp:"MSD_CLIENT"
 *              device being disconnected
 * RETURNS    : TRUE or FALSE
 * CALLED BY  : Class Driver Thread to whom it is registered. (Registered
 *              at the time of usb_client structure initialization)
 *****************************************************************************/
uint8_t MUSB_User_App_Discon_Ind(void *pPrivateData)
{
	if(MUSB_Disconnet_Event != NULL)
	{
		MUSB_Disconnet_Event(pPrivateData);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
