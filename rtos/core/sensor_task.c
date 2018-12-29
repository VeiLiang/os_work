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
// ���֧��4������ͷSensor

// �쳣���
#define	SENSOR_FATAL		XM_printf
// �������
#define	SENSOR_PRINT		XM_printf

#define	USE_PRE_PROCESS

//#define	INCLUDE_ISP_DEBUG	// ISP����

#define	XMSYS_SENSOR_FRAME_COUNT			4	// ISPӲ������FIFOΪ4

static OS_TASK TCB_SensorTask;
__no_init static OS_STACKPTR int StackSensorTask[XMSYS_SENSOR_TASK_STACK_SIZE/4];          /* Task stacks */


// �������
static XMSYSSENSORPACKET SensorPacketPool[XMSYS_SENSOR_COUNT][XMSYS_SENSOR_FRAME_COUNT];
static queue_s	SensorPacketData[XMSYS_SENSOR_COUNT];		// �Ѳɼ�Sensor������
static queue_s SensorPacketPrep[XMSYS_SENSOR_COUNT];		// �����Ԥ����İ�����(����ʱ���/����Logo��־����Ϣ)
static unsigned int data_packet_discard_count[XMSYS_SENSOR_COUNT];
static unsigned int prep_packet_discard_count[XMSYS_SENSOR_COUNT];
//static unsigned int data_packet_total_count;
static unsigned int data_packet_count[XMSYS_SENSOR_COUNT];

static XMSYSSENSORPACKET *SensorCurrentDataPacket[XMSYS_SENSOR_COUNT];		// ��ǰ����

//#define	DEADLINE_TICKET_ENABLE

#ifdef DEADLINE_TICKET_ENABLE
static volatile unsigned int deadline_ticket_to_remove;		// �Ѳɼ������е�sensor֡����ʱ����һ��deadʱ��, ������ʱ��, ֡���붪��.
																			//		һ�����ʱ������Ϊ��֡ʱ��
#endif

// Sensor��Դ���ʻ����ź���
static OS_RSEMA		SensorSema[XMSYS_SENSOR_COUNT];	

static OS_RSEMA				Ark7116ProtectSema;		// ����7116��ʼ��
static unsigned int			old_rMCTL_ALPMR;			// DDR AutoPD����
static OS_MAILBOX sensor_mailbox;		// һ�ֽ����䡣
static char sensor_mailbuf;				// һ���ֽڵ�����

#define	MAX_FRAME_COUNT_TO_DISCARD_TO_REMOVE_FLASH	3	// 3֡

static int frames_to_discard_to_remove_flash;		// ����sensor��, Ϊ������˸ʱ��Ҫ�ӵ���֡��

// Sensor�ߴ綨��

static const XMSIZE	SensorSize[XMSYS_SENSOR_COUNT] = {
	{ XMSYS_CH0_SENSOR_FRAME_WIDTH,	XMSYS_CH0_SENSOR_FRAME_HEIGHT },
#if XMSYS_SENSOR_COUNT > 1
	{ XMSYS_CH1_SENSOR_FRAME_WIDTH,	XMSYS_CH1_SENSOR_FRAME_HEIGHT },
#endif
};


static int const *SensorBuffer[XMSYS_SENSOR_COUNT][XMSYS_SENSOR_FRAME_COUNT];
static char *arkn141_isp_user_data;				// cache line����ĵ�ַ
static char *arkn141_isp_user_data_base;		// ��ַ

static int SensorCaptureEnable;		// sensor״̬, 1 ���� 0 ֹͣ


// ��Ӳ��FIFO�����в���ָ���ĵ�ַ
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


// ��sensor�жϳ������
// ֪ͨһ���µ�����֡�Ѳ���, ����֡ѹ�뵽YUV����֡����
// ����ʱ�ж���Ҫ�ر�
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
		// ������˸�ӵ���֡
		set_frame_ready (channel, frame_id);
		frames_to_discard_to_remove_flash --;
		return;
	}

	if(!queue_empty(&SensorPacketData[channel]))
	{
		// �ǿ�
		// ��δ����İ�����
#ifdef DEADLINE_TICKET_ENABLE
		XMSYS_SensorCheckDeadLineTicket (channel); // ����Ƿ��Ѵ���deadlineʱ��, ������, ɾ��
		deadline_ticket_to_remove = XM_GetTickCount() + 10;	// ����deadlineʱ��
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

	// �������뵽YUV������������
	packet = &SensorPacketPool[channel][frame_id];

	// ���Ƶ�ǰ��Ӳ���ع���Ϣ
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
	if(packet->prep_ref)		// �˴�����ȱ��
	{
		XM_printf ("XMSYS_SensorNotifyDataFrame error, illegal prep_ref count(0x%08x)\n", packet->prep_ref);
	}
	packet->data_ref ++;
	queue_insert ((queue_s *)packet, &SensorPacketData[channel]);
	// 20180213 ʵʱ��Ƶ���²�����codec�߳�����Ӱ��
	SensorCurrentDataPacket[channel] = packet;

	data_packet_count[channel] ++;
	OS_SignalEvent((char)(1 << channel), &TCB_SensorTask); /* ֪ͨ�¼� */
}

#ifdef DEADLINE_TICKET_ENABLE
// ���deadlineʱ���Ƿ�����.
// ������, ����Ƿ��ѳ�ʱ.
// ����ʱ, ���sensor���ݶ����Ƿ���ڶ���(2֡)��֡. ɾ�����ݶ����ж����sensor֡(2֡��ɾ��1֡, 1֡��ɾ��)
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
				// ��������2��sensor֡
				// ��δ����İ�����
				// ������ͷ����֡ɾ��
				data_packet_discard_count ++;
				packet = (XMSYSSENSORPACKET *)queue_delete_next (&SensorPacketData[channel]);
				set_frame_ready (channel, packet->frame_id);
			}
			deadline_ticket_to_remove = 0;		// ɾ��deadlineʱ��
		}
	}
}
#endif

// ����һ������H264�������Ƶ֡
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
	// ֻ����Ԥ������H264���빲��һ���ź���, ��ֻ��һ֡������Ԥ������߱���
	// ����, Ӳ�������ܹ�4֡, 2֡��FIFO�ɼ�����, 1֡������Ԥ�������, 1֡��Ԥ������߱���
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
	
	// 20181008 ������������Ƶ�ļ�д��busyʱ������Ƶ����
	// FIFO�ǿ�ʱ�����ź���, ��ֹԤ����ִ��
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

// ����Ƿ����sensor���ݰ���Ҫ����
// 1 ��ʾ����һ��ͨ���ϴ�������һ�����ݰ���Ҫ����
// 0 û�����ݰ���Ҫ����
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
		
	// �������²��뵽Ӳ������
	assert (packet->frame_id < 4);
	
	XM_lock();
	//XM_printf ("psub 0x%08x\n", packet);
	packet->prep_ref --;
	set_frame_ready (channel, packet->frame_id);
	XM_unlock();
	
#ifdef USE_PRE_PROCESS
	OS_Use(&SensorSema[channel]);

#ifdef DEADLINE_TICKET_ENABLE
	// ����Ƿ����δ�����sensor֡. ������, ֪ͨԤ�����߳�
	if(!queue_empty ((queue_s *)&SensorPacketData[channel]))
	{
		OS_SignalEvent((char)(1 << channel), &TCB_SensorTask); /* ֪ͨ�¼� */
	}
#endif	
	
	// ����ź�������
	// Ԥ������H264���빲��һ���ź���
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
	
	// �������²��뵽Ӳ������
	assert (packet->frame_id < 4);
	
	XM_lock();
	//XM_printf ("psub 0x%08x\n", packet);
	packet->prep_ref --;
	set_frame_ready (channel, packet->frame_id);
	XM_unlock();
	
}

// һ���������½�sensor֡Ͷ�ݵ�Ԥ�������, �ṩ��H264������ʹ��
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

	// ���뵽Ԥ�������β��
	queue_insert ((queue_s *)packet, (queue_s *)&SensorPacketPrep[channel]);
		
	// ����ź�������
	// Ԥ������H264���빲��һ���ź���
	OS_Unuse(&SensorSema[channel]); /* Make sure nobody else uses */	
#else
	// �������²��뵽Ӳ������
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

// ��ȡָ��ͨ�����µ�sensor֡
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
	queue_initialize (&SensorPacketData[0]);		// ԭʼ������
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
	queue_initialize (&SensorPacketData[1]);		// ԭʼ������
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
	queue_initialize (&SensorPacketData[2]);		// ԭʼ������
	queue_initialize (&SensorPacketPrep[2]);
	OS_Unuse(&SensorSema[2]); 
}



void SensorInitCaptureFIFO (void)
{
	int i, j;
	unsigned int user_data_size;

	// 32�ֽڶ���(Cache Line aligned), �Ż�������memcpy����
	#if 0
	user_data_size = isp_get_isp_data_size() + isp_get_exp_data_size() + 4 + 4 + 8;
	user_data_size = (user_data_size + 0x1F) & (~0x1F);
	#endif
	
	// ISPͨ��(ͨ�����0)����
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
	queue_initialize (&SensorPacketData[0]);		// ԭʼ������
	queue_initialize (&SensorPacketPrep[0]);
	OS_Unuse(&SensorSema[0]);
	
	// ��2·CVBS
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
	queue_initialize (&SensorPacketData[1]);		// ԭʼ������
	queue_initialize (&SensorPacketPrep[1]);
	OS_Unuse(&SensorSema[1]);
/*
	//����·
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
	queue_initialize (&SensorPacketData[2]);		// ԭʼ������
	queue_initialize (&SensorPacketPrep[2]);
	OS_Unuse(&SensorSema[2]);
*/    
	#if 0
	for (i = 0; i < XMSYS_SENSOR_COUNT; i ++)
	{
		queue_initialize (&SensorPacketData[i]);		// ԭʼ������
		queue_initialize (&SensorPacketPrep[i]);
    }
    #endif
}

//  ����Sensor Capture������
void SensorConfigCaptureFIFO (void)
{
	int i, j;
	// ����Sensor Capture������
	for (i = 0; i < XMSYS_SENSOR_COUNT; i ++)
	{
		OS_Use(&SensorSema[i]); /* Make sure nobody else uses */
		for (j = 0; j < XMSYS_SENSOR_FRAME_COUNT; j++)
		{
			dma_inv_range ((u32)SensorPacketPool[i][j].buffer,
								((u32)SensorPacketPool[i][j].buffer) + SensorPacketPool[i][j].size);
			// ���뵽Sensor�ɼ�����
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
		// ��YUV���������ȡ���ݰ�
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
						
		//��ͨ��û��ready��
		if(packet == NULL)
			break;
		
#if 1
		//¼����ƵǶ��ʱ��ˮӡ
		//XM_printf(">>>>>>>>AP_GetMenuItem(APPMENUITEM_TIME_STAMP):%d\r\n", AP_GetMenuItem(APPMENUITEM_TIME_STAMP));
		if(packet && (AP_GetMenuItem(APPMENUITEM_TIME_STAMP) == AP_SETTING_VIDEO_TIMESTAMP_ON))
		{
			void embed_local_time_into_video_frame(
													unsigned int width,			// ��Ƶ���
													unsigned int height,		// ��Ƶ�߶�
													unsigned int stride,		// Y_UV���ֽڳ���
													char *data					// Y_UV������
													);
					
			unsigned int vaddr = page_addr_to_vaddr((unsigned int)packet->buffer);
			unsigned char *virtual_address = (unsigned char *)vaddr;
			// Ƕ��ʱ�����Ϣ
			// ʹ�����ַ(Cache)�Ż��㷨
			embed_local_time_into_video_frame (packet->width, packet->height, 
																  packet->width, 
																  //packet->buffer
																  (char *)virtual_address
																);		
					
		}
#endif		

		// 1·601���������Ϊ¼��23֡(������Դ����)
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
		
		// ����ʱ���/����Logo��־��Ϣ, ��������뵽Ԥ��������
		// ���Ԥ��������Ƿ������δ����İ�. ������, �����Ԥ���������ɾ��,������ѹ�뵽Ӳ������
		if(!queue_empty(&SensorPacketPrep[channel]))
		{
			// ��鲢ɾ��
			XMSYSSENSORPACKET *packet_to_delete = (XMSYSSENSORPACKET *)queue_delete_next ((queue_s *)&SensorPacketPrep[channel]);
			if(packet_to_delete->data_ref != 0 || packet_to_delete->prep_ref != 1)
			{
				XM_printf ("packet (0x%08x) 's ref illegal\n", (unsigned int)packet);
			}
			prep_packet_discard_count[channel] ++;
			// ����ѹ�뵽Ӳ������
			XMSYS_SensorReturnPacket (channel, packet_to_delete);
		}

		// ���뵽Ԥ�������β��
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
		// ����ISP������Ϣ
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
			// ��isp���ݸ��Ƶ���������ݿռ�
			isp_get_isp_data (isp_data_buff, isp_get_isp_data_size());
		}
		
		// ����ʱ����Ϣ
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
		// �ȴ��ⲿ�¼�
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
			// ����Sensor�ɼ�
			// ����ͨ��0��Capture

			// 20170609
			// ���sensor�Ƿ�������.
			if(SensorCaptureEnable == 0)
			{
				XM_printf ("SENSOR_CAPTURE_START\n");
				// ��λSensor������
				SensorInitCaptureFIFO ();
				//SensorInitCaptureFIFO ();
				frames_to_discard_to_remove_flash = MAX_FRAME_COUNT_TO_DISCARD_TO_REMOVE_FLASH;
				//OS_Delay (100);
				SensorCaptureEnable = 1;
#if 1
				// ͨ��0 (ISP)
				//XMSYS_SensorStart ();
				Notification_Itu601_Isp_Scaler_Init();//����֪ͨ��601 �����������ݶ���
				rxchip_itu656_PlugIn();
#else//XMSYS_ITU656_IN_SUPPORT
				// ͨ��1 (ITU656 IN)
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
			// ֹͣSensor�ɼ�
			XM_printf ("SENSOR_CAPTURE_STOP\n");
			SensorData_Stop_Rec = TRUE;
			// 20170703 ����ISP��Sensor task֮�以��, ����ʱ��ռ��SensorSema��Դ
			// OS_Use(&SensorSema); /* Make sure nobody else uses */
			// ��ֹͣISP scalar�� ��ֹͣISP��isp scalar����ISPʱ��
			//xm_itu601_scalar_stop ();
			//XMSYS_SensorStop ();
			SensorCaptureEnable = 0;
			// OS_Unuse(&SensorSema); /* Make sure nobody else uses */
			//ֹͣ����601 ����
			Notification_Itu601_Isp_Scaler_Stop();
			rxchip_itu656_PlugOut();
			
			XMSYS_SensorResetCurrentPacket(0);
			XMSYS_SensorResetCurrentPacket(1);
			//XMSYS_SensorResetCurrentPacket(2);

			// ��λSensor�̵߳������ⲿ�¼�
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


// Sensor�����ʼ��
void XMSYS_SensorInit (void)
{
	int i, j;
	unsigned int user_data_size, user_data_size_base;
	// �����߳�ͨ������
	OS_CREATEMB (&sensor_mailbox, 1, 1, &sensor_mailbuf);

	// ������������ź���
	for (i = 0; i < XMSYS_SENSOR_COUNT; i ++)
		OS_CREATERSEMA(&SensorSema[i]); /* Creates resource semaphore */

	OS_CREATERSEMA(&Ark7116ProtectSema);

#ifndef INCLUDE_ISP_DEBUG
	arkn141_isp_user_data = NULL;
#else
	user_data_size = 4 + 4 + 8 + isp_get_isp_data_size() + isp_get_exp_data_size();
	user_data_size_base = (user_data_size + 0x1F) & (~0x1F);		// ����32�ֽڶ��䲢ռ��������cache line(Cortex A5 Cache line size);
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

	// SensorӲ����ʼ��
	//XMSYS_SensorHardwareInit ();

	//SENSOR_PRINT ("XMSYS_SensorStart\n");

	//SensorConfigCaptureFIFO ();

	// ����Sensor�̲߳�����
	OS_CREATETASK(&TCB_SensorTask, "SensorTask", XMSYS_SensorTask, XMSYS_SENSOR_TASK_PRIORITY, StackSensorTask);

	// ����sensor�ɼ�
	//XMSYS_SensorStart ();
}

// Sensor�������
void XMSYS_SensorExit (void)
{
	XMSYS_SensorCaptureStop ();
	OS_SignalEvent(XMSYS_SENSOR_CAPTURE_EXIT, &TCB_SensorTask); /* ֪ͨ�¼� */
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

// ����Sensor�ɼ�
void XMSYS_SensorCaptureStart (void)
{
	char ret = 0;
	OS_SignalEvent(XMSYS_SENSOR_CAPTURE_START, &TCB_SensorTask); /* ֪ͨ�¼� */
	// �ȴ�Sensor����Ӧ��
	// OS_GetMail1 (&sensor_mailbox, &ret);
	if(OS_GetMailTimed (&sensor_mailbox, &ret, 1000) == 1)
	{
		XM_printf ("timeout to SensorCaptureStart\n");
	}
}

// ֹͣSensor�ɼ�
void XMSYS_SensorCaptureStop (void)
{
	char ret = 0;
	OS_SignalEvent(XMSYS_SENSOR_CAPTURE_STOP, &TCB_SensorTask); /* ֪ͨ�¼� */
	// �ȴ�Sensor����Ӧ��
	// OS_GetMail1 (&sensor_mailbox, &ret);
	if(OS_GetMailTimed (&sensor_mailbox, &ret, 1000) == 1)
	{
		XM_printf ("timeout to SensorCaptureStop\n");
	}
}

// sensor�쳣ʱ, ��λ��Ӧ��ͨ��
int XMSYS_SensorCaptureReset (int channel)
{
	int i;
	int busy;
	int ret = -1;
	unsigned int ticket_to_timeout;
	if(channel >= XMSYS_SENSOR_COUNT)
		return -1;
	
	// ���Ѳɼ�Sensor�����м�Ԥ������еİ��ͷ�
	OS_Use(&SensorSema[channel]); /* Make sure nobody else uses */
	XM_lock ();
	// �Ѳɼ�Sensor�����а��ͷ�
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
	// Ԥ������а��ͷ�
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
	// �ȴ�Ԥ������ͷ�(����)
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
	return 0; //ȷʵĬ��Ϊ1080P
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
// ����ؼ���, ����7116��ʼ������
void enter_region_to_protect_7116_setup (void)
{
	OS_Use (&Ark7116ProtectSema);
	// �ر�DDR PreCharge PowerDown Feature, ��ֹ������������
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

// ��˽�����ݼ��뵽sensor˽�����ݻ�����(data!=NULL) ���� ������ռ����ں�����˽�����ݴ洢(user_data==NULL)
// ����ֵָ����˽�����ݵĻ�������ַ��ʧ�ܷ���NULL
//		id				֡˽����������
//		user_data	˽�����ݵ�ַ
//		user_size	˽�������ֽڴ�С
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
	// ���鱣����8���ֽڣ�4�ֽ����ݳ��ȣ�4�ֽڰ汾
	size = (XM_PACKET_USER_DATA_SIZE - 8) - packet->isp_user_size;
	if(size >= (user_size + 4 + 4))
	{
		// ˽������ID(4�ֽ�) + ˽�����ݳ���(user_size, 4�ֽ�) + ˽����������(����Ϊuser_size�ֽ�)
		// ID�ֶ�
		buff = ((char *)packet->isp_user_buff) + 8 + packet->isp_user_size;
		set_long(buff,id);
		buff += 4;
		set_long(buff,user_size);
		buff += 4;
		if(user_data)
			memcpy (buff, user_data, user_size);
		size -= (user_size + 8);
		packet->isp_user_size += user_size + 8;
		// ���ݳ���
		set_long(((char *)packet->isp_user_buff), packet->isp_user_size + 8);
		// ���ݰ汾���ͻ��˸��ݰ汾�Ž�����������
		unsigned int ver 	= ('I' <<  0)
							| ('E' <<  8) 
							| ('2' << 16)		// big version '2', �������ϵİ汾'1'
							| ('0' << 24)		/// little version
							;
		set_long (((char *)packet->isp_user_buff) + 4, ver);
		packet->isp_user_data = (char *)packet->isp_user_buff;
	}
	XM_unlock();
	return buff;
}


XMSYSTEMTIME  localtimeinfor;           //����ʱ��
unsigned char get_localtimefor;

// ˽�����ݽ���
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
		// AE & ISP˽������
	}
	else if(ver == VER2)
	{
		isp_data += 8;
		isp_size -= 8;
		// ����ÿ��ID��
		while(isp_size > 0)
		{
			// ˽������ID(4�ֽ�) + ˽�����ݳ���(user_size, 4�ֽ�) + ˽����������(����Ϊuser_size�ֽ�)
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



