//****************************************************************************
//
//	Copyright (C) 2011 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_canvas.h
//	  canvas �򵥻���API
//
//	Revision history
//
//		2011.05.27	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _XM_CANVAS_H_
#define _XM_CANVAS_H_

#include <xm_type.h>
#include <hw_osd_layer.h>		// ����format����
#include <xm_image.h>

#if defined (__cplusplus)
	extern "C"{
#endif

#define	XM_CANVAS_ID			0x564E4143		// "CANV"

// XFERMODEת��ģʽ
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
	unsigned int	id;						// ���CANVAS��������

	unsigned int	format;					// ����ͼ���ʽ(ARGB888, RGB565, ARGB454)
													//		��ARGB888��ǰ֧��
	unsigned int	xfermode;				// ת��ģʽ
	unsigned int	stride;					// �������ֽڳ���
	unsigned int	width;					// �������ؿ��
	unsigned int	height;					// �������ظ߶�

	unsigned int	rev[2];

	// ����32�ֽڶ���(256bit)
	unsigned char	image[4];				// ����ƽ�����ݻ�����
} XM_CANVAS;

// ����һ��ָ����С�Ļ�������
XM_CANVAS * XM_CanvasCreate (unsigned int width, unsigned int height);

// ʹ��IMAGE������Ϊ��������һ����������
XM_CANVAS * XM_CanvasCreateFromImage (XM_IMAGE *lpImage);

// ʹ��PNGͼ��������Ϊ��������һ����������
XM_CANVAS * XM_CanvasCreateFromImageData (VOID *lpImageBase, DWORD dwImageSize);

// ʹ��PNGͼ���ļ���Ϊ��������һ����������
XM_CANVAS * XM_CanvasCreateFromImageFile (const char *lpImageFile, DWORD dwFormat);

// ����ͼ�����ʱ��ת��ģʽ
// �ɹ����� 0
// ʧ�ܷ��� -1
int XM_CanvasSetXferMode (XM_CANVAS *lpCanvas, DWORD dwXferMode);

// �ڻ����ϻ���ͼ��
// �ɹ����� 0
// ʧ�ܷ��� -1
int XM_CanvasDrawImage (XM_CANVAS *lpCanvas, XM_IMAGE *lpImage, int x, int y);

// �ͷŻ���������Դ
// �ɹ����� 0
// ʧ�ܷ��� -1
int XM_CanvasDelete (XM_CANVAS *lpCanvas);



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_CANVAS_H_
