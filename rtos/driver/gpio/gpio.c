
/***********************************************************************
*Copyright (c)2012  Arkmicro Technologies Inc. 
*All Rights Reserved 
*
*Filename:    GPIO.c
*Version :    1.0 
*Date    :    2011.10.15
*Author  :    ls
*Abstract:    Ark1630 soc gpio   
*History :     
* 
*Version :    2.0 
*Date    :    2012.02.27
*Author  :    ls 
*Abstract:    ark1660  Ver1.0 MPW driver remove waring
*History :    1.0
*
*Version :    2.0 
*Date    :    2012.07.30
*Author  :    ls 
*Abstract:    ark1660    MPW2  gpio  driver  
*History :    1.0
************************************************************************/
 

#include "types.h"
#include "ark1960.h"
#include "gpio.h"
#include "printk.h"
#include "irqs.h"
#include "string.h"


static void (*lg_fnGPIOPadIRQHandlers[MAX_GPIO_PAD])(void); 

/********************************************************************************
 * 2 变量定义
 */

/********************************************************************************
 * 3 外部数据声明
 */

/********************************************************************************
 * 4 外部函数声明
 *
 */

/********************************************************************************
 * 5 函数定义
 */ 
static void GPIOIntHander(void);

/********************************************************************************
 函数功能：设置GPIO 端口去抖时间长度
 输入参数：   
 	ulGPIO_Pad: 		GPIO 端口号
 	debance_ms:		去抖时间长度，以毫秒为单位
 返回值:
 	0:		操作成功
 	-1:		去抖时间长度太长，寄存器值溢出
 	-2:		GPIO 端口不正确或者指定端口不具备去抖功能，只
 			有GPIO_S_0-GPIO_S_7 之间的八个GPIO Pad 脚有去抖功能

函数使用注意事项:
	去抖时间计算如下:
		(1/32.768k) *  debance_count = debance_ms

	配置100ms debance举例:
		SetGpioDebounce(GPIO_S_xx, 100)
		1,(1/33k)*debance_count = 100ms => debance_count = 33kx100ms = 3300 


	配置125msdebance举例:
		SetGpioDebounce(GPIO_S_xx, 125)
		(1/33)*debance_count = 125 => debance_count = 4125 
 *******************************************************************************/
INT32   SetGpioDebounce(UINT32 ulGPIO_Pad,UINT32 debance_ms)
{
	INT32 ret = 0;
	
	unsigned int debance_count;

	if(ulGPIO_Pad < GPIO_S_0 || ulGPIO_Pad > GPIO_S_7)
	{
		printk("config gpio error!!\n");
		return -2;
	}
	// 24MHz debounce clock
	debance_count = 24000*debance_ms;
	if(debance_count>0xffffff)
	{
		printk("config debance_count error!!\n");
		return -1;
	}

	
	switch(ulGPIO_Pad)
	{
		case GPIO_S_0:
			rGPIO_DEBOUNCE_CNT_0 = debance_count;
			break;
			
		case GPIO_S_1:
			rGPIO_DEBOUNCE_CNT_1= debance_count;
			break;
			
		case GPIO_S_2:
			rGPIO_DEBOUNCE_CNT_2= debance_count;
			break;
			
		case GPIO_S_3:
			rGPIO_DEBOUNCE_CNT_3= debance_count;
			break;
			
		case GPIO_S_4:
			rGPIO_DEBOUNCE_CNT_4= debance_count;
			break;
			
		case GPIO_S_5:
			rGPIO_DEBOUNCE_CNT_5 = debance_count;	 
			break;
			
		case GPIO_S_6:
			rGPIO_DEBOUNCE_CNT_6= debance_count;
			break;
			
		case GPIO_S_7:
			rGPIO_DEBOUNCE_CNT_7= debance_count;
			break;
	}
//	rGPIO_DEBOUNCE_ENABLE&=~(1<<ulGPIO_Pad);
	rGPIO_DEBOUNCE_ENABLE|= (1<<(ulGPIO_Pad-GPIO_BANK_S_START)); 		

	return ret;
}

/********************************************************************************
 函数功能：初始化GPIO 模块
 输入参数：   
	无
 返回值: 
 	无
 *******************************************************************************/
 void InitGPIO(void)
{
	// SYS_DEVICE_CLK_CFG0
	// 27	R/W	1	Gpio_debnc_clk_sel
	//				1:clk_24m
	//				0:rtc_clk
	// rSYS_DEVICE_CLK_CFG0 &= ~(1 << 27);  // Select gpio0-7 pad debounce clock source as rtc 32.768k
	rSYS_DEVICE_CLK_CFG0 |= (1 << 27);  // Select gpio0-7 pad debounce clock source as 24M
	memset(lg_fnGPIOPadIRQHandlers, 0, sizeof(lg_fnGPIOPadIRQHandlers));
	request_irq(GPIO_INT, PRIORITY_FIFTEEN, GPIOIntHander);
}

/********************************************************************************
 函数功能：注册GPIO 端口中断处理函数
 输入参数：   
 	ulGPIO_Pad: 		GPIO 端口号
 	euLevel:			GPIO 中断触发电平
 	fn_irq_handler:	GPIO 端口中断响应函数
 返回值:
 	0:		操作成功
 	-1:		GPIO0-GPIO15不能作为输入脚，因此不会响应中断
 	-2:		该GPIO 端口号不存在
 	-3:		中断触发电平不正确
 	-4:		中断处理函数不能为空
 *******************************************************************************/
INT32 GPIO_IRQ_Request(UINT32 ulGPIO_Pad, EU_TRIG_LEVEL euLevel, void(*fn_irq_handler)(void))
{
	INT32 ret = 0;

	if(fn_irq_handler == NULL)
		return -4;
	
	if(ulGPIO_Pad < MAX_GPIO_PAD)
	{
		if(euLevel == euLowLevelTrig)
		{
			if(ulGPIO_Pad <= GPIO_BANK_A_END)
			{
				//if(ulGPIO_Pad < GPIO16)
				//	return -1;
				
				rGPIO_PA_LEVEL &= ~(1<<ulGPIO_Pad);
			}
			
			else if(ulGPIO_Pad <= GPIO_BANK_B_END)
				rGPIO_PB_LEVEL &= ~(1<<(ulGPIO_Pad-GPIO_BANK_B_START));
			
			else if(ulGPIO_Pad <= GPIO_BANK_C_END)
				rGPIO_PC_LEVEL &= ~(1<<(ulGPIO_Pad-GPIO_BANK_C_START));			
			
			else if(ulGPIO_Pad <= GPIO_BANK_D_END)
				rGPIO_PD_LEVEL &= ~(1<<(ulGPIO_Pad-GPIO_BANK_D_START));

			else 
				rGPIO_PA_LEVEL &= ~(1<<(ulGPIO_Pad-GPIO_BANK_S_START));			
		}

		else if(euLevel == euHightLevelTrig )
		{
			if(ulGPIO_Pad <= GPIO_BANK_A_END)
				rGPIO_PA_LEVEL |= (1<<ulGPIO_Pad);
				
			else if(ulGPIO_Pad <= GPIO_BANK_B_END)
				rGPIO_PB_LEVEL |= (1<<(ulGPIO_Pad-GPIO_BANK_B_START));
			
			else if(ulGPIO_Pad <= GPIO_BANK_C_END)
				rGPIO_PC_LEVEL |= (1<<(ulGPIO_Pad-GPIO_BANK_C_START));				
			
			else if(ulGPIO_Pad <= GPIO_BANK_D_END)
				rGPIO_PD_LEVEL |= (1<<(ulGPIO_Pad-GPIO_BANK_D_START));

			else 
				rGPIO_PA_LEVEL |= (1<<(ulGPIO_Pad-GPIO_BANK_S_START));
		}
		
		else
		{
			printk("Please input the correct GPIO interrupt triggle level!\n");
			ret = -3;
		}

		if(ret == 0)
		{
			if(ulGPIO_Pad >= GPIO_BANK_S_START && ulGPIO_Pad <= GPIO_BANK_S_END)
			{
				lg_fnGPIOPadIRQHandlers[ulGPIO_Pad-GPIO_BANK_S_START] = fn_irq_handler;
			}
			else
			{
				lg_fnGPIOPadIRQHandlers[ulGPIO_Pad] = fn_irq_handler;
			}
			EnableGPIOPadIRQ(ulGPIO_Pad);
		}
	}

	else
	{
		printk("Please input the correct GPIO pad(GPIO0-GPIO_S_15),not others!\n");
		ret = -2;
	}

	return ret;
}

/******************************************************************************** 
 函数功能：打开GPIO 脚响应中断
 输入参数：   
 	ulGPIO_Pad: 		GPIO 端口号
 返回值:
 	0:		操作成功
 	-1:		GPIO0-GPIO15不能作为输入脚，因此不会响应中断
 	-2:		该GPIO 端口号不存在
 *******************************************************************************/
INT32 EnableGPIOPadIRQ(UINT32 ulGPIO_Pad)
{
	INT32 ret = 0;
	
	if(ulGPIO_Pad <= GPIO_BANK_A_END)
	{
		//if(ulGPIO_Pad < GPIO16)
		//	return -1;
		
		rGPIO_PA_INTEN |= (1<<ulGPIO_Pad);
	}
	
	else if(ulGPIO_Pad <= GPIO_BANK_B_END)
		rGPIO_PB_INTEN |= (1<<(ulGPIO_Pad-GPIO_BANK_B_START));
	
	else if(ulGPIO_Pad <= GPIO_BANK_C_END)
		rGPIO_PC_INTEN |= (1<<(ulGPIO_Pad-GPIO_BANK_C_START));
	
	else if(ulGPIO_Pad <= GPIO_BANK_D_END)
		rGPIO_PD_INTEN |= (1<<(ulGPIO_Pad-GPIO_BANK_D_START));

	else if(ulGPIO_Pad <= GPIO_BANK_S_END)
		rGPIO_PA_INTEN |= (1<<(ulGPIO_Pad-GPIO_BANK_S_START));
	
	else
	{
		printk("Please input the correct GPIO pad(GPIO0-GPIO_S_15),not others!\n");
		ret = -2;
	}

	return ret;
}

/********************************************************************************
 函数功能：禁止GPIO 脚响应中断
 参数：   
 	ulGPIO_Pad: 		GPIO 端口号
 返回值:
 	0:		操作成功
 	-1:		GPIO0-GPIO15不能作为输入脚，因此不会响应中断
 	-2:		该GPIO 端口号不存在
 *******************************************************************************/
INT32 DisableGPIOPadIRQ(UINT32 ulGPIO_Pad)
{
	INT32 ret = 0;
	
	if(ulGPIO_Pad <= GPIO_BANK_A_END)
	{
		//if(ulGPIO_Pad < GPIO16)
		//	return -1;
		
		rGPIO_PA_INTEN &= ~(1<<ulGPIO_Pad);
	}
	
	else if(ulGPIO_Pad <= GPIO_BANK_B_END)
		rGPIO_PB_INTEN &= ~(1<<(ulGPIO_Pad-GPIO_BANK_B_START));
	
	else if(ulGPIO_Pad <= GPIO_BANK_C_END)
		rGPIO_PC_INTEN &= ~(1<<(ulGPIO_Pad-GPIO_BANK_C_START));
	
	else if(ulGPIO_Pad <= GPIO_BANK_D_END)
		rGPIO_PD_INTEN &= ~(1<<(ulGPIO_Pad-GPIO_BANK_D_START));

	else if(ulGPIO_Pad <= GPIO_BANK_S_END)
		rGPIO_PA_INTEN &= ~(1<<(ulGPIO_Pad-GPIO_BANK_S_START));
	
	else
	{
		printk("Please input the correct GPIO pad(GPIO0-GPIO_S_15),not others!\n");
		ret = -2;
	}

	return ret;
}

/********************************************************************************
 函数功能：GPIO 模块中断处理函数
 输入参数：   无
 返回值：无
 *******************************************************************************/
static void GPIOIntHander(void)
{
	UINT32 i;
	UINT32 ulPA_Pend;
	UINT32 ulPB_Pend;
	UINT32 ulPC_Pend;
	UINT32 ulPD_Pend;

	/*reset value of rGPIO_PX_PEND is 0*/
	ulPA_Pend = rGPIO_PA_PEND;
	ulPB_Pend = rGPIO_PB_PEND;	
	ulPC_Pend = rGPIO_PC_PEND;
	ulPD_Pend = rGPIO_PD_PEND;
	
	if(ulPA_Pend)
	{
		for(i=0;i<32;i++)
		{
			if(ulPA_Pend&(1<<i))
			{
//				printk("Call gpio pad interrupt handler on GPIO Bank A pad %d\n",i);
				if(lg_fnGPIOPadIRQHandlers[i+GPIO_BANK_A_START])
				{
					lg_fnGPIOPadIRQHandlers[i+GPIO_BANK_A_START]();				
				}
				rGPIO_PA_PEND &= ~(1<<i);            //clear GPIO_PEND
			}
		
		}
	}

	if(ulPB_Pend)
	{
		for(i=0;i<32;i++)
		{
			if(ulPB_Pend&(1<<i))
			{
//				printk("Call gpio pad interrupt handler on GPIO Bank B pad %d\n",i+GPIO_BANK_B_START);
				if(lg_fnGPIOPadIRQHandlers[i+GPIO_BANK_B_START])
				{
					lg_fnGPIOPadIRQHandlers[i+GPIO_BANK_B_START]();				
				}				
				rGPIO_PB_PEND &= ~(1<<i);//clear GPIO_PEND
			}
		
		}
	}

	if(ulPC_Pend)
	{
		for(i=0;i<32;i++)
		{
			if(ulPC_Pend&(1<<i))
			{
//				printk("Call gpio pad interrupt handler on GPIO Bank C pad %d\n",i+GPIO_BANK_C_START);
				if(lg_fnGPIOPadIRQHandlers[i+GPIO_BANK_C_START])
				{
					lg_fnGPIOPadIRQHandlers[i+GPIO_BANK_C_START]();				
				}				
				rGPIO_PC_PEND &= ~(1<<i);//clear GPIO_PEND
			}
		
		}
	}
	
	if(ulPD_Pend)
	{
		for(i=0;i<32;i++)
		{
			if(ulPD_Pend&(1<<i))
			{
//				printk("Call gpio pad interrupt handler on GPIO Bank D pad %d\n",i+GPIO_BANK_D_START);
				if(lg_fnGPIOPadIRQHandlers[i+GPIO_BANK_D_START])
				{
					lg_fnGPIOPadIRQHandlers[i+GPIO_BANK_D_START]();				
				}				
				rGPIO_PD_PEND &= ~(1<<i);//clear GPIO_PEND
			}
		
		}
	}	
}

/********************************************************************************
 函数功能：设置GPIO 端口输入输出方向
 输入参数：   
 	ulGPIO_Pad:	GPIO 端口号
 	euDirection:	0输入，1输出
 返回值：
  	0: 设置成功
  	-1: 	GPIO0-GPIO15不能作为输入脚
 	-2: 	GPIO 端口号不存在
 	-3: 管脚输入输出方向参数不正确
*******************************************************************************/
INT32 SetGPIOPadDirection(UINT32 ulGPIO_Pad, EU_GPIO_Direction euDirection)
{
	INT32 ret = 0;

	if(euDirection == euInputPad)
	{
		if((ulGPIO_Pad<=GPIO_BANK_A_END) && (ulGPIO_Pad>=GPIO_BANK_A_START))
		{
			//if(ulGPIO_Pad < GPIO16)
			{
				//printk("you have select special gpio pad, then this mux gpio pad:%d can not be configed as input pad\r\n",ulGPIO_Pad);
				//return -1;
			}
			rGPIO_PA_MOD |=(1<<(ulGPIO_Pad-GPIO_BANK_A_START));			//high-- input
		}
		
		else if(ulGPIO_Pad<=GPIO_BANK_B_END)
			rGPIO_PB_MOD |=(1<<(ulGPIO_Pad-GPIO_BANK_B_START));

		else if(ulGPIO_Pad<=GPIO_BANK_C_END)
			rGPIO_PC_MOD |=(1<<(ulGPIO_Pad-GPIO_BANK_C_START));
		
		else if(ulGPIO_Pad<=GPIO_BANK_D_END)
			rGPIO_PD_MOD |=(1<<(ulGPIO_Pad-GPIO_BANK_D_START));

		else if(ulGPIO_Pad <= GPIO_BANK_S_END)
			rGPIO_PA_MOD |=(1<<(ulGPIO_Pad-GPIO_BANK_S_START));			//high-- input
		else
		{
			printk("Please input the correct GPIO pad(GPIO0-GPIO_S_15),not others!\n");
			ret = -2;
		}
	}
	
	else if(euDirection == euOutputPad)
	{
		if((ulGPIO_Pad<=GPIO_BANK_A_END) && (ulGPIO_Pad>=GPIO_BANK_A_START))
			rGPIO_PA_MOD &= ~(1<<(ulGPIO_Pad-GPIO_BANK_A_START));//low-- ouput
		
		else if(ulGPIO_Pad<=GPIO_BANK_B_END)
			rGPIO_PB_MOD &= ~(1<<(ulGPIO_Pad-GPIO_BANK_B_START));

		else if(ulGPIO_Pad<=GPIO_BANK_C_END)
			rGPIO_PC_MOD &= ~(1<<(ulGPIO_Pad-GPIO_BANK_C_START));
		
		else if(ulGPIO_Pad<=GPIO_BANK_D_END)
			rGPIO_PD_MOD &= ~(1<<(ulGPIO_Pad-GPIO_BANK_D_START));

		else if(ulGPIO_Pad<=GPIO_BANK_S_END)
			rGPIO_PA_MOD &= ~(1<<(ulGPIO_Pad-GPIO_BANK_S_START));
		
		else
		{
			printk("Please input the correct GPIO pad(GPIO0-GPIO_S_15),not others!\n");
			ret = -2;
		}
	}

	else
	{
		printk("Please input the correct GPIO direction!\n");
		ret = -3;
	}

	return ret;
}

/********************************************************************************
 函数功能：用于设置管脚输出状态
 输入参数：    ulGPIO_Pad IO编号
 			euData 管脚输出电平高低
 返回值：
 	0: 设置成功
 	-2: GPIO 管脚编号不正确
 	-3: 管脚输出电平参数不正确
********************************************************************************/
INT32 SetGPIOPadData(UINT32 ulGPIO_Pad, EU_GPIO_Data euData)
{
	INT32 ret = 0;
	
	if(euData == euDataHigh)
	{
	
		if(ulGPIO_Pad<=GPIO_BANK_A_END)
			rGPIO_PA_RDATA |=(1<<ulGPIO_Pad);//high-- input
			
		else if(ulGPIO_Pad<=GPIO_BANK_B_END)
			rGPIO_PB_RDATA |=(1<<(ulGPIO_Pad-GPIO_BANK_B_START));

		else if(ulGPIO_Pad<=GPIO_BANK_C_END)
			rGPIO_PC_RDATA |=(1<<(ulGPIO_Pad-GPIO_BANK_C_START));
		
		else if(ulGPIO_Pad<=GPIO_BANK_D_END)
			rGPIO_PD_RDATA |=(1<<(ulGPIO_Pad-GPIO_BANK_D_START));		

		else if(ulGPIO_Pad<=GPIO_BANK_S_END)
			rGPIO_PA_RDATA |=(1<<(ulGPIO_Pad-GPIO_BANK_S_START));		
		
		else
		{
			printk("Please input the correct GPIO pad(GPIO0-GPIO_S_15),not others!\n");
			ret = -2;
		}
	}

	else if(euData == euDataLow)
	{
		if(ulGPIO_Pad<=GPIO_BANK_A_END)
			rGPIO_PA_RDATA &= ~(1<<ulGPIO_Pad);//high-- input
	
		else if(ulGPIO_Pad<=GPIO_BANK_B_END)
			rGPIO_PB_RDATA &= ~(1<<(ulGPIO_Pad-GPIO_BANK_B_START));

		else if(ulGPIO_Pad<=GPIO_BANK_C_END)
			rGPIO_PC_RDATA &= ~(1<<(ulGPIO_Pad-GPIO_BANK_C_START));
		
		else if(ulGPIO_Pad<=GPIO_BANK_D_END)
			rGPIO_PD_RDATA &= ~(1<<(ulGPIO_Pad-GPIO_BANK_D_START));

		else if(ulGPIO_Pad<=GPIO_BANK_S_END)
			rGPIO_PA_RDATA &= ~(1<<(ulGPIO_Pad-GPIO_BANK_S_START));

		else
		{
			printk("Please input the correct GPIO pad(GPIO0-GPIO_S_15),not others!\n");
			ret = -2;
		}
		
	}

	else
	{
		printk("Please input the correct GPIO data value(0-1)!\n");
		ret = -3;
	}

	return ret;
}

/********************************************************************************
 函数功能：用于获取管脚输入状态
 输入参数： 
	ulGPIO_Pad:	GPIO 端口号
 返回值：
	若GPIO 端口号不正确返回euDataNone，否则返回正确的电平
 *******************************************************************************/
EU_GPIO_Data GetGPIOPadData(UINT32 ulGPIO_Pad)
{
	EU_GPIO_Data ret;
	
	if(ulGPIO_Pad <= GPIO_BANK_A_END)
		ret = (EU_GPIO_Data)((rGPIO_PA_RDATA>>(ulGPIO_Pad - GPIO_BANK_A_START)) & 0x01);	
	
	else if(ulGPIO_Pad <= GPIO_BANK_B_END)
		ret = (EU_GPIO_Data)((rGPIO_PB_RDATA>>(ulGPIO_Pad - GPIO_BANK_B_START)) & 0x01);		
	
	else if(ulGPIO_Pad <= GPIO_BANK_C_END)
		ret = (EU_GPIO_Data)((rGPIO_PC_RDATA>>(ulGPIO_Pad - GPIO_BANK_C_START)) & 0x01);		
	
	else if(ulGPIO_Pad <= GPIO_BANK_D_END)
		ret = (EU_GPIO_Data)((rGPIO_PD_RDATA>>(ulGPIO_Pad - GPIO_BANK_D_START)) & 0x01);		

	else if(ulGPIO_Pad <= GPIO_BANK_S_END)
		ret = (EU_GPIO_Data)((rGPIO_PA_RDATA>>(ulGPIO_Pad - GPIO_BANK_S_START)) & 0x01);		
	
	else
	{
		printk("Please input the correct GPIO pad(GPIO0-GPIO_S_15),not others!\n");
		ret = euDataNone;
	}

	return ret;
}

/********************************************************************************
 函数功能：获取指定GPIO 端口输入输出方向
 参数:
 	ulGPIO_Pad: GPIO 端口编号
 返回值:
	若GPIO 端口号不正确返回euNoneSettingPad，否则返回输入输出
	方向
 ********************************************************************************/
EU_GPIO_Direction GetGPIODataDirection(UINT32 ulGPIO_Pad)
{
	EU_GPIO_Direction ret;
	
	if(ulGPIO_Pad <= GPIO_BANK_A_END)
		ret = (EU_GPIO_Direction)((rGPIO_PA_MOD>>(ulGPIO_Pad-GPIO_BANK_A_START))&0x01);	
	
	else if(ulGPIO_Pad <= GPIO_BANK_B_END)
		ret = (EU_GPIO_Direction)((rGPIO_PB_MOD>>(ulGPIO_Pad-GPIO_BANK_B_START))&0x01);		
	
	else if(ulGPIO_Pad <= GPIO_BANK_C_END)
		ret = (EU_GPIO_Direction)((rGPIO_PC_MOD>>(ulGPIO_Pad-GPIO_BANK_C_START))&0x01);		
	
	else if(ulGPIO_Pad <= GPIO_BANK_D_END)
		ret = (EU_GPIO_Direction)((rGPIO_PD_MOD>>(ulGPIO_Pad-GPIO_BANK_D_START))&0x01);		

	else if(ulGPIO_Pad <= GPIO_BANK_S_END)
		ret = (EU_GPIO_Direction)((rGPIO_PA_MOD>>(ulGPIO_Pad-GPIO_BANK_S_START))&0x01);	
	
	else
	{
		printk("Please input the correct GPIO pad(GPIO0-GPIO_S_15),not others!\n");
		ret = euNoneSettingPad;
	}

	return ret;
}


/********************************************************************************
 函数功能：设置指定GPIO 端口的复用脚
 参数:
 	ulGPIO_Pad: GPIO 端口编号
 返回值:
	无
 函数使用注意事项:
 	其中GPIO85 连接到bias_fb_p_pad脚，只能做输出
 	GPIO86连接到bias_ovp_p_pad脚，只能做输出
 	GPIO0-GPIO15只能做输出，不能做输入
 ********************************************************************************/
void Select_GPIO_Pad(UINT32 ulGPIO_Pad)
{
	UINT32 rReg;

	if(ulGPIO_Pad == GPIO0)
	{
		rReg = rSYS_PAD_CTRL09;
		rReg &=~(0x3<<0);
		rSYS_PAD_CTRL09= rReg;		
	}
	else if(ulGPIO_Pad == GPIO1)
	{
		rReg = rSYS_PAD_CTRL09;
		rReg &=~(0x3<<2);
		rSYS_PAD_CTRL09= rReg;		
	}
	else if(ulGPIO_Pad <= GPIO9)
	{
/*===================================================
独立GPIO0-15  can used to input/output(0-7 have debounce)
复用GPIO0-15  (eg:reused LCD) only output
because our project used debance, we used to "no reused function" IO
====================================================*/
		rReg = rSYS_PAD_CTRL00;
		rReg &=~(0xf<<((ulGPIO_Pad - GPIO2)*4));
		rSYS_PAD_CTRL00= rReg;		
	}
	else if(ulGPIO_Pad <= GPIO17)
	{
		rReg = rSYS_PAD_CTRL01;
		rReg &=~(0xf<<((ulGPIO_Pad -GPIO10)*4));
		rSYS_PAD_CTRL01= rReg;		
	}
	else if(ulGPIO_Pad <= GPIO25)
	{
		rReg = rSYS_PAD_CTRL02;
		rReg &=~(0xf<<((ulGPIO_Pad -GPIO18)*4));
		rSYS_PAD_CTRL02= rReg;		
	}
	else if(ulGPIO_Pad <= GPIO29)
	{
		rReg = rSYS_PAD_CTRL03;
		rReg &=~(0xf<<((ulGPIO_Pad -GPIO26)*4));
		rSYS_PAD_CTRL03= rReg;		
	}
	else if(ulGPIO_Pad <= GPIO38)
	{
		rReg = rSYS_PAD_CTRL07;
		rReg &=~(0x3<<((ulGPIO_Pad -GPIO30)*2));
		rSYS_PAD_CTRL07= rReg;		
	}
	else if(ulGPIO_Pad <= GPIO46)
	{
		rReg = rSYS_PAD_CTRL04;
		rReg &=~(0xf<<((ulGPIO_Pad -GPIO39)*4));
		rSYS_PAD_CTRL04= rReg;		
	}
	else if(ulGPIO_Pad <= GPIO54)
	{
		rReg = rSYS_PAD_CTRL05;
		rReg &=~(0xf<<((ulGPIO_Pad -GPIO47)*4));
		rSYS_PAD_CTRL05= rReg;		
	}
	else if(ulGPIO_Pad <= GPIO61)
	{
		rReg = rSYS_PAD_CTRL06;
		rReg &=~(0xf<<((ulGPIO_Pad -GPIO55)*4));
		rSYS_PAD_CTRL06= rReg;		
	}
	else if(ulGPIO_Pad <= GPIO71)
	{
		rReg = rSYS_PAD_CTRL08;
		rReg &=~(0x3<<((ulGPIO_Pad -GPIO62)*2));
		rSYS_PAD_CTRL08= rReg;		
	}
	else if(ulGPIO_Pad <= GPIO84)
	{
		rReg = rSYS_PAD_CTRL09;
		rReg &=~(0x3<<((ulGPIO_Pad -GPIO72)*2+4));
		rSYS_PAD_CTRL09= rReg;		
	}
	else if(ulGPIO_Pad <= GPIO86)
	{
		//在我们的系统中，GPIO85有两个PAD脚可供连接，分别是bias_fb_p_pad和BGA289_sig1_pad，如
		//果GPIO85做输入脚则只能连接到BGA289_sig1_pad，若做输出，则两脚都可以。在目前的BSP中，
		//BGA289_sig1_pad脚被复用为sd0的卡探测脚，因此GPIO85目前只能做输出复用到bias_fb_p_pad脚
		//和PWM4的输出脚复用
		
		//在我们的系统中，GPIO86有两个PAD脚可供连接，分别是bias_ovp_p_pad和BGA289_sig2_pad，如
		//果GPIO86做输入脚则只能连接到BGA289_sig2_pad，若做输出，则两脚都可以。在目前的BSP中，
		//BGA289_sig2_pad脚被复用为sd0的数据线d0，因此GPIO86目前只能做输出复用到bias_ovp_p_pad脚
		//和PWM4的输出脚复用		
		rReg = rSYS_PAD_CTRL09;
		rReg &=~(0x1<<((ulGPIO_Pad -GPIO85)+30));
		rSYS_PAD_CTRL09= rReg;		
	}
	else if(ulGPIO_Pad <= GPIO117)
	{
		rReg = rSYS_PAD_CTRL0A;
		//rReg &=~(0x1<<(ulGPIO_Pad -GPIO87 + 2));
		rReg &=~(0x3<<((ulGPIO_Pad -GPIO116 )*2));
		rSYS_PAD_CTRL0A= rReg;	
	}
	else if(ulGPIO_Pad <= GPIO127)
	{
		rReg = rSYS_PAD_CTRL0C;
		rReg &=~(0x1<<(ulGPIO_Pad -GPIO117));
		rSYS_PAD_CTRL0C= rReg;	
	}
	else
	{
		
	}		
}

/********************************************************************************
 函数功能：设置指定GPIO 端口的输出电平
 参数:
 	ulGPIO_Pad: GPIO 端口编号
 	IOstate_high_Low: 输出电平
 返回值:
  	0: 设置成功
  	-1: 	GPIO0-GPIO15不能作为输入脚
 	-2: 	GPIO 端口号不存在
  	-3: 管脚输出电平参数不正确
 ********************************************************************************/
INT32 Set_GPIO_Output(UINT32  GpioNum, EU_GPIO_Data IOstate_high_Low )
{
	INT32 ret = 0;
	if(GpioNum >= GPIO0 && GpioNum <= MAX_GPIO_NUM)
	{			
		Select_GPIO_Pad(GpioNum);
		ret = SetGPIOPadDirection(GpioNum,euOutputPad);// set Gpio is output	
		if(ret == 0)
			ret = SetGPIOPadData(GpioNum,IOstate_high_Low);	
	}
	else
		ret = -2;

	return ret;
}

/********************************************************************************
 函数功能：设置指定GPIO 端口为输入端口
 参数:
 	ulGPIO_Pad: GPIO 端口编号
 	IOstate_high_Low: 输出电平
 返回值:
  	0: 设置成功
  	-1: 	GPIO85, GPIO86, GPIO0-GPIO15不能作为输入脚
 	-2: 	GPIO 端口号不存在
  	-3: 管脚输出电平参数不正确

  函数使用注意事项:
  	其中GPIO85, GPIO86, GPIO0-GPIO15脚只能做输出用，因此调用	这个
  	函数设置这两个脚为输入脚将无效
 ********************************************************************************/
INT32 Set_GPIO_Input(UINT32  GpioNum)
{
	INT32 ret;
	
	if(GpioNum == GPIO85 || GpioNum == GPIO86)
	{
		return -1;
	}
	else
	{
		if(GpioNum >= GPIO0 && GpioNum <= MAX_GPIO_NUM)
		{		
			Select_GPIO_Pad(GpioNum);
			ret = SetGPIOPadDirection(GpioNum,euInputPad);
		}
		else
			ret = -2;
	}

	return ret;
}


