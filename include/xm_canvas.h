//****************************************************************************
//
//	Copyright (C) 2011 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_canvas.h
//	  canvas 简单画布API
//
//	Revision history
//
//		2011.05.27	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _XM_CANVAS_H_
#define _XM_CANVAS_H_

#include <xm_type.h>
#include <hw_osd_layer.h>		// 包含format定义
#include <xm_image.h>

#if defined (__cplusplus)
	extern "C"{
#endif

#define	XM_CANVAS_ID			0x564E4143		// "CANV"

// XFERMODE转换模式
enum	{
	XM_CANVAS_XFERMODE_CLEAR = 0,	
	XM_CANVAS_XFERMODE_SRC,
	XM_CANVAS_XFERMODE_DST,
	XM_CANVAS_XFERMODE_SRC_OVER,
	XM_CANVAS_XFERMODE_DST_OVER,
	XM_CANVAS_XFERMODE_SRC_IN,
	XM_CANVAS_XFERMODE_DST_IN,
	XM_CANVAS_XFERMODE_SRC_OUT,
	XM_CANVAS_XFERMODE_DST_OUT,
	XM_CANVAS_XFERMODE_XOR,
	XM_CANVAS_XFERMODE_DARKEN,
	XM_CANVAS_XFERMODE_LIGHTEN,
	XM_CANVAS_XFERMODE_MULTIPLY,
	XM_CANVAS_XFERMODE_SCREEN,
	XM_CANVAS_XFERMODE_COUNT
};


typedef struct tagXM_CANVAS {
	unsigned int	id;						// 标记CANVAS画布对象

	unsigned int	format;					// 画布图像格式(ARGB888, RGB565, ARGB454)
													//		仅ARGB888当前支持
	unsigned int	xfermode;				// 转换模式
	unsigned int	stride;					// 画布行字节长度
	unsigned int	width;					// 画布像素宽度
	unsigned int	height;					// 画布像素高度

	unsigned int	rev[2];

	// 数据32字节对齐(256bit)
	unsigned char	image[4];				// 画布平面数据缓冲区
} XM_CANVAS;

// 创建一个指定大小的画布对象
XM_CANVAS * XM_CanvasCreate (unsigned int width, unsigned int height);

// 使用IMAGE对象作为背景创建一个画布对象
XM_CANVAS * XM_CanvasCreateFromImage (XM_IMAGE *lpImage);

// 使用PNG图像数据作为背景创建一个画布对象
XM_CANVAS * XM_CanvasCreateFromImageData (VOID *lpImageBase, DWORD dwImageSize);

// 使用PNG图像文件作为背景创建一个画布对象
XM_CANVAS * XM_CanvasCreateFromImageFile (const char *lpImageFile, DWORD dwFormat);

// 设置图像绘制时的转换模式
// 成功返回 0
// 失败返回 -1
int XM_CanvasSetXferMode (XM_CANVAS *lpCanvas, DWORD dwXferMode);

// 在画布上绘制图像
// 成功返回 0
// 失败返回 -1
int XM_CanvasDrawImage (XM_CANVAS *lpCanvas, XM_IMAGE *lpImage, int x, int y);

// 释放画布对象资源
// 成功返回 0
// 失败返回 -1
int XM_CanvasDelete (XM_CANVAS *lpCanvas);



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_CANVAS_H_
