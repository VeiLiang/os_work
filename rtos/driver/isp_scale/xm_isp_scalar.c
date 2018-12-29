//****************************************************************************
//
//	Copyright (C) 2012~2016 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_isp_scalar.c
//	  ARKN141 ISP scalar device driver
//
//	Revision history
//
//		2016.04.02	ZhuoYongHong Initial version
//
//****************************************************************************
#include "hardware.h"
#include "types.h"
#include <xm_type.h>
#include "RTOS.H"
#include <xm_file.h>
#include <xm_base.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "xm_queue.h"
#include "xm_dev.h"
#include "hw_osd_layer.h"
#include "xm_isp_scalar.h"
#include "xm_printf.h"
#include "arkn141_isp.h"
#include "xm_printf.h"
#include "rxchip.h"

// 5	R/W	0	AXI SCALE write back middle finish interupt
//	2	R/W	0	AXI SCALE write back bresp error interrupt
// 0	R/W	0	AXI SCALE write back frame finish interupt
#define  FRAME_FINISH                      		 		(1 << 0)
#define	write_back_bresp_error_interrupt				(1 << 2)
#define	write_back_middle_finish_interupt				(1 << 5)

// 2	R/W	0	AXI SCALE write back middle finish interupt clear
//					Write 1 to clear
// 1	R/W	0	AXI SCALE write back bresp error interrupt clear
//					Write 1 to clear
//0	R/W	0	AXI SCALE write back frame finish interupt clear
//					Write 1 to clear
#define  CLEAR_FRAME_FINISH                 			(1 << 0)
#define	write_back_bresp_error_interrupt_clear		(1 << 1)
#define	write_back_middle_finish_interupt_clear		(1 << 2)

#define	ISP_SCALAR_STOP					rISP_SCALE_EN = 0x0
#define	ISP_SCALAR_RUN						rISP_SCALE_EN = 0x03

#define	ISP_SCALAR_BUFFER_COUNT			4		// ARKN141 ISP ScalarӲ����������fifo depthΪ4

#define	ISP_SCALAR_ID						0x52415349		// ISAR

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

// ���ò���
static xm_isp_scalar_configuration_parameters isp_scalar_parameters;


static xm_isp_scalar_buffer_s isp_scalar_buffer[ISP_SCALAR_BUFFER_COUNT];

static  queue_s isp_scalar_packet_fifo;		// Ӳ��FIFO����
static  queue_s isp_scalar_packet_free;		// ���ж���

static volatile int isp_scaler_inited;

static volatile int isp_scalar_index;		// ���, һֱ�ۼ�
static volatile int isp_scalar_object;	// ��������

static OS_RSEMA 	isp_scalar_access_sema;		// ��������ź���

#define	MAX_FRAME_COUNT_TO_DISCARD_TO_REMOVE_FLASH	3	// 3֡

static int isp_scalar_frames_to_discard_to_remove_flash;		// ����sensor��, Ϊ������˸ʱ��Ҫ�ӵ���֡��

static void isp_scale_clock_init (void)
{
	unsigned int Itu_clk_b_dly;
	unsigned int Isp_Scale_clk_div, Int_isp_scale_clk_sel;
	unsigned int val;	

	// ISP-Scalar��ʱ�� >= ISP��ʱ�� 
	// ISP-Scalar��Ҫ�Ŵ�Դͼ��ʱ, ��Ҫ���Ŵ������ʱ��Ƶ�� 
	
	// ISP Scalar��ʱ�� == ISP��ʱ��
	// SYS_device_clk_cfg3
	// 29-23	R/W	0x0	Itu_clk_b_dly
	Itu_clk_b_dly = 0;
	
	// 13-11	R/W	0x0	Isp_Scale clk div
	//							Isp_Scale clk = int_isp_scale_clk/div
	//Isp_Scale_clk_div = 2;	// 300/3 = 100
#if CZS_USB_01
	Isp_Scale_clk_div = 4;
	//Isp_Scale_clk_div = 3;
	// 10-8	R/W	0x4	Int_isp_scale_clk_sel
	//							4'b0000: Clk_240m
	//							4'b0001: Syspll_clk
	//							4'b0010: audpll_clk
	//							4'b0100: Clk_24m
	//Int_isp_scale_clk_sel = 0;		// syspll
	//Int_isp_scale_clk_sel = 1;
	Int_isp_scale_clk_sel = 2;
#else
	//Isp_Scale_clk_div = 4;
        Isp_Scale_clk_div = 2;
	//Isp_Scale_clk_div = 3;
	// 10-8	R/W	0x4	Int_isp_scale_clk_sel
	//							4'b0000: Clk_240m
	//							4'b0001: Syspll_clk
	//							4'b0010: audpll_clk
	//							4'b0100: Clk_24m
	//Int_isp_scale_clk_sel = 0;		// syspll
	//Int_isp_scale_clk_sel = 1;	// Syspll_clk
	Int_isp_scale_clk_sel = 2;		// audpll_clk
#endif

	XM_lock ();
	
   // Itu_clk_b_sel
   rSYS_DEVICE_CLK_CFG0 &= ~(0x1<<9);//isp scale  ʱ����λȡ��  
	
	val = rSYS_DEVICE_CLK_CFG3;
	val &= ~( (0x7F << 23) | (0x07 << 11) | (0x07 << 8) );
	val |=  (Itu_clk_b_dly << 23) | (Isp_Scale_clk_div << 11) | (Int_isp_scale_clk_sel << 8);
	rSYS_DEVICE_CLK_CFG3 = val;
	
	XM_unlock ();
	
	sys_clk_enable (Isp_Scale_clk_enable);
}

static void isp_scale_clock_stop (void)
{
	sys_clk_disable (Isp_Scale_clk_enable);
}


void xm_isp_scalar_init (void)
{
	xm_isp_scalar_buffer_s *buffer;
	
	OS_EnterRegion();
	if(isp_scaler_inited)
	{
		OS_LeaveRegion();
		return;
	}
	
	isp_scale_clock_stop ();
	delay (100);
	sys_soft_reset (softreset_isp_scale);
	
	memset (&isp_scalar_parameters, 0, sizeof(isp_scalar_parameters));
	
	queue_initialize (&isp_scalar_packet_free);
	queue_initialize (&isp_scalar_packet_fifo);
	
	buffer = isp_scalar_buffer;
	for( int no = 0; no < ISP_SCALAR_BUFFER_COUNT ; no ++  )
	{
		queue_insert ((queue_s *)buffer, &isp_scalar_packet_free);
		buffer ++;
	}
	
	OS_CREATERSEMA (&isp_scalar_access_sema);
		
	ISP_SCALAR_STOP;
	
	irq_disable (ISP_SCALE_INT);
	
	isp_scaler_inited = 1;
	isp_scalar_index = 0;
	isp_scalar_object = -1;
		
	OS_LeaveRegion();	
}

void xm_isp_scalar_exit (void)
{
	OS_EnterRegion();
	ISP_SCALAR_STOP;
	irq_disable (ISP_SCALE_INT);
	
	OS_DeleteRSema (&isp_scalar_access_sema);
	isp_scaler_inited = 0;
	OS_LeaveRegion();	
}

int xm_isp_scalar_config ( xm_isp_scalar_configuration_parameters  *scalar_parameters )
{
	int val;
   
   if( !scalar_parameters )
	{
		XM_printf("xm_isp_scalar_config scalar_parameters == NULL ! \r\n");
		return XM_ISP_SCALAR_ERRCODE_ILLEGAL_PARA;
	}
	if( scalar_parameters->mid_line >= scalar_parameters->dst_height )
	{
		XM_printf("xm_isp_scalar_config mid_line > dst_height! \r\n");
		return XM_ISP_SCALAR_ERRCODE_ILLEGAL_PARA;
	}
	
	// �ж��Ƿ�����С����LCD(С��VGA). ����,����4֡��ʾ 
#if HYBRID_H264_MJPG
	rISP_SCALE_Frame_Mask = 0xFEFEFEFE;
	//rISP_SCALE_Frame_Mask = 0xBF7DF7DF;	// 
#elif CZS_USB_01
	if(scalar_parameters->dst_width >= 1024 && scalar_parameters->dst_height >= 600)
		rISP_SCALE_Frame_Mask = 0x77777777;	// 22֡
	else
		rISP_SCALE_Frame_Mask = 0xFEFEFEFE;
#elif TULV
	//rISP_SCALE_Frame_Mask = 0xFEFEFEFE;
	rISP_SCALE_Frame_Mask = 0x77777777;
#else
	if(scalar_parameters->dst_width < 640 && scalar_parameters->dst_height < 480)
		rISP_SCALE_Frame_Mask = 0xFEFEFEFE;
	else
		//rISP_SCALE_Frame_Mask = 0xFFFFFFFF;
		rISP_SCALE_Frame_Mask = 0xFEFEFEFE;
	//rISP_SCALE_Frame_Mask = 0xFEFEFEFE;		// 20170322�������� ��·���Ч����, ��֡������PC��(USB Camera)���Ե�ͣ��. 
												// 20170407 Ӧ������������,����JPEG����ͣ��
	//rISP_SCALE_Frame_Mask = 0x11111111;
#endif
	
	//rISP_SCALE_Frame_Mask = 0x10101010;
	//rISP_SCALE_Frame_Mask = 0xFFFFFFFF;
	rISP_SCALE_Frame_Mask = 0x7FFF7FFF;		// ����2֡
	
	val = 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x0<<6|0x0<<4|0x0<<3;
	
	if(scalar_parameters->src_format == XM_ISP_SCALAR_FORMAT_Y_UV422)
		val |= (1 << 2) | (0 << 0);		// 0: yuv422, plane format
	else if(scalar_parameters->src_format == XM_ISP_SCALAR_FORMAT_Y_UV420)
		val |= (1 << 2) | (1 << 0);		// 1: yuv420, plane format
	else if(scalar_parameters->src_format == XM_ISP_SCALAR_FORMAT_YUV422)
		val |= (0 << 2) | (1 << 1)|(0 << 0);		// 0: yuv422, packet format
		//val |= (0 << 2) |(0 << 0);		// 0: yuv422, plane format
	else if(scalar_parameters->src_format == XM_ISP_SCALAR_FORMAT_YUV420)
		val |= (0 << 2) | (1 << 0);		// 0: yuv422, plane format
	else
	{
		XM_printf("xm_isp_scalar_config, unsupported format(%d)\n", scalar_parameters->src_format);
		return XM_ISP_SCALAR_ERRCODE_ILLEGAL_PARA;
	}
	
	OS_Use (&isp_scalar_access_sema);
			
	rISP_SCALE_RESERVED = (scalar_parameters->mid_line & 0xfff);

	// ���Դ������Ŀ�Ĵ��ڴ�С�Ƿ���ͬ, ��ͬ���ֹFIR�˲���
	if( 	scalar_parameters->src_window_width == scalar_parameters->dst_window_width
		&&	scalar_parameters->src_window_height == scalar_parameters->dst_window_height )
		val |= (1 << 5);
	   
	rISP_SCALE_CONTROL = val;
	
	// Դͼ���С
	rISP_SCALE_VIDEO_SOURCE_SIZE	= ((scalar_parameters->src_width & 0xFFF) << 0)
											| ((scalar_parameters->src_height & 0xFFF) << 12);
	
	// Դ���ڶ���
	rISP_SCALE_VIDEO_WINDOW_POINT = 	((scalar_parameters->src_window_x & 0xFFF) << 0)
											|	((scalar_parameters->src_window_y & 0xFFF) << 12);	
	rISP_SCALE_VIDEO_WINDOW_SIZE  =	((scalar_parameters->src_window_width & 0xFFF) << 0)
											|	((scalar_parameters->src_window_height & 0xFFF) << 12);
	
	// Ŀ��ͼ�� (Ŀ�괰��)��С
	rISP_SCALE_VIDEO_SIZE	= ((scalar_parameters->dst_window_width & 0xFFF) << 0)
									| ((scalar_parameters->dst_window_height & 0xFFF) << 12);
	
      
   //��������� ��ͬʱ�����Դ��С��Ŀ���С һ��
   if( (scalar_parameters->src_window_height ==  scalar_parameters->dst_window_height) 
		&& (scalar_parameters->src_window_width ==  scalar_parameters->dst_window_width) )
		rISP_SCALE_SCALE_CTL = 0<<9| 0<<8 | 1<<7| 0<<6 | 1<<5 | 0<<4 | 0<<3 | 0<<2 | 0<<1 | 0;
	else
		rISP_SCALE_SCALE_CTL = 0<<9| 1<<8 | 1<<7| 1<<6 | 1<<5 | 0<<4 | 0<<3 | 0<<2 | 0<<1 | 0;

	rISP_SCALE_SCALE_VXMOD = 512<<11 | 0<<0;/*V_xmod_odd_init<<11 | V_xmod_even_init */
	
	/*left cut number in line scaler<<18 | hfz */
   rISP_SCALE_SCALE_CTL0 = 20<<18 | ((((scalar_parameters->src_window_width)-20) * 1024 / scalar_parameters->dst_window_width) << 0); 

	/* Up cut line number in veritical scaler <<18 | vfz */
   rISP_SCALE_SCALE_CTL1 = 0<<18 | (( scalar_parameters->src_window_height * 1024 / scalar_parameters->dst_window_height) << 0); 

	/* right_cut_num<<8 | bottom_cut_num */
   rISP_SCALE_RIGHT_BOTTOM_CUT_NUM = 0<<8  | 0<<0; 
   
	/* д��ram���ݵ�����ÿ�е��� */
	/*The number of the horizontal pix in write back ram, it is equal or bigger than the wide of scale, x16 */
   rISP_SCALE_WB_DATA_HSIZE_RAM = scalar_parameters->dst_stride; 
	
	// Source :YUV FORMAT Address set
   // bypass to isp_scale ����ҪдԴ��ַ
	// ISPscale_arkn141_set_source_addr(yuv ,inwidth, inheight );
   // rISP_SCALE_VIDEO_ADDR1 =  (int)yuv;
	
   if(scalar_parameters->mid_line)//rISP_SCALE_INT_CTL
      val = (1 << 0) | (1 << 2);
   else
      val = (1 << 0) | (1 << 1) ;
   
   // mask interrupt                                  /*Pix_abort mask*/
   // bit.0: frame finish interupt
   // bit.1: bresp error interrupt
   // bit.2: middle interrupt
   // bit.3: Y addr buffer push error
   // bit.4: Y addr buffer pop error
   // bit.5: UV addr buffer push error
   // bit.6: UV addr buffer pop error
   // bit.7: Finish  addr buffer push error
   // bit.8: Finish  addr buffer pop error
   // bit.9: Pix_abort mask  
   /*������֡�쳣����ʹ��һ���д��ַ�������ᱣ�浽finish��FIFO��*/
	if(scalar_parameters->mid_line)
		rISP_SCALE_INT_CTL = (1<<1) | (1<<2) | (0x3f<<3);	// ��ֵ�ж�
	else
		rISP_SCALE_INT_CTL = (1<<0) | (1<<1) | (0x3f<<3);	// ֡�ж�
   
   //offset :0x74
   
   //bit.0: ISP_scale_sel_601
   //bit.0: 0:isp    1:itu601
   
   //bit.1: Valid_polarity: the valid polarity of the hsync and vsync
   //bit.1: 0: high level valid   1: low level valid

   //bit.2: 0:uyvy  1:yuyv
   //rISP_SCALE_WB_CTL |= 0|(0<<3)|(1<<2);
	
	val = ((scalar_parameters->src_ycbcr_sequence & 0x01) << 2)		// 1:yuyv
		|	(0 << 3);	// 0:uv	
	if(scalar_parameters->src_hsync_polarity == XM_ISP_SCALAR_SRC_SYNC_PLOARITY_HIGH_LEVEL)
		val |= (0 << 1);	// HSYNC Valid_polarity high level
	else
		val |= (1 << 1);	// HSYNC Valid_polarity low level
	if(scalar_parameters->src_vsync_polarity == XM_ISP_SCALAR_SRC_SYNC_PLOARITY_HIGH_LEVEL)
		val |= (0 << 4);	// VSYNC Valid_polarity high level
	else
		val |= (1 << 4);	// VSYNC Valid_polarity low level
	
	
	if(scalar_parameters->src_channel == XM_ISP_SCALAR_SRC_CHANNEL_ISP)
		val |= (0 << 0);
	else	// ISP_SCALAR_SRC_CHANNEL_ITU601
		val |= (1 << 0);

	rISP_SCALE_WB_CTL = val;
	
	// �������
	memcpy (&isp_scalar_parameters, scalar_parameters, sizeof(isp_scalar_parameters));
   
	OS_Unuse (&isp_scalar_access_sema);
	
   return  XM_ISP_SCALAR_ERRCODE_OK ;
}

// ������е�ַ��Ϣ
static void xm_isp_scalar_buffer_clear (void)
{
	// �������BUFFER��ַ��Ϣ
	rISP_SCALE_WB_STATUS |= (u32_t)(1 << 31);	// 31	R/W	0	Write 1 to clean all the addr fifo
}

extern void isp_scalar_run(void);

// ѹ�뵽Ӳ��֡���п�ʼ�ɼ�����
static void isp_scalar_push_hardware_address (xm_isp_scalar_buffer_s *buffer)
{
	if(buffer->window_y == NULL || buffer->window_uv == NULL)
	{
		XM_printf ("isp_scalar failed, illegal address\n");
		return;
	}
	if(buffer->id != ISP_SCALAR_ID)
	{
		XM_printf ("illegal isp frame(0x%08x), id(%d) invalid\n", buffer, buffer->id);
	}
	
	
	XM_lock();
	assert (buffer->next == buffer && buffer->prev == buffer);
	//XM_printf ("PUSH y=0x%08x, t=%d\n", buffer->window_y, XM_GetTickCount());
	rISP_SCALE_WB_DEST_YADDR = buffer->window_y;
	rISP_SCALE_WB_DEST_UADDR = buffer->window_uv; 
	if(queue_empty(&isp_scalar_packet_fifo))
	{
		queue_insert ((queue_s *)buffer, &isp_scalar_packet_fifo);
		isp_scalar_run();
		//ISP_SCALAR_RUN; 
		//XM_printf ("scalar run\n");
	}
	else
	{
		queue_insert ((queue_s *)buffer, &isp_scalar_packet_fifo);
	}
	XM_unlock();
}

static void isp_scalar_recovery (void)
{
	queue_s *curr_frame, *next_frame;
	queue_s *buffer;
	unsigned int wb_status;
	queue_s temp_link;
	
	// ���FIFO�����Ƿ�Ϊ��
	if(queue_empty (&isp_scalar_packet_fifo))
		return;
	
	// ���Ӳ��FIFO�Ƿ�Ϊ��
	if( (rISP_SCALE_WB_STATUS >> 2) & 0x07 )
	{
		// 4-2	R	0	Y_dest_addr_fifo data num
		return;
	}
	
	XM_printf ("isp_scalar_recovery\n");
	
	queue_initialize (&temp_link);
	next_frame = queue_next (&isp_scalar_packet_fifo);
	while(next_frame != &isp_scalar_packet_fifo)
	{
		curr_frame = next_frame;
		next_frame = queue_next (next_frame);
		queue_delete (curr_frame);
		// ���뵽��ʱ����
		queue_insert (curr_frame, &temp_link);
	}
	
	// ����ʱ����ĵ�Ԫ����ѹ�뵽Ӳ������
	next_frame = queue_next (&temp_link);
	while(next_frame != &temp_link)
	{
		curr_frame = next_frame;
		next_frame = queue_next (next_frame);
		queue_delete (curr_frame);
		// ����ѹ�뵽Ӳ��FIFO׼����������
		isp_scalar_push_hardware_address ((xm_isp_scalar_buffer_s *)curr_frame);
	}
}

// ��ISP scalar�жϳ������
// ֪ͨһ���µ�ISP scalar֡�Ѳ���
static void isp_scalar_frame_notify (unsigned int finished_y_addr)
{	
	int event;
	xm_isp_scalar_buffer_s *curr_frame, *next_frame, *data_frame;
	int no;
	
	//XM_printf ("frame_notify y=0x%08x, t=%d\n", finished_y_addr, XM_GetTickCount());
	if(finished_y_addr == NULL)
	{
		XM_printf ("isp_scalar_frame_notify failed, NULL address\n");
		return;
	}
		
	// ��Ӳ������������current_y_addr
	if(queue_empty(&isp_scalar_packet_fifo))
	{
		XM_printf ("mismatch 0x%08x, scalar fifo empty, scalar stop\n", finished_y_addr);
		//ISP_SCALAR_STOP;
		return;
	}
	else
	{
		// ����Ƿ���Ӳ��������
		curr_frame = (xm_isp_scalar_buffer_s *)queue_next (&isp_scalar_packet_fifo);
		while(curr_frame != (xm_isp_scalar_buffer_s *)&isp_scalar_packet_fifo)
		{
			if(curr_frame->window_y == finished_y_addr)
				break;
			curr_frame = curr_frame->next;
		}
		
		if(curr_frame == (xm_isp_scalar_buffer_s *)&isp_scalar_packet_fifo)
		{
			// �����Y��ַ����Ӳ��������
			// ϵͳ�쳣
			XM_printf ("abormal 0x%08x\n", finished_y_addr);
			isp_scalar_recovery ();
			return;
		}
		
		// ��Ӳ���ŶӶ�����ɨ�������Y��ַ
		// �������Y��ַ��Ӧ��Ԫ֮ǰ�ĵ�Ԫ��Ϊ�Ǵ���ĵ�Ԫ
		curr_frame = (xm_isp_scalar_buffer_s *)queue_next (&isp_scalar_packet_fifo);
		while(curr_frame != (xm_isp_scalar_buffer_s *)&isp_scalar_packet_fifo)
		{
			next_frame = curr_frame->next;
			queue_delete ((queue_s *)curr_frame);
			
			if(curr_frame->window_y == finished_y_addr)
			{
				if(curr_frame->id != ISP_SCALAR_ID)
				{
					XM_printf ("illegal isp frame(0x%08x), id(%d) invalid\n", curr_frame, curr_frame->id);
				}
				if(isp_scalar_frames_to_discard_to_remove_flash > 0)
				{
					// ������˸��Ҫ������֡
					isp_scalar_frames_to_discard_to_remove_flash --;
					// ����ѹ�뵽Ӳ������
					isp_scalar_push_hardware_address (curr_frame);
					break;
				}
				
				// �����û��ص�����
				if(curr_frame->data_ready_user_callback)
				{
					(*curr_frame->data_ready_user_callback)(curr_frame->user_private_data);
				}
				
				// ��֡��Ԫ�ͷŵ�δʹ�ö���
				curr_frame->id = 0;
				curr_frame->window_y = 0;
				curr_frame->window_uv = 0;
				curr_frame->user_private_data = 0;
				queue_insert ((queue_s *)curr_frame, &isp_scalar_packet_free);
				break;
			}
			else
			{
				// ��ǰ�����֡��δ��ɵ�֡���ߴ����֡, ��Ҫ���½���ѹ�뵽Ӳ������
				//XM_printf ("error scalar frame 0x%08x\n", curr_frame);
				isp_scalar_push_hardware_address (curr_frame);
			}
			curr_frame = next_frame;
		}
	}
	
	// ���Ӳ�������Ƿ�Ϊ��. ����, ֹͣscalar
	if(queue_empty (&isp_scalar_packet_fifo))
	{
		//ISP_SCALAR_STOP;
		//XM_printf ("fifo empty, scalar stop\n");
		//XM_printf ("scalar fifo empty\n");
	}
}



static void isp_scalar_int_handler (void)
{
   unsigned int Y_finish, UV_finish;
	unsigned int status    = rISP_SCALE_INT_STATUS;
   unsigned int wb_status = rISP_SCALE_WB_STATUS;
   unsigned int clr_wb_status =0;
	unsigned int Y_current = rISP_SCALE_WB_DEST_YADDR;
   
	if( status & FRAME_FINISH )
	{
		//ISP_SCALAR_STOP;
		rISP_SCALE_CLCD_INT_CLR = CLEAR_FRAME_FINISH;// clear frame int
		if(isp_scalar_parameters.mid_line == 0)
		{
			u32_t finish_count = (wb_status >> 12) & 0x7;
			// ÿ�������Ϊ2, ����ż���й�?
			//XM_printf ("finish_count=%d\n", finish_count);
	  if( finish_count >= 2 )
			{
				// ���� FIFO ��֡�ж� ��ȴû�� ���һ������֡������
				Y_finish  = rISP_SCALE_WB_FINISH_YADDR;
				UV_finish = rISP_SCALE_WB_FINISH_UADDR;
				(void)(UV_finish);
				
				if(Y_finish == NULL && UV_finish == NULL)
				{
					isp_scalar_recovery ();
					//return;
				}
				else
				{
				
					// �����֡Y��ַ�����뵱ǰ֡Y��ַһ��, ȡ���ڼĴ�����ȡ��ʱ��
					// ѹ�뵽֡����
					isp_scalar_frame_notify (Y_finish);
				}
			}
		}
	}
	if( status & write_back_bresp_error_interrupt)
	{
		rISP_SCALE_CLCD_INT_CLR = write_back_bresp_error_interrupt_clear;
		//XM_printf ("isp scalar write_back_bresp_error\n");
	}
	if( status & write_back_middle_finish_interupt)
	{
		rISP_SCALE_CLCD_INT_CLR = write_back_middle_finish_interupt_clear;
		if(isp_scalar_parameters.mid_line_user_callback)
		{
			(*isp_scalar_parameters.mid_line_user_callback)(isp_scalar_parameters.mid_line_user_data);
		}
		else
		{
			// ȱʡ����
			unsigned int current_used_y_addess = rISP_SCALE_WB_DEST_YADDR;
			unsigned int current_used_uv_addess = rISP_SCALE_WB_DEST_UADDR;
			(void)(current_used_uv_addess);
			if(current_used_y_addess)
				isp_scalar_frame_notify (current_used_y_addess);
		}
	}
   	
	int y_uv_pop_error = 0;
	
	// Y/UV Pop Error����ʱ��Ӧ���sensor������������.
	// Ӧ�ʵ�����������������ʹY/UV Pop Error���ٳ���
	if( wb_status & (1 << 1) )
	{
		// bit.1 Y_dest_addr_fifo pop error
		clr_wb_status |= (1 << 1);
		y_uv_pop_error = 1;
		XM_printf ("Y pop error\n");
	}
	if( wb_status & (1 << 6) )
	{
		// bit.6 UV_dest_addr_fifo pop error
		clr_wb_status |= (1 << 6);
		y_uv_pop_error = 1;
		XM_printf ("UV pop error\n");
	}
	if(y_uv_pop_error)
	{
		isp_scalar_recovery ();
	}
	
	/*
   if(wb_status & ((1<<0)|(1<<5)) )
   {
      // bit.3 = push
      clr_wb_status |=  wb_status & ((1<<0)|(1<<5))  ;
      XM_printf("push error!\n");
   }
   */
   
   if(wb_status & (1 << 0))
   {
      // bit.0 = Y_dest_addr_fifo push error
      clr_wb_status |=  (1 << 0);
      //XM_printf("Y push error!\n");
   }	
	if(wb_status & (1 << 5))
   {
      // bit.5 = UV_dest_addr_fifo push error
      clr_wb_status |=  (1 << 5);
      //XM_printf("UV push error!\n");
   }	

   // ����һ����������֡ ������һ����ַ��ȴ������finish �� ѹջ
   if(wb_status & (1 << 16))
   {
		// bit.16 Pix_abort
      clr_wb_status |= wb_status & (1 << 16) ;
      //XM_printf("pix abort!\n");
   }
	
   if(wb_status & (1 << 11))
   {
      // bit.11 finish pop error
      clr_wb_status |= wb_status & (1 << 11) ;
      //XM_printf("finish pop error!!!\n");
   }   
   if(wb_status & (1 << 10))
   {
      // bit.10 finish push error
      clr_wb_status |= wb_status & (1 << 10) ;
      //XM_printf("finish push error!!!\n");
   }
   rISP_SCALE_WB_STATUS = clr_wb_status ;	
	//rISP_SCALE_INT_STATUS = status;
}

//#define	ISP_SCALAR_DEBUG

extern int sys_clk_check (unsigned int clk);

int xm_isp_scalar_run (xm_isp_scalar_configuration_parameters  *scalar_parameters )
{
	int ret;
		
	unsigned int ticket_to_timeout;
	if(!isp_scaler_inited)
	{
		return XM_ISP_SCALAR_ERRCODE_DEV_NO_INIT;
	}
	
	ticket_to_timeout = XM_GetTickCount() + 200;
	while(1)
	{
		// ���ISPʱ���Ƿ�������
		// ISP-Scalar������ISPʱ��
		if(sys_clk_check(Isp_clk_enable))
			break;
		if(XM_GetTickCount() >= ticket_to_timeout)
			break;
		OS_Delay (30);
	}
	// ���ISPʱ���Ƿ�������
	if(!sys_clk_check(Isp_clk_enable))
	{
		printf ("xm_isp_scalar_run failed, isp clock don't run before\n");
		return -1;
	}
	
	OS_Use (&isp_scalar_access_sema);

	do
	{
		if(isp_scalar_object != (-1))
		{
			ret = XM_ISP_SCALAR_ERRCODE_OBJ_RE_CREATE;
			XM_printf("xm_isp_scalar_run failed, object created before\n");
			break;
		}
		
		XM_printf ("xm_isp_scalar_run\n");
				
		ISP_SCALAR_STOP;
		irq_disable (ISP_SCALE_INT);
		isp_scale_clock_stop ();
		sys_soft_reset (softreset_isp_scale);
		isp_scale_clock_init ();
		rISP_SCALE_WB_CTL = 0;
		
		ret = xm_isp_scalar_config (scalar_parameters);
		if(ret != XM_ISP_SCALAR_ERRCODE_OK)
		{
			ret = XM_ISP_SCALAR_ERRCODE_ILLEGAL_PARA;
			XM_printf("xm_isp_scalar_run failed, xm_isp_scalar_config NG\n");
			break;
		}
		
		xm_isp_scalar_buffer_clear ();
		
		// �����ӵ���֡��
		isp_scalar_frames_to_discard_to_remove_flash = MAX_FRAME_COUNT_TO_DISCARD_TO_REMOVE_FLASH;
		
		// ���� ISP Scale ֮ǰ ���ж� 
		request_irq (ISP_SCALE_INT, PRIORITY_FIFTEEN,  isp_scalar_int_handler );
	
	#ifdef ISP_SCALAR_DEBUG
		u32_t addr = ISP_SCALE_BASE;
		while(addr <= (ISP_SCALE_BASE + 0x7C))
		{
			XM_printf ("0x%08x : 0x%08x\n", addr, *(volatile u32_t *)addr);
			addr += 4;	
		}
	#endif
		
		// ����ISP Scalar
		//rISP_SCALE_EN		= 0x03;
		isp_scalar_object = isp_scalar_index;
		isp_scalar_index ++;	// ָ����һ�����
		ret = isp_scalar_object;
	} while(0);
		
	OS_Unuse (&isp_scalar_access_sema);
	
	return ret;
}

// ע��һ��scalar֡��Y/UV��ַ���ص�������˽�����ݾ��
int xm_isp_scalar_register_user_buffer (	int scalar_object,
													   u32_t y_addr, 
														u32_t uv_addr,
														void	(*data_ready_user_callback) (void *user_private_data),
														void  (*frame_free_user_callback) (void *user_private_data),
														void *user_private_data
													)
{
	int ret = -1;
	xm_isp_scalar_buffer_s *scalar_buffer = NULL;
	
	if(!isp_scaler_inited)
	{
		XM_printf ("xm_isp_scalar_register_user_buffer failed, isp_scaler don't init before\n");
		return -1;
	}
	
	if(y_addr == 0 || uv_addr == 0)
	{
		XM_printf ("xm_isp_scalar_register_user_buffer failed, y_addr or uv_addr is NULL\n");
		return -1;
	}
	if(data_ready_user_callback == 0 || user_private_data == 0)
	{
		XM_printf ("xm_isp_scalar_register_user_buffer failed, callback or private data is NULL\n");
		return -1;
	}
	
	XM_lock();	// �����жϷ����е���	
	//OS_Use (&isp_scalar_access_sema);
	do 
	{
		if(isp_scalar_object == (-1) || scalar_object != isp_scalar_object)
		{
			XM_printf ("xm_isp_scalar_register_user_buffer failed, illegal scalar object %d\n", scalar_object);
			break;			
		}
		
		// �жϴ����������isp_scalar_packet_free, ��˽�ֹ�ж�
		//XM_lock();		
		// ������fifo�Ƿ����. ISP Scalar�������4��Ӳ��FIFOѹ��
		if(queue_empty (&isp_scalar_packet_free))
		{
			//XM_unlock ();
			XM_printf ("xm_isp_scalar_register_user_buffer failed, resource busy\n");
			break;
		}
		
		scalar_buffer = (xm_isp_scalar_buffer_s *)queue_delete_next (&isp_scalar_packet_free);
		scalar_buffer->id = ISP_SCALAR_ID;
		scalar_buffer->window_y = y_addr;
		scalar_buffer->window_uv = uv_addr;
		scalar_buffer->data_ready_user_callback = data_ready_user_callback;
		scalar_buffer->frame_free_user_callback = frame_free_user_callback;
		scalar_buffer->user_private_data = user_private_data;
		
		isp_scalar_push_hardware_address (scalar_buffer);
		//XM_unlock ();
		
		ret = 0;
	}while (0);
	
	XM_unlock ();
	//OS_Unuse (&isp_scalar_access_sema);
	
	return ret;
}

// ɾ��scalar����
int xm_isp_scalar_delete (int scalar_object)
{
	xm_isp_scalar_buffer_s *buffer;
	if(!isp_scaler_inited)
	{
		XM_printf ("xm_isp_scalar_delete failed, isp_scalar don't create before\n");
		return -1;
	}	
	
	OS_Use (&isp_scalar_access_sema);
	if(scalar_object != isp_scalar_object)
	{
		OS_Unuse (&isp_scalar_access_sema);
		XM_printf ("xm_isp_scalar_delete failed, invalid scalar-object (%d)\n", scalar_object);
		return -1;
	}
	irq_disable (ISP_SCALE_INT);
	XM_printf ("xm_isp_scalar_delete, obj=%d\n", scalar_object);
	ISP_SCALAR_STOP;
	// ��ʱ100ms(����һ֡)ȷ��ISP SCALAR��ȫֹͣ
	OS_Delay (100);
	isp_scale_clock_stop ();
	OS_Delay (1);
	XM_lock();
	while(!queue_empty(&isp_scalar_packet_fifo))
	{
		buffer = (xm_isp_scalar_buffer_s *)queue_delete_next (&isp_scalar_packet_fifo);
		if(buffer->id != ISP_SCALAR_ID)
		{
			XM_printf ("illegal isp frame(0x%08x), id(%d) invalid\n", buffer, buffer->id);
		}
		//if(buffer->data_ready_user_callback)
		if(buffer->frame_free_user_callback)
		{
			//(*buffer->data_ready_user_callback)(buffer->user_private_data);
			(*buffer->frame_free_user_callback)(buffer->user_private_data);
		}
		buffer->id = 0;
		queue_insert ((queue_s *)buffer, &isp_scalar_packet_free);
	}
	XM_unlock();

	isp_scalar_object = -1;

	OS_Unuse (&isp_scalar_access_sema);
	return 0;
}

int xm_isp_scalar_create (unsigned int video_w, unsigned int video_h)
{	
	int ret;
	unsigned int video_format;
	xm_isp_scalar_configuration_parameters scalar_parameters;
	if(video_w == 0 || video_h == 0)
	{
		XM_printf ("xm_isp_scalar_create failed, illegal video_w(%d) or video_h(%d)\n", video_w, video_h);
		return -1;
	}
	
	//XM_printf ("xm_isp_scalar_create w=%d, h=%d\n", video_w, video_h);
		
	memset (&scalar_parameters, 0, sizeof(scalar_parameters));
	
	scalar_parameters.src_channel = XM_ISP_SCALAR_SRC_CHANNEL_ISP;
	scalar_parameters.src_hsync_polarity = XM_ISP_SCALAR_SRC_SYNC_PLOARITY_HIGH_LEVEL;
	#ifdef EN_RN6752M1080N_CHIP
	scalar_parameters.src_vsync_polarity = XM_ISP_SCALAR_SRC_SYNC_PLOARITY_LOW_LEVEL;
	#else
	scalar_parameters.src_vsync_polarity = XM_ISP_SCALAR_SRC_SYNC_PLOARITY_HIGH_LEVEL;
	#endif
	// Դͼ���������
	video_format = isp_get_video_format();
	if(video_format == 0)
		scalar_parameters.src_format = XM_ISP_SCALAR_FORMAT_Y_UV420;
	else if(video_format == 1)
		scalar_parameters.src_format = XM_ISP_SCALAR_FORMAT_Y_UV422;
	else if(video_format == 2)	
		scalar_parameters.src_format = XM_ISP_SCALAR_FORMAT_YUV420;
	else
		scalar_parameters.src_format = XM_ISP_SCALAR_FORMAT_YUV422;
	
	scalar_parameters.src_width = (u16_t)isp_get_video_width ();
	scalar_parameters.src_height = (u16_t)isp_get_video_height ();
	scalar_parameters.src_stride = (u16_t)isp_get_video_width ();
	scalar_parameters.src_window_x = 0;
	scalar_parameters.src_window_y = 0;
	scalar_parameters.src_window_width = scalar_parameters.src_width;
	scalar_parameters.src_window_height = scalar_parameters.src_height;
	
	// Ŀ��ͼ���������
	scalar_parameters.dst_format = XM_ISP_SCALAR_FORMAT_Y_UV420;
	scalar_parameters.dst_width = (u16_t)video_w;
	scalar_parameters.dst_height = (u16_t)video_h;
	scalar_parameters.dst_stride = (u16_t)video_w;
	scalar_parameters.dst_window_x = 0;
	scalar_parameters.dst_window_y = 0;
	scalar_parameters.dst_window_width = (u16_t)video_w;
	scalar_parameters.dst_window_height = (u16_t)video_h;
	ret =  xm_isp_scalar_run (&scalar_parameters);

	XM_printf("isp_scalar src w=%d,h=%d\n",scalar_parameters.src_width,scalar_parameters.src_height);
	XM_printf ("isp_scalar obj w=%d, h=%d, ret = %d\n", video_w, video_h, ret);
	//XM_printf ("xm_isp_scalar_create, ret = %d\n", ret);
	return ret;
}

int xm_isp_scalar_create_ex (
									  unsigned int src_channel,			// Դͼ�������ͨ��ѡ�� (�ڲ�isp��� ���� �ⲿitu601����)
									  unsigned int	src_hsync_polarity,	// Դͼ�����ͬ���źż���ѡ��
									  unsigned int	src_vsync_polarity,	// Դͼ�����ͬ���źż���ѡ��
									  unsigned int	src_format,				// YUV422/Y_UV422/YUV420/Y_UV420
									  unsigned int src_ycbcr_sequence,	// y/cbcr��˳��, 0:uyvy 1:yuyv
									  
									  unsigned int	src_width,				// Դͼ��Ŀ��
									  unsigned int	src_height,				// Դͼ��ĸ߶�
									  unsigned int	src_stride,				// Դͼ����п��
									  
									  // Դ���ڶ���(��������, ���϶���)
									  unsigned int	src_window_x,			// Դ���������Դͼ��ԭ���X����ƫ��
									  unsigned int	src_window_y,			// Դ���������Դͼ��ԭ���Y����ƫ��
									  unsigned int	src_window_width,		// Դ���ڵĿ��
									  unsigned int	src_window_height,	// Դ���ڵĸ߶�
									  
									  // ���ͼ����
									  unsigned int dst_width,				// ���ͼ��Ŀ��
									  unsigned int dst_height,				// ���ͼ��ĸ߶�
									  unsigned int dst_stride,				// ���ͼ����п��
									  
									  unsigned int mid_line_counter,
									  void (*mid_line_callback)(void *),
									  void * mid_line_user_data
									  )
{	
	int ret;
	unsigned int video_format;
	xm_isp_scalar_configuration_parameters scalar_parameters;
	
	//XM_printf ("xm_isp_scalar_create w=%d, h=%d\n", video_w, video_h);
		
	memset (&scalar_parameters, 0, sizeof(scalar_parameters));
	
	scalar_parameters.src_channel = src_channel;
	scalar_parameters.src_hsync_polarity = src_hsync_polarity;
	scalar_parameters.src_vsync_polarity = src_vsync_polarity;
	
	// Դͼ���������
	scalar_parameters.src_format = src_format;
	scalar_parameters.src_ycbcr_sequence = src_ycbcr_sequence;
	
	scalar_parameters.src_width = (u16_t)src_width;
	scalar_parameters.src_height = (u16_t)src_height;
	scalar_parameters.src_stride = (u16_t)src_stride;
	scalar_parameters.src_window_x = src_window_x;
	scalar_parameters.src_window_y = src_window_y;
	scalar_parameters.src_window_width = src_window_width;
	scalar_parameters.src_window_height = src_window_height;
	
	// Ŀ��ͼ���������
	scalar_parameters.dst_format = XM_ISP_SCALAR_FORMAT_Y_UV420;
	scalar_parameters.dst_width = (u16_t)dst_width;
	scalar_parameters.dst_height = (u16_t)dst_height;
	scalar_parameters.dst_stride = (u16_t)dst_stride;
	scalar_parameters.dst_window_x = 0;
	scalar_parameters.dst_window_y = 0;
	scalar_parameters.dst_window_width = (u16_t)dst_width;
	scalar_parameters.dst_window_height = (u16_t)dst_height;
	
	scalar_parameters.mid_line = mid_line_counter;
	scalar_parameters.mid_line_user_callback = mid_line_callback;
	scalar_parameters.mid_line_user_data = mid_line_user_data;
	ret =  xm_isp_scalar_run (&scalar_parameters);

	XM_printf("isp_scalar src w=%d,h=%d\n",scalar_parameters.src_width,scalar_parameters.src_height);
	//XM_printf ("xm_isp_scalar_create, ret = %d\n", ret);
	return ret;
}

