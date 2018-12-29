//****************************************************************************
//
//	Copyright (C) 2011 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_device.h
//	  �����豸�ӿ� (��������ʱ����MCI)
//
//	Revision history
//
//		2011.05.31	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_DEVICE_H_
#define _XM_DEVICE_H_

#include <xm_type.h>
#include <xm_dev.h>
#include "xm_config.h"

// ��������
XMBOOL XM_KeyDriverGetEvent (WORD *key, WORD *modifier);
void XM_KeyDriverOpen (void);
void XM_KeyDriverClose (void);
XMBOOL XM_KeyDriverPoll(void);

XMBOOL XM_TouchDriverGetEvent (DWORD *point, DWORD *type);
void XM_TouchDriverOpen (void);
void XM_TouchDriverClose (void);
XMBOOL XM_TouchDriverPoll(void);

// ��ʱ������
XMBOOL XM_TimerDriverDispatchEvent (void);
void XM_TimerDriverOpen (void);
void XM_TimerDriverClose (void);
XMBOOL XM_TimerDriverPoll (void);

// UART����
XMBOOL XM_UartDriverGetEvent (WORD *key, WORD *modifier);
void XM_UartDriverOpen (void);
void XM_UartDriverClose (void);
XMBOOL XM_UartDriverPoll(void);

// MCI����
void XM_MciDriverOpen (void);
void XM_MciDriverClose (void);
XMBOOL XM_MciDriverPoll (void);
XMBOOL XM_MciDriverDispatchEvent (void);


// USB������
void XM_UsbDriverOpen (void);
// USB�����ر�
void XM_UsbDriverClose (void);
// ��ȡ��ǰ��USB�¼�
unsigned char XM_UsbDriverGetEvent (void);
//=========================
// ��ѯ�Ƿ����δ����USB�¼�
XMBOOL XM_UsbDriverPoll(void);




void winuser_init (void);
void wingdi_init (void);
void winmem_init (void);


DWORD XM_MainLoop (VOID);

#endif	// #ifndef _GDI_DEVICE_H_

