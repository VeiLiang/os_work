#include <xm_proj_define.h>
#include <stdio.h>
#include <assert.h>
#include <xm_type.h>
#include <xm_base.h>
#include <xm_queue.h>
#include <xm_file.h>
#include "xm_h264_cache_file.h"
#include "xm_h264_cache_command_queue.h"
#include "xm_h264_file.h"
#include "xm_semaphore.h"
#include "xm_printf.h"
#include "xm_power.h"

#define	HAVE_H264_CACHE



#define	H264_FILE_COUNT		16		// H264�ļ�����
#define	H264_FILE_TYPE_R		0
#define	H264_FILE_TYPE_W		1

#define	H264_FILE_ID		0x46343632

typedef struct _H264_FILE {
	void 	*	prev;
	void	*	next;

	char	filename[64];	// �ļ���
	
	int		id;			// ��ʾ
	int		type;			// 0 ��ͨ�ļ�  1  Cache�ļ�
	void *	handle;		// �ļ����
} H264_FILE;

static H264_FILE  h264file_unit[H264_FILE_COUNT];
static queue_s		h264file_free;		// �����ļ�����
static XM_RSEMA 	h264file_sema;		// ��������ź���, ����H264�ļ�ϵͳ�ڲ�����



void xm_h264_file_init (void)
{
	int i;
	queue_initialize (&h264file_free);
	memset (h264file_unit, 0, sizeof(h264file_unit));
	for (i = 0; i < H264_FILE_COUNT; i++)
	{
		queue_insert ((queue_s *)&h264file_unit[i], &h264file_free);
	}	
	SEMA_CREATE(&h264file_sema); /* Creates resource semaphore */
}

void *h264_fopen (const char *filename, const char *mode)
{
#ifdef HAVE_H264_CACHE
	int type = H264_FILE_TYPE_R;
	H264_FILE *file;
	
	XM_printf ("h264_fopen %s, %s\n", filename, mode);
	
	if(filename == NULL || *filename == 0)
		return NULL;
	if(mode == NULL || *mode == 0)
		return NULL;
	
	if(!strcmp (mode, "r") || !strcmp (mode, "rb"))
	{
		type = H264_FILE_TYPE_R;
	}
	else if(!strcmp (mode, "w") || !strcmp (mode, "wb"))
	{
		// ����ⲿACC��Դ״��
		// ACC����ʱ���������ļ�����
		if(xm_power_check_acc_safe_or_not() == 0)
		{
			// �ǰ�ȫ��ѹ
			XM_printf ("h264_fopen failed, bad acc\n");
			return (NULL);		
		}
		
		type = H264_FILE_TYPE_W;
	}
	else
	{
		XM_printf ("h264_fopen Unsupport mode(%s)\n", mode);
		return NULL;
	}
	
	SEMA_LOCK (&h264file_sema); /* Make sure nobody else uses */
	if(queue_empty (&h264file_free))
	{
		SEMA_UNLOCK (&h264file_sema);
		H264_FATAL ("H264_FILE CreateH264File failed\n");
		return NULL;
	}
	file = (H264_FILE *)queue_delete_next(&h264file_free);
	file->type = type;
	strcpy (file->filename, filename);
	SEMA_UNLOCK (&h264file_sema);
	if(type == H264_FILE_TYPE_R)
	{
		// ����
		file->handle = XM_fopen(filename, "rb");
	}
	else
	{
		// ��д
		file->handle = H264CacheFileOpen (filename);
	}
	
	if(file->handle == NULL)
	{
		SEMA_LOCK (&h264file_sema); /* Make sure nobody else uses */
		queue_insert ((queue_s *)file, &h264file_free);
		SEMA_UNLOCK (&h264file_sema);
		XM_printf ("h264_fopen failed\n");
		return NULL;
	}

	file->id = H264_FILE_ID;
	
	//XM_printf ("h264_fopen %s, %s, %x success\n", filename, mode, file);
	
	return file;
	
#else
	//seek_times = 0;
	return XM_fopen (filename, mode);
#endif
}

int h264_fclose (void *stream)
{
	int ret;
#ifdef HAVE_H264_CACHE
	H264_FILE *file = stream;
	
	//XM_printf ("h264_fclose %x\n", file);
	
	if(file == NULL)
		return -3;
	if(file->id != H264_FILE_ID)
	{
		H264_FATAL ("h264_fclose invalid id\n");
		return -3;
	}
	
	if(file->type == H264_FILE_TYPE_R)
	{
		ret = XM_fclose (file->handle);
	}
	else
	{
		ret = H264CacheFileClose (file->handle);
	}
	SEMA_LOCK (&h264file_sema); /* Make sure nobody else uses */
	file->handle = NULL;
	file->id = 0;
	queue_insert ((queue_s *)file, &h264file_free);
	SEMA_UNLOCK (&h264file_sema);
	
#else
	//XM_printf ("seek_times=%d\n", seek_times);
	//seek_times = 0;
	ret = XM_fclose (stream);
	FS_CACHE_Clean ("");
#endif
	
	return ret;
}


int h264_fseek (void *stream, long offset, int mode)
{
#ifdef HAVE_H264_CACHE
	H264_FILE *file = stream;
	int ret;	

	//XM_printf ("h264_fseek(%x) offset=%d, mode=%d\n", stream, offset, mode);

	if(file == NULL)
		return -3;
	if(file->id != H264_FILE_ID)
	{
		H264_FATAL ("h264_fclose invalid id\n");
		return -3;
	}
	
	if(file->type == H264_FILE_TYPE_R)
	{
		ret = XM_fseek (file->handle, offset, mode);
	}
	else
	{
		ret = (int)H264CacheFileSeek (file->handle, offset, mode);
	}
	return ret;
#else
	//seek_times ++;
	//XM_printf ("h264_fseek offset=%d, mode=%d\n", offset, mode);
	return XM_fseek (stream, offset, mode);
	
#endif
}

size_t h264_fread (void *ptr, size_t size, size_t nelem, void *stream)
{
#ifdef HAVE_H264_CACHE
	
	H264_FILE *file = stream;
	int ret;
	if(file == NULL)
		return (size_t)-3;
	if(file->id != H264_FILE_ID)
	{
		H264_FATAL ("h264_fclose invalid id\n");
		return (size_t)-3;
	}
	
	if(file->type == H264_FILE_TYPE_R)
	{
		ret = XM_fread (ptr, size, nelem, file->handle);
	}
	else
	{
		ret = H264CacheFileRead (ptr, size, nelem, file->handle);
	}	
	
	return ret;
#else
	return XM_fread (ptr, size, nelem, stream);
#endif
}

size_t h264_fwrite (void *ptr, size_t size, size_t nelem, void *stream)
{
	// ����ACC�µ�ʱ������������д��
#if 0
	// ����ⲿACC��Դ״��
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// �ǰ�ȫ��ѹ
		return (size_t)(-3);		
	}
#endif
	
#ifdef HAVE_H264_CACHE
	H264_FILE *file = stream;
	int ret;
	if(file == NULL)
		return (size_t)(-3);
	if(file->id != H264_FILE_ID)
	{
		H264_FATAL ("h264_fclose invalid id\n");
		return (size_t)(-3);
	}
	
	if(file->type == H264_FILE_TYPE_R)
	{
		ret = XM_fwrite (ptr, size, nelem, file->handle);
	}
	else
	{
		ret = H264CacheFileWrite (ptr, size, nelem, file->handle);
	}
	return ret;
#else
	int ret = XM_fwrite (ptr, size, nelem, stream);
	//XM_printf ("FS_FWrite size=%d, ret=%d\n", size * nelem, ret);
	return ret;
#endif
}

int h264_filelength (void *stream)
{
#ifdef HAVE_H264_CACHE
	H264_FILE *file = stream;
	int ret;
	if(file == NULL)
		return -3;
	if(file->id != H264_FILE_ID)
	{
		H264_FATAL ("h264_fclose invalid id\n");
		return -3;
	}
	
	if(file->type == H264_FILE_TYPE_R)
	{
		ret = XM_filesize (file->handle);
	}
	else
	{
		ret = XM_filesize (file->handle);
	}
	//XM_printf ("h264_filelength=%d\n", ret);
	return ret;
#else
	int ret = XM_filesize (stream);
	return ret;
#endif
	
}

