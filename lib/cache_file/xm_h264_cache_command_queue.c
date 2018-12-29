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

// ������У�����H264 codec���͵�����
static H264_CACHE_FILE_CMD cmd_unit[H264_CACHE_FILE_CMD_QUEUE_SIZE];	
static queue_s		cmd_pool;		// �Ŷ���������
static queue_s		cmd_free;		// ������������

static void *cache_cmd_access_sema;		// Cache�����ڲ����ݻ�������ź���
static void *cache_cmd_count_sema;		// Cache��������ź���

// ��¼�Ŷ����������е�ǰ���������
static int cache_cmd_count;
// ��¼�Ŷ���������������������
static int max_cache_cmd_count;		
// ͳ��������������ʱ
static int maximum_command_insert_delay_time;

static unsigned int max_cmd_done_delay;

// Cache������г�ʼ��
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

	// ��������ź���
	cache_cmd_count_sema = XM_CreateSemaphore ("cache_cmd_count_sema", 0);
	assert (cache_cmd_count_sema);
	// ���ʻ��Ᵽ���ź���
	cache_cmd_access_sema = XM_CreateSemaphore ("cache_cmd_access_sema", 1);
	assert (cache_cmd_count_sema);

	maximum_command_insert_delay_time = 0;
	max_cache_cmd_count = 0;
	cache_cmd_count = 0;
	max_cmd_done_delay = 0;
}

// Cache��������˳�
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

// ��Cache�ļ�������뵽�Ŷ�������е�β����H264 Cache�ļ����ã�
// 1 �ɹ�
// 0 ʧ��
int xm_h264_cache_command_queue_insert_command (int file_index, int cache_cmd, void *cmd_para)
{
	H264_CACHE_FILE_CMD *cmd;
	H264_CACHE_FILE_CMD *loop_cmd;
	H264_CACHE_BLOCK *block;		// ��¼д������Ŀ��ַ
	int skip = 0;
	int s_ticket, e_ticket;


	// printf ("InsertCommand file=%d, cmd=%d\n", cache_file, cache_cmd);
	if(cache_cmd == H264_CACHE_FILE_CMD_WRITE)
	{
		block = (H264_CACHE_BLOCK *)cmd_para;

		if(XM_WaitSemaphore (cache_cmd_access_sema) == 0) /* Make sure nobody else uses */
			return 0;

		// ������ͬ�Ŀ�д�����Ƿ���Cache
		loop_cmd = (H264_CACHE_FILE_CMD *)queue_next (&cmd_pool);
		while((queue_s *)loop_cmd != &cmd_pool)
		{
			// ���д�����Ƿ������ͬ���ַ����
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
		if(skip)	// ͬһ�����ַд�����
			return 1;
	}
	
	s_ticket = XM_GetTickCount ();
	if(XM_WaitSemaphore (cache_cmd_access_sema) == 0) /* Make sure nobody else uses */
		return 0;
	cmd = NULL;
	while(!cmd)
	{
		// ����Ƿ����δʹ�õĿ������Ԫ
		if(queue_empty (&cmd_free))
		{
			// H264_FATAL ("H264_CACHE_FILE Can't Insert Command(%d)\n", cache_cmd);
			XM_SignalSemaphore (cache_cmd_access_sema);
			// ��ʱ���ȴ�Cache�ļ������ͷ�����
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

	// ��ʼ�������뵽�������β��
	cmd->file_index = file_index;
	cmd->cache_cmd = cache_cmd;
	cmd->cmd_para = cmd_para;
	queue_insert ((queue_s *)cmd, &cmd_pool);

	// ͳ�ƶ�������δִ�е��������
	cache_cmd_count ++;

	if(cache_cmd_count > max_cache_cmd_count)
	{
		max_cache_cmd_count = cache_cmd_count;
		//printf ("maximum cache command count %d\n", max_cache_cmd_count);
	}		
	
	// �ͷŷ��ʻ��Ᵽ��
	XM_SignalSemaphore (cache_cmd_access_sema);

	// �ۼ�cache���������
	XM_SignalSemaphore (cache_cmd_count_sema);

	return 1;
}

// ��Cache�ļ��������ȡ��һ�����
// ������Ϊ��ʱ�������������ֱ���µ�������뵽���������
// ����ֵ 1  �ɹ�ȡ��һ������ 
//        0  ����� 
//        -1 ʧ��
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
	
	// ���Ŷ�������ж�ȡ��һ���ļ����������
	if(queue_empty (&cmd_pool))
	{
		// ����Ϊ��
		ret = 0;
	}
	else
	{
		cmd = (H264_CACHE_FILE_CMD *)queue_delete_next (&cmd_pool);
		*file_index = cmd->file_index;
		*cache_cmd = cmd->cache_cmd;
		*cmd_para = cmd->cmd_para;
		// �ͷŵ���������
		queue_insert ((queue_s *)cmd, &cmd_free);
		ret = 1;
	}
	
	XM_SignalSemaphore (cache_cmd_access_sema);
	return ret;
}

// �ȴ�ֱ������Cache�������
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