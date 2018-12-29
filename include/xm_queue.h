//****************************************************************************
//
// COPYRIGHT (c) 2004-2005 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_queue.h
//	  definition of kernel queue toolbox
//
//	Revision history
//
//		2004.10.08	ZhuoYongHong add kerenl queue toolbox
//
//****************************************************************************
#ifndef _XM_QUEUE_TOOLBOX_H_
#define _XM_QUEUE_TOOLBOX_H_

#if defined (__cplusplus)
	extern "C"{
#endif

typedef struct _queue_s {
	struct _queue_s			*prev;
	struct _queue_s			*next;
} queue_s;

/* queue toolbox procedure */
void			queue_initialize		(queue_s *queue);
void			queue_insert			(queue_s *entry, queue_s *queue);
void			queue_delete			(queue_s *entry);
queue_s *	queue_delete_next		(queue_s *queue);
int			queue_empty				(queue_s *queue);
queue_s *	queue_head				(queue_s *queue);
queue_s *	queue_tail				(queue_s *queue);
queue_s *	queue_next				(queue_s *queue);
queue_s *	queue_prev				(queue_s *queue);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	//_QUEUE_TOOLBOX_H_