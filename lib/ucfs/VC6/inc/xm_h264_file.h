//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_h264_file.h
//
//	Revision history
//
//		2012.12.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_H264_FILE_
#define _XM_H264_FILE_

void *h264_fopen (const char *filename, const char *mode);

int h264_fclose  (void *stream);

int h264_fseek   (void *stream, long offset, int mode);

size_t h264_fread  (void *ptr, size_t size, size_t nelem, void *stream);

size_t h264_fwrite (void *ptr, size_t size, size_t nelem, void *stream);

int h264_filelength (void *stream);

#endif // _XM_H264_FILE_
