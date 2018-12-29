//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_heap_malloc.h
//	  全局堆分配函数
//
//	Revision history
//
//		2010.09.10	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_HEAP_MALLOC_H_
#define _XM_HEAP_MALLOC_H_

#if defined (__cplusplus)
	extern "C"{
#endif

#define	HEAP_MALLOC_DEBUG

#ifdef HEAP_MALLOC_DEBUG

// 从堆中分配内存. 失败返回NULL。
void *	XM_heap_debug_malloc	(int size, char *file, int line);

// 从堆中分配内存，并将分配的字节全部清0. 失败返回NULL
void *	XM_heap_debug_calloc	(int size, char *file, int line);

// 释放用户RAM到堆
void XM_heap_debug_free	(void *mem, char *file, int line);

#define	XM_heap_malloc(size)		XM_heap_debug_malloc(size,__FILE__,__LINE__)
#define	XM_heap_calloc(size)		XM_heap_debug_calloc(size,__FILE__,__LINE__)
#define	XM_heap_free(mem)			XM_heap_debug_free(mem,__FILE__,__LINE__)

#else

// 从堆中分配内存. 失败返回NULL。
void *	XM_heap_malloc	(int size);

// 从堆中分配内存，并将分配的字节全部清0. 失败返回NULL
void *	XM_heap_calloc	(int size);

// 释放用户RAM到堆
void		XM_heap_free	(void *mem);

#endif

extern void XM_heap_init (void);
extern void XM_heap_exit (void);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif	// _XM_HEAP_MALLOC_H_
