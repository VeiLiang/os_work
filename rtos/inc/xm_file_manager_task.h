//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_file_manager_task.h
//		异步文件操作
//
//	Revision history
//
//		2012.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_FILE_MANAGER_H_
#define _XM_FILE_MANAGER_H_

#define	XMSYS_FILE_MANAGER_FS_MOUNT			0x01		// 将卡挂接到文件系统
#define	XMSYS_FILE_MANAGER_CARD_CAPACITY		0x02		// 获取文件系统的容量
#define	XMSYS_FILE_MANAGER_CARD_FORMAT		0x04		// 卡文件系统格式化
#define	XMSYS_FILE_MANAGER_FILE_CHECK			0x08		// 文件检查
#define	XMSYS_FILE_MANAGER_FILE_DELETE		0x10		// 文件删除
#define	XMSYS_FILE_MANAGER_VOLUME_INFO		0x20		// 卡卷信息			
#define	XMSYS_FILE_MANAGER_SYSTEM_UPDATE		0x40		// 系统升级
#define	XMSYS_FILE_MANAGER_SYSTEM_SHUTDOWN	0x80		// 系统关闭或重启


// 文件管理器任务初始化
void XMSYS_FileManagerInit (void);

// 文件管理器任务结束
void XMSYS_FileManagerExit (void);

#define	XMSYS_CARD_STATE_INSERT			0		// 卡插入
#define	XMSYS_CARD_STATE_WITHDRAW		1		// 卡拔出
#define	XMSYS_CARD_STATE_FULL			2		// 卡写满
#define	XMSYS_CARD_STATE_DAMAGE			3		// 卡损坏 (读写校验失败)
#define	XMSYS_CARD_STATE_FSERROR		4		// 卡文件系统非法

int XMSYS_FileManagerGetCardState (void);

// 删除文件
// file_name为全路径名
int XMSYS_FileManagerFileDelete (unsigned char channel, unsigned char type, unsigned char file_name[8]);

// 读取SD卡的容量
int XMSYS_FileManagerGetCardCapacity (void);

int XMSYS_FileManagerCardFormat (void);

// 系统升级
int XMSYS_FileManagerSystemUpdate (void);

// 读取SD卡的卷容量
int XMSYS_FileManagerGetVolumeInfo (const char *volume, 
												unsigned int *SectorsPerCluster,
												unsigned int *BytesPerSector,
												unsigned int *NumberOfFreeClusters,
												unsigned int *TotalNumberOfClusters);

// 系统关机或重启
int XMSYS_FileManagerSystemShutDown (int bShutDownType);

#endif	// _XM_FILE_MANAGER_H_

