//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_recycle.c
//	  ����վ����
//
//	Revision history
//
//		2012.09.22	ZhuoYongHong Initial version
//
//****************************************************************************
#include <string.h>
#include <stdlib.h>
#include "rtos.h"
#include <xm_user.h>
#include <stdio.h>
#include <xm_printf.h>
#include <xm_videoitem.h>
#include "xm_recycle.h"
#include "xm_core.h"

extern int XMSYS_file_system_block_write_access_lock (void);
extern void XMSYS_file_system_block_write_access_unlock (void);

static unsigned int recycle_item_count = 0;		// ����������
static XM_RECYCLEITEM *recycle_item_pool;			// ������

// �ж��Ƿ��Ƿ��ϱ�׼��������Ƭ�ļ�
int is_valid_image_filename (char *file_name)
{
	char *ch;
	int name_count = 0;
		
	ch = file_name;
	while(*ch)
	{
		// �ļ���������ֻ��������, ��ĸ���»���
		if(*ch >= '0' && *ch <= '9' || *ch >= 'A' && *ch <= 'Z' || *ch == '_')
		{
			name_count ++;
			if(name_count > XM_STD_VIDEOFILENAME_SIZE)
				return 0;
		}
		else if(*ch == '.')
		{
			// ����Ƿ��Ǳ�׼�����ļ���
			if(name_count != XM_STD_VIDEOFILENAME_SIZE)
				return 0;
			ch ++;
			break;
		}
		else
		{
			// �Ǳ�׼�����ַ�
			return 0;
		}
		ch ++;
	}
	
	if(name_count != XM_STD_VIDEOFILENAME_SIZE)
		return 0;

	if(	(ch[0] == 'j' || ch[0] == 'J')
		&&	(ch[1] == 'p' || ch[1] == 'P')
		&&	(ch[2] == 'g' || ch[2] == 'G')
		&& (ch[3] == 0) )
	{
		// ��Ч���ļ���
		return 1;
	}
	else
		return 0;
}

// �ж��Ƿ��Ƿ��ϱ�׼��������Ƶ�ļ�
int is_valid_video_filename (char *file_name)
{
	char *ch;
	int name_count = 0;
		
	ch = file_name;
	while(*ch)
	{
		// �ļ���������ֻ��������
		if(*ch >= '0' && *ch <= '9' || *ch >= 'A' && *ch <= 'Z'|| *ch == '_')
		{
			name_count ++;
			if(name_count > XM_STD_VIDEOFILENAME_SIZE)
				return 0;
		}
		else if(*ch == '.')
		{
			// ����Ƿ��Ǳ�׼�����ļ���
			if(name_count != XM_STD_VIDEOFILENAME_SIZE)
				return 0;
			ch ++;
			break;
		}
		else
		{
			// �Ǳ�׼�����ַ�
			return 0;
		}
		ch ++;
	}
	
	if(name_count != XM_STD_VIDEOFILENAME_SIZE)
		return 0;

	if(	(ch[0] == 'a' || ch[0] == 'A')
		&&	(ch[1] == 'v' || ch[1] == 'V')
		&&	(ch[2] == 'i' || ch[2] == 'I')
		&& (ch[3] == 0) )
	{
		// ��Ч���ļ���
		return 1;
	}
	else
		return 0;
}

// ��ȡ�������ȫ·���ļ���
int get_recycle_item_name (XM_RECYCLEITEM *item, char *name)
{
	const char *video_path = XM_VideoItemGetVideoPath (item->channel);
	if(video_path == NULL)
		return -1;
	sprintf (name, "%s\\RECYCLE\\%s.%s", 
			video_path,
			item->item_name, 
			(item->file_type == XM_FILE_TYPE_VIDEO) ? XM_VIDEO_FILE_EXT : XM_PHOTO_FILE_EXT
		);	
	return 0;
}

// ��ȡ��ԭ�ļ���ȫ·���ļ���
static int get_restore_item_name (XM_RECYCLEITEM *item, char *name)
{
	const char *video_path = XM_VideoItemGetVideoPath (item->channel);
	if(video_path == NULL)
		return -1;
	sprintf (name, "%s\\%s.%s", video_path, 
		item->item_name, 
		(item->file_type == XM_FILE_TYPE_VIDEO) ? XM_VIDEO_FILE_EXT : XM_PHOTO_FILE_EXT);
	return 0;
}

static unsigned int recycle_enum_item (unsigned int channel, unsigned int file_type, XM_RECYCLEITEM *item, unsigned int item_count)
{
	char ImagePath[64];
	char fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	XMFILEFIND fileFind;
	unsigned int count = 0;
	const char *video_path;
	
	if(channel >= XM_VIDEO_CHANNEL_COUNT)
		return 0;
	if(file_type >= XM_FILE_TYPE_COUNT)
		return 0;
	if(item_count == 0)
		return 0;

	video_path = XM_VideoItemGetVideoPath (channel);
	if(video_path == NULL)
		return 0;

#ifdef _WINDOWS
	sprintf (ImagePath, "%s\\RECYCLE\\*.*", video_path);
#else
	sprintf (ImagePath, "%s\\RECYCLE", video_path);
#endif
	
	if(XM_FindFirstFile (ImagePath, &fileFind, fileName, XM_MAX_FILEFIND_NAME))
	{
		do
		{
			// ���Ŀ¼������
			DWORD dwFileAttributes = fileFind.dwFileAttributes;
			// ����Ŀ¼���Ƿ����ļ�����
			if((dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == 0x00)
			{
				// ��ͨ�ļ�
				if(	file_type == XM_FILE_TYPE_PHOTO && is_valid_image_filename (fileName)
					||	file_type == XM_FILE_TYPE_VIDEO && is_valid_video_filename (fileName)
					)
				{
					item->channel = (unsigned char)channel;
					item->file_type = (unsigned char)file_type;
#if LFN_SUPPORT
					// 2018_0430_1234_50
					memcpy (item->item_name, fileName, VIDEOITEM_LFN_SIZE);
					item->item_name[VIDEOITEM_LFN_SIZE] = 0;					
#else
					memcpy (item->item_name, fileName, 8);
					item->item_name[8] = 0;
#endif

					count ++;		// �ۼ�item������
					item ++;
					item_count --;
				}
			}

			if(item_count == 0)
				break;

		} while(XM_FindNextFile (&fileFind));
		XM_FindClose (&fileFind);
	}
	else
	{
		return 0;
	}

	return count;
}

// ��ȡ����վ��Ŀ�ĸ���
// channel  ͨ��, 0, 1
// file_type �ļ�����(��Ƶ����Ƭ)
// ����ֵ
// 0 ��ʾ�޻��յ���Ŀ
// > 0 ��ʾ�ɻ��յ���Ŀ����
static unsigned int _RecycleGetItemCount (unsigned int channel, unsigned int file_type)
{
	char ImagePath[64];
	char fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	XMFILEFIND fileFind;
	unsigned int item_count = 0;
	const char *video_path;
	
	if(channel >= XM_VIDEO_CHANNEL_COUNT)
		return 0;
	if(file_type >= XM_FILE_TYPE_COUNT)
		return 0;

	video_path = XM_VideoItemGetVideoPath (channel);
	if(video_path == NULL)
		return 0;

#ifdef _WINDOWS
	sprintf (ImagePath, "%s\\RECYCLE\\*.*", video_path);
#else
	sprintf (ImagePath, "%s\\RECYCLE", video_path);
#endif
	
	if(XM_FindFirstFile (ImagePath, &fileFind, fileName, XM_MAX_FILEFIND_NAME))
	{
		do
		{
			// ���Ŀ¼������
			DWORD dwFileAttributes = fileFind.dwFileAttributes;
			// ����Ŀ¼���Ƿ����ļ�����
			if((dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == 0x00)
			{
				// ��ͨ�ļ�
				if(file_type == XM_FILE_TYPE_PHOTO && is_valid_image_filename (fileName))
					item_count ++;
				else if(file_type == XM_FILE_TYPE_VIDEO && is_valid_video_filename (fileName))
					item_count ++;
			}
		} while(XM_FindNextFile (&fileFind));
		XM_FindClose (&fileFind);
	}
	else
	{
		return 0;
	}
	return item_count;
}

// ��ȡ��ǰ�ɻ�����Ŀ������
unsigned int XM_RecycleGetItemCount (void)
{
	return recycle_item_count;
}

// ��ȡָ��������ʶ�Ļ�����
XM_RECYCLEITEM* XM_RecycleGetItem (unsigned int index)
{
	if(recycle_item_pool == NULL)
		return NULL;
	if(index >= recycle_item_count)
	{
		XM_printf ("the index(%d) of recycle item invalid\n", index);
		return NULL;
	}
	return recycle_item_pool + index;
}

// ���ҿ��õĻ�ԭ�ļ���, ȷ���û�ԭ�ļ�û�б�ռ��
static int XM_RecycleGetRestoreFileName (XM_RECYCLEITEM *item, char *lpFileName, unsigned int cbFileName)
{
	XMSYSTEMTIME	CreationTime;
	XMFILEFIND FindFileData;
	char FileNameBuffer[XM_MAX_FILEFIND_NAME];
	char FullFileName[XM_MAX_FILEFIND_NAME];
	int loop;
	int ret = 0;

	XM_RECYCLEITEM restoreItem;

	memcpy (&restoreItem, item, sizeof(XM_RECYCLEITEM));

	// ��ȡ�������ļ����ļ�����ʱ��
	XM_GetDateTimeFromFileName (restoreItem.item_name, &CreationTime);

	memset (FullFileName, 0, sizeof(FullFileName));

	loop = 0;
	while(loop < 60)
	{
		// �Ӵ���ʱ�����ļ���
		XM_MakeFileNameFromCurrentDateTime (&CreationTime, restoreItem.item_name, sizeof(restoreItem.item_name));

		// ��ȡ��ԭ�ļ���ȫ·���ļ���
		memset (FullFileName, 0, sizeof(FullFileName));
		if(get_restore_item_name (&restoreItem, FullFileName) < 0)
		{
			return -1;
		}

		// ����Ƿ������ͬ�������ļ�
		if( !XM_FindFirstFile(FullFileName, &FindFileData, FileNameBuffer, XM_MAX_FILEFIND_NAME) )
			break;	// δ�ҵ�
		XM_FindClose (&FindFileData);

		// �ҵ���ͬ�ļ������ļ�
		
		// �������ָ��ܿ�ͬ���ļ�
		CreationTime.bSecond ++;
		if(CreationTime.bSecond >= 60)
			CreationTime.bSecond = 0;
		loop ++;
	}
	if(loop == 60)
	{
		// ���������������ͬ��
		ret = -1;
	}
	else
	{
		if(strlen(FullFileName) >= cbFileName)
		{
			ret = -1;
		}
		else
		{
			strcpy (lpFileName, FullFileName);
			ret = 0;
		}
	}
	return ret;	
}

// ���ҿ��õĻ����ļ���, ȷ���û����ļ�û�б�ռ��
static int XM_RecycleGetRecycleFileName (XM_RECYCLEITEM *item, char *lpFileName, unsigned int cbFileName)
{
	XMSYSTEMTIME	CreationTime;
	XMFILEFIND FindFileData;
	char FileNameBuffer[XM_MAX_FILEFIND_NAME];
	char FullFileName[XM_MAX_FILEFIND_NAME];
	int loop;
	int ret = 0;

	XM_RECYCLEITEM recycleItem;

	memcpy (&recycleItem, item, sizeof(XM_RECYCLEITEM));

	// ��ȡ�������ļ����ļ�����ʱ��
	XM_GetDateTimeFromFileName (recycleItem.item_name, &CreationTime);

	memset (FullFileName, 0, sizeof(FullFileName));

	loop = 0;
	while(loop < 60)
	{
		// �Ӵ���ʱ�����ļ���
		XM_MakeFileNameFromCurrentDateTime (&CreationTime, recycleItem.item_name, sizeof(recycleItem.item_name));

		// ��ȡ�����ļ���ȫ·���ļ���
		memset (FullFileName, 0, sizeof(FullFileName));
		if(get_recycle_item_name (&recycleItem, FullFileName) < 0)
			return -1;

		// ����Ƿ������ͬ�������ļ�
		//if( !XM_FindFirstFile(FullFileName, &FindFileData, FileNameBuffer, XM_MAX_FILEFIND_NAME) )
		//	break;	// δ�ҵ�
		//XM_FindClose (&FindFileData);
		void *fp = XM_fopen (FullFileName, "rb");
		if(fp == NULL)
		{
			// �ļ���û�б�ռ��
			break;
		}
		XM_fclose (fp);
		// �ҵ���ͬ�ļ������ļ�
		
		// �������ָ��ܿ�ͬ���ļ�
		CreationTime.bSecond ++;
		if(CreationTime.bSecond >= 60)
			CreationTime.bSecond = 0;
		loop ++;
	}
	if(loop == 60)
	{
		// ���������������ͬ��
		ret = -1;
	}
	else
	{
		if(strlen(FullFileName) >= cbFileName)
		{
			ret = -1;
		}
		else
		{
			strcpy (lpFileName, FullFileName);
			ret = 0;
		}
	}
	return ret;	
}

// ��ԭָ��������ʶ���ļ�
// ����ֵ
//   -1  ʧ��
//   0   �ɹ�
int XM_RecycleRestoreFile (unsigned int index)
{
	char recycle_file_name[VIDEOITEM_MAX_FILE_NAME];	// �����ļ���
	char restore_file_name[VIDEOITEM_MAX_FILE_NAME];	// ��ԭ�ļ���
	char *src, *dst;
	char *last;
	XM_RECYCLEITEM *item;

	if(index >= recycle_item_count)
	{
		XM_printf ("the index(%d) of recycle item invalid\n", index);
		return -1;
	}
	if(recycle_item_pool == NULL)
	{
		XM_printf ("restore file failed because item_pool is NULL\n");
		return -1;
	}

	item = recycle_item_pool + index;
	// �����ļ���
	if(get_recycle_item_name (item, recycle_file_name) < 0)
	{
		XM_printf ("restore file failed because recycle_file_name is invalid\n");
		return -1;
	}

	// ��ԭ�ļ���
	//get_restore_item_name (recycle_item_pool + index, restore_file_name);
	if(XM_RecycleGetRestoreFileName (item, restore_file_name, sizeof(restore_file_name)) < 0)
	{
		XM_printf ("restore file(%s) failed because no available name\n", recycle_file_name);
		return -1;
	}


	// ��ԭ�ļ�
	if(XM_MoveFile (recycle_file_name, restore_file_name) < 0)
	{
		XM_printf ("restore file failed,  move file from (%s) to file(%s) NG\n", recycle_file_name, restore_file_name);
		return -1;
	}
	
	FS_CACHE_Clean ("");

	// ����Ƿ�����Ƶ�ļ�
	if(item->file_type == XM_FILE_TYPE_VIDEO)	// ��Ƶ
	{
		// ������뵽��Ƶ�����ݿ�
		XM_VideoItemAppendVideoFile (item->channel, restore_file_name);
	}

	// ���ѻ�ԭ����Ŀ�����ݿ���ɾ��
	last = (char *)(recycle_item_pool + recycle_item_count);
	src = (char *)(recycle_item_pool + index + 1);
	dst = (char *)(recycle_item_pool + index);
	while(src < last)
	{
		*dst ++ = *src ++;
	}

	recycle_item_count --;

	return 0;
}




// ����վ��ʼ��, �����������Դ
void XM_RecycleInit (void)
{
	unsigned int item_count = 0;
	unsigned char ch;
	XM_RECYCLEITEM *item;

	if(recycle_item_count)
	{
		XM_printf ("error, recycle init again\n");
		return;
	}

	for (ch = 0; ch < XM_VIDEO_CHANNEL_COUNT; ch ++)
	{
		item_count += _RecycleGetItemCount (ch, XM_FILE_TYPE_VIDEO);
		item_count += _RecycleGetItemCount (ch, XM_FILE_TYPE_PHOTO);
	}

	recycle_item_count = item_count;
	if(recycle_item_count >= 1024)
		recycle_item_count = 1024;

	if(recycle_item_count == 0)
		return;

	recycle_item_pool = kernel_malloc (recycle_item_count * sizeof(XM_RECYCLEITEM));
	if(recycle_item_pool == NULL)
	{
		XM_printf ("XM_RecycleInit failed, malloc (recycle_item_count = %d) NG\n", recycle_item_count);
		return;
	}

	item = recycle_item_pool;
	item_count = recycle_item_count;
	for (ch = 0; ch < XM_VIDEO_CHANNEL_COUNT; ch ++)
	{
		unsigned int count;
		count = recycle_enum_item (ch, XM_FILE_TYPE_VIDEO, item, item_count);
		item += count;
		item_count -= count;
		count = recycle_enum_item (ch, XM_FILE_TYPE_PHOTO, item, item_count);
		item += count;
		item_count -= count;
	}
}

// ����վ�ر�, �ͷŷ������Դ
void XM_RecycleExit (void)
{
	recycle_item_count = 0;
	if(recycle_item_pool)
	{
		kernel_free (recycle_item_pool);
		recycle_item_pool = NULL;
	}
}

int XM_RecycleDeleteFile (unsigned int channel, unsigned int type, const char *file_name)
{
	char fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	XMFILEFIND fileFind;
	char recycle_file_name[VIDEOITEM_MAX_FILE_NAME];	// �����ļ���
	char restore_file_name[VIDEOITEM_MAX_FILE_NAME];	// ��ԭ�ļ���
	const char *video_path;
	XM_RECYCLEITEM recycleItem;
	int ret;

	if(channel >= XM_VIDEO_CHANNEL_COUNT)
		return -1;
	if(type >= XM_FILE_TYPE_COUNT)
		return -1;

	video_path = XM_VideoItemGetVideoPath (channel);
	if(video_path == NULL)
		return -1;

	// ��鲢���������ļ���·��
	sprintf (recycle_file_name, "%s\\RECYCLE", video_path);
	// XM_MkDir (recycle_file_name);
	// ��鱣������ļ���Ŀ¼�Ƿ����
	// if( !XM_FindFirstFile(recycle_file_name, &fileFind, fileName, XM_MAX_FILEFIND_NAME) )
	//	return -1;
	// XM_FindClose (&fileFind);
	if(check_and_create_directory (recycle_file_name) < 0)
	{
		XM_printf ("XM_RecycleDeleteFile Failed, can't create folder(%s)\n", recycle_file_name);
		return -1;
	}

	recycleItem.channel = (unsigned char)channel;
	recycleItem.file_type = (unsigned char)type;
	recycleItem.rev = 0;
	strcpy (recycleItem.item_name, file_name);

	if(XM_RecycleGetRecycleFileName (&recycleItem, recycle_file_name, VIDEOITEM_MAX_FILE_NAME) < 0)
		return -1;

	sprintf (restore_file_name, "%s\\%s.%s", video_path, file_name, 
		(type == XM_FILE_TYPE_VIDEO) ? XM_VIDEO_FILE_EXT : XM_PHOTO_FILE_EXT );

	ret = XM_MoveFile (restore_file_name, recycle_file_name);
	if(ret == 0)
	{
		FS_CACHE_Clean ("");
	}
	return ret;
}

extern int XMSYS_file_system_block_write_access_lock (void);
extern void XMSYS_file_system_block_write_access_unlock (void);

// �������վ�е������ļ�
// 0   �ɹ�
// < 0 ʧ��
int XM_RecycleCleanFile (char *recycle_path)
{
	char fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	XMFILEFIND fileFind;
	char full_path_file_name[XM_MAX_FILEFIND_NAME];
	int ret = 0;
	XMBOOL find_ret;
	
	// ����������̴Ӱ�ȫ���ø�Ϊ�ǰ�ȫ�����, ��Ҫ����
	if(XMSYS_file_system_block_write_access_lock () < 0)
	{
		XMSYS_file_system_block_write_access_unlock ();
		return -1;
	}

	if(XM_FindFirstFile (recycle_path, &fileFind, fileName, XM_MAX_FILEFIND_NAME))
	{
		XMSYS_file_system_block_write_access_unlock ();
		do
		{
			DWORD dwFileAttributes = fileFind.dwFileAttributes;
			if((dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == 0x00)
			{
				sprintf (full_path_file_name, "%s\\%s", recycle_path, fileName);
				//if(XM_RemoveFile (full_path_file_name) == 0)			// ǰ̨�ļ�ɾ��, ����
				//	ret = -1;
				if(XMSYS_remove_file (full_path_file_name) == 0)		// ��̨�ļ�ɾ��, ����
					ret = -1;
			}
			
			if(XMSYS_file_system_block_write_access_lock () < 0)
			{
				XMSYS_file_system_block_write_access_unlock ();
				break;
			}
			find_ret = XM_FindNextFile (&fileFind);
			XMSYS_file_system_block_write_access_unlock ();
		// } while(XM_FindNextFile (&fileFind));
		} while(find_ret);	
		
		// XM_FindClose (&fileFind);
		if(XMSYS_file_system_block_write_access_lock () == 0)
		{
			XM_FindClose (&fileFind);
		}
		XMSYS_file_system_block_write_access_unlock ();
	}
	else
	{
		XMSYS_file_system_block_write_access_unlock ();
		ret = -1;
	}
	
	return ret;
}

