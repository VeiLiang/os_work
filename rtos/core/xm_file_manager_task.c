//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_file_manager_task.c
//	  �ļ�����
//
//	Revision history
//
//		2012.09.01	ZhuoYongHong Initial version
//
//****************************************************************************#include <stdio.h>
#include "FS.h"
#include "hardware.h"
#include "xm_core.h"
#include <assert.h>
#include <string.h>
#include "xm_file_manager_task.h"
#include "xm_uvc_socket.h"
#include <xm_videoitem.h>
#include <xm_printf.h>
#include "xm_systemupdate.h"

extern int PostMediaFileListMessage (int ch, int type);


#define	FILEMANAGER_FS_MOUNT_TIMEOUT				(10*1000)		// SD���ҽ����ʱʱ�䣬10��
#define	FILEMANAGER_VOLUME_INFO_TIMEOUT			(10*1000)		// SD������Ϣ���ʱʱ�䣬10��

#define	FILEMANAGER_CARD_CAPACITY_TIMEOUT		(300)				// ��ȡ���������ʱʱ�䣬10��

#define	FILEMANAGER_TIMEOUT							(300)				// �ļ�������һ��������ʱʱ�䣬100ms��ʱ���

static OS_TASK TCB_FileManagerTask;
__no_init static OS_STACKPTR int StackFileManagerTask[XMSYS_FILE_MANAGER_TASK_STACK_SIZE/4];          /* Task stacks */

static OS_RSEMA filemanager_sema;			// ���Ᵽ��
static OS_MAILBOX filemanager_mailbox;		// һ�ֽ����䡣
static char filemanager_mailbuf;					// һ���ֽڵ�����


static unsigned char to_delete_file_name[8];
static unsigned char to_delete_channel;
static unsigned char to_delete_type;

static 	FS_DISK_INFO DiskInfo; 
static int shutdown_type;

static void XMSYS_FileManagerLock (void)
{
	OS_Use(&filemanager_sema); /* Make sure nobody else uses */
}

static void XMSYS_FileManagerUnlock (void)
{
	OS_Unuse(&filemanager_sema);
}

// codec����Ӧ���������������
static void filemanager_reply_response_message (char response_code)
{
	// �����������ݣ���ֹ����
	OS_ClearMB (&filemanager_mailbox);
	OS_PutMail1 (&filemanager_mailbox, &response_code);
}

// ��������Ӧ����Ϣ
static void filemanager_clear_response_message (void)
{
	OS_ClearMB (&filemanager_mailbox);
}

void XMSYS_FileManagerTask (void)
{
	OS_U8 os_event;
	unsigned char file[64];
	int ret;
	char resp_code;
	
	while(1)
	{
		// �ȴ��ⲿ�¼�
		os_event = OS_WaitEvent(	
											// 1) �����ʱ�Ŀ����ļ�ϵͳ�Ĳ���
											// 	�����ļ�ɾ��������ʽ������MOUNT����������ȡ
											//	2) ������������������������һ���ԵĲ���
											//		�ļ����
											XMSYS_FILE_MANAGER_CARD_CAPACITY
										|	XMSYS_FILE_MANAGER_VOLUME_INFO		
										|	XMSYS_FILE_MANAGER_FILE_DELETE
										|	XMSYS_FILE_MANAGER_CARD_FORMAT	
										|	XMSYS_FILE_MANAGER_SYSTEM_UPDATE	
										|	XMSYS_FILE_MANAGER_SYSTEM_SHUTDOWN
											);
		
		if(os_event & XMSYS_FILE_MANAGER_FILE_DELETE)
		{
			// 
			XMBOOL result;
			unsigned char notification_delete[11];
			//printf ("FileManagerTask FILE_DELETE\n");
			XMSYS_FileManagerLock ();	// ��������
			combine_fullpath_media_filename(file, sizeof(file), to_delete_channel, to_delete_type, to_delete_file_name);

			XM_printf(">>>>del fiel:%s\r\n", file);
			// Ӧ��client
			filemanager_reply_response_message (0);
			notification_delete[0] = to_delete_channel;
			notification_delete[1] = to_delete_type;
			memcpy (notification_delete+2, to_delete_file_name, 8);
			XMSYS_FileManagerUnlock ();	// �������
						
			// ɾ���ļ�
			if(to_delete_type == 0)		// ��Ƶ��
				result = XM_VideoItemDeleteVideoItemHandleFromFileName ((char *)file);
			else
			{
				result = XMSYS_remove_file ((char *)file);
				if(result)
				{
					// �����ļ��б������Ϣ(�ļ�ɾ��)
					XMSYS_UvcSocketFileListUpdate (to_delete_channel, to_delete_type, to_delete_file_name, 0, 0, 0);
				}
			}

			if(result)	// �ɹ�
				notification_delete[10] = 0;														 
			else			// ʧ��
				notification_delete[10] = 1;	
						
			// Ͷ���첽��Ϣ
			XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_ASYC,
																	XMSYS_UVC_NID_FILE_DELETE_FINISH,
																	notification_delete,
																	11
																	);
		}
		
		if(os_event & XMSYS_FILE_MANAGER_CARD_CAPACITY)
		{
			unsigned int spaces[4];
			XMINT64 TotalSpace, LockedSpace, RecycleSpace, OtherSpace;
			// ��ȡSD������
			// Ӧ��client
			printf ("FileManagerTask CARD_CAPACITY\n");
			filemanager_reply_response_message (0);
			// 16�ֽڳ��ȣ�
			//	SD���ռ䣨4�ֽڣ�MB��λ���������ļ��ռ䣨4�ֽڣ�MB��λ����
			//	ѭ��¼��ռ䣨4�ֽڣ�MB��λ��������ռ�ÿռ䣨4�ֽڣ�MB��λ��
			// ��ȡ���̿ռ�
			TotalSpace = 0;
			LockedSpace = 0;
			RecycleSpace = 0;
			OtherSpace = 0;
			if(XM_VideoItemGetDiskUsage (&TotalSpace, &LockedSpace, &RecycleSpace, &OtherSpace) == 0)
			{
				TotalSpace = TotalSpace >> 20;
				LockedSpace = LockedSpace >> 20;
				RecycleSpace = RecycleSpace >> 20;
				OtherSpace = OtherSpace >> 20;
							  
				spaces[0] = (unsigned int)TotalSpace;
				spaces[1] = (unsigned int)LockedSpace;
				spaces[2] = (unsigned int)RecycleSpace;
				spaces[3] = (unsigned int)OtherSpace;
				
				XM_printf ("FILE_MANAGER_CARD_CAPACITY\n");
				XM_printf ("\tTotalSpace = %d MB, LockedSpace = %d MB, RecycleSpace = %d MB, OtherSpace = %d MB\n",
										spaces[0], spaces[1], spaces[2], spaces[3]);
				XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_ASYC,
																	XMSYS_UVC_NID_SDCARD_CAPACITY,
																	(unsigned char *)spaces,
																	16
																	);
			}
			//printf ("FileManagerTask CARD_CAPACITY end\n");
		}
		
		if(os_event & XMSYS_FILE_MANAGER_VOLUME_INFO)
		{
			printf ("XMSYS_FILE_MANAGER_VOLUME_INFO event\n");
			if(FS_GetVolumeInfo ("mmc:0:", &DiskInfo) == 0)
			{
				ret = 0;
			}
			else
			{
				ret = -1;
			}
			printf ("FS_GetVolumeInfo %s\n", ret == 0 ? "OK" : "NG");
			filemanager_reply_response_message (ret);
		}
		
		if(os_event & XMSYS_FILE_MANAGER_CARD_FORMAT)
		{
			int loop = 4;
			printf ("XMSYS_FILE_MANAGER_CARD_FORMAT event\n");
			// Ӧ��, ��ʼִ�и�ʽ������
			filemanager_reply_response_message (0);		
			
			
			// ����UVC����ͨ�������ٽ���socket����
			XMSYS_UvcSocketSetEnable (0);
			
			// ֹͣ���Ż�¼���
			XMSYS_H264CodecStop ();
			
			xm_close_volume_service ("mmc:0");
				
			while (loop > 0)
			{
				FS_FORMAT_INFO FormatInfo;
				FormatInfo.SectorsPerCluster = 128;		// 64, ����Ĵش�С, ���ƴط���/�ͷ�/д��/��ȡ���ٶ�
				FormatInfo.NumRootDirEntries = 512;
				FormatInfo.pDevInfo = NULL;
				
				if(FS_Format ("mmc:0:", &FormatInfo) == 0)
				{
					FS_CACHE_Clean ("");
					if(FS_Mount ("mmc:0:") >= 0)
					{
						if(xm_open_volume_service("mmc:0:", 0) == 0)
						{
							resp_code = 0;
							break;
						}
						else
						{
							//resp_code = 1;
						}							
					}
					else
					{
						//XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_INVALID);
						//resp_code = 1;
					}
				}
				
				loop --;
			}
			
			if(loop == 0)
			{
				// ��ʽ��ʧ��, ��ǿ���Ч
				XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_INVALID);
				resp_code = 1;
			}
			else
			{
				resp_code = 0;
			}
			printf ("FS_Format %s\n", resp_code == 0 ? "OK" : "NG");
			// Ͷ���첽��Ϣ, ֪ͨ��ʽ�������Ľ��
			XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_ASYC,
																XMSYS_UVC_NID_SDCARD_FORMAT_FINISH,
																(unsigned char *)&resp_code,
																1
																);
			
			// ��������ͨ�������½���socket����
			XMSYS_UvcSocketSetEnable (1);
		}
		
		if(os_event & XMSYS_FILE_MANAGER_SYSTEM_UPDATE)
		{
			// ������ִ��ǰ,��Ҫ�������ֹͣ
			printf ("XMSYS_FILE_MANAGER_SYSTEM_UPDATE event\n");
			// Ӧ��0
			filemanager_reply_response_message (0);
			// ִ��ϵͳ����
			XMSYS_system_update ();
		}
		
		if(os_event & XMSYS_FILE_MANAGER_SYSTEM_SHUTDOWN)
		{
			// Ӧ��0
			filemanager_reply_response_message (0);
			//�ȴ�Ӧ������͸�������
			OS_Delay (300);			
						
			if(shutdown_type == XM_SHUTDOWN_POWERDOWN)
			{
				XM_printf ("POWEROFF\n");
				// XM_ShutDownSystem (SDS_POWEROFF);
				XM_ShutDownSystem (SDS_POWERACC);
			}
			else
			{
				XM_printf ("REBOOT\n");
				XM_ShutDownSystem (SDS_REBOOT);
			}
		}
		
	}
}



// �ļ������������ʼ��
void XMSYS_FileManagerInit (void)
{	
	OS_CREATERSEMA(&filemanager_sema); /* Creates resource semaphore */
	OS_CREATEMB(&filemanager_mailbox, 1, 1, &filemanager_mailbuf);	

	OS_CREATETASK(&TCB_FileManagerTask, "FileManager", XMSYS_FileManagerTask, XMSYS_FILE_MANAGER_TASK_PRIORITY, StackFileManagerTask);
}

// �ļ��������������
void XMSYS_FileManagerExit (void)
{
	
}



// ��ȡSD��������
int XMSYS_FileManagerGetCardCapacity (void)
{
	char response_code;
	int ret = -1;
	
	// ǿ����������������Ϣ����������
	filemanager_clear_response_message ();
		
	OS_SignalEvent (XMSYS_FILE_MANAGER_CARD_CAPACITY, &TCB_FileManagerTask);
		
	// �ȴ��ļ��������߳�Ӧ��
	if(OS_GetMailTimed (&filemanager_mailbox, &response_code, FILEMANAGER_CARD_CAPACITY_TIMEOUT) == 0)
	{
		// codec��Ӧ�𣬻����Ϣ
		ret = (int)response_code;
		printf ("XMSYS_FileManagerGetCardCapacity resp=%d\n", ret);
	}
	else
	{
		// ��ʱ
		printf ("XMSYS_FileManagerGetCardCapacity timeout\n");
		ret = -2;
	}	
	return ret;
}

// ��ȡSD���ľ�����
int XMSYS_FileManagerGetVolumeInfo(const char *volume, 
												unsigned int *SectorsPerCluster,
												unsigned int *BytesPerSector,
												unsigned int *NumberOfFreeClusters,
												unsigned int *TotalNumberOfClusters)
{
	char response_code;
	int ret = -1;
	if(volume == NULL)
		return -1;
	
	memset (&DiskInfo, 0, sizeof(DiskInfo));
	
	// ǿ����������������Ϣ����������
	filemanager_clear_response_message ();
		
	OS_SignalEvent (XMSYS_FILE_MANAGER_VOLUME_INFO, &TCB_FileManagerTask);
		
	// �ȴ��ļ��������߳�Ӧ��
	if(OS_GetMailTimed (&filemanager_mailbox, &response_code, FILEMANAGER_VOLUME_INFO_TIMEOUT) == 0)
	{
		// codec��Ӧ�𣬻����Ϣ
		ret = (int)response_code;
		printf ("XMSYS_FileManagerGetVolumeInfo resp=%d\n", ret);
		if(ret == 0)
		{
			if(SectorsPerCluster)
				*SectorsPerCluster = DiskInfo.SectorsPerCluster;
			if(BytesPerSector)
				*BytesPerSector = DiskInfo.BytesPerSector;
			if(NumberOfFreeClusters)
				*NumberOfFreeClusters = DiskInfo.NumFreeClusters;
			if(TotalNumberOfClusters)
				*TotalNumberOfClusters = DiskInfo.NumTotalClusters;
		}
	}
	else
	{
		// ��ʱ
		printf ("XMSYS_FileManagerGetVolumeInfo timeout\n");
		ret = -2;		
	}	
	return ret;
}

// �������SD���Ƿ����
int XMSYS_check_sd_card_exist (void);

// ϵͳ��ʽ��
int XMSYS_FileManagerCardFormat (void)
{
	char response_code;
	int ret = -1;
	
	if(XMSYS_check_sd_card_exist() == 0)
	{
		ret = -1;
		return ret;
	}
	
	
	// ǿ����������������Ϣ����������
	filemanager_clear_response_message ();
	
	XMSYS_FileManagerLock ();		// ��������
	OS_SignalEvent (XMSYS_FILE_MANAGER_CARD_FORMAT, &TCB_FileManagerTask);
	XMSYS_FileManagerUnlock ();	// �������
		
	// �ȴ��ļ��������߳�Ӧ��
	if(OS_GetMailTimed (&filemanager_mailbox, &response_code, FILEMANAGER_TIMEOUT) == 0)
	{
		// codec��Ӧ�𣬻����Ϣ
		ret = (int)response_code;
		//printf ("FileManagerFileDelete resp=%d\n", ret);
	}
	else
	{
		// ��ʱ
		//printf ("FileManagerFileDelete timeout\n");
		ret = -2;
	}	
	return ret;
}

// ɾ���ļ�
// file_nameΪȫ·����
int XMSYS_FileManagerFileDelete (unsigned char channel, unsigned char type, unsigned char file_name[8])
{
	char response_code;
	int ret = -1;
	
	// ǿ����������������Ϣ����������
	filemanager_clear_response_message ();
	
	XMSYS_FileManagerLock ();		// ��������
	memset (to_delete_file_name, 0, sizeof(to_delete_file_name));
	memcpy (to_delete_file_name, file_name, 8);
	to_delete_channel = channel;
	to_delete_type = type;
	OS_SignalEvent(XMSYS_FILE_MANAGER_FILE_DELETE, &TCB_FileManagerTask);
	XMSYS_FileManagerUnlock ();	// �������
		
	// �ȴ��ļ��������߳�Ӧ��
	if(OS_GetMailTimed (&filemanager_mailbox, &response_code, FILEMANAGER_TIMEOUT) == 0)
	{
		// codec��Ӧ�𣬻����Ϣ
		ret = (int)response_code;
		//printf ("FileManagerFileDelete resp=%d\n", ret);
	}
	else
	{
		// ��ʱ
		//printf ("FileManagerFileDelete timeout\n");
		ret = -2;
	}	
	return ret;
}

// ϵͳ����
int XMSYS_FileManagerSystemUpdate (void)
{
	char response_code;
	int ret = -1;
	
	// ǿ����������������Ϣ����������
	filemanager_clear_response_message ();
	
	XMSYS_FileManagerLock ();		// ��������
	OS_SignalEvent (XMSYS_FILE_MANAGER_SYSTEM_UPDATE, &TCB_FileManagerTask);
	XMSYS_FileManagerUnlock ();	// �������
		
	// �ȴ��ļ��������߳�Ӧ��
	if(OS_GetMailTimed (&filemanager_mailbox, &response_code, FILEMANAGER_TIMEOUT) == 0)
	{
		// codec��Ӧ�𣬻����Ϣ
		ret = (int)response_code;
	}
	else
	{
		// ��ʱ
		ret = -2;
	}	
	return ret;	
}

// ϵͳ�ػ�������
int XMSYS_FileManagerSystemShutDown (int bShutDownType)
{
	char response_code;
	int ret = -1;
	
	// ǿ����������������Ϣ����������
	filemanager_clear_response_message ();
	
	XMSYS_FileManagerLock ();		// ��������
	shutdown_type = bShutDownType;
	OS_SignalEvent (XMSYS_FILE_MANAGER_SYSTEM_SHUTDOWN, &TCB_FileManagerTask);
	XMSYS_FileManagerUnlock ();	// �������
		
	// �ȴ��ļ��������߳�Ӧ��
	if(OS_GetMailTimed (&filemanager_mailbox, &response_code, FILEMANAGER_TIMEOUT) == 0)
	{
		// codec��Ӧ�𣬻����Ϣ
		ret = (int)response_code;
	}
	else
	{
		// ��ʱ
		ret = -2;
	}	
	return ret;		
}