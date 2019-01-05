#include "hardware.h"
#include "gpio.h"
#include "gpioi2c.h"
#include "ark7116_drv.h"
#include "xm_core.h"
#include "xm_printf.h"
#include "xm_dev.h"
#include "..\..\app\app.h"
#include <xm_video_task.h>
#include "types.h"
#include "xm_app_menudata.h"

#include "trigger_array_stack.h"
#include "acc_detect.h"
#include "board_config.h"


#define AH2_MODE 4
#define TRIG_FILT_PAR    	10
#define TRIG_DETECT_TIME     25

static OS_TIMER Trigger_Timer;
static OS_TIMER Trigger_Timer_Delay;//延迟触发

static u8 trig_change_state = FALSE;
static u8 trig_change_set_perch_state = FALSE;

static u8 trig_notify_delay_time = 0;

extern void hw_backlight_set_auto_off_ticket (unsigned int ticket);

//返回触发状态
u8 get_trigger_det_status(void)
{
    u8 res = 0;
    
    u8 acc1_status = 0;//GetGPIOPadData (ACC1_PIN)? 0 : 1;
    u8 acc2_status = get_trig_det_pin_status() ? 0 : 1;
    u8 acc3_status = 0;//GetGPIOPadData (ACC3_PIN)? 0 : 1;//read_pr0_value() ? 0 : 1; //
    u8 acc4_status = 0;//GetGPIOPadData (ACC4_PIN)? 0 : 1;
    res = acc1_status + (acc2_status << 1) + (acc3_status << 2) + (acc4_status << 3);

    //printf(">>>acc1 status:%d\r\n", acc1_status);
    //printf(">>>acc2 status:%d\r\n", acc2_status);
    //printf(">>>acc3 status:%d\r\n", acc3_status);
    //printf(">>>acc4 status:%d\r\n", acc4_status);
    //printf(">>>res:%d\r\n", res);
    
    return res;
}
static u8 last_trig = 0;

struct sys_event {
    u8 type;
    void *arg;

};

//触发改变状态:但正在处理触发时,置1,要等触发完以后才能进行下次触发
u8 get_trig_change_state()
{
    return trig_change_state;
}

void set_trig_change_state(u8 state)
{
    if(state==FALSE)
    {
        trig_change_state=FALSE;
    }
    else
    {
        trig_change_state=TRUE;
    }
}

struct trig_detect_info
{
    int status;
    int count;

    int timer;

    u8 parking_trig_status;
    u8 delay_return_trig;
    TRIGGER_STACK_PTR trig_stack;
};

typedef struct trig_notify_params
{
    u8 notify_trig;
    u8 notify_status;
}TRIG_NOTIFY_PARAMS,*TRIG_NOTIFY_PARAMS_PTR;
static  struct trig_notify_params trig_notify_param;

static struct trig_detect_info g_trig_detect;

static u8 trig_on_change = 0;//比上一次新增加的触发
static u8 trig_off_change = 0;//比上一次新断开的触发

static u8 trig_pre_ch = 0xff;

u8 get_parking_trig_status(void)
{
    return g_trig_detect.parking_trig_status;
}

u8 get_delay_return_trig()
{
    return g_trig_detect.delay_return_trig;
}

u8 get_trig_on_change(void)
{
    return trig_on_change;
}

u8 get_trig_off_change(void)
{
    return trig_off_change;
}

void trig_reset(void)
{
    trig_on_change = 0;
    g_trig_detect.status = 0;
}

//获取触发器检测状态
int get_trig1_status(void)
{
    return (g_trig_detect.status & TRIG_1);
}

int get_trig2_status(void)
{
    return (g_trig_detect.status & TRIG_2);
}

int get_trig3_status(void)
{
    return (g_trig_detect.status & TRIG_3);
}

int get_trig4_status(void)
{
    return (g_trig_detect.status & TRIG_4);
}

int get_parking_status(void)
{
#if PARKING_TRIG== TRIG_1
    return get_trig1_status();
#elif PARKING_TRIG== TRIG_2
    return get_trig2_status();
#elif PARKING_TRIG== TRIG_3
    return get_trig3_status();
#elif PARKING_TRIG== TRIG_4
    return get_trig4_status();
#else
    return get_trig1_status();
#endif
}

///////////////////////////////////
//触发器栈操作

static void remove_trig_info(u8 trig_change)
{
    //TRIGGER_STACK_PTR trig_stack = g_trig_detect.trig_stack;
    if(get_trigger_stack_size() > 0)
    {
        if(trig_change & TRIG_1)
        {
            remove_trigger_data_by_name(TRIG_1);
        }
        if(trig_change & TRIG_2)
        {
            remove_trigger_data_by_name(TRIG_2);
        }
        if(trig_change & TRIG_3)
        {
            remove_trigger_data_by_name(TRIG_3);
        }
        if(trig_change & TRIG_4)
        {
            remove_trigger_data_by_name(TRIG_4);
        }
    }
}

static void push_trig_info(u8 trig_change)
{
    //TRIGGER_STACK_PTR trig_stack = g_trig_detect.trig_stack;
    if(trig_change & TRIG_1)
    {
        push_trigger_data(TRIG_1,0);// get_sys_config(SPT_TRIG1_DELAY));
    }
    if(trig_change & TRIG_2)
    {
        push_trigger_data(TRIG_2,0);// get_sys_config(SPT_TRIG2_DELAY));
    }
    if(trig_change & TRIG_3)
    {
        push_trigger_data(TRIG_3,0);// get_sys_config(SPT_TRIG3_DELAY));
    }
    if(trig_change & TRIG_4)
    {
        push_trigger_data(TRIG_4, 0);//get_sys_config(SPT_TRIG4_DELAY));
    }
}
//
/////////////////////////////////////////////////////////////////////////////////

u8 get_top_trig_ch()
{
    u8 cur_trig_ch = 0xff;
    //TRIGGER_STACK_PTR trig_stack = g_trig_detect.trig_stack;
    if(get_trigger_stack_size() > 0)
    {
        TRIGGER_DATA_NODE_PTR trig_node = get_top_trigger_data();//trig_stack->get_top_data(trig_stack);
        if( trig_node->trig_index == TRIG_1)
        {
            cur_trig_ch = 0x01 ;//get_sys_config(SPT_TRIG1);
        }
        else if( trig_node->trig_index == TRIG_2 )
        {
            cur_trig_ch = 0x02 ;//get_sys_config(SPT_TRIG2);
        }
        else if( trig_node->trig_index == TRIG_3 )
        {
            cur_trig_ch = 0x04 ;//get_sys_config(SPT_TRIG3);
        }
        else if(trig_node->trig_index == TRIG_4)
        {
            cur_trig_ch = 0x08 ;//get_sys_config(SPT_TRIG4);
        }
    }
    else
    {
        cur_trig_ch = AP_GetMenuItem(APPMENUITEM_PRE_CH);//读取先前通道
        
    }
    return cur_trig_ch;
}

static u8 get_top_trig_type()
{
    u8 cur_trig_ch = 0xff;
    //TRIGGER_STACK_PTR trig_stack = g_trig_detect.trig_stack;
    printf(" -----------------------> get_trigger_stack_size() : %x\r\n",get_trigger_stack_size());
    if(get_trigger_stack_size() > 0)
    {
        TRIGGER_DATA_NODE_PTR trig_node = get_top_trigger_data();
        if(trig_node == NULL)
        {
            printf(" -----------------------> trig_node NULL\r\n");
            return cur_trig_ch;
        }
        if( trig_node->trig_index == TRIG_1 )
        {
            cur_trig_ch = TRIG_1;
        }
        else if( trig_node->trig_index == TRIG_2 )
        {
            cur_trig_ch = TRIG_2;
        }
        else if( trig_node->trig_index == TRIG_3 )
        {
            cur_trig_ch = TRIG_3;
        }
        else if( trig_node->trig_index == TRIG_4 )
        {
            cur_trig_ch = TRIG_4;
        }
    }
    return cur_trig_ch;
}

int trig_on(u8 trig_index)
{
    u8 trig_ch = 0xff;
    u8 cur_trig = 0;

    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>trig_on>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
    if(trig_change_state)
    {
        return 0;
    }
    trig_change_state = TRUE;
    cur_trig = trig_index;

    if(last_trig == cur_trig)
    {
        trig_change_state = FALSE;
        return 0;
    }
    last_trig = cur_trig;
    if(trig_change_set_perch_state == FALSE)
    {
        AP_SetMenuItem(APPMENUITEM_PRE_CH, AP_GetMenuItem(APPMENUITEM_CH));//保存当前通道
        printf(" > set per ch : %x \r\n",AP_GetMenuItem(APPMENUITEM_PRE_CH));
        trig_change_set_perch_state = TRUE;
    }

    if(AP_GetMenuItem(APPMENUITEM_CH)!=trig_ch)
    {//当前通道与触发器对应通道不同
		AP_SetMenuItem(APPMENUITEM_CH, CH_AHD2);  //进入触发通道,这里只有一路触发,默认通道2
    }
	
    if(cur_trig==PARKING_TRIG)
    {
    	HW_LCD_BackLightOff();
    	AP_PostSystemEvent(SYSTEM_EVENT_CARBACKLINE_ENTER);

    }   
    trig_change_state = FALSE;
    hw_backlight_set_auto_off_ticket(0xFFFFFFFF);//关闭屏保
    return 0;
}

void trig_off(u8 trig_index)
{

    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>trig_off>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");


    if(trig_change_state)
    {
        return;
    }
   // reset_screen_saver_cnt();
    trig_change_state = TRUE;
    u8 pre_ch = 0xff;

	rxchip_check_ctrl(0);
	reset_check_time();
	
    pre_ch = get_top_trig_ch();
    AP_SetMenuItem(APPMENUITEM_CH, AP_GetMenuItem(APPMENUITEM_PRE_CH));//读取先前通道
    last_trig = 0;
    if(trig_index == PARKING_TRIG)
    {
		AP_PostSystemEvent(SYSTEM_EVENT_CARBACKLINE_EXIT);
    //    hide_park_ui();
   //     OSTimeDly(50);
    //    show_rec_main_ui();
    	//XM_PushWindow(0);
    }

//	OSTimeDly(50);
//	rxchip_check_ctrl(1);
#if 0
    if( (get_sys_config(SPT_POWER)==POWER_ON) && (get_acc_det_status() == ACC_ON) )
	{
        lcd_backctrl(1);//backlight on
	}
	else if(get_sys_config(SPT_POWER)==POWER_STATE_OFF )
    {
 		lcd_backctrl(0);//backlight off
	}
#endif	
	if(get_trigger_count() == 0)
	{
    	trig_change_set_perch_state = FALSE;
	}
	trig_change_state = FALSE;
    AP_SetMenuItem(APPMENUITEM_LCD, AP_GetMenuItem(APPMENUITEM_LCD));//关闭屏保
}

void sys_event_notify(struct sys_event *p_eve)//
{
    u8_t notify_trig = (u8_t)p_eve->arg;
     if(notify_trig != TRIG_1 || notify_trig != TRIG_2 ||notify_trig != TRIG_3 || notify_trig != TRIG_4 )
       {
         
           //printf(" TRIG ---------------------->DEVICE_EVENT %s\r\n",(char *)event->arg);
           u8 pre_ch = 0xff;
    
           switch (p_eve->type) {
               case DEVICE_EVENT_IN:
               #if 0
                   if(get_sys_config(SPT_POWER)==POWER_ON)
                   {
                       set_sys_config(SPT_PARKINGSTATE, POWER_ON_PARKING);
                   }
                   else
                   {
                       set_sys_config(SPT_PARKINGSTATE, POWER_OFF_PARKING);
                   }
                   if( ((char *)event->arg)==HX_PARKING_TRIG )
                   {
                       set_redraw_run_time_flg(true);
                   }
               #endif
                   trig_on((u8)p_eve->arg);
                   break;
    
               case DEVICE_EVENT_OUT:// trig off
                //   set_sys_config(SPT_PARKINGSTATE, NORMAL_GO_AHEAD);
                
                   trig_off((u8)p_eve->arg);
                   break;
    
               default:                
                   break;
           }
       }

}

//触发器状态消息发送
void trig_det_notify(u8 trig_type, u8 status)
{
    struct sys_event eve;
    #if 0
    switch(trig_type)
    {
        case TRIG_1:
            eve.arg = HX_TRIG1;
            break;
        case TRIG_2:
            eve.arg = HX_TRIG2;
            break;
        case TRIG_3:
            eve.arg = HX_TRIG3;
            break;
        case TRIG_4:
            eve.arg = HX_TRIG4;
            break;
    }
    #endif
    eve.arg = (void *)trig_type;
    eve.type = status;
    
	if (status == DEVICE_EVENT_IN) {
	   
	    printf("----------------> trig_det_notify DEVICE_EVENT_IN \n");
        if(trig_type == PARKING_TRIG)
        {
            g_trig_detect.parking_trig_status = 1;
        }
		//sys_key_event_disable();
	} 
	else if(status == DEVICE_EVENT_OUT)
	{
	    printf("----------------> trig_det_notify DEVICE_EVENT_OUT \n");
        if(trig_type == PARKING_TRIG)
        {
            g_trig_detect.parking_trig_status = 0;
        }
        g_trig_detect.delay_return_trig = TRIGGER_DELAY_END;
        if(get_trigger_stack_size() == 0)
        {
        //	sys_key_event_enable();
        }
	}
	sys_event_notify(&eve);//MSG FOR parking ui DISP
}

u8 get_prior_trig(u8 trigs)  
{
    u8 trig_type = 0;
    u8 trig_bit = log2(trigs);
    if(trig_bit <= 3)   
    {
        trig_type = 0x01 << trig_bit;
    }
    if(trigs & PARKING_TRIG)
    {
        trig_type = PARKING_TRIG;
    }
    return trig_type;
}

static void reset_trig_notify_delay_time()
{
    trig_notify_delay_time = 0;
}
static void trig_det_notify_lmp()    //只有触发器延时情况下 才调用
{
    if((AP_GetMenuItem(APPMENUITEM_PARKING_DELAY) * 10) <= trig_notify_delay_time)
    {
        trig_det_notify(trig_notify_param.notify_trig ,trig_notify_param.notify_status);
        OS_DeleteTimer(&Trigger_Timer_Delay);
        trig_notify_delay_time = 0;
    }
    else
    {
        printf("----------------------> trig_det_notify_lmp trig_notify_delay_time : %d\n",trig_notify_delay_time);
        OS_RetriggerTimer(&Trigger_Timer_Delay);
        trig_notify_delay_time ++;
    }
}

static int trig_det_notify_delay(u8 notify_trig,u8 notify_status)
{
    u32 delay_time = 0;
    if(notify_status == DEVICE_EVENT_OUT)
    {
        if(AP_GetMenuItem(APPMENUITEM_PARKING_DELAY))
        {
            delay_time = 10 * 1000;
        }
        if(delay_time != 0)
        {
            trig_notify_param.notify_status=notify_status;
            trig_notify_param.notify_trig=notify_trig;
            g_trig_detect.delay_return_trig = notify_trig;
            if(trig_notify_delay_time != 0)
            {
                trig_notify_delay_time = 0;
            }
            else
            {
                OS_CREATETIMER(&Trigger_Timer_Delay, trig_det_notify_lmp, 1000);
            }
        }
        else
        {
            trig_det_notify(notify_trig,notify_status);
        }
    }
    else    //触发器接入时 取消触发器延时
    {
        if(get_delay_return_trig() == notify_trig)
        {
            trig_notify_delay_time = 0;
            OS_DeleteTimer(&Trigger_Timer_Delay);
        }
        if(get_parking_trig_status() == 0)
        {
            trig_det_notify(notify_trig,notify_status);
        }
    }
    return 0;
}

//倒车检测扫描函数
static void trigger_dectect_ctrl()
{
    u8 cur_trig_status = 0;
    
    OS_RetriggerTimer(&Trigger_Timer);
    
    cur_trig_status = get_trigger_det_status();
    if (g_trig_detect.status != cur_trig_status ) 
    {
        if (g_trig_detect.count <= TRIG_FILT_PAR ) 
        {
            g_trig_detect.count++;
        }
    } 
    else 
    {
        if (g_trig_detect.count) 
        {
            g_trig_detect.count = 0;
        }
    }

    if ((g_trig_detect.count > TRIG_FILT_PAR) /*&& get_acc_det_status() == ACC_ON*/) 
    {
        printf("---------------------------> trigger_dectect_ctrl trig_change_state : %x\r\n",get_trig_change_state());
        if(get_trig_change_state())
        {
            return;
        }

        u8 notify_trig = 0;
        u8 notify_status = 0xff;

        printf("---------------------------> cur_trig_status : %d\r\n",cur_trig_status);
        trig_on_change = ~(g_trig_detect.status) & cur_trig_status;
        trig_off_change = ~(cur_trig_status) & g_trig_detect.status;
        printf("---------------------------> trig_on_change : %d\r\n",trig_on_change);
        printf("---------------------------> trig_off_change : %d\r\n",trig_off_change);
        if(trig_off_change)//如果有新断开的触发
        {
            notify_status = DEVICE_EVENT_OUT;
            notify_trig = get_prior_trig(trig_off_change);

            remove_trig_info(trig_off_change);
        }

        if(trig_on_change)//如果有新增加的触发
        {
            if(notify_trig != PARKING_TRIG )
            {
                notify_status = DEVICE_EVENT_IN;
                notify_trig = get_prior_trig(trig_on_change);
            }
            push_trig_info(trig_on_change);
        }
        g_trig_detect.status = cur_trig_status;
        if(notify_status == DEVICE_EVENT_IN
            || (notify_status == DEVICE_EVENT_OUT
                    && ((notify_trig & trig_off_change)
                           || (get_parking_trig_status() != 0
                           && notify_trig != PARKING_TRIG))))
        {
            trig_det_notify_delay(notify_trig,notify_status);//trig_det_notify(notify_trig,notify_status);
        }
    }
}

int get_trigger_count()
{
    return get_trigger_stack_size();
}

//倒车检测扫描注册,由系统调用
void trig_detect_init()
{
    memset(&g_trig_detect, 0x0, sizeof(struct trig_detect_info));
    // g_trig_detect.count = TRIG_FILT_PAR;//在上电倒车情况上加快判断速度
	//trigger_dectect_ctrl(NULL);//?
    trig_det_pin_init();
    init_trigger_stack();
    g_trig_detect.delay_return_trig = TRIGGER_DELAY_END;
	printf("**********************register trig detect**********************\r\n") ; 
	OS_CREATETIMER(&Trigger_Timer, trigger_dectect_ctrl, TRIG_DETECT_TIME);
}



