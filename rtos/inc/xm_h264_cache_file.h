//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_h264_cache_file.h
//
//	Revision history
//
//		2012.12.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_H264_CACHE_FILE_
#define _XM_H264_CACHE_FILE_

#ifdef WIN32
#include <Windows.h>
#else
#include "rtos.h"
#include "xm_core.h"
#include "xm_queue.h" 
#include "xm_base.h"
#include "xm_printf.h"
#endif

#include <stdio.h>
#include <stdlib.h>

//#define	H264_DEBUG

#if defined(_WINDOWS) && defined(_DEBUG)
// ���ģ��汾

#else
// Ӳ���汾

#define H264_FATAL(fmt, args...)	XM_printf(fmt, ## args) 

#ifdef H264_DEBUG
#define H264_PRINT(fmt, args...)	XM_printf(fmt, ## args) 
#else
#define H264_PRINT(fmt, args...)
#endif

#endif



#define SEMA_LOCK(sema)			OS_Use(sema)
#define SEMA_UNLOCK(sema)		OS_Unuse(sema)	
#define SEMA_CREATE(sema)		OS_CREATERSEMA(sema)
#define SEMA_DELETE(sema)		OS_DeleteRSema(sema)



typedef	OS_RSEMA				XM_RSEMA	;


#ifndef XMSYS_H264FILE_TASK_STACK_SIZE
#define	XMSYS_H264FILE_TASK_STACK_SIZE	0x10000
#endif

//
// �ļ���ģʽ
#define	H264_CACHE_FILE_MODE_R			1
#define	H264_CACHE_FILE_MODE_W			2
#define	H264_CACHE_FILE_MODE_RW			3

// H264�ļ������֧���ļ�д�룩
#define	H264_CACHE_FILE_CMD_OPEN		0
#define	H264_CACHE_FILE_CMD_WRITE		1
#define	H264_CACHE_FILE_CMD_CLOSE		2

#define	H264_STREAM_PATH_COUNT			2			// ͬʱ��¼����ͨ��������ÿ��ͨ����������д�뵽��ͬ�����ļ�

#define	H264_CACHE_FILE_COUNT			(2*H264_STREAM_PATH_COUNT)	// ϵͳ����ͬʱ�򿪵�Cache�ļ�������
																						//		ÿ·H264����¼������Ҫ����Cache�ļ�
																						//		(һ���ر�д���У���һ���򿪴�����)



#define	H264_CACHE_FILE_BLOCK_SIZE		(512*1024)		// H264�ļ�ÿ��Cache����ֽڴ�С��
//#define	H264_CACHE_FILE_BLOCK_SIZE		(256*1024)		// H264�ļ�ÿ��Cache����ֽڴ�С��
//#define	H264_CACHE_FILE_BLOCK_SIZE		(128*1024)		// H264�ļ�ÿ��Cache����ֽڴ�С��ֵԽС��д���ٶȽ��ͣ��������ݸ��Ӱ�ȫ
															// Cache�ļ���дʱһ�㰴Cache���ֽڴ�С��д

#define	H264_CACHE_READ_BACK_SIZE		(8*1024)		//	Cache�ļ��ض����ֽڴ�С
															//	H264_CACHE_FILE_BLOCK_SIZE = H264_CACHE_READ_BACK_SIZE * 2n



#define	H264_CACHE_FILE_ID				0x34363243		// "C264"
#define	H264_CACHE_BLCK_ID				0x4B434C42		// "BLCK"		


// Cache���㷨�����ܱ�֤����ȵ�д�뵽�ļ�ϵͳ�������ڹ�����ʱ�䡣
// ��Cache��һ����Ϊ���У�������δʹ�ã����ࣩ,�����䰲��׼��д�뵽�ļ�ϵͳ��


// Cache��
typedef struct _H264_CACHE_BLOCK {
	volatile void *		prev;
	volatile void *		next;
	
	unsigned int	id;			//		"BLCK"	

	XM_RSEMA		usage_sema;			// ���ݽṹ���ʻ����ź���

	volatile char	header_flag;		// �����ļ�ͷ����ʹ�ã�����


	volatile char	lock_flag;			// ������־������lock_flag==0ʱ����Cache���ͷ�/�滻��
											//	���һ�η���ʱ�Զ�������ֱ����д����������־�Զ������
											// ÿ��Cache�ļ��ĵ�1����(�ļ�ƫ��0)��ǿ��������ֱ��Cache�ļ��رա�
											//		H264���ļ��ر�ʱ���ֱ��������޸����ļ�ͷ, Ϊ���ƹر�ʱ��Ҫ���¶������ļ�ͷ��Ϣ���µ���ʱ��
											//		�����Ӧ�ĵ�һ����ǿ��������

	volatile char	full_flag;			// ��ǿ���д��
											//		Cache���״�д��ʱ�������ύ��Cache�ļ��������Cache�����ļ�д�������
											//		Cache��д���󣬵����������޸�ʱ(ͨ��dirty_flag��־ʶ��)����ʱ�����̽���д�뵽�ļ�ϵͳ��
											//		���ǵȵ�������Cache�����Cacheд�����ʱ������Cache���ύ��Cache�ļ�ϵͳ�����ļ�д�����		

	volatile char	dirty_flag;			// �������־��������д�뵽�ļ�ϵͳ���������־����0��
											// �޸�ʱ�����������write_sema��������д�뻥������ź�����
	// ��������
	volatile size_t		dirty_start_addr;	// ������ʼ��ַ
	volatile size_t		dirty_end_addr;	// �����������ַ(�����򲻰���������ַ)

	volatile int	reload_flag;		// 1 ���Cache����ļ������¼��ء�
										//		
	// �����¼��ص�������
	volatile size_t		reload_start_addr;
	volatile size_t		reload_end_addr;


	volatile size_t		file_offset;		// buffer�׵�ַ��Ӧ���ļ�ƫ�Ƶ�ַ
	volatile void *		cache_file;		// Cache�ļ����

	// Cache�黺���� (L2CC��ҪDMA��ַ����32��ַ���룬��һ��Cache Line��С)
	//int			aligned_bytes[6];	
	char*			buffer;
	char			un_aligned_buffer[H264_CACHE_FILE_BLOCK_SIZE + 64];
} H264_CACHE_BLOCK;

// Cache�ļ�
typedef struct _H264_CACHE_FILE {	
	char					filename[128];

	volatile queue_s		cache_block;	// ��Cache�ļ��ѷ����cache�����������δʹ�����򡣼������ײ���Cache�����δʹ��
												//	���滻ʱ�����δʹ��˳���滻�����滻���ײ���

	H264_CACHE_BLOCK*	current_block;	// ��ǰ���ڲ�����Cache��

	volatile size_t	h264_offset;	// ��д��ƫ��(����ǰ��ָ��)
	volatile size_t	h264_length;	// ��д����������ֽڳ��ȣ���H264�ѱ�����������ֽڴ�С		
	volatile int		index;			// Cache�ļ��ڲ�����
	volatile int		alloc;			// 1 ����ѷ���
												//	0 ����
	volatile int		error;			// 0 �ļ��޴���
	// ����ֵ��ʾ����ļ�����ʧ�ܵĴ�����
	volatile int		closed;			// 1 ����ļ��ѹر�
												//   �ѹرյ��ļ�����Cache��д��������ͷ�

	volatile void *		fd;			// CACHE�ļ���Ӧ�����ļ�ָ��
	XM_RSEMA 			sema;			// CACHE�ļ����ʻ����ź������������̷߳���
	
	XM_RSEMA			sema_file_io;		// Cache�ļ���д���ʻ����ź����������ļ���д����
	
	// ͳ��Seek��������
	int					seek_times;
	
	// Ч�ʲ��Դ��룬���Cache�ļ��ض��������ض��ֽ�, �����㷨�޸�/�Ƚ�
	size_t				block_read_times;
	size_t				block_read_bytes;	
	unsigned int		average_seek_delay;		// ��¼��Seek�������ض��ļ�/���滻�Ȳ������µ�H264��������ƽ��ʱ��
	unsigned int		maximum_seek_delay;		// ��¼��Seek�������ض��ļ�/���滻�Ȳ������µ�H264�����������ʱ��
	
	// ��¼SEEKд��Խ��(д����һ��Cache���ַ)���µ�Cache��ض�
	size_t				write_read_times;
	size_t				write_read_bytes;
	unsigned int		average_write_read_delay;		// ��¼��Seekд��Խ����ض��ļ�/���滻�Ȳ������µ�H264��������ƽ��ʱ��
	unsigned int		maximum_write_read_delay;		// ��¼��Seekд��Խ����ض��ļ�/���滻�Ȳ������µ�H264�����������ʱ��

	XMSYSTEMTIME		create_time;
} H264_CACHE_FILE;

void xm_h264_file_init (void);


// H264 Cache�ļ��򿪣���H264 Codec�̵߳���
H264_CACHE_FILE* H264CacheFileOpen (const char *filename);

// H264 Cache�ļ��رգ���H264 Codec�̵߳���
int H264CacheFileClose (H264_CACHE_FILE* cache_file);

// H264 Cache�ļ���λ����H264 Codec�̵߳���
XMINT64 H264CacheFileSeek (H264_CACHE_FILE* cache_file, XMINT64 off_64, int whence);

// H264 Cache�ļ�������H264 Codec�̵߳���
// ��ǰ�汾��֧��
int H264CacheFileRead (unsigned char *buf, size_t size, size_t nelem, H264_CACHE_FILE *fd);

// H264 Cache�ļ�д����H264 Codec�̵߳���
int H264CacheFileWrite (unsigned char *buf, size_t size, size_t nelem, H264_CACHE_FILE *cache_file);

void XMSYS_H264FileInit (void);

void XMSYS_H264FileExit (void);

// �ļ�ϵͳ��ȫ��������
//		��XMSYS_file_system_block_write_access_unlock���ʹ��
// ����ֵ
// -1 �ļ�ϵͳ�ѽ��mount, �ļ�ϵͳ���ʲ���ȫ
//  0 �ļ�ϵͳ������, �ļ�ϵͳ���ʰ�ȫ
int XMSYS_file_system_block_write_access_lock (void);

// �ļ�ϵͳ��ȫ���ʽ���
// ��XMSYS_file_system_block_write_access_lock���ʹ��
void XMSYS_file_system_block_write_access_unlock (void);

// �ļ�ϵͳ��ȫ��������(��ʱ����)
//		��XMSYS_file_system_block_write_access_unlock���ʹ��
// ����ֵ
// -2 ��ʱ, ����ʧ��
// -1 �ļ�ϵͳ�ѽ��mount, �ļ�ϵͳ���ʲ���ȫ
//  0 �ļ�ϵͳ������, �ļ�ϵͳ���ʰ�ȫ
int XMSYS_file_system_block_write_access_lock_timeout (unsigned int timeout);


XMBOOL XMSYS_file_system_is_busy (void);

// �ȴ�Cache�ļ�ϵͳ��æ�����У�״̬
// timeout_ms ��ʱ�ȴ���ʱ�䣬���뵥λ
// 	0  ��ʾ���޵ȴ�
// 	���ʱ�ȴ�ʱ��0x7FFFFFFF
// ����ֵ
// 	0  ��ʾ�ɹ�
//		1	��ʾ��ʱ��ϵͳһֱæ
int XMSYS_file_system_waiting_for_cache_system_nonbusy_state (unsigned int timeout_ms);

// �ȴ�Cache�ļ�ϵͳ������ȫ��ˢ�µ�����洢����
// timeout_ms	��ʱ�ȴ���ʱ��(����)
// 0	�ɹ�
// -1	��ʱ
int XMSYS_WaitingForCacheSystemFlush (unsigned int timeout_ms);

#endif // _XM_H264_CACHE_FILE_
