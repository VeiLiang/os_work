//****************************************************************************
//
//	Copyright (C) 2010~2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: win32_rtos_simulate.c
//	  Win32 系统内核基本服务接口模拟
//
//	Revision history
//
//		2011.08.18	ZhuoYongHong Initial version
//
//****************************************************************************#include <io.h>
#include <stdio.h>
#include <direct.h>
#include <afx.h>
#include <assert.h>
#include <winbase.h>
#include <xmprintf.h>

#include "rtos.h"

extern "C" {

void OS_EVENT_Create (OS_EVENT* pEvent)
{
	HANDLE handle = CreateEvent (NULL, 0, 0, NULL);
	if(handle == NULL)
	{
		XM_printf ("OS_EVENT_Create failed\n");
	}
	*pEvent = handle;
}

void OS_EVENT_Delete (OS_EVENT* pEvent)
{
	if(CloseHandle (*pEvent) == 0)
	{
		XM_printf ("OS_EVENT_Delete failed\n");
	}
}

void OS_EVENT_Set (OS_EVENT* pEvent)
{
	if(SetEvent (*pEvent) == 0)
	{
		XM_printf ("OS_EVENT_Set failed\n");
	}
}

void OS_EVENT_Reset (OS_EVENT* pEvent)
{
	if(ResetEvent (*pEvent) == 0)
	{
		XM_printf ("OS_EVENT_Reset failed\n");
	}
}

void OS_EVENT_Wait (OS_EVENT* pEvent)
{
	if(WaitForSingleObject (*pEvent, INFINITE) != WAIT_OBJECT_0)
	{
		XM_printf ("OS_EVENT_Wait failed\n");
	}
}

char OS_EVENT_WaitTimed (OS_EVENT* pEvent, OS_TIME Timeout)
{
	DWORD ret;
	ret = WaitForSingleObject (*pEvent, Timeout);
	if(ret == WAIT_OBJECT_0)
	{
		return 0;	// 0: Success, the event was signaled within the specified time.
	}
	else if(ret == WAIT_TIMEOUT)
	{
		return 1;	// 1: The event was not signaled and a timeout occurred.
	}
	else
	{
		XM_printf ("OS_EVENT_WaitTimed failed\n");
		return 1;
	}
}

int OS_Use (OS_RSEMA* pRSema)
{
	EnterCriticalSection (pRSema);
	return 1;
}

void OS_Unuse (OS_RSEMA* pRSema)
{
	LeaveCriticalSection (pRSema);
}

char OS_Request (OS_RSEMA* pRSema)
{
	if(TryEnterCriticalSection (pRSema))
		return 1;		// 1: Resource was available, now in use by calling task
	return 0;			// 0: Resource was not available.
}

void OS_CREATERSEMA (OS_RSEMA* pRSema)
{
	InitializeCriticalSection (pRSema);
}

void OS_DeleteRSema (OS_RSEMA* pRSema)
{
	DeleteCriticalSection (pRSema);
}

void OS_CreateCSema (OS_CSEMA* pCSema, OS_UINT InitValue)
{
	HANDLE handle = CreateSemaphore (NULL, InitValue, 65535, NULL);
	if(handle == 0)
	{
		XM_printf ("OS_CreateCSema failed\n");
		*pCSema = 0;
	}
	else
		*pCSema = handle;
}

// Increments the counter of a semaphore.
void OS_SignalCSema (OS_CSEMA * pCSema)
{
	if(ReleaseSemaphore (*pCSema, 1, NULL) == 0)
	{
		XM_printf ("OS_SignalCSema failed\n");
	}
}

// Decrements the counter of a semaphore.
void OS_WaitCSema (OS_CSEMA* pCSema)
{
	DWORD ret = WaitForSingleObject (*pCSema, INFINITE);
	if(ret != WAIT_OBJECT_0)
	{
		XM_printf ("OS_SignalCSema failed\n");
	}
}

void OS_DeleteCSema (OS_CSEMA* pCSema)
{
	if(CloseHandle(*pCSema))
	{
		XM_printf ("OS_DeleteCSema failed\n");
	}
}



int OS_GetTime (void)
{
	return GetTickCount();
}

void OS_Delay (int ms)		// Suspends the calling task for a specified period of time
{
	Sleep (ms);
}

void OS_CreateTask  ( OS_TASK * pTask,
                      OS_ROM_DATA const char* Name,
                      OS_U8 Priority,
                      void (*pRoutine)(void),
                      void OS_STACKPTR *pStack,
                      OS_UINT StackSize
                      OS_CREATE_TASK_PARA_TS
        )
{
	HANDLE thread = CreateThread (NULL, StackSize, (unsigned long (__stdcall *)(void *))pRoutine, 0, 0, NULL);
	*pTask = thread;
}


void OS_DelayUntil (int t)
{
	DWORD now = GetTickCount();
	if(t < 0)
		return;

	while(now < (DWORD)t)
	{
		Sleep (1);
		now = GetTickCount();
	}
}

}