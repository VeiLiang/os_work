//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_osd_framebuffer.h
//	  constant��macro & basic typedef definition of OSD layer's framebuffer
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//		2014.02.21  ZhuoYongHong add ARK1960 OSD framebuffer�ӿ�
//
//****************************************************************************


#ifndef _XM_OSD_FRAMEBUFFER_H_
#define _XM_OSD_FRAMEBUFFER_H_

#include "xm_osd_layer.h"

#if defined (__cplusplus)
	extern "C"{
#endif

#define	OSD_YUV_FRAMEBUFFER_COUNT		3		// YUV��framebuffer�������
//#define	OSD_YUV_FRAMEBUFFER_COUNT		1		// YUV��framebuffer�������

#define	OSD_RGB_FRAMEBUFFER_COUNT		3		// RGB��framebuffer�������(��С��Ҫ3��������)


// framebuffer type���� (YUV/RGB)
enum {
	XM_OSD_FRAMEBUFFER_TYPE_YUV = 0,
	XM_OSD_FRAMEBUFFER_TYPE_RGB,
	XM_OSD_FRAMEBUFFER_TYPE_COUNT
};	

typedef struct tagXM_LCDC_CHANNEL_CONFIG {
	unsigned int		lcdc_channel;				// ͨ����
	unsigned int		lcdc_type;					// ͨ���ӿ����� (RGB, VGA, CVBS, CPU, YPbPr, BT601)
	unsigned int		lcdc_width;					// ͨ�����
	unsigned int		lcdc_height;				// ͨ���߶�
	XM_COLORREF			lcdc_background_moon_color;	// ͨ������ɫ(����)
	XM_COLORREF			lcdc_background_sun_color;		// ͨ������ɫ(����)

	// OSD������طֱ�������
	XMSIZE				lcdc_osd_size[XM_OSD_LAYER_COUNT];
} XM_LCDC_CHANNEL_CONFIG;

#define	MAX_XM_MOVE_STEP		32

// �ƶ�����
typedef struct tagXM_MOVE_CONTROL {
	int				osd_step_count;			// 
	short int		osd_offset_x[MAX_XM_MOVE_STEP];		// X�᷽��OSD��ԭ��λ��(���LCDC��Ƶ���ԭ��, LCDC�����Ͻ�)
	short int		osd_offset_y[MAX_XM_MOVE_STEP];		// Y�᷽��OSD��ԭ��λ��(���LCDC��Ƶ���ԭ��, LCDC�����Ͻ�)
	unsigned char	osd_brightness[MAX_XM_MOVE_STEP];	// �������� (0 ~ 64)
	unsigned char	osd_alpha[MAX_XM_MOVE_STEP];			// �Ӵ�alpha����	
} XM_MOVE_CONTROL;

typedef struct tagXM_OSD_FRAMEBUFFER {
	void *				prev;
	void *				next;
	
	unsigned int		lcd_channel;	// ͨ����(ͨ��0��ͨ��1)
	unsigned int		osd_layer;		// OSD���

	HANDLE				view_handle;	// framebuffer��Ӧ���Ӵ����


	unsigned int		configed;		// ����Ƿ������õ�OSD����㡣
												//	1 �����á��������޸�framebuffer�Ĺ�����(�߶ȡ���ȡ���ʽ����Ϣ)
												// 0 δ����

	unsigned int		width;			// x�����طֱ���
	unsigned int		height;			// y�����طֱ���
	int					offset_x;		// �����LCD��ʾ���ԭ���X��ƫ��
	int					offset_y;		// �����LCD��ʾ���ԭ���Y��ƫ��
	unsigned int		format;			// ֡������Դ���ݸ�ʽ, OSD Layer��ʽ(YUV420, ARGB888, RGB565, ARGB454, )
	unsigned int		stride;			// ֡������Դ�������ֽڳ���
	unsigned char *	address;			// ֡������Դ���ݵ�ַ
												//		��OSD Layer�ĸ�ʽ(format)ΪYUV420, YUV�������
	
	XM_MOVE_CONTROL	move_control;	// �ƶ�����

	char					user_data[2048];	// ˽�и�ʽ����
	unsigned int		frame_ticket;		// ��������Ƶ֡����ʱ��(����)


}XM_OSD_FRAMEBUFFER, * xm_osd_framebuffer_t;



// ����һ���µ�framebufferʵ�� 
// ����framebufferʵ����Դ����ʱ������NULL
// ��ָ��OSD��ĸ�ʽΪYUV420, YUVƽ�������������
xm_osd_framebuffer_t XM_osd_framebuffer_create (
												unsigned int lcd_channel,	// ��Ƶ���ͨ�����
												unsigned int osd_layer,		// OSD�����
												unsigned int layer_format,	// OSD���ʽ
												unsigned int layer_width,	// OSD����(���ص����)
												unsigned int layer_height,	// OSD��߶�(���ص����)
												HANDLE view_handle,			// ��framebuffer�������Ӵ������
																					//		��Ϊ0, �����κ��Ӵ�û�й���
												unsigned int copy_last_view_framebuffer_context,
																					// �Ƿ������һ�ε���ͬ�Ӵ������framebuffer����
																					//		0 ������  1 ����
												unsigned int clear_framebuffer_context
											);


// ��ָ��OSD�����ʹ��(�����һ�δ���)��framebufferʵ����
// ����OSD���framebufferʵ����û�д������򷵻�NULL��
//		����NULL��ʾ��ʧ��(���������framebufferʵ������ʧ�ܵ�ԭ����)
// �򿪲��������ϴ��Ѵ�����framebufferʵ����Ȼ��ֱ��ʹ�����framebufferʵ������GDI��д����(ֱ��д������)
xm_osd_framebuffer_t XM_osd_framebuffer_open (unsigned int lcd_channel,		// ��Ƶ���ͨ�����
															 unsigned int osd_layer,		// OSD�����
															 unsigned int layer_format,	// OSD���ʽ
															 unsigned int layer_width,		// OSD����(���ص����)
														    unsigned int layer_height		// OSD��߶�(���ص����)
															 );	

// ��ȡframebuffer��������ݲ�ָ��
// ����YUV420, �õ� Y / U / V �ܼ�3������ݻ�������ַ
// ����Y_UV420, �õ� Y / UV �ܼ�2������ݻ�������ַ
// ����ARGB888, �õ� ARGB �ܼ�1������ݻ�������ַ
int XM_osd_framebuffer_get_buffer (xm_osd_framebuffer_t framebuffer, unsigned char *data[4]);

// ɾ��һ��framebuffer����/ʵ�����������
int XM_osd_framebuffer_delete (xm_osd_framebuffer_t framebuffer);


// �ر�framebufferʵ����ʹ��, ����framebuffer��ʾ����Ӧ��OSD��
// ����framebufferʵ�������ӣ��򽫶�Ӧ��OSD��رա�(��OSD����ʹ��)
// ����framebufferʵ�����ӣ�  �򽫶�Ӧ��OSD�㿪����(��OSD���ѹر�)
// force_to_config_osd  1  ǿ��ʹ���µĲ�����ʼ��OSD
int XM_osd_framebuffer_close (xm_osd_framebuffer_t framebuffer, int force_to_config_osd);


// �ͷ�ָ��OSD�������framebufferʵ����ͬʱ����OSD���ֹ(����OSD����ʾ�ر�)��
void XM_osd_framebuffer_release (unsigned int lcd_channel, unsigned int osd_layer);

// ���ƻ����������ݣ�Դframebuffer��Ŀ��framebuffer�������ͬ�ĳߴ硢��ʾ��ʽ, �����ƻ�ʧ��
// �ɹ�����0��ʧ�ܷ���-1
int XM_osd_framebuffer_copy (xm_osd_framebuffer_t dst, xm_osd_framebuffer_t src);

// ʹ��ָ������ɫ(ARGB)���֡������
int XM_osd_framebuffer_clear (xm_osd_framebuffer_t framebuffer, 
										unsigned char alpha, 
										unsigned char r, 
										unsigned char g, 
										unsigned char b);


// �ⲿ��Ƶ����ӿ�����
void XM_osd_framebuffer_init (void);
void XM_osd_framebuffer_exit (void);


// ϵͳ������YUV��RGB���͵�framebuffer�������ú���������˫������(pingpong buffer)������
// framebuffer����Ļ�������ַû�����ã�ʹ��ǰ��Ҫ��XM_osd_framebuffer_config�����н������á�
// Ӧ�ÿ��Ի�ȡÿ������(YUV��RGB)��framebuffer�������������ʹ��ǰ����framebuffer����Ļ�������ַ

// framebuffer������û����ó�ʼ������, ��ϵͳ����
void XM_osd_framebuffer_config_init (void);

// framebuffer������û����ý�������, ��ϵͳ����
void XM_osd_framebuffer_config_exit (void);


// ��ȡϵͳ�����OSD��framebuffer�������
// ʧ�� ����0
// �ɹ� ���ض����framebuffer����
int XM_osd_framebuffer_get_framebuffer_count (	unsigned int lcd_channel,			// ��Ƶ���ͨ�����
																unsigned int framebuffer_type		// YUV/RGB����
																);

// ����ϵͳ�����framebuffer����Ļ�������ַ
// ʧ�� ����0
// �ɹ� ����1
int XM_osd_framebuffer_set_framebuffer_address ( unsigned int lcd_channel,			// ��Ƶ���ͨ�����
																 unsigned int framebuffer_type,	// YUV/RGB����
																 unsigned int framebuffer_index,	// framebuffer�������(0��ʼ)
																 unsigned char *framebuffer_base	// ��������ַ
																 );

int XM_osd_framebuffer_set_video_ticket (xm_osd_framebuffer_t framebuffer, unsigned int ticket);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif // _XM_OSD_FRAMEBUFFER_H_
