#ifndef _XMSYS_MESSAGE_SOCKET_H_
#define _XMSYS_MESSAGE_SOCKET_H_

#define	XMSYS_MESSAGE_SOCKET_TYPE_ID_SYNC		0xFF00
#define	XMSYS_MESSAGE_SOCKET_TYPE_ID_ASYC		0xAA55

enum {
	XMSYS_MESSAGE_SOCKET_TYPE_SYNC	=	0,		// 同步管道
	XMSYS_MESSAGE_SOCKET_TYPE_ASYC,				// 异步管道
	XMSYS_MESSAGE_SOCKET_TYPE_COUNT
};

#define	XMSYS_NID_IMAGE_OUTPUT					0x00
#define	XMSYS_NID_PLAYBACK_FINISH				0x01
#define	XMSYS_NID_SDCARD_CAPACITY				0x02
#define	XMSYS_NID_SDCARD_FORMAT_FINISH		0x03
#define	XMSYS_NID_FILE_DELETE_FINISH			0x04
#define	XMSYS_NID_SDCARD_WITHDRAW				0x05		// SD卡拔出	0x05	消息包数据为空。
#define	XMSYS_NID_SDCARD_INSERT					0x06		// SD卡插入	0x06	消息包数据为空。
#define	XMSYS_NID_SDCARD_FULL					0x07		// SD卡写满	0x07	"消息包数据为空。
																		//		一键保护的视频文件过多，导致无法记录新的视频"
#define	XMSYS_NID_SDCARD_DAMAGE					0x08		//	SD卡损坏	0x08	"消息包数据为空。
																		//		SD卡文件读写校验失败，SD卡可能已损坏，SD卡需要格式化"

#define	XMSYS_NID_FSERROR							0x09		// "SD卡文件系统非法"	0x09	
																		//		"消息包数据为空。无法访问SD卡的文件系统，SD卡需要格式化"

#define	XMSYS_NID_FILE_LIST_UPDATE				0x0A		// 文件列表更新
																		// 消息包数据12个字节
																		//		文件名(XXXXXXXX,8字节), CH(通道，1字节), TYPE(类型，1字节), CODE(操作码，2字节）
																		//		CH --> 0x00 前置摄像头
																		//		CH --> 0x01 后置摄像头
																		//		TYPE --> 0x00 视频
																		//		TYPE --> 0x01 照片
																		//		CODE --> 0 (删除)
																		//		CODE --> 1 (增加)
#define	XMSYS_NID_SYSTEM_UPDATE_PACKAGE		0x0B		// 系统升级包通知
#define	XMSYS_NID_SYSTEM_UPDATE_STEP			0x0C		// 辅机升级状态
																		// 消息包数据1个字节
																		// STEP(1字节)
																		// STEP(1 ~ 100) ，100表示升级完成。
																		// 0xFF 表示升级失败
																		// 升级完成后辅机自动重启。
																		//	主机需重新发送“通信建立”命令来建立通信。
#define	XMSYS_NID_TOO_MORE_LOCKED_RESOURCE	0x0D		// 太多锁定的视频项
#define	XMSYS_NID_SYSTEM_DAMAGE					0x0E		// 辅机系统损坏
#define	XMSYS_NID_PLAYBACK_PROGRESS			0x0F		// 回放进度指示
																		//		当前视频播放位置(2字节，秒单位)
#define	XMSYS_NID_SDCARD_INVALID				0x10		// SD卡无法识别（卡接触不良或卡无效）
#define	XMSYS_NID_SYSTEM_STARTUP				0x11		// 系统启动消息
#define	XMSYS_NID_SDCARD_FSUNSUPPORT			0x12		// SD卡文件系统为exFAT或者NTFS

#define	XMSYS_CID_SOCKET_CONNECT				0xF0		//	通信建立	0xF0	"命令包数据为空。
																		//	主机启动后向辅机发送，开始建立通信连接。
																		//	只有建立连接后，主机才能向辅机发送其他命令或者辅机向主机发送通知消息。
																		//	通信建立后，主机辅机将各自的CID及NID复位清0。"	
																		//	应答包数据格式 （辅机应答）
																		//		"1字节长度，
																		//		0x00 --> ACK
																		//		0x01 --> NAK"

#define	XMSYS_CID_SOCKET_DISCONNECT			0xF1		// 通信断开

#define	XMSYS_CID_SYSTEM_UPDATE					0xF2		//	系统升级
#define	XMSYS_CID_GET_SYSTEM_VERSION			0xF3		// 获取系统版本号
#define	XMSYS_CID_SHUTDOWN						0xF4		// 关机或复位重启
#	define	XM_SHUTDOWN_POWERDOWN				0			// 关机
#	define	XM_SHUTDOWN_REBOOT					1			// 复位重启

#define	XMSYS_CID_DATETIME_SET					0x00		// 时间设置
#define	XMSYS_CID_VIDEO_SEGMENT_LENGTH_SET	0x01		// 录像分段时间长度设置
#define	XMSYS_CID_MIC_SET							0x02		// 录音开启、关闭
#define	XMSYS_CID_MIC_GET							0x03		// 读取录音开关状态
#define	XMSYS_CID_VIDEO_OUTPUT_SET_ONOFF		0x04		// 视频输出开启、关闭
#define	XMSYS_CID_AUDIO_MUTE						0x05		// 音量静音设置
#define	XMSYS_CID_AUDIO_VOLUME					0x06		// 音量设置
#define	XMSYS_CID_CVBS_TYPE_SET					0x07		// CVBS制式设置

#define	XMSYS_CID_RECORD_START					0x10		// 录像启动
#define	XMSYS_CID_RECORD_STOP					0x11		// 录像停止
#define	XMSYS_CID_RECORD_CHANNEL_SWITCH		0x12		// 录像通道切换
#define	XMSYS_CID_RECORD_ONE_KEY_PROTECT		0x13		// 一键保护
#define	XMSYS_CID_RECORD_ONE_KEY_PHOTOGRAPH	0x14		// 一键拍照
#define	XMSYS_CID_SENSOR_START					0x15		// 开启摄像头

#define	XMSYS_CID_GET_MEDIA_FILE_COUNT		0x20		// 读取录像/照片文件数量
#define	XMSYS_CID_GET_MEDIA_FILE_LIST			0x21		// 读取录像/照片文件列表
#define	XMSYS_CID_PLAYBACK_START				0x22		// 回放启动
#define	XMSYS_CID_PLAYBACK_PAUSE				0x23		// 回放暂停
#define	XMSYS_CID_PLAYBACK_STOP					0x24		// 回放停止
#define	XMSYS_CID_PLAYBACK_FORWARD				0x25		// 回放快进
#define	XMSYS_CID_PLAYBACK_BACKWARD			0x26		// 回放快退
#define	XMSYS_CID_PLAYBACK_SEEK					0x27		// 回放定位


#define	XMSYS_CID_SDCARD_GET_STATE				0x30		// SD卡状态
#define	XMSYS_CID_SDCARD_GET_CAPACITY			0x31		// SD卡容量
#define	XMSYS_CID_SDCARD_FORMAT					0x32		// SD卡格式化
#define	XMSYS_CID_MEDIA_FILE_CHECK				0x33		// 文件检查
#define	XMSYS_CID_MEDIA_FILE_DELETE			0x34		// 文件删除
#define	XMSYS_CID_VIDEO_LOCK						0x35		// 视频文件锁定/解锁
#define	XMSYS_CID_SDCARD_CLEAR_STATE			0x36		// 清除卡的写满、卡损坏 (读写校验失败)状态，其他状态无法清除。
																		//		当主机选择忽略上述状态（写满、卡损坏）时，应将其清除，避免状态保持。

// 0 	成功
// -1 失败
int  XMSYS_MessageSocketTransferResponsePacket (unsigned int socket_type,  
																unsigned int message_no,	// 包序号	
												  				unsigned int message_id,	// 命令ID
												  				unsigned char *resp_packet_buffer,
												  				unsigned int resp_packet_length
															);

// 0 	成功
// -1 失败
int  XMSYS_MessageSocketReceiveCommandPacket (unsigned int message_no,		// 包序号	
															 unsigned int message_id,		// 命令ID
															 unsigned char *command_packet_buffer,
															 unsigned int command_packet_length
															);

void  XMSYS_MessageSocketInit (void);

void  XMSYS_MessageSocketExit (void);

// 文件列表更新
void	XMSYS_MessageSocketFileListUpdate (unsigned char channel,
													  unsigned char type,
													  unsigned char file_name[8],
													  unsigned int create_time,		// 文件创建时间
													  unsigned int record_time,		// 视频录像时间(秒)
													  unsigned short code);

// 图像输出
// 消息包数据为空。
// 视频流的第一帧或者JPEG图像已解码并输出，用于主机开启视频输入的显示。
int XMSYS_MessageSocketImageOutputReady (unsigned short width, unsigned short height);

// 回放完成 (当前录像视频文件回放完成)
void XMSYS_MessageSocketPlaybackFinish (unsigned char type);

// 回放进度指示
// 消息包数据2个字节 当前视频播放位置(2字节，秒单位)
void XMSYS_MessageSocketPlaybackProgress (unsigned int playback_progress);

// 向UI层报告SD卡的状态
// sdcard_SystemEvent	与SD卡相关的系统事件
void XMSYS_MessageSocketReportSDCardSystemEvent (unsigned int sdcard_SystemEvent);

// 使能或禁止socket命令
void XMSYS_MessageSocketSetEnable (int enable);

void XMSYS_SetCardLogicalState (unsigned int sdcard_SystemEvent);

int XMSYS_SendSystemStartupMessage (void);

// 检查socket通信是否已建立
int  XMSYS_MessageSocketCheckConnection (void);

#endif	// _XMSYS_MESSAGE_SOCKET_H_