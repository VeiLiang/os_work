//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_videoitem.c
//	  视频列表接口
//
//	Revision history
//
//		2011.12.16	ZhuoYongHong Initial version
//		2018.04.30	ZhuoYongHong 改为使用长文件名命名视频项文件
//
//****************************************************************************
#include <xm_proj_define.h>
#include "rtos.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <xm_user.h>
#include <xm_base.h>
#include <xm_file.h>
#include <xm_queue.h>
#include <xm_heap_malloc.h>
#include <xm_semaphore.h>
#include <xm_heap_malloc.h>
#include <xm_videoitem.h>
#include <xm_assert.h>
#include "common_rbtr.h"
#include <xm_printf.h>
#include <xm_dev.h>
#include <xm_key.h>
#include <xm_event.h>
#include "FS.h"

#include "xm_message_socket.h"
#include "xm_h264_cache_file.h"
#include "xm_h264_file.h"
#include "xm_recycle.h"
#include "xm_power.h"
#include "xm_core.h"

#ifdef _XM_VOICE_ASSISTANT_ENABLE_
#include <xm_voice_prompts.h>
#endif

#include <common_string.h>

#ifdef _WINDOWS
#pragma warning(disable:4127) // VC下不显示4127号警告信息
#pragma warning(disable:4115) // VC下不显示4127号警告信息
#endif

#define	VIDEO_ITEM_FOLDER_NAMING_PROFILE_2	1

// 路径管理
//
// 方案1 (VIDEO_ITEM_FOLDER_NAMING_PROFILE_1)
//
//    VIDEO_F		
//    VIDEO_B
//
//
// 方案2 (VIDEO_ITEM_FOLDER_NAMING_PROFILE_2)
//
//    VIDEO_F
//			NORMAL
//			MANUAL
//			URGENT
//    PHOTO_F
//			NORMAL
//			MANUAL
//			URGENT
//    VIDEO_B
//			NORMAL
//			MANUAL
//			URGENT
//    PHOTO_B
//			NORMAL
//			MANUAL
//			URGENT


//#define	XM_printf(...)

extern XMBOOL XMSYS_H264CodecIsBusy (void);
extern void XMSYS_DoRecycle (void);
// 获取Cache系统忙等待的级别
// 0 ~ 1 表示系统Cache基本空闲, 流畅流畅
// 2 ~ 8 级别越大, 表示使用的Cache缓存越多
// 9     表示内部Cache已满, 暂时无法写入
unsigned int XMSYS_GetCacheSystemBusyLevel (void);

// TEMPNNNN_00P.AVI
//	20180318_121056_00000002_00U.AVI		 
static int videoitem_extract_info (const char *lpVideoFileName, 
											  char *lpVideoItemName, int cbVideoItemName,
											  unsigned int *file_type,
											  XMSYSTEMTIME *create_time, unsigned int *volume_index, unsigned int *channel_index, unsigned int *class_index,
											  unsigned int *serial_number);

static int is_valid_time (XMSYSTEMTIME *lpCreateTime);

#define	VIM_ID		0x52425453	// "RBTS"

typedef struct {
	void *prev;
	void *next;
	unsigned int id;
	XMVIDEOITEM video_item;
} videoitem_s;

//#define	MAX_VIDEO_ITEM_COUNT				1024	
#define	MAX_VIDEO_ITEM_COUNT				8192			// 满足视频2分钟分段时连续11天记录 4096*2/60/24=11.4天

#define	MAX_VIDEO_CHANNEL_PATH_NAME		32			// 视频通道路径名最大字节长度(包含结束\0)

#define	MAX_VIDEO_CHANNEL_VOLUME_NAME		16

#define	MAX_VIDEO_CHANNEL_COUNT				8			// 视频通道的最大个数

#define	MAX_VIDEO_TEMP_FILE_COUNT			16

static const char videoitem_class_id[XM_VIDEOITEM_CLASS_COUNT] = {'N', 'N', 'U'};
static const char *videoitem_file_type_id[XM_FILE_TYPE_COUNT] = {"AVI", "JPG"};

__no_init static videoitem_s   rbt_pool[MAX_VIDEO_ITEM_COUNT];
static queue_s rbt_free;		// 空闲的单元

#define	SERIAL_NUMBER_TYPE_SHARE		0		// 每种视频项类型共享相同的序号, 如照片与视频使用同一个序号计数
#define	SERIAL_NUMBER_TYPE_ALONE		1		// 每种视频项类型具有独立的序号
static unsigned int serial_number_type = SERIAL_NUMBER_TYPE_SHARE;

// 序号管理
static unsigned int video_item_serial_number[XM_VIDEOITEM_VOLUME_COUNT][XM_FILE_TYPE_COUNT][XM_VIDEO_CHANNEL_COUNT];	
static unsigned int video_item_ordering = XM_VIDEOITEM_ORDERING_SERIAL_NUMBER;		// 排序方法
static unsigned int channel_reverse_ordering = 1;		// 通道号逆序排序 1 逆序(通道0排在最后) 0 顺序(通道0排在前面)

void   videoitem_int (void)
{
	int i;
	queue_initialize (&rbt_free);
	for (i = 0; i < MAX_VIDEO_ITEM_COUNT; i ++)
	{
		rbt_pool[i].id = VIM_ID;
		queue_insert ((queue_s *)&rbt_pool[i], &rbt_free);
	}
}

// 视频项的分配是随着时间断续分配的, 使用全局堆分配时会产生碎片, 从而导致其他大内存分配失败. 
// 使用静态的结构来分配视频项,  避免上述碎片.
static void * videoitem_malloc (void)
{
	videoitem_s *item = NULL;
	XM_lock ();
	if(!queue_empty (&rbt_free))
	{
		item = (videoitem_s *)queue_delete_next (&rbt_free);
	}
	XM_unlock();
	
	if(item)
	{
		if(item->id != VIM_ID)
		{
			XM_printf ("os fatal error\n");
			assert (0);		
		}
		return &item->video_item;
	}
	else
		return NULL;
}

 
static void videoitem_free (void *p)
{
	videoitem_s *item;
	if(p)
	{
		item = (videoitem_s *)(((char *)p) - ((unsigned int)&(((videoitem_s *)0)->video_item)));
		if(item->id != VIM_ID)
		{
			XM_printf ("os fatal error\n");
			assert (0);			
		}
		XM_lock ();
		queue_insert ( (queue_s *)item, &rbt_free);
		XM_unlock();
	}
}


// 使用文件名排序
static int compare(void *a, void *b)
{
	int ret;
	
	if(channel_reverse_ordering)
	{
		char ta[VIDEOITEM_MAX_FULL_NAME];
		char tb[VIDEOITEM_MAX_FULL_NAME];
		memcpy (ta, a, VIDEOITEM_MAX_FULL_NAME);
		memcpy (tb, b, VIDEOITEM_MAX_FULL_NAME);
		ta[VIDEOITEM_LFN_SIZE - 3] = '0' + XM_VIDEO_CHANNEL_COUNT - 1 - (ta[VIDEOITEM_LFN_SIZE - 3] - '0');
		tb[VIDEOITEM_LFN_SIZE - 3] = '0' + XM_VIDEO_CHANNEL_COUNT - 1 - (tb[VIDEOITEM_LFN_SIZE - 3] - '0');
		if(video_item_ordering == XM_VIDEOITEM_ORDERING_NAME)
			ret = memcmp ((char *)ta, (char *)tb, VIDEOITEM_MAX_FULL_NAME);
		else
			ret = memcmp ((char *)ta + 16, (char *)tb + 16, VIDEOITEM_MAX_FULL_NAME - 16);	// 从序号开始比较
	}
	else
	{
		if(video_item_ordering == XM_VIDEOITEM_ORDERING_NAME)
			ret = memcmp ((char *)a, (char *)b, VIDEOITEM_MAX_FULL_NAME);
		else
			ret = memcmp ((char *)a + 16, (char *)b + 16, VIDEOITEM_MAX_FULL_NAME - 16);	// 从序号开始比较
	}
	
	ret = -ret;
	return ret;
}


static OS_RSEMA videoItemMutexSema;			// 视频项服务访问互斥保护
static OS_RSEMA videoItemMonitorSema;			// 视频项监控及回收服务访问互斥保护

static OS_EVENT codecStartEvent;			// 录像启动同步事件
VOID  XM_VideoItemCreateResourceMutexSemaphore (VOID)
{
	OS_CREATERSEMA(&videoItemMutexSema); /* Creates resource semaphore */
	OS_CREATERSEMA(&videoItemMonitorSema);
	OS_EVENT_Create (&codecStartEvent);
}

VOID XM_VideoItemDeleteResourceMutexSemaphore (VOID)
{
	OS_DeleteRSema (&videoItemMutexSema);
	OS_DeleteRSema (&videoItemMonitorSema);
	OS_EVENT_Delete (&codecStartEvent);
}

VOID XM_VideoItemLock (VOID)
{
	OS_Use(&videoItemMutexSema);
}

VOID XM_VideoItemUnlock (VOID)
{
	OS_Unuse(&videoItemMutexSema);
}

// 视频项服务等待"编码器启动事件"
// 返回值 
//		0	 编码器启动事件在指定时间内被设置
//    -1	 编码器启动事件在指定时间内没有被设置
int XM_CodecStartEventWait (unsigned int milliseconds_to_timeout)		// 等待编码器启动事件
{
	int ret = OS_EVENT_WaitTimed (&codecStartEvent, milliseconds_to_timeout);
	if(ret == 0)
		return 0;
	else
		return -1;
}

// 清除"编码器启动事件"
void XM_CodecStartEventReset (void)
{
	OS_EVENT_Reset (&codecStartEvent);
}

// "外部编码器任务"调用该函数通知"视频项服务"编码器已启动编码过程
// "视频项服务"等待该事件，当收到该事件后，"视频项服务"启动后续耗时的磁盘信息获取，视频扫描过程，
//	这些过程会导致文件系统服务出现阻塞，导致"外部编码器任务"的文件创建过程阻塞。
void XM_CodecStartEventSet (void)			// 设置编码器启动事件
{
	OS_EVENT_Set (&codecStartEvent);
}


static RbtHandle h;


// 路径管理
static unsigned int		video_path_count;			// 视频通道个数(由外部应用定义，不可修改)
static char 	video_path_name[XM_VIDEOITEM_VOLUME_COUNT][XM_VIDEO_CHANNEL_COUNT][XM_FILE_TYPE_COUNT][XM_VIDEOITEM_CLASS_COUNT][MAX_VIDEO_CHANNEL_PATH_NAME];	// 路径名，不包含结束'\'字符
static char 	video_path_valid[XM_VIDEOITEM_VOLUME_COUNT][XM_VIDEO_CHANNEL_COUNT][XM_FILE_TYPE_COUNT][XM_VIDEOITEM_CLASS_COUNT];		// 有效标志， 1表示该目录是有效的 0表示该目录是冗余的

// 卷管理
static DWORD	fat_cluster_size[XM_VIDEOITEM_VOLUME_COUNT];	// 文件系统族字节大小
static int		video_volume_count;		// 视频使用的磁盘卷个数(由外部应用定义，不可修改)
													//		为了优化写入性能或安全记录需求，会使用2个或多个SD卡作为存储空间
static char		video_volume_valid[XM_VIDEOITEM_VOLUME_COUNT];	// 标记卷是否有效, 1 卷有效 0 卷无效

//static int		video_temp_index;

static int		basic_service_opened = 0;		// 基础服务允许标志。 
															//		1 表示基础服务已开启，视频服务可以开始录像
															//		0 表示基础服务已关闭，视频服务无法访问磁盘

// 每次基础服务成功打开，累加该值。用于标记当前的服务ID。
//		重新打开某个卷的VIDEO ITEM数据库后，累加与该卷对应的basic_service_id值
//		应用使用该值判断是否存在视频项库存在关闭/打开的过程(对应SD卡的拔出/插入操作)
static unsigned int		basic_service_id;			
																						
static volatile int		delay_cutoff_flag;				// 延迟文件删除操作

static volatile int		delay_recycle_clean;			// 是否有延期执行的回收站清除操作 1 存在延期的回收站清除操作 0 没有

static volatile int		block_file_delete;			// 阻塞文件删除操作

static volatile int videoitem_service_opened;	// 表示视频项所有服务开启的标志
													//	1 表示视频项服务已开启
static int		videoitem_debug = 1;

static int		disk_free_space_busy;		// 标记磁盘剩余空间是否不能满足实时记录, 1 磁盘剩余空间不足,无法满足实时记录 0 可以满足实时记录

static int		automatic_safe_save = 0;	// 当存在多个卷时，外部SD卡写入失败时，系统自动切换到内置的eMMC卡，保证录像安全写入

// ***** 视频项配额管理（分为2部分, 磁盘空间及项目数量）*********
//		(根据磁盘卷, 文件类型及录制分类分割)
// 1) 视频项文件数量配额管理，非0值表示存在配额限制，表示允许的最大项目数量，若超出配额限制，报警或自动清除
static unsigned int item_count_quota[XM_VIDEOITEM_VOLUME_COUNT][XM_FILE_TYPE_COUNT][XM_VIDEOITEM_CLASS_COUNT];
// 2) 视频项磁盘空间配额管理(按录制分类定义)，非0值表示存在该配额限制，表示允许的最大字节容量，若超出配额限制，报警或自动清除
static XMINT64	disk_space_quota_level_class[XM_VIDEOITEM_VOLUME_COUNT][XM_FILE_TYPE_COUNT][XM_VIDEOITEM_CLASS_COUNT];
//    视频项磁盘空间配额管理(按文件类型定义)，非0值表示存在该配额限制，表示允许的最大字节容量，若超出配额限制，报警或自动清除
static XMINT64	disk_space_quota_level_type[XM_VIDEOITEM_VOLUME_COUNT][XM_FILE_TYPE_COUNT];
//    视频项磁盘空间配额管理(按磁盘卷定义)，  非0值表示存在该配额限制，表示允许的最大字节容量，若超出配额限制，报警或自动清除
static XMINT64	disk_space_quota_level_volume[XM_VIDEOITEM_VOLUME_COUNT];

static XMINT64 minimum_space_limit;

// 	可回收空间最低配额定义, 小于该值将报警
static XMINT64 recycle_space_limit;

// ***** 视频项记录管理
// 1) 视频项文件数量记录
static volatile int		 video_item_count;		// 视频项纪录个数
static int		 class_item_count[XM_VIDEOITEM_VOLUME_COUNT][XM_FILE_TYPE_COUNT][XM_VIDEOITEM_CLASS_COUNT];		// 视频项分类统计纪录个数

// 2) 视频项磁盘空间记录
static XMINT64	video_item_recycle_space;							// 可循环项总字节大小
static XMINT64	video_item_locked_space;							// 非循环项总字节大小(锁定空间)
// 空间占用信息统计
static XMINT64 video_item_space[XM_VIDEOITEM_VOLUME_COUNT][XM_FILE_TYPE_COUNT][XM_VIDEOITEM_CLASS_COUNT];		// 字节统计信息


// 视频项磁盘空间记录
static void videoitem_inc_space (XMVIDEOITEM *lpVideoItem, unsigned int stream_class)
{
	XMINT64 temp = video_item_space[lpVideoItem->volume_index][lpVideoItem->file_type][stream_class];
	temp = temp + lpVideoItem->video_size;
	video_item_space[lpVideoItem->volume_index][lpVideoItem->file_type][stream_class] = temp;
}

static void videoitem_dec_space (XMVIDEOITEM *lpVideoItem, unsigned int stream_class)
{
	XMINT64 temp = video_item_space[lpVideoItem->volume_index][lpVideoItem->file_type][stream_class];
	temp = temp - lpVideoItem->video_size;
	if(temp < 0)
	{
		XM_printf ("videoitem_dec_space fatal error\n");
		temp = 0;
	}
	video_item_space[lpVideoItem->volume_index][lpVideoItem->file_type][stream_class] = temp;
}

// 视频项文件数量记录
static void videoitem_dec_count (XMVIDEOITEM *lpVideoItem, unsigned int stream_class)
{
	class_item_count[lpVideoItem->volume_index][lpVideoItem->file_type][stream_class] --;
	if(class_item_count[lpVideoItem->volume_index][lpVideoItem->file_type][stream_class] < 0)
	{
		assert (0);
	}
}

static void videoitem_inc_count (XMVIDEOITEM *lpVideoItem, unsigned int stream_class)
{
	class_item_count[lpVideoItem->volume_index][lpVideoItem->file_type][stream_class] ++;
}


// 根据SD卡编号获取定义的卷名称
//		VolumeIndex		存储卡编号(从0开始)
// 返回值
//		NULL		失败
//		非空值	卷字符串名
const char *XM_VideoItemGetVolumeName (unsigned int VolumeIndex)
{
	if(VolumeIndex == 0)
		return "mmc:0:";
	else if(VolumeIndex == 1)
		return "mmc:1:";
	else
		return "";	// 返回空串
}

// 根据通道号获取定义的视频项通道名称
//		VideoChannel	视频通道编号(从0开始)
//		FileType			文件类型(视频或照片)
// 返回值
//		NULL		失败
//		非空值	通道字符串名
const char *XM_VideoItemGetChannelName (unsigned int VideoChannel, unsigned int FileType)
{
#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2
	if(FileType >= XM_FILE_TYPE_COUNT)
		return NULL;
	
	if(FileType == XM_FILE_TYPE_VIDEO)
	{
		if(VideoChannel == 0)
			return "VIDEO_CH1";
		else if(VideoChannel == 1)
			return "VIDEO_CH2";
		else if(VideoChannel == 2)
			return "VIDEO_L";
		else if(VideoChannel == 3)
			return "VIDEO_R";
		else
			return NULL;	
	}
	else if(FileType == XM_FILE_TYPE_PHOTO)
	{
		if(VideoChannel == 0)
			return "PHOTO_CH1";
		else 	if(VideoChannel == 1)
			return "PHOTO_CH2";
		else 	if(VideoChannel == 2)
			return "PHOTO_L";
		else 	if(VideoChannel == 3)
			return "PHOTO_R";
		else
			return NULL;			
	}
	else
		return NULL;
#else
	if(VideoChannel == 0)
		return "VIDEO_F";
	else 	if(VideoChannel == 1)
		return "VIDEO_B";
	else 	if(VideoChannel == 2)
		return "VIDEO_L";
	else 	if(VideoChannel == 3)
		return "VIDEO_R";
	else
		return NULL;
#endif
}



// 根据文件类型获取定义的文件类型名
const char *XM_VideoItemGetFileTypeName (unsigned int FileType)
{
	if(FileType >= XM_FILE_TYPE_COUNT)
		return NULL;	// 返回空串
	if(FileType == XM_FILE_TYPE_VIDEO)
		return "VIDEO";
	else if(FileType == XM_FILE_TYPE_PHOTO)
		return "PHOTO";
	else
		return "";	// 返回空串
}

// 根据录制类型获取定义的录制类型名
const char *XM_VideoItemGetStreamClassName (unsigned int StreamClass)
{
	if(StreamClass >= XM_VIDEOITEM_CLASS_COUNT)
		return NULL;	// 返回空串
	else if(StreamClass == XM_VIDEOITEM_CLASS_NORMAL)
		return "NORMAL";
	else if(StreamClass == XM_VIDEOITEM_CLASS_MANUAL)
		return "MANUAL";
	else if(StreamClass == XM_VIDEOITEM_CLASS_URGENT)
		return "URGENT";
	else
		return "";	// 返回空串
}

// 获取当前视频项数据库的服务ID，每次基础服务成功打开，累加该服务ID。
// 20180626 用于一致性检查，避免快速卡拔插导致的异常。
//		某些场景下，recycle线程(后台运行，优先级低)的一个文件删除过程中会嵌入"卡拔出、卡重新插入"的过程，此时出现系统挂机，
//		加入一致性检查，避免该挂机现象
unsigned int XM_VideoItemGetServiceID (void)
{
	return basic_service_id;
}

// 通过全路径文件名查找视频项句柄及通道号
HANDLE XM_VideoItemGetVideoItemHandle (const char *lpVideoFile, int* pVideoChannel)
{
	char path_name[VIDEOITEM_MAX_FULL_PATH_NAME];
	char *ch;
	NodeType * i = NULL;
	unsigned int channel;
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return NULL;

	XM_VideoItemLock ();
	
	do
	{
		if(lpVideoFile == NULL || *lpVideoFile == 0)
			break;
		if(h == NULL)
			break;
		
		ch = strrchr (lpVideoFile, '\\');
		if(ch)
		{
			ch ++;
		}
		else
		{
			ch = (char *)lpVideoFile;
		}
		if(*ch == '\0')
			break;
		int len = strlen(ch);
		if(len != VIDEOITEM_MAX_FULL_NAME)
			break;
		i = (NodeType *)rbtFind(h, ch);
		if(i && pVideoChannel)
		{
			XMVIDEOITEM *video_item;
			// found an existing node
			video_item = (XMVIDEOITEM *)i->val;
			*pVideoChannel = video_item->video_channel;
		}
		
	} while (0);
	
	XM_VideoItemUnlock ();

	return i;
}

// 通过全路径的临时视频项文件名查找对应的视频通道号
// // TEMPNNNN_00P.AVI
XMBOOL XM_VideoItemGetVideoChannelFromTempVideoItemName (const char *lpVideoFile, int* pVideoChannel)
{
	char *ch;
	int ret = 0;
	unsigned int channel;
		
	do
	{
		if(lpVideoFile == NULL || *lpVideoFile == 0)
			break;
	
		// 提取路径名
		ch = strrchr (lpVideoFile, '\\');
		if(ch)
		{
			ch ++;
		}
		else
		{
			ch = (char *)lpVideoFile;
		}
		int len = strlen(ch);
		if(len != TEMPITEM_MAX_FILE_NAME)
			break;
		ch = ch + 9;
		if(*ch >= '0' && *ch < ('0' +  video_path_count))
			channel = *ch - '0';
		else
			break;
	
		if(pVideoChannel)
			*pVideoChannel = channel;
		ret = 1;
	} while (0);
	
	return (XMBOOL)ret;
}

XMBOOL XM_VideoItemIsTempVideoItemFile (const char *lpVideoFileName)
{
	// 检查是否是临时视频项文件
	if(!strstr (lpVideoFileName, "TEMP"))
		return 0;
	return 1;	
}

// 获取指定卷的FAT簇大小(字节单位)
static DWORD get_fat_cluster_size_from_volume (unsigned volume_index)
{
	if(volume_index >= video_volume_count)
		return 0;

	return fat_cluster_size[volume_index];
}



// 测试SD卡 Class10 Toshiba
// 1) 使用XMSYS_FAT_FAST_FILE_REMOVE的算法(优化簇删除，按照相同扇区原则将簇释放)，测试10分钟(文件写入及文件删除并发执行)
//	显示脏块的统计信息 (参考get_dirty_cache_block_count函数)
//		1  826%   6348
//    2  123%    945
//    3   10%    83
// 2) 未使用XMSYS_FAT_FAST_FILE_REMOVE，测试10分钟(文件写入及文件删除并发执行)
//	显示脏块的统计信息 (参考get_dirty_cache_block_count函数)
//    1  614%  4405
//    2  278%  1999
//    3   22%   159
// 3) 可以看出，任何时刻，Cache中的脏块个数为1,2,3的比例从 91.4% 增加到 95.9%，即任何时刻掉电，完整保存的信息增加了。

// 以下算法明显改善文件删除导致的H264编码阻塞现象。
// 返回值定义
// 1 成功
// 0 失败
// 10分钟视频录像的删除时间将延长到最大20秒
static int _cutoff_file (const char *filename)
{
#ifdef WIN32
	remove (filename);
	return 1;
#else
	extern int FS_CutOffFile (FS_FILE * pFile);

	int ret = 0;
	FS_FILE *fp;
	unsigned int service_id;		// 保存服务ID
	
	// ACC下电时不再允许文件删除操作
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// bad acc
		return 0;
	}
	
	XM_VideoItemLock ();
	fp = FS_FOpen(filename, "r+");
	service_id = XM_VideoItemGetServiceID();
	XM_VideoItemUnlock ();
	if(fp == NULL)
		return 0;
	while(fp)
	{
		int fs_ret;
		if(block_file_delete)
		{
			XM_Sleep (50);
			continue;
		}
		// 20170929
		// 检查是否存在延缓文件删除操作的请求. 若存在,清除该请求并等待0.1秒. 
		while(delay_cutoff_flag)
		{
			delay_cutoff_flag = 0;
			//XM_Sleep (100);
			XM_Sleep (200);
		}
		// 过大超时参数设置导致文件删除很慢
		//XMSYS_file_system_waiting_for_cache_system_nonbusy_state (3000);	
		XMSYS_file_system_waiting_for_cache_system_nonbusy_state (1000);
		XM_VideoItemLock ();		// 阻止磁盘unmount
		if(!XM_VideoItemIsBasicServiceOpened())
		{
			XM_VideoItemUnlock ();
			XM_printf ("FS_CutOffFile end, service closed\n");
			break;
		}
		// 检查服务ID是否相同。若不同，终止删除
		if(XM_VideoItemGetServiceID() != service_id)
		{
			// 视频项数据库已变更，同理SD卡也已变更
			XM_VideoItemUnlock ();
			XM_printf ("FS_CutOffFile end, service changed\n");
			break;			
		}
		if(xm_power_check_acc_safe_or_not() == 0)
		{
			XM_VideoItemUnlock ();
			break;
		}
		fs_ret = FS_CutOffFile (fp);
		if(fs_ret < 0)
		{
			// 异常
			XM_VideoItemUnlock ();
			break;			
		}
		
		// 检查操作是否已完成, 1表示簇已回收完毕
		if(fs_ret)
		{
			ret = 1;		// 标记簇删除已成功
			XM_VideoItemUnlock ();
			break;
		}
		XM_VideoItemUnlock ();
		// 给予其他低优先线程及Cache线程更多机会及时间获取视频项数据库控制权，避免阻塞
		XM_Sleep (2);
	}
	
	// 检查外部ACC是否完全
	//if(xm_power_check_acc_safe_or_not() == 0)
	//{
		// 不执行目录项删除操作, 防止ACC掉电造成的目录项损坏.
		// 此处不执行删除操作不会造成簇空间丢失
		//return 0;
	//}
	
	
	// 文件系统安全访问锁定
	if(XMSYS_file_system_block_write_access_lock () == 0)
	{
		// 检查外部ACC是否完全
		if(xm_power_check_acc_safe_or_not() == 1)
		{
			// 修改并删除目录项
			FS_FClose (fp);
			ret = XM_RemoveFile (filename);
			if(ret == 1)
			{
				// 更新目录项
				FS_CACHE_Clean("");	
			}
		}
		else
		{
			ret = 0;
		}
		
		// 文件系统安全访问解锁	
		// XMSYS_file_system_block_write_access_unlock ();	

	}
	else
	{
		// 非安全访问
		ret = 0;
	}
	
	// 文件系统安全访问解锁	
	XMSYS_file_system_block_write_access_unlock ();	
	
	// 此处无需安排Cache回写. 因为仅剩下目录项更新
	//FS_CACHE_Clean("");		
	return ret;	
#endif
}

// 返回值
// 1 成功
// 0 失败
static int cutoff_file (const char *filename)
{
	// 首先将文件名改名为00000000.TMP, 然后再执行删除操作.
	// 避免删除过程中因为"拔卡或者掉电"导致的原删除文件部分内容存在, 导致照片/视频浏览器显示异常等问题.
	// 20170319 将文件名改名为00000000.TMP存在风险.
	//		当2个文件删除操作同时进行时, 导致 _cutoff_file操作同时进行, 并阻塞整个系统
	//			1) XMSYS_RecycleTask 删除循环项
	//			2) H264FileTask 删除小于1秒的视频
	//		改为将文件后缀名更名为TMP进行删除
	char del_filename[80];
	char tempname[VIDEOITEM_MAX_FULL_PATH_NAME];
	char *ch;
	void *fp;
	
	if(!xm_power_check_acc_safe_or_not())
		return 0;
	
	memset (tempname, 0, sizeof(tempname));
	strcpy (del_filename, filename);
	// ch = strrchr (del_filename, '\\');
	ch = strrchr (del_filename, '.');
	if(ch)
		*ch = '\0';
	else
	{
		// 非法文件名, 无扩展名
		return 0;
	}
	// strcat (del_filename, "\\00000000.TMP");
	strcat (del_filename, ".TMP");
	ch = strrchr (del_filename, '\\');
	if(ch)
		strcpy (tempname, ch + 1);
	else
	{
		// 非法文件名, 无路径
		return 0;		
	}
	
	// 检查同名的临时文件是否存在. 若存在,将其删除
	// 文件系统安全访问锁定
	if(XMSYS_file_system_block_write_access_lock() == 0)
	{
		fp = XM_fopen (del_filename, "rb");
		if(fp)
		{
			XM_fclose (fp);
			XMSYS_file_system_block_write_access_unlock();
			// 删除之前的同名文件
			if(_cutoff_file (del_filename) == 0)
			{
				// 删除失败
				// 直接删除要删除的文件
				return _cutoff_file (filename);
			}
		}
		else
		{
			XMSYS_file_system_block_write_access_unlock();
		}
	}
	else
	{
		XMSYS_file_system_block_write_access_unlock();
	}
	
	// 将删除文件改名为临时文件
	//if(XM_rename(filename, "00000000.TMP") < 0)
	if(XM_rename(filename, tempname) < 0)
	{
		// 改名失败
		return _cutoff_file (filename);
	}
	else
	{
		// 改名成功
		return _cutoff_file (del_filename);
	}
}

int XMSYS_remove_file (const char *filename)
{
	return cutoff_file (filename);
}

static XMBOOL remove_video_files (VIDEOFILETOREMOVE *lpVideoFileToRemove, int bPostFileListUpdateMessage)
{
	int i;
	XMBOOL ret = 1;
	if(lpVideoFileToRemove == NULL)
		return 0;
	
	for (i = 0; i < lpVideoFileToRemove->count; i ++)
	{
		unsigned int ticket = XM_GetTickCount();
		XMBOOL do_remove;
		XM_printf ("remove_file %s\n", lpVideoFileToRemove->file_name[i]);
		// 避免只读文件无法删除
		XM_SetFileAttributes(lpVideoFileToRemove->file_name[i], XM_FILE_ATTRIBUTE_ARCHIVE);
		// XMBOOL do_remove = XM_RemoveFile (lpVideoFileToRemove->file_name[i]);
		do_remove = (XMBOOL)cutoff_file (lpVideoFileToRemove->file_name[i]);
		ticket = XM_GetTickCount() - ticket;
		if(do_remove)
		{
			unsigned int channel;
			unsigned int type;
			unsigned char file_name[VIDEOITEM_MAX_FULL_PATH_NAME+1];
			
			if(videoitem_extract_info (lpVideoFileToRemove->file_name[i], (char *)file_name, sizeof(file_name), &type, NULL, NULL, &channel, NULL, NULL) < 0)
			{
				if(videoitem_debug)
					XM_printf ("File %s, remove failed\n", lpVideoFileToRemove->file_name[i]);
				continue;
			}

			// 发送文件列表更新消息(文件删除)
			// 20170702 更新列表操作已关闭
			// 此操作因为需求读取每个视频文件来获取其视频录像时间而导致卡访问效率下降
			if(bPostFileListUpdateMessage)
				XMSYS_MessageSocketFileListUpdate (channel, type, file_name, 0, 0, 0);
		}
		else
		{
			ret = 0;
			if(videoitem_debug)
				XM_printf ("File %s, %d ms remove failed\n", lpVideoFileToRemove->file_name[i], ticket);
		}
	}
	
	return ret;
}

static int insert_video_item (char *file_name, unsigned int stream_class, BYTE attribute, DWORD dwFileSize, XMSYSTEMTIME* createTime)
{
	NodeType * i;
	XMVIDEOITEM *video_item;
	RbtStatus status;
	int loop;
	char *ch;
	DWORD cluster_size;
	char videoitem_name[VIDEOITEM_MAX_NAME+1];
	unsigned int file_type, channel_index, volume_index;
	unsigned int serial_number;

	memset (videoitem_name, 0, sizeof(videoitem_name));
	strcpy (videoitem_name, file_name);

	XM_ASSERT (channel < video_path_count);
	if ((i = (NodeType *)rbtFind(h, videoitem_name)) != NULL)
	{
		((void)(i));
		unsigned char		video_name[VIDEOITEM_MAX_FULL_NAME + 1];
		unsigned char		full_path_name[VIDEOITEM_MAX_FULL_PATH_NAME + 1];
		// found an existing node
		XM_printf ("found same node (%s)\n", videoitem_name);
		//assert (0);
		if(videoitem_extract_info (file_name, (char *)video_name, sizeof(video_name), 
											&file_type, NULL, &volume_index, &channel_index, NULL, &serial_number) < 0)
			return 0;
		
		if(XM_VideoItemMakeFullPathFileName ((char *)video_name, volume_index, channel_index, file_type, stream_class, (char *)full_path_name, sizeof(full_path_name)) < 0)
			return 0;
		
		XM_printf ("delete same node (%s)\n", full_path_name);
		XM_RemoveFile ((char *)full_path_name);
		return 0;
	}
	else
	{
		// 创建新节点
		// 检查是否已达到最大的视频项记录个数
		if(video_item_count >= MAX_VIDEO_ITEM_COUNT)
		{
			XM_printf ("exceed maximum video item count %d\n", video_item_count);
			return 0;
		}
		
		video_item = (XMVIDEOITEM *)videoitem_malloc ();
		if(video_item == NULL)
		{
			//XM_printf ("videoitem_malloc XMVIDEOITEM failed\n");
			return 0;
		}
		memset (video_item, 0, sizeof(XMVIDEOITEM));
		
		if(videoitem_extract_info (file_name, (char *)video_item->video_name, sizeof(video_item->video_name), 
											&file_type, NULL, &volume_index, &channel_index, NULL, &serial_number) < 0)
		{
			videoitem_free (video_item);
			return 0;
		}

		video_item->video_channel = channel_index;
		video_item->video_update |= (1 << channel_index);
		video_item->video_create_time = *createTime;
		if(attribute & D_RDONLY)
			video_item->video_lock = 1;
		else
			video_item->video_lock = 0;
		// 累计文件大小(以族为单位)
		video_item->file_size = dwFileSize;
		cluster_size = get_fat_cluster_size_from_volume (volume_index);
		if(cluster_size)
			video_item->video_size = ((dwFileSize + cluster_size - 1) / cluster_size) * cluster_size;
		video_item->volume_index = volume_index;
		video_item->file_type = file_type;
		video_item->stream_class = stream_class;
		/* 
		if(stream_class == XM_VIDEOITEM_CLASS_NORMAL && video_item->video_lock)
		{
			video_item->stream_class = XM_VIDEOITEM_CLASS_MANUAL;
		}
		*/
		video_item->serial_number = serial_number;

		// insert in red-black tree
		status = rbtInsert(h, video_item->video_name, video_item);
		if (status != RBT_STATUS_OK)
		{
			XM_printf("fail to insert new node: status = %d\n", status);
			videoitem_free (video_item);
			return 0;
		}
		
		// 更新最大值
		if(serial_number_type == SERIAL_NUMBER_TYPE_SHARE)
		{
			// 每种视频项类型共享相同的序号
			if(serial_number > video_item_serial_number[volume_index][0][channel_index])
				video_item_serial_number[volume_index][0][channel_index] = serial_number;
		}
		else
		{
			// 每种视频项类型具有独立的序号
			if(serial_number > video_item_serial_number[volume_index][file_type][channel_index])
				video_item_serial_number[volume_index][file_type][channel_index] = serial_number;
		}

		// 累加视频项个数
		video_item_count ++;
		videoitem_inc_count (video_item, video_item->stream_class);
	}
	
	return 1;
}

// 将符合标准命名的视频文件加入到视频文件列表
static int append_one_video (char *file_name, unsigned int stream_class, BYTE attribute, DWORD dwFileSize, XMSYSTEMTIME* createTime)
{
	char *ch;
	int name_count = 0;
	int ret;
	unsigned int file_type, volume_index, channel_index;
	
	if(strstr (file_name, "TEMP"))
		return 0;
	
	ch = file_name;
	while(*ch)
	{
		// 文件名的命名只包括数字
		if(*ch >= '0' && *ch <= '9' || *ch >= 'A' && *ch <= 'Z' || *ch == '_')
		{
			name_count ++;
			if(name_count > VIDEOITEM_LFN_SIZE)
				return 0;
		}
		else if(*ch == '.')
		{
			// 检查是否是标准长度文件名
			if(name_count != VIDEOITEM_LFN_SIZE)
				return 0;
			ch ++;
			break;
		}
		else
		{
			// 非标准命名字符
			return 0;
		}
		ch ++;
	}
	
	if(name_count != VIDEOITEM_MAX_NAME)
		return 0;
	
	if(	(ch[0] == 't' || ch[0] == 'T')
		&&	(ch[1] == 'm' || ch[1] == 'M')
		&&	(ch[2] == 'p' || ch[2] == 'P')
		&& (ch[3] == 0) )			
	{
		// 20180928 此处不删除TMP文件
		return 0;	// 错误
#if 0
		// 删除临时文件
		char FullFileName[XM_MAX_FILEFIND_NAME];
		unsigned int file_type; 

		if(videoitem_extract_info (file_name, NULL, 0, &file_type, NULL, &volume_index, &channel_index, NULL, NULL) < 0)
			return 0;
		
		if(XM_VideoItemMakeFullPathFileName (file_name, volume_index, channel_index, file_type, stream_class, FullFileName, sizeof(FullFileName)) < 0)
			return 0;
		if(XM_RemoveFile (FullFileName))
			return 1;	// OK
		else
			return 0;	// 错误
#endif
	}
	
	if(videoitem_extract_info (file_name, NULL, 0, &file_type, NULL, &volume_index, &channel_index, NULL, NULL) < 0)
		return 0;

#if _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_AVI_
	if(	(ch[0] == 'a' || ch[0] == 'A')
		&&	(ch[1] == 'v' || ch[1] == 'V')
		&&	(ch[2] == 'i' || ch[2] == 'I')
#elif _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_MKV_
	if(	(ch[0] == 'm' || ch[0] == 'M')
		&&	(ch[1] == 'k' || ch[1] == 'K')
		&&	(ch[2] == 'v' || ch[2] == 'V')
#elif _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_MOV_
	if(	(ch[0] == 'm' || ch[0] == 'M')
		&&	(ch[1] == 'o' || ch[1] == 'O')
		&&	(ch[2] == 'v' || ch[2] == 'V')
#endif
		&& (ch[3] == 0) )
	{
		// 有效的文件名
		// 检查文件长度是否为0. 若是, 删去0字节大小的文件
		if(dwFileSize == 0)
		{
			char FullFileName[VIDEOITEM_MAX_FULL_PATH_NAME];
			if(XM_VideoItemMakeFullPathFileName (file_name, volume_index, channel_index, file_type, stream_class, FullFileName, sizeof(FullFileName)) < 0)
				return 0;
			if(XM_RemoveFile (FullFileName))
				return 1;	// OK
			else
				return 0;	// 错误
		}
		//XM_printf(">>>>inser video, file_name:%s\r\n", file_name);
		ret = insert_video_item (file_name, stream_class, attribute, dwFileSize, createTime);
		/*
		if(ret)
		{
			XM_printf ("insert_video_item OK, channel=%d, file_name=%s\n", channel, file_name);
		}
		else
		{
			XM_printf ("insert_video_item NG, channel=%d, file_name=%s\n", channel, file_name);
		}
		*/
		return ret;
	}
	if(	(ch[0] == 'j' || ch[0] == 'J')
		&&	(ch[1] == 'p' || ch[1] == 'P')
		&&	(ch[2] == 'g' || ch[2] == 'G')
		&& (ch[3] == 0) )			
	{
		if(dwFileSize == 0)
		{
			char FullFileName[VIDEOITEM_MAX_FULL_PATH_NAME];
			if(XM_VideoItemMakeFullPathFileName (file_name, volume_index, channel_index, file_type, stream_class, FullFileName, sizeof(FullFileName)) < 0)
				return 0;
			if(XM_RemoveFile (FullFileName))
				return 1;	// OK
			else
				return 0;	// 错误			
		}
		//XM_printf(">>>>inser photo, file_name:%s\r\n", file_name);
		
		// 照片文件
		return insert_video_item (file_name, stream_class, attribute, dwFileSize, createTime);
	}
	else
		return 0;
}
			 			 
int XM_VideoItemInitEx (unsigned int volume_count, unsigned int channel_count, const char **video_channel_file_path)
{
	unsigned int volume_index, channel_index;
	if(volume_count > XM_VIDEOITEM_VOLUME_COUNT)
		return -1;
	if(channel_count > XM_VIDEO_CHANNEL_COUNT)
		return -1;

	videoitem_int ();
	
	rbtr_int ();
	
	h = NULL;


	// 配额及记录管理
	video_item_recycle_space = 0;
	video_item_locked_space = 0;
	video_item_count = 0;
	memset (video_item_space, 0, sizeof(video_item_space));
	memset (class_item_count, 0, sizeof(class_item_count));
	memset (item_count_quota, 0, sizeof(item_count_quota));
	memset (disk_space_quota_level_class, 0, sizeof(disk_space_quota_level_class));
	memset (disk_space_quota_level_type, 0, sizeof(disk_space_quota_level_type));
	memset (disk_space_quota_level_volume, 0, sizeof(disk_space_quota_level_volume));
	
	minimum_space_limit = 0;
	recycle_space_limit = 0;
		
	basic_service_opened = 0;
	videoitem_service_opened = 0;
	
	memset (video_item_serial_number, 0, sizeof(video_item_serial_number));

	memset (video_path_name, 0, sizeof(video_path_name));
	memset (video_path_valid, 0, sizeof(video_path_valid));
	memset (video_volume_valid, 0, sizeof(video_volume_valid));
	memset (fat_cluster_size, 0, sizeof(fat_cluster_size));
	video_path_count = channel_count;
	video_volume_count = volume_count;
	
	for (volume_index = 0; volume_index < video_volume_count; volume_index ++)
	{
		for (channel_index = 0; channel_index < video_path_count; channel_index ++)
		{
			for (unsigned int file_type = XM_FILE_TYPE_VIDEO; file_type < XM_FILE_TYPE_COUNT; file_type ++)
			{
				for (unsigned int stream_class = XM_VIDEOITEM_CLASS_NORMAL; stream_class < XM_VIDEOITEM_CLASS_COUNT; stream_class ++)
				{
					XM_VideoItemMakeFullPathName (volume_index, channel_index, file_type, stream_class, 
															video_path_name[volume_index][channel_index][file_type][stream_class], 
															sizeof(video_path_name[volume_index][channel_index][file_type][stream_class]));
				 
					#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2			
					// 普通项，手动项，紧急项均保存在各自独立的目录下
					video_path_valid[volume_index][channel_index][file_type][stream_class] = 1;
					#else
					// 普通项，手动项，紧急项均保存在同一个目录下
					if(file_type == XM_FILE_TYPE_VIDEO && stream_class == XM_VIDEOITEM_CLASS_NORMAL)
						video_path_valid[volume_index][channel_index][file_type][stream_class] = 1;
					else
						video_path_valid[volume_index][channel_index][file_type][stream_class] = 0;
					#endif
				}
			}
			//XM_VideoItemMakePathName (volume_index, channel_index, video_path_name[volume_index][channel_index], sizeof(video_path_name[volume_index][channel_index]));
		}
	}
	
	// 创建互斥信号量
	XM_VideoItemCreateResourceMutexSemaphore ();
	return 0;
}
				 
				 
		 
int XM_VideoItemInit (unsigned int video_channel_count, const char **video_channel_file_path)
{
	return XM_VideoItemInitEx (1, video_channel_count, video_channel_file_path);
}

		 
void XM_VideoItemExit (void)
{
	XM_VideoItemCloseService ();

	XM_VideoItemDeleteResourceMutexSemaphore ();

	h = NULL;
	video_path_count = 0;
	memset (video_path_name, 0, sizeof(video_path_name));
	video_volume_count = 0;
	memset (video_volume_valid, 0, sizeof(video_volume_valid));
}

// 获取视频文件通道(路径)的个数
int XM_VideoItemGetVideoFileChannel (void)
{
	return video_path_count;
}

// SD卡拔出后，关闭视频项服务
void XM_VideoItemCloseService (void)
{
	RbtIterator i;
	
	XM_VideoItemLock ();
	
	basic_service_opened = 0;
	videoitem_service_opened = 0;
		
	if(h == 0)
	{
		XM_VideoItemUnlock ();
		return;
	}

	if(h)
	{
		// delete my allocated memory
		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			void *keyp, *valuep;
			rbtKeyValue(h, i, &keyp, &valuep);
			videoitem_free (valuep);
		}
		
		// delete red-black tree
		rbtDelete(h);
		h = NULL;
	}
	
	video_item_recycle_space = 0;
	video_item_locked_space = 0;
	video_item_count = 0;
	memset (video_item_space, 0, sizeof(video_item_space));
	memset (class_item_count, 0, sizeof(class_item_count));
	memset (item_count_quota, 0, sizeof(item_count_quota));
	memset (disk_space_quota_level_class, 0, sizeof(disk_space_quota_level_class));
	memset (disk_space_quota_level_type, 0, sizeof(disk_space_quota_level_type));
	memset (disk_space_quota_level_volume, 0, sizeof(disk_space_quota_level_volume));
	
	minimum_space_limit = 0;
	recycle_space_limit = 0;
	
	memset (video_item_serial_number, 0, sizeof(video_item_serial_number));
		
	memset (fat_cluster_size, 0, sizeof(fat_cluster_size));
	
	delay_recycle_clean = 0;		// 清除回收站清除操作的标志

	disk_free_space_busy = 0;

	XM_VideoItemUnlock ();
}
			 
// TEMPNNNN_0P0.AVI
//	20180318_121056_00000002_00U.AVI		 
static int videoitem_extract_info(	const char *lpVideoFileName, 
									char *lpVideoItemName, int cbVideoItemName,
									unsigned int *file_type,
									XMSYSTEMTIME *create_time, 
									unsigned int *volume_index, unsigned int *channel_index, unsigned int *class_index,
									unsigned int *serial_number
								 )
{
	char VideoItemFileName[VIDEOITEM_MAX_FULL_NAME + 1];
	char *ch;
	int is_tempfile = 0;
	int len;
	
	ch = strrchr (lpVideoFileName, '\\');
	if(ch)
	{
		ch ++;
	}
	else
	{
		ch = (char *)lpVideoFileName;
	}
	if(*ch == 'T' && *(ch+1) == 'E' && *(ch+2) == 'M' && *(ch+3) == 'P')
		is_tempfile = 1;
	len = strlen(ch);
	if(is_tempfile)
	{
		if(len != TEMPITEM_MAX_FILE_NAME)
			return -1;
		if(lpVideoItemName)
		{
			if(cbVideoItemName < (len + 1))
				return -1;
			strcpy (lpVideoItemName, ch);
		}
		ch = ch + 9;
	}
	else
	{
		if(len != VIDEOITEM_MAX_FULL_NAME)
			return -1;
		if(lpVideoItemName)
		{
			if(cbVideoItemName < (len + 1))
				return -1;
			strcpy (lpVideoItemName, ch);
		}
		ch = ch + 16;
	}
		
	if(create_time)
	{
		// 获取文件创建时间
		if(XM_GetFileCreateTime (lpVideoFileName, create_time) < 0)
			return -1;
		create_time->wMilliSecond = 0;
	}
	
	// 序号
	if(serial_number)
	{
		unsigned int sn = 0;
		for (unsigned int i = 0; i < 8; i ++)
		{
			if(*ch < '0' || *ch > '9')
				return -1;
			sn *= 10;
			sn += ch[i] - '0';
		}
		*serial_number = sn;
	}
	ch += 9;
	
	// 通道号
	if(channel_index)
	{
		if(*ch >= '0' && *ch < ('0' + XM_VIDEO_CHANNEL_COUNT))
			*channel_index = *ch - '0';
		else
			return -1;
	}
	ch ++;
	// 磁盘号
	if(volume_index)
	{
		if(*ch >= '0' && *ch < ('0' + video_volume_count))
			*volume_index = *ch - '0';
		else
			return -1;
	}
	ch ++;
	// 录制分类
	if(class_index)
	{
		if(*ch == 'N')
			*class_index = XM_VIDEOITEM_CLASS_NORMAL;
		else if(*ch == 'M')
			*class_index = XM_VIDEOITEM_CLASS_MANUAL;
		else if(*ch == 'U')
			*class_index = XM_VIDEOITEM_CLASS_URGENT;
		else
			return -1;
	}
	ch += 2;
	if(file_type)
	{
		if(!xm_stricmp (ch, XM_VIDEO_FILE_EXT))
			*file_type = XM_FILE_TYPE_VIDEO;
		else if(!xm_stricmp (ch, "JPG"))
			*file_type = XM_FILE_TYPE_PHOTO;
		else
			return -1;
	}
	return 0;
}


			 
int check_and_create_directory (char *path);

static int safe_send_system_event (unsigned short system_event)
{
	int i;
	i = 0;
	while( i < 10 )
	{
		if(XM_KeyEventProc (VK_AP_SYSTEM_EVENT, system_event ))
			break;

		XM_Sleep (1);
		i ++;
	}
	if(i == 10)
		return 0; // failed
	return 1;	// success
}

int FS_GetDiskFatInfo(const char * sVolume, FS_DISK_INFO * pInfo);
int FS_GetDiskTotalFreeClusters (const char * sVolume, U32 *pNumFreeClusters,
									 void (*pUserCallback)(void *), void *pUserPrivate);

// 将视频项文件添加到视频项数据库
// 	ch					通道编号(0, 1, ...)
// 	lpVideoFile		全路径视频文件名
// 返回值
//	1		成功
//	0		失败
XMBOOL XM_VideoItemAppendVideoFile (unsigned int ch, const char *lpVideoFile)
{
	XMBOOL ret = 0;
	DWORD file_size;
	char filename[VIDEOITEM_MAX_FILE_NAME + 1];
	XMSYSTEMTIME system_time;
	unsigned int volume_index, channel_index, file_type, class_index, serial_number;
	memset (filename, 0, sizeof(filename));
	
	XM_VideoItemLock ();
	do
	{
		// 检查目录项完整服务是否已开启
		if(videoitem_service_opened == 0)
			break;
		
		if(h)
		{
			if(videoitem_extract_info (lpVideoFile, filename, sizeof(filename), &file_type, NULL, &volume_index, &channel_index, &class_index, &serial_number) < 0)
				break;
			file_size = XM_GetFileSize (lpVideoFile);
			if( file_size == (DWORD)(-1) )
				break;
			
			if(XM_GetFileCreateTime (lpVideoFile, &system_time) < 0)
				break;
			// 文件创建时间中没有毫秒读数，置0
			system_time.wMilliSecond = 0;
						
			// 普通文件
			if( append_one_video (filename, class_index, 0, file_size, &system_time) == 0 )
			{
				ret = 1;
			}
		}
	} while(0);
	
	XM_VideoItemUnlock ();
	return ret;
}

// 检查物理SD卡是否插入
int XMSYS_check_sd_card_exist (void);

// 检查卡的视频写入性能
// 返回值
//		-1		无法获取性能值
//		0		视频性能可能较差
//		1		视频性能满足记录要求
int XM_VideoItemCheckCardPerformance (void)
{
	int ret = 0;
	int index;
	XM_VideoItemLock ();	
	if(videoitem_service_opened == 0)
	{
		ret = -1;
	}
	else
	{
		for (index = 0; index < video_volume_count; index ++)
		{
			if(fat_cluster_size[index] <= (512 * 32))		// 16, 使用更大容量的簇大小, 优化FAT空间分配/释放速度
			{
				ret = 0;
			}
			else
			{
				ret = 1;
			}
		}
	}
	XM_VideoItemUnlock ();
	return ret;
}

// 检查指定卡的可用循环录像空间容量(= 磁盘空闲字节容量 + 循环录像视频占用的文件字节容量)是否满足最低的录像要求
// 返回值
//		-1		无法获取该性能参数
//		0		无法满足最低录像要求, 提示格式化卡
//		1		满足最低录像要求
int XM_VideoItemCheckCardRecycleSpaceEx (unsigned int volume_index)
{
	int ret = -1;
	if(volume_index >= video_volume_count)
		return -1;
	XM_VideoItemLock ();	
	do
	{
		if(videoitem_service_opened == 0)
		{
			break;
		}
		else
		{
			DWORD SectorsPerCluster = 0;    // sectors per cluster
			DWORD BytesPerSector = 0;        // bytes per sector
			DWORD NumberOfFreeClusters = 0;  // free clusters
			DWORD TotalNumberOfClusters = 0;  // total clusters
			char *volume_name;
			
			XMINT64	available_videoitem_space = 0;	// 可用于视频项记录的空闲空间		
			
			volume_name = (char *)XM_VideoItemGetVolumeName (volume_index);
			if(volume_name == NULL)
				break;
						
			if(XM_GetDiskFreeSpace ("\\", &SectorsPerCluster, &BytesPerSector,
						&NumberOfFreeClusters, &TotalNumberOfClusters, volume_name) == 0)
			{
				break;
			}
			
			// 检查可循环使用空间容量+空闲空间容量
			available_videoitem_space = NumberOfFreeClusters;
			available_videoitem_space *= SectorsPerCluster;
			available_videoitem_space *= BytesPerSector;
			available_videoitem_space += video_item_recycle_space;
			
			// 检查“循环录像空间”是否满足最低录像需求5分钟, 按照12Mbps码流计算
			if(available_videoitem_space  < (540*1024*1024))
			{
				// 无法满足最低录像要求
				XM_printf ("volume(%s)'s total recycle space (%d) MB is too small\n", volume_name, (unsigned int)(available_videoitem_space/(1024*1024)));
				ret = 0; 
			}
			else
			{
				// 基本满足最低录像要求
				ret = 1;
			}
		}
	} while (0);
	
	XM_VideoItemUnlock ();
	return ret;		
}


// 检查卡可用拍照空间容量
// 返回剩余可用于拍照空间容量
XMINT64 XM_VideoItemCheckCardPhotoSpaceEx (void)
{
	int i;
	unsigned int volumn_index;
	int quota_check_result;
	XMINT64 total_space;
	XMINT64 quota;
	unsigned int class_index;
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	// 
	XM_VideoItemLock ();
	if(h == NULL || video_volume_count == 0)
	{
		XM_VideoItemUnlock ();
		return 0;
	}
	XM_VideoItemUnlock ();
	
	// 检查是否需要执行回收站清除操作
	if(delay_recycle_clean)
	{
		delay_recycle_clean = 0;
		for (int ch = 0; ch < video_path_count; ch ++)
		{
			char recycle_path[XM_MAX_FILEFIND_NAME];
			//sprintf (recycle_path, "%s\\RECYCLE", video_path_name[ch]);
			if(XM_VideoItemMakePathName(XM_VIDEOITEM_VOLUME_0, ch, recycle_path, sizeof(recycle_path)) >= 0)
			{
				strcat (recycle_path, "\\RECYCLE");
				XM_RecycleCleanFile (recycle_path);
			}
		}
	}
    
    for (volumn_index = 0; volumn_index < video_volume_count; volumn_index ++)
	{
		// 检查簇扫描是否已完成
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		// 照片空间检查	
		// 检查基于该文件类型的配额限制是否存在
		quota = disk_space_quota_level_type[volumn_index][XM_FILE_TYPE_PHOTO];
		if(quota)
		{
			// 非0值表示存在配额定义
			// 累加该文件类型的所有录制分类的容量
			total_space = 0;
			for (class_index = 0; class_index < XM_VIDEOITEM_CLASS_COUNT; class_index ++)
			{
				total_space += video_item_space[volumn_index][XM_FILE_TYPE_PHOTO][class_index];
			}
			if(total_space > quota)
				return 0;
			else
				return (quota - total_space);
		}
	}
	return 0;
}


// 检查卡0的可用循环录像空间容量(= 磁盘空闲字节容量 + 循环录像视频占用的文件字节容量)是否满足最低的录像要求
// 返回值
//		-1		无法获取该性能参数
//		0		无法满足最低录像要求, 提示格式化卡
//		1		满足最低录像要求
int XM_VideoItemCheckCardRecycleSpace (void)
{
	return XM_VideoItemCheckCardRecycleSpaceEx (XM_VIDEOITEM_VOLUME_0);
}

// 检查卡可用拍照空间容量
// 返回剩余可用于拍照空间容量
XMINT64 XM_VideoItemCheckCardPhotoSpace (void)
{
	return XM_VideoItemCheckCardPhotoSpaceEx ();
}

// 检查并创建视频项路径
static int check_and_create_video_folder_old (unsigned int volume_index)
{
	unsigned int channel_index;
	unsigned int file_type;
	unsigned int stream_class;
	char VideoPath[VIDEOITEM_MAX_FULL_PATH_NAME];
	XMFILEFIND fileFind;
	char fileName[XM_MAX_FILEFIND_NAME];		// 保存文件查找过程的文件名
	int ret = -1;
	for (channel_index = XM_VIDEO_CHANNEL_0; channel_index < video_path_count; channel_index ++)
	{
		for (file_type = XM_FILE_TYPE_VIDEO; file_type < XM_FILE_TYPE_COUNT; file_type ++)
		{
			// 检查同名的目录或文件是否存在
			if(XM_VideoItemMakePathNameEx(volume_index, channel_index, file_type, VideoPath, sizeof(VideoPath)) < 0)
				return -1;
			if(XM_FindFirstFile ((char *)VideoPath, &fileFind, fileName, XM_MAX_FILEFIND_NAME))
			{
				// 同名目录已找到
				XM_FindClose (&fileFind);
				continue;
			}
			else
			{
				// 创建新目录
				if(check_and_create_directory(VideoPath) != 0)
				{
					if(videoitem_debug)
						XM_printf ("failed to create video folder(%s)\n", VideoPath);
					
					// 退出。通过UI提示用户卡无法记录
					return -1;
				}
			}
		}
	}
	return 0;
}

// 检查并创建视频项路径
static int check_and_create_video_folder (unsigned int volume_index)
{
	unsigned int channel_index;
	unsigned int file_type;
	unsigned int stream_class;
	char *VideoPath;
	XMFILEFIND fileFind;
	char fileName[XM_MAX_FILEFIND_NAME];		// 保存文件查找过程的文件名
	int ret = -1;

	XM_printf(">>>>>check_and_create_video_folder......\r\n");
	XM_printf(">>>>>video_path_count:%d\r\n", video_path_count);
	
	for (channel_index = XM_VIDEO_CHANNEL_0; channel_index < video_path_count; channel_index ++)
	{
		for (file_type = XM_FILE_TYPE_VIDEO; file_type < XM_FILE_TYPE_COUNT; file_type ++)
		{
			for (stream_class = XM_VIDEOITEM_CLASS_NORMAL; stream_class < XM_VIDEOITEM_CLASS_COUNT; stream_class ++)
			{
				if(video_path_valid[volume_index][channel_index][file_type][stream_class])
				{
					VideoPath = video_path_name[volume_index][channel_index][file_type][stream_class];
					XM_printf(">>>>>VideoPath:%s\r\n", VideoPath);
					
					// 检查同名的目录或文件是否存在
					if(XM_FindFirstFile ((char *)VideoPath, &fileFind, fileName, XM_MAX_FILEFIND_NAME))
					{
						// 同名目录已找到
						XM_FindClose (&fileFind);
						continue;
					}
					else
					{
						// 创建新目录
						if(check_and_create_directory(VideoPath) != 0)
						{
							if(videoitem_debug)
								XM_printf ("failed to create video folder(%s)\n", VideoPath);
							
							// 退出。通过UI提示用户卡无法记录
							return -1;
						}
					}					
				}
			}
		}	// file_type
	}	// channel_index
	return 0;
}

static int scan_one_video_path (unsigned int stream_class, char *VideoPath)
{
	XMFILEFIND fileFind;
	XMSYSTEMTIME create_time;
	char fileName[XM_MAX_FILEFIND_NAME];		// 保存文件查找过程的文件名
	char FullPathName[VIDEOITEM_MAX_FULL_PATH_NAME + 1];
	unsigned int scan_ticket = XM_GetTickCount();
	
	do
	{		
		int file_count = 0;
		int err_count = 0;
		// 遍历扫描所有视频路径
			
#ifdef _WINDOWS
		strcat (VideoPath, "\\*.*");
#endif
		if(XM_FindFirstFile (VideoPath, &fileFind, fileName, XM_MAX_FILEFIND_NAME))
		{
			do
			{
				DWORD dwFileAttributes;
				// 检查目录项属性
				file_count ++;
				if(file_count >= 65536)	// 超出65536个文件
				{
					XM_printf ("scan_video_item_file Failed, too many file in video folder\n");
				video_folder_error:
					// 可以认为是已损坏的文件目录或者已严重影响FAT访问效率, 提示格式化
					XM_FindClose (&fileFind);
					return -1;
				}
				dwFileAttributes = fileFind.dwFileAttributes;
				// XM_printf ("name=%s, attr=0x%x\n", fileName, dwFileAttributes);
				// 检查该目录项是否是文件属性
				if((dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == 0x00)
				{
					// 普通文件
					//XM_printf ("%d, %s, %s\n", ch, fileName, (dwFileAttributes & D_RDONLY) ? "Locked" : "Normal");
					// 20180722 zhuoyonghong
					// 	手动类型在扫描时强制加上只读属性，避免文件改名与属性修改未能同时完成而导致的手动类型文件缺乏只读属性设置的bug
#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2
					if(stream_class == XM_VIDEOITEM_CLASS_NORMAL)
						fileFind.dwFileAttributes &= ~XM_FILE_ATTRIBUTE_READONLY;
					else if(stream_class == XM_VIDEOITEM_CLASS_MANUAL)
						fileFind.dwFileAttributes |= XM_FILE_ATTRIBUTE_READONLY;
#endif
					if(append_one_video (fileName, stream_class, (BYTE)(fileFind.dwFileAttributes), fileFind.nFileSize, &fileFind.CreationTime) == 0)
					{
						// 20180928 检查是否是TMP文件。若是，直接删除
						if(strstr (fileName, "TMP") || strstr (fileName, "tmp"))
						{
							sprintf (FullPathName, "%s\\%s", VideoPath, fileName);
							if(XM_RemoveFile (FullPathName))
							{
								XM_printf ("Remove %s OK\n", FullPathName);
							}
							else
							{
								XM_printf ("Remove %s NG\n", FullPathName);
							}
						}
						// 文件名称异常/同名等
						err_count ++;
					}
				}
				else if( dwFileAttributes & 0xC0 )	// 检查目录属性高2位是否不为0. 若不为0, 属性异常
				{
					// The upper two bits of the attribute byte are reserved and should
					//	always be set to 0 when a file is created and never modified or
					//	looked at after that.
					// 文件属性异常
					err_count ++;
				}
				else if( (dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == ATTR_DIRECTORY)
				{
					// 目录
					if(xm_stricmp (fileName, "RECYCLE") == 0)
					{
						// 回收站目录
						// 将回收站文件夹中的图片全部删除
						// char recycle_path[XM_MAX_FILEFIND_NAME];
						// sprintf (recycle_path, "%s\\%s", VideoPath, fileName);
						// XM_RecycleCleanFile (recycle_path);
						// 延期执行回收站清除操作
						delay_recycle_clean = 1;		// 标记需要执行回收站清除操作
					}
				}
				if(err_count >= 1024)
				{
					XM_printf ("scan_video_item_file Failed, too many directory entry error in video folder\n");
					goto video_folder_error;
				}
					
				// 检查ACC供电是否安全
				if(!xm_power_check_acc_safe_or_not())
					break;
					
			} while(XM_FindNextFile (&fileFind));
			XM_FindClose (&fileFind);
		}
		else
		{
			// 视频保存目录无法创建
			XM_printf ("scan video's dir (%s) failed\n", VideoPath);
			return -1;
		}

		// 刷新文件系统, 保护上面扫描操作过程中所有对文件系统的更新写入到文件系统
		FS_CACHE_Clean ("");

	} while (0);
	scan_ticket = XM_GetTickCount() - scan_ticket;
	//XM_printf ("scan_video_path spend %d ms\n", scan_ticket);	
	return 0;	
}

// 扫描卷的视频项文件夹
static int scan_video_item_file (unsigned int volume_index)
{
	unsigned int channel_index;
	unsigned int class_index;
	unsigned int file_type;
	unsigned int volume;
	unsigned int scan_ticket = XM_GetTickCount();

	if(volume_index >= XM_VIDEOITEM_VOLUME_COUNT)
		return -1;
	
	for (channel_index = 0; channel_index < XM_VIDEO_CHANNEL_COUNT; channel_index ++)
	{
		for (file_type = XM_FILE_TYPE_VIDEO; file_type < XM_FILE_TYPE_COUNT; file_type ++)
		{
			for (class_index = XM_VIDEOITEM_CLASS_NORMAL; class_index < XM_VIDEOITEM_CLASS_COUNT; class_index ++)
			{
				// 检查对应的路径是否有效或唯一
				if(video_path_valid[volume_index][channel_index][file_type][class_index])
				{
					if(scan_one_video_path (class_index, video_path_name[volume_index][channel_index][file_type][class_index]) < 0)
						return -1;
				}
			}
		}
	}
	
	scan_ticket = XM_GetTickCount() - scan_ticket;
	XM_printf ("scan_video_item_file spend %d ms\n", scan_ticket);	
	
	return 0;
}
			
// 计算所有视频项统计信息			
static void calculate_videoitem_statistics (void)
{
	RbtIterator i;
	XMVIDEOITEM *video_item;
	memset (&video_item_space, 0, sizeof(video_item_space));
	// 扫描所有视频项，统计文件信息，计算可循环利用的空间
	for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
	{
		void *keyp, *valuep;
		DWORD cluster_size;
		rbtKeyValue(h, i, &keyp, &valuep);
		// XM_printf("%d %d\n", *(int *)keyp, *(int *)valuep);
		video_item = (XMVIDEOITEM *)keyp;
		XM_ASSERT (video_item);
		// 更新video_size
		cluster_size = get_fat_cluster_size_from_volume (video_item->volume_index);
		if(cluster_size)
			video_item->video_size = ((video_item->file_size + cluster_size - 1) / cluster_size) * cluster_size;
		if(video_item->video_lock)
		{
			// 只要其中任意一个通道锁定，则这个视频项锁定(即同时锁定或解锁)
			// 累加已锁定视频文件的字节大小
			video_item_locked_space += video_item->video_size;
			//XM_printf ("locked file size(%lld),  locked_space(%lld)\n",  video_item->video_size, video_item_locked_space);
		}
		else
		{
			// 只要其中任意一个通道锁定，则这个视频项锁定(即同时锁定或解锁)
			// 累加可回收(循环)视频文件字节大小
			video_item_recycle_space += video_item->video_size;
			//XM_printf ("100 recycle + file size(%lld),  recycle_space(%lld)\n",  video_item->video_size, video_item_recycle_space);
		}
		
		assert (video_item->volume_index < video_volume_count);
		assert (video_item->file_type < XM_FILE_TYPE_COUNT);
		assert (video_item->stream_class < XM_VIDEOITEM_CLASS_COUNT);
		
		// 将视频项文件大小信息按卷，文件类型，录制类型等分类统计
		videoitem_inc_space (video_item, video_item->stream_class);
		//video_item_space[video_item->volume_index][video_item->file_type][video_item->stream_class] += video_item->video_size;
	}	
}
			

// 开机或插入SD卡后重新扫描所有视频文件, 开启视频项服务
int XM_VideoItemOpenService (void)
{	
	char VideoPath[VIDEOITEM_MAX_FULL_PATH_NAME];
	char fileName[XM_MAX_FILEFIND_NAME];		// 保存文件查找过程的文件名
	XMFILEFIND fileFind;
	RbtIterator i;
	XMVIDEOITEM *video_item;

	unsigned int index;
	unsigned int volume_index;

	unsigned int ch;

	DWORD SectorsPerCluster = 0;    // sectors per cluster
	DWORD BytesPerSector = 0;        // bytes per sector
	DWORD NumberOfFreeClusters = 0;  // free clusters
	DWORD TotalNumberOfClusters = 0;  // total clusters
	
	//XMINT64	available_videoitem_space = 0;	// 可用于视频项记录的空闲空间


	// 统计扫描时间
	DWORD dwScanTicket;
	
	//int ret = 0;
	
	if(videoitem_debug)
		XM_printf ("XM_VideoItemOpenService\n");

	// 强制关闭
	XM_VideoItemCloseService ();

	XM_VideoItemLock ();
	// 遍历每个卷，检查视频项路径，若不存在并创建
	//XM_printf ("ticket=%d, service folder check\n", XM_GetTickCount ());
	for (volume_index = XM_VIDEOITEM_VOLUME_0; volume_index < video_volume_count; volume_index ++)
	{
		if(check_and_create_video_folder (volume_index) < 0)
		{
			// 退出。通过UI提示用户卡无法记录
			//return -1;
		}
		video_volume_valid[volume_index] = 1;
	}	
			
	
	// 检查至少存在一个有效卷
	for (volume_index = XM_VIDEOITEM_VOLUME_0; volume_index < video_volume_count; volume_index ++)	
	{
		if(video_volume_valid[volume_index])
			break;
	}
	
	if(volume_index == video_volume_count)
	{
		// 退出。通过UI提示用户卡无法记录
		XM_VideoItemUnlock ();
		XM_printf ("XM_VideoItemOpenService faile, no valid volume\n");
		return -1;
	}
	
	// 重新打开服务
	XM_ASSERT (h == NULL);
	h = rbtNew(compare);
	if(h == NULL)
	{
		// 系统异常，内存分配失败
		XM_printf ("XM_VideoItemOpenService NG, rbtNew failed\n");
		//XM_ASSERT (0);
		XM_VideoItemUnlock ();

		// 系统异常，内存分配失败，系统无法录像。此时需要通知系统，重新启动
		XM_ShutDownSystem (SDS_REBOOT);
		return (-2);
	}
	video_item_count = 0;
	memset (class_item_count, 0, sizeof(class_item_count));
	video_item_recycle_space = 0;
	video_item_locked_space = 0;
	memset (&video_item_space, 0, sizeof(video_item_space));

	// 扫描每个卷，将所有视频项文件加入到视频项数据库
	//XM_printf ("scan file\n");
	for (volume_index = 0; volume_index < video_volume_count; volume_index ++)
	{
		if(scan_video_item_file (volume_index) < 0)
		{
			// 扫描发现异常
			video_volume_valid[volume_index] = 0;		// 将改卷标记为无效，不再使用
		}
	}
	
	// 允许视频记录开始工作。"编码器"等待basic_service_opened标志置1后才能开始录像
	basic_service_opened = 1;
	videoitem_service_opened = 1;
	
	basic_service_id ++;

	XM_VideoItemUnlock ();
	
	//video_temp_index = 0;

	XM_CodecStartEventReset ();		// 清除编码器启动事件
	
	
	//XM_printf ("ticket=%d, basic service opened\n", XM_GetTickCount ());

	
	// 延时1秒，等待codec工作
	// 插入卡，且卡被正确识别，录像即开始(任何状况下)。
	//
	// 超时等待录像线程发出的录像启动信号,  主要考虑GetDiskFreeSpace会花费较长的时间, 等待录像线程开始后，再执行耗时的操作。
//	XM_Sleep (1000);
	index = 0;
	while(index < 20)
	{
		if(XM_CodecStartEventWait (50) == 0)
			break;

		// 检查卡是否拔出
		// 检查卡的物理状态
		if(XMSYS_check_sd_card_exist() == 0)
		{
			XM_printf ("the card does not exist\n");
			// 卡已拔出
			return -1;
		}
		index ++;
	}
	
	// ---------------------------------------------
	// ******** 启动磁盘扫描 （耗时操作）***********
	// ---------------------------------------------
	XM_VideoItemLock ();
	//XM_printf ("ticket=%d, get disk space\n", XM_GetTickCount ());

	// 扫描每个视频项路径，获取每个卷的容量信息
	// for (index = 0; index < video_path_count; index ++)
	// 仅扫描第一个路径
	for (index = 0; index < video_volume_count; index ++)
	{
		char *volume = (char *)XM_VideoItemGetVolumeName (index);
		assert(volume);
		
		// 计算卷的簇大小
		if(XM_GetDiskFreeSpace ("\\", &SectorsPerCluster, &BytesPerSector,
					&NumberOfFreeClusters, &TotalNumberOfClusters, volume))
		{		
			if(videoitem_debug)
				XM_printf ("Disk(%s) Space, NumberOfFreeClusters=%d, TotalNumberOfClusters=%d\n",
							  volume,
							  NumberOfFreeClusters, TotalNumberOfClusters);
			fat_cluster_size[index] = SectorsPerCluster * BytesPerSector;
		}
		else
		{
			// 标记卷不存在
			fat_cluster_size[index] = 0;
		}
		//XM_printf ("XM_GetDiskFreeSpace end\n");
	}

	// 统计视频项的空间占用信息
	calculate_videoitem_statistics ();
	
	
	XM_VideoItemUnlock ();
	
	// 请求回收线程马上执行回收处理
	XMSYS_DoRecycle ();



	//XM_printf ("ItemMonitor\n");
	// 关闭执行视频项回收过程 XM_VideoItemMonitorAndRecycleGarbage.
	// 	当视频项回收过程进行中, 此时拔出SD卡, 因为message任务正在执行视频项服务开启(调用 XM_VideoItemOpenService),
	//		会阻塞消息的处理, 导致系统锁死.
	// 录像启动时会开启 XM_VideoItemMonitorAndRecycleGarbage
	// XM_VideoItemMonitorAndRecycleGarbage (0);

	//videoitem_service_opened = 1;
	
	//XM_VideoItemUnlock ();
	
	//XM_printf ("ticket=%d, service start\n", XM_GetTickCount());
	return 0;
}

// 根据视频项属性(卷属性，文件类型属性，通道号及录制分类等)获取符合属性的视频项的数量
//		1)	获取卷0上的前摄像头的普通录制视频文件总数
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_0, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL);
//		2) 获取卷0上的所有通道的紧急录制(G-Sensor)视频文件总数
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, 0xFFFFFFFF,  XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		3) 获取卷1上的所有通道的保护视频(手动录制，紧急录制)文件总数
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_1, 0xFFFFFFFF, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_MANUAL|XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		4) 获取卷0上的后摄像头的所有照片文件总数
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_1, XM_FILE_TYPE_PHOTO, 0xFFFFFFFF);
DWORD XM_VideoItemGetVideoItemCount(unsigned int volume_index, 				// SD卡卷序号(从0开始), 0xFFFFFFFF表示所有卷
									unsigned int video_channel, 			// 通道号， 0xFFFFFFFF表示所有通道
									unsigned int file_type, 				// 文件类型(视频或照片, 0xFFFFFFFF表示所有文件类型)
									unsigned int stream_class_bitmask		// 录像分类掩码，0xFFFFFFFF表示所有录像
										 									// XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL 表示包含普通及手动分类
									)
{
	int ret = -1;
	int item_count = 0;
	XMVIDEOITEM *video_item;
	RbtIterator i;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return -1;
	
	// 锁定
	XM_VideoItemLock ();
	do
	{
		if(h == NULL || video_volume_count == 0)
		{
			break;
		}
		
		// 遍历视频项数据库
		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			void *keyp, *valuep;
			
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = (XMVIDEOITEM *)keyp;
			
			// 属性匹配
			// 磁盘卷
			if(volume_index != 0xFFFFFFFF && volume_index != video_item->volume_index)
				continue;
			// 通道号
			if(video_channel != 0xFFFFFFFF && video_channel != video_item->video_channel)
				continue;
			// 文件类型
			if(file_type != 0xFFFFFFFF && file_type != video_item->file_type)
				continue;
			// 录制分类
			if(stream_class_bitmask & (1 << video_item->stream_class))	
				item_count ++;
		}
		
		ret = item_count;
	} while (0);
	
	// 解锁
	XM_VideoItemUnlock ();
	
	return ret;		
}

// 根据定义的属性获取指定的视频项句柄
//		0) 获取卷0上的后摄像头的最新日期照片(日期最大)的视频项句柄
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_1, XM_FILE_TYPE_PHOTO, 0xFFFFFFFF, 0， 1 );
// 返回值
// == 0 失败
// != 0 读出的列表项纪录句柄
HANDLE XM_VideoItemGetVideoItemHandleEx (
											 unsigned int volume_index,			// 磁盘卷号, 0xFFFFFFFF表示所有卷
											 unsigned int video_channel, 			// 通道号， 0xFFFFFFFF表示所有通道
											 unsigned int file_type,				// 文件类型, 0xFFFFFFFF表示所有文件类型
											 unsigned int stream_class_bitmask,	// 录像分类掩码，0xFFFFFFFF表示所有录像
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL 表示包含普通及手动分类
											 unsigned int index,						// 序号，从0开始
											 unsigned int reverse_time_ordered	// 按时间反向排序， 1 反向排序(时间从大至小) 0 顺序排序(时间从小到大)
											)
{
	HANDLE handle;
	unsigned int item_index = 0;
	XMVIDEOITEM *video_item;
	DWORD dwTotalCount;
	RbtIterator i;
	void *keyp, *valuep;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return NULL;
	
	dwTotalCount = XM_VideoItemGetVideoItemCount (volume_index, video_channel, file_type, stream_class_bitmask);
	if(index >= dwTotalCount)
		return NULL;
	
	if(reverse_time_ordered == 0)
		index = dwTotalCount - 1 - index;
	
	handle = NULL;
	// 锁定
	XM_VideoItemLock ();
	do
	{
		if(h == NULL || video_volume_count == 0)
		{
			break;
		}
		
		// 遍历视频项数据库
		// 定位开始序号
		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = (XMVIDEOITEM *)keyp;
			// 属性匹配
			// 通道号
			if(video_channel != 0xFFFFFFFF && video_channel != video_item->video_channel)
				continue;
			// 磁盘卷
			if(volume_index != 0xFFFFFFFF && volume_index != video_item->volume_index)
				continue;
			// 文件类型
			if(file_type != 0xFFFFFFFF && file_type != video_item->file_type)
				continue;
			// 录像分类
			if(!(stream_class_bitmask & (1 << video_item->stream_class)))
				continue;
			
			if(item_index == index)
			{
				// 找到匹配的开始项序号
				handle = i;
				break;
			}
				
			item_index ++;			
		}
		
	} while (0);
		
	// 解锁
	XM_VideoItemUnlock ();
			
	return handle;	
}

// 获取视频列表文件个数
// mode 指定循环录像模式还是保护录像模式
DWORD AP_VideoItemGetVideoFileCount (BYTE mode)
{
	RbtIterator i;
	XMVIDEOITEM *video_item;
	DWORD dwCount;
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	XM_VideoItemLock ();
	dwCount = 0;
	do
	{	
		if(h == NULL)
			break;

		// output nodes in order
		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			void *keyp, *valuep;
			rbtKeyValue(h, i, &keyp, &valuep);
			// XM_printf("%d %d\n", *(int *)keyp, *(int *)valuep);
			video_item = (XMVIDEOITEM *)keyp;
			XM_ASSERT (video_item);

			// 检查模式是否是保护录像模式
			if(mode == VIDEOITEM_PROTECT_VIDEO_MODE)
			{
				if(video_item->video_lock)
					dwCount ++;
			}
			else
			{
				// 循环录像模式
				dwCount ++;
			}
		}
		
	} while (0);

	XM_VideoItemUnlock ();

	return dwCount;
}

// 获取视频列表文件个数
// mode 指定循环录像模式还是保护录像模式
// nVideoChannel	指定视频通道, 0 前置摄像头 1 后置摄像头
DWORD AP_VideoItemGetVideoFileCountEx (BYTE mode, unsigned int nVideoChannel)
{
	RbtIterator i;
	XMVIDEOITEM *video_item;
	DWORD dwCount;
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	if(nVideoChannel >= video_path_count)
		return 0;
	
	XM_VideoItemLock ();
	dwCount = 0;
	do
	{	
		if(h == NULL)
			break;

		// output nodes in order
		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			void *keyp, *valuep;
			rbtKeyValue(h, i, &keyp, &valuep);
			//XM_printf("%d %d\n", *(int *)keyp, *(int *)valuep);
			video_item = (XMVIDEOITEM *)keyp;
			//XM_printf(">>>>video_item_name:%s\r\n", video_item->video_name);
			XM_ASSERT (video_item);
			
			
			// 检查模式是否是保护录像模式所有通道
			if(mode == VIDEOITEM_PROTECT_VIDEO_MODE)
			{
				if(video_item->video_lock)
					dwCount ++;
			}
			else
			{
				if(video_item->video_channel == nVideoChannel)
				{
					// 循环录像模式
					dwCount ++;
				}
			}
			
		}
		
	} while (0);

	XM_VideoItemUnlock ();

	return dwCount;
}

// 获取视频列表(循环或保护视频列表)中指定序号的视频项目句柄 (按时间从大到小)
// dwVideoIndex 从0开始
HANDLE AP_VideoItemGetVideoItemHandleReverseOrder (BYTE mode, DWORD dwVideoIndex)
{
	RbtIterator i;
	XMVIDEOITEM *video_item;
	DWORD dwCount;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return NULL;
	
	XM_VideoItemLock ();

	dwCount = 0;
	i = NULL;
	do
	{
		if(h == NULL)
			break;

		dwVideoIndex ++;

		// output nodes in order
		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			void *keyp, *valuep;
			rbtKeyValue(h, i, &keyp, &valuep);
			// XM_printf("%d %d\n", *(int *)keyp, *(int *)valuep);
			video_item = (XMVIDEOITEM *)keyp;
			XM_ASSERT (video_item);

			// 检查模式是否是保护录像模式
			if(mode == VIDEOITEM_PROTECT_VIDEO_MODE)
			{
				if(video_item->video_lock)
					dwCount ++;
			}
			else
			{
				// 循环录像模式
				dwCount ++;
			}

			if(dwCount == dwVideoIndex)
			{
				break;
			}
		}
	} while (0);

	XM_VideoItemUnlock ();

	return i;
}

// 获取视频列表(循环或保护视频列表)中指定序号的视频项目句柄 (按时间从大到小)
// dwVideoIndex 从0开始
// nVideoChannel 视频通道号 0 前置摄像头 1 后置摄像头
HANDLE AP_VideoItemGetVideoItemHandleReverseOrderEx (BYTE mode, DWORD dwVideoIndex, unsigned int nVideoChannel)
{
	RbtIterator i;
	XMVIDEOITEM *video_item;
	DWORD dwCount;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return NULL;
	
	XM_VideoItemLock ();

	dwCount = 0;
	i = NULL;
	do
	{
		if(h == NULL)
			break;

		dwVideoIndex ++;

		// output nodes in order
		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			void *keyp, *valuep;
			rbtKeyValue(h, i, &keyp, &valuep);
			// XM_printf("%d %d\n", *(int *)keyp, *(int *)valuep);
			video_item = (XMVIDEOITEM *)keyp;
			XM_ASSERT (video_item);

			if(video_item->video_channel == nVideoChannel)
			{
				// 检查模式是否是保护录像模式
				if(mode == VIDEOITEM_PROTECT_VIDEO_MODE)
				{
					if(video_item->video_lock)
						dwCount ++;
				}
				else
				{
					// 循环录像模式
					dwCount ++;
				}
			}

			if(dwCount == dwVideoIndex)
			{
				break;
			}
		}
	} while (0);

	XM_VideoItemUnlock ();

	return i;
}

// 获取视频列表(循环或保护视频列表)中指定序号的视频项目句柄(按时间从小到大)
// dwVideoIndex 从0开始
HANDLE AP_VideoItemGetVideoItemHandle (BYTE mode, DWORD dwVideoIndex)
{
	RbtIterator i;
	XMVIDEOITEM *video_item;
	DWORD dwCount;
	
	DWORD dwTotalCount = AP_VideoItemGetVideoFileCount (mode);
	if(dwTotalCount == 0)
		return NULL;
	if(dwVideoIndex >= dwTotalCount)
		return NULL;
	
	dwVideoIndex = dwTotalCount - dwVideoIndex - 1;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return NULL;
	
	XM_VideoItemLock ();

	dwCount = 0;
	i = NULL;
	do
	{
		if(h == NULL)
			break;

		dwVideoIndex ++;

		// output nodes in order
		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			void *keyp, *valuep;
			rbtKeyValue(h, i, &keyp, &valuep);
			// XM_printf("%d %d\n", *(int *)keyp, *(int *)valuep);
			video_item = (XMVIDEOITEM *)keyp;
			XM_ASSERT (video_item);

			// 检查模式是否是保护录像模式
			if(mode == VIDEOITEM_PROTECT_VIDEO_MODE)
			{
				if(video_item->video_lock)
					dwCount ++;
			}
			else
			{
				// 循环录像模式
				dwCount ++;
			}

			if(dwCount == dwVideoIndex)
			{
				break;
			}
		}
	} while (0);

	XM_VideoItemUnlock ();

	return i;
}

// 获取视频列表(循环或保护视频列表)中指定序号的视频项目句柄(按时间从小到大)
// dwVideoIndex 从0开始
// nVideoChannel 视频通道号 0 前置摄像头 1 后置摄像头
HANDLE AP_VideoItemGetVideoItemHandleEx (BYTE mode, DWORD dwVideoIndex, unsigned int nVideoChannel)
{
	RbtIterator i;
	XMVIDEOITEM *video_item;
	DWORD dwCount;
	
	DWORD dwTotalCount = AP_VideoItemGetVideoFileCountEx (mode, nVideoChannel);
	//DWORD dwTotalCount = (unsigned int)XM_VideoItemGetVideoItemCount(0,nVideoChannel,mode,XM_VIDEOITEM_CLASS_BITMASK_ALL);
	if(dwTotalCount == 0)
		return NULL;
	if(dwVideoIndex >= dwTotalCount)
		return NULL;
	
	dwVideoIndex = dwTotalCount - dwVideoIndex - 1;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return NULL;
	
	XM_VideoItemLock ();

	dwCount = 0;
	i = NULL;
	do
	{
		if(h == NULL)
			break;

		dwVideoIndex ++;

		// output nodes in order
		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			void *keyp, *valuep;
			rbtKeyValue(h, i, &keyp, &valuep);
			// XM_printf("%d %d\n", *(int *)keyp, *(int *)valuep);
			video_item = (XMVIDEOITEM *)keyp;
			XM_ASSERT (video_item);
             
			// 检查模式是否是保护录像模式
			if(mode == VIDEOITEM_PROTECT_VIDEO_MODE)
			{
				if(video_item->video_lock)
					dwCount ++;
			}
			else
			{
				if(video_item->video_channel == nVideoChannel)
				{
					// 循环录像模式
					dwCount ++;
				}
			}
			if(dwCount == dwVideoIndex)
			{
				break;
			}
		}
	} while (0);

	XM_VideoItemUnlock ();

	return i;
}

// 从视频项句柄获取视频项结构指针
XMVIDEOITEM * AP_VideoItemGetVideoItemFromHandle (HANDLE hVideoItemHandle)
{
	XMVIDEOITEM *video_item;
	void *keyp, *valuep;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return NULL;
	
	XM_VideoItemLock ();

	video_item = NULL;
	do
	{
		if(h == NULL)
			break;
		if(hVideoItemHandle == NULL)
			break;
		rbtKeyValue(h, hVideoItemHandle, &keyp, &valuep);
		// XM_printf("%d %d\n", *(int *)keyp, *(int *)valuep);
		video_item = (XMVIDEOITEM *)keyp;
		XM_ASSERT (video_item);
	} while (0);
	
	XM_VideoItemUnlock ();

	return video_item;
}

// 获取前一个视频索引号, -1表示不存在
DWORD AP_VideoItemGetPrevVideoIndex (BYTE mode, DWORD dwVideoIndex)
{
	DWORD item_count;
	
	item_count = AP_VideoItemGetVideoFileCount(mode);
	if(h == NULL)
		return (DWORD)(-1);
	if(dwVideoIndex == 0)
		return item_count;
	return dwVideoIndex - 1;
}

// 获取下一个视频索引号, -1表示不存在
DWORD AP_VideoItemGetNextVideoIndex (BYTE mode, DWORD dwVideoIndex)
{
	DWORD item_count;
	if(h == NULL)
		return (DWORD)(-1);
	
	item_count = AP_VideoItemGetVideoFileCount(mode);
	if(item_count == 0)
		return (DWORD)(-1);
	if((dwVideoIndex + 1) >= item_count)
		return (DWORD)(-1);
	return dwVideoIndex + 1;
}

// 显示格式化指定视频项句柄的视频文件名
char *AP_VideoItemFormatVideoFileName (XMVIDEOITEM *lpVideoItem, char *lpDisplayName, int cbDisplayName)
{
	const char *name;
	if(h == NULL)
		return NULL;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return NULL;
	
	if(lpDisplayName == NULL)
		return NULL;
	if(cbDisplayName < (VIDEOITEM_LFN_SIZE+1) )
		return NULL;
	if(lpVideoItem == NULL)
		return NULL;

	XM_VideoItemLock ();
	name = (const char *)lpVideoItem->video_name;
	memcpy (lpDisplayName, name, VIDEOITEM_LFN_SIZE);
	lpDisplayName[VIDEOITEM_LFN_SIZE] = 0;
	XM_VideoItemUnlock ();
	
	return lpDisplayName;
}

#if _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_MKV_
union av_intfloat64 {
    XMINT64 i;
    double   f;
};
static  double av_int2double(XMINT64 i)
{
    union av_intfloat64 v;
    v.i = i;
    return v.f;
}
#endif

// 获取视频流的时间长度信息
DWORD AP_VideoItemGetVideoStreamSize (XMVIDEOITEM *lpVideoItem, unsigned int nVideoChannel)
{
	char video_file[VIDEOITEM_MAX_FILE_NAME];
	void *xmfile;
	double video_time = 0.0;
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	XM_VideoItemLock ();
	do
	{
		if(h == NULL)
			break;
		if(nVideoChannel >= video_path_count)
			break;	
		if(lpVideoItem == NULL)
			break;
		if(lpVideoItem->file_type != XM_FILE_TYPE_VIDEO)
			break;
		// 检查流时间信息是否已获取
		if(lpVideoItem->stream_length_valid)
		{
			// 已获取
			video_time = lpVideoItem->stream_length;
			break;
		}
		// 未获取, 从流文件中读取
		memset (video_file, 0, sizeof(video_file));
		
		// 检查是否是新创建视频(正在录制的视频暂时无法获取时间)
		if(!(lpVideoItem->video_update & (1 << nVideoChannel)))
			break;
			
		if(lpVideoItem->video_channel == nVideoChannel)
			XM_VideoItemGetVideoFilePath (lpVideoItem, nVideoChannel, video_file, 64);
		else
			break;
	
#if _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_MKV_
		// 从MKV文件的0x160偏移处读取时间长度信息（64位字节）		
		xmfile = XM_fopen (video_file, "rb");
		if(xmfile)
		{
			//XMINT64 i64;
			unsigned char i64[8];
			unsigned char i64_r[8];
			if(XM_fseek (xmfile, 0x160 + 3, XM_SEEK_SET) == 0)
			{
				XM_fread (&i64, 1, 8, xmfile);
				i64_r[0] = i64[7];
				i64_r[1] = i64[6];
				i64_r[2] = i64[5];
				i64_r[3] = i64[4];
				i64_r[4] = i64[3];
				i64_r[5] = i64[2];
				i64_r[6] = i64[1];
				i64_r[7] = i64[0];
	
				video_time = av_int2double ( *(XMINT64 *)(&i64_r[0]) );
			}
			XM_fclose (xmfile);
			// 标记流时间有效, 
			lpVideoItem->stream_length_valid = 1;
			lpVideoItem->stream_length = (DWORD)video_time;
		}
		break;
#elif _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_AVI_
		// 总帧数, 文件偏移0x30, 4字节
		xmfile = XM_fopen(video_file, "rb");
		if(xmfile)
		{
			unsigned int total_frames = 0;
			unsigned int micro_sec_per_frame;		// 视频帧间隔时间（以微秒为单位）
			if(XM_fseek (xmfile, 0x20, XM_SEEK_SET) >= 0)
			{
				XM_fread (&micro_sec_per_frame, 1, 4, xmfile);
			}
			// 总帧数, 文件偏移0x30, 4字节
			if(XM_fseek (xmfile, 0x30, XM_SEEK_SET) >= 0)
			{
				XM_fread (&total_frames, 1, 4, xmfile);
				video_time = (total_frames * micro_sec_per_frame + 500) / 1000;
			}
			XM_fclose (xmfile);
			// 标记流时间有效, 
			lpVideoItem->stream_length_valid = 1;
			lpVideoItem->stream_length = (DWORD)video_time;
		}	
		break;
#else
		// 标记流时间有效, 
		lpVideoItem->stream_length_valid = 1;
		lpVideoItem->stream_length = 0;
		break;;
#endif
	} while (0);
	XM_VideoItemUnlock ();
	
	return (DWORD)video_time;
}

// 获取视频项通道对应的文件路径
const char * XM_VideoItemGetVideoPath (unsigned int nVideoChannel)
{
	if(nVideoChannel >= XM_VIDEO_CHANNEL_COUNT)
		return NULL;
	
	//return video_path_name[XM_VIDEOITEM_VOLUME_0][nVideoChannel];
	return video_path_name[XM_VIDEOITEM_VOLUME_0][nVideoChannel][XM_FILE_TYPE_VIDEO][XM_VIDEOITEM_CLASS_NORMAL];
}

// 获取视频项通道对应的文件路径,根据类型
const char * XM_VideoItemGetVideoPathByType(unsigned int nVideoChannel, unsigned char type)
{
	if(nVideoChannel >= XM_VIDEO_CHANNEL_COUNT)
		return NULL;
	
	//return video_path_name[XM_VIDEOITEM_VOLUME_0][nVideoChannel];
	return video_path_name[XM_VIDEOITEM_VOLUME_0][nVideoChannel][type][XM_VIDEOITEM_CLASS_NORMAL];
}

// 获取视频项指定通道的全路径文件名
XMBOOL XM_VideoItemGetVideoFilePath (XMVIDEOITEM *lpVideoItem, unsigned int nVideoChannel, char *lpFileName, int cbFileName)
{
	XMBOOL ret = 0;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	XM_VideoItemLock ();
	do
	{
		if(h == NULL)
			break;
		
		if(lpVideoItem == NULL)
			break;
		
		//if(nVideoChannel >= video_path_count)
		//	break;
		
		if(XM_VideoItemMakeFullPathFileName ((char *)lpVideoItem->video_name, lpVideoItem->volume_index, lpVideoItem->video_channel, lpVideoItem->file_type, lpVideoItem->stream_class, lpFileName, cbFileName) < 0)
			break;
		
		ret = 1;
	} while(0);
	XM_VideoItemUnlock ();

	return ret;
}

// 将视频列表(循环或保护视频列表)中指定序号的视频文件锁定(保护) (保护所有通道)
// 注意：
//		一键保护执行时视频句柄对应的视频文件可能还没有创建(文件系统的目录项还没有创建)
XMBOOL AP_VideoItemLockVideoFile (XMVIDEOITEM *lpVideoItem)
{
	char FileName[VIDEOITEM_MAX_FILE_NAME];
	unsigned int channel;
	XMBOOL ret = 0;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	XM_VideoItemLock ();

	do
	{
		if(h == NULL)
			break;

		if(lpVideoItem == NULL)
			break;
#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2
		// 回放过程禁止锁定、解锁操作
		if(lpVideoItem->video_ref_count)
			break;
#endif
		
		ret = 1;
		// 检查是否是紧急录制类型，无需加锁
		if(lpVideoItem->stream_class == XM_VIDEOITEM_CLASS_URGENT)
			break;
		// 检查是否已锁定(任意一个通道锁定即可)
		if(lpVideoItem->video_lock)
		{
			break;
		}

		memset (FileName, 0, sizeof(FileName));
		XM_VideoItemGetVideoFilePath (lpVideoItem, lpVideoItem->video_channel, FileName, sizeof(FileName));
	
#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2
		if(lpVideoItem->video_update)
		{
			// 文件已更新，更改目录
			char NewFileName[VIDEOITEM_MAX_FILE_NAME];
			if(XM_VideoItemMakeFullPathFileName ((const char *)lpVideoItem->video_name, lpVideoItem->volume_index, lpVideoItem->video_channel, lpVideoItem->file_type,
															 XM_VIDEOITEM_CLASS_MANUAL, NewFileName, sizeof(NewFileName)) < 0)
			{
				ret = 0;
				break; 
			}
			// 文件从NORMAL目录移动到MANUAL目录下
			if(FS_Move (FileName, NewFileName) < 0)
			{
				ret = 0;
			}
			XM_printf ("class rename from %s to %s %s\n", FileName, NewFileName, ret == 1 ? "OK" : "NG");
			if(ret == 0)
				break;
		}		
#else
		// 检查文件是否存在
		if(XM_SetFileAttributes (FileName, XM_FILE_ATTRIBUTE_ARCHIVE | XM_FILE_ATTRIBUTE_READONLY))
		{
			//XM_printf ("%s set readonly OK\n", FileName);
		}
		else
		{
			//XM_printf ("%s set readonly NG\n", FileName);
		}
#endif
		
		
		lpVideoItem->video_lock = 1;
		if(lpVideoItem->stream_class == XM_VIDEOITEM_CLASS_NORMAL)
			lpVideoItem->stream_class = XM_VIDEOITEM_CLASS_MANUAL;
		
		videoitem_dec_count (lpVideoItem, XM_VIDEOITEM_CLASS_NORMAL);
		videoitem_inc_count (lpVideoItem, XM_VIDEOITEM_CLASS_MANUAL);
		// 检查视频项对应的视频磁盘文件是否已创建并关闭
		if(lpVideoItem->video_update)
		{
			// 视频项对应的视频磁盘文件已创建并关闭
			// 减去被锁定文件的空间
			video_item_recycle_space -= lpVideoItem->video_size;
			//XM_printf ("200 recycle - file size(%lld),  recycle_space(%lld)\n",  lpVideoItem->video_size, video_item_recycle_space);
			video_item_locked_space += lpVideoItem->video_size;
			//XM_printf ("locked file size(%lld),  locked_space(%lld)\n",  lpVideoItem->video_size, video_item_locked_space);
			
			videoitem_dec_space (lpVideoItem, XM_VIDEOITEM_CLASS_NORMAL);
			videoitem_inc_space (lpVideoItem, XM_VIDEOITEM_CLASS_MANUAL);
		}

	} while (0);

	XM_VideoItemUnlock ();
	
	if(ret)
	{
		// 将修改及时同步到物理文件系统
		FS_CACHE_Clean ("");
	}

	return (XMBOOL)ret;
}

// 将指定文件名(全路径)的视频锁定
XMBOOL AP_VideoItemLockVideoFileFromFileName (const char *lpVideoFileName)
{
	HANDLE hVideoHandle;
	XMBOOL ret = 0;
	XMVIDEOITEM *video_item = NULL;
	void *keyp = NULL, *valuep = NULL;
	int nVideoChannel;
	
	// 检查是否是临时视频项文件
	if(XM_VideoItemIsTempVideoItemFile (lpVideoFileName))
	{
		return XM_SetFileAttributes (lpVideoFileName, XM_FILE_ATTRIBUTE_ARCHIVE | XM_FILE_ATTRIBUTE_READONLY);
	}
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	// XM_VideoItemLock允许同一个线程内多次调用，只需匹配对应的XM_VideoItemUnlock次数
	XM_VideoItemLock ();
	hVideoHandle = XM_VideoItemGetVideoItemHandle (lpVideoFileName, &nVideoChannel);
	if(hVideoHandle)
	{
		rbtKeyValue(h, hVideoHandle, &keyp, &valuep);
		video_item = (XMVIDEOITEM *)keyp;
	}
	else
	{
		 
	}
	if(video_item)
		ret = AP_VideoItemLockVideoFile (video_item);
	XM_VideoItemUnlock ();
	return ret;
}

// 将视频列表(循环或保护视频列表)中指定序号的视频文件解锁(可循环覆盖)
XMBOOL AP_VideoItemUnlockVideoFile (XMVIDEOITEM *lpVideoItem)
{
	char FileName[VIDEOITEM_MAX_FILE_NAME];
	unsigned int channel;
	XMBOOL ret = 0;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	XM_VideoItemLock ();

	do
	{
		if(h == NULL)
			break;

		if(lpVideoItem == NULL)
			break;
#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2
		// 回放过程禁止锁定、解锁操作
		if(lpVideoItem->video_ref_count)
			break;
#endif

		ret = 1;
		// 检查是否是紧急录制类型，无需加锁
		if(lpVideoItem->stream_class == XM_VIDEOITEM_CLASS_URGENT)
			break;
		// 检查是否已解锁
		if(!lpVideoItem->video_lock)
			break;

		memset (FileName, 0, sizeof(FileName));
		XM_VideoItemGetVideoFilePath (lpVideoItem, lpVideoItem->video_channel, FileName, sizeof(FileName));
		// 检查文件是否存在
#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2
		if(lpVideoItem->video_update)
		{
			// 文件已创建并关闭
			char NewFileName[VIDEOITEM_MAX_FILE_NAME];
			if(XM_VideoItemMakeFullPathFileName ((const char *)lpVideoItem->video_name, lpVideoItem->volume_index, lpVideoItem->video_channel, lpVideoItem->file_type,
															 XM_VIDEOITEM_CLASS_NORMAL, NewFileName, sizeof(NewFileName)) < 0)
			{
				ret = 0;
				break; 
			}
			// 文件从MANUAL目录移动到NORMAL目录下
			XM_SetFileAttributes (FileName, XM_FILE_ATTRIBUTE_ARCHIVE);
			if(FS_Move (FileName, NewFileName) < 0)
				ret = 0;
			else
			    ret = 1;
			
			XM_printf ("unlock class rename from %s to %s %s\n", FileName, NewFileName, ret == 1 ? "OK" : "NG");			
			if(ret == 0)
				break;
		}
#else
		XM_SetFileAttributes (FileName, XM_FILE_ATTRIBUTE_ARCHIVE);
#endif
		lpVideoItem->video_lock = 0;
		if(lpVideoItem->stream_class == XM_VIDEOITEM_CLASS_MANUAL)
			lpVideoItem->stream_class = XM_VIDEOITEM_CLASS_NORMAL;
		// 检查视频项对应的视频磁盘文件是否已创建并关闭
		videoitem_dec_count (lpVideoItem, XM_VIDEOITEM_CLASS_MANUAL);
		videoitem_inc_count (lpVideoItem, XM_VIDEOITEM_CLASS_NORMAL);
		if(lpVideoItem->video_update)
		{
			// 累加被锁定文件的空间
			video_item_recycle_space += lpVideoItem->video_size;
			//XM_printf ("300 recycle + file size(%lld),  recycle_space(%lld)\n",  lpVideoItem->video_size, video_item_recycle_space);
			video_item_locked_space -= lpVideoItem->video_size;
			//XM_printf ("unlocked file size(%lld), locked_space(%lld)\n",  lpVideoItem->video_size, video_item_locked_space);
			
			videoitem_dec_space (lpVideoItem, XM_VIDEOITEM_CLASS_MANUAL);
			videoitem_inc_space (lpVideoItem, XM_VIDEOITEM_CLASS_NORMAL);
		}
	} while (0);

	XM_VideoItemUnlock ();
	
	if(ret)
	{
		// 将修改及时同步到物理文件系统
		FS_CACHE_Clean ("");
	}
	return ret;
}

XMBOOL AP_VideoItemUnlockVideoFileFromFileName (const char *lpVideoFileName)
{
	HANDLE hVideoHandle;
	XMBOOL ret = 0;
	XMVIDEOITEM *video_item = NULL;
	void *keyp = NULL, *valuep = NULL;
	int nVideoChannel;
	
	// 检查是否是临时视频项文件
	if(XM_VideoItemIsTempVideoItemFile (lpVideoFileName))
	{
		return XM_SetFileAttributes (lpVideoFileName, XM_FILE_ATTRIBUTE_ARCHIVE);
	}
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	// XM_VideoItemLock允许同一个线程内多次调用，只需匹配对应的XM_VideoItemUnlock次数
	XM_VideoItemLock ();
	hVideoHandle = XM_VideoItemGetVideoItemHandle (lpVideoFileName, &nVideoChannel);
	if(hVideoHandle)
	{
		rbtKeyValue(h, hVideoHandle, &keyp, &valuep);
		video_item = (XMVIDEOITEM *)keyp;
	}
	if(video_item)
		ret = AP_VideoItemUnlockVideoFile (video_item);
	XM_VideoItemUnlock ();
	return ret;
}


// 获取下一个可循环覆盖的视频文件项句柄
HANDLE XM_VideoItemGetNextCircularVideoItemHandle (unsigned int volume_index, unsigned int file_type, unsigned int stream_class_bitmask)
{
	RbtIterator i;
	void *keyp, *valuep;
	XMVIDEOITEM *video_item;
	RbtIterator next = NULL;
	
	if(volume_index >= video_volume_count)
		return NULL;
	if(file_type >= XM_FILE_TYPE_COUNT)
		return NULL;

	XM_VideoItemLock ();
	i = NULL;

	do
	{
		if(h == NULL)
			break;

		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			// 检查其属性(非锁定属性)
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = valuep;
			
			// 检查卷及文件类型是否一致
			if(video_item->file_type != file_type)
				continue;
			if(video_item->volume_index != volume_index)
				continue;
			if( !((1 << video_item->stream_class) & stream_class_bitmask) )		// 检查流类型是否包含
				continue;

			// 视频项对应的视频文件已创建，且没有锁定
			// if(!video_item->video_lock && video_item->video_update)
			// 视频项对应的视频文件已创建，且为普通录制类型
			//if(video_item->stream_class == XM_VIDEOITEM_CLASS_NORMAL && video_item->video_update)
			if(video_item->video_update)
			{
				next = i;
				// 没有通道锁定
				//break;
			}
		}
	} while (0);

	XM_VideoItemUnlock ();

	return next;
}

// 检查视频项基本服务是否已开启。若开启，则允许视频记录
XMBOOL XM_VideoItemIsBasicServiceOpened (void)
{
	return (XMBOOL)basic_service_opened;
}

// 检查视频项服务是否已开启
XMBOOL XM_VideoItemIsServiceOpened (void)
{
	return (XMBOOL)videoitem_service_opened;
}

// 获取简单视频项(只包含1路视频)的通道序号
// >= 0  返回的通道序号
// < 0   无效的句柄或者视频项句柄是一个复合视频项(多个通道的组合)
int XM_VideoItemGetVideoChannel (HANDLE hVideoItemHandle)
{
	int ret = -1;
	RbtIterator i;
	void *keyp, *valuep;
	XMVIDEOITEM *video_item;
	unsigned int channel;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return -1;	
	if(hVideoItemHandle == NULL)
		return -1;

	XM_VideoItemLock ();
	do 
	{
		if(h == NULL)
			break;
		i = (RbtIterator)hVideoItemHandle;
		if(i == NULL)
			break;
		rbtKeyValue(h, i, &keyp, &valuep);
		video_item = (XMVIDEOITEM *)valuep;

		ret = video_item->video_channel;

	} while (0);

	XM_VideoItemUnlock ();

	return ret;
}

// 保证所有通道序号更新的一致性
static unsigned int videoitem_get_serial_number (unsigned int volume_index, unsigned int file_type, unsigned int video_channel)
{
	unsigned int max_id;
	if(serial_number_type == SERIAL_NUMBER_TYPE_SHARE)
	{
		// 不同录制类型共享同一个序号
		file_type = 0;
	}
	
	if(video_channel == 0)
	{
		// 通道0
		// 计算所有通道的最大值max_id，然后将max_id + 1更新到通道0
		max_id = 0;
		for (unsigned int i = 0; i < video_path_count; i ++)
		{
			if(video_item_serial_number[volume_index][file_type][i] > max_id)
				max_id = video_item_serial_number[volume_index][file_type][i];
		}
		video_item_serial_number[volume_index][file_type][0] = max_id + 1;
	}
	else
	{
		// 非0通道
		// 首先累加当前通道的序号值
		video_item_serial_number[volume_index][file_type][video_channel] ++;
		// 检查每个通道的序号值，计算当前最大的序号值
		max_id = 0;
		for (unsigned int i = 0; i < video_channel; i ++)
		{
			if(video_item_serial_number[volume_index][file_type][i] > max_id)
				max_id = video_item_serial_number[volume_index][file_type][i];
		}
		// 将最大序号写入到当前通道
		if(max_id > video_item_serial_number[volume_index][file_type][video_channel])
			video_item_serial_number[volume_index][file_type][video_channel] = max_id;
	}
	return video_item_serial_number[volume_index][file_type][video_channel];
}

// 根据视频项属性(通道号，创建时间，文件类型，视频项分类，保存的卷序号)创建一个视频项句柄
//		1) 创建一个位于卡0上的通道0的紧急录制类型的录像视频项
//			XM_VideoItemCreateVideoItemHandleEx (XM_VIDEO_CHANNEL_0, create_time, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_URGENT, XM_VIDEOITEM_VOLUME_0);
//		2) 创建一个位于卡0上的通道1的普通录制类型的照片视频项
//			XM_VideoItemCreateVideoItemHandleEx (XM_VIDEO_CHANNEL_1, create_time, XM_FILE_TYPE_PHOTO, XM_VIDEOITEM_CLASS_NORMAL, XM_VIDEOITEM_VOLUME_0);
//		3) 创建一个位于卡1上的通道1的手动录制类型的视频视频项
//			XM_VideoItemCreateVideoItemHandleEx (XM_VIDEO_CHANNEL_1, create_time, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_MANUAL, XM_VIDEOITEM_VOLUME_1);
// 返回值
//		0			失败
//		非0值		视频项句柄
HANDLE XM_VideoItemCreateVideoItemHandleEx (unsigned int video_channel,		// 通道号
														  XMSYSTEMTIME *videoitem_create_time, 		// 视频项创建时间
														  unsigned int file_type,			//	视频项文件类型(视频或照片)	
														  unsigned int videoitem_class, 	// 视频项类型(正常、手动或者紧急)
														  unsigned int video_volume_index	// 保存的SD卡卷序号(从0开始)
														  )
{
	NodeType * i;
	XMVIDEOITEM *video_item;
	RbtStatus status;
	char file_name[VIDEOITEM_MAX_FULL_NAME + 1];		// 20180430_164001_00000018_00N.AVI
	//char *ch;
	//int loop;
	XMINT64 max_number;
	int ret = -1;
	XMSYSTEMTIME create_time;

	//XMINT64  video_number;
	
	VIDEOFILETOREMOVE VideoFileToRemove;
	
	int		 last_video_item_count;
	
	if(video_channel >= XM_VIDEO_CHANNEL_COUNT)
		return NULL;
	if(file_type >= XM_FILE_TYPE_COUNT)
		return NULL;
	if(videoitem_class >= XM_VIDEOITEM_CLASS_COUNT)
		return NULL;
	if(videoitem_create_time && is_valid_time(videoitem_create_time) < 0)
		return NULL;
	if(video_volume_index >= video_volume_count)
		return NULL;
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return NULL;
	
	if(videoitem_create_time)
		create_time = *videoitem_create_time;
	else
		XM_GetLocalTime (&create_time);
		
	// 20180522 缺省将毫秒时间置为0, 但同名视频文件存在时，累加该值避免覆盖旧的视频文件
	//videoitem_create_time->wMilliSecond = 0;
	create_time.wMilliSecond = 0;

	// 检查视频项服务是否已开启

	XM_VideoItemLock ();
	
	if(video_volume_valid[video_volume_index] == 0)
	{
		// 写入卷无效
		if(automatic_safe_save)
		{
			// 搜索下一个安全的可用卷
			unsigned int i;
			for (i = 0; i < video_volume_count; i ++)
			{
				if(video_volume_valid[i] && i != video_volume_index)
					break;
			}
			if(i == video_volume_count)
			{
				XM_VideoItemUnlock ();
				return NULL;
			}
			// 使用安全的卷
			video_volume_index = i;
		}
		else
		{
			XM_VideoItemUnlock ();
			return NULL;
		}
	}
	
	memset (file_name, 0, sizeof(file_name));

	i = NULL;
	
	video_item = NULL;
	
	last_video_item_count = video_item_count;

	do
	{
		if(h == NULL)
			break;
		
		// 检查视频项数量是否已达到系统定义的最大值
		if(video_item_count >= MAX_VIDEO_ITEM_COUNT)
		{
			if(videoitem_debug)
				XM_printf ("CreateVideoItemHandleEx NG, Video Item Exceed Maximum count %d\n", MAX_VIDEO_ITEM_COUNT);
			// 检查并回收可循环项
			//XM_VideoItemUnlock ();
			//XM_VideoItemMonitorAndRecycleGarbage (0);
			//XM_VideoItemLock ();
			if(video_item_count == last_video_item_count)		// 回收后无变化，异常退出
			{
				XM_printf ("can't recycle video item (%d)\n", video_item_count);
				break;
			}
			if(video_item_count >= MAX_VIDEO_ITEM_COUNT)
			{
				XM_printf ("recycle NG, video item (%d)\n", video_item_count);
				break;				
			}
			//XM_printf ("recycle OK, video item (%d)\n", video_item_count);
		}

		//XM_printf ("XM_VideoItemCreateVideoItemHandle (%s)\n", file_name);

		// 创建新节点
		video_item = (XMVIDEOITEM *)videoitem_malloc ();
		if(video_item == NULL)
		{
			//XM_printf ("videoitem_malloc XMVIDEOITEM failed\n");
			break;
		}
		memset (video_item, 0, sizeof(XMVIDEOITEM));

		video_item->serial_number = videoitem_get_serial_number (video_volume_index, file_type, video_channel);
		// 获取视频项命名
		ret = XM_MakeFileNameFromCurrentDateTimeEx (&create_time, video_channel, file_type, videoitem_class, video_volume_index, 
																  video_item->serial_number,
																  file_name, sizeof(file_name));
		if(ret < 0)
		{
			videoitem_free (video_item);
			break;
		}
		video_item->video_channel = (unsigned char)(video_channel);
		if(videoitem_class == XM_VIDEOITEM_CLASS_MANUAL)
			video_item->video_lock = 1;
		else
			video_item->video_lock = 0;
		video_item->video_size = 0;
		video_item->video_update = 0;
		video_item->file_type = file_type;
		video_item->stream_class = videoitem_class;
		video_item->initial_class = videoitem_class;
		memcpy (video_item->video_name, file_name, sizeof(file_name));

		// insert in red-black tree
		// XM_printf ("video_name=%s\n", videoitem_name);
		status = rbtInsert(h, video_item->video_name, video_item);
		if (status != RBT_STATUS_OK)
		{
			//XM_printf("fail: status = %d\n", status);
			videoitem_free (video_item);
			video_item = NULL;
			break;
		}	
		i = (NodeType *)rbtFind(h, file_name);
		XM_ASSERT (i);
		video_item_count ++;
		videoitem_inc_count (video_item, videoitem_class);
	} while (0);

	XM_VideoItemUnlock ();
	
	return i;	
}

// 创建一个新的视频项句柄
//   (缺省为视频文件，普通分类，卷0)
// nVideoChannel = 0, 1, 2, 3 ... 表示创建一个指定通道的视频项
HANDLE XM_VideoItemCreateVideoItemHandle (unsigned int VideoChannel, XMSYSTEMTIME *filetime)
{
	return XM_VideoItemCreateVideoItemHandleEx (VideoChannel, filetime, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_NORMAL, XM_VIDEOITEM_VOLUME_0);
}

// 删除一个视频项句柄(包括删除其所有通道的视频文件)
XMBOOL XM_VideoItemDeleteVideoItemHandle (HANDLE hVideoItemHandle, VIDEOFILETOREMOVE *lpVideoFileToRemove)
{
	RbtIterator i;
	void *keyp, *valuep;
	XMVIDEOITEM *video_item;
	unsigned int channel;
	char file_name[VIDEOITEM_MAX_FILE_NAME];
	XMBOOL ret;
	
	XM_VIDEOITEM_EVENT videoitem_event;
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;	
	
	memset (file_name, 0, sizeof(file_name));
	ret = 0;
	
	if(lpVideoFileToRemove == NULL)
	{
		//XM_printf ("lpVideoFileToRemove == NULL\n");
		return 0;
	}
	
	XM_VideoItemLock ();
	
	lpVideoFileToRemove->count = 0;

	do
	{
		if(h == NULL)
			break;

		i = (RbtIterator)hVideoItemHandle;
		if(i == NULL)
			break;

		rbtKeyValue(h, i, &keyp, &valuep);
		video_item = (XMVIDEOITEM *)valuep;

		// 检查视频项对应的磁盘文件是否已创建并关闭
		if(!video_item->video_update)
		{
			XM_printf ("Can't delete video(%s, ref_count=%d) which don't updated before\n", video_item->video_name, video_item->video_ref_count);
			break;
		}
		
		// 检查视频项是否被引用中(如正在播放)
		if(video_item->video_ref_count)
		{
			XM_printf ("Can't delete video item which mark usage now\n");
			break;
		}
		
		{
			if( XM_VideoItemGetVideoFilePath(video_item, video_item->video_channel, file_name, VIDEOITEM_MAX_FILE_NAME) )
			{
				// 修改文件属性
				if(video_item->video_lock)
				{
					XM_SetFileAttributes(file_name, XM_FILE_ATTRIBUTE_ARCHIVE);
				}
				strcpy (lpVideoFileToRemove->file_name[lpVideoFileToRemove->count], file_name);
				// 暂时
				lpVideoFileToRemove->volume_index[lpVideoFileToRemove->count] = video_item->volume_index;
				lpVideoFileToRemove->channel[lpVideoFileToRemove->count] = video_item->video_channel;
				lpVideoFileToRemove->count ++;
				memset (&videoitem_event, 0, sizeof(videoitem_event));
				videoitem_event.event = XM_EVENT_VIDEOITEM_DELETE;
				videoitem_event.channel = (unsigned char)video_item->video_channel;
				strcpy ((char *)videoitem_event.file_name, file_name);
				XM_PostVideoItemEvent (videoitem_event.event, &videoitem_event, sizeof(videoitem_event));

			}
		}
		
		if(video_item->stream_class == XM_VIDEOITEM_CLASS_NORMAL)
		{
			videoitem_dec_space (video_item, XM_VIDEOITEM_CLASS_NORMAL);
			videoitem_dec_count (video_item, XM_VIDEOITEM_CLASS_NORMAL);
		}
		else if(video_item->stream_class == XM_VIDEOITEM_CLASS_MANUAL)
		{
			videoitem_dec_space (video_item, XM_VIDEOITEM_CLASS_MANUAL);
			videoitem_dec_count (video_item, XM_VIDEOITEM_CLASS_MANUAL);
		}
		else if(video_item->stream_class == XM_VIDEOITEM_CLASS_URGENT)
		{
			videoitem_dec_space (video_item, XM_VIDEOITEM_CLASS_URGENT);
			videoitem_dec_count (video_item, XM_VIDEOITEM_CLASS_URGENT);
		}

		// 检查是否是可循环使用视频项
		if(video_item->video_lock)
		{
			// 锁定项
			video_item_locked_space -= video_item->video_size;
			//XM_printf ("unlocked file size(%lld),  locked_space(%lld)\n",  video_item->video_size, video_item_locked_space);
		}
		else //if(!video_item->video_lock)
		{
			// 减去可回收(循环)视频文件字节大小
			video_item_recycle_space -= video_item->video_size;
			//XM_printf ("400 recycle - file size(%lld),  recycle_space(%lld)\n",  video_item->video_size, video_item_recycle_space);
		}

		video_item_count --;
		XM_ASSERT (video_item_count >= 0);

		videoitem_free (valuep);
		rbtErase (h, (RbtIterator)i);
		ret = 1;
	} while (0);

	XM_VideoItemUnlock ();

	return ret;
}

XMBOOL XM_VideoItemDeleteVideoItemHandleFromFileNameEx (const char *lpVideoFileName, int bPostFileListUpdateMessage)
{
	HANDLE hVideoHandle;
	XMBOOL ret = 0;
	int nVideoChannel;
	VIDEOFILETOREMOVE VideoFileToRemove;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	memset (&VideoFileToRemove, 0, sizeof(VideoFileToRemove));
	// XM_VideoItemLock允许同一个线程内多次调用，只需匹配对应的XM_VideoItemUnlock次数
	XM_VideoItemLock ();
	hVideoHandle = XM_VideoItemGetVideoItemHandle (lpVideoFileName, &nVideoChannel);
	if(hVideoHandle)
	{
		ret = XM_VideoItemDeleteVideoItemHandle (hVideoHandle, &VideoFileToRemove);
	}
	XM_VideoItemUnlock ();
	
	if(ret)
	{
		ret = remove_video_files(&VideoFileToRemove, bPostFileListUpdateMessage);
	}
	return ret;
	
}

XMBOOL XM_VideoItemDeleteVideoItemHandleFromFileName (const char *lpVideoFileName)
{
	return XM_VideoItemDeleteVideoItemHandleFromFileNameEx (lpVideoFileName, 1);
}

// 指定通道的视频文件已创建/写入并关闭，需更新其对应的视频句柄项中的信息
//
// 此函数仅在H264 Cache文件系统线程关闭文件时调用，不会引起H264编码线程的阻塞，
//
// 因此在此处可加入磁盘剩余空间检查、可循环录制文件释放、可录时间报警检查 等操作
//
// *** 新录制的视频文件在录制过程中可能被锁定(一键锁定功能)，
//		 若存在锁定，需要检查文件属性是否已设置为只读属性。
//	pNewVideoItemHandle	表示是否新句柄已重新创建
XMBOOL XM_VideoItemUpdateVideoItemHandle (HANDLE hVideoItemHandle, int nVideoChannel, unsigned int cbVideoFileSize,
														XMSYSTEMTIME *create_time)
{
	RbtIterator i;
	void *keyp, *valuep;
	XMVIDEOITEM *video_item;
	char file_name[VIDEOITEM_MAX_FILE_NAME];
	XMBOOL ret;
	DWORD size = 0;
	DWORD attr;
	DWORD cluster_size = 0;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	XM_VideoItemLock ();

	ret = 0;

	do
	{
		if(h == NULL)
			break;

		// 查找文件对应的视频项句柄(已创建)

		i = (RbtIterator)hVideoItemHandle;
		if(i == NULL)
			break;

		rbtKeyValue(h, i, &keyp, &valuep);
		video_item = (XMVIDEOITEM *)valuep;


		// 检查该项对应的通道视频文件并更新信息
		memset (file_name, 0, sizeof(file_name));
		if(XM_VideoItemGetVideoFilePath (video_item, nVideoChannel, file_name, VIDEOITEM_MAX_FILE_NAME))
		{
			// 检查该通道是否已设置
			char FileName[VIDEOITEM_MAX_FILE_NAME];
			char NewFileName[VIDEOITEM_MAX_FILE_NAME];

			// 读取文件的文件字节大小及文件属性信息
			// 检查通道对应的文件是否创建
			if(video_item->video_channel == (nVideoChannel))
			{
				video_item->video_update |= (1 << nVideoChannel);
				
				// 更新视频项文件的创建时间
				video_item->video_create_time = *create_time;
				
				size = cbVideoFileSize;
	
				// 累计文件大小
				// 计算按族分配的文件大小
				cluster_size = get_fat_cluster_size_from_volume (video_item->volume_index);
				if(cluster_size)
					size = ((size + cluster_size - 1) / cluster_size) * cluster_size;
				
				video_item->file_size = cbVideoFileSize;
				// 20141227 ZhuoYongHong
				// 修复bug，未更新视频项的文件大小
				video_item->video_size += size;

				// 一键锁定功能会导致新创建的视频项加锁
				// 检查视频项是否已锁定。
#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2
				// 检查流类型是否存在修改. 若存在，修改目录
				if(video_item->initial_class != video_item->stream_class)
				{
					memset (FileName, 0, sizeof(FileName));
					memset (NewFileName, 0, sizeof(NewFileName));
					XM_VideoItemMakeFullPathFileName ((const char *)video_item->video_name, video_item->volume_index, video_item->video_channel, video_item->file_type,
																	 video_item->initial_class, FileName, sizeof(FileName));
					XM_VideoItemMakeFullPathFileName ((const char *)video_item->video_name, video_item->volume_index, video_item->video_channel, video_item->file_type,
																	 video_item->stream_class, NewFileName, sizeof(NewFileName));
					XM_SetFileAttributes (FileName, XM_FILE_ATTRIBUTE_ARCHIVE);
					if(FS_Move (FileName, NewFileName) < 0)
						ret = 0;
					else
					{
						ret = 1;
						if(video_item->video_lock)
							XM_SetFileAttributes (NewFileName, XM_FILE_ATTRIBUTE_ARCHIVE|XM_FILE_ATTRIBUTE_READONLY);
					}
					
					XM_printf ("class rename from %s to %s %s\n", FileName, NewFileName, ret == 1 ? "OK" : "NG");			
					if(ret == 0)
						break;
				}
				if(video_item->video_lock)
				{
					// 累加已锁定视频文件字节大小
					video_item_locked_space += size;
					//XM_printf ("locked file (%s) , size(%d),  locked_space(%lld)\n",  file_name, size, video_item_locked_space);
					videoitem_inc_space (video_item, XM_VIDEOITEM_CLASS_MANUAL);
				}
				else
				{
					// 累加可回收(循环)视频文件字节大小
					video_item_recycle_space += size;
					//XM_printf ("500 recycle + file size(%d),  recycle_space(%lld)\n",  size, video_item_recycle_space);
					videoitem_inc_space (video_item, XM_VIDEOITEM_CLASS_NORMAL);
					
				}
#else
				attr = XM_GetFileAttributes (file_name);
				if(video_item->video_lock)
				{
					// attr = XM_GetFileAttributes (file_name);
					if(attr == (DWORD)(-1))
					{
						// 文件操作异常
						//XM_printf ("Get File Attr(%s) NG\n", file_name);
						break;				
					}
					// 已锁定，检查文件属性是否已设置为只读属性
					if( !(attr & D_RDONLY) )
					{
						//XM_printf ("XM_SetFileAttributes %s\n", file_name);
						XM_SetFileAttributes (file_name, XM_FILE_ATTRIBUTE_ARCHIVE | XM_FILE_ATTRIBUTE_READONLY);
					}
					
					// 累加已锁定视频文件字节大小
					video_item_locked_space += size;
					//XM_printf ("locked file (%s) , size(%d),  locked_space(%lld)\n",  file_name, size, video_item_locked_space);
					videoitem_inc_space (video_item, XM_VIDEOITEM_CLASS_MANUAL);
				}
				else if( (attr != (DWORD)(-1)) && (attr & D_RDONLY) )
				{
					// 只读文件 (临时视频项被设置为只读属性)
					video_item->video_lock = 1;
					// 累加已锁定视频文件字节大小
					video_item_locked_space += size;		
					//XM_printf ("locked file (%s) , size(%d),  locked_space(%lld)\n",  file_name, size, video_item_locked_space);
					videoitem_inc_space (video_item, XM_VIDEOITEM_CLASS_MANUAL);
				}
				else
				{
					// 未锁定
					/*
					size = XM_GetFileSize (file_name);
					if(size == (DWORD)(-1))
					{
						// 文件操作异常
						XM_printf ("Get File Size(%s) NG\n", file_name);
						break;				
					}
					XM_printf ("size=%d, cbVideoFileSize=%d\n", size, cbVideoFileSize);*/
					//size = cbVideoFileSize;
	
					// 累计文件大小
					// 计算按族分配的文件大小
					//cluster_size = get_fat_cluster_size_from_channel(nVideoChannel);
					//size = ((size + cluster_size - 1) / cluster_size) * cluster_size;
					//video_item->video_size += size;
							
					// 累加可回收(循环)视频文件字节大小
					video_item_recycle_space += size;
					//XM_printf ("500 recycle + file size(%d),  recycle_space(%lld)\n",  size, video_item_recycle_space);
					videoitem_inc_space (video_item, XM_VIDEOITEM_CLASS_NORMAL);
				}
#endif
				
			}
		}
		else
		{
			break;
		}

		ret = 1;
	} while (0);

	XM_VideoItemUnlock ();

	// 不要在此处调用 XM_VideoItemMonitorAndRecycleGarbage, 会阻塞文件关闭等操作
	// 加入磁盘剩余空间检查、可循环录制文件释放、可录时间报警检查
	//XM_VideoItemMonitorAndRecycleGarbage (0);

	return ret;
}

XMBOOL XM_VideoItemUpdateVideoItemFromTempName (const char *lpVideoFileName, unsigned int cbVideoFileSize, 
																XMSYSTEMTIME *create_time)
{
	//HANDLE hVideoHandle;
	XMBOOL ret = 0;
	int nVideoChannel;
	XMVIDEOITEM *pVideoItem;
	char ItemName[64];
	XMSYSTEMTIME CurrTime;
	HANDLE hVideoItem;
	char *newName;
	XMBOOL read_only = 0;

	// 检查是否是临时视频项文件
	if(!XM_VideoItemIsTempVideoItemFile (lpVideoFileName))
	{
		//XM_printf ("XM_VideoItemUpdateVideoItemFromTempName NG, illegal temp file (%s)\n", lpVideoFileName);
		return 0;
	}
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
	{
		XM_printf ("UpdateVideoItemFromTempName NG, Service closed\n");
		return 0;
	}
	
	// 获取RTC时间
	XM_GetLocalTime(&CurrTime);
		
	// 从临时视频项文件名解析出视频通道号
	nVideoChannel = -1;
	if(!XM_VideoItemGetVideoChannelFromTempVideoItemName (lpVideoFileName, &nVideoChannel))
	{
		return 0;
	}
	if(nVideoChannel == (int)(-1))
		return 0;
		
	// 创建一个新的视频项
	XM_VideoItemLock ();
	hVideoItem = XM_VideoItemCreateVideoItemHandle (nVideoChannel, create_time);	
	if(hVideoItem == NULL)
	{
		XM_VideoItemUnlock ();
		return 0;
	}
		
	pVideoItem = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
	if(pVideoItem == NULL)
	{
		XM_VideoItemUnlock ();
		return 0;
	}
		
	// 获取视频项的全路径名
	if(!XM_VideoItemGetVideoFilePath (pVideoItem, nVideoChannel, ItemName, sizeof(ItemName)))
	{
		XM_VideoItemUnlock ();
		return 0;
	}
	
	newName = strrchr (ItemName, '\\');
	if(newName == NULL)
	{
		//XM_VideoItemUnlock ();
		//return 0;
		newName = ItemName;
	}
	else
	{
		newName = newName + 1;
	}
		
	// 将视频文件名从临时视频项文件名更名为ItemName
	// 检查是否是只读文件
	DWORD attr = XM_GetFileAttributes(lpVideoFileName);
	read_only = 0;
	if(attr == (DWORD)(-1))
	{
	}
	else
	{
		if(attr & D_RDONLY)
		{
			// 只读文件, 已锁定
			read_only = 1;
			XM_SetFileAttributes (lpVideoFileName, XM_FILE_ATTRIBUTE_ARCHIVE);
		}
	}
	// XM_rename在只读文件改名时会失败
	if(XM_rename (lpVideoFileName, newName))
	{
		XM_VideoItemUnlock ();
		XM_printf ("XM_rename from (%s) to (%s) NG\n", lpVideoFileName, ItemName);
		return 0;		
	}
	if(read_only)
	{
		// 恢复锁定
		XM_SetFileAttributes (ItemName, XM_FILE_ATTRIBUTE_ARCHIVE | XM_FILE_ATTRIBUTE_READONLY);
	}
	XM_VideoItemUnlock ();
	
	FS_CACHE_Clean("");
	
	// 更名成功
	ret =  XM_VideoItemUpdateVideoItemFromFileName (ItemName, cbVideoFileSize, create_time);	
	
	// 检查是否是停车监控模式
	if(XMSYS_GSensorCheckParkingCollisionStartup())
	{
		// 锁定
		XMBOOL success = AP_VideoItemLockVideoFileFromFileName  (ItemName);
		XM_printf ("parking monitor, lock %s %s\n", ItemName, success ? "OK" : "NG");
	}
	
	XM_printf ("update from %s to %s, ret = %d\n", lpVideoFileName, ItemName, ret);
				  
	return ret;
}

XMBOOL XM_VideoItemUpdateVideoItemFromFileName (const char *lpVideoFileName, unsigned int cbVideoFileSize, 
																XMSYSTEMTIME *create_time)
{
	HANDLE hVideoHandle;
	XMBOOL ret = 0;
	int nVideoChannel;
	DWORD dwCreateTime = 0;
	DWORD dwVideoTime = 0;
	XMVIDEOITEM *video_item;
	//XMSYSTEMTIME CurrTime;
	int to_delete = 0;		// 删除录像时间小于1秒的视频文件
	
	//unsigned int ticket = XM_GetTickCount ();
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
		
	// XM_VideoItemLock允许同一个线程内多次调用，只需匹配对应的XM_VideoItemUnlock次数
	XM_VideoItemLock ();
	hVideoHandle = XM_VideoItemGetVideoItemHandle (lpVideoFileName, &nVideoChannel);
	if(hVideoHandle)
	{	
		ret = XM_VideoItemUpdateVideoItemHandle (hVideoHandle, nVideoChannel, cbVideoFileSize, create_time);
		if(ret == 1)
		{
			video_item = AP_VideoItemGetVideoItemFromHandle (hVideoHandle);
			if(video_item && (video_item->video_channel == (nVideoChannel)) )
			{
				XMSYSTEMTIME *create_time;
				// 格式化输出 时间
				create_time = &video_item->video_create_time;
				dwCreateTime  = create_time->bSecond & 0x3F;
				dwCreateTime |= (create_time->bMinute & 0x3F) << 6;
				dwCreateTime |= (create_time->bHour & 0x1F) << 12;
				dwCreateTime |= (create_time->bDay & 0x1F) << 17;
				dwCreateTime |= (create_time->bMonth & 0x0F) << 22;
				dwCreateTime |= ((create_time->wYear - 2014) & 0x1F) << 26;
#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2
#else
				// 保护位
				if(video_item->video_lock)
					dwCreateTime |= 0x80000000;
#endif
				
				// 获取录像时间长度(单位 秒)
				dwVideoTime = AP_VideoItemGetVideoStreamSize (video_item, nVideoChannel);
				
				// 删除录像时间小于1秒的视频文件
				if(dwVideoTime < 1)
				{
					// 20170319 不在Cache任务中删除小于1秒的视频, 由RecycleTask自动扫描并删除小于1秒的视频
					//		避免Cache任务中执行删除操作导致Cache写入被阻塞
					// to_delete = 1; 
				}
				else
				{
					XM_printf ("UPDATE %s, ch=%d, videotime=%d s, %d/%d/%d-%d:%d:%d\n",  
							  lpVideoFileName, nVideoChannel, dwVideoTime, 
							  create_time->wYear, 
							  create_time->bMonth,
							  create_time->bDay,
							  create_time->bHour,
							  create_time->bMinute,
							  create_time->bSecond);
				}
			}
		}
	}
	else
	{
		//XM_printf ("can't find video item handle\n");
	}

	XM_VideoItemUnlock ();

	//XM_printf ("XM_VideoItemUpdateVideoItemFromFileName (%s) %s\n", lpVideoFileName, ret ? "Success" : "failed");
	//XM_printf ("Update %s, %d\n", lpVideoFileName, XM_GetTickCount() - ticket);
	
	if(ret == 1)
	{
		// 视频项更新成功
		unsigned char ch;
		unsigned char type;
		unsigned char file_name[VIDEOITEM_MAX_FULL_NAME + 1];
		if(to_delete)
		{
			XM_printf ("delete %s, which time less than 1s\n", lpVideoFileName);
			XM_VideoItemDeleteVideoItemHandleFromFileNameEx (lpVideoFileName, 0);
		}
		else
		{
			memset (file_name, 0, sizeof(file_name));
			if(extract_fullpath_media_filename ((unsigned char *)lpVideoFileName, &ch, &type, file_name, sizeof(file_name)) == 0)
			{
				// 20170702 更新列表操作已关闭
				// 此操作因为需求读取每个视频文件来获取其视频录像时间而导致卡访问效率下降
				XMSYS_MessageSocketFileListUpdate (ch, type, file_name, dwCreateTime, dwVideoTime, 1);
			}
		}
	}

	// 加入磁盘剩余空间检查、可循环录制文件释放、可录时间报警检查
	//XM_VideoItemMonitorAndRecycleGarbage (0);

	// 延迟文件删除的操作, 避免增加磁盘读写的负载.
	delay_cutoff_flag = 1;
	
	return ret;
}

// 设置视频项的可用文件数量配额, 可以单独为每个卷，每种文件类型及每种记录类型定义数量配额
// 返回值
//		0		设置成功
//		-1		设置失败
int XM_VideoItemSetItemCountQuota (unsigned int volumn_index, 	// SD卡卷序号(从0开始)
											  unsigned int file_type,		// 文件类型(视频或照片)
											  unsigned int stream_class, 	// 视频项分类(正常、手动或紧急)
											  unsigned int quota				// 允许的可用项目文件数量, 0 表示无限制
											)
{
	if(volumn_index >= XM_VIDEOITEM_VOLUME_COUNT)
		return -1;
	if(file_type >= XM_FILE_TYPE_COUNT)
		return -1;
	if(stream_class >= XM_VIDEOITEM_CLASS_COUNT)
		return -1;
	XM_VideoItemLock ();
	item_count_quota[volumn_index][file_type][stream_class] = quota;
	XM_VideoItemUnlock ();
	return 0;
}

// 设置视频项的磁盘空间占用配额, 为"指定卷上的指定文件类型的指定记录类型"定义空间配额
// 返回值
//		0		设置成功
//		-1		设置失败
int XM_VideoItemSetDiskSpaceQuotaLevelStreamClass (unsigned int volumn_index, 	// SD卡卷序号(从0开始)
																	unsigned int file_type, 		// 文件类型(视频或照片)
																	unsigned int stream_class, 	// 视频项分类(正常、手动或紧急)
																	XMINT64 quota						//	允许的磁盘空间占用字节, 0表示无限制
																	)
{
	if(volumn_index >= XM_VIDEOITEM_VOLUME_COUNT)
		return -1;
	if(file_type >= XM_FILE_TYPE_COUNT)
		return -1;
	if(stream_class >= XM_VIDEOITEM_CLASS_COUNT)
		return -1;
	XM_VideoItemLock ();
	disk_space_quota_level_class[volumn_index][file_type][stream_class] = quota;
	XM_VideoItemUnlock ();
	return 0;
}

// 设置视频项的磁盘空间占用配额, 为"指定卷上的指定文件类型"(包含所有录制分类)定义空间配额
// 返回值
//		0		设置成功
//		-1		设置失败
int XM_VideoItemSetDiskSpaceQuotaLevelFileType (unsigned int volumn_index, 	// SD卡卷序号(从0开始)
																unsigned int file_type,			// 文件类型(视频或照片)
																XMINT64 quota						//	允许的磁盘空间占用字节, 0表示无限制
																)
{
	if(volumn_index >= XM_VIDEOITEM_VOLUME_COUNT)
		return -1;
	if(file_type >= XM_FILE_TYPE_COUNT)
		return -1;
	XM_VideoItemLock ();
	disk_space_quota_level_type[volumn_index][file_type] = quota;
	XM_VideoItemUnlock ();
	return 0;	
}

// 设置视频项的磁盘空间占用配额, 为"指定卷"(包含所有文件类型，所有录制分类)定义空间配额
// 返回值
//		0		设置成功
//		-1		设置失败
int XM_VideoItemSetDiskSpaceQuotaLevelDiskVolume (unsigned int volumn_index, 	// SD卡卷序号(从0开始)
																  XMINT64 quota					//	允许的磁盘空间占用字节, 0表示无限制
																)
{
	if(volumn_index >= XM_VIDEOITEM_VOLUME_COUNT)
		return -1;
	XM_VideoItemLock ();
	disk_space_quota_level_volume[volumn_index] = quota;
	XM_VideoItemUnlock ();
	return 0;	
}


// 检查视频项文件数量配额是否超出配额定义。
// 返回值
//		1		超出配额定义
//		0		没有超出
static XMBOOL videoitem_check_item_count_quota (unsigned int volumn_index, unsigned int file_type, unsigned int stream_class)
{
	// 非0值表示存在配额限制
	if(item_count_quota[volumn_index][file_type][stream_class])
	{
		if(class_item_count[volumn_index][file_type][stream_class] >= item_count_quota[volumn_index][file_type][stream_class])
			return 1;
	}
	return 0;
	/*
	if(video_item_count >= (MAX_VIDEO_ITEM_COUNT - 8))
		return 1;
	else
		return 0;
	*/
}

// 检查视频项磁盘空间配额是否超出配额定义
// 返回值
//		1		超出配额定义
//		0		没有超出
static XMBOOL videoitem_check_disk_space_quota (unsigned int volumn_index, unsigned int file_type, unsigned int stream_class)
{
	// 非0值表示存在配额限制
	if(disk_space_quota_level_class[volumn_index][file_type][stream_class])
	{
		if(video_item_space[volumn_index][file_type][stream_class] >= disk_space_quota_level_class[volumn_index][file_type][stream_class])
			return 1;
	}
	return 0;
}


// 回收视频项资源并删除文件资源
// 返回值
//		0或1	已删除的视频项个数
static int videoitem_recycle (unsigned int volumn_index, unsigned int file_type, unsigned int stream_class_bitmask)
{
	HANDLE handle;

	VIDEOFILETOREMOVE VideoFileToRemove;
	int result;

	int deleted_file_count = 0; 		// 记录已删除的文件个数
	
	deleted_file_count = 0;
	
	// 检查视频句柄项个数是否已达到自动释放可循环项的阈值
	do
	{
		memset (&VideoFileToRemove, 0, sizeof(VideoFileToRemove));
		result = 0;
		
		XM_VideoItemLock ();
		if(!XM_VideoItemIsServiceOpened())
		{
			XM_VideoItemUnlock ();
			return 0;
		}
		handle = XM_VideoItemGetNextCircularVideoItemHandle (volumn_index, file_type, stream_class_bitmask);
		if(handle)
		{
			// 删除视频项数据库
			result = XM_VideoItemDeleteVideoItemHandle (handle, &VideoFileToRemove);
		}
		XM_VideoItemUnlock ();
		
		if(result)
		{
			// 将视频项对应的磁盘文件删除
			remove_video_files (&VideoFileToRemove, 1);
			deleted_file_count ++;		// 累加已删除文件的个数
		}
		
		if(handle == NULL)
		{
			return 0;
		}
	} while (0);	
	return deleted_file_count;
}

// 卷可回收空间检查
static int videoitem_recycle_space_check (unsigned int volumn_index)
{
	DWORD SectorsPerCluster;    // sectors per cluster
	DWORD BytesPerSector;        // bytes per sector
	DWORD NumberOfFreeClusters;  // free clusters
	DWORD TotalNumberOfClusters;  // total clusters
	XMINT64 FreeSpace;
	XMINT64 RequiredSpace;

	if(volumn_index >= video_volume_count)
		return -1;

	if(fat_cluster_size[volumn_index] == 0)
		return 0;
	
	if(XM_GetDiskFreeSpace ("\\", &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters , XM_VideoItemGetVolumeName(volumn_index)) == 0)
		return 0;
		
	// 检查磁盘剩余空间是否满足最低记录要求
	FreeSpace = SectorsPerCluster;
	FreeSpace = FreeSpace * BytesPerSector;
	FreeSpace = FreeSpace * NumberOfFreeClusters;
	
	RequiredSpace = FreeSpace + video_item_space[volumn_index][XM_FILE_TYPE_VIDEO][XM_VIDEOITEM_CLASS_NORMAL];
	
	// 检查可回收空间是否小于定义值
	if( recycle_space_limit && (RequiredSpace < recycle_space_limit) )
		return 1;
	
	return 0;
}

// 卷保护空间(手动、紧急)配额检查
static int videoitem_quota_check_protect_space (unsigned int volumn_index, unsigned int file_type)
{
	XMINT64 total_quota;
	XMINT64 total_space;
	if(volumn_index >= video_volume_count)
		return -1;
	if(file_type >= XM_FILE_TYPE_COUNT)
		return -1;
	// 检查普通录制分类的配额及该文件类型的配额定义是否都存在
	if(disk_space_quota_level_type[volumn_index][file_type] && disk_space_quota_level_class[volumn_index][file_type][XM_VIDEOITEM_CLASS_NORMAL])
	{
		total_quota = disk_space_quota_level_type[volumn_index][file_type] - disk_space_quota_level_class[volumn_index][file_type][XM_VIDEOITEM_CLASS_NORMAL];
		assert( total_quota > 0 );
		total_space = video_item_space[volumn_index][file_type][XM_VIDEOITEM_CLASS_MANUAL] + video_item_space[volumn_index][file_type][XM_VIDEOITEM_CLASS_URGENT];
		if(total_space > total_quota)
		{
			// 超出配额定义
			return 1;
		}
	}
	return 0;
}

// 检查是否存在基于文件类型(视频或照片)的配额限制，若存在，检查是否超出配额值
static int videoitem_quota_check_level_type (unsigned int volumn_index, unsigned int file_type)
{
	XMINT64 total_space;
	XMINT64 quota;
	unsigned int class_index;
	if(volumn_index >= video_volume_count)
		return -1;
	if(file_type >= XM_FILE_TYPE_COUNT)
		return -1;
	// 检查基于该文件类型的配额限制是否存在
	quota = disk_space_quota_level_type[volumn_index][file_type];
	if(quota)
	{
		// 非0值表示存在配额定义
		// 累加该文件类型的所有录制分类的容量
		total_space = 0;
		for (class_index = 0; class_index < XM_VIDEOITEM_CLASS_COUNT; class_index ++)
		{
			total_space += video_item_space[volumn_index][file_type][class_index];
		}
		if(total_space > quota)
			return 1;
	}
	return 0;
}

// 每10秒中扫描一次卷
// 监控每个卷，当剩余空间低于某个阈值时释放可循环记录项
// 每个卷保留最低320MB录像空间。每10秒中扫描一次卷(执行XM_VideoItemMonitorAndRecycleGarbage操作)  
void __XM_VideoItemMonitorAndRecycleGarbage (XMBOOL bReportEstimatedRecordingTime)
{
	HANDLE handle;
	DWORD SectorsPerCluster;    // sectors per cluster
	DWORD BytesPerSector;        // bytes per sector
	DWORD NumberOfFreeClusters;  // free clusters
	DWORD TotalNumberOfClusters;  // total clusters

	XMINT64 RequiredSpace;
	XMINT64 MinimumRequiredSpace;
	XMINT64 FreeSpace;
	XMBOOL ret;
	VIDEOFILETOREMOVE VideoFileToRemove;
	int result;
	
	int i;
	unsigned int volumn_index;
	int quota_check_result;
	
	int deleted_file_count = 0; 		// 记录已删除的文件个数

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return;
	
	// 
	XM_VideoItemLock ();
	if(h == NULL || video_volume_count == 0)
	{
		XM_VideoItemUnlock ();
		return;
	}
	XM_VideoItemUnlock ();
	
	// 检查是否需要执行回收站清除操作
	if(delay_recycle_clean)
	{
		delay_recycle_clean = 0;
		for (int ch = 0; ch < video_path_count; ch ++)
		{
			char recycle_path[XM_MAX_FILEFIND_NAME];
			//sprintf (recycle_path, "%s\\RECYCLE", video_path_name[ch]);
			if(XM_VideoItemMakePathName(XM_VIDEOITEM_VOLUME_0, ch, recycle_path, sizeof(recycle_path)) >= 0)
			{
				strcat (recycle_path, "\\RECYCLE");
				XM_RecycleCleanFile (recycle_path);
			}
		}
	}
	
	// ********* 1) 记录仪常规空间配额检查 ******************
	// ********* 1.1) 可回收空间配额检查
	for (volumn_index = 0; volumn_index < video_volume_count; volumn_index ++)
	{
		// 检查簇扫描是否已完成
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		// 卡满检查	
		quota_check_result = videoitem_recycle_space_check (volumn_index);
		if(quota_check_result == 1)
		{
			// 卡满限制(可循环录像空间低于设定值)
			XM_printf ("VIDEOITEM_LOW_SPACE, recycle video item\n");
			XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_VIDEOITEM_LOW_SPACE);
			
			// 20180825 zhuoyonghong
			// 可回收总空间低时，自动触发卷回收操作
			int del_count = videoitem_recycle (volumn_index, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL); 
			printf("-------------------> del_count ---------------->: %d\n",del_count);
		}
	}
	
	// ********* 1.2) 紧急录像空间配额检查 (包括手动及紧急)
	for (volumn_index = 0; volumn_index < video_volume_count; volumn_index ++)
	{
		// 检查簇扫描是否已完成
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		quota_check_result = videoitem_quota_check_protect_space (volumn_index, XM_FILE_TYPE_VIDEO);
		if(quota_check_result == 1)
		{
			// 紧急录像满报警
			XM_printf ("VIDEOITEM_PROTECT_VIDEO_FULL, recycle protect item\n");
			XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_VIDEOITEM_PROTECT_VIDEO_FULL);
			
			// 20180915 zhuoyonghong
			// 紧急录像满时，自动触发回收锁定视频
			videoitem_recycle (volumn_index, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_MANUAL|XM_VIDEOITEM_CLASS_BITMASK_URGENT);
		}
	}
	
	// ********* 1.3) 照片空间配额检查(包括所有录制分类)
	for (volumn_index = 0; volumn_index < video_volume_count; volumn_index ++)
	{
		// 检查簇扫描是否已完成
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		// 照片满检查	
		quota_check_result = videoitem_quota_check_level_type (volumn_index, XM_FILE_TYPE_PHOTO);
		if(quota_check_result == 1)
		{
			// 照片满报警
			XM_printf ("VIDEOITEM_PHOTO_FULL\n");
			XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_VIDEOITEM_PHOTO_FULL);
		}
	}
	
	// ********* 2) 可回收项目检查 ******************
	// ********* 2.1) 视频项项目数量配额检查 **************
	//
	// 2.1.1) 普通视频文件
	// 每次删除的文件数不要太多，避免阻塞
	deleted_file_count = 0;
	for (volumn_index = 0; volumn_index < video_volume_count; volumn_index ++)
	{
		// 检查簇扫描是否已完成
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		
		// 检查卷的普通视频数量是否已超出配额。若超出，执行回收操作
		while(videoitem_check_item_count_quota (volumn_index, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_NORMAL))
		{
			// 视频删除会花费较长时间。 检查Cache系统是否忙。若忙，推迟处理
			if(XMSYS_GetCacheSystemBusyLevel() > 2)
				break;
			
			XM_printf ("video(normal)'s count great than quota, recycle video item\n"); 
			if(videoitem_recycle (volumn_index, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL))
			{
				deleted_file_count ++;
				if(deleted_file_count >= 2)
					break;
			}
			else
				break;
		}
		if(deleted_file_count >= 2)
			break;
	}
	
	// 2.1.2) 普通照片文件
	for (volumn_index = 0; volumn_index < video_volume_count; volumn_index ++)
	{
		// 检查簇扫描是否已完成
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		
		deleted_file_count = 0;
		// 检查卷的普通照片数量是否已超出配额。若超出，执行回收操作
		while(videoitem_check_item_count_quota (volumn_index, XM_FILE_TYPE_PHOTO, XM_VIDEOITEM_CLASS_NORMAL))
		{
			XM_printf ("photo(normal)'s count great than quota, recycle photo item\n"); 
			if(videoitem_recycle (volumn_index, XM_FILE_TYPE_PHOTO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL))
			{
				deleted_file_count ++;
				if(deleted_file_count >= 8)
					break;				
			}
			else
				break;
		}
	}
	
	//
	// ********* 2.2) 视频项磁盘空间配额检查 **************
	//
	// 2.1) 普通视频文件
	for (volumn_index = 0; volumn_index < video_volume_count; volumn_index ++)
	{
		// 检查簇扫描是否已完成
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		
		deleted_file_count = 0;
		// 检查卷的普通视频占用空间是否已超出配额。若超出，执行回收操作
		while(videoitem_check_disk_space_quota (volumn_index, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_NORMAL))
		{
			XM_printf ("video(normal)'s space great than quota, recycle video item\n"); 
			if(videoitem_recycle (volumn_index, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL))
			{
				deleted_file_count ++;
				if(deleted_file_count >= 2)
					break;
			}
			else
				break;
		}
	}
	
	// 2.2) 普通照片文件
	for (volumn_index = 0; volumn_index < video_volume_count; volumn_index ++)
	{
		// 检查簇扫描是否已完成
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		
		deleted_file_count = 0;
		// 检查卷的普通照片占用空间是否已超出配额。若超出，执行回收操作
		while(videoitem_check_disk_space_quota (volumn_index, XM_FILE_TYPE_PHOTO, XM_VIDEOITEM_CLASS_NORMAL))
		{
			XM_printf ("photo(normal)'s space great than quota, recycle photo item\n"); 
			if(videoitem_recycle (volumn_index, XM_FILE_TYPE_PHOTO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL))
			{
				deleted_file_count ++;
				if(deleted_file_count >= 8)
					break;				
			}
			else
				break;
		}
	}
	
	//
	// ********* 3) 卷保留空间(最低要求磁盘空间)检查 **************
	//
	ret = 1;

	// 每个卷保留最低360MB录像空间。  
	RequiredSpace = 360*1024*1024;
	//RequiredSpace = 396*1024*1024;
	//RequiredSpace = 1024*1024;
	//RequiredSpace *= 7000;
	MinimumRequiredSpace = 256 * 1024 * 1024;		// 最低卷磁盘空间，低于该值时，必须执行文件删除操作。
	FreeSpace = 0;
	
	deleted_file_count = 0;
	
	// 每次检查重新设置
	disk_free_space_busy = 0;

	// 遍历每个视频卷
	for (i = 0; i < video_volume_count; )
	{
		ret = XM_GetDiskFreeSpace ("\\", &SectorsPerCluster, &BytesPerSector,
				&NumberOfFreeClusters, &TotalNumberOfClusters , XM_VideoItemGetVolumeName(i) );
		if(ret == 0)
		{
			// 卷存在异常
			if(videoitem_debug)
				XM_printf ("Fatal Error of volume(%s) File System\n", XM_VideoItemGetVolumeName(i));
			XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_FS_ERROR);
			// 遍历下一个卷
			i ++;
			continue;
		}
		

		// 检查磁盘剩余空间是否满足最低记录要求
		FreeSpace = SectorsPerCluster;
		FreeSpace = FreeSpace * BytesPerSector;
		FreeSpace = FreeSpace * NumberOfFreeClusters;

		#if 0
		XM_printf ("SectorsPerCluster=%d, NumberOfFreeClusters=%d, TotalNumberOfClusters=%d, Free=%d MB, Required=%d MB\n", 
				  SectorsPerCluster, NumberOfFreeClusters, TotalNumberOfClusters, 
				  (unsigned int)(FreeSpace/(1024*1024)),
				  (unsigned int)(RequiredSpace/(1024*1024)))
				  ;
		#endif
		
		if(FreeSpace > RequiredSpace)
		{
			// 遍历下一个卷
			i++;
			continue;
		}
		
		// 标记磁盘剩余空间已不能满足实时记录
		// 20180205 仅当磁盘空间低于最低要求空间时设置忙标志 disk_free_space_busy
		if(FreeSpace < MinimumRequiredSpace)
		{
			// 20180825 修改disk_free_space_busy定义，改为磁盘剩余空间不足程度(0 ~ 9), 0 表示磁盘空间满足记录， 9 表示磁盘剩余空间严重不足
			// 当FreeSpace >= MinimumRequiredSpace，disk_free_space_busy = 0
			// 当FreeSpace < MinimumRequiredSpace/9，disk_free_space_busy = 9
			// disk_free_space_busy = 1;
			if(FreeSpace <= (MinimumRequiredSpace/9))
				disk_free_space_busy = 9;
			else
				disk_free_space_busy = MinimumRequiredSpace / FreeSpace;
			if(disk_free_space_busy >= 9)
				disk_free_space_busy = 9;
			XM_printf ("disk free space busy %d\n", disk_free_space_busy);
		}
		
		// 检查系统是否忙
		if(XMSYS_H264CodecIsBusy () && FreeSpace > MinimumRequiredSpace)
		{
			// 文件删除会引发文件IO操作。
			// 检查H264编码器忙(正在等待写入)，此时需要延期文件删除，避免阻塞编码器写入,
			// 减少文件IO的并发操作(H264编码器文件写入与文件删除IO操作)
			
			// 遍历下一个卷
			i ++;
			continue;
		}
		
#ifdef _WINDOWS
		if(videoitem_debug)
			XM_printf ("Free %I64d, Required %I64d\n", FreeSpace, RequiredSpace);
#else
		if(videoitem_debug)
			XM_printf ("Free %lld, Required %lld, Minimum %lld\n", FreeSpace, RequiredSpace, MinimumRequiredSpace);
#endif
		
		// 释放一个视频项句柄(一个视频项包括位于多个卷上的视频文件)		
		memset (&VideoFileToRemove, 0, sizeof(VideoFileToRemove));
		result = 0;
		
		// 保护查找与删除操作的完整性
		XM_VideoItemLock ();
		// 查找一个可循环的记录项(视频文件)
		handle = XM_VideoItemGetNextCircularVideoItemHandle (i, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL);
		if(handle)
		{
			result = XM_VideoItemDeleteVideoItemHandle (handle, &VideoFileToRemove);
		}
		XM_VideoItemUnlock ();
		if(result)
		{
			// 找到一个可以删除的视频项，删除其文件
			if(remove_video_files (&VideoFileToRemove, 1) == 0)
			{
				// 删除磁盘文件失败
				// 遍历下一个卷
				i ++;
				continue;
			}
			else
			{
				// 删除磁盘文件成功
				// 20180205 zhuoyonghong 
				// 清除磁盘空间忙标志，等待下一次循环重新检查并设置disk_free_space_busy
				// 20180825 重新计算磁盘余空间不足程度
				// disk_free_space_busy = 0;
				if(XM_GetDiskFreeSpace ("\\", &SectorsPerCluster, &BytesPerSector,
					&NumberOfFreeClusters, &TotalNumberOfClusters , XM_VideoItemGetVolumeName(i) ))
				{
					// 检查磁盘剩余空间是否满足最低记录要求
					FreeSpace = SectorsPerCluster;
					FreeSpace = FreeSpace * BytesPerSector;
					FreeSpace = FreeSpace * NumberOfFreeClusters;
					if(FreeSpace <= (MinimumRequiredSpace/9))
						disk_free_space_busy = 9;
					else
						disk_free_space_busy = MinimumRequiredSpace / FreeSpace;
					if(disk_free_space_busy >= 9)
						disk_free_space_busy = 9;
					XM_printf ("disk free space busy %d\n", disk_free_space_busy);
				}
				
				deleted_file_count ++;
				//if(deleted_file_count >= 8)
				//	break;
				
				if(deleted_file_count >= 4 && FreeSpace >= MinimumRequiredSpace)
				{
					// 不要一次删除过多文件，导致反应迟钝
					break;
				}
				// 继续遍历当前卷
			}		
		}
		else 
		{
			// 没有找到可以删除的视频项

			// 检查视频项的总数量，如果视频项的数目大于或等于2，则表示所有的视频项已全部锁定，无法替换删除。
			if(video_item_count >= 2)
			{
				// 所有视频项已锁定，可循环使用资源已耗尽
				if(!XM_VideoItemIsServiceOpened())
				{
					return;
				}

				// 系统严重错误，需要手工删除视频或者格式化SD卡
				if(videoitem_debug)
					XM_printf ("Recycle Resource Consumed\n");
				
				safe_send_system_event (SYSTEM_EVENT_CARD_DISK_FULL);
				ret = 0;
				// 遍历下一个卷
				i ++;
				continue;
			}
			else		// 只有一个视频项
			{
				// 第一个视频录制中，磁盘空间不够
				if(!XM_VideoItemIsServiceOpened())
				{
					return;
				}

				if(videoitem_debug)
					XM_printf ("Disk Space low capacity\n");
				safe_send_system_event (SYSTEM_EVENT_CARD_DISK_FULL);
				ret = 0;
				// 遍历下一个卷
				i ++;
				continue;
			}
		}
		
		// 循环，重新检查卷容量
	}
		
	if(disk_free_space_busy == 0)
	{
		//XM_printf ("disk free space ready\n");
	}

}

static volatile int recycle_busy = 0;

// 检查回收任务是否忙
// 返回值
//		1		busy
//		0		free
int XM_VideoItemCheckRecycleMonitorBusy (void)
{
	return recycle_busy;
}

// 每10秒中扫描一次卷
// 监控每个卷，当剩余空间低于某个阈值时释放可循环记录项
	// 每个卷保留最低320MB录像空间。每10秒中扫描一次卷(执行XM_VideoItemMonitorAndRecycleGarbage操作)  
void XM_VideoItemMonitorAndRecycleGarbage (XMBOOL bReportEstimatedRecordingTime)
{
	// 仅且一个线程访问。若其他线程正在访问，返回。
	if(OS_Request (&videoItemMonitorSema))
	{
		unsigned int ticket = XM_GetTickCount();
		// XM_printf ("start disk monitor...\n");
		recycle_busy = 1;
		__XM_VideoItemMonitorAndRecycleGarbage (bReportEstimatedRecordingTime);
		recycle_busy = 0;
		//XM_printf ("disk monitor ticket=%d\n", XM_GetTickCount() - ticket);
		OS_Unuse (&videoItemMonitorSema);
	}
}

// 根据定义的属性读取视频项数据库中视频项的个数
// 返回值
// -1 表示错误
// >= 0 表示返回的符合定义的视频项个数
//		1)	获取卷0上的前摄像头的普通录制视频文件总数
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_0, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL);
//		2) 获取卷0上的所有通道的紧急录制(G-Sensor)视频文件总数
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_0, 0xFFFFFFFF,  XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		3) 获取卷1上的所有通道的保护视频(手动录制，紧急录制)文件总数
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_1, 0xFFFFFFFF, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_MANUAL|XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		4) 获取卷0上的后摄像头的所有照片文件总数
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_1, XM_FILE_TYPE_PHOTO, 0xFFFFFFFF);
int XM_VideoItemGetVideoItemCountEx (unsigned int volume_index, 			// 磁盘卷，0xFFFFFFFF表示包含所有卷
												 unsigned int video_channel, 			// 通道号，0xFFFFFFFF表示包含所有通道
												 unsigned int file_type, 				// 文件类型(视频或照片)，0xFFFFFFFF表示包含所有文件类型
												 unsigned int stream_class_bitmask  // 录制分类掩码,  0xFFFFFFFF表示包含所有录制类型
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL 表示包含普通及手动分类
												 )
{
	int ret = -1;
	int item_count = 0;
	XMVIDEOITEM *video_item;
	RbtIterator i;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return -1;
	
	// 锁定
	XM_VideoItemLock ();
	do
	{
		if(h == NULL || video_volume_count == 0)
		{
			break;
		}
		
		// 遍历视频项数据库
		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			void *keyp, *valuep;
			
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = (XMVIDEOITEM *)keyp;
			
			// 属性匹配
			// 磁盘卷
			if(volume_index != 0xFFFFFFFF && volume_index != video_item->volume_index)
				continue;
			// 通道号
			if(video_channel != 0xFFFFFFFF && video_channel != video_item->video_channel)
				continue;
			// 文件类型
			if(file_type != 0xFFFFFFFF && file_type != video_item->file_type)
				continue;
			// 录制分类
			if(stream_class_bitmask & (1 << video_item->stream_class))	
				item_count ++;
		}
		
		ret = item_count;
	} while (0);
	
	// 解锁
	XM_VideoItemUnlock ();
	
	return ret;	
}


// 根据定义的属性读取视频项指定属性的文件列表
// video_channel 视频项通道
// index 读取的记录开始序号
// count 读取的记录条数(包含开始序号)
// packet_addr 包缓冲区地址
// packet_size 包缓冲区字节大小
// 返回值
// < 0 失败
// >= 0 读出的列表项纪录数据的字节长度 
int XM_VideoItemGetVideoItemListEx (unsigned int video_channel, 		// 通道号， 0xFFFFFFFF表示所有通道
											 unsigned int volume_index,			// 磁盘卷号, 0xFFFFFFFF表示所有卷
											 unsigned int file_type,				// 文件类型, 0xFFFFFFFF表示所有文件类型
											 unsigned int stream_class_bitmask,	// 录像分类掩码，0xFFFFFFFF表示所有录像
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL 表示包含普通及手动分类
											 unsigned int index, 
											 unsigned int count,
											 void *packet_addr, 
											 int packet_size
											)
{
	// CH(通道，1字节), TYPE(类型，1字节)，COUNT（记录个数，2字节）,
	// 
	// RECORD(记录数据，40 * COUNT)
	// 每条记录为40字节
	// (32字节文件名 + 4字节时间表示 + 4字节录像时间长度(单位 秒)）
	//	时间表示（4字节长度 ）
	//（32位整数表示，最高位P为保护位）
	//	PYYYYYMMMMDDDDDhhhhhmmmmmmssssss
	//	最高位P表示视频是否保护，1保护。
	//	(照片最高位为0)	
	int ret = -1;
	unsigned int item_index = 0;
	//int item_count = 0;
	char *buff;
	int size;
	XMVIDEOITEM *video_item;
	RbtIterator i;
	void *keyp, *valuep;
	
	int file_count = 0;

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return -1;
	
	// 锁定
	XM_VideoItemLock ();
	do
	{
		if(h == NULL || video_volume_count == 0)
		{
			break;
		}
		
		// 遍历视频项数据库
		// 定位开始序号
		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = (XMVIDEOITEM *)keyp;
			// 属性匹配
			// 通道号
			if(video_channel != 0xFFFFFFFF && video_channel != video_item->video_channel)
				continue;
			// 磁盘卷
			if(volume_index != 0xFFFFFFFF && volume_index != video_item->volume_index)
				continue;
			// 文件类型
			if(file_type != 0xFFFFFFFF && file_type != video_item->file_type)
				continue;
			// 录像分类
			if(!(stream_class_bitmask & (1 << video_item->stream_class)))
				continue;
			
			if(item_index == index)
			{
				// 找到匹配的开始项序号
				break;
			}
				
			item_index ++;			
		}
		
		if(i == rbtEnd(h))
		{
			// 没有找到匹配的开始项
			// 4 表示头信息的字节长度，CH(通道，1字节), TYPE(类型，1字节)，COUNT（记录个数，2字节）,
			ret = 4;
			break;
		}
		

		// 从开始项开始，遍历视频项数据库，将找到的视频项项目格式化到包缓冲区
		buff = packet_addr;
		size = packet_size;
		buff += 4;		// 保留头信息
		size -= 4;
		
		// 每个数据项记录的长度(32字节文件名 + 4字节时间表示 + 4字节录像时间长度(单位 秒)）
		while (count > 0 && size >= (VIDEOITEM_MAX_FULL_NAME + 8))			
		{
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = (XMVIDEOITEM *)keyp;
			if( 	((video_channel == 0xFFFFFFFF) || (video_channel == video_item->video_channel))
				&&	((volume_index == 0xFFFFFFFF) || (volume_index == video_item->volume_index))
				&& ((file_type == 0xFFFFFFFF) || (file_type == video_item->file_type))
				&& (stream_class_bitmask & (1 << video_item->stream_class)) )
			{
				DWORD dwCreateTime = 0;
				DWORD dwVideoTime = 0;
				XMSYSTEMTIME *create_time;
				// 相同视频通道
				
				// 格式化输出 文件名
				memcpy (buff, &video_item->video_name, VIDEOITEM_MAX_FULL_NAME);	
				// 格式化输出 时间
				create_time = &video_item->video_create_time;
				dwCreateTime  = create_time->bSecond & 0x3F;
				dwCreateTime |= (create_time->bMinute & 0x3F) << 6;
				dwCreateTime |= (create_time->bHour & 0x1F) << 12;
				dwCreateTime |= (create_time->bDay & 0x1F) << 17;
				dwCreateTime |= (create_time->bMonth & 0x0F) << 22;
				dwCreateTime |= ((create_time->wYear - 2014) & 0x1F) << 26;
				// 保护位
				if(video_item->video_lock)
					dwCreateTime |= 0x80000000;
				
				memcpy (buff + VIDEOITEM_MAX_FULL_NAME, &dwCreateTime, 4);
				
				// 获取录像时间长度(单位 秒)
				dwVideoTime = AP_VideoItemGetVideoStreamSize (video_item, video_channel);
				
				memcpy (buff + VIDEOITEM_MAX_FULL_NAME + 4, &dwVideoTime, 4);
				
				buff += (VIDEOITEM_MAX_FULL_NAME+8);
				size -= (VIDEOITEM_MAX_FULL_NAME+8);		
				count --;
				
				file_count ++;
			}
			i = rbtNext(h, i);
			
			// 检查是否到达数据库的尾部
			if(i == rbtEnd(h))
				break;
		} // while (count > 0 && size >= (VIDEOITEM_MAX_FULL_NAME + 8))	
		
		ret = packet_size - size;
	} while (0);
	
	
	// 写入头信息
	// CH(通道，1字节), TYPE(类型，1字节)，COUNT（记录个数，2字节）,
	((unsigned char *)packet_addr)[0] = (unsigned char)video_channel;
	((unsigned char *)packet_addr)[1] = 0;		// 视频类型
	((unsigned char *)packet_addr)[2] = (unsigned char)(file_count);	// little endian
	((unsigned char *)packet_addr)[3] = (unsigned char)(file_count >> 8);
	
	// 解锁
	XM_VideoItemUnlock ();
	
	XM_printf ("ch=%d, video, file_count=%d\n", video_channel, file_count);
		
	return ret;	
}

// 读取指定录像通道的文件列表
// video_channel 视频项通道
// index 读取的记录开始序号
// count 读取的记录条数(包含开始序号)
// packet_addr 包缓冲区地址
// packet_size 包缓冲区字节大小
// 返回值
// < 0 失败
// >= 0 读出的列表项纪录数据的字节长度 
int XM_VideoItemGetVideoItemList (unsigned int video_channel, 
											 unsigned int index, 
											 unsigned int count,
											 void *packet_addr, 
											 int packet_size
											)
{
	return XM_VideoItemGetVideoItemListEx (video_channel,
														XM_VIDEOITEM_VOLUME_0,
														XM_FILE_TYPE_VIDEO,
														(1 << XM_VIDEOITEM_CLASS_NORMAL),
														index,
														count,
														packet_addr, packet_size);
}

void combine_fullpath_media_filename(	unsigned char *full_path_file_name, 
										int cb_full_path_file_name,
										unsigned char ch, 
										unsigned char type, 
										const unsigned char* name
									)
{
	unsigned int file_type, stream_class;
	
	if(full_path_file_name && cb_full_path_file_name > 0)
	{
		*full_path_file_name = 0;		
	}
	
	if(videoitem_extract_info ((char *)name, NULL, 0, &file_type, NULL, NULL, NULL, &stream_class, NULL) < 0)
	{
		return;
	}
	XM_VideoItemMakeFullPathFileName ((char *)name, XM_VIDEOITEM_VOLUME_0, ch, file_type, stream_class, (char *)full_path_file_name, cb_full_path_file_name);
}

int extract_fullpath_media_filename (const unsigned char* full_path_file_name, 
												 unsigned char* ch, 
												 unsigned char* type, 
												 unsigned char* name,
												 int cb_name)
{
	unsigned int channel_index, file_type;
	if(videoitem_extract_info ((char *)full_path_file_name, (char *)name, cb_name, &file_type, NULL, NULL, &channel_index, NULL, NULL) < 0)
		return -1;
	*ch = (unsigned char)channel_index;
	*type = (unsigned char)file_type;
	return 0;
}

int XM_VideoItemGetDiskUsageEx (
										unsigned int volume_index,	// 卷号
										XMINT64 *TotalSpace, 		// SD卡空间
										XMINT64 *LockedSpace, 		// 已锁定的文件空间
										XMINT64 *RecycleSpace,		// 可循环使用文件 + 空闲空间
										XMINT64 *OtherSpace			// 其他文件(系统)占用空间
									)
{
	int ret;
	DWORD SectorsPerCluster = 0;    // sectors per cluster
	DWORD BytesPerSector = 0;        // bytes per sector
	DWORD NumberOfFreeClusters = 0;  // free clusters
	DWORD TotalNumberOfClusters = 0;  // total clusters
	char *volumn_name;
	
	if(volume_index >= video_volume_count)
		return -1;
	volumn_name = (char *)XM_VideoItemGetVolumeName (volume_index);
	if(volumn_name == NULL)
		return -1;
	
	XMINT64 temp64;
	XMINT64 free64;
	XMINT64 other64;
	if(TotalSpace == NULL || LockedSpace == NULL || RecycleSpace == NULL || OtherSpace == NULL)
		return -1;
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return -1;
	
	ret = -1;
	XM_VideoItemLock ();
	
	do 
	{
		if(h == NULL || video_volume_count == 0)
			break;
		
		if(!XM_GetDiskFreeSpace ("\\", &SectorsPerCluster, &BytesPerSector,
					&NumberOfFreeClusters, &TotalNumberOfClusters , volumn_name ))
			break;
		
		// 文件系统总空间
		temp64 = SectorsPerCluster * BytesPerSector;
		temp64 = temp64 * TotalNumberOfClusters;
		// 文件系统空闲空间
		free64 = SectorsPerCluster * BytesPerSector;
		free64 = free64 * NumberOfFreeClusters;
		*TotalSpace = temp64;
		// 已锁定的视频文件总空间
		*LockedSpace = video_item_locked_space;
		// 可循环录制的总空间 = 未锁定的视频文件总空间 + 文件系统空闲空间
		*RecycleSpace = video_item_recycle_space + free64;
		// 其他文件占用空间
		// *OtherSpace = temp64 - video_item_locked_space - video_item_recycle_space;
		other64 = temp64 - free64 - video_item_locked_space - video_item_recycle_space;
		if(other64 < 0)
		{
			other64 = 0;
		}
		*OtherSpace = other64;
		
		XM_printf ("\n");
		XM_printf ("total space   = %lld bytes\n", temp64);
		XM_printf ("total free    = %lld bytes\n", free64);
		XM_printf ("total locked  = %lld bytes\n", video_item_locked_space);
		XM_printf ("total recycle = %lld bytes\n", video_item_recycle_space);
		XM_printf ("\n");
		
		ret = 0;
	} while (0);
	
	XM_VideoItemUnlock ();
	
	return ret;
}

// 此处没有考虑多个卷的情况
// 返回值定义
//	0	已获取卷信息
// -1	无法获取卷信息
int XM_VideoItemGetDiskUsage( 	XMINT64 *TotalSpace, 		// SD卡空间
								XMINT64 *LockedSpace, 		// 已锁定的文件空间
								XMINT64 *RecycleSpace,		// 可循环使用文件 + 空闲空间
								XMINT64 *OtherSpace			// 其他文件(系统)占用空间
							)
{
	int ret;
	DWORD SectorsPerCluster = 0;    // sectors per cluster
	DWORD BytesPerSector = 0;        // bytes per sector
	DWORD NumberOfFreeClusters = 0;  // free clusters
	DWORD TotalNumberOfClusters = 0;  // total clusters
	char *volumn_name = (char *)XM_VideoItemGetVolumeName (XM_VIDEOITEM_VOLUME_0);
	if(volumn_name == NULL)
		return -1;
	
	XMINT64 temp64;
	XMINT64 free64;
	XMINT64 other64;
	if(TotalSpace == NULL || LockedSpace == NULL || RecycleSpace == NULL || OtherSpace == NULL)
		return -1;
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return -1;
	
	ret = -1;
	XM_VideoItemLock ();
	
	do 
	{
		if(h == NULL || video_volume_count == 0)
			break;
		
		if(!XM_GetDiskFreeSpace ("\\", &SectorsPerCluster, &BytesPerSector,
					&NumberOfFreeClusters, &TotalNumberOfClusters , volumn_name ))
			break;
		
		// 文件系统总空间
		temp64 = SectorsPerCluster * BytesPerSector;
		temp64 = temp64 * TotalNumberOfClusters;
		// 文件系统空闲空间
		free64 = SectorsPerCluster * BytesPerSector;
		free64 = free64 * NumberOfFreeClusters;
		*TotalSpace = temp64;
		// 已锁定的视频文件总空间
		*LockedSpace = video_item_locked_space;
		// 可循环录制的总空间 = 未锁定的视频文件总空间 + 文件系统空闲空间
		*RecycleSpace = video_item_recycle_space + free64;
		// 其他文件占用空间
		// *OtherSpace = temp64 - video_item_locked_space - video_item_recycle_space;
		other64 = temp64 - free64 - video_item_locked_space - video_item_recycle_space;
		if(other64 < 0)
		{
			other64 = 0;
		}
		*OtherSpace = other64;
		
		XM_printf ("\n");
		XM_printf ("total space   = %lld bytes\n", temp64);
		XM_printf ("total free    = %lld bytes\n", free64);
		XM_printf ("total locked  = %lld bytes\n", video_item_locked_space);
		XM_printf ("total recycle = %lld bytes\n", video_item_recycle_space);
		XM_printf ("\n");
		
		ret = 0;
	} while (0);
	
	XM_VideoItemUnlock ();
	
	return ret;
}

// 从视频项文件命名恢复文件创建的时间
void XM_GetDateTimeFromShortFileName (char* filename, XMSYSTEMTIME *filetime)
{
	if(filetime == NULL)
		return;
	if(filename[0] >= '0' && filename[0] <= '9')
		filetime->wYear = (unsigned short)(2014 + filename[0] - '0');
	else if(filename[0] >= 'A' && filename[0] <= 'Z')
		filetime->wYear = (unsigned short)(2024 + filename[0] - 'A');
	else
		filetime->wYear = 2014;

	// bMonth;			// 1 ~ 12
	if(filename[1] >= '1' && filename[1] <= '9')
		filetime->bMonth = (BYTE)(1 + filename[1] - '1');
	else if(filename[1] >= 'A' && filename[1] <= 'C')
		filetime->bMonth = (BYTE)(10 + filename[1] - 'A');
	else
		filetime->bMonth = 1;

	// bDay;				// 1 ~ 31
	if(filename[2] >= '1' && filename[2] <= '9')
		filetime->bDay = (BYTE)(1 + filename[2] - '1');
	else if(filename[2] >= 'A' && filename[2] <= 'Z')
		filetime->bDay = (BYTE)(10 + filename[2] - 'A');
	else
		filetime->bDay = 1;
	if(filetime->bDay > 31)
		filetime->bDay = 31;

	// bHour;			// 0 ~ 23
	if(filename[3] >= '0' && filename[3] <= '9')
		filetime->bHour = (BYTE)(0 + filename[3] - '0');
	else if(filename[3] >= 'A' && filename[3] <= 'Z')
		filetime->bHour = (BYTE)(10 + filename[3] - 'A');
	else
		filetime->bHour = 0;
	if(filetime->bHour > 23)
		filetime->bHour = 0;

	// bMinute;			// 0 ~ 59
	filetime->bMinute = (BYTE)((filename[4] - '0') * 10 + (filename[5] - '0'));
	if(filetime->bMinute > 59)
		filetime->bMinute = 0;

	// bSecond;			// 0 ~ 59
	filetime->bSecond = (BYTE)((filename[6] - '0') * 10 + (filename[7] - '0'));
	if(filetime->bSecond > 59)
		filetime->bSecond = 0;

	filetime->wMilliSecond = 0;
}

// 从视频项长文件命名恢复文件创建的时间
// 20180318_121056_00000001_00P.AVI
void XM_GetDateTimeFromLongFileName (char* filename, XMSYSTEMTIME *filetime)
{
	if(filetime == NULL)
		return;

	//XM_VideoItemMakePathName
	filetime->wYear = 2018;
	filetime->bMonth = 1;
	filetime->bDay = 1;
	filetime->bHour = 0;
	filetime->bMinute = 0;
	filetime->bSecond = 0;
	filetime->wMilliSecond = 0;
	
	// wYear
	//printf("11111111111 filename: %s \n",filename);
	if(	filename[0] == '2' && filename[1] == '0' && filename[2] >= '0' && filename[2] <= '9' && filename[3] >= '0' && filename[2] <= '9' )
		filetime->wYear = (unsigned short)(2000 + (filename[2] - '0') * 10 + (filename[3] - '0'));

	// bMonth;			// 1 ~ 12
	if( (filename[4] >= '0') && (filename[4] <= '1') && (filename[5] >= '0') && (filename[5] <= '9') )
	{
		filetime->bMonth = (BYTE)( (filename[4] - '0')*10 + filename[5] - '0');
		if(filetime->bMonth == 0)
			filetime->bMonth = 1;
		else if(filetime->bMonth > 12)
			filetime->bMonth = 1;
	}
	
	// bDay;				// 1 ~ 31
	if( (filename[6] >= '0') && (filename[6] <= '9') && (filename[7] >= '0') && (filename[7] <= '9') )
	{
		filetime->bDay = (BYTE)( (filename[6] - '0')*10 + filename[7] - '0');
		if(filetime->bDay == 0)
			filetime->bDay = 1;
		else if(filetime->bDay > 31)
			filetime->bDay = 1;
	}
	
	// bHour;			// 0 ~ 23
	if( (filename[9] >= '0') && (filename[9] <= '9') && (filename[10] >= '0') && (filename[10] <= '9') )
	{
		filetime->bHour = (BYTE)( (filename[9] - '0')*10 + filename[10] - '0');
		if(filetime->bHour > 23)
			filetime->bHour = 0;
	}

	// bMinute;			// 0 ~ 59
	if( (filename[11] >= '0') && (filename[11] <= '9') && (filename[12] >= '0') && (filename[12] <= '9') )
	{
		filetime->bMinute = (BYTE)( (filename[11] - '0')*10 + filename[12] - '0');
		if(filetime->bMinute > 59)
			filetime->bMinute = 0;
	}

	// bSecond;			// 0 ~ 59
	if( (filename[13] >= '0') && (filename[13] <= '9') && (filename[14] >= '0') && (filename[14] <= '9') )
	{
		filetime->bSecond = (BYTE)( (filename[13] - '0')*10 + filename[14] - '0');
		if(filetime->bSecond > 59)
			filetime->bSecond = 0;
	}
	
	filetime->wMilliSecond = 0;
}

static int is_valid_time (XMSYSTEMTIME *lpCreateTime)
{
	if(lpCreateTime->wYear < 1970 || lpCreateTime->wYear > 2099)
		return -1;
	if(lpCreateTime->bMonth < 1 || lpCreateTime->bMonth > 12)
		return -1;
	if(lpCreateTime->bDay < 1 || lpCreateTime->bDay > 31)
		return -1;
	if(lpCreateTime->bHour >= 24)
		return -1;
	if(lpCreateTime->bMinute >= 60)
		return -1;
	if(lpCreateTime->bSecond >= 60)
		return -1;
	return 0;
}

// 根据视频项属性(创建时间，录像通道号，文件类型，视频项分类等)创建文件命名
// 20180411_115623_00000000_00P.JPG		保存在卷0上的通道0的手动保护的照片	
// 20180411_115623_00012345_10N.AVI		保存在卷0上的通道1的可循环覆盖的普通类型视频文件
// 20180411_115623_00011110_01U.AVI		保存在卷1上的通道0的紧急类型的视频文件，由G-Sensor触发
// 20180411_115623_10000000_11U.JPG		保存在卷1上的通道1的紧急类型的照片文件，由G-Sensor触发
// 返回值
//	 0      创建文件名成功
//  -1     失败
int XM_MakeFileNameFromCurrentDateTimeEx(	XMSYSTEMTIME *lpCreateTime, 			// 文件创建时间， 为空表示使用当前时间
											unsigned int VideoChannel,				// 录像通道号
											unsigned int FileType, 					// 文件类型(视频或照片)
											unsigned int VideoItemClass,			// 视频项分类(正常、手动或紧急)
											unsigned int CardVolumeIndex,			// SD卡卷序号(从0开始)
											unsigned int SerialNumber,				// 序号
											char* lpFileName, int cbFileName		// 保存文件命名的缓冲区地址及缓冲区字节大小
										)
{
	XMSYSTEMTIME localTime;
	char *ch;
	// 缓冲区包含结束\0字符
	if(lpFileName == NULL || cbFileName < (VIDEOITEM_MAX_FULL_NAME + 1))
	{
		XM_printf ("MakeFileNameFromCurrentDateTimeEx failed, illegal lpFileName=0x%08x,  cbFileName=%d\n", lpFileName, cbFileName);
		return -1;
	}
	if(VideoChannel >= XM_VIDEO_CHANNEL_COUNT)
	{
		XM_printf ("MakeFileNameFromCurrentDateTimeEx failed, illegal VideoChannel=%d\n", VideoChannel);
		return -1;
	}
	if(FileType >= XM_FILE_TYPE_COUNT)
	{
		XM_printf ("MakeFileNameFromCurrentDateTimeEx failed, illegal FileType=%d\n", FileType);
		return -1;
	}
	if(VideoItemClass >= XM_VIDEOITEM_CLASS_COUNT)
	{
		XM_printf ("MakeFileNameFromCurrentDateTimeEx failed, illegal VideoItemClass=%d\n", VideoItemClass);
		return -1;
	}
	if(CardVolumeIndex >= video_volume_count)
	{
		XM_printf ("MakeFileNameFromCurrentDateTimeEx failed, illegal CardVolumeIndex=%d\n", CardVolumeIndex);
		return -1;
	}
	if(lpCreateTime && is_valid_time (lpCreateTime) < 0)
	{
		XM_printf ("MakeFileNameFromCurrentDateTimeEx failed, illegal lpCreateTime\n");
		return -1;		
	}
	
	if(lpCreateTime)
		memcpy (&localTime, lpCreateTime, sizeof(localTime));
	else
		XM_GetLocalTime (&localTime);
	ch = lpFileName;
	// 20180411_115623_00000012_00U.AVI
	sprintf (ch, "%04d%02d%02d_%02d%02d%02d_%08d_%d%d%c.%s", localTime.wYear, localTime.bMonth, localTime.bDay,
				localTime.bHour, localTime.bMinute, localTime.bSecond, SerialNumber, 
				VideoChannel,
				CardVolumeIndex,
				videoitem_class_id[VideoItemClass],
				videoitem_file_type_id[FileType]);

	XM_printf(">>>>>>>>>>make file lpFileName:%s\r\n", lpFileName);
	return 0;
}

// 创建路径名 "mmc:0:\\VIDEO_F"
int XM_VideoItemMakePathName(unsigned int CardVolumeIndex,			// SD卡卷序号(从0开始)
							unsigned int VideoChannel,				// 通道号(从0开始)
							char *lpFullPathName, int cbFullPathName// 保存视频项文件的全路径文件名缓存及全路径文件名缓存字节长度
							)
{
	int full_path_name_size;
	char *volume_name, *channel_name;
	if(CardVolumeIndex >= video_volume_count)
		return -1;
	volume_name = (char *)XM_VideoItemGetVolumeName(CardVolumeIndex);
	if(volume_name == NULL)
		return -1;
	channel_name = (char *)XM_VideoItemGetChannelName (VideoChannel, XM_FILE_TYPE_VIDEO);
	if(channel_name == NULL)
		return -1;
	full_path_name_size = strlen(volume_name) + 1 + strlen(channel_name) + 1;
	if(full_path_name_size > cbFullPathName)
		return -1;
	sprintf (lpFullPathName, "%s\\%s", volume_name, channel_name);
	return full_path_name_size;
}

int XM_VideoItemMakePathNameEx(unsigned int CardVolumeIndex,			// SD卡卷序号(从0开始)
								unsigned int VideoChannel,				// 通道号(从0开始)
								unsigned int FileType,					// 文件类型(VIDEO/PHOTO)
								char *lpFullPathName, int cbFullPathName// 保存视频项文件的全路径文件名缓存及全路径文件名缓存字节长度
								)
{
	int full_path_name_size;
	char *volume_name, *channel_name;
	if(CardVolumeIndex >= video_volume_count)
		return -1;
	volume_name = (char *)XM_VideoItemGetVolumeName(CardVolumeIndex);
	if(volume_name == NULL)
		return -1;
	channel_name = (char *)XM_VideoItemGetChannelName (VideoChannel, FileType);
	if(channel_name == NULL)
		return -1;
	full_path_name_size = strlen(volume_name) + 1 + strlen(channel_name) + 1;
	if(full_path_name_size > cbFullPathName)
		return -1;
	sprintf (lpFullPathName, "%s\\%s", volume_name, channel_name);
	return full_path_name_size;
}

// 计算全路径目录名
// "mmc:0:\\VIDEO_F\\"
// "mmc:0:\\VIDEO_F\\NORMAL\\"
// "mmc:0:\\VIDEO_F\\URGENT\\"
// 返回值
//		< 0		失败
int XM_VideoItemMakeFullPathName(unsigned int CardVolumeIndex,				// SD卡卷序号(从0开始)
								unsigned int VideoChannel,					// 通道号(从0开始)
								unsigned int FileType,						// 文件类型(VIDEO/PHOTO)
								unsigned int StreamClass,					// 录制类型(普通，手动，紧急)
								char *lpFullPathName, int cbFullPathName	// 保存视频项文件的全路径文件名缓存及全路径文件名缓存字节长度
								)
{
	int full_path_name_size;
	char *volume_name, *channel_name;
	char *class_name;
	
	if(CardVolumeIndex >= video_volume_count)
		return -1;
	
	volume_name = (char *)XM_VideoItemGetVolumeName(CardVolumeIndex);//mmc0: or mmc1
	if(volume_name == NULL)
		return -1;
	
	channel_name = (char *)XM_VideoItemGetChannelName (VideoChannel, FileType);
	if(channel_name == NULL)
		return -1;
	
#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2
	// "mmc:0:\\VIDEO_F\\NORMAL\\20180318_121056_00001201_00N.AVI"
	// "mmc:0:\\VIDEO_F\\URGENT\\20180318_121056_00001201_00U.AVI"
	// "mmc:0:\\VIDEO_B\\NORMAL\\20180318_121056_00001201_10N.AVI"
	// "mmc:0:\\VIDEO_B\\NORMAL\\20180318_121056_00001201_10U.AVI"
	// "mmc:0:\\PHOTO_F\\NORMAL\\20180318_121056_00001201_00N.JPG"
	// "mmc:0:\\PHOTO_F\\URGENT\\20180318_121056_00001201_00U.JPG"
	class_name = (char *)XM_VideoItemGetStreamClassName (StreamClass);
	if(class_name == NULL)
		return -1;
	full_path_name_size = strlen(volume_name) + 1 + strlen(channel_name) + 1 + strlen(class_name) + 1 ;
	if(full_path_name_size > cbFullPathName)
		return -1;
	
	sprintf (lpFullPathName, "%s\\%s\\%s\\", volume_name, channel_name, class_name);
#else
	// "mmc:0:\\VIDEO_F\\20180318_121056_00001201_00N.AVI"
	// "mmc:0:\\VIDEO_F\\20180318_121056_00001201_00U.AVI"
	// "mmc:0:\\VIDEO_F\\20180318_121056_00001201_00N.JPG"
	// "mmc:0:\\VIDEO_B\\20180318_121056_00001201_10N.AVI"
	// "mmc:0:\\VIDEO_B\\20180318_121056_00001201_10U.AVI"
	full_path_name_size = strlen(volume_name) + 1 + strlen(channel_name) + 1;
	if(full_path_name_size > cbFullPathName)
		return -1;
	
	sprintf (lpFullPathName, "%s\\%s\\", volume_name, channel_name);
#endif
	return full_path_name_size;
}


// 计算全路径文件名
// "mmc:0:\\VIDEO_F\\20180318_121056_00001201_00P.AVI"
int XM_VideoItemMakeFullPathFileName (const char *VideoItemName,				// 视频项命名
											 unsigned int CardVolumeIndex,			// SD卡卷序号(从0开始)
											 unsigned int VideoChannel,				// 通道号(从0开始)
											 unsigned int FileType,						// 视频或照片
											 unsigned int StreamClass,					// 录制类型(普通，手动，紧急)
											 char *lpFullPathName, int cbFullPathName	// 保存视频项文件的全路径文件名缓存及全路径文件名缓存字节长度
											)
{
	int full_path_name_size;
	char *volume_name, *channel_name;
	char *class_name;

	if(VideoItemName == NULL || CardVolumeIndex >= video_volume_count)
		return -1;
	volume_name = (char *)XM_VideoItemGetVolumeName(CardVolumeIndex);
	if(volume_name == NULL)
		return -1;
	channel_name = (char *)XM_VideoItemGetChannelName (VideoChannel, FileType);
	if(channel_name == NULL)
		return -1;

#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2
	// "mmc:0:\\VIDEO_F\\NORMAL\\20180318_121056_00001201_00N.AVI"
	// "mmc:0:\\VIDEO_F\\URGENT\\20180318_121056_00001201_00U.AVI"
	// "mmc:0:\\VIDEO_B\\NORMAL\\20180318_121056_00001201_10N.AVI"
	// "mmc:0:\\VIDEO_B\\NORMAL\\20180318_121056_00001201_10U.AVI"
	// "mmc:0:\\PHOTO_F\\NORMAL\\20180318_121056_00001201_00N.JPG"
	// "mmc:0:\\PHOTO_F\\URGENT\\20180318_121056_00001201_00U.JPG"
	class_name = (char *)XM_VideoItemGetStreamClassName (StreamClass);
	if(class_name == NULL)
		return -1;
	full_path_name_size = strlen(volume_name) + 1 + strlen(channel_name) + 1 + strlen(class_name) + 1 + strlen(VideoItemName) + 1;
	if(full_path_name_size > cbFullPathName)
		return -1;
	sprintf (lpFullPathName, "%s\\%s\\%s\\%s", volume_name, channel_name, class_name, VideoItemName);
#else
	// "mmc:0:\\VIDEO_F\\20180318_121056_00001201_00N.AVI"
	// "mmc:0:\\VIDEO_F\\20180318_121056_00001201_00U.AVI"
	// "mmc:0:\\VIDEO_F\\20180318_121056_00001201_00N.JPG"
	// "mmc:0:\\VIDEO_B\\20180318_121056_00001201_10N.AVI"
	// "mmc:0:\\VIDEO_B\\20180318_121056_00001201_10U.AVI"
	full_path_name_size = strlen(volume_name) + 1 + strlen(channel_name) + 1 + strlen(VideoItemName) + 1;
	if(full_path_name_size > cbFullPathName)
		return -1;
	sprintf (lpFullPathName, "%s\\%s\\%s", volume_name, channel_name, VideoItemName);
#endif
	return full_path_name_size;
}

int XM_MakeFileNameFromCurrentDateTime (XMSYSTEMTIME *filetime, char* filename, int cb_filename)
{
	return XM_MakeFileNameFromCurrentDateTimeEx (filetime, 
													  XM_VIDEO_CHANNEL_0,
													  XM_FILE_TYPE_VIDEO,
													  XM_VIDEOITEM_CLASS_NORMAL,
													  XM_VIDEOITEM_VOLUME_0,
													  videoitem_get_serial_number (XM_VIDEOITEM_VOLUME_0, XM_FILE_TYPE_VIDEO, XM_VIDEO_CHANNEL_0),
													  filename, cb_filename);
}

// 从视频项文件命名恢复文件创建的时间
void XM_GetDateTimeFromFileName (char* filename, XMSYSTEMTIME *filetime)
{
	XM_GetDateTimeFromLongFileName (filename, filetime);
}




// 检查视频项句柄是否已更新(已关闭对应的视频文件)
// 通过文件名查找而不是使用句柄，句柄可能因为文件被删除而无效
XMBOOL XM_VideoItemWaitingForUpdate (const char *lpVideoFile)
{
	HANDLE hVideoItem;
	XMVIDEOITEM *video_item;
	unsigned int update;
	int video_channel;
	XMBOOL ret = 0;
	UINT timeout_ticket;
	unsigned char attr;
	
	if(lpVideoFile == NULL || *lpVideoFile == '\0')
		return 0;
	
	// 最大等待3秒钟等待视频项文件关闭
	timeout_ticket = XM_GetTickCount() + 3000;
	
	// 检查是否是临时视频项文件
	if(XM_VideoItemIsTempVideoItemFile (lpVideoFile))
	{
		// 等待临时视频文件关闭并改名
		while(XM_GetTickCount() < timeout_ticket)
		{
			void *fp;
			
			// 检查ACC是否下电. 若已下电, 不再等待TEMP文件更名
			if(!xm_power_check_acc_safe_or_not())
			{
				XM_printf ("WaitingForUpdate Stop, ACC down\n");
				return 0;
			}
			
			if(XMSYS_file_system_block_write_access_lock() == 0)
			{
				// 检查临时文件是否存在
				fp = XM_fopen (lpVideoFile, "rb");
				if(fp)
				{
					XM_fclose (fp);
				}
				XMSYS_file_system_block_write_access_unlock ();
				if(fp == NULL)
				{
					// 文件已不存在
					ret = 1;
					break;
				}
				XM_Sleep (1);
			}
			else
			{
				XMSYS_file_system_block_write_access_unlock ();
				// 非安全的文件系统
				break;
			}
		}
		
		return ret;
	}
	
	block_file_delete = 1;
	//XM_printf ("XM_VideoItemWaitingForUpdate 0x%8x\n", hVideoItem);
	do
	{
		update = 0;
		XM_VideoItemLock ();
		hVideoItem = XM_VideoItemGetVideoItemHandle (lpVideoFile, &video_channel);
		if(hVideoItem == NULL)
		{
			// 文件已被删除
			XM_VideoItemUnlock ();
			break;
		}
		video_item = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
		if(video_item)
			update = video_item->video_update & (1 << video_channel);
		XM_VideoItemUnlock ();
		
		if(video_item == NULL)	// 视频项数据库已关闭
			break;
		
		if(update)
		{
			ret = 1;
			break;
		}
		
		// 检查视频项文件是否已创建. 
		// 视频项文件使用cache文件机制, 文件创建不存在失败. 因此使用物理文件检查来确认文件是否存在.
		// 若物理文件不存在, 退出
		attr = FS_GetFileAttributes (lpVideoFile);
		if(attr == 0xFF)	// == 0xFF: In case of any error.
		{
			XM_printf ("XM_VideoItemWaitingForUpdate failed, file(%s) seems bad\n", lpVideoFile);
			ret = 0;
			break;
		}
			 
		if( XM_GetTickCount() >= timeout_ticket )
		{
			XM_printf ("XM_VideoItemWaitingForUpdate failed, it's timeout to wait for file(%s) update\n", lpVideoFile);
			ret = 0;
			break;
		}
		// 等待视频项文件关闭
		XM_Sleep (1);
	} while (video_item);
	
	//XM_printf ("XM_VideoItemWaitingForUpdate end\n");
	block_file_delete = 0;
	return ret;
}

// 将指定文件名的视频标记使用中，防止删除操作将视频项删除
XMBOOL XM_VideoItemMarkVideoFileUsage (const char *lpVideoFileName)
{
	HANDLE hVideoHandle;
	XMBOOL ret = 0;
	XMVIDEOITEM *video_item = NULL;
	void *keyp = NULL, *valuep = NULL;
	int nVideoChannel;
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	XM_VideoItemLock ();
	hVideoHandle = XM_VideoItemGetVideoItemHandle (lpVideoFileName, &nVideoChannel);
	if(hVideoHandle)
	{
		rbtKeyValue(h, hVideoHandle, &keyp, &valuep);
		video_item = (XMVIDEOITEM *)keyp;
	}
	if(video_item)
	{
		video_item->video_ref_count ++;
		ret = 1;
	}
	XM_VideoItemUnlock ();
	return ret;
}

// 将指定文件名的视频标记未使用
XMBOOL XM_VideoItemMarkVideoFileUnuse (const char *lpVideoFileName)
{
	HANDLE hVideoHandle;
	XMBOOL ret = 0;
	XMVIDEOITEM *video_item = NULL;
	void *keyp = NULL, *valuep = NULL;
	int nVideoChannel;
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	XM_VideoItemLock ();
	hVideoHandle = XM_VideoItemGetVideoItemHandle (lpVideoFileName, &nVideoChannel);
	if(hVideoHandle)
	{
		rbtKeyValue(h, hVideoHandle, &keyp, &valuep);
		video_item = (XMVIDEOITEM *)keyp;
	}
	if(video_item)
	{
		if(video_item->video_ref_count)
		{
			video_item->video_ref_count --;
			ret = 1;
		}
		else
		{
			XM_printf ("mark the video item(%s) usless NG\n", lpVideoFileName);
		}
	}
	XM_VideoItemUnlock ();
	return ret;	
}

static int image_item_compare (const void *image_item_1, const void *image_item_2)
{
	// 每条记录为40字节
	// (32字节文件名 + 4字节时间表示 + 4字节录像时间长度(单位 秒)）	
	//	时间表示（4字节长度 ）
	//（32位整数表示，最高位P为保护位）
	//	PYYYYYMMMMDDDDDhhhhhmmmmmmssssss
	//	最高位P表示视频是否保护，1保护。
	//	(照片最高位为0)		
	unsigned int time1, time2;
	time1 = get_long((char *)image_item_1 + VIDEOITEM_LFN_SIZE);
	time2 = get_long((char *)image_item_2 + VIDEOITEM_LFN_SIZE);	
	if(time1 > time2)
		return -1;
	else if(time1 == time2)
		return 0;
	else
		return 1;
}

// 读取指定照片通道的文件列表, 按照视频列表格式组织
// image_channel 照片项通道
// index 读取的记录开始序号
// count 读取的记录条数(包含开始序号)
// packet_addr 包缓冲区地址
// packet_size 包缓冲区字节大小
// 返回值
// < 0 失败
// = 0 读出的文件数为0
// > 0 读出的有效文件数
int XM_VideoItemGetImageItemList (unsigned int image_channel, 
											 unsigned int index, 
											 unsigned int count,
											 void *packet_addr, 
											 int packet_size
											)
{
	return XM_VideoItemGetVideoItemListEx (image_channel,
														XM_VIDEOITEM_VOLUME_0,
														XM_FILE_TYPE_PHOTO,
														(1 << XM_VIDEOITEM_CLASS_NORMAL),
														index,
														count,
														packet_addr, packet_size);
}

// 将指定的视频文件(全路径名)删除至回收站
// 1 成功
// 0 失败
XMBOOL XM_VideoItemDeleteVideoFileIntoRecycleBin (const char *lpVideoFileName)
{
	HANDLE hVideoHandle = NULL;
	XMBOOL ret = 0;
	int nVideoChannel;
	VIDEOFILETOREMOVE VideoFileToRemove;
	XMVIDEOITEM *video_item = NULL;
	char video_name[VIDEOITEM_MAX_NAME + 1];
	
	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	memset (video_name, 0, sizeof(video_name));
	
	// XM_VideoItemLock允许同一个线程内多次调用，只需匹配对应的XM_VideoItemUnlock次数
	XM_VideoItemLock ();
	hVideoHandle = XM_VideoItemGetVideoItemHandle (lpVideoFileName, &nVideoChannel);
	if(hVideoHandle)
	{
		video_item = AP_VideoItemGetVideoItemFromHandle (hVideoHandle);
		if(video_item)
		{
			strcpy (video_name, (const char *)video_item->video_name);
			ret = XM_VideoItemDeleteVideoItemHandle (hVideoHandle, &VideoFileToRemove);
		}
	}
	XM_VideoItemUnlock ();
	
	if(ret)
	{
		// 将被删除的文件移至回收站
		int i;
		for (i = 0; i < VideoFileToRemove.count; i ++)
		{
			if(XM_RecycleDeleteFile (VideoFileToRemove.channel[i], XM_FILE_TYPE_VIDEO, video_name) < 0)
			{
				ret = 0;
				break;
			}
		}		
	}
	return ret;
}

// 检查磁盘剩余空间是否无法满足实时录像需求
// 返回值
// 1   无法满足实时录像需求
// 0   可以满足实时录像需求
// 20180825 修改disk_free_space_busy定义，改为磁盘剩余空间不足程度(0 ~ 9), 0 表示磁盘空间满足记录， 9 表示磁盘剩余空间严重不足
XMBOOL XM_VideoItemCheckDiskFreeSpaceBusy (void)
{
	return disk_free_space_busy;
}

// 20170929
// 延时现在的文件删除操作
void XM_VideoItemSetDelayCutOff (void)
{
	delay_cutoff_flag = 1;
}

int XM_VideoItemGetDelayCutOff (void)
{
	return delay_cutoff_flag;
}

// 设置磁盘报警阈值
// 	MinimumSpaceLimit(MB)		磁盘最低容量限制定义
//	RecycleSpaceLimit(MB)		可回收空间最低配额定义, 小于该值将报警
int XM_VideoItemSetDiskSpaceLimit (unsigned int MinimumSpaceLimit, unsigned int RecycleSpaceLimit)
{
	minimum_space_limit = MinimumSpaceLimit;
	minimum_space_limit *= 1024 * 1024;
	recycle_space_limit = RecycleSpaceLimit;
	recycle_space_limit *= 1024 * 1024;
	return 0;
}

// 使用预定义的配额管理(在磁盘卷可用后设置， 否则设置可能失败)
// 返回值
//		0		设置成功
//		< 0	设置失败
int XM_VideoItemSetQuotaProfile (unsigned int PredefinedQuotaProfile)
{
	unsigned int volume_index;
	DWORD SectorsPerCluster = 0;    // sectors per cluster
	DWORD BytesPerSector = 0;        // bytes per sector
	DWORD NumberOfFreeClusters = 0;  // free clusters
	DWORD TotalNumberOfClusters = 0;  // total clusters
	char *volume_name;
	if(PredefinedQuotaProfile >= XM_VIDEOITEM_QUOTA_PROFILE_COUNT)
		return -1;
	
	XM_VideoItemLock ();
	if(PredefinedQuotaProfile == XM_VIDEOITEM_QUOTA_PROFILE_1)
	{
		// 定义每个卷的可回收空间均为1500MB。当低于该阈值时，系统报警，提示磁盘已满。
		XM_VideoItemSetDiskSpaceLimit (500, 1500); 
		// 测试用途，定义个卷的可回收空间均为8000MB。当低于该阈值时，系统报警，提示磁盘已满。
		//XM_VideoItemSetDiskSpaceLimit (500, 8000); 
		
		for (volume_index = 0; volume_index < video_volume_count; volume_index ++)
		{
			XMINT64 total64;
			volume_name = (char *)XM_VideoItemGetVolumeName (volume_index);
			if(XM_GetDiskFreeSpace ("\\", &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters, volume_name) == 0)
				continue;
			
			// 文件系统总空间
			total64 = SectorsPerCluster * BytesPerSector;
			total64 = total64 * TotalNumberOfClusters;
			
			// 设置指定卷的照片空间配额(照片配额200M, 当该卷的"所有录制类型的"照片总空间和大于200MB时，系统提示照片空间满)
			XM_VideoItemSetDiskSpaceQuotaLevelFileType (volume_index, XM_FILE_TYPE_PHOTO, 200 * 1024 * 1024);
			// 测试用途，设置指定卷的照片空间配额(照片配额3M, 当该卷的"所有录制类型的"照片总空间和大于3MB时，系统提示照片空间满)
			//XM_VideoItemSetDiskSpaceQuotaLevelFileType (volume_index, XM_FILE_TYPE_PHOTO, 3 * 1024 * 1024);
			
			// 测试用途，设置"普通录制照片"空间配额为2MB(当大于该2MB配额，将自动启动普通录制照片回收机制将最老的照片删除)
			//XM_VideoItemSetDiskSpaceQuotaLevelStreamClass (volume_index, XM_FILE_TYPE_PHOTO, XM_VIDEOITEM_CLASS_NORMAL, 2 * 1024 * 1024);		// 测试
			
			// 设置总的视频空间配额(卡容量 - 照片配额200M - 500M(卡最低容量限制))
			total64 -= 200 * 1024 * 1024;
			total64 -= 500 * 1024 * 1024;
			//total64 = 1024 * 1024 * 1024;
			//total64 *= 6;
			XM_VideoItemSetDiskSpaceQuotaLevelFileType (volume_index, XM_FILE_TYPE_VIDEO, total64);
			
			// 一般录像分配(卡容量 - 照片配额200M - 500M(卡最低容量限制))x70%
			// 紧急录像分配(卡容量 - 照片配额200M - 500M(卡最低容量限制))x30%
			// 设置一般录像分配(卡容量 - 照片配额200M - 500M(卡最低容量限制))x70% (大于该配额，将自动启动普通录制视频回收机制)
			XM_VideoItemSetDiskSpaceQuotaLevelStreamClass (volume_index, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_NORMAL, total64 * 7 / 10);
		}
	}
	XM_VideoItemUnlock ();
	
	return 0;
}

// 根据定义的属性删除所有满足属性定义的视频项
// 返回值
//		< 0		异常
//		0			正常结束退出
//		1			用户终止退出
int XM_VideoItemDeleteVideoItemList (
											 unsigned int volume_index,			// 磁盘卷号, 0xFFFFFFFF表示所有卷
											 unsigned int video_channel, 			// 通道号， 0xFFFFFFFF表示所有通道
											 unsigned int file_type,				// 文件类型, 0xFFFFFFFF表示所有文件类型
											 unsigned int stream_class_bitmask,	// 录像分类掩码，0xFFFFFFFF表示所有录像
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL 表示包含普通及手动分类
											 int (*fp_user_callback) (void *user_data, char *full_path_name_to_delete),
											 												// 用户回调函数， 用于用户取消某个长时间操作。为0表示无回调函数
																							//		返回值为1表示终止当前的操作并退出，返回值为0表示继续当前的操作
																							//		user_data 传入的用户私有数据
																							//		full_path_name_to_delete 准备删除的视频项全路径文件名
											 void *user_data							//	用户私有数据
											)
{
	RbtIterator i;
	void *keyp, *valuep;
	XMVIDEOITEM *video_item;
	char file_name[VIDEOITEM_MAX_FILE_NAME];
	int ret;
	
	if(volume_index != 0xFFFFFFFF && volume_index >= video_volume_count)
		return -1;
	
	if(video_channel != 0xFFFFFFFF && video_channel >= XM_VIDEO_CHANNEL_COUNT)
		return -1;
	
	if(file_type != 0xFFFFFFFF && file_type >= XM_FILE_TYPE_COUNT)
		return -1;	

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return -1;	

	memset (file_name, 0, sizeof(file_name));
	ret = -1;
	// 锁定
	XM_VideoItemLock ();
		
	do
	{
		if(h == NULL || video_volume_count == 0)
		{
			break;
		}
		
		ret = 0;
		// 遍历视频项数据库
		// 定位开始序号
		for (i = rbtBegin(h); i != rbtEnd(h); )
		{
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = (XMVIDEOITEM *)keyp;
			
			//XM_printf("video_item->video_name =%s,video_item->volume_index=%d,video_item->video_channel=%d\n",(char *)video_item->video_name,video_item->volume_index,video_item->video_channel);
			
			// 属性匹配
			// 通道号
			if(video_channel != 0xFFFFFFFF && video_channel != video_item->video_channel)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// 磁盘卷
			if(volume_index != 0xFFFFFFFF && volume_index != video_item->volume_index)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// 文件类型
			if(file_type != 0xFFFFFFFF && file_type != video_item->file_type)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// 录像分类
			if(!(stream_class_bitmask & (1 << video_item->stream_class)))
			{
				i = rbtNext(h, i);
				continue;
			}

			if(XM_VideoItemMakeFullPathFileName ((char *)video_item->video_name, video_item->volume_index, video_item->video_channel, video_item->file_type, video_item->stream_class, file_name, sizeof(file_name)) < 0)
			{
				ret = -1;
				break;
			}
			
			if(fp_user_callback)
			{
				if( (*fp_user_callback)(user_data, file_name) == 1 )	// 终止当前的删除操作，返回
				{
					ret = 1;
					break;
				}
			}
			// 获取下一个节点，避免当前节点删除后，下一个节点信息异常。
			i = rbtNext(h, i);
			XM_VideoItemDeleteVideoItemHandleFromFileNameEx (file_name, 0);
		}
	} while (0);
	
	// 解锁
	XM_VideoItemUnlock ();
	
	return ret;	
}

// 将符合属性定义组合的视频项全部加锁
// 返回值
//		0		成功
//		-1		失败
int XM_VideoItemLockVideoItem (
											 unsigned int volume_index,			// 磁盘卷号, 0xFFFFFFFF表示所有卷
											 unsigned int video_channel, 			// 通道号， 0xFFFFFFFF表示所有通道
											 unsigned int file_type,				// 文件类型, 0xFFFFFFFF表示所有文件类型
											 unsigned int stream_class_bitmask,	// 录像分类掩码，0xFFFFFFFF表示所有录像
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL 表示包含普通及手动分类
											 int (*fp_user_callback) (void *user_data, char *full_path_name_to_delete),
											 												// 用户回调函数， 用于用户取消某个长时间操作。为0表示无回调函数
																							//		返回值为1表示终止当前的操作并退出，返回值为0表示继续当前的操作
																							//		user_data 传入的用户私有数据
																							//		full_path_name_to_delete 准备删除的视频项全路径文件名
											 void *user_data							//	用户私有数据
											)
{
	RbtIterator i;
	void *keyp, *valuep;
	XMVIDEOITEM *video_item;
	char file_name[VIDEOITEM_MAX_FILE_NAME];
	int ret;
	
	if(volume_index != 0xFFFFFFFF && volume_index >= video_volume_count)
		return -1;
	
	if(video_channel != 0xFFFFFFFF && video_channel >= XM_VIDEO_CHANNEL_COUNT)
		return -1;
	
	if(file_type != 0xFFFFFFFF && file_type >= XM_FILE_TYPE_COUNT)
		return -1;	

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return -1;	

	memset (file_name, 0, sizeof(file_name));
	ret = -1;
	// 锁定
	XM_VideoItemLock ();
		
	do
	{
		if(h == NULL || video_volume_count == 0)
		{
			break;
		}
		
		ret = 0;
		// 遍历视频项数据库
		// 定位开始序号
		for (i = rbtBegin(h); i != rbtEnd(h); )
		{
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = (XMVIDEOITEM *)keyp;
			
			//XM_printf("video_item->video_name =%s,video_item->volume_index=%d,video_item->video_channel=%d\n",(char *)video_item->video_name,video_item->volume_index,video_item->video_channel);
			
			// 属性匹配
			// 通道号
			if(video_channel != 0xFFFFFFFF && video_channel != video_item->video_channel)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// 磁盘卷
			if(volume_index != 0xFFFFFFFF && volume_index != video_item->volume_index)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// 文件类型
			if(file_type != 0xFFFFFFFF && file_type != video_item->file_type)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// 录像分类
			if(!(stream_class_bitmask & (1 << video_item->stream_class)))
			{
				i = rbtNext(h, i);
				continue;
			}

			if(XM_VideoItemMakeFullPathFileName ((char *)video_item->video_name, video_item->volume_index, video_item->video_channel, video_item->file_type, video_item->stream_class, file_name, sizeof(file_name)) < 0)
			{
				ret = -1;
				break;
			}

			AP_VideoItemLockVideoFile (video_item);
			
			if(fp_user_callback)
			{
				if( (*fp_user_callback)(user_data, file_name) == 1 )	// 终止当前的锁定操作，返回
				{
					ret = 1;
					break;
				}
			}
			// 获取下一个节点，避免当前节点删除后，下一个节点信息异常。
			i = rbtNext(h, i);
		}
	} while (0);
	
	// 解锁
	XM_VideoItemUnlock ();
	
	return ret;		
}

// 将符合属性定义组合的视频项全部解锁
// 返回值
//		0		成功
//		-1		失败
int XM_VideoItemUnlockVideoItem (
											 unsigned int volume_index,			// 磁盘卷号, 0xFFFFFFFF表示所有卷
											 unsigned int video_channel, 			// 通道号， 0xFFFFFFFF表示所有通道
											 unsigned int file_type,				// 文件类型, 0xFFFFFFFF表示所有文件类型
											 unsigned int stream_class_bitmask,	// 录像分类掩码，0xFFFFFFFF表示所有录像
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL 表示包含普通及手动分类
											 int (*fp_user_callback) (void *user_data, char *full_path_name_to_delete),
											 												// 用户回调函数， 用于用户取消某个长时间操作。为0表示无回调函数
																							//		返回值为1表示终止当前的操作并退出，返回值为0表示继续当前的操作
																							//		user_data 传入的用户私有数据
																							//		full_path_name_to_delete 准备删除的视频项全路径文件名
											 void *user_data							//	用户私有数据
											)
{
	RbtIterator i;
	void *keyp, *valuep;
	XMVIDEOITEM *video_item;
	char file_name[VIDEOITEM_MAX_FILE_NAME];
	int ret;
	
	if(volume_index != 0xFFFFFFFF && volume_index >= video_volume_count)
		return -1;
	
	if(video_channel != 0xFFFFFFFF && video_channel >= XM_VIDEO_CHANNEL_COUNT)
		return -1;
	
	if(file_type != 0xFFFFFFFF && file_type >= XM_FILE_TYPE_COUNT)
		return -1;	

	// 检查视频项服务是否已开启	
	if(!XM_VideoItemIsServiceOpened())
		return -1;	

	memset (file_name, 0, sizeof(file_name));
	ret = -1;
	// 锁定
	XM_VideoItemLock ();
		
	do
	{
		if(h == NULL || video_volume_count == 0)
		{
			break;
		}
		
		ret = 0;
		// 遍历视频项数据库
		// 定位开始序号
		for (i = rbtBegin(h); i != rbtEnd(h); )
		{
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = (XMVIDEOITEM *)keyp;
			
			//XM_printf("video_item->video_name =%s,video_item->volume_index=%d,video_item->video_channel=%d\n",(char *)video_item->video_name,video_item->volume_index,video_item->video_channel);
			
			// 属性匹配
			// 通道号
			if(video_channel != 0xFFFFFFFF && video_channel != video_item->video_channel)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// 磁盘卷
			if(volume_index != 0xFFFFFFFF && volume_index != video_item->volume_index)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// 文件类型
			if(file_type != 0xFFFFFFFF && file_type != video_item->file_type)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// 录像分类
			if(!(stream_class_bitmask & (1 << video_item->stream_class)))
			{
				i = rbtNext(h, i);
				continue;
			}

			if(XM_VideoItemMakeFullPathFileName ((char *)video_item->video_name, video_item->volume_index, video_item->video_channel, video_item->file_type, video_item->stream_class, file_name, sizeof(file_name)) < 0)
			{
				ret = -1;
				break;
			}

			AP_VideoItemUnlockVideoFile (video_item);
			
			if(fp_user_callback)
			{
				if( (*fp_user_callback)(user_data, file_name) == 1 )	// 终止当前的锁定操作，返回
				{
					ret = 1;
					break;
				}
			}
			// 获取下一个节点，避免当前节点删除后，下一个节点信息异常。
			i = rbtNext(h, i);
		}
	} while (0);
	
	// 解锁
	XM_VideoItemUnlock ();
	
	return ret;		
}