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
//		2014.02.27	initial version����ӦARK1960 LCDC������
//
//****************************************************************************
// OSD��Ʒ���ã������ⲿOSD����Ĳ���

// LCDͨ������(ʹ��RGB��)
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
#define	LCD_720X576			7		// CVBS�ֱ���
#define	LCD_1280X720		8
#define	LCD_1920X1080		9
#define	LCD_848X480			10		
#define LCD_1600X400        11
#define LCD_1024X600        12


// LCD_SIZE �������ϵͳ������֧�ֵķֱ���
//	   ��� LCD_SIZE ����Ϊ LCD_1280X720, ���������֧��720P��������·ֱ���
//    ��� LCD_SIZE ����Ϊ LCD_320X240, ������޷�֧��CVBS���
// 1) ʵ��1 (RGB��320X240, CVBS���)
//     LCD_SIZE ��Ҫ֧�� 
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

// LCDC�ӿ�0 OSD��ߴ綨��(С�ڻ����LCD�ĳߴ�)
#define	CH0_OSD_YUV_WIDTH		800
#define	CH0_OSD_YUV_HEIGHT	480
#define	CH0_OSD_RGB_WIDTH		800
#define	CH0_OSD_RGB_HEIGHT	480

#elif LCD_SIZE == LCD_848X480

// LCDC�ӿ�0 OSD��ߴ綨��(С�ڻ����LCD�ĳߴ�)
#define	CH0_OSD_YUV_WIDTH		848
#define	CH0_OSD_YUV_HEIGHT	576		// PAL
#define	CH0_OSD_RGB_WIDTH		848
#define	CH0_OSD_RGB_HEIGHT	576

#elif LCD_SIZE == LCD_1600X400

// LCDC�ӿ�0 OSD��ߴ綨��(С�ڻ����LCD�ĳߴ�)

#define	CH0_OSD_YUV_WIDTH		1600
#define	CH0_OSD_YUV_HEIGHT	400		
#define	CH0_OSD_RGB_WIDTH		848
#define	CH0_OSD_RGB_HEIGHT	400

#elif LCD_SIZE == LCD_1920X1080			// HDMI 1080i/1080p�ֱ��� ֧��

// LCDC�ӿ�0 OSD��ߴ綨��(С�ڻ����LCD�ĳߴ�)
#define	CH0_OSD_YUV_WIDTH		1920
#define	CH0_OSD_YUV_HEIGHT	1080
#define	CH0_OSD_RGB_WIDTH		960
#define	CH0_OSD_RGB_HEIGHT	640

#elif LCD_SIZE == LCD_1280X720			// HDMI 720i/720p�ֱ��� ֧��

// LCDC�ӿ�0 OSD��ߴ綨��(С�ڻ����LCD�ĳߴ�)
#define	CH0_OSD_YUV_WIDTH		1280
#define	CH0_OSD_YUV_HEIGHT	720
#define	CH0_OSD_RGB_WIDTH		720
#define	CH0_OSD_RGB_HEIGHT	480

#elif LCD_SIZE == LCD_1024X600
// LCDC�ӿ�0 OSD��ߴ綨��(С�ڻ����LCD�ĳߴ�)
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




// ��Ƶ����ӿڶ��� (ģ�����ֲ�ͬ��ʾ�ֱ���, RGB����AVOUT��HDMI)
static const XM_LCDC_CHANNEL_CONFIG lcdc_channel_config[] = {

	// LCDC ��Ƶ���ͨ�� 0 ���� 0 (320X240 RGB��)
	
#if HDMI_720P
	
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_HDMI_720p,				// LCDC��Ƶ���ͨ���ӿ�����
		1280,	 720,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�
		XM_RGB(0, 0, 0),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  1280, 720 },			// OSD 0
			{  480,  320 },			// OSD 1
			{  480,  320 }				// OSD 2
		}
	},	
#elif SRGB_320X240
	// SRGB 320x240
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_SRGB,				// LCDC��Ƶ���ͨ���ӿ�����
		320,	 240,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�
		XM_RGB(0, 0, 0),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  320,  240 },			// OSD 0
			{  320,  240 },			// OSD 1
			{  320,  240 }				// OSD 2
		}
	},
	
#elif RGB_480X272	// LCD 480X272
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_RGB,				// LCDC��Ƶ���ͨ���ӿ�����
	//	480,	 320,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�
	//	320,	 240,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�
	//	800,	 480,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�
		480,	 272,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�
		XM_RGB(0, 0, 0),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
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
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_RGB,				// LCDC��Ƶ���ͨ���ӿ�����
		800,	 480,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�
		XM_RGB(0, 0, 0),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  800,  480 },			// OSD 0
			{  800,  480 },			// OSD 1
			{  800,  480 }				// OSD 2
		}
	},
#elif RGB_1024X600	// LCD 1024x600
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_RGB,				// LCDC��Ƶ���ͨ���ӿ�����
		1024,	 600,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  1024, 600},			// OSD 0
			{  1024, 600},			// OSD 1
			{  1024, 600}			// OSD 2
		}
	},
#elif RGB_848X480	// LCD 848x480
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_RGB,				// LCDC��Ƶ���ͨ���ӿ�����
		848,	 480,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�
		//XM_RGB(255, 255, 255),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		//XM_RGB(255, 255, 255),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
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
		XM_LCDC_CHANNEL_0,		// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_RGB,		// LCDC��Ƶ���ͨ���ӿ�����
		1600,	400,			// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  1600, 400 },		// OSD 0
			{  600,  400 },		// OSD 1
			{  600,  400 }			// OSD 2
		}
	},
	
#else

	// SRGB 320x240
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_SRGB,				// LCDC��Ƶ���ͨ���ӿ�����
		320,	 240,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�
		XM_RGB(0, 0, 0),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  320,  240 },			// OSD 0
			{  320,  240 },			// OSD 1
			{  320,  240 }				// OSD 2
		}
	},
	
#endif

	// LCDC ��Ƶ���ͨ�� 0 ���� 1 (AVOUT�ӿ� NTSC)
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_CVBS_NTSC,		// LCDC��Ƶ���ͨ���ӿ�����	
		720,	480,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�, 
											//		�˴�NTSC�Ŀ��Ϊ720
		XM_RGB(0, 0, 0),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  720,  480 },			// OSD 0
			{  480,  320 },			// OSD 1
			{  480,  320 }				// OSD 2
		}

	},

	// LCDC ��Ƶ���ͨ�� 0 ���� 2 (AVOUT�ӿ� PAL)
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_CVBS_PAL,		// LCDC��Ƶ���ͨ���ӿ�����	
		720,	576,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�, 
											//		�˴�NTSC�Ŀ��Ϊ720
		XM_RGB(0, 0, 0),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  720,  576 },			// OSD 0
			{  640,  480 },			// OSD 1
			{  640,  480 }				// OSD 2
		}
	},

	// LCDC ��Ƶ���ͨ�� 0 ���� 3 (AVOUT�ӿ� YPbPr_480i )
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_YPbPr_480i,		// LCDC��Ƶ���ͨ���ӿ�����	
		720,	480,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�, 
											//		�˴�NTSC�Ŀ��Ϊ720
		XM_RGB(0, 0, 0),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  640,  480 },			// OSD 0
			{  480,  320 },			// OSD 1
			{  480,  320 }				// OSD 2
		}
	},

	// LCDC ��Ƶ���ͨ�� 0 ���� 4 (AVOUT�ӿ� YPbPr_480p )
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_YPbPr_480p,		// LCDC��Ƶ���ͨ���ӿ�����	
		720,	480,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�, 
											//		�˴�NTSC�Ŀ��Ϊ720
		XM_RGB(0, 0, 0),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  640,  480 },			// OSD 0
			{  480,  320 },			// OSD 1
			{  480,  320 }				// OSD 2
		}
	},

	// LCDC ��Ƶ���ͨ�� 0 ���� 5 (AVOUT�ӿ� YPbPr_576i )
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_YPbPr_576i,		// LCDC��Ƶ���ͨ���ӿ�����	
		720,	576,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�, 
											//		�˴�NTSC�Ŀ��Ϊ720
		XM_RGB(0, 0, 0),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  640,  480 },			// OSD 0
			{  480,  320 },			// OSD 1
			{  480,  320 }				// OSD 2
		}
	},

	// LCDC ��Ƶ���ͨ�� 0 ���� 6 (AVOUT�ӿ� YPbPr_576p )
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_YPbPr_576p,		// LCDC��Ƶ���ͨ���ӿ�����	
		720,	576,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�, 
											//		�˴�NTSC�Ŀ��Ϊ720
		XM_RGB(0, 0, 0),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  640,  480 },			// OSD 0
			{  480,  320 },			// OSD 1
			{  480,  320 }				// OSD 2
		}
	},

	// LCDC ��Ƶ���ͨ�� 0 ���� 7 (HDMI�ӿ� 1080i)
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_HDMI_1080i,	// LCDC��Ƶ���ͨ���ӿ�����	
		1920,	1080,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�
	
		XM_RGB(0, 0, 0),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  1920, 1080 },			// OSD 0
			{  960,  640  },			// OSD 1
			{  960,  640  }			// OSD 2
		}
	},

	// LCDC ��Ƶ���ͨ�� 0 ���� 8 (HDMI�ӿ�  1080p)
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_HDMI_1080p,	// LCDC��Ƶ���ͨ���ӿ�����	
		1920,	1080,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�

		XM_RGB(0, 0, 0),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  1920, 1080 },			// OSD 0
			{  960,  640  },			// OSD 1
			{  960,  640  }			// OSD 2
		}
	},

	// LCDC ��Ƶ���ͨ�� 0 ���� 9 (HDMI�ӿ� 720i)
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_HDMI_720i,		// LCDC��Ƶ���ͨ���ӿ�����	
		1280,	720,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�
	
		XM_RGB(0, 0, 0),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  1280, 720 },			// OSD 0
			{  960,  640  },			// OSD 1
			{  960,  640  }			// OSD 2
		}
	},

	// LCDC ��Ƶ���ͨ�� 0 ���� 10 (HDMI�ӿ� 720p)
	{
		XM_LCDC_CHANNEL_0,			// LCDC��Ƶ���ͨ����
		XM_LCDC_TYPE_HDMI_720p,		// LCDC��Ƶ���ͨ���ӿ�����	
		1280,	720,						// LCDC��Ƶ���ͨ�����ؿ�ȡ��߶�
	
		XM_RGB(0, 0, 0),				// LCDC��Ƶ���ͨ������ɫ(��ɫ������)
		XM_RGB(0, 0, 0),		// LCDC��Ƶ���ͨ������ɫ(��ɫ������)

		// OSD��ֱ�������
		{
			{  1280, 720 },			// OSD 0
			{  960,  640  },			// OSD 1
			{  960,  640  }			// OSD 2
		}
	}

};


// ��Ƶͨ��0 framebuffer�������Ƶ���ݻ���������
#ifdef __ICCARM__
#pragma data_alignment=128 
#endif
// ����YUV��ʽ����Ƶ���ݻ�����
__no_init static unsigned char ch0_osd_yuv_buffer[OSD_YUV_FRAMEBUFFER_COUNT][CH0_OSD_YUV_WIDTH * CH0_OSD_YUV_HEIGHT * 3/2];
#ifdef __ICCARM__
#pragma data_alignment=128 
#endif
// ����ARGB8888��ʽ��UI���ݻ�����(OSD1�㼰OSD2����)
__no_init static unsigned char ch0_osd_rgb_buffer[OSD_RGB_FRAMEBUFFER_COUNT][CH0_OSD_RGB_WIDTH * CH0_OSD_RGB_HEIGHT * 4];


void XM_osd_framebuffer_config_init (void)
{
	int i;
	unsigned int lcdc_channel;		// ��Ƶ���ͨ�����
	xm_osd_framebuffer_t yuv_framebuffer;
	int yuv_framebuffer_count, rgb_framebuffer_count;


	int lcd_config_index;

	// *** ���á���Ƶ���ͨ����OSD��framebuffer������ ***
	
	// 1) YUV��ʽ��framebuffer��������ַ����
	// YUV��ʽframebuffer, ����OSD Layer 0���YUV��Ƶ���
	//	��ȡϵͳ�����YUV��ʽ��framebuffer�������(����˫������(pingpong buffer)����)
	yuv_framebuffer_count = XM_osd_framebuffer_get_framebuffer_count (XM_LCDC_CHANNEL_0, XM_OSD_FRAMEBUFFER_TYPE_YUV);
	// ����ÿ��YUV��ʽ��framebuffer����Ļ�������ַ
	for (i = 0; i < yuv_framebuffer_count; i ++)
	{
		// ��������ַһ�����ã������޸�
		XM_osd_framebuffer_set_framebuffer_address (
					XM_LCDC_CHANNEL_0,				// ��Ƶ���ͨ�����
					XM_OSD_FRAMEBUFFER_TYPE_YUV,	// YUV��ʽ
					i,										// ���
					ch0_osd_yuv_buffer[i]			// ��ַ
					);
	}

	// 2) RGB��ʽ��framebuffer��������ַ����
	// RGB��ʽframebuffer������OSD Layer 1��OSD Layer 2���RGB��Ƶ���
	//	��ȡϵͳ�����RGB��ʽ��framebuffer�������(����˫������(pingpong buffer)����)
	rgb_framebuffer_count = XM_osd_framebuffer_get_framebuffer_count (XM_LCDC_CHANNEL_0, XM_OSD_FRAMEBUFFER_TYPE_RGB);
	// ����ÿ��RGB��ʽ��framebuffer����Ļ�������ַ
	for (i = 0; i < rgb_framebuffer_count; i ++)
	{
		// ��������ַһ�����ã������޸�
		XM_osd_framebuffer_set_framebuffer_address (
					XM_LCDC_CHANNEL_0,				// ��Ƶ���ͨ�����
					XM_OSD_FRAMEBUFFER_TYPE_RGB,	// RGB��ʽ
					i,										// ���
					ch0_osd_rgb_buffer[i]			// ��ַ
					);
	}

	// ��ʼ��LCDC��Ƶ����ӿ� 0 (�˴����ò�ʹ��LCDC���ͨ��0)
	
	// ��Ƶ���ͨ������(ͨ���š�����ӿ����͡���Ƶ�ߴ�)

	// ���HDMI�����Ƿ����
	if(XM_GetFmlDeviceCap(DEVCAP_HDMI))
	{
		// ��ȡϵͳ�����HDMI�����������
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
	// ���AVOUT�����Ƿ����
	else if(XM_GetFmlDeviceCap(DEVCAP_AVOUT))
	{
		// AVOUT����
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
		// LCD������
		lcd_config_index = 0;
	}

	// ���OSD 0�ĳߴ������Ƿ񳬳�YUV�����������ߴ綨��
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
	
	// ������Ƶ���ͨ�������͡�ͨ���ߴ�
	HW_lcdc_init (
			lcdc_channel,		// lcdc���ͨ����
			lcdc_channel_config[lcd_config_index].lcdc_type,		// lcdc���ͨ������(RGB/CPU/VGA/CVBS/BT601/YPbPr��)		
			lcdc_channel_config[lcd_config_index].lcdc_width,		// LCD���ͨ�����
			lcdc_channel_config[lcd_config_index].lcdc_height		// LCD���ͨ���߶�
			);

	// ��Ƶ���ͨ������ɫ����
	{
		HW_lcdc_set_background_color (
				lcdc_channel,				// lcd_channel
				XM_GetRValue(lcdc_channel_config[lcd_config_index].lcdc_background_sun_color),		// r
				XM_GetGValue(lcdc_channel_config[lcd_config_index].lcdc_background_sun_color),		// g	
				XM_GetBValue(lcdc_channel_config[lcd_config_index].lcdc_background_sun_color)		// b
				);
	}

	// ��Ƶ���ͨ����ʾ����
	if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_CONNECT)
		HW_lcdc_set_display_on (lcdc_channel, 1);

	// ��Ƶ���ͨ�����⿪?

	if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_CONNECT)
		HW_lcdc_set_osd_on (lcdc_channel, 1);
     

	// ��ʼ��OSD��ķֱ�������

	// ���OSD 1 �� OSD 2�ķֱ��������Ƿ�һ��
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

	// ������ʾ������Ϣ(lcdc_channel_config), ִ��OSD��ֱ��ʳ�ʼ��
	for (i = 0; i < XM_OSD_LAYER_COUNT; i ++)
	{
		// OSD��ֱ�������
		unsigned int osd_cx = lcdc_channel_config[lcd_config_index].lcdc_osd_size[i].cx;
		unsigned int osd_cy = lcdc_channel_config[lcd_config_index].lcdc_osd_size[i].cy;

		// ���OSD��Ķ����Ƿ񳬳��ѷ����framebuffer�����֧�ֳߴ�
		if(i == 0)
		{
			// YUV��
			if(osd_cx > CH0_OSD_YUV_WIDTH)
			{
				XM_printf ("ERROR, OSD %d's  (width=%d) great than the (width = %d) of framebuffer\n", 
					i, osd_cx, CH0_OSD_YUV_WIDTH);
				// �ߴ�Լ��
				osd_cx = CH0_OSD_YUV_WIDTH;
			}
			if(osd_cy > CH0_OSD_YUV_HEIGHT)
			{
				XM_printf ("ERROR, OSD %d's  (height=%d) great than the (height = %d) of framebuffer\n", 
					i, osd_cy, CH0_OSD_YUV_HEIGHT);
				// �ߴ�Լ��
				osd_cy = CH0_OSD_YUV_HEIGHT;
			}
		}
		else
		{
			// RGB��
			if(osd_cx > CH0_OSD_RGB_WIDTH)
			{
				XM_printf ("ERROR, OSD %d's  (width=%d) great than the (width = %d) of framebuffer\n", 
					i, osd_cx, CH0_OSD_RGB_WIDTH);
				// �ߴ�Լ��
				osd_cx = CH0_OSD_RGB_WIDTH;
			}
			if(osd_cy > CH0_OSD_RGB_HEIGHT)
			{
				XM_printf ("ERROR, OSD %d's  (height=%d) great than the (height = %d) of framebuffer\n", 
					i, osd_cy, CH0_OSD_RGB_HEIGHT);
				// �ߴ�Լ��
				osd_cy = CH0_OSD_RGB_HEIGHT;
			}
		}

		// ����Ƿ����LCDCͨ��������ֱ�������
		if(osd_cx > lcdc_channel_config[lcd_config_index].lcdc_width)
		{
			XM_printf ("ERROR, OSD %d's width (%d) great than the width (%d) of LCDC\n", 
				i, osd_cx, lcdc_channel_config[lcd_config_index].lcdc_width);
			// �ߴ�Լ��
			osd_cx = lcdc_channel_config[lcd_config_index].lcdc_width;
		}
		if(osd_cy > lcdc_channel_config[lcd_config_index].lcdc_height)
		{
			XM_printf ("ERROR, OSD %d's height (%d) great than the height (%d) of LCDC\n", 
				i, osd_cy, lcdc_channel_config[lcd_config_index].lcdc_height);
			// �ߴ�Լ��
			osd_cy = lcdc_channel_config[lcd_config_index].lcdc_height;
		}

		// ����Լ��
		osd_cx = XM_lcdc_osd_horz_align (lcdc_channel, osd_cx);
		osd_cy = XM_lcdc_osd_vert_align (lcdc_channel, osd_cy);

		XM_lcdc_osd_set_width  (lcdc_channel, XM_OSD_LAYER_0 + i, osd_cx);
		XM_lcdc_osd_set_height (lcdc_channel, XM_OSD_LAYER_0 + i, osd_cy);
	}


	// ��Ƶ�����ʼ��
	//XMSYS_VideoInit ();

	// ���OSD���ѳ�ʼ�����, ���Դ������framebuffer��Դ
	XM_SetFmlDeviceCap (DEVCAP_OSD, 1);
}

void XM_osd_framebuffer_config_exit (void)
{
	int lcd_config_index;
	unsigned int lcdc_channel;		// ��Ƶ���ͨ�����


	// ��Ƶ�����ʼ��
	//XMSYS_VideoExit ();


	// ��Ƶ���ͨ������(ͨ���š�����ӿ����͡���Ƶ�ߴ�)
	lcd_config_index = 0;
	lcdc_channel = lcdc_channel_config[lcd_config_index].lcdc_channel;


	// ��Ƶ���ͨ������ر�
	HW_lcdc_set_backlight_on (lcdc_channel, 0);

	// ��Ƶ���ͨ����ʾ�ر�
	HW_lcdc_set_display_on (lcdc_channel, 0);

	// ��Ƶ���ͨ���߼��ر�(�ر�ʱ��)
	HW_lcdc_exit (lcdc_channel);

	memset (ch0_osd_yuv_buffer, 0, sizeof(ch0_osd_yuv_buffer));
	memset (ch0_osd_rgb_buffer, 0, sizeof(ch0_osd_rgb_buffer));

}
