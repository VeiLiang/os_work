//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_videoitem.c
//	  ��Ƶ�б�ӿ�
//
//	Revision history
//
//		2011.12.16	ZhuoYongHong Initial version
//		2018.04.30	ZhuoYongHong ��Ϊʹ�ó��ļ���������Ƶ���ļ�
//
//****************************************************************************
#ifndef _XM_VIDEO_ITEM_H_
#define _XM_VIDEO_ITEM_H_

#include <xm_proj_define.h>
#include <xm_type.h>
#include <xm_base.h>
#include <xm_file.h>

#if defined (__cplusplus)
extern "C"{
#endif

enum {
	XM_FILE_TYPE_VIDEO =	0,			// ��Ƶ
	XM_FILE_TYPE_PHOTO,				// ��Ƭ
	XM_FILE_TYPE_COUNT
};

enum {
	XM_VIDEOITEM_CLASS_NORMAL = 0,		// ��ͨ¼������
	XM_VIDEOITEM_CLASS_MANUAL,			// �ֶ�¼������
	XM_VIDEOITEM_CLASS_URGENT,			// ����¼������(G-Sensor����)
	XM_VIDEOITEM_CLASS_COUNT	
};

enum {
	XM_VIDEOITEM_CLASS_BITMASK_NORMAL = (1 << XM_VIDEOITEM_CLASS_NORMAL),
	XM_VIDEOITEM_CLASS_BITMASK_MANUAL = (1 << XM_VIDEOITEM_CLASS_MANUAL),
	XM_VIDEOITEM_CLASS_BITMASK_URGENT = (1 << XM_VIDEOITEM_CLASS_URGENT),
	XM_VIDEOITEM_CLASS_BITMASK_ALL = 0xFFFFFFFF
};

enum {
	XM_VIDEO_CHANNEL_0 = 0,			// ͨ��0 --> ǰ������ͷ
	XM_VIDEO_CHANNEL_1,				// ͨ��1 --> ��������ͷ
	XM_VIDEO_CHANNEL_2,				// ͨ��2 --> �������ͷ
	XM_VIDEO_CHANNEL_3,				// ͨ��3 --> �Ҳ�����ͷ
	XM_VIDEO_CHANNEL_COUNT			// ͨ������
};

enum {
	XM_VIDEOITEM_VOLUME_0 = 0,
	//XM_VIDEOITEM_VOLUME_1,		
	XM_VIDEOITEM_VOLUME_COUNT		// ������
};

// ���򷽷�
enum {
	XM_VIDEOITEM_ORDERING_NAME = 0,			// ȱʡ��������
	XM_VIDEOITEM_ORDERING_SERIAL_NUMBER,	// ���������
	XM_VIDEOITEM_ORDERING_COUNT
};

// Ԥ��������������Ŀ
enum {
	XM_VIDEOITEM_QUOTA_PROFILE_1 = 0,		// 
	XM_VIDEOITEM_QUOTA_PROFILE_COUNT
};

#if _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_AVI_
#define	XM_VIDEO_FILE_EXT		"AVI"
#elif _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_MKV_
#define	XM_VIDEO_FILE_EXT		"MKV"
#elif _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_MOV_
#define	XM_VIDEO_FILE_EXT		"MOV"
#endif

#define	XM_PHOTO_FILE_EXT		"JPG"

// ��Ƶͨ�����ڱ�־
#define	XM_VIDEO_CH_0		0x01
#define	XM_VIDEO_CH_1		0x02
#define	XM_VIDEO_CH_2		0x04
#define	XM_VIDEO_CH_3		0x08

// ��Ƶ�б�ģʽ����(ѭ��������)
#define	VIDEOITEM_CIRCULAR_VIDEO_MODE			0		// ��ѭ��¼���б�ģʽ
#define	VIDEOITEM_PROTECT_VIDEO_MODE			1		// ������¼���б�ģʽ

#define	VIDEOITEM_DRIVER_NAME_SIZE			

// �����Ƶ�ļ�ȫ·�������� (��������0)
#define	VIDEOITEM_MAX_FULL_PATH_NAME			64		// ���������Ƶ�ȫ·������
#define	VIDEOITEM_MAX_FILE_NAME					VIDEOITEM_MAX_FULL_PATH_NAME		

#define	TEMPITEM_MAX_FILE_NAME					16		// TEMPNNNN_00P.AVI


#define	VIDEOITEM_LFN_SIZE						28		// 20180318_121056_00000000_00N		// �����ڿ�0�ϵ�ͨ��0�����Ϊ0���ֶ�������Ƶ��(�ֶ�����ͨ��Ƶͨ��ֻ����������)
																	//	20180318_121056_00000000_11N		// �����ڿ�1�ϵ�ͨ��1�����Ϊ0����ͨ��Ƶ��
																	//	20180318_121056_00000001_00U		// �����ڿ�0�ϵ�ͨ��0�����Ϊ1�Ľ�����Ƶ��

#define	VIDEOITEM_MAX_NAME						(VIDEOITEM_LFN_SIZE)		// ��Ƶ�ļ�������ֽڳ���


#define	VIDEOITEM_MAX_FULL_NAME					(VIDEOITEM_LFN_SIZE + 4)	// �����ļ���չ������Ƶ���ļ�����

typedef struct _XMVIDEOITEM {
	unsigned char		video_name[VIDEOITEM_MAX_FULL_NAME + 1];	// ��Ƶ���ļ�����(20180318_121056_001_00P.AVI, 20180318_121056_001_11N.JPG)
	unsigned char		video_channel;				// ͨ�����ڱ�־��ÿ��ͨ��ʹ��һλ��ʾ������ʾ8��ͨ��	
	unsigned char		video_lock;					// �ļ����� 
													// ֻ����ʾ���� ������һ������ͨ��ֻ������ʾΪ����������
	unsigned char		video_update;				// ����´�������Ƶ�ļ��Ƿ��ѹرղ�����
	unsigned char		video_ref_count;			// ��Ƶ�����ü���. ����Ƶ������ʱ(�����ڲ���), ��ֹɾ������
	
	XMSYSTEMTIME		video_create_time;			// ��Ƶ��ÿ��ͨ���Ĵ���ʱ��
	
	unsigned char		volume_index;				// SD�����
	unsigned char		file_type;					// �ļ�����
	unsigned char		stream_class;				// ������(��ͨ���ֶ������)
	
	unsigned char		initial_class;				// ��ʼ����ʱ��������
	
	unsigned char		stream_length_valid;		// ��ʱ�䳤���Ƿ���Ч
	unsigned int		stream_length;				// ��ʱ�䳤��(����),¼��ʱ��

	unsigned int		file_size;					// ���ļ��ֽڳ���
	XMINT64				video_size;					// ��Ƶ�ļ��ֽڴ�С(����ͨ���ۼ�)
	unsigned int		serial_number;				// ���
	
} XMVIDEOITEM;

// ����ṹ�壬����ɾ����Ƶ���Ӧ�Ĵ����ļ�
typedef struct tagVIDEOFILETOREMOVE {
	int		count;							// ɾ���ļ�����
	int		volume_index[XM_VIDEO_CHANNEL_COUNT];						// ���̾�����
	char		file_name[XM_VIDEO_CHANNEL_COUNT][VIDEOITEM_MAX_FILE_NAME];		// ���8��ͨ��
//	unsigned char	type[8];					// ɾ���ļ�������
	unsigned char	channel[XM_VIDEO_CHANNEL_COUNT];				// ɾ���ļ���ͨ��	
} VIDEOFILETOREMOVE;

int XM_VideoItemInit (unsigned int video_channel_count, const char **video_channel_file_path);
int XM_VideoItemInitEx (unsigned int video_volume_count, unsigned int video_channel_count, const char **video_channel_file_path);

void XM_VideoItemExit (void);

// ���������SD��������ɨ��������Ƶ�ļ�, ������Ƶ�����
// 0  ��Ƶ��������ɹ�
// -1 ��Ƶ�������ʧ��(���̾���ļ�·��������)
// -2 ��Ƶ�������ʧ��(�ڴ�����쳣)
int XM_VideoItemOpenService (void);

// �γ�SD����, �ر���Ƶ�����
void XM_VideoItemCloseService (void);

// ��ȡ��Ƶ�б��ļ�����
// mode ָ��ѭ��¼��ģʽ���Ǳ���¼��ģʽ
DWORD AP_VideoItemGetVideoFileCount (BYTE mode);

// ��ȡ��Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ��Ŀ���(��ʱ���С����)
// dwVideoIndex ��0��ʼ
HANDLE AP_VideoItemGetVideoItemHandle (BYTE mode, DWORD dwVideoIndex);

// ��ȡ��Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ��Ŀ��� (��ʱ��Ӵ�С)
// dwVideoIndex ��0��ʼ
HANDLE AP_VideoItemGetVideoItemHandleReverseOrder (BYTE mode, DWORD dwVideoIndex);


// ����Ƶ������ȡ��Ƶ��ṹָ��
XMVIDEOITEM * AP_VideoItemGetVideoItemFromHandle (HANDLE hVideoItemHandle);

// ��ȡ��Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ��Ŀ
// dwVideoIndex ��0��ʼ
XMVIDEOITEM *AP_VideoItemGetVideoItem (BYTE mode, DWORD dwVideoIndex);

// ��ȡǰһ����Ƶ������, -1��ʾ������
DWORD AP_VideoItemGetPrevVideoIndex (BYTE mode, DWORD dwVideoIndex);

// ��ȡ��һ����Ƶ������, -1��ʾ������
DWORD AP_VideoItemGetNextVideoIndex (BYTE mode, DWORD dwVideoIndex);

// ��ʾ��ʽ��ָ����ŵ���Ƶ�ļ���
char *AP_VideoItemFormatVideoFileName (XMVIDEOITEM *lpVideoItem, char *lpDisplayName, int cbDisplayName);

// ����Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ�ļ�����(����) (��������ͨ��)
XMBOOL AP_VideoItemLockVideoFile (XMVIDEOITEM *lpVideoItem);

// ��ָ���ļ�������Ƶ����
XMBOOL AP_VideoItemLockVideoFileFromFileName (const char *lpVideoFile);

// ����Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ�ļ�����(��ѭ������)
XMBOOL AP_VideoItemUnlockVideoFile (XMVIDEOITEM *lpVideoItem);
XMBOOL AP_VideoItemUnlockVideoFileFromFileName (const char *lpVideoFile);

// ��ָ���ļ�������Ƶ���ʹ���У���ֹɾ����������Ƶ��ɾ��
XMBOOL XM_VideoItemMarkVideoFileUsage (const char *lpVideoFile);
// ��ָ���ļ�������Ƶ���δʹ��
XMBOOL XM_VideoItemMarkVideoFileUnuse (const char *lpVideoFile);

// ����һ���µ���Ƶ���� (�ѷ�������ʹ��XM_VideoItemCreateVideoItemHandleEx)
//   (ȱʡΪ��Ƶ�ļ�����ͨ���࣬��0)
// nVideoChannel = 0, 1, 2, 3 ... ��ʾ����һ��ָ��ͨ������Ƶ��
HANDLE XM_VideoItemCreateVideoItemHandle (unsigned int VideoChannel, XMSYSTEMTIME *filetime);

// ɾ��һ����Ƶ����(����ɾ��������ͨ������Ƶ�ļ�)
XMBOOL XM_VideoItemDeleteVideoItemHandle (HANDLE hVideoItemHandle, VIDEOFILETOREMOVE *lpVideoFileToRemove);
XMBOOL XM_VideoItemDeleteVideoItemHandleFromFileName (const char *lpVideoFile);

// ��ȡ����Ƶ��(ֻ����1·��Ƶ)��ͨ�����
// >= 0  ���ص�ͨ�����
// < 0   ��Ч�ľ��������Ƶ������һ��������Ƶ��(���ͨ�������)
int XM_VideoItemGetVideoChannel (HANDLE hVideoItemHandle);

// ��ȡ��Ƶ��ָ��ͨ����ȫ·���ļ���
XMBOOL XM_VideoItemGetVideoFilePath (XMVIDEOITEM *lpVideoItem, unsigned int nVideoChannel, char *lpFileName, int cbFileName);

// ��ȡ��Ƶ����ʱ�䳤����Ϣ
DWORD AP_VideoItemGetVideoStreamSize (XMVIDEOITEM *lpVideoItem, unsigned int nVideoChannel);

// ͨ��ȫ·���ļ���������Ƶ������ͨ����
HANDLE XM_VideoItemGetVideoItemHandle (const char *lpVideoFile, int* pVideoChannel);

// ָ��ͨ������Ƶ�ļ��Ѵ���/д�벢�رգ���������Ӧ����Ƶ������е���Ϣ
XMBOOL XM_VideoItemUpdateVideItemHandle (HANDLE hVideoItemHandle, int nVideoChannel, unsigned int cbVideoFileSize);

XMBOOL XM_VideoItemUpdateVideoItemFromFileName (const char *lpVideoFileName, unsigned int cbVideoFileSize, XMSYSTEMTIME *create_time);

// �����Ƶ�����Ƿ��Ѹ���(�ѹرն�Ӧ����Ƶ�ļ�)
// ͨ���ļ������Ҷ�����ʹ�þ�������������Ϊ�ļ���ɾ������Ч
XMBOOL XM_VideoItemWaitingForUpdate (const char *lpVideoFileName);

// ��ز��ͷſ�ѭ����¼���¼ʱ����ʾ, ��¼ʱ�䱨��
// bReportEstimatedRecordingTime 1 ��¼ʱ���鲢��ʾ
VOID XM_VideoItemMonitorAndRecycleGarbage (XMBOOL bReportEstimatedRecordingTime);


// ����/ɾ�������ź���
VOID  XM_VideoItemCreateResourceMutexSemaphore (VOID);
VOID	XM_VideoItemDeleteResourceMutexSemaphore (VOID);

VOID	XM_VideoItemLock (VOID);
VOID	XM_VideoItemUnlock (VOID);

// ��Ƶ�����ȴ�"�����������¼�"
// ����ֵ 
//		0	 �����������¼���ָ��ʱ���ڱ�����
//    -1	 �����������¼���ָ��ʱ����û�б�����
int XM_CodecStartEventWait (unsigned int milliseconds_to_timeout);		// �ȴ������������¼�
// ���"�����������¼�"
void XM_CodecStartEventReset (void);
// "�ⲿ����������"���øú���֪ͨ"��Ƶ�����"�������������������
void XM_CodecStartEventSet (void);			// ���ñ����������¼�

// ���ݶ�������Զ�ȡ��Ƶ��ָ�����Ե��ļ��б�
// video_channel ��Ƶ��ͨ��
// index ��ȡ�ļ�¼��ʼ���
// count ��ȡ�ļ�¼����(������ʼ���)
// packet_addr ����������ַ
// packet_size ���������ֽڴ�С
// ����ֵ
// < 0 ʧ��
// >= 0 �������б����¼���ݵ��ֽڳ��� 
int XM_VideoItemGetVideoItemListEx (unsigned int video_channel, 		// ͨ���ţ� 0xFFFFFFFF��ʾ����ͨ��
											 unsigned int volume_index,			// ���̾��, 0xFFFFFFFF��ʾ���о�
											 unsigned int file_type,				// �ļ�����, 0xFFFFFFFF��ʾ�����ļ�����
											 unsigned int stream_class_bitmask,	// ¼��������룬0xFFFFFFFF��ʾ����¼��
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL ��ʾ������ͨ���ֶ�����
											 unsigned int index, 
											 unsigned int count,
											 void *packet_addr, 
											 int packet_size
											);

// ��ȡָ��¼��ͨ�����ļ��б�
// video_channel ��Ƶ��ͨ��
// index ��ȡ�ļ�¼��ʼ���
// count ��ȡ�ļ�¼����(������ʼ���)
// packet_addr ����������ַ
// packet_size ���������ֽڴ�С
// ����ֵ
// < 0 ʧ��
// >= 0 �������б����¼���ݵ��ֽڳ��� 
int XM_VideoItemGetVideoItemList (unsigned int video_channel, 
											 unsigned int index, 
											 unsigned int count,
											 void *packet_addr, 
											 int packet_size
											);

void combine_fullpath_media_filename (unsigned char* full_path_file_name, 
												  int cb_full_path_file_name,
												 unsigned char ch, 
												 unsigned char type, 
												 const unsigned char* name);
// 0 �ɹ�����
// -1 ����ʧ��
int extract_fullpath_media_filename (const unsigned char* full_path_file_name, 
												 unsigned char* ch, 
												 unsigned char* type, 
												 unsigned char* name, int cb_name);

// ����ֵ����
//	0	�ѻ�ȡ����Ϣ
// -1	�޷���ȡ����Ϣ
int XM_VideoItemGetDiskUsage (XMINT64 *TotalSpace, 		// SD���ռ�
										XMINT64 *LockedSpace, 		// ���������ļ��ռ�
										XMINT64 *RecycleSpace,		// ��ѭ��ʹ���ļ� + ���пռ�
										XMINT64 *OtherSpace			// �����ļ�(ϵͳ)ռ�ÿռ�
									);

// �ļ�ɾ��
// ����ֵ����
// 0 ɾ��ʧ��
// 1 ɾ���ɹ�
int XMSYS_remove_file (const char *filename);

// �����Ƶ������Ƿ��ѿ���
// 1 ��Ƶ������ѿ���
// 0 ��Ƶ�����δ����
XMBOOL XM_VideoItemIsServiceOpened (void);

// ������Ƶ���ļ��Ĵ���ʱ�������ļ���
// YMDHMMSS
// Y  0 -- > 2014
//    Z -- > 2014 + 9 + 26 = 2049
// M  1 -- >  1��
//    C -- > 12��
// D  1 -- >  1��
//    V -- > 31��
// H  0 -- >  0��
//    M -- > 23��
// MM 0 -- >  0��
//    59-- > 59��
// SS 0 -- >  0��
//    59-- > 59��
int XM_MakeFileNameFromCurrentDateTime (XMSYSTEMTIME *filetime, char* filename, int cb_filename);


// ������Ƶ������(����ʱ�䣬¼��ͨ���ţ��ļ����ͣ���Ƶ������)�����ļ�����
// 20180411_115623_000_00P.JPG		���浽��0��ͨ��0���ֶ���������Ƭ	
// 20180411_115623_000_11N.AVI		���浽��1��ͨ��1�Ŀ�ѭ�����ǵ���ͨ������Ƶ�ļ�
// 20180411_115623_000_01U.AVI		���浽��1��ͨ��0�Ľ������͵���Ƶ�ļ�����G-Sensor����
// 20180411_115623_000_10U.JPG		���浽��0��ͨ��1�Ľ������͵���Ƭ�ļ�����G-Sensor����
// ����ֵ
//	 0      �ɹ�
//  -1     ʧ��
int XM_MakeFileNameFromCurrentDateTimeEx (XMSYSTEMTIME *lpCreateTime, 			// �ļ�����ʱ�䣬 Ϊ�ձ�ʾʹ�õ�ǰʱ��
														unsigned int VideoChannel,				// ¼��ͨ����
														unsigned int FileType, 					// �ļ�����(��Ƶ����Ƭ)
														unsigned int VideoItemClass,			// ��Ƶ�����(�������ֶ������)
														unsigned int CardVolumeIndex,			// SD�������(��0��ʼ)
														unsigned int SerialNumber,				// ���
														char* lpFileName, int cbFileName		// �����ļ������Ļ�������ַ���������ֽڴ�С
														);

// ������Ƶ������(ͨ���ţ�����ʱ�䣬�ļ����ͣ���Ƶ����࣬����ľ����)����һ����Ƶ����
//		1) ����һ��λ�ڿ�0�ϵ�ͨ��0�Ľ���¼�����͵�¼����Ƶ��
//			XM_VideoItemCreateVideoItemHandleEx (XM_VIDEO_CHANNEL_0, create_time, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_URGENT, XM_VIDEOITEM_VOLUME_0);
//		2) ����һ��λ�ڿ�0�ϵ�ͨ��1����ͨ¼�����͵���Ƭ��Ƶ��
//			XM_VideoItemCreateVideoItemHandleEx (XM_VIDEO_CHANNEL_1, create_time, XM_FILE_TYPE_PHOTO, XM_VIDEOITEM_CLASS_NORMAL, XM_VIDEOITEM_VOLUME_0);
//		3) ����һ��λ�ڿ�1�ϵ�ͨ��1���ֶ�¼�����͵���Ƶ��Ƶ��
//			XM_VideoItemCreateVideoItemHandleEx (XM_VIDEO_CHANNEL_1, create_time, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_MANUAL, XM_VIDEOITEM_VOLUME_1);
// ����ֵ
//		0			ʧ��
//		��0ֵ		��Ƶ����
HANDLE XM_VideoItemCreateVideoItemHandleEx (unsigned int video_channel,		// ͨ����
														  XMSYSTEMTIME *create_time, 		// ��Ƶ���ʱ��
														  unsigned int file_type,			//	��Ƶ���ļ�����(��Ƶ����Ƭ)	
														  unsigned int videoitem_class, 	// ��Ƶ������(�������ֶ����߽���)
														  unsigned int video_volume_index	// �����SD�������(��0��ʼ)
														  );

// ֱ�Ӵ�����Ƶ���ȫ·���ļ���
// ����ֵ
//	  NULL		����ʧ��
//	  !=NULL		ȫ·���ļ���
char * XM_VideoItemCreateFullPathFileName (XMSYSTEMTIME *lpCreateTime, 			// �ļ�����ʱ�䣬 Ϊ�ձ�ʾʹ�õ�ǰʱ��
													 unsigned int VideoChannel, 			// ¼��ͨ����
													 unsigned int FileType, 				// �ļ�����(��Ƶ����Ƭ)
													 unsigned int VideoItemClass,			// ��Ƶ�����(�������ֶ������)
													 unsigned int CardVolumeIndex,		// SD�������(��0��ʼ)
													 char *lpFileName, int cbFileName	// ������Ƶ����ʱ�ļ����ļ������ļ����ֽڳ���
													);



// ����Ƶ���ļ������ָ��ļ�������ʱ��
void XM_GetDateTimeFromFileName (char* filename, XMSYSTEMTIME *filetime);


// ������Ƶ�����ʱ�ļ� (ʹ��ȱʡ��card 0����ͨ¼�����ͣ�ʹ�õ�ǰϵͳʱ�䣬��Ƶ����)
// VideoChannel ��Ƶ��ͨ����
//	lpVideoTempFileName  ������Ƶ����ʱ�ļ����ļ������ļ����ֽڳ���
// cbVideoTempFileName
char * XM_VideoItemCreateVideoTempFile (unsigned int nVideoChannel, char *lpFileName, int cbFileName);

// ͨ��ȫ·������ʱ��Ƶ���ļ������Ҷ�Ӧ����Ƶͨ����
XMBOOL XM_VideoItemGetVideoChannelFromTempVideoItemName (const char *lpVideoFile, int* pVideoChannel);

// ����Ƿ�����ʱ��Ƶ���ļ�
XMBOOL XM_VideoItemIsTempVideoItemFile (const char *lpVideoFileName);

XMBOOL XM_VideoItemUpdateVideoItemFromTempName (const char *lpVideoFileName, unsigned int cbVideoFileSize, 
																XMSYSTEMTIME *create_time);

// �����Ƶ����������Ƿ��ѿ���������������������Ƶ��¼
XMBOOL XM_VideoItemIsBasicServiceOpened (void);

// bPostFileListUpdateMessage �Ƿ�Ͷ����Ƶ���ļ��б������Ϣ
XMBOOL XM_VideoItemDeleteVideoItemHandleFromFileNameEx (const char *lpVideoFileName, int bPostFileListUpdateMessage);

// ���һ���µ���Ƶ�ļ�
XMBOOL XM_VideoItemAppendVideoFile (unsigned int ch, const char *lpVideoFile);


// ��ȡ��Ƶ��ͨ����Ӧ���ļ�·��
const char * XM_VideoItemGetVideoPath (unsigned int nVideoChannel);
const char * XM_VideoItemGetVideoPathByType(unsigned int nVideoChannel, unsigned char type);

// ��ȡ��Ƶ�ļ�ͨ��(·��)�ĸ���
int XM_VideoItemGetVideoFileChannel (void);


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
											);

// ��鿨����Ƶд������
// ����ֵ
//		-1		�޷���ȡ����ֵ
//		0		��Ƶ���ܿ��ܽϲ�
//		1		��Ƶ���������¼Ҫ��
int XM_VideoItemCheckCardPerformance (void);

// ��鿨�Ŀ���ѭ��¼��ռ�����(= ���̿����ֽ����� + ѭ��¼����Ƶռ�õ��ļ��ֽ�����)�Ƿ�������͵�¼��Ҫ��
// ����ֵ
//		-1		�޷���ȡ�����ܲ���
//		0		�޷��������¼��Ҫ��, ��ʾ��ʽ����
//		1		�������¼��Ҫ��
int XM_VideoItemCheckCardRecycleSpace (void);

// ���ָ�����Ŀ���ѭ��¼��ռ�����(= ���̿����ֽ����� + ѭ��¼����Ƶռ�õ��ļ��ֽ�����)�Ƿ�������͵�¼��Ҫ��
// ����ֵ
//		-1		�޷���ȡ�����ܲ���
//		0		�޷��������¼��Ҫ��, ��ʾ��ʽ����
//		1		�������¼��Ҫ��
int XM_VideoItemCheckCardRecycleSpaceEx (unsigned int volume_index);


// �ж��Ƿ��Ƿ��ϱ�׼��������Ƭ�ļ�
int is_valid_image_filename (char *file_name);

// �ж��Ƿ��Ƿ��ϱ�׼��������Ƶ�ļ�
int is_valid_video_filename (char *file_name);

// ��ȡָ����Ƭͨ�����ļ��б�, ������Ƶ�б��ʽ��֯
// image_channel ��Ƭ��ͨ��
// index ��ȡ�ļ�¼��ʼ���
// count ��ȡ�ļ�¼����(������ʼ���)
// packet_addr ����������ַ
// packet_size ���������ֽڴ�С
// ����ֵ
// < 0 ʧ��
// = 0 �������ļ���Ϊ0
// > 0 ��������Ч�ļ���
int XM_VideoItemGetImageItemList (unsigned int image_channel, 
											 unsigned int index, 
											 unsigned int count,
											 void *packet_addr, 
											 int packet_size
											);

// �򿪾����
// 0  ����ɹ�ִ��
// -1 ����ɹ�ʧ��
int xm_open_volume_service (char *volume_name, int b_post_insert_message);

// �رվ����
// 0  ����ɹ�ִ��
// -1 ����ɹ�ʧ��
int xm_close_volume_service (char *volume_name);

// ��ȡ��Ƶ�б��ļ�����
// mode ָ��ѭ��¼��ģʽ���Ǳ���¼��ģʽ
// nVideoChannel	ָ����Ƶͨ��, 0 ǰ������ͷ 1 ��������ͷ
DWORD AP_VideoItemGetVideoFileCountEx (BYTE mode, unsigned int nVideoChannel);

// ��ȡ��Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ��Ŀ��� (��ʱ��Ӵ�С)
// dwVideoIndex ��0��ʼ
// nVideoChannel ��Ƶͨ���� 0 ǰ������ͷ 1 ��������ͷ
HANDLE AP_VideoItemGetVideoItemHandleReverseOrderEx (BYTE mode, DWORD dwVideoIndex, unsigned int nVideoChannel);

// ��ȡ��Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ��Ŀ���(��ʱ���С����)
// dwVideoIndex ��0��ʼ
// nVideoChannel ��Ƶͨ���� 0 ǰ������ͷ 1 ��������ͷ
HANDLE AP_VideoItemGetVideoItemHandleEx (BYTE mode, DWORD dwVideoIndex, unsigned int nVideoChannel);


// ��ָ������Ƶ�ļ�(ȫ·����)ɾ��������վ
// 1 �ɹ�
// 0 ʧ��
XMBOOL XM_VideoItemDeleteVideoFileIntoRecycleBin (const char *lpVideoFileName);

// ����Ƶ���ļ���ӵ���Ƶ�����ݿ�
// 	ch					ͨ�����(0, 1, ...)
// 	lpVideoFile		ȫ·����Ƶ�ļ���
// ����ֵ
//	1		�ɹ�
//	0		ʧ��
XMBOOL XM_VideoItemAppendVideoFile (unsigned int ch, const char *lpVideoFile);

// ������ʣ��ռ��Ƿ��޷�����ʵʱ¼������
// ����ֵ
// 1   �޷�����ʵʱ¼������
// 0   ��������ʵʱ¼������
XMBOOL XM_VideoItemCheckDiskFreeSpaceBusy (void);


// 20170929
// ��ʱ���ڵ��ļ�ɾ������
void XM_VideoItemSetDelayCutOff (void);

int XM_VideoItemGetDelayCutOff (void);

// ����SD����Ż�ȡ����ľ�����
//		VolumeIndex		�洢�����(��0��ʼ)
// ����ֵ
//		NULL		ʧ��
//		�ǿ�ֵ	���ַ�����
const char *XM_VideoItemGetVolumeName (unsigned int VolumeIndex);

// ����ͨ���Ż�ȡ�������Ƶ��ͨ������
//		VideoChannel	��Ƶͨ�����(��0��ʼ)
//		FileType			�ļ�����(��Ƶ����Ƭ)
// ����ֵ
//		NULL		ʧ��
//		�ǿ�ֵ	ͨ���ַ�����
const char *XM_VideoItemGetChannelName (unsigned int VideoChannel, unsigned int FileType);


// ����ȫ·���ļ���
// "mmc:0:\\VIDEO_F\\20180318_121056_001_00P.AVI"
int XM_VideoItemMakeFullPathFileName (const char *VideoItemName,				// ��Ƶ������
											 unsigned int CardVolumeIndex,			// SD�������(��0��ʼ)
											 unsigned int VideoChannel,				// ͨ����(��0��ʼ),
											 unsigned int FileType,						// �ļ�����(��Ƶ����Ƭ)
											 unsigned int StreamClass,					// ¼������(��ͨ���ֶ�������)
											 char *lpFullPathName, int cbFullPathName	// ������Ƶ���ļ���ȫ·���ļ������漰ȫ·���ļ��������ֽڳ���
											);

// ����·����
int XM_VideoItemMakePathName (
											 unsigned int CardVolumeIndex,			// SD�������(��0��ʼ)
											 unsigned int VideoChannel,				// ͨ����(��0��ʼ)
											 char *lpFullPathName, int cbFullPathName	// ������Ƶ���ļ���ȫ·���ļ������漰ȫ·���ļ��������ֽڳ���
											);

// ������Ƶ�����ʱ�ļ� 
// VideoChannel ��Ƶ��ͨ����
//	lpVideoTempFileName  ������Ƶ����ʱ�ļ����ļ������ļ����ֽڳ���
// cbVideoTempFileName
char * XM_VideoItemCreateVideoTempFileEx (unsigned int VideoChannel,				// ¼��ͨ����
														unsigned int FileType, 					// �ļ�����(��Ƶ����Ƭ)
														unsigned int VideoItemClass,			// ��Ƶ�����(�������ֶ������)
														unsigned int CardVolumeIndex,			// SD�������(��0��ʼ)
														char* lpFileName, int cbFileName		// �����ļ������Ļ�������ַ���������ֽڴ�С
														);

// ������Ƶ��Ŀ����ļ��������, ���Ե���Ϊÿ����ÿ���ļ����ͼ�ÿ�ּ�¼���Ͷ����������
// ����ֵ
//		0		���óɹ�
//		-1		����ʧ��
int XM_VideoItemSetItemCountQuota (unsigned int volumn_index, 	// SD�������(��0��ʼ)
											  unsigned int file_type,		// �ļ�����(��Ƶ����Ƭ)
											  unsigned int stream_class, 	// ��Ƶ�����(�������ֶ������)
											  unsigned int quota				// ����Ŀ�����Ŀ�ļ�����, 0 ��ʾ������
											);

// ������Ƶ��Ĵ��̿ռ�ռ�����, Ϊ"ָ�����ϵ�ָ���ļ����͵�ָ����¼����"����ռ����
// ����ֵ
//		0		���óɹ�
//		-1		����ʧ��
int XM_VideoItemSetDiskSpaceQuotaLevelStreamClass (unsigned int volumn_index, 	// SD�������(��0��ʼ)
																	unsigned int file_type, 		// �ļ�����(��Ƶ����Ƭ)
																	unsigned int stream_class, 	// ��Ƶ�����(�������ֶ������)
																	XMINT64 quota						//	����Ĵ��̿ռ�ռ���ֽ�, 0��ʾ������
																	);

// ������Ƶ��Ĵ��̿ռ�ռ�����, Ϊ"ָ�����ϵ�ָ���ļ�����"(��������¼�Ʒ���)����ռ����
// ����ֵ
//		0		���óɹ�
//		-1		����ʧ��
int XM_VideoItemSetDiskSpaceQuotaLevelFileType (unsigned int volumn_index, 	// SD�������(��0��ʼ)
																unsigned int file_type,			// �ļ�����(��Ƶ����Ƭ)
																XMINT64 quota						//	����Ĵ��̿ռ�ռ���ֽ�, 0��ʾ������
																);

// ������Ƶ��Ĵ��̿ռ�ռ�����, Ϊ"ָ����"(���������ļ����ͣ�����¼�Ʒ���)����ռ����
// ����ֵ
//		0		���óɹ�
//		-1		����ʧ��
int XM_VideoItemSetDiskSpaceQuotaLevelDiskVolume (unsigned int volumn_index, 	// SD�������(��0��ʼ)
																  XMINT64 quota					//	����Ĵ��̿ռ�ռ���ֽ�, 0��ʾ������
																);


// ���ô��̱�����ֵ
// 	MinimumSpaceLimit(MB)		��������������ƶ���
//		RecycleSpaceLimit(MB)		�ɻ��տռ��������, С�ڸ�ֵ������( SYSTEM_EVENT_VIDEOITEM_LOW_SPACE )
// ����ֵ
//		0		���óɹ�
//		< 0	����ʧ��
int XM_VideoItemSetDiskSpaceLimit (unsigned int MinimumSpaceLimit, unsigned int RecycleSpaceLimit);

// ʹ��Ԥ��������������Ŀ(�ڴ��̾���ú����ã� �������ÿ���ʧ��)
// 	PredefinedQuotaProfile		ϵͳԤ�����profile
// ����ֵ
//		0		���óɹ�
//		< 0	����ʧ��
int XM_VideoItemSetQuotaProfile (unsigned int PredefinedQuotaProfile);

// ������Ƶ������(�����ԣ��ļ��������ԣ�ͨ���ż�¼�Ʒ����)��ȡ�������Ե���Ƶ�������
// ����ֵ
// -1 ��ʾ����
// >= 0 ��ʾ���صķ��϶������Ƶ�����
//		1)	��ȡ��0�ϵ�ǰ����ͷ����ͨ¼����Ƶ�ļ�����
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_0, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL);
//		2) ��ȡ��0�ϵ�����ͨ���Ľ���¼��(G-Sensor)��Ƶ�ļ�����
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_0, 0xFFFFFFFF,  XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		3) ��ȡ��1�ϵ�����ͨ���ı�����Ƶ(�ֶ�¼�ƣ�����¼��)�ļ�����
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_1, 0xFFFFFFFF, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_MANUAL|XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		4) ��ȡ��0�ϵĺ�����ͷ��������Ƭ�ļ�����
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_1, XM_FILE_TYPE_PHOTO, 0xFFFFFFFF);
int XM_VideoItemGetVideoItemCountEx (unsigned int volume_index, 				// SD�������(��0��ʼ), 0xFFFFFFFF��ʾ���о�
										  unsigned int video_channel, 			// ͨ���ţ� 0xFFFFFFFF��ʾ����ͨ��
										  unsigned int file_type, 					// �ļ�����(��Ƶ����Ƭ, 0xFFFFFFFF��ʾ�����ļ�����)
										  unsigned int stream_class_bitmask		// ¼��������룬0xFFFFFFFF��ʾ����¼��
												 											//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL ��ʾ������ͨ���ֶ�����
										);

// ���ݶ�������Ի�ȡָ������Ƶ����
//		0) ��ȡ��0�ϵĺ�����ͷ������������Ƭ(�������)����Ƶ����
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_1, XM_FILE_TYPE_PHOTO, 0xFFFFFFFF, 0�� 1 );
// ����ֵ
// == 0 ʧ��
// != 0 �������б����¼���
HANDLE XM_VideoItemGetVideoItemHandleEx (
											 unsigned int volume_index,			// ���̾��, 0xFFFFFFFF��ʾ���о�
											 unsigned int video_channel, 			// ͨ���ţ� 0xFFFFFFFF��ʾ����ͨ��
											 unsigned int file_type,				// �ļ�����, 0xFFFFFFFF��ʾ�����ļ�����
											 unsigned int stream_class_bitmask,	// ¼��������룬0xFFFFFFFF��ʾ����¼��
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL ��ʾ������ͨ���ֶ�����
											 unsigned int index,						// ��ţ���0��ʼ
											 unsigned int reverse_time_ordered	// ��ʱ�䷴������ 1 ��������(ʱ��Ӵ���С) 0 ˳������(ʱ���С����)
											);


// ���ݶ��������ɾ�������������Զ������Ƶ��
// ����ֵ
//		< 0		�쳣
//		0			���������˳�
//		1			�û���ֹ�˳�
int XM_VideoItemDeleteVideoItemList (
											 unsigned int volume_index,			// ���̾��, 0xFFFFFFFF��ʾ���о�
											 unsigned int video_channel, 			// ͨ���ţ� 0xFFFFFFFF��ʾ����ͨ��
											 unsigned int file_type,				// �ļ�����, 0xFFFFFFFF��ʾ�����ļ�����
											 unsigned int stream_class_bitmask,	// ¼��������룬0xFFFFFFFF��ʾ����¼��
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL ��ʾ������ͨ���ֶ�����
											 int (*fp_user_callback) (void *user_data, char *full_path_name_to_delete),
											 												// �û��ص������� �����û�ȡ��ĳ����ʱ�������Ϊ0��ʾ�޻ص�����
																							//		����ֵΪ1��ʾ��ֹ��ǰ�Ĳ������˳�������ֵΪ0��ʾ������ǰ�Ĳ���
																							//		user_data ������û�˽������
																							//		full_path_name_to_delete ׼��ɾ������Ƶ��ȫ·���ļ���
											 void *user_data								//	�û�˽������
											);													
												

// ����ȫ·��Ŀ¼��
// "mmc:0:\\VIDEO_F\\"
// "mmc:0:\\VIDEO_F\\NORMAL\\"
// "mmc:0:\\VIDEO_F\\URGENT\\"
// ����ֵ
//		< 0		ʧ��
int XM_VideoItemMakeFullPathName (unsigned int CardVolumeIndex,			// SD�������(��0��ʼ)
											 unsigned int VideoChannel,				// ͨ����(��0��ʼ)
											 unsigned int FileType,						// �ļ�����(VIDEO/PHOTO)
											 unsigned int StreamClass,					// ¼������(��ͨ���ֶ�������)
											 char *lpFullPathName, int cbFullPathName	// ������Ƶ���ļ���ȫ·���ļ������漰ȫ·���ļ��������ֽڳ���
											);

int XM_VideoItemMakePathNameEx (	 unsigned int CardVolumeIndex,			// SD�������(��0��ʼ)
											 unsigned int VideoChannel,				// ͨ����(��0��ʼ)
											 unsigned int FileType,						// �ļ�����(VIDEO/PHOTO)
											 char *lpFullPathName, int cbFullPathName	// ������Ƶ���ļ���ȫ·���ļ������漰ȫ·���ļ��������ֽڳ���
											);

// ������Ƶ������(�����ԣ��ļ��������ԣ�ͨ���ż�¼�Ʒ����)��ȡ�������Ե���Ƶ�������
//		1)	��ȡ��0�ϵ�ǰ����ͷ����ͨ¼����Ƶ�ļ�����
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_0, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL);
//		2) ��ȡ��0�ϵ�����ͨ���Ľ���¼��(G-Sensor)��Ƶ�ļ�����
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, 0xFFFFFFFF,  XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		3) ��ȡ��1�ϵ�����ͨ���ı�����Ƶ(�ֶ�¼�ƣ�����¼��)�ļ�����
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_1, 0xFFFFFFFF, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_MANUAL|XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		4) ��ȡ��0�ϵĺ�����ͷ��������Ƭ�ļ�����
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_1, XM_FILE_TYPE_PHOTO, 0xFFFFFFFF);
DWORD XM_VideoItemGetVideoItemCount (unsigned int volume_index, 				// SD�������(��0��ʼ), 0xFFFFFFFF��ʾ���о�
										  unsigned int video_channel, 			// ͨ���ţ� 0xFFFFFFFF��ʾ����ͨ��
										  unsigned int file_type, 					// �ļ�����(��Ƶ����Ƭ, 0xFFFFFFFF��ʾ�����ļ�����)
										  unsigned int stream_class_bitmask		// ¼��������룬0xFFFFFFFF��ʾ����¼��
												 											//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL ��ʾ������ͨ���ֶ�����
										);

#if defined (__cplusplus)
}
#endif		/* end of __cplusplus */

#endif	// _XM_VIDEO_ITEM_H_

