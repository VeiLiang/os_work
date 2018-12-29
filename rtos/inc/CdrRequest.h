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
	
// �̶�Ϊ16�ֽ�, ����ȫ����Ϣ
typedef struct _XM_CDR_INFO {
	unsigned int  uvc_checksum;		// UVC���ݵ�CheckSum�ֶ�, 32λ���ݵ��ۼӺ�, ����ʹ�ø��ֶ�����UVC Payload���ݵļ��
	unsigned char uvc_channel;			// UVC����ͨ����,
												//	1) UVC Video Sampleͨ����(1�ֽ�), ����Video Sample����Դ, 
												//	0 --> ǰ������ͷ
												//	1 --> ��������ͷ
												//	2 --> ��Ԫ����(һ���ߴ�̶��ĺڰ�ͼ��, ���ڸ�����Ƶ�ļ�����)
												//	3 --> JPEG Image
												//	4 --> ��Ƶ�ļ��ط�	
												//		��Ԫ����Ϊȱʡ״̬
	unsigned char card_state;		// ��״̬
	unsigned char asyn_message_count;	// �첽��Ϣ��������, ��������첽��Ϣͬʱ����
	unsigned char rev[9];			// ����
} XM_CDR_INFO;

// ��һ���첽��Ϣ֡���뵽CDR֡��
// lpCdrBuffer		CDR֡��������ַ
// cbCdrBuffer		CDR֡�������ֽڴ�С
// lpExtendMessage	��������չ��Ϣ���ݻ�����
// cbExtendMessage	��������չ��Ϣ�ֽڳ���	

// 1) UVC��չ"ͷ��־�ֶ�"  (8�ֽ�, 0xFF, 0xFF, 0x00, 0x00, 'X' 'M' 'C', 'D'), 32�ֽڱ߽����.
// 2) UVC��չ"����ֶ�" (4�ֽ�), ÿ��֡����Ψһ���, ��Ŵ�0��ʼ�ۼ�. ͨ��֡���, �ж�֡�Ƿ��ط�֡
// 3) UVC��չ"��Ϣ�ֶ�"		// 16�ֽ�
// 4) UVC��չ"�����ֶ�"(�䳤�ֶ�)
// 5) UVC��չ"���ݳ����ֶ�"(4�ֽ�)
// 6) UVC��չ"β��־�ֶ�"  (8�ֽ�, 0xFF, 0x00, 0xFF, 0x00, 'X' 'M' 'C', 'D'), 32�ֽڱ߽����.
int XMSYS_CdrFrameInsertAsycMessage (char *lpExtendMessage, int cbExtendMessage);

	
// CDR˽��Э�鴦��
void XMSYS_CdrPrivateProtocolHandler (unsigned char protocol_type, unsigned char protocol_data);

int XMSYS_JpegFrameInsertExtendMessage (unsigned char uvc_channel, char *lpCdrBuffer, int cbCdrBuffer);

// ��һ���첽��Ϣ�����뵽UVC��չЭ��
//	AsycMessage		�첽��ϢID
//	lpAsycMessage	�첽��Ϣ����
//	cbAsycMessage	
// ����ֵ
//		-1			ʧ��
//		0			�ɹ�
int XMSYS_CdrInsertAsycMessage (unsigned char AsycMessage, char *lpAsycMessage, int cbAsycMessage);

void XMSYS_CdrInit (void);

// ��CDR���ļ�
int XMSYS_CdrOpenStream (unsigned char ch, unsigned char type, const char *filename);

// �رյ�ǰ��CDR��
void XMSYS_CdrCloseStream (void);

// CDR����λ
// ����ֵ
//		-1		��ʧ��
//		0		�򿪳ɹ�
int XMSYS_CdrSeekStream (unsigned int offset);

// ��CDR���ĵ�ǰ��λ�ö���ָ���ֽڵ�����.
// �����ɹ���, ��ָ��ָ�����¶����Ľ���λ��.
//	����ֵ
//		-1		����IO����
//		0		�����ɹ�
int XMSYS_CdrReadStream (unsigned int offset, unsigned int size);

// ����Hybrid��
int XMSYS_CdrInsertHybridStream (unsigned int ch, unsigned int type, unsigned int frame_no, char *hybrid_data, unsigned int hybrid_size);

#ifdef __cplusplus
}
#endif

#endif