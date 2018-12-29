// 系统消息任务
// 1) 处理各种硬件事件（如卡插拔、电源、异常处理）
// 2) 将处理过的消息投递到UI层
#include "hardware.h"
#include <stdlib.h>
#include <stdio.h>
#include "xm_core.h"
#include <xm_type.h>
#include <xm_dev.h>
#include <fs.h>
#include <assert.h>
#include <xm_queue.h>
#include "fevent.h"
#include "sdmmc.h"
#include "xm_videoitem.h"
#include "xm_h264_cache_file.h"
#include "printk.h"
#include "xm_systemupdate.h"
#include "xm_message_socket.h"
#include "xm_key.h"
#include "xm_user.h"



#define	MAX_SYSTEM_MESSAGE_COUNT		16

typedef struct tagSYSTEM_MESSAGE {
	void *		prev;
	void *		next;
	fEvent		event;
} SYSTEM_MESSAGE;

static OS_TASK TCB_MessageTask;
__no_init static OS_STACKPTR int StackMessageTask[XMSYS_MESSAGE_TASK_STACK_SIZE/4];          /* Task stacks */

static SYSTEM_MESSAGE	message_unit[MAX_SYSTEM_MESSAGE_COUNT];
static queue_s				message_free;	// 空闲单元链表
static queue_s				message_live;	// 消息单元链表
static OS_CSEMA			message_sema;	// 已缓冲的系统消息计数

static OS_RSEMA			message_access_sema;	

int xm_open_volume_service (char *volume_name, int b_post_insert_message)
{
	int ret;
	// 打开视频项服务
	OS_Use (&message_access_sema);
	
	// 20180609
	// 使用预定义的视频项磁盘配额方案1
	XM_VideoItemSetQuotaProfile (XM_VIDEOITEM_QUOTA_PROFILE_1);
	ret = XM_VideoItemOpenService ();
	if(ret == 0)
	{
		// 视频项打开成功
		extern void XM_SetCardState (DWORD dwValue);
						
		// 标记卡已插入的状态
		XM_SetCardState (DEVCAP_SDCARDSTATE_INSERT);
						
		// 检查卡的可用循环录像空间是否满足最低视频录像需求
		if(XM_VideoItemCheckCardRecycleSpace() == 0)
		{
			// 总的可用循环空间无法满足最低录像需求
			XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_VIDEOITEM_LOW_SPACE);
		}
		// 检查卡的性能
		else if(XM_VideoItemCheckCardPerformance() == 0)		
		{
			// 低性能
			// 提示视频项写入的性能不是最佳，提示格式化
			XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_VIDEOITEM_LOW_PREFORMANCE);
		}
		else
		{
			// 卡已插入,满足录像的要求
			if(b_post_insert_message)
				XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_INSERT);
		}
		
		XM_printf ("OpenService success\n");
	}
	else
	{
		XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_FS_ERROR);
		XM_printf ("OpenService failure\n");
	}
	OS_Unuse (&message_access_sema);
	return ret;	
}

int xm_close_volume_service (char *volume_name)
{
	OS_Use (&message_access_sema);
	XM_printf ("XM_VideoItemCloseService start\n");
	XM_VideoItemCloseService ();
	// Invalidates all file/directory handles and unmounts the volume.
	//	FS_UnmountForced() should be called if a volume has been removed before 
	// it could be regular unmounted. It invalidates all file handles. 
	//	If you use FS_UnmountForced() there is no guarantee that all file handles to this volume
	// are closed and the directory entries for the files are updated.
   XM_printf ("FS_UnmountForced (%s)\n", volume_name);
	int safe_file_io = XMSYS_file_system_block_write_access_lock ();
	if(safe_file_io == 0)
	{
		// 20180414 zhuoyonghong
		// save FSInfo(FAT32)
		extern void FS_VolumnClean (const char * sVolume);
		FS_VolumnClean (volume_name);
		FS_CACHE_Clean(volume_name);
	}
	FS_UnmountForced (volume_name);
	//FS_Unmount (volume_name);
	XMSYS_H264CodecForcedEncoderStop ();	// 强制codec退出
	XMSYS_file_system_block_write_access_unlock ();
	OS_Unuse (&message_access_sema);
	return 0;
}

static int sdmmc_message_handler (fCard_Event *card_event)
{
	int ret;
	char temp[32];
	if(card_event->cardType == SD_DEVICE)
	{
		// SDMMC卡
		if(card_event->cardID >= SDMMC_DEV_COUNT)
		{
			XM_printf ("illegal card id(%d)\n", card_event->cardID);
			return -1;
		}
		if(card_event->plugIn > SDCARD_REINIT)
		{
			XM_printf ("illegal card event(%d)\n", card_event->plugIn);
			return -1;
		}
		if(card_event->cardID == 0)
			sprintf (temp, "mmc:%d:", 0/*card_event->cardID*/);
		else
			sprintf (temp, "mmc1:%d:", 0 /*card_event->cardID*/);
		if(card_event->plugIn == SDCARD_INSERT)	// insert
		{
			int loop = 2;
			//XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_DETECT);//取消SD卡准备中信息
			//OS_Delay (250);	// Power Up Time, 250ms max
			while (loop > 0)
			{
				int card_ret;
				ResetSDMMCCard (card_event->cardID);
				//OS_Delay (10);
				OS_Delay (250);	// Power Up Time, 250ms max
				enter_region_to_protect_7116_setup ();
				card_ret = InitSDMMCCard (card_event->cardID);
				leave_region_to_protect_7116_setup ();
				if(card_ret == 0)
				{
					// 自动Mount。SD卡一旦插入，无需手工干涉，自动进入记录模式
					ret = FS_Mount (temp);
					XM_printf ("FS_Mount(%s) ret=%d\n", temp, ret);
					//if(ret == 3)	// Volume is mounted read/write
					if(ret >= 0)	// Volume is mounted read/write
					{
						// 打开卷服务
						ret = xm_open_volume_service (temp, 1);
						// 启动后台检查程序, 检查系统升级包是否存在
						XMSYS_system_update_check ();
						XM_printf(">>>>>>>>>>>sd insert, system update check file..........\r\n");
						break;
					}
					else
					{
						//XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_INVALID);
					}
				}
				else
				{
					//XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_INVALID);
				}
				//ResetSDMMCCard (card_event->cardID);
				//OS_Delay (10);
				loop --;
			}
			
			if(loop == 0)
				XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_INVALID);
		}
		else if(card_event->plugIn == SDCARD_REMOVE)	// remove
		{
			XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_UNPLUG);
			
			StopSDMMCCard (card_event->cardID);

			// 关闭卷服务
			xm_close_volume_service (temp);
			// 当一个文件删除操作正在执行的时候, 卡拔出又迅速插入, 此时消息任务优先级高于CacheFile任务优先级,
			// 导致消息任务完成remove操作后(XM_VideoItemCloseService, FS_UnmountForced)(卡拔出), 又重新完成(FS_Mount, XM_VideoItemOpenService)操作(卡插入),
			// CacheFile任务无法检查出其中的变化, 继续使用老的文件句柄, 因为句柄无效而导致访问异常(产生系统异常).
			
			// 等待Cache系统关闭
			XMSYS_WaitingForCacheSystemFlush (1000);
			
			// 此时加入0.5秒的延时, 等待其他任务(CacheFile任务)的退出.
			OS_Delay (500);
			//XM_printf ("Card-Remove Delay end\n");
		}
		else if(card_event->plugIn == SDCARD_REINIT)	// reinit
		{
			// 卡busy的时候需要对卡复位, 重新初始化卡及文件系统
			int loop;
			XM_printf ("do SDCARD_REINIT\n");
			// 关闭卷服务
			xm_close_volume_service (temp);
			OS_Delay (500);
			
			// 标记卡REINIT过程中, 阻止REINIT消息递归投递
			SetSDMMCCardDoReInit (card_event->cardID, 1);
			
			loop = 3;
			while (loop > 0)
			{
				int card_ret;
				XM_printf ("REINIT %d\n", loop);
				
				ResetSDMMCCard (card_event->cardID);
				OS_Delay (250);
				enter_region_to_protect_7116_setup ();
				card_ret = InitSDMMCCard (card_event->cardID);
				leave_region_to_protect_7116_setup ();
				if(card_ret == 0)
				{
					// 自动Mount。SD卡一旦插入，无需手工干涉，自动进入记录模式
					ret = FS_Mount (temp);
					XM_printf ("FS_Mount(%s) ret=%d\n", temp, ret);
					//if(ret == 3)	// Volume is mounted read/write
					if(ret >= 0)	// Volume is mounted read/write
					{
						// 打开卷服务
						ret = xm_open_volume_service (temp, 1);
						break;
					}
					else
					{
						//XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_INVALID);
					}
				}
				else
				{
					//XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_INVALID);
				}
				loop --;
			}
			
			// 标记卡REINIT过程已完成
			SetSDMMCCardDoReInit (card_event->cardID, 0);
			
			if(loop == 0)
			{
				XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_INVALID);
				XM_SetFmlDeviceCap (DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_STOP);
			}
		}
					
		// 发送UI通知事件到UI任务
	}	
	
	return 0;
}

void XMSYS_MessageTask (void)
{
	//int ret;
	SYSTEM_MESSAGE *message;
	while (1)
	{
		OS_WaitCSema (&message_sema);
		
		// 从系统消息队列获取一个消息
		XM_lock ();	// 关中断
		message = (SYSTEM_MESSAGE *)queue_delete_next (&message_live);
		XM_unlock ();	// 恢复中断状态
		
		//printk ("system message (%d)\n", message->event.uFEvent.fKind);
		// 处理消息
		switch (message->event.uFEvent.fKind)
		{
			case CARD_EVENT:
				// 卡插拔系统消息
				sdmmc_message_handler (&message->event.uFEvent.card_event);
				break;
		}
		
		// 将消息单元插入到空闲单元链表
		XM_lock ();	// 关中断
		queue_insert ((queue_s *)message, &message_free);
		XM_unlock ();	// 恢复中断状态
	}
}

int addEventQueue(fEvent *pEvent)
{
	SYSTEM_MESSAGE *message;
	if(pEvent == NULL)
		return -1;
	
	XM_lock ();	// 关中断
	if(queue_empty (&message_free))
	{
		XM_unlock ();	// 恢复中断状态
		XM_printf ("message queue full, fKind=%d\n", pEvent->uFEvent.fKind);
		return -1;
	}

	message = (SYSTEM_MESSAGE *)queue_delete_next (&message_free);
	memcpy (&message->event, pEvent, sizeof(fEvent));
	queue_insert ((queue_s *)message, &message_live);
	XM_unlock ();	// 恢复中断状态
	
	OS_SignalCSema (&message_sema);
	
	return 0;
}

// 20170702
// 检查相同的消息是否已在系统消息队列中存在.
// 避免某些消息的不断重发(SDCARD_REINIT)导致系统响应迟钝
// 若存在, 返回1
// 不存在, 返回0
int chkEventQueue(fEvent *pEvent)
{
	SYSTEM_MESSAGE *message;
	int ret = 0;
	if(pEvent == NULL)
		return ret;
	
	XM_lock ();	// 关中断
	if(queue_empty (&message_live))
	{
		// 消息队列空
		XM_unlock ();	// 恢复中断状态
		return ret;
	}

	message = (SYSTEM_MESSAGE *)queue_next (&message_live);
	while( (queue_s *)message !=  &message_live )
	{
		if(memcmp (&message->event, pEvent, sizeof(fEvent)) == 0)
		{
			ret = 1;
			break;
		}
		message = (SYSTEM_MESSAGE *)queue_next ((queue_s *)message);
	}
	XM_unlock ();	// 恢复中断状态
	
	return ret;
}


void XMSYS_MessageInit (void)
{
	int i;
	queue_initialize (&message_free);
	queue_initialize (&message_live);
	memset (&message_unit, 0, sizeof(message_unit));
	for (i = 0; i < MAX_SYSTEM_MESSAGE_COUNT; i ++)
	{
		queue_insert ((queue_s *)&message_unit[i], &message_free);
	}
	
	OS_CreateCSema (&message_sema, 0);
	
	OS_CREATERSEMA (&message_access_sema); /* Creates resource semaphore */
	
	OS_CREATETASK(&TCB_MessageTask, "MessageTask", XMSYS_MessageTask, XMSYS_MESSAGE_TASK_PRIORITY, StackMessageTask);
}

void XMSYS_MessageExit (void)
{
	
}


// 发送"同步应答包"或者"异步消息包"
int  XMSYS_MessageSocketTransferResponsePacket (unsigned int socket_type,  
																unsigned int command_no,			// 命令包序号
												  				unsigned int command_id,			// 命令包ID
												  				unsigned char *command_packet_buffer,
												  				unsigned int command_packet_length
															)
{
	return 0;
}

