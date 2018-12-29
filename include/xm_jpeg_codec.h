//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_jpeg_codec.h
//	  constant��macro & basic typedef definition of JPEG codec library
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

// ��֧��NV12��ʽ�������
int xm_jpeg_decode (const char *jpeg_data, size_t jpeg_size, 
						unsigned char *yuv,		// Y_UV420�����ַ(NV12��ʽ)
						unsigned int width,		// ������������ؿ��
						unsigned int height		// ������������ظ߶�
						);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_JPEG_CODEC_H_