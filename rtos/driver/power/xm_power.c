//****************************************************************************
//
//	Copyright (C) 2012~2016 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_power.c
//	  power device driver
//
//	Revision history
//
//		2015.12.02	ZhuoYongHong Initial version
//	
//
//****************************************************************************
#include "hardware.h"
#include "gpio.h"
#include "xm_key.h"
#include "xm_dev.h"
#include "rtos.h"
#include "xm_power.h"
#include "xm_adc.h"
#include "xm_base.h"
#include "xm_user.h"
#include "lcd.h"
#include "xm_printf.h"
#include "board_config.h"

#define	printk	XM_printf

extern void SetAutoPowerUpTime(UINT32 power_off_delay_seconds);
extern void PowerOn(void);

//按键状态机定义
#define	FSM_SCAN				1			// scan state
#define	FSM_CHECK				2			// check state
#define	FSM_DELAY				3			// repeat delay state
#define	FSM_REPEAT				4			// repeat state


// 电源按键 
#if DEMO_BB
#define	POWER_DET_GPIO		GPIO31
#define	POWER_CTRL_GPIO	GPIO30

#elif CZS_USB_01
#define	POWER_DET_GPIO		GPIO31
#define	POWER_CTRL_GPIO	GPIO30

#elif HONGJING_CVBS
#define	POWER_CTRL_GPIO	GPIO30
#define	POWER_DET_GPIO		GPIO31

#else
#define	POWER_DET_GPIO		GPIO34
#define	POWER_CTRL_GPIO	GPIO109//gpio33
#endif

static int powerkey_state;
static unsigned short powerkey_delay_time;
static int powerkey_first = 1;		// 标记开机时的按键状态

OS_TIMER_EX timer_100hz;		// 100hz定时器,监控power按键

static void powerkey_intr (void)
{
	XM_printf("powerkey intr\n");
}


// 检测开关机( GPIO34 )按键的状态，返回0x8000表示按键按下，0表示按键释放
int xm_powerkey_test (void)
{
	if(GetGPIOPadData(POWER_DET_GPIO) == euDataHigh )
	{
		//printk ("powerkey RELEASE\n");
		return 0;
	}
	else
	{
		//printk ("powerkey PRESS\n");
		return 0x8000;
	}
}

// 10Hz时钟
static void powerkey_process (void)
{
	if(powerkey_first)
	{
		// 检测PowerKey释放
		if(xm_powerkey_test() == 0)
		{
			// PowerKey已释放
			powerkey_first = 0;
		}
		return;
	}
	
	if(powerkey_state == FSM_SCAN)
	{
		// 按键扫描状态
		if(xm_powerkey_test())
		{
			#if 0
			// 按键按下
			// 切换到按键检查状态
			powerkey_state = FSM_CHECK;
			#else   //预防干扰
			delay (1000);
			if(xm_powerkey_test())
			{
			   	// 按键按下
				// 切换到按键检查状态
				powerkey_state = FSM_CHECK;
			}
			#endif
		}
	}
	else if(powerkey_state == FSM_CHECK)
	{
		// 第一次键扫描
		// re-scan to check circuit noise or not
		if(xm_powerkey_test())
		{
			#if 0
			// 连续两次均检测到按键按下，进入到按键释放检查阶段
			powerkey_state = FSM_DELAY;
			powerkey_delay_time = 200;	// 连续2秒按下执行关机
			#else //预防干扰
			delay (1000);
			if(xm_powerkey_test())
			{
				// 连续两次均检测到按键按下，进入到按键释放检查阶段
				powerkey_state = FSM_DELAY;
				powerkey_delay_time = 200;	// 连续2秒按下执行关机
			}
			#endif
		}
		else
		{
			// 键抖动
			powerkey_state = FSM_SCAN;
		}
	}
	else if(powerkey_state == FSM_DELAY)
	{
		powerkey_delay_time --;
		if(powerkey_delay_time == 0)
		{
			// 发送关机按键消息
			printk ("VK_POWER event\n");
			powerkey_delay_time = 200;		// 继续下一次Power按键检测
#if TULV
			// 当检测到外部供电为非USB供电时, 执行关机操作
			if(xm_power_check_usb_on() == 0)
			{
				XM_printf ("Power Key, VK_POWER\n"); 
				// XM_KeyEventProc (VK_POWER, XMKEY_PRESSED);
				XM_KeyEventPost (VK_POWER, XMKEY_PRESSED);
			}
#else
			// XM_KeyEventProc (VK_POWER, XMKEY_PRESSED);
			XM_KeyEventPost (VK_POWER, XMKEY_PRESSED);
#endif
			//SetAutoPowerUpTime (10);	// 测试代码, 设置延时开机功能, 延时时间为当前时间延后10秒
			//delay (100);
			//xm_power_unlock ();
		}
		// 检测按键是否释放
		else if(xm_powerkey_test() == 0)
		{
			#if 0
			// 返回按键扫描状态
			printk ("VK_POWER Key\n");
			powerkey_state = FSM_SCAN;
			
#if TULV
			// 短按 POWER键
			// 背光状态翻转
			if(XM_GetFmlDeviceCap (DEVCAP_BACKLIGHT))
			{
				//XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_BL_OFF);
				XM_KeyEventPost (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_BL_OFF);
			}
			else
			{
				//XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_BL_ON);
				XM_KeyEventPost (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_BL_ON);
			}
#endif
			#else  //预防干扰
			delay (1000);
			if(xm_powerkey_test()==0)
			{
				// 返回按键扫描状态
				printk ("VK_POWER Key\n");
				powerkey_state = FSM_SCAN;
				
#if TULV
				// 短按 POWER键
				// 背光状态翻转
				if(XM_GetFmlDeviceCap (DEVCAP_BACKLIGHT))
				{
					//XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_BL_OFF);
					XM_KeyEventPost (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_BL_OFF);
				}
				else
				{
					//XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_BL_ON);
					XM_KeyEventPost (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_BL_ON);
				}
#endif
			}
			#endif
		}
	}
	else
	{
		// 异常状态
		//XM_printf ("powerkey error state\n");
		powerkey_state = FSM_SCAN;
	}
}

//电源键：       
// KPB --> KEYSCAN2---( GPIO34 )----INPUT	输入脚，检测到连续低电平3秒关机
// ( GPIO33 )----OUTPUT 输出脚，开机后置为高电平。关机时，置为低电平

void xm_power_lock (void)
{
	printk ("power_lock\n");
	
	SetGPIOPadDirection (POWER_CTRL_GPIO, euOutputPad);	
	SetGPIOPadData (POWER_CTRL_GPIO, euDataHigh);
}

extern void PowerOff(void);

void xm_power_unlock (void)
{
	printk ("power_unlock\n");

	XM_lock ();
	PowerOff ();	// 将alarm pin脚输出置为低电平
	delay (1000);
	// 设置POWER_CTRL_GPIO为输出口，输出低电平
	SetGPIOPadDirection (POWER_CTRL_GPIO, euOutputPad);	
	SetGPIOPadData (POWER_CTRL_GPIO, euDataLow);
}

// 100hz定时器
static void timer_100hz_callback (void *data)
{
	powerkey_process ();
#if TULV
	if(!xm_power_check_acc_safe_or_not())
	{
		// 每0.5秒投递一次ACC掉电事件
		static unsigned int ticket_count = 0;
		ticket_count++;
		if(ticket_count % 50 == 0)
		{
			XM_printf ("VK_POWER\n");
			// XM_KeyEventProc (VK_POWER, XMKEY_PRESSED);
			XM_KeyEventPost (VK_POWER, XMKEY_PRESSED);
		}
	}
#endif
	OS_RetriggerTimerEx (&timer_100hz); /* Make timer periodical */
}

static unsigned int acc_power_state = 0;
static unsigned int acc_power_locked = 0;


// 检测ACC是否已上电
//	返回值
//		1		Acc已上电
//		0		Acc未上电
int xm_power_check_acc_on (void)
{
#if 0
#if HONGJING_CVBS
	#define	ACC_DETECT_GPIO	GPIO31
	
	unsigned int val;
	EU_GPIO_Data AccState;
	// GPIO31/ACC
	// pad_ctl3
	// [5:3]	itu_c_vsync_in	itu_c_vsync_in_pad	GPIO31
	// 
	XM_lock ();
	val = rSYS_PAD_CTRL03;
	val &= ~( (7 << 3) );
	rSYS_PAD_CTRL03 = val;
	XM_unlock ();

	XM_lock();
	SetGPIOPadDirection (ACC_DETECT_GPIO, euInputPad);
	XM_unlock();	
	
	AccState = GetGPIOPadData (ACC_DETECT_GPIO);
	// ACC通过三极管反相接到GPIO31
	if(AccState)
		return 0;
	else
		return 1;
#else
    #if 0
	#define	ACC_DETECT_GPIO	GPIO32
	
	unsigned int val;
	EU_GPIO_Data AccState;
	// GPIO32/ACC
	// pad_ctl3
	// [8:6]	itu_c_hsync_in	itu_c_hsync_in_pad	GPIO32
	// 
	XM_lock ();
	val = rSYS_PAD_CTRL03;
	val &= ~( (7 << 6) );
	rSYS_PAD_CTRL03 = val;
	XM_unlock ();

	XM_lock();
	SetGPIOPadDirection (ACC_DETECT_GPIO, euInputPad);
	XM_unlock();	
	
	AccState = GetGPIOPadData (ACC_DETECT_GPIO);
	if(AccState)
		return 1;
	else
		return 0;
    #endif
    return 1;
#endif
#endif
	return 1;
}

// 检查ACC是否安全电压
// 监控ACC上电 --> ACC掉电的过程, 当该过程发生时, 认为ACC非安全
// 返回值
//		1		Acc供电安全
//		0		Acc供电不安全
int xm_power_check_acc_safe_or_not (void)
{
#if 0
#if HONGJING_CVBS
	int acc_curr_state;
	int ret = 1;
	XM_lock ();
	if(acc_power_locked)
	{
		// 表示ACC上电 --> ACC掉电 过程已存在
		ret = 0;
	}
	else
	{
		// 比较ACC是否变化
		acc_curr_state = xm_power_check_acc_on ();
		if(acc_curr_state == 0 && acc_power_state == 1)
		{
			// ACC上电 --> ACC掉电
			acc_power_state = 0;
			acc_power_locked = 1;	// 锁定该状态
			ret = 0;
			XM_printf ("ACC ON -> ACC OFF\n");
		}
		else if(acc_curr_state == 1 && acc_power_state == 0)
		{
			// ACC掉电 -- > ACC上电
			// g-sensor触发启动后, ACC随后上电
			acc_power_state = 1; 	// 修改ACC初始状态
			XM_printf ("ACC OFF -> ACC ON\n");
		}
	}
	XM_unlock ();
	return ret;
	
#elif TULV
	
	//return xm_power_check_usb_on();
	//return 1;
	
	int acc_curr_state;
	DWORD dwVoltageLevel;
	// 判断是否USB供电. 
	if(xm_power_check_usb_on())
	{
		// USB已供电, 安全
		acc_power_locked = 0;	// 解除锁定状态
		acc_power_state = 1;
		return 1;
	}
	else
	{
		// 比较ACC是否变化
		acc_curr_state = xm_power_check_acc_on ();
		if(acc_curr_state == 0 && acc_power_state == 1)
		{
		   	// ACC上电 --> ACC掉电
			acc_power_state = 0;
			acc_power_locked = 1;	// 锁定该状态
			XM_printf ("ACC ON -> ACC OFF\n");
			return 0;
		}
		
		// USB未供电
		// 检查ACC状态是否已锁定, 即电池电压进入过WORST状态一次
		if(acc_power_locked)
		{
			// ACC供电非安全
			return 0;
		}
		// 电池供电. 检查主电池电压级别
		dwVoltageLevel = XM_GetFmlDeviceCap (DEVCAP_MAINBATTERYVOLTAGE);
		if(dwVoltageLevel == DEVCAP_VOLTAGE_WORST)
		{
			XM_printf ("worst case, power state locked\n");
			acc_power_locked = 1;	// 锁定该状态
			return 0;	// 非安全供电
		}
		return 1;		// 供电安全
	}
#else
	return 1;
#endif	
#endif

	return 1;
}

// 检查USB是否供电
int xm_power_check_usb_on (void)
{
	return 1;
}

void xm_board_power_ctl(u8 state)
{   
    printf("-----------------------> xm_board_power_ctl : %d \n",state);

	if(state==ON)
	{
		set_power5v_ctrl_pin_value(1);
	}
	else if(state==OFF)
	{
		set_power5v_ctrl_pin_value(0);
	}
}

void xm_powerkey_init (void)
{
	power5v_ctrl_pin_init();
	xm_board_power_ctl(ON);
}