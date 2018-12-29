#ifndef _XM_H264_CODEC_
#define _XM_H264_CODEC_

#if defined (__cplusplus)
	extern "C"{
#endif
	
// AVI����ʹ�õ���Ƶ��ʽ
// video format		
enum {
	ARKN141_VIDEO_FORMAT_1080P_30 = 0,
	ARKN141_VIDEO_FORMAT_720P_30,
#if HONGJING_CVBS
	ARKN141_VIDEO_FORMAT_720X480P,
	ARKN141_VIDEO_FORMAT_640X480P,
	ARKN141_VIDEO_FORMAT_320X240P,
#else	
	ARKN141_VIDEO_FORMAT_720P_60,
#endif
	ARKN141_VIDEO_FORMAT_COUNT
};
		
// H264 Codec����ģʽ����
#define	XMSYS_H264_CODEC_MODE_IDLE					0		// H264 Codec����ģʽ
#define	XMSYS_H264_CODEC_MODE_RECORD				1		// H264 Codec¼���¼ģʽ
#define	XMSYS_H264_CODEC_MODE_PLAY					2		// H264 Codec¼��ط�ģʽ
		
// H264 Codec����״̬����
#define	XMSYS_H264_CODEC_STATE_STOP					0		// H264ֹͣ״̬
#define	XMSYS_H264_CODEC_STATE_WORK					1		// H264����״̬
#define	XMSYS_H264_CODEC_STATE_PAUSE				2		// H264��ͣ״̬(��¼��طž���)

// ��Ƶ����ģʽ
#define	XM_VIDEO_PLAYBACK_MODE_NORMAL				0		// ��������ģʽ (��������I֡��P֡����ʾ)
#define	XM_VIDEO_PLAYBACK_MODE_FORWARD				1		// �������ģʽ (������I֡����ͷ��β˳����ʾ)
#define	XM_VIDEO_PLAYBACK_MODE_BACKWARD				2		// ���˲���ģʽ (������I֡����β��ͷ˳����ʾ)
#define	XM_VIDEO_PLAYBACK_MODE_FRAME				3		// ��֡����ģʽ (ÿ�β��ŵ�֡, �ȴ�����������һ֡)

// ��ȡ��ǰH264 Codec�Ĺ���ģʽ
int XMSYS_H264CodecGetMode (void);

// ��ȡH264 Codec��ǰ�Ĺ���״̬
int XMSYS_H264CodecGetState (void);

// ������Ƶ¼��
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecRecorderStart (void);

// ֹͣ¼���¼
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecRecorderStop  (void);

// �л�����ͷͨ��
// ����ֵ
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecRecorderSwitchChannel  (unsigned int camera_channel);

// һ������
// 1��¼���� ����ǰ¼�Ƶ���Ƶ������������¼�����ӳ�3���ӡ�
// 2���ط��� ����ǰ�طŵ���Ƶ����
// ����ֵ
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecOneKeyProtect (void);


// ����ָ����¼���¼
// lpVideoFile			�ط���Ƶ�ļ���ȫ·����
// playback_mode		��Ƶ����ģʽ
// start_point			������ʼ�� (���뵥λ)							
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecPlayerStart (const char *lpVideoFile, unsigned int playback_mode, unsigned int start_point);

// ֹͣ����Ƶ���š�
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecPlayerStop (void);

// ��ͣ��ǰ���Ų���
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecPlayerPause (void);

// �ָ���������ͣ��¼���¼
void XMSYS_H264CodecPlayerResume (void);

// �����������
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecPlayerForward (void);

// �������˲���
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecPlayerBackward (void);

// ����һ�����չ���
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_JpegCodecOneKeyPhotograph (void);

// ��֡����
int XMSYS_H264CodecPlayerFrame (void);

// ֹͣ��Ƶ�������
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecStop (void);

// ��λ���ض�ʱ��λ��(time_to_play, ��λ��)
// -2 ����ִ�г�ʱ
// -1 ����Ƿ�������ִ��ʧ��
// 0  �����ѳɹ�ִ��
int XMSYS_H264CodecPlayerSeek (unsigned int time_to_play);

// ����AVI����ʹ�õ���Ƶ��ʽ
int XMSYS_H264CodecSetVideoFormat (int video_format);

// ��ȡAVI����ʹ�õ���Ƶ��ʽ
int XMSYS_H264CodecGetVideoFormat (void);

// ����H264��������
int XMSYS_H264CodecSetVideoQuality (unsigned int vide_quality);
		
// ���H264������Ƿ�æ
// 1 busy
// 0 idle
int XMSYS_H264CodecCheckBusy (void);

// ��ȡAVI��Ƶ�ĵ�һ֡ͼ��
// ����ֵ
//	0		�ɹ�
// -1		ʧ��
// ��֧��Y_UV420���
int  h264_decode_avi_stream_thumb_frame (	const char *filename, 				// AVI��ƵԴ�ļ�
														unsigned int thumb_width, 			// ���ųߴ綨��
														unsigned int thumb_height, 
														unsigned int thumb_stride,
														unsigned char *thumb_image[3]		// thumb_image[0]	Y
																									// thumb_image[1]	U/UV
																									// thumb_image[2] V
														);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */
		
#endif	// _XM_H264_CODEC_