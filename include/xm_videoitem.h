//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_videoitem.c
//	  视频列表接口
//
//	Revision history
//
//		2011.12.16	ZhuoYongHong Initial version
//		2018.04.30	ZhuoYongHong 改为使用长文件名命名视频项文件
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
	XM_FILE_TYPE_VIDEO =	0,			// 视频
	XM_FILE_TYPE_PHOTO,				// 照片
	XM_FILE_TYPE_COUNT
};

enum {
	XM_VIDEOITEM_CLASS_NORMAL = 0,		// 普通录制类型
	XM_VIDEOITEM_CLASS_MANUAL,			// 手动录制类型
	XM_VIDEOITEM_CLASS_URGENT,			// 紧急录制类型(G-Sensor触发)
	XM_VIDEOITEM_CLASS_COUNT	
};

enum {
	XM_VIDEOITEM_CLASS_BITMASK_NORMAL = (1 << XM_VIDEOITEM_CLASS_NORMAL),
	XM_VIDEOITEM_CLASS_BITMASK_MANUAL = (1 << XM_VIDEOITEM_CLASS_MANUAL),
	XM_VIDEOITEM_CLASS_BITMASK_URGENT = (1 << XM_VIDEOITEM_CLASS_URGENT),
	XM_VIDEOITEM_CLASS_BITMASK_ALL = 0xFFFFFFFF
};

enum {
	XM_VIDEO_CHANNEL_0 = 0,			// 通道0 --> 前置摄像头
	XM_VIDEO_CHANNEL_1,				// 通道1 --> 后置摄像头
	XM_VIDEO_CHANNEL_2,				// 通道2 --> 左侧摄像头
	XM_VIDEO_CHANNEL_3,				// 通道3 --> 右侧摄像头
	XM_VIDEO_CHANNEL_COUNT			// 通道数量
};

enum {
	XM_VIDEOITEM_VOLUME_0 = 0,
	//XM_VIDEOITEM_VOLUME_1,		
	XM_VIDEOITEM_VOLUME_COUNT		// 卷数量
};

// 排序方法
enum {
	XM_VIDEOITEM_ORDERING_NAME = 0,			// 缺省名称排序
	XM_VIDEOITEM_ORDERING_SERIAL_NUMBER,	// 以序号排序
	XM_VIDEOITEM_ORDERING_COUNT
};

// 预定义的配额配置项目
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

// 视频通道存在标志
#define	XM_VIDEO_CH_0		0x01
#define	XM_VIDEO_CH_1		0x02
#define	XM_VIDEO_CH_2		0x04
#define	XM_VIDEO_CH_3		0x08

// 视频列表模式定义(循环、保护)
#define	VIDEOITEM_CIRCULAR_VIDEO_MODE			0		// “循环录像”列表模式
#define	VIDEOITEM_PROTECT_VIDEO_MODE			1		// “保护录像”列表模式

#define	VIDEOITEM_DRIVER_NAME_SIZE			

// 最大视频文件全路径名长度 (包含结束0)
#define	VIDEOITEM_MAX_FULL_PATH_NAME			64		// 包括卷名称的全路径命名
#define	VIDEOITEM_MAX_FILE_NAME					VIDEOITEM_MAX_FULL_PATH_NAME		

#define	TEMPITEM_MAX_FILE_NAME					16		// TEMPNNNN_00P.AVI


#define	VIDEOITEM_LFN_SIZE						28		// 20180318_121056_00000000_00N		// 保存在卡0上的通道0的序号为0的手动保护视频项(手动与普通视频通过只读属性区分)
																	//	20180318_121056_00000000_11N		// 保存在卡1上的通道1的序号为0的普通视频项
																	//	20180318_121056_00000001_00U		// 保存在卡0上的通道0的序号为1的紧急视频项

#define	VIDEOITEM_MAX_NAME						(VIDEOITEM_LFN_SIZE)		// 视频文件名最大字节长度


#define	VIDEOITEM_MAX_FULL_NAME					(VIDEOITEM_LFN_SIZE + 4)	// 包含文件扩展名的视频项文件命名

typedef struct _XMVIDEOITEM {
	unsigned char		video_name[VIDEOITEM_MAX_FULL_NAME + 1];	// 视频项文件命名(20180318_121056_001_00P.AVI, 20180318_121056_001_11N.JPG)
	unsigned char		video_channel;				// 通道存在标志，每个通道使用一位表示。最多表示8个通道	
	unsigned char		video_lock;					// 文件属性 
													// 只读表示锁定 （任意一个或多个通道只读均表示为锁定保护）
	unsigned char		video_update;				// 标记新创建的视频文件是否已关闭并更新
	unsigned char		video_ref_count;			// 视频项引用计数. 当视频被引用时(如正在播放), 防止删除操作
	
	XMSYSTEMTIME		video_create_time;			// 视频项每个通道的创建时间
	
	unsigned char		volume_index;				// SD卡序号
	unsigned char		file_type;					// 文件类型
	unsigned char		stream_class;				// 流类型(普通、手动或紧急)
	
	unsigned char		initial_class;				// 初始创建时的流类型
	
	unsigned char		stream_length_valid;		// 流时间长度是否有效
	unsigned int		stream_length;				// 流时间长度(毫秒),录像时长

	unsigned int		file_size;					// 流文件字节长度
	XMINT64				video_size;					// 视频文件字节大小(所有通道累加)
	unsigned int		serial_number;				// 序号
	
} XMVIDEOITEM;

// 定义结构体，保存删除视频项对应的磁盘文件
typedef struct tagVIDEOFILETOREMOVE {
	int		count;							// 删除文件个数
	int		volume_index[XM_VIDEO_CHANNEL_COUNT];						// 磁盘卷索引
	char		file_name[XM_VIDEO_CHANNEL_COUNT][VIDEOITEM_MAX_FILE_NAME];		// 最多8个通道
//	unsigned char	type[8];					// 删除文件的类型
	unsigned char	channel[XM_VIDEO_CHANNEL_COUNT];				// 删除文件的通道	
} VIDEOFILETOREMOVE;

int XM_VideoItemInit (unsigned int video_channel_count, const char **video_channel_file_path);
int XM_VideoItemInitEx (unsigned int video_volume_count, unsigned int video_channel_count, const char **video_channel_file_path);

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

// 获取视频列表(循环或保护视频列表)中指定序号的视频项目句柄(按时间从小到大)
// dwVideoIndex 从0开始
HANDLE AP_VideoItemGetVideoItemHandle (BYTE mode, DWORD dwVideoIndex);

// 获取视频列表(循环或保护视频列表)中指定序号的视频项目句柄 (按时间从大到小)
// dwVideoIndex 从0开始
HANDLE AP_VideoItemGetVideoItemHandleReverseOrder (BYTE mode, DWORD dwVideoIndex);


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

// 创建一个新的视频项句柄 (已废弃，请使用XM_VideoItemCreateVideoItemHandleEx)
//   (缺省为视频文件，普通分类，卷0)
// nVideoChannel = 0, 1, 2, 3 ... 表示创建一个指定通道的视频项
HANDLE XM_VideoItemCreateVideoItemHandle (unsigned int VideoChannel, XMSYSTEMTIME *filetime);

// 删除一个视频项句柄(包括删除其所有通道的视频文件)
XMBOOL XM_VideoItemDeleteVideoItemHandle (HANDLE hVideoItemHandle, VIDEOFILETOREMOVE *lpVideoFileToRemove);
XMBOOL XM_VideoItemDeleteVideoItemHandleFromFileName (const char *lpVideoFile);

// 获取简单视频项(只包含1路视频)的通道序号
// >= 0  返回的通道序号
// < 0   无效的句柄或者视频项句柄是一个复合视频项(多个通道的组合)
int XM_VideoItemGetVideoChannel (HANDLE hVideoItemHandle);

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

// 根据定义的属性读取视频项指定属性的文件列表
// video_channel 视频项通道
// index 读取的记录开始序号
// count 读取的记录条数(包含开始序号)
// packet_addr 包缓冲区地址
// packet_size 包缓冲区字节大小
// 返回值
// < 0 失败
// >= 0 读出的列表项纪录数据的字节长度 
int XM_VideoItemGetVideoItemListEx (unsigned int video_channel, 		// 通道号， 0xFFFFFFFF表示所有通道
											 unsigned int volume_index,			// 磁盘卷号, 0xFFFFFFFF表示所有卷
											 unsigned int file_type,				// 文件类型, 0xFFFFFFFF表示所有文件类型
											 unsigned int stream_class_bitmask,	// 录像分类掩码，0xFFFFFFFF表示所有录像
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL 表示包含普通及手动分类
											 unsigned int index, 
											 unsigned int count,
											 void *packet_addr, 
											 int packet_size
											);

// 读取指定录像通道的文件列表
// video_channel 视频项通道
// index 读取的记录开始序号
// count 读取的记录条数(包含开始序号)
// packet_addr 包缓冲区地址
// packet_size 包缓冲区字节大小
// 返回值
// < 0 失败
// >= 0 读出的列表项纪录数据的字节长度 
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
// 0 成功解析
// -1 解析失败
int extract_fullpath_media_filename (const unsigned char* full_path_file_name, 
												 unsigned char* ch, 
												 unsigned char* type, 
												 unsigned char* name, int cb_name);

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

// 根据视频项文件的创建时间命名文件名
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
int XM_MakeFileNameFromCurrentDateTime (XMSYSTEMTIME *filetime, char* filename, int cb_filename);


// 根据视频项属性(创建时间，录像通道号，文件类型，视频项分类等)创建文件命名
// 20180411_115623_000_00P.JPG		保存到卡0的通道0的手动保护的照片	
// 20180411_115623_000_11N.AVI		保存到卡1的通道1的可循环覆盖的普通类型视频文件
// 20180411_115623_000_01U.AVI		保存到卡1的通道0的紧急类型的视频文件，由G-Sensor触发
// 20180411_115623_000_10U.JPG		保存到卡0的通道1的紧急类型的照片文件，由G-Sensor触发
// 返回值
//	 0      成功
//  -1     失败
int XM_MakeFileNameFromCurrentDateTimeEx (XMSYSTEMTIME *lpCreateTime, 			// 文件创建时间， 为空表示使用当前时间
														unsigned int VideoChannel,				// 录像通道号
														unsigned int FileType, 					// 文件类型(视频或照片)
														unsigned int VideoItemClass,			// 视频项分类(正常、手动或紧急)
														unsigned int CardVolumeIndex,			// SD卡卷序号(从0开始)
														unsigned int SerialNumber,				// 序号
														char* lpFileName, int cbFileName		// 保存文件命名的缓冲区地址及缓冲区字节大小
														);

// 根据视频项属性(通道号，创建时间，文件类型，视频项分类，保存的卷序号)创建一个视频项句柄
//		1) 创建一个位于卡0上的通道0的紧急录制类型的录像视频项
//			XM_VideoItemCreateVideoItemHandleEx (XM_VIDEO_CHANNEL_0, create_time, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_URGENT, XM_VIDEOITEM_VOLUME_0);
//		2) 创建一个位于卡0上的通道1的普通录制类型的照片视频项
//			XM_VideoItemCreateVideoItemHandleEx (XM_VIDEO_CHANNEL_1, create_time, XM_FILE_TYPE_PHOTO, XM_VIDEOITEM_CLASS_NORMAL, XM_VIDEOITEM_VOLUME_0);
//		3) 创建一个位于卡1上的通道1的手动录制类型的视频视频项
//			XM_VideoItemCreateVideoItemHandleEx (XM_VIDEO_CHANNEL_1, create_time, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_MANUAL, XM_VIDEOITEM_VOLUME_1);
// 返回值
//		0			失败
//		非0值		视频项句柄
HANDLE XM_VideoItemCreateVideoItemHandleEx (unsigned int video_channel,		// 通道号
														  XMSYSTEMTIME *create_time, 		// 视频项创建时间
														  unsigned int file_type,			//	视频项文件类型(视频或照片)	
														  unsigned int videoitem_class, 	// 视频项类型(正常、手动或者紧急)
														  unsigned int video_volume_index	// 保存的SD卡卷序号(从0开始)
														  );

// 直接创建视频项的全路径文件名
// 返回值
//	  NULL		创建失败
//	  !=NULL		全路径文件名
char * XM_VideoItemCreateFullPathFileName (XMSYSTEMTIME *lpCreateTime, 			// 文件创建时间， 为空表示使用当前时间
													 unsigned int VideoChannel, 			// 录像通道号
													 unsigned int FileType, 				// 文件类型(视频或照片)
													 unsigned int VideoItemClass,			// 视频项分类(正常、手动或紧急)
													 unsigned int CardVolumeIndex,		// SD卡卷序号(从0开始)
													 char *lpFileName, int cbFileName	// 保存视频项临时文件的文件名及文件名字节长度
													);



// 从视频项文件命名恢复文件创建的时间
void XM_GetDateTimeFromFileName (char* filename, XMSYSTEMTIME *filetime);


// 创建视频项的临时文件 (使用缺省的card 0，普通录制类型，使用当前系统时间，视频类型)
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

// 添加一个新的视频文件
XMBOOL XM_VideoItemAppendVideoFile (unsigned int ch, const char *lpVideoFile);


// 获取视频项通道对应的文件路径
const char * XM_VideoItemGetVideoPath (unsigned int nVideoChannel);
const char * XM_VideoItemGetVideoPathByType(unsigned int nVideoChannel, unsigned char type);

// 获取视频文件通道(路径)的个数
int XM_VideoItemGetVideoFileChannel (void);


// 读取指定照片通道的文件列表
// image_channel 照片项通道
// index 读取的记录开始序号
// count 读取的记录条数(包含开始序号)
// packet_addr 包缓冲区地址
// packet_size 包缓冲区字节大小
// 返回值
// < 0 失败
// = 0 读出的文件数为0
// > 0 读出的有效文件数
int XM_ImageItemGetImageItemList (unsigned int image_channel, 
											 unsigned int index, 
											 unsigned int count,
											 void *packet_addr, 
											 int packet_size
											);

// 检查卡的视频写入性能
// 返回值
//		-1		无法获取性能值
//		0		视频性能可能较差
//		1		视频性能满足记录要求
int XM_VideoItemCheckCardPerformance (void);

// 检查卡的可用循环录像空间容量(= 磁盘空闲字节容量 + 循环录像视频占用的文件字节容量)是否满足最低的录像要求
// 返回值
//		-1		无法获取该性能参数
//		0		无法满足最低录像要求, 提示格式化卡
//		1		满足最低录像要求
int XM_VideoItemCheckCardRecycleSpace (void);

// 检查指定卡的可用循环录像空间容量(= 磁盘空闲字节容量 + 循环录像视频占用的文件字节容量)是否满足最低的录像要求
// 返回值
//		-1		无法获取该性能参数
//		0		无法满足最低录像要求, 提示格式化卡
//		1		满足最低录像要求
int XM_VideoItemCheckCardRecycleSpaceEx (unsigned int volume_index);


// 判断是否是符合标准命名的照片文件
int is_valid_image_filename (char *file_name);

// 判断是否是符合标准命名的视频文件
int is_valid_video_filename (char *file_name);

// 读取指定照片通道的文件列表, 按照视频列表格式组织
// image_channel 照片项通道
// index 读取的记录开始序号
// count 读取的记录条数(包含开始序号)
// packet_addr 包缓冲区地址
// packet_size 包缓冲区字节大小
// 返回值
// < 0 失败
// = 0 读出的文件数为0
// > 0 读出的有效文件数
int XM_VideoItemGetImageItemList (unsigned int image_channel, 
											 unsigned int index, 
											 unsigned int count,
											 void *packet_addr, 
											 int packet_size
											);

// 打开卷服务
// 0  命令成功执行
// -1 命令成功失败
int xm_open_volume_service (char *volume_name, int b_post_insert_message);

// 关闭卷服务
// 0  命令成功执行
// -1 命令成功失败
int xm_close_volume_service (char *volume_name);

// 获取视频列表文件个数
// mode 指定循环录像模式还是保护录像模式
// nVideoChannel	指定视频通道, 0 前置摄像头 1 后置摄像头
DWORD AP_VideoItemGetVideoFileCountEx (BYTE mode, unsigned int nVideoChannel);

// 获取视频列表(循环或保护视频列表)中指定序号的视频项目句柄 (按时间从大到小)
// dwVideoIndex 从0开始
// nVideoChannel 视频通道号 0 前置摄像头 1 后置摄像头
HANDLE AP_VideoItemGetVideoItemHandleReverseOrderEx (BYTE mode, DWORD dwVideoIndex, unsigned int nVideoChannel);

// 获取视频列表(循环或保护视频列表)中指定序号的视频项目句柄(按时间从小到大)
// dwVideoIndex 从0开始
// nVideoChannel 视频通道号 0 前置摄像头 1 后置摄像头
HANDLE AP_VideoItemGetVideoItemHandleEx (BYTE mode, DWORD dwVideoIndex, unsigned int nVideoChannel);


// 将指定的视频文件(全路径名)删除至回收站
// 1 成功
// 0 失败
XMBOOL XM_VideoItemDeleteVideoFileIntoRecycleBin (const char *lpVideoFileName);

// 将视频项文件添加到视频项数据库
// 	ch					通道编号(0, 1, ...)
// 	lpVideoFile		全路径视频文件名
// 返回值
//	1		成功
//	0		失败
XMBOOL XM_VideoItemAppendVideoFile (unsigned int ch, const char *lpVideoFile);

// 检查磁盘剩余空间是否无法满足实时录像需求
// 返回值
// 1   无法满足实时录像需求
// 0   可以满足实时录像需求
XMBOOL XM_VideoItemCheckDiskFreeSpaceBusy (void);


// 20170929
// 延时现在的文件删除操作
void XM_VideoItemSetDelayCutOff (void);

int XM_VideoItemGetDelayCutOff (void);

// 根据SD卡编号获取定义的卷名称
//		VolumeIndex		存储卡编号(从0开始)
// 返回值
//		NULL		失败
//		非空值	卷字符串名
const char *XM_VideoItemGetVolumeName (unsigned int VolumeIndex);

// 根据通道号获取定义的视频项通道名称
//		VideoChannel	视频通道编号(从0开始)
//		FileType			文件类型(视频或照片)
// 返回值
//		NULL		失败
//		非空值	通道字符串名
const char *XM_VideoItemGetChannelName (unsigned int VideoChannel, unsigned int FileType);


// 计算全路径文件名
// "mmc:0:\\VIDEO_F\\20180318_121056_001_00P.AVI"
int XM_VideoItemMakeFullPathFileName (const char *VideoItemName,				// 视频项命名
											 unsigned int CardVolumeIndex,			// SD卡卷序号(从0开始)
											 unsigned int VideoChannel,				// 通道号(从0开始),
											 unsigned int FileType,						// 文件类型(视频或照片)
											 unsigned int StreamClass,					// 录制类型(普通，手动，紧急)
											 char *lpFullPathName, int cbFullPathName	// 保存视频项文件的全路径文件名缓存及全路径文件名缓存字节长度
											);

// 创建路径名
int XM_VideoItemMakePathName (
											 unsigned int CardVolumeIndex,			// SD卡卷序号(从0开始)
											 unsigned int VideoChannel,				// 通道号(从0开始)
											 char *lpFullPathName, int cbFullPathName	// 保存视频项文件的全路径文件名缓存及全路径文件名缓存字节长度
											);

// 创建视频项的临时文件 
// VideoChannel 视频项通道号
//	lpVideoTempFileName  保存视频项临时文件的文件名及文件名字节长度
// cbVideoTempFileName
char * XM_VideoItemCreateVideoTempFileEx (unsigned int VideoChannel,				// 录像通道号
														unsigned int FileType, 					// 文件类型(视频或照片)
														unsigned int VideoItemClass,			// 视频项分类(正常、手动或紧急)
														unsigned int CardVolumeIndex,			// SD卡卷序号(从0开始)
														char* lpFileName, int cbFileName		// 保存文件命名的缓冲区地址及缓冲区字节大小
														);

// 设置视频项的可用文件数量配额, 可以单独为每个卷，每种文件类型及每种记录类型定义数量配额
// 返回值
//		0		设置成功
//		-1		设置失败
int XM_VideoItemSetItemCountQuota (unsigned int volumn_index, 	// SD卡卷序号(从0开始)
											  unsigned int file_type,		// 文件类型(视频或照片)
											  unsigned int stream_class, 	// 视频项分类(正常、手动或紧急)
											  unsigned int quota				// 允许的可用项目文件数量, 0 表示无限制
											);

// 设置视频项的磁盘空间占用配额, 为"指定卷上的指定文件类型的指定记录类型"定义空间配额
// 返回值
//		0		设置成功
//		-1		设置失败
int XM_VideoItemSetDiskSpaceQuotaLevelStreamClass (unsigned int volumn_index, 	// SD卡卷序号(从0开始)
																	unsigned int file_type, 		// 文件类型(视频或照片)
																	unsigned int stream_class, 	// 视频项分类(正常、手动或紧急)
																	XMINT64 quota						//	允许的磁盘空间占用字节, 0表示无限制
																	);

// 设置视频项的磁盘空间占用配额, 为"指定卷上的指定文件类型"(包含所有录制分类)定义空间配额
// 返回值
//		0		设置成功
//		-1		设置失败
int XM_VideoItemSetDiskSpaceQuotaLevelFileType (unsigned int volumn_index, 	// SD卡卷序号(从0开始)
																unsigned int file_type,			// 文件类型(视频或照片)
																XMINT64 quota						//	允许的磁盘空间占用字节, 0表示无限制
																);

// 设置视频项的磁盘空间占用配额, 为"指定卷"(包含所有文件类型，所有录制分类)定义空间配额
// 返回值
//		0		设置成功
//		-1		设置失败
int XM_VideoItemSetDiskSpaceQuotaLevelDiskVolume (unsigned int volumn_index, 	// SD卡卷序号(从0开始)
																  XMINT64 quota					//	允许的磁盘空间占用字节, 0表示无限制
																);


// 设置磁盘报警阈值
// 	MinimumSpaceLimit(MB)		磁盘最低容量限制定义
//		RecycleSpaceLimit(MB)		可回收空间最低配额定义, 小于该值将报警( SYSTEM_EVENT_VIDEOITEM_LOW_SPACE )
// 返回值
//		0		设置成功
//		< 0	设置失败
int XM_VideoItemSetDiskSpaceLimit (unsigned int MinimumSpaceLimit, unsigned int RecycleSpaceLimit);

// 使用预定义的配额配置项目(在磁盘卷可用后设置， 否则设置可能失败)
// 	PredefinedQuotaProfile		系统预定义的profile
// 返回值
//		0		设置成功
//		< 0	设置失败
int XM_VideoItemSetQuotaProfile (unsigned int PredefinedQuotaProfile);

// 根据视频项属性(卷属性，文件类型属性，通道号及录制分类等)获取符合属性的视频项的数量
// 返回值
// -1 表示错误
// >= 0 表示返回的符合定义的视频项个数
//		1)	获取卷0上的前摄像头的普通录制视频文件总数
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_0, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL);
//		2) 获取卷0上的所有通道的紧急录制(G-Sensor)视频文件总数
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_0, 0xFFFFFFFF,  XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		3) 获取卷1上的所有通道的保护视频(手动录制，紧急录制)文件总数
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_1, 0xFFFFFFFF, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_MANUAL|XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		4) 获取卷0上的后摄像头的所有照片文件总数
//			XM_VideoItemGetVideoItemCountEx (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_1, XM_FILE_TYPE_PHOTO, 0xFFFFFFFF);
int XM_VideoItemGetVideoItemCountEx (unsigned int volume_index, 				// SD卡卷序号(从0开始), 0xFFFFFFFF表示所有卷
										  unsigned int video_channel, 			// 通道号， 0xFFFFFFFF表示所有通道
										  unsigned int file_type, 					// 文件类型(视频或照片, 0xFFFFFFFF表示所有文件类型)
										  unsigned int stream_class_bitmask		// 录像分类掩码，0xFFFFFFFF表示所有录像
												 											//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL 表示包含普通及手动分类
										);

// 根据定义的属性获取指定的视频项句柄
//		0) 获取卷0上的后摄像头的最新日期照片(日期最大)的视频项句柄
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_1, XM_FILE_TYPE_PHOTO, 0xFFFFFFFF, 0， 1 );
// 返回值
// == 0 失败
// != 0 读出的列表项纪录句柄
HANDLE XM_VideoItemGetVideoItemHandleEx (
											 unsigned int volume_index,			// 磁盘卷号, 0xFFFFFFFF表示所有卷
											 unsigned int video_channel, 			// 通道号， 0xFFFFFFFF表示所有通道
											 unsigned int file_type,				// 文件类型, 0xFFFFFFFF表示所有文件类型
											 unsigned int stream_class_bitmask,	// 录像分类掩码，0xFFFFFFFF表示所有录像
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL 表示包含普通及手动分类
											 unsigned int index,						// 序号，从0开始
											 unsigned int reverse_time_ordered	// 按时间反向排序， 1 反向排序(时间从大至小) 0 顺序排序(时间从小到大)
											);


// 根据定义的属性删除所有满足属性定义的视频项
// 返回值
//		< 0		异常
//		0			正常结束退出
//		1			用户终止退出
int XM_VideoItemDeleteVideoItemList (
											 unsigned int volume_index,			// 磁盘卷号, 0xFFFFFFFF表示所有卷
											 unsigned int video_channel, 			// 通道号， 0xFFFFFFFF表示所有通道
											 unsigned int file_type,				// 文件类型, 0xFFFFFFFF表示所有文件类型
											 unsigned int stream_class_bitmask,	// 录像分类掩码，0xFFFFFFFF表示所有录像
												 												//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL 表示包含普通及手动分类
											 int (*fp_user_callback) (void *user_data, char *full_path_name_to_delete),
											 												// 用户回调函数， 用于用户取消某个长时间操作。为0表示无回调函数
																							//		返回值为1表示终止当前的操作并退出，返回值为0表示继续当前的操作
																							//		user_data 传入的用户私有数据
																							//		full_path_name_to_delete 准备删除的视频项全路径文件名
											 void *user_data								//	用户私有数据
											);													
												

// 计算全路径目录名
// "mmc:0:\\VIDEO_F\\"
// "mmc:0:\\VIDEO_F\\NORMAL\\"
// "mmc:0:\\VIDEO_F\\URGENT\\"
// 返回值
//		< 0		失败
int XM_VideoItemMakeFullPathName (unsigned int CardVolumeIndex,			// SD卡卷序号(从0开始)
											 unsigned int VideoChannel,				// 通道号(从0开始)
											 unsigned int FileType,						// 文件类型(VIDEO/PHOTO)
											 unsigned int StreamClass,					// 录制类型(普通，手动，紧急)
											 char *lpFullPathName, int cbFullPathName	// 保存视频项文件的全路径文件名缓存及全路径文件名缓存字节长度
											);

int XM_VideoItemMakePathNameEx (	 unsigned int CardVolumeIndex,			// SD卡卷序号(从0开始)
											 unsigned int VideoChannel,				// 通道号(从0开始)
											 unsigned int FileType,						// 文件类型(VIDEO/PHOTO)
											 char *lpFullPathName, int cbFullPathName	// 保存视频项文件的全路径文件名缓存及全路径文件名缓存字节长度
											);

// 根据视频项属性(卷属性，文件类型属性，通道号及录制分类等)获取符合属性的视频项的数量
//		1)	获取卷0上的前摄像头的普通录制视频文件总数
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_0, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_NORMAL);
//		2) 获取卷0上的所有通道的紧急录制(G-Sensor)视频文件总数
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, 0xFFFFFFFF,  XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		3) 获取卷1上的所有通道的保护视频(手动录制，紧急录制)文件总数
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_1, 0xFFFFFFFF, XM_FILE_TYPE_VIDEO, XM_VIDEOITEM_CLASS_BITMASK_MANUAL|XM_VIDEOITEM_CLASS_BITMASK_URGENT);
//		4) 获取卷0上的后摄像头的所有照片文件总数
//			XM_VideoItemGetVideoItemCount (XM_VIDEOITEM_VOLUME_0, XM_VIDEO_CHANNEL_1, XM_FILE_TYPE_PHOTO, 0xFFFFFFFF);
DWORD XM_VideoItemGetVideoItemCount (unsigned int volume_index, 				// SD卡卷序号(从0开始), 0xFFFFFFFF表示所有卷
										  unsigned int video_channel, 			// 通道号， 0xFFFFFFFF表示所有通道
										  unsigned int file_type, 					// 文件类型(视频或照片, 0xFFFFFFFF表示所有文件类型)
										  unsigned int stream_class_bitmask		// 录像分类掩码，0xFFFFFFFF表示所有录像
												 											//		XM_VIDEOITEM_CLASS_BITMASK_NORMAL|XM_VIDEOITEM_CLASS_BITMASK_MANUAL 表示包含普通及手动分类
										);

#if defined (__cplusplus)
}
#endif		/* end of __cplusplus */

#endif	// _XM_VIDEO_ITEM_H_

