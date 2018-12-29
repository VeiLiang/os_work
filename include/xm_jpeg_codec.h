//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_jpeg_codec.h
//	  constant，macro & basic typedef definition of JPEG codec library
//
//	Revision history
//
//		2011.08.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_JPEG_CODEC_H_
#define _XM_JPEG_CODEC_H_


#if defined (__cplusplus)
	extern "C"{
#endif

// 仅支持NV12格式解码输出
int xm_jpeg_decode (const char *jpeg_data, size_t jpeg_size, 
						unsigned char *yuv,		// Y_UV420输出地址(NV12格式)
						unsigned int width,		// 解码输出的像素宽度
						unsigned int height		// 解码输出的像素高度
						);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_JPEG_CODEC_H_