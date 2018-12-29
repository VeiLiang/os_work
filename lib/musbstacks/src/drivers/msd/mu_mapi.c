/******************************************************************
*                                                                *
*        Copyright Mentor Graphics Corporation 2005              *
*                                                                *
*                All Rights Reserved.                            *
*                                                                *
*    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION *
*  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS   *
*  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                    *
*                                                                *
******************************************************************/

/*
 * Top-level mass-storage class driver module
 * $Revision: 1.7 $
 */

#include "mu_bits.h"
#include "mu_cdi.h"
#include "mu_descs.h"
#include "mu_diag.h"
#include "mu_hfi.h"
#include "mu_mem.h"
#include "mu_stdio.h"

#include "mu_mpcsi.h"
#include "mu_msd.h"

#include "mu_mapi.h"

extern MGC_MsdCmdSet* MGC_GetScsiCmdSet(void);
extern MGC_MsdProtocol* MGC_CreateBotInstance(MUSB_Device* pDevice,
					      MUSB_BusHandle hBus,
					      const MUSB_InterfaceDescriptor* pIfaceDesc,
					      const MUSB_EndpointDescriptor* pInEnd,
					      const MUSB_EndpointDescriptor* pOutEnd,
					      MGC_MsdCmdSet* pCmdSet);
extern void MGC_DestroyBotInstance(MGC_MsdProtocol* pProtocol);

void MUSB_MsdBusResumed(void *pPrivateData, MUSB_BusHandle hBus);

static uint32_t MGC_MsdConfigureDevice(MGC_MsdDevice *pMsdDevice);
static void MGC_MsdSetConfigCallback(void* pContext, MUSB_ControlIrp *pControlIrp);

/**************************** GLOBALS *****************************/
static unsigned char MGC_aMsdPresent = 0;

static uint32_t MGC_aMsdTimerResolution[1] = { 10 };

static const uint8_t MGC_aMsdPeripheralList[] =
{
    MUSB_TARGET_CLASS, MUSB_CLASS_PER_INTERFACE, 
    MUSB_TARGET_INTERFACE, 0, 
    MUSB_TARGET_CLASS, MUSB_CLASS_MASS_STORAGE, 
    MUSB_TARGET_SUBCLASS, MGC_MSD_SCSI_SUBCLASS,
    MUSB_TARGET_PROTOCOL, MGC_MSD_BOT_PROTOCOL,
    MUSB_TARGET_ACCEPT
};

/** Mass Storage device driver */
MUSB_DeviceDriver MGC_MsdDeviceDriver =
{
    "MSD_CLIENT",
    sizeof(MGC_aMsdTimerResolution) / sizeof(uint32_t),
    MGC_aMsdTimerResolution,
    MUSB_MsdConnect,
    MUSB_MsdDisconnect,
    NULL,
    NULL,
    MUSB_User_App_Connect_Ind,
    MUSB_User_App_Discon_Ind
};

/*************************** FUNCTIONS ****************************/

uint16_t MGC_FillStorageClassPeripheralList(uint8_t* pList, uint16_t wListLength)
{
	uint16_t wResult = MUSB_MIN((uint16_t)sizeof(MGC_aMsdPeripheralList), wListLength);

	MUSB_MemCopy(pList, MGC_aMsdPeripheralList, wResult);
	return wResult;
}

MUSB_DeviceDriver* MGC_GetStorageClassDriver(void)
{
	return &MGC_MsdDeviceDriver;
}

/**
 * Allocate per-device data
 */
static MGC_MsdDevice* MGC_MsdDeviceInit(MUSB_Device* pUsbDevice)
{
	MGC_MsdDevice            *pMsdDevice;

	pMsdDevice = (MGC_MsdDevice *)MUSB_MemAlloc (sizeof(MGC_MsdDevice) );
	if (NULL == pMsdDevice) 
	{   
		MUSB_DIAG_STRING(MUSB_MSD_DIAG_ERROR, "MSD Error: Insufficient memory");
		return (NULL);
	}
	MUSB_MemSet((void *)pMsdDevice, 0, sizeof (MGC_MsdDevice));

	pMsdDevice->pDriver = &MGC_MsdDeviceDriver;

	MUSB_DIAG_STRING(MUSB_MSD_DIAG_SUCCESS, "MSD: Initialization Completed");

	return pMsdDevice;
}

/** This function is called when Mass Storage device is connected.*/
uint8_t 
MUSB_MsdConnect(  void              *pPrivateData, 
                  MUSB_BusHandle     hBus,
                  MUSB_Device       *pUsbDevice,
                  const uint8_t     *pPeripheralList)
{
	uint8_t bIndex, bEnd;
	uint8_t bSubclass, bProtocol;
	const MUSB_InterfaceDescriptor* pIface;
	const MUSB_EndpointDescriptor* pEnd;
	const MUSB_EndpointDescriptor* pInEnd;
	const MUSB_EndpointDescriptor* pOutEnd;
	MGC_MsdDevice	         *pMsdDevice;
	MGC_MsdProtocol* pProtocol = NULL;
	MGC_MsdCmdSet* pCmdSet = NULL;
	/* assume first config */
	const MUSB_ConfigurationDescriptor* pConfig = pUsbDevice->apConfigDescriptors[0];
	uint8_t bStatus = FALSE;

	/* Device is connected */		
	MUSB_PRINTK("MUSB_MsdConnect \n");
	
	/* find first interface with supported subclass/protocol combination */
	for(bIndex = 0; bIndex < pConfig->bNumInterfaces; bIndex++)
	{
		/* assume no alternates */
		pIface = MUSB_FindInterfaceDescriptor(pConfig, bIndex, 0);
		if(pIface && (MUSB_CLASS_MASS_STORAGE == pIface->bInterfaceClass) && (pIface->bNumEndpoints >= 2))
		{
			bSubclass = pIface->bInterfaceSubClass;
			bProtocol = pIface->bInterfaceProtocol;
			pInEnd = pOutEnd = NULL;
			if(MGC_MSD_BOT_PROTOCOL == bProtocol)
			{
				/* BOT: find bulk-in & bulk-out ends */
				for(bEnd = 0; bEnd < pIface->bNumEndpoints; bEnd++)
				{
					pEnd = MUSB_FindEndpointDescriptor(pConfig, pIface, bEnd);
					if(pEnd && 
						(MUSB_ENDPOINT_XFER_BULK == (pEnd->bmAttributes & MUSB_ENDPOINT_XFERTYPE_MASK)))
					{
						if(MUSB_ENDPOINT_DIR_MASK & pEnd->bEndpointAddress)
						{
							pInEnd = pEnd;
						}
						else
						{
							pOutEnd = pEnd;
						}
					}
				}
				/* continue if not found */
				if(!pInEnd || !pOutEnd)
				{
					MUSB_DIAG1(1, "MSD: insufficient endpoints for BOT in interface ", bIndex, 10, 0);
					continue;
				}
				/* determine command-set or continue on unsupported */
				switch(bSubclass)
				{
					case MGC_MSD_SCSI_SUBCLASS:
						pCmdSet = MGC_GetScsiCmdSet();
					break;
					
					default:
					continue;
				}
				pProtocol = MGC_CreateBotInstance(pUsbDevice, hBus, pIface, pInEnd, pOutEnd, pCmdSet);
			}
		}
	}

	if(pProtocol)
	{
		pMsdDevice = MGC_MsdDeviceInit(pUsbDevice);
		if(pMsdDevice)
		{
			pMsdDevice->pProtocol = pProtocol;
			pUsbDevice->pDriverPrivateData = pMsdDevice;

			/* Initialize */
			pMsdDevice->pUsbDevice     = pUsbDevice;
			pMsdDevice->hBus           = hBus;

			/* Configure the device */
			bStatus = (uint8_t) MGC_MsdConfigureDevice(pMsdDevice);
			if (MUSB_STATUS_OK == bStatus)
			{	
				MUSB_DeviceDriver* pMsdDriver;
				MUSB_PRINTK("Configure the device MUSB_STATUS_OK\n");	
				if(lg_Usb_Present)
				{
					pMsdDriver = MGC_GetStorageClassDriver();
					MUSB_User_App_Connect_Ind(pMsdDriver->pPrivateData);  
					MGC_aMsdPresent = 1;
				}
				bStatus = TRUE;
			}
		}
	}
	else
	{
		MUSB_DIAG_STRING(MUSB_MSD_DIAG_ERROR, "MSD Error: No interface has supported subclass/protocol");
	}

	MUSB_PRINTK("MUSB_MsdConnect is 0x%x\n", bStatus);

	return (bStatus); 
}/* End MUSB_MsdConnect() */

/** Disconnect Handler for Mass Storage Device Driver */
void MUSB_MsdDisconnect (void           *pPrivateData, 
                         MUSB_BusHandle  hBus,
                         MUSB_Device    *pUsbDevice)
{
	MGC_MsdDevice		*pMsdDevice;
	MUSB_DeviceDriver	*pMsdDriver;
	
	pMsdDevice = (MGC_MsdDevice *)pUsbDevice->pDriverPrivateData;

	/* Check against the USB device and bus handle */
	if( (hBus != pMsdDevice->hBus) || (pUsbDevice != pMsdDevice->pUsbDevice) )
	{
		MUSB_DIAG_STRING(MUSB_MSD_DIAG_ERROR,"MSD Error: MSD Device Disconnect Callback");
		MUSB_DIAG_STRING(MUSB_MSD_DIAG_ERROR,"MSD Error: Wrong 'Usb Device' and 'Bus' handles ");
		return;
	}
	
	MUSB_PRINTK("MUSB_MsdDisconnect \n");
	
	/* Release the resources held */
	MGC_DestroyBotInstance(pMsdDevice->pProtocol);

	if(MGC_aMsdPresent)
	{
		pMsdDriver = MGC_GetStorageClassDriver();
		MUSB_User_App_Discon_Ind(pMsdDriver->pPrivateData);  
		MGC_aMsdPresent = 0;
	}
	
	MUSB_MemFree(pMsdDevice);
	pMsdDevice = NULL;	
	pUsbDevice->pDriverPrivateData = NULL;

	MUSB_DIAG_STRING(MUSB_MSD_DIAG_SUCCESS,"Mass Storage Device Disconnected Successfully");
	return;
}/* End MUSB_MsdDisconnect () */

/** After getting connect callback,  device driver calls this function to configure device */
static uint32_t MGC_MsdConfigureDevice(MGC_MsdDevice *pMsdDevice)
{
	MUSB_DeviceRequest      *pSetup;
	MUSB_ControlIrp         *pControlIrp;
	uint32_t                 dwStatus;

	pSetup          = &(pMsdDevice->Setup);
	pControlIrp     = &(pMsdDevice->ControlIrp);

	MUSB_PRINTK("MGC_MsdConfigureDevice \n");

	/** Prepare the Setup Packet for sending Set Config Request */
	MGC_MSD_PREPARE_SETUP_PACKET(pSetup,
		                         (MUSB_DIR_OUT| MUSB_TYPE_STANDARD | MUSB_RECIP_DEVICE),
		                         MUSB_REQ_SET_CONFIGURATION,
		                         1,
		                         0,
		                         0);
	/** Fill Control Irp */
	MGC_MSD_FILL_CONTROL_IRP( pMsdDevice,
		                      pControlIrp,
		                      (uint8_t *)pSetup,
		                      sizeof(MUSB_DeviceRequest),
		                      NULL,
		                      0,
		                      MGC_MsdSetConfigCallback);

	//Nicholas Xu
#if 1
	if((pMsdDevice->pUsbDevice) && (pMsdDevice->pUsbDevice->pPort))
#endif
	{
		dwStatus = MUSB_StartControlTransfer (pMsdDevice->pUsbDevice->pPort, pControlIrp);

		//快速插拔会出问题所以加的延迟
		OSTimeDly(50);
		
		if (dwStatus)
		{
			/* Log an Error and return state */
			MUSB_DIAG1(MUSB_MSD_DIAG_ERROR, "MSD Error: Set Configuration Request failed, dwStatus: ", dwStatus, 16, FALSE);
		}
	}
	return (dwStatus);
}/* End MGC_MsdConfigureDevice () */

/** Callback function when device acknowledges set config reqeust. */
static void MGC_MsdSetConfigCallback(void* pContext, MUSB_ControlIrp *pControlIrp)
{
	MGC_MsdDevice            *pMsdDevice = (MGC_MsdDevice  *) pContext;

	if (MUSB_STATUS_OK != pControlIrp->dwStatus)
	{
		MUSB_DIAG1(MUSB_MSD_DIAG_ERROR, "MSD Error: Set Config Callback Status", pControlIrp->dwStatus, 10, FALSE);
		MUSB_RejectDevice(pMsdDevice->hBus, pMsdDevice->pUsbDevice);
		return;
	}
	/* Set the Current Configuration Descriptor to Default as Set Config is Success */
	pMsdDevice->pUsbDevice->pCurrentConfiguration = pMsdDevice->pUsbDevice->apConfigDescriptors[0];

	/* Device is connected */
	MUSB_DIAG_STRING(MUSB_MSD_DIAG_SUCCESS, "Mass Storage Device Configured Successfully");
	MUSB_PRINTK("Mass Storage Device Configured Successfully \n");    
	if(!pMsdDevice->pProtocol->pfStartDevice(pMsdDevice->pProtocol->pProtocolData, pMsdDevice->pUsbDevice))
	{
		MUSB_DIAG_STRING(MUSB_MSD_DIAG_ERROR, "MSD Error: Failed to start device");
	}
}
