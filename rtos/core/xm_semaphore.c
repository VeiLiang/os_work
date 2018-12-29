#include <stdlib.h>
#include <string.h>
#include "xm_semaphore.h"

#ifdef _WIN32	// Windows

// Windows实现

#include <Windows.h>

// 创建一个命名的计数信号量
// semaphoreName                 信号量字符串，不能为NULL或者空串。
//                               iOS下不支持创建无名的信号量
// semaphoreInitialCount         信号量初始值
void *XM_CreateSemaphore (const char *semaphoreName, int semaphoreInitialCount)
{
	HANDLE hSemaphore;
	if(semaphoreName == NULL || *semaphoreName == '\0')
		return NULL;
	
	hSemaphore = CreateSemaphore (NULL, semaphoreInitialCount,  0xFFFF, semaphoreName);
	return (void *)hSemaphore;
}

// 关闭信号量
int	XM_CloseSemaphore (void *xm_semaphore)
{
	if(xm_semaphore == NULL)
		return 0;
	return (int)CloseHandle (xm_semaphore);
}

// 删除信号量
int   XM_DeleteSemaphore (const char *semaphoreName)
{
	// Windows不需要此操作
	return 1;
}


// 等待信号量
int   XM_WaitSemaphore   (void *xm_semaphore)
{
	DWORD ret;
	if(xm_semaphore == NULL)
		return 0;

	ret = WaitForSingleObject ((HANDLE)xm_semaphore, INFINITE);
	if(ret == WAIT_OBJECT_0)
		return 1;
	return 0;
}

// 触发信号量
int	XM_SignalSemaphore (void *xm_semaphore)
{
	if(xm_semaphore == NULL)
		return 0;
	return (int)ReleaseSemaphore ((HANDLE)xm_semaphore, 1, NULL);
}

#elif defined(__ICCARM__)

#include "rtos.h"
#include <string.h>
#include <stdlib.h>
#include "xm_printf.h"
#include "xm_core.h"

void *XM_CreateSemaphore (const char *semaphoreName, int semaphoreInitialCount)
{
	OS_CSEMA *csema = (OS_CSEMA *)kernel_malloc (sizeof(OS_CSEMA));
	if(csema)
	{
		OS_CreateCSema (csema, semaphoreInitialCount);
	}
	else
	{
		XM_printf ("XM_CreateSemaphore(%s) failed\n", semaphoreName);
	}
	return csema;
}

// 关闭信号量
int   XM_CloseSemaphore (void *xm_semaphore)
{
	if(xm_semaphore == NULL)
		return 0;
	OS_DeleteCSema (xm_semaphore);
	kernel_free (xm_semaphore);
	return 1;
}

// 删除信号量
int	XM_DeleteSemaphore (const char *semaphoreName)
{
	// IAR PowerPac不需要此操作
	return 1;
}

// 等待信号量
int   XM_WaitSemaphore   (void *xm_semaphore)
{
	if(xm_semaphore == NULL)
		return 0;

	OS_WaitCSema (xm_semaphore);
	return 1;
}

// 触发信号量
int	XM_SignalSemaphore (void *xm_semaphore)
{
	if(xm_semaphore == NULL)
		return 0;
	OS_SignalCSema (xm_semaphore);
	return 1;
}

#else

// POSIX实现

#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <semaphore.h>
#include "xmprintf.h"

void *XM_CreateSemaphore (const char *semaphoreName, int semaphoreInitialCount)
{
	// 有名信号量需要先删除
	sem_unlink (semaphoreName);
	sem_t * semaphore = sem_open (semaphoreName, O_CREAT, 0644, semaphoreInitialCount);
    XM_printf ("sem_open(%s)=%x\n", semaphoreName, semaphore);
	return (void *)semaphore;
}

// 关闭信号量
int   XM_CloseSemaphore (void *xm_semaphore)
{
	int ret = sem_close ( xm_semaphore );   
	if(ret == -1)
		return 0;
	return 1;
}

// 删除信号量
int	XM_DeleteSemaphore (const char *semaphoreName)
{
	int ret = sem_unlink (semaphoreName);
	if(ret == -1)
		return 0;
	else
		return 1;
}

// 等待信号量
int   XM_WaitSemaphore   (void *xm_semaphore)
{
	int ret = sem_wait( xm_semaphore ); 
	if(ret == -1)
		return 0;
	else
		return 1;
}

// 触发信号量
int	XM_SignalSemaphore (void *xm_semaphore)
{
	int ret = sem_post( xm_semaphore );  
	if(ret == -1)
		return 0;
	else
		return 1;
}

#endif	// _WIN32

