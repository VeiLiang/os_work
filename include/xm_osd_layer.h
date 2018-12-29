//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_osd_layer.h
//	  constant��macro & basic typedef definition of OSD layer service
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
// OSD�߼���

#ifndef _XM_OSD_LAYER_H_
#define _XM_OSD_LAYER_H_

#include "hw_osd_layer.h"

#if defined (__cplusplus)
	extern "C"{
#endif


typedef unsigned int XM_COLORREF;	// 0xaarrggbb

#define	XM_RGB(r,g,b)		( (((XM_COLORREF)(b))&0xFF) | ((((XM_COLORREF)(g))&0xFF)<<8) | ((((XM_COLORREF)(r))&0xFF)<<16) | (((XM_COLORREF)0xFF) << 24))
#define	XM_ARGB(a,r,g,b)		( (((XM_COLORREF)(b))&0xFF) | ((((XM_COLORREF)(g))&0xFF)<<8) | ((((XM_COLORREF)(r))&0xFF)<<16) | (((XM_COLORREF)(a)) << 24))
#define	XM_GetBValue(rgb)	( (unsigned char)((rgb) & 0xFF) )
#define	XM_GetGValue(rgb)	( (unsigned char)(((rgb) >> 8) & 0xFF) )
#define	XM_GetRValue(rgb)	( (unsigned char)(((rgb) >> 16) & 0xFF) )

#define	ARGB888_TO_ARGB454(clr)		(unsigned short)(((((clr) >> 16) & 0xF0) << 5) \
											| ((((clr) >> 8) & 0xF8) << 1) \
											| (((clr) & 0xF0) >> 4) \
											| (((clr) >> 29) << 13) )

#define	ARGB454_TO_ARGB888(clr)		(unsigned int)( ((((clr) >> 9) & 0x0F) << 20) \
											| ((((clr) >> 4) & 0x1F) << 11) \
											| ((((clr) >> 0) & 0x0F) << 4) \
											| ((((clr) >> 13) & 0x07) << 29) )

#define	ARGB888_TO_RGB565(clr)		(unsigned short)(((((clr) >> 19) & 0x1F) << 11) \
											| ((((clr) >> 10) & 0x3F) << 5) \
											| (((clr) & 0xF8) >> 3) )

#define	RGB565_TO_ARGB888(clr)		(unsigned int)( ((((clr) >> 11) & 0x1F) << 19) \
											| ((((clr) >> 5) & 0x3F) << 10) \
											| ((((clr) >> 0) & 0x1F) << 3) | 0xFF000000 )


typedef struct tagXM_OSDLAYER {
	unsigned int		osd_layer_format;				// OSD�����ظ�ʽ
	unsigned int		osd_stride;						// OSDԴ�������ֽڳ���(RGB��ʽԴ����ʹ��)
	unsigned int		osd_brightness_coeff;		// OSD���������ӣ�0 ~ 64
																//		0	
																//		64	ԭʼ����ֵ
	int					osd_x_position;				// OSD�����LCDԭ���ˮƽƫ��
	int					osd_y_position;				// OSD�����LCDԭ��Ĵ�ֱƫ��
	unsigned int		osd_width;						// OSD����ʾ���
	unsigned int		osd_height;						// OSD����ʾ�߶�
	
	unsigned int		viewable_en;					// OSD�߼����������ʹ�ܱ�־ 0 ��ֹ 1 ʹ��
	unsigned int		viewable_x;						// OSD�߼�������������LCDԭ���ˮƽƫ��
	unsigned int		viewable_y;						// OSD�߼�������������LCDԭ��Ĵ�ֱƫ��
	unsigned int		viewable_w;						// OSD�߼����������Ŀ��
	unsigned int		viewable_h;						// OSD�߼����������ĸ߶�
	
	unsigned int		osd_global_coeff;				// ��ǰ����ǰһ���ȫ�ֻ������
	unsigned int		osd_global_coeff_enable;	// ȫ�ֻ������ʹ�ܱ�־ 0 ��ֹ 1 ʹ��
	unsigned int		osd_enable;						// OSD��ʹ�ܱ�־ 0 ��ֹ 1 ʹ��
	unsigned char *	osd_y_addr;						// 1) YUV420��ʽԴ���ݵ�Y������ʼ��ַ
																// 2) ������ʽԴ���ݵ���ʼ��ַ
	unsigned char *	osd_u_addr;						// 1) YUV420��ʽԴ���ݵ�U������ʼ��ַ
	unsigned char *	osd_v_addr;						// 1) YUV420��ʽԴ���ݵ�V������ʼ��ַ

	unsigned int		osd_h_min_moving_pixels;	// OSD��ˮƽ������С�ƶ����ص����
																// ��ͬ���ظ�ʽ��ˮƽ��С�ƶ����ص㲻ͬ
	unsigned int		osd_v_min_moving_pixels;	// OSD�㴹ֱ������С�ƶ����ص����
																// ��ͬ���ظ�ʽ��ˮƽ��С�ƶ����ص㲻ͬ

	osd_callback_t		osd_callback;
	void *				osd_private;

	
} XM_OSDLAYER;

// YUVӲ��Buffer���ƣ�ÿ16�����ص�Ϊһ����СBuffer��Ԫ��ʾ��
// ��YUVƽ���з������Ϊ16�ֽڶ��룬�з���Ϊ2�ж���
unsigned int XM_lcdc_osd_horz_align (unsigned int lcdc_channel, unsigned int x);
unsigned int XM_lcdc_osd_vert_align (unsigned int lcdc_channel, unsigned int y);

// ����һ��OSD�߼���
// ��layer_enable == 1����OSD��ʹ�ܣ����������Ӧ��LCDC���ͨ��
// ����ֵ == NULL ��ʾ����ʧ��
XM_OSDLAYER * XM_osd_layer_init (
								unsigned int lcd_channel,						// LCDC���ͨ��
								unsigned int osd_layer,							// OSD��ͨ����
								unsigned int layer_enable,						// ͨ��ʹ��(1)��ر�(0)
								unsigned int layer_format,						// OSDͨ��ƽ���ʽ
								unsigned int layer_global_coeff,				// ȫ��coeff���� (0 ~ 63)
								unsigned int layer_global_coeff_enable,	// �Ƿ�ʹ��ȫ��coeff����(0 ~ 1)
								unsigned int layer_brightness_coeff,		// �������� (0 ~ 64)
								unsigned int layer_width, unsigned int layer_height,
																						// OSDͨ��ƽ��ߴ�
								unsigned int layer_stride,						// OSDͨ����ʾ�������ֽڳ���
								int layer_offset_x, int layer_offset_y,	// OSDͨ�������LCD��ʾԭ��(���Ͻ�)�ĳ�ʼƫ��
								unsigned char *y_addr, unsigned char *u_addr, unsigned char *v_addr,
																						// OSDͨ����ʾ���ݻ�����ָ��
																						// RGB��ʽ����y_addr��Ч
																						// YUV��ʽ��y_addr,u_addr,v_addr����Ҫ
								osd_callback_t layer_callback,				// layer�ص�����������
								void *layer_private
								);


// LCDC OSD�߼����ʼ��
void XM_lcdc_osd_init (void);
void XM_lcdc_osd_exit (void);


// ʹ�ܻ��ֹOSD�߼���
void XM_lcdc_osd_set_enable (unsigned int lcd_channel, unsigned int osd_layer, int enable);
unsigned int XM_lcdc_osd_get_enable (unsigned int lcd_channel, unsigned int osd_layer);

// ʹ�ܻ��ֹOSD�߼����ȫ��alpha����
void XM_lcdc_osd_set_global_coeff_enable (unsigned int lcd_channel, unsigned int osd_layer, unsigned int enable);
unsigned int XM_lcdc_osd_get_global_coeff_enable (unsigned int lcd_channel, unsigned int osd_layer);

// ����OSD�߼����ȫ��alpha����
void XM_lcdc_osd_set_global_coeff (unsigned int lcd_channel, unsigned int osd_layer, unsigned int global_coeff);
unsigned int XM_lcdc_osd_get_global_coeff (unsigned int lcd_channel, unsigned int osd_layer);

// ����OSD�߼����Դ������ʾ��ʽ
void XM_lcdc_osd_set_format (unsigned int lcd_channel, unsigned int osd_layer, unsigned int format);
unsigned int  XM_lcdc_osd_get_format (unsigned int lcd_channel, unsigned int osd_layer);

// ����OSD�߼���ĳߴ�
void XM_lcdc_osd_set_width (unsigned int lcd_channel, unsigned int osd_layer, unsigned int width);
unsigned int XM_lcdc_osd_get_width (unsigned int lcd_channel, unsigned int osd_layer);
void XM_lcdc_osd_set_height (unsigned int lcd_channel, unsigned int osd_layer, unsigned int height);
unsigned int XM_lcdc_osd_get_height (unsigned int lcd_channel, unsigned int osd_layer);

// ����OSD�߼���ԭ��(OSD�߼������Ͻ�)��LCD���ͨ���ϵ����ƫ��(���LCD�豸�����Ͻǣ�������)
void XM_lcdc_osd_set_x_offset (unsigned int lcd_channel, unsigned int osd_layer, int x_offset);
int XM_lcdc_osd_get_x_offset (unsigned int lcd_channel, unsigned int osd_layer);
void XM_lcdc_osd_set_y_offset (unsigned int lcd_channel, unsigned int osd_layer, int y_offset);
int XM_lcdc_osd_get_y_offset (unsigned int lcd_channel, unsigned int osd_layer);

// ����OSD�߼������ʾƽ�����ݻ�����
// RGB��ʽ����Ҫy_addr
// YUV��ʽ��ָ��y_addr��u_addr��v_addr
void XM_lcdc_osd_set_yuv_addr (unsigned int lcd_channel, unsigned int osd_layer, 
										 unsigned char *y_addr, unsigned char *u_addr, unsigned char *v_addr);
unsigned char * XM_lcdc_osd_get_y_addr (unsigned int lcd_channel, unsigned int osd_layer);
unsigned char * XM_lcdc_osd_get_u_addr (unsigned int lcd_channel, unsigned int osd_layer);
unsigned char * XM_lcdc_osd_get_v_addr (unsigned int lcd_channel, unsigned int osd_layer);

// ����OSD�߼������������
void XM_lcdc_osd_set_brightness_coeff (unsigned int lcd_channel, unsigned int osd_layer, unsigned int mult_coeff);
unsigned int XM_lcdc_osd_get_brightness_coeff (unsigned int lcd_channel, unsigned int osd_layer);

// ����OSD�߼�����Ƶ�����������ֽڳ���
void XM_lcdc_osd_set_stride (unsigned int lcd_channel, unsigned int osd_layer, unsigned int stride);
unsigned int XM_lcdc_osd_get_stride (unsigned int lcd_channel, unsigned int osd_layer);

// ����OSD�߼���ص�����
void XM_lcdc_osd_set_callback (unsigned int lcd_channel, unsigned int osd_layer, 
										 osd_callback_t osd_callback, void *osd_private);

// ����OSD�߼���Ŀ�������
void XM_lcdc_osd_set_viewable_width (unsigned int lcd_channel, unsigned int osd_layer, unsigned int width);
unsigned int XM_lcdc_osd_get_viewable_width (unsigned int lcd_channel, unsigned int osd_layer);
void XM_lcdc_osd_set_viewable_height (unsigned int lcd_channel, unsigned int osd_layer, unsigned int height);
unsigned int XM_lcdc_osd_get_viewable_height (unsigned int lcd_channel, unsigned int osd_layer);

// ����OSD�߼�����������ƫ��λ��(�����Ƶ��ʾͨ�������Ͻ�)
void XM_lcdc_osd_set_viewable_x_offset (unsigned int lcd_channel, unsigned int osd_layer, int x_offset);
int XM_lcdc_osd_get_viewable_x_offset (unsigned int lcd_channel, unsigned int osd_layer);
void XM_lcdc_osd_set_viewable_y_offset (unsigned int lcd_channel, unsigned int osd_layer, int y_offset);
int XM_lcdc_osd_get_viewable_y_offset (unsigned int lcd_channel, unsigned int osd_layer);

// ʹ�ܻ��ֹOSD�߼���Ŀ�������
void XM_lcdc_osd_set_viewable_enable (unsigned int lcd_channel, unsigned int osd_layer, int enable);
unsigned int XM_lcdc_osd_get_viewable_enable (unsigned int lcd_channel, unsigned int osd_layer);

// ����LCDC��UI����Ƶ��ʽ
void XM_lcdc_osd_set_ui_format (unsigned int lcd_channel, unsigned int layer_format);
// ��ȡLCDC��UI����Ƶ��ʽ
unsigned int XM_lcdc_osd_get_ui_format (unsigned int lcd_channel);

// ����/����LCDC��OSD��
void XM_lcdc_osd_layer_set_lock (unsigned int lcd_channel, unsigned int osd_layer_mask, unsigned int lock_mask);


// ��PNG��ʽ��Դ������ʾ��OSD Layer��ָ��λ��
// ��֧��RGB��ʽ��OSD��
// osd_layer_buffer    OSD layer��ԭ�㻺����ָ��
// osd_layer_format    OSD layer�ĸ�ʽ(ARGB8888/RGB565/ARGB3454/YUV420)
// osd_layer_width     OSD layer�����ؿ��
// osd_layer_height    OSD layer�����ظ߶�
// osd_layer_stride    OSD layer�����ֽڿ��
// osd_image_x         PNG image����ʾ����(�����OSD��ԭ���ƫ�ƣ�>=0 )
// osd_image_y
// png_image_buff	     PNG image���ݻ�����
// png_image_size		  PNG image�����ֽڳ���
// transparent_color   ����PNG image��͸��ɫ����
//                     (unsigned int)(0xFFFFFFFF)  ͸��ɫ
//                     ��Ϊ����ɫ�����OSD layerָ����λ��
// layer_alpha			   PNG image��ȫ����Alpha����
int  XM_lcdc_load_png_image (unsigned char *osd_layer_buffer, 
									unsigned int osd_layer_format,
									unsigned int osd_layer_width, unsigned int osd_layer_height,
									unsigned int osd_layer_stride,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *png_image_buff, unsigned int png_image_size,
									XM_COLORREF transparent_color,
									unsigned char layer_alpha);


// ��PNG��ʽ��Դ������ʾ��OSD Layer��ָ��λ��
// ��֧��RGB��ʽ��OSD��
// lcd_channel			  LCDCͨ����
// osd_layer			  OSD�����
// osd_image_x         PNG image����ʾ����(�����OSD��ԭ���ƫ�ƣ�>=0 )
// osd_image_y
// png_image_buff		  PNG image�����ݻ���
// png_image_size		  PNG image�����ݻ����ֽڴ�С
// transparent_color   ����PNG image��͸��ɫ����
//                     (unsigned int)(0xFFFFFFFF)  ͸��ɫ
//                     ��Ϊ����ɫ�����OSD layerָ����λ��
int  XM_lcdc_osd_layer_load_png_image (unsigned int lcd_channel,
													unsigned int osd_layer,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *png_image_buff, unsigned int png_image_size,
									XM_COLORREF transparent_color,
									unsigned char alpha);

// ��PNG��ʽ���ļ���ʾ��OSD Layer��ָ��λ��
// ��֧��RGB��ʽ��OSD��
// lcd_channel			  LCDCͨ����
// osd_layer			  OSD�����
// osd_image_x         PNG image����ʾ����(�����OSD��ԭ���ƫ�ƣ�>=0 )
// osd_image_y
// png_image_name		  PNG image���ļ���
// transparent_color   ����PNG image��͸��ɫ����
//                     (unsigned int)(0xFFFFFFFF)  ͸��ɫ
//                     ��Ϊ����ɫ�����OSD layerָ����λ��
int  XM_lcdc_osd_layer_load_png_image_file (	
										unsigned int lcd_channel,
										unsigned int osd_layer,
										unsigned int osd_image_x, unsigned int osd_image_y,
										const char *png_image_name,
										XM_COLORREF transparent_color,
										unsigned char alpha);

// ��PNG��ʽ���ļ���ʾ��OSD Layer��ָ��λ��
// ��֧��RGB��ʽ��OSD��
// osd_layer_buffer    OSD layer��ԭ�㻺����ָ��
// osd_layer_format    OSD layer�ĸ�ʽ(ARGB8888/RGB565/ARGB3454/YUV420)
// osd_layer_width     OSD layer�����ؿ��
// osd_layer_height    OSD layer�����ظ߶�
// osd_layer_stride    OSD layer�����ֽڿ��
// osd_image_x         PNG image����ʾ����(�����OSD��ԭ���ƫ�ƣ�>=0 )
// osd_image_y
// png_image_name	     PNG image�ļ�
// transparent_color   ����PNG image��͸��ɫ����
//                     (unsigned int)(0xFFFFFFFF)  ͸��ɫ
//                     ��Ϊ����ɫ�����OSD layerָ����λ��
int  XM_lcdc_load_png_image_file (unsigned char *osd_layer_buffer, 
									unsigned int osd_layer_format,
									unsigned int osd_layer_width, unsigned int osd_layer_height,
									unsigned int osd_layer_stride,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *png_image_name,
									XM_COLORREF transparent_color,
									unsigned char alpha);


// ��YUV420��ʽԴͼ���ָ������(copy_w, copy_h)���Ƶ�OSD YUV���ָ��λ��(osd_offset_x, osd_offset_y)
// ��YUV420 ���� Y_UV420��֧��
void XM_lcdc_load_yuv_image (
						  unsigned char *yuv_img,		// YUV420��ʽ��Դͼ��,
						  unsigned int yuv_format,		// XM_OSD_LAYER_FORMAT_YUV420 ���� XM_OSD_LAYER_FORMAT_Y_UV420
						  unsigned int img_w, unsigned int img_h,		
																// Դͼ��ԭʼ�ߴ�
						  unsigned int img_offset_x, unsigned int img_offset_y,			
																// ��������λ��Դͼ���е�ƫ��
						  unsigned char *osd_y,			// YUV420 OSD�� Y��ַ
																// Y_UV420 OSD�� Y��ַ
						  unsigned char *osd_u,			// YUV420 OSD�� U��ַ
																// Y_UV420 OSD�� UV��ַ
						  unsigned char *osd_v,			// YUV420 OSD�� V��ַ
																//	Y_UV420 δʹ��
						  unsigned int osd_w, unsigned int osd_h,		
																// OSD���ȡ��߶���Ϣ
						  unsigned int osd_offset_x, unsigned int osd_offset_y,			
																// ��������λ��OSD���ƫ��
						  unsigned int copy_w, unsigned int copy_h		
																// Դͼ������Ҫ���Ƶ�����ߴ�(��Դͼ�����Ͻ�ԭ�㿪ʼ)
						  );


// ��YUV420��ʽԴͼ���ָ������(copy_w, copy_h)���Ƶ�OSD YUV���ָ��λ��(osd_offset_x, osd_offset_y)
// ��YUV��֧��
void XM_lcdc_osd_layer_load_yuv_image (
						  unsigned int lcd_channel,						// LCDCͨ��
						  unsigned int osd_layer,							// OSD��
						  unsigned char *yuv_img,							// YUV420��ʽ��Դͼ��
						  unsigned int img_w, unsigned int img_h,		// Դͼ��ԭʼ�ߴ�
						  unsigned int img_offset_x, unsigned int img_offset_y,			// ��������λ��Դͼ���е�ƫ��
						  unsigned int osd_offset_x, unsigned int osd_offset_y,			// ��������λ��OSD���ƫ��
						  unsigned int copy_w, unsigned int copy_h		// Դͼ������Ҫ���Ƶ�����ߴ�(��Դͼ�����Ͻ�ԭ�㿪ʼ)
						  );

// ��YUV420��ʽԴ�ļ���ָ������(copy_w, copy_h)���Ƶ�OSD YUV���ָ��λ��(osd_offset_x, osd_offset_y)
// ��YUV��֧��
int XM_lcdc_load_yuv_image_file (
						  unsigned char *yuv_file,		// YUV420��ʽ���ļ���
						  unsigned int yuv_format,		// XM_OSD_LAYER_FORMAT_YUV420 ���� XM_OSD_LAYER_FORMAT_Y_UV420
						  unsigned int img_w, unsigned int img_h,		// Դͼ��ԭʼ�ߴ�
						  unsigned int img_offset_x, unsigned int img_offset_y,			// ��������λ��Դͼ���е�ƫ��
						  unsigned char *osd_y,			// OSD�� YUV��ַ
						  unsigned char *osd_u,
						  unsigned char *osd_v,
						  unsigned int osd_w, unsigned int osd_h,			// OSD���ȡ��߶���Ϣ
						  unsigned int osd_offset_x, unsigned int osd_offset_y,			// ��������λ��OSD���ƫ��
						  unsigned int copy_w, unsigned int copy_h		// Դͼ������Ҫ���Ƶ�����ߴ�(��Դͼ�����Ͻ�ԭ�㿪ʼ)
						  );

// ��YUV420��ʽԴ�ļ���ָ������(copy_w, copy_h)���Ƶ�OSD YUV���ָ��λ��(osd_offset_x, osd_offset_y)
// ��YUV��֧��
int XM_lcdc_osd_layer_load_yuv_image_file (
						  unsigned int lcd_channel,						// LCDCͨ��
						  unsigned int osd_layer,							// OSD��
						  unsigned char *yuv_file,							// YUV420��ʽ���ļ���
						  unsigned int img_w, unsigned int img_h,		// Դͼ��ԭʼ�ߴ�
						  unsigned int img_offset_x, unsigned int img_offset_y,			// ��������λ��Դͼ���е�ƫ��
						  unsigned int osd_offset_x, unsigned int osd_offset_y,			// ��������λ��OSD���ƫ��
						  unsigned int copy_w, unsigned int copy_h		// Դͼ������Ҫ���Ƶ�����ߴ�(��Դͼ�����Ͻ�ԭ�㿪ʼ)
						  );

int XM_lcdc_load_gif_image (unsigned char *osd_layer_buffer, 
									unsigned int osd_layer_format,
									unsigned int osd_layer_width, unsigned int osd_layer_height,
									unsigned int osd_layer_stride,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *gif_image_buff, unsigned int gif_image_size,
									XM_COLORREF transparent_color,
									unsigned int alpha);

int XM_lcdc_load_gif_image_file (unsigned char *osd_layer_buffer, 
									unsigned int osd_layer_format,
									unsigned int osd_layer_width, unsigned int osd_layer_height,
									unsigned int osd_layer_stride,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *gif_image_file,
									XM_COLORREF transparent_color,
									unsigned int alpha);

int XM_lcdc_osd_layer_load_gif_image_file (
									unsigned int lcd_channel,
									unsigned int osd_layer,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *gif_image_file,
									XM_COLORREF transparent_color,
									unsigned int alpha);



void XM_lcdc_osd_layer_load_ayuv420_image (
						  unsigned char *img_y,			// YUV��ʽԴͼ���Y������ַ
						  unsigned char *img_u,			// YUV��ʽԴͼ���U������ַ
						  unsigned char *img_v,			// YUV��ʽԴͼ���V������ַ
						  unsigned char *img_a,			// YUV��ʽԴͼ���Alpha������ַ
						  unsigned int img_w,			// YUV��ʽԴͼ������ؿ��
						  unsigned int img_h,			// YUV��ʽԴͼ������ظ߶�
						  unsigned int img_stride,		// YUV��ʽԴͼ������ֽڳ���
						  unsigned int img_offset_x,	// ��������λ��Դͼ���ƫ��(��Դͼ�����Ͻ�ԭ�㿪ʼ)
						  unsigned int img_offset_y,
						  unsigned int copy_w, unsigned int copy_h,		// Դͼ������Ҫ���Ƶ�����ߴ�(��Դͼ�����Ͻ�ԭ�㿪ʼ)
						  unsigned char *osd_y,			// OSD�� YUV��ַ
						  unsigned char *osd_u,
						  unsigned char *osd_v,
						  unsigned int osd_w,			// OSD���ȡ��߶���Ϣ
						  unsigned int osd_h,			
						  unsigned int osd_stride,		// OSD������ֽڳ���
						  unsigned int osd_offset_x,	// ��������λ��OSD���ƫ��
						  unsigned int osd_offset_y			

						  );


// ��ARGB888��ʽ��Դͼ��ת��ΪYUV444��ʽ
void XM_lcdc_convert_argb888_to_ayuv444 (
						  unsigned char *argb_img,		// ARGB888��ʽ��Դͼ��
						  unsigned int img_stride,		// Դͼ������ֽڳ���
						  unsigned int img_w,			// Դͼ��Ŀ��(���ص�)
						  unsigned int img_h,			// Դͼ��ĸ߶�(���ص�)
						  unsigned char *osd_y,			// Ŀ��OSD���YUV��ַ
						  unsigned char *osd_u,
						  unsigned char *osd_v,
						  unsigned char *osd_a,			// ����a����
						  unsigned int osd_stride,		// Ŀ��OSD��(Y����)�����ֽڳ���
																//		(UV��������2)
						  unsigned int osd_w,			// Ŀ��OSD������ؿ��
						  unsigned int osd_h			// Ŀ��OSD������ظ߶�
						  );

void XM_lcdc_osd_layer_load_ayuv444_image_normal (
						  unsigned char *img_y,			// YUV��ʽԴͼ���Y������ַ
						  unsigned char *img_u,			// YUV��ʽԴͼ���U������ַ
						  unsigned char *img_v,			// YUV��ʽԴͼ���V������ַ
						  unsigned char *img_a,			// YUV��ʽԴͼ���Alpha������ַ
						  unsigned int img_w,			// YUV��ʽԴͼ������ؿ��
						  unsigned int img_h,			// YUV��ʽԴͼ������ظ߶�
						  unsigned int img_stride,		// YUV��ʽԴͼ������ֽڳ���
						  unsigned int img_offset_x,	// ��������λ��Դͼ���ƫ��(��Դͼ�����Ͻ�ԭ�㿪ʼ)
						  unsigned int img_offset_y,
						  unsigned int copy_w, unsigned int copy_h,		// Դͼ������Ҫ���Ƶ�����ߴ�(��Դͼ�����Ͻ�ԭ�㿪ʼ)
						  unsigned int	osd_layer_format,
						  unsigned char *osd_y,			// OSD�� YUV��ַ
						  unsigned char *osd_u,
						  unsigned char *osd_v,
						  unsigned int osd_w,			// OSD���ȡ��߶���Ϣ
						  unsigned int osd_h,			
						  unsigned int osd_stride,		// OSD������ֽڳ���
						  unsigned int osd_offset_x,	// ��������λ��OSD���ƫ��
						  unsigned int osd_offset_y
						  );

void XM_lcdc_osd_layer_load_ayuv444_image (
						  unsigned char *img_y,			// YUV��ʽԴͼ���Y������ַ
						  unsigned char *img_u,			// YUV��ʽԴͼ���U������ַ
						  unsigned char *img_v,			// YUV��ʽԴͼ���V������ַ
						  unsigned char *img_a,			// YUV��ʽԴͼ���Alpha������ַ
						  unsigned int img_w,			// YUV��ʽԴͼ������ؿ��
						  unsigned int img_h,			// YUV��ʽԴͼ������ظ߶�
						  unsigned int img_stride,		// YUV��ʽԴͼ������ֽڳ���
						  unsigned int img_offset_x,	// ��������λ��Դͼ���ƫ��(��Դͼ�����Ͻ�ԭ�㿪ʼ)
						  unsigned int img_offset_y,
						  unsigned int copy_w, unsigned int copy_h,		// Դͼ������Ҫ���Ƶ�����ߴ�(��Դͼ�����Ͻ�ԭ�㿪ʼ)
						  unsigned int	osd_layer_format,
						  unsigned char *osd_y,			// OSD�� YUV��ַ
						  unsigned char *osd_u,
						  unsigned char *osd_v,
						  unsigned int osd_w,			// OSD���ȡ��߶���Ϣ
						  unsigned int osd_h,			
						  unsigned int osd_stride,		// OSD������ֽڳ���
						  unsigned int osd_offset_x,	// ��������λ��OSD���ƫ��
						  unsigned int osd_offset_y
						  );

// YUVA444�ֿ�洢
void XM_lcdc_osd_layer_load_yuva444_image (
						  unsigned char *img_y,			// YUV��ʽԴͼ���Y������ַ
						  unsigned int img_w,			// YUV��ʽԴͼ������ؿ��
						  unsigned int img_h,			// YUV��ʽԴͼ������ظ߶�
						  unsigned int img_stride,		// YUV��ʽԴͼ������ֽڳ���
						  unsigned int img_offset_x,	// ��������λ��Դͼ���ƫ��(��Դͼ�����Ͻ�ԭ�㿪ʼ)
						  unsigned int img_offset_y,
						  unsigned int copy_w, unsigned int copy_h,		// Դͼ������Ҫ���Ƶ�����ߴ�(��Դͼ�����Ͻ�ԭ�㿪ʼ)
						  unsigned int	osd_layer_format,
						  unsigned char *osd_y,			// OSD�� YUV��ַ
						  unsigned char *osd_u,
						  unsigned char *osd_v,
						  unsigned int osd_w,			// OSD���ȡ��߶���Ϣ
						  unsigned int osd_h,			
						  unsigned int osd_stride,		// OSD������ֽڳ���
						  unsigned int osd_offset_x,	// ��������λ��OSD���ƫ��
						  unsigned int osd_offset_y
						  );


// ����ѹ����RAW��ʽͼ�����ݸ��Ƶ���ͬ��ʾ��ʽ��OSD��
int XM_lcdc_copy_raw_image (unsigned char *osd_layer_buffer,		// Ŀ��OSD�Ļ�������ַ
																						//		YUV420ΪY��U��V�����ֿ��ʽ
									unsigned int osd_layer_format,			// Ŀ��OSD����Ƶ��ʽ
									unsigned int osd_layer_width,				// Ŀ��OSD�����ؿ�ȡ����ظ߶�
									unsigned int osd_layer_height,	
									unsigned int osd_layer_stride,			// Ŀ��OSD�����ֽڳ���
																						//		YUV420��ʽ��ʾY������UV��������2
									unsigned int osd_offset_x,					//	����������OSD���ƫ�� (���OSDƽ������Ͻ�)
									unsigned int osd_offset_y,
									unsigned char *raw_image_buffer,			// Դͼ�����ݵĻ�������ַ
									unsigned int raw_image_width,				// Դͼ������ؿ��
									unsigned int raw_image_height,			// Դͼ������ظ߶�
									unsigned int raw_image_stride,			// Դͼ������ֽڳ���	
									unsigned int raw_offset_x,					// ����������Դͼ���ƫ��(���Դͼ��ƽ������Ͻ�)
									unsigned int raw_offset_y,
									unsigned int copy_image_w,					// ��������������ؿ�ȡ����ظ߶�		
									unsigned int copy_image_h,
									unsigned int layer_alpha					// OSD���ȫ��Alpha����
									);

int XM_lcdc_load_raw_image (unsigned char *osd_layer_buffer,		// Ŀ��OSD�Ļ�������ַ
																						//		YUV420ΪY��U��V�����ֿ��ʽ
									unsigned int osd_layer_format,			// Ŀ��OSD����Ƶ��ʽ
									unsigned int osd_layer_width,				// Ŀ��OSD�����ؿ�ȡ����ظ߶�
									unsigned int osd_layer_height,	
									unsigned int osd_layer_stride,			// Ŀ��OSD�����ֽڳ���
																						//		YUV420��ʽ��ʾY������UV��������2
									unsigned int osd_offset_x,					//	����������OSD���ƫ�� (���OSDƽ������Ͻ�)
									unsigned int osd_offset_y,
									unsigned char *raw_image_buffer,			// Դͼ�����ݵĻ�������ַ
									unsigned int raw_image_width,				// Դͼ������ؿ��
									unsigned int raw_image_height,			// Դͼ������ظ߶�
									unsigned int raw_image_stride,			// Դͼ������ֽڳ���	
									unsigned int raw_offset_x,					// ����������Դͼ���ƫ��(���Դͼ��ƽ������Ͻ�)
									unsigned int raw_offset_y,
									unsigned int copy_image_w,					// ��������������ؿ�ȡ����ظ߶�		
									unsigned int copy_image_h,
									unsigned int layer_alpha					// OSD���ȫ��Alpha����
									);

int XM_lcdc_flush_dirty_region (unsigned char *osd_layer_buffer,	// OSDƽ��Ļ����ַ
											unsigned int   osd_layer_format,	// OSDƽ��ĸ�ʽ
											unsigned int osd_width, 	// OSDƽ��Ŀ��/�߶�
											unsigned int osd_height,
											unsigned int osd_stride,	// OSDƽ������ֽڳ���
											unsigned int d_x,		// ������ƫ��(�����OSDƽ������Ͻ�ԭ��)
											unsigned int d_y,
											unsigned int d_w,		// �����ĳߴ�
											unsigned int d_h
										);

int XM_lcdc_inv_region (unsigned char *osd_layer_buffer,	// OSDƽ��Ļ����ַ
											unsigned int osd_layer_format,	// OSDƽ��ĸ�ʽ
											unsigned int osd_width, 	// OSDƽ��Ŀ��/�߶�
											unsigned int osd_height,
											unsigned int osd_stride,	// OSDƽ������ֽڳ���
											unsigned int d_x,		// ������ƫ��(�����OSDƽ������Ͻ�ԭ��)
											unsigned int d_y,
											unsigned int d_w,		// �����ĳߴ�
											unsigned int d_h
										);


// ��LCD��Ļ����ת��ΪOSD����߼�����
// ����ֵ
//		-1		ʧ��
//		0		�ɹ�
int XM_lcdc_lcd_coordinate_to_osd_coordinate (unsigned int lcd_channel, unsigned int osd_layer, int *x, int *y);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_OSD_LAYER_H_
