//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: time_drv.c
//	  ��ʱ���¼�����
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#include <string.h>
#include <xm_user.h>
#include <xm_base.h>
#include <xm_dev.h>
#include <common_string.h>

// ϵͳ��������ʱ��XM_MAX_TIMER�����ֱ�ʹ��0 ~ XM_MAX_TIMER-1���ø���ʱ��

// ��¼��ʱ����ʱ��������ʱ��δʹ��ʱ��ֵΪ0.
static DWORD	dwTimeOut[XM_MAX_TIMER];

// ��¼��ʱ����һ�β�����ʱ��㣨����Ϊ��ʱ��λ��
static DWORD	dwClockExpires[XM_MAX_TIMER];

static volatile BYTE	ActiveTimerCount;		// ��ǰ��Ծ��TIMER����
static volatile BYTE	TimerEvent;				// ����Ƿ���Ҫ������ʱ��
static volatile BYTE TimerInit = 0;			// ��������Ƿ��ѳ�ʼ��

// ���ⲿʱ���жϵ���
unsigned char XM_TimerEventProc (void)
{
	/* called once timer interrupt */
	if(TimerInit && ActiveTimerCount)
	{
		TimerEvent = 1;
		XM_wakeup ();
	}
	return 1;
}

/* Open the Timer. */
void XM_TimerDriverOpen (void)
{
	ActiveTimerCount = 0;
	TimerEvent = 0;
         
	xm_memset (dwTimeOut, 0, sizeof(dwTimeOut));

	TimerInit = 1;	// ��������ѳ�ʼ�����
}

/* Close the Timer.  */
void XM_TimerDriverClose (void)
{
	// �ر����ж�ʱ��
	BYTE i;
	
	TimerInit = 0;		// ������������

	for (i = 0; i < XM_MAX_TIMER; i++)
	{
		if(dwTimeOut[i])
		{
			XM_KillTimer (i);		// ɾ���д����Ϣ
			dwTimeOut[i] = 0;
		}
	}
	ActiveTimerCount = 0;
}


/* Poll for Timer events */
XMBOOL XM_TimerDriverPoll (void)
{
	// ��������Ƿ��ѳ�ʼ�����
	if(TimerInit == 0)
		return 0;
	if(ActiveTimerCount == 0)	// ����Ƿ���ڻ�Ծ�Ķ�ʱ��
		return 0;
	return TimerEvent;	/* Timer Event empty */
}

XMBOOL XM_TimerDriverDispatchEvent (void)
{
	BYTE i;
	BYTE PostTimerEvent = 0;

	if(TimerEvent == 0)
		return 0;

	TimerEvent = 0;	// �޶�ʱ��ʹ��

	if(ActiveTimerCount)
	{
		// ���ڻ�Ծ�Ķ�ʱ��
		DWORD dwTime;

		/* determine if timer expired*/
		dwTime = XM_GetTickCount();
		i = 0;
		while (i < XM_MAX_TIMER)
		{
			if(dwTimeOut[i] > 0)
			{
				// ��ʱ����Ч
				if(dwClockExpires[i] <= dwTime)			// timer expired
				{
					PostTimerEvent = 1;	// ��Ǵ���TIMER��Ϣ��Ͷ��
					XM_PostMessage (XM_TIMER, i, 0);
					// ������һ�ζ�ʱ��������ʱ��
					dwClockExpires[i] = dwTime + (DWORD)dwTimeOut[i];
				}

			}
			i ++;
		}
	}
	return PostTimerEvent;
}


XMBOOL XM_SetTimer (BYTE idTimer, DWORD dwTimeoutClock)
{
	if(idTimer >= XM_MAX_TIMER)
		return 0;

	if(dwTimeoutClock == 0)	// ��ʱ�����������Ϊ0
		return 0;

	if(dwTimeOut[idTimer] == 0)
	{
		// δʹ��
		ActiveTimerCount ++;
	}
	dwTimeOut[idTimer] = dwTimeoutClock;

	dwClockExpires[idTimer] = XM_GetTickCount() + dwTimeoutClock;

	// ɾȥ��Ϣ�����вд��TIMER��Ϣ
	while(XM_PeekMessage (NULL, XM_TIMER, XM_TIMER)); 

	return 1;
}

XMBOOL XM_KillTimer (BYTE idTimer)
{
	if(idTimer >= XM_MAX_TIMER)
		return 0;

	if(dwTimeOut[idTimer] == 0)
		return 0;
	dwTimeOut[idTimer] = 0;

	ActiveTimerCount --;
			
	// ɾȥ��Ϣ�����вд��TIMER��Ϣ
	while(XM_PeekMessage (NULL, XM_TIMER, XM_TIMER)); 

	return 1;
}

