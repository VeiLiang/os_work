//****************************************************************************
//
//	Copyright (C) 2011 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: xmfile.h
//	  constant，macro, data structure，function protocol definition of file interface
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
// ******************   文件查找服务 FindFind     *******************
//
/////////////////////////////////////////////////////////////////////

// 最小文件查找缓冲区字节大小	
#define	XM_MAX_FILEFIND_NAME				0x80//14
#define	XM_STD_VIDEOFILENAME				14
#define	XM_STD_VIDEOFILENAME_SIZE		8			// 格式为 YMDhmmss
#define	XM_STD_VIDEOFILEEXT_SIZE		3			// 格式为 .MP4

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

// file attributes constant 文件属性定义
#define XM_FILE_ATTRIBUTE_READONLY        0x00000001
#define XM_FILE_ATTRIBUTE_HIDDEN          0x00000002
#define XM_FILE_ATTRIBUTE_SYSTEM          0x00000004
#define XM_FILE_ATTRIBUTE_DIRECTORY       0x00000010
#define XM_FILE_ATTRIBUTE_ARCHIVE         0x00000020
#define XM_FILE_ATTRIBUTE_NORMAL          0x00000080

// 简化版的文件查找，仅支持ASCII编码的文件系统。
typedef struct tagXMFILEFIND {
	DWORD				dwFileAttributes;		// Specifies the file attributes of the file found 文件属性. 	
	XMSYSTEMTIME	CreationTime;			// file creation time 文件创建时间
	XMSYSTEMTIME	LastWriteTime;			// 最后一次写入时间
	DWORD				nFileSize;				// Specifies the DWORD value of the file size, in bytes 文件大小.
	VOID*				Reserved;				// 私有数据，文件查找内部使用，不可以对其操作
	DWORD				magic;
	FS_FIND_DATA	FindData;
} XMFILEFIND;

// 文件查找初始化。
// lpFileName 指定查找的文件或目录名
// lpFileNameBuffer及cbFileNameBuffer定义一个缓冲，保存查找过程返回的文件或目录名
XMBOOL 	XM_FindFirstFile	(char* lpFileName, XMFILEFIND *lpFindFileData, char *lpFileNameBuffer, int cbFileNameBuffer);
// 查找下一个文件
XMBOOL 	XM_FindNextFile	(XMFILEFIND * lpFindFileData);
// 关闭文件查找
VOID 		XM_FindClose		(XMFILEFIND * lpFindFileData);

// 文件属性读取
DWORD		XM_GetFileAttributes (
  const char * lpFileName   // name of file or directory
);

// 文件属性设置
XMBOOL	XM_SetFileAttributes (
  const char * lpFileName,      // file name
  DWORD dwFileAttributes   // attributes
);

// 从系统时间得到符合DOS8.3文件名命名规范的前缀文件名（8个字符长，YMDhmmss）
// 文件名仅使用36个ASCII字符，"0123456789ABCDEFGH...Z"
// '0'表示0，字母'A'表示10，字母'Z'表示35
// Y  为基于2013年起始的时间，'0' ~ 'Z'表示2013 ~ 2048年
// M  '0' ~ 'B'   一个字符编码，表示1 ~ 12月
// D  '0' ~ 'U'   一个字符编码，表示1 ~ 31号
// h  '0' ~ 'O'   一个字符编码，表示0 ~ 23时
// mm '00' ~ '59' 两个字符编码，表示0 ~ 59分
// ss '00' ~ '59' 两个字符编码，表示0 ~ 59秒
// 成功返回1，失败返回0
int XM_SystemTime2FileName (const XMSYSTEMTIME* pSystemTime, char *lpFileName, int cbFileName);

// 从文件名提取系统时间。文件命名符合XMSYS_SystemTime2FileName的定义
int XM_FileName2SystemTime (XMSYSTEMTIME* pSystemTime, const char *lpFileName, int cbFileName);




/////////////////////////////////////////////////////////////////////
//
// ****************** 内存映射文件接口（只读访问）*******************
//
/////////////////////////////////////////////////////////////////////

#define	XM_MAPFILEID		0x4650414d	// 'MAPF'

#define	XM_SEEK_SET       0	/* offset is beginning of file  */
#define	XM_SEEK_CUR       1	/* offset is relative to current position of file pointer */
#define	XM_SEEK_END       2	/* offset is relative to end of file */


typedef struct _tagXMMAPFILE {
	DWORD	id;				// 有效映射文件的ID必须为XM_MAPFILEID
	const void *base;		// 内存映射文件基址
	LONG	size;				// 内存映射文件大小
	LONG	offset;			//	当前读指针偏移(相对基址)
} XMMAPFILE;

typedef XMBOOL	(*FPXM_MAPFILEOPEN) (const VOID *base, LONG size, XMMAPFILE *lpMapFile);
typedef LONG	(*FPXM_MAPFILEREAD) (XMMAPFILE *lpMapFile, VOID *lpBuffer, LONG cbBuffer);
typedef LONG	(*FPXM_MAPFILESEEK) (XMMAPFILE *lpMapFile, LONG offset, LONG whence);
typedef LONG	(*FPXM_MAPFILETELL) (XMMAPFILE *lpMapFile);
typedef XMBOOL	(*FPXM_MAPFILECLOSE)(XMMAPFILE *lpMapFile);


// 创建内存映射文件
XMBOOL	XM_MapFileOpen (const VOID *base, LONG size, XMMAPFILE *lpMapFile);

// 从内存映射文件当前读指针位置读取字节数为cbBuffer的内容到缓冲区lpBuffer
// 返回值 < 0    表示错误
//        = 0    表示达到文件尾部
//        其他值 表示读取的字节数
LONG		XM_MapFileRead	(XMMAPFILE *lpMapFile, VOID *lpBuffer, LONG cbBuffer);

// 将读指针定位到指定的位置
// 返回值 < 0		表示错误
//        其他值	设置前的读指针位置
LONG		XM_MapFileSeek (XMMAPFILE *lpMapFile, LONG offset, LONG whence);

// 获取内存映射文件当前的读指针
LONG		XM_MapFileTell	(XMMAPFILE *lpMapFile);

// 关闭内存映射文件
XMBOOL	XM_MapFileClose (XMMAPFILE *lpMapFile);

// 删除文件
XMBOOL	XM_RemoveFile (const char *pFileName);

// 创建文件目录
// 成功返回1，失败返回0
XMBOOL	XM_MkDir (const char *pDirName);

// 删除文件目录
// 成功返回1，失败返回0
XMBOOL	XM_RmDir (const char *pDirName);

// 获取文件的大小
// 失败返回(-1), 其他为文件的字节大小(最大值 0xFFFFFFFE)
DWORD XM_GetFileSize (const char *pFileName);

// 文件更名
// sOldName   全路径名 "mmc:0:\\VIDEO_F\\TEMP1234.AVI"
// sNewName	  文件名   "01234567.AVI"	, 表示将"mmc:0:\\VIDEO_F\\TEMP1234.AVI"改名为"mmc:0:\\VIDEO_F\\01234567.AVI"
// 0 	表示文件更名成功
// -1	表示文件更名失败
int XM_rename (const char * sOldName, const char * sNewName);



// 获取磁盘剩余空间及其他信息
XMBOOL	XM_GetDiskFreeSpace (
  const char * lpRootPathName,          // root path
  DWORD      * lpSectorsPerCluster,     // sectors per cluster
  DWORD      * lpBytesPerSector,        // bytes per sector
  DWORD      * lpNumberOfFreeClusters,  // free clusters
  DWORD      * lpTotalNumberOfClusters, // total clusters
  const char * lpVolumeName             // volume name
);

// 检查指定设备的文件系统是否已Mount
XMBOOL	XM_IsVolumeMounted (int dev);

void *XM_fopen (const char *file, const char *mode);
int XM_fseek( void *stream, long offset, int origin );
unsigned int XM_fread( void *buffer, unsigned int size, unsigned int count, void *stream );
unsigned int XM_fwrite ( void *buffer, unsigned int size, unsigned int count, void *stream );
// Native接口，流文件关闭
int XM_fclose( void *stream );
// Native接口，获取流文件字节长度
unsigned int XM_filesize ( void *stream );

// 文件回读校验
// 从文件当前位置读取指定字节长度的数据，并与指定缓冲区中的数据进行比较
// 0  	表示文件校验成功
// -1		表示文件校验失败
int XM_fverify (void *stream, const void *buffer, unsigned int *verify_buffer, unsigned int length);

// 从文件路径名获取文件所在卷的卷名
// lpPathName		文件路径名，\0结束
// lpVolumeName	串缓冲区，保存解析的卷名
// cbVolumeName	卷缓冲区字节长度
// 返回值
// -1		解析失败或参数错误
// > 0	已解析的卷名长度
int XM_GetVolumeNameFromFilePathName (const char *lpPathName, char* lpVolumeName, int cbVolumeName);



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_FILE_H_
