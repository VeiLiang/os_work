/*****************************************************************************
 *                                                                           *
 *      Copyright Mentor Graphics Corporation 2003-2004                      *
 *                                                                           *
 *                All Rights Reserved.                                       *
 *                                                                           *
 *  THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION            *
 *  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS              *
 *  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                               *
 *                                                                           *
 ****************************************************************************/

/*
 * IPOD driver functionality for use by upper layers
 * $Revision: 1.1 $
 */

#ifndef __MUSB_IPOD_API_H__
#define __MUSB_IPOD_API_H__

#include "plat_cnf.h"
#ifdef SYS_UCOSII
#include "TaskStack.h"
#include "includes.h"
#endif
#ifdef SYS_IAR_RTOS
#include "target_ucos_ii.h"
#endif


#ifdef SYS_UCOSII
extern OS_EVENT *IpodDeviceSem;
#endif
#ifdef SYS_IAR_RTOS
extern void *IpodDeviceSem;
#endif

#define MUSB_IPOD_VID_H						0x05
#define MUSB_IPOD_VID_L						0xAC

#define BULK_TRANSFER_TYPE					0x2
#define INT_TRANSFER_TYPE					0x3

/* Macro to prepare setup packet for IPOD Class driver*/
#define MGC_IPOD_PREPARE_SETUP_PACKET(pSetup,\
                                     bmActualRequestType,\
                                     bActualRequest,\
                                     wActualValue,\
                                     wActualIndex,\
                                     wActualLength)\
{\
    (pSetup)->bmRequestType = (uint8_t) bmActualRequestType;\
    (pSetup)->bRequest      = (uint8_t) bActualRequest;\
    (pSetup)->wValue        = (uint16_t) MUSB_SWAP16(wActualValue);\
    (pSetup)->wIndex        = (uint16_t) MUSB_SWAP16(wActualIndex);\
    (pSetup)->wLength       = (uint16_t) MUSB_SWAP16(wActualLength);\
}

/* Macro to fill control Irp for IPOD Class driver */
#define MGC_IPOD_FILL_CONTROL_IRP(pIpodDevice,\
                                 pControlIrp,\
                                 pActualOutBuffer,\
                                 dwRequestedOutLength,\
                                 pActualInBuffer,\
                                 dwRequestedInLength,\
                                 pfControlIrpComplete)\
{\
    (pControlIrp)->pDevice           = pIpodDevice->pUsbDevice;\
    (pControlIrp)->pOutBuffer        = pActualOutBuffer;\
    (pControlIrp)->dwOutLength       = dwRequestedOutLength;\
    (pControlIrp)->pInBuffer         = pActualInBuffer;\
    (pControlIrp)->dwInLength        = dwRequestedInLength;\
    (pControlIrp)->dwStatus          = 0;\
    (pControlIrp)->dwActualOutLength = 0;\
    (pControlIrp)->dwActualInLength  = 0;\
    (pControlIrp)->pfIrpComplete     = pfControlIrpComplete;\
    (pControlIrp)->pCompleteParam    = (void *) pIpodDevice;\
}

typedef struct _MGC_IPOD_USER_DATA_
{
  unsigned char *userdata;
  unsigned int userdatalen;
}MGC_IPOD_USER_DATA;

typedef struct 
{ 
    MUSB_BusHandle                  	hBus;
    MUSB_Device                    		*pUsbDevice;
    MUSB_DeviceDriver              		*pDriver;     
    void                           			*pDeviceId;
    MUSB_DeviceRequest			SetupPacket;
    MUSB_ControlIrp         			ControlIrp;
    MUSB_Irp						InterruptIrp;
    MUSB_Pipe                           	InterruptPipe;  		
    const MUSB_EndpointDescriptor	pIntEndpoint; 
    uint8_t						ctrl_buffer[128];
    uint8_t						int_buffer[64];
	
    /****for APP****/
    uint32_t 						(*user_event)( void *);
    MGC_IPOD_USER_DATA 			userData;
} MGC_IpodDevice;


/**
 * Fill an array with the targetted peripheral list entry appropriate
 * for the uvc class driver, ending with the MUSB_TARGET_ACCEPT.
 * @param pList array
 * @param wListLength how many bytes are available in the array
 * @return how many bytes were filled in the array.
 * If this equals bListLength, the caller should assume there is insufficient space
 * in the array and the list entry is incomplete.
 */
extern uint16_t MGC_FillIpodClassPeripheralList(uint8_t* pList, uint16_t wListLength);

/**
 * Get a pointer to the uvc class driver
 */
extern MUSB_DeviceDriver* MGC_GetIpodClassDriver(void);
extern MGC_IpodDevice* MGC_GetIpodDeviceContext(void);

#endif /* End of __MUSB_IPOD_API_H__ */

