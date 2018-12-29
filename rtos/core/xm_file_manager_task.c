//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_file_manager_task.c
//	  文件管理
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


#define	FILEMANAGER_FS_MOUNT_TIMEOUT				(10*1000)		// SD卡挂接最大超时时间，10秒
#define	FILEMANAGER_VOLUME_INFO_TIMEOUT			(10*1000)		// SD卡卷信息最大超时时间，10秒

#define	FILEMANAGER_CARD_CAPACITY_TIMEOUT		(300)				// 获取卷容量最大超时时间，10秒

#define	FILEMANAGER_TIMEOUT							(300)				// 文件管理器一般操作最大超时时间，100ms超时间隔

static OS_TASK TCB_FileManagerTask;
__no_init static OS_STACKPTR int StackFileManagerTask[XMSYS_FILE_MANAGER_TASK_STACK_SIZE/4];          /* Task stacks */

static OS_RSEMA filemanager_sema;			// 互斥保护
static OS_MAILBOX filemanager_mailbox;		// 一字节邮箱。
static char filemanager_mailbuf;					// 一个字节的邮箱


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

// codec返回应答码给服务请求者
static void filemanager_reply_response_message (char response_code)
{
	// 清除信箱的内容，防止阻塞
	OS_ClearMB (&filemanager_mailbox);
	OS_PutMail1 (&filemanager_mailbox, &response_code);
}

// 清除缓存的应答消息
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
		// 等待外部事件
		os_event = OS_WaitEvent(	
											// 1) 处理耗时的卡、文件系统的操作
											// 	包括文件删除、卡格式化、卡MOUNT、卡容量获取
											//	2) 及其他必须与上述操作保持一致性的操作
											//		文件检查
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
			XMSYS_FileManagerLock ();	// 互斥锁定
			combine_fullpath_media_filename(file, sizeof(file), to_delete_channel, to_delete_type, to_delete_file_name);

			XM_printf(">>>>del fiel:%s\r\n", file);
			// 应答client
			filemanager_reply_response_message (0);
			notification_delete[0] = to_delete_channel;
			notification_delete[1] = to_delete_type;
			memcpy (notification_delete+2, to_delete_file_name, 8);
			XMSYS_FileManagerUnlock ();	// 互斥解锁
						
			// 删除文件
			if(to_delete_type == 0)		// 视频项
				result = XM_VideoItemDeleteVideoItemHandleFromFileName ((char *)file);
			else
			{
				result = XMSYS_remove_file ((char *)file);
				if(result)
				{
					// 发送文件列表更新消息(文件删除)
					XMSYS_UvcSocketFileListUpdate (to_delete_channel, to_delete_type, to_delete_file_name, 0, 0, 0);
				}
			}

			if(result)	// 成功
				notification_delete[10] = 0;														 
			else			// 失败
				notification_delete[10] = 1;	
						
			// 投递异步消息
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
			// 读取SD卡容量
			// 应答client
			printf ("FileManagerTask CARD_CAPACITY\n");
			filemanager_reply_response_message (0);
			// 16字节长度，
			//	SD卡空间（4字节，MB单位）、锁定文件空间（4字节，MB单位）、
			//	循环录像空间（4字节，MB单位）、其他占用空间（4字节，MB单位）
			// 获取磁盘空间
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
			// 应答, 开始执行格式化操作
			filemanager_reply_response_message (0);		
			
			
			// 锁定UVC命令通道，不再接收socket命令
			XMSYS_UvcSocketSetEnable (0);
			
			// 停止播放或录像过
			XMSYS_H264CodecStop ();
			
			xm_close_volume_service ("mmc:0");
				
			while (loop > 0)
			{
				FS_FORMAT_INFO FormatInfo;
				FormatInfo.SectorsPerCluster = 128;		// 64, 更大的簇大小, 改善簇分配/释放/写入/读取的速度
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
				// 格式化失败, 标记卡无效
				XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_INVALID);
				resp_code = 1;
			}
			else
			{
				resp_code = 0;
			}
			printf ("FS_Format %s\n", resp_code == 0 ? "OK" : "NG");
			// 投递异步消息, 通知格式化操作的结果
			XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_ASYC,
																XMSYS_UVC_NID_SDCARD_FORMAT_FINISH,
																(unsigned char *)&resp_code,
																1
																);
			
			// 开启命令通道，重新接收socket命令
			XMSYS_UvcSocketSetEnable (1);
		}
		
		if(os_event & XMSYS_FILE_MANAGER_SYSTEM_UPDATE)
		{
			// 主机端执行前,需要将编解码停止
			printf ("XMSYS_FILE_MANAGER_SYSTEM_UPDATE event\n");
			// 应答0
			filemanager_reply_response_message (0);
			// 执行系统升级
			XMSYS_system_update ();
		}
		
		if(os_event & XMSYS_FILE_MANAGER_SYSTEM_SHUTDOWN)
		{
			// 应答0
			filemanager_reply_response_message (0);
			//等待应答包发送给主机端
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



// 文件管理器任务初始化
void XMSYS_FileManagerInit (void)
{	
	OS_CREATERSEMA(&filemanager_sema); /* Creates resource semaphore */
	OS_CREATEMB(&filemanager_mailbox, 1, 1, &filemanager_mailbuf);	

	OS_CREATETASK(&TCB_FileManagerTask, "FileManager", XMSYS_FileManagerTask, XMSYS_FILE_MANAGER_TASK_PRIORITY, StackFileManagerTask);
}

// 文件管理器任务结束
void XMSYS_FileManagerExit (void)
{
	
}



// 读取SD卡的容量
int XMSYS_FileManagerGetCardCapacity (void)
{
	char response_code;
	int ret = -1;
	
	// 强制清除缓存的信箱信息，避免阻塞
	filemanager_clear_response_message ();
		
	OS_SignalEvent (XMSYS_FILE_MANAGER_CARD_CAPACITY, &TCB_FileManagerTask);
		
	// 等待文件管理器线程应答
	if(OS_GetMailTimed (&filemanager_mailbox, &response_code, FILEMANAGER_CARD_CAPACITY_TIMEOUT) == 0)
	{
		// codec已应答，获得消息
		ret = (int)response_code;
		printf ("XMSYS_FileManagerGetCardCapacity resp=%d\n", ret);
	}
	else
	{
		// 超时
		printf ("XMSYS_FileManagerGetCardCapacity timeout\n");
		ret = -2;
	}	
	return ret;
}

// 读取SD卡的卷容量
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
	
	// 强制清除缓存的信箱信息，避免阻塞
	filemanager_clear_response_message ();
		
	OS_SignalEvent (XMSYS_FILE_MANAGER_VOLUME_INFO, &TCB_FileManagerTask);
		
	// 等待文件管理器线程应答
	if(OS_GetMailTimed (&filemanager_mailbox, &response_code, FILEMANAGER_VOLUME_INFO_TIMEOUT) == 0)
	{
		// codec已应答，获得消息
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
		// 超时
		printf ("XMSYS_FileManagerGetVolumeInfo timeout\n");
		ret = -2;		
	}	
	return ret;
}

// 检查物理SD卡是否插入
int XMSYS_check_sd_card_exist (void);

// 系统格式化
int XMSYS_FileManagerCardFormat (void)
{
	char response_code;
	int ret = -1;
	
	if(XMSYS_check_sd_card_exist() == 0)
	{
		ret = -1;
		return ret;
	}
	
	
	// 强制清除缓存的信箱信息，避免阻塞
	filemanager_clear_response_message ();
	
	XMSYS_FileManagerLock ();		// 互斥锁定
	OS_SignalEvent (XMSYS_FILE_MANAGER_CARD_FORMAT, &TCB_FileManagerTask);
	XMSYS_FileManagerUnlock ();	// 互斥解锁
		
	// 等待文件管理器线程应答
	if(OS_GetMailTimed (&filemanager_mailbox, &response_code, FILEMANAGER_TIMEOUT) == 0)
	{
		// codec已应答，获得消息
		ret = (int)response_code;
		//printf ("FileManagerFileDelete resp=%d\n", ret);
	}
	else
	{
		// 超时
		//printf ("FileManagerFileDelete timeout\n");
		ret = -2;
	}	
	return ret;
}

// 删除文件
// file_name为全路径名
int XMSYS_FileManagerFileDelete (unsigned char channel, unsigned char type, unsigned char file_name[8])
{
	char response_code;
	int ret = -1;
	
	// 强制清除缓存的信箱信息，避免阻塞
	filemanager_clear_response_message ();
	
	XMSYS_FileManagerLock ();		// 互斥锁定
	memset (to_delete_file_name, 0, sizeof(to_delete_file_name));
	memcpy (to_delete_file_name, file_name, 8);
	to_delete_channel = channel;
	to_delete_type = type;
	OS_SignalEvent(XMSYS_FILE_MANAGER_FILE_DELETE, &TCB_FileManagerTask);
	XMSYS_FileManagerUnlock ();	// 互斥解锁
		
	// 等待文件管理器线程应答
	if(OS_GetMailTimed (&filemanager_mailbox, &response_code, FILEMANAGER_TIMEOUT) == 0)
	{
		// codec已应答，获得消息
		ret = (int)response_code;
		//printf ("FileManagerFileDelete resp=%d\n", ret);
	}
	else
	{
		// 超时
		//printf ("FileManagerFileDelete timeout\n");
		ret = -2;
	}	
	return ret;
}

// 系统升级
int XMSYS_FileManagerSystemUpdate (void)
{
	char response_code;
	int ret = -1;
	
	// 强制清除缓存的信箱信息，避免阻塞
	filemanager_clear_response_message ();
	
	XMSYS_FileManagerLock ();		// 互斥锁定
	OS_SignalEvent (XMSYS_FILE_MANAGER_SYSTEM_UPDATE, &TCB_FileManagerTask);
	XMSYS_FileManagerUnlock ();	// 互斥解锁
		
	// 等待文件管理器线程应答
	if(OS_GetMailTimed (&filemanager_mailbox, &response_code, FILEMANAGER_TIMEOUT) == 0)
	{
		// codec已应答，获得消息
		ret = (int)response_code;
	}
	else
	{
		// 超时
		ret = -2;
	}	
	return ret;	
}

// 系统关机或重启
int XMSYS_FileManagerSystemShutDown (int bShutDownType)
{
	char response_code;
	int ret = -1;
	
	// 强制清除缓存的信箱信息，避免阻塞
	filemanager_clear_response_message ();
	
	XMSYS_FileManagerLock ();		// 互斥锁定
	shutdown_type = bShutDownType;
	OS_SignalEvent (XMSYS_FILE_MANAGER_SYSTEM_SHUTDOWN, &TCB_FileManagerTask);
	XMSYS_FileManagerUnlock ();	// 互斥解锁
		
	// 等待文件管理器线程应答
	if(OS_GetMailTimed (&filemanager_mailbox, &response_code, FILEMANAGER_TIMEOUT) == 0)
	{
		// codec已应答，获得消息
		ret = (int)response_code;
	}
	else
	{
		// 超时
		ret = -2;
	}	
	return ret;		
}