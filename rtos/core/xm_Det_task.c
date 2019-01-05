#include "rtos.h"
#include "xm_type.h"
#include "types.h"
#include "board_config.h"
#include "gpio.h"
#include "xm_app_menudata.h"
#include "xm_user.h"
//#include "adc.h"

//Battery
#define BATTERY_LVL_0                 0
#define BATTERY_LVL_1                 1
#define BATTERY_LVL_2                 2
#define BATTERY_LVL_3                 3
#define BATTERY_LVL_4                 4
#define BATTERY_LVL_5                 5
#define BATTERY_LVL_6                 6
#define BATTERY_LVL_7                 7
#define BATTERY_LVL_8                 8
#define BATTERY_LVL_9                 9
#define BATTERY_LVL_10                10
#define BATTERY_LVL_11                11
#define LOW_POWER_LVL               BATTERY_LVL_8

#define BATTERY_ADC_0					 945	  // 9v
#define BATTERY_ADC_1					 1008	  // 9.5
#define BATTERY_ADC_2					 1090	  // 10
#define BATTERY_ADC_3					 1212	  // 11
#define BATTERY_ADC_12V                  1309
#define BATTERY_ADC_15V                  1665
#define BATTERY_ADC_18V                  2005
#define BATTERY_ADC_20V                  2245
#define BATTERY_ADC_23V                  2560
#define BATTERY_ADC_24V                  2676
#define BATTERY_ADC_25V                  2815
#define BATTERY_DIFF                     0x01


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


typedef struct tag_battery {
    u16 ad_min_value;
    u16 ad_max_value;
    u8  lvl;   // ��Ӧ�ĵȼ�
} ADC_BATTERY;


extern void MotionDetectStart(void);
extern void XM_BatteryDriverOpen(void);
extern u8 get_acc_state(void);

#define ACC_FILT_PAR    				10
#define ACC_DETECT_TIME     			50
#define DET_ACC_TICKET_EVENT    		0x02
#define DET_BAT_TICKET_EVENT			0x03

#define BATTERY_FILT_PAR    			15//����̫С,��ֹ�ڱ߽��ʱ���󴥷�


#define	BATTERY_VOLTAGE_COUNT	0x10		// ƽ��ֵ

static u8_t ticket_cnt = 0;
static u8 g_acc_status = 0xff;
static struct acc_detect_info g_acc_detect;
static OS_TASK TCB_DetEvent_Task;
#define	XMSYS_DetEvent_TASK_PRIORITY					100		
#define	XMSYS_DetEvent_TASK_STACK_SIZE					(0x400)
__no_init static OS_STACKPTR int DetEventTask[XMSYS_DetEvent_TASK_STACK_SIZE/4];
static OS_TIMER DetEvent_DetTImer;
static OS_MAILBOX det_mailbox;		// һ�ֽ����䡣
static char det_mailbuf;			// һ���ֽڵ�����

// ��ص�ѹ���
static unsigned int battery_voltage_value[BATTERY_VOLTAGE_COUNT];	// �����, �����Ѳɼ��ĵ�ص�ѹֵ����
static unsigned int battery_voltage_index;			// ������е�һ����Ч����������
static unsigned int battery_voltage_count;			// �Ѳɼ��ĵ�ص�ѹֵ��������
static volatile u8 battery_lvl=0xff;		
static volatile u8 battery_count=0x00;
static OS_TIMER m_notify_low_power_timer;
static OS_TIMER m_shutdown_timer;
static u8_t lowbattery_create_timer_flag = FALSE;

static const ADC_BATTERY adc_battery[] = {
    {0,                 BATTERY_ADC_0 ,     BATTERY_LVL_0},
    {BATTERY_ADC_0 ,    BATTERY_ADC_1 ,     BATTERY_LVL_1},
    {BATTERY_ADC_1 ,    BATTERY_ADC_2 ,     BATTERY_LVL_2},
    {BATTERY_ADC_2 ,    BATTERY_ADC_3 ,     BATTERY_LVL_3},
    {BATTERY_ADC_3 ,    BATTERY_ADC_12V ,   BATTERY_LVL_4},//12λADC
    {BATTERY_ADC_12V ,  BATTERY_ADC_15V ,   BATTERY_LVL_5},//12λADC
    {BATTERY_ADC_15V ,  BATTERY_ADC_18V ,   BATTERY_LVL_6},//12λADC
    {BATTERY_ADC_18V ,  BATTERY_ADC_20V ,   BATTERY_LVL_7},
    {BATTERY_ADC_20V ,  BATTERY_ADC_23V ,   BATTERY_LVL_8},
    {BATTERY_ADC_23V ,  BATTERY_ADC_24V ,   BATTERY_LVL_9},
    {BATTERY_ADC_24V ,  BATTERY_ADC_25V ,   BATTERY_LVL_10},
    {BATTERY_ADC_25V ,  0xfff ,             BATTERY_LVL_11},
};



u8_t battery_get_lvl(unsigned int batteryvalue)
{
    unsigned char i;

    for(i=0; i<(sizeof(adc_battery)/(sizeof(adc_battery[0]))); i++)
    {
		if( (batteryvalue>=adc_battery[i].ad_min_value) && (batteryvalue<adc_battery[i].ad_max_value))
		{
			return i;
		}
    }
	return 0;
}


static void shutdown_timer_fun()
{
    OS_DeleteTimer(&m_shutdown_timer);
    if(AP_GetMenuItem(APPMENUITEM_PARKMONITOR) == MOT_ON && get_acc_state() == ACC_OFF)
    {
        xm_board_power_ctl(OFF);
    }
}

static void notify_low_power()
{
    printf("--------------------------> notify_low_power \n");
    static u8 notify_count = 0;
	
    if( (AP_GetMenuItem(APPMENUITEM_PARKMONITOR) == MOT_ON) && (get_acc_state() == ACCOFF_TO_MOTOPEN) )
    {
        if(notify_count == 0 )
        {
			//���ж��Ƿ���¼��״̬
			notify_count ++ ;
        }
        else
        {
            printf("--------------------------> xm_board_power_ctl off \n");
            notify_count = 0;
            xm_board_power_ctl(OFF);
        }
    }
    else
    {
        notify_count = 0;
    }
	OS_RetriggerTimer(&m_notify_low_power_timer);
}


void reset_battery_dec_state(void)
{
    battery_lvl = 0xff;
}


//��ȡACC״̬
u8 get_acc_state(void)
{
    return g_acc_status;
}

//����ACC״̬
void set_acc_state(u8 value)
{
	g_acc_status =  value;
}


//ACC�Ͽ�5s��,�رյ�Դ
static void acc_timer_out_ctrl()
{
    static u8 i=0;
    static int notify_count = 0;
	
    XM_printf(">>>>>acc_timer_out_ctrl over \r\n");
    OS_DeleteTimer(&g_acc_detect.ACC_TimerOut); 

	if( (get_acc_det_pin_status()==ACC_ON) && (get_acc_state()==ACCOFF_TO_DELAY) )
	{//Ԥ��ACC ON�������ж���
		set_acc_state(ACCOFF_TO_ACCON);
	}
	else
	{
		set_acc_state(ACCON_TO_ACCOFF);
		HW_LCD_BackLightOff();
		AP_PostSystemEvent(SYSTEM_EVENT_STOP_REC);//ֹͣ¼��,desk�����ж�
	}
}

//acc״̬�ı�֪ͨ
static void acc_det_notify(u8 acc_status)
{
    switch(acc_status)
    {
        case DEVICE_EVENT_IN:
            XM_printf("------------------------->HX_ACC  DEVICE_EVENT_IN \r\n");
			if(get_acc_state()==ACCOFF_TO_MOTOPEN)
			{//��Ҫ����¼��,accoff--->accon
				XM_printf(">>>>>start rec......\r\n");
				HW_LCD_BackLightOn();
				set_acc_state(POWERON_TO_ACCON);
				AP_PostSystemEvent(SYSTEM_EVENT_START_REC);
			}
            break; 

        case DEVICE_EVENT_OUT:
            XM_printf("------------------------->HX_ACC  DEVICE_EVENT_OUT \r\n");
			set_acc_state(ACCOFF_TO_DELAY);
            reset_battery_dec_state();  //���õ������״̬
            OS_CREATETIMER(&g_acc_detect.ACC_TimerOut, acc_timer_out_ctrl, 3000);//3000MS,���һ��
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
    int i;
    int sum;
	u8_t cur_battery_lvl;
	
	while(1) 
	{
        Det_event = OS_WaitEvent(DET_ACC_TICKET_EVENT|DET_BAT_TICKET_EVENT);

        if(Det_event & DET_ACC_TICKET_EVENT)
        {//�����Լ��ACC
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

		//�ƶ�����״̬��,�������ݴ���
		if( (Det_event & DET_BAT_TICKET_EVENT) && (AP_GetMenuItem(APPMENUITEM_PARKMONITOR) == MOT_ON) )
		{//�����Լ�����
			if(XM_BatteryDriverPoll())
			{
				unsigned int batteryvalue;
				if(XM_BatteryDriverGetEvent(&batteryvalue))
				{
					//XM_printf(">>>>batteryvalue:%d\r\n", batteryvalue);
					battery_voltage_value[battery_voltage_index] = batteryvalue;
					battery_voltage_index++;
					if(battery_voltage_index>=BATTERY_VOLTAGE_COUNT)
					{
						battery_voltage_index = 0;
					    sum = 0;
					    for(i= 0; i< BATTERY_VOLTAGE_COUNT; i++)
						{
					        sum += battery_voltage_value[i];
							battery_voltage_value[i] = 0;
					    }
					    sum = sum / BATTERY_VOLTAGE_COUNT;
						//XM_printf(">>>>sum:%d\r\n\r\n", sum);
					}
			        cur_battery_lvl = battery_get_lvl(sum);
			        if(battery_lvl != cur_battery_lvl)
			        {
			           if(battery_count <= BATTERY_FILT_PAR + 1)
			           {
			               battery_count ++;
			           }
			        }
			        else
			        {
			           if(battery_count)
			           {
			              battery_count = 0;
			           }
			        }

			        if( battery_count > BATTERY_FILT_PAR )
			        {
						XM_printf("cur_battery_lvl %d \r\n",cur_battery_lvl);
						battery_lvl = cur_battery_lvl;
						if(battery_lvl <= LOW_POWER_LVL)//20V-23V�����¶���Ϊ�͵���
						{
							printf(">>>>>>low power\r\n");
							AP_PostSystemEvent(SYSTEM_EVENT_ONLY_STOP_REC);//ֹͣ¼��,desk�����ж�
							if( (AP_GetMenuItem(APPMENUITEM_PARKMONITOR) == MOT_ON) && (get_acc_state() == ACCOFF_TO_MOTOPEN) && (lowbattery_create_timer_flag==FALSE) )
							{
								lowbattery_create_timer_flag = TRUE;
								OS_CREATETIMER(&m_notify_low_power_timer, notify_low_power, 3 * 1000);
							}
						}
			        }
				}
			}
			else
			{
				XM_printf(">>>>>battery buffer over.........\r\n");
			}
		}

		
		if(OS_GetMailTimed(&det_mailbox, &response_code, 2) == 0)
		{
			XM_printf(">>>>DetEvent_task response_code:%d\r\n", response_code);
		    XM_printf("-------------------> AP_GetMenuItem(APPMENUITEM_PARKMONITOR) : %d\n",AP_GetMenuItem(APPMENUITEM_PARKMONITOR));
			
			if(response_code==0)
			{//ACCON-->ACCOFF,ֹͣ¼��󣬸����ƶ������д���
				if(AP_GetMenuItem(APPMENUITEM_PARKMONITOR) == MOT_OFF )
			    {//�ƶ�����
			    	XM_printf(">>>>close main power.......\r\n");
					set_acc_state(ACCOFF_TO_MOTCLOSE);
					xm_board_power_ctl(OFF);
			    }
				else
				{//�ƶ���⿪
					//motion�������ƶ�����
					if(IsMotionDetectStarted() == FALSE)
					{
						MotionDetectStart();
					}
					set_acc_state(ACCOFF_TO_MOTOPEN);
				}
			}
			else if(response_code==1)
			{//ACCOFF--->ACCON������¼���,��Ҫ�ص��ƶ������
				if(IsMotionDetectStarted() == TRUE)
				{
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
	OS_ClearMB (&det_mailbox);//��ֹ����
	OS_PutMail(&det_mailbox, &ret);
}

void DetEvent_TicketCallback(void)
{
	ticket_cnt++;
    OS_SignalEvent(DET_ACC_TICKET_EVENT, &TCB_DetEvent_Task); /* ֪ͨ�¼� */
	if(ticket_cnt>3)
	{
		ticket_cnt = 0;
    	OS_SignalEvent(DET_BAT_TICKET_EVENT, &TCB_DetEvent_Task); /* ֪ͨ�¼� */
	}
	OS_RetriggerTimer(&DetEvent_DetTImer);
}


void xm_battery_det_init(void)
{
	XM_BatteryDriverOpen();

	// ��ؼ��
	battery_voltage_count = 0;
	battery_voltage_index = 0;
	battery_voltage_index = battery_voltage_index;//compile warning
	battery_voltage_count = battery_voltage_count;//compile warning
	memset (battery_voltage_value, 0, sizeof(battery_voltage_value));
}


void XMSYS_DetEventInit (void)
{
	XM_printf(">>>>>>XMSYS_DetEventInit........\r\n");
	
    xm_board_power_ctl(ON);//����ʱ,��ʱ������ĸߵ�ƽֻ��1.5V����,�����쳣

	set_acc_state(POWERON_TO_ACCON);
	acc_ctrl_clear();//���Ʋ�����0
	acc_det_pin_init();
	xm_battery_det_init();
	
	OS_CREATEMB(&det_mailbox, 1, 1, &det_mailbuf);	
	OS_CREATETIMER(&DetEvent_DetTImer, DetEvent_TicketCallback, 100);//50ms����ʱ�����¼�
	OS_CREATETASK(&TCB_DetEvent_Task, "DetEvent", DetEvent_task, XMSYS_DetEvent_TASK_PRIORITY, DetEventTask);
}



