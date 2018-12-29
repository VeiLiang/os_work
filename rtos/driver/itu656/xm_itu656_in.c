// YUV sensor(ITU601/ITU656�����ʽ��YUV sensor)
//
#include "hardware.h"
#include <xm_printf.h>

#include "RTOS.h"
#include "fs.h"
#include "xm_dev.h"
#include "xm_user.h"
#include "xm_key.h"
#include "irqs.h"
#include "xm_core.h"
#include "xm_itu656_in.h"
#include <assert.h>
#include "xm_queue.h"
#include "xm_printf.h"
#include "arkn141_scale.h"
#include "arkn141_isp.h"
#include "app_head.h"
#include "deinterlace.h"
#include "rxchip.h"

//#define ITU656_INPUT

#ifdef TULV_BB_TEST
#define	SIMULATE_7116
#endif

#define	ITU656_IN_IDLE		0
#define	ITU656_IN_RUN		1

#define	ITU656_ID		0x36353649		// "I656"

#define	ITU656_IN_MODULE_STATE_IDLE			0		// ����
#define	ITU656_IN_MODULE_STATE_DETECTING	1		// 656 in����Դ���ģʽ
#define	ITU656_IN_MODULE_STATE_DETERMINED	2		// 656 in����Դ���ж�ģʽ(�ѻ�ȡ֡���/�߶���Ϣ)
#define	ITU656_IN_MODULE_STATE_CAPTURE		3		// 656 in���ݰ�ץȡģʽ

#define	ITU656IN_EVENT_STOP			0x01		// ITU656 IN STOP
#define	ITU656IN_EVENT_START		0x02		// 7116������,����������ȷ����

#define	ITU656IN_EVENT_DEINTERLACE_TICKET			0x04		// ��ʱ��
#define ITU656IN_EVENT_DEINTERLACE_BACK				0x08

int itu656_in_setup (unsigned int in_width, unsigned int in_height);
static void itu656_in_reset_fifo (void);


// ��itu656 in �жϷ������, ֪ͨһ��֡�ѽ������, ����ȡ��
static void itu656_in_frame_ready (void);
static int itu656_in_get_ready_frame_id (void);


static u32_t itu656_in_state;
static volatile unsigned int itu656_in_stop = 0;
static u32_t itu656_in_width;
static u32_t itu656_in_height;

static OS_RSEMA				Itu656InSema;
static OS_TASK TCB_Itu656In;                        /* Task-control-blocks */
static OS_TIMER itu656_in_Timer;		// ��ʱ��

static XMSYSSENSORPACKET itu656_in_unit[ITU656_IN_FRAME_COUNT];		// ֡��Ԫ
static queue_s itu656_in_free;			// ���ж���
static queue_s itu656_in_fifo;			// �ŶӶ���, �ȴ�����֡����


#pragma data_alignment=32
__no_init OS_STACKPTR int Stack_Itu656In[XMSYS_ITU656_IN_TASK_STACK_SIZE/4];          /* Task stacks */



static int skip_frames_to_on_lcd = 1;		// ��һ�ο���ʱʹ��

static int itu656_do_disable;			// 1 ��ʾ����ִ��disable����

static unsigned int monitor_itu656_overflow_signal;			// �Ƿ��ⳡ��ʼ�ź�
#define	OVERFLOW_FLAG		0x55AA00FF
static unsigned int *overflow_flag_address;		// �������õı���ֶεĵ�ַ
static unsigned int *next_overflow_flag_address;		// �������õı���ֶεĵ�ַ
static unsigned int restore_total_pix;

static unsigned int itu656_in_sensor_channel = 1;		// ITU656 IN ��Ӧ��sensorͨ����

void itu656in_SelPad( void )
{
	//ʹ�� pad:itu_b 
	/* ITU656 padlist : ʹ��9����: itu_b_clk + ������ITU_B:0-7  */
	unsigned int val;
	XM_lock ();
	val = rSYS_PAD_CTRL01 ;
	val &= ~((0x7<<15)|(0x7<<24)|(0x7<<27));
	val |=((1<<15)|(1<<24)|(1<<27)); // clk  din0 din1
	rSYS_PAD_CTRL01 = val ;
	val = rSYS_PAD_CTRL02;
	val &= 0xfffc0000;
	// =  0b 00 1001 0010 0100 1001 (from din2 to din7) 6 pin
	val |= 0x9249;  
	rSYS_PAD_CTRL02 = val;
	XM_unlock ();
}

void itu601in_SelPad (void)
{
	unsigned int val;
	XM_lock ();
	val = rSYS_PAD_CTRL01;
	val &=0xc0007fff;
	//     ituclk| vsync | hsync | din0  | din1
	val |=(1<<15)|(1<<18)|(1<<21)|(1<<24)|(1<<27);
	rSYS_PAD_CTRL01 = val;
   
	val = rSYS_PAD_CTRL02;
	val &=0xfffc0000;
	//     din2 | din3 | din4 | din5 | din6  | din7
	val |=(1<<0)|(1<<3)|(1<<6)|(1<<9)|(1<<12)|(1<<15);
	rSYS_PAD_CTRL02 = val;
 	XM_unlock ();
}


static unsigned int next_monitor_ticket = 0;
static unsigned int overflow_count = 0;
static unsigned int next_overflow_count = 0;
void itu601_in_field_start_monitor (void)
{
	if(monitor_itu656_overflow_signal && restore_total_pix == 0)
	{
		if(next_monitor_ticket && XM_GetTickCount() <= next_monitor_ticket)
			return;
		
		next_monitor_ticket = XM_GetTickCount () + 4;
		
		// ��һ��������λ��
		if(overflow_flag_address && *overflow_flag_address != OVERFLOW_FLAG )
		{
			// ���ܵ������̣�������ǰ���쳣���չ���
			unsigned int rdata = rITU656IN_ISR;
			restore_total_pix = rITU656IN_TOTAL_PIX;
			rITU656IN_TOTAL_PIX = 16;

			*overflow_flag_address = OVERFLOW_FLAG;
			overflow_count ++;
			XM_printf ("601in ov count %d, %d\n",overflow_count, next_overflow_count);
		}
		
		// �ڶ���������λ��
		if(next_overflow_flag_address && *next_overflow_flag_address != OVERFLOW_FLAG)
		{
			next_overflow_count ++;
			*next_overflow_flag_address = OVERFLOW_FLAG;
			XM_printf ("601in protect ov count %d\n", next_overflow_count);
		}
	}
}


static void ov_post_process (void)
{
	// ����Ƿ������������ĺ�������
	if(restore_total_pix)
	{
		// �ָ��ܵ�������������
		rITU656IN_TOTAL_PIX = restore_total_pix;
		restore_total_pix = 0;
		//XM_printf ("restore_total_pix\n");
	}
	
}

// ֡ģʽ�жϴ���
static void itu601in_int_framehandler (void)
{
	unsigned int itu656_int_mask ;
	unsigned int rdata = rITU656IN_ISR;
	
	#if 0
	XM_printf ("656IN ISR = 0x%08x\n", rdata);
	XM_printf (">>>tol_pix_num_per_line = %d\n", (rITU656IN_PIX_NUM_PER_LINE >> 16) & 0xFFF);
	XM_printf (">>>Act_pix_num_per_line = %d\n", rITU656IN_PIX_NUM_PER_LINE & 0xFFF);
	XM_printf (">>>tol_line_num_per_field = %d\n", (rITU656IN_LINE_NUM_PER_FIELD >> 16) & 0x7FF);
	XM_printf (">>>Act_line_num_per_field = %d\n", (rITU656IN_LINE_NUM_PER_FIELD >> 0) & 0x7FF);
	#endif
	
	if(rdata & (1 << 10))
	{
		// FIFO�е�ַΪ�գ������Ҫѹ��Y/UV��ַ
		// Addr fifo pop error mask
		XM_printf ("fifo pop error\n");
		itu656_in_reset_fifo ();
		//rdata &= ~((1 << 3)|(1 << 0));
		ov_post_process ();
	}

	if(rdata & (1 << 3) )
	{
		//XM_printf ("even ");
		itu656_in_frame_ready ();
		ov_post_process ();
	}
	if(rdata & (1 << 0))
	{
		//XM_printf ("odd  ");
		itu656_in_frame_ready ();
		ov_post_process ();
	}
	
	if(rdata & (1 << 2))
	{
		// Pre fifo push error interrupt request mask enable
		// �޷�������д�뵽DDR
		//XM_printf ("fifo push error\n");
	}
	if(rdata & (1 << 1))
	{
		// Pre fifo push error interrupt request mask enable
		//XM_printf ("PN change\n");
	}
	if(rdata & ((1 << 5)|(1 << 7)))
	{
		//  5	R	0	The active pix number per line changed interrupt request
		//		0:no interrupt
		//		1:interrupt
 		//	7	R0	The active line number per field changed interrupt request(with debounce )
		//		0:no interrupt
		//		1:interrupt
		//XM_printf ("Act_pix_num = %d\n", rITU656IN_PIX_NUM_PER_LINE & 0xFFF);
		//XM_printf ("Act_line_num = %d\n", (rITU656IN_LINE_NUM_PER_FIELD >> 0) & 0x7FF);
	}
	
	if(rdata & (1 << 8))
	{
		ov_post_process (); 
	}
	
	/*
	//����
	// Field start
	if (rdata & (1 << 8)) 
	{
		// �жϵ�ǰ��Ϊ�泡��ż��
      if( !(rITU656IN_ISR & (1 << 11)) )
      {
			unsigned int itu656_int_mask ;
			
			// 12	R/W	1'b0	frame_intr_odd_even_sel : 
			//		for frame interrupt request, the register describe which field the interrupt is output;
			//		0: interrupt output when odd field;
			//		1: interrupt output when even field;
			// ֡�ж���ż�����
         	rITU656IN_ENABLE_REG |= ( 1 << 0 ) | (1 << 12);		
			
			// 3	field interrupt request:even field
			//		0 : no interrupt
			//		1 : interrupt
		   itu656_int_mask	= 	(1 << 3) 				// "ż��"���ж�����ʹ��
									| 	(1 << 7)		// "����Ч�б仯"�ж�����ʹ��
									; 
			
			// ��ַ�쳣�ж�(push/pop error)
		   itu656_int_mask 	|= (1 << 10)	// Addr fifo pop error
									|	(1 << 2) 	// Pre fifo push error interrupt request
									;
			
		   rITU656IN_IMR = itu656_int_mask;
		   
      }
	}
	
	// ż�����ж�
	if (rdata & (1 << 0)) // even field : use for frame
	{	   
      // ���ݵ�ǰ����ʹ�õ�Y/CbCr��ַ���жϸո���ɵ�֡
		// 656inʹ��FIFO(�Ƚ��ȳ�����)����������4����ַ��(Y/CbCr),
		//	ͨ���ж�656inӲ����ǰ����ʹ�õĵ�ַ��(Y/CbCr), 
		//	������������ǰ��ĵ�ַ�Ա����˸ս�����ɵ�֡����
		itu656_in_frame_ready ();
	}
	*/
	
	// ����ж�
	rITU656IN_ICR  = rdata; 
}

// ����601(��/��ͬ���ź�)֡ģʽ
void itu601_config_frame_mode ( itu656_window_s *window )
{
	// ��λ��ʹ��ʱ��
#ifndef SIMULATE_7116
	sys_clk_disable (Itu_b_clk_enable);
	sys_clk_disable (itu656_xclk_enable);
	sys_soft_reset (softreset_itu);
	sys_clk_enable (itu656_xclk_enable);
	sys_clk_enable (Itu_b_clk_enable);	// ͨ��b
	
	//itu601in_SelPad();

	XM_lock ();
	// SYS_DEVICE_CLK_CFG0
	// 9	R/W	0	Itu_clk_b_sel
	// 				1:itu_clk_b_inv
	//				0:itu_clk_b
	//rSYS_DEVICE_CLK_CFG0 |= (0x1 << 9);
	rSYS_DEVICE_CLK_CFG0 &= ~(0x1 << 9);
	XM_unlock ();
	
	XM_lock ();
	unsigned int Itu_clk_b_dly = 0;
	unsigned int reg = rSYS_DEVICE_CLK_CFG3;
	reg &= ~(0x7F << 23);
	reg |= (Itu_clk_b_dly << 23);	
	rSYS_DEVICE_CLK_CFG3 = reg;
	XM_unlock ();
	
	rITU656IN_ModuleEn = 0x0<<5 |1<<4| 0x0<<1 | 0x1 | (1 << 2);		// ��ֹitu656 data write back to DDR2 
	
	// 0	R/W	1'b0	0 : sel itu601 input
	//					1 : select itu656 input
	
	#ifdef ITU656_INPUT
	rITU656IN_INPUT_SEL = 0x01;	// itu656 in
	#else
	rITU656IN_INPUT_SEL = 0x0;	// itu601 in
	#endif
	
	// ��ֹ�����ж�
	//rITU656IN_IMR = 0x0; 
	//rITU656IN_IMR = (1<<0)|(1<<3)  ; // bit.0 odd int    bit.3  even int 
	rITU656IN_IMR = 0x7FF;
	
	// ����ж�
	rITU656IN_ICR = 0x7ff;
	rITU656IN_SLICE_PIXEL_NUM = 0;

	// open window setting
	rITU656IN_H_CUT = ((window->Left_cut & 0x7ff) << 16) | (window->right_cut & 0x7ff );
	rITU656IN_V_CUT = ((window->up_cut & 0x7ff) << 16) | (window->down_cut & 0x7ff );
	
	#ifdef ITU656_INPUT
	// frame mode
	//rITU656IN_ENABLE_REG = (0<<13)|(1<<12)|  (1<<11)| (0<<6) |(1<<5) |(0<<4) | (0<<3) | (1<<1) | 0;
	
	//	field mode                        // bit.5: 0.field mode or 1.frame mode
	rITU656IN_ENABLE_REG = (0<<13)|(1<<12)|  (1<<11) |(0<<5) |(1<<4) | (0<<3) | (1<<1) | 0;
	#else
	// 601��������frame mode
	//	field mode                        // bit.5: 0.field mode or 1.frame mode
	rITU656IN_ENABLE_REG = (0<<13)|(1<<12)|  (1<<11) |(0<<5) |(1<<4) | (0<<3) | (1<<1) | 0;
	#endif
	
	//����֡�� |֡���в���| �м����ز���
	rITU656IN_DELTA_NUM = 5<<16 | (10<<8)|20;

	request_irq(ITU656_INT, PRIORITY_FIFTEEN, itu601in_int_framehandler);

	rITU656IN_SIZE = (window->width) << 16; //wide
	rITU656IN_DATA_OUT_LINE_NUM_PER_FIELD = window->height;
	//����Ч���� 
	rITU656IN_TOTAL_PIX = (window->width  - window->Left_cut - window->right_cut ) 
				  * (window->height - window->up_cut   - window->down_cut  ) 
				  ;  
	XM_printf("width:%d\n",  rITU656IN_SIZE>>16);
	XM_printf("height:%d\n", rITU656IN_DATA_OUT_LINE_NUM_PER_FIELD);
	XM_printf("TOTAL_PIX:%d\n",  rITU656IN_TOTAL_PIX );

	// 0	R/W	1'b2	0 : Cb Y Cr Y
	//					1 : Y Cb Y Cr
	//HS normal,VS INV,Y Cb Y Cr
	#ifdef EN_RN6752M1080N_CHIP
    rITU656IN_INPUT_CTL |= ((1<<15)|(1<<13)|(1<<2)); // ����
    #else
    rITU656IN_INPUT_CTL |= ((1<<15)|(0<<13)|(1<<2)); // ����
	#endif
#endif
	
   //rITU656IN_ENABLE_REG |= ( 3 << 1 );// | (1<<0) ;//bit.3:Axi_cmd_max
  //rITU656IN_ENABLE_REG |= ( 1 << 0 );
}


static unsigned int itu656in_busy (void)
{
	#define  status_val    ((0xf<<24)|(0x7<<20)|(0x3<<16))
	/* ����״̬��Ϊ���ʱ�򣬲�����и�λ�������κβ��� */
	/* 
		Axi_state:4bit:bit.27:24
		Aw_state: 3bit:bit.22:20
		W_state : 2bit:bit.17:16
	*/	
	if( rITU656IN_MODULE_STATUS & status_val )  
	{
      return 1;
   }
	else
	{
      return 0;
   }
}

// ʹ��ITU656/ITU601���ݽ���
//  (���ղ�����������ݣ�ͳ�����������ܵ�������Ч�У���Ч�е���Ϣ������д�뵽DDR)
void itu656in_enable (void)
{
   monitor_itu656_overflow_signal = 1;
   rITU656IN_ENABLE_REG |= ( 3 << 1 );// | (1<<0) ;//bit.3:Axi_cmd_max
   rITU656IN_ENABLE_REG |= ( 1 << 0 );
}

void itu656in_enable_receive (void)
{
   rITU656IN_ENABLE_REG |= ( 1 << 0 );	
}

void itu656in_disable (void)
{
	unsigned int ticket_to_timeout = XM_GetTickCount () + 100;
	// �ر�656 in, ���ȴ�656ģ��ֹͣ����.
	rITU656IN_ENABLE_REG = 0;
	monitor_itu656_overflow_signal = 0;
	while( itu656in_busy() )
	{
		if(XM_GetTickCount() >= ticket_to_timeout)
		{
			XM_printf ("itu656in_disable timeout\n");
			break;
		}
	}
}

// ��ȡYUV sensor��Ƶ������ߴ綨��
u32_t itu656_in_get_video_width  (void)
{
	return itu656_in_width; 	
}

u32_t itu656_in_get_video_height (void)
{
	return itu656_in_height;
}

// ��ȡYUV sensor�����ͼ���ֽڴ�С
// Y_UV420��ʽ
u32_t itu656_in_get_video_image_size (void)
{
	return itu656_in_width * itu656_in_height * 3 / 2;
}



// ��Ӳ��FIFO�����в���ָ���ĵ�ַ
static queue_s *match_y_addr_in_hw_fifo (unsigned int y_addr)
{
	queue_s *unit;
	XMSYSSENSORPACKET *packet;
	// �����Ƿ�Ϊ��
	if(queue_empty(&itu656_in_fifo))
		return NULL;
	
	unit = queue_next (&itu656_in_fifo);
	while (unit != &itu656_in_fifo)
	{
		packet = (XMSYSSENSORPACKET *)unit;
		if( (unsigned int)packet->data[0] == y_addr)
			return unit;
		unit = queue_next (unit);
	}
	return NULL;
}

// ��Ӳ��FIFO�����в���ָ����֡���
static queue_s *match_frame_id_in_hw_fifo (unsigned int frame_id)
{
	queue_s *unit;
	XMSYSSENSORPACKET *packet;
	// �����Ƿ�Ϊ��
	if(queue_empty(&itu656_in_fifo))
		return NULL;
	unit = queue_next (&itu656_in_fifo);
	while (unit != &itu656_in_fifo)
	{
		packet = (XMSYSSENSORPACKET *)unit;
		if( packet->frame_id == frame_id)
			return unit;
		unit = queue_next (unit);
	}
	return NULL;
}

// ��ʾ��ǰ�� FIFO�л��ж������ַ����
#define	FIFO_GROUP_ADDRESS_NUM		(rITU656IN_MODULE_STATUS >> 29)

// �����쳣ʱ, ��ģ�������û�е�ַ��Ԫ, ��FIFO�����еĵ�Ԫ����ѹ�뵽Ӳ��FIFO
static void itu656_in_reset_fifo (void)
{
	queue_s *unit;
	XMSYSSENSORPACKET *packet;
	queue_s temp_link;
	int Repeat_Num = 0;
	// �Ӷ���ͷ��ʼ����ÿһ֡, �����FIFO��ɾ��, ������ѹ�뵽Ӳ��FIFO׼����������
	if(queue_empty (&itu656_in_fifo))
		return;
	
	// ��ʾ��ǰ�� FIFO�л��ж������ַ����
	unsigned int count = (rITU656IN_MODULE_STATUS >> 29);
	if(count)
	{
		XM_printf ("error, FIFO left addr count = %d\n", count);
	}
	
	// XM_printf ("itu656_in_reset_fifo\n");
	queue_initialize (&temp_link);
	unit = queue_next (&itu656_in_fifo);
	while(unit != (queue_s *)&itu656_in_fifo)
	{
		// ȡ�����е�ÿ����Ԫ, ����FIFO�������Ƴ�
		packet = (XMSYSSENSORPACKET *)unit;
		unit = queue_next (unit);
		queue_delete ((queue_s *)packet);	
		// ��ʱ����
		queue_insert ((queue_s *)packet, &temp_link);
	}
	
	unit = queue_next (&temp_link);
	
	while(unit != (queue_s *)&temp_link)
	{
		packet = (XMSYSSENSORPACKET *)unit;
		unit = queue_next (unit);
		queue_delete ((queue_s *)packet);	
		// ����ѹ�뵽Ӳ��FIFO׼���������� 
		itu656_in_set_frame_ready (packet->frame_id);
	}
	
}

// ������֡(frame_id)ѹ�뵽Ӳ������׼����������
// ��sensor�������, �ж�����, ִ��, �жϽ���
void itu656_in_set_frame_ready (unsigned int frame_id)
{		
	if(frame_id >= ITU656_IN_FRAME_COUNT)
	{
		XM_printf ("illegal itu656 in frame id (%d)\n", frame_id);
		return;
	}
	
	// ����֡id�Ƿ�����Ӳ��������, ����, �쳣
	if(match_frame_id_in_hw_fifo (frame_id))
	{
		XM_printf ("itu656_in_set_frame_ready error, (%d) still in fifo\n", frame_id);
		return;
	}

	// ѹ�뵽656 in��Ӳ������
	//XM_printf ("push 0x%08x\n", (unsigned int)itu656_in_unit[frame_id].data[0]);
	rITU656IN_DRAM_Y_ADDR = (unsigned int)itu656_in_unit[frame_id].data[0];
	rITU656IN_DRAM_Y_ADDR = (unsigned int)itu656_in_unit[frame_id].data[1];
	
	queue_insert ((queue_s *)&itu656_in_unit[frame_id], &itu656_in_fifo);		// ���뵽Ӳ��FIFO����β��
}


// ��ȡ�ս�����֡���ݵ�֡id
static int itu656_in_get_ready_frame_id (void)
{
	queue_s *unit;
	XMSYSSENSORPACKET *packet;
	XMSYSSENSORPACKET *data_frame;
	unsigned int fifo_group_address_num;
	
	// ���ȼ��FIFO_GROUP_NUM.
	// ��Ϊ0������field interrupt request, ���������в�����ѹ��
	fifo_group_address_num = FIFO_GROUP_ADDRESS_NUM;
	if(fifo_group_address_num == 0)
	{
		itu656_in_reset_fifo ();
		return -1;
	}
	
	// ��ȡӲ����ǰ����ʹ�õ�Y/UV��ַ(����ǰ����ɽ���һ֡��Y��ַ), �ж���׼���õ�֡id
	unsigned int y_addr = rITU656IN_DRAM_Y_ADDR;
	//XM_printf ("READY 0x%08x\n", y_addr);
	
	// ��Ӳ��FIFO����ͷ��ʼ����, ֱ���ҵ���y_addrƥ��ĵ�Ԫ, ����ƥ�䵥Ԫ����ŷ���
	if(queue_empty(&itu656_in_fifo))
	{
		// Ӳ��FIFO����Ϊ��, 
		// XM_printf ("itu656 in error, frame (y = 0x%08x) missed, queue empty\n", y_addr);
		return -1;
	}
	
	unit = match_y_addr_in_hw_fifo (y_addr);
	if(unit == NULL)
	{
		// δ����Ӳ��FIFO�������ҵ���ǰӲ��ʹ�õ�Ԫ, �쳣
		// XM_printf ("itu656 in error, no unit match frame (y = 0x%08x)\n", y_addr);
		return -2;		// �쳣
	}
	
	// 20180627 
	// y_addrָ��ǰ�ս�����ϵ�֡��ʼ��ַ(Y��ַ)
	data_frame = (XMSYSSENSORPACKET *)unit;
	
	// �Ӷ���ͷ��ʼ����ÿһֱ֡���ս�����ϵ�����֡, �����FIFO��ɾ��, ������ѹ�뵽Ӳ��FIFO׼����������
	unit = queue_next (&itu656_in_fifo);
	while(unit != (queue_s *)data_frame)
	{
		// ȡ�����е�ÿ����Ԫ, ����FIFO�������Ƴ�
		packet = (XMSYSSENSORPACKET *)unit;
		unit = queue_next (unit);
		queue_delete ((queue_s *)packet);
		
		// ����ѹ�뵽Ӳ��FIFO׼����������
		// XM_printf ("itu656 in lost, y=0x%08x\n", packet->data[0]);
		itu656_in_set_frame_ready (packet->frame_id);
		
		unit = queue_next (&itu656_in_fifo);
	}
	
	// ���ѽ�����ϵ�����֡��FIFO�����жϿ�
	queue_delete ((queue_s *)data_frame);
	
	return data_frame->frame_id;
}

// ��itu656 in �жϷ������, ֪ͨһ��֡�ѽ������, ����ȡ��
static void itu656_in_frame_ready (void)
{
	int frame_id = itu656_in_get_ready_frame_id ();
	if(frame_id == -1)
	{
		// XM_printf ("frame_ready, missed\n");
		// û��׼���õ�֡
		return;
	}
	else if(frame_id < 0)
	{
		// �쳣
		// Ͷ���¼��������߳�itu656_in_task
		return;
	}
	//XM_printf ("pop 0x%08x\n", (unsigned int)itu656_in_unit[frame_id].data[0]);
	XMSYS_SensorNotifyDataFrame (itu656_in_sensor_channel, frame_id);
}

extern char *itu656_in_get_frame_buffer (unsigned int frame_id, unsigned int frame_size);

static int itu656_in_fifo_setup (unsigned int in_width, unsigned int in_height)
{
	int i;
	// �����⵽������֡���/�߶��Ƿ�����Ѿ�̬���仺���������ߴ�
	if(in_width > ITU656_IN_MAX_WIDTH || in_height > ITU656_IN_MAX_HEIGHT)
	{
		XM_printf ("itu656_in_fifo_setup failed, big size (w = %d, h = %d)\n", in_width, in_height);
		return -1;
	}
    
	OS_Use (&Itu656InSema);
	queue_initialize (&itu656_in_free);
	queue_initialize (&itu656_in_fifo);

	memset (itu656_in_unit, 0, sizeof(itu656_in_unit));
	for (i = 0; i < ITU656_IN_FRAME_COUNT; i ++)
	{
		itu656_in_unit[i].frame_id = i;
		itu656_in_unit[i].id = ITU656_ID;
		itu656_in_unit[i].channel = 1;		// ͨ����
		// ITU656����ΪY_UV420��ʽ
		itu656_in_unit[i].data[0] = itu656_in_get_frame_buffer(i,in_width * in_height*3/2);		// Y
		itu656_in_unit[i].data[1] = itu656_in_get_frame_buffer(i,in_width * in_height*3/2) + in_width * in_height;	// CbCr
		queue_insert ((queue_s *)&itu656_in_unit[i], &itu656_in_free);
	}
	
	// д���������ֶ�
	unsigned int *addr = (unsigned int *)vaddr_to_page_addr((unsigned int)itu656_in_get_frame_buffer (ITU656_IN_FRAME_COUNT+1, in_width * in_height*3/2
																																	  ));
	overflow_flag_address = (unsigned int *)(((char *)addr));
	next_overflow_flag_address = (unsigned int *)(((char *)overflow_flag_address) + 512 * 1024 - 4);
	*overflow_flag_address = OVERFLOW_FLAG;
	*next_overflow_flag_address = OVERFLOW_FLAG;
	OS_Unuse (&Itu656InSema);
	return 0;
}

int itu656_in_setup (unsigned int in_width, unsigned int in_height)
{
	int ret;
	itu656_window_s itu656_config;
	if( (in_width*in_height) > (ITU656_IN_MAX_WIDTH * ITU656_IN_MAX_HEIGHT))
	{
		XM_printf ("itu656_in_setup failed, 656in size(%d, %d) exceed maximum size of fifo definition(%d, %d)\n", in_width, in_height, ITU656_IN_MAX_WIDTH, ITU656_IN_MAX_HEIGHT);
		return -1;
	}
	
	ret = itu656_in_fifo_setup (in_width, in_height);
	if(ret < 0)
	{
		XM_printf ("itu656_in_setup failed, fifo_setup NG\n");
		return -1;
	}
	itu656_config.Left_cut = 0;
	itu656_config.right_cut = 0;
	itu656_config.up_cut = 0;
	itu656_config.down_cut = 0;
	itu656_config.width = in_width + itu656_config.Left_cut + itu656_config.right_cut;
	itu656_config.height = in_height + itu656_config.up_cut + itu656_config.down_cut;
	itu656_config.real_w = in_width-itu656_config.Left_cut-itu656_config.right_cut;
	itu656_config.real_h = in_height-itu656_config.up_cut-itu656_config.down_cut ;
	
	itu601_config_frame_mode (&itu656_config);
	return 0;
}

void rxchip_itu656_PlugOut(void)
{
	OS_SignalEvent (ITU656IN_EVENT_STOP, &TCB_Itu656In);
}

void rxchip_itu656_PlugIn(void)
{
	OS_SignalEvent (ITU656IN_EVENT_START, &TCB_Itu656In);
}

u32_t camera_get_video_width_2 (void);
u32_t camera_get_video_height_2 (void);

void itu656_in_task (void)
{
	OS_U8 itu656_in_event;

	while(1)
	{
	  	itu656_in_event = OS_WaitEvent (ITU656IN_EVENT_STOP | ITU656IN_EVENT_START);
		if(itu656_in_event & ITU656IN_EVENT_STOP)
		{
			itu656in_disable();
			OS_Delay(10);
			XMSYS_SensorCaptureReset (1);
			OS_Delay(10);
			XMSYS_SensorResetCurrentPacket(1);
			itu656_in_state = ITU656_IN_IDLE;
		}
		
		if(itu656_in_event & ITU656IN_EVENT_START)
		{
			#ifdef ITU656_INPUT
			itu656in_SelPad();
			#else
			itu601in_SelPad();
			#endif

			itu656_in_width = camera_get_video_width_2();
			itu656_in_height = camera_get_video_height_2();
			
			// ����ʼ���Ƿ��쳣
			if(itu656_in_setup (itu656_in_width, itu656_in_height) < 0)
				continue;
			
			for (unsigned int frame_id = 0; frame_id < ITU656_IN_FRAME_COUNT; frame_id ++)
			{
				itu656_in_set_frame_ready(frame_id);
			}
			OS_Delay(10);
			SensorInitCaptureFIFO_Second();
			OS_Delay(10);
			itu656_in_state = ITU656_IN_RUN;
			itu656in_enable();
			itu656in_enable_receive();//add luo
			rITU656IN_ModuleEn &= ~(1 << 2);
		}
	}
	
	
}

void xm_itu656_in_init (void)
{
	itu656_in_state = ITU656_IN_IDLE;
    OS_CREATERSEMA(&Itu656InSema); /* Creates resource semaphore */
	OS_CREATETASK (&TCB_Itu656In, "Itu656In", itu656_in_task, XMSYS_ITU656_IN_TASK_PRIORITY, Stack_Itu656In);
}



