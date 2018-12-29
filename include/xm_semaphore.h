//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_semaphore.h
//	  调试接口 
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

// 创建一个命名的计数信号量
// semaphoreName                 信号量字符串，不能为NULL或者空串。
//                               iOS下不支持创建无名的信号量
// semaphoreInitialCount         信号量初始值
void *XM_CreateSemaphore (const char *semaphoreName, int semaphoreInitialCount); 

// 关闭信号量
int   XM_CloseSemaphore (void *xm_semaphore);

// 删除信号量
int   XM_DeleteSemaphore (const char *semaphoreName);


// 等待信号量
int   XM_WaitSemaphore   (void *xm_semaphore);

// 触发信号量
int	XM_SignalSemaphore (void *xm_semaphore);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_SEMAPHORE_H_
