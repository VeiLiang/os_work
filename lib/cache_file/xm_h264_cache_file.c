//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_h264_cache_file.c
//	  ������ӿ�, �������ļ���cache����/�滻/д��/��ȡ
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
#define	H264_CACHE_SIZE					0x500000		// ����Cache��Ļ�����memory�ֽڴ�С
#else
#define	H264_CACHE_SIZE					0x1000000//0xE00000
#endif


#define	H264_CACHE_BLOCK_COUNT			(H264_CACHE_SIZE/H264_CACHE_FILE_BLOCK_SIZE)


// ��������
// д��16MB
// 1) H264_CACHE_FILE_BLOCK_SIZE == 128
// 7.2s

//#define	FS_VERIFY_CYCLE_MS		(20*1000)		// �ļ���дУ��������(����)
#define	FS_VERIFY_CYCLE_MS		(2*1000)		// �ļ���дУ��������(����)
#pragma data_alignment=4096
static unsigned int verify_buffer[4096/4];		// 64KB�ļ��Ƚ�

// H264�ļ�Cacheд������
static OS_TASK TCB_H264FileTask;
__no_init static int StackH264FileTask[XMSYS_H264FILE_TASK_STACK_SIZE/4];          /* Task stacks */
static unsigned long	next_ticket_to_verify_file_system;		// ��һ�ν����ļ���дУ���ʱ��


#define	HEADER_USAGE


#if _XM_PROJ_ == _XM_PROJ_2_SENSOR_1080P_CVBS
#define	CACHE_BLOCK_FOR_FILE_HEADER	4			// ����4��

#elif _XM_PROJ_ == _XM_PROJ_DVR_4_SENSOR_D1_

#define	CACHE_BLOCK_FOR_FILE_HEADER	12			// ����8��

#else
#define	CACHE_BLOCK_FOR_FILE_HEADER	3			// ����3��
#endif


#pragma data_alignment=1024
__no_init static H264_CACHE_BLOCK h264CacheBlock[H264_CACHE_BLOCK_COUNT];
static volatile queue_s		cache_block_free;		// ���е�Cache������
static XM_RSEMA	cache_block_access_sema;		// ����cache�黥����ʵ��ź���

static OS_EVENT	cache_file_system_not_busy_event;	// ���Cache�ļ�ϵͳ�Ƿ��д�����������ݵȴ�д�룬��cacheϵͳ�Ƿ�æ
static int			cache_file_system_busy_block_count;	// Cache�ļ�ϵͳ�ȴ�д��Ŀ�
static unsigned int cache_file_system_dirty_block_statistics[H264_CACHE_BLOCK_COUNT+1];

// Cache�ļ��������
static H264_CACHE_FILE h264CacheFileHandle[H264_CACHE_FILE_COUNT];

static XM_RSEMA			h264CacheFileSema;			// ��������ź���, ����Cacheϵͳ�ڲ�����


// ��¼�����п����ȴ�ʱ��, �����㷨ͳ�Ʒ���
static int maximum_block_alloc_delay_time = 0;

// ��¼���д��ʱ��
static unsigned int max_write_time = 0;


static XM_RSEMA		block_write_access_sema;	// �ļ�ϵͳ��д�뻥������ź���
												// �����ļ���д�밴CACHE����䡢д��
												// �����ܱ����ļ���Ƭ�Ĳ���


static H264_CACHE_BLOCK *SearchReplacedCacheBlockFromCacheFile (H264_CACHE_FILE *cache_file);

#define	CACHE_DEBUG	
#ifdef CACHE_DEBUG
static void cache_debug_assert (void)
{
	assert (0);
}
#endif




// Cache���ʼ��
static void CacheBlockInit (void)
{
	int i;
	queue_initialize ((queue_s *)&cache_block_free);
	for (i = 0; i < H264_CACHE_BLOCK_COUNT; i ++)
	{
		// ��Cache��ͷ��0
		memset (&h264CacheBlock[i], 0, sizeof(H264_CACHE_BLOCK) - H264_CACHE_FILE_BLOCK_SIZE);
		
		// ����Cache�鵥Ԫר����Cache�ļ�ͷ��д�룬��CACHE_BLOCK_FOR_FILE_HEADER��
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
	
	// �ļ�ϵͳ��д�뻥������ź���
	SEMA_CREATE (&block_write_access_sema);
	
	// �ļ�ϵͳ���У���æ���¼�
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

// �ļ�ϵͳ��ȫ��������
//		��XMSYS_file_system_block_write_access_unlock���ʹ��
// ����ֵ
// -1 �ļ�ϵͳ�ѽ��mount, �ļ�ϵͳ���ʲ���ȫ
//  0 �ļ�ϵͳ������, �ļ�ϵͳ���ʰ�ȫ
int XMSYS_file_system_block_write_access_lock (void)
{
	SEMA_LOCK (&block_write_access_sema);
	if(XM_VideoItemIsBasicServiceOpened())
		return 0;
	return -1;
}

// �ļ�ϵͳ��ȫ��������(��ʱ����)
//		��XMSYS_file_system_block_write_access_unlock���ʹ��
// ����ֵ
// -2 ��ʱ, ����ʧ��
// -1 �ļ�ϵͳ�ѽ��mount, �ļ�ϵͳ���ʲ���ȫ
//  0 �ļ�ϵͳ������, �ļ�ϵͳ���ʰ�ȫ
int XMSYS_file_system_block_write_access_lock_timeout (unsigned int timeout)
{
	if(OS_Use_timeout	(&block_write_access_sema, timeout) == 0)
		return -2;
	if(XM_VideoItemIsBasicServiceOpened())
		return 0;
	return -1;	
}

// ����ļ�ϵͳ��������
void XMSYS_file_system_block_write_access_unlock (void)
{
	SEMA_UNLOCK (&block_write_access_sema);
}

// ��ȡCacheϵͳæ�ȴ��ļ���
// 0 ~ 1 ��ʾϵͳCache��������, ��������
// 2 ~ 8 ����Խ��, ��ʾʹ�õ�Cache����Խ��
// 9     ��ʾ�ڲ�Cache����, ��ʱ�޷�д��
unsigned int XMSYS_GetCacheSystemBusyLevel (void)
{
	unsigned int count = 0;
	unsigned int level;
	int i;
	OS_EnterRegion ();	// ��ֹ�����л�
	for (i = 0; i < H264_CACHE_BLOCK_COUNT; i ++)
	{
		if(h264CacheBlock[i].dirty_flag)
			count ++;
	}
	OS_LeaveRegion ();
	level = count * 10 / H264_CACHE_BLOCK_COUNT;
	if(level > 9)
		level = 9;
	
	// 20180825 ���Ǵ���ʣ��ռ䲻��̶ȵ�Ӱ��
	unsigned int disk_busy_level = XM_VideoItemCheckDiskFreeSpaceBusy();
	level += disk_busy_level;
	if(level > 9)
		level = 9;
	
	return level;
}

// �ȴ�Cache�ļ�ϵͳ������ȫ��ˢ�µ�����洢����
// timeout_ms	��ʱ�ȴ���ʱ��(����)
// 0	�ɹ�
// -1	��ʱ
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
		OS_EnterRegion ();	// ��ֹ�����л�
		// �ȴ�����cache�ļ��ر�
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
		// ��ʾ����ͳ����Ϣ
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

// �ȴ�Cache�ļ�ϵͳ��æ�����У�״̬
// timeout_ms ��ʱ�ȴ���ʱ�䣬���뵥λ
// 	0  ��ʾ���޵ȴ�
// 	���ʱ�ȴ�ʱ��0x7FFFFFFF
// ����ֵ
// 	0  ��ʾ�ɹ�
//	1~100 ��ʾ��ʱ��ϵͳһֱæ. ��ʾϵͳæ�İٷֱȡ�100%��ʾ����æ��
//		
// ���ݻ������ݵ�ʱ�䳤�ȣ������ļ�ɾ���ȴ��Ĳ��ԡ�
// ����ʱ��Խ�̣�ɾ������Խ��
// ����ʱ��Խ����ɾ������Խ��
// 0.125��	����ִ������ɾ��
// 0.25�룬 ִ������ɾ����50%����������
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

// ����ʣ��δʹ�õ�cache���ֽڴ�С
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

// �����е�H264��д��cache�����Ƿ������С����ռ�
int xm_h264_check_cache_exhausted (void)
{
	// �������δʹ�õ�cache���ֽڴ�С
	unsigned int free_block_sizes = get_free_cache_block_size ();
	if(free_block_sizes >= 0x180000)	// ����1.5MB
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
	
	// ��������£�ÿ·ÿ����Ƶ�ļ���CACHE�б�����3��Cache�飬����block 0���ļ�ͷ��һ���ǵ�ǰ����д���������cache�飬һ��������д�뵽SD���е�cache��
	// ǰ��˫·¼��ʱһ�㱣��2 * 3��cache��
	if(cache_block_count > (3 * video_channel))		// ��ʾæ
	{
		//XM_printf ("cache busy to write, count=%d\n",  cache_block_count);
		return 1;
	}
	
	return 0;
}

// ���H264�������Ƿ�æ(ϵͳ�ȴ�д��)
XMBOOL XMSYS_H264CodecIsBusy (void)
{
	return 0;
}



static void adjust_codec_frame_rate (void)
{
	// �������δʹ�õ�cache�����ֽڴ�С
	unsigned int free_block_sizes = get_free_cache_block_size ();
	
	// ����Cacheʣ��ռ��С����H264 Codec����ʱ�Ƿ�֡����֡������
	// 1) ����ISP��֡�ʿ���vifrasel0��vifrasel1
	// 2) ����H264������������
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
	block->lock_flag = 0;		// Cache��շ���ʱ�Զ���������ֹ���滻
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
		
	// ����ʣ�����֡��������codec����֡��
	adjust_codec_frame_rate ();
}

// ��Cache����г��з���һ��Cache��
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
		// �ڿ��п��в���
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
			// �Ѵ������жϿ�
			// ���ʣ���ĸ���
#ifdef CACHE_DEBUG
			if(block->id != H264_CACHE_BLCK_ID)
			{
				XM_printf ("cache block 0x%08x break\n", block);
				cache_debug_assert ();
			}
#endif
				
			// ����ʣ�����֡��������codec����֡��
			adjust_codec_frame_rate ();
		}
		
		
		if(block)
		{
			// �ҵ����п�
			break;
		}
		else if(h264_offset == 0)
		{
			// ����Ƿ��ǵ�һ��Cache��(�����ļ�ͷ��Ϣ)
			//H264_FATAL ("Please check CACHE_BLOCK_FOR_FILE_HEADER(%d) is small\n", CACHE_BLOCK_FOR_FILE_HEADER);
			//assert (0);
		}
		else
		{
			// �Ӹ�Cache�ļ��Լ���Cache���������ҵ����滻��Cache��
			// ������Cache�ļ���Cache���������ҿ��滻��Cache��
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
		
		// �ȴ�Cache�ļ������������д�뵽�ļ�ϵͳ
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
		// Ͷ����Ϣ����Ƶ��д���ٶȵͣ���ʾ�û�����
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
	block->lock_flag = 1;		// Cache��շ���ʱ�Զ���������ֹ���滻
	block->cache_file = cache_file;
	block->file_offset = h264_offset;

	block->dirty_start_addr = h264_offset;
	block->dirty_end_addr = h264_offset;

	block->reload_flag = 0;
	
	return block;
}

// ��Cache�ļ��в���һ�����õĿ飬�����ÿ��Cache�ļ���Cache�������жϿ�
static H264_CACHE_BLOCK *SearchReplacedCacheBlockFromCacheFile (H264_CACHE_FILE *cache_file)
{
	H264_CACHE_BLOCK *block = NULL;
	if(cache_file == NULL)
		return NULL;
	
	// ��һ��ɨ�裬����Ƿ��п��滻�Ŀ��п�(���ࡢ��������
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
		// ���ҵ���Cache���Cache�ļ���Cache�������жϿ�
		queue_delete ((queue_s *)block);
	}
	else
	{
		block = NULL;
	}
		
	SEMA_UNLOCK (&cache_file->sema);
	return block;
}

// ����ָ��ƫ�Ƶ�Cache���Ƿ���ڡ�
// ��Cache����Cache�ļ���Cache�������д��ڣ�
//		1) ���ÿ�������
//		2)	�Ƶ�����β��
//		3)	����
// ��������
//		1) ����Cache�ļ���������ź���
//		2)	����
//static H264_CACHE_BLOCK *LocateCacheBlockFromFileOffset (H264_CACHE_FILE *cache_file, size_t h264_offset, int read_data_from_file)
//{
//
//}

// ��Cache�ļ���Cache�������в���ָ����λ�õ�Cache���Ƿ����
// ���ҽ���һ��H264����������ã�
static H264_CACHE_BLOCK *LocateCacheBlockByFilePosition (H264_CACHE_FILE *cache_file, size_t h264_offset)
{
	H264_CACHE_BLOCK *block = NULL;
	H264_CACHE_BLOCK *loop;
	SEMA_LOCK (&cache_file->sema);		// �����ļ��Ŀ�����
	do
	{
		// �����λ���Ƿ�λ�ڵ�ǰCache����
		/*
		if(cache_file->current_block)
		{
			if(	h264_offset >= cache_file->current_block->file_offset
				&& h264_offset < (cache_file->current_block->file_offset + H264_CACHE_FILE_BLOCK_SIZE))
			{
				// ��λ�ڵ�ǰ����
				block = cache_file->current_block;
				break;
			}
		}
		*/

		// ����Cache�ļ���Cache�飬�����λ�ö�Ӧ��Cache���Ƿ����
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


// ����Cache�ļ��ĵ�ǰ��ԾCache��
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

	// ���Cache�ļ��ĵ�ǰCache���Ƿ�������
	if(cache_file->current_block == NULL)
	{
		cache_file->current_block = block;
		SEMA_UNLOCK (&cache_file->sema);
		return;
	}

	// ��ǰ��������
	current_block = cache_file->current_block;
	cache_file->current_block = block;
	SEMA_UNLOCK (&cache_file->sema);

	// ��鵱ǰ���Ƿ����
	SEMA_LOCK (&current_block->usage_sema);
	if(!current_block->dirty_flag)
	{
		// �����
		SEMA_UNLOCK (&current_block->usage_sema);
		return;
	}
	// �����Ƿ���д��
	if(!current_block->full_flag)
	{
		// �����飬��ʱ��ִ��д�����
		SEMA_UNLOCK (&current_block->usage_sema);
		return;
	}
	
	SEMA_UNLOCK (&current_block->usage_sema);
	
	// ���ϵĻ�ԾCache���ύ��Cache�ļ�����ִ���첽�ļ�д�����
	xm_h264_cache_command_queue_insert_command (cache_file->index, H264_CACHE_FILE_CMD_WRITE, current_block);
}

// ��Cache����뵽Cache������β��
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

// ��Cache���е�������ˢ�µ��ļ�ϵͳ��ȥ(��Cache�ļ��������)
// update_fat_cache	�Ƿ�ˢ��FAT cache���ļ�ϵͳ
static int CacheBlockFlushDirtyContextToFileSystem (H264_CACHE_BLOCK *block, int update_fat_manage_cache)
{
	int ret = 0;
	size_t begin, end;
	H264_CACHE_FILE *cache_file;
	if(block == NULL)
		return 0;
	
	// Cache���ļ�д��ʱ��2���ź�������������������д��ͬ�������������˳�򣬷�ֹ��������
	// ��������ֹ�ⲿCacheд��
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
		// ����ACC�µ�ʱ������������д��
#if 0
		if(xm_power_check_acc_safe_or_not() == 0)
		{
			XM_printf ("CacheFlush failed, bad acc\n"); 
			ret = -1;
			break;
		}
#endif
		
		// ��������
		// �������������������ַ
		// �����Ƿ���д������δд������ÿ��ˢ�������ļ��رյ��¡�
		// Ϊ�����ļ�ϵͳ��Ƭ�Ĳ���, �ļ�ϵͳ��д����CACHE��Ϊ��λ���䡢д�롢ɾ��
		// ����轫δд����β�鲹0д��
		// �����ԣ�������Ч�����������£���ͬ�̵߳Ĳ���д��ͬ��������Ƭ�Ĳ�����
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

		// д�ļ�
		assert (end > begin);
		// ��ȡCache���Cache�ļ����
		cache_file = (H264_CACHE_FILE *)block->cache_file;
		assert (cache_file);
		if (cache_file->fd == NULL)	// �ļ�������
		{
			ret = -1;
			break;
		}
		
		// ��ʱ���������������write_sema������H264 Codec���ļ�д����ͬ���޸������������޸���������
		// ����������write_sema������H264 Codec�����������		
		if(cache_file->error == 0)
		{
			int to_write;
			unsigned int ticket, ticket_seek;
			static unsigned int max_block_write_ticket = 0;
			static unsigned int max_block_seek_ticket = 0;
			// �����ļ����ļ�IO��������ź���������д��/�����������ļ�ָ���޸�
			SEMA_LOCK (&cache_file->sema_file_io);

			// ��λ��д���ļ�ϵͳ
			ticket = XM_GetTickCount ();
			// �ļ�ϵͳ��ȫ��������
			if(XMSYS_file_system_block_write_access_lock () == 0)
			{
				// ��ȫ����
				if(XM_fseek ( (void *)cache_file->fd, begin, XM_SEEK_SET) < 0)
				{
					cache_file->error = -1;	// ��Ǵ���
					XMSYS_file_system_block_write_access_unlock ();
					// �ļ�IO������ʽ���
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
				// �ǰ�ȫ����
				cache_file->error = -1;	// ��Ǵ���
				XMSYS_file_system_block_write_access_unlock ();
				// �ļ�IO������ʽ���
				SEMA_UNLOCK (&cache_file->sema_file_io);
				XM_printf ("CacheBlockFlushDirtyContextToFileSystem failed, un-safe io\n");
				ret = -1;
				break;
			}
			// �ļ�ϵͳ��ȫ���ʽ���
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
					// �����Ƿ����
					if(FS_IsVolumeMounted("mmc:0:"))
					{
						// ����д��
						XM_printf ("Error, FS diskfull\n");
						XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_DISK_FULL);
						ret = -2;
						cache_file->error = -2;
					}
					else
					{
						// �ٱ���
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
			// ��鿨У��ʱ���Ƿ��ѹ���
			else if(cache_file->error == 0 && next_ticket_to_verify_file_system < XM_GetTickCount ())
			{
				//XM_printf ("verify fseek\n");
				// ���cache�Ƿ�æ(��������ݵȴ�д��), ����, ������ʱ����
				if(cache_file_system_busy_block_count <= 2)
				{
					// �ļ�ϵͳ��ȫ��������
					if(XMSYS_file_system_block_write_access_lock () == 0)
					{
						if(XM_fseek ( (void *)cache_file->fd, begin, XM_SEEK_SET) >= 0)
						{
							// �ļ�����У��
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
								// �ļ�ϵͳ�ض�У��ʧ�ܡ�SD����Ҫ����
								//XM_printf ("FS verify NG\n");
								// Ͷ�ݿ�У��ʧ�ܴ������Ѹ���SD��
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
				
					// �����´��ļ�ϵͳУ���ʱ��
					next_ticket_to_verify_file_system = XM_GetTickCount () + FS_VERIFY_CYCLE_MS;
				}
			}
			// �ļ�IO������ʽ���
			SEMA_UNLOCK (&cache_file->sema_file_io);
		}
		else
		{
			ret = -1;
		}
	} while (0);
	
	// �ļ�ϵͳ���������������Խ��в���
	// 1) FS_CACHE_SetMode("", FS_SECTOR_TYPE_MASK_DIR|FS_SECTOR_TYPE_MASK_MAN, FS_CACHE_MODE_WT);
	//	  ticket=144645  226MB  1.56MB/s   ��ʿ��8GB TF��
	//    ticket=151163  226MB  1.50MB/s   ��ʿ��2GB SD��
	//                          1.66MB/s   Class 4 SD��
	//	2) FS_CACHE_SetMode("", FS_SECTOR_TYPE_MASK_DIR|FS_SECTOR_TYPE_MASK_MAN, FS_CACHE_MODE_WB);	
	//    ÿ��Cache��д��ʱִ��FS_CACHE_Clean
	//	  ticket=113198  226MB  2MB/s      ��ʿ��8GB TF��
	//    ticket=226280  226MB  1MB/s      ��ʿ��2GB SD��
	//                          2.3MB/s    Class 4 SD��
	// 3) FS_CACHE_SetMode("", FS_SECTOR_TYPE_MASK_DIR|FS_SECTOR_TYPE_MASK_MAN, FS_CACHE_MODE_WB);
	//	  ÿ4��Cache��д��ʱִ��FS_CACHE_Clean (1MB)
	//    ticket=103025  226MB  2.19MB/s   ��ʿ��8GB TF��
	//    ticket=99768   226MB  2.26MB/s   ��ʿ��2GB SD��
	//                          2.72MB/s   Class 4 SD��
	//
	//	1) ����1��д��ƽ���ٶ�Ϊ1.66MB/s, ����2��д��ƽ���ٶ�Ϊ2.3MB/s, ����3��д��ƽ���ٶ�Ϊ2.72MB/s,
	// 2) ����1�����2�Ķ�����������ֽ���ͬ
	// 3) ����1�����2��д������д���ֽڲ���ͬ������1�Ȳ���2����16%��д�����
	
	// ��Ƶ�ļ��ر�ʱ��ÿ��Cache���ˢ�¹��̲���ǿ��ͬ������FAT������(�ر�Ŀ¼��)��
	// �������ļ��ر�ʱ��FAT������д�룬���ٴ��̵�д������������ļ��رչ��̡�
	if(update_fat_manage_cache == 0)
		goto cache_flush_end;
	
	// �ļ�ϵͳ��ȫ��������
	if(XMSYS_file_system_block_write_access_lock () == 0)
	{
#if 1
		// �µ�FAT�����д�����
		// CacheϵͳԽæ, ����FAT��ļ��Խ��, �����ɼ���FAT�����д�����, ����Cache�����޷�ʵʱд���״��.
		// һ���������ʱ��, Cache���æ. ��ʱ�ʵ��ӳ�FAT������µ�ʱ��, ����FAT����д��Ĵ���, ���������д��������������ݵ�д��.
		unsigned int busy_level = XMSYS_GetCacheSystemBusyLevel ();
		unsigned int fat_update_size = H264_CACHE_FILE_BLOCK_SIZE;	// ȱʡΪCache���С, ��ÿ��cache��д��ʱ������FAT����
		if(busy_level <= 2)
			fat_update_size = H264_CACHE_FILE_BLOCK_SIZE;
		else 
			fat_update_size = H264_CACHE_FILE_BLOCK_SIZE * busy_level;
		//if(!(block->file_offset & (fat_update_size - 1)))
		// ����ʱ����
		if( (block->file_offset % fat_update_size) == 0 )
		{
			FS_CACHE_Clean ("");
		}
#else
		// ÿ��Cache�������FAT����д��
		//if(!(block->file_offset & (0x100000 - 1))) // 1MBˢ��
		//if(!(block->file_offset & (0x40000 - 1)))	// 256KBˢ��
		//if(!(block->file_offset & (0x20000 - 1)))	// 256KBˢ��
		{
			FS_CACHE_Clean ("");
		}
#endif
	}
	// �ļ�ϵͳ��ȫ���ʽ���
	XMSYS_file_system_block_write_access_unlock ();	

	// ���������־.
	// ע��: Cache���滻�㷨ͨ���ж�dirty_flag���ж��Ƿ��������
	//block->dirty_flag = 0;

cache_flush_end:
	// �ͷſ�����ź���
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



// ����һ��H264 Cache�ļ�����(H264 Codec�̵߳���)
static H264_CACHE_FILE *CreateCacheFile (const char *filename)
{
	int i;
	H264_CACHE_FILE *cache_file = NULL;
	
	do
	{
		SEMA_LOCK (&h264CacheFileSema); /* Make sure nobody else uses */
			
		// ����һ��Cache��Ԫ
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
	
	cache_file->fd = 0;						// ��ʱ�����ļ��������ļ�����ʱ����H264�����ӳ�
	cache_file->alloc = 1;					// ��Ƿ�����	
	cache_file->error = 0;					// ����޴���
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

	// ��ʼ����ǰʹ�ÿ�Ϊ��
	cache_file->current_block = NULL;
	queue_initialize ((queue_s *)&cache_file->cache_block);
	
	// ���������ļ��Ļ����ź���
	SEMA_CREATE (&cache_file->sema);
	SEMA_CREATE (&cache_file->sema_file_io);
	
	SEMA_UNLOCK (&h264CacheFileSema);
	//XM_printf ("CreateCacheFile %x\n", cache_file);
	return cache_file;	
}

// ɾ��һ��H264 Cache�ļ�����(��Cache�ļ��������)
static int DeleteCacheFile (H264_CACHE_FILE *cacheFile)
{
	if(cacheFile == NULL)
		return 0;

	SEMA_LOCK (&h264CacheFileSema);
	// ���Cache�ļ�����Ƿ�Ϸ�
	if(cacheFile->alloc == 0)
	{
		H264_PRINT ("illegal Cache File(0x%08x), alloc=%d\n", cacheFile, cacheFile->alloc);
		SEMA_UNLOCK (&h264CacheFileSema);
		return 0;
	}
		
	// Cache�ļ��ѷ���
	
	// ɾ�������ļ��Ļ����ź���
	SEMA_DELETE (&cacheFile->sema);
	SEMA_DELETE (&cacheFile->sema_file_io);
	
	// �ͷ�����Cache��
	cacheFile->alloc = 0;
	cacheFile->error = 0;
	cacheFile->closed = 0;
	SEMA_UNLOCK (&h264CacheFileSema);
	return 1;
}

// H264 Cache�ļ��򿪣���H264 Codec�̵߳���
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
	// ֪ͨH264��������ļ��¼�
	if(xm_h264_cache_command_queue_insert_command (fp->index, H264_CACHE_FILE_CMD_OPEN, NULL) == 0)
	{
		DeleteCacheFile (fp);
		H264_PRINT ("H264CacheFileOpen (%s) failed, Can't insert cache open command\n", filename);
		return NULL;
	}
	H264_PRINT ("Open %d\n", XM_GetTickCount());
	return fp;
}

// H264 Cache�ļ��رգ���H264 Codec�̵߳���
int H264CacheFileClose (H264_CACHE_FILE* cache_file)
{
	H264_CACHE_BLOCK *block;
	if(cache_file == NULL)
	{
		H264_PRINT ("H264CacheFileClose failed, cache_file == NULL\n");
		return -3;
	}
	SEMA_LOCK(&h264CacheFileSema); /* Make sure nobody else uses */
	// ���H264 Cache�ļ�����Ƿ�Ϸ�
	if(cache_file->alloc == 0)
	{
		H264_PRINT ("H264CacheFileClose NG, illegal Cache File(0x%08x), alloc=%d\n", 
			cache_file, cache_file->alloc);
		SEMA_UNLOCK (&h264CacheFileSema);
		return -3;
	}
	SEMA_LOCK (&cache_file->sema);	// ����Cache�ļ�
	cache_file->closed = 1;		// ����ļ��ر���
	H264_PRINT ("H264CacheFileClose (%s) \n", cache_file->filename);

	
	SEMA_UNLOCK (&h264CacheFileSema);
	
#if 1
	// ɨ�赱ǰ�ļ�ӵ�е����з���Cache�鲢�����ͷţ��������ļ�����Cache��ʱ���ӳ�
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
			
			if(block->dirty_flag || block->lock_flag)		// ����������
			{
				SEMA_UNLOCK (&block->usage_sema);
				block = (H264_CACHE_BLOCK *)queue_next ((queue_s *)block);
			}
			else
			{
				// ����飬����
				SEMA_UNLOCK (&block->usage_sema);
				H264_CACHE_BLOCK *next = (H264_CACHE_BLOCK *)queue_next ((queue_s *)block);
				queue_delete ((queue_s *)block);	// ����
				CacheBlockFree (block);
				block = next;
			}
		}
	}
#endif
	
	SEMA_UNLOCK (&cache_file->sema);

	// ��Cache�ļ��ر�����Ͷ�ݵ��������
	if(xm_h264_cache_command_queue_insert_command (cache_file->index, H264_CACHE_FILE_CMD_CLOSE, NULL) == 0)
	{
		H264_PRINT ("H264CacheFileClose (%s) failed, Can't insert cache close command\n", cache_file->filename);
		return -3;
	}
	return 0;
}

// H264 Cache�ļ���λ������H264 Codec�̵߳��ã��ҽ���һ��H264��������ͬʱ����
// �˴����Խ�������ı������ƽ�ֹ
XMINT64 H264CacheFileSeek (H264_CACHE_FILE* cache_file, XMINT64 off_64, int whence)
{
	size_t h264_offset;

	// �ȴ���Cache�ļ�֮ǰ������ȫ��ִ�����
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
	// ���㵱ǰ������λλ��
	if(whence == XM_SEEK_CUR)
	{
		// �������ƫ�Ƽ���
		h264_offset = cache_file->h264_offset + (int)off_64;
	}
	else if(whence == XM_SEEK_SET)
	{
		// ��λ����ָ��λ��
		h264_offset = (int)off_64;
	}
	else if(whence == XM_SEEK_END)
	{
		// ��λ����β��
		h264_offset = cache_file->h264_length + (int)off_64;
	}
	else
	{
		H264_FATAL ("Cache File Seek Failed, illegal origin(%d)\n", whence);
		// SEMA_UNLOCK (&cache_file->sema);
		return -3;
	}

	// ���IO�Ƿ��쳣
	if(cache_file->error)
	{
		// 20140805 ZhuoYongHong
		// �ļ�IO�쳣ʱ���ش���
		return -3;
		
		// �ļ�IO�쳣ʱ�����ش���
		// SEMA_UNLOCK (&cache_file->sema);
		// return h264_offset;
	}

	// �����µ���λ���Ƿ񳬳�����ǰ���ѱ����ֽڳ��ȣ�����汾�в�����
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

// H264 Cache�ļ�������H264 Codec�̵߳���
// ��ǰ�汾��֧��
int H264CacheFileRead (unsigned char *buf, size_t size, size_t nelem, H264_CACHE_FILE *fd)
{
	// �ȴ���Cache�ļ�֮ǰ������ȫ��ִ�����
	H264_FATAL ("Current Version Don't support H264 read operation\n");
	return -3;
}

// H264 Cache�ļ�д������һ��H264 Codec�̵߳���
int H264CacheFileWrite (unsigned char *buf, size_t size, size_t nelem, H264_CACHE_FILE *cache_file)
{
	size_t count;
	size_t to_copy;
	unsigned int offset;
	size_t h264_offset;
	size_t block_file_offset;	// ����ļ�ƫ�ƻ�ַ
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
		// �޸�Ϊ�����쳣��ʹ�ü�¼���������˳������л����޿���¼ģʽ
		// IO�쳣ʱCacheд��������سɹ�
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
		// ����Cache���ڵ�ƫ��
		block_file_offset = h264_offset & ~(H264_CACHE_FILE_BLOCK_SIZE - 1);
		// ���㵱ǰ��д����ֽ��� to_copy
		to_copy = H264_CACHE_FILE_BLOCK_SIZE - (h264_offset - block_file_offset);
		assert (to_copy > 0);
		assert (to_copy <= H264_CACHE_FILE_BLOCK_SIZE);
		// ����д���ƫ��λ��offset��д���ֽڴ�Сto_copy
		offset = h264_offset - block_file_offset;
		assert (offset < H264_CACHE_FILE_BLOCK_SIZE);
		if(to_copy > count)
			to_copy = count;

		// ����ƫ��ֵ��λCache�鵥Ԫ
		SEMA_LOCK (&cache_file->sema);
		block = LocateCacheBlockByFilePosition (cache_file, h264_offset);
		if(block)
		{
			// ��Cache�������жϿ�
#ifdef CACHE_DEBUG
			if(block->id != H264_CACHE_BLCK_ID)
			{
				XM_printf ("cache block 0x%08x break\n", block);
				cache_debug_assert ();
			}
#endif
			queue_delete ((queue_s *)block);
			// ��Cache����뵽Cache�ļ���Cache�������β��
			InsertCacheBlockToTail (block, cache_file);
			SEMA_UNLOCK (&cache_file->sema);
		}
		else
		{
			size_t file_offset;
			// �鲻���ڡ������µ�Cache��
			SEMA_UNLOCK (&cache_file->sema);
			block = CacheBlockAlloc (cache_file, h264_offset);
			// assert (block);
			if(block == NULL)
			{
				cache_file->error = -1;
				H264_PRINT ("H264CacheFileWrite failed due to CacheBlockAlloc failed\n");
				return -3;
			}
			
			// ���Cache����ļ�ƫ�Ƶ�ַ�Ƿ�С�ڵ�ǰ��������ֽڴ�С��
			// ��С�ڣ�������ļ�ϵͳ��ȡ�ѱ�д���������
			file_offset = block->file_offset;
			if( file_offset < cache_file->h264_length)
			{
				// ��Cache���ѱ�д�뵽�����ļ�, �ұ��滻��ȥ
				// ���ļ�ϵͳ�����¶�ȡ��
				unsigned int s_ticket, e_ticket;
				// ע�⣺�˴��ᵼ��H264�����߳���������ͳ������Ϣ��������
				size_t length = cache_file->h264_length - file_offset;
				int ret;
				assert (length >= H264_CACHE_FILE_BLOCK_SIZE);
				if(length > H264_CACHE_FILE_BLOCK_SIZE)
				{
					length = H264_CACHE_FILE_BLOCK_SIZE;
				}
				
				s_ticket = XM_GetTickCount();
				
				// �����ļ�IO�ź���
				SEMA_LOCK (&cache_file->sema_file_io);
				
				unsigned int read_bytes;
				// �ļ�ϵͳ��ȫ��������
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
				// �ļ�ϵͳ��ȫ�����������
				XMSYS_file_system_block_write_access_unlock();
				if(read_bytes != length)
				{
					cache_file->error = -1;
					SEMA_UNLOCK (&cache_file->sema_file_io);
					CacheBlockFree (block);
					XM_printf ("H264CacheFileWrite failed due to XM_fread error, length=%d, read_bytes=%d \n", length, read_bytes);
					return -3;
				}
				//�ͷ��ļ�IO�ź���
				SEMA_UNLOCK (&cache_file->sema_file_io);

				// ��ǿ���д��
				block->full_flag = 1;
				
				e_ticket = XM_GetTickCount();
				// ����CacheЧ�ʷ�������
				cache_file->write_read_times ++;
				cache_file->write_read_bytes += H264_CACHE_FILE_BLOCK_SIZE;
				if(cache_file->maximum_write_read_delay < (e_ticket - s_ticket))
				{
					cache_file->maximum_write_read_delay = e_ticket - s_ticket;
				}
				// �ۼ�ÿ�λض����������ڵ�ʱ�䣬��ƽ��ֵ��ӦCache���滻ʱƽ��������ʱ��
				cache_file->average_write_read_delay += (e_ticket - s_ticket);
			}
			
			// ��Cache����뵽Cache�ļ���Cache�������β��
			InsertCacheBlockToTail (block, cache_file);
		}

		// assert (block_file_offset == block->file_offset);
		if(block_file_offset != block->file_offset)
		{
			assert (0);
		}
		
		// ���õ�ǰCache��
		//CacheFileSetCurrentBlock (block, cache_file);

		submit_to_write = 0;
        
		// Cache�д��ڿ��пռ�
		// ��������д�뵽Cache���е���������
		// ������д��ʱ��2���ź�������������Cache���ļ�д��ͬ�������������˳�򣬷�ֹ��������
		// "Cache�黺������д��"��"Cache���ļ�д��"���Ᵽ��
		SEMA_LOCK (&block->usage_sema);
		
		// �����ļ�IO�ź���
		//SEMA_LOCK (&cache_file->sema_file_io);
		
		//if(offset & 3 || (((unsigned int)buf) & 3) || (to_copy & 3))
		{
			//XM_printf ("offset=0x%08x, buf=0x%08x\n, copy=0x%08x\n", offset, buf, to_copy);
		}
		extern void dma_memcpy (unsigned char *dst_buf, unsigned char *src_buf, unsigned int len);

		// 20170314 ʹ��dma_memcpy���ԽϺø���ϵͳЧ��, ��UVC֡�����Ը���.
		// dma_memcpyʹ��ʱ������Ϊlock��־, �ᵼ��ISP bus bandwidth abnormal
#ifdef UVC_PIO		
		memcpy (block->buffer + offset, (const void *)buf, to_copy);
#else
		dma_memcpy ((unsigned char *)block->buffer + offset, (unsigned char *)buf, to_copy);
#endif
		
		// ����������
		if(block->dirty_flag == 0)
		{
			// �����򲻴���
			block->dirty_start_addr = h264_offset;
			block->dirty_end_addr = h264_offset + to_copy;
			block->dirty_flag = 1;
			
			get_dirty_cache_block_count ();
		}
		else
		{
			// �������Ѵ���
			if(block->dirty_start_addr > h264_offset)
				block->dirty_start_addr = h264_offset;
			if(block->dirty_end_addr < (h264_offset + to_copy))
				block->dirty_end_addr = (h264_offset + to_copy);
		}
		// ����Ƿ�����
		if(!block->full_flag)
		{
			size_t dirty_size = block->dirty_end_addr;
			dirty_size = dirty_size - block->file_offset;
			if(dirty_size == H264_CACHE_FILE_BLOCK_SIZE)
			{
				// ����д������
				// Cache���״�д��ʱ�������ύ��Cache�ļ�ϵͳ�����ļ�д�����
				block->full_flag = 1;
				// �����Ҫ�ύ��Cache�ļ���������ļ�д��
				submit_to_write = 1;
				// ���һ�η���ʱ�Զ�������ֱ����д�������������
				// ��1����(�ļ�ƫ��0)ǿ����������ΪH264�ļ��ر�ʱ���޸��ļ�ͷ
				if(block->file_offset)
				{
					// ���ǵ�һ���飬��д��ʱ�Զ�����
					block->lock_flag = 0;	
				}
			}
		}		
		
		count -= to_copy;
		buf += to_copy;
		h264_offset += to_copy;
		// ����Cache�ļ���ָ��
		cache_file->h264_offset = h264_offset;
		// ������д�����ֽڳ���
		if(h264_offset > cache_file->h264_length)
			cache_file->h264_length = h264_offset;
		
		// �����ļ�IO�ź���
		//SEMA_UNLOCK (&cache_file->sema_file_io);
		SEMA_UNLOCK (&block->usage_sema);
		
		// ����Ƿ���Ҫ��Cache���Ŷ�ִ���첽д�����
		if(submit_to_write)
		{
			// ����Cache���ύ��Cache�ļ�����ִ���첽�ļ�д�����
			// xm_h264_cache_command_queue_insert_command (cache_file->index, H264_CACHE_FILE_CMD_WRITE, block);	
		}

		// ���õ�ǰCache��
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

// Cache�������CacheFileFlush ˢ��������鲢�ͷ�Cache��
// ��Cache�ļ�������Cache�����ݸ��µ��ļ�ϵͳ���ͷ�Cache��
static void CacheFileFlush (H264_CACHE_FILE *cache_file)
{
	H264_CACHE_BLOCK *block;
	int ret = 0;
	int is_diskfull = 0;

	// cache�ļ�����Ϸ��Լ��
	if(cache_file == NULL)
		return;
	if(cache_file->alloc == 0)
	{
		XM_printf ("cache_file %x alloc==0\n", cache_file);
		return;
	}

	assert (cache_file->closed == 1);

	// ���Cache������
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
			// ��Ƶ�ļ��ر�ʱ��ÿ��Cache���ˢ�¹��̲���ǿ��ͬ������FAT������(�ر�Ŀ¼��)�������ļ��رչ���
			ret = CacheBlockFlushDirtyContextToFileSystem (block, 0);
			if(ret == (-2))
				is_diskfull = 1;			
		}
		else
		{
			block->dirty_flag = 0;
		}
		// ��Cache���ͷŵ�����Cache���
		CacheBlockFree (block);
	}
	SEMA_UNLOCK (&cache_file->sema);
	
	if(is_diskfull)
	{
		//OS_Delay (1000);
		XM_printf ("Cache File Closed, disk full\n"); 
	}
}

// ���л������ᵼ����Դ�ĳ�ʱ��������
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
		// ��ȡһ��Cache����
		int ret;
		void *fd;
		ret = xm_h264_cache_command_queue_remove_command (&file_index, &cache_cmd, &cmd_param);
		if(ret == 1)
		{
			// ��ȡһ��Cache�ļ�����
			//XM_printf ("get command file=%d, cmd=%d\n", cache_file, cache_cmd);
#ifdef DEBUG
			assert ( file_index < H264_CACHE_FILE_COUNT );
			assert ( cache_cmd >= H264_CACHE_FILE_CMD_OPEN && cache_cmd <= H264_CACHE_FILE_CMD_CLOSE );
#endif
			SEMA_LOCK (&h264CacheFileSema); /* Make sure nobody else uses */
			fp = &h264CacheFileHandle[file_index];
			// ������Ƿ��ѷ���
			if( fp->alloc == 0 )
			{
				SEMA_UNLOCK (&h264CacheFileSema);
				H264_FATAL ("H264 File Internal Error, fp=%x(%d), fp->alloc == 0\n", fp, file_index);
				continue;
			}
			// �������CACHE�ļ��ı���
			SEMA_UNLOCK (&h264CacheFileSema);
			
			switch (cache_cmd)
			{
				case H264_CACHE_FILE_CMD_OPEN:
					H264_PRINT ("H264_CACHE_FILE_CMD_OPEN\n");
					SEMA_LOCK (&fp->sema);
					// �ļ��򿪲���
					if(fp->fd != NULL)
					{
						SEMA_UNLOCK (&fp->sema);
						H264_FATAL ("H264 File Internal Error, fd != NULL\n");
						continue;
					}
					// ����ļ�ϵͳ�Ƿ����
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
					// �ļ��رղ���
					H264_PRINT ("H264_CACHE_FILE_CMD_CLOSE %x\n", fp);
					SEMA_LOCK (&fp->sema);
					XM_printf ("close cache file (%s), fd=0x%08x, error=%d\n", fp->filename, fp->fd, fp->error);
					
					error = fp->error;
					
					// ����Ƿ������δд��������ݣ�ǿ��ˢ�²��ͷ�Cache��
					CacheFileFlush (fp);
					{
					int dirty_count = get_dirty_cache_block_count ();
					//XM_printf ("dirty_blk=%d\n", dirty_count);
					}
					if(fp->fd)
					{
						// �ļ��ѳɹ���
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
						
						// �ļ�ϵͳ��ȫ��������
						if(XMSYS_file_system_block_write_access_lock() == 0)
						{
							XM_fclose ((void *)fp->fd);
							FS_CACHE_Clean("");
						}
						fp->fd = NULL;
						XMSYS_file_system_block_write_access_unlock();
						
	#ifdef H264_CACHE_FILE_TEST
	#else
						// ������Ƶ�����ݿ�
						//H264_PRINT ("VideoItemUpdate %s\n", fp->filename);
						if(XM_VideoItemIsTempVideoItemFile(fp->filename))
							XM_VideoItemUpdateVideoItemFromTempName (fp->filename, fp->h264_length, &fp->create_time);
						else
							XM_VideoItemUpdateVideoItemFromFileName (fp->filename, fp->h264_length, &fp->create_time);
	#endif
					}
					SEMA_UNLOCK (&fp->sema);
					// ɾ��CacheFile����
					DeleteCacheFile (fp);
					//H264_PRINT ("H264_CACHE_FILE_CMD_CLOSE end\n");
					if(error != 0)
					{
						// �ļ����ڴ���д�����
						// �����̿ռ��Ƿ�����쳣
						XM_printf ("File Error, forced RecycleGarbage\n");
						XM_VideoItemMonitorAndRecycleGarbage (0);
					}
					break;
				
				case H264_CACHE_FILE_CMD_WRITE:
					// �ļ�д��
					//H264_PRINT ("H264_CACHE_FILE_CMD_WRITE\n");
					block = (H264_CACHE_BLOCK *)cmd_param;		
					cache_file = NULL;
					// ��ֹ���滻
					//SEMA_LOCK (&fp->sema);	// ����Cache�ļ�,
					SEMA_LOCK (&block->usage_sema);
					lock_flag = block->lock_flag;		// ����
					block->lock_flag = 1;
					SEMA_UNLOCK (&block->usage_sema);
					
					// cache�鵥��д��ʱͬ������FAT������
					ret = CacheBlockFlushDirtyContextToFileSystem (block, 1);
					/*
					if(ret == (-2))
						is_diskfull = 1;
					else
						is_diskfull = 0;
					*/
					
					// Blockˢ�º���ܻ����±���. ����ͷ�ʱ��Ҫ���¼��
					
					SEMA_LOCK (&fp->sema);	// ����Cache�ļ�,
					SEMA_LOCK (&block->usage_sema);
					block->lock_flag = lock_flag;	// �ָ�
					// ���Cache���Ӧ��Cache�ļ��Ƿ���Ͷ���ļ��ر��������Cache�鲻���ļ��ĵ�һ��Cache��(header_flag)
					// ���ǣ�����Cache�鶪��
					
					// ��Ҫ����ȷ�ϸÿ�Ϊ����鼰������
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

						// ��Cache������Ͽ�
						queue_delete ((queue_s *)block);

						// �ͷ�Cache����Դ
						CacheBlockFree (block);
					}
					// �ͷſ�����ź���
					SEMA_UNLOCK (&block->usage_sema);
					SEMA_UNLOCK (&fp->sema);
						
					// �ļ�ϵͳ��ȫ��������
					if(XMSYS_file_system_block_write_access_lock () == 0)
					{	
						// ���԰�ȫ����
						// ����Ƿ�����쳣
						if(ret == 0 && cache_file)
						{
							int dirty_count;
							// ���ļ�ϵͳ�쳣
							// �����������. �����������С, ���Ŵؼ�ɨ�������
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
					// �ļ�ϵͳ��ȫ���ʽ���
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
	
	// ������һ��д��У���ʱ��
	next_ticket_to_verify_file_system = XM_GetTickCount () + FS_VERIFY_CYCLE_MS;

	SEMA_CREATE (&h264CacheFileSema); /* Creates resource semaphore */
	xm_h264_file_init ();

	// ��ʼ���������
	xm_h264_cache_command_queue_init ();
	
	CacheBlockInit ();
		
	// ����Cache�ļ���������
	OS_CREATETASK(&TCB_H264FileTask, "H264FileTask", XMSYS_H264FileTask, XMSYS_H264File_TASK_PRIORITY, StackH264FileTask);
	
	h264file_inited = 1;
}

void XMSYS_H264FileExit (void)
{
	
}

