//****************************************************************************
//
//	Copyright (C) 2011 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_fb_drv.c
//	  LCD framebufferÇý¶¯
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

void	fb_ARGB888_SetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y, XMCOLOR color);
XMCOLOR	fb_ARGB888_GetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y);
void fb_ARGB888_DrawHorzLine (xm_osd_framebuffer_t framebuffer, XMCOORD x1, XMCOORD x2, XMCOORD y, XMCOLOR c);
void	fb_ARGB888_DrawVertLine (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y1, XMCOORD y2, XMCOLOR color);
void	fb_ARGB888_FillRect (xm_osd_framebuffer_t framebuffer, XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, XMCOLOR color);

void	fb_RGB565_SetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y, XMCOLOR color);
XMCOLOR	fb_RGB565_GetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y);
void fb_RGB565_DrawHorzLine (xm_osd_framebuffer_t framebuffer, XMCOORD x1, XMCOORD x2, XMCOORD y, XMCOLOR c);
void	fb_RGB565_DrawVertLine (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y1, XMCOORD y2, XMCOLOR color);
void	fb_RGB565_FillRect (xm_osd_framebuffer_t framebuffer, XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, XMCOLOR color);

void	fb_ARGB454_SetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y, XMCOLOR color);
XMCOLOR	fb_ARGB454_GetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y);
void fb_ARGB454_DrawHorzLine (xm_osd_framebuffer_t framebuffer, XMCOORD x1, XMCOORD x2, XMCOORD y, XMCOLOR c);
void	fb_ARGB454_DrawVertLine (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y1, XMCOORD y2, XMCOLOR color);
void	fb_ARGB454_FillRect (xm_osd_framebuffer_t framebuffer, XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, XMCOLOR color);




void	fb_SetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y, XMCOLOR color)
{
	if(framebuffer == NULL)
		return;
	if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB888)
		fb_ARGB888_SetPixel (framebuffer, x, y, color);
	else if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB454)
		fb_ARGB454_SetPixel (framebuffer, x, y, color);
	else if(framebuffer->format == XM_OSD_LAYER_FORMAT_RGB565)
		fb_RGB565_SetPixel (framebuffer, x, y, color);
}

XMCOLOR	fb_GetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y)
{
	if(framebuffer == NULL)
		return 0;
	if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB888)
		return fb_ARGB888_GetPixel (framebuffer, x, y);
	else if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB454)
		return fb_ARGB454_GetPixel (framebuffer, x, y);
	else if(framebuffer->format == XM_OSD_LAYER_FORMAT_RGB565)
		return fb_RGB565_GetPixel (framebuffer, x, y);
	else
		return 0;
}

void fb_DrawHorzLine (xm_osd_framebuffer_t framebuffer, XMCOORD x1, XMCOORD x2, XMCOORD y, XMCOLOR c)
{
	if(framebuffer == NULL)
		return;
	if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB888)
		fb_ARGB888_DrawHorzLine (framebuffer, x1, x2, y, c);
	else if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB454)
		fb_ARGB454_DrawHorzLine (framebuffer, x1, x2, y, c);
	else if(framebuffer->format == XM_OSD_LAYER_FORMAT_RGB565)
		fb_RGB565_DrawHorzLine (framebuffer, x1, x2, y, c);
}

void	fb_DrawVertLine (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y1, XMCOORD y2, XMCOLOR color)
{
	if(framebuffer == NULL)
		return;
	if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB888)
		fb_ARGB888_DrawVertLine (framebuffer, x, y1, y2, color);
	else if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB454)
		fb_ARGB454_DrawVertLine (framebuffer, x, y1, y2, color);
	else if(framebuffer->format == XM_OSD_LAYER_FORMAT_RGB565)
		fb_RGB565_DrawVertLine (framebuffer, x, y1, y2, color);
}

void	fb_FillRect (xm_osd_framebuffer_t framebuffer, XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, XMCOLOR color)
{
	if(framebuffer == NULL)
		return;
	if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB888)
		fb_ARGB888_FillRect (framebuffer, x1, y1, x2, y2, color);
	else if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB454)
		fb_ARGB454_FillRect (framebuffer, x1, y1, x2, y2, color);
	else if(framebuffer->format == XM_OSD_LAYER_FORMAT_RGB565)
		fb_RGB565_FillRect  (framebuffer, x1, y1, x2, y2, color);
}




