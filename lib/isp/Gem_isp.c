// =============================================================================
// File        : Gem_isp.c
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#include "hardware.h"
#include <xm_printf.h>
#include "Gem_isp.h"
#include "Gem_isp_colors.h"
#include "Gem_isp_denoise.h"
#include "Gem_isp_eris.h"
#include "Gem_isp_fesp.h"
#include "Gem_isp_enhance.h"
#include "Gem_isp_awb.h"
#include "Gem_isp_ae.h"
#include "Gem_isp_sys.h"
#include "Gem_isp_config.h"
#include "Gem_isp_io.h"

#include "RTOS.h"
#include "xm_h264_cache_file.h"
#include "fs.h"
#include "xm_dev.h"
#include "xm_user.h"
#include "xm_key.h"
#include "irqs.h"
#include "arkn141_isp_sensor_cfg.h"
#include "xm_core.h"


#include "arkn141_isp_exposure_cmos.h"
#include "arkn141_isp_exposure.h"
#include "xm_app_menudata.h"

extern int xm_arkn141_isp_set_flicker_freq  (int flicker_freq);


static const char *isp_get_work_mode_name (unsigned int mode);

#define ISP_DATA_ALIGNMENT_NUM        2048

// ISP RAW图像的bayer格式
static unsigned int isp_raw_image_bayer_mode = ARKN141_ISP_RAW_IMAGE_BAYER_MODE_RGGB;


//static u32_t isp_video_width  = 1024;
//static u32_t isp_video_height = 2048;

//static u32_t isp_video_width  = 2048;
//static u32_t isp_video_height = 1024;

//static u32_t isp_video_width  = 1920;
//static u32_t isp_video_height = 1080;

//static u32_t isp_video_width  = 1280;
//static u32_t isp_video_height = 720;

//static u32_t isp_video_width  = 176;
//static u32_t isp_video_height = 144;


//static u32_t isp_sensor_bit = ARKN141_ISP_SENSOR_BIT_12;
//static u32_t isp_sensor_bit = ARKN141_ISP_SENSOR_BIT_10;

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX322 || ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX323
// 1920*1080 12bit	IMX322/AR0330
static u32_t isp_video_width  = 1920;
static u32_t isp_video_height = 1080;
//static u32_t isp_video_width  = 1280;
//static u32_t isp_video_height = 720;
#if CZS_USB_01
static u32_t isp_sensor_bit = ARKN141_ISP_SENSOR_BIT_10;		// 创智视使用10bit 1080p
#else
static u32_t isp_sensor_bit = ARKN141_ISP_SENSOR_BIT_12;
#endif

static unsigned int isp_yuv_format = ARKN141_ISP_YUV_FORMAT_Y_UV420;		// ISP的YUV输出格式定义
												// 	0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 
#endif

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX291
static u32_t isp_video_width  = 1920;
static u32_t isp_video_height = 1080;
static u32_t isp_sensor_bit = ARKN141_ISP_SENSOR_BIT_12;

static unsigned int isp_yuv_format = ARKN141_ISP_YUV_FORMAT_Y_UV420;		// ISP的YUV输出格式定义
												// 	0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 
#endif

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_AR0330 || ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_AR0238 || ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_AR0230
// 1920*1080 12bit	AR0238/AR0330
static u32_t isp_video_width  = 1920;
static u32_t isp_video_height = 1080;
static u32_t isp_sensor_bit = ARKN141_ISP_SENSOR_BIT_12;
static unsigned int isp_yuv_format = ARKN141_ISP_YUV_FORMAT_Y_UV420;		// ISP的YUV输出格式定义
												// 	0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 
#endif

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_PS1210K
// 1920*1080 10bit		PS1210
static u32_t isp_video_width  = 1920;
static u32_t isp_video_height = 1080;
static u32_t isp_sensor_bit = ARKN141_ISP_SENSOR_BIT_10;
static unsigned int isp_yuv_format = ARKN141_ISP_YUV_FORMAT_Y_UV420;		// ISP的YUV输出格式定义
												// 	0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 
#endif


#if 0		
// 1280*720 10bit		IMX322
static u32_t isp_video_width  = 1280;
static u32_t isp_video_height = 720;
static u32_t isp_sensor_bit = ARKN141_ISP_SENSOR_BIT_10;
//static u32_t isp_sensor_bit = ARKN141_ISP_SENSOR_BIT_12;
static unsigned int isp_yuv_format = ARKN141_ISP_YUV_FORMAT_Y_UV420;	//ARKN141_ISP_YUV_FORMAT_YUV422;		// ISP的YUV输出格式定义
												// 	0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 
//static unsigned int isp_yuv_format = ARKN141_ISP_YUV_FORMAT_YUV422;		// ISP的YUV输出格式定义
#endif

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_BG0806
static u32_t isp_video_width  = 1280;//1920;
static u32_t isp_video_height = 720;//1080;
static u32_t isp_sensor_bit = ARKN141_ISP_SENSOR_BIT_12;
static unsigned int isp_yuv_format = ARKN141_ISP_YUV_FORMAT_Y_UV420;		// ISP的YUV输出格式定义
#endif

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_GC0308
static u32_t isp_video_width  = 640;
static u32_t isp_video_height = 480;
static u32_t isp_sensor_bit = ARKN141_ISP_SENSOR_BIT_10;
static unsigned int isp_yuv_format = ARKN141_ISP_YUV_FORMAT_Y_UV420;		// ISP的YUV输出格式定义
#endif

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_JX_H65
static u32_t isp_video_width  = 1280;
static u32_t isp_video_height = 720;
static u32_t isp_sensor_bit = ARKN141_ISP_SENSOR_BIT_10;
static unsigned int isp_yuv_format = ARKN141_ISP_YUV_FORMAT_Y_UV420;		// ISP的YUV输出格式定义
#endif


static isp_param_t g_isp_param;
isp_sys_t p_sys;
static isp_sen_t p_sen; 
isp_colors_t p_colors;
isp_denoise_t p_denoise;
isp_eris_t p_eris;
isp_fesp_t p_fesp;
isp_enhance_t p_enhance;
isp_awb_t p_awb;
isp_ae_t p_ae;
cmos_exposure_t isp_exposure;

extern isp_ae_frame_record_t ae_rcd;

// 缺省开启所有auto-run项目
static unsigned int isp_auto_run_state = (unsigned int)0xFFFFFFFF;

//#define	ISP_RAW_ENABLE


#if ISP_3D_DENOISE_SUPPORT
#pragma data_alignment = ISP_DATA_ALIGNMENT_NUM
__no_init static double ref_buf [IMAGE_H_SZ_MAX*IMAGE_V_SZ_MAX*2/8];
#endif

#ifdef ISP_RAW_ENABLE
// 以下仅考虑YUV420或者Y_UV420格式抓取RAW数据, 未考虑YUV422或者Y_UV422格式
#pragma data_alignment = ISP_DATA_ALIGNMENT_NUM
__no_init double raw_buf0 [IMAGE_H_SZ_MAX*IMAGE_V_SZ_MAX*2/8];
#pragma data_alignment = ISP_DATA_ALIGNMENT_NUM
__no_init double raw_buf1 [IMAGE_H_SZ_MAX*IMAGE_V_SZ_MAX*2/8];
#pragma data_alignment = ISP_DATA_ALIGNMENT_NUM
__no_init double raw_buf2 [IMAGE_H_SZ_MAX*IMAGE_V_SZ_MAX*2/8];
#pragma data_alignment = ISP_DATA_ALIGNMENT_NUM
__no_init double raw_buf3 [IMAGE_H_SZ_MAX*IMAGE_V_SZ_MAX*2/8];
#endif

#pragma data_alignment = ISP_DATA_ALIGNMENT_NUM
__no_init double yuv_buf0[IMAGE_H_SZ_MAX*IMAGE_V_SZ_MAX*3/2/8];
#pragma data_alignment = ISP_DATA_ALIGNMENT_NUM
__no_init double yuv_buf1[IMAGE_H_SZ_MAX*IMAGE_V_SZ_MAX*3/2/8];
#pragma data_alignment = ISP_DATA_ALIGNMENT_NUM
__no_init double yuv_buf2[IMAGE_H_SZ_MAX*IMAGE_V_SZ_MAX*3/2/8];
#pragma data_alignment = ISP_DATA_ALIGNMENT_NUM
__no_init double yuv_buf3[IMAGE_H_SZ_MAX*IMAGE_V_SZ_MAX*3/2/8];

void set_isp_autoae(unsigned char auto_ae )
{
    
}

// 设置ISP auto-run项目的状态
// state --> 1  使能auto run
// state --> 0  禁止auto run
void isp_set_auto_run_state (unsigned int item, unsigned int state)
{
	if(item >= ISP_AUTO_RUN_COUNT)
		return;
	if(state)
		isp_auto_run_state |=  (1 << item);
	else
		isp_auto_run_state &= ~(1 << item);
}

unsigned int isp_get_auto_run_state  (unsigned int item)
{
	if(item >= ISP_AUTO_RUN_COUNT)
		return 0;
	return isp_auto_run_state & (1 << item);
}


unsigned int *isp_get_yuv_buffer (unsigned int id)
{
	if(id == 0)
		return (unsigned int *)yuv_buf0;
	else if(id == 1)
		return (unsigned int *)yuv_buf1;
	else if(id == 2)
		return (unsigned int *)yuv_buf2;
	else //if(id == 3)
		return (unsigned int *)yuv_buf3;
}

void isp_set_video_format (unsigned int video_format)
{
	isp_yuv_format = video_format & 3;
}

//0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 
unsigned int isp_get_video_format (void)
{
	return isp_yuv_format;
}

unsigned int isp_get_sensor_bit (void)
{
	return isp_sensor_bit;
}

void isp_set_sensor_bit (u32_t sensor_bit)
{
	isp_sensor_bit = sensor_bit;
}


// 设置ISP视频的输出尺寸 (1080P或者720P)
void isp_set_video_width (u32_t width)
{
	//if(width == 1920 || width == 1280)
		isp_video_width = width;
}

void isp_set_video_height (u32_t height)
{
	//if(height == 1080 || height == 720)
		isp_video_height = height;
}

// 获取ISP视频的输出尺寸定义(1080P或者720P)
u32_t isp_get_video_width  (void)
{
	return isp_video_width; 	
}

u32_t isp_get_video_height (void)
{
	return isp_video_height;
}

// 设置/获取ISP RAW图像的BAYER 模式
unsigned int isp_get_raw_image_bayer_mode (void)
{
	return isp_raw_image_bayer_mode;
}

void isp_set_raw_image_bayer_mode (unsigned int bayer_mode)
{
	if(bayer_mode > ARKN141_ISP_RAW_IMAGE_BAYER_MODE_GBRG)
	{
		XM_printf ("illegal bayer mode (%d)\n", bayer_mode);
		return;
	}
	isp_raw_image_bayer_mode = bayer_mode;
}


unsigned int isp_get_video_image_size (void)
{
	if(isp_yuv_format == ARKN141_ISP_YUV_FORMAT_Y_UV420 || isp_yuv_format == ARKN141_ISP_YUV_FORMAT_YUV420)
		return isp_video_width * isp_video_height * 3 / 2;
	else	// ARKN141_ISP_YUV_FORMAT_Y_UV422 ARKN141_ISP_YUV_FORMAT_Y_UV422
		return isp_video_width * isp_video_height * 2;
}

void set_isp_ae_lumTarget (unsigned char lumTarget )
{
    p_ae.lumTarget = lumTarget&0xff ; 
}

unsigned char get_isp_ae_lumTarget (  )
{
    return (p_ae.lumTarget&0xff) ; 
}


unsigned char *get_ref_buf()
{
#if ISP_3D_DENOISE_SUPPORT
   return (unsigned char *)ref_buf;
#else	
	return NULL;
#endif
}
void  io_write(unsigned int addr, unsigned int val )
{
	*(volatile unsigned int *)addr = val;
}
unsigned int  io_read(unsigned int addr )
{
	return (*(volatile unsigned int *)addr) ;
}
void io_write_bak(unsigned int addr,unsigned int val,unsigned int bak)
{
   unsigned int writeval ;
   writeval = io_read( addr );
   writeval = (writeval&bak)|val;
   io_write( addr,  writeval );
}



void isp_videobuf_alloc (isp_param_ptr_t p_isp)
{
  	unsigned int u_offset, v_offset, i, *refb;
	unsigned int video_format = isp_get_video_format ();
	
	p_isp->image_width = IMAGE_H_SZ ;
	p_isp->image_height = IMAGE_V_SZ ;
	p_isp->image_stride = (p_isp->image_width+0xf)&0xfff0;
	
	
	// Y_UV420/YUV420
	if(video_format == 0 || video_format == 2)
	{
		u_offset = p_isp->image_stride*p_isp->image_height;
		v_offset = (u_offset) + (u_offset>>2);
	}
	// Y_UV422/YUV422
	else
	{
		u_offset = p_isp->image_stride*p_isp->image_height;
		v_offset = (u_offset) + (u_offset>>1);
		
	}
	
#if ISP_3D_DENOISE_SUPPORT
	p_isp->ref_addr = (unsigned int)(ref_buf);
	dma_inv_range ((unsigned int)ref_buf, sizeof(ref_buf) + (unsigned int)ref_buf);
#else
	p_isp->ref_addr = 0;
#endif
	
#ifdef ISP_RAW_ENABLE
	p_isp->raw_addr[0] = (unsigned int)(raw_buf0);
	p_isp->raw_addr[1] = (unsigned int)(raw_buf1);
	p_isp->raw_addr[2] = (unsigned int)(raw_buf2);
	p_isp->raw_addr[3] = (unsigned int)(raw_buf3);
#else
	p_isp->raw_addr[0] = (unsigned int)(0);
	p_isp->raw_addr[1] = (unsigned int)(0);
	p_isp->raw_addr[2] = (unsigned int)(0);
	p_isp->raw_addr[3] = (unsigned int)(0);
	
#endif
   //XM_printf("raw_addr[0]:0x%08x\n",p_isp->raw_addr[0]);
   //XM_printf("raw_addr[1]:0x%08x\n",p_isp->raw_addr[1]);
   //XM_printf("raw_addr[2]:0x%08x\n",p_isp->raw_addr[2]);
   //XM_printf("raw_addr[3]:0x%08x\n",p_isp->raw_addr[3]);	
	
	p_isp->y_addr[0] = (unsigned int)(yuv_buf0);
	p_isp->u_addr[0] = p_isp->y_addr[0] + u_offset;
	p_isp->v_addr[0] = p_isp->y_addr[0] + v_offset;
	//XM_printf("yuv_buf0:0x%08x 0x%08x 0x%08x\n",p_isp->y_addr[0],p_isp->u_addr[0],p_isp->v_addr[0]);
	
	p_isp->y_addr[1] = (unsigned int)(yuv_buf1);
	p_isp->u_addr[1] = p_isp->y_addr[1] + u_offset;
	p_isp->v_addr[1] = p_isp->y_addr[1] + v_offset;
	//XM_printf("yuv_buf1:0x%08x 0x%08x 0x%08x\n",p_isp->y_addr[1],p_isp->u_addr[1],p_isp->v_addr[1]);
	
	p_isp->y_addr[2] = (unsigned int)(yuv_buf2);
	p_isp->u_addr[2] = p_isp->y_addr[2] + u_offset;
	p_isp->v_addr[2] = p_isp->y_addr[2] + v_offset;
	//XM_printf("yuv_buf2:0x%08x 0x%08x 0x%08x\n",p_isp->y_addr[2],p_isp->u_addr[2],p_isp->v_addr[2]);
	
	p_isp->y_addr[3] = (unsigned int)(yuv_buf3);
	p_isp->u_addr[3] = p_isp->y_addr[3] + u_offset;
	p_isp->v_addr[3] = p_isp->y_addr[3] + v_offset;
	//XM_printf("yuv_buf3:0x%08x 0x%08x 0x%08x\n",p_isp->y_addr[3],p_isp->u_addr[3],p_isp->v_addr[3]);
	
	dma_inv_range ((unsigned int)yuv_buf0, sizeof(yuv_buf0) + (unsigned int)yuv_buf0);
	dma_inv_range ((unsigned int)yuv_buf1, sizeof(yuv_buf1) + (unsigned int)yuv_buf1);
	dma_inv_range ((unsigned int)yuv_buf2, sizeof(yuv_buf2) + (unsigned int)yuv_buf2);
	dma_inv_range ((unsigned int)yuv_buf3, sizeof(yuv_buf3) + (unsigned int)yuv_buf3);
	
	p_isp->yuv_id[0] = 0;
	p_isp->yuv_id[1] = 1;
	p_isp->yuv_id[2] = 2;
	p_isp->yuv_id[3] = 3;
}


void isp_videobuf_init (isp_sys_ptr_t p_sys, isp_param_ptr_t p_isp)
{
	//p_sys->bayermode = _ISP_BAYER_MODE_;
	p_sys->imagewidth = p_isp->image_width;
	p_sys->imageheight = p_isp->image_height;
	// p_sys->imagehblank = 200;  
 // p_sys->imagehblank =500;	// 调整该值("行列场"的列消隐周期)可适配未完成的帧写入. 
  										// ISP Core内部使用"ISP行列场"逻辑将帧数据写入到外部存储,
  										//	该值等价于该"ISP行列场"的列消隐周期, 
  										//	列消隐周期的设置需满足ISP帧数据的完整写入.
  										//	该值与ISP Core的时钟相关
	p_sys->hstride = p_isp->image_stride; //图像跨度 16字节倍数
	p_sys->refaddr = p_isp->ref_addr;    //参考帧地址
	p_sys->rawaddr0 = p_isp->raw_addr[0];   
	p_sys->rawaddr1 = p_isp->raw_addr[1];   
	p_sys->rawaddr2 = p_isp->raw_addr[2];   
	p_sys->rawaddr3 = p_isp->raw_addr[3];   
	p_sys->yaddr0 = p_isp->y_addr[0]; 
	p_sys->uaddr0 = p_isp->u_addr[0];
	p_sys->vaddr0 = p_isp->v_addr[0];

	p_sys->yaddr1 = p_isp->y_addr[1];    
	p_sys->uaddr1 = p_isp->u_addr[1];
	p_sys->vaddr1 = p_isp->v_addr[1];

	p_sys->yaddr2 = p_isp->y_addr[2]; 
	p_sys->uaddr2 = p_isp->u_addr[2]; 
	p_sys->vaddr2 = p_isp->v_addr[2];

	p_sys->yaddr3 = p_isp->y_addr[3];              
	p_sys->uaddr3 = p_isp->u_addr[3];     
	p_sys->vaddr3 = p_isp->v_addr[3];
}


void isp_set_sensor_readout_direction (unsigned int horz_reverse_direction, unsigned int vert_reverse_direction)
{
	arkn141_isp_set_sensor_readout_direction (&isp_exposure, horz_reverse_direction, vert_reverse_direction);
}


int arkn141_isp_ae_set (unsigned int ae_compensation, unsigned int ae_black_target, unsigned int ae_bright_target )
{
	isp_system_ae_bright_target_write (&isp_exposure.cmos_ae, ae_bright_target);
	isp_system_ae_black_target_write  (&isp_exposure.cmos_ae, ae_black_target);
	isp_system_ae_compensation_write (&isp_exposure.cmos_ae, ae_compensation);
	
	isp_auto_exposure_compensation (&isp_exposure.cmos_ae, isp_exposure.cmos_ae.histogram.bands);	
	return 0;
}

int arkn141_isp_ae_get (unsigned int * ae_compensation, unsigned int * ae_black_target, unsigned int *ae_bright_target )
{
	u8_t bright_target, black_target, compensation;
	bright_target = isp_system_ae_bright_target_read (&isp_exposure.cmos_ae);
	black_target = isp_system_ae_black_target_read (&isp_exposure.cmos_ae);
	compensation = isp_system_ae_compensation_read (&isp_exposure.cmos_ae);
	*ae_compensation = compensation;
	*ae_black_target = black_target;
	*ae_bright_target = bright_target;
	return 0;
}


const char *isp_get_sensor_name (void)
{
	cmos_sensor_t	*cmos_sensor = &isp_exposure.cmos_sensor;
	if(cmos_sensor && cmos_sensor->cmos_sensor_get_sensor_name)
		return (*cmos_sensor->cmos_sensor_get_sensor_name)();
	else
		return "SENSOR";
}

void get_isp_dispaddr(unsigned int order,unsigned int *Yaddr, unsigned int *Uaddr , unsigned int *Vaddr )
{
	*Yaddr = g_isp_param.y_addr[order];
	*Uaddr = g_isp_param.u_addr[order];
	*Vaddr = g_isp_param.v_addr[order];
}

void get_isp_rawaddr(unsigned int order,unsigned int *Yaddr  )
{
	*Yaddr = g_isp_param.raw_addr[order];
}

#include "rtos.h"
#include "xm_core.h"
#include <assert.h>

#define	ISP_IDLE		0
#define	ISP_RUN		1

static volatile unsigned int isp_state = 0;
static volatile unsigned int isp_stop = 0;
#ifdef ISP_RAW_ENABLE
static volatile unsigned int isp_mode = ISP_WORK_MODE_RAW;
#else
static volatile unsigned int isp_mode = ISP_WORK_MODE_NORMAL;
#endif

#ifdef USE_ISR_TO_SWITCH_MODE
volatile unsigned int new_isp_mode = (unsigned int)(-1);
#endif

#pragma data_alignment=32
OS_STACKPTR int Stack_Isp[XMSYS_ISP_TASK_STACK_SIZE/4];          /* Task stacks */
#pragma data_alignment=32
OS_TASK TCB_Isp;                        /* Task-control-blocks */
#pragma data_alignment=32
// ISP控制信息访问互斥信号量
static OS_RSEMA				IspSema;	

static OS_TIMER IspTimer;		// 定时器


void xm_isp_init (void)
{
	OS_CREATERSEMA(&IspSema); /* Creates resource semaphore */
}

void xm_isp_exit (void)
{
	OS_DeleteRSema (&IspSema); /* Creates resource semaphore */
}

void isp_ae_done_event_set (void)
{
	OS_SignalEvent (ISP_EVENT_INFO_DONE, &TCB_Isp);
}

unsigned int isp_get_work_mode (void)
{
	return isp_mode;
}

int isp_set_work_mode (unsigned int mode)
{
#ifndef ISP_RAW_ENABLE
	if(mode == ISP_WORK_MODE_RAW)
	{
		XM_printf ("current version don't support RAW\n");
		return -1;		// 禁止RAW写入模式
	}
#endif
	
	if(mode >= ISP_WORK_MODE_COUNT)
		return -1;
	
	if(isp_mode == mode)
		return 0;
	
	if(isp_state == ISP_IDLE)
	{
		return -1;
	}
	
#ifdef USE_ISR_TO_SWITCH_MODE
	isp_mode = mode;
	new_isp_mode = isp_mode;
	OS_Delay (100); // 等待至少3帧
	if(new_isp_mode == (unsigned int)(-1))
	{
		XM_printf ("isp mode switch to (%s) OK\n", isp_get_work_mode_name(isp_mode) );
	}
	else
	{
		XM_printf ("isp mode switch to (%s) NG\n", isp_get_work_mode_name(isp_mode) );
	}
#else	
	isp_mode = mode;
	OS_SignalEvent (ISP_EVENT_MODE_CHANGE, &TCB_Isp);
#endif
	return 0;
}

static const char *isp_work_mode_name[] = {
	"Normal",
	"Raw",
	"AutoTest",
	"Null"
};

static const char *isp_get_work_mode_name (unsigned int mode)
{
	if(mode >= ISP_WORK_MODE_COUNT)
		return isp_work_mode_name[ISP_WORK_MODE_COUNT];
	else
		return isp_work_mode_name[mode];
}

extern void isp_sensor_set_reset_pin_high (void);
extern void isp_sensor_set_reset_pin_low (void);
// ISP上电复位
void isp_cold_reset (void);

static void IspTicketCallback (void)
{
	OS_SignalEvent (ISP_EVENT_TICKET, &TCB_Isp); /* 通知事件 */
	OS_RetriggerTimer (&IspTimer);
}

// 星光模式监控
// Bug 20170913 清晨场景录像画面一直在亮暗闪烁
//   分析视频流记录, 发现场景的亮度在 2(模拟增益3.20) ~ 15(模拟增益8.00) 之间变换, 刚好满足原来定义的星光模式(Low-->4  ~  High-->14)切换阈值.
//   修改上述Bug需要扩大最小最大阈值为( Low-->3, High-->16 ), 降低模式反复切换的概率
//#define	DIM_LUM_HIGH			(14)
//#define	DIM_LUM_LOW				(4)
#define	DIM_LUM_HIGH			(16)
#define	DIM_LUM_LOW				(2)

#define	DIM_LIGHT_COUNT		25	// 1.5秒

static unsigned int dim_light_lum_count;
static unsigned int dim_light_lum_index;
static unsigned short dim_light_lum_value[DIM_LIGHT_COUNT];
static void dim_light_mode_init (void)
{
	dim_light_lum_index = 0;
	dim_light_lum_count = 0;
	memset (dim_light_lum_value, 0, sizeof(dim_light_lum_value));
}

// 星光模式监控
// 根据场景的光照强度自动设置白天/夜晚模式
static void dim_light_mode_monitor (unsigned int inttime_gain)
{
#if 1
	// 使用增益值区分白天/夜晚模式
	if(inttime_gain >= 30 * 1125)
	{
		if(AP_GetMenuItem(APPMENUITEM_DAY_NIGHT_MODE) == 0)
		{
			XM_printf ("switch to night mode\n");
			AP_SetMenuItem (APPMENUITEM_DAY_NIGHT_MODE, 1);
		}
	}
	else
	{
		if(AP_GetMenuItem(APPMENUITEM_DAY_NIGHT_MODE) == 1)
		{
			XM_printf ("switch to day mode\n");
			AP_SetMenuItem (APPMENUITEM_DAY_NIGHT_MODE, 0);
		}
	}
#else
	if(dim_light_lum_count < DIM_LIGHT_COUNT)
	{
		// 采集样本未满
		dim_light_lum_value[dim_light_lum_count] = Gem_read (GEM_AE1_BASE+0x00) & 0xff;		// 亮度平均值
		dim_light_lum_count ++;
	}
	else
	{
		unsigned int i;
		unsigned int lum_total;
		unsigned int lum_averg;
		// 采集样本已满
		dim_light_lum_value[dim_light_lum_index] = Gem_read (GEM_AE1_BASE+0x00) & 0xff;
		dim_light_lum_index ++;
		if(dim_light_lum_index >= DIM_LIGHT_COUNT)
			dim_light_lum_index = 0;
		
		// 计算亮度均值
		lum_total = 0;
		for (i = 0; i < DIM_LIGHT_COUNT; i ++)
		{
			lum_total += dim_light_lum_value[i];
		}
		lum_averg = (lum_total + DIM_LIGHT_COUNT/2) / DIM_LIGHT_COUNT;
		
		// 根据场景的光照强度自动设置白天/夜晚模式
	
		// 1) 低亮度场景下启动星光模式
		if(lum_averg <  DIM_LUM_LOW)	
		{
			// 开启夜间模式
			if(AP_GetMenuItem(APPMENUITEM_DAY_NIGHT_MODE) == 0)
			{
				XM_printf ("switch to night mode\n");
				AP_SetMenuItem (APPMENUITEM_DAY_NIGHT_MODE, 1);
				
				if(isp_exposure.cmos_sensor.cmos_isp_set_day_night_mode)
					(*isp_exposure.cmos_sensor.cmos_isp_set_day_night_mode) (&isp_exposure.cmos_gain, 1);
			}
		}
		// 2) 高亮度场景下禁止星光模式
		else if(lum_averg > DIM_LUM_HIGH)	
		{
			// 切换回白天模式
			if(AP_GetMenuItem(APPMENUITEM_DAY_NIGHT_MODE) == 1)
			{
				XM_printf ("switch to day mode\n");
				AP_SetMenuItem (APPMENUITEM_DAY_NIGHT_MODE, 0);
				
				if(isp_exposure.cmos_sensor.cmos_isp_set_day_night_mode)
					(*isp_exposure.cmos_sensor.cmos_isp_set_day_night_mode) (&isp_exposure.cmos_gain, 0);
			}
		}
		// 3) 其他场景下维持原有的模式
	}
#endif
}

#define	BACK_LIGHT_COUNT	15
// Back Light 逆光模式监控
static int backlight_max_lum[BACK_LIGHT_COUNT];
static int backlight_ground_lum[BACK_LIGHT_COUNT];
static unsigned int backlight_lum_count;
static unsigned int backlight_lum_index;
static unsigned int backlight_ae_enable;

static void back_light_mode_init (void)
{
	backlight_lum_index = 0;
	backlight_lum_count = 0;
	backlight_ae_enable = 1;
	memset (backlight_max_lum, 0, sizeof(backlight_max_lum));
	memset (backlight_ground_lum, 0, sizeof(backlight_ground_lum));
}

void cmos_exposure_set_back_light_ae (int enable)
{
	backlight_ae_enable = enable;
	if(backlight_ae_enable == 0)
	{
		isp_system_ae_compensation_write (&isp_exposure.cmos_ae, (u8_t)AE_COMPENSATION_DEFAULT);	
	}
}


void back_light_mode_monitor (unsigned int inttime_gain)
{
	int i;
	int max_lum, ground_lum;
	unsigned short *yavg_s;
	if(backlight_ae_enable == 0)
	{
		return;
	}
	
	yavg_s = &p_ae.yavg_s[0][0];
	max_lum = 0;
	XM_lock ();
	for (i = 0; i < 9; i ++)
	{
		if(max_lum < yavg_s[i])
			max_lum = yavg_s[i];
	}
	ground_lum = p_ae.yavg_s[2][0] + p_ae.yavg_s[2][1] + p_ae.yavg_s[2][2];
	XM_unlock ();
	if(backlight_lum_count < BACK_LIGHT_COUNT)
	{
		backlight_max_lum[backlight_lum_count] = max_lum;
		backlight_ground_lum[backlight_lum_count] = ground_lum;
		backlight_lum_count ++;
	}
	else
	{
		backlight_max_lum[backlight_lum_index] = max_lum;
		backlight_ground_lum[backlight_lum_index] = ground_lum;
		backlight_lum_index ++;
		if(backlight_lum_index >= BACK_LIGHT_COUNT)
			backlight_lum_index = 0;
		
		int total_max, total_ground;
		int ratio;
		u8_t compensation;
		
		total_max = 0;
		total_ground = 0;
		for (i = 0; i < BACK_LIGHT_COUNT; i ++)
		{
			total_max += backlight_max_lum[i];
			total_ground += backlight_ground_lum[i];
		}
		if(total_ground == 0)
			total_ground ++;
		ratio = total_max / total_ground;
		unsigned int old_compensation = isp_system_ae_compensation_read (&isp_exposure.cmos_ae);
		float f_compensation = AE_COMPENSATION_DEFAULT;
		f_compensation = f_compensation * total_max / total_ground;
		f_compensation /= 2;
		if(f_compensation > 144)
			f_compensation = 144;
		else if(f_compensation < AE_COMPENSATION_DEFAULT)
			f_compensation = AE_COMPENSATION_DEFAULT;
		
		compensation = (u8_t)f_compensation;
		
		if(old_compensation != compensation)
		{
			isp_system_ae_compensation_write (&isp_exposure.cmos_ae, (u8_t)compensation);	
			isp_auto_exposure_compensation (&isp_exposure.cmos_ae, isp_exposure.cmos_ae.histogram.bands);	
			XM_printf ("comp %d --> %d\n", old_compensation, compensation);
		}
	}
	
	
	
}

static unsigned int eris_dimlight_enable = 1;

void cmos_exposure_set_eris_dimlight (unsigned int enable)
{
	eris_dimlight_enable = enable;
}

unsigned int cmos_exposure_get_eris_dimlight (void)
{
	return eris_dimlight_enable;
}

static unsigned char raw_sensor_exist = 0;		// 摄像头检查标志， 1 正常 0 错误

unsigned char XMSYS_IspGetRawSensorState (void)
{
	return raw_sensor_exist;
}

void isp_task (void)
{
	OS_U8 isp_event;
	unsigned int inttime_gain, new_inttime_gain;
	unsigned int light_freq;
	
	int reset_continue;
	
	int block_7116 = 0;	
	unsigned int block_7116_timeout;
	unsigned int frame_ticket;		// 当前帧时间, 计算相邻帧之间的之间, 判断ISP是否异常, 需要复位
		
	//DEBUG_PRINT("isp_task\n");
	isp_state = ISP_RUN;

next_loop:	
	
	enter_region_to_protect_7116_setup ();
	//阻止7116初始化, 直到保护超时时间已达到
	block_7116 = 1;
	block_7116_timeout = XM_GetTickCount () + 300;	// 300ms内阻止7116执行初始化
	
   isp_SelPad();
	
	reset_continue = 0;
	
	// ISP reset
	isp_clock_stop();
   sys_soft_reset (softreset_isp);
	
	// sensor reset
	isp_sensor_set_reset_pin_low ();
	// ISP时钟设置
	isp_clock_init ();
	isp_sensor_set_reset_pin_high ();
		
	isp_i2c_init();
		
	isp_videobuf_alloc (&g_isp_param);
	
	isp_ae_event_init ();	// 用于多次曝光稳定的事件
	
	// 初始化sensor实例
	
	arkn141_isp_ae_initialize (&isp_exposure);
	cmos_sensor_t	*cmos_sensor = &isp_exposure.cmos_sensor;
	
	// 根据设置值配置ISP参数
	unsigned int sharpening_value = AP_GetMenuItem(APPMENUITEM_SHARPENING);
	if(sharpening_value == 0)
	{
		p_enhance.sharp.strength = 32;		// 轻微
	}
	else if(sharpening_value == 1)
	{
		p_enhance.sharp.strength = 64;		// 柔和
	}
	else if(sharpening_value == 2)
	{
		p_enhance.sharp.strength = 160;	// 强烈
	}
	
	// 初始化sensor
	if((*cmos_sensor->cmos_isp_sensor_init) (&p_sen) < 0)
	{
		// sensor seems bad or no response
		// 发送sensor故障消息
		XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CCD0_LOST_CONNECT );
		raw_sensor_exist = 0;
		goto sensor_no_response;
	}
	else
	{
		raw_sensor_exist = 1;
	}
	
	OS_Delay (100);
	
	if(cmos_sensor->cmos_isp_colors_init)
	{
	//	DEBUG_PRINT ("cmos_isp_colors_init\n");
		cmos_sensor->cmos_isp_colors_init(&p_colors);
	}
	else
	{
		XM_printf ("Warning: Miss cmos_isp_colors_init\n");
	}
	
	if(cmos_sensor->cmos_isp_denoise_init)
	{
		//DEBUG_PRINT ("cmos_isp_denoise_init\n");
		cmos_sensor->cmos_isp_denoise_init (&p_denoise);
	}
	else
	{
		XM_printf ("Warning: Miss cmos_isp_denoise_init\n");
	}
		
	if(cmos_sensor->cmos_isp_eris_init)
	{
		//DEBUG_PRINT ("cmos_isp_eris_init\n");
		cmos_sensor->cmos_isp_eris_init (&p_eris);
	}
	else
		XM_printf ("Warning: Miss cmos_isp_eris_init\n");
	
	if(cmos_sensor->cmos_isp_fesp_init)
	{
		//DEBUG_PRINT ("cmos_isp_fesp_init\n");
		cmos_sensor->cmos_isp_fesp_init (&p_fesp);
	}
	else
		XM_printf ("Warning: Miss cmos_isp_fesp_init\n");
	
	if(cmos_sensor->cmos_isp_enhance_init)
	{
		//DEBUG_PRINT ("cmos_isp_enhance_init\n");
		cmos_sensor->cmos_isp_enhance_init (&p_enhance);
	}
	else
		XM_printf ("Warning: Miss cmos_isp_enhance_init\n");
	
	if(cmos_sensor->cmos_isp_awb_init)
	{
		//DEBUG_PRINT ("cmos_isp_awb_init\n");
		cmos_sensor->cmos_isp_awb_init (&p_awb);
	}
	else
		XM_printf ("Warning: Miss cmos_isp_awb_init\n");
	
	
	if(cmos_sensor->cmos_isp_ae_init)
	{
		//DEBUG_PRINT ("cmos_isp_ae_init\n");
		cmos_sensor->cmos_isp_ae_init (&p_ae);
	}
	else
		XM_printf ("Warning: Miss cmos_isp_ae_init\n");

	if(cmos_sensor->cmos_isp_sys_init)
	{
		//DEBUG_PRINT("run isp_sys_init()\n");
		cmos_sensor->cmos_isp_sys_init (&p_sys, &g_isp_param);
	}
	else
		XM_printf ("Warning: Miss cmos_isp_sys_init\n");
	
	// 中断初始化
	isp_int_init ();
	
	light_freq = AP_GetMenuItem(APPMENUITEM_LIGHT_FREQ);
	if(light_freq == AP_SETTING_LIGHT_FREQ_50HZ)
	{
		xm_arkn141_isp_set_flicker_freq (50);
	}
	else if(light_freq == AP_SETTING_LIGHT_FREQ_60HZ)
	{
		xm_arkn141_isp_set_flicker_freq (60);
	}
	else
	{
		xm_arkn141_isp_set_flicker_freq (0);
	}	
   	
	inttime_gain = (unsigned int)(-1);
	
	//OS_Delay (150);
	
	// ISP系统初始化并启动ISP
   isp_sys_init_io (&p_sys);
	
	// 创建一个33ms的时钟 (30Hz)
	OS_CREATETIMER (&IspTimer, IspTicketCallback, 33);
	
	
	XM_printf ("\nISP AE start, %d\n\n", XM_GetTickCount());
	
	frame_ticket = 0;
		
	dim_light_mode_init ();
	
	back_light_mode_init ();
	
	while(1)
	{
	  	isp_event = OS_WaitEvent (	ISP_EVENT_INFO_DONE
										 |	ISP_EVENT_AE_STOP
										 |	ISP_EVENT_ABNORMAL
										 | ISP_EVENT_MODE_CHANGE
										 | ISP_EVENT_TICKET	 
										);
		//OS_EnterRegion();
		//XM_printf ("isp_event=0x%08x\n", isp_event);
		//OS_LeaveRegion();	
		
		// ISP启动造成的影响已趋于稳定, 允许7116初始化
		if(block_7116 && XM_GetTickCount() >= block_7116_timeout )
		{
			leave_region_to_protect_7116_setup ();
			block_7116 = 0;
		}
		
		if(isp_event & ISP_EVENT_TICKET)
		{
			// 100ms无响应, 认为I2C异常
			if( frame_ticket == 0 )
				frame_ticket = XM_GetTickCount ();
#ifdef ISP_RAW_ENABLE
			// ISP DEBUG时关闭
			else if ( (XM_GetTickCount() - frame_ticket) > 1000 )
			{
				frame_ticket = XM_GetTickCount();
				XM_printf ("No Frame Recv (%d)\n", frame_ticket/1000); 
			}
#else
			else if ( (XM_GetTickCount() - frame_ticket) > 100 )
			{
				XM_printf ("ISP abnormal & restart\n");
		
				reset_continue = 1;
				break;
			}
#endif
		}
		
		if( (isp_event & ISP_EVENT_AE_STOP) || isp_stop)
		{
			// 关闭通道复位, 防止ISP关闭时死锁
			reset_continue = 0;
			break;
		}
		if(isp_event & ISP_EVENT_ABNORMAL)
		{
			// 异常处理
		}
		if(isp_event & ISP_EVENT_MODE_CHANGE)
		{
			unsigned int data0;
			
#ifdef USE_ISR_TO_SWITCH_MODE
#else	
			isp_disable ();
			OS_Delay (70);	// 等待至少1帧
			
#if ISP_3D_DENOISE_SUPPORT
			if(isp_mode == ISP_WORK_MODE_NORMAL)	// 正常模式下开启3D
				p_denoise.enable3d = 7;
			else
				p_denoise.enable3d = 0;
			data0 =  ((p_denoise.enable2d & 0x07) <<  0) 
					| ((p_denoise.enable3d & 0x07) <<  3) 
					| ((p_denoise.sel_3d_table & 0x03) << 8)	// 3D高斯表选择
					| ((p_denoise.sensitiv0 & 0x07) << 10)	// 2D滤波器0(差分)灵敏度设置, 0 滤波关闭	
					| ((p_denoise.sensitiv1 & 0x07) << 13)	// 2D滤波器1(边沿)灵敏度设置, 0 滤波关闭
					| ((p_denoise.sel_3d_matrix & 0x01) << 16)		// 3D相干性矩阵区域选择， 0 邻近 1 中心点
					;
			Gem_write ((GEM_DENOISE_BASE+0x00), data0);
#endif
			//isp_denoise_init_io (&p_denoise);	
			if(isp_mode == ISP_WORK_MODE_NORMAL)	// 正常模式下开启3D
			{
				p_sys.debugmode  = 0;
				p_sys.testenable = 0; // 开启dram测试模式  
				p_sys.rawmenable = 0; // 1 允许RAW写出
				p_sys.yuvenable  = 1; // 0:关掉数据输出  1:打开
#if ISP_3D_DENOISE_SUPPORT
				p_sys.refenable  = 1; // 1;3D 参考帧开启 0:关闭 
#else
				p_sys.refenable  = 0; // 1;3D 参考帧开启 0:关闭
#endif
			}
			else
			{
				// RAW
				p_sys.debugmode  = 1;
				p_sys.testenable = 0; // 开启dram测试模式  
				p_sys.rawmenable = 1; // 1 允许RAW写出
				p_sys.yuvenable  = 1; // 0:关掉数据输出  1:打开
				p_sys.refenable  = 0; // 1;3D 参考帧开启 0:关闭 
			}
			//cmos_sensor->cmos_isp_sys_init (&p_sys, &g_isp_param);
		  	data0 = (p_sys.debugmode) 
					| (p_sys.testenable << 1) 
					| (p_sys.rawmenable << 2) 
					| (p_sys.yuvenable << 3) 
					| (p_sys.refenable << 4) 
					| (p_sys.yuvformat << 5) 
					| (p_sys.dmalock << 7) 
					| (p_sys.hstride << 16)
					;
			Gem_write ((GEM_SYS_BASE+0x14), data0);
			//isp_sys_init_io (&p_sys);
			isp_enable ();
#endif
		}		
		
		if(isp_event & ISP_EVENT_INFO_DONE)
		{
			// 仅在正确的曝光信息获取后更新当前帧时间
			frame_ticket = XM_GetTickCount ();
			{
#if 1
				// 每2帧检查一次曝光值. 曝光参数修改后不能在下一帧马上得到反馈, 需要等到设置之后的第二帧才能得到反馈.
				static unsigned int frame_index = 0;
				frame_index ++;
				if(frame_index & 1)
					continue;
#endif
			}
			
			// 逆光模式监控
			//back_light_mode_monitor (inttime_gain);
			
			// 星光模式监控
			dim_light_mode_monitor (inttime_gain);
			
			
			///XMINT64 		XM_GetHighResolutionTickCount (void);

			OS_Use(&IspSema);
			//XMINT64 t = XM_GetHighResolutionTickCount();
			isp_ae_run (&p_ae); 
			//t = XM_GetHighResolutionTickCount () - t;
			//XM_printf ("exp_t=%d\n", (unsigned int)t);
			
			//new_inttime_gain = (isp_exposure.cmos_inttime.exposure_ashort * isp_exposure.cmos_gain.again) / (1 << isp_exposure.cmos_gain.again_shift);
			new_inttime_gain = cmos_calc_inttime_gain (&isp_exposure);
			
#if XMSYS_INCLUDE_MOTION_DETECTOR
			xm_motion_detector_set_pixel_threshold (new_inttime_gain);
#endif

#if _ISP_SENSOR_DEBUG_
			// sensor调试使用
			goto next;
#endif			
			
			if(new_inttime_gain != inttime_gain)
			{
				inttime_gain = new_inttime_gain;
				
				if(isp_get_auto_run_state(ISP_AUTO_RUN_AWB) && isp_exposure.cmos_sensor.cmos_isp_awb_run)
				{
					(*isp_exposure.cmos_sensor.cmos_isp_awb_run) (&p_awb);
					// XM_printf ("\r\np_awb->gain_r2g = 0x%08x\n", p_awb.gain_r2g);
					// XM_printf ("\r\np_awb->gain_b2g = 0x%08x\n", p_awb.gain_b2g);
				}
			
				if(isp_exposure.cmos_sensor.cmos_isp_ae_run)
				{
					(*isp_exposure.cmos_sensor.cmos_isp_ae_run)(&p_ae);
				}
				
				if(isp_exposure.cmos_sensor.cmos_isp_fesp_run)
				{
					(*isp_exposure.cmos_sensor.cmos_isp_fesp_run)(&p_fesp, &p_ae);
				}
				
				if(isp_exposure.cmos_sensor.cmos_isp_eris_run)
				{
					(*isp_exposure.cmos_sensor.cmos_isp_eris_run)(&p_eris, &p_ae);
				}
				
				if(isp_exposure.cmos_sensor.cmos_isp_colors_run)
				{
					(*isp_exposure.cmos_sensor.cmos_isp_colors_run)(&p_colors, &p_awb, &p_ae);
				}
				
				if(isp_get_auto_run_state(ISP_AUTO_RUN_ENHANCE) && isp_exposure.cmos_sensor.cmos_isp_enhance_run)
				{
					(*isp_exposure.cmos_sensor.cmos_isp_enhance_run)(&p_enhance);
				}
			
				if(isp_get_auto_run_state(ISP_AUTO_RUN_DENOISE) && isp_exposure.cmos_sensor.cmos_isp_denoise_run)
				{
					(*isp_exposure.cmos_sensor.cmos_isp_denoise_run)(&p_denoise, &p_ae);
				}
			}
			
		next:
			
       	OS_Unuse(&IspSema);
		}
	}
	
	OS_DeleteTimer (&IspTimer);

sensor_no_response:
	if(block_7116 == 0)
	{
		enter_region_to_protect_7116_setup ();
		block_7116 = 1;
	}
	// 关闭中断
	isp_int_exit ();
	XM_printf ("isp_disable\n");
	// ISP停止
	// ISP停止时需要等待ISP挂起完成，否则调用isp_clock_stop停止时钟可能会导致ISP挂起从而将整个系统锁死。
	isp_disable ();
	unsigned int ticket_to_timeout = XM_GetTickCount() + 100;
	while(XM_GetTickCount() < ticket_to_timeout)
	{
		// 等待ISP挂起完成
		int int_status  = Gem_read(GEM_STS_BASE+0x00);	
		if(  int_status  & (0x01<<6) )
		{
			// ISP挂起完成
			XM_printf ("isp suspend\n");
			break;
		}
	}
	// 等待一帧延时不能确保ISP挂起已完成，需要等待3帧延时
	//OS_Delay (35);	// 等待超过1帧延时

	isp_ae_event_exit ();
	
	isp_i2c_exit();

	isp_UnSelPad ();		

	XM_printf ("isp_clock_stop\n");
	// 暂时不关闭ISP时钟， 因为ISP-scalar依赖ISP时钟。关闭ISP时钟会导致ISP-scalar挂起系统
	// isp_clock_stop ();
	
	OS_Delay (1);
	
	if(block_7116)
	{
		leave_region_to_protect_7116_setup ();
		block_7116 = 0;
	}
	
	if(reset_continue)
	{
		XMSYS_SensorCaptureReset (0);	// 复位ISP对应的sensor通道
		goto next_loop;
	}
	
	isp_state = ISP_IDLE;
	
	XM_printf ("\nISP AE stop\n\n");
	
	OS_Terminate (NULL);
}

void XMSYS_SensorStart (void)
{
	XM_printf ("XMSYS_SensorStart\n");
	
	//UART1_RX_init();
	
	if(isp_state != ISP_IDLE)
	{
		//XM_printf ("Gem_isp start before\n");
		return;
	}
	assert (isp_state == ISP_IDLE);
	OS_CREATETASK (&TCB_Isp, "Isp", isp_task, XMSYS_ISP_TASK_PRIORITY, Stack_Isp);
	// 等待ISP AE线程启动
	while(isp_state == ISP_IDLE)
	{
		OS_Delay (1);
	}
	
}

void XMSYS_SensorStop (void)
{
	XM_printf ("XMSYS_SensorStop\n");
	if(isp_state == ISP_IDLE)
		return;
	
	// 发送AE STOP事件
	OS_SignalEvent (ISP_EVENT_AE_STOP, &TCB_Isp);
	// 等待ISP AE线程退出
	isp_stop = 1;
	while(isp_state != ISP_IDLE)
	{
		OS_Delay (1);
	}
	isp_stop = 0;
}

// 获取ISP设置参数包的字节大小
unsigned short isp_get_isp_data_size (void)
{
	unsigned int size = sizeof(isp_awb_t)
							+ sizeof(isp_colors_t)
							+ sizeof(isp_denoise_t)
							+ sizeof(isp_enhance_t)
							+ sizeof(isp_eris_t)
							+ sizeof(isp_fesp_t)
							+ sizeof(isp_sys_t)
							;
	return (unsigned short)size;
}

int OS_Use_timeout (OS_RSEMA *sema, unsigned int ms_to_timeout)
{
	unsigned int ticket_to_timeout = XM_GetTickCount() + ms_to_timeout;
	while(1)
	{
		if(OS_Request(sema))
		{
			return 1;
		}
		
		if(XM_GetTickCount () >= ticket_to_timeout)
			break;
		
		OS_Delay (1);
	}
	return 0;
}

int isp_get_isp_data (char *isp_data, int isp_size)
{
	if(isp_size != isp_get_isp_data_size())
		return -1;
	
	// 20170618修复i2c异常时视频无法记录的bug
	// 修复sensor i2c无应答时, ISP线程延时重发导致sensor线程阻塞而无法将YUV帧保存到sensor帧已准备队列, 
	//     导致AVI codec无法保存视频.
	if(OS_Use_timeout(&IspSema, 10) == 0)
		return -1;
	
	memcpy (isp_data, &p_awb, sizeof(p_awb));
	isp_data += sizeof(p_awb);
	memcpy (isp_data, &p_colors, sizeof(p_colors));
	isp_data += sizeof(p_colors);
	memcpy (isp_data, &p_denoise, sizeof(p_denoise));
	isp_data += sizeof(p_denoise);
	memcpy (isp_data, &p_enhance, sizeof(p_enhance));
	isp_data += sizeof(p_enhance);
	memcpy (isp_data, &p_eris, sizeof(p_eris));
	isp_data += sizeof(p_eris);
	memcpy (isp_data, &p_fesp, sizeof(p_fesp));
	isp_data += sizeof(p_fesp);
	memcpy (isp_data, &p_sys, sizeof(p_sys));
	OS_Unuse(&IspSema);
	return isp_size;
}

// 获取ISP设置参数包的版本
unsigned int isp_get_isp_data_version (void)
{
	unsigned int ver 	= ('I' <<  0)
							| ('E' <<  8) 
							| ('1' << 16)		// big version
							| ('0' << 24)		/// little version
							;
	return ver;
}

// 获取EXP控制参数包的字节大小
unsigned short isp_get_exp_data_size (void)
{
	unsigned int size = sizeof(isp_ae_t) 
							+ sizeof(isp_ae_frame_record_t)
							;
	return size;	
}


int isp_get_exp_data (char *exp_data, int exp_size)
{
	if(exp_size != isp_get_exp_data_size())
		return -1;
	isp_ae_info_read ((isp_ae_t *)exp_data);
	isp_ae_sts2_read ((isp_ae_t *)exp_data);
	exp_data += sizeof(isp_ae_t);
	memcpy (exp_data, &ae_rcd, sizeof(ae_rcd));
	return exp_size;
}

extern cmos_exposure_t isp_exposure;

// xm_arkn141_isp_ae_set_exposure_compensation
// 	ISP设置曝光补偿
// ev	
//		曝光补偿值, -3 ~ +3
//
//	返回值
//		0	成功
//		-1	失败
int xm_arkn141_isp_set_exposure_compensation (int ev)
{
	unsigned int ae_black_target;
	unsigned int ae_bright_target;
	unsigned int ae_compensation;
	
	if(ev < -3 || ev > 3)
	{
		XM_printf ("invalid ev value (%d)\n", ev);
		return -1;
	}
	
	ae_black_target = isp_exposure.cmos_ae.ae_black_target;
	ae_bright_target = isp_exposure.cmos_ae.ae_bright_target;
	ae_compensation = AE_COMPENSATION_DEFAULT;
	
	ae_compensation += ev * 8;

	OS_EnterRegion();
	isp_system_ae_bright_target_write (&isp_exposure.cmos_ae, ae_bright_target);
	isp_system_ae_black_target_write  (&isp_exposure.cmos_ae, ae_black_target);
	isp_system_ae_compensation_write (&isp_exposure.cmos_ae, ae_compensation);
	
	isp_auto_exposure_compensation (&isp_exposure.cmos_ae, isp_exposure.cmos_ae.histogram.bands);	
	OS_LeaveRegion();
	
	return 0;
}

// xm_arkn141_isp_set_sharpening_value
// 	设置锐化程度
//	sharpening_value
// 	0 轻微
// 	1 柔和
//		2 强烈
// 
//	返回值
//		0	成功
//		-1	失败
int xm_arkn141_isp_set_sharpening_value (int sharpening_value)
{
	unsigned int data0;
	if(sharpening_value < 0 || sharpening_value > 2)
		return -1;
	
	XM_lock ();
	if(sharpening_value == 0)
	{
		p_enhance.sharp.strength = 32;		// 轻微
	}
	else if(sharpening_value == 1)
	{
		p_enhance.sharp.strength = 64;		// 柔和
	}
	else if(sharpening_value == 2)
	{
		p_enhance.sharp.strength = 160;	// 强烈
	}
	
  	data0 = ((p_enhance.sharp.enable   &  0x01) <<  0) 
	  		| ((p_enhance.sharp.mode     &  0x01) <<  1) 
			| ((p_enhance.sharp.coring   &  0x07) <<  5) 
			| ((p_enhance.sharp.strength &  0xFF) <<  8) 
			| ((p_enhance.sharp.gainmax  & 0x3FF) << 16)
			;
	Gem_write ((GEM_ENHANCE_BASE+0x00), data0);
	XM_unlock ();
	
	return 0;
}

// 使能/禁止flicker
extern int isp_cmos_set_flicker_freq (cmos_exposure_ptr_t p_exp, u8_t flicker_freq);

// 设置光源频率
// flicker_freq	
//			0			禁止光源
//			50			50hz光源
//			60			60hz光源
//			其他值	无效
//	返回值
//		0	成功
//		-1	失败
int xm_arkn141_isp_set_flicker_freq  (int flicker_freq)
{
	return isp_cmos_set_flicker_freq (&isp_exposure, flicker_freq);
}