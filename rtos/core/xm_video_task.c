//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_video_task.c
//	  ��Ƶ����
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
#include "rtos.h"		// ������غ���
#include "xm_proj_define.h"
#include "rom.h"
#include "gpio.h"

extern int PowerOnShowLogo;
void rotate_degree90 (unsigned char *y_uv420_src,
						  unsigned char *y_uv420_dst,
						  int width, // ����ͼ���С  �����߶�
						  int height,
						  int o_width, // ����ͼ���С  �����߶�
						  int o_height);


#define	VIDEO_ISP_SCALAR_FRAME_COUNT		4

static int	_XMSYS_VideoRenderVideoFrameIntoDisplay (	unsigned char *frame_data,
																unsigned int frame_w,
																unsigned int frame_h
															);



static OS_TASK TCB_VideoTask;
__no_init static OS_STACKPTR int StackVideoTask[XMSYS_VIDEO_TASK_STACK_SIZE/4];          /* Task stacks */

static queue_s ready_framebuffer;		// ������׼���õ�framebuffer

static unsigned char *video_framebuffer;
static unsigned int video_w, video_h;
static unsigned int video_playback_ticket;
u32_t SensorData_Source;

#pragma data_alignment=32
#define	ITU656_IN_STATIC_FIFO_SIZE		((ITU656_IN_FRAME_COUNT+1) * (ITU656_IN_MAX_WIDTH * ITU656_IN_MAX_HEIGHT * 3/2 + 2048))
#define	ITU656_IN_MONITOR_SPACE_SIZE		512*1024	
// �м�Ԥ��1��512*1024�ֽڵĻ��壬���ڼ��itu601in�Ƿ���ڽ�������������ڽӵ�isp scalar����������һ����ʱ����(3.95ms 720p 60fps itu601 in)���ᱻ����
__no_init char itu656_isp_scalar_static_fifo[ITU656_IN_STATIC_FIFO_SIZE + ITU656_IN_MONITOR_SPACE_SIZE + ISP_SCALAR_STATIC_FIFO_SIZE];

static OS_RSEMA video_task_semaphore;		// �����ź�������


#define	XMSYS_VIDEO_EVENT_SCALAR_FRAME_READY	0x01		// scalar֡������׼�����
#define	XMSYS_VIDEO_EVENT_TICKET					0x02

#define	XMSYS_VIDEO_EVENT_FRAME_RENDER			0x10		// ��Ƶ֡��Ⱦ

static OS_TIMER VideoTimer;
static int lcd_video_opened;

static int usb_video_opened;
static int skip_frames_to_on_lcd = 1;		// ��һ�ο���ʱʹ��


#ifdef ISP_SCALAR_USE_STATIC_FIFO
#pragma data_alignment=4096
__no_init char isp_scalar_static_fifo[ISP_SCALAR_STATIC_FIFO_SIZE];
#else

#endif
#pragma data_alignment=4096
__no_init static unsigned char assemble_video_buffer[320*240*3/2];		// ��ʾ��ʱ������

#define	SUB_WINDOW_W		320
#define	SUB_WINDOW_H		240

static OS_RSEMA assembly_semaphore;		// ͼ��ƴ�ӷ����ź�������

#if TULV
																					//	�˻������������VIDEO��ʾ��������
#if 0
#pragma data_alignment=4096
__no_init static unsigned char rotate_video_buffer[320*240*3/2];		// ��ת��ʱ������
//__no_init static unsigned char assemble_video_buffer[1600*400*3/2];		// ƴ����ʱ������
#endif

#define	DIRECT_USE_REAL_IMAGE

static int assembly_mode = XMSYS_ASSEMBLY_MODE_FRONT_ONLY;	// ͼ��ƴ��ģʽ
//static int assembly_mode = XMSYS_ASSEMBLY_MODE_REAL_ONLY;
int ITU656_in=0;
int Reversing=0;


//�ط���Ƶ�����˶೤ʱ��
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
// ���û���ƴ��ģʽ
int XMSYS_VideoSetImageAssemblyMode (unsigned int image_assembly_mode)
{
	if(image_assembly_mode >= XMSYS_ASSEMBLY_MODE_COUNT)
		return -1;

	OS_Use (&assembly_semaphore);

	assembly_mode = image_assembly_mode;

	// �����ʷ��¼
	OS_Unuse (&assembly_semaphore);
	return 0;
}

// ���ص�ǰ��Ƶͼ��ƴ��ģʽ
unsigned int XMSYS_VideoGetImageAssemblyMode (void)
{
	return assembly_mode;
}

#endif


char *get_ch2_video_image()
{
	return assemble_video_buffer;
}


// �жϷ����е���

//#define	VIDEO_DEBUG	1

static void VideoTicketCallback (void)
{
	//XM_printf ("JpegTicketCallback\n");
	OS_SignalEvent(XMSYS_VIDEO_EVENT_TICKET, &TCB_VideoTask); /* ֪ͨ�¼� */
	OS_RetriggerTimer (&VideoTimer);
}

// ʵʱ��Ƶ��ʾ
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
								(unsigned char *)video_framepacket->buffer,		// ����ͼ��
								0,
								1,			// �������ʹ��
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// �������λ�ö���
								video_framepacket->width, video_framepacket->height,		// ����ͼ���С  �����߶�
								osd_w, osd_h,						// ���ź��ͼ���С  �����߶�
								osd_w,
								FORMAT_Y_UV420,
								(unsigned char *)new_framebuffer->address,		// ���ͼ��
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
								(unsigned char *)Itu656_framepacket->buffer,		// ����ͼ��
								0,
								1,			// �������ʹ��
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// �������λ�ö���
								Itu656_framepacket->width, Itu656_framepacket->height,		// ����ͼ���С  �����߶�
								osd_w, osd_h,						// ���ź��ͼ���С  �����߶�
								osd_w,
								FORMAT_Y_UV420,
								(unsigned char *)new_framebuffer->address,		// ���ͼ��
								(unsigned char *)new_framebuffer->address + osd_w * osd_h,
								(unsigned char *)new_framebuffer->address + osd_w * osd_h,
								0,
								0,
								0
								);
				break;

			case CH_V_AHD12://��ֱ�ָ�
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
								(unsigned char *)video_framepacket->buffer,		// ����ͼ��
								0,
								1,			// �������ʹ��
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// �������λ�ö���
								video_framepacket->width, video_framepacket->height,		// ����ͼ���С  �����߶�
								osd_w/2, osd_h,						// ���ź��ͼ���С  �����߶�
								osd_w,
								FORMAT_Y_UV420,
								(unsigned char *)new_framebuffer->address,		// ���ͼ��
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
								(unsigned char *)Itu656_framepacket->buffer,		// ����ͼ��
								0,
								1,			// �������ʹ��
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// �������λ�ö���
								Itu656_framepacket->width, Itu656_framepacket->height,		// ����ͼ���С  �����߶�
								window_w, window_h,					// ���ź��ͼ���С  �����߶�
								osd_w,
								FORMAT_Y_UV420,
								(unsigned char *)new_framebuffer->address + window_y * osd_w + window_x,		// ���ͼ��
								(unsigned char *)new_framebuffer->address + osd_w * osd_h + window_y/2 * osd_w + window_x,
								(unsigned char *)new_framebuffer->address + osd_w * osd_h + window_y/2 * osd_w + window_x,
								0,
								0,
								0
					);
				}
				break;

			case CH_V_AHD21://��ֱ�ָ�
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
								(unsigned char *)Itu656_framepacket->buffer,		// ����ͼ��
								0,
								1,			// �������ʹ��
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// �������λ�ö���
								Itu656_framepacket->width, Itu656_framepacket->height,		// ����ͼ���С  �����߶�
								osd_w/2, osd_h,					// ���ź��ͼ���С  �����߶�
								osd_w,
								FORMAT_Y_UV420,
								(unsigned char *)new_framebuffer->address,		// ���ͼ��
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
								(unsigned char *)video_framepacket->buffer,		// ����ͼ��
								0,
								1,			// �������ʹ��
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// �������λ�ö���
								video_framepacket->width, video_framepacket->height,		// ����ͼ���С  �����߶�
								window_w, window_h,						// ���ź��ͼ���С  �����߶�
								osd_w,
								FORMAT_Y_UV420,
		        					(unsigned char *)new_framebuffer->address + window_y * osd_w + window_x,		// ���ͼ��
		        					(unsigned char *)new_framebuffer->address + osd_w * osd_h + window_y/2 * osd_w + window_x,
		        					(unsigned char *)new_framebuffer->address + osd_w * osd_h + window_y/2 * osd_w + window_x,
								0,
								0,
								0
								);
				break;

			case CH_H_AHD12://ˮƽ�ָ�
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
								(unsigned char *)video_framepacket->buffer,		// ����ͼ��
								0,
								1,			// �������ʹ��
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// �������λ�ö���
								video_framepacket->width, video_framepacket->height,		// ����ͼ���С  �����߶�
								osd_w, osd_h/2,							// ���ź��ͼ���С  �����߶�
								osd_w,
								FORMAT_Y_UV420,
								(unsigned char *)new_framebuffer->address,		// ���ͼ��
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
							(unsigned char *)Itu656_framepacket->buffer,		// ����ͼ��
							0,
							1,			// �������ʹ��
							openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// �������λ�ö���
							Itu656_framepacket->width, Itu656_framepacket->height,		// ����ͼ���С  �����߶�
							window_w, window_h,					// ���ź��ͼ���С  �����߶�
							osd_w,
							FORMAT_Y_UV420,
							(unsigned char *)new_framebuffer->address + window_y * osd_w + window_x,		// ���ͼ��
							(unsigned char *)new_framebuffer->address + osd_w * osd_h + window_y/2 * osd_w + window_x,
							(unsigned char *)new_framebuffer->address + osd_w * osd_h + window_y/2 * osd_w + window_x,
							0,
							0,
							0
							);
           		}		
				break;
				
			case CH_H_AHD21://ˮƽ�ָ�
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
	        					(unsigned char *)Itu656_framepacket->buffer,		// ����ͼ��
	        					0,
	        					1,			// �������ʹ��
								openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// �������λ�ö���
	        					Itu656_framepacket->width, Itu656_framepacket->height,		// ����ͼ���С  �����߶�
	        					osd_w, osd_h/2,					// ���ź��ͼ���С  �����߶�
	        					osd_w,
	        					FORMAT_Y_UV420,
								(unsigned char *)new_framebuffer->address,		// ���ͼ��
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
					(unsigned char *)video_framepacket->buffer,		// ����ͼ��
					0,
					1,			// �������ʹ��
					openwindow_x, openwindow_y, openwindow_width, openwindow_height,		// �������λ�ö���
					video_framepacket->width, video_framepacket->height,		// ����ͼ���С  �����߶�
					window_w, window_h,						// ���ź��ͼ���С  �����߶�
					osd_w,
					FORMAT_Y_UV420,
					(unsigned char *)new_framebuffer->address + window_y * osd_w + window_x,		// ���ͼ��
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
									(unsigned char *)Itu656_motion_framepacket->buffer,		// ����ͼ��
									0,
									1,			// �������ʹ��
									0, 0, Itu656_motion_framepacket->width, Itu656_motion_framepacket->height/2,		// �������λ�ö���
									Itu656_motion_framepacket->width, Itu656_motion_framepacket->height,		// ����ͼ���С  �����߶�
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
		// ����Ҫdma_flush_range������XM_osd_framebuffer_close��ִ��dma_flush_range����
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

// ��Ƶ����������

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


#ifdef LCD_VIDEO_FPS_MINITOR		// LCD��Ƶ֡�ʼ��
	unsigned int scalar_count = 0;
	unsigned int scalar_timeout;	//
#endif
	while(1)
	{
		OS_U8 video_event = 0;

		video_event = OS_WaitEvent(XMSYS_VIDEO_EVENT_TICKET|XMSYS_VIDEO_EVENT_FRAME_RENDER);

		if(video_event & (XMSYS_VIDEO_EVENT_TICKET) )
		{

			// ¼�����������LCD��ʾ������֡
			OS_Use (&video_task_semaphore);

			#if 0//ȥ���������,
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
				// ODS�㿪��ʱ,  ִ����Ƶ��ʾ
				//if(XM_GetFmlDeviceCap(DEVCAP_OSD)&&PowerOnShowLogo)//logo��ʾ���ٿ���Ƶ
				//Close_Audio_Sound: ��PR2000 ��⵽����֮ǰ�ػ�,LCD_set_enable�ᵼ�±��⿪��
				if(XM_GetFmlDeviceCap(DEVCAP_OSD)&&PowerOnShowLogo&& Close_Audio_Sound)//logo��ʾ���ٿ���Ƶ
				{

					if(skip_frames_to_on_lcd == 1)
					{
						LCD_set_enable (1);
					}
                    
					video_display (video_framepacket);
                    //������ƬICON��ʾ�״β���Ҫ����
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
                XMSYS_SensorResetCurrentPacket(0);// �ѵ�ǰ��ʾ���һ���������
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


// ¼��ģʽ����Ƶϵͳ�����ʼ��
int XMSYS_VideoInit (void)
{
	// ������������ź���
	// XM_printf ("XMSYS_VideoInit\n");

	// �����ź���
	OS_CREATERSEMA (&video_task_semaphore); /* Creates resource semaphore */

	OS_CREATERSEMA (&assembly_semaphore); /* Creates resource semaphore */

	queue_initialize (&ready_framebuffer);

	lcd_video_opened = 0;

	usb_video_opened = 0;
	OS_CREATETASK(&TCB_VideoTask, "VideoTask", XMSYS_VideoTask, XMSYS_VIDEO_TASK_PRIORITY, StackVideoTask);

	return 0;
}

// ��Ƶϵͳ�������
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
		// ���USB��Ƶ�Ƿ��ѿ���.
		// ����, ����LCD��Ƶʧ��
		#if 0
		if(XMSYS_CameraIsReady())
		{
			XM_printf ("XMSYS_VideoOpen failed, PC Camera is Ready\n");
			// �л���ʵʱ��
			XMSYS_JpegSetupMode (XMSYS_JPEG_MODE_ISP, NULL);
			ret = -1;
			break;
		}
		#endif
		XM_IconInit ();

		// ����100hz�Ķ�ʱ��
		// OS_CREATETIMER (&VideoTimer, VideoTicketCallback, 10);
		// ����25hz�Ķ�ʱ��
		OS_CREATETIMER (&VideoTimer, VideoTicketCallback, 40);//25������
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
		// ���LCD��Ƶ�Ƿ��ѿ���. ������,����ر�
		if(lcd_video_opened)
		{
			XMSYS_VideoClose ();
		}
		else if(usb_video_opened)
		{
			// USB��Ƶ�ѿ���.
			// ���´�USB��Ƶ.
		}
		// ����Ƿ���¼��ģʽ.
		if(XMSYS_H264CodecGetMode() == XMSYS_H264_CODEC_MODE_RECORD)
		{
			// ȱʡʹ��live(isp scalar)ģʽ, ����USB��Ƶ
			if(XMSYS_JpegSetupMode (XMSYS_JPEG_MODE_ISP, NULL) < 0)		// ʹ��ISP��ԭʼ����
			//if(XMSYS_JpegSetupMode (XMSYS_JPEG_MODE_ISP_SCALAR, NULL) < 0)	// ʹ��ISP_Scalar���������
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
			// ʹ�ÿ�����ģʽ, ���������/Ӧ��/�ļ�����
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
			// �ر�UVCͨ��, �ر�USB��Ƶ
			XMSYS_JpegSetupMode (-1, NULL);
			usb_video_opened = 0;
		}

		// ����LCD��Ƶ
		// ����Ƿ���¼��ģʽ. ����, �Զ�����LCD��Ƶ
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
					// 1080P��������1088�߶�(16�ı���)����,��Ҫȥ��8��
					arkn141_scalar_process (frame_data,
															1, 0,
															0, 0, frame_w, 1080,	// 1080P����
															frame_w, frame_h,
															_w, _h, _w,
															FORMAT_Y_UV420,
															(unsigned char *)temp, (unsigned char *)temp + _w * _h, NULL,
															0, 0, 0);
					#else

					arkn141_scalar_process (
										frame_data,	// ����ͼ��
										0,
										1,			// �������ʹ��
										0, (1080-480)/2, 1920, 480,	// �������λ�ö���
										//0, 0, frame_w, 1080,	// �������λ�ö���
										frame_w, frame_h,		// ����ͼ���С  �����߶�
										_w, _h,					// ���ź��ͼ���С  �����߶�
										_w,
										FORMAT_Y_UV420,
										(unsigned char *)temp,		// ���ͼ��
										(unsigned char *)temp + _w * _h,
										(unsigned char *)temp + _w * _h,
										0,
										0,
										0
										);

		#if 1 // Ƕ�뱾��ʱ�����Ϣ���ط���Ƶ��
		XM_printf(">>>>>>>>>>>>>>>>>>>>>AP_GetMenuItem(APPMENUITEM_TIME_STAMP):%d\r\n", AP_GetMenuItem(APPMENUITEM_TIME_STAMP));
		if(AP_GetMenuItem(APPMENUITEM_TIME_STAMP) == AP_SETTING_VIDEO_TIMESTAMP_ON)
		{
			void embed_local_time_into_rendervideo_frame (
														 unsigned int width,	// ��Ƶ���
														 unsigned int height,	// ��Ƶ�߶�
														 unsigned int stride,	// Y_UV���ֽڳ���
														 char *data				// Y_UV������
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
										1,			// �������ʹ��
										0, 200, 1280, 320,		// �������λ�ö���
										//0, 0, frame_w, frame_h,	// �������λ�ö���
										frame_w, frame_h,		// ����ͼ���С  �����߶�
										_w, _h,					// ���ź��ͼ���С  �����߶�
										_w,
										FORMAT_Y_UV420,
										(unsigned char *)temp,
										(unsigned char *)temp + _w * _h,
										(unsigned char *)temp + _w * _h,
										0,
										0,
										0
										);
		#if 1  		// Ƕ�뱾��ʱ�����Ϣ���ط���Ƶ��
		XM_printf(">>>>>>>>>>>AP_GetMenuItem(APPMENUITEM_TIME_STAMP):%d\r\n", AP_GetMenuItem(APPMENUITEM_TIME_STAMP));
		//if(AP_GetMenuItem(APPMENUITEM_TIME_STAMP) == AP_SETTING_VIDEO_TIMESTAMP_ON)
		{
			void embed_local_time_into_rendervideo_frame (
														 unsigned int width,			// ��Ƶ���
														 unsigned int height,		// ��Ƶ�߶�
														 unsigned int stride,	// Y_UV���ֽڳ���
														 char *data					// Y_UV������
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
			// Ƕ�뱾��ʱ�����Ϣ���ط���Ƶ��
            void embed_local_time_into_rendervideo_frame (
															 unsigned int width,	// ��Ƶ���
															 unsigned int height,	// ��Ƶ�߶�
															 unsigned int stride,	// Y_UV���ֽڳ���
															 char *data				// Y_UV������
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
		// ����˽���û�����
		XMSYS_DecodeUserData ((char *)stream->user_data[0], stream->user_data[0][0]);
	}
	
	OS_SignalEvent(XMSYS_VIDEO_EVENT_FRAME_RENDER, &TCB_VideoTask); /* ֪ͨ�¼� */
	return 0;
}

// �ȴ��ط�֡�������
// �طŻ�ռ��H264�������Դ(����֡������)�����֡���Ų���.
// ���طŽ���ʱ, �ط��߳�û�еȴ���Ƶ�̴߳�����Ͼ��ͷ���Դ(����֡������)������������;,
// ������Ƶ�̴߳���(����֡������)ʱ�����ѱ���Ⱦ.
void XMSYS_VideoWaitingForRenderDone (unsigned int ms_to_timeout)
{
	// �ȴ��ط��߳�ʹ�������Դ(����֡������)���ͷ�
	unsigned int ticket_to_timeout = XM_GetTickCount () + ms_to_timeout;
	do
	{
		if(video_framebuffer == NULL)
			break;

	} while (XM_GetTickCount () < ticket_to_timeout);

	// ֱ���ÿ�, �����ź�������
	// OS_Use (&video_task_semaphore);
	// if(video_framebuffer)
		video_framebuffer = NULL;
	// OS_Unuse (&video_task_semaphore);
}

