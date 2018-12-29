//****************************************************************************
//
//	Copyright (C) ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xmtester.h
//	  �Զ����Խӿ�
//
//	Revision history
//
//
//		2010.04.25	ZhuoYongHong first version
//
//***************************************************************************

#ifndef _XM_TESTER_H_
#define _XM_TESTER_H_

#ifdef	__cplusplus
extern "C" {
#endif


// ����ģʽ����
#define	TESTER_MODE_SCRIPT				1		//	�ű��ļ�����ģʽ(��Ļλͼ�ػ����)
#define	TESTER_MODE_BITMAP				2		// �ű��ļ�����ģʽ(ֱ��BITMAPλͼ���)

// ���������
#define	TESTER_CMD_START					1		// ���Կ�ʼ
#define	TESTER_CMD_STOP					2		// ������ֹ
#define	TESTER_CMD_PAUSE					3		// ������ͣ
#define	TESTER_CMD_MESSAGE				4		// ��ȡ��һ��������Ϣ(Ӳ����ʹ������ʵ��)
#define	TESTER_CMD_TEST					5		// ����Զ������Ƿ���
#define	TESTER_CMD_RECORD					6		// ��ؼ�������	
#define	TESTER_CMD_KEYMAP					7		// ���ؼ��̶����
#define	TESTER_CMD_PEN						8		// ��ش�������

// ���Է���ֵ����
#define	TESTERSYSERR_NOERROR					0		// �޴���
#define	TESTERSYSERR_INVALIDCOMMAND		(-1)	// ��Ч����
#define	TESTERSYSERR_INVALIDPARAM			(-2)	// ��Ч�������
#define	TESTERSYSERR_BUSY						(-3)	// ����æ
#define	TESTERSYSERR_UNSUPPORT				(-4)	// δ֧������
#define	TESTERSYSERR_INVALIDSCRIPTFILE	(-5)	// ��Ч�Ľű��ļ�
#define	TESTERSYSERR_INVALIDSCRIPTLINE	(-6)	// ��Ч�Ľű������
#define	TESTERSYSERR_ENDOFSCRIPT			(-7)	// �ű��ļ�����
#define	TESTERSYSERR_BEGINOFMESSAGE		(-8)	// ��Ϣ�¼���ʼ
#define	TESTERSYSERR_INVALIDKEYMAPFILE	(-9)	// ��Ч�ļ���ӳ����ļ�
#define	TESTERSYSERR_FAILTTOCREATELOG		(-10)	// ���ܴ���LOG�ļ�
#define	TESTERSYSERR_FAILTOCREATERECORD	(-11)	// ���ܴ���RECORD�ļ�
#define	TESTERSYSERR_FAILTOCREATEBITMAP	(-12)	// ���ܴ���BITMAP�ļ�
#define	TESTERSYSERR_PENSCRIPT				(-13)	// �Ƿ��ı��¼���¼

// �Զ����Խ���֪ͨ����
#define	NTF_TESTER_FINISH						(LONG)(1)	// �Զ�������������
#define	NTF_TESTER_STOP						(LONG)(2)	// �Զ�����ǿ�ƽ���
#define	NTF_TESTER_EXCEPTION					(LONG)(3)	// �Զ������쳣����


#define	TESTER_RECORD_ON						((DWORD)(-1))
#define	TESTER_RECORD_OFF						((DWORD)(-2))

// ��ʾλͼ��׽����
typedef int (*FPCAPTUREBITMAP) (void *lpbm, void * lpbi);
// �Զ����Խ����ص�����
typedef int (*FPAUTOTESTSTOP)  (UINT uNitifyCode);	
//	uNotifyCode  �Զ����Խ���֪ͨ����
//		


typedef struct _XMTESTERSTART {
	UINT						uTestMode;					// ����ģʽ
	const char *			lpScriptFile;				// �ű��ļ�·��
	FPCAPTUREBITMAP		fpCaptureBitmap;			// λͼ��ȡ���������λͼ·��
																//		1)	��Ļλͼ�ػ����
																//			��ʾλͼ��׽����ָ��
																//		2)	ֱ��BITMAPλͼ���
																//			���λͼ·��
	const char *			lpKeyMapFile;				// ���̶����ļ�
	FPAUTOTESTSTOP			fpAutotestStop;			// �Զ����Խ����ص�����
} XMTESTERSTART;

typedef struct _XMTESTERMESSAGE {
	WORD					message;
	WORD					wParam;
	DWORD					lParam;
} XMTESTERMESSAGE;

typedef struct _XMTESTERRECORD {
	DWORD					key;
	DWORD					reserved;
} XMTESTERRECORD;

typedef struct _XMTESTERKEYMAP {
	char *				lpKeyMapFile;
} XMTESTERKEYMAP;

int	XM_TesterSendCommand (UINT uTestCommand, VOID *pCommandParam);

#ifdef	__cplusplus
}
#endif

#endif // _WINTESTER_H_