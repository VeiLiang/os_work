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

typedef	DWORD				OS_TASK;	
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

  #define OS_CREATETASK_EX(pTask, Name, Hook, Priority, pStack, pContext) \
  OS_CreateTaskEx  (pTask,                                                \
                    Name,                                                 \
                    Priority,                                             \
                    Hook,                                                 \
                    (void OS_STACKPTR*)pStack,                            \
                    sizeof(pStack)                                        \
                    CTPARA_TIMESLICE,                                     \
                    pContext                                              \
               )

void OS_CreateTask  ( OS_TASK * pTask,
                      OS_ROM_DATA const char* Name,
                      OS_U8 Priority,
                      void (*pRoutine)(void),
                      void OS_STACKPTR *pStack,
                      OS_UINT StackSize
                      OS_CREATE_TASK_PARA_TS
        );

#define OS_CREATE_TASK_PARA_NAME      OS_ROM_DATA const char* Name,

void OS_CreateTaskEx  ( OS_TASK * pTask,
                        OS_CREATE_TASK_PARA_NAME
                        OS_U8 Priority,
                        void (*pRoutine)(void *),
                        void OS_STACKPTR *pStack,
                        OS_UINT StackSize
                        OS_CREATE_TASK_PARA_TS,
                        void * pContext
        );
        

// Ends (terminates) a task.
void OS_Terminate (OS_TASK* pTask);

// Suspends the calling task until a specified time.
// The calling task will be put into the TS_DELAY state until the time specified.
// The OS_DelayUntil() function delays until the value of the time-variable OS_Time has reached a certain value. It
// is very useful if you have to avoid accumulating delays.
void OS_DelayUntil (int t);		


// Waits for the specified events for a given time, and clears the event memory after an event occurs.
char OS_WaitEventTimed (char EventMask, OS_TIME TimeOut);

// Waits for one of the events specified in the bitmask and clears the event memory after an event occurs.
// If none of the specified events are signaled, the task is suspended. The first of the specified events will wake the task.
// These events are signaled by another task, a S/W timer or an interrupt handler. Any bit in the 8-bit event mask may
// enable the corresponding event.
char OS_WaitEvent (char EventMask);

// Waits for one of the events specified by the bitmask and clears only that event after it occurs.
//		Return value		All masked events that have actually occurred.
// If none of the specified events are signaled, the task is suspended. The first of the specified events will wake the task.
// These events are signaled by another task, a S/W timer, or an interrupt handler. Any bit in the 8-bit event mask may
// enable the corresponding event. All unmasked events remain unchanged.
char OS_WaitSingleEvent (char EventMask);

// Signals event(s) to a specified task.
// If the specified task is waiting for one of these events, it will be put in the READY state and activated according to the rules of the scheduler.
void OS_SignalEvent (char Event, OS_TASK* pTask);

// Returns a list of events that have occurred for a specified task.
// The event mask of the events that have actually occurred.
// By calling this function, the actual events remain signaled. The event memory is not cleared. This is one way for a task
// to find out which events have been signaled. The task is not suspended if no events are available.
char OS_GetEventsOccurred (OS_TASK* pTask);

// Returns the actual state of events and then clears the events of a specified task.
// pTask					The task who's event mask is to be returned,
//							NULL means current task.
//
// Return value		The events that were actually signaled before clearing.
char OS_ClearEvents (OS_TASK* pTask);

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

typedef  __int64			OS_MAILBOX;

// Parameter Description
//		pMB         Pointer to a data structure of type OS_MAILBOX reserved for managing the mailbox.
//		sizeofMsg   Size of a message in bytes. (1 <= sizeofMsg <= 127)
//		maxnoMsg    Maximum number of messages. (1 <= MaxnofMsg <= 32767)
//		pMsg        Pointer to a memory area used as buffer. The buffer has to be big enough to hold the given
//                number of messages of the specified size: sizeofMsg * maxnoMsg bytes.
void OS_CREATEMB (OS_MAILBOX* pMB, unsigned char sizeofMsg, unsigned int maxnofMsg, void* pMsg);

// Deletes a specified mailbox.
// Additional Information
// To keep the system fully dynamic, it is essential that mailboxes can be created dynamically. This also means there has
// to be a way to delete a mailbox when it is no longer needed. The memory that has been used by the mailbox for the
// control structure and the buffer can then be reused or reallocated.
// It is the programmer's responsibility to:
// ¡ñ make sure that the program no longer uses the mailbox to be deleted
// ¡ñ make sure that the mailbox to be deleted actually exists (for example, has been created first).
void OS_DeleteMB (OS_MAILBOX* pMB);

// Clears all messages in a specified mailbox.
void OS_ClearMB (OS_MAILBOX* pMB);

// Stores a new message of a predefined size in a mailbox.
// If the mailbox is full, the calling task is suspended.
void OS_PutMail (OS_MAILBOX* pMB,void* pMail);

void OS_PutMail1 (OS_MAILBOX* pMB, const char* pMail);

// Retrieves a new message of a predefined size from a mailbox, if a message is available within a given time.
// 1 <= Timeout <= 231-1 = 0x7FFFFFFF for 32-bit CPUs
//   0: Success; message retrieved.
//   1: Message could not be retrieved (mailbox is empty); destination remains unchanged.
char OS_GetMailTimed (OS_MAILBOX* pMB, void* pDest, OS_TIME Timeout);

// Retrieves a new message of a predefined size from a mailbox.
// If the mailbox is empty, the task is suspended until the mailbox receives a new message. Because this routine might
// require a suspension, it may not be called from an interrupt routine. Use OS_GetMailCond/OS_GetMailCond1
// instead if you have to retrieve data from a mailbox from within an ISR.
void OS_GetMail (OS_MAILBOX* pMB, void* pDest);
void OS_GetMail1 (OS_MAILBOX* pMB,char* pDest);
/**********************************************************************
*
*       OS_TIMER
*/
typedef struct OS_timer OS_TIMER;
struct OS_timer {
  void * prev;
  void * next;
  void (*Hook)(void);
  OS_TIME Time;
  OS_TIME Period;
  char    Active;
  int		id;
};

typedef void OS_TIMERROUTINE(void);

// Creates a software timer (but does not start it).
void OS_CreateTimer (OS_TIMER* pTimer, OS_TIMERROUTINE* Callback, OS_TIME Timeout);

// Starts a specified timer.
void OS_StartTimer (OS_TIMER* pTimer);

// Stops and deletes a specified timer.
void OS_DeleteTimer (OS_TIMER* pTimer);

// Restarts a specified timer with its initial time value.
void OS_RetriggerTimer (OS_TIMER* pTimer);


#define OS_CREATETIMER(pTimer,c,d)  \
        OS_CreateTimer(pTimer,c,d); \
        OS_StartTimer(pTimer);


void OS_EnterRegion(void);
void OS_LeaveRegion(void);


#endif	// WIN32

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_RTOS_H_