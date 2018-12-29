//****************************************************************************
//
//	Copyright (C) 2012~2016 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_power.h
//	  power device driver
//
//	Revision history
//
//		2012.10.02	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_POWER_H_
#define _XM_POWER_H_

// �ϵ�����
void xm_power_lock (void);

// �ϵ����,�ػ�
void xm_power_unlock (void);

void xm_powerkey_init (void);

// ��⿪�ػ�������״̬������0x8000��ʾ�������£�0��ʾ�����ͷ�
int xm_powerkey_test (void);

// ���ACC�Ƿ����ϵ�
//	����ֵ
//		1		Acc���ϵ�
//		0		Accδ�ϵ�
int xm_power_check_acc_on (void);

// ���ACC�Ƿ�ȫ��ѹ
// ����ֵ
//		1		Acc���簲ȫ
//		0		Acc���粻��ȫ
int xm_power_check_acc_safe_or_not (void);

// ���USB�Ƿ񹩵�
// ����ֵ
//		1		USB����
//		0		USBδ����
int xm_power_check_usb_on (void);
void xm_board_power_ctl(u8 state);

#endif	// _XM_POWER_H_