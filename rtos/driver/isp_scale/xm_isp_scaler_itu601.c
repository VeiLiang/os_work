
#include <stdio.h>
#include "hardware.h"
#include "RTOS.h"		// OS头文件
#include "FS.h"
#include <xm_type.h>
#include "xm_core.h"
#include <xm_dev.h>
#include <xm_queue.h>
#include <assert.h>
#include <string.h>
#include <xm_printf.h>
#include <xm_core_linear_memory.h>
#include "arkn141_codec.h"
#include "xm_isp_scalar.h"
#include "xm_file.h"
#include "arkn141_isp.h"
#include "rxchip.h"

#define	ITU656_IN_IDLE							0
#define	ITU656_IN_RUN							1

#define	ITU601_ISP_SCALAR_FRAME_COUNT			4
#define	ITU601_ISP_SCALAR_EVENT_TICKET			0x01		
#define	ITU601_ISP_SCALAR_EVENT_INIT		 	0x02		
#define	ITU601_ISP_SCALAR_EVENT_STOP		 	0x04		


extern int match_frame_id_in_isp_scaler_fifo (unsigned int y_addr);
static OS_TASK TCB_Itu601ScalerTask;
__no_init static OS_STACKPTR int StackItu601ScalerTask[XMSYS_ITU601SCALER_TASK_STACK_SIZE/4]; 

// 与ISP scalar模式相关的变量
static int itu601_isp_scalar_object;			// ISP scalar对象标识号
static queue_s itu601_isp_scalar_data;		// 数据已准备好的ISP scalar帧队列
static XMSYSSENSORPACKET itu601_isp_scalar_fifo[ITU601_ISP_SCALAR_FRAME_COUNT];	

#ifdef EN_RN6752M1080N_CHIP
static u32_t itu601_isp_scaler_in_width = 960;
static u32_t itu601_isp_scaler_in_height = 1080;
#else
static u32_t itu601_isp_scaler_in_width = 1280;
static u32_t itu601_isp_scaler_in_height = 720;
#endif

static int itu601_scalar_state;
static volatile unsigned int itu601_scalar_stop = 0;

OS_TASK get_isp_itu601_task(void)
{
	return TCB_Itu601ScalerTask;
}

static void itu601_isp_scalar_frame_free_user_callback (void *private_data)
{
	XM_lock ();
	queue_insert ((queue_s *)private_data, &itu601_isp_scalar_data);
	XM_unlock ();
}
static void itu601_isp_scalar_data_ready_user_callback (void *private_data)
{
	XM_lock ();
	XMSYSSENSORPACKET *sensorPacket = (XMSYSSENSORPACKET *)private_data;
	unsigned int frame_id = match_frame_id_in_isp_scaler_fifo((u32_t)sensorPacket->data[0]);
	XMSYS_SensorNotifyDataFrame (0, frame_id);
	XM_unlock ();
}

static void itu601_isp_scalar_mode_exit (void)
{
	int i;
	if(itu601_isp_scalar_object >= 0)
	{
		xm_isp_scalar_delete (itu601_isp_scalar_object);
		itu601_isp_scalar_object = -1;
	}
	for (i = 0; i < ITU601_ISP_SCALAR_FRAME_COUNT; i ++)
	{
		if(itu601_isp_scalar_fifo[i].buffer)
		{
			itu601_isp_scalar_fifo[i].buffer = NULL;
		}
	}	
}


static int itu601_isp_scalar_mode_init (void)
{
	int ret = -1;
	int i;
		
	// 根据itu601 输入尺寸创建一个scalar对象
	assert (itu601_isp_scalar_object == -1);
	do
	{
		queue_initialize (&itu601_isp_scalar_data);
        //itu601_isp_scaler_in_height = camera_get_video_height();
        //itu601_isp_scaler_in_width = camera_get_video_width();
		ret = xm_isp_scalar_create_ex (
												 XM_ISP_SCALAR_SRC_CHANNEL_ITU601,
												 XM_ISP_SCALAR_SRC_SYNC_PLOARITY_HIGH_LEVEL,
												 #ifdef EN_RN6752M1080N_CHIP
												 XM_ISP_SCALAR_SRC_SYNC_PLOARITY_LOW_LEVEL,
												 #else
												 XM_ISP_SCALAR_SRC_SYNC_PLOARITY_HIGH_LEVEL,
												 #endif
												 XM_ISP_SCALAR_FORMAT_YUV422,
												 XM_ISP_SCALAR_SRC_YCBCR_UYVY,
												 itu601_isp_scaler_in_width,
												 itu601_isp_scaler_in_height,
												 itu601_isp_scaler_in_width,
												 0,	
												 0,	
												 itu601_isp_scaler_in_width,	
												 itu601_isp_scaler_in_height,
												 itu601_isp_scaler_in_width,
												 itu601_isp_scaler_in_height,
												 itu601_isp_scaler_in_width,
												 itu601_isp_scaler_in_height-100,
												 0,
												 0
												);
		if(ret < 0)
		{
			XM_printf ("itu601_isp_scalar_mode_init failed, can't create scalar object\n");
			return -1;
		}
		itu601_isp_scalar_object = ret;
		
		// 分配帧内存
		for (i = 0; i < ITU601_ISP_SCALAR_FRAME_COUNT; i ++)
		{
			u32_t image_size = itu601_isp_scaler_in_width * itu601_isp_scaler_in_height * 3/2 + 2048;	
			
			itu601_isp_scalar_fifo[i].width = itu601_isp_scaler_in_width;
			itu601_isp_scalar_fifo[i].height = itu601_isp_scaler_in_height;
			
			itu601_isp_scalar_fifo[i].buffer = isp_scalar_get_frame_buffer (i, image_size);
			if(itu601_isp_scalar_fifo[i].buffer == NULL)
			{
				printf ("itu601_isp_scalar_mode_init failed, allocate memory NG\n");
				goto err_exit;
			}
			itu601_isp_scalar_fifo[i].data[0] = (char *)(((unsigned int)(itu601_isp_scalar_fifo[i].buffer) + 1023) & ~1023);
			itu601_isp_scalar_fifo[i].data[1] = itu601_isp_scalar_fifo[i].data[0] + itu601_isp_scaler_in_width * itu601_isp_scaler_in_height;
			
			dma_inv_range ((unsigned int)(itu601_isp_scalar_fifo[i].data[0]), (unsigned int)(itu601_isp_scalar_fifo[i].data[0]) + itu601_isp_scaler_in_width * itu601_isp_scaler_in_height * 3/2);
		}
		
		// 将分配的帧缓存注册到ISP scalar对象
		for (i = 0; i < ITU601_ISP_SCALAR_FRAME_COUNT; i ++)
			xm_isp_scalar_register_user_buffer (itu601_isp_scalar_object,
																	(u32_t)itu601_isp_scalar_fifo[i].data[0], (u32_t)itu601_isp_scalar_fifo[i].data[1],
																	itu601_isp_scalar_data_ready_user_callback, 
																	itu601_isp_scalar_frame_free_user_callback, 
																	&itu601_isp_scalar_fifo[i]
																);		
	} while(0);
	
	XM_printf("itu601_isp_scalar_mode_init\n");
	
	return 0;
	
err_exit:
	itu601_isp_scalar_mode_exit ();
	return -1;
}


void Notification_Itu601_Isp_Scaler_Init(void)
{
    OS_SignalEvent(ITU601_ISP_SCALAR_EVENT_INIT, &TCB_Itu601ScalerTask); /* 通知事件 */
}

void Notification_Itu601_Isp_Scaler_Stop(void)
{
    OS_SignalEvent(ITU601_ISP_SCALAR_EVENT_STOP, &TCB_Itu601ScalerTask); /* 通知事件 */
}

void ISP_itu601In_SelPad (void)
{
	unsigned int val;
	XM_lock ();
	val = rSYS_PAD_CTRL00;
	val &= ~((0x7<<0)|(0x7<<3)|(0x7<<6)|(0x7<<21)|(0x7<<24)|(0x7<<27));
	//     ituclk| vsync | hsync | din0  | din1
	val |=(1<<0)|(1<<3)|(1<<6)|(1<<21)|(1<<24)|(1<<27);
	rSYS_PAD_CTRL00 = val;
   
	val = rSYS_PAD_CTRL01;
	val &= ~((0x7<<0)|(0x7<<3)|(0x7<<6)|(0x7<<9)|(0x7<<12));
	//     din2 | din3 | din4 | din5 | din6  | din7
	val |=(1<<0)|(1<<3)|(1<<6)|(1<<9)|(1<<12);
	rSYS_PAD_CTRL01 = val;
 	XM_unlock ();
}


#define	ISP_SCALAR_RUN				rISP_SCALE_EN = 0x03
#define	ISP_SCALAR_STOP				rISP_SCALE_EN = 0x00

extern BOOL get_rxchip_ready_state(void);
extern void SensorInitCaptureFIFO_One (void);
extern int XMSYS_SensorCaptureReset (int channel);
BOOL Itu601_Data_Status = FALSE;


void XMSYS_Itu601Scaler_Task (void)
{
    OS_U8 itu601scaler_event;
	
	itu601_scalar_state = ITU656_IN_RUN;
    while(get_rxchip_ready_state() != TRUE) {
        OS_Delay(1);
    }	
	while(1)
	{
	    
		// 等待外部事件
		itu601scaler_event = OS_WaitEvent(ITU601_ISP_SCALAR_EVENT_INIT|ITU601_ISP_SCALAR_EVENT_STOP);

		if(itu601scaler_event & ITU601_ISP_SCALAR_EVENT_INIT)
		{
		    //初始化601-该函数一次只能调用一次,防止重复调用
		    if(Itu601_Data_Status == FALSE)
	    	{
	    		Itu601_Data_Status = TRUE;
	    		rITU656IN_ENABLE_REG &= ~( 1<<0 );
				ISP_itu601In_SelPad();
	            itu601_isp_scalar_mode_init();//初始化buffer空间
	            OS_Delay(10);
				SensorInitCaptureFIFO_One();
				OS_Delay(10);
				ISP_SCALAR_RUN; 
	    	}
		}		
		
		if(itu601scaler_event & ITU601_ISP_SCALAR_EVENT_STOP)
		{
    		if(Itu601_Data_Status == TRUE) 
			{
				Itu601_Data_Status = FALSE;
				ISP_SCALAR_STOP;
		        itu601_isp_scalar_mode_exit();
	            memset (itu601_isp_scalar_fifo, 0, sizeof(itu601_isp_scalar_fifo));
				OS_Delay(10);
				XMSYS_SensorCaptureReset (0);
				XMSYS_SensorResetCurrentPacket(0);
			}
			
		}
	}
	itu601_scalar_state = ITU656_IN_IDLE;
	OS_Terminate (NULL);
}


void itu601_isp_scaler_set_frame_ready (unsigned int frame_id)
{	
	XMSYSSENSORPACKET * sensorPacket;

	if(frame_id >= ITU601_ISP_SCALAR_FRAME_COUNT)
	{
		XM_printf ("illegal itu601 isp scaler in frame id (%d)\n", frame_id);
		return;
	}
	sensorPacket =  &itu601_isp_scalar_fifo[frame_id];	
	xm_isp_scalar_register_user_buffer (itu601_isp_scalar_object,
																	(u32_t)sensorPacket->data[0], (u32_t)sensorPacket->data[1],
																	itu601_isp_scalar_data_ready_user_callback, 
																	itu601_isp_scalar_frame_free_user_callback,
																	sensorPacket
																);
}

u32_t itu601_isp_scaler_get_video_width  (void)
{
	return itu601_isp_scaler_in_width; 	
}

u32_t itu601_isp_scaler_get_video_height (void)
{
	return itu601_isp_scaler_in_height;
}

u32_t itu601_isp_scaler_get_video_image_size (void)
{
	return itu601_isp_scaler_in_width * itu601_isp_scaler_in_height * 3 / 2;
}

char *itu601_isp_scaler_get_yuv_buffer (unsigned int frame_id)
{
	u32_t image_size = itu601_isp_scaler_in_width * itu601_isp_scaler_in_height * 3/2 + 2048;	
	if(frame_id < ITU601_ISP_SCALAR_FRAME_COUNT)
		return (char *)(((unsigned int)isp_scalar_get_frame_buffer (frame_id, image_size) + 1023)& ~1023);
	else
		return NULL;
}

// ITU601 Scaler任务初始化
void XMSYS_Itu601ScalerInit (void)
{
	itu601_isp_scalar_object = -1;
	queue_initialize (&itu601_isp_scalar_data);
	memset (itu601_isp_scalar_fifo, 0, sizeof(itu601_isp_scalar_fifo));
	OS_CREATETASK(&TCB_Itu601ScalerTask, "Itu601ScalerTask", XMSYS_Itu601Scaler_Task, XMSYS_ITU601SCALER_TASK_PRIORITY, StackItu601ScalerTask);
}

void XMSYS_Itu601ScalerExit (void)
{
	
}

#if 0
void xm_itu601_scalar_start (void)
{
	if(itu601_scalar_state != ITU656_IN_IDLE)
	{
		XM_printf ("xm_itu601_scalar_start before\n");
		return;
	}
	assert (itu601_scalar_state == ITU656_IN_IDLE);
	
	// 等待YUV Sensor线程启动
	while(itu601_scalar_state == ITU656_IN_IDLE)
	{
		OS_Delay (1);
	}
}

void xm_itu601_scalar_stop (void)
{
	if(itu601_scalar_state == ITU656_IN_IDLE)
		return;
	
	XM_printf ("xm_itu601_scalar_stop ...\n");
	// 发送YUV Sensor STOP事件
	OS_SignalEvent (ITU601_ISP_SCALAR_EVENT_STOP, &TCB_Itu601ScalerTask);
	itu601_scalar_stop = 1;
	// 等待ISP SCALER ITU601线程退出
	while(itu601_scalar_state != ITU656_IN_IDLE)
	{
		OS_Delay (1);
	}
	itu601_scalar_stop = 0;
	
}
#endif

