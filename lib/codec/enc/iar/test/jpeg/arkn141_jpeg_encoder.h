//****************************************************************************
//
//	Copyright (C) 2014 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: arkn141_jpeg_codec.h
//	  constant，macro & basic typedef definition of jpeg encoder/decoder
//
//	Revision history
//
//		2014.12.18	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _ARKN141_JPEG_CODEC_H_
#define _ARKN141_JPEG_CODEC_H_


int arkn141_jpeg_encode (	
	char *imgY_in, 			// 输入图像的Y/Cb/Cr
	char *imgCb_in, 
	char *imgCr_in ,
	unsigned int frame_type,
	int width,							// 输入图像的宽度/高度
	int height,
	unsigned char *jpeg_stream, 	// 保存JPEG编码流的缓冲区
	int *jpeg_len						// in,  表示JPEG编码流的缓冲区的字节长度
											// Out, 表示编码成功后JPEG码流的字节长度
) ;

#endif	// _ARKN141_JPEG_ENCODER_H_

