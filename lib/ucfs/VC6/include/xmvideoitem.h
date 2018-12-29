#ifndef _XM_VIDEO_ITEM_H_
#define _XM_VIDEO_ITEM_H_

#include <xm_proj_define.h>
#include <xmtype.h>
#include <xmbase.h>
#include <xmfile.h>

#if defined (__cplusplus)
extern "C"{
#endif

#if _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_AVI_
#define	XM_VIDEO_FILE_EXT		"AVI"
#elif _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_MKV_
#define	XM_VIDEO_FILE_EXT		"MKV"
#elif _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_MOV_
#define	XM_VIDEO_FILE_EXT		"MOV"
#endif

// ��Ƶͨ�����ڱ�־
#define	XM_VIDEO_CH_0		0x01
#define	XM_VIDEO_CH_1		0x02
#define	XM_VIDEO_CH_2		0x04
#define	XM_VIDEO_CH_3		0x08

// ��Ƶ�б�ģʽ����(ѭ��������)
#define	VIDEOITEM_CIRCULAR_VIDEO_MODE			0		// ��ѭ��¼���б�ģʽ
#define	VIDEOITEM_PROTECT_VIDEO_MODE			1		// ������¼���б�ģʽ

// �����Ƶ�ļ�ȫ·�������� (��������0)
#define	VIDEOITEM_MAX_FILE_NAME					32		

#define	VIDEOITEM_MAX_NAME						16		// ��Ƶ�ļ�������ֽڳ���

typedef struct _XMVIDEOITEM {
	unsigned char		video_name[VIDEOITEM_MAX_NAME];	// ��Ƶ�ļ���Ϊ16���ֽ�
	unsigned char		video_channel;		// ͨ�����ڱ�־��ÿ��ͨ��ʹ��һλ��ʾ������ʾ8��ͨ��	
	unsigned char		video_lock;			// �ļ����� 
														// ֻ����ʾ���� ������һ������ͨ��ֻ������ʾΪ����������
	unsigned char		video_update;		// ����´�������Ƶ�ļ��Ƿ��ѹرղ�����
	
	XMSYSTEMTIME		video_create_time[8];		// ��Ƶ��ÿ��ͨ���Ĵ���ʱ��

	XMINT64				video_size;			// ��Ƶ�ļ��ֽڴ�С(����ͨ���ۼ�)
	
	unsigned int		video_ref_count;	// ��Ƶ�����ü���
} XMVIDEOITEM;

// ����ṹ�壬����ɾ����Ƶ���Ӧ�Ĵ����ļ�
typedef struct tagVIDEOFILETOREMOVE {
	int		count;							// ɾ���ļ�����
	int		volume_index[8];						// ���̾�����
	char		file_name[8][VIDEOITEM_MAX_FILE_NAME];		// ���8��ͨ��
//	unsigned char	type[8];					// ɾ���ļ�������
//	unsigned char	channel[8];				// ɾ���ļ���ͨ��	
} VIDEOFILETOREMOVE;

int XM_VideoItemInit (unsigned int video_channel_count, const char **video_channel_file_path);
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

// ��ȡ��Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ��Ŀ���
// dwVideoIndex ��0��ʼ
HANDLE AP_VideoItemGetVideoItemHandle (BYTE mode, DWORD dwVideoIndex);

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

// ����һ���µ���Ƶ����
HANDLE XM_VideoItemCreateVideoItemHandle (unsigned int VideoChannel, XMSYSTEMTIME *filetime);

// ɾ��һ����Ƶ����(����ɾ��������ͨ������Ƶ�ļ�)
XMBOOL XM_VideoItemDeleteVideoItemHandle (HANDLE hVideoItemHandle, VIDEOFILETOREMOVE *lpVideoFileToRemove);
XMBOOL XM_VideoItemDeleteVideoItemHandleFromFileName (const char *lpVideoFile);


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


// ��ȡ��Ƶ�����ݿ�����Ƶ��ĸ���
// video_channel ��Ƶ��ͨ��
// ����ֵ
// -1 ��ʾ����
// 0 ��ʾ����Ƶ��
// > 0 ��ʾ�Ѽ�¼����Ƶ�����
int XM_VideoItemGetVideoItemCount (unsigned int video_channel);

// ��ȡָ��¼��ͨ�����ļ��б�
// video_channel ��Ƶ��ͨ��
// index ��ȡ�ļ�¼��ʼ���
// count ��ȡ�ļ�¼����(������ʼ���)
// packet_addr ����������ַ
// packet_size ���������ֽڴ�С
// ����ֵ
// < 0 ʧ��
// = 0 
int XM_VideoItemGetVideoItemList (unsigned int video_channel, 
											 unsigned int index, 
											 unsigned int count,
											 void *packet_addr, 
											 int packet_size
											);

void combine_fullpath_media_filename (unsigned char full_path_file_name[32], 
												 unsigned char ch, 
												 unsigned char type, 
												 const unsigned char name[8]);
// 0 �ɹ�����
// -1 ����ʧ��
int extract_fullpath_media_filename (const unsigned char full_path_file_name[32], 
												 unsigned char* ch, 
												 unsigned char* type, 
												 unsigned char name[8]);

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
void XM_MakeFileNameFromCurrentDateTime (XMSYSTEMTIME *filetime, char filename[8]);

// ������Ƶ�����ʱ�ļ�
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

// ��ȡ��Ƶ�ļ�ͨ��(·��)�ĸ���
int XM_VideoItemGetVideoFileChannel (void);

#if defined (__cplusplus)
}
#endif		/* end of __cplusplus */

#endif	// _XM_VIDEO_ITEM_H_