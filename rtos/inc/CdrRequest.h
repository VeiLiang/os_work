//****************************************************************************
//
//	Copyright (C) 2013 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: CdrRequest.h
//
//	Revision history
//
//		2013.5.1 ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_CDR_REQUEST_H_
#define _XM_CDR_REQUEST_H_

#include <xm_proj_define.h>
#include <string.h>
#include "rtos.h"

#ifdef __cplusplus
extern "C" {
#endif
	
// 固定为16字节, 描述全局信息
typedef struct _XM_CDR_INFO {
	unsigned int  uvc_checksum;		// UVC数据的CheckSum字段, 32位数据的累加和, 可以使用该字段用作UVC Payload数据的检查
	unsigned char uvc_channel;			// UVC数据通道号,
												//	1) UVC Video Sample通道号(1字节), 定义Video Sample的来源, 
												//	0 --> 前置摄像头
												//	1 --> 后置摄像头
												//	2 --> 哑元数据(一幅尺寸固定的黑白图像, 用于高速视频文件下载)
												//	3 --> JPEG Image
												//	4 --> 视频文件回放	
												//		哑元数据为缺省状态
	unsigned char card_state;		// 卡状态
	unsigned char asyn_message_count;	// 异步消息包的数量, 允许多条异步消息同时传送
	unsigned char rev[9];			// 保留
} XM_CDR_INFO;

// 将一个异步消息帧加入到CDR帧中
// lpCdrBuffer		CDR帧缓冲区地址
// cbCdrBuffer		CDR帧缓冲区字节大小
// lpExtendMessage	待传的扩展消息数据缓冲区
// cbExtendMessage	待传的扩展消息字节长度	

// 1) UVC扩展"头标志字段"  (8字节, 0xFF, 0xFF, 0x00, 0x00, 'X' 'M' 'C', 'D'), 32字节边界对齐.
// 2) UVC扩展"序号字段" (4字节), 每个帧具有唯一序号, 序号从0开始累加. 通过帧序号, 判断帧是否重发帧
// 3) UVC扩展"信息字段"		// 16字节
// 4) UVC扩展"数据字段"(变长字段)
// 5) UVC扩展"数据长度字段"(4字节)
// 6) UVC扩展"尾标志字段"  (8字节, 0xFF, 0x00, 0xFF, 0x00, 'X' 'M' 'C', 'D'), 32字节边界对齐.
int XMSYS_CdrFrameInsertAsycMessage (char *lpExtendMessage, int cbExtendMessage);

	
// CDR私有协议处理
void XMSYS_CdrPrivateProtocolHandler (unsigned char protocol_type, unsigned char protocol_data);

int XMSYS_JpegFrameInsertExtendMessage (unsigned char uvc_channel, char *lpCdrBuffer, int cbCdrBuffer);

// 将一个异步消息包加入到UVC扩展协议
//	AsycMessage		异步消息ID
//	lpAsycMessage	异步消息数据
//	cbAsycMessage	
// 返回值
//		-1			失败
//		0			成功
int XMSYS_CdrInsertAsycMessage (unsigned char AsycMessage, char *lpAsycMessage, int cbAsycMessage);

void XMSYS_CdrInit (void);

// 打开CDR流文件
int XMSYS_CdrOpenStream (unsigned char ch, unsigned char type, const char *filename);

// 关闭当前的CDR流
void XMSYS_CdrCloseStream (void);

// CDR流定位
// 返回值
//		-1		打开失败
//		0		打开成功
int XMSYS_CdrSeekStream (unsigned int offset);

// 从CDR流的当前流位置读出指定字节的数据.
// 读出成功后, 流指针指向最新读出的结束位置.
//	返回值
//		-1		读出IO错误
//		0		读出成功
int XMSYS_CdrReadStream (unsigned int offset, unsigned int size);

// 插入Hybrid流
int XMSYS_CdrInsertHybridStream (unsigned int ch, unsigned int type, unsigned int frame_no, char *hybrid_data, unsigned int hybrid_size);

#ifdef __cplusplus
}
#endif

#endif