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
 * $Revision: 1.2 $
 */
#include "mu_mem.h"
#include "mu_cdi.h"
#include "mu_uctrl.h"
#include "xm_queue.h"
#include "rtos.h"
#include "mu_impl.h"

#if 0
#define UVC_DBG		printf
#else
#define UVC_DBG
#endif
/****************************** CONSTANTS ********************************/


/******************************** TYPES **********************************/


/******************************* FORWARDS ********************************/
static uint32_t MGC_UvcCmdGetProbeControl(void* pContext);
static uint32_t MGC_UvcCmdSetProbeControl(void* pContext, uint8_t *pConfigBuf);
static uint32_t MGC_UvcCmdSetCommitControl(void* pContext, uint8_t *pConfigBuf);
static void MGC_VideoReceiveCallback(void* pContext, MUSB_IsochIrp *pIsochIrp);
static void MGC_AudioReceiveCallback(void* pContext, MUSB_IsochIrp *pIsochIrp);
/******************************* GLOBALS *********************************/
static MGC_UVC_PROBE_PARAM	ProbeParam;
static uint8_t cmd_count = 0;
static uint8_t receive_finish = 0;

static uint8_t *Video_Packet = NULL;
static uint8_t *Audio_Packet = NULL;


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
	MUSB_PRINTK("=== MGC_UvcGetProbeControl ===\n");
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
	MUSB_PRINTK("=============================\n");
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
	//int i;
	
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

		//for(i=0; i<26; i++)
			//UVC_DBG("### pdata[%d]:%x\n",i,pdata[i + SetupLen]);

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

static uint32_t MGC_UvcCmdOpenVideo(void* pContext)
{
	MGC_UvcDevice *pUvcDevice = (MGC_UvcDevice *)pContext;
	MUSB_DeviceRequest		*pSetup;
	MUSB_ControlIrp 			*pControlIrp;
	MGC_UVC_PROBE_PARAM	*pProbeParam;
    uint8_t						bAlternateSetting = 0;
	uint32_t					dwStatus = 0xFFFFFFFF;
	
	MUSB_PRINTK("===MGC_UvcCmdOpenVideo===\n");
	
	pSetup = &(pUvcDevice->SetupPacket);
	pControlIrp = &(pUvcDevice->ControlIrp);
	bAlternateSetting = pUvcDevice->pIsoVideoInterface.bAlternateSetting;	//端点
	MUSB_PRINTK("### bAlternateSetting:%d\n",bAlternateSetting);

	/** Prepare the Setup Packet for sending Set Config Request */
	MGC_UVC_PREPARE_SETUP_PACKET(pSetup,
								MUSB_DIR_OUT | MUSB_TYPE_STANDARD |MUSB_RECIP_INTERFACE,
								MUSB_REQ_SET_INTERFACE,
								bAlternateSetting,//0x6,
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
	MUSB_PRINTK("===MGC_UvcCmdOpenVideo dwStatus is 0x%x===\n", dwStatus); 
	return dwStatus;
}

static uint32_t MGC_UvcCmdStopVideo(void* pContext)
{
	MGC_UvcDevice *pUvcDevice = (MGC_UvcDevice *)pContext;
	MUSB_DeviceRequest		*pSetup;
	MUSB_ControlIrp 			*pControlIrp;
	MGC_UVC_PROBE_PARAM	*pProbeParam;
	uint32_t					dwStatus = 0xFFFFFFFF;

#if 0
	MUSB_IsochIrp *pIsochIrp = &(pUvcDevice->IsoVideoIrp);
	MGC_Pipe* pPipe = (MGC_Pipe*)pIsochIrp->hPipe;
	MGC_EndpointResource* pEnd = pPipe->pLocalEnd;
	
	MUSB_CancelIsochTransfer(pIsochIrp);
	
	if(pEnd->pRxIrp)
	{
		pEnd->pRxIrp = NULL;
	}
#endif

	MUSB_PRINTK("===MGC_UvcCmdStopVideo===\n");
	
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
	MUSB_PRINTK("===MGC_UvcCmdStopVideo dwStatus is 0x%x===\n", dwStatus); 

	return dwStatus;
}

static uint32_t MGC_UvcCleanVideoIrp(void* pContext)
{
	MGC_UvcDevice *pUvcDevice = (MGC_UvcDevice *)pContext;
	MUSB_IsochIrp *pIsochIrp = &(pUvcDevice->IsoVideoIrp);
	MGC_Pipe* pPipe = (MGC_Pipe*)pIsochIrp->hPipe;
	MGC_EndpointResource* pEnd = pPipe->pLocalEnd;
	
	MUSB_PRINTK("===MGC_UvcCleanVideoIrp===\n");
	MUSB_CancelIsochTransfer(pIsochIrp);
	
	if(pEnd->pRxIrp)
	{
		pEnd->pRxIrp = NULL;
	}

	return 0;
}

static uint32_t MGC_UvcCmdOpenAudio(void* pContext)
{
	MGC_UvcDevice *pUvcDevice = (MGC_UvcDevice *)pContext;
	MUSB_DeviceRequest		*pSetup;
	MUSB_ControlIrp 			*pControlIrp;
	MGC_UVC_PROBE_PARAM	*pProbeParam;
    uint8_t						bAlternateSetting = 0;
	uint32_t					dwStatus = 0xFFFFFFFF;
	
	MUSB_PRINTK("===MGC_UvcCmdOpenAudio===\n");
	
	pSetup = &(pUvcDevice->SetupPacket);
	pControlIrp = &(pUvcDevice->ControlIrp);
	bAlternateSetting = pUvcDevice->pIsoAudioInterface.bAlternateSetting;

	/** Prepare the Setup Packet for sending Set Config Request */
	MGC_UVC_PREPARE_SETUP_PACKET(pSetup,
								MUSB_DIR_OUT | MUSB_TYPE_STANDARD |MUSB_RECIP_INTERFACE,
								MUSB_REQ_SET_INTERFACE,
								bAlternateSetting,//0x1,
								0x3, 
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
	MUSB_PRINTK("===MGC_UvcCmdOpenAudio dwStatus is 0x%x===\n", dwStatus); 
	return dwStatus;
}

static uint32_t MGC_UvcCmdStopAudio(void* pContext)
{
	MGC_UvcDevice *pUvcDevice = (MGC_UvcDevice *)pContext;
	MUSB_DeviceRequest		*pSetup;
	MUSB_ControlIrp 			*pControlIrp;
	MGC_UVC_PROBE_PARAM	*pProbeParam;
	uint32_t					dwStatus = 0xFFFFFFFF;
	
	MUSB_PRINTK("===MGC_UvcCmdStopAudio===\n");
	
	pSetup = &(pUvcDevice->SetupPacket);
	pControlIrp = &(pUvcDevice->ControlIrp);

	/** Prepare the Setup Packet for sending Set Config Request */
	MGC_UVC_PREPARE_SETUP_PACKET(pSetup,
								MUSB_DIR_OUT | MUSB_TYPE_STANDARD |MUSB_RECIP_INTERFACE,
								MUSB_REQ_SET_INTERFACE,
								0x0,
								0x3, 
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
	MUSB_PRINTK("===MGC_UvcCmdStopAudio dwStatus is 0x%x===\n", dwStatus); 
	return dwStatus;
}

extern unsigned char lg_Usb_Present;
void MGC_UvcStartVideoReceive(MGC_UvcDevice *pUvcDevice)
{
    MUSB_IsochIrp *pIsochIrp = &(pUvcDevice->IsoVideoIrp);
	uint16_t	wFrameIndex = pUvcDevice->userVideoFrame.curpackindex;
	uint32_t	dwStatus = 0xFFFFFFFF;

	video_frame_s *curr_frame = pUvcDevice->userVideoFrame.frame;
    MGC_UVC_USER_DATA userVideoData = pUvcDevice->userVideoData;
	
//	queue_s *video_ready_fifo = pUvcDevice->userVideoFrame.ready_fifo;
//	queue_s *video_free_fifo = pUvcDevice->userVideoFrame.free_fifo;
	
	if(!pUvcDevice->IsoVideoIrp.adwLength)
		pUvcDevice->IsoVideoIrp.adwLength = MUSB_MemAlloc(UVC_MAX_ISO_PACKETS*sizeof(uint32_t));
	if(!pUvcDevice->IsoVideoIrp.adwStatus)
		pUvcDevice->IsoVideoIrp.adwStatus = MUSB_MemAlloc(UVC_MAX_ISO_PACKETS*sizeof(uint32_t));
	if(!pUvcDevice->IsoVideoIrp.adwActualLength)
		pUvcDevice->IsoVideoIrp.adwActualLength = MUSB_MemAlloc(UVC_MAX_ISO_PACKETS*sizeof(uint32_t));
	
	//if(video_ready_fifo && video_free_fifo)
	if(pUvcDevice->userVideoFrame.UserVideoAudioPrivateCallback)
	{		
		if(curr_frame == NULL || curr_frame->cur == NULL)
		{
			userVideoData.usertype = UVC_VIDEO_GET_FREE_BUF_EVENT;
			curr_frame = (video_frame_s *)pUvcDevice->userVideoFrame.UserVideoAudioPrivateCallback(&userVideoData);
			if(curr_frame == NULL)
				return ;
			
			curr_frame->cur = curr_frame->buf;
			curr_frame->id = 0;
			curr_frame->len = 0;
		}

		pUvcDevice->userVideoFrame.curFrameBuf = curr_frame->cur;
		pUvcDevice->userVideoFrame.frame = curr_frame;
		
		if(pUvcDevice->userVideoFrame.curFrameBuf)
		{
			//pUvcDevice->userVideoFrame.curFrameBuf += pIsochIrp->adwActualLength[wFrameIndex-1];
			//pUvcDevice->userVideoFrame.curFrameBuf += PACKET_LEN*wFrameIndex;
				
			//printk("index: %d\n", wFrameIndex);
			/* Fill the Iso IRP */
			pIsochIrp->hPipe = pUvcDevice->isoVideoPipe;
			pIsochIrp->pBuffer = pUvcDevice->userVideoFrame.curFrameBuf;
			pIsochIrp->wFrameCount = UVC_MAX_ISO_PACKETS;
			pIsochIrp->wCurrentFrame = 0;
			pIsochIrp->bIsrCallback = TRUE;
			pIsochIrp->pfIrpComplete = MGC_VideoReceiveCallback;
			pIsochIrp->pCompleteParam = (void *)pUvcDevice;

			dwStatus = MUSB_ScheduleIsochTransfer(0, pIsochIrp);
			if(dwStatus)
			{
				OSTimeDly(3);	//uvc 拔出时，延时等待uvc 中断先执行
				if(lg_Usb_Present)
				{
						MGC_UvcCleanVideoIrp(pUvcDevice);	//uvc 异常停止时取消iso 传输
						//MUSB_CancelIsochTransfer(pIsochIrp);	//此函数清除无效
				}

				UVC_DBG("### MUSB_ScheduleIsochTransfer failed dwStatus:%x\n",dwStatus);

				userVideoData.usertype = UVC_VIDEO_RECAPTURE_EVENT;
				if(pUvcDevice)
					pUvcDevice->userVideoFrame.UserVideoAudioPrivateCallback(&userVideoData);
			}
		}
	}
}

void MGC_UvcStartAudioReceive(MGC_UvcDevice *pUvcDevice)
{
    MUSB_IsochIrp *pIsochIrp = &(pUvcDevice->IsoAudioIrp);
	uint16_t	wFrameIndex = pUvcDevice->userAudioFrame.curpackindex;
	uint32_t	dwStatus = 0xFFFFFFFF;
	
	if(!pUvcDevice->IsoAudioIrp.adwLength)
		pUvcDevice->IsoAudioIrp.adwLength = MUSB_MemAlloc(UVC_MAX_ISO_PACKETS*sizeof(uint32_t));
	if(!pUvcDevice->IsoAudioIrp.adwStatus)
		pUvcDevice->IsoAudioIrp.adwStatus = MUSB_MemAlloc(UVC_MAX_ISO_PACKETS*sizeof(uint32_t));
	if(!pUvcDevice->IsoAudioIrp.adwActualLength)
		pUvcDevice->IsoAudioIrp.adwActualLength = MUSB_MemAlloc(UVC_MAX_ISO_PACKETS*sizeof(uint32_t));

	if(pUvcDevice->userAudioFrame.frameBuf1 && pUvcDevice->userAudioFrame.frameBuf2)
	{
		if(!Audio_Packet)
			Audio_Packet = pUvcDevice->userAudioFrame.frameBuf1;

		pUvcDevice->userAudioFrame.curFrameBuf = Audio_Packet;
		
		if(pUvcDevice->userAudioFrame.curFrameBuf)
		{
			//pUvcDevice->userAudioFrame.curFrameBuf += pIsochIrp->adwActualLength[wFrameIndex-1];
			//pUvcDevice->userAudioFrame.curFrameBuf += PACKET_LEN*wFrameIndex;
				
			//printk("index: %d\n", wFrameIndex);
			/* Fill the Iso IRP */
			pIsochIrp->hPipe = pUvcDevice->isoAudioPipe;
			pIsochIrp->pBuffer = pUvcDevice->userAudioFrame.curFrameBuf;
			pIsochIrp->wFrameCount = UVC_MAX_ISO_PACKETS;
			pIsochIrp->wCurrentFrame = 0;
			pIsochIrp->bIsrCallback = TRUE;
			//pIsochIrp->bAllowDma = TRUE;
			pIsochIrp->pfIrpComplete = MGC_AudioReceiveCallback;
			pIsochIrp->pCompleteParam = (void *)pUvcDevice;

			while(dwStatus)
			{
				dwStatus = MUSB_ScheduleIsochTransfer(0, pIsochIrp);
			}
		}
	}
}


/** For ARK3299 **/
/** Callback function*/
//static int uvc_times = 0;
//static int uvc_count = 0;
static unsigned char cover_data[20];	//后一个payload buffer数据会覆盖掉前一个buffer的尾部(大小为帧头)，提前要把帧为保存，以便后续拷贝复原。
static uint8_t bfh_fid;		//新图片帧翻转标记
static void MGC_VideoReceiveCallback(void* pContext, MUSB_IsochIrp *pIsochIrp)
{
	MGC_UvcDevice		*pUvcDevice = (MGC_UvcDevice  *)pContext;
	uint16_t			wFrameIndex;
	uint32_t			ulen = 0;
	uint8_t				header;
	uint8_t				bfh;
	video_frame_s *curr_frame = pUvcDevice->userVideoFrame.frame;
//	queue_s *video_ready_fifo = pUvcDevice->userVideoFrame.ready_fifo;
//	queue_s *video_free_fifo = pUvcDevice->userVideoFrame.free_fifo;	
	
	wFrameIndex = pIsochIrp->wCurrentFrame;   

	//UVC_DBG("### MGC_VideoReceiveCallback\n");
	if(pIsochIrp->adwStatus[wFrameIndex-1] == MUSB_STATUS_OK)
	{
		ulen = pIsochIrp->adwActualLength[wFrameIndex-1];
		header = curr_frame->cur[0];
		bfh = curr_frame->cur[1];
		//UVC_DBG("## bfh:%x\n",bfh);
		
		if(header != 0x0c || (bfh & 0xf0) != 0x80 || ulen <= header)
		//if(header != 0x0c || (bfh & 0xfc) != 0x8c || ulen <= header)
		{
			//UVC_DBG("MGC_VideoReceiveCallback:  no header or no data \n");
			goto uvc_rcv_done;
		}
		
		
		if((bfh&UVC_STREAM_FID) != bfh_fid)	//新帧
		{				
			bfh_fid = (bfh & UVC_STREAM_FID); //帧翻转时保存翻转后状态
			if(curr_frame->buf != curr_frame->cur)
				memcpy(curr_frame->buf,curr_frame->cur,ulen);
			//UVC_DBG("## FID\n");
			curr_frame->len = 0;
			curr_frame->id = 1;
			curr_frame->cur = curr_frame->buf;
		}
		else if(curr_frame->id == 0)	//没头，则返回
		{
			goto uvc_rcv_done;
		}
		else	//有头且是头后面的数据包非头
		{
			memcpy(curr_frame->cur, cover_data, header);
		}
		
		if(bfh & UVC_STREAM_EOF)
		{
			curr_frame->len =  (unsigned int)(curr_frame->cur - curr_frame->buf + ulen - header);
			//UVC_DBG("## EOF, LEN:%x\n",curr_frame->len);
			
			queue_s *video_temp_fifo = pUvcDevice->userVideoFrame.temp_fifo;
			if(video_temp_fifo)
				queue_insert ((queue_s *)curr_frame, video_temp_fifo);			
			curr_frame->cur = NULL;
			
			if((curr_frame->len > header) && (pUvcDevice->user_video_event))
			{
				pUvcDevice->userVideoData.usertype = UVC_VIDEO_FRAME_EVENT;
				pUvcDevice->user_video_event(&(pUvcDevice->userVideoData));
//				if(uvc_count++ >= 30)
//				{
//					printf("time:%d\n",XM_GetTickCount() - uvc_times);
//					uvc_times = XM_GetTickCount();
//					uvc_count = 0;
//				}
			}
		}
		else   
		{
			curr_frame->cur += ulen - header;
			memcpy(cover_data, curr_frame->cur, header);
		}
	}
uvc_rcv_done:
	pUvcDevice->userVideoData.usertype = UVC_VIDEO_NEW_CAPTURE_EVENT;
	pUvcDevice->user_video_event(&(pUvcDevice->userVideoData));
}

/** Callback function*/
static void MGC_AudioReceiveCallback(void* pContext, MUSB_IsochIrp *pIsochIrp)
{
	MGC_UvcDevice		*pUvcDevice = (MGC_UvcDevice  *)pContext;
	uint16_t				wFrameIndex;
	uint8_t				*udata;
	uint32_t				ulen;
	
	wFrameIndex = pIsochIrp->wCurrentFrame;

	if(pIsochIrp->adwStatus[wFrameIndex-1] == MUSB_STATUS_OK)
	{
		udata = pIsochIrp->pBuffer;
		ulen = pIsochIrp->adwActualLength[wFrameIndex-1];
	
		pUvcDevice->userAudioData.usertype = UVC_AUDIO_FRAME_EVENT;
		pUvcDevice->userAudioData.userdata = Audio_Packet;
		pUvcDevice->userAudioData.userdatalen = ulen;

		if((ulen) && (pUvcDevice->user_audio_event))
			pUvcDevice->user_audio_event(&(pUvcDevice->userAudioData));

		if(pUvcDevice->userAudioFrame.frameBuf1 == Audio_Packet)
			Audio_Packet = pUvcDevice->userAudioFrame.frameBuf2;
		else if(pUvcDevice->userAudioFrame.frameBuf2 == Audio_Packet)
			Audio_Packet = pUvcDevice->userAudioFrame.frameBuf1;
	}

	MGC_UvcStartAudioReceive(pContext);
}

uint32_t MUSB_UVC_Control(uint16_t Command, void *Arg)
{
	uint32_t dwStatus = 0xFFFFFFFF;
	//OS_ERR myerr;
	INT8U myerr;
	CPU_TS ts;
	MGC_UvcDevice *pUvcDevice =  MGC_GetUvcDeviceContext();
	MGC_UVC_PROBE_PARAM *pProbeParam = (MGC_UVC_PROBE_PARAM *)Arg;
	MGC_UVC_FRAME *pUvcFrame = (MGC_UVC_FRAME *)Arg;
	//OS_ERR err;
	
	//防止USB 热插拔时被锁住
	if(Command == REQUEST_UVC_RECEIVE_FINISH)
	{
		unsigned int timeout = 0xf;
		while((cmd_count > 0) && (timeout--))
		{
			OSSemPost(UvcDeviceSem);
			cmd_count--;
		}
		receive_finish = 1;
		return UVC_OK;
	}
	
	if(!pUvcDevice)
	{
		MUSB_PRINTK("UvcDevice is not Present Command:%x\n",Command);
		dwStatus = UVC_NO_DEVICE;
		return dwStatus;
	}
	

	if((!pProbeParam) && (Command < REQUEST_OPEN_UVC_VIDEO))
	{
		MUSB_PRINTK("Arg is NULL \n");
		return dwStatus;
	}

	
	if(cmd_count)
		//OSSemPend(UvcDeviceSem, 0, OS_OPT_PEND_BLOCKING, &ts, &myerr);
		OSSemPend(UvcDeviceSem, 0, &myerr);

	cmd_count++;
	switch(Command)
	{
		case REGISTER_USER_VIDEO_FUNC:
		{
			pUvcDevice->user_video_event = (UserVideoAudioEvent) Arg;
			dwStatus = 0;
			break;
		}
		case REGISTER_USER_AUDIO_FUNC:
		{
			pUvcDevice->user_audio_event = (UserVideoAudioEvent)Arg;
			dwStatus = 0;
			break;
		}
		case REGISTER_USER_VIDEO_ARG:
		{
			pUvcDevice->userVideoFrame.maxframelen = pUvcFrame->maxframelen;
//			pUvcDevice->userVideoFrame.frameBuf1 = pUvcFrame->frameBuf1;
//			pUvcDevice->userVideoFrame.frameBuf2 = pUvcFrame->frameBuf2;
			if(pUvcFrame->ready_fifo)
				pUvcDevice->userVideoFrame.ready_fifo = pUvcFrame->ready_fifo;
			if(pUvcFrame->free_fifo)
				pUvcDevice->userVideoFrame.free_fifo = pUvcFrame->free_fifo;
			if(pUvcFrame->temp_fifo)
				pUvcDevice->userVideoFrame.temp_fifo = pUvcFrame->temp_fifo;
			if(pUvcFrame->UserVideoAudioPrivateCallback)
				pUvcDevice->userVideoFrame.UserVideoAudioPrivateCallback = pUvcFrame->UserVideoAudioPrivateCallback;
			dwStatus = 0;
			break;
		}
		case REGISTER_USER_AUDIO_ARG:
		{
			pUvcDevice->userAudioFrame.maxframelen = pUvcFrame->maxframelen;
			pUvcDevice->userAudioFrame.frameBuf1 = pUvcFrame->frameBuf1;
			pUvcDevice->userAudioFrame.frameBuf2 = pUvcFrame->frameBuf2;
			dwStatus = 0;
			break;
		}
		case REQUEST_GETPROBE_CONTROL:
		{
			MUSB_MemSet(&ProbeParam, 0, sizeof(MGC_UVC_PROBE_PARAM));
			dwStatus = MGC_UvcCmdGetProbeControl(pUvcDevice);
			
			OSTimeDly(50);
			MUSB_MemCopy(pProbeParam, &ProbeParam, sizeof(MGC_UVC_PROBE_PARAM));
			//OSTimeDly(50, OS_OPT_TIME_DLY, &err);
			OSTimeDly(50);
			break;
		}
		case REQUEST_SETPROBE_CONTROL:
		{
			dwStatus = MGC_UvcCmdSetProbeControl(pUvcDevice, (uint8_t *)pProbeParam);
			//OSTimeDly(50, OS_OPT_TIME_DLY, &err);
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
			//OSTimeDly(50, OS_OPT_TIME_DLY, &err);
			OSTimeDly(50);
			break;
		}
		case REQUEST_OPEN_UVC_VIDEO:
		{
			receive_finish = 1;
			dwStatus = MGC_UvcCmdOpenVideo(pUvcDevice);
			MUSB_PRINTK("MUSB_UVC_Control Command is 0x%x Status is 0x%x \n", Command, dwStatus);
			//OSTimeDly(50, OS_OPT_TIME_DLY, &err);
			OSTimeDly(50);
			break;
		}
		case REQUEST_STOP_UVC_VIDEO:
		{
			dwStatus = MGC_UvcCmdStopVideo(pUvcDevice);	
			MUSB_PRINTK("MUSB_UVC_Control Command is 0x%x Status is 0x%x \n", Command, dwStatus);
			//OSTimeDly(50, OS_OPT_TIME_DLY, &err);
			//OSTimeDly(50);
			break;
		}
		case REQUEST_OPEN_UVC_AUDIO:
		{
			receive_finish = 1;
			dwStatus = MGC_UvcCmdOpenAudio(pUvcDevice);
			MUSB_PRINTK("MUSB_UVC_Control Command is 0x%x Status is 0x%x \n", Command, dwStatus);
			//OSTimeDly(50, OS_OPT_TIME_DLY, &err);
			OSTimeDly(50);
			break;
		}
		case REQUEST_STOP_UVC_AUDIO:
		{
			dwStatus = MGC_UvcCmdStopAudio(pUvcDevice);	
			MUSB_PRINTK("MUSB_UVC_Control Command is 0x%x Status is 0x%x \n", Command, dwStatus);
			//OSTimeDly(50, OS_OPT_TIME_DLY, &err);
			OSTimeDly(50);
			break;
		}
		case REQUEST_UVC_VIDEO_RECEIVE:
		{
			MGC_UvcStartVideoReceive(pUvcDevice);
			dwStatus = 0;
			//OSTimeDly(50, OS_OPT_TIME_DLY, &err);
			//OSTimeDly(50);
			break;
		}
		case REQUEST_UVC_AUDIO_RECEIVE:
		{
			MGC_UvcStartAudioReceive(pUvcDevice);	
			dwStatus = 0;
			//OSTimeDly(50, OS_OPT_TIME_DLY, &err);
			OSTimeDly(50);
			break;
		}
		case REQUEST_UVC_VIDEO_DISCONNECT:
		{
			MGC_UvcCleanVideoIrp(pUvcDevice);
			dwStatus = 0;
			break;
		}
		default:
			MUSB_PRINTK("This Command (%d) is not Support \n",Command);
			break;
	}
	//MUSB_PRINTK("MUSB_UVC_Control Command is 0x%x Status is 0x%x \n", Command, dwStatus);
	//OSSemPost(UvcDeviceSem, OS_OPT_POST_1, &myerr);
	OSSemPost(UvcDeviceSem);
	cmd_count--;
	return dwStatus;
}
