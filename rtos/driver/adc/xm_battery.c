//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_touch_drv.c
//	  触摸事件驱动
//
//	Revision history
//
//		2010.09.02	ZhuoYongHong Initial version
//
//****************************************************************************

#include <xm_type.h>
#include <xm_user.h>
#include <xm_dev.h>
#include <xm_base.h>
#include <xm_key.h>
#include <xm_printf.h>



typedef struct {
	unsigned int battery;
}BATTERY_EVENT;

#define	BATTERY_BUFFER_SIZE	0x1F		/* maximum key strobe buffer */


static BATTERY_EVENT		BatteryBuffer[BATTERY_BUFFER_SIZE+1];		// 循环队列
static volatile	BYTE	   	BatteryBPos;			// 循环队列首部
static volatile	BYTE		BatteryEPos;			// 循环队列尾部
static volatile	BYTE		BatteryInit = 0;		// 标识触摸驱动是否已初始化


// 投递硬件触摸事件, 由外部硬件驱动调用
// 加入到队尾
// 投递触摸事件到消息队列
// point		触摸位置
// type		触摸事件类型
// 返回值定义
//		1		事件投递到事件缓冲队列成功
//		0		事件投递到事件缓冲队列失败
unsigned char XM_BatteryEventProc(unsigned int batteryvalue)
{
	BATTERY_EVENT *bat;

	XM_lock ();
	if(BatteryInit == 0)
	{
		XM_unlock ();
		return 0;
	}

	// 检查自动测试是否存在。自动测试下禁止触摸操作
	//if(TesterSendCommand(TESTER_CMD_TEST, NULL))
	//	return 0;

	// 检查队列已满
	if( ((BatteryEPos + 1) & BATTERY_BUFFER_SIZE) == (BatteryBPos & BATTERY_BUFFER_SIZE) )
	{
		XM_unlock ();
		return 0;
	}

	bat = BatteryBuffer + (BatteryEPos & BATTERY_BUFFER_SIZE);
	bat->battery = batteryvalue;
	BatteryEPos ++;

	XM_wakeup ();

	XM_unlock ();

	return 1;
}

XMBOOL XM_BatteryDriverGetEvent(unsigned int *batteryvalue)
{
	BATTERY_EVENT *bat;
	XMBOOL ret = FALSE;
	
	if(BatteryInit == 0)
		return FALSE;
	
	XM_lock ();	// 保护互斥访问
	if( BatteryBPos != BatteryEPos )
	{
		ret = TRUE;
		bat = BatteryBuffer + (BatteryBPos & BATTERY_BUFFER_SIZE);

		*batteryvalue = bat->battery;

		BatteryBPos ++;
	}
	XM_unlock ();
	
	return ret;
}


void XM_BatteryDriverOpen(void)
{
	XM_lock ();
	
	BatteryBPos = BatteryEPos = 0;
	BatteryInit = 1;
		
	XM_unlock ();
}


void XM_BatteryDriverClose (void)
{
	XM_lock ();
	BatteryInit = 0;
	BatteryBPos = BatteryEPos = 0;
	XM_unlock ();
}


XMBOOL XM_BatteryDriverPoll(void)
{
	XMBOOL ret = FALSE;

	XM_lock();	// 保护互斥访问
	if( BatteryEPos != BatteryBPos )	/* Key Event buffer empty */
		ret = TRUE;
	XM_unlock();

	return ret;
}
