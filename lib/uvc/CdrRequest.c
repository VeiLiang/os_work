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


#define	UVC_MAX_RESPONSE_DATA		64				// ����ͬ������Ӧ���������ֽڳ���

static unsigned int uvc_command = 0xFFFFFFFF;	// 0xFFFFFFFF���û�������������
static unsigned char uvc_parameter[255];			// �������, ���255�ֽ�
static unsigned int uvc_parameter_len;				// ��������ֽڳ���
static unsigned int uvc_parameter_received;		// �ѽ��յ��������

// CDR˽��Э�鴦�� 
// ������CID(1�ֽ�, 0x80 ~ 0xEF, ��չ�����ʶ)�� ��չ�����������LEN(1�ֽ�), 

// ������CID(1�ֽ�, 0xF0, ��չ���������ʶ), ��չ�������PAR(1�ֽ�)
// ������CID(1�ֽ�, 0xF1, ��չ���������ʶ), ��չ�������PAR(1�ֽ�)

// Ӧ���Э�鶨�� (������ͬ��Ӧ���)
// �����������, ��Ӧ���(2�ֽ�)����չӦ���(�䳤)
// ��չӦ������ݱ�ʶ 0xF2
// ��չӦ���������ʶ 0xF3

static unsigned int uvc_response_command;			// Ӧ�������
static unsigned char uvc_response_data[UVC_MAX_RESPONSE_DATA];		// Ӧ�������
static unsigned int uvc_response_len;				// Ӧ��������ֽڳ���
static unsigned int uvc_response_left_to_transfer;	// ��δ�����Ӧ��������ֽڳ���


// �첽��Ϣ
#if HONGJING_CVBS
#define	MAX_ASYC_MESSAGE		0x40000
#else
#define	MAX_ASYC_MESSAGE		0x80000
#endif
#define	MAX_ASYC_COUNT			16							// ����ͬʱ������첽��Ϣ������

static int	asyn_message_count = 0;						// ��д����첽��Ϣ���ĸ���
#pragma data_alignment=2048
__no_init static unsigned char asyn_message_buffer[MAX_ASYC_MESSAGE];	// ���512KB�첽������
static unsigned int asyc_message_length;				// ��д����첽��Ϣ�ֽڴ�С
static OS_RSEMA AsycMessageSema;				// �첽��Ϣ���ʻ���
static unsigned int asyc_frame_index = 0;		// uvc��չ֡���

// ���ļ�(������һ����)
static unsigned int asyn_stream_length;		// ���ֽڳ���
static unsigned int asyn_stream_offset;		// ����ǰ��ȡƫ��

#if HONGJING_CVBS
#define	PREFETCH_SIZE		0x40000
#else
#define	PREFETCH_SIZE		0x20000
#endif
static unsigned int asyn_stream_prefetch_offset;		// Ԥȡƫ��
static unsigned int asyn_stream_prefetch_length;		// Ԥȡ��С
static unsigned int asyn_stream_prefetch_size;			// ��Ԥȡ����Ч�ֽ�
#pragma data_alignment=2048
__no_init static unsigned char asyn_stream_prefetch_cache[PREFETCH_SIZE];

static void *asyn_stream_handle;					// ���ļ����
static unsigned char asyn_stream_channel;		// ��ͨ����
static unsigned char asyn_stream_type;			// ������
static unsigned char asyn_stream_name[16];	// ���ļ���




void XMSYS_CdrPrivateProtocolHandler (unsigned char protocol_type, unsigned char protocol_data)
{
	if(protocol_type & 0x80)
	{
		if(protocol_type >= 0xF0)		// Э�����̿�������
		{
			if(protocol_type == 0xF0 || protocol_type == 0xF2)	
			{
				// ������CID(1�ֽ�, 0xF0, ��չ���������ʶ), ��չ�������PAR(1�ֽ�)
				// ������CID(1�ֽ�, 0xF2, ��չ���������ʶPAR), ��չ�������PAR(1�ֽ� = PAR + 0x80)
				if(uvc_command == 0xFFFFFFFF)
				{
					XM_printf ("UVC paramter discard\n");
					return;
				}
				if(uvc_parameter_received >= uvc_parameter_len)
				{
					XM_printf ("UVC command(0x%02x) discard, the size of UVC paramter exceed %d bytes\n", uvc_command, uvc_parameter_len);
					uvc_command = 0xFFFFFFFF;	// ���������Ч
					return;
				}
				if(protocol_type == 0xF2)
					protocol_data = (unsigned char)(protocol_data + 0x80);
				uvc_parameter[uvc_parameter_received ++] = protocol_data;
				return;
			}
			else if(protocol_type == 0xF1 || protocol_type == 0xF3)
			{
				// ������CID(1�ֽ�, 0xF1, ��չ���������ʶ), ��չ�������PAR(1�ֽ�)
				// ������CID(1�ֽ�, 0xF3, ��չ���������ʶEOF), ��չ�������PAR(1�ֽ� = PAR + 0x80)
				if(uvc_command == 0xFFFFFFFF)
				{
					XM_printf ("UVC eof command discard, no command defined\n");
					return;
				}
				if(uvc_parameter_received >= uvc_parameter_len)
				{
					XM_printf ("UVC command(0x%02x) discard, the size of received paramter exceed %d bytes\n", uvc_command, uvc_parameter_len);
					uvc_command = 0xFFFFFFFF;	// ���������Ч
					return;
				}
				if(protocol_type == 0xF3)
					protocol_data = (unsigned char)(protocol_data + 0x80);
				uvc_parameter[uvc_parameter_received ++] = protocol_data;
				
				if(uvc_parameter_received != uvc_parameter_len)
				{
					XM_printf ("UVC command(0x%02x) discard, the size of received parameter (%d) != %d bytes\n", uvc_command, uvc_parameter_received, uvc_parameter_len);
					uvc_command = 0xFFFFFFFF;	// ���������Ч
					return;					
				}
				
				// ��չ���������ȫ���������
			}
		}
		else									// UVC���� (0x80 ~ 0xEF)
		{
			// ������CID(1�ֽ�, 0x80 ~ 0xEF, ��չ�����ʶ)�� ��չ�����������LEN(1�ֽ�), 
			if(uvc_command != 0xFFFFFFFF)
			{
				// �ѽ��յ�������δ����
				XM_printf ("UVC command(0x%02x) discard, cpu busy to do this\n", uvc_command);
			}
			uvc_command = (unsigned char)(protocol_type - 0x80);
			uvc_parameter_len = protocol_data;		// ����������Ĳ����ֽڳ���
			uvc_parameter_received = 0;		// �ѽ��յ���������ֽڳ���
			return;
		}
	}
	else
	{
		// ���յ��µ�����
		if(uvc_command != 0xFFFFFFFF)
		{
			// �ѽ��յ�������δ����
			XM_printf ("UVC command(0x%02x) discard, cpu busy to do this\n", uvc_command);
		}
		uvc_parameter_len = 1;
		uvc_parameter_received = 0;
		uvc_parameter[0] = protocol_data;
		uvc_command = protocol_type;
	}
	
	// ����ȱʡ��Ӧ�������
	uvc_response_command = 0xFFFFFFFF;		// ��ʾ��Ч
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
		
		// ��������Ѵ���
		uvc_command = 0xFFFFFFFF;
	}
	XM_unlock ();
	
	if(_uvc_command != 0xFFFFFFFF)
	{
		XMSYS_UvcSocketReceiveCommandPacket (_uvc_command, _uvc_parameter, _uvc_parameter_len);
	}
	
}

// XMSYS_CdrSendResponse �� XMSYS_CdrGetResponseFrame ��ͬһ���߳��е���, ��˲���Ҫ���Ᵽ��

int XMSYS_CdrSendResponse (unsigned int packet_type, unsigned char command_id, 
									unsigned char *response_data_buffer, unsigned int response_data_length)
{
	if(packet_type == XMSYS_UVC_SOCKET_TYPE_ID_SYNC)
	{
		// ͬ��Ӧ���
		// ͨ��UVC����ͨ������
		
		// ����Ƿ�����ϴ�δ������ϵ�����Ӧ���������չӦ���
		if(uvc_response_len)
		{
			XM_printf ("UVC warning, last uvc response package can't finish to transfer, cmd = 0x%02x, len = %d, left_to_transfer = %d\n", 
						  uvc_response_command, uvc_response_len, uvc_response_left_to_transfer);
		}
		
		// Ӧ��������ݳ�������һ���ֽ�
		if(response_data_length == 0 || response_data_buffer == NULL)
		{
			XM_printf ("UVC Error, the size of UVC response data is 0\n");
			return -1;
		}
		// "ͬ��Ӧ���"�����ݳ���������, ����������ݴ���ʱ���µ�Э���ӳ�
		// �����ݴ���ʹ��"�첽��Ϣ��"
		if(response_data_length > UVC_MAX_RESPONSE_DATA)
		{
			XM_printf ("UVC Error, the size (%d) of UVC response data exceed maximum %d\n", response_data_length, UVC_MAX_RESPONSE_DATA);
			return -1;
		}
		// Ӧ���������RID(1�ֽ�, 0x00 ~ 0x6F, RID=CID)
		if(command_id >= 0x70)	// ����ֵ
		{
			XM_printf ("UVC Error, illegal command id(0x%02x) in response package\n", command_id);
			return -1;
		}
		
		// ���Ӧ��������Ƿ�1�ֽ�. ����, �ж��������Ƿ�С��0x80. ����0x80��������"����Ӧ���"�н�ֹ.
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
		// �첽��Ϣ��
		// ͨ��UVC����ͨ������
		return XMSYS_CdrInsertAsycMessage (command_id, (char *)response_data_buffer, response_data_length);
	}
	return -1;
}

// ��ȡ����Ӧ��֡������չӦ��֡
// ÿ��Ӧ��֡��2���ֽڹ���
// ����Ӧ�����һ��Ӧ��֡����
// ��չӦ���������2��Ӧ��֡����
unsigned short XMSYS_CdrGetResponseFrame (void)
{
	unsigned int response_frame = 0;
	if(uvc_response_len == 0)
	{
		// ��Ӧ���, �����쳣��
		response_frame = 0x00FF;			// ���ر�ʾ�쳣��Ӧ���
	}
	else if(uvc_response_len == 1)
	{
		// Ӧ������ݳ���Ϊһ���ֽڵ�ʹ��"����Ӧ���"���̴���Ӧ���
		// "����Ӧ���", ��һ������Ӧ��֡����
		// Ӧ���������RID(1�ֽ�, 0x00 ~ 0x7F, RID=CID)��Ӧ�������(1�ֽ�).
		
		response_frame = uvc_response_command | (uvc_response_data[0] << 8);
		
		uvc_response_len = 0;		// �������Ӧ����Ѵ������
	}
	else
	{
		// Ӧ������ݳ��ȴ��ڻ����2�ֽڵ�ʹ��"��չӦ���"���̴���Ӧ���
		// "��չӦ���", ��1��"��չӦ�������֡", ����1��"��չӦ�������֡"��һ��"��չӦ�������֡"����
		
		if(uvc_response_len == uvc_response_left_to_transfer)
		{
			// ����Ǵ�����չӦ�����"����֡"���ǵ�һ��"����֡"
			if(uvc_response_command != 0xFFFFFFFF)
			{
				// ��չӦ����ĵ�һ����չӦ�������֡
				// ��չӦ�������֡ �����ʶRID(1�ֽ�, 0x80 ~ 0xEF, RID=CID+0x80)����չӦ������ݳ���LEN(1�ֽ�),
				response_frame = (uvc_response_command + 0x80) | (uvc_response_len << 8);
				uvc_response_command = 0xFFFFFFFF;		// ��ʾ"����֡"�ѷ���
			}
			else 
			{
				// ��չӦ����ĵ�һ����չӦ�������֡
				unsigned int data = uvc_response_data[0];
				if(data >= 0x80)
				{
					// ��չӦ�������֡ ���ݱ�ʶDID(1�ֽ�, 0xF4), ��չӦ�������DAT(1�ֽ� = DAT + 0x80).
					response_frame = 0xF4 | ((data - 0x80) << 8);
				}
				else
				{
					// ��չӦ�������֡ ���ݱ�ʶDID(1�ֽ�, 0xF2), ��չӦ�������DAT(1�ֽ�).
					response_frame = 0xF2 | (data << 8);	
				}
				uvc_response_left_to_transfer --;
			}
		}
		else if(uvc_response_left_to_transfer != 1)
		{
			// �м����չӦ�����"����֡"
			unsigned int data = uvc_response_data[uvc_response_len - uvc_response_left_to_transfer];
			if(data >= 0x80)
			{
				// ��չӦ�������֡ ���ݱ�ʶDID(1�ֽ�, 0xF4), ��չӦ�������DAT(1�ֽ� = DAT + 0x80).
				response_frame = 0xF4 | ((data - 0x80) << 8);
			}
			else
			{
				// ��չӦ�������֡ ���ݱ�ʶDID(1�ֽ�, 0xF2), ��չӦ�������DAT(1�ֽ�).
				response_frame = 0xF2 | (data << 8);
			}
			uvc_response_left_to_transfer --;
		}
		else
		{
			// ��չӦ�����"����֡"
			unsigned int data = uvc_response_data[uvc_response_len - uvc_response_left_to_transfer];
			if(data >= 0x80)
			{
				// ��չӦ�������֡ ���ݱ�ʶDID(1�ֽ�, 0xF5), ��չӦ�������DAT(1�ֽ� = DAT + 0x80).
				response_frame = 0xF5 | ((data - 0x80) << 8);
			}
			else
			{
				// ��չӦ�������֡ ������ʶEID(1�ֽ�, 0xF3), ��չӦ�������DAT(1�ֽ�).
				response_frame = 0xF3 | (data << 8);
			}
			uvc_response_left_to_transfer --;
			
			uvc_response_len = 0;	// �����չӦ����Ѵ������
		}
	}
	
	//printf ("UVC response_frame=0x%04x\n", response_frame);
	
	return (unsigned short)response_frame;
}

// ��һ���첽��Ϣ֡���뵽CDR֡��
// lpCdrBuffer		CDR֡��������ַ
// cbCdrBuffer		CDR֡�������ֽڴ�С
// lpExtendMessage	��������չ��Ϣ���ݻ�����
// cbExtendMessage	��������չ��Ϣ�ֽڳ���	

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

// ��һ���첽��Ϣ�����뵽UVC��չЭ��
//	AsycMessage		�첽��ϢID
//	lpAsycMessage	�첽��Ϣ����
//	cbAsycMessage	
// ����ֵ
//		-1			ʧ��
//		0			�ɹ�
int XMSYS_CdrInsertAsycMessage (unsigned char AsycMessage, char *lpAsycMessage, int cbAsycMessage)
{
	int ret = -1;
	unsigned int dummy_size = (4 - (cbAsycMessage & 3)) & 3;
	unsigned char *message;
	
	// ���UVC socket�Ƿ��
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
		
		// �첽��Ϣ���Ŀ����ֶ�Ϊ12�ֽڳ���
		// 0xFE, 0x00, NID(1�ֽ�)��FILL(1�ֽ�, ����ֶγ���, ȷ�����е��첽��Ϣ�����Ⱦ�Ϊ4�ֽڶ���)
		// �첽��Ϣ�������ֽڳ���LEN(4�ֽ�)���첽��Ϣ������DAT + FILL(4�ֽ����)�ֶ�, CHECKSUMУ��(4�ֽ�).
		// CHECKSUM = �����ֶΰ�32λ�������ۼ�ȡ��		
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
		// ����ֶ�
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

// 1) UVC��չ"ͷ��־�ֶ�"  (8�ֽ�, 0xFF, 0xFF, 0x00, 0x00, 'X' 'M' 'C', 'D'), 32�ֽڱ߽����.
// 2) UVC��չ"����ֶ�" (4�ֽ�), ÿ��֡����Ψһ���, ��Ŵ�0��ʼ�ۼ�. ͨ��֡���, �ж�֡�Ƿ��ط�֡
// 3) UVC��չ"��Ϣ�ֶ�" (16�ֽ�)
// 4) UVC��չ"�����ֶ�"(�䳤�ֶ�)
// 5) UVC��չ"���ݳ����ֶ�"(4�ֽ�)
// 6) UVC��չ"β��־�ֶ�"  (8�ֽ�, 0xFF, 0x00, 0xFF, 0x00, 'X' 'M' 'C', 'D'), 32�ֽڱ߽����.
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
		
		// UVC��չ"ͷ��־�ֶ�" 32�ֽڶ���
		// ����ֽ�
		packing_bytes = (0x20 - (((unsigned int)(lpCdrBuffer)) & 0x1F)) & (0x1F);
		
		// ��ȥ��������Ҫ������ֽ�
		//lpCdrBuffer += packing_bytes;
		//cbCdrBuffer -= packing_bytes;
		for (int i = 0; i < packing_bytes; i ++)
		{
			*lpCdrBuffer ++ = 0;
			cbCdrBuffer --;
		}
		
		// UVC��չ�ֶο�ʼ��ַ
		lpUvcExtInfo = lpCdrBuffer;
		
		// 8�ֽ���չ"ͷ��־�ֶ�"
		memcpy (lpCdrBuffer, head_id, 8);
		lpCdrBuffer += 8;
		cbCdrBuffer -= 8;
		
		// 2) UVC��չ"����ֶ�" (4�ֽ�), ÿ��֡����Ψһ���, ��Ŵ�0��ʼ�ۼ�. ͨ��֡���, �ж�֡�Ƿ��ط�֡
		*(unsigned int *)lpCdrBuffer = asyc_frame_index;
		asyc_frame_index ++;
		lpCdrBuffer += 4;
		cbCdrBuffer -= 4;
		
		// 3) UVC��չ"��Ϣ�ֶ�" (16�ֽ�)
		// unsigned int  uvc_checksum;		// UVC���ݵ�CheckSum�ֶ�, 32λ���ݵ��ۼӺ�, ����ʹ�ø��ֶ�����UVC Payload���ݵļ��
		*(unsigned int *)lpCdrBuffer = 0;		// 
		lpUvcCheckSum = (unsigned int *)lpCdrBuffer;
		lpCdrBuffer += 4;
		cbCdrBuffer -= 4;
		// unsigned char uvc_channel;			// UVC����ͨ����,
		*lpCdrBuffer ++ = uvc_channel;
		cbCdrBuffer -= 1;
		// unsigned char card_state;		// ��״̬
		// *lpCdrBuffer ++ = 0;
		*lpCdrBuffer ++ = (unsigned char)XMSYS_UvcSocketGetCardState ();
		cbCdrBuffer -= 1;
		// unsigned char asyn_message_count;	// �첽��Ϣ��������, ��������첽��Ϣͬʱ����
		*lpCdrBuffer ++ =	asyn_message_count;
		cbCdrBuffer -= 1;
		// unsigned char rev[9];			// ����
		memset (lpCdrBuffer, 0, 9);
		lpCdrBuffer += 9;
		cbCdrBuffer -= 9;
		
		// 4) UVC��չ"�����ֶ�"(�䳤�ֶ�)
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
			if(data_size != asyc_message_length)		// ��������ֶ�
			{
				// ���0
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
		// 5) UVC��չ"���ݳ����ֶ�"(4�ֽ�)
		*(unsigned int *)lpCdrBuffer = data_size;
		lpCdrBuffer += 4;
		cbCdrBuffer -= 4;
		
		// 6) UVC��չ"β��־�ֶ�"  (8�ֽ�, 0xFF, 0x00, 0xFF, 0x00, 'X' 'M' 'C', 'D'), 32�ֽڱ߽����.
		memcpy (lpCdrBuffer, tail_id, 8);
		lpCdrBuffer += 8;
		cbCdrBuffer -= 8;
		
		ret = size - cbCdrBuffer;
		
		// ͳ��UVC��չ��Ϣ�ֶεĳ���
		cbUvcExtInfo = lpCdrBuffer - lpUvcExtInfo;

		// ͳ��UVC��չ��Ϣ�ֶε�32bit checksum
		*(unsigned int *)lpUvcCheckSum = calc_32bit_checksum ((unsigned char *)lpUvcExtInfo, cbUvcExtInfo);	

		//XM_printf ("frame number = %d, data_size = %d\n", asyc_frame_index - 1, data_size);
		
	} while (0);
	
	// �첽��Ϣ��ȫ��д�뵽UVC����
	// ���첽��Ϣ����ȫ�����
	asyn_message_count = 0;
	asyc_message_length = 0;
	OS_Unuse (&AsycMessageSema);
	
	return ret;
}

// ��CDR���ļ�
// ����ֵ
//		-1		��ʧ��
//		0		�򿪳ɹ�
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
		// ������Ƿ��ѿ���. ������, �رյ�ǰ����
		if(asyn_stream_handle)
		{
			XMSYS_CdrCloseStream ();
		}
		
		memset (file_path, 0, sizeof(file_path));
		combine_fullpath_media_filename ((unsigned char *)file_path, sizeof(file_path), ch, type, (const unsigned char *)filename);

		if(type == 0)
		{
			// ��Ƶ��ʽ�ļ�
			// ������ļ���"ʹ��������"��־, ��ֹ���滻ɾ�� 
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
		
		// �������ֽڳ���
		ret = asyn_stream_length;
		
	} while (0);
	
	if(ret == (-1))
	{
		// ʧ��
		if(type == 0 && mark_usage)
		{
			// �����Ƶ��ʽ�ļ���"ʹ��������"��־
			XM_VideoItemMarkVideoFileUnuse (file_path);
		}
	}
		
	OS_Unuse (&AsycMessageSema);
	return ret;
}


// �رյ�ǰ��CDR��
void XMSYS_CdrCloseStream (void)
{
	char file_path[32];
	OS_Use (&AsycMessageSema);	
	asyn_stream_prefetch_length = 0;
	asyn_stream_prefetch_size = 0;
	
	if(asyn_stream_handle)
	{
		// ȫ·���ļ���
		memset (file_path, 0, sizeof(file_path));
		combine_fullpath_media_filename ((unsigned char *)file_path, sizeof(file_path), asyn_stream_channel, asyn_stream_type, asyn_stream_name);
		
		XM_fclose (asyn_stream_handle);
		if(asyn_stream_type == 0)
		{
			// ��Ƶ����, ���"ʹ��������"��־
			XM_VideoItemMarkVideoFileUnuse (file_path);
		}
		asyn_stream_handle = NULL;
		asyn_stream_length = 0;
		asyn_stream_offset = 0;
	}
	
	OS_Unuse (&AsycMessageSema);
}

// CDR����λ
// ����ֵ
//		-1		��ʧ��
//		0		�򿪳ɹ�
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

// ��CDR���ĵ�ǰ��λ�ö���ָ���ֽڵ�����.
// �����ɹ���, ��ָ��ָ�����¶����Ľ���λ��.
//	����ֵ
//		-1		����IO����
//		0		�����ɹ�
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
		// ����Ƿ񳬳��ļ�����
		if( (size + asyn_stream_offset) > asyn_stream_length )
			size = asyn_stream_length - asyn_stream_offset;
		
		// �����첽��Ϣ����ʣ��ռ�
		
		// 12���첽��Ϣ֡�Ŀ�����Ϣ�ֽڳ���
		// "0xFE(1�ֽ�), 0x00(1�ֽ�), NID(1�ֽ�)��FILL(1�ֽ�, 4�ֽڶ�������ֽڳ���)

		//  �첽��Ϣ�������ֽڳ���LEN(4�ֽ�)��
		//  �첽��Ϣ������DAT, ����ֶ�(����Ϊ4�ֽڶ���), 
		// CHECKSUMУ��(4�ֽ�), = CHECKSUM֮ǰ�������ֶΰ�4�ֽ��ۼ�

		//	20�ֽ����ļ���֡������
		// 	CH(ͨ����1�ֽ�),TYPE(���ͣ�1�ֽ�), rev(����, �̶�Ϊ0, 2�ֽ�), 
		//		Filename (8�ֽ��ļ���), offset (�ļ�ƫ��, 4�ֽ�), size (�ļ����ֽڳ���), 4�ֽ�, ������(size�ֽڳ���)
		//
		// 4 dummy
		if( (size + 12 + 20 + 4) > (MAX_ASYC_MESSAGE - asyc_message_length) )
		{
			// �����ĳ��ȳ���UVC�첽��Ϣ����ʣ��ռ�
			size = (MAX_ASYC_MESSAGE - asyc_message_length) - 12 - 20 - 4;
		}
		
		to_read = size;					
		if(to_read == 0)
		{
			ret = 0;
			break;
		}
		
		dummy_size = (4 - (to_read & 3)) & 3;
		
		// Ƕ���첽��Ϣ֡"�ļ���"���첽��Ϣ����
		message = asyn_message_buffer + asyc_message_length;
		
		// ���Ԥȡ�Ƿ����
		if(asyn_stream_prefetch_size == to_read && asyn_stream_offset == asyn_stream_prefetch_offset)
		{
			// Ԥȡ������ƫ��/��Сƥ��
			memcpy (message + 8 + 20, asyn_stream_prefetch_cache, to_read);
		}
		else
		{
			asyn_stream_prefetch_size = 0;
			asyn_stream_prefetch_length = 0;
			
			// ����λ���ֹ�����ϴ�, �ȴ�readָ��	
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
		
		// �����һ��Ԥȡ������
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
		// "�ļ���"֡�������ֶ�
		*message ++ = asyn_stream_channel;	// 1�ֽ�ͨ����
		*message ++ = asyn_stream_type;		// 1�ֽ�������
		*message ++ = 0;		// 2�ֽڱ���, ���0
		*message ++ = 0;
		memcpy (message, asyn_stream_name, 8);	// 8�ֽ��ļ���
		message += 8;
		*(unsigned int *)message = asyn_stream_offset; // offset(�ļ�ƫ��, 4�ֽ�)
		message += 4;
		*(unsigned int *)message = to_read;	// size(�ļ����ֽڳ���, 4�ֽ�),
		message += 4;
		
		// �޸��ļ�ƫ��ֵ
		asyn_stream_offset = asyn_stream_offset + to_read;
		
		// ����checksum
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

// CDR�ļ�Ԥȡ(��USB���䲢��ִ��)
void XMSYS_CdrFilePrefetch (void)
{
	// ¼��ģʽ�½�ֹԤȡ, ����H264����д������
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

// ����Hybrid��
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
		// ��������ֽڳ����Ƿ񳬳���ǰ֡
						// CH(ͨ����1�ֽ�), TYPE(���ͣ�1�ֽ�), rev(����, �̶�Ϊ0, 2�ֽ�), NO(֡���, 4�ֽ�),
						//		size(hubrid���ֽڳ���, 4�ֽ�), ������(size�ֽڳ���, 4�ֽڶ���)
						//	CH   --> 0x00 ǰ������ͷ, 0x01 ��������ͷ, 0x02 ��Ƶ����.
						//	TYPE --> 0x00 I֡, 0x01 P֡, 0x02 8K����������PCM��Ƶ����.
						//	NO   --> ֡���, ÿ��ͨ�����ж��������, ��ŵ���, �����˸���֡�������ж��Ƿ�֡.
						//	SIZE --> hubrid���ֽڳ���
		//	20�ֽ����ļ���֡������
		// 4 dummy
		if( (hybrid_size + 12 + 16 + 4) > (MAX_ASYC_MESSAGE - asyc_message_length) )
		{
			// �쳣
			break;
		}
		
		dummy_size = (4 - (hybrid_size & 3)) & 3;
		// Ƕ���첽��Ϣ֡"�ļ���"���첽��Ϣ����
		message = asyn_message_buffer + asyc_message_length;
		
		*message ++ = 0xFE;
		*message ++ = 0x00;
		*message ++ = XMSYS_UVC_NID_HYBRID_STREAM;		// hybrid stream id
		*message ++ = (unsigned char)dummy_size;
		*(unsigned int *)message = hybrid_size + 12;
		message += 4;
		// "hybrid��"�����ֶ�
		*message ++ = (unsigned char)ch;	// 1�ֽ�ͨ����
		*message ++ = (unsigned char)type;
		*message ++ = 0;
		*message ++ = 0;
		*(unsigned int *)message = frame_no;
		message += 4;
		*(unsigned int *)message = hybrid_size;
		message += 4;
		// ����hybrid�����ݵ��첽��
		dma_memcpy (message, (unsigned char *)hybrid_data, hybrid_size + dummy_size);
		// ����checksum
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

