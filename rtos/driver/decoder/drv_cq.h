#ifndef __DRV_CQ_H__
#define __DRV_CQ_H__

#ifdef __LINUX_SYSTEM__
//support linux kfifo kernel api. if use linux, define.

#include "pr1000.h"

#include <asm/types.h>
#include <linux/list.h>
#include <linux/slab.h> 
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/kfifo.h>
#include <linux/version.h>

typedef struct kfifo FIFO;

/* fifo of pointers */
static inline int PR1000_CQ_new(FIFO *fifo, int size)
{
	return kfifo_alloc(fifo, size * sizeof(void *), GFP_KERNEL);
}

static inline void PR1000_CQ_delete(FIFO *fifo)
{
	kfifo_free(fifo);
}

static inline void PR1000_CQ_reset(FIFO *fifo)
{
	kfifo_reset(fifo);
}

static inline void PR1000_CQ_remove(FIFO *fifo)
{
	unsigned int sz;
	void *p;

	while(!kfifo_is_empty(fifo))
	{
		sz = kfifo_out(fifo, (void *)&p, sizeof(p));

		if (sz == sizeof(p))
		{
			FREE(p);
		}
	}
}

static inline void PR1000_CQ_free_item(void *pItem)
{
	if(pItem)
	{
		FREE(pItem);
	}
}

static inline unsigned int PR1000_CQ_howmany(FIFO *fifo, SPINLOCK_T *lock)
{
	unsigned int ret = 0;
	unsigned long flags;

	if(kfifo_is_empty(fifo)) 
	{
		ret = 0;
	}
	else
	{
		spin_lock_irqsave(lock, flags);

		ret = kfifo_len(fifo) / sizeof(void *);

		spin_unlock_irqrestore(lock, flags);
	}
	return(ret);
}

static inline int PR1000_CQ_is_empty(FIFO *fifo)
{
	return(kfifo_is_empty(fifo));
}

static inline int PR1000_CQ_is_full(FIFO *fifo)
{
	return(kfifo_is_full(fifo));
}

static inline int PR1000_CQ_put(FIFO *fifo, void *p)
{
	return kfifo_in(fifo, (void *)&p, sizeof(p));
}

static inline void *PR1000_CQ_get(FIFO *fifo)
{
	unsigned int sz;
	void *p;

	if(kfifo_is_empty(fifo)) return(NULL);

	sz = kfifo_out(fifo, (void *)&p, sizeof(p));
	if (sz != sizeof(p))
		return NULL;

	return p;
}

static inline void *PR1000_CQ_peek(FIFO *fifo)
{
	unsigned int sz;
	void *p;

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
	sz = kfifo_out_peek(fifo, (void *)&p, sizeof(p));
	#else
	sz = kfifo_out_peek(fifo, (void *)&p, sizeof(p), 0);
	#endif
	if (sz != sizeof(p))
		return NULL;

	return p;
}

static inline int PR1000_CQ_put_locked(FIFO *fifo, void *p, SPINLOCK_T *lock)
{
	#if 0
	unsigned long flags;
	unsigned int ret;

	spin_lock_irqsave(lock, flags);

	ret = kfifo_in(fifo, (void *)&p, sizeof(p));

	spin_unlock_irqrestore(lock, flags);

	return(ret);
	#endif
    return(kfifo_in_locked(fifo, (void *)&p, sizeof(p), lock));
}

static inline void *PR1000_CQ_get_locked(FIFO *fifo, SPINLOCK_T *lock)
{
	unsigned int sz;
	void *p;
	#if 0
	unsigned long flags;

	if(kfifo_is_empty(fifo)) return(NULL);

	spin_lock_irqsave(lock, flags);

	sz = kfifo_out(fifo, (void *)&p, sizeof(p));
	if (sz != sizeof(p))
	{
		spin_unlock_irqrestore(lock, flags);
		return NULL;
	}

	spin_unlock_irqrestore(lock, flags);

	return p;
	#endif
	if(kfifo_is_empty(fifo)) return(NULL);

    sz = kfifo_out_locked(fifo, (void *)&p, sizeof(p), lock);
    if (sz != sizeof(p))
    {   
        return NULL;
    }   

    return p;


}

#else //#ifdef __LINUX_SYSTEM__

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#include "sglib.h"
/* 

  This is SGLIB version 1.0.3 ***< [20160418:OkHyun] Modified **

  (C) by Marian Vittek, Bratislava, http://www.xref-tech.com/sglib, 2003-5

  License Conditions: You can use a verbatim copy (including this
  copyright notice) of sglib freely in any project, commercial or not.
  You can also use derivative forms freely under terms of Open Source
  Software license or under terms of GNU Public License.  If you need
  to use a derivative form in a commercial project, or you need sglib
  under any other license conditions, contact the author.



*/


#ifndef _SGLIB__h_
#define _SGLIB__h_

/* the assert is used exclusively to write unexpected error messages */
//#include <assert.h>

/* -------------------------------- queue (in an array) ------------------ */
/* queue is a quadruple (a,i,j,dim) such that:                             */
/*  a is the array storing values                                          */
/*  i is the index of the first used element in the array                  */
/*  j is the index of the first free element in the array                  */
/*  dim is the size of the array a                                         */
/* !!!!!!! This data structure is NOT documented, do not use it !!!!!!!!!! */

#define SGLIB_QUEUE_INIT(type, a, i, j) { i = j = 0; }
#define SGLIB_QUEUE_IS_EMPTY(type, a, i, j) ((i)==(j))
#define SGLIB_QUEUE_IS_FULL(type, a, i, j, dim) ((i)==((j)+1)%(dim))
#define SGLIB_QUEUE_FIRST_ELEMENT(type, a, i, j) (a[i])
#define SGLIB_QUEUE_ADD_NEXT(type, a, i, j, dim) {\
  if (SGLIB_QUEUE_IS_FULL(type, a, i, j, dim)) /*assert(0 && "the queue is full")*/;\
  (j) = ((j)+1) % (dim);\
}
#define SGLIB_QUEUE_ADD(type, a, elem, i, j, dim) {\
  a[j] = (elem);\
  SGLIB_QUEUE_ADD_NEXT(type, a, i, j, dim);\
}
#define SGLIB_QUEUE_DELETE_FIRST(type, a, i, j, dim) {\
  if (SGLIB_QUEUE_IS_EMPTY(type, a, i, j)) /*assert(0 && "the queue is empty")*/;\
  (i) = ((i)+1) % (dim);\
}
#define SGLIB_QUEUE_DELETE(type, a, i, j, dim) {\
  SGLIB_QUEUE_DELETE_FIRST(type, a, i, j, dim);\
}
/**< [20160418:OkHyun] add _LEN */
#define SGLIB_QUEUE_LEN(type, a, i, j, dim, result) {\
  (result) = 0;\
  if (i <= j) (result)=(j-i);\
  else (result)=((j+dim)-i);\
}

/* ----------------------------- array queue (level 1) ------------------- */
/* sglib's queue is stored in a fixed sized array                          */
/* queue_type MUST be a structure containing fields:                       */
/*  afield is the array storing elem_type                                  */
/*  ifield is the index of the first element in the queue                  */
/*  jfield is the index of the first free element after the queue          */
/*  dim is the size of the array afield                                    */
/* !!!!!!! This data structure is NOT documented, do not use it !!!!!!!!!! */


#define SGLIB_DEFINE_QUEUE_PROTOTYPES(queue_type, elem_type, afield, ifield, jfield, dim) \
 extern static void sglib_##queue_type##_init(queue_type *q); \
 extern static int sglib_##queue_type##_is_empty(queue_type *q); \
 extern static int sglib_##queue_type##_is_full(queue_type *q); \
 extern static elem_type sglib_##queue_type##_first_element(queue_type *q); \
 extern static elem_type *sglib_##queue_type##_first_element_ptr(queue_type *q); \
 extern static void sglib_##queue_type##_add_next(queue_type *q); \
 extern static void sglib_##queue_type##_add(queue_type *q, elem_type elem); \
 extern static void sglib_##queue_type##_delete_first(queue_type *q); \
 extern static void sglib_##queue_type##_delete(queue_type *q); \
 extern static int sglib_##queue_type##_len(queue_type *q);

#define SGLIB_DEFINE_QUEUE_FUNCTIONS(queue_type, elem_type, afield, ifield, jfield, dim) \
 static void sglib_##queue_type##_init(queue_type *q) {\
  SGLIB_QUEUE_INIT(elem_type, q->afield, q->ifield, q->jfield);\
 }\
 static int sglib_##queue_type##_is_empty(queue_type *q) {\
  return(SGLIB_QUEUE_IS_EMPTY(elem_type, q->afield, q->ifield, q->jfield));\
 }\
 static int sglib_##queue_type##_is_full(queue_type *q) {\
  return(SGLIB_QUEUE_IS_FULL(elem_type, q->afield, q->ifield, q->jfield, dim));  /**< [20160418:OkHyun] add dim */\
 }\
 static elem_type sglib_##queue_type##_first_element(queue_type *q) {\
  return(SGLIB_QUEUE_FIRST_ELEMENT(elem_type, q->afield, q->ifield, q->jfield));\
 }\
 static elem_type *sglib_##queue_type##_first_element_ptr(queue_type *q) {\
  return(& SGLIB_QUEUE_FIRST_ELEMENT(elem_type, q->afield, q->ifield, q->jfield));\
 }\
 static void sglib_##queue_type##_add_next(queue_type *q) {\
  SGLIB_QUEUE_ADD_NEXT(elem_type, q->afield, q->ifield, q->jfield, dim);\
 }\
 static void sglib_##queue_type##_add(queue_type *q, elem_type elem) {\
  SGLIB_QUEUE_ADD(elem_type, q->afield, elem, q->ifield, q->jfield, dim);\
 }\
 static void sglib_##queue_type##_delete_first(queue_type *q) {\
  SGLIB_QUEUE_DELETE_FIRST(elem_type, q->afield, q->ifield, q->jfield, dim);\
 }\
 static void sglib_##queue_type##_delete(queue_type *q) {\
  SGLIB_QUEUE_DELETE(elem_type, q->afield, q->ifield, q->jfield, dim);\
 }\
 static int sglib_##queue_type##_len(queue_type *q) {\
  int result; \
  SGLIB_QUEUE_LEN(elem_type, q->afield, q->ifield, q->jfield, dim, result);\
  return(result); \
 }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif /* _SGLIB__h_ */

#define MAX_PARAMS	64
typedef unsigned char _elemType;
typedef struct
{
	_elemType data[MAX_PARAMS];

	int tail, head;
}FIFO;
SGLIB_DEFINE_QUEUE_FUNCTIONS(FIFO, _elemType, data, tail, head, MAX_PARAMS)

/* fifo of pointers */
static int PR1000_CQ_new(FIFO *fifo, int size)
{
	sglib_FIFO_init(fifo);
	return(0);
}

static void PR1000_CQ_delete(FIFO *fifo)
{
	sglib_FIFO_init(fifo);
}

static void PR1000_CQ_reset(FIFO *fifo)
{
	sglib_FIFO_init(fifo);
}

static void PR1000_CQ_remove(FIFO *fifo)
{
	while(! sglib_FIFO_is_empty(fifo)) {
		sglib_FIFO_delete(fifo);
	}
}

static void PR1000_CQ_free_item(void *pItem)
{
	return;
}

static unsigned int PR1000_CQ_howmany(FIFO *fifo, SPINLOCK_T *lock)
{
	unsigned int ret = 0;

	ret = sglib_FIFO_len(fifo);
	return(ret);
}

static int PR1000_CQ_is_empty(FIFO *fifo)
{
	return(sglib_FIFO_is_empty(fifo));	
}

static int PR1000_CQ_is_full(FIFO *fifo)
{
	return(sglib_FIFO_is_full(fifo));	
}

static int PR1000_CQ_put(FIFO *fifo, void *p)
{
	sglib_FIFO_add(fifo, (unsigned char)*(unsigned char *)p);
	return(0);
}

static void *PR1000_CQ_get(FIFO *fifo)
{
	void *p;

	p = (void *)sglib_FIFO_first_element_ptr(fifo);
	sglib_FIFO_delete(fifo);

	return p;
}

static void *PR1000_CQ_peek(FIFO *fifo)
{
	void *p;

	p = (void *)sglib_FIFO_first_element_ptr(fifo);

	return p;
}

static int PR1000_CQ_put_locked(FIFO *fifo, void *p, SPINLOCK_T *lock)
{
	sglib_FIFO_add(fifo, (unsigned char)*(unsigned char *)p);
	return(0);
}

static void *PR1000_CQ_get_locked(FIFO *fifo, SPINLOCK_T *lock)
{
	void *p;

	p = (void *)sglib_FIFO_first_element_ptr(fifo);
	sglib_FIFO_delete(fifo);

	return p;
}

#endif // __LINUX_SYSTEM__

#endif /* __DRV_CQ_H__ */
