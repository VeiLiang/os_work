//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_video_task.c
//	  视频任务
//
//	Revision history
//
//		2012.09.06	ZhuoYongHong Initial version
//
//****************************************************************************
#include <string.h>
#include "hardware.h"
#include <xm_user.h>
#include <xm_base.h>
#include <xm_gdi.h>
#include <xm_semaphore.h>
#include <xm_printf.h>
#include <assert.h>
#include <xm_osd_layer.h>
#include <xm_osd_framebuffer.h>
#include <xm_video_task.h>
#include <xm_h264_codec.h>
#include "xm_icon_manage.h"
#include "xm_isp_scalar.h"
#include <xm_queue.h>
#include <xm_dev.h>
#include <stdio.h>
#include "arkn141_isp.h"
#include "xm_core.h"
#include "arkn141_scale.h"
#include "lcd.h"
#include "xm_rotate.h"
#include "rtos.h"		// 任务相关函数
#include "xm_proj_define.h"
#include "rom.h"
#include "gpio.h"

extern int PowerOnShowLogo;
void rotate_degree90 (unsigned char *y_uv420_src,
						  unsigned char *y_uv420_dst,
						  int width, // 输入图像大小  宽度与高度
						  int height,
						  int o_width, // 输入图像大小  宽度与高度
						  int o_height);


#define	VIDEO_ISP_SCALAR_FRAME_COUNT		4

static int	_XMSYS_VideoRenderVideoFrameIntoDisplay (	unsigned char *frame_data,
																unsigned int frame_w,
																unsigned int frame_h
															);



static OS_TASK TCB_VideoTask;
__no_init static OS_STACKPTR int StackVideoTask[XMSYS_VIDEO_TASK_STACK_SIZE/4];          /* Task stacks */

static queue_s ready_framebuffer;		// 数据已准备好的framebuffer

static unsigned char *video_framebuffer;
static unsigned int video_w, video_h;
static unsigned int video_playback_ticket;
u32_t SensorData_Source;

#pragma data_alignment=32
#define	ITU656_IN_STATIC_FIFO_SIZE		((ITU656_IN_FRAME_COUNT+1) * (ITU656_IN_MAX_WIDTH * ITU656_IN_MAX_HEIGHT * 3/2 + 2048))
#define	ITU656_IN_MONITOR_SPACE_SIZE		512*1024	
// 中间预留1个512*1024字节的缓冲，用于监控itu601in是否存在接收溢出并保护邻接的isp scalar缓存内容在一定的时间内(3.95ms 720p 60fps itu601 in)不会被覆盖
__no_init char itu656_isp_scalar_static_fifo[ITU656_IN_STATIC_FIFO_SIZE + ITU656_IN_MONITOR_SPACE_SIZE + ISP_SCALAR_STATIC_FIFO_SIZE];

static OS_RSEMA video_task_semaphore;		// 访问信号量控制


#define	XMSYS_VIDEO_EVENT_SCALAR_FRAME_READY	0x01		// scalar帧数据已准备完成
#define	XMSYS_VIDEO_EVENT_TICKET					0x02

#define	XMSYS_VIDEO_EVENT_FRAME_RENDER			0x10		// 视频帧渲染

static OS_TIMER VideoTimer;
static int lcd_video_opened;

static int usb_video_opened;
static int skip_frames_to_on_lcd = 1;		// 第一次开机时使用


#ifdef ISP_SCALAR_USE_STATIC_FIFO
#pragma data_alignment=4096
__no_init char isp_scalar_static_fifo[ISP_SCALAR_STATIC_FIFO_SIZE];
#else

#endif
#pragma data_alignment=4096
__no_init static unsigned char assemble_video_buffer[320*240*3/2];		// 显示临时缓冲区

#define	SUB_WINDOW_W		320
#define	SUB_WINDOW_H		240

static OS_RSEMA assembly_semaphore;		// 图像拼接访问信号量控制

#if TULV
																					//	此缓冲区必须大于VIDEO显示缓冲区大
#if 0
#pragma data_alignment=4096
__no_init static unsigned char rotate_video_buffer[320*240*3/2];		// 旋转临时缓冲区
//__no_init static unsigned char assemble_video_buffer[1600*400*3/2];		// 拼接临时缓冲区
#endif

#define	DIRECT_USE_REAL_IMAGE

static int assembly_mode = XMSYS_ASSEMBLY_MODE_FRONT_ONLY;	// 图像拼接模式
//static int assembly_mode = XMSYS_ASSEMBLY_MODE_REAL_ONLY;
int ITU656_in=0;
int Reversing=0;


//回放视频播放了多长时间
unsigned int get_videodecoder_times(void)
{
	return video_playback_ticket;
}


void clear_videodecoder_times(void)
{
	video_playback_ticket = 0;
}


uchar GetReversingSta()
{
	return Reversing;
}
// 设置画面拼接模式
int XMSYS_VideoSetImageAssemblyMode (unsigned int image_assembly_mode)
{
	if(image_assembly_mode >= XMSYS_ASSEMBLY_MODE_COUNT)
		return -1;

	OS_Use (&assembly_semaphore);

	assembly_mode = image_assembly_mode;

	// 清除历史记录
	OS_Unuse (&assembly_semaphore);
	return 0;
}

// 返回当前视频图像拼接模式
unsigned int XMSYS_VideoGetImageAssemblyMode (void)
{
	return assembly_mode;
}

#endif


char *get_ch2_video_image()
{
	return assemble_video_buffer;
}


// 中断服务中调用

//#define	VIDEO_DEBUG	1

static void VideoTicketCallback (void)
{
	//XM_printf ("JpegTicketCallback\n");
	OS_SignalEvent(XMSYS_VIDEO_EVENT_TICKET, &TCB_VideoTask); /* 通知事件 */
	OS_RetriggerTimer (&VideoTimer);
}

// 实时视频显示
#if VIDEO_DEBUG
static unsigned int t_create, t_scalar, t_rotate, t_icon, t_close;
static unsigned int display_count;
#endif
BOOL First_ShowData = FALSE;
uchar perpipmode;
extern uchar pipmode;
extern u8_t ACC_Select;
extern uchar sensor2reverse;
extern uchar sensor1reverse;
void SetPerpipmode(uchar mode)
{
	perpipmode	= mode;
}

uchar GetPerpipmode()
{
	return perpipmode;
}
static void video_display (XMSYSSENSORPACKET *video_framepacket)
{
	unsigned int osd_w, osd_h;
    
    unsigned int window_x, window_y, window_w, window_h;
	XM_OSD_FRAMEBUFFER * yuv_framebuffer, *new_framebuffer;
    XMSYSSENSORPACKET *Itu656_framepacket = NULL;
    XMSYSSENSORPACKET *Itu656_motion_framepacket = NULL;

#if VIDEO_DEBUG
	XMINT64	t1, t2;
#endif
	if(video_framepacket == NULL)
		return;

	//if(	assembly_mode != XMSYS_ASSEMBLY_MODE_FRONT_ONLY
	//	&& assembly_mode != XMSYS_ASSEMBLY_MODE_REAL_ONLY	)
	//	return;

#if VIDEO_DEBUG
	t1 = XM_GetHighResolutionTickCount ();
#endif
	osd_w = XM_lcdc_osd_get_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	osd_h = XM_lcdc_osd_get_height(XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	new_framebuffer = XM_osd_framebuffer_create (0,
									XM_OSD_LAYER_0,
									XM_OSD_LAYER_FORMAT_Y_UV420,
									osd_w,
									osd_h,
									NULL,
									0,
									0		// clear_framebuffer_context
									);
#if VIDEO_DEBUG
	t2 = XM_GetHighResolutionTickCount();
	t_create += (unsigned int)(t2 - t1);
	t1 = t2;
#endif
	if(new_framebuffer)
	{
		//u8_t *data[4];
		//new_framebuffer->address = video_framepacket->buffer;
		dma_inv_range ((unsigned int)new_framebuffer->address,(unsigned int)new_framebuffer->address + osd_w * osd_h * 3/2);
		//XM_osd_framebuffer_get_buffer (new_framebuffer, data);
		
		#if 1
		unsigned char curch = AP_GetMenuItem(APPMENUITEM_CH);
		switch(curch)
		{
			int openwindow_x;
			int openwindow_y;
			int openwindow_width;
			int openwindow_height;
				
			case CH_AHD1:
				#ifdef EN_RN6752M1080N_CHIP
				openwindow_x = 10;
				openwindow_y = 40;
				openwindow_width = video_framepacket->width-10;
				openwindow_height = video_framepacket->height-40;
				#else
				openwindow_x = 10;
				openwindow_y = 0;
				openwindow_width = video_framepacket->width-10;
				openwindow_height = video_framepacket->height-0;
				#endif

		        arkn141_scalar_process (
								(unsigned char *)video_framepacket->buffer,		// 输入图像
								0,
								1,			// 输出窗口使能
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// 输出窗口位置定义
								video_framepacket->width, video_framepacket->height,		// 输入图像大小  宽度与高度
								osd_w, osd_h,						// 缩放后的图像大小  宽度与高度
								osd_w,
								FORMAT_Y_UV420,
								(unsigned char *)new_framebuffer->address,		// 输出图像
								(unsigned char *)new_framebuffer->address + osd_w * osd_h,
								(unsigned char *)new_framebuffer->address + osd_w * osd_h,
								0,
								0,
								0
								);
				break;

			case CH_AHD2:
			    Itu656_framepacket = NULL;
		        Itu656_framepacket = XMSYS_SensorGetCurrentPacket (1);
				#ifdef EN_RN6752M1080N_CHIP
				openwindow_x = 10;
				openwindow_y = 40;
				openwindow_width = Itu656_framepacket->width-10;
				openwindow_height = Itu656_framepacket->height-40;
				#else
				openwindow_x = 10;
				openwindow_y = 0;
				openwindow_width = Itu656_framepacket->width-10;
				openwindow_height = Itu656_framepacket->height-0;
				#endif
				
		        arkn141_scalar_process (
								(unsigned char *)Itu656_framepacket->buffer,		// 输入图像
								0,
								1,			// 输出窗口使能
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// 输出窗口位置定义
								Itu656_framepacket->width, Itu656_framepacket->height,		// 输入图像大小  宽度与高度
								osd_w, osd_h,						// 缩放后的图像大小  宽度与高度
								osd_w,
								FORMAT_Y_UV420,
								(unsigned char *)new_framebuffer->address,		// 输出图像
								(unsigned char *)new_framebuffer->address + osd_w * osd_h,
								(unsigned char *)new_framebuffer->address + osd_w * osd_h,
								0,
								0,
								0
								);
				break;

			case CH_V_AHD12://垂直分割
				//CH1
				#ifdef EN_RN6752M1080N_CHIP
				openwindow_x = 220;
				openwindow_y = 40;
				openwindow_width = 520;
				openwindow_height = video_framepacket->height-40;
				#else
				openwindow_x = 230;
				openwindow_y = 0;
				openwindow_width = 800;
				openwindow_height = video_framepacket->height-0;
				#endif
				arkn141_scalar_process (
								(unsigned char *)video_framepacket->buffer,		// 输入图像
								0,
								1,			// 输出窗口使能
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// 输出窗口位置定义
								video_framepacket->width, video_framepacket->height,		// 输入图像大小  宽度与高度
								osd_w/2, osd_h,						// 缩放后的图像大小  宽度与高度
								osd_w,
								FORMAT_Y_UV420,
								(unsigned char *)new_framebuffer->address,		// 输出图像
								(unsigned char *)new_framebuffer->address + osd_w * osd_h,
								(unsigned char *)new_framebuffer->address + osd_w * osd_h,
								0,
								0,
								0
				);

				//CH2
				Itu656_framepacket = NULL;
				Itu656_framepacket = XMSYS_SensorGetCurrentPacket (1);

				#ifdef EN_RN6752M1080N_CHIP
				openwindow_x = 220;
				openwindow_y = 40;
				openwindow_width = 520;
				openwindow_height = Itu656_framepacket->height-40;
				#else
				openwindow_x = 230;
				openwindow_y = 0;
				openwindow_width = 800;
				openwindow_height = Itu656_framepacket->height-0;
				#endif
				
				if(Itu656_framepacket) {
				    window_w = osd_w/2;
					window_h = osd_h;
					window_x = osd_w - window_w; 
					window_y = 0;
					window_x &= ~0xF;

					arkn141_scalar_process (
								(unsigned char *)Itu656_framepacket->buffer,		// 输入图像
								0,
								1,			// 输出窗口使能
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// 输出窗口位置定义
								Itu656_framepacket->width, Itu656_framepacket->height,		// 输入图像大小  宽度与高度
								window_w, window_h,					// 缩放后的图像大小  宽度与高度
								osd_w,
								FORMAT_Y_UV420,
								(unsigned char *)new_framebuffer->address + window_y * osd_w + window_x,		// 输出图像
								(unsigned char *)new_framebuffer->address + osd_w * osd_h + window_y/2 * osd_w + window_x,
								(unsigned char *)new_framebuffer->address + osd_w * osd_h + window_y/2 * osd_w + window_x,
								0,
								0,
								0
					);
				}
				break;

			case CH_V_AHD21://垂直分割
				//CH2
				Itu656_framepacket = NULL;
				Itu656_framepacket = XMSYS_SensorGetCurrentPacket (1);

				#ifdef EN_RN6752M1080N_CHIP
				openwindow_x = 220;
				openwindow_y = 40;
				openwindow_width = 520;
				openwindow_height = Itu656_framepacket->height-40;
				#else
				openwindow_x = 230;
				openwindow_y = 0;
				openwindow_width = 800;
				openwindow_height = Itu656_framepacket->height-0;
				#endif
				
				if(Itu656_framepacket) {
				    arkn141_scalar_process (
								(unsigned char *)Itu656_framepacket->buffer,		// 输入图像
								0,
								1,			// 输出窗口使能
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// 输出窗口位置定义
								Itu656_framepacket->width, Itu656_framepacket->height,		// 输入图像大小  宽度与高度
								osd_w/2, osd_h,					// 缩放后的图像大小  宽度与高度
								osd_w,
								FORMAT_Y_UV420,
								(unsigned char *)new_framebuffer->address,		// 输出图像
								(unsigned char *)new_framebuffer->address + osd_w * osd_h,
								(unsigned char *)new_framebuffer->address + osd_w * osd_h,
								0,
								0,
								0
								);
				}

				//CH1
		        window_w = osd_w/2;
		        window_h = osd_h;
		        window_x = osd_w - window_w; 
		        window_y = 0;
		        window_x &= ~0xF;

				#ifdef EN_RN6752M1080N_CHIP
				openwindow_x = 220;
				openwindow_y = 40;
				openwindow_width = 520;
				openwindow_height = video_framepacket->height-40;
				#else
				openwindow_x = 230;
				openwindow_y = 0;
				openwindow_width = 800;
				openwindow_height = video_framepacket->height-0;
				#endif
				arkn141_scalar_process (
								(unsigned char *)video_framepacket->buffer,		// 输入图像
								0,
								1,			// 输出窗口使能
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// 输出窗口位置定义
								video_framepacket->width, video_framepacket->height,		// 输入图像大小  宽度与高度
								window_w, window_h,						// 缩放后的图像大小  宽度与高度
								osd_w,
								FORMAT_Y_UV420,
		        					(unsigned char *)new_framebuffer->address + window_y * osd_w + window_x,		// 输出图像
		        					(unsigned char *)new_framebuffer->address + osd_w * osd_h + window_y/2 * osd_w + window_x,
		        					(unsigned char *)new_framebuffer->address + osd_w * osd_h + window_y/2 * osd_w + window_x,
								0,
								0,
								0
								);
				break;

			case CH_H_AHD12://水平分割
				#ifdef EN_RN6752M1080N_CHIP
				openwindow_x = 10;
				openwindow_y = 300;
				openwindow_width = video_framepacket->width-10;
				openwindow_height = 500;
				#else
				openwindow_x = 10;
				openwindow_y = 200;
				openwindow_width = video_framepacket->width-10;
				openwindow_height = 320;
				#endif
				
				arkn141_scalar_process (
								(unsigned char *)video_framepacket->buffer,		// 输入图像
								0,
								1,			// 输出窗口使能
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// 输出窗口位置定义
								video_framepacket->width, video_framepacket->height,		// 输入图像大小  宽度与高度
								osd_w, osd_h/2,							// 缩放后的图像大小  宽度与高度
								osd_w,
								FORMAT_Y_UV420,
								(unsigned char *)new_framebuffer->address,		// 输出图像
								(unsigned char *)new_framebuffer->address + osd_w * osd_h,
								(unsigned char *)new_framebuffer->address + osd_w * osd_h,
								0,
								0,
								0
				);
				
				Itu656_framepacket = NULL;
				Itu656_framepacket = XMSYS_SensorGetCurrentPacket (1);

				#ifdef EN_RN6752M1080N_CHIP
				openwindow_x = 10;
				openwindow_y = 300;
				openwindow_width = Itu656_framepacket->width-10;
				openwindow_height = 500;
				#else
				openwindow_x = 10;
				openwindow_y = 200;
				openwindow_width = Itu656_framepacket->width-10;
				openwindow_height = 320;
				#endif
				
				if(Itu656_framepacket) {
					
				window_h = osd_h/2;
				window_w = osd_w;
		        window_x = 0;
		        window_y = osd_h - window_h; 
		        window_x &= ~0xF;
				arkn141_scalar_process (
							(unsigned char *)Itu656_framepacket->buffer,		// 输入图像
							0,
							1,			// 输出窗口使能
							openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// 输出窗口位置定义
							Itu656_framepacket->width, Itu656_framepacket->height,		// 输入图像大小  宽度与高度
							window_w, window_h,					// 缩放后的图像大小  宽度与高度
							osd_w,
							FORMAT_Y_UV420,
							(unsigned char *)new_framebuffer->address + window_y * osd_w + window_x,		// 输出图像
							(unsigned char *)new_framebuffer->address + osd_w * osd_h + window_y/2 * osd_w + window_x,
							(unsigned char *)new_framebuffer->address + osd_w * osd_h + window_y/2 * osd_w + window_x,
							0,
							0,
							0
							);
           		}		
				break;
				
			case CH_H_AHD21://水平分割
				Itu656_framepacket = NULL;
				Itu656_framepacket = XMSYS_SensorGetCurrentPacket (1);

				#ifdef EN_RN6752M1080N_CHIP
				openwindow_x = 10;
				openwindow_y = 300;
				openwindow_width = Itu656_framepacket->width-10;
				openwindow_height = 500;
				#else
				openwindow_x = 10;
				openwindow_y = 200;
				openwindow_width = Itu656_framepacket->width-10;
				openwindow_height = 320;
				#endif
				
	           if(Itu656_framepacket) {
	                arkn141_scalar_process (
	        					(unsigned char *)Itu656_framepacket->buffer,		// 输入图像
	        					0,
	        					1,			// 输出窗口使能
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// 输出窗口位置定义
	        					Itu656_framepacket->width, Itu656_framepacket->height,		// 输入图像大小  宽度与高度
	        					osd_w, osd_h/2,					// 缩放后的图像大小  宽度与高度
	        					osd_w,
	        					FORMAT_Y_UV420,
								(unsigned char *)new_framebuffer->address,		// 输出图像
								(unsigned char *)new_framebuffer->address + osd_w * osd_h,
								(unsigned char *)new_framebuffer->address + osd_w * osd_h,
	        					0,
	        					0,
	        					0
	        					);
           		}
				   
		        window_h = osd_h/2;
		        window_w = osd_w;
		        window_x = 0;
		        window_y = osd_h - window_h; 
		        window_x &= ~0xF;

				#ifdef EN_RN6752M1080N_CHIP
				openwindow_x = 10;
				openwindow_y = 300;
				openwindow_width = video_framepacket->width-10;
				openwindow_height = 500;
				#else
				openwindow_x = 10;
				openwindow_y = 200;
				openwindow_width = video_framepacket->width-10;
				openwindow_height = 320;
				#endif
				
				arkn141_scalar_process (
					(unsigned char *)video_framepacket->buffer,		// 输入图像
					0,
					1,			// 输出窗口使能
					openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// 输出窗口位置定义
					video_framepacket->width, video_framepacket->height,		// 输入图像大小  宽度与高度
					window_w, window_h,						// 缩放后的图像大小  宽度与高度
					osd_w,
					FORMAT_Y_UV420,
					(unsigned char *)new_framebuffer->address + window_y * osd_w + window_x,		// 输出图像
					(unsigned char *)new_framebuffer->address + osd_w * osd_h + window_y/2 * osd_w + window_x,
					(unsigned char *)new_framebuffer->address + osd_w * osd_h + window_y/2 * osd_w + window_x,
					0,
					0,
					0
				);				
				break;


				
			default:
				break;
		}
		#endif
		
		//if(GetReversingSta() == FALSE)
		if(sensor2reverse == 0 && sensor1reverse == 0)
		SetPerpipmode(pipmode);

		if(AP_GetMenuItem(APPMENUITEM_PARKMONITOR) == MOT_ON)
		{
			Itu656_motion_framepacket = NULL;
			Itu656_motion_framepacket = XMSYS_SensorGetCurrentPacket (1);
			arkn141_scalar_process(
									(unsigned char *)Itu656_motion_framepacket->buffer,		// 输入图像
									0,
									1,			// 输出窗口使能
									0, 0, Itu656_motion_framepacket->width, Itu656_motion_framepacket->height/2,		// 输出窗口位置定义
									Itu656_motion_framepacket->width, Itu656_motion_framepacket->height,		// 输入图像大小  宽度与高度
									SUB_WINDOW_W, SUB_WINDOW_H,
									SUB_WINDOW_W,
									FORMAT_Y_UV420,
									(unsigned char *)assemble_video_buffer,
									(unsigned char *)assemble_video_buffer +SUB_WINDOW_W*SUB_WINDOW_H,
									(unsigned char *)assemble_video_buffer +SUB_WINDOW_W*SUB_WINDOW_H,
									0,
									0,
									0
								  );
		}


        if(First_ShowData) {
    		XM_OSD_FRAMEBUFFER framebuffer;
    		memset (&framebuffer, 0, sizeof(framebuffer));
    		framebuffer.width = osd_w;
    		framebuffer.height = osd_h;
    		framebuffer.stride = osd_w;
    		framebuffer.format = XM_OSD_LAYER_FORMAT_Y_UV420;
    		framebuffer.lcd_channel = 0;
    		framebuffer.osd_layer = XM_OSD_LAYER_0;
    		framebuffer.address = (unsigned char *)new_framebuffer->address;
    		XM_IconDisplay (&framebuffer);
            dma_flush_range( (unsigned int)new_framebuffer->address, new_framebuffer->stride * new_framebuffer->height * 3/2 + (unsigned int)new_framebuffer->address);
        }
        First_ShowData = TRUE;
#if VIDEO_DEBUG
		t2 = XM_GetHighResolutionTickCount ();
		t_icon += (unsigned int)(t2 - t1);
		// 不需要dma_flush_range操作，XM_osd_framebuffer_close中执行dma_flush_range操作
		// dma_flush_range((u32_t)data[0], (u32_t)data[0]+osd_w*osd_h*3/2);
		t1 = t2;
#endif
		XM_osd_framebuffer_close (new_framebuffer, 0);
		HW_lcdc_waiting_for_vsyn_event (20);

#if VIDEO_DEBUG
		t2 = XM_GetHighResolutionTickCount ();
		t_close += (unsigned int)(t2 - t1);

		display_count ++;
		if(display_count == 200)
		{
			XM_printf ("create=%d, scalar=%d, rotate=%d, icon=%d, t_close=%d\n", t_create/20, t_scalar/20, t_rotate/20, t_icon/20, t_close/20);
			display_count = 0;
			t_create = 0;
			t_scalar = 0;
			t_rotate = 0;
			t_icon = 0;
			t_close = 0;
		}
#endif
	}
}

char *itu656_in_get_frame_buffer (unsigned int frame_id, unsigned int frame_size)
{
	if(frame_size > (ITU656_IN_MAX_WIDTH * ITU656_IN_MAX_HEIGHT * 3/2 + 2048))
		return NULL;
	return itu656_isp_scalar_static_fifo + frame_id * frame_size;
}


char *isp_scalar_get_frame_buffer (unsigned int frame_id, unsigned int frame_size)
{
	if(frame_size > (ISP_SCALAR_FRAME_MAX_WIDTH * ISP_SCALAR_FRAME_MAX_HEIGHT * 3/2 + 2048))
		return NULL;
	return itu656_isp_scalar_static_fifo + ITU656_IN_STATIC_FIFO_SIZE + ITU656_IN_MONITOR_SPACE_SIZE + frame_id * frame_size;
}

// 视频任务主函数

extern u8_t ACC_Select ;
u8_t ACC_Select_Pre = 0;
extern u8_t Delay_NVP6214_Data;
extern BOOL Close_Audio_Sound;
extern unsigned int buzz_time;
void XMSYS_VideoTask (void)
{
	int count;

	XM_OSD_FRAMEBUFFER *new_framebuffer;
	XMSYSSENSORPACKET *video_framepacket = NULL;
    unsigned int window_x, window_y, window_w, window_h;
	unsigned int osd_w, osd_h;
    XMSYSSENSORPACKET *Itu656_framepacket = NULL;


#ifdef LCD_VIDEO_FPS_MINITOR		// LCD视频帧率监控
	unsigned int scalar_count = 0;
	unsigned int scalar_timeout;	//
#endif
	while(1)
	{
		OS_U8 video_event = 0;

		video_event = OS_WaitEvent(XMSYS_VIDEO_EVENT_TICKET|XMSYS_VIDEO_EVENT_FRAME_RENDER);

		if(video_event & (XMSYS_VIDEO_EVENT_TICKET) )
		{

			// 录像过程中用于LCD显示的缩放帧
			OS_Use (&video_task_semaphore);

			#if 0//去掉这个控制,
			if(OSTimeGet() - buzz_time > 100)
			{
				XM_lock ();	
				SetGPIOPadData (GPIO30, euDataLow);
				XM_unlock();
			}
			#endif
			
			video_framepacket = NULL;
			if(assembly_mode == XMSYS_ASSEMBLY_MODE_FRONT_ONLY)
			{
				video_framepacket = XMSYS_SensorGetCurrentPacket(0);
			}
			else if(assembly_mode == XMSYS_ASSEMBLY_MODE_REAL_ONLY)
			{
				video_framepacket = XMSYS_SensorGetCurrentPacket (1);
			}
			if((video_framepacket) &&(Delay_NVP6214_Data == 0)/*&&(Delay_LCD_Rotate == 0)*/)//video_framepacket
			{
				// ODS层开启时,  执行视频显示
				//if(XM_GetFmlDeviceCap(DEVCAP_OSD)&&PowerOnShowLogo)//logo显示完再开视频
				//Close_Audio_Sound: 在PR2000 检测到数据之前关机,LCD_set_enable会导致背光开启
				if(XM_GetFmlDeviceCap(DEVCAP_OSD)&&PowerOnShowLogo&& Close_Audio_Sound)//logo显示完再开视频
				{

					if(skip_frames_to_on_lcd == 1)
					{
						LCD_set_enable (1);
					}
                    
					video_display (video_framepacket);
                    //开机照片ICON显示首次不需要进入
                    #if 0
                    if(First_ShowData == FALSE)
                        First_ShowData = TRUE;
                    #endif
					if(skip_frames_to_on_lcd > 0)
						skip_frames_to_on_lcd --;
				}
			}else {
			    if(Delay_NVP6214_Data) {
                   // if(ACC_Select_Pre != ACC_Select)
                        //HW_LCD_BackLightOff();
    			    Delay_NVP6214_Data ++;

                    if(Delay_NVP6214_Data > 15)
					{
                        skip_frames_to_on_lcd = 1;
                        Delay_NVP6214_Data = 0;
                        ACC_Select_Pre = AP_GetAHD_Select();
                   }
                }
                XMSYS_SensorResetCurrentPacket(0);// 把当前显示最后一针数据清空
                if((ACC_Select_Pre != ACC_Select)&& First_ShowData) {
                    ACC_Select_Pre = ACC_Select;
                    if(AP_GetBlue())
                        ShowLogo_Logo(ROM_T18_COMMON_DESKTOP_BLUE_JPG,ROM_T18_COMMON_DESKTOP_BLUE_JPG_SIZE,1);
                    else 
                        ShowLogo_Logo(ROM_T18_COMMON_DESKTOP_BLACK_JPG,ROM_T18_COMMON_DESKTOP_BLACK_JPG_SIZE,1);
                }
            }
			OS_Unuse (&video_task_semaphore);
		}

		if(video_event & XMSYS_VIDEO_EVENT_FRAME_RENDER)
		{
			unsigned char *buf;
			unsigned int w, h;

			OS_Use (&video_task_semaphore);
			buf = video_framebuffer;
			w = video_w;
			h = video_h;
			video_framebuffer = NULL;
			OS_Unuse (&video_task_semaphore);

			//XM_printf(">>>>>>>>>>XMSYS_VideoTask, video_event:%x\r\n", video_event);
			if(buf)
			{
				_XMSYS_VideoRenderVideoFrameIntoDisplay (buf, w, h);
			}
		}
	}
}


// 录像模式下视频系统任务初始化
int XMSYS_VideoInit (void)
{
	// 创建互斥访问信号量
	// XM_printf ("XMSYS_VideoInit\n");

	// 访问信号量
	OS_CREATERSEMA (&video_task_semaphore); /* Creates resource semaphore */

	OS_CREATERSEMA (&assembly_semaphore); /* Creates resource semaphore */

	queue_initialize (&ready_framebuffer);

	lcd_video_opened = 0;

	usb_video_opened = 0;
	OS_CREATETASK(&TCB_VideoTask, "VideoTask", XMSYS_VideoTask, XMSYS_VIDEO_TASK_PRIORITY, StackVideoTask);

	return 0;
}

// 视频系统任务结束
int XMSYS_VideoExit (void)
{
	return 0;
}


int XMSYS_VideoOpen (void)
{
	int ret = -1;
	unsigned int osd_w, osd_h;
	unsigned int rotate_matrix_width;
	xm_osd_framebuffer_t yuv_framebuffer;
	int i;
	//XM_printf ("Video Opened failed, itu656 in disabled\n");

	OS_Use (&video_task_semaphore);
	do
	{
		if(lcd_video_opened)
		{
			XM_printf ("Video Opened before\n");
			ret = 0;
			break;
		}
		// 检查USB视频是否已开启.
		// 若是, 开启LCD视频失败
		#if 0
		if(XMSYS_CameraIsReady())
		{
			XM_printf ("XMSYS_VideoOpen failed, PC Camera is Ready\n");
			// 切换到实时流
			XMSYS_JpegSetupMode (XMSYS_JPEG_MODE_ISP, NULL);
			ret = -1;
			break;
		}
		#endif
		XM_IconInit ();

		// 创建100hz的定时器
		// OS_CREATETIMER (&VideoTimer, VideoTicketCallback, 10);
		// 创建25hz的定时器
		OS_CREATETIMER (&VideoTimer, VideoTicketCallback, 40);//25针数据
		lcd_video_opened = 1;

		ret = 0;
	} while(0);
	OS_Unuse (&video_task_semaphore);
	//XM_printf ("XMSYS_VideoOpen %s\n", ret == 0 ? "OK" : "NG");

	return ret;
}

int XMSYS_VideoClose (void)
{
	unsigned int osd_w, osd_h;
	xm_osd_framebuffer_t yuv_framebuffer;
	XMSYSSENSORPACKET *scalar_framepacket;

	XM_printf ("XMSYS_VideoClose\n");

	OS_Use (&video_task_semaphore);
	do
	{
		if(lcd_video_opened == 0)
			break;

		lcd_video_opened = 0;
		OS_DeleteTimer (&VideoTimer);
		XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
		XM_IconExit ();

	} while(0);

	OS_Unuse (&video_task_semaphore);
	return 0;
}

int XMSYS_UsbVideoOpen (void)
{
	int ret = -1;
	XM_printf ("XMSYS_UsbVideoOpen\n");
	OS_Use (&video_task_semaphore);
	do
	{
		// 检查LCD视频是否已开启. 若开启,将其关闭
		if(lcd_video_opened)
		{
			XMSYS_VideoClose ();
		}
		else if(usb_video_opened)
		{
			// USB视频已开启.
			// 重新打开USB视频.
		}
		// 检查是否是录像模式.
		if(XMSYS_H264CodecGetMode() == XMSYS_H264_CODEC_MODE_RECORD)
		{
			// 缺省使用live(isp scalar)模式, 开启USB视频
			if(XMSYS_JpegSetupMode (XMSYS_JPEG_MODE_ISP, NULL) < 0)		// 使用ISP的原始数据
			//if(XMSYS_JpegSetupMode (XMSYS_JPEG_MODE_ISP_SCALAR, NULL) < 0)	// 使用ISP_Scalar输出的数据
			{
				XM_printf ("XMSYS_UsbVideoOpen live mode failed, XMSYS_JpegSetupMode NG\n");
				ret = -1;
			}
			else
			{
				usb_video_opened = 1;
				ret = 0;
			}
		}
		else
		{
			// 使用空数据模式, 允许命令发送/应答/文件下载
			if(XMSYS_JpegSetupMode (XMSYS_JPEG_MODE_DUMMY, NULL) < 0)
			{
				XM_printf ("XMSYS_UsbVideoOpen null mode failed, XMSYS_JpegSetupMode NG\n");
				ret = -1;
			}
			else
			{
				usb_video_opened = 1;
				ret = 0;
			}

		}

	} while(0);

	OS_Unuse (&video_task_semaphore);
	XM_printf ("XMSYS_UsbVideoOpen %s\n", ret == 0 ? "OK" : "NG");
	return ret;
}

int XMSYS_UsbVideoClose (void)
{
	int ret = -1;
	OS_Use (&video_task_semaphore);
	do
	{
		if(usb_video_opened == 0)
		{
			ret = 0;
		}
		else
		{
			// 关闭UVC通信, 关闭USB视频
			XMSYS_JpegSetupMode (-1, NULL);
			usb_video_opened = 0;
		}

		// 开启LCD视频
		// 检查是否是录像模式. 若是, 自动开启LCD视频
		if(XMSYS_H264CodecGetMode() == XMSYS_H264_CODEC_MODE_RECORD)
		{
			if(lcd_video_opened == 0)
			{
				XMSYS_VideoOpen ();
			}
		}
		ret = 0;
	} while (0);

	OS_Unuse (&video_task_semaphore);

	return ret;
}


int	XMSYS_VideoSetCameraMode (int camera_mode)
{
	return 0;
}


static int	_XMSYS_VideoRenderVideoFrameIntoDisplay (	unsigned char *frame_data,
																unsigned int frame_w,
																unsigned int frame_h
															)
{
	int ret = -1;
	XM_OSD_FRAMEBUFFER *yuv_framebuffer;
	unsigned int _w, _h;
	char *temp;


	_w = XM_lcdc_osd_get_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	_h = XM_lcdc_osd_get_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	yuv_framebuffer = XM_osd_framebuffer_create (0,
							XM_OSD_LAYER_0,
							XM_OSD_LAYER_FORMAT_Y_UV420,
							_w,
							_h,
							NULL,
							0,
							0		// clear_framebuffer_context
							);
	if(yuv_framebuffer)
	{
		uint8_t *dst_data[4];
		if(XM_osd_framebuffer_get_buffer (yuv_framebuffer, dst_data) == 0)
		{
		
#ifdef LCD_ROTATE_90
			temp = kernel_malloc (_w * _h * 3/2);
			if(temp)
			{
				dma_inv_range ((unsigned int)temp, _w * _h * 3/2 + (unsigned int)temp);
				if(frame_w == 1920 && frame_h == 1088)
				{
					#if 0
					// 1080P解码器按1088高度(16的倍数)解码,需要去除8行
					arkn141_scalar_process (frame_data,
															1, 0,
															0, 0, frame_w, 1080,	// 1080P窗口
															frame_w, frame_h,
															_w, _h, _w,
															FORMAT_Y_UV420,
															(unsigned char *)temp, (unsigned char *)temp + _w * _h, NULL,
															0, 0, 0);
					#else

					arkn141_scalar_process (
										frame_data,	// 输入图像
										0,
										1,			// 输出窗口使能
										0, (1080-480)/2, 1920, 480,	// 输出窗口位置定义
										//0, 0, frame_w, 1080,	// 输出窗口位置定义
										frame_w, frame_h,		// 输入图像大小  宽度与高度
										_w, _h,					// 缩放后的图像大小  宽度与高度
										_w,
										FORMAT_Y_UV420,
										(unsigned char *)temp,		// 输出图像
										(unsigned char *)temp + _w * _h,
										(unsigned char *)temp + _w * _h,
										0,
										0,
										0
										);

		#if 1 // 嵌入本地时间戳信息到回放视频中
		XM_printf(">>>>>>>>>>>>>>>>>>>>>AP_GetMenuItem(APPMENUITEM_TIME_STAMP):%d\r\n", AP_GetMenuItem(APPMENUITEM_TIME_STAMP));
		if(AP_GetMenuItem(APPMENUITEM_TIME_STAMP) == AP_SETTING_VIDEO_TIMESTAMP_ON)
		{
			void embed_local_time_into_rendervideo_frame (
														 unsigned int width,	// 视频宽度
														 unsigned int height,	// 视频高度
														 unsigned int stride,	// Y_UV行字节长度
														 char *data				// Y_UV缓冲区
														);
			unsigned int vaddr = page_addr_to_vaddr((unsigned int)temp);
			unsigned char *virtual_address = (unsigned char *)vaddr;
			embed_local_time_into_rendervideo_frame (_w, _h, _w, (char *)virtual_address);
		}
		#endif

		xm_rotate_90_by_zoom_opt((u8_t *)temp,
						  (u8_t *)temp + _w*_h,
						  _w,	_h,	_w,
						  0,
						  0,
						  _w,
						  _h,
						  dst_data[0],
						  dst_data[1],
						  _w, _h,
						  0,1);
				   #endif
				}
				else
				{
					#if 0
					arkn141_scalar_process (frame_data,
															0, 0,
															0, 0, frame_w, frame_h,
															frame_w, frame_h,
															_w, _h, _w,
															FORMAT_Y_UV420,
															(unsigned char *)temp, (unsigned char *)temp + _w * _h, NULL,
															0, 0, 0);
					#else

					arkn141_scalar_process (
										frame_data,
										0,
										1,			// 输出窗口使能
										0, 200, 1280, 320,		// 输出窗口位置定义
										//0, 0, frame_w, frame_h,	// 输出窗口位置定义
										frame_w, frame_h,		// 输入图像大小  宽度与高度
										_w, _h,					// 缩放后的图像大小  宽度与高度
										_w,
										FORMAT_Y_UV420,
										(unsigned char *)temp,
										(unsigned char *)temp + _w * _h,
										(unsigned char *)temp + _w * _h,
										0,
										0,
										0
										);
		#if 1  		// 嵌入本地时间戳信息到回放视频中
		XM_printf(">>>>>>>>>>>AP_GetMenuItem(APPMENUITEM_TIME_STAMP):%d\r\n", AP_GetMenuItem(APPMENUITEM_TIME_STAMP));
		//if(AP_GetMenuItem(APPMENUITEM_TIME_STAMP) == AP_SETTING_VIDEO_TIMESTAMP_ON)
		{
			void embed_local_time_into_rendervideo_frame (
														 unsigned int width,			// 视频宽度
														 unsigned int height,		// 视频高度
														 unsigned int stride,	// Y_UV行字节长度
														 char *data					// Y_UV缓冲区
														);
			unsigned int vaddr = page_addr_to_vaddr((unsigned int)temp);
			unsigned char *virtual_address = (unsigned char *)vaddr;
			embed_local_time_into_rendervideo_frame (_w, _h, _w, (char *)virtual_address);
		}
		#endif

					 xm_rotate_90_by_zoom_opt((u8_t *)temp,
						  (u8_t *)temp + _w*_h,
						  _w,	_h,	_w,
						  0,
						  0,
						  _w,
						  _h,
						  dst_data[0],
						  dst_data[1],
						  _w, _h,
						  0,1);
				#endif
				}
				/*
				rotate_degree90 ((unsigned char *)temp,
											  dst_data[0],
											  _w, _h,
											  _h, _w
								  );
				*/

				kernel_free (temp);
			}
#else
			/* convert to destination format */
			arkn141_scalar_process (frame_data,
															0, 0,
															0, 0, frame_w, frame_h,
															frame_w, frame_h,
															_w, _h, _w,
															FORMAT_Y_UV420,
															dst_data[0], dst_data[1], NULL,
															0, 0, 0);

			//XM_printf(">>>>>>>>>>>AP_GetMenuItem(APPMENUITEM_TIME_STAMP):%d\r\n", AP_GetMenuItem(APPMENUITEM_TIME_STAMP));
			#if 0
			// 嵌入本地时间戳信息到回放视频中
            void embed_local_time_into_rendervideo_frame (
															 unsigned int width,	// 视频宽度
															 unsigned int height,	// 视频高度
															 unsigned int stride,	// Y_UV行字节长度
															 char *data				// Y_UV缓冲区
															);
				unsigned int vaddr = page_addr_to_vaddr((unsigned int)dst_data[0]);
				unsigned char *virtual_address = (unsigned char *)vaddr;
				embed_local_time_into_rendervideo_frame (_w, _h, _w, (char *)virtual_address);
			#endif
#endif
			ret = 0;
		}
		XM_osd_framebuffer_close (yuv_framebuffer, 0);
		HW_lcdc_waiting_for_vsyn_event (20);
	}
	return ret;
}



int	XMSYS_VideoRenderVideoFrameIntoDisplay (struct H264DecodeStream *stream,
															   unsigned char *frame_data,
																unsigned int frame_w,
																unsigned int frame_h
															)
{
	OS_Use (&video_task_semaphore);
	video_framebuffer = frame_data;
	video_w = frame_w;
	video_h = frame_h;
	video_playback_ticket = stream->frame_index / stream->fps;
	OS_Unuse (&video_task_semaphore);

	//XM_printf(">>>>>>>>XMSYS_VideoRenderVideoFrameIntoDisplay, stream->frame_index:%d\r\n", stream->frame_index);
	//XM_printf(">>>>>>>>XMSYS_VideoRenderVideoFrameIntoDisplay, stream->frame_count:%d\r\n", stream->frame_count);
	//XM_printf(">>>>>>>>XMSYS_VideoRenderVideoFrameIntoDisplay, stream->fps:%d\r\n", stream->fps);
	//XM_printf(">>>>>>>>XMSYS_VideoRenderVideoFrameIntoDisplay, video_playback_ticket:%d\r\n", video_playback_ticket);

	if(stream->user_data[0][0])
	{
		// 解析私有用户数据
		XMSYS_DecodeUserData ((char *)stream->user_data[0], stream->user_data[0][0]);
	}
	
	OS_SignalEvent(XMSYS_VIDEO_EVENT_FRAME_RENDER, &TCB_VideoTask); /* 通知事件 */
	return 0;
}

// 等待回放帧处理完毕
// 回放会占用H264解码的资源(解码帧缓冲区)来完成帧缩放操作.
// 当回放结束时, 回放线程没有等待视频线程处理完毕就释放资源(解码帧缓冲区)并用作其他用途,
// 导致视频线程处理(解码帧缓冲区)时内容已被污染.
void XMSYS_VideoWaitingForRenderDone (unsigned int ms_to_timeout)
{
	// 等待回放线程使用完毕资源(解码帧缓冲区)并释放
	unsigned int ticket_to_timeout = XM_GetTickCount () + ms_to_timeout;
	do
	{
		if(video_framebuffer == NULL)
			break;

	} while (XM_GetTickCount () < ticket_to_timeout);

	// 直接置空, 避免信号量阻塞
	// OS_Use (&video_task_semaphore);
	// if(video_framebuffer)
		video_framebuffer = NULL;
	// OS_Unuse (&video_task_semaphore);
}

