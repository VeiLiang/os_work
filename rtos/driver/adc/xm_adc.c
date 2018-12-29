/*
 * ADC.c
 *
 *  Created on: 2010-10-15
 *  Author: Administrator
 */
#include "hardware.h"
#include "adc.h"
#include "gpio.h"
#include "xm_key.h"
#include "xm_dev.h"
#include "xm_base.h"
#include  "lcd.h"
#include <xm_user.h>
#include "xm_app_menudata.h"

#define KEY_DEBUG

#define	ADC_REF_3300MV


extern u8 get_acc_state(void);
extern void xm_board_power_ctl(u8 state);
extern VOID AP_PostSystemEvent (unsigned int event);



// 1) 老的ASIC测试版(176封装)使用AUX1作为按键,
// 2) 产品DEMO版(128封装)使用AUX0作为按键, AUX1作为电池电压检测

#define ADC_NULL        0xffffffff          // 无按键
#define KEY_SHORT_TIME  200          // 短按时间阈值,大于此值时说明已经不属于短按了
static OS_TIMER Buzzer_Timer;

#if HONGJING_CVBS
// 宏景项目使用ADC检测ACC电压值来判断是否ACC关机, 需要极短时间内正确判断ACC掉电, 然后系统关机.
#define	BATTERY_VOLTAGE_COUNT	0x4		// 平均值
#else
#define	BATTERY_VOLTAGE_COUNT	0x10		// 平均值
#endif

#ifdef ADC_REF_3300MV
// 3.3v adc
#define	BATTERY_VOLTAGE_REF		3300		// 参考电压
#else
// 3.0v adc
#define	BATTERY_VOLTAGE_REF		3000		// 参考电压
#endif

#define	MAX_ADC_DEVICE		2		// AUX0, AUX1

// 按键状态机定义
#define	FSM_SCAN				   1			// scan state
#define	FSM_CHECK				2			// check state
#define	FSM_DELAY				3			// repeat delay state
#define	FSM_REPEAT				4			// repeat state

#define	ADC_MODE_KEY			1			// 按键检测模式
													//		按键电压测量时,需要切换到3.3v的参考电压(该参考电压跟随VCC变化而变化, 因此保证测量的按键值一致)
#define	ADC_MODE_BAT			2			// 电压检测模式
													//		电池电压测量时,需要切换到2.0v的参考电压(该参考电压不随温度/VCC变化而变化)
#define VOLDET_KEY_LVL_0                 0
#define VOLDET_KEY_LVL_1                 1
#define VOLDET_KEY_LVL_2                 2
#define VOLDET_KEY_LVL_3                 3
#define VOLDET_KEY_LVL_4                 4
#define VOLDET_KEY_LVL_5                 5
#define VOLDET_KEY_LVL_6                 6

#define KEY_ADC_LEVEL_0					 0x0   //power
#define KEY_ADC_LEVEL_1					 0x3BC //AV
#define KEY_ADC_LEVEL_2					 0x950 //down
#define KEY_ADC_LEVEL_3					 0x716 //Menu
#define KEY_ADC_LEVEL_4					 0xB68 //UP
#define KEY_ADC_LEVEL_5					 0xD22 //OK
#define KEY_ADC_LEVEL_6					 0
#define	DIFF	0x100			// 按键AD采样抖动值

typedef struct tagADC_KEY {
    u16 ad_min_value;
    u16 ad_max_value;
    u8  key;
} ADC_KEY;


//按键板
typedef struct
{
    u16               	m_ad_val_min;       ///< 有效按键的最小值
    u16               	m_ad_val_max;       ///< 有效按键的最大值
    ADC_KEY*      		m_adkey_val_array;  ///< 指向AD按键的映身表
    u8      			m_adkey_val_array_length;  ///< 指向AD按键的映身表
}KEYPAD_PARAM;


//通用6key按键板值
static ADC_KEY keyboard_6key[] = {
    {0x0, 		   0x0   +DIFF, VK_AP_POWER},//power
    {0x3BC - DIFF, 0x3BC +DIFF, VK_AP_FONT_BACK_SWITCH},//OK
    {0x950 - DIFF, 0x950 +DIFF, VK_AP_DOWN},//down
    {0x716 - DIFF, 0x716 +DIFF, VK_AP_MENU},//Menu
    {0xB68 - DIFF, 0xB68 +DIFF, VK_AP_UP},//UP
    {0xD22 - DIFF, 0xD22 +DIFF, VK_AP_SWITCH},//av
};


typedef struct tagKEY_MOD {
	unsigned int 	key;	// 对应的按键
	unsigned int	mode;
}KEY_MOD;


//KEY有效状态
static const KEY_MOD keyboard_vaild[] = {
        {VK_AP_POWER,				0},
		{VK_AP_FONT_BACK_SWITCH,	0},
		{VK_AP_DOWN,				XMKEY_REPEAT},
		{VK_AP_MENU,		    	0},
		{VK_AP_UP,		        	XMKEY_REPEAT},
		{VK_AP_SWITCH,		        XMKEY_LONGTIME},
};

static const KEYPAD_PARAM Keypad_6KEY_Param = 
{
	0x0,
	0xD22 +DIFF,

	keyboard_6key,
	(sizeof(keyboard_6key)/sizeof(keyboard_6key[0])),
};


typedef struct tag_battery {
    u16 ad_min_value;
    u16 ad_max_value;
    u8  lvl;   // 对应的等级
} ADC_BATTERY;


static const ADC_BATTERY    adc_battery[] = {
    {0,                 BATTERY_ADC_0 ,     BATTERY_LVL_0},
    {BATTERY_ADC_0 ,    BATTERY_ADC_1 ,     BATTERY_LVL_1},
    {BATTERY_ADC_1 ,    BATTERY_ADC_2 ,     BATTERY_LVL_2},
    {BATTERY_ADC_2 ,    BATTERY_ADC_3 ,     BATTERY_LVL_3},
    {BATTERY_ADC_3 ,    BATTERY_ADC_12V ,   BATTERY_LVL_4},//12位ADC
    {BATTERY_ADC_12V ,  BATTERY_ADC_15V ,   BATTERY_LVL_5},//12位ADC
    {BATTERY_ADC_15V ,  BATTERY_ADC_18V ,   BATTERY_LVL_6},//12位ADC
    {BATTERY_ADC_18V ,  BATTERY_ADC_20V ,   BATTERY_LVL_7},
    {BATTERY_ADC_20V ,  BATTERY_ADC_23V ,   BATTERY_LVL_8},
    {BATTERY_ADC_23V ,  0xfff ,             BATTERY_LVL_9},


};

static volatile unsigned int last_key;		// 最近按下的键值, 判断是否重复按键,相同按键
														//	0xFFFFFFFF 表示没有按键按下
static volatile unsigned int short_key_time;

static volatile unsigned int last_key_ticket;		// 最近按下键的时间
static volatile u8 battery_lvl=0xff;		//
static volatile u8 battery_count=0x00;
#define BATTERY_FILT_PAR    	15//不能太小,防止在边界的时候误触发


unsigned int buzz_time;

// 电池电压检测
static unsigned int battery_voltage_value[BATTERY_VOLTAGE_COUNT];	// 缓冲池, 保存已采集的电池电压值样本
static unsigned int battery_voltage_index;			// 缓冲池中第一个有效样本的索引
static unsigned int battery_voltage_count;			// 已采集的电池电压值样本个数

//static int adc_mode;			// 按键测量模式/电压检测模式


static void (*adc_callback[MAX_ADC_DEVICE])(unsigned int adc_sample_value);


static void set_debounce_time (UINT32 debounce_time) // 削抖时间, 微秒
{
	UINT32 dbcount;
	//dbcount = arkn141_get_clks (ARKN141_CLK_APB) * (debounce_time * 1000) / (1000000);

	//dbcount =  (debounce_time / 1000000) / (1/arkn141_get_clks (ARKN141_CLK_APB));
	dbcount =  (debounce_time * arkn141_get_clks (ARKN141_CLK_APB) / 1000000) ;

	if(dbcount >= 0xFFFFF)
		dbcount = 0xFFFFF;
	rADC_DBNCNT = dbcount;
}

static void set_transform_interval (UINT32 interval_millisecond) //设置间隔时间(毫秒)
{
	UINT32 reg;

	// 1M Hz sample clock
	// adc_clk = clk_24m/((adc_clk_div+1)*2).
	reg = rSYS_DEVICE_CLK_CFG1;
	reg &= ~0x7FFF;
	reg |= (12 - 1);
	rSYS_DEVICE_CLK_CFG1 = reg;

	reg = interval_millisecond * 1000;		// 转换为微秒计数
	if(reg > 0xFFFF)
		reg = 0xFFFF;
	rADC_DETINTER = reg;
}

UINT32 ADCValue_show = 0;
UINT32 ADCValue_Data = 0;
unsigned short key_Mode = 0;
UINT32 battery_data = 0;

UINT16 KeyADC_GetKey(void)
{
    unsigned char i;
	
	//get adc
	ADCValue_Data = rADC_AUX1; 	// 读取ADC采样值
	rADC_STA &= ~AUX1_VALUE_INT;		// 清中断
	
  	//XM_printf(">>>>KeyADC_GetKey, ADCValue :0x%x\r\n",ADCValue_Data);
	if( (ADCValue_Data >= Keypad_6KEY_Param.m_ad_val_min) && (ADCValue_Data<=Keypad_6KEY_Param.m_ad_val_max) )
	{
	    for(i=0; i<Keypad_6KEY_Param.m_adkey_val_array_length; i++)
	    {
			if((ADCValue_Data>=Keypad_6KEY_Param.m_adkey_val_array[i].ad_min_value)&&(ADCValue_Data<=Keypad_6KEY_Param.m_adkey_val_array[i].ad_max_value))
			{
				return Keypad_6KEY_Param.m_adkey_val_array[i].key;
			}
	    }
	}

	return KEY_NULL;
}

//返回电量等级
u8 battery_get_lvl(void)
{
    unsigned char i;
    battery_data = rADC_AUX0;// 读取ADC采样值
    rADC_STA &= ~AUX0_VALUE_INT;// 清中断
  //  printf("battery_data........ %d \n",battery_data);

    for(i=0;i<(sizeof(adc_battery)/(sizeof(adc_battery[0])));i++)
    {
          if((battery_data>=adc_battery[i].ad_min_value)&&(battery_data<=adc_battery[i].ad_max_value))
          {
              return i;

          }

    }
     return 0;

}

void Buzz_Work()
{
#if 0
	unsigned int val;
	//XM_lock();
	val = rSYS_PAD_CTRL03;
	val &= ~(0x07 << 0);
	rSYS_PAD_CTRL03 = val;
	SetGPIOPadDirection (GPIO30, euOutputPad);
	//XM_unlock();
	buzz_time = OSTimeGet();
	//XM_lock ();	
	SetGPIOPadData (GPIO30, euDataHigh);
	//XM_unlock();
#endif	
}


//按键重映射
static int key_remap(volatile unsigned int * keyvalue, unsigned short * keymode)
{
    if(* keyvalue == VK_AP_DOWN)
    {
        if(* keymode==XMKEY_REPEAT)
        {
            * keymode=XMKEY_PRESSED;
           
        }
    }
    else if(* keyvalue == VK_AP_UP)
    {
        if(* keymode==XMKEY_REPEAT)
        {
            * keymode=XMKEY_PRESSED;
           
        }
    }
 
    return 0;
}


extern BOOL Close_Audio_Sound;
static OS_TIMER m_notify_low_power_timer;
static OS_TIMER m_shutdown_timer;

static void shutdown_timer_fun()
{
    OS_DeleteTimer(&m_shutdown_timer);
    if(AP_GetMenuItem(APPMENUITEM_PARKMONITOR) == MOT_ON && get_acc_det_pin_status() == ACC_OFF)
    {
        xm_board_power_ctl(OFF);
    }
}

static void notify_low_power()
{
    printf("--------------------------> notify_low_power \n");
    static u8 notify_count = 0;
    if(AP_GetMenuItem(APPMENUITEM_PARKMONITOR) == MOT_ON && get_acc_det_pin_status() == ACC_OFF)
    {
        if(notify_count == 0 )
        {
            OS_RetriggerTimer(&m_notify_low_power_timer);
            AP_PostSystemEvent (SYSTEM_EVENT_MAIN_BATTERY);
            //AP_PostSystemEvent (SYSTEM_EVENT_ONE_KEY_PROTECT);
            notify_count ++ ;
        }
        else if(notify_count == 1)
        {
            //OS_DeleteTimer(&m_notify_low_power_timer);
            printf("--------------------------> send SYSTEM_EVENT_SHUTDOWN_SOON \n");
            AP_PostSystemEvent (SYSTEM_EVENT_SHUTDOWN_SOON);
            OS_RetriggerTimer(&m_notify_low_power_timer);
            notify_count ++ ;
            //OS_CreateTimer(&m_shutdown_timer,shutdown_timer_fun,2000);
        }
        else
        {
            printf("--------------------------> xm_board_power_ctl off \n");
            notify_count = 0;
            OS_DeleteTimer(&m_notify_low_power_timer);
            xm_board_power_ctl(OFF);
        }
    }
    else
    {
        notify_count = 0;
    }
}


void reset_battery_dec_state(void)
{
    battery_lvl = 0xff;
}


void adc_int_handler(void)
{
	UINT32 adcStatus;
	UINT32 ADCValue;
	static int delay_time = 0;
	static u8 longkeyflg = FALSE;
	static u8 cur_battery_lvl;
	static u8 checktimes = 0;
	
	adcStatus = rADC_STA;

	// AUX0 (BAT)
	if(adcStatus & AUX0_START_INT)
	{
		rADC_STA &= ~AUX0_START_INT;
	}
	if(adcStatus & AUX0_VALUE_INT)
	{
        cur_battery_lvl= battery_get_lvl();
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
           printf("cur_battery_lvl %d \r\n",cur_battery_lvl);
           battery_lvl = cur_battery_lvl;
            if(battery_lvl <= LOW_POWER_LVL)
            {
                printf("low power\n");//关背光
                
                if(AP_GetMenuItem(APPMENUITEM_PARKMONITOR) == MOT_ON && get_acc_det_pin_status() == ACC_OFF)
                {
                    OS_CREATETIMER(&m_notify_low_power_timer,notify_low_power,5 * 1000);
                }
           }
        }

    	#if  0
        #if 0
       	if(ADCValue <(BATTERY_ADC_LEVEL_1 + BATTERY_DIFF))
            battery_voltage_count = VOLDET_BATTERY_LVL_1;
        else if(ADCValue <(BATTERY_ADC_LEVEL_2 + BATTERY_DIFF))
            battery_voltage_count = VOLDET_BATTERY_LVL_2;
        else if(ADCValue <(BATTERY_ADC_LEVEL_3 + BATTERY_DIFF))
            battery_voltage_count = VOLDET_BATTERY_LVL_3;
        else
            battery_voltage_count = VOLDET_BATTERY_LVL_4;
        #endif
        #if 1
		if(battery_voltage_count < BATTERY_VOLTAGE_COUNT)
		{
			// 采集样本未满
			printf("..........%s %d \n",__FUNCTION__,__LINE__);
			battery_voltage_value[battery_voltage_count] = ADCValue;
			battery_voltage_count ++;
		}
		else
		{
			// 采集样本已满
			battery_voltage_value[battery_voltage_index] = ADCValue;
			battery_voltage_index ++;
			if(battery_voltage_index >= BATTERY_VOLTAGE_COUNT)
				battery_voltage_index = 0;
		}
		#endif
	
	     #endif
	
	}
	if(adcStatus & AUX0_STOP_INT)
	{
		rADC_STA &= ~AUX0_STOP_INT;
	}

	// AUX1 (KEY)
	if(adcStatus & AUX1_START_INT)
	{
		rADC_STA &= ~AUX1_START_INT;
		// 关闭AUX0. ADC是互斥资源, 只允许一个AUX使用.
		// 当AUX1有需要转换的数据时(按键电压值转换), 将AUX0关闭, 分配ADC资源给AUX1使用.
		// AUX1转换完毕后(STOP_INT), 重新使能AUX0
		rADC_CTR &= ~(1 << AUX0_CHANNEL);
	}
	
	if(adcStatus & AUX1_VALUE_INT)
	{
		int i;
		UINT32 key_value = KeyADC_GetKey();
		unsigned int key_mode;

		//短按,抬起有效; 长按:到达长按时间发送一次消息; 重复按:重复执行
		//XM_printf(">>>>>>>>>>>key_value:%x\r\n", key_value);
		if(last_key == 0xFFFFFFFF) 
		{
			if(key_value!=KEY_NULL)
			{
				checktimes++;
			}
			if(checktimes>1)
			{
				checktimes = 0;
				last_key = key_value;
				last_key_ticket = XM_GetTickCount();
			}
		}
		else 
		{
			delay_time = XM_GetTickCount() - last_key_ticket;
			//XM_printf(">>>>2 delay_time..........%d\r\n", delay_time);

			for(i=0; i<sizeof(keyboard_vaild)/sizeof(keyboard_vaild[0]); i++)
			{
				if(key_value==keyboard_vaild[i].key)
				{
					key_mode = keyboard_vaild[i].mode;
					break;
				}
			}
			
			if(key_mode & XMKEY_REPEAT)
			{
				if(delay_time > 300) 
				{
					last_key_ticket = XM_GetTickCount();
					key_Mode = XMKEY_REPEAT;
					XM_printf(">>>>>>>>>reapte key:%x, status:%x\r\n", last_key, key_Mode);
					XM_KeyEventProc(last_key, key_Mode);
				}
			}
			else if(key_mode & XMKEY_LONGTIME) 
			{
				if((delay_time > 1000)&&(!longkeyflg)) 
				{
					longkeyflg = TRUE;
					last_key_ticket = XM_GetTickCount();
					key_Mode = XMKEY_LONGTIME;
					XM_printf(">>>>>>>>>long key:%x, status:%x\r\n", last_key, key_Mode);
					XM_KeyEventProc(last_key, key_Mode);
				}
			}
		}
	}
	
	if(adcStatus & AUX1_STOP_INT)
	{
		#if 0
		// 按键释放
		XM_printf("delay_time..........%d \n",short_key_time);
        if(fisrt==1)
        {
            fisrt=0;  //释放后,允许短按重新计数     
        }

		rADC_STA &= ~AUX1_STOP_INT;
		if(last_key != 0xFFFFFFFF) 
		{
		
            XM_printf(">>>>>>>>>key:%x, status:%x\r\n", last_key, key_Mode);
            key_Mode = XMKEY_PRESSED;
            key_remap(&last_key,&key_Mode);
            
			if(short_key_time<KEY_SHORT_TIME)
                XM_KeyEventProc (last_key, key_Mode);
            else
                XM_KeyEventProc (KEY_NULL, key_Mode);
                
            last_key = 0xFFFFFFFF;

			#if 0
	    	#ifdef BUZZER_EN
			// buzzer_init();
			buzzer_onoff(ON);

			OS_CREATETIMER(&Buzzer_Timer, buzzer_off, 20);//
			// OS_Delay(30); //调用这个会打印调试错误,在中断中调用延时会异常,延时也属于中断?
        	#endif
			#endif
        }
		#endif
        
		rADC_STA &= ~AUX1_STOP_INT;
		if(last_key != 0xFFFFFFFF) 
		{
			//XM_printf(">>>>1 delay_time..........%d\r\n", delay_time);
			if(delay_time < 200)
			{
            	key_Mode = XMKEY_PRESSED;
				XM_printf(">>>>>>>>>short key:%x, status:%x\r\n", last_key, key_Mode);
				XM_KeyEventProc(last_key, key_Mode);
			}
            last_key = 0xFFFFFFFF;
			delay_time = 0;
        }
		longkeyflg = FALSE;
		key_Mode = 0;
		checktimes = 0;
		// 使用完毕后, 重新允许AUX0. AUX0可以继续采样电压值.
		rADC_CTR |= (1 << AUX0_CHANNEL);
	}
}

short int XM_GetKeyState (int vKey)
{
	if(last_key != 0xFFFFFFFF && last_key == vKey)
		return 0x8000;
	else
		return 0;
}

// 获取电池的当前电压(毫伏)
u8_t XM_GetBatteryVoltage (void)
{
	#if 1
    int i;
    int  sum = 0;
    for(i= 0; i< BATTERY_VOLTAGE_COUNT; i++){
        sum += battery_voltage_value[i];
		battery_voltage_value[i] = 0;
    }
    sum = sum / BATTERY_VOLTAGE_COUNT;
	if(sum > 2000)
		return 1;
    return 0;
	#endif

    #if 0
	unsigned int sum = 0;
	unsigned int i;

    return 3100* 3;
	irq_disable (ADC_INT);
	for (i = 0; i < battery_voltage_count; i ++)
	{
		sum += battery_voltage_value[i];
	}
	irq_enable (ADC_INT);

	if(battery_voltage_count == 0)
		return BATTERY_VOLTAGE_REF * 3;

	sum = sum * BATTERY_VOLTAGE_REF / (battery_voltage_count * 4096);		// 12bit adc
	return sum * 3;		// 200K/100K
    #endif
}


void xm_adc_init(void)
{
	sys_clk_disable (adc_pclk_enable);
	sys_clk_disable (adc_clk_enable);
	delay (1000);
	sys_soft_reset (softreset_adc);
	delay (1000);
	sys_clk_enable (adc_pclk_enable);
	sys_clk_enable (adc_clk_enable);
	delay (1000);

	//adc_mode = ADC_MODE_KEY;

	// 电池电压测量时,需要切换到2.0v的参考电压(该参考电压不随温度/VCC变化而变化)
	// 按键电压测量时,需要切换到3.3v的参考电压(该参考电压跟随VCC变化而变化, 因此保证测量的按键值一致)
	rSYS_ANALOG_REG0 &= ~(1 << 22); // ref : 3.3v
	rSYS_ANALOG_REG0 |=  (1 << 21);
	//rSYS_ANALOG_REG0  |= (1 << 22);	// ref : 2.0v

	//reset adc module
	rADC_CTR |= 1<<0;
	delay (100);
	rADC_CTR  = 0;

	// debounce time (APB clock)
	// KEY 去抖动时间为100us
	set_debounce_time (100);

#if HONGJING_CVBS
	set_transform_interval (4);	// 电压检测需要较快的速度来检测ACC掉电, 理论上16毫秒可以判定
#else
	set_transform_interval (16);	// 每16ms采样一次, 设置为较大值(如64ms时), 按键不灵敏, 经常丢键. 改为16ms后按键很灵敏
#endif
	//set_transform_interval (128);
	// AUX0 --> BAT
	// AUX1 --> KEY
	//register irq
	// 中断使能 (AUX0, AUX1)
	rADC_IMR = 	(0x07 << 0)		// AUX0 (BAT)
				|	(0x07 << 3)		// AUX1 (KEY)
				 ;

	// 按键
	last_key = 0xFFFFFFFF;

	// 电池检测
	battery_voltage_count = 0;
	battery_voltage_index = 0;
	battery_voltage_index = battery_voltage_index;//compile warning
	battery_voltage_count = battery_voltage_count;//compile warning
	memset (battery_voltage_value, 0, sizeof(battery_voltage_value));

	rADC_CTR = 0;
	// 清除所有中断
	rADC_STA = 0;		// bit clear清除
	request_irq (ADC_INT, PRIORITY_FIFTEEN, adc_int_handler);

	//set detect level
#if  0// sch  default is low , key press is high
	rSYS_ANALOG_REG1 |= (1 << 14); // 1: connect a pull-down resister (0.3v-3.3v have interrupt)
#else// sch  default is high , key press is low
 	rSYS_ANALOG_REG1 &= ~(1 << 14);  //   0: connect a pull-up resister  (3.0v - 0v have interrupt)
 #endif
	rADC_CTR |= 1<<8;      // 1: aux0_det high valid.
	rADC_CTR |= 1<<9;    // 1: aux1_det high valid.


#ifdef KEY_USE_AUX0
	Enable_ADC_Channel (AUX0_CHANNEL);	// 按键

#if CZS_USB_01
#else
	Enable_ADC_Channel (AUX1_CHANNEL);	// 电压检测
#endif

#else

#if HONGJING_CVBS
	//Enable_ADC_Channel (AUX0_CHANNEL);	// 电压检测
#else
	Enable_ADC_Channel (AUX1_CHANNEL);	// 按键
	Enable_ADC_Channel (AUX0_CHANNEL);	// 电压检测
#endif

#endif

	//printk("xm_adc_init is finished\n");

}


static void Enable_ADC_Channel(UINT32 channel)
{
	XM_lock ();
#ifdef KEY_USE_AUX0

	switch(channel)
	{
		case AUX1_CHANNEL:
			// Bat 电池电压检测
			rADC_CTR |= (1 << AUX1_CHANNEL);
			rADC_STA &= ~(0x07 << 3);			// 清除中断状态
			//rADC_IMR &= ~((1<<5)|(1<<4));		// 使用stop, value 2个中断源
			rADC_IMR &= ~(1 << 5);		// 仅使用value中断源
			break;

		case AUX0_CHANNEL:
			// Key 按键
			rADC_CTR |= (1 << AUX0_CHANNEL);
			rADC_STA &= ~(0x07 << 0);	//  清除中断状态
			rADC_IMR &= ~((1<<1)|(1<<2));		// 使用stop, value 2个中断源
			break;

		default:
			break;
	}

#else

	switch(channel)
	{
		case AUX0_CHANNEL:		// 电池电压检测
			rADC_STA &= ~((1<<2)|(1<<1)|(1<<0));		// 清除中断状态
			rADC_IMR &= ~((1<<2)|(1<<1)|(1<<0));
			//rADC_IMR |= (1 << 1) | (1 << 0);		// mask start/stop interrupt
			rADC_CTR |= (1 << AUX0_CHANNEL);		// 使能
			break;

		case AUX1_CHANNEL:		// 按键
			rADC_CTR |= (1<<AUX1_CHANNEL);
			rADC_IMR &=~((1<<5)|(1<<4)|(1<<3));
			//rADC_IMR &= ~((1<<5)|(1<<4));		// 不使用START
			break;

		default:
			break;
	}

#endif
	XM_unlock();
}

static void Disable_ADC_Channel(UINT32 channel)
{
	XM_lock ();
#ifdef KEY_USE_AUX0

	switch(channel)
	{
		case AUX0_CHANNEL:
			rADC_CTR |= (1<<8);
			rADC_CTR &= ~(1 << AUX0_CHANNEL);
			rADC_STA = 0;
			rADC_IMR |= ((1<<2)|(1<<1)|(1<<0));		// 禁止3个中断源
			break;

		case AUX1_CHANNEL:
			rADC_CTR |= (1<<9);
			rADC_CTR &= ~(1 << AUX1_CHANNEL);
			rADC_IMR |= ((1<<5)|(1<<4)|(1<<3));		// 禁止3个中断源
			break;

		default:
			break;
	}

#else
	switch(channel)
	{
		case AUX0_CHANNEL:
			rADC_CTR |= (1<<8);
			rADC_CTR |= (1<<AUX0_CHANNEL);
			rADC_STA=0;
			rADC_IMR &=~((1<<2)|(1<<1)|(1<<0));
			break;

		case AUX1_CHANNEL:
			rADC_CTR |= (1<<9)|(1<<AUX1_CHANNEL);
			rADC_IMR &=~((1<<5)|(1<<4)|(1<<3));
			break;

		default:
			break;
	}
#endif
	XM_unlock();
}

