//****************************************************************************
//
//	Copyright (C) 2013 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: CdrRequest.c
//
//	Revision history
//
//		2013.5.1 ZhuoYongHong Initial version
//
//****************************************************************************
#include "CdrRequest.h"
#include <xm_core.h>
#include <xm_base.h>
#include <stdio.h>
#include "xm_printf.h"
#include "xm_uvc_socket.h"
#include "xm_dev.h"
#include "rtos.h"
#include "xm_file.h"
#include "xm_videoitem.h"
#include "xm_h264_codec.h"
#include "xm_hybrid_stream.h"

extern void dma_memcpy (unsigned char *dst_buf, unsigned char *src_buf, unsigned int len);


#define	UVC_MAX_RESPONSE_DATA		64				// 允许同步传输应答包的最大字节长度

static unsigned int uvc_command = 0xFFFFFFFF;	// 0xFFFFFFFF标记没有命令参数接收
static unsigned char uvc_parameter[255];			// 命令参数, 最大255字节
static unsigned int uvc_parameter_len;				// 命令参数字节长度
static unsigned int uvc_parameter_received;		// 已接收到命令参数

// CDR私有协议处理 
// 命令码CID(1字节, 0x80 ~ 0xEF, 扩展命令标识)， 扩展命令参数长度LEN(1字节), 

// 命令码CID(1字节, 0xF0, 扩展命令参数标识), 扩展命令参数PAR(1字节)
// 命令码CID(1字节, 0xF1, 扩展命令结束标识), 扩展命令参数PAR(1字节)

// 应答包协议定义 (仅用于同步应答包)
// 类似于命令包, 有应答包(2字节)及扩展应答包(变长)
// 扩展应答包数据标识 0xF2
// 扩展应答包结束标识 0xF3

static unsigned int uvc_response_command;			// 应答的命令
static unsigned char uvc_response_data[UVC_MAX_RESPONSE_DATA];		// 应答包数据
static unsigned int uvc_response_len;				// 应答包数据字节长度
static unsigned int uvc_response_left_to_transfer;	// 尚未传输的应答包数据字节长度


// 异步消息
#if HONGJING_CVBS
#define	MAX_ASYC_MESSAGE		0x40000
#else
#define	MAX_ASYC_MESSAGE		0x80000
#endif
#define	MAX_ASYC_COUNT			16							// 允许同时传输的异步消息包个数

static int	asyn_message_count = 0;						// 已写入的异步消息包的个数
#pragma data_alignment=2048
__no_init static unsigned char asyn_message_buffer[MAX_ASYC_MESSAGE];	// 最大512KB异步缓冲区
static unsigned int asyc_message_length;				// 已写入的异步消息字节大小
static OS_RSEMA AsycMessageSema;				// 异步消息访问互斥
static unsigned int asyc_frame_index = 0;		// uvc扩展帧序号

// 流文件(仅允许一个流)
static unsigned int asyn_stream_length;		// 流字节长度
static unsigned int asyn_stream_offset;		// 流当前读取偏移

#if HONGJING_CVBS
#define	PREFETCH_SIZE		0x40000
#else
#define	PREFETCH_SIZE		0x20000
#endif
static unsigned int asyn_stream_prefetch_offset;		// 预取偏移
static unsigned int asyn_stream_prefetch_length;		// 预取大小
static unsigned int asyn_stream_prefetch_size;			// 已预取的有效字节
#pragma data_alignment=2048
__no_init static unsigned char asyn_stream_prefetch_cache[PREFETCH_SIZE];

static void *asyn_stream_handle;					// 流文件句柄
static unsigned char asyn_stream_channel;		// 流通道号
static unsigned char asyn_stream_type;			// 流类型
static unsigned char asyn_stream_name[16];	// 流文件名




void XMSYS_CdrPrivateProtocolHandler (unsigned char protocol_type, unsigned char protocol_data)
{
	if(protocol_type & 0x80)
	{
		if(protocol_type >= 0xF0)		// 协议流程控制命令
		{
			if(protocol_type == 0xF0 || protocol_type == 0xF2)	
			{
				// 命令码CID(1字节, 0xF0, 扩展命令参数标识), 扩展命令参数PAR(1字节)
				// 命令码CID(1字节, 0xF2, 扩展命令参数标识PAR), 扩展命令参数PAR(1字节 = PAR + 0x80)
				if(uvc_command == 0xFFFFFFFF)
				{
					XM_printf ("UVC paramter discard\n");
					return;
				}
				if(uvc_parameter_received >= uvc_parameter_len)
				{
					XM_printf ("UVC command(0x%02x) discard, the size of UVC paramter exceed %d bytes\n", uvc_command, uvc_parameter_len);
					uvc_command = 0xFFFFFFFF;	// 标记命令无效
					return;
				}
				if(protocol_type == 0xF2)
					protocol_data = (unsigned char)(protocol_data + 0x80);
				uvc_parameter[uvc_parameter_received ++] = protocol_data;
				return;
			}
			else if(protocol_type == 0xF1 || protocol_type == 0xF3)
			{
				// 命令码CID(1字节, 0xF1, 扩展命令结束标识), 扩展命令参数PAR(1字节)
				// 命令码CID(1字节, 0xF3, 扩展命令结束标识EOF), 扩展命令参数PAR(1字节 = PAR + 0x80)
				if(uvc_command == 0xFFFFFFFF)
				{
					XM_printf ("UVC eof command discard, no command defined\n");
					return;
				}
				if(uvc_parameter_received >= uvc_parameter_len)
				{
					XM_printf ("UVC command(0x%02x) discard, the size of received paramter exceed %d bytes\n", uvc_command, uvc_parameter_len);
					uvc_command = 0xFFFFFFFF;	// 标记命令无效
					return;
				}
				if(protocol_type == 0xF3)
					protocol_data = (unsigned char)(protocol_data + 0x80);
				uvc_parameter[uvc_parameter_received ++] = protocol_data;
				
				if(uvc_parameter_received != uvc_parameter_len)
				{
					XM_printf ("UVC command(0x%02x) discard, the size of received parameter (%d) != %d bytes\n", uvc_command, uvc_parameter_received, uvc_parameter_len);
					uvc_command = 0xFFFFFFFF;	// 标记命令无效
					return;					
				}
				
				// 扩展命令参数已全部接收完毕
			}
		}
		else									// UVC命令 (0x80 ~ 0xEF)
		{
			// 命令码CID(1字节, 0x80 ~ 0xEF, 扩展命令标识)， 扩展命令参数长度LEN(1字节), 
			if(uvc_command != 0xFFFFFFFF)
			{
				// 已接收的命令尚未处理
				XM_printf ("UVC command(0x%02x) discard, cpu busy to do this\n", uvc_command);
			}
			uvc_command = (unsigned char)(protocol_type - 0x80);
			uvc_parameter_len = protocol_data;		// 该命令待传的参数字节长度
			uvc_parameter_received = 0;		// 已接收的命令参数字节长度
			return;
		}
	}
	else
	{
		// 接收到新的命令
		if(uvc_command != 0xFFFFFFFF)
		{
			// 已接收的命令尚未处理
			XM_printf ("UVC command(0x%02x) discard, cpu busy to do this\n", uvc_command);
		}
		uvc_parameter_len = 1;
		uvc_parameter_received = 0;
		uvc_parameter[0] = protocol_data;
		uvc_command = protocol_type;
	}
	
	// 设置缺省的应答包数据
	uvc_response_command = 0xFFFFFFFF;		// 表示无效
	uvc_response_len = 0;
	
	XMSYS_UvcSocketReceiveCommandPacket (uvc_command, uvc_parameter, uvc_parameter_len);
	
	uvc_command = 0xFFFFFFFF;
}

void XMSYS_CdrPrivateProtocolProcess (void)
{
	unsigned char _uvc_parameter[255];
	unsigned int _uvc_command;
	unsigned int _uvc_parameter_len;
	
	_uvc_command = 0xFFFFFFFF;
	XM_lock();
	if(uvc_command != 0xFFFFFFFF && uvc_parameter_len == uvc_parameter_received)
	{
		_uvc_command = uvc_command;
		_uvc_parameter_len = uvc_parameter_len;
		if(_uvc_parameter_len)
			memcpy (_uvc_parameter, uvc_parameter, uvc_parameter_len);
		
		// 标记命令已处理
		uvc_command = 0xFFFFFFFF;
	}
	XM_unlock ();
	
	if(_uvc_command != 0xFFFFFFFF)
	{
		XMSYS_UvcSocketReceiveCommandPacket (_uvc_command, _uvc_parameter, _uvc_parameter_len);
	}
	
}

// XMSYS_CdrSendResponse 与 XMSYS_CdrGetResponseFrame 在同一个线程中调用, 因此不需要互斥保护

int XMSYS_CdrSendResponse (unsigned int packet_type, unsigned char command_id, 
									unsigned char *response_data_buffer, unsigned int response_data_length)
{
	if(packet_type == XMSYS_UVC_SOCKET_TYPE_ID_SYNC)
	{
		// 同步应答包
		// 通过UVC命令通道传送
		
		// 检查是否存在上次未传送完毕的命令应答包或者扩展应答包
		if(uvc_response_len)
		{
			XM_printf ("UVC warning, last uvc response package can't finish to transfer, cmd = 0x%02x, len = %d, left_to_transfer = %d\n", 
						  uvc_response_command, uvc_response_len, uvc_response_left_to_transfer);
		}
		
		// 应答包的数据长度至少一个字节
		if(response_data_length == 0 || response_data_buffer == NULL)
		{
			XM_printf ("UVC Error, the size of UVC response data is 0\n");
			return -1;
		}
		// "同步应答包"的数据长度有限制, 避免大量数据传输时导致的协议延迟
		// 大数据传输使用"异步消息包"
		if(response_data_length > UVC_MAX_RESPONSE_DATA)
		{
			XM_printf ("UVC Error, the size (%d) of UVC response data exceed maximum %d\n", response_data_length, UVC_MAX_RESPONSE_DATA);
			return -1;
		}
		// 应答的命令码RID(1字节, 0x00 ~ 0x6F, RID=CID)
		if(command_id >= 0x70)	// 保留值
		{
			XM_printf ("UVC Error, illegal command id(0x%02x) in response package\n", command_id);
			return -1;
		}
		
		// 检查应答包长度是否1字节. 若是, 判断其数据是否小于0x80. 大于0x80的数据在"命令应答包"中禁止.
		if(response_data_length == 1 && response_data_buffer[0] >= 0x80)
		{
			XM_printf ("UVC Error, illegal data (0x%02x) >= 0x80 in one byte response package\n", response_data_buffer[0]);
			return -1;
		}
		
		//XM_printf ("CdrSendResponse cid=0x%02x, len=%d\n", command_id, response_data_length);
			
		uvc_response_command = command_id;
		uvc_response_len = response_data_length;
		uvc_response_left_to_transfer = response_data_length;
		if(response_data_length)
		{
			memcpy (uvc_response_data, response_data_buffer, response_data_length);
		}
		return 0;
	}
	else if(packet_type == XMSYS_UVC_SOCKET_TYPE_ID_ASYC)
	{
		// 异步消息包
		// 通过UVC数据通道传送
		return XMSYS_CdrInsertAsycMessage (command_id, (char *)response_data_buffer, response_data_length);
	}
	return -1;
}

// 读取命令应答帧或者扩展应答帧
// 每个应答帧由2个字节构成
// 命令应答包由一个应答帧构成
// 扩展应答包由至少2个应答帧构成
unsigned short XMSYS_CdrGetResponseFrame (void)
{
	unsigned int response_frame = 0;
	if(uvc_response_len == 0)
	{
		// 无应答包, 返回异常码
		response_frame = 0x00FF;			// 返回表示异常的应答包
	}
	else if(uvc_response_len == 1)
	{
		// 应答包数据长度为一个字节的使用"命令应答包"流程传输应答包
		// "命令应答包", 由一个命令应答帧构成
		// 应答的命令码RID(1字节, 0x00 ~ 0x7F, RID=CID)，应答的数据(1字节).
		
		response_frame = uvc_response_command | (uvc_response_data[0] << 8);
		
		uvc_response_len = 0;		// 标记命令应答包已传输完成
	}
	else
	{
		// 应答包数据长度大于或等于2字节的使用"扩展应答包"流程传输应答包
		// "扩展应答包", 由1个"扩展应答包命令帧", 至少1个"扩展应答包数据帧"及一个"扩展应答包结束帧"构成
		
		if(uvc_response_len == uvc_response_left_to_transfer)
		{
			// 检查是传输扩展应答包的"命令帧"还是第一个"数据帧"
			if(uvc_response_command != 0xFFFFFFFF)
			{
				// 扩展应答包的第一个扩展应答包命令帧
				// 扩展应答包命令帧 命令标识RID(1字节, 0x80 ~ 0xEF, RID=CID+0x80)，扩展应答包数据长度LEN(1字节),
				response_frame = (uvc_response_command + 0x80) | (uvc_response_len << 8);
				uvc_response_command = 0xFFFFFFFF;		// 表示"命令帧"已发送
			}
			else 
			{
				// 扩展应答包的第一个扩展应答包数据帧
				unsigned int data = uvc_response_data[0];
				if(data >= 0x80)
				{
					// 扩展应答包数据帧 数据标识DID(1字节, 0xF4), 扩展应答包数据DAT(1字节 = DAT + 0x80).
					response_frame = 0xF4 | ((data - 0x80) << 8);
				}
				else
				{
					// 扩展应答包数据帧 数据标识DID(1字节, 0xF2), 扩展应答包数据DAT(1字节).
					response_frame = 0xF2 | (data << 8);	
				}
				uvc_response_left_to_transfer --;
			}
		}
		else if(uvc_response_left_to_transfer != 1)
		{
			// 中间的扩展应答包的"数据帧"
			unsigned int data = uvc_response_data[uvc_response_len - uvc_response_left_to_transfer];
			if(data >= 0x80)
			{
				// 扩展应答包数据帧 数据标识DID(1字节, 0xF4), 扩展应答包数据DAT(1字节 = DAT + 0x80).
				response_frame = 0xF4 | ((data - 0x80) << 8);
			}
			else
			{
				// 扩展应答包数据帧 数据标识DID(1字节, 0xF2), 扩展应答包数据DAT(1字节).
				response_frame = 0xF2 | (data << 8);
			}
			uvc_response_left_to_transfer --;
		}
		else
		{
			// 扩展应答包的"结束帧"
			unsigned int data = uvc_response_data[uvc_response_len - uvc_response_left_to_transfer];
			if(data >= 0x80)
			{
				// 扩展应答包结束帧 数据标识DID(1字节, 0xF5), 扩展应答包数据DAT(1字节 = DAT + 0x80).
				response_frame = 0xF5 | ((data - 0x80) << 8);
			}
			else
			{
				// 扩展应答包结束帧 结束标识EID(1字节, 0xF3), 扩展应答包数据DAT(1字节).
				response_frame = 0xF3 | (data << 8);
			}
			uvc_response_left_to_transfer --;
			
			uvc_response_len = 0;	// 标记扩展应答包已传输完成
		}
	}
	
	//printf ("UVC response_frame=0x%04x\n", response_frame);
	
	return (unsigned short)response_frame;
}

// 将一个异步消息帧加入到CDR帧中
// lpCdrBuffer		CDR帧缓冲区地址
// cbCdrBuffer		CDR帧缓冲区字节大小
// lpExtendMessage	待传的扩展消息数据缓冲区
// cbExtendMessage	待传的扩展消息字节长度	

void XMSYS_CdrInit (void)
{
	asyn_message_count = 0;
	asyc_message_length = 0;
	asyc_frame_index = 0;
	
	asyn_stream_length = 0;
	asyn_stream_offset = 0;
	asyn_stream_handle = 0;
	
	XM_HybridStreamInit ();
	
	OS_CREATERSEMA (&AsycMessageSema); /* Creates resource semaphore */
}


static unsigned int checksum_int(unsigned int *buf, int len)
{
	register int counter;
	register unsigned int checksum = 0;
	for( counter = 0; counter < len; counter++)
		checksum += *(unsigned int *)buf++;

	return ((unsigned int)(checksum));
}

// 将一个异步消息包加入到UVC扩展协议
//	AsycMessage		异步消息ID
//	lpAsycMessage	异步消息数据
//	cbAsycMessage	
// 返回值
//		-1			失败
//		0			成功
int XMSYS_CdrInsertAsycMessage (unsigned char AsycMessage, char *lpAsycMessage, int cbAsycMessage)
{
	int ret = -1;
	unsigned int dummy_size = (4 - (cbAsycMessage & 3)) & 3;
	unsigned char *message;
	
	// 检查UVC socket是否打开
	if(!XMSYS_CameraIsReady())
		return -1;
				
	OS_Use (&AsycMessageSema);
	do
	{
		if(asyn_message_count >= MAX_ASYC_COUNT)
		{
			XM_printf ("CdrInsertAsycMessage Failed, message (0x%02x) discard due to maximum asyc count\n", AsycMessage);
			break;
		}
		if(cbAsycMessage && lpAsycMessage == NULL)
		{
			XM_printf ("CdrInsertAsycMessage Failed, message (0x%02x) discard due to parameter error\n", AsycMessage);
			break;
		}
		
		// 异步消息包的控制字段为12字节长度
		// 0xFE, 0x00, NID(1字节)，FILL(1字节, 填充字段长度, 确保所有的异步消息包长度均为4字节对齐)
		// 异步消息包数据字节长度LEN(4字节)，异步消息包数据DAT + FILL(4字节填充)字段, CHECKSUM校验(4字节).
		// CHECKSUM = 所有字段按32位正整数累加取反		
		if( (cbAsycMessage + 12 + dummy_size) > MAX_ASYC_MESSAGE )
		{
			XM_printf ("CdrInsertAsycMessage Failed, message (0x%02x) discard due to resource busy\n", AsycMessage);
			break;			
		}
		
		message = asyn_message_buffer + asyc_message_length;
		*message ++ = 0xFE;
		*message ++ = 0x00;
		*message ++ = AsycMessage;
		*message ++ = (unsigned char)dummy_size;
		*(unsigned int *)message = cbAsycMessage;
		message += 4;
		if(cbAsycMessage)
		{
			memcpy (message, lpAsycMessage, cbAsycMessage);
			message += cbAsycMessage;
		}
		// 填充字段
		for (int i = 0; i < dummy_size; i ++)
		{
			*message ++ = 0;
		}
		unsigned int count = message - (asyn_message_buffer + asyc_message_length);
#ifdef CDR_CHECKSUM
		unsigned int checksum = checksum_int ((unsigned int *)(asyn_message_buffer + asyc_message_length), count / 4);
#else
		unsigned int checksum = 0;
#endif
		*(unsigned int *)message = ~checksum;
		message += 4;
		
		asyn_message_count ++;
		asyc_message_length += count + 4;
		
		ret = 0;
		
	} while (0);	
	OS_Unuse (&AsycMessageSema);
	
	return ret;
}


static const char head_id[] = {0xFF, 0xFF, 0x00, 0x00, 'X', 'M', 'C', 'D'};
static const char tail_id[] = {0xFF, 0x00, 0xFF, 0x00, 'X', 'M', 'C', 'D'};

static unsigned int calc_32bit_checksum ( unsigned char *lpCdrBuffer, int cbCdrBuffer )
{
	unsigned int checksum = 0;
	while(cbCdrBuffer >= 4)
	{
		checksum += lpCdrBuffer[0] | (lpCdrBuffer[1] << 8) | (lpCdrBuffer[2] << 16) | (lpCdrBuffer[3] << 24);
		cbCdrBuffer -= 4;
		lpCdrBuffer += 4;
	}
	if(cbCdrBuffer == 3)
	{
		checksum += lpCdrBuffer[0] | (lpCdrBuffer[1] << 8) | (lpCdrBuffer[2] << 16);
	}
	else if(cbCdrBuffer == 2)
	{
		checksum += lpCdrBuffer[0] | (lpCdrBuffer[1] << 8);
	}
	else if(cbCdrBuffer == 1)
	{
		checksum += lpCdrBuffer[0];
	}
	return checksum;
}

// 1) UVC扩展"头标志字段"  (8字节, 0xFF, 0xFF, 0x00, 0x00, 'X' 'M' 'C', 'D'), 32字节边界对齐.
// 2) UVC扩展"序号字段" (4字节), 每个帧具有唯一序号, 序号从0开始累加. 通过帧序号, 判断帧是否重发帧
// 3) UVC扩展"信息字段" (16字节)
// 4) UVC扩展"数据字段"(变长字段)
// 5) UVC扩展"数据长度字段"(4字节)
// 6) UVC扩展"尾标志字段"  (8字节, 0xFF, 0x00, 0xFF, 0x00, 'X' 'M' 'C', 'D'), 32字节边界对齐.
int XMSYS_JpegFrameInsertExtendMessage (unsigned char uvc_channel, char *lpCdrBuffer, int cbCdrBuffer)
{
	int ret = -1;
	int packing_bytes;
	unsigned int data_size;
	int size = cbCdrBuffer;
	
	char *lpUvcExtInfo;
	int cbUvcExtInfo;
	unsigned int *lpUvcCheckSum;
	
	//return 0;
	
	OS_Use (&AsycMessageSema);		
	do 
	{
		/*
		if(asyn_message_count == 0)
		{
			ret = 0;
			break;
		}*/
		
		// UVC扩展"头标志字段" 32字节对齐
		// 填充字节
		packing_bytes = (0x20 - (((unsigned int)(lpCdrBuffer)) & 0x1F)) & (0x1F);
		
		// 减去对齐所需要的填充字节
		//lpCdrBuffer += packing_bytes;
		//cbCdrBuffer -= packing_bytes;
		for (int i = 0; i < packing_bytes; i ++)
		{
			*lpCdrBuffer ++ = 0;
			cbCdrBuffer --;
		}
		
		// UVC扩展字段开始地址
		lpUvcExtInfo = lpCdrBuffer;
		
		// 8字节扩展"头标志字段"
		memcpy (lpCdrBuffer, head_id, 8);
		lpCdrBuffer += 8;
		cbCdrBuffer -= 8;
		
		// 2) UVC扩展"序号字段" (4字节), 每个帧具有唯一序号, 序号从0开始累加. 通过帧序号, 判断帧是否重发帧
		*(unsigned int *)lpCdrBuffer = asyc_frame_index;
		asyc_frame_index ++;
		lpCdrBuffer += 4;
		cbCdrBuffer -= 4;
		
		// 3) UVC扩展"信息字段" (16字节)
		// unsigned int  uvc_checksum;		// UVC数据的CheckSum字段, 32位数据的累加和, 可以使用该字段用作UVC Payload数据的检查
		*(unsigned int *)lpCdrBuffer = 0;		// 
		lpUvcCheckSum = (unsigned int *)lpCdrBuffer;
		lpCdrBuffer += 4;
		cbCdrBuffer -= 4;
		// unsigned char uvc_channel;			// UVC数据通道号,
		*lpCdrBuffer ++ = uvc_channel;
		cbCdrBuffer -= 1;
		// unsigned char card_state;		// 卡状态
		// *lpCdrBuffer ++ = 0;
		*lpCdrBuffer ++ = (unsigned char)XMSYS_UvcSocketGetCardState ();
		cbCdrBuffer -= 1;
		// unsigned char asyn_message_count;	// 异步消息包的数量, 允许多条异步消息同时传送
		*lpCdrBuffer ++ =	asyn_message_count;
		cbCdrBuffer -= 1;
		// unsigned char rev[9];			// 保留
		memset (lpCdrBuffer, 0, 9);
		lpCdrBuffer += 9;
		cbCdrBuffer -= 9;
		
		// 4) UVC扩展"数据字段"(变长字段)
		data_size = (asyc_message_length + 0x1F) & (~0x1F);
		if(data_size > cbCdrBuffer)
		{
			XM_printf ("uvc asyc buffer overflow\n");
			break;
		}
		
		if(asyc_message_length && asyn_message_count)
		{
			//XM_printf ("asyn message, msg count = %d\n", asyn_message_count);
		}
						  
		if(asyc_message_length)
		{
			memcpy (lpCdrBuffer, asyn_message_buffer, asyc_message_length);
			if(data_size != asyc_message_length)		// 对齐填充字段
			{
				// 填充0
				memset (lpCdrBuffer + asyc_message_length, 0, data_size - asyc_message_length);
			}
		}
		lpCdrBuffer += data_size;
		cbCdrBuffer -= data_size;
		
		if(cbCdrBuffer < 12)
		{
			XM_printf ("uvc asyc buffer overflow\n");
			break;
		}
		// 5) UVC扩展"数据长度字段"(4字节)
		*(unsigned int *)lpCdrBuffer = data_size;
		lpCdrBuffer += 4;
		cbCdrBuffer -= 4;
		
		// 6) UVC扩展"尾标志字段"  (8字节, 0xFF, 0x00, 0xFF, 0x00, 'X' 'M' 'C', 'D'), 32字节边界对齐.
		memcpy (lpCdrBuffer, tail_id, 8);
		lpCdrBuffer += 8;
		cbCdrBuffer -= 8;
		
		ret = size - cbCdrBuffer;
		
		// 统计UVC扩展信息字段的长度
		cbUvcExtInfo = lpCdrBuffer - lpUvcExtInfo;

		// 统计UVC扩展信息字段的32bit checksum
		*(unsigned int *)lpUvcCheckSum = calc_32bit_checksum ((unsigned char *)lpUvcExtInfo, cbUvcExtInfo);	

		//XM_printf ("frame number = %d, data_size = %d\n", asyc_frame_index - 1, data_size);
		
	} while (0);
	
	// 异步消息已全部写入到UVC包中
	// 将异步消息数据全部清除
	asyn_message_count = 0;
	asyc_message_length = 0;
	OS_Unuse (&AsycMessageSema);
	
	return ret;
}

// 打开CDR流文件
// 返回值
//		-1		打开失败
//		0		打开成功
int XMSYS_CdrOpenStream (unsigned char ch, unsigned char type, const char *filename)
{
	char file_path[32];
	int ret = -1;
	int mark_usage = 0;
	
	OS_Use (&AsycMessageSema);	
	
	asyn_stream_prefetch_length = 0;
	asyn_stream_prefetch_size = 0;

	do 
	{
		// 检查流是否已开启. 若存在, 关闭当前的流
		if(asyn_stream_handle)
		{
			XMSYS_CdrCloseStream ();
		}
		
		memset (file_path, 0, sizeof(file_path));
		combine_fullpath_media_filename ((unsigned char *)file_path, sizeof(file_path), ch, type, (const unsigned char *)filename);

		if(type == 0)
		{
			// 视频格式文件
			// 标记流文件的"使用中锁定"标志, 防止被替换删除 
			mark_usage = XM_VideoItemMarkVideoFileUsage (file_path);
			if(!mark_usage)
			{
				XM_printf ("XMSYS_CdrOpenStream failed, can't mark usage(%s)\n", file_path);
				break;
			}
		}
		
		asyn_stream_handle = XM_fopen (file_path, "rb");
		if(asyn_stream_handle == NULL)
		{
			XM_printf ("XMSYS_CdrOpenStream failed, can't open stream(%s)\n", file_path);
			break;
		}
		
		asyn_stream_length = XM_filesize (asyn_stream_handle);
		if(asyn_stream_length == 0)
		{
			XM_fclose (asyn_stream_handle);
			asyn_stream_handle = NULL;
			XM_printf ("XMSYS_CdrOpenStream failed, illegal file size, can't open stream(%s)\n", file_path);
			break;			
		}
		
		XM_printf ("XMSYS_CdrOpenStream (%s) success\n", file_path);
		
		asyn_stream_offset = 0;
		asyn_stream_channel = ch;
		asyn_stream_type = type;
		memset (asyn_stream_name, 0, sizeof(asyn_stream_name));
		memcpy (asyn_stream_name, filename, 8);
		
		// 返回流字节长度
		ret = asyn_stream_length;
		
	} while (0);
	
	if(ret == (-1))
	{
		// 失败
		if(type == 0 && mark_usage)
		{
			// 解除视频格式文件的"使用中锁定"标志
			XM_VideoItemMarkVideoFileUnuse (file_path);
		}
	}
		
	OS_Unuse (&AsycMessageSema);
	return ret;
}


// 关闭当前的CDR流
void XMSYS_CdrCloseStream (void)
{
	char file_path[32];
	OS_Use (&AsycMessageSema);	
	asyn_stream_prefetch_length = 0;
	asyn_stream_prefetch_size = 0;
	
	if(asyn_stream_handle)
	{
		// 全路径文件名
		memset (file_path, 0, sizeof(file_path));
		combine_fullpath_media_filename ((unsigned char *)file_path, sizeof(file_path), asyn_stream_channel, asyn_stream_type, asyn_stream_name);
		
		XM_fclose (asyn_stream_handle);
		if(asyn_stream_type == 0)
		{
			// 视频类型, 解除"使用中锁定"标志
			XM_VideoItemMarkVideoFileUnuse (file_path);
		}
		asyn_stream_handle = NULL;
		asyn_stream_length = 0;
		asyn_stream_offset = 0;
	}
	
	OS_Unuse (&AsycMessageSema);
}

// CDR流定位
// 返回值
//		-1		打开失败
//		0		打开成功
int XMSYS_CdrSeekStream (unsigned int offset)
{
	int ret = -1;
	OS_Use (&AsycMessageSema);	
	
	asyn_stream_prefetch_length = 0;
	asyn_stream_prefetch_size = 0;
	
	do
	{
		if(asyn_stream_handle == NULL)
		{
			XM_printf ("XMSYS_CdrSeekStream failed, stream closed\n");
			break;
		}
		if(offset >= asyn_stream_length)
		{
			XM_printf ("XMSYS_CdrSeekStream failed, offset(%d) >= size(%d)\n", offset, asyn_stream_length);
			break;
		}
		
		asyn_stream_offset = offset;
		
		ret = 0;
	} while (0);
	
	OS_Unuse (&AsycMessageSema);
	
	return ret;
}

// 从CDR流的当前流位置读出指定字节的数据.
// 读出成功后, 流指针指向最新读出的结束位置.
//	返回值
//		-1		读出IO错误
//		0		读出成功
int XMSYS_CdrReadStream (unsigned int offset, unsigned int size)
{
	int ret = -1;
	unsigned int to_read;
	unsigned int dummy_size;
	unsigned char *message;
	
	OS_Use (&AsycMessageSema);	
	asyn_stream_offset = offset;
	//XM_printf ("CdrRead off = 0x%08x, len = 0x%08x\n", asyn_stream_offset, size);
	do
	{
		if(asyn_stream_handle == NULL)
		{
			XM_printf ("XMSYS_CdrReadStream failed, stream closed\n");
			break;
		}
		if(asyn_stream_offset >= asyn_stream_length)
		{
			ret = 0;
			break;
		}
		// 检查是否超出文件长度
		if( (size + asyn_stream_offset) > asyn_stream_length )
			size = asyn_stream_length - asyn_stream_offset;
		
		// 计算异步消息包的剩余空间
		
		// 12是异步消息帧的控制信息字节长度
		// "0xFE(1字节), 0x00(1字节), NID(1字节)，FILL(1字节, 4字节对齐填充字节长度)

		//  异步消息包数据字节长度LEN(4字节)，
		//  异步消息包数据DAT, 填充字段(补齐为4字节对齐), 
		// CHECKSUM校验(4字节), = CHECKSUM之前的所有字段按4字节累加

		//	20字节是文件流帧的属性
		// 	CH(通道，1字节),TYPE(类型，1字节), rev(保留, 固定为0, 2字节), 
		//		Filename (8字节文件名), offset (文件偏移, 4字节), size (文件流字节长度), 4字节, 流数据(size字节长度)
		//
		// 4 dummy
		if( (size + 12 + 20 + 4) > (MAX_ASYC_MESSAGE - asyc_message_length) )
		{
			// 读出的长度超出UVC异步消息包的剩余空间
			size = (MAX_ASYC_MESSAGE - asyc_message_length) - 12 - 20 - 4;
		}
		
		to_read = size;					
		if(to_read == 0)
		{
			ret = 0;
			break;
		}
		
		dummy_size = (4 - (to_read & 3)) & 3;
		
		// 嵌入异步消息帧"文件流"到异步消息包中
		message = asyn_message_buffer + asyc_message_length;
		
		// 检查预取是否存在
		if(asyn_stream_prefetch_size == to_read && asyn_stream_offset == asyn_stream_prefetch_offset)
		{
			// 预取存在且偏移/大小匹配
			memcpy (message + 8 + 20, asyn_stream_prefetch_cache, to_read);
		}
		else
		{
			asyn_stream_prefetch_size = 0;
			asyn_stream_prefetch_length = 0;
			
			// 流定位后禁止继续上传, 等待read指令	
			if(XM_fseek (asyn_stream_handle, asyn_stream_offset, XM_SEEK_SET) < 0)
			{
				XM_printf ("XMSYS_CdrSeekStream failed, fseek io error\n");
				break;
			}
			
			if(XM_fread (message + 8 + 20, 1, to_read, asyn_stream_handle) != to_read)
			{
				XM_printf ("XMSYS_CdrReadStream failed, fread io error\n");
				break;
			}
		}
		
		// 标记下一次预取的数据
		if( (asyn_stream_offset + to_read) < asyn_stream_length && to_read <= PREFETCH_SIZE)
		{
			asyn_stream_prefetch_offset = asyn_stream_offset + to_read;
			asyn_stream_prefetch_length = to_read;
			asyn_stream_prefetch_size = 0;
		}
		else
		{
			asyn_stream_prefetch_length = 0;
			asyn_stream_prefetch_size = 0;
		}
		
		// 
		*message ++ = 0xFE;
		*message ++ = 0x00;
		*message ++ = XMSYS_UVC_NID_DOWNLOAD;
		*message ++ = (unsigned char)dummy_size;
		*(unsigned int *)message = to_read + 20;
		message += 4;
		// "文件流"帧的属性字段
		*message ++ = asyn_stream_channel;	// 1字节通道号
		*message ++ = asyn_stream_type;		// 1字节流类型
		*message ++ = 0;		// 2字节保留, 填充0
		*message ++ = 0;
		memcpy (message, asyn_stream_name, 8);	// 8字节文件名
		message += 8;
		*(unsigned int *)message = asyn_stream_offset; // offset(文件偏移, 4字节)
		message += 4;
		*(unsigned int *)message = to_read;	// size(文件流字节长度, 4字节),
		message += 4;
		
		// 修改文件偏移值
		asyn_stream_offset = asyn_stream_offset + to_read;
		
		// 计算checksum
		unsigned int checksum = checksum_int ((unsigned int *)(message - 8 - 20), (to_read + dummy_size + 8 + 20)/4);
		message += to_read + dummy_size;
		*(unsigned int *)message = ~checksum;
		message += 4;
		
		asyn_message_count ++;
		asyc_message_length = message - asyn_message_buffer;
		
		ret = 0;
	} while (0);
	
	OS_Unuse (&AsycMessageSema);
	
	return ret;
	
}

// CDR文件预取(与USB传输并发执行)
void XMSYS_CdrFilePrefetch (void)
{
	// 录像模式下禁止预取, 避免H264码流写入阻塞
	if(XMSYS_H264CodecGetMode() == XMSYS_H264_CODEC_MODE_RECORD)
		return;
	
	OS_Use (&AsycMessageSema);	
	do
	{
		if(asyn_stream_prefetch_length && asyn_stream_handle)
		{
			if(XM_fseek (asyn_stream_handle, asyn_stream_prefetch_offset, XM_SEEK_SET) < 0)
			{
				XM_printf ("XMSYS_CdrLoadFilePrefetch failed, fseek io error\n");
				asyn_stream_prefetch_length = 0;
				asyn_stream_prefetch_size = 0;
				break;
			}
			
			asyn_stream_prefetch_size = XM_fread (asyn_stream_prefetch_cache, 1, asyn_stream_prefetch_length, asyn_stream_handle);
		}
	} while(0);
	OS_Unuse (&AsycMessageSema);
}

// 插入Hybrid流
int XMSYS_CdrInsertHybridStream (unsigned int ch, unsigned int type, unsigned int frame_no, char *hybrid_data, unsigned int hybrid_size)
{
	int ret = -1;
	unsigned int dummy_size;
	unsigned char *message;
	if(ch >= XM_HYBRID_CH_COUNT)
	{
		XM_printf ("XMSYS_CdrInsertHybridStream Failed, illegal ch (%d)\n", ch);
		return -1;
	}
	if(type >= XM_HYBRID_TYPE_COUNT)
	{
		XM_printf ("XMSYS_CdrInsertHybridStream Failed, illegal type (%d)\n", type);
		return -1;
	}
	if(hybrid_data == NULL || hybrid_size == 0)
	{
		return -1;
	}
	OS_Use (&AsycMessageSema);	
	do
	{
		// 检查混合流字节长度是否超出当前帧
						// CH(通道，1字节), TYPE(类型，1字节), rev(保留, 固定为0, 2字节), NO(帧序号, 4字节),
						//		size(hubrid流字节长度, 4字节), 流数据(size字节长度, 4字节对齐)
						//	CH   --> 0x00 前置摄像头, 0x01 后置摄像头, 0x02 音频数据.
						//	TYPE --> 0x00 I帧, 0x01 P帧, 0x02 8K采样单声道PCM音频数据.
						//	NO   --> 帧序号, 每个通道具有独立的序号, 序号递增, 主机端根据帧号用来判断是否丢帧.
						//	SIZE --> hubrid流字节长度
		//	20字节是文件流帧的属性
		// 4 dummy
		if( (hybrid_size + 12 + 16 + 4) > (MAX_ASYC_MESSAGE - asyc_message_length) )
		{
			// 异常
			break;
		}
		
		dummy_size = (4 - (hybrid_size & 3)) & 3;
		// 嵌入异步消息帧"文件流"到异步消息包中
		message = asyn_message_buffer + asyc_message_length;
		
		*message ++ = 0xFE;
		*message ++ = 0x00;
		*message ++ = XMSYS_UVC_NID_HYBRID_STREAM;		// hybrid stream id
		*message ++ = (unsigned char)dummy_size;
		*(unsigned int *)message = hybrid_size + 12;
		message += 4;
		// "hybrid流"属性字段
		*message ++ = (unsigned char)ch;	// 1字节通道号
		*message ++ = (unsigned char)type;
		*message ++ = 0;
		*message ++ = 0;
		*(unsigned int *)message = frame_no;
		message += 4;
		*(unsigned int *)message = hybrid_size;
		message += 4;
		// 复制hybrid流数据到异步流
		dma_memcpy (message, (unsigned char *)hybrid_data, hybrid_size + dummy_size);
		// 计算checksum
		unsigned int checksum = checksum_int ((unsigned int *)(message - 8 - 12), (hybrid_size + dummy_size + 8 + 12)/4);
		message += hybrid_size + dummy_size;
		*(unsigned int *)message = ~checksum;
		message += 4;

		asyn_message_count ++;
		asyc_message_length = message - asyn_message_buffer;
				
		ret = 0;
	} while (0);

	OS_Unuse (&AsycMessageSema);	
	return ret;
}

