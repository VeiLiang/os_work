//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_heap_malloc.h
//	  ȫ�ֶѷ��亯��
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

// �Ӷ��з����ڴ�. ʧ�ܷ���NULL��
void *	XM_heap_debug_malloc	(int size, char *file, int line);

// �Ӷ��з����ڴ棬����������ֽ�ȫ����0. ʧ�ܷ���NULL
void *	XM_heap_debug_calloc	(int size, char *file, int line);

// �ͷ��û�RAM����
void XM_heap_debug_free	(void *mem, char *file, int line);

#define	XM_heap_malloc(size)		XM_heap_debug_malloc(size,__FILE__,__LINE__)
#define	XM_heap_calloc(size)		XM_heap_debug_calloc(size,__FILE__,__LINE__)
#define	XM_heap_free(mem)			XM_heap_debug_free(mem,__FILE__,__LINE__)

#else

// �Ӷ��з����ڴ�. ʧ�ܷ���NULL��
void *	XM_heap_malloc	(int size);

// �Ӷ��з����ڴ棬����������ֽ�ȫ����0. ʧ�ܷ���NULL
void *	XM_heap_calloc	(int size);

// �ͷ��û�RAM����
void		XM_heap_free	(void *mem);

#endif

extern void XM_heap_init (void);
extern void XM_heap_exit (void);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif	// _XM_HEAP_MALLOC_H_
