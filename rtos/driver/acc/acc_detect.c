#include "hardware.h"
#include "gpio.h"
#include "gpioi2c.h"
#include "ark7116_drv.h"
#include "xm_core.h"
#include "xm_printf.h"
#include "xm_dev.h"
#include "..\..\app\app.h"
#include <xm_video_task.h>
#include "acc_detect.h"
#include "xm_power.h"
#include "board_config.h"

#if 0
extern void reset_battery_dec_state(void);

#define ACC_FILT_PAR    	10
#define ACC_DETECT_TIME     50

struct acc_detect_info
{
    int acc_det_count;
    int acc_det_status;
	OS_TIMER ACC_Timer;
	OS_TIMER ACC_TimerOut;
	OS_TIMER ShutDownTimer;
};

static u8 g_acc_status = 0xff;
static struct acc_detect_info g_acc_detect;


//获取ACC状态
u8 get_acc_state(void)
{
    return g_acc_status;
}

//设置ACC状态
void set_acc_state(u8 value)
{
	if(value==ACC_OFF)
	{
		g_acc_status=ACC_OFF;
	}
	else if(value==ACC_ON)
	{
		g_acc_status=ACC_ON;
	}
}



void acc_ctrl_clear(void)
{
	g_acc_detect.acc_det_count =0;
	g_acc_detect.acc_det_status =0xff;
}

static void shutdown_timer_fun()
{
    OS_DeleteTimer(&g_acc_detect.ShutDownTimer);
    printf("--------------------> AP_GetMenuItem(APPMENUITEM_PARKMONITOR) : %d\n",AP_GetMenuItem(APPMENUITEM_PARKMONITOR));
    printf("--------------------> get_acc_state() : %d\n", get_acc_state());
    if(AP_GetMenuItem(APPMENUITEM_PARKMONITOR) == MOT_OFF && get_acc_state() == ACC_OFF)
    {
        xm_board_power_ctl(OFF);
    }
}


//ACC断开5s后,关闭电源
static void acc_timer_out_ctrl()
{
    static u8 i=0;
    static int notify_count = 0;
	
    XM_printf(">>>>>acc_timer_out_ctrl over \r\n");
    OS_DeleteTimer(&g_acc_detect.ACC_TimerOut); 
    if(AP_GetMenuItem(APPMENUITEM_PARKMONITOR) == MOT_OFF )
    {
        //if(notify_count == 0)
        //{
            printf("-------------------> AP_GetMenuItem(APPMENUITEM_PARKMONITOR) : %d\n",AP_GetMenuItem(APPMENUITEM_PARKMONITOR));
            AP_PostSystemEvent(SYSTEM_EVENT_SHUTDOWN_SOON);
            OS_CREATETIMER(&g_acc_detect.ShutDownTimer,shutdown_timer_fun,3000);
        //}
    }
}

//acc状态改变通知
static void acc_det_notify(u8 acc_status)
{
    switch(acc_status)
    {
        case DEVICE_EVENT_IN:
            set_acc_state(ACC_ON); 
            printf("------------------------->HX_ACC  DEVICE_EVENT_IN \r\n");
            //OS_DeleteTimer (&ACC_TimerOut); 
            //xm_board_power_ctl(ON); 
            AP_PostSystemEvent(SYSTEM_EVENT_ACC_CONNECT);
            break; 

        case DEVICE_EVENT_OUT:
            printf("------------------------->HX_ACC  DEVICE_EVENT_OUT \r\n");
            set_acc_state(ACC_OFF);
            reset_battery_dec_state();  //重置电量检测状态
            AP_PostSystemEvent(SYSTEM_EVENT_ACC_LOST_CONNECT);
            reset_battery_dec_state();
            OS_CREATETIMER(&g_acc_detect.ACC_TimerOut, acc_timer_out_ctrl, 5000);//5000MS,检测一次
            break;

		default:
			break;
    }
}


/**
 *
 *
 *ACC 定时检测函数
 * 
 */
static void acc_detect_ctrl()
{
    unsigned char cur_acc_status = 0xff;

    cur_acc_status = get_acc_det_pin_status();
    if(g_acc_detect.acc_det_status != cur_acc_status)
    {
		if(g_acc_detect.acc_det_count <= ACC_FILT_PAR + 1)
        {
            g_acc_detect.acc_det_count ++;
        }
    }
    else
    {
        if(g_acc_detect.acc_det_count)
        {
            g_acc_detect.acc_det_count = 0;
        }
    }

    if( g_acc_detect.acc_det_count > ACC_FILT_PAR )
    {
        printf("------------------------> cur_acc_status %x \r\n",cur_acc_status);
        acc_det_notify(cur_acc_status);
        g_acc_detect.acc_det_status = cur_acc_status;
    }

    OS_RetriggerTimer(&g_acc_detect.ACC_Timer);	
}


/**
 *
 *
 *ACC 任务初始化
 * 
 */
void XMSYS_ACC_Init(void)
{
    XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>XMSYS_ACC_Init\r\n");
    xm_board_power_ctl(ON);//仿真时,有时候输出的高电平只有1.5V左右,导致异常

	acc_ctrl_clear();//控制参数清0
	acc_det_pin_init();

	OS_CREATETIMER(&g_acc_detect.ACC_Timer, acc_detect_ctrl, ACC_DETECT_TIME);//50MS,检测一次
}

#endif
