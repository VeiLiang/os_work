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

#define _ARKN141_H264_ENCODE_INTERRUPT_	// ʹ�ܱ����ж�

#define _ARKN141_CODEC_DECODE_INTERRUPT_	// ʹ�ܽ����ж�

#if defined (__cplusplus)
	extern "C"{
#endif
		
// Ӳ�������������		
enum {
	ARKN141_CODEC_TYPE_H264_ENCODER = 0,
	ARKN141_CODEC_TYPE_JPEG_ENCODER,
	ARKN141_CODEC_TYPE_H264_DECODER,
	ARKN141_CODEC_TYPE_JPEG_DECODER,
	ARKN141_CODEC_TYPE_COUNT
};


// ARKN141 Ӳ�����������ʼ��������ϵͳ��Դ
int arkn141_codec_init (void);

// ARKN141 Ӳ����������رգ��ͷ�ϵͳ��Դ
int arkn141_codec_exit (void);

// �����Ӳ��codec��Դ�Ļ���ʹ��(������ʹ��)
int arkn141_codec_reserve_hw (int b_is_decoder);

// �ͷŶ�Ӳ��codec��Դ�Ļ���ʹ��(������ʹ��)
int arkn141_codec_release_hw (int b_is_decoder);

// ��Ӳ���������
int arkn141_codec_open  (int codec_type);

// �ر�Ӳ���������
int arkn141_codec_close (int codec_type);

// �ȴ�H264���������
// 0   h264 codec encode finish
// -1  h264 codec δ��ʼ��
// -2  h264 codec timeout
int  arkn141_codec_waiting_for_h264_encode_finished (void);

// ����H264��������¼�
void arkn141_codec_trigger_h264_encode_finish_event (void);

// �ȴ��������������
// timeout	   timeout period for the wait specified in milliseconds; 
// -1          means an infinit wait   
// ����ֵ����
// 0   codec decode finish
// -1  codec δ��ʼ��
// -2  codec timeout
int arkn141_codec_waiting_for_decode_finished (unsigned int timeout);

// ������������������¼�
void arkn141_codec_trigger_decode_finish_event (void);



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif  // _ARKN141_CODEC_H_

