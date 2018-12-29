//****************************************************************************
//
//	Copyright (C) 2011 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_image.h
//	  GIF/PNG����
//
//	Revision history
//
//		2011.05.27	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _XM_IMAGE_H_
#define _XM_IMAGE_H_

#include <xm_type.h>
#include "hw_osd_layer.h"		// ����format����
#include "xm_osd_framebuffer.h"

#if defined (__cplusplus)
	extern "C"{
#endif

#define	XM_IMAGE_ID			0x474D4958		// "XIMG"

typedef struct tagXM_IMAGE {
	unsigned int	id;						// ���IMAGE����

	unsigned int	format;					// ͼ���ʽ(ARGB888, RGB565, ARGB454)
													//		��ARGB888��ǰ֧��
	unsigned int	stride;					// ͼ�����ֽڳ���
	unsigned int	width;					// ͼ�����ؿ��
	unsigned int	height;					// ͼ�����ظ߶�

	void *			ayuv;						// AYUV444�������

	unsigned int	rev[2];					// ����32�ֽ�				

	// ����32�ֽڶ���(256bit)
	unsigned int	image[1];				// ͼ��ƽ�����ݻ�����
} XM_IMAGE;

// ��������

#define	XMIMG_DRAW_POS_LEFTTOP		1	// ����ʾ��������Ͻ�λ�ÿ�ʼ��ʾ
#define	XMIMG_DRAW_POS_CENTER		2	// ��ʾ���������ʾ
#define	XMIMG_DRAW_POS_RIGHT		3	// ��ʾ������Ҳ���ʾ
#define	XMIMG_DRAW_POS_PAINT        4   // �������

#define	XMGIF_DRAW_POS_LEFTTOP		XMIMG_DRAW_POS_LEFTTOP	// ����ʾ��������Ͻ�λ�ÿ�ʼ��ʾ
#define	XMGIF_DRAW_POS_CENTER		XMIMG_DRAW_POS_CENTER	// ��ʾ���������ʾ
#define	XMGIF_DRAW_POS_RIGHT		XMIMG_DRAW_POS_RIGHT	// ��ʾ�����Ҳ���ʾ
#define	XMGIF_DRAW_POS_PAINT		XMIMG_DRAW_POS_PAINT	// �����


// ���Ӵ�ָ���������PNGͼ��
// dwFlag��ָ������ķ��
// ����ֵ
// 0     �ɹ�
// < 0   ʧ��
int XM_DrawGifImageEx (VOID *lpImageBase,			// GIFͼ�����ݻ�������ַ
						  DWORD dwImageSize,			// GIFͼ�����ݻ���������
						  HANDLE hWnd,					// �Ӵ����
						  XMRECT *lpRect,				// �Ӵ��еľ�������
						  DWORD dwFlag					// ָ������ķ��
						  );	


// ���Ӵ�ָ���������PNGͼ��
// dwFlag��ָ������ķ��
// ����ֵ
// 0     �ɹ�
// < 0   ʧ��
int XM_DrawPngImageEx (VOID *lpImageBase,		// PNGͼ�����ݻ�������ַ
							  DWORD dwImageSize,		// PNGͼ�����ݻ���������	
							  HANDLE hWnd,				// �Ӵ����
							  XMRECT *lpRect,			// �Ӵ��еľ�������
							  DWORD dwFlag				// ָ������ķ��
							  );

// ���Ӵ�ָ���������PNG/GIFͼ��
// dwFlag��ָ������ķ��
// ����ֵ
// 0     �ɹ�
// < 0   ʧ��
int XM_DrawImageEx (VOID *lpImageBase,			// GIF/PNGͼ�����ݻ�������ַ
						  DWORD dwImageSize,			// GIF/PNGͼ�����ݻ���������
						  HANDLE hWnd,					// �Ӵ����
						  XMRECT *lpRect,				// �Ӵ��еľ�������
						  DWORD dwFlag					// ָ������ķ��
						  );	

// ��ȡGIFͼ��ĳߴ�
// ����ֵ
// 0     �ɹ�
// < 0   ʧ��
int XM_GetGifImageSize (VOID *lpImageBase, DWORD dwImageSize, XMSIZE *lpSize);

// ��ȡPNGͼ��ĳߴ�
// ����ֵ
// 0     �ɹ�
// < 0   ʧ��
int XM_GetPngImageSize (VOID *lpImageBase, DWORD dwImageSize, XMSIZE *lpSize);


// ��ȡPNG/GIFͼ��ĳߴ�
// ����ֵ
// 0     �ɹ�
// < 0   ʧ��
int XM_GetImageSize (VOID *lpImageBase, DWORD dwImageSize, XMSIZE *lpSize);

// ���ڴ�(ͼ���ʽԴ����ΪPNG/GIF��ʽ)����IMAGE����
// ��ǰ�汾��֧��ARGB888��ʽ
XM_IMAGE * XM_ImageCreate (VOID *lpImageBase, DWORD dwImageSize, DWORD dwFormat);

// ���ļ�(ͼ���ʽԴ����ΪPNG/GIF��ʽ)����IMAGE����
// ��ǰ�汾��֧��ARGB888��ʽ
XM_IMAGE * XM_ImageCreateFromFile (const char *lpImageFile, DWORD dwFormat);

// �ͷ�IMAGE������Դ
// �ɹ����� 0
// ʧ�ܷ��� -1
int XM_ImageDelete (XM_IMAGE *lpImage);

// ��IMAGE��ʽ������ת��Ϊָ�������ݸ�ʽ
// ��֧�ִ� ARGB888-->AYUV444
XM_IMAGE * XM_ImageConvert (XM_IMAGE *lpImage, DWORD dwNewFormat);

// ��IMAGE��ʽ��ͼ����С
// uScaleRatio ��С�ı�����
//   0��1    ����
//   2		 �ߴ�Ϊԭ����1/2
//   3		 �ߴ�Ϊԭ����1/3
//   n       �ߴ�Ϊԭ����1/n
XM_IMAGE * XM_ImageScale (XM_IMAGE *lpImage, UINT uScaleRatio);

// ����һ���µ�IMAGE����, ����ΪԭʼIMAGE��������� * �µ��������� fBrightnessRatio
// fBrightnessRatio >= 0.0
XM_IMAGE * XM_ImageCloneWithBrightness (XM_IMAGE *lpImage, float fBrightnessRatio);


// ����һ���ҶȻ�IMAGE����
XM_IMAGE * XM_ImageCloneWithGrayScaleEffect (XM_IMAGE *lpImage);


// ��IMAGEͼ����ʾ���Ӵ�ָ��λ��
// ֧���Ӵ���Alpha���ӻ��
// ��ʱ��֧��ARGB888��ʽ
int XM_ImageDisplay (XM_IMAGE *lpImage, HANDLE hWnd, XMCOORD x, XMCOORD y);


// ����־ˮӡIMAGEͼ����YUV��ʽ����Ƶ����
int XM_BlendFlagWaterMarkImage ( unsigned char *osd_layer_buffer, 
								unsigned int osd_layer_width, unsigned int osd_layer_height,
								unsigned int osd_layer_stride
							 );

// ��IMAGEͼ����ʾ��OSDͼ���ָ��λ��
// �ɹ�����		0
// ʧ�ܷ���		-1
int XM_ImageBlend (
						XM_IMAGE *lpImage,
						unsigned int img_offset_x,					// ����������Դͼ���ƫ��(���Դͼ��ƽ������Ͻ�)
						unsigned int img_offset_y,
						unsigned int img_w,							// ��������������ؿ�ȡ����ظ߶�		
						unsigned int img_h,

						unsigned int lcdc_channel,
						unsigned int osd_layer,
						unsigned int osd_offset_x,					//	����������OSD���ƫ�� (���OSDƽ������Ͻ�)
						unsigned int osd_offset_y
						);

int XM_ImageBlendToFrameBuffer (	
						XM_IMAGE *lpImage,
						unsigned int img_offset_x,					// ����������Դͼ���ƫ��(���Դͼ��ƽ������Ͻ�)
						unsigned int img_offset_y,
						unsigned int img_w,							// ��������������ؿ�ȡ����ظ߶�		
						unsigned int img_h,
						xm_osd_framebuffer_t framebuffer,
						unsigned int osd_offset_x,					//	����������OSD���ƫ�� (���OSDƽ������Ͻ�)
						unsigned int osd_offset_y
						);

// --------------------------
// ROM IMAGE��Դ����
// --------------------------
#include <xm_queue.h>

typedef struct _tagROMIMAGE {
	queue_s	link;					// ����ָ��
	DWORD		dwImageBase;		// ROM��Դƫ�ƣ�����Ψһ��ʶ
	XM_IMAGE *lpImage;			// IMAGE���
} XM_ROMIMAGE;

void XM_RomImageInit (void);
void XM_RomImageExit (void);

// ��һ��ROM��Դָ���IMAGE���
// dwImageBase  ROM��Դ���ֽ�ƫ��
// dwImageSize  ROM��Դ���ֽڴ�С
// ����ֵ
//  == 0   ��ʾʧ��
//  != 0   ��ʾ�ѳɹ��򿪵�ͼ����
XM_IMAGE *XM_RomImageOpen (DWORD dwImageBase, DWORD dwImageSize);

// ��ָ�����ڵ�ָ���������Rom Image
// <  0  ʧ��
// =  0  �ɹ�
int XM_RomImageDraw (	DWORD dwImageBase, DWORD dwImageSize, 
								HANDLE hWnd, 
								XMRECT *lpRect, DWORD dwFlag
							);


// �ͷ�����ROM Image��Դ
void XM_RomImageFreeAll (void);

int XM_ImageBlend_argb888_to_yuv420_normal (
						XM_IMAGE *lpImage,
						unsigned int img_offset_x,					// ����������Դͼ���ƫ��(���Դͼ��ƽ������Ͻ�)
						unsigned int img_offset_y,
						unsigned int img_w,							// ��������������ؿ�ȡ����ظ߶�		
						unsigned int img_h,

						unsigned char *osd_layer_buffer,			// Ŀ��OSD����Ƶ���ݻ�����
						unsigned int osd_layer_format,			// Ŀ��OSD����Ƶ��ʽ
						unsigned int osd_layer_width,				// Ŀ��OSD�����ؿ�ȡ����ظ߶�
						unsigned int osd_layer_height,	
						unsigned int osd_layer_stride,			// Ŀ��OSD�����ֽڳ���
																			//		YUV420��ʽ��ʾY������UV��������2
						unsigned int osd_offset_x,					//	����������OSD���ƫ�� (���OSDƽ������Ͻ�)
						unsigned int osd_offset_y

						);

int XM_ImageBlend_argb888_to_yuv420 (
						XM_IMAGE *lpImage,
						unsigned int img_offset_x,					// ����������Դͼ���ƫ��(���Դͼ��ƽ������Ͻ�)
						unsigned int img_offset_y,
						unsigned int img_w,							// ��������������ؿ�ȡ����ظ߶�		
						unsigned int img_h,

						unsigned char *osd_layer_buffer,			// Ŀ��OSD����Ƶ���ݻ�����
						unsigned int osd_layer_format,			// Ŀ��OSD����Ƶ��ʽ
						unsigned int osd_layer_width,				// Ŀ��OSD�����ؿ�ȡ����ظ߶�
						unsigned int osd_layer_height,	
						unsigned int osd_layer_stride,			// Ŀ��OSD�����ֽڳ���
																			//		YUV420��ʽ��ʾY������UV��������2
						unsigned int osd_offset_x,					//	����������OSD���ƫ�� (���OSDƽ������Ͻ�)
						unsigned int osd_offset_y

						);

void XM_ImageConvert_YUV420_to_ARGB(unsigned char *ycbcr, 
										 unsigned char *rgb, 
										 unsigned int w, 
										 unsigned int h);
void XM_ImageConvert_Y_UV420_to_ARGB(unsigned char *ycbcr, 
										 unsigned char *rgb, 
										 unsigned int w, 
										 unsigned int h);
void XM_ImageConvert_Y_UV420toYUV420(char *Src,char *Dst, int w ,int h);


// ��ȡPNG/GIFͼ��ĳߴ�
// ����ֵ
// 0     �ɹ�
// < 0   ʧ��
int XM_GetRomImageSize (DWORD dwImageBase, DWORD dwImageSize, XMSIZE *lpSize);

// ��ROM��Դ(ͼ���ʽԴ����ΪPNG/GIF��ʽ)����IMAGE����
XM_IMAGE * XM_RomImageCreate (DWORD dwImageBase, DWORD dwImageSize, DWORD dwFormat);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_IMAGE_H_
