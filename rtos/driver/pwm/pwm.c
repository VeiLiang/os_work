#include "hardware.h"
#include "RTOS.h"
#include "pwm.h"
#include "cpuclkcounter.h"

/* Register access macros */
#define pwm_readl(reg, ch) 			*(volatile unsigned int *)( PWMx_BASE(ch)+ reg)
#define pwm_writel(value,reg, ch)	(*(volatile unsigned int *)( PWMx_BASE(ch)+reg) = value)
#define pwm_clear(value,reg, ch)	(*(volatile unsigned int *)( PWMx_BASE(ch)+reg) &= ~value )

#define  PWM_MAINCLK       35000000

//共 两  组 PWM 通道  4*2 = 8 通道PWM
#define 	XM_PWM_GROUP_0       0		// 0
#define 	XM_PWM_GROUP_1       4		// 1

#define 	NEW_SEL_PAD_2014_11_18     1

//base  address
#define 	PWM_CMD			    0x0  
#define 	PWM_DUTY			0x4 //set the high level width
#define 	PWM_CNTDIV		    0x8 //clock div and cnt for duty
#define 	PWM_PULSE_NUM	    0xC //clock div and cnt for duty


//command
#define 	PWM_CMD_ena	             (1<<0)  // 通道使能
#define 	PWM_CMD_activate         (1<<2)  // 
#define 	PWM_CMD_intr_en          (1<<3)  // <结束中断>使能
#define 	PWM_CMD_polarity         (1<<4)  // 极性

//int status check bit         在各自的cmd上都可查询状态
#define  PWM_GROUP0_BASE          (PWM_BASE )
#define  PWM_INT_PWM0             (1<<27) //PWM_CMD bit26 对应  查询PWM0状态 是否是<PWM0>产生了<结束中断>
#define  PWM_INT_PWM1             (1<<28) //PWM_CMD bit27 对应 PWM1 查询状态
#define  PWM_INT_PWM2             (1<<29) //PWM_CMD bit28 对应 PWM2 
#define  PWM_INT_PWM3             (1<<30) //PWM_CMD bit29 对应 PWM3
#define  PWM_GROUP0_OPEN          (PWM_BASE +0x58)

//低4位代表每个通道 
#define  PWM_GROUP1_BASE          (PWM_BASE +0x80)
#define  PWM_INT_PWM4             (1<<27) //PWM_CMD bit26 对应  查询PWM4状态 是否是<PWM0>产生了<结束中断>
#define  PWM_INT_PWM5             (1<<28) //PWM_CMD bit27 对应 PWM5 查询状态
#define  PWM_INT_PWM6             (1<<29) //PWM_CMD bit28 对应 PWM6 
#define  PWM_INT_PWM7             (1<<30) //PWM_CMD bit29 对应 PWM7
#define  PWM_GROUP1_OPEN          (PWM_GROUP1_BASE+0x58 )


int PWMx_BASE(int ch)
{
	if(ch<4)
		return (PWM_GROUP0_BASE+(ch<<4));
	else
		return (PWM_GROUP1_BASE+((ch-4)<<4));
}

static void SelectPadForPwm(int channel )
{
	UINT32 val;
	// 2014-11-18所使用的padlist
	// pwm_out[7] 该引脚用于时钟，所以暂时无法检测
	// 测波形顺序 TP3 -->TP9 7个 TP10是时钟，测试不了 
	if(channel < 4 )
	{
		val = rSYS_PAD_CTRL01;
		val &= ~(0x7<<(3+(channel*3)));   // value=16 + x*2 : 16 17; 18 19; 20 21; 22 23
		val |= (0x3<<(3+(channel*3)));       //choose one pad of the channel 0
		rSYS_PAD_CTRL01 = val;
	}
	else
	{
		if(channel == 7)
		{
			printf("channel 7 use for clock .Cann't use for PWM now!!!\r\n");
			return;
		}
		val = rSYS_PAD_CTRL02;
		//PWM4: from bit.18~20 (6+4*3=18)
		//PWM5: from bit.21~23 (6+5*3=21)
		//PWM6: from bit.24~26 (6+6*3=24)
		//PWM7: from bit.27~29 (6+7*3=27)		
		val &= ~(0x7<<(6+(channel*3)));   // value=16 + x*2 : 16 17; 18 19; 20 21; 22 23
		val |= (0x3<<(6+(channel*3)));       //choose one pad of the channel 0
		rSYS_PAD_CTRL02 = val;
	}
}

static void UnSelectPadForPwm(int channel )
{
	UINT32 val;
	//pwm_out[7] 该引脚用于时钟，所以暂时无法检测
	if(channel < 4 )
	{
		val = rSYS_PAD_CTRL01;
		val &= ~(0x7<<(3+(channel*3)));   // value=16 + x*2 : 16 17; 18 19; 20 21; 22 23
		rSYS_PAD_CTRL01 = val;
	}
	else
	{
		if(channel == 7)
		{
			printf("channel 7 use for clock .Cann't use for PWM now!!!\r\n");
			return;
		}
		val = rSYS_PAD_CTRL02;
		//PWM4: from bit.18~20 (6+4*3=18)
		//PWM5: from bit.21~23 (6+5*3=21)
		//PWM6: from bit.24~26 (6+6*3=24)
		//PWM7: from bit.27~29 (6+7*3=27)
		val &= ~(0x7<<(6+(channel*3)));   // value=16 + x*2 : 16 17; 18 19; 20 21; 22 23
		rSYS_PAD_CTRL02 = val;
	}
}


/*******************************************************************************
*	PWM_Reset()
*
* 	1. close all pwm channel 
*	2. Unselect pin
********************************************************************************/
void PWM_close(int ch )
{
	pwm_writel(0,   PWM_CMD,       ch);
	pwm_writel(0,   PWM_DUTY,      ch); // 每周期的上沿是div的多少，duty/div数值
	pwm_writel(0,   PWM_CNTDIV,    ch);// apb主时钟/div  决定了波形周期
	pwm_writel(0,   PWM_PULSE_NUM, ch);  //共计出现多少波形
}


void PWM_Reset()
{
   sys_soft_reset (softreset_pwm);
}


void PWM_SelPad()
{
	int ch;
	for(ch=0;ch<XM_PWM_CHANNEL_COUNT;ch++ )
		SelectPadForPwm(ch);
}


void PWM_Init()
{
	UCHAR readval;
	
	*((volatile unsigned int *)(PWM_GROUP0_OPEN)) = 0xf;
	*((volatile unsigned int *)(PWM_GROUP1_OPEN)) = 0xf;
	//*((volatile unsigned int *)(PWM_GROUP0_OPEN)) = 0x23;
	//readval = *((volatile unsigned int *)PWM_GROUP0_OPEN);
	//printf("readval = 0x%x\r\n",readval);
	PWM_Reset ();
	PWM_ClkEnable ();
}


void PWM_CFG(int ch,int cmd,int div,int duty,int cycle )
{
	OS_IncDI();// don't interrupt   要求处理时间极为短暂，
	pwm_writel(0  ,   PWM_CMD,       ch); //clear  int 
	pwm_writel(duty,  PWM_DUTY,      ch); //每周期的上沿是div的多少，duty/div数值
	pwm_writel(div,   PWM_CNTDIV,    ch); //apb主时钟/div  决定了波形周期
	pwm_writel(cycle, PWM_PULSE_NUM, ch); //共计出现多少波形
	pwm_writel(cmd,   PWM_CMD,       ch);
	OS_DecRI(); // don't interrupt 
}


BOOL PWM_Enable(VOID) 
{
   // printf("PWM_Enable: %x \n",pwm_readl(PWM_CMD,0));
    if(pwm_readl(PWM_CMD,0)&0x1) {
        return TRUE;
    }
    return FALSE;
}

void PWM_OnOff(int ch,int OnOff)
{
	int val; 	//bit 0=PWM0    ------bit 3=PWM3
 
#if 0
	if(ch <4 )
		val = *((volatile unsigned int *)PWM_GROUP0_OPEN);
	else
		val = *((volatile unsigned int *)PWM_GROUP1_OPEN);
    //@Edison add ##20180921 //PWM_GROUP1_OPEN //bit 0=PWM0    ------bit 3=PWM3
	if(OnOff ){
        if(ch <4 )
		    val&=~(1<<ch);
        else 
            val&=~(1<<(ch-4));
	}else {
	    if(ch <4 )
		    val|= (1<<ch);
        else
            val|= (1<<(ch-4));
    }
    
	if(ch <4 )
		 *((volatile unsigned int *)PWM_GROUP0_OPEN) = val;
	else
		 *((volatile unsigned int *)PWM_GROUP1_OPEN) = val;

#endif


#if 0
if(ch <4 )
{
    if(OnOff)
        *((volatile unsigned int *)PWM_GROUP0_OPEN)&=~(1<<ch);
    else 
        *((volatile unsigned int *)PWM_GROUP0_OPEN)|=(1<<ch);
}
else 
{
//这样赋值也会先读取PWM_GROUP1_OPEN的状态,然后&|其他值,所以还是不能这样配置
    if(OnOff )
        *((volatile unsigned int *)PWM_GROUP1_OPEN)&= ~(1<<(ch-4));
    else
        *((volatile unsigned int *)PWM_GROUP1_OPEN)|= (1<<(ch-4));
}
#endif

	if(ch ==1 )//VCOM一直打开
	{
		*((volatile unsigned int *)(PWM_GROUP0_OPEN)) = 0x0D;
	}

	if(ch ==0 )//蜂鸣器
	{
	    if(OnOff)
	    {
	        *((volatile unsigned int *)(PWM_GROUP0_OPEN)) = 0x0C;
		}
	    else 
	    {
	        *((volatile unsigned int *)(PWM_GROUP0_OPEN)) = 0x0D;
		}
	}
}


