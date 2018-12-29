// 使用IAR PowerPac模拟ucos ii的API

#include <stdio.h>
#include "hardware.h"

//#include "RTOS.h"		// OS头文件
#include "FS.h"
#include "xm_core.h"

void  OSIntEnter (void)
{
}
void  OSIntExit (void)
{
}

void *OSSemCreate (INT16U cnt)
{
	OS_CSEMA* pCSema = (OS_CSEMA*)kernel_malloc (sizeof(OS_CSEMA));
	if(pCSema)
	{
		memset (pCSema, 0, sizeof(OS_CSEMA));
		OS_CreateCSema (pCSema, cnt);
	}
	// printf ("OSSemCreate cnt=%d, %x\n", cnt, pCSema);
	return (void *)pCSema;
}

void  *OSSemDel (void *pevent, INT8U opt, INT8U *err)
{
	OS_CSEMA* pCSema = pevent;
	if(pCSema == NULL)
	{
		*err = 4u;
		return NULL;
	}
	// printf ("OSSemDel %x\n", pevent);
	OS_DeleteCSema (pCSema);
	kernel_free (pCSema);
	*err = 0;
	return pevent;
}

INT8U  OSSemPost (void *pevent)
{
	if(pevent == NULL)
		return 4u;
	//  printf ("OSSemPost %x\n", pevent);
	OS_SignalCSema (pevent);
	return 0;
}

// 20131212 ZhuoYongHong
// OSSemPend 修改为32位超时单位。UCOS II为16位。
// MGC_UcFsDeviceWriteBurst 会出现超出16位的timeout
void  OSSemPend (void *pevent, INT32U timeout, INT8U *err)
{
	if(pevent == NULL)
	{
		*err = 4u;
		return;
	}
	
	if(timeout == 0)
	{
		//printf ("OS_WaitCSema %x\n", pevent);
		OS_WaitCSema (pevent);
		//printf ("OS_WaitCSema %x OK\n", pevent);
		*err = 0;
	}
	else
	{
		//printf ("OS_WaitCSemaTimed %x, %d\n", pevent, timeout*10);
		int ret = OS_WaitCSemaTimed (pevent, timeout*10);
		if(ret == 0)
		{
			// 0: Failed, semaphore not available before timeout
			*err = 10u;	// timeout
			//printf ("OS_WaitCSemaTimed %x timeout\n", pevent);
		}
		else
		{
			// 1: OK, semaphore was available and counter decremented.
			*err = 0;
			//printf ("OS_WaitCSemaTimed %x OK\n", pevent);
		}
		return;
	}
}

void  OSTimeDly (INT16U ticks)
{
	OS_Delay (ticks);
}

INT32U  OSTimeGet (void)
{
	return OS_GetTime32();
}

