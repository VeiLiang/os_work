//****************************************************************************
//
//	Copyright (C) 2013 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: FAT_FreeCluster.c
//	  空闲簇集管理
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

//#define MAX_CLUSTER_SET	4			// 极限测试使用, 测试各种异常处理
//#define HASH_COUNT			4			// 极限测试使用, 测试各种异常处理
#define MAX_CLUSTER_SET		(64*1024)
#define HASH_COUNT			(512)	
#define CLUSTER_CACHE_ID	0x4643494E


// 簇集合(一个或多个序号连续的空白簇构成的组合)
typedef struct tagCLUSTER_SET CLUSTER_SET;
typedef struct tagCLUSTER_SET{
	CLUSTER_SET *		prev;
	CLUSTER_SET *		next;
	U32			cluster;	// 起始簇号
	U32			size;		// 序号连续的簇的数量
} CLUSTER_SET;

typedef struct {
	U32			id;						// "FCIN"

	queue_s		free[HASH_COUNT];		// 空闲簇集的链表入口
	queue_s		pool;						// 未使用簇集单元的链表入口
	
	// start/end表示当前已扫描区域的最小/最大簇号
	// 用于当簇集cache全部耗尽时, 需要扫描FAT表来搜索未使用的簇并加入到簇集cache
	// 前进扫描(增加end) 或者 后退扫描(减小start)
	U32			start;		// 已扫描区域的起始簇号
	U32			end;			// 已扫描区域的结束簇号

	U32			count;		// 空闲簇的个数, 用于快速计算是否需要启动扫描来搜索更多的空闲簇号(当低于某个阈值时)
	
	U32			last_free_cluster;	// 最近释放的簇号
												// 记录最近释放的簇号(下一次重用的机会很大, 因为该簇对应的FAT扇区具有最大的可能性存在于Cache中, 重用可减少磁盘的读访问)

	CLUSTER_SET *curr_set;	// 当前正在使用的用于空闲簇分配的簇集
										//	只有当前簇集分配完毕后(size == 0), 才寻找下一个空闲簇集用于后续的簇分配


	CLUSTER_SET set[MAX_CLUSTER_SET];		// 簇集单元数值


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

// 获取起始簇号最小的簇集单元
static CLUSTER_SET *get_head_set (CLUSTER_CACHE *cache)
{
	int i;
	CLUSTER_SET *curr_set;
	CLUSTER_SET *head_set = NULL;
	// 遍历所有的散列表, 比较每个散列表头部的簇集的起始簇号, 记录具有最小起始簇号的簇集单元
	for (i = 0; i < HASH_COUNT; i ++)
	{
		if(!queue_empty(&cache->free[i]))
		{
			// 指向链表中的第一个簇集单元, 其起始簇号最小
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

// 获取起始簇号最大的簇集单元
static CLUSTER_SET *get_tail_set (CLUSTER_CACHE *cache)
{
	int i;
	CLUSTER_SET *curr_set;
	CLUSTER_SET *tail_set = NULL;
	// 遍历所有的散列表, 比较每个散列表尾部的簇集的起始簇号, 记录具有最大起始簇号的簇集单元
	for (i = 0; i < HASH_COUNT; i ++)
	{
		if(!queue_empty(&cache->free[i]))
		{
			// 指向链表中的最后一个簇集单元, 其起始簇号最大
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

// 获取具有最大簇数量的簇集单元
static CLUSTER_SET *get_max_count_set (CLUSTER_CACHE *cache, U32 require_cluster_count)
{
	int i;
	CLUSTER_SET *curr_set;
	CLUSTER_SET *max_count_set = NULL;
	U32 max_count = 0;
	// 遍历所有的散列表, 比较每个散列表头部的簇集的起始簇号, 记录具有最小起始簇号的簇集单元
	for (i = 0; i < HASH_COUNT; i ++)
	{
		queue_s *head = &cache->free[i];
		if(!queue_empty(head))
		{
			// 指向链表中的第一个簇集单元
			curr_set = (CLUSTER_SET *)queue_next (head);
			while(curr_set != (CLUSTER_SET *)head)
			{
				if(curr_set->size > max_count)
				{
					max_count = curr_set->size;
					max_count_set = curr_set;
					if(max_count >= require_cluster_count)		// 找到第一个大于所需簇数量的簇集
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


// 查找与指定簇号匹配的簇集
static CLUSTER_SET *match_cluster_set (CLUSTER_CACHE *cache, U32 Cluster)
{
	CLUSTER_SET *curr_set;
	// 计算hash索引
	U32 hash_index;
	
	if(Cluster == 0)
		return NULL;
	
	hash_index = (Cluster >> 12) & (HASH_COUNT - 1);
	// 遍历具有相同散列值的链表
	if(!queue_empty(&cache->free[hash_index]))
	{
		// 遍历链表
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

// 1) 搜索指定簇号的簇集单元, 若找到, 则返回该簇集单元
// 2) 若没有找到, 则查找起始簇号大于该簇号并且最邻接的簇集单元.
// 3) 若未找到,则
static CLUSTER_SET *find_cluster_set (CLUSTER_CACHE *cache, U32 Cluster)
{
	int i;
	CLUSTER_SET *curr_set;
	CLUSTER_SET *match_set = NULL;
	// 遍历所有的散列表, 比较每个散列表尾部的簇集的起始簇号, 记录具有最大起始簇号的簇集单元
	for (i = 0; i < HASH_COUNT; i ++)
	{
		if(!queue_empty(&cache->free[i]))
		{
			// 遍历链表
			curr_set = (CLUSTER_SET *)queue_next (&cache->free[i]);
			while( (queue_s *)curr_set != &cache->free[i])
			{
				if( curr_set < &cache->set[0] || curr_set >= &cache->set[MAX_CLUSTER_SET] )
				{
					assert (0);
				}
				if(curr_set->cluster == Cluster)
					return curr_set;

				// 记录大于且最接近匹配值的簇集
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

// 将一个空闲簇加入到空闲簇Cache
void FS_FAT_ClusterCacheFree (FS_FAT_INFO * pFATInfo, U32 Cluster)
{
	CLUSTER_CACHE *cache;
	CLUSTER_SET *curr_unit;
	// 计算hash索引
	U32 hash_index = (Cluster >> 12) & (HASH_COUNT - 1);
	queue_s *free;
	CLUSTER_SET *set_to_insert;		// 记录簇序号大于Cluster的集合,新加簇插入到其前面

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
	
	// 检查新加的簇是否位于start/end之间 (start <= Cluster <= end)
	if(Cluster < cache->start || Cluster > cache->end)
	{
		// 新加簇超出空闲簇cache已有簇的边界, 无需插入
		return;
	}
	
	// 记录最近释放的簇号(下一次重用的机会很大, 因为该簇对应的FAT扇区具有最大的可能性存在于Cache中, 重用可减少磁盘的读访问)
	if(cache->last_free_cluster == 0)
		cache->last_free_cluster = Cluster; 

	// 定位该簇号对应的哈希值对应的空闲簇集链表
	free = &cache->free[hash_index];
	
	// 初始化插入位置为空
	set_to_insert = NULL;	

	// 检查空闲簇集链表是否非空.  	
	// 若非空, 遍历该链表, 1) 查找新加的簇是否可合并到已有的簇集. 2)若无法合并,查找插入点
	if(!queue_empty(free))
	{
		// 存在空闲的簇集单元, 尝试将新的Cluster插入到链表中合适的位置
		CLUSTER_SET *set = (CLUSTER_SET *)queue_next (free);
		// 遍历每个簇集单元
		while((queue_s *)set != free)
		{
#if _DEBUG
			assert (set->cluster != Cluster);
#endif				
			// 检查新加簇是否与已有簇集单元邻接
			// 与簇集尾部邻接 ?
			if( (set->cluster + set->size) == Cluster )
			{
				// 将新加入的簇合并到当前的集合
				set->size ++;		// 更新簇集的簇数量

				// 累加空闲簇数量
				cache->count ++;
				return;
			}
			// 与簇集首部邻接 ?
			else if( (Cluster + 1) == set->cluster )
			{
				// 将新加入的簇合并到当前的集合
				set->size ++;				// 更新簇集的簇数量
				set->cluster = Cluster;		// 更新簇集的起始簇号
				// 累加空闲簇数量
				cache->count ++;
				return;
			}

			if(set->cluster > Cluster)
			{
				// 终止继续搜索
				// 记录该插入点, 插入点是指将新加簇集单元插入到该簇集合的前面
				set_to_insert = set;
				break;
			}

			set = (CLUSTER_SET *)set->next;
		}
	}

	// 未找到一个可以合并的簇集. 需要为新加的簇创建一个簇集单元来保存簇的信息

	// 检查是否存在可用的空闲簇集单元
	if(queue_empty(&cache->pool))
	{
		// 无可用的空闲簇集单元, 需要替换起始簇号最小或起始簇号最大的簇集单元

		CLUSTER_SET *head_set, *tail_set;

		// 替换头部或尾部的集合 ?
		// 避免替换当前正在分配的簇集, 确保尽量分配大块连续的簇
		head_set = get_head_set (cache);
		tail_set = get_tail_set (cache);

		// 检查新加的空闲簇簇号是否小于第一个簇集的起始簇号. 
		// 若是, 该簇不加入到簇集cache中, 只需重新调整cache的start值, 将该簇从已扫描区域中排除
		if(Cluster < head_set->cluster)
		{
			cache->start = Cluster + 1;
			return;
		}
		// 检查新加的空闲簇簇号是否大于最后一个簇集的结束簇号. 
		// 若是, 该簇不加入到簇集cache中, 只需重新调整cache的end值, 将该簇从已扫描区域中排除
		else if(Cluster > (tail_set->cluster + tail_set->size - 1))
		{
			cache->end = Cluster - 1;
			return;
		}

		// 检查当前正在分配的簇集是否是第一个簇集(具有最小起始簇号), 若是, 强制替换最后一个簇集单元(具有最大起始簇号).   
		if(cache->curr_set == head_set)	
		{
			// 替换最后一个簇集单元
			curr_unit = tail_set;
		}
		// 检查当前正在分配的簇集是最后一个簇集(具有最大起始簇号). 若是, 强制替换第一个簇集
		else if(cache->curr_set == tail_set)
		{
			// 替换第一个簇集
			curr_unit = head_set;
		}
		// 替换空闲簇数量相对较小的簇集
		else if( head_set->size > tail_set->size )
		{
			// 替换最后一个簇集单元
			curr_unit = tail_set;
		}
		else
		{
			// 替换第一个簇集
			curr_unit = head_set;
		}

		if(curr_unit == tail_set)
		{
			// 替换最后一个簇集单元
			XM_printf ("replace tail_set, cache(start=%d, end=%d), Cluster=%d, curr_set(cluster=%d, size=%d), head_set(cluster=%d, size=%d), tail_set(cluster=%d, size=%d)\n", 
					cache->start, cache->end, Cluster, cache->curr_set->cluster, cache->curr_set->size, head_set->cluster, head_set->size, tail_set->cluster, tail_set->size);

			// 将被替换簇集单元的簇从已扫描区域中排除
			cache->count -= tail_set->size;		// 减去空闲簇的数量
#if _DEBUG
			assert ( (tail_set->cluster + tail_set->size - 1) <= cache->end  );
#endif
			// 修改已扫描区域的上限(end), 该上限表示下一次"前进扫描"从cache->end + 1开始, 即当前被替换出去的簇集的起始簇号
			cache->end = tail_set->cluster - 1;		

			// 检查该替换集合是否是插入点
			if(set_to_insert == tail_set)
			{
				XM_printf ("set_to_insert == tail_set\n");
				set_to_insert = NULL;
			}
		}
		else
		{
			// 替换第一个簇集单元
			XM_printf ("replace head_set, cache(start=%d, end=%d), Cluster=%d, curr_set(cluster=%d, size=%d), head_set(cluster=%d, size=%d), tail_set(cluster=%d, size=%d)\n", 
					cache->start, cache->end, Cluster, cache->curr_set->cluster, cache->curr_set->size, head_set->cluster, head_set->size, tail_set->cluster, tail_set->size);

			// 替换第一个簇集单元
			// 将被替换簇集单元的簇从已扫描区域中排除
			cache->count -= head_set->size;		// 减去空闲簇的数量
			// 修改已扫描区域的下限(start), 该下限表示下一次"后退扫描"从cache->start - 1开始, 即当前被替换出去的簇集的最大簇号
#if _DEBUG
			assert ( head_set->cluster >= cache->start );
#endif
			// 将被替换簇集从已扫描区域中排除
			cache->start = head_set->cluster + head_set->size;		
			// 检查该替换集合是否是插入点. 不能是插入点
			assert(set_to_insert != head_set);
		}

		// 从空闲簇集链表中断开
		queue_delete ((queue_s *)curr_unit);
		curr_unit->cluster = 0;
		curr_unit->size = 0;
	}
	else
	{
		// 存在至少一个空闲的"簇集"单元
		// 从未使用簇集池中获取一个簇集单元
		curr_unit = (CLUSTER_SET *)queue_delete_next (&cache->pool);
	}

#if _DEBUG
	assert (curr_unit->cluster == 0);
	assert (curr_unit->size == 0);
#endif

	// 已找到一个空闲的簇集单元curr_unit
	curr_unit->cluster = Cluster;
	curr_unit->size = 1;
	cache->count ++;	// 累加空闲簇数量

	// 按照簇序号将簇集合插入到合适的位置
	if(set_to_insert) 
	{
		// 找到新插入点,将新集合插入到该插入点的前面
		queue_insert ((queue_s *)curr_unit, (queue_s *)set_to_insert);
	}
	else
	{
		queue_insert ((queue_s *)curr_unit, &cache->free[hash_index]);		// 插入到空闲簇cache的队列尾部
	}
}



// 从空闲簇cache中分配一个空闲簇
// 返回值
// 0    空闲簇cache中已没有空闲的簇块, 分配失败
//      可用的空闲簇簇号
U32 FS_FAT_ClusterCacheAllocate (FS_FAT_INFO * pFATInfo)
{
	CLUSTER_CACHE *cache;
	CLUSTER_SET *curr_set;

	U32 Cluster;
	// 计算hash索引
	U32 hash_index;

	cache = (CLUSTER_CACHE *)pFATInfo->ClusterCache;
	if(cache == NULL)
		return 0;
#if _DEBUG
	assert (cache->id == CLUSTER_CACHE_ID);
#endif
	if(cache->id != CLUSTER_CACHE_ID)
		return 0;
	// 检查空闲簇cache是否空
	if(cache->count == 0)
		return 0;

	// 检查当前空闲簇分配使用的簇集合. 如果为空, 指向空闲簇cache的第一个集合(具有最小簇号)
	if(cache->curr_set == NULL)
	{
		// 遍历所有散列表的链表, 直到找到具有最小簇号的簇集
		cache->curr_set = get_head_set (cache);
#if _DEBUG
		assert (cache->curr_set);
#endif
		//printf ("cluster=%d, size=%d\n", cache->curr_set->cluster, cache->curr_set->size);
	}
	curr_set = cache->curr_set;

	// 从当前空闲簇分配的簇集中分配一个簇
	Cluster = curr_set->cluster;	// 集合中的第一个簇
	curr_set->size --;
	// 修正空闲簇的数量
	cache->count --;
	hash_index = (Cluster >> 12) & (HASH_COUNT - 1);

	// 判断簇集的簇资源是否已全部分配
	if(curr_set->size > 0)
	{
		// 簇集中的簇未全部分配
		curr_set->cluster ++;		// 修正下一个待分配簇的簇号
	}
	else
	{
		// 簇集中的簇已全部分配
		// 寻找下一个簇集用于簇分配

		// 记录该簇集所在哈希散列链表的下一个簇集.
		CLUSTER_SET *next_set = curr_set->next;
		// 将该簇集从空闲簇cache中删除
		queue_delete ((queue_s *)curr_set);
		// 将其插入到未使用集合的链表
		curr_set->cluster = 0;
		curr_set->size = 0;
		queue_insert ((queue_s *)curr_set, &cache->pool);
		
#if 1
		// 策略 1
		// 	查找簇号连续的下一个簇对应的簇集是否存在
		// 	若存在,将该簇集用作下一次分配
		next_set = match_cluster_set (cache, Cluster + 1);
		// 20170225 修复bug. 
		// if(next_set == NULL)
		if(next_set)	
		{
			// 存在地址连续的下一个簇集, 使用该簇集作为下一个簇分配的簇集
			cache->curr_set = next_set;
		}
		else
		{
			// 不存在匹配的簇集
			
			// 20170225 zhuoyonghong使用上层文件Cache系统的忙状态来修正簇的分配
			//	当Cache系统繁忙时, 分配一个具有最大块数量的簇集作为下一个簇分配的簇集,
			// 当Cache系统空闲时, 按照正常簇分配策略
			
			// 获取Cache系统忙等待的级别
			// 0 ~ 1 表示系统Cache基本空闲, 流畅流畅
			// 2 ~ 8 级别越大, 表示使用的Cache缓存越多
			// 9     表示内部Cache已满, 暂时无法写入
			unsigned int XMSYS_GetCacheSystemBusyLevel (void);
			
			if(XMSYS_GetCacheSystemBusyLevel() >= 5)
			{
				// Cache系统已经负载很大, 需要优化簇写入. 分配连续的簇块可以减少簇表的访问次数
				unsigned int require_cluster_count = 0x200000 / pFATInfo->BytesPerCluster;
				next_set = get_max_count_set (cache, require_cluster_count);
				if(next_set)
				{
					cache->curr_set = next_set;
					goto cache_alloc_exit;
				}
			}
			
			// 查找最近释放的簇对应的簇集(最近释放的簇意味着该簇对应的FAT扇区位于cache中,可以减少读磁盘的访问开销)
			next_set = match_cluster_set (cache, cache->last_free_cluster);
			if(next_set && next_set->size >= 2)		// 簇集的数量不小于2
			{
				// 将簇号对应的簇集用作下一次分配, 并将该最近释放的簇号标记为无效
				cache->curr_set = next_set;
				cache->last_free_cluster = 0;
			}
			else
			{
				// 查找比查找值大且最接近的簇集
				next_set = find_cluster_set (cache, Cluster + 1);
				// 	若不存在, 则找到具有最小簇号的簇集用作下一次分配
				if(next_set == NULL)
					next_set = get_head_set (cache);
				cache->curr_set = next_set;
			}
		}
		
#else
		// 判断下一个簇集是否存在
		if(next_set == (CLUSTER_SET *)(&cache->free[hash_index]))
		{
			// 下一个簇集不存在(指向链表入口)
			// curr_set是链表中的最后一个簇集
			// 检查链表是否为空
			if(queue_empty(&cache->free[hash_index]))
			{
				// 下次簇分配时将遍历整个簇集, 将具有最小簇号的簇集作为下一个簇分配的当前簇集合
				cache->curr_set = NULL;
			}
			else
			{
				// 将链表队首的第一个簇集作为下一个簇分配的当前簇集合
				cache->curr_set = (CLUSTER_SET *)queue_next (&cache->free[hash_index]);
			}
		}
		else
		{
			// 存在下一个簇集, 其簇具有较大的可能与原先分配的簇邻接.
			// 将其作为下一个簇分配的当前簇集合
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

// 返回空闲簇cache的簇数量
U32 FS_FAT_ClusterCacheCount (FS_FAT_INFO * pFATInfo)
{
	CLUSTER_CACHE *cache;
	cache = (CLUSTER_CACHE *)pFATInfo->ClusterCache;
	if(cache == NULL)
		return 0;
#if _DEBUG
	assert (cache->id == CLUSTER_CACHE_ID);
#endif
	// 检查空闲簇cache是否空
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

// 扫描空闲的簇块并将其加入到空闲簇cache
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

	// 检查未使用的簇集链表是否为空.
	// 若为空, 此时没有可以分配的簇集, 不用安排扫描
	if(queue_empty(&cache->pool))
		return;

	// 至少存在一个空闲的簇集

	// 检查start/end是否已覆盖所有的FAT区.
	// 若已覆盖, 无需扫描
	if(cache->start == FAT_FIRST_CLUSTER && cache->end == (pFATInfo->NumClusters + FAT_FIRST_CLUSTER - 1))
	{
		// 若已覆盖, 无需扫描
		return;
	}

	if(cache->start == 0 && cache->end == 0)
	{
		// 开始扫描
		cache->start = 2;
		cache->end = 1;
	}

	FATType = pFATInfo->FATType;
	// 没有完全覆盖,且存在未使用的簇集单元, 可以安排空闲簇扫描
	// 1) 判断是否可以向后扫描 (增加上限). 若已到达簇号最大值, 转到2)
	if(cache->end < (pFATInfo->NumClusters + FAT_FIRST_CLUSTER - 1))
	{
		scan_start_cluster = cache->end + 1;
		scan_last_cluster = pFATInfo->NumClusters + FAT_FIRST_CLUSTER - 1;
		LastSectorNo = 0;
		SectorCount = 0;
		while(scan_start_cluster <= scan_last_cluster)
		{
			cache->end ++;	// 调整已扫描区域边界的上限
			if(cache->end == scan_last_cluster)
			{
				XM_printf ("scan has reached up-limit %d\n", cache->end);
				if(cache->start == FAT_FIRST_CLUSTER)
				{
					XM_printf ("scan has full coveraged\n");
				}
			}
			// 判断该簇是否未使用
			if (FS_FAT_ReadFATEntry(pVolume, pSB, scan_start_cluster) == 0)
			{
				// 未使用的簇(空白簇)
				// 将其加入到空闲簇集cache
				FS_FAT_ClusterCacheFree (pFATInfo, scan_start_cluster);
			}
				
			// 累加扫描过程中读取的不同扇区数量.
			// 超出一定数量后,退出扫描循环, 避免扫描导致其他任务等待时间过长
			Off = _ClusterId2FATOff(FATType, scan_start_cluster);
			SectorNo = pFATInfo->RsvdSecCnt + (Off >> pFATInfo->ldBytesPerSector);
			if(LastSectorNo != SectorNo)
			{
				LastSectorNo = SectorNo;
				SectorCount ++;
				if(SectorCount >= 4)		// 4个扇区的读取时间+最多4个脏扇区的回写时间
				{
					break;
				}
			}
			scan_start_cluster ++;
		}
	}
	// 2) 判断是否可以向前扫描 (减小下限).
	else if(cache->start != FAT_FIRST_CLUSTER)
	{
		scan_start_cluster = cache->start - 1;
		LastSectorNo = 0;
		SectorCount = 0;
		while(scan_start_cluster >= FAT_FIRST_CLUSTER)
		{
			cache->start --;	// 调整已扫描区域边界的下限
			if(cache->start == FAT_FIRST_CLUSTER)
			{
				XM_printf ("scan has reached down-limit %d\n", cache->start);
				if(cache->end == (pFATInfo->NumClusters + FAT_FIRST_CLUSTER - 1))
				{
					XM_printf ("scan has full coveraged\n");
				}
			}
			// 判断该簇是否未使用
			if (FS_FAT_ReadFATEntry(pVolume, pSB, scan_start_cluster) == 0)
			{
				// 未使用的簇
				// 将其加入到空闲簇集cache
				FS_FAT_ClusterCacheFree (pFATInfo, scan_start_cluster);
			}
				
			// 累加扫描过程中读取的不同扇区数量.
			// 超出一定数量后,退出扫描循环, 避免扫描导致其他任务等待时间过程
			Off = _ClusterId2FATOff(FATType, scan_start_cluster);
			SectorNo = pFATInfo->RsvdSecCnt + (Off >> pFATInfo->ldBytesPerSector);
			if(LastSectorNo != SectorNo)
			{
				LastSectorNo = SectorNo;
				SectorCount ++;
				if(SectorCount >= 4)		// 4个扇区的读取时间+最多4个脏扇区的回写时间
				{
					break;
				}
			}

			scan_start_cluster --;
		}
	}

}

// 检查簇集cache是否已覆盖所有的FAT区域
//   未全部覆盖时, pLowLimit返回已扫描区域的下限, pHighLimit返回已扫描区域的上限
// 返回值
// 1    已全部覆盖
// 0    未全部覆盖
// -1	  错误
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