//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_videoitem.c
//	  ��Ƶ�б�ӿ�
//
//	Revision history
//
//		2011.12.16	ZhuoYongHong Initial version
//		2018.04.30	ZhuoYongHong ��Ϊʹ�ó��ļ���������Ƶ���ļ�
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
#pragma warning(disable:4127) // VC�²���ʾ4127�ž�����Ϣ
#pragma warning(disable:4115) // VC�²���ʾ4127�ž�����Ϣ
#endif

#define	VIDEO_ITEM_FOLDER_NAMING_PROFILE_2	1

// ·������
//
// ����1 (VIDEO_ITEM_FOLDER_NAMING_PROFILE_1)
//
//    VIDEO_F		
//    VIDEO_B
//
//
// ����2 (VIDEO_ITEM_FOLDER_NAMING_PROFILE_2)
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
// ��ȡCacheϵͳæ�ȴ��ļ���
// 0 ~ 1 ��ʾϵͳCache��������, ��������
// 2 ~ 8 ����Խ��, ��ʾʹ�õ�Cache����Խ��
// 9     ��ʾ�ڲ�Cache����, ��ʱ�޷�д��
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
#define	MAX_VIDEO_ITEM_COUNT				8192			// ������Ƶ2���ӷֶ�ʱ����11���¼ 4096*2/60/24=11.4��

#define	MAX_VIDEO_CHANNEL_PATH_NAME		32			// ��Ƶͨ��·��������ֽڳ���(��������\0)

#define	MAX_VIDEO_CHANNEL_VOLUME_NAME		16

#define	MAX_VIDEO_CHANNEL_COUNT				8			// ��Ƶͨ����������

#define	MAX_VIDEO_TEMP_FILE_COUNT			16

static const char videoitem_class_id[XM_VIDEOITEM_CLASS_COUNT] = {'N', 'N', 'U'};
static const char *videoitem_file_type_id[XM_FILE_TYPE_COUNT] = {"AVI", "JPG"};

__no_init static videoitem_s   rbt_pool[MAX_VIDEO_ITEM_COUNT];
static queue_s rbt_free;		// ���еĵ�Ԫ

#define	SERIAL_NUMBER_TYPE_SHARE		0		// ÿ����Ƶ�����͹�����ͬ�����, ����Ƭ����Ƶʹ��ͬһ����ż���
#define	SERIAL_NUMBER_TYPE_ALONE		1		// ÿ����Ƶ�����;��ж��������
static unsigned int serial_number_type = SERIAL_NUMBER_TYPE_SHARE;

// ��Ź���
static unsigned int video_item_serial_number[XM_VIDEOITEM_VOLUME_COUNT][XM_FILE_TYPE_COUNT][XM_VIDEO_CHANNEL_COUNT];	
static unsigned int video_item_ordering = XM_VIDEOITEM_ORDERING_SERIAL_NUMBER;		// ���򷽷�
static unsigned int channel_reverse_ordering = 1;		// ͨ������������ 1 ����(ͨ��0�������) 0 ˳��(ͨ��0����ǰ��)

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

// ��Ƶ��ķ���������ʱ����������, ʹ��ȫ�ֶѷ���ʱ�������Ƭ, �Ӷ������������ڴ����ʧ��. 
// ʹ�þ�̬�Ľṹ��������Ƶ��,  ����������Ƭ.
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


// ʹ���ļ�������
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
			ret = memcmp ((char *)ta + 16, (char *)tb + 16, VIDEOITEM_MAX_FULL_NAME - 16);	// ����ſ�ʼ�Ƚ�
	}
	else
	{
		if(video_item_ordering == XM_VIDEOITEM_ORDERING_NAME)
			ret = memcmp ((char *)a, (char *)b, VIDEOITEM_MAX_FULL_NAME);
		else
			ret = memcmp ((char *)a + 16, (char *)b + 16, VIDEOITEM_MAX_FULL_NAME - 16);	// ����ſ�ʼ�Ƚ�
	}
	
	ret = -ret;
	return ret;
}


static OS_RSEMA videoItemMutexSema;			// ��Ƶ�������ʻ��Ᵽ��
static OS_RSEMA videoItemMonitorSema;			// ��Ƶ���ؼ����շ�����ʻ��Ᵽ��

static OS_EVENT codecStartEvent;			// ¼������ͬ���¼�
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

// ��Ƶ�����ȴ�"�����������¼�"
// ����ֵ 
//		0	 �����������¼���ָ��ʱ���ڱ�����
//    -1	 �����������¼���ָ��ʱ����û�б�����
int XM_CodecStartEventWait (unsigned int milliseconds_to_timeout)		// �ȴ������������¼�
{
	int ret = OS_EVENT_WaitTimed (&codecStartEvent, milliseconds_to_timeout);
	if(ret == 0)
		return 0;
	else
		return -1;
}

// ���"�����������¼�"
void XM_CodecStartEventReset (void)
{
	OS_EVENT_Reset (&codecStartEvent);
}

// "�ⲿ����������"���øú���֪ͨ"��Ƶ�����"�������������������
// "��Ƶ�����"�ȴ����¼������յ����¼���"��Ƶ�����"����������ʱ�Ĵ�����Ϣ��ȡ����Ƶɨ����̣�
//	��Щ���̻ᵼ���ļ�ϵͳ�����������������"�ⲿ����������"���ļ���������������
void XM_CodecStartEventSet (void)			// ���ñ����������¼�
{
	OS_EVENT_Set (&codecStartEvent);
}


static RbtHandle h;


// ·������
static unsigned int		video_path_count;			// ��Ƶͨ������(���ⲿӦ�ö��壬�����޸�)
static char 	video_path_name[XM_VIDEOITEM_VOLUME_COUNT][XM_VIDEO_CHANNEL_COUNT][XM_FILE_TYPE_COUNT][XM_VIDEOITEM_CLASS_COUNT][MAX_VIDEO_CHANNEL_PATH_NAME];	// ·����������������'\'�ַ�
static char 	video_path_valid[XM_VIDEOITEM_VOLUME_COUNT][XM_VIDEO_CHANNEL_COUNT][XM_FILE_TYPE_COUNT][XM_VIDEOITEM_CLASS_COUNT];		// ��Ч��־�� 1��ʾ��Ŀ¼����Ч�� 0��ʾ��Ŀ¼�������

// �����
static DWORD	fat_cluster_size[XM_VIDEOITEM_VOLUME_COUNT];	// �ļ�ϵͳ���ֽڴ�С
static int		video_volume_count;		// ��Ƶʹ�õĴ��̾����(���ⲿӦ�ö��壬�����޸�)
													//		Ϊ���Ż�д�����ܻ�ȫ��¼���󣬻�ʹ��2������SD����Ϊ�洢�ռ�
static char		video_volume_valid[XM_VIDEOITEM_VOLUME_COUNT];	// ��Ǿ��Ƿ���Ч, 1 ����Ч 0 ����Ч

//static int		video_temp_index;

static int		basic_service_opened = 0;		// �������������־�� 
															//		1 ��ʾ���������ѿ�������Ƶ������Կ�ʼ¼��
															//		0 ��ʾ���������ѹرգ���Ƶ�����޷����ʴ���

// ÿ�λ�������ɹ��򿪣��ۼӸ�ֵ�����ڱ�ǵ�ǰ�ķ���ID��
//		���´�ĳ�����VIDEO ITEM���ݿ���ۼ���þ��Ӧ��basic_service_idֵ
//		Ӧ��ʹ�ø�ֵ�ж��Ƿ������Ƶ�����ڹر�/�򿪵Ĺ���(��ӦSD���İγ�/�������)
static unsigned int		basic_service_id;			
																						
static volatile int		delay_cutoff_flag;				// �ӳ��ļ�ɾ������

static volatile int		delay_recycle_clean;			// �Ƿ�������ִ�еĻ���վ������� 1 �������ڵĻ���վ������� 0 û��

static volatile int		block_file_delete;			// �����ļ�ɾ������

static volatile int videoitem_service_opened;	// ��ʾ��Ƶ�����з������ı�־
													//	1 ��ʾ��Ƶ������ѿ���
static int		videoitem_debug = 1;

static int		disk_free_space_busy;		// ��Ǵ���ʣ��ռ��Ƿ�������ʵʱ��¼, 1 ����ʣ��ռ䲻��,�޷�����ʵʱ��¼ 0 ��������ʵʱ��¼

static int		automatic_safe_save = 0;	// �����ڶ����ʱ���ⲿSD��д��ʧ��ʱ��ϵͳ�Զ��л������õ�eMMC������֤¼��ȫд��

// ***** ��Ƶ����������Ϊ2����, ���̿ռ估��Ŀ������*********
//		(���ݴ��̾�, �ļ����ͼ�¼�Ʒ���ָ�)
// 1) ��Ƶ���ļ�������������0ֵ��ʾ����������ƣ���ʾ����������Ŀ������������������ƣ��������Զ����
static unsigned int item_count_quota[XM_VIDEOITEM_VOLUME_COUNT][XM_FILE_TYPE_COUNT][XM_VIDEOITEM_CLASS_COUNT];
// 2) ��Ƶ����̿ռ�������(��¼�Ʒ��ඨ��)����0ֵ��ʾ���ڸ�������ƣ���ʾ���������ֽ�������������������ƣ��������Զ����
static XMINT64	disk_space_quota_level_class[XM_VIDEOITEM_VOLUME_COUNT][XM_FILE_TYPE_COUNT][XM_VIDEOITEM_CLASS_COUNT];
//    ��Ƶ����̿ռ�������(���ļ����Ͷ���)����0ֵ��ʾ���ڸ�������ƣ���ʾ���������ֽ�������������������ƣ��������Զ����
static XMINT64	disk_space_quota_level_type[XM_VIDEOITEM_VOLUME_COUNT][XM_FILE_TYPE_COUNT];
//    ��Ƶ����̿ռ�������(�����̾���)��  ��0ֵ��ʾ���ڸ�������ƣ���ʾ���������ֽ�������������������ƣ��������Զ����
static XMINT64	disk_space_quota_level_volume[XM_VIDEOITEM_VOLUME_COUNT];

static XMINT64 minimum_space_limit;

// 	�ɻ��տռ��������, С�ڸ�ֵ������
static XMINT64 recycle_space_limit;

// ***** ��Ƶ���¼����
// 1) ��Ƶ���ļ�������¼
static volatile int		 video_item_count;		// ��Ƶ���¼����
static int		 class_item_count[XM_VIDEOITEM_VOLUME_COUNT][XM_FILE_TYPE_COUNT][XM_VIDEOITEM_CLASS_COUNT];		// ��Ƶ�����ͳ�Ƽ�¼����

// 2) ��Ƶ����̿ռ��¼
static XMINT64	video_item_recycle_space;							// ��ѭ�������ֽڴ�С
static XMINT64	video_item_locked_space;							// ��ѭ�������ֽڴ�С(�����ռ�)
// �ռ�ռ����Ϣͳ��
static XMINT64 video_item_space[XM_VIDEOITEM_VOLUME_COUNT][XM_FILE_TYPE_COUNT][XM_VIDEOITEM_CLASS_COUNT];		// �ֽ�ͳ����Ϣ


// ��Ƶ����̿ռ��¼
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

// ��Ƶ���ļ�������¼
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


// ����SD����Ż�ȡ����ľ�����
//		VolumeIndex		�洢�����(��0��ʼ)
// ����ֵ
//		NULL		ʧ��
//		�ǿ�ֵ	���ַ�����
const char *XM_VideoItemGetVolumeName (unsigned int VolumeIndex)
{
	if(VolumeIndex == 0)
		return "mmc:0:";
	else if(VolumeIndex == 1)
		return "mmc:1:";
	else
		return "";	// ���ؿմ�
}

// ����ͨ���Ż�ȡ�������Ƶ��ͨ������
//		VideoChannel	��Ƶͨ�����(��0��ʼ)
//		FileType			�ļ�����(��Ƶ����Ƭ)
// ����ֵ
//		NULL		ʧ��
//		�ǿ�ֵ	ͨ���ַ�����
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



// �����ļ����ͻ�ȡ������ļ�������
const char *XM_VideoItemGetFileTypeName (unsigned int FileType)
{
	if(FileType >= XM_FILE_TYPE_COUNT)
		return NULL;	// ���ؿմ�
	if(FileType == XM_FILE_TYPE_VIDEO)
		return "VIDEO";
	else if(FileType == XM_FILE_TYPE_PHOTO)
		return "PHOTO";
	else
		return "";	// ���ؿմ�
}

// ����¼�����ͻ�ȡ�����¼��������
const char *XM_VideoItemGetStreamClassName (unsigned int StreamClass)
{
	if(StreamClass >= XM_VIDEOITEM_CLASS_COUNT)
		return NULL;	// ���ؿմ�
	else if(StreamClass == XM_VIDEOITEM_CLASS_NORMAL)
		return "NORMAL";
	else if(StreamClass == XM_VIDEOITEM_CLASS_MANUAL)
		return "MANUAL";
	else if(StreamClass == XM_VIDEOITEM_CLASS_URGENT)
		return "URGENT";
	else
		return "";	// ���ؿմ�
}

// ��ȡ��ǰ��Ƶ�����ݿ�ķ���ID��ÿ�λ�������ɹ��򿪣��ۼӸ÷���ID��
// 20180626 ����һ���Լ�飬������ٿ��β嵼�µ��쳣��
//		ĳЩ�����£�recycle�߳�(��̨���У����ȼ���)��һ���ļ�ɾ�������л�Ƕ��"���γ��������²���"�Ĺ��̣���ʱ����ϵͳ�һ���
//		����һ���Լ�飬����ùһ�����
unsigned int XM_VideoItemGetServiceID (void)
{
	return basic_service_id;
}

// ͨ��ȫ·���ļ���������Ƶ������ͨ����
HANDLE XM_VideoItemGetVideoItemHandle (const char *lpVideoFile, int* pVideoChannel)
{
	char path_name[VIDEOITEM_MAX_FULL_PATH_NAME];
	char *ch;
	NodeType * i = NULL;
	unsigned int channel;
	
	// �����Ƶ������Ƿ��ѿ���	
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

// ͨ��ȫ·������ʱ��Ƶ���ļ������Ҷ�Ӧ����Ƶͨ����
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
	
		// ��ȡ·����
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
	// ����Ƿ�����ʱ��Ƶ���ļ�
	if(!strstr (lpVideoFileName, "TEMP"))
		return 0;
	return 1;	
}

// ��ȡָ�����FAT�ش�С(�ֽڵ�λ)
static DWORD get_fat_cluster_size_from_volume (unsigned volume_index)
{
	if(volume_index >= video_volume_count)
		return 0;

	return fat_cluster_size[volume_index];
}



// ����SD�� Class10 Toshiba
// 1) ʹ��XMSYS_FAT_FAST_FILE_REMOVE���㷨(�Ż���ɾ����������ͬ����ԭ�򽫴��ͷ�)������10����(�ļ�д�뼰�ļ�ɾ������ִ��)
//	��ʾ����ͳ����Ϣ (�ο�get_dirty_cache_block_count����)
//		1  826%   6348
//    2  123%    945
//    3   10%    83
// 2) δʹ��XMSYS_FAT_FAST_FILE_REMOVE������10����(�ļ�д�뼰�ļ�ɾ������ִ��)
//	��ʾ����ͳ����Ϣ (�ο�get_dirty_cache_block_count����)
//    1  614%  4405
//    2  278%  1999
//    3   22%   159
// 3) ���Կ������κ�ʱ�̣�Cache�е�������Ϊ1,2,3�ı����� 91.4% ���ӵ� 95.9%�����κ�ʱ�̵��磬�����������Ϣ�����ˡ�

// �����㷨���Ը����ļ�ɾ�����µ�H264������������
// ����ֵ����
// 1 �ɹ�
// 0 ʧ��
// 10������Ƶ¼���ɾ��ʱ�佫�ӳ������20��
static int _cutoff_file (const char *filename)
{
#ifdef WIN32
	remove (filename);
	return 1;
#else
	extern int FS_CutOffFile (FS_FILE * pFile);

	int ret = 0;
	FS_FILE *fp;
	unsigned int service_id;		// �������ID
	
	// ACC�µ�ʱ���������ļ�ɾ������
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
		// ����Ƿ�����ӻ��ļ�ɾ������������. ������,��������󲢵ȴ�0.1��. 
		while(delay_cutoff_flag)
		{
			delay_cutoff_flag = 0;
			//XM_Sleep (100);
			XM_Sleep (200);
		}
		// ����ʱ�������õ����ļ�ɾ������
		//XMSYS_file_system_waiting_for_cache_system_nonbusy_state (3000);	
		XMSYS_file_system_waiting_for_cache_system_nonbusy_state (1000);
		XM_VideoItemLock ();		// ��ֹ����unmount
		if(!XM_VideoItemIsBasicServiceOpened())
		{
			XM_VideoItemUnlock ();
			XM_printf ("FS_CutOffFile end, service closed\n");
			break;
		}
		// ������ID�Ƿ���ͬ������ͬ����ֹɾ��
		if(XM_VideoItemGetServiceID() != service_id)
		{
			// ��Ƶ�����ݿ��ѱ����ͬ��SD��Ҳ�ѱ��
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
			// �쳣
			XM_VideoItemUnlock ();
			break;			
		}
		
		// �������Ƿ������, 1��ʾ���ѻ������
		if(fs_ret)
		{
			ret = 1;		// ��Ǵ�ɾ���ѳɹ�
			XM_VideoItemUnlock ();
			break;
		}
		XM_VideoItemUnlock ();
		// ���������������̼߳�Cache�̸߳�����ἰʱ���ȡ��Ƶ�����ݿ����Ȩ����������
		XM_Sleep (2);
	}
	
	// ����ⲿACC�Ƿ���ȫ
	//if(xm_power_check_acc_safe_or_not() == 0)
	//{
		// ��ִ��Ŀ¼��ɾ������, ��ֹACC������ɵ�Ŀ¼����.
		// �˴���ִ��ɾ������������ɴؿռ䶪ʧ
		//return 0;
	//}
	
	
	// �ļ�ϵͳ��ȫ��������
	if(XMSYS_file_system_block_write_access_lock () == 0)
	{
		// ����ⲿACC�Ƿ���ȫ
		if(xm_power_check_acc_safe_or_not() == 1)
		{
			// �޸Ĳ�ɾ��Ŀ¼��
			FS_FClose (fp);
			ret = XM_RemoveFile (filename);
			if(ret == 1)
			{
				// ����Ŀ¼��
				FS_CACHE_Clean("");	
			}
		}
		else
		{
			ret = 0;
		}
		
		// �ļ�ϵͳ��ȫ���ʽ���	
		// XMSYS_file_system_block_write_access_unlock ();	

	}
	else
	{
		// �ǰ�ȫ����
		ret = 0;
	}
	
	// �ļ�ϵͳ��ȫ���ʽ���	
	XMSYS_file_system_block_write_access_unlock ();	
	
	// �˴����谲��Cache��д. ��Ϊ��ʣ��Ŀ¼�����
	//FS_CACHE_Clean("");		
	return ret;	
#endif
}

// ����ֵ
// 1 �ɹ�
// 0 ʧ��
static int cutoff_file (const char *filename)
{
	// ���Ƚ��ļ�������Ϊ00000000.TMP, Ȼ����ִ��ɾ������.
	// ����ɾ����������Ϊ"�ο����ߵ���"���µ�ԭɾ���ļ��������ݴ���, ������Ƭ/��Ƶ�������ʾ�쳣������.
	// 20170319 ���ļ�������Ϊ00000000.TMP���ڷ���.
	//		��2���ļ�ɾ������ͬʱ����ʱ, ���� _cutoff_file����ͬʱ����, ����������ϵͳ
	//			1) XMSYS_RecycleTask ɾ��ѭ����
	//			2) H264FileTask ɾ��С��1�����Ƶ
	//		��Ϊ���ļ���׺������ΪTMP����ɾ��
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
		// �Ƿ��ļ���, ����չ��
		return 0;
	}
	// strcat (del_filename, "\\00000000.TMP");
	strcat (del_filename, ".TMP");
	ch = strrchr (del_filename, '\\');
	if(ch)
		strcpy (tempname, ch + 1);
	else
	{
		// �Ƿ��ļ���, ��·��
		return 0;		
	}
	
	// ���ͬ������ʱ�ļ��Ƿ����. ������,����ɾ��
	// �ļ�ϵͳ��ȫ��������
	if(XMSYS_file_system_block_write_access_lock() == 0)
	{
		fp = XM_fopen (del_filename, "rb");
		if(fp)
		{
			XM_fclose (fp);
			XMSYS_file_system_block_write_access_unlock();
			// ɾ��֮ǰ��ͬ���ļ�
			if(_cutoff_file (del_filename) == 0)
			{
				// ɾ��ʧ��
				// ֱ��ɾ��Ҫɾ�����ļ�
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
	
	// ��ɾ���ļ�����Ϊ��ʱ�ļ�
	//if(XM_rename(filename, "00000000.TMP") < 0)
	if(XM_rename(filename, tempname) < 0)
	{
		// ����ʧ��
		return _cutoff_file (filename);
	}
	else
	{
		// �����ɹ�
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
		// ����ֻ���ļ��޷�ɾ��
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

			// �����ļ��б������Ϣ(�ļ�ɾ��)
			// 20170702 �����б�����ѹر�
			// �˲�����Ϊ�����ȡÿ����Ƶ�ļ�����ȡ����Ƶ¼��ʱ������¿�����Ч���½�
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
		// �����½ڵ�
		// ����Ƿ��Ѵﵽ������Ƶ���¼����
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
		// �ۼ��ļ���С(����Ϊ��λ)
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
		
		// �������ֵ
		if(serial_number_type == SERIAL_NUMBER_TYPE_SHARE)
		{
			// ÿ����Ƶ�����͹�����ͬ�����
			if(serial_number > video_item_serial_number[volume_index][0][channel_index])
				video_item_serial_number[volume_index][0][channel_index] = serial_number;
		}
		else
		{
			// ÿ����Ƶ�����;��ж��������
			if(serial_number > video_item_serial_number[volume_index][file_type][channel_index])
				video_item_serial_number[volume_index][file_type][channel_index] = serial_number;
		}

		// �ۼ���Ƶ�����
		video_item_count ++;
		videoitem_inc_count (video_item, video_item->stream_class);
	}
	
	return 1;
}

// �����ϱ�׼��������Ƶ�ļ����뵽��Ƶ�ļ��б�
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
		// �ļ���������ֻ��������
		if(*ch >= '0' && *ch <= '9' || *ch >= 'A' && *ch <= 'Z' || *ch == '_')
		{
			name_count ++;
			if(name_count > VIDEOITEM_LFN_SIZE)
				return 0;
		}
		else if(*ch == '.')
		{
			// ����Ƿ��Ǳ�׼�����ļ���
			if(name_count != VIDEOITEM_LFN_SIZE)
				return 0;
			ch ++;
			break;
		}
		else
		{
			// �Ǳ�׼�����ַ�
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
		// 20180928 �˴���ɾ��TMP�ļ�
		return 0;	// ����
#if 0
		// ɾ����ʱ�ļ�
		char FullFileName[XM_MAX_FILEFIND_NAME];
		unsigned int file_type; 

		if(videoitem_extract_info (file_name, NULL, 0, &file_type, NULL, &volume_index, &channel_index, NULL, NULL) < 0)
			return 0;
		
		if(XM_VideoItemMakeFullPathFileName (file_name, volume_index, channel_index, file_type, stream_class, FullFileName, sizeof(FullFileName)) < 0)
			return 0;
		if(XM_RemoveFile (FullFileName))
			return 1;	// OK
		else
			return 0;	// ����
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
		// ��Ч���ļ���
		// ����ļ������Ƿ�Ϊ0. ����, ɾȥ0�ֽڴ�С���ļ�
		if(dwFileSize == 0)
		{
			char FullFileName[VIDEOITEM_MAX_FULL_PATH_NAME];
			if(XM_VideoItemMakeFullPathFileName (file_name, volume_index, channel_index, file_type, stream_class, FullFileName, sizeof(FullFileName)) < 0)
				return 0;
			if(XM_RemoveFile (FullFileName))
				return 1;	// OK
			else
				return 0;	// ����
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
				return 0;	// ����			
		}
		//XM_printf(">>>>inser photo, file_name:%s\r\n", file_name);
		
		// ��Ƭ�ļ�
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


	// ����¼����
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
					// ��ͨ��ֶ��������������ڸ��Զ�����Ŀ¼��
					video_path_valid[volume_index][channel_index][file_type][stream_class] = 1;
					#else
					// ��ͨ��ֶ���������������ͬһ��Ŀ¼��
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
	
	// ���������ź���
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

// ��ȡ��Ƶ�ļ�ͨ��(·��)�ĸ���
int XM_VideoItemGetVideoFileChannel (void)
{
	return video_path_count;
}

// SD���γ��󣬹ر���Ƶ�����
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
	
	delay_recycle_clean = 0;		// �������վ��������ı�־

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
		// ��ȡ�ļ�����ʱ��
		if(XM_GetFileCreateTime (lpVideoFileName, create_time) < 0)
			return -1;
		create_time->wMilliSecond = 0;
	}
	
	// ���
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
	
	// ͨ����
	if(channel_index)
	{
		if(*ch >= '0' && *ch < ('0' + XM_VIDEO_CHANNEL_COUNT))
			*channel_index = *ch - '0';
		else
			return -1;
	}
	ch ++;
	// ���̺�
	if(volume_index)
	{
		if(*ch >= '0' && *ch < ('0' + video_volume_count))
			*volume_index = *ch - '0';
		else
			return -1;
	}
	ch ++;
	// ¼�Ʒ���
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

// ����Ƶ���ļ���ӵ���Ƶ�����ݿ�
// 	ch					ͨ�����(0, 1, ...)
// 	lpVideoFile		ȫ·����Ƶ�ļ���
// ����ֵ
//	1		�ɹ�
//	0		ʧ��
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
		// ���Ŀ¼�����������Ƿ��ѿ���
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
			// �ļ�����ʱ����û�к����������0
			system_time.wMilliSecond = 0;
						
			// ��ͨ�ļ�
			if( append_one_video (filename, class_index, 0, file_size, &system_time) == 0 )
			{
				ret = 1;
			}
		}
	} while(0);
	
	XM_VideoItemUnlock ();
	return ret;
}

// �������SD���Ƿ����
int XMSYS_check_sd_card_exist (void);

// ��鿨����Ƶд������
// ����ֵ
//		-1		�޷���ȡ����ֵ
//		0		��Ƶ���ܿ��ܽϲ�
//		1		��Ƶ���������¼Ҫ��
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
			if(fat_cluster_size[index] <= (512 * 32))		// 16, ʹ�ø��������Ĵش�С, �Ż�FAT�ռ����/�ͷ��ٶ�
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

// ���ָ�����Ŀ���ѭ��¼��ռ�����(= ���̿����ֽ����� + ѭ��¼����Ƶռ�õ��ļ��ֽ�����)�Ƿ�������͵�¼��Ҫ��
// ����ֵ
//		-1		�޷���ȡ�����ܲ���
//		0		�޷��������¼��Ҫ��, ��ʾ��ʽ����
//		1		�������¼��Ҫ��
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
			
			XMINT64	available_videoitem_space = 0;	// ��������Ƶ���¼�Ŀ��пռ�		
			
			volume_name = (char *)XM_VideoItemGetVolumeName (volume_index);
			if(volume_name == NULL)
				break;
						
			if(XM_GetDiskFreeSpace ("\\", &SectorsPerCluster, &BytesPerSector,
						&NumberOfFreeClusters, &TotalNumberOfClusters, volume_name) == 0)
			{
				break;
			}
			
			// ����ѭ��ʹ�ÿռ�����+���пռ�����
			available_videoitem_space = NumberOfFreeClusters;
			available_videoitem_space *= SectorsPerCluster;
			available_videoitem_space *= BytesPerSector;
			available_videoitem_space += video_item_recycle_space;
			
			// ��顰ѭ��¼��ռ䡱�Ƿ��������¼������5����, ����12Mbps��������
			if(available_videoitem_space  < (540*1024*1024))
			{
				// �޷��������¼��Ҫ��
				XM_printf ("volume(%s)'s total recycle space (%d) MB is too small\n", volume_name, (unsigned int)(available_videoitem_space/(1024*1024)));
				ret = 0; 
			}
			else
			{
				// �����������¼��Ҫ��
				ret = 1;
			}
		}
	} while (0);
	
	XM_VideoItemUnlock ();
	return ret;		
}


// ��鿨�������տռ�����
// ����ʣ����������տռ�����
XMINT64 XM_VideoItemCheckCardPhotoSpaceEx (void)
{
	int i;
	unsigned int volumn_index;
	int quota_check_result;
	XMINT64 total_space;
	XMINT64 quota;
	unsigned int class_index;
	
	// �����Ƶ������Ƿ��ѿ���	
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
	
	// ����Ƿ���Ҫִ�л���վ�������
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
		// ����ɨ���Ƿ������
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		// ��Ƭ�ռ���	
		// �����ڸ��ļ����͵���������Ƿ����
		quota = disk_space_quota_level_type[volumn_index][XM_FILE_TYPE_PHOTO];
		if(quota)
		{
			// ��0ֵ��ʾ��������
			// �ۼӸ��ļ����͵�����¼�Ʒ��������
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


// ��鿨0�Ŀ���ѭ��¼��ռ�����(= ���̿����ֽ����� + ѭ��¼����Ƶռ�õ��ļ��ֽ�����)�Ƿ�������͵�¼��Ҫ��
// ����ֵ
//		-1		�޷���ȡ�����ܲ���
//		0		�޷��������¼��Ҫ��, ��ʾ��ʽ����
//		1		�������¼��Ҫ��
int XM_VideoItemCheckCardRecycleSpace (void)
{
	return XM_VideoItemCheckCardRecycleSpaceEx (XM_VIDEOITEM_VOLUME_0);
}

// ��鿨�������տռ�����
// ����ʣ����������տռ�����
XMINT64 XM_VideoItemCheckCardPhotoSpace (void)
{
	return XM_VideoItemCheckCardPhotoSpaceEx ();
}

// ��鲢������Ƶ��·��
static int check_and_create_video_folder_old (unsigned int volume_index)
{
	unsigned int channel_index;
	unsigned int file_type;
	unsigned int stream_class;
	char VideoPath[VIDEOITEM_MAX_FULL_PATH_NAME];
	XMFILEFIND fileFind;
	char fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	int ret = -1;
	for (channel_index = XM_VIDEO_CHANNEL_0; channel_index < video_path_count; channel_index ++)
	{
		for (file_type = XM_FILE_TYPE_VIDEO; file_type < XM_FILE_TYPE_COUNT; file_type ++)
		{
			// ���ͬ����Ŀ¼���ļ��Ƿ����
			if(XM_VideoItemMakePathNameEx(volume_index, channel_index, file_type, VideoPath, sizeof(VideoPath)) < 0)
				return -1;
			if(XM_FindFirstFile ((char *)VideoPath, &fileFind, fileName, XM_MAX_FILEFIND_NAME))
			{
				// ͬ��Ŀ¼���ҵ�
				XM_FindClose (&fileFind);
				continue;
			}
			else
			{
				// ������Ŀ¼
				if(check_and_create_directory(VideoPath) != 0)
				{
					if(videoitem_debug)
						XM_printf ("failed to create video folder(%s)\n", VideoPath);
					
					// �˳���ͨ��UI��ʾ�û����޷���¼
					return -1;
				}
			}
		}
	}
	return 0;
}

// ��鲢������Ƶ��·��
static int check_and_create_video_folder (unsigned int volume_index)
{
	unsigned int channel_index;
	unsigned int file_type;
	unsigned int stream_class;
	char *VideoPath;
	XMFILEFIND fileFind;
	char fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
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
					
					// ���ͬ����Ŀ¼���ļ��Ƿ����
					if(XM_FindFirstFile ((char *)VideoPath, &fileFind, fileName, XM_MAX_FILEFIND_NAME))
					{
						// ͬ��Ŀ¼���ҵ�
						XM_FindClose (&fileFind);
						continue;
					}
					else
					{
						// ������Ŀ¼
						if(check_and_create_directory(VideoPath) != 0)
						{
							if(videoitem_debug)
								XM_printf ("failed to create video folder(%s)\n", VideoPath);
							
							// �˳���ͨ��UI��ʾ�û����޷���¼
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
	char fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	char FullPathName[VIDEOITEM_MAX_FULL_PATH_NAME + 1];
	unsigned int scan_ticket = XM_GetTickCount();
	
	do
	{		
		int file_count = 0;
		int err_count = 0;
		// ����ɨ��������Ƶ·��
			
#ifdef _WINDOWS
		strcat (VideoPath, "\\*.*");
#endif
		if(XM_FindFirstFile (VideoPath, &fileFind, fileName, XM_MAX_FILEFIND_NAME))
		{
			do
			{
				DWORD dwFileAttributes;
				// ���Ŀ¼������
				file_count ++;
				if(file_count >= 65536)	// ����65536���ļ�
				{
					XM_printf ("scan_video_item_file Failed, too many file in video folder\n");
				video_folder_error:
					// ������Ϊ�����𻵵��ļ�Ŀ¼����������Ӱ��FAT����Ч��, ��ʾ��ʽ��
					XM_FindClose (&fileFind);
					return -1;
				}
				dwFileAttributes = fileFind.dwFileAttributes;
				// XM_printf ("name=%s, attr=0x%x\n", fileName, dwFileAttributes);
				// ����Ŀ¼���Ƿ����ļ�����
				if((dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == 0x00)
				{
					// ��ͨ�ļ�
					//XM_printf ("%d, %s, %s\n", ch, fileName, (dwFileAttributes & D_RDONLY) ? "Locked" : "Normal");
					// 20180722 zhuoyonghong
					// 	�ֶ�������ɨ��ʱǿ�Ƽ���ֻ�����ԣ������ļ������������޸�δ��ͬʱ��ɶ����µ��ֶ������ļ�ȱ��ֻ���������õ�bug
#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2
					if(stream_class == XM_VIDEOITEM_CLASS_NORMAL)
						fileFind.dwFileAttributes &= ~XM_FILE_ATTRIBUTE_READONLY;
					else if(stream_class == XM_VIDEOITEM_CLASS_MANUAL)
						fileFind.dwFileAttributes |= XM_FILE_ATTRIBUTE_READONLY;
#endif
					if(append_one_video (fileName, stream_class, (BYTE)(fileFind.dwFileAttributes), fileFind.nFileSize, &fileFind.CreationTime) == 0)
					{
						// 20180928 ����Ƿ���TMP�ļ������ǣ�ֱ��ɾ��
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
						// �ļ������쳣/ͬ����
						err_count ++;
					}
				}
				else if( dwFileAttributes & 0xC0 )	// ���Ŀ¼���Ը�2λ�Ƿ�Ϊ0. ����Ϊ0, �����쳣
				{
					// The upper two bits of the attribute byte are reserved and should
					//	always be set to 0 when a file is created and never modified or
					//	looked at after that.
					// �ļ������쳣
					err_count ++;
				}
				else if( (dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == ATTR_DIRECTORY)
				{
					// Ŀ¼
					if(xm_stricmp (fileName, "RECYCLE") == 0)
					{
						// ����վĿ¼
						// ������վ�ļ����е�ͼƬȫ��ɾ��
						// char recycle_path[XM_MAX_FILEFIND_NAME];
						// sprintf (recycle_path, "%s\\%s", VideoPath, fileName);
						// XM_RecycleCleanFile (recycle_path);
						// ����ִ�л���վ�������
						delay_recycle_clean = 1;		// �����Ҫִ�л���վ�������
					}
				}
				if(err_count >= 1024)
				{
					XM_printf ("scan_video_item_file Failed, too many directory entry error in video folder\n");
					goto video_folder_error;
				}
					
				// ���ACC�����Ƿ�ȫ
				if(!xm_power_check_acc_safe_or_not())
					break;
					
			} while(XM_FindNextFile (&fileFind));
			XM_FindClose (&fileFind);
		}
		else
		{
			// ��Ƶ����Ŀ¼�޷�����
			XM_printf ("scan video's dir (%s) failed\n", VideoPath);
			return -1;
		}

		// ˢ���ļ�ϵͳ, ��������ɨ��������������ж��ļ�ϵͳ�ĸ���д�뵽�ļ�ϵͳ
		FS_CACHE_Clean ("");

	} while (0);
	scan_ticket = XM_GetTickCount() - scan_ticket;
	//XM_printf ("scan_video_path spend %d ms\n", scan_ticket);	
	return 0;	
}

// ɨ������Ƶ���ļ���
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
				// ����Ӧ��·���Ƿ���Ч��Ψһ
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
			
// ����������Ƶ��ͳ����Ϣ			
static void calculate_videoitem_statistics (void)
{
	RbtIterator i;
	XMVIDEOITEM *video_item;
	memset (&video_item_space, 0, sizeof(video_item_space));
	// ɨ��������Ƶ�ͳ���ļ���Ϣ�������ѭ�����õĿռ�
	for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
	{
		void *keyp, *valuep;
		DWORD cluster_size;
		rbtKeyValue(h, i, &keyp, &valuep);
		// XM_printf("%d %d\n", *(int *)keyp, *(int *)valuep);
		video_item = (XMVIDEOITEM *)keyp;
		XM_ASSERT (video_item);
		// ����video_size
		cluster_size = get_fat_cluster_size_from_volume (video_item->volume_index);
		if(cluster_size)
			video_item->video_size = ((video_item->file_size + cluster_size - 1) / cluster_size) * cluster_size;
		if(video_item->video_lock)
		{
			// ֻҪ��������һ��ͨ���������������Ƶ������(��ͬʱ���������)
			// �ۼ���������Ƶ�ļ����ֽڴ�С
			video_item_locked_space += video_item->video_size;
			//XM_printf ("locked file size(%lld),  locked_space(%lld)\n",  video_item->video_size, video_item_locked_space);
		}
		else
		{
			// ֻҪ��������һ��ͨ���������������Ƶ������(��ͬʱ���������)
			// �ۼӿɻ���(ѭ��)��Ƶ�ļ��ֽڴ�С
			video_item_recycle_space += video_item->video_size;
			//XM_printf ("100 recycle + file size(%lld),  recycle_space(%lld)\n",  video_item->video_size, video_item_recycle_space);
		}
		
		assert (video_item->volume_index < video_volume_count);
		assert (video_item->file_type < XM_FILE_TYPE_COUNT);
		assert (video_item->stream_class < XM_VIDEOITEM_CLASS_COUNT);
		
		// ����Ƶ���ļ���С��Ϣ�����ļ����ͣ�¼�����͵ȷ���ͳ��
		videoitem_inc_space (video_item, video_item->stream_class);
		//video_item_space[video_item->volume_index][video_item->file_type][video_item->stream_class] += video_item->video_size;
	}	
}
			

// ���������SD��������ɨ��������Ƶ�ļ�, ������Ƶ�����
int XM_VideoItemOpenService (void)
{	
	char VideoPath[VIDEOITEM_MAX_FULL_PATH_NAME];
	char fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
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
	
	//XMINT64	available_videoitem_space = 0;	// ��������Ƶ���¼�Ŀ��пռ�


	// ͳ��ɨ��ʱ��
	DWORD dwScanTicket;
	
	//int ret = 0;
	
	if(videoitem_debug)
		XM_printf ("XM_VideoItemOpenService\n");

	// ǿ�ƹر�
	XM_VideoItemCloseService ();

	XM_VideoItemLock ();
	// ����ÿ���������Ƶ��·�����������ڲ�����
	//XM_printf ("ticket=%d, service folder check\n", XM_GetTickCount ());
	for (volume_index = XM_VIDEOITEM_VOLUME_0; volume_index < video_volume_count; volume_index ++)
	{
		if(check_and_create_video_folder (volume_index) < 0)
		{
			// �˳���ͨ��UI��ʾ�û����޷���¼
			//return -1;
		}
		video_volume_valid[volume_index] = 1;
	}	
			
	
	// ������ٴ���һ����Ч��
	for (volume_index = XM_VIDEOITEM_VOLUME_0; volume_index < video_volume_count; volume_index ++)	
	{
		if(video_volume_valid[volume_index])
			break;
	}
	
	if(volume_index == video_volume_count)
	{
		// �˳���ͨ��UI��ʾ�û����޷���¼
		XM_VideoItemUnlock ();
		XM_printf ("XM_VideoItemOpenService faile, no valid volume\n");
		return -1;
	}
	
	// ���´򿪷���
	XM_ASSERT (h == NULL);
	h = rbtNew(compare);
	if(h == NULL)
	{
		// ϵͳ�쳣���ڴ����ʧ��
		XM_printf ("XM_VideoItemOpenService NG, rbtNew failed\n");
		//XM_ASSERT (0);
		XM_VideoItemUnlock ();

		// ϵͳ�쳣���ڴ����ʧ�ܣ�ϵͳ�޷�¼�񡣴�ʱ��Ҫ֪ͨϵͳ����������
		XM_ShutDownSystem (SDS_REBOOT);
		return (-2);
	}
	video_item_count = 0;
	memset (class_item_count, 0, sizeof(class_item_count));
	video_item_recycle_space = 0;
	video_item_locked_space = 0;
	memset (&video_item_space, 0, sizeof(video_item_space));

	// ɨ��ÿ������������Ƶ���ļ����뵽��Ƶ�����ݿ�
	//XM_printf ("scan file\n");
	for (volume_index = 0; volume_index < video_volume_count; volume_index ++)
	{
		if(scan_video_item_file (volume_index) < 0)
		{
			// ɨ�跢���쳣
			video_volume_valid[volume_index] = 0;		// ���ľ���Ϊ��Ч������ʹ��
		}
	}
	
	// ������Ƶ��¼��ʼ������"������"�ȴ�basic_service_opened��־��1����ܿ�ʼ¼��
	basic_service_opened = 1;
	videoitem_service_opened = 1;
	
	basic_service_id ++;

	XM_VideoItemUnlock ();
	
	//video_temp_index = 0;

	XM_CodecStartEventReset ();		// ��������������¼�
	
	
	//XM_printf ("ticket=%d, basic service opened\n", XM_GetTickCount ());

	
	// ��ʱ1�룬�ȴ�codec����
	// ���뿨���ҿ�����ȷʶ��¼�񼴿�ʼ(�κ�״����)��
	//
	// ��ʱ�ȴ�¼���̷߳�����¼�������ź�,  ��Ҫ����GetDiskFreeSpace�Ứ�ѽϳ���ʱ��, �ȴ�¼���߳̿�ʼ����ִ�к�ʱ�Ĳ�����
//	XM_Sleep (1000);
	index = 0;
	while(index < 20)
	{
		if(XM_CodecStartEventWait (50) == 0)
			break;

		// ��鿨�Ƿ�γ�
		// ��鿨������״̬
		if(XMSYS_check_sd_card_exist() == 0)
		{
			XM_printf ("the card does not exist\n");
			// ���Ѱγ�
			return -1;
		}
		index ++;
	}
	
	// ---------------------------------------------
	// ******** ��������ɨ�� ����ʱ������***********
	// ---------------------------------------------
	XM_VideoItemLock ();
	//XM_printf ("ticket=%d, get disk space\n", XM_GetTickCount ());

	// ɨ��ÿ����Ƶ��·������ȡÿ�����������Ϣ
	// for (index = 0; index < video_path_count; index ++)
	// ��ɨ���һ��·��
	for (index = 0; index < video_volume_count; index ++)
	{
		char *volume = (char *)XM_VideoItemGetVolumeName (index);
		assert(volume);
		
		// �����Ĵش�С
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
			// ��Ǿ�����
			fat_cluster_size[index] = 0;
		}
		//XM_printf ("XM_GetDiskFreeSpace end\n");
	}

	// ͳ����Ƶ��Ŀռ�ռ����Ϣ
	calculate_videoitem_statistics ();
	
	
	XM_VideoItemUnlock ();
	
	// ��������߳�����ִ�л��մ���
	XMSYS_DoRecycle ();



	//XM_printf ("ItemMonitor\n");
	// �ر�ִ����Ƶ����չ��� XM_VideoItemMonitorAndRecycleGarbage.
	// 	����Ƶ����չ��̽�����, ��ʱ�γ�SD��, ��Ϊmessage��������ִ����Ƶ�������(���� XM_VideoItemOpenService),
	//		��������Ϣ�Ĵ���, ����ϵͳ����.
	// ¼������ʱ�Ὺ�� XM_VideoItemMonitorAndRecycleGarbage
	// XM_VideoItemMonitorAndRecycleGarbage (0);

	//videoitem_service_opened = 1;
	
	//XM_VideoItemUnlock ();
	
	//XM_printf ("ticket=%d, service start\n", XM_GetTickCount());
	return 0;
}

// ������Ƶ������(�����ԣ��ļ��������ԣ�ͨ���ż�¼�Ʒ����)��ȡ�������Ե���Ƶ�������
//		1)	��ȡ��0�ϵ�ǰ����ͷ����ͨ¼����Ƶ�ļ�����
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_0, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL);
//		2) ��ȡ��0�ϵ�����ͨ���Ľ���¼��(G-Sensor)��Ƶ�ļ�����
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, 0xFFFFFFFF,  XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		3) ��ȡ��1�ϵ�����ͨ���ı�����Ƶ(�ֶ�¼�ƣ�����¼��)�ļ�����
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_1, 0xFFFFFFFF, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_MANUAL|XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		4) ��ȡ��0�ϵĺ�����ͷ��������Ƭ�ļ�����
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_1, XM_FILE_TYPE_PHOTO, 0xFFFFFFFF);
DWORD XM_VideoItemGetVideoItemCount(unsigned int volume_index, 				// SD�������(��0��ʼ), 0xFFFFFFFF��ʾ���о�
									unsigned int video_channel, 			// ͨ���ţ� 0xFFFFFFFF��ʾ����ͨ��
									unsigned int file_type, 				// �ļ�����(��Ƶ����Ƭ, 0xFFFFFFFF��ʾ�����ļ�����)
									unsigned int stream_class_bitmask		// ¼��������룬0xFFFFFFFF��ʾ����¼��
										 									// XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL ��ʾ������ͨ���ֶ�����
									)
{
	int ret = -1;
	int item_count = 0;
	XMVIDEOITEM *video_item;
	RbtIterator i;

	// �����Ƶ������Ƿ��ѿ���	
	if(!XM_VideoItemIsServiceOpened())
		return -1;
	
	// ����
	XM_VideoItemLock ();
	do
	{
		if(h == NULL || video_volume_count == 0)
		{
			break;
		}
		
		// ������Ƶ�����ݿ�
		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			void *keyp, *valuep;
			
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = (XMVIDEOITEM *)keyp;
			
			// ����ƥ��
			// ���̾�
			if(volume_index != 0xFFFFFFFF && volume_index != video_item->volume_index)
				continue;
			// ͨ����
			if(video_channel != 0xFFFFFFFF && video_channel != video_item->video_channel)
				continue;
			// �ļ�����
			if(file_type != 0xFFFFFFFF && file_type != video_item->file_type)
				continue;
			// ¼�Ʒ���
			if(stream_class_bitmask & (1 << video_item->stream_class))	
				item_count ++;
		}
		
		ret = item_count;
	} while (0);
	
	// ����
	XM_VideoItemUnlock ();
	
	return ret;		
}

// ���ݶ�������Ի�ȡָ������Ƶ����
//		0) ��ȡ��0�ϵĺ�����ͷ������������Ƭ(�������)����Ƶ����
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_1, XM_FILE_TYPE_PHOTO, 0xFFFFFFFF, 0�� 1 );
// ����ֵ
// == 0 ʧ��
// != 0 �������б����¼���
HANDLE XM_VideoItemGetVideoItemHandleEx (
											 unsigned int volume_index,			// ���̾��, 0xFFFFFFFF��ʾ���о�
											 unsigned int video_channel, 			// ͨ���ţ� 0xFFFFFFFF��ʾ����ͨ��
											 unsigned int file_type,				// �ļ�����, 0xFFFFFFFF��ʾ�����ļ�����
											 unsigned int stream_class_bitmask,	// ¼��������룬0xFFFFFFFF��ʾ����¼��
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL ��ʾ������ͨ���ֶ�����
											 unsigned int index,						// ��ţ���0��ʼ
											 unsigned int reverse_time_ordered	// ��ʱ�䷴������ 1 ��������(ʱ��Ӵ���С) 0 ˳������(ʱ���С����)
											)
{
	HANDLE handle;
	unsigned int item_index = 0;
	XMVIDEOITEM *video_item;
	DWORD dwTotalCount;
	RbtIterator i;
	void *keyp, *valuep;

	// �����Ƶ������Ƿ��ѿ���	
	if(!XM_VideoItemIsServiceOpened())
		return NULL;
	
	dwTotalCount = XM_VideoItemGetVideoItemCount (volume_index, video_channel, file_type, stream_class_bitmask);
	if(index >= dwTotalCount)
		return NULL;
	
	if(reverse_time_ordered == 0)
		index = dwTotalCount - 1 - index;
	
	handle = NULL;
	// ����
	XM_VideoItemLock ();
	do
	{
		if(h == NULL || video_volume_count == 0)
		{
			break;
		}
		
		// ������Ƶ�����ݿ�
		// ��λ��ʼ���
		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = (XMVIDEOITEM *)keyp;
			// ����ƥ��
			// ͨ����
			if(video_channel != 0xFFFFFFFF && video_channel != video_item->video_channel)
				continue;
			// ���̾�
			if(volume_index != 0xFFFFFFFF && volume_index != video_item->volume_index)
				continue;
			// �ļ�����
			if(file_type != 0xFFFFFFFF && file_type != video_item->file_type)
				continue;
			// ¼�����
			if(!(stream_class_bitmask & (1 << video_item->stream_class)))
				continue;
			
			if(item_index == index)
			{
				// �ҵ�ƥ��Ŀ�ʼ�����
				handle = i;
				break;
			}
				
			item_index ++;			
		}
		
	} while (0);
		
	// ����
	XM_VideoItemUnlock ();
			
	return handle;	
}

// ��ȡ��Ƶ�б��ļ�����
// mode ָ��ѭ��¼��ģʽ���Ǳ���¼��ģʽ
DWORD AP_VideoItemGetVideoFileCount (BYTE mode)
{
	RbtIterator i;
	XMVIDEOITEM *video_item;
	DWORD dwCount;
	
	// �����Ƶ������Ƿ��ѿ���	
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

			// ���ģʽ�Ƿ��Ǳ���¼��ģʽ
			if(mode == VIDEOITEM_PROTECT_VIDEO_MODE)
			{
				if(video_item->video_lock)
					dwCount ++;
			}
			else
			{
				// ѭ��¼��ģʽ
				dwCount ++;
			}
		}
		
	} while (0);

	XM_VideoItemUnlock ();

	return dwCount;
}

// ��ȡ��Ƶ�б��ļ�����
// mode ָ��ѭ��¼��ģʽ���Ǳ���¼��ģʽ
// nVideoChannel	ָ����Ƶͨ��, 0 ǰ������ͷ 1 ��������ͷ
DWORD AP_VideoItemGetVideoFileCountEx (BYTE mode, unsigned int nVideoChannel)
{
	RbtIterator i;
	XMVIDEOITEM *video_item;
	DWORD dwCount;
	
	// �����Ƶ������Ƿ��ѿ���	
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
			
			
			// ���ģʽ�Ƿ��Ǳ���¼��ģʽ����ͨ��
			if(mode == VIDEOITEM_PROTECT_VIDEO_MODE)
			{
				if(video_item->video_lock)
					dwCount ++;
			}
			else
			{
				if(video_item->video_channel == nVideoChannel)
				{
					// ѭ��¼��ģʽ
					dwCount ++;
				}
			}
			
		}
		
	} while (0);

	XM_VideoItemUnlock ();

	return dwCount;
}

// ��ȡ��Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ��Ŀ��� (��ʱ��Ӵ�С)
// dwVideoIndex ��0��ʼ
HANDLE AP_VideoItemGetVideoItemHandleReverseOrder (BYTE mode, DWORD dwVideoIndex)
{
	RbtIterator i;
	XMVIDEOITEM *video_item;
	DWORD dwCount;

	// �����Ƶ������Ƿ��ѿ���	
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

			// ���ģʽ�Ƿ��Ǳ���¼��ģʽ
			if(mode == VIDEOITEM_PROTECT_VIDEO_MODE)
			{
				if(video_item->video_lock)
					dwCount ++;
			}
			else
			{
				// ѭ��¼��ģʽ
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

// ��ȡ��Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ��Ŀ��� (��ʱ��Ӵ�С)
// dwVideoIndex ��0��ʼ
// nVideoChannel ��Ƶͨ���� 0 ǰ������ͷ 1 ��������ͷ
HANDLE AP_VideoItemGetVideoItemHandleReverseOrderEx (BYTE mode, DWORD dwVideoIndex, unsigned int nVideoChannel)
{
	RbtIterator i;
	XMVIDEOITEM *video_item;
	DWORD dwCount;

	// �����Ƶ������Ƿ��ѿ���	
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
				// ���ģʽ�Ƿ��Ǳ���¼��ģʽ
				if(mode == VIDEOITEM_PROTECT_VIDEO_MODE)
				{
					if(video_item->video_lock)
						dwCount ++;
				}
				else
				{
					// ѭ��¼��ģʽ
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

// ��ȡ��Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ��Ŀ���(��ʱ���С����)
// dwVideoIndex ��0��ʼ
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

	// �����Ƶ������Ƿ��ѿ���	
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

			// ���ģʽ�Ƿ��Ǳ���¼��ģʽ
			if(mode == VIDEOITEM_PROTECT_VIDEO_MODE)
			{
				if(video_item->video_lock)
					dwCount ++;
			}
			else
			{
				// ѭ��¼��ģʽ
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

// ��ȡ��Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ��Ŀ���(��ʱ���С����)
// dwVideoIndex ��0��ʼ
// nVideoChannel ��Ƶͨ���� 0 ǰ������ͷ 1 ��������ͷ
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

	// �����Ƶ������Ƿ��ѿ���	
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
             
			// ���ģʽ�Ƿ��Ǳ���¼��ģʽ
			if(mode == VIDEOITEM_PROTECT_VIDEO_MODE)
			{
				if(video_item->video_lock)
					dwCount ++;
			}
			else
			{
				if(video_item->video_channel == nVideoChannel)
				{
					// ѭ��¼��ģʽ
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

// ����Ƶ������ȡ��Ƶ��ṹָ��
XMVIDEOITEM * AP_VideoItemGetVideoItemFromHandle (HANDLE hVideoItemHandle)
{
	XMVIDEOITEM *video_item;
	void *keyp, *valuep;

	// �����Ƶ������Ƿ��ѿ���	
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

// ��ȡǰһ����Ƶ������, -1��ʾ������
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

// ��ȡ��һ����Ƶ������, -1��ʾ������
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

// ��ʾ��ʽ��ָ����Ƶ��������Ƶ�ļ���
char *AP_VideoItemFormatVideoFileName (XMVIDEOITEM *lpVideoItem, char *lpDisplayName, int cbDisplayName)
{
	const char *name;
	if(h == NULL)
		return NULL;

	// �����Ƶ������Ƿ��ѿ���	
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

// ��ȡ��Ƶ����ʱ�䳤����Ϣ
DWORD AP_VideoItemGetVideoStreamSize (XMVIDEOITEM *lpVideoItem, unsigned int nVideoChannel)
{
	char video_file[VIDEOITEM_MAX_FILE_NAME];
	void *xmfile;
	double video_time = 0.0;
	
	// �����Ƶ������Ƿ��ѿ���	
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
		// �����ʱ����Ϣ�Ƿ��ѻ�ȡ
		if(lpVideoItem->stream_length_valid)
		{
			// �ѻ�ȡ
			video_time = lpVideoItem->stream_length;
			break;
		}
		// δ��ȡ, �����ļ��ж�ȡ
		memset (video_file, 0, sizeof(video_file));
		
		// ����Ƿ����´�����Ƶ(����¼�Ƶ���Ƶ��ʱ�޷���ȡʱ��)
		if(!(lpVideoItem->video_update & (1 << nVideoChannel)))
			break;
			
		if(lpVideoItem->video_channel == nVideoChannel)
			XM_VideoItemGetVideoFilePath (lpVideoItem, nVideoChannel, video_file, 64);
		else
			break;
	
#if _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_MKV_
		// ��MKV�ļ���0x160ƫ�ƴ���ȡʱ�䳤����Ϣ��64λ�ֽڣ�		
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
			// �����ʱ����Ч, 
			lpVideoItem->stream_length_valid = 1;
			lpVideoItem->stream_length = (DWORD)video_time;
		}
		break;
#elif _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_AVI_
		// ��֡��, �ļ�ƫ��0x30, 4�ֽ�
		xmfile = XM_fopen(video_file, "rb");
		if(xmfile)
		{
			unsigned int total_frames = 0;
			unsigned int micro_sec_per_frame;		// ��Ƶ֡���ʱ�䣨��΢��Ϊ��λ��
			if(XM_fseek (xmfile, 0x20, XM_SEEK_SET) >= 0)
			{
				XM_fread (&micro_sec_per_frame, 1, 4, xmfile);
			}
			// ��֡��, �ļ�ƫ��0x30, 4�ֽ�
			if(XM_fseek (xmfile, 0x30, XM_SEEK_SET) >= 0)
			{
				XM_fread (&total_frames, 1, 4, xmfile);
				video_time = (total_frames * micro_sec_per_frame + 500) / 1000;
			}
			XM_fclose (xmfile);
			// �����ʱ����Ч, 
			lpVideoItem->stream_length_valid = 1;
			lpVideoItem->stream_length = (DWORD)video_time;
		}	
		break;
#else
		// �����ʱ����Ч, 
		lpVideoItem->stream_length_valid = 1;
		lpVideoItem->stream_length = 0;
		break;;
#endif
	} while (0);
	XM_VideoItemUnlock ();
	
	return (DWORD)video_time;
}

// ��ȡ��Ƶ��ͨ����Ӧ���ļ�·��
const char * XM_VideoItemGetVideoPath (unsigned int nVideoChannel)
{
	if(nVideoChannel >= XM_VIDEO_CHANNEL_COUNT)
		return NULL;
	
	//return video_path_name[XM_VIDEOITEM_VOLUME_0][nVideoChannel];
	return video_path_name[XM_VIDEOITEM_VOLUME_0][nVideoChannel][XM_FILE_TYPE_VIDEO][XM_VIDEOITEM_CLASS_NORMAL];
}

// ��ȡ��Ƶ��ͨ����Ӧ���ļ�·��,��������
const char * XM_VideoItemGetVideoPathByType(unsigned int nVideoChannel, unsigned char type)
{
	if(nVideoChannel >= XM_VIDEO_CHANNEL_COUNT)
		return NULL;
	
	//return video_path_name[XM_VIDEOITEM_VOLUME_0][nVideoChannel];
	return video_path_name[XM_VIDEOITEM_VOLUME_0][nVideoChannel][type][XM_VIDEOITEM_CLASS_NORMAL];
}

// ��ȡ��Ƶ��ָ��ͨ����ȫ·���ļ���
XMBOOL XM_VideoItemGetVideoFilePath (XMVIDEOITEM *lpVideoItem, unsigned int nVideoChannel, char *lpFileName, int cbFileName)
{
	XMBOOL ret = 0;

	// �����Ƶ������Ƿ��ѿ���	
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

// ����Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ�ļ�����(����) (��������ͨ��)
// ע�⣺
//		һ������ִ��ʱ��Ƶ�����Ӧ����Ƶ�ļ����ܻ�û�д���(�ļ�ϵͳ��Ŀ¼�û�д���)
XMBOOL AP_VideoItemLockVideoFile (XMVIDEOITEM *lpVideoItem)
{
	char FileName[VIDEOITEM_MAX_FILE_NAME];
	unsigned int channel;
	XMBOOL ret = 0;

	// �����Ƶ������Ƿ��ѿ���	
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
		// �طŹ��̽�ֹ��������������
		if(lpVideoItem->video_ref_count)
			break;
#endif
		
		ret = 1;
		// ����Ƿ��ǽ���¼�����ͣ��������
		if(lpVideoItem->stream_class == XM_VIDEOITEM_CLASS_URGENT)
			break;
		// ����Ƿ�������(����һ��ͨ����������)
		if(lpVideoItem->video_lock)
		{
			break;
		}

		memset (FileName, 0, sizeof(FileName));
		XM_VideoItemGetVideoFilePath (lpVideoItem, lpVideoItem->video_channel, FileName, sizeof(FileName));
	
#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2
		if(lpVideoItem->video_update)
		{
			// �ļ��Ѹ��£�����Ŀ¼
			char NewFileName[VIDEOITEM_MAX_FILE_NAME];
			if(XM_VideoItemMakeFullPathFileName ((const char *)lpVideoItem->video_name, lpVideoItem->volume_index, lpVideoItem->video_channel, lpVideoItem->file_type,
															 XM_VIDEOITEM_CLASS_MANUAL, NewFileName, sizeof(NewFileName)) < 0)
			{
				ret = 0;
				break; 
			}
			// �ļ���NORMALĿ¼�ƶ���MANUALĿ¼��
			if(FS_Move (FileName, NewFileName) < 0)
			{
				ret = 0;
			}
			XM_printf ("class rename from %s to %s %s\n", FileName, NewFileName, ret == 1 ? "OK" : "NG");
			if(ret == 0)
				break;
		}		
#else
		// ����ļ��Ƿ����
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
		// �����Ƶ���Ӧ����Ƶ�����ļ��Ƿ��Ѵ������ر�
		if(lpVideoItem->video_update)
		{
			// ��Ƶ���Ӧ����Ƶ�����ļ��Ѵ������ر�
			// ��ȥ�������ļ��Ŀռ�
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
		// ���޸ļ�ʱͬ���������ļ�ϵͳ
		FS_CACHE_Clean ("");
	}

	return (XMBOOL)ret;
}

// ��ָ���ļ���(ȫ·��)����Ƶ����
XMBOOL AP_VideoItemLockVideoFileFromFileName (const char *lpVideoFileName)
{
	HANDLE hVideoHandle;
	XMBOOL ret = 0;
	XMVIDEOITEM *video_item = NULL;
	void *keyp = NULL, *valuep = NULL;
	int nVideoChannel;
	
	// ����Ƿ�����ʱ��Ƶ���ļ�
	if(XM_VideoItemIsTempVideoItemFile (lpVideoFileName))
	{
		return XM_SetFileAttributes (lpVideoFileName, XM_FILE_ATTRIBUTE_ARCHIVE | XM_FILE_ATTRIBUTE_READONLY);
	}
	
	// �����Ƶ������Ƿ��ѿ���	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	// XM_VideoItemLock����ͬһ���߳��ڶ�ε��ã�ֻ��ƥ���Ӧ��XM_VideoItemUnlock����
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

// ����Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ�ļ�����(��ѭ������)
XMBOOL AP_VideoItemUnlockVideoFile (XMVIDEOITEM *lpVideoItem)
{
	char FileName[VIDEOITEM_MAX_FILE_NAME];
	unsigned int channel;
	XMBOOL ret = 0;

	// �����Ƶ������Ƿ��ѿ���	
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
		// �طŹ��̽�ֹ��������������
		if(lpVideoItem->video_ref_count)
			break;
#endif

		ret = 1;
		// ����Ƿ��ǽ���¼�����ͣ��������
		if(lpVideoItem->stream_class == XM_VIDEOITEM_CLASS_URGENT)
			break;
		// ����Ƿ��ѽ���
		if(!lpVideoItem->video_lock)
			break;

		memset (FileName, 0, sizeof(FileName));
		XM_VideoItemGetVideoFilePath (lpVideoItem, lpVideoItem->video_channel, FileName, sizeof(FileName));
		// ����ļ��Ƿ����
#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2
		if(lpVideoItem->video_update)
		{
			// �ļ��Ѵ������ر�
			char NewFileName[VIDEOITEM_MAX_FILE_NAME];
			if(XM_VideoItemMakeFullPathFileName ((const char *)lpVideoItem->video_name, lpVideoItem->volume_index, lpVideoItem->video_channel, lpVideoItem->file_type,
															 XM_VIDEOITEM_CLASS_NORMAL, NewFileName, sizeof(NewFileName)) < 0)
			{
				ret = 0;
				break; 
			}
			// �ļ���MANUALĿ¼�ƶ���NORMALĿ¼��
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
		// �����Ƶ���Ӧ����Ƶ�����ļ��Ƿ��Ѵ������ر�
		videoitem_dec_count (lpVideoItem, XM_VIDEOITEM_CLASS_MANUAL);
		videoitem_inc_count (lpVideoItem, XM_VIDEOITEM_CLASS_NORMAL);
		if(lpVideoItem->video_update)
		{
			// �ۼӱ������ļ��Ŀռ�
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
		// ���޸ļ�ʱͬ���������ļ�ϵͳ
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
	
	// ����Ƿ�����ʱ��Ƶ���ļ�
	if(XM_VideoItemIsTempVideoItemFile (lpVideoFileName))
	{
		return XM_SetFileAttributes (lpVideoFileName, XM_FILE_ATTRIBUTE_ARCHIVE);
	}
	
	// �����Ƶ������Ƿ��ѿ���	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	// XM_VideoItemLock����ͬһ���߳��ڶ�ε��ã�ֻ��ƥ���Ӧ��XM_VideoItemUnlock����
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


// ��ȡ��һ����ѭ�����ǵ���Ƶ�ļ�����
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
			// ���������(����������)
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = valuep;
			
			// �����ļ������Ƿ�һ��
			if(video_item->file_type != file_type)
				continue;
			if(video_item->volume_index != volume_index)
				continue;
			if( !((1 << video_item->stream_class) & stream_class_bitmask) )		// ����������Ƿ����
				continue;

			// ��Ƶ���Ӧ����Ƶ�ļ��Ѵ�������û������
			// if(!video_item->video_lock && video_item->video_update)
			// ��Ƶ���Ӧ����Ƶ�ļ��Ѵ�������Ϊ��ͨ¼������
			//if(video_item->stream_class == XM_VIDEOITEM_CLASS_NORMAL && video_item->video_update)
			if(video_item->video_update)
			{
				next = i;
				// û��ͨ������
				//break;
			}
		}
	} while (0);

	XM_VideoItemUnlock ();

	return next;
}

// �����Ƶ����������Ƿ��ѿ���������������������Ƶ��¼
XMBOOL XM_VideoItemIsBasicServiceOpened (void)
{
	return (XMBOOL)basic_service_opened;
}

// �����Ƶ������Ƿ��ѿ���
XMBOOL XM_VideoItemIsServiceOpened (void)
{
	return (XMBOOL)videoitem_service_opened;
}

// ��ȡ����Ƶ��(ֻ����1·��Ƶ)��ͨ�����
// >= 0  ���ص�ͨ�����
// < 0   ��Ч�ľ��������Ƶ������һ��������Ƶ��(���ͨ�������)
int XM_VideoItemGetVideoChannel (HANDLE hVideoItemHandle)
{
	int ret = -1;
	RbtIterator i;
	void *keyp, *valuep;
	XMVIDEOITEM *video_item;
	unsigned int channel;

	// �����Ƶ������Ƿ��ѿ���	
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

// ��֤����ͨ����Ÿ��µ�һ����
static unsigned int videoitem_get_serial_number (unsigned int volume_index, unsigned int file_type, unsigned int video_channel)
{
	unsigned int max_id;
	if(serial_number_type == SERIAL_NUMBER_TYPE_SHARE)
	{
		// ��ͬ¼�����͹���ͬһ�����
		file_type = 0;
	}
	
	if(video_channel == 0)
	{
		// ͨ��0
		// ��������ͨ�������ֵmax_id��Ȼ��max_id + 1���µ�ͨ��0
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
		// ��0ͨ��
		// �����ۼӵ�ǰͨ�������ֵ
		video_item_serial_number[volume_index][file_type][video_channel] ++;
		// ���ÿ��ͨ�������ֵ�����㵱ǰ�������ֵ
		max_id = 0;
		for (unsigned int i = 0; i < video_channel; i ++)
		{
			if(video_item_serial_number[volume_index][file_type][i] > max_id)
				max_id = video_item_serial_number[volume_index][file_type][i];
		}
		// ��������д�뵽��ǰͨ��
		if(max_id > video_item_serial_number[volume_index][file_type][video_channel])
			video_item_serial_number[volume_index][file_type][video_channel] = max_id;
	}
	return video_item_serial_number[volume_index][file_type][video_channel];
}

// ������Ƶ������(ͨ���ţ�����ʱ�䣬�ļ����ͣ���Ƶ����࣬����ľ����)����һ����Ƶ����
//		1) ����һ��λ�ڿ�0�ϵ�ͨ��0�Ľ���¼�����͵�¼����Ƶ��
//			XM_VideoItemCreateVideoItemHandleEx (XM_VIDEO_CHANNEL_0, create_time, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_URGENT, XM_VIDEOITEM_VOLUME_0);
//		2) ����һ��λ�ڿ�0�ϵ�ͨ��1����ͨ¼�����͵���Ƭ��Ƶ��
//			XM_VideoItemCreateVideoItemHandleEx (XM_VIDEO_CHANNEL_1, create_time, XM_FILE_TYPE_PHOTO, XM_VIDEOITEM_CLASS_NORMAL, XM_VIDEOITEM_VOLUME_0);
//		3) ����һ��λ�ڿ�1�ϵ�ͨ��1���ֶ�¼�����͵���Ƶ��Ƶ��
//			XM_VideoItemCreateVideoItemHandleEx (XM_VIDEO_CHANNEL_1, create_time, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_MANUAL, XM_VIDEOITEM_VOLUME_1);
// ����ֵ
//		0			ʧ��
//		��0ֵ		��Ƶ����
HANDLE XM_VideoItemCreateVideoItemHandleEx (unsigned int video_channel,		// ͨ����
														  XMSYSTEMTIME *videoitem_create_time, 		// ��Ƶ���ʱ��
														  unsigned int file_type,			//	��Ƶ���ļ�����(��Ƶ����Ƭ)	
														  unsigned int videoitem_class, 	// ��Ƶ������(�������ֶ����߽���)
														  unsigned int video_volume_index	// �����SD�������(��0��ʼ)
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
	
	// �����Ƶ������Ƿ��ѿ���	
	if(!XM_VideoItemIsServiceOpened())
		return NULL;
	
	if(videoitem_create_time)
		create_time = *videoitem_create_time;
	else
		XM_GetLocalTime (&create_time);
		
	// 20180522 ȱʡ������ʱ����Ϊ0, ��ͬ����Ƶ�ļ�����ʱ���ۼӸ�ֵ���⸲�Ǿɵ���Ƶ�ļ�
	//videoitem_create_time->wMilliSecond = 0;
	create_time.wMilliSecond = 0;

	// �����Ƶ������Ƿ��ѿ���

	XM_VideoItemLock ();
	
	if(video_volume_valid[video_volume_index] == 0)
	{
		// д�����Ч
		if(automatic_safe_save)
		{
			// ������һ����ȫ�Ŀ��þ�
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
			// ʹ�ð�ȫ�ľ�
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
		
		// �����Ƶ�������Ƿ��Ѵﵽϵͳ��������ֵ
		if(video_item_count >= MAX_VIDEO_ITEM_COUNT)
		{
			if(videoitem_debug)
				XM_printf ("CreateVideoItemHandleEx NG, Video Item Exceed Maximum count %d\n", MAX_VIDEO_ITEM_COUNT);
			// ��鲢���տ�ѭ����
			//XM_VideoItemUnlock ();
			//XM_VideoItemMonitorAndRecycleGarbage (0);
			//XM_VideoItemLock ();
			if(video_item_count == last_video_item_count)		// ���պ��ޱ仯���쳣�˳�
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

		// �����½ڵ�
		video_item = (XMVIDEOITEM *)videoitem_malloc ();
		if(video_item == NULL)
		{
			//XM_printf ("videoitem_malloc XMVIDEOITEM failed\n");
			break;
		}
		memset (video_item, 0, sizeof(XMVIDEOITEM));

		video_item->serial_number = videoitem_get_serial_number (video_volume_index, file_type, video_channel);
		// ��ȡ��Ƶ������
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

// ����һ���µ���Ƶ����
//   (ȱʡΪ��Ƶ�ļ�����ͨ���࣬��0)
// nVideoChannel = 0, 1, 2, 3 ... ��ʾ����һ��ָ��ͨ������Ƶ��
HANDLE XM_VideoItemCreateVideoItemHandle (unsigned int VideoChannel, XMSYSTEMTIME *filetime)
{
	return XM_VideoItemCreateVideoItemHandleEx (VideoChannel, filetime, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_NORMAL, XM_VIDEOITEM_VOLUME_0);
}

// ɾ��һ����Ƶ����(����ɾ��������ͨ������Ƶ�ļ�)
XMBOOL XM_VideoItemDeleteVideoItemHandle (HANDLE hVideoItemHandle, VIDEOFILETOREMOVE *lpVideoFileToRemove)
{
	RbtIterator i;
	void *keyp, *valuep;
	XMVIDEOITEM *video_item;
	unsigned int channel;
	char file_name[VIDEOITEM_MAX_FILE_NAME];
	XMBOOL ret;
	
	XM_VIDEOITEM_EVENT videoitem_event;
	
	// �����Ƶ������Ƿ��ѿ���	
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

		// �����Ƶ���Ӧ�Ĵ����ļ��Ƿ��Ѵ������ر�
		if(!video_item->video_update)
		{
			XM_printf ("Can't delete video(%s, ref_count=%d) which don't updated before\n", video_item->video_name, video_item->video_ref_count);
			break;
		}
		
		// �����Ƶ���Ƿ�������(�����ڲ���)
		if(video_item->video_ref_count)
		{
			XM_printf ("Can't delete video item which mark usage now\n");
			break;
		}
		
		{
			if( XM_VideoItemGetVideoFilePath(video_item, video_item->video_channel, file_name, VIDEOITEM_MAX_FILE_NAME) )
			{
				// �޸��ļ�����
				if(video_item->video_lock)
				{
					XM_SetFileAttributes(file_name, XM_FILE_ATTRIBUTE_ARCHIVE);
				}
				strcpy (lpVideoFileToRemove->file_name[lpVideoFileToRemove->count], file_name);
				// ��ʱ
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

		// ����Ƿ��ǿ�ѭ��ʹ����Ƶ��
		if(video_item->video_lock)
		{
			// ������
			video_item_locked_space -= video_item->video_size;
			//XM_printf ("unlocked file size(%lld),  locked_space(%lld)\n",  video_item->video_size, video_item_locked_space);
		}
		else //if(!video_item->video_lock)
		{
			// ��ȥ�ɻ���(ѭ��)��Ƶ�ļ��ֽڴ�С
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

	// �����Ƶ������Ƿ��ѿ���	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	memset (&VideoFileToRemove, 0, sizeof(VideoFileToRemove));
	// XM_VideoItemLock����ͬһ���߳��ڶ�ε��ã�ֻ��ƥ���Ӧ��XM_VideoItemUnlock����
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

// ָ��ͨ������Ƶ�ļ��Ѵ���/д�벢�رգ���������Ӧ����Ƶ������е���Ϣ
//
// �˺�������H264 Cache�ļ�ϵͳ�̹߳ر��ļ�ʱ���ã���������H264�����̵߳�������
//
// ����ڴ˴��ɼ������ʣ��ռ��顢��ѭ��¼���ļ��ͷš���¼ʱ�䱨����� �Ȳ���
//
// *** ��¼�Ƶ���Ƶ�ļ���¼�ƹ����п��ܱ�����(һ����������)��
//		 ��������������Ҫ����ļ������Ƿ�������Ϊֻ�����ԡ�
//	pNewVideoItemHandle	��ʾ�Ƿ��¾�������´���
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

	// �����Ƶ������Ƿ��ѿ���	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	XM_VideoItemLock ();

	ret = 0;

	do
	{
		if(h == NULL)
			break;

		// �����ļ���Ӧ����Ƶ����(�Ѵ���)

		i = (RbtIterator)hVideoItemHandle;
		if(i == NULL)
			break;

		rbtKeyValue(h, i, &keyp, &valuep);
		video_item = (XMVIDEOITEM *)valuep;


		// �������Ӧ��ͨ����Ƶ�ļ���������Ϣ
		memset (file_name, 0, sizeof(file_name));
		if(XM_VideoItemGetVideoFilePath (video_item, nVideoChannel, file_name, VIDEOITEM_MAX_FILE_NAME))
		{
			// ����ͨ���Ƿ�������
			char FileName[VIDEOITEM_MAX_FILE_NAME];
			char NewFileName[VIDEOITEM_MAX_FILE_NAME];

			// ��ȡ�ļ����ļ��ֽڴ�С���ļ�������Ϣ
			// ���ͨ����Ӧ���ļ��Ƿ񴴽�
			if(video_item->video_channel == (nVideoChannel))
			{
				video_item->video_update |= (1 << nVideoChannel);
				
				// ������Ƶ���ļ��Ĵ���ʱ��
				video_item->video_create_time = *create_time;
				
				size = cbVideoFileSize;
	
				// �ۼ��ļ���С
				// ���㰴�������ļ���С
				cluster_size = get_fat_cluster_size_from_volume (video_item->volume_index);
				if(cluster_size)
					size = ((size + cluster_size - 1) / cluster_size) * cluster_size;
				
				video_item->file_size = cbVideoFileSize;
				// 20141227 ZhuoYongHong
				// �޸�bug��δ������Ƶ����ļ���С
				video_item->video_size += size;

				// һ���������ܻᵼ���´�������Ƶ�����
				// �����Ƶ���Ƿ���������
#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2
				// ����������Ƿ�����޸�. �����ڣ��޸�Ŀ¼
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
					// �ۼ���������Ƶ�ļ��ֽڴ�С
					video_item_locked_space += size;
					//XM_printf ("locked file (%s) , size(%d),  locked_space(%lld)\n",  file_name, size, video_item_locked_space);
					videoitem_inc_space (video_item, XM_VIDEOITEM_CLASS_MANUAL);
				}
				else
				{
					// �ۼӿɻ���(ѭ��)��Ƶ�ļ��ֽڴ�С
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
						// �ļ������쳣
						//XM_printf ("Get File Attr(%s) NG\n", file_name);
						break;				
					}
					// ������������ļ������Ƿ�������Ϊֻ������
					if( !(attr & D_RDONLY) )
					{
						//XM_printf ("XM_SetFileAttributes %s\n", file_name);
						XM_SetFileAttributes (file_name, XM_FILE_ATTRIBUTE_ARCHIVE | XM_FILE_ATTRIBUTE_READONLY);
					}
					
					// �ۼ���������Ƶ�ļ��ֽڴ�С
					video_item_locked_space += size;
					//XM_printf ("locked file (%s) , size(%d),  locked_space(%lld)\n",  file_name, size, video_item_locked_space);
					videoitem_inc_space (video_item, XM_VIDEOITEM_CLASS_MANUAL);
				}
				else if( (attr != (DWORD)(-1)) && (attr & D_RDONLY) )
				{
					// ֻ���ļ� (��ʱ��Ƶ�����Ϊֻ������)
					video_item->video_lock = 1;
					// �ۼ���������Ƶ�ļ��ֽڴ�С
					video_item_locked_space += size;		
					//XM_printf ("locked file (%s) , size(%d),  locked_space(%lld)\n",  file_name, size, video_item_locked_space);
					videoitem_inc_space (video_item, XM_VIDEOITEM_CLASS_MANUAL);
				}
				else
				{
					// δ����
					/*
					size = XM_GetFileSize (file_name);
					if(size == (DWORD)(-1))
					{
						// �ļ������쳣
						XM_printf ("Get File Size(%s) NG\n", file_name);
						break;				
					}
					XM_printf ("size=%d, cbVideoFileSize=%d\n", size, cbVideoFileSize);*/
					//size = cbVideoFileSize;
	
					// �ۼ��ļ���С
					// ���㰴�������ļ���С
					//cluster_size = get_fat_cluster_size_from_channel(nVideoChannel);
					//size = ((size + cluster_size - 1) / cluster_size) * cluster_size;
					//video_item->video_size += size;
							
					// �ۼӿɻ���(ѭ��)��Ƶ�ļ��ֽڴ�С
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

	// ��Ҫ�ڴ˴����� XM_VideoItemMonitorAndRecycleGarbage, �������ļ��رյȲ���
	// �������ʣ��ռ��顢��ѭ��¼���ļ��ͷš���¼ʱ�䱨�����
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

	// ����Ƿ�����ʱ��Ƶ���ļ�
	if(!XM_VideoItemIsTempVideoItemFile (lpVideoFileName))
	{
		//XM_printf ("XM_VideoItemUpdateVideoItemFromTempName NG, illegal temp file (%s)\n", lpVideoFileName);
		return 0;
	}
	
	// �����Ƶ������Ƿ��ѿ���	
	if(!XM_VideoItemIsServiceOpened())
	{
		XM_printf ("UpdateVideoItemFromTempName NG, Service closed\n");
		return 0;
	}
	
	// ��ȡRTCʱ��
	XM_GetLocalTime(&CurrTime);
		
	// ����ʱ��Ƶ���ļ�����������Ƶͨ����
	nVideoChannel = -1;
	if(!XM_VideoItemGetVideoChannelFromTempVideoItemName (lpVideoFileName, &nVideoChannel))
	{
		return 0;
	}
	if(nVideoChannel == (int)(-1))
		return 0;
		
	// ����һ���µ���Ƶ��
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
		
	// ��ȡ��Ƶ���ȫ·����
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
		
	// ����Ƶ�ļ�������ʱ��Ƶ���ļ�������ΪItemName
	// ����Ƿ���ֻ���ļ�
	DWORD attr = XM_GetFileAttributes(lpVideoFileName);
	read_only = 0;
	if(attr == (DWORD)(-1))
	{
	}
	else
	{
		if(attr & D_RDONLY)
		{
			// ֻ���ļ�, ������
			read_only = 1;
			XM_SetFileAttributes (lpVideoFileName, XM_FILE_ATTRIBUTE_ARCHIVE);
		}
	}
	// XM_rename��ֻ���ļ�����ʱ��ʧ��
	if(XM_rename (lpVideoFileName, newName))
	{
		XM_VideoItemUnlock ();
		XM_printf ("XM_rename from (%s) to (%s) NG\n", lpVideoFileName, ItemName);
		return 0;		
	}
	if(read_only)
	{
		// �ָ�����
		XM_SetFileAttributes (ItemName, XM_FILE_ATTRIBUTE_ARCHIVE | XM_FILE_ATTRIBUTE_READONLY);
	}
	XM_VideoItemUnlock ();
	
	FS_CACHE_Clean("");
	
	// �����ɹ�
	ret =  XM_VideoItemUpdateVideoItemFromFileName (ItemName, cbVideoFileSize, create_time);	
	
	// ����Ƿ���ͣ�����ģʽ
	if(XMSYS_GSensorCheckParkingCollisionStartup())
	{
		// ����
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
	int to_delete = 0;		// ɾ��¼��ʱ��С��1�����Ƶ�ļ�
	
	//unsigned int ticket = XM_GetTickCount ();
	
	// �����Ƶ������Ƿ��ѿ���	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
		
	// XM_VideoItemLock����ͬһ���߳��ڶ�ε��ã�ֻ��ƥ���Ӧ��XM_VideoItemUnlock����
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
				// ��ʽ����� ʱ��
				create_time = &video_item->video_create_time;
				dwCreateTime  = create_time->bSecond & 0x3F;
				dwCreateTime |= (create_time->bMinute & 0x3F) << 6;
				dwCreateTime |= (create_time->bHour & 0x1F) << 12;
				dwCreateTime |= (create_time->bDay & 0x1F) << 17;
				dwCreateTime |= (create_time->bMonth & 0x0F) << 22;
				dwCreateTime |= ((create_time->wYear - 2014) & 0x1F) << 26;
#if VIDEO_ITEM_FOLDER_NAMING_PROFILE_2
#else
				// ����λ
				if(video_item->video_lock)
					dwCreateTime |= 0x80000000;
#endif
				
				// ��ȡ¼��ʱ�䳤��(��λ ��)
				dwVideoTime = AP_VideoItemGetVideoStreamSize (video_item, nVideoChannel);
				
				// ɾ��¼��ʱ��С��1�����Ƶ�ļ�
				if(dwVideoTime < 1)
				{
					// 20170319 ����Cache������ɾ��С��1�����Ƶ, ��RecycleTask�Զ�ɨ�貢ɾ��С��1�����Ƶ
					//		����Cache������ִ��ɾ����������Cacheд�뱻����
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
		// ��Ƶ����³ɹ�
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
				// 20170702 �����б�����ѹر�
				// �˲�����Ϊ�����ȡÿ����Ƶ�ļ�����ȡ����Ƶ¼��ʱ������¿�����Ч���½�
				XMSYS_MessageSocketFileListUpdate (ch, type, file_name, dwCreateTime, dwVideoTime, 1);
			}
		}
	}

	// �������ʣ��ռ��顢��ѭ��¼���ļ��ͷš���¼ʱ�䱨�����
	//XM_VideoItemMonitorAndRecycleGarbage (0);

	// �ӳ��ļ�ɾ���Ĳ���, �������Ӵ��̶�д�ĸ���.
	delay_cutoff_flag = 1;
	
	return ret;
}

// ������Ƶ��Ŀ����ļ��������, ���Ե���Ϊÿ����ÿ���ļ����ͼ�ÿ�ּ�¼���Ͷ����������
// ����ֵ
//		0		���óɹ�
//		-1		����ʧ��
int XM_VideoItemSetItemCountQuota (unsigned int volumn_index, 	// SD�������(��0��ʼ)
											  unsigned int file_type,		// �ļ�����(��Ƶ����Ƭ)
											  unsigned int stream_class, 	// ��Ƶ�����(�������ֶ������)
											  unsigned int quota				// ����Ŀ�����Ŀ�ļ�����, 0 ��ʾ������
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

// ������Ƶ��Ĵ��̿ռ�ռ�����, Ϊ"ָ�����ϵ�ָ���ļ����͵�ָ����¼����"����ռ����
// ����ֵ
//		0		���óɹ�
//		-1		����ʧ��
int XM_VideoItemSetDiskSpaceQuotaLevelStreamClass (unsigned int volumn_index, 	// SD�������(��0��ʼ)
																	unsigned int file_type, 		// �ļ�����(��Ƶ����Ƭ)
																	unsigned int stream_class, 	// ��Ƶ�����(�������ֶ������)
																	XMINT64 quota						//	����Ĵ��̿ռ�ռ���ֽ�, 0��ʾ������
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

// ������Ƶ��Ĵ��̿ռ�ռ�����, Ϊ"ָ�����ϵ�ָ���ļ�����"(��������¼�Ʒ���)����ռ����
// ����ֵ
//		0		���óɹ�
//		-1		����ʧ��
int XM_VideoItemSetDiskSpaceQuotaLevelFileType (unsigned int volumn_index, 	// SD�������(��0��ʼ)
																unsigned int file_type,			// �ļ�����(��Ƶ����Ƭ)
																XMINT64 quota						//	����Ĵ��̿ռ�ռ���ֽ�, 0��ʾ������
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

// ������Ƶ��Ĵ��̿ռ�ռ�����, Ϊ"ָ����"(���������ļ����ͣ�����¼�Ʒ���)����ռ����
// ����ֵ
//		0		���óɹ�
//		-1		����ʧ��
int XM_VideoItemSetDiskSpaceQuotaLevelDiskVolume (unsigned int volumn_index, 	// SD�������(��0��ʼ)
																  XMINT64 quota					//	����Ĵ��̿ռ�ռ���ֽ�, 0��ʾ������
																)
{
	if(volumn_index >= XM_VIDEOITEM_VOLUME_COUNT)
		return -1;
	XM_VideoItemLock ();
	disk_space_quota_level_volume[volumn_index] = quota;
	XM_VideoItemUnlock ();
	return 0;	
}


// �����Ƶ���ļ���������Ƿ񳬳����塣
// ����ֵ
//		1		��������
//		0		û�г���
static XMBOOL videoitem_check_item_count_quota (unsigned int volumn_index, unsigned int file_type, unsigned int stream_class)
{
	// ��0ֵ��ʾ�����������
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

// �����Ƶ����̿ռ�����Ƿ񳬳�����
// ����ֵ
//		1		��������
//		0		û�г���
static XMBOOL videoitem_check_disk_space_quota (unsigned int volumn_index, unsigned int file_type, unsigned int stream_class)
{
	// ��0ֵ��ʾ�����������
	if(disk_space_quota_level_class[volumn_index][file_type][stream_class])
	{
		if(video_item_space[volumn_index][file_type][stream_class] >= disk_space_quota_level_class[volumn_index][file_type][stream_class])
			return 1;
	}
	return 0;
}


// ������Ƶ����Դ��ɾ���ļ���Դ
// ����ֵ
//		0��1	��ɾ������Ƶ�����
static int videoitem_recycle (unsigned int volumn_index, unsigned int file_type, unsigned int stream_class_bitmask)
{
	HANDLE handle;

	VIDEOFILETOREMOVE VideoFileToRemove;
	int result;

	int deleted_file_count = 0; 		// ��¼��ɾ�����ļ�����
	
	deleted_file_count = 0;
	
	// �����Ƶ���������Ƿ��Ѵﵽ�Զ��ͷſ�ѭ�������ֵ
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
			// ɾ����Ƶ�����ݿ�
			result = XM_VideoItemDeleteVideoItemHandle (handle, &VideoFileToRemove);
		}
		XM_VideoItemUnlock ();
		
		if(result)
		{
			// ����Ƶ���Ӧ�Ĵ����ļ�ɾ��
			remove_video_files (&VideoFileToRemove, 1);
			deleted_file_count ++;		// �ۼ���ɾ���ļ��ĸ���
		}
		
		if(handle == NULL)
		{
			return 0;
		}
	} while (0);	
	return deleted_file_count;
}

// ��ɻ��տռ���
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
		
	// ������ʣ��ռ��Ƿ�������ͼ�¼Ҫ��
	FreeSpace = SectorsPerCluster;
	FreeSpace = FreeSpace * BytesPerSector;
	FreeSpace = FreeSpace * NumberOfFreeClusters;
	
	RequiredSpace = FreeSpace + video_item_space[volumn_index][XM_FILE_TYPE_VIDEO][XM_VIDEOITEM_CLASS_NORMAL];
	
	// ���ɻ��տռ��Ƿ�С�ڶ���ֵ
	if( recycle_space_limit && (RequiredSpace < recycle_space_limit) )
		return 1;
	
	return 0;
}

// �����ռ�(�ֶ�������)�����
static int videoitem_quota_check_protect_space (unsigned int volumn_index, unsigned int file_type)
{
	XMINT64 total_quota;
	XMINT64 total_space;
	if(volumn_index >= video_volume_count)
		return -1;
	if(file_type >= XM_FILE_TYPE_COUNT)
		return -1;
	// �����ͨ¼�Ʒ���������ļ����͵������Ƿ񶼴���
	if(disk_space_quota_level_type[volumn_index][file_type] && disk_space_quota_level_class[volumn_index][file_type][XM_VIDEOITEM_CLASS_NORMAL])
	{
		total_quota = disk_space_quota_level_type[volumn_index][file_type] - disk_space_quota_level_class[volumn_index][file_type][XM_VIDEOITEM_CLASS_NORMAL];
		assert( total_quota > 0 );
		total_space = video_item_space[volumn_index][file_type][XM_VIDEOITEM_CLASS_MANUAL] + video_item_space[volumn_index][file_type][XM_VIDEOITEM_CLASS_URGENT];
		if(total_space > total_quota)
		{
			// ��������
			return 1;
		}
	}
	return 0;
}

// ����Ƿ���ڻ����ļ�����(��Ƶ����Ƭ)��������ƣ������ڣ�����Ƿ񳬳����ֵ
static int videoitem_quota_check_level_type (unsigned int volumn_index, unsigned int file_type)
{
	XMINT64 total_space;
	XMINT64 quota;
	unsigned int class_index;
	if(volumn_index >= video_volume_count)
		return -1;
	if(file_type >= XM_FILE_TYPE_COUNT)
		return -1;
	// �����ڸ��ļ����͵���������Ƿ����
	quota = disk_space_quota_level_type[volumn_index][file_type];
	if(quota)
	{
		// ��0ֵ��ʾ��������
		// �ۼӸ��ļ����͵�����¼�Ʒ��������
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

// ÿ10����ɨ��һ�ξ�
// ���ÿ������ʣ��ռ����ĳ����ֵʱ�ͷſ�ѭ����¼��
// ÿ���������320MB¼��ռ䡣ÿ10����ɨ��һ�ξ�(ִ��XM_VideoItemMonitorAndRecycleGarbage����)  
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
	
	int deleted_file_count = 0; 		// ��¼��ɾ�����ļ�����

	// �����Ƶ������Ƿ��ѿ���	
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
	
	// ����Ƿ���Ҫִ�л���վ�������
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
	
	// ********* 1) ��¼�ǳ���ռ������ ******************
	// ********* 1.1) �ɻ��տռ������
	for (volumn_index = 0; volumn_index < video_volume_count; volumn_index ++)
	{
		// ����ɨ���Ƿ������
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		// �������	
		quota_check_result = videoitem_recycle_space_check (volumn_index);
		if(quota_check_result == 1)
		{
			// ��������(��ѭ��¼��ռ�����趨ֵ)
			XM_printf ("VIDEOITEM_LOW_SPACE, recycle video item\n");
			XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_VIDEOITEM_LOW_SPACE);
			
			// 20180825 zhuoyonghong
			// �ɻ����ܿռ��ʱ���Զ���������ղ���
			int del_count = videoitem_recycle (volumn_index, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL); 
			printf("-------------------> del_count ---------------->: %d\n",del_count);
		}
	}
	
	// ********* 1.2) ����¼��ռ������ (�����ֶ�������)
	for (volumn_index = 0; volumn_index < video_volume_count; volumn_index ++)
	{
		// ����ɨ���Ƿ������
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		quota_check_result = videoitem_quota_check_protect_space (volumn_index, XM_FILE_TYPE_VIDEO);
		if(quota_check_result == 1)
		{
			// ����¼��������
			XM_printf ("VIDEOITEM_PROTECT_VIDEO_FULL, recycle protect item\n");
			XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_VIDEOITEM_PROTECT_VIDEO_FULL);
			
			// 20180915 zhuoyonghong
			// ����¼����ʱ���Զ���������������Ƶ
			videoitem_recycle (volumn_index, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_MANUAL|XM_VIDEOITEM_CLASS_BITMASK_URGENT);
		}
	}
	
	// ********* 1.3) ��Ƭ�ռ������(��������¼�Ʒ���)
	for (volumn_index = 0; volumn_index < video_volume_count; volumn_index ++)
	{
		// ����ɨ���Ƿ������
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		// ��Ƭ�����	
		quota_check_result = videoitem_quota_check_level_type (volumn_index, XM_FILE_TYPE_PHOTO);
		if(quota_check_result == 1)
		{
			// ��Ƭ������
			XM_printf ("VIDEOITEM_PHOTO_FULL\n");
			XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_VIDEOITEM_PHOTO_FULL);
		}
	}
	
	// ********* 2) �ɻ�����Ŀ��� ******************
	// ********* 2.1) ��Ƶ����Ŀ��������� **************
	//
	// 2.1.1) ��ͨ��Ƶ�ļ�
	// ÿ��ɾ�����ļ�����Ҫ̫�࣬��������
	deleted_file_count = 0;
	for (volumn_index = 0; volumn_index < video_volume_count; volumn_index ++)
	{
		// ����ɨ���Ƿ������
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		
		// �������ͨ��Ƶ�����Ƿ��ѳ�������������ִ�л��ղ���
		while(videoitem_check_item_count_quota (volumn_index, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_NORMAL))
		{
			// ��Ƶɾ���Ứ�ѽϳ�ʱ�䡣 ���Cacheϵͳ�Ƿ�æ����æ���Ƴٴ���
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
	
	// 2.1.2) ��ͨ��Ƭ�ļ�
	for (volumn_index = 0; volumn_index < video_volume_count; volumn_index ++)
	{
		// ����ɨ���Ƿ������
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		
		deleted_file_count = 0;
		// �������ͨ��Ƭ�����Ƿ��ѳ�������������ִ�л��ղ���
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
	// ********* 2.2) ��Ƶ����̿ռ������ **************
	//
	// 2.1) ��ͨ��Ƶ�ļ�
	for (volumn_index = 0; volumn_index < video_volume_count; volumn_index ++)
	{
		// ����ɨ���Ƿ������
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		
		deleted_file_count = 0;
		// �������ͨ��Ƶռ�ÿռ��Ƿ��ѳ�������������ִ�л��ղ���
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
	
	// 2.2) ��ͨ��Ƭ�ļ�
	for (volumn_index = 0; volumn_index < video_volume_count; volumn_index ++)
	{
		// ����ɨ���Ƿ������
		if(fat_cluster_size[volumn_index] == 0)
			continue;
		
		deleted_file_count = 0;
		// �������ͨ��Ƭռ�ÿռ��Ƿ��ѳ�������������ִ�л��ղ���
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
	// ********* 3) �����ռ�(���Ҫ����̿ռ�)��� **************
	//
	ret = 1;

	// ÿ���������360MB¼��ռ䡣  
	RequiredSpace = 360*1024*1024;
	//RequiredSpace = 396*1024*1024;
	//RequiredSpace = 1024*1024;
	//RequiredSpace *= 7000;
	MinimumRequiredSpace = 256 * 1024 * 1024;		// ��;���̿ռ䣬���ڸ�ֵʱ������ִ���ļ�ɾ��������
	FreeSpace = 0;
	
	deleted_file_count = 0;
	
	// ÿ�μ����������
	disk_free_space_busy = 0;

	// ����ÿ����Ƶ��
	for (i = 0; i < video_volume_count; )
	{
		ret = XM_GetDiskFreeSpace ("\\", &SectorsPerCluster, &BytesPerSector,
				&NumberOfFreeClusters, &TotalNumberOfClusters , XM_VideoItemGetVolumeName(i) );
		if(ret == 0)
		{
			// ������쳣
			if(videoitem_debug)
				XM_printf ("Fatal Error of volume(%s) File System\n", XM_VideoItemGetVolumeName(i));
			XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_FS_ERROR);
			// ������һ����
			i ++;
			continue;
		}
		

		// ������ʣ��ռ��Ƿ�������ͼ�¼Ҫ��
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
			// ������һ����
			i++;
			continue;
		}
		
		// ��Ǵ���ʣ��ռ��Ѳ�������ʵʱ��¼
		// 20180205 �������̿ռ�������Ҫ��ռ�ʱ����æ��־ disk_free_space_busy
		if(FreeSpace < MinimumRequiredSpace)
		{
			// 20180825 �޸�disk_free_space_busy���壬��Ϊ����ʣ��ռ䲻��̶�(0 ~ 9), 0 ��ʾ���̿ռ������¼�� 9 ��ʾ����ʣ��ռ����ز���
			// ��FreeSpace >= MinimumRequiredSpace��disk_free_space_busy = 0
			// ��FreeSpace < MinimumRequiredSpace/9��disk_free_space_busy = 9
			// disk_free_space_busy = 1;
			if(FreeSpace <= (MinimumRequiredSpace/9))
				disk_free_space_busy = 9;
			else
				disk_free_space_busy = MinimumRequiredSpace / FreeSpace;
			if(disk_free_space_busy >= 9)
				disk_free_space_busy = 9;
			XM_printf ("disk free space busy %d\n", disk_free_space_busy);
		}
		
		// ���ϵͳ�Ƿ�æ
		if(XMSYS_H264CodecIsBusy () && FreeSpace > MinimumRequiredSpace)
		{
			// �ļ�ɾ���������ļ�IO������
			// ���H264������æ(���ڵȴ�д��)����ʱ��Ҫ�����ļ�ɾ������������������д��,
			// �����ļ�IO�Ĳ�������(H264�������ļ�д�����ļ�ɾ��IO����)
			
			// ������һ����
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
		
		// �ͷ�һ����Ƶ����(һ����Ƶ�����λ�ڶ�����ϵ���Ƶ�ļ�)		
		memset (&VideoFileToRemove, 0, sizeof(VideoFileToRemove));
		result = 0;
		
		// ����������ɾ��������������
		XM_VideoItemLock ();
		// ����һ����ѭ���ļ�¼��(��Ƶ�ļ�)
		handle = XM_VideoItemGetNextCircularVideoItemHandle (i, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL);
		if(handle)
		{
			result = XM_VideoItemDeleteVideoItemHandle (handle, &VideoFileToRemove);
		}
		XM_VideoItemUnlock ();
		if(result)
		{
			// �ҵ�һ������ɾ������Ƶ�ɾ�����ļ�
			if(remove_video_files (&VideoFileToRemove, 1) == 0)
			{
				// ɾ�������ļ�ʧ��
				// ������һ����
				i ++;
				continue;
			}
			else
			{
				// ɾ�������ļ��ɹ�
				// 20180205 zhuoyonghong 
				// ������̿ռ�æ��־���ȴ���һ��ѭ�����¼�鲢����disk_free_space_busy
				// 20180825 ���¼��������ռ䲻��̶�
				// disk_free_space_busy = 0;
				if(XM_GetDiskFreeSpace ("\\", &SectorsPerCluster, &BytesPerSector,
					&NumberOfFreeClusters, &TotalNumberOfClusters , XM_VideoItemGetVolumeName(i) ))
				{
					// ������ʣ��ռ��Ƿ�������ͼ�¼Ҫ��
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
					// ��Ҫһ��ɾ�������ļ������·�Ӧ�ٶ�
					break;
				}
				// ����������ǰ��
			}		
		}
		else 
		{
			// û���ҵ�����ɾ������Ƶ��

			// �����Ƶ����������������Ƶ�����Ŀ���ڻ����2�����ʾ���е���Ƶ����ȫ���������޷��滻ɾ����
			if(video_item_count >= 2)
			{
				// ������Ƶ������������ѭ��ʹ����Դ�Ѻľ�
				if(!XM_VideoItemIsServiceOpened())
				{
					return;
				}

				// ϵͳ���ش�����Ҫ�ֹ�ɾ����Ƶ���߸�ʽ��SD��
				if(videoitem_debug)
					XM_printf ("Recycle Resource Consumed\n");
				
				safe_send_system_event (SYSTEM_EVENT_CARD_DISK_FULL);
				ret = 0;
				// ������һ����
				i ++;
				continue;
			}
			else		// ֻ��һ����Ƶ��
			{
				// ��һ����Ƶ¼���У����̿ռ䲻��
				if(!XM_VideoItemIsServiceOpened())
				{
					return;
				}

				if(videoitem_debug)
					XM_printf ("Disk Space low capacity\n");
				safe_send_system_event (SYSTEM_EVENT_CARD_DISK_FULL);
				ret = 0;
				// ������һ����
				i ++;
				continue;
			}
		}
		
		// ѭ�������¼�������
	}
		
	if(disk_free_space_busy == 0)
	{
		//XM_printf ("disk free space ready\n");
	}

}

static volatile int recycle_busy = 0;

// �����������Ƿ�æ
// ����ֵ
//		1		busy
//		0		free
int XM_VideoItemCheckRecycleMonitorBusy (void)
{
	return recycle_busy;
}

// ÿ10����ɨ��һ�ξ�
// ���ÿ������ʣ��ռ����ĳ����ֵʱ�ͷſ�ѭ����¼��
	// ÿ���������320MB¼��ռ䡣ÿ10����ɨ��һ�ξ�(ִ��XM_VideoItemMonitorAndRecycleGarbage����)  
void XM_VideoItemMonitorAndRecycleGarbage (XMBOOL bReportEstimatedRecordingTime)
{
	// ����һ���̷߳��ʡ��������߳����ڷ��ʣ����ء�
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

// ���ݶ�������Զ�ȡ��Ƶ�����ݿ�����Ƶ��ĸ���
// ����ֵ
// -1 ��ʾ����
// >= 0 ��ʾ���صķ��϶������Ƶ�����
//		1)	��ȡ��0�ϵ�ǰ����ͷ����ͨ¼����Ƶ�ļ�����
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_0, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL);
//		2) ��ȡ��0�ϵ�����ͨ���Ľ���¼��(G-Sensor)��Ƶ�ļ�����
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_0, 0xFFFFFFFF,  XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		3) ��ȡ��1�ϵ�����ͨ���ı�����Ƶ(�ֶ�¼�ƣ�����¼��)�ļ�����
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_1, 0xFFFFFFFF, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_MANUAL|XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		4) ��ȡ��0�ϵĺ�����ͷ��������Ƭ�ļ�����
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_1, XM_FILE_TYPE_PHOTO, 0xFFFFFFFF);
int XM_VideoItemGetVideoItemCountEx (unsigned int volume_index, 			// ���̾�0xFFFFFFFF��ʾ�������о�
												 unsigned int video_channel, 			// ͨ���ţ�0xFFFFFFFF��ʾ��������ͨ��
												 unsigned int file_type, 				// �ļ�����(��Ƶ����Ƭ)��0xFFFFFFFF��ʾ���������ļ�����
												 unsigned int stream_class_bitmask  // ¼�Ʒ�������,  0xFFFFFFFF��ʾ��������¼������
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL ��ʾ������ͨ���ֶ�����
												 )
{
	int ret = -1;
	int item_count = 0;
	XMVIDEOITEM *video_item;
	RbtIterator i;

	// �����Ƶ������Ƿ��ѿ���	
	if(!XM_VideoItemIsServiceOpened())
		return -1;
	
	// ����
	XM_VideoItemLock ();
	do
	{
		if(h == NULL || video_volume_count == 0)
		{
			break;
		}
		
		// ������Ƶ�����ݿ�
		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			void *keyp, *valuep;
			
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = (XMVIDEOITEM *)keyp;
			
			// ����ƥ��
			// ���̾�
			if(volume_index != 0xFFFFFFFF && volume_index != video_item->volume_index)
				continue;
			// ͨ����
			if(video_channel != 0xFFFFFFFF && video_channel != video_item->video_channel)
				continue;
			// �ļ�����
			if(file_type != 0xFFFFFFFF && file_type != video_item->file_type)
				continue;
			// ¼�Ʒ���
			if(stream_class_bitmask & (1 << video_item->stream_class))	
				item_count ++;
		}
		
		ret = item_count;
	} while (0);
	
	// ����
	XM_VideoItemUnlock ();
	
	return ret;	
}


// ���ݶ�������Զ�ȡ��Ƶ��ָ�����Ե��ļ��б�
// video_channel ��Ƶ��ͨ��
// index ��ȡ�ļ�¼��ʼ���
// count ��ȡ�ļ�¼����(������ʼ���)
// packet_addr ����������ַ
// packet_size ���������ֽڴ�С
// ����ֵ
// < 0 ʧ��
// >= 0 �������б����¼���ݵ��ֽڳ��� 
int XM_VideoItemGetVideoItemListEx (unsigned int video_channel, 		// ͨ���ţ� 0xFFFFFFFF��ʾ����ͨ��
											 unsigned int volume_index,			// ���̾��, 0xFFFFFFFF��ʾ���о�
											 unsigned int file_type,				// �ļ�����, 0xFFFFFFFF��ʾ�����ļ�����
											 unsigned int stream_class_bitmask,	// ¼��������룬0xFFFFFFFF��ʾ����¼��
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL ��ʾ������ͨ���ֶ�����
											 unsigned int index, 
											 unsigned int count,
											 void *packet_addr, 
											 int packet_size
											)
{
	// CH(ͨ����1�ֽ�), TYPE(���ͣ�1�ֽ�)��COUNT����¼������2�ֽڣ�,
	// 
	// RECORD(��¼���ݣ�40 * COUNT)
	// ÿ����¼Ϊ40�ֽ�
	// (32�ֽ��ļ��� + 4�ֽ�ʱ���ʾ + 4�ֽ�¼��ʱ�䳤��(��λ ��)��
	//	ʱ���ʾ��4�ֽڳ��� ��
	//��32λ������ʾ�����λPΪ����λ��
	//	PYYYYYMMMMDDDDDhhhhhmmmmmmssssss
	//	���λP��ʾ��Ƶ�Ƿ񱣻���1������
	//	(��Ƭ���λΪ0)	
	int ret = -1;
	unsigned int item_index = 0;
	//int item_count = 0;
	char *buff;
	int size;
	XMVIDEOITEM *video_item;
	RbtIterator i;
	void *keyp, *valuep;
	
	int file_count = 0;

	// �����Ƶ������Ƿ��ѿ���	
	if(!XM_VideoItemIsServiceOpened())
		return -1;
	
	// ����
	XM_VideoItemLock ();
	do
	{
		if(h == NULL || video_volume_count == 0)
		{
			break;
		}
		
		// ������Ƶ�����ݿ�
		// ��λ��ʼ���
		for (i = rbtBegin(h); i != rbtEnd(h); i = rbtNext(h, i))
		{
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = (XMVIDEOITEM *)keyp;
			// ����ƥ��
			// ͨ����
			if(video_channel != 0xFFFFFFFF && video_channel != video_item->video_channel)
				continue;
			// ���̾�
			if(volume_index != 0xFFFFFFFF && volume_index != video_item->volume_index)
				continue;
			// �ļ�����
			if(file_type != 0xFFFFFFFF && file_type != video_item->file_type)
				continue;
			// ¼�����
			if(!(stream_class_bitmask & (1 << video_item->stream_class)))
				continue;
			
			if(item_index == index)
			{
				// �ҵ�ƥ��Ŀ�ʼ�����
				break;
			}
				
			item_index ++;			
		}
		
		if(i == rbtEnd(h))
		{
			// û���ҵ�ƥ��Ŀ�ʼ��
			// 4 ��ʾͷ��Ϣ���ֽڳ��ȣ�CH(ͨ����1�ֽ�), TYPE(���ͣ�1�ֽ�)��COUNT����¼������2�ֽڣ�,
			ret = 4;
			break;
		}
		

		// �ӿ�ʼ�ʼ��������Ƶ�����ݿ⣬���ҵ�����Ƶ����Ŀ��ʽ������������
		buff = packet_addr;
		size = packet_size;
		buff += 4;		// ����ͷ��Ϣ
		size -= 4;
		
		// ÿ���������¼�ĳ���(32�ֽ��ļ��� + 4�ֽ�ʱ���ʾ + 4�ֽ�¼��ʱ�䳤��(��λ ��)��
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
				// ��ͬ��Ƶͨ��
				
				// ��ʽ����� �ļ���
				memcpy (buff, &video_item->video_name, VIDEOITEM_MAX_FULL_NAME);	
				// ��ʽ����� ʱ��
				create_time = &video_item->video_create_time;
				dwCreateTime  = create_time->bSecond & 0x3F;
				dwCreateTime |= (create_time->bMinute & 0x3F) << 6;
				dwCreateTime |= (create_time->bHour & 0x1F) << 12;
				dwCreateTime |= (create_time->bDay & 0x1F) << 17;
				dwCreateTime |= (create_time->bMonth & 0x0F) << 22;
				dwCreateTime |= ((create_time->wYear - 2014) & 0x1F) << 26;
				// ����λ
				if(video_item->video_lock)
					dwCreateTime |= 0x80000000;
				
				memcpy (buff + VIDEOITEM_MAX_FULL_NAME, &dwCreateTime, 4);
				
				// ��ȡ¼��ʱ�䳤��(��λ ��)
				dwVideoTime = AP_VideoItemGetVideoStreamSize (video_item, video_channel);
				
				memcpy (buff + VIDEOITEM_MAX_FULL_NAME + 4, &dwVideoTime, 4);
				
				buff += (VIDEOITEM_MAX_FULL_NAME+8);
				size -= (VIDEOITEM_MAX_FULL_NAME+8);		
				count --;
				
				file_count ++;
			}
			i = rbtNext(h, i);
			
			// ����Ƿ񵽴����ݿ��β��
			if(i == rbtEnd(h))
				break;
		} // while (count > 0 && size >= (VIDEOITEM_MAX_FULL_NAME + 8))	
		
		ret = packet_size - size;
	} while (0);
	
	
	// д��ͷ��Ϣ
	// CH(ͨ����1�ֽ�), TYPE(���ͣ�1�ֽ�)��COUNT����¼������2�ֽڣ�,
	((unsigned char *)packet_addr)[0] = (unsigned char)video_channel;
	((unsigned char *)packet_addr)[1] = 0;		// ��Ƶ����
	((unsigned char *)packet_addr)[2] = (unsigned char)(file_count);	// little endian
	((unsigned char *)packet_addr)[3] = (unsigned char)(file_count >> 8);
	
	// ����
	XM_VideoItemUnlock ();
	
	XM_printf ("ch=%d, video, file_count=%d\n", video_channel, file_count);
		
	return ret;	
}

// ��ȡָ��¼��ͨ�����ļ��б�
// video_channel ��Ƶ��ͨ��
// index ��ȡ�ļ�¼��ʼ���
// count ��ȡ�ļ�¼����(������ʼ���)
// packet_addr ����������ַ
// packet_size ���������ֽڴ�С
// ����ֵ
// < 0 ʧ��
// >= 0 �������б����¼���ݵ��ֽڳ��� 
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
										unsigned int volume_index,	// ���
										XMINT64 *TotalSpace, 		// SD���ռ�
										XMINT64 *LockedSpace, 		// ���������ļ��ռ�
										XMINT64 *RecycleSpace,		// ��ѭ��ʹ���ļ� + ���пռ�
										XMINT64 *OtherSpace			// �����ļ�(ϵͳ)ռ�ÿռ�
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
	
	// �����Ƶ������Ƿ��ѿ���	
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
		
		// �ļ�ϵͳ�ܿռ�
		temp64 = SectorsPerCluster * BytesPerSector;
		temp64 = temp64 * TotalNumberOfClusters;
		// �ļ�ϵͳ���пռ�
		free64 = SectorsPerCluster * BytesPerSector;
		free64 = free64 * NumberOfFreeClusters;
		*TotalSpace = temp64;
		// ����������Ƶ�ļ��ܿռ�
		*LockedSpace = video_item_locked_space;
		// ��ѭ��¼�Ƶ��ܿռ� = δ��������Ƶ�ļ��ܿռ� + �ļ�ϵͳ���пռ�
		*RecycleSpace = video_item_recycle_space + free64;
		// �����ļ�ռ�ÿռ�
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

// �˴�û�п��Ƕ��������
// ����ֵ����
//	0	�ѻ�ȡ����Ϣ
// -1	�޷���ȡ����Ϣ
int XM_VideoItemGetDiskUsage( 	XMINT64 *TotalSpace, 		// SD���ռ�
								XMINT64 *LockedSpace, 		// ���������ļ��ռ�
								XMINT64 *RecycleSpace,		// ��ѭ��ʹ���ļ� + ���пռ�
								XMINT64 *OtherSpace			// �����ļ�(ϵͳ)ռ�ÿռ�
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
	
	// �����Ƶ������Ƿ��ѿ���	
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
		
		// �ļ�ϵͳ�ܿռ�
		temp64 = SectorsPerCluster * BytesPerSector;
		temp64 = temp64 * TotalNumberOfClusters;
		// �ļ�ϵͳ���пռ�
		free64 = SectorsPerCluster * BytesPerSector;
		free64 = free64 * NumberOfFreeClusters;
		*TotalSpace = temp64;
		// ����������Ƶ�ļ��ܿռ�
		*LockedSpace = video_item_locked_space;
		// ��ѭ��¼�Ƶ��ܿռ� = δ��������Ƶ�ļ��ܿռ� + �ļ�ϵͳ���пռ�
		*RecycleSpace = video_item_recycle_space + free64;
		// �����ļ�ռ�ÿռ�
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

// ����Ƶ���ļ������ָ��ļ�������ʱ��
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

// ����Ƶ��ļ������ָ��ļ�������ʱ��
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

// ������Ƶ������(����ʱ�䣬¼��ͨ���ţ��ļ����ͣ���Ƶ������)�����ļ�����
// 20180411_115623_00000000_00P.JPG		�����ھ�0�ϵ�ͨ��0���ֶ���������Ƭ	
// 20180411_115623_00012345_10N.AVI		�����ھ�0�ϵ�ͨ��1�Ŀ�ѭ�����ǵ���ͨ������Ƶ�ļ�
// 20180411_115623_00011110_01U.AVI		�����ھ�1�ϵ�ͨ��0�Ľ������͵���Ƶ�ļ�����G-Sensor����
// 20180411_115623_10000000_11U.JPG		�����ھ�1�ϵ�ͨ��1�Ľ������͵���Ƭ�ļ�����G-Sensor����
// ����ֵ
//	 0      �����ļ����ɹ�
//  -1     ʧ��
int XM_MakeFileNameFromCurrentDateTimeEx(	XMSYSTEMTIME *lpCreateTime, 			// �ļ�����ʱ�䣬 Ϊ�ձ�ʾʹ�õ�ǰʱ��
											unsigned int VideoChannel,				// ¼��ͨ����
											unsigned int FileType, 					// �ļ�����(��Ƶ����Ƭ)
											unsigned int VideoItemClass,			// ��Ƶ�����(�������ֶ������)
											unsigned int CardVolumeIndex,			// SD�������(��0��ʼ)
											unsigned int SerialNumber,				// ���
											char* lpFileName, int cbFileName		// �����ļ������Ļ�������ַ���������ֽڴ�С
										)
{
	XMSYSTEMTIME localTime;
	char *ch;
	// ��������������\0�ַ�
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

// ����·���� "mmc:0:\\VIDEO_F"
int XM_VideoItemMakePathName(unsigned int CardVolumeIndex,			// SD�������(��0��ʼ)
							unsigned int VideoChannel,				// ͨ����(��0��ʼ)
							char *lpFullPathName, int cbFullPathName// ������Ƶ���ļ���ȫ·���ļ������漰ȫ·���ļ��������ֽڳ���
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

int XM_VideoItemMakePathNameEx(unsigned int CardVolumeIndex,			// SD�������(��0��ʼ)
								unsigned int VideoChannel,				// ͨ����(��0��ʼ)
								unsigned int FileType,					// �ļ�����(VIDEO/PHOTO)
								char *lpFullPathName, int cbFullPathName// ������Ƶ���ļ���ȫ·���ļ������漰ȫ·���ļ��������ֽڳ���
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

// ����ȫ·��Ŀ¼��
// "mmc:0:\\VIDEO_F\\"
// "mmc:0:\\VIDEO_F\\NORMAL\\"
// "mmc:0:\\VIDEO_F\\URGENT\\"
// ����ֵ
//		< 0		ʧ��
int XM_VideoItemMakeFullPathName(unsigned int CardVolumeIndex,				// SD�������(��0��ʼ)
								unsigned int VideoChannel,					// ͨ����(��0��ʼ)
								unsigned int FileType,						// �ļ�����(VIDEO/PHOTO)
								unsigned int StreamClass,					// ¼������(��ͨ���ֶ�������)
								char *lpFullPathName, int cbFullPathName	// ������Ƶ���ļ���ȫ·���ļ������漰ȫ·���ļ��������ֽڳ���
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


// ����ȫ·���ļ���
// "mmc:0:\\VIDEO_F\\20180318_121056_00001201_00P.AVI"
int XM_VideoItemMakeFullPathFileName (const char *VideoItemName,				// ��Ƶ������
											 unsigned int CardVolumeIndex,			// SD�������(��0��ʼ)
											 unsigned int VideoChannel,				// ͨ����(��0��ʼ)
											 unsigned int FileType,						// ��Ƶ����Ƭ
											 unsigned int StreamClass,					// ¼������(��ͨ���ֶ�������)
											 char *lpFullPathName, int cbFullPathName	// ������Ƶ���ļ���ȫ·���ļ������漰ȫ·���ļ��������ֽڳ���
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

// ����Ƶ���ļ������ָ��ļ�������ʱ��
void XM_GetDateTimeFromFileName (char* filename, XMSYSTEMTIME *filetime)
{
	XM_GetDateTimeFromLongFileName (filename, filetime);
}




// �����Ƶ�����Ƿ��Ѹ���(�ѹرն�Ӧ����Ƶ�ļ�)
// ͨ���ļ������Ҷ�����ʹ�þ�������������Ϊ�ļ���ɾ������Ч
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
	
	// ���ȴ�3���ӵȴ���Ƶ���ļ��ر�
	timeout_ticket = XM_GetTickCount() + 3000;
	
	// ����Ƿ�����ʱ��Ƶ���ļ�
	if(XM_VideoItemIsTempVideoItemFile (lpVideoFile))
	{
		// �ȴ���ʱ��Ƶ�ļ��رղ�����
		while(XM_GetTickCount() < timeout_ticket)
		{
			void *fp;
			
			// ���ACC�Ƿ��µ�. �����µ�, ���ٵȴ�TEMP�ļ�����
			if(!xm_power_check_acc_safe_or_not())
			{
				XM_printf ("WaitingForUpdate Stop, ACC down\n");
				return 0;
			}
			
			if(XMSYS_file_system_block_write_access_lock() == 0)
			{
				// �����ʱ�ļ��Ƿ����
				fp = XM_fopen (lpVideoFile, "rb");
				if(fp)
				{
					XM_fclose (fp);
				}
				XMSYS_file_system_block_write_access_unlock ();
				if(fp == NULL)
				{
					// �ļ��Ѳ�����
					ret = 1;
					break;
				}
				XM_Sleep (1);
			}
			else
			{
				XMSYS_file_system_block_write_access_unlock ();
				// �ǰ�ȫ���ļ�ϵͳ
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
			// �ļ��ѱ�ɾ��
			XM_VideoItemUnlock ();
			break;
		}
		video_item = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
		if(video_item)
			update = video_item->video_update & (1 << video_channel);
		XM_VideoItemUnlock ();
		
		if(video_item == NULL)	// ��Ƶ�����ݿ��ѹر�
			break;
		
		if(update)
		{
			ret = 1;
			break;
		}
		
		// �����Ƶ���ļ��Ƿ��Ѵ���. 
		// ��Ƶ���ļ�ʹ��cache�ļ�����, �ļ�����������ʧ��. ���ʹ�������ļ������ȷ���ļ��Ƿ����.
		// �������ļ�������, �˳�
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
		// �ȴ���Ƶ���ļ��ر�
		XM_Sleep (1);
	} while (video_item);
	
	//XM_printf ("XM_VideoItemWaitingForUpdate end\n");
	block_file_delete = 0;
	return ret;
}

// ��ָ���ļ�������Ƶ���ʹ���У���ֹɾ����������Ƶ��ɾ��
XMBOOL XM_VideoItemMarkVideoFileUsage (const char *lpVideoFileName)
{
	HANDLE hVideoHandle;
	XMBOOL ret = 0;
	XMVIDEOITEM *video_item = NULL;
	void *keyp = NULL, *valuep = NULL;
	int nVideoChannel;
	
	// �����Ƶ������Ƿ��ѿ���	
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

// ��ָ���ļ�������Ƶ���δʹ��
XMBOOL XM_VideoItemMarkVideoFileUnuse (const char *lpVideoFileName)
{
	HANDLE hVideoHandle;
	XMBOOL ret = 0;
	XMVIDEOITEM *video_item = NULL;
	void *keyp = NULL, *valuep = NULL;
	int nVideoChannel;
	
	// �����Ƶ������Ƿ��ѿ���	
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
	// ÿ����¼Ϊ40�ֽ�
	// (32�ֽ��ļ��� + 4�ֽ�ʱ���ʾ + 4�ֽ�¼��ʱ�䳤��(��λ ��)��	
	//	ʱ���ʾ��4�ֽڳ��� ��
	//��32λ������ʾ�����λPΪ����λ��
	//	PYYYYYMMMMDDDDDhhhhhmmmmmmssssss
	//	���λP��ʾ��Ƶ�Ƿ񱣻���1������
	//	(��Ƭ���λΪ0)		
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

// ��ȡָ����Ƭͨ�����ļ��б�, ������Ƶ�б��ʽ��֯
// image_channel ��Ƭ��ͨ��
// index ��ȡ�ļ�¼��ʼ���
// count ��ȡ�ļ�¼����(������ʼ���)
// packet_addr ����������ַ
// packet_size ���������ֽڴ�С
// ����ֵ
// < 0 ʧ��
// = 0 �������ļ���Ϊ0
// > 0 ��������Ч�ļ���
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

// ��ָ������Ƶ�ļ�(ȫ·����)ɾ��������վ
// 1 �ɹ�
// 0 ʧ��
XMBOOL XM_VideoItemDeleteVideoFileIntoRecycleBin (const char *lpVideoFileName)
{
	HANDLE hVideoHandle = NULL;
	XMBOOL ret = 0;
	int nVideoChannel;
	VIDEOFILETOREMOVE VideoFileToRemove;
	XMVIDEOITEM *video_item = NULL;
	char video_name[VIDEOITEM_MAX_NAME + 1];
	
	// �����Ƶ������Ƿ��ѿ���	
	if(!XM_VideoItemIsServiceOpened())
		return 0;
	
	memset (video_name, 0, sizeof(video_name));
	
	// XM_VideoItemLock����ͬһ���߳��ڶ�ε��ã�ֻ��ƥ���Ӧ��XM_VideoItemUnlock����
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
		// ����ɾ�����ļ���������վ
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

// ������ʣ��ռ��Ƿ��޷�����ʵʱ¼������
// ����ֵ
// 1   �޷�����ʵʱ¼������
// 0   ��������ʵʱ¼������
// 20180825 �޸�disk_free_space_busy���壬��Ϊ����ʣ��ռ䲻��̶�(0 ~ 9), 0 ��ʾ���̿ռ������¼�� 9 ��ʾ����ʣ��ռ����ز���
XMBOOL XM_VideoItemCheckDiskFreeSpaceBusy (void)
{
	return disk_free_space_busy;
}

// 20170929
// ��ʱ���ڵ��ļ�ɾ������
void XM_VideoItemSetDelayCutOff (void)
{
	delay_cutoff_flag = 1;
}

int XM_VideoItemGetDelayCutOff (void)
{
	return delay_cutoff_flag;
}

// ���ô��̱�����ֵ
// 	MinimumSpaceLimit(MB)		��������������ƶ���
//	RecycleSpaceLimit(MB)		�ɻ��տռ��������, С�ڸ�ֵ������
int XM_VideoItemSetDiskSpaceLimit (unsigned int MinimumSpaceLimit, unsigned int RecycleSpaceLimit)
{
	minimum_space_limit = MinimumSpaceLimit;
	minimum_space_limit *= 1024 * 1024;
	recycle_space_limit = RecycleSpaceLimit;
	recycle_space_limit *= 1024 * 1024;
	return 0;
}

// ʹ��Ԥ�����������(�ڴ��̾���ú����ã� �������ÿ���ʧ��)
// ����ֵ
//		0		���óɹ�
//		< 0	����ʧ��
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
		// ����ÿ����Ŀɻ��տռ��Ϊ1500MB�������ڸ���ֵʱ��ϵͳ��������ʾ����������
		XM_VideoItemSetDiskSpaceLimit (500, 1500); 
		// ������;���������Ŀɻ��տռ��Ϊ8000MB�������ڸ���ֵʱ��ϵͳ��������ʾ����������
		//XM_VideoItemSetDiskSpaceLimit (500, 8000); 
		
		for (volume_index = 0; volume_index < video_volume_count; volume_index ++)
		{
			XMINT64 total64;
			volume_name = (char *)XM_VideoItemGetVolumeName (volume_index);
			if(XM_GetDiskFreeSpace ("\\", &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters, volume_name) == 0)
				continue;
			
			// �ļ�ϵͳ�ܿռ�
			total64 = SectorsPerCluster * BytesPerSector;
			total64 = total64 * TotalNumberOfClusters;
			
			// ����ָ�������Ƭ�ռ����(��Ƭ���200M, ���þ��"����¼�����͵�"��Ƭ�ܿռ�ʹ���200MBʱ��ϵͳ��ʾ��Ƭ�ռ���)
			XM_VideoItemSetDiskSpaceQuotaLevelFileType (volume_index, XM_FILE_TYPE_PHOTO, 200 * 1024 * 1024);
			// ������;������ָ�������Ƭ�ռ����(��Ƭ���3M, ���þ��"����¼�����͵�"��Ƭ�ܿռ�ʹ���3MBʱ��ϵͳ��ʾ��Ƭ�ռ���)
			//XM_VideoItemSetDiskSpaceQuotaLevelFileType (volume_index, XM_FILE_TYPE_PHOTO, 3 * 1024 * 1024);
			
			// ������;������"��ͨ¼����Ƭ"�ռ����Ϊ2MB(�����ڸ�2MB�����Զ�������ͨ¼����Ƭ���ջ��ƽ����ϵ���Ƭɾ��)
			//XM_VideoItemSetDiskSpaceQuotaLevelStreamClass (volume_index, XM_FILE_TYPE_PHOTO, XM_VIDEOITEM_CLASS_NORMAL, 2 * 1024 * 1024);		// ����
			
			// �����ܵ���Ƶ�ռ����(������ - ��Ƭ���200M - 500M(�������������))
			total64 -= 200 * 1024 * 1024;
			total64 -= 500 * 1024 * 1024;
			//total64 = 1024 * 1024 * 1024;
			//total64 *= 6;
			XM_VideoItemSetDiskSpaceQuotaLevelFileType (volume_index, XM_FILE_TYPE_VIDEO, total64);
			
			// һ��¼�����(������ - ��Ƭ���200M - 500M(�������������))x70%
			// ����¼�����(������ - ��Ƭ���200M - 500M(�������������))x30%
			// ����һ��¼�����(������ - ��Ƭ���200M - 500M(�������������))x70% (���ڸ������Զ�������ͨ¼����Ƶ���ջ���)
			XM_VideoItemSetDiskSpaceQuotaLevelStreamClass (volume_index, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_NORMAL, total64 * 7 / 10);
		}
	}
	XM_VideoItemUnlock ();
	
	return 0;
}

// ���ݶ��������ɾ�������������Զ������Ƶ��
// ����ֵ
//		< 0		�쳣
//		0			���������˳�
//		1			�û���ֹ�˳�
int XM_VideoItemDeleteVideoItemList (
											 unsigned int volume_index,			// ���̾��, 0xFFFFFFFF��ʾ���о�
											 unsigned int video_channel, 			// ͨ���ţ� 0xFFFFFFFF��ʾ����ͨ��
											 unsigned int file_type,				// �ļ�����, 0xFFFFFFFF��ʾ�����ļ�����
											 unsigned int stream_class_bitmask,	// ¼��������룬0xFFFFFFFF��ʾ����¼��
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL ��ʾ������ͨ���ֶ�����
											 int (*fp_user_callback) (void *user_data, char *full_path_name_to_delete),
											 												// �û��ص������� �����û�ȡ��ĳ����ʱ�������Ϊ0��ʾ�޻ص�����
																							//		����ֵΪ1��ʾ��ֹ��ǰ�Ĳ������˳�������ֵΪ0��ʾ������ǰ�Ĳ���
																							//		user_data ������û�˽������
																							//		full_path_name_to_delete ׼��ɾ������Ƶ��ȫ·���ļ���
											 void *user_data							//	�û�˽������
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

	// �����Ƶ������Ƿ��ѿ���	
	if(!XM_VideoItemIsServiceOpened())
		return -1;	

	memset (file_name, 0, sizeof(file_name));
	ret = -1;
	// ����
	XM_VideoItemLock ();
		
	do
	{
		if(h == NULL || video_volume_count == 0)
		{
			break;
		}
		
		ret = 0;
		// ������Ƶ�����ݿ�
		// ��λ��ʼ���
		for (i = rbtBegin(h); i != rbtEnd(h); )
		{
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = (XMVIDEOITEM *)keyp;
			
			//XM_printf("video_item->video_name =%s,video_item->volume_index=%d,video_item->video_channel=%d\n",(char *)video_item->video_name,video_item->volume_index,video_item->video_channel);
			
			// ����ƥ��
			// ͨ����
			if(video_channel != 0xFFFFFFFF && video_channel != video_item->video_channel)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// ���̾�
			if(volume_index != 0xFFFFFFFF && volume_index != video_item->volume_index)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// �ļ�����
			if(file_type != 0xFFFFFFFF && file_type != video_item->file_type)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// ¼�����
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
				if( (*fp_user_callback)(user_data, file_name) == 1 )	// ��ֹ��ǰ��ɾ������������
				{
					ret = 1;
					break;
				}
			}
			// ��ȡ��һ���ڵ㣬���⵱ǰ�ڵ�ɾ������һ���ڵ���Ϣ�쳣��
			i = rbtNext(h, i);
			XM_VideoItemDeleteVideoItemHandleFromFileNameEx (file_name, 0);
		}
	} while (0);
	
	// ����
	XM_VideoItemUnlock ();
	
	return ret;	
}

// ���������Զ�����ϵ���Ƶ��ȫ������
// ����ֵ
//		0		�ɹ�
//		-1		ʧ��
int XM_VideoItemLockVideoItem (
											 unsigned int volume_index,			// ���̾��, 0xFFFFFFFF��ʾ���о�
											 unsigned int video_channel, 			// ͨ���ţ� 0xFFFFFFFF��ʾ����ͨ��
											 unsigned int file_type,				// �ļ�����, 0xFFFFFFFF��ʾ�����ļ�����
											 unsigned int stream_class_bitmask,	// ¼��������룬0xFFFFFFFF��ʾ����¼��
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL ��ʾ������ͨ���ֶ�����
											 int (*fp_user_callback) (void *user_data, char *full_path_name_to_delete),
											 												// �û��ص������� �����û�ȡ��ĳ����ʱ�������Ϊ0��ʾ�޻ص�����
																							//		����ֵΪ1��ʾ��ֹ��ǰ�Ĳ������˳�������ֵΪ0��ʾ������ǰ�Ĳ���
																							//		user_data ������û�˽������
																							//		full_path_name_to_delete ׼��ɾ������Ƶ��ȫ·���ļ���
											 void *user_data							//	�û�˽������
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

	// �����Ƶ������Ƿ��ѿ���	
	if(!XM_VideoItemIsServiceOpened())
		return -1;	

	memset (file_name, 0, sizeof(file_name));
	ret = -1;
	// ����
	XM_VideoItemLock ();
		
	do
	{
		if(h == NULL || video_volume_count == 0)
		{
			break;
		}
		
		ret = 0;
		// ������Ƶ�����ݿ�
		// ��λ��ʼ���
		for (i = rbtBegin(h); i != rbtEnd(h); )
		{
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = (XMVIDEOITEM *)keyp;
			
			//XM_printf("video_item->video_name =%s,video_item->volume_index=%d,video_item->video_channel=%d\n",(char *)video_item->video_name,video_item->volume_index,video_item->video_channel);
			
			// ����ƥ��
			// ͨ����
			if(video_channel != 0xFFFFFFFF && video_channel != video_item->video_channel)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// ���̾�
			if(volume_index != 0xFFFFFFFF && volume_index != video_item->volume_index)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// �ļ�����
			if(file_type != 0xFFFFFFFF && file_type != video_item->file_type)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// ¼�����
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
				if( (*fp_user_callback)(user_data, file_name) == 1 )	// ��ֹ��ǰ����������������
				{
					ret = 1;
					break;
				}
			}
			// ��ȡ��һ���ڵ㣬���⵱ǰ�ڵ�ɾ������һ���ڵ���Ϣ�쳣��
			i = rbtNext(h, i);
		}
	} while (0);
	
	// ����
	XM_VideoItemUnlock ();
	
	return ret;		
}

// ���������Զ�����ϵ���Ƶ��ȫ������
// ����ֵ
//		0		�ɹ�
//		-1		ʧ��
int XM_VideoItemUnlockVideoItem (
											 unsigned int volume_index,			// ���̾��, 0xFFFFFFFF��ʾ���о�
											 unsigned int video_channel, 			// ͨ���ţ� 0xFFFFFFFF��ʾ����ͨ��
											 unsigned int file_type,				// �ļ�����, 0xFFFFFFFF��ʾ�����ļ�����
											 unsigned int stream_class_bitmask,	// ¼��������룬0xFFFFFFFF��ʾ����¼��
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL ��ʾ������ͨ���ֶ�����
											 int (*fp_user_callback) (void *user_data, char *full_path_name_to_delete),
											 												// �û��ص������� �����û�ȡ��ĳ����ʱ�������Ϊ0��ʾ�޻ص�����
																							//		����ֵΪ1��ʾ��ֹ��ǰ�Ĳ������˳�������ֵΪ0��ʾ������ǰ�Ĳ���
																							//		user_data ������û�˽������
																							//		full_path_name_to_delete ׼��ɾ������Ƶ��ȫ·���ļ���
											 void *user_data							//	�û�˽������
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

	// �����Ƶ������Ƿ��ѿ���	
	if(!XM_VideoItemIsServiceOpened())
		return -1;	

	memset (file_name, 0, sizeof(file_name));
	ret = -1;
	// ����
	XM_VideoItemLock ();
		
	do
	{
		if(h == NULL || video_volume_count == 0)
		{
			break;
		}
		
		ret = 0;
		// ������Ƶ�����ݿ�
		// ��λ��ʼ���
		for (i = rbtBegin(h); i != rbtEnd(h); )
		{
			rbtKeyValue(h, i, &keyp, &valuep);
			video_item = (XMVIDEOITEM *)keyp;
			
			//XM_printf("video_item->video_name =%s,video_item->volume_index=%d,video_item->video_channel=%d\n",(char *)video_item->video_name,video_item->volume_index,video_item->video_channel);
			
			// ����ƥ��
			// ͨ����
			if(video_channel != 0xFFFFFFFF && video_channel != video_item->video_channel)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// ���̾�
			if(volume_index != 0xFFFFFFFF && volume_index != video_item->volume_index)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// �ļ�����
			if(file_type != 0xFFFFFFFF && file_type != video_item->file_type)
			{
				i = rbtNext(h, i);
				continue;
			}
			
			// ¼�����
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
				if( (*fp_user_callback)(user_data, file_name) == 1 )	// ��ֹ��ǰ����������������
				{
					ret = 1;
					break;
				}
			}
			// ��ȡ��һ���ڵ㣬���⵱ǰ�ڵ�ɾ������һ���ڵ���Ϣ�쳣��
			i = rbtNext(h, i);
		}
	} while (0);
	
	// ����
	XM_VideoItemUnlock ();
	
	return ret;		
}