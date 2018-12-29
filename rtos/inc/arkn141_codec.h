//****************************************************************************
//
//	Copyright (C) 2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: arkn141_codec.h
//
//	Revision history
//
//		2014.12.21	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _ARKN141_CODEC_H_
#define _ARKN141_CODEC_H_

#include <xm_proj_define.h>

#define _ARKN141_H264_ENCODE_INTERRUPT_	// 使能编码中断

#define _ARKN141_CODEC_DECODE_INTERRUPT_	// 使能解码中断

#if defined (__cplusplus)
	extern "C"{
#endif
		
// 硬件编解码器类型		
enum {
	ARKN141_CODEC_TYPE_H264_ENCODER = 0,
	ARKN141_CODEC_TYPE_JPEG_ENCODER,
	ARKN141_CODEC_TYPE_H264_DECODER,
	ARKN141_CODEC_TYPE_JPEG_DECODER,
	ARKN141_CODEC_TYPE_COUNT
};


// ARKN141 硬件编解码器初始化，分配系统资源
int arkn141_codec_init (void);

// ARKN141 硬件编解码器关闭，释放系统资源
int arkn141_codec_exit (void);

// 请求对硬件codec资源的互斥使用(排他性使用)
int arkn141_codec_reserve_hw (int b_is_decoder);

// 释放对硬件codec资源的互斥使用(排他性使用)
int arkn141_codec_release_hw (int b_is_decoder);

// 打开硬件编解码器
int arkn141_codec_open  (int codec_type);

// 关闭硬件编解码器
int arkn141_codec_close (int codec_type);

// 等待H264编码器完成
// 0   h264 codec encode finish
// -1  h264 codec 未初始化
// -2  h264 codec timeout
int  arkn141_codec_waiting_for_h264_encode_finished (void);

// 触发H264编码完成事件
void arkn141_codec_trigger_h264_encode_finish_event (void);

// 等待解码器解码完成
// timeout	   timeout period for the wait specified in milliseconds; 
// -1          means an infinit wait   
// 返回值定义
// 0   codec decode finish
// -1  codec 未初始化
// -2  codec timeout
int arkn141_codec_waiting_for_decode_finished (unsigned int timeout);

// 触发解码器解码完成事件
void arkn141_codec_trigger_decode_finish_event (void);



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif  // _ARKN141_CODEC_H_

