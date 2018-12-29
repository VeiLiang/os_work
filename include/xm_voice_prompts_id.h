#ifndef _XM_VOICE_PROMPTS_ID_H_
#define _XM_VOICE_PROMPTS_ID_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// ϵͳ����ID����
enum {
	XM_VOICE_ID_SYSCHECKING = 1,				// ϵͳ�ѿ����������Լ�
	XM_VOICE_ID_INIT_SETTING,					// ϵͳ����ʹ�ã�������
	XM_VOICE_ID_CARD_INSERT,					// ���ݿ��Ѳ���
	XM_VOICE_ID_CARD_NOCARD,					// ���ݿ������ڣ�����뿨
	XM_VOICE_ID_CARD_WRITEPROTECT,			// ���ݿ�д����������ȥ����д����
	XM_VOICE_ID_CARD_FILESYSTEMERROR,		// ���ݿ��ļ�ϵͳ�쳣
	XM_VOICE_ID_CARD_DAMAGE,					// ���ݿ����𻵣������
	XM_VOICE_ID_CARD_INVALID,					// SD���޷�ʶ�������²��������¿�
	XM_VOICE_ID_CARD_FORMAT,					// ��ȷ�����ݿ���ʽ������
	XM_VOICE_ID_TIMESETTING,					// ������ϵͳʱ��
	XM_VOICE_ID_BACKUPBATTERY,					// ���ݵ�ص����ľ��������
	XM_VOICE_ID_MAINBATTERY_WORST,			// ��ص����Ѻľ���ϵͳ�����ػ���
	XM_VOICE_ID_MIC_OFF,							// ¼���ѹر�	
	XM_VOICE_ID_MIC_ON,							// ¼���ѿ���
	XM_VOICE_ID_VOICE_PROMPTS_ON,				// ������ʾ�ѿ���
	XM_VOICE_ID_VOICE_PROMPTS_OFF,			// ������ʾ�ѹر�
	XM_VOICE_ID_TIMESTAMP_OFF,					// ʱ��ˮӡ�ѹر�	
	XM_VOICE_ID_TIMESTAMP_ON,					// ʱ��ˮӡ�ѿ���	
	XM_VOICE_ID_NOCARD_DEMO_OPERATIONMODE,	//	�޿�/ֻ����ʾģʽ
	XM_VOICE_ID_FORCASTRECORDTIME_LOW,		// ��ѭ��¼��ʱ���������ֵ
	XM_VOICE_ID_FORCASTRECORDTIME_TITLE,	// ѭ��¼��ʱ��Ԥ��
	XM_VOICE_ID_FORCASTRECORDTIME_WORST,	// ����20����
	XM_VOICE_ID_FORCASTRECORDTIME_20M,		// 20����
	XM_VOICE_ID_FORCASTRECORDTIME_30M,		// 30����
	XM_VOICE_ID_FORCASTRECORDTIME_40M,		// 40����
	XM_VOICE_ID_FORCASTRECORDTIME_50M,		// 50����
	XM_VOICE_ID_FORCASTRECORDTIME_1H,		// 1Сʱ
	XM_VOICE_ID_FORCASTRECORDTIME_2H,		// 2Сʱ
	XM_VOICE_ID_FORCASTRECORDTIME_3H,		// 3Сʱ
	XM_VOICE_ID_FORCASTRECORDTIME_4H,		// 4Сʱ
	XM_VOICE_ID_FORCASTRECORDTIME_5H,		// 5Сʱ
	XM_VOICE_ID_FORCASTRECORDTIME_6H,		// 6Сʱ
	XM_VOICE_ID_FORCASTRECORDTIME_7H,		// 7Сʱ
	XM_VOICE_ID_FORCASTRECORDTIME_8H,		// 8Сʱ
	XM_VOICE_ID_FORCASTRECORDTIME_9H,		// 9Сʱ
	XM_VOICE_ID_FORCASTRECORDTIME_10H,		// 10Сʱ
	XM_VOICE_ID_FORCASTRECORDTIME_11H,		// 11Сʱ
	XM_VOICE_ID_FORCASTRECORDTIME_12H,		// ����12Сʱ
	XM_VOICE_ID_ERGENT_RECORD,					// ����¼��������
	XM_VOICE_ID_ERGENT_RECORD_5M,				// ʱ�䳤��5����
	XM_VOICE_ID_ERGENT_RECORD_10M,			// ʱ�䳤��10����
	XM_VOICE_ID_ERGENT_RECORD_15M,			// ʱ�䳤��15����
	XM_VOICE_ID_ERGENT_RECORD_20M,			// ʱ�䳤��20����

	XM_VOICE_ID_CAMERA_DISCONNECT,			// Ӳ���쳣���޷����ӵ�����ͷ

	XM_VOICE_ID_USB_DISCONNECT,				// USB�Ѱγ�
	XM_VOICE_ID_USB_CONNECT_UDISK,			// U������ʹ��
	XM_VOICE_ID_USB_CONNECT_CHARGE,			// USB���ڳ��

	XM_VOICE_ID_RADAR_ALARM,					// ǰ���в����״�
	XM_VOICE_ID_BEEP_KEYBOARD,					// ����Beep��

	XM_VOICE_ID_DRIVING_LONGTIME_WARNING,	// ƣ�ͼ�ʻ(��ʱ���ʻ)Ԥ��
	XM_VOICE_ID_DRIVING_LONGTIME_ALARM,		// ƣ�ͼ�ʻ(��ʱ���ʻ)����
	
	XM_VOICE_ID_COUNT,

	XM_VOICE_ID_CCD_REAR,						// ������ͷ(�����������γ���������)
	XM_VOICE_ID_CCD_LEFT,						// ������ͷ(�����������γ���������)
	XM_VOICE_ID_CCD_RIGHT,						// ������ͷ(�����������γ���������)
	XM_VOICE_ID_CCD_INNER,						// ������ͷ(�����������γ���������)
	XM_VOICE_ID_CCD_LOST_CONNECT,				// δ������
	XM_VOICE_ID_CCD_CONNECT						// ������
};

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_VOICE_PROMPTS_ID_H_
