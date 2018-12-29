//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xmmalloc.h
//	  constant��macro & basic typedef definition of user memory allocation & free function
//
//	Revision history
//
//		2010.09.10	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_MALLOC_H_
#define _XM_MALLOC_H_

#include <xm_type.h>

#define	XM_USER_MEM_SIZE		0x500000		// 32K�ֽ��û���̬�ڴ�ռ�

// �û�RAM����ʱ��󶨵�ǰ�����Ӵ���
// ���Ӵ��˳�ʱ��ϵͳ�Զ�������Ӵ��������û�RAMȫ���ͷ�, ��ʹ�û���û���ͷš�


// �����û�RAM. ����һ��ʶ����(-1)
void *	XM_malloc	(int size);

// �����û�RAM����������ֽ�ȫ����0. ʧ�ܷ���NULL
void *	XM_calloc	(int size);

// �ͷ��û�RAM
void XM_free(void *mem);



#endif	// _XM_MALLOC_H_
