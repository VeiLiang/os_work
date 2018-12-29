//****************************************************************************
//
//	Copyright (C) 2011 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_fb_argb888_drv.c
//	  LCD framebuffer驱动
//
//	Revision history
//
//		2011.05.26	ZhuoYongHong Initial version
//
//****************************************************************************

#include <xm_type.h>
#include <xm_gdi.h>
#include <string.h>
#include <xm_dev.h>
#include "xm_gdi_device.h"
#include "xm_osd_framebuffer.h"


void fb_ARGB888_SetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y, XMCOLOR color)
{
	DWORD *addr;
	// 检查边界
	if(x < 0 || y < 0)
		return;
	if(x >= (XMCOORD)framebuffer->width || y >= (XMCOORD)framebuffer->height)
		return;
	
#if VERT_REVERSE
	y = framebuffer->height - 1 - y;
#endif
	
	addr = (DWORD *)(framebuffer->address + y * framebuffer->stride + (x << 2));
	*addr = (DWORD)(color);
}

XMCOLOR	fb_ARGB888_GetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y)
{
	// 检查边界
	if(x < 0 || y < 0)
		return 0;
	if(x >= (XMCOORD)framebuffer->width || y >= (XMCOORD)framebuffer->height)
		return 0;
	
#if VERT_REVERSE
	y = framebuffer->height - 1 - y;
#endif
	
	return *(DWORD *)(framebuffer->address + y * framebuffer->stride + (x << 2));
}

void fb_ARGB888_DrawHorzLine (xm_osd_framebuffer_t framebuffer, XMCOORD x1, XMCOORD x2, XMCOORD y, XMCOLOR c)
{
	XMCOORD x;
	XMCOORD size;
	DWORD *addr;

	// 检查参数是否越界
	if(y < 0 || y >= (XMCOORD)framebuffer->height)
		return;
	if(x1 < 0 || x1 >= (XMCOORD)framebuffer->width)
		return;
	if(x2 < 0 || x2 >= (XMCOORD)framebuffer->width)
		return;

#if VERT_REVERSE
	y = framebuffer->height - 1 - y;
#endif
	
	if(x1 > x2)
	{
		x = x1;
		x1 = x2;
		x2 = x;
	}
	size = (XMCOORD)(x2 - x1 + 1);
	addr = (DWORD *)(framebuffer->address + y * framebuffer->stride + (x1 << 2));
	while(size > 0)
	{
		*addr ++ = (DWORD)(c); 
		size --;
	}	
}

void fb_ARGB888_DrawVertLine (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y1, XMCOORD y2, XMCOLOR color)
{
	unsigned char *addr;
	XMCOORD y;
	int size;

	// 检查参数是否越界
	if(x < 0 || x >= (XMCOORD)framebuffer->width)
		return;
	if(y1 < 0 || y1 >= (XMCOORD)framebuffer->height)
		return;
	if(y2 < 0 || y2 >= (XMCOORD)framebuffer->height)
		return;
	
#if VERT_REVERSE
	y1 = framebuffer->height - 1 - y1;
	y2 = framebuffer->height - 1 - y2;
#endif	
	
	if (y1 > y2) 
	{
		y = y1;
		y1 = y2;
		y2 = y;
	}
	addr = (framebuffer->address + y1 * framebuffer->stride + (x << 2));
	size = y2 - y1 + 1;

	while(size > 0) 
	{
		*(DWORD *)addr = (DWORD)color;
		addr = addr + framebuffer->stride;
		size --;
	}
}

void fb_ARGB888_FillRect (xm_osd_framebuffer_t framebuffer, XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, XMCOLOR color)
{
	DWORD *addr;
	XMCOORD x, y;
	int size;
	unsigned int rotate = 0;
	
#ifdef LCD_ROTATE_90
	rotate = 1;
#endif
	
	if(x1 < 0 || x1 >= (XMCOORD)framebuffer->width)
		return;
	if(x2 < 0 || x2 >= (XMCOORD)framebuffer->width)
		return;
	if(y1 < 0 || y1 >= (XMCOORD)framebuffer->height)
		return;
	if(y2 < 0 || y2 >= (XMCOORD)framebuffer->height)
		return;
	
#if VERT_REVERSE
	y1 = framebuffer->height - 1 - y1;
	y2 = framebuffer->height - 1 - y2;
#endif

	if(x1 > x2)
	{
		x = x1;
		x1 = x2;
		x2 = x;
	}
	if(y1 > y2)
	{
		y = y1;
		y1 = y2;
		y2 = y;
	}

	if(rotate)
	{
		while(y1 <= y2)
		{
			// fb_ARGB888_DrawHorzLine (framebuffer, x1, x2, y1, color);
			size = (XMCOORD)(x2 - x1 + 1);
			addr = (DWORD *)(framebuffer->address + y1 * 4 + x1 * framebuffer->height * 4);
			while(size >= 8)
			{
				*addr = (DWORD)(color); 
				addr += framebuffer->height;
				*addr = (DWORD)(color); 
				addr += framebuffer->height;
				*addr = (DWORD)(color); 
				addr += framebuffer->height;
				*addr = (DWORD)(color); 
				addr += framebuffer->height;
				*addr = (DWORD)(color); 
				addr += framebuffer->height;
				*addr = (DWORD)(color); 
				addr += framebuffer->height;
				*addr = (DWORD)(color); 
				addr += framebuffer->height;
				*addr = (DWORD)(color); 
				addr += framebuffer->height;
				size -= 8;
			}
			while(size > 0)
			{
				*addr = (DWORD)(color); 
				addr += framebuffer->height;
				size --;
			}	
			y1 ++;
		}
	}
	else
	{
		while(y1 <= y2)
		{
			// fb_ARGB888_DrawHorzLine (framebuffer, x1, x2, y1, color);
			size = (XMCOORD)(x2 - x1 + 1);
			addr = (DWORD *)(framebuffer->address + y1 * framebuffer->stride + (x1 << 2));
			while(size >= 8)
			{
				*addr ++ = (DWORD)(color); 
				*addr ++ = (DWORD)(color); 
				*addr ++ = (DWORD)(color); 
				*addr ++ = (DWORD)(color); 
				*addr ++ = (DWORD)(color); 
				*addr ++ = (DWORD)(color); 
				*addr ++ = (DWORD)(color); 
				*addr ++ = (DWORD)(color); 
				size -= 8;
			}
			while(size > 0)
			{
				*addr ++ = (DWORD)(color); 
				size --;
			}	
			y1 ++;
		}
	}
}




