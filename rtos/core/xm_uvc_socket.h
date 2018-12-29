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
	XMSYS_UVC_SOCKET_TYPE_SYNC	=	0,		// ͬ���ܵ�
	XMSYS_UVC_SOCKET_TYPE_ASYC,				// �첽�ܵ�
	XMSYS_UVC_SOCKET_TYPE_COUNT
};

#define	XMSYS_UVC_NID_IMAGE_OUTPUT					0x00		// ͼ�����, ��Ϣ������Ϊ4���ֽڡ�
																			//		WIDTH(�����ȣ�2�ֽ�),HEIGHT(����߶ȣ�2�ֽ�)��
																			//		��Ƶ���ĵ�һ֡����JPEGͼ���ѽ��벢�������������������Ƶ�������ʾ��
#define	XMSYS_UVC_NID_PLAYBACK_FINISH				0x01		// �ط����
																			// 	��Ϣ������Ϊ�ա���ǰ¼����Ƶ�ļ��ط����
#define	XMSYS_UVC_NID_SDCARD_CAPACITY				0x02		//	SD������
	
#define	XMSYS_UVC_NID_SDCARD_FORMAT_FINISH		0x03		// "SD����ʽ�����"

#define	XMSYS_UVC_NID_FILE_DELETE_FINISH			0x04		// "�ļ�ɾ�����"

#define	XMSYS_UVC_NID_SDCARD_WITHDRAW				0x05		// SD���γ�	0x05	��Ϣ������Ϊ�ա�
#define	XMSYS_UVC_NID_SDCARD_INSERT				0x06		// SD������	0x06	��Ϣ������Ϊ�ա�
#define	XMSYS_UVC_NID_SDCARD_FULL					0x07		// SD��д��	0x07	"��Ϣ������Ϊ�ա�
																			//		һ����������Ƶ�ļ����࣬�����޷���¼�µ���Ƶ"
#define	XMSYS_UVC_NID_SDCARD_DAMAGE				0x08		//	SD����	0x08	"��Ϣ������Ϊ�ա�
																			//		SD���ļ���дУ��ʧ�ܣ�SD���������𻵣�SD����Ҫ��ʽ��"

#define	XMSYS_UVC_NID_FSERROR						0x09		// "SD���ļ�ϵͳ�Ƿ�"	0x09	
																			//		"��Ϣ������Ϊ�ա��޷�����SD�����ļ�ϵͳ��SD����Ҫ��ʽ��"

#define	XMSYS_UVC_NID_SDCARD_INVALID				0x0A		// SD���޷�ʶ�𣨿��Ӵ���������Ч��

#define	XMSYS_UVC_NID_FILE_LIST_UPDATE			0x0B		// �ļ��б����
																				// ��Ϣ������12���ֽ�
																				//		�ļ���(XXXXXXXX,8�ֽ�), CH(ͨ����1�ֽ�), TYPE(���ͣ�1�ֽ�), CODE(�����룬2�ֽڣ�
																				//		CH --> 0x00 ǰ������ͷ
																				//		CH --> 0x01 ��������ͷ
																				//		TYPE --> 0x00 ��Ƶ
																				//		TYPE --> 0x01 ��Ƭ
																				//		CODE --> 0 (ɾ��)
																				//		CODE --> 1 (����)
#define	XMSYS_UVC_NID_SYSTEM_UPDATE_PACKAGE		0x0C		// ϵͳ������֪ͨ
#define	XMSYS_UVC_NID_SYSTEM_UPDATE_STEP			0x0D		// ��������״̬
																				// ��Ϣ������1���ֽ�
																				// STEP(1�ֽ�)
																				// STEP(1 ~ 100) ��100��ʾ������ɡ�
																				// 0xFF ��ʾ����ʧ��
																				// ������ɺ󸨻��Զ�������
																				//	���������·��͡�ͨ�Ž���������������ͨ�š�
#define	XMSYS_UVC_NID_TOO_MORE_LOCKED_RESOURCE	0x0E		// ̫����������Ƶ��
#define	XMSYS_UVC_NID_PLAYBACK_PROGRESS			0x0F		// �طŽ���ָʾ
																			//		��ǰ��Ƶ����λ��(2�ֽڣ��뵥λ)
#define	XMSYS_UVC_NID_FILELIST						0x10		// �ļ�Ŀ¼�б�
#define	XMSYS_UVC_NID_DOWNLOAD						0x11		// ��������
#define	XMSYS_UVC_NID_LOW_PREFORMANCE				0x12		// SD���ļ�ϵͳд���ٶȽ���
																			//		SD���ļ�ϵͳ��д���ٶ�(FAT���ֽڴ�С̫С)�����޷�����ʵʱ¼��, �����û���ʽ��SD��.
#define	XMSYS_UVC_NID_HYBRID_STREAM				0x13		//	hubrid�����

#define	XMSYS_UVC_CID_SOCKET_CONNECT				0x50		//	ͨ�Ž���	0xF0	"���������Ϊ�ա�
																		//	�����������򸨻����ͣ���ʼ����ͨ�����ӡ�
																		//	ֻ�н������Ӻ����������򸨻���������������߸�������������֪ͨ��Ϣ��
																		//	ͨ�Ž������������������Ե�CID��NID��λ��0��"	
																		//	Ӧ������ݸ�ʽ ������Ӧ��
																		//		"1�ֽڳ��ȣ�
																		//		0x00 --> ACK
																		//		0x01 --> NAK"

#define	XMSYS_UVC_CID_SOCKET_DISCONNECT			0x51		// ͨ�ŶϿ�

#define	XMSYS_UVC_CID_SYSTEM_UPDATE				0x52		//	ϵͳ����
#define	XMSYS_UVC_CID_GET_SYSTEM_VERSION			0x53		// ��ȡϵͳ�汾��
#define	XMSYS_UVC_CID_SHUTDOWN						0x54		// �ػ���λ����
#define	XM_SHUTDOWN_POWERDOWN				0			// �ػ�
#define	XM_SHUTDOWN_REBOOT					1			// ��λ����
#define	XMSYS_UVC_CID_AUTOTEST_KEY					0x55		// �Զ����԰���

#define	XMSYS_UVC_CID_HYBRID_START					0x60		// HYBRID������
#define	XMSYS_UVC_CID_HYBRID_STOP					0x61		// HYBRID��ֹͣ
#define	XMSYS_UVC_CID_HYBRID_FORCE_IFRAME		0x62		// ǿ��I֡
																			// CH (1�ֽڳ���)
																			//			CH --> 0 ǿ��ǰ������ͷI֡����
																			//			CH --> 1 ǿ�ƺ�������ͷI֡����
																			//			����ֵ����

// ����
#define	XMSYS_UVC_CID_DATETIME_SET					0x00		// ʱ������
#define	XMSYS_UVC_CID_VIDEO_SEGMENT_LENGTH_SET	0x01		// ¼��ֶ�ʱ�䳤������
#define	XMSYS_UVC_CID_MIC_SET						0x02		// ¼���������ر�
#define	XMSYS_UVC_CID_MIC_GET						0x03		// ��ȡ¼������״̬
#define	XMSYS_UVC_CID_VIDEO_OUTPUT_SET_ONOFF	0x04		// ��Ƶ����������ر�
#define	XMSYS_UVC_CID_AUDIO_MUTE					0x05		// ������������
#define	XMSYS_UVC_CID_AUDIO_VOLUME					0x06		// ��������
#define	XMSYS_UVC_CID_OUTPUT_IMAGE_SIZE			0x07		// ���ͼ������ WIDTH(2�ֽ�),HEIGHT(2�ֽ�)
#define	XMSYS_UVC_CID_VIDEO_FORMAT					0x08		// ¼����Ƶ��ʽ���� (1080p30fps, 720p30fps, 720p60fps)
#define	XMSYS_UVC_CID_SENSOR_PROBE					0x09		// �������ͷ�Ƿ����

// ¼��/���տ���
#define	XMSYS_UVC_CID_RECORD_START						0x10		// ¼������
#define	XMSYS_UVC_CID_RECORD_STOP						0x11		// ¼��ֹͣ
#define	XMSYS_UVC_CID_RECORD_CHANNEL_SWITCH			0x12		// ¼��ͨ���л�
#define	XMSYS_UVC_CID_RECORD_ONE_KEY_PROTECT		0x13		// һ������
#define	XMSYS_UVC_CID_RECORD_ONE_KEY_PHOTOGRAPH	0x14		// һ������
#define	XMSYS_UVC_CID_SENSOR_START						0x15		// ��������ͷ
#define	XMSYS_UVC_CID_SET_FORCED_NON_RECORDING_MODE	0x16		// ʹ��/��ֹǿ�Ʒ�¼��ģʽ
#define	XMSYS_UVC_CID_GET_FORCED_NON_RECORDING_MODE	0x17		// ��ȡǿ�Ʒ�¼��ģʽ
#define	XMSYS_UVC_CID_LOCK_CURRENT_VIDEO					0x18		// ����ǰ¼����Ƶ����
#define	XMSYS_UVC_CID_UNLOCK_CURRENT_VIDEO				0x19		// ����ǰ¼����Ƶ����
#define	XMSYS_UVC_CID_GET_VIDEO_LOCK_STATE				0x1A		// ��ȡ��ǰ¼����Ƶ������״̬


// ��Ƶ/ͼƬ�����¼��ط�
#define	XMSYS_UVC_CID_GET_MEDIA_FILE_COUNT			0x20		// ��ȡ¼��/��Ƭ�ļ�����
#define	XMSYS_UVC_CID_GET_MEDIA_FILE_LIST			0x21		// ��ȡ¼��/��Ƭ�ļ��б�
#	define		XM_UVC_TYPE_VIDEO		0						//			TYPE --> 0x00 ��Ƶ
#	define		XM_UVC_TYPE_PHOTO		1						//			TYPE --> 0x01 ��Ƭ
#	define		XM_UVC_TYPE_ALL		7						//			TYPE --> 0x07 ��Ƭ����Ƶ	

#define	XMSYS_UVC_CID_PLAYBACK_START					0x22		// �ط�����
#define	XMSYS_UVC_CID_PLAYBACK_PAUSE					0x23		// �ط���ͣ
#define	XMSYS_UVC_CID_PLAYBACK_STOP					0x24		// �ط�ֹͣ
#define	XMSYS_UVC_CID_PLAYBACK_FORWARD				0x25		// �طſ��
#define	XMSYS_UVC_CID_PLAYBACK_BACKWARD				0x26		// �طſ���
#define	XMSYS_UVC_CID_PLAYBACK_SEEK					0x27		// �طŶ�λ

// �����ļ�����
#define	XMSYS_UVC_CID_SDCARD_GET_STATE				0x30		// SD��״̬
#define	XMSYS_UVC_CID_SDCARD_GET_CAPACITY			0x31		// SD������
#define	XMSYS_UVC_CID_SDCARD_FORMAT					0x32		// SD����ʽ��
#define	XMSYS_UVC_CID_MEDIA_FILE_CHECK				0x33		// �ļ����
#define	XMSYS_UVC_CID_MEDIA_FILE_DELETE				0x34		// �ļ�ɾ��
#define	XMSYS_UVC_CID_VIDEO_LOCK						0x35		// ��Ƶ�ļ�����/����
#define	XMSYS_UVC_CID_SDCARD_CLEAR_STATE				0x36		// �������д�������� (��дУ��ʧ��)״̬������״̬�޷������
																				//		������ѡ���������״̬��д�������𻵣�ʱ��Ӧ�������������״̬���֡�

// ��Ƶ/ͼƬ����֧��
#define	XMSYS_UVC_CID_DOWNLOAD_OPEN					0x40		// ���ؿ���, ����Ƶ�ļ����ص�����, ����fopen
#define	XMSYS_UVC_CID_DOWNLOAD_CLOSE					0x41		// ���عر�, �رյ�ǰ���ڽ��е���Ƶ����. ����fclose
#define	XMSYS_UVC_CID_DOWNLOAD_SEEK					0x42		// ���ض�λ, ��λ���ص�λ��, ����fseek
#define	XMSYS_UVC_CID_DOWNLOAD_READ					0x43		// ���ض�ȡ, ��ȡ��������, ����fread

// 0 	�ɹ�
// -1 ʧ��
int  XMSYS_UvcSocketTransferResponsePacket (unsigned int socket_type,  
												  				unsigned char message_id,	// ����ID
												  				unsigned char *resp_packet_buffer,
												  				unsigned int resp_packet_length
															);

// 0 	�ɹ�
// -1 ʧ��
int  XMSYS_UvcSocketReceiveCommandPacket (
															 unsigned char message_id,		// ����ID
															 unsigned char *command_packet_buffer,
															 unsigned int command_packet_length
															);

void  XMSYS_UvcSocketInit (void);

void  XMSYS_UvcSocketExit (void);

// �ļ��б����
void	XMSYS_UvcSocketFileListUpdate (unsigned char channel,
													  unsigned char type,
													  unsigned char file_name[8],
													  unsigned int create_time,		// �ļ�����ʱ��
													  unsigned int record_time,		// ��Ƶ¼��ʱ��(��)
													  unsigned short code				// CODE --> 0 (ɾ��, ѭ��¼���ļ�ɾ��)
																								//	CODE --> 1 (����, ����¼���ļ����»�����Ƭ�ļ�����)
													);

// ͼ�����
// ��Ϣ������Ϊ�ա�
// ��Ƶ���ĵ�һ֡����JPEGͼ���ѽ��벢�������������������Ƶ�������ʾ��
int XMSYS_UvcSocketImageOutputReady (unsigned short width, unsigned short height);

// �ط���� (��ǰ¼����Ƶ�ļ��ط����)
void XMSYS_UvcSocketPlaybackFinish (unsigned char type);

// �طŽ���ָʾ
// ��Ϣ������2���ֽ� ��ǰ��Ƶ����λ��(2�ֽڣ��뵥λ)
void XMSYS_UvcSocketPlaybackProgress (unsigned int playback_progress);

// ��UI�㱨��SD����״̬
// sdcard_SystemEvent	��SD����ص�ϵͳ�¼�
void XMSYS_UvcSocketReportSDCardSystemEvent (unsigned int sdcard_SystemEvent);

// ʹ�ܻ��ֹsocket����
void XMSYS_UvcSocketSetEnable (int enable);

void XMSYS_UvcSocketSetCardLogicalState (unsigned int sdcard_SystemEvent);

// ���socketͨ���Ƿ��ѽ���
int  XMSYS_UvcSocketCheckConnection (void);

void XMSYS_CdrPrivateProtocolProcess (void);

int XMSYS_CdrSendResponse (unsigned int packet_type, unsigned char command_id, 
									unsigned char *response_data_buffer, unsigned int response_data_length);


// ��ȡ����Ӧ��֡������չӦ��֡
// ÿ��Ӧ��֡��2���ֽڹ���
// ����Ӧ�����һ��Ӧ��֡����
// ��չӦ���������2��Ӧ��֡����
unsigned short XMSYS_CdrGetResponseFrame (void);

// ��ȡ��״̬��
// һ�ֽڳ��ȣ�
// 0x01 --> SD���γ�
// 0x02 --> SD������
// 0x03 --> SD���ļ�ϵͳ����
// 0x04 --> SD���޷�ʶ��
int XMSYS_UvcSocketGetCardState (void);

#define	XM_UVC_ID			"XSPACE_USB_VIDEO"
#define	XM_UVC_VERSION		0x00010000		// 1.0�汾
#define	XM_UVC_TYPE_JPEG	0x00000000
#define	XM_UVC_TYPE_H264	0x00000001

#define	XM_UVC_SECTOR_NUMBER_INFO			(1024*1024*1024/512)						// �豸��Ϣ(����)/USB��Ƶ������ֹͣ(д��)
#define	XM_UVC_SECTOR_NUMBER_COMMAND		(XM_UVC_SECTOR_NUMBER_INFO + 1)		// UVC˽��Э������(д��)/Ӧ��(����)			
#define	XM_UVC_SECTOR_NUMBER_STREAM		(XM_UVC_SECTOR_NUMBER_COMMAND + 1)	// ��Ƶ֡����Ϣ(ֻ��)
#define	XM_UVC_SECTOR_NUMBER_DATA			(XM_UVC_SECTOR_NUMBER_STREAM + 1)	// ��Ƶ֡���ݶ���(ֻ��),16MB
#define	XM_UVC_SECTOR_NUMBER_VOICE			(XM_UVC_SECTOR_NUMBER_DATA + 0x1000000/512)

typedef struct XM_UVC_INFO {
	unsigned char id[16];		// "XSPACE_USB_VIDEO"
	unsigned int version;		// �汾
	unsigned int stream_type;	// JPEG/H264��ʽ��Ϣ
	unsigned int width;			// UVC��Ƶ�ߴ�, 16�ı���
	unsigned int height;
	unsigned int start;			// ����/ֹͣUVC����
} XM_UVC_INFO;

#define	XM_UVC_STREAM_QUALITY_0			0			// �������
#define	XM_UVC_STREAM_QUALITY_9			9			// �������
// ��ȡ��ǰ��UVC������������
unsigned int xm_uvc_get_video_quality_level (void);
// ����UVC��Ƶ������������
int xm_uvc_set_video_quality_level (unsigned int quality_level);


typedef struct XM_UVC_STREAM {
	unsigned int stream_type;		// JPEG/H264��ʽ��Ϣ
	unsigned int width;				// UVC��Ƶ�ߴ�
	unsigned int height;
	unsigned int stream_size;		// UVC��Ƶ���ֽڳ���
	unsigned int quality;			// UVC��Ƶ����������
} XM_UVC_STREAM;



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XMSYS_UVC_SOCKET_H_