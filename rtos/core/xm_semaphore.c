#include <stdlib.h>
#include <string.h>
#include "xm_semaphore.h"

#ifdef _WIN32	// Windows

// Windowsʵ��

#include <Windows.h>

// ����һ�������ļ����ź���
// semaphoreName                 �ź����ַ���������ΪNULL���߿մ���
//                               iOS�²�֧�ִ����������ź���
// semaphoreInitialCount         �ź�����ʼֵ
void *XM_CreateSemaphore (const char *semaphoreName, int semaphoreInitialCount)
{
	HANDLE hSemaphore;
	if(semaphoreName == NULL || *semaphoreName == '\0')
		return NULL;
	
	hSemaphore = CreateSemaphore (NULL, semaphoreInitialCount,  0xFFFF, semaphoreName);
	return (void *)hSemaphore;
}

// �ر��ź���
int	XM_CloseSemaphore (void *xm_semaphore)
{
	if(xm_semaphore == NULL)
		return 0;
	return (int)CloseHandle (xm_semaphore);
}

// ɾ���ź���
int   XM_DeleteSemaphore (const char *semaphoreName)
{
	// Windows����Ҫ�˲���
	return 1;
}


// �ȴ��ź���
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

// �����ź���
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

// �ر��ź���
int   XM_CloseSemaphore (void *xm_semaphore)
{
	if(xm_semaphore == NULL)
		return 0;
	OS_DeleteCSema (xm_semaphore);
	kernel_free (xm_semaphore);
	return 1;
}

// ɾ���ź���
int	XM_DeleteSemaphore (const char *semaphoreName)
{
	// IAR PowerPac����Ҫ�˲���
	return 1;
}

// �ȴ��ź���
int   XM_WaitSemaphore   (void *xm_semaphore)
{
	if(xm_semaphore == NULL)
		return 0;

	OS_WaitCSema (xm_semaphore);
	return 1;
}

// �����ź���
int	XM_SignalSemaphore (void *xm_semaphore)
{
	if(xm_semaphore == NULL)
		return 0;
	OS_SignalCSema (xm_semaphore);
	return 1;
}

#else

// POSIXʵ��

#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <semaphore.h>
#include "xmprintf.h"

void *XM_CreateSemaphore (const char *semaphoreName, int semaphoreInitialCount)
{
	// �����ź�����Ҫ��ɾ��
	sem_unlink (semaphoreName);
	sem_t * semaphore = sem_open (semaphoreName, O_CREAT, 0644, semaphoreInitialCount);
    XM_printf ("sem_open(%s)=%x\n", semaphoreName, semaphore);
	return (void *)semaphore;
}

// �ر��ź���
int   XM_CloseSemaphore (void *xm_semaphore)
{
	int ret = sem_close ( xm_semaphore );   
	if(ret == -1)
		return 0;
	return 1;
}

// ɾ���ź���
int	XM_DeleteSemaphore (const char *semaphoreName)
{
	int ret = sem_unlink (semaphoreName);
	if(ret == -1)
		return 0;
	else
		return 1;
}

// �ȴ��ź���
int   XM_WaitSemaphore   (void *xm_semaphore)
{
	int ret = sem_wait( xm_semaphore ); 
	if(ret == -1)
		return 0;
	else
		return 1;
}

// �����ź���
int	XM_SignalSemaphore (void *xm_semaphore)
{
	int ret = sem_post( xm_semaphore );  
	if(ret == -1)
		return 0;
	else
		return 1;
}

#endif	// _WIN32

