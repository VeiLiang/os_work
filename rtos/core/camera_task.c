#include <stdio.h>
#include <stdlib.h>
#include "hardware.h"

#include "xm_core.h"
#include <xm_type.h>
#include <xm_dev.h>
#include <assert.h>
#include <xm_uvc_video.h>
#include <xm_queue.h>
#include <rtos.h>
#include <fs.h>

#define	_SENSOR_DEBUG_

#define	CAMERA_FRAME_TTL		(3+1)		// ����ط�����
#define	CAMERA_FRAME_COUNT	2



static OS_TASK TCB_CameraTask;
__no_init static OS_STACKPTR int StackCameraTask[XMSYS_CAMERA_TASK_STACK_SIZE/4];          /* Task stacks */

// PC Camera֡������
#pragma data_alignment=64
static volatile int CameraFreeIndex;	// ��д����



// USB Frame��������

static OS_RSEMA CameraSema;					// Camera֡�����ź���

static OS_CSEMA CameraDataFrameCount;		// Camera����֡����
static OS_CSEMA CameraFreeFrameCount;		// Camera����֡����

static XMCAMERAFRAME	CameraDataFrame[CAMERA_FRAME_COUNT];	
static queue_s CameraFrameFree;				// Camera����֡������
static queue_s CameraFrameData;				// Camera����֡������

static int CameraReady = 0;

// ��ȡ��һ������֡����֡��¼
int * XMSYS_CameraGetFreeFrame (void)
{
	return NULL;
}

// �ͷſ���֡
void XMSYS_CameraPutFreeFrame (int *FreeFrame)
{
	
}

// ֪ͨһ���µ�����֡�Ѳ���
int XMSYS_CameraNewDataFrame (int *lpCameraFrame, int cbCameraFrame, void *lpPrivateData, XMSYS_CAMERA_CALLBACK fp_terminate,
		XMSYS_CAMERA_RETRANSFER fpRetransfer)
{
	XMCAMERAFRAME *frame;
		
	if(lpCameraFrame == NULL || cbCameraFrame == 0)
	{
		return XMSYS_CAMERA_ERROR_OTHER;
	}
	// �ȴ�����֡
	OS_WaitCSema (&CameraFreeFrameCount);
	
	// ���Ᵽ��
	SEMA_LOCK (&CameraSema);
	
	// �ӿ���֡���л�ȡһ������֡
	frame = (XMCAMERAFRAME *)queue_delete_next (&CameraFrameFree);
	
	frame->frame_base = (unsigned char *)lpCameraFrame;
	frame->frame_size = cbCameraFrame;
	frame->frame_private = lpPrivateData;
	frame->fp_terminate = fp_terminate;
	frame->fp_retransfer = fpRetransfer;
	frame->ttl = CAMERA_FRAME_TTL;
	
	// ���뵽����֡����
	queue_insert ((queue_s *)frame, &CameraFrameData);
	
	SEMA_UNLOCK (&CameraSema);
	
	OS_SignalCSema (&CameraDataFrameCount);
		
	OS_SignalEvent(XMSYS_CAMERA_PACKET_READY, &TCB_CameraTask);
	
	return XMSYS_CAMERA_SUCCESS;
}


// ��ȡһ��Camera����֡
XMCAMERAFRAME *XMSYS_CameraGetDataFrame (void)
{
	XMCAMERAFRAME *DataFrame;
	//XM_printf ("CameraFreeIndex=%d\n", CameraFreeIndex);
	// �ȴ�����֡
	OS_WaitCSema (&CameraDataFrameCount);
	// ���Ᵽ��
	SEMA_LOCK (&CameraSema);
	DataFrame = (XMCAMERAFRAME *)queue_delete_next (&CameraFrameData);
	SEMA_UNLOCK (&CameraSema);
	if(DataFrame->ttl != CAMERA_FRAME_TTL)
	{
		// ����֡�����̵߳��ط����������Ա���ɱ����ĳЩ�������������ط���־��
		if(DataFrame->fp_retransfer)
			(*DataFrame->fp_retransfer)(DataFrame->frame_private);
	}
	return DataFrame;
}

// �ͷ�һ��Camera����֡
void XMSYS_CameraFreeDataFrame (XMCAMERAFRAME *lpDataFrame, int ErrCode)
{
	void *lpPrivateData;
	XMSYS_CAMERA_CALLBACK fpCallback;
	
	assert (lpDataFrame);

	if(ErrCode == XMSYS_CAMERA_ERROR_USB_UVCSTOP && lpDataFrame->ttl == CAMERA_FRAME_TTL)
	{
		lpDataFrame->ttl --;
	}
	
	// ��鷢���Ƿ�ʧ�ܡ����ǣ������ط�
	// ֡��Ҫ�ط���Σ������ط�֡��ʧ����Vista�£��ط�һ�ζ�ʧ�ļ��ʷǳ���
	if(lpDataFrame->ttl != CAMERA_FRAME_TTL)	//ErrCode == XMSYS_CAMERA_ERROR_USB_UVCSTOP)
	{
		// UVC�����쳣��ִ���ط�
		if(lpDataFrame->ttl > 0)
		{			
			lpDataFrame->ttl --;
			// XM_printf ("Camera Frame %x require to re-transfer\n", lpDataFrame);
			// ���뵽����֡���е��ײ����ȴ��ط�
			// ���Ᵽ��
			SEMA_LOCK (&CameraSema);
			if(queue_empty(&CameraFrameData))
			{
				queue_insert ((queue_s *)lpDataFrame, &CameraFrameData);
			}
			else
			{
				queue_insert ((queue_s *)lpDataFrame, queue_next(&CameraFrameData));
			}
			
			assert (queue_next(&CameraFrameData) == (queue_s *)lpDataFrame);
			
			SEMA_UNLOCK (&CameraSema);

			OS_SignalCSema (&CameraDataFrameCount);
		
			return;
		}
		
		// XM_printf ("Camera Frame %x re-transfer end\n", lpDataFrame);
	}
	
	lpPrivateData = lpDataFrame->frame_private;
	fpCallback = lpDataFrame->fp_terminate;
	
	lpDataFrame->frame_base = NULL;
	lpDataFrame->frame_size = 0;
	lpDataFrame->fp_terminate = NULL;
	lpDataFrame->frame_private = NULL;
	
	// ���Ᵽ��
	SEMA_LOCK (&CameraSema);
	// ���뵽��������
	queue_insert ((queue_s *)lpDataFrame, &CameraFrameFree);
	SEMA_UNLOCK (&CameraSema);
	
	// �ۼӿ���֡����
	OS_SignalCSema (&CameraFreeFrameCount);
	
	// ֪ͨUSB֡�Ĵ����ߣ�֡�Ѵ������
	(*fpCallback) (lpPrivateData, ErrCode);
}

// ��USB�жϳ������, ֪ͨCamera����Camera֡�Ѵ������
void XMSYS_CameraNotifyDataFrameTransferDone (void)
{
	// XM_printf ("trans end notify\n");
	OS_SignalEvent(XMSYS_CAMERA_PACKET_TRANSFER_DONE, &TCB_CameraTask); /* ֪ͨ�¼� */
}

void XMSYS_CameraNotifyUsbEvent (void)
{
	OS_SignalEvent(XMSYS_CAMERA_USB_EVENT, &TCB_CameraTask);
}

void XMSYS_CameraNotifyCdrEvent (void)
{
	OS_SignalEvent (XMSYS_CAMERA_CDR, &TCB_CameraTask);
}

void XMSYS_CameraPostEvent (unsigned char Event)
{
	OS_SignalEvent(Event, &TCB_CameraTask);
}

int XMSYS_CameraIsReady (void)
{
	return CameraReady;
}

extern int is_usb_suspend (void);


void XMSYS_CameraTask (void)
{
	XMCAMERAFRAME *Frame = NULL;
	struct UVCVIDEOSAMPLE *VideoSample = NULL;

	OS_U8 camera_event;
		
	// XM_printf ("XMSYS_CameraTask\n");
			
	
	while(1)
	{	
		camera_event = OS_WaitEvent(	XMSYS_CAMERA_USB_EVENT
											 |	XMSYS_CAMERA_PACKET_READY
											 |	XMSYS_CAMERA_PACKET_TRANSFER_DONE
											 | XMSYS_CAMERA_UVC_START
											 | XMSYS_CAMERA_UVC_STOP	
											 | XMSYS_CAMERA_UVC_TRANSFER	
											 | XMSYS_CAMERA_CDR
											 | XMSYS_CAMERA_UVC_BREAK
											 );
		
		if(camera_event & XMSYS_CAMERA_USB_EVENT)
		{
			//XM_printf ("pccamera_process\n");
#if _XMSYS_USB_USAGE_ == _XMSYS_USB_USAGE_DEVICE_		
			pccamera_process ();
#endif
		}
		
		
		if(camera_event & XMSYS_CAMERA_UVC_STOP)
		{
		uvc_stop:
			// Э����ֹ
			XM_printf ("XMSYS_CAMERA_UVC_STOP\n");
			// UVC��·�Ͽ�			
			if(Frame)
			{
				XMSYS_CameraFreeDataFrame (Frame, XMSYS_CAMERA_ERROR_USB_UVCSTOP);
				Frame = NULL;
			}
			
			// �ͷ��ŶӶ��е���Դ
			while (OS_GetCSemaValue(&CameraDataFrameCount) != 0)
			{
				Frame = XMSYS_CameraGetDataFrame ();
				XMSYS_CameraFreeDataFrame (Frame, XMSYS_CAMERA_ERROR_USB_DISCONNECT);
				Frame = NULL;
			}
			// �ͷ�UVC��Դ
			if(VideoSample)
			{
				// stop_or_break
				//	1 --> STOP
				// 2 --> BREAK
				UVCVideoDelete (VideoSample, 1);
				VideoSample = NULL;
			}
			
			//XMSYS_SensorCaptureStop ();
			CameraReady = 0;
			// XM_printf ("XMSYS_CAMERA_UVC_STOP END\n");
			XMSYS_UsbVideoClose ();
		}
		
		if(camera_event & XMSYS_CAMERA_UVC_BREAK)
		{
			// �쳣��ֹ
			XM_printf ("XMSYS_CAMERA_UVC_BREAK\n");
			// UVC��·�Ͽ�			
			if(Frame)
			{
				XMSYS_CameraFreeDataFrame (Frame, XMSYS_CAMERA_ERROR_USB_UVCSTOP);
				Frame = NULL;
			}
			
			// �ͷ��ŶӶ��е���Դ
			while (OS_GetCSemaValue(&CameraDataFrameCount) != 0)
			{
				Frame = XMSYS_CameraGetDataFrame ();
				XMSYS_CameraFreeDataFrame (Frame, XMSYS_CAMERA_ERROR_USB_DISCONNECT);
				Frame = NULL;
			}
			// �ͷ�UVC��Դ
			if(VideoSample)
			{
				// stop_or_break
				// 1 --> STOP
				// 2 --> BREAK
				UVCVideoDelete (VideoSample, 2);
				VideoSample = NULL;
			}
			
			//XMSYS_SensorCaptureStop ();
			CameraReady = 0;
			// XM_printf ("XMSYS_CAMERA_UVC_STOP END\n");
			XMSYS_UsbVideoClose ();
		}
		
		
		if(camera_event & XMSYS_CAMERA_UVC_START)
		{
			// 20170816 �޸�UVC STARTʱ, �ϴ�UVC����δ���յ�UVC STOP��Ϣ, ����ϵͳ�����Bug
			if(Frame)
			{
				XM_printf ("Stop UVC before XMSYS_CAMERA_UVC_START\n");
				goto uvc_stop;
			}
			
			// UVC��·����
			if(is_usb_suspend())
				goto check_packet_ready;
			
			XM_printf ("XMSYS_CAMERA_UVC_START\n");
			//XMSYS_SensorCaptureStart ();
			if(VideoSample == NULL)
				VideoSample = UVCVideoCreate ();
			CameraReady = 1;
			
			XMSYS_UsbVideoOpen ();
			if(Frame == NULL)
			{
				goto transfer_frame;
			}
			// XM_printf ("XMSYS_CAMERA_UVC_START END\n");
		}

check_packet_ready:
		if(camera_event & XMSYS_CAMERA_PACKET_READY)
		{
			//XM_printf ("XMSYS_CAMERA_PACKET_READY %x\n", Frame);	
			// ����Ƿ���USB֡���ڴ���
			if(Frame)
			{
				// Camera����δ�������
			}
			else
			{
			transfer_frame:				
				// Camera���Ѵ�����ϡ�USB������ֹͣ
				// ��ȡ�µ�Camera֡�����д���
				//XM_printf ("New Camera Frame Ready\n");
				if (OS_GetCSemaValue(&CameraDataFrameCount) != 0)
				{
					Frame = XMSYS_CameraGetDataFrame ();
					
					// ���UVC�Ƿ���
					if(VideoSample == NULL)
					{
						// UVCδ����
						// �ͷ�����֡
						XMSYS_CameraFreeDataFrame (Frame, XMSYS_CAMERA_ERROR_USB_DISCONNECT);
						Frame = NULL;
					}
					else
					{
						// UVC�ѿ���
						// ͨ��UVC����
						//XM_printf ("UVCVideoSetupPayload\n");
						//dma_inv_range ((unsigned int)Frame->frame_base, Frame->frame_size + (unsigned int)Frame->frame_base);
#ifdef UVC_PIO
						UVCVideoSetupPayload ((BYTE *)Frame->frame_base, Frame->frame_size);	
#else
						UVC_VideoSetupVideoSample ((BYTE *)Frame->frame_base, Frame->frame_size);	
#endif
					}
				}
			}
		}
		
		if(camera_event & XMSYS_CAMERA_PACKET_TRANSFER_DONE)
		{
			// Camera USB֡������� (USB�жϴ���)
		
			// �ͷ�����Դ, ��֪ͨUSB���Ĳ�����
			if(Frame)
			{
				//XM_printf ("XMSYS_CameraFreeDataFrame\n");
				XMSYS_CameraFreeDataFrame (Frame, XMSYS_CAMERA_SUCCESS);
				//XM_printf ("XMSYS_CameraFreeDataFrame end\n");
				Frame = NULL;
			}
			
			// ��ȡ�µ�Camera֡
			//XM_printf ("OS_GetCSemaValue\n");
			if (OS_GetCSemaValue(&CameraDataFrameCount) != 0)
			{
				//XM_printf ("XMSYS_CameraGetDataFrame\n");
				Frame = XMSYS_CameraGetDataFrame ();
				// ���UVC�Ƿ���
				if(VideoSample == NULL)
				{
					//XM_printf ("XMSYS_CameraFreeDataFrame\n");
					// UVCδ����
					// �ͷ�����֡
					XMSYS_CameraFreeDataFrame (Frame, XMSYS_CAMERA_ERROR_USB_UVCSTOP);
					Frame = NULL;
				}
				else
				{
					//XM_printf ("UVCVideoSetupPayload\n");
					// UVC�ѿ���
					// ͨ��UVC����
#ifdef UVC_PIO
					UVCVideoSetupPayload ((BYTE *)Frame->frame_base, Frame->frame_size);
#else
					UVC_VideoSetupVideoSample ((BYTE *)Frame->frame_base, Frame->frame_size);
#endif
				}
			}
			
			//XM_printf ("XMSYS_CAMERA_PACKET_TRANSFER_DONE end\n");
		}
		
		
		// UVC Packet����
		if(camera_event & XMSYS_CAMERA_UVC_TRANSFER)
		{
			UVCVideoTransfer ();
		}
		
		/*
		if(camera_event & XMSYS_CAMERA_CDR)
		{
			void XMSYS_CdrPrivateProtocolProcess (void);
			
			XMSYS_CdrPrivateProtocolProcess ();
		}
		*/
	}
}



void XMSYS_CameraInit (void)
{
	int i;
		
	queue_initialize (&CameraFrameData);
	OS_CreateCSema (&CameraDataFrameCount, 0);

	queue_initialize (&CameraFrameFree);
	for (i = 0; i < CAMERA_FRAME_COUNT; i++)
	{
		queue_insert ((queue_s *)&CameraDataFrame[i], &CameraFrameFree);
	}
	OS_CreateCSema (&CameraFreeFrameCount, CAMERA_FRAME_COUNT);
	
	SEMA_CREATE (&CameraSema);
	
#if _XMSYS_USB_USAGE_ == _XMSYS_USB_USAGE_DEVICE_		
	pccamera_init ();
#endif
	
	OS_CREATETASK(&TCB_CameraTask, "CameraTask", XMSYS_CameraTask, XMSYS_CAMERA_TASK_PRIORITY, StackCameraTask);
}

void XMSYS_CameraExit (void)
{
	
}


