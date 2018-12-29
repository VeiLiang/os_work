#ifndef _XM_VOICE_PROMPTS_H_
#define _XM_VOICE_PROMPTS_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// ϵͳ����ID����
enum {
	XM_VOICE_ID_BEEP_KEYBOARD = 1,			// ����Beep��
	XM_VOICE_ID_ONEKEY_PHOTOGRAPH,			// �����ǲ���
	XM_VOICE_ID_ALARM,							// ������
	XM_VOICE_ID_STARTUP,							// ¼��������

	
	XM_VOICE_ID_COUNT,


};

// ������ʾ�����ʼ��
// 0		�ɹ�
// < 0	ʧ��
int XM_voice_prompts_init (void);

// ������ʾ�������
// 0		�ɹ�
// < 0	ʧ��
int XM_voice_prompts_exit (void);

// ����һ���µ���ʾ����������������
// 0		�ɹ�
// < 0	ʧ��
int XM_voice_prompts_insert_voice (unsigned int voice_id);

// ɾ�����Ŷ�����ָ������ID����ʾ����
// 0		�ɹ�
// < 0	ʧ��
int XM_voice_prompts_remove_voice (unsigned int voice_id);

// ɾ�����Ŷ����е�������ʾ����
// 0		�ɹ�
// < 0	ʧ��
int XM_voice_prompts_remove_all_voice (void);


// ��ȡ��ʾ�������ı���Ϣ
const char *XM_voice_prompts_get_text (unsigned int voice_id);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_VOICE_PROMPTS_H_
