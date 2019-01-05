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
extern unsigned char XM_BatteryEventProc(unsigned int batteryvalue);

static OS_TIMER Buzzer_Timer;


// 1) �ϵ�ASIC���԰�(176��װ)ʹ��AUX1��Ϊ����,
// 2) ��ƷDEMO��(128��װ)ʹ��AUX0��Ϊ����, AUX1��Ϊ��ص�ѹ���

#define ADC_NULL        	0xffffffff	// �ް���
#define KEY_SHORT_TIME  	200			// �̰�ʱ����ֵ,���ڴ�ֵʱ˵���Ѿ������ڶ̰���
#define	MAX_ADC_DEVICE		2			// AUX0, AUX1

// ����״̬������
#define	FSM_SCAN				1			// scan state
#define	FSM_CHECK				2			// check state
#define	FSM_DELAY				3			// repeat delay state
#define	FSM_REPEAT				4			// repeat state

#define	ADC_MODE_KEY			1			// �������ģʽ
													//		������ѹ����ʱ,��Ҫ�л���3.3v�Ĳο���ѹ(�òο���ѹ����VCC�仯���仯, ��˱�֤�����İ���ֵһ��)
#define	ADC_MODE_BAT			2			// ��ѹ���ģʽ
													//		��ص�ѹ����ʱ,��Ҫ�л���2.0v�Ĳο���ѹ(�òο���ѹ�����¶�/VCC�仯���仯)
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
#define	DIFF	0x100			// ����AD��������ֵ

typedef struct tagADC_KEY {
    u16 ad_min_value;
    u16 ad_max_value;
    u8  key;
} ADC_KEY;


//������
typedef struct
{
    u16               	m_ad_val_min;       ///< ��Ч��������Сֵ
    u16               	m_ad_val_max;       ///< ��Ч���������ֵ
    ADC_KEY*      		m_adkey_val_array;  ///< ָ��AD������ӳ���
    u8      			m_adkey_val_array_length;  ///< ָ��AD������ӳ���
}KEYPAD_PARAM;


//ͨ��6key������ֵ
static ADC_KEY keyboard_6key[] = {
    {0x0, 		   0x0   +DIFF, VK_AP_POWER},//power
    {0x3BC - DIFF, 0x3BC +DIFF, VK_AP_FONT_BACK_SWITCH},//OK
    {0x950 - DIFF, 0x950 +DIFF, VK_AP_DOWN},//down
    {0x716 - DIFF, 0x716 +DIFF, VK_AP_MENU},//Menu
    {0xB68 - DIFF, 0xB68 +DIFF, VK_AP_UP},//UP
    {0xD22 - DIFF, 0xD22 +DIFF, VK_AP_SWITCH},//av
};


typedef struct tagKEY_MOD {
	unsigned int 	key;	// ��Ӧ�İ���
	unsigned int	mode;
}KEY_MOD;


//KEY��Ч״̬
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


static volatile unsigned int last_key;		// ������µļ�ֵ, �ж��Ƿ��ظ�����,��ͬ����
											//	0xFFFFFFFF ��ʾû�а�������
static volatile unsigned int short_key_time;
static volatile unsigned int last_key_ticket;		// ������¼���ʱ��


static void (*adc_callback[MAX_ADC_DEVICE])(unsigned int adc_sample_value);


static void set_debounce_time (UINT32 debounce_time) // ����ʱ��, ΢��
{
	UINT32 dbcount;
	//dbcount = arkn141_get_clks (ARKN141_CLK_APB) * (debounce_time * 1000) / (1000000);

	//dbcount =  (debounce_time / 1000000) / (1/arkn141_get_clks (ARKN141_CLK_APB));
	dbcount =  (debounce_time * arkn141_get_clks (ARKN141_CLK_APB) / 1000000) ;

	if(dbcount >= 0xFFFFF)
		dbcount = 0xFFFFF;
	rADC_DBNCNT = dbcount;
}

static void set_transform_interval (UINT32 interval_millisecond) //���ü��ʱ��(����)
{
	UINT32 reg;

	// 1M Hz sample clock
	// adc_clk = clk_24m/((adc_clk_div+1)*2).
	reg = rSYS_DEVICE_CLK_CFG1;
	reg &= ~0x7FFF;
	reg |= (12 - 1);
	rSYS_DEVICE_CLK_CFG1 = reg;

	reg = interval_millisecond * 1000;		// ת��Ϊ΢�����
	if(reg > 0xFFFF)
		reg = 0xFFFF;
	rADC_DETINTER = reg;
}

UINT32 ADCValue_Data = 0;
unsigned short key_Mode = 0;
UINT32 battery_data = 0;

UINT16 KeyADC_GetKey(void)
{
    unsigned char i;
	
	//get adc
	ADCValue_Data = rADC_AUX1; 			// ��ȡADC����ֵ
	rADC_STA &= ~AUX1_VALUE_INT;		// ���ж�
	
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

//������ӳ��
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
		battery_data = rADC_AUX0;// ��ȡADC����ֵ
    	rADC_STA &= ~AUX0_VALUE_INT;// ���ж�
		XM_BatteryEventProc(battery_data);
	}
	if(adcStatus & AUX0_STOP_INT)
	{
		rADC_STA &= ~AUX0_STOP_INT;
	}

	// AUX1 (KEY)
	if(adcStatus & AUX1_START_INT)
	{
		rADC_STA &= ~AUX1_START_INT;
		// �ر�AUX0. ADC�ǻ�����Դ, ֻ����һ��AUXʹ��.
		// ��AUX1����Ҫת��������ʱ(������ѹֵת��), ��AUX0�ر�, ����ADC��Դ��AUX1ʹ��.
		// AUX1ת����Ϻ�(STOP_INT), ����ʹ��AUX0
		rADC_CTR &= ~(1 << AUX0_CHANNEL);
	}
	
	if(adcStatus & AUX1_VALUE_INT)
	{
		int i;
		UINT32 key_value = KeyADC_GetKey();
		unsigned int key_mode;

		//�̰�,̧����Ч; ����:���ﳤ��ʱ�䷢��һ����Ϣ; �ظ���:�ظ�ִ��
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
		// �����ͷ�
		XM_printf("delay_time..........%d \n",short_key_time);
        if(fisrt==1)
        {
            fisrt=0;  //�ͷź�,����̰����¼���     
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
			// OS_Delay(30); //����������ӡ���Դ���,���ж��е�����ʱ���쳣,��ʱҲ�����ж�?
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
		// ʹ����Ϻ�, ��������AUX0. AUX0���Լ���������ѹֵ.
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

	// ��ص�ѹ����ʱ,��Ҫ�л���2.0v�Ĳο���ѹ(�òο���ѹ�����¶�/VCC�仯���仯)
	// ������ѹ����ʱ,��Ҫ�л���3.3v�Ĳο���ѹ(�òο���ѹ����VCC�仯���仯, ��˱�֤�����İ���ֵһ��)
	rSYS_ANALOG_REG0 &= ~(1 << 22); // ref : 3.3v
	rSYS_ANALOG_REG0 |=  (1 << 21);
	//rSYS_ANALOG_REG0  |= (1 << 22);	// ref : 2.0v

	//reset adc module
	rADC_CTR |= 1<<0;
	delay (100);
	rADC_CTR  = 0;

	// debounce time (APB clock)
	// KEY ȥ����ʱ��Ϊ100us
	set_debounce_time (100);

#if HONGJING_CVBS
	set_transform_interval (4);	// ��ѹ�����Ҫ�Ͽ���ٶ������ACC����, ������16��������ж�
#else
	set_transform_interval (16);	// ÿ16ms����һ��, ����Ϊ�ϴ�ֵ(��64msʱ), ����������, ��������. ��Ϊ16ms�󰴼�������
#endif
	//set_transform_interval (128);
	// AUX0 --> BAT
	// AUX1 --> KEY
	//register irq
	// �ж�ʹ�� (AUX0, AUX1)
	rADC_IMR = 	(0x07 << 0)		// AUX0 (BAT)
				|	(0x07 << 3)		// AUX1 (KEY)
				 ;

	// ����
	last_key = 0xFFFFFFFF;

	rADC_CTR = 0;
	// ��������ж�
	rADC_STA = 0;		// bit clear���
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
	Enable_ADC_Channel (AUX0_CHANNEL);	// ����

	#if CZS_USB_01
	#else
	Enable_ADC_Channel (AUX1_CHANNEL);	// ��ѹ���
	#endif

#else

	#if HONGJING_CVBS
	//Enable_ADC_Channel (AUX0_CHANNEL);	// ��ѹ���
	#else
	Enable_ADC_Channel (AUX1_CHANNEL);	// ����
	Enable_ADC_Channel (AUX0_CHANNEL);	// ��ѹ���
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
			// Bat ��ص�ѹ���
			rADC_CTR |= (1 << AUX1_CHANNEL);
			rADC_STA &= ~(0x07 << 3);			// ����ж�״̬
			//rADC_IMR &= ~((1<<5)|(1<<4));		// ʹ��stop, value 2���ж�Դ
			rADC_IMR &= ~(1 << 5);		// ��ʹ��value�ж�Դ
			break;

		case AUX0_CHANNEL:
			// Key ����
			rADC_CTR |= (1 << AUX0_CHANNEL);
			rADC_STA &= ~(0x07 << 0);	//  ����ж�״̬
			rADC_IMR &= ~((1<<1)|(1<<2));		// ʹ��stop, value 2���ж�Դ
			break;

		default:
			break;
	}

#else

	switch(channel)
	{
		case AUX0_CHANNEL:		// ��ص�ѹ���
			rADC_STA &= ~((1<<2)|(1<<1)|(1<<0));		// ����ж�״̬
			rADC_IMR &= ~((1<<2)|(1<<1)|(1<<0));
			//rADC_IMR |= (1 << 1) | (1 << 0);		// mask start/stop interrupt
			rADC_CTR |= (1 << AUX0_CHANNEL);		// ʹ��
			break;

		case AUX1_CHANNEL:		// ����
			rADC_CTR |= (1<<AUX1_CHANNEL);
			rADC_IMR &=~((1<<5)|(1<<4)|(1<<3));
			//rADC_IMR &= ~((1<<5)|(1<<4));		// ��ʹ��START
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
			rADC_IMR |= ((1<<2)|(1<<1)|(1<<0));		// ��ֹ3���ж�Դ
			break;

		case AUX1_CHANNEL:
			rADC_CTR |= (1<<9);
			rADC_CTR &= ~(1 << AUX1_CHANNEL);
			rADC_IMR |= ((1<<5)|(1<<4)|(1<<3));		// ��ֹ3���ж�Դ
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

