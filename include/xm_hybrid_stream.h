//****************************************************************************
//
//	Copyright (C) ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_hybrid_stream.h
//	  ������ӿ�
//
//	Revision history
//
//
//		2013.03.25	ZhuoYongHong first version
//
//***************************************************************************
#ifndef _XM_HYBRID_STREAM_H_
#define _XM_HYBRID_STREAM_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// ���֡ͨ�����
enum {	
	XM_HYBRID_CH_VIDEO_F	 = 0,		// ǰ������ͷ
	XM_HYBRID_CH_VIDEO_B,			// ��������ͷ
	XM_HYBRID_CH_VOICE ,				// ��Ƶ
	XM_HYBRID_CH_COUNT
};

// ���֡֡����
enum {
	XM_HYBRID_TYPE_I_FRAME = 0,	// I֡
	XM_HYBRID_TYPE_P_FRAME,			// P֡
	XM_HYBRID_TYPE_PCM_8000,		// PCM������8K����
	XM_HYBRID_TYPE_COUNT
};

int XM_HybridStreamInit (void);

int XM_HybridStreamSave (unsigned char ch, unsigned char type, unsigned int no, unsigned int size, char *data);
						// CH(ͨ����1�ֽ�), TYPE(���ͣ�1�ֽ�), rev(����, �̶�Ϊ0, 2�ֽ�), NO(֡���, 4�ֽ�),
						//		size(hubrid���ֽڳ���, 4�ֽ�), ������(size�ֽڳ���, 4�ֽڶ���)
						//	CH   --> 0x00 ǰ������ͷ, 0x01 ��������ͷ, 0x02 ��Ƶ����.
						//	TYPE --> 0x00 I֡, 0x01 P֡, 0x02 8K����������PCM��Ƶ����.
						//	NO   --> ֡���, ÿ��ͨ�����ж��������, ��ŵ���, �����˸���֡�������ж��Ƿ�֡.
						//	SIZE --> hubrid���ֽڳ���

// ǿ����Ƶͨ�����¿�ʼI֡����
// �����˷���֡��ʧʱ, ��������һ֡��I֡����
int XM_HybridStreamForceIFrame (void);

// ������ϱ��봫��ģʽ
int XM_HybridStreamStart (void);

// ֹͣ���ͨ������ģʽ
int XM_HybridStreamStop (void);

// ����Hybrid��
int XM_HybridInsertHybridStream (unsigned int ch, unsigned int type, char *hybrid_data, unsigned int hybrid_size);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_HYBRID_STREAM_H_
