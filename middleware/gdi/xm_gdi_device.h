//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: device.h
//	  constant，macro, data structure，function protocol definition of GDI device 
//
//	Revision history
//
//		2010.08.31	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_GDI_DEVICE_H_
#define _XM_GDI_DEVICE_H_

#include <xm_type.h>
#include <xm_osd_framebuffer.h>

#define	FONT_MATRIX_SIZE	32					// 最大字符尺寸


#define	XP_COLOR_PALETTE	0xF0000000
#define	XP_COLOR_MASK		0x0000000F		// 16色

typedef struct tagXMSCREENDEVICE* PSD;

typedef struct tagXMSCREENDEVICE {
	XMCOORD	xres;		/* X screen res (real) */
	XMCOORD	yres;		/* Y screen res (real) */
	BYTE		bpp;		/* # bpp*/
	WORD		linelen;	/* line length in bytes */
	DWORD		ncolors;	/* # screen colors*/
	LONG		size;		/* size of memory allocated*/
} XMSCREENDEVICE;

extern const XMSCREENDEVICE ScreenDevice;


#if defined(_XM_GRY2BPP_DEVICE_) 

#define	ScreenDevice_Open				GRAY2_Open
#define	ScreenDevice_Close			GRAY2_Close
#define	ScreenDevice_DrawPixel		GRAY2_DrawPixel
#define	ScreenDevice_DrawHorzLine	GRAY2_DrawHorzLine
#define	ScreenDevice_DrawVertLine	GRAY2_DrawVertLine
#define	ScreenDevice_FillRect		GRAY2_FillRect
#define	ScreenDevice_DrawFont		GRAY2_DrawFont
#define	ScreenDevice_DrawDDB			GRAY2_DrawDDB
#define	ScreenDevice_DrawROMDDB		GRAY2_DrawROMDDB
#define	ScreenDevice_Begin			GRAY2_Begin
#define	ScreenDevice_End				GRAY2_End

#elif defined(_XM_GRY4BPP_DEVICE_)

#define	ScreenDevice_Open				GRAY4_Open
#define	ScreenDevice_Close			GRAY4_Close
#define	ScreenDevice_DrawPixel		GRAY4_DrawPixel
#define	ScreenDevice_DrawHorzLine	GRAY4_DrawHorzLine
#define	ScreenDevice_DrawVertLine	GRAY4_DrawVertLine
#define	ScreenDevice_FillRect		GRAY4_FillRect
#define	ScreenDevice_DrawFont		GRAY4_DrawFont
#define	ScreenDevice_DrawDDB			GRAY4_DrawDDB
#define	ScreenDevice_DrawROMDDB		GRAY4_DrawROMDDB
#define	ScreenDevice_Begin			GRAY4_Begin
#define	ScreenDevice_End				GRAY4_End

#elif defined(_GRAY2_ST7586S_DEVICE_) 

#define	ScreenDevice_Open				GRAY2_ST7586S_Open
#define	ScreenDevice_Close			GRAY2_ST7586S_Close
#define	ScreenDevice_DrawPixel		GRAY2_ST7586S_DrawPixel
#define	ScreenDevice_DrawHorzLine	GRAY2_ST7586S_DrawHorzLine
#define	ScreenDevice_DrawVertLine	GRAY2_ST7586S_DrawVertLine
#define	ScreenDevice_FillRect		GRAY2_ST7586S_FillRect
#define	ScreenDevice_DrawFont		GRAY2_ST7586S_DrawFont
#define	ScreenDevice_DrawDDB			GRAY2_ST7586S_DrawDDB
#define	ScreenDevice_DrawROMDDB		GRAY2_ST7586S_DrawROMDDB
#define	ScreenDevice_Begin			GRAY2_ST7586S_Begin
#define	ScreenDevice_End				GRAY2_ST7586S_End

#endif

void	ScreenDevice_Open			(VOID);
void	ScreenDevice_Close		(VOID);

void	ScreenDevice_DrawPixel	(XMCOORD x, XMCOORD y, XMCOLOR c);

void	ScreenDevice_DrawHorzLine(XMCOORD x1, XMCOORD x2, XMCOORD y, XMCOLOR c);
void	ScreenDevice_DrawVertLine(XMCOORD x, XMCOORD y1, XMCOORD y2, XMCOLOR c);

void	ScreenDevice_FillRect	(XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, XMCOLOR c);

void	ScreenDevice_DrawFont	(XMCOORD x, XMCOORD y, BYTE w, BYTE h, BYTE *imagebits, BYTE linelen); 

// 画设备相关位图(位图已转换成设备显示兼容的格式)
void	ScreenDevice_DrawDDB		(XMCOORD x, XMCOORD y, XMCOORD cx, XMCOORD cy, BYTE *imagebits, WORD linelen, XMCOLOR transcolor);
void	ScreenDevice_DrawROMDDB	(XMCOORD x, XMCOORD y, XMCOORD cx, XMCOORD cy, const BYTE *imagebits, WORD linelen, XMCOLOR transcolor);


void	ScreenDevice_Begin		(void);			/* 开始执行屏幕输出 */
void	ScreenDevice_End			(void);			/* 屏幕输出执行完毕 */
void	ScreenDevice_SetColor	(XMCOLOR forcolor, XMCOLOR bkgcolor);

void	fb_SetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y, XMCOLOR color);
XMCOLOR	fb_GetPixel (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y);
void fb_DrawHorzLine (xm_osd_framebuffer_t framebuffer, XMCOORD x1, XMCOORD x2, XMCOORD y, XMCOLOR c);
void	fb_DrawVertLine (xm_osd_framebuffer_t framebuffer, XMCOORD x, XMCOORD y1, XMCOORD y2, XMCOLOR color);
void	fb_FillRect (xm_osd_framebuffer_t framebuffer, XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, XMCOLOR color);



XMBOOL KeyDriverGetEvent (BYTE *key, BYTE *modifier);
void KeyDriverOpen (void);
void KeyDriverClose (void);
XMBOOL KeyDriverPoll(void);

#if defined (HWVER)
void KeyScan(void);
#endif

VOID TimerDriverOpen (void);
VOID TimerDriverClose (void);
XMBOOL TimerDriverPoll (void);

void FontDriverOpen (void);
void FontDriverClose (void);

void ResetAlarm (void);

void winuser_init (void);
void winuser_exit (void);
void wingdi_init (void);
void wingdi_exit (void);

void winmem_init (void);
void winmem_exit (void);



#endif	// #ifndef _XM_GDI_DEVICE_H_

