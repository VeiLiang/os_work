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

//����״̬������
#define	FSM_SCAN				1			// scan state
#define	FSM_CHECK				2			// check state
#define	FSM_DELAY				3			// repeat delay state
#define	FSM_REPEAT				4			// repeat state


// ��Դ���� 
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
static int powerkey_first = 1;		// ��ǿ���ʱ�İ���״̬

OS_TIMER_EX timer_100hz;		// 100hz��ʱ��,���power����

static void powerkey_intr (void)
{
	XM_printf("powerkey intr\n");
}


// ��⿪�ػ�( GPIO34 )������״̬������0x8000��ʾ�������£�0��ʾ�����ͷ�
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

// 10Hzʱ��
static void powerkey_process (void)
{
	if(powerkey_first)
	{
		// ���PowerKey�ͷ�
		if(xm_powerkey_test() == 0)
		{
			// PowerKey���ͷ�
			powerkey_first = 0;
		}
		return;
	}
	
	if(powerkey_state == FSM_SCAN)
	{
		// ����ɨ��״̬
		if(xm_powerkey_test())
		{
			#if 0
			// ��������
			// �л����������״̬
			powerkey_state = FSM_CHECK;
			#else   //Ԥ������
			delay (1000);
			if(xm_powerkey_test())
			{
			   	// ��������
				// �л����������״̬
				powerkey_state = FSM_CHECK;
			}
			#endif
		}
	}
	else if(powerkey_state == FSM_CHECK)
	{
		// ��һ�μ�ɨ��
		// re-scan to check circuit noise or not
		if(xm_powerkey_test())
		{
			#if 0
			// �������ξ���⵽�������£����뵽�����ͷż��׶�
			powerkey_state = FSM_DELAY;
			powerkey_delay_time = 200;	// ����2�밴��ִ�йػ�
			#else //Ԥ������
			delay (1000);
			if(xm_powerkey_test())
			{
				// �������ξ���⵽�������£����뵽�����ͷż��׶�
				powerkey_state = FSM_DELAY;
				powerkey_delay_time = 200;	// ����2�밴��ִ�йػ�
			}
			#endif
		}
		else
		{
			// ������
			powerkey_state = FSM_SCAN;
		}
	}
	else if(powerkey_state == FSM_DELAY)
	{
		powerkey_delay_time --;
		if(powerkey_delay_time == 0)
		{
			// ���͹ػ�������Ϣ
			printk ("VK_POWER event\n");
			powerkey_delay_time = 200;		// ������һ��Power�������
#if TULV
			// ����⵽�ⲿ����Ϊ��USB����ʱ, ִ�йػ�����
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
			//SetAutoPowerUpTime (10);	// ���Դ���, ������ʱ��������, ��ʱʱ��Ϊ��ǰʱ���Ӻ�10��
			//delay (100);
			//xm_power_unlock ();
		}
		// ��ⰴ���Ƿ��ͷ�
		else if(xm_powerkey_test() == 0)
		{
			#if 0
			// ���ذ���ɨ��״̬
			printk ("VK_POWER Key\n");
			powerkey_state = FSM_SCAN;
			
#if TULV
			// �̰� POWER��
			// ����״̬��ת
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
			#else  //Ԥ������
			delay (1000);
			if(xm_powerkey_test()==0)
			{
				// ���ذ���ɨ��״̬
				printk ("VK_POWER Key\n");
				powerkey_state = FSM_SCAN;
				
#if TULV
				// �̰� POWER��
				// ����״̬��ת
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
		// �쳣״̬
		//XM_printf ("powerkey error state\n");
		powerkey_state = FSM_SCAN;
	}
}

//��Դ����       
// KPB --> KEYSCAN2---( GPIO34 )----INPUT	����ţ���⵽�����͵�ƽ3��ػ�
// ( GPIO33 )----OUTPUT ����ţ���������Ϊ�ߵ�ƽ���ػ�ʱ����Ϊ�͵�ƽ

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
	PowerOff ();	// ��alarm pin�������Ϊ�͵�ƽ
	delay (1000);
	// ����POWER_CTRL_GPIOΪ����ڣ�����͵�ƽ
	SetGPIOPadDirection (POWER_CTRL_GPIO, euOutputPad);	
	SetGPIOPadData (POWER_CTRL_GPIO, euDataLow);
}

// 100hz��ʱ��
static void timer_100hz_callback (void *data)
{
	powerkey_process ();
#if TULV
	if(!xm_power_check_acc_safe_or_not())
	{
		// ÿ0.5��Ͷ��һ��ACC�����¼�
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


// ���ACC�Ƿ����ϵ�
//	����ֵ
//		1		Acc���ϵ�
//		0		Accδ�ϵ�
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
	// ACCͨ�������ܷ���ӵ�GPIO31
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

// ���ACC�Ƿ�ȫ��ѹ
// ���ACC�ϵ� --> ACC����Ĺ���, ���ù��̷���ʱ, ��ΪACC�ǰ�ȫ
// ����ֵ
//		1		Acc���簲ȫ
//		0		Acc���粻��ȫ
int xm_power_check_acc_safe_or_not (void)
{
#if 0
#if HONGJING_CVBS
	int acc_curr_state;
	int ret = 1;
	XM_lock ();
	if(acc_power_locked)
	{
		// ��ʾACC�ϵ� --> ACC���� �����Ѵ���
		ret = 0;
	}
	else
	{
		// �Ƚ�ACC�Ƿ�仯
		acc_curr_state = xm_power_check_acc_on ();
		if(acc_curr_state == 0 && acc_power_state == 1)
		{
			// ACC�ϵ� --> ACC����
			acc_power_state = 0;
			acc_power_locked = 1;	// ������״̬
			ret = 0;
			XM_printf ("ACC ON -> ACC OFF\n");
		}
		else if(acc_curr_state == 1 && acc_power_state == 0)
		{
			// ACC���� -- > ACC�ϵ�
			// g-sensor����������, ACC����ϵ�
			acc_power_state = 1; 	// �޸�ACC��ʼ״̬
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
	// �ж��Ƿ�USB����. 
	if(xm_power_check_usb_on())
	{
		// USB�ѹ���, ��ȫ
		acc_power_locked = 0;	// �������״̬
		acc_power_state = 1;
		return 1;
	}
	else
	{
		// �Ƚ�ACC�Ƿ�仯
		acc_curr_state = xm_power_check_acc_on ();
		if(acc_curr_state == 0 && acc_power_state == 1)
		{
		   	// ACC�ϵ� --> ACC����
			acc_power_state = 0;
			acc_power_locked = 1;	// ������״̬
			XM_printf ("ACC ON -> ACC OFF\n");
			return 0;
		}
		
		// USBδ����
		// ���ACC״̬�Ƿ�������, ����ص�ѹ�����WORST״̬һ��
		if(acc_power_locked)
		{
			// ACC����ǰ�ȫ
			return 0;
		}
		// ��ع���. �������ص�ѹ����
		dwVoltageLevel = XM_GetFmlDeviceCap (DEVCAP_MAINBATTERYVOLTAGE);
		if(dwVoltageLevel == DEVCAP_VOLTAGE_WORST)
		{
			XM_printf ("worst case, power state locked\n");
			acc_power_locked = 1;	// ������״̬
			return 0;	// �ǰ�ȫ����
		}
		return 1;		// ���簲ȫ
	}
#else
	return 1;
#endif	
#endif

	return 1;
}

// ���USB�Ƿ񹩵�
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