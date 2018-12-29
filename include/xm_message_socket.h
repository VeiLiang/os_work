#ifndef _XMSYS_MESSAGE_SOCKET_H_
#define _XMSYS_MESSAGE_SOCKET_H_

#define	XMSYS_MESSAGE_SOCKET_TYPE_ID_SYNC		0xFF00
#define	XMSYS_MESSAGE_SOCKET_TYPE_ID_ASYC		0xAA55

enum {
	XMSYS_MESSAGE_SOCKET_TYPE_SYNC	=	0,		// ͬ���ܵ�
	XMSYS_MESSAGE_SOCKET_TYPE_ASYC,				// �첽�ܵ�
	XMSYS_MESSAGE_SOCKET_TYPE_COUNT
};

#define	XMSYS_NID_IMAGE_OUTPUT					0x00
#define	XMSYS_NID_PLAYBACK_FINISH				0x01
#define	XMSYS_NID_SDCARD_CAPACITY				0x02
#define	XMSYS_NID_SDCARD_FORMAT_FINISH		0x03
#define	XMSYS_NID_FILE_DELETE_FINISH			0x04
#define	XMSYS_NID_SDCARD_WITHDRAW				0x05		// SD���γ�	0x05	��Ϣ������Ϊ�ա�
#define	XMSYS_NID_SDCARD_INSERT					0x06		// SD������	0x06	��Ϣ������Ϊ�ա�
#define	XMSYS_NID_SDCARD_FULL					0x07		// SD��д��	0x07	"��Ϣ������Ϊ�ա�
																		//		һ����������Ƶ�ļ����࣬�����޷���¼�µ���Ƶ"
#define	XMSYS_NID_SDCARD_DAMAGE					0x08		//	SD����	0x08	"��Ϣ������Ϊ�ա�
																		//		SD���ļ���дУ��ʧ�ܣ�SD���������𻵣�SD����Ҫ��ʽ��"

#define	XMSYS_NID_FSERROR							0x09		// "SD���ļ�ϵͳ�Ƿ�"	0x09	
																		//		"��Ϣ������Ϊ�ա��޷�����SD�����ļ�ϵͳ��SD����Ҫ��ʽ��"

#define	XMSYS_NID_FILE_LIST_UPDATE				0x0A		// �ļ��б����
																		// ��Ϣ������12���ֽ�
																		//		�ļ���(XXXXXXXX,8�ֽ�), CH(ͨ����1�ֽ�), TYPE(���ͣ�1�ֽ�), CODE(�����룬2�ֽڣ�
																		//		CH --> 0x00 ǰ������ͷ
																		//		CH --> 0x01 ��������ͷ
																		//		TYPE --> 0x00 ��Ƶ
																		//		TYPE --> 0x01 ��Ƭ
																		//		CODE --> 0 (ɾ��)
																		//		CODE --> 1 (����)
#define	XMSYS_NID_SYSTEM_UPDATE_PACKAGE		0x0B		// ϵͳ������֪ͨ
#define	XMSYS_NID_SYSTEM_UPDATE_STEP			0x0C		// ��������״̬
																		// ��Ϣ������1���ֽ�
																		// STEP(1�ֽ�)
																		// STEP(1 ~ 100) ��100��ʾ������ɡ�
																		// 0xFF ��ʾ����ʧ��
																		// ������ɺ󸨻��Զ�������
																		//	���������·��͡�ͨ�Ž���������������ͨ�š�
#define	XMSYS_NID_TOO_MORE_LOCKED_RESOURCE	0x0D		// ̫����������Ƶ��
#define	XMSYS_NID_SYSTEM_DAMAGE					0x0E		// ����ϵͳ��
#define	XMSYS_NID_PLAYBACK_PROGRESS			0x0F		// �طŽ���ָʾ
																		//		��ǰ��Ƶ����λ��(2�ֽڣ��뵥λ)
#define	XMSYS_NID_SDCARD_INVALID				0x10		// SD���޷�ʶ�𣨿��Ӵ���������Ч��
#define	XMSYS_NID_SYSTEM_STARTUP				0x11		// ϵͳ������Ϣ
#define	XMSYS_NID_SDCARD_FSUNSUPPORT			0x12		// SD���ļ�ϵͳΪexFAT����NTFS

#define	XMSYS_CID_SOCKET_CONNECT				0xF0		//	ͨ�Ž���	0xF0	"���������Ϊ�ա�
																		//	�����������򸨻����ͣ���ʼ����ͨ�����ӡ�
																		//	ֻ�н������Ӻ����������򸨻���������������߸�������������֪ͨ��Ϣ��
																		//	ͨ�Ž������������������Ե�CID��NID��λ��0��"	
																		//	Ӧ������ݸ�ʽ ������Ӧ��
																		//		"1�ֽڳ��ȣ�
																		//		0x00 --> ACK
																		//		0x01 --> NAK"

#define	XMSYS_CID_SOCKET_DISCONNECT			0xF1		// ͨ�ŶϿ�

#define	XMSYS_CID_SYSTEM_UPDATE					0xF2		//	ϵͳ����
#define	XMSYS_CID_GET_SYSTEM_VERSION			0xF3		// ��ȡϵͳ�汾��
#define	XMSYS_CID_SHUTDOWN						0xF4		// �ػ���λ����
#	define	XM_SHUTDOWN_POWERDOWN				0			// �ػ�
#	define	XM_SHUTDOWN_REBOOT					1			// ��λ����

#define	XMSYS_CID_DATETIME_SET					0x00		// ʱ������
#define	XMSYS_CID_VIDEO_SEGMENT_LENGTH_SET	0x01		// ¼��ֶ�ʱ�䳤������
#define	XMSYS_CID_MIC_SET							0x02		// ¼���������ر�
#define	XMSYS_CID_MIC_GET							0x03		// ��ȡ¼������״̬
#define	XMSYS_CID_VIDEO_OUTPUT_SET_ONOFF		0x04		// ��Ƶ����������ر�
#define	XMSYS_CID_AUDIO_MUTE						0x05		// ������������
#define	XMSYS_CID_AUDIO_VOLUME					0x06		// ��������
#define	XMSYS_CID_CVBS_TYPE_SET					0x07		// CVBS��ʽ����

#define	XMSYS_CID_RECORD_START					0x10		// ¼������
#define	XMSYS_CID_RECORD_STOP					0x11		// ¼��ֹͣ
#define	XMSYS_CID_RECORD_CHANNEL_SWITCH		0x12		// ¼��ͨ���л�
#define	XMSYS_CID_RECORD_ONE_KEY_PROTECT		0x13		// һ������
#define	XMSYS_CID_RECORD_ONE_KEY_PHOTOGRAPH	0x14		// һ������
#define	XMSYS_CID_SENSOR_START					0x15		// ��������ͷ

#define	XMSYS_CID_GET_MEDIA_FILE_COUNT		0x20		// ��ȡ¼��/��Ƭ�ļ�����
#define	XMSYS_CID_GET_MEDIA_FILE_LIST			0x21		// ��ȡ¼��/��Ƭ�ļ��б�
#define	XMSYS_CID_PLAYBACK_START				0x22		// �ط�����
#define	XMSYS_CID_PLAYBACK_PAUSE				0x23		// �ط���ͣ
#define	XMSYS_CID_PLAYBACK_STOP					0x24		// �ط�ֹͣ
#define	XMSYS_CID_PLAYBACK_FORWARD				0x25		// �طſ��
#define	XMSYS_CID_PLAYBACK_BACKWARD			0x26		// �طſ���
#define	XMSYS_CID_PLAYBACK_SEEK					0x27		// �طŶ�λ


#define	XMSYS_CID_SDCARD_GET_STATE				0x30		// SD��״̬
#define	XMSYS_CID_SDCARD_GET_CAPACITY			0x31		// SD������
#define	XMSYS_CID_SDCARD_FORMAT					0x32		// SD����ʽ��
#define	XMSYS_CID_MEDIA_FILE_CHECK				0x33		// �ļ����
#define	XMSYS_CID_MEDIA_FILE_DELETE			0x34		// �ļ�ɾ��
#define	XMSYS_CID_VIDEO_LOCK						0x35		// ��Ƶ�ļ�����/����
#define	XMSYS_CID_SDCARD_CLEAR_STATE			0x36		// �������д�������� (��дУ��ʧ��)״̬������״̬�޷������
																		//		������ѡ���������״̬��д�������𻵣�ʱ��Ӧ�������������״̬���֡�

// 0 	�ɹ�
// -1 ʧ��
int  XMSYS_MessageSocketTransferResponsePacket (unsigned int socket_type,  
																unsigned int message_no,	// �����	
												  				unsigned int message_id,	// ����ID
												  				unsigned char *resp_packet_buffer,
												  				unsigned int resp_packet_length
															);

// 0 	�ɹ�
// -1 ʧ��
int  XMSYS_MessageSocketReceiveCommandPacket (unsigned int message_no,		// �����	
															 unsigned int message_id,		// ����ID
															 unsigned char *command_packet_buffer,
															 unsigned int command_packet_length
															);

void  XMSYS_MessageSocketInit (void);

void  XMSYS_MessageSocketExit (void);

// �ļ��б����
void	XMSYS_MessageSocketFileListUpdate (unsigned char channel,
													  unsigned char type,
													  unsigned char file_name[8],
													  unsigned int create_time,		// �ļ�����ʱ��
													  unsigned int record_time,		// ��Ƶ¼��ʱ��(��)
													  unsigned short code);

// ͼ�����
// ��Ϣ������Ϊ�ա�
// ��Ƶ���ĵ�һ֡����JPEGͼ���ѽ��벢�������������������Ƶ�������ʾ��
int XMSYS_MessageSocketImageOutputReady (unsigned short width, unsigned short height);

// �ط���� (��ǰ¼����Ƶ�ļ��ط����)
void XMSYS_MessageSocketPlaybackFinish (unsigned char type);

// �طŽ���ָʾ
// ��Ϣ������2���ֽ� ��ǰ��Ƶ����λ��(2�ֽڣ��뵥λ)
void XMSYS_MessageSocketPlaybackProgress (unsigned int playback_progress);

// ��UI�㱨��SD����״̬
// sdcard_SystemEvent	��SD����ص�ϵͳ�¼�
void XMSYS_MessageSocketReportSDCardSystemEvent (unsigned int sdcard_SystemEvent);

// ʹ�ܻ��ֹsocket����
void XMSYS_MessageSocketSetEnable (int enable);

void XMSYS_SetCardLogicalState (unsigned int sdcard_SystemEvent);

int XMSYS_SendSystemStartupMessage (void);

// ���socketͨ���Ƿ��ѽ���
int  XMSYS_MessageSocketCheckConnection (void);

#endif	// _XMSYS_MESSAGE_SOCKET_H_