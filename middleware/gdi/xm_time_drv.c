//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: time_drv.c
//	  定时器事件驱动
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

// 系统共定义软时钟XM_MAX_TIMER个，分别使用0 ~ XM_MAX_TIMER-1引用该软时钟

// 记录定时器的时间间隔。定时器未使用时其值为0.
static DWORD	dwTimeOut[XM_MAX_TIMER];

// 记录定时器下一次产生的时间点（毫秒为计时单位）
static DWORD	dwClockExpires[XM_MAX_TIMER];

static volatile BYTE	ActiveTimerCount;		// 当前活跃的TIMER个数
static volatile BYTE	TimerEvent;				// 标记是否需要遍历定时器
static volatile BYTE TimerInit = 0;			// 标记驱动是否已初始化

// 由外部时钟中断调用
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

	TimerInit = 1;	// 标记驱动已初始化完毕
}

/* Close the Timer.  */
void XM_TimerDriverClose (void)
{
	// 关闭所有定时器
	BYTE i;
	
	TimerInit = 0;		// 标记驱动已完毕

	for (i = 0; i < XM_MAX_TIMER; i++)
	{
		if(dwTimeOut[i])
		{
			XM_KillTimer (i);		// 删除残存的消息
			dwTimeOut[i] = 0;
		}
	}
	ActiveTimerCount = 0;
}


/* Poll for Timer events */
XMBOOL XM_TimerDriverPoll (void)
{
	// 检查驱动是否已初始化完毕
	if(TimerInit == 0)
		return 0;
	if(ActiveTimerCount == 0)	// 检查是否存在活跃的定时器
		return 0;
	return TimerEvent;	/* Timer Event empty */
}

XMBOOL XM_TimerDriverDispatchEvent (void)
{
	BYTE i;
	BYTE PostTimerEvent = 0;

	if(TimerEvent == 0)
		return 0;

	TimerEvent = 0;	// 无定时器使用

	if(ActiveTimerCount)
	{
		// 存在活跃的定时器
		DWORD dwTime;

		/* determine if timer expired*/
		dwTime = XM_GetTickCount();
		i = 0;
		while (i < XM_MAX_TIMER)
		{
			if(dwTimeOut[i] > 0)
			{
				// 定时器有效
				if(dwClockExpires[i] <= dwTime)			// timer expired
				{
					PostTimerEvent = 1;	// 标记存在TIMER消息的投递
					XM_PostMessage (XM_TIMER, i, 0);
					// 更新下一次定时器产生的时间
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

	if(dwTimeoutClock == 0)	// 定时器间隔不允许为0
		return 0;

	if(dwTimeOut[idTimer] == 0)
	{
		// 未使用
		ActiveTimerCount ++;
	}
	dwTimeOut[idTimer] = dwTimeoutClock;

	dwClockExpires[idTimer] = XM_GetTickCount() + dwTimeoutClock;

	// 删去消息队列中残存的TIMER消息
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
			
	// 删去消息队列中残存的TIMER消息
	while(XM_PeekMessage (NULL, XM_TIMER, XM_TIMER)); 

	return 1;
}

