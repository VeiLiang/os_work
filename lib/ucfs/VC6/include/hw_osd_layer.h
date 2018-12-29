//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: hw_osd_layer.h
//	  constant��macro & basic typedef definition of OSD layer service
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************

// ARK1960 OSD LAYERӲ���ӿ�
#ifndef _HW_OSD_LAYER_H_
#define _HW_OSD_LAYER_H_

#if defined (__cplusplus)
	extern "C"{
#endif

#define	_XM_USE_Y_UV420_		// ʹ��Y_UV420, UV������ģʽ

// ARK1960����1����Ƶ���ͨ��
enum {
	XM_LCDC_CHANNEL_0 = 0,			// ͨ��0
//	XM_LCDC_CHANNEL_1,				// ͨ��1
	XM_LCDC_CHANNEL_COUNT
}; 


// LCDC���ͨ�����Ͷ���
enum {
	// RGB LCD���ӿ�
	XM_LCDC_TYPE_RGB = 0,		// LCD RGB���ӿ�

	// CPU LCD���ӿ�
	XM_LCDC_TYPE_CPU,				// LCD CPU���ӿ�

	XM_LCDC_TYPE_BT601,			// ITU BT601������Ƶ

	// VGA����
	XM_LCDC_TYPE_VGA,				// VGA��Ƶ�ӿ� 640 X 480
	XM_LCDC_TYPE_SVGA,			// SVGA��Ƶ�ӿ� 800 X 600

	// AVOUT ����
	XM_LCDC_TYPE_CVBS_NTSC,		// CVBS��Ƶ�ӿ� 720 X 480
	XM_LCDC_TYPE_CVBS_PAL,		// CVBS��Ƶ�ӿ� 720 X 576
	XM_LCDC_TYPE_YPbPr_480i,	// YPbPr ɫ������ӿ� 720 X 480
	XM_LCDC_TYPE_YPbPr_480p,	// YPbPr ɫ������ӿ� 720 X 480
	XM_LCDC_TYPE_YPbPr_576i,	// YPbPr ɫ������ӿ� 720 X 576
	XM_LCDC_TYPE_YPbPr_576p,	// YPbPr ɫ������ӿ� 720 X 576

	// HDMI ����
	XM_LCDC_TYPE_HDMI_1080i,	// HDMI 1080i�ӿڣ�����ɨ�� 1920 X 1080
	XM_LCDC_TYPE_HDMI_1080p,	// HDMI 1080p�ӿڣ�����ɨ�� 1920 X 1080
	XM_LCDC_TYPE_HDMI_720i,		// HDMI 720i�ӿڣ�����ɨ�� 1280 X 720
	XM_LCDC_TYPE_HDMI_720p,		// HDMI 720p�ӿڣ�����ɨ�� 1280 X 720

	XM_LCDC_TYPE_COUNT
};

// OSD�㶨��(3��)
enum {
	XM_OSD_LAYER_0 = 0,			// OSD ��0��
	XM_OSD_LAYER_1,				// OSD ��1��
	XM_OSD_LAYER_2,				// OSD ��2��
	XM_OSD_LAYER_COUNT
};

// OSD Layer��ʽ(YUV420, ARGB888, RGB565, ARGB454, )
enum {
	XM_OSD_LAYER_FORMAT_YUV420 = 	0,	// YUV420,	Y��U��V���ݷֿ���,
	XM_OSD_LAYER_FORMAT_ARGB888,		// 32λ AAAAAAAARRRRRRRRGGGGGGGGBBBBBBBB
	XM_OSD_LAYER_FORMAT_RGB565,		// 16λ RRRRRGGGGGGBBBBB
	XM_OSD_LAYER_FORMAT_ARGB454,		// 16λ AAARRRRGGGGGBBBB
	XM_OSD_LAYER_FORMAT_AYUV444,		// YUV444
	XM_OSD_LAYER_FORMAT_Y_UV420,		// Y_UV420, Y��UV���ݷֿ���, UV���ݽ�����
	XM_OSD_LAYER_FORMAT_COUNT
};

// OSD�����ʾ��(ԭ��λ�����Ͻ�)ƫ��
#define	OSD_MAX_H_POSITION	(2048 - 1)		// ���ˮƽ����ƫ��(������Ͻ�)
#define	OSD_MAX_V_POSITION	(2048 - 1)		// ���ֱ����ƫ��(������Ͻ�)

#define	OSD_MAX_H_SIZE			(2048)			// OSD������ؿ��
#define	OSD_MAX_V_SIZE			(2048)			// OSD������ظ߶�


#define	OSD_MAX_YUV_SIZE		(2048)			// OSD YUV�����ߴ�
#define	OSD_MAX_RGB_SIZE		(1280)			// OSD RGB�����ߴ�

typedef void (*osd_callback_t)(void *osd_data);


// LCDC���ͨ����ʼ��
// lcdc_channel	LCDC��Ƶ���ͨ����(��ʾ�豸)
// lcdc_type		LCDC��Ƶ���ͨ���ӿ�����
// xdots				LCDC��Ƶ���ͨ�����ؿ�� 
// ydots				LCDC��Ƶ���ͨ�����ظ߶�
void HW_lcdc_init (unsigned int lcdc_channel, unsigned int lcdc_type, unsigned int xdots, unsigned int ydots);

// LCDC���ͨ���ر�(�ر�ʱ�ӡ��رտ����߼�)
void HW_lcdc_exit (unsigned int lcdc_channel);


// ����/�ر�LCDC��Ƶͨ������ʾ
// on  1  ������ʾ
// on  0  �ر���ʾ
void HW_lcdc_set_display_on (unsigned int lcdc_channel, unsigned int on);

// ����/�ر�LCDC��Ƶͨ���ı�����ʾ
// lcdc_channel	LCDC��Ƶ���ͨ����(��ʾ�豸)
// on  0.0 ~ 1.0  
//     0.0  ����ر�
//     1.0  �����������
void HW_lcdc_set_backlight_on (unsigned int lcdc_channel, float on);
void HW_lcdc_set_osd_on (unsigned int lcdc_channel, float on);
// ��ȡLCDC��Ƶͨ���ı�����ʾ����
// ����ֵ 0.0 ~ 1.0
//     0.0  ����ر�
//     1.0  �����������
float HW_lcdc_get_backlight_on (unsigned int lcdc_channel);

// ��ȡCDC���ͨ����ƽ����
unsigned int HW_lcdc_get_xdots (unsigned int lcd_channel);
// ��ȡCDC���ͨ����ƽ��߶�
unsigned int HW_lcdc_get_ydots (unsigned int lcd_channel);


// ����LCDC���ͨ������ɫ
void HW_lcdc_set_background_color (unsigned int lcd_channel, 
													unsigned char r, unsigned char g, unsigned char b);

void HW_lcdc_osd_set_enable (unsigned int lcd_channel, unsigned int osd_layer, int enable);
unsigned int HW_lcdc_osd_get_enable (unsigned int lcd_channel, unsigned int osd_layer);


void HW_lcdc_osd_set_global_coeff_enable (unsigned int lcd_channel, unsigned int osd_layer, int enable);

void HW_lcdc_osd_set_global_coeff (unsigned int lcd_channel, unsigned int osd_layer, unsigned int global_coeff);

void HW_lcdc_osd_set_format (unsigned int lcd_channel, unsigned int osd_layer, unsigned int format);

void HW_lcdc_osd_set_width (unsigned int lcd_channel, unsigned int osd_layer, unsigned int width);

void HW_lcdc_osd_set_height (unsigned int lcd_channel, unsigned int osd_layer, unsigned int height);

void HW_lcdc_osd_set_h_position (unsigned int lcd_channel, unsigned int osd_layer, unsigned int h_position);

void HW_lcdc_osd_set_v_position (unsigned int lcd_channel, unsigned int osd_layer, unsigned int v_position);

void HW_lcdc_osd_set_left_position (unsigned int lcd_channel, unsigned int osd_layer, unsigned int left_position);

// ����OSD�������Ƶ�������������س���
void HW_lcdc_osd_set_stride (unsigned int lcd_channel, unsigned int osd_layer, unsigned int stride);

void HW_lcdc_osd_set_brightness_coeff (unsigned int lcd_channel, unsigned int osd_layer, unsigned int brightness_coeff);

void HW_lcdc_osd_set_yuv_addr (unsigned int lcd_channel, unsigned int osd_layer, unsigned char *y_addr, unsigned char *u_addr, unsigned char *v_addr);

// LCDC֡ˢ���жϴ���
void HW_lcdc_interrupt_handler (void);

// ��װOSD֡ˢ���жϴ�����
void HW_lcdc_osd_set_callback (unsigned int lcd_channel, unsigned int osd_layer, 
										 osd_callback_t osd_callback, void *osd_private);

const char *XM_lcdc_channel_name (unsigned int lcdc_channel);

const char *XM_lcdc_channel_type (unsigned int lcdc_type);

// ͬ��OSD������LCDC���������ڲ��Ĵ���
void HW_lcdc_set_osd_coef_syn (unsigned int lcdc_channel, unsigned int coef_syn);

// ��ȡͬ��״̬��
// 0 ��ʾOSD������ͬ����LCDC���������ڲ��Ĵ���
unsigned int HW_lcdc_get_osd_coef_syn (unsigned int lcdc_channel);



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _HW_OSD_LAYER_H_
