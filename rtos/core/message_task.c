// ϵͳ��Ϣ����
// 1) �������Ӳ���¼����翨��Ρ���Դ���쳣����
// 2) �����������ϢͶ�ݵ�UI��
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
static queue_s				message_free;	// ���е�Ԫ����
static queue_s				message_live;	// ��Ϣ��Ԫ����
static OS_CSEMA			message_sema;	// �ѻ����ϵͳ��Ϣ����

static OS_RSEMA			message_access_sema;	

int xm_open_volume_service (char *volume_name, int b_post_insert_message)
{
	int ret;
	// ����Ƶ�����
	OS_Use (&message_access_sema);
	
	// 20180609
	// ʹ��Ԥ�������Ƶ���������1
	XM_VideoItemSetQuotaProfile (XM_VIDEOITEM_QUOTA_PROFILE_1);
	ret = XM_VideoItemOpenService ();
	if(ret == 0)
	{
		// ��Ƶ��򿪳ɹ�
		extern void XM_SetCardState (DWORD dwValue);
						
		// ��ǿ��Ѳ����״̬
		XM_SetCardState (DEVCAP_SDCARDSTATE_INSERT);
						
		// ��鿨�Ŀ���ѭ��¼��ռ��Ƿ����������Ƶ¼������
		if(XM_VideoItemCheckCardRecycleSpace() == 0)
		{
			// �ܵĿ���ѭ���ռ��޷��������¼������
			XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_VIDEOITEM_LOW_SPACE);
		}
		// ��鿨������
		else if(XM_VideoItemCheckCardPerformance() == 0)		
		{
			// ������
			// ��ʾ��Ƶ��д������ܲ�����ѣ���ʾ��ʽ��
			XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_VIDEOITEM_LOW_PREFORMANCE);
		}
		else
		{
			// ���Ѳ���,����¼���Ҫ��
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
	XMSYS_H264CodecForcedEncoderStop ();	// ǿ��codec�˳�
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
		// SDMMC��
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
			//XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_DETECT);//ȡ��SD��׼������Ϣ
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
					// �Զ�Mount��SD��һ�����룬�����ֹ����棬�Զ������¼ģʽ
					ret = FS_Mount (temp);
					XM_printf ("FS_Mount(%s) ret=%d\n", temp, ret);
					//if(ret == 3)	// Volume is mounted read/write
					if(ret >= 0)	// Volume is mounted read/write
					{
						// �򿪾����
						ret = xm_open_volume_service (temp, 1);
						// ������̨������, ���ϵͳ�������Ƿ����
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

			// �رվ����
			xm_close_volume_service (temp);
			// ��һ���ļ�ɾ����������ִ�е�ʱ��, ���γ���Ѹ�ٲ���, ��ʱ��Ϣ�������ȼ�����CacheFile�������ȼ�,
			// ������Ϣ�������remove������(XM_VideoItemCloseService, FS_UnmountForced)(���γ�), ���������(FS_Mount, XM_VideoItemOpenService)����(������),
			// CacheFile�����޷��������еı仯, ����ʹ���ϵ��ļ����, ��Ϊ�����Ч�����·����쳣(����ϵͳ�쳣).
			
			// �ȴ�Cacheϵͳ�ر�
			XMSYS_WaitingForCacheSystemFlush (1000);
			
			// ��ʱ����0.5�����ʱ, �ȴ���������(CacheFile����)���˳�.
			OS_Delay (500);
			//XM_printf ("Card-Remove Delay end\n");
		}
		else if(card_event->plugIn == SDCARD_REINIT)	// reinit
		{
			// ��busy��ʱ����Ҫ�Կ���λ, ���³�ʼ�������ļ�ϵͳ
			int loop;
			XM_printf ("do SDCARD_REINIT\n");
			// �رվ����
			xm_close_volume_service (temp);
			OS_Delay (500);
			
			// ��ǿ�REINIT������, ��ֹREINIT��Ϣ�ݹ�Ͷ��
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
					// �Զ�Mount��SD��һ�����룬�����ֹ����棬�Զ������¼ģʽ
					ret = FS_Mount (temp);
					XM_printf ("FS_Mount(%s) ret=%d\n", temp, ret);
					//if(ret == 3)	// Volume is mounted read/write
					if(ret >= 0)	// Volume is mounted read/write
					{
						// �򿪾����
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
			
			// ��ǿ�REINIT���������
			SetSDMMCCardDoReInit (card_event->cardID, 0);
			
			if(loop == 0)
			{
				XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_INVALID);
				XM_SetFmlDeviceCap (DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_STOP);
			}
		}
					
		// ����UI֪ͨ�¼���UI����
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
		
		// ��ϵͳ��Ϣ���л�ȡһ����Ϣ
		XM_lock ();	// ���ж�
		message = (SYSTEM_MESSAGE *)queue_delete_next (&message_live);
		XM_unlock ();	// �ָ��ж�״̬
		
		//printk ("system message (%d)\n", message->event.uFEvent.fKind);
		// ������Ϣ
		switch (message->event.uFEvent.fKind)
		{
			case CARD_EVENT:
				// �����ϵͳ��Ϣ
				sdmmc_message_handler (&message->event.uFEvent.card_event);
				break;
		}
		
		// ����Ϣ��Ԫ���뵽���е�Ԫ����
		XM_lock ();	// ���ж�
		queue_insert ((queue_s *)message, &message_free);
		XM_unlock ();	// �ָ��ж�״̬
	}
}

int addEventQueue(fEvent *pEvent)
{
	SYSTEM_MESSAGE *message;
	if(pEvent == NULL)
		return -1;
	
	XM_lock ();	// ���ж�
	if(queue_empty (&message_free))
	{
		XM_unlock ();	// �ָ��ж�״̬
		XM_printf ("message queue full, fKind=%d\n", pEvent->uFEvent.fKind);
		return -1;
	}

	message = (SYSTEM_MESSAGE *)queue_delete_next (&message_free);
	memcpy (&message->event, pEvent, sizeof(fEvent));
	queue_insert ((queue_s *)message, &message_live);
	XM_unlock ();	// �ָ��ж�״̬
	
	OS_SignalCSema (&message_sema);
	
	return 0;
}

// 20170702
// �����ͬ����Ϣ�Ƿ�����ϵͳ��Ϣ�����д���.
// ����ĳЩ��Ϣ�Ĳ����ط�(SDCARD_REINIT)����ϵͳ��Ӧ�ٶ�
// ������, ����1
// ������, ����0
int chkEventQueue(fEvent *pEvent)
{
	SYSTEM_MESSAGE *message;
	int ret = 0;
	if(pEvent == NULL)
		return ret;
	
	XM_lock ();	// ���ж�
	if(queue_empty (&message_live))
	{
		// ��Ϣ���п�
		XM_unlock ();	// �ָ��ж�״̬
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
	XM_unlock ();	// �ָ��ж�״̬
	
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


// ����"ͬ��Ӧ���"����"�첽��Ϣ��"
int  XMSYS_MessageSocketTransferResponsePacket (unsigned int socket_type,  
																unsigned int command_no,			// ��������
												  				unsigned int command_id,			// �����ID
												  				unsigned char *command_packet_buffer,
												  				unsigned int command_packet_length
															)
{
	return 0;
}

