//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_h264_cache_command_queue.h
//
//	Revision history
//
//		2012.12.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_H264_CACHE_COMMAND_QUEUE_
#define _XM_H264_CACHE_COMMAND_QUEUE_

#if defined (__cplusplus)
	extern "C"{
#endif

#define	XMSYS_H264_CACHE_FILE_COMMAND_EVENT			1		// �����¼�


#define	H264_CACHE_FILE_CMD_QUEUE_SIZE	256		// ������д�С

//
// �ļ���ģʽ
#define	H264_CACHE_FILE_MODE_R			1
#define	H264_CACHE_FILE_MODE_W			2
#define	H264_CACHE_FILE_MODE_RW			3

// H264�ļ������֧���ļ�д�룩
#define	H264_CACHE_FILE_CMD_OPEN			0
#define	H264_CACHE_FILE_CMD_WRITE		1
#define	H264_CACHE_FILE_CMD_CLOSE		2



// Cache�ļ�����
typedef struct _H264_CACHE_FILE_CMD {
	void *				prev;					// ˫������
	void *				next;
	
	int					file_index;			// cache�ļ�������
	int					cache_cmd;			// cache�ļ�����
	void *				cmd_para;			// Cache����Ӳ���
} H264_CACHE_FILE_CMD;

// Cache�ļ�������г�ʼ��
void xm_h264_cache_command_queue_init (void);

// Cache�ļ���������˳�
void xm_h264_cache_command_queue_exit (void);

// ��һ��������뵽Cache�ļ��������
// ����ֵ 1 �ɹ� 0 ʧ��
int xm_h264_cache_command_queue_insert_command (int file_index, int cache_cmd, void *cmd_para);

// ��Cache�ļ��������ȡ��һ�����
// ������Ϊ��ʱ�������������ֱ���µ�������뵽���������
// ����ֵ 1 �ɹ� 0 ʧ��
int xm_h264_cache_command_queue_remove_command (int* file_index, int* cache_cmd, void **cmd_para);

// �ȴ�ֱ������Cache�������
void xm_h264_cache_command_queue_waitfor_cache_command_done (void);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif // _XM_H264_CACHE_COMMAND_QUEUE_
