#include "hardware.h"
#include "RTOS.h"		// OS头文件
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "xm_mci.h"
#include "xm_dev.h"
#include "xm_key.h"
#include "xm_base.h"
#include "gpio.h"
#include <xm_type.h>
#include "xm_core.h"
#include "board_config.h"

#include "fs.h"

// 静音控制
static void audio_mute_open (void);
static void audio_mute_close (void);
static void audio_mute_enable (void);
static void audio_mute_disable (void);

#define  IISADC_BASE			I2S_ADC_BASE		// ADC 0


#define IISADC_GCR	0x0
#define IIS_GCR1		0x4
#define IISDAC_VCR	0x8
#define IISADC_FIFOS	0xc
#define IISDAC_CR1	0x10
#define IISADC_IMR	0x14
#define IISADC_CR		0x1c
#define IISADC_DR		0x80

#define MAX_FIFO_ENTRYS				32



/* IISDAC global control reg */
#define  IISADC_GCR_VREF_3V3			0
#define  IISADC_GCR_VREF_2V2			(1<< 28)
#define  IISADC_GCR_PGA_VOL1V5		(0)
#define  IISADC_GCR_PGA_VOL1V65		(1 << 27)
#define  IISADC_GCR_MICIN				(0)
#define  IISADC_GCR_LINEIN			(1<< 26)
#define  IISADC_GCR_PWRON				(1<<25)
#define  IISADC_GCR_ADCDATA			(1<<24)
#define  IISADC_GCR_ADCEN				(1 << 23)
#define  IISADC_GCR_DACEN				(0 << 22)

#define  IISADC_GCR_VREF_DISPD		(0 << 21)
#define  IISADC_GCR_RXTHR(x)			(x<<16) //max val = 31
#define  IISADC_GCR_TXTHR(x)			(x<<8) //max val = 31
#define  IISADC_GCR_DACCES			(1<<7) //data output at the middle of dac clk

#define  IISADC_GCR_RDTDMAEN			(1 << 6) //not 6 (changed form 6 to 3)
//#define  IISADC_GCR_TDMAEN			(1 << 3) //also is bit 3
#define  IISADC_GCR_RST				(1<< 4)
#define  IISADC_GCR_BCLKIN			(0)
#define  IISADC_GCR_BCLKOUT			(1 << 2)
#define  IISADC_GCR_WCLKIN			(0)
#define  IISADC_GCR_WCLKOUT			(1 << 1)

#define  IISADC_GCR_GEN				(1)


/* IIS Control 1 Register */

#define IIS_GCR1_REC_EN	 (0)
#define IIS_GCR1_REC_DIS 	 (1)
#define IIS_GCR1_PLAY_EN	 (0)
#define IIS_GCR1_PLAY_DIS 	 (1<<1)

/* IISDAC Volume Control  Register */

#define IISDAC_VCR_L(x)		(x) //volume control(0~100)
#define IISDAC_VCR_R(x)		(x<< 8)//volume control(0~100)

/* IISDAC  Control 1 Register */

#define IISDAC_CR1_LRS		(1 << 5) //Left/Right channel switch
#define IISDAC_CR1_VTH(x)		(x<< 6)//volume threshold




/* IISDAC adc control reg */

#define  IISADC_CR_R_RATE1		(3 << 11)		// 11 bypass
#define  IISADC_CR_R_RATE8		(2 << 11)		// 10 1/8 sample rate
#define  IISADC_CR_R_RATE2		(1 << 11)		// 01 1/2 sample rate
#define  IISADC_CR_R_RATE4		(0 << 11)		// 00 1/4 sample rate

#define  IISADC_CR_L_RATE1		(3 << 9)
#define  IISADC_CR_L_RATE2		(1 << 9)
#define  IISADC_CR_L_RATE4		(0 << 9)
#define  IISADC_CR_L_RATE8		(2 << 9)

#define	IISADC_CR_L_PGA_SEL_enhance	(1 << 14)		// 26db
#define	IISADC_CR_L_PGA_SEL_disable	(0 << 14)		// 20db
#define	IISADC_CR_R_PGA_SEL_enhance	(1 << 15)
#define	IISADC_CR_R_PGA_SEL_disable	(0 << 15)

#define	IISADC_CR_PD_PGA				(1 << 25)
#define	IISADC_CR_ADC_SEL				(1 << 24)		// 1 playback source selection
#define	IISADC_CR_ADC_EN				(1 << 23)		// ADC enable


#define  IISADC_CR_R_VOL(x)		(x << 4)
#define  IISADC_CR_L_VOL(x)		(x)

//fifo status
#define	IISADC_FIFOS_TFF		(1 << 0)
#define	IISADC_FIFOS_RFE		(1 << 1)



/* Register access macros */
#define iisadc_readl(reg)					\
	*(volatile unsigned int *)( IISADC_BASE+ reg)
#define iisadc_writel(value, reg )				\
	(*(volatile unsigned int *)( IISADC_BASE+reg) = value)


// DAC
#define iisdac_readl(reg)					\
	*(volatile unsigned int *)( I2S_DAC_BASE+ reg)
#define iisdac_writel(value, reg )				\
	(*(volatile unsigned int *)( I2S_DAC_BASE+reg) = value)


static void dac_dma_start (u32_t buffer, u32_t length, int index);
static void play_dma_stop (void);


static const unsigned short nco_step_nco_mod[] = {
	192,	1875,			// 48000
	1764,	18750,		// 44100
	128,	1875,			// 32000
	192,	3750,			// 24000
	882,	18750,		// 22050
	192,	5625,			// 16000
	441,	18750,		// 11025
	192,	11250			//  8000
};

// sample_clock 采样时钟
	// 按照下面的公式计算, 公式1及公式2计算, 根据mclk,
	// 48000 采样频率
	// 256 定值
	// 240000000	I2S选择的时钟
	//
	// 48KHz sample clock
	// 48000 * 256 = mclk = 12288000 (公式1)
	// 240000000 * i2s_nco_step/(I2s_nco_mod*2) = mclk (公式2)
	// 240000000 * 192 / (2 * 1875) = 12288000
static int SetSpeed (unsigned int i2s_dev, unsigned int sample_clock)
{
	// I2S选择的时钟固定为240MHz
	u32 val;
	unsigned short nco_step, nco_mod;
	
	switch (sample_clock)
	{
		case 48000:		nco_step = 192;	nco_mod = 1875;	break;
		case 44100:		nco_step = 1764;	nco_mod = 18750;	break;
		case 32000:		nco_step = 128;	nco_mod = 1875;	break;
		case 24000:		nco_step = 192;	nco_mod = 3750;	break;
		case 22050:		nco_step = 882;	nco_mod = 18750;	break;
		case 16000:		nco_step = 192;	nco_mod = 5625;	break;
		case 11025:		nco_step = 441;	nco_mod = 18750;	break;
		case 8000:		nco_step = 192;	nco_mod = 11250;	break;
		default:	
			printf ("un-support sample_clock (%d)\n", sample_clock);
			return -1;
	}
	
	XM_lock ();
	if(i2s_dev == XMMCI_DEV_0)		// I2S 0
	{
		// SYS_DEVICE_CLK_CFG0
		// 0	R/W	0	I2s_clk_sel
		// 1: audio_pll
		// 0: clk_240m
		val = rSYS_DEVICE_CLK_CFG0;
		val &= ~(1 << 0);
		rSYS_DEVICE_CLK_CFG0 = val;
		rSYS_I2S_NCO_CFG = (nco_step << 16) | nco_mod;
	}
	else if(i2s_dev == XMMCI_DEV_1)	// I2S 1
	{
		// SYS_DEVICE_CLK_CFG1
		// 24	R/W	0	I2s1_clk_sel
		// 1: audio_pll
		// 0: clk_240m
		val = rSYS_DEVICE_CLK_CFG1;
		val &= ~(1 << 24);
		rSYS_DEVICE_CLK_CFG1 = val;
		rSYS_I2S1_NCO_CFG = (nco_step << 16) | nco_mod;
	}
	XM_unlock ();
	return 0;
}


#define	MCI_FIFO_COUNT				8
#define	MCI_DMA_FIFO_SIZE			(800)	// DMA包长(字节大小为320字节，含16位左右声道)
#define	_PLAY						0
#define	_REC0						1		// MIC0通道
#define	_REC						_REC0

#define	_PRODUCER	0		// 产生者
#define	_CONSUMER	1		// 消费者

static volatile u8_t device_state = 0;
static volatile u8_t	deviceStatus[2];

static XMWAVEFORMAT deviceWaveFormat[2];		// 设备打开格式参数
static volatile u8_t device_ref_count[2];		// 设备引用次数

#pragma data_alignment=32
__no_init static u32_t deviceFifo[2][MCI_FIFO_COUNT][MCI_DMA_FIFO_SIZE];
#pragma data_alignment=32
static struct dw_lli dmaDesc[2][MCI_FIFO_COUNT];

static volatile u32_t deviceProducer[2];	// 产生者DMA包索引
static volatile u32_t deviceConsumer[2];	// 消费者DMA包索引
static volatile u32_t workingFifoSampleCount[2];

static volatile u32_t rec_count = 0;

// 读写同步事件，用于读写过程与DMA之间同步
static OS_CSEMA deviceSemaphore[2];

static OS_RSEMA			iis_access_sema;	

static u32_t dac_mute = 0;
static u32_t dac_vol = 80;

static volatile u32_t adc_channel;
static volatile u32_t dac_channel;


// ADC第一级增益 burst 10倍 20db
// ADC第二级增益 0x00 ~ 0x0E 为线性变化，从-9.0db 到15.0db. 0x0F为22.5db
// 总db数 = 第一级增益 + 第二级增益
//#define	ADC_VOLUME	0x0E 	// 总增益 = 20 + 10.2 = 30.2db
//#define	ADC_VOLUME	0x00		// -9.0db
//#define	ADC_VOLUME	0x0E			// 15.0db
#define	ADC_VOLUME	0x0F		// 22.5db

static int lg_ulLeftVolume = 0;
static int lg_ulRightVolume = 0;

#define DMA_P2M_CTL_L			((0<<28)\
										|(0<<27)\
										|(0<<25)\
										|(0<<23)\
										|(P2M_DMAC<<20)\
										|(0<<18)\
										|(0<<17)\
										|(DMA_BURST_SIZE_4<<14)\
										|(DMA_BURST_SIZE_4<<11)\
										|(SRC_FIX<<9)\
										|(0<<7)\
										|(DMA_WIDTH_32<<4)\
										|(DMA_WIDTH_32<<1)\
										|(1<<0)) //interrupt

#define I2S_RX_DMA_CTL_L 		((0<<28)\
								|(0<<27)\
								|(0<<25)\
								|(0<<23)\
								|(P2M_DMAC<<20)\
								|(0<<18)\
								|(0<<17)\
								|(DMA_BURST_SIZE_16<<14)\
								|(DMA_BURST_SIZE_16<<11)\
								|(2<<9)\
								|(0<<7)\
								|(DMA_WIDTH_32<<4)\
								|(DMA_WIDTH_32<<1)\
								|(1<<0))

#define I2S_RX_DMA_CFG_L		((0<<31)\
								|(0<<30)\
								|(0<<17)\
								|(0<<8))


#define I2S_TX_DMA_CTL_H 		(0<<12)




#define DMA_M2P_CFG_H			((1<<6)\
										|(1<<5)\
										|(1<<1)\
										| DMA_CFGH_DST_PER(I2S_TX_FIFO_DMACBREQ))
										
		
#define DMA_P2M_CFG_H			((1<<6)\
										|(1<<5)\
										|(1<<1) \
										| DMA_CFGH_SRC_PER(I2S_RX_FIFO_DMACBREQ))

		
#define I2S_RX_DMA_CFG_H		((0<<12)\
								|(I2S_RX_FIFO_DMACBREQ<<7)\
								|(1<<6)\
								|(1<<5)\
								|(1<<1))

#define DMA_CFGH_SRC_PER(x)	((x) << 7)
#define DMA_CFGH_DST_PER(x)	((x) << 12)

#define I2S_AD_DATA_FIFO					(I2S_ADC_BASE + 0x80)




static void adc_dma_start (u32_t buffer, u32_t length)
{
	UINT32 DmaRequestTime;
	UINT32 I2sDmaCh;
	dma_inv_range (buffer, buffer + length);
	deviceStatus[_REC] = 1;	// 标记运行状态
	
	rDMA_MASK_TRF_L = (1<<(8+adc_channel)|(1<<adc_channel));
	rDMA_MASK_ERR_L = (1<<(8+adc_channel)|(1<<adc_channel));

	rDMA_CFG_L = 0xffffffff ;
	rDMA_CFG_H = 0xffffffff ;
	
	rDMA_SAR2_L = IISADC_BASE+ IISADC_DR;
	rDMA_DAR2_L = buffer;
	rDMA_LLP2_L = 0;
	rDMA_CTL2_L = DMA_P2M_CTL_L;
	rDMA_CTL2_H = length >> 2;
	rDMA_CFG2_L = 0;
	rDMA_CFG2_H = DMA_P2M_CFG_H;
	rDMA_SGR2_L = 0;
	rDMA_DSR2_L = 0;

	rDMA_CHEN_L = ((1<<(8+adc_channel))|(1<<adc_channel));//USE_DMA_TXCH enable
	//dma_start (adc_channel);
	
	return;
	/*
	DmaRequestTime = 0;
	do{
		I2sDmaCh = dma_request_dma_channel(0);
		if((DmaRequestTime++)>50000)
		{
			printf("i2s request dma channel failed!\n");
			return;
		}
	}while(I2sDmaCh == ALL_CHANNEL_RUNNING);
	dma_clr_int(I2sDmaCh);
	dma_cfg_channel(I2sDmaCh,
						 I2S_AD_DATA_FIFO, (UINT32)buffer,
						 0,
						 I2S_RX_DMA_CTL_L,I2S_TX_DMA_CTL_H|(length/4),
						 I2S_RX_DMA_CFG_L|(I2sDmaCh<<5),I2S_RX_DMA_CFG_H,
						 0,0);
	dma_start_channel(I2sDmaCh);
	
	while(!dma_transfer_over(I2sDmaCh))
	{
		
	}
	
	dma_clr_int(I2sDmaCh);
	dma_detect_ch_disable(I2sDmaCh);
	dma_release_channel(I2sDmaCh);
	*/
}

static void adc_dma_stop (void)
{
	deviceStatus[_REC] = 0;	// 标记停止状态
}

static void adc_init(void)
{
	unsigned int gcr , temp ;
	
	//set sample rate and vol
	temp =  IISADC_CR_R_RATE4 
			| IISADC_CR_L_RATE4 
			| IISADC_CR_L_PGA_SEL_enhance		//	ADC Mic pga sel (left)  enhance 1.enhance 0 disable
			| IISADC_CR_R_PGA_SEL_enhance		//	ADC Mic pga sel (right) enhance 1.enhance 0 disable
			| IISADC_CR_R_VOL(ADC_VOLUME)
			| IISADC_CR_L_VOL(ADC_VOLUME) 
			;
	iisadc_writel(temp   , IISADC_CR);

//	gcr = IISADC_GCR_VREF_3V3  | IISADC_GCR_ADCDATA |IISADC_GCR_ADCEN |\
//		IISADC_GCR_VREF_DISPD |IISADC_GCR_RXTHR(0x10) | \
//		IISADC_GCR_RDTDMAEN |IISADC_GCR_WCLKOUT| IISADC_GCR_GEN;
	gcr = iisadc_readl(IISADC_GCR);
	gcr &= ~(1 << 28);	// saradc vref 2v 3.3v select 0:3.3v
	gcr &= ~(1 << 27);	// ADC PGA op commond voltage sel (0:1.5, 1:1.65;)
	gcr &= ~(1 << 26);	// MIC LINE SEL (0: micin, 1: line in)
	gcr |= IISADC_GCR_ADCDATA;		// 使用saradc data
	gcr |= IISADC_GCR_ADCEN;		// ??? SARADC_EN (1: Disable, 0: Enable)		
	gcr &= ~(1 << 21);	// VREF power down (1:power down, 0:active)
	gcr &= ~(0x1F << 16);
	
	//gcr &= ~(1 << 22);
	
	gcr |= IISADC_GCR_RXTHR(0x10);
	gcr |= IISADC_GCR_RDTDMAEN;	// Transmit/RX FIFO DMA transfer enable: (0: disable., 1: enable.)
	gcr |= IISADC_GCR_WCLKOUT;
	gcr |= IISADC_GCR_GEN;
	
	// 20130522
	gcr |= IISADC_GCR_PWRON;
	
	temp = iisadc_readl(IIS_GCR1);
	temp &= ~IIS_GCR1_REC_DIS;
	iisadc_writel(temp ,IIS_GCR1);
	
	iisadc_writel(gcr ,IISADC_GCR);
}

// ADC关闭
static void adc_exit (void)
{
	unsigned int gcr;
	// disable adc
	gcr = iisadc_readl(IIS_GCR1);
	gcr |= IIS_GCR1_REC_DIS;
	iisadc_writel (gcr, IIS_GCR1);
}

static void ADC_DMA_INT_handler (UINT32 ulIRQFactors, UINT32 channel, void *channel_private_data)
{
	dma_clr_trans(adc_channel);
	if(rDMA_STATUS_ERR_L & (1<<adc_channel))
	{
		static int err_cnt = 0;
		err_cnt ++;
		printf ("adc dma error %d\n", err_cnt);
	}
	dma_clr_err(adc_channel);
	
	rec_count += MCI_DMA_FIFO_SIZE;
	deviceProducer[_REC] ++;
	OS_SignalCSema (&deviceSemaphore[_REC]);
	//printf ("ADC Int, count=%d, prod=%d\n", rec_count, deviceProducer[_REC] );
	// 检查DMA是否传输结束
	if(	(deviceProducer[_REC] - deviceConsumer[_REC]) >= MCI_FIFO_COUNT
		|| !(device_state & XMMCIDEVICE_RECORD) )
	{
		// 所有DMA FIFO已填充
		adc_dma_stop ();
		//printf ("adc stop\n");
	}
	else
	{
		u32_t index = deviceProducer[_REC];
		index %= MCI_FIFO_COUNT;
		adc_dma_start ((u32_t)(&deviceFifo[_REC][index][0]), MCI_DMA_FIFO_SIZE << 2);
	}
}

static void VOICE_DMA_INT_handler (UINT32 ulIRQFactors, UINT32 channel, void *channel_private_data)
{
	u32_t index;
	//printf ("dma %d\n", deviceConsumer[_PLAY]);
	dma_clr_trans(dac_channel);
	dma_clr_block(dac_channel);
	if(rDMA_STATUS_ERR_L & (1<<dac_channel))
	{
		static int err_cnt = 0;
		err_cnt ++;
		printf ("voice dma error %d\n", err_cnt);
	}
	dma_clr_err(dac_channel);
	
	
	deviceConsumer[_PLAY] ++;
	OS_SignalCSema (&deviceSemaphore[_PLAY]);
	
	// 检查DMA是否传输结束
	if(deviceConsumer[_PLAY] >= deviceProducer[_PLAY])
	{
		play_dma_stop ();
	}
	else
	{
		index = deviceConsumer[_PLAY];
		index %= MCI_FIFO_COUNT;
		//printf ("ri=%d\n", index);
		dac_dma_start ((u32_t)(&deviceFifo[_PLAY][index][0]), MCI_DMA_FIFO_SIZE << 2, index);
		//dac_dma_chain ((u32_t)(&deviceFifo[_PLAY][index][0]), MCI_DMA_FIFO_SIZE << 2, index);
	}
}

static void set_volume (void)
{
	if(dac_mute)
	{
		rIIS_DACR0 = 0;
	}
	else
	{
		rIIS_DACR0 = ((dac_vol & 0x7F) << 8) | (dac_vol & 0x7F);
	}
	
}

static void AudioCodec_SetVolume(UINT8 left, UINT8 right)
{
	dac_vol = left;
	lg_ulLeftVolume = left & 0x7F;
	lg_ulRightVolume = right & 0x7F;
	
	//rIIS_DACR0 = ((right & 0x7F) << 8)| (left & 0x7F);
	set_volume ();
}

// I2S 1
static void dac_init (void)
{
	UINT32 val;
	/*
	rSYS_SOFT_RSTNA &= ~(1<<15);
	__asm ("nop");
   __asm ("nop");
	rSYS_SOFT_RSTNA |= (1<<15);
   __asm ("nop");
   __asm ("nop");
	*/

	//设置复位之后，要手动清除该位
	rIIS_SACR0 |= (0x1 << 4 );
	//delay (100);
	rIIS_SACR0 &=~ (0x1 << 4 );//设置复位之后，要手动清除该位
	
	//rSACR0 = (0<<22) + (0<<21) + (0xf<<16) + (0xf<<8)+ (0x0<<6) + (0x0<<3) + (0x1<<2) + (0x1<<1) + 0x1;
      //(1<<22) + (1<<21) 
	rIIS_SACR0 =  (0xf<<16) + (0xf<<8)+ (0x0<<6) + (0x0<<3) + (0x1<<2) + (0x1<<1) + 0x1;
	rIIS_SACR0 &= ~(0x1 << 21); 
	rIIS_SACR0 &= ~(0x1 << 22); 
	rIIS_SACR0 &= ~(0x1 << 24); // 0:external i2s data  外部数据
	//	rIIS_SACR0|=(0x1 << 24); // 0:external i2s data  外部数据
	
	rIIS_SACR0 |= (1 << 3);
	
	rIIS_SACR1 = 0x1;//recording Function is Disabled. 
	rIIS_SAIMR = 0x00;
		
	SetSpeed ( XMMCI_DEV_1, 8000);
		
	AudioCodec_SetVolume (127, 127);//80,80
	//AudioCodec_SetVolume (0x6f, 0x6f);//100 
	XM_printf ("AudioCodec_SetVolume\n");
	audio_mute_disable ();
}

// DAC关闭
static void dac_exit (void)
{
	// 如何正确关闭DAC通道
	unsigned int gcr;
	  XM_printf ("dac_exit\n");
	 
	// 音量关闭
	iisdac_writel (IISDAC_VCR_L(0x00)|IISDAC_VCR_R(0x00) ,IISDAC_VCR);
	
	// disable dac
	gcr = iisdac_readl(IIS_GCR1);
	gcr |= IIS_GCR1_PLAY_DIS;
	iisdac_writel (gcr, IIS_GCR1);
	
	delay (1000);
	
	audio_mute_enable ();


}

#define DWC_PLAY_CTLLO		(DWC_CTLL_DST_MSIZE(3)		\
				| DWC_CTLL_SRC_MSIZE(3)		\
				| DWC_CTLL_DMS(0)		\
				| DWC_CTLL_SMS(0)		\
				| DWC_CTLL_LLP_D_EN		\
				| DWC_CTLL_LLP_S_EN)


static void dac_dma_start (u32_t buffer, u32_t length, int index)
{
	u32_t ctllo, ctlhi, cfghi, cfglo;
	u32_t reg;
	int next;
		
	index %= MCI_FIFO_COUNT;
	if(index == (MCI_FIFO_COUNT - 1))
		next = 0;
	else
		next = index + 1;
	
	// 标记DMA启动
	deviceStatus[_PLAY] = 1;
	
	//rDMA_MASK_TRF_L = (1<<(8+dac_channel)|(1<<dac_channel));
	rDMA_MASK_BLOCK_L = (1<<(8+dac_channel)|(1<<dac_channel));
	rDMA_MASK_ERR_L = (1<<(8+dac_channel)|(1<<dac_channel));
	
	reg = I2S_DAC_BASE + IISADC_DR;
	
	rDMA_SAR1_L = buffer;
	rDMA_DAR1_L = reg;
	
	ctllo = (DWC_PLAY_CTLLO
					| DWC_CTLL_DST_WIDTH(2)|DWC_CTLL_SRC_WIDTH(2)
					| DWC_CTLL_DST_FIX
					| DWC_CTLL_SRC_INC
					| DWC_CTLL_FC_M2P
					| DWC_CTLL_INT_EN	);
	ctlhi = length >> 2;
	
	cfglo = 0;		//DWC_CFGL_LOCK_BUS;//0;
	cfghi = DWC_CFGH_FIFO_MODE |DWC_CFGH_DS_UPD_EN	\
				|DWC_CFGH_SS_UPD_EN |(DWC_CFGH_SRC_PER(0)	| DWC_CFGH_DST_PER(I2S1_TX_FIFO_DMACBREQ));
	
	rDMA_CTL1_L = ctllo;
	rDMA_CTL1_H = ctlhi;

	rDMA_CFG1_H = cfghi;
	rDMA_CFG1_L = cfglo;
	
	rDMA_SGR1_L = 0;
	rDMA_DSR1_L = 0;
	
	dmaDesc[_PLAY][index].sar = buffer;
	dmaDesc[_PLAY][index].dar = reg;
	dmaDesc[_PLAY][index].ctllo = ctllo;
	dmaDesc[_PLAY][index].ctlhi = ctlhi;
	dmaDesc[_PLAY][index].sstat = 0;
	dmaDesc[_PLAY][index].dstat = 0;
	
#ifdef _DYNAMIC_DMA_
	dmaDesc[_PLAY][index].llp = 0;
#else
	//dmaDesc[_PLAY][index].llp = (u32_t)&dmaDesc[_PLAY][next];
	dmaDesc[_PLAY][index].llp = 0;		// 不支持链式队列。
													// 主要考虑 播放时Audio解码的实时性可能不能满足实时播放，
													// 会存在断续的情况
#endif
	
	dma_clean_range ((u32)&dmaDesc[_PLAY][index], (u32)&dmaDesc[_PLAY][index+1]);
	
	rDMA_LLP1_L = (u32_t)&dmaDesc[_PLAY][index];
	
	//printf_dma_desc ();
	
	rDMA_CHEN_L = ((1<<(8+dac_channel))|(1<<dac_channel));//USE_DMA_TXCH enable	
	//dma_start (dac_channel);
}

static void dac_dma_chain (u32_t buffer, u32_t length, int index, int last_block)
{
	u32_t ctllo, ctlhi;
	u32_t reg;
	
#ifdef _DYNAMIC_DMA_
	int prev;
	
	index %= MCI_FIFO_COUNT;
	if(index == 0)
		prev = MCI_FIFO_COUNT - 1;
	else
		prev = index - 1;
	
#else
	int next;
	
	index %= MCI_FIFO_COUNT;
	if(index == MCI_FIFO_COUNT - 1)
		next = 0;
	else
		next = index + 1;
#endif
			
	reg = IISADC_BASE+ IISADC_DR;
		
	ctllo = (DWC_PLAY_CTLLO
					| DWC_CTLL_DST_WIDTH(2)|DWC_CTLL_SRC_WIDTH(2)
					| DWC_CTLL_DST_FIX
					| DWC_CTLL_SRC_INC
					| DWC_CTLL_FC_M2P
					| DWC_CTLL_INT_EN	);
	ctlhi = length >> 2;
			
	dmaDesc[_PLAY][index].sar = buffer;
	dmaDesc[_PLAY][index].dar = reg;
	dmaDesc[_PLAY][index].ctllo = ctllo;
	dmaDesc[_PLAY][index].ctlhi = ctlhi;
	dmaDesc[_PLAY][index].sstat = 0;
	dmaDesc[_PLAY][index].dstat = 0;
#ifdef _DYNAMIC_DMA_
	dmaDesc[_PLAY][index].llp = 0;
#else
	if(last_block)
		dmaDesc[_PLAY][index].llp = 0;
	else
		dmaDesc[_PLAY][index].llp = (u32_t)&dmaDesc[_PLAY][next];
#endif
	
	dma_clean_range ((u32)&dmaDesc[_PLAY][index], (u32)&dmaDesc[_PLAY][index+1]);
	
#ifdef _DYNAMIC_DMA_
	dmaDesc[_PLAY][prev].llp = (u32_t)&dmaDesc[_PLAY][index];
	dma_clean_range ((u32)&dmaDesc[_PLAY][prev].llp, 4 + (u32)&dmaDesc[_PLAY][prev].llp);
#endif		
	
}

static void play_dma_stop (void)
{
	deviceStatus[_PLAY] = 0;	// 标记停止状态
	//printf ("dac stop\n");
}


static int wav_inited;
// 设备驱动初始化
void XM_WaveInit (void)
{
	// DMA资源分配
	if(wav_inited)
		return;
	
	OS_CREATERSEMA (&iis_access_sema);
	
	adc_channel = dma_request_dma_channel(ADC_DMA_CHANNEL);
	dac_channel = dma_request_dma_channel(VOICE_DMA_CHANNEL);
	XM_printf ("adc_ch=%d, dac_ch=%d\n", adc_channel, dac_channel);
	
	audio_mute_open ();
	
	dac_exit ();
	adc_exit ();
		
	//iisadc_clock_init ();
	//iisdac_clock_init ();
	
	// 创建与播放相关的空闲DMA包计数器，初始为最大DMA包个数
	OS_CreateCSema (&deviceSemaphore[_PLAY], MCI_FIFO_COUNT);

	// 创建与录音相关的已录DMA包计数器，初始为0
	OS_CreateCSema (&deviceSemaphore[_REC], 0);
	wav_inited = 1;

}

// 设备驱动退出
void XM_WaveExit (void)
{
	if(!wav_inited)
		return;
	
	audio_mute_close ();
	
	wav_inited = 0;
	OS_DeleteCSema (&deviceSemaphore[_PLAY]);
	OS_DeleteCSema (&deviceSemaphore[_REC]);
	
	OS_DeleteRSema (&iis_access_sema);
}

LONG XM_WaveOpen (DWORD dwDevice, XMWAVEFORMAT *pWaveFormat)
{
	//return XMMCIERRORCODE_SUCCESS;
	LONG ret = XMMCIERRORCODE_SUCCESS;
	
	XM_printf ("XM_WaveOpen 0x%x\n", dwDevice);
	OS_Use (&iis_access_sema);
	do 
	{
		if(dwDevice == 0 || pWaveFormat == NULL)
		{
			ret = XMMCIERRORCODE_INVALIDPARAMETER;
			break;
		}
		if(dwDevice & device_state)
		{
			// 设备已打开, 检查参数是否一致
			if(dwDevice & XMMCIDEVICE_PLAY)
			{
				// 回放模式
				if(	pWaveFormat->dwSamplesPerSec == deviceWaveFormat[_PLAY].dwSamplesPerSec
					&&	pWaveFormat->wBitsPerSample == deviceWaveFormat[_PLAY].wBitsPerSample
					&& pWaveFormat->wChannels == deviceWaveFormat[_PLAY].wChannels)
				{
					// 参数相同
					
					// 仅支持2个应用开启音频设备
					if(device_ref_count[_PLAY] >= 2)
					{
						printf ("ref_count > 2\n");
						ret = XMMCIERRORCODE_DEVICEBUSY;
						break;
					}
				}
				else
				{
					ret = XMMCIERRORCODE_DEVICEBUSY;
					break;
				}
			}
			else
			{
				// 录音模式, 仅支持单个应用打开
				ret = XMMCIERRORCODE_DEVICEBUSY;
				break;
			}
		}
		
		if(pWaveFormat->wChannels != 1 && pWaveFormat->wChannels != 2)
		{
			ret = XMMCIERRORCODE_INVALIDFORMAT;
			break;
		}
		if(pWaveFormat->wBitsPerSample != 16)
		{
			ret = XMMCIERRORCODE_INVALIDFORMAT;
			break;
		}
	
		if( pWaveFormat->dwSamplesPerSec != 8000
			&&pWaveFormat->dwSamplesPerSec != 11025
			&&pWaveFormat->dwSamplesPerSec != 16000			
			&& pWaveFormat->dwSamplesPerSec != 22050
			&&pWaveFormat->dwSamplesPerSec != 24000	
			&&pWaveFormat->dwSamplesPerSec != 32000
			&&pWaveFormat->dwSamplesPerSec != 44100	
			&&pWaveFormat->dwSamplesPerSec != 48000)
		{
			ret = XMMCIERRORCODE_INVALIDFORMAT;
			break;
		}
		
		if(dwDevice & XMMCIDEVICE_PLAY)
			memcpy (&deviceWaveFormat[_PLAY], pWaveFormat, sizeof(XMWAVEFORMAT));
		if(dwDevice & XMMCIDEVICE_RECORD)
			memcpy (&deviceWaveFormat[_REC], pWaveFormat, sizeof(XMWAVEFORMAT));
		
		if(dwDevice & XMMCIDEVICE_PLAY)
		{
			// 累加设备引用计数
			device_ref_count[_PLAY] ++;
			XM_printf ("wave_open, ref_count=%d\n", device_ref_count[_PLAY]); 
			if(device_ref_count[_PLAY] >= 2)
			{
				// 设备已开启
				ret = XMMCIERRORCODE_SUCCESS;
				break;
			}
			// 打开播放设备
			deviceProducer[_PLAY] = 0;
			deviceConsumer[_PLAY] = 0;
			OS_SetCSemaValue (&deviceSemaphore[_PLAY], MCI_FIFO_COUNT);
			
			// 等待空闲写缓冲区
			OS_WaitCSema (&deviceSemaphore[_PLAY]);
			workingFifoSampleCount[_PLAY] = MCI_DMA_FIFO_SIZE;	
			
			device_state |= XMMCIDEVICE_PLAY;
			
			deviceStatus[_PLAY] = 0;	// 标记停止状态
			
			dac_init();
			register_channel_IRQHandler (dac_channel, 
											  VOICE_DMA_INT_handler, 
											  NULL,
											  //(1<< BLOCK_INT_BIT)|(1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
											  (1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
											  );
			
		}
		
		if(dwDevice & XMMCIDEVICE_RECORD)
		{
			// 打开录音设备
			rec_count = 0;
			deviceProducer[_REC] = 0;
			deviceConsumer[_REC] = 0;
			workingFifoSampleCount[_REC] = 0;
			
			device_state |= XMMCIDEVICE_RECORD;
			
			deviceStatus[_REC] = 1;	// 标记运行状态
			
			OS_SetCSemaValue (&deviceSemaphore[_REC], 0);
			
			dma_inv_range ((u32_t)&deviceFifo[_REC][0][0], ((unsigned int)&deviceFifo[_REC][0][0]) + sizeof(deviceFifo[_REC]));
			
			//SetSpeed2 (pWaveFormat->dwSamplesPerSec);
			SetSpeed (XMMCI_DEV_0, pWaveFormat->dwSamplesPerSec);
			
			adc_init();
			register_channel_IRQHandler (adc_channel, 
											  ADC_DMA_INT_handler, 
											  NULL,
											  //(1<< BLOCK_INT_BIT)|(1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
											  (1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
											  );
			
			// 启动DMA
			adc_dma_start ((u32_t) &deviceFifo[_REC][0][0],  MCI_DMA_FIFO_SIZE << 2);
		}
		
		//iisdac_init();
		
		ret = XMMCIERRORCODE_SUCCESS;
	} while (0);
	
	OS_Unuse (&iis_access_sema);
	XM_printf ("XM_WaveOpen ret=%d\n", ret);
	return ret;
}

// 检测最后一个DMA块中的第一个过零点位置, 将该位置之后的内容全部置为0, 然后将其播放
static void detect_zero_crossing_point_and_close (void)
{
	// 检查是否存在未写满的DMA FIFO
	u32_t count = workingFifoSampleCount[_PLAY];
	u32_t dma_index;
	u32_t i;
	u32_t *fifo;
	u32_t l_zcp, r_zcp;
	u16_t *sub_fifo;
	int to_play;
	if(count == 0)
	{
		return;
	}
	
	if(count == MCI_DMA_FIFO_SIZE)
	{
		// 最后一个DMA FIFO为空, 查找前一个DMA块
		if(deviceProducer[_PLAY] == 0)
			return;
		
		dma_index = deviceProducer[_PLAY] - 1;
		dma_index %= MCI_FIFO_COUNT;
		count = 0;
		to_play = 0;		// 已在DMA播放链表中
	}
	else
	{
		// 未写满的DMA FIFO
		dma_index = deviceProducer[_PLAY];
		dma_index %= MCI_FIFO_COUNT;
		to_play = 1;		// 未在DMA播放链表中
	}

	// 从左遍历DMA FIFO, 查找零点或者过零点
	{
		// 检查第一个过零点 (左右通道)
		fifo = &deviceFifo[_PLAY][dma_index][0];
		l_zcp = (u32_t)(-1);	// 左通道过零点
		r_zcp = (u32_t)(-1);	// 右通道过零点
		for (i = 0; i < (MCI_DMA_FIFO_SIZE - count - 1); i ++)
		{
			unsigned int sample = fifo[i];
			signed short left = (signed short)(sample & 0x0000FFFF);
			signed short right = (signed short)( (sample >> 16) & 0x0000FFFF );
			
			if(l_zcp == (u32_t)(-1))
			{
				// 过零点尚未找到
				// 零点
				if(left == 0)		
				{
					// 左通道过零点
					l_zcp = i;
				}
				else		// 非零点, 
				{
					// 判断左通道的下一个点是否翻转
					signed short next_pcm = (signed short)(fifo[i + 1] & 0x0000FFFF);
					if(left < 0 && next_pcm >= 0)
					{
						// 下一个点为过零点
						l_zcp = i + 1;
					}
					else if(left > 0 && next_pcm <= 0)
					{
						// 下一个点为过零点
						l_zcp = i + 1;						
					}
				}
			}
			
			if(r_zcp == (u32_t)(-1))
			{
				// 过零点尚未找到
				// 零点
				if(right == 0)
				{
					// 右通道过零点
					r_zcp = i;
				}
				else	// 非零点,
				{
					// 判断右通道的下一个点是否翻转
					signed short next_pcm = (signed short)((fifo[i + 1] >> 16) & 0x0000FFFF);
					if(right < 0 && next_pcm >= 0)
					{
						// 下一个点为过零点
						r_zcp = i + 1;
					}
					else if(right > 0 && next_pcm <= 0)
					{
						// 下一个点为过零点
						r_zcp = i + 1;						
					}
				}
			}
			
			if( l_zcp != (u32_t)(-1) && r_zcp != (u32_t)(-1) )
				break;
		}
		
		// 将过零点之后的数据清为0
		if(l_zcp != (u32_t)(-1))
		{
			i = l_zcp;
			sub_fifo = (u16_t *)fifo;
			sub_fifo += l_zcp * 2;
			for (; i < MCI_DMA_FIFO_SIZE; i ++)
			{
				*sub_fifo = 0;
				sub_fifo += 2;
			}
		}
		if(r_zcp != (u32_t)(-1))
		{
			i = r_zcp;
			sub_fifo = (u16_t *)fifo;
			sub_fifo += r_zcp * 2 + 1;
			for (; i < MCI_DMA_FIFO_SIZE; i ++)
			{
				*sub_fifo = 0;
				sub_fifo += 2;
			}
		}
				
		// 将Cache中的数据更新到SDRAM	
		dma_clean_range ((u32)&deviceFifo[_PLAY][dma_index][0],
								  (MCI_DMA_FIFO_SIZE << 2) + (u32)&deviceFifo[_PLAY][dma_index][0] );
			
		//printf ("count %d\n", count);
		#if 0
		if(to_play)
		{
			OS_IncDI();		// 关中断
			deviceProducer[_PLAY] ++;
			OS_DecRI();		// 开中断
			// 检查DMA是否停止。若是，启动播放
			if(deviceStatus[_PLAY] == 0)
			{
				dac_dma_start ((u32_t) &deviceFifo[_PLAY][dma_index][0], MCI_DMA_FIFO_SIZE << 2, dma_index);
				//dac_dma_chain ((u32_t) &deviceFifo[_PLAY][index][0], MCI_DMA_FIFO_SIZE << 2, index);
			}
			else
			{
				//dac_dma_chain ((u32_t) &deviceFifo[_PLAY][index][0], MCI_DMA_FIFO_SIZE << 2, index, 1);
			}
		}
		#endif
	}	
}

LONG XM_WaveClose (DWORD dwDevice)
{	
	//return XMMCIERRORCODE_SUCCESS;
	XM_printf ("XM_WaveClose 0x%x\n", dwDevice);	
	OS_Use (&iis_access_sema);
	
	dwDevice &= XMMCIDEVICE_PLAY | XMMCIDEVICE_RECORD;
	
	if(dwDevice & XMMCIDEVICE_RECORD)
	{
		// 标记设备关闭
		XM_lock ();
		device_state &= ~XMMCIDEVICE_RECORD;
		XM_unlock ();

		int loop = 0;
		// 等待ADC DMA完毕
		while(deviceStatus[_REC])
		{
			loop ++;
			if(loop > 100)
			{
				printf ("XM_WaveClose ADC error\n");
				// while(1);
				break;
			}
			OS_Delay (10);	// 等待10毫秒
		}

		// 关闭录音设备
		adc_exit ();
	}		
	
	if(dwDevice & XMMCIDEVICE_PLAY)
	{
		if(device_ref_count[_PLAY] == 0)
		{
			printf ("illegal ref_count 0\n");
			OS_Unuse (&iis_access_sema);
			return XMMCIERRORCODE_DEVICECLOSED;
		}
		device_ref_count[_PLAY] --;
		if(device_ref_count[_PLAY] > 0)
		{
			printf ("wav_close, ref_count=%d\n", device_ref_count[_PLAY]);
			OS_Unuse (&iis_access_sema);
			return XMMCIERRORCODE_SUCCESS;
		}
		detect_zero_crossing_point_and_close ();
		
		int loop = 0;
		// 等待DMA播放完毕
		while(deviceStatus[_PLAY])
		{
			loop ++;
			if(loop > 100)
			{
				printf ("XM_WaveClose DAC error\n");
				// while(1);
				break;
			}
			OS_Delay (10);	// 等待10毫秒
		}

		// 关闭播放设备
		dac_exit ();
		
		device_state &= ~XMMCIDEVICE_PLAY;
		
		printf ("wav_close, ref_count=%d\n", device_ref_count[_PLAY]);
	}	
	
	OS_Unuse (&iis_access_sema);
	
	XM_printf ("XM_WaveClose end\n");
	return XMMCIERRORCODE_SUCCESS;
}

// 音量静音设置
void XM_WaveSetDacMute (int mute)
{
	OS_Use (&iis_access_sema);
	dac_mute = mute;
	set_volume ();
	OS_Unuse (&iis_access_sema);
}

void XM_WaveSetDacVolume (unsigned int volume)
{
	unsigned int old_vol = dac_vol;
	volume = volume * 0x7F / 20;
	if(volume > 0x7F)
		volume = 0x7F;
	if(dac_vol == volume && !dac_mute)
		return;
	
	OS_Use (&iis_access_sema);
	dac_vol = volume;
	
	if(dac_vol == 0)
	{
		XM_WaveSetDacMute (1);
	}
	else if(old_vol == 0 && dac_vol)
	{
		XM_WaveSetDacMute (0);
	}
	else
	{
		if(!dac_mute)
		{
			set_volume ();
		}
	}
	OS_Unuse (&iis_access_sema);
}


// 向播放设备写入PCM采样数据
LONG XM_WaveWrite	(DWORD dwDevice, WORD *lpPcmSample, LONG cbPcmSample)
{	
	//return 0;
	OS_Use (&iis_access_sema);
	if(!(device_state & XMMCIDEVICE_PLAY))
	{
		OS_Unuse (&iis_access_sema);
		return XMMCIERRORCODE_DEVICECLOSED;	
	}
	if(lpPcmSample == NULL || cbPcmSample == 0)
	{
		OS_Unuse (&iis_access_sema);
		return 0;
	}
	
	//return 0;
	
	// 将PCM数据写入到当前工作缓冲区
	// 计算
	u32_t count = workingFifoSampleCount[_PLAY];
	assert (count > 0);
	
	u32_t sample;
	u32_t *fifo;
		
	// 当前工作缓冲区存在空闲单元
	u32_t dma_index = deviceProducer[_PLAY] % MCI_FIFO_COUNT;
	//printf ("PcmSample=%d\n", cbPcmSample);
	fifo = &deviceFifo[_PLAY][dma_index][0];
	fifo += MCI_DMA_FIFO_SIZE - count;
	
	//printf ("cbPcmSample=%d\n", cbPcmSample);
	while(cbPcmSample > 0)
	{
		if(deviceWaveFormat[_PLAY].wChannels == 1)
		{
			// 单声道
			sample = *lpPcmSample ++;
			sample |= (sample << 16);
			cbPcmSample --;
		}
		else
		{
			// 双声道
			sample = *lpPcmSample ++;
			sample |= *lpPcmSample << 16;
			lpPcmSample ++;
			cbPcmSample -= 2;
		}
		
		*fifo ++ = sample;

		count --;
		if(count == 0)
		{
			// 当前缓冲区已写满
			u32_t index;
			// 将Cache中的数据更新到SDRAM	
			index = deviceProducer[_PLAY];
			index %= MCI_FIFO_COUNT;
			// 将Cache中的数据更新到SDRAM	
			dma_clean_range ((u32)&deviceFifo[_PLAY][index][0],
								  (MCI_DMA_FIFO_SIZE << 2) + (u32)&deviceFifo[_PLAY][index][0] );
					
			deviceProducer[_PLAY] ++;
			
			// 检查DMA是否停止。若是，启动播放
			if(deviceStatus[_PLAY] == 0)
			{
				dac_dma_start ((u32_t) &deviceFifo[_PLAY][index][0],  MCI_DMA_FIFO_SIZE << 2, index);
			}
			else
			{
				//dac_dma_chain ((u32_t) &deviceFifo[_PLAY][index][0],  MCI_DMA_FIFO_SIZE << 2, index, 0);
			}
			
			//deviceProducer[_PLAY] ++;
				
			// 查找下一个缓冲区
			// 等待空闲写缓冲区
			// printf ("OS_WaitCSema\n");
			OS_WaitCSema (&deviceSemaphore[_PLAY]);
			// printf ("OS_WaitCSema End\n");
			workingFifoSampleCount[_PLAY] = MCI_DMA_FIFO_SIZE;
			if(cbPcmSample > 0)
			{
				dma_index = deviceProducer[_PLAY] % MCI_FIFO_COUNT;
				//printf ("wi=%d\n", dma_index);
				fifo = &deviceFifo[_PLAY][dma_index][0];
				count = workingFifoSampleCount[_PLAY];
			}
		}
	}
	
	if(count)
	{
		workingFifoSampleCount[_PLAY] = count;
	}
	
	OS_Unuse (&iis_access_sema);
	
	return 0;
}

// 从录音设备读取PCM采样数据
LONG XM_WaveRead	(DWORD dwDevice, WORD *lpPcmSample, LONG cbPcmSample)
{
	LONG ret = cbPcmSample;
	
	// 避免WaveWrite阻塞
	// OS_Use (&iis_access_sema);
	if(OS_Use_timeout (&iis_access_sema, 5) == 0)
		return 0;
	
	if(!(device_state & XMMCIDEVICE_RECORD))
	{
		OS_Unuse (&iis_access_sema);
		return XMMCIERRORCODE_DEVICECLOSED;
	}
	if(lpPcmSample == NULL || cbPcmSample == 0)
	{
		OS_Unuse (&iis_access_sema);
		return 0;
	}
	
	//printf ("XM_WaveRead cbPcmSample=%d, rec_count=%d\n", cbPcmSample, rec_count);
	
	//printf ("IISADC_CR=0x%08x\n", iisadc_readl(IISADC_CR));
	//printf ("EXT_PARAM_REG11=0x%08x\n", EXT_PARAM_REG11);
	
	OS_IncDI();
	if(cbPcmSample > rec_count)
	{
		OS_DecRI();
		OS_Unuse (&iis_access_sema);
		return 0;
	}
	rec_count -= cbPcmSample;
	OS_DecRI();
	
	while(cbPcmSample > 0)
	{
		u32_t index;
		u32_t count;
		u32_t *fifo;
		u32_t sample;
		
		
		index = deviceConsumer[_REC];
		index %= MCI_FIFO_COUNT;
		
		// 检查当前工作缓冲区
		if(workingFifoSampleCount[_REC] == 0)
		{
			//printf ("read %d, count=%d\n", deviceConsumer[_REC], rec_count);
			OS_WaitCSema (&deviceSemaphore[_REC]);
			/*
			int ret = OS_WaitCSemaTimed (&deviceSemaphore[_REC], 100);
			if(ret == 0)	// timeout
			{
				printf ("OS_WaitCSemaTimed\n");
				printf ("Consumer=%d, Producer=%d\n", deviceConsumer[_REC], deviceProducer[_REC]);
				printf ("deviceStatus=%08x, device_state=%08x\n", deviceStatus[_REC], device_state);
				while(1);
			}
			*/
			workingFifoSampleCount[_REC] = MCI_DMA_FIFO_SIZE;
			
			fifo = &deviceFifo[_REC][index][0];
			dma_inv_range ((u32_t)fifo, ((unsigned int)fifo) + MCI_DMA_FIFO_SIZE);
		}
		
		// assert (deviceProducer[_REC] > deviceConsumer[_REC]);
		
		//index = deviceConsumer[_REC];
		//index %= MCI_FIFO_COUNT;
		
		count = workingFifoSampleCount[_REC];
		fifo = &deviceFifo[_REC][index][MCI_DMA_FIFO_SIZE - count];
		sample = *fifo ++;
		workingFifoSampleCount[_REC] --;	
		if(workingFifoSampleCount[_REC] == 0)
		{
			deviceConsumer[_REC] ++;
			
			// 检查ADC DMA是否已停止
			if(deviceStatus[_REC] == 0)
			{
				//printf ("ADC restart\n");
				u32_t index = deviceProducer[_REC];
				index %= MCI_FIFO_COUNT;
				adc_dma_start ((u32_t)(&deviceFifo[_REC][index][0]), MCI_DMA_FIFO_SIZE << 2);
			}
		}
		if(deviceWaveFormat[_REC].wChannels == 1)
		{
			// 单声道
			*lpPcmSample ++ = (unsigned short)(sample & 0xFFFF);
			//*lpPcmSample ++ = (unsigned short)(sample >> 16);
			cbPcmSample --;
		}
		else
		{
			// 双声道
			*lpPcmSample ++ = (unsigned short)(sample & 0xFFFF);
			*lpPcmSample ++ = (unsigned short)(sample >> 16);
			cbPcmSample -= 2;
		}
	}
	
	OS_Unuse (&iis_access_sema);
	return ret;
}

//#define	DMA_ADC	1
//#define	DMA_DAC	1

// I2S 0
#define	ADC_BUFF_SIZE	0x100000
int iis_adc_test (void)
{
	u32 reg, readval;
	u32 i,j=0;
	
	u16_t *adc_buff, *adc_buff2;
	int rec_size = 0;
	
	while(1)
	{
		int volume_status = FS_GetVolumeStatus ("mmc:0:");
		if(volume_status == FS_MEDIA_IS_PRESENT)
		{
			printf ("SDMMC (slot %d) present\n", 0);
			break;
		}
		OS_Delay (100);
	}
	
	adc_buff  = kernel_malloc (ADC_BUFF_SIZE * sizeof(unsigned short));
	adc_buff2 = kernel_malloc (ADC_BUFF_SIZE * sizeof(unsigned short));
	do
	{
		u16 *rcv_buf  = adc_buff;
		u16 *rcv_buf2 = adc_buff2;
		u16 *temp = rcv_buf;
		u16 *temp2 = rcv_buf2;
		
		if(adc_buff == NULL || adc_buff2 == NULL)
		{
			printf ("iis_adc_test failed, alloc buff NG\n");
			break;
		}
		
		memset (adc_buff,  0, ADC_BUFF_SIZE * sizeof(unsigned short));
		memset (adc_buff2, 0, ADC_BUFF_SIZE * sizeof(unsigned short));
	
	
		rSYS_SOFT_RSTNA &= ~(1 << 15);
		delay (100);
		rSYS_SOFT_RSTNA |=  (1 << 15);
		
		//SetSpeed (XMMCI_DEV_0, 8000);

		//u32 val = rSYS_DEVICE_CLK_CFG0;
		//val &= ~((1<<0));
		//rSYS_DEVICE_CLK_CFG0 = val;
		
		
		printf ("iis_adc_test\n");
		
		while(1)
		{
			// 检查录音键是否按下
			if(XM_GetKeyState(VK_AP_MENU) & 0x8000)
				break;
		}
	
		printf ("recording start\n");
		
		
#ifdef DMA_ADC
		XMWAVEFORMAT fmt;
		fmt.wChannels = 1;
		fmt.wBitsPerSample = 16;
		fmt.dwSamplesPerSec = 8000;
		XM_WaveOpen (XMMCIDEVICE_RECORD, &fmt);
		while(1)
		{
			if(XM_WaveRead (XMMCIDEVICE_RECORD, adc_buff + rec_size, 800) == 800)
			{
				rec_size += 800;
				if(rec_size >= ADC_BUFF_SIZE)
					break;
			}
			OS_Delay (1);
				// 录制结束,退出. 
				if(XM_GetKeyState(VK_AP_MODE) & 0x8000)
					break;
		}
		XM_WaveClose (XMMCIDEVICE_RECORD);
		//rec_size = 8000 * 2;
		/*
		adc_init ();
		rec_size = 0;
		while(1)
		{
			unsigned int adc_fifo[16];
			adc_dma_start ((u32_t)(adc_fifo), 16*sizeof(UINT32));
			
			rec_size += 16*sizeof(UINT32);
			if(rec_size >= (ADC_BUFF_SIZE))
				break;
				// 录制结束,退出. 
				if(XM_GetKeyState(VK_AP_MODE) & 0x8000)
					break;
		}*/
		
#else
		
#if DEMO_BB
		// MUTE(低有效)
		SetGPIOPadDirection (GPIO32, euOutputPad);
		SetGPIOPadData (GPIO32, euDataLow);
#endif
		OS_Delay (1);
		SetSpeed (XMMCI_DEV_0, 8000);
		adc_init ();
		
		rec_size = 0;
		while(1)
		{
			reg =  iisadc_readl(IISADC_FIFOS); //
			if (reg & (1 << 6))
				printf("rcv fifo overrun\n");
			//printf("reg fifo status =%x\n", reg);
			if(!(reg & IISADC_FIFOS_RFE))
			{
				//u32 num = ((reg >> 16) & 0x3f);
				//printf("num =%d, status =%x\n", num, reg);
				for(i=0; i < 1 ; i++)
				{
					readval = iisadc_readl(IISADC_DR);	
					*temp = (unsigned short)readval;
					temp++;
					*temp2 = (unsigned short)(readval>>16); 
					temp2++;
				}
				
				rec_size = (temp - rcv_buf);
				if( rec_size >= ADC_BUFF_SIZE )
				{
					break;
				}
				
				// 录制结束,退出. 
				if(XM_GetKeyState(VK_AP_MODE) & 0x8000)
					break;
			}
			
		}
#endif
		
		// 保存左声道
		FS_FILE    * pFile;
		pFile = FS_FOpen ("\\rec.pcm", "wb");
		if(pFile)
		{
			FS_Write (pFile, rcv_buf, rec_size * 2);
			FS_FClose (pFile);
			FS_CACHE_Clean ("");
		}
		
		// 保存右声道
		pFile = FS_FOpen ("\\rec2.pcm", "wb");
		if(pFile)
		{
			FS_Write (pFile, rcv_buf2, rec_size * 2);
			FS_FClose (pFile);
			FS_CACHE_Clean ("");
		}
		
	} while(0);
	
	if(adc_buff)
		kernel_free (adc_buff);
	if(adc_buff2)
		kernel_free (adc_buff2);
		
	printf ("recording end\n");
	
#if DEMO_BB
	// MUTE 关闭
	SetGPIOPadDirection (GPIO32, euOutputPad);
	SetGPIOPadData (GPIO32, euDataHigh);
#endif
	OS_Delay (1);
	
	return 0;
}

// I2S 1
int iis_dac_test (void)
{
	int ret = -1;
	u16_t pcm;
	u16 *adc_buff = kernel_malloc (ADC_BUFF_SIZE * sizeof(unsigned short));
	do
	{
		u16 *snd_buf = adc_buff;
		int  snd_cnt;
		u32  reg;
	
		while(1)
		{
			int volume_status = FS_GetVolumeStatus ("mmc:0:");
			if(volume_status == FS_MEDIA_IS_PRESENT)
			{
				printf ("SDMMC (slot %d) present\n", 0);
				break;
			}
			OS_Delay (100);
		}
		
				
		printf ("iis_dac_test\n");
		FS_FILE *fp = FS_FOpen ("\\REC.PCM", "rb");
		if(fp == NULL)
		{
			printf ("open \\rec.pcm failed\n");
			break;
		}
		snd_cnt = FS_FRead (adc_buff, 1, ADC_BUFF_SIZE * sizeof(unsigned short), fp);
		FS_FClose (fp);
		if(snd_cnt == 0)
		{
			printf ("open \\rec.pcm failed\n");
			break;		
		}
		snd_cnt /= 2;
			
#ifdef DMA_DAC

		XMWAVEFORMAT fmt;
		fmt.wChannels = 1;
		fmt.wBitsPerSample = 16;
		fmt.dwSamplesPerSec = 8000;
		XM_WaveOpen (XMMCIDEVICE_PLAY, &fmt);
		u32_t idx = 0;
		while(1)
		{
			if(XM_WaveWrite (XMMCIDEVICE_PLAY, adc_buff + idx, 800) == 0)
			{
				idx += 800;
				if(idx >= snd_cnt)
					break;
			}
			OS_Delay (1);
		}
		XM_WaveClose (XMMCIDEVICE_PLAY);
		
#else
		dac_init ();

		
		// mute设置
		SetGPIOPadDirection (GPIO0,  euOutputPad);
		SetGPIOPadData (GPIO0, euDataHigh);
				
		int count = snd_cnt;
		snd_cnt = count;
		snd_buf = adc_buff;
		while(snd_cnt > 0)
		{
			// bit 0
			// TNF 	Transmit FIFO not full:
			//			0: transmit FIFO is not full.
			// 		1: transmit FIFO is full.
			while( rIIS_SASR0&0x1);
				
			pcm = *snd_buf;
			rIIS_SATR = pcm | (pcm << 16);
			snd_buf ++;
			snd_cnt --;
		}
		
		OS_Delay (100);
		dac_exit ();
#endif
	
		printf ("play end\n");
		ret = 0;
	} while(0);
	
	if(adc_buff)
		kernel_free (adc_buff);
	
	return ret;
}

void adc_dac_test (void)
{
	XM_WaveInit ();
	while(1)
	{
		iis_adc_test ();
		iis_dac_test ();
	}
}

#if TULV
#define	AUDIO_MUTE_GPIO	GPIO32
#elif HONGJING_CVBS
#define	AUDIO_MUTE_GPIO	GPIO32
#endif

static void audio_mute_open (void)
{
#if TULV
	
	unsigned int val;
	// MUTE	cd3
	// pad_ctl8 
	XM_lock ();
    #if 0
	val = rSYS_PAD_CTRL08;
	val &= ~(0x03 << 6);
	rSYS_PAD_CTRL08 = val;
    #endif
	XM_unlock ();
	
#elif HONGJING_CVBS
	unsigned int val;
	// MUTE	GPIO32
	// pad_ctl3
	// [8:6]	itu_c_hsync_in	itu_c_hsync_in_pad	GPIO32	itu_c_hsync_in
	XM_lock ();
	val = rSYS_PAD_CTRL03;
	val &= ~(0x07 << 6);
	rSYS_PAD_CTRL03 = val;
	XM_unlock ();	
#endif	

#if TULV || HONGJING_CVBS
//	XM_lock ();	
//	SetGPIOPadDirection (AUDIO_MUTE_GPIO, euOutputPad);
//	SetGPIOPadData (AUDIO_MUTE_GPIO, euDataLow);		// 静音
//	XM_unlock();
#endif
    pa_close();

}

static void audio_mute_close (void)
{
	
}

// 静音使能
static void audio_mute_enable (void)
{
    XM_printf("audio_mute_enable\n");
    pa_close();


#if TULV || HONGJING_CVBS
//	XM_lock ();	
//	SetGPIOPadData (AUDIO_MUTE_GPIO, euDataLow);		
//	XM_unlock();
#endif	
}

// 正常播放
static void audio_mute_disable (void)
{
	XM_printf("audio_mute_disable\n");
    pa_set_d_class();

#if TULV || HONGJING_CVBS
//	XM_lock ();	
//	SetGPIOPadData (AUDIO_MUTE_GPIO, euDataHigh);	
//	XM_unlock();	
#endif		
}
