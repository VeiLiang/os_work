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
#include <xm_type.h>
#include <hw_osd_layer.h>
#include "RTOS.H"
#include <xm_file.h>
#include <xm_base.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ark1960_testcase.h>
#include "xm_core.h"
#include <xm_osd_layer.h>
#include <xm_osd_framebuffer.h>
#include "xm_isp_scalar.h"
#include <xm_queue.h>
#include <xm_dev.h>

static queue_s ready_framebuffer;		// 数据已准备好的framebuffer

int xm_isp_scalar_test (void)
{	
	xm_isp_scalar_configuration_parameters scalar_parameters;
	unsigned int osd_w, osd_h;
	
	queue_initialize (&ready_framebuffer);
	
	// 适用于LCD显示的尺寸
	osd_w = XM_lcdc_osd_get_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	osd_h = XM_lcdc_osd_get_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	
	memset (&scalar_parameters, 0, sizeof(scalar_parameters));
	
	scalar_parameters.src_channel = XM_ISP_SCALAR_SRC_CHANNEL_ISP;
	scalar_parameters.src_hsync_polarity = XM_ISP_SCALAR_SRC_SYNC_PLOARITY_HIGH_LEVEL;
	scalar_parameters.src_vsync_polarity = XM_ISP_SCALAR_SRC_SYNC_PLOARITY_HIGH_LEVEL;
	
	unsigned int video_format = isp_get_video_format();
	
	//video_format = 0;
	if(video_format == 0)
		scalar_parameters.src_format = XM_ISP_SCALAR_FORMAT_Y_UV420;
	else if(video_format == 1)
		scalar_parameters.src_format = XM_ISP_SCALAR_FORMAT_Y_UV422;
	else if(video_format == 2)	
		scalar_parameters.src_format = XM_ISP_SCALAR_FORMAT_YUV420;
	else
		scalar_parameters.src_format = XM_ISP_SCALAR_FORMAT_YUV422;
	scalar_parameters.src_width = isp_get_video_width ();
	scalar_parameters.src_height = isp_get_video_height ();
	scalar_parameters.src_stride = isp_get_video_width ();
	scalar_parameters.src_window_x = 0;
	scalar_parameters.src_window_y = 0;
	scalar_parameters.src_window_width = scalar_parameters.src_width;
	scalar_parameters.src_window_height = scalar_parameters.src_height;
	
	// 初始化LCD显示的设置(Scalar目标)
	scalar_parameters.dst_format = XM_ISP_SCALAR_FORMAT_Y_UV420;
	scalar_parameters.dst_width = osd_w;
	scalar_parameters.dst_height = osd_h;
	//scalar_parameters.dst_stride = 800;
	scalar_parameters.dst_stride = MAX_SCALAR_WIDTH;
	scalar_parameters.dst_window_x = 0;
	scalar_parameters.dst_window_y = 0;
	scalar_parameters.dst_window_width = osd_w;
	scalar_parameters.dst_window_height = osd_h;
	xm_isp_scalar_init ();
	
	xm_isp_scalar_run (&scalar_parameters);
	
	return 0;
}

// 中断服务中调用
static void scalar_frame_ready_user_callback (void *private_data)
{
	XM_OSD_FRAMEBUFFER *yuv_framebuffer = (XM_OSD_FRAMEBUFFER *)private_data;
	queue_insert ((queue_s *)yuv_framebuffer, &ready_framebuffer);
	
	//XM_osd_framebuffer_close (yuv_framebuffer, 0);
}

int isp_scalar_init(void)
{
	unsigned int osd_w, osd_h;
	XM_OSD_FRAMEBUFFER *new_framebuffer;	// 新获取的framebuffer
	XM_OSD_FRAMEBUFFER *lcd_framebuffer;	// 正在显示使用的framebuffer
	
	osd_w = XM_lcdc_osd_get_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	osd_h = XM_lcdc_osd_get_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	
   /* 使用源 601 */
   /* 输出 Y_UV420 数据 */
   //rSYS_DEVICE_CLK_CFG0 |= (0x1<<9);		// itu_clk_b_inv, 20150605的ISP scale使用反相时钟
   rSYS_DEVICE_CLK_CFG0 &= ~(0x1<<9);	// itu_clk_b
	xm_isp_scalar_test ();
	
	// 构建视频framebuffer链表
	while(1)
	{
		lcd_framebuffer = XM_osd_framebuffer_create (0,
							XM_OSD_LAYER_0,
							XM_OSD_LAYER_FORMAT_Y_UV420,
							osd_w,
							osd_h,
							NULL,
							0,
							0		// clear_framebuffer_context
							);
		if(lcd_framebuffer)
		{
			uint8_t *data[4];
			XM_osd_framebuffer_get_buffer (lcd_framebuffer, data);
			isp_scalar_register_user_buffer ((u32_t)data[0], (u32_t)data[1],
															scalar_frame_ready_user_callback, 
															lcd_framebuffer
															);
		}
		else
			break;
	}
	
	
	lcd_framebuffer = NULL;
	new_framebuffer = NULL;
	while(1)
	{
		// 检查是否存在新的数据已准备好的framebuffer帧
		XM_lock ();
		if(queue_empty (&ready_framebuffer))
			new_framebuffer = NULL;
		else
			new_framebuffer = (XM_OSD_FRAMEBUFFER *)queue_delete_next (&ready_framebuffer);
		XM_unlock ();
		
		if(new_framebuffer)
		{
			// 数据已准备OK的新的framebuffer
			XM_osd_framebuffer_close (new_framebuffer, 0);
			
			lcd_framebuffer = XM_osd_framebuffer_create (0,
								XM_OSD_LAYER_0,
								XM_OSD_LAYER_FORMAT_Y_UV420,
								osd_w,
								osd_h,
								NULL,
								0,
								0		// clear_framebuffer_context
								);
			if(lcd_framebuffer)
			{
				uint8_t *data[4];
				XM_osd_framebuffer_get_buffer (lcd_framebuffer, data);
				isp_scalar_register_user_buffer ((u32_t)data[0], (u32_t)data[1],
																scalar_frame_ready_user_callback, 
																lcd_framebuffer
																);
			}
		}
		else
		{
			OS_Delay (1);
		}
		
	}
	return 0;
}