//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_h264_cache_file.c
//	  流管理接口, 并发流文件的cache分配/替换/写入/读取
//
//	Revision history
//
//		2012.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#include <xm_proj_define.h>
#include <stdio.h>
#include <assert.h>
#include <xm_type.h>
#include <xm_base.h>
#include <xm_key.h>
#include <xm_dev.h>
#include <xm_user.h>
#include <xm_queue.h>
#include <xm_file.h>
#include "xm_h264_cache_file.h"
#include "xm_h264_cache_command_queue.h"
#include "xm_semaphore.h"
#include "xm_printf.h"
#include "FS.h"
#include "rtos.h"
#include "xm_power.h"


#include <xm_videoitem.h>

#ifdef WIN32
#include <io.h>
#endif

#ifdef ISP_RAW_ENABLE
#define	H264_CACHE_SIZE					0x500000		// 用于Cache块的缓冲区memory字节大小
#else
#define	H264_CACHE_SIZE					0x1000000//0xE00000
#endif


#define	H264_CACHE_BLOCK_COUNT			(H264_CACHE_SIZE/H264_CACHE_FILE_BLOCK_SIZE)


// 测试条件
// 写入16MB
// 1) H264_CACHE_FILE_BLOCK_SIZE == 128
// 7.2s

//#define	FS_VERIFY_CYCLE_MS		(20*1000)		// 文件读写校验间隔周期(毫秒)
#define	FS_VERIFY_CYCLE_MS		(2*1000)		// 文件读写校验间隔周期(毫秒)
#pragma data_alignment=4096
static unsigned int verify_buffer[4096/4];		// 64KB文件比较

// H264文件Cache写入任务
static OS_TASK TCB_H264FileTask;
__no_init static int StackH264FileTask[XMSYS_H264FILE_TASK_STACK_SIZE/4];          /* Task stacks */
static unsigned long	next_ticket_to_verify_file_system;		// 下一次进行文件读写校验的时间


#define	HEADER_USAGE


#if _XM_PROJ_ == _XM_PROJ_2_SENSOR_1080P_CVBS
#define	CACHE_BLOCK_FOR_FILE_HEADER	4			// 保留4个

#elif _XM_PROJ_ == _XM_PROJ_DVR_4_SENSOR_D1_

#define	CACHE_BLOCK_FOR_FILE_HEADER	12			// 保留8个

#else
#define	CACHE_BLOCK_FOR_FILE_HEADER	3			// 保留3个
#endif


#pragma data_alignment=1024
__no_init static H264_CACHE_BLOCK h264CacheBlock[H264_CACHE_BLOCK_COUNT];
static volatile queue_s		cache_block_free;		// 空闲的Cache块链表
static XM_RSEMA	cache_block_access_sema;		// 保护cache块互斥访问的信号量

static OS_EVENT	cache_file_system_not_busy_event;	// 标记Cache文件系统是否有大量缓存的数据等待写入，即cache系统是否忙
static int			cache_file_system_busy_block_count;	// Cache文件系统等待写入的块
static unsigned int cache_file_system_dirty_block_statistics[H264_CACHE_BLOCK_COUNT+1];

// Cache文件句柄数组
static H264_CACHE_FILE h264CacheFileHandle[H264_CACHE_FILE_COUNT];

static XM_RSEMA			h264CacheFileSema;			// 互斥访问信号量, 保护Cache系统内部变量


// 记录最大空闲块分配等待时间, 用于算法统计分析
static int maximum_block_alloc_delay_time = 0;

// 记录最大写入时间
static unsigned int max_write_time = 0;


static XM_RSEMA		block_write_access_sema;	// 文件系统块写入互斥访问信号量
												// 保护文件的写入按CACHE块分配、写入
												// 尽可能避免文件碎片的产生


static H264_CACHE_BLOCK *SearchReplacedCacheBlockFromCacheFile (H264_CACHE_FILE *cache_file);

#define	CACHE_DEBUG	
#ifdef CACHE_DEBUG
static void cache_debug_assert (void)
{
	assert (0);
}
#endif




// Cache块初始化
static void CacheBlockInit (void)
{
	int i;
	queue_initialize ((queue_s *)&cache_block_free);
	for (i = 0; i < H264_CACHE_BLOCK_COUNT; i ++)
	{
		// 将Cache块头清0
		memset (&h264CacheBlock[i], 0, sizeof(H264_CACHE_BLOCK) - H264_CACHE_FILE_BLOCK_SIZE);
		
		// 保留Cache块单元专用于Cache文件头的写入，共CACHE_BLOCK_FOR_FILE_HEADER个
		if(i < CACHE_BLOCK_FOR_FILE_HEADER)
			h264CacheBlock[i].header_flag = 1;
		else
			h264CacheBlock[i].header_flag = 0;
		h264CacheBlock[i].id = H264_CACHE_BLCK_ID;
		
		h264CacheBlock[i].buffer = (char *)( (((unsigned int)h264CacheBlock[i].un_aligned_buffer) + 0x1F) & (~0x1F) );
		
		queue_insert ((queue_s *)&h264CacheBlock[i], (queue_s *)&cache_block_free);
		SEMA_CREATE (&h264CacheBlock[i].usage_sema);
	}
	SEMA_CREATE (&cache_block_access_sema);
	
	// 文件系统块写入互斥访问信号量
	SEMA_CREATE (&block_write_access_sema);
	
	// 文件系统空闲（非忙）事件
	cache_file_system_busy_block_count = 0;
	memset (cache_file_system_dirty_block_statistics, 0, sizeof(cache_file_system_dirty_block_statistics));
	OS_EVENT_Create (&cache_file_system_not_busy_event);
}

static void CacheBlockExit (void)
{
	int i;
	for (i = 0; i < H264_CACHE_BLOCK_COUNT; i ++)
	{
		SEMA_DELETE (&h264CacheBlock[i].usage_sema);
	}
	SEMA_DELETE (&cache_block_access_sema);
	
	SEMA_DELETE (&block_write_access_sema);
}

// 文件系统安全访问锁定
//		与XMSYS_file_system_block_write_access_unlock配对使用
// 返回值
// -1 文件系统已解除mount, 文件系统访问不安全
//  0 文件系统已锁定, 文件系统访问安全
int XMSYS_file_system_block_write_access_lock (void)
{
	SEMA_LOCK (&block_write_access_sema);
	if(XM_VideoItemIsBasicServiceOpened())
		return 0;
	return -1;
}

// 文件系统安全访问锁定(超时设置)
//		与XMSYS_file_system_block_write_access_unlock配对使用
// 返回值
// -2 超时, 锁定失败
// -1 文件系统已解除mount, 文件系统访问不安全
//  0 文件系统已锁定, 文件系统访问安全
int XMSYS_file_system_block_write_access_lock_timeout (unsigned int timeout)
{
	if(OS_Use_timeout	(&block_write_access_sema, timeout) == 0)
		return -2;
	if(XM_VideoItemIsBasicServiceOpened())
		return 0;
	return -1;	
}

// 解除文件系统访问锁定
void XMSYS_file_system_block_write_access_unlock (void)
{
	SEMA_UNLOCK (&block_write_access_sema);
}

// 获取Cache系统忙等待的级别
// 0 ~ 1 表示系统Cache基本空闲, 流畅流畅
// 2 ~ 8 级别越大, 表示使用的Cache缓存越多
// 9     表示内部Cache已满, 暂时无法写入
unsigned int XMSYS_GetCacheSystemBusyLevel (void)
{
	unsigned int count = 0;
	unsigned int level;
	int i;
	OS_EnterRegion ();	// 禁止任务切换
	for (i = 0; i < H264_CACHE_BLOCK_COUNT; i ++)
	{
		if(h264CacheBlock[i].dirty_flag)
			count ++;
	}
	OS_LeaveRegion ();
	level = count * 10 / H264_CACHE_BLOCK_COUNT;
	if(level > 9)
		level = 9;
	
	// 20180825 考虑磁盘剩余空间不足程度的影响
	unsigned int disk_busy_level = XM_VideoItemCheckDiskFreeSpaceBusy();
	level += disk_busy_level;
	if(level > 9)
		level = 9;
	
	return level;
}

// 等待Cache文件系统的数据全部刷新到物理存储介质
// timeout_ms	超时等待的时间(毫秒)
// 0	成功
// -1	超时
int XMSYS_WaitingForCacheSystemFlush (unsigned int timeout_ms)
{
	unsigned int count = 0;
	unsigned int level;
	int i;
	unsigned int enter_ticket, leave_ticket;
	int ret = -1;
	
	enter_ticket = XM_GetTickCount();
	leave_ticket = enter_ticket + timeout_ms;
	
	while(XM_GetTickCount() < leave_ticket)
	{
		OS_Delay (50);
		OS_EnterRegion ();	// 禁止任务切换
		// 等待所有cache文件关闭
		for (i = 0; i < H264_CACHE_FILE_COUNT; i++)
		{
			if(h264CacheFileHandle[i].alloc)
				break;
		}
		OS_LeaveRegion ();
		if(i == H264_CACHE_FILE_COUNT)
		{
			ret = 0;
			break;
		}
	}
	XM_printf ("WaitingForCacheSystemFlush %s, spend %d ms\n", 
			  (ret == 0) ? "Success" : "Timeout", XM_GetTickCount() - enter_ticket);
	return ret;
}

static unsigned int get_dirty_cache_block_count (void)
{
	static unsigned int _get_count_index = 0;
	unsigned int count = 0;
	int i;
	_get_count_index++;
	SEMA_LOCK (&cache_block_access_sema);
	for (i = 0; i < H264_CACHE_BLOCK_COUNT; i ++)
	{
		if(h264CacheBlock[i].dirty_flag)
			count ++;
	}
	cache_file_system_busy_block_count = count;
	if(count <= 1)
	{
		OS_EVENT_Set (&cache_file_system_not_busy_event);
	}
	else
	{
		OS_EVENT_Reset (&cache_file_system_not_busy_event);
	}
	
	if(cache_file_system_busy_block_count <= H264_CACHE_BLOCK_COUNT)
		cache_file_system_dirty_block_statistics[cache_file_system_busy_block_count]++;
	
	SEMA_UNLOCK (&cache_block_access_sema);
	
	if(cache_file_system_busy_block_count >= 8)
	{
		//XM_printf ("more busy block\n");
	}
	
	if( (_get_count_index & 0xFFF) == 0 )
	{
		// 显示脏块的统计信息
		unsigned int total_count = 0;
		for (i = 0; i <= H264_CACHE_BLOCK_COUNT; i++)
			total_count += cache_file_system_dirty_block_statistics[i];
		XM_printf ("\nblock cache statistics, count=%d, Ticket=%d\n", H264_CACHE_BLOCK_COUNT, XM_GetTickCount());
		for (i = 0; i <= H264_CACHE_BLOCK_COUNT; i++)
		{
			if(cache_file_system_dirty_block_statistics[i])
				XM_printf ("%2d %3d%%, %d\n", i,   cache_file_system_dirty_block_statistics[i] * 1000 / total_count,  cache_file_system_dirty_block_statistics[i]);
		}
	}
	
	return count;
}

// 等待Cache文件系统非忙（空闲）状态
// timeout_ms 超时等待的时间，毫秒单位
// 	0  表示无限等待
// 	最大超时等待时间0x7FFFFFFF
// 返回值
// 	0  表示成功
//	1~100 表示超时，系统一直忙. 表示系统忙的百分比。100%表示超级忙，
//		
// 根据缓存数据的时间长度，决定文件删除等待的策略。
// 缓存时间越短，删除长度越大。
// 缓存时间越长，删除长度越短
// 0.125秒	马上执行正常删除
// 0.25秒， 执行正常删除的50%，其他类推
int XMSYS_file_system_waiting_for_cache_system_nonbusy_state (unsigned int timeout_ms)
{
	int ret = 0;
	if(timeout_ms >= 0x80000000)
		timeout_ms = 0x7FFFFFFF;
	
	if(cache_file_system_busy_block_count <= 1)
		return 0;
	
	if(timeout_ms == 0)
		OS_EVENT_Wait (&cache_file_system_not_busy_event);
	else
		ret = OS_EVENT_WaitTimed (&cache_file_system_not_busy_event, timeout_ms);
	if(ret)
	{
		ret = cache_file_system_busy_block_count * 100 / H264_CACHE_BLOCK_COUNT;
		if(ret <= 5)
			ret = 5;
	}
	return ret;
}

// 计算剩余未使用的cache块字节大小
static unsigned int get_free_cache_block_size (void)
{
	unsigned int count = 0;
	H264_CACHE_BLOCK *loop;
	
	SEMA_LOCK (&cache_block_access_sema);
	loop = (H264_CACHE_BLOCK *)queue_next ((queue_s *)&cache_block_free);
	while(loop != (H264_CACHE_BLOCK *)&cache_block_free)
	{
		if(!loop->header_flag)
		{
			count++;
		}
		
#ifdef CACHE_DEBUG
		if(loop->id != H264_CACHE_BLCK_ID)
		{
			XM_printf ("cache block 0x%08x break\n", loop);
			cache_debug_assert ();
		}
#endif
		
		loop = (H264_CACHE_BLOCK *)queue_next ((queue_s *)loop);
	}
	
	SEMA_UNLOCK (&cache_block_access_sema);
	
	return count * H264_CACHE_FILE_BLOCK_SIZE;
}

// 检查空闲的H264流写入cache缓冲是否大于最小所需空间
int xm_h264_check_cache_exhausted (void)
{
	// 计算空闲未使用的cache块字节大小
	unsigned int free_block_sizes = get_free_cache_block_size ();
	if(free_block_sizes >= 0x180000)	// 空闲1.5MB
		return 0;
	else
		return 1;
}

XMBOOL XMSYS_file_system_is_busy (void)
{
	unsigned int cache_block_sizes = H264_CACHE_SIZE - get_free_cache_block_size ();
	unsigned int cache_block_count;
	unsigned int video_channel = XM_VideoItemGetVideoFileChannel ();
	
	cache_block_count = cache_block_sizes / H264_CACHE_FILE_BLOCK_SIZE;
	if(cache_block_count >= H264_CACHE_BLOCK_COUNT)
		cache_block_count = H264_CACHE_BLOCK_COUNT;
	
	// 正常情况下，每路每个视频文件的CACHE中保存有3个Cache块，其中block 0是文件头，一个是当前正在写入编码流的cache块，一个是正在写入到SD卡中的cache块
	// 前后双路录像时一般保存2 * 3个cache块
	if(cache_block_count > (3 * video_channel))		// 表示忙
	{
		//XM_printf ("cache busy to write, count=%d\n",  cache_block_count);
		return 1;
	}
	
	return 0;
}

// 检查H264编码器是否忙(系统等待写入)
XMBOOL XMSYS_H264CodecIsBusy (void)
{
	return 0;
}



static void adjust_codec_frame_rate (void)
{
	// 计算空闲未使用的cache块总字节大小
	unsigned int free_block_sizes = get_free_cache_block_size ();
	
	// 根据Cache剩余空间大小决定H264 Codec编码时是否丢帧及丢帧的数量
	// 1) 设置ISP的帧率控制vifrasel0，vifrasel1
	// 2) 调整H264编码器的码率
}

static void CacheBlockFree (H264_CACHE_BLOCK *block)
{
	if(block == NULL)
		return;
	
	if(block->dirty_flag)
	{
		XM_printf ("fatal error, the block is lock or dirty\n");
	}

	block->full_flag = 0;
	block->dirty_flag = 0;
	block->lock_flag = 0;		// Cache块刚分配时自动锁定，防止被替换
	block->cache_file = NULL;
	block->file_offset = 0;
	block->reload_flag = 0;

	SEMA_LOCK (&cache_block_access_sema);
	
#ifdef CACHE_DEBUG
	if(block->id != H264_CACHE_BLCK_ID)
	{
		XM_printf ("cache block 0x%08x break\n", block);
			cache_debug_assert ();
	}
#endif
	
	queue_insert ((queue_s *)block, (queue_s *)&cache_block_free);
	SEMA_UNLOCK (&cache_block_access_sema);
		
	// 根据剩余空闲帧计数调整codec编码帧率
	adjust_codec_frame_rate ();
}

// 从Cache块空闲池中分配一个Cache块
static H264_CACHE_BLOCK *CacheBlockAlloc (H264_CACHE_FILE *cache_file, size_t h264_offset)
{
	int s_ticket, e_ticket;

	H264_CACHE_BLOCK *block = NULL;
	if(cache_file == NULL)
	{
		H264_FATAL ("CacheBlockAlloc failed, cache_file==NULL\n");
		return NULL;
	}

	h264_offset = h264_offset & ~(H264_CACHE_FILE_BLOCK_SIZE - 1);
	s_ticket = XM_GetTickCount ();
	
	while(!block)
	{
		// 在空闲块中查找
		SEMA_LOCK (&cache_block_access_sema);
		if(!queue_empty ((queue_s *)&cache_block_free))
		{
			H264_CACHE_BLOCK *loop;
			loop = (H264_CACHE_BLOCK *)queue_next ((queue_s *)&cache_block_free);
			while(loop != (H264_CACHE_BLOCK *)&cache_block_free)
			{
#ifdef CACHE_DEBUG
				if(loop->id != H264_CACHE_BLCK_ID)
				{
					XM_printf ("cache block 0x%08x break\n", loop);
					cache_debug_assert ();
				}
#endif
				
				if(h264_offset == 0 && loop->header_flag)
				{
					// specifical for file header usage
					block = loop;
					break;
				}
				else if(h264_offset && !loop->header_flag)
				{
					block = loop;
					break;
				}
				loop = (H264_CACHE_BLOCK *)queue_next ((queue_s *)loop);
			}
									
			if(block)
			{
				queue_delete ((queue_s *)block);		
			}
		}
		
		SEMA_UNLOCK (&cache_block_access_sema);
		
		if(block)
		{
			// 已从链表中断开
			// 检查剩余块的个数
#ifdef CACHE_DEBUG
			if(block->id != H264_CACHE_BLCK_ID)
			{
				XM_printf ("cache block 0x%08x break\n", block);
				cache_debug_assert ();
			}
#endif
				
			// 根据剩余空闲帧计数调整codec编码帧率
			adjust_codec_frame_rate ();
		}
		
		
		if(block)
		{
			// 找到空闲块
			break;
		}
		else if(h264_offset == 0)
		{
			// 检查是否是第一个Cache块(包含文件头信息)
			//H264_FATAL ("Please check CACHE_BLOCK_FOR_FILE_HEADER(%d) is small\n", CACHE_BLOCK_FOR_FILE_HEADER);
			//assert (0);
		}
		else
		{
			// 从该Cache文件自己的Cache块链表中找到可替换的Cache块
			// 遍历该Cache文件的Cache块链表，查找可替换的Cache块
			block = SearchReplacedCacheBlockFromCacheFile (cache_file);
			if(block)
				break;
		}

		e_ticket = XM_GetTickCount ();
		/*
		if( (e_ticket - s_ticket) > 100 )
		{
			H264_FATAL  ("CacheBlockAlloc delay long time, please check\n");
		}
		*/
		
		// 等待Cache文件任务将其他脏块写入到文件系统
		XM_Delay (1);
	}
	
#ifdef CACHE_DEBUG
	if(block->id != H264_CACHE_BLCK_ID)
	{
		XM_printf ("cache block 0x%08x break\n", block);
		cache_debug_assert ();
	}
#endif

	e_ticket = XM_GetTickCount ();
		
	if( (e_ticket - s_ticket) > 3000)
	{
		// 投递消息，视频项写入速度低，提示用户处理
		//XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_VIDEOITEM_LOW_SPEED);
		H264_FATAL  ("CacheBlockAlloc delay long time(%d), off=0x%08x\n", (e_ticket - s_ticket), h264_offset);
	}
	else if( (e_ticket - s_ticket) > 300)
	{
		H264_FATAL  ("CacheBlockAlloc delay long time(%d), off=0x%08x\n", (e_ticket - s_ticket), h264_offset);
	}
	
	if( (e_ticket - s_ticket) > maximum_block_alloc_delay_time)
	{
		maximum_block_alloc_delay_time = (e_ticket - s_ticket);
		H264_PRINT ("maximum_block_alloc_delay_time=%d\n", maximum_block_alloc_delay_time);
	}
	
	block->full_flag = 0;
	block->dirty_flag = 0;
	block->lock_flag = 1;		// Cache块刚分配时自动锁定，防止被替换
	block->cache_file = cache_file;
	block->file_offset = h264_offset;

	block->dirty_start_addr = h264_offset;
	block->dirty_end_addr = h264_offset;

	block->reload_flag = 0;
	
	return block;
}

// 从Cache文件中查找一个可用的块，并将该块从Cache文件的Cache块链表中断开
static H264_CACHE_BLOCK *SearchReplacedCacheBlockFromCacheFile (H264_CACHE_FILE *cache_file)
{
	H264_CACHE_BLOCK *block = NULL;
	if(cache_file == NULL)
		return NULL;
	
	// 第一遍扫描，检查是否有可替换的空闲块(非脏、非锁定）
	SEMA_LOCK (&cache_file->sema);
	if(queue_empty ((queue_s *)&cache_file->cache_block))
	{
		SEMA_UNLOCK (&cache_file->sema);
		return NULL;
	}
	
	block = (H264_CACHE_BLOCK *)queue_next ((queue_s *)&cache_file->cache_block);
	while(block != (H264_CACHE_BLOCK *)&cache_file->cache_block)
	{
		int has_free_block = 0;
		SEMA_LOCK (&block->usage_sema);
#ifdef CACHE_DEBUG
		if(block->id != H264_CACHE_BLCK_ID)
		{
			XM_printf ("cache block 0x%08x break\n", block);
			cache_debug_assert ();
		}
#endif
		
		if(!block->dirty_flag && !block->lock_flag && !block->header_flag)
		{
			has_free_block = 1;
		}
		SEMA_UNLOCK (&block->usage_sema);
		
		if(has_free_block)
			break;
		
		block = (H264_CACHE_BLOCK *)queue_next ((queue_s *)block);
	}

	if(block != (H264_CACHE_BLOCK *)&cache_file->cache_block)
	{
		// 将找到的Cache块从Cache文件的Cache块链表中断开
		queue_delete ((queue_s *)block);
	}
	else
	{
		block = NULL;
	}
		
	SEMA_UNLOCK (&cache_file->sema);
	return block;
}

// 查找指定偏移的Cache块是否存在。
// 若Cache块在Cache文件的Cache块链表中存在，
//		1) 将该块锁定、
//		2)	移到链表尾部
//		3)	返回
// 若不存在
//		1) 锁定Cache文件链表访问信号量
//		2)	分配
//static H264_CACHE_BLOCK *LocateCacheBlockFromFileOffset (H264_CACHE_FILE *cache_file, size_t h264_offset, int read_data_from_file)
//{
//
//}

// 在Cache文件的Cache块链表中查找指定流位置的Cache块是否存在
// 有且仅被一个H264编码任务调用，
static H264_CACHE_BLOCK *LocateCacheBlockByFilePosition (H264_CACHE_FILE *cache_file, size_t h264_offset)
{
	H264_CACHE_BLOCK *block = NULL;
	H264_CACHE_BLOCK *loop;
	SEMA_LOCK (&cache_file->sema);		// 访问文件的块链表
	do
	{
		// 检查流位置是否位于当前Cache块中
		/*
		if(cache_file->current_block)
		{
			if(	h264_offset >= cache_file->current_block->file_offset
				&& h264_offset < (cache_file->current_block->file_offset + H264_CACHE_FILE_BLOCK_SIZE))
			{
				// 定位在当前块中
				block = cache_file->current_block;
				break;
			}
		}
		*/

		// 搜素Cache文件的Cache块，检查流位置对应的Cache块是否存在
		loop = (H264_CACHE_BLOCK *)queue_next ((queue_s *)&cache_file->cache_block);
		while(loop != (H264_CACHE_BLOCK *)&cache_file->cache_block)
		{
#ifdef CACHE_DEBUG
			if(loop->id != H264_CACHE_BLCK_ID)
			{
				XM_printf ("cache block 0x%08x break\n", loop);
				cache_debug_assert ();
			}
#endif
			if(h264_offset >= loop->file_offset && h264_offset < (loop->file_offset + H264_CACHE_FILE_BLOCK_SIZE))
			{
				block = loop;
				break;
			}
			loop = (H264_CACHE_BLOCK *)queue_next((queue_s *)loop);
		}
	} while (0);

	SEMA_UNLOCK (&cache_file->sema);

	return block;
}


// 设置Cache文件的当前活跃Cache块
static void CacheFileSetCurrentBlock (H264_CACHE_BLOCK *block, H264_CACHE_FILE *cache_file)
{
	H264_CACHE_BLOCK *current_block;
	if(block == NULL || cache_file == NULL)
	{
		XM_printf ("CacheFileSetCurrentBlock NG, block=%x, cache_file=%x\n", block, cache_file);
		return;
	}
	
	SEMA_LOCK (&cache_file->sema);
#ifdef CACHE_DEBUG
	if(block->id != H264_CACHE_BLCK_ID)
	{
		XM_printf ("cache block 0x%08x break\n", block);
		cache_debug_assert ();
	}
#endif
	if(block == cache_file->current_block)
	{
		SEMA_UNLOCK (&cache_file->sema);
		return;
	}

	// 检查Cache文件的当前Cache块是否已设置
	if(cache_file->current_block == NULL)
	{
		cache_file->current_block = block;
		SEMA_UNLOCK (&cache_file->sema);
		return;
	}

	// 当前块已设置
	current_block = cache_file->current_block;
	cache_file->current_block = block;
	SEMA_UNLOCK (&cache_file->sema);

	// 检查当前块是否脏块
	SEMA_LOCK (&current_block->usage_sema);
	if(!current_block->dirty_flag)
	{
		// 非脏块
		SEMA_UNLOCK (&current_block->usage_sema);
		return;
	}
	// 检查块是否已写满
	if(!current_block->full_flag)
	{
		// 非满块，暂时不执行写入操作
		SEMA_UNLOCK (&current_block->usage_sema);
		return;
	}
	
	SEMA_UNLOCK (&current_block->usage_sema);
	
	// 将老的活跃Cache块提交到Cache文件任务，执行异步文件写入操作
	xm_h264_cache_command_queue_insert_command (cache_file->index, H264_CACHE_FILE_CMD_WRITE, current_block);
}

// 将Cache块插入到Cache块链的尾部
static void InsertCacheBlockToTail (H264_CACHE_BLOCK *block, H264_CACHE_FILE *cache_file)
{
	if(block == NULL || cache_file == NULL)
	{
		XM_printf ("InsertCacheBlockToTail NG, block=%x, cache_file=%x\n", block, cache_file);
		return;
	}
	SEMA_LOCK (&cache_file->sema);
#ifdef CACHE_DEBUG
	if(block->id != H264_CACHE_BLCK_ID)
	{
		XM_printf ("cache block 0x%08x break\n", block);
		cache_debug_assert ();
	}
#endif
	queue_insert ((queue_s *)block, (queue_s *)&cache_file->cache_block);
	SEMA_UNLOCK (&cache_file->sema);
}

// 将Cache块中的脏内容刷新到文件系统中去(由Cache文件任务调用)
// update_fat_cache	是否刷新FAT cache到文件系统
static int CacheBlockFlushDirtyContextToFileSystem (H264_CACHE_BLOCK *block, int update_fat_manage_cache)
{
	int ret = 0;
	size_t begin, end;
	H264_CACHE_FILE *cache_file;
	if(block == NULL)
		return 0;
	
	// Cache块文件写入时，2个信号量锁定保持与流数据写入同样的锁定与解锁顺序，防止死锁发生
	// 锁定，防止外部Cache写入
	SEMA_LOCK (&block->usage_sema);
		
	if(block->dirty_flag == 0)
	{
		SEMA_UNLOCK (&block->usage_sema);
		return 0;
	}

#ifdef CACHE_DEBUG
	if(block->id != H264_CACHE_BLCK_ID)
	{
		XM_printf ("cache block 0x%08x break\n", block);
		cache_debug_assert ();
	}
#endif
	
	block->dirty_flag = 0;

	do
	{
		// 允许ACC下电时将已有数据流写入
#if 0
		if(xm_power_check_acc_safe_or_not() == 0)
		{
			XM_printf ("CacheFlush failed, bad acc\n"); 
			ret = -1;
			break;
		}
#endif
		
		// 存在脏区
		// 计算脏区的扇区对齐地址
		// 检查块是否已写满。若未写满，则该块的刷新是由文件关闭导致。
		// 为避免文件系统碎片的产生, 文件系统的写入以CACHE块为单位分配、写入、删除
		// 因此需将未写满的尾块补0写满
		// 经测试，改善无效果。多任务下，不同线程的并发写入同样导致碎片的产生。
		begin = block->dirty_start_addr & ~511;
		if(block->full_flag)
		{
			end = block->dirty_end_addr;
			end += 511;
			end = end & ~511;
		}
		else
		{
			size_t offset = (size_t)block->dirty_end_addr;
			offset = offset - (size_t)block->file_offset;
			memset (block->buffer + offset, 
					  0, 
					  H264_CACHE_FILE_BLOCK_SIZE - offset );
			end = block->file_offset + H264_CACHE_FILE_BLOCK_SIZE;
		}

		// 写文件
		assert (end > begin);
		// 获取Cache块的Cache文件句柄
		cache_file = (H264_CACHE_FILE *)block->cache_file;
		assert (cache_file);
		if (cache_file->fd == NULL)	// 文件句柄检查
		{
			ret = -1;
			break;
		}
		
		// 此时清除脏区，并解锁write_sema，允许H264 Codec在文件写入中同步修改数据流（并修改脏区），
		// 改善因锁定write_sema而导致H264 Codec任务挂起的情况		
		if(cache_file->error == 0)
		{
			int to_write;
			unsigned int ticket, ticket_seek;
			static unsigned int max_block_write_ticket = 0;
			static unsigned int max_block_seek_ticket = 0;
			// 锁定文件的文件IO互斥访问信号量，避免写入/读出过程中文件指针修改
			SEMA_LOCK (&cache_file->sema_file_io);

			// 定位并写入文件系统
			ticket = XM_GetTickCount ();
			// 文件系统安全访问锁定
			if(XMSYS_file_system_block_write_access_lock () == 0)
			{
				// 安全访问
				if(XM_fseek ( (void *)cache_file->fd, begin, XM_SEEK_SET) < 0)
				{
					cache_file->error = -1;	// 标记错误
					XMSYS_file_system_block_write_access_unlock ();
					// 文件IO互斥访问解锁
					SEMA_UNLOCK (&cache_file->sema_file_io);
					XM_printf ("CacheBlockFlushDirtyContextToFileSystem failed, fseek NG\n");
					ret = -1;
					break;
				}
				ticket_seek = XM_GetTickCount () - ticket;
				to_write = XM_fwrite (block->buffer + begin - block->file_offset, 1, end - begin, (void *)cache_file->fd);
			}
			else
			{
				// 非安全访问
				cache_file->error = -1;	// 标记错误
				XMSYS_file_system_block_write_access_unlock ();
				// 文件IO互斥访问解锁
				SEMA_UNLOCK (&cache_file->sema_file_io);
				XM_printf ("CacheBlockFlushDirtyContextToFileSystem failed, un-safe io\n");
				ret = -1;
				break;
			}
			// 文件系统安全访问解锁
			XMSYS_file_system_block_write_access_unlock ();
			ticket = XM_GetTickCount () - ticket;
			
			if(ticket_seek > max_block_seek_ticket)
			{
				max_block_seek_ticket = ticket_seek;
				//XM_printf ("max block seek ticket = %d\n", max_block_seek_ticket);
			}
			
			if(ticket > 500)
			{
				//XM_printf ("block write offset=0x%x, len=0x%x, seek=%d, ticket = %d\n", begin, end - begin, ticket_seek, ticket);
			}
			if(ticket > max_block_write_ticket)
			{
				max_block_write_ticket = ticket;
				XM_printf ("max block write ticket = %d\n", max_block_write_ticket);
			}
			if( to_write != (int)(end - begin))
			{
				I16 err_code;
				cache_file->error = -1;
				ret = -1;
				XM_printf ("CacheBlockFlushDirtyContextToFileSystem failed,(%s), begin=0x%08x, offset=0x%08x, (%d != %d)\n", 
						  cache_file->filename,
						  (int)begin, block->file_offset, to_write, (int)(end - begin));
				err_code = FS_FError ((void *)cache_file->fd);
				if(err_code == FS_ERR_DISKFULL)
				{
					// 检查卷是否存在
					if(FS_IsVolumeMounted("mmc:0:"))
					{
						// 磁盘写满
						XM_printf ("Error, FS diskfull\n");
						XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_DISK_FULL);
						ret = -2;
						cache_file->error = -2;
					}
					else
					{
						// 假报警
					}
				}
				else if(err_code == FS_ERR_WRITEERROR)
				{
					if(FS_IsVolumeMounted("mmc:0:"))
					{
						XM_printf ("Error, FS write NG\n");
						XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_VERIFY_ERROR);
					}
				}
				if(err_code != FS_ERR_OK)
				{
					FS_ClearErr((void *)cache_file->fd);
				}
			}
			// 检查卡校验时间是否已过期
			else if(cache_file->error == 0 && next_ticket_to_verify_file_system < XM_GetTickCount ())
			{
				//XM_printf ("verify fseek\n");
				// 检查cache是否忙(缓存的数据等待写入), 若是, 继续延时处理
				if(cache_file_system_busy_block_count <= 2)
				{
					// 文件系统安全访问锁定
					if(XMSYS_file_system_block_write_access_lock () == 0)
					{
						if(XM_fseek ( (void *)cache_file->fd, begin, XM_SEEK_SET) >= 0)
						{
							// 文件内容校验
							//XM_printf ("verify start\n");
							unsigned int verify_size = (end - begin);
							if(verify_size > sizeof(verify_buffer))
								verify_size = sizeof(verify_buffer);
							if( (unsigned int)(block->buffer + begin - block->file_offset) >= 0x84000000)
							{
								XM_printf ("illegal address, block=0x%08x, begin=0x%08x\n", 
											  (unsigned int)block, begin);
								//assert (0);
							}
							if(XM_fverify ((void *)cache_file->fd, 
													block->buffer + begin - block->file_offset, 
													verify_buffer, verify_size) != 0)
							{
								// 文件系统回读校验失败。SD卡需要更换
								//XM_printf ("FS verify NG\n");
								// 投递卡校验失败错误，提醒更新SD卡
								if(FS_IsVolumeMounted("mmc:0:"))
								{
									XM_printf ("Error, FS(%s) verify NG, offset=0x%08x, begin=0x%08x, size=%d\n", 
												  cache_file->filename ,block->file_offset, begin, verify_size);
									XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_VERIFY_ERROR);
								}
							}
						}
					}
					XMSYS_file_system_block_write_access_unlock ();
				
					// 设置下次文件系统校验的时间
					next_ticket_to_verify_file_system = XM_GetTickCount () + FS_VERIFY_CYCLE_MS;
				}
			}
			// 文件IO互斥访问解锁
			SEMA_UNLOCK (&cache_file->sema_file_io);
		}
		else
		{
			ret = -1;
		}
	} while (0);
	
	// 文件系统采用以下两个策略进行测试
	// 1) FS_CACHE_SetMode("", FS_SECTOR_TYPE_MASK_DIR|FS_SECTOR_TYPE_MASK_MAN, FS_CACHE_MODE_WT);
	//	  ticket=144645  226MB  1.56MB/s   金士顿8GB TF卡
	//    ticket=151163  226MB  1.50MB/s   金士顿2GB SD卡
	//                          1.66MB/s   Class 4 SD卡
	//	2) FS_CACHE_SetMode("", FS_SECTOR_TYPE_MASK_DIR|FS_SECTOR_TYPE_MASK_MAN, FS_CACHE_MODE_WB);	
	//    每个Cache块写入时执行FS_CACHE_Clean
	//	  ticket=113198  226MB  2MB/s      金士顿8GB TF卡
	//    ticket=226280  226MB  1MB/s      金士顿2GB SD卡
	//                          2.3MB/s    Class 4 SD卡
	// 3) FS_CACHE_SetMode("", FS_SECTOR_TYPE_MASK_DIR|FS_SECTOR_TYPE_MASK_MAN, FS_CACHE_MODE_WB);
	//	  每4个Cache块写入时执行FS_CACHE_Clean (1MB)
	//    ticket=103025  226MB  2.19MB/s   金士顿8GB TF卡
	//    ticket=99768   226MB  2.26MB/s   金士顿2GB SD卡
	//                          2.72MB/s   Class 4 SD卡
	//
	//	1) 策略1的写入平均速度为1.66MB/s, 策略2的写入平均速度为2.3MB/s, 策略3的写入平均速度为2.72MB/s,
	// 2) 策略1与策略2的读次数与读出字节相同
	// 3) 策略1与策略2的写次数与写入字节不相同，策略1比策略2减少16%的写入次数
	
	// 视频文件关闭时，每个Cache块的刷新过程不再强制同步更新FAT管理区(簇表，目录表)，
	// 而是在文件关闭时将FAT管理区写入，减少磁盘的写入次数，加速文件关闭过程。
	if(update_fat_manage_cache == 0)
		goto cache_flush_end;
	
	// 文件系统安全访问锁定
	if(XMSYS_file_system_block_write_access_lock () == 0)
	{
#if 1
		// 新的FAT表更新写入策略
		// Cache系统越忙, 更新FAT表的间隔越大, 这样可减少FAT表项的写入次数, 缓解Cache内容无法实时写入的状况.
		// 一般码流大的时候, Cache会较忙. 此时适当延长FAT表项更新的时间, 减少FAT表项写入的次数, 这样多出的写入次数被用于内容的写入.
		unsigned int busy_level = XMSYS_GetCacheSystemBusyLevel ();
		unsigned int fat_update_size = H264_CACHE_FILE_BLOCK_SIZE;	// 缺省为Cache块大小, 即每个cache块写入时均更新FAT表项
		if(busy_level <= 2)
			fat_update_size = H264_CACHE_FILE_BLOCK_SIZE;
		else 
			fat_update_size = H264_CACHE_FILE_BLOCK_SIZE * busy_level;
		//if(!(block->file_offset & (fat_update_size - 1)))
		// 倍数时更新
		if( (block->file_offset % fat_update_size) == 0 )
		{
			FS_CACHE_Clean ("");
		}
#else
		// 每个Cache块均更新FAT表项写入
		//if(!(block->file_offset & (0x100000 - 1))) // 1MB刷新
		//if(!(block->file_offset & (0x40000 - 1)))	// 256KB刷新
		//if(!(block->file_offset & (0x20000 - 1)))	// 256KB刷新
		{
			FS_CACHE_Clean ("");
		}
#endif
	}
	// 文件系统安全访问解锁
	XMSYS_file_system_block_write_access_unlock ();	

	// 清除脏区标志.
	// 注意: Cache块替换算法通过判断dirty_flag来判断是否可重利用
	//block->dirty_flag = 0;

cache_flush_end:
	// 释放块访问信号量
	SEMA_UNLOCK (&block->usage_sema);
	
	/*
	if(ret == 0)
	{
		U32 FS_PreFindFreeClusters (FS_FILE * pFile, U32 BlockSize);
		int i;
		for (i = 0; i < 2; i ++)
		{
			if(cache_file_system_busy_block_count <= 2)
				FS_PreFindFreeClusters ((FS_FILE *)cache_file->fd, H264_CACHE_FILE_BLOCK_SIZE);
		}
	}
	*/
		
	return ret;
}



// 创建一个H264 Cache文件对象(H264 Codec线程调用)
static H264_CACHE_FILE *CreateCacheFile (const char *filename)
{
	int i;
	H264_CACHE_FILE *cache_file = NULL;
	
	do
	{
		SEMA_LOCK (&h264CacheFileSema); /* Make sure nobody else uses */
			
		// 分配一个Cache单元
		for (i = 0; i < H264_CACHE_FILE_COUNT; i++)
		{
			if(h264CacheFileHandle[i].alloc == 0)
				break;
		}
		if(i == H264_CACHE_FILE_COUNT)
		{
			SEMA_UNLOCK (&h264CacheFileSema);
			// H264_FATAL ("CreateCacheFile Failed, no more handles\n");	
			XM_Delay (10);
			continue;							
		}
		cache_file = h264CacheFileHandle + i;
	} while(cache_file == NULL);
	
	cache_file->fd = 0;						// 暂时不打开文件，避免文件创建时导致H264编码延迟
	cache_file->alloc = 1;					// 标记分配中	
	cache_file->error = 0;					// 标记无错误
	cache_file->index = i;
	cache_file->h264_offset = 0;
	cache_file->h264_length = 0;
	cache_file->closed = 0;
	memset (cache_file->filename, 0, sizeof(cache_file->filename));
	if(strlen(filename) > sizeof(cache_file->filename))
	{
		XM_printf ("CreateCacheFile failed, illegal filename\n");
		return NULL;
	}
	strcpy (cache_file->filename, filename);

	cache_file->seek_times = 0;
	cache_file->block_read_times = 0;
	cache_file->block_read_bytes = 0;
	cache_file->average_seek_delay = 0;
	cache_file->maximum_seek_delay = 0;

	cache_file->write_read_times = 0;
	cache_file->write_read_bytes = 0;
	cache_file->average_write_read_delay = 0;
	cache_file->maximum_write_read_delay = 0;

	// 初始化当前使用块为空
	cache_file->current_block = NULL;
	queue_initialize ((queue_s *)&cache_file->cache_block);
	
	// 创建基于文件的互斥信号量
	SEMA_CREATE (&cache_file->sema);
	SEMA_CREATE (&cache_file->sema_file_io);
	
	SEMA_UNLOCK (&h264CacheFileSema);
	//XM_printf ("CreateCacheFile %x\n", cache_file);
	return cache_file;	
}

// 删除一个H264 Cache文件对象(由Cache文件任务调用)
static int DeleteCacheFile (H264_CACHE_FILE *cacheFile)
{
	if(cacheFile == NULL)
		return 0;

	SEMA_LOCK (&h264CacheFileSema);
	// 检查Cache文件句柄是否合法
	if(cacheFile->alloc == 0)
	{
		H264_PRINT ("illegal Cache File(0x%08x), alloc=%d\n", cacheFile, cacheFile->alloc);
		SEMA_UNLOCK (&h264CacheFileSema);
		return 0;
	}
		
	// Cache文件已分配
	
	// 删除基于文件的互斥信号量
	SEMA_DELETE (&cacheFile->sema);
	SEMA_DELETE (&cacheFile->sema_file_io);
	
	// 释放所有Cache块
	cacheFile->alloc = 0;
	cacheFile->error = 0;
	cacheFile->closed = 0;
	SEMA_UNLOCK (&h264CacheFileSema);
	return 1;
}

// H264 Cache文件打开，由H264 Codec线程调用
H264_CACHE_FILE* H264CacheFileOpen (const char *filename)
{
	H264_CACHE_FILE *fp;
	H264_PRINT ("H264CacheFileOpen (%s)\n", filename);
	if(filename == NULL || *filename == 0)
	{
		H264_PRINT ("H264CacheFileOpen failed, invalid filename\n");
		return NULL;
	}
	
	fp = CreateCacheFile (filename);
	if(fp == NULL)
	{
		H264_PRINT ("H264CacheFileOpen (%s) failed\n", filename);
		return NULL;
	}
	// 通知H264流任务打开文件事件
	if(xm_h264_cache_command_queue_insert_command (fp->index, H264_CACHE_FILE_CMD_OPEN, NULL) == 0)
	{
		DeleteCacheFile (fp);
		H264_PRINT ("H264CacheFileOpen (%s) failed, Can't insert cache open command\n", filename);
		return NULL;
	}
	H264_PRINT ("Open %d\n", XM_GetTickCount());
	return fp;
}

// H264 Cache文件关闭，由H264 Codec线程调用
int H264CacheFileClose (H264_CACHE_FILE* cache_file)
{
	H264_CACHE_BLOCK *block;
	if(cache_file == NULL)
	{
		H264_PRINT ("H264CacheFileClose failed, cache_file == NULL\n");
		return -3;
	}
	SEMA_LOCK(&h264CacheFileSema); /* Make sure nobody else uses */
	// 检查H264 Cache文件句柄是否合法
	if(cache_file->alloc == 0)
	{
		H264_PRINT ("H264CacheFileClose NG, illegal Cache File(0x%08x), alloc=%d\n", 
			cache_file, cache_file->alloc);
		SEMA_UNLOCK (&h264CacheFileSema);
		return -3;
	}
	SEMA_LOCK (&cache_file->sema);	// 锁定Cache文件
	cache_file->closed = 1;		// 标记文件关闭中
	H264_PRINT ("H264CacheFileClose (%s) \n", cache_file->filename);

	
	SEMA_UNLOCK (&h264CacheFileSema);
	
#if 1
	// 扫描当前文件拥有的所有非脏Cache块并立刻释放，减少新文件申请Cache块时的延迟
	if(!queue_empty((queue_s *)&cache_file->cache_block))
	{
		block = (H264_CACHE_BLOCK *)queue_next ((queue_s *)&cache_file->cache_block);
		while((queue_s *)block != &cache_file->cache_block)
		{
			SEMA_LOCK (&block->usage_sema);
#ifdef CACHE_DEBUG
			if(block->id != H264_CACHE_BLCK_ID)
			{
				XM_printf ("cache block 0x%08x break\n", block);
				cache_debug_assert ();
			}
#endif
			
			if(block->dirty_flag || block->lock_flag)		// 脏块或已锁定
			{
				SEMA_UNLOCK (&block->usage_sema);
				block = (H264_CACHE_BLOCK *)queue_next ((queue_s *)block);
			}
			else
			{
				// 非脏块，丢弃
				SEMA_UNLOCK (&block->usage_sema);
				H264_CACHE_BLOCK *next = (H264_CACHE_BLOCK *)queue_next ((queue_s *)block);
				queue_delete ((queue_s *)block);	// 断链
				CacheBlockFree (block);
				block = next;
			}
		}
	}
#endif
	
	SEMA_UNLOCK (&cache_file->sema);

	// 将Cache文件关闭命令投递到命令队列
	if(xm_h264_cache_command_queue_insert_command (cache_file->index, H264_CACHE_FILE_CMD_CLOSE, NULL) == 0)
	{
		H264_PRINT ("H264CacheFileClose (%s) failed, Can't insert cache close command\n", cache_file->filename);
		return -3;
	}
	return 0;
}

// H264 Cache文件定位，仅由H264 Codec线程调用，且仅有一个H264编码任务同时调用
// 此处可以将多任务的保护机制禁止
XMINT64 H264CacheFileSeek (H264_CACHE_FILE* cache_file, XMINT64 off_64, int whence)
{
	size_t h264_offset;

	// 等待该Cache文件之前的命令全部执行完毕
	if(cache_file == NULL)
	{
		H264_PRINT ("H264CacheFileSeek NG, NULL Cache File\n");
		return (-3);
	}

	if(cache_file->alloc == 0)
	{
		H264_PRINT ("H264CacheFileSeek NG, illegal Cache File(0x%08x), alloc=%d\n", 
			cache_file, cache_file->alloc);
		return (-3);
	}
	
	// SEMA_LOCK (&cache_file->sema);
	cache_file->seek_times ++;
	
	//XM_printf ("off_64=%d, whence=%d\n", (int)off_64, whence);
	// 计算当前的流定位位置
	if(whence == XM_SEEK_CUR)
	{
		// 根据相对偏移计算
		h264_offset = cache_file->h264_offset + (int)off_64;
	}
	else if(whence == XM_SEEK_SET)
	{
		// 定位到流指定位置
		h264_offset = (int)off_64;
	}
	else if(whence == XM_SEEK_END)
	{
		// 定位到流尾部
		h264_offset = cache_file->h264_length + (int)off_64;
	}
	else
	{
		H264_FATAL ("Cache File Seek Failed, illegal origin(%d)\n", whence);
		// SEMA_UNLOCK (&cache_file->sema);
		return -3;
	}

	// 检查IO是否异常
	if(cache_file->error)
	{
		// 20140805 ZhuoYongHong
		// 文件IO异常时返回错误
		return -3;
		
		// 文件IO异常时不返回错误
		// SEMA_UNLOCK (&cache_file->sema);
		// return h264_offset;
	}

	// 计算新的流位置是否超出流当前的已编码字节长度（这个版本中不允许）
	if(h264_offset > cache_file->h264_length)
	{
		H264_FATAL ("Cache File Seek Failed, current version can't support the offset(%d) exceed stream maximum length(%d)\n",
			h264_offset, cache_file->h264_length);	
		// SEMA_UNLOCK (&cache_file->sema);
		return -3;
	}

	cache_file->h264_offset = h264_offset;
	// SEMA_UNLOCK (&cache_file->sema);
	return h264_offset;
}

// H264 Cache文件读，由H264 Codec线程调用
// 当前版本不支持
int H264CacheFileRead (unsigned char *buf, size_t size, size_t nelem, H264_CACHE_FILE *fd)
{
	// 等待该Cache文件之前的命令全部执行完毕
	H264_FATAL ("Current Version Don't support H264 read operation\n");
	return -3;
}

// H264 Cache文件写，仅由一个H264 Codec线程调用
int H264CacheFileWrite (unsigned char *buf, size_t size, size_t nelem, H264_CACHE_FILE *cache_file)
{
	size_t count;
	size_t to_copy;
	unsigned int offset;
	size_t h264_offset;
	size_t block_file_offset;	// 块的文件偏移基址
	H264_CACHE_BLOCK *block;
	int submit_to_write;
	
	unsigned long w_s_ticket, w_e_ticket;
				
	if(cache_file == NULL)
		return (-3);
	

	if(cache_file->alloc == 0)
	{
		//H264_PRINT ("Cache File write Failed due to illegal Cache File\n");
		return (-3);
	}
	
	if(cache_file->error)
	{
		// 20140802 ZhuoYongHong
		// 修改为返回异常，使得记录过程马上退出，并切换到无卡记录模式
		// IO异常时Cache写入继续返回成功
		//H264_PRINT ("Cache File Write Failed due to file io error\n");
		// return nelem;
		if(cache_file->error == (-2))
			return (-2);
		return (-3);
	}
	
	w_s_ticket = XM_GetTickCount();
	
	h264_offset = cache_file->h264_offset;

	count = size * nelem;
	while(count)
	{
		// 计算Cache块内的偏移
		block_file_offset = h264_offset & ~(H264_CACHE_FILE_BLOCK_SIZE - 1);
		// 计算当前可写入的字节数 to_copy
		to_copy = H264_CACHE_FILE_BLOCK_SIZE - (h264_offset - block_file_offset);
		assert (to_copy > 0);
		assert (to_copy <= H264_CACHE_FILE_BLOCK_SIZE);
		// 计算写入的偏移位置offset及写入字节大小to_copy
		offset = h264_offset - block_file_offset;
		assert (offset < H264_CACHE_FILE_BLOCK_SIZE);
		if(to_copy > count)
			to_copy = count;

		// 根据偏移值定位Cache块单元
		SEMA_LOCK (&cache_file->sema);
		block = LocateCacheBlockByFilePosition (cache_file, h264_offset);
		if(block)
		{
			// 从Cache块链表中断开
#ifdef CACHE_DEBUG
			if(block->id != H264_CACHE_BLCK_ID)
			{
				XM_printf ("cache block 0x%08x break\n", block);
				cache_debug_assert ();
			}
#endif
			queue_delete ((queue_s *)block);
			// 将Cache块插入到Cache文件的Cache块链表的尾部
			InsertCacheBlockToTail (block, cache_file);
			SEMA_UNLOCK (&cache_file->sema);
		}
		else
		{
			size_t file_offset;
			// 块不存在。分配新的Cache块
			SEMA_UNLOCK (&cache_file->sema);
			block = CacheBlockAlloc (cache_file, h264_offset);
			// assert (block);
			if(block == NULL)
			{
				cache_file->error = -1;
				H264_PRINT ("H264CacheFileWrite failed due to CacheBlockAlloc failed\n");
				return -3;
			}
			
			// 检查Cache块的文件偏移地址是否小于当前流的最大字节大小。
			// 若小于，则需从文件系统读取已被写入的流数据
			file_offset = block->file_offset;
			if( file_offset < cache_file->h264_length)
			{
				// 该Cache块已被写入到磁盘文件, 且被替换出去
				// 从文件系统中重新读取。
				unsigned int s_ticket, e_ticket;
				// 注意：此处会导致H264编码线程阻塞，需统计其信息进行评估
				size_t length = cache_file->h264_length - file_offset;
				int ret;
				assert (length >= H264_CACHE_FILE_BLOCK_SIZE);
				if(length > H264_CACHE_FILE_BLOCK_SIZE)
				{
					length = H264_CACHE_FILE_BLOCK_SIZE;
				}
				
				s_ticket = XM_GetTickCount();
				
				// 锁定文件IO信号量
				SEMA_LOCK (&cache_file->sema_file_io);
				
				unsigned int read_bytes;
				// 文件系统安全访问锁定
				if(XMSYS_file_system_block_write_access_lock() == 0)
				{
					ret = XM_fseek ((void *)cache_file->fd, block->file_offset, XM_SEEK_SET);
					if(ret < 0)
					{
						cache_file->error = -1;
						XMSYS_file_system_block_write_access_unlock();
						SEMA_UNLOCK (&cache_file->sema_file_io);
						CacheBlockFree (block);
						XM_printf ("H264CacheFileWrite failed due to XM_fseek error (ret=%d)\n", ret);
						return -3;
					}
					read_bytes = XM_fread (block->buffer, 1, H264_CACHE_FILE_BLOCK_SIZE, (void *)cache_file->fd);
				}
				else
				{
					read_bytes = 0;
				}
				// 文件系统安全访问锁定解除
				XMSYS_file_system_block_write_access_unlock();
				if(read_bytes != length)
				{
					cache_file->error = -1;
					SEMA_UNLOCK (&cache_file->sema_file_io);
					CacheBlockFree (block);
					XM_printf ("H264CacheFileWrite failed due to XM_fread error, length=%d, read_bytes=%d \n", length, read_bytes);
					return -3;
				}
				//释放文件IO信号量
				SEMA_UNLOCK (&cache_file->sema_file_io);

				// 标记块已写满
				block->full_flag = 1;
				
				e_ticket = XM_GetTickCount();
				// 更新Cache效率分析数据
				cache_file->write_read_times ++;
				cache_file->write_read_bytes += H264_CACHE_FILE_BLOCK_SIZE;
				if(cache_file->maximum_write_read_delay < (e_ticket - s_ticket))
				{
					cache_file->maximum_write_read_delay = e_ticket - s_ticket;
				}
				// 累加每次回读操作所延期的时间，该平均值反应Cache块替换时平均的阻塞时间
				cache_file->average_write_read_delay += (e_ticket - s_ticket);
			}
			
			// 将Cache块插入到Cache文件的Cache块链表的尾部
			InsertCacheBlockToTail (block, cache_file);
		}

		// assert (block_file_offset == block->file_offset);
		if(block_file_offset != block->file_offset)
		{
			assert (0);
		}
		
		// 设置当前Cache块
		//CacheFileSetCurrentBlock (block, cache_file);

		submit_to_write = 0;
        
		// Cache中存在空闲空间
		// 将流数据写入到Cache块中的流缓冲区
		// 流数据写入时，2个信号量锁定保持与Cache块文件写入同样的锁定与解锁顺序，防止死锁发生
		// "Cache块缓冲区流写入"及"Cache块文件写入"互斥保护
		SEMA_LOCK (&block->usage_sema);
		
		// 锁定文件IO信号量
		//SEMA_LOCK (&cache_file->sema_file_io);
		
		//if(offset & 3 || (((unsigned int)buf) & 3) || (to_copy & 3))
		{
			//XM_printf ("offset=0x%08x, buf=0x%08x\n, copy=0x%08x\n", offset, buf, to_copy);
		}
		extern void dma_memcpy (unsigned char *dst_buf, unsigned char *src_buf, unsigned int len);

		// 20170314 使用dma_memcpy可以较好改善系统效率, 如UVC帧率明显改善.
		// dma_memcpy使用时不能置为lock标志, 会导致ISP bus bandwidth abnormal
#ifdef UVC_PIO		
		memcpy (block->buffer + offset, (const void *)buf, to_copy);
#else
		dma_memcpy ((unsigned char *)block->buffer + offset, (unsigned char *)buf, to_copy);
#endif
		
		// 更新脏区域
		if(block->dirty_flag == 0)
		{
			// 脏区域不存在
			block->dirty_start_addr = h264_offset;
			block->dirty_end_addr = h264_offset + to_copy;
			block->dirty_flag = 1;
			
			get_dirty_cache_block_count ();
		}
		else
		{
			// 脏区域已存在
			if(block->dirty_start_addr > h264_offset)
				block->dirty_start_addr = h264_offset;
			if(block->dirty_end_addr < (h264_offset + to_copy))
				block->dirty_end_addr = (h264_offset + to_copy);
		}
		// 检查是否已满
		if(!block->full_flag)
		{
			size_t dirty_size = block->dirty_end_addr;
			dirty_size = dirty_size - block->file_offset;
			if(dirty_size == H264_CACHE_FILE_BLOCK_SIZE)
			{
				// 块已写满数据
				// Cache块首次写满时，将其提交到Cache文件系统进行文件写入操作
				block->full_flag = 1;
				// 标记需要提交到Cache文件任务进行文件写入
				submit_to_write = 1;
				// 块第一次分配时自动锁定，直到块写满，锁定清除。
				// 第1个块(文件偏移0)强制锁定，因为H264文件关闭时会修改文件头
				if(block->file_offset)
				{
					// 不是第一个块，块写满时自动解锁
					block->lock_flag = 0;	
				}
			}
		}		
		
		count -= to_copy;
		buf += to_copy;
		h264_offset += to_copy;
		// 更新Cache文件流指针
		cache_file->h264_offset = h264_offset;
		// 更新已写入流字节长度
		if(h264_offset > cache_file->h264_length)
			cache_file->h264_length = h264_offset;
		
		// 解锁文件IO信号量
		//SEMA_UNLOCK (&cache_file->sema_file_io);
		SEMA_UNLOCK (&block->usage_sema);
		
		// 检查是否需要将Cache块排队执行异步写入操作
		if(submit_to_write)
		{
			// 将该Cache块提交到Cache文件任务，执行异步文件写入操作
			// xm_h264_cache_command_queue_insert_command (cache_file->index, H264_CACHE_FILE_CMD_WRITE, block);	
		}

		// 设置当前Cache块
		CacheFileSetCurrentBlock (block, cache_file);
	} // while(count)
	
	w_e_ticket = XM_GetTickCount() - w_s_ticket;
	
	if(w_e_ticket > 100)
	{
		H264_PRINT ("%d Cache Write\n", w_e_ticket);
		if(w_e_ticket > max_write_time)
		{
			max_write_time = w_e_ticket;
			H264_PRINT ("%d Maximum  Cache Write\n", max_write_time);
		}
	}
		
	return nelem;
}

// Cache任务调用CacheFileFlush 刷新所有脏块并释放Cache块
// 将Cache文件的所有Cache块数据更新到文件系统并释放Cache块
static void CacheFileFlush (H264_CACHE_FILE *cache_file)
{
	H264_CACHE_BLOCK *block;
	int ret = 0;
	int is_diskfull = 0;

	// cache文件句柄合法性检查
	if(cache_file == NULL)
		return;
	if(cache_file->alloc == 0)
	{
		XM_printf ("cache_file %x alloc==0\n", cache_file);
		return;
	}

	assert (cache_file->closed == 1);

	// 检查Cache块链表
	SEMA_LOCK (&cache_file->sema);
	if(queue_empty ((queue_s *)&cache_file->cache_block))
	{
		SEMA_UNLOCK (&cache_file->sema);
		//XM_printf ("cache_file %x empty block\n", cache_file);
		return;
	}
	
	if(cache_file->error == (-2))
		is_diskfull = 1;

	while(!queue_empty((queue_s *)&cache_file->cache_block))
	{
		block = (H264_CACHE_BLOCK *)queue_delete_next ((queue_s *)&cache_file->cache_block);
#ifdef CACHE_DEBUG
		if(block->id != H264_CACHE_BLCK_ID)
		{
			XM_printf ("cache block 0x%08x break\n", block);
			cache_debug_assert ();
		}
#endif

		if(cache_file->fd && cache_file->error == 0)
		{
			// 视频文件关闭时，每个Cache块的刷新过程不再强制同步更新FAT管理区(簇表，目录表)，加速文件关闭过程
			ret = CacheBlockFlushDirtyContextToFileSystem (block, 0);
			if(ret == (-2))
				is_diskfull = 1;			
		}
		else
		{
			block->dirty_flag = 0;
		}
		// 将Cache块释放到空闲Cache块池
		CacheBlockFree (block);
	}
	SEMA_UNLOCK (&cache_file->sema);
	
	if(is_diskfull)
	{
		//OS_Delay (1000);
		XM_printf ("Cache File Closed, disk full\n"); 
	}
}

// 串行化操作会导致资源的长时间锁定。
// 
void  XMSYS_H264FileTask (void)
{
	int file_index;
	int cache_cmd;
	H264_CACHE_FILE *fp;
	void *cmd_param;
	H264_CACHE_BLOCK *block;
	//int is_diskfull;
	int error;
	unsigned int lock_flag;
	H264_CACHE_FILE *cache_file;
	
	H264_PRINT ("XMSYS_H264FileTask\n");
	
	while(1)
	{
		// 读取一个Cache命令
		int ret;
		void *fd;
		ret = xm_h264_cache_command_queue_remove_command (&file_index, &cache_cmd, &cmd_param);
		if(ret == 1)
		{
			// 获取一个Cache文件命令
			//XM_printf ("get command file=%d, cmd=%d\n", cache_file, cache_cmd);
#ifdef DEBUG
			assert ( file_index < H264_CACHE_FILE_COUNT );
			assert ( cache_cmd >= H264_CACHE_FILE_CMD_OPEN && cache_cmd <= H264_CACHE_FILE_CMD_CLOSE );
#endif
			SEMA_LOCK (&h264CacheFileSema); /* Make sure nobody else uses */
			fp = &h264CacheFileHandle[file_index];
			// 检查句柄是否已分配
			if( fp->alloc == 0 )
			{
				SEMA_UNLOCK (&h264CacheFileSema);
				H264_FATAL ("H264 File Internal Error, fp=%x(%d), fp->alloc == 0\n", fp, file_index);
				continue;
			}
			// 锁定针对CACHE文件的保护
			SEMA_UNLOCK (&h264CacheFileSema);
			
			switch (cache_cmd)
			{
				case H264_CACHE_FILE_CMD_OPEN:
					H264_PRINT ("H264_CACHE_FILE_CMD_OPEN\n");
					SEMA_LOCK (&fp->sema);
					// 文件打开操作
					if(fp->fd != NULL)
					{
						SEMA_UNLOCK (&fp->sema);
						H264_FATAL ("H264 File Internal Error, fd != NULL\n");
						continue;
					}
					// 检查文件系统是否存在
					XM_GetLocalTime (&fp->create_time);
					SEMA_UNLOCK (&fp->sema);
					fd = XM_fopen (fp->filename, "w+b");
					//fp->fd = XM_fopen (fp->filename, "w+b");
					SEMA_LOCK (&fp->sema);
					fp->fd = fd;
					if(fp->fd == NULL)
					{
						fp->error = (-1);
						XM_printf ("open cache file(%s) failed\n", fp->filename);
					}
					else
					{
						if(fp->error != 0)
						{
							XM_printf ("open cache file (%s) inernal error\n", fp->filename);
						}
						else
						{
							XM_printf ("open cache file (%s) success\n", fp->filename);
						}
					}
					SEMA_UNLOCK (&fp->sema);
					break;
				
				case H264_CACHE_FILE_CMD_CLOSE:
					// 文件关闭操作
					H264_PRINT ("H264_CACHE_FILE_CMD_CLOSE %x\n", fp);
					SEMA_LOCK (&fp->sema);
					XM_printf ("close cache file (%s), fd=0x%08x, error=%d\n", fp->filename, fp->fd, fp->error);
					
					error = fp->error;
					
					// 检查是否存在尚未写入的流数据，强制刷新并释放Cache块
					CacheFileFlush (fp);
					{
					int dirty_count = get_dirty_cache_block_count ();
					//XM_printf ("dirty_blk=%d\n", dirty_count);
					}
					if(fp->fd)
					{
						// 文件已成功打开
						if(fp->block_read_times)
						{
							H264_PRINT ("seeks times =%d\n", fp->seek_times);
							H264_PRINT ("block_read_times=%d, block_read_bytes=%d\n", fp->block_read_times, fp->block_read_bytes);
							H264_PRINT ("average_seek_delay=%d, maximum_seek_delay=%d\n",
								fp->average_seek_delay / fp->block_read_times,
								fp->maximum_seek_delay);
						}
						if(fp->write_read_times)
						{
							H264_PRINT ("write_read_times=%d, write_read_bytes=%d\n", fp->write_read_times, fp->write_read_bytes);
							H264_PRINT ("average_write_read_delay=%d, maximum_write_read_delay=%d\n",
								fp->average_write_read_delay / fp->write_read_times,
								fp->maximum_write_read_delay);
						}
						
						// 文件系统安全访问锁定
						if(XMSYS_file_system_block_write_access_lock() == 0)
						{
							XM_fclose ((void *)fp->fd);
							FS_CACHE_Clean("");
						}
						fp->fd = NULL;
						XMSYS_file_system_block_write_access_unlock();
						
	#ifdef H264_CACHE_FILE_TEST
	#else
						// 更新视频项数据库
						//H264_PRINT ("VideoItemUpdate %s\n", fp->filename);
						if(XM_VideoItemIsTempVideoItemFile(fp->filename))
							XM_VideoItemUpdateVideoItemFromTempName (fp->filename, fp->h264_length, &fp->create_time);
						else
							XM_VideoItemUpdateVideoItemFromFileName (fp->filename, fp->h264_length, &fp->create_time);
	#endif
					}
					SEMA_UNLOCK (&fp->sema);
					// 删除CacheFile对象
					DeleteCacheFile (fp);
					//H264_PRINT ("H264_CACHE_FILE_CMD_CLOSE end\n");
					if(error != 0)
					{
						// 文件存在错误，写入错误
						// 检查磁盘空间是否存在异常
						XM_printf ("File Error, forced RecycleGarbage\n");
						XM_VideoItemMonitorAndRecycleGarbage (0);
					}
					break;
				
				case H264_CACHE_FILE_CMD_WRITE:
					// 文件写入
					//H264_PRINT ("H264_CACHE_FILE_CMD_WRITE\n");
					block = (H264_CACHE_BLOCK *)cmd_param;		
					cache_file = NULL;
					// 阻止被替换
					//SEMA_LOCK (&fp->sema);	// 锁定Cache文件,
					SEMA_LOCK (&block->usage_sema);
					lock_flag = block->lock_flag;		// 保存
					block->lock_flag = 1;
					SEMA_UNLOCK (&block->usage_sema);
					
					// cache块单独写入时同步更新FAT管理区
					ret = CacheBlockFlushDirtyContextToFileSystem (block, 1);
					/*
					if(ret == (-2))
						is_diskfull = 1;
					else
						is_diskfull = 0;
					*/
					
					// Block刷新后可能会重新变脏. 因此释放时需要重新检查
					
					SEMA_LOCK (&fp->sema);	// 锁定Cache文件,
					SEMA_LOCK (&block->usage_sema);
					block->lock_flag = lock_flag;	// 恢复
					// 检查Cache块对应的Cache文件是否已投递文件关闭命令或者Cache块不是文件的第一个Cache块(header_flag)
					// 若是，将该Cache块丢弃
					
					// 需要重新确认该块为非脏块及锁定块
					if(!block->dirty_flag && !block->lock_flag && (fp->closed || !block->header_flag ))
					{
#ifdef CACHE_DEBUG
						if(block->id != H264_CACHE_BLCK_ID)
						{
							XM_printf ("cache block 0x%08x break\n", block);
							cache_debug_assert ();
						}
#endif
						cache_file = (H264_CACHE_FILE *)block->cache_file;

						// 从Cache块链表断开
						queue_delete ((queue_s *)block);

						// 释放Cache块资源
						CacheBlockFree (block);
					}
					// 释放块访问信号量
					SEMA_UNLOCK (&block->usage_sema);
					SEMA_UNLOCK (&fp->sema);
						
					// 文件系统安全访问锁定
					if(XMSYS_file_system_block_write_access_lock () == 0)
					{	
						// 可以安全访问
						// 检查是否存在异常
						if(ret == 0 && cache_file)
						{
							int dirty_count;
							// 无文件系统异常
							// 检查脏块的数量. 若脏块数量较小, 安排簇集扫描等任务
							//int i;
							//for (i = 0; i < 2; i ++)
							{
								U32 FS_PreFindFreeClusters (FS_FILE * pFile, U32 BlockSize);
								dirty_count = get_dirty_cache_block_count ();
								//XM_printf ("dirty_blk=%d\n", dirty_count);
								if(dirty_count <= 2)
								{
									FS_PreFindFreeClusters ((FS_FILE *)cache_file->fd, H264_CACHE_FILE_BLOCK_SIZE);
								}
							}
						}
					} 
					// 文件系统安全访问解锁
					XMSYS_file_system_block_write_access_unlock ();
					break;
			}
		}	// if(file_event & XMSYS_H264_CACHE_FILE_COMMAND_EVENT)
	}
}



static int h264file_inited = 0;

void XMSYS_H264FileInit (void)
{
	if(h264file_inited)
	{
		XM_printf ("WARNING, execute XMSYS_H264FileInit again\n");
		return;
	}
	
	//XM_printf ("XMSYS_H264FileInit\n");
	
	// 设置下一次写读校验的时间
	next_ticket_to_verify_file_system = XM_GetTickCount () + FS_VERIFY_CYCLE_MS;

	SEMA_CREATE (&h264CacheFileSema); /* Creates resource semaphore */
	xm_h264_file_init ();

	// 初始化命令队列
	xm_h264_cache_command_queue_init ();
	
	CacheBlockInit ();
		
	// 创建Cache文件服务任务
	OS_CREATETASK(&TCB_H264FileTask, "H264FileTask", XMSYS_H264FileTask, XMSYS_H264File_TASK_PRIORITY, StackH264FileTask);
	
	h264file_inited = 1;
}

void XMSYS_H264FileExit (void)
{
	
}

