#include <xm_proj_define.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <xm_type.h>
#include <xm_queue.h>
#include <xm_printf.h>
#include <xm_base.h>
#include "xm_h264_cache_command_queue.h"
#include "xm_h264_cache_file.h"
#include "xm_semaphore.h"

extern void XM_Delay (unsigned int ms);

// 命令队列，保存H264 codec发送的命令
static H264_CACHE_FILE_CMD cmd_unit[H264_CACHE_FILE_CMD_QUEUE_SIZE];	
static queue_s		cmd_pool;		// 排队命令链表
static queue_s		cmd_free;		// 空闲命令链表

static void *cache_cmd_access_sema;		// Cache命令内部数据互斥访问信号量
static void *cache_cmd_count_sema;		// Cache命令计数信号量

// 记录排队命令链表中当前的命令个数
static int cache_cmd_count;
// 记录排队命令链表的最大的命令个数
static int max_cache_cmd_count;		
// 统计命令插入最大延时
static int maximum_command_insert_delay_time;

static unsigned int max_cmd_done_delay;

// Cache命令队列初始化
void xm_h264_cache_command_queue_init (void)
{
	int i;
	queue_initialize (&cmd_free);
	queue_initialize (&cmd_pool);
	memset (cmd_unit, 0, sizeof(cmd_unit));
	for (i = 0; i < H264_CACHE_FILE_CMD_QUEUE_SIZE; i++)
	{
		queue_insert ((queue_s *)&cmd_unit[i], &cmd_free);
	}

	// 命令计数信号量
	cache_cmd_count_sema = XM_CreateSemaphore ("cache_cmd_count_sema", 0);
	assert (cache_cmd_count_sema);
	// 访问互斥保护信号量
	cache_cmd_access_sema = XM_CreateSemaphore ("cache_cmd_access_sema", 1);
	assert (cache_cmd_count_sema);

	maximum_command_insert_delay_time = 0;
	max_cache_cmd_count = 0;
	cache_cmd_count = 0;
	max_cmd_done_delay = 0;
}

// Cache命令队列退出
void xm_h264_cache_command_queue_exit (void)
{
	if(XM_WaitSemaphore (cache_cmd_access_sema) == 0)
	{
		H264_FATAL ("cache_cmd_access_sema illegal\n");
		return;
	}

	if(cache_cmd_count_sema)
	{
		XM_CloseSemaphore (cache_cmd_count_sema);
		XM_DeleteSemaphore (cache_cmd_count_sema);
		cache_cmd_count_sema = NULL;
	}
	if(cache_cmd_access_sema)
	{
		XM_CloseSemaphore (cache_cmd_access_sema);
		XM_DeleteSemaphore (cache_cmd_access_sema);
		cache_cmd_access_sema = NULL;
	}
}

// 将Cache文件命令插入到排队命令队列的尾部（H264 Cache文件调用）
// 1 成功
// 0 失败
int xm_h264_cache_command_queue_insert_command (int file_index, int cache_cmd, void *cmd_para)
{
	H264_CACHE_FILE_CMD *cmd;
	H264_CACHE_FILE_CMD *loop_cmd;
	H264_CACHE_BLOCK *block;		// 记录写入命令的块地址
	int skip = 0;
	int s_ticket, e_ticket;


	// printf ("InsertCommand file=%d, cmd=%d\n", cache_file, cache_cmd);
	if(cache_cmd == H264_CACHE_FILE_CMD_WRITE)
	{
		block = (H264_CACHE_BLOCK *)cmd_para;

		if(XM_WaitSemaphore (cache_cmd_access_sema) == 0) /* Make sure nobody else uses */
			return 0;

		// 搜索相同的块写命令是否已Cache
		loop_cmd = (H264_CACHE_FILE_CMD *)queue_next (&cmd_pool);
		while((queue_s *)loop_cmd != &cmd_pool)
		{
			// 检查写命令是否存在相同块地址操作
			if(	loop_cmd->cache_cmd == H264_CACHE_FILE_CMD_WRITE
				&& loop_cmd->cmd_para == block	)
			{
				// printf ("same block write\n");
				skip = 1;
				break;
			}
			loop_cmd = (H264_CACHE_FILE_CMD *)queue_next ((queue_s *)loop_cmd);
		}
		XM_SignalSemaphore (cache_cmd_access_sema);
		if(skip)	// 同一个块地址写入操作
			return 1;
	}
	
	s_ticket = XM_GetTickCount ();
	if(XM_WaitSemaphore (cache_cmd_access_sema) == 0) /* Make sure nobody else uses */
		return 0;
	cmd = NULL;
	while(!cmd)
	{
		// 检查是否存在未使用的空闲命令单元
		if(queue_empty (&cmd_free))
		{
			// H264_FATAL ("H264_CACHE_FILE Can't Insert Command(%d)\n", cache_cmd);
			XM_SignalSemaphore (cache_cmd_access_sema);
			// 延时，等待Cache文件服务释放命令
			XM_Delay (5);
			if( (XM_GetTickCount() - s_ticket) > 1000 )
			{
				H264_FATAL ("too long command insert delay time=%d, cmd=%d\n", XM_GetTickCount() - s_ticket, cache_cmd);
				return 0;
			}
			if(XM_WaitSemaphore (cache_cmd_access_sema) == 0) /* Make sure nobody else uses */
				return 0;
			continue;
		}
		else
		{
			cmd = (H264_CACHE_FILE_CMD *)queue_delete_next(&cmd_free);
		}
	}

	e_ticket = XM_GetTickCount ();
	if( (e_ticket - s_ticket) > maximum_command_insert_delay_time )
	{
		maximum_command_insert_delay_time = (e_ticket - s_ticket);
		H264_FATAL ("maximum command insert delay time=%d\n", maximum_command_insert_delay_time);
	}

	// 初始化并插入到命令队列尾部
	cmd->file_index = file_index;
	cmd->cache_cmd = cache_cmd;
	cmd->cmd_para = cmd_para;
	queue_insert ((queue_s *)cmd, &cmd_pool);

	// 统计队列中尚未执行的命令个数
	cache_cmd_count ++;

	if(cache_cmd_count > max_cache_cmd_count)
	{
		max_cache_cmd_count = cache_cmd_count;
		//printf ("maximum cache command count %d\n", max_cache_cmd_count);
	}		
	
	// 释放访问互斥保护
	XM_SignalSemaphore (cache_cmd_access_sema);

	// 累加cache命令计数器
	XM_SignalSemaphore (cache_cmd_count_sema);

	return 1;
}

// 从Cache文件命令队列取出一个命令。
// 当队列为空时，调用任务挂起直到新的命令插入到命令队列中
// 返回值 1  成功取出一个命令 
//        0  命令空 
//        -1 失败
int xm_h264_cache_command_queue_remove_command (int* file_index, int* cache_cmd, void **cmd_para)
{
	int ret;
	H264_CACHE_FILE_CMD *cmd;
	
	if(XM_WaitSemaphore (cache_cmd_count_sema) == 0)
		return -1;

	if(XM_WaitSemaphore (cache_cmd_access_sema) == 0) /* Make sure nobody else uses */
	{
		H264_FATAL ("illegal cache_cmd_access_sema\n");
		return -1;
	}
	
	// 从排队命令队列读取下一个文件句柄及命令
	if(queue_empty (&cmd_pool))
	{
		// 队列为空
		ret = 0;
	}
	else
	{
		cmd = (H264_CACHE_FILE_CMD *)queue_delete_next (&cmd_pool);
		*file_index = cmd->file_index;
		*cache_cmd = cmd->cache_cmd;
		*cmd_para = cmd->cmd_para;
		// 释放到空闲链表
		queue_insert ((queue_s *)cmd, &cmd_free);
		ret = 1;
	}
	
	XM_SignalSemaphore (cache_cmd_access_sema);
	return ret;
}

// 等待直到所有Cache命令完成
void xm_h264_cache_command_queue_waitfor_cache_command_done (void)
{
	int loop = 1;
	unsigned int s_ticket = XM_GetTickCount();
	while(loop)
	{
		if(XM_WaitSemaphore (cache_cmd_access_sema) == 0)
			return;
		if(queue_empty (&cmd_pool))
		{
			loop = 0;
		}
		XM_SignalSemaphore (cache_cmd_access_sema);
		if(loop)
		{
			XM_Delay (2);
		}
	}
	if( (XM_GetTickCount() - s_ticket) > max_cmd_done_delay)
	{
		max_cmd_done_delay = (XM_GetTickCount() - s_ticket);
		H264_PRINT ("max_cmd_done_delay=%d ms\n", max_cmd_done_delay);
	}
}