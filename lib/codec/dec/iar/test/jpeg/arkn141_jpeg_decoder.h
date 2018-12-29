//****************************************************************************
//
//	Copyright (C) 2014 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: arkn141_jpeg_decoder.h
//	  constant，macro & basic typedef definition of jpeg encoder/decoder
//
//	Revision history
//
//		2014.12.18	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _ARKN141_JPEG_DECODER_H_
#define _ARKN141_JPEG_DECODER_H_

// only YUV420 support
int arkn141_jpeg_get_image_info (
	char *jpeg_stream,			// JPEG流缓存区指针
	int   jpeg_length,			// JPEG流字节长度
	int  *img_width,				// 解码的图片宽度(解码缓冲区分配)
	int  *img_height,				// 解码的图片高度
	int  *img_display_width,		// 图片的实际显示宽度
	int  *img_display_height		// 图片的实际显示高度
);

int arkn141_jpeg_decode (	
	char *jpeg_stream,			// JPEG流缓存区指针
	int   jpeg_length,			// JPEG流字节长度
	char*	image_y,
	char* image_cb,
	char* image_cr
) ;

#endif	// _ARKN141_JPEG_ENCODER_H_

