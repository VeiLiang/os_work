//****************************************************************************
//
//	Copyright (C) 2010 Shenzhen Exceedspace Digital Technology Co.,LTD
//
//	Author	ZhuoYongHong
//
//	File name: xm_autotest.c
//	 auto tester interface
//
//	Revision history
//
//		2006.04.25	ZhuoYongHong begin
//		2009.08.01	ZhuoYongHong ������¼�֧��
//
//****************************************************************************
#ifndef _XM_AUTOTEST_H_
#define _XM_AUTOTEST_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// �Զ����Գ�ʼ��
void xm_autotest_init (void);

// �Զ���������
// autotest_script_file	�ű��ļ�
// ����ֵ
//	0		ִ�гɹ�
// < 0	ִ��ʧ��
int xm_autotest_run (const char *autotest_script_file);

// �Զ�������ֹ
// ����ֵ
//	0		ִ�гɹ�
// < 0	ִ��ʧ��
int xm_autotest_stop (void);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif		// _XM_AUTOTEST_H_