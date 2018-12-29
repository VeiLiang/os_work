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

#define	CAMERA_FRAME_TTL		(3+1)		// 最大重发次数
#define	CAMERA_FRAME_COUNT	2



static OS_TASK TCB_CameraTask;
__no_init static OS_STACKPTR int StackCameraTask[XMSYS_CAMERA_TASK_STACK_SIZE/4];          /* Task stacks */

// PC Camera帧缓冲区
#pragma data_alignment=64
static volatile int CameraFreeIndex;	// 读写索引



// USB Frame管理数据

static OS_RSEMA CameraSema;					// Camera帧互斥信号量

static OS_CSEMA CameraDataFrameCount;		// Camera数据帧数量
static OS_CSEMA CameraFreeFrameCount;		// Camera数据帧数量

static XMCAMERAFRAME	CameraDataFrame[CAMERA_FRAME_COUNT];	
static queue_s CameraFrameFree;				// Camera数据帧链表首
static queue_s CameraFrameData;				// Camera空闲帧链表首

static int CameraReady = 0;

// 获取下一个空闲帧用于帧记录
int * XMSYS_CameraGetFreeFrame (void)
{
	return NULL;
}

// 释放空闲帧
void XMSYS_CameraPutFreeFrame (int *FreeFrame)
{
	
}

// 通知一个新的数据帧已产生
int XMSYS_CameraNewDataFrame (int *lpCameraFrame, int cbCameraFrame, void *lpPrivateData, XMSYS_CAMERA_CALLBACK fp_terminate,
		XMSYS_CAMERA_RETRANSFER fpRetransfer)
{
	XMCAMERAFRAME *frame;
		
	if(lpCameraFrame == NULL || cbCameraFrame == 0)
	{
		return XMSYS_CAMERA_ERROR_OTHER;
	}
	// 等待空闲帧
	OS_WaitCSema (&CameraFreeFrameCount);
	
	// 互斥保护
	SEMA_LOCK (&CameraSema);
	
	// 从空闲帧池中获取一个空闲帧
	frame = (XMCAMERAFRAME *)queue_delete_next (&CameraFrameFree);
	
	frame->frame_base = (unsigned char *)lpCameraFrame;
	frame->frame_size = cbCameraFrame;
	frame->frame_private = lpPrivateData;
	frame->fp_terminate = fp_terminate;
	frame->fp_retransfer = fpRetransfer;
	frame->ttl = CAMERA_FRAME_TTL;
	
	// 插入到待传帧链表
	queue_insert ((queue_s *)frame, &CameraFrameData);
	
	SEMA_UNLOCK (&CameraSema);
	
	OS_SignalCSema (&CameraDataFrameCount);
		
	OS_SignalEvent(XMSYS_CAMERA_PACKET_READY, &TCB_CameraTask);
	
	return XMSYS_CAMERA_SUCCESS;
}


// 获取一个Camera数据帧
XMCAMERAFRAME *XMSYS_CameraGetDataFrame (void)
{
	XMCAMERAFRAME *DataFrame;
	//XM_printf ("CameraFreeIndex=%d\n", CameraFreeIndex);
	// 等待数据帧
	OS_WaitCSema (&CameraDataFrameCount);
	// 互斥保护
	SEMA_LOCK (&CameraSema);
	DataFrame = (XMCAMERAFRAME *)queue_delete_next (&CameraFrameData);
	SEMA_UNLOCK (&CameraSema);
	if(DataFrame->ttl != CAMERA_FRAME_TTL)
	{
		// 调用帧发送线程的重发处理函数，以便完成必须的某些操作，如设置重发标志等
		if(DataFrame->fp_retransfer)
			(*DataFrame->fp_retransfer)(DataFrame->frame_private);
	}
	return DataFrame;
}

// 释放一个Camera数据帧
void XMSYS_CameraFreeDataFrame (XMCAMERAFRAME *lpDataFrame, int ErrCode)
{
	void *lpPrivateData;
	XMSYS_CAMERA_CALLBACK fpCallback;
	
	assert (lpDataFrame);

	if(ErrCode == XMSYS_CAMERA_ERROR_USB_UVCSTOP && lpDataFrame->ttl == CAMERA_FRAME_TTL)
	{
		lpDataFrame->ttl --;
	}
	
	// 检查发送是否失败。若是，安排重发
	// 帧需要重发多次，避免重发帧丢失。在Vista下，重发一次丢失的几率非常高
	if(lpDataFrame->ttl != CAMERA_FRAME_TTL)	//ErrCode == XMSYS_CAMERA_ERROR_USB_UVCSTOP)
	{
		// UVC传输异常，执行重发
		if(lpDataFrame->ttl > 0)
		{			
			lpDataFrame->ttl --;
			// XM_printf ("Camera Frame %x require to re-transfer\n", lpDataFrame);
			// 插入到数据帧队列的首部，等待重发
			// 互斥保护
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
	
	// 互斥保护
	SEMA_LOCK (&CameraSema);
	// 插入到空闲链表
	queue_insert ((queue_s *)lpDataFrame, &CameraFrameFree);
	SEMA_UNLOCK (&CameraSema);
	
	// 累加空闲帧计数
	OS_SignalCSema (&CameraFreeFrameCount);
	
	// 通知USB帧的创建者，帧已传输完毕
	(*fpCallback) (lpPrivateData, ErrCode);
}

// 由USB中断程序调用, 通知Camera任务Camera帧已传输完毕
void XMSYS_CameraNotifyDataFrameTransferDone (void)
{
	// XM_printf ("trans end notify\n");
	OS_SignalEvent(XMSYS_CAMERA_PACKET_TRANSFER_DONE, &TCB_CameraTask); /* 通知事件 */
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
			// 协议终止
			XM_printf ("XMSYS_CAMERA_UVC_STOP\n");
			// UVC链路断开			
			if(Frame)
			{
				XMSYS_CameraFreeDataFrame (Frame, XMSYS_CAMERA_ERROR_USB_UVCSTOP);
				Frame = NULL;
			}
			
			// 释放排队队列的资源
			while (OS_GetCSemaValue(&CameraDataFrameCount) != 0)
			{
				Frame = XMSYS_CameraGetDataFrame ();
				XMSYS_CameraFreeDataFrame (Frame, XMSYS_CAMERA_ERROR_USB_DISCONNECT);
				Frame = NULL;
			}
			// 释放UVC资源
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
			// 异常终止
			XM_printf ("XMSYS_CAMERA_UVC_BREAK\n");
			// UVC链路断开			
			if(Frame)
			{
				XMSYS_CameraFreeDataFrame (Frame, XMSYS_CAMERA_ERROR_USB_UVCSTOP);
				Frame = NULL;
			}
			
			// 释放排队队列的资源
			while (OS_GetCSemaValue(&CameraDataFrameCount) != 0)
			{
				Frame = XMSYS_CameraGetDataFrame ();
				XMSYS_CameraFreeDataFrame (Frame, XMSYS_CAMERA_ERROR_USB_DISCONNECT);
				Frame = NULL;
			}
			// 释放UVC资源
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
			// 20170816 修复UVC START时, 上次UVC传输未能收到UVC STOP消息, 导致系统挂起的Bug
			if(Frame)
			{
				XM_printf ("Stop UVC before XMSYS_CAMERA_UVC_START\n");
				goto uvc_stop;
			}
			
			// UVC链路开启
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
			// 检查是否有USB帧正在传输
			if(Frame)
			{
				// Camera包尚未传输完毕
			}
			else
			{
			transfer_frame:				
				// Camera包已传输完毕。USB传输已停止
				// 获取新的Camera帧并进行传输
				//XM_printf ("New Camera Frame Ready\n");
				if (OS_GetCSemaValue(&CameraDataFrameCount) != 0)
				{
					Frame = XMSYS_CameraGetDataFrame ();
					
					// 检查UVC是否开启
					if(VideoSample == NULL)
					{
						// UVC未开启
						// 释放数据帧
						XMSYS_CameraFreeDataFrame (Frame, XMSYS_CAMERA_ERROR_USB_DISCONNECT);
						Frame = NULL;
					}
					else
					{
						// UVC已开启
						// 通过UVC传输
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
			// Camera USB帧传输完毕 (USB中断触发)
		
			// 释放其资源, 并通知USB包的产生者
			if(Frame)
			{
				//XM_printf ("XMSYS_CameraFreeDataFrame\n");
				XMSYS_CameraFreeDataFrame (Frame, XMSYS_CAMERA_SUCCESS);
				//XM_printf ("XMSYS_CameraFreeDataFrame end\n");
				Frame = NULL;
			}
			
			// 获取新的Camera帧
			//XM_printf ("OS_GetCSemaValue\n");
			if (OS_GetCSemaValue(&CameraDataFrameCount) != 0)
			{
				//XM_printf ("XMSYS_CameraGetDataFrame\n");
				Frame = XMSYS_CameraGetDataFrame ();
				// 检查UVC是否开启
				if(VideoSample == NULL)
				{
					//XM_printf ("XMSYS_CameraFreeDataFrame\n");
					// UVC未开启
					// 释放数据帧
					XMSYS_CameraFreeDataFrame (Frame, XMSYS_CAMERA_ERROR_USB_UVCSTOP);
					Frame = NULL;
				}
				else
				{
					//XM_printf ("UVCVideoSetupPayload\n");
					// UVC已开启
					// 通过UVC传输
#ifdef UVC_PIO
					UVCVideoSetupPayload ((BYTE *)Frame->frame_base, Frame->frame_size);
#else
					UVC_VideoSetupVideoSample ((BYTE *)Frame->frame_base, Frame->frame_size);
#endif
				}
			}
			
			//XM_printf ("XMSYS_CAMERA_PACKET_TRANSFER_DONE end\n");
		}
		
		
		// UVC Packet传输
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


