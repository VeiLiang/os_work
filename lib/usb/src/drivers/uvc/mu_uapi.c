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
 * UVC driver top-layer module (UCD callbacks and API)
 * $Revision: 1.2 $
 */

#include "mu_cdi.h"
#include "mu_descs.h"
#include "mu_diag.h"
#include "mu_mem.h"

#include "mu_uapi.h"
#include "mu_uctrl.h"

/******************************* FORWARDS ********************************/
static uint8_t MUSB_UvcConnect(void *pPrivateData, 
                  MUSB_BusHandle     hBus,
                  MUSB_Device       *pUsbDevice,
                  const uint8_t     *pPeripheralList);
                  
static void MUSB_UvcDisconnect (void *pPrivateData, 
					   MUSB_BusHandle  hBus,
					   MUSB_Device	  *pUsbDevice);

static uint32_t MGC_UvcConfigureDevice(MGC_UvcDevice *pUvcDevice);
static void MGC_UvcSetConfigCallback(void* pContext, MUSB_ControlIrp *pControlIrp);
static uint32_t MGC_UvcOpenVideoPipe(MGC_UvcDevice *pUvcDevice);
static uint32_t MGC_UvcOpenAudioPipe(MGC_UvcDevice *pUvcDevice);
static uint32_t MGC_UvcCloseDevice(MGC_UvcDevice *pUvcDevice);
/**************************** GLOBALS *****************************/
//static OS_SEM lg_UvcDeviceSem = {0};
//OS_SEM *UvcDeviceSem = NULL;
OS_EVENT *UvcDeviceSem = NULL;

static MGC_UvcDevice *MGC_UvcDeviceContext = NULL;

static uint32_t MGC_aUvcTimerResolution[1] = { 10 };

static const uint8_t MGC_aUvcPeripheralList[] =
{
    MUSB_TARGET_CLASS, 
    MUSB_CLASS_UVC, 
    MUSB_TARGET_ACCEPT
};

MUSB_DeviceDriver MGC_UvcClassDriver =
{
    "UVC_CLIENT",
    sizeof(MGC_aUvcTimerResolution) / sizeof(uint32_t),
    MGC_aUvcTimerResolution,
    MUSB_UvcConnect,
    MUSB_UvcDisconnect,
    NULL,
    NULL,
    MUSB_User_App_Connect_Ind,
    MUSB_User_App_Discon_Ind
};

/*************************** FUNCTIONS ****************************/

uint16_t MGC_FillUvcClassPeripheralList(uint8_t* pList, uint16_t wListLength)
{
	uint16_t wResult = MUSB_MIN((uint16_t)sizeof(MGC_aUvcPeripheralList), wListLength);

	MUSB_MemCopy(pList, MGC_aUvcPeripheralList, wResult);
	return wResult;
}

MUSB_DeviceDriver* MGC_GetUvcClassDriver(void)
{
	return &MGC_UvcClassDriver;
}

MGC_UvcDevice* MGC_GetUvcDeviceContext(void)
{
	return MGC_UvcDeviceContext;
}

/**
 * Allocate per-device data
 */
static MGC_UvcDevice* MGC_UvcDeviceInit(MUSB_Device* pUsbDevice)
{
	MGC_UvcDevice 		*pUvcDevice;
	OS_ERR err;

	UvcDeviceSem = OSSemCreate(0);
	if(!UvcDeviceSem)
	//OSSemCreate(&lg_UvcDeviceSem, NULL, 0, &err);
	//if(OS_ERR_NONE != err)
	{
		MUSB_PRINTK("UVC Error: UvcDeviceSem is NULL\n");
		return (NULL);
	}
	//else
		//UvcDeviceSem = &lg_UvcDeviceSem;
	
	pUvcDevice = (MGC_UvcDevice *)MUSB_MemAlloc(sizeof(MGC_UvcDevice));
	if(NULL == pUvcDevice) 
	{	
		MUSB_PRINTK("UVC Error: Insufficient memory\n");
		return (NULL);
	}
	MUSB_MemSet((void *)pUvcDevice, 0, sizeof(MGC_UvcDevice));
	
	pUvcDevice->pDriver 				= &MGC_UvcClassDriver;
	pUvcDevice->pUsbDevice			= pUsbDevice;
	
	MUSB_PRINTK("UVC: Initialization Completed\n");
	return pUvcDevice;
}

/** This function is called when UVC device is connected.*/
static uint8_t MUSB_UvcConnect(  void              *pPrivateData, 
                  MUSB_BusHandle     hBus,
                  MUSB_Device       *pUsbDevice,
                  const uint8_t     *pPeripheralList)
{
	uint8_t bConfig, bIndex, bAlternateSetting, bEndIndex;
	//uint8_t bSubclass, bProtocol;
	const MUSB_InterfaceDescriptor* pIface;
	const MUSB_EndpointDescriptor* pEnd;
	const MUSB_ConfigurationDescriptor* pConfig;	
	MUSB_DeviceDriver	*pUvcDriver; 	
	MGC_UvcDevice		*pUvcDevice;
	uint8_t bStatus = FALSE;

	//bSubclass = 0;
	//bProtocol = 0;
		
	/* Device is connected */
	MUSB_PRINTK("UVC Device Connected \n");
	pUvcDevice = MGC_UvcDeviceInit(pUsbDevice);
	if(!pUvcDevice)
		return bStatus;
	else
		MGC_UvcDeviceContext = pUvcDevice;

	pUsbDevice->pDriverPrivateData = pUvcDevice;
	/* Initialize */
	pUvcDevice->pUsbDevice	= pUsbDevice;
	pUvcDevice->hBus			= hBus;

	//这里如果不是config0  那么要仿照IPOD  的驱动找到相应的config	
	/* assume first config */
	pConfig = pUsbDevice->apConfigDescriptors[0];
	
#if 1
	MUSB_PRINTK("------------------------------------------\n");
	MUSB_PRINTK("<<<<<<<<<   Config Descriptors   >>>>>>>>>\n");
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
		/* assume no alternates */
		bAlternateSetting = 0;
		do
		{
			pIface = MUSB_FindInterfaceDescriptor(pConfig, bIndex, bAlternateSetting);
			if(pIface)
			{
				//this case is find the video interfaces
				if(pIface->bInterfaceClass == UVC_VIDEO_INTERFACE_CLASS)
				{
#if 1
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
					for(bEndIndex = 0; bEndIndex < pIface->bNumEndpoints; bEndIndex++)
					{
						pEnd = MUSB_FindEndpointDescriptor(pConfig, pIface, 0);
#if 1					
					MUSB_PRINTK("=====pEnd index is 0x%x=====\n", bEndIndex);
					MUSB_PRINTK("bLength is 0x%x\n", pEnd->bLength);
					MUSB_PRINTK("bDescriptorType is 0x%x\n", pEnd->bDescriptorType);
					MUSB_PRINTK("bEndpointAddress is 0x%x\n", pEnd->bEndpointAddress);
					MUSB_PRINTK("bmAttributes is 0x%x\n", pEnd->bmAttributes);
					MUSB_PRINTK("wMaxPacketSize is 0x%x\n", pEnd->wMaxPacketSize);
					MUSB_PRINTK("bInterval is 0x%x\n", pEnd->bInterval);
					MUSB_PRINTK("=======================================\n");
#endif		
						//find the pEnd which one of the wMaxPacketSize is max	MAX_ENDPOINT_LEN
						if((pEnd->wMaxPacketSize <= MAX_ENDPOINT_LEN) &&
							(pEnd->wMaxPacketSize > pUvcDevice->pIsoVideoEndpoint.wMaxPacketSize))
						{
							((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoVideoInterface))->bLength = pIface->bLength;
							((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoVideoInterface))->bDescriptorType = pIface->bDescriptorType;
							((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoVideoInterface))->bInterfaceNumber = pIface->bInterfaceNumber;
							((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoVideoInterface))->bAlternateSetting = pIface->bAlternateSetting;
							((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoVideoInterface))->bNumEndpoints = pIface->bNumEndpoints;
							((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoVideoInterface))->bInterfaceClass = pIface->bInterfaceClass;
							((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoVideoInterface))->bInterfaceSubClass = pIface->bInterfaceSubClass;
							((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoVideoInterface))->bInterfaceProtocol = pIface->bInterfaceProtocol;
							((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoVideoInterface))->iInterface = pIface->iInterface;
							MUSB_PRINTK("Video pEnd->wMaxPacketSize is 0x%x\n", pEnd->wMaxPacketSize);
							((MUSB_EndpointDescriptor *)(&pUvcDevice->pIsoVideoEndpoint))->bLength = pEnd->bLength;
							((MUSB_EndpointDescriptor *)(&pUvcDevice->pIsoVideoEndpoint))->bDescriptorType = pEnd->bDescriptorType;
							((MUSB_EndpointDescriptor *)(&pUvcDevice->pIsoVideoEndpoint))->bEndpointAddress = pEnd->bEndpointAddress;
							((MUSB_EndpointDescriptor *)(&pUvcDevice->pIsoVideoEndpoint))->bmAttributes = pEnd->bmAttributes;
							((MUSB_EndpointDescriptor *)(&pUvcDevice->pIsoVideoEndpoint))->wMaxPacketSize = pEnd->wMaxPacketSize;
							((MUSB_EndpointDescriptor *)(&pUvcDevice->pIsoVideoEndpoint))->bInterval = pEnd->bInterval;
						}
					}	
				}

				//this case is find the audio interfaces
				if(pIface->bInterfaceClass == UVC_AUDIO_INTERFACE_CLASS)
				{				
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
					((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoAudioInterface))->bLength = pIface->bLength;
					((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoAudioInterface))->bDescriptorType = pIface->bDescriptorType;
					((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoAudioInterface))->bInterfaceNumber = pIface->bInterfaceNumber;
					((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoAudioInterface))->bAlternateSetting = pIface->bAlternateSetting;
					((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoAudioInterface))->bNumEndpoints = pIface->bNumEndpoints;
					((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoAudioInterface))->bInterfaceClass = pIface->bInterfaceClass;
					((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoAudioInterface))->bInterfaceSubClass = pIface->bInterfaceSubClass;
					((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoAudioInterface))->bInterfaceProtocol = pIface->bInterfaceProtocol;
					((MUSB_InterfaceDescriptor *)(&pUvcDevice->pIsoAudioInterface))->iInterface = pIface->iInterface;
					
					for(bEndIndex = 0; bEndIndex < pIface->bNumEndpoints; bEndIndex++)
					{
						pEnd = MUSB_FindEndpointDescriptor(pConfig, pIface, 0);
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
						//find the pEnd which one of the wMaxPacketSize is max	MAX_ENDPOINT_LEN
						if((pEnd->wMaxPacketSize <= MAX_ENDPOINT_LEN) &&
							(pEnd->wMaxPacketSize > pUvcDevice->pIsoAudioEndpoint.wMaxPacketSize))
						{
							MUSB_PRINTK("Audio pEnd->wMaxPacketSize is 0x%x\n", pEnd->wMaxPacketSize);
							((MUSB_EndpointDescriptor *)(&pUvcDevice->pIsoAudioEndpoint))->bLength = pEnd->bLength;
							((MUSB_EndpointDescriptor *)(&pUvcDevice->pIsoAudioEndpoint))->bDescriptorType = pEnd->bDescriptorType;
							((MUSB_EndpointDescriptor *)(&pUvcDevice->pIsoAudioEndpoint))->bEndpointAddress = pEnd->bEndpointAddress;
							((MUSB_EndpointDescriptor *)(&pUvcDevice->pIsoAudioEndpoint))->bmAttributes = pEnd->bmAttributes;
							((MUSB_EndpointDescriptor *)(&pUvcDevice->pIsoAudioEndpoint))->wMaxPacketSize = pEnd->wMaxPacketSize;
							((MUSB_EndpointDescriptor *)(&pUvcDevice->pIsoAudioEndpoint))->bInterval = pEnd->bInterval;
						}
					}	
				}
				
				bAlternateSetting++;
			}
			else
			{
				MUSB_PRINTK("=====pIface is NULL=====\n");
				break;
			}
		}while(bAlternateSetting);	
	}

    if((pUvcDevice->pIsoVideoEndpoint.bmAttributes&0x3) == ISO_TRANSFER_TYPE)	
	{
		/* Configure the device */
		bStatus = (uint8_t)MGC_UvcConfigureDevice(pUvcDevice);
		if(MUSB_STATUS_OK == bStatus)
		{
			MUSB_PRINTK("Configure the device MUSB_STATUS_OK\n");
			bStatus = MGC_UvcOpenVideoPipe(pUvcDevice);
			
			if(MUSB_STATUS_OK != bStatus)
			{
				MUSB_PRINTK("MGC_UvcOpenPipe ERROR\n");
				MUSB_RejectDevice(pUvcDevice->hBus, pUvcDevice->pUsbDevice);
				return (bStatus); 
			}			
		}
		else
			return (bStatus); 
	}

	if((pUvcDevice->pIsoAudioEndpoint.bmAttributes&0x3) == ISO_TRANSFER_TYPE)	
	{
		bStatus = MGC_UvcOpenAudioPipe(pUvcDevice);
		if(MUSB_STATUS_OK != bStatus)
		{
			MUSB_PRINTK("MGC_UvcOpenPipe ERROR\n");
			MUSB_RejectDevice(pUvcDevice->hBus, pUvcDevice->pUsbDevice);
			return (bStatus); 
		}			
	}

	pUvcDriver = MGC_GetUvcClassDriver();
	//MUSB_User_App_Connect_Ind(pUvcDriver->pPrivateData);  
	
	MUSB_PRINTK("MUSB_UvcConnect is 0x%x\n", bStatus);
	return (bStatus); 
}/* End MUSB_UvcConnect() */

/** Disconnect Handler for UVC Device Driver */
static void MUSB_UvcDisconnect (void           *pPrivateData, 
                         MUSB_BusHandle  hBus,
                         MUSB_Device    *pUsbDevice)
{
	MUSB_DeviceDriver 	*pUvcDriver = MGC_GetUvcClassDriver();
	MGC_UvcDevice		*pUvcDevice;
	//OS_ERR				myerr;
    INT8U   myerr;
	
	pUvcDevice = (MGC_UvcDevice *)pUsbDevice->pDriverPrivateData;
	
	MUSB_User_App_Discon_Ind(pUvcDriver->pPrivateData);  

	/* Check against the USB device and bus handle */
	if( (hBus != pUvcDevice->hBus) || (pUsbDevice != pUvcDevice->pUsbDevice) )
	{
		MUSB_PRINTK("UVC Error: UVC Device Disconnect Callback\n");
		return;
	}
		
	MGC_UvcCloseDevice(pUvcDevice);
	
	if(UvcDeviceSem)
	{
		OSSemDel(UvcDeviceSem, OS_OPT_DEL_NO_PEND, &myerr);
		UvcDeviceSem = NULL;
	}
	
	if(pUvcDevice->IsoVideoIrp.adwLength)
	{
		MUSB_MemFree(pUvcDevice->IsoVideoIrp.adwLength);
		pUvcDevice->IsoVideoIrp.adwLength = NULL;
	}
	if(pUvcDevice->IsoVideoIrp.adwStatus)
	{
		MUSB_MemFree(pUvcDevice->IsoVideoIrp.adwStatus);
		pUvcDevice->IsoVideoIrp.adwStatus = NULL;
	}
	if(pUvcDevice->IsoVideoIrp.adwActualLength)
	{
		MUSB_MemFree(pUvcDevice->IsoVideoIrp.adwActualLength);
		pUvcDevice->IsoVideoIrp.adwActualLength = NULL;
	}

	if(pUvcDevice->IsoAudioIrp.adwLength)
	{
		MUSB_MemFree(pUvcDevice->IsoAudioIrp.adwLength);
		pUvcDevice->IsoAudioIrp.adwLength = NULL;
	}
	if(pUvcDevice->IsoAudioIrp.adwStatus)
	{
		MUSB_MemFree(pUvcDevice->IsoAudioIrp.adwStatus);
		pUvcDevice->IsoAudioIrp.adwStatus = NULL;
	}
	if(pUvcDevice->IsoAudioIrp.adwActualLength)
	{
		MUSB_MemFree(pUvcDevice->IsoAudioIrp.adwActualLength);
		pUvcDevice->IsoAudioIrp.adwActualLength = NULL;
	}
	
	MGC_UvcDeviceContext = NULL;
	MUSB_MemFree(pUvcDevice);
	pUvcDevice = NULL;
	pUsbDevice->pDriverPrivateData = NULL;

	//MUSB_User_App_Discon_Ind(pUvcDriver->pPrivateData);  
	
	MUSB_PRINTK("UVC Device Disconnected Successfully\n");	
	
	return;
}/* End MUSB_UvcDisconnect () */


/** After getting connect callback,  device driver calls this function to configure device */
static uint32_t MGC_UvcConfigureDevice(MGC_UvcDevice *pUvcDevice)
{
	MUSB_DeviceRequest      *pSetup;
	MUSB_ControlIrp         *pControlIrp;
	uint32_t                 dwStatus;

	pSetup          = &(pUvcDevice->SetupPacket);
	pControlIrp     = &(pUvcDevice->ControlIrp);

	MUSB_PRINTK("MGC_UvcConfigureDevice \n");

	/** Prepare the Setup Packet for sending Set Config Request */
	MGC_UVC_PREPARE_SETUP_PACKET(pSetup,
	                             (MUSB_DIR_OUT| MUSB_TYPE_STANDARD | MUSB_RECIP_DEVICE),
	                             MUSB_REQ_SET_CONFIGURATION,
	                             1,
	                             0,
	                             0);

	/** Fill Control Irp */
	MGC_UVC_FILL_CONTROL_IRP(pUvcDevice,
	                          pControlIrp,
	                          (uint8_t *)pSetup,
	                          sizeof(MUSB_DeviceRequest),
	                          NULL,
	                          0,
	                          MGC_UvcSetConfigCallback);
	
	//Nicholas Xu
#if 1
	if((pUvcDevice->pUsbDevice) && (pUvcDevice->pUsbDevice->pPort))
#endif		
	{
		dwStatus = MUSB_StartControlTransfer(pUvcDevice->pUsbDevice->pPort, pControlIrp);
		if (dwStatus)
		{
		    /* Log an Error and return state */
		  MUSB_PRINTK("MSD Error: Set Configuration Request failed, dwStatus: 0x%x\n", dwStatus);
		}
	}

	return (dwStatus);
}/* End MGC_MsdConfigureDevice () */

/** Callback function when device acknowledges set config reqeust. */
static void MGC_UvcSetConfigCallback(void* pContext, MUSB_ControlIrp *pControlIrp)
{
	//MUSB_DeviceDriver		*pUvcDriver;
	MGC_UvcDevice			*pUvcDevice;
	MUSB_DeviceDriver 	*pUvcDriver = MGC_GetUvcClassDriver();

	pUvcDevice = (MGC_UvcDevice *) pContext;

	MUSB_PRINTK("====MGC_UvcSetConfigCallback====\n");

	if (MUSB_STATUS_OK != pControlIrp->dwStatus)
	{
		MUSB_DIAG1(MUSB_MSD_DIAG_ERROR, 
		"UVC Error: Set Config Callback Status", pControlIrp->dwStatus, 10, FALSE);
		MUSB_RejectDevice(pUvcDevice->hBus, pUvcDevice->pUsbDevice);
		return;
	}
	/* Set the Current Configuration Descriptor to Default as Set Config is Success */
	pUvcDevice->pUsbDevice->pCurrentConfiguration = pUvcDevice->pUsbDevice->apConfigDescriptors[0];

	/* Device is connected */
	MUSB_DIAG_STRING(MUSB_MSD_DIAG_SUCCESS, "UVC Device Configured Successfully");
	MUSB_PRINTK("UVC Device Configured Successfully \n"); 
	
	MUSB_User_App_Connect_Ind(pUvcDriver->pPrivateData);  
}

static uint32_t MGC_UvcOpenVideoPipe(MGC_UvcDevice *pUvcDevice)
{
	MUSB_EndpointDescriptor	*pEndpDscr;
	MUSB_DeviceEndpoint		devEndp;	
	uint16_t					wNakLimit = 0xFFFF; /* Disable the NAK Limit */
	uint32_t 					dwStatus = 0xFFFFFFFF;

	pEndpDscr = (MUSB_EndpointDescriptor *)(&(pUvcDevice->pIsoVideoEndpoint));

	MUSB_PRINTK("====MGC_UvcOpenVideoPipe====\n");
	MUSB_MemCopy((void *)(&(devEndp.UsbDescriptor)), (void *)pEndpDscr, 0x07);
	devEndp.wNakLimit   = wNakLimit;
	devEndp.pDevice = pUvcDevice->pUsbDevice;
	
	//Nicholas Xu
#if 1	
	if(!pUvcDevice->hBus)
		return dwStatus;
#endif	
	pUvcDevice->isoVideoPipe = MUSB_OpenPipe(pUvcDevice->hBus, &devEndp, NULL);
	if(pUvcDevice->isoVideoPipe == NULL)
	{
		MUSB_PRINTK("UVC Error: OpenVideoPipe status=0x%x\n", dwStatus);	
		return dwStatus;
	}
	else
		return 0;
}/* End MGC_UvcConfigureDevice () */

static uint32_t MGC_UvcOpenAudioPipe(MGC_UvcDevice *pUvcDevice)
{
	MUSB_EndpointDescriptor	*pEndpDscr;
	MUSB_DeviceEndpoint		devEndp;	
	uint16_t					wNakLimit = 0xFFFF; /* Disable the NAK Limit */
	uint32_t 					dwStatus = 0xFFFFFFFF;

	pEndpDscr = (MUSB_EndpointDescriptor *)(&(pUvcDevice->pIsoAudioEndpoint));

	MUSB_PRINTK("====MGC_UvcOpenAudioPipe====\n");
	MUSB_MemCopy((void *)(&(devEndp.UsbDescriptor)), (void *)pEndpDscr, 0x07);
	devEndp.wNakLimit   = wNakLimit;
	devEndp.pDevice = pUvcDevice->pUsbDevice;
	
	//Nicholas Xu
#if 1	
	if(!pUvcDevice->hBus)
		return dwStatus;
#endif	
	pUvcDevice->isoAudioPipe = MUSB_OpenPipe(pUvcDevice->hBus, &devEndp, NULL);
	if(pUvcDevice->isoAudioPipe == NULL)
	{
		MUSB_PRINTK("UVC Error: OpenAudioPipe status=0x%x\n", dwStatus);	
		return dwStatus;
	}
	else
		return 0;
}/* End MGC_UvcConfigureDevice () */

static uint32_t MGC_UvcCloseDevice(MGC_UvcDevice *pUvcDevice)
{
	//Nicholas Xu
	if(pUvcDevice->isoVideoPipe)	
		MUSB_ClosePipe(pUvcDevice->isoVideoPipe);
	if(pUvcDevice->isoAudioPipe)	
		MUSB_ClosePipe(pUvcDevice->isoAudioPipe);

	return 0;
}
