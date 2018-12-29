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
#define	XMSYS_IPRX_TASK_PRIORITY					199
#define	XMSYS_DHCPD_TASK_PRIORITY				100		// DHCPD����		
	
#define	XMSYS_VIDEO_TASK_PRIORITY				200		// 254
#define	XMSYS_ROTATE_TASK_PRIORITY				253
#define	XMSYS_SENSOR_TASK_PRIORITY				252
#define	XMSYS_CAMERA_TASK_PRIORITY				253		// ������ȼ���ȷ�����ʱ����Э��Ҫ���ʱ��Ҫ��
#define	XMSYS_GPS_TASK_PRIORITY					120
#define	XMSYS_ISP_TASK_PRIORITY					255		// ��Ҫ��������������ȼ�����Ϊ255, ��ISPΪ255


// ����H264 Codec����������ȼ������Դﵽÿ��������֡��	
#define	XMSYS_H264CODEC_TASK_PRIORITY			253		// H264�����߳����ȼ�
#define	XMSYS_H264File_TASK_PRIORITY				252		// H264д���߳����ȼ���Ӧ���ڱ����߳�
#define	XMSYS_ONE_KEY_PHOTOGRAPH_TASK_PRIORITY	254		// һ�������߳�, Ӧ����H264�����߳�, ���Ȼ�ȡsensor��
	
#define	XMSYS_APP_TASK_PRIORITY					200	//120 //	UI�߳�ӵ��������ȼ�
	
#define	XMSYS_JPEG_TASK_PRIORITY					120		//	JPEG�߳�
	
#define	XMSYS_HARD_TIMER_TASK_PRIORITY			254		//	Ӳ����ʱ���������񣨾�������������ȼ���
#define	XMSYS_MESSAGE_TASK_PRIORITY				254		//	ϵͳ��Ϣ�������񣬽�����Ӳ����ʱ������

#define	XMSYS_SYSTEM_UPDATE_TASK_PRIORITY		200

#define	XMSYS_USBHOST_TASK_PRIORITY				233		// USB HOST�ķ�������			
	
// ������������ȼ���Ҫ������Ƶ/APP�������ȼ�������CPU��Դ����ʱ����������û�л�ʱ��ɻ������񣬵��¶�֡����	
//#define	XMSYS_RECYCLE_TASK_PRIORITY				100
//#define	XMSYS_RECYCLE_TASK_PRIORITY				(XMSYS_VIDEO_TASK_PRIORITY+1)	

// 20180324 zhuoyonghong
// �޸Ļ����̵߳����ȼ�����Ϊ��Cache�߳�һ�£�����ϵͳæʱ����������߳��޷�����ļ�ϵͳ����Ȩ
#define	XMSYS_RECYCLE_TASK_PRIORITY				XMSYS_H264File_TASK_PRIORITY	
	
#define	XMSYS_UART_SOCKET_TASK_PRIORITY			253
#define	XMSYS_HONGJING_SOCKET_TASK_PRIORITY		253
	
#define	XMSYS_FILE_MANAGER_TASK_PRIORITY		120			// �ļ��������߳�
	
#define	XMSYS_AUTOTEST_TASK_PRIORITY				252			// �Զ���������
	
#define	XMSYS_USBHOST_TASK_PRIORITY				233		// USB HOST�ķ�������	
	
#define	XMSYS_ARK7116_TASK_PRIORITY				253		// ARK7116�����߳�
	
#define	XMSYS_NVP6134C_TASK_PRIORITY				253		// NVP6134C�����߳�

#define	XMSYS_NVP6124B_TASK_PRIORITY				120		// NVP6124B�����߳�

#define	XMSYS_PR2000_TASK_PRIORITY				253		// PR2000�����߳�

#define	XMSYS_PR1000_TASK_PRIORITY				253		// PR1000�����߳�
#define	XMSYS_RXCHIP_TASK_PRIORITY				253		// PR2000�����߳�
#define	XMSYS_ITU601SCALER_TASK_PRIORITY		120	    // ITU601 Scaler �߳�
#define	XMSYS_AES_TASK_PRIORITY					253		// aes�����߳�

#define	XMSYS_MAIN_TASK_STACK_SIZE			(0x2000)			// 8KB
#define	XMSYS_HARD_TIMER_STASK_SIZE			(0x1000)			// Ӳ����ʱ������ջ
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
#define	XMSYS_H264FILE_TASK_STACK_SIZE		(0x2000)			// 16KB	// ��Ҫ�����ļ�ϵͳ	
#define	XMSYS_RECYCLE_TASK_STACK_SIZE		(0x2000)			
#define	XMSYS_SYSTEM_UPDATE_TASK_STACK_SIZE	(0x2000)
#define	XMSYS_ISP_TASK_STACK_SIZE			(0x2000)

#define	XMSYS_JPEG_TASK_STACK_SIZE				(0x1000)			// 1KB
#define	XMSYS_USBHOST_TASK_STACK_SIZE			(4*1024)
		
#define	XMSYS_APP_TASK_STACK_SIZE					(0x4000)			// 16KB	UI�û��߳�
	
#define	XMSYS_FILE_MANAGER_TASK_STACK_SIZE			(0x1000)
		
#define	XMSYS_MESSAGE_TASK_STACK_SIZE				(0x2000)
#define	XMSYS_ONE_KEY_PHOTOGRAPH_TASK_STACK_SIZE	(0x2000)
	
#define	XMSYS_AUTOTEST_TASK_STACK_SIZE				(0x800)

#define	XMSYS_ARK7116_TASK_STACK_SIZE				(0x800)		// ARK7116�����̶߳�ջ

#define	XMSYS_NVP6134C_TASK_STACK_SIZE				(0x800)		// NVP6134C�����̶߳�ջ

#define	XMSYS_NVP6124B_TASK_STACK_SIZE				(0x800)		// NVP6124B�����̶߳�ջ

#define	XMSYS_PR2000_TASK_STACK_SIZE					(0x800)		// PR2000�����̶߳�ջ

#define	XMSYS_PR1000_TASK_STACK_SIZE					(0x800)		// PR1000�����̶߳�ջ
#define	XMSYS_RXCHIP_TASK_STACK_SIZE				(0x1000)		// D6752�����̶߳�ջ
#define	XMSYS_ITU601SCALER_TASK_STACK_SIZE			(0x1000)		
#define	XMSYS_AES_TASK_STACK_SIZE					(0x800)	// aes�����̶߳�ջ


// ����UVC������Դ	
#define	XMSYS_JPEG_MODE_ISP				0		// ISP		
#define	XMSYS_JPEG_MODE_ISP_SCALAR		1		// ISP Scalar
#define	XMSYS_JPEG_MODE_601				2
#define	XMSYS_JPEG_MODE_601_SCALAR		3
#define	XMSYS_JPEG_MODE_VIDEO			4		// ��Ƶ�ļ�����
#define	XMSYS_JPEG_MODE_IMAGE			5		// JPEGͼ��
#define	XMSYS_JPEG_MODE_DUMMY			6		// ���ڸ����ļ�����, ��̨����

#define	XMSYS_JPEG_EVENT_ENCODE							0x10		// ����JPEG����
#define	XMSYS_JPEG_EVENT_TICKET							0x20
#define	XMSYS_JPEG_EVENT_MODE_SETUP					0x40		// 	
	
// mode						UVC������Դ
// mode_private_data		��UVCԴ������ص�˽������
int XMSYS_JpegSetupMode (unsigned int mode, void *mode_private_data);
	
	
// ϵͳ����ģ���ʼ��
void SYS_NetworkModuleInit (void);


int XMSYS_VideoInit (void);
int XMSYS_VideoExit (void);
int XMSYS_VideoOpen (void);
int XMSYS_VideoClose (void);

void XMSYS_VideoTask (void);

int XMSYS_UsbVideoClose (void);
int XMSYS_UsbVideoOpen (void);

void XMSYS_AutoTestInit (void);		// �Զ����Գ�ʼ��

// ֡˽����������ID����
enum {
	XM_PACKET_USER_DATA_ID_EXP = 0,		// Exp
	XM_PACKET_USER_DATA_ID_ISP,			// ISP
	XM_PACKET_USER_DATA_ID_DATETIME,		// ����
	XM_PACKET_USER_DATA_ID_GPS,			// GPS
	XM_PACKET_USER_DATA_ID_COUNT
};

#define	XM_PACKET_USER_DATA_SIZE		2048		

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
	unsigned int		stride[3];
	char *				data[3];
	
	char *				buffer;		// ָ��YUV������
		
	unsigned int		frame_id;	// ISP֡ID
	int					channel;			// ͨ����
	
	volatile int		data_ref;
	volatile int		prep_ref;

	// ˽������(ISP���ü��ع�������汾)
	char *				isp_user_data;
	unsigned short		isp_user_size;
	
	// ������֡��ص�˽������
	unsigned int		isp_user_buff[XM_PACKET_USER_DATA_SIZE/4];
	
	
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
#define	XMSYS_SENSOR_CAPTURE_EXIT							0x40		// �˳�

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

// ֪ͨһ���µ�����֡�Ѳ���, ����֡ѹ�뵽YUV����֡����
void XMSYS_SensorNotifyDataFrame (int channel, int frame_id);

int XMSYS_SensorStartCapture (unsigned int channel, unsigned char *Frame);

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
// sensor�쳣ʱ, ��λ��Ӧ��ͨ��
int XMSYS_SensorCaptureReset (int channel);


// SensorӲ����ʼ��
void XMSYS_SensorHardwareInit (void);

// ����һ��1280*960��D1X4���֡
XMSYSSENSORPACKET * XMSYS_SensorCreate4D1ComposedSensorPacket (void);
// ɾ��һ��1280*960��D1X4���֡
void XMSYS_SensorDelete4D1ComposedSensorPacket (XMSYSSENSORPACKET *CompFrame);

XMSYSSENSORPACKET * XMSYS_SensorCreatePacket (int channel);

// ����Ƿ����sensor���ݰ���Ҫ����
// 1 ��ʾ����һ��ͨ���ϴ�������һ�����ݰ���Ҫ����
// 0 û�����ݰ���Ҫ����
int XMSYS_SensorCheckPacketReady (void);

void XMSYS_SensorDeletePacket (int channel, XMSYSSENSORPACKET *packet);
// һ���������½�sensor֡Ͷ�ݵ�Ԥ�������, �ṩ��H264������ʹ��
void XMSYS_SensorRecyclePacket (int channel, XMSYSSENSORPACKET *packet);
void XMSYS_SensorReturnPacket (int channel, XMSYSSENSORPACKET *packet);

void XMSYS_SensorStartCaptureAccount (void);
void XMSYS_SensorStopCaptureAccount (void);





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
#define	XMSYS_CAMERA_CDR									0x40	// ���յ�һ��CDR����
#define	XMSYS_CAMERA_UVC_BREAK							0x80	// UVC�쳣����, usb reset/usb suspend��

// �����붨��
#define	XMSYS_CAMERA_SUCCESS							0			// �ɹ�
#define	XMSYS_CAMERA_ERROR_USB_DISCONNECT			(-1)		// USB���ӶϿ�
#define	XMSYS_CAMERA_ERROR_USB_EXCEPTION			(-2)		// USB�����쳣
#define	XMSYS_CAMERA_ERROR_USB_UVCSTOP				(-3)		// USB UVC��ֹ
#define	XMSYS_CAMERA_ERROR_OTHER						(-4)		// �����쳣

#define	XMSYS_CAMERA_ERROR_MODE_ERROR				(-5)		// CDRģʽ��֧��
#define	XMSYS_CAMERA_ERROR_SAVE_FILE_OPEN			(-6)		// ����CDR�����ļ�û�д�
#define	XMSYS_CAMERA_ERROR_SAVE_FILE_WRITE			(-7)		// ����CDR�����ļ�д��ʧ��

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
// ���Camera�Ƿ���׼���� 1 ׼���� 0 δ׼��
int XMSYS_CameraIsReady (void);

void XMSYS_ScaleD1FrameToD1Frame (int channel, unsigned char *srcFrame, unsigned char *dstFrame );

void XMSYS_CameraSetNtscPal (unsigned int NtscPal);

void XMSYS_CameraNotifyCdrEvent (void);


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
#define	XMSYS_H264_CODEC_EVENT_SWITCH_CHANNEL				0x02		// �л�¼��ͨ��
#define	XMSYS_H264_CODEC_EVENT_ONE_KEY_PROTECT				0x04		// һ������, ��¼����

// H264 codec�ⲿ�����
#define	XMSYS_H264_CODEC_COMMAND_RECORDER_START					1		// ¼���¼��������
#define	XMSYS_H264_CODEC_COMMAND_RECORDER_STOP					2		// ¼���¼ֹͣ����
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_START					3		// ������������
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_STOP					4		// ����ֹͣ����
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_PAUSE					5
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_RESUME					6
#define	XMSYS_H264_CDOEC_COMMAND_PLAYER_FORWARD					7		// �ط�ǰ��
#define	XMSYS_H264_CDOEC_COMMAND_PLAYER_BACKWARD				8		// �طź���
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_SEEK					9		// ��λ��ĳ��ʱ��λ�ò�����
#define	XMSYS_H264_CODEC_COMMAND_PLAYER_FRAME					10		// ��֡��������

#define	XMSYS_H264_CODEC_COMMAND_ID							0x34363248

#define	XMSYS_H264_CODEC_MAX_FILE_NAME							63

// H264 Codec�����������
typedef struct _H264CODE_COMMAND_PARAM {
	int		id;							//
	int		command;						// h264 codec����
	char		file_name[XMSYS_H264_CODEC_MAX_FILE_NAME + 1];				// �����ļ�ȫ·����
	char		file_name2[XMSYS_H264_CODEC_MAX_FILE_NAME + 1];				// �����ļ�ȫ·����
	unsigned int	playback_mode;													// ��ʼ�ط�ģʽ
	unsigned int	seek_time;														// ��λ����ʱ���λ��(��λ ��)
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
	int						start_ticket;		// ��ʼʱ��
	int						stop_ticket;		// ����ʱ��
	// int						frame_rate;			// ÿ��֡�� fps
	// 20180609 ÿ��ͨ����ͬ֡������
	unsigned char			frame_rate[4];		// ÿ��֡�� fps
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

#define	XM_VIDEO_PLAYBACK_MODE_NORMAL	0		// ��������ģʽ (��������I֡��P֡����ʾ)
#define	XM_VIDEO_PLAYBACK_MODE_FORWARD	1		// �������ģʽ (������I֡����ͷ��β˳����ʾ)
#define	XM_VIDEO_PLAYBACK_MODE_BACKWARD	2		// ���˲���ģʽ (������I֡����β��ͷ˳����ʾ)
#define	XM_VIDEO_PLAYBACK_MODE_FRAME		3		// ��֡����ģʽ (ÿ�β��ŵ�֡, �ȴ�����������һ֡)

struct H264DecodeStream {
	int						curr_ticket;
	
	char file_name[4][64];	// ���ļ�ȫ·����
		
	// Scale����
	int						do_scale[4];			// ����Ƿ���Ҫscale����
	int						scale_width[4];
	int						scale_height[4];
	int						scale_stride[4];
	unsigned char *			scale_y[4];
	unsigned char *			scale_u[4];
	unsigned char *			scale_v[4];
	
	int file_lock;			// 1 ������ѱ���
							// 0 �������δ����
	
	unsigned int			width;
	unsigned int			height;
	unsigned int			frame_count;		// ֡������
												// 0 ��ʾ�޷�ͳ��֡��������Ƶͷ��Ϣû��д��
	unsigned int			fps;				// ÿ�����֡�� (�̶�֡�ʱ���)									
	unsigned int			frame_index;		// ��ǰ����֡����
	
	unsigned int			playback_mode;		// �ط�ģʽ (���������������)	
	
	unsigned int			seek_time;			// ��λ����ʱ���(��Ϊ��λ)
	
	unsigned int			command_player_stop;		
														// 1 ����ⲿCOMMAND_STOP������ֹ����
														//	0 ����ԭ�򣬽�����ֹ

	unsigned int			system_ticket;		// �Ӳ��ŵ㿪ʼ��ϵͳʱ��,-1��ʾ��Ҫ��λ��ʼ֡��ʱ��
	unsigned int			video_ticket;		// �Ӳ��ŵ㿪ʼ����ʼ֡��֡��ʾʱ��	
	
	unsigned int			user_data[4][2048/4];		// ��֡��ص�˽����Ϣ
	
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
// ******************* APP ģ�鶨��      **************************//
//                                                                 //
/////////////////////////////////////////////////////////////////////
#define	XMSYS_APP_KEYBD_EVENT				0x01		// �����¼�
#define	XMSYS_APP_TIMER_EVENT				0x02		// TIMER�¼�
void XMSYS_AppInit (void);
void XMSYS_AppExit (void);
void MotionTaskInit(void);

/////////////////////////////////////////////////////////////////////
//                                                                 //
// ******************* HARD TIMER ģ�鶨��   **********************//
//                                                                 //
/////////////////////////////////////////////////////////////////////

// ��ʼ��������ģ��Ӳ���Ķ�ʱ������ʱ���߳̾�������߳����ȼ���
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

// ��˽�����ݼ��뵽sensor˽�����ݻ�����(data!=NULL) ���� ������ռ����ں�����˽�����ݴ洢(user_data==NULL)
// ����ֵָ����˽�����ݵĻ�������ַ��ʧ�ܷ���NULL
//		id				֡˽����������
//		user_data	˽�����ݵ�ַ
//		user_size	˽�������ֽڴ�С
char * XMSYS_SensorInsertUserDataIntoPacket (XMSYSSENSORPACKET *packet, unsigned int id, char *user_data, int user_size);
void XMSYS_DecodeUserData (char *isp_data, int isp_size );


extern int h264_encode_avi_stream (char *filename, char *filename2, struct H264EncodeStream *H264Stream,
											  const char **argv, int argc);
extern int  h264_decode_avi_stream (const char *filename, const char *filename2, struct H264DecodeStream *stream);

void XMSYS_H264CodecInit (void);

void XMSYS_H264CodecExit (void);



/////////////////////////////////////////////////////////////////////
//                                                                 //
// ***************** MESSAGE ϵͳ��Ϣģ�鶨��  *********************//
//                                                                 //
/////////////////////////////////////////////////////////////////////
void XMSYS_MessageInit (void);
void XMSYS_MessageExit (void);


#if defined(CORE_HEAP_DEBUG)

// �ں��ڴ���亯��, ����������
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

// �ں��ڴ���亯��, ����������
void* kernel_malloc(unsigned int n);
void* kernel_mallocz (unsigned int n);
void  kernel_free  (void* pMemBlock);
void *kernel_realloc(void *ptr, unsigned int size);
void* kernel_calloc (unsigned int n, unsigned int s);

#endif

// JPEG�����ʼ��
void XMSYS_JpegInit (void);

// JPEG�������
void XMSYS_JpegExit (void);

// һ�����������ʼ��
void XMSYS_OneKeyPhotographInit (void);
// һ�������������
void XMSYS_OneKeyPhotographExit (void);

// ֹͣ��Ƶ�������
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecStop (void);

// һ������
// ��ʱδ����ʵ�֣��˴�����ʵ��
int XMSYS_H264CodecOneKeyProtect (void);

// ������Ƶ¼��
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecRecorderStart (void);

// ����һ�����չ���
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
// ��ʱδ����ʵ�֣��˴�����ʵ��
int XMSYS_JpegCodecOneKeyPhotograph (void);

// ֹͣ¼���¼
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecRecorderStop  (void);

// ���H264������Ƿ�æ
// 1 busy
// 0 idle
int XMSYS_H264CodecCheckBusy (void);
// ǿ��codec�˳�
void XMSYS_H264CodecForcedEncoderStop (void);

// ����ǰ¼����Ƶ����
void XMSYS_H264CodecLockCurrentVideo (void);

// ����ǰ¼����Ƶ����
void XMSYS_H264CodecUnLockCurrentVideo (void);

// ��ȡ��ǰ¼����Ƶ������״̬
// 1 ������
// 0 δ����
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



// G-Sensor��ʼ��
void XMSYS_GSensorInit (void);
void XMSYS_GSensorExit (void);

// ��鲢�����ײ�¼�
// ����ֵ
//		1		��ײ�ѷ���
//		0		��ײδ����
int XMSYS_GSensorCheckAndClearCollisionEvent (void);

// ����Ƿ���ͣ����ײ����
//	����ֵ
//		1		ͣ����ײ����
//		0		��ͣ����ײ����
int XMSYS_GSensorCheckParkingCollisionStartup (void);

int OS_Use_timeout (OS_RSEMA *sema, unsigned int ms_to_timeout);

// ARK7116����
void XMSYS_Ark7116Init (void);
void XMSYS_Ark7116Exit (void);

//NVP6134C����
void XMSYS_Nvp6134cInit (void);
void XMSYS_Nvp6134cExit (void);


//NVP6124B����
void XMSYS_Nvp6124bInit (void);
void XMSYS_Nvp6124bExit (void);


//PR2000����
void XMSYS_Pr2000Init (void);
void XMSYS_Pr2000Exit (void);

void XMSYS_Itu601ScalerInit (void);
void XMSYS_Itu601ScalerExit (void);

// ���ú�������ͷ��Ƶͼ��
void set_real_video_image (unsigned char *image, unsigned int w, unsigned int h);
// ��ȡ��������ͷ��Ƶͼ��
unsigned char *get_real_video_image (void);

// ����ǿ�ƷǼ�¼ģʽ
// forced_non_recording
//		1			ǿ�ƷǼ�¼ģʽ (��ģʽ�²���¼��, ���������ģʽ)
//		0			��ͨ��¼ģʽ 
void XMSYS_H264CodecSetForcedNonRecordingMode (int forced_non_recording);

// ��鵱ǰģʽ�Ƿ���ǿ�ƷǼ�¼ģʽ
// ����ֵ
// 	1			ǿ�ƷǼ�¼ģʽ (��ģʽ�²���¼��, ���������ģʽ)
//		0			��ͨ��¼ģʽ 
int XMSYS_H264CodecGetForcedNonRecordingMode (void);

void xm_video_display_init (void);
// ��ȡһ�����е���Ƶ����֡
XMSYSSENSORPACKET *xm_video_display_get_free_frame (int channel);
// ���һ����Ƶ����֡������׼���ã�����������ʾ
int xm_video_display_set_data_frame_ready (XMSYSSENSORPACKET *data_frame);
// ��ȡһ����Ч����Ƶ����֡
XMSYSSENSORPACKET *xm_video_display_get_data_frame (void);
// �����Ƶ���ݻ���֡����ʹ��
int xm_video_display_free_data_frame (XMSYSSENSORPACKET *data_frame);

// ����ؼ���, ����7116��ʼ������
void enter_region_to_protect_7116_setup (void);
void leave_region_to_protect_7116_setup (void);

XMSYSSENSORPACKET * XMSYS_SensorGetCurrentPacket (int channel);
void xm_itu601_scalar_start (void);
void xm_itu601_scalar_stop (void);

void XM_AppInitStartupTicket (void);

// ����Ƭ��IRAM������Դ����Ȩ
void enter_region_to_access_iram (void);
// �ͷ�Ƭ��IRAM������Դ����Ȩ
void leave_region_to_access_iram (void);


#ifdef __cplusplus
}
#endif

#endif	// _XMCORE_H_
