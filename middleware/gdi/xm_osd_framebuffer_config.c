//****************************************************************************
//
//	Copyright (C) 2012~2014 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_osd_framebuffere_config.c
//
//	Revision history
//
//		2014.02.27	initial version，适应ARK1960 LCDC控制器
//
//****************************************************************************
// OSD产品配置，设置外部OSD所需的参数

// LCD通道配置(使用RGB屏)
#include <xm_proj_define.h>
#include <string.h>
#include <stdlib.h>
#include "xm_type.h"
#include "xm_rom.h"
#include "hw_osd_layer.h"
#include "xm_osd_layer.h"
#include "xm_semaphore.h"
#include "xm_queue.h"
#include "xm_osd_framebuffer.h"
#include "xm_printf.h"
#include "xm_dev.h"
#include "xm_user.h"
#include <xm_image.h>
#include "xm_base.h"
#include <xm_video_task.h>

//#define	HDMI_720P			1
//#define	SRGB_320X240		1
//#define	RGB_480X272			1

#define	LCD_320X240			1
#define	LCD_800X480			2
#define	LCD_640X480			3
#define	LCD_480X320			4
#define	LCD_960X240			5
#define	LCD_480X272			6
#define	LCD_720X576			7		// CVBS分辨率
#define	LCD_1280X720		8
#define	LCD_1920X1080		9
#define	LCD_848X480			10		
#define LCD_1600X400        11
#define LCD_1024X600        12


// LCD_SIZE 定义的是系统最大可以支持的分辨率
//	   如果 LCD_SIZE 定义为 LCD_1280X720, 则输出可以支持720P输出及以下分辨率
//    如果 LCD_SIZE 定义为 LCD_320X240, 则输出无法支持CVBS输出
// 1) 实例1 (RGB屏320X240, CVBS输出)
//     LCD_SIZE 需要支持 
#define LCD_SIZE                   LCD_1024X600//LCD_848X480

#if SRGB_320X240
#define	LCD_SIZE					LCD_320X240
#endif

//#define	LCD_SIZE				LCD_960X240
//#define	LCD_SIZE				LCD_800X480
//#define	LCD_SIZE				LCD_640X480
//#define	LCD_SIZE				LCD_480X320
#if RGB_480X272
#define	LCD_SIZE					LCD_480X272
#endif
//#define	LCD_SIZE				LCD_1920X1080

#if HDMI_720P
#define	LCD_SIZE					LCD_1280X720
#endif

//#define	LCD_SIZE				LCD_720X576	

#if LCD_SIZE == LCD_320X240

#define	CH0_OSD_YUV_WIDTH		320
#define	CH0_OSD_YUV_HEIGHT	240
#define	CH0_OSD_RGB_WIDTH		320
#define	CH0_OSD_RGB_HEIGHT	240

#elif LCD_SIZE == LCD_960X240

#define	CH0_OSD_YUV_WIDTH		960
#define	CH0_OSD_YUV_HEIGHT	240
#define	CH0_OSD_RGB_WIDTH		960
#define	CH0_OSD_RGB_HEIGHT	240

#elif LCD_SIZE == LCD_480X272

#define	CH0_OSD_YUV_WIDTH		480
#define	CH0_OSD_YUV_HEIGHT	272
#define	CH0_OSD_RGB_WIDTH		480
#define	CH0_OSD_RGB_HEIGHT	272

#elif LCD_SIZE == LCD_480X320

#define	CH0_OSD_YUV_WIDTH		480
#define	CH0_OSD_YUV_HEIGHT	320
#define	CH0_OSD_RGB_WIDTH		480
#define	CH0_OSD_RGB_HEIGHT	320

#elif LCD_SIZE == LCD_640X480

#define	CH0_OSD_YUV_WIDTH		640
#define	CH0_OSD_YUV_HEIGHT	480
#define	CH0_OSD_RGB_WIDTH		640
#define	CH0_OSD_RGB_HEIGHT	480

#elif LCD_SIZE == LCD_720X576

#define	CH0_OSD_YUV_WIDTH		720
#define	CH0_OSD_YUV_HEIGHT	576
#define	CH0_OSD_RGB_WIDTH		720
#define	CH0_OSD_RGB_HEIGHT	576

#elif LCD_SIZE == LCD_800X480

// LCDC接口0 OSD层尺寸定义(小于或等于LCD的尺寸)
#define	CH0_OSD_YUV_WIDTH		800
#define	CH0_OSD_YUV_HEIGHT	480
#define	CH0_OSD_RGB_WIDTH		800
#define	CH0_OSD_RGB_HEIGHT	480

#elif LCD_SIZE == LCD_848X480

// LCDC接口0 OSD层尺寸定义(小于或等于LCD的尺寸)
#define	CH0_OSD_YUV_WIDTH		848
#define	CH0_OSD_YUV_HEIGHT	576		// PAL
#define	CH0_OSD_RGB_WIDTH		848
#define	CH0_OSD_RGB_HEIGHT	576

#elif LCD_SIZE == LCD_1600X400

// LCDC接口0 OSD层尺寸定义(小于或等于LCD的尺寸)

#define	CH0_OSD_YUV_WIDTH		1600
#define	CH0_OSD_YUV_HEIGHT	400		
#define	CH0_OSD_RGB_WIDTH		848
#define	CH0_OSD_RGB_HEIGHT	400

#elif LCD_SIZE == LCD_1920X1080			// HDMI 1080i/1080p分辨率 支持

// LCDC接口0 OSD层尺寸定义(小于或等于LCD的尺寸)
#define	CH0_OSD_YUV_WIDTH		1920
#define	CH0_OSD_YUV_HEIGHT	1080
#define	CH0_OSD_RGB_WIDTH		960
#define	CH0_OSD_RGB_HEIGHT	640

#elif LCD_SIZE == LCD_1280X720			// HDMI 720i/720p分辨率 支持

// LCDC接口0 OSD层尺寸定义(小于或等于LCD的尺寸)
#define	CH0_OSD_YUV_WIDTH		1280
#define	CH0_OSD_YUV_HEIGHT	720
#define	CH0_OSD_RGB_WIDTH		720
#define	CH0_OSD_RGB_HEIGHT	480

#elif LCD_SIZE == LCD_1024X600
// LCDC接口0 OSD层尺寸定义(小于或等于LCD的尺寸)
#define	CH0_OSD_YUV_WIDTH		1024
#define	CH0_OSD_YUV_HEIGHT	600		
#define	CH0_OSD_RGB_WIDTH		1024
#define	CH0_OSD_RGB_HEIGHT	600
#else

#define	CH0_OSD_YUV_WIDTH		320
#define	CH0_OSD_YUV_HEIGHT	240
#define	CH0_OSD_RGB_WIDTH		320
#define	CH0_OSD_RGB_HEIGHT	240

//#error please define LCD_SIZE

#endif




// 视频输出接口定义 (模拟三种不同显示分辨率, RGB屏、AVOUT及HDMI)
static const XM_LCDC_CHANNEL_CONFIG lcdc_channel_config[] = {

	// LCDC 视频输出通道 0 配置 0 (320X240 RGB屏)
	
#if HDMI_720P
	
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_HDMI_720p,				// LCDC视频输出通道接口类型
		1280,	 720,						// LCDC视频输出通道像素宽度、高度
		XM_RGB(0, 0, 0),				// LCDC视频输出通道背景色(黑色，晚上)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  1280, 720 },			// OSD 0
			{  480,  320 },			// OSD 1
			{  480,  320 }				// OSD 2
		}
	},	
#elif SRGB_320X240
	// SRGB 320x240
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_SRGB,				// LCDC视频输出通道接口类型
		320,	 240,						// LCDC视频输出通道像素宽度、高度
		XM_RGB(0, 0, 0),				// LCDC视频输出通道背景色(黑色，晚上)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  320,  240 },			// OSD 0
			{  320,  240 },			// OSD 1
			{  320,  240 }				// OSD 2
		}
	},
	
#elif RGB_480X272	// LCD 480X272
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_RGB,				// LCDC视频输出通道接口类型
	//	480,	 320,						// LCDC视频输出通道像素宽度、高度
	//	320,	 240,						// LCDC视频输出通道像素宽度、高度
	//	800,	 480,						// LCDC视频输出通道像素宽度、高度
		480,	 272,						// LCDC视频输出通道像素宽度、高度
		XM_RGB(0, 0, 0),				// LCDC视频输出通道背景色(黑色，晚上)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
		//	{  640,  400 },			// OSD 0
		//	{  480,  320 },			// OSD 0
			{  480,  272 },			// OSD 0
		//	{  480,  320 },			// OSD 1
		//	{  480,  320 },			// OSD 2
		//	{  320,  240 },			// OSD 0
			{  320,  240 },			// OSD 1
			{  320,  240 }				// OSD 2
		}
	},
#elif RGB_800X480	// LCD 800x480
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_RGB,				// LCDC视频输出通道接口类型
		800,	 480,						// LCDC视频输出通道像素宽度、高度
		XM_RGB(0, 0, 0),				// LCDC视频输出通道背景色(黑色，晚上)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  800,  480 },			// OSD 0
			{  800,  480 },			// OSD 1
			{  800,  480 }				// OSD 2
		}
	},
#elif RGB_1024X600	// LCD 1024x600
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_RGB,				// LCDC视频输出通道接口类型
		1024,	 600,						// LCDC视频输出通道像素宽度、高度
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  1024, 600},			// OSD 0
			{  1024, 600},			// OSD 1
			{  1024, 600}			// OSD 2
		}
	},
#elif RGB_848X480	// LCD 848x480
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_RGB,				// LCDC视频输出通道接口类型
		848,	 480,						// LCDC视频输出通道像素宽度、高度
		//XM_RGB(255, 255, 255),				// LCDC视频输出通道背景色(黑色，晚上)
		//XM_RGB(255, 255, 255),		// LCDC视频输出通道背景色(白色，白天)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			//{  720,  448 },			// OSD 0
			//{  640,  480 },			// OSD 1
			//{  640,  480 }			// OSD 2
			{  848,  480 },			// OSD 0
			{  848,  480 },	//448		// OSD 1
			{  848,  480 }				// OSD 2
		}
	},	

#elif RGB_1600X400	// LCD 1600x400
	{
		XM_LCDC_CHANNEL_0,		// LCDC视频输出通道号
		XM_LCDC_TYPE_RGB,		// LCDC视频输出通道接口类型
		1600,	400,			// LCDC视频输出通道像素宽度、高度
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  1600, 400 },		// OSD 0
			{  600,  400 },		// OSD 1
			{  600,  400 }			// OSD 2
		}
	},
	
#else

	// SRGB 320x240
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_SRGB,				// LCDC视频输出通道接口类型
		320,	 240,						// LCDC视频输出通道像素宽度、高度
		XM_RGB(0, 0, 0),				// LCDC视频输出通道背景色(黑色，晚上)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  320,  240 },			// OSD 0
			{  320,  240 },			// OSD 1
			{  320,  240 }				// OSD 2
		}
	},
	
#endif

	// LCDC 视频输出通道 0 配置 1 (AVOUT接口 NTSC)
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_CVBS_NTSC,		// LCDC视频输出通道接口类型	
		720,	480,						// LCDC视频输出通道像素宽度、高度, 
											//		此处NTSC的宽度为720
		XM_RGB(0, 0, 0),				// LCDC视频输出通道背景色(黑色，晚上)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  720,  480 },			// OSD 0
			{  480,  320 },			// OSD 1
			{  480,  320 }				// OSD 2
		}

	},

	// LCDC 视频输出通道 0 配置 2 (AVOUT接口 PAL)
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_CVBS_PAL,		// LCDC视频输出通道接口类型	
		720,	576,						// LCDC视频输出通道像素宽度、高度, 
											//		此处NTSC的宽度为720
		XM_RGB(0, 0, 0),				// LCDC视频输出通道背景色(黑色，晚上)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  720,  576 },			// OSD 0
			{  640,  480 },			// OSD 1
			{  640,  480 }				// OSD 2
		}
	},

	// LCDC 视频输出通道 0 配置 3 (AVOUT接口 YPbPr_480i )
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_YPbPr_480i,		// LCDC视频输出通道接口类型	
		720,	480,						// LCDC视频输出通道像素宽度、高度, 
											//		此处NTSC的宽度为720
		XM_RGB(0, 0, 0),				// LCDC视频输出通道背景色(黑色，晚上)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  640,  480 },			// OSD 0
			{  480,  320 },			// OSD 1
			{  480,  320 }				// OSD 2
		}
	},

	// LCDC 视频输出通道 0 配置 4 (AVOUT接口 YPbPr_480p )
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_YPbPr_480p,		// LCDC视频输出通道接口类型	
		720,	480,						// LCDC视频输出通道像素宽度、高度, 
											//		此处NTSC的宽度为720
		XM_RGB(0, 0, 0),				// LCDC视频输出通道背景色(黑色，晚上)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  640,  480 },			// OSD 0
			{  480,  320 },			// OSD 1
			{  480,  320 }				// OSD 2
		}
	},

	// LCDC 视频输出通道 0 配置 5 (AVOUT接口 YPbPr_576i )
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_YPbPr_576i,		// LCDC视频输出通道接口类型	
		720,	576,						// LCDC视频输出通道像素宽度、高度, 
											//		此处NTSC的宽度为720
		XM_RGB(0, 0, 0),				// LCDC视频输出通道背景色(黑色，晚上)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  640,  480 },			// OSD 0
			{  480,  320 },			// OSD 1
			{  480,  320 }				// OSD 2
		}
	},

	// LCDC 视频输出通道 0 配置 6 (AVOUT接口 YPbPr_576p )
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_YPbPr_576p,		// LCDC视频输出通道接口类型	
		720,	576,						// LCDC视频输出通道像素宽度、高度, 
											//		此处NTSC的宽度为720
		XM_RGB(0, 0, 0),				// LCDC视频输出通道背景色(黑色，晚上)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  640,  480 },			// OSD 0
			{  480,  320 },			// OSD 1
			{  480,  320 }				// OSD 2
		}
	},

	// LCDC 视频输出通道 0 配置 7 (HDMI接口 1080i)
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_HDMI_1080i,	// LCDC视频输出通道接口类型	
		1920,	1080,						// LCDC视频输出通道像素宽度、高度
	
		XM_RGB(0, 0, 0),				// LCDC视频输出通道背景色(黑色，晚上)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  1920, 1080 },			// OSD 0
			{  960,  640  },			// OSD 1
			{  960,  640  }			// OSD 2
		}
	},

	// LCDC 视频输出通道 0 配置 8 (HDMI接口  1080p)
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_HDMI_1080p,	// LCDC视频输出通道接口类型	
		1920,	1080,						// LCDC视频输出通道像素宽度、高度

		XM_RGB(0, 0, 0),				// LCDC视频输出通道背景色(黑色，晚上)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  1920, 1080 },			// OSD 0
			{  960,  640  },			// OSD 1
			{  960,  640  }			// OSD 2
		}
	},

	// LCDC 视频输出通道 0 配置 9 (HDMI接口 720i)
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_HDMI_720i,		// LCDC视频输出通道接口类型	
		1280,	720,						// LCDC视频输出通道像素宽度、高度
	
		XM_RGB(0, 0, 0),				// LCDC视频输出通道背景色(黑色，晚上)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  1280, 720 },			// OSD 0
			{  960,  640  },			// OSD 1
			{  960,  640  }			// OSD 2
		}
	},

	// LCDC 视频输出通道 0 配置 10 (HDMI接口 720p)
	{
		XM_LCDC_CHANNEL_0,			// LCDC视频输出通道号
		XM_LCDC_TYPE_HDMI_720p,		// LCDC视频输出通道接口类型	
		1280,	720,						// LCDC视频输出通道像素宽度、高度
	
		XM_RGB(0, 0, 0),				// LCDC视频输出通道背景色(黑色，晚上)
		XM_RGB(0, 0, 0),		// LCDC视频输出通道背景色(白色，白天)

		// OSD层分辨率设置
		{
			{  1280, 720 },			// OSD 0
			{  960,  640  },			// OSD 1
			{  960,  640  }			// OSD 2
		}
	}

};


// 视频通道0 framebuffer对象的视频数据缓冲区定义
#ifdef __ICCARM__
#pragma data_alignment=128 
#endif
// 定义YUV格式的视频数据缓冲区
__no_init static unsigned char ch0_osd_yuv_buffer[OSD_YUV_FRAMEBUFFER_COUNT][CH0_OSD_YUV_WIDTH * CH0_OSD_YUV_HEIGHT * 3/2];
#ifdef __ICCARM__
#pragma data_alignment=128 
#endif
// 定义ARGB8888格式的UI数据缓冲区(OSD1层及OSD2共用)
__no_init static unsigned char ch0_osd_rgb_buffer[OSD_RGB_FRAMEBUFFER_COUNT][CH0_OSD_RGB_WIDTH * CH0_OSD_RGB_HEIGHT * 4];


void XM_osd_framebuffer_config_init (void)
{
	int i;
	unsigned int lcdc_channel;		// 视频输出通道序号
	xm_osd_framebuffer_t yuv_framebuffer;
	int yuv_framebuffer_count, rgb_framebuffer_count;


	int lcd_config_index;

	// *** 设置“视频输出通道”OSD的framebuffer缓冲区 ***
	
	// 1) YUV格式的framebuffer缓冲区基址配置
	// YUV格式framebuffer, 用于OSD Layer 0层的YUV视频输出
	//	获取系统定义的YUV格式的framebuffer对象个数(用于双缓冲区(pingpong buffer)操作)
	yuv_framebuffer_count = XM_osd_framebuffer_get_framebuffer_count (XM_LCDC_CHANNEL_0, XM_OSD_FRAMEBUFFER_TYPE_YUV);
	// 设置每个YUV格式的framebuffer对象的缓冲区基址
	for (i = 0; i < yuv_framebuffer_count; i ++)
	{
		// 缓冲区地址一旦设置，不能修改
		XM_osd_framebuffer_set_framebuffer_address (
					XM_LCDC_CHANNEL_0,				// 视频输出通道序号
					XM_OSD_FRAMEBUFFER_TYPE_YUV,	// YUV格式
					i,										// 序号
					ch0_osd_yuv_buffer[i]			// 基址
					);
	}

	// 2) RGB格式的framebuffer缓冲区基址配置
	// RGB格式framebuffer，用于OSD Layer 1、OSD Layer 2层的RGB视频输出
	//	获取系统定义的RGB格式的framebuffer对象个数(用于双缓冲区(pingpong buffer)操作)
	rgb_framebuffer_count = XM_osd_framebuffer_get_framebuffer_count (XM_LCDC_CHANNEL_0, XM_OSD_FRAMEBUFFER_TYPE_RGB);
	// 设置每个RGB格式的framebuffer对象的缓冲区基址
	for (i = 0; i < rgb_framebuffer_count; i ++)
	{
		// 缓冲区地址一旦设置，不能修改
		XM_osd_framebuffer_set_framebuffer_address (
					XM_LCDC_CHANNEL_0,				// 视频输出通道序号
					XM_OSD_FRAMEBUFFER_TYPE_RGB,	// RGB格式
					i,										// 序号
					ch0_osd_rgb_buffer[i]			// 基址
					);
	}

	// 初始化LCDC视频输出接口 0 (此处配置并使用LCDC输出通道0)
	
	// 视频输出通道设置(通道号、输出接口类型、视频尺寸)

	// 检测HDMI外设是否接入
	if(XM_GetFmlDeviceCap(DEVCAP_HDMI))
	{
		// 读取系统定义的HDMI外设输出类型
		int hdmi_type = XM_GetFmlDeviceCap (DEVCAP_HDMI_TYPE);
		if( hdmi_type == XM_LCDC_TYPE_HDMI_1080i )
		{
			lcd_config_index = 7;
		}
		else if(hdmi_type == XM_LCDC_TYPE_HDMI_1080p )
		{
			lcd_config_index = 8;
		}
		else if( hdmi_type == XM_LCDC_TYPE_HDMI_720i )
		{
			lcd_config_index = 9;
		}
		else if(hdmi_type == XM_LCDC_TYPE_HDMI_720p )
		{
			lcd_config_index = 10;
		}
		else
		{
			lcd_config_index = 10;
		}
	}
	// 检测AVOUT外设是否接入
	else if(XM_GetFmlDeviceCap(DEVCAP_AVOUT))
	{
		// AVOUT插入
		int avout_type = XM_GetFmlDeviceCap (DEVCAP_AVOUT_TYPE);
		if(avout_type == XM_LCDC_TYPE_CVBS_NTSC)
			lcd_config_index = 1;
		else if(avout_type == XM_LCDC_TYPE_CVBS_PAL)
			lcd_config_index = 2;
		else if(avout_type == XM_LCDC_TYPE_YPbPr_480i)
			lcd_config_index = 3;
		else if(avout_type == XM_LCDC_TYPE_YPbPr_480p)
			lcd_config_index = 4;
		else if(avout_type == XM_LCDC_TYPE_YPbPr_576i)
			lcd_config_index = 5;
		else if(avout_type == XM_LCDC_TYPE_YPbPr_576p)
			lcd_config_index = 6;
		else
			lcd_config_index = 1;
	}
	else
	{
		// LCD屏设置
		lcd_config_index = 0;
	}

	// 检查OSD 0的尺寸设置是否超出YUV缓冲区的最大尺寸定义
	if(lcdc_channel_config[lcd_config_index].lcdc_osd_size[XM_OSD_LAYER_0].cx > CH0_OSD_YUV_WIDTH)
	{
		XM_printf ("ERROR, OSD 0's  (width=%d) great than the (width = %d) of framebuffer\n", 
				lcdc_channel_config[lcd_config_index].lcdc_osd_size[XM_OSD_LAYER_0].cx, CH0_OSD_YUV_WIDTH);
		return;
	}
	if(lcdc_channel_config[lcd_config_index].lcdc_osd_size[XM_OSD_LAYER_0].cy > CH0_OSD_YUV_HEIGHT)
	{
		XM_printf ("ERROR, OSD 0's  (width=%d) great than the (width = %d) of framebuffer\n", 
				lcdc_channel_config[lcd_config_index].lcdc_osd_size[XM_OSD_LAYER_0].cy, CH0_OSD_YUV_HEIGHT);
		return;
	}


	lcdc_channel = lcdc_channel_config[lcd_config_index].lcdc_channel;
	
	// 配置视频输出通道的类型、通道尺寸
	HW_lcdc_init (
			lcdc_channel,		// lcdc输出通道号
			lcdc_channel_config[lcd_config_index].lcdc_type,		// lcdc输出通道类型(RGB/CPU/VGA/CVBS/BT601/YPbPr等)		
			lcdc_channel_config[lcd_config_index].lcdc_width,		// LCD输出通道宽度
			lcdc_channel_config[lcd_config_index].lcdc_height		// LCD输出通道高度
			);

	// 视频输出通道背景色设置
	{
		HW_lcdc_set_background_color (
				lcdc_channel,				// lcd_channel
				XM_GetRValue(lcdc_channel_config[lcd_config_index].lcdc_background_sun_color),		// r
				XM_GetGValue(lcdc_channel_config[lcd_config_index].lcdc_background_sun_color),		// g	
				XM_GetBValue(lcdc_channel_config[lcd_config_index].lcdc_background_sun_color)		// b
				);
	}

	// 视频输出通道显示开启
	if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_CONNECT)
		HW_lcdc_set_display_on (lcdc_channel, 1);

	// 视频输出通道背光开?

	if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_CONNECT)
		HW_lcdc_set_osd_on (lcdc_channel, 1);
     

	// 初始化OSD层的分辨率设置

	// 检查OSD 1 与 OSD 2的分辨率设置是否一致
	if(	lcdc_channel_config[lcd_config_index].lcdc_osd_size[XM_OSD_LAYER_1].cx 
		!= lcdc_channel_config[lcd_config_index].lcdc_osd_size[XM_OSD_LAYER_2].cx )
	{
		XM_printf ("ERROR, OSD 1's width (%d) != OSD 2's width (%d)\n", 
				lcdc_channel_config[lcd_config_index].lcdc_osd_size[XM_OSD_LAYER_1].cx,
				lcdc_channel_config[lcd_config_index].lcdc_osd_size[XM_OSD_LAYER_2].cx);
	}
	if(	lcdc_channel_config[lcd_config_index].lcdc_osd_size[XM_OSD_LAYER_1].cy 
		!= lcdc_channel_config[lcd_config_index].lcdc_osd_size[XM_OSD_LAYER_2].cy )
	{
		XM_printf ("ERROR, OSD 1's height (%d) != OSD 2's height (%d)\n", 
				lcdc_channel_config[lcd_config_index].lcdc_osd_size[XM_OSD_LAYER_1].cy,
				lcdc_channel_config[lcd_config_index].lcdc_osd_size[XM_OSD_LAYER_2].cy);
	}

	// 根据显示配置信息(lcdc_channel_config), 执行OSD层分辨率初始化
	for (i = 0; i < XM_OSD_LAYER_COUNT; i ++)
	{
		// OSD层分辨率设置
		unsigned int osd_cx = lcdc_channel_config[lcd_config_index].lcdc_osd_size[i].cx;
		unsigned int osd_cy = lcdc_channel_config[lcd_config_index].lcdc_osd_size[i].cy;

		// 检查OSD层的定义是否超出已分配的framebuffer的最大支持尺寸
		if(i == 0)
		{
			// YUV层
			if(osd_cx > CH0_OSD_YUV_WIDTH)
			{
				XM_printf ("ERROR, OSD %d's  (width=%d) great than the (width = %d) of framebuffer\n", 
					i, osd_cx, CH0_OSD_YUV_WIDTH);
				// 尺寸约束
				osd_cx = CH0_OSD_YUV_WIDTH;
			}
			if(osd_cy > CH0_OSD_YUV_HEIGHT)
			{
				XM_printf ("ERROR, OSD %d's  (height=%d) great than the (height = %d) of framebuffer\n", 
					i, osd_cy, CH0_OSD_YUV_HEIGHT);
				// 尺寸约束
				osd_cy = CH0_OSD_YUV_HEIGHT;
			}
		}
		else
		{
			// RGB层
			if(osd_cx > CH0_OSD_RGB_WIDTH)
			{
				XM_printf ("ERROR, OSD %d's  (width=%d) great than the (width = %d) of framebuffer\n", 
					i, osd_cx, CH0_OSD_RGB_WIDTH);
				// 尺寸约束
				osd_cx = CH0_OSD_RGB_WIDTH;
			}
			if(osd_cy > CH0_OSD_RGB_HEIGHT)
			{
				XM_printf ("ERROR, OSD %d's  (height=%d) great than the (height = %d) of framebuffer\n", 
					i, osd_cy, CH0_OSD_RGB_HEIGHT);
				// 尺寸约束
				osd_cy = CH0_OSD_RGB_HEIGHT;
			}
		}

		// 检查是否大于LCDC通道的物理分辨率设置
		if(osd_cx > lcdc_channel_config[lcd_config_index].lcdc_width)
		{
			XM_printf ("ERROR, OSD %d's width (%d) great than the width (%d) of LCDC\n", 
				i, osd_cx, lcdc_channel_config[lcd_config_index].lcdc_width);
			// 尺寸约束
			osd_cx = lcdc_channel_config[lcd_config_index].lcdc_width;
		}
		if(osd_cy > lcdc_channel_config[lcd_config_index].lcdc_height)
		{
			XM_printf ("ERROR, OSD %d's height (%d) great than the height (%d) of LCDC\n", 
				i, osd_cy, lcdc_channel_config[lcd_config_index].lcdc_height);
			// 尺寸约束
			osd_cy = lcdc_channel_config[lcd_config_index].lcdc_height;
		}

		// 对齐约束
		osd_cx = XM_lcdc_osd_horz_align (lcdc_channel, osd_cx);
		osd_cy = XM_lcdc_osd_vert_align (lcdc_channel, osd_cy);

		XM_lcdc_osd_set_width  (lcdc_channel, XM_OSD_LAYER_0 + i, osd_cx);
		XM_lcdc_osd_set_height (lcdc_channel, XM_OSD_LAYER_0 + i, osd_cy);
	}


	// 视频任务初始化
	//XMSYS_VideoInit ();

	// 标记OSD层已初始化完毕, 可以创建或打开framebuffer资源
	XM_SetFmlDeviceCap (DEVCAP_OSD, 1);
}

void XM_osd_framebuffer_config_exit (void)
{
	int lcd_config_index;
	unsigned int lcdc_channel;		// 视频输出通道序号


	// 视频任务初始化
	//XMSYS_VideoExit ();


	// 视频输出通道设置(通道号、输出接口类型、视频尺寸)
	lcd_config_index = 0;
	lcdc_channel = lcdc_channel_config[lcd_config_index].lcdc_channel;


	// 视频输出通道背光关闭
	HW_lcdc_set_backlight_on (lcdc_channel, 0);

	// 视频输出通道显示关闭
	HW_lcdc_set_display_on (lcdc_channel, 0);

	// 视频输出通道逻辑关闭(关闭时钟)
	HW_lcdc_exit (lcdc_channel);

	memset (ch0_osd_yuv_buffer, 0, sizeof(ch0_osd_yuv_buffer));
	memset (ch0_osd_rgb_buffer, 0, sizeof(ch0_osd_rgb_buffer));

}
