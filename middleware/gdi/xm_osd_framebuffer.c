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
//		2014.02.27	ZhuoYongHong�޸ģ���ӦARK1960 LCDC������
//
//****************************************************************************
// OSD��Ʒ���ã������ⲿOSD����Ĳ���

// LCDͨ������(ʹ��RGB��)

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


#ifdef WIN32	// Win32ģ�⻷����ʹ����·LCDC��������ڵ���UI
#define	PROJ_LCDC_CHANNEL_COUNT		1		// ��Ŀ�������Ƶ���ͨ������Ϊ2
#else
#define	PROJ_LCDC_CHANNEL_COUNT		1		// ��Ŀ�������Ƶ���ͨ������Ϊ1
#endif

extern void dma_clean_range(UINT32 ulStart, UINT32 ulEnd);
extern void dma_flush_range(UINT32 ulStart, UINT32 ulEnd);


static void *osd_framebuffer_semaphore;				// �̻߳�����ʱ����ź���

// ����framebuffer����Ŀ���/��ʹ��˫������
static queue_s osd_framebuffer_free_link[PROJ_LCDC_CHANNEL_COUNT][XM_OSD_FRAMEBUFFER_TYPE_COUNT];		
																	// YUV/RGB���ж�������

static queue_s osd_framebuffer_live_link[PROJ_LCDC_CHANNEL_COUNT][XM_OSD_FRAMEBUFFER_TYPE_COUNT];		
																	// YUV/RGB��ʹ�ö�������

// ����framebuffer����
static XM_OSD_FRAMEBUFFER osd_yuv_framebuffer[PROJ_LCDC_CHANNEL_COUNT][OSD_YUV_FRAMEBUFFER_COUNT];
static XM_OSD_FRAMEBUFFER osd_rgb_framebuffer[PROJ_LCDC_CHANNEL_COUNT][OSD_RGB_FRAMEBUFFER_COUNT];

// ��Դ�����ñ�־, 1 ��ʾ��Դ������ 0 ��ʾ��Դ��׼����,����ʹ��
static int osd_framebuffer_busy;

static unsigned int get_framebuffer_index_from_osd_layer (unsigned int osd_layer)
{
	if(osd_layer == XM_OSD_LAYER_0)	// Layer 0 �̶�ΪYUV��Ƶ��
	{
		// YUV����֡����
		return XM_OSD_FRAMEBUFFER_TYPE_YUV;
	}
	else
	{
		// ����RGB�㹲��ͬһ��RGB��framebufferʵ����Ԫ��
		// RGB����֡����
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
	// ��"��ʹ��"�����жϿ�
	queue_delete ((queue_s *)framebuffer);

	lcd_channel = framebuffer->lcd_channel;
	// ���framenuffer��Ч
	
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

	// ���뵽��������
	queue_insert ((queue_s *)framebuffer, &osd_framebuffer_free_link[lcd_channel][framebuffer_index]);
}

// osd��������
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
	
	// ����ʹ������ȡ�����һ�δ�����framebuffer����(ƥ��LCDCͨ����OSD������)
	if(!queue_empty(&osd_framebuffer_live_link[lcd_channel][framebuffer_index]))
	{
		// ����"��ʹ��"����
		queue_s *framebuffer_head = &osd_framebuffer_live_link[lcd_channel][framebuffer_index];
		XM_OSD_FRAMEBUFFER *framebuffer_link;
		if(!queue_empty(framebuffer_head))
		{
			// ��������,
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

// ����һ���µ�framebuffer���� (framebuffer), һ��framebuffer����ʵ����(XM_osd_framebuffer_close)����һ��OSD����������
// ����framebuffer������Դ����ʱ������NULL
// ��ָ��OSD��ĸ�ʽΪYUV420, YUVƽ�������������
// ��OSD���µ�framebuffer��ʾ(XM_osd_framebuffer_display)ʱ�����������framebufferʵ�����Զ��ͷ�����ѭ��ʹ�á�
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

	// �����ʾ���Ƿ�������
	if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_DISCONNECT)
		return NULL;
	
	// ���OSD�Ƿ��ѹر�
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
	// ���framebuffer����ĳߴ��Ƿ񳬳���Ƶ���ͨ������ĳߴ�
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
	// Լ��OSD��Ŀ�ȡ��߶���Ϣ
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

	// ����������� (UI��������Ƶ��������ͬʱ����)
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return NULL;

	// ���OSD�Ƿ��ѹر�
	if(!XM_GetFmlDeviceCap(DEVCAP_OSD))
	{
		// OSD off
		XM_SignalSemaphore (osd_framebuffer_semaphore);
		XM_printf ("XM_osd_framebuffer_create failed, OSD Off\n");
		return NULL;
	}
	

	// �ֱ����л�ʱ������osd_framebuffer_busyΪ1, ��ֹ��������Դ����
	if(osd_framebuffer_busy)
	{
		XM_SignalSemaphore (osd_framebuffer_semaphore);
		XM_printf ("osd framebuffer busy\n");
		return NULL;
	}

	free_link = &osd_framebuffer_free_link[lcd_channel][framebuffer_index];
	
	// �����������Ƿ�Ϊ��
	if(!queue_empty (free_link))
	{
		// ȡ�����������еĵ�һ�����е�Ԫ
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
		// ����ʱ��OSD������
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
		
		// ����ȱʡframebuffer�����LCD��ʾԭ���ƫ��
		// ȱʡ������ʾ
		framebuffer->offset_x = (HW_lcdc_get_xdots(lcd_channel)  - layer_width)/2;
		framebuffer->offset_y = (HW_lcdc_get_ydots(lcd_channel)  - layer_height)/2;
		// Լ��OSD����������ͨ��ƽ��ԭ���ƫ��
		framebuffer->offset_x = XM_lcdc_osd_horz_align (lcd_channel, framebuffer->offset_x);
		framebuffer->offset_y = XM_lcdc_osd_vert_align (lcd_channel, framebuffer->offset_y);

		if(framebuffer->format == XM_OSD_LAYER_FORMAT_YUV420 || framebuffer->format == XM_OSD_LAYER_FORMAT_Y_UV420)
		{
			// YUV��ʽ
			// ��ȡ����Sensor����Ƶ��������
			BYTE y, u, v;

			XM_RGB2YUV (0, 0, 255, &y, &u, &v); // ����ͼ��0�ı���ɫ

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
			// RGB��ʽ
			if(copy_last_framebuffer_context)
			{
				// ��������Ĳ�����ͬ��framebuffer
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
					// �����ʾ������(����Alphaͨ��)
					if(view_handle && ((XMHWND *)view_handle)->erase)
						memset (framebuffer->address, 0, framebuffer->stride * framebuffer->height);
				}
			}
			else
			{
				// �����ʾ������(����Alphaͨ��)
				if(	clear_framebuffer_context 
					||	(view_handle && ((XMHWND *)view_handle)->erase) )
					memset (framebuffer->address, 0, framebuffer->stride * framebuffer->height);
			}
		}
		framebuffer->configed = 0;
	}	

	// ������ʽ���
	XM_SignalSemaphore (osd_framebuffer_semaphore);
	
	return framebuffer;
}

// ��ȡframebuffer��������ݲ�ָ��
// ����YUV420, �õ� Y / U / V �ܼ�3������ݻ�������ַ
// ����Y_UV420, �õ� Y / UV �ܼ�2������ݻ�������ַ
// ����ARGB888, �õ� ARGB �ܼ�1������ݻ�������ַ
int XM_osd_framebuffer_get_buffer (xm_osd_framebuffer_t framebuffer, unsigned char *data[4])
{
	int ret = -1;
	if(framebuffer == NULL)
		return ret;

	// ����������� (UI��������Ƶ��������ͬʱ����)
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

	// ������ʽ���
	XM_SignalSemaphore (osd_framebuffer_semaphore);
	return ret;
}

int XM_osd_framebuffer_get_user_buffer (xm_osd_framebuffer_t framebuffer, char **user_data)
{
	int ret = -1;
	if(framebuffer == NULL || user_data == NULL)
		return ret;

	// ����������� (UI��������Ƶ��������ͬʱ����)
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return ret;

	*user_data = framebuffer->user_data;
	ret = 0;

	// ������ʽ���
	XM_SignalSemaphore (osd_framebuffer_semaphore);
	return ret;
}

// ʹ��ָ������ɫ(ARGB)���֡������
int XM_osd_framebuffer_clear (xm_osd_framebuffer_t framebuffer, 
										unsigned char alpha, 
										unsigned char r, 
										unsigned char g, 
										unsigned char b)
{
	int ret = -1;
	if(framebuffer == NULL)
		return ret;

	// ����������� (UI��������Ƶ��������ͬʱ����)
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
	// ������ʽ���
	XM_SignalSemaphore (osd_framebuffer_semaphore);
	return ret;
}


// ɾ��һ��framebuffer����/ʵ�����������
int XM_osd_framebuffer_delete (xm_osd_framebuffer_t framebuffer)
{
	if(framebuffer == NULL)
		return -1;

	// ����������� (UI��������Ƶ��������ͬʱ����)
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return NULL;

	// ����ʱʹ���������佫OSD����, ɾ��ʱ��Ҫ��ԵĽ���.  
	//		XM_lcdc_osd_layer_set_lock (lcd_channel, (1 << osd_layer), (1 << osd_layer) );
	// ������framebuffer������OSD layer
	XM_lcdc_osd_layer_set_lock (framebuffer->lcd_channel, (1 << framebuffer->osd_layer), 0);

	// ���뵽���������β
	framebuffer_release (framebuffer);

	// ������ʽ���
	XM_SignalSemaphore (osd_framebuffer_semaphore);

	return 0;
}

// ��ָ��OSD�����ʹ��(�����һ�δ���)��framebufferʵ����
// ����OSD���framebufferʵ����û�д������򷵻�NULL��
//		����NULL��ʾ��ʧ��(���������framebufferʵ������ʧ�ܵ�ԭ����)
// �򿪲��������ϴ��Ѵ�����framebufferʵ����Ȼ��ֱ��ʹ�����framebufferʵ������GDI��д����(ֱ��д������)
XM_OSD_FRAMEBUFFER * XM_osd_framebuffer_open (unsigned int lcd_channel, 
															 unsigned int osd_layer,
															 unsigned int layer_format,
															 unsigned int layer_width, 
														    unsigned int layer_height)
{
	XM_OSD_FRAMEBUFFER *framebuffer = NULL;

	// �����ʾ���Ƿ�������
	if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_DISCONNECT)
		return NULL;

	// ���OSD�Ƿ��ѹر�
	if(!XM_GetFmlDeviceCap(DEVCAP_OSD))
	{
		// OSD off
		return NULL;
	}
	
	if(osd_framebuffer_busy)
		return NULL;
	
		// �����߳̾�����Դ
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return NULL;

	// �ֱ����л�ʱ������osd_framebuffer_busyΪ1, ��ֹ��������Դ����
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

	// ����
	XM_SignalSemaphore (osd_framebuffer_semaphore);

	return framebuffer;
}

// �ر�framebuffer����/ʵ����ʹ��, ����framebuffer��ʾ����Ӧ��OSD�㣬����framebuffer������һ��OSD�����
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
		
	// ���´�����framebuffer���뵽��ʹ�������β��
	framebuffer_index = get_framebuffer_index_from_osd_layer (framebuffer->osd_layer);

	// �����߳̾�����Դ
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
	{
		XM_printf ("XM_osd_framebuffer_close (0x%08x) failed, resource busy\n", framebuffer);
		return -1;
	}

	//XM_printf ("XM_osd_framebuffer_close, 0x%08x\n", framebuffer);

	if(!framebuffer->configed)
	{
		// ɨ��ǰһ��������ͬLCDCͨ����OSD���framebufferʵ������ʵ�������ͷš�
		queue_s *framebuffer_head = &osd_framebuffer_live_link[framebuffer->lcd_channel][framebuffer_index];
		XM_OSD_FRAMEBUFFER *framebuffer_link;
		XM_OSD_FRAMEBUFFER *framebuffer_last = NULL;
		if(!queue_empty(framebuffer_head))
		{
			// ��������,
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

		// ���framebuffer�����ò����Ƿ���OSDӲ��������һ��
		if(	(XM_lcdc_osd_get_format (framebuffer->lcd_channel, framebuffer->osd_layer) == framebuffer->format)
			&&	(XM_lcdc_osd_get_width (framebuffer->lcd_channel, framebuffer->osd_layer) == framebuffer->width)
			&& (XM_lcdc_osd_get_height (framebuffer->lcd_channel, framebuffer->osd_layer) == framebuffer->height) 
			&& !force_to_config_osd )
		{
			// ���õĲ�����OSDӲ����ԭ�������ò���һ�£����޸Ļ�����ָ��
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
				// RGB565ʹ��ȫ�����ӿ��Ʋ�Alpha
				XM_lcdc_osd_set_yuv_addr (framebuffer->lcd_channel, framebuffer->osd_layer, 
					framebuffer->address, NULL, NULL);
				if(framebuffer->format == XM_OSD_LAYER_FORMAT_RGB565)
				{
					unsigned int layer_alpha = XM_GetWindowAlpha (framebuffer->view_handle);
					// ȫ��global coeff����ȡֵ��Χ0 ~ 64, Alpha����ȡֵ��Χ0 ~ 255
					unsigned int global_coeff = layer_alpha / 4;
					XM_lcdc_osd_set_global_coeff (framebuffer->lcd_channel, 
											framebuffer->osd_layer,
											global_coeff);
				}
			}
		}
		else
		{
			// ���õĲ�����OSDԭ�������ò�����һ��
			
			// �ر�OSD����ʾ
			XM_lcdc_osd_set_enable (framebuffer->lcd_channel, framebuffer->osd_layer, 0);
			
			if(framebuffer->format == XM_OSD_LAYER_FORMAT_YUV420 || framebuffer->format == XM_OSD_LAYER_FORMAT_Y_UV420)
			{
				XM_osd_layer_init (
					framebuffer->lcd_channel,		// lcd_channel
					framebuffer->osd_layer,			// osd_layer
					1,				// ͨ��ʹ��(1)��ر�(0)
					framebuffer->format,	// osd_layer_format
					63,				// layer_global_coeff
					0,				// layer_global_coeff_enable
					64,			// layer_brightness_coeff,		// �������� (0 ~ 64)
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
					1,				// ͨ��ʹ��(1)��ر�(0)
					framebuffer->format,	// osd_layer_format
					layer_alpha / 4,				// layer_global_coeff
					1,				// layer_global_coeff_enable
					64,			// layer_brightness_coeff,		// �������� (0 ~ 64)
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
					1,				// ͨ��ʹ��(1)��ر�(0)
					framebuffer->format,	// osd_layer_format
					63,				// layer_global_coeff
					0,				// layer_global_coeff_enable
					64,			// layer_brightness_coeff,		// �������� (0 ~ 64)
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
		
		// �²�����ȫ�����ã����¿���OSD����ʾ
		if(!XM_lcdc_osd_get_enable (framebuffer->lcd_channel, framebuffer->osd_layer))
		{
			// OSD ��δʹ��
			XM_lcdc_osd_set_enable (framebuffer->lcd_channel, framebuffer->osd_layer, 1);
		}

		// ���뵽����ʹ�á�����
		queue_insert ((queue_s *)framebuffer, &osd_framebuffer_live_link[framebuffer->lcd_channel][framebuffer_index]);

		framebuffer->configed = 1;

		// �ͷ��ϵľ�����ͬLCDCͨ����OSD���framebufferʵ��
		if(framebuffer_last)
		{
			framebuffer_release (framebuffer_last);
		}

		// ����ָ����OSD��
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
		
	// ����
	XM_SignalSemaphore (osd_framebuffer_semaphore);

	return 0;
}


// �ͷ�ָ��OSD�������framebufferʵ����ͬʱ����OSD���ֹ(����OSD����ʾ�ر�)��
void XM_osd_framebuffer_release (unsigned int lcd_channel, unsigned int osd_layer)
{
	int framebuffer_index;
	XM_OSD_FRAMEBUFFER *framebuffer = NULL;
	queue_s *link;

	// �����ʾ���Ƿ�������
	if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_DISCONNECT)
		return;

	/*
	// ���OSD�Ƿ��ѹر�
	if(!XM_GetFmlDeviceCap(DEVCAP_OSD))
	{
		// OSD off
		return;
	}
	*/


	if(!lcdc_osd_layer_verify (lcd_channel, osd_layer))
		return;
	
	framebuffer_index = get_framebuffer_index_from_osd_layer (osd_layer);
	
	// �����߳̾�����Դ
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return;

	// �رն�Ӧ��OSD�����ʾ
	XM_lcdc_osd_set_enable (lcd_channel, osd_layer, 0);
	
	// ����Ҫ����OSD�����ͬ��
	if(osd_layer == XM_OSD_LAYER_0)
	{
		// ��OSD0��ر�,�����������ʾ����,��OSD0�����ʾ�ر�
		// �����������������ʾ����
		HW_lcdc_set_osd_coef_syn (lcd_channel, 1 << osd_layer);
	}

	if(!queue_empty(&osd_framebuffer_live_link[lcd_channel][framebuffer_index]))
	{
		link = queue_next (&osd_framebuffer_live_link[lcd_channel][framebuffer_index]);
		// ��������"��ʹ��"����
		while(link != &osd_framebuffer_live_link[lcd_channel][framebuffer_index])
		{
			framebuffer = (XM_OSD_FRAMEBUFFER *)link;
			link = link->next;
			if(framebuffer->lcd_channel == lcd_channel && framebuffer->osd_layer == osd_layer)
			{
				// ��"��ʹ��"�����жϿ�
				framebuffer_release (framebuffer);
			}
		}
	}

	// ����
	XM_SignalSemaphore (osd_framebuffer_semaphore);
}

// ��Դframebuffer��"���ü���Ƶ����������"���Ƶ�Ŀ��framebuffer
// Դframebuffer��Ŀ��framebuffer�������ͬ�ĳߴ硢��ʾ��ʽ, �����ƻ�ʧ��
// �ɹ�����0��ʧ�ܷ���-1
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

	// �����߳̾�����Դ
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
	// ����
	XM_SignalSemaphore (osd_framebuffer_semaphore);

	return 0;
}

// ��ȡϵͳ�����OSD��framebuffer�������
// ʧ�� ����0
// �ɹ� ���ض����framebuffer����
int XM_osd_framebuffer_get_framebuffer_count (	unsigned int lcd_channel,			// ��Ƶ���ͨ�����
																unsigned int framebuffer_type		// YUV/RGB����
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

// ����ϵͳ�����framebuffer����Ļ�������ַ
// ʧ�� ����0
// �ɹ� ����1
int XM_osd_framebuffer_set_framebuffer_address ( unsigned int lcd_channel,			// ��Ƶ���ͨ�����
																 unsigned int framebuffer_type,	// YUV/RGB����
																 unsigned int framebuffer_index,	// framebuffer�������(0��ʼ)
																 unsigned char *framebuffer_base	// ��������ַ
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
	// �����߳̾�����Դ
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

	// ����
	XM_SignalSemaphore (osd_framebuffer_semaphore);

	return ret;
}

int XM_osd_framebuffer_set_video_ticket (xm_osd_framebuffer_t framebuffer, unsigned int ticket)
{
	if(framebuffer == NULL)
		return -1;

	// ����������� (UI��������Ƶ��������ͬʱ����)
	if(!XM_WaitSemaphore(osd_framebuffer_semaphore))
		return -1;

	framebuffer->frame_ticket = ticket;

	// ����
	XM_SignalSemaphore (osd_framebuffer_semaphore);
	return 0;
}


// �ⲿ��Ƶ����ӿ�����
void XM_osd_framebuffer_init (void)
{
	int i;
	int lcdc_index;

	osd_framebuffer_busy = 0;
	// framebuffer�����ʼ��
	//memset (osd_yuv_framebuffer, 0, sizeof(osd_yuv_framebuffer));
	//memset (osd_rgb_framebuffer, 0, sizeof(osd_rgb_framebuffer));

	// ��ʼ��framebuffer�������/��������

	// ��ʼ��YUV/RGB����/��ʹ�������Ϊ��
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
		// ��YUV��ʽ��framebuffer������뵽YUV��ʽ�Ŀ��ж�������
		for (i = 0; i < OSD_YUV_FRAMEBUFFER_COUNT; i ++)
		{
			queue_insert ((queue_s *)&osd_yuv_framebuffer[lcdc_index][i], 
				&osd_framebuffer_free_link[lcdc_index][XM_OSD_FRAMEBUFFER_TYPE_YUV]);
		}

		// ��RGB��ʽ��framebuffer������뵽RGB��ʽ�Ŀ��ж�������
		for (i = 0; i < OSD_RGB_FRAMEBUFFER_COUNT; i ++)
		{
			queue_insert ((queue_s *)&osd_rgb_framebuffer[lcdc_index][i], 
				&osd_framebuffer_free_link[lcdc_index][XM_OSD_FRAMEBUFFER_TYPE_RGB]);
		}
	}
	
	// ���������ź���
	osd_framebuffer_semaphore = XM_CreateSemaphore ("framebuffer_sema", 1);
	if(osd_framebuffer_semaphore == NULL)
	{
		XM_printf ("CreateSemaphore \"framebuffer_sema\" NG\n");
		return;
	}

	XM_lcdc_osd_init ();
	
	// ����framebuffer������û����ù���
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
	// 1) ����OSD�ر�, ����Ч��������framebuffer������򿪲���, ��ֹ�����̼߳��������ʹ��framebuffer��Դ
	XM_SetFmlDeviceCap (DEVCAP_OSD, 0);
	XM_SignalSemaphore (osd_framebuffer_semaphore); 

	// �ȴ������߳��ͷ�framebuffer��Դ
	OS_Delay (200);

	// 2) �ͷ�OSD�㵱ǰ��ʾ����ʹ�õ�OSD��ʾ����(����ˢ��), ͬʱ�ر�OSD����ʾ
	for (lcdc_index = 0; lcdc_index < PROJ_LCDC_CHANNEL_COUNT; lcdc_index ++)
	{
		XM_osd_framebuffer_release (lcdc_index, XM_OSD_LAYER_0);
		XM_osd_framebuffer_release (lcdc_index, XM_OSD_LAYER_1);
		XM_osd_framebuffer_release (lcdc_index, XM_OSD_LAYER_2);
	}

	// 3) �ȴ����������ͷ��ѷ����framebuffer��Դ. 
	//    ����Ƶ�����߳̿����ѷ�����OSD 0���һ��framebuffer, ʹ����Ϻ�, ���ܽ����ͷ�
	// ���OSD��Դ�Ƿ�ʹ����. ����, �ȴ��ͷ�
	// ���ȱ���˳�״̬, ��������Ч���к�����osd framebuffer����
	XM_WaitSemaphore(osd_framebuffer_semaphore);
	osd_framebuffer_busy = 1;
	XM_SignalSemaphore (osd_framebuffer_semaphore);

	// ���OSD��Դ�Ƿ�ʹ����. ����, �ȴ��ͷ�
	while(1)
	{
		busy = 0;
		
		XM_Sleep(1);

		XM_WaitSemaphore(osd_framebuffer_semaphore);
		for (lcdc_index = 0; lcdc_index < PROJ_LCDC_CHANNEL_COUNT; lcdc_index ++)
		{
#if 1
			// ���ȴ�YUV��Ƶͨ��
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

	// ����framebuffer������û������˳�����
	XM_osd_framebuffer_config_exit ();

	XM_lcdc_osd_exit ();

	// �����߳̾�����Դ
	XM_WaitSemaphore(osd_framebuffer_semaphore);
	if(osd_framebuffer_semaphore)
	{
		void *semaphore = osd_framebuffer_semaphore;
		osd_framebuffer_semaphore = NULL;
		// �ر��źŵ�
		XM_CloseSemaphore (semaphore);
		// ɾ���źŵ�
		XM_DeleteSemaphore (semaphore);

	}

	XM_printf ("XM_osd_framebuffer_exit end\n");
}