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
// 软件模拟版本

#else
// 硬件版本

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
// 文件打开模式
#define	H264_CACHE_FILE_MODE_R			1
#define	H264_CACHE_FILE_MODE_W			2
#define	H264_CACHE_FILE_MODE_RW			3

// H264文件命令（仅支持文件写入）
#define	H264_CACHE_FILE_CMD_OPEN		0
#define	H264_CACHE_FILE_CMD_WRITE		1
#define	H264_CACHE_FILE_CMD_CLOSE		2

#define	H264_STREAM_PATH_COUNT			2			// 同时记录的流通道个数，每个通道的流数据写入到不同的流文件

#define	H264_CACHE_FILE_COUNT			(2*H264_STREAM_PATH_COUNT)	// 系统允许同时打开的Cache文件个数。
																						//		每路H264流记录至少需要两个Cache文件
																						//		(一个关闭写入中，下一个打开创建中)



#define	H264_CACHE_FILE_BLOCK_SIZE		(512*1024)		// H264文件每个Cache块的字节大小，
//#define	H264_CACHE_FILE_BLOCK_SIZE		(256*1024)		// H264文件每个Cache块的字节大小，
//#define	H264_CACHE_FILE_BLOCK_SIZE		(128*1024)		// H264文件每个Cache块的字节大小，值越小，写入速度降低，但是数据更加安全
															// Cache文件读写时一般按Cache块字节大小读写

#define	H264_CACHE_READ_BACK_SIZE		(8*1024)		//	Cache文件回读块字节大小
															//	H264_CACHE_FILE_BLOCK_SIZE = H264_CACHE_READ_BACK_SIZE * 2n



#define	H264_CACHE_FILE_ID				0x34363243		// "C264"
#define	H264_CACHE_BLCK_ID				0x4B434C42		// "BLCK"		


// Cache块算法尽可能保证块均匀的写入到文件系统，不存在过长的时间。
// 即Cache块一旦认为空闲（短期内未使用，且脏）,即将其安排准备写入到文件系统。


// Cache块
typedef struct _H264_CACHE_BLOCK {
	volatile void *		prev;
	volatile void *		next;
	
	unsigned int	id;			//		"BLCK"	

	XM_RSEMA		usage_sema;			// 数据结构访问互斥信号量

	volatile char	header_flag;		// 用于文件头分配使用，保留


	volatile char	lock_flag;			// 锁定标志。仅在lock_flag==0时允许Cache块释放/替换。
											//	块第一次分配时自动锁定，直到块写满，锁定标志自动清除。
											// 每个Cache文件的第1个块(文件偏移0)被强制锁定，直到Cache文件关闭。
											//		H264流文件关闭时部分编码器会修改流文件头, 为改善关闭时需要重新读入流文件头信息导致的延时，
											//		将其对应的第一个块强制锁定。

	volatile char	full_flag;			// 标记块已写满
											//		Cache块首次写满时，将其提交到Cache文件服务进行Cache数据文件写入操作。
											//		Cache块写满后，当后续存在修改时(通过dirty_flag标志识别)，此时不立刻将其写入到文件系统。
											//		而是等到对其他Cache块进行Cache写入操作时，将该Cache块提交到Cache文件系统进行文件写入操作		

	volatile char	dirty_flag;			// 脏区域标志。脏区域写入到文件系统后，脏区域标志被清0。
											// 修改时必须进行锁定write_sema“缓冲区写入互斥访问信号量”
	// 脏区域定义
	volatile size_t		dirty_start_addr;	// 脏区域开始地址
	volatile size_t		dirty_end_addr;	// 脏区域结束地址(脏区域不包括结束地址)

	volatile int	reload_flag;		// 1 标记Cache块从文件中重新加载。
										//		
	// 已重新加载的区域定义
	volatile size_t		reload_start_addr;
	volatile size_t		reload_end_addr;


	volatile size_t		file_offset;		// buffer首地址对应的文件偏移地址
	volatile void *		cache_file;		// Cache文件句柄

	// Cache块缓冲区 (L2CC需要DMA地址必须32地址对齐，即一个Cache Line大小)
	//int			aligned_bytes[6];	
	char*			buffer;
	char			un_aligned_buffer[H264_CACHE_FILE_BLOCK_SIZE + 64];
} H264_CACHE_BLOCK;

// Cache文件
typedef struct _H264_CACHE_FILE {	
	char					filename[128];

	volatile queue_s		cache_block;	// 该Cache文件已分配的cache块链表，按最近未使用排序。即靠近首部的Cache块最近未使用
												//	块替换时按最近未使用顺序替换，即替换块首部块

	H264_CACHE_BLOCK*	current_block;	// 当前正在操作的Cache块

	volatile size_t	h264_offset;	// 流写入偏移(即当前流指针)
	volatile size_t	h264_length;	// 已写入的流数据字节长度，即H264已编码流的最大字节大小		
	volatile int		index;			// Cache文件内部索引
	volatile int		alloc;			// 1 标记已分配
												//	0 空闲
	volatile int		error;			// 0 文件无错误
	// 其他值表示最近文件操作失败的错误码
	volatile int		closed;			// 1 标记文件已关闭
												//   已关闭的文件，其Cache块写入后将立刻释放

	volatile void *		fd;			// CACHE文件对应的流文件指针
	XM_RSEMA 			sema;			// CACHE文件访问互斥信号量，保护多线程访问
	
	XM_RSEMA			sema_file_io;		// Cache文件读写访问互斥信号量，保护文件读写操作
	
	// 统计Seek操作次数
	int					seek_times;
	
	// 效率测试代码，监控Cache文件回读次数及回读字节, 用于算法修改/比较
	size_t				block_read_times;
	size_t				block_read_bytes;	
	unsigned int		average_seek_delay;		// 记录因Seek操作而回读文件/块替换等操作导致的H264编码阻塞平均时间
	unsigned int		maximum_seek_delay;		// 记录因Seek操作而回读文件/块替换等操作导致的H264编码阻塞最大时间
	
	// 记录SEEK写入越界(写到下一个Cache块地址)导致的Cache块回读
	size_t				write_read_times;
	size_t				write_read_bytes;
	unsigned int		average_write_read_delay;		// 记录因Seek写入越界而回读文件/块替换等操作导致的H264编码阻塞平均时间
	unsigned int		maximum_write_read_delay;		// 记录因Seek写入越界而回读文件/块替换等操作导致的H264编码阻塞最大时间

	XMSYSTEMTIME		create_time;
} H264_CACHE_FILE;

void xm_h264_file_init (void);


// H264 Cache文件打开，由H264 Codec线程调用
H264_CACHE_FILE* H264CacheFileOpen (const char *filename);

// H264 Cache文件关闭，由H264 Codec线程调用
int H264CacheFileClose (H264_CACHE_FILE* cache_file);

// H264 Cache文件定位，由H264 Codec线程调用
XMINT64 H264CacheFileSeek (H264_CACHE_FILE* cache_file, XMINT64 off_64, int whence);

// H264 Cache文件读，由H264 Codec线程调用
// 当前版本不支持
int H264CacheFileRead (unsigned char *buf, size_t size, size_t nelem, H264_CACHE_FILE *fd);

// H264 Cache文件写，由H264 Codec线程调用
int H264CacheFileWrite (unsigned char *buf, size_t size, size_t nelem, H264_CACHE_FILE *cache_file);

void XMSYS_H264FileInit (void);

void XMSYS_H264FileExit (void);

// 文件系统安全访问锁定
//		与XMSYS_file_system_block_write_access_unlock配对使用
// 返回值
// -1 文件系统已解除mount, 文件系统访问不安全
//  0 文件系统已锁定, 文件系统访问安全
int XMSYS_file_system_block_write_access_lock (void);

// 文件系统安全访问解锁
// 与XMSYS_file_system_block_write_access_lock配对使用
void XMSYS_file_system_block_write_access_unlock (void);

// 文件系统安全访问锁定(超时设置)
//		与XMSYS_file_system_block_write_access_unlock配对使用
// 返回值
// -2 超时, 锁定失败
// -1 文件系统已解除mount, 文件系统访问不安全
//  0 文件系统已锁定, 文件系统访问安全
int XMSYS_file_system_block_write_access_lock_timeout (unsigned int timeout);


XMBOOL XMSYS_file_system_is_busy (void);

// 等待Cache文件系统非忙（空闲）状态
// timeout_ms 超时等待的时间，毫秒单位
// 	0  表示无限等待
// 	最大超时等待时间0x7FFFFFFF
// 返回值
// 	0  表示成功
//		1	表示超时，系统一直忙
int XMSYS_file_system_waiting_for_cache_system_nonbusy_state (unsigned int timeout_ms);

// 等待Cache文件系统的数据全部刷新到物理存储介质
// timeout_ms	超时等待的时间(毫秒)
// 0	成功
// -1	超时
int XMSYS_WaitingForCacheSystemFlush (unsigned int timeout_ms);

#endif // _XM_H264_CACHE_FILE_
