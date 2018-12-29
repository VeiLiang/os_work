#include "types.h"
#include "hardware.h"
#include "gpio.h"
#include "irqs.h"
#include "ark1960_testcase.h"
#include "printk.h"


static void (*lg_fnGPIOPadIRQHandlers[MAX_GPIO_PAD])(void); 


static void InitGPIO_IRQ()
{
	memset(lg_fnGPIOPadIRQHandlers, 0, sizeof(lg_fnGPIOPadIRQHandlers));
}

/*
void GPIO_IRQ_Request(UINT32 ulGPIO_Pad, EU_TRIG_LEVEL euLevel, void(*fn_irq_handler)(void))
{
	if(ulGPIO_Pad < MAX_GPIO_PAD)
	{
		if(euLevel == euLowLevelTrig)
		{
			if(ulGPIO_Pad <= GPIO_BANK_A_END)
				rGPIO_PA_LEVEL &= ~(1<<ulGPIO_Pad);
			
			else if(ulGPIO_Pad <= GPIO_BANK_B_END)
				rGPIO_PB_LEVEL &= ~(1<<(ulGPIO_Pad-GPIO_BANK_B_START));
			
			else
				rGPIO_PC_LEVEL &= ~(1<<(ulGPIO_Pad-GPIO_BANK_C_START));
			
		}

		else if(euLevel == euHightLevelTrig )
		{
			if(ulGPIO_Pad <= GPIO_BANK_A_END)
				rGPIO_PA_LEVEL |= (1<<ulGPIO_Pad);
				
			else if(ulGPIO_Pad <= GPIO_BANK_B_END)
				rGPIO_PB_LEVEL |= (1<<(ulGPIO_Pad-GPIO_BANK_B_START));
				
			else 
				rGPIO_PC_LEVEL |= (1<<(ulGPIO_Pad-GPIO_BANK_C_START));

		}
		
		else
			printk("Please input the correct GPIO interrupt triggle level!\n");

		lg_fnGPIOPadIRQHandlers[ulGPIO_Pad] = fn_irq_handler;
		EnableGPIOPadIRQ(ulGPIO_Pad);
	}

	else
		printk("Please input the correct GPIO pad(0-95),not others!\n");
	

	
}*/

/*
EU_GPIO_Data GetGPIOPadData(UINT32 ulGPIO_Pad)
{
	EU_GPIO_Data ret;
	
	if(ulGPIO_Pad <= GPIO_BANK_A_END)
		ret = rGPIO_PA_RDATA&(1<<ulGPIO_Pad);
	
	else if(ulGPIO_Pad <= GPIO_BANK_B_END)
		ret = rGPIO_PB_RDATA&(1<<(ulGPIO_Pad-GPIO_BANK_B_START));
	
	else if(ulGPIO_Pad <= GPIO_BANK_C_END)
		ret = rGPIO_PC_RDATA&(1<<(ulGPIO_Pad-GPIO_BANK_C_START));
	
	else
	{
		printk("Please input the correct GPIO pad(0-95),not others!\n");
		ret = euDataNone;
	}

	return ret;
}*/
/*
void SetGPIOPadData(UINT32 ulGPIO_Pad, EU_GPIO_Data euData)
{

	if(euData == euDataHigh)
	{
	
		if(ulGPIO_Pad<=GPIO_BANK_A_END)
			rGPIO_PA_RDATA |=(1<<ulGPIO_Pad);//high-- input
			
		else if(ulGPIO_Pad<=GPIO_BANK_B_END)
			rGPIO_PB_RDATA |=(1<<(ulGPIO_Pad-GPIO_BANK_B_START));

		else if(ulGPIO_Pad<=GPIO_BANK_C_END)
			rGPIO_PC_RDATA |=(1<<(ulGPIO_Pad-GPIO_BANK_C_START));

		else
			printk("Please input the correct GPIO pad(0-95),not others!\n");
	}

	else if(euData == euDataLow)
	{
		if(ulGPIO_Pad<=GPIO_BANK_A_END)
			rGPIO_PA_RDATA &= ~(1<<ulGPIO_Pad);//high-- input
	
		else if(ulGPIO_Pad<=GPIO_BANK_B_END)
			rGPIO_PB_RDATA &= ~(1<<(ulGPIO_Pad-GPIO_BANK_B_START));

		else if(ulGPIO_Pad<=GPIO_BANK_C_END)
			rGPIO_PC_RDATA &= ~(1<<(ulGPIO_Pad-GPIO_BANK_C_START));

		else
			printk("Please input the correct GPIO pad(0-95),not others!\n");
		
	}

	else
		printk("Please input the correct GPIO data value(0-1)!\n");
	
}
*/
void SetGPIODataDirection(UINT32 ulGPIO_Pad, EU_GPIO_Direction euDirection)
{
	

	if(euDirection == euInputPad)
	{
		if((ulGPIO_Pad<=GPIO_BANK_A_END) && (ulGPIO_Pad>=0))
			rGPIO_PA_MOD |=(1<<ulGPIO_Pad);//high-- input
		
		else if(ulGPIO_Pad<=GPIO_BANK_B_END)
			rGPIO_PB_MOD |=(1<<(ulGPIO_Pad-GPIO_BANK_B_START));

		else if(ulGPIO_Pad<=GPIO_BANK_C_END)
			rGPIO_PC_MOD |=(1<<(ulGPIO_Pad-GPIO_BANK_C_START));

		else
			printk("Please input the correct GPIO pad(0-95),not others!\n");
	}
	
	else if(euDirection == euOutputPad)
	{
		if((ulGPIO_Pad<=GPIO_BANK_A_END) && (ulGPIO_Pad>=0))
			rGPIO_PA_MOD &= ~(1<<ulGPIO_Pad);//low-- ouput
		
		else if(ulGPIO_Pad<=GPIO_BANK_B_END)
			rGPIO_PB_MOD &= ~(1<<(ulGPIO_Pad-GPIO_BANK_B_START));

		else if(ulGPIO_Pad<=GPIO_BANK_C_END)
			rGPIO_PC_MOD &= ~(1<<(ulGPIO_Pad-GPIO_BANK_C_START));

		else
			printk("Please input the correct GPIO pad(0-95),not others!\n");
	}

	else
		printk("Please input the correct GPIO direction!\n");
}
/*
EU_GPIO_Direction GetGPIODataDirection(UINT32 ulGPIO_Pad)
{
	EU_GPIO_Direction ret;
	
	if(ulGPIO_Pad <= GPIO_BANK_A_END)
		ret = rGPIO_PA_MOD&(1<<ulGPIO_Pad);
	
	else if(ulGPIO_Pad <= GPIO_BANK_B_END)
		ret = rGPIO_PB_MOD&(1<<(ulGPIO_Pad-GPIO_BANK_B_START));
	
	else if(ulGPIO_Pad <= GPIO_BANK_C_END)
		ret = rGPIO_PC_MOD&(1<<(ulGPIO_Pad-GPIO_BANK_C_START));
	
	else
	{
		printk("Please input the correct GPIO pad(0-95),not others!\n");
		ret = euNoneSettingPad;
	}

	return ret;
}*/
/*
void EnableGPIOPadIRQ(UINT32 ulGPIO_Pad)
{
	if(ulGPIO_Pad <= GPIO_BANK_A_END)
		rGPIO_PA_INTEN |= (1<<ulGPIO_Pad);
	
	else if(ulGPIO_Pad <= GPIO_BANK_B_END)
		rGPIO_PB_INTEN |= (1<<(ulGPIO_Pad-GPIO_BANK_B_START));
	
	else if(ulGPIO_Pad <= GPIO_BANK_C_END)
		rGPIO_PC_INTEN |= (1<<(ulGPIO_Pad-GPIO_BANK_C_START));

	else
		printk("Please input the correct GPIO pad(0-95),not others!\n");
		
}

void DisableGPIOPadIRQ(UINT32 ulGPIO_Pad)
{
	if(ulGPIO_Pad <= GPIO_BANK_A_END)
		rGPIO_PA_INTEN &= ~(1<<ulGPIO_Pad);
	
	else if(ulGPIO_Pad <= GPIO_BANK_B_END)
		rGPIO_PB_INTEN &= ~(1<<(ulGPIO_Pad-GPIO_BANK_B_START));
	
	else if(ulGPIO_Pad <= GPIO_BANK_C_END)
		rGPIO_PC_INTEN &= ~(1<<(ulGPIO_Pad-GPIO_BANK_C_START));
	
	else
		printk("Please input the correct GPIO pad(0-95),not others!\n");
}
*/
/*Test three GPIO pins,you can test other GPIO pin from ark1660_PADLIST_For_fpga.xls*/
void SelGPIOPad(void)
{
	unsigned int val;
	//bit[5:4] = gpio 0 in rSYS_PAD_CTRL0A
	//bit[7:6] = gpio 1
	val = rSYS_PAD_CTRL0A ;
	val&=~(0xf<<4);
	rSYS_PAD_CTRL0A = val;

	//bit[25:24] = gpio 2 in  rSYS_PAD_CTRL09
	val = rSYS_PAD_CTRL09;
	val&=~(0x3<<24);
	rSYS_PAD_CTRL09 = val;
	
	/*Bank A:SD CMD used as gpio 28*/
//	rAHB_SYS_REG2A &= ~(0xf<<0);
//	rAHB_SYS_REG2A |=(0x7<<0);

	/*Bank B:I2S LRCK(sync) used as gpio 58*/
//	rAHB_SYS_REG27 &= ~(3<<12);
//	rAHB_SYS_REG27 |=(0x1<<12);

	/*Bank C:SD CLK used as gpio 81*/
//	rAHB_SYS_REG2A &= ~(0xf<<4);
//	rAHB_SYS_REG2A |= (0x7<<4);
	
	
	//below add by tangchao 
//	rGPIO_DEBOUNCE_ENABLE &= ~(0xff<<0);// pad a - gpio:0-2
	rGPIO_DEBOUNCE_ENABLE |=(0xff<<0);
	// DEBOUNCE set 
#if 0
	rGPIO_DEBOUNCE_CNT_0 =0x10000;// max ->24bit  .eg:0x10000 ≈2 ms .clk=35 MHZ
	rGPIO_DEBOUNCE_CNT_1 =0x10000;// max ->24bit    (MAX: 0x)
	rGPIO_DEBOUNCE_CNT_2 =0x10000;// max ->24bit
#else
	rGPIO_DEBOUNCE_CNT_0 =0x7f0000;// max ->24bit  .eg:0x10000 ≈2 ms .clk=35 MHZ
	rGPIO_DEBOUNCE_CNT_1 =0x7f0000;// max ->24bit    (MAX: 0x)
	rGPIO_DEBOUNCE_CNT_2 =0x7f0000;// max ->24bit  0xffffff= 16777215 最长 0.47s 的消颤 。
#endif
}

void TestGPIOInput(void)
{
	UINT8 i; 
	UINT32 banka_old_data,bankb_old_data,bankc_old_data;
	UINT32 temp_a,temp_b,temp_c;

	banka_old_data = rGPIO_PA_RDATA;
	bankb_old_data = rGPIO_PB_RDATA;
	bankc_old_data = rGPIO_PC_RDATA;
	
	
	rGPIO_PA_MOD = 0xffffffff;//high-gpio data input,low-gpio data output
//	rGPIO_PA_MOD = 0x7;
//	rGPIO_PA_MOD = 0xe0000000;
//	rGPIO_PB_MOD = 0xffffffff;
//	rGPIO_PC_MOD = 0xffffffff;
	rGPIO_PB_MOD = 0x0;
	rGPIO_PC_MOD = 0x0;
	
	while(1)
	{
		/*if no input on GPIO,the value of temp_x is 0xffffffff,
		   if there is  input on GPIO, the correspond bit  of temp_x will be 1,
		   then detect which bit change from 0 to 1,that can judge which gpio pin has input*/
		temp_a = (~banka_old_data)^rGPIO_PA_RDATA;//
		temp_b = (~bankb_old_data)^rGPIO_PB_RDATA;
		temp_c = (~bankc_old_data)^rGPIO_PC_RDATA;
		
		//		for(i=0;i<32;i++)//gpuo:8 9会不停出信号，属于时钟信号
		for(i=0;i<4;i++)// set 9->can output 8 input
		{
			if((~temp_a)&(1<<i))
			{
				printk("GPIO has detect an input  on GPIO%d\n",i);
			}
			if((~temp_b)&(1<<i))
			{
				printk("GPIO has detect an input  on GPIO%d\n",i+32);
			}
			if((~temp_c)&(1<<i))
			{
				printk("GPIO has detect an input  on GPIO%d\n",i+64);
			}
		}
	}
}

void TestGPIOOutput(void)
{
	SetGPIODataDirection(28, euOutputPad);//high-gpio data input,low-gpio data output
	SetGPIODataDirection(58, euOutputPad);
	SetGPIODataDirection(81, euOutputPad);
	/*generate squard wave on SD_CMD(gpio28),I2S LRCK(gpio58),SD_CLK(gpio81)*/
	while(1)
	{
		SetGPIOPadData(28, euDataHigh);
		SetGPIOPadData(58, euDataHigh);
		SetGPIOPadData(81, euDataHigh);
		busy_delay(1000);
		
		SetGPIOPadData(28, euDataLow);
		SetGPIOPadData(58, euDataLow);
		SetGPIOPadData(81, euDataLow);
		busy_delay(1000);
	}
	
}

void GPIOIntHander(void)
{
	UINT32 i;
	UINT32 ulPA_Pend;
	UINT32 ulPB_Pend;
	UINT32 ulPC_Pend;

	/*reset value of rGPIO_PX_PEND is 0*/
	ulPA_Pend = rGPIO_PA_PEND;
	ulPB_Pend = rGPIO_PB_PEND;	
	ulPC_Pend = rGPIO_PC_PEND;

	if(ulPA_Pend)
	{
		for(i=0;i<32;i++)
		{
			if(ulPA_Pend&(1<<i))
			{
				printk("Call gpio pad interrupt handler on GPIO Bank A pad %d\n",i);
				if(lg_fnGPIOPadIRQHandlers[i+GPIO_BANK_A_START])
				{
					lg_fnGPIOPadIRQHandlers[i+GPIO_BANK_A_START]();				
				}
				rGPIO_PA_PEND &= ~(1<<i);//clear GPIO_PEND
			}
		
		}
	}

	if(ulPB_Pend)
	{
		for(i=0;i<32;i++)
		{
			if(ulPB_Pend&(1<<i))
			{
				printk("Call gpio pad interrupt handler on GPIO Bank B pad %d\n",i+GPIO_BANK_B_START);
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
				printk("Call gpio pad interrupt handler on GPIO Bank C pad %d\n",i+GPIO_BANK_C_START);
				if(lg_fnGPIOPadIRQHandlers[i+GPIO_BANK_C_START])
				{
					lg_fnGPIOPadIRQHandlers[i+GPIO_BANK_C_START]();				
				}
	//			ulPC_Pend &= ~(1<<i);//clear GPIO_PEND
				rGPIO_PC_PEND &= ~(1<<i);
			}
		
		}
	}
}

void GPIO0IntHandler(void)
{
	printk("Enter into GPIO-0-IntHandler\n");
}
void GPIO1IntHandler(void)
{
	printk("Enter into GPIO-1-IntHandler\n");
}
void GPIO2IntHandler(void)
{
	printk("Enter into GPIO-2-IntHandler\n");
}
void GPIO8IntHandler(void)
{
	printk("Enter into GPIO-8-IntHandler\n");
}
void GPIO9IntHandler(void)
{
	printk("Enter into GPIO-9-IntHandler\n");
}
void GPIO28IntHandler(void)
{
	printk("Enter into GPIO-28-IntHandler\n");
}

void GPIO58IntHandler(void)
{
	printk("Enter into GPIO-58-IntHandler\n");
}

void GPIO81IntHandler(void)
{
	printk("Enter into GPIO-81-IntHandler\n");
}

/*edge triggle can work,level triggle can not work*/
void TestGPIOInterrupt(void)
{
	/*init gpio handlers ,then enable GPIO interrupt on GPIO CONTROLLER*/
//	GPIO_IRQ_Request(28, euLowLevelTrig,GPIO28IntHandler);
//	GPIO_IRQ_Request(58,euLowLevelTrig,GPIO58IntHandler);
	
//	GPIO_IRQ_Request(0,euHightLevelTrig,GPIO0IntHandler);
	GPIO_IRQ_Request(0,euHightLevelTrig,GPIO0IntHandler);
	GPIO_IRQ_Request(1,euHightLevelTrig,GPIO1IntHandler);	
	GPIO_IRQ_Request(2,euHightLevelTrig,GPIO2IntHandler);
//	GPIO_IRQ_Request(8,euLowLevelTrig,GPIO8IntHandler);
//	GPIO_IRQ_Request(9,euLowLevelTrig,GPIO9IntHandler);
	//rGPIO_PA_LEVEL |=(1<<28);
	//rGPIO_PA_RDATA |=(1<<28);
   
	/*init gpio handlers,then enable GPIO interrupt on ICU*/
	request_irq(GPIO_INT, PRIORITY_FIFTEEN, GPIOIntHander);
	
}

#ifdef  xm_Test_gpio

void TestGPIO(void)
{
	INT8  string[50];
	UINT8 i;
	void  *GPIOFunction[][2] =
	{
		(void*)TestGPIOInput,  "Test   GPIO   Data  Input",
		(void*)TestGPIOOutput,   "Test   GPIO   Data  Output",
		(void*)TestGPIOInterrupt,"Test   GPIO   Interrupt  Detect",
		0,0
	};
	
	SelGPIOPad();
	


	printk("TEST 1960 GPIO\n");
       printk("Start Test , Please Put Key Num....\n");

        while(1)
        {
        	printk("0 : Test   GPIO   Data  Input!\n");
        	printk("1 : Test   GPIO   Data  Output!\n");
        	printk("2 : Test   GPIO   Interrupt  Detect!\n");
        	printk(" Please Select (0--2).......\n");
              Uart_GetString(string);
              printk("Uart string = %s\n",string);
              i = Uart_GetIntNum(string);
              printk("select num is = %d\n",i);

              if(i>=0 && (i<(sizeof(GPIOFunction)/8)) )
              {
              		printk("%s\n",GPIOFunction[i][1]);
                  ( (void (*)(void)) (GPIOFunction[i][0]) )();
              }
        }

}
#endif