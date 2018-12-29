//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: app_processview.h
//	  ���̲����Ӵ�(��ʽ����ϵͳ�������ָ���������)
//
//	  ���̲����Ӵ�һ���ɹ̶��Ĺ��̲������, ����
//      1) ������ʼ���׶�
//         ��ʾ����ִ�еĶ�����Ϣ, ��ʾ�û���β��������뵽��һ�����̽׶�. ��
//	           ��"ȷ��"����ʼϵͳ����, ��"ȡ��"������. 
//      2) ����ȷ�Ͻ׶�
//         ��ʾ������Ϣ, ��ʾ��β���������������ò���. ��
//            ���°���"ȷ��"��ִ��ϵͳ����, ������������ϵ�. ��"ȡ��"������.
//      3) ��������ִ��״̬
//         ����ִ����, ��ʾ������Ϣ��ȴ�������֪�û���ǰ������״̬. ��
//            ϵͳ��������! ����ϵ��ػ�!
//      4) �������״̬
//         ����ִ�����, ��ʾ�ɹ�����ʧ�ܵ���Ϣ
//
//	Revision history
//
//		2013.09.01	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _XM_APP_PROCESS_VIEW_API_H_
#define _XM_APP_PROCESS_VIEW_API_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// �˵�ѡ��ص�����
#define PROCESS_FAIL		(-1)
#define PROCESS_INITIAL	(0)

#define	PROCESS_STATE_INITIAL			0	// ������ʼ��״̬
#define	PROCESS_STATE_CONFIRM			1	// ����ȷ����״̬
#define	PROCESS_STATE_WORKING			2	// ����ִ����״̬
#define	PROCESS_STATE_SUCCESS			3	// �����ɹ�״̬
#define	PROCESS_STATE_FAILURE			4	// ����ʧ��״̬


typedef int (*FPOPERATIONCB)		(void *);		// ���̲�������
typedef void (*FPSYSTEMEVENTCB)	(XMMSG *msg, int process_state);		// ϵͳ�¼��ص�����

// �����Ӵ�����
enum {
	AP_PROCESS_VIEW_FORMAT = 0,			// ��ʽ�������Ӵ�
	AP_PROCESS_VIEW_SYSTEMUPDATE,		// ϵͳ���������Ӵ�
	AP_PROCESS_VIEW_RESTORESETTING,		// �ָ��������ù����Ӵ�
	AP_PROCESS_VERSION,					//�汾����ʾ
	AP_PROCESS_VIEW_COUNT
};

typedef struct tagAP_PROCESSINFO {
	int					type;						// �Ӵ�����
	APPMENUID			Title;						// ���ڱ���
	APPMENUID			DispItem[APP_DISPLAY_ITEM_COUNT];				// ��ʾ��Ϣ
	int					nDispItemNum;				// ��ʾ��Ϣ��
	int					nMaxProgress;				// ������ֵ
	FPOPERATIONCB		fpStartProcess;				// ��ʼ������̻ص�����
	FPOPERATIONCB		fpQueryProgress;			// ������̻ص�����
	FPOPERATIONCB		fpEndProcess;				// ����������̻ص�����
	FPSYSTEMEVENTCB	fpSystemEventCb;				// ϵͳ�¼�����ص�����
	void *				lpPrivateData;				// ����˽������
} AP_PROCESSINFO;



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_APP_MENUDATA_H_


