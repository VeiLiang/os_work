//****************************************************************************
//
//	Copyright (C) 2012~2014 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_osd_layer.c
//
//	Revision history
//
//		2012.01.11	ZhuoYongHong initial version
//		2014.02.10	ZhuoYongHong修改，适应ARK1960 LCDC控制器
//
//****************************************************************************
#include <string.h>
#include <stdlib.h>
#include <lpng\png.h>
#include <zlib\zlib.h>
#include <xm_heap_malloc.h>
#include "xm_printf.h"
#include "xm_type.h"
#include "xm_base.h"
#include "xm_file.h"
#include "hw_osd_layer.h"
#include "xm_osd_layer.h"
#include <xm_image.h>
#include "xm_gdi.h"


static XM_OSDLAYER lcdc_osd_layer[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT];
static int	lcdc_osd_locked[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT];

static unsigned int osd_ui_format = XM_OSD_LAYER_FORMAT_ARGB888;
//static unsigned int osd_ui_format = XM_OSD_LAYER_FORMAT_ARGB454;
//static unsigned int osd_ui_format = XM_OSD_LAYER_FORMAT_RGB565;

// YUV硬件Buffer限制，每16个像素点为一个最小Buffer单元显示。
// 即YUV平面行方向必须为16字节对齐，列方向为2行对齐
// 水平方向最小移动像素单位
static const unsigned char osd_minimum_moving_h_pixels[XM_OSD_LAYER_FORMAT_COUNT] = 
{
	16,		// YUV420
	16,		// ARGB888 (实际需4个像素点对齐)
	16,		// RGB565  (实际需4个像素点对齐)
	16,		// ARGB454 (实际需4个像素点对齐)
	16
};
static const unsigned char osd_minimum_moving_h_pixels_clr[XM_OSD_LAYER_FORMAT_COUNT] = 
{
	0xf,		// YUV420
	0xf,		// ARGB888
	0xf,		// RGB565
	0xf,		// ARGB454
	0xf		//
};

#ifdef LCD_ROTATE_90
static const unsigned char osd_minimum_moving_v_pixels[XM_OSD_LAYER_FORMAT_COUNT] = 
{
	16,		// YUV420
	16,		// ARGB888
	16,		// RGB565
	16,		// ARGB454
	16
};
static const unsigned char osd_minimum_moving_v_pixels_clr[XM_OSD_LAYER_FORMAT_COUNT] = 
{
	0xf,	// YUV420
	0xf,		// ARGB888
	0xf,		// RGB565
	0xf,		// ARGB454
	0xf
};
#else
// 垂直方向最小移动像素单位
static const unsigned char osd_minimum_moving_v_pixels[XM_OSD_LAYER_FORMAT_COUNT] = 
{
	2,		// YUV420
	2,		// ARGB888
	2,		// RGB565
	2,		// ARGB454
	2
};
static const unsigned char osd_minimum_moving_v_pixels_clr[XM_OSD_LAYER_FORMAT_COUNT] = 
{
	0x1,	// YUV420
	0x1,		// ARGB888
	0x1,		// RGB565
	0x1,		// ARGB454
	0x1
};
#endif

// YUV硬件Buffer限制，每16个像素点为一个最小Buffer单元显示。
// 即YUV平面行方向必须为16字节对齐，列方向为2行对齐
unsigned int XM_lcdc_osd_horz_align (unsigned int lcd_channel, unsigned int x)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return x;
	}
	x += osd_minimum_moving_h_pixels_clr[XM_OSD_LAYER_FORMAT_YUV420] - 1;
	x &= ~osd_minimum_moving_h_pixels_clr[XM_OSD_LAYER_FORMAT_YUV420];
	return x;
}

unsigned int XM_lcdc_osd_vert_align (unsigned int lcd_channel, unsigned int y)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return y;
	}
	y += osd_minimum_moving_v_pixels_clr[XM_OSD_LAYER_FORMAT_YUV420] - 1;
	y &= ~osd_minimum_moving_v_pixels_clr[XM_OSD_LAYER_FORMAT_YUV420];
	return y;
}


static int is_legal_osd_layer (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 0;
	}
	return 1;
}

// lcdc osd层初始化
void XM_lcdc_osd_init (void)
{
	int i, j;
	memset (lcdc_osd_layer, 0, sizeof(lcdc_osd_layer));
	// 关闭所有OSD物理层
	for (i = 0; i < XM_LCDC_CHANNEL_COUNT; i ++)
	{
		for (j = 0; j < XM_OSD_LAYER_COUNT; j ++)
		{
			HW_lcdc_osd_set_enable (i, j, 0);
		}
	}
}

void XM_lcdc_osd_exit (void)
{
	int i, j;
	memset (lcdc_osd_layer, 0, sizeof(lcdc_osd_layer));
	for (i = 0; i < XM_LCDC_CHANNEL_COUNT; i ++)
	{
		for (j = 0; j < XM_OSD_LAYER_COUNT; j ++)
		{
			HW_lcdc_osd_set_enable (i, j, 0);
		}
	}
}

// 配置OSD层的位置信息
static void osd_layer_setup_position (unsigned int lcd_channel, unsigned int osd_layer)
{
	unsigned int reg_h_position, reg_v_position, reg_left_position;
	unsigned char *reg_y, *reg_u, *reg_v;
	int reg_height,reg_width;
	unsigned int align_h, align_v;
	//unsigned int mode ,step;
	int channel_w = HW_lcdc_get_xdots (lcd_channel) ;//显示通道的宽度高度
	int channel_h = HW_lcdc_get_ydots (lcd_channel) ;

	XM_OSDLAYER *xm_osdlayer = &lcdc_osd_layer[lcd_channel][osd_layer];

	reg_width  = xm_osdlayer->osd_width;
	reg_height = xm_osdlayer->osd_height;
	
	//align_h = osd_minimum_moving_h_pixels[xm_osdlayer->osd_layer_format];
	align_h = osd_minimum_moving_h_pixels_clr[xm_osdlayer->osd_layer_format];
	//align_v = osd_minimum_moving_v_pixels[xm_osdlayer->osd_layer_format];
	align_v = osd_minimum_moving_v_pixels_clr[xm_osdlayer->osd_layer_format];

	if(	xm_osdlayer->osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420 
		|| xm_osdlayer->osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		// YUV420格式
		if(xm_osdlayer->osd_x_position >= 0) //水平右移
		{
			reg_h_position = xm_osdlayer->osd_x_position;
			// 应用水平移动最小距离约束
			//reg_h_position = (reg_h_position / align_h) * align_h;
			reg_h_position &= ~ align_h;
			reg_left_position = 0;
			if( (int)(reg_h_position + reg_width) > channel_w ) // 在 屏幕内，位置+OSD宽度《=屏幕宽度
				reg_width = channel_w - reg_h_position;
		}
		else                                //水平左移
		{
			reg_h_position = 0;
			reg_left_position = - xm_osdlayer->osd_x_position;
			// 应用水平移动最小距离约束
			//reg_left_position = (reg_left_position / align_h) * align_h;
			reg_left_position &= ~ align_h;
			reg_width = reg_width-reg_left_position;
		}

		if(xm_osdlayer->osd_y_position >= 0)
		{
			// OSD层的顶部位于显示区   (下方)
			// 应用垂直方向最小移动距离约束
			//reg_v_position = (xm_osdlayer->osd_y_position / align_v) * align_v;
			reg_v_position = xm_osdlayer->osd_y_position &~ align_v;
			//reg_v_position &= ~ align_v;	//只要不是向上移动，单个像素点在显示区域中移动，显示还是正常的
			reg_y = xm_osdlayer->osd_y_addr;
			reg_u = xm_osdlayer->osd_u_addr;
			reg_v = xm_osdlayer->osd_v_addr;
			if( (int)(reg_v_position + reg_height) > channel_h )
				reg_height = channel_h - reg_v_position;	//屏幕高度-坐标高度;减少带宽消耗
		}
		else
		{
			// OSD层的顶部位于显示区的顶部之上 ( 上方 )

			int y_offset = -xm_osdlayer->osd_y_position;
			// 应用垂直方向最小移动距离约束
			//y_offset = (y_offset / align_v) * align_v;
			y_offset &= ~ align_v;//向上移动，移动数据产生上移的效果中，必须偶数位置移动
			reg_v_position = 0;
			reg_y = xm_osdlayer->osd_y_addr + (y_offset) * xm_osdlayer->osd_width;
			reg_u = xm_osdlayer->osd_u_addr + ((y_offset) * xm_osdlayer->osd_width >>2);
			reg_v = xm_osdlayer->osd_v_addr + ((y_offset) * xm_osdlayer->osd_width >>2);
			// 修改OSD平面高度
			reg_height += xm_osdlayer->osd_y_position;
		}
		// 检查是否应用约束条件来规避OSD硬件的缺陷
	}
	else if(xm_osdlayer->osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{//2014-11-05 添加 Y_UV420 格式
		// Y_UV420格式
		if(xm_osdlayer->osd_x_position >= 0) //水平右移
		{
			reg_h_position = xm_osdlayer->osd_x_position;
			// 应用水平移动最小距离约束
			//reg_h_position = (reg_h_position / align_h) * align_h;
			reg_h_position &= ~ align_h;
			reg_left_position = 0;
			if( (int)(reg_h_position + reg_width) > channel_w ) // 在 屏幕内，位置+OSD宽度《=屏幕宽度
				reg_width = channel_w - reg_h_position;
		}
		else                                //水平左移
		{
			reg_h_position = 0;
			reg_left_position = - xm_osdlayer->osd_x_position;
			// 应用水平移动最小距离约束
			//reg_left_position = (reg_left_position / align_h) * align_h;
			reg_left_position &= ~ align_h;
			reg_width = reg_width-reg_left_position;
		}

		if(xm_osdlayer->osd_y_position >= 0)
		{
			// OSD层的顶部位于显示区   (下方)
			// 应用垂直方向最小移动距离约束
			//reg_v_position = (xm_osdlayer->osd_y_position / align_v) * align_v;
			reg_v_position = xm_osdlayer->osd_y_position &~ align_v;
			//reg_v_position &= ~ align_v;	//只要不是向上移动，单个像素点在显示区域中移动，显示还是正常的
			reg_y = xm_osdlayer->osd_y_addr;
			reg_u = xm_osdlayer->osd_u_addr;
			reg_v = xm_osdlayer->osd_v_addr;
			if( (int)(reg_v_position + reg_height) > channel_h )
				reg_height = channel_h - reg_v_position;	//屏幕高度-坐标高度;减少带宽消耗
		}
		else
		{
			// OSD层的顶部位于显示区的顶部之上 ( 上方 )

			int y_offset = -xm_osdlayer->osd_y_position;
			// 应用垂直方向最小移动距离约束
			//y_offset = (y_offset / align_v) * align_v;
			y_offset &= ~ align_v;//向上移动，移动数据产生上移的效果中，必须偶数位置移动
			reg_v_position = 0;
			reg_y = xm_osdlayer->osd_y_addr + (y_offset) * xm_osdlayer->osd_width;
			reg_u = xm_osdlayer->osd_u_addr + ((y_offset) * xm_osdlayer->osd_width >>1 );
			reg_v = 0; // Y_UV420  :UV data is interweaved togher
			// 修改OSD平面高度
			reg_height += xm_osdlayer->osd_y_position;
		}
		// 检查是否应用约束条件来规避OSD硬件的缺陷
		//tail 2014-11-05 添加 Y_UV420 格式
	}
	else
	{
		// RGB格式
		if(xm_osdlayer->osd_x_position >= 0)// 向右边移动
		{
			reg_h_position = xm_osdlayer->osd_x_position;
			// 应用水平移动最小距离约束
			//reg_h_position = (reg_h_position / align_h) * align_h;
			reg_h_position &= ~ align_h;
			reg_left_position = 0 ;
			if( (int)(reg_h_position + reg_width) > channel_w ) // 在 屏幕内，位置+OSD宽度《=屏幕宽度
				reg_width = reg_width - (reg_h_position + reg_width - channel_w);
		}
		else                                // 向左移动
		{
			reg_h_position = 0;
			reg_left_position = - xm_osdlayer->osd_x_position;
			// 应用水平移动最小距离约束
			//reg_left_position = (reg_left_position / align_h) * align_h;
			reg_left_position &= ~  align_h;
			reg_width = reg_width-reg_left_position;
		}

		if(xm_osdlayer->osd_y_position >= 0)
		{
			// 应用垂直方向最小移动距离约束 (下方)
			//reg_v_position = (xm_osdlayer->osd_y_position / align_v) * align_v;
			reg_v_position = xm_osdlayer->osd_y_position &~ align_v;
			//reg_v_position &= ~ align_v;
			reg_y = xm_osdlayer->osd_y_addr;
			reg_u = NULL;
			reg_v = NULL;
			// 修改OSD平面高度
			reg_height = channel_h-reg_v_position;//屏幕高度-坐标高度;减少带宽消耗
		}
		else
		{
			// OSD层的顶部位于显示区的顶部之上 (上方)
			int y_offset = -xm_osdlayer->osd_y_position;
			// 应用垂直方向最小移动距离约束
			//y_offset = (y_offset / align_v) * align_v;
			//y_offset &= ~ align_v;//这句可省略
			reg_v_position = 0;
	//		if( xm_osdlayer->osd_layer_format == XM_OSD_LAYER_FORMAT_ARGB888 )
				reg_y = xm_osdlayer->osd_y_addr + (y_offset) * xm_osdlayer->osd_stride;
	//		else //如果支持RGB565 或 RGB454 再修改 stride，每行的数据量 在各自的格式下不相等
	//			reg_y = xm_osdlayer->osd_y_addr + (y_offset) * xm_osdlayer->osd_stride>>1;
			reg_u = NULL;
			reg_v = NULL;
			// 修改OSD平面高度
			reg_height += xm_osdlayer->osd_y_position;
		}
		// 检查是否应用约束条件来规避OSD硬件的缺陷
	}

	if(reg_width <= 0 || reg_height <= 0 )
	{
		HW_lcdc_osd_set_enable (lcd_channel, osd_layer, 0);
	}
	else
	{
	}

	//检查是否还有显示内容在显示区域
	HW_lcdc_osd_set_width  (lcd_channel, osd_layer, reg_width );
	HW_lcdc_osd_set_stride  (lcd_channel, osd_layer, reg_width );
	HW_lcdc_osd_set_height (lcd_channel, osd_layer, reg_height);
	HW_lcdc_osd_set_h_position (lcd_channel, osd_layer, reg_h_position);
	HW_lcdc_osd_set_v_position (lcd_channel, osd_layer, reg_v_position);
	
	HW_lcdc_osd_set_left_position (lcd_channel, osd_layer, reg_left_position);
	HW_lcdc_osd_set_yuv_addr (lcd_channel, osd_layer, reg_y, reg_u, reg_v);	
	
	if(reg_width <= 0 || reg_height <= 0 )
		;
	else if(xm_osdlayer->osd_enable)
	{
		//XM_printf ("LCD channel(%d)'s OSD layer(%d) ENABLE\n", lcd_channel, osd_layer);
		HW_lcdc_osd_set_enable (lcd_channel, osd_layer, 1);
	}
}

//------------------------------------------------------------------------------
// 配置OSD层的位置信息  限制指定的区域显示
static void osd_layer_setup_position_limit (unsigned int lcd_channel, 
														  unsigned int osd_layer
															  )
{
	unsigned int reg_h_position, reg_v_position, reg_left_position;
	unsigned char *reg_y, *reg_u, *reg_v;
	int reg_height,reg_width;
	unsigned int align_h, align_v;
	//unsigned int mode ,step;
	
	//int channel_w = osd_CHANNEL_width_height[lcd_channel][0] ;//显示通道的宽度高度
	//int channel_h = osd_CHANNEL_width_height[lcd_channel][1] ;
	//以下四元素定义了一个可显示区域，顶点坐标，矩形长宽
	int rec_w ;// 可视区域 宽度
	int rec_h ;// 可视区域 高度
	int rec_x ;// 可视区域 横轴坐标
	int rec_y ;// 可视区域 纵轴坐标

	int x_offset, y_offset;

	XM_OSDLAYER *xm_osdlayer = &lcdc_osd_layer[lcd_channel][osd_layer];
	
	unsigned int osd_layer_rotate = 0;
#ifdef LCD_ROTATE_90
	osd_layer_rotate = 1;
#endif		
	

	reg_width  = xm_osdlayer->osd_width;
	reg_height = xm_osdlayer->osd_height;

	align_h = osd_minimum_moving_h_pixels_clr[xm_osdlayer->osd_layer_format];
	align_v = osd_minimum_moving_v_pixels_clr[xm_osdlayer->osd_layer_format];
	//检查是否已经完全无显示内容在通道
	rec_x = xm_osdlayer->viewable_x;
	rec_y = xm_osdlayer->viewable_y;
	rec_w = xm_osdlayer->viewable_w;
	rec_h = xm_osdlayer->viewable_h;

	x_offset = 0;
	y_offset = 0;
	
	
	if(	xm_osdlayer->osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420
		||	xm_osdlayer->osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		// YUV格式
		if(xm_osdlayer->osd_x_position >= rec_x )
		{//水平
			reg_h_position = xm_osdlayer->osd_x_position;
			reg_h_position &= ~ align_h;
			//reg_left_position = 0;
			x_offset = 0;
			if( (int)(reg_h_position + reg_width) > (rec_x + rec_w) ) // 在 可视区域右侧穿边处理
				reg_width = rec_x + rec_w - reg_h_position;// 如果是负数表示没有可视区域，-->后续处理关闭层
			
				reg_width &= ~ align_h;
		}
		else                              // 屏幕左测
		{
			//水平
			reg_h_position = rec_x;//固定数值  x轴水平位置
			// 穿越屏幕左侧的情况
			//reg_left_position = rec_x- xm_osdlayer->osd_x_position;
			//reg_left_position &= ~ align_h;
			//reg_width = reg_width-reg_left_position;//-rec_x;
			x_offset = rec_x - xm_osdlayer->osd_x_position;
			x_offset &= ~ align_h;
			reg_width = reg_width - x_offset;
		}

		if(xm_osdlayer->osd_y_position >= rec_y )
		{
			//垂直方向  向下
			// OSD层的顶部位于显示区
			// 应用垂直方向最小移动距离约束
			reg_v_position = xm_osdlayer->osd_y_position &~ align_v;
			y_offset = 0;
			/*
			if(reg_v_position == 0 )
			{//特殊情形：清理掉图像显示只有一行，这时数据判断为0行，不做显示。
				HW_lcdc_osd_set_enable (lcd_channel, osd_layer, 0);
				return ;
			}*/
		//	reg_y = xm_osdlayer->osd_y_addr;
		//	reg_u = xm_osdlayer->osd_u_addr;
		//	reg_v = xm_osdlayer->osd_v_addr;
			//reg_height = rec_y+rec_h-xm_osdlayer->osd_y_position;
			if( (int)(rec_y + rec_h) < (int)(reg_v_position + reg_height) )
				reg_height = rec_y + rec_h - reg_v_position;
			
			//reg_v_position = xm_osdlayer->osd_y_position &~ align_v;
			//reg_height = rec_y+rec_h-xm_osdlayer->osd_y_position;
		}
		else
		{
			//垂直方向 向上
			// OSD层的顶部位于显示区的顶部之上

			y_offset = rec_y - xm_osdlayer->osd_y_position;
			// 应用垂直方向最小移动距离约束
			//y_offset = (y_offset / align_v) * align_v;
			y_offset &= ~ align_v;
			reg_v_position = rec_y;
			//reg_y = xm_osdlayer->osd_y_addr + (y_offset) * xm_osdlayer->osd_width;
			//reg_u = xm_osdlayer->osd_u_addr + (y_offset) * xm_osdlayer->osd_width / 4;
			//reg_v = xm_osdlayer->osd_v_addr + (y_offset) * xm_osdlayer->osd_width / 4;

			// 修改OSD平面高度
			reg_height = reg_height + xm_osdlayer->osd_y_position - rec_y;
		}
		// 检查是否应用约束条件来规避OSD硬件的缺陷
	}
	else
	{
		// RGB格式
		if(xm_osdlayer->osd_x_position >= rec_x )// 向右边移动
		{//水平
			reg_h_position = xm_osdlayer->osd_x_position;
			// 应用水平移动最小距离约束
			//reg_h_position = (reg_h_position / align_h) * align_h;
			reg_h_position &= ~ align_h;
			//reg_left_position = 0 ;
			x_offset = 0;
			if( (int)(reg_h_position + reg_width) > (rec_x + rec_w) ) // 在 可视区域右侧穿边处理
				reg_width = rec_x + rec_w - reg_h_position;
			
			reg_width &= ~ align_h;//此条用于避免显示非对齐的情况
		}
		else                                // 向左移动
		{//水平
			reg_h_position = rec_x;//固定数值  x轴水平位置
			// 穿越屏幕左侧的情况
			//reg_left_position = rec_x- xm_osdlayer->osd_x_position;
			//reg_left_position &= ~ align_h;
			//reg_width = reg_width-reg_left_position;//-rec_x;
			x_offset = rec_x - xm_osdlayer->osd_x_position;
			x_offset &= ~ align_h;
			reg_width = reg_width - x_offset;
		}
		if(xm_osdlayer->osd_y_position >= rec_y )
		{//垂直方向  向下
			// 应用垂直方向最小移动距离约束
			//reg_v_position = (xm_osdlayer->osd_y_position / align_v) * align_v;
			reg_v_position = xm_osdlayer->osd_y_position &~ align_v;
			//reg_v_position &= ~ align_v;
		//	reg_y = xm_osdlayer->osd_y_addr;
		//	reg_u = NULL;
		//	reg_v = NULL;
			reg_height = rec_y + rec_h - xm_osdlayer->osd_y_position;
			y_offset = 0;
		}
		else
		{
			//垂直方向  向上
			// OSD层的顶部位于显示区的顶部之上
			y_offset = rec_y - xm_osdlayer->osd_y_position;
			// 应用垂直方向最小移动距离约束
			//y_offset = (y_offset / align_v) * align_v;
			y_offset &= ~ align_v;
			reg_v_position = rec_y;
			//reg_y = xm_osdlayer->osd_y_addr + (y_offset) * xm_osdlayer->osd_stride;
			//reg_u = NULL;
			//reg_v = NULL;
			// 修改OSD平面高度
			reg_height = reg_height + xm_osdlayer->osd_y_position - rec_y;
		}
		// 检查是否应用约束条件来规避OSD硬件的缺陷
	}
	
	//此处已经获得是否有显示内容在层中 。
	if(reg_width <= 0 || reg_height <= 0 )
	{
		HW_lcdc_osd_set_enable (lcd_channel, osd_layer, 0);
	}
	else
	{
	}

	
	if(osd_layer_rotate)
	{
		
		// 根据x_offset, y_offset计算 y/u/v地址
		if(xm_osdlayer->osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420
			||	xm_osdlayer->osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
		{
			// YUV
			reg_left_position = y_offset;
			reg_y = xm_osdlayer->osd_y_addr + (x_offset) * xm_osdlayer->osd_height;
			reg_u = xm_osdlayer->osd_u_addr + (x_offset) * xm_osdlayer->osd_height / 4;
			reg_v = xm_osdlayer->osd_v_addr + (x_offset) * xm_osdlayer->osd_height / 4;
		}
		else
		{
			// RGB
			reg_left_position = y_offset;
			reg_y = xm_osdlayer->osd_y_addr + (x_offset) * xm_osdlayer->osd_height * 4;
			reg_u = NULL;
			reg_v = NULL;
		}
		
		HW_lcdc_osd_set_width  (lcd_channel, osd_layer, reg_height );
		HW_lcdc_osd_set_stride  (lcd_channel, osd_layer, xm_osdlayer->osd_height );
		HW_lcdc_osd_set_height (lcd_channel, osd_layer, reg_width);
		HW_lcdc_osd_set_h_position (lcd_channel, osd_layer, reg_v_position);
		HW_lcdc_osd_set_v_position (lcd_channel, osd_layer, reg_h_position);
		
		HW_lcdc_osd_set_left_position (lcd_channel, osd_layer, reg_left_position);
		HW_lcdc_osd_set_yuv_addr (lcd_channel, osd_layer, reg_y, reg_u, reg_v);	
		
	}
	else
	{
		// 根据x_offset, y_offset计算 y/u/v地址
		if(xm_osdlayer->osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420
			||	xm_osdlayer->osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
		{
			// YUV
			reg_left_position = x_offset;
			reg_y = xm_osdlayer->osd_y_addr + (y_offset) * xm_osdlayer->osd_width;
			reg_u = xm_osdlayer->osd_u_addr + (y_offset) * xm_osdlayer->osd_width / 4;
			reg_v = xm_osdlayer->osd_v_addr + (y_offset) * xm_osdlayer->osd_width / 4;
		}
		else
		{
			// RGB
			reg_left_position = x_offset;
			reg_y = xm_osdlayer->osd_y_addr + (y_offset) * xm_osdlayer->osd_stride;
			reg_u = NULL;
			reg_v = NULL;
		}
		
		//                                              
		HW_lcdc_osd_set_width  (lcd_channel, osd_layer, reg_width );
		HW_lcdc_osd_set_stride  (lcd_channel, osd_layer,  xm_osdlayer->osd_width );
		HW_lcdc_osd_set_height (lcd_channel, osd_layer, reg_height);
		HW_lcdc_osd_set_h_position (lcd_channel, osd_layer, reg_h_position);
		HW_lcdc_osd_set_v_position (lcd_channel, osd_layer, reg_v_position);
		
		HW_lcdc_osd_set_left_position (lcd_channel, osd_layer, reg_left_position);
		HW_lcdc_osd_set_yuv_addr (lcd_channel, osd_layer, reg_y, reg_u, reg_v);	
		
	}
	

	// 在参数设置完毕后使能OSD，防止参数设置过程中导致的非正常显示
	if(reg_width <= 0 || reg_height <= 0 )
	{
	}
	else if(xm_osdlayer->osd_enable)
		HW_lcdc_osd_set_enable (lcd_channel, osd_layer, 1);
}

void XM_lcdc_osd_layer_setup_position(unsigned int lcd_channel, unsigned int osd_layer )
{
	// 检查OSD层是否锁定。若锁定，不执行实际的物理层IO操作
//	if(lcdc_osd_layer[lcd_channel][osd_layer].osd_locked)
//		return;

	//使用可视的显示区域
	if(lcdc_osd_layer[lcd_channel][osd_layer].viewable_en )
		osd_layer_setup_position_limit (lcd_channel, osd_layer);
	else
		osd_layer_setup_position (lcd_channel, osd_layer);
}

// 锁定/解锁OSD层的修改
void XM_lcdc_osd_layer_set_lock (unsigned int lcd_channel, unsigned int osd_layer_mask, unsigned int lock_mask)
{
	unsigned int osd_layer;
	unsigned int unlock_layer = 0;
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
	}

	for (osd_layer = XM_OSD_LAYER_0; osd_layer < XM_OSD_LAYER_COUNT; osd_layer ++)
	{
		if( (1 << osd_layer) & osd_layer_mask )
		{
			// 当前层被选择
			if( (1 << osd_layer) & lock_mask )
			{
				// 该层被锁定
				if(lcdc_osd_locked[lcd_channel][osd_layer])
				{
					//XM_printf ("locked %d\n", lcdc_osd_locked[lcd_channel][osd_layer]);
				}
				lcdc_osd_locked[lcd_channel][osd_layer] ++;
			}
			else
			{
				// 该层被解锁
				if(lcdc_osd_locked[lcd_channel][osd_layer] == 0)
				{
					XM_printf ("OSDC internal error, osd layer(%d) locked count less than 0\n", osd_layer);
				}
				else
				{
					lcdc_osd_locked[lcd_channel][osd_layer] --;
				}

				// 该层锁定次数为0 或者 该层为第0层(视频层)
				if(lcdc_osd_locked[lcd_channel][osd_layer] == 0 || osd_layer == 0)
				{
					unlock_layer |= (1 << osd_layer);
				}
			}
		}
	}

	if(unlock_layer)
	{
		// 存在需要解锁的OSD层
		// 将需要解锁OSD层进行同步
		HW_lcdc_set_osd_coef_syn (lcd_channel, unlock_layer);
	}

	/*

	if(lock)
	{
		lcdc_osd_locked[lcd_channel] ++;
		return;
	}
	else
	{
		if(lcdc_osd_locked[lcd_channel] == 0)
		{
			XM_printf ("OSDC internal error, osd locked count less than 0\n");
			return;
		}
		lcdc_osd_locked[lcd_channel] --;
	}

	if(lcdc_osd_locked[lcd_channel] == 0)
	{
		// 解锁
		if(lcdc_osd_layer[lcd_channel][osd_layer].viewable_en )
			osd_layer_setup_position_limit (lcd_channel, osd_layer);
		else
			osd_layer_setup_position (lcd_channel, osd_layer);
			
		HW_lcdc_set_osd_coef_syn (lcd_channel, 1);

	}
  */
}

// 创建一个OSD逻辑层
// 当layer_enable == 1，该OSD层使能，并输出到相应的LCDC输出通道
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
								)
{
	unsigned int osd_layer_stride = layer_stride;

	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return NULL;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return NULL;
	}
	if(layer_format >= XM_OSD_LAYER_FORMAT_COUNT)
	{
		XM_printf ("illegal layer_format (%d)\n", layer_format);
		return NULL;
	}

	if(layer_enable)
	{
		// 检查参数
		if(	layer_format == XM_OSD_LAYER_FORMAT_YUV420 
			||	layer_format == XM_OSD_LAYER_FORMAT_Y_UV420 )
		{
			// YUV格式
			if(y_addr == NULL || u_addr == NULL)
			{
				XM_printf ("NULL address in YUV format\n");
				return NULL;
			}
			if(v_addr == NULL && layer_format == XM_OSD_LAYER_FORMAT_YUV420)
			{
				XM_printf ("NULL V address in YUV420 format\n");
				return NULL;
			}
			// 检查OSD层尺寸
			if(layer_width > OSD_MAX_YUV_SIZE)
			{
				XM_printf ("osd width(%d) exceed maximum value (%d)\n", layer_width, OSD_MAX_YUV_SIZE);
				return NULL;
			}
			if(layer_height > OSD_MAX_YUV_SIZE)
			{
				XM_printf ("osd height(%d) exceed maximum value (%d)\n", layer_height, OSD_MAX_YUV_SIZE);
				return NULL;
			}

		}
		else 
		{
			// RGB格式
			if(y_addr == NULL)
			{
				XM_printf ("NULL address in RGB format\n");
				return NULL;
			}
			// 检查OSD层尺寸
			if(layer_width > OSD_MAX_RGB_SIZE)
			{
				XM_printf ("osd width(%d) exceed maximum value (%d)\n", layer_width, OSD_MAX_RGB_SIZE);
				return NULL;
			}
			if(layer_height > OSD_MAX_RGB_SIZE)
			{
				XM_printf ("osd height(%d) exceed maximum value (%d)\n", layer_height, OSD_MAX_RGB_SIZE);
				return NULL;
			}
		}
	}

	// 转换stride定义，从行字节长度转换为行像素长度
	if(	layer_format == XM_OSD_LAYER_FORMAT_YUV420 
		|| layer_format == XM_OSD_LAYER_FORMAT_Y_UV420 )
	{
		
	}
	else if(layer_format == XM_OSD_LAYER_FORMAT_ARGB888)
	{
		osd_layer_stride >>= 2;
	}
	else if(layer_format == XM_OSD_LAYER_FORMAT_RGB565)
	{
		osd_layer_stride >>= 1;
	}
	else if(layer_format == XM_OSD_LAYER_FORMAT_ARGB454)
	{
		osd_layer_stride >>= 1;
	}

	lcdc_osd_layer[lcd_channel][osd_layer].osd_layer_format = layer_format;
	lcdc_osd_layer[lcd_channel][osd_layer].osd_stride = layer_stride;
	lcdc_osd_layer[lcd_channel][osd_layer].osd_brightness_coeff = layer_brightness_coeff;
	lcdc_osd_layer[lcd_channel][osd_layer].osd_x_position = layer_offset_x;
	lcdc_osd_layer[lcd_channel][osd_layer].osd_y_position = layer_offset_y;
	lcdc_osd_layer[lcd_channel][osd_layer].osd_width = layer_width;
	lcdc_osd_layer[lcd_channel][osd_layer].osd_height = layer_height;
	lcdc_osd_layer[lcd_channel][osd_layer].osd_global_coeff = layer_global_coeff;
	lcdc_osd_layer[lcd_channel][osd_layer].osd_global_coeff_enable = layer_global_coeff_enable;
	lcdc_osd_layer[lcd_channel][osd_layer].osd_enable = layer_enable;
	//设置可视区域
	lcdc_osd_layer[lcd_channel][osd_layer].viewable_en = 1;
	lcdc_osd_layer[lcd_channel][osd_layer].viewable_x = layer_offset_x;
	lcdc_osd_layer[lcd_channel][osd_layer].viewable_y = layer_offset_y;
	lcdc_osd_layer[lcd_channel][osd_layer].viewable_w = layer_width;
	lcdc_osd_layer[lcd_channel][osd_layer].viewable_h = layer_height;

	lcdc_osd_layer[lcd_channel][osd_layer].osd_y_addr = y_addr;
	lcdc_osd_layer[lcd_channel][osd_layer].osd_u_addr = u_addr;
	lcdc_osd_layer[lcd_channel][osd_layer].osd_v_addr = v_addr;

	lcdc_osd_layer[lcd_channel][osd_layer].osd_h_min_moving_pixels = osd_minimum_moving_h_pixels[layer_format];
	lcdc_osd_layer[lcd_channel][osd_layer].osd_v_min_moving_pixels = osd_minimum_moving_v_pixels[layer_format];

	// 回调函数及参数
	lcdc_osd_layer[lcd_channel][osd_layer].osd_callback = layer_callback;
	lcdc_osd_layer[lcd_channel][osd_layer].osd_private = layer_private;

	// LCDC OSD IO寄存器设置
	HW_lcdc_osd_set_format (lcd_channel, osd_layer, layer_format);
	HW_lcdc_osd_set_brightness_coeff (lcd_channel, osd_layer, layer_brightness_coeff);
	HW_lcdc_osd_set_global_coeff (lcd_channel, osd_layer, layer_global_coeff);
	HW_lcdc_osd_set_global_coeff_enable (lcd_channel, osd_layer, layer_global_coeff_enable);
	
	// 根据相对于显示原点的偏移值，设置YUV地址、offset、h、v等参数
	//osd_layer_setup_position (lcd_channel, osd_layer);
	XM_lcdc_osd_layer_setup_position(lcd_channel, osd_layer);
	
	HW_lcdc_osd_set_callback (lcd_channel, osd_layer, layer_callback, layer_private);

	// HW_lcdc_osd_set_enable (lcd_channel, osd_layer, layer_enable);
	XM_lcdc_osd_set_enable (lcd_channel, osd_layer, layer_enable);

	return &lcdc_osd_layer[lcd_channel][osd_layer];
}

void XM_lcdc_osd_set_enable (unsigned int lcd_channel, unsigned int osd_layer, int enable)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return;
	}

	lcdc_osd_layer[lcd_channel][osd_layer].osd_enable = (unsigned char)enable;
	/*
	// 检查OSD层是否锁定。若锁定，不执行实际的物理层IO操作
	if(lcdc_osd_locked[lcd_channel].osd_locked)
		return;
	*/
	HW_lcdc_osd_set_enable (lcd_channel, osd_layer, enable);

	//XM_printf ("LCD channel(%d)'s OSD layer(%d) %s\n", lcd_channel, osd_layer, enable ? "ENABLE" : "DISABLE");
}

unsigned int XM_lcdc_osd_get_enable (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 0;
	}
	return lcdc_osd_layer[lcd_channel][osd_layer].osd_enable;
}

void XM_lcdc_osd_set_global_coeff_enable (unsigned int lcd_channel, unsigned int osd_layer, unsigned int enable)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return;
	}
	lcdc_osd_layer[lcd_channel][osd_layer].osd_global_coeff_enable = (unsigned char)enable;

	HW_lcdc_osd_set_global_coeff_enable (lcd_channel, osd_layer, enable);
}

unsigned int XM_lcdc_osd_get_global_coeff_enable (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 0;
	}
	return lcdc_osd_layer[lcd_channel][osd_layer].osd_global_coeff_enable;
}

void XM_lcdc_osd_set_global_coeff (unsigned int lcd_channel, unsigned int osd_layer, unsigned int global_coeff)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return;
	}
	if(global_coeff >= 64)
	{
		XM_printf ("illegal global_coeff (%d), exceed maximum value (63)\n", global_coeff);
		return;
	}
	lcdc_osd_layer[lcd_channel][osd_layer].osd_global_coeff = global_coeff;

	HW_lcdc_osd_set_global_coeff (lcd_channel, osd_layer, global_coeff);
}

unsigned int XM_lcdc_osd_get_global_coeff (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 0;
	}
	return lcdc_osd_layer[lcd_channel][osd_layer].osd_global_coeff;
}

void XM_lcdc_osd_set_format (unsigned int lcd_channel, unsigned int osd_layer, unsigned int format)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return;
	}
	if(format >= XM_OSD_LAYER_FORMAT_COUNT)
	{
		XM_printf ("illegal osd_formad (%d)\n", format);
		return;
	}
	lcdc_osd_layer[lcd_channel][osd_layer].osd_layer_format = format;

	HW_lcdc_osd_set_format (lcd_channel, osd_layer, format);
}

unsigned int  XM_lcdc_osd_get_format (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 0;
	}
	return lcdc_osd_layer[lcd_channel][osd_layer].osd_layer_format;
}

void XM_lcdc_osd_set_width (unsigned int lcd_channel, unsigned int osd_layer, unsigned int width)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return;
	}
	if(lcdc_osd_layer[lcd_channel][osd_layer].osd_enable)
	{
		if(	lcdc_osd_layer[lcd_channel][osd_layer].osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420
			||	lcdc_osd_layer[lcd_channel][osd_layer].osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
		{
			if(width > OSD_MAX_YUV_SIZE)
			{
				XM_printf ("osd width(%d) exceed maximum value (%d)\n", width, OSD_MAX_YUV_SIZE);
				return;
			}
		}
		else
		{
			if(width > OSD_MAX_RGB_SIZE)
			{
				XM_printf ("osd width(%d) exceed maximum value (%d)\n", width, OSD_MAX_RGB_SIZE);
				return;
			}
		}
		if(width > HW_lcdc_get_xdots(lcd_channel))
		{
			XM_printf ("illegal osd_width (%d)exceed LCD size(%d) \n", width, HW_lcdc_get_xdots(lcd_channel));
			return;
		}
	}
	lcdc_osd_layer[lcd_channel][osd_layer].osd_width = width;
	
	XM_lcdc_osd_layer_setup_position (lcd_channel, osd_layer);
}

unsigned int XM_lcdc_osd_get_width (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 0;
	}
	return lcdc_osd_layer[lcd_channel][osd_layer].osd_width;
}

void XM_lcdc_osd_set_height (unsigned int lcd_channel, unsigned int osd_layer, unsigned int height)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return;
	}
	if(lcdc_osd_layer[lcd_channel][osd_layer].osd_enable)
	{
		if(	lcdc_osd_layer[lcd_channel][osd_layer].osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420
			||	lcdc_osd_layer[lcd_channel][osd_layer].osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
		{
			if(height > OSD_MAX_YUV_SIZE)
			{
				XM_printf ("osd height(%d) exceed maximum value (%d)\n", height, OSD_MAX_YUV_SIZE);
				return;
			}
		}
		else
		{
			if(height > OSD_MAX_RGB_SIZE)
			{
				XM_printf ("osd height(%d) exceed maximum value (%d)\n", height, OSD_MAX_RGB_SIZE);
				return;
			}
		}
		if(height > HW_lcdc_get_ydots(lcd_channel))
		{
			XM_printf ("illegal osd_height (%d)exceed LCD size(%d) \n", height, HW_lcdc_get_ydots(lcd_channel));
			return;
		}
	}
	lcdc_osd_layer[lcd_channel][osd_layer].osd_height = height;

	XM_lcdc_osd_layer_setup_position (lcd_channel, osd_layer);
}

unsigned int XM_lcdc_osd_get_height (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 0;
	}
	return lcdc_osd_layer[lcd_channel][osd_layer].osd_height;
}

void XM_lcdc_osd_set_x_offset (unsigned int lcd_channel, unsigned int osd_layer, int x_offset)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return;
	}
	if(x_offset > OSD_MAX_H_POSITION)
	{
		XM_printf ("illegal x_offset (%d)\n", x_offset);
		return;
	}
	lcdc_osd_layer[lcd_channel][osd_layer].osd_x_position = x_offset;

	XM_lcdc_osd_layer_setup_position (lcd_channel, osd_layer);
}

int XM_lcdc_osd_get_x_offset (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 0;
	}
	return lcdc_osd_layer[lcd_channel][osd_layer].osd_x_position;
}

void XM_lcdc_osd_set_y_offset (unsigned int lcd_channel, unsigned int osd_layer, int y_offset)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return;
	}
	if(y_offset > OSD_MAX_V_POSITION)
	{
		XM_printf ("illegal y_offset (%d)\n", y_offset);
		return;
	}
	lcdc_osd_layer[lcd_channel][osd_layer].osd_y_position = y_offset;

	XM_lcdc_osd_layer_setup_position (lcd_channel, osd_layer);
}

int XM_lcdc_osd_get_y_offset (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 0;
	}
	return lcdc_osd_layer[lcd_channel][osd_layer].osd_y_position;
}

void XM_lcdc_osd_set_yuv_addr (unsigned int lcd_channel, unsigned int osd_layer, unsigned char *y_addr, unsigned char *u_addr, unsigned char *v_addr)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return;
	}

	lcdc_osd_layer[lcd_channel][osd_layer].osd_y_addr = y_addr;
	lcdc_osd_layer[lcd_channel][osd_layer].osd_u_addr = u_addr;
	lcdc_osd_layer[lcd_channel][osd_layer].osd_v_addr = v_addr;

	XM_lcdc_osd_layer_setup_position (lcd_channel, osd_layer);
}

unsigned char * XM_lcdc_osd_get_y_addr (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return NULL;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return NULL;
	}
	return lcdc_osd_layer[lcd_channel][osd_layer].osd_y_addr;
}

unsigned char * XM_lcdc_osd_get_u_addr (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return NULL;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return NULL;
	}
	return lcdc_osd_layer[lcd_channel][osd_layer].osd_u_addr;
}

unsigned char * XM_lcdc_osd_get_v_addr (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return NULL;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return NULL;
	}
	return lcdc_osd_layer[lcd_channel][osd_layer].osd_v_addr;
}



void XM_lcdc_osd_set_stride (unsigned int lcd_channel, unsigned int osd_layer, unsigned int layer_stride)
{
	unsigned int osd_layer_stride = layer_stride;
	unsigned int layer_format;
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return;
	}
	lcdc_osd_layer[lcd_channel][osd_layer].osd_stride = layer_stride;
	
	layer_format = lcdc_osd_layer[lcd_channel][osd_layer].osd_layer_format;
	if(layer_format == XM_OSD_LAYER_FORMAT_ARGB888)
		osd_layer_stride >>= 2;
	else if(layer_format == XM_OSD_LAYER_FORMAT_RGB565)
		osd_layer_stride >>= 1;
	else if(layer_format == XM_OSD_LAYER_FORMAT_ARGB454)
		osd_layer_stride >>= 1;
	
	XM_lcdc_osd_layer_setup_position (lcd_channel, osd_layer);
}

unsigned int XM_lcdc_osd_get_stride (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 0;
	}
	return lcdc_osd_layer[lcd_channel][osd_layer].osd_stride;
}

void XM_lcdc_osd_set_brightness_coeff (unsigned int lcd_channel, unsigned int osd_layer, 
													unsigned int brightness_coeff)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return;
	}
	lcdc_osd_layer[lcd_channel][osd_layer].osd_brightness_coeff = brightness_coeff;

	HW_lcdc_osd_set_brightness_coeff (lcd_channel, osd_layer, brightness_coeff);
}

unsigned int XM_lcdc_osd_get_brightness_coeff (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 0;
	}
	return lcdc_osd_layer[lcd_channel][osd_layer].osd_brightness_coeff;

}

// 设置OSD层回调函数
void XM_lcdc_osd_set_callback (unsigned int lcd_channel, unsigned int osd_layer, 
										 osd_callback_t osd_callback, void *osd_private)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return;
	}

	lcdc_osd_layer[lcd_channel][osd_layer].osd_callback = osd_callback;
	lcdc_osd_layer[lcd_channel][osd_layer].osd_private = osd_private;

	HW_lcdc_osd_set_callback (lcd_channel, osd_layer, osd_callback, osd_private);
}

// 设置OSD逻辑层的可视区域
void XM_lcdc_osd_set_viewable_width (unsigned int lcd_channel, unsigned int osd_layer, unsigned int width)
{
	if(!is_legal_osd_layer (lcd_channel, osd_layer))
		return;
	lcdc_osd_layer[lcd_channel][osd_layer].viewable_w = width;

	XM_lcdc_osd_layer_setup_position (lcd_channel, osd_layer);
}

unsigned int XM_lcdc_osd_get_viewable_width (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(!is_legal_osd_layer (lcd_channel, osd_layer))
		return 0;
	return lcdc_osd_layer[lcd_channel][osd_layer].viewable_w;
}

void XM_lcdc_osd_set_viewable_height (unsigned int lcd_channel, unsigned int osd_layer, unsigned int height)
{
	if(!is_legal_osd_layer (lcd_channel, osd_layer))
		return;
	lcdc_osd_layer[lcd_channel][osd_layer].viewable_h = height;

	XM_lcdc_osd_layer_setup_position (lcd_channel, osd_layer);
}

unsigned int XM_lcdc_osd_get_viewable_height (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(!is_legal_osd_layer (lcd_channel, osd_layer))
		return 0;
	return lcdc_osd_layer[lcd_channel][osd_layer].viewable_h;
}

// 设置OSD逻辑层可视区域的偏移位置(相对视频显示通道的左上角)
void XM_lcdc_osd_set_viewable_x_offset (unsigned int lcd_channel, unsigned int osd_layer, int x_offset)
{
	if(!is_legal_osd_layer (lcd_channel, osd_layer))
		return;
	lcdc_osd_layer[lcd_channel][osd_layer].viewable_x = x_offset;

	XM_lcdc_osd_layer_setup_position (lcd_channel, osd_layer);
}

int XM_lcdc_osd_get_viewable_x_offset (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(!is_legal_osd_layer (lcd_channel, osd_layer))
		return 0;
	return lcdc_osd_layer[lcd_channel][osd_layer].viewable_x;
}

void XM_lcdc_osd_set_viewable_y_offset (unsigned int lcd_channel, unsigned int osd_layer, int y_offset)
{
	if(!is_legal_osd_layer (lcd_channel, osd_layer))
		return;
	lcdc_osd_layer[lcd_channel][osd_layer].viewable_y = y_offset;

	XM_lcdc_osd_layer_setup_position (lcd_channel, osd_layer);
}

int XM_lcdc_osd_get_viewable_y_offset (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(!is_legal_osd_layer (lcd_channel, osd_layer))
		return 0;
	return lcdc_osd_layer[lcd_channel][osd_layer].viewable_y;
}

// 使能或禁止OSD逻辑层的可视区域
void XM_lcdc_osd_set_viewable_enable (unsigned int lcd_channel, unsigned int osd_layer, int enable)
{
	if(!is_legal_osd_layer (lcd_channel, osd_layer))
		return;
	lcdc_osd_layer[lcd_channel][osd_layer].viewable_en = enable;

	XM_lcdc_osd_layer_setup_position (lcd_channel, osd_layer);
}

unsigned int XM_lcdc_osd_get_viewable_enable (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(!is_legal_osd_layer (lcd_channel, osd_layer))
		return 0;
	return lcdc_osd_layer[lcd_channel][osd_layer].viewable_en;
}

void _imp_lcdc_copy_raw_image_ARGB888 (unsigned char *src, 
													unsigned char *dst,
													unsigned int copy_image_w,					// 待复制区域的像素宽度、像素高度		
													unsigned int copy_image_h,
													unsigned int raw_image_stride,			// 源图像的行字节长度	
													unsigned int osd_layer_stride,			// 目标OSD的行字节长度
																										//		YUV420格式表示Y分量，UV分量除以2
													unsigned int osd_layer_height,													
													unsigned int osd_layer_rotate,			// 90度旋转													
													unsigned int layer_alpha					// OSD层的全局Alpha因子
													)
{
	unsigned int i, j;
		for (j = 0; j < copy_image_h; j ++)
		{
			unsigned char *s, *d;
			s = src;
			d = dst;
			for (i = 0; i < copy_image_w; i ++)
			{
				unsigned int s_argb = *(unsigned int *)s;
				unsigned int s_a = s_argb >> 24;
				if(s_a == 0)
				{
					// 全透明
				}
				else if(s_a == 0xFF)
				{
					// 全覆盖
					*(unsigned int *)d = (unsigned int)( (s_argb & 0x00FFFFFF) | (layer_alpha << 24));
				}
				else
				{
					//unsigned char s_r = (unsigned char)(s_argb >> 16);
					//unsigned char s_g = (unsigned char)(s_argb >> 8);
					//unsigned char s_b = (unsigned char)(s_argb >> 0);
					unsigned int d_argb = *(unsigned int *)d;
					//unsigned int d_a = (d_argb >> 24);	// 使用背景的Alpha
					//unsigned int d_r = ((d_argb >> 16) & 0xFF);
					//unsigned int d_g = ((d_argb >> 8 ) & 0xFF);
					//unsigned int d_b = (d_argb & 0xFF) ;
					unsigned int rev_s_a;
					//unsigned int d_a, d_r, d_g, d_b;
					unsigned int d_data;
					s_a = (s_a * layer_alpha >> 8);
					rev_s_a = 255 - s_a;

					//d_a = (( d_argb >> 24)         * rev_s_a +   s_a * s_a) >> 8;
					//d_r = (((d_argb >> 16) & 0xFF) * rev_s_a + ((s_argb >> 16) & 0xFF) * s_a) >> 8;
					//d_g = (((d_argb >> 8 ) & 0xFF) * rev_s_a + ((s_argb >>  8) & 0xFF) * s_a) >> 8;
					//d_b = (( d_argb        & 0xFF) * rev_s_a + ((s_argb      ) & 0xFF) * s_a) >> 8;
					//*(unsigned int *)d = (unsigned int)((d_r << 16) | (d_g << 8) | d_b | (d_a << 24));

					d_data  = (( d_argb >> 24)         * rev_s_a +   s_a * s_a) >> 8;
					d_data <<= 8;
					d_data |= (((d_argb >> 16) & 0xFF) * rev_s_a + ((s_argb >> 16) & 0xFF) * s_a) >> 8;
					d_data <<= 8;
					d_data |= (((d_argb >> 8 ) & 0xFF) * rev_s_a + ((s_argb >>  8) & 0xFF) * s_a) >> 8;
					d_data <<= 8;
					d_data |= (( d_argb        & 0xFF) * rev_s_a + ((s_argb      ) & 0xFF) * s_a) >> 8;
					*(unsigned int *)d = d_data;
				}
				s += 4;
				
				if(osd_layer_rotate)
					d += osd_layer_height * 4;
				else
					d += 4;
			}

#if VERT_REVERSE
			src -= raw_image_stride;
#else
			src += raw_image_stride;
#endif
			if(osd_layer_rotate)
				dst += 4;
			else
				dst += osd_layer_stride;
		}
}

// 将非压缩的RAW格式图像数据加载到相同显示格式的OSD层
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
									)
{
	unsigned int i, j;
	unsigned char *src, *dst;
	unsigned int osd_layer_rotate = 0;
	
#ifdef LCD_ROTATE_90
	osd_layer_rotate = 1;
#endif

	if(osd_layer_buffer == NULL)
		return 0;
	if(osd_layer_format >= XM_OSD_LAYER_FORMAT_COUNT)
	{
		XM_printf ("illegal osd_format (%d)\n", osd_layer_format);
		return 0;
	}
	if(osd_layer_width == 0 || osd_layer_height == 0)
		return 0;
	if(osd_layer_stride < osd_layer_width)
		return 0;
	if(osd_offset_x >= osd_layer_width)
		return 0;
	if(osd_offset_y >= osd_layer_height)
		return 0;
	
	if(raw_image_buffer == NULL)
		return 0;
	if(raw_image_width == 0 || raw_image_height == 0)
		return 0;
	if(raw_image_stride < raw_image_width)
		return 0;
	if(raw_offset_x >= raw_image_width)
		return 0;
	if(raw_offset_y >= raw_image_height)
		return 0;

	// 计算有效的复制区域
	if(copy_image_w > (osd_layer_width - osd_offset_x))
		copy_image_w = osd_layer_width - osd_offset_x;
	if(copy_image_w > (raw_image_width - raw_offset_x))
		copy_image_w = raw_image_width - raw_offset_x;
	if(copy_image_h > (osd_layer_height - osd_offset_y))
		copy_image_h = osd_layer_height - osd_offset_y;
	if(copy_image_h > (raw_image_height - raw_offset_y))
		copy_image_h = raw_image_height - raw_offset_y;
	
#if VERT_REVERSE
	osd_offset_y = osd_layer_height - 1 - (osd_offset_y + copy_image_h);
#endif	

	layer_alpha &= 0x000000FF;

	if(osd_layer_format == XM_OSD_LAYER_FORMAT_ARGB888)
	{
		// ARGB888格式
#if VERT_REVERSE
		src = (unsigned char *)(raw_image_buffer + (raw_image_height - 1 - raw_offset_y) * raw_image_stride + raw_offset_x * 4);
#else
		src = (unsigned char *)(raw_image_buffer + raw_offset_y * raw_image_stride + raw_offset_x * 4);
#endif
		if(osd_layer_rotate)
		{
			dst = (unsigned char *)(osd_layer_buffer + osd_offset_y * 4 + osd_offset_x * 4 * osd_layer_height);
		}
		else
			dst = (unsigned char *)(osd_layer_buffer + osd_offset_y * osd_layer_stride + osd_offset_x * 4);

#if 1
		_imp_lcdc_copy_raw_image_ARGB888 (src, dst, copy_image_w, copy_image_h,
													raw_image_stride, osd_layer_stride, osd_layer_height, osd_layer_rotate, layer_alpha);
#else
		for (j = 0; j < copy_image_h; j ++)
		{
			unsigned char *s, *d;
			s = src;
			d = dst;
			for (i = 0; i < copy_image_w; i ++)
			{
				unsigned int s_argb = *(unsigned int *)s;
				unsigned int s_a = s_argb >> 24;
				if(s_a == 0)
				{
					// 全透明
				}
				else if(s_a == 0xFF)
				{
					// 全覆盖
					*(unsigned int *)d = (unsigned int)( (s_argb & 0x00FFFFFF) | (layer_alpha << 24));
				}
				else
				{
					//unsigned char s_r = (unsigned char)(s_argb >> 16);
					//unsigned char s_g = (unsigned char)(s_argb >> 8);
					//unsigned char s_b = (unsigned char)(s_argb >> 0);
					unsigned int d_argb = *(unsigned int *)d;
					//unsigned int d_a = (d_argb >> 24);	// 使用背景的Alpha
					//unsigned int d_r = ((d_argb >> 16) & 0xFF);
					//unsigned int d_g = ((d_argb >> 8 ) & 0xFF);
					unsigned int d_b = (d_argb & 0xFF) ;
					unsigned int rev_s_a;
					//unsigned int d_a, d_r, d_g, d_b;
					unsigned int d_data;
					s_a = (s_a * layer_alpha >> 8);
					rev_s_a = 255 - s_a;

					//d_a = (( d_argb >> 24)         * rev_s_a +   s_a * s_a) >> 8;
					//d_r = (((d_argb >> 16) & 0xFF) * rev_s_a + ((s_argb >> 16) & 0xFF) * s_a) >> 8;
					//d_g = (((d_argb >> 8 ) & 0xFF) * rev_s_a + ((s_argb >>  8) & 0xFF) * s_a) >> 8;
					//d_b = (( d_argb        & 0xFF) * rev_s_a + ((s_argb      ) & 0xFF) * s_a) >> 8;
					//*(unsigned int *)d = (unsigned int)((d_r << 16) | (d_g << 8) | d_b | (d_a << 24));

					d_data  = (( d_argb >> 24)         * rev_s_a +   s_a * s_a) >> 8;
					d_data <<= 8;
					d_data |= (((d_argb >> 16) & 0xFF) * rev_s_a + ((s_argb >> 16) & 0xFF) * s_a) >> 8;
					d_data <<= 8;
					d_data |= (((d_argb >> 8 ) & 0xFF) * rev_s_a + ((s_argb >>  8) & 0xFF) * s_a) >> 8;
					d_data <<= 8;
					d_data |= (( d_argb        & 0xFF) * rev_s_a + ((s_argb      ) & 0xFF) * s_a) >> 8;
					*(unsigned int *)d = d_data;
				}
				s += 4;
				if(osd_layer_rotate)
				{
					d += osd_layer_height * 4;
				}
				else
					d += 4;
			}

#if VERT_REVERSE
			src -= raw_image_stride;
#else
			src += raw_image_stride;
#endif
			if(osd_layer_rotate)
			{
				dst += 4;
			}
			else	
				dst += osd_layer_stride;
		}
#endif
	}
	else if(osd_layer_format == XM_OSD_LAYER_FORMAT_ARGB454)
	{
		layer_alpha >>= 5;
		src = (unsigned char *)(raw_image_buffer + raw_offset_y * raw_image_stride + raw_offset_x * 2);
		dst = (unsigned char *)(osd_layer_buffer + osd_offset_y * osd_layer_stride + osd_offset_x * 2);
		for (j = 0; j < copy_image_h; j ++)
		{
			unsigned char *s, *d;
			s = src;
			d = dst;
			for (i = 0; i < copy_image_w; i ++)
			{
				unsigned int s_argb = *(unsigned short *)s;
				unsigned char s_a = (unsigned char)(s_argb >> 13);
				if(s_a == 0)
				{
					// 全透明
				}
				else if(s_a == 0x07)
				{
					// 全覆盖
					*(unsigned short *)d = (unsigned short)( (s_argb & 0x1FFF) | (layer_alpha  << 13));
				}
				else
				{
					unsigned char s_r = (unsigned char)((s_argb >> 11) << 3);
					unsigned char s_g = (unsigned char)((s_argb & 0x07E0) >> 3);
					unsigned char s_b = (unsigned char)((s_argb & 0x001F) << 3);
					unsigned int d_r, d_g, d_b; 
					unsigned int d_argb = *(unsigned short *)d;
					unsigned int d_a = (d_argb >> 13);	// 使用背景的Alpha

					s_a = (unsigned char)((s_a * layer_alpha) / 7);
					d_r = ( ((d_argb & 0x1E00) >> 5) * (7 - s_a) + s_r * s_a) / 7;
					d_g = ( ((d_argb >> 8 ) & 0xFF) * (7 - s_a) + s_g * s_a) / 7;
					d_b = ( (d_argb & 0xFF)  * (7 - s_a) + s_b * s_a) / 7;
					d_a = (s_a * s_a  + (7 - s_a) * d_a) / 7;
					*(unsigned short *)d = (unsigned short)(((d_r & 0xF0) << 5) | ((d_g & 0xF8) << 1) | (d_b >> 4) | (d_a << 13));
				}
				s += 2;
				d += 2;
			}

			src += raw_image_stride;
			dst += osd_layer_stride;
		}
	}

	return 1;
}

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
									)
{
	unsigned int i, j;
	unsigned char *src, *dst;

	if(osd_layer_buffer == NULL)
		return 0;
	if(osd_layer_format >= XM_OSD_LAYER_FORMAT_COUNT)
	{
		XM_printf ("illegal osd_format (%d)\n", osd_layer_format);
		return 0;
	}
	if(osd_layer_width == 0 || osd_layer_height == 0)
		return 0;
	if(osd_layer_stride < osd_layer_width)
		return 0;
	if(osd_offset_x >= osd_layer_width)
		return 0;
	if(osd_offset_y >= osd_layer_height)
		return 0;
	
	if(raw_image_buffer == NULL)
		return 0;
	if(raw_image_width == 0 || raw_image_height == 0)
		return 0;
	if(raw_image_stride < raw_image_width)
		return 0;
	if(raw_offset_x >= raw_image_width)
		return 0;
	if(raw_offset_y >= raw_image_height)
		return 0;

	// 计算有效的复制区域
	if(copy_image_w > (osd_layer_width - osd_offset_x))
		copy_image_w = osd_layer_width - osd_offset_x;
	if(copy_image_w > (raw_image_width - raw_offset_x))
		copy_image_w = raw_image_width - raw_offset_x;
	if(copy_image_h > (osd_layer_height - osd_offset_y))
		copy_image_h = osd_layer_height - osd_offset_y;
	if(copy_image_h > (raw_image_height - raw_offset_y))
		copy_image_h = raw_image_height - raw_offset_y;

	layer_alpha &= 0x000000FF;

	if(osd_layer_format == XM_OSD_LAYER_FORMAT_ARGB888)
	{
		// ARGB888格式
		src = (unsigned char *)(raw_image_buffer + raw_offset_y * raw_image_stride + raw_offset_x * 4);
		dst = (unsigned char *)(osd_layer_buffer + osd_offset_y * osd_layer_stride + osd_offset_x * 4);
		for (j = 0; j < copy_image_h; j ++)
		{
			unsigned char *s, *d;
			s = src;
			d = dst;
			for (i = 0; i < copy_image_w; i ++)
			{
				unsigned int s_argb = *(unsigned int *)s;
				unsigned char s_a = (unsigned char)(s_argb >> 24);
				if(s_a == 0)
				{
					// 全透明
				}
				else //if(s_a == 0xFF)
				{
					// 全覆盖
					*(unsigned int *)d = (unsigned int)( (s_argb & 0x00FFFFFF) | (layer_alpha << 24));
				}
				/*else
				{
					unsigned char s_r = (unsigned char)(s_argb >> 16);
					unsigned char s_g = (unsigned char)(s_argb >> 8);
					unsigned char s_b = (unsigned char)(s_argb >> 0);
					unsigned int d_argb = *(unsigned int *)d;
					unsigned int d_a = (d_argb >> 24);	// 使用背景的Alpha
					unsigned int d_r = ((d_argb >> 16) & 0xFF);
					unsigned int d_g = ((d_argb >> 8 ) & 0xFF);
					unsigned int d_b = (d_argb & 0xFF) ;
					s_a = (unsigned char)(s_a * layer_alpha >> 8);

					d_a = (d_a * (255 - s_a) + s_a * s_a) >> 8;
					d_r = (d_r * (255 - s_a) + s_r * s_a) >> 8;
					d_g = (d_g * (255 - s_a) + s_g * s_a) >> 8;
					d_b = (d_b * (255 - s_a) + s_b * s_a) >> 8;
					*(unsigned int *)d = (unsigned int)((d_r << 16) | (d_g << 8) | d_b | (d_a << 24));
				}*/
				s += 4;
				d += 4;
			}

			src += raw_image_stride;
			dst += osd_layer_stride;
		}
	}
	else if(osd_layer_format == XM_OSD_LAYER_FORMAT_ARGB454)
	{
		layer_alpha >>= 5;
		src = (unsigned char *)(raw_image_buffer + raw_offset_y * raw_image_stride + raw_offset_x * 2);
		dst = (unsigned char *)(osd_layer_buffer + osd_offset_y * osd_layer_stride + osd_offset_x * 2);
		for (j = 0; j < copy_image_h; j ++)
		{
			unsigned char *s, *d;
			s = src;
			d = dst;
			for (i = 0; i < copy_image_w; i ++)
			{
				unsigned int s_argb = *(unsigned short *)s;
				unsigned char s_a = (unsigned char)(s_argb >> 13);
				if(s_a == 0)
				{
					// 全透明
				}
				else if(s_a == 0x07)
				{
					// 全覆盖
					*(unsigned short *)d = (unsigned short)( (s_argb & 0x1FFF) | (layer_alpha  << 13));
				}
				else
				{
					unsigned char s_r = (unsigned char)((s_argb >> 11) << 3);
					unsigned char s_g = (unsigned char)((s_argb & 0x07E0) >> 3);
					unsigned char s_b = (unsigned char)((s_argb & 0x001F) << 3);
					unsigned int d_r, d_g, d_b; 
					unsigned int d_argb = *(unsigned short *)d;
					unsigned int d_a = (d_argb >> 13);	// 使用背景的Alpha

					s_a = (unsigned char)((s_a * layer_alpha) / 7);
					d_r = ( ((d_argb & 0x1E00) >> 5) * (7 - s_a) + s_r * s_a) / 7;
					d_g = ( ((d_argb >> 8 ) & 0xFF) * (7 - s_a) + s_g * s_a) / 7;
					d_b = ( (d_argb & 0xFF)  * (7 - s_a) + s_b * s_a) / 7;
					d_a = (s_a * s_a  + (7 - s_a) * d_a) / 7;
					*(unsigned short *)d = (unsigned short)(((d_r & 0xF0) << 5) | ((d_g & 0xF8) << 1) | (d_b >> 4) | (d_a << 13));
				}
				s += 2;
				d += 2;
			}

			src += raw_image_stride;
			dst += osd_layer_stride;
		}
	}

	return 1;
}

// 将PNG格式的文件显示在OSD Layer的指定位置
// osd_layer_buffer    OSD layer的原点缓冲区指针
// osd_layer_format    OSD layer的格式(ARGB8888/RGB565/ARGB3454/YUV420)
// osd_layer_width     OSD layer的像素宽度
// osd_layer_height    OSD layer的像素高度
// osd_layer_stride    OSD layer的行字节宽度
// osd_image_x         PNG image的显示坐标(相对于OSD层原点的偏移，>=0 )
// osd_image_y
// png_image	        PNG image数据缓冲区
// png_image_size		  PNG image数据字节长度
// transparent_color   定义PNG image的透明色处理
//                     (unsigned int)(0xFFFFFFFF)  透明色
//                     作为背景色填充在OSD layer指定的位置
int  XM_lcdc_load_png_image (unsigned char *osd_layer_buffer, 
									unsigned int osd_layer_format,
									unsigned int osd_layer_width, unsigned int osd_layer_height,
									unsigned int osd_layer_stride,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *png_image_buff, unsigned int png_image_size,
									XM_COLORREF transparent_color,
									unsigned char layer_alpha)
{
	int ret;
	png_image image; /* The control structure used by libpng */
	png_bytep buffer;
	//png_color color;	// 背景色填充
	//png_color *background;
	unsigned int osd_layer_rotate = 0;
	
#ifdef LCD_ROTATE_90
	osd_layer_rotate = 1;	// 旋转90度
#endif

	// 检查OSD Layer的参数
	if(osd_layer_buffer == NULL)
	{
		XM_printf ("XM_lcdc_osd_load_png_image NG, osd_layer_buffer==NULL\n");
		return 0;
	}

	// 仅支持RGB格式
	if(	osd_layer_format != XM_OSD_LAYER_FORMAT_ARGB888
		//&&	osd_layer_format != XM_OSD_LAYER_FORMAT_RGB565
		//&& osd_layer_format != XM_OSD_LAYER_FORMAT_ARGB454
		)
	{
		XM_printf ("XM_lcdc_osd_load_png_image NG, osd_layer_format(%d) illegal\n", osd_layer_format);
		return 0;
	}

	// 检查区域是否可视
	if(osd_image_x >= osd_layer_width)
	{
		XM_printf ("XM_lcdc_osd_load_png_image NG, osd_image_x(%d) >= osd_layer_width(%d)\n", 
			osd_image_x, osd_layer_width);
		return 0;
	}
	if(osd_image_y >= osd_layer_height)
	{
		XM_printf ("XM_lcdc_osd_load_png_image NG, osd_image_y(%d) >= osd_layer_height(%d)\n", 
			osd_image_y, osd_layer_height);
		return 0;
	}

	if(png_image_buff == NULL)
		return 0;

	/* Initialize the 'png_image' structure. */
	memset(&image, 0, (sizeof image));
	image.version = PNG_IMAGE_VERSION;
	ret = 0;
	//background = NULL;

	// 读取PNG图像的格式信息
	if (png_image_begin_read_from_memory(&image, png_image_buff, png_image_size) == 0)
	{
		return 0;
	}

	do
	{
		int pixel_bytes;
		// 检查PNG图像的尺寸是否超出定义的OSD显示区域
		/*
		if(image.width > (osd_layer_width - osd_image_x))
		{
			XM_printf ("XM_lcdc_osd_load_png_image NG, image.width(%d) > (osd_layer_width - osd_image_x)(%d)\n", 
				image.width, (osd_layer_width - osd_image_x));
			break;
		}
		if(image.height > (osd_layer_height - osd_image_y))
		{
			XM_printf ("XM_lcdc_osd_load_png_image NG, image.height(%d) > (osd_layer_height - osd_image_y)(%d)\n", 
				image.height, (osd_layer_height - osd_image_y));
			break;
		}*/

		// 定义PNG图像解码的格式
		// image.format = PNG_FORMAT_ARGB;
		image.format = PNG_FORMAT_BGRA;
		// image.format = PNG_FORMAT_BGRA|PNG_FORMAT_FLAG_ALPHA;
		
		if(osd_layer_format == XM_OSD_LAYER_FORMAT_ARGB888)
			pixel_bytes = 4;
		else
			pixel_bytes = 2;
		// RGB565, ARGB454
		// 分配临时的缓冲区，保存解码后的ARGB数据
		buffer = (unsigned char *)XM_heap_malloc(PNG_IMAGE_SIZE(image));
		if(buffer == NULL)
		{
			XM_printf ("XM_lcdc_load_png_image NG, XM_heap_malloc size=%d failed\n", PNG_IMAGE_SIZE(image));
			break;
		}
				
		if(png_image_finish_read(&image, NULL/*background*/, 
			buffer, image.width * 4/*row_stride*/, NULL/*colormap*/))
		{
			unsigned int h, w;
#if VERT_REVERSE
			unsigned char *src = (unsigned char *)buffer + (image.height - 1) * image.width * 4;
#else
			unsigned char *src = (unsigned char *)buffer;
#endif
			unsigned char *dst;
			unsigned int viewable_width, viewable_height;
			
#if VERT_REVERSE
			osd_image_y = osd_layer_height - 1 - (osd_image_y + image.height);
#endif			

			// 计算可视的图像宽/高度
			viewable_width = osd_layer_width - osd_image_x;
			if(viewable_width > image.width)
				viewable_width = image.width;
			viewable_height = osd_layer_height - osd_image_y;
			if(viewable_height > image.height)
				viewable_height = image.height;

			if(osd_layer_rotate)
			{
				osd_layer_buffer += osd_image_x * 4 * osd_layer_height;
			}
			else
			{
				osd_layer_buffer += osd_image_y  * osd_layer_stride;
			}
			for (h = 0; h < viewable_height; h++)
			{
				unsigned char *argb = src;
				if(osd_layer_rotate)		// 旋转90度
				{
					dst = (unsigned char *)osd_layer_buffer;
					dst += osd_image_y * 4;
				}
				else
				{
					dst = (unsigned char *)osd_layer_buffer;
					dst += osd_image_x * pixel_bytes;		// RGB565, ARGB454
				}
				for (w = 0; w < viewable_width; w++)
				{
					//unsigned char *argb = src + h * image.width * 4 + w * 4;
					// aarrggbb
					unsigned int b = *argb ++;	// alpha
					unsigned int g = *argb ++;
					unsigned int r = *argb ++;
					unsigned int a = *argb ++;
					if(a == 0)
					{
						// 全透
					}
					else if(a == 0xFF)
					{
						// 非透明 
						if(osd_layer_format == XM_OSD_LAYER_FORMAT_ARGB888)
						{
							// AAAAAAAARRRRRRRRGGGGGGGGBBBBBBBB
							// *(unsigned int *)dst = (unsigned int)((r << 16) | (g << 8) | b | (a << 24));
							*(unsigned int *)dst = (unsigned int)((r << 16) | (g << 8) | b | (layer_alpha << 24));
						}
						else if(osd_layer_format == XM_OSD_LAYER_FORMAT_RGB565)
						{
							// RRRRRGGGGGGBBBBB
							*(unsigned short *)dst = (unsigned short)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
						}
						else //if(osd_layer_format == XM_OSD_LAYER_FORMAT_ARGB454)
						{
							// AAARRRRGGGGGBBBB
							// *(unsigned short *)dst = (unsigned short)(((r & 0xF0) << 5) | ((g & 0xF8) << 1) | (b >> 4) | 0xE000);
							*(unsigned short *)dst = (unsigned short)(((r & 0xF0) << 5) | ((g & 0xF8) << 1) | (b >> 4) 
															| ((layer_alpha << 8) & 0xE000) );
						}
					}
					else
					{
						// Alpha混合
						unsigned int a_r, a_g, a_b; 
						// a = a * layer_alpha / 255;
						a = a * layer_alpha >> 8;
						if(osd_layer_format == XM_OSD_LAYER_FORMAT_ARGB888)
						{
							unsigned int d_rgb = *(unsigned int *)dst;
							unsigned int a_a = (d_rgb >> 24);	// 使用背景的Alpha
							// a_a = (a * a + (255 - a) * a_a) / 255;
							a_a = (a * a  + (255 - a) * a_a) >> 8;
							// a_r = ( ((d_rgb >> 16) & 0xFF) * (255 - a) + r * a) / 255;
							a_r = ( ((d_rgb >> 16) & 0xFF) * (255 - a) + r * a) >> 8;
							// a_g = ( ((d_rgb >> 8 ) & 0xFF) * (255 - a) + g * a) / 255;
							a_g = ( ((d_rgb >> 8 ) & 0xFF) * (255 - a) + g * a) >> 8;
							// a_b = ( (d_rgb & 0xFF)  * (255 - a) + b * a) / 255;
							a_b = ( (d_rgb & 0xFF)  * (255 - a) + b * a) >> 8;
							*(unsigned int *)dst = (unsigned int)((a_r << 16) | (a_g << 8) | a_b | (a_a << 24));
						}
						else if(osd_layer_format == XM_OSD_LAYER_FORMAT_RGB565)
						{
							// RRRRRGGGGGGBBBBB
							unsigned short d_rgb = *(unsigned short *)dst;
							a_r = (((d_rgb >> 11) << 3) * (255 - a) + r * a) / 255;
							a_g = (((d_rgb & 0x07E0) >> 3) * (255 - a) + g * a) / 255;
							a_b = (((d_rgb & 0x001F) << 3) * (255 - a) + b * a) / 255;
							*(unsigned short *)dst = (unsigned short)(((a_r & 0xF8) << 8) | ((a_g & 0xFC) << 3) | (a_b >> 3));
						}
						else //if(osd_layer_format == XM_OSD_LAYER_FORMAT_ARGB454)
						{
							// AAARRRRGGGGGBBBB
							unsigned short d_rgb = *(unsigned short *)dst;
							unsigned int a_a = ((d_rgb & 0xE000) >> 13) << 5;	// 使用背景的Alpha
							a_a = (a * a  + (255 - a) * a_a) / 255;
							a_a = (a_a >> 5) << 13;
							a_r = (((d_rgb & 0x1E00) >> 5) * (255 - a) + r * a) / 255;
							a_g = (((d_rgb & 0x01F0) >> 1) * (255 - a) + g * a) / 255;
							a_b = (((d_rgb & 0x000F) << 4) * (255 - a) + b * a) / 255;
							*(unsigned short *)dst = (unsigned short)(((a_r & 0xF0) << 5) | ((a_g & 0xF8) << 1) | (a_b >> 4) | a_a);
						}
					}
					
					if(osd_layer_rotate)
						dst += osd_layer_height * 4;
					else
						dst += pixel_bytes;
				}

#if VERT_REVERSE
				src -= image.width * 4;
#else
				src += image.width * 4;
#endif
				if(osd_layer_rotate)
					osd_layer_buffer += 4;		// 仅支持ARGB888
				else
					osd_layer_buffer += osd_layer_stride;
			}
			
			ret = 1;
		}
		XM_heap_free (buffer);
		break;
	} while(osd_layer_buffer);

	// 释放png解码库分配的资源
	png_image_free (&image);

	return ret;
}

// 将PNG格式的文件显示在OSD Layer的指定位置
// lcd_channel			  LCDC通道号
// osd_layer			  OSD层序号
// osd_image_x         PNG image的显示坐标(相对于OSD层原点的偏移，>=0 )
// osd_image_y
// png_image		     PNG image的数据缓冲
// png_image_size		  PNG image的数据缓冲字节大小
// transparent_color   定义PNG image的透明色处理
//                     (unsigned int)(0xFFFFFFFF)  透明色
//                     作为背景色填充在OSD layer指定的位置
int  XM_lcdc_osd_layer_load_png_image (unsigned int lcd_channel,
													unsigned int osd_layer,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *png_image, unsigned int png_image_size,
									XM_COLORREF transparent_color,
									unsigned char alpha)
{
	XM_OSDLAYER *xm_osdlayer;
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 0;
	}
	
	xm_osdlayer = &lcdc_osd_layer[lcd_channel][osd_layer];
	return XM_lcdc_load_png_image (	xm_osdlayer->osd_y_addr, 
												xm_osdlayer->osd_layer_format,
												xm_osdlayer->osd_width,
												xm_osdlayer->osd_height,
												xm_osdlayer->osd_stride,
												osd_image_x, osd_image_y,
												png_image, png_image_size,
												transparent_color,
												alpha
												);
}

int  XM_lcdc_load_png_image_file (unsigned char *osd_layer_buffer, 
									unsigned int osd_layer_format,
									unsigned int osd_layer_width, unsigned int osd_layer_height,
									unsigned int osd_layer_stride,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *png_image_name,
									XM_COLORREF transparent_color,
									unsigned char alpha)
{
	unsigned int file_size;
	char *png_image = NULL;
	int ret = 0;
	void *fp = XM_fopen (png_image_name, "rb");
	do
	{
		if(fp == NULL)
			break;
		
		file_size = XM_filesize (fp);
		if(file_size == 0)
			break;
		
		png_image = XM_heap_malloc (file_size);
		if(png_image == NULL)
		{
			XM_printf ("XM_lcdc_load_png_image_file %s NG, XM_heap_malloc failed\n", png_image_name);
			break;
		}

		if(XM_fread (png_image, 1, file_size, fp) != file_size)
			break;

		ret = XM_lcdc_load_png_image (osd_layer_buffer, 
												osd_layer_format,
												osd_layer_width, osd_layer_height,
												osd_layer_stride,
												osd_image_x, osd_image_y,
												png_image, file_size,
												transparent_color,
												alpha);
		if(ret == 0)
		{
			XM_printf ("XM_lcdc_load_png_image_file %s NG, load failed\n", png_image_name);
		}
		break;
	} while (fp);

	if(fp)
		XM_fclose (fp);
	if(png_image)
		XM_heap_free (png_image);

	return ret;
}

int  XM_lcdc_osd_layer_load_png_image_file (	unsigned int lcd_channel,
												unsigned int osd_layer,
												unsigned int osd_image_x, unsigned int osd_image_y,
												const char *png_image_name,
												XM_COLORREF transparent_color,
												unsigned char alpha)
{
	XM_OSDLAYER *xm_osdlayer;
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 0;
	}
	
	xm_osdlayer = &lcdc_osd_layer[lcd_channel][osd_layer];
	return XM_lcdc_load_png_image_file (xm_osdlayer->osd_y_addr,
												xm_osdlayer->osd_layer_format,
												xm_osdlayer->osd_width,
												xm_osdlayer->osd_height,
												xm_osdlayer->osd_stride,
												osd_image_x, osd_image_y,
												png_image_name,
												transparent_color,
												alpha);
}




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
						  )
{	
	unsigned char *src;
	unsigned char *y_addr, *u_addr, *v_addr, *a_addr;
	unsigned int x, y;
	if(argb_img == NULL || osd_y == NULL || osd_u == NULL || osd_v == NULL || osd_a == NULL)
		return;
	
	// 源图像参数检查
	if (img_w == 0 || img_h == 0 || img_stride == 0)
		return;
	if ((img_w * 4) > img_stride)
		return;
	img_w &= ~1;	// 偶数
	img_h &= ~1;

	//if (img_w != osd_w)
	//	return;
	//if (img_h != osd_h)
	//	return;
	if (osd_stride < osd_w)
		return;

	for (y = 0; y < img_h; y += 2)
	{
		src = argb_img;
		y_addr = osd_y;
		u_addr = osd_u;
		v_addr = osd_v;
		a_addr = osd_a;
		for (x = 0; x < img_w; x += 2)
		{
			BYTE y1, y2, y3, y4;
			BYTE u1, u2, u3, u4;
			BYTE v1, v2, v3, v4;
			unsigned int argb;
			unsigned int r, g, b;
			unsigned int a1, a2, a3, a4; 

			argb = *(unsigned int *)src;
			a1 = argb >> 24;
			r = (argb >> 16) & 0xFF;
			g = (argb >> 8) & 0xFF;
			b = argb & 0xFF;
			XM_RGB2YUV ((BYTE)r, (BYTE)g, (BYTE)b, &y1, &u1, &v1);
		
			argb = *(unsigned int *)(src + 4) ;
			a2 = argb >> 24;
			r = (argb >> 16) & 0xFF;
			g = (argb >> 8) & 0xFF;
			b = argb & 0xFF;
			XM_RGB2YUV ((BYTE)r, (BYTE)g, (BYTE)b, &y2, &u2, &v2);

			argb = *(unsigned int *)(src + img_stride) ;
			a3 = argb >> 24;
			r = (argb >> 16) & 0xFF;
			g = (argb >> 8) & 0xFF;
			b = argb & 0xFF;
			XM_RGB2YUV ((BYTE)r, (BYTE)g, (BYTE)b, &y3, &u3, &v3);

			argb = *(unsigned int *)(src + img_stride + 4) ;
			a4 = argb >> 24;
			r = (argb >> 16) & 0xFF;
			g = (argb >> 8) & 0xFF;
			b = argb & 0xFF;
			XM_RGB2YUV ((BYTE)r, (BYTE)g, (BYTE)b, &y4, &u4, &v4);

			*y_addr = (unsigned char)y1;
			*(y_addr + 1) = (unsigned char)y2;
			*(y_addr + osd_stride) = (unsigned char)y3;
			*(y_addr + osd_stride + 1) = (unsigned char)y4;
			
			*a_addr = (unsigned char)a1;
			*(a_addr + 1) = (unsigned char)a2;
			*(a_addr + osd_stride) = (unsigned char)a3;
			*(a_addr + osd_stride + 1) = (unsigned char)a4;

			*u_addr  = (unsigned char)u1;
			*(u_addr + 1)  = (unsigned char)u2;
			*(u_addr + osd_stride)  = (unsigned char)u3;
			*(u_addr + osd_stride + 1)  = (unsigned char)u4;

			*v_addr  = (unsigned char)v1;
			*(v_addr + 1) = (unsigned char)v2;
			*(v_addr + osd_stride) = (unsigned char)v3;
			*(v_addr + osd_stride + 1) = (unsigned char)v4;

			a_addr += 2;
			y_addr += 2;
			u_addr += 2;
			v_addr += 2;
			src += 8;
		}
		
		argb_img += img_stride * 2;
		osd_y += osd_stride * 2;
		osd_u += osd_stride * 2;
		osd_v += osd_stride * 2;
		osd_a += osd_stride * 2;
	}
}

// 将ARGB888格式的源图像转换为YUV444格式(AYUVAYUVAYUV交错存放)
void XM_lcdc_convert_argb888_to_ayuv444_packed (
						  unsigned char *argb_img,		// ARGB888格式的源图像
						  unsigned int img_stride,		// 源图像的行字节长度
						  unsigned int img_w,			// 源图像的宽度(像素点)
						  unsigned int img_h,			// 源图像的高度(像素点)
						  unsigned char *ayuv_img,		// 目标OSD层的YUV地址
						  unsigned int osd_stride,		// 目标OSD层的行像素(对齐)长度
						  unsigned int osd_w,			// 目标OSD层的像素宽度
						  unsigned int osd_h				// 目标OSD层的像素高度
						  )
{	
	unsigned int *src;
	unsigned int *dst;
	unsigned int x, y;
	if(argb_img == NULL || ayuv_img == NULL)
		return;
	
	// 源图像参数检查
	if (img_w == 0 || img_h == 0 || img_stride == 0)
		return;
	if ((img_w * 4) > img_stride)
		return;
	img_w &= ~1;	// 偶数
	img_h &= ~1;

	//if (img_w != osd_w)
	//	return;
	//if (img_h != osd_h)
	//	return;
	if (osd_stride < osd_w)
		return;

	for (y = 0; y < img_h; y += 1)
	{
		src = (unsigned int *)argb_img;
		dst = (unsigned int *)ayuv_img;
		for (x = 0; x < img_w; x += 1)
		{
			BYTE y1, y2, y3, y4;
			BYTE u1, u2, u3, u4;
			BYTE v1, v2, v3, v4;
			unsigned int argb;
			unsigned int r, g, b;
			unsigned int a1, a2, a3, a4; 

			argb = *src ++;
			a1 = argb >> 24;
			r = (argb >> 16) & 0xFF;
			g = (argb >> 8) & 0xFF;
			b = argb & 0xFF;
			XM_RGB2YUV ((BYTE)r, (BYTE)g, (BYTE)b, &y1, &u1, &v1);
			*dst ++ = (a1 << 24) | (y1 << 16) | (u1 << 8) | (v1); 		
		}
		
		argb_img += img_stride;
		ayuv_img += osd_stride;
	}
}

// 简化版本
void XM_lcdc_osd_layer_load_ayuv444_image_normal_simp_old (
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

						  )
{
	unsigned int i, j;
	unsigned char *src_y, *src_u, *src_v, *src_a;
	unsigned char *dst_y, *dst_u, *dst_v;
	if(img_y == NULL || img_u == NULL || img_v == NULL || img_a == NULL)
		return;
	if(img_w == 0 || img_h == 0 || copy_w == 0 || copy_h == 0)
		return;
	if(img_w > img_stride)
		return;
	if(img_offset_x >= img_w || img_offset_y >= img_h)
		return;
	if(img_offset_x + copy_w > img_w)
		copy_w = img_w - img_offset_x;
	if(img_offset_y + copy_h > img_h)
		copy_h = img_h - img_offset_y;

	if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		if(osd_y == NULL || osd_u == NULL)
			return;
	}
	else
	{
		XM_printf ("only Y_UV420 supported\n");
		return;
	}
	if(osd_w == 0 || osd_h == 0)
		return;
	if(osd_w > osd_stride)
		return;
	if(osd_offset_x >= osd_w || osd_offset_y >= osd_h)
		return;
	if(osd_offset_x + copy_w > osd_w)
		copy_w = osd_w - osd_offset_x;
	if(osd_offset_y + copy_h > osd_h)
		copy_h = osd_h - osd_offset_y;

	img_y += img_offset_y * img_stride + img_offset_x;
	img_u += img_offset_y * img_stride + img_offset_x;
	img_v += img_offset_y * img_stride + img_offset_x;
	img_a += img_offset_y * img_stride + img_offset_x;

	osd_y += (osd_offset_y * osd_stride + osd_offset_x);
	{
		// Y_UV420
		osd_u += (osd_offset_y * osd_stride/2) + ((osd_offset_x >> 1) << 1);
		osd_v = osd_u + 1;
	}

	copy_h >>= 1;
	copy_w >>= 1;
	for (j = 0; j < copy_h; j ++)
	{
		src_y = img_y;
		src_u = img_u;
		src_v = img_v;
		src_a = img_a;
		dst_y = osd_y;
		dst_u = osd_u;
		dst_v = osd_v;
		for (i = 0; i < copy_w; i ++)
		{
			unsigned int y, y2, u, v;
			unsigned int a1 = *src_a;
			unsigned int a2 = *(src_a + 1);
			unsigned int a3 = *(src_a + img_stride);
			unsigned int a4 = *(src_a + 1 + img_stride);
			unsigned int temp;
		//	y = (*src_y * a1 + *dst_y * (0xFF - a1)) / 255;
			y  = (*src_y * a1 + *dst_y * (0xFF - a1)) >> 8;
			//*dst_y = (unsigned char)y;
			y2 = (*(src_y + 1) * a2 + *(dst_y + 1) * (0xFF - a2)) >> 8;
		//	y = (*(src_y + 1) * a2 + *(dst_y + 1) * (0xFF - a2)) / 255;
			//*(dst_y + 1) = (unsigned char)y;
			*(unsigned short *)dst_y = (unsigned short)(y | (y2 << 8));
			y  = (*(src_y + img_stride) * a3 + *(dst_y + osd_stride) * (0xFF - a3)) >> 8;
		//	y = (*(src_y + img_stride) * a3 + *(dst_y + osd_stride) * (0xFF - a3)) / 255;
		//	*(dst_y + osd_stride) = (unsigned char)y;
			y2 = (*(src_y + img_stride + 1) * a4 + *(dst_y + osd_stride + 1) * (0xFF - a4)) >> 8;
		//	y = (*(src_y + img_stride + 1) * a4 + *(dst_y + osd_stride + 1) * (0xFF - a4)) / 255;
		//	*(dst_y + osd_stride + 1) = (unsigned char)y;
			*(unsigned short *)(dst_y + osd_stride) = (unsigned short)(y | (y2 << 8));

			temp = 0xFF - a1;
			u  = *dst_u * temp;
			v  = *dst_v * temp;
			u += *src_u * a1;
			v += *src_v * a1;
			u >>= 8;
			v >>= 8;
			
			*(unsigned short *)dst_u = u | (v << 8);
			dst_u += 2;
			dst_v += 2;

			src_y += 2;
			src_u += 2;
			src_v += 2;
			src_a += 2;
			dst_y += 2;
		}

		img_y += img_stride * 2;
		img_u += img_stride * 2;
		img_v += img_stride * 2;
		img_a += img_stride * 2;
		osd_y += osd_stride * 2;
		osd_u += osd_stride;
		osd_v += osd_stride;
	}

}		

// 简化版本
void XM_lcdc_osd_layer_load_ayuv444_image_normal_opt_old (
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

						  )
{
	unsigned int i, j;
	unsigned char *src_y, *src_u, *src_v, *src_a;
	unsigned char *dst_y, *dst_u, *dst_v;
	if(img_y == NULL || img_u == NULL || img_v == NULL || img_a == NULL)
		return;
	if(img_w == 0 || img_h == 0 || copy_w == 0 || copy_h == 0)
		return;
	if(img_w > img_stride)
		return;
	if(img_offset_x >= img_w || img_offset_y >= img_h)
		return;
	if(img_offset_x + copy_w > img_w)
		copy_w = img_w - img_offset_x;
	if(img_offset_y + copy_h > img_h)
		copy_h = img_h - img_offset_y;

	if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		if(osd_y == NULL || osd_u == NULL)
			return;
	}
	else
	{
		XM_printf ("only Y_UV420 supported\n");
		return;
	}
	if(osd_w == 0 || osd_h == 0)
		return;
	if(osd_w > osd_stride)
		return;
	if(osd_offset_x >= osd_w || osd_offset_y >= osd_h)
		return;
	if(osd_offset_x + copy_w > osd_w)
		copy_w = osd_w - osd_offset_x;
	if(osd_offset_y + copy_h > osd_h)
		copy_h = osd_h - osd_offset_y;

	img_y += img_offset_y * img_stride + img_offset_x;
	img_u += img_offset_y * img_stride + img_offset_x;
	img_v += img_offset_y * img_stride + img_offset_x;
	img_a += img_offset_y * img_stride + img_offset_x;

	osd_y += (osd_offset_y * osd_stride + osd_offset_x);
	{
		// Y_UV420
		osd_u += (osd_offset_y * osd_stride/2) + ((osd_offset_x >> 1) << 1);
		osd_v = osd_u + 1;
	}
	
#if 1
	unsigned char *old_a = img_a;
	for (j = 0; j < copy_h; j ++)
	{
		src_y = img_y;
		src_a = img_a;
		dst_y = osd_y;
		for (i = 0; i < copy_w/2; i ++)
		{
			unsigned int y, y2;
			unsigned int a1 = *src_a;
			unsigned int a2 = *(src_a + 1);
			y  = (*src_y * a1 + *dst_y * (0xFF - a1)) >> 8;
			y2 = (*(src_y + 1) * a2 + *(dst_y + 1) * (0xFF - a2)) >> 8;
			*(unsigned short *)dst_y = (unsigned short)(y | (y2 << 8));
			src_y += 2;
			src_a += 2;
			dst_y += 2;
		}
		img_y += img_stride;
		img_a += img_stride;
		osd_y += osd_stride;
	}
	copy_h >>= 1;
	copy_w >>= 1;
	
	img_a = old_a;
	
	for (j = 0; j < copy_h; j ++)
	{
		src_u = img_u;
		src_v = img_v;
		src_a = img_a;
		dst_u = osd_u;
		dst_v = osd_v;
		for (i = 0; i < copy_w; i ++)
		{
			unsigned int u, v;
			unsigned int a1 = *src_a;
			unsigned int temp;

			temp = 0xFF - a1;
			u  = *dst_u * temp;
			v  = *dst_v * temp;
			u += *src_u * a1;
			v += *src_v * a1;
			u >>= 8;
			v >>= 8;
			
			*(unsigned short *)dst_u = u | (v << 8);
			dst_u += 2;
			dst_v += 2;

			src_u += 2;
			src_v += 2;
			src_a += 2;
		}

		img_u += img_stride * 2;
		img_v += img_stride * 2;
		img_a += img_stride * 2;
		osd_u += osd_stride;
		osd_v += osd_stride;
	}	
	
#else

	copy_h >>= 1;
	copy_w >>= 1;
	for (j = 0; j < copy_h; j ++)
	{
		src_y = img_y;
		src_u = img_u;
		src_v = img_v;
		src_a = img_a;
		dst_y = osd_y;
		dst_u = osd_u;
		dst_v = osd_v;
		for (i = 0; i < copy_w; i ++)
		{
			unsigned int y, y2, u, v;
			unsigned int a1 = *src_a;
			unsigned int a2 = *(src_a + 1);
			unsigned int a3 = *(src_a + img_stride);
			unsigned int a4 = *(src_a + 1 + img_stride);
			unsigned int temp;
		//	y = (*src_y * a1 + *dst_y * (0xFF - a1)) / 255;
			y  = (*src_y * a1 + *dst_y * (0xFF - a1)) >> 8;
			//*dst_y = (unsigned char)y;
			y2 = (*(src_y + 1) * a2 + *(dst_y + 1) * (0xFF - a2)) >> 8;
		//	y = (*(src_y + 1) * a2 + *(dst_y + 1) * (0xFF - a2)) / 255;
			//*(dst_y + 1) = (unsigned char)y;
			*(unsigned short *)dst_y = (unsigned short)(y | (y2 << 8));
			y  = (*(src_y + img_stride) * a3 + *(dst_y + osd_stride) * (0xFF - a3)) >> 8;
		//	y = (*(src_y + img_stride) * a3 + *(dst_y + osd_stride) * (0xFF - a3)) / 255;
		//	*(dst_y + osd_stride) = (unsigned char)y;
			y2 = (*(src_y + img_stride + 1) * a4 + *(dst_y + osd_stride + 1) * (0xFF - a4)) >> 8;
		//	y = (*(src_y + img_stride + 1) * a4 + *(dst_y + osd_stride + 1) * (0xFF - a4)) / 255;
		//	*(dst_y + osd_stride + 1) = (unsigned char)y;
			*(unsigned short *)(dst_y + osd_stride) = (unsigned short)(y | (y2 << 8));

			temp = 0xFF - a1;
			u  = *dst_u * temp;
			v  = *dst_v * temp;
			u += *src_u * a1;
			v += *src_v * a1;
			u >>= 8;
			v >>= 8;
			
			*(unsigned short *)dst_u = u | (v << 8);
			dst_u += 2;
			dst_v += 2;

			src_y += 2;
			src_u += 2;
			src_v += 2;
			src_a += 2;
			dst_y += 2;
		}

		img_y += img_stride * 2;
		img_u += img_stride * 2;
		img_v += img_stride * 2;
		img_a += img_stride * 2;
		osd_y += osd_stride * 2;
		osd_u += osd_stride;
		osd_v += osd_stride;
	}
#endif
}		

void XM_lcdc_osd_layer_load_ayuv444_image_normal_old (
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

						  )
{
	unsigned int i, j;
	unsigned char *src_y, *src_u, *src_v, *src_a;
	unsigned char *dst_y, *dst_u, *dst_v;
	if(img_y == NULL || img_u == NULL || img_v == NULL || img_a == NULL)
		return;
	if(img_w == 0 || img_h == 0 || copy_w == 0 || copy_h == 0)
		return;
	if(img_w > img_stride)
		return;
	if(img_offset_x >= img_w || img_offset_y >= img_h)
		return;
	if(img_offset_x + copy_w > img_w)
		copy_w = img_w - img_offset_x;
	if(img_offset_y + copy_h > img_h)
		copy_h = img_h - img_offset_y;

	if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420)
	{
		if(osd_y == NULL || osd_u == NULL || osd_v == NULL)
			return;
	}
	else if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		if(osd_y == NULL || osd_u == NULL)
			return;
	}
	else
	{
		XM_printf ("YUV420 or Y_UV420 supported\n");
		return;
	}
	if(osd_w == 0 || osd_h == 0)
		return;
	if(osd_w > osd_stride)
		return;
	if(osd_offset_x >= osd_w || osd_offset_y >= osd_h)
		return;
	if(osd_offset_x + copy_w > osd_w)
		copy_w = osd_w - osd_offset_x;
	if(osd_offset_y + copy_h > osd_h)
		copy_h = osd_h - osd_offset_y;

	img_y += img_offset_y * img_stride + img_offset_x;
	img_u += img_offset_y * img_stride + img_offset_x;
	img_v += img_offset_y * img_stride + img_offset_x;
	img_a += img_offset_y * img_stride + img_offset_x;

	osd_y += (osd_offset_y * osd_stride + osd_offset_x);
	if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420)
	{
		// YUV420
		osd_u += (osd_offset_y * osd_stride/2 + osd_offset_x) >> 1;
		osd_v += (osd_offset_y * osd_stride/2 + osd_offset_x) >> 1;
	}
	else //if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		// Y_UV420
		osd_u += (osd_offset_y * osd_stride/2) + ((osd_offset_x >> 1) << 1);
		osd_v = osd_u + 1;
	}

	copy_h >>= 1;
	copy_w >>= 1;
	for (j = 0; j < copy_h; j ++)
	{
		src_y = img_y;
		src_u = img_u;
		src_v = img_v;
		src_a = img_a;
		dst_y = osd_y;
		dst_u = osd_u;
		dst_v = osd_v;
		for (i = 0; i < copy_w; i ++)
		{
			unsigned int y, y2, u, v;
			unsigned int a1 = *src_a;
			unsigned int a2 = *(src_a + 1);
			unsigned int a3 = *(src_a + img_stride);
			unsigned int a4 = *(src_a + 1 + img_stride);
			unsigned int temp;
		//	y = (*src_y * a1 + *dst_y * (0xFF - a1)) / 255;
			y  = (*src_y * a1 + *dst_y * (0xFF - a1)) >> 8;
			//*dst_y = (unsigned char)y;
			y2 = (*(src_y + 1) * a2 + *(dst_y + 1) * (0xFF - a2)) >> 8;
		//	y = (*(src_y + 1) * a2 + *(dst_y + 1) * (0xFF - a2)) / 255;
			//*(dst_y + 1) = (unsigned char)y;
			*(unsigned short *)dst_y = (unsigned short)(y | (y2 << 8));
			y  = (*(src_y + img_stride) * a3 + *(dst_y + osd_stride) * (0xFF - a3)) >> 8;
		//	y = (*(src_y + img_stride) * a3 + *(dst_y + osd_stride) * (0xFF - a3)) / 255;
		//	*(dst_y + osd_stride) = (unsigned char)y;
			y2 = (*(src_y + img_stride + 1) * a4 + *(dst_y + osd_stride + 1) * (0xFF - a4)) >> 8;
		//	y = (*(src_y + img_stride + 1) * a4 + *(dst_y + osd_stride + 1) * (0xFF - a4)) / 255;
		//	*(dst_y + osd_stride + 1) = (unsigned char)y;
			*(unsigned short *)(dst_y + osd_stride) = (unsigned short)(y | (y2 << 8));

#if 1
			temp = 0xFF - a1;
			u  = *dst_u * temp;
			v  = *dst_v * temp;
			u += *src_u * a1;
			v += *src_v * a1;
			u >>= 8;
			v >>= 8;
			
#else
			temp = (0xFF*4 - a1 - a2 - a3 - a4);
			u  = *dst_u * temp;
			v  = *dst_v * temp;
			u += *src_u * a1;
			v += *src_v * a1;
			u += *(src_u + 1) * a2;
			v += *(src_v + 1) * a2;
			u += *(src_u + img_stride) * a3;
			v += *(src_v + img_stride) * a3;
			u += *(src_u + img_stride + 1) * a4;
			v += *(src_v + img_stride + 1) * a4;
			u >>= 10;
			v >>= 10;
#endif

			if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420)
			{
				*dst_u ++ = (unsigned char)u;
				*dst_v ++ = (unsigned char)v;
			}
			else //if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
			{
				*(unsigned short *)dst_u = u | (v << 8);
				//*dst_u = (unsigned char)u;
				dst_u += 2;
				//*dst_v = (unsigned char)v;
				dst_v += 2;
			}

			src_y += 2;
			src_u += 2;
			src_v += 2;
			src_a += 2;
			dst_y += 2;
		}

		img_y += img_stride * 2;
		img_u += img_stride * 2;
		img_v += img_stride * 2;
		img_a += img_stride * 2;
		osd_y += osd_stride * 2;
		if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420)
		{
			osd_u += osd_stride >> 1;
			osd_v += osd_stride >> 1;
		}
		else if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
		{
			osd_u += osd_stride;
			osd_v += osd_stride;
		}
	}

}		

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

						  )
{
	unsigned int i, j;
	unsigned char *src_y, *src_u, *src_v, *src_a;
	unsigned char *dst_y, *dst_u, *dst_v;
	unsigned int osd_layer_rotate = 0;
	
#ifdef AYUV420_ROTATE
	osd_layer_rotate = 1;
#endif
	if(img_y == NULL || img_u == NULL || img_v == NULL || img_a == NULL)
		return;
	if(img_w == 0 || img_h == 0 || copy_w == 0 || copy_h == 0)
		return;
	if(img_w > img_stride)
		return;
	if(img_offset_x >= img_w || img_offset_y >= img_h)
		return;
	if(img_offset_x + copy_w > img_w)
		copy_w = img_w - img_offset_x;
	if(img_offset_y + copy_h > img_h)
		copy_h = img_h - img_offset_y;

	if(osd_y == NULL || osd_u == NULL || osd_v == NULL)
		return;
	if(osd_w == 0 || osd_h == 0)
		return;
	if(osd_w > osd_stride)
		return;
	if(osd_offset_x >= osd_w || osd_offset_y >= osd_h)
		return;
	if(osd_offset_x + copy_w > osd_w)
		copy_w = osd_w - osd_offset_x;
	if(osd_offset_y + copy_h > osd_h)
		copy_h = osd_h - osd_offset_y;

#ifdef AYUV420_ROTATE
	img_y += (img_h - 1 - img_offset_y) * img_stride + img_offset_x;
	img_u += ((img_h - 1 - img_offset_y) * img_stride + img_offset_x) >> 2;
	img_v += ((img_h - 1 - img_offset_y) * img_stride + img_offset_x) >> 2;
	img_a += (img_h - 1 - img_offset_y) * img_stride + img_offset_x;
#else
	img_y += img_offset_y * img_stride +  img_offset_x;
	img_u += (img_offset_y * img_stride + img_offset_x) >> 2;
	img_v += (img_offset_y * img_stride + img_offset_x) >> 2;
	img_a += img_offset_y * img_stride + img_offset_x;
#endif

#ifdef AYUV420_ROTATE
	osd_offset_y = osd_h - 1 - (osd_offset_y + img_h);
#endif
	
	if(osd_layer_rotate)
	{
		osd_y += (osd_offset_x * osd_h + osd_offset_y);
		osd_u += (osd_offset_x * osd_h/2 + osd_offset_y) >> 1;
		osd_v += (osd_offset_x * osd_h/2 + osd_offset_y) >> 1;
	}
	else
	{
		osd_y += (osd_offset_y * osd_stride + osd_offset_x);
		osd_u += (osd_offset_y * osd_stride/2 + osd_offset_x) >> 1;
		osd_v += (osd_offset_y * osd_stride/2 + osd_offset_x) >> 1;
	}
	
	copy_h >>= 1;
	copy_w >>= 1;
	for (j = 0; j < copy_h; j ++)
	{
		src_y = img_y;
		src_u = img_u;
		src_v = img_v;
		src_a = img_a;
		dst_y = osd_y;
		dst_u = osd_u;
		dst_v = osd_v;
		for (i = 0; i < copy_w; i ++)
		{
			unsigned int y, u, v;
			unsigned char a1 = *src_a;
			unsigned char a2 = *(src_a + 1);
			unsigned char a3 = *(src_a + img_stride);
			unsigned char a4 = *(src_a + 1 + img_stride);
			if(osd_layer_rotate)
			{
				y = (*src_y * a1 + *dst_y * (0xFF - a1)) / 0xFF;
				*dst_y = (unsigned char)y;
				y = (*(src_y + 1) * a2 + *(dst_y + 1) * (0xFF - a2)) / 0xFF;
				*(dst_y + 1) = (unsigned char)y;
				y = (*(src_y + img_stride) * a3 + *(dst_y + osd_stride) * (0xFF - a3)) / 0xFF;
				*(dst_y + osd_stride) = (unsigned char)y;
				y = (*(src_y + img_stride + 1) * a4 + *(dst_y + osd_stride + 1) * (0xFF - a4)) / 0xFF;
				*(dst_y + osd_stride + 1) = (unsigned char)y;
				u = (*src_u * a1 + *dst_u * (0xFF - a1)) / 0xFF;
				*dst_u ++ = (unsigned char)u;
				v = (*src_v * a1 + *dst_v * (0xFF - a1)) / 0xFF;
				*dst_v ++ = (unsigned char)v;				
			}
			else
			{
				y = (*src_y * a1 + *dst_y * (0xFF - a1)) / 0xFF;
				*dst_y = (unsigned char)y;
				y = (*(src_y + 1) * a2 + *(dst_y + 1) * (0xFF - a2)) / 0xFF;
				*(dst_y + 1) = (unsigned char)y;
				y = (*(src_y + img_stride) * a3 + *(dst_y + osd_stride) * (0xFF - a3)) / 0xFF;
				*(dst_y + osd_stride) = (unsigned char)y;
				y = (*(src_y + img_stride + 1) * a4 + *(dst_y + osd_stride + 1) * (0xFF - a4)) / 0xFF;
				*(dst_y + osd_stride + 1) = (unsigned char)y;
				u = (*src_u * a1 + *dst_u * (0xFF - a1)) / 0xFF;
				*dst_u ++ = (unsigned char)u;
				v = (*src_v * a1 + *dst_v * (0xFF - a1)) / 0xFF;
				*dst_v ++ = (unsigned char)v;
			}

			src_y += 2;
			src_u ++;
			src_v ++;
			src_a += 2;
			dst_y += 2;
		}

		img_y += img_stride * 2;
		img_u += img_stride >> 1;
		img_v += img_stride >> 1;
		img_a += img_stride * 2;
		osd_y += osd_stride * 2;
		osd_u += osd_stride >> 1;
		osd_v += osd_stride >> 1;
	}

}

// 简化版本，参考上面原始的代码
// 只针对Y_UV420, VERT_REVERSE，osd_layer_rotate=1
// 比较上面原始的版本，icon显示的时间从14ms --> 10ms
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

						  )
{
	unsigned int i, j;
	unsigned char *src_y, *src_u, *src_v, *src_a;
	unsigned char *dst_y, *dst_u, *dst_v;
	
	
	if(img_y == NULL || img_u == NULL || img_v == NULL || img_a == NULL)
		return;
	if(img_w == 0 || img_h == 0 || copy_w == 0 || copy_h == 0)
		return;
	if(img_w > img_stride)
		return;
	if(img_offset_x >= img_w || img_offset_y >= img_h)
		return;
	if(img_offset_x + copy_w > img_w)
		copy_w = img_w - img_offset_x;
	if(img_offset_y + copy_h > img_h)
		copy_h = img_h - img_offset_y;

	if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420)
	{
		if(osd_y == NULL || osd_u == NULL || osd_v == NULL)
			return;
	}
	else if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		if(osd_y == NULL || osd_u == NULL)
			return;
	}
	else
	{
		XM_printf ("YUV420 or Y_UV420 supported\n");
		return;
	}
	if(osd_w == 0 || osd_h == 0)
		return;
	if(osd_w > osd_stride)
		return;
	if(osd_offset_x >= osd_w || osd_offset_y >= osd_h)
		return;
	if(osd_offset_x + copy_w > osd_w)
		copy_w = osd_w - osd_offset_x;
	if(osd_offset_y + copy_h > osd_h)
		copy_h = osd_h - osd_offset_y;

	osd_offset_y = osd_h - 1 - (osd_offset_y + copy_h);
	osd_offset_y &= ~1;

	img_y += (img_h - 1 - img_offset_y) * img_stride + img_offset_x;
	img_u += (img_h - 1 - img_offset_y) * img_stride + img_offset_x;
	img_v += (img_h - 1 - img_offset_y) * img_stride + img_offset_x;
	img_a += (img_h - 1 - img_offset_y) * img_stride + img_offset_x;

	{
		osd_y += (osd_offset_y * 1 + osd_offset_x * osd_h);
		// Y_UV420
		osd_u += ((osd_offset_y >> 1) << 1) + osd_offset_x * osd_h/2;
		osd_v = osd_u + 1;
	}

	copy_h >>= 1;
	copy_w >>= 1;
	for (j = 0; j < copy_h; j ++)
	{
		src_y = img_y;
		src_u = img_u;
		src_v = img_v;
		src_a = img_a;
		dst_y = osd_y;
		dst_u = osd_u;
		dst_v = osd_v;
		for (i = 0; i < copy_w; i ++)
		{
			unsigned int y, y2, u, v;
			unsigned int a1 = *src_a;
			unsigned int a2 = *(src_a + 1);
			unsigned int a3 = *(src_a - img_stride);
			unsigned int a4 = *(src_a + 1 - img_stride);
			unsigned int temp;
			y  = (*src_y * a1 + *dst_y * (0xFF - a1)) >> 8;
			y2 = (*(src_y + 1) * a2 + *(dst_y + osd_h) * (0xFF - a2)) >> 8;
			*dst_y = y;
			*(dst_y + osd_h) = y2;
			y  = (*(src_y - img_stride) * a3 + *(dst_y + 1) * (0xFF - a3)) >> 8;
			y2 = (*(src_y - img_stride + 1) * a4 + *(dst_y + 1 + osd_h) * (0xFF - a4)) >> 8;
			*(dst_y + 1) = y;
			*(dst_y + 1 + osd_h) = y2;

#if 1
			// 仅使用1个点的UV数据(比较4点，显示质量基本无差异)，优化DDR访问效率
			temp = 0xFF - a1;
			u  = *dst_u * temp;
			v  = *dst_v * temp;
			u += *src_u * a1;
			v += *src_v * a1;
			u >>= 8;
			v >>= 8;
			
#else
			temp = (0xFF*4 - a1 - a2 - a3 - a4);
			u  = *dst_u * temp;
			v  = *dst_v * temp;
			u += *src_u * a1;
			v += *src_v * a1;
			u += *(src_u + 1) * a2;
			v += *(src_v + 1) * a2;
			u += *(src_u - img_stride) * a3;
			v += *(src_v - img_stride) * a3;
			u += *(src_u - img_stride + 1) * a4;
			v += *(src_v - img_stride + 1) * a4;
			u >>= 10;
			v >>= 10;
#endif

			*dst_u = (unsigned char)u;
			dst_u += osd_h;
			*dst_v = (unsigned char)v;
			dst_v += osd_h;

			src_y += 2;
			src_u += 2;
			src_v += 2;
			src_a += 2;
			dst_y += osd_h * 2;
		}

		img_y -= img_stride * 2;
		img_u -= img_stride * 2;
		img_v -= img_stride * 2;
		img_a -= img_stride * 2;
		
		osd_y += 2;
		osd_u += 2;
		osd_v += 2;
			
	}

}	

// 简化版本，参考上面原始的代码
// 只针对Y_UV420, VERT_REVERSE，osd_layer_rotate=1
// 比较上面原始的版本，icon显示的时间从14ms --> 10ms
// YUVA444分块存储
void XM_lcdc_osd_layer_load_yuva444_image_old (
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

						  )
{
	unsigned int i, j;
	unsigned char *src_y, *src_u, *src_v, *src_a;
	unsigned char *dst_y, *dst_u, *dst_v;
	unsigned int block_size;
	
	
	if(img_y == NULL)
		return;
	if(img_w == 0 || img_h == 0 || copy_w == 0 || copy_h == 0)
		return;
	if(img_w > img_stride)
		return;
	if(img_offset_x >= img_w || img_offset_y >= img_h)
		return;
	if(img_offset_x + copy_w > img_w)
		copy_w = img_w - img_offset_x;
	if(img_offset_y + copy_h > img_h)
		copy_h = img_h - img_offset_y;

	if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420)
	{
		if(osd_y == NULL || osd_u == NULL || osd_v == NULL)
			return;
	}
	else if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		if(osd_y == NULL || osd_u == NULL)
			return;
	}
	else
	{
		XM_printf ("YUV420 or Y_UV420 supported\n");
		return;
	}
	if(osd_w == 0 || osd_h == 0)
		return;
	if(osd_w > osd_stride)
		return;
	if(osd_offset_x >= osd_w || osd_offset_y >= osd_h)
		return;
	if(osd_offset_x + copy_w > osd_w)
		copy_w = osd_w - osd_offset_x;
	if(osd_offset_y + copy_h > osd_h)
		copy_h = osd_h - osd_offset_y;

	osd_offset_y = osd_h - 1 - (osd_offset_y + copy_h);
	osd_offset_y &= ~1;

	img_y += (img_h - 1 - img_offset_y) * img_stride + img_offset_x;

	{
		osd_y += (osd_offset_y * 1 + osd_offset_x * osd_h);
		// Y_UV420
		osd_u += ((osd_offset_y >> 1) << 1) + osd_offset_x * osd_h/2;
		osd_v = osd_u + 1;
	}
	
	block_size = img_stride * img_h;

	copy_h >>= 1;
	copy_w >>= 1;
	for (j = 0; j < copy_h; j ++)
	{
		src_y = img_y;
		src_u = src_y + block_size;
		src_v = src_u + block_size;
		src_a = src_v + block_size;
		
		dst_y = osd_y;
		dst_u = osd_u;
		//dst_v = osd_v;
		for (i = 0; i < copy_w; i ++)
		{
			unsigned int y, y2, u, v;
			unsigned int a1;
			unsigned int a2;
			unsigned int a3;
			unsigned int a4;
			unsigned int temp;
			
			temp = *(unsigned short *)src_a;
			a1 = temp & 0xFF;
			a2 = temp >> 8;
			temp = *(unsigned short *)(src_a - img_stride);
			a3 = temp & 0xFF;
			a4 = temp >> 8;
			//unsigned int a1 = *src_a;
			//unsigned int a2 = *(src_a + 1);
			//unsigned int a3 = *(src_a - img_stride);
			//unsigned int a4 = *(src_a + 1 - img_stride);
				
			//temp = *(unsigned short *)dst_y;
			//y = (*src_y * a1 + (temp & 0xFF) * (0xFF - a1)) >> 8;
			//y |= ((*(src_y - img_stride) * a3 + (temp >> 8) * (0xFF - a3)) >> 8) << 8;
			//*(unsigned short *)dst_y = y;
			y  = (*src_y * a1 + *dst_y * (0xFF - a1)) >> 8;
			//*dst_y = y;
			y2  = (*(src_y - img_stride) * a3 + *(dst_y + 1) * (0xFF - a3)) >> 8;
			//*(dst_y + 1) = y;
			*(unsigned short *)dst_y = y | (y2 << 8);
			
			temp = (unsigned int)(dst_y + osd_h);
			y = (*(src_y + 1) * a2 + *((unsigned char *)temp) * (0xFF - a2)) >> 8;
			//*((unsigned char *)temp) = y2;
			y2 = (*(src_y - img_stride + 1) * a4 + *((unsigned char *)temp + 1) * (0xFF - a4)) >> 8;
			//*((unsigned char *)temp + 1) = y2;
			*(unsigned short *)temp = y | (y2 << 8);
			//y2 = (*(src_y + 1) * a2 + *(dst_y + osd_h) * (0xFF - a2)) >> 8;
			//*(dst_y + osd_h) = y2;
			//y2 = (*(src_y - img_stride + 1) * a4 + *(dst_y + 1 + osd_h) * (0xFF - a4)) >> 8;
			//*(dst_y + 1 + osd_h) = y2;

#if 1
			// 仅使用1个点的UV数据(比较4点，显示质量基本无差异)，优化DDR访问效率
			temp = 0xFF - a1;
			a2 = *(unsigned short *)dst_u;
			u = (a2 & 0xFF) * temp;
			v = (a2 >> 8) * temp;
			//u  = *dst_u * temp;
			//v  = *dst_v * temp;
			u += *src_u * a1;
			v += *src_v * a1;
			u >>= 8;
			v >>= 8;
			
#else
			temp = (0xFF*4 - a1 - a2 - a3 - a4);
			u  = *dst_u * temp;
			v  = *dst_v * temp;
			u += *src_u * a1;
			v += *src_v * a1;
			u += *(src_u + 1) * a2;
			v += *(src_v + 1) * a2;
			u += *(src_u - img_stride) * a3;
			v += *(src_v - img_stride) * a3;
			u += *(src_u - img_stride + 1) * a4;
			v += *(src_v - img_stride + 1) * a4;
			u >>= 10;
			v >>= 10;
#endif

			//*dst_u = (unsigned char)u;
			*(unsigned short *)dst_u = u | (v << 8);
			dst_u += osd_h;
			//*dst_v = (unsigned char)v;
			//dst_v += osd_h;

			src_y += 2;
			src_u += 2;
			src_v += 2;
			src_a += 2;
			dst_y += osd_h * 2;
		}

		img_y -= img_stride * 2;
		
		osd_y += 2;
		osd_u += 2;
		//osd_v += 2;
			
	}

}	



void XM_lcdc_osd_layer_load_ayuv444_image_full (
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

						  )
{
	unsigned int i, j;
	unsigned char *src_y, *src_u, *src_v, *src_a;
	unsigned char *dst_y, *dst_u, *dst_v;
	unsigned int osd_layer_rotate = 0;
	
#ifdef LCD_ROTATE_90
	osd_layer_rotate = 1;
#endif
	if(img_y == NULL || img_u == NULL || img_v == NULL || img_a == NULL)
		return;
	if(img_w == 0 || img_h == 0 || copy_w == 0 || copy_h == 0)
		return;
	if(img_w > img_stride)
		return;
	if(img_offset_x >= img_w || img_offset_y >= img_h)
		return;
	if(img_offset_x + copy_w > img_w)
		copy_w = img_w - img_offset_x;
	if(img_offset_y + copy_h > img_h)
		copy_h = img_h - img_offset_y;

	if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420)
	{
		if(osd_y == NULL || osd_u == NULL || osd_v == NULL)
			return;
	}
	else if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		if(osd_y == NULL || osd_u == NULL)
			return;
	}
	else
	{
		XM_printf ("YUV420 or Y_UV420 supported\n");
		return;
	}
	if(osd_w == 0 || osd_h == 0)
		return;
	if(osd_w > osd_stride)
		return;
	if(osd_offset_x >= osd_w || osd_offset_y >= osd_h)
		return;
	if(osd_offset_x + copy_w > osd_w)
		copy_w = osd_w - osd_offset_x;
	if(osd_offset_y + copy_h > osd_h)
		copy_h = osd_h - osd_offset_y;

#if VERT_REVERSE
	osd_offset_y = osd_h - 1 - (osd_offset_y + copy_h);
	osd_offset_y &= ~1;
#endif

#if VERT_REVERSE
	img_y += (img_h - 1 - img_offset_y) * img_stride + img_offset_x;
	img_u += (img_h - 1 - img_offset_y) * img_stride + img_offset_x;
	img_v += (img_h - 1 - img_offset_y) * img_stride + img_offset_x;
	img_a += (img_h - 1 - img_offset_y) * img_stride + img_offset_x;
#else
	img_y += img_offset_y * img_stride + img_offset_x;
	img_u += img_offset_y * img_stride + img_offset_x;
	img_v += img_offset_y * img_stride + img_offset_x;
	img_a += img_offset_y * img_stride + img_offset_x;
#endif
	if(osd_layer_rotate)
	{
		osd_y += (osd_offset_y * 1 + osd_offset_x * osd_h);
		if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420)
		{
			// YUV420
			osd_u += (osd_offset_y * 1/2 + osd_offset_x * osd_h) >> 1;
			osd_v += (osd_offset_y * 1/2 + osd_offset_x * osd_h) >> 1;
		}
		else //if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
		{
			// Y_UV420
			//osd_u += ((osd_offset_y >> 1) << 1) + osd_offset_x/2 * osd_h;
			osd_u += ((osd_offset_y >> 1) << 1) + osd_offset_x * osd_h/2;
			osd_v = osd_u + 1;
		}
	}
	else
	{
		osd_y += (osd_offset_y * osd_stride + osd_offset_x);
		if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420)
		{
			// YUV420
			osd_u += (osd_offset_y * osd_stride/2 + osd_offset_x) >> 1;
			osd_v += (osd_offset_y * osd_stride/2 + osd_offset_x) >> 1;
		}
		else //if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
		{
			// Y_UV420
			osd_u += (osd_offset_y * osd_stride/2) + ((osd_offset_x >> 1) << 1);
			osd_v = osd_u + 1;
		}		
	}

	copy_h >>= 1;
	copy_w >>= 1;
	for (j = 0; j < copy_h; j ++)
	{
		src_y = img_y;
		src_u = img_u;
		src_v = img_v;
		src_a = img_a;
		dst_y = osd_y;
		dst_u = osd_u;
		dst_v = osd_v;
		for (i = 0; i < copy_w; i ++)
		{
			unsigned int y, y2, u, v;
			unsigned int a1 = *src_a;
			unsigned int a2 = *(src_a + 1);
#if VERT_REVERSE
			unsigned int a3 = *(src_a - img_stride);
			unsigned int a4 = *(src_a + 1 - img_stride);
#else
			unsigned int a3 = *(src_a + img_stride);
			unsigned int a4 = *(src_a + 1 + img_stride);
#endif
			unsigned int temp;
		//	y = (*src_y * a1 + *dst_y * (0xFF - a1)) / 255;
			y  = (*src_y * a1 + *dst_y * (0xFF - a1)) >> 8;
			//*dst_y = (unsigned char)y;
			if(osd_layer_rotate)
				y2 = (*(src_y + 1) * a2 + *(dst_y + osd_h) * (0xFF - a2)) >> 8;
			else
				y2 = (*(src_y + 1) * a2 + *(dst_y + 1) * (0xFF - a2)) >> 8;
		//	y = (*(src_y + 1) * a2 + *(dst_y + 1) * (0xFF - a2)) / 255;
			//*(dst_y + 1) = (unsigned char)y;
			if(osd_layer_rotate)
			{
				*dst_y = y;
				*(dst_y + osd_h) = y2;
			}
			else
			{
				*(unsigned short *)dst_y = (unsigned short)(y | (y2 << 8));
			}
#if VERT_REVERSE
			if(osd_layer_rotate)
				y  = (*(src_y - img_stride) * a3 + *(dst_y + 1) * (0xFF - a3)) >> 8;
			else
				y  = (*(src_y - img_stride) * a3 + *(dst_y + osd_stride) * (0xFF - a3)) >> 8;
#else
			if(osd_layer_rotate)
				y  = (*(src_y + img_stride) * a3 + *(dst_y + 1) * (0xFF - a3)) >> 8;
			else
				y  = (*(src_y + img_stride) * a3 + *(dst_y + osd_stride) * (0xFF - a3)) >> 8;
#endif
		//	y = (*(src_y + img_stride) * a3 + *(dst_y + osd_stride) * (0xFF - a3)) / 255;
		//	*(dst_y + osd_stride) = (unsigned char)y;
#if VERT_REVERSE
			if(osd_layer_rotate)
				y2 = (*(src_y - img_stride + 1) * a4 + *(dst_y + 1 + osd_h) * (0xFF - a4)) >> 8;
			else
				y2 = (*(src_y - img_stride + 1) * a4 + *(dst_y + osd_stride + 1) * (0xFF - a4)) >> 8;
#else
			if(osd_layer_rotate)
				y2 = (*(src_y + img_stride + 1) * a4 + *(dst_y + 1 + osd_h) * (0xFF - a4)) >> 8;
			else
				y2 = (*(src_y + img_stride + 1) * a4 + *(dst_y + osd_stride + 1) * (0xFF - a4)) >> 8;
#endif
		//	y = (*(src_y + img_stride + 1) * a4 + *(dst_y + osd_stride + 1) * (0xFF - a4)) / 255;
		//	*(dst_y + osd_stride + 1) = (unsigned char)y;
			if(osd_layer_rotate)
			{
				*(dst_y + 1) = y;
				*(dst_y + 1 + osd_h) = y2;
			}
			else
			{
				*(unsigned short *)(dst_y + osd_stride) = (unsigned short)(y | (y2 << 8));
			}
#if 1
			temp = (0xFF*4 - a1 - a2 - a3 - a4);
			u  = *dst_u * temp;
			v  = *dst_v * temp;
			u += *src_u * a1;
			v += *src_v * a1;
			u += *(src_u + 1) * a2;
			v += *(src_v + 1) * a2;
#if VERT_REVERSE
			u += *(src_u - img_stride) * a3;
			v += *(src_v - img_stride) * a3;
			u += *(src_u - img_stride + 1) * a4;
			v += *(src_v - img_stride + 1) * a4;
#else
			u += *(src_u + img_stride) * a3;
			v += *(src_v + img_stride) * a3;
			u += *(src_u + img_stride + 1) * a4;
			v += *(src_v + img_stride + 1) * a4;
#endif
			u >>= 10;
			v >>= 10;
#else
			u  = (*src_u * a1                    + *dst_u * (0xFF - a1));
			u += (*(src_u + 1) * a2              + *dst_u * (0xFF - a2));
			u += (*(src_u + img_stride) * a3     + *dst_u * (0xFF - a3));
			u += (*(src_u + img_stride + 1) * a4 + *dst_u * (0xFF - a4));
			//u /= (0xFF*4);
			u >>= 10;
			//if(u < 16)
			//	u = 16;
			//else if(u > 240)
			//	u = 240;
			//if(u > 255)
			//	u = 255;

			/*
			if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420)
				*dst_u ++ = (unsigned char)u;
			else if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
			{
				*dst_u = (unsigned char)u;
				dst_u += 2;
			}*/

			v  = (*src_v * a1                    + *dst_v * (0xFF - a1));
			v += (*(src_v + 1) * a2              + *dst_v * (0xFF - a2));
			v += (*(src_v + img_stride) * a3     + *dst_v * (0xFF - a3));
			v += (*(src_v + img_stride + 1) * a4 + *dst_v * (0xFF - a4));
			// v /= (0xFF*4);
			v >>= 10;
			//if(v < 16)
			//	v = 16;
			//else if(v > 240)
			//	v = 240;
			//if(v > 255)
			//	v = 255;
#endif

			if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420)
			{
				if(osd_layer_rotate)
				{
					*dst_u  = (unsigned char)u;
					*dst_v  = (unsigned char)v;
					dst_u += osd_h >> 1;
					dst_v += osd_h >> 1;
				}
				else
				{
					*dst_u ++ = (unsigned char)u;
					*dst_v ++ = (unsigned char)v;
				}
			}
			else //if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
			{
				if(osd_layer_rotate)
				{
					*dst_u = (unsigned char)u;
					dst_u += osd_h;
					*dst_v = (unsigned char)v;
					dst_v += osd_h;
				}
				else
				{
					*dst_u = (unsigned char)u;
					dst_u += 2;
					*dst_v = (unsigned char)v;
					dst_v += 2;
				}
			}

			src_y += 2;
			src_u += 2;
			src_v += 2;
			src_a += 2;
			if(osd_layer_rotate)
			{
				dst_y += osd_h * 2;
			}
			else
				dst_y += 2;
		}

#if VERT_REVERSE
		img_y -= img_stride * 2;
		img_u -= img_stride * 2;
		img_v -= img_stride * 2;
		img_a -= img_stride * 2;
#else
		img_y += img_stride * 2;
		img_u += img_stride * 2;
		img_v += img_stride * 2;
		img_a += img_stride * 2;
#endif
		
		if(osd_layer_rotate)
		{
			osd_y += 2;
			if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420)
			{
				osd_u += 1;
				osd_v += 1;
			}
			else if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
			{
				osd_u += 2;
				osd_v += 2;
			}
			
		}
		else
		{
			osd_y += osd_stride * 2;
			if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420)
			{
				osd_u += osd_stride >> 1;
				osd_v += osd_stride >> 1;
			}
			else if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
			{
				osd_u += osd_stride;
				osd_v += osd_stride;
			}
		}
	}

}		


// 将YUV420格式源图像的指定区域(copy_w, copy_h)复制到OSD YUV层的指定位置(osd_offset_x, osd_offset_y)
// 仅YUV420 或者 Y_UV420层支持
void XM_lcdc_load_yuv_image (
						  unsigned char *yuv_img,		// YUV420格式的源图像
						  unsigned int yuv_format,		// XM_OSD_LAYER_FORMAT_YUV420 或者 XM_OSD_LAYER_FORMAT_Y_UV420
						  unsigned int img_w, unsigned int img_h,		// 源图像原始尺寸
						  unsigned int img_offset_x, unsigned int img_offset_y,
						  unsigned char *osd_y,			// OSD层 YUV地址
						  unsigned char *osd_u,
						  unsigned char *osd_v,
						  unsigned int osd_w, unsigned int osd_h,			// OSD层宽度、高度信息
						  unsigned int osd_offset_x, unsigned int osd_offset_y,			// 复制区域位于OSD层的偏移
						  unsigned int copy_w, unsigned int copy_h		// 源图像中需要复制的区域尺寸(从源图像左上角原点开始)
						  )
{
	unsigned int y;
	unsigned int x;
	unsigned char *src, *dst;

	if(yuv_format == XM_OSD_LAYER_FORMAT_YUV420)
	{
		if (osd_y == NULL || osd_u == NULL || osd_v == NULL)
		{
			XM_printf ("invalid YUV420 parameter\n");
			return;
		}
	}
	else if(yuv_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		if (osd_y == NULL || osd_u == NULL)
		{
			XM_printf ("invalid Y_UV420 parameter\n");
			return;
		}
	}
	else
	{
		XM_printf ("invalid YUV420/Y_UV420 parameter\n");
		return;
	}

	if(img_offset_x >= img_w)
		return;
	if(img_offset_y >= img_h)
		return;

	if( (img_offset_x + copy_w) > img_w )
		copy_w = img_w - img_offset_x;
	if( (img_offset_y + copy_h) > img_h )
		copy_h = img_h - img_offset_y;

	
	// copy Y
	src = yuv_img;
	src += img_offset_y * img_w + img_offset_x;
	dst = osd_y + osd_offset_y * osd_w + osd_offset_x;
	y = copy_h;
	while(y > 0)
	{
		for (x = 0; x < copy_w; x ++)
			dst[x] = src[x];
		dst += osd_w;
		src += img_w;
		y -- ;
	}
	
	if(yuv_format == XM_OSD_LAYER_FORMAT_YUV420)
	{
		// YUV420 U、V分块存放
		// copy U
		src = yuv_img + img_h * img_w;
		src += img_offset_y * img_w/4 + img_offset_x/2;
		dst = osd_u + osd_offset_y * osd_w/4 + osd_offset_x/2;
		y = copy_h/2;
		while(y > 0)
		{
			for (x = 0; x < copy_w/2; x ++)
				dst[x] = src[x];
			dst += osd_w/2;
			src += img_w/2;
			y --;
		}

		// copy V
		src = yuv_img + img_h * img_w + img_h * img_w/4;
		src += img_offset_y * img_w/4 + img_offset_x/2;
		dst = osd_v  + osd_offset_y * osd_w/4 + osd_offset_x/2;
		y = copy_h/2;
		while(y > 0)
		{
			for (x = 0; x < copy_w/2; x ++)
				dst[x] = src[x];
			dst += osd_w/2;
			src += img_w/2;
			y --;
		}
	}
	else if(yuv_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		// Y_UV420 UV合并在一块中，交替存放UVUV
		src = yuv_img + img_h * img_w;
		src += img_offset_y * img_w/2 + (img_offset_x/2)*2;
		dst = osd_u + osd_offset_y * osd_w/2 + (osd_offset_x/2)*2;
		y = copy_h/2;
		while(y > 0)
		{
			for (x = 0; x < copy_w; x ++)
				dst[x] = src[x];
			dst += osd_w;
			src += img_w;
			y --;
		}
	}
}


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
						  )
{
	XM_OSDLAYER *xm_osdlayer;
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return;
	}
	
	xm_osdlayer = &lcdc_osd_layer[lcd_channel][osd_layer];
	if(	xm_osdlayer->osd_layer_format != XM_OSD_LAYER_FORMAT_YUV420 
		&&	xm_osdlayer->osd_layer_format != XM_OSD_LAYER_FORMAT_Y_UV420 )
	{
		XM_printf ("illegal layer format(%d), only YUV420 or Y_UV420 layer support\n", xm_osdlayer->osd_layer_format);
		return;
	}

	XM_lcdc_load_yuv_image (
		yuv_img, 
		xm_osdlayer->osd_layer_format,
		img_w, img_h,
		img_offset_x, img_offset_y,
		xm_osdlayer->osd_y_addr, xm_osdlayer->osd_u_addr, xm_osdlayer->osd_v_addr,
		xm_osdlayer->osd_width, xm_osdlayer->osd_height,
		osd_offset_x, osd_offset_y,
		copy_w, copy_h);
}

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
						  )
{
	unsigned int file_size;
	char *yuv_image = NULL;
	int ret = 0;
	void *fp;


	if(yuv_format == XM_OSD_LAYER_FORMAT_YUV420)
	{
		if (osd_y == NULL || osd_u == NULL || osd_v == NULL)
			return 0;
	}
	else if(yuv_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		if (osd_y == NULL || osd_u == NULL)
			return 0;
	}
	else
	{
		// only XM_OSD_LAYER_FORMAT_YUV420 or XM_OSD_LAYER_FORMAT_Y_UV420 support
		return 0;
	}

	if( img_offset_x >= img_w )
		return 0;
	if( (img_w - img_offset_x) < copy_w )
		copy_w = img_w - img_offset_x;
	if( img_offset_y >= img_h )
		return 0;
	if( (img_h - img_offset_y) < copy_h )
		copy_h = img_h - img_offset_y;

	if( osd_offset_x > osd_w )
		return 0;
	if( osd_offset_y > osd_h )
		return 0;
	if( (osd_w - osd_offset_x) < copy_w )
		copy_w = osd_w - osd_offset_x;
	if( (osd_h - osd_offset_y) < copy_h )
		copy_h = osd_h - osd_offset_y;

	
	fp = XM_fopen ((const char *)yuv_file, "rb");
	do
	{
		if(fp == NULL)
			break;
		
		file_size = XM_filesize (fp);
		if(file_size == 0)
			break;
		
		yuv_image = XM_heap_malloc (file_size);
		if(yuv_image == NULL)
		{
			XM_printf ("XM_lcdc_load_yuv_image_file %s NG, XM_heap_malloc failed\n", yuv_file);
			break;
		}

		if(XM_fread (yuv_image, 1, file_size, fp) != file_size)
			break;

		XM_lcdc_load_yuv_image ((unsigned char *)yuv_image, 
												yuv_format,
												img_w, img_h,
												img_offset_x, img_offset_y,
												osd_y, osd_u, osd_v,
												osd_w, osd_h,
												osd_offset_x, osd_offset_y,
												copy_w, copy_h);
		ret = 1;
	} while (!ret);

	if(fp)
		XM_fclose (fp);
	if(yuv_image)
		XM_heap_free (yuv_image);

	return ret;
}

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
						  )
{
	XM_OSDLAYER *xm_osdlayer;
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 0;
	}
	
	xm_osdlayer = &lcdc_osd_layer[lcd_channel][osd_layer];
	return XM_lcdc_load_yuv_image_file (yuv_file,
												xm_osdlayer->osd_layer_format,
												img_w, img_h,
												img_offset_x, img_offset_y,
												xm_osdlayer->osd_y_addr,
												xm_osdlayer->osd_u_addr,
												xm_osdlayer->osd_v_addr,
												xm_osdlayer->osd_width, 
												xm_osdlayer->osd_height,
												osd_offset_x, osd_offset_y,
												copy_w, copy_h);
}

int XM_lcdc_osd_layer_load_gif_image_file (
									unsigned int lcd_channel,
									unsigned int osd_layer,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *gif_image_file,
									XM_COLORREF transparent_color,
									unsigned int alpha)
{
	XM_OSDLAYER *xm_osdlayer;
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 0;
	}
	
	xm_osdlayer = &lcdc_osd_layer[lcd_channel][osd_layer];

	return XM_lcdc_load_gif_image_file (xm_osdlayer->osd_y_addr,
												xm_osdlayer->osd_layer_format,
												xm_osdlayer->osd_width,
												xm_osdlayer->osd_height,
												xm_osdlayer->osd_stride,
												osd_image_x, osd_image_y,
												gif_image_file,
												transparent_color,
												alpha);
}

// 设置LCDC的UI层视频格式
void XM_lcdc_osd_set_ui_format (unsigned int lcd_channel, unsigned int layer_format)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	if(layer_format >= XM_OSD_LAYER_FORMAT_COUNT)
	{
		XM_printf ("illegal layer_format (%d)\n", layer_format);
		return;
	}
	osd_ui_format = layer_format;
}

// 读取LCDC的UI层视频格式
unsigned int XM_lcdc_osd_get_ui_format (unsigned int lcd_channel)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	return osd_ui_format;
}

extern void dma_flush_range(UINT32 ulStart, UINT32 ulEnd);
extern void dma_inv_range(UINT32 ulStart, UINT32 ulEnd);


int XM_lcdc_inv_region (unsigned char *osd_layer_buffer,	// OSD平面的缓存地址
											unsigned int   osd_layer_format,	// OSD平面的格式
											unsigned int osd_width, 	// OSD平面的宽度/高度
											unsigned int osd_height,
											unsigned int osd_stride,	// OSD平面的行字节长度
											unsigned int d_x,		// 脏区的偏移(相对于OSD平面的左上角原点)
											unsigned int d_y,
											unsigned int d_w,		// 脏区的尺寸
											unsigned int d_h
										)
{
	unsigned char *buf_y, *buf_uv;
	unsigned int i;
	
	// 将嵌入字符对应的脏区域写入到RAM
	if(osd_layer_format != XM_OSD_LAYER_FORMAT_Y_UV420)
		return -1;
	
	if(d_w == 0 || d_h == 0)
		return -1;
	
	if(osd_layer_buffer == NULL)
		return -1;
	
	/*
	buf_y  = osd_layer_buffer + osd_stride * d_y + d_x;						// Y分量脏区起始缓冲
	for (i = 0; i < d_h; i ++)
	{
		dma_inv_range ((unsigned int)buf_y,  d_w + (unsigned int)buf_y);
		buf_y += osd_stride;
	}
	*/
	dma_inv_range ( (unsigned int)(osd_layer_buffer + osd_stride * d_y) , 
						 (unsigned int)(osd_layer_buffer + osd_stride * (d_y + d_h)));

	/*
	buf_uv = osd_layer_buffer + osd_stride * osd_height + osd_stride * d_y / 2 + ((d_x >> 1) << 1);		// UV分量脏区起始缓冲
	for (i = 0; i < d_h/2; i ++)
	{
		dma_inv_range ((unsigned int)buf_uv, d_w + (unsigned int)buf_uv);
		buf_uv += osd_stride;
	}
	*/
	dma_inv_range ( (unsigned int)(osd_layer_buffer + osd_stride * osd_height + osd_stride * d_y/2) , 
						 (unsigned int)(osd_layer_buffer + osd_stride * osd_height + osd_stride * (d_y + d_h)/2));
	
	return 0;
}	

int XM_lcdc_flush_dirty_region (unsigned char *osd_layer_buffer,	// OSD平面的缓存地址
											unsigned int   osd_layer_format,	// OSD平面的格式
											unsigned int osd_width, 	// OSD平面的宽度/高度
											unsigned int osd_height,
											unsigned int osd_stride,	// OSD平面的行字节长度
											unsigned int d_x,		// 脏区的偏移(相对于OSD平面的左上角原点)
											unsigned int d_y,
											unsigned int d_w,		// 脏区的尺寸
											unsigned int d_h
										)
{
	unsigned char *buf_y, *buf_uv;
	unsigned int i;
	
	unsigned int a_x, a_y, a_w, a_h;
	
	// 将嵌入字符对应的脏区域写入到RAM
	if(osd_layer_format != XM_OSD_LAYER_FORMAT_Y_UV420)
		return -1;
	
	if(d_w == 0 || d_h == 0)
		return -1;
	
	if(osd_layer_buffer == NULL)
		return -1;
	
	//a_x = d_x & ~31;
	//a_w = ((d_x + d_w + 16) & ~31) - a_x;
	
	dma_flush_range ( (unsigned int)(osd_layer_buffer + osd_stride * d_y) , 
						   (unsigned int)(osd_layer_buffer + osd_stride * (d_y + d_h)));
		
	/*
	buf_y  = osd_layer_buffer + osd_stride * d_y + a_x;						// Y分量脏区起始缓冲
	for (i = 0; i < d_h; i ++)
	{
		dma_flush_range ((unsigned int)buf_y,  a_w + (unsigned int)buf_y);
		buf_y += osd_stride;
	}
	*/
	dma_flush_range ( (unsigned int)(osd_layer_buffer + osd_stride * osd_height + osd_stride * d_y/2) , 
						   (unsigned int)(osd_layer_buffer + osd_stride * osd_height + osd_stride * (d_y + d_h)/2));
						 
	/*			 
	buf_uv = osd_layer_buffer + osd_stride * osd_height + osd_stride * d_y / 2 + ((a_x >> 1) << 1);		// UV分量脏区起始缓冲
	for (i = 0; i < d_h/2; i ++)
	{
		dma_flush_range ((unsigned int)buf_uv, a_w + (unsigned int)buf_uv);
		buf_uv += osd_stride;
	}
	*/
	
	//dma_flush_range ( (unsigned int)(osd_layer_buffer) , (unsigned int)(osd_layer_buffer + osd_width * osd_height * 3/2));
	
	return 0;
}	

// 将LCD屏幕坐标转换为OSD层的逻辑坐标
// 返回值
//		-1		失败
//		0		成功
int XM_lcdc_lcd_coordinate_to_osd_coordinate (unsigned int lcd_channel, unsigned int osd_layer, int *x, int *y)
{
	XM_OSDLAYER *xm_osdlayer = &lcdc_osd_layer[lcd_channel][osd_layer];
	int lcd_x;
	int lcd_y;
	int lcd_w, lcd_h;

	if(x == 0 || y == 0)
		return -1;

	lcd_x = *x;
	lcd_y = *y;
	if(lcd_x < 0 || lcd_y < 0)
		return -1;

	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return -1;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return -1;
	}
	lcd_w = HW_lcdc_get_xdots (lcd_channel);
	lcd_h = HW_lcdc_get_ydots (lcd_channel);
	if(lcd_x >= lcd_w || lcd_y >= lcd_h)
		return -1;

	if(!xm_osdlayer->osd_enable)
		return -1;
	if(xm_osdlayer->osd_width == 0 || xm_osdlayer->osd_height == 0)
		return -1;

	lcd_x -= xm_osdlayer->osd_x_position;
	lcd_y -= xm_osdlayer->osd_y_position;
	if(lcd_x < 0 || lcd_x >= xm_osdlayer->osd_width)
		return -1;
	if(lcd_y < 0 || lcd_y >= xm_osdlayer->osd_height)
		return -1;

	*x = lcd_x;
	*y = lcd_y;
	return 0;
}
