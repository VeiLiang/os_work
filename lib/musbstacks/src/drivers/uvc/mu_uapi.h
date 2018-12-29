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
 * UVC driver functionality for use by upper layers
 * $Revision: 1.5 $
 */

#ifndef __MUSB_UVC_API_H__
#define __MUSB_UVC_API_H__

#include "plat_cnf.h"
//#include "TaskStack.h"


/* Debug message values for success and error */
#define MUSB_MSD_DIAG_ERROR			0x1     
#define MUSB_MSD_DIAG_SUCCESS			0x2


#ifdef SYS_UCOSII
extern OS_EVENT *UvcDeviceSem;
#else
extern OS_CSEMA UvcDeviceSem;
#endif

//这里MAX_ENDPOINT_LEN  是硬件决定了，实际上是可以从控制器初始化中读取到
#define MAX_ENDPOINT_LEN					0x2000

/* Maximum number of packets per isochronous URB. */
#define UVC_MAX_ISO_PACKETS					1
#define UVC_MAX_ISO_GROUP					2			//video  audio
#define UVC_MAX_PACKET_LEN					MAX_ENDPOINT_LEN

#define UVC_AUDIO_INTERFACE_CLASS			0x1
#define UVC_VIDEO_INTERFACE_CLASS			0xE

#define UVC_AUDIO_INTERFACE_SUBCLASS		0x1
#define UVC_VIDEO_INTERFACE_SUBCLASS			0x2

#define INT_TRANSFER_TYPE					0x3
#define ISO_TRANSFER_TYPE					0x5

/* Macro to prepare setup packet for UVC Class driver*/
#define MGC_UVC_PREPARE_SETUP_PACKET(pSetup,\
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

/* Macro to fill control Irp for UVC Class driver */
#define MGC_UVC_FILL_CONTROL_IRP(pUvcDevice,\
                                 pControlIrp,\
                                 pActualOutBuffer,\
                                 dwRequestedOutLength,\
                                 pActualInBuffer,\
                                 dwRequestedInLength,\
                                 pfControlIrpComplete)\
{\
    (pControlIrp)->pDevice           = pUvcDevice->pUsbDevice;\
    (pControlIrp)->pOutBuffer        = pActualOutBuffer;\
    (pControlIrp)->dwOutLength       = dwRequestedOutLength;\
    (pControlIrp)->pInBuffer         = pActualInBuffer;\
    (pControlIrp)->dwInLength        = dwRequestedInLength;\
    (pControlIrp)->dwStatus          = 0;\
    (pControlIrp)->dwActualOutLength = 0;\
    (pControlIrp)->dwActualInLength  = 0;\
    (pControlIrp)->pfIrpComplete     = pfControlIrpComplete;\
    (pControlIrp)->pCompleteParam    = (void *) pUvcDevice;\
}

typedef struct _MGC_UVC_USER_DATA_
{
  int usertype;
  unsigned char *userdata;
  unsigned int userdatalen;
}MGC_UVC_USER_DATA;

typedef struct _MGC_UVC_VIDEO_FRAME_
{
  uint32_t curpackindex;    	//当前包的数
  uint32_t curframelen;     	//当前计算帧的长度
  uint32_t maxframelen;     	//帧的长度
  
  uint8_t *frameBuf1;       	// 用来做pingbang BUFFER 1
  uint8_t *frameBuf2;       	// 用来做pingbang BUFFER 2
  uint8_t *curFrameBuf;		//当前操作的BUFFER
}MGC_UVC_VIDEO_FRAME;

typedef struct 
{
    uint8_t                         			bBusAddress;             
    uint8_t                         			bInterfaceNumber;			
    uint8_t                         			bAlternateSetting;
    uint8_t                         			bProtocol;
    uint8_t                         			bSubclass;
    uint8_t						ctrl_buffer[64];
    MUSB_BusHandle                  	hBus;
    MUSB_Device                    		*pUsbDevice;
    MUSB_DeviceDriver              		*pDriver;     
    void                           			*pDeviceId;
    MUSB_DeviceRequest			SetupPacket;
    MUSB_ControlIrp         			ControlIrp;	
    MUSB_IsochIrp					IsochIrp;
    MUSB_Pipe                           	isoInPipe;  	
    const MUSB_EndpointDescriptor	pIsoInEndpoint; 
    const MUSB_EndpointDescriptor	pIsoOutEndpoint; 
    /****for APP****/
    uint32_t 						(*user_event)( void *);
    MGC_UVC_USER_DATA 			userData;
    MGC_UVC_VIDEO_FRAME			userVideoFrame;
} MGC_UvcDevice;


/**
 * Fill an array with the targetted peripheral list entry appropriate
 * for the uvc class driver, ending with the MUSB_TARGET_ACCEPT.
 * @param pList array
 * @param wListLength how many bytes are available in the array
 * @return how many bytes were filled in the array.
 * If this equals bListLength, the caller should assume there is insufficient space
 * in the array and the list entry is incomplete.
 */
extern uint16_t MGC_FillUvcClassPeripheralList(uint8_t* pList, uint16_t wListLength);

/**
 * Get a pointer to the uvc class driver
 */
extern MUSB_DeviceDriver* MGC_GetUvcClassDriver(void);
extern MGC_UvcDevice* MGC_GetUvcDeviceContext(void);

#endif /* End of __MUSB_UVC_API_H__ */

