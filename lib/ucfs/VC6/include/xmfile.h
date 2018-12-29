//****************************************************************************
//
//	Copyright (C) 2011 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: xmfile.h
//	  constant��macro, data structure��function protocol definition of file interface
//
//	Revision history
//
//		2011.05.27	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_FILE_H_
#define _XM_FILE_H_

#if defined (__cplusplus)
	extern "C"{
#endif

#include  <xm_proj_define.h>
#include <fs.h>
#include <xmtype.h>
#include <xmbase.h>

/////////////////////////////////////////////////////////////////////
//
// ******************   �ļ����ҷ��� FindFind     *******************
//
/////////////////////////////////////////////////////////////////////

// ��С�ļ����һ������ֽڴ�С	
#define	XM_MAX_FILEFIND_NAME				0x80//14
#define	XM_STD_VIDEOFILENAME				14
#define	XM_STD_VIDEOFILENAME_SIZE		8			// ��ʽΪ YMDhmmss
#define	XM_STD_VIDEOFILEEXT_SIZE		3			// ��ʽΪ .MP4

#define	D_NORMAL					0			/* normal			*/
#define	D_RDONLY					0x01		/* read-only file	*/
#define	D_HIDDEN					0x02		/* hidden			*/
#define	D_SYSTEM					0x04		/* system			*/
#define	D_VOLID					0x08		/* volume id		*/
#define	D_DIR						0x10		/* subdir			*/
#define	D_ARCHIVE				0x20		/* archive bit		*/
#define	D_LONGNAME				(D_RDONLY|D_HIDDEN|D_SYSTEM|D_VOLID)
#define	D_LONGNAMEMASK			(D_RDONLY|D_HIDDEN |D_SYSTEM |D_VOLID|D_DIR|D_ARCHIVE)

#define	ATTR_DIRECTORY			D_DIR
#define	ATTR_VOLUME_ID			D_VOLID
#define	ATTR_LONG_NAME			(D_RDONLY|D_HIDDEN |D_SYSTEM |D_VOLID)
#define	ATTR_LONG_NAME_MASK	(D_RDONLY|D_HIDDEN |D_SYSTEM |D_VOLID|D_DIR|D_ARCHIVE)

// file attributes constant �ļ����Զ���
#define XM_FILE_ATTRIBUTE_READONLY        0x00000001
#define XM_FILE_ATTRIBUTE_HIDDEN          0x00000002
#define XM_FILE_ATTRIBUTE_SYSTEM          0x00000004
#define XM_FILE_ATTRIBUTE_DIRECTORY       0x00000010
#define XM_FILE_ATTRIBUTE_ARCHIVE         0x00000020
#define XM_FILE_ATTRIBUTE_NORMAL          0x00000080

// �򻯰���ļ����ң���֧��ASCII������ļ�ϵͳ��
typedef struct tagXMFILEFIND {
	DWORD				dwFileAttributes;		// Specifies the file attributes of the file found �ļ�����. 	
	XMSYSTEMTIME	CreationTime;			// file creation time �ļ�����ʱ��
	XMSYSTEMTIME	LastWriteTime;			// ���һ��д��ʱ��
	DWORD				nFileSize;				// Specifies the DWORD value of the file size, in bytes �ļ���С.
	VOID*				Reserved;				// ˽�����ݣ��ļ������ڲ�ʹ�ã������Զ������
	DWORD				magic;
	FS_FIND_DATA	FindData;
} XMFILEFIND;

// �ļ����ҳ�ʼ����
// lpFileName ָ�����ҵ��ļ���Ŀ¼��
// lpFileNameBuffer��cbFileNameBuffer����һ�����壬������ҹ��̷��ص��ļ���Ŀ¼��
XMBOOL 	XM_FindFirstFile	(char* lpFileName, XMFILEFIND *lpFindFileData, char *lpFileNameBuffer, int cbFileNameBuffer);
// ������һ���ļ�
XMBOOL 	XM_FindNextFile	(XMFILEFIND * lpFindFileData);
// �ر��ļ�����
VOID 		XM_FindClose		(XMFILEFIND * lpFindFileData);

// �ļ����Զ�ȡ
DWORD		XM_GetFileAttributes (
  const char * lpFileName   // name of file or directory
);

// �ļ���������
XMBOOL	XM_SetFileAttributes (
  const char * lpFileName,      // file name
  DWORD dwFileAttributes   // attributes
);

// ��ϵͳʱ��õ�����DOS8.3�ļ��������淶��ǰ׺�ļ�����8���ַ�����YMDhmmss��
// �ļ�����ʹ��36��ASCII�ַ���"0123456789ABCDEFGH...Z"
// '0'��ʾ0����ĸ'A'��ʾ10����ĸ'Z'��ʾ35
// Y  Ϊ����2013����ʼ��ʱ�䣬'0' ~ 'Z'��ʾ2013 ~ 2048��
// M  '0' ~ 'B'   һ���ַ����룬��ʾ1 ~ 12��
// D  '0' ~ 'U'   һ���ַ����룬��ʾ1 ~ 31��
// h  '0' ~ 'O'   һ���ַ����룬��ʾ0 ~ 23ʱ
// mm '00' ~ '59' �����ַ����룬��ʾ0 ~ 59��
// ss '00' ~ '59' �����ַ����룬��ʾ0 ~ 59��
// �ɹ�����1��ʧ�ܷ���0
int XM_SystemTime2FileName (const XMSYSTEMTIME* pSystemTime, char *lpFileName, int cbFileName);

// ���ļ�����ȡϵͳʱ�䡣�ļ���������XMSYS_SystemTime2FileName�Ķ���
int XM_FileName2SystemTime (XMSYSTEMTIME* pSystemTime, const char *lpFileName, int cbFileName);




/////////////////////////////////////////////////////////////////////
//
// ****************** �ڴ�ӳ���ļ��ӿڣ�ֻ�����ʣ�*******************
//
/////////////////////////////////////////////////////////////////////

#define	XM_MAPFILEID		0x4650414d	// 'MAPF'

#define	XM_SEEK_SET       0	/* offset is beginning of file  */
#define	XM_SEEK_CUR       1	/* offset is relative to current position of file pointer */
#define	XM_SEEK_END       2	/* offset is relative to end of file */


typedef struct _tagXMMAPFILE {
	DWORD	id;				// ��Чӳ���ļ���ID����ΪXM_MAPFILEID
	const void *base;		// �ڴ�ӳ���ļ���ַ
	LONG	size;				// �ڴ�ӳ���ļ���С
	LONG	offset;			//	��ǰ��ָ��ƫ��(��Ի�ַ)
} XMMAPFILE;

typedef XMBOOL	(*FPXM_MAPFILEOPEN) (const VOID *base, LONG size, XMMAPFILE *lpMapFile);
typedef LONG	(*FPXM_MAPFILEREAD) (XMMAPFILE *lpMapFile, VOID *lpBuffer, LONG cbBuffer);
typedef LONG	(*FPXM_MAPFILESEEK) (XMMAPFILE *lpMapFile, LONG offset, LONG whence);
typedef LONG	(*FPXM_MAPFILETELL) (XMMAPFILE *lpMapFile);
typedef XMBOOL	(*FPXM_MAPFILECLOSE)(XMMAPFILE *lpMapFile);


// �����ڴ�ӳ���ļ�
XMBOOL	XM_MapFileOpen (const VOID *base, LONG size, XMMAPFILE *lpMapFile);

// ���ڴ�ӳ���ļ���ǰ��ָ��λ�ö�ȡ�ֽ���ΪcbBuffer�����ݵ�������lpBuffer
// ����ֵ < 0    ��ʾ����
//        = 0    ��ʾ�ﵽ�ļ�β��
//        ����ֵ ��ʾ��ȡ���ֽ���
LONG		XM_MapFileRead	(XMMAPFILE *lpMapFile, VOID *lpBuffer, LONG cbBuffer);

// ����ָ�붨λ��ָ����λ��
// ����ֵ < 0		��ʾ����
//        ����ֵ	����ǰ�Ķ�ָ��λ��
LONG		XM_MapFileSeek (XMMAPFILE *lpMapFile, LONG offset, LONG whence);

// ��ȡ�ڴ�ӳ���ļ���ǰ�Ķ�ָ��
LONG		XM_MapFileTell	(XMMAPFILE *lpMapFile);

// �ر��ڴ�ӳ���ļ�
XMBOOL	XM_MapFileClose (XMMAPFILE *lpMapFile);

// ɾ���ļ�
XMBOOL	XM_RemoveFile (const char *pFileName);

// �����ļ�Ŀ¼
// �ɹ�����1��ʧ�ܷ���0
XMBOOL	XM_MkDir (const char *pDirName);

// ɾ���ļ�Ŀ¼
// �ɹ�����1��ʧ�ܷ���0
XMBOOL	XM_RmDir (const char *pDirName);

// ��ȡ�ļ��Ĵ�С
// ʧ�ܷ���(-1), ����Ϊ�ļ����ֽڴ�С(���ֵ 0xFFFFFFFE)
DWORD XM_GetFileSize (const char *pFileName);

// �ļ�����
// sOldName   ȫ·���� "mmc:0:\\VIDEO_F\\TEMP1234.AVI"
// sNewName	  �ļ���   "01234567.AVI"	, ��ʾ��"mmc:0:\\VIDEO_F\\TEMP1234.AVI"����Ϊ"mmc:0:\\VIDEO_F\\01234567.AVI"
// 0 	��ʾ�ļ������ɹ�
// -1	��ʾ�ļ�����ʧ��
int XM_rename (const char * sOldName, const char * sNewName);



// ��ȡ����ʣ��ռ估������Ϣ
XMBOOL	XM_GetDiskFreeSpace (
  const char * lpRootPathName,          // root path
  DWORD      * lpSectorsPerCluster,     // sectors per cluster
  DWORD      * lpBytesPerSector,        // bytes per sector
  DWORD      * lpNumberOfFreeClusters,  // free clusters
  DWORD      * lpTotalNumberOfClusters, // total clusters
  const char * lpVolumeName             // volume name
);

// ���ָ���豸���ļ�ϵͳ�Ƿ���Mount
XMBOOL	XM_IsVolumeMounted (int dev);

void *XM_fopen (const char *file, const char *mode);
int XM_fseek( void *stream, long offset, int origin );
unsigned int XM_fread( void *buffer, unsigned int size, unsigned int count, void *stream );
unsigned int XM_fwrite ( void *buffer, unsigned int size, unsigned int count, void *stream );
// Native�ӿڣ����ļ��ر�
int XM_fclose( void *stream );
// Native�ӿڣ���ȡ���ļ��ֽڳ���
unsigned int XM_filesize ( void *stream );

// �ļ��ض�У��
// ���ļ���ǰλ�ö�ȡָ���ֽڳ��ȵ����ݣ�����ָ���������е����ݽ��бȽ�
// 0  	��ʾ�ļ�У��ɹ�
// -1		��ʾ�ļ�У��ʧ��
int XM_fverify (void *stream, const void *buffer, unsigned int *verify_buffer, unsigned int length);

// ���ļ�·������ȡ�ļ����ھ�ľ���
// lpPathName		�ļ�·������\0����
// lpVolumeName	������������������ľ���
// cbVolumeName	�������ֽڳ���
// ����ֵ
// -1		����ʧ�ܻ��������
// > 0	�ѽ����ľ�������
int XM_GetVolumeNameFromFilePathName (const char *lpPathName, char* lpVolumeName, int cbVolumeName);



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_FILE_H_
