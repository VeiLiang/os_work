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
 * IPOD driver top-layer module (UCD callbacks and API)
 * $Revision: 1.1 $
 */

#include "mu_cdi.h"
#include "mu_descs.h"
#include "mu_diag.h"
#include "mu_mem.h"

#include "mu_iapi.h"
#include "mu_ictrl.h"

/******************************* FORWARDS ********************************/
static uint8_t MUSB_IpodConnect(void *pPrivateData, 
                  MUSB_BusHandle     hBus,
                  MUSB_Device       *pUsbDevice,
                  const uint8_t     *pPeripheralList);
                  
static void MUSB_IpodDisconnect (void *pPrivateData, 
					   MUSB_BusHandle  hBus,
					   MUSB_Device	  *pUsbDevice);

static uint32_t MGC_IpodConfigure0Device(MGC_IpodDevice *pIpodDevice);
static uint32_t MGC_IpodConfigure1Device(MGC_IpodDevice *pIpodDevice);
static uint32_t MGC_OpenIpodDevice(MGC_IpodDevice *pIpodDevice);
static uint32_t MGC_IpodFillAndSubmitIntrIrp(MGC_IpodDevice *pIpodDevice);
/**************************** GLOBALS *****************************/
//static OS_SEM lg_IpodDeviceSem = {0};
//static OS_CSEMA * lg_IpodDeviceSem = NULL;
//OS_SEM *IpodDeviceSem = NULL;
OS_EVENT *IpodDeviceSem = NULL;

static MGC_IpodDevice *MGC_IpodDeviceContext = NULL;

static uint32_t MGC_aIpodTimerResolution[1] = { 10 };

static const uint8_t MGC_aIpodPeripheralList[] =
{
    MUSB_TARGET_CLASS, MUSB_CLASS_PER_INTERFACE, 
    MUSB_TARGET_VID, MUSB_IPOD_VID_L, MUSB_IPOD_VID_H,
    MUSB_TARGET_ACCEPT
};

MUSB_DeviceDriver MGC_IpodClassDriver =
{
    "IPOD_CLIENT",
    sizeof(MGC_aIpodTimerResolution) / sizeof(uint32_t),
    MGC_aIpodTimerResolution,
    MUSB_IpodConnect,
    MUSB_IpodDisconnect,
    NULL,
    NULL,
    MUSB_User_App_Connect_Ind,
    MUSB_User_App_Discon_Ind
};

/*************************** FUNCTIONS ****************************/

uint16_t MGC_FillIpodClassPeripheralList(uint8_t* pList, uint16_t wListLength)
{
	uint16_t wResult = MUSB_MIN((uint16_t)sizeof(MGC_aIpodPeripheralList), wListLength);

	MUSB_MemCopy(pList, MGC_aIpodPeripheralList, wResult);
	return wResult;
}

MUSB_DeviceDriver* MGC_GetIpodClassDriver(void)
{
	return &MGC_IpodClassDriver;
}

MGC_IpodDevice* MGC_GetIpodDeviceContext(void)
{
	return MGC_IpodDeviceContext;
}

/**
 * Allocate per-device data
 */
static MGC_IpodDevice* MGC_IpodDeviceInit(MUSB_Device* pUsbDevice)
{
	MGC_IpodDevice 		*pIpodDevice;
	OS_ERR err;

	IpodDeviceSem = OSSemCreate(0);
	if(!IpodDeviceSem)
	//OSSemCreate(&lg_IpodDeviceSem, NULL, 0, &err);
	//if(OS_ERR_NONE != err)
	{
		MUSB_PRINTK("IPOD Error: IpodDeviceSem is NULL\n");
		return (NULL);
	}
	//else
		//IpodDeviceSem = &lg_IpodDeviceSem;
	
	pIpodDevice = (MGC_IpodDevice *)MUSB_MemAlloc (sizeof(MGC_IpodDevice));
	if(NULL == pIpodDevice) 
	{	
		MUSB_PRINTK("IPOD Error: Insufficient memory\n");
		return (NULL);
	}
	MUSB_MemSet((void *)pIpodDevice, 0, sizeof(MGC_IpodDevice));
	
	pIpodDevice->pDriver 				= &MGC_IpodClassDriver;
	pIpodDevice->pUsbDevice			= pUsbDevice;
	
	MUSB_PRINTK("IPOD: Initialization Completed\n");
	return pIpodDevice;
}

/** This function is called when IPOD device is connected.*/
static uint8_t MUSB_IpodConnect(  void              *pPrivateData, 
                  MUSB_BusHandle     hBus,
                  MUSB_Device       *pUsbDevice,
                  const uint8_t     *pPeripheralList)
{
	uint8_t bConfig, bIndex, bEndIndex;
	const MUSB_InterfaceDescriptor* pIface;
	const MUSB_EndpointDescriptor* pEnd;
	const MUSB_ConfigurationDescriptor* pConfig;	
	MGC_IpodDevice			*pIpodDevice;
	uint8_t bStatus = FALSE;
	OS_ERR err;
	
	/* Device is connected */
	MUSB_PRINTK("IPOD Device Connected \n");
	pIpodDevice = MGC_IpodDeviceInit(pUsbDevice);
	if(!pIpodDevice)
		return bStatus;
	else
		MGC_IpodDeviceContext = pIpodDevice;

	pUsbDevice->pDriverPrivateData = pIpodDevice;
	/* Initialize */
	pIpodDevice->pUsbDevice     = pUsbDevice;
	pIpodDevice->hBus           = hBus;

	MUSB_PRINTK("bNumConfigurations is 0x%x\n", pUsbDevice->DeviceDescriptor.bNumConfigurations);

	for(bConfig = 0; bConfig < pUsbDevice->DeviceDescriptor.bNumConfigurations; bConfig++)
	{
		/* assume first config */
		pConfig = pUsbDevice->apConfigDescriptors[bConfig];
	
#if 0
		MUSB_PRINTK("=====pConfig index is 0x%x=====\n", bConfig);
		MUSB_PRINTK("bLength is 0x%x\n", pConfig->bLength);
		MUSB_PRINTK("bDescriptorType is 0x%x\n", pConfig->bDescriptorType);
		MUSB_PRINTK("wTotalLength is 0x%x\n", pConfig->wTotalLength);
		MUSB_PRINTK("bNumInterfaces is 0x%x\n", pConfig->bNumInterfaces);
		MUSB_PRINTK("bConfigurationValue is 0x%x\n", pConfig->bConfigurationValue);
		MUSB_PRINTK("iConfiguration is 0x%x\n", pConfig->iConfiguration);
		MUSB_PRINTK("bmAttributes is 0x%x\n", pConfig->bmAttributes);
		MUSB_PRINTK("bMaxPower is 0x%x\n", pConfig->bMaxPower);
		MUSB_PRINTK("------------------------------------------\n");
#endif
		/* find first interface with supported subclass/protocol combination */
		for(bIndex = 0; bIndex < pConfig->bNumInterfaces; bIndex++)
		{
			pIface = MUSB_FindInterfaceDescriptor(pConfig, bIndex, 0);
			
#if 0
			MUSB_PRINTK("=====pIface index is 0x%x=====\n", bIndex);
			MUSB_PRINTK("bLength is 0x%x\n", pIface->bLength);
			MUSB_PRINTK("bDescriptorType is 0x%x\n", pIface->bDescriptorType);
			MUSB_PRINTK("bInterfaceNumber is 0x%x\n", pIface->bInterfaceNumber);
			MUSB_PRINTK("bAlternateSetting is 0x%x\n", pIface->bAlternateSetting);
			MUSB_PRINTK("bNumEndpoints is 0x%x\n", pIface->bNumEndpoints);
			MUSB_PRINTK("bInterfaceClass is 0x%x\n", pIface->bInterfaceClass);
			MUSB_PRINTK("bInterfaceSubClass is 0x%x\n", pIface->bInterfaceSubClass);
			MUSB_PRINTK("bInterfaceProtocol is 0x%x\n", pIface->bInterfaceProtocol);
			MUSB_PRINTK("iInterface is 0x%x\n", pIface->iInterface);
			MUSB_PRINTK("=======================================\n");
#endif

			if(pIface)
			{
				for(bEndIndex = 0; bEndIndex < pIface->bNumEndpoints; bEndIndex++)
				{
					pEnd = MUSB_FindEndpointDescriptor(pConfig, pIface, bEndIndex);

#if 0			
					MUSB_PRINTK("=====pEnd index is 0x%x=====\n", bEndIndex);
					MUSB_PRINTK("bLength is 0x%x\n", pEnd->bLength);
					MUSB_PRINTK("bDescriptorType is 0x%x\n", pEnd->bDescriptorType);
					MUSB_PRINTK("bEndpointAddress is 0x%x\n", pEnd->bEndpointAddress);
					MUSB_PRINTK("bmAttributes is 0x%x\n", pEnd->bmAttributes);
					MUSB_PRINTK("wMaxPacketSize is 0x%x\n", pEnd->wMaxPacketSize);
					MUSB_PRINTK("bInterval is 0x%x\n", pEnd->bInterval);
					MUSB_PRINTK("=======================================\n");
#endif						
					if(pEnd->bmAttributes == INT_TRANSFER_TYPE)
					{
						((MUSB_EndpointDescriptor *)(&pIpodDevice->pIntEndpoint))->bLength = pEnd->bLength;
						((MUSB_EndpointDescriptor *)(&pIpodDevice->pIntEndpoint))->bDescriptorType = pEnd->bDescriptorType;
						((MUSB_EndpointDescriptor *)(&pIpodDevice->pIntEndpoint))->bEndpointAddress = pEnd->bEndpointAddress;
						((MUSB_EndpointDescriptor *)(&pIpodDevice->pIntEndpoint))->bmAttributes = pEnd->bmAttributes;
						((MUSB_EndpointDescriptor *)(&pIpodDevice->pIntEndpoint))->wMaxPacketSize = pEnd->wMaxPacketSize;
						((MUSB_EndpointDescriptor *)(&pIpodDevice->pIntEndpoint))->bInterval = pEnd->bInterval;
					}
				}
			}
			else
			{
				MUSB_PRINTK("=====pIface is NULL=====\n");
				break;
			}
		}
	}
	
	/* Configure the device */
	bStatus = (uint8_t)MGC_IpodConfigure0Device(pIpodDevice);
	if(MUSB_STATUS_OK == bStatus)
	{
		MUSB_PRINTK("Configure0 the device MUSB_STATUS_OK\n");
		//OSTimeDly(50, OS_OPT_TIME_DLY, &err);
		OSTimeDly(50);
		
		bStatus = (uint8_t)MGC_IpodConfigure1Device(pIpodDevice);
		if(MUSB_STATUS_OK == bStatus)
		{
			MUSB_DeviceDriver	*pIpodDriver;
			MUSB_PRINTK("Configure1 the device MUSB_STATUS_OK\n");
			/* Set the Current Configuration Descriptor to Default as Set Config is Success */
			pIpodDevice->pUsbDevice->pCurrentConfiguration = pIpodDevice->pUsbDevice->apConfigDescriptors[0];
			
			bStatus = (uint8_t)MGC_IpodFillAndSubmitIntrIrp(pIpodDevice);
			if(MUSB_STATUS_OK == bStatus)
			{
				MUSB_PRINTK("MGC_IpodFillAndSubmitIntrIrp OK\n");
				pIpodDriver = MGC_GetIpodClassDriver();
				MUSB_User_App_Connect_Ind(pIpodDriver->pPrivateData);  
			}
			else
			{
				MUSB_PRINTK("MGC_IpodFillAndSubmitIntrIrp ERROR\n");
				MUSB_RejectDevice(pIpodDevice->hBus, pIpodDevice->pUsbDevice);
			}
		}
		else
		{
			MUSB_PRINTK("Configure1 the device MUSB_STATUS_ERROR\n");
			MUSB_RejectDevice(pIpodDevice->hBus, pIpodDevice->pUsbDevice);
		}
	}
	else
	{
		MUSB_PRINTK("Configure0 the device MUSB_STATUS_ERROR\n");
		MUSB_RejectDevice(pIpodDevice->hBus, pIpodDevice->pUsbDevice);
	}
		
	MUSB_PRINTK("MUSB_IpodConnect is 0x%x\n", bStatus);
	return (bStatus); 
}/* End MUSB_IpodConnect() */

/** Disconnect Handler for IPOD Device Driver */
static void MUSB_IpodDisconnect (void           *pPrivateData, 
                         MUSB_BusHandle  hBus,
                         MUSB_Device    *pUsbDevice)
{
	MUSB_DeviceDriver 	*pIpodDriver = MGC_GetIpodClassDriver();
	MGC_IpodDevice		*pIpodDevice;
	//OS_ERR				myerr;
	INT8U				myerr;
	
	pIpodDevice = (MGC_IpodDevice *)pUsbDevice->pDriverPrivateData;

	/* Check against the USB device and bus handle */
	if( (hBus != pIpodDevice->hBus) || (pUsbDevice != pIpodDevice->pUsbDevice) )
	{
		MUSB_PRINTK("IPOD Error: IPOD Device Disconnect Callback\n");
		return;
	}
	
	//Nicholas Xu
#if 1
	if(pIpodDevice->InterruptPipe)
		MUSB_ClosePipe(pIpodDevice->InterruptPipe);
#else
	MUSB_ClosePipe(pIpodDevice->InterruptPipe);
#endif
	
	if(IpodDeviceSem)
	{
		OSSemDel(IpodDeviceSem, OS_OPT_DEL_NO_PEND, &myerr);
		IpodDeviceSem = NULL;
	}
	
	MGC_IpodDeviceContext = NULL;
	MUSB_MemFree(pIpodDevice);
	pIpodDevice = NULL;
	pUsbDevice->pDriverPrivateData = NULL;

	MUSB_User_App_Discon_Ind(pIpodDriver->pPrivateData);  
	
	MUSB_PRINTK("IPOD Device Disconnected Successfully\n");	
	return;
}/* End MUSB_IpodDisconnect () */


/** After getting connect callback,  device driver calls this function to configure device */
static uint32_t MGC_IpodConfigure0Device(MGC_IpodDevice *pIpodDevice)
{
    MUSB_DeviceRequest      *pSetup;
    MUSB_ControlIrp         *pControlIrp;
    uint32_t			dwStatus;

    pSetup		= &(pIpodDevice->SetupPacket);
    pControlIrp	= &(pIpodDevice->ControlIrp);

    MUSB_PRINTK("MGC_IpodConfigure0Device \n");

    /** Prepare the Setup Packet for sending Set Config Request */
	MGC_IPOD_PREPARE_SETUP_PACKET(pSetup,
								 MUSB_DIR_OUT | MUSB_TYPE_STANDARD | MUSB_RECIP_DEVICE,
								 MUSB_REQ_SET_CONFIGURATION,
								 2,
								 0,
								 0);
	/** Fill Control Irp */
	MGC_IPOD_FILL_CONTROL_IRP(pIpodDevice,
							  pControlIrp,
							  (uint8_t *)pSetup,
							  sizeof(MUSB_DeviceRequest),
							  NULL,
							  0,
							  NULL);
	
	//Nicholas Xu
#if 1
	if((pIpodDevice->pUsbDevice) && (pIpodDevice->pUsbDevice->pPort))
#endif	
	{
		dwStatus = MUSB_StartControlTransfer(pIpodDevice->pUsbDevice->pPort, pControlIrp);
		if(dwStatus)
		{
			/* Log an Error and return state */
		  	MUSB_PRINTK("IPOD Error: Set Configuration0 Request failed, dwStatus: 0x%x\n", dwStatus);
		}
	}
	
	return (dwStatus);
}/* End MGC_MsdConfigureDevice () */

/** After getting connect callback,  device driver calls this function to configure device */
static uint32_t MGC_IpodConfigure1Device(MGC_IpodDevice *pIpodDevice)
{
    MUSB_DeviceRequest      *pSetup;
    MUSB_ControlIrp         *pControlIrp;
    uint32_t			dwStatus;

    pSetup		= &(pIpodDevice->SetupPacket);
    pControlIrp	= &(pIpodDevice->ControlIrp);

    MUSB_PRINTK("MGC_IpodConfigure1Device \n");

    /** Prepare the Setup Packet for sending Set Config Request */
	MGC_IPOD_PREPARE_SETUP_PACKET(pSetup,
								 MUSB_DIR_OUT | MUSB_TYPE_STANDARD | MUSB_RECIP_INTERFACE,
								 MUSB_REQ_SET_CONFIGURATION,
								 2,
								 0,
								 0);
	/** Fill Control Irp */
	MGC_IPOD_FILL_CONTROL_IRP(pIpodDevice,
							  pControlIrp,
							  (uint8_t *)pSetup,
							  sizeof(MUSB_DeviceRequest),
							  NULL,
							  0,
							  NULL);
	
	//Nicholas Xu
#if 1
	if((pIpodDevice->pUsbDevice) && (pIpodDevice->pUsbDevice->pPort))
#endif	
	{
		dwStatus = MUSB_StartControlTransfer(pIpodDevice->pUsbDevice->pPort, pControlIrp);
		if(dwStatus)
		{
			/* Log an Error and return state */
		  	MUSB_PRINTK("IPOD Error: Set Configuration1 Request failed, dwStatus: 0x%x\n", dwStatus);
		}
	}
	return (dwStatus);
}/* End MGC_MsdConfigureDevice () */

static uint32_t MGC_OpenIpodDevice(MGC_IpodDevice *pIpodDevice)
{
	MUSB_DeviceRequest		*pSetup;
	MUSB_ControlIrp 			*pControlIrp;
	uint32_t					SetupLen = sizeof(MUSB_DeviceRequest);
	uint32_t					dwStatus = 0xFFFFFFFF;

	pSetup = &(pIpodDevice->SetupPacket);
	pControlIrp = &(pIpodDevice->ControlIrp);
	
	/** Prepare the Setup Packet for sending Set Config Request */
	MGC_IPOD_PREPARE_SETUP_PACKET(pSetup,
								 MUSB_DIR_OUT | MUSB_TYPE_STANDARD | MUSB_RECIP_DEVICE,
								 MUSB_REQ_SET_CONFIGURATION,
								 2,
								 0,
								 0);
	/** Fill Control Irp */
	MGC_IPOD_FILL_CONTROL_IRP(pIpodDevice,
							  pControlIrp,
							  (uint8_t *)pSetup,
							  SetupLen,
							  NULL,
							  0,
							  NULL);
	dwStatus = MUSB_StartControlTransfer (pIpodDevice->pUsbDevice->pPort, pControlIrp);
	if(dwStatus != 0)
		printk("MGC_OpenIpod is failed pos0\n");
	
	MGC_IPOD_PREPARE_SETUP_PACKET(pSetup,
								 MUSB_DIR_OUT | MUSB_TYPE_STANDARD | MUSB_RECIP_INTERFACE,
								 MUSB_REQ_SET_CONFIGURATION,
								 2,
								 0,
								 0);
	MGC_IPOD_FILL_CONTROL_IRP(pIpodDevice,
							  pControlIrp,
							  (uint8_t *)pSetup,
							  SetupLen,
							  NULL,
							  0,
							  NULL);
	dwStatus = MUSB_StartControlTransfer (pIpodDevice->pUsbDevice->pPort, pControlIrp);
	if(dwStatus != 0)
		printk("MGC_OpenIpod is failed pos1\n");
	
	MGC_IPOD_PREPARE_SETUP_PACKET(pSetup,
								 0x81,
								 0x06,
								 0x2200,
								 0x2,
								 0x60);
	MGC_IPOD_FILL_CONTROL_IRP(pIpodDevice,
							  pControlIrp,
							  (uint8_t *)pSetup,
							  SetupLen,
							  pIpodDevice->ctrl_buffer,
							  0x60,
							  NULL);
	dwStatus = MUSB_StartControlTransfer (pIpodDevice->pUsbDevice->pPort, pControlIrp);
	if(dwStatus != 0)
		printk("MGC_OpenIpod is failed pos2\n");

	return TRUE;
}

/** Callback function*/
static void MGC_IpodInterruptCallback(void* pContext, MUSB_InterruptIrp *pIntIrp)
{
	MGC_IpodDevice		*pIpodDevice = (MGC_IpodDevice  *)pContext;

	if(!pIntIrp->dwStatus)
	{
		if(pIntIrp->dwActualLength)
		{
			pIpodDevice->userData.userdata = pIntIrp->pBuffer;
			pIpodDevice->userData.userdatalen = pIntIrp->dwActualLength;

			if(pIpodDevice->user_event)
				pIpodDevice->user_event(&(pIpodDevice->userData));
		}
		else
			MUSB_PRINTK("dwActualLength is Error\n");
	}
}

static uint32_t MGC_IpodFillAndSubmitIntrIrp(MGC_IpodDevice *pIpodDevice)
{
	
	const MUSB_EndpointDescriptor		*pEndpDscr;
	MUSB_DeviceEndpoint				devEndp;
	MUSB_Irp						*pIntrInIrp; 
	uint16_t							 wNakLimit = 0xFFFF; /* Disable the NAK Limit */
	uint32_t							 dwStatus = 0xFFFFFFFF;
	
	pEndpDscr = &(pIpodDevice->pIntEndpoint);
	pIntrInIrp = &(pIpodDevice->InterruptIrp); 

	MUSB_MemCopy((void *)(&(devEndp.UsbDescriptor)), (void *)pEndpDscr, 0x07);
	devEndp.wNakLimit   = wNakLimit;
	devEndp.pDevice = pIpodDevice->pUsbDevice;

	//Nicholas Xu
#if 1
	if(!pIpodDevice->hBus)
		return dwStatus;
#endif
	
	pIpodDevice->InterruptPipe = MUSB_OpenPipe(pIpodDevice->hBus, &devEndp, NULL);
	if (pIpodDevice->InterruptPipe == NULL)
	{
		MUSB_PRINTK("IPOD Error: OpenPipe status = 0x%x \n", dwStatus);
		return dwStatus;
	}
	
	/* Fill the Interrupt IRP */
	pIntrInIrp->hPipe = pIpodDevice->InterruptPipe;
	pIntrInIrp->bAllowShortTransfer = FALSE;
	pIntrInIrp->pBuffer = pIpodDevice->int_buffer;
	pIntrInIrp->dwLength = pEndpDscr->wMaxPacketSize;
	pIntrInIrp->pfIrpComplete = MGC_IpodInterruptCallback;
	pIntrInIrp->pCompleteParam = (void *)pIpodDevice;

	MUSB_PRINTK("IPOD: Submitting interrupt IRP\n");
	dwStatus = MUSB_StartTransfer(pIntrInIrp);
	if (dwStatus)
	{
		MUSB_PRINTK("IPOD Error: MUSB_StartTransfer() status = 0x%x \n", dwStatus);
		if(pIpodDevice->InterruptPipe)
			MUSB_ClosePipe(pIpodDevice->InterruptPipe);
	}

	dwStatus = 0;
	
	return dwStatus;
}



