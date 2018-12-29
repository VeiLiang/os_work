//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_osd_layer.h
//	  constant，macro & basic typedef definition of OSD layer service
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
// OSD逻辑层

#ifndef _XM_OSD_LAYER_H_
#define _XM_OSD_LAYER_H_

#include "hw_osd_layer.h"

#if defined (__cplusplus)
	extern "C"{
#endif


typedef unsigned int XM_COLORREF;	// 0xaarrggbb

#define	XM_RGB(r,g,b)		( (((XM_COLORREF)(b))&0xFF) | ((((XM_COLORREF)(g))&0xFF)<<8) | ((((XM_COLORREF)(r))&0xFF)<<16) | (((XM_COLORREF)0xFF) << 24))
#define	XM_ARGB(a,r,g,b)		( (((XM_COLORREF)(b))&0xFF) | ((((XM_COLORREF)(g))&0xFF)<<8) | ((((XM_COLORREF)(r))&0xFF)<<16) | (((XM_COLORREF)(a)) << 24))
#define	XM_GetBValue(rgb)	( (unsigned char)((rgb) & 0xFF) )
#define	XM_GetGValue(rgb)	( (unsigned char)(((rgb) >> 8) & 0xFF) )
#define	XM_GetRValue(rgb)	( (unsigned char)(((rgb) >> 16) & 0xFF) )

#define	ARGB888_TO_ARGB454(clr)		(unsigned short)(((((clr) >> 16) & 0xF0) << 5) \
											| ((((clr) >> 8) & 0xF8) << 1) \
											| (((clr) & 0xF0) >> 4) \
											| (((clr) >> 29) << 13) )

#define	ARGB454_TO_ARGB888(clr)		(unsigned int)( ((((clr) >> 9) & 0x0F) << 20) \
											| ((((clr) >> 4) & 0x1F) << 11) \
											| ((((clr) >> 0) & 0x0F) << 4) \
											| ((((clr) >> 13) & 0x07) << 29) )

#define	ARGB888_TO_RGB565(clr)		(unsigned short)(((((clr) >> 19) & 0x1F) << 11) \
											| ((((clr) >> 10) & 0x3F) << 5) \
											| (((clr) & 0xF8) >> 3) )

#define	RGB565_TO_ARGB888(clr)		(unsigned int)( ((((clr) >> 11) & 0x1F) << 19) \
											| ((((clr) >> 5) & 0x3F) << 10) \
											| ((((clr) >> 0) & 0x1F) << 3) | 0xFF000000 )


typedef struct tagXM_OSDLAYER {
	unsigned int		osd_layer_format;				// OSD层像素格式
	unsigned int		osd_stride;						// OSD源数据行字节长度(RGB格式源数据使用)
	unsigned int		osd_brightness_coeff;		// OSD层亮度因子，0 ~ 64
																//		0	
																//		64	原始像素值
	int					osd_x_position;				// OSD层相对LCD原点的水平偏移
	int					osd_y_position;				// OSD层相对LCD原点的垂直偏移
	unsigned int		osd_width;						// OSD层显示宽度
	unsigned int		osd_height;						// OSD层显示高度
	
	unsigned int		viewable_en;					// OSD逻辑层可视区域使能标志 0 禁止 1 使能
	unsigned int		viewable_x;						// OSD逻辑层可视区域相对LCD原点的水平偏移
	unsigned int		viewable_y;						// OSD逻辑层可视区域相对LCD原点的垂直偏移
	unsigned int		viewable_w;						// OSD逻辑层可视区域的宽度
	unsigned int		viewable_h;						// OSD逻辑层可视区域的高度
	
	unsigned int		osd_global_coeff;				// 当前层与前一层的全局混合因子
	unsigned int		osd_global_coeff_enable;	// 全局混合因子使能标志 0 禁止 1 使能
	unsigned int		osd_enable;						// OSD层使能标志 0 禁止 1 使能
	unsigned char *	osd_y_addr;						// 1) YUV420格式源数据的Y分量起始地址
																// 2) 其他格式源数据的起始地址
	unsigned char *	osd_u_addr;						// 1) YUV420格式源数据的U分量起始地址
	unsigned char *	osd_v_addr;						// 1) YUV420格式源数据的V分量起始地址

	unsigned int		osd_h_min_moving_pixels;	// OSD层水平方向最小移动像素点个数
																// 不同像素格式的水平最小移动像素点不同
	unsigned int		osd_v_min_moving_pixels;	// OSD层垂直方向最小移动像素点个数
																// 不同像素格式的水平最小移动像素点不同

	osd_callback_t		osd_callback;
	void *				osd_private;

	
} XM_OSDLAYER;

// YUV硬件Buffer限制，每16个像素点为一个最小Buffer单元显示。
// 即YUV平面行方向必须为16字节对齐，列方向为2行对齐
unsigned int XM_lcdc_osd_horz_align (unsigned int lcdc_channel, unsigned int x);
unsigned int XM_lcdc_osd_vert_align (unsigned int lcdc_channel, unsigned int y);

// 创建一个OSD逻辑层
// 当layer_enable == 1，该OSD层使能，并输出到相应的LCDC输出通道
// 返回值 == NULL 表示创建失败
XM_OSDLAYER * XM_osd_layer_init (
								unsigned int lcd_channel,						// LCDC输出通道
								unsigned int osd_layer,							// OSD层通道号
								unsigned int layer_enable,						// 通道使能(1)或关闭(0)
								unsigned int layer_format,						// OSD通道平面格式
								unsigned int layer_global_coeff,				// 全局coeff因子 (0 ~ 63)
								unsigned int layer_global_coeff_enable,	// 是否使能全局coeff因子(0 ~ 1)
								unsigned int layer_brightness_coeff,		// 亮度因子 (0 ~ 64)
								unsigned int layer_width, unsigned int layer_height,
																						// OSD通道平面尺寸
								unsigned int layer_stride,						// OSD通道显示数据行字节长度
								int layer_offset_x, int layer_offset_y,	// OSD通道相对于LCD显示原点(左上角)的初始偏移
								unsigned char *y_addr, unsigned char *u_addr, unsigned char *v_addr,
																						// OSD通道显示数据缓冲区指针
																						// RGB格式，仅y_addr有效
																						// YUV格式，y_addr,u_addr,v_addr均需要
								osd_callback_t layer_callback,				// layer回调函数及参数
								void *layer_private
								);


// LCDC OSD逻辑层初始化
void XM_lcdc_osd_init (void);
void XM_lcdc_osd_exit (void);


// 使能或禁止OSD逻辑层
void XM_lcdc_osd_set_enable (unsigned int lcd_channel, unsigned int osd_layer, int enable);
unsigned int XM_lcdc_osd_get_enable (unsigned int lcd_channel, unsigned int osd_layer);

// 使能或禁止OSD逻辑层的全局alpha因子
void XM_lcdc_osd_set_global_coeff_enable (unsigned int lcd_channel, unsigned int osd_layer, unsigned int enable);
unsigned int XM_lcdc_osd_get_global_coeff_enable (unsigned int lcd_channel, unsigned int osd_layer);

// 设置OSD逻辑层的全局alpha因子
void XM_lcdc_osd_set_global_coeff (unsigned int lcd_channel, unsigned int osd_layer, unsigned int global_coeff);
unsigned int XM_lcdc_osd_get_global_coeff (unsigned int lcd_channel, unsigned int osd_layer);

// 设置OSD逻辑层的源数据显示格式
void XM_lcdc_osd_set_format (unsigned int lcd_channel, unsigned int osd_layer, unsigned int format);
unsigned int  XM_lcdc_osd_get_format (unsigned int lcd_channel, unsigned int osd_layer);

// 设置OSD逻辑层的尺寸
void XM_lcdc_osd_set_width (unsigned int lcd_channel, unsigned int osd_layer, unsigned int width);
unsigned int XM_lcdc_osd_get_width (unsigned int lcd_channel, unsigned int osd_layer);
void XM_lcdc_osd_set_height (unsigned int lcd_channel, unsigned int osd_layer, unsigned int height);
unsigned int XM_lcdc_osd_get_height (unsigned int lcd_channel, unsigned int osd_layer);

// 设置OSD逻辑层原点(OSD逻辑层左上角)在LCD输出通道上的相对偏移(相对LCD设备的左上角，正负数)
void XM_lcdc_osd_set_x_offset (unsigned int lcd_channel, unsigned int osd_layer, int x_offset);
int XM_lcdc_osd_get_x_offset (unsigned int lcd_channel, unsigned int osd_layer);
void XM_lcdc_osd_set_y_offset (unsigned int lcd_channel, unsigned int osd_layer, int y_offset);
int XM_lcdc_osd_get_y_offset (unsigned int lcd_channel, unsigned int osd_layer);

// 设置OSD逻辑层的显示平面数据缓冲区
// RGB格式仅需要y_addr
// YUV格式需指定y_addr，u_addr，v_addr
void XM_lcdc_osd_set_yuv_addr (unsigned int lcd_channel, unsigned int osd_layer, 
										 unsigned char *y_addr, unsigned char *u_addr, unsigned char *v_addr);
unsigned char * XM_lcdc_osd_get_y_addr (unsigned int lcd_channel, unsigned int osd_layer);
unsigned char * XM_lcdc_osd_get_u_addr (unsigned int lcd_channel, unsigned int osd_layer);
unsigned char * XM_lcdc_osd_get_v_addr (unsigned int lcd_channel, unsigned int osd_layer);

// 设置OSD逻辑层的亮度因子
void XM_lcdc_osd_set_brightness_coeff (unsigned int lcd_channel, unsigned int osd_layer, unsigned int mult_coeff);
unsigned int XM_lcdc_osd_get_brightness_coeff (unsigned int lcd_channel, unsigned int osd_layer);

// 设置OSD逻辑层视频缓冲区的行字节长度
void XM_lcdc_osd_set_stride (unsigned int lcd_channel, unsigned int osd_layer, unsigned int stride);
unsigned int XM_lcdc_osd_get_stride (unsigned int lcd_channel, unsigned int osd_layer);

// 设置OSD逻辑层回调函数
void XM_lcdc_osd_set_callback (unsigned int lcd_channel, unsigned int osd_layer, 
										 osd_callback_t osd_callback, void *osd_private);

// 设置OSD逻辑层的可视区域
void XM_lcdc_osd_set_viewable_width (unsigned int lcd_channel, unsigned int osd_layer, unsigned int width);
unsigned int XM_lcdc_osd_get_viewable_width (unsigned int lcd_channel, unsigned int osd_layer);
void XM_lcdc_osd_set_viewable_height (unsigned int lcd_channel, unsigned int osd_layer, unsigned int height);
unsigned int XM_lcdc_osd_get_viewable_height (unsigned int lcd_channel, unsigned int osd_layer);

// 设置OSD逻辑层可视区域的偏移位置(相对视频显示通道的左上角)
void XM_lcdc_osd_set_viewable_x_offset (unsigned int lcd_channel, unsigned int osd_layer, int x_offset);
int XM_lcdc_osd_get_viewable_x_offset (unsigned int lcd_channel, unsigned int osd_layer);
void XM_lcdc_osd_set_viewable_y_offset (unsigned int lcd_channel, unsigned int osd_layer, int y_offset);
int XM_lcdc_osd_get_viewable_y_offset (unsigned int lcd_channel, unsigned int osd_layer);

// 使能或禁止OSD逻辑层的可视区域
void XM_lcdc_osd_set_viewable_enable (unsigned int lcd_channel, unsigned int osd_layer, int enable);
unsigned int XM_lcdc_osd_get_viewable_enable (unsigned int lcd_channel, unsigned int osd_layer);

// 设置LCDC的UI层视频格式
void XM_lcdc_osd_set_ui_format (unsigned int lcd_channel, unsigned int layer_format);
// 读取LCDC的UI层视频格式
unsigned int XM_lcdc_osd_get_ui_format (unsigned int lcd_channel);

// 锁定/解锁LCDC的OSD层
void XM_lcdc_osd_layer_set_lock (unsigned int lcd_channel, unsigned int osd_layer_mask, unsigned int lock_mask);


// 将PNG格式的源数据显示在OSD Layer的指定位置
// 仅支持RGB格式的OSD层
// osd_layer_buffer    OSD layer的原点缓冲区指针
// osd_layer_format    OSD layer的格式(ARGB8888/RGB565/ARGB3454/YUV420)
// osd_layer_width     OSD layer的像素宽度
// osd_layer_height    OSD layer的像素高度
// osd_layer_stride    OSD layer的行字节宽度
// osd_image_x         PNG image的显示坐标(相对于OSD层原点的偏移，>=0 )
// osd_image_y
// png_image_buff	     PNG image数据缓冲区
// png_image_size		  PNG image数据字节长度
// transparent_color   定义PNG image的透明色处理
//                     (unsigned int)(0xFFFFFFFF)  透明色
//                     作为背景色填充在OSD layer指定的位置
// layer_alpha			   PNG image的全覆盖Alpha因子
int  XM_lcdc_load_png_image (unsigned char *osd_layer_buffer, 
									unsigned int osd_layer_format,
									unsigned int osd_layer_width, unsigned int osd_layer_height,
									unsigned int osd_layer_stride,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *png_image_buff, unsigned int png_image_size,
									XM_COLORREF transparent_color,
									unsigned char layer_alpha);


// 将PNG格式的源数据显示在OSD Layer的指定位置
// 仅支持RGB格式的OSD层
// lcd_channel			  LCDC通道号
// osd_layer			  OSD层序号
// osd_image_x         PNG image的显示坐标(相对于OSD层原点的偏移，>=0 )
// osd_image_y
// png_image_buff		  PNG image的数据缓冲
// png_image_size		  PNG image的数据缓冲字节大小
// transparent_color   定义PNG image的透明色处理
//                     (unsigned int)(0xFFFFFFFF)  透明色
//                     作为背景色填充在OSD layer指定的位置
int  XM_lcdc_osd_layer_load_png_image (unsigned int lcd_channel,
													unsigned int osd_layer,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *png_image_buff, unsigned int png_image_size,
									XM_COLORREF transparent_color,
									unsigned char alpha);

// 将PNG格式的文件显示在OSD Layer的指定位置
// 仅支持RGB格式的OSD层
// lcd_channel			  LCDC通道号
// osd_layer			  OSD层序号
// osd_image_x         PNG image的显示坐标(相对于OSD层原点的偏移，>=0 )
// osd_image_y
// png_image_name		  PNG image的文件名
// transparent_color   定义PNG image的透明色处理
//                     (unsigned int)(0xFFFFFFFF)  透明色
//                     作为背景色填充在OSD layer指定的位置
int  XM_lcdc_osd_layer_load_png_image_file (	
										unsigned int lcd_channel,
										unsigned int osd_layer,
										unsigned int osd_image_x, unsigned int osd_image_y,
										const char *png_image_name,
										XM_COLORREF transparent_color,
										unsigned char alpha);

// 将PNG格式的文件显示在OSD Layer的指定位置
// 仅支持RGB格式的OSD层
// osd_layer_buffer    OSD layer的原点缓冲区指针
// osd_layer_format    OSD layer的格式(ARGB8888/RGB565/ARGB3454/YUV420)
// osd_layer_width     OSD layer的像素宽度
// osd_layer_height    OSD layer的像素高度
// osd_layer_stride    OSD layer的行字节宽度
// osd_image_x         PNG image的显示坐标(相对于OSD层原点的偏移，>=0 )
// osd_image_y
// png_image_name	     PNG image文件
// transparent_color   定义PNG image的透明色处理
//                     (unsigned int)(0xFFFFFFFF)  透明色
//                     作为背景色填充在OSD layer指定的位置
int  XM_lcdc_load_png_image_file (unsigned char *osd_layer_buffer, 
									unsigned int osd_layer_format,
									unsigned int osd_layer_width, unsigned int osd_layer_height,
									unsigned int osd_layer_stride,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *png_image_name,
									XM_COLORREF transparent_color,
									unsigned char alpha);


// 将YUV420格式源图像的指定区域(copy_w, copy_h)复制到OSD YUV层的指定位置(osd_offset_x, osd_offset_y)
// 仅YUV420 或者 Y_UV420层支持
void XM_lcdc_load_yuv_image (
						  unsigned char *yuv_img,		// YUV420格式的源图像,
						  unsigned int yuv_format,		// XM_OSD_LAYER_FORMAT_YUV420 或者 XM_OSD_LAYER_FORMAT_Y_UV420
						  unsigned int img_w, unsigned int img_h,		
																// 源图像原始尺寸
						  unsigned int img_offset_x, unsigned int img_offset_y,			
																// 复制区域位于源图像中的偏移
						  unsigned char *osd_y,			// YUV420 OSD层 Y地址
																// Y_UV420 OSD层 Y地址
						  unsigned char *osd_u,			// YUV420 OSD层 U地址
																// Y_UV420 OSD层 UV地址
						  unsigned char *osd_v,			// YUV420 OSD层 V地址
																//	Y_UV420 未使用
						  unsigned int osd_w, unsigned int osd_h,		
																// OSD层宽度、高度信息
						  unsigned int osd_offset_x, unsigned int osd_offset_y,			
																// 复制区域位于OSD层的偏移
						  unsigned int copy_w, unsigned int copy_h		
																// 源图像中需要复制的区域尺寸(从源图像左上角原点开始)
						  );


// 将YUV420格式源图像的指定区域(copy_w, copy_h)复制到OSD YUV层的指定位置(osd_offset_x, osd_offset_y)
// 仅YUV层支持
void XM_lcdc_osd_layer_load_yuv_image (
						  unsigned int lcd_channel,						// LCDC通道
						  unsigned int osd_layer,							// OSD层
						  unsigned char *yuv_img,							// YUV420格式的源图像
						  unsigned int img_w, unsigned int img_h,		// 源图像原始尺寸
						  unsigned int img_offset_x, unsigned int img_offset_y,			// 复制区域位于源图像中的偏移
						  unsigned int osd_offset_x, unsigned int osd_offset_y,			// 复制区域位于OSD层的偏移
						  unsigned int copy_w, unsigned int copy_h		// 源图像中需要复制的区域尺寸(从源图像左上角原点开始)
						  );

// 将YUV420格式源文件的指定区域(copy_w, copy_h)复制到OSD YUV层的指定位置(osd_offset_x, osd_offset_y)
// 仅YUV层支持
int XM_lcdc_load_yuv_image_file (
						  unsigned char *yuv_file,		// YUV420格式的文件名
						  unsigned int yuv_format,		// XM_OSD_LAYER_FORMAT_YUV420 或者 XM_OSD_LAYER_FORMAT_Y_UV420
						  unsigned int img_w, unsigned int img_h,		// 源图像原始尺寸
						  unsigned int img_offset_x, unsigned int img_offset_y,			// 复制区域位于源图像中的偏移
						  unsigned char *osd_y,			// OSD层 YUV地址
						  unsigned char *osd_u,
						  unsigned char *osd_v,
						  unsigned int osd_w, unsigned int osd_h,			// OSD层宽度、高度信息
						  unsigned int osd_offset_x, unsigned int osd_offset_y,			// 复制区域位于OSD层的偏移
						  unsigned int copy_w, unsigned int copy_h		// 源图像中需要复制的区域尺寸(从源图像左上角原点开始)
						  );

// 将YUV420格式源文件的指定区域(copy_w, copy_h)复制到OSD YUV层的指定位置(osd_offset_x, osd_offset_y)
// 仅YUV层支持
int XM_lcdc_osd_layer_load_yuv_image_file (
						  unsigned int lcd_channel,						// LCDC通道
						  unsigned int osd_layer,							// OSD层
						  unsigned char *yuv_file,							// YUV420格式的文件名
						  unsigned int img_w, unsigned int img_h,		// 源图像原始尺寸
						  unsigned int img_offset_x, unsigned int img_offset_y,			// 复制区域位于源图像中的偏移
						  unsigned int osd_offset_x, unsigned int osd_offset_y,			// 复制区域位于OSD层的偏移
						  unsigned int copy_w, unsigned int copy_h		// 源图像中需要复制的区域尺寸(从源图像左上角原点开始)
						  );

int XM_lcdc_load_gif_image (unsigned char *osd_layer_buffer, 
									unsigned int osd_layer_format,
									unsigned int osd_layer_width, unsigned int osd_layer_height,
									unsigned int osd_layer_stride,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *gif_image_buff, unsigned int gif_image_size,
									XM_COLORREF transparent_color,
									unsigned int alpha);

int XM_lcdc_load_gif_image_file (unsigned char *osd_layer_buffer, 
									unsigned int osd_layer_format,
									unsigned int osd_layer_width, unsigned int osd_layer_height,
									unsigned int osd_layer_stride,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *gif_image_file,
									XM_COLORREF transparent_color,
									unsigned int alpha);

int XM_lcdc_osd_layer_load_gif_image_file (
									unsigned int lcd_channel,
									unsigned int osd_layer,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *gif_image_file,
									XM_COLORREF transparent_color,
									unsigned int alpha);



void XM_lcdc_osd_layer_load_ayuv420_image (
						  unsigned char *img_y,			// YUV格式源图像的Y分量地址
						  unsigned char *img_u,			// YUV格式源图像的U分量地址
						  unsigned char *img_v,			// YUV格式源图像的V分量地址
						  unsigned char *img_a,			// YUV格式源图像的Alpha分量地址
						  unsigned int img_w,			// YUV格式源图像的像素宽度
						  unsigned int img_h,			// YUV格式源图像的像素高度
						  unsigned int img_stride,		// YUV格式源图像的行字节长度
						  unsigned int img_offset_x,	// 复制区域位于源图像的偏移(从源图像左上角原点开始)
						  unsigned int img_offset_y,
						  unsigned int copy_w, unsigned int copy_h,		// 源图像中需要复制的区域尺寸(从源图像左上角原点开始)
						  unsigned char *osd_y,			// OSD层 YUV地址
						  unsigned char *osd_u,
						  unsigned char *osd_v,
						  unsigned int osd_w,			// OSD层宽度、高度信息
						  unsigned int osd_h,			
						  unsigned int osd_stride,		// OSD层的行字节长度
						  unsigned int osd_offset_x,	// 复制区域位于OSD层的偏移
						  unsigned int osd_offset_y			

						  );


// 将ARGB888格式的源图像转换为YUV444格式
void XM_lcdc_convert_argb888_to_ayuv444 (
						  unsigned char *argb_img,		// ARGB888格式的源图像
						  unsigned int img_stride,		// 源图像的行字节长度
						  unsigned int img_w,			// 源图像的宽度(像素点)
						  unsigned int img_h,			// 源图像的高度(像素点)
						  unsigned char *osd_y,			// 目标OSD层的YUV地址
						  unsigned char *osd_u,
						  unsigned char *osd_v,
						  unsigned char *osd_a,			// 保存a因子
						  unsigned int osd_stride,		// 目标OSD层(Y分量)的行字节长度
																//		(UV分量除以2)
						  unsigned int osd_w,			// 目标OSD层的像素宽度
						  unsigned int osd_h			// 目标OSD层的像素高度
						  );

void XM_lcdc_osd_layer_load_ayuv444_image_normal (
						  unsigned char *img_y,			// YUV格式源图像的Y分量地址
						  unsigned char *img_u,			// YUV格式源图像的U分量地址
						  unsigned char *img_v,			// YUV格式源图像的V分量地址
						  unsigned char *img_a,			// YUV格式源图像的Alpha分量地址
						  unsigned int img_w,			// YUV格式源图像的像素宽度
						  unsigned int img_h,			// YUV格式源图像的像素高度
						  unsigned int img_stride,		// YUV格式源图像的行字节长度
						  unsigned int img_offset_x,	// 复制区域位于源图像的偏移(从源图像左上角原点开始)
						  unsigned int img_offset_y,
						  unsigned int copy_w, unsigned int copy_h,		// 源图像中需要复制的区域尺寸(从源图像左上角原点开始)
						  unsigned int	osd_layer_format,
						  unsigned char *osd_y,			// OSD层 YUV地址
						  unsigned char *osd_u,
						  unsigned char *osd_v,
						  unsigned int osd_w,			// OSD层宽度、高度信息
						  unsigned int osd_h,			
						  unsigned int osd_stride,		// OSD层的行字节长度
						  unsigned int osd_offset_x,	// 复制区域位于OSD层的偏移
						  unsigned int osd_offset_y
						  );

void XM_lcdc_osd_layer_load_ayuv444_image (
						  unsigned char *img_y,			// YUV格式源图像的Y分量地址
						  unsigned char *img_u,			// YUV格式源图像的U分量地址
						  unsigned char *img_v,			// YUV格式源图像的V分量地址
						  unsigned char *img_a,			// YUV格式源图像的Alpha分量地址
						  unsigned int img_w,			// YUV格式源图像的像素宽度
						  unsigned int img_h,			// YUV格式源图像的像素高度
						  unsigned int img_stride,		// YUV格式源图像的行字节长度
						  unsigned int img_offset_x,	// 复制区域位于源图像的偏移(从源图像左上角原点开始)
						  unsigned int img_offset_y,
						  unsigned int copy_w, unsigned int copy_h,		// 源图像中需要复制的区域尺寸(从源图像左上角原点开始)
						  unsigned int	osd_layer_format,
						  unsigned char *osd_y,			// OSD层 YUV地址
						  unsigned char *osd_u,
						  unsigned char *osd_v,
						  unsigned int osd_w,			// OSD层宽度、高度信息
						  unsigned int osd_h,			
						  unsigned int osd_stride,		// OSD层的行字节长度
						  unsigned int osd_offset_x,	// 复制区域位于OSD层的偏移
						  unsigned int osd_offset_y
						  );

// YUVA444分块存储
void XM_lcdc_osd_layer_load_yuva444_image (
						  unsigned char *img_y,			// YUV格式源图像的Y分量地址
						  unsigned int img_w,			// YUV格式源图像的像素宽度
						  unsigned int img_h,			// YUV格式源图像的像素高度
						  unsigned int img_stride,		// YUV格式源图像的行字节长度
						  unsigned int img_offset_x,	// 复制区域位于源图像的偏移(从源图像左上角原点开始)
						  unsigned int img_offset_y,
						  unsigned int copy_w, unsigned int copy_h,		// 源图像中需要复制的区域尺寸(从源图像左上角原点开始)
						  unsigned int	osd_layer_format,
						  unsigned char *osd_y,			// OSD层 YUV地址
						  unsigned char *osd_u,
						  unsigned char *osd_v,
						  unsigned int osd_w,			// OSD层宽度、高度信息
						  unsigned int osd_h,			
						  unsigned int osd_stride,		// OSD层的行字节长度
						  unsigned int osd_offset_x,	// 复制区域位于OSD层的偏移
						  unsigned int osd_offset_y
						  );


// 将非压缩的RAW格式图像数据复制到相同显示格式的OSD层
int XM_lcdc_copy_raw_image (unsigned char *osd_layer_buffer,		// 目标OSD的缓冲区基址
																						//		YUV420为Y、U、V分量分块格式
									unsigned int osd_layer_format,			// 目标OSD的视频格式
									unsigned int osd_layer_width,				// 目标OSD的像素宽度、像素高度
									unsigned int osd_layer_height,	
									unsigned int osd_layer_stride,			// 目标OSD的行字节长度
																						//		YUV420格式表示Y分量，UV分量除以2
									unsigned int osd_offset_x,					//	复制区域在OSD层的偏移 (相对OSD平面的左上角)
									unsigned int osd_offset_y,
									unsigned char *raw_image_buffer,			// 源图像数据的缓冲区基址
									unsigned int raw_image_width,				// 源图像的像素宽度
									unsigned int raw_image_height,			// 源图像的像素高度
									unsigned int raw_image_stride,			// 源图像的行字节长度	
									unsigned int raw_offset_x,					// 复制区域在源图像的偏移(相对源图像平面的左上角)
									unsigned int raw_offset_y,
									unsigned int copy_image_w,					// 待复制区域的像素宽度、像素高度		
									unsigned int copy_image_h,
									unsigned int layer_alpha					// OSD层的全局Alpha因子
									);

int XM_lcdc_load_raw_image (unsigned char *osd_layer_buffer,		// 目标OSD的缓冲区基址
																						//		YUV420为Y、U、V分量分块格式
									unsigned int osd_layer_format,			// 目标OSD的视频格式
									unsigned int osd_layer_width,				// 目标OSD的像素宽度、像素高度
									unsigned int osd_layer_height,	
									unsigned int osd_layer_stride,			// 目标OSD的行字节长度
																						//		YUV420格式表示Y分量，UV分量除以2
									unsigned int osd_offset_x,					//	复制区域在OSD层的偏移 (相对OSD平面的左上角)
									unsigned int osd_offset_y,
									unsigned char *raw_image_buffer,			// 源图像数据的缓冲区基址
									unsigned int raw_image_width,				// 源图像的像素宽度
									unsigned int raw_image_height,			// 源图像的像素高度
									unsigned int raw_image_stride,			// 源图像的行字节长度	
									unsigned int raw_offset_x,					// 复制区域在源图像的偏移(相对源图像平面的左上角)
									unsigned int raw_offset_y,
									unsigned int copy_image_w,					// 待复制区域的像素宽度、像素高度		
									unsigned int copy_image_h,
									unsigned int layer_alpha					// OSD层的全局Alpha因子
									);

int XM_lcdc_flush_dirty_region (unsigned char *osd_layer_buffer,	// OSD平面的缓存地址
											unsigned int   osd_layer_format,	// OSD平面的格式
											unsigned int osd_width, 	// OSD平面的宽度/高度
											unsigned int osd_height,
											unsigned int osd_stride,	// OSD平面的行字节长度
											unsigned int d_x,		// 脏区的偏移(相对于OSD平面的左上角原点)
											unsigned int d_y,
											unsigned int d_w,		// 脏区的尺寸
											unsigned int d_h
										);

int XM_lcdc_inv_region (unsigned char *osd_layer_buffer,	// OSD平面的缓存地址
											unsigned int osd_layer_format,	// OSD平面的格式
											unsigned int osd_width, 	// OSD平面的宽度/高度
											unsigned int osd_height,
											unsigned int osd_stride,	// OSD平面的行字节长度
											unsigned int d_x,		// 脏区的偏移(相对于OSD平面的左上角原点)
											unsigned int d_y,
											unsigned int d_w,		// 脏区的尺寸
											unsigned int d_h
										);


// 将LCD屏幕坐标转换为OSD层的逻辑坐标
// 返回值
//		-1		失败
//		0		成功
int XM_lcdc_lcd_coordinate_to_osd_coordinate (unsigned int lcd_channel, unsigned int osd_layer, int *x, int *y);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_OSD_LAYER_H_
