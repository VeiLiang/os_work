//****************************************************************************
//
//	Copyright (C) 2014 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: arkn141_jpeg_decoder.h
//	  constant��macro & basic typedef definition of jpeg encoder/decoder
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
	char *jpeg_stream,			// JPEG��������ָ��
	int   jpeg_length,			// JPEG���ֽڳ���
	int  *img_width,				// �����ͼƬ���(���뻺��������)
	int  *img_height,				// �����ͼƬ�߶�
	int  *img_display_width,		// ͼƬ��ʵ����ʾ���
	int  *img_display_height		// ͼƬ��ʵ����ʾ�߶�
);

int arkn141_jpeg_decode (	
	char *jpeg_stream,			// JPEG��������ָ��
	int   jpeg_length,			// JPEG���ֽڳ���
	char*	image_y,
	char* image_cb,
	char* image_cr
) ;

#endif	// _ARKN141_JPEG_ENCODER_H_

