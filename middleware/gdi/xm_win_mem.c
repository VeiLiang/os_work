//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_win_mem.c
//	  constant，macro & basic typedef definition of user memory allocation & free function
//
//	Revision history
//
//		2010.09.15	ZhuoYongHong Initial version
//
//****************************************************************************
#include <xm_proj_define.h>
#include <xm_type.h>
#include <xm_malloc.h>
#include <xm_assert.h>
#include <string.h>
#include <xm_semaphore.h>
#include "xm_printf.h"

#define	HEAP_DEBUG

#if defined(PCVER) || defined(WIN32)
#define	HEAP_DEBUG
#endif

#if defined(HEAP_DEBUG)

#define	MAGIC_SIZE		4					// sizeof(int)
#define	MAGIC				0x68656170		/* "heap" */

#else

#define	MAGIC_SIZE		0
#define	MAGIC				0x68656170

#endif

//#define	PTRSIZE			((BYTE) sizeof(CHAR *))
#define	PTRSIZE			(32)	// 按照cortex a5的cache line size (32)分配
#define	Align(x,a)		(((x) + (a - 1)) & ~(a - 1))
#define	NextSlot(p)		(* (CHAR **) ((p) - PTRSIZE))
#define	NextFree(p)		(* (CHAR **) (p))

// 堆控制变量定义
#pragma data_alignment=512
__no_init static CHAR	_heap_mem[XM_USER_MEM_SIZE];
static CHAR *	_heap_bottom;
static CHAR *	_heap_top;
static CHAR *	_heap_empty;

static void *  _heap_semaphore;

void winmem_init (void)
{
	CHAR *addr;
	CHAR *p, *np;

	_heap_semaphore = XM_CreateSemaphore ("_heap_semaphore", 1);
#ifdef WIN32
	XM_ASSERT(_heap_semaphore);
#endif

	addr = _heap_mem;
	p = addr + PTRSIZE;
	np = p + XM_USER_MEM_SIZE - 2 * PTRSIZE;

	NextFree(p)		= np;
	NextSlot(p)		= np;
	NextFree(np)	= 0;
	NextSlot(np)	= 0;

	_heap_empty		= p;
	_heap_bottom	= p;
	_heap_top		= addr + XM_USER_MEM_SIZE;
}

void winmem_exit (void)
{
	if(XM_WaitSemaphore (_heap_semaphore))
	{
		// 关闭信号灯
		XM_CloseSemaphore (_heap_semaphore);
		// 删除信号灯
		XM_DeleteSemaphore (_heap_semaphore);
		_heap_semaphore = NULL;
	}
}

// 保存winmem系统的运行环境
void winmem_save_environment (unsigned char *buff, unsigned int size)
{
	unsigned char *start = buff;
	memcpy (buff, 	_heap_mem, sizeof(_heap_mem));
	buff += sizeof(_heap_mem);
	memcpy (buff,  &_heap_bottom, sizeof(_heap_bottom));
	buff += sizeof(_heap_bottom);
	memcpy (buff, &_heap_top, sizeof(_heap_top));
	buff += sizeof(_heap_top);
	memcpy (buff, &_heap_empty, sizeof(_heap_empty));
	buff += sizeof(_heap_empty);

	size -= buff - start;
}

// 恢复winmem系统的运行环境
void winmem_restore_environment (unsigned char *buff, unsigned int size)
{
	memcpy (_heap_mem, buff, 	sizeof(_heap_mem));
	buff += sizeof(_heap_mem);
	memcpy (&_heap_bottom, buff,  sizeof(_heap_bottom));
	buff += sizeof(_heap_bottom);
	memcpy (&_heap_top, buff, sizeof(_heap_top));
	buff += sizeof(_heap_top);
	memcpy (&_heap_empty, buff, sizeof(_heap_empty));
	buff += sizeof(_heap_empty);
}



static void _memset (void *addr, int c, int size)
{
	char *dst = addr;
	while(size > 0)
	{
		*dst = (char)c;
		dst ++;
		size --;
	}
}

// 分配用户RAM. 返回一标识符。(-1)
void *	XM_malloc	(int size)
{
	CHAR *prev, *p, *next, *np;
	UINT len;

	if (size == 0) 
		return NULL;

	size += MAGIC_SIZE;		// add debug infrmation

	len = (UINT)(Align(size, PTRSIZE) + PTRSIZE);
	if (len < 2 * PTRSIZE)
	{
		XM_printf ("XM_malloc failed, len=%d\n", len);
		return NULL;
	}

	XM_WaitSemaphore (_heap_semaphore);

#if defined(HEAP_DEBUG)
	for (p = _heap_bottom; (next = NextSlot(p)) != 0; p = next)
	{
		XM_ASSERT (next > p);
	}
	XM_ASSERT (p == _heap_top - PTRSIZE);
#endif

	for (prev = 0, p = _heap_empty; p != 0; prev = p, p = NextFree(p))
	{
		XM_ASSERT (p != prev);
		next = NextSlot(p);
		np = p + len;	/* easily overflows!! */
		if (np > next || np <= p)
			continue;		/* too small */
		
		if ((np + PTRSIZE + MAGIC_SIZE) < next)
		{
			/* too big, so split */
			/* + PTRSIZE avoids tiny slots on free list */
			NextSlot(np)	= next;
			NextSlot(p)	= np;
			NextFree(np)	= NextFree(p);
			NextFree(p)	= np;
		}

		if (prev)
			NextFree(prev) = NextFree(p);
		else
			_heap_empty   = NextFree(p);

#if defined (HEAP_DEBUG)
		*((long *)p) = MAGIC;
#endif
		
		XM_SignalSemaphore (_heap_semaphore);
		return (void *)(p + MAGIC_SIZE);
	}
	
	//XM_printf ("XM_malloc failed, size=%d, len=%d\n", size, len);

	XM_SignalSemaphore (_heap_semaphore);
	
	return NULL;
}

// 分配用户RAM并将分配的字节全部清0. 失败返回NULL
void *	XM_calloc	(int size)
{
    
	void *mem = XM_malloc (size);

	if(mem)
	{
		_memset (mem, 0, size);
	}
	return mem;
}

// 释放用户RAM
void		XM_free		(void *mem)
{
	CHAR *prev, *next;
	CHAR *p = (CHAR *)mem;

	if (p == NULL) 
		return;
      
	XM_WaitSemaphore (_heap_semaphore);

#if defined(HEAP_DEBUG)
	// check to see pointer valid or not
	XM_ASSERT (*((long *)(p - MAGIC_SIZE)) == MAGIC);
	*((long *)(p - MAGIC_SIZE))  = 0x00000000;		// 标记为无效句柄
#endif

	p -= MAGIC_SIZE;		// add debug infrmation

	XM_ASSERT(NextSlot(p) > (CHAR *)p);

	for (prev = 0, next = _heap_empty; next != 0; prev = next, next = NextFree(next))
	{
		XM_ASSERT (next != prev);
		if (p < next)
			break;
	}
	NextFree(p) = next;
	if (prev)
		NextFree(prev) = p;
	else
		_heap_empty = p;

	if (next && next != (_heap_top - PTRSIZE))
	{
		XM_ASSERT(NextSlot(p) <= (CHAR *)next);
		if (NextSlot(p) == next)
		{		
			/* merge p and next */
			NextSlot(p) = NextSlot(next);
			NextFree(p) = NextFree(next);
		}
	}
	if (prev)
	{
		XM_ASSERT(NextSlot(prev) <= (CHAR *)p);
		if (NextSlot(prev) == p)
		{	
			/* merge prev and p */
			NextSlot(prev) = NextSlot(p);
			NextFree(prev) = NextFree(p);
		}
	}

	XM_SignalSemaphore (_heap_semaphore);
}

