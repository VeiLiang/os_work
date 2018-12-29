//****************************************************************************
//
//	Copyright (C) 2014 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: arkn141_jpeg_codec.h
//	  constant��macro & basic typedef definition of jpeg encoder/decoder
//
//	Revision history
//
//		2014.12.18	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _ARKN141_JPEG_CODEC_H_
#define _ARKN141_JPEG_CODEC_H_


int arkn141_jpeg_encode (	
	char *imgY_in, 			// ����ͼ���Y/Cb/Cr
	char *imgCb_in, 
	char *imgCr_in ,
	unsigned int frame_type,
	int width,							// ����ͼ��Ŀ��/�߶�
	int height,
	unsigned char *jpeg_stream, 	// ����JPEG�������Ļ�����
	int *jpeg_len						// in,  ��ʾJPEG�������Ļ��������ֽڳ���
											// Out, ��ʾ����ɹ���JPEG�������ֽڳ���
) ;

#endif	// _ARKN141_JPEG_ENCODER_H_

