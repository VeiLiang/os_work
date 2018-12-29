//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: rtos.h
//	  constant£¬macro, data structure£¬function protocol definition of lowlevel rtos interface 
//
//	Revision history
//
//		2011.09.08	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_RTOS_H_
#define _XM_RTOS_H_

#if defined (__cplusplus)
	extern "C"{
#endif

#ifdef WIN32

#include <windows.h>
#include <winbase.h>

#ifndef   OS_I8
  #define OS_I8 signed char
#endif

#ifndef   OS_U8
  #define OS_U8 unsigned char
#endif

#ifndef   OS_I16
  #define OS_I16 signed short
#endif

#ifndef   OS_U16
  #define OS_U16 unsigned short
#endif

#ifndef   OS_I32
  #define OS_I32 long
#endif

#ifndef   OS_U32
  #define OS_U32 unsigned OS_I32
#endif

#ifndef   OS_INT
  #define OS_INT       int
#endif

#ifndef   OS_UINT
  #define OS_UINT      unsigned OS_INT
#endif

#ifndef   OS_TIME
  #define OS_TIME      int
#endif

#ifndef   OS_STAT
  #define OS_STAT      OS_U8
#endif

#ifndef   OS_PRIO
  #define OS_PRIO      OS_U8
#endif

#ifndef   OS_BOOL
  #define OS_BOOL      OS_U8
#endif

typedef	HANDLE				OS_TASK;	
#ifndef OS_STACKPTR
  #define OS_STACKPTR
#endif

#ifndef OS_ROM_DATA
  #define OS_ROM_DATA
#endif

#define CTPARA_TIMESLICE ,2
#define OS_CREATE_TASK_PARA_TS   ,OS_UINT TimeSlice

  #define OS_CREATETASK(pTask, Name, Hook, Priority, pStack) \
  OS_CreateTask (pTask,                                      \
                  Name,                                      \
                  Priority,                                  \
                  Hook,                                      \
                  (void OS_STACKPTR*)pStack,                 \
                  sizeof(pStack)                             \
                  CTPARA_TIMESLICE                           \
               )

void OS_CreateTask  ( OS_TASK * pTask,
                      OS_ROM_DATA const char* Name,
                      OS_U8 Priority,
                      void (*pRoutine)(void),
                      void OS_STACKPTR *pStack,
                      OS_UINT StackSize
                      OS_CREATE_TASK_PARA_TS
        );


// Suspends the calling task until a specified time.
// The calling task will be put into the TS_DELAY state until the time specified.
// The OS_DelayUntil() function delays until the value of the time-variable OS_Time has reached a certain value. It
// is very useful if you have to avoid accumulating delays.
void OS_DelayUntil (int t);		


#ifndef   OS_TIME
  #define OS_TIME      int
#endif

#undef OS_IncDI
#undef OS_DecRI
#define OS_IncDI()      
#define OS_DecRI()      


int OS_GetTime (void);

void OS_Delay (int ms);		// Suspends the calling task for a specified period of time

typedef	CRITICAL_SECTION	OS_RSEMA;

/*
typedef struct {
	CRITICAL_SECTION	rsema;
	int	count;
} OS_RSEMA;
*/

int OS_Use (OS_RSEMA* pRSema);

void OS_Unuse (OS_RSEMA* pRSema);
char OS_Request (OS_RSEMA* pRSema);
void OS_CREATERSEMA (OS_RSEMA* pRSema);
void OS_DeleteRSema (OS_RSEMA* pRSema);

typedef  HANDLE				OS_EVENT;

void OS_EVENT_Create (OS_EVENT* pEvent);
void OS_EVENT_Delete (OS_EVENT* pEvent);
void OS_EVENT_Set (OS_EVENT* pEvent);
void OS_EVENT_Reset (OS_EVENT* pEvent);
void OS_EVENT_Wait (OS_EVENT* pEvent);
char OS_EVENT_WaitTimed (OS_EVENT* pEvent, OS_TIME Timeout);	

typedef	HANDLE				OS_CSEMA;

void OS_CreateCSema (OS_CSEMA* pCSema, OS_UINT InitValue);

// Increments the counter of a semaphore.
void OS_SignalCSema (OS_CSEMA * pCSema);

// Decrements the counter of a semaphore.
void OS_WaitCSema (OS_CSEMA* pCSema);

void OS_DeleteCSema (OS_CSEMA* pCSema);

void* OS_malloc(unsigned int n);
void  OS_free  (void* pMemBlock);





#endif	// WIN32

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_RTOS_H_