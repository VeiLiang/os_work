#include "hardware.h"
#include  <stdio.h>
#include  "lcd.h"
#include "hw_osd_layer.h"
#include "xm_printf.h"
#include "xm_dev.h"
#include "xm_base.h"

static unsigned char*reg_osd_y_addr[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT] = {0};
static unsigned char*reg_osd_u_addr[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT] = {0};
static unsigned char*reg_osd_v_addr[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT] = {0};

// 回调函数
static osd_callback_t lcdc_osd_callback[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT] = {0};
static void* lcdc_osd_private[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT] = {0};
 
// LCDC物理尺寸
static unsigned int lcdc_xdots[XM_LCDC_CHANNEL_COUNT];
static unsigned int lcdc_ydots[XM_LCDC_CHANNEL_COUNT];
// 输出通道类型
static unsigned int lcdc_type[XM_LCDC_CHANNEL_COUNT];

// LCDC输出通道使能标志
static unsigned int lcdc_enable[XM_LCDC_CHANNEL_COUNT] = {0};
// LCDC输出通道显示开启关闭标志
static unsigned int lcdc_display_on[XM_LCDC_CHANNEL_COUNT] = {0};
static float lcdc_backlight_on[XM_LCDC_CHANNEL_COUNT] = {0.0};

static void *lcdc_semaphore[XM_LCDC_CHANNEL_COUNT] = {0};

static struct OS_EVENT lcdc_v_syn_event;		// 场同步事件

static const char *lcdc_channel_name (unsigned int lcdc_channel)
{
	if(lcdc_channel == XM_LCDC_CHANNEL_0)
		return "LCDC CH0";
#if XM_LCDC_CHANNEL_COUNT > 1
	else if(lcdc_channel == XM_LCDC_CHANNEL_1)
		return "LCDC CH1";
#endif
	else
		return "null";
}

const char *XM_lcdc_channel_type (unsigned int lcdc_type)
{
	if(lcdc_type == XM_LCDC_TYPE_RGB)
		return "RGB";
	else if(lcdc_type == XM_LCDC_TYPE_CPU)
		return "CPU";
	else if(lcdc_type == XM_LCDC_TYPE_SRGB)
		return "SRGB";
	else if(lcdc_type == XM_LCDC_TYPE_BT601)
		return "BT601";
	else if(lcdc_type == XM_LCDC_TYPE_VGA)
		return "VGA";
	else if(lcdc_type == XM_LCDC_TYPE_SVGA)
		return "SVGA";
	else if(lcdc_type == XM_LCDC_TYPE_CVBS_NTSC)
		return "CVBS NTSC";
	else if(lcdc_type == XM_LCDC_TYPE_CVBS_PAL)
		return "CVBS PAL";
	else if(lcdc_type == XM_LCDC_TYPE_YPbPr_480i)
		return "YPbPr 480i";
	else if(lcdc_type == XM_LCDC_TYPE_YPbPr_480p)
		return "YPbPr 480p";
	else if(lcdc_type == XM_LCDC_TYPE_YPbPr_576i)
		return "YPbPr 576i";
	else if(lcdc_type == XM_LCDC_TYPE_YPbPr_576p)
		return "YPbPr 576p";
	else if(lcdc_type == XM_LCDC_TYPE_HDMI_1080i)
		return "HDMI 1080i";
	else if(lcdc_type == XM_LCDC_TYPE_HDMI_1080p)
		return "HDMI 1080p";

	else
		return "null";
}

// LCDC输出通道初始化
// lcd_channel		LCDC输出设备通道号(显示设备)
// xdots				LCDC输出设备像素宽度 
// ydots				LCDC输出设备像素高度
void HW_lcdc_init (unsigned int lcdc_channel, unsigned int lcd_type, unsigned int xdots, unsigned int ydots)
{
	if(lcdc_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcdc_channel);
		return;
	}

	XM_printf ("lcdc_type(%s) init\n", XM_lcdc_channel_type(lcd_type));
	if(lcd_type == XM_LCDC_TYPE_RGB )
	{
		//LCD_SelPad();
		//LCD_Init();
		//LCD_Read_OSDINFO();
		//LCD_SelPad();
		//LCD_Init();
#if RGB_480X272
		XM_printf("RGB_480X272\r\n");
		HW_LCD_Init ();
#endif
		
#if RGB_848X480
		XM_printf("RGB_848X480_R90\r\n");
		HW_LCD_Init ();
#endif

#if RGB_800X480
		XM_printf("RGB_800X480\r\n");
		HW_LCD_Init ();
#endif

#if RGB_1600X400
		XM_printf("RGB_1600X400\r\n");
		HW_LCD_Init ();
#endif

#if RGB_1024X600
		XM_printf("RGB_1024X600\r\n");
		HW_LCD_Init ();
#endif

	}
	else if(lcd_type == XM_LCDC_TYPE_HDMI_720p)
	{
#if HDMI_720P
		XM_printf("HDMI_720P\r\n");
		HW_HDMI_Init();
#endif
	}
	else if(lcd_type == XM_LCDC_TYPE_CPU )
	{
		// CPU屏配置管脚
		//LCD_CPU_SelPad ();
		//CS_LP035HV38_LCD_Init();
		//CS_LP035HV38_Screen_Init();
	}
	else if(lcd_type == XM_LCDC_TYPE_SRGB )
	{
#if SRGB_320X240
		printf("SRGB_320X240 init\r\n");
		HW_SRGB_Init ();
#endif
	}
	
	else if(lcd_type == XM_LCDC_TYPE_CVBS_PAL)
	{
#ifdef    _SUPPORT_TV_PAL_
		XM_printf("TV PAL 720*576\r\n");
		HW_tv_pal_init ();
#endif
	}
	
	else if(lcd_type == XM_LCDC_TYPE_CVBS_NTSC)
	{
#ifdef    _SUPPORT_TV_NTSC_
		XM_printf("TV NTSC 720*480\r\n");
		HW_tv_ntsc_init ();
#endif
	}
	
	else if(lcd_type == XM_LCDC_TYPE_YPbPr_480p)
	{
		XM_printf("YPbPr 720*480 P\r\n");
		//HW_ypbpr_480p_init ();
	}
	else if(lcd_type == XM_LCDC_TYPE_YPbPr_480i)
	{
		XM_printf("YPbPr 720*480 I\r\n");
		//HW_ypbpr_480i_init ();
	}
	else if(lcd_type == XM_LCDC_TYPE_YPbPr_576p)
	{
		XM_printf("YPbPr 720*576 P\r\n");
		//HW_ypbpr_576p_init ();
	}
	else if(lcd_type == XM_LCDC_TYPE_YPbPr_576i)
	{
		XM_printf("YPbPr 720*576 I\r\n");
		//HW_ypbpr_576i_init ();
	}
	else if(lcd_type == XM_LCDC_TYPE_VGA )
	{
		XM_printf("VGA 640*480 I\r\n");
	      //HW_vga_640480_init ();
	}
	else if(lcd_type == XM_LCDC_TYPE_SVGA )
	{
		XM_printf("VGA 640*480 I\r\n");
		//HW_vga_800600_init ();
	}
	else
	{
		XM_printf ("lcdc_type(%s) don't support now\n", XM_lcdc_channel_type(lcd_type));
		return;
	}
	
	lcdc_type[lcdc_channel] = lcd_type;
	lcdc_xdots[lcdc_channel] = xdots;
	lcdc_ydots[lcdc_channel] = ydots;
	
	request_irq(LCD_INT, PRIORITY_FIFTEEN, HW_lcdc_interrupt_handler );	
	
	//lcdc_semaphore[lcdc_channel] = XM_CreateSemaphore ("lcdc_semaphore_0", 1);
}

// LCDC输出通道关闭(关闭时钟、关闭控制逻辑)
void HW_lcdc_exit (unsigned int lcdc_channel)
{
	if(lcdc_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcdc_channel);
		return;
	}
	
	XM_printf ("lcdc_type(%s) exit\n", 
						  	XM_lcdc_channel_type(lcdc_type[lcdc_channel]));
	
	request_irq(LCD_INT, PRIORITY_FIFTEEN, 0 );	
	
	switch (lcdc_type[lcdc_channel])
	{
		case XM_LCDC_TYPE_RGB:
			// 关闭时钟、关闭控制逻辑
			//HW_LCD_Exit();
			break;

		case XM_LCDC_TYPE_CPU:
			// 关闭时钟、关闭控制逻辑
		//	LCD_CPU_Exit();
			break;
			
		case XM_LCDC_TYPE_SRGB:
#if SRGB_320X240
			// 关闭时钟、关闭控制逻辑
			HW_SRGB_Exit();
#endif
			break;
			
		case XM_LCDC_TYPE_CVBS_PAL:
		//	HW_tv_pal_exit ();
			break;
			
		case XM_LCDC_TYPE_CVBS_NTSC:
		//	HW_tv_ntsc_exit ();
			break;
			
		case XM_LCDC_TYPE_YPbPr_480p:
		//	HW_ypbpr_480p_exit ();
			break;
		
		case XM_LCDC_TYPE_YPbPr_480i:
		//	HW_ypbpr_480i_exit ();
			break;
			
		case XM_LCDC_TYPE_YPbPr_576p:
		//	HW_ypbpr_576p_exit ();
			break;

		case XM_LCDC_TYPE_YPbPr_576i:
		//	HW_ypbpr_576i_exit ();
			break;

		default:
			XM_printf ("lcdc_type(%s) don't support HW_lcdc_exit now\n", 
						  	XM_lcdc_channel_type(lcdc_type[lcdc_channel]));
			break;
	}
	
	lcdc_type[lcdc_channel] = (unsigned int)(-1);
	lcdc_xdots[lcdc_channel] = 0;
	lcdc_ydots[lcdc_channel] = 0;
	//lcdc_enable[lcdc_channel] = 0;
	
	
}

// 获取CDC输出通道的平面宽度
unsigned int HW_lcdc_get_xdots (unsigned int lcd_channel)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	return lcdc_xdots[lcd_channel];
}

// 获取CDC输出通道的平面高度
unsigned int HW_lcdc_get_ydots (unsigned int lcd_channel)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	return lcdc_ydots[lcd_channel];
}

// osd layer开启/关闭
void HW_lcdc_osd_set_enable (unsigned int lcd_channel, unsigned int osd_layer, int enable)
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
	if(enable)
	{
		// 检查显示缓存地址是否设置
		if(reg_osd_y_addr[lcd_channel][osd_layer] == NULL )
		{
			XM_printf ("ERROR, lcdc(%s) osd(%d) enabled before YUV address setup\n", 
						  lcdc_channel_name(lcd_channel), osd_layer);
			return;
		}
	}
	
	if(XM_GetFmlDeviceCap(DEVCAP_OSD))
	{
		if(enable)
			LCD_Layer_Set (osd_layer);
		else
			LCD_Layer_Clr (osd_layer);
	}
	/*
	switch( lcdc_type[lcd_channel] )
	{
		case XM_LCDC_TYPE_RGB:
			if(enable)
				LCD_Layer_Set (osd_layer);
			else
				LCD_Layer_Clr (osd_layer);
			break;
			
		default:
			XM_printf ("ERROR, lcdc_type(%s) don't implement HW_lcdc_osd_set_enable\n", 
						  XM_lcdc_channel_type(lcdc_type[lcd_channel]));
			break;
	}
	*/
	
	//reg_osd_enable[lcd_channel][osd_layer] = (unsigned char)enable;
	
	//XM_printf ("LCD channel(%d)'s OSD layer(%d) %s\n", lcd_channel, osd_layer, enable ? "ENABLE" : "DISABLE");
}

void HW_lcdc_osd_set_global_coeff_enable (unsigned int lcd_channel, unsigned int osd_layer, int enable)
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
	//if( lcdc_type[lcd_channel] == XM_LCDC_TYPE_RGB )
	{
		if(enable )
			LCD_Set_Layer_Alpha(osd_layer );
		else
			LCD_Set_Src_Alpha( osd_layer);
	}
}

// 设置LCDC输出通道背景色
void HW_lcdc_set_background_color (unsigned int lcd_channel, 
													unsigned char r, unsigned char g, unsigned char b)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	LCD_Set_background_color (r,g,b);
}

void HW_lcdc_osd_set_global_coeff (unsigned int lcd_channel, unsigned int osd_layer, unsigned int global_coeff)
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
	LCD_Layer_set_global_coeff(osd_layer , global_coeff );
}

void HW_lcdc_osd_set_format (unsigned int lcd_channel, unsigned int osd_layer, unsigned int format)
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
	LCD_Layer_Set_Mode( osd_layer , format );
}
int HW_lcdc_osd_get_format (unsigned int lcd_channel, unsigned int osd_layer)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 999;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return 999;
	}
	return LCD_Layer_Get_Mode(osd_layer);
}

void HW_lcdc_osd_set_width (unsigned int lcd_channel, unsigned int osd_layer, unsigned int width)
{
	int val;
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
	
	val = LCD_Layer_Get_Mode( osd_layer ) ;
	if(val == XM_OSD_LAYER_FORMAT_ARGB888 )
	{
		if(width > 1024)
		{
			XM_printf ("illegal width (%d >=1024) in FORMAT_ARGB888 \n", width);
			return;
		}
	}
	else
	{
		if(width> 2048)
		{
			XM_printf ("illegal width (%d >=2048) in YUV420 RGB565 RGB454 \n", width);
			return;
		}
	}
	LCD_Layer_set_width( osd_layer , width );
}


void HW_lcdc_osd_set_height (unsigned int lcd_channel, unsigned int osd_layer, unsigned int height)
{
	int val;
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

	val = LCD_Layer_Get_Mode( osd_layer ) ;
	if(val == XM_OSD_LAYER_FORMAT_ARGB888 )
	{
		if(height > 1024)
		{
			XM_printf ("illegal height (%d >=1024) in FORMAT_ARGB888 \n", height);
			return;
		}
	}
	else
	{
		if(height > 2048)
		{
			XM_printf ("illegal height (%d >=2048) in YUV420/RGB565/RGB454 \n", height);
			return;
		}
	}
	LCD_Layer_set_height( osd_layer , height );
}


void HW_lcdc_osd_set_h_position (unsigned int lcd_channel, unsigned int osd_layer, unsigned int h_position)
{
	unsigned int mode;
	
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
	if(h_position > OSD_MAX_H_POSITION)
	{
		XM_printf ("illegal osd_h_position (%d)\n", h_position);
		return;
	}
	//水平方向移动注意  YUV必须16个点移动，其他4个移动
	mode = LCD_Layer_Get_Mode( osd_layer );
	if(mode)
		h_position &= ~0x3;//h_position//step=4;
	else
		h_position &= ~0xf;//step=16;
	LCD_Layer_h_position( osd_layer , h_position );
}

void HW_lcdc_osd_set_v_position (unsigned int lcd_channel, unsigned int osd_layer, unsigned int v_position)
{
	int mode;
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
	if(v_position > OSD_MAX_V_POSITION)
	{
		XM_printf ("illegal osd_v_position (%d)\n", v_position);
		return;
	}
	{//垂直方向移动注意  YUV必须双数移动
		mode = LCD_Layer_Get_Mode( osd_layer );
		if( mode==XM_OSD_LAYER_FORMAT_YUV420 )
			v_position &= ~0x1;//h_position//step=4;
		LCD_Layer_v_position( osd_layer , v_position );
	}
}

void HW_lcdc_osd_set_left_position (unsigned int lcd_channel, unsigned int osd_layer, unsigned int left_position)
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
	if(left_position > OSD_MAX_H_POSITION)
	{
		XM_printf ("illegal left_position (%d)\n", left_position);
		return;
	}
	LCD_Layer_set_left_position( osd_layer , left_position);
}


void HW_lcdc_osd_set_stride (unsigned int lcd_channel, unsigned int osd_layer, unsigned int stride)
{
	int val;
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
		
	val = LCD_Layer_Get_Mode( osd_layer ) ;
	if(val == XM_OSD_LAYER_FORMAT_ARGB888 )
	{
		if(stride > 1024)
		{
			XM_printf ("illegal stride (%d >=1024) in FORMAT_ARGB888 \n", stride);
			return;
		}
	}
	else
	{
		if(stride > 2048)
		{
			XM_printf ("illegal stride (%d >=2048) in YUV420/RGB565/RGB454 \n", stride);
			return;
		}
	}
	LCD_Layer_set_stride( osd_layer , stride);
}


void HW_lcdc_osd_set_brightness_coeff (unsigned int lcd_channel, unsigned int osd_layer, unsigned int brightness_coeff)
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
	if(brightness_coeff >= 0xFF)
	{
		XM_printf ("illegal brightness_coeff (%d)\n", brightness_coeff);
		return;
	}
		
	LCD_Layer_set_brightness_coeff(osd_layer , brightness_coeff );
}


void HW_lcdc_osd_set_yuv_addr (unsigned int lcd_channel, unsigned int osd_layer, 
										 unsigned char *y_addr, unsigned char *u_addr, unsigned char *v_addr)
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
		
	LCD_Layer_fill_address(osd_layer , (unsigned int)y_addr,(unsigned int)u_addr,(unsigned int)v_addr);
	reg_osd_y_addr[lcd_channel][osd_layer] = y_addr;
	reg_osd_u_addr[lcd_channel][osd_layer] = u_addr;
	reg_osd_v_addr[lcd_channel][osd_layer] = v_addr;
}

void HW_lcdc_osd_set_callback (unsigned int lcd_channel, unsigned int osd_layer, 
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

	// 关中断
	XM_lock ();

	lcdc_osd_callback[lcd_channel][osd_layer] = osd_callback;
	lcdc_osd_private[lcd_channel][osd_layer] = osd_private;

	// 开中断
	XM_unlock ();
}

// LCDC帧刷新中断处理
void HW_lcdc_interrupt_handler (void)
{
	unsigned int LCD_STATUS = LCD_STATUS_reg;
	// 清除中断
	LCD_INTR_CLR_reg = LCD_STATUS & 0x07;
	if( LCD_STATUS & (0x01 << 0) )	// Lcd_done_intr LCD场结束中断状态位
	{
		// 进入消隐区, 切换framebuffer
	}
	if( LCD_STATUS & (0x01 << 1) )	// V_syn_intr LCD场同步中断状态位
	{
		// 进入刷新区, 表示帧切换完成
		HW_lcdc_set_vsyn_event ();
	}
	if( LCD_STATUS & (0x1 << 2) )		// Lcd_vsyn_pos_intr LCD场方向任意位置中断状态
	{
		
	}
}



// 开启/关闭LCDC视频通道的显示
// on  1  开启显示
// on  0  关闭显示
void HW_lcdc_set_display_on (unsigned int lcdc_channel, unsigned int on)
{
	if(lcdc_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcdc_channel);
		return;
	}
	lcdc_display_on[lcdc_channel] = on;
}

// 开启/关闭LCDC视频通道的背光显示
// lcdc_channel	LCDC视频输出通道号(显示设备)
// on  0.0 ~ 1.0  
//     0.0  背光关闭
//     1.0  背光最大亮度
void HW_lcdc_set_backlight_on (unsigned int lcdc_channel, float on)
{
	if(lcdc_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcdc_channel);
		return;
	}
	if(on < 0.0 || on > 1.0)
	{
		XM_printf ("illegal backlight on(%f)\n", on);
		return;
	}
	lcdc_backlight_on[lcdc_channel] = on;
	
#if DEMO_BB || DEV_BB_176
	if(on)
		HW_LCD_BackLightOn ();
	else
		HW_LCD_BackLightOff ();
#elif TULV
	//HW_LCD_BackLightSetLevel (on);
	#if 1
	if(on)
		HW_LCD_BackLightOn ();
	else
		HW_LCD_BackLightOff ();

	#endif
#endif
}
void HW_lcdc_set_osd_on (unsigned int lcdc_channel, float on)
{
	if(lcdc_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcdc_channel);
		return;
	}
	if(on < 0.0 || on > 1.0)
	{
		XM_printf ("illegal backlight on(%f)\n", on);
		return;
	}
	lcdc_backlight_on[lcdc_channel] = on;
	
}
// 获取LCDC视频通道的背光显示亮度
// 返回值 0.0 ~ 1.0
//     0.0  背光关闭
//     1.0  背光最大亮度
float HW_lcdc_get_backlight_on (unsigned int lcdc_channel)
{
	if(lcdc_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcdc_channel);
		return 0.0;
	}
	return lcdc_backlight_on[lcdc_channel];
}

void HW_lcdc_set_osd_coef_syn (unsigned int lcdc_channel, unsigned int coef_syn)
{
	unsigned long start, times;
	if(lcdc_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcdc_channel);
		return;
	}
	if(coef_syn)
	{
		// LCD_cpu_osd_coef_syn_reg = coef_syn;
		// 等待硬件清除
		times = XM_GetTickCount () + 50;
		// while ( (LCD_cpu_osd_coef_syn_reg & coef_syn) )
		while ( (LCD_cpu_osd_coef_syn_reg ) )
		{
			//times = abs(XM_GetTickCount () - start);
			//if(times >= 50)	// 最大延迟50ms
			//	break;
			if(XM_GetTickCount() >= times)
				break;
			OS_Delay (1);
		}
		LCD_cpu_osd_coef_syn_reg = coef_syn;
	}
}

unsigned int HW_lcdc_get_osd_coef_syn (unsigned int lcdc_channel)
{
	if(lcdc_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcdc_channel);
		return 0;
	}
	return LCD_cpu_osd_coef_syn_reg & 1;	
}

void arkn141_lcdc_init (void)
{
	sys_clk_disable (lcd_xclk_enable);
	sys_clk_disable (Tv_out_lcd_pixel_clk_enable);
	sys_clk_disable (lcd_out_clk_enable);
	sys_clk_disable (lcd_clk_enable);
	
	sys_soft_reset (softreset_lcd);
	delay (100);
	sys_clk_enable (lcd_xclk_enable);
	sys_clk_enable (Tv_out_lcd_pixel_clk_enable);
	sys_clk_enable (lcd_out_clk_enable);
	sys_clk_enable (lcd_clk_enable);
	
	OS_EVENT_Create (&lcdc_v_syn_event);
}

void arkn141_lcdc_exit (void)
{
	OS_EVENT_Delete (&lcdc_v_syn_event);
}


void HW_lcdc_reset_vsyn_event (void)
{
	OS_EVENT_Reset(&lcdc_v_syn_event);
}

void HW_lcdc_set_vsyn_event (void)
{
	OS_EVENT_Set(&lcdc_v_syn_event);
}

void HW_lcdc_waiting_for_vsyn_event (unsigned int timeout)
{
	OS_EVENT_WaitTimed(&lcdc_v_syn_event, timeout);
}
