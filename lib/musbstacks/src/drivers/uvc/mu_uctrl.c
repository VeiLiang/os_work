/*****************************************************************************
 *                                                                           *
 *      Copyright Mentor Graphics Corporation 2003-2006                      *
 *                                                                           *
 *                All Rights Reserved.                                       *
 *                                                                           *
 *    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION            *
 *  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS              *
 *  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                               *
 *                                                                           *
 ****************************************************************************/

/*
 * UVC client functions (driver interfacing, command transfer, and
 * command preparation)
 * $Revision: 1.6 $
 */

#include "mu_mem.h"
#include "mu_cdi.h"
#include "mu_uctrl.h"

/****************************** CONSTANTS ********************************/


/******************************** TYPES **********************************/


/******************************* FORWARDS ********************************/
static uint32_t MGC_UvcCmdGetProbeControl(void* pContext);
static uint32_t MGC_UvcCmdSetProbeControl(void* pContext, uint8_t *pConfigBuf);
static uint32_t MGC_UvcCmdSetCommitControl(void* pContext, uint8_t *pConfigBuf);
static uint32_t MGC_UvcCmdStartDevice(void* pContext);
static uint32_t MGC_UvcCmdStopDevice(void* pContext);
static void MGC_UvcReceiveCallback(void* pContext, MUSB_IsochIrp *pIsochIrp);
/******************************* GLOBALS *********************************/
static MGC_UVC_PROBE_PARAM	ProbeParam;
static uint8_t cmd_count = 0;
static uint8_t receive_finish = 0;

static uint8_t *Video_Packet = NULL;
/****************************** FUNCTIONS ********************************/

static void MGC_UvcGetProbeControlCallback (void *pContext, MUSB_ControlIrp *pControlIrp)
{
	unsigned char *pdata = NULL;

	pdata = pControlIrp->pInBuffer;
	
	ProbeParam.bmHint = (pdata[1]<<8) | pdata[0];
	ProbeParam.bFormatIndex = pdata[2];
	ProbeParam.bFrameIndex = pdata[3];
	ProbeParam.dwFrameInterval = (pdata[7]<<24)|(pdata[6]<<16)|(pdata[5]<<8) | pdata[4];
	ProbeParam.wKeyFrameRate = (pdata[9]<<8) | pdata[8];

	ProbeParam.wPFrameRate = (pdata[11]<<8) | pdata[10];
	ProbeParam.wCompQuality = (pdata[13]<<8) | pdata[12];
	ProbeParam.wCompWindowSize = (pdata[15]<<8) | pdata[14];
	ProbeParam.wDelay = (pdata[17]<<8) | pdata[16];
	ProbeParam.dwMaxVideoFrameSize = (pdata[21]<<24)|(pdata[20]<<16)|(pdata[19]<<8) | pdata[18];
	ProbeParam.dwMaxPayloadTransferSize = (pdata[21]<<25)|(pdata[24]<<16)|(pdata[23]<<8) | pdata[22];

#if 1
	MUSB_PRINTK("bmHint = 0x%x\n", ProbeParam.bmHint);
	MUSB_PRINTK("bFormatIndex = 0x%x\n", ProbeParam.bFormatIndex);
	MUSB_PRINTK("bFrameIndex = 0x%x\n", ProbeParam.bFrameIndex);
	MUSB_PRINTK("dwFrameInterval = 0x%x\n", ProbeParam.dwFrameInterval);
	MUSB_PRINTK("wKeyFrameRate = 0x%x\n", ProbeParam.wKeyFrameRate);
	MUSB_PRINTK("wPFrameRate = 0x%x\n", ProbeParam.wPFrameRate);
	MUSB_PRINTK("wCompQuality = 0x%x\n", ProbeParam.wCompQuality);
	MUSB_PRINTK("wCompWindowSize = 0x%x\n", ProbeParam.wCompWindowSize);
	MUSB_PRINTK("wDelay = 0x%x\n", ProbeParam.wDelay);
	MUSB_PRINTK("dwMaxVideoFrameSize = 0x%x\n", ProbeParam.dwMaxVideoFrameSize);
	MUSB_PRINTK("dwMaxPayloadTransferSize = 0x%x\n", ProbeParam.dwMaxPayloadTransferSize);
#endif
}

static uint32_t MGC_UvcCmdGetProbeControl(void* pContext)
{
	MGC_UvcDevice *pUvcDevice = (MGC_UvcDevice *)pContext;
	MUSB_DeviceRequest		*pSetup;
	MUSB_ControlIrp 			*pControlIrp;
	uint32_t					dwStatus = 0xFFFFFFFF;
	
	MUSB_PRINTK("===MGC_UvcCmdGetProbeControl===\n");
	
	pSetup = &(pUvcDevice->SetupPacket);
	pControlIrp = &(pUvcDevice->ControlIrp);
	
	/** Prepare the Setup Packet for sending Set Config Request */
	MGC_UVC_PREPARE_SETUP_PACKET(pSetup,
								 0xA1,
								 0x81,
								 0x100,
								 0x1,
								 0x1A);
	/** Fill Control Irp */
	MGC_UVC_FILL_CONTROL_IRP(pUvcDevice,
							  pControlIrp,
							  (uint8_t *)pSetup,
							  sizeof(MUSB_DeviceRequest),
							  pUvcDevice->ctrl_buffer,
							  0x1A,
							  MGC_UvcGetProbeControlCallback);
	
	dwStatus = MUSB_StartControlTransfer (pUvcDevice->pUsbDevice->pPort, pControlIrp);
	MUSB_PRINTK("===MGC_UvcCmdGetProbeControl dwStatus is 0x%x===\n", dwStatus); 
	return dwStatus;
}

static uint32_t MGC_UvcCmdSetProbeControl(void* pContext, uint8_t *pConfigBuf)
{
	MGC_UvcDevice *pUvcDevice = (MGC_UvcDevice *)pContext;
	MUSB_DeviceRequest		*pSetup;
	MUSB_ControlIrp 			*pControlIrp;
	MGC_UVC_PROBE_PARAM	*pProbeParam;
	uint32_t					SetupLen = sizeof(MUSB_DeviceRequest);
	uint32_t					dwStatus = 0xFFFFFFFF;
	
	MUSB_PRINTK("===MGC_UvcCmdSetProbeControl===\n");
	
	pSetup = &(pUvcDevice->SetupPacket);
	pControlIrp = &(pUvcDevice->ControlIrp);
	
	if(pConfigBuf != NULL)
	{
		unsigned char *pdata=NULL;
		pProbeParam = (MGC_UVC_PROBE_PARAM *)pConfigBuf;

		/** Prepare the Setup Packet for sending Set Config Request */
		MGC_UVC_PREPARE_SETUP_PACKET(pSetup,
									 0x21,
									 0x1,
									 0x100,
									 0x1,
									 0x1A);

		pdata = (unsigned char *)(pUvcDevice->ctrl_buffer);

		MUSB_MemCopy(pdata, pSetup, SetupLen);
		
		pdata[0 + SetupLen] = pProbeParam->bmHint;
		pdata[1 + SetupLen] = (pProbeParam->bmHint>>8);
		pdata[2 + SetupLen] = pProbeParam->bFormatIndex;
		pdata[3 + SetupLen] = pProbeParam->bFrameIndex;
		pdata[4 + SetupLen] = pProbeParam->dwFrameInterval;
		pdata[5 + SetupLen] = pProbeParam->dwFrameInterval >> 8;
		pdata[6 + SetupLen] = pProbeParam->dwFrameInterval>>16;
		pdata[7 + SetupLen] = pProbeParam->dwFrameInterval>>24;

		pdata[8 + SetupLen] = pProbeParam->wKeyFrameRate;
		pdata[9 + SetupLen] = (pProbeParam->wKeyFrameRate>>8);
		pdata[10 + SetupLen] = pProbeParam->wPFrameRate;
		pdata[11 + SetupLen] = (pProbeParam->wPFrameRate>>8);
		pdata[12 + SetupLen] = pProbeParam->wCompQuality;
		pdata[13 + SetupLen] = (pProbeParam->wCompQuality>>8);
		pdata[14 + SetupLen] = pProbeParam->wCompWindowSize;
		pdata[15 + SetupLen] = (pProbeParam->wCompWindowSize>>8);

		pdata[16 + SetupLen] = pProbeParam->wDelay;
		pdata[17 + SetupLen] = (pProbeParam->wDelay>>8);

		pdata[18 + SetupLen] = pProbeParam->dwMaxVideoFrameSize;
		pdata[19 + SetupLen] = (pProbeParam->dwMaxVideoFrameSize>>8);
		pdata[20 + SetupLen] = pProbeParam->dwMaxVideoFrameSize >> 16;
		pdata[21 + SetupLen] = (pProbeParam->dwMaxVideoFrameSize>>24);

		pdata[22 + SetupLen] = pProbeParam->dwMaxPayloadTransferSize;
		pdata[23 + SetupLen] = (pProbeParam->dwMaxPayloadTransferSize>>8);
		pdata[24 + SetupLen] = pProbeParam->dwMaxPayloadTransferSize >> 16;
		pdata[25 + SetupLen] = (pProbeParam->dwMaxPayloadTransferSize>>24);

		/** Fill Control Irp */
		MGC_UVC_FILL_CONTROL_IRP(pUvcDevice,
								  pControlIrp,
								  pUvcDevice->ctrl_buffer,
								  SetupLen + 0x1A,
								  NULL,
								  0,
								  NULL);

		dwStatus = MUSB_StartControlTransfer (pUvcDevice->pUsbDevice->pPort, pControlIrp);
		MUSB_PRINTK("===MGC_UvcCmdSetProbeControl dwStatus is 0x%x===\n", dwStatus); 
	}
	else
		MUSB_PRINTK("===MGC_UvcCmdSetProbeControl pConfigBuf is NULL===\n"); 

	return dwStatus;
}

static uint32_t MGC_UvcCmdSetCommitControl(void* pContext, uint8_t *pConfigBuf)
{
	MGC_UvcDevice *pUvcDevice = (MGC_UvcDevice *)pContext;
	MUSB_DeviceRequest		*pSetup;
	MUSB_ControlIrp 			*pControlIrp;
	MGC_UVC_PROBE_PARAM	*pProbeParam;
	uint32_t					SetupLen = sizeof(MUSB_DeviceRequest);
	uint32_t					dwStatus = 0xFFFFFFFF;
	
	MUSB_PRINTK("===MGC_UvcCmdSetCommitControl===\n");
	
	pSetup = &(pUvcDevice->SetupPacket);
	pControlIrp = &(pUvcDevice->ControlIrp);

	if(pConfigBuf != NULL)
	{
		unsigned char *pdata=NULL;
		pProbeParam = (MGC_UVC_PROBE_PARAM *)pConfigBuf;

		/** Prepare the Setup Packet for sending Set Config Request */
		MGC_UVC_PREPARE_SETUP_PACKET(pSetup,
									 0x21,
									 0x1,
									 0x200,
									 0x1,
									 0x1A);
				
		pdata = (unsigned char *)(pUvcDevice->ctrl_buffer);
		
		MUSB_MemCopy(pdata, pSetup, SetupLen);
		
		pdata[0 + SetupLen] = pProbeParam->bmHint;
		pdata[1 + SetupLen] = (pProbeParam->bmHint>>8);
		pdata[2 + SetupLen] = pProbeParam->bFormatIndex;
		pdata[3 + SetupLen] = pProbeParam->bFrameIndex;
		pdata[4 + SetupLen] = pProbeParam->dwFrameInterval;
		pdata[5 + SetupLen] = pProbeParam->dwFrameInterval >> 8;
		pdata[6 + SetupLen] = pProbeParam->dwFrameInterval>>16;
		pdata[7 + SetupLen] = pProbeParam->dwFrameInterval>>24;
		
		pdata[8 + SetupLen] = pProbeParam->wKeyFrameRate;
		pdata[9 + SetupLen] = (pProbeParam->wKeyFrameRate>>8);
		pdata[10 + SetupLen] = pProbeParam->wPFrameRate;
		pdata[11 + SetupLen] = (pProbeParam->wPFrameRate>>8);
		pdata[12 + SetupLen] = pProbeParam->wCompQuality;
		pdata[13 + SetupLen] = (pProbeParam->wCompQuality>>8);
		pdata[14 + SetupLen] = pProbeParam->wCompWindowSize;
		pdata[15 + SetupLen] = (pProbeParam->wCompWindowSize>>8);
		
		pdata[16 + SetupLen] = pProbeParam->wDelay;
		pdata[17 + SetupLen] = (pProbeParam->wDelay>>8);
		
		pdata[18 + SetupLen] = pProbeParam->dwMaxVideoFrameSize;
		pdata[19 + SetupLen] = (pProbeParam->dwMaxVideoFrameSize>>8);
		pdata[20 + SetupLen] = pProbeParam->dwMaxVideoFrameSize >> 16;
		pdata[21 + SetupLen] = (pProbeParam->dwMaxVideoFrameSize>>24);
		
		pdata[22 + SetupLen] = pProbeParam->dwMaxPayloadTransferSize;
		pdata[23 + SetupLen] = (pProbeParam->dwMaxPayloadTransferSize>>8);
		pdata[24 + SetupLen] = pProbeParam->dwMaxPayloadTransferSize >> 16;
		pdata[25 + SetupLen] = (pProbeParam->dwMaxPayloadTransferSize>>24);

		/** Fill Control Irp */
		MGC_UVC_FILL_CONTROL_IRP(pUvcDevice,
								  pControlIrp,
								  pUvcDevice->ctrl_buffer,
								  SetupLen + 0x1A,
								  NULL,
								  0,
								  NULL);

		dwStatus = MUSB_StartControlTransfer (pUvcDevice->pUsbDevice->pPort, pControlIrp);
		MUSB_PRINTK("===MGC_UvcCmdSetCommitControl dwStatus is 0x%x===\n", dwStatus); 
	}
	else
		MUSB_PRINTK("===MGC_UvcCmdSetCommitControl pConfigBuf is NULL===\n"); 

	return dwStatus;
}

static uint32_t MGC_UvcCmdStartDevice(void* pContext)
{
	MGC_UvcDevice *pUvcDevice = (MGC_UvcDevice *)pContext;
	MUSB_DeviceRequest		*pSetup;
	MUSB_ControlIrp 			*pControlIrp;
	MGC_UVC_PROBE_PARAM	*pProbeParam;
	uint32_t					dwStatus = 0xFFFFFFFF;
	
	MUSB_PRINTK("===MGC_UvcCmdStartDevice===\n");
	
	pSetup = &(pUvcDevice->SetupPacket);
	pControlIrp = &(pUvcDevice->ControlIrp);

	/** Prepare the Setup Packet for sending Set Config Request */
	MGC_UVC_PREPARE_SETUP_PACKET(pSetup,
								MUSB_DIR_OUT | MUSB_TYPE_STANDARD |MUSB_RECIP_INTERFACE,
								MUSB_REQ_SET_INTERFACE,
								0x1,
								0x1, 
								0x0);
	/** Fill Control Irp */
	MGC_UVC_FILL_CONTROL_IRP(pUvcDevice,
							  pControlIrp,
							  (uint8_t *)pSetup,
							  sizeof(MUSB_DeviceRequest),
							  pUvcDevice->ctrl_buffer,
							  0x0,
							  NULL);

	dwStatus = MUSB_StartControlTransfer (pUvcDevice->pUsbDevice->pPort, pControlIrp);	
	MUSB_PRINTK("===MGC_UvcCmdStartDevice dwStatus is 0x%x===\n", dwStatus); 
	return dwStatus;
}

static uint32_t MGC_UvcCmdStopDevice(void* pContext)
{
	MGC_UvcDevice *pUvcDevice = (MGC_UvcDevice *)pContext;
	MUSB_DeviceRequest		*pSetup;
	MUSB_ControlIrp 			*pControlIrp;
	MGC_UVC_PROBE_PARAM	*pProbeParam;
	uint32_t					dwStatus = 0xFFFFFFFF;
	
	MUSB_PRINTK("===MGC_UvcCmdStopDevice===\n");
	
	pSetup = &(pUvcDevice->SetupPacket);
	pControlIrp = &(pUvcDevice->ControlIrp);

	/** Prepare the Setup Packet for sending Set Config Request */
	MGC_UVC_PREPARE_SETUP_PACKET(pSetup,
								MUSB_DIR_OUT | MUSB_TYPE_STANDARD |MUSB_RECIP_INTERFACE,
								MUSB_REQ_SET_INTERFACE,
								0x0,
								0x1, 
								0x0);
	/** Fill Control Irp */
	MGC_UVC_FILL_CONTROL_IRP(pUvcDevice,
							  pControlIrp,
							  (uint8_t *)pSetup,
							  sizeof(MUSB_DeviceRequest),
							  pUvcDevice->ctrl_buffer,
							  0x0,
							  NULL);

	dwStatus = MUSB_StartControlTransfer (pUvcDevice->pUsbDevice->pPort, pControlIrp);	
	MUSB_PRINTK("===MGC_UvcCmdStopDevice dwStatus is 0x%x===\n", dwStatus); 
	return dwStatus;
}

void MGC_UvcStartReceive(MGC_UvcDevice *pUvcDevice)
{
    	MUSB_IsochIrp *pIsochIrp = &(pUvcDevice->IsochIrp);
	uint16_t	wFrameIndex = pUvcDevice->userVideoFrame.curpackindex;
	uint32_t	dwStatus = 0xFFFFFFFF;
	
	if(!pUvcDevice->IsochIrp.adwLength)
		pUvcDevice->IsochIrp.adwLength = MUSB_MemAlloc(UVC_MAX_ISO_PACKETS*sizeof(uint32_t));
	if(!pUvcDevice->IsochIrp.adwStatus)
		pUvcDevice->IsochIrp.adwStatus = MUSB_MemAlloc(UVC_MAX_ISO_PACKETS*sizeof(uint32_t));
	if(!pUvcDevice->IsochIrp.adwActualLength)
		pUvcDevice->IsochIrp.adwActualLength = MUSB_MemAlloc(UVC_MAX_ISO_PACKETS*sizeof(uint32_t));

	if(pUvcDevice->userVideoFrame.frameBuf1 && pUvcDevice->userVideoFrame.frameBuf2)
	{
		if(!Video_Packet)
			Video_Packet = pUvcDevice->userVideoFrame.frameBuf1;

		pUvcDevice->userVideoFrame.curFrameBuf = Video_Packet;
		
		if(pUvcDevice->userVideoFrame.curFrameBuf)
		{
			//pUvcDevice->userVideoFrame.curFrameBuf += pIsochIrp->adwActualLength[wFrameIndex-1];
			pUvcDevice->userVideoFrame.curFrameBuf += PACKET_LEN*wFrameIndex;
				
			//printk("index: %d\n", wFrameIndex);
			/* Fill the Iso IRP */
			pIsochIrp->hPipe = pUvcDevice->isoInPipe;
			pIsochIrp->pBuffer = pUvcDevice->userVideoFrame.curFrameBuf;
			pIsochIrp->wFrameCount = UVC_MAX_ISO_PACKETS;
			pIsochIrp->wCurrentFrame = 0;
			pIsochIrp->bIsrCallback = TRUE;
			pIsochIrp->pfIrpComplete = MGC_UvcReceiveCallback;
			pIsochIrp->pCompleteParam = (void *)pUvcDevice;

			while(dwStatus)
			{
				dwStatus = MUSB_ScheduleIsochTransfer(0, pIsochIrp);
			}
		}
	}
}

/** Callback function*/
static void MGC_UvcReceiveCallback(void* pContext, MUSB_IsochIrp *pIsochIrp)
{
	MGC_UvcDevice		*pUvcDevice = (MGC_UvcDevice  *)pContext;
	uint16_t				wFrameIndex;
	uint8_t				*udata;
	uint32_t				ulen;
	uint32_t				index;
	
	wFrameIndex = pIsochIrp->wCurrentFrame;

	if(pIsochIrp->adwStatus[wFrameIndex-1] == MUSB_STATUS_OK)
	{
		udata = pIsochIrp->pBuffer;
		ulen = pIsochIrp->adwActualLength[wFrameIndex-1];
		index = (udata[3]<<8 | udata[2]);

		if ((pUvcDevice->userVideoFrame.curFrameBuf + ulen - Video_Packet) > (PACKET_COUNT_PRE_FRAME*PACKET_LEN))
		{
			MUSB_PRINTK("UVC curFrameBuf must be error!!!\n");
			MUSB_PRINTK("MGC 0x%x, 0x%x, 0x%x\n", pUvcDevice->userVideoFrame.curFrameBuf, ulen, Video_Packet);
		}

		if (ulen < 2 || udata[0] < 2 || udata[0] > ulen)
		{
			pUvcDevice->userVideoFrame.curframelen = 0;
		}
		else if (udata[1] & UVC_STREAM_ERR)
		{
			pUvcDevice->userVideoFrame.curframelen = 0;
		}
		else if ((udata[1] & 0x80) != 0x80)
		{
			pUvcDevice->userVideoFrame.curframelen = 0;
		}
		else
		{
			if(udata[1] & UVC_STREAM_EOF)
			{
				if(pUvcDevice->userVideoFrame.curframelen == pUvcDevice->userVideoFrame.maxframelen)
				{
					pUvcDevice->userVideoFrame.curFrameBuf[2] = ulen;
					pUvcDevice->userVideoFrame.curFrameBuf[3] = ulen >> 8;
				
					pUvcDevice->userData.usertype = UVC_VIDEO_FRAME_EVENT;
					pUvcDevice->userData.userdata = Video_Packet;

					if(receive_finish)
					{
						if(pUvcDevice->user_event)
							pUvcDevice->user_event(&(pUvcDevice->userData));
						
						if(pUvcDevice->userVideoFrame.frameBuf1 == Video_Packet)
							Video_Packet = pUvcDevice->userVideoFrame.frameBuf2;
						else if(pUvcDevice->userVideoFrame.frameBuf2 == Video_Packet)
							Video_Packet = pUvcDevice->userVideoFrame.frameBuf1;

						receive_finish = 0;
					}
				}
				pUvcDevice->userVideoFrame.curpackindex = 0;
				pUvcDevice->userVideoFrame.curframelen = 0;
			}
			else
			{
				pUvcDevice->userVideoFrame.curFrameBuf[2] = ulen;
				pUvcDevice->userVideoFrame.curFrameBuf[3] = ulen >> 8;

				pUvcDevice->userVideoFrame.curpackindex++;
				pUvcDevice->userVideoFrame.curframelen += (ulen-udata[0]);
				if (pUvcDevice->userVideoFrame.curpackindex > PACKET_COUNT_PRE_FRAME)
				{
					pUvcDevice->userVideoFrame.curpackindex = 0;
					pUvcDevice->userVideoFrame.curframelen = 0;
				}
			}
		}
	}

	MGC_UvcStartReceive(pContext);
}

uint32_t MUSB_UVC_Control(uint16_t Command, void *Arg)
{
	uint32_t dwStatus = 0xFFFFFFFF;
	uint8_t myerr;
	MGC_UvcDevice *pUvcDevice =  MGC_GetUvcDeviceContext();
	MGC_UVC_PROBE_PARAM *pProbeParam = (MGC_UVC_PROBE_PARAM *)Arg;
	MGC_UVC_VIDEO_FRAME *pUvcVideoFrame = (MGC_UVC_VIDEO_FRAME *)Arg;
	
	if(!pUvcDevice)
	{
		MUSB_PRINTK("UvcDevice is not Present \n");
		return dwStatus;
	}

	if((!pProbeParam) && (Command < REQUEST_START_UVC))
	{
		MUSB_PRINTK("Arg is NULL \n");
		return dwStatus;
	}
	
	if(cmd_count)
#ifdef SYS_UCOSII		
		OSSemPend(UvcDeviceSem, 0, &myerr);
#else
		OS_WaitCSema (&UvcDeviceSem);
#endif

	cmd_count++;
	switch(Command)
	{
		case REGISTER_USER_FUNC:
		{
			pUvcDevice->user_event = (uint32_t (*)(void *))Arg;
			dwStatus = 0;
			break;
		}
		case REGISTER_USER_ARG:
		{
			pUvcDevice->userVideoFrame.maxframelen = pUvcVideoFrame->maxframelen;
			pUvcDevice->userVideoFrame.frameBuf1 = pUvcVideoFrame->frameBuf1;
			pUvcDevice->userVideoFrame.frameBuf2 = pUvcVideoFrame->frameBuf2;
			dwStatus = 0;
			break;
		}
		case REQUEST_GETPROBE_CONTROL:
		{
			MUSB_MemSet(&ProbeParam, 0, sizeof(MGC_UVC_PROBE_PARAM));
			dwStatus = MGC_UvcCmdGetProbeControl(pUvcDevice);
			MUSB_MemCopy(pProbeParam, &ProbeParam, sizeof(MGC_UVC_PROBE_PARAM));
			OSTimeDly(50);
			break;
		}
		case REQUEST_SETPROBE_CONTROL:
		{
			dwStatus = MGC_UvcCmdSetProbeControl(pUvcDevice, (uint8_t *)pProbeParam);
			OSTimeDly(50);
			break;
		}
		case REQUEST_GETCOMMIT_CONTROL:
		{
			MUSB_PRINTK("This Command is not Support \n");
			break;
		}
		case REQUEST_SETCOMMIT_CONTROL:
		{
			dwStatus = MGC_UvcCmdSetCommitControl(pUvcDevice, (uint8_t *)pProbeParam);	
			OSTimeDly(50);
			break;
		}
		case REQUEST_START_UVC:
		{
			receive_finish = 1;
			dwStatus = MGC_UvcCmdStartDevice(pUvcDevice);
			MUSB_PRINTK("MUSB_UVC_Control Command is 0x%x Status is 0x%x \n", Command, dwStatus);
			OSTimeDly(50);
			break;
		}
		case REQUEST_STOP_UVC:
		{
			dwStatus = MGC_UvcCmdStopDevice(pUvcDevice);	
			MUSB_PRINTK("MUSB_UVC_Control Command is 0x%x Status is 0x%x \n", Command, dwStatus);
			OSTimeDly(50);
			break;
		}
		case REQUEST_UVC_RECEIVE:
		{
			MGC_UvcStartReceive(pUvcDevice);
			dwStatus = 0;
			OSTimeDly(50);
			break;
		}
		case REQUEST_UVC_RECEIVE_FINISH:
		{
			receive_finish = 1;
			dwStatus = 0;
			break;
		}
		default:
			MUSB_PRINTK("This Command is not Support \n");
			break;
	}
	//MUSB_PRINTK("MUSB_UVC_Control Command is 0x%x Status is 0x%x \n", Command, dwStatus);
	
#ifdef SYS_UCOSII		
	OSSemPost(UvcDeviceSem);
#else
	OS_SignalCSema (&UvcDeviceSem);
#endif
	cmd_count--;
	return dwStatus;
}
