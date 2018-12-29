// deinterlace.c
#include "hardware.h"
#include "deinterlace.h"
#include <stdio.h>
#include <stdlib.h>
#include "cpuClkCounter.h"

#define	_USE_DEINTERLACE_INTERRUPT_

#ifdef _USE_DEINTERLACE_INTERRUPT_
static OS_EVENT deinterlace_event;
static volatile int deinterlace_status = 0;
#endif
static int deinterlace_inited = 0;


static void _deinterlace_delay (int count)
{
	while(count > 0)
	{
		count --;
	}
}

static void deinterlace_reset (void)
{
	// 1) 内部软复位
	// 22	R/W	1	softreset_deinter
	rDe_ctrl0 = (1 << 0);	// 软复位
	_deinterlace_delay (10);
	rDe_ctrl0 = (0 << 0);	// 复位清除
	_deinterlace_delay (10);
	
	// 复位前关闭时钟
	sys_clk_disable (deinterlace_xclk_enable);
	// 2) IP RESET
	// 仅使用以下IP RESET在某种情况下会导致整个系统挂起
	// 需要先使用1)内部软复位, 然后再使用2) IP RESET
	sys_soft_reset (softreset_deinter);
	
	sys_clk_enable (deinterlace_xclk_enable);
	
	rDe_Int_clr = 0x03;
}

#ifdef _USE_DEINTERLACE_INTERRUPT_
static void deinterlace_interrupt_handler (void)
{
	unsigned int raw_int = rDe_Raw_Int;
	deinterlace_status = 0;
	if(raw_int & (1 << 0))
	{
		// 场转换结束中断 Field finish interrupt
		rDe_Int_clr = 0x01;
		deinterlace_status = 0x01;
	}
	if(raw_int & (1 << 1))
	{
		// 异常 AXI_wr error interrupt
		rDe_Int_clr = 0x02;
		deinterlace_status = 0x02;
	}
	//XM_printf ("isr %d\n", deinterlace_status);
	OS_EVENT_Set (&deinterlace_event);
}
#endif

void deinterlace_init (void)
{
#ifdef _USE_DEINTERLACE_INTERRUPT_
	deinterlace_status = 0;
	OS_EVENT_Create (&deinterlace_event);
#endif
	deinterlace_reset ();
	rDe_Int_mask = 0;

	deinterlace_inited = 1;

#ifdef _USE_DEINTERLACE_INTERRUPT_
	rDe_Int_mask = 0x03;
	request_irq (DEINTERLACE_INT, PRIORITY_FIFTEEN, deinterlace_interrupt_handler);
#endif	

}

void deinterlace_exit (void)
{
	rDe_Int_mask = 0;
	irq_disable (DEINTERLACE_INT);
#ifdef _USE_DEINTERLACE_INTERRUPT_
	OS_EVENT_Delete (&deinterlace_event);
#endif
	deinterlace_inited = 0;
}

int deinterlace_process_timeout (unsigned int deinterlace_size, 
							 unsigned int data_mode,
							 unsigned int deinterlace_type,
							 unsigned int deinterlace_field,
							 unsigned char *src_field_addr_0,
							 unsigned char *src_field_addr_1,
							 unsigned char *src_field_addr_2,
							 unsigned char *dst_y_addr,
							 unsigned char *dst_u_addr,
							 unsigned char *dst_v_addr,
							 int timeout
							)
{
	unsigned int pixel_per_line;
	unsigned int total_line;
	unsigned int pn;
	unsigned int denoise_bypass;
	unsigned int stride;
	unsigned int only_wr_1_field;
	unsigned int field;
	int ret = DEINTERLACE_SUCCESS;
	
	if(deinterlace_inited == 0)
		return DEINTERLACE_PARA_ERROR;
	
	if(src_field_addr_0 == NULL || src_field_addr_1 == NULL || src_field_addr_2 == NULL)
	{
		XM_printf ("illegal deinterlace src field address\n");
		return DEINTERLACE_PARA_ERROR;
	}
	
	if(	deinterlace_field != DEINTERLACE_FIELD_ODD
		&&	deinterlace_field != DEINTERLACE_FIELD_EVEN)
	{
		XM_printf ("invalid deinterlace field (%d)\n", deinterlace_field);
		return DEINTERLACE_PARA_ERROR;				
	}
	field = deinterlace_field;
	
	if(deinterlace_size == DEINTERLACE_LINE_SIZE_960H)
		pixel_per_line = 120 * (1 + data_mode);
	else if(deinterlace_size == DEINTERLACE_LINE_SIZE_720H)
		pixel_per_line = 90 * (1 + data_mode);
	else
	{
		XM_printf ("invalid deinterlace size (%d)\n", deinterlace_size);
		return DEINTERLACE_PARA_ERROR;
	}
	
	if(data_mode == DEINTERLACE_DATA_MODE_420)
	{
		denoise_bypass = 1;
		stride = pixel_per_line;
		only_wr_1_field = 1;
	}
	else if(data_mode == DEINTERLACE_DATA_MODE_422)
	{
		// 检查目标YUV地址
		if(dst_y_addr == NULL || dst_u_addr == NULL || dst_v_addr == NULL)
		{
			XM_printf ("illegal deinterlace dst Y U V address\n");
			return DEINTERLACE_PARA_ERROR;
		}
		denoise_bypass = 1;
		stride = 0;
		only_wr_1_field = 0;
	}
	else
	{
		XM_printf ("invalid deinterlace data mode (%d)\n", data_mode);
		return DEINTERLACE_PARA_ERROR;
	}
	
	if(deinterlace_type == DEINTERLACE_TYPE_PAL)
	{
		pn = 0;
		total_line = 288;
	}
	else if(deinterlace_type == DEINTERLACE_TYPE_NTSC)
	{
		pn = 1;
		total_line = 240;
	}
	else
	{
		XM_printf ("invalid deinterlace type (%d)\n", deinterlace_type);
		return DEINTERLACE_PARA_ERROR;		
	}		

   rDe_ctrl0	=  	(0x0 << 29)
						|  (only_wr_1_field << 28)  	  // field_1 only_wr_1_field                      	
						|  (pixel_per_line << 20)       // pixel_pl
						|  (total_line << 11)           // total_line
						|  (stride << 3)                // stride
						|  (data_mode << 2)             // data_mode
						|  (field << 1)                 // field
					;	

	
	//rDe_ctrl1 = 0x0800070a;
	//rDe_ctrl2 = 0x30100230;
	rDe_ctrl1 = 0x0000700d;
	rDe_ctrl2 = 0x30004230;

	// denoise bypass  pn: 1:n display_motion line_intra global_cnt display_mv_0
	rDe_ctrl3 = 				  (denoise_bypass << 15)
	                                | (0x2 << 16)
									| (pn << 13) 
									| (0x1 << 11) //
									| (0x1 << 9) //
									| (0x0 << 8)
									| (0x0 << 7)
									| (0x0 << 6)
									| (0x0 << 5)
									| (0x1 << 3)
									| (0x1 << 1)
									;
   
    
    rFilm_mode_ctrl = (unsigned int)(0x0 << 31);
		
	rDe_S_addr_0 = (unsigned int)src_field_addr_0;
	rDe_S_addr_1 = (unsigned int)src_field_addr_1;
	rDe_S_addr_2 = (unsigned int)src_field_addr_2;
		
	// YUV422模式下的帧输出缓冲区设置，YUV420不需要
	if(data_mode == DEINTERLACE_DATA_MODE_422)
	{
		rDe_des_addr_y = (unsigned int)dst_y_addr;		// dy_0  when filed = 1 ,data_mode = 0  then dy_0 = s0_0 
                                             //      when filed = 0 ,data_mode = 0  then dy_0 = s2_0 
		rDe_des_addr_u = (unsigned int)dst_u_addr;
		rDe_des_addr_v = (unsigned int)dst_v_addr;
	}
        
	//	pingpong addr fetch 
	rAddr_switch_mode = 0x00;
   
	// 清除中断
	//	1		W		0			AXI_wr error interrupt clr : 
	//								1:clear the interrupt
	//	0		W		0			Field finish interrupt clr: 
	//								1: clear the interrupt 
	rDe_Int_clr = 0x03;
	
	// start de-interlace
	rDe_start = 0x01;

#ifdef _USE_DEINTERLACE_INTERRUPT_
	if(OS_EVENT_WaitTimed (&deinterlace_event, timeout))
	{
		// timeout
		XM_printf ("3D de-interlace timeout\n");
		deinterlace_reset ();
		ret = DEINTERLACE_TIMEOUT;
	}
	else if(deinterlace_status == 2)
	{
		// 异常
		deinterlace_reset ();
		ret = DEINTERLACE_AXI_ERROR;
	}
	else //if(deinterlace_status == 1)
	{
		// OK
		ret = DEINTERLACE_SUCCESS;
	}
#else
	while(1)
	{
		unsigned int rdata = rDe_Raw_Int;
		if(rdata & 0x01)
		{
			rDe_Int_clr = 0x01;
			break;
		}
		else if(rdata & 0x02)
		{
			// AXI_wr error interrupt
			XM_printf ("AXI_wr error interrupt\n");
			rDe_Int_clr = 0x02;
			ret = -1;
			deinterlace_reset ();
			break;
		}
	}
#endif
 
	return ret;
}

int deinterlace_process (unsigned int deinterlace_size, 
							 unsigned int data_mode,
							 unsigned int deinterlace_type,
							 unsigned int deinterlace_field,
							 unsigned char *src_field_addr_0,
							 unsigned char *src_field_addr_1,
							 unsigned char *src_field_addr_2,
							 unsigned char *dst_y_addr,
							 unsigned char *dst_u_addr,
							 unsigned char *dst_v_addr
							)
{
	return deinterlace_process_timeout (deinterlace_size, 
										  data_mode,
										  deinterlace_type,
										  deinterlace_field,
										  src_field_addr_0,
										  src_field_addr_1,
										  src_field_addr_2,
										  dst_y_addr,
										  dst_u_addr,
										  dst_v_addr,
										  500);
}