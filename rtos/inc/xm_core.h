#ifndef _XM_CORE_H_
#define _XM_CORE_H_

#ifdef __cplusplus
extern "C" {
#endif
	
#include <xm_type.h>
	
#include <xm_proj_define.h>
	
#include "rtos.h"
	
//#include "types.h"
#include <xm_base.h>
#include <xm_printf.h>
#include "rxchip.h"
	
#define SEMA_LOCK(sema)			OS_Use(sema)
#define SEMA_UNLOCK(sema)		OS_Unuse(sema)	
#define SEMA_CREATE(sema)		OS_CREATERSEMA(sema)
#define SEMA_DELETE(sema)		OS_DeleteRSema(sema)
	

	
// 内核线程定义
// 1)  主线程 MAIN_TASK (UI线程)
//	2)  SENSOR任务 SensorTask	
// 3)  录像记录任务 RecordTask
// 4)  录像回放任务 PlaybackTask
// 5)  流接收任务 ReceiveTask
// 6)  流传输任务 TransferTask
// 7)  Video显示任务 VideoTask
// 8)  PC Camera任务CameraTask
// 9)  GPS轨迹记录任务 GpsTask
// 10) 系统监控任务 SystemMonitorTask
// 11) 语音播放任务 VoiceTask
// 12) IP任务
// 13) IP_RECV任务
// 14) DHCPD dhcp服务任务(为辅机分配IP地址)
	
// 任务优先级定义(值越大优先级越高)
#define	XMSYS_IP_TASK_PRIORITY					200
#define	XMSYS_IPRX_TASK_PRIORITY					199
#define	XMSYS_DHCPD_TASK_PRIORITY				100		// DHCPD服务		
	
#define	XMSYS_VIDEO_TASK_PRIORITY				200		// 254
#define	XMSYS_ROTATE_TASK_PRIORITY				253
#define	XMSYS_SENSOR_TASK_PRIORITY				252
#define	XMSYS_CAMERA_TASK_PRIORITY				253		// 最高优先级，确保插拔时满足协议要求的时间要求
#define	XMSYS_GPS_TASK_PRIORITY					120
#define	XMSYS_ISP_TASK_PRIORITY					255		// 不要讲其他任务的优先级设置为255, 仅ISP为255


// 设置H264 Codec任务最高优先级，可以达到每秒最大编码帧率	
#define	XMSYS_H264CODEC_TASK_PRIORITY			253		// H264编码线程优先级
#define	XMSYS_H264File_TASK_PRIORITY				252		// H264写入线程优先级，应高于编码线程
#define	XMSYS_ONE_KEY_PHOTOGRAPH_TASK_PRIORITY	254		// 一键拍照线程, 应高于H264编码线程, 优先获取sensor包
	
#define	XMSYS_APP_TASK_PRIORITY					200	//120 //	UI线程拥有最高优先级
	
#define	XMSYS_JPEG_TASK_PRIORITY					120		//	JPEG线程
	
#define	XMSYS_HARD_TIMER_TASK_PRIORITY			254		//	硬件定时器服务任务（具有最高任务优先级）
#define	XMSYS_MESSAGE_TASK_PRIORITY				254		//	系统消息服务任务，仅低于硬件定时器任务

#define	XMSYS_SYSTEM_UPDATE_TASK_PRIORITY		200

#define	XMSYS_USBHOST_TASK_PRIORITY				233		// USB HOST的服务任务			
	
// 回收任务的优先级不要低于视频/APP任务优先级，否则CPU资源紧张时，回收任务没有机时完成回收任务，导致丢帧现象。	
//#define	XMSYS_RECYCLE_TASK_PRIORITY				100
//#define	XMSYS_RECYCLE_TASK_PRIORITY				(XMSYS_VIDEO_TASK_PRIORITY+1)	

// 20180324 zhuoyonghong
// 修改回收线程的优先级，改为与Cache线程一致，这样系统忙时，避免回收线程无法获得文件系统控制权
#define	XMSYS_RECYCLE_TASK_PRIORITY				XMSYS_H264File_TASK_PRIORITY	
	
#define	XMSYS_UART_SOCKET_TASK_PRIORITY			253
#define	XMSYS_HONGJING_SOCKET_TASK_PRIORITY		253
	
#define	XMSYS_FILE_MANAGER_TASK_PRIORITY		120			// 文件管理器线程
	
#define	XMSYS_AUTOTEST_TASK_PRIORITY				252			// 自动测试任务
	
#define	XMSYS_USBHOST_TASK_PRIORITY				233		// USB HOST的服务任务	
	
#define	XMSYS_ARK7116_TASK_PRIORITY				253		// ARK7116配置线程
	
#define	XMSYS_NVP6134C_TASK_PRIORITY				253		// NVP6134C配置线程

#define	XMSYS_NVP6124B_TASK_PRIORITY				120		// NVP6124B配置线程

#define	XMSYS_PR2000_TASK_PRIORITY				253		// PR2000配置线程

#define	XMSYS_PR1000_TASK_PRIORITY				253		// PR1000配置线程
#define	XMSYS_RXCHIP_TASK_PRIORITY				253		// PR2000配置线程
#define	XMSYS_ITU601SCALER_TASK_PRIORITY		120	    // ITU601 Scaler 线程
#define	XMSYS_AES_TASK_PRIORITY					253		// aes配置线程

#define	XMSYS_MAIN_TASK_STACK_SIZE			(0x2000)			// 8KB
#define	XMSYS_HARD_TIMER_STASK_SIZE			(0x1000)			// 硬件定时器任务栈
#define	XMSYS_SENSOR_TASK_STACK_SIZE			(0x1000)			// 1KB
#define	XMSYS_H264CODEC_TASK_STACK_SIZE		(0x4000)			// 8KB
#define	XMSYS_PLAYBACK_TASK_STACK_SIZE		(0x0400)			// 1KB	
#define	XMSYS_RECEIVE_TASK_STACK_SIZE		(0x0400)			// 1KB
#define	XMSYS_TRANSFER_TASK_STACK_SIZE		(0x0400)			// 1KB
#define	XMSYS_VIDEO_TASK_STACK_SIZE			(0x2000)			// 1KB
#define	XMSYS_CAMERA_TASK_STACK_SIZE			(0x2000)			// 1KB
#define	XMSYS_IP_TASK_STACK_SIZE				(0x0400)			// 1KB
#define	XMSYS_IPRX_TASK_STACK_SIZE			(0x0400)			// 1KB
#define	XMSYS_DHCPD_TASK_STACK_SIZE			(0x0200)			// 512
#define	XMSYS_GPS_TASK_STACK_SIZE			(0x0400)			// 1KB
#define	XMSYS_H264FILE_TASK_STACK_SIZE		(0x2000)			// 16KB	// 需要访问文件系统	
#define	XMSYS_RECYCLE_TASK_STACK_SIZE		(0x2000)			
#define	XMSYS_SYSTEM_UPDATE_TASK_STACK_SIZE	(0x2000)
#define	XMSYS_ISP_TASK_STACK_SIZE			(0x2000)

#define	XMSYS_JPEG_TASK_STACK_SIZE				(0x1000)			// 1KB
#define	XMSYS_USBHOST_TASK_STACK_SIZE			(4*1024)
		
#define	XMSYS_APP_TASK_STACK_SIZE					(0x4000)			// 16KB	UI用户线程
	
#define	XMSYS_FILE_MANAGER_TASK_STACK_SIZE			(0x1000)
		
#define	XMSYS_MESSAGE_TASK_STACK_SIZE				(0x2000)
#define	XMSYS_ONE_KEY_PHOTOGRAPH_TASK_STACK_SIZE	(0x2000)
	
#define	XMSYS_AUTOTEST_TASK_STACK_SIZE				(0x800)

#define	XMSYS_ARK7116_TASK_STACK_SIZE				(0x800)		// ARK7116配置线程堆栈

#define	XMSYS_NVP6134C_TASK_STACK_SIZE				(0x800)		// NVP6134C配置线程堆栈

#define	XMSYS_NVP6124B_TASK_STACK_SIZE				(0x800)		// NVP6124B配置线程堆栈

#define	XMSYS_PR2000_TASK_STACK_SIZE					(0x800)		// PR2000配置线程堆栈

#define	XMSYS_PR1000_TASK_STACK_SIZE					(0x800)		// PR1000配置线程堆栈
#define	XMSYS_RXCHIP_TASK_STACK_SIZE				(0x1000)		// D6752配置线程堆栈
#define	XMSYS_ITU601SCALER_TASK_STACK_SIZE			(0x1000)		
#define	XMSYS_AES_TASK_STACK_SIZE					(0x800)	// aes配置线程堆栈


// 定义UVC流的来源	
#define	XMSYS_JPEG_MODE_ISP				0		// ISP		
#define	XMSYS_JPEG_MODE_ISP_SCALAR		1		// ISP Scalar
#define	XMSYS_JPEG_MODE_601				2
#define	XMSYS_JPEG_MODE_601_SCALAR		3
#define	XMSYS_JPEG_MODE_VIDEO			4		// 视频文件解码
#define	XMSYS_JPEG_MODE_IMAGE			5		// JPEG图像
#define	XMSYS_JPEG_MODE_DUMMY			6		// 用于高速文件下载, 后台连接

#define	XMSYS_JPEG_EVENT_ENCODE							0x10		// 启动JPEG编码
#define	XMSYS_JPEG_EVENT_TICKET							0x20
#define	XMSYS_JPEG_EVENT_MODE_SETUP					0x40		// 	
	
// mode						UVC流的来源
// mode_private_data		与UVC源定义相关的私有数据
int XMSYS_JpegSetupMode (unsigned int mode, void *mode_private_data);
	
	
// 系统网络模块初始化
void SYS_NetworkModuleInit (void);


int XMSYS_VideoInit (void);
int XMSYS_VideoExit (void);
int XMSYS_VideoOpen (void);
int XMSYS_VideoClose (void);

void XMSYS_VideoTask (void);

int XMSYS_UsbVideoClose (void);
int XMSYS_UsbVideoOpen (void);

void XMSYS_AutoTestInit (void);		// 自动测试初始化

// 帧私有数据类型ID定义
enum {
	XM_PACKET_USER_DATA_ID_EXP = 0,		// Exp
	XM_PACKET_USER_DATA_ID_ISP,			// ISP
	XM_PACKET_USER_DATA_ID_DATETIME,		// 日期
	XM_PACKET_USER_DATA_ID_GPS,			// GPS
	XM_PACKET_USER_DATA_ID_COUNT
};

#define	XM_PACKET_USER_DATA_SIZE		2048		

/////////////////////////////////////////////////////////////////////
//                                                                 //
// ******************* Sensor模块定义 *****************************//
//                                                                 //
/////////////////////////////////////////////////////////////////////

typedef struct tagXMSYSSENSORPACKET {
	void *				prev;			
	void *				next;
	int					id;
	
	unsigned int		width;
	unsigned int		height;
	unsigned int		size;			// 缓冲区字节大小
	unsigned int		stride[3];
	char *				data[3];
	
	char *				buffer;		// 指向YUV缓冲区
		
	unsigned int		frame_id;	// ISP帧ID
	int					channel;			// 通道号
	
	volatile int		data_ref;
	volatile int		prep_ref;

	// 私有数据(ISP设置及曝光参数包版本)
	char *				isp_user_data;
	unsigned short		isp_user_size;
	
	// 保存与帧相关的私有数据
	unsigned int		isp_user_buff[XM_PACKET_USER_DATA_SIZE/4];
	
	
} XMSYSSENSORPACKET;
	
#define	XMSYS_RAW_FRAME_SIZE_1280X960			(1280*960*3/2)
#define	XMSYS_RAW_FRAME_SIZE_720P				(1280*720*3/2)		// 此处设置为1280*720*2出现异常，????
#define	XMSYS_RAW_FRAME_SIZE_VGA				(640*480*3/2)
#define	XMSYS_RAW_FRAME_SIZE_QVGA				(320*240*3/2)
#define	XMSYS_RAW_FRAME_SIZE_D1					(704*576*3/2)

// 前置摄像头帧大小(720P)
#define	XMSYS_FRONT_SENSOR_FRAME_SIZE			XMSYS_RAW_FRAME_SIZE_720P
// 后置摄像头帧大小(VGA)
#define	XMSYS_REAR_SENSOR_FRAME_SIZE			XMSYS_RAW_FRAME_SIZE_VGA

#define	XMSYS_D1_SENSOR_FRAME_SIZE				(704*576*3/2)

// SENSOR_TASK的事件定义
#define	XMSYS_SENSOR_CH_0_PACKET_READY_EVENT			0x01		// 前置数据帧已采集完毕，由CAPTURE中断产生
#define	XMSYS_SENSOR_CH_1_PACKET_READY_EVENT			0x02		// 后置数据帧已采集完毕，由CAPTURE中断产生
#define	XMSYS_SENSOR_CH_2_PACKET_READY_EVENT			0x04		// 左侧数据帧已采集完毕，由CAPTURE中断产生
#define	XMSYS_SENSOR_CH_3_PACKET_READY_EVENT			0x08		// 右侧数据帧已采集完毕，由CAPTURE中断产生

#define	XMSYS_SENSOR_CAPTURE_START							0x10		// 启动Sensor采集（所有通道）
#define	XMSYS_SENSOR_CAPTURE_STOP							0x20		// 停止Sensor采集（所有通道）
#define	XMSYS_SENSOR_CAPTURE_EXIT							0x40		// 退出

#define	XMSYS_UVC_PROGRAM_KEYID								0x80


// 获取一个前置数据帧
int *XMSYS_SensorFrontGetDataFrame (void);
// 释放一个前置数据帧
void XMSYS_SensorFrontFreeDataFrame (int *DataFrame);
// 通知一个新的数据帧已产生
void XMSYS_SensorFrontNewDataFrame (int *DataFrame);
// 获取下一个空闲帧用于帧记录
int * XMSYS_SensorFrontGetFreeFrame (void);
// 由Capture中断程序调用
// 通知一个新的前置数据帧已产生
void XMSYS_SensorFrontNotifyDataFrame (void);
// 获取一个后置数据帧
int *XMSYS_SensorRearGetDataFrame (void);
// 释放一个后置数据帧
void XMSYS_SensorRearFreeDataFrame (int *DataFrame);
// 通知一个新的数据帧已产生
void XMSYS_SensorRearNewDataFrame (int *DataFrame);
// 获取下一个空闲帧用于帧记录
int * XMSYS_SensorRearGetFreeFrame (void);
// 由Capture中断程序调用
// 通知一个新的前置数据帧已产生
void XMSYS_SensorRearNotifyDataFrame (void);

// 通知一个新的数据帧已产生, 并将帧压入到YUV数据帧链表
void XMSYS_SensorNotifyDataFrame (int channel, int frame_id);

int XMSYS_SensorStartCapture (unsigned int channel, unsigned char *Frame);

// 获取前置帧地址
void *XMSYS_SensorGetHardwareAddress (int channel, unsigned int *PacketSize);

// H264编码线程获取一个Sensor采样包
XMSYSSENSORPACKET * XMSYS_SensorGetAndLockSensorPacket (int channel);
void XMSYS_SensorReleaseSensorPacket (int channel, XMSYSSENSORPACKET *Packet);



// 将YUV格式的原始720P帧裁剪为320*240格式的视图帧
void XMSYS_Scale720PFrameToViewFrame (int *Raw720PFrame, int *ViewFrame);
// 将YUV格式的原始VGA帧裁剪为320*240格式的视图帧
void XMSYS_ScaleVGAFrameToViewFrame (int *RawVGAFrame, int *ViewFrame);

// 将YUV格式的原始720P帧裁剪为640*480格式的视图帧
void XMSYS_Scale720PFrameToVGAFrame (int *Raw720PFrame, int *VGAFrame);

// 将YUV格式的原始D1帧(704*576)裁剪为(320*240)格式的视图帧
void XMSYS_ScaleD1FrameToViewFrame (int *RawD1Frame, int *ViewFrame);
// 将YUV格式的原始D1帧(704*576)裁剪为(640*480)格式的视图帧
void XMSYS_ScaleD1FrameToVGAFrame (int *RawD1Frame, int *ViewFrame);

void XMSYS_Scale1280X960FrameTo720PFrame (int *RawFrame, int *ScaleFrame);

void XMSYS_Scale1280X960FrameToVGAFrame (int *RawFrame, int *ScaleFrame);

void XMSYS_Scale1280X960FrameToViewFrame (int *RawFrame, int *ScaleFrame);

void XMSYS_ScaleFrame (int *InFrame, int *OutFrame,
							  int in_width, int in_height,
							  int out_width, int out_height);


void *XMSYS_SensorFrontGetHardwareAddress (unsigned int *PacketSize);

int XMSYS_SensorFrontStartCapture (int *Frame);

void XMSYS_SensorStart (void);
void XMSYS_SensorStop  (void);


void XMSYS_SensorInit (void);
void XMSYS_SensorExit (void);

// 启动Sensor采集
void XMSYS_SensorCaptureStart (void);
// 停止Sensor采集
void XMSYS_SensorCaptureStop (void);
// sensor异常时, 复位相应的通道
int XMSYS_SensorCaptureReset (int channel);


// Sensor硬件初始化
void XMSYS_SensorHardwareInit (void);

// 创建一个1280*960的D1X4组合帧
XMSYSSENSORPACKET * XMSYS_SensorCreate4D1ComposedSensorPacket (void);
// 删除一个1280*960的D1X4组合帧
void XMSYS_SensorDelete4D1ComposedSensorPacket (XMSYSSENSORPACKET *CompFrame);

XMSYSSENSORPACKET * XMSYS_SensorCreatePacket (int channel);

// 检查是否存在sensor数据包需要处理
// 1 表示任意一个通道上存在至少一个数据包需要处理
// 0 没有数据包需要处理
int XMSYS_SensorCheckPacketReady (void);

void XMSYS_SensorDeletePacket (int channel, XMSYSSENSORPACKET *packet);
// 一键拍照重新将sensor帧投递到预处理队列, 提供给H264编码器使用
void XMSYS_SensorRecyclePacket (int channel, XMSYSSENSORPACKET *packet);
void XMSYS_SensorReturnPacket (int channel, XMSYSSENSORPACKET *packet);

void XMSYS_SensorStartCaptureAccount (void);
void XMSYS_SensorStopCaptureAccount (void);





/////////////////////////////////////////////////////////////////////
//                                                                 //
// ******************* Camera模块定义 *****************************//
//                                                                 //
/////////////////////////////////////////////////////////////////////

#define	XMSYS_CAMERA_USB_EVENT							0x01	// USB物理事件(如枚举等)
#define	XMSYS_CAMERA_UVC_START							0x02	// UVC启动
#define	XMSYS_CAMERA_UVC_STOP								0x04	// UVC结束
#define	XMSYS_CAMERA_UVC_TRANSFER						0x08	// 请求传输下一个帧
#define	XMSYS_CAMERA_PACKET_TRANSFER_DONE				0x10	// Camera包已传输完毕，由USB中断产生
#define	XMSYS_CAMERA_PACKET_READY						0x20	// Camera包Ready事件, 表示一个新的UVC包READY，等待传输
#define	XMSYS_CAMERA_CDR									0x40	// 接收到一个CDR命令
#define	XMSYS_CAMERA_UVC_BREAK							0x80	// UVC异常结束, usb reset/usb suspend等

// 错误码定义
#define	XMSYS_CAMERA_SUCCESS							0			// 成功
#define	XMSYS_CAMERA_ERROR_USB_DISCONNECT			(-1)		// USB连接断开
#define	XMSYS_CAMERA_ERROR_USB_EXCEPTION			(-2)		// USB传输异常
#define	XMSYS_CAMERA_ERROR_USB_UVCSTOP				(-3)		// USB UVC终止
#define	XMSYS_CAMERA_ERROR_OTHER						(-4)		// 其他异常

#define	XMSYS_CAMERA_ERROR_MODE_ERROR				(-5)		// CDR模式不支持
#define	XMSYS_CAMERA_ERROR_SAVE_FILE_OPEN			(-6)		// 本地CDR保存文件没有打开
#define	XMSYS_CAMERA_ERROR_SAVE_FILE_WRITE			(-7)		// 本地CDR保存文件写入失败

typedef void (*XMSYS_CAMERA_CALLBACK) (void *handle, int err_code);			// Camera传输结束回调函数
typedef void (*XMSYS_CAMERA_RETRANSFER) (void *handle);


// USB Camera 数据帧定义
typedef struct tagXMCAMERAFRAME {
	void *			prev;
	void *			next;

	int				retransfer;		//
	int				ttl;					// 重发次数
	
	unsigned char *frame_base;			// 待传USB帧基址
	unsigned int   frame_size;			// 待传USB帧字节大小
	
	void *         frame_private;		// 待传USB帧的私有数据
	XMSYS_CAMERA_CALLBACK fp_terminate;	// USB帧传输完毕回调函数

	XMSYS_CAMERA_RETRANSFER fp_retransfer;	// USB重传回调函数
	
} XMCAMERAFRAME;


#define	XMSYS_CAMERA_FRAME_SIZE		(1280*960*2)

// 获取下一个空闲帧用于帧记录
int * XMSYS_CameraGetFreeFrame (void);
// 释放空闲帧
void XMSYS_CameraPutFreeFrame (int *FreeFrame);


// 通知一个新的数据帧已产生
int XMSYS_CameraNewDataFrame (int *lpCameraFrame, int cbCameraFrame, void *lpPrivateData,
						XMSYS_CAMERA_CALLBACK fpCallback, XMSYS_CAMERA_RETRANSFER fpRetransfer);
// 获取一个Camera数据帧
XMCAMERAFRAME *XMSYS_CameraGetDataFrame (void);
// 释放一个Camera数据帧
void XMSYS_CameraFreeDataFrame (XMCAMERAFRAME *lpDataFrame, int ErrCode);

void XMSYS_CameraInit (void);
void XMSYS_CameraExit (void);

void pccamera_init (void);
void pccamera_process (void);

// 由USB中断程序调用, 通知Camera任务Camera帧已传输完毕
void XMSYS_CameraNotifyDataFrameTransferDone (void);
void XMSYS_CameraGetWindowSize (XMSIZE *lpSize);
void XMSYS_CameraPostEvent (unsigned char Event);
// 检查Camera是否已准备好 1 准备好 0 未准备
int XMSYS_CameraIsReady (void);

void XMSYS_ScaleD1FrameToD1Frame (int channel, unsigned char *srcFrame, unsigned char *dstFrame );

void XMSYS_CameraSetNtscPal (unsigned int NtscPal);

void XMSYS_CameraNotifyCdrEvent (void);


// UVC协议设置帧缩放比例因子
// 0 无缩放
// 1 长宽等比例缩小至1/2
// 2 长宽等比例缩小至1/4
void XMSYS_UVCCameraSetFrameRatio (unsigned int FrameRatio);

void XMSYS_H264CodecSetSharpFactor (signed char SharpFactor);
signed char XMSYS_H264CodecGetSharpFactor (void);

void XMSYS_H264CodecSetLumaFactor (signed char LumaFactor);
signed char XMSYS_H264CodecGetLumaFactor (void);

void XMSYS_H264CodecSetChromaFactor (signed char ChromaFactor);
signed char XMSYS_H264CodecGetChromaFactor (void);

// 检测视频输入端的插入状态(每个位表示通道的插入状态，1表示Video插入)
unsigned int XMSYS_CameraGetVideoStatus (void);

void XMSYS_SensorEnableDeinterlace (unsigned char Deinterlace);

/////////////////////////////////////////////////////////////////////
//                                                                 //
// ******************* H264Codec模块定义 **************************//
//                                                                 //
/////////////////////////////////////////////////////////////////////
#define	XMSYS_H264_CODEC_EVENT_COMMAND						0x01		// 外部命令事件
#define	XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL				0x02		// 切换录像通道
#define	XMSYS_H264_CODEC_EVENT_ONE_KEY_PROTECT				0x04		// 一键保护, 记录保护

// H264 codec外部命令定义
#define	XMSYS_H264_CODEC_COMMAND_RECORDER_START					1		// 录像记录启动命令
#define	XMSYS_H264_CODEC_COMMAND_RECORDER_STOP					2		// 录像记录停止命令
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_START					3		// 播放启动命令
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_STOP					4		// 播放停止命令
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_PAUSE					5
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_RESUME					6
#define	XMSYS_H264_CDOEC_COMMAND_PLAYER_FORWARD					7		// 回放前进
#define	XMSYS_H264_CDOEC_COMMAND_PLAYER_BACKWARD				8		// 回放后退
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_SEEK					9		// 定位到某个时间位置并播放
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_FRAME					10		// 逐帧播放命令

#define	XMSYS_H264_CODEC_COMMAND_ID							0x34363248

#define	XMSYS_H264_CODEC_MAX_FILE_NAME							63

// H264 Codec命令参数定义
typedef struct _H264CODE_COMMAND_PARAM {
	int		id;							//
	int		command;						// h264 codec命令
	char		file_name[XMSYS_H264_CODEC_MAX_FILE_NAME + 1];				// 播放文件全路径名
	char		file_name2[XMSYS_H264_CODEC_MAX_FILE_NAME + 1];				// 播放文件全路径名
	unsigned int	playback_mode;													// 初始回放模式
	unsigned int	seek_time;														// 定位到的时间点位置(单位 秒)
} H264CODEC_COMMAND_PARAM;

struct H264EncodeStream {
	int width[4];
	int height[4];
	
	char file_name[4][64];	// 流文件全路径名
	
	int file_save;			// 1 需要写入到文件
								// 0 不用写入到文件
	int file_lock;			// 1 标记流已保护
								// 0 标记流尚未保护
	
	int type;				// 0 file
								// 1 mem

	int		forced_640X576_mode;	// 1 强制源为640X576						

	void *video_file;		// 视频数据文件句柄, 测试使用
	void *audio_file;		// 音频数据文件句柄, 测试使用
		
	int (*get_video_frame)(void *EncodeStream, void *video_frame);
	int (*get_audio_frame)(void *EncodeStream, void *audio_frame);
	
	unsigned char *		audio_fifo;
	unsigned char *		audio_data;
	
	unsigned char *		Y[4];
	unsigned char *		U[4];
	unsigned char *		V[4];
	
	int						curr_ticket;		// 当前时间
	int						start_ticket;		// 开始时间
	int						stop_ticket;		// 结束时间
	// int						frame_rate;			// 每秒帧数 fps
	// 20180609 每个通道不同帧率设置
	unsigned char			frame_rate[4];		// 每秒帧数 fps
	int						frame_number;		// 文件中已记录的视频帧数量
	
	// Scale参数
	int						do_scale[4];			// 标记是否需要scale操作
	int						scale_width[4];
	int						scale_height[4];
	int						scale_stride[4];
	unsigned char *		scale_y[4];
	unsigned char *		scale_u[4];
	unsigned char *		scale_v[4];

	short	 sharp_ctrl_flag ;
	short	 luma_ctrl_flag ;
	short	 chroma_ctrl_flag ;
	
};

#define	XM_VIDEO_PLAYBACK_MODE_NORMAL	0		// 正常播放模式 (解码所有I帧及P帧并显示)
#define	XM_VIDEO_PLAYBACK_MODE_FORWARD	1		// 快进播放模式 (仅解码I帧，从头到尾顺序显示)
#define	XM_VIDEO_PLAYBACK_MODE_BACKWARD	2		// 快退播放模式 (仅解码I帧，从尾到头顺序显示)
#define	XM_VIDEO_PLAYBACK_MODE_FRAME		3		// 逐帧播放模式 (每次播放单帧, 等待按键继续下一帧)

struct H264DecodeStream {
	int						curr_ticket;
	
	char file_name[4][64];	// 流文件全路径名
		
	// Scale参数
	int						do_scale[4];			// 标记是否需要scale操作
	int						scale_width[4];
	int						scale_height[4];
	int						scale_stride[4];
	unsigned char *			scale_y[4];
	unsigned char *			scale_u[4];
	unsigned char *			scale_v[4];
	
	int file_lock;			// 1 标记流已保护
							// 0 标记流尚未保护
	
	unsigned int			width;
	unsigned int			height;
	unsigned int			frame_count;		// 帧总数，
												// 0 表示无法统计帧数，如视频头信息没有写入
	unsigned int			fps;				// 每秒采样帧数 (固定帧率编码)									
	unsigned int			frame_index;		// 当前播放帧索引
	
	unsigned int			playback_mode;		// 回放模式 (正常、快进、快退)	
	
	unsigned int			seek_time;			// 定位到的时间点(秒为单位)
	
	unsigned int			command_player_stop;		
														// 1 标记外部COMMAND_STOP命令终止解码
														//	0 其他原因，解码终止

	unsigned int			system_ticket;		// 从播放点开始的系统时间,-1表示需要定位起始帧的时间
	unsigned int			video_ticket;		// 从播放点开始的起始帧的帧显示时间	
	
	unsigned int			user_data[4][2048/4];		// 与帧相关的私有信息
	
};

unsigned long get_long (unsigned char *ch);
void set_long (unsigned char *ch, unsigned int v);

unsigned short get_word (unsigned char *ch);
void set_word (unsigned char *ch, unsigned short v);

unsigned short get_byte (unsigned char *ch);
void set_byte (unsigned char *ch, unsigned char v);

#define get_long(ch)		\
	((unsigned int)(*(unsigned char *)(ch) + (*((unsigned char *)(ch)+1) << 8) + (*((unsigned char *)(ch)+2) << 16) + (*((unsigned char *)(ch)+3) << 24)))

#define set_long(ch,v)	\
{	\
	*(unsigned char *)(ch) = (unsigned char)(v);	\
	*((unsigned char *)(ch) + 1) = (unsigned char)((v) >> 8);	\
	*((unsigned char *)(ch) + 2) = (unsigned char)((v) >> 16);	\
	*((unsigned char *)(ch) + 3) = (unsigned char)((v) >> 24);	\
}

#define	get_word(ch)	\
	((unsigned short)(*(unsigned char *)(ch) + (*((unsigned char *)(ch) + 1) << 8)))

#define set_word(ch,v)	\
{	\
	*((unsigned char *)(ch)) = (unsigned char)(v);	\
	*((unsigned char *)(ch) + 1) = (unsigned char)((v) >> 8);	\
}

#define	get_byte(ch)	\
	((unsigned char)(*(unsigned char *)(ch)))

#define set_byte(ch,v)	\
	*((unsigned char *)(ch)) = (unsigned char)(v);


/////////////////////////////////////////////////////////////////////
//                                                                 //
// ******************* APP 模块定义      **************************//
//                                                                 //
/////////////////////////////////////////////////////////////////////
#define	XMSYS_APP_KEYBD_EVENT				0x01		// 按键事件
#define	XMSYS_APP_TIMER_EVENT				0x02		// TIMER事件
void XMSYS_AppInit (void);
void XMSYS_AppExit (void);
void MotionTaskInit(void);

/////////////////////////////////////////////////////////////////////
//                                                                 //
// ******************* HARD TIMER 模块定义   **********************//
//                                                                 //
/////////////////////////////////////////////////////////////////////

// 初始化并创建模拟硬件的定时器（定时器线程具有最高线程优先级）
void XMSYS_HardTimerInit (void);
void XMSYS_HardTimerExit (void);


unsigned int XMSYS_SensorGetCameraChannel (void);
void XMSYS_SensorSetCameraChannel (unsigned int channel);
void XMSYS_SensorGetChannelSize (unsigned int channel, unsigned int *width, unsigned int *height);
void XMSYS_SensorStartCaptureAccount (void);
void XMSYS_SensorStopCaptureAccount (void);

unsigned int XMSYS_SensorGetCameraChannelWidth (unsigned int channel);
unsigned int XMSYS_SensorGetCameraChannelHeight (unsigned int channel);

void XMSYS_SensorResetUserData (XMSYSSENSORPACKET *packet);
void XMSYS_SensorSyncUserData (XMSYSSENSORPACKET *packet);

// 将私有数据加入到sensor私有数据缓存中(data!=NULL) 或者 仅分配空间用于后续的私有数据存储(user_data==NULL)
// 返回值指向存放私有数据的缓冲区地址，失败返回NULL
//		id				帧私有数据类型
//		user_data	私有数据地址
//		user_size	私有数据字节大小
char * XMSYS_SensorInsertUserDataIntoPacket (XMSYSSENSORPACKET *packet, unsigned int id, char *user_data, int user_size);
void XMSYS_DecodeUserData (char *isp_data, int isp_size );


extern int h264_encode_avi_stream (char *filename, char *filename2, struct H264EncodeStream *H264Stream,
											  const char **argv, int argc);
extern int  h264_decode_avi_stream (const char *filename, const char *filename2, struct H264DecodeStream *stream);

void XMSYS_H264CodecInit (void);

void XMSYS_H264CodecExit (void);



/////////////////////////////////////////////////////////////////////
//                                                                 //
// ***************** MESSAGE 系统消息模块定义  *********************//
//                                                                 //
/////////////////////////////////////////////////////////////////////
void XMSYS_MessageInit (void);
void XMSYS_MessageExit (void);


#if defined(CORE_HEAP_DEBUG)

// 内核内存分配函数, 允许并发访问
void* _kernel_malloc(unsigned int n, char *file, int line);
void* _kernel_mallocz (unsigned int n, char *file, int line);
void  _kernel_free  (void* pMemBlock, char *file, int line);
void *_kernel_realloc(void *ptr, unsigned int size, char *file, int line);
void* _kernel_calloc (unsigned int n, unsigned int s, char *file, int line);

#define	kernel_malloc(size)		_kernel_malloc(size,__FILE__,__LINE__)
#define	kernel_mallocz(size)		_kernel_mallocz(size,__FILE__,__LINE__)
#define	kernel_free(mem)			_kernel_free(mem,__FILE__,__LINE__)
#define	kernel_realloc(ptr,size)		_kernel_realloc(ptr,size,__FILE__,__LINE__)
#define	kernel_calloc(n,s)		_kernel_calloc(n,s,__FILE__,__LINE__)

#else

// 内核内存分配函数, 允许并发访问
void* kernel_malloc(unsigned int n);
void* kernel_mallocz (unsigned int n);
void  kernel_free  (void* pMemBlock);
void *kernel_realloc(void *ptr, unsigned int size);
void* kernel_calloc (unsigned int n, unsigned int s);

#endif

// JPEG任务初始化
void XMSYS_JpegInit (void);

// JPEG任务结束
void XMSYS_JpegExit (void);

// 一键拍照任务初始化
void XMSYS_OneKeyPhotographInit (void);
// 一键拍照任务结束
void XMSYS_OneKeyPhotographExit (void);

// 停止视频编解码器
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecStop (void);

// 一键保护
// 暂时未完整实现，此处仅简单实现
int XMSYS_H264CodecOneKeyProtect (void);

// 启动视频录像
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecRecorderStart (void);

// 启动一键拍照功能
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
// 暂时未完整实现，此处仅简单实现
int XMSYS_JpegCodecOneKeyPhotograph (void);

// 停止录像记录
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecRecorderStop  (void);

// 检查H264编解码是否忙
// 1 busy
// 0 idle
int XMSYS_H264CodecCheckBusy (void);
// 强制codec退出
void XMSYS_H264CodecForcedEncoderStop (void);

// 将当前录制视频锁定
void XMSYS_H264CodecLockCurrentVideo (void);

// 将当前录制视频解锁
void XMSYS_H264CodecUnLockCurrentVideo (void);

// 获取当前录像视频的锁定状态
// 1 锁定中
// 0 未锁定
unsigned int XMSYS_H264CodecGetVideoLockState (void);

// 656/601 IN
#ifdef EN_RN6752M1080N_CHIP
#define	ITU656_IN_MAX_WIDTH					960
#define	ITU656_IN_MAX_HEIGHT				1080
#else
#define	ITU656_IN_MAX_WIDTH					1280
#define	ITU656_IN_MAX_HEIGHT				720
#endif

#define	ITU656_IN_FRAME_COUNT				4
// ISP-Scalar IN
#define	ISP_SCALAR_FRAME_MAX_WIDTH		1920
#define	ISP_SCALAR_FRAME_MAX_HEIGHT		1080
#define	ISP_SCALAR_FRAME_COUNT			4
#define	ISP_SCALAR_STATIC_FIFO_SIZE	((ISP_SCALAR_FRAME_MAX_WIDTH*ISP_SCALAR_FRAME_MAX_HEIGHT*3/2 + 2048) * ISP_SCALAR_FRAME_COUNT)

#ifdef ISP_SCALAR_USE_STATIC_FIFO
extern char isp_scalar_static_fifo[ISP_SCALAR_STATIC_FIFO_SIZE];
#endif

char *isp_scalar_get_frame_buffer (unsigned int frame_id, unsigned int frame_size);
char *itu656_in_get_frame_buffer  (unsigned int frame_id, unsigned int frame_size);

u32_t itu656_in_get_video_width  (void);
u32_t itu656_in_get_video_height (void);



// G-Sensor初始化
void XMSYS_GSensorInit (void);
void XMSYS_GSensorExit (void);

// 检查并清除碰撞事件
// 返回值
//		1		碰撞已发生
//		0		碰撞未发生
int XMSYS_GSensorCheckAndClearCollisionEvent (void);

// 检查是否是停车碰撞启动
//	返回值
//		1		停车碰撞启动
//		0		非停车碰撞启动
int XMSYS_GSensorCheckParkingCollisionStartup (void);

int OS_Use_timeout (OS_RSEMA *sema, unsigned int ms_to_timeout);

// ARK7116驱动
void XMSYS_Ark7116Init (void);
void XMSYS_Ark7116Exit (void);

//NVP6134C驱动
void XMSYS_Nvp6134cInit (void);
void XMSYS_Nvp6134cExit (void);


//NVP6124B驱动
void XMSYS_Nvp6124bInit (void);
void XMSYS_Nvp6124bExit (void);


//PR2000驱动
void XMSYS_Pr2000Init (void);
void XMSYS_Pr2000Exit (void);

void XMSYS_Itu601ScalerInit (void);
void XMSYS_Itu601ScalerExit (void);

// 设置后置摄像头视频图像
void set_real_video_image (unsigned char *image, unsigned int w, unsigned int h);
// 获取后置摄像头视频图像
unsigned char *get_real_video_image (void);

// 设置强制非记录模式
// forced_non_recording
//		1			强制非记录模式 (该模式下不再录像, 除非清除该模式)
//		0			普通记录模式 
void XMSYS_H264CodecSetForcedNonRecordingMode (int forced_non_recording);

// 检查当前模式是否是强制非记录模式
// 返回值
// 	1			强制非记录模式 (该模式下不再录像, 除非清除该模式)
//		0			普通记录模式 
int XMSYS_H264CodecGetForcedNonRecordingMode (void);

void xm_video_display_init (void);
// 获取一个空闲的视频缓存帧
XMSYSSENSORPACKET *xm_video_display_get_free_frame (int channel);
// 标记一个视频缓存帧数据已准备好，可以用于显示
int xm_video_display_set_data_frame_ready (XMSYSSENSORPACKET *data_frame);
// 获取一个有效的视频数据帧
XMSYSSENSORPACKET *xm_video_display_get_data_frame (void);
// 标记视频数据缓存帧不再使用
int xm_video_display_free_data_frame (XMSYSSENSORPACKET *data_frame);

// 进入关键区, 保护7116初始化过程
void enter_region_to_protect_7116_setup (void);
void leave_region_to_protect_7116_setup (void);

XMSYSSENSORPACKET * XMSYS_SensorGetCurrentPacket (int channel);
void xm_itu601_scalar_start (void);
void xm_itu601_scalar_stop (void);

void XM_AppInitStartupTicket (void);

// 请求片内IRAM共享资源访问权
void enter_region_to_access_iram (void);
// 释放片内IRAM共享资源访问权
void leave_region_to_access_iram (void);


#ifdef __cplusplus
}
#endif

#endif	// _XMCORE_H_
