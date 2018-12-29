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
	

	
// �ں��̶߳���
// 1)  ���߳� MAIN_TASK (UI�߳�)
//	2)  SENSOR���� SensorTask	
// 3)  ¼���¼���� RecordTask
// 4)  ¼��ط����� PlaybackTask
// 5)  ���������� ReceiveTask
// 6)  ���������� TransferTask
// 7)  Video��ʾ���� VideoTask
// 8)  PC Camera����CameraTask
// 9)  GPS�켣��¼���� GpsTask
// 10) ϵͳ������� SystemMonitorTask
// 11) ������������ VoiceTask
// 12) IP����
// 13) IP_RECV����
// 14) DHCPD dhcp��������(Ϊ��������IP��ַ)
	
// �������ȼ�����(ֵԽ�����ȼ�Խ��)
#define	XMSYS_IP_TASK_PRIORITY					200
#define	XMSYS_IPRX_TASK_PRIORITY				199
#define	XMSYS_DHCPD_TASK_PRIORITY				100			// DHCPD����		
	
#define	XMSYS_VIDEO_TASK_PRIORITY				120
#define	XMSYS_SENSOR_TASK_PRIORITY				200
#define	XMSYS_CAMERA_TASK_PRIORITY				202			// ������ȼ���ȷ�����ʱ����Э��Ҫ���ʱ��Ҫ��
#define	XMSYS_GPS_TASK_PRIORITY					120

// �쳣���
// 1 XMSYS_H264CODEC_TASK_PRIORITY=130  XMSYS_H264File_TASK_PRIORITY=131 XMSYS_VIDEO_TASK_PRIORITY=120
//   XMSYS_SENSOR_TASK_PRIORITY=200	 XMSYS_APP_TASK_PRIORITY=120
//   	
//   ÿ�����о�����(����)
//   Capture FPS = 21 Encode FPS = 12	
//
//   **************************	
// 2	XMSYS_H264CODEC_TASK_PRIORITY=201  XMSYS_H264File_TASK_PRIORITY=131 XMSYS_VIDEO_TASK_PRIORITY=120
//    XMSYS_SENSOR_TASK_PRIORITY=200	 XMSYS_APP_TASK_PRIORITY=120
//	
//		����40���ӣ���ʱδ��������
//		Capture FPS = 22.171177, Encode FPS = 15.149779
//		maximum_search_time=974	
//   	
//   **************************	
// 3	XMSYS_H264CODEC_TASK_PRIORITY=132  XMSYS_H264File_TASK_PRIORITY=131 XMSYS_VIDEO_TASK_PRIORITY=120
//   XMSYS_SENSOR_TASK_PRIORITY=200	 XMSYS_APP_TASK_PRIORITY=120
//   	
//   ��������(����)
// 	
//	
// 4	XMSYS_H264CODEC_TASK_PRIORITY=201  XMSYS_H264File_TASK_PRIORITY=202 XMSYS_VIDEO_TASK_PRIORITY=120
//   XMSYS_SENSOR_TASK_PRIORITY=200	 XMSYS_APP_TASK_PRIORITY=120
//   	
//   ����40���ӣ���ʱδ��������
//   Capture FPS = 16.746562, Encode FPS = 11.240063
//	  291 Maximum  Cache Write
//   maximum_search_time=18
//
//   **********************************	
//   ��������
//   **********************************	
//	  1) XMSYS_H264CODEC_TASK_PRIORITY�����XMSYS_SENSOR_TASK_PRIORITY,�򲻻��������
//	  2) XMSYS_H264CODEC_TASK_PRIORITY����XMSYS_H264File_TASK_PRIORITY����H264 Encode FPS�ϸ�
//      ��ԭ������ֹ�ļ�ϵͳ�����д�룬ʹ�ú�����fseekд�������ǰ���д������һ��д�뵽SD��������д�����	
//   3) ������ԭ�����ҵ�����XMSYS_SensorGetHardwareAddress��XMSYS_SensorStartCapture�Ļ��������ء�
//				
	
//#define	XMSYS_H264CODEC_TASK_PRIORITY			201		// H264�����߳����ȼ�


// DVR 4������ͷ��Ŀ���ļ����湦�ܣ���Camera�ɼ���H264���뼰USB Camera����
// ����H264 Codec����������ȼ������Դﵽÿ��������֡��	
#define	XMSYS_H264CODEC_TASK_PRIORITY			206		// H264�����߳����ȼ�
#define	XMSYS_H264File_TASK_PRIORITY			251		// H264д���߳����ȼ���Ӧ���ڱ����߳�
	
#define	XMSYS_APP_TASK_PRIORITY					200	//120		//	UI�߳�ӵ��������ȼ�
	
#define	XMSYS_JPEG_TASK_PRIORITY				120		//	JPEG�߳�
	
#define	XMSYS_CDR_TASK_PRIORITY					133	

#define	XMSYS_HARD_TIMER_TASK_PRIORITY		255		//	Ӳ����ʱ���������񣨾�������������ȼ���
#define	XMSYS_MESSAGE_TASK_PRIORITY			254		//	ϵͳ��Ϣ�������񣬽�����Ӳ����ʱ������
	

//#define	XMSYS_USBHOST_TASK_PRIORITY			133		// USB HOST�ķ�������		
#define	XMSYS_USBHOST_TASK_PRIORITY			233		// USB HOST�ķ�������			
	
#define	XMSYS_MAIN_TASK_STACK_SIZE				(0x20000)			// 8KB
#define	XMSYS_HARD_TIMER_STASK_SIZE			(0x1000)			// Ӳ����ʱ������ջ
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
#define	XMSYS_H264FILE_TASK_STACK_SIZE		(0xC000)			// 32KB	// ��Ҫ�����ļ�ϵͳ	

#define	XMSYS_JPEG_TASK_STACK_SIZE				(0x1000)			// 1KB
#define	XMSYS_USBHOST_TASK_STACK_SIZE			(4*1024)
	
	
#define	XMSYS_APP_TASK_STACK_SIZE				(0x20000)			// 32KB	UI�û��߳�
	
#define	XMSYS_CDR_TASK_STACK_SIZE				(0x8000)			// ��Ҫ�����ļ�ϵͳ	
	
#define	XMSYS_MESSAGE_TASK_STACK_SIZE			(0x2000)
	

//
// ***** 4·D1 DVR ϵͳ�ڴ���䶨�� **************
//
// 0x81000000 ~ 0x810FFFFF 1MB��scale������������scale���D1֡
// 0x81100000 ~ 0x812FFFFF	 ��һ֡���֡(MJPEGͼƬ + 4·D1 H264���� + 2·��Ƶ����)
// 0x81300000 ~ 0x814FFFFF  �ڶ�֡���֡(MJPEGͼƬ + 4·D1 H264���� + 2·��Ƶ����)
// 0x81500000 ~ 0x816FFFFF  FILE LOAD/SAVE BUFFER
// 0x81700000 ~ 0x81EFFFFF H264Ӳ������, ����8MB����H264Ӳ������

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

	
// H264�����ڴ涨��	
#define	XMSYS_H264_ENC_POOL_BASE					(XMSYS_CAMERA_BUFF_BASE + XMSYS_CAMERA_BUFF_SIZE)
#define	XMSYS_H264_ENC_POOL_SIZE					(0x800000)		// ����8MB����H264Ӳ������

	
	

// JPEG����������
#define	XMSYS_JPEG_FRAME_BASE					(XMSYS_D1_COMPOSE_BASE + XMSYS_DI_COMPOSE_SIZE)	
#define	XMSYS_JPEG_FRAME_SIZE					0x00200000
#define	XMSYS_JPEG_FRAME_BASE_1					XMSYS_JPEG_FRAME_BASE	
#define	XMSYS_JPEG_FRAME_BASE_2					(XMSYS_JPEG_FRAME_BASE_1 + XMSYS_JPEG_FRAME_SIZE)

// JPEGʹ�õ�VGA��������ַ	
#define	XMSYS_JPEG_SCALE_VGA_BASE				(XMSYS_JPEG_FRAME_BASE_2 + XMSYS_JPEG_FRAME_SIZE)
// JPEGʹ�õ�QVGA��������ַ		
#define	XMSYS_JPEG_SCALE_QVGA_BASE				(XMSYS_JPEG_FRAME_BASE_2 + XMSYS_JPEG_FRAME_SIZE)




// ϵͳ����ģ���ʼ��
void SYS_NetworkModuleInit (void);


// ϵͳ����ģʽ���� (PC CAMERAģʽ , VIDEO_RECORD��¼ģʽ)
#define	XMSYS_WORKMODE_VIDEOREC			0		// �г���¼ģʽ
#define	XMSYS_WORKMODE_PCCAMERA			1		// PCCameraģʽ

// ��ȡ��ǰϵͳ�Ĺ���ģʽ
int	XMSYS_GetWorkMode (void);
// ���õ�ǰϵͳ�Ĺ���ģʽ
void	XMSYS_SetWorkMode	(int WorkMode);

/////////////////////////////////////////////////////////////////////
//                                                                 //
// ******************* Videoģ�鶨�� ******************************//
//                                                                 //
/////////////////////////////////////////////////////////////////////

// VIDEO COMMAND PARAMETER����
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

// ǰ����ͼģʽVIDEO VIEW MODE����
#define	XMSYS_VIDEO_VIEW_MODE_FRONT				0		// ����ǰ��ͼ
#define	XMSYS_VIDEO_VIEW_MODE_REAR					1		// ���Ժ���ͼ
#define	XMSYS_VIDEO_VIEW_MODE_FRONT_REAR			2		// ǰ����
#define	XMSYS_VIDEO_VIEW_MODE_REAR_FRONT			3		// ǰ������

#define	XMSYS_VIDEO_VIEW_MODE_MAIN					0		// �ϳ�ģʽ
#define	XMSYS_VIDEO_VIEW_MODE_CH_0					1		// ͨ��0����
#define	XMSYS_VIDEO_VIEW_MODE_CH_1					2		// ͨ��1����
#define	XMSYS_VIDEO_VIEW_MODE_CH_2					3		// ͨ��0����
#define	XMSYS_VIDEO_VIEW_MODE_CH_3					4		// ͨ��1����


// VIDEO COMMAND����
#define	XMSYS_VIDEO_COMMAND_LCD_ONOFF				0		// ON/OFF����
#define	XMSYS_VIDEO_COMMAND_LCD_BACKLIGHT		1		// ��������
#define	XMSYS_VIDEO_COMMAND_OSD_ONOFF				2		// OSD��(UI)��ʾ����/�ر�
#define	XMSYS_VIDEO_COMMAND_VIEW_ONOFF			3		// ��Ƶ�㿪��/�ر�
#define	XMSYS_VIDEO_COMMAND_VIEW_MODE				4		// ǰ����ͼģʽ����
#define	XMSYS_VIDEO_COMMAND_TIMESTAMP_ONOFF		5		// ʱ�������/�ر�

// VIDEO_TASK���¼�����
#define	XMSYS_VIDEO_UI_COMMAND_EVENT				0x01		// VIDEO��������
#define	XMSYS_VIDEO_UI_VIEW_EVENT					0x02		// UI��ʾ�����¼�
#define	XMSYS_VIDEO_FRONT_VIEW_PACKET_EVENT		0x04		// ǰ��Sensor��ʾ����
#define	XMSYS_VIDEO_REAR_VIEW_PACKET_EVENT		0x08		// ����Sensor��ʾ����


// ��ȡǰ��Sensor YUV������
int *XMSYS_VideoGetFrontViewPacket (void);
// �ͷ�ǰ��Sensor YUV������
void XMSYS_VideoPutFrontViewPacket (void);
// ��ȡ����Sensor YUV������
int *XMSYS_VideoGetRearViewPacket (void);
// �ͷź���Sensor YUV������
void XMSYS_VideoPutRearViewPacket (void);

int *XMSYS_VideoGetViewPacket (int channel);
int *XMSYS_VideoLockViewPacket (int channel);
void XMSYS_VideoUnlockViewPacket (int channel);

// ֱ�ӻ�ȡһ����ƵFrameBuffer
unsigned char *XMSYS_VideoRequestVideoFrameBuffer (void);
// �ͷ�һ����ƵFrameBuffer
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

// D1����ɨ��ת��ΪD1����ɨ��
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
// ******************* Sensorģ�鶨�� *****************************//
//                                                                 //
/////////////////////////////////////////////////////////////////////

typedef struct tagXMSYSSENSORPACKET {
	void *				prev;			
	void *				next;
	int					id;
	
	unsigned int		width;
	unsigned int		height;
	unsigned int		size;			// �������ֽڴ�С
	unsigned int		locked;		// �����������ʹ����
	unsigned int		used;			// ����Ƿ���ʹ�á�����ͳ�ƶ�����֡
	unsigned int		scaled;		// ����Ƿ�������
		
	char *				buffer;		// ָ��YUV������
	
//	unsigned long		capture_start_ticket;		//  ��¼����ʱ��
	
} XMSYSSENSORPACKET;
	
#define	XMSYS_RAW_FRAME_SIZE_1280X960			(1280*960*3/2)
#define	XMSYS_RAW_FRAME_SIZE_720P				(1280*720*3/2)		// �˴�����Ϊ1280*720*2�����쳣��????
#define	XMSYS_RAW_FRAME_SIZE_VGA				(640*480*3/2)
#define	XMSYS_RAW_FRAME_SIZE_QVGA				(320*240*3/2)
#define	XMSYS_RAW_FRAME_SIZE_D1					(704*576*3/2)

// ǰ������ͷ֡��С(720P)
#define	XMSYS_FRONT_SENSOR_FRAME_SIZE			XMSYS_RAW_FRAME_SIZE_720P
// ��������ͷ֡��С(VGA)
#define	XMSYS_REAR_SENSOR_FRAME_SIZE			XMSYS_RAW_FRAME_SIZE_VGA

#define	XMSYS_D1_SENSOR_FRAME_SIZE				(704*576*3/2)

// SENSOR_TASK���¼�����
#define	XMSYS_SENSOR_CH_0_PACKET_READY_EVENT			0x01		// ǰ������֡�Ѳɼ���ϣ���CAPTURE�жϲ���
#define	XMSYS_SENSOR_CH_1_PACKET_READY_EVENT			0x02		// ��������֡�Ѳɼ���ϣ���CAPTURE�жϲ���
#define	XMSYS_SENSOR_CH_2_PACKET_READY_EVENT			0x04		// �������֡�Ѳɼ���ϣ���CAPTURE�жϲ���
#define	XMSYS_SENSOR_CH_3_PACKET_READY_EVENT			0x08		// �Ҳ�����֡�Ѳɼ���ϣ���CAPTURE�жϲ���

#define	XMSYS_SENSOR_CAPTURE_START							0x10		// ����Sensor�ɼ�������ͨ����
#define	XMSYS_SENSOR_CAPTURE_STOP							0x20		// ֹͣSensor�ɼ�������ͨ����

#define	XMSYS_UVC_PROGRAM_KEYID								0x80


// ��ȡһ��ǰ������֡
int *XMSYS_SensorFrontGetDataFrame (void);
// �ͷ�һ��ǰ������֡
void XMSYS_SensorFrontFreeDataFrame (int *DataFrame);
// ֪ͨһ���µ�����֡�Ѳ���
void XMSYS_SensorFrontNewDataFrame (int *DataFrame);
// ��ȡ��һ������֡����֡��¼
int * XMSYS_SensorFrontGetFreeFrame (void);
// ��Capture�жϳ������
// ֪ͨһ���µ�ǰ������֡�Ѳ���
void XMSYS_SensorFrontNotifyDataFrame (void);
// ��ȡһ����������֡
int *XMSYS_SensorRearGetDataFrame (void);
// �ͷ�һ����������֡
void XMSYS_SensorRearFreeDataFrame (int *DataFrame);
// ֪ͨһ���µ�����֡�Ѳ���
void XMSYS_SensorRearNewDataFrame (int *DataFrame);
// ��ȡ��һ������֡����֡��¼
int * XMSYS_SensorRearGetFreeFrame (void);
// ��Capture�жϳ������
// ֪ͨһ���µ�ǰ������֡�Ѳ���
void XMSYS_SensorRearNotifyDataFrame (void);

int XMSYS_SensorStartCapture (int channel, int *Frame);

// ��ȡǰ��֡��ַ
void *XMSYS_SensorGetHardwareAddress (int channel, unsigned int *PacketSize);

// H264�����̻߳�ȡһ��Sensor������
XMSYSSENSORPACKET * XMSYS_SensorGetAndLockSensorPacket (int channel);
void XMSYS_SensorReleaseSensorPacket (int channel, XMSYSSENSORPACKET *Packet);



// ��YUV��ʽ��ԭʼ720P֡�ü�Ϊ320*240��ʽ����ͼ֡
void XMSYS_Scale720PFrameToViewFrame (int *Raw720PFrame, int *ViewFrame);
// ��YUV��ʽ��ԭʼVGA֡�ü�Ϊ320*240��ʽ����ͼ֡
void XMSYS_ScaleVGAFrameToViewFrame (int *RawVGAFrame, int *ViewFrame);

// ��YUV��ʽ��ԭʼ720P֡�ü�Ϊ640*480��ʽ����ͼ֡
void XMSYS_Scale720PFrameToVGAFrame (int *Raw720PFrame, int *VGAFrame);

// ��YUV��ʽ��ԭʼD1֡(704*576)�ü�Ϊ(320*240)��ʽ����ͼ֡
void XMSYS_ScaleD1FrameToViewFrame (int *RawD1Frame, int *ViewFrame);
// ��YUV��ʽ��ԭʼD1֡(704*576)�ü�Ϊ(640*480)��ʽ����ͼ֡
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

// ����Sensor�ɼ�
void XMSYS_SensorCaptureStart (void);
// ֹͣSensor�ɼ�
void XMSYS_SensorCaptureStop (void);

// SensorӲ����ʼ��
void XMSYS_SensorHardwareInit (void);

// ����һ��1280*960��D1X4���֡
XMSYSSENSORPACKET * XMSYS_SensorCreate4D1ComposedSensorPacket (void);
// ɾ��һ��1280*960��D1X4���֡
void XMSYS_SensorDelete4D1ComposedSensorPacket (XMSYSSENSORPACKET *CompFrame);

XMSYSSENSORPACKET * XMSYS_SensorCreatePacket (int channel);
void XMSYS_SensorDeletePacket (int channel, XMSYSSENSORPACKET *packet);

void XMSYS_SensorStartCaptureAccount (void);
void XMSYS_SensorStopCaptureAccount (void);

// ����/��ȡD1����ͷ����ʽ(NTSC��PAL)
void XMSYS_SensorSetD1SensorType (unsigned char D1SensorType);
unsigned char XMSYS_SensorGetD1SensorType (void);



/////////////////////////////////////////////////////////////////////
//                                                                 //
// ******************* Cameraģ�鶨�� *****************************//
//                                                                 //
/////////////////////////////////////////////////////////////////////

#define	XMSYS_CAMERA_USB_EVENT							0x01	// USB�����¼�(��ö�ٵ�)
#define	XMSYS_CAMERA_UVC_START							0x02	// UVC����
#define	XMSYS_CAMERA_UVC_STOP								0x04	// UVC����
#define	XMSYS_CAMERA_UVC_TRANSFER						0x08	// ��������һ��֡
#define	XMSYS_CAMERA_PACKET_TRANSFER_DONE				0x10	// Camera���Ѵ�����ϣ���USB�жϲ���
#define	XMSYS_CAMERA_PACKET_READY						0x20	// Camera��Ready�¼�, ��ʾһ���µ�UVC��READY���ȴ�����

// �����붨��
#define	XMSYS_CAMERA_SUCCESS								0			// �ɹ�
#define	XMSYS_CAMERA_ERROR_USB_DISCONNECT				(-1)		// USB���ӶϿ�
#define	XMSYS_CAMERA_ERROR_USB_EXCEPTION				(-2)		// USB�����쳣
#define	XMSYS_CAMERA_ERROR_USB_UVCSTOP					(-3)		// USB UVC��ֹ
#define	XMSYS_CAMERA_ERROR_OTHER							(-4)		// �����쳣

#define	XMSYS_CAMERA_ERROR_MODE_ERROR					(-5)		// CDRģʽ��֧��
#define	XMSYS_CAMERA_ERROR_SAVE_FILE_OPEN				(-6)		// ����CDR�����ļ�û�д�
#define	XMSYS_CAMERA_ERROR_SAVE_FILE_WRITE				(-7)		// ����CDR�����ļ�д��ʧ��

typedef void (*XMSYS_CAMERA_CALLBACK) (void *handle, int err_code);			// Camera��������ص�����
typedef void (*XMSYS_CAMERA_RETRANSFER) (void *handle);


// USB Camera ����֡����
typedef struct tagXMCAMERAFRAME {
	void *			prev;
	void *			next;

	int				retransfer;		//
	int				ttl;					// �ط�����
	
	unsigned char *frame_base;			// ����USB֡��ַ
	unsigned int   frame_size;			// ����USB֡�ֽڴ�С
	
	void *         frame_private;		// ����USB֡��˽������
	XMSYS_CAMERA_CALLBACK fp_terminate;	// USB֡������ϻص�����

	XMSYS_CAMERA_RETRANSFER fp_retransfer;	// USB�ش��ص�����
	
} XMCAMERAFRAME;


#define	XMSYS_CAMERA_FRAME_SIZE		(1280*960*2)

// ��ȡ��һ������֡����֡��¼
int * XMSYS_CameraGetFreeFrame (void);
// �ͷſ���֡
void XMSYS_CameraPutFreeFrame (int *FreeFrame);


// ֪ͨһ���µ�����֡�Ѳ���
int XMSYS_CameraNewDataFrame (int *lpCameraFrame, int cbCameraFrame, void *lpPrivateData,
						XMSYS_CAMERA_CALLBACK fpCallback, XMSYS_CAMERA_RETRANSFER fpRetransfer);
// ��ȡһ��Camera����֡
XMCAMERAFRAME *XMSYS_CameraGetDataFrame (void);
// �ͷ�һ��Camera����֡
void XMSYS_CameraFreeDataFrame (XMCAMERAFRAME *lpDataFrame, int ErrCode);

void XMSYS_CameraInit (void);
void XMSYS_CameraExit (void);

void pccamera_init (void);
void pccamera_process (void);

// ��USB�жϳ������, ֪ͨCamera����Camera֡�Ѵ������
void XMSYS_CameraNotifyDataFrameTransferDone (void);
void XMSYS_CameraGetWindowSize (XMSIZE *lpSize);
void XMSYS_CameraPostEvent (unsigned char Event);

void XMSYS_ScaleD1FrameToD1Frame (int channel, unsigned char *srcFrame, unsigned char *dstFrame );

void XMSYS_CameraSetNtscPal (unsigned int NtscPal);

// UVCЭ������֡���ű�������
// 0 ������
// 1 ����ȱ�����С��1/2
// 2 ����ȱ�����С��1/4
void XMSYS_UVCCameraSetFrameRatio (unsigned int FrameRatio);

void XMSYS_H264CodecSetSharpFactor (signed char SharpFactor);
signed char XMSYS_H264CodecGetSharpFactor (void);

void XMSYS_H264CodecSetLumaFactor (signed char LumaFactor);
signed char XMSYS_H264CodecGetLumaFactor (void);

void XMSYS_H264CodecSetChromaFactor (signed char ChromaFactor);
signed char XMSYS_H264CodecGetChromaFactor (void);

// �����Ƶ����˵Ĳ���״̬(ÿ��λ��ʾͨ���Ĳ���״̬��1��ʾVideo����)
unsigned int XMSYS_CameraGetVideoStatus (void);

void XMSYS_SensorEnableDeinterlace (unsigned char Deinterlace);

/////////////////////////////////////////////////////////////////////
//                                                                 //
// ******************* H264Codecģ�鶨�� **************************//
//                                                                 //
/////////////////////////////////////////////////////////////////////
#define	XMSYS_H264_CODEC_EVENT_COMMAND						0x01		// �ⲿ�����¼�
#define	XMSYS_H264_CODEC_EVENT_RECORD_PROTECT				0x02		// ��¼����

// H264 codec�ⲿ�����
#define	XMSYS_H264_CODEC_COMMAND_RECORDER_START				1		// ¼���¼��������
#define	XMSYS_H264_CODEC_COMMAND_RECORDER_STOP					2		// ¼���¼ֹͣ����
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_START					3		// ������������
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_STOP					4		// ����ֹͣ����
#define	XMSUS_H264_CODEC_COMMAND_PLAYER_PAUSE					5
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_RESUME					6

#define	XMSYS_H264_CODEC_COMMAND_ID							0x34363248

#define	XMSYS_H264_CODEC_MAX_FILE_NAME							63
// H264 Codec�����������
typedef struct _H264CODE_COMMAND_PARAM {
	int		id;							//
	int		command;						// h264 codec����
	char		file_name[XMSYS_H264_CODEC_MAX_FILE_NAME + 1];				// �����ļ�ȫ·����
	char		file_name2[XMSYS_H264_CODEC_MAX_FILE_NAME + 1];				// �����ļ�ȫ·����
} H264CODEC_COMMAND_PARAM;

struct H264EncodeStream {
	int width[4];
	int height[4];
	
	char file_name[4][64];	// ���ļ�ȫ·����
	
	int file_save;			// 1 ��Ҫд�뵽�ļ�
								// 0 ����д�뵽�ļ�
	int file_lock;			// 1 ������ѱ���
								// 0 �������δ����
	
	int type;				// 0 file
								// 1 mem

	int		forced_640X576_mode;	// 1 ǿ��ԴΪ640X576						

	void *video_file;		// ��Ƶ�����ļ����, ����ʹ��
	void *audio_file;		// ��Ƶ�����ļ����, ����ʹ��
		
	int (*get_video_frame)(void *EncodeStream, void *video_frame);
	int (*get_audio_frame)(void *EncodeStream, void *audio_frame);
	
	unsigned char *		audio_fifo;
	unsigned char *		audio_data;
	
	unsigned char *		Y[4];
	unsigned char *		U[4];
	unsigned char *		V[4];
	
	int						curr_ticket;		// ��ǰʱ��
	int						start_ticket;
	int						stop_ticket;		// ����ʱ��
	int						frame_rate;			// fps
	int						frame_number;		// �ļ����Ѽ�¼����Ƶ֡����
	
	// Scale����
	int						do_scale[4];			// ����Ƿ���Ҫscale����
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
	
	char file_name[4][64];	// ���ļ�ȫ·����
		
	// Scale����
	int						do_scale[4];			// ����Ƿ���Ҫscale����
	int						scale_width[4];
	int						scale_height[4];
	int						scale_stride[4];
	unsigned char *		scale_y[4];
	unsigned char *		scale_u[4];
	unsigned char *		scale_v[4];
	
	int file_lock;			// 1 ������ѱ���
								// 0 �������δ����
	
	unsigned int			width;
	unsigned int			height;
	unsigned int			frame_count;		// ֡������
														//		0 ��ʾ�޷�ͳ��֡��������Ƶͷ��Ϣû��д��
	unsigned int			fps;					// ÿ�����֡�� (�̶�֡�ʱ���)									
	unsigned int			frame_index;		// ��ǰ����֡����
	
	unsigned int			playback_mode;		// �ط�ģʽ (���������������)													
	unsigned int			seek_time;			// ��λ����ʱ���(��Ϊ��λ)
	
};

unsigned long get_long (unsigned char *ch);
void set_long (unsigned char *ch, unsigned int v);

/////////////////////////////////////////////////////////////////////
//                                                                 //
// ******************* APP ģ�鶨��      **************************//
//                                                                 //
/////////////////////////////////////////////////////////////////////
#define	XMSYS_APP_KEYBD_EVENT				0x01		// �����¼�
#define	XMSYS_APP_TIMER_EVENT				0x02		// TIMER�¼�
void XMSYS_AppInit (void);
void XMSYS_AppExit (void);

/////////////////////////////////////////////////////////////////////
//                                                                 //
// *******************  HYBRID(DVR) ģ�鶨��  *********************//
//                                                                 //
/////////////////////////////////////////////////////////////////////

#define	HYBRID_FRAME_ID			0x44425948

#define	HYBRID_H264_ID				0x00E0FFFF

#define	XMSYS_HYBRID_FLAG_RETRANSFER			0x000000001		// ָʾ�ش�
#define	XMSYS_HYBRID_FLAG_FATALERROR			0x800000000		// ָʾVDRϵͳ����

void XMSYS_HybridFrameInit (void);
void XMSYS_HybridFrameExit (void);

unsigned int XMSYS_HybridGetH264DataOffset (void);


// ����һ���µĻ��֡
void * XMSYS_HybridFrameCreate (void);

// ���֡������׼����ϣ��ύ�ȴ�USB����
void XMSYS_HybridFrameSubmit (void *lpHybridFrame);

// ��һ��H264��Ƶ֡���뵽���֡��
int XMSYS_HybridFrameInsertVideoStream (void *lpHybridFrame, int VideoChannel,
													 unsigned long TimeStamp, void *lpVideoStream, int cbVideoStream, int i_frame);

// ��һ����Ƶ֡���뵽���֡��
int XMSYS_HybridFrameInsertAudioStream (void *lpHybridFrame, int AudioChannel,
													 unsigned long TimeStamp, void *lpAudioStream, int cbAudioStream);

// ��һ��keyid���뵽���֡��
int XMSYS_HybridFrameInsertKeyidStream (void *lpHybridFrame, void *lpKeyidStream, int cbKeyidStream);

// ��ȡDVR��¼ģʽ(ʵʱ���䡢���ر��桢���ش���)
int XMSYS_HybridGetDvrMode (void);
// // ����CDR����ģʽ (ʵʱ���䡢���ر��桢���ش���)
void XMSYS_HybridSetDvrMode (int mode);

// 1 �ϴ�KEYID��Ϣ��0 �ر��ϴ�KEYID��Ϣ
void XMSYS_HybridForcedKeyID (int force_keyid);

// ����֡�ʣ�1-29���֣���ʾÿ�����֡
void XMSYS_H264CodecSetFrameRate (unsigned int FrameRate);

// ���ݸ�����CDR֡���ݴ���һ���ļ��ϴ����֡(���ڱ��ر����CDR�����ϴ���ÿ�δ���һ������CDR֡)
void * XMSYS_HybridFrameCreateCdrLoadFrame (void *lpCdrFrame, int cbCdrFrame, int cdr_frame_count);

// ��һ��LocalFileInfo���뵽���֡��
int XMSYS_HybridFrameInsertLocalFileInfoStream (void *lpHybridFrame, void *lpLocalFileInfoStream, int cbLocalFileInfoStream);

// ��CDR�ϴ�֡��ͨ�����ݲ��뵽���֡
int XMSYS_HybridFrameInsertCdrLoadFrameChannelData (void *lpHybridFrame, 
																	 void *lpCdrFrameChannelData, int cbCdrFrameChannelData);


/////////////////////////////////////////////////////////////////////
//                                                                 //
// ******************* HARD TIMER ģ�鶨��   **********************//
//                                                                 //
/////////////////////////////////////////////////////////////////////

// ��ʼ��������ģ��Ӳ���Ķ�ʱ������ʱ���߳̾�������߳����ȼ���
void XMSYS_HardTimerInit (void);
void XMSYS_HardTimerExit (void);


/////////////////////////////////////////////////////////////////////
//                                                                 //
// ***************** MESSAGE ϵͳ��Ϣģ�鶨��  *********************//
//                                                                 //
/////////////////////////////////////////////////////////////////////
void XMSYS_MessageInit (void);
void XMSYS_MessageExit (void);

#ifdef __cplusplus
}
#endif

#endif	// _XMCORE_H_
