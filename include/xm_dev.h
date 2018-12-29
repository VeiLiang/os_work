//****************************************************************************
//
//	Copyright (C) 2011 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xmdev.h
//	  constant��macro & basic typedef definition of X-Mini System
//
//	Revision history
//
//		2011.05.31	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_DEV_H_
#define _XM_DEV_H_

#if defined (__cplusplus)
	extern "C"{
#endif


// Modifier���Զ���
#define	XMKEY_PRESSED			0x10		/* key pressed */
#define	XMKEY_REPEAT			0x20		/* Key Repeat Status */
#define	XMKEY_STROKE			0x40		/* indicate hardware stroke */
#define	XMKEY_LONGTIME			0x80		/* long time pressed */

#define	XMEXIT_POWERDOWN			(WORD)(0)		// �����˳����ػ�
#define	XMEXIT_ROMERROR			(WORD)(-1)		// ROM��Ч�����


// OSD�����ʾ��(ԭ��λ�����Ͻ�)ƫ��
#define	OSD_MAX_H_POSITION	(2048 - 1)		// ���ˮƽ����ƫ��(������Ͻ�)
#define	OSD_MAX_V_POSITION	(2048 - 1)		// ���ֱ����ƫ��(������Ͻ�)

#define	OSD_MAX_H_SIZE			(2048)			// OSD������ؿ��
#define	OSD_MAX_V_SIZE			(2048)			// OSD������ظ߶�

// GUI��������ڣ�����ֵΪXMEXIT_xxx�˳���
int XM_Main (void);

// Ͷ�ݰ����¼����¼���Ϣ����
// ����ֵ����
//		1		�¼�Ͷ�ݵ��¼�������гɹ�
//		0		�¼�Ͷ�ݵ��¼��������ʧ��
unsigned char XM_KeyEventProc (unsigned short key, unsigned short mod);

// ��鲢Ͷ��Ӳ���¼�, ���ⲿӲ����������
// ����¼�δ��Ӳ����Ϣ������, �����¼����뵽��β.
// �����¼�����Ӳ����Ϣ������, �����κδ���, ����
// ����ֵ
//   1        ��ʾ����Ϣ��Ͷ�ݵ�Ӳ������
//   0        ��ʾ����Ϣδ��Ͷ�ݵ�Ӳ������, ��ϢͶ��ʧ��
unsigned char XM_KeyEventPost (unsigned short key, unsigned short mod);


// GUI���ں�ע�ᶨʱ���¼��ص�����
unsigned char XM_TimerEventProc (void);
// GUI���ں�ע��MCI�¼��ص�����
unsigned char XM_MciEventProc (void);


#define	TOUCH_TYPE_DOWN	1	// ���������¼�����
#define	TOUCH_TYPE_UP		2	// �����ͷ��¼�����
#define	TOUCH_TYPE_MOVE	3	// �����ƶ��¼�����
// Ͷ�ݴ����¼�����Ϣ����
// point		����λ��
// type		�����¼�����
// ����ֵ����
//		1		�¼�Ͷ�ݵ��¼�������гɹ�
//		0		�¼�Ͷ�ݵ��¼��������ʧ��
unsigned char XM_TouchEventProc (unsigned long point, unsigned int type);


// USB�¼�����
#define	USB_EVENT_NONE						0		// ��USB�¼�
#define	USB_EVENT_SYSTEM_UPDATE			1		// ϵͳ�����¼�

// Ͷ��Ӳ��USB�¼�, ���ⲿӲ����������
unsigned char XM_UsbEventProc (unsigned char UsbEvent);

#ifndef _XMBOOL_DEFINED_
#define _XMBOOL_DEFINED_
typedef unsigned char		XMBOOL;		// BOOL����
#endif

// ����Ϣ����Ͷ����Ϣ
XMBOOL	XM_PostMessage (WORD message, WORD wp, DWORD lp);

// ����Ϣ�����ײ�������Ϣ
XMBOOL	XM_InsertMessage (WORD message, WORD wp, DWORD lp);


// ���ж�
void XM_lock (void);

// ���ж�
void XM_unlock (void);

// �������״̬
void XM_idle (void);

// ����XM������
void XM_wakeup (void);

// �ж��豸������������ģʽ
// 1 ���� 0 ����
int XM_IsMasterDevice (void);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_DEV_H_
