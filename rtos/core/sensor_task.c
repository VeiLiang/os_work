#include <stdio.h>
#include "hardware.h"
#include <xm_type.h>
//#include "FS.h"
#include "xm_core.h"
#include <xm_dev.h>
#include <xm_queue.h>
#include <assert.h>
#include <xm_base.h>
#include "arkn141_isp.h"
#include "xm_app_menudata.h"
#include "xm_itu656_in.h"
#include "ark1960.h"
#include "app_head.h"

extern void Notification_Itu601_Isp_Scaler_Init(void);
extern void XMSYS_SensorCheckDeadLineTicket (int channel);

#define	ISP_SCALAR_STOP		rISP_SCALE_EN = 0x0
#define	SENSOR_ID		0x50504E53
// 最多支持4个摄像头Sensor

// 异常输出
#define	SENSOR_FATAL		XM_printf
// 调试输出
#define	SENSOR_PRINT		XM_printf

#define	USE_PRE_PROCESS

//#define	INCLUDE_ISP_DEBUG	// ISP测试

#define	XMSYS_SENSOR_FRAME_COUNT			4	// ISP硬件缓冲FIFO为4

static OS_TASK TCB_SensorTask;
__no_init static OS_STACKPTR int StackSensorTask[XMSYS_SENSOR_TASK_STACK_SIZE/4];          /* Task stacks */


// 包缓冲池
static XMSYSSENSORPACKET SensorPacketPool[XMSYS_SENSOR_COUNT][XMSYS_SENSOR_FRAME_COUNT];
static queue_s	SensorPacketData[XMSYS_SENSOR_COUNT];		// 已采集Sensor包队列
static queue_s SensorPacketPrep[XMSYS_SENSOR_COUNT];		// 已完成预处理的包链表(加入时间戳/厂家Logo标志等信息)
static unsigned int data_packet_discard_count[XMSYS_SENSOR_COUNT];
static unsigned int prep_packet_discard_count[XMSYS_SENSOR_COUNT];
//static unsigned int data_packet_total_count;
static unsigned int data_packet_count[XMSYS_SENSOR_COUNT];

static XMSYSSENSORPACKET *SensorCurrentDataPacket[XMSYS_SENSOR_COUNT];		// 当前编码

//#define	DEADLINE_TICKET_ENABLE

#ifdef DEADLINE_TICKET_ENABLE
static volatile unsigned int deadline_ticket_to_remove;		// 已采集队列中的sensor帧丢弃时设置一个dead时间, 超出此时间, 帧必须丢弃.
																			//		一般这个时间设置为半帧时间
#endif

// Sensor资源访问互斥信号量
static OS_RSEMA		SensorSema[XMSYS_SENSOR_COUNT];	

static OS_RSEMA				Ark7116ProtectSema;		// 保护7116初始化
static unsigned int			old_rMCTL_ALPMR;			// DDR AutoPD参数
static OS_MAILBOX sensor_mailbox;		// 一字节邮箱。
static char sensor_mailbuf;				// 一个字节的邮箱

#define	MAX_FRAME_COUNT_TO_DISCARD_TO_REMOVE_FLASH	3	// 3帧

static int frames_to_discard_to_remove_flash;		// 开启sensor后, 为避免闪烁时需要扔掉的帧数

// Sensor尺寸定义

static const XMSIZE	SensorSize[XMSYS_SENSOR_COUNT] = {
	{ XMSYS_CH0_SENSOR_FRAME_WIDTH,	XMSYS_CH0_SENSOR_FRAME_HEIGHT },
#if XMSYS_SENSOR_COUNT > 1
	{ XMSYS_CH1_SENSOR_FRAME_WIDTH,	XMSYS_CH1_SENSOR_FRAME_HEIGHT },
#endif
};


static int const *SensorBuffer[XMSYS_SENSOR_COUNT][XMSYS_SENSOR_FRAME_COUNT];
static char *arkn141_isp_user_data;				// cache line对齐的地址
static char *arkn141_isp_user_data_base;		// 基址

static int SensorCaptureEnable;		// sensor状态, 1 工作 0 停止


// 在硬件FIFO队列中查找指定的地址
int match_frame_id_in_isp_scaler_fifo (unsigned int y_addr)
{
	int i;
	int frame_id = 0;
	
	for (i = 0; i < XMSYS_SENSOR_FRAME_COUNT; i ++)
	{
		if(((unsigned int)(SensorPacketPool[0][i].buffer)) == y_addr)
		{
			frame_id = SensorPacketPool[0][i].frame_id;
			break;
		}
	}
		  
	return frame_id;
}

extern u8_t Video_Format_DstResource;
//itu601
extern char *itu601_isp_scaler_get_yuv_buffer (unsigned int frame_id);
extern u32_t itu601_isp_scaler_get_video_height();
extern u32_t itu601_isp_scaler_get_video_width();
extern u32_t itu601_isp_scaler_get_video_image_size();
//itu656
extern char *itu656_in_get_yuv_buffer (unsigned int frame_id);
extern u32_t itu656_in_get_video_height();
extern u32_t itu656_in_get_video_width();
extern u32_t itu656_in_get_video_image_size();

u32_t SensorData_get_video_Width (void)
{
   /* if((Video_Format_DstResource == AP_DSTRESOURCE_CVBS_N) || (Video_Format_DstResource == AP_DSTRESOURCE_CVBS_P))
    {
        return itu656_in_get_video_width();
    }else*/ {
        return itu656_in_get_video_width();
    }
}

u32_t SensorData_get_video_Height (void)
{
   /* if((Video_Format_DstResource == AP_DSTRESOURCE_CVBS_N) || (Video_Format_DstResource == AP_DSTRESOURCE_CVBS_P))
    {
        return itu656_in_get_video_height();
    }else*/ {
        return itu656_in_get_video_height();
    }
}

u32_t SensorData_get_video_image_size (void)
{
    /*if((Video_Format_DstResource == AP_DSTRESOURCE_CVBS_N) || (Video_Format_DstResource == AP_DSTRESOURCE_CVBS_P))
    {
        return itu656_in_get_video_image_size();
    }else */{
        return itu656_in_get_video_image_size();
    }
}

char *SensorData_get_yuv_buffer (unsigned int frame_id)
{
   /* if((Video_Format_DstResource == AP_DSTRESOURCE_CVBS_N) || (Video_Format_DstResource == AP_DSTRESOURCE_CVBS_P))
    {
        return itu656_in_get_yuv_buffer(frame_id);
    }else*/ {
        return itu656_in_get_yuv_buffer(frame_id);
    }
}

extern void itu601_isp_scaler_set_frame_ready (unsigned int frame_id);
static void set_frame_ready (int channel, int frame_id)
{
	/*
    if((Video_Format_DstResource == AP_DSTRESOURCE_CVBS_N) || (Video_Format_DstResource == AP_DSTRESOURCE_CVBS_P))
    {
        return ;
    }
	*/
	if(channel == 0)
		itu601_isp_scaler_set_frame_ready(frame_id);
	else if(channel == 1)
		itu656_in_set_frame_ready (frame_id);
}


// 由sensor中断程序调用
// 通知一个新的数据帧已产生, 并将帧压入到YUV数据帧链表
// 调用时中断需要关闭
void XMSYS_SensorNotifyDataFrame (int channel, int frame_id)
{
	XMSYSSENSORPACKET *packet;

	if(channel >= XMSYS_SENSOR_COUNT)
		return;

	if(frame_id >= XMSYS_SENSOR_FRAME_COUNT)
		return;

	if(SensorCaptureEnable == 0)
	{
		XM_printf ("XMSYS_SensorNotifyDataFrame error, channel: %d \n",channel);
		return;
	}

	if(frames_to_discard_to_remove_flash > 0)
	{
		// 避免闪烁扔掉的帧
		set_frame_ready (channel, frame_id);
		frames_to_discard_to_remove_flash --;
		return;
	}

	if(!queue_empty(&SensorPacketData[channel]))
	{
		// 非空
		// 将未处理的包丢弃
#ifdef DEADLINE_TICKET_ENABLE
		XMSYS_SensorCheckDeadLineTicket (channel); // 检查是否已存在deadline时刻, 若存在, 删除
		deadline_ticket_to_remove = XM_GetTickCount() + 10;	// 设置deadline时刻
#else
		data_packet_discard_count[channel] ++;
		packet = (XMSYSSENSORPACKET *)queue_delete_next (&SensorPacketData[channel]);
		packet->data_ref --;
		if(packet->data_ref < 0)
		{
			XM_printf ("XMSYS_SensorNotifyDataFrame error, illegal data_ref count(0x%08x)\n", packet->data_ref);
		}
		//XM_printf ("discard %d, frame %d\n", channel,packet->frame_id);
		set_frame_ready (channel, packet->frame_id);
#endif
	}

	// 将包加入到YUV数据数据链表
	packet = &SensorPacketPool[channel][frame_id];

	// 复制当前的硬件曝光信息
	if(packet->isp_user_data)
	{
		// size(4) + ver(4) + sensor_name(8) + exp_data + isp_data
		isp_get_exp_data (packet->isp_user_data + 4 + 4 + 8, isp_get_exp_data_size());
	}

	if(packet->data_ref)
	{
		XM_printf ("XMSYS_SensorNotifyDataFrame error, illegal data_ref count(0x%08x)\n", packet->data_ref);
	}
	//printf("XMSYS_SensorNotifyDataFrame packet->prep_ref: %d packet->frame_id: %d\n",packet->prep_ref,packet->frame_id);
	if(packet->prep_ref)		// 此处存在缺陷
	{
		XM_printf ("XMSYS_SensorNotifyDataFrame error, illegal prep_ref count(0x%08x)\n", packet->prep_ref);
	}
	packet->data_ref ++;
	queue_insert ((queue_s *)packet, &SensorPacketData[channel]);
	// 20180213 实时视频更新不再受codec线程阻塞影响
	SensorCurrentDataPacket[channel] = packet;

	data_packet_count[channel] ++;
	OS_SignalEvent((char)(1 << channel), &TCB_SensorTask); /* 通知事件 */
}

#ifdef DEADLINE_TICKET_ENABLE
// 检查deadline时刻是否设置.
// 若设置, 检查是否已超时.
// 若超时, 检查sensor数据队列是否存在多余(2帧)的帧. 删除数据队列中多余的sensor帧(2帧则删除1帧, 1帧不删除)
void XMSYS_SensorCheckDeadLineTicket (int channel)
{
	XMSYSSENSORPACKET *packet;
	if(deadline_ticket_to_remove)
	{
		if(XM_GetTickCount() >= deadline_ticket_to_remove)
		{
			queue_s *prev, *next;
			prev = SensorPacketData[channel].prev;
			next = SensorPacketData[channel].next;
			if(	prev != next
				&& prev != &SensorPacketData[channel]
				&& next != &SensorPacketData[channel]
				)
			{
				// 存在至少2个sensor帧
				// 将未处理的包丢弃
				// 将队列头部的帧删除
				data_packet_discard_count ++;
				packet = (XMSYSSENSORPACKET *)queue_delete_next (&SensorPacketData[channel]);
				set_frame_ready (channel, packet->frame_id);
			}
			deadline_ticket_to_remove = 0;		// 删除deadline时刻
		}
	}
}
#endif

// 创建一个用于H264编码的视频帧
XMSYSSENSORPACKET * XMSYS_SensorCreatePacket (int channel)
{
	XMSYSSENSORPACKET *packet = NULL;
	if(channel >= XMSYS_SENSOR_COUNT)
	{
		XM_printf ("XMSYS_SensorCreatePacket failed, illegal channel(%d)\n", channel);
		return NULL;
	}
	if(SensorCaptureEnable == 0)
	{
		//XM_printf ("XMSYS_SensorCreatePacket failed, sensor stoped before\n");
		return NULL;
	}
#ifdef USE_PRE_PROCESS
	// 只允许预处理与H264编码共用一个信号量, 即只有一帧数据在预处理或者编码
	// 这样, 硬件队列总共4帧, 2帧在FIFO采集数据, 1帧缓存在预处理队列, 1帧在预处理或者编码
	OS_Use(&SensorSema[channel]); /* Make sure nobody else uses */
	if(!queue_empty((queue_s *)&SensorPacketPrep[channel]))
	{
		packet = (XMSYSSENSORPACKET *)queue_delete_next ((queue_s *)&SensorPacketPrep[channel]);
		if(packet->id != SENSOR_ID)
		{
			XM_printf ("XMSYS_SensorCreatePacket failed, illegal packet id=0x%08x\n", packet->id);
		}
		if(packet->data_ref != 0)
		{
			XM_printf ("XMSYS_SensorCreatePacket failed, illegal data_ref=(0x%08x)\n", packet->data_ref);
		}
		if(packet->prep_ref != 1)
		{
			XM_printf ("XMSYS_SensorCreatePacket failed, illegal data_ref=(0x%08x)\n", packet->prep_ref);
		}
	}
	
	// 20181008 解锁，避免视频文件写入busy时阻塞视频更新
	// FIFO非空时锁定信号量, 阻止预处理执行
	//if(packet == NULL)
		OS_Unuse(&SensorSema[channel]); /* Make sure nobody else uses */	
	
#else
	XM_lock();
	if(!queue_empty((queue_s *)&SensorPacketData[channel]))
	{
		packet = (XMSYSSENSORPACKET *)queue_delete_next ((queue_s *)&SensorPacketData[channel]);
	}
	XM_unlock();
#endif
	
	//XM_printf ("enc %d\n", packet->frame_id);
	return packet;
}

// 检查是否存在sensor数据包需要处理
// 1 表示任意一个通道上存在至少一个数据包需要处理
// 0 没有数据包需要处理
int XMSYS_SensorCheckPacketReady (void)
{
	int channel;
	int ready = 0;
	if(SensorCaptureEnable == 0)
		return ready;
	
#ifdef USE_PRE_PROCESS
	for (channel = 0; channel < XMSYS_SENSOR_COUNT; channel ++)
	{
		int is_empty;
		OS_Use(&SensorSema[channel]); /* Make sure nobody else uses */
		is_empty = queue_empty((queue_s *)&SensorPacketPrep[channel]);
		OS_Unuse(&SensorSema[channel]); /* Make sure nobody else uses */	
		if(!is_empty)
		{
			ready = 1;
			break;
		}
		
	}
#else
	XM_lock();
	for (channel = 0; channel < XMSYS_SENSOR_COUNT; channel ++)
	{
		if(!queue_empty((queue_s *)&SensorPacketData[channel]))
		{
			ready = 1;
			break;
		}
	}
	XM_unlock();
#endif
	
	return ready;
}

void XMSYS_SensorDeletePacket (int channel, XMSYSSENSORPACKET *packet)
{
	if(channel >= XMSYS_SENSOR_COUNT)
	{
		XM_printf ("XMSYS_SensorDeletePacket failed, illegal channel(%d)\n", channel);
		return;
	}
	if(packet == NULL)
	{
		XM_printf ("XMSYS_SensorDeletePacket failed, NULL packet\n");
		return;
	}
	if(packet->id != SENSOR_ID)
	{
		XM_printf ("XMSYS_SensorDeletePacket failed, illegal packet id=0x%08x\n", packet->id);
		return;
	}
	if(SensorCaptureEnable == 0)
	{
		XM_printf ("XMSYS_SensorDeletePacket failed, sensor stoped before\n");
	}
	
	if(packet->data_ref != 0)
	{
		XM_printf ("XMSYS_SensorDeletePacket failed, illegal data_ref=(0x%08x)\n", packet->data_ref);
	}
	if(packet->prep_ref != 1)
	{
		XM_printf ("XMSYS_SensorDeletePacket failed, illegal data_ref=(0x%08x)\n", packet->prep_ref);
	}
		
	// 将其重新插入到硬件队列
	assert (packet->frame_id < 4);
	
	XM_lock();
	//XM_printf ("psub 0x%08x\n", packet);
	packet->prep_ref --;
	set_frame_ready (channel, packet->frame_id);
	XM_unlock();
	
#ifdef USE_PRE_PROCESS
	OS_Use(&SensorSema[channel]);

#ifdef DEADLINE_TICKET_ENABLE
	// 检查是否存在未处理的sensor帧. 若存在, 通知预处理线程
	if(!queue_empty ((queue_s *)&SensorPacketData[channel]))
	{
		OS_SignalEvent((char)(1 << channel), &TCB_SensorTask); /* 通知事件 */
	}
#endif	
	
	// 解除信号量锁定
	// 预处理与H264编码共用一个信号量
	OS_Unuse(&SensorSema[channel]); /* Make sure nobody else uses */	
#endif	
}

void XMSYS_SensorReturnPacket (int channel, XMSYSSENSORPACKET *packet)
{
	if(channel >= XMSYS_SENSOR_COUNT)
	{
		XM_printf ("XMSYS_SensorReturnPacket failed, illegal channel(%d)\n", channel);
		return;
	}
	if(packet == NULL)
	{
		XM_printf ("XMSYS_SensorReturnPacket failed, NULL packet\n");
		return;
	}
	if(packet->id != SENSOR_ID)
	{
		XM_printf ("XMSYS_SensorReturnPacket failed, illegal packet id=0x%08x\n", packet->id);
		return;
	}
	if(SensorCaptureEnable == 0)
	{
		XM_printf ("XMSYS_SensorReturnPacket failed, sensor stoped before\n");
	}
	if(packet->data_ref != 0)
	{
		XM_printf ("XMSYS_SensorReturnPacket failed, illegal packet data_ref=(0x%08x)\n", packet->data_ref);
		return;
	}
	if(packet->prep_ref != 1)
	{
		XM_printf ("XMSYS_SensorReturnPacket failed, illegal packet prep_ref=(0x%08x)\n", packet->prep_ref);
		return;
	}
	
	// 将其重新插入到硬件队列
	assert (packet->frame_id < 4);
	
	XM_lock();
	//XM_printf ("psub 0x%08x\n", packet);
	packet->prep_ref --;
	set_frame_ready (channel, packet->frame_id);
	XM_unlock();
	
}

// 一键拍照重新将sensor帧投递到预处理队列, 提供给H264编码器使用
void XMSYS_SensorRecyclePacket (int channel, XMSYSSENSORPACKET *packet)
{
	if(channel >= XMSYS_SENSOR_COUNT)
	{
		XM_printf ("XMSYS_SensorRecyclePacket failed, illegal channel(%d)\n", channel);
		return;
	}
	if(packet == NULL)
	{
		XM_printf ("XMSYS_SensorRecyclePacket failed, NULL packet\n");
		return;
	}
	if(packet->id != SENSOR_ID)
	{
		XM_printf ("XMSYS_SensorRecyclePacket failed, illegal packet id=0x%08x\n", packet->id);
		return;
	}
	if(SensorCaptureEnable == 0)
	{
		XM_printf ("XMSYS_SensorRecyclePacket failed, sensor stoped before\n");
	}
	if(packet->data_ref != 0)
	{
		XM_printf ("XMSYS_SensorRecyclePacket failed, illegal data_ref=(0x%08x)\n", packet->data_ref);
	}
	if(packet->prep_ref != 1)
	{
		XM_printf ("XMSYS_SensorRecyclePacket failed, illegal data_ref=(0x%08x)\n", packet->prep_ref);
	}
	
	
#ifdef USE_PRE_PROCESS
	OS_Use(&SensorSema[channel]);

	// 加入到预处理队列尾部
	queue_insert ((queue_s *)packet, (queue_s *)&SensorPacketPrep[channel]);
		
	// 解除信号量锁定
	// 预处理与H264编码共用一个信号量
	OS_Unuse(&SensorSema[channel]); /* Make sure nobody else uses */	
#else
	// 将其重新插入到硬件队列
	assert (packet->frame_id < 4);
	
	XM_lock();
	set_frame_ready (channel, packet->frame_id);
	XM_unlock();	
#endif

}
VOID XMSYS_SensorResetCurrentPacket(int channel)
{
    XM_lock ();
   SensorCurrentDataPacket[channel] = NULL; 
   XM_unlock ();
}

// 获取指定通道最新的sensor帧
XMSYSSENSORPACKET * XMSYS_SensorGetCurrentPacket (int channel)
{
	XMSYSSENSORPACKET *frame;
	XM_lock ();
	frame = SensorCurrentDataPacket[channel];
    //SensorCurrentDataPacket[channel] = NULL;
	XM_unlock ();
	return frame;
}

extern void sensor_init (void);
extern const char *isp_get_sensor_name (void);

void SensorInitCaptureFIFO_One (void)
{
	int j;
	OS_Use(&SensorSema[0]);
	for (j = 0; j < XMSYS_SENSOR_FRAME_COUNT; j ++)
	{
		SensorPacketPool[0][j].buffer = (char *)itu601_isp_scaler_get_yuv_buffer(j);//(isp_scalar_static_fifo+j*image_size);//(char *)isp_get_yuv_buffer(j);
		SensorPacketPool[0][j].width = itu601_isp_scaler_get_video_width();//isp_scaler_get_video_width();//1920;//isp_get_video_width();
		SensorPacketPool[0][j].height =itu601_isp_scaler_get_video_height();//isp_scaler_get_video_height();//1080;isp_get_video_height();
		SensorPacketPool[0][j].size = itu601_isp_scaler_get_video_image_size();//isp_scaler_get_video_image_size();//1920*1080*3/2;//isp_get_video_image_size ();
		SensorPacketPool[0][j].id = SENSOR_ID;
		SensorPacketPool[0][j].frame_id = j;
		SensorPacketPool[0][j].isp_user_size = 0;
		SensorPacketPool[0][j].isp_user_data = 0;
	}
	queue_initialize (&SensorPacketData[0]);		// 原始包队列
	queue_initialize (&SensorPacketPrep[0]);
	OS_Unuse(&SensorSema[0]); 
}

void SensorInitCaptureFIFO_Second (void)
{
	int j;
	OS_Use(&SensorSema[1]);
	for (j = 0; j < XMSYS_SENSOR_FRAME_COUNT; j ++)
	{
		SensorPacketPool[1][j].buffer = (char *)itu656_in_get_frame_buffer(j,itu656_in_get_video_image_size());//(char *)itu656_in_get_yuv_buffer(j);//(isp_scalar_static_fifo+j*image_size);//(char *)isp_get_yuv_buffer(j);
		SensorPacketPool[1][j].width = itu656_in_get_video_width();//isp_scaler_get_video_width();//1920;//isp_get_video_width();
		SensorPacketPool[1][j].height =itu656_in_get_video_height();//isp_scaler_get_video_height();//1080;isp_get_video_height();
		SensorPacketPool[1][j].size = itu656_in_get_video_image_size();//isp_scaler_get_video_image_size();//1920*1080*3/2;//isp_get_video_image_size ();
		SensorPacketPool[1][j].id = SENSOR_ID;
		SensorPacketPool[1][j].frame_id = j;
		SensorPacketPool[1][j].isp_user_size = 0;
		SensorPacketPool[1][j].isp_user_data = 0;
	}
	queue_initialize (&SensorPacketData[1]);		// 原始包队列
	queue_initialize (&SensorPacketPrep[1]);
	OS_Unuse(&SensorSema[1]); 
}

void SensorInitCaptureFIFO_Three (void)
{
	int j;
	OS_Use(&SensorSema[2]);
	for (j = 0; j < XMSYS_SENSOR_FRAME_COUNT; j ++)
	{
		SensorPacketPool[2][j].buffer = (char *)Usb_get_yuv_buffer(j);//(isp_scalar_static_fifo+j*image_size);//(char *)isp_get_yuv_buffer(j);
		SensorPacketPool[2][j].width = Usb_get_video_Width();//isp_scaler_get_video_width();//1920;//isp_get_video_width();
		SensorPacketPool[2][j].height =Usb_get_video_height();//isp_scaler_get_video_height();//1080;isp_get_video_height();
		SensorPacketPool[2][j].size = Usb_get_video_image_size();//isp_scaler_get_video_image_size();//1920*1080*3/2;//isp_get_video_image_size ();
		SensorPacketPool[2][j].id = SENSOR_ID;
		SensorPacketPool[2][j].frame_id = j;
		SensorPacketPool[2][j].isp_user_size = 0;
		SensorPacketPool[2][j].isp_user_data = 0;
	}
	queue_initialize (&SensorPacketData[2]);		// 原始包队列
	queue_initialize (&SensorPacketPrep[2]);
	OS_Unuse(&SensorSema[2]); 
}



void SensorInitCaptureFIFO (void)
{
	int i, j;
	unsigned int user_data_size;

	// 32字节对齐(Cache Line aligned), 优化后续的memcpy操作
	#if 0
	user_data_size = isp_get_isp_data_size() + isp_get_exp_data_size() + 4 + 4 + 8;
	user_data_size = (user_data_size + 0x1F) & (~0x1F);
	#endif
	
	// ISP通道(通道序号0)配置
	OS_Use(&SensorSema[0]);
	for (j = 0; j < XMSYS_SENSOR_FRAME_COUNT; j ++)
	{
		SensorPacketPool[0][j].buffer = (char *)itu601_isp_scaler_get_yuv_buffer(j);//(isp_scalar_static_fifo+j*image_size);//(char *)isp_get_yuv_buffer(j);
		SensorPacketPool[0][j].width = itu601_isp_scaler_get_video_width();//isp_scaler_get_video_width();//1920;//isp_get_video_width();
		SensorPacketPool[0][j].height =itu601_isp_scaler_get_video_height();//isp_scaler_get_video_height();//1080;isp_get_video_height();
		SensorPacketPool[0][j].size = itu601_isp_scaler_get_video_image_size();//isp_scaler_get_video_image_size();//1920*1080*3/2;//isp_get_video_image_size ();
		SensorPacketPool[0][j].id = SENSOR_ID;
		SensorPacketPool[0][j].frame_id = j;
		SensorPacketPool[0][j].isp_user_size = 0;
		SensorPacketPool[0][j].isp_user_data = 0;
	}
	queue_initialize (&SensorPacketData[0]);		// 原始包队列
	queue_initialize (&SensorPacketPrep[0]);
	OS_Unuse(&SensorSema[0]);
	
	// 第2路CVBS
	OS_Use(&SensorSema[1]);
	for (j = 0; j < XMSYS_SENSOR_FRAME_COUNT; j ++)
	{
		SensorPacketPool[1][j].buffer = (char *)itu656_in_get_frame_buffer(j,itu656_in_get_video_image_size());//(char *)itu656_in_get_yuv_buffer(j);//(isp_scalar_static_fifo+j*image_size);//(char *)isp_get_yuv_buffer(j);
		SensorPacketPool[1][j].width = itu656_in_get_video_width();//isp_scaler_get_video_width();//1920;//isp_get_video_width();
		SensorPacketPool[1][j].height =itu656_in_get_video_height();//isp_scaler_get_video_height();//1080;isp_get_video_height();
		SensorPacketPool[1][j].size = itu656_in_get_video_image_size();//isp_scaler_get_video_image_size();//1920*1080*3/2;//isp_get_video_image_size ();
		SensorPacketPool[1][j].id = SENSOR_ID;
		SensorPacketPool[1][j].frame_id = j;
		SensorPacketPool[1][j].isp_user_size = 0;
		SensorPacketPool[1][j].isp_user_data = 0;
	}
	queue_initialize (&SensorPacketData[1]);		// 原始包队列
	queue_initialize (&SensorPacketPrep[1]);
	OS_Unuse(&SensorSema[1]);
/*
	//第三路
	OS_Use(&SensorSema[2]);
	for (j = 0; j < XMSYS_SENSOR_FRAME_COUNT; j ++)
	{
		SensorPacketPool[2][j].buffer = (char *)Usb_get_yuv_buffer(j);//(isp_scalar_static_fifo+j*image_size);//(char *)isp_get_yuv_buffer(j);
		SensorPacketPool[2][j].width = Usb_get_video_Width();//isp_scaler_get_video_width();//1920;//isp_get_video_width();
		SensorPacketPool[2][j].height =Usb_get_video_height();//isp_scaler_get_video_height();//1080;isp_get_video_height();
		SensorPacketPool[2][j].size = Usb_get_video_image_size();//isp_scaler_get_video_image_size();//1920*1080*3/2;//isp_get_video_image_size ();
		SensorPacketPool[2][j].id = SENSOR_ID;
		SensorPacketPool[2][j].frame_id = j;
		SensorPacketPool[2][j].isp_user_size = 0;
		SensorPacketPool[2][j].isp_user_data = 0;
	}
	queue_initialize (&SensorPacketData[2]);		// 原始包队列
	queue_initialize (&SensorPacketPrep[2]);
	OS_Unuse(&SensorSema[2]);
*/    
	#if 0
	for (i = 0; i < XMSYS_SENSOR_COUNT; i ++)
	{
		queue_initialize (&SensorPacketData[i]);		// 原始包队列
		queue_initialize (&SensorPacketPrep[i]);
    }
    #endif
}

//  设置Sensor Capture缓冲区
void SensorConfigCaptureFIFO (void)
{
	int i, j;
	// 设置Sensor Capture缓冲区
	for (i = 0; i < XMSYS_SENSOR_COUNT; i ++)
	{
		OS_Use(&SensorSema[i]); /* Make sure nobody else uses */
		for (j = 0; j < XMSYS_SENSOR_FRAME_COUNT; j++)
		{
			dma_inv_range ((u32)SensorPacketPool[i][j].buffer,
								((u32)SensorPacketPool[i][j].buffer) + SensorPacketPool[i][j].size);
			// 加入到Sensor采集缓存
			//XMSYS_SensorStartCapture (i, (int *)SensorPacketPool[i][j].buffer);
		}
		OS_Unuse(&SensorSema[i]); /* Make sure nobody else uses */	
	}
}

//#define SENSOR_DEBUG

static void process_packet_ready_event (unsigned int channel)
{
	XMSYSSENSORPACKET *packet;
	if(channel >= XMSYS_SENSOR_COUNT)
		return;

#ifdef USE_PRE_PROCESS
	OS_Use(&SensorSema[channel]); /* Make sure nobody else uses */
	do
	{
		// 从YUV数据链表获取数据包
		XM_lock ();
		if(queue_empty ((queue_s *)&SensorPacketData[channel]))
		{
			packet = NULL;
		}
		else
		{
			packet = (XMSYSSENSORPACKET *)queue_delete_next ((queue_s *)&SensorPacketData[channel]);
			if(packet->data_ref != 1)
			{
				XM_printf ("ch(%d)'s packet (0x%08x) 's data_ref (0x%08x) illegal\n", channel, (unsigned int)packet, packet->data_ref);
			}
			if(packet->prep_ref != 0)
			{
				XM_printf ("ch(%d)'s packet (0x%08x) 's prep_ref (0x%08x) illegal\n", channel, (unsigned int)packet, packet->prep_ref);
			}
		}
		XM_unlock ();
				
		if(SensorCaptureEnable == 0)
		{
			XM_printf ("illegal state, sensor stoped before\n");
		}
						
		//该通道没有ready包
		if(packet == NULL)
			break;
		
#if 1
		//录像视频嵌入时间水印
		//XM_printf(">>>>>>>>AP_GetMenuItem(APPMENUITEM_TIME_STAMP):%d\r\n", AP_GetMenuItem(APPMENUITEM_TIME_STAMP));
		if(packet && (AP_GetMenuItem(APPMENUITEM_TIME_STAMP) == AP_SETTING_VIDEO_TIMESTAMP_ON))
		{
			void embed_local_time_into_video_frame(
													unsigned int width,			// 视频宽度
													unsigned int height,		// 视频高度
													unsigned int stride,		// Y_UV行字节长度
													char *data					// Y_UV缓冲区
													);
					
			unsigned int vaddr = page_addr_to_vaddr((unsigned int)packet->buffer);
			unsigned char *virtual_address = (unsigned char *)vaddr;
			// 嵌入时间戳信息
			// 使用虚地址(Cache)优化算法
			embed_local_time_into_video_frame (packet->width, packet->height, 
																  packet->width, 
																  //packet->buffer
																  (char *)virtual_address
																);		
					
		}
#endif		

		// 1路601输入均调整为录制23帧(带宽资源紧张)
		unsigned int channel_1_mask = 0x7FFF7FFF;
		unsigned int channel_1_shift = 1 << (data_packet_count[channel] % 32);
		if( (channel == 1) && (channel_1_mask & channel_1_shift) == 0)
		{
			prep_packet_discard_count[channel] ++;
			packet->data_ref --;
			packet->prep_ref ++;
			XMSYS_SensorReturnPacket (channel, packet);
			break;
		}
		
		// 加入时间戳/厂家Logo标志信息, 并将其加入到预处理链表
		// 检查预处理队列是否存在尚未处理的包. 若存在, 将其从预处理队列中删除,并重新压入到硬件队列
		if(!queue_empty(&SensorPacketPrep[channel]))
		{
			// 检查并删除
			XMSYSSENSORPACKET *packet_to_delete = (XMSYSSENSORPACKET *)queue_delete_next ((queue_s *)&SensorPacketPrep[channel]);
			if(packet_to_delete->data_ref != 0 || packet_to_delete->prep_ref != 1)
			{
				XM_printf ("packet (0x%08x) 's ref illegal\n", (unsigned int)packet);
			}
			prep_packet_discard_count[channel] ++;
			// 重新压入到硬件队列
			XMSYS_SensorReturnPacket (channel, packet_to_delete);
		}

		// 加入到预处理队列尾部
		packet->data_ref --;
		packet->prep_ref ++;
		//XM_printf ("prep 0x%08x\n", packet);
		queue_insert ((queue_s *)packet, (queue_s *)&SensorPacketPrep[channel]);
			
#ifdef SENSOR_DEBUG
		static XMINT64 pre_ticket[XMSYS_SENSOR_COUNT] = {0};
		static unsigned int pre_count[XMSYS_SENSOR_COUNT] = {0};
		static unsigned int s_ticket[XMSYS_SENSOR_COUNT] = {0};
		XMINT64 t1 = XM_GetHighResolutionTickCount(); 
		if(s_ticket[channel] == 0)
			s_ticket[channel] = XM_GetTickCount();
#endif

		#if 0
		// 复制ISP控制信息
		if(packet && packet->isp_user_data)
		{
			// size(4) + ver(4) + sensor_name(8) + exp_data + isp_data
			//dma_inv_range ((unsigned int)packet->isp_user_data, packet->isp_user_size + (unsigned int)packet->isp_user_data);
			isp_get_isp_data (packet->isp_user_data + 4 + 4 + 8 + isp_get_exp_data_size(), isp_get_isp_data_size());
			dma_flush_range ((unsigned int)packet->isp_user_data, packet->isp_user_size + (unsigned int)packet->isp_user_data);
		}
		#endif
		
		char *isp_data_buff = XMSYS_SensorInsertUserDataIntoPacket (packet, XM_PACKET_USER_DATA_ID_ISP, NULL, isp_get_isp_data_size());
		if(isp_data_buff)
		{
			// 将isp数据复制到分配的数据空间
			isp_get_isp_data (isp_data_buff, isp_get_isp_data_size());
		}
		
		// 保存时间信息
		//XMSYSTEMTIME LocalTime;
		//XM_GetLocalTime (&LocalTime);
		//XMSYS_SensorInsertUserDataIntoPacket (packet, XM_PACKET_USER_DATA_ID_DATETIME, (char *)&LocalTime, sizeof(LocalTime));

#ifdef SENSOR_DEBUG
		unsigned int ticket = (unsigned int)(XM_GetHighResolutionTickCount() - t1);
		pre_ticket[channel] += ticket;
		pre_count[channel] ++;
		if(pre_count[channel] % 300 == 0)
		{
			unsigned int e_ticket = XM_GetTickCount() - s_ticket[channel];
			XM_printf ("ch=%d, fps=%d, pre_ticket=%d\n", channel, pre_count[channel] * 10000 / e_ticket, (unsigned int)(pre_ticket[channel] / pre_count[channel]));
			pre_ticket[channel] = 0;
			pre_count[channel] = 0;
			s_ticket[channel] = 0;
			//XM_printf ("ch(%d), sensor discard count, data=%d, prep=%d\n", channel, data_packet_discard_count[channel], prep_packet_discard_count[channel]);
			data_packet_discard_count[channel] = 0;
			prep_packet_discard_count[channel] = 0;
		}
#endif
	} while(0);

	OS_Unuse(&SensorSema[channel]); /* Make sure nobody else uses */			
#endif
}
BOOL SensorData_Stop_Rec = FALSE;
BOOL SensorData_CH0_Status = FALSE;
BOOL SensorData_CH1_Status = FALSE;
BOOL SensorData_CH2_Status = FALSE;

void XMSYS_SensorTask (void)
{
	OS_U8 sensor_event;
	int *RawFrame;
	char ret;
	unsigned int packet_size;
	XMSYSSENSORPACKET *packet;

	while(1)
	{
		// 等待外部事件
		sensor_event = OS_WaitEvent(	XMSYS_SENSOR_CH_0_PACKET_READY_EVENT
											|	XMSYS_SENSOR_CH_1_PACKET_READY_EVENT
											| 	XMSYS_SENSOR_CH_2_PACKET_READY_EVENT
											|	XMSYS_SENSOR_CAPTURE_START
											|	XMSYS_SENSOR_CAPTURE_STOP
											|	XMSYS_SENSOR_CAPTURE_EXIT
											);

		if(sensor_event & XMSYS_SENSOR_CH_0_PACKET_READY_EVENT)
		{
			process_packet_ready_event (0);
		}
		if(sensor_event & XMSYS_SENSOR_CH_1_PACKET_READY_EVENT)
		{
			process_packet_ready_event (1);
		}
        if(sensor_event & XMSYS_SENSOR_CH_2_PACKET_READY_EVENT)
		{
			process_packet_ready_event (2);
		}

		if(sensor_event & XMSYS_SENSOR_CAPTURE_START)
		{
			// 启动Sensor采集
			// 启动通道0的Capture

			// 20170609
			// 检查sensor是否已启动.
			if(SensorCaptureEnable == 0)
			{
				XM_printf ("SENSOR_CAPTURE_START\n");
				// 复位Sensor包队列
				SensorInitCaptureFIFO ();
				//SensorInitCaptureFIFO ();
				frames_to_discard_to_remove_flash = MAX_FRAME_COUNT_TO_DISCARD_TO_REMOVE_FLASH;
				//OS_Delay (100);
				SensorCaptureEnable = 1;
#if 1
				// 通道0 (ISP)
				//XMSYS_SensorStart ();
				Notification_Itu601_Isp_Scaler_Init();//发送通知给601 开启接受数据动作
				rxchip_itu656_PlugIn();
#else//XMSYS_ITU656_IN_SUPPORT
				// 通道1 (ITU656 IN)
				xm_itu656_in_start ();
				xm_itu601_scalar_start();
#endif
			}
			else
			{
				XM_printf ("SENSOR_CAPTURE_RE_START\n");
			}

			ret = 0;
			OS_PutMail1 (&sensor_mailbox, &ret);
		}

		//else if(sensor_event & XMSYS_SENSOR_CAPTURE_STOP)
		if(sensor_event & XMSYS_SENSOR_CAPTURE_STOP)
		{
			// 停止Sensor采集
			XM_printf ("SENSOR_CAPTURE_STOP\n");
			SensorData_Stop_Rec = TRUE;
			// 20170703 避免ISP与Sensor task之间互锁, 进入时不占用SensorSema资源
			// OS_Use(&SensorSema); /* Make sure nobody else uses */
			// 先停止ISP scalar， 再停止ISP。isp scalar依赖ISP时钟
			//xm_itu601_scalar_stop ();
			//XMSYS_SensorStop ();
			SensorCaptureEnable = 0;
			// OS_Unuse(&SensorSema); /* Make sure nobody else uses */
			//停止接受601 数据
			Notification_Itu601_Isp_Scaler_Stop();
			rxchip_itu656_PlugOut();
			
			XMSYS_SensorResetCurrentPacket(0);
			XMSYS_SensorResetCurrentPacket(1);
			//XMSYS_SensorResetCurrentPacket(2);

			// 复位Sensor线程的所有外部事件
			//OS_Use(&SensorSema); /* Make sure nobody else uses */
			OS_ClearEvents (&TCB_SensorTask);
			//OS_Unuse(&SensorSema); /* Make sure nobody else uses */
			ret = 0;

			XM_printf ("SENSOR_CAPTURE_STOP PutMail\n");
			OS_PutMail1 (&sensor_mailbox, &ret);
		}

		if(sensor_event & XMSYS_SENSOR_CAPTURE_EXIT)
		{
			break;
		}
	}

	OS_Terminate (NULL);
}


// Sensor任务初始化
void XMSYS_SensorInit (void)
{
	int i, j;
	unsigned int user_data_size, user_data_size_base;
	// 创建线程通信邮箱
	OS_CREATEMB (&sensor_mailbox, 1, 1, &sensor_mailbuf);

	// 创建互斥访问信号量
	for (i = 0; i < XMSYS_SENSOR_COUNT; i ++)
		OS_CREATERSEMA(&SensorSema[i]); /* Creates resource semaphore */

	OS_CREATERSEMA(&Ark7116ProtectSema);

#ifndef INCLUDE_ISP_DEBUG
	arkn141_isp_user_data = NULL;
#else
	user_data_size = 4 + 4 + 8 + isp_get_isp_data_size() + isp_get_exp_data_size();
	user_data_size_base = (user_data_size + 0x1F) & (~0x1F);		// 按照32字节对其并占用完整的cache line(Cortex A5 Cache line size);
	arkn141_isp_user_data_base = kernel_malloc ((user_data_size_base * XMSYS_SENSOR_FRAME_COUNT + 0x3F) & (~0x3F));
	if(arkn141_isp_user_data_base)
	{
		arkn141_isp_user_data = (char *)((((unsigned int)arkn141_isp_user_data_base) + 0x1F) & (~0x1F));
		memset (arkn141_isp_user_data, 0, user_data_size_base * XMSYS_SENSOR_FRAME_COUNT);
	}
	else
	{
		arkn141_isp_user_data = NULL;
	}
	assert (arkn141_isp_user_data);
#endif
    SensorCaptureEnable = 1;
	SensorInitCaptureFIFO ();

	// Sensor硬件初始化
	//XMSYS_SensorHardwareInit ();

	//SENSOR_PRINT ("XMSYS_SensorStart\n");

	//SensorConfigCaptureFIFO ();

	// 创建Sensor线程并启动
	OS_CREATETASK(&TCB_SensorTask, "SensorTask", XMSYS_SensorTask, XMSYS_SENSOR_TASK_PRIORITY, StackSensorTask);

	// 启动sensor采集
	//XMSYS_SensorStart ();
}

// Sensor任务结束
void XMSYS_SensorExit (void)
{
	XMSYS_SensorCaptureStop ();
	OS_SignalEvent(XMSYS_SENSOR_CAPTURE_EXIT, &TCB_SensorTask); /* 通知事件 */
	OS_Delay (20);
	if(arkn141_isp_user_data)
	{
		kernel_free (arkn141_isp_user_data_base);
		arkn141_isp_user_data_base = NULL;
		arkn141_isp_user_data = NULL;
	}
	for (int i = 0; i < XMSYS_SENSOR_COUNT; i ++)
		OS_DeleteRSema (&SensorSema[i]); /* Creates resource semaphore */
	OS_DeleteMB (&sensor_mailbox);
}

// 启动Sensor采集
void XMSYS_SensorCaptureStart (void)
{
	char ret = 0;
	OS_SignalEvent(XMSYS_SENSOR_CAPTURE_START, &TCB_SensorTask); /* 通知事件 */
	// 等待Sensor任务应答
	// OS_GetMail1 (&sensor_mailbox, &ret);
	if(OS_GetMailTimed (&sensor_mailbox, &ret, 1000) == 1)
	{
		XM_printf ("timeout to SensorCaptureStart\n");
	}
}

// 停止Sensor采集
void XMSYS_SensorCaptureStop (void)
{
	char ret = 0;
	OS_SignalEvent(XMSYS_SENSOR_CAPTURE_STOP, &TCB_SensorTask); /* 通知事件 */
	// 等待Sensor任务应答
	// OS_GetMail1 (&sensor_mailbox, &ret);
	if(OS_GetMailTimed (&sensor_mailbox, &ret, 1000) == 1)
	{
		XM_printf ("timeout to SensorCaptureStop\n");
	}
}

// sensor异常时, 复位相应的通道
int XMSYS_SensorCaptureReset (int channel)
{
	int i;
	int busy;
	int ret = -1;
	unsigned int ticket_to_timeout;
	if(channel >= XMSYS_SENSOR_COUNT)
		return -1;
	
	// 将已采集Sensor包队列及预处理队列的包释放
	OS_Use(&SensorSema[channel]); /* Make sure nobody else uses */
	XM_lock ();
	// 已采集Sensor包队列包释放
	while(!queue_empty ((queue_s *)&SensorPacketData[channel]))
	{
		XMSYSSENSORPACKET *packet = (XMSYSSENSORPACKET *)queue_delete_next ((queue_s *)&SensorPacketData[channel]);
		if(packet->data_ref != 1)
		{
			XM_printf ("packet (0x%08x) 's data_ref (0x%08x) illegal\n", (unsigned int)packet, packet->data_ref);
		}
		if(packet->prep_ref != 0)
		{
			XM_printf ("packet (0x%08x) 's prep_ref (0x%08x) illegal\n", (unsigned int)packet, packet->prep_ref);
		}
		packet->data_ref = 0;
		packet->prep_ref = 0;
	}
	// 预处理队列包释放
	while(!queue_empty ((queue_s *)&SensorPacketPrep[channel]))
	{
		XMSYSSENSORPACKET *packet = (XMSYSSENSORPACKET *)queue_delete_next ((queue_s *)&SensorPacketPrep[channel]);
		if(packet->data_ref != 0)
		{
			XM_printf ("packet (0x%08x) 's data_ref (0x%08x) illegal\n", (unsigned int)packet, packet->data_ref);
		}
		if(packet->prep_ref != 1)
		{
			XM_printf ("packet (0x%08x) 's prep_ref (0x%08x) illegal\n", (unsigned int)packet, packet->prep_ref);
		}
		packet->data_ref = 0;
		packet->prep_ref = 0;

		//XM_printf ("psub 0x%08x\n", packet);
	}
	XM_unlock ();
	OS_Unuse(&SensorSema[channel]); /* Make sure nobody else uses */
	
	ticket_to_timeout = XM_GetTickCount() + 1000;
	// 等待预处理包释放(编码)
	do
	{
		OS_Delay (1);

		busy = 0;
		XM_lock ();
		for (i = 0; i < XMSYS_SENSOR_FRAME_COUNT; i ++)
		{
			if(SensorPacketPool[channel][i].data_ref || SensorPacketPool[channel][i].prep_ref)
			{
				XM_printf ("packet 0x%08x busy\n", (unsigned int)&SensorPacketPool[channel][i]);
				busy = 1;
			}
		}
		XM_unlock ();

		if(busy == 0)
		{
			ret = 0;
			break;
		}

		if(XM_GetTickCount() >= ticket_to_timeout)
		{
			ret = -1;
			XM_printf ("timeout to waiting for preprocess fifo\n");
			break;
		}
	} while(busy);

	return ret;
}


void XMSYS_SensorStartCaptureAccount (void)
{

}

void XMSYS_SensorStopCaptureAccount (void)
{

}

unsigned int XMSYS_SensorGetCameraChannel (void)
{
	return 0;
}

void XMSYS_SensorGetChannelSize (unsigned int channel, unsigned int *width, unsigned int *height)
{
	if(channel >= XMSYS_SENSOR_COUNT)
	{
		*width = 0;
		*height = 0;
	}
	if(channel == 0)	// ISP
	{
		*width = isp_get_video_width();
		*height = isp_get_video_height();
	}
#if XMSYS_ITU656_IN_SUPPORT
	else
	{
		*width = itu601_isp_scaler_get_video_width();//itu656_in_get_video_width();
		*height = itu601_isp_scaler_get_video_height();//itu656_in_get_video_height();
	}
#endif
}

void XMSYS_SensorSetCameraChannel (unsigned int channel)
{

}

unsigned int XMSYS_SensorGetCameraChannelWidth (unsigned int channel)
{
	if(channel >= XMSYS_SENSOR_COUNT)
		return 0;
	if(channel == 0)		// ISP
		return isp_get_video_width();
#if 1//XMSYS_ITU656_IN_SUPPORT
	else
		return itu601_isp_scaler_get_video_width();//itu656_in_get_video_width();
#else
	return 0;
#endif
}

unsigned int XMSYS_SensorGetCameraChannelHeight (unsigned int channel)
{
	if(channel >= XMSYS_SENSOR_COUNT)
		return 0;
	if(channel == 0)	// ISP
		return isp_get_video_height();
#if 1//XMSYS_ITU656_IN_SUPPORT
	else
		return itu601_isp_scaler_get_video_height();//itu656_in_get_video_height();
#else
	return 0;
#endif

}

unsigned int SensorData_CH0_Format = AP_DSTRESOURCE_720P_25;
unsigned int SensorData_CH1_Format = AP_DSTRESOURCE_720P_25;
unsigned int SensorData_CH2_Format = AP_DSTRESOURCE_720P_25;

BOOL Compare_CH0_1080P_Status(VOID)
{
	if((SensorData_CH0_Format == AP_DSTRESOURCE_1080P_25) || (SensorData_CH0_Format == AP_DSTRESOURCE_1080P_30))
		return TRUE;
	else 
		return FALSE;
}
unsigned int SensorData_Get_FormatInfo(VOID)
{
#if 0
	//1080P_720P
	if(((SensorData_CH0_Format == AP_DSTRESOURCE_1080P_25)||(SensorData_CH0_Format == AP_DSTRESOURCE_1080P_30)) \
	  &&((SensorData_CH1_Format == AP_DSTRESOURCE_720P_25) || (SensorData_CH1_Format == AP_DSTRESOURCE_720P_30) \
	  ||(SensorData_CH1_Format == AP_DSTRESOURCE_720P_50)||(SensorData_CH1_Format == AP_DSTRESOURCE_720P_60)))
	{
		return AP_SETTING_VIDEORESOLUTION_1080P_720P_30;
	}
	else if(((SensorData_CH0_Format == AP_DSTRESOURCE_1080P_25)||(SensorData_CH0_Format == AP_DSTRESOURCE_1080P_30))\
	  && (SensorData_CH1_Format == AP_DSTRESOURCE_CVBS_N))
	{
		return AP_SETTING_VIDEORESOLUTION_1080P_NSTL_30;
  	}
	else if(((SensorData_CH0_Format == AP_DSTRESOURCE_1080P_25)||(SensorData_CH0_Format == AP_DSTRESOURCE_1080P_30))\
	  && (SensorData_CH1_Format == AP_DSTRESOURCE_CVBS_P))
	{
		return AP_SETTING_VIDEORESOLUTION_1080P_PAL_30;
  	}
	else if(((SensorData_CH0_Format == AP_DSTRESOURCE_720P_25) || (SensorData_CH0_Format == AP_DSTRESOURCE_720P_30) \
	  ||(SensorData_CH0_Format == AP_DSTRESOURCE_720P_50)||(SensorData_CH0_Format == AP_DSTRESOURCE_720P_60))\
	  && ((SensorData_CH1_Format == AP_DSTRESOURCE_720P_25) || (SensorData_CH1_Format == AP_DSTRESOURCE_720P_30) \
	  ||(SensorData_CH1_Format == AP_DSTRESOURCE_720P_50)||(SensorData_CH1_Format == AP_DSTRESOURCE_720P_60)))
	{
		return AP_SETTING_VIDEORESOLUTION_720P_720P_30;
  	}
	else if(((SensorData_CH0_Format == AP_DSTRESOURCE_720P_25) || (SensorData_CH0_Format == AP_DSTRESOURCE_720P_30) \
	  ||(SensorData_CH0_Format == AP_DSTRESOURCE_720P_50)||(SensorData_CH0_Format == AP_DSTRESOURCE_720P_60)) \
	  && (SensorData_CH1_Format == AP_DSTRESOURCE_CVBS_N))
	{
		return AP_SETTING_VIDEORESOLUTION_720P_NSTL_30;
  	}
	else if(((SensorData_CH0_Format == AP_DSTRESOURCE_720P_25) || (SensorData_CH0_Format == AP_DSTRESOURCE_720P_30) \
	  ||(SensorData_CH0_Format == AP_DSTRESOURCE_720P_50)||(SensorData_CH0_Format == AP_DSTRESOURCE_720P_60)) \
	  && (SensorData_CH1_Format == AP_DSTRESOURCE_CVBS_P))
	{
		return AP_SETTING_VIDEORESOLUTION_720P_PAL_30;
  	}
	else if((SensorData_CH0_Format == AP_DSTRESOURCE_CVBS_N) \
	  && ((SensorData_CH1_Format == AP_DSTRESOURCE_720P_25) || (SensorData_CH1_Format == AP_DSTRESOURCE_720P_30) \
	  ||(SensorData_CH1_Format == AP_DSTRESOURCE_720P_50)||(SensorData_CH1_Format == AP_DSTRESOURCE_720P_60)))
	{
		return AP_SETTING_VIDEORESOLUTION_NSTL_720P_30;
  	}
	else if((SensorData_CH0_Format == AP_DSTRESOURCE_CVBS_N)&&(SensorData_CH1_Format == AP_DSTRESOURCE_CVBS_N))
	{
		return AP_SETTING_VIDEORESOLUTION_NSTL_NSTL_30;
	}
	else if((SensorData_CH0_Format == AP_DSTRESOURCE_CVBS_N)&&(SensorData_CH1_Format == AP_DSTRESOURCE_CVBS_P))
	{
		return AP_SETTING_VIDEORESOLUTION_NSTL_PAL_30;
	}
	else if((SensorData_CH0_Format == AP_DSTRESOURCE_CVBS_P) \
	  && ((SensorData_CH1_Format == AP_DSTRESOURCE_720P_25) || (SensorData_CH1_Format == AP_DSTRESOURCE_720P_30) \
	  ||(SensorData_CH1_Format == AP_DSTRESOURCE_720P_50)||(SensorData_CH1_Format == AP_DSTRESOURCE_720P_60)))
	{
		return AP_SETTING_VIDEORESOLUTION_PAL_720P_30;
  	}
	else if((SensorData_CH0_Format == AP_DSTRESOURCE_CVBS_P)&&(SensorData_CH1_Format == AP_DSTRESOURCE_CVBS_N))
	{
		return AP_SETTING_VIDEORESOLUTION_PAL_NSTL_30;
	}
	else if((SensorData_CH0_Format == AP_DSTRESOURCE_CVBS_P)&&(SensorData_CH1_Format == AP_DSTRESOURCE_CVBS_P))
	{
		return AP_SETTING_VIDEORESOLUTION_PAL_PAL_30;
	}
	else if(((SensorData_CH0_Format == AP_DSTRESOURCE_1080P_25)||(SensorData_CH0_Format == AP_DSTRESOURCE_1080P_30)) \
	  && (SensorData_CH1_Format == 0))
	{
		return AP_SETTING_VIDEORESOLUTION_1080P_30;
  	}
	else if(((SensorData_CH0_Format == AP_DSTRESOURCE_720P_25) || (SensorData_CH0_Format == AP_DSTRESOURCE_720P_30) \
	  ||(SensorData_CH0_Format == AP_DSTRESOURCE_720P_50)||(SensorData_CH0_Format == AP_DSTRESOURCE_720P_60)) \
	  && (SensorData_CH1_Format == 0))
	{
		return AP_SETTING_VIDEORESOLUTION_720P_30;
  	}
	return 0; //确实默认为1080P
#endif

	if( ((SensorData_CH0_Format == AP_DSTRESOURCE_720P_25) && (SensorData_CH1_Format == AP_DSTRESOURCE_720P_25)) )
	{
		return AP_SETTING_VIDEORESOLUTION_720P_720P_25;
	}
	else if( (SensorData_CH0_Format == AP_DSTRESOURCE_720P_30) && (SensorData_CH1_Format == AP_DSTRESOURCE_720P_30) )
	{
		return AP_SETTING_VIDEORESOLUTION_720P_720P_30;
	}
	else
	{
		return AP_SETTING_VIDEORESOLUTION_720P_720P_25;
	}
}


void sensor_test (void)
{
	OS_Delay (1000);

	XMSYS_SensorCaptureStart ();
	while (1)
	{
		OS_Delay (1);
	}
}

#define MCTL_ALPMR_AUTOPD						(1 << 26)	//	Enable automatic Power-Down entry
															 //	after (LPPERIOD0 + LPPERIOD1) period of time.
// 进入关键区, 保护7116初始化过程
void enter_region_to_protect_7116_setup (void)
{
	OS_Use (&Ark7116ProtectSema);
	// 关闭DDR PreCharge PowerDown Feature, 阻止大电流情况发生
	old_rMCTL_ALPMR = rMCTL_ALPMR;
	rMCTL_ALPMR &= ~MCTL_ALPMR_AUTOPD;
}
void leave_region_to_protect_7116_setup (void)
{
	rMCTL_ALPMR = old_rMCTL_ALPMR;
	OS_Unuse (&Ark7116ProtectSema);
}

void XMSYS_SensorResetUserData (XMSYSSENSORPACKET *packet)
{
	packet->isp_user_size = 0;
	packet->isp_user_data = NULL;
	packet->isp_user_buff[0] = 0;
}

void XMSYS_SensorSyncUserData (XMSYSSENSORPACKET *packet)
{
	if(packet && packet->isp_user_data && packet->isp_user_size)
	{
		dma_flush_range ((unsigned int)packet->isp_user_data, packet->isp_user_size + 4 + 4 + (unsigned int)packet->isp_user_data);
	}
}

// 将私有数据加入到sensor私有数据缓存中(data!=NULL) 或者 仅分配空间用于后续的私有数据存储(user_data==NULL)
// 返回值指向存放私有数据的缓冲区地址，失败返回NULL
//		id				帧私有数据类型
//		user_data	私有数据地址
//		user_size	私有数据字节大小
char * XMSYS_SensorInsertUserDataIntoPacket (XMSYSSENSORPACKET *packet, unsigned int id, char *user_data, int user_size)
{
	int size;
	char *buff = NULL;
	if(packet == NULL)
		return NULL;
	if(id >= XM_PACKET_USER_DATA_ID_COUNT)
		return NULL;
	if(user_size == 0)
		return NULL;
	XM_lock();
	// 数组保留首8个字节，4字节数据长度，4字节版本
	size = (XM_PACKET_USER_DATA_SIZE - 8) - packet->isp_user_size;
	if(size >= (user_size + 4 + 4))
	{
		// 私有数据ID(4字节) + 私有数据长度(user_size, 4字节) + 私有数据内容(长度为user_size字节)
		// ID字段
		buff = ((char *)packet->isp_user_buff) + 8 + packet->isp_user_size;
		set_long(buff,id);
		buff += 4;
		set_long(buff,user_size);
		buff += 4;
		if(user_data)
			memcpy (buff, user_data, user_size);
		size -= (user_size + 8);
		packet->isp_user_size += user_size + 8;
		// 数据长度
		set_long(((char *)packet->isp_user_buff), packet->isp_user_size + 8);
		// 数据版本，客户端根据版本号解析数据内容
		unsigned int ver 	= ('I' <<  0)
							| ('E' <<  8) 
							| ('2' << 16)		// big version '2', 区别于老的版本'1'
							| ('0' << 24)		/// little version
							;
		set_long (((char *)packet->isp_user_buff) + 4, ver);
		packet->isp_user_data = (char *)packet->isp_user_buff;
	}
	XM_unlock();
	return buff;
}


XMSYSTEMTIME  localtimeinfor;           //本地时间
unsigned char get_localtimefor;

// 私有数据解析
void XMSYS_DecodeUserData (char *isp_data, int isp_size )
{
   //char data[512];
	char *buffer;
	char *data;
   int i,j;
	char sensor_name[9];
	unsigned int ver;

   unsigned int data0,data1,data2,data3;
	unsigned int VER1 	= ('I' <<  0)
							| ('E' <<  8) 
							| ('1' << 16)		// big version
							| ('0' << 24)		/// little version
							;
	unsigned int VER2 	= ('I' <<  0)
							| ('E' <<  8) 
							| ('2' << 16)		// big version
							| ('0' << 24)		/// little version
							;

	ver = get_long (isp_data + 4);
	if(ver == VER1)
	{
		// AE & ISP私有数据
	}
	else if(ver == VER2)
	{
		isp_data += 8;
		isp_size -= 8;
		// 遍历每个ID块
		while(isp_size > 0)
		{
			// 私有数据ID(4字节) + 私有数据长度(user_size, 4字节) + 私有数据内容(长度为user_size字节)
			// get id
			unsigned int id, user_size;
			id = get_long (isp_data);
			user_size = get_long(isp_data+4);
			isp_size -= 8;
			isp_data += 8;
			if(id == XM_PACKET_USER_DATA_ID_EXP)	//
			{
				isp_data += user_size;
			}
			else if(id == XM_PACKET_USER_DATA_ID_ISP)
			{
				isp_data += user_size;
			}
			else if(id == XM_PACKET_USER_DATA_ID_DATETIME)
			{
				memcpy (&localtimeinfor, isp_data, user_size);
				get_localtimefor = 1;
				//XM_printf ("Time = %02d:%02d:%02d:%03d\n", localtimeinfor.bHour, localtimeinfor.bMinute, localtimeinfor.bSecond, localtimeinfor.wMilliSecond);
				isp_data += user_size;
			}
            #if 0
			else if(id == XM_PACKET_USER_DATA_ID_GPS)
			{
			    memcpy (gpsinfor_altitude, isp_data, sizeof(gpsinfor_altitude));
				isp_data += sizeof(gpsinfor_altitude);
				memcpy (gpsinfor_latitude, isp_data, sizeof(gpsinfor_latitude));
				isp_data += sizeof(gpsinfor_latitude);
				memcpy (gpsinfor_speed, isp_data, sizeof(gpsinfor_speed));
				get_gpsinfor = 1;
				isp_data += user_size;
			}
            #endif
            
			else
			{
				isp_data += user_size;
			}

			isp_size -= user_size;
		}
	}
}



