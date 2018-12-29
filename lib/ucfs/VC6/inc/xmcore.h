#ifndef _XMCORE_H_
#define _XMCORE_H_

#ifdef __cplusplus
extern "C" {
#endif
	
#include <xmtype.h>
	
#include <xm_proj_define.h>
	
#include "rtos.h"


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
#define	XMSYS_IPRX_TASK_PRIORITY				199
#define	XMSYS_DHCPD_TASK_PRIORITY				100			// DHCPD服务		
	
#define	XMSYS_VIDEO_TASK_PRIORITY				120
#define	XMSYS_SENSOR_TASK_PRIORITY				200
#define	XMSYS_CAMERA_TASK_PRIORITY				202			// 最高优先级，确保插拔时满足协议要求的时间要求
#define	XMSYS_GPS_TASK_PRIORITY					120

// 异常情况
// 1 XMSYS_H264CODEC_TASK_PRIORITY=130  XMSYS_H264File_TASK_PRIORITY=131 XMSYS_VIDEO_TASK_PRIORITY=120
//   XMSYS_SENSOR_TASK_PRIORITY=200	 XMSYS_APP_TASK_PRIORITY=120
//   	
//   每次运行均死机(待查)
//   Capture FPS = 21 Encode FPS = 12	
//
//   **************************	
// 2	XMSYS_H264CODEC_TASK_PRIORITY=201  XMSYS_H264File_TASK_PRIORITY=131 XMSYS_VIDEO_TASK_PRIORITY=120
//    XMSYS_SENSOR_TASK_PRIORITY=200	 XMSYS_APP_TASK_PRIORITY=120
//	
//		编码40分钟，暂时未出现死机
//		Capture FPS = 22.171177, Encode FPS = 15.149779
//		maximum_search_time=974	
//   	
//   **************************	
// 3	XMSYS_H264CODEC_TASK_PRIORITY=132  XMSYS_H264File_TASK_PRIORITY=131 XMSYS_VIDEO_TASK_PRIORITY=120
//   XMSYS_SENSOR_TASK_PRIORITY=200	 XMSYS_APP_TASK_PRIORITY=120
//   	
//   运行死机(待查)
// 	
//	
// 4	XMSYS_H264CODEC_TASK_PRIORITY=201  XMSYS_H264File_TASK_PRIORITY=202 XMSYS_VIDEO_TASK_PRIORITY=120
//   XMSYS_SENSOR_TASK_PRIORITY=200	 XMSYS_APP_TASK_PRIORITY=120
//   	
//   编码40分钟，暂时未出现死机
//   Capture FPS = 16.746562, Encode FPS = 11.240063
//	  291 Maximum  Cache Write
//   maximum_search_time=18
//
//   **********************************	
//   初步结论
//   **********************************	
//	  1) XMSYS_H264CODEC_TASK_PRIORITY需高于XMSYS_SENSOR_TASK_PRIORITY,则不会出现死机
//	  2) XMSYS_H264CODEC_TASK_PRIORITY高于XMSYS_H264File_TASK_PRIORITY，则H264 Encode FPS较高
//      ，原因是阻止文件系统过快的写入，使得后续的fseek写入操作与前面的写入数据一起写入到SD卡，减少写入次数	
//   3) 死机的原因已找到，与XMSYS_SensorGetHardwareAddress及XMSYS_SensorStartCapture的互斥访问相关。
//				
	
//#define	XMSYS_H264CODEC_TASK_PRIORITY			201		// H264编码线程优先级


// DVR 4个摄像头项目无文件保存功能，仅Camera采集、H264编码及USB Camera功能
// 设置H264 Codec任务最高优先级，可以达到每秒最大编码帧率	
#define	XMSYS_H264CODEC_TASK_PRIORITY			206		// H264编码线程优先级
#define	XMSYS_H264File_TASK_PRIORITY			251		// H264写入线程优先级，应高于编码线程
	
#define	XMSYS_APP_TASK_PRIORITY					200	//120		//	UI线程拥有最高优先级
	
#define	XMSYS_JPEG_TASK_PRIORITY				120		//	JPEG线程
	
#define	XMSYS_CDR_TASK_PRIORITY					133	

#define	XMSYS_HARD_TIMER_TASK_PRIORITY		255		//	硬件定时器服务任务（具有最高任务优先级）
#define	XMSYS_MESSAGE_TASK_PRIORITY			254		//	系统消息服务任务，仅低于硬件定时器任务
	

//#define	XMSYS_USBHOST_TASK_PRIORITY			133		// USB HOST的服务任务		
#define	XMSYS_USBHOST_TASK_PRIORITY			233		// USB HOST的服务任务			
	
#define	XMSYS_MAIN_TASK_STACK_SIZE				(0x20000)			// 8KB
#define	XMSYS_HARD_TIMER_STASK_SIZE			(0x1000)			// 硬件定时器任务栈
#define	XMSYS_SENSOR_TASK_STACK_SIZE			(0x8000)			// 1KB
#define	XMSYS_H264CODEC_TASK_STACK_SIZE		(0x18000)		// 64KB
#define	XMSYS_PLAYBACK_TASK_STACK_SIZE		(0x0400)			// 1KB	
#define	XMSYS_RECEIVE_TASK_STACK_SIZE			(0x0400)			// 1KB
#define	XMSYS_TRANSFER_TASK_STACK_SIZE		(0x0400)			// 1KB
#define	XMSYS_VIDEO_TASK_STACK_SIZE			(0x2000)			// 1KB
#define	XMSYS_CAMERA_TASK_STACK_SIZE			(0x4000)			// 1KB
#define	XMSYS_IP_TASK_STACK_SIZE				(0x0400)			// 1KB
#define	XMSYS_IPRX_TASK_STACK_SIZE				(0x0400)			// 1KB
#define	XMSYS_DHCPD_TASK_STACK_SIZE			(0x0200)			// 512
#define	XMSYS_GPS_TASK_STACK_SIZE				(0x0400)			// 1KB
#define	XMSYS_H264FILE_TASK_STACK_SIZE		(0xC000)			// 32KB	// 需要访问文件系统	

#define	XMSYS_JPEG_TASK_STACK_SIZE				(0x1000)			// 1KB
#define	XMSYS_USBHOST_TASK_STACK_SIZE			(4*1024)
	
	
#define	XMSYS_APP_TASK_STACK_SIZE				(0x20000)			// 32KB	UI用户线程
	
#define	XMSYS_CDR_TASK_STACK_SIZE				(0x8000)			// 需要访问文件系统	
	
#define	XMSYS_MESSAGE_TASK_STACK_SIZE			(0x2000)
	

//
// ***** 4路D1 DVR 系统内存分配定义 **************
//
// 0x81000000 ~ 0x810FFFFF 1MB的scale缓冲区，保存scale后的D1帧
// 0x81100000 ~ 0x812FFFFF	 第一帧混合帧(MJPEG图片 + 4路D1 H264码流 + 2路音频码流)
// 0x81300000 ~ 0x814FFFFF  第二帧混合帧(MJPEG图片 + 4路D1 H264码流 + 2路音频码流)
// 0x81500000 ~ 0x816FFFFF  FILE LOAD/SAVE BUFFER
// 0x81700000 ~ 0x81EFFFFF H264硬件编码, 保留8MB用于H264硬件编码

#define XMSYS_D1_COMPOSE_BASE						0x82000000
#define XMSYS_D1_COMPOSE_SIZE						0x00100000
#define XMSYS_HYBRID_FRAME_BASE					(XMSYS_D1_COMPOSE_BASE + XMSYS_D1_COMPOSE_SIZE)
#define XMSYS_HYBRID_FRAME_SIZE					0x00200000
#define XMSYS_HYBRID_FRAME_BASE_1				XMSYS_HYBRID_FRAME_BASE
#define XMSYS_HYBRID_FRAME_BASE_2				(XMSYS_HYBRID_FRAME_BASE_1 + XMSYS_HYBRID_FRAME_SIZE)

#define XMSYS_HYBRID_FRAME_LOAD_FIFO			(XMSYS_HYBRID_FRAME_BASE_2 + XMSYS_HYBRID_FRAME_SIZE)
#define XMSYS_HYBRID_FRAME_SAVE_FIFO			(XMSYS_HYBRID_FRAME_LOAD_FIFO)

#define XMSYS_CAMERA_BUFF_BASE					(XMSYS_HYBRID_FRAME_LOAD_FIFO + XMSYS_HYBRID_FRAME_SIZE)
#define XMSYS_CAMERA_BUFF_SIZE					0x00100000

	
// H264编码内存定义	
#define	XMSYS_H264_ENC_POOL_BASE					(XMSYS_CAMERA_BUFF_BASE + XMSYS_CAMERA_BUFF_SIZE)
#define	XMSYS_H264_ENC_POOL_SIZE					(0x800000)		// 保留8MB用于H264硬件编码

	
	

// JPEG编码区域定义
#define	XMSYS_JPEG_FRAME_BASE					(XMSYS_D1_COMPOSE_BASE + XMSYS_DI_COMPOSE_SIZE)	
#define	XMSYS_JPEG_FRAME_SIZE					0x00200000
#define	XMSYS_JPEG_FRAME_BASE_1					XMSYS_JPEG_FRAME_BASE	
#define	XMSYS_JPEG_FRAME_BASE_2					(XMSYS_JPEG_FRAME_BASE_1 + XMSYS_JPEG_FRAME_SIZE)

// JPEG使用的VGA缓冲区基址	
#define	XMSYS_JPEG_SCALE_VGA_BASE				(XMSYS_JPEG_FRAME_BASE_2 + XMSYS_JPEG_FRAME_SIZE)
// JPEG使用的QVGA缓冲区基址		
#define	XMSYS_JPEG_SCALE_QVGA_BASE				(XMSYS_JPEG_FRAME_BASE_2 + XMSYS_JPEG_FRAME_SIZE)




// 系统网络模块初始化
void SYS_NetworkModuleInit (void);


// 系统工作模式定义 (PC CAMERA模式 , VIDEO_RECORD记录模式)
#define	XMSYS_WORKMODE_VIDEOREC			0		// 行车记录模式
#define	XMSYS_WORKMODE_PCCAMERA			1		// PCCamera模式

// 获取当前系统的工作模式
int	XMSYS_GetWorkMode (void);
// 设置当前系统的工作模式
void	XMSYS_SetWorkMode	(int WorkMode);

/////////////////////////////////////////////////////////////////////
//                                                                 //
// ******************* Video模块定义 ******************************//
//                                                                 //
/////////////////////////////////////////////////////////////////////

// VIDEO COMMAND PARAMETER定义
#define	XMSYS_VIDEO_LCD_OFF				0
#define	XMSYS_VIDEO_LCD_ON				1
#define	XMSYS_VIDEO_OSD_OFF				0
#define	XMSYS_VIDEO_OSD_ON				1
#define	XMSYS_VIDEO_VIEW_OFF				0
#define	XMSYS_VIDEO_VIEW_ON				1
#define	XMSYS_VIDEO_BACKLIGHT_OFF		0
#define	XMSYS_VIDEO_BACKLIGHT_ON		1
#define	XMSYS_VIDEO_TIMESTAMP_OFF		0
#define	XMSYS_VIDEO_TIMESTAMP_ON		1

// 前后视图模式VIDEO VIEW MODE定义
#define	XMSYS_VIDEO_VIEW_MODE_FRONT				0		// 单显前视图
#define	XMSYS_VIDEO_VIEW_MODE_REAR					1		// 单显后视图
#define	XMSYS_VIDEO_VIEW_MODE_FRONT_REAR			2		// 前主后副
#define	XMSYS_VIDEO_VIEW_MODE_REAR_FRONT			3		// 前副后主

#define	XMSYS_VIDEO_VIEW_MODE_MAIN					0		// 合成模式
#define	XMSYS_VIDEO_VIEW_MODE_CH_0					1		// 通道0单显
#define	XMSYS_VIDEO_VIEW_MODE_CH_1					2		// 通道1单显
#define	XMSYS_VIDEO_VIEW_MODE_CH_2					3		// 通道0单显
#define	XMSYS_VIDEO_VIEW_MODE_CH_3					4		// 通道1单显


// VIDEO COMMAND定义
#define	XMSYS_VIDEO_COMMAND_LCD_ONOFF				0		// ON/OFF设置
#define	XMSYS_VIDEO_COMMAND_LCD_BACKLIGHT		1		// 背光设置
#define	XMSYS_VIDEO_COMMAND_OSD_ONOFF				2		// OSD层(UI)显示开启/关闭
#define	XMSYS_VIDEO_COMMAND_VIEW_ONOFF			3		// 视频层开启/关闭
#define	XMSYS_VIDEO_COMMAND_VIEW_MODE				4		// 前后视图模式设置
#define	XMSYS_VIDEO_COMMAND_TIMESTAMP_ONOFF		5		// 时间戳开启/关闭

// VIDEO_TASK的事件定义
#define	XMSYS_VIDEO_UI_COMMAND_EVENT				0x01		// VIDEO控制命令
#define	XMSYS_VIDEO_UI_VIEW_EVENT					0x02		// UI显示更新事件
#define	XMSYS_VIDEO_FRONT_VIEW_PACKET_EVENT		0x04		// 前置Sensor显示更新
#define	XMSYS_VIDEO_REAR_VIEW_PACKET_EVENT		0x08		// 后置Sensor显示更新


// 获取前置Sensor YUV包缓冲
int *XMSYS_VideoGetFrontViewPacket (void);
// 释放前置Sensor YUV包缓冲
void XMSYS_VideoPutFrontViewPacket (void);
// 获取后置Sensor YUV包缓冲
int *XMSYS_VideoGetRearViewPacket (void);
// 释放后置Sensor YUV包缓冲
void XMSYS_VideoPutRearViewPacket (void);

int *XMSYS_VideoGetViewPacket (int channel);
int *XMSYS_VideoLockViewPacket (int channel);
void XMSYS_VideoUnlockViewPacket (int channel);

// 直接获取一个视频FrameBuffer
unsigned char *XMSYS_VideoRequestVideoFrameBuffer (void);
// 释放一个视频FrameBuffer
void XMSYS_VideoReleaseVideoFrameBuffer (unsigned char *framebuffer);

VOID	XMSYS_VideoScaleCopyVideoImageIntoFrameBuffer (int camera_index, unsigned char *video_buffer);



void XMSYS_VideoInit (void);
void XMSYS_VideoExit (void);

void XMSYS_VideoTask (void);

void video_driver_update_video (int ch, char *video_buffer, int width, int height);
void video_driver_init (void);

void  scalar_copy(
	unsigned char *Yin  , unsigned char *Uin  ,  unsigned char  *Vin ,
	unsigned char *Yout, unsigned char *Uout, unsigned char  *Vout ,
	int  in_width, int  in_height, int in_stride, int  out_width, int  out_height, int out_stride
	);

// D1隔行扫描转换为D1逐行扫描
void  scalar_copy_D1ToD1(
	unsigned char *Yin  , unsigned char *Uin  ,  unsigned char  *Vin ,
	unsigned char *Yout, unsigned char *Uout, unsigned char  *Vout ,
	int  in_width, int  in_height, int in_stride, int  out_width, int  out_height, int out_stride
	);

void  block_copy(
	unsigned char *src  , unsigned char *dst  ,
	int  in_width, int  in_height, int in_stride, int out_stride
	);


int XMVideo_CalcTimeStampPosition (int channel, int *x, int *y, int *w, int *h);

void XMVideo_FillTimeStamp (unsigned char *osd_buffer, int w, int h);

void XMVideo_OSDLayerInit (void);
void XMVideo_OSDLayerExit (void);



//void Dhcpd_Task (void);


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
	unsigned int		locked;		// 标记正在锁定使用中
	unsigned int		used;			// 标记是否已使用。用于统计丢弃的帧
	unsigned int		scaled;		// 标记是否已缩放
		
	char *				buffer;		// 指向YUV缓冲区
	
//	unsigned long		capture_start_ticket;		//  记录启动时间
	
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

int XMSYS_SensorStartCapture (int channel, int *Frame);

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

// Sensor硬件初始化
void XMSYS_SensorHardwareInit (void);

// 创建一个1280*960的D1X4组合帧
XMSYSSENSORPACKET * XMSYS_SensorCreate4D1ComposedSensorPacket (void);
// 删除一个1280*960的D1X4组合帧
void XMSYS_SensorDelete4D1ComposedSensorPacket (XMSYSSENSORPACKET *CompFrame);

XMSYSSENSORPACKET * XMSYS_SensorCreatePacket (int channel);
void XMSYS_SensorDeletePacket (int channel, XMSYSSENSORPACKET *packet);

void XMSYS_SensorStartCaptureAccount (void);
void XMSYS_SensorStopCaptureAccount (void);

// 设置/获取D1摄像头的制式(NTSC或PAL)
void XMSYS_SensorSetD1SensorType (unsigned char D1SensorType);
unsigned char XMSYS_SensorGetD1SensorType (void);



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

// 错误码定义
#define	XMSYS_CAMERA_SUCCESS								0			// 成功
#define	XMSYS_CAMERA_ERROR_USB_DISCONNECT				(-1)		// USB连接断开
#define	XMSYS_CAMERA_ERROR_USB_EXCEPTION				(-2)		// USB传输异常
#define	XMSYS_CAMERA_ERROR_USB_UVCSTOP					(-3)		// USB UVC终止
#define	XMSYS_CAMERA_ERROR_OTHER							(-4)		// 其他异常

#define	XMSYS_CAMERA_ERROR_MODE_ERROR					(-5)		// CDR模式不支持
#define	XMSYS_CAMERA_ERROR_SAVE_FILE_OPEN				(-6)		// 本地CDR保存文件没有打开
#define	XMSYS_CAMERA_ERROR_SAVE_FILE_WRITE				(-7)		// 本地CDR保存文件写入失败

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

void XMSYS_ScaleD1FrameToD1Frame (int channel, unsigned char *srcFrame, unsigned char *dstFrame );

void XMSYS_CameraSetNtscPal (unsigned int NtscPal);

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
#define	XMSYS_H264_CODEC_EVENT_RECORD_PROTECT				0x02		// 记录保护

// H264 codec外部命令定义
#define	XMSYS_H264_CODEC_COMMAND_RECORDER_START				1		// 录像记录启动命令
#define	XMSYS_H264_CODEC_COMMAND_RECORDER_STOP					2		// 录像记录停止命令
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_START					3		// 播放启动命令
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_STOP					4		// 播放停止命令
#define	XMSUS_H264_CODEC_COMMAND_PLAYER_PAUSE					5
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_RESUME					6

#define	XMSYS_H264_CODEC_COMMAND_ID							0x34363248

#define	XMSYS_H264_CODEC_MAX_FILE_NAME							63
// H264 Codec命令参数定义
typedef struct _H264CODE_COMMAND_PARAM {
	int		id;							//
	int		command;						// h264 codec命令
	char		file_name[XMSYS_H264_CODEC_MAX_FILE_NAME + 1];				// 播放文件全路径名
	char		file_name2[XMSYS_H264_CODEC_MAX_FILE_NAME + 1];				// 播放文件全路径名
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
	int						start_ticket;
	int						stop_ticket;		// 结束时间
	int						frame_rate;			// fps
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


struct H264DecodeStream {
	int						curr_ticket;
	
	char file_name[4][64];	// 流文件全路径名
		
	// Scale参数
	int						do_scale[4];			// 标记是否需要scale操作
	int						scale_width[4];
	int						scale_height[4];
	int						scale_stride[4];
	unsigned char *		scale_y[4];
	unsigned char *		scale_u[4];
	unsigned char *		scale_v[4];
	
	int file_lock;			// 1 标记流已保护
								// 0 标记流尚未保护
	
	unsigned int			width;
	unsigned int			height;
	unsigned int			frame_count;		// 帧总数，
														//		0 表示无法统计帧数，如视频头信息没有写入
	unsigned int			fps;					// 每秒采样帧数 (固定帧率编码)									
	unsigned int			frame_index;		// 当前播放帧索引
	
	unsigned int			playback_mode;		// 回放模式 (正常、快进、快退)													
	unsigned int			seek_time;			// 定位到的时间点(秒为单位)
	
};

unsigned long get_long (unsigned char *ch);
void set_long (unsigned char *ch, unsigned int v);

/////////////////////////////////////////////////////////////////////
//                                                                 //
// ******************* APP 模块定义      **************************//
//                                                                 //
/////////////////////////////////////////////////////////////////////
#define	XMSYS_APP_KEYBD_EVENT				0x01		// 按键事件
#define	XMSYS_APP_TIMER_EVENT				0x02		// TIMER事件
void XMSYS_AppInit (void);
void XMSYS_AppExit (void);

/////////////////////////////////////////////////////////////////////
//                                                                 //
// *******************  HYBRID(DVR) 模块定义  *********************//
//                                                                 //
/////////////////////////////////////////////////////////////////////

#define	HYBRID_FRAME_ID			0x44425948

#define	HYBRID_H264_ID				0x00E0FFFF

#define	XMSYS_HYBRID_FLAG_RETRANSFER			0x000000001		// 指示重传
#define	XMSYS_HYBRID_FLAG_FATALERROR			0x800000000		// 指示VDR系统错误，

void XMSYS_HybridFrameInit (void);
void XMSYS_HybridFrameExit (void);

unsigned int XMSYS_HybridGetH264DataOffset (void);


// 创建一个新的混合帧
void * XMSYS_HybridFrameCreate (void);

// 混合帧数据已准备完毕，提交等待USB传输
void XMSYS_HybridFrameSubmit (void *lpHybridFrame);

// 将一个H264视频帧加入到混合帧中
int XMSYS_HybridFrameInsertVideoStream (void *lpHybridFrame, int VideoChannel,
													 unsigned long TimeStamp, void *lpVideoStream, int cbVideoStream, int i_frame);

// 将一个音频帧加入到混合帧中
int XMSYS_HybridFrameInsertAudioStream (void *lpHybridFrame, int AudioChannel,
													 unsigned long TimeStamp, void *lpAudioStream, int cbAudioStream);

// 将一个keyid加入到混合帧中
int XMSYS_HybridFrameInsertKeyidStream (void *lpHybridFrame, void *lpKeyidStream, int cbKeyidStream);

// 获取DVR记录模式(实时传输、本地保存、本地传输)
int XMSYS_HybridGetDvrMode (void);
// // 设置CDR工作模式 (实时传输、本地保存、本地传输)
void XMSYS_HybridSetDvrMode (int mode);

// 1 上传KEYID信息，0 关闭上传KEYID信息
void XMSYS_HybridForcedKeyID (int force_keyid);

// 设置帧率，1-29数字，表示每秒多少帧
void XMSYS_H264CodecSetFrameRate (unsigned int FrameRate);

// 根据给定的CDR帧数据创建一个文件上传混合帧(用于本地保存的CDR数据上传，每次传输一个或多个CDR帧)
void * XMSYS_HybridFrameCreateCdrLoadFrame (void *lpCdrFrame, int cbCdrFrame, int cdr_frame_count);

// 将一个LocalFileInfo加入到混合帧中
int XMSYS_HybridFrameInsertLocalFileInfoStream (void *lpHybridFrame, void *lpLocalFileInfoStream, int cbLocalFileInfoStream);

// 将CDR上传帧的通道数据插入到混合帧
int XMSYS_HybridFrameInsertCdrLoadFrameChannelData (void *lpHybridFrame, 
																	 void *lpCdrFrameChannelData, int cbCdrFrameChannelData);


/////////////////////////////////////////////////////////////////////
//                                                                 //
// ******************* HARD TIMER 模块定义   **********************//
//                                                                 //
/////////////////////////////////////////////////////////////////////

// 初始化并创建模拟硬件的定时器（定时器线程具有最高线程优先级）
void XMSYS_HardTimerInit (void);
void XMSYS_HardTimerExit (void);


/////////////////////////////////////////////////////////////////////
//                                                                 //
// ***************** MESSAGE 系统消息模块定义  *********************//
//                                                                 //
/////////////////////////////////////////////////////////////////////
void XMSYS_MessageInit (void);
void XMSYS_MessageExit (void);

#ifdef __cplusplus
}
#endif

#endif	// _XMCORE_H_
