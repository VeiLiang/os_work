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

#define	XMSYS_H264_CACHE_FILE_COMMAND_EVENT			1		// 命令事件


#define	H264_CACHE_FILE_CMD_QUEUE_SIZE	256		// 命令队列大小

//
// 文件打开模式
#define	H264_CACHE_FILE_MODE_R			1
#define	H264_CACHE_FILE_MODE_W			2
#define	H264_CACHE_FILE_MODE_RW			3

// H264文件命令（仅支持文件写入）
#define	H264_CACHE_FILE_CMD_OPEN			0
#define	H264_CACHE_FILE_CMD_WRITE		1
#define	H264_CACHE_FILE_CMD_CLOSE		2



// Cache文件命令
typedef struct _H264_CACHE_FILE_CMD {
	void *				prev;					// 双向链表
	void *				next;
	
	int					file_index;			// cache文件索引号
	int					cache_cmd;			// cache文件命令
	void *				cmd_para;			// Cache命令附加参数
} H264_CACHE_FILE_CMD;

// Cache文件命令队列初始化
void xm_h264_cache_command_queue_init (void);

// Cache文件命令队列退出
void xm_h264_cache_command_queue_exit (void);

// 将一个命令插入到Cache文件命令队列
// 返回值 1 成功 0 失败
int xm_h264_cache_command_queue_insert_command (int file_index, int cache_cmd, void *cmd_para);

// 从Cache文件命令队列取出一个命令。
// 当队列为空时，调用任务挂起直到新的命令插入到命令队列中
// 返回值 1 成功 0 失败
int xm_h264_cache_command_queue_remove_command (int* file_index, int* cache_cmd, void **cmd_para);

// 等待直到所有Cache命令完成
void xm_h264_cache_command_queue_waitfor_cache_command_done (void);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif // _XM_H264_CACHE_COMMAND_QUEUE_
