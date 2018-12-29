//****************************************************************************
//
//	Copyright (C) 2013 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: FAT_FreeCluster.c
//	  ���дؼ�����
//
//	Revision history
//
//		2013.01.02	ZhuoYongHong Initial version
//
//****************************************************************************



/*********************************************************************
*
*             #include Section
*
**********************************************************************
*/

#include "FAT_Intern.h"
#include "xm_queue.h"
#include "rtos.h"
#include <assert.h>
#include "xm_printf.h"
#include "xm_core.h"

//#define MAX_CLUSTER_SET	4			// ���޲���ʹ��, ���Ը����쳣����
//#define HASH_COUNT			4			// ���޲���ʹ��, ���Ը����쳣����
#define MAX_CLUSTER_SET		(64*1024)
#define HASH_COUNT			(512)	
#define CLUSTER_CACHE_ID	0x4643494E


// �ؼ���(һ��������������Ŀհ״ع��ɵ����)
typedef struct tagCLUSTER_SET CLUSTER_SET;
typedef struct tagCLUSTER_SET{
	CLUSTER_SET *		prev;
	CLUSTER_SET *		next;
	U32			cluster;	// ��ʼ�غ�
	U32			size;		// ��������Ĵص�����
} CLUSTER_SET;

typedef struct {
	U32			id;						// "FCIN"

	queue_s		free[HASH_COUNT];		// ���дؼ����������
	queue_s		pool;						// δʹ�ôؼ���Ԫ���������
	
	// start/end��ʾ��ǰ��ɨ���������С/���غ�
	// ���ڵ��ؼ�cacheȫ���ľ�ʱ, ��Ҫɨ��FAT��������δʹ�õĴز����뵽�ؼ�cache
	// ǰ��ɨ��(����end) ���� ����ɨ��(��Сstart)
	U32			start;		// ��ɨ���������ʼ�غ�
	U32			end;			// ��ɨ������Ľ����غ�

	U32			count;		// ���дصĸ���, ���ڿ��ټ����Ƿ���Ҫ����ɨ������������Ŀ��дغ�(������ĳ����ֵʱ)
	
	U32			last_free_cluster;	// ����ͷŵĴغ�
												// ��¼����ͷŵĴغ�(��һ�����õĻ���ܴ�, ��Ϊ�ôض�Ӧ��FAT�����������Ŀ����Դ�����Cache��, ���ÿɼ��ٴ��̵Ķ�����)

	CLUSTER_SET *curr_set;	// ��ǰ����ʹ�õ����ڿ��дط���Ĵؼ�
										//	ֻ�е�ǰ�ؼ�������Ϻ�(size == 0), ��Ѱ����һ�����дؼ����ں����Ĵط���


	CLUSTER_SET set[MAX_CLUSTER_SET];		// �ؼ���Ԫ��ֵ


} CLUSTER_CACHE;

#if FS_FAT_USE_CLUSTER_CACHE
/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
void FS_FAT_ClusterCacheInitialize (FS_FAT_INFO * pFATInfo)
{
	CLUSTER_CACHE *cache = kernel_malloc (sizeof(CLUSTER_CACHE));
	if(cache)
	{
		U32 i;
		memset (cache, 0, sizeof(CLUSTER_CACHE));
		cache->id = CLUSTER_CACHE_ID;
		cache->count = 0;
		cache->start = 0;
		cache->end = 0;
		cache->last_free_cluster = 0;
		cache->curr_set = NULL;
		queue_initialize (&cache->pool);
		for (i = 0; i < MAX_CLUSTER_SET; i ++)
		{
			queue_insert ((queue_s *)(&cache->set[i]), &cache->pool);
		}
		for (i = 0; i < HASH_COUNT; i ++)
			queue_initialize (&cache->free[i]);
	}
	pFATInfo->ClusterCache = cache;
}

void FS_FAT_ClusterCacheDeInitialize (FS_FAT_INFO * pFATInfo)
{
	CLUSTER_CACHE *cache;
	if(pFATInfo)
	{
		cache = (CLUSTER_CACHE *)pFATInfo->ClusterCache;
		if(cache)
		{
#if _DEBUG
			assert (cache->id == CLUSTER_CACHE_ID);
#endif			
			if(cache->id != CLUSTER_CACHE_ID)
				return;
			cache->id = 0;
			kernel_free (cache);
			pFATInfo->ClusterCache = NULL;
		}
	}
}

// ��ȡ��ʼ�غ���С�Ĵؼ���Ԫ
static CLUSTER_SET *get_head_set (CLUSTER_CACHE *cache)
{
	int i;
	CLUSTER_SET *curr_set;
	CLUSTER_SET *head_set = NULL;
	// �������е�ɢ�б�, �Ƚ�ÿ��ɢ�б�ͷ���Ĵؼ�����ʼ�غ�, ��¼������С��ʼ�غŵĴؼ���Ԫ
	for (i = 0; i < HASH_COUNT; i ++)
	{
		if(!queue_empty(&cache->free[i]))
		{
			// ָ�������еĵ�һ���ؼ���Ԫ, ����ʼ�غ���С
			curr_set = (CLUSTER_SET *)queue_next (&cache->free[i]);
			if(head_set == NULL)
				head_set = curr_set;
			else
			{
				if(curr_set->cluster < head_set->cluster)
					head_set = curr_set;
			}
		}
	}
	return head_set;
}

// ��ȡ��ʼ�غ����Ĵؼ���Ԫ
static CLUSTER_SET *get_tail_set (CLUSTER_CACHE *cache)
{
	int i;
	CLUSTER_SET *curr_set;
	CLUSTER_SET *tail_set = NULL;
	// �������е�ɢ�б�, �Ƚ�ÿ��ɢ�б�β���Ĵؼ�����ʼ�غ�, ��¼���������ʼ�غŵĴؼ���Ԫ
	for (i = 0; i < HASH_COUNT; i ++)
	{
		if(!queue_empty(&cache->free[i]))
		{
			// ָ�������е����һ���ؼ���Ԫ, ����ʼ�غ����
			curr_set = (CLUSTER_SET *)queue_prev (&cache->free[i]);
			if(tail_set == NULL)
				tail_set = curr_set;
			else
			{
				if(curr_set->cluster > tail_set->cluster)
					tail_set = curr_set;
			}
		}
	}
	return tail_set;
}

// ��ȡ�������������Ĵؼ���Ԫ
static CLUSTER_SET *get_max_count_set (CLUSTER_CACHE *cache, U32 require_cluster_count)
{
	int i;
	CLUSTER_SET *curr_set;
	CLUSTER_SET *max_count_set = NULL;
	U32 max_count = 0;
	// �������е�ɢ�б�, �Ƚ�ÿ��ɢ�б�ͷ���Ĵؼ�����ʼ�غ�, ��¼������С��ʼ�غŵĴؼ���Ԫ
	for (i = 0; i < HASH_COUNT; i ++)
	{
		queue_s *head = &cache->free[i];
		if(!queue_empty(head))
		{
			// ָ�������еĵ�һ���ؼ���Ԫ
			curr_set = (CLUSTER_SET *)queue_next (head);
			while(curr_set != (CLUSTER_SET *)head)
			{
				if(curr_set->size > max_count)
				{
					max_count = curr_set->size;
					max_count_set = curr_set;
					if(max_count >= require_cluster_count)		// �ҵ���һ����������������Ĵؼ�
					{
						return max_count_set;
					}
				}
				curr_set = (CLUSTER_SET *)queue_next ((queue_s *)curr_set);
			}
		}
	}
	return max_count_set;
}


// ������ָ���غ�ƥ��Ĵؼ�
static CLUSTER_SET *match_cluster_set (CLUSTER_CACHE *cache, U32 Cluster)
{
	CLUSTER_SET *curr_set;
	// ����hash����
	U32 hash_index;
	
	if(Cluster == 0)
		return NULL;
	
	hash_index = (Cluster >> 12) & (HASH_COUNT - 1);
	// ����������ͬɢ��ֵ������
	if(!queue_empty(&cache->free[hash_index]))
	{
		// ��������
		curr_set = (CLUSTER_SET *)queue_next (&cache->free[hash_index]);
		while( (queue_s *)curr_set != &cache->free[hash_index])
		{
			if(curr_set->cluster <= Cluster && (curr_set->cluster + curr_set->size) > Cluster )
				return curr_set;

			curr_set = curr_set->next;
		}
	}

	return NULL;
}

// 1) ����ָ���غŵĴؼ���Ԫ, ���ҵ�, �򷵻ظôؼ���Ԫ
// 2) ��û���ҵ�, �������ʼ�غŴ��ڸôغŲ������ڽӵĴؼ���Ԫ.
// 3) ��δ�ҵ�,��
static CLUSTER_SET *find_cluster_set (CLUSTER_CACHE *cache, U32 Cluster)
{
	int i;
	CLUSTER_SET *curr_set;
	CLUSTER_SET *match_set = NULL;
	// �������е�ɢ�б�, �Ƚ�ÿ��ɢ�б�β���Ĵؼ�����ʼ�غ�, ��¼���������ʼ�غŵĴؼ���Ԫ
	for (i = 0; i < HASH_COUNT; i ++)
	{
		if(!queue_empty(&cache->free[i]))
		{
			// ��������
			curr_set = (CLUSTER_SET *)queue_next (&cache->free[i]);
			while( (queue_s *)curr_set != &cache->free[i])
			{
				if( curr_set < &cache->set[0] || curr_set >= &cache->set[MAX_CLUSTER_SET] )
				{
					assert (0);
				}
				if(curr_set->cluster == Cluster)
					return curr_set;

				// ��¼��������ӽ�ƥ��ֵ�Ĵؼ�
				if(curr_set->cluster > Cluster && curr_set->size >= 2)
				{
					if(match_set == NULL)
						match_set = curr_set;
					else
					{
						if(curr_set->cluster < match_set->cluster)
							match_set = curr_set;
					}
				}
				curr_set = curr_set->next;
			}
		}
	}

	return match_set;
}

// ��һ�����дؼ��뵽���д�Cache
void FS_FAT_ClusterCacheFree (FS_FAT_INFO * pFATInfo, U32 Cluster)
{
	CLUSTER_CACHE *cache;
	CLUSTER_SET *curr_unit;
	// ����hash����
	U32 hash_index = (Cluster >> 12) & (HASH_COUNT - 1);
	queue_s *free;
	CLUSTER_SET *set_to_insert;		// ��¼����Ŵ���Cluster�ļ���,�¼Ӵز��뵽��ǰ��

#if _DEBUG
	assert (Cluster >= 2);
#endif

	cache = (CLUSTER_CACHE *)pFATInfo->ClusterCache;
	if(cache == NULL)
		return;
#if _DEBUG
	assert (cache->id == CLUSTER_CACHE_ID);
#endif
	if(cache->id != CLUSTER_CACHE_ID)
		return;
	
	// ����¼ӵĴ��Ƿ�λ��start/end֮�� (start <= Cluster <= end)
	if(Cluster < cache->start || Cluster > cache->end)
	{
		// �¼Ӵس������д�cache���дصı߽�, �������
		return;
	}
	
	// ��¼����ͷŵĴغ�(��һ�����õĻ���ܴ�, ��Ϊ�ôض�Ӧ��FAT�����������Ŀ����Դ�����Cache��, ���ÿɼ��ٴ��̵Ķ�����)
	if(cache->last_free_cluster == 0)
		cache->last_free_cluster = Cluster; 

	// ��λ�ôغŶ�Ӧ�Ĺ�ϣֵ��Ӧ�Ŀ��дؼ�����
	free = &cache->free[hash_index];
	
	// ��ʼ������λ��Ϊ��
	set_to_insert = NULL;	

	// �����дؼ������Ƿ�ǿ�.  	
	// ���ǿ�, ����������, 1) �����¼ӵĴ��Ƿ�ɺϲ������еĴؼ�. 2)���޷��ϲ�,���Ҳ����
	if(!queue_empty(free))
	{
		// ���ڿ��еĴؼ���Ԫ, ���Խ��µ�Cluster���뵽�����к��ʵ�λ��
		CLUSTER_SET *set = (CLUSTER_SET *)queue_next (free);
		// ����ÿ���ؼ���Ԫ
		while((queue_s *)set != free)
		{
#if _DEBUG
			assert (set->cluster != Cluster);
#endif				
			// ����¼Ӵ��Ƿ������дؼ���Ԫ�ڽ�
			// ��ؼ�β���ڽ� ?
			if( (set->cluster + set->size) == Cluster )
			{
				// ���¼���Ĵغϲ�����ǰ�ļ���
				set->size ++;		// ���´ؼ��Ĵ�����

				// �ۼӿ��д�����
				cache->count ++;
				return;
			}
			// ��ؼ��ײ��ڽ� ?
			else if( (Cluster + 1) == set->cluster )
			{
				// ���¼���Ĵغϲ�����ǰ�ļ���
				set->size ++;				// ���´ؼ��Ĵ�����
				set->cluster = Cluster;		// ���´ؼ�����ʼ�غ�
				// �ۼӿ��д�����
				cache->count ++;
				return;
			}

			if(set->cluster > Cluster)
			{
				// ��ֹ��������
				// ��¼�ò����, �������ָ���¼Ӵؼ���Ԫ���뵽�ôؼ��ϵ�ǰ��
				set_to_insert = set;
				break;
			}

			set = (CLUSTER_SET *)set->next;
		}
	}

	// δ�ҵ�һ�����Ժϲ��Ĵؼ�. ��ҪΪ�¼ӵĴش���һ���ؼ���Ԫ������ص���Ϣ

	// ����Ƿ���ڿ��õĿ��дؼ���Ԫ
	if(queue_empty(&cache->pool))
	{
		// �޿��õĿ��дؼ���Ԫ, ��Ҫ�滻��ʼ�غ���С����ʼ�غ����Ĵؼ���Ԫ

		CLUSTER_SET *head_set, *tail_set;

		// �滻ͷ����β���ļ��� ?
		// �����滻��ǰ���ڷ���Ĵؼ�, ȷ�����������������Ĵ�
		head_set = get_head_set (cache);
		tail_set = get_tail_set (cache);

		// ����¼ӵĿ��дشغ��Ƿ�С�ڵ�һ���ؼ�����ʼ�غ�. 
		// ����, �ôز����뵽�ؼ�cache��, ֻ�����µ���cache��startֵ, ���ôش���ɨ���������ų�
		if(Cluster < head_set->cluster)
		{
			cache->start = Cluster + 1;
			return;
		}
		// ����¼ӵĿ��дشغ��Ƿ�������һ���ؼ��Ľ����غ�. 
		// ����, �ôز����뵽�ؼ�cache��, ֻ�����µ���cache��endֵ, ���ôش���ɨ���������ų�
		else if(Cluster > (tail_set->cluster + tail_set->size - 1))
		{
			cache->end = Cluster - 1;
			return;
		}

		// ��鵱ǰ���ڷ���Ĵؼ��Ƿ��ǵ�һ���ؼ�(������С��ʼ�غ�), ����, ǿ���滻���һ���ؼ���Ԫ(���������ʼ�غ�).   
		if(cache->curr_set == head_set)	
		{
			// �滻���һ���ؼ���Ԫ
			curr_unit = tail_set;
		}
		// ��鵱ǰ���ڷ���Ĵؼ������һ���ؼ�(���������ʼ�غ�). ����, ǿ���滻��һ���ؼ�
		else if(cache->curr_set == tail_set)
		{
			// �滻��һ���ؼ�
			curr_unit = head_set;
		}
		// �滻���д�������Խ�С�Ĵؼ�
		else if( head_set->size > tail_set->size )
		{
			// �滻���һ���ؼ���Ԫ
			curr_unit = tail_set;
		}
		else
		{
			// �滻��һ���ؼ�
			curr_unit = head_set;
		}

		if(curr_unit == tail_set)
		{
			// �滻���һ���ؼ���Ԫ
			XM_printf ("replace tail_set, cache(start=%d, end=%d), Cluster=%d, curr_set(cluster=%d, size=%d), head_set(cluster=%d, size=%d), tail_set(cluster=%d, size=%d)\n", 
					cache->start, cache->end, Cluster, cache->curr_set->cluster, cache->curr_set->size, head_set->cluster, head_set->size, tail_set->cluster, tail_set->size);

			// �����滻�ؼ���Ԫ�Ĵش���ɨ���������ų�
			cache->count -= tail_set->size;		// ��ȥ���дص�����
#if _DEBUG
			assert ( (tail_set->cluster + tail_set->size - 1) <= cache->end  );
#endif
			// �޸���ɨ�����������(end), �����ޱ�ʾ��һ��"ǰ��ɨ��"��cache->end + 1��ʼ, ����ǰ���滻��ȥ�Ĵؼ�����ʼ�غ�
			cache->end = tail_set->cluster - 1;		

			// �����滻�����Ƿ��ǲ����
			if(set_to_insert == tail_set)
			{
				XM_printf ("set_to_insert == tail_set\n");
				set_to_insert = NULL;
			}
		}
		else
		{
			// �滻��һ���ؼ���Ԫ
			XM_printf ("replace head_set, cache(start=%d, end=%d), Cluster=%d, curr_set(cluster=%d, size=%d), head_set(cluster=%d, size=%d), tail_set(cluster=%d, size=%d)\n", 
					cache->start, cache->end, Cluster, cache->curr_set->cluster, cache->curr_set->size, head_set->cluster, head_set->size, tail_set->cluster, tail_set->size);

			// �滻��һ���ؼ���Ԫ
			// �����滻�ؼ���Ԫ�Ĵش���ɨ���������ų�
			cache->count -= head_set->size;		// ��ȥ���дص�����
			// �޸���ɨ�����������(start), �����ޱ�ʾ��һ��"����ɨ��"��cache->start - 1��ʼ, ����ǰ���滻��ȥ�Ĵؼ������غ�
#if _DEBUG
			assert ( head_set->cluster >= cache->start );
#endif
			// �����滻�ؼ�����ɨ���������ų�
			cache->start = head_set->cluster + head_set->size;		
			// �����滻�����Ƿ��ǲ����. �����ǲ����
			assert(set_to_insert != head_set);
		}

		// �ӿ��дؼ������жϿ�
		queue_delete ((queue_s *)curr_unit);
		curr_unit->cluster = 0;
		curr_unit->size = 0;
	}
	else
	{
		// ��������һ�����е�"�ؼ�"��Ԫ
		// ��δʹ�ôؼ����л�ȡһ���ؼ���Ԫ
		curr_unit = (CLUSTER_SET *)queue_delete_next (&cache->pool);
	}

#if _DEBUG
	assert (curr_unit->cluster == 0);
	assert (curr_unit->size == 0);
#endif

	// ���ҵ�һ�����еĴؼ���Ԫcurr_unit
	curr_unit->cluster = Cluster;
	curr_unit->size = 1;
	cache->count ++;	// �ۼӿ��д�����

	// ���մ���Ž��ؼ��ϲ��뵽���ʵ�λ��
	if(set_to_insert) 
	{
		// �ҵ��²����,���¼��ϲ��뵽�ò�����ǰ��
		queue_insert ((queue_s *)curr_unit, (queue_s *)set_to_insert);
	}
	else
	{
		queue_insert ((queue_s *)curr_unit, &cache->free[hash_index]);		// ���뵽���д�cache�Ķ���β��
	}
}



// �ӿ��д�cache�з���һ�����д�
// ����ֵ
// 0    ���д�cache����û�п��еĴؿ�, ����ʧ��
//      ���õĿ��дشغ�
U32 FS_FAT_ClusterCacheAllocate (FS_FAT_INFO * pFATInfo)
{
	CLUSTER_CACHE *cache;
	CLUSTER_SET *curr_set;

	U32 Cluster;
	// ����hash����
	U32 hash_index;

	cache = (CLUSTER_CACHE *)pFATInfo->ClusterCache;
	if(cache == NULL)
		return 0;
#if _DEBUG
	assert (cache->id == CLUSTER_CACHE_ID);
#endif
	if(cache->id != CLUSTER_CACHE_ID)
		return 0;
	// �����д�cache�Ƿ��
	if(cache->count == 0)
		return 0;

	// ��鵱ǰ���дط���ʹ�õĴؼ���. ���Ϊ��, ָ����д�cache�ĵ�һ������(������С�غ�)
	if(cache->curr_set == NULL)
	{
		// ��������ɢ�б������, ֱ���ҵ�������С�غŵĴؼ�
		cache->curr_set = get_head_set (cache);
#if _DEBUG
		assert (cache->curr_set);
#endif
		//printf ("cluster=%d, size=%d\n", cache->curr_set->cluster, cache->curr_set->size);
	}
	curr_set = cache->curr_set;

	// �ӵ�ǰ���дط���Ĵؼ��з���һ����
	Cluster = curr_set->cluster;	// �����еĵ�һ����
	curr_set->size --;
	// �������дص�����
	cache->count --;
	hash_index = (Cluster >> 12) & (HASH_COUNT - 1);

	// �жϴؼ��Ĵ���Դ�Ƿ���ȫ������
	if(curr_set->size > 0)
	{
		// �ؼ��еĴ�δȫ������
		curr_set->cluster ++;		// ������һ��������صĴغ�
	}
	else
	{
		// �ؼ��еĴ���ȫ������
		// Ѱ����һ���ؼ����ڴط���

		// ��¼�ôؼ����ڹ�ϣɢ���������һ���ؼ�.
		CLUSTER_SET *next_set = curr_set->next;
		// ���ôؼ��ӿ��д�cache��ɾ��
		queue_delete ((queue_s *)curr_set);
		// ������뵽δʹ�ü��ϵ�����
		curr_set->cluster = 0;
		curr_set->size = 0;
		queue_insert ((queue_s *)curr_set, &cache->pool);
		
#if 1
		// ���� 1
		// 	���Ҵغ���������һ���ض�Ӧ�Ĵؼ��Ƿ����
		// 	������,���ôؼ�������һ�η���
		next_set = match_cluster_set (cache, Cluster + 1);
		// 20170225 �޸�bug. 
		// if(next_set == NULL)
		if(next_set)	
		{
			// ���ڵ�ַ��������һ���ؼ�, ʹ�øôؼ���Ϊ��һ���ط���Ĵؼ�
			cache->curr_set = next_set;
		}
		else
		{
			// ������ƥ��Ĵؼ�
			
			// 20170225 zhuoyonghongʹ���ϲ��ļ�Cacheϵͳ��æ״̬�������صķ���
			//	��Cacheϵͳ��æʱ, ����һ���������������Ĵؼ���Ϊ��һ���ط���Ĵؼ�,
			// ��Cacheϵͳ����ʱ, ���������ط������
			
			// ��ȡCacheϵͳæ�ȴ��ļ���
			// 0 ~ 1 ��ʾϵͳCache��������, ��������
			// 2 ~ 8 ����Խ��, ��ʾʹ�õ�Cache����Խ��
			// 9     ��ʾ�ڲ�Cache����, ��ʱ�޷�д��
			unsigned int XMSYS_GetCacheSystemBusyLevel (void);
			
			if(XMSYS_GetCacheSystemBusyLevel() >= 5)
			{
				// Cacheϵͳ�Ѿ����غܴ�, ��Ҫ�Ż���д��. ���������Ĵؿ���Լ��ٴر�ķ��ʴ���
				unsigned int require_cluster_count = 0x200000 / pFATInfo->BytesPerCluster;
				next_set = get_max_count_set (cache, require_cluster_count);
				if(next_set)
				{
					cache->curr_set = next_set;
					goto cache_alloc_exit;
				}
			}
			
			// ��������ͷŵĴض�Ӧ�Ĵؼ�(����ͷŵĴ���ζ�Ÿôض�Ӧ��FAT����λ��cache��,���Լ��ٶ����̵ķ��ʿ���)
			next_set = match_cluster_set (cache, cache->last_free_cluster);
			if(next_set && next_set->size >= 2)		// �ؼ���������С��2
			{
				// ���غŶ�Ӧ�Ĵؼ�������һ�η���, ����������ͷŵĴغű��Ϊ��Ч
				cache->curr_set = next_set;
				cache->last_free_cluster = 0;
			}
			else
			{
				// ���ұȲ���ֵ������ӽ��Ĵؼ�
				next_set = find_cluster_set (cache, Cluster + 1);
				// 	��������, ���ҵ�������С�غŵĴؼ�������һ�η���
				if(next_set == NULL)
					next_set = get_head_set (cache);
				cache->curr_set = next_set;
			}
		}
		
#else
		// �ж���һ���ؼ��Ƿ����
		if(next_set == (CLUSTER_SET *)(&cache->free[hash_index]))
		{
			// ��һ���ؼ�������(ָ���������)
			// curr_set�������е����һ���ؼ�
			// ��������Ƿ�Ϊ��
			if(queue_empty(&cache->free[hash_index]))
			{
				// �´δط���ʱ�����������ؼ�, ��������С�غŵĴؼ���Ϊ��һ���ط���ĵ�ǰ�ؼ���
				cache->curr_set = NULL;
			}
			else
			{
				// ��������׵ĵ�һ���ؼ���Ϊ��һ���ط���ĵ�ǰ�ؼ���
				cache->curr_set = (CLUSTER_SET *)queue_next (&cache->free[hash_index]);
			}
		}
		else
		{
			// ������һ���ؼ�, ��ؾ��нϴ�Ŀ�����ԭ�ȷ���Ĵ��ڽ�.
			// ������Ϊ��һ���ط���ĵ�ǰ�ؼ���
			cache->curr_set = next_set;
		}
		if(cache->curr_set)
		{
			//printf ("cluster=%d, size=%d\n", cache->curr_set->cluster, cache->curr_set->size);
		}
#endif
	}

cache_alloc_exit:
	return Cluster;
}

// ���ؿ��д�cache�Ĵ�����
U32 FS_FAT_ClusterCacheCount (FS_FAT_INFO * pFATInfo)
{
	CLUSTER_CACHE *cache;
	cache = (CLUSTER_CACHE *)pFATInfo->ClusterCache;
	if(cache == NULL)
		return 0;
#if _DEBUG
	assert (cache->id == CLUSTER_CACHE_ID);
#endif
	// �����д�cache�Ƿ��
	return cache->count;
}

static U32 _ClusterId2FATOff(U8 FATType, U32 ClusterId) {
  switch (FATType) {
  case FS_FAT_TYPE_FAT12:
    return ClusterId + (ClusterId >> 1);
  case FS_FAT_TYPE_FAT16:
    return ClusterId << 1;
  }
  return ClusterId << 2;
}

// ɨ����еĴؿ鲢������뵽���д�cache
void FS_FAT_ClusterCacheScan (FS_VOLUME * pVolume, FS_SB * pSB)
{
	CLUSTER_CACHE *cache;
	FS_FAT_INFO * pFATInfo;
	U32 scan_start_cluster, scan_last_cluster;
	U8  FATType;
	U32 Off;          /* Total offset in bytes */
	U32 SectorNo;
	U32 LastSectorNo;
	U32 SectorCount;

	pFATInfo = &pVolume->FSInfo.FATInfo;
	cache = (CLUSTER_CACHE *)pFATInfo->ClusterCache;
	if(cache == NULL)
		return;
#if _DEBUG
	assert (cache->id == CLUSTER_CACHE_ID);
#endif

	// ���δʹ�õĴؼ������Ƿ�Ϊ��.
	// ��Ϊ��, ��ʱû�п��Է���Ĵؼ�, ���ð���ɨ��
	if(queue_empty(&cache->pool))
		return;

	// ���ٴ���һ�����еĴؼ�

	// ���start/end�Ƿ��Ѹ������е�FAT��.
	// ���Ѹ���, ����ɨ��
	if(cache->start == FAT_FIRST_CLUSTER && cache->end == (pFATInfo->NumClusters + FAT_FIRST_CLUSTER - 1))
	{
		// ���Ѹ���, ����ɨ��
		return;
	}

	if(cache->start == 0 && cache->end == 0)
	{
		// ��ʼɨ��
		cache->start = 2;
		cache->end = 1;
	}

	FATType = pFATInfo->FATType;
	// û����ȫ����,�Ҵ���δʹ�õĴؼ���Ԫ, ���԰��ſ��д�ɨ��
	// 1) �ж��Ƿ�������ɨ�� (��������). ���ѵ���غ����ֵ, ת��2)
	if(cache->end < (pFATInfo->NumClusters + FAT_FIRST_CLUSTER - 1))
	{
		scan_start_cluster = cache->end + 1;
		scan_last_cluster = pFATInfo->NumClusters + FAT_FIRST_CLUSTER - 1;
		LastSectorNo = 0;
		SectorCount = 0;
		while(scan_start_cluster <= scan_last_cluster)
		{
			cache->end ++;	// ������ɨ������߽������
			if(cache->end == scan_last_cluster)
			{
				XM_printf ("scan has reached up-limit %d\n", cache->end);
				if(cache->start == FAT_FIRST_CLUSTER)
				{
					XM_printf ("scan has full coveraged\n");
				}
			}
			// �жϸô��Ƿ�δʹ��
			if (FS_FAT_ReadFATEntry(pVolume, pSB, scan_start_cluster) == 0)
			{
				// δʹ�õĴ�(�հ״�)
				// ������뵽���дؼ�cache
				FS_FAT_ClusterCacheFree (pFATInfo, scan_start_cluster);
			}
				
			// �ۼ�ɨ������ж�ȡ�Ĳ�ͬ��������.
			// ����һ��������,�˳�ɨ��ѭ��, ����ɨ�赼����������ȴ�ʱ�����
			Off = _ClusterId2FATOff(FATType, scan_start_cluster);
			SectorNo = pFATInfo->RsvdSecCnt + (Off >> pFATInfo->ldBytesPerSector);
			if(LastSectorNo != SectorNo)
			{
				LastSectorNo = SectorNo;
				SectorCount ++;
				if(SectorCount >= 4)		// 4�������Ķ�ȡʱ��+���4���������Ļ�дʱ��
				{
					break;
				}
			}
			scan_start_cluster ++;
		}
	}
	// 2) �ж��Ƿ������ǰɨ�� (��С����).
	else if(cache->start != FAT_FIRST_CLUSTER)
	{
		scan_start_cluster = cache->start - 1;
		LastSectorNo = 0;
		SectorCount = 0;
		while(scan_start_cluster >= FAT_FIRST_CLUSTER)
		{
			cache->start --;	// ������ɨ������߽������
			if(cache->start == FAT_FIRST_CLUSTER)
			{
				XM_printf ("scan has reached down-limit %d\n", cache->start);
				if(cache->end == (pFATInfo->NumClusters + FAT_FIRST_CLUSTER - 1))
				{
					XM_printf ("scan has full coveraged\n");
				}
			}
			// �жϸô��Ƿ�δʹ��
			if (FS_FAT_ReadFATEntry(pVolume, pSB, scan_start_cluster) == 0)
			{
				// δʹ�õĴ�
				// ������뵽���дؼ�cache
				FS_FAT_ClusterCacheFree (pFATInfo, scan_start_cluster);
			}
				
			// �ۼ�ɨ������ж�ȡ�Ĳ�ͬ��������.
			// ����һ��������,�˳�ɨ��ѭ��, ����ɨ�赼����������ȴ�ʱ�����
			Off = _ClusterId2FATOff(FATType, scan_start_cluster);
			SectorNo = pFATInfo->RsvdSecCnt + (Off >> pFATInfo->ldBytesPerSector);
			if(LastSectorNo != SectorNo)
			{
				LastSectorNo = SectorNo;
				SectorCount ++;
				if(SectorCount >= 4)		// 4�������Ķ�ȡʱ��+���4���������Ļ�дʱ��
				{
					break;
				}
			}

			scan_start_cluster --;
		}
	}

}

// ���ؼ�cache�Ƿ��Ѹ������е�FAT����
//   δȫ������ʱ, pLowLimit������ɨ�����������, pHighLimit������ɨ�����������
// ����ֵ
// 1    ��ȫ������
// 0    δȫ������
// -1	  ����
int FS_FAT_ClusterCacheIsFullCoverage (FS_FAT_INFO * pFATInfo, U32 *pLowLimit, U32 *pHighLimit)
{
	CLUSTER_CACHE *cache;
	cache = (CLUSTER_CACHE *)pFATInfo->ClusterCache;
	if(cache == NULL)
	{
#if _DEBUG
		assert (0);
#endif
		return -1;
	}
#if _DEBUG
	assert (cache->id == CLUSTER_CACHE_ID);
#endif
	if(cache->start == FAT_FIRST_CLUSTER && cache->end == (pFATInfo->NumClusters + FAT_FIRST_CLUSTER - 1))
		return 1;
	else
	{
		*pLowLimit = cache->start;
		*pHighLimit = cache->end;
		return 0;
	}
}
#endif