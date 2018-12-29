//****************************************************************************
//
//	Copyright (C) 2011 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_image.h
//	  GIF/PNG解码
//
//	Revision history
//
//		2011.05.27	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _XM_IMAGE_H_
#define _XM_IMAGE_H_

#include <xm_type.h>
#include "hw_osd_layer.h"		// 包含format定义
#include "xm_osd_framebuffer.h"

#if defined (__cplusplus)
	extern "C"{
#endif

#define	XM_IMAGE_ID			0x474D4958		// "XIMG"

typedef struct tagXM_IMAGE {
	unsigned int	id;						// 标记IMAGE对象

	unsigned int	format;					// 图像格式(ARGB888, RGB565, ARGB454)
													//		仅ARGB888当前支持
	unsigned int	stride;					// 图像行字节长度
	unsigned int	width;					// 图像像素宽度
	unsigned int	height;					// 图像像素高度

	void *			ayuv;						// AYUV444解码加速

	unsigned int	rev[2];					// 补齐32字节				

	// 数据32字节对齐(256bit)
	unsigned int	image[1];				// 图像平面数据缓冲区
} XM_IMAGE;

// 对齐风格定义

#define	XMIMG_DRAW_POS_LEFTTOP		1	// 从显示区域的左上角位置开始显示
#define	XMIMG_DRAW_POS_CENTER		2	// 显示区域居中显示
#define	XMIMG_DRAW_POS_RIGHT		3	// 显示区域从右侧显示
#define	XMIMG_DRAW_POS_PAINT        4   // 按坐标点

#define	XMGIF_DRAW_POS_LEFTTOP		XMIMG_DRAW_POS_LEFTTOP	// 从显示区域的左上角位置开始显示
#define	XMGIF_DRAW_POS_CENTER		XMIMG_DRAW_POS_CENTER	// 显示区域居中显示
#define	XMGIF_DRAW_POS_RIGHT		XMIMG_DRAW_POS_RIGHT	// 显示区域右侧显示
#define	XMGIF_DRAW_POS_PAINT		XMIMG_DRAW_POS_PAINT	// 坐标点


// 在视窗指定区域绘制PNG图像
// dwFlag可指定对齐的风格
// 返回值
// 0     成功
// < 0   失败
int XM_DrawGifImageEx (VOID *lpImageBase,			// GIF图像数据缓冲区地址
						  DWORD dwImageSize,			// GIF图像数据缓冲区长度
						  HANDLE hWnd,					// 视窗句柄
						  XMRECT *lpRect,				// 视窗中的矩形区域
						  DWORD dwFlag					// 指定对齐的风格
						  );	


// 在视窗指定区域绘制PNG图像
// dwFlag可指定对齐的风格
// 返回值
// 0     成功
// < 0   失败
int XM_DrawPngImageEx (VOID *lpImageBase,		// PNG图像数据缓冲区地址
							  DWORD dwImageSize,		// PNG图像数据缓冲区长度	
							  HANDLE hWnd,				// 视窗句柄
							  XMRECT *lpRect,			// 视窗中的矩形区域
							  DWORD dwFlag				// 指定对齐的风格
							  );

// 在视窗指定区域绘制PNG/GIF图像
// dwFlag可指定对齐的风格
// 返回值
// 0     成功
// < 0   失败
int XM_DrawImageEx (VOID *lpImageBase,			// GIF/PNG图像数据缓冲区地址
						  DWORD dwImageSize,			// GIF/PNG图像数据缓冲区长度
						  HANDLE hWnd,					// 视窗句柄
						  XMRECT *lpRect,				// 视窗中的矩形区域
						  DWORD dwFlag					// 指定对齐的风格
						  );	

// 获取GIF图像的尺寸
// 返回值
// 0     成功
// < 0   失败
int XM_GetGifImageSize (VOID *lpImageBase, DWORD dwImageSize, XMSIZE *lpSize);

// 获取PNG图像的尺寸
// 返回值
// 0     成功
// < 0   失败
int XM_GetPngImageSize (VOID *lpImageBase, DWORD dwImageSize, XMSIZE *lpSize);


// 获取PNG/GIF图像的尺寸
// 返回值
// 0     成功
// < 0   失败
int XM_GetImageSize (VOID *lpImageBase, DWORD dwImageSize, XMSIZE *lpSize);

// 从内存(图像格式源数据为PNG/GIF格式)创建IMAGE对象
// 当前版本仅支持ARGB888格式
XM_IMAGE * XM_ImageCreate (VOID *lpImageBase, DWORD dwImageSize, DWORD dwFormat);

// 从文件(图像格式源数据为PNG/GIF格式)创建IMAGE对象
// 当前版本仅支持ARGB888格式
XM_IMAGE * XM_ImageCreateFromFile (const char *lpImageFile, DWORD dwFormat);

// 释放IMAGE对象资源
// 成功返回 0
// 失败返回 -1
int XM_ImageDelete (XM_IMAGE *lpImage);

// 将IMAGE格式的数据转换为指定的数据格式
// 仅支持从 ARGB888-->AYUV444
XM_IMAGE * XM_ImageConvert (XM_IMAGE *lpImage, DWORD dwNewFormat);

// 将IMAGE格式的图像缩小
// uScaleRatio 缩小的倍数，
//   0、1    不变
//   2		 尺寸为原来的1/2
//   3		 尺寸为原来的1/3
//   n       尺寸为原来的1/n
XM_IMAGE * XM_ImageScale (XM_IMAGE *lpImage, UINT uScaleRatio);

// 创建一个新的IMAGE对象, 亮度为原始IMAGE对象的亮度 * 新的亮度因子 fBrightnessRatio
// fBrightnessRatio >= 0.0
XM_IMAGE * XM_ImageCloneWithBrightness (XM_IMAGE *lpImage, float fBrightnessRatio);


// 创建一个灰度化IMAGE对象
XM_IMAGE * XM_ImageCloneWithGrayScaleEffect (XM_IMAGE *lpImage);


// 将IMAGE图像显示在视窗指定位置
// 支持视窗的Alpha因子混合
// 暂时仅支持ARGB888格式
int XM_ImageDisplay (XM_IMAGE *lpImage, HANDLE hWnd, XMCOORD x, XMCOORD y);


// 将标志水印IMAGE图像与YUV格式的视频层混合
int XM_BlendFlagWaterMarkImage ( unsigned char *osd_layer_buffer, 
								unsigned int osd_layer_width, unsigned int osd_layer_height,
								unsigned int osd_layer_stride
							 );

// 将IMAGE图像显示在OSD图层的指定位置
// 成功返回		0
// 失败返回		-1
int XM_ImageBlend (
						XM_IMAGE *lpImage,
						unsigned int img_offset_x,					// 复制区域在源图像的偏移(相对源图像平面的左上角)
						unsigned int img_offset_y,
						unsigned int img_w,							// 待复制区域的像素宽度、像素高度		
						unsigned int img_h,

						unsigned int lcdc_channel,
						unsigned int osd_layer,
						unsigned int osd_offset_x,					//	复制区域在OSD层的偏移 (相对OSD平面的左上角)
						unsigned int osd_offset_y
						);

int XM_ImageBlendToFrameBuffer (	
						XM_IMAGE *lpImage,
						unsigned int img_offset_x,					// 复制区域在源图像的偏移(相对源图像平面的左上角)
						unsigned int img_offset_y,
						unsigned int img_w,							// 待复制区域的像素宽度、像素高度		
						unsigned int img_h,
						xm_osd_framebuffer_t framebuffer,
						unsigned int osd_offset_x,					//	复制区域在OSD层的偏移 (相对OSD平面的左上角)
						unsigned int osd_offset_y
						);

// --------------------------
// ROM IMAGE资源管理
// --------------------------
#include <xm_queue.h>

typedef struct _tagROMIMAGE {
	queue_s	link;					// 链表指针
	DWORD		dwImageBase;		// ROM资源偏移，用作唯一标识
	XM_IMAGE *lpImage;			// IMAGE句柄
} XM_ROMIMAGE;

void XM_RomImageInit (void);
void XM_RomImageExit (void);

// 打开一个ROM资源指向的IMAGE句柄
// dwImageBase  ROM资源的字节偏移
// dwImageSize  ROM资源的字节大小
// 返回值
//  == 0   表示失败
//  != 0   表示已成功打开的图像句柄
XM_IMAGE *XM_RomImageOpen (DWORD dwImageBase, DWORD dwImageSize);

// 在指定窗口的指定区域绘制Rom Image
// <  0  失败
// =  0  成功
int XM_RomImageDraw (	DWORD dwImageBase, DWORD dwImageSize, 
								HANDLE hWnd, 
								XMRECT *lpRect, DWORD dwFlag
							);


// 释放所有ROM Image资源
void XM_RomImageFreeAll (void);

int XM_ImageBlend_argb888_to_yuv420_normal (
						XM_IMAGE *lpImage,
						unsigned int img_offset_x,					// 复制区域在源图像的偏移(相对源图像平面的左上角)
						unsigned int img_offset_y,
						unsigned int img_w,							// 待复制区域的像素宽度、像素高度		
						unsigned int img_h,

						unsigned char *osd_layer_buffer,			// 目标OSD的视频数据缓冲区
						unsigned int osd_layer_format,			// 目标OSD的视频格式
						unsigned int osd_layer_width,				// 目标OSD的像素宽度、像素高度
						unsigned int osd_layer_height,	
						unsigned int osd_layer_stride,			// 目标OSD的行字节长度
																			//		YUV420格式表示Y分量，UV分量除以2
						unsigned int osd_offset_x,					//	复制区域在OSD层的偏移 (相对OSD平面的左上角)
						unsigned int osd_offset_y

						);

int XM_ImageBlend_argb888_to_yuv420 (
						XM_IMAGE *lpImage,
						unsigned int img_offset_x,					// 复制区域在源图像的偏移(相对源图像平面的左上角)
						unsigned int img_offset_y,
						unsigned int img_w,							// 待复制区域的像素宽度、像素高度		
						unsigned int img_h,

						unsigned char *osd_layer_buffer,			// 目标OSD的视频数据缓冲区
						unsigned int osd_layer_format,			// 目标OSD的视频格式
						unsigned int osd_layer_width,				// 目标OSD的像素宽度、像素高度
						unsigned int osd_layer_height,	
						unsigned int osd_layer_stride,			// 目标OSD的行字节长度
																			//		YUV420格式表示Y分量，UV分量除以2
						unsigned int osd_offset_x,					//	复制区域在OSD层的偏移 (相对OSD平面的左上角)
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


// 获取PNG/GIF图像的尺寸
// 返回值
// 0     成功
// < 0   失败
int XM_GetRomImageSize (DWORD dwImageBase, DWORD dwImageSize, XMSIZE *lpSize);

// 从ROM资源(图像格式源数据为PNG/GIF格式)创建IMAGE对象
XM_IMAGE * XM_RomImageCreate (DWORD dwImageBase, DWORD dwImageSize, DWORD dwFormat);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_IMAGE_H_
