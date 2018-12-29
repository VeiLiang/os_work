/*
 * uvcapp.c
 *
 *  Created on: 2017-6-1
 *      Author: Administrator
 */
#include "printk.h"
#include "mu_uctrl.h"
#include "mu_uapi.h"
#include "xm_queue.h"
#include "rtos.h"
#include "xm_core.h"
#include "stdbool.h"
#include <assert.h>

typedef enum{
	UVC_TEST_UDF = 0,
	UVC_TEST_CLOSE,
	UVC_TEST_OPEN,
	UVC_TEST_STOP,
	UVC_TEST_READY,
}UVC_TEST_T;

typedef struct _USER_VIDEO_FRAME_
{
  unsigned char last_fid;        //�ж�֡�����ı�־
  unsigned char lastpackindex;   //�ϴΰ�������
  unsigned char curpackindex;    //��ǰ������
  unsigned int  lastframelen;     //�ϴ�֡�ĳ���
  unsigned int  curframelen;     //��ǰ����֡�ĳ���


  unsigned int *displayFrame;    //�Դ��ַ
  unsigned int *backFrame;       //��̨�ڴ��ַ

  unsigned int *frameBuf1;       // ������pingbang BUFFER 1
  unsigned int *frameBuf2;       // ������pingbang BUFFER 2

  unsigned int maxpacket;        //���İ�
  unsigned int packetheader;     //ͷ����
  unsigned int lastpacket;       //һ��֡���İ�
  unsigned int packetcnt;        //һ��֡�İ���

  unsigned int curFrameFlag;     //�Ƿ���ȷ֡
  unsigned int errorFrame;       //����֡
  unsigned int totalFrame;       //֡������
}USER_VIDEO_FRAME;

typedef struct _xm_isp_scalar_buffer_s {
	void		*prev;
	void		*next;
	unsigned int	id;
	void		*user_private_data;
	void		(*data_ready_user_callback) (void *user_private_data);		// ������׼����
	void		(*frame_free_user_callback) (void *user_private_data);		// ֡�ͷ�
	// ������Ϣ
	u32_t		window_y;
	u32_t		window_uv;
	
} xm_isp_scalar_buffer_s;

#define MUSB_MemSet(_pDest, _iData, _iSize) memset((void*)_pDest, _iData, _iSize)

//usb ����ͷ�ֱ��ʴ�С
unsigned int  Usb_Output_Width = 1280;
unsigned int  Usb_Output_Height = 720;

#define JPEG_FIFO_Frame_Count	4
#define XMSYS_UVC_TEST_TASK_STACK_SIZE 0x2000

//MUSB_UVC_Decode event
#define XMSYS_UVC_DECODE_START_EVENT		0x01
#define XMSYS_UVC_DECODE_STOP_EVENT			0x02

//MUSB_UVC_Caputre  event	0x40 && 0x80 is used 
#define XMSYS_UVC_TEST_FRAME_CAPTURE_EVENT		0x01
#define XMSYS_UVC_TASK_START_EVENT				0x02
#define XMSYS_UVC_TASK_STOP_EVENT				0x04
#define XMSYS_UVC_TASK_RESET_EVENT				0x08
#define XMSYS_UVC_BACKLIGHT_CHECK_EVENT			0x10

//MUSB_UVC_Task event
#define XMSYS_UVC_BACKLIGHT_CALLBACK_EVENT		0x01

#if 1
#define UVC_DBG		printf
#else
#define UVC_DBG
#endif


static USER_VIDEO_FRAME	uservideoframe;
#pragma data_alignment=32
static MGC_UVC_PROBE_PARAM	myprobe;

//task
static UVC_TEST_T uvc_status = UVC_TEST_UDF;
#pragma data_alignment=32
static OS_STACKPTR int StackUvcCapture[XMSYS_UVC_TEST_TASK_STACK_SIZE/4];		  /* Task stacks */
#pragma data_alignment=32
static OS_STACKPTR int StackUvcDecode[XMSYS_UVC_TEST_TASK_STACK_SIZE/4];		  /* Task stacks */
#pragma data_alignment=32
OS_TASK TCB_UVC_CAPTURE;					/* Task-control-blocks */
#pragma data_alignment=32
static OS_TASK TCB_UVC_DECODE;					/* Task-control-blocks */
#pragma data_alignment=32
static OS_TASK TCB_UVC_TASK;
#pragma data_alignment=32
static OS_STACKPTR int StackUvcTask[XMSYS_UVC_TEST_TASK_STACK_SIZE/4];		  /* Task stacks */
//��ʱ��
#pragma data_alignment=32
static OS_TIMER UvcTaskTimer;


#pragma data_alignment=32
static queue_s jpeg_ready_fifo;			//jpeg��������ͷ
#pragma data_alignment=32
static queue_s jpeg_free_fifo;			//����������ͷ
#pragma data_alignment=32
static queue_s jpeg_temp_fifo;			//��������ͷ(��jpeg_free_fifoɾ���ڵ����ڽ������ݣ���δ����jpeg_ready_fifo)

#pragma data_alignment=32
static video_frame_s jpeg_fifo[JPEG_FIFO_Frame_Count];	//jpeg fifo
#pragma data_alignment=32
static unsigned char jpeg_fifo_buf[JPEG_FIFO_Frame_Count][1280 * 720];	//���1k ����
#pragma data_alignment=32
OS_RSEMA usb_video_jpeg_semaphore;		// �����ź�������

//����jpeg ������yuv422����
//#pragma data_alignment=1024
//static unsigned char yuv422_buf[1280*720*2+1024];//422 buffer cache

//#pragma data_alignment=32
//static char Usb_out_buffer[JPEG_FIFO_Frame_Count][640*480*3/2];

//uvc �쳣��λ�꿪��
#define UVC_ABNORMAL_RESET	
#ifdef UVC_ABNORMAL_RESET
//uvc ��ȡ����ʧ�ܴ�������ʱ����������Ҫ��λUVC Camera
static unsigned int uvc_capture_failed_times = 0;
//��ʱ���ص�����
static unsigned int timer_cb_times = 0;
#endif

//��ʱ����ʱ���(��λ: ms)
#define TIMER_TIMEOUT		300	

int xm_uvc_jpeg_decode (const char *jpeg_data,	size_t jpeg_size, 
								//unsigned char *yuv_422, 		// Y_UV422δ���������ַ(YUY2��ʽ)
								unsigned char *yuv_420			// Y_UV420δ���������ַ(NV12��ʽ)
								//unsigned char *yuv_scale		// Y_UV420���ź������ַ(NV12��ʽ)
							  );
//extern char *itu656_in_get_yuv_buffer (unsigned int frame_id);	
extern char *uvc_get_yuv_buffer (unsigned int *fid);
extern void uvc_frame_notify (unsigned int finished_y_addr);
extern char *xm_uvc_get_yuv_buffer (unsigned int id);

extern void XMSYS_SensorNotifyDataFrame (int channel, int frame_id);

//�����жϺ��ֹͣ�������(ͨ����ʱ�������������)����OS_Use()ͬʱʹ��ʱXM_lock()����OS_Use() ��OS_Unuse()�м䡣
extern void XM_lock (void);
extern void XM_unlock (void);
extern void XMSYS_UvcCaptureStart (void);
extern void XMSYS_UvcCaptureStop (void);
//extern int XM_UsbFrameBuferInit (void);

//extern int ITU656_in;
extern unsigned char lg_Usb_Present;
extern unsigned char lg_Usb_Bubble;

//4��usb ����USB
#define USB_Data_FIFO_SIZE ((1280*720*3/2 + 2048)*5)
#pragma data_alignment=32
__no_init char USB_FIFO_buffer[USB_Data_FIFO_SIZE];


static void UvcTaskTicketCallback (void)
{
	if((uvc_status == UVC_TEST_READY) && (lg_Usb_Present == 1))
	{
#ifdef UVC_ABNORMAL_RESET
		//case 1: ��ȡyuv420 buf Ƶ��(��Ъ��)ʧ�ܣ�����֡�ʽ���(���ܳ��ֿ���) ,��Ҫ����
		timer_cb_times ++;
		//ÿ���Ӽ��һ�λ�ȡyuv420(����h264����+ lcd ��ʾ)ʧ�ܴ������ﵽ��ֵ������uvc camera
		if(timer_cb_times >= 60000/TIMER_TIMEOUT)	// 1min ���һ��
		{
			timer_cb_times = 0;
			
			if(uvc_capture_failed_times > 60)
			{
				uvc_capture_failed_times = 0;
				OS_SignalEvent(XMSYS_UVC_TASK_RESET_EVENT, &TCB_UVC_CAPTURE);
				goto uvc_timer_done;
			}
			uvc_capture_failed_times = 0;
		}
#endif

	}
	
uvc_timer_done:
	OS_RetriggerTimer (&UvcTaskTimer);
}
#if 0
static void MUSB_UVC_Task(void)
{
	unsigned char event;
	
	OS_CREATETIMER (&UvcTaskTimer, UvcTaskTicketCallback, TIMER_TIMEOUT);

	while(1)
	{
		event = OS_WaitSingleEvent(XMSYS_UVC_BACKLIGHT_CALLBACK_EVENT
											//| XMSYS_UVC_BACKLIGHT_CHECK_EVENT
											);
		if(event & XMSYS_UVC_BACKLIGHT_CALLBACK_EVENT)	
		{
			
		}
	}

}
#endif

u32_t Usb_get_video_Width (void)
{
	return Usb_Output_Width;
}

u32_t Usb_get_video_height (void)
{
	return Usb_Output_Height;
}

u32_t Usb_get_video_image_size (void)
{
	return Usb_Output_Width * Usb_Output_Height* 3 / 2;
}

char *Usb_get_yuv_buffer (unsigned int frame_id)
{
	u32_t image_size = Usb_Output_Width * Usb_Output_Height * 3/2 + 2048;	
	if(frame_id < JPEG_FIFO_Frame_Count)
		return (char *)(((unsigned int)(USB_FIFO_buffer+frame_id*image_size)+1023)& ~1023);
	else
		return NULL;
}


/* ��ʼ��jpeg ��������*/
static void jpeg_fifo_init(void)
{
	video_frame_s *buffer;
	int i;
	
	OS_Use(&usb_video_jpeg_semaphore);
	buffer = jpeg_fifo;
	
	queue_initialize (&jpeg_ready_fifo);	//�������jpeg֡���ݽڵ�(���ж���ʹ��)
	queue_initialize (&jpeg_free_fifo);		//��Ž�����ͷŵĽڵ㣬���´�ʹ��(���ж���ʹ��)
	
	XM_lock ();
	queue_initialize (&jpeg_temp_fifo);		//��Ÿս�����uvc ���ݵĽڵ㣬��δ����jpeg_ready_fifo (���ж��в���)
	XM_unlock ();
	
	memset (jpeg_fifo_buf, 0, sizeof(jpeg_fifo_buf));
	for(i=0; i<JPEG_FIFO_Frame_Count; i++)
	{
		buffer->buf = jpeg_fifo_buf[i];
		buffer->cur = NULL;
		buffer->id = 0;
		buffer->len = 0;
		queue_insert ((queue_s *)buffer, &jpeg_free_fifo);
		buffer++;
	}
	OS_Unuse(&usb_video_jpeg_semaphore);
}


/* Receive ���ݵ� �жϻص�����*/
static uint32_t MUSB_UVC_Event(void *eventData)
{
	MGC_UVC_USER_DATA *Event = (MGC_UVC_USER_DATA *)eventData;

	switch(Event->usertype)
	{
		case UVC_VIDEO_FRAME_EVENT:	//������һ��֡ͼƬ����
			OS_SignalEvent(XMSYS_UVC_DECODE_START_EVENT, &TCB_UVC_DECODE);
		break;
		case UVC_VIDEO_NEW_CAPTURE_EVENT:	//������һ�λ�ȡ΢֡���ݲ���
			OS_SignalEvent(XMSYS_UVC_TEST_FRAME_CAPTURE_EVENT, &TCB_UVC_CAPTURE);
			break;
		default:
			break;
	}
	return 0;
}

//���жϻص�������ʹ���ź���(Ҳ�����ж��е��ã���Ӧcase �ﲻ�����ź�������)
void *uvc_private_callback(void *arg)
{
	MGC_UVC_USER_DATA *cb = (MGC_UVC_USER_DATA *)arg;
	video_frame_s *curr_frame;
	void * ret = NULL;

	switch(cb->usertype)
	{
		case UVC_VIDEO_GET_FREE_BUF_EVENT:	//��ȡ��������Ŀսڵ㣬�洢uvc ����
		{
			OS_Use(&usb_video_jpeg_semaphore);
			if(!queue_empty(&jpeg_free_fifo))
			{
				curr_frame = (video_frame_s *)queue_next(&jpeg_free_fifo);
				queue_delete((queue_s *)curr_frame);
				OS_Unuse(&usb_video_jpeg_semaphore);
				ret = (void *)curr_frame;
			}
			else
			{
				OS_Unuse(&usb_video_jpeg_semaphore);
				//jpeg�����H264 ����Ч�� �ͣ��ᵼ��FIFO �����á�
				UVC_DBG("### no fifo\n");
				OSTimeDly(30);
				OS_SignalEvent(XMSYS_UVC_TEST_FRAME_CAPTURE_EVENT, &TCB_UVC_CAPTURE);
				//ret = NULL;
			}
			break;
		}
		case UVC_VIDEO_RECAPTURE_EVENT:	//��ȡuvc ����ʧ�ܣ����·�������
		{
			uvc_capture_failed_times++;
			if((uvc_status == UVC_TEST_READY) && (lg_Usb_Present == 1))	//����������·���
			{
				OS_SignalEvent(XMSYS_UVC_TEST_FRAME_CAPTURE_EVENT, &TCB_UVC_CAPTURE);
			}
			break;
		}
		case UVC_VIDEO_RESET_EVENT:
		{
			if((uvc_status == UVC_TEST_READY) && (lg_Usb_Present == 1))	//�����������UVC
				OS_SignalEvent(XMSYS_UVC_TASK_RESET_EVENT, &TCB_UVC_CAPTURE);
			break;
		}
		default:
			break;
	}

	return ret;
}

/********************************************************************************
 * ��������                                 ConfigGpsDevice
 * �������ܣ�
 * ������    id �豸ID
 * ����ֵ����
 */
static void ConfigUvcDevice (void)
{
	MGC_UVC_FRAME uvcVideoFrame = {0};

	printk ("ConfigUvcDevice\n");
	MUSB_UVC_Control(REQUEST_UVC_RECEIVE_FINISH, NULL);

	MUSB_UVC_Control(REGISTER_USER_VIDEO_FUNC, (void*)MUSB_UVC_Event);
	uvcVideoFrame.maxframelen = ISO_FRAME_SIZE;	//usb video δʹ��
//	uvcVideoFrame.frameBuf1 = UvcDataFrame[0];	//usb video δʹ��
//	uvcVideoFrame.frameBuf2 = UvcDataFrame[1];	//usb video δʹ��
	uvcVideoFrame.ready_fifo = &jpeg_ready_fifo;	
	uvcVideoFrame.free_fifo = &jpeg_free_fifo;
	uvcVideoFrame.temp_fifo = &jpeg_temp_fifo;
	uvcVideoFrame.UserVideoAudioPrivateCallback = uvc_private_callback;
	MUSB_UVC_Control(REGISTER_USER_VIDEO_ARG, (void*)&uvcVideoFrame);
	
	MUSB_UVC_Control(REQUEST_GETPROBE_CONTROL, (void *)&myprobe);
#if 0	//׿��
	myprobe.bFormatIndex = 1;	//mjpeg
	myprobe.bFrameIndex = 0x04;//640x480	4;// 1280*720		//�ֱ���ѡ��
	myprobe.dwMaxVideoFrameSize = 0x001C2000;
	myprobe.dwFrameInterval = 0x00061A80;
	myprobe.wCompQuality = 8000;
#elif 0 //˫�ָ�Demo 1280X720 
    myprobe.bFormatIndex = 1;	//1:mpgj;	2:yuy2	 [Depend On Format Type Descriptor ]
	myprobe.bFrameIndex = 1;//0x02:1600X1200 0x05:1280X720
	myprobe.dwMaxVideoFrameSize = 0x001C224D;//1600X1200:0x003A9800
	myprobe.dwFrameInterval = 0x00051615;//0x000A2C2A;//30fps
	myprobe.wCompQuality = 12000;
	//myprobe.dwMaxPayloadTransferSize = 0x13fc;
#elif 0 //����Դ 1280X720
    myprobe.bFormatIndex = 1;	//mjpeg
	myprobe.bFrameIndex = 0x04;//640x480	4;// 1280*720		//�ֱ���ѡ��
	myprobe.dwMaxVideoFrameSize = 0x001C2000;
	myprobe.dwFrameInterval = 0x000A2C2A;
	myprobe.wCompQuality = 10000;
#elif 0 //�챦�� 1920X1080 
	myprobe.bFormatIndex = 1;	//1:mpgj;	2:yuy2	 [Depend On Format Type Descriptor ]
	myprobe.bFrameIndex = 1;//0x02:1600X1200 0x05:1280X720
	myprobe.dwMaxVideoFrameSize = 0x003F4800;//1600X1200:0x003A9800
	myprobe.dwFrameInterval = 0x00051615;//0x000A2C2A;
	myprobe.wCompQuality = 10000;
	//myprobe.dwMaxPayloadTransferSize = 0x13fc;
#elif 1
	myprobe.bFormatIndex = 1;	//mjpeg
	myprobe.bFrameIndex = 1;//640x480	4;// 1280*720		//�ֱ���ѡ��
	myprobe.dwMaxVideoFrameSize = 0x001C224D;
	myprobe.dwFrameInterval = 0x00051615;
	myprobe.wCompQuality = 8000;
#endif
	MUSB_UVC_Control(REQUEST_SETPROBE_CONTROL, (void *)&myprobe);

	MUSB_UVC_Control(REQUEST_GETPROBE_CONTROL, (void *)&myprobe);
	MUSB_UVC_Control(REQUEST_SETPROBE_CONTROL, (void *)&myprobe);
	
	MUSB_UVC_Control(REQUEST_GETPROBE_CONTROL, (void *)&myprobe);
	MUSB_UVC_Control(REQUEST_SETCOMMIT_CONTROL, (void *)&myprobe);

	MUSB_UVC_Control(REQUEST_OPEN_UVC_VIDEO, NULL); 
	
	uvc_status = UVC_TEST_READY;
	OS_SignalEvent(XMSYS_UVC_TEST_FRAME_CAPTURE_EVENT, &TCB_UVC_CAPTURE);
}

#define	USB_THREAD_Decode_IDLE		0
#define	USB_THREAD_Decode_RUN		1
static u32_t USB_THREAD_Decode_state;

#define UVC_FPS_MINITOR		// UVC ��Ƶ֡�ʼ��
//#define UVC_JPEG_BITRATE_MONITOR	// UVC �������
static void MUSB_UVC_Decode(void)
{
	int ret;
	unsigned char event = 0;
	video_frame_s *current_frame;
	char *yuv420_buf = NULL;				//�����λ���ŵ�YUV��ַ�����ڱ���洢
	char *yuv420_display = NULL;
	unsigned int uvc_frame_id = 0;			//yuv����buf���±�
	unsigned int uvc_yuv_scaler_id = 0;		//yuv���ź�����buf���±�

#ifdef UVC_FPS_MINITOR
	unsigned int scalar_count = 0;
	unsigned int scalar_timeout;	// 
#endif

	USB_THREAD_Decode_state = USB_THREAD_Decode_RUN;
	while(1)
	{
		event = OS_WaitSingleEvent(XMSYS_UVC_DECODE_START_EVENT|XMSYS_UVC_DECODE_STOP_EVENT);
		if(event & XMSYS_UVC_DECODE_START_EVENT)
		{
			//���պõ�jpeg ���ݲ鵽jpeg_ready_fifo ��
			while(1)
			{
				XM_lock ();
				if(queue_empty(&jpeg_temp_fifo))
				{
					XM_unlock ();
					break;
				}
				current_frame = (video_frame_s *)queue_next(&jpeg_temp_fifo);				
				queue_delete((queue_s *)current_frame);
				XM_unlock ();
				
				OS_Use(&usb_video_jpeg_semaphore);
				queue_insert((queue_s *)current_frame, &jpeg_ready_fifo);
				OS_Unuse(&usb_video_jpeg_semaphore);
			}
			
			//jpeg FIFO ��Ϊ�գ���ʼ����
			while(1)
			{
				OS_Use(&usb_video_jpeg_semaphore);
				if(queue_empty(&jpeg_ready_fifo))
				{
					OS_Unuse(&usb_video_jpeg_semaphore);
					break;
				}
				current_frame = (video_frame_s *)queue_next(&jpeg_ready_fifo);
				queue_delete((queue_s *)current_frame);
				OS_Unuse(&usb_video_jpeg_semaphore);
								 
				if(uvc_frame_id > 3)
					uvc_frame_id = 0;
				//yuv420_buf = (char *)isp_get_yuv_buffer(uvc_frame_id);
				yuv420_buf = (char *)Usb_get_yuv_buffer(uvc_frame_id);
								
				if(uvc_yuv_scaler_id > 3)
					uvc_yuv_scaler_id = 0;
				//yuv420_display = xm_uvc_get_yuv_buffer(uvc_yuv_scaler_id++);
				
#ifdef UVC_JPEG_BITRATE_MONITOR
				static unsigned int uvc_stream_monitor_total_size, uvc_stream_monitor_frame_count, uvc_stream_monitor_timeout;
				
				uvc_stream_monitor_total_size += current_frame->len;
				if(uvc_stream_monitor_frame_count == 0)
				{
					uvc_stream_monitor_timeout = XM_GetTickCount() + 5000;
				}
				uvc_stream_monitor_frame_count ++;
				if(uvc_stream_monitor_timeout <= XM_GetTickCount())
				{
					XM_printf ("UVC Stream, frames per second(x10) = %d, bytes per frame = %d bytes, bytes per second = %d bytes\n", 
								  uvc_stream_monitor_frame_count * 10 / 5,
								  uvc_stream_monitor_total_size / (uvc_stream_monitor_frame_count), 
								  uvc_stream_monitor_total_size / 3);
					uvc_stream_monitor_frame_count = 0;
					uvc_stream_monitor_total_size = 0;
				}
#endif
				
				{
					//jpeg����+ yuv��ʽת��+ ����
					//ret = xm_uvc_jpeg_decode((const char *)current_frame->buf+0x0c, current_frame->len, yuv422_buf, (unsigned char *)yuv420_buf, (unsigned char *)yuv420_display);
					//ret = xm_uvc_jpeg_decode((const char *)current_frame->buf+0x0c, current_frame->len, yuv422_buf, (unsigned char *)yuv420_buf);/*, (unsigned char *)yuv420_display);*/
					ret = xm_uvc_jpeg_decode((const char *)current_frame->buf+0x0c, current_frame->len, (unsigned char *)yuv420_buf);/*, (unsigned char *)yuv420_display*/
					//printf("tm:%d\n",XM_GetTickCount() - tm);
                                       // printf("111111111111111 yuv420_buf: %x \n",yuv420_buf);
					if(ret == 0)
					{
						//H264 ����
						XM_lock ();		//�����жϣ�������жϳ���ͬʱ����
						XMSYS_SensorNotifyDataFrame(2, uvc_frame_id++);
						XM_unlock ();
						
						//uvc_frame_notify ((unsigned int)yuv420_display);

#ifdef UVC_FPS_MINITOR		// UVC ��Ƶ֡�ʼ��
						// ͳ��ʵ�ʵ�֡��
						if(scalar_count == 0)
						{
							scalar_timeout = XM_GetTickCount() + 30000;
							scalar_count ++;
						}
						else
						{
							scalar_count ++;
						}
						if(scalar_timeout <= XM_GetTickCount())
						{
							UVC_DBG ("UVC video fps = %d\n", scalar_count / 30);
							scalar_count = 0;			  
						}
#endif
					}
				}
				OS_Use(&usb_video_jpeg_semaphore);
				queue_insert((queue_s *)current_frame, &jpeg_free_fifo);
				OS_Unuse(&usb_video_jpeg_semaphore);
			}
		}

		if(event & XMSYS_UVC_DECODE_STOP_EVENT)
		{
			break;
		}
	}
	
	USB_THREAD_Decode_state = USB_THREAD_Decode_IDLE;
	OS_ClearEvents(&TCB_UVC_DECODE);
	OS_Terminate (&TCB_UVC_DECODE);
}
extern void SensorInitCaptureFIFO (void);

void Usb_Decode_Capture_Start(void)
{
	if(USB_THREAD_Decode_state == USB_THREAD_Decode_RUN)
	{
		printf("Decode capture start is run \n");
		return;
	}
	assert (USB_THREAD_Decode_state == USB_THREAD_Decode_IDLE);
	//create thread
	OS_CREATETASK(&TCB_UVC_DECODE, "MUSB_UVC_Decode", MUSB_UVC_Decode, 252, StackUvcDecode);
	//wait usb Decode start
	while(USB_THREAD_Decode_state == USB_THREAD_Decode_IDLE)
	{

		OS_Delay(1);
	}
}

extern BOOL SensorData_CH2_Status;
extern int XMSYS_SensorCaptureReset (int channel);
void Usb_Decode_Capture_Stop(void)
{
	if(USB_THREAD_Decode_state == USB_THREAD_Decode_IDLE)
	{
		printf("Decode capture start is stop \n");
		return;
	}

	//close thread
	OS_SignalEvent(XMSYS_UVC_DECODE_STOP_EVENT, &TCB_UVC_DECODE);
	//wait usb Decode stop
	while(USB_THREAD_Decode_state == USB_THREAD_Decode_RUN)
	{

		OS_Delay(1);
	}
    OS_Delay(10);
	XMSYS_SensorCaptureReset(2);//��ն�Ӧ��buffer
	OS_Delay(10);
	XMSYS_SensorResetCurrentPacket(2); //�ͷ���ʾ�����е�����
}

extern void SensorInitCaptureFIFO_Three (void);
/* �ȴ�����receive �źţ��Դ�����ͷ��ȡ����*/
static void MUSB_UVC_Caputre(void)
{	
	unsigned char event;
	//�ȴ��źŽ�������
	while(1)
	{
		event = OS_WaitSingleEvent(XMSYS_UVC_TEST_FRAME_CAPTURE_EVENT 
										| XMSYS_UVC_BACKLIGHT_CHECK_EVENT
										| XMSYS_UVC_TASK_START_EVENT
										| XMSYS_UVC_TASK_STOP_EVENT
										| XMSYS_UVC_TASK_RESET_EVENT);

		XM_printf(">>>>>>>>>>>>>>>>>>MUSB_UVC_Caputre, event:%d\r\n", event);
		
		if(event & XMSYS_UVC_TEST_FRAME_CAPTURE_EVENT)
		{
			if((uvc_status == UVC_TEST_READY) && (lg_Usb_Present == 1))	//(work && insert)
				MUSB_UVC_Control(REQUEST_UVC_VIDEO_RECEIVE, NULL);	//receive ����
		}
#ifdef UVC_ABNORMAL_RESET
		if(event & XMSYS_UVC_TASK_RESET_EVENT)
		{			
			uvc_status = UVC_TEST_CLOSE;
			MUSB_UVC_Control(REQUEST_UVC_VIDEO_DISCONNECT, NULL);
			OS_Delay(50);
			event = XMSYS_UVC_TASK_START_EVENT;
		}
#endif
		if(event & XMSYS_UVC_TASK_STOP_EVENT)	//�γ�uvc 
		{
			//close Decode Thread
			Usb_Decode_Capture_Stop();
		}
		if(event & XMSYS_UVC_TASK_START_EVENT)	//����uvc 
		{
			//start Decode Thread
			jpeg_fifo_init();
			//reset sensor buffer
			OS_Delay(10);
			SensorInitCaptureFIFO_Three();
			OS_Delay(10);
			Usb_Decode_Capture_Start();
			uvc_status = UVC_TEST_OPEN;
			//uvc ��������
			ConfigUvcDevice();
		}
	}
}


/* USB����ͷ�����uvc task start */
int USB_Show_Video = 0;
int  usb_uvc_task_start(void * pdata)
{
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>###usb_uvc_task_start....................\n");
	SensorData_CH2_Status = FALSE;
	uvc_status = UVC_TEST_OPEN;
    
    USB_Show_Video = 1;
	OS_SignalEvent(XMSYS_UVC_TASK_START_EVENT, &TCB_UVC_CAPTURE);
	
	return 0;
}

/* USB����ͷ�γ���uvc task stop */
int usb_uvc_task_stop(void *pdata)
{
	printf("### usb_uvc_task_stop\n");
    SensorData_CH2_Status = TRUE;
    uvc_status = UVC_TEST_CLOSE;
    
	MUSB_UVC_Control(REQUEST_UVC_VIDEO_DISCONNECT, NULL);
	OS_SignalEvent(XMSYS_UVC_TASK_STOP_EVENT, &TCB_UVC_CAPTURE);
	return 0;
}
int usb_uvc_task_stop_PlayBack(void *pdata)
{
    uvc_status = UVC_TEST_CLOSE;
	MUSB_UVC_Control(REQUEST_UVC_VIDEO_DISCONNECT, NULL);
	OS_SignalEvent(XMSYS_UVC_TASK_STOP_EVENT, &TCB_UVC_CAPTURE);
	return 0;
}
void MUSB_User_App_Register_Connect_Ind(int (*USER_Connect_Event)(void *pPrivateData));
void MUSB_User_App_Register_Discon_Ind(int (*USER_Discon_Event)(void *pPrivateData));


//��������ע��Ӧ�ó���Ļص�����: usb video �豸���ӺͶϿ��ɹ��������Ӧ�Ļص�
void uvc_init(void)
{
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>uvc_init...................\r\n");
	USB_THREAD_Decode_state = USB_THREAD_Decode_IDLE;
	MUSB_User_App_Register_Connect_Ind(usb_uvc_task_start);
	MUSB_User_App_Register_Discon_Ind(usb_uvc_task_stop);
    
	OS_CREATERSEMA (&usb_video_jpeg_semaphore); /* Creates resource semaphore */
	//OS_CREATETASK(&TCB_UVC_DECODE, "MUSB_UVC_Decode", MUSB_UVC_Decode, 252, StackUvcDecode);
	OS_CREATETASK(&TCB_UVC_CAPTURE, "MUSB_UVC_Caputre", MUSB_UVC_Caputre, 253, StackUvcCapture);	//��ȡ���ݵ����ȼ����
	//OS_CREATETASK(&TCB_UVC_TASK, "MUSB_UVC_Task", MUSB_UVC_Task, 100, StackUvcTask);
}

