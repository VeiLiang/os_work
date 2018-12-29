//****************************************************************************
//
//	Copyright (C) 2011 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_argb32bpp_drv.c
//	  LCD���� (ģ��ʱLCD��ʾ����֧�ֻض�)
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

#if defined(_XM_ARGB32BPP_DEVICE_)

#define	ARGB32BPP_Open	ScreenDevice_Open				
#define	ARGB32BPP_Close	ScreenDevice_Close			
#define	ARGB32BPP_DrawPixel	ScreenDevice_DrawPixel		
#define	ARGB32BPP_DrawHorzLine	ScreenDevice_DrawHorzLine	
#define	ARGB32BPP_DrawVertLine	ScreenDevice_DrawVertLine	
#define	ARGB32BPP_FillRect	ScreenDevice_FillRect		
#define	ARGB32BPP_DrawFont	ScreenDevice_DrawFont		
#define	ARGB32BPP_DrawDDB	ScreenDevice_DrawDDB			
#define	ARGB32BPP_DrawROMDDB	ScreenDevice_DrawROMDDB		
#define	ARGB32BPP_Begin	ScreenDevice_Begin			
#define	ARGB32BPP_End	ScreenDevice_End				

static WORD txtColor;	// ȱʡǰ��ɫ��ɫ������
static WORD bkgColor;	// ȱʡ����ɫ��ɫ������

#if defined(FONT_SCALE)
extern BYTE		fontScaleRatio;
#endif

const XMSCREENDEVICE ScreenDevice = {
	LCD_XDOTS,
	LCD_YDOTS,
	32,
	LCD_XDOTS * 4,	
	1 << 24,
	LCD_XDOTS * LCD_YDOTS * 4,
};

VOID ARGB32BPP_Open (VOID)
{
	txtColor = 0;
	bkgColor = 0;

	XMDDW_Open ();
}

VOID ARGB32BPP_Close (VOID)
{
	XMDDW_Close ();
}

void	ARGB32BPP_DrawPixel(XMCOORD x, XMCOORD y, XMCOLOR color)
{
	// ���߽�
	if(x < 0 || y < 0)
		return;
	if(x >= LCD_XDOTS || y >= LCD_YDOTS)
		return;

	XMDDW_SetPixel (x, y, color);
	XMDDW_Flush ();
}

void ARGB32BPP_DrawHorzLine (XMCOORD x1, XMCOORD x2, XMCOORD y, XMCOLOR c)
{
	XMCOORD x;

	// �������Ƿ�Խ��
	if(y < 0 || y >= LCD_YDOTS)
		return;
	if(x1 < 0 || x1 >= LCD_XDOTS)
		return;
	if(x2 < 0 || x2 >= LCD_XDOTS)
		return;

	if(x1 > x2)
	{
		x = x1;
		x1 = x2;
		x2 = x;
	}

	//XMDDW_DrawHorzLine (x1, y, (XMCOORD)(x2 - x1 + 1), c);
	XMDDW_FillRect (x1, y, x2, y, c);
}

void	ARGB32BPP_DrawVertLine (XMCOORD x, XMCOORD y1, XMCOORD y2, XMCOLOR color)
{
	XMCOORD y;

	// �������Ƿ�Խ��
	if(x < 0 || x >= LCD_XDOTS)
		return;
	if(y1 < 0 || y1 >= LCD_YDOTS)
		return;
	if(y2 < 0 || y2 >= LCD_YDOTS)
		return;
	
	if (y1 > y2) 
	{
		y = y1;
		y1 = y2;
		y2 = y;
	}
	//XMDDW_DrawVertLine (x, y1, (XMCOORD)(y2 - y1 + 1), color);
	XMDDW_FillRect (x, y1, x, y2, color);
}

void	ARGB32BPP_FillRect (XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, XMCOLOR color)
{
	if(x1 < 0 || x1 >= LCD_XDOTS)
		return;
	if(x2 < 0 || x2 >= LCD_XDOTS)
		return;
	if(y1 < 0 || y1 >= LCD_YDOTS)
		return;
	if(y2 < 0 || y2 >= LCD_YDOTS)
		return;

	XMDDW_FillRect (x1, y1, x2, y2, color);
	/*
	while(y1 <= y2)
	{
		XMDDW_DrawHorzLine (x1, y1, (XMCOORD)(x2 - x1 + 1), color);
		y1 ++;
	}*/

}

#pragma arm section code = "LCD_FAST"
// δ���ǲü����ܣ�Ӧ�ò�������ַ�������ж��Ƿ����
void	ARGB32BPP_DrawFont (XMCOORD sx, XMCOORD sy,	// �������
							 BYTE w, BYTE h,	// �ַ���ȡ��߶�
							 BYTE *imagebits,			// �ַ���������		
							 BYTE slinelen				// �ַ������ֽ��г�
							 )		
{
	BYTE *src;
	XMCOORD x, y;
	if(w == 0 || h == 0)
		return;

	// �������ַ�����ʾ
#if defined(FONT_SCALE)
	if(sx < 0 || (sx + (w << fontScaleRatio) ) >= ScreenDevice.xres )
		return;
	if(sy < 0 || (sy + (h << fontScaleRatio) ) >= ScreenDevice.yres )
		return;
#else
	if(sx < 0 || (sx + w ) >= ScreenDevice.xres )
		return;
	if(sy < 0 || (sy + h ) >= ScreenDevice.yres )
		return;
#endif

	x = (XMCOORD)sx;
	y = (XMCOORD)sy;

	src = imagebits;
	
	while(h > 0)
	{
#if defined(FONT_SCALE)
		int loop = fontScaleRatio + 1;

		while(loop)
		{
#endif
			BYTE *	s = src;
			int dw = w;
			//BYTE	s_mask = 0x01;
			BYTE	s_mask = 0x80;
			BYTE sbits = *s ++;
			x = (XMCOORD)sx;
			while(--dw >= 0)
			{
				if(sbits & s_mask)
				{
					XMDDW_SetPixel (x, y, txtColor);
#if defined(FONT_SCALE)
					if(fontScaleRatio)
						XMDDW_SetPixel ((XMCOORD)(x+1), y, txtColor);
#endif
				}

				//s_mask <<= 1;
				s_mask >>= 1;
				if(s_mask == 0)
				{
					//s_mask = 0x01;
					s_mask = 0x80;
					sbits = *s ++;
				}

#if defined(FONT_SCALE)
				x = (XMCOORD)(x + (1 << fontScaleRatio));
#else
				x ++;
#endif
			}

			y ++;

#if defined(FONT_SCALE)
			loop --;
		}
#endif

		src += slinelen;
		h --;
	}

	// XMDDW_SetPixel����Cache����Ҫǿ��ˢ��
	XMDDW_Flush ();
}

#pragma arm section code = "LCD_SLOW"
// imagebitsΪָ��RAM�еĵ�ַ
void	ARGB32BPP_DrawDDB (XMCOORD x, XMCOORD y, XMCOORD cx, XMCOORD cy, BYTE *imagebits, WORD slinelen, XMCOLOR transcolor)
{
}


// imagebitsָ��ROM��ĳ��BANK���Ѿ�ת��ΪBANK�ڵĵ�ַ����ִ�иú���ǰ�����DATA��BANK�л���ִ�����ָ�ԭ����BANK����
void	ARGB32BPP_DrawROMDDB (XMCOORD x, XMCOORD y, XMCOORD cx, XMCOORD cy, const BYTE *imagebits, WORD slinelen, XMCOLOR transcolor)
{
}


void	ARGB32BPP_Begin (void)
{

}

void	ARGB32BPP_End (void)
{

}

void	ScreenDevice_SetColor	(XMCOLOR txtcolor, XMCOLOR bkgcolor)
{
	// ����ȱʡǰ����ɫ����ɫ
	txtColor = (WORD)txtcolor;
	bkgColor = (WORD)bkgColor;
}



#endif	// #if defined(_XM_ARGB32BPP_DEVICE_)

