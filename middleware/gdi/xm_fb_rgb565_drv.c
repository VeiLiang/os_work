//****************************************************************************
//
//	Copyright (C) 2011 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_fb_rgb565_drv.c
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



void	fb_RGB565_SetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y, XMCOLOR color)
{
	WORD *addr;
	// 检查边界
	if(x < 0 || y < 0)
		return;
	if(x >= (XMCOORD)framebuffer->width || y >= (XMCOORD)framebuffer->height)
		return;
	addr = (WORD *)(framebuffer->address + y * framebuffer->stride + (x << 1));
	*addr = ARGB888_TO_RGB565(color);

}

XMCOLOR	fb_RGB565_GetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y)
{
	WORD clr;
	// 检查边界
	if(x < 0 || y < 0)
		return 0;
	if(x >= (XMCOORD)framebuffer->width || y >= (XMCOORD)framebuffer->height)
		return 0;
	clr = *(WORD *)(framebuffer->address + y * framebuffer->stride + (x << 1));
	return RGB565_TO_ARGB888(clr);
}

void fb_RGB565_DrawHorzLine (xm_osd_framebuffer_t framebuffer,XMCOORD x1, XMCOORD x2, XMCOORD y, XMCOLOR c)
{
	XMCOORD x;
	XMCOORD size;
	WORD *addr;
	WORD clr = ARGB888_TO_RGB565(c);

	// 检查参数是否越界
	if(y < 0 || y >= (XMCOORD)framebuffer->height)
		return;
	if(x1 < 0 || x1 >= (XMCOORD)framebuffer->width)
		return;
	if(x2 < 0 || x2 >= (XMCOORD)framebuffer->width)
		return;

	if(x1 > x2)
	{
		x = x1;
		x1 = x2;
		x2 = x;
	}
	size = (XMCOORD)(x2 - x1 + 1);

	addr = (WORD *)(framebuffer->address + y * framebuffer->stride + (x1 << 1));
	while(size > 0)
	{
		*addr ++ = (WORD)(clr); 
		size --;
	}	
}

void	fb_RGB565_DrawVertLine (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y1, XMCOORD y2, XMCOLOR color)
{
	unsigned char *addr;
	XMCOORD y;
	int size;
	WORD clr = ARGB888_TO_RGB565(color);

	// 检查参数是否越界
	if(x < 0 || x >= (XMCOORD)framebuffer->width)
		return;
	if(y1 < 0 || y1 >= (XMCOORD)framebuffer->height)
		return;
	if(y2 < 0 || y2 >= (XMCOORD)framebuffer->height)
		return;
	
	if (y1 > y2) 
	{
		y = y1;
		y1 = y2;
		y2 = y;
	}
	addr = (framebuffer->address + y1 * framebuffer->stride + (x << 1));
	size = y2 - y1 + 1;

	while(size > 0) 
	{
		*(WORD *)addr = (WORD)clr;
		addr = addr + framebuffer->stride;
		size --;
	}
}

void	fb_RGB565_FillRect (xm_osd_framebuffer_t framebuffer, XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, XMCOLOR color)
{
	if(x1 < 0 || x1 >= (XMCOORD)framebuffer->width)
		return;
	if(x2 < 0 || x2 >= (XMCOORD)framebuffer->width)
		return;
	if(y1 < 0 || y1 >= (XMCOORD)framebuffer->height)
		return;
	if(y2 < 0 || y2 >= (XMCOORD)framebuffer->height)
		return;

	while(y1 <= y2)
	{
		fb_RGB565_DrawHorzLine (framebuffer, x1, x2, y1, color);
		y1 ++;
	}

}


