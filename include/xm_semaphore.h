//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_semaphore.h
//	  ���Խӿ� 
//
//	Revision history
//
//		2010.09.08	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_SEMAPHORE_H_
#define	_XM_SEMAPHORE_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// ����һ�������ļ����ź���
// semaphoreName                 �ź����ַ���������ΪNULL���߿մ���
//                               iOS�²�֧�ִ����������ź���
// semaphoreInitialCount         �ź�����ʼֵ
void *XM_CreateSemaphore (const char *semaphoreName, int semaphoreInitialCount); 

// �ر��ź���
int   XM_CloseSemaphore (void *xm_semaphore);

// ɾ���ź���
int   XM_DeleteSemaphore (const char *semaphoreName);


// �ȴ��ź���
int   XM_WaitSemaphore   (void *xm_semaphore);

// �����ź���
int	XM_SignalSemaphore (void *xm_semaphore);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_SEMAPHORE_H_
