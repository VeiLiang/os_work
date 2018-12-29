//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_album.c
//	  ��������
//
//	Revision history
//
//		2012.09.22	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"


#define	MAX_PHOTO_FILE_COUNT		1024

static unsigned int  PhotoFileCount = 0;
__no_init static XM_RECYCLEITEM PhotoFileList[MAX_PHOTO_FILE_COUNT];	// �������е���Ƭ�ļ���


// �ж��Ƿ��Ƿ��ϱ�׼��������Ƭ�ļ�
static int is_valid_image_filename (char *file_name)
{
	char *ch;
	int name_count = 0;
		
	ch = file_name;
	while(*ch)
	{
		// �ļ���������ֻ��������
		if(*ch >= '0' && *ch <= '9' || *ch >= 'A' && *ch <= 'Z')
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

	if(	(ch[0] == 'j' || ch[0] == 'J') && (ch[1] == 'p' || ch[1] == 'P') &&	(ch[2] == 'g' || ch[2] == 'G') && (ch[3] == 0) )
	{
		// ��Ч���ļ���
		return 1;
	}
	else
		return 0;
}


// ��ȡ��Ƭ�����ݿ�����Ƭ��ĸ���
// image_channel ��Ƭ��ͨ��, 0,1
// ����ֵ
// 0 ��ʾ����Ƭ��
// > 0 ��ʾ�Ѽ�¼����Ƭ�����
int XM_ImageItemGetImageItemCount (unsigned int image_channel)
{
	char ImagePath[64];
	char fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	XMFILEFIND fileFind;
	int item_count = 0;
	const char *video_path;
	
	if(image_channel >= XM_VIDEO_CHANNEL_COUNT)
		return 0;

	video_path = XM_VideoItemGetVideoPath (image_channel);
	if(video_path == NULL)
		return 0;

#ifdef _WINDOWS
	sprintf (ImagePath, "%s\\*.*", video_path);
#else
	sprintf (ImagePath, "%s", video_path);
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
				if(is_valid_image_filename (fileName))
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

// ��ȡָ����Ƭͨ�����ļ��б�
// image_channel ��Ƭ��ͨ��
// index ��ȡ�ļ�¼��ʼ���
// count ��ȡ�ļ�¼����(������ʼ���)
// packet_addr ����������ַ
// packet_size ���������ֽڴ�С
// ����ֵ
// < 0 ʧ��
// = 0 �������ļ���Ϊ0
// > 0 ��������Ч�ļ���
int XM_ImageItemGetImageItemList (unsigned int image_channel, 
											 unsigned int index, 
											 unsigned int count,
											 void *packet_addr, 
											 int packet_size
											)
{
	char ImagePath[64];
	char fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	XMFILEFIND fileFind;
	unsigned int item_index = 0;
	const char *video_path;
	
	char *buff = (char *)packet_addr;
	int size = packet_size;
	
	if(image_channel >= XM_VIDEO_CHANNEL_COUNT)
		return -1;
	if(packet_size < 16)
		return -1;
	if(packet_addr == NULL)
		return -1;
	if(count == 0)
		return 0;

	video_path = XM_VideoItemGetVideoPath (image_channel);
	if(video_path == NULL)
		return -1;
#ifdef _WINDOWS
	sprintf (ImagePath, "%s\\*.*", video_path);
#else
	sprintf (ImagePath, "%s", video_path);
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
				if(is_valid_image_filename (fileName))
				{
					if(item_index >= index)
					{
						memcpy (buff, fileName, 8);
						buff[8] = 0;
						buff += 9;
						size -= 9;
						count --;
						
						if(size < 9)
							break;
					}
					item_index ++;
				}
			}
			memset (fileName, 0, 12);		
		} while(XM_FindNextFile (&fileFind) && count > 0);
		XM_FindClose (&fileFind);
	}
	else
	{
		return -1;
	}
	return item_index;
}



void AP_AlbumInit (void)
{
	unsigned char ch;
	char ImagePath[64];
	char fileName[XM_MAX_FILEFIND_NAME];		// �����ļ����ҹ��̵��ļ���
	XMFILEFIND fileFind;

	PhotoFileCount = 0;
	
	for (ch = 0; ch < XM_VIDEO_CHANNEL_COUNT; ch ++)
	{
		const char *video_path = XM_VideoItemGetVideoPath (ch);
		if(video_path == NULL)
			continue;

#ifdef _WINDOWS
		sprintf (ImagePath, "%s\\*.*", video_path);
#else
		sprintf (ImagePath, "%s", video_path);
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
					if(is_valid_image_filename (fileName))
					{
						PhotoFileList[PhotoFileCount].channel = ch;
						PhotoFileList[PhotoFileCount].file_type = XM_FILE_TYPE_PHOTO;
						PhotoFileList[PhotoFileCount].rev = 0;
						memcpy (PhotoFileList[PhotoFileCount].item_name, fileName, 8);
						PhotoFileList[PhotoFileCount].item_name[8] = 0;

						PhotoFileCount ++;
						if(PhotoFileCount >= MAX_PHOTO_FILE_COUNT)
							break;
					}
				}
			} while(XM_FindNextFile (&fileFind));		
			XM_FindClose (&fileFind);
		}

		if(PhotoFileCount >= MAX_PHOTO_FILE_COUNT)
			break;
	}
}

unsigned int AP_AlbumGetPhotoCount (void)
{
	return PhotoFileCount;
}

const char *AP_AlbumGetPhotoName (unsigned int index)
{
	if(index >= PhotoFileCount)
		return NULL;
	return PhotoFileList[index].item_name;
}

unsigned int AP_AlbumGetPhotoChannel (unsigned int index)
{
	if(index >= PhotoFileCount)
		return XM_VIDEO_CHANNEL_0;
	return PhotoFileList[index].channel;
}


// ������Ƭ����ֵ����Ƭ�������ɾ��
int AP_AlbumDeletePhoto (unsigned int index)
{
	XM_RECYCLEITEM *item;
	char photo_filename[32];

	char *src, *dst;
	char *last;
	if(index >= PhotoFileCount)
		return -1;

	if(AP_AlbumGetPhotoFileName (index, photo_filename, sizeof(photo_filename)) < 0)
		return -1;

	item = &PhotoFileList[index];

	//XM_RemoveFile (photo_filename);
	if(XM_RecycleDeleteFile (item->channel, item->file_type, item->item_name) < 0)
		return -1;

	last = (char *)(PhotoFileList + PhotoFileCount);
	
	src = (char *)(PhotoFileList + (index + 1));
	dst = (char *)(PhotoFileList + index);
	while(src < last)
	{
		*dst ++ = *src ++;
	}

	PhotoFileCount --;
	return 0;
}

// ɾ����������е���Ƭ
// ����ֵ
// < 0     ʧ��
//   0     �ɹ�
int AP_AlbumDeleteAllPhoto (void)
{
	int ret = 0;
	while(PhotoFileCount > 0)
	{
		if(AP_AlbumDeletePhoto (PhotoFileCount - 1) < 0)
		{
			ret = -1;
			break;
		}
	}

	return ret;
}


// ������Ƭ����ֵ��ȡ��Ƭ��Ӧ���ļ���
int AP_AlbumGetPhotoFileName (unsigned int index, char *lpFileName, int cbFileName)
{
	const char *video_path;
	XM_RECYCLEITEM *item;
	if(index >= PhotoFileCount)
		return -1;

	item = &PhotoFileList[index];
	if(lpFileName == NULL)
		return -1;
	if(cbFileName < 32)
		return -1;

	video_path = XM_VideoItemGetVideoPath (item->channel);
	if(video_path == NULL)
		return -1;

	sprintf (lpFileName, "%s\\%s.JPG", video_path, item->item_name);
	return 0;
}

