//****************************************************************************
//
//	Copyright (C) 2013 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_uvc_socket.h
//
//	Revision history
//
//		2013.5.23 ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XMSYS_UVC_SOCKET_H_
#define _XMSYS_UVC_SOCKET_H_

#if defined (__cplusplus)
	extern "C"{
#endif

#define	XMSYS_UVC_SOCKET_TYPE_ID_SYNC		0xFF00
#define	XMSYS_UVC_SOCKET_TYPE_ID_ASYC		0xFE00

enum {
	XMSYS_UVC_SOCKET_TYPE_SYNC	=	0,		// 同步管道
	XMSYS_UVC_SOCKET_TYPE_ASYC,				// 异步管道
	XMSYS_UVC_SOCKET_TYPE_COUNT
};

#define	XMSYS_UVC_NID_IMAGE_OUTPUT					0x00		// 图像输出, 消息包数据为4个字节。
																			//		WIDTH(输出宽度，2字节),HEIGHT(输出高度，2字节)。
																			//		视频流的第一帧或者JPEG图像已解码并输出，用于主机开启视频输入的显示。
#define	XMSYS_UVC_NID_PLAYBACK_FINISH				0x01		// 回放完成
																			// 	消息包数据为空。当前录像视频文件回放完成
#define	XMSYS_UVC_NID_SDCARD_CAPACITY				0x02		//	SD卡容量
	
#define	XMSYS_UVC_NID_SDCARD_FORMAT_FINISH		0x03		// "SD卡格式化完成"

#define	XMSYS_UVC_NID_FILE_DELETE_FINISH			0x04		// "文件删除完成"

#define	XMSYS_UVC_NID_SDCARD_WITHDRAW				0x05		// SD卡拔出	0x05	消息包数据为空。
#define	XMSYS_UVC_NID_SDCARD_INSERT				0x06		// SD卡插入	0x06	消息包数据为空。
#define	XMSYS_UVC_NID_SDCARD_FULL					0x07		// SD卡写满	0x07	"消息包数据为空。
																			//		一键保护的视频文件过多，导致无法记录新的视频"
#define	XMSYS_UVC_NID_SDCARD_DAMAGE				0x08		//	SD卡损坏	0x08	"消息包数据为空。
																			//		SD卡文件读写校验失败，SD卡可能已损坏，SD卡需要格式化"

#define	XMSYS_UVC_NID_FSERROR						0x09		// "SD卡文件系统非法"	0x09	
																			//		"消息包数据为空。无法访问SD卡的文件系统，SD卡需要格式化"

#define	XMSYS_UVC_NID_SDCARD_INVALID				0x0A		// SD卡无法识别（卡接触不良或卡无效）

#define	XMSYS_UVC_NID_FILE_LIST_UPDATE			0x0B		// 文件列表更新
																				// 消息包数据12个字节
																				//		文件名(XXXXXXXX,8字节), CH(通道，1字节), TYPE(类型，1字节), CODE(操作码，2字节）
																				//		CH --> 0x00 前置摄像头
																				//		CH --> 0x01 后置摄像头
																				//		TYPE --> 0x00 视频
																				//		TYPE --> 0x01 照片
																				//		CODE --> 0 (删除)
																				//		CODE --> 1 (增加)
#define	XMSYS_UVC_NID_SYSTEM_UPDATE_PACKAGE		0x0C		// 系统升级包通知
#define	XMSYS_UVC_NID_SYSTEM_UPDATE_STEP			0x0D		// 辅机升级状态
																				// 消息包数据1个字节
																				// STEP(1字节)
																				// STEP(1 ~ 100) ，100表示升级完成。
																				// 0xFF 表示升级失败
																				// 升级完成后辅机自动重启。
																				//	主机需重新发送“通信建立”命令来建立通信。
#define	XMSYS_UVC_NID_TOO_MORE_LOCKED_RESOURCE	0x0E		// 太多锁定的视频项
#define	XMSYS_UVC_NID_PLAYBACK_PROGRESS			0x0F		// 回放进度指示
																			//		当前视频播放位置(2字节，秒单位)
#define	XMSYS_UVC_NID_FILELIST						0x10		// 文件目录列表
#define	XMSYS_UVC_NID_DOWNLOAD						0x11		// 下载数据
#define	XMSYS_UVC_NID_LOW_PREFORMANCE				0x12		// SD卡文件系统写入速度较慢
																			//		SD卡文件系统的写入速度(FAT簇字节大小太小)可能无法满足实时录像, 建议用户格式化SD卡.
#define	XMSYS_UVC_NID_HYBRID_STREAM				0x13		//	hubrid混合流

#define	XMSYS_UVC_CID_SOCKET_CONNECT				0x50		//	通信建立	0xF0	"命令包数据为空。
																		//	主机启动后向辅机发送，开始建立通信连接。
																		//	只有建立连接后，主机才能向辅机发送其他命令或者辅机向主机发送通知消息。
																		//	通信建立后，主机辅机将各自的CID及NID复位清0。"	
																		//	应答包数据格式 （辅机应答）
																		//		"1字节长度，
																		//		0x00 --> ACK
																		//		0x01 --> NAK"

#define	XMSYS_UVC_CID_SOCKET_DISCONNECT			0x51		// 通信断开

#define	XMSYS_UVC_CID_SYSTEM_UPDATE				0x52		//	系统升级
#define	XMSYS_UVC_CID_GET_SYSTEM_VERSION			0x53		// 获取系统版本号
#define	XMSYS_UVC_CID_SHUTDOWN						0x54		// 关机或复位重启
#define	XM_SHUTDOWN_POWERDOWN				0			// 关机
#define	XM_SHUTDOWN_REBOOT					1			// 复位重启
#define	XMSYS_UVC_CID_AUTOTEST_KEY					0x55		// 自动测试按键

#define	XMSYS_UVC_CID_HYBRID_START					0x60		// HYBRID流启动
#define	XMSYS_UVC_CID_HYBRID_STOP					0x61		// HYBRID流停止
#define	XMSYS_UVC_CID_HYBRID_FORCE_IFRAME		0x62		// 强制I帧
																			// CH (1字节长度)
																			//			CH --> 0 强制前置摄像头I帧编码
																			//			CH --> 1 强制后置摄像头I帧编码
																			//			其他值保留

// 设置
#define	XMSYS_UVC_CID_DATETIME_SET					0x00		// 时间设置
#define	XMSYS_UVC_CID_VIDEO_SEGMENT_LENGTH_SET	0x01		// 录像分段时间长度设置
#define	XMSYS_UVC_CID_MIC_SET						0x02		// 录音开启、关闭
#define	XMSYS_UVC_CID_MIC_GET						0x03		// 读取录音开关状态
#define	XMSYS_UVC_CID_VIDEO_OUTPUT_SET_ONOFF	0x04		// 视频输出开启、关闭
#define	XMSYS_UVC_CID_AUDIO_MUTE					0x05		// 音量静音设置
#define	XMSYS_UVC_CID_AUDIO_VOLUME					0x06		// 音量设置
#define	XMSYS_UVC_CID_OUTPUT_IMAGE_SIZE			0x07		// 输出图像设置 WIDTH(2字节),HEIGHT(2字节)
#define	XMSYS_UVC_CID_VIDEO_FORMAT					0x08		// 录像视频格式设置 (1080p30fps, 720p30fps, 720p60fps)
#define	XMSYS_UVC_CID_SENSOR_PROBE					0x09		// 检查摄像头是否存在

// 录像/拍照控制
#define	XMSYS_UVC_CID_RECORD_START						0x10		// 录像启动
#define	XMSYS_UVC_CID_RECORD_STOP						0x11		// 录像停止
#define	XMSYS_UVC_CID_RECORD_CHANNEL_SWITCH			0x12		// 录像通道切换
#define	XMSYS_UVC_CID_RECORD_ONE_KEY_PROTECT		0x13		// 一键保护
#define	XMSYS_UVC_CID_RECORD_ONE_KEY_PHOTOGRAPH	0x14		// 一键拍照
#define	XMSYS_UVC_CID_SENSOR_START						0x15		// 开启摄像头
#define	XMSYS_UVC_CID_SET_FORCED_NON_RECORDING_MODE	0x16		// 使能/禁止强制非录像模式
#define	XMSYS_UVC_CID_GET_FORCED_NON_RECORDING_MODE	0x17		// 获取强制非录像模式
#define	XMSYS_UVC_CID_LOCK_CURRENT_VIDEO					0x18		// 将当前录制视频锁定
#define	XMSYS_UVC_CID_UNLOCK_CURRENT_VIDEO				0x19		// 将当前录制视频解锁
#define	XMSYS_UVC_CID_GET_VIDEO_LOCK_STATE				0x1A		// 获取当前录像视频的锁定状态


// 视频/图片浏览及录像回放
#define	XMSYS_UVC_CID_GET_MEDIA_FILE_COUNT			0x20		// 读取录像/照片文件数量
#define	XMSYS_UVC_CID_GET_MEDIA_FILE_LIST			0x21		// 读取录像/照片文件列表
#	define		XM_UVC_TYPE_VIDEO		0						//			TYPE --> 0x00 视频
#	define		XM_UVC_TYPE_PHOTO		1						//			TYPE --> 0x01 照片
#	define		XM_UVC_TYPE_ALL		7						//			TYPE --> 0x07 照片及视频	

#define	XMSYS_UVC_CID_PLAYBACK_START					0x22		// 回放启动
#define	XMSYS_UVC_CID_PLAYBACK_PAUSE					0x23		// 回放暂停
#define	XMSYS_UVC_CID_PLAYBACK_STOP					0x24		// 回放停止
#define	XMSYS_UVC_CID_PLAYBACK_FORWARD				0x25		// 回放快进
#define	XMSYS_UVC_CID_PLAYBACK_BACKWARD				0x26		// 回放快退
#define	XMSYS_UVC_CID_PLAYBACK_SEEK					0x27		// 回放定位

// 卡及文件管理
#define	XMSYS_UVC_CID_SDCARD_GET_STATE				0x30		// SD卡状态
#define	XMSYS_UVC_CID_SDCARD_GET_CAPACITY			0x31		// SD卡容量
#define	XMSYS_UVC_CID_SDCARD_FORMAT					0x32		// SD卡格式化
#define	XMSYS_UVC_CID_MEDIA_FILE_CHECK				0x33		// 文件检查
#define	XMSYS_UVC_CID_MEDIA_FILE_DELETE				0x34		// 文件删除
#define	XMSYS_UVC_CID_VIDEO_LOCK						0x35		// 视频文件锁定/解锁
#define	XMSYS_UVC_CID_SDCARD_CLEAR_STATE				0x36		// 清除卡的写满、卡损坏 (读写校验失败)状态，其他状态无法清除。
																				//		当主机选择忽略上述状态（写满、卡损坏）时，应将其清除，避免状态保持。

// 视频/图片下载支持
#define	XMSYS_UVC_CID_DOWNLOAD_OPEN					0x40		// 下载开启, 将视频文件下载到主机, 类似fopen
#define	XMSYS_UVC_CID_DOWNLOAD_CLOSE					0x41		// 下载关闭, 关闭当前正在进行的视频下载. 类似fclose
#define	XMSYS_UVC_CID_DOWNLOAD_SEEK					0x42		// 下载定位, 定位下载的位置, 类似fseek
#define	XMSYS_UVC_CID_DOWNLOAD_READ					0x43		// 下载读取, 读取下载数据, 类似fread

// 0 	成功
// -1 失败
int  XMSYS_UvcSocketTransferResponsePacket (unsigned int socket_type,  
												  				unsigned char message_id,	// 命令ID
												  				unsigned char *resp_packet_buffer,
												  				unsigned int resp_packet_length
															);

// 0 	成功
// -1 失败
int  XMSYS_UvcSocketReceiveCommandPacket (
															 unsigned char message_id,		// 命令ID
															 unsigned char *command_packet_buffer,
															 unsigned int command_packet_length
															);

void  XMSYS_UvcSocketInit (void);

void  XMSYS_UvcSocketExit (void);

// 文件列表更新
void	XMSYS_UvcSocketFileListUpdate (unsigned char channel,
													  unsigned char type,
													  unsigned char file_name[8],
													  unsigned int create_time,		// 文件创建时间
													  unsigned int record_time,		// 视频录像时间(秒)
													  unsigned short code				// CODE --> 0 (删除, 循环录像文件删除)
																								//	CODE --> 1 (增加, 新增录像文件更新或者照片文件更新)
													);

// 图像输出
// 消息包数据为空。
// 视频流的第一帧或者JPEG图像已解码并输出，用于主机开启视频输入的显示。
int XMSYS_UvcSocketImageOutputReady (unsigned short width, unsigned short height);

// 回放完成 (当前录像视频文件回放完成)
void XMSYS_UvcSocketPlaybackFinish (unsigned char type);

// 回放进度指示
// 消息包数据2个字节 当前视频播放位置(2字节，秒单位)
void XMSYS_UvcSocketPlaybackProgress (unsigned int playback_progress);

// 向UI层报告SD卡的状态
// sdcard_SystemEvent	与SD卡相关的系统事件
void XMSYS_UvcSocketReportSDCardSystemEvent (unsigned int sdcard_SystemEvent);

// 使能或禁止socket命令
void XMSYS_UvcSocketSetEnable (int enable);

void XMSYS_UvcSocketSetCardLogicalState (unsigned int sdcard_SystemEvent);

// 检查socket通信是否已建立
int  XMSYS_UvcSocketCheckConnection (void);

void XMSYS_CdrPrivateProtocolProcess (void);

int XMSYS_CdrSendResponse (unsigned int packet_type, unsigned char command_id, 
									unsigned char *response_data_buffer, unsigned int response_data_length);


// 读取命令应答帧或者扩展应答帧
// 每个应答帧由2个字节构成
// 命令应答包由一个应答帧构成
// 扩展应答包由至少2个应答帧构成
unsigned short XMSYS_CdrGetResponseFrame (void);

// 读取卡状态码
// 一字节长度，
// 0x01 --> SD卡拔出
// 0x02 --> SD卡插入
// 0x03 --> SD卡文件系统错误
// 0x04 --> SD卡无法识别
int XMSYS_UvcSocketGetCardState (void);

#define	XM_UVC_ID			"XSPACE_USB_VIDEO"
#define	XM_UVC_VERSION		0x00010000		// 1.0版本
#define	XM_UVC_TYPE_JPEG	0x00000000
#define	XM_UVC_TYPE_H264	0x00000001

#define	XM_UVC_SECTOR_NUMBER_INFO			(1024*1024*1024/512)						// 设备信息(读出)/USB视频启动或停止(写入)
#define	XM_UVC_SECTOR_NUMBER_COMMAND		(XM_UVC_SECTOR_NUMBER_INFO + 1)		// UVC私有协议命令(写入)/应答(读出)			
#define	XM_UVC_SECTOR_NUMBER_STREAM		(XM_UVC_SECTOR_NUMBER_COMMAND + 1)	// 视频帧流信息(只读)
#define	XM_UVC_SECTOR_NUMBER_DATA			(XM_UVC_SECTOR_NUMBER_STREAM + 1)	// 视频帧数据读出(只读),16MB
#define	XM_UVC_SECTOR_NUMBER_VOICE			(XM_UVC_SECTOR_NUMBER_DATA + 0x1000000/512)

typedef struct XM_UVC_INFO {
	unsigned char id[16];		// "XSPACE_USB_VIDEO"
	unsigned int version;		// 版本
	unsigned int stream_type;	// JPEG/H264格式信息
	unsigned int width;			// UVC视频尺寸, 16的倍数
	unsigned int height;
	unsigned int start;			// 启动/停止UVC传输
} XM_UVC_INFO;

#define	XM_UVC_STREAM_QUALITY_0			0			// 质量最差
#define	XM_UVC_STREAM_QUALITY_9			9			// 质量最好
// 读取当前的UVC编码质量级别
unsigned int xm_uvc_get_video_quality_level (void);
// 设置UVC视频编码质量级别
int xm_uvc_set_video_quality_level (unsigned int quality_level);


typedef struct XM_UVC_STREAM {
	unsigned int stream_type;		// JPEG/H264格式信息
	unsigned int width;				// UVC视频尺寸
	unsigned int height;
	unsigned int stream_size;		// UVC视频流字节长度
	unsigned int quality;			// UVC视频流编码质量
} XM_UVC_STREAM;



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XMSYS_UVC_SOCKET_H_