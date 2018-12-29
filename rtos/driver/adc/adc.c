/*
 * ADC.c
 *
 *  Created on: 2010-10-15
 *      Author: Administrator
 */
#include "hardware.h"
#include "adc.h"
#include "gpio.h"

#define	printk	printf

#define ADC_NULL        0xffffffff          // 无按键

#define	MAX_ADC_DEVICE		2		// AUX0, AUX1

#define  MAXKEYADCLEN    10

#define  MAXPANADCLEN    20

typedef struct
{
	UINT32 *pValue;
	UINT32 header;
	UINT32 trail;
	UINT32 quelen;
} ADCValueQueue;


ADCValueQueue  keyAdcQueue;

UINT32 keyADC[MAXKEYADCLEN];

#ifdef TOUCHPAN
ADCValueQueue  panADCXQueue;
ADCValueQueue  panADCYQueue;
UINT32 PANADC_X[MAXPANADCLEN];
UINT32 PANADC_Y[MAXPANADCLEN];
#endif

static void (*adc_callback[MAX_ADC_DEVICE])(unsigned int adc_sample_value);

INT32 Queue_ADCValue_Init(ADCValueQueue *pQueue, UINT32 *pValue, UINT32 quelen)
{
	INT32 ret=0;
	
	if((pQueue != 0) && (pValue != 0) && (quelen > 0))
	{
		pQueue->pValue = pValue;
		pQueue->quelen = quelen;
		pQueue->header = 0;
		pQueue->trail = 0;
		ret =1;
	}

	return ret;
}

INT32 Queue_ADCValue_Length(ADCValueQueue *pQueue)
{
	INT32 queuelen=0;
	
	if(pQueue != 0)
	{
		if(pQueue->trail < pQueue->header)
		{
			queuelen = pQueue->quelen +(pQueue->trail- pQueue->header);
		}
		else
			queuelen =  pQueue->trail- pQueue->header;
	}

	return queuelen;
}

INT32 Queue_ADCValue_Add(ADCValueQueue *pQueue,unsigned int value)
{
	INT32 ret=0;
	UINT32 *pValue=0;
	
	if(pQueue != 0)
	{
		pValue = pQueue->pValue + pQueue->trail;
		*pValue = value;

		pQueue->trail++;
		if(pQueue->trail >= pQueue->quelen)
		{
			pQueue->trail = 0;
		}

		if(pQueue->trail == pQueue->header)
		{
			pQueue->header++;
			if(pQueue->header >= pQueue->quelen)
			{
				pQueue->header = 1;
				pQueue->trail=0;
			}
		}

//		printk("1 pQueue->header = %d pQueue->trail= %d \n",pQueue->header,pQueue->trail);
		ret=1;

//		printk("queue len = %d \n",Queue_ADCValue_Length(pQueue));
	}

	return ret;
}


INT32 Queue_ADCValue_Read(ADCValueQueue *pQueue, UINT32 *pValue)
{
	INT32 ret=0;
	UINT32 *pHeaderValue=0;

	if((pQueue != 0) && (pValue != 0))
	{
		if(Queue_ADCValue_Length(pQueue) > 0)
		{
			pHeaderValue = pQueue->pValue + pQueue->header;
			*pValue = *pHeaderValue;

			pQueue->header++;
			if(pQueue->header >= pQueue->quelen)
			{
				pQueue->header = 0;
			}

//			printk("2 pQueue->header = %d pQueue->trail= %d \n",pQueue->header,pQueue->trail);

			ret=1;

		}
	}

	return ret;
}



//////////////////////////////////////////////////////////////////
void DivADCCLK(UINT16 divvalue) //ADC CLK, 以24MHZ为基准。
{
	rSYS_DEVICE_CLK_CFG1 &= ~(0xFFFE);
	rSYS_DEVICE_CLK_CFG1 |= ((divvalue & 0x7FFF) << 1);
}

void SetDBCNT(UINT32 dbcount) //以系统的PCLK为单位,削抖时间
{
	rADC_DBNCNT = dbcount;
}

void SetDETInter(UINT32 incnt) //设置间隔时间
{
	rADC_DETINTER = incnt;
}

void SetDETInter_new(UINT32 incnt) //设置间隔时间
{
	UINT32 ulFreqAPB;
	UINT32 ulADC_Div;
	UINT32 ulDBNCNT;
	UINT32 ulMinTransInterval;
	UINT32 reg;
	
//	trans_interval > dbncnt * freq_adc /(2*freq_apb) 
//                          = dbncnt * (24MHz/((adc_div+1)*2))/(2*freq_apb) 
//                          = dbncnt * 6 / ((adc_div+1)*freq_apb)

	// adc_clk = clk_24m/((adc_clk_div+1)*2).
	ulFreqAPB =  24000000; 	// FPGA config  35M
	ulADC_Div = 1000000;    // FPGA config  1M
	reg = rSYS_DEVICE_CLK_CFG1;
	reg &= ~0x7FFF;
	reg |= 11;
	rSYS_DEVICE_CLK_CFG1 = reg;
	
	rADC_DETINTER = 0xFFFF;
	
	printk("incnt : 0x%x rADC_DETINTER: 0x%x\n",incnt,rADC_DETINTER);
}



#define MAX_LEN_INT_FLAG			512
static unsigned int int_flag[MAX_LEN_INT_FLAG][17];
static unsigned int lg_ulCurIndex = 0;
static volatile unsigned int sampleEnd = 0;

void Init_IntFlag()
{
		int i, j;
		
		for(i=0;i<MAX_LEN_INT_FLAG;i++)
			for(j=0;j<17;j++)
				int_flag[i][j] = 0xFFFFFFFF;
}

void CheckStartFlag()
{
}

void CheckStopFlag()
{
}

static volatile int bFindStop = 1;
static volatile int bFindStart = 0;
void Check_NoStop(void)
{
	int i;
	for(i=0;i<lg_ulCurIndex;i++)
	{
		if(int_flag[i][0] & (1<<12))
		{
//				bFindStart = 1;
				if(bFindStop == 0)
				{
					printk("Error, not detect stop signal at %d\r\n", i);
				}
				bFindStop = 0;
		}
		
		if(int_flag[i][0] & (1<<13))
		{
				bFindStop = 1;
//				bFindStart = 0;
		}		
	}
}

void adc_int_handler(void)
{
	UINT32 adcStatus;
	INT32 i=0;
	UINT32 ADCValue=0;
	INT32 index = 0;
	
	adcStatus = rADC_STA;
  	// printk("rADC_STA = 0x%x\n", adcStatus);
	if(adcStatus & AUX0_VALUE_INT)
	{
		ADCValue = rADC_AUX0; 
	}
	if(adcStatus & AUX1_VALUE_INT)
	{
		
	}
	
	i=1;
	while(adcStatus)
	{
		if(adcStatus & i)
		{
			switch(i)
			{
				case AUX0_START_INT:
 					printk("0_START: 0x%0x\n",rADC_AUX0);
					break;
				case AUX0_STOP_INT:
 					printk("0_STOP : 0x%0x \n",rADC_AUX0);
					break;
				case AUX0_VALUE_INT:
					 printk("0_VALUE : 0x%0x \n",rADC_AUX0);
					ADCValue = rADC_AUX0;
					Queue_ADCValue_Add(&keyAdcQueue,ADCValue);
					break;

				case AUX1_START_INT:
 					 printk("1_START\n");
					break;
				case AUX1_STOP_INT:
 					 printk("1_STOP\n");
					break;
				case AUX1_VALUE_INT:
					ADCValue = rADC_AUX1;
					Queue_ADCValue_Add(&keyAdcQueue,ADCValue);
 					printk("1_Value =0x%x\n",rADC_AUX1);
					break;

				default:
					break;
			}

			adcStatus&= ~i;
		
			if(lg_ulCurIndex < MAX_LEN_INT_FLAG)
				int_flag[lg_ulCurIndex][index++] = rADC_STA;		
				
			rADC_STA&=~i;
		}
		i = i << 1;
	}

	if(!sampleEnd)
		lg_ulCurIndex++;
//	printk("adc_int_handler \n");
//	printk("rADC_STA=0x%x\n",rADC_STA);
//	printk("rADC_SCTR=0x%x\n",rADC_SCTR&0x10);
// 	printk("rADC_AUX1=0x%x\n",rADC_AUX1);
//	printk("rADC_PANXZ1=0x%x rADC_PANXZ2= 0x%x \n",rADC_PANXZ1,rADC_PANXZ2);
//	rADC_STA=0;
}

void Choose_ADC_Channel(int channel)
{
	rADC_CTR &= ~(0x7E);
	rADC_IMR = 0xffffffff;
	rADC_STA=0;

	switch(channel)
	{
		case BAT_CHANNEL:
			rADC_CTR |= (1<<BAT_CHANNEL);
			rADC_IMR &=~(1<<15);
			break;

		case TOUCHPAN_CHANNEL:
			rADC_CTR |= (1<<TOUCHPAN_CHANNEL);
			rADC_IMR &=~((1<<14)|(1<<13)|(1<<12));
			break;

		case AUX0_CHANNEL:
			rADC_CTR |= (1<<8)|(1<<AUX0_CHANNEL);
			rADC_IMR &=~((1<<2)|(1<<1)|(1<<0));
			break;

		case AUX1_CHANNEL:
			rADC_CTR |= (1<<9)|(1<<AUX1_CHANNEL);
			//rADC_CTR &= ~(1<<9);
			rADC_IMR &=~((1<<5)|(1<<4)|(1<<3));
			break;

		case AUX2_CHANNEL:
			rADC_CTR |= (1<<10)|(1<<AUX2_CHANNEL);
			rADC_IMR &=~((1<<8)|(1<<7)|(1<<6));
			break;

		case AUX3_CHANNEL:
			rADC_CTR |= (1<<11)|(1<<AUX3_CHANNEL);
			rADC_IMR &=~((1<<11)|(1<<10)|(1<<9));
			break;

		default:
			break;
	}
}

void ADC_init(void)
{
	sys_soft_reset (softreset_adc);
	
	sys_clk_enable (adc_pclk_enable);
	sys_clk_enable (adc_clk_enable);
		
	rSYS_ANALOG_REG0 &=~(1 << 22); // ref : 3.3v
	
	
//	DivADCCLK(160);
//    FPGA  外部配置固定时钟1M,       
	
	//reset adc module
	rADC_CTR |= 1<<0;
	delay (10);
	//rADC_CTR &= ~(0x7E);
	rADC_CTR  = 0;
	
	Queue_ADCValue_Init(&keyAdcQueue, keyADC, MAXKEYADCLEN);
#ifdef TOUCHPAN
	Queue_ADCValue_Init(&panADCXQueue, PANADC_X, MAXPANADCLEN);
	Queue_ADCValue_Init(&panADCYQueue, PANADC_Y, MAXPANADCLEN);
#endif

	// AUX0 --> BAT
	// AUX1 --> KEY
	//register irq
	// 中断使能 (AUX0, AUX1)
	rADC_IMR = 	(0x07 << 0)		// AUX0 (BAT)
				|	(0x07 << 3)		// AUX1 (KEY)
				 ;
	// 清除所有中断
	rADC_STA = 0;		// bit clear清除
	request_irq (ADC_INT, PRIORITY_FIFTEEN, adc_int_handler);
	
	
	//set sampling parameter
// 	SetDBCNT(1000); 
 //	SetDETInter(1000);
 
//#ifdef TOUCHPAN
// 	SetDBCNT(1000);// max 2^24 ??       0xff ffff
//	SetDETInter(1000);
//#else
	//SetDBCNT(0x206E0);       // FPGA 一直出中断,设置太大
	
	//SetDBCNT(50000);              // 50000 * (1/apb_35M) = 1.4ms 
	//SetDETInter_new(1000);      //  1000 * (1/1m) = 1000 * 1us =  1ms

//	SetDBCNT(100000);              // 100000 * (1/apb_35M) = 2.8ms 
//	SetDETInter_new(10000);      //  10000 * (1/1m) = 10000 * 1us =  10ms

//	SetDBCNT(100000);              // 100000 * (1/apb_35M) = 2.8ms 
//	SetDETInter_new(4000);      //  4000 * (1/1m) = 4000 * 1us =  4ms

//	SetDBCNT(20000);              // 20000 * (1/apb_35M) =0.56ms  = 560us
//	SetDETInter_new(1000);      //  1000 * (1/1m) = 1000 * 1us =  1ms 

	SetDBCNT(10000);              // 10000 * (1/apb_35M) =0.28ms  = 280us
	SetDETInter_new(600);      //  600 * (1/1m) = 600 * 1us =  600us = 0.6ms 
	
//#endif

	//set detect level
#if  0// sch  default is low , key press is high
	rSYS_ANALOG_REG1 |= (1 << 14); // 1: connect a pull-down resister (0.3v-3.3v have interrupt)
#else// sch  default is high , key press is low 
 	rSYS_ANALOG_REG1 &= ~(1 << 14);  //   0: connect a pull-up resister  (3.0v - 0v have interrupt)
 #endif
	rADC_CTR |= 1<<8;      // 1: aux0_det high valid.
	rADC_CTR |= (1<<9);    // 1: aux1_det high valid.
//	rADC_CTR |= (1<<10); // 1: aux2_det high valid.
//	rADC_CTR |= (1<<11); // 1: aux3_det high valid.   note: low no used 
printk("ADC_init is finished\n");
 	
}



INT32 GetCurChannel(void)
{
	INT32 channel=0;
	UINT32 enADCValue=0;

	enADCValue = rADC_CTR;
	enADCValue &=0x7E;
	while(enADCValue)
	{
		enADCValue=enADCValue>>1;
		channel++;
		if((enADCValue& 0x1))
		{
			break;
		}
	}

	return channel;
}

void Switch_ADC_Channel(void)
{
	INT32 channel=0;
	
	channel = GetCurChannel();
	if(channel == 0)
	{
		Choose_ADC_Channel(AUX0_CHANNEL);
	}
#ifdef TOUCHPAN
	else if(channel == AUX0_CHANNEL)
	{
		Choose_ADC_Channel(TOUCHPAN_CHANNEL);
	}
#endif
	else
	{
		Choose_ADC_Channel(AUX0_CHANNEL);
	}
}

void Enable_ADC_Channel(UINT32 channel)
{
	switch(channel)
	{
		case BAT_CHANNEL:
			rADC_CTR |= (1<<BAT_CHANNEL);
			rADC_IMR &=~(1<<15);
			break;

		case TOUCHPAN_CHANNEL:
			rADC_CTR |= (1<<TOUCHPAN_CHANNEL);
			rADC_STA=0;
			rADC_IMR &=~((1<<14)|(1<<13)|(1<<12));
			break;

		case AUX0_CHANNEL:
			rADC_CTR |= (1<<AUX0_CHANNEL);
			rADC_STA=0;
			rADC_IMR &=~((1<<2)|(1<<1)|(1<<0));
			break;

		case AUX1_CHANNEL:
			rADC_CTR |= (1<<AUX1_CHANNEL);
			rADC_IMR &=~((1<<5)|(1<<4)|(1<<3));
			break;

		case AUX2_CHANNEL:
			rADC_CTR |=(1<<AUX2_CHANNEL);
			rADC_IMR &=~((1<<8)|(1<<7)|(1<<6));
			break;

		case AUX3_CHANNEL:
			rADC_CTR |= (1<<AUX3_CHANNEL);
			rADC_IMR &=~((1<<11)|(1<<10)|(1<<9));
			break;

		default:
			break;
	}

	//printk("rADC_CTR :0x%x  rADC_IMR:0x%x \r\n",rADC_CTR, rADC_IMR);

}

void Disable_ADC_Channel(UINT32 channel)
{
	switch(channel)
	{
		case BAT_CHANNEL:
			rADC_CTR |= (1<<BAT_CHANNEL);
			rADC_IMR &=~(1<<15);
			break;

		case TOUCHPAN_CHANNEL:
			rADC_CTR |= (1<<TOUCHPAN_CHANNEL);
			rADC_STA=0;
			rADC_IMR &=~((1<<14)|(1<<13)|(1<<12));
			break;

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

		case AUX2_CHANNEL:
			rADC_CTR |= (1<<10)|(1<<AUX2_CHANNEL);
			rADC_IMR &=~((1<<8)|(1<<7)|(1<<6));
			break;

		case AUX3_CHANNEL:
			rADC_CTR |= (1<<11)|(1<<AUX3_CHANNEL);
			rADC_IMR &=~((1<<11)|(1<<10)|(1<<9));
			break;

		default:
			break;
	}
}

//////////////////////////////////////////////////
UINT32 GetADCKey(void)
{
	UINT32 adckey=0;
	INT32 ret=0;
	
	ret = Queue_ADCValue_Read(&keyAdcQueue,&adckey);
	if(ret == 1)
	{
		return adckey;
	}
	return ADCKEY_NULL;
}

//////////////////////////////////////////////////
#ifdef TOUCHPAN
UINT32 GetADCTouch(PAN_DATA *pPan)
{
	UINT32 panX;
	UINT32 panY;

	INT32 ret=0;

	if(pPan != 0)
	{
		ret = Queue_ADCValue_Read(&panADCXQueue,&panX);
		if(ret==1)
		{
			ret = Queue_ADCValue_Read(&panADCYQueue,&panY);
			if(ret == 1)
			{
				pPan->adcX = panX;
				pPan->adcY = panY;
			}
		}
	}

	return ret;
}
#endif

void ADC_Force_Transform(void)
{
	rADC_CTR |= (1<<12);// force enter pan interrupt
	delay(10000);
	rADC_CTR &=~ (1<<12); 
	
	rADC_CTR |= (1<<13);// force enter pan interrupt
	delay(10000);
	rADC_CTR &=~ (1<<13); 
	
	rADC_CTR |= (1<<14);// force enter pan interrupt
	delay(10000);
	rADC_CTR &=~ (1<<14); 

	Enable_ADC_Channel(BAT_CHANNEL);
	delay(10000);
	rADC_CTR &=~ (1<<1); 
}

void TestADCQueryValue(void)
{
	INT32 channel=0;
	UINT32 adckeyvalue=0;
	PAN_DATA touchData;
	unsigned int i;
	unsigned int j;
	UINT32 bPrinted = 0;

#ifdef TOUCHPAN
 	Enable_ADC_Channel(TOUCHPAN_CHANNEL);
#endif
  	// Enable_ADC_Channel(AUX0_CHANNEL); //AUX0 - AUX3, if no test , must 接地
  Enable_ADC_Channel(AUX1_CHANNEL); 
  	
 // 	Enable_ADC_Channel(AUX2_CHANNEL);
 // 	Enable_ADC_Channel(AUX3_CHANNEL); 

printk("Enable_ADC_Channel \n");
//	ADC_Force_Transform();
	
	while(1)
	{
//		OSTimeDly(10);
		adckeyvalue = GetADCKey();
		if(adckeyvalue != ADCKEY_NULL)
		{
//			printk("adckeyvalue = 0x%0x \n",adckeyvalue);
		}

#ifdef   TOUCHPAN
		if(GetADCTouch(&touchData)==1)
		{
//			printk("panX= 0x%0x  panY= 0x%0x\n",touchData.adcX,touchData.adcY);
		}
#endif
	
		delay(100000000);
		
		if(!sampleEnd)
		{
//			printk("sample1 %d \n",lg_ulCurIndex );
		}
		else
		{
			if(!bPrinted)
			{
				for(i=0;i<lg_ulCurIndex;i++)
				{
						printk("Init  %d  0x%x,",i, int_flag[i][0]);
						for(j=1;j<17;j++)
							if(int_flag[i][j] != 0xFFFFFFFF)
							{
									printk("0x%x, ", int_flag[i][j]);
							}
						printk("\r\n");
				}

				Check_NoStop();
				bPrinted = 1;
			}
		}
	}
}




void testADCAUX2(void)
{
	Choose_ADC_Channel(AUX2_CHANNEL);
	while(1)
	{
//		OSTimeDly(10);
	}
}

void testADCAUX3(void)
{
	Choose_ADC_Channel(AUX3_CHANNEL);
	while(1)
	{
//		OSTimeDly(10);
	}
}

void TestADC(void)
{
	unsigned int i;


	Init_IntFlag();
	//ShowHWFreqInfo();
	ADC_init();
	TestADCQueryValue();
	delay(100000);
	
//	testADCAUX2();
//	testADCAUX3();
}



