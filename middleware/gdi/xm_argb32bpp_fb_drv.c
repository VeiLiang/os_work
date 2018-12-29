//****************************************************************************
//
//	Copyright (C) 2011 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_argb32bpp_drv.c
//	  LCD驱动 (模拟时LCD显示内容支持回读)
//
//	Revision history
//
//		2011.05.26	ZhuoYongHong Initial version
//
//****************************************************************************

#include <xmtype.h>
#include <xmgdi.h>
#include <string.h>
#include <xmdev.h>
#include <xmddw.h>
#include "xm_gdi_device.h"
#include "xm_osd_framebuffer.h"


void	fb_ARGB32BPP_SetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y, XMCOLOR color)
{
	DWORD *addr;
	// 检查边界
	if(x < 0 || y < 0)
		return;
	if(x >= (XMCOORD)framebuffer->width || y >= (XMCOORD)framebuffer->height)
		return;
	addr = (DWORD *)(framebuffer->address + y * framebuffer->stride + (x << 2));
	*addr = (DWORD)(color);
}

XMCOLOR	fb_ARGB32BPP_GetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y)
{
	// 检查边界
	if(x < 0 || y < 0)
		return 0;
	if(x >= (XMCOORD)framebuffer->width || y >= (XMCOORD)framebuffer->height)
		return 0;
	return *(DWORD *)(framebuffer->address + y * framebuffer->stride + (x << 2));
}

void fb_ARGB32BPP_DrawHorzLine (xm_osd_framebuffer_t framebuffer, XMCOORD x1, XMCOORD x2, XMCOORD y, XMCOLOR c)
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

void	fb_ARGB32BPP_DrawVertLine (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y1, XMCOORD y2, XMCOLOR color)
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

void	fb_ARGB32BPP_FillRect (xm_osd_framebuffer_t framebuffer, XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, XMCOLOR color)
{
	if(x1 < 0 || x1 >= framebuffer->width)
		return;
	if(x2 < 0 || x2 >= framebuffer->width)
		return;
	if(y1 < 0 || y1 >= framebuffer->height)
		return;
	if(y2 < 0 || y2 >= framebuffer->height)
		return;

	while(y1 <= y2)
	{
		fb_ARGB32BPP_DrawHorzLine (framebuffer, x1, x2, y1, color);
		y1 ++;
	}
}




