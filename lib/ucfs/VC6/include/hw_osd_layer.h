//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: hw_osd_layer.h
//	  constant，macro & basic typedef definition of OSD layer service
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************

// ARK1960 OSD LAYER硬件接口
#ifndef _HW_OSD_LAYER_H_
#define _HW_OSD_LAYER_H_

#if defined (__cplusplus)
	extern "C"{
#endif

#define	_XM_USE_Y_UV420_		// 使用Y_UV420, UV交替存放模式

// ARK1960定义1个视频输出通道
enum {
	XM_LCDC_CHANNEL_0 = 0,			// 通道0
//	XM_LCDC_CHANNEL_1,				// 通道1
	XM_LCDC_CHANNEL_COUNT
}; 


// LCDC输出通道类型定义
enum {
	// RGB LCD屏接口
	XM_LCDC_TYPE_RGB = 0,		// LCD RGB屏接口

	// CPU LCD屏接口
	XM_LCDC_TYPE_CPU,				// LCD CPU屏接口

	XM_LCDC_TYPE_BT601,			// ITU BT601数字视频

	// VGA类型
	XM_LCDC_TYPE_VGA,				// VGA视频接口 640 X 480
	XM_LCDC_TYPE_SVGA,			// SVGA视频接口 800 X 600

	// AVOUT 类型
	XM_LCDC_TYPE_CVBS_NTSC,		// CVBS视频接口 720 X 480
	XM_LCDC_TYPE_CVBS_PAL,		// CVBS视频接口 720 X 576
	XM_LCDC_TYPE_YPbPr_480i,	// YPbPr 色差分量接口 720 X 480
	XM_LCDC_TYPE_YPbPr_480p,	// YPbPr 色差分量接口 720 X 480
	XM_LCDC_TYPE_YPbPr_576i,	// YPbPr 色差分量接口 720 X 576
	XM_LCDC_TYPE_YPbPr_576p,	// YPbPr 色差分量接口 720 X 576

	// HDMI 类型
	XM_LCDC_TYPE_HDMI_1080i,	// HDMI 1080i接口，隔行扫描 1920 X 1080
	XM_LCDC_TYPE_HDMI_1080p,	// HDMI 1080p接口，逐行扫描 1920 X 1080
	XM_LCDC_TYPE_HDMI_720i,		// HDMI 720i接口，隔行扫描 1280 X 720
	XM_LCDC_TYPE_HDMI_720p,		// HDMI 720p接口，逐行扫描 1280 X 720

	XM_LCDC_TYPE_COUNT
};

// OSD层定义(3层)
enum {
	XM_OSD_LAYER_0 = 0,			// OSD 第0层
	XM_OSD_LAYER_1,				// OSD 第1层
	XM_OSD_LAYER_2,				// OSD 第2层
	XM_OSD_LAYER_COUNT
};

// OSD Layer格式(YUV420, ARGB888, RGB565, ARGB454, )
enum {
	XM_OSD_LAYER_FORMAT_YUV420 = 	0,	// YUV420,	Y、U、V数据分块存放,
	XM_OSD_LAYER_FORMAT_ARGB888,		// 32位 AAAAAAAARRRRRRRRGGGGGGGGBBBBBBBB
	XM_OSD_LAYER_FORMAT_RGB565,		// 16位 RRRRRGGGGGGBBBBB
	XM_OSD_LAYER_FORMAT_ARGB454,		// 16位 AAARRRRGGGGGBBBB
	XM_OSD_LAYER_FORMAT_AYUV444,		// YUV444
	XM_OSD_LAYER_FORMAT_Y_UV420,		// Y_UV420, Y、UV数据分块存放, UV数据交替存放
	XM_OSD_LAYER_FORMAT_COUNT
};

// OSD相对显示区(原点位于左上角)偏移
#define	OSD_MAX_H_POSITION	(2048 - 1)		// 最大水平像素偏移(相对左上角)
#define	OSD_MAX_V_POSITION	(2048 - 1)		// 最大垂直像素偏移(相对左上角)

#define	OSD_MAX_H_SIZE			(2048)			// OSD最大像素宽度
#define	OSD_MAX_V_SIZE			(2048)			// OSD最大像素高度


#define	OSD_MAX_YUV_SIZE		(2048)			// OSD YUV层最大尺寸
#define	OSD_MAX_RGB_SIZE		(1280)			// OSD RGB层最大尺寸

typedef void (*osd_callback_t)(void *osd_data);


// LCDC输出通道初始化
// lcdc_channel	LCDC视频输出通道号(显示设备)
// lcdc_type		LCDC视频输出通道接口类型
// xdots				LCDC视频输出通道像素宽度 
// ydots				LCDC视频输出通道像素高度
void HW_lcdc_init (unsigned int lcdc_channel, unsigned int lcdc_type, unsigned int xdots, unsigned int ydots);

// LCDC输出通道关闭(关闭时钟、关闭控制逻辑)
void HW_lcdc_exit (unsigned int lcdc_channel);


// 开启/关闭LCDC视频通道的显示
// on  1  开启显示
// on  0  关闭显示
void HW_lcdc_set_display_on (unsigned int lcdc_channel, unsigned int on);

// 开启/关闭LCDC视频通道的背光显示
// lcdc_channel	LCDC视频输出通道号(显示设备)
// on  0.0 ~ 1.0  
//     0.0  背光关闭
//     1.0  背光最大亮度
void HW_lcdc_set_backlight_on (unsigned int lcdc_channel, float on);
void HW_lcdc_set_osd_on (unsigned int lcdc_channel, float on);
// 获取LCDC视频通道的背光显示亮度
// 返回值 0.0 ~ 1.0
//     0.0  背光关闭
//     1.0  背光最大亮度
float HW_lcdc_get_backlight_on (unsigned int lcdc_channel);

// 获取CDC输出通道的平面宽度
unsigned int HW_lcdc_get_xdots (unsigned int lcd_channel);
// 获取CDC输出通道的平面高度
unsigned int HW_lcdc_get_ydots (unsigned int lcd_channel);


// 设置LCDC输出通道背景色
void HW_lcdc_set_background_color (unsigned int lcd_channel, 
													unsigned char r, unsigned char g, unsigned char b);

void HW_lcdc_osd_set_enable (unsigned int lcd_channel, unsigned int osd_layer, int enable);
unsigned int HW_lcdc_osd_get_enable (unsigned int lcd_channel, unsigned int osd_layer);


void HW_lcdc_osd_set_global_coeff_enable (unsigned int lcd_channel, unsigned int osd_layer, int enable);

void HW_lcdc_osd_set_global_coeff (unsigned int lcd_channel, unsigned int osd_layer, unsigned int global_coeff);

void HW_lcdc_osd_set_format (unsigned int lcd_channel, unsigned int osd_layer, unsigned int format);

void HW_lcdc_osd_set_width (unsigned int lcd_channel, unsigned int osd_layer, unsigned int width);

void HW_lcdc_osd_set_height (unsigned int lcd_channel, unsigned int osd_layer, unsigned int height);

void HW_lcdc_osd_set_h_position (unsigned int lcd_channel, unsigned int osd_layer, unsigned int h_position);

void HW_lcdc_osd_set_v_position (unsigned int lcd_channel, unsigned int osd_layer, unsigned int v_position);

void HW_lcdc_osd_set_left_position (unsigned int lcd_channel, unsigned int osd_layer, unsigned int left_position);

// 设置OSD物理层视频缓冲区的行像素长度
void HW_lcdc_osd_set_stride (unsigned int lcd_channel, unsigned int osd_layer, unsigned int stride);

void HW_lcdc_osd_set_brightness_coeff (unsigned int lcd_channel, unsigned int osd_layer, unsigned int brightness_coeff);

void HW_lcdc_osd_set_yuv_addr (unsigned int lcd_channel, unsigned int osd_layer, unsigned char *y_addr, unsigned char *u_addr, unsigned char *v_addr);

// LCDC帧刷新中断处理
void HW_lcdc_interrupt_handler (void);

// 安装OSD帧刷新中断处理函数
void HW_lcdc_osd_set_callback (unsigned int lcd_channel, unsigned int osd_layer, 
										 osd_callback_t osd_callback, void *osd_private);

const char *XM_lcdc_channel_name (unsigned int lcdc_channel);

const char *XM_lcdc_channel_type (unsigned int lcdc_type);

// 同步OSD参数到LCDC控制器的内部寄存器
void HW_lcdc_set_osd_coef_syn (unsigned int lcdc_channel, unsigned int coef_syn);

// 读取同步状态。
// 0 表示OSD参数已同步到LCDC控制器的内部寄存器
unsigned int HW_lcdc_get_osd_coef_syn (unsigned int lcdc_channel);



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _HW_OSD_LAYER_H_
