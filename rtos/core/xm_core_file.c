#include <xm_proj_define.h>
#include <stdio.h>
#include <assert.h>
#include <xm_type.h>
#include <xm_file.h>
#include "xm_power.h"
#include "xm_printf.h"
#include "FS.h"
#include "rtos.h"
#include "common.h"

#ifdef WIN32
#include <io.h>
#endif

void *XM_fopen (const char *file, const char *mode)
{
	return FS_FOpen (file, mode);
}

int XM_fseek( void *stream, long offset, int origin )
{
	return FS_FSeek (stream, offset, origin);
}

unsigned int XM_fread( void *buffer, unsigned int size, unsigned int count, void *stream )
{
	return FS_FRead (buffer, size, count, stream);
}

unsigned int XM_fwrite ( void *buffer, unsigned int size, unsigned int count, void *stream )
{
	// ����ACC����ʱ�ļ�д��, ������ȷ�ر�
#if 0
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		printf ("XM_fwrite failed, bad acc\n"); 
		return 0;
	}
#endif
	
	return FS_FWrite (buffer, size, count, stream);
}

// Native�ӿڣ����ļ��ر�
int XM_fclose( void *stream )
{
	return FS_FClose (stream);
}

int XM_ftell ( void *stream )
{
	return FS_FTell (stream);
}

unsigned int XM_filesize (void *stream)
{
	return FS_GetFileSize (stream);
}

// ���ļ�ˢ��
int XM_fflush( void *stream )
{
	return 0;
}

// ��ȡ����ǰλ��
// If successful, XM_fgetpos returns 0. On failure, it returns a negative nonzero value.
int XM_fgetpos(void *stream, i64_t *pos)
{
	int ret = FS_GetFilePos (stream);
	if(ret == (-1))	// In case of any error the return value is -1.
		return -1;
	if(pos == NULL)
		return -1;
	*pos = ret;
	return 0;
}

int XM_fsetpos(void *stream, const i64_t *pos)
{
	int off, ret;
	
	if(pos == NULL || stream == NULL)
		return -1;
	off = (int)*pos;
	ret = FS_SetFilePos (stream, off, FS_FILE_BEGIN);
	if(ret < 0)
		return -1;
	return 0;
}

char *  XM_fgets(char *string, int n, void *stream )
{
	char ch;
	char *s = string;
	int size = 0;
	while(n > 0)
	{
		if(FS_Read (stream, &ch, 1) == 0)
		{
			*string = '\0';
			break;
		}
		else if(ch == 0x0A)
		{
			*string = 0x0A;
			string ++;
			n --;
			size ++;
			break;
		}
		*string = (char)ch;
		string ++;
		size ++;
		n --;
	}

	if(n > 0)
		*string = '\0';

	if(size)
		return s;
	else
		return NULL;
}

// �ļ��ض�У��
// ���ļ���ǰλ�ö�ȡָ���ֽڳ��ȵ����ݣ�����ָ���������е����ݽ��бȽ�
// 0  	��ʾ�ļ�У��ɹ�
// -1		��ʾ�ļ�У��ʧ��
int XM_fverify (void *stream, const void *buffer, unsigned int *verify_buffer, unsigned int length)
{
	if(FS_FRead (verify_buffer, 1, length, stream) != length)
		return 1;
	if(memcmp (buffer, verify_buffer, length) == 0)
		return 0;
	else
		return 1;
	//return FS_Verify (stream, buffer, length);	
}

// �ļ�����
// sOldName   ȫ·���� "mmc:0:\\VIDEO_F\\TEMP1234.AVI"
// sNewName	  �ļ���   "01234567.AVI"	, ��ʾ��"mmc:0:\\VIDEO_F\\TEMP1234.AVI"����Ϊ"mmc:0:\\VIDEO_F\\01234567.AVI"
// 0 	��ʾ�ļ������ɹ�
// -1	��ʾ�ļ�����ʧ��
int XM_rename (const char * sOldName, const char * sNewName)
{
	// ����TEMP�ļ���ACC�µ�ǰ����
#if 0
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		printf ("rename failed, bad acc\n"); 
		return -1;
	}
#endif
	return FS_Rename (sOldName, sNewName);
}

void XM_Delay (unsigned int ms)
{
	OS_Delay (ms);
}

extern 	void *os_fopen (const char *filename, const char *mode);
extern 	int os_fclose (void *stream);
extern int os_fseek (void *stream, long offset, int mode);
extern 	size_t os_fread (void *ptr, size_t size, size_t nelem, void *stream);
extern 	size_t os_fwrite (void *ptr, size_t size, size_t nelem, void *stream);

void *os_fopen (const char *filename, const char *mode)
{
	return FS_FOpen (filename, mode);
}

int os_fclose (void *stream)
{
	return FS_FClose (stream);
}

int os_fseek (void *stream, long offset, int mode)
{
	return FS_FSeek (stream, offset, mode);
}

size_t os_fread (void *ptr, size_t size, size_t nelem, void *stream)
{
	return FS_FRead (ptr, size, nelem, stream);
}

size_t os_fwrite (void *ptr, size_t size, size_t nelem, void *stream)
{
	// ����ACC����ʱ�ļ�д��, ������ȷ�ر�
#if 0
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		printf ("os_fwrite failed, bad acc\n"); 
		return 0;
	}
#endif
	return FS_FWrite (ptr, size, nelem, stream);
}

// ���ļ�·������ȡ�ļ����ھ�ľ���
// lpPathName		�ļ�·������\0����
// lpVolumeName	������������������ľ���
// cbVolumeName	�������ֽڳ���
// ����ֵ
// -1		����ʧ�ܻ��������
// > 0	�ѽ����ľ�������
int XM_GetVolumeNameFromFilePathName (const char *lpPathName, char* lpVolumeName, int cbVolumeName)
{
	char volume[64];
	char *split_char;
	int ret;
	if(lpVolumeName == NULL || cbVolumeName == 0)
		return -1;
	memset (volume, 0, sizeof(volume));
	if(lpPathName == NULL || *lpPathName == '\0')
	{
#if defined(_XM_FILE_SYSTEM_WIN32_SIMULATE_)
		// Pointer to a null-terminated string that specifies the root directory of the disk to return information about.
		//  For example, you would specify "\\\\MyServer\\MyShare" as "\\\\MyServer\\MyShare\\"
		strcpy (volume, "g:\\");
#else
		// Valid values for sVolume have the following structure:
		//		[DevName:[UnitNum:]]
		strcpy (volume, "mmc:");
#endif
	}
	else
	{
		// "mmc:0:\\F_VIDEO\\"
		// "g:\\F_VIDEO\\"
		strcpy (volume, lpPathName);
		split_char = strchr (volume, '\\');
		if(split_char == NULL)
		{
			XM_printf ("Path Name(%s) invalid\n", volume);
			return -1;
		}
#if defined(_XM_FILE_SYSTEM_WIN32_SIMULATE_)
		// Pointer to a null-terminated string that specifies the root directory of the disk to return information about.
		//  For example, "G:\\"
		*(split_char + 1) = '\0';
#else
		// Valid values for sVolume have the following structure:
		//		[DevName:[UnitNum:]]
		*split_char = '\0';
#endif
	}
	ret = strlen (volume);
	if(ret >= cbVolumeName)
	{
		XM_printf ("volume buffer too small (%d) to save volume name(%s)\n", cbVolumeName, volume);
		return -1;
	}
	strcpy (lpVolumeName, volume);
	return ret;
}

// 0123456789
// abcdefghij
// klmnopqrst
// uvwxyz

static const char filename_codes[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V', 'W', 'X', 'Y', 'Z', '_', '\0'
};

// ��ϵͳʱ��õ�����DOS8.3�ļ��������淶��ǰ׺�ļ�����8���ַ�����YMDhmmss��
// �ļ�����ʹ��36��ASCII�ַ���"0123456789ABCDEFGH...Z"
// '0'��ʾ0����ĸ'A'��ʾ10����ĸ'Z'��ʾ35
// Y  Ϊ����2013����ʼ��ʱ�䣬'0' ~ 'Z'��ʾ2013 ~ 2048��
// M  '0' ~ 'B'   һ���ַ����룬��ʾ1 ~ 12��
// D  '0' ~ 'U'   һ���ַ����룬��ʾ1 ~ 31��
// h  '0' ~ 'O'   һ���ַ����룬��ʾ0 ~ 23ʱ
// mm '00' ~ '59' �����ַ����룬��ʾ0 ~ 59��
// ss '00' ~ '59' �����ַ����룬��ʾ0 ~ 59��
int XM_SystemTime2FileName (const XMSYSTEMTIME* pSystemTime, char *lpFileName, int cbFileName)
{
	char *ch = lpFileName;
	int Y, M, D, h, mm, ss;
	if(cbFileName < 8)
		return 0;
	if(pSystemTime == NULL || lpFileName == NULL)
		return 0;
	
	Y = pSystemTime->wYear;
	if(Y < 2013)
		Y = 2013;
	else if(Y > 2048)
		Y = 2048;
	*ch ++ = filename_codes[Y - 2013];
	
	M = pSystemTime->bMonth;	// 1 ~ 12
	if(M < 1)
		M = 1;
	if(M > 12)
		M = 12;
	*ch ++ = filename_codes[M - 1];
	
	D = pSystemTime->bDay;	// 1 ~ 31
	if(D < 1)
		D = 1;
	else if(D > 31)
		D = 31;
	*ch ++ = filename_codes[D - 1];
	
	h = pSystemTime->bHour;	// 0 ~ 23
	if(h < 0)
		h = 0;
	else if(h > 23)
		h = 23;
	*ch ++ = filename_codes[h];
	
	mm = pSystemTime->bMinute;	// 0 ~ 59
	if(mm < 0)
		mm = 0;
	else if(mm > 59)
		mm = 59;
	*ch ++ = filename_codes[mm / 10];
	*ch ++ = filename_codes[mm % 10];
	
	ss = pSystemTime->bSecond;	// 0 ~ 59
	if(ss < 0)
		ss = 0;
	else if(ss > 59)
		ss = 59;
	*ch ++ = filename_codes[ss / 10];
	*ch    = filename_codes[ss % 10];
	
	return 1;
}

static int Char2Code (int c)
{
	if(c >= '0' && c <= '9')
		return c - '0';
	else if(c >= 'A' && c <= 'Z')
		return 10 + c - 'A';
	else
		return (-1);	// ��Ч�ַ�
}

// ���ļ�����ȡϵͳʱ�䡣�ļ���������XMSYS_SystemTime2FileName�Ķ���
int XM_FileName2SystemTime (XMSYSTEMTIME* pSystemTime, const char *lpFileName, int cbFileName)
{
	int Y, M, D, h, mm, ss;
	const char *ch;
	if(pSystemTime == NULL || lpFileName == NULL || cbFileName < 8)
		return 0;
	
	ch = lpFileName;
	Y = Char2Code(*ch);
	// �����Ч�ַ�
	if(Y < 0)
		return 0;
	Y += 2013;
	if(Y > 2048)
		return 0;
	pSystemTime->wYear = (WORD)Y;
	ch ++;
	
	M = Char2Code(*ch);
	if(M < 0)
		return 0;
	M += 1;
	if(M > 12)
		return 0;
	pSystemTime->bMonth = (BYTE)M;	// 1 ~ 12
	ch ++;
	
	D = Char2Code(*ch);
	if(D < 0)
		return 0;
	D += 1;
	if(D > 31)
		return 0;
	pSystemTime->bDay = (BYTE)D;
	ch ++;
	
	h = Char2Code(*ch);
	if(h < 0)
		return 0;
	if(h > 23)
		return 0;
	pSystemTime->bHour = (BYTE)h;
	ch ++;
	
	mm = Char2Code(*ch);
	if(mm < 0)
		return 0;
	if(mm > 5)
		return 0;
	pSystemTime->bMinute = (BYTE)(mm *  10);
	ch ++;
	mm = Char2Code(*ch);
	if(mm < 0)
		return 0;
	if(mm > 9)
		return 0;
	pSystemTime->bMinute = (BYTE)(pSystemTime->bMinute + mm);
	if(pSystemTime->bMinute > 59)
		return 0;
	ch ++;
	
	ss = Char2Code(*ch);
	if(ss < 0)
		return 0;
	if(ss > 5)
		return 0;
	pSystemTime->bSecond = (BYTE)(ss *  10);
	ch ++;
	ss = Char2Code(*ch);
	if(ss < 0)
		return 0;
	if(ss > 9)
		return 0;
	pSystemTime->bSecond = (BYTE)(pSystemTime->bSecond + ss);
	if(pSystemTime->bSecond > 59)
		return 0;
	
	return 1;
}

int check_and_create_directory (char *path)
{
	XMFILEFIND fileFind;
	char fileName[XM_MAX_FILEFIND_NAME];
	char dir_name[XM_MAX_FILEFIND_NAME];
	char *root;
	char *ch;
	int ret = 0;
	int count = 0;
	int len;
	if(path == NULL || *path == '\0')
		return -1;
	len = strlen(path);
	if(len >= XM_MAX_FILEFIND_NAME)
		return -1;
	
	//printf ("check_and_create_directory %s\n", path);
	while(1)
	{
		// ����Ŀ¼���Ƿ����
		if(XM_FindFirstFile (path, &fileFind, fileName, XM_MAX_FILEFIND_NAME))
		{
			// �ҵ�ƥ���Ŀ¼��
			ret = 0;
			XM_FindClose (&fileFind);
			break;
		}
		else
		{
			// δ�ҵ�ƥ���Ŀ¼��
			
			// ����Ŀ¼��
			// ɾ������'\\'�ַ�
			if(FS_IsVolumeMounted(path) == 0)
			{
				return -1;
			}

			strcpy (dir_name, path);
			root = strchr (dir_name, '\\');
			if(root == NULL)
			{
				XM_printf ("illegal directory name (%s)\n", dir_name);
				return -1;
			}
			
			// ɾ��β����'\\'
			if(dir_name[len - 1] == '\\')
			{
				 if( (dir_name + len - 1) == root )
				 {
					 // ���ҵ���Ŀ¼
					 return 0;
				 }
				 else
				 {
					 // ���Ǹ�·���ַ�, ����ɾ��
					 dir_name[len - 1] = '\0';
				 }
			}
				
			if(!XM_MkDir (dir_name))
			{
				// Ŀ¼����ʧ��
				// �����һ���ļ�·�����Ƿ����
				char motherdir_name[XM_MAX_FILEFIND_NAME];
				memset (motherdir_name , 0 , sizeof(motherdir_name));
				ch = strrchr (dir_name, '\\');
				if(ch)
				{
					// ��һ���ļ�·��������
					// ����'\\'�ַ�
					memcpy (motherdir_name, dir_name, ch - dir_name + 1);
					// �����һ��·���Ƿ��Ǹ�Ŀ¼, ����, �˳�
					root = strchr (motherdir_name, '\\');
					ch = strrchr (motherdir_name, '\\');
					if(root && root == ch)
					{
						return -1;
					}
					ret = check_and_create_directory ( motherdir_name );
					if(ret < 0)
						return -1;
				}
				else
				{
					return -1;
				}
			}	
			else
			{
				// Ŀ¼�����ɹ�
				ret = 0;
				break;
			}
		}
	}
	return ret;
}

#include "xm_h264_file.h"

#define O_ACCMODE	   0003
#define O_RDONLY	     00
#define O_WRONLY	     01
#define O_RDWR		     02
#define O_CREAT		   0100	/* not fcntl */
#define O_EXCL		   0200	/* not fcntl */
#define O_NOCTTY	   0400	/* not fcntl */
#define O_TRUNC		  01000	/* not fcntl */
#define O_APPEND	  02000
#define O_NONBLOCK	  04000
#define O_NDELAY	O_NONBLOCK
#define O_SYNC		 010000
#define O_FSYNC		 O_SYNC
#define O_ASYNC		 020000

int open64(const char *pathname, int oflag,...)
{
	void *fp;
	int fd;
	if(pathname == NULL)
		return (-3);
	if((oflag & O_ACCMODE) == O_RDONLY)
	{
		// ��ģʽ
		//printf ("read mode\n");
		fp = h264_fopen (pathname, "r");
	}
	else if((oflag & O_ACCMODE) == O_WRONLY)
	{
		// дģʽ
		//printf ("write mode\n");
		// ����ⲿACC��Դ״��
		// �ļ�������ACC�µ�ʱ������
		if(xm_power_check_acc_safe_or_not() == 0)
		{
			// �ǰ�ȫ��ѹ
			return (-3);		
		}
		
		if(oflag & O_APPEND)
		{
			printf ("O_APPEND don't support\n");
			return (-3);
		}
		fp = h264_fopen (pathname, "w");
	}
	else // if((oflag & O_RDWR) == O_RDWR)
	{
		printf ("O_RDWR don't support\n");
		return (-3);
	}
	
	if(fp == NULL)
	{
		printf ("open64 %s oflag=%08x failed\n", pathname, oflag);
		return (-3);
	}
	
	fd = (int)( ((unsigned int)fp) - 0x80000000 );
	//printf ("open64 %s success %x\n", pathname, fd);
	
	return fd;
}

int close (int handle)
{
	void *fp;
	if(handle < 0)
		return (-3);
	fp = (void *)(0x80000000 + handle);
	return h264_fclose (fp);
}

int filelength (int handle)
{
	void *fp;
	if(handle < 0)
		return (off64_t)(-3);
	fp = (void *)(0x80000000 + handle);
	
	return h264_filelength (fp);
	
}

#define	R_OK	4		/* Test for read permission.  */
#define	W_OK	2		/* Test for write permission.  */
#define	X_OK	1		/* Test for execute permission.  */
#define	F_OK	0		/* Test for existence.  */ 

int access( const char *path, int mode )
{
	U8 attr = FS_GetFileAttributes (path);
	if(attr == 0xFF)	// In case of any error.
		return -3;
	if(attr & FS_ATTR_DIRECTORY)
	{
		XM_printf ("access NG, %s is directory\n", path);
		return -3;
	}
	if(mode == X_OK)
	{
		XM_printf ("access NG, %s can't execute\n", path);
		return -3;		
	}
	if(mode == F_OK || mode == W_OK || mode == R_OK)
	{
		return 0;
	}
	return -3;
}

off64_t lseek64(int handle, off64_t offset, int whence)
{
	void *fp;
	if(handle < 0)
		return (off64_t)(-3);
	fp = (void *)(0x80000000 + handle);
	return (off64_t)h264_fseek (fp, (long)offset, whence);
}

int read( int handle, void *buf, unsigned int count )
{
	void *fp;
	int ret;
	unsigned char *buffer = buf;
	if(handle < 0)
		return (-3);
	fp = (void *)(0x80000000 + handle);
	
	ret = h264_fread (buffer, 1, count, fp);
	/*printf ("read count=%d, ret=%d, fp=%x, %x, %x, %x, %x, %x, %x, %x, %x\n",
			  count, ret, fp,
			  buffer[0], buffer[1], buffer[2], buffer[3],
			  buffer[0x10], buffer[0x11], buffer[0x12], buffer[0x13]);*/
	return ret;
}

int write ( int handle, void *buffer, unsigned int count )
{
	void *fp;
	if(handle < 0)
		return (-3);

	// �����ļ�����ACC�µ��ǽ�ʣ������д��
#if 0
	// ����ⲿACC��Դ״��
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// �ǰ�ȫ��ѹ
		return (-3);		
	}
#endif

	fp = (void *)(0x80000000 + handle);
	//printf ("write count=%d, buffer=%x, fp=%x\n",
	//		  count, buffer, fp);
	
	return h264_fwrite (buffer, 1, count, fp);
}
