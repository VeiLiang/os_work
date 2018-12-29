//****************************************************************************
//
//	Copyright (C) ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_hybrid_stream.h
//	  混合流接口
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

// 混合帧通道编号
enum {	
	XM_HYBRID_CH_VIDEO_F	 = 0,		// 前视摄像头
	XM_HYBRID_CH_VIDEO_B,			// 后视摄像头
	XM_HYBRID_CH_VOICE ,				// 音频
	XM_HYBRID_CH_COUNT
};

// 混合帧帧类型
enum {
	XM_HYBRID_TYPE_I_FRAME = 0,	// I帧
	XM_HYBRID_TYPE_P_FRAME,			// P帧
	XM_HYBRID_TYPE_PCM_8000,		// PCM单声道8K采样
	XM_HYBRID_TYPE_COUNT
};

int XM_HybridStreamInit (void);

int XM_HybridStreamSave (unsigned char ch, unsigned char type, unsigned int no, unsigned int size, char *data);
						// CH(通道，1字节), TYPE(类型，1字节), rev(保留, 固定为0, 2字节), NO(帧序号, 4字节),
						//		size(hubrid流字节长度, 4字节), 流数据(size字节长度, 4字节对齐)
						//	CH   --> 0x00 前置摄像头, 0x01 后置摄像头, 0x02 音频数据.
						//	TYPE --> 0x00 I帧, 0x01 P帧, 0x02 8K采样单声道PCM音频数据.
						//	NO   --> 帧序号, 每个通道具有独立的序号, 序号递增, 主机端根据帧号用来判断是否丢帧.
						//	SIZE --> hubrid流字节长度

// 强制视频通道重新开始I帧编码
// 主机端发现帧丢失时, 可请求下一帧从I帧编码
int XM_HybridStreamForceIFrame (void);

// 启动混合编码传输模式
int XM_HybridStreamStart (void);

// 停止混合通道编码模式
int XM_HybridStreamStop (void);

// 插入Hybrid流
int XM_HybridInsertHybridStream (unsigned int ch, unsigned int type, char *hybrid_data, unsigned int hybrid_size);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_HYBRID_STREAM_H_
