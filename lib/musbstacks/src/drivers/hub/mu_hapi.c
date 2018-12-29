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
 * Hub driver top-layer module (UCD callbacks and API)
 * $Revision: 1.2 $
 */

#include "mu_cdi.h"
#include "mu_descs.h"
#include "mu_diag.h"
#include "mu_mem.h"

#include "mu_hdef.h"
#include "mu_hub.h"
#include "mu_hcreq.h"
#include "mu_hcore.h"
#include "mu_hapi.h"

/******************************* FORWARDS ********************************/

static uint8_t MGC_HubConnect(void* pDriverPrivateData, 
			      MUSB_BusHandle busHandle, 
			      MUSB_Device* pUsbDevice, 
			      const uint8_t *pPeriList);
static void MGC_HubDisconnect(void* pDriverPrivateData, 
			      MUSB_BusHandle busHandle, 
			      MUSB_Device* pUsbDevice);

/**************************** GLOBALS *****************************/

/* in case some compilers don't support merging like strings */
static const char* MGC_aHubDevNotFound = "HubError: This Is Not A Hub Device/NULL";
static const char* MGC_aHubInvalidTtType = "HubError: Invalid TT type";
static const char* MGC_aNoInterveningHub = "HubError: Device not connected through a hub";
static const char* MGC_aDisconnectHub = "HubError: Please disconnect hub";

static uint8_t bInitialized = FALSE;

/* NOTE: this MUST be maintained with MUSB_HUB_MAX_HUB_SUPPORTED elements */
uint32_t aTimerResolution[MUSB_HUB_MAX_HUB_SUPPORTED] = {
    MUSB_HUB_MIN_RESET_TIME,
    MUSB_HUB_MIN_RESET_TIME,
    MUSB_HUB_MIN_RESET_TIME,
    MUSB_HUB_MIN_RESET_TIME,
    MUSB_HUB_MIN_RESET_TIME,
    MUSB_HUB_MIN_RESET_TIME
};

MUSB_HubDriverContext  hubDriverContext;

static const uint8_t MGC_aHubPeripheralList[] =
{
    MUSB_TARGET_CLASS, MUSB_CLASS_HUB, 
    MUSB_TARGET_ACCEPT
};

MUSB_DeviceDriver MGC_HubClassDriver =
{
    &hubDriverContext,
    MUSB_HUB_MAX_HUB_SUPPORTED,
    aTimerResolution,
    MGC_HubConnect,
    MGC_HubDisconnect,
    NULL,
    NULL
};

/*************************** FUNCTIONS ****************************/

uint16_t MGC_FillHubClassPeripheralList(uint8_t* pList, uint16_t wListLength)
{
	uint16_t wResult = MUSB_MIN((uint16_t)sizeof(MGC_aHubPeripheralList), wListLength);

	MUSB_MemCopy(pList, MGC_aHubPeripheralList, wResult);
	return wResult;
}

MUSB_DeviceDriver* MGC_GetHubClassDriver()
{
	return &MGC_HubClassDriver;
}

static void MGC_HubDriverInit(void)
{
	uint8_t bIndex;

	hubDriverContext.bNumOfHubActive = 0x00;
	hubDriverContext.pDriver = &MGC_HubClassDriver;
	/* Make all the state free and set bNumOfHubActive to 0x00 */
	for (bIndex = 0x00; bIndex < MUSB_HUB_MAX_HUB_SUPPORTED; bIndex++)
	{
		hubDriverContext.hubDeviceList[bIndex].bState = MUSB_HUB_STATE_FREE;
	}
	MGC_HubClassDriver.pPrivateData = (void *) &hubDriverContext;

	bInitialized = TRUE;
	return;
}

/**
 * This function is called by UCD when a hub is connected to the
 * system just after the enumeration of device (Hub is considered a device).
 * This function allocates memory for newly connected hub and initializes it. 
 *
 * @param pData pPrivateData from the client's registration struct 
 * @param busHandle session "handle"
 * @param pUsbDevice Pointer to USB device which is given by UCD. 
 *
 * @return TRUE in case of success or FALSE in case of failure.
 */
static uint8_t MGC_HubConnect(void* pDriverPrivateData, 
			      MUSB_BusHandle busHandle, 
			      MUSB_Device* pUsbDevice, 
			      const uint8_t *pPeriList)
{
	MUSB_Hub                        *pHubDevice;
	MUSB_ConfigurationDescriptor    *pCfgDscr;
	const MUSB_InterfaceDescriptor  *pInfDscr;
	const MUSB_EndpointDescriptor   *pEndpDscr;
	MUSB_HubDriverContext           *pDriverContext;
	MUSB_Hub                        *pHubParent;
	MUSB_Device* pWorkDevice;
	uint32_t                         dwStatus;
	uint8_t                          bHubIndex;
	uint8_t                          bIndex;
	uint8_t                          bEndIndex;
	uint8_t                          bDepthInHubChain = 0x01;
	/* direction In and endpoint type Interrupt  */
	uint8_t                          bEndpDir = 0x80;   
	uint8_t                          bEndpType = 0x03;

	if(!bInitialized)
	{
		MGC_HubDriverInit();
	}

	/* 1. Findout depth of this hub in HubChain.If depth is proper then
	*    go ahead otherwise return False.
	* 1. Get driver context which has list of all hub device.
	* 2. Get free index at which information about newly connected
	*    hub can be store. 
	* 3. Update pDriverContext information.
	* 4. Update descriptor related information.
	* 5. Update/Initialize require field in pHubDevice.
	* 6. Call connect Handler.
	*/

	MUSB_PRINTK("MUSB_HubConnect() >> \n");

	/* Find-out depth of this hub in present hub chain. If this hub's 
	* depth is < MUSB_MAX_HUB_CHAIN or hub's depth == MUSB_MAX_HUB_CHAIN 
	* and hub is not part of compound device then there is no problem and 
	* this hub  should submit Intr Irp for getting hub and port status 
	* Change. But, If this hub's depth == MUSB_MAX_HUB_CHAIN and it is 
	* part of compound device then this hub should be rejected by the 
	* driver.  This hub belongs to compound device or not can be known 
	* after getting hub descriptor.
	*/
	pWorkDevice = pUsbDevice;
	while (pWorkDevice->pParentUsbDevice)
	{
		pWorkDevice = pWorkDevice->pParentUsbDevice;
		bDepthInHubChain++;
	}

	MUSB_PRINTK("Depth of connected hub: 0x%x\n", bDepthInHubChain);

	if (bDepthInHubChain > MUSB_MAX_HUB_CHAIN)
	{
		MUSB_PRINTK("HubError: Hub tree is too deep\n");	
		return (FALSE);
	}
	/* Control Comes here it means this hub's depth is <= MUSB_MAX_HUB_CHAIN
	* But IF this hub's depth is == MUSB_MAX_HUB_CHAIN and it is part of compound device
	* then this hub should be rejected as soon as found that it is part of compound devie. 
	* i.e. after getting hub descriptor.
	*/

	/* From driver context get free hub device structure and then update number of
	* activeHub in the system.
	*/
	pDriverContext = (MUSB_HubDriverContext*)pDriverPrivateData;

	dwStatus = MGC_HubFindFreeHubDeviceIndex(pDriverContext, &bHubIndex);
	if ( MUSB_FAILURE_UNSIGNED == dwStatus )
	{
		MUSB_PRINTK("HubError: Free pHubDevice not available\n");		
		return (FALSE);
	}
	pDriverContext->bNumOfHubActive++;

	pHubDevice = &(pDriverContext->hubDeviceList[bHubIndex]);
	pHubDevice->bIndexAtHubDeviceList = bHubIndex;

	if(pUsbDevice->pParentUsbDevice)
	{
		dwStatus    = MGC_HubFindHubDevice(pUsbDevice->pParentUsbDevice, &pHubParent);
		if(MUSB_SUCCESS != dwStatus)
		{
			MUSB_DIAG_STRING(1, MGC_aHubDevNotFound);
			return (FALSE);
		}
		if( (MUSB_HUB_BUS_POWERED == pHubParent->bSelfPower) &&
			(!(pUsbDevice->apConfigDescriptors[0]->bmAttributes&MUSB_HUB_CONFIG_SELF_POWER_BM)) )
		{
			/*
			* Bus Powered Cannot Drive Another BusPowered Hub
			*/
			MUSB_PRINTK("HubError: Bus-powered hub cannot drive another bus-powered hub\n");		
			MUSB_PRINTK(MGC_aDisconnectHub);	
			MUSB_PRINTK("\n");		
			return (FALSE);        
		}
	}

	/* Update descriptors. Since Hub always have one configuration
	* descriptor hence there is no need to store it. Interface as well as
	* endpoint descriptor is used to know driver is selected for which 
	* interface and for creation of pipe respectively.
	*/
	pCfgDscr = pUsbDevice->apConfigDescriptors[0];

	/* Todo: Update interface descriptor as well as endpoint descriptor.
	* Suppose to get interface descriptor from UCD for which driver is
	* selected. Put it into pHubDevice. Presently Just assuming that Oth Alternate
	* setting is active.
	*/
	pInfDscr =   MUSB_FindInterfaceDescriptor(pCfgDscr, 0x00, 0x00);
	if ((pInfDscr == NULL) || (pInfDscr->bNumEndpoints == 0x00))
	{
		MUSB_PRINTK("HubError: no endpoints; please disconnect hub\n");		
		return (FALSE);
	}

	for(bEndIndex = 0; bEndIndex < pInfDscr->bNumEndpoints; bEndIndex++)
	{
		pEndpDscr = MUSB_FindEndpointDescriptor(pCfgDscr, pInfDscr, bEndIndex);
		if((bEndpDir == (pEndpDscr->bEndpointAddress & MUSB_ENDPOINT_DIR_MASK)) &&
			(bEndpType == (pEndpDscr->bmAttributes & MUSB_ENDPOINT_XFERTYPE_MASK)))
		{
			break;
		}
		pEndpDscr = NULL;
	}
	if (NULL ==  pEndpDscr)
	{
		MUSB_PRINTK("HubError: Interrupt-In endpoint is not available; please disconnect hub\n");		
		return (FALSE);
	}

	pHubDevice->pInfDscr = pInfDscr;
	pHubDevice->pEndpDscr = pEndpDscr;

	/* Now everything required by Hub device is available i.e. 
	* free index, interface, required endpoint. So Go ahead and update requred field
	* in pDriverContext as well as pHubDevice before calling connect handler.
	*/

	if (0x00 == bHubIndex)
	{
		/* It means connected hub is first hub in the system and it is
		* Root Hub.
		*/
		MUSB_PRINTK("Root hub is connected\n");			
		pUsbDevice->pParentUsbDevice = NULL;
	}
	pUsbDevice->pDriverPrivateData    =  (void *)pDriverPrivateData;

	/* Initialize newly connected hub's State and other required fields. */
	pHubDevice->busHandle       =  busHandle;
	pHubDevice->pUsbDevice      =  pUsbDevice;

	pHubDevice->bDepthInHubChain = bDepthInHubChain;

	pHubDevice->bState          =  MUSB_HUB_STATE_CONNECT;
	pHubDevice->bNextState      =  MUSB_HUB_STATE_CONFIGURED;

	pHubDevice->bPortState     =  MUSB_HUB_PORT_STATE_DEFAULT;
	pHubDevice->bPortNextState =  MUSB_HUB_PORT_STATE_DEFAULT;

	pHubDevice->bActivePortNum = MUSB_HUB_INVALID_PORT_NUMBER;
	pHubDevice->bIntrIrpExecutionErrorCount    = 0x00;

	/* Initialize debounce parameter.    */    
	pHubDevice->debounceParam.dwWaitingTime = MUSB_HUB_MIN_DEBOUNCE_TIME;
	pHubDevice->debounceParam.bErrorCount   = 0x00;

	/* Initialize Reset parameter.    */    
	pHubDevice->resetParam.dwWaitingTime    = MUSB_HUB_MIN_RESET_TIME;
	pHubDevice->resetParam.dwNumOfTry       = 0x00;

	for (bIndex= 0x00; bIndex < MUSB_HUB_MAX_PORTS; bIndex++)
	{
		pHubDevice->pChild[bIndex] = NULL;
	}

	dwStatus = MGC_HubConnectHandler(pHubDevice, pUsbDevice, 
	pUsbDevice->bBusAddress, 
	pUsbDevice->pPort);
	if (MUSB_FAILURE == dwStatus)
	{
		/* Here number of activeHub pHubContext as well as bState in pHubDevice
		* is not updated because when this hub is disconnected at that time
		* those fields are updated.
		*/
		MUSB_PRINTK("HubError: MUSB_HubConnect() <<\n");		
		MUSB_PRINTK(MGC_aDisconnectHub);	
		MUSB_PRINTK("\n");	
		return (FALSE);
	}

	MUSB_PRINTK("HubSuccess: MUSB_HubConnect() <<\n");		
	return (TRUE);
} /* End of function MUSB_HubConnect  */


/**
 * This function called by UCD when a hub is disconnected from the system.
 *
 * @param pData pPrivateData from the client's registration struct 
 * @param busHandle session "handle"
 * @param pUsbDev Pointer to USB device which is given by UCD.
 * @return None.
 */
static void MGC_HubDisconnect(void* pDriverPrivateData, 
			      MUSB_BusHandle busHandle, 
			      MUSB_Device* pUsbDevice)
{
	MUSB_Hub                        *pHubDevice;
	MUSB_HubDriverContext           *pDriverContext;
	uint32_t                         dwStatus;

	/* 1. Get pHubDevice pointer which is stored in pPrivateData of the pUsbDev
	*    pointer.
	* 2. Check disconnected hub is root hub or not. If its root hub then 
	*    update static global "MUSB_bRootHub" with MUSB_ROOT_HUB_NOT_CONNECTED.
	* 4. Make pUsbDev->pPrivateData = NULL;
	*/

	/* To make sure that hub driver should not access pUsbDev during disconnect
	* handling, this function sends different message during disconnection of 
	* root hub and other hub. There is only way to know disconnected hub is
	* root hub or not through pUsbDev->pParentUsbDevice.
	*/

	pDriverContext =  (MUSB_HubDriverContext*)pDriverPrivateData;

	dwStatus = MGC_HubFindHubDevice (pUsbDevice, &pHubDevice);

	if (MUSB_SUCCESS == dwStatus)
	{
		if (pUsbDevice->pParentUsbDevice != NULL ) 
		{
			MGC_HubDisconnectHandler(pHubDevice, MUSB_NORMAL_HUB, pDriverContext); 
		}
		else
		{
			/* It means this root Hub is disconnected. */        
			MGC_HubDisconnectHandler(pHubDevice, MUSB_ROOT_HUB, pDriverContext); 
		}

		/* Everything is over for disconnected hub. Now time to make 
		* hub device FREE and reduce number of active hub in the system.
		*/
		pHubDevice->bState = MUSB_HUB_STATE_FREE;
		pDriverContext->bNumOfHubActive--;
	}
	else
	{
		MUSB_DIAG_STRING(1, "HubError: MUSB_HubDisconnect() called on non-hub");
	}   

	return;
} /* End of function MUSB_HubDisconnect () */


 /* 
  * This is an API provided by Hub Class driver, Hub class driver reset TT
  * after configuration CD will call this when identfies 
  * that the TT is in unknown state such that it no longer functioning correctly. 
  * This request moves the TT in known state
  */
uint8_t MUSB_HubResetTt(MUSB_Device   *pUsbDevice,
                        MUSB_pfControlIrpComplete ControlIrpComplete,
                        void          *pContext,
                        uint16_t       wPortNumber)
{
	MUSB_Hub                           *pHubDevice;
	MUSB_ControlIrp                    *pControlIrp;
	MUSB_DeviceRequest                 *pSetupPacket;
	uint32_t                            dwStatus;
	uint16_t                            wIndex; 

	/* Get Hub device info */
	dwStatus    = MGC_HubFindHubDevice(pUsbDevice, &pHubDevice); 
	if(MUSB_SUCCESS != dwStatus)
	{
		MUSB_DIAG_STRING(1, MGC_aHubDevNotFound);
		return(FALSE);
	}

	pControlIrp        = &(pHubDevice->ctrlIrp);
	pSetupPacket       = &(pHubDevice->setupPacket);
	/*
	* Lower Nibble(0-bit) represents the present configuration of Hub-TT
	*/
	if( MUSB_HUB_MULTIPLE_TT == (pHubDevice->bTtType & MUSB_HUB_CURRENT_TT_TYPE_BM) )
	{
		/* For Multiple-TT, Port Number(wPortNumber) should be passed as wIndex */
		wIndex    = wPortNumber; 
	}
	else if( MUSB_HUB_SINGLE_TT == (pHubDevice->bTtType & MUSB_HUB_CURRENT_TT_TYPE_BM) )
	{
		wIndex= 0x01; /* For single-TT, wIndex is one*/
	}
	else
	{
		MUSB_DIAG_STRING(1, MGC_aHubInvalidTtType);
		return(FALSE);
	}
	/* 
	* Fill the setup packet, it does swapping for 16-bit field to get
	* USB-Byte Order
	*/
	MUSB_HubPrepareSetupPkt(pSetupPacket,
					MUSB_HUB_RESET_TT_BMREQUEST,
					MUSB_HUB_REQ_RESET_TT,
					0x0000 /* wValue */,
					wIndex,
					MUSB_HUB_NO_DATA_PHASE);

	/* Fill the control IRP */
	MUSB_FILL_CTRL_IRP(pControlIrp, 
					pUsbDevice->bBusAddress, 
					pSetupPacket, 
					MUSB_HUB_SETUP_PACKET_LENGTH, 
					NULL, 
					MUSB_HUB_NO_DATA_PHASE, 
					ControlIrpComplete, 
					pContext);

	dwStatus = MUSB_StartControlTransfer (pUsbDevice->pPort, pControlIrp);
	/* Function return's 0x00 on success. */
	if (dwStatus != 0x00)
	{
		MUSB_DIAG_STRING(1, "HubError: Reset TT control request failed");
		return (FALSE);
	}

	return(TRUE);
} /* End of MUSB_HubResetTT() */


/*
 * Clears TT Errors occurred during the Control/Bulk Split transactions 
 * CD knows that which transaction failed, so, it calls this function 
 * when it encounters this error
 */
uint8_t MUSB_HubClearTtBuffer(uint8_t        bEndpointAddress,
                              uint8_t        bDeviceAdress,
                              uint8_t        bEndpointType,
                              MUSB_Device   *pUsbDevice,
                              MUSB_pfControlIrpComplete ControlIrpComplete,
                              void          *pContext)
{
	MUSB_Hub                           *pHubDevice;
	MUSB_ControlIrp                    *pControlIrp;
	MUSB_DeviceRequest                 *pSetupPacket;
	uint32_t                           dwStatus;
	uint16_t                           wValue = 0x0000;
	uint16_t                           wPortNumber; 

	/* Get Hub device info */
	dwStatus    = MGC_HubFindHubDevice(pUsbDevice, &pHubDevice);
	if(MUSB_SUCCESS != dwStatus)
	{
		MUSB_DIAG_STRING(1, MGC_aHubDevNotFound);
		return(FALSE);
	}

	pControlIrp        = &(pHubDevice->ctrlIrp);
	pSetupPacket       = &(pHubDevice->setupPacket);
	/*
	* Lower Nibble first bit(0-bit) represents the present configuration of Hub-TT
	*/
	if(MUSB_HUB_MULTIPLE_TT == (pHubDevice->bTtType & 0x01))
	{
		/* For Multiple-TT, wIndex is Port Number */
		wPortNumber = pUsbDevice->bHubPort; 
	}
	else if(MUSB_HUB_SINGLE_TT == (pHubDevice->bTtType&0x01))
	{
		wPortNumber = 1; /* Single-TT hub, wIndex is one*/
	}
	else 
	{
		MUSB_DIAG_STRING(1, MGC_aHubInvalidTtType);
		return(FALSE);
	}
	/*
	* get the wValue from the parameters 
	*    Bits 3..0 Endpoint number
	*    Bits 10..4 Device Address
	*    Bits 12..11 Endpoint Type
	*    Bits 14..13 zero
	*    Bit 15 Direction , 1 = IN, 0 = OUT
	*/
	wValue = (  (uint16_t)(bEndpointAddress & 0x0F) 
			| ( (uint16_t)bDeviceAdress << 4 )
			| ( (uint16_t)bEndpointType << 11 )
			| ( (uint16_t)(bEndpointAddress&0x80)  << 8 ) 
			);

	/* 
	* Fill the setup packet, it does swapping for 16-bit field to get
	* USB-Byte Order
	*/
	MUSB_HubPrepareSetupPkt(pSetupPacket,
					MUSB_HUB_CLEAR_TT_BUFFER_BMREQUEST,
					MUSB_HUB_REQ_CLEAR_TT_BUFFER,
					wValue,
					wPortNumber,
					MUSB_HUB_NO_DATA_PHASE);

	/* Fill the control IRP */
	MUSB_FILL_CTRL_IRP(pControlIrp, 
					pUsbDevice->bBusAddress, 
					pSetupPacket, 
					MUSB_HUB_SETUP_PACKET_LENGTH, 
					NULL, 
					MUSB_HUB_NO_DATA_PHASE, 
					ControlIrpComplete, 
					pContext);

	dwStatus = MUSB_StartControlTransfer (pUsbDevice->pPort, pControlIrp);
	/* Function return's 0x00 on success. */
	if (dwStatus != 0x00)
	{ 
		MUSB_DIAG_STRING(1, "HubError: Clear TT control request failed");
		return (FALSE);
	}

	return(TRUE);
} /* End of MUSB_HubClearTTBuffer() */


/*
 * This returns the TT related information, which will be used for 
 * programming the Split transactions( TT related registers in the HW).
 */
int8_t MUSB_HubGetTtInfo(MUSB_Device *pUsbDevice)
{
	MUSB_Hub *pHubDevice;
	uint32_t dwStatus;

	/* Get Hub device info */
	dwStatus = MGC_HubFindHubDevice(pUsbDevice, &pHubDevice);
	if(MUSB_SUCCESS != dwStatus)
	{
		MUSB_DIAG_STRING(1, MGC_aHubDevNotFound);
		return((int8_t)MUSB_HUB_INVALID_TT);
	}
	return(pHubDevice->bTtType);
} /* End of MUSB_HubGetTTInfo() */


/**
 * This needs to be called by the CD when it wants to change the TT
 * operating mode of the hub
 *
 * This changes the TT operating mode of the hub by setting the corresponding
 * alternate setting through Set Interface Request
 */
uint8_t MUSB_HubChangeTtMode(int8_t          bNewTtMode,
                             MUSB_Device    *pUsbDevice,
                             MUSB_pfControlIrpComplete ControlIrpComplete,
                             void           *pContext)
{
	MUSB_Hub                           *pHubDevice;
	MUSB_ControlIrp                    *pControlIrp;
	MUSB_DeviceRequest                 *pSetupPacket;
	uint32_t                           dwStatus;
	uint8_t                            bAlternateSetting;

	/* Get Hub device info */
	dwStatus = MGC_HubFindHubDevice(pUsbDevice, &pHubDevice);

	if(MUSB_SUCCESS != dwStatus)
	{
		MUSB_DIAG_STRING(1, MGC_aHubDevNotFound);
		return(FALSE);
	}

	if(MUSB_HUB_INVALID_TT == pHubDevice->bTtType)
	{
		MUSB_DIAG_STRING(1, MGC_aHubInvalidTtType);
		return(FALSE);
	}

	/*
	* Lower Nibble first bit(0th-Bit) represents current hub-TT configuration 
	* If the Current Hub TT configuration and the request is same then
	* return Success
	*/
	if((pHubDevice->bTtType & MUSB_HUB_CURRENT_TT_TYPE_BM) == bNewTtMode)
	{
		return(TRUE);
	}

	/* upper nibble first bit(4th-Bit) represents the Hub-TT Type */
	if((pHubDevice->bTtType & MUSB_HUB_TT_TYPE_BM) == MUSB_HUB_SINGLE_TT)
	{    
		MUSB_DIAG_STRING(1, "HubError: Single-TT hub cannot change TT type");
		return(FALSE);        
	} 

	if( (MUSB_HUB_SINGLE_TT == bNewTtMode) || (MUSB_HUB_MULTIPLE_TT == bNewTtMode) )
	{
		bAlternateSetting  =   (uint8_t)bNewTtMode;
	}
	else
	{
		MUSB_DIAG_STRING(1, "HubError: Invalid new TT type");
		return(FALSE);
	}
	pControlIrp        = &(pHubDevice->ctrlIrp);
	pSetupPacket       = &(pHubDevice->setupPacket);
	bAlternateSetting  = bNewTtMode;

	/* 
	* Fill the setup packet, it does swapping for 16-bit field to get
	* USB-Byte Order
	*/
	MUSB_HubPrepareSetupPkt(pSetupPacket,
					(uint8_t) (MUSB_DIR_OUT | MUSB_TYPE_STANDARD | MUSB_RECIP_INTERFACE),
					MUSB_REQ_SET_INTERFACE,
					(uint16_t)bAlternateSetting, /* Alternate Setting */
					0, /* Hub-Only one Interface - Interface Number */
					MUSB_HUB_NO_DATA_PHASE);

	/* Fill the control IRP */
	MUSB_FILL_CTRL_IRP(pControlIrp, 
					pUsbDevice->bBusAddress, 
					pSetupPacket, 
					MUSB_HUB_SETUP_PACKET_LENGTH, 
					NULL, 
					MUSB_HUB_NO_DATA_PHASE, 
					ControlIrpComplete, 
					pContext);

	dwStatus = MUSB_StartControlTransfer (pUsbDevice->pPort, pControlIrp);
	/* Function return's 0x00 on success. */
	if (dwStatus != 0x00)
	{
		MUSB_DIAG_STRING(1, "HubError: Set interface request failed");
		return (FALSE);
	}

	return(TRUE);
} /* End of MUSB_HubChangeTtMode() */


/*
 * This verifies that the Global suspend is possible or not
 * Check for remote wakeup feature for each device in the tree.
 * If any device is not supporting then return FALSE 
 * which are supporting remote wakeup
 * If all the devices in the tree supporting remote wakeup, then return TRUE
 * to the CD. CD will do global suspend.
 *
 * CD call this API with First hub connected to the XDRC core.
 * CD call this API whenever CD wants to do Global suspend.
 * This API should not be called when application wants to do global suspend 
 * to save the power and during that time CD do Gloabl suspend by stopping the SOFs
 */
uint8_t MUSB_HubVerifyGlobalSuspend(MUSB_Device    *pUsbDevice)
{
	/* Get Hub device info */
	if(pUsbDevice->pParentUsbDevice)
	{
		MUSB_DIAG_STRING(1, "HubError: This is not the first hub connected to the Host");
		return(FALSE);
	}
	return(MGC_HubIsAnyPortNotSupportsRemoteWakeup(pUsbDevice));
} /* End of MUSB_HubCanGlobalSuspend() */


/*
 * Selective suspend: Suspending a bus between a hub and device or port
 * This setting the PORT_SUSPEND feature causes bus traffic to cease on 
 * that port and, consequently, the device to suspend.
 */
uint8_t MUSB_HubSelectiveSuspend (MUSB_Device    *pUsbDevice,
                                  MUSB_pfControlIrpComplete ControlIrpComplete,
                                  void           *pContext)
{
	MUSB_Hub                           *pHubDevice;
	MUSB_ControlIrp                    *pControlIrp;
	MUSB_DeviceRequest                 *pSetupPacket;
	int32_t                             dwStatus;

	/* Get Hub device info */
	if(pUsbDevice->pParentUsbDevice)
	{
		dwStatus    = MGC_HubFindHubDevice(pUsbDevice->pParentUsbDevice, &pHubDevice);
		if(MUSB_SUCCESS != dwStatus)
		{
			MUSB_DIAG_STRING(1, MGC_aHubDevNotFound);
			return(FALSE);
		}
	}
	else
	{
		MUSB_DIAG_STRING(1, MGC_aNoInterveningHub);
		return(FALSE);
	}

	pControlIrp        = &(pHubDevice->ctrlIrp);
	pSetupPacket       = &(pHubDevice->setupPacket);

	/* Fill the setup packet */
	MUSB_HubPrepareSetupPkt(pSetupPacket,
					(MUSB_TYPE_CLASS | MUSB_RECIP_OTHER),
					MUSB_REQ_SET_FEATURE,
					MUSB_HUB_PORT_SUSPEND_FEATURE,
					(uint16_t)pUsbDevice->bHubPort,
					MUSB_HUB_NO_DATA_PHASE);

	/* Fill the control IRP */
	MUSB_FILL_CTRL_IRP(pControlIrp, 
					pUsbDevice->bBusAddress, 
					pSetupPacket, 
					MUSB_HUB_SETUP_PACKET_LENGTH, 
					NULL, 
					MUSB_HUB_NO_DATA_PHASE, 
					ControlIrpComplete, 
					pContext);

	dwStatus = MUSB_StartControlTransfer (pUsbDevice->pPort, pControlIrp);
	/* Function return's 0x00 on success. */
	if (dwStatus != 0x00)
	{ 
		MUSB_DIAG_STRING(1, "HubError: Set feature suspend request failed");
		return (FALSE);
	}

	return(TRUE);
} /* End of MUSB_HubSelectiveSuspend() */

/*
 * Selective Resume: Clearing a port suspend
 * This resumes the bus between the device and the hub.
 * This function will be called whenever CD/Application/Driver
 * wants to resume given port/bus/device
 */
uint8_t MUSB_HubSelectiveResume(MUSB_Device        *pUsbDevice,
                                MUSB_pfControlIrpComplete ControlIrpComplete,
                                void               *pContext)
{
	MUSB_Hub                           *pHubDevice;
	MUSB_ControlIrp                    *pControlIrp;
	MUSB_DeviceRequest                 *pSetupPacket;
	uint32_t                           dwStatus;

	/* Get Hub device info */
	if(pUsbDevice->pParentUsbDevice != NULL)
	{
		dwStatus = MGC_HubFindHubDevice(pUsbDevice->pParentUsbDevice, &pHubDevice);

		if(MUSB_SUCCESS != dwStatus)
		{
			MUSB_DIAG_STRING(1, MGC_aHubDevNotFound);
			return(FALSE);
		}
	}
	else
	{
		MUSB_DIAG_STRING(1, MGC_aNoInterveningHub);
		return(FALSE);
	}

	pControlIrp        = &(pHubDevice->ctrlIrp);
	pSetupPacket       = &(pHubDevice->setupPacket);

	/* Fill the setup packet */
	MUSB_HubPrepareSetupPkt(pSetupPacket,
					(MUSB_TYPE_CLASS | MUSB_RECIP_OTHER),
					MUSB_REQ_CLEAR_FEATURE,
					MUSB_HUB_PORT_SUSPEND_FEATURE,
					(uint16_t)pUsbDevice->bHubPort,
					MUSB_HUB_NO_DATA_PHASE);

	/* Fill the control IRP */
	MUSB_FILL_CTRL_IRP(pControlIrp, 
					pUsbDevice->bBusAddress, 
					pSetupPacket, 
					MUSB_HUB_SETUP_PACKET_LENGTH, 
					NULL, 
					MUSB_HUB_NO_DATA_PHASE, 
					ControlIrpComplete, 
					(void *) pContext);

	dwStatus = MUSB_StartControlTransfer (pUsbDevice->pPort, pControlIrp);
	/* Function return's 0x00 on success. */
	if (dwStatus != 0x00)
	{
		MUSB_DIAG_STRING(1, "HubError: Clear feature suspend request failed");
		return (FALSE);
	}

	return(TRUE);
} /* End of MUSB_HubSelectiveResume() */

