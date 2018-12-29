//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: win_gdi.c
//	  gdi����
//
//	Revision history
//
//		2010.09.02	ZhuoYongHong Initial version
//
//****************************************************************************
#include <string.h>
#include <xm_user.h>
#include <xm_base.h>
#include <xm_gdi.h>
#include <xm_dev.h>
#include <xm_assert.h>
#include <common_wstring.h>
#include "xm_gdi_device.h"
#include "xm_osd_layer.h"
#include "xm_osd_framebuffer.h"
#include <xm_user.h>
#include <lpng\png.h>
#include <zlib\zlib.h>
#include <xm_printf.h>
#include <xm_file.h>
#include <xm_image.h>

// �����ⲿ�����Ӵ����
XMHWND_DECLARE(Desktop)

// ǰ��ɫ
XMCOLOR		txtColor;	
// ����ɫ
XMCOLOR		bkgColor;

static XMCOLOR		sysColor[XM_TOTAL_SYSTEM_COLOR];

void wingdi_init (void)
{
	sysColor[XM_COLOR_SCROLLBAR]				= XM_RGB(212, 208, 200);
	sysColor[XM_COLOR_BACKGROUND]				= XM_RGB(  0, 128, 128);
	sysColor[XM_COLOR_ACTIVECAPTION]			= XM_RGB(255, 255, 255);
	sysColor[XM_COLOR_INACTIVECAPTION]		= XM_RGB(255, 255, 255);
	sysColor[XM_COLOR_MENU]						= XM_RGB(212, 208, 200);
	sysColor[XM_COLOR_WINDOW]					= XM_RGB(255, 255, 255);
	//sysColor[XM_COLOR_WINDOW]					= XM_ARGB(212, 255, 255, 255);
	sysColor[XM_COLOR_WINDOWFRAME]			= XM_RGB( 64,  64,  64);
	sysColor[XM_COLOR_MENUTEXT]				= XM_RGB(  0,   0,   0);
	sysColor[XM_COLOR_WINDOWTEXT]				= XM_RGB(  0,   0,   0);
	sysColor[XM_COLOR_CAPTIONTEXT]			= XM_RGB(  0,   0,   0);
	sysColor[XM_COLOR_ACTIVEBORDER]			= XM_RGB(212, 208, 200);
	sysColor[XM_COLOR_INACTIVEBORDER]		= XM_RGB(212, 208, 200);
	sysColor[XM_COLOR_HIGHLIGHT]				= XM_RGB(  0,   0, 128);
	sysColor[XM_COLOR_HIGHLIGHTTEXT]			= XM_RGB(255, 255, 255);
	sysColor[XM_COLOR_BTNFACE]					= XM_RGB(212, 208, 200);
	sysColor[XM_COLOR_BTNSHADOW]				= XM_RGB(128, 128, 128);
	sysColor[XM_COLOR_GRAYTEXT]				= XM_RGB(128, 128, 128);
	sysColor[XM_COLOR_BTNTEXT]					= XM_RGB(  0,   0,   0);
	sysColor[XM_COLOR_INACTIVECAPTIONTEXT]	= XM_RGB(212, 208, 200);
	sysColor[XM_COLOR_BTNHIGHLIGHT]			= XM_RGB(255, 255, 255);
	sysColor[XM_COLOR_3DDKSHADOW]				= XM_RGB( 64,  64,  64);
	sysColor[XM_COLOR_3DLIGHT]					= XM_RGB(212, 208, 200);

	bkgColor = XM_GetSysColor (XM_COLOR_WINDOW);
	txtColor = XM_GetSysColor (XM_COLOR_WINDOWTEXT);

	XM_RomImageInit ();
}

void wingdi_exit (void)
{
	XM_RomImageExit ();
}

XMCOLOR	XM_GetSysColor	(BYTE index)
{
	if(index >= XM_TOTAL_SYSTEM_COLOR)
		return (XMCOLOR)(-1);
	return sysColor[index];
}

XMCOLOR XM_SetTextColor (XMCOLOR color)
{
	XMCOLOR	oldcolor;

	oldcolor = txtColor;
	txtColor = color;

	return oldcolor;
}

XMCOLOR XM_SetBkColor (XMCOLOR color)
{
	XMCOLOR	oldcolor;

	oldcolor = bkgColor;
	bkgColor = color;

	return oldcolor;
}

// ��ȡ��ǰ�ַ���ʾ���ı���ɫ
XMCOLOR XM_GetTextColor (void)
{
	return txtColor;
}

// ��ȡ��ǰ�ַ���ʾ�ı�����ɫ
XMCOLOR XM_GetBkColor (void)
{
	return bkgColor;
}


// ����
void  XM_SetPixel  (HANDLE hWnd, XMCOORD x, XMCOORD y, XMCOLOR color)
{
	XMPOINT	pt;
	unsigned int alpha;
	
	pt.x = x;
	pt.y = y;
	/*
	if(hWnd)
	{
		if(!XM_ClientToScreen (hWnd, &pt))
			return;
	}*/

	alpha = XM_GetWindowAlpha(hWnd);
	alpha = alpha * (color >> 24) / 255;
	alpha &= 0xFF;
	color &= ~0xFF000000;
	color = color | (alpha << 24);

	fb_SetPixel (XM_GetWindowFrameBuffer(hWnd), pt.x, pt.y, color);
}

// ������(�㻮�ߡ����ߡ�ʵ�߻���)
// hWnd �Ӵ���ؼ��ľ��
// hWnd == NULL, x,y����Ϊ��������
// hWnd != NULL, x,y����Ϊ������Ӵ����Ͻ�ԭ�������
void XM_DrawHLine (
	HANDLE hWnd,
	XMCOORD x, 
	XMCOORD y, 
	XMCOORD size, 
	BYTE mask,
	XMCOLOR color)
{
	XMPOINT	pt;
	BYTE shift;
	XMCOORD i;
	xm_osd_framebuffer_t framebuffer;
	unsigned int alpha;

	pt.x = x;
	pt.y = y;

	/*
	if(hWnd)
	{
		if(!XM_ClientToScreen (hWnd, &pt))
			return;
	}*/

	alpha = XM_GetWindowAlpha(hWnd);
	alpha = alpha * (color >> 24) / 255;
	alpha &= 0xFF;
	color &= ~0xFF000000;
	color = color | (alpha << 24);

	framebuffer = XM_GetWindowFrameBuffer(hWnd);

	if(mask == (BYTE)(-1))
	{
		fb_DrawHorzLine (framebuffer, pt.x, (XMCOORD)(pt.x + size - 1), pt.y, color);
	}
	else
	{
		// ʹ��mask����
		shift	= 0x80;
		for (i = 0; i < size; i ++)
		{
			if ((shift >> (i & 0x7)) & mask)
				fb_SetPixel (framebuffer, pt.x, pt.y, color);
			pt.x ++;
		}
	}
}

// ������(�㻮�ߡ����ߡ�ʵ�߻���)
// hWnd �Ӵ���ؼ��ľ��
// hWnd == NULL, x,y����Ϊ��������
// hWnd != NULL, x,y����Ϊ������Ӵ����Ͻ�ԭ�������
void XM_DrawVLine (
	HANDLE hWnd,
	XMCOORD x, 
	XMCOORD y, 
	XMCOORD size, 
	BYTE mask,
	XMCOLOR color)
{
	XMPOINT	pt;
	BYTE shift;
	XMCOORD i;
	xm_osd_framebuffer_t framebuffer;
	unsigned int alpha;

	pt.x = x;
	pt.y = y;

	/*
	if(hWnd)
	{
		if(!XM_ClientToScreen (hWnd, &pt))
			return;
	}
	*/
	
	alpha = XM_GetWindowAlpha(hWnd);
	alpha = alpha * (color >> 24) / 255;
	alpha &= 0xFF;
	color &= ~0xFF000000;
	color = color | (alpha << 24);

	framebuffer = XM_GetWindowFrameBuffer(hWnd);

	if(mask == (BYTE)(-1))
	{
		fb_DrawVertLine (framebuffer, pt.x, (XMCOORD)(pt.y + size - 1), pt.y, color);
	}
	else
	{
		// ʹ��mask����
		shift	= 0x80;
		for (i = 0; i < size; i ++)
		{
			if ((shift >> (i & 0x7)) & mask)
				fb_SetPixel (framebuffer, pt.x, pt.y, color);
			pt.y ++;
		}
	}
}

static XMBOOL RectClientToScreen (HANDLE hWnd, XMCOORD* left, XMCOORD* top, XMCOORD* right, XMCOORD* bottom)
{
	XMPOINT pt;
		
	pt.x = *left;
	pt.y = *top;
	if(!XM_ClientToScreen (hWnd, &pt))
		return 0;
	*left = pt.x;
	*top = pt.y;
		
	pt.x = *right;
	pt.y = *bottom;
	if(!XM_ClientToScreen (hWnd, &pt))
		return 0;
	*right = pt.x;
	*bottom = pt.y;
	return 1;
}


// ʹ��ָ������ɫ������
// hWnd �Ӵ���ؼ��ľ��
// hWnd == NULL, ��������Ϊ��������
// hWnd != NULL, ��������Ϊ������Ӵ����Ͻ�ԭ�������
void XM_FillRect (HANDLE hWnd, XMCOORD left, XMCOORD top, XMCOORD right, XMCOORD bottom, XMCOLOR color)
{
	unsigned int alpha;

	/*
	if(hWnd)
	{
		if(!RectClientToScreen (hWnd, &left, &top, &right, &bottom))
			return;
	}*/

	alpha = XM_GetWindowAlpha(hWnd);
	alpha = alpha * (color >> 24) / 255;
	alpha &= 0xFF;
	color &= ~0xFF000000;
	color = color | (alpha << 24);
	
	// �������
	if(left > right)
		return;
	if(top > bottom)
		return;

	fb_FillRect (XM_GetWindowFrameBuffer(hWnd), left, top, right, bottom, color);
}

// �����α߿�
// hWnd �Ӵ���ؼ��ľ��
// hWnd == NULL, ��������Ϊ��������
// hWnd != NULL, ��������Ϊ������Ӵ����Ͻ�ԭ�������
void XM_Rectangle (HANDLE hWnd, XMCOORD left, XMCOORD top, XMCOORD right, XMCOORD bottom, XMCOLOR color)
{
	unsigned int alpha;

	/*
	if(hWnd == NULL)
	{
		// ��Ը��Ӵ�(����)
		hWnd = XMHWND_HANDLE(Desktop);
	}
	if(!RectClientToScreen (hWnd, &left, &top, &right, &bottom))
		return;
	*/
	
	alpha = XM_GetWindowAlpha(hWnd);
	alpha = alpha * (color >> 24) / 255;
	alpha &= 0xFF;
	color &= ~0xFF000000;
	color = color | (alpha << 24);

	// ����
	fb_DrawHorzLine (XM_GetWindowFrameBuffer(hWnd), left, right, top, color);
	if(bottom == top)
		return;
	
	fb_DrawHorzLine (XM_GetWindowFrameBuffer(hWnd), left, right, bottom, color);
	
	if(bottom == (top + 1))
		return;

	// ����
	if(left == right || (left + 1) == right)
		return;

	top ++;
	bottom --;
	fb_DrawVertLine (XM_GetWindowFrameBuffer(hWnd),  left,  top , bottom, color);
	fb_DrawVertLine (XM_GetWindowFrameBuffer(hWnd), right, top , bottom, color);
}

// ���۽��߿�
// hWnd �Ӵ���ؼ��ľ��
// hWnd == NULL, ��������Ϊ��������
// hWnd != NULL, ��������Ϊ������Ӵ����Ͻ�ԭ�������
void XM_FocusRect (HANDLE hWnd, XMCOORD left, XMCOORD top, XMCOORD right, XMCOORD bottom, XMCOLOR color)
{
	XM_DrawHLine (hWnd, left, top,    (XMCOORD)(right - left + 1), 0xaa, color);
	if(top == bottom)
		return;
	XM_DrawHLine (hWnd, left, bottom, (XMCOORD)(right - left + 1), 0x55, color);
	if( (top + 1) == bottom)
		return;

	top ++;
	bottom --;

	XM_DrawVLine (hWnd, left,  top, (XMCOORD)(bottom - top + 1), 0xaa, color);
	XM_DrawVLine (hWnd, right, top, (XMCOORD)(bottom - top + 1), 0x55, color);
}

void XM_Draw3DRect (HANDLE hWnd, XMRECT* lpRect, XMCOLOR clrTopLeft, XMCOLOR clrBottomRight)
{	
	// Draw the top and left sides
	XM_DrawHLine (hWnd, lpRect->left, lpRect->top, (XMCOORD)(lpRect->right - lpRect->left + 1), 0xFF, clrTopLeft);
	XM_DrawVLine (hWnd, lpRect->left, lpRect->top, (XMCOORD)(lpRect->bottom - lpRect->top + 1), 0xFF, clrTopLeft);

	// Draw the bottom and right sides
	XM_DrawHLine (hWnd, lpRect->left, lpRect->bottom, (XMCOORD)(lpRect->right - lpRect->left + 1), 0xFF, clrBottomRight);
	XM_DrawVLine (hWnd, lpRect->right, lpRect->top, (XMCOORD)(lpRect->bottom - lpRect->top + 1), 0xFF, clrBottomRight);
}

// ��ȡPNGͼ��ĳߴ�
int XM_GetPngImageSize (VOID *lpImageBase, DWORD dwImageSize, XMSIZE *lpSize)
{
	png_image image; /* The control structure used by libpng */
	if(lpImageBase == NULL || dwImageSize == 0)
		return -1;

	if(lpSize == NULL)
		return -1;

	/* Initialize the 'png_image' structure. */
	memset(&image, 0, (sizeof image));
	image.version = PNG_IMAGE_VERSION;

	// ��ȡPNGͼ��ĸ�ʽ��Ϣ
	if (png_image_begin_read_from_memory(&image, lpImageBase, dwImageSize) == 0)
		return -1;

	lpSize->cx = image.width;
	lpSize->cy = image.height;

	// �ͷ�png�����������Դ
	png_image_free (&image);

	return 0;
}

// ��ȡPNG/GIFͼ��ĳߴ�
// ����ֵ
// 0     �ɹ�
// < 0   ʧ��
int XM_GetImageSize (VOID *lpImageBase, DWORD dwImageSize, XMSIZE *lpSize)
{
	unsigned char *image = lpImageBase;
	if(lpImageBase == NULL || dwImageSize < 4)
		return -1;
	if(image[0] == 'G' && image[1] == 'I' && image[2] == 'F')
		return XM_GetGifImageSize (lpImageBase, dwImageSize, lpSize);
	else if(image[1] == 'P' && image[2] == 'N' && image[3] == 'G')
		return XM_GetPngImageSize (lpImageBase, dwImageSize, lpSize);
	else
		return -1;
}

// ���Ӵ�ָ���������PNGͼ��
// dwFlag��ָ������ķ��
// ����ֵ
// 0     �ɹ�
// < 0   ʧ��
int XM_DrawPngImageEx (VOID *lpImageBase, DWORD dwImageSize, HANDLE hWnd, 
						  XMRECT *lpRect, DWORD dwFlag)
{
	png_image image; /* The control structure used by libpng */
	//unsigned int osd_layer_format;
	unsigned int osd_layer_width, osd_layer_height;
	unsigned int osd_layer_stride;
	int osd_image_x, osd_image_y;
	unsigned char view_alpha;

	xm_osd_framebuffer_t framebuffer;

	if(lpImageBase == NULL || dwImageSize == 0)
		return -1;
	if(hWnd == NULL)
		return -1;
	if(lpRect == NULL)
		return -1;
	
	// ��ȡframebuffer
	framebuffer = XM_GetWindowFrameBuffer (hWnd);
	if(framebuffer == NULL)
		return -1;

	//osd_layer_format = framebuffer->format;

	/* Initialize the 'png_image' structure. */
	memset(&image, 0, (sizeof image));
	image.version = PNG_IMAGE_VERSION;

	// ��ȡPNGͼ��ĸ�ʽ��Ϣ
	if (png_image_begin_read_from_memory(&image, lpImageBase, dwImageSize) == 0)
		return -1;

	// �ͷ�png�����������Դ
	png_image_free (&image);

	view_alpha = XM_GetWindowAlpha (hWnd);

	// ���PNGͼ��ĳߴ��Ƿ񳬳������OSD��ʾ����
	/*
	if(image.width > framebuffer->width)
	{
		XM_printf ("XM_DrawPngImageEx NG, image.width(%d) > framebuffer->width(%d)\n", 
				image.width, framebuffer->width);
		return -1;
	}
	if(image.height > framebuffer->height)
	{
		XM_printf ("XM_DrawPngImageEx NG, image.height(%d) > framebuffer->height(%d)\n", 
				image.height, framebuffer->height);
		return -1;
	}*/

	/*
	if((int)image.width > (lpRect->right - lpRect->left + 1))
	{
		XM_printf ("XM_DrawPngImageEx NG, image.width(%d) > (lpRect->right - lpRect->left + 1)(%d)\n", 
				image.width, (lpRect->right - lpRect->left + 1));
		return -1;
	}
	if((int)image.height > (lpRect->bottom - lpRect->top + 1))
	{
		XM_printf ("XM_DrawPngImageEx NG, image.height(%d) > (lpRect->bottom - lpRect->top + 1)(%d)\n", 
				image.width, (lpRect->bottom - lpRect->top + 1));
		return -1;
	}*/

	osd_layer_width = framebuffer->width;
	osd_layer_height = framebuffer->height;
	osd_layer_stride = framebuffer->stride;

	if(dwFlag == XMGIF_DRAW_POS_CENTER)
	{
		// ��PNGͼƬ��λ����ʾ������м�λ��
		int temp = (lpRect->right - lpRect->left + 1);
		temp -= image.width;
		osd_image_x = lpRect->left + (temp) / 2;
		temp = (lpRect->bottom - lpRect->top + 1);
		temp -= image.height;
		osd_image_y = lpRect->top + ( temp ) / 2;

		if(osd_image_x < 0 || osd_image_y < 0)
		{
			XM_printf ("XM_DrawPngImageEx NG, osd_image_x = %d, osd_image_y = %d\n", osd_image_x, osd_image_y);
			return -1;
		}
	}
	else
	{
		// ��PNGͼƬ��λ����ʾ��������Ͻ�
		osd_image_x = lpRect->left;
		osd_image_y = lpRect->top;

		// ���������������ηǿ�ʱ��������ʾ�����Լ��
		if(lpRect->right != lpRect->left)
		{
			if( osd_layer_width > (unsigned int)(lpRect->right + 1))
				osd_layer_width = lpRect->right + 1;
			if( osd_layer_height > (unsigned int)(lpRect->bottom + 1))
				osd_layer_height = lpRect->bottom + 1;
		}
	}


	if(XM_lcdc_load_png_image (
					framebuffer->address,
					framebuffer->format,
					osd_layer_width,
					osd_layer_height,
					osd_layer_stride,
					osd_image_x,
					osd_image_y,
					lpImageBase,
					dwImageSize,
					(unsigned int)(-1),
					view_alpha
					) == 0)
		return -1;

	return 0;
}

// ���Ӵ�ָ���������PNG/GIFͼ��
// dwFlag��ָ������ķ��
// ����ֵ
// 0     �ɹ�
// < 0   ʧ��
int XM_DrawImageEx (VOID *lpImageBase, DWORD dwImageSize, HANDLE hWnd, 
						  XMRECT *lpRect, DWORD dwFlag)
{
	unsigned char *image = lpImageBase;
	if(lpImageBase == NULL || dwImageSize < 4)
		return -1;
	if(image[0] == 'G' && image[1] == 'I' && image[2] == 'F')
		return XM_DrawGifImageEx (lpImageBase, dwImageSize, hWnd, lpRect, dwFlag);
	else if(image[1] == 'P' && image[2] == 'N' && image[3] == 'G')
		return XM_DrawPngImageEx (lpImageBase, dwImageSize, hWnd, lpRect, dwFlag);
	else
		return -1;
}

// Converting 8-bit YUV to RGB888
// ʹ��
void	XM_RGB2YUV (BYTE R, BYTE G, BYTE B, BYTE *Y, BYTE *U, BYTE *V)
{
#ifdef _HDTV_000255_		//HDTV 0~255
	// HDTV:0-255
	// Y709 = 0.183R�� + 0.614G�� + 0.062B�� + 16
	// Cb = �C0.101R�� �C 0.338G�� + 0.439B�� + 128
	// Cr = 0.439R�� �C 0.399G�� �C 0.040B�� + 128
	int y = ( (  (int)(0.183*255) * R + (int)(0.614*255) * G + (int)(0.062*255) * B + 128) >> 8) +  16;
	int u = ( ( -(int)(0.101*255) * R - (int)(0.338*255) * G + (int)(0.439*255) * B + 128) >> 8) + 128;
	int v = ( (  (int)(0.439*255) * R - (int)(0.399*255) * G - (int)(0.040*255) * B + 128) >> 8) + 128;
	
	if(y < 0)
		y = 0;
	else if(y > 255)
		y = 255;
	
	if(u < 0)
		u = 0;
	else if(u > 255)
		u = 255;
	
	if(v < 0)
		v = 0;
	else if(v > 255)
		v = 255;
	
	*Y = (BYTE)(y);
	*U = (BYTE)(u);
	*V = (BYTE)(v);
#else
	*Y = (BYTE)(( (  66 * R + 129 * G +  25 * B + 128) >> 8) +  16);
	*U = (BYTE)(( ( -38 * R -  74 * G + 112 * B + 128) >> 8) + 128);
	*V = (BYTE)(( ( 112 * R -  94 * G -  18 * B + 128) >> 8) + 128);
#endif
}

// conversion from YUV to RGB
void	XM_YUV2RGB (BYTE Y, BYTE U, BYTE V, BYTE *R, BYTE *G, BYTE *B)
{
	int C = Y - 16;
	int D = U - 128;
	int E = V - 128;
	*R = (BYTE)(( 298 * C           + 409 * E + 128) >> 8);
	*G = (BYTE)(( 298 * C - 100 * D - 208 * E + 128) >> 8);
	*B = (BYTE)(( 298 * C + 516 * D           + 128) >> 8);
}