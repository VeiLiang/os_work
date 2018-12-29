
/***********************************************************************
*Copyright (c)2012  Arkmicro Technologies Inc. 
*All Rights Reserved 
*
*Filename: remote.c
*Version : 1.0
*Date    : 2007.06.11
*Author  : Harey
*Abstract: ark1600 soc Remote Controller module code	      
*History :     
* 
*Version : 2.0 
*Date    : 2012.02.27
*Author  :  
*Abstract: ark1660  Ver1.0 MPW driver remove waring
*History : 1.0

************************************************************************/
#include "types.h" 
#include "common.h"
#include "ark1960.h"
#include "irqs.h"
#include "remote.h"
#include "fevent.h"
//#include "timer.h"
#include "printk.h"
//#include "xmtype.h"
//#include "xmbase.h"
#include "cpuClkCounter.h"
#include "sys_soft_reset.h"

#ifdef REMOTE_CODE_TACHOGRAPH
//#include "xmdev.h"
//#include "xmprintf.h"
#endif

#define	GetSysTimer_ms		XM_GetTickCount

#if 1//def CONFIG_ARK1960_ASIC
#define INPUTCLK				24000000
#else
//#define INPUTCLK				27000000
#define INPUTCLK				35000000
//#define INPUTCLK				45000000
//#define INPUTCLK				20000000
//#define INPUTCLK				54000000
#endif

#define PREDIV                              (INPUTCLK/760000-1)

#define SEC_TimeoneH  		0.00056
#define SEC_TimeoneL 		0.00169
#define SEC_TimezeroH 		0.00056
#define SEC_TimezeroL 		0.00056
#define SEC_TimestartH		0.009
#define SEC_TimestartL		0.0045
#define SEC_Timerp2_3H	 	0.009
#define SEC_Timerp2_3L		0.00225
#define SEC_Timerp4_5H		0.00056
#define SEC_Timerp4_5L		0.0962


#define IR_CS    1

UINT32 flag=0;
REMOTE_DATA tmpRemote1;
void remote_config(void) //进行遥控接收器的参数配置
{
#ifdef REMOTE_CODE_ARK9

	// nec
	INT8 pulse_data_polarity = 0x1;//[3 :0 ]
	INT8 valid_bitrange = 0x05;//[6 :4 ]
	INT8 dispersion = 0x5;//[15:8 ]
	INT8 prediv = 0x3f;//[23:16]   24M: 0x1f  48M: 0x3f
	INT8 filtertime = 0x10;//[31:24]

	INT8 start_valueh = 0x69;//[23:16]
	INT8 start_valuel = 0x34;//[31:24]

	INT8 one_valueh = 0x6; //0x6;//[7 : 0]
	INT8 one_valuel = 0x13;//[15: 8]

	INT8 zero_valueh = 0x6;//[23:16]
	INT8 zero_valuel = 0x6;//[31:24]

	INT8 rp0_valueh = 0xff;//[7 : 0]
	INT8 rp0_valuel = 0xff;//[15: 8]

	INT8 rp1_valueh = 0xff;//[23:16]
	INT8 rp1_valuel = 0xff;//[31:24]

	INT8 rp2_3_valueh = 0x69;//[7 : 0]
	INT8 rp2_3_valuel = 0x1a;//[15: 8]

	INT8 rp4_5_valueh =0x6;//[23:16]
	INT8 rp4_5_valuel = 0x67;//[31:24]

	INT8 keyrelease_timeh = 0x91;//[7 : 0]
	INT8 keyrelease_timel = 0x05;//[15: 8]

	INT8 int_num = 0x01;//[23:16]
	INT8 mode_sel_reg = 0x03;//[25:24]
	INT8 nec_release_int_en = 0x01;//[26]
	INT8 user_code1_l = 0x02;//[7 : 0]
	INT8 user_code1_h = 0x00;//[15: 8]
	INT8 user_code2_l = 0xff;//[23:16]
	INT8 user_code2_h =0x00;//[31:24]
	INT8 NEC_bit_2_pulse = 0xd3;//[15: 0]
	INT8 user_code_sel_reg = 0x00;//[23:16]

	INT8 user_code3_l = 0x02;//[7 : 0]
	INT8 user_code3_h = 0x00;//[15: 8]
	INT8 user_code4_l = 0xff;//[23:16]
	INT8 user_code4_h =0x00;//[31:24]

#endif

#ifdef REMOTE_CODE_CHIP

	// nec
	INT8 pulse_data_polarity = 0x1;//[3 :0 ]
	INT8 valid_bitrange = 0x05;//[6 :4 ]
	INT8 dispersion = 0x5;//[15:8 ]
	INT8 prediv = 0x1f;//[23:16]   24M: 0x1f  48M: 0x3f
	INT8 filtertime = 0x10;//[31:24]

	INT8 start_valueh = 0x69;//[23:16]
	INT8 start_valuel = 0x34;//[31:24]

	INT8 one_valueh = 0x6;//[7 : 0]
	INT8 one_valuel = 0x13;//[15: 8]

	INT8 zero_valueh = 0x6;//[23:16]
	INT8 zero_valuel = 0x6;//[31:24]

	INT8 rp0_valueh = 0xff;//[7 : 0]
	INT8 rp0_valuel = 0xff;//[15: 8]

	INT8 rp1_valueh = 0xff;//[23:16]
	INT8 rp1_valuel = 0xff;//[31:24]

	INT8 rp2_3_valueh = 0x69;//[7 : 0]
	INT8 rp2_3_valuel = 0x1a;//[15: 8]

	INT8 rp4_5_valueh = 0x6;//[23:16]
	INT8 rp4_5_valuel = 0x67;//[31:24]

	INT8 keyrelease_timeh = 0x91;//[7 : 0]
	INT8 keyrelease_timel = 0x05;//[15: 8]

	INT8 int_num = 0x01;//[23:16]
	INT8 mode_sel_reg = 0x03;//[31:24]
	INT8 nec_release_int_en = 0x01;//[26]
	INT8 user_code1_l = 0x00;//[7 : 0]
	INT8 user_code1_h = 0x00;//[15: 8]
	INT8 user_code2_l = 0xff;//[23:16]
	INT8 user_code2_h =0x00;//[31:24]
	INT8 NEC_bit_2_pulse = 0xd3;//[15: 0]
	INT8 user_code_sel_reg = 0x00;//[23:16]
	
	INT8 user_code3_l = 0x02;//[7 : 0]
	INT8 user_code3_h = 0x00;//[15: 8]
	INT8 user_code4_l = 0xff;//[23:16]
	INT8 user_code4_h =0x00;//[31:24]
	
	#endif

#ifdef REMOTE_CODE_SOUND
	// nec
	INT8 pulse_data_polarity = 0x1;//[3 :0 ]
	INT8 valid_bitrange = 0x05;//[6 :4 ]
	INT8 dispersion = 0x5;//[15:8 ]
	INT8 prediv = 0x3f;//[23:16]   24M: 0x1f  48M: 0x3f
	INT8 filtertime = 0x10;//[31:24]

	INT8 start_valueh = 0x69;//[23:16]//0x69
	INT8 start_valuel = 0x34;//[31:24]

	INT8 one_valueh = 0x06;//0x6;//[7 : 0]
	INT8 one_valuel = 0x13;//[15: 8]

	INT8 zero_valueh = 0x6;//[23:16]
	INT8 zero_valuel = 0x6;//[31:24]

	INT8 rp0_valueh = 0xff;//[7 : 0]
	INT8 rp0_valuel = 0xff;//[15: 8]

	INT8 rp1_valueh = 0xff;//[23:16]
	INT8 rp1_valuel = 0xff;//[31:24]

	INT8 rp2_3_valueh = 0x69;//[7 : 0]
	INT8 rp2_3_valuel = 0x1a;//[15: 8]

	INT8 rp4_5_valueh = 0x6;//[23:16]
	INT8 rp4_5_valuel = 0x67;//[31:24]

	INT8 keyrelease_timeh = 0x91;//[7 : 0]
	INT8 keyrelease_timel = 0x05;//[15: 8]

	INT8 int_num = 0x01;//[23:16]
	INT8 mode_sel_reg = 0x03;//[31:24]
	INT8 nec_release_int_en = 0x01;//[26]
	INT8 user_code1_l = 0x00;//[7 : 0]
	INT8 user_code1_h = 0x00;//[15: 8]
	INT8 user_code2_l = 0xff;//[23:16]
	INT8 user_code2_h =0x00;//[31:24]
	INT8 NEC_bit_2_pulse = 0xd3;//[15: 0]
	INT8 user_code_sel_reg = 0x00;//[23:16]

	INT8 user_code3_l = 0x02;//[7 : 0]
	INT8 user_code3_h = 0x00;//[15: 8]
	INT8 user_code4_l = 0xff;//[23:16]
	INT8 user_code4_h =0x00;//[31:24]

#endif

#ifdef remote_mode
//	nec mode
	char pulse_data_polarity = 0x01;//[3 :0 ]   
	char valid_bitrange = 0x04;//[6 :4 ]
	char dispersion = 0x8;//[15:7 ]
	//char prediv = 0x1f;//PREDIV;//[23:16]   12M: 0xf    24M: 0x1f  35M: 0x2d
	char prediv = 0x2d;//PREDIV;//[23:16]   12M: 0xf    24M: 0x1f  35M: 0x2d
	char filtertime = 0x08;	//0x01;//[31:24]  

	char start_valueh = (SEC_TimestartH*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[23:16]
	
	char start_valuel = (SEC_TimestartL*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[31:24] 

	char one_valueh = (SEC_TimeoneH*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[7 : 0]
	char one_valuel = (SEC_TimeoneL*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[15: 8]
	char zero_valueh = (SEC_TimezeroH*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[23:16]
	char zero_valuel = (SEC_TimezeroL*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[31:24]

	char rp2_3_valueh =(SEC_Timerp2_3H*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[7 : 0]
	char rp2_3_valuel = (SEC_Timerp2_3L*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[15: 8]
	char rp4_5_valueh = (SEC_Timerp4_5H*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[23:16]
	char rp4_5_valuel =(SEC_Timerp4_5L*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[31:24]
	char rp4_5_valuel_delta = 0x12;

	char rp0_valueh = 0x01;//[7 : 0]//start_h_dis
	char rp0_valuel = 0x01;//[15: 8]//start_l_dis
	char rp1_valueh = 0x01;//[23:16]//rp23_h_dis
	char rp1_valuel = 0x01;//[31:24]//rp23_l_dis
	
	char keyrelease_timeh = 0xf0;//[7 : 0]
	char keyrelease_timel = 0xf0;//[15: 8]  
	char int_num = 0x01;//[23:16]
	char mode_sel_reg = 0x03;//[31:24]
	
	char user_code1_l = 0x00;//[7 : 0]
	char user_code1_h = 0x00;//[15: 8]  
	char user_code2_l = 0xff;//[23:16]
	char user_code2_h =0x00;//[31:24]
	char NEC_bit_2_pulse = 0xd3;//[15: 0]  
	char user_code_sel_reg = 0x00;//[23:16]

	char nec_release_int_en = 0x01;//[26]
#endif
	
#ifdef REMOTE_CODE_TACHOGRAPH		// 行车记录仪遥控
//	nec mode
	char pulse_data_polarity = 0x01;//[3 :0 ]   
	char valid_bitrange = 0x04;//[6 :4 ]
	char dispersion = 0x8;//[15:7 ]
	//char prediv = 0x1f;//PREDIV;//[23:16]   12M: 0xf    24M: 0x1f  35M: 0x2d
	char prediv = 0x1f;//PREDIV;//[23:16]   12M: 0xf    24M: 0x1f  35M: 0x2d
	char filtertime = 0x08;	//0x01;//[31:24]  

	char start_valueh = (SEC_TimestartH*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[23:16]
	char start_valuel = (SEC_TimestartL*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[31:24] 
	char one_valueh = (SEC_TimeoneH*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[7 : 0]
	char one_valuel = (SEC_TimeoneL*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[15: 8]
	char zero_valueh = (SEC_TimezeroH*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[23:16]
	char zero_valuel = (SEC_TimezeroL*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[31:24]

	char rp2_3_valueh =(SEC_Timerp2_3H*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[7 : 0]
	char rp2_3_valuel = (SEC_Timerp2_3L*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[15: 8]
	char rp4_5_valueh = (SEC_Timerp4_5H*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[23:16]
	char rp4_5_valuel =(SEC_Timerp4_5L*INPUTCLK)/((prediv+1)*(2<<valid_bitrange));//[31:24]
	char rp4_5_valuel_delta = 0x12;

	char rp0_valueh = 0x01;//[7 : 0]//start_h_dis
	char rp0_valuel = 0x01;//[15: 8]//start_l_dis
	char rp1_valueh = 0x01;//[23:16]//rp23_h_dis
	char rp1_valuel = 0x01;//[31:24]//rp23_l_dis
	
	char keyrelease_timeh = 0xf0;//[7 : 0]
	char keyrelease_timel = 0xf0;//[15: 8]  
	char int_num = 0x01;//[23:16]
	char mode_sel_reg = 0x03;//[31:24]
	
	char user_code1_l = 0x00;//[7 : 0]
	char user_code1_h = 0x00;//[15: 8]  
	char user_code2_l = 0xff;//[23:16]
	char user_code2_h =0x00;//[31:24]
	char NEC_bit_2_pulse = 0xd3;//[15: 0]  
	char user_code_sel_reg = 0x00;//[23:16]

	char nec_release_int_en = 0x01;//[26]
#endif	

	rRC_DATA0  = (pulse_data_polarity<<0)
				+ (valid_bitrange<<4)
				+ (dispersion<<7)
				+ (prediv<<15)
				+ (filtertime<<24);

	rRC_DATA1 = (rp4_5_valuel_delta << 0)
				+ (start_valueh<<16)
			   + (start_valuel<<24);


	rRC_DATA2 = (one_valueh<<0)
			   + (one_valuel<<8)
			   + (zero_valueh<<16)
			   + (zero_valuel<<24);

	rRC_DATA3 = (rp0_valueh<<0)
			   + (rp0_valuel<<8)
			   + (rp1_valueh<<16)
			   + (rp1_valuel<<24);

	rRC_DATA4 = (rp2_3_valueh<<0)
			   + (rp2_3_valuel<<8)
			   + (rp4_5_valueh<<16)
			   + (rp4_5_valuel<<24);

	rRC_DATA5 = (keyrelease_timeh<<0)
			   + (keyrelease_timel<<8)
			   + (int_num<<16)
			   + (mode_sel_reg<<24)
			   + (nec_release_int_en << 26);

	rRC_DATA6 = (user_code1_l<<0)
			   + (user_code1_h<<8)
			   + (user_code2_l<<16)
			   + (user_code2_h<<24);

/*	rRC_RT_USER_CODE_3_4 =  (user_code3_l<<0)
			   + (user_code3_h<<8)
			   + (user_code4_l<<16)
			   + (user_code4_h<<24);
*/
	rRC_DATA7 = (NEC_bit_2_pulse<<0)
			   + (user_code_sel_reg<<16)
			   +(1<<24)
			   +(1<<25)
			   +(1<<26);
	
}

    //0x1b_up
    //0x1a_down
    //0x04_left
    //0x06_right
    //0x05_enter
#ifdef REMOTE_CODE_TACHOGRAPH		// 行车记录仪遥控
	#define REMOTE_POWER        	0x12
	#define REMOTE_MODE	        	0x02
	#define REMOTE_MENU	        	0x05	// 播放、暂停
	#define REMOTE_SWITCH			0x00

	#define REMOTE_UP				0x1B
	#define REMOTE_DOWN	     		0x1A
	#define REMOTE_LEFT				0x04
	#define REMOTE_RIGHT			0x06

	#define REMOTE_VOLINC			0x52		// 播放音量
	#define REMOTE_VOLDEC			0x40
	#define REMOTE_MICINC			0x1C		// 录音音量
	#define REMOTE_MICDEC			0x53

	#define REMOTE_URGENT			0x41		// 紧急录像 （第二排第一个）	

	#define REMOTE_0					0x5a		// --> AVOUT IN		AVOUT插入
	#define REMOTE_1					0x46		// --> AVOUT OUT		AVOUT拔出
	#define REMOTE_2					0x19		// --> ADJUST BELL VOLUME
	#define REMOTE_3					0x0D		// --> ADJUST MIC VOLUME
	#define REMOTE_4					0x4B		// --> 一键锁定(紧急录像)
	#define REMOTE_5					0x5F		// --> 疲劳驾驶预警
	#define REMOTE_6					0x5B		// --> 疲劳驾驶报警
	#define REMOTE_7					0x4F		// --> 可录时间报警
	#define REMOTE_8					0x57		// --> USB断开
	#define REMOTE_9					0x5E		// --> USB连接，充电线
#endif

#ifdef remote_mode	

	#define REMOTE_POWER        	0x12
	#define REMOTE_CARD         		0x1A
	#define REMOTE_MUTE  	 	0x1E
	#define REMOTE_PHOTO		0x01
	#define REMOTE_MUSIC		0x02
	#define REMOTE_MOVIE		0x03

	#define REMOTE_PLAY			0x04
	#define REMOTE_STOP	        	0x06
	#define REMOTE_ENTER        	0x08
	#define REMOTE_UP			0x05
	#define REMOTE_LEFT			0x07
	#define REMOTE_RIGHT		0x09
	#define REMOTE_DOWN	        0x1B

	#define REMOTE_SETUP		0x0A
	#define REMOTE_ZOOM	        	0x1F
	#define REMOTE_VOLINC		0x0C
	#define REMOTE_VOLDEC		0x00
	#define REMOTE_PREV 			0x0D
	#define REMOTE_NEXT	    		0x0E
	#define REMOTE_MENU	        	0x0F
	#define REMOTE_MODE	        	0x19
#endif

#ifdef REMOTE_CODE_ARK9

#define REMOTE_KEY_0		0x00
#define REMOTE_KEY_1		0x02
#define REMOTE_KEY_2		0x03
#define REMOTE_KEY_3		0x04
#define REMOTE_KEY_4		0x05
#define REMOTE_KEY_5		0x08
#define REMOTE_KEY_6		0x09
#define REMOTE_KEY_7		0x0c
#define REMOTE_KEY_8		0x0d

#endif

#ifdef REMOTE_CODE_CHIP

#define REMOTE_POWER        0x46//0x12
#define REMOTE_CARD         0x1A
#define REMOTE_MUTE  	 	0x1E
#define REMOTE_PHOTO		0x01
#define REMOTE_MUSIC		0x02
#define REMOTE_MOVIE		0x03

#define REMOTE_PLAY			0x04
#define REMOTE_STOP	        0x06
#define REMOTE_ENTER        0x18//0x08
#define REMOTE_UP			0x10//0x05
#define REMOTE_LEFT			0x17//0x07
#define REMOTE_RIGHT		0x16//0x09
#define REMOTE_DOWN	        0x19//0x1B

#define REMOTE_SETUP		0x0A
#define REMOTE_ZOOM	        0x1F
#define REMOTE_VOLINC	    0x0C
#define REMOTE_VOLDEC		0x00
#define REMOTE_PREV 		0x0D
#define REMOTE_NEXT	    	0x0E
#define REMOTE_MENU	        0x0F
#define REMOTE_MODE	        0x0F//0x19

#endif

#ifdef REMOTE_CODE_SOUND

#define REMOTE_POWER		0x46
#define REMOTE_MUTE			0x49
#define REMOTE_SETUP		0x51
#define REMOTE_OPEN			0x59
#define REMOTE_VOLINC		0x58
#define REMOTE_VOLDEC		0x0a

#define REMOTE_0			0x14
#define REMOTE_1			0x5a
#define REMOTE_2			0x47
#define REMOTE_3			0x50
#define REMOTE_4			0x02
#define REMOTE_5			0x05
#define REMOTE_6			0x54
#define REMOTE_7			0x1e
#define REMOTE_8			0x0e
#define REMOTE_9			0x1c

#define REMOTE_MENU			0x0f
#define REMOTE_AUDIO		0x0c
#define REMOTE_CLR			0x1a
#define REMOTE_DISP			0x0d
#define REMOTE_EQ			0x13
#define REMOTE_SYS			0x12
#define REMOTE_PROG			0x1b
#define REMOTE_DANGLE		0x1d

#define REMOTE_UP			0x10
#define REMOTE_DOWN		0x19
#define REMOTE_LEFT			0x17
#define REMOTE_RIGHT		0x16
#define REMOTE_ENTER		0x18

#define REMOTE_PLAY			0x5c
#define REMOTE_STOP			0x1f
#define REMOTE_PREV			0x5e
#define REMOTE_NEXT			0x5d
#define REMOTE_BWD			0x11
#define REMOTE_FWD			0x15

#define REMOTE_TF			0x5f
#define REMOTE_SLOW			0x4f
#define REMOTE_PBN			0x4e
#define REMOTE_PH			0x4d
#define REMOTE_PP			0x4c
#define REMOTE_ZOOM			0x43
#define REMOTE_PBP			0x42
#define REMOTE_MP			0x41
#define REMOTE_AP			0x40

#endif

#define NORMAL_KEY      0
#define RELEASE_KEY     1
#define REPEAT_KEY      2

#ifdef REMOTE_CODE_TACHOGRAPH
#include "xm_key.h"
#include "xm_dev.h"
//#include <xmuser.h>


/*
// 将遥控按键翻译为系统事件
UINT8 Remote2SystemEvent[] = 
{
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	
	0,
	0,
	0,
	0,
	0,
	
	// REMOTE 0 ~ 9按键
	SYSTEM_EVENT_AVOUT_PLUGIN,						// 0
	SYSTEM_EVENT_AVOUT_PLUGOUT,					//	1
	SYSTEM_EVENT_ADJUST_BELL_VOLUME,				//	2
	SYSTEM_EVENT_ADJUST_MIC_VOLUME,				//	3
	SYSTEM_EVENT_URGENT_RECORD,					//	4
	SYSTEM_EVENT_DRIVING_LONGTIME_WARNING,		//	5
	SYSTEM_EVENT_DRIVING_LONGTIME_ALARM,		//	6
	SYSTEM_EVENT_RECORD_SPACE_INSUFFICIENT,	//	7
	SYSTEM_EVENT_USB_DISCONNECT,					// 8
	SYSTEM_EVENT_USB_CONNECT_CHARGE				// 9
};
*/


// 将遥控按键翻译为键盘按键事件
UINT8 Remote2Keyboard[] =
{
    //VK_AP_POWER,
    //VK_AP_MENU,
    //VK_AP_FONT_BACK_SWITCH,
    //VK_AP_OK,
    //VK_AP_DOWN,
    //VK_AP_UP,
    REMOTE_KEY_POWER,
    REMOTE_KEY_MENU,
    REMOTE_KEY_UP,
	REMOTE_KEY_DOWN,
	REMOTE_KEY_LEFT,
	REMOTE_KEY_RIGHT,
#if 0
	REMOTE_KEY_POWER,
	VK_AP_MODE,
	REMOTE_KEY_MENU,
	VK_AP_SWITCH,
	REMOTE_KEY_UP,
	REMOTE_KEY_DOWN,
	REMOTE_KEY_LEFT,
	REMOTE_KEY_RIGHT,
	
	VK_AP_VOLINC,
	VK_AP_VOLDEC,
	VK_AP_MICINC,
	VK_AP_MICDEC,
	VK_AP_URGENT,
	
	// REMOTE 0 ~ 9按键
	VK_AP_SYSTEM_EVENT,
	VK_AP_SYSTEM_EVENT,
	VK_AP_SYSTEM_EVENT,
	VK_AP_SYSTEM_EVENT,
	VK_AP_SYSTEM_EVENT,
	VK_AP_SYSTEM_EVENT,
	VK_AP_SYSTEM_EVENT,
	VK_AP_SYSTEM_EVENT,
	VK_AP_SYSTEM_EVENT,
	VK_AP_SYSTEM_EVENT
#endif	
};

#endif


UINT8 RemoteKeyMap[] =
{
	REMOTE_POWER,		// 电源键  
    REMOTE_MENU,        // 菜单键
    REMOTE_UP,          
    REMOTE_DOWN,                
    REMOTE_LEFT,            
    REMOTE_RIGHT,   

#if 0
#ifdef REMOTE_CODE_TACHOGRAPH
	// 功能键4个
	REMOTE_POWER,		// 电源键  
	REMOTE_MODE,		// 模式键
	REMOTE_MENU,		// 菜单键
	REMOTE_SWITCH,		// 通道切换键
	REMOTE_UP,			
	REMOTE_DOWN,	        	
	REMOTE_LEFT,			
	REMOTE_RIGHT,	

	REMOTE_VOLINC,	   	
	REMOTE_VOLDEC,		
	REMOTE_MICINC,       	
	REMOTE_MICDEC,	
	REMOTE_URGENT,
	
	REMOTE_0,
	REMOTE_1,
	REMOTE_2,
	REMOTE_3,
	REMOTE_4,
	REMOTE_5,
	REMOTE_6,
	REMOTE_7,
	REMOTE_8,
	REMOTE_9,
#endif	

#ifdef remote_mode

	REMOTE_POWER,        		
	REMOTE_CARD,         		
	REMOTE_MUTE,  	 		
	REMOTE_PHOTO,			
	REMOTE_MUSIC,			
	REMOTE_MOVIE,			

	REMOTE_PLAY,			
	REMOTE_STOP,	        	
	REMOTE_ENTER,        		
	REMOTE_UP,			
	REMOTE_LEFT,			
	REMOTE_RIGHT,			
	REMOTE_DOWN,	        	

	REMOTE_SETUP,			
	REMOTE_ZOOM,	        	
	REMOTE_VOLINC,	   	
	REMOTE_VOLDEC,		
	REMOTE_PREV ,		
	REMOTE_NEXT	,    		
	REMOTE_MENU	 ,       	
	REMOTE_MODE,	        	
#endif

#ifdef REMOTE_CODE_ARK9

	REMOTE_KEY_0,
	REMOTE_KEY_1,
	REMOTE_KEY_2,
	REMOTE_KEY_3,
	REMOTE_KEY_4,
	REMOTE_KEY_5,
	REMOTE_KEY_6,
	REMOTE_KEY_7,
	REMOTE_KEY_8,
	
#endif

#ifdef REMOTE_CODE_CHIP

	REMOTE_POWER,                       // POWER    KEY_MODE/KEY_POWER
	REMOTE_MODE,                        // MODE     KEY_MODE
	REMOTE_ENTER,                       // ENTER    KEY_PLAY/KEY_OPTION
	REMOTE_UP,                      	// UP       KEY_UP
	REMOTE_LEFT,                      	// LEFT     KEY_LEFT
	REMOTE_RIGHT,                      	// RIGHT    KEY_RIGHT
	REMOTE_DOWN,                      	// DOWN     KEY_DOWN
	REMOTE_STOP,                        // STOP     KEY_ROTATE
	REMOTE_ZOOM,                        // ZOOM     KEY_ZOOM
	REMOTE_SETUP,                     	// SETUP    KEY_SETUP
	REMOTE_MENU,                        // MENU	    KEY_MENU/KEY_HOME
	REMOTE_PLAY,                      	// PLAY     KEY_PHOTOSLIDE
	REMOTE_PHOTO,                       // PHOTO    KEY_PHOT
	REMOTE_PREV,                        // PREV     KEY_PREV
	REMOTE_NEXT,                        // NEXT     KEY_NEXT
	REMOTE_VOLINC,                      // VOLINC   KEY_VOLINC
	REMOTE_VOLDEC,                      // VOLDEC   KEY_VOLDEC
	REMOTE_MUTE,                        // MUTE     KEY_MUTE
	REMOTE_CARD,                        // CARD     KEY_CARD
	REMOTE_MUSIC,                       // MUSIC    KEY_MUSIC
	REMOTE_MOVIE,                       // MOVIE    KEY_MOVIE

#endif

#ifdef REMOTE_CODE_SOUND

	REMOTE_POWER,
	REMOTE_MUTE,
	REMOTE_SETUP,
	REMOTE_OPEN,
	REMOTE_1,
	REMOTE_2,
	REMOTE_3,
	REMOTE_VOLINC,
	REMOTE_4,
	REMOTE_5,
	REMOTE_6,
	REMOTE_VOLDEC,
	REMOTE_7,
	REMOTE_8,
	REMOTE_9,
	REMOTE_0,
	REMOTE_MENU,
	REMOTE_AUDIO,
	REMOTE_CLR,
	REMOTE_DISP,
	REMOTE_EQ,
	REMOTE_UP,
	REMOTE_SYS,
	REMOTE_BWD,
	REMOTE_LEFT,
	REMOTE_ENTER,
	REMOTE_RIGHT,
	REMOTE_FWD,
	REMOTE_PROG,
	REMOTE_DOWN,
	REMOTE_DANGLE,
	REMOTE_STOP,
	REMOTE_TF,
	REMOTE_PREV,
	REMOTE_NEXT,
	REMOTE_PLAY,
	REMOTE_SLOW,
	REMOTE_PBN,
	REMOTE_PH,
	REMOTE_PP,		
	REMOTE_ZOOM,
	REMOTE_PBP,
	REMOTE_MP,
	REMOTE_AP,
  	
#endif
#endif
};
  
  
volatile UINT32 cRemoteKey;               // 当前键值
volatile UINT32 cRemoteStatus;            // 当前键状态

extern int addEventQueue(fEvent *pEvent);
extern unsigned char bAvoutPlugIn;

u8_t LongPress_Data = 0;
BOOL LongPress_Data_Status = FALSE;

static int Remotekey_state;
#define REMOTE_NULL                 0
#define	REMOTE_SCAN				   1			// scan state
#define	REMOTE_CHECK				2			// check state
#define	REMOTE_DELAY				3			// repeat delay state
#define	REMOTE_REPEAT				4			// repeat state

static unsigned short Remotekey_delay_time;

static void SendRemoteKeyEvent(UINT32 key_type, UINT32 nKeyCode)
{
#ifdef REMOTE_CODE_TACHOGRAPH
	unsigned char key;

	// 将遥控键翻译为按键事件
	if(nKeyCode >= sizeof(Remote2Keyboard))
		return;
	
	key = Remote2Keyboard[nKeyCode];
    
	// 判断是否是系统事件
	if(key == VK_AP_SYSTEM_EVENT)
	{
		// 系统事件模拟
		if(key_type == KEY_DOWN)
		{
		}
	}
	else
	{
		// 按键事件模拟
		#if 1
        #if 0
		if(key_type == KEY_DOWN)
		{
		   // XM_printf ("key=0x%02x, %s\n", key, (key_type != KEY_UP) ? "DOWN" : "UP");
			XM_KeyEventProc (key, XMKEY_PRESSED|XMKEY_STROKE);
		}
        #endif
        Remotekey_delay_time ++;
        if((Remotekey_delay_time == 10) && !LongPress_Data_Status){
             //Remotekey_delay_time = 0;

            LongPress_Data_Status = TRUE;
            if(key == REMOTE_KEY_MENU)
            {
                XM_KeyEventProc (key, XMKEY_LONGTIME|XMKEY_STROKE);
            }
        }
        else
        {
            if(key_type == KEY_UP) 
            {
                if(Remotekey_delay_time < 10) {
                    XM_KeyEventProc (key, XMKEY_PRESSED|XMKEY_STROKE); 
                }
                Remotekey_delay_time = 0;
                LongPress_Data_Status = FALSE;
            }
            else if(key_type == KEY_REPEAT) //
            {
                printf("remote_key=%d\n",key);
                if((key==REMOTE_KEY_LEFT)||(key==REMOTE_KEY_RIGHT))
              //  if(Remotekey_delay_time < 10) 
                {
                    XM_KeyEventProc (key, XMKEY_REPEAT|XMKEY_STROKE); 
                }
            }                     
        }
        #else
        if(key_type == KEY_DOWN) //按键按下
        {
            Remotekey_state = REMOTE_SCAN;
        }

        if(Remotekey_state == REMOTE_SCAN)
        {
            if(key_type == KEY_REPEAT)
                Remotekey_state = REMOTE_CHECK;
        }else if(Remotekey_state == REMOTE_CHECK){
            if(key_type == KEY_REPEAT) {
                Remotekey_state = REMOTE_DELAY;
                Remotekey_delay_time = 20;
            }else {
                Remotekey_state = REMOTE_SCAN;
            }
            
        }else if(Remotekey_state == REMOTE_DELAY) {
            Remotekey_delay_time --;
            if(Remotekey_delay_time == 0) { // 20次
                printf("11111111111111111111111 XMKEY_LONGTIME\n");
                XM_KeyEventProc (key, XMKEY_LONGTIME|XMKEY_STROKE);
            }else if(key_type == KEY_UP) //按键松开
            {
                Remotekey_state = REMOTE_SCAN;
                printf("11111111111111111111111 XMKEY_PRESSED\n");
                XM_KeyEventProc (key, XMKEY_PRESSED|XMKEY_STROKE);
            }
       }else {
            Remotekey_state = REMOTE_SCAN;
       }
       #endif
	}
#else
	fEvent curEvent;
	fKey_Event *pKeyEvent;

	pKeyEvent = (fKey_Event *)(&curEvent.uFEvent);
	pKeyEvent->event_type = KEY_EVENT;
	pKeyEvent->key_type = key_type;
	pKeyEvent->keyCode = nKeyCode;
	addEventQueue(&curEvent);
#endif
}


static UINT32 lg_ulMaxRepeatMs = 0;
static UINT32 lg_ulLastRepeatMs = 0;

#define REMOTE_STATE_IDLE			0
#define REMOTE_STATE_PRESS		1
#define REMOTE_STATE_REPEATE		2

#define REMOTE_PRESS_EVENT		0
#define REMOTE_RELEASE_EVENT		1
#define REMOTE_REPEATE_EVENT		2

static UINT32 lg_ulRemoteStateMachine = REMOTE_STATE_IDLE;

/*********************************************************************
	Set the report interval for remote repeating event
Parameter:
	ulMillisecond: interval, unit as millisecond, this parameter should be the multiply of
	 10 millsecond.
*********************************************************************/
void SetRemoteKeyRepeateInterval(UINT32 ulMillisecond)
{
	lg_ulMaxRepeatMs = ulMillisecond/10;
}

/*********************************************************************
	Get the report interval for remote repeating event
Return:
	millisendonds for interval
*********************************************************************/
UINT32 GetRemoteKeyRepeatInterval(void)
{
	return lg_ulMaxRepeatMs * 10;
}

static UINT32 GetLogicKeyCode(UINT32 ulSampleKeyCode)
{
	UINT32 i;

	for(i=0;i<sizeof(RemoteKeyMap)/sizeof(RemoteKeyMap[0]);i++)
	{
		if(ulSampleKeyCode == RemoteKeyMap[i])
			return i;
	}

	return REMOTE_NULL;
}

static void RemoteEventHandler(UINT32 ulEvent, UINT32 ulSampleKeyCode)
{
	UINT32 ulKeyCode;

	switch(lg_ulRemoteStateMachine)
	{
		case REMOTE_STATE_IDLE:
			if(ulEvent == REMOTE_PRESS_EVENT)
			{
				lg_ulRemoteStateMachine = REMOTE_STATE_REPEATE;
				lg_ulLastRepeatMs = GetSysTimer_ms();
				//send remote key press message
				ulKeyCode = GetLogicKeyCode(ulSampleKeyCode);
				SendRemoteKeyEvent(KEY_DOWN, ulKeyCode);	
			}
			break;

		case REMOTE_STATE_REPEATE:
			if(ulEvent == REMOTE_REPEATE_EVENT)
			{
				UINT32 ulCurMs;
				ulCurMs = GetSysTimer_ms();
				if(ulCurMs - lg_ulLastRepeatMs > lg_ulMaxRepeatMs)
				{
					lg_ulLastRepeatMs = ulCurMs;
					//send repeat message
					ulKeyCode = GetLogicKeyCode(ulSampleKeyCode);
					SendRemoteKeyEvent(KEY_REPEAT, ulKeyCode);
				}				
			}
			else
			{
				//send remote key release message
				lg_ulRemoteStateMachine = REMOTE_STATE_IDLE;
				ulKeyCode = GetLogicKeyCode(ulSampleKeyCode);
				SendRemoteKeyEvent(KEY_UP, ulKeyCode);					
			}
			break;
	}
}


static void  remote_int_handler(void)
{
	//printk("Enter remote interrupt!\n");

	cRemoteStatus = rRC_STATUS&0x0F;    // 取遥控码状态
	cRemoteKey = rRC_KEYVAL;            // 取键值
	rRC_KEYVAL = 0xff;

	rRC_RT_INTER_REQ_CLR = 0xff;
    //printf("11111111111 cRemoteStatus: %d cRemoteKey: %d \n",cRemoteStatus,cRemoteKey);
	if(cRemoteStatus & 0x02)
	{
		RemoteEventHandler(REMOTE_REPEATE_EVENT, cRemoteKey);
	}
	else if(cRemoteStatus & 0x01)
	{
		RemoteEventHandler(REMOTE_RELEASE_EVENT, cRemoteKey);
	}
	else
	{
		RemoteEventHandler(REMOTE_PRESS_EVENT, cRemoteKey);
	}
}

static void select_remote_pad(void)
{
	OS_IncDI();	// disable interrupt
	
#ifdef OLD_PAD
#if IR_CS
	rSYS_PAD_CTRL09 |= (0x2 << 12);
	rSYS_PAD_CTRL0A |= 0x1;
#else
	rSYS_PAD_CTRL08 |= 0x3;
	rSYS_PAD_CTRL0A &= ~(1<<0);
#endif
#else
	rSYS_PAD_CTRL09 &= 0x3FFFFFFF;
	rSYS_PAD_CTRL09 |= (2 << 30);
#endif

	OS_DecRI();	// enable interrupt			
}


void RemoteKeyInit(void)
{	
#ifdef _XMSYS_REMOTE_CONTROL_SUPPORT_	
	cRemoteKey = REMOTE_NULL;
	cRemoteStatus = NORMAL_KEY;

	sys_clk_disable(rcrt_pclk_enable);
	sys_clk_disable(rcrt_clk_out_enable);
	sys_soft_reset(softreset_rcrt);
	sys_clk_enable (rcrt_clk_out_enable);
	sys_clk_enable (rcrt_pclk_enable);
	
	select_remote_pad();
	remote_config();
	
	SetRemoteKeyRepeateInterval(1000);
	Remotekey_state = REMOTE_SCAN;
	//register remote irq
	request_irq(RCRT_INT, PRIORITY_FIFTEEN, remote_int_handler);
	
	XM_printf("RemoteKeyInit\n");
	
#endif
}

void RemoteKeyExit (void)
{
#ifdef _XMSYS_REMOTE_CONTROL_SUPPORT_	
	irq_disable (RCRT_INT);
#endif
}




