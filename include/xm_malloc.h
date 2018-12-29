//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xmmalloc.h
//	  constant，macro & basic typedef definition of user memory allocation & free function
//
//	Revision history
//
//		2010.09.10	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_MALLOC_H_
#define _XM_MALLOC_H_

#include <xm_type.h>

#define	XM_USER_MEM_SIZE		0x500000		// 32K字节用户动态内存空间

// 用户RAM分配时会绑定当前的主视窗。
// 主视窗退出时，系统自动将与该视窗关联的用户RAM全部释放, 即使用户并没有释放。


// 分配用户RAM. 返回一标识符。(-1)
void *	XM_malloc	(int size);

// 分配用户RAM并将分配的字节全部清0. 失败返回NULL
void *	XM_calloc	(int size);

// 释放用户RAM
void XM_free(void *mem);



#endif	// _XM_MALLOC_H_
