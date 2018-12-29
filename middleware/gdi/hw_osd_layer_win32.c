//****************************************************************************
//
//	Copyright (C) 2013~2014 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: hw_osd_layer_win32.c
//
//	Revision history
//
//		2013.12.20	ZhuoYongHong Initial version
//
//****************************************************************************

// ARK1960 OSD LAYER�����Win32ģ��

// ARK1960 Ӳ���Ĵ�������ʾ��ǰLCDC����ˢ����Ƶʹ�õ�OSD��ַ
// 41 * 4    OSD 0 Y
// 42 * 4    OSD 0 U
// 43 * 4    OSD 0 V

// 44 * 4    OSD 1 Y
// 45 * 4    OSD 1 U
// 46 * 4    OSD 1 V

// 47 * 4    OSD 2 Y
// 48 * 4    OSD 2 U
// 49 * 4    OSD 2 V

#ifdef WIN32

#include <stdio.h>
#include <windows.h>
#include <assert.h>

#include "..\..\include\xmprintf.h"
#include "..\..\include\xmdev.h"

#include "hw_osd_layer.h"
#include "xm_osd_layer.h"

// ARK1960 LCDC OSD�Ĵ���IOģ��
// ����ɫ
static unsigned char reg_background_r[XM_LCDC_CHANNEL_COUNT] = {0};
static unsigned char reg_background_g[XM_LCDC_CHANNEL_COUNT] = {0};
static unsigned char reg_background_b[XM_LCDC_CHANNEL_COUNT] = {0};	

static unsigned char reg_osd_enable[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT] = {0};
static unsigned char reg_osd_global_coeff_enable[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT] = {0};
static unsigned char reg_osd_global_coeff[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT];		
static unsigned char reg_osd_format[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT];			// OSDƽ����ʾԴ���ݸ�ʽ
static unsigned int reg_osd_width[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT];				// OSDƽ����
static unsigned int reg_osd_height[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT];				// OSDƽ��߶�
static unsigned int reg_osd_stride[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT];				// OSDƽ����ʾԴ�������ֽڳ���
static unsigned int reg_osd_h_position[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT];		// OSDƽ�����LCD��ʾԭ���ˮƽ��ƫ��
static unsigned int reg_osd_left_position[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT];	// OSDƽ�����LCD��ʾԭ���ˮƽ��ƫ��
static unsigned int reg_osd_v_position[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT];		// OSDƽ�����LCD��ʾԭ��Ĵ�ֱ��ƫ��
static unsigned int reg_osd_brightness_coeff[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT];// ��������
static unsigned char*reg_osd_y_addr[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT] = {0};
static unsigned char*reg_osd_u_addr[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT] = {0};
static unsigned char*reg_osd_v_addr[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT] = {0};

// �ص�����
static osd_callback_t lcdc_osd_callback[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT] = {0};
static void* lcdc_osd_private[XM_LCDC_CHANNEL_COUNT][XM_OSD_LAYER_COUNT] = {0};
 
// LCDC����ߴ�

// LCDC���ͨ��
static unsigned int lcdc_xdots[XM_LCDC_CHANNEL_COUNT];
static unsigned int lcdc_ydots[XM_LCDC_CHANNEL_COUNT];

// ���ͨ������
static unsigned int lcdc_type[XM_LCDC_CHANNEL_COUNT];
// LCDC���ͨ��ʹ�ܱ�־
static unsigned int lcdc_enable[XM_LCDC_CHANNEL_COUNT] = {0};
// LCDC���ͨ����ʾ�����رձ�־
static unsigned int lcdc_display_on[XM_LCDC_CHANNEL_COUNT] = {0};
static float lcdc_backlight_on[XM_LCDC_CHANNEL_COUNT] = {0.0};



const char *XM_lcdc_channel_name (unsigned int lcdc_channel)
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

// YUV2RGBת��
static long int crv_tab[256];
static long int cbu_tab[256];
static long int cgu_tab[256];

static long int cgv_tab[256];
static long int tab_76309[256];
static unsigned char clp[1024];


static void init_dither_tab(void)
{
	long int crv,cbu,cgu,cgv;
	int i,ind;
	
	crv = 104597; cbu = 132201; 
	cgu = 25675;  cgv = 53279;
	
	for (i = 0; i < 256; i++) 
	{
		crv_tab[i] = (i-128) * crv;
		cbu_tab[i] = (i-128) * cbu;
		cgu_tab[i] = (i-128) * cgu;
		cgv_tab[i] = (i-128) * cgv;
		tab_76309[i] = 76309*(i-16);
	}
	
	for (i=0; i<384; i++)
		clp[i] = 0;
	ind = 384;
	for (i = 0; i < 256; i++)
		clp[ind++] = (unsigned char)i;
	ind = 640;
	for (i = 0; i < 384; i++)
		clp[ind++] = 255;
}

// LCDC���ͨ����ʼ��
// lcdc_channel	LCDC��Ƶ���ͨ����(��ʾ�豸)
// lcdc_type		LCDC��Ƶ���ͨ���ӿ�����
// xdots				LCDC��Ƶ���ͨ�����ؿ�� 
// ydots				LCDC��Ƶ���ͨ�����ظ߶�
void HW_lcdc_init (unsigned int lcdc_channel, unsigned int lcd_type, unsigned int xdots, unsigned int ydots)
{
	init_dither_tab ();

	if(lcdc_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcdc_channel);
		return;
	}
	lcdc_type[lcdc_channel] = lcd_type;
	lcdc_xdots[lcdc_channel] = xdots;
	lcdc_ydots[lcdc_channel] = ydots;
	lcdc_enable[lcdc_channel] = 1;
}

// LCDC���ͨ���ر�(�ر�ʱ�ӡ��رտ����߼�)
void HW_lcdc_exit (unsigned int lcdc_channel)
{
	if(lcdc_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcdc_channel);
		return;
	}
	lcdc_type[lcdc_channel] = (unsigned int)(-1);
	lcdc_xdots[lcdc_channel] = 0;
	lcdc_ydots[lcdc_channel] = 0;
	lcdc_enable[lcdc_channel] = 0;
}


// ����/�ر�LCDC��Ƶͨ������ʾ
// on  1  ������ʾ
// on  0  �ر���ʾ
void HW_lcdc_set_display_on (unsigned int lcdc_channel, unsigned int on)
{
	if(lcdc_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcdc_channel);
		return;
	}
	lcdc_display_on[lcdc_channel] = on;
}

// ����/�ر�LCDC��Ƶͨ���ı�����ʾ
// lcdc_channel	LCDC��Ƶ���ͨ����(��ʾ�豸)
// on  0.0 ~ 1.0  
//     0.0  ����ر�
//     1.0  �����������
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
}

// ��ȡLCDC��Ƶͨ���ı�����ʾ����
// ����ֵ 0.0 ~ 1.0
//     0.0  ����ر�
//     1.0  �����������
float HW_lcdc_get_backlight_on (unsigned int lcdc_channel)
{
	if(lcdc_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcdc_channel);
		return 0.0;
	}
	return lcdc_backlight_on[lcdc_channel];
}


// ��ȡCDC���ͨ����ƽ����
unsigned int HW_lcdc_get_xdots (unsigned int lcd_channel)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	return lcdc_xdots[lcd_channel];
}

// ��ȡCDC���ͨ����ƽ��߶�
unsigned int HW_lcdc_get_ydots (unsigned int lcd_channel)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return 0;
	}
	return lcdc_ydots[lcd_channel];
}


// osd layer����/�ر�
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
		// �����ʾ�����ַ�Ƿ�����
		if(reg_osd_y_addr[lcd_channel][osd_layer] == NULL)
		{
			XM_printf ("ERROR, lcdc(%s) osd(%d) enabled before YUV address setup\n", XM_lcdc_channel_name(lcd_channel), osd_layer);
		}
	}
	else
	{
		// ������ʾ�����ַ��NULL
	}

	XM_printf ("LCD channel(%d)'s OSD layer(%d) %s\n", lcd_channel, osd_layer, enable ? "ENABLE" : "DISABLE");
	reg_osd_enable[lcd_channel][osd_layer] = (unsigned char)enable;
}

unsigned int HW_lcdc_osd_get_enable (unsigned int lcd_channel, unsigned int osd_layer)
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
	return reg_osd_enable[lcd_channel][osd_layer];
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
	reg_osd_global_coeff_enable[lcd_channel][osd_layer] = (unsigned char)enable;
}

// ����LCDC���ͨ������ɫ
void HW_lcdc_set_background_color (unsigned int lcd_channel, 
													unsigned char r, unsigned char g, unsigned char b)
{
	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcd_channel);
		return;
	}
	reg_background_r[lcd_channel] = r;
	reg_background_g[lcd_channel] = g;
	reg_background_b[lcd_channel] = b;
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
	reg_osd_global_coeff[lcd_channel][osd_layer] = (unsigned char)global_coeff;
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
	reg_osd_format[lcd_channel][osd_layer] = (unsigned char)format;
}

void HW_lcdc_osd_set_width (unsigned int lcd_channel, unsigned int osd_layer, unsigned int width)
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
	reg_osd_width[lcd_channel][osd_layer] = width;
}

void HW_lcdc_osd_set_height (unsigned int lcd_channel, unsigned int osd_layer, unsigned int height)
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
	reg_osd_height[lcd_channel][osd_layer] = height;
}

void HW_lcdc_osd_set_h_position (unsigned int lcd_channel, unsigned int osd_layer, unsigned int h_position)
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
	if(h_position > OSD_MAX_H_POSITION)
	{
		XM_printf ("illegal osd_h_position (%d)\n", h_position);
		return;
	}
	reg_osd_h_position[lcd_channel][osd_layer] = h_position;
}

void HW_lcdc_osd_set_v_position (unsigned int lcd_channel, unsigned int osd_layer, unsigned int v_position)
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
	if(v_position > OSD_MAX_V_POSITION)
	{
		XM_printf ("illegal osd_v_position (%d)\n", v_position);
		return;
	}
	reg_osd_v_position[lcd_channel][osd_layer] = v_position;
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
	reg_osd_left_position[lcd_channel][osd_layer] = left_position;
}


void HW_lcdc_osd_set_stride (unsigned int lcd_channel, unsigned int osd_layer, unsigned int stride)
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
	reg_osd_stride[lcd_channel][osd_layer] = stride;
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
	reg_osd_brightness_coeff[lcd_channel][osd_layer] = brightness_coeff;
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

	// ���ж�
	XM_lock ();

	lcdc_osd_callback[lcd_channel][osd_layer] = osd_callback;
	lcdc_osd_private[lcd_channel][osd_layer] = osd_private;

	// ���ж�
	XM_unlock ();
}

// LCDC֡ˢ���жϴ���
void HW_lcdc_interrupt_handler (void)
{
	int i, j;
	for (i = 0; i < XM_LCDC_CHANNEL_COUNT; i++)
	{
		for (j = 0; j < XM_OSD_LAYER_COUNT; j++)
		{
			// ����OSDʹ��ʱ������Ӧ��OSD�ص�����
			if(reg_osd_enable[i][j] && lcdc_osd_callback[i][j])
			{
				(*lcdc_osd_callback[i][j])(lcdc_osd_private[i][j]);
			}
		}
	}
}

// Win32 HDCģ��lcdc���ͨ��
BITMAPINFOHEADER * video_create_bitmap (unsigned int lcd_channel, BOOL Global)
{
	BITMAPINFOHEADER	*lpbi;  
	unsigned int      w, h;
	DWORD             *lpData;
	DWORD					*lpdwColor;
	int					layer;
	HANDLE				hBitmap = NULL;	

	unsigned int		xdots, ydots;

	if(lcd_channel >= XM_LCDC_CHANNEL_COUNT)
		return NULL;
	// ���LCDC���ͨ���Ƿ��ʼ��
	if(lcdc_enable[lcd_channel] == 0)
		return NULL;

	// If the biCompression member of the BITMAPINFOHEADER is BI_BITFIELDS, 
	// the bmiColors member contains three DWORD color masks that specify the red, green, 
	// and blue components, respectively, of each pixel. 
	// Each WORD in the bitmap array represents a single pixel.
	// ��ȡLCDC���ͨ���ĳߴ�
	xdots = lcdc_xdots[lcd_channel];
	ydots = lcdc_ydots[lcd_channel];
	
	if(Global)
	{
		hBitmap = GlobalAlloc (GMEM_MOVEABLE ,sizeof(BITMAPINFOHEADER) + 3 * 4 + 4 * xdots * ydots);
		lpbi = (BITMAPINFOHEADER *)GlobalLock (hBitmap);
	}
	else
	{
		lpbi = (BITMAPINFOHEADER *)malloc (sizeof(BITMAPINFOHEADER) + 3 * 4 + 4 * xdots * ydots);
	}
	if(lpbi == NULL)
		return NULL;
	// д���ļ�ͷ��Ϣ
	lpbi->biSize               = sizeof(BITMAPINFOHEADER);
	lpbi->biWidth              = xdots;
	lpbi->biHeight             = ydots;
	lpbi->biPlanes             = 1;
	lpbi->biBitCount           = 32;
	lpbi->biCompression        = BI_BITFIELDS;
	lpbi->biSizeImage          = 0;
	lpbi->biXPelsPerMeter      = 0;
	lpbi->biYPelsPerMeter      = 0;
	lpbi->biClrUsed            = 256;
	lpbi->biClrImportant       = 0;
	// д���ɫ����Ϣ
	lpdwColor = (DWORD *)(lpbi+1);
	*lpdwColor ++ = 0x00FF0000;		// R������Ĥ
	*lpdwColor ++ = 0x0000FF00;		// G������Ĥ	
	*lpdwColor ++ = 0x000000FF;		// B������Ĥ	

	// д��λͼ����
	lpData = (DWORD *)lpdwColor;
	for (h = 0; h < ydots; h++)
	{
		for (w = 0; w < xdots; w++)
		{
			unsigned int r, g, b;
			unsigned int a1, r1, g1, b1;
			unsigned char *rgb;

			r = reg_background_r[lcd_channel];
			g = reg_background_g[lcd_channel];
			b = reg_background_b[lcd_channel];

			// ����OSD��
			for (layer = 0; layer < XM_OSD_LAYER_COUNT; layer ++)
			{
				if(reg_osd_enable[lcd_channel][layer] && reg_osd_y_addr[lcd_channel][layer] == NULL)
				{
				}
				else if(reg_osd_enable[lcd_channel][layer] && reg_osd_y_addr[lcd_channel][layer])
				{
					// OSDʹ��
					unsigned int osd_format = reg_osd_format[lcd_channel][layer];
					int x, y;
					unsigned int osd_width, osd_height;
					unsigned int osd_brightness_coeff;
					x = w - reg_osd_h_position[lcd_channel][layer];
					x += reg_osd_left_position[lcd_channel][layer];
					y = h - reg_osd_v_position[lcd_channel][layer];
					osd_width = reg_osd_width[lcd_channel][layer];
					osd_height = reg_osd_height[lcd_channel][layer];
					if(w < reg_osd_h_position[lcd_channel][layer])
					{
						// ��Ӧ��ǰLCD�����OSD1�㲻�ɼ�
						continue;
					}
					if(x < 0 || y < 0)
					{
						// ��Ӧ��ǰLCD�����OSD1�㲻�ɼ�
						continue;
					}
				//	else if(x >= (int)(osd_width) || y >= (int)(osd_height))					
					else if(x >= (int)(osd_width + reg_osd_left_position[lcd_channel][layer]) || y >= (int)(osd_height))
					{
						// ��Ӧ��ǰLCD�����OSD1�㲻�ɼ�
						continue;
					}

					rgb = reg_osd_y_addr[lcd_channel][layer];
					osd_brightness_coeff = reg_osd_brightness_coeff[lcd_channel][layer];
					if(osd_format == XM_OSD_LAYER_FORMAT_YUV420)
					{
						unsigned char *y_addr = reg_osd_y_addr[lcd_channel][layer];
						unsigned char *u_addr = reg_osd_u_addr[lcd_channel][layer];
						unsigned char *v_addr = reg_osd_v_addr[lcd_channel][layer];
						
						int c1, c2, c3, c4;
						int y1;
						y_addr += reg_osd_width[lcd_channel][layer] * y + x;
						u_addr += (reg_osd_width[lcd_channel][layer] * (y >> 1) + x) >> 1;
						v_addr += (reg_osd_width[lcd_channel][layer] * (y >> 1) + x) >> 1;
						c1 = crv_tab[*v_addr];
						c2 = cgu_tab[*u_addr];
						c3 = cgv_tab[*v_addr];
						c4 = cbu_tab[*u_addr];

						y1 = tab_76309[*y_addr]; 
						r1 = clp[384 + ((y1 + c1)>>16)];  
						g1 = clp[384 + ((y1 - c2 - c3)>>16)];
						b1 = clp[384 + ((y1 + c4)>>16)];
						// ʹ��ȫ������
						a1 = reg_osd_global_coeff[lcd_channel][layer];
						// YUV��֧����������
						r1 = r1 * osd_brightness_coeff / 64;
						g1 = g1 * osd_brightness_coeff / 64;
						b1 = b1 * osd_brightness_coeff / 64;	

					}
					else if(osd_format == XM_OSD_LAYER_FORMAT_ARGB888)
					{
						DWORD* clr =  (DWORD *)(rgb + (reg_osd_stride[lcd_channel][layer] * y + x) * 4);
						a1 = (*clr >> 24) & 0xFF;
						r1 = (*clr >> 16) & 0xFF;
						g1 = (*clr >> 8 ) & 0xFF;
						b1 = (*clr >> 0 ) & 0xFF;
						r1 = r1 * osd_brightness_coeff / 64;
						g1 = g1 * osd_brightness_coeff / 64;
						b1 = b1 * osd_brightness_coeff / 64;	
					}
					else if(osd_format == XM_OSD_LAYER_FORMAT_RGB565)
					{
						WORD* clr =  (WORD *)(rgb + (reg_osd_stride[lcd_channel][layer] * y + x) * 2);
						// RGB565��alpha����ʹ��osd_global_coeff, ��ʹosd_global_coeff_enable[1] == 0
						a1 = reg_osd_global_coeff[lcd_channel][layer];
						r1 = (*clr >> 11) << 3;
						g1 = ((*clr >> 5) & 0x3F) << 2;
						b1 = ((*clr >> 0) & 0x1F) << 3;
						r1 = r1 * osd_brightness_coeff / 64;
						g1 = g1 * osd_brightness_coeff / 64;
						b1 = b1 * osd_brightness_coeff / 64;	
					}
					else if(osd_format == XM_OSD_LAYER_FORMAT_ARGB454)
					{
						WORD* clr =  (WORD *)(rgb + (reg_osd_stride[lcd_channel][layer] * y + x) * 2);
						a1 = (*clr >> 13) << 5;
						if(a1 == 0xE0)	// 0xE0����Alphaֵ255
							a1 = 0xFF;
						r1 = ((*clr >> 9) & 0x0F) << 4;
						g1 = ((*clr >> 4) & 0x1F) << 3;
						b1 = ((*clr >> 0) & 0x0F) << 4;
						r1 = r1 * osd_brightness_coeff / 64;
						g1 = g1 * osd_brightness_coeff / 64;
						b1 = b1 * osd_brightness_coeff / 64;	
					}
					else
					{
						a1 = 0;
						r1 = 0;
						g1 = 0;
						b1 = 0;
					}

					if(reg_osd_global_coeff_enable[lcd_channel][layer] 
						|| osd_format == XM_OSD_LAYER_FORMAT_YUV420		// YUV420��alpha����ʹ��osd_global_coeff
						|| osd_format == XM_OSD_LAYER_FORMAT_RGB565		// RGB565��alpha����ʹ��osd_global_coeff
						)
					{
						// ȫ�ֻ������
						unsigned int global_coeff = reg_osd_global_coeff[lcd_channel][layer];
						r = (r1 * global_coeff + r * (63 - global_coeff)) / 63;
						g = (g1 * global_coeff + g * (63 - global_coeff)) / 63;
						b = (b1 * global_coeff + b * (63 - global_coeff)) / 63;
					}
					else
					{
						// ʹ��a���ӻ��
						r = (r1 * a1 + r * (0xFF - a1)) / 0xFF;
						g = (g1 * a1 + g * (0xFF - a1)) / 0xFF;
						b = (b1 * a1 + b * (0xFF - a1)) / 0xFF;
					}
					r &= 0xFF;
					g &= 0xFF;
					b &= 0xFF;

				}
			}

			if(lcdc_display_on[lcd_channel])
				lpData[(ydots - 1 - h)*xdots + w] = (r << 16) | (g << 8) | (b << 0);
			else
				lpData[(ydots - 1 - h)*xdots + w] = 0;
		}
	}

	if(Global)
	{
		GlobalUnlock (hBitmap);
		return (BITMAPINFOHEADER *)hBitmap;
	}

	return lpbi;
}

HBITMAP video_create_bitmap_display (unsigned int lcd_channel)
{
	BITMAPINFOHEADER	*lpbi;  
	HDC               hdc;
	HBITMAP           hbm;
	
	lpbi = video_create_bitmap (lcd_channel, FALSE);
	if(lpbi == NULL)
		return NULL;
	hdc = GetDC(NULL);
	hbm = CreateDIBitmap (hdc,
                (LPBITMAPINFOHEADER)lpbi,
                (LONG)CBM_INIT,
                (LPSTR)lpbi + lpbi->biSize + 3*4,
                (LPBITMAPINFO)lpbi,
                DIB_RGB_COLORS );

	free (lpbi);
	ReleaseDC (NULL, hdc);
	return hbm;

}

void video_draw_bitmap (HDC hdc, HBITMAP  hbm, int x, int y)
{
	HDC       hdcBits;
	BITMAP    bm;
	BOOL		 f;
	HBITMAP   hOldBitmap;		
	
	hdcBits = CreateCompatibleDC(hdc);
	GetObject (hbm, sizeof(BITMAP), (LPSTR)&bm);
	hOldBitmap = (HBITMAP)SelectObject(hdcBits, hbm);
	f = BitBlt (hdc, x, y, bm.bmWidth, bm.bmHeight, hdcBits, 0, 0, SRCCOPY);
	SelectObject(hdcBits, hOldBitmap);
	DeleteDC(hdcBits);
}

// ��Win32 HDC�豸��ָ��λ����ʾlcdc�����ͨ��
void FML_VideoUpdate (HDC hdc, int x, int y, unsigned int lcdc_channel)
{
	HBITMAP hbmp;

	hbmp = video_create_bitmap_display(lcdc_channel);
	if(hbmp)
	{
		video_draw_bitmap (hdc, hbmp, x, y);
		DeleteObject (hbmp);
	}
}

// ����win32λͼclipboard
void FML_CreateBitmapClipboard (HWND hWnd, unsigned int lcd_channel)
{
	BITMAPINFOHEADER *hBmpInfo;
	HWND hWndOwner;

	hBmpInfo = (BITMAPINFOHEADER *)video_create_bitmap	(lcd_channel, TRUE);

	hWndOwner = GetOpenClipboardWindow();
	if(hWndOwner)
	{
		CloseClipboard ();
	}
	OpenClipboard (hWnd);
	EmptyClipboard();
	SetClipboardData (CF_DIB, hBmpInfo);
	CloseClipboard ();
}

#endif