//****************************************************************************
//
//	Copyright (C) 2010~2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_canvas.c
//	  简单画布API函数
//
//	Revision history
//
//		2010.09.02	ZhuoYongHong Initial version
//
//****************************************************************************

#include <string.h>
#include <stdlib.h>
#include <xm_type.h>
#include <lpng\png.h>
#include <zlib\zlib.h>
#include <xm_file.h>
#include <xm_canvas.h>
#include <xm_printf.h>
#include <xm_heap_malloc.h>

// 创建一个指定大小的画布对象
XM_CANVAS * XM_CanvasCreate (unsigned int width, unsigned int height)
{
	unsigned int stride;
	XM_CANVAS *lpCanvas;
	stride = (width + 7) & (~7);
	stride *= 4;
	lpCanvas = (XM_CANVAS *)XM_heap_malloc (2 * (stride * height + sizeof(XM_CANVAS) - 4));
	if(lpCanvas)
	{
		memset (lpCanvas, 0, 2 * (stride * height + sizeof(XM_CANVAS) - 4));
		lpCanvas->id = XM_CANVAS_ID;
		lpCanvas->format = XM_OSD_LAYER_FORMAT_ARGB888;
		lpCanvas->width = width;
		lpCanvas->stride = stride;
		lpCanvas->height = height;
		lpCanvas->xfermode = XM_CANVAS_XFERMODE_SRC;
	}
	else
	{
		XM_printf ("XM_CanvasCreate NG, width=%d, height=%d\n", width, height);
	}
	return lpCanvas;
}

// 使用IMAGE对象作为背景创建一个画布对象
XM_CANVAS * XM_CanvasCreateFromImage (XM_IMAGE *lpImage)
{
	unsigned int stride;
	XM_CANVAS *lpCanvas;
	if(lpImage == NULL || lpImage->format != XM_OSD_LAYER_FORMAT_ARGB888)
		return NULL;
	stride = lpImage->stride;
	lpCanvas = (XM_CANVAS *)XM_heap_malloc (2 * (stride * lpImage->height) + sizeof(XM_CANVAS) - 4);
	if(lpCanvas)
	{
		lpCanvas->id = XM_CANVAS_ID;
		lpCanvas->format = XM_OSD_LAYER_FORMAT_ARGB888;
		lpCanvas->width = lpImage->width;
		lpCanvas->stride = lpImage->stride;
		lpCanvas->height = lpImage->height;
		lpCanvas->xfermode = XM_CANVAS_XFERMODE_SRC;
		memcpy (lpCanvas->image, lpImage->image, stride * lpImage->height);	
		memcpy (lpCanvas->image + stride * lpImage->height, lpImage->image, stride * lpImage->height);
	}
	else
	{
		XM_printf ("XM_CanvasCreateFromImage NG, width=%d, height=%d\n", lpImage->width, lpImage->height);
	}
	return lpCanvas;
}

// 使用PNG图像数据作为背景创建一个画布对象
XM_CANVAS * XM_CanvasCreateFromImageData (VOID *lpImageBase, DWORD dwImageSize)
{
	XM_CANVAS *lpCanvas = NULL;
	unsigned int stride;
	if(lpImageBase == NULL || dwImageSize == 0)
		return NULL;
	
	if(	((char *)lpImageBase)[0] == 'G' 
		&& ((char *)lpImageBase)[1] == 'I' 
		&& ((char *)lpImageBase)[2] == 'F')
	{
		// 当前不支持
		return NULL;
	}
	else if( ((char *)lpImageBase)[1] == 'P' 
			&& ((char *)lpImageBase)[2] == 'N' 
			&& ((char *)lpImageBase)[3] == 'G')
	{
		png_image image; /* The control structure used by libpng */
		memset(&image, 0, (sizeof image));
		image.version = PNG_IMAGE_VERSION;
		// 读取PNG图像的格式信息
		if (png_image_begin_read_from_memory(&image, lpImageBase, dwImageSize) == 0)
			return NULL;
		
		// 分配内存
		stride = (image.width + 7) & (~7);  // 总线及cache优化
		stride *= 4;
		lpCanvas = XM_heap_malloc (2 * (stride * image.height + sizeof(XM_CANVAS) - 4));
		if(lpCanvas)
		{
			image.format = PNG_FORMAT_BGRA;
			if( png_image_finish_read (&image, NULL, lpCanvas->image, stride, NULL) == 1)
			{
				lpCanvas->id = XM_CANVAS_ID;
				lpCanvas->format = XM_OSD_LAYER_FORMAT_ARGB888;
				lpCanvas->stride = stride;
				lpCanvas->width = image.width;
				lpCanvas->height = image.height;
				lpCanvas->xfermode = XM_CANVAS_XFERMODE_SRC;
				memset (lpCanvas->rev, 0, sizeof(lpCanvas->rev));

				memcpy (lpCanvas->image + lpCanvas->stride * lpCanvas->height, 
						lpCanvas->image, lpCanvas->stride * lpCanvas->height);
			}
			else
			{
				XM_heap_free (lpCanvas);
				lpCanvas = NULL;
			}
		}
		png_image_free (&image);
	}

	return lpCanvas;
}

// 使用PNG图像文件作为背景创建一个画布对象
XM_CANVAS * XM_CanvasCreateFromImageFile (const char *lpImageFile, DWORD dwFormat)
{
	unsigned int file_size;
	char *png_image = NULL;
	XM_CANVAS *lpCanvas = 0;
	void *fp = XM_fopen (lpImageFile, "rb");
	int stop = 1;
	do
	{
		if(fp == NULL)
			break;
		
		file_size = XM_filesize (fp);
		if(file_size == 0)
			break;
		
		png_image = XM_heap_malloc (file_size);
		if(png_image == NULL)
			break;

		if(XM_fread (png_image, 1, file_size, fp) != file_size)
			break;

		lpCanvas = XM_CanvasCreateFromImageData (png_image, file_size);
	} while (!stop);

	if(fp)
		XM_fclose (fp);
	if(png_image)
		XM_heap_free (png_image);

	return lpCanvas;
}

// 设置图像绘制时的转换模式
// 成功返回 0
// 失败返回 -1
int XM_CanvasSetXferMode (XM_CANVAS *lpCanvas, DWORD dwXferMode)
{
	if(lpCanvas == 0 || lpCanvas->id != XM_CANVAS_ID)
		return -1;
	if(dwXferMode >= XM_CANVAS_XFERMODE_COUNT)
		return -1;
	lpCanvas->xfermode = dwXferMode;
	return 0;
}

static void canvas_clear (XM_CANVAS *lpCanvas)
{
	memset (lpCanvas->image, lpCanvas->stride * lpCanvas->height, 0);
}

// /** [Sa, Sc] */ 
// 将SRC图像完全覆盖在画布上
static void canvas_src (unsigned char *src, int src_x, int src_y, int src_stride,
								unsigned char *dst, int dst_x, int dst_y,	int dst_stride,
								int w, int h)
{
	int i, j;
	unsigned char *s, *d;
	for (j = 0; j < h; j ++)
	{
		s = src;
		d = dst;
		for (i = 0; i < w; i ++)
		{
			*(unsigned int *)d = *(unsigned int *)s;
			s += 4;
			d += 4;
		}
		src += src_stride;
		dst += dst_stride;
	}
}

// SRC_OVER 
// 将SRC图像的非透明区域(Alpha不为0)覆盖在画布上
static void canvas_src_over (unsigned char *src, int src_x, int src_y, int src_stride,
								unsigned char *dst, int dst_x, int dst_y,	int dst_stride,
								int w, int h)
{
	int i, j;
	unsigned char *s, *d;
	//XM_printf ("\n\ncanvas_src_over\n\n");
	for (j = 0; j < h; j ++)
	{
		s = src;
		d = dst;
		for (i = 0; i < w; i ++)
		{
			unsigned int s_a = *(s + 3);
			unsigned int s_r, s_g, s_b;
			unsigned int d_a;
			unsigned int d_r, d_g, d_b;

			if(s_a == 0xFF)
			{
				*(unsigned int *)d = *(unsigned int *)s;
			}
			else if(s_a)
			{
				s_r = *(s + 2);
				s_g = *(s + 1);
				s_b = *(s + 0);
				d_a = *(d + 3);
				d_r = *(d + 2);
				d_g = *(d + 1);
				d_b = *(d + 0);

				d_a = (d_a * (255 - s_a) + s_a * s_a) >> 8;
				d_r = (d_r * (255 - s_a) + s_r * s_a) >> 8;
				d_g = (d_g * (255 - s_a) + s_g * s_a) >> 8;
				d_b = (d_b * (255 - s_a) + s_b * s_a) >> 8;
				//d_a = (d_a * (255 - s_a) >> 8) + s_a;
				//d_r = (d_r * (255 - s_a) >> 8) + s_r;
				//d_g = (d_g * (255 - s_a) >> 8) + s_g;
				//d_b = (d_b * (255 - s_a) >> 8) + s_b;
			//	if(d_a > 255)
			//		d_a = 255;
			//	if(d_r > 255)
			//		d_r = 255;
			//	if(d_g > 255)
			//		d_g = 255;
			//	if(d_b > 255)
			//		d_b = 255;
				*(unsigned int *)d = (unsigned int)((d_r << 16) | (d_g << 8) | d_b | (d_a << 24));

				//*(unsigned int *)d = *(unsigned int *)s;

			}
			s += 4;
			d += 4;
		}
		src += src_stride;
		dst += dst_stride;
	}

	//XM_printf ("\n\ncanvas_src_over END\n\n");
}

// DST_OVER 
// 将SRC图像的非透明区域(Alpha不为0)覆盖在画布的未填充(Alpha为0)区域
static void canvas_dst_over (unsigned char *src, int src_x, int src_y, int src_stride,
								unsigned char *dst, int dst_x, int dst_y,	int dst_stride,
								int w, int h)
{
	int i, j;
	unsigned char *s, *d;
	for (j = 0; j < h; j ++)
	{
		s = src;
		d = dst;
		for (i = 0; i < w; i ++)
		{
			if(*d == 0 && *s)
			{
				// 未覆盖区域
				*(unsigned int *)d = *(unsigned int *)s;
			}
			s += 4;
			d += 4;
		}
		src += src_stride;
		dst += dst_stride;
	}
}

// SRC_IN 
// 将SRC图像与DST图像的交集部分显示在画布上，将画布其他区域清空
static void canvas_src_in (
								unsigned char *src, 
								int src_x, int src_y, int src_stride,
								int src_w, int src_h,
								unsigned char *dst, int dst_stride,
								int dst_w, int dst_h)
{
	int i, j;
	unsigned char *s, *d;
	for (j = 0; j < dst_h; j ++)
	{
		d = dst;
		for (i = 0; i < dst_w; i ++)
		{
			if(*(d+3))
			{
				int off_x;
				int off_y = j - src_y;
				if(off_y < 0 || off_y >= src_h)
				{
					*(unsigned int *)d = 0;
				}
				else
				{
					off_x = i - src_x;
					if(off_x < 0 || off_x >= src_w)
					{
						*(unsigned int *)d = 0;
					}
					else
					{
						s = src + src_stride * off_y + (off_x << 2);
						if(*(s+3))
						{
							// 源与目标点存在交集
							// [Sa * Da, Sc * Da]
							unsigned int d_a = *(d + 3);
							unsigned int s_a;
							unsigned int s_r, s_g, s_b;
							s_a = (*(s + 3) * d_a) >> 8;
							s_r = (*(s + 2) * d_a) >> 8;
							s_g = (*(s + 1) * d_a) >> 8;
							s_b = (*(s + 0) * d_a) >> 8;

							*(unsigned int *)d = (unsigned int)((s_a << 24) | (s_r << 16) | (s_g << 8) | s_b);
						}
						else
						{
							*(unsigned int *)d = 0;
						}
					}
				}
			}
			else
			{
				// 清除
				*(unsigned int *)d = 0;
			}
			d += 4;
		}
		dst += dst_stride;
	}
}

// DST_IN 
// 将DST图像与SRC图像的交集部分显示在画布上，将画布其他区域清空
static void canvas_dst_in (
								unsigned char *src, 
								int src_x, int src_y, int src_stride,
								int src_w, int src_h,
								unsigned char *dst, int dst_stride,
								int dst_w, int dst_h)
{
	int i, j;
	unsigned char *s, *d;
	for (j = 0; j < dst_h; j ++)
	{
		d = dst;
		for (i = 0; i < dst_w; i ++)
		{
			if(*d)
			{
				int off_x;
				int off_y = j - src_y;
				if(off_y < 0 || off_y >= src_h)
				{
					*(unsigned int *)d = 0;
				}
				else
				{
					off_x = i - src_x;
					if(off_x < 0 || off_x >= src_w)
					{
						*(unsigned int *)d = 0;
					}
					else
					{
						s = src + src_stride * off_y + (off_x << 2);
						if(*s)
						{
							// 源与目标点存在交集
							// 保留目标点信息
						}
						else
						{
							*(unsigned int *)d = 0;
						}
					}
				}
				
			}
			else
			{
				// 清除
				*(unsigned int *)d = 0;
			}
			d += 4;
		}
		dst += dst_stride;
	}
}


// /** [Sa * Sa + (1 - Sa)*Da, Rc = Sc * Sa + (1 - Sa)*Dc] */  
// SCREEN 
static void canvas_screen (unsigned char *src, int src_x, int src_y, int src_stride,
								unsigned char *dst, int dst_x, int dst_y,	int dst_stride,
								int w, int h)
{
	int i, j;
	unsigned char *s, *d;
	for (j = 0; j < h; j ++)
	{
		s = src;
		d = dst;
		for (i = 0; i < w; i ++)
		{
			if(*s == 255)
			{
				*(unsigned int *)d = *(unsigned int *)s;
			}
			else if(*s)
			{
				unsigned int s_a = *(s + 0);
				unsigned int s_r = *(s + 1);
				unsigned int s_g = *(s + 2);
				unsigned int s_b = *(s + 3);
				unsigned int d_a = *(d + 0);
				unsigned int d_r = *(d + 1);
				unsigned int d_g = *(d + 2);
				unsigned int d_b = *(d + 3);
				d_a = (s_a * s_a + d_a * (255 - s_a)) >> 8;
				d_r = (s_r * s_a + d_r * (255 - s_a)) >> 8;
				d_g = (s_g * s_a + d_g * (255 - s_a)) >> 8;
				d_b = (s_b * s_a + d_b * (255 - s_a)) >> 8;
				*(unsigned int *)d = (unsigned int)((d_a << 24) | (d_r << 16) | (d_g << 8) | d_b );
			}
			s += 4;
			d += 4;
		}
		src += src_stride;
		dst += dst_stride;
	}
}


// 在画布上绘制图像
// 成功返回 0
// 失败返回 -1
int XM_CanvasDrawImage (XM_CANVAS *lpCanvas, XM_IMAGE *lpImage, int x, int y)
{
	int w, h;
	//int i, j;
	int src_x, src_y;		// 偏移
	int dst_x, dst_y;
	unsigned char *src, *dst;
	//unsigned char *s, *d;
	if(lpCanvas == NULL || lpCanvas->id != XM_CANVAS_ID)
		return -1;

	if(lpCanvas->xfermode == XM_CANVAS_XFERMODE_CLEAR)
	{
		canvas_clear (lpCanvas);
		return 0;
	}
	else if(lpCanvas->xfermode == XM_CANVAS_XFERMODE_DST)
	{
		memcpy (lpCanvas->image, ((unsigned char *)lpCanvas->image) + lpCanvas->stride * lpCanvas->height, 
				lpCanvas->stride * lpCanvas->height);
		return 0;
	}

	if(lpImage == NULL || lpImage->id != XM_IMAGE_ID)
		return -1;
	if(x >= (int)lpCanvas->width)
		return -1;
	if( ((int)(x + lpImage->width)) < 0)
		return -1;
	if(y >= (int)lpCanvas->height)
		return -1;
	if( ((int)(y + lpImage->height)) < 0)
		return -1;
	w = lpCanvas->width;
	h = lpCanvas->height;

	if(x < 0)
	{
		src_x = -x;
		dst_x = 0;
	}
	else
	{
		src_x = 0;
		dst_x = x;
	}
	w = lpImage->width - src_x;
	if(w <= 0)
		return -1;
	if((lpCanvas->width - dst_x) <= 0)
		return -1;
	if(w > (int)(lpCanvas->width - dst_x))
		w = (lpCanvas->width - dst_x);

	if(y < 0)
	{
		src_y = -y;
		dst_y = 0;
	}
	else
	{
		src_y = 0;
		dst_y = y;
	}
	h = lpImage->height - src_y;
	if(h <= 0)
		return -1;
	if((lpCanvas->height - dst_y) <= 0)
		return -1;
	if(h > (int)(lpCanvas->height - dst_y))
		h = lpCanvas->height - dst_y;

	// 新画, 前景
	src = ((unsigned char *)lpImage->image) + src_y * lpImage->stride + (src_x << 2);
	// 原图，背景
	dst = ((unsigned char *)lpCanvas->image) + dst_y * lpCanvas->stride + (dst_x << 2);
	switch (lpCanvas->xfermode)
	{
		case XM_CANVAS_XFERMODE_SRC:
			canvas_clear (lpCanvas);
			canvas_src (src, src_x, src_y, lpImage->stride,
							dst, dst_x, dst_y, lpCanvas->stride,
							w, h);
			break;

		case XM_CANVAS_XFERMODE_SRC_OVER:
			canvas_src_over (src, src_x, src_y, lpImage->stride,
							dst, dst_x, dst_y, lpCanvas->stride,
							w, h);
			break;

		case XM_CANVAS_XFERMODE_DST_OVER:
			canvas_dst_over (src, src_x, src_y, lpImage->stride,
							dst, dst_x, dst_y, lpCanvas->stride,
							w, h);
			break;

		case XM_CANVAS_XFERMODE_SRC_IN:
			canvas_src_in ((unsigned char *)lpImage->image, x, y, lpImage->stride,
							lpImage->width, lpImage->height,
							lpCanvas->image, lpCanvas->stride, lpCanvas->width, lpCanvas->height);
			break;

		case XM_CANVAS_XFERMODE_DST_IN:
			canvas_dst_in (src, src_x, src_y, lpImage->stride,
							lpImage->width, lpImage->height,
							dst, lpCanvas->stride, lpCanvas->width, lpCanvas->height);
			break;


		case XM_CANVAS_XFERMODE_SCREEN:
			canvas_screen (src, src_x, src_y, lpImage->stride,
							dst, dst_x, dst_y, lpCanvas->stride,
							w, h);
			break;	
	}

	return 0;
}

// 释放画布对象资源
// 成功返回 0
// 失败返回 -1
int XM_CanvasDelete (XM_CANVAS *lpCanvas)
{
	if(lpCanvas == NULL || lpCanvas->id != XM_CANVAS_ID)
		return -1;

	lpCanvas->id = 0;
	XM_heap_free (lpCanvas);
	return 0;
}



