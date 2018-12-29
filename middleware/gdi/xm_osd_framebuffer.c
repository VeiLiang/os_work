//****************************************************************************
//
//	Copyright (C) 2012~2014 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_osd_framebuffere.c
//
//	Revision history
//
//		2012.01.11	ZhuoYongHong initial version
//		2014.02.27	ZhuoYongHong修改，适应ARK1960 LCDC控制器
//
//****************************************************************************
// OSD产品配置，设置外部OSD所需的参数

// LCD通道配置(使用RGB屏)

#include <string.h>
#include "rtos.h"
#include "xm_type.h"
#include "hw_osd_layer.h"
#include "xm_osd_layer.h"
#include "xm_semaphore.h"
#include "xm_queue.h"
#include "xm_osd_framebuffer.h"
#include "xm_printf.h"
#include "xm_dev.h"
#include "xm_user.h"
#include "xm_gdi.h"
#include "xm_base.h"
#include <assert.h>


#ifdef WIN32	// Win32模拟环境下使用两路LCDC输出，便于调试UI
#define	PROJ_LCDC_CHANNEL_COUNT		1		// 项目定义的视频输出通道个数为2
#else
#define	PROJ_LCDC_CHANNEL_COUNT		1		// 项目定义的视频输出通道个数为1
#endif

extern void dma_clean_range(UINT32 ulStart, UINT32 ulEnd);
extern void dma_flush_range(UINT32 ulStart, UINT32 ulEnd);


static void *osd_framebuffer_semaphore;				// 线程互斥访问保护信号量

// 定义framebuffer对象的空闲/已使用双向链表
static queue_s osd_framebuffer_free_link[PROJ_LCDC_CHANNEL_COUNT][XM_OSD_FRAMEBUFFER_TYPE_COUNT];		
																	// YUV/RGB空闲对象链表

static queue_s osd_framebuffer_live_link[PROJ_LCDC_CHANNEL_COUNT][XM_OSD_FRAMEBUFFER_TYPE_COUNT];		
																	// YUV/RGB已使用对象链表

// 定义framebuffer对象
static XM_OSD_FRAMEBUFFER osd_yuv_framebuffer[PROJ_LCDC_CHANNEL_COUNT][OSD_YUV_FRAMEBUFFER_COUNT];
static XM_OSD_FRAMEBUFFER osd_rgb_framebuffer[PROJ_LCDC_CHANNEL_COUNT][OSD_RGB_FRAMEBUFFER_COUNT];

// 资源不可用标志, 1 表示资源不可用 0 表示资源已准备好,可以使用
static int osd_framebuffer_busy;

static unsigned int get_framebuffer_index_from_osd_layer (unsigned int osd_layer)
{
	if(osd_layer == XM_OSD_LAYER_0)	// Layer 0 固定为YUV视频层
	{
		// YUV空闲帧链表
		return XM_OSD_FRAMEBUFFER_TYPE_YUV;
	}
	else
	{
		// 所有RGB层共用同一个RGB的framebuffer实例单元池
		// RGB空闲帧链表
		return XM_OSD_FRAMEBUFFER_TYPE_RGB;
	}
}

static void framebuffer_release (XM_OSD_FRAMEBUFFER *framebuffer)
{
	int framebuffer_index;
	unsigned int lcd_channel;
	if(framebuffer == NULL)
		return;
	assert (framebuffer->osd_layer != (unsigned int)(-1));
	assert (framebuffer->lcd_channel != (unsigned int)(-1));

	framebuffer_index = get_framebuffer_index_from_osd_layer (framebuffer->osd_layer);
	// 从"已使用"链表中断开
	queue_delete ((queue_s *)framebuffer);

	lcd_channel = framebuffer->lcd_channel;
	// 标记framenuffer无效
	
	framebuffer->lcd_channel = (unsigned int)(-1);
	framebuffer->osd_layer = (unsigned int)(-1);
	framebuffer->view_handle = NULL;
	framebuffer->configed = 0;
	framebuffer->format = 0;
	framebuffer->offset_x = 0;
	framebuffer->offset_y = 0;
	framebuffer->width = 0;
	framebuffer->height = 0;
	framebuffer->stride = 0;
	framebuffer->move_control.osd_step_count = 0;

	// 插入到空闲链表
	queue_insert ((queue_s *)framebuffer, &osd_framebuffer_free_link[lcd_channel][framebuffer_index]);
}

// osd层参数检查
static int lcdc_osd_layer_verify (unsigned int lcd_channel, unsigned int osd_layer)
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

static XM_OSD_FRAMEBUFFER *get_last_framebuffer (unsigned int lcd_channel, 
															 unsigned int osd_layer,
															 unsigned int layer_format,
															 unsigned int layer_width, 
														    unsigned int layer_height)
{
	int framebuffer_index;
	XM_OSD_FRAMEBUFFER *framebuffer = NULL;

	if(!lcdc_osd_layer_verify (lcd_channel, osd_layer))
		return NULL;
	
	framebuffer_index = get_framebuffer_index_from_osd_layer (osd_layer);
	
	// 从已使用链表取出最近一次创建的framebuffer对象(匹配LCDC通道及OSD层属性)
	if(!queue_empty(&osd_framebuffer_live_link[lcd_channel][framebuffer_index]))
	{
		// 遍历"已使用"链表
		queue_s *framebuffer_head = &osd_framebuffer_live_link[lcd_channel][framebuffer_index];
		XM_OSD_FRAMEBUFFER *framebuffer_link;
		if(!queue_empty(framebuffer_head))
		{
			// 遍历链表,
			framebuffer_link = (XM_OSD_FRAMEBUFFER *)queue_next (framebuffer_head);
			while( (queue_s *)framebuffer_link != framebuffer_head)
			{
				if(	framebuffer_link->lcd_channel == lcd_channel 
					&& framebuffer_link->osd_layer == osd_layer
					&& framebuffer_link->format == layer_format
					&& framebuffer_link->width == layer_width
					&& framebuffer_link->height == layer_height
					)
				{
					framebuffer = framebuffer_link;
					break;
				}
				framebuffer_link = framebuffer_link->next;
			}
		}
	}

	return framebuffer;
}

// 创建一个新的framebuffer对象 (framebuffer), 一个framebuffer对象实例化(XM_osd_framebuffer_close)后与一个OSD物理层关联。
// 当无framebuffer对象资源可用时，返回NULL
// 若指定OSD层的格式为YUV420, YUV平面数据连续存放
// 当OSD层新的framebuffer显示(XM_osd_framebuffer_display)时，最近创建的framebuffer实例将自动释放用于循环使用。
XM_OSD_FRAMEBUFFER * XM_osd_framebuffer_create (
																unsigned int lcd_channel, 
																unsigned int osd_layer,
																unsigned int layer_format,
																unsigned int layer_width, 
																unsigned int layer_height,
																HANDLE view_handle,
																unsigned int copy_last_framebuffer_context,
																unsigned int clear_framebuffer_context
																)
{
	queue_s *free_link;
	XM_OSD_FRAMEBUFFER *framebuffer = NULL;
	int framebuffer_index;

	// 检查显示屏是否已连接
	if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_DISCONNECT)
		return NULL;
	
	// 检查OSD是否已关闭
	if(!XM_GetFmlDeviceCap(DEVCAP_OSD))
	{
		// OSD off
		return NULL;
	}

	if(!lcdc_osd_layer_verify (lcd_channel, osd_layer))
	{
		XM_printf ("illegal lcd_channel (%d) or osd_layer(%d)\n", lcd_channel, osd_layer);
		return NULL;
	}

	if(layer_format >= XM_OSD_LAYER_FORMAT_COUNT)
	{
		XM_printf ("illegal layer_format (%d)\n", layer_format);
		return NULL;
	}
	// 检查framebuffer对象的尺寸是否超出视频输出通道定义的尺寸
	if(layer_width > HW_lcdc_get_xdots(lcd_channel) )
	{
		XM_printf ("osd width(%d) exceed LCD's width (%d)\n",  
			layer_width, HW_lcdc_get_xdots(lcd_channel));
		return NULL;
	}
	if(layer_height > HW_lcdc_get_ydots(lcd_channel) )
	{
		XM_printf ("osd height(%d) exceed LCD's height (%d)\n", 
			layer_height, HW_lcdc_get_ydots(lcd_channel) );
		return NULL;
	}
	// 约束OSD层的宽度、高度信息
	layer_width = XM_lcdc_osd_horz_align (lcd_channel, layer_width);
	layer_height = XM_lcdc_osd_vert_align (lcd_channel, layer_height);
	if(layer_width == 0 || layer_height == 0)
	{
		XM_printf ("osd height(%d) or width(%d) invalid\n", 
			layer_height, layer_width );

		return NULL;
	}

	framebuffer_index = get_framebuffer_index_from_osd_layer (osd_layer);
	
	if(osd_framebuffer_busy)
		return NULL;

	// 互斥访问锁定 (UI任务与视频输出任务会同时访问)
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return NULL;

	// 检查OSD是否已关闭
	if(!XM_GetFmlDeviceCap(DEVCAP_OSD))
	{
		// OSD off
		XM_SignalSemaphore (osd_framebuffer_semaphore);
		XM_printf ("XM_osd_framebuffer_create failed, OSD Off\n");
		return NULL;
	}
	

	// 分辨率切换时会设置osd_framebuffer_busy为1, 阻止后续的资源申请
	if(osd_framebuffer_busy)
	{
		XM_SignalSemaphore (osd_framebuffer_semaphore);
		XM_printf ("osd framebuffer busy\n");
		return NULL;
	}

	free_link = &osd_framebuffer_free_link[lcd_channel][framebuffer_index];
	
	// 检查空闲链表是否不为空
	if(!queue_empty (free_link))
	{
		// 取出空闲链表中的第一个空闲单元
		framebuffer = (XM_OSD_FRAMEBUFFER *)queue_delete_next (free_link);
	}
	else
	{
		XM_printf ("no free framebuffer in lcd_channel(%d) osd_layer(%d)\n", lcd_channel, osd_layer);
	}

	if(framebuffer && framebuffer->address == NULL)
	{
		XM_printf ("Fatal Error, can't use framebuffer before config the base address of framebuffer object\n");
		queue_insert ((queue_s *)framebuffer, free_link);
		XM_SignalSemaphore (osd_framebuffer_semaphore);
		return NULL;
	}

	//XM_printf ("XM_osd_framebuffer_create 0x%08x, lcd_channel = %d osd_layer = %d\n", framebuffer, lcd_channel, osd_layer);

	if(framebuffer)
	{
		// 创建时将OSD层锁定
		XM_lcdc_osd_layer_set_lock (lcd_channel, (1 << osd_layer), (1 << osd_layer) );

		framebuffer->lcd_channel = lcd_channel;
		framebuffer->osd_layer = osd_layer;
		framebuffer->width = layer_width;
		framebuffer->height = layer_height;
		framebuffer->format = layer_format;
		framebuffer->view_handle = view_handle;
		if(framebuffer->format == XM_OSD_LAYER_FORMAT_YUV420)
			framebuffer->stride = 1 * layer_width;
		else if(framebuffer->format == XM_OSD_LAYER_FORMAT_Y_UV420)
			framebuffer->stride = 1 * layer_width;
		else if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB888)
			framebuffer->stride = 4 * layer_width;
		else if(framebuffer->format == XM_OSD_LAYER_FORMAT_RGB565)
			framebuffer->stride = 2 * layer_width;
		else if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB454)
			framebuffer->stride = 2 * layer_width;
		
		// 计算缺省framebuffer相对于LCD显示原点的偏移
		// 缺省居中显示
		framebuffer->offset_x = (HW_lcdc_get_xdots(lcd_channel)  - layer_width)/2;
		framebuffer->offset_y = (HW_lcdc_get_ydots(lcd_channel)  - layer_height)/2;
		// 约束OSD层相对于输出通道平面原点的偏移
		framebuffer->offset_x = XM_lcdc_osd_horz_align (lcd_channel, framebuffer->offset_x);
		framebuffer->offset_y = XM_lcdc_osd_vert_align (lcd_channel, framebuffer->offset_y);

		if(framebuffer->format == XM_OSD_LAYER_FORMAT_YUV420 || framebuffer->format == XM_OSD_LAYER_FORMAT_Y_UV420)
		{
			// YUV格式
			// 获取来自Sensor的视频数据内容
			BYTE y, u, v;

			XM_RGB2YUV (0, 0, 255, &y, &u, &v); // 调试图层0的背景色

			//XM_RGB2YUV (255, 255, 255, &y, &u, &v);
			if(copy_last_framebuffer_context)
			{
			}
			else if(clear_framebuffer_context)
			{
				unsigned int len = framebuffer->stride * framebuffer->height;
				memset (framebuffer->address, y, len);
				if(framebuffer->format == XM_OSD_LAYER_FORMAT_YUV420)
				{
					memset (framebuffer->address + len, u, len >> 2);
					memset (framebuffer->address + len * 5 / 4, v, len >> 2);
				}
				else if(framebuffer->format == XM_OSD_LAYER_FORMAT_Y_UV420)
				{
					unsigned int i;
					unsigned short *uv_addr = (unsigned short *)(framebuffer->address + len);
					unsigned short uv = (unsigned short)(v << 8);
					uv = (unsigned short)(uv | u);
					for (i = 0; i < (len >> 2); i ++)
					{
						*uv_addr ++ = uv;
					}
				}
			}
		}
		else
		{
			// RGB格式
			if(copy_last_framebuffer_context)
			{
				// 拷贝最近的参数相同的framebuffer
				XM_OSD_FRAMEBUFFER *last_framebuffer = get_last_framebuffer (
									lcd_channel, 
									osd_layer, 
									layer_format, 
									layer_width, 
									layer_height);
				if(last_framebuffer && last_framebuffer->view_handle == view_handle)
				{
					memcpy (framebuffer->address, last_framebuffer->address, 
						framebuffer->stride * framebuffer->height);
				}
				else
				{
					// 清除显示缓冲区(包括Alpha通道)
					if(view_handle && ((XMHWND *)view_handle)->erase)
						memset (framebuffer->address, 0, framebuffer->stride * framebuffer->height);
				}
			}
			else
			{
				// 清除显示缓冲区(包括Alpha通道)
				if(	clear_framebuffer_context 
					||	(view_handle && ((XMHWND *)view_handle)->erase) )
					memset (framebuffer->address, 0, framebuffer->stride * framebuffer->height);
			}
		}
		framebuffer->configed = 0;
	}	

	// 互斥访问解锁
	XM_SignalSemaphore (osd_framebuffer_semaphore);
	
	return framebuffer;
}

// 获取framebuffer对象的数据层指针
// 对于YUV420, 得到 Y / U / V 总计3层的数据缓冲区地址
// 对于Y_UV420, 得到 Y / UV 总计2层的数据缓冲区地址
// 对于ARGB888, 得到 ARGB 总计1层的数据缓冲区地址
int XM_osd_framebuffer_get_buffer (xm_osd_framebuffer_t framebuffer, unsigned char *data[4])
{
	int ret = -1;
	if(framebuffer == NULL)
		return ret;

	// 互斥访问锁定 (UI任务与视频输出任务会同时访问)
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return ret;
	if(framebuffer->format == XM_OSD_LAYER_FORMAT_YUV420)
	{
		data[0] = framebuffer->address;
		data[1] = framebuffer->address + framebuffer->height * framebuffer->width;
		data[2] = framebuffer->address + framebuffer->height * framebuffer->width * 5/4;
		data[3] = NULL;
		ret = 0;
	}
	else if(framebuffer->format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		data[0] = framebuffer->address;
		data[1] = framebuffer->address + framebuffer->height * framebuffer->width;
		data[2] = NULL;
		data[3] = NULL;
		ret = 0;
	}
	else if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB888)
	{
		data[0] = framebuffer->address;
		data[1] = NULL;
		data[2] = NULL;
		data[3] = NULL;
		ret = 0;
	}
	else
	{
		// unsupport format
		ret = -1;
	}

	// 互斥访问解锁
	XM_SignalSemaphore (osd_framebuffer_semaphore);
	return ret;
}

int XM_osd_framebuffer_get_user_buffer (xm_osd_framebuffer_t framebuffer, char **user_data)
{
	int ret = -1;
	if(framebuffer == NULL || user_data == NULL)
		return ret;

	// 互斥访问锁定 (UI任务与视频输出任务会同时访问)
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return ret;

	*user_data = framebuffer->user_data;
	ret = 0;

	// 互斥访问解锁
	XM_SignalSemaphore (osd_framebuffer_semaphore);
	return ret;
}

// 使用指定的颜色(ARGB)清除帧缓冲区
int XM_osd_framebuffer_clear (xm_osd_framebuffer_t framebuffer, 
										unsigned char alpha, 
										unsigned char r, 
										unsigned char g, 
										unsigned char b)
{
	int ret = -1;
	if(framebuffer == NULL)
		return ret;

	// 互斥访问锁定 (UI任务与视频输出任务会同时访问)
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return ret;

	if(framebuffer->format == XM_OSD_LAYER_FORMAT_YUV420 || framebuffer->format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		BYTE y, u, v;
		unsigned int len;
		XM_RGB2YUV (r, g, b, &y, &u, &v);
		len = framebuffer->stride * framebuffer->height;
		memset (framebuffer->address, y, len);
		if(framebuffer->format == XM_OSD_LAYER_FORMAT_YUV420)
		{
			memset (framebuffer->address + len, u, len >> 2);
			memset (framebuffer->address + len * 5 / 4, v, len >> 2);
		}
		else if(framebuffer->format == XM_OSD_LAYER_FORMAT_Y_UV420)
		{
			unsigned int i;
			unsigned short *uv_addr = (unsigned short *)(framebuffer->address + len);
			unsigned short uv = (unsigned short)(v << 8);
			uv = (unsigned short)(uv | u);
			for (i = 0; i < (len >> 2); i ++)
			{
				*uv_addr ++ = uv;
			}
		}
		ret = 0;
	}
	else if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB888)
	{
		unsigned int i, j;
		unsigned int *buff = (unsigned int *)framebuffer->address;
		unsigned int argb = (alpha << 24) | (r << 16) | (g << 8) | (b);
		for (j = 0; j < framebuffer->height; j ++)
		{
			for (i = 0; i < framebuffer->width; i ++)
			{
				*buff ++ = argb;
			}
		}
		ret = 0;
	}
	else
	{
		// un-support format
		ret = -1;
	}
	// 互斥访问解锁
	XM_SignalSemaphore (osd_framebuffer_semaphore);
	return ret;
}


// 删除一个framebuffer对象/实例并将其回收
int XM_osd_framebuffer_delete (xm_osd_framebuffer_t framebuffer)
{
	if(framebuffer == NULL)
		return -1;

	// 互斥访问锁定 (UI任务与视频输出任务会同时访问)
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return NULL;

	// 创建时使用下面的语句将OSD锁定, 删除时需要配对的解锁.  
	//		XM_lcdc_osd_layer_set_lock (lcd_channel, (1 << osd_layer), (1 << osd_layer) );
	// 解锁与framebuffer关联的OSD layer
	XM_lcdc_osd_layer_set_lock (framebuffer->lcd_channel, (1 << framebuffer->osd_layer), 0);

	// 插入到空闲链表队尾
	framebuffer_release (framebuffer);

	// 互斥访问解锁
	XM_SignalSemaphore (osd_framebuffer_semaphore);

	return 0;
}

// 打开指定OSD层最近使用(即最后一次创建)的framebuffer实例。
// 若该OSD层的framebuffer实例还没有创建，则返回NULL。
//		返回NULL表示打开失败(如参数错误、framebuffer实例创建失败等原因导致)
// 打开操作返回上次已创建的framebuffer实例，然后直接使用这个framebuffer实例进行GDI读写操作(直接写屏操作)
XM_OSD_FRAMEBUFFER * XM_osd_framebuffer_open (unsigned int lcd_channel, 
															 unsigned int osd_layer,
															 unsigned int layer_format,
															 unsigned int layer_width, 
														    unsigned int layer_height)
{
	XM_OSD_FRAMEBUFFER *framebuffer = NULL;

	// 检查显示屏是否已连接
	if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_DISCONNECT)
		return NULL;

	// 检查OSD是否已关闭
	if(!XM_GetFmlDeviceCap(DEVCAP_OSD))
	{
		// OSD off
		return NULL;
	}
	
	if(osd_framebuffer_busy)
		return NULL;
	
		// 锁定线程竞争资源
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return NULL;

	// 分辨率切换时会设置osd_framebuffer_busy为1, 阻止后续的资源申请
	if(osd_framebuffer_busy)
	{
		XM_SignalSemaphore (osd_framebuffer_semaphore);
		return NULL;
	}

	framebuffer = get_last_framebuffer (lcd_channel, osd_layer, layer_format, layer_width, layer_height);
#if WIN32 && _DEBUG
	XM_printf ("XM_osd_framebuffer_open 0x%08x, lcd_channel=%d, osd_layer=%d\n", framebuffer, lcd_channel, osd_layer);
#endif
	if(framebuffer)
	{
#if WIN32 && _DEBUG
		assert (framebuffer->configed);
#endif
	}

	// 解锁
	XM_SignalSemaphore (osd_framebuffer_semaphore);

	return framebuffer;
}

// 关闭framebuffer对象/实例的使用, 并将framebuffer显示在相应的OSD层，即将framebuffer对象与一个OSD层关联
int XM_osd_framebuffer_close (XM_OSD_FRAMEBUFFER *framebuffer, int force_to_config_osd)
{
	int framebuffer_index;
	unsigned int unlock_layer = 0;
	if(framebuffer == NULL)
	{
#if WIN32 && _DEBUG
		assert (0);
#endif
		return -1;
	}
	if(framebuffer->lcd_channel == (unsigned int)(-1) || framebuffer->osd_layer == (unsigned int)(-1))
	{
		XM_printf ("illegal framebuffer (0x%08x)\n", framebuffer);
		return -1;
	}
		
	// 将新创建的framebuffer加入到已使用链表的尾部
	framebuffer_index = get_framebuffer_index_from_osd_layer (framebuffer->osd_layer);

	// 锁定线程竞争资源
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
	{
		XM_printf ("XM_osd_framebuffer_close (0x%08x) failed, resource busy\n", framebuffer);
		return -1;
	}

	//XM_printf ("XM_osd_framebuffer_close, 0x%08x\n", framebuffer);

	if(!framebuffer->configed)
	{
		// 扫描前一个具有相同LCDC通道及OSD层的framebuffer实例，该实例将被释放。
		queue_s *framebuffer_head = &osd_framebuffer_live_link[framebuffer->lcd_channel][framebuffer_index];
		XM_OSD_FRAMEBUFFER *framebuffer_link;
		XM_OSD_FRAMEBUFFER *framebuffer_last = NULL;
		if(!queue_empty(framebuffer_head))
		{
			// 遍历链表,
			framebuffer_link = (XM_OSD_FRAMEBUFFER *)queue_next (framebuffer_head);
			while( (queue_s *)framebuffer_link != framebuffer_head )
			{
				if(framebuffer_link == framebuffer)
				{
					XM_printf ("same framebuffer\n");
					assert (0);
				}

				if(	framebuffer_link->lcd_channel == framebuffer->lcd_channel 
					&& framebuffer_link->osd_layer == framebuffer->osd_layer
					)
				{
					framebuffer_last = framebuffer_link;
					break;
				}
				framebuffer_link = framebuffer_link->next;
			}
		}

		// 检查framebuffer的配置参数是否与OSD硬件层设置一致
		if(	(XM_lcdc_osd_get_format (framebuffer->lcd_channel, framebuffer->osd_layer) == framebuffer->format)
			&&	(XM_lcdc_osd_get_width (framebuffer->lcd_channel, framebuffer->osd_layer) == framebuffer->width)
			&& (XM_lcdc_osd_get_height (framebuffer->lcd_channel, framebuffer->osd_layer) == framebuffer->height) 
			&& !force_to_config_osd )
		{
			// 配置的参数与OSD硬件层原来的设置参数一致，仅修改缓冲区指针
			if(framebuffer->format == XM_OSD_LAYER_FORMAT_YUV420)
			{
				XM_lcdc_osd_set_yuv_addr (framebuffer->lcd_channel, framebuffer->osd_layer, 
					framebuffer->address, 
					framebuffer->address + framebuffer->width * framebuffer->height, 
					framebuffer->address + framebuffer->width * framebuffer->height * 5 / 4 );

			}
			else if(framebuffer->format == XM_OSD_LAYER_FORMAT_Y_UV420)
			{
				XM_lcdc_osd_set_yuv_addr (framebuffer->lcd_channel, framebuffer->osd_layer, 
					framebuffer->address, 
					framebuffer->address + framebuffer->width * framebuffer->height, 
					NULL );
			}
			else
			{
				// RGB565使用全局因子控制层Alpha
				XM_lcdc_osd_set_yuv_addr (framebuffer->lcd_channel, framebuffer->osd_layer, 
					framebuffer->address, NULL, NULL);
				if(framebuffer->format == XM_OSD_LAYER_FORMAT_RGB565)
				{
					unsigned int layer_alpha = XM_GetWindowAlpha (framebuffer->view_handle);
					// 全局global coeff因子取值范围0 ~ 64, Alpha因子取值范围0 ~ 255
					unsigned int global_coeff = layer_alpha / 4;
					XM_lcdc_osd_set_global_coeff (framebuffer->lcd_channel, 
											framebuffer->osd_layer,
											global_coeff);
				}
			}
		}
		else
		{
			// 配置的参数与OSD原来的设置参数不一致
			
			// 关闭OSD层显示
			XM_lcdc_osd_set_enable (framebuffer->lcd_channel, framebuffer->osd_layer, 0);
			
			if(framebuffer->format == XM_OSD_LAYER_FORMAT_YUV420 || framebuffer->format == XM_OSD_LAYER_FORMAT_Y_UV420)
			{
				XM_osd_layer_init (
					framebuffer->lcd_channel,		// lcd_channel
					framebuffer->osd_layer,			// osd_layer
					1,				// 通道使能(1)或关闭(0)
					framebuffer->format,	// osd_layer_format
					63,				// layer_global_coeff
					0,				// layer_global_coeff_enable
					64,			// layer_brightness_coeff,		// 亮度因子 (0 ~ 64)
					framebuffer->width, framebuffer->height,	// layer_width, layer_height
					framebuffer->stride,		// layer_stride
					framebuffer->offset_x, framebuffer->offset_y,			// layer_offset_x, layer_offset_y
					(unsigned char *)framebuffer->address, 
					framebuffer->address + framebuffer->width * framebuffer->height, 
					framebuffer->address + framebuffer->width * framebuffer->height * 5 / 4,
					NULL,			// layer_callback
					NULL			// layer_private
					);

			}
			else if(framebuffer->format == XM_OSD_LAYER_FORMAT_RGB565)
			{
				unsigned int layer_alpha = XM_GetWindowAlpha (framebuffer->view_handle);
				XM_osd_layer_init (
					framebuffer->lcd_channel,		// lcd_channel
					framebuffer->osd_layer,			// osd_layer
					1,				// 通道使能(1)或关闭(0)
					framebuffer->format,	// osd_layer_format
					layer_alpha / 4,				// layer_global_coeff
					1,				// layer_global_coeff_enable
					64,			// layer_brightness_coeff,		// 亮度因子 (0 ~ 64)
					framebuffer->width, framebuffer->height,	// layer_width, layer_height
					framebuffer->stride,		// layer_stride
					framebuffer->offset_x, framebuffer->offset_y,		// layer_offset_x, layer_offset_y
					(unsigned char *)framebuffer->address, 
					NULL, 
					NULL,
					NULL,			// layer_callback
					NULL			// layer_private
					);

			}
			else
			{
				XM_osd_layer_init (
					framebuffer->lcd_channel,		// lcd_channel
					framebuffer->osd_layer,			// osd_layer
					1,				// 通道使能(1)或关闭(0)
					framebuffer->format,	// osd_layer_format
					63,				// layer_global_coeff
					0,				// layer_global_coeff_enable
					64,			// layer_brightness_coeff,		// 亮度因子 (0 ~ 64)
					framebuffer->width, framebuffer->height,	// layer_width, layer_height
					framebuffer->stride,		// layer_stride
					framebuffer->offset_x, framebuffer->offset_y,		// layer_offset_x, layer_offset_y
					(unsigned char *)framebuffer->address, 
					NULL, 
					NULL,
					NULL,			// layer_callback
					NULL			// layer_private
					);
			}
		}
		
		// 新参数已全部配置，重新开启OSD层显示
		if(!XM_lcdc_osd_get_enable (framebuffer->lcd_channel, framebuffer->osd_layer))
		{
			// OSD 尚未使能
			XM_lcdc_osd_set_enable (framebuffer->lcd_channel, framebuffer->osd_layer, 1);
		}

		// 插入到“已使用”链表
		queue_insert ((queue_s *)framebuffer, &osd_framebuffer_live_link[framebuffer->lcd_channel][framebuffer_index]);

		framebuffer->configed = 1;

		// 释放老的具有相同LCDC通道及OSD层的framebuffer实例
		if(framebuffer_last)
		{
			framebuffer_release (framebuffer_last);
		}

		// 解锁指定的OSD层
		unlock_layer = (1 << framebuffer->osd_layer);
		// XM_lcdc_osd_layer_set_lock (framebuffer->lcd_channel, (1 << framebuffer->osd_layer), 0);
	}
	
	if(framebuffer && framebuffer->address)
	{
		unsigned int size = 0;
		if(framebuffer->format == XM_OSD_LAYER_FORMAT_YUV420 || framebuffer->format == XM_OSD_LAYER_FORMAT_Y_UV420)
			size = framebuffer->height * framebuffer->width * 3 / 2;
		else if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB888)
			//size = framebuffer->height * framebuffer->width * 4;
			size = framebuffer->height * framebuffer->stride;
		else
			size = framebuffer->height * framebuffer->width * 2;
		dma_flush_range ((unsigned int)framebuffer->address, (unsigned int)framebuffer->address + size);
	}

	if(unlock_layer)
	{
		XM_lcdc_osd_layer_set_lock (framebuffer->lcd_channel, unlock_layer, 0);
	}
		
	// 解锁
	XM_SignalSemaphore (osd_framebuffer_semaphore);

	return 0;
}


// 释放指定OSD层的所有framebuffer实例，同时将该OSD层禁止(即该OSD层显示关闭)。
void XM_osd_framebuffer_release (unsigned int lcd_channel, unsigned int osd_layer)
{
	int framebuffer_index;
	XM_OSD_FRAMEBUFFER *framebuffer = NULL;
	queue_s *link;

	// 检查显示屏是否已连接
	if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_DISCONNECT)
		return;

	/*
	// 检查OSD是否已关闭
	if(!XM_GetFmlDeviceCap(DEVCAP_OSD))
	{
		// OSD off
		return;
	}
	*/


	if(!lcdc_osd_layer_verify (lcd_channel, osd_layer))
		return;
	
	framebuffer_index = get_framebuffer_index_from_osd_layer (osd_layer);
	
	// 锁定线程竞争资源
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return;

	// 关闭对应的OSD层的显示
	XM_lcdc_osd_set_enable (lcd_channel, osd_layer, 0);
	
	// 将需要解锁OSD层进行同步
	if(osd_layer == XM_OSD_LAYER_0)
	{
		// 当OSD0层关闭,允许其更新显示设置,将OSD0层的显示关闭
		// 其他层则不允许更新显示设置
		HW_lcdc_set_osd_coef_syn (lcd_channel, 1 << osd_layer);
	}

	if(!queue_empty(&osd_framebuffer_live_link[lcd_channel][framebuffer_index]))
	{
		link = queue_next (&osd_framebuffer_live_link[lcd_channel][framebuffer_index]);
		// 遍历整个"已使用"链表
		while(link != &osd_framebuffer_live_link[lcd_channel][framebuffer_index])
		{
			framebuffer = (XM_OSD_FRAMEBUFFER *)link;
			link = link->next;
			if(framebuffer->lcd_channel == lcd_channel && framebuffer->osd_layer == osd_layer)
			{
				// 从"已使用"链表中断开
				framebuffer_release (framebuffer);
			}
		}
	}

	// 解锁
	XM_SignalSemaphore (osd_framebuffer_semaphore);
}

// 将源framebuffer的"设置及视频缓冲区数据"复制到目标framebuffer
// 源framebuffer及目标framebuffer需具有相同的尺寸、显示格式, 否则复制会失败
// 成功返回0，失败返回-1
int XM_osd_framebuffer_copy (xm_osd_framebuffer_t dst, xm_osd_framebuffer_t src)
{
	if(dst == NULL || src == NULL)
		return -1;

	if(dst->format != src->format)
		return -1;
	if(dst->width != src->width)
		return -1;
	if(dst->height != src->height)
		return -1;

	// 锁定线程竞争资源
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return -1;

	dst->offset_x = src->offset_x;
	dst->offset_y = src->offset_y;
	if(src->format == XM_OSD_LAYER_FORMAT_YUV420 || src->format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		memcpy (dst->address, src->address, src->width * src->height * 5/4);
	}
	else if(	src->format == XM_OSD_LAYER_FORMAT_ARGB888
			||	src->format == XM_OSD_LAYER_FORMAT_RGB565
			|| src->format == XM_OSD_LAYER_FORMAT_ARGB454)
	{
		memcpy (dst->address, src->address, src->stride * src->height);
	}
	// 解锁
	XM_SignalSemaphore (osd_framebuffer_semaphore);

	return 0;
}

// 获取系统定义的OSD的framebuffer对象个数
// 失败 返回0
// 成功 返回定义的framebuffer个数
int XM_osd_framebuffer_get_framebuffer_count (	unsigned int lcd_channel,			// 视频输出通道序号
																unsigned int framebuffer_type		// YUV/RGB类型
																)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(framebuffer_type >= XM_OSD_FRAMEBUFFER_TYPE_COUNT)
	{
		XM_printf ("illegal framebuffer_type (%d)\n", framebuffer_type);
		return 0;
	}
	if(framebuffer_type == XM_OSD_FRAMEBUFFER_TYPE_YUV)
		return OSD_YUV_FRAMEBUFFER_COUNT;
	else	//if(framebuffer_type == XM_OSD_FRAMEBUFFER_TYPE_RGB)
		return OSD_RGB_FRAMEBUFFER_COUNT;
}

// 设置系统定义的framebuffer对象的缓冲区基址
// 失败 返回0
// 成功 返回1
int XM_osd_framebuffer_set_framebuffer_address ( unsigned int lcd_channel,			// 视频输出通道序号
																 unsigned int framebuffer_type,	// YUV/RGB类型
																 unsigned int framebuffer_index,	// framebuffer对象序号(0开始)
																 unsigned char *framebuffer_base	// 缓冲区基址
																 )
{
	int ret = 0;
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	if(framebuffer_type >= XM_OSD_FRAMEBUFFER_TYPE_COUNT)
	{
		XM_printf ("illegal framebuffer_type (%d)\n", framebuffer_type);
		return 0;
	}
	if(osd_framebuffer_busy)
		return 0;
	// 锁定线程竞争资源
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return 0;
	if(osd_framebuffer_busy)
	{
		XM_SignalSemaphore (osd_framebuffer_semaphore);
		return 0;
	}
	if(framebuffer_type == XM_OSD_FRAMEBUFFER_TYPE_YUV)
	{
		if(framebuffer_index >= OSD_YUV_FRAMEBUFFER_COUNT)
		{
			XM_printf ("illegal index(%d), \"OSD YUV\" only support maximum %d framebuffer\n", 
				framebuffer_index,
				OSD_YUV_FRAMEBUFFER_COUNT);
		}
		else
		{
			osd_yuv_framebuffer[lcd_channel][framebuffer_index].address = framebuffer_base;
			ret = 1;
		}
	}
	else //if(framebuffer_type == XM_OSD_FRAMEBUFFER_TYPE_RGB)
	{
		if(framebuffer_index >= OSD_RGB_FRAMEBUFFER_COUNT)
		{
			XM_printf ("illegal index(%d), \"OSD RGB\" only support maximum %d framebuffer\n", 
				framebuffer_index,
				OSD_RGB_FRAMEBUFFER_COUNT);
		}
		else
		{
			osd_rgb_framebuffer[lcd_channel][framebuffer_index].address = framebuffer_base;
			ret = 1;
		}
	}

	// 解锁
	XM_SignalSemaphore (osd_framebuffer_semaphore);

	return ret;
}

int XM_osd_framebuffer_set_video_ticket (xm_osd_framebuffer_t framebuffer, unsigned int ticket)
{
	if(framebuffer == NULL)
		return -1;

	// 互斥访问锁定 (UI任务与视频输出任务会同时访问)
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return -1;

	framebuffer->frame_ticket = ticket;

	// 解锁
	XM_SignalSemaphore (osd_framebuffer_semaphore);
	return 0;
}


// 外部视频输出接口配置
void XM_osd_framebuffer_init (void)
{
	int i;
	int lcdc_index;

	osd_framebuffer_busy = 0;
	// framebuffer对象初始化
	//memset (osd_yuv_framebuffer, 0, sizeof(osd_yuv_framebuffer));
	//memset (osd_rgb_framebuffer, 0, sizeof(osd_rgb_framebuffer));

	// 初始化framebuffer对象空闲/已用链表

	// 初始化YUV/RGB空闲/已使用链表均为空
	for (lcdc_index = 0; lcdc_index < PROJ_LCDC_CHANNEL_COUNT; lcdc_index ++)
	{
		for (i = 0; i < XM_OSD_FRAMEBUFFER_TYPE_COUNT; i ++)
		{
			queue_initialize (&osd_framebuffer_free_link[lcdc_index][i]);
			queue_initialize (&osd_framebuffer_live_link[lcdc_index][i]);
		}
	}

	for (lcdc_index = 0; lcdc_index < PROJ_LCDC_CHANNEL_COUNT; lcdc_index ++)
	{
		// 将YUV格式的framebuffer对象加入到YUV格式的空闲对象链表
		for (i = 0; i < OSD_YUV_FRAMEBUFFER_COUNT; i ++)
		{
			queue_insert ((queue_s *)&osd_yuv_framebuffer[lcdc_index][i], 
				&osd_framebuffer_free_link[lcdc_index][XM_OSD_FRAMEBUFFER_TYPE_YUV]);
		}

		// 将RGB格式的framebuffer对象加入到RGB格式的空闲对象链表
		for (i = 0; i < OSD_RGB_FRAMEBUFFER_COUNT; i ++)
		{
			queue_insert ((queue_s *)&osd_rgb_framebuffer[lcdc_index][i], 
				&osd_framebuffer_free_link[lcdc_index][XM_OSD_FRAMEBUFFER_TYPE_RGB]);
		}
	}
	
	// 创建互斥信号量
	osd_framebuffer_semaphore = XM_CreateSemaphore ("framebuffer_sema", 1);
	if(osd_framebuffer_semaphore == NULL)
	{
		XM_printf ("CreateSemaphore \"framebuffer_sema\" NG\n");
		return;
	}

	XM_lcdc_osd_init ();
	
	// 调用framebuffer对象的用户配置过程
	XM_osd_framebuffer_config_init ();
	
}

void XM_osd_framebuffer_exit (void)
{
	int count;
	int lcdc_index;
	int busy;

	queue_s* link;	

	XM_printf ("XM_osd_framebuffer_exit ...\n");

	XM_WaitSemaphore(osd_framebuffer_semaphore);
	// 1) 设置OSD关闭, 将无效随后的所有framebuffer创建或打开操作, 阻止其他线程继续分配或使用framebuffer资源
	XM_SetFmlDeviceCap (DEVCAP_OSD, 0);
	XM_SignalSemaphore (osd_framebuffer_semaphore); 

	// 等待其他线程释放framebuffer资源
	OS_Delay (200);

	// 2) 释放OSD层当前显示正在使用的OSD显示缓存(物理刷新), 同时关闭OSD层显示
	for (lcdc_index = 0; lcdc_index < PROJ_LCDC_CHANNEL_COUNT; lcdc_index ++)
	{
		XM_osd_framebuffer_release (lcdc_index, XM_OSD_LAYER_0);
		XM_osd_framebuffer_release (lcdc_index, XM_OSD_LAYER_1);
		XM_osd_framebuffer_release (lcdc_index, XM_OSD_LAYER_2);
	}

	// 3) 等待其他任务释放已分配的framebuffer资源. 
	//    如视频播放线程可能已分配了OSD 0层的一个framebuffer, 使用完毕后, 才能将其释放
	// 检查OSD资源是否使用中. 若是, 等待释放
	// 首先标记退出状态, 这样将无效所有后续的osd framebuffer请求
	XM_WaitSemaphore(osd_framebuffer_semaphore);
	osd_framebuffer_busy = 1;
	XM_SignalSemaphore (osd_framebuffer_semaphore);

	// 检查OSD资源是否使用中. 若是, 等待释放
	while(1)
	{
		busy = 0;
		
		XM_Sleep(1);

		XM_WaitSemaphore(osd_framebuffer_semaphore);
		for (lcdc_index = 0; lcdc_index < PROJ_LCDC_CHANNEL_COUNT; lcdc_index ++)
		{
#if 1
			// 不等待YUV视频通道
			count = 0;
			if(!queue_empty(&osd_framebuffer_free_link[lcdc_index][XM_OSD_FRAMEBUFFER_TYPE_YUV]))
			{
				link = queue_next (&osd_framebuffer_free_link[lcdc_index][XM_OSD_FRAMEBUFFER_TYPE_YUV]);
				while(link != &osd_framebuffer_free_link[lcdc_index][XM_OSD_FRAMEBUFFER_TYPE_YUV])
				{
					count ++;
					link = queue_next (link);
				}
				if(count != OSD_YUV_FRAMEBUFFER_COUNT)
				{
					busy = 1;
					break;
				}
			}
#endif
			count = 0;
			if(!queue_empty(&osd_framebuffer_free_link[lcdc_index][XM_OSD_FRAMEBUFFER_TYPE_RGB]))
			{
				link = queue_next (&osd_framebuffer_free_link[lcdc_index][XM_OSD_FRAMEBUFFER_TYPE_RGB]);
				while(link != &osd_framebuffer_free_link[lcdc_index][XM_OSD_FRAMEBUFFER_TYPE_RGB])
				{
					count ++;
					link = queue_next (link);
				}
				if(count != OSD_RGB_FRAMEBUFFER_COUNT)
				{
					busy = 1;
					break;
				}
			}
		}
		XM_SignalSemaphore (osd_framebuffer_semaphore); 
		
		if(!busy)
			break;
		XM_Sleep(1);
	}

	// 调用framebuffer对象的用户配置退出过程
	XM_osd_framebuffer_config_exit ();

	XM_lcdc_osd_exit ();

	// 锁定线程竞争资源
	XM_WaitSemaphore(osd_framebuffer_semaphore);
	if(osd_framebuffer_semaphore)
	{
		void *semaphore = osd_framebuffer_semaphore;
		osd_framebuffer_semaphore = NULL;
		// 关闭信号灯
		XM_CloseSemaphore (semaphore);
		// 删除信号灯
		XM_DeleteSemaphore (semaphore);

	}

	XM_printf ("XM_osd_framebuffer_exit end\n");
}