#include "rtos.h"
#include "xm_type.h"
#include "types.h"
#include "board_config.h"
#include "gpio.h"
#include "xm_app_menudata.h"
#include "xm_user.h"


struct acc_detect_info
{
    int acc_det_count;
    int acc_det_status;
	OS_TIMER ACC_Timer;
	OS_TIMER ACC_TimerOut;
	OS_TIMER ShutDownTimer;
};

enum 
{
    DEVICE_EVENT_IN,
    DEVICE_EVENT_OUT,
};


enum 
{
    POWERON_TO_ACCON,
    ACCON_TO_ACCOFF,
    ACCOFF_TO_DELAY,
    ACCOFF_TO_ACCON,
    ACCOFF_TO_ACCOFF,
    ACCOFF_TO_MOTOPEN,
    ACCOFF_TO_MOTCLOSE,
    ACCOFF_MOTOPEN_TO_ACCON,
};

extern void MotionDetectStart(void);

#define ACC_FILT_PAR    				10
#define ACC_DETECT_TIME     			50
#define DET_ACC_TICKET_EVENT    		0x02

static u8 g_acc_status = 0xff;
static struct acc_detect_info g_acc_detect;
static OS_TASK TCB_DetEvent_Task;
#define	XMSYS_DetEvent_TASK_PRIORITY					100		// PR2000配置线程
#define	XMSYS_DetEvent_TASK_STACK_SIZE					(0x400)
__no_init static OS_STACKPTR int DetEventTask[XMSYS_DetEvent_TASK_STACK_SIZE/4];
static OS_TIMER DetEvent_DetTImer;
static OS_MAILBOX det_mailbox;		// 一字节邮箱。
static char det_mailbuf;			// 一个字节的邮箱


void dettask_send_mail(char ret);

static u8_t Brightness_first = 0;
u8_t Brightness_add = 0;

//获取ACC状态
u8 get_acc_state(void)
{
    return g_acc_status;
}

//设置ACC状态
void set_acc_state(u8 value)
{
	g_acc_status =  value;
}


//ACC断开5s后,关闭电源
static void acc_timer_out_ctrl()
{
    static u8 i=0;
    static BOOL need_shutdown = FALSE;
	OS_TIMER* acc_timeout = &(g_acc_detect.ACC_TimerOut);
    XM_printf(">>>>>acc_timer_out_ctrl over \r\n");

    XM_printf(">>>>>>>>>>>>>>>>>. get_acc_det_pin_status : %d\n",get_acc_det_pin_status());
    XM_printf(">>>>>>>>>>>>>>>>>. get_acc_state : %d\n",get_acc_state());
	if( (get_acc_det_pin_status()==ACC_ON) && (get_acc_state()==ACCOFF_TO_DELAY) )
	{//预防ACC ON到点火会有抖动
		set_acc_state(ACCOFF_TO_ACCON);
        OS_DeleteTimer(acc_timeout); 
        XM_printf("------------------------> acc_timer_out_ctrl 1 \n");
	}
	else if(get_acc_det_pin_status()==ACC_OFF) 
	{
	    if(get_acc_state() == ACCON_TO_ACCOFF || get_acc_state() == ACCOFF_TO_MOTOPEN)
	    {
	        if(AP_GetMenuItem(APPMENUITEM_PARKMONITOR) == MOT_OFF)
	        {
	            //若移动侦测关，这发送即将断线消息，下一次定时处理做关机操作
	            if(need_shutdown == FALSE)
	            {
	                XM_printf("------------------------> acc_timer_out_ctrl 2\n");
		            HW_LCD_BackLightOff();
		            AP_SetMenuItem(APPMENUITEM_POWER_STATE, POWER_OFF);
            		AP_PostSystemEvent(SYSTEM_EVENT_SHUTDOWN_SOON);//停止录像,desk中做判断
                    OS_RetriggerTimer(acc_timeout);
                    need_shutdown = TRUE;
                }
                else if(need_shutdown)
                {
                    //目前不会执行到这里，在邮件中做关机逻辑
                    XM_printf("------------------------> acc_timer_out_ctrl 3 \n");
                    OS_DeleteTimer(acc_timeout);
                    xm_board_power_ctl(POWER_OFF);
                }
            }
            else
            {
                //移动侦测打开状态，则关闭屏幕并结束定时器
                //向detmail发送邮件,启动移动侦测
        		HW_LCD_BackLightOff();
        		AP_SetMenuItem(APPMENUITEM_POWER_STATE, POWER_OFF);
        		XM_printf("------------------------> acc_timer_out_ctrl 4 \n");
                OS_DeleteTimer(acc_timeout); 
            } 
        }
        else if(get_acc_state() == ACCOFF_TO_DELAY)
        {// If acc is disconnected, an acc disconnect message is sent to the system;
    	    AP_PostSystemEvent(SYSTEM_EVENT_ACC_LOST_CONNECT);
    		set_acc_state(ACCON_TO_ACCOFF);
    		XM_printf("------------------------> acc_timer_out_ctrl 5 \n");
            OS_RetriggerTimer(acc_timeout);
        }
	}
}

//acc状态改变通知
static void acc_det_notify(u8 acc_status)
{
    switch(acc_status)
    {
        case DEVICE_EVENT_IN:
            XM_printf("------------------------->HX_ACC  DEVICE_EVENT_IN \r\n");
			if(get_acc_state()==ACCOFF_TO_MOTOPEN)
			{//需要启动录像,accoff--->accon
				XM_printf(">>>>>start rec......\r\n");
				HW_LCD_BackLightOn();
				set_acc_state(POWERON_TO_ACCON);
				//dettask_send_mail(0);
	            AP_PostSystemEvent(SYSTEM_EVENT_ACC_CONNECT);
		    	//MotionDetectStop();
			}
            break; 
        case DEVICE_EVENT_OUT:
        {
            OS_TIMER* acc_timeout = &(g_acc_detect.ACC_TimerOut);
            XM_printf("------------------------->HX_ACC  DEVICE_EVENT_OUT \r\n");
			set_acc_state(ACCOFF_TO_DELAY);
			reset_battery_dec_state();  //重置电量检测状态
			OS_CREATETIMER(acc_timeout, acc_timer_out_ctrl, 3000);//3000MS,检测一次
	    }
            break;
		default:
			break;
    }
}

void DetEvent_task(void)
{
	u8_t Det_event;
    u8_t cur_acc_status;
	char response_code;
	
	while(1) 
	{
        Det_event = OS_WaitEvent(DET_ACC_TICKET_EVENT);

        if(Det_event & DET_ACC_TICKET_EVENT)
        {//周期性检测ACC
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
        }

		if(OS_GetMailTimed(&det_mailbox, &response_code, 2) == 0)
		{
			XM_printf(">>>>DetEvent_task response_code:%d\r\n", response_code);
		    XM_printf("-------------------> AP_GetMenuItem(APPMENUITEM_PARKMONITOR) : %d\n",AP_GetMenuItem(APPMENUITEM_PARKMONITOR));
            if(get_acc_det_pin_status() == ACC_OFF && get_acc_state() == ACCON_TO_ACCOFF)
            {
                if(AP_GetMenuItem(APPMENUITEM_PARKMONITOR) == MOT_ON)
    			{//移动侦测开
    				//motion任务检测移动物体
    				set_acc_state(ACCOFF_TO_MOTOPEN);
    				if(IsMotionDetectStarted()== FALSE)
    				{
    				    MotionDetectStart();
    				}
    			}
    			else if(AP_GetMenuItem(APPMENUITEM_PARKMONITOR) == MOT_OFF )
    		    {//移动侦测关
    		    	XM_printf(">>>>close main power.......\r\n");
    			    if(IsMotionDetectStarted() == TRUE)
    			    {
    			        XM_printf("-----------------------------------------> MotionDetectStop 3 \n");
        		    	MotionDetectStop();
    		    	}
    				set_acc_state(ACCOFF_TO_MOTCLOSE);
                    xm_board_power_ctl(OFF);
    		    }
			}
			else
			{
			    if(IsMotionDetectStarted() == TRUE)
			    {
			        XM_printf("-----------------------------------------> MotionDetectStop 4 \n");
			        MotionDetectStop();
			    }
			}
		}		
    }
}

void acc_ctrl_clear(void)
{
	g_acc_detect.acc_det_count =0;
	g_acc_detect.acc_det_status =0xff;
}

void dettask_send_mail(char ret)
{
	OS_ClearMB (&det_mailbox);//防止阻塞
	OS_PutMail(&det_mailbox, &ret);
}

void DetEvent_TicketCallback(void)
{
    OS_SignalEvent(DET_ACC_TICKET_EVENT, &TCB_DetEvent_Task); /* 通知事件 */
	OS_RetriggerTimer(&DetEvent_DetTImer);
}

void XMSYS_DetEventInit (void)
{
	XM_printf(">>>>>>XMSYS_DetEventInit........\r\n");
    xm_board_power_ctl(ON);//仿真时,有时候输出的高电平只有1.5V左右,导致异常

	set_acc_state(POWERON_TO_ACCON);
	acc_ctrl_clear();//控制参数清0
	acc_det_pin_init();

	OS_CREATEMB(&det_mailbox, 1, 1, &det_mailbuf);	
	OS_CREATETIMER(&DetEvent_DetTImer, DetEvent_TicketCallback, 100);//50ms，定时发送事件
	OS_CREATETASK(&TCB_DetEvent_Task, "DetEvent", DetEvent_task, XMSYS_DetEvent_TASK_PRIORITY, DetEventTask);
}



