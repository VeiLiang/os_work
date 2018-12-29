//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_file_manager_task.h
//		�첽�ļ�����
//
//	Revision history
//
//		2012.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_FILE_MANAGER_H_
#define _XM_FILE_MANAGER_H_

#define	XMSYS_FILE_MANAGER_FS_MOUNT			0x01		// �����ҽӵ��ļ�ϵͳ
#define	XMSYS_FILE_MANAGER_CARD_CAPACITY		0x02		// ��ȡ�ļ�ϵͳ������
#define	XMSYS_FILE_MANAGER_CARD_FORMAT		0x04		// ���ļ�ϵͳ��ʽ��
#define	XMSYS_FILE_MANAGER_FILE_CHECK			0x08		// �ļ����
#define	XMSYS_FILE_MANAGER_FILE_DELETE		0x10		// �ļ�ɾ��
#define	XMSYS_FILE_MANAGER_VOLUME_INFO		0x20		// ������Ϣ			
#define	XMSYS_FILE_MANAGER_SYSTEM_UPDATE		0x40		// ϵͳ����
#define	XMSYS_FILE_MANAGER_SYSTEM_SHUTDOWN	0x80		// ϵͳ�رջ�����


// �ļ������������ʼ��
void XMSYS_FileManagerInit (void);

// �ļ��������������
void XMSYS_FileManagerExit (void);

#define	XMSYS_CARD_STATE_INSERT			0		// ������
#define	XMSYS_CARD_STATE_WITHDRAW		1		// ���γ�
#define	XMSYS_CARD_STATE_FULL			2		// ��д��
#define	XMSYS_CARD_STATE_DAMAGE			3		// ���� (��дУ��ʧ��)
#define	XMSYS_CARD_STATE_FSERROR		4		// ���ļ�ϵͳ�Ƿ�

int XMSYS_FileManagerGetCardState (void);

// ɾ���ļ�
// file_nameΪȫ·����
int XMSYS_FileManagerFileDelete (unsigned char channel, unsigned char type, unsigned char file_name[8]);

// ��ȡSD��������
int XMSYS_FileManagerGetCardCapacity (void);

int XMSYS_FileManagerCardFormat (void);

// ϵͳ����
int XMSYS_FileManagerSystemUpdate (void);

// ��ȡSD���ľ�����
int XMSYS_FileManagerGetVolumeInfo (const char *volume, 
												unsigned int *SectorsPerCluster,
												unsigned int *BytesPerSector,
												unsigned int *NumberOfFreeClusters,
												unsigned int *TotalNumberOfClusters);

// ϵͳ�ػ�������
int XMSYS_FileManagerSystemShutDown (int bShutDownType);

#endif	// _XM_FILE_MANAGER_H_

