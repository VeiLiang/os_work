//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xmwidget.h
//	  constant��macro & basic typedef definition of widget
//
//	Revision history
//
//		2010.09.10	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_WIDGET_H_
#define _XM_WIDGET_H_

#include <xm_type.h>

// ����ؼ��ܸ���
#define	XM_WIDGET_COUNT			16

// ����ؼ�WIDGET������
#define	XM_WIDGET_NULL				0x00			// 
#define	XM_WIDGET_BUTTON			0x01			// ��ť		
#define	XM_WIDGET_CHECKBOX		0x02			// ��ѡ��
#define	XM_WIDGET_RADIO			0x03			// Բ�ΰ�ť
#define	XM_WIDGET_SLIDER			0x04			// ���пؼ�
#define	XM_WIDGET_LIST				0x05			// �б�
#define	XM_WIDGET_SCROLLBAR		0x06			// ������
#define	XM_WIDGET_IMAGE			0x07			// ͼƬ�ؼ�
#define	XM_WIDGET_EDIT				0x08			// ���б༭��
#define	XM_WIDGET_STATIC			0x09			// ��̬�ı���ʾ��
#define	XM_WIDGET_TITLE			0x0A			// ��ͨ�Ӵ��������ؼ�(��ʾ���⡢ʱ�䡢��ص���Ϣ)
#define	XM_WIDGET_IME				0x0B			// ���뷨�ؼ�
#define	XM_WIDGET_INPUTBAR		0x0C			// ��������༭��(�ֵ�ʹ��)

// �ؼ��û����ݽṹ����

// �ӿؼ������⡱���û����ݽṹ
typedef struct tagXM_TITLEUSERDATA {
	WCHAR *lpTitle;	// ���⴮��ָ��
	CHAR	cbTitle;		// ���⴮���ַ�����
} XM_TITLEUSERDATA;

// �ӿؼ��������������û����ݽṹ
typedef struct tagXM_SLIDERUSERDATA {
	XMCOORD ticks;		// �̶�����
	XMCOORD cursor;		// �α�λ��
} XM_SLIDERUSERDATA;


// �ؼ���Ϣ�������ⲿ����
XMWDGTPROC_DECLARE(BUTTON)
XMWDGTPROC_DECLARE(CHECKBOX)
XMWDGTPROC_DECLARE(RADIO)
XMWDGTPROC_DECLARE(SLIDER)
XMWDGTPROC_DECLARE(LIST)
XMWDGTPROC_DECLARE(SCROLLBAR)
XMWDGTPROC_DECLARE(IMAGE)
XMWDGTPROC_DECLARE(EDIT)
XMWDGTPROC_DECLARE(STATIC)
XMWDGTPROC_DECLARE(TITLE)
XMWDGTPROC_DECLARE(IME)
XMWDGTPROC_DECLARE(INPUTBAR)

// ���ÿؼ�����Ϣ������
// bWidgetType �ؼ�WIDGET������
// hWidget  �ؼ��ľ��
VOID XM_WidgetProc (const XMWDGT *pWidget, BYTE bWidgetFlag, VOID *pUserData, XMMSG *msg);

// ����Button�ؼ����ı���ʹ�ÿؼ���ǰ״̬������ʾ�������Ҫ�ڿؼ���ʾǰ���ؼ�״̬��ȷ����
VOID XM_ButtonSetText (HANDLE hWidget, BYTE bWidgetFlag, WCHAR *lpText, BYTE cbText);

VOID XM_CheckBoxSetText (HANDLE hWidget, BYTE bWidgetFlag, WCHAR *lpText, BYTE cbText);

VOID XM_RadioSetText (HANDLE hWidget, BYTE bWidgetFlag, WCHAR *lpText, BYTE cbText);

VOID XM_SliderSetText (HANDLE hWidget, BYTE bWidgetFlag, WCHAR *lpText, BYTE cbText);

VOID XM_EditSetText (HANDLE hWidget, BYTE bWidgetFlag, WCHAR *lpText, BYTE cbText);

VOID XM_StaticSetText (HANDLE hWidget, BYTE bWidgetFlag, WCHAR *lpText, BYTE cbText);

VOID XM_TitleSetText (HANDLE hWidget, BYTE bWidgetFlag, WCHAR *lpText, BYTE cbText);

// �����Ӵ��пؼ���״̬
XMBOOL XM_SetWidgetFlag (BYTE bWidgetIndex, BYTE bWidgetFlag);

// ��ȡ�Ӵ��пؼ���״̬
BYTE XM_GetWidgetFlag (BYTE bWidgetIndex);

// �����Ӵ��о������뽹��Ŀؼ���bWidgetIndexΪ�Ӵ��еĿؼ�����
XMBOOL XM_SetFocus (BYTE bWidgetIndex);

// ��ȡ�Ӵ��о������뽹��Ŀؼ�����
BYTE XM_GetFocus (VOID);

// �����Ӵ��пؼ���ѡ��״̬
XMBOOL XM_SetSelect (BYTE bWidgetIndex, XMBOOL bSelect);

// �����Ӵ��пؼ���ʹ��״̬
XMBOOL XM_SetEnable (BYTE bWidgetIndex, XMBOOL bEnable);

// ���ػ���ʾ�ؼ�
XMBOOL XM_SetVisual (BYTE bWidgetIndex, XMBOOL bVisual);


#endif	// _XM_WIDGET_H_