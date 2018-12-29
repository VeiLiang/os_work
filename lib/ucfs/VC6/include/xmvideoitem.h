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

// 视频通道存在标志
#define	XM_VIDEO_CH_0		0x01
#define	XM_VIDEO_CH_1		0x02
#define	XM_VIDEO_CH_2		0x04
#define	XM_VIDEO_CH_3		0x08

// 视频列表模式定义(循环、保护)
#define	VIDEOITEM_CIRCULAR_VIDEO_MODE			0		// “循环录像”列表模式
#define	VIDEOITEM_PROTECT_VIDEO_MODE			1		// “保护录像”列表模式

// 最大视频文件全路径名长度 (包含结束0)
#define	VIDEOITEM_MAX_FILE_NAME					32		

#define	VIDEOITEM_MAX_NAME						16		// 视频文件名最大字节长度

typedef struct _XMVIDEOITEM {
	unsigned char		video_name[VIDEOITEM_MAX_NAME];	// 视频文件名为16个字节
	unsigned char		video_channel;		// 通道存在标志，每个通道使用一位表示。最多表示8个通道	
	unsigned char		video_lock;			// 文件属性 
														// 只读表示锁定 （任意一个或多个通道只读均表示为锁定保护）
	unsigned char		video_update;		// 标记新创建的视频文件是否已关闭并更新
	
	XMSYSTEMTIME		video_create_time[8];		// 视频项每个通道的创建时间

	XMINT64				video_size;			// 视频文件字节大小(所有通道累加)
	
	unsigned int		video_ref_count;	// 视频项引用计数
} XMVIDEOITEM;

// 定义结构体，保存删除视频项对应的磁盘文件
typedef struct tagVIDEOFILETOREMOVE {
	int		count;							// 删除文件个数
	int		volume_index[8];						// 磁盘卷索引
	char		file_name[8][VIDEOITEM_MAX_FILE_NAME];		// 最多8个通道
//	unsigned char	type[8];					// 删除文件的类型
//	unsigned char	channel[8];				// 删除文件的通道	
} VIDEOFILETOREMOVE;

int XM_VideoItemInit (unsigned int video_channel_count, const char **video_channel_file_path);
void XM_VideoItemExit (void);

// 开机或插入SD卡后重新扫描所有视频文件, 开启视频项服务
// 0  视频项服务开启成功
// -1 视频项服务开启失败(磁盘卷或文件路径不存在)
// -2 视频项服务开启失败(内存分配异常)
int XM_VideoItemOpenService (void);

// 拔出SD卡后, 关闭视频项服务
void XM_VideoItemCloseService (void);

// 获取视频列表文件个数
// mode 指定循环录像模式还是保护录像模式
DWORD AP_VideoItemGetVideoFileCount (BYTE mode);

// 获取视频列表(循环或保护视频列表)中指定序号的视频项目句柄
// dwVideoIndex 从0开始
HANDLE AP_VideoItemGetVideoItemHandle (BYTE mode, DWORD dwVideoIndex);

// 从视频项句柄获取视频项结构指针
XMVIDEOITEM * AP_VideoItemGetVideoItemFromHandle (HANDLE hVideoItemHandle);

// 获取视频列表(循环或保护视频列表)中指定序号的视频项目
// dwVideoIndex 从0开始
XMVIDEOITEM *AP_VideoItemGetVideoItem (BYTE mode, DWORD dwVideoIndex);

// 获取前一个视频索引号, -1表示不存在
DWORD AP_VideoItemGetPrevVideoIndex (BYTE mode, DWORD dwVideoIndex);

// 获取下一个视频索引号, -1表示不存在
DWORD AP_VideoItemGetNextVideoIndex (BYTE mode, DWORD dwVideoIndex);

// 显示格式化指定序号的视频文件名
char *AP_VideoItemFormatVideoFileName (XMVIDEOITEM *lpVideoItem, char *lpDisplayName, int cbDisplayName);

// 将视频列表(循环或保护视频列表)中指定序号的视频文件锁定(保护) (保护所有通道)
XMBOOL AP_VideoItemLockVideoFile (XMVIDEOITEM *lpVideoItem);

// 将指定文件名的视频锁定
XMBOOL AP_VideoItemLockVideoFileFromFileName (const char *lpVideoFile);

// 将视频列表(循环或保护视频列表)中指定序号的视频文件解锁(可循环覆盖)
XMBOOL AP_VideoItemUnlockVideoFile (XMVIDEOITEM *lpVideoItem);
XMBOOL AP_VideoItemUnlockVideoFileFromFileName (const char *lpVideoFile);

// 将指定文件名的视频标记使用中，防止删除操作将视频项删除
XMBOOL XM_VideoItemMarkVideoFileUsage (const char *lpVideoFile);
// 将指定文件名的视频标记未使用
XMBOOL XM_VideoItemMarkVideoFileUnuse (const char *lpVideoFile);

// 创建一个新的视频项句柄
HANDLE XM_VideoItemCreateVideoItemHandle (unsigned int VideoChannel, XMSYSTEMTIME *filetime);

// 删除一个视频项句柄(包括删除其所有通道的视频文件)
XMBOOL XM_VideoItemDeleteVideoItemHandle (HANDLE hVideoItemHandle, VIDEOFILETOREMOVE *lpVideoFileToRemove);
XMBOOL XM_VideoItemDeleteVideoItemHandleFromFileName (const char *lpVideoFile);


// 获取视频项指定通道的全路径文件名
XMBOOL XM_VideoItemGetVideoFilePath (XMVIDEOITEM *lpVideoItem, unsigned int nVideoChannel, char *lpFileName, int cbFileName);

// 获取视频流的时间长度信息
DWORD AP_VideoItemGetVideoStreamSize (XMVIDEOITEM *lpVideoItem, unsigned int nVideoChannel);

// 通过全路径文件名查找视频项句柄及通道号
HANDLE XM_VideoItemGetVideoItemHandle (const char *lpVideoFile, int* pVideoChannel);

// 指定通道的视频文件已创建/写入并关闭，需更新其对应的视频句柄项中的信息
XMBOOL XM_VideoItemUpdateVideItemHandle (HANDLE hVideoItemHandle, int nVideoChannel, unsigned int cbVideoFileSize);

XMBOOL XM_VideoItemUpdateVideoItemFromFileName (const char *lpVideoFileName, unsigned int cbVideoFileSize, XMSYSTEMTIME *create_time);

// 检查视频项句柄是否已更新(已关闭对应的视频文件)
// 通过文件名查找而不是使用句柄，句柄可能因为文件被删除而无效
XMBOOL XM_VideoItemWaitingForUpdate (const char *lpVideoFileName);

// 监控并释放可循环记录项，可录时间提示, 可录时间报警
// bReportEstimatedRecordingTime 1 可录时间检查并提示
VOID XM_VideoItemMonitorAndRecycleGarbage (XMBOOL bReportEstimatedRecordingTime);


// 创建/删除互斥信号量
VOID  XM_VideoItemCreateResourceMutexSemaphore (VOID);
VOID	XM_VideoItemDeleteResourceMutexSemaphore (VOID);

VOID	XM_VideoItemLock (VOID);
VOID	XM_VideoItemUnlock (VOID);

// 视频项服务等待"编码器启动事件"
// 返回值 
//		0	 编码器启动事件在指定时间内被设置
//    -1	 编码器启动事件在指定时间内没有被设置
int XM_CodecStartEventWait (unsigned int milliseconds_to_timeout);		// 等待编码器启动事件
// 清除"编码器启动事件"
void XM_CodecStartEventReset (void);
// "外部编码器任务"调用该函数通知"视频项服务"编码器已启动编码过程
void XM_CodecStartEventSet (void);			// 设置编码器启动事件


// 读取视频项数据库中视频项的个数
// video_channel 视频项通道
// 返回值
// -1 表示错误
// 0 表示无视频项
// > 0 表示已记录的视频项个数
int XM_VideoItemGetVideoItemCount (unsigned int video_channel);

// 读取指定录像通道的文件列表
// video_channel 视频项通道
// index 读取的记录开始序号
// count 读取的记录条数(包含开始序号)
// packet_addr 包缓冲区地址
// packet_size 包缓冲区字节大小
// 返回值
// < 0 失败
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
// 0 成功解析
// -1 解析失败
int extract_fullpath_media_filename (const unsigned char full_path_file_name[32], 
												 unsigned char* ch, 
												 unsigned char* type, 
												 unsigned char name[8]);

// 返回值定义
//	0	已获取卷信息
// -1	无法获取卷信息
int XM_VideoItemGetDiskUsage (XMINT64 *TotalSpace, 		// SD卡空间
										XMINT64 *LockedSpace, 		// 已锁定的文件空间
										XMINT64 *RecycleSpace,		// 可循环使用文件 + 空闲空间
										XMINT64 *OtherSpace			// 其他文件(系统)占用空间
									);

// 文件删除
// 返回值定义
// 0 删除失败
// 1 删除成功
int XMSYS_remove_file (const char *filename);

// 检查视频项服务是否已开启
// 1 视频项服务已开启
// 0 视频项服务未开启
XMBOOL XM_VideoItemIsServiceOpened (void);

// YMDHMMSS
// Y  0 -- > 2014
//    Z -- > 2014 + 9 + 26 = 2049
// M  1 -- >  1月
//    C -- > 12月
// D  1 -- >  1号
//    V -- > 31号
// H  0 -- >  0点
//    M -- > 23点
// MM 0 -- >  0分
//    59-- > 59分
// SS 0 -- >  0秒
//    59-- > 59秒
void XM_MakeFileNameFromCurrentDateTime (XMSYSTEMTIME *filetime, char filename[8]);

// 创建视频项的临时文件
// VideoChannel 视频项通道号
//	lpVideoTempFileName  保存视频项临时文件的文件名及文件名字节长度
// cbVideoTempFileName
char * XM_VideoItemCreateVideoTempFile (unsigned int nVideoChannel, char *lpFileName, int cbFileName);

// 通过全路径的临时视频项文件名查找对应的视频通道号
XMBOOL XM_VideoItemGetVideoChannelFromTempVideoItemName (const char *lpVideoFile, int* pVideoChannel);

// 检查是否是临时视频项文件
XMBOOL XM_VideoItemIsTempVideoItemFile (const char *lpVideoFileName);

XMBOOL XM_VideoItemUpdateVideoItemFromTempName (const char *lpVideoFileName, unsigned int cbVideoFileSize, 
																XMSYSTEMTIME *create_time);

// 检查视频项基本服务是否已开启。若开启，则允许视频记录
XMBOOL XM_VideoItemIsBasicServiceOpened (void);

// bPostFileListUpdateMessage 是否投递视频项文件列表更新消息
XMBOOL XM_VideoItemDeleteVideoItemHandleFromFileNameEx (const char *lpVideoFileName, int bPostFileListUpdateMessage);

// 获取视频文件通道(路径)的个数
int XM_VideoItemGetVideoFileChannel (void);

#if defined (__cplusplus)
}
#endif		/* end of __cplusplus */

#endif	// _XM_VIDEO_ITEM_H_