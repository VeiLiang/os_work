//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_gdi_image.c
//	  gdi函数
//
//	Revision history
//
//		2010.09.02	ZhuoYongHong Initial version
//
//****************************************************************************
#include <string.h>
#include <stdlib.h>
#include <xm_type.h>
#include <xm_heap_malloc.h>
#include <lpng\png.h>
#include <zlib\zlib.h>
#include <xm_file.h>
#include <xm_image.h>
#include <xm_gif.h>
#include <xm_rom.h>
#include "xm_osd_layer.h"
#include "xm_osd_framebuffer.h"
#include "xm_user.h"
#include "xm_printf.h"

// 从内存(图像格式源数据为PNG/GIF格式)创建IMAGE对象
XM_IMAGE * XM_ImageCreate (VOID *lpImageBase, DWORD dwImageSize, DWORD dwFormat)
{
	XM_IMAGE *lpImage = NULL;
	unsigned int stride;
	if(lpImageBase == NULL || dwImageSize == 0)
	{
		XM_printf ("XM_ImageCreate failed, illegal param\n");
		return NULL;
	}
	if(dwFormat != XM_OSD_LAYER_FORMAT_ARGB888)
	{
		XM_printf ("XM_ImageCreate failed, illegal format\n");
		return NULL;
	}
	
	if(	((char *)lpImageBase)[0] == 'G' 
		&& ((char *)lpImageBase)[1] == 'I' 
		&& ((char *)lpImageBase)[2] == 'F')
	{
		XMSIZE	size;
		// 获取GIF图像的尺寸信息
		if(XM_GetGifImageSize (lpImageBase, dwImageSize, &size) < 0)
			return NULL;
		// 分配内存
		stride = (size.cx + 7) & (~7);  // 总线及cache优化
		stride *= 4;
		lpImage = XM_heap_malloc (stride * size.cy + sizeof(XM_IMAGE) - 4);
		if(lpImage)
		{
			int ret;
			memset (lpImage->image, 0, stride * size.cy);
			ret = XM_lcdc_load_gif_image ((unsigned char *)lpImage->image,
					dwFormat,
					size.cx,
					size.cy,
					stride,
					0,
					0,
					lpImageBase,
					dwImageSize,
					0xFFFFFFFF,		// 透明色
					0xFF);	
			if(ret == IMG_OPER_SUCCESS)
			{
				lpImage->id = XM_IMAGE_ID;
				lpImage->format = XM_OSD_LAYER_FORMAT_ARGB888;
				lpImage->stride = (unsigned short)stride;
				lpImage->width = (unsigned short)size.cx;
				lpImage->height = (unsigned short)size.cy;
				lpImage->ayuv = NULL;
			}
			else
			{
				XM_heap_free (lpImage);
				lpImage = NULL;
			}
		}
		return lpImage;
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
		{
			XM_printf ("XM_ImageCreate failed, image info read failed\n");
			return NULL;
		}
		
		// 分配内存
		stride = (image.width + 7) & (~7);  // 总线及cache优化
		stride *= 4;
		lpImage = XM_heap_malloc (stride * image.height + sizeof(XM_IMAGE) - 4);
		if(lpImage)
		{
			image.format = PNG_FORMAT_BGRA;
			if( png_image_finish_read (&image, NULL, lpImage->image, stride, NULL) == 1)
			{
				lpImage->id = XM_IMAGE_ID;
				lpImage->format = XM_OSD_LAYER_FORMAT_ARGB888;
				lpImage->stride = (unsigned short)stride;
				lpImage->width = (unsigned short)image.width;
				lpImage->height = (unsigned short)image.height;
				lpImage->ayuv = NULL;
			}
			else
			{
				XM_heap_free (lpImage);
				lpImage = NULL;
			}
		}
		else
		{
			XM_printf ("XM_ImageCreate failed, heap busy\n");			
		}
		png_image_free (&image);
	}

	return lpImage;
}

// 从文件(图像格式源数据为PNG/GIF格式)创建IMAGE对象
XM_IMAGE * XM_ImageCreateFromFile (const char *lpImageFile, DWORD dwFormat)
{
	unsigned int file_size;
	char *png_image = NULL;
	XM_IMAGE *lpImage = 0;
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

		lpImage = XM_ImageCreate (png_image, file_size, dwFormat);
	} while (!stop);

	if(fp)
		XM_fclose (fp);
	if(png_image)
		XM_heap_free (png_image);

	return lpImage;
}

// 释放IMAGE对象资源
int XM_ImageDelete (XM_IMAGE *lpImage)
{
	if(lpImage == NULL || lpImage->id != XM_IMAGE_ID)
		return -1;

	if(lpImage->ayuv)
	{
		XM_ImageDelete ((XM_IMAGE *)lpImage->ayuv);
		lpImage->ayuv = NULL;
	}

	lpImage->id = 0;
	XM_heap_free (lpImage);

	return 0;
}

// 将IMAGE格式的数据转换为指定的数据格式
// 仅支持从 ARGB888-->AYUV444
XM_IMAGE * XM_ImageConvert (XM_IMAGE *lpImage, DWORD dwNewFormat)
{
	XM_IMAGE *lpNewImage;
	unsigned int stride;
	if(lpImage == NULL || lpImage->id != XM_IMAGE_ID)
	{
		XM_printf ("illegal image\n");
		return NULL;
	}
	if(lpImage->format != XM_OSD_LAYER_FORMAT_ARGB888)
	{
		XM_printf ("illegal image format\n");
		return NULL;
	}
	if( (lpImage->width & 1) || (lpImage->height & 1) )
	{
		//XM_printf ("XM_ImageConvert failed, illegal width(%d) or height(%d)\n", lpImage->width, lpImage->height);
		//return NULL;
	}
	
	if(dwNewFormat != XM_OSD_LAYER_FORMAT_AYUV444)
	{
		XM_printf ("illegal image yuv format\n");
		return NULL;
	}
	stride = lpImage->stride >> 2;
	stride = (stride + 7) & (~7);		// 16字节对齐, 总线宽度(128bit)对齐
	lpNewImage = XM_heap_malloc (sizeof(XM_IMAGE) - 4 + stride * lpImage->height * 4);
	if(lpNewImage)
	{
		lpNewImage->id = XM_IMAGE_ID;
		lpNewImage->format = XM_OSD_LAYER_FORMAT_AYUV444;
		lpNewImage->stride = (unsigned short)stride;
		lpNewImage->width = (unsigned short)(lpImage->width & ~1);
		lpNewImage->height = (unsigned short)(lpImage->height & ~1);
		lpNewImage->ayuv = NULL;

#if AYUV444_PACKED
		// 打包保存
		// AYUVAYUVAYUV
		XM_lcdc_convert_argb888_to_ayuv444_packed (
			(unsigned char *)lpImage->image,
			lpImage->stride,
			lpImage->width,
			lpImage->height,
			((unsigned char *)lpNewImage->image),
			lpNewImage->stride,
			lpNewImage->width,
			lpNewImage->height);			
#else
		// 分块保存
		// YYY...
		// UUU...
		// VVV...
		// AAA...
		XM_lcdc_convert_argb888_to_ayuv444 (
			(unsigned char *)lpImage->image,
			lpImage->stride,
			lpImage->width,
			lpImage->height,
			((unsigned char *)lpNewImage->image),
			((unsigned char *)lpNewImage->image) + lpNewImage->stride * lpNewImage->height,
			((unsigned char *)lpNewImage->image) + lpNewImage->stride * lpNewImage->height * 2,
			((unsigned char *)lpNewImage->image) + lpNewImage->stride * lpNewImage->height * 3,
			lpNewImage->stride,
			lpNewImage->width,
			lpNewImage->height);
#endif
	}
	else
	{
		XM_printf ("XM_ImageConvert failed, memory busy\n"); 
	}

	return lpNewImage;
}


void ResizeImage( unsigned char *InImage, unsigned char *OutImage, unsigned int SWidth,    
                 unsigned int SHeight, unsigned int SStride, unsigned int DWidth, unsigned int DHeight, unsigned int DStride)     
{   
	unsigned int PtAA = 0, PtBA = 0, PtCA = 0, PtDA = 0, PixelValueA = 0; 
	unsigned int PtAR = 0, PtBR = 0, PtCR = 0, PtDR = 0, PixelValueR = 0;     
	unsigned int PtAG = 0, PtBG = 0, PtCG = 0, PtDG = 0, PixelValueG = 0;     
	unsigned int PtAB = 0, PtBB = 0, PtCB = 0, PtDB = 0, PixelValueB = 0;     
	register unsigned SpixelColNum = 0, SpixelRowNum = 0, DestCol = 0, DestRow = 0;   
	unsigned int SpixelColAddr = 0, SpixelRowAddr = 0;     
	unsigned int ColDelta = 0, RowDelta = 0, scaleV = 0, scaleH = 0;     
	unsigned int ContribAandBA = 0, ContribCandDA = 0;     
	unsigned int ContribAandBR = 0, ContribCandDR = 0;     
	unsigned int ContribAandBG = 0, ContribCandDG = 0;     
	unsigned int ContribAandBB = 0, ContribCandDB = 0;     
	unsigned int ContribTem[4096 * 4];// max size of image is 4096   
	int i = 0;     
   
	scaleH = (SWidth << 8) / DWidth;     
	scaleV = (SHeight << 8) / DHeight;     
	// first line   
	for (DestCol = 0; DestCol < DWidth; DestCol++)   
	{   
		SpixelColAddr = DestCol * scaleH;   
		ColDelta = SpixelColAddr & 255;   
		SpixelColNum = (SpixelColAddr - ColDelta) >> 8;   
		PtAA = InImage[4 * SpixelRowNum * SStride + 4 * SpixelColNum]; 
		PtAR = InImage[4 * SpixelRowNum * SStride + 4 * SpixelColNum + 1];   
		PtAG = InImage[4 * SpixelRowNum * SStride + 4 * SpixelColNum + 2];   
		PtAB = InImage[4 * SpixelRowNum * SStride + 4 * SpixelColNum + 3];   
		
		if ((SpixelColNum + 1) < SWidth)     
		{   
			PtBA = InImage[4 * SpixelRowNum * SStride + 4 * (SpixelColNum + 1)];  
			PtBR = InImage[4 * SpixelRowNum * SStride + 4 * (SpixelColNum + 1) + 1];   
			PtBG = InImage[4 * SpixelRowNum * SStride + 4 * (SpixelColNum + 1) + 2];   
			PtBB = InImage[4 * SpixelRowNum * SStride + 4 * (SpixelColNum + 1) + 3];   
			
			PtCA = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * SpixelColNum];
			PtCR = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * SpixelColNum + 1];   
			PtCG = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * SpixelColNum + 2];   
			PtCB = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * SpixelColNum + 3];   
			
			PtDA = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * (SpixelColNum + 1)];
			PtDR = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * (SpixelColNum + 1) + 1];   
			PtDG = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * (SpixelColNum + 1) + 2];   
			PtDB = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * (SpixelColNum + 1) + 3];   
		}   
		else   
		{   
			PtBA = PtCA = PtDA = PtAA;
			PtBR = PtCR = PtDR = PtAR;   
			PtBG = PtCG = PtDG = PtAG;   
			PtBB = PtCB = PtDB = PtAB;   
		}   
		
		ContribAandBA = ColDelta * (PtBA - PtAA) + (PtAA << 8);   
		ContribCandDA = ColDelta * (PtDA - PtCA) + (PtCA << 8);   
		ContribTem[i++] = ContribCandDA;     
		PixelValueA = ((ContribAandBA << 8) + (ContribCandDA - ContribAandBA) * RowDelta) >> 16;   

		ContribAandBR = ColDelta * (PtBR - PtAR) + (PtAR << 8);   
		ContribCandDR = ColDelta * (PtDR - PtCR) + (PtCR << 8);   
		ContribTem[i++] = ContribCandDR;     
		PixelValueR = ((ContribAandBR << 8) + (ContribCandDR - ContribAandBR) * RowDelta) >> 16;   
		
		ContribAandBG = ColDelta * (PtBG - PtAG) + (PtAG << 8);   
		ContribCandDG = ColDelta * (PtDG - PtCG) + (PtCG << 8);   
		ContribTem[i++] = ContribCandDG;   
		PixelValueG = ((ContribAandBG << 8) + (ContribCandDG - ContribAandBG) * RowDelta) >> 16;   
		
		ContribAandBB = ColDelta * (PtBB - PtAB) + (PtAB << 8);   
		ContribCandDB = ColDelta * (PtDB - PtCB) + (PtCB << 8);   
		ContribTem[i++] = ContribCandDB;   
		PixelValueB = ((ContribAandBB << 8) + (ContribCandDB - ContribAandBB) * RowDelta) >> 16;   

		OutImage[4 * DestRow * DStride + 4 * DestCol + 0] = (unsigned char)PixelValueA;
		OutImage[4 * DestRow * DStride + 4 * DestCol + 1] = (unsigned char)PixelValueR;     
		OutImage[4 * DestRow * DStride + 4 * DestCol + 2] = (unsigned char)PixelValueG;     
		OutImage[4 * DestRow * DStride + 4 * DestCol + 3] = (unsigned char)PixelValueB;     
	}   
	// other line   
	for (DestRow = 1; DestRow < DHeight; DestRow++)     
	{   
		i = 0;   
		SpixelRowAddr = DestRow * scaleV;     
		RowDelta = SpixelRowAddr & 255;     
		SpixelRowNum = (SpixelRowAddr - RowDelta) >> 8;                    
		
		for (DestCol = 0; DestCol < DWidth; DestCol++)     
		{     
			SpixelColAddr = DestCol * scaleH;     
			ColDelta = SpixelColAddr & 255;     
			SpixelColNum = (SpixelColAddr - ColDelta) >> 8;     
			PtAA = InImage[4 * SpixelRowNum * SStride + 4 * SpixelColNum];     
			PtAR = InImage[4 * SpixelRowNum * SStride + 4 * SpixelColNum + 1];     
			PtAG = InImage[4 * SpixelRowNum * SStride + 4 * SpixelColNum + 2];   
			PtAB = InImage[4 * SpixelRowNum * SStride + 4 * SpixelColNum + 3];     
			
			if (((SpixelColNum + 1) < SWidth) && ((SpixelRowNum + 1) < SHeight))   
			{   
				PtCA = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * SpixelColNum];     
				PtCR = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * SpixelColNum + 1];     
				PtCG = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * SpixelColNum + 2];     
				PtCB = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * SpixelColNum + 3];     
				
				PtDA = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * (SpixelColNum + 1)]; 
				PtDR = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * (SpixelColNum + 1) + 1];     
				PtDG = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * (SpixelColNum + 1) + 2];     
				PtDB = InImage[4 * (SpixelRowNum + 1) * SStride + 4 * (SpixelColNum + 1) + 3];     
			}   
			else     
			{   
				PtBA = PtCA = PtDA = PtAA;   
				PtBR = PtCR = PtDR = PtAR;   
				PtBG = PtCG = PtDG = PtAG;   
				PtBB = PtCB = PtDB = PtAB;   
			}     

			ContribAandBA = ContribTem[i];   
			ContribCandDA = ColDelta * (PtDA - PtCA) + (PtCA << 8);   
			ContribTem[i++] = ContribCandDA;   
			PixelValueA = ((ContribAandBA << 8) + (ContribCandDA - ContribAandBA) * RowDelta) >> 16;     
			
			ContribAandBR = ContribTem[i];     
			ContribCandDR = ColDelta * (PtDR - PtCR) + (PtCR << 8);     
			ContribTem[i++] = ContribCandDR;     
			PixelValueR = ((ContribAandBR << 8) + (ContribCandDR - ContribAandBR) * RowDelta) >> 16;                         
			
			ContribAandBG = ContribTem[i];   
			ContribCandDG = ColDelta * (PtDG - PtCG) + (PtCG << 8);     
			ContribTem[i++] = ContribCandDG;     
			PixelValueG = ((ContribAandBG << 8) + (ContribCandDG - ContribAandBG) * RowDelta) >> 16;     
			
			ContribAandBB = ContribTem[i];   
			ContribCandDB = ColDelta * (PtDB - PtCB) + (PtCB << 8);   
			ContribTem[i++] = ContribCandDB;   
			PixelValueB = ((ContribAandBB << 8) + (ContribCandDB - ContribAandBB) * RowDelta) >> 16;     
			
			OutImage[4 * DestRow * DStride + 4 * DestCol + 0] = (unsigned char)PixelValueA;     
			OutImage[4 * DestRow * DStride + 4 * DestCol + 1] = (unsigned char)PixelValueR;     
			OutImage[4 * DestRow * DStride + 4 * DestCol + 2] = (unsigned char)PixelValueG;     
			OutImage[4 * DestRow * DStride + 4 * DestCol + 3] = (unsigned char)PixelValueB;   
		}   
	}   
}

// 创建一个新的IMAGE对象, 亮度为原始IMAGE对象的亮度 * 新的亮度因子 fBrightnessRatio
// fBrightnessRatio >= 0.0
XM_IMAGE * XM_ImageCloneWithBrightness (XM_IMAGE *lpImage, float fBrightnessRatio)
{
	unsigned int width;
	unsigned int height;
	unsigned int stride;
	unsigned int x, y;
	XM_IMAGE *lpScaleImage;
	unsigned char *dst, *src;
	unsigned char *s, *d;
	unsigned int BrightnessRatio = (unsigned int)(fBrightnessRatio * 255);
	if(lpImage == NULL || fBrightnessRatio < 0.0)
		return NULL;
	
	// 检查是否为合法的IMAGE对象
	if(lpImage->id != XM_IMAGE_ID)
		return NULL;

	// 暂时仅支持ARGB888模式
	if(lpImage->format != XM_OSD_LAYER_FORMAT_ARGB888)
		return NULL;

	width = lpImage->width;
	height = lpImage->height;
	stride = (width + 7) & (~7);
	stride *= 4;

	lpScaleImage = XM_heap_malloc (sizeof(XM_IMAGE) - 4 + stride * height); 
	if(lpScaleImage == NULL)
		return NULL;
	
	dst = (unsigned char *)lpScaleImage->image;
	src = (unsigned char *)lpImage->image;
	for (y = 0; y < height; y ++)
	{
		d = dst;
		s = src;
		for (x = 0; x < width; x ++)
		{
			unsigned int argb = *(unsigned int *)s;
			unsigned int r, g, b;
			r = (argb >> 16) & 0xFF;
			g = (argb >> 8) & 0xFF;
			b = argb & 0xFF;
			r = (r * BrightnessRatio) >> 8;
			g = (g * BrightnessRatio) >> 8;
			b = (b * BrightnessRatio) >> 8;
			if(r > 255)
				r = 255;
			if(g > 255)
				g = 255;
			if(b > 255)
				b = 255;
			*(unsigned int *)d = (argb & 0xFF000000) | (r << 16) | (g << 8) | b;
			d += 4;
			s += 4;
		}
		
		dst += stride;
		src += lpImage->stride;
	}
	lpScaleImage->id = XM_IMAGE_ID;
	lpScaleImage->format = lpImage->format;
	lpScaleImage->width = width;
	lpScaleImage->height = height;
	lpScaleImage->stride = (unsigned short)stride;
	lpScaleImage->ayuv = NULL;

	return lpScaleImage;
}
  
// 创建一个灰度化的IMAGE对象
XM_IMAGE * XM_ImageCloneWithGrayScaleEffect (XM_IMAGE *lpImage)
{
	unsigned int width;
	unsigned int height;
	unsigned int stride;
	unsigned int x, y;
	XM_IMAGE *lpScaleImage;
	unsigned char *dst, *src;
	unsigned char *s, *d;
	unsigned int gray;
	if(lpImage == NULL)
		return NULL;
	
	// 检查是否为合法的IMAGE对象
	if(lpImage->id != XM_IMAGE_ID)
		return NULL;

	// 暂时仅支持ARGB888模式
	if(lpImage->format != XM_OSD_LAYER_FORMAT_ARGB888)
		return NULL;

	width = lpImage->width;
	height = lpImage->height;
	stride = (width + 7) & (~7);
	stride *= 4;

	lpScaleImage = XM_heap_malloc (sizeof(XM_IMAGE) - 4 + stride * height); 
	if(lpScaleImage == NULL)
		return NULL;
	
	dst = (unsigned char *)lpScaleImage->image;
	src = (unsigned char *)lpImage->image;
	for (y = 0; y < height; y ++)
	{
		d = dst;
		s = src;
		for (x = 0; x < width; x ++)
		{
			unsigned int argb = *(unsigned int *)s;
			unsigned int r, g, b;
			r = (argb >> 16) & 0xFF;
			g = (argb >> 8) & 0xFF;
			b = argb & 0xFF;
			gray = (r*19595 + g*38469 + b*7472) >> 16;
			if(gray > 255)
				gray = 255;
			*(unsigned int *)d = (argb & 0xFF000000) | (gray << 16) | (gray << 8) | gray;
			d += 4;
			s += 4;
		}
		
		dst += stride;
		src += lpImage->stride;
	}
	lpScaleImage->id = XM_IMAGE_ID;
	lpScaleImage->format = lpImage->format;
	lpScaleImage->width = width;
	lpScaleImage->height = height;
	lpScaleImage->stride = (unsigned short)stride;
	lpScaleImage->ayuv = NULL;

	return lpScaleImage;
}

// 将IMAGE格式的图像缩小
// uScaleRatio 缩小的倍数，
//   0、1    不变
//   2		 尺寸为原来的1/2
//   3		 尺寸为原来的1/3
//   n       尺寸为原来的1/n
// 暂时仅支持ARGB888模式
XM_IMAGE * XM_ImageScale (XM_IMAGE *lpImage, UINT uScaleRatio)
{
	unsigned int width;
	unsigned int height;
	unsigned int stride;
	unsigned int x, y;
	XM_IMAGE *lpScaleImage;
	unsigned char *dst, *src;
	if(lpImage == NULL || uScaleRatio > 8)
		return NULL;
	
	// 检查是否为合法的IMAGE对象
	if(lpImage->id != XM_IMAGE_ID)
		return NULL;

	// 暂时仅支持ARGB888模式
	if(lpImage->format != XM_OSD_LAYER_FORMAT_ARGB888)
		return NULL;

	if(uScaleRatio == 0)
		uScaleRatio = 1;
	width = lpImage->width / uScaleRatio;
	height = lpImage->height / uScaleRatio;
	if( (width * height) == 0 )
		return NULL;
	stride = (width + 7) & (~7);
	stride *= 4;

	lpScaleImage = XM_heap_malloc (sizeof(XM_IMAGE) - 4 + stride * height); 
	if(lpScaleImage == NULL)
		return NULL;
	
	if(uScaleRatio == 1)
	{
		memcpy (lpScaleImage, lpImage, sizeof(XM_IMAGE) - 4 + stride * height);
	}
	else
	{
#if 0
		ResizeImage (	(unsigned char *)lpImage->image, (unsigned char *)lpScaleImage->image, 
							lpImage->width, lpImage->height, lpImage->stride/4,
							width, height, stride/4);
		lpScaleImage->id = XM_IMAGE_ID;
		lpScaleImage->format = lpImage->format;
		lpScaleImage->width = width;
		lpScaleImage->height = height;
		lpScaleImage->stride = (unsigned short)stride;
#else
		unsigned char *s, *d;
		dst = (unsigned char *)lpScaleImage->image;
		src = (unsigned char *)lpImage->image;
		for (y = 0; y < height; y ++)
		{
			d = dst;
			s = src;
			for (x = 0; x < width; x ++)
			{
				*(unsigned int *)d = *(unsigned int *)s;
				d += 4;
				s += 4 * uScaleRatio;
			}

			dst += stride;
			src += lpImage->stride * uScaleRatio;
		}
		lpScaleImage->id = XM_IMAGE_ID;
		lpScaleImage->format = lpImage->format;
		lpScaleImage->width = width;
		lpScaleImage->height = height;
		lpScaleImage->stride = (unsigned short)stride;
		lpScaleImage->ayuv = NULL;
#endif
	}

	return lpScaleImage;
}


// 将IMAGE图像显示在视窗指定位置。
// 窗口需要已关联framebuffer对象
// 支持视窗的Alpha因子混合
// 暂时仅支持ARGB888格式
int XM_ImageDisplay (XM_IMAGE *lpImage, HANDLE hWnd, XMCOORD x, XMCOORD y)
{
	xm_osd_framebuffer_t framebuffer;
	//unsigned int i, j;
	//unsigned char *src, *dst;
	unsigned char layer_alpha;
	if(hWnd == NULL)
		return 0;
	if(lpImage == NULL)
		return 0;
	if(lpImage->id != XM_IMAGE_ID)
		return 0;

	framebuffer = XM_GetWindowFrameBuffer (hWnd);
	if(framebuffer == NULL)
		return 0;

	// 检查显示格式是否一致
	if(framebuffer->format != lpImage->format)
		return 0;

	layer_alpha = XM_GetWindowAlpha (hWnd);

	return XM_lcdc_copy_raw_image (
				framebuffer->address,
				framebuffer->format,
				framebuffer->width, framebuffer->height,
				framebuffer->stride,
				x, y,
				(unsigned char *)lpImage->image,
				lpImage->width, lpImage->height,
				lpImage->stride,
				0, 0,
				lpImage->width,
				lpImage->height,
				layer_alpha
				);
}

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

						)
{
	XM_IMAGE *lpImage_AYUV = NULL;
	if(lpImage == NULL || lpImage->id != XM_IMAGE_ID)
	{
		XM_printf ("illegal Image\n");
		return -1;
	}

	if(lpImage->format != XM_OSD_LAYER_FORMAT_ARGB888)
	{
		XM_printf ("illegal Image format\n");
		return -1;
	}

	// 转换ARGB888到YUV444格式
	if(lpImage->ayuv == NULL)
	{
		lpImage_AYUV = XM_ImageConvert (lpImage, XM_OSD_LAYER_FORMAT_AYUV444);
		if(lpImage_AYUV == NULL)
		{
			XM_printf ("ayuv failed, memory busy\n");
			return -1;
		}
		lpImage->ayuv = lpImage_AYUV;
	}
	else
	{
		lpImage_AYUV = (XM_IMAGE *)lpImage->ayuv;
	}

	osd_offset_x &= ~0x1;
	osd_offset_y &= ~0x1;

	XM_lcdc_osd_layer_load_ayuv444_image_normal (
				(unsigned char *)lpImage_AYUV->image,
				((unsigned char *)lpImage_AYUV->image) + lpImage_AYUV->stride * lpImage_AYUV->height,
				((unsigned char *)lpImage_AYUV->image) + lpImage_AYUV->stride * lpImage_AYUV->height * 2,
				((unsigned char *)lpImage_AYUV->image) + lpImage_AYUV->stride * lpImage_AYUV->height * 3,
				lpImage_AYUV->width, lpImage_AYUV->height,
				lpImage_AYUV->stride,
				img_offset_x, img_offset_y,
				lpImage_AYUV->width, lpImage_AYUV->height,
				osd_layer_format,
				osd_layer_buffer,
				osd_layer_buffer + osd_layer_stride * osd_layer_height,
				osd_layer_buffer + osd_layer_stride * osd_layer_height * 5 /4,
				osd_layer_width,
				osd_layer_height,
				osd_layer_stride,
				osd_offset_x, osd_offset_y);
//	XM_ImageDelete (lpImage_AYUV);
	return 0;
} 

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

						)
{
	XM_IMAGE *lpImage_AYUV = NULL;
	if(lpImage == NULL || lpImage->id != XM_IMAGE_ID)
	{
		XM_printf (" 111 illegal Image\n");
		return -1;
	}

	if(lpImage->format != XM_OSD_LAYER_FORMAT_ARGB888)
	{
		XM_printf ("illegal Image format\n");
		return -1;
	}

	// 转换ARGB888到YUV444格式
	if(lpImage->ayuv == NULL)
	{
		lpImage_AYUV = XM_ImageConvert (lpImage, XM_OSD_LAYER_FORMAT_AYUV444);
		if(lpImage_AYUV == NULL)
		{
			XM_printf ("ayuv failed, memory busy\n");
			return -1;
		}
		lpImage->ayuv = lpImage_AYUV;
	}
	else
	{
		lpImage_AYUV = (XM_IMAGE *)lpImage->ayuv;
	}

	osd_offset_x &= ~0x1;
	osd_offset_y &= ~0x1;

#if 0
	XM_lcdc_osd_layer_load_yuva444_image (
				(unsigned char *)lpImage_AYUV->image,
				lpImage_AYUV->width, lpImage_AYUV->height,
				lpImage_AYUV->stride,
				img_offset_x, img_offset_y,
				lpImage_AYUV->width, lpImage_AYUV->height,
				osd_layer_format,
				osd_layer_buffer,
				osd_layer_buffer + osd_layer_stride * osd_layer_height,
				osd_layer_buffer + osd_layer_stride * osd_layer_height * 5 /4,
				osd_layer_width,
				osd_layer_height,
				osd_layer_stride,
				osd_offset_x, osd_offset_y);	
#else
	XM_lcdc_osd_layer_load_ayuv444_image_full(
				(unsigned char *)lpImage_AYUV->image,
				((unsigned char *)lpImage_AYUV->image) + lpImage_AYUV->stride * lpImage_AYUV->height,
				((unsigned char *)lpImage_AYUV->image) + lpImage_AYUV->stride * lpImage_AYUV->height * 2,
				((unsigned char *)lpImage_AYUV->image) + lpImage_AYUV->stride * lpImage_AYUV->height * 3,
				lpImage_AYUV->width, lpImage_AYUV->height,
				lpImage_AYUV->stride,
				img_offset_x, img_offset_y,
				lpImage_AYUV->width, lpImage_AYUV->height,
				osd_layer_format,
				osd_layer_buffer,
				osd_layer_buffer + osd_layer_stride * osd_layer_height,
				osd_layer_buffer + osd_layer_stride * osd_layer_height * 5 /4,
				osd_layer_width,
				osd_layer_height,
				osd_layer_stride,
				osd_offset_x, osd_offset_y);
#endif
//	XM_ImageDelete (lpImage_AYUV);
	return 0;
} 

int XM_ImageBlendToFrameBuffer (	
						XM_IMAGE *lpImage,
						unsigned int img_offset_x,					// 复制区域在源图像的偏移(相对源图像平面的左上角)
						unsigned int img_offset_y,
						unsigned int img_w,							// 待复制区域的像素宽度、像素高度		
						unsigned int img_h,
						xm_osd_framebuffer_t framebuffer,
						unsigned int osd_offset_x,					//	复制区域在OSD层的偏移 (相对OSD平面的左上角)
						unsigned int osd_offset_y
						)
{
	unsigned int osd_layer_format;			// 目标OSD的视频格式
	unsigned int osd_layer_width;				// 目标OSD的像素宽度、像素高度
	unsigned int osd_layer_height;
	unsigned char *osd_layer_buffer;
	if(lpImage == NULL)
		return -1;

	if(framebuffer == NULL)
		return -1;

	if(framebuffer->lcd_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", framebuffer->lcd_channel);
		return -1;
	}
	if(framebuffer->osd_layer >= XM_OSD_LAYER_COUNT)
	{
		printf ("illegal osd_layer (%d)\n", framebuffer->osd_layer);
		return -1;
	}

	if(img_offset_x >= lpImage->width || img_offset_y >= lpImage->height)
	{
		return -1;
	}
	if( (img_offset_x + img_w) > lpImage->width )
	{
		img_w = lpImage->width - img_offset_x;
	}
	if( (img_offset_y + img_h) > lpImage->height )
	{
		img_h = lpImage->height - img_offset_y;
	}

	osd_layer_buffer = framebuffer->address;
	osd_layer_format = XM_lcdc_osd_get_format (framebuffer->lcd_channel, framebuffer->osd_layer);
	osd_layer_width  = XM_lcdc_osd_get_width (framebuffer->lcd_channel, framebuffer->osd_layer);
	osd_layer_height = XM_lcdc_osd_get_height (framebuffer->lcd_channel, framebuffer->osd_layer);

	if(osd_layer_buffer == NULL)
		return -1;
	
	if( osd_offset_x >= osd_layer_width )
		return -1;
	if( osd_offset_y >= osd_layer_height )
		return -1;
	if( (osd_offset_x + img_w) > osd_layer_width )
		img_w = osd_layer_width - osd_offset_x;
	if( (osd_offset_y + img_h) > osd_layer_height )
		img_h = osd_layer_height - osd_offset_y;

	if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420 || osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		XM_ImageBlend_argb888_to_yuv420 (
					lpImage,
					img_offset_x, img_offset_y,
					img_w, 
					img_h,
					osd_layer_buffer,
					osd_layer_format,
					osd_layer_width,
					osd_layer_height,
					osd_layer_width,
					osd_offset_x,
					osd_offset_y
					);
	}
	else
	{
		XM_printf ("XM_ImageBlend NG, unsupport format(%d)\n", osd_layer_format);
		return -1;
	}

	return 0;
}

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
						)
{
	unsigned int osd_layer_format;			// 目标OSD的视频格式
	unsigned int osd_layer_width;				// 目标OSD的像素宽度、像素高度
	unsigned int osd_layer_height;
	unsigned char *osd_layer_buffer;

	if(lpImage == NULL)
		return -1;

	if(lcdc_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcdc_channel);
		return -1;
	}
	if(osd_layer >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer);
		return -1;
	}

	if(img_offset_x >= lpImage->width || img_offset_y >= lpImage->height)
	{
		return -1;
	}
	if( (img_offset_x + img_w) > lpImage->width )
	{
		img_w = lpImage->width - img_offset_x;
	}
	if( (img_offset_y + img_h) > lpImage->height )
	{
		img_h = lpImage->height - img_offset_y;
	}

	osd_layer_buffer = XM_lcdc_osd_get_y_addr (lcdc_channel, osd_layer);
	osd_layer_format = XM_lcdc_osd_get_format (lcdc_channel, osd_layer);
	osd_layer_width  = XM_lcdc_osd_get_width (lcdc_channel, osd_layer);
	osd_layer_height = XM_lcdc_osd_get_height (lcdc_channel, osd_layer);

	if(osd_layer_buffer == NULL)
		return -1;
	
	if( osd_offset_x >= osd_layer_width )
		return -1;
	if( osd_offset_y >= osd_layer_height )
		return -1;
	if( (osd_offset_x + img_w) > osd_layer_width )
		img_w = osd_layer_width - osd_offset_x;
	if( (osd_offset_y + img_h) > osd_layer_height )
		img_h = osd_layer_height - osd_offset_y;

	if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420 || osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		XM_ImageBlend_argb888_to_yuv420 (
					lpImage,
					img_offset_x, img_offset_y,
					img_w, 
					img_h,
					osd_layer_buffer,
					osd_layer_format,
					osd_layer_width,
					osd_layer_height,
					osd_layer_width,
					osd_offset_x,
					osd_offset_y
					);
	}
	else
	{
		XM_printf ("XM_ImageBlend NG, unsupport format(%d)\n", osd_layer_format);
		return -1;
	}

	return 0;
}

/*
// 将标志水印IMAGE图像与YUV格式的视频层混合
int XM_BlendFlagWaterMarkImage ( unsigned char *osd_layer_buffer, 
								unsigned int osd_layer_width, unsigned int osd_layer_height,
								unsigned int osd_layer_stride
							 )
{
	int ret = 0;
	XM_IMAGE *lpImage_ARGB = NULL;
	//XM_IMAGE *lpImage_AYUV = NULL;
	int cbPngBuffer = 0;
	int off_x, off_y;
	// 载入标记水印PNG图片
	char *lpPngBuffer;
	
	if(AP_GetMenuItem (APPMENUITEM_FLAG_STAMP) != AP_SETTING_VIDEO_TIMESTAMP_ON)
		return ret;
	
#if 1
	lpPngBuffer = XM_heap_malloc (MAX_FLAG_PNG_SIZE);
	if(lpPngBuffer)
	{
		cbPngBuffer = XM_FlagWaterMarkPngImageLoad (lpPngBuffer, MAX_FLAG_PNG_SIZE);
		if(cbPngBuffer > 0)
		{
			lpImage_ARGB = XM_ImageCreate (lpPngBuffer, cbPngBuffer, XM_OSD_LAYER_FORMAT_ARGB888);
		}
		XM_heap_free (lpPngBuffer);
	}

	if(lpImage_ARGB == NULL)
		return -1;
	// 将标记水印居中显示
	off_x = (osd_layer_width - lpImage_ARGB->width) / 2;
	off_y = (osd_layer_height - lpImage_ARGB->height) / 2;

	ret = XM_ImageBlend (
			lpImage_ARGB,
			0, 0,
			lpImage_ARGB->width,
			lpImage_ARGB->height,
			XM_LCDC_CHANNEL_0,
			XM_OSD_LAYER_0,
			off_x,
			off_y
			);
	XM_ImageDelete (lpImage_ARGB);
#else

	lpPngBuffer = XM_heap_malloc (MAX_FLAG_PNG_SIZE);
	if(lpPngBuffer)
	{
		cbPngBuffer = XM_FlagWaterMarkPngImageLoad (lpPngBuffer, MAX_FLAG_PNG_SIZE);
		if(cbPngBuffer > 0)
		{
			lpImage_ARGB = XM_ImageCreate (lpPngBuffer, cbPngBuffer, XM_OSD_LAYER_FORMAT_ARGB888);
			lpImage_AYUV = XM_ImageConvert (lpImage_ARGB, XM_OSD_LAYER_FORMAT_AYUV444);
		}
		XM_heap_free (lpPngBuffer);
	}
	
	if(lpImage_AYUV)
	{
		int off_x, off_y;
		// 将标记水印居中显示
		off_x = (osd_layer_width - lpImage_AYUV->width) / 2;
		off_y = (osd_layer_height - lpImage_AYUV->height) / 2;
		if(off_x >= 0 && off_y >= 0)
		{
			// 偶地址约束
			off_x &= ~0x1;
			off_y &= ~0x1;
			XM_lcdc_osd_layer_load_ayuv444_image (
				(unsigned char *)lpImage_AYUV->image,
				((unsigned char *)lpImage_AYUV->image) + lpImage_AYUV->stride * lpImage_AYUV->height,
				((unsigned char *)lpImage_AYUV->image) + lpImage_AYUV->stride * lpImage_AYUV->height * 2,
				((unsigned char *)lpImage_AYUV->image) + lpImage_AYUV->stride * lpImage_AYUV->height * 3,
				lpImage_AYUV->width, lpImage_AYUV->height,
				lpImage_AYUV->stride,
				0, 0,
				lpImage_AYUV->width, lpImage_AYUV->height,
				osd_layer_buffer,
				osd_layer_buffer + osd_layer_stride * osd_layer_height,
				osd_layer_buffer + osd_layer_stride * osd_layer_height * 5 /4,
				osd_layer_width,
				osd_layer_height,
				osd_layer_stride,
				off_x, off_y);
			ret = 1;
		}
	}
	XM_ImageDelete (lpImage_AYUV);
	XM_ImageDelete (lpImage_ARGB);
#endif
	return ret;
}
*/

//Y_UV420转换为ARGB
void XM_ImageConvert_YUV420_to_ARGB(unsigned char *ycbcr, 
										 unsigned char *rgb, 
										 unsigned int w, 
										 unsigned int h)
{
    double c_r, c_g, c_b;
	int i_r, i_g, i_b;
	unsigned int i, j;

	
	unsigned char *y  = ycbcr;
	unsigned char *cb = ycbcr + w * h;
	unsigned char *cr = ycbcr + w * h * 5/4;

	for (j = 0; j < h; j ++)
	{
	   	for(i = 0; i < w; i ++)
	   	{
	   	    unsigned char d_y = y[j * w + i];
			unsigned char d_cb = cb[(j >> 1)*(w >> 1) + (i >> 1)];
			unsigned char d_cr = cr[(j >> 1)*(w >> 1) + (i >> 1)];

			c_r = 1.164 * (d_y - 16) + 1.793 * (d_cr - 128) + 0.5;
			c_g = 1.164 * (d_y - 16) - 0.534 * (d_cr - 128) - 0.213 * (d_cb - 128) + 0.5;
			c_b = 1.164 * (d_y - 16) + 2.115 * (d_cb - 128) + 0.5;

			i_r = (int)c_r;
			if(i_r > 255)
				i_r = 255;
			else if(i_r < 0)
				i_r = 0;

			i_g = (int)c_g;
			if(i_g > 255)
				i_g = 255;
			else if(i_g < 0)
				i_g = 0;

			i_b = (int)c_b;
			if(i_b > 255)
				i_b = 255;
			else if(i_b < 0)
				i_b = 0;
			
			*rgb ++ = (unsigned char)i_b;
			*rgb ++ = (unsigned char)i_g;
			*rgb ++ = (unsigned char)i_r;
			*rgb ++ = 0xFF;
	   	}
	}
}


//Y_UV420转换为ARGB
void XM_ImageConvert_Y_UV420_to_ARGB(unsigned char *ycbcr, 
										 unsigned char *rgb, 
										 unsigned int w, 
										 unsigned int h)
{
    double c_r, c_g, c_b;
	int i_r, i_g, i_b;
	unsigned int i, j;
	char *dst_buffer;
	char *src_yaddr,*src_uvaddr;
	char *dst_yaddr,*dst_uaddr,*dst_vaddr;
	unsigned char *y;
	unsigned char *cb;
	unsigned char *cr;

	// Y_UV420 数据转换为 YUV420
    dst_buffer = (unsigned char *)kernel_malloc (w * h * 3/2);
	dst_yaddr = dst_buffer;
	dst_uaddr= dst_buffer + w*h;
	dst_vaddr= dst_buffer + w*h*5/4;
	
	src_yaddr  = ycbcr;
	src_uvaddr = ycbcr + w*h;
	memcpy(dst_yaddr, src_yaddr, w*h);
	for(i=0;i<h/2;i++)
	{
		for(j=0;j<w/2;j++)
		{
			*dst_uaddr++ = *src_uvaddr++;
			*dst_vaddr++ = *src_uvaddr++;	
		}
	}

	y  = dst_yaddr;
	cb = dst_yaddr + w * h;
	cr = dst_yaddr + w * h * 5/4;

	for (j = 0; j < h; j ++)
	{
	   	for(i = 0; i < w; i ++)
	   	{
	   	    unsigned char d_y = y[j * w + i];
			unsigned char d_cb = cb[(j >> 1)*(w >> 1) + (i >> 1)];
			unsigned char d_cr = cr[(j >> 1)*(w >> 1) + (i >> 1)];

			c_r = 1.164 * (d_y - 16) + 1.793 * (d_cr - 128) + 0.5;
			c_g = 1.164 * (d_y - 16) - 0.534 * (d_cr - 128) - 0.213 * (d_cb - 128) + 0.5;
			c_b = 1.164 * (d_y - 16) + 2.115 * (d_cb - 128) + 0.5;

			i_r = (int)c_r;
			if(i_r > 255)
				i_r = 255;
			else if(i_r < 0)
				i_r = 0;

			i_g = (int)c_g;
			if(i_g > 255)
				i_g = 255;
			else if(i_g < 0)
				i_g = 0;

			i_b = (int)c_b;
			if(i_b > 255)
				i_b = 255;
			else if(i_b < 0)
				i_b = 0;
			
			*rgb ++ = (unsigned char)i_b;
			*rgb ++ = (unsigned char)i_g;
			*rgb ++ = (unsigned char)i_r;
			*rgb ++ = 0xFF;
	   	}
	}
	kernel_free (dst_buffer);
}


// Y_UV420 数据转换为 YUV420
void XM_ImageConvert_Y_UV420toYUV420(char *Src,char *Dst, int w ,int h)
{
	int i,j;
	char *Src_Yaddr,*Src_UVaddr;
	char *Dst_Yaddr,*Dst_Uaddr,*Dst_Vaddr;

	Src_Yaddr  = Src;
	Src_UVaddr = Src_Yaddr + w*h;

	Dst_Yaddr = Dst;
	Dst_Uaddr= Dst_Yaddr + w*h;
	Dst_Vaddr= Dst_Uaddr + w*h/4;
	
	memcpy(Dst, Src, w*h);
	
	for(i=0;i<h/2;i++)
	{
		for(j=0;j<w/2;j++)
		{
			*Dst_Uaddr++ = *Src_UVaddr++;
			*Dst_Vaddr++ = *Src_UVaddr++;	
		}
	} 
}


