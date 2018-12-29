//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xmuser.h
//	  constant��macro & basic typedef definition of user
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_USER_H_
#define _XM_USER_H_

#include <xmtype.h>
#include <xm_osd_framebuffer.h>
#include <xm_osd_layer_animate.h>

#if defined (__cplusplus)
	extern "C"{
#endif

// Macro definition

// ��ʱ����Դ
#define	XM_MAX_TIMER				16			// ϵͳ���ɷ��䶨ʱ����Դ

#define	XM_MAX_MSG					8			// ϵͳ��Ϣ���д�С

#define	MAX_HWND_STACK				8			// �Ӵ�ջ�����

#define	MAX_HWND_WIDGET_COUNT	4			// һ�����������������Ŀؼ�����

#define	MAX_STACK_WIDGET_COUNT	8		// �Ӵ�ջ�������ͬʱ����Ŀؼ����� (�����Ӵ��еĿؼ��ۼӸ���)



// ��Ϣ���Ͷ���

#define	XM_KEYDOWN		0x01
										// wp ������ֵ��lp ����״̬
										
#define	XM_KEYUP			0x02
										// wp ������ֵ��lp ����״̬
										
#define	XM_QUIT			0x03	// ��Ϣѭ���˳���Ϣ
										// wp����Ϊ0, lpΪ�������õķ���ֵ
										
#define	XM_CHAR			0x04	// �ַ���Ϣ
										// wp =  0, lpΪ16ΪUnicode16����
										// wp != 0, �ַ�����ֵΪUnicode���� (wp << 16) | lp,
										
#define	XM_TIMER			0x05	// ��ʱ����Ϣ
										// wp ��ʱ��ID��lp ����Ϊ0
										
#define	XM_PAINT			0x06	// ��Ļˢ����Ϣ
										// wp,lp����Ϊ0
										
#define	XM_COMMAND		0x07	// ������Ϣ
										// wpΪ����ID��lp������ID���
										
#define	XM_ALARM			0x08	// ������Ϣ


#define	XM_CLOCK			0x09	// ÿ����Ϣ
										// wp,lp����Ϊ0
										
#define	XM_MCI			0x0A	// ��ý�岥�ſ�����Ϣ
										// wp MCI֪ͨ��
										// lp ��wp��صĸ�����Ϣ

#define	XM_ENTER			0x0B	// �Ӵ�����
										// lp����Ϊ0
										// wp = 0, ���Ӵ����� (���Ӵ�δ����)
										// wp = 1, �������Ӵ����ص����Ӵ� (���Ӵ��Ѵ���)

#define	XM_LEAVE			0x0C	// �Ӵ��˳�
										// lp����Ϊ0
										// wp = 0, �Ӵ������˳� (���Ӵ����ݻ�)
										// wp = 1, �Ӵ���ʱ�뿪����ѹ���µ������Ӵ� (���Ӵ���ǰ״̬����)

#define	XM_SYSTEMEVENT	0x10	// ϵͳ�¼�����
										// lp ����Ϊ0
										// wp �ο� XM_SYSTEMEVENT �� WP ��������
										//		1)	һ��������Ӵ�view���账����¼�����ϵͳȱʡ����
										//		2)	���Ӵ�view��Ҫ����ĳ���¼�������������ж��Ƿ�������¼����ݸ�ϵͳȱʡ����
										//			ͨ������API XM_BreakSystemEventDefaultProcess ����ֹϵͳȱʡ����(��ϵͳ���ٴ�����¼�)						

// XM_SYSTEMEVENT �� WP ��������

// �����ļ�ϵͳ�¼�
#define SYSTEM_EVENT_CARD_DETECT							0		// SD��׼����(SD�������)
#define SYSTEM_EVENT_CARD_WITHDRAW						1		// SD���γ��¼�
#define SYSTEM_EVENT_CARD_UNPLUG							1		// SD���γ��¼�

#define SYSTEM_EVENT_CARD_INSERT_WRITE_PROTECT		2		// SD������(д����)
#define SYSTEM_EVENT_CARD_INSERT							3		// SD������(��д����)
#define SYSTEM_EVENT_CARD_FSERROR						4		// SD������(�ļ�ϵͳ�����쳣)
#define SYSTEM_EVENT_CARD_FS_ERROR	SYSTEM_EVENT_CARD_FSERROR
#define SYSTEM_EVENT_CARD_VERIFY_ERROR					5		// SD����дУ����ʧ�ܣ�SD������
#define SYSTEM_EVENT_CARD_INVALID						6		// SD���޷�ʶ��
#define SYSTEM_EVENT_CARD_DISKFULL						7		// SD����������
#define SYSTEM_EVENT_CARD_FSUNSUPPORT					8		// �޷�֧�ֵ������ļ�ϵͳ��ʽexFAT����NTFS
#define SYSTEM_EVENT_RECORD_SPACE_INSUFFICIENT		9		// ��¼ʱ�䱨���¼�(��¼�ռ��)

// ��ʾ������롢�Ͽ��¼�
#define SYSTEM_EVENT_AVOUT_PLUGOUT						10		// AVOUT�γ�
#define SYSTEM_EVENT_AVOUT_PLUGIN						11		// AVOUT����
#define SYSTEM_EVENT_HDMI_PLUGIN							12		// HDMI�豸����
#define SYSTEM_EVENT_HDMI_PLUGOUT						13		// HDMI�豸�γ�

// ��ݰ����¼�
#define SYSTEM_EVENT_ADJUST_BELL_VOLUME				20		// ��ݰ����¼������������������������ر�
#define SYSTEM_EVENT_ADJUST_MIC_VOLUME					21		// ��ݰ����¼�������¼���������������ر�
#define SYSTEM_EVENT_URGENT_RECORD						22		// ��ݰ����¼�������¼��(һ������)

// ϵͳ����¼�
#define SYSTEM_EVENT_MAIN_BATTERY						30		// ����ر仯�¼�
#define SYSTEM_EVENT_BACKUP_BATTERY						31		// ���ݵ�ر仯�¼�

// USB�β��¼�
#define SYSTEM_EVENT_USB_DISCONNECT						40		// USB�Ͽ�����
#define SYSTEM_EVENT_USB_CONNECT_CHARGE				41		// USB��Ϊ�����ʹ��
#define SYSTEM_EVENT_USB_CONNECT_UDISK					42		// USB�����ӣ���Ϊ�����豸��U��ʹ��
#define SYSTEM_EVENT_USB_CONNECT_CAMERA				43		// USB�����ӣ���Ϊ�����豸��Camera����ʹ��

// GPS���������¼�
#define SYSTEM_EVENT_GPSBD_DISCONNECT					50		// GPSBD�ѶϿ�
#define SYSTEM_EVENT_GPSBD_CONNECT_ANTENNA_OPEN		51		// GPSBD������(����δ����)
#define SYSTEM_EVENT_GPSBD_CONNECT_ANTENNA_SHORT	52		// GPSBD������(���߶�·)
#define SYSTEM_EVENT_GPSBD_CONNECT_ANTENNA_OK		53		// GPSBD������(������������)
#define SYSTEM_EVENT_GPSBD_CONNECT_LOCATE_OK			54		// GPSBD�Ѷ�λ

// ��ʻ�¼�
#define SYSTEM_EVENT_DRIVING_LONGTIME_WARNING		60		// ƣ�ͼ�ʻԤ���¼�
#define SYSTEM_EVENT_DRIVING_LONGTIME_ALARM			61		// ƣ�ͼ�ʻ�����¼�

// ��Ƶ���¼�
#define SYSTEM_EVENT_VIDEOITEM_LOW_SPACE				70		// ��Ƶ���ѭ���ռ�ͣ�������������ռ�ÿռ�̫������������ļ�ռ��̫��
																			//		����¼��ڿ�����ʱ����Ƶ��¼�����з�����

#define SYSTEM_EVENT_VIDEOITEM_LOW_SPEED				71		// ��Ƶ���ļ�д���ٶȵ�(���ٿ����ļ�ϵͳ��Ƭ������ᵼ��)
																			//		����¼��ڼ�¼�����з�����
																			//		��д���ٶ����޷�����ʵʱ��¼ʱ(�������ض�֡����)�����Ѹ�ʽ��SD��	

#define SYSTEM_EVENT_VIDEOITEM_LOW_PREFORMANCE		72		// �ļ�ϵͳ�Ĳ�����Ҫ�������ã�ȷ��������Ƶ��¼��Ҫ��
																			//		һ��ָSD���Ĵش�С̫С�����¿���д�ٶ��޷����������Ƶʵʱ��д��
																			//		����¼��ڿ�����ʱ����
																			//		����Ҫ���Ѹ�ʽ��SD��

#define SYSTEM_EVENT_VIDEOITEM_ERROR					73		//	��Ƶ�����ݿ��쳣����Ҫ��ʽ��SD��
																			//		����¼��ڿ�����ʱ����

#define SYSTEM_EVENT_VIDEOITEM_RECYCLE_CONSUMED		74		//	��Ƶ�����ݿ��쳣����ѭ��ʹ�õ���Դ�Ѻľ���
																			//		��Ҫ�ֹ�ɾ����Ƶ���߸�ʽ��SD��
																			//		����¼��ڿ�¼�ƹ����з���

#define SYSTEM_EVENT_SYSTEM_UPDATE_FILE_CHECKED		80		// �ҵ��Ϸ���ϵͳ�����ļ�

#define SYSTEM_EVENT_SYSTEM_UPDATE_FILE_MISSED		81		// �޷��ҵ�ϵͳ�����ļ���
																			//		������ʧ�ܺ�(�����쳣)���´�����ʱ�Զ����뵽ϵͳ����ģʽ��
																			//		���޷��ҵ������ļ���Ͷ�ݸ�ϵͳ�¼�
#define SYSTEM_EVENT_SYSTEM_UPDATE_FILE_ILLEGAL		82		// �Ƿ���ϵͳ�����ļ�
																			//		
#define SYSTEM_EVENT_SYSTEM_UPDATE_SUCCESS			83
#define SYSTEM_EVENT_SYSTEM_UPDATE_FAILURE			84

// �����״ﱨ���¼�
#define SYSTEM_EVENT_RADAR_ALARM							(100+24)		// �����״��źű���

//#define SYSTEM_EVENT_RECORD_SPACE
// CCD�Ͽ��¼�
#define SYSTEM_EVENT_CCD0_LOST_CONNECT				100
#define SYSTEM_EVENT_CCD1_LOST_CONNECT				101
#define SYSTEM_EVENT_CCD2_LOST_CONNECT				102
#define SYSTEM_EVENT_CCD3_LOST_CONNECT				103
#define SYSTEM_EVENT_CCD4_LOST_CONNECT				104
#define SYSTEM_EVENT_CCD5_LOST_CONNECT				105
#define SYSTEM_EVENT_CCD6_LOST_CONNECT				106
#define SYSTEM_EVENT_CCD7_LOST_CONNECT				107
#define SYSTEM_EVENT_CCD8_LOST_CONNECT				108
#define SYSTEM_EVENT_CCD9_LOST_CONNECT				109
#define SYSTEM_EVENT_CCDA_LOST_CONNECT				110
#define SYSTEM_EVENT_CCDB_LOST_CONNECT				111
#define SYSTEM_EVENT_CCDC_LOST_CONNECT				112
#define SYSTEM_EVENT_CCDD_LOST_CONNECT				113
#define SYSTEM_EVENT_CCDE_LOST_CONNECT				114
#define SYSTEM_EVENT_CCDF_LOST_CONNECT				115
// CCD�����¼�
#define SYSTEM_EVENT_CCD0_CONNECT					120
#define SYSTEM_EVENT_CCD1_CONNECT					121
#define SYSTEM_EVENT_CCD2_CONNECT					122
#define SYSTEM_EVENT_CCD3_CONNECT					123
#define SYSTEM_EVENT_CCD4_CONNECT					124
#define SYSTEM_EVENT_CCD5_CONNECT					125
#define SYSTEM_EVENT_CCD6_CONNECT					126
#define SYSTEM_EVENT_CCD7_CONNECT					127
#define SYSTEM_EVENT_CCD8_CONNECT					128
#define SYSTEM_EVENT_CCD9_CONNECT					129
#define SYSTEM_EVENT_CCDA_CONNECT					130
#define SYSTEM_EVENT_CCDB_CONNECT					131
#define SYSTEM_EVENT_CCDC_CONNECT					132
#define SYSTEM_EVENT_CCDD_CONNECT					133
#define SYSTEM_EVENT_CCDE_CONNECT					134
#define SYSTEM_EVENT_CCDF_CONNECT					135


#define	XM_BARCODE		0x11	// �����������¼�
										// wp, lp����Ϊ0

#define	XM_USB			0x12	// USB�����¼�
										// wp USB�¼�����
										// lp ����Ϊ0

#define	XM_VIDEOSTOP	0x20	// ��Ƶ���Ž�����Ϣ
										// wp �˳��� (�ο�app_video.h����)
// ��Ƶ������Ϣ(XM_VIDEOSTOP)���˳��붨��
#define	AP_VIDEOEXITCODE_FINISH			0x0000		// ���벥�Ž���
#define	AP_VIDEOEXITCODE_LOWVOLTAGE	0x0001		// ��ѹ�쳣����
#define	AP_VIDEOEXITCODE_STREAMERROR	0x0002		// ����ʽ����
#define	AP_VIDEOEXITCODE_OTHERERROR	0x0003		// �����쳣(��SD���쳣���ļ�ϵͳ�쳣��)									

#define	XM_USER			0x80	// �û��Զ�����Ϣ(0x80 ~ 0xFF)

// Window�Ӵ���־����
#define	HWND_DISPATCH	((BYTE)0x01)		// ��Ϣ�ɷ���
#define	HWND_DIRTY		((BYTE)0x02)		// ��ʶ������Ҫ�ػ�(�������пؼ�)
#define	HWND_ANIMATE	((BYTE)0x80)		// ��ʶ�Ӵ�֧�ֶ���Ч��

// Widget�ؼ���־����
#define	WDGT_VISUAL		((BYTE)0x01)		// ��ʶ�ؼ����ڿ���״̬
#define	WDGT_ENABLE		((BYTE)0x02)		// ��ʶ�ؼ�����ʹ��״̬
#define	WDGT_FOCUS		((BYTE)0x04)		// ��ʶ�ؼ����пɾ۽����ԣ����ɱ��۽�
#define	WDGT_SELECT		((BYTE)0x08)		// ��ʶ�ؼ�����ѡ��״̬ (CheckBox�ؼ��ɾ��иñ�־)
#define	WDGT_FOCUSED	((BYTE)0x10)		// ��ʶ�ؼ����ھ۽�״̬����ӵ�����뽹��
#define	WDGT_DIRTY		((BYTE)0x20)		// ��ʶ�ؼ�������״̬����Ҫ�ػ�

#define	WIDGET_IS_FOCUS(flag)		((flag) & WDGT_FOCUS)
#define	WIDGET_IS_FOCUSED(flag)		((flag) & WDGT_FOCUSED)
#define	WIDGET_IS_SELECT(flag)		((flag) & WDGT_SELECT)
#define	WIDGET_IS_ENABLE(flag)		((flag) & WDGT_ENABLE)
#define	WIDGET_IS_VISUAL(flag)		((flag) & WDGT_VISUAL)
#define	WIDGET_IS_DIRTY(flag)		((flag) & WDGT_DIRTY)

#define	HWND_VIEW			0x01				// �Ӵ�����
#define	HWND_ALERT			0x02				// ��������
#define	HWND_EVENT			0x03				// ֪ͨ����

#define	XM_VIEW_DEFAULT_ALPHA		200	// �Ӵ�ȱʡ͸����

// ȱʡ��������
#define	HWND_CUSTOM_DEFAULT			((DWORD)(0))

// ��ȡ�Ӵ��ص�����
#define	XMPROC(hwnd)		HWND_##hwnd##_WindowProc

// �Ӵ��ص���������
#define	XMPROC_DECLARE(hwnd)	\
extern VOID HWND_##hwnd##_WindowProc (XMMSG *msg);

// �Ӵ��ص��������忪ʼ
#define	XM_MESSAGE_MAP_BEGIN(hwnd) \
VOID HWND_##hwnd##_WindowProc (XMMSG *msg) {

// ��Ϣ����������
#define	XM_ON_MESSAGE(event,proc) \
	if(msg->message == event) \
	{	\
		proc (msg); \
		return; \
	}

// �Ӵ��ص������������
#define	XM_MESSAGE_MAP_END \
	XM_DefaultProc (msg);	\
}

// �Ӵ�����
#define	XMHWND_DEFINE(x,y,cx,cy,hwnd,erase,lpWidget,cbWidget,alpha,type) \
XMHWND hWnd_##hwnd## = {\
	x,y,cx,cy, \
	XMPROC(hwnd),	\
	lpWidget, cbWidget, \
	alpha, \
	erase, \
	type, \
	0, 0, 0, 0 \
};

#define	XMHWND_DECLARE(hwnd)		extern XMHWND hWnd_##hwnd##;

// ��ȡ�Ӵ��ص�����
#define	XMWDGTPROC(hwnd)		WDGT_##hwnd##_WindowProc

// �Ӵ��ص���������
#define	XMWDGTPROC_DECLARE(hwnd)	\
extern VOID WDGT_##hwnd##_WindowProc (const XMWDGT *pWidget, BYTE bWidgetFlag, VOID *pUserData, XMMSG *msg);

// �Ӵ��ص��������忪ʼ
#define	WIDGET_MESSAGE_MAP_BEGIN(hwnd) \
VOID WDGT_##hwnd##_WindowProc (const XMWDGT *pWidget,BYTE bWidgetFlag,VOID *pUserData,XMMSG *msg) {

// ��Ϣ����������
#define	WIDGET_ON_MESSAGE(event,proc) \
	if(msg->message == event) \
	{	\
		proc (pWidget,bWidgetFlag,pUserData,msg); \
		return; \
	}

// �Ӵ��ص������������
#define	WIDGET_MESSAGE_MAP_END \
}

// ��Ϣ���ݿ���ѡ��
#define	XMMSG_OPTION_SYSTEMEVENT_DEFAULT_PROCESS		0x00000001		// ����ϵͳ�¼���Ϣ��ȱʡ����

// structure definition
// ��Ϣ�ṹ����
typedef struct tagXMMSG {
	WORD		message;			// ��Ϣ����
	WORD		wp;				// ��Ϣ�ֽڲ���
	DWORD		lp;				// ��Ϣ�ֲ���
	DWORD		option;			// ��Ϣ����ѡ��
} XMMSG;

// �Ӵ�ջ����
typedef struct _HWND_NODE {
	HANDLE	hwnd;
	BYTE *	lpWidgetFlag;		// �Ӵ����ӿؼ���־
	VOID **	UserData;			// �Ӵ��ӿؼ����û�����
	VOID *	PrivateData;		// ���ڵ�˽������
	BYTE		flag;					// �Ӵ���״̬��־
	BYTE		cbWidget;			// �ӿؼ�����

	BYTE		alpha;				// �Ӵ�alpha����


	UINT		animatingDirection;
	xm_osd_framebuffer_t framebuffer;
} HWND_NODE;

// ��ȡ�Ӵ��ṹ�ľ��
#define	XMHWND_HANDLE(hwnd)						((HANDLE)(&(hWnd_##hwnd##)))

#define	ADDRESS_OF_HANDLE(handle)				((void *)(handle))

// typedef definition
typedef VOID (*XMWNDPROC)(XMMSG *);

typedef struct tagXMWND {
//	XMWNDPROC	lpfnWndProc;	// �ؼ���Ϣ�ص�����
	XMCOORD		_x;				// �ؼ�X����(��������)
	XMCOORD		_y;				// �ؼ�X����(��������)
	XMCOORD		_cx;				// �ؼ����
	XMCOORD		_cy;				// �ؼ��߶�
} XMWND;

typedef struct tagXMWDGT {
//	XMWNDPROC	lpfnWndProc;	// �ؼ���Ϣ�ص�����
	XMCOORD		_x;				// �ؼ�X����(��������)
	XMCOORD		_y;				// �ؼ�X����(��������)
	XMCOORD		_cx;				// �ؼ����
	XMCOORD		_cy;				// �ؼ��߶�
	
	DWORD			dwTitleID;		// �ؼ������ı�����ԴID.
										//		0 ��ʾ�Ӵ����ƣ����Ӵ�������Ӧ�Ŀؼ��ı����ú�����XM_XXXSetText
										//		(DWORD)(-1)��ʾ������������簴ťΪͼƬʱ

	WORD			wForBmpID;		// ǰ��ͼƬID, 0 ��ʾ��ͼƬ����Button�ж�Ӧ���ͷ�ʱ��ͼƬЧ��
	WORD			wBkgBmpID;		// ����ͼƬID, 0 ��ʾ��ͼƬ����Button�ж�Ӧ�ڰ���ʱ��ͼƬЧ��
	WORD			wDisBmpID;		// ��ֹ״̬ͼƬID, 0 ��ʾ��ͼƬ
	
	BYTE			bHotKey;			// ��ؼ���Ӧ���ȼ���0 ��ʾ���ȼ���Ӧ
	BYTE			bCommand;		// �ؼ����0 ��ʾ�����
										//		�ؼ��ȼ����»�۽�״̬�°��¿ո񣬿ؼ���������Ͷ�ݵ���Ϣ���С�
	BYTE			bFlag;			// �ؼ���ʼ״̬����	Widget�ؼ���־����
	BYTE			bType;			// �ӿؼ����͡�ͨ���ؼ����ͻ�ȡ�ؼ��Ļص�����

} XMWDGT;


// �Ӵ��ṹ���� (32�ֽڣ�Cache line aligned)
typedef struct tagXMHWND {
	XMCOORD			_x;				// �Ӵ�X����(��������)		
	XMCOORD			_y;				// �Ӵ�Y����(��������)
	XMCOORD			_cx;				// �Ӵ����
	XMCOORD			_cy;				// �Ӵ����

	XMWNDPROC		lpfnWndProc;	// �Ӵ���Ϣ�ص�����

	

	const XMWDGT	*lpWidget;		// �Ӵ��ӿؼ��б�
											//		�ؼ�������ROM�У���СRAM�����󣬲������޸�
											//		�������޸Ŀؼ��ı�־(Flag), �� Widget�ؼ���־����
	BYTE				cbWidget;		// �ؼ��ĸ���
	BYTE				alpha;			// ͸������Alpha��0 ȫ͸�� 255 ȫ����
	BYTE				erase;			// �Ƿ��������, 0 ��ʾ���������� 1 ��ʾ��������
	BYTE				type;				// �������� (�Ӵ�������)

	XMCOORD			view_x;			// �Ӵ����ڵ�x����
	XMCOORD			view_y;			// �Ӵ����ڵ�y����
	XMCOORD			view_cx;			// �Ӵ����ڵĿ��
	XMCOORD			view_cy;			// �Ӵ����ڵĸ߶�
	DWORD				scale_mode;		// �Ӵ�����ģʽ��
											//		0 --> �Զ�����ģʽ(ȱʡ����)  
											//		1 --> ������ģʽ


} XMHWND, *PXMHWND;

// function protocol type

// ��ȡ��Ϣ������Ϣ����Ϊ��ʱ���û��������
// �˺����������ں�ʹ�ã�Ӧ�ó�������ʹ�á�
// XMBOOL	XM_GetMessage (XMMSG *msg);

// ����ָ������Ϣ��
// msg��ΪNULL����ɾ��ָ������Ϣ
// bMsgFilterMin = 0 && bMsgFilterMax = 0��ʾ����������Ϣ
// �ɹ�����TRUE��ʧ�ܷ���FALSE
XMBOOL 	XM_PeekMessage(XMMSG *msg,	BYTE bMsgFilterMin, BYTE bMsgFilterMax);

//
XMBOOL	XM_GetMessage (XMMSG *msg);

// ����Ϣ����Ͷ����Ϣ
XMBOOL	XM_PostMessage (WORD message, WORD wp, DWORD lp);

// ֱ�ӵ����Ӵ��ص�����
// XMBOOL	XM_SendMessage (WORD message, WORD wp, DWORD lp);	// ��ȡ���ú���

// �ɷ���Ϣ
XMBOOL	XM_DispatchMessage (XMMSG *msg);

// ��ֹϵͳ�¼���Ϣ��ȱʡ����
VOID		XM_BreakSystemEventDefaultProcess (XMMSG *msg);

// Ͷ����Ϣѭ��������Ϣ
// wExitCode ����
#define	XMEXIT_REBOOT					(1)	// ����
#define	XMEXIT_SLEEP					(2)	// �ػ�
#define	XMEXIT_EXCEPT					(3)	// �쳣
#define	XMEXIT_CHANGE_RESOLUTION	(3)	// �޸�UI�ֱ����¼�

XMBOOL	XM_PostQuitMessage (WORD wExitCode);

// �����Ϣ�����е�������Ϣ
void 		XM_FlushMessage (void);

// ɾ�����������з�COMMAND��Ϣ
void 		XM_FlushMessageExcludeCommandAndSystemEvent (void);

// ���ü���ȡ˽������
XMBOOL	XM_SetWindowPrivateData (HANDLE hWnd, void *PrivateData);
void *	XM_GetWindowPrivateData (HANDLE hWnd);

// ���ö�ʱ������msΪ��λ����idTimerΪ��ʱ��ID
XMBOOL	XM_SetTimer		(BYTE idTimer , DWORD dwTimeout);

// ɾ����ʱ��
XMBOOL	XM_KillTimer	(BYTE idTimer);

// ϵͳȱʡ��Ϣ������
VOID		XM_DefaultProc (XMMSG *msg);

// ϵͳ�¼���Ϣȱʡ����
// hWnd  ��ǰ�Ӵ����
// msg	ϵͳ�¼���Ϣ
VOID		XM_DefaultSystemEventProc (HANDLE hWnd, XMMSG *msg);

// ʹ����������Ч��ϵͳͶ��XM_PAINT��Ϣ����Ϣ����β��
XMBOOL	XM_InvalidateWindow (void);

// ʹĳ���ؼ���Ч��ϵͳͶ��XM_PAINT��Ϣ����Ϣ����β��
XMBOOL XM_InvalidateWidget (BYTE bWidgetIndex);


// ϵͳ�����Ϣ�����е�XM_PAINT��Ϣ�������ڣ�����������Ϣ�����ײ����Ա�����һ����Ϣѭ����XM_PAINT��Ϣ������
XMBOOL	XM_UpdateWindow (void);

// XM_PushWindow
// �ڵ�ǰ�Ӵ���ѹ���µ��Ӵ���Ӧ������Ҫ����������ҿ����𼶷��ص����Ρ�
// ִ��Push��������Ϣ���б����
// ���治��Ҫѹ�뵽ջ�С���ջΪ��ʱ���Զ�������ѹ�뵽ջ����
// ���ֵ�Ӧ���У�������ʼ����-->�����������-->�ֵ����Ľ���-->�������Խ��棬���Ӵ�ջ��ʾ����
//    ջ��-->          �������Խ���
//                     �ֵ����Ľ���
//                     �����������
//    ջ��-->          ������ʼ����
XMBOOL	XM_PushWindow	(HANDLE hWnd);

// ��ѹ���µ��Ӵ�ʱ��ͬʱָ���´��ڵĶ�����Ϣ
// ������Ϣ(dwCustomData)ͨ��XM_ENTER��Ϣ��lp��������
XMBOOL	XM_PushWindowEx	(HANDLE hWnd, DWORD dwCustomData);


// XM_PullWindow
// 1) hWnd = 0, ����ջ���Ӵ�,���ڷ��ص�ǰһ���Ӵ���
//  ��� ���������Խ��桱 ���ص� ���ֵ����Ľ��桱
//
// 2) hWnd != 0, ѭ������ջ���Ӵ���ֱ���ҵ�ָ�����Ӵ�(hWnd)���������Ӵ���Ϊջ���Ӵ�
// �����һ���ַ������´� ���������Խ��桱 ���ص� ��������ʼ���桱
XMBOOL	XM_PullWindow	(HANDLE hWnd);

// XM_JumpWindow
// ���Ӵ�ջ�������Ӵ������Ȼ�������µ��Ӵ���һ�����ڹ�������תʹ��
// ��������Ӵ�ջ����£����¡�ϵͳʱ�䡱���ܼ�����ֱ�ӵ���XM_WndGoto����ת����ϵͳʱ�䡱��
XMBOOL	XM_JumpWindow	(HANDLE hWnd);

// ��ת(JUMP)�Ӵ��������Ͷ���
typedef enum {
	XM_JUMP_POPDEFAULT = 0,			//	���Ӵ�ջ�������Ӵ����(��ȥ����)��Ȼ�������µ��Ӵ�
	XM_JUMP_POPDESKTOP,				// ���Ӵ�ջ�������Ӵ����(��������)��Ȼ�������µ��Ӵ�
	XM_JUMP_POPTOPVIEW,				// ���Ӵ�ջ��ջ���Ӵ������Ȼ�������µ��Ӵ� 
} XM_JUMP_TYPE;

// XM_JumpWindow
// ���Ӵ�ջ��ջ���Ӵ��������Ӵ����(ȡ����JumpType)��Ȼ�������µ��Ӵ���һ�����ڹ�������תʹ��
// ��������Ӵ�ջ����£����¡�ϵͳʱ�䡱���ܼ�����ֱ�ӵ���XM_WndGoto����ת����ϵͳʱ�䡱��
XMBOOL	XM_JumpWindowEx	(HANDLE hWnd, DWORD dwCustomData, XM_JUMP_TYPE JumpType);

// XM_GetWindowID
// ��ȡ��ǰ�Ӵ�(��ջ���Ӵ�)��ΨһID
// ��ջ�д��ڶ��ͬһ�Ӵ�������Ӵ�ʱ��������ÿ���Ӵ���
BYTE		XM_GetWindowID	(VOID);

// ��������
XMBOOL XM_InflateRect (XMRECT *lprc, XMCOORD dx, XMCOORD dy);

// �����ƶ�
XMBOOL XM_OffsetRect (XMRECT *lprc, XMCOORD dx, XMCOORD dy);

// ������(�Ӵ���ؼ�)����ת��Ϊ��Ļ����
XMBOOL XM_ClientToScreen (HANDLE hWnd, XMPOINT *lpPoint);

// ����Ļ����ת��Ϊ����(�Ӵ���ؼ�)����
XMBOOL XM_ScreenToClient (HANDLE hWnd, XMPOINT *lpPoint);

// ��ȡ����(�Ӵ���ؼ�)��λ����Ϣ
XMBOOL XM_GetWindowRect (HANDLE hwnd, XMRECT* lpRect);

// ���ô���ʵ����λ��(�����ʾ��ԭ��)
XMBOOL XM_SetWindowPos (HANDLE hWnd, 
								XMCOORD x, XMCOORD y,			// ��Ļ����
								XMCOORD cx, XMCOORD cy
								);

// ��ȡ������Ӵ����
HANDLE XM_GetDesktop (void);

// ��ȡ�������ʾ��������
VOID XM_GetDesktopRect (XMRECT *lpRect);

XMBOOL XM_SetRect (XMRECT* lprc, XMCOORD xLeft, XMCOORD yTop, XMCOORD xRight, XMCOORD yBottom);

// ʹ���Ӵ��������ر�ʱ�Ķ���Ч��
XMBOOL XM_EnableViewAnimate (HANDLE hWnd);
// ��ֹ�Ӵ��������ر�ʱ�Ķ���Ч��
XMBOOL XM_DisableViewAnimate (HANDLE hWnd);

// ���õ�ǰ�۽��ؼ�
XMBOOL XM_SetFocus (BYTE bWidgetIndex);

// (BYTE)(-1)��ʾû�о۽��Ŀؼ�
BYTE XM_GetFocus (VOID);

// �����ӿؼ���ѡ��״̬
XMBOOL XM_SetSelect (BYTE bWidgetIndex, XMBOOL bSelect);

// �����ӿؼ���ʹ��״̬
XMBOOL XM_SetEnable (BYTE bWidgetIndex, XMBOOL bEnable);

// �����ӿؼ��Ŀ���״̬
XMBOOL XM_SetVisual (BYTE bWidgetIndex, XMBOOL bVisual);

// ��ȡ�ؼ����Ӵ��ӿؼ�����, ʧ�ܷ���(BYTE)(-1)
BYTE XM_GetWidgetIndex (const XMWDGT *pWidget);

// ��ȡ�ؼ����û�����
VOID *XM_GetWidgetUserData (BYTE bWidgetIndex);

// ���ÿؼ����û�����
XMBOOL XM_SetWidgetUserData (BYTE bWidgetIndex, VOID *pUserData);

// ��ȡ��ǰ�Ӵ��ľ��
HANDLE XM_GetWindow (VOID);

// ϵͳ����ʱ��ϵͳ������ã�ֻ����һ��
VOID XM_AppInit (VOID);

// ϵͳ����ʱ��ϵͳ������ã�ֻ����һ��
VOID XM_AppExit (VOID);

// ��ȡ�Ӵ�������framebuffer����
//	����XM_PAINT��Ϣʱ�Ӵ����Զ�����һ��framebuffer����
//	������Ϣ��������У�����Ҫ����ˢ����ʾ��ִ����������
//	1)	����һ�����Ӵ�������framebuffer����XM_osd_framebuffer_create
//	2)	����framebuffer�����Ӵ����� XM_SetWindowFrameBuffer
// 3) ִ����ʾ����
// 4) ����NULL framebuffer�����Ӵ����� XM_SetWindowFrameBuffer
//	5)	�ر�framebuffer,���޸�ˢ�µ���ʾ�豸 XM_osd_framebuffer_close
//		
//		��ϸ��ο� alert_view��CountDownPaintʵ��
//
xm_osd_framebuffer_t XM_GetWindowFrameBuffer (HANDLE hWnd);

// �����Ӵ�������framebuffer����
//		
//		��ϸ��ο� alert_view��CountDownPaintʵ��
//
XMBOOL XM_SetWindowFrameBuffer (HANDLE hWnd, xm_osd_framebuffer_t framebuffer);

// ��ȡ�Ӵ���ȫ��Alpha����
unsigned char XM_GetWindowAlpha (HANDLE hWnd);

// �����Ӵ���ȫ��Alpha����
XMBOOL XM_SetWindowAlpha (HANDLE hWnd, unsigned char alpha);

// ������ͼ�л�ʱAnimatingЧ������
void XM_SetViewSwitchAnimatingDirection (UINT AnimatingDirection);

// ALERTVIEW��ͼ�İ����ص���������
// uKeyPressed ��ʾ��ǰ���µ���������ֵ
typedef VOID (*FPALERTCB) (UINT uKeyPressed);

#define	XM_COMMAND_OK			0
#define	XM_COMMAND_CANCEL		1
typedef VOID (*FPOKCANCELCB) (UINT Command);

#define	XM_OKCANCEL_OPTION_ENABLE_POPTOPVIEW		0x00000001		// ����ջ���Ӵ�

// ALERTVIEW��ͼ�Ŀ���ѡ��
#define	XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN		0x00000001		// "����ʱ��ʾ"ʹ��
#define	XM_ALERTVIEW_OPTION_ENABLE_KEYDISABLE		0x00000002		// "��ֹ��������"ʹ��
#define	XM_ALERTVIEW_OPTION_ENABLE_CALLBACK			0x00000004		// "�����ص�����"ʹ��
																						//		��ť����ʱ�����ð����ص�����
																						//		��ť��������Ϊ0���0		
#define	XM_ALERTVIEW_OPTION_ADJUST_COUNTDOWN		0x00000008		// ʹ�ܵ���ʱ��������


// �Ӵ���ʾ���뷽ʽѡ���
#define	XM_VIEW_ALIGN_CENTRE			0x00000001			// �Ӵ����ж���(�����ʾ����)
#define	XM_VIEW_ALIGN_BOTTOM			0x00000002			// �Ӵ��ײ����ж���(�����ʾ����)

// ��ʾ��Ϣ��ͼ
// 0 ��ͼ����������ͼ��ʾʧ��
// 1 ��ͼ��������ʾ
XMBOOL XM_OpenAlertView ( 
								 DWORD dwInfoTextID,				// ��Ϣ�ı���ԴID
																		//		��0ֵ��ָ����ʾ������Ϣ����ԴID
								 DWORD dwImageID,					// ͼƬ��Ϣ��ԴID
																		//		��0ֵ��ָ����ʾͼƬ��Ϣ����ԴID	
								 DWORD dwButtonCount,			// ��ť����
																		//		������һ����ťʱ��
																		//		��һ����ť��ʹ��VK_F1(Menu)
																		//		�ڶ���������ʹ��VK_F2(Mode)
																		//		������������ʹ��VK_F3(Switch)
								 DWORD dwButtonNormalTextID[],	//	��ť������ԴID
																		//		0 
																		//			��ʾû�ж���Button��Alert��Ϊһ����Ϣ��ʾ���ڡ�
																		//			���������ʱ���Զ��ر�
																		//		����ֵ
																		//			��ʾButton������Ϣ����ԴID��Alert��Ҫ��ʾһ����ť
								 DWORD dwButtonPressedTextID[],	//	��ť����������ԴID
																		//		0
																		//			��ʾû�ж������Դ��
																		//			��ť����ʱʹ��dwButtonNormalTextID��Դ
																		//		����ֵ
																		//			��ʾButton����ʱ������Ϣ����ԴID
								 DWORD dwBackgroundColor,		// ����ɫ
																		//		0
																		//			��ʾʹ��ȱʡ����ɫ
																		//		����ֵ
																		//			��ʾ��Ч��䱳��ɫ������ɫ��ָ��Alpha����
								 float fAutoCloseTime,			//	ָ���Զ��ر�ʱ�� (�뵥λ)��
																		//		0.0	��ʾ��ֹ�Զ��ر�
								 float fViewAlpha,				// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
																		//		0.0	��ʾȫ͸
																		//		1.0	��ʾȫ����
								 
								 FPALERTCB alertcb,				// �����ص�����

								 DWORD dwAlignOption,			//	��ͼ����ʾ����������Ϣ
																		//		���OSD��ʾ��ԭ��(OSD��Ч��������Ͻ�)
																		//		XM_VIEW_ALIGN_CENTRE	
																		//			�Ӵ����ж���(���OSD��ʾ����)
																		//		XM_VIEW_ALIGN_BOTTOM
																		//			�Ӵ��ײ����ж���(���OSD��ʾ����)
								 DWORD dwOption					// ALERTVIEW��ͼ�Ŀ���ѡ��
																		//		XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN
																		//			����ʱ��ʾʹ��
																		//		XM_ALERTVIEW_OPTION_ENABLE_KEYDISABLE
																		//			"��ֹ��������"ʹ��
								 );


// ���������������Ӵ�
XMBOOL XM_OpenBellSoundVolumeSettingView (VOID);
// ��MIC¼�����������Ӵ�
XMBOOL XM_OpenMicSoundVolumeSettingView (VOID);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif	// _XM_USER_H_
