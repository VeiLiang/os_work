
/***********************************************************************
*Copyright (c)2012  Arkmicro Technologies Inc. 
*All Rights Reserved 
*
*Filename:    dma.c
*Version :    1.0 
*Date    :     
*Author  :     
*Abstract:    ark1660  Ver1.0 MPW driver remove waring   
*History :     
* 
*Version :    2.0 
*Date    :    2012.02.27
*Author  :     
*Abstract:    ark1660  Ver1.0 MPW driver remove waring
*History :    1.0

************************************************************************/
 
#include <xm_proj_define.h>
#include "hardware.h"
#include <stdlib.h>
#include "xm_core.h"
#include <assert.h>

#undef printk
#define printk printf

typedef struct
{
	UINT32 IdleStatus;
	UINT32 IRQMask;
	
	void (*pfnChannelIRQHandler)(UINT32 ulIRQFactors, UINT32 channel, void *channel_private_data);// removed waring
   
   // 20140106 ZhuoYongHong
   void *channel_private_data;      // 通道私有数据
}TChannelManagerInfo;

#pragma data_alignment = 32
static TChannelManagerInfo lg_stChannelManagerInfo[MAX_DMA_CH_NUM];


#pragma data_alignment = 32
static OS_CSEMA DmaChxTfrOver_Sem[MAX_DMA_CH_NUM];
static OS_RSEMA DmaChannelManagerAccessSema;		// 保护lg_stChannelManagerInfo的互斥访问

// 静态DMA拷贝比动态DMA拷贝稍快
#define	STATIC_DMA_MEMCPY_CHANNEL

#define	_DMA_MEMCPY_USE_LLI_DMA_


#ifdef _DMA_MEMCPY_USE_LLI_DMA_

// 20170319
// DMA MEMCPY资源预先分配
#define DMA_MEMCPY_MAX_BATCH_TRANS_PAGE		8		// page's size is 512
#define DMA_MEMCPY_CHANNEL							3

#define	DMA_MEMCPY_MAX_LENGTH		0x100000		// DMA MEMCPY每次最大拷贝字节长度

#ifdef STATIC_DMA_MEMCPY_CHANNEL			// 专用于DMA MEMCPY的静态分配的DMA通道, 只允许顺序访问
static unsigned int dma_memcpy_dma_channel;		// DMA通道
#pragma data_alignment = 32
static struct dw_lli dma_memcpy_dma_lli[DMA_MEMCPY_MAX_LENGTH/(DMA_MEMCPY_MAX_BATCH_TRANS_PAGE*512)];			// DMA链
static OS_RSEMA dma_memcpy_access_sema;
#endif

#endif

// 预留DMA通道用于scatter DMA拷贝, scatter DMA用于旋转，Icon显示过程， 大量访问
static unsigned int scatter_dma_memcpy_dma_channel;		// DMA通道
static OS_RSEMA scatter_dma_memcpy_access_sema;


// 0: channel is transfering; other:data is transfer over
inline UINT32 dma_transfer_over(UINT8 channel)
{
	return rDMA_RAW_TRF_L & (1<<channel);
}

void dma_channel_status_init(void)
{
	INT32 i;
	
	rDMA_CHEN_L = 0xff00;//disable channels 

	for(i=0;i<MAX_DMA_CH_NUM;i++)
	{
		lg_stChannelManagerInfo[i].IdleStatus = CHANNEL_STATE_IDLE;
	}	
}

UINT32 dma_request_dma_channel(UINT32 nFavoriteChannel)
{
	UINT32 ulChannel;
	
	OS_Use(&DmaChannelManagerAccessSema);
	if(nFavoriteChannel < MAX_DMA_CH_NUM)
	{
		if(lg_stChannelManagerInfo[nFavoriteChannel].IdleStatus == CHANNEL_STATE_IDLE)
		{
			lg_stChannelManagerInfo[nFavoriteChannel].IdleStatus = CHANNEL_STATE_RUNNING;
			OS_Unuse(&DmaChannelManagerAccessSema);
			return nFavoriteChannel;
		}
		else
		{
			for(ulChannel=0; ulChannel<MAX_DMA_CH_NUM; ulChannel++)
			{
				if(lg_stChannelManagerInfo[ulChannel].IdleStatus == CHANNEL_STATE_IDLE)
				{
					lg_stChannelManagerInfo[ulChannel].IdleStatus = CHANNEL_STATE_RUNNING;
					OS_Unuse(&DmaChannelManagerAccessSema);
					return ulChannel;
				}
			}

			OS_Unuse(&DmaChannelManagerAccessSema);
			return ALL_CHANNEL_RUNNING;
		}
	}
	else
	{
		OS_Unuse(&DmaChannelManagerAccessSema);
		return ALL_CHANNEL_RUNNING;
	}
}

void dma_release_channel(UINT32 nChannel)
{
	OS_Use(&DmaChannelManagerAccessSema);
	if(nChannel < MAX_DMA_CH_NUM)
	{
		lg_stChannelManagerInfo[nChannel].IdleStatus = CHANNEL_STATE_IDLE;
		lg_stChannelManagerInfo[nChannel].pfnChannelIRQHandler = NULL;
		lg_stChannelManagerInfo[nChannel].IRQMask = 0;
		lg_stChannelManagerInfo[nChannel].channel_private_data = NULL;

		rDMA_MASK_BLOCK_L = (1 << (8+nChannel));
		rDMA_MASK_DST_TRAN_L = (1 << (8+nChannel));
		rDMA_MASK_ERR_L = (1 << (8+nChannel));
		rDMA_MASK_SRC_TRAN_L = (1 << (8+nChannel));
		rDMA_MASK_TRF_L = (1 << (8+nChannel));

		dma_stop_channel(nChannel);
	}
	OS_Unuse(&DmaChannelManagerAccessSema);
}

void dma_stop_channel(UINT32 nChannel)
{
	unsigned int timeout = XM_GetTickCount() + 1000;
	if(nChannel < MAX_DMA_CH_NUM)
	{
		if(!(rDMA_CHEN_L & (1<<nChannel)))
			return;
	
		rDMA_CFGx_L(nChannel) |= 1<<8;
		// while(!(rDMA_CFGx_L(nChannel) & (1<<9)));	//to avoid hangup while src dma width is different from dst dma width, i note this line code. Donier 2012-04-21

		rDMA_CHEN_L = 1<<(8+nChannel);
		while(rDMA_CHEN_L & (1<<nChannel))
		{
			if(XM_GetTickCount() >= timeout)
			{
				XM_printf ("dma_stop_channel %d timeout\n", nChannel);
				break;
			}
		}
		rDMA_CFGx_L(nChannel) &= ~(1<<8);
	}
}

// 20140106 zhuoyonghong
void register_channel_IRQHandler(UINT32 channel, 
	void (*pfnIRQHandler)(UINT32 ulIRQFactors, UINT32 channel, void *channel_private_data), 
   void *channel_private_data,
	UINT32 irq_mask)
{
	OS_Use(&DmaChannelManagerAccessSema);
	if(channel < MAX_DMA_CH_NUM)
	{
		if(pfnIRQHandler)
		{
			lg_stChannelManagerInfo[channel].pfnChannelIRQHandler = pfnIRQHandler;
			lg_stChannelManagerInfo[channel].IRQMask = irq_mask;
			lg_stChannelManagerInfo[channel].channel_private_data = channel_private_data;

			if(irq_mask & (1<< BLOCK_INT_BIT))
				rDMA_MASK_BLOCK_L |= (1<<(8+channel)|(1<<channel));
//				rDMA_MASK_BLOCK_L |= 1 << channel;

			if(irq_mask & (1<<DST_INT_BIT))
				rDMA_MASK_DST_TRAN_L |= (1<<(8+channel)|(1<<channel));
				//rDMA_MASK_DST_TRAN_L |= 1 << DST_INT_BIT;
				
			if(irq_mask & (1<<ERR_INT_BIT))
				rDMA_MASK_ERR_L |= (1<<(8+channel)|(1<<channel));
				//rDMA_MASK_ERR_L |= 1 << ERR_INT_BIT;
				
			if(irq_mask & (1<<SRC_INT_BIT))
				rDMA_MASK_SRC_TRAN_L |= (1<<(8+channel)|(1<<channel));
				//rDMA_MASK_SRC_TRAN_L |= 1 << SRC_INT_BIT;
			
			if(irq_mask & (1<<TFR_INT_BIT))
				rDMA_MASK_TRF_L |= (1<<(8+channel)|(1<<channel));
				//rDMA_MASK_TRF_L |= 1 << TFR_INT_BIT;			
		}
	}
	OS_Unuse(&DmaChannelManagerAccessSema);
}

static void dma_IRQHandler()
{	
	UINT32 ulRegStatusBlock;
	UINT32 ulRegStatusDstTran;
	UINT32 ulRegStatusErr;
	UINT32 ulRegStatusSrcTran;
	UINT32 ulRegStatusTfr;
	UINT32 ulChannelIRQ_Factor;
	UINT32 i;

	ulRegStatusBlock = rDMA_STATUS_BLOCK_L;
	ulRegStatusDstTran = rDMA_STATUS_DST_TRAN_L;
	ulRegStatusErr = rDMA_STATUS_ERR_L;
	ulRegStatusSrcTran = rDMA_STATUS_SRC_TRAN_L;
	ulRegStatusTfr = rDMA_STATUS_TRF_L;
	
	for(i=0;i<MAX_DMA_CH_NUM;i++)
	{
		ulChannelIRQ_Factor = 0;
		
		if(ulRegStatusBlock & (1<< i))
			ulChannelIRQ_Factor |= 1 << BLOCK_INT_BIT;

		if(ulRegStatusDstTran & (1<<i))
			ulChannelIRQ_Factor |= 1 << DST_INT_BIT;
			
		if(ulRegStatusErr & (1<<i))
			ulChannelIRQ_Factor |= 1 << ERR_INT_BIT;
			
		if(ulRegStatusSrcTran & (1<<i))
			ulChannelIRQ_Factor |= 1 << SRC_INT_BIT;
		
		if(ulRegStatusTfr & (1<<i))
			ulChannelIRQ_Factor |= 1 << TFR_INT_BIT;
			
		if(ulChannelIRQ_Factor)
		{
			if(lg_stChannelManagerInfo[i].pfnChannelIRQHandler)
			{
				// lg_stChannelManagerInfo[i].pfnChannelIRQHandler(ulChannelIRQ_Factor);// removed waring
            lg_stChannelManagerInfo[i].pfnChannelIRQHandler(ulChannelIRQ_Factor,
                                                            i, 
                                                            lg_stChannelManagerInfo[i].channel_private_data);// removed waring
				// 20140513 ZhuoYongHong
				// 屏蔽下面的清除中断语句。
				//	在应用DMA接收UART字符时，使能下面的dma_clr_int将导致UART DMA无法连续接收。
				// DMA中断处理函数需要在实现中自己清除中断
				// dma_clr_int(i);
			}
		}		
	}
}

void dma_cfg_channel(
	UINT8 channel,
	UINT32 srcaddr,
	UINT32 destaddr,
	UINT32 lli,
	UINT32 control_l,
	UINT32 control_h,
	UINT32 configure_l,
	UINT32 configure_h,
	UINT32 srcGather,
	UINT32 dstScatter
	)
{
	if(channel >= MAX_DMA_CH_NUM)
	{
		printk ("illegal dma channel (%d)\n", channel);
		return;
	}

	rDMA_SARx_L(channel)=  		srcaddr;
			
	rDMA_DARx_L(channel)=  		destaddr;
			
	rDMA_LLPx_L(channel)	= 		lli;
			
	rDMA_CTLx_L(channel) = 		control_l;
	rDMA_CTLx_H(channel) = 		control_h;
			
	rDMA_CFGx_L(channel)	= 		configure_l;
	rDMA_CFGx_H(channel) = 		configure_h;
	// gather and scatter only support in channel:ch0 ch1 ch2 ch3,(total 4 channels)
	if(channel < (MAX_DMA_CH_NUM>>1) )
	{
		rDMA_SGRx_L(channel) = 		srcGather;
		rDMA_DSRx_L(channel) = 		dstScatter;
	}
}
 
//#define	DMA_PRINT_PARAMETER
void dma_init(void)
{	
	int i;
	for (i = 0; i < MAX_DMA_CH_NUM; i ++)
		OS_CreateCSema (&DmaChxTfrOver_Sem[i], 0);
	OS_CREATERSEMA(&DmaChannelManagerAccessSema);
   
	sys_clk_disable (dma_hclk_enable);
	
	sys_soft_reset (softreset_dma);
	sys_soft_reset (softreset_h2xdma);
	/*
	rSYS_SOFT_RSTNA &= ~(1<<24);
	__asm ("nop");
   __asm ("nop");
	rSYS_SOFT_RSTNA |= (1<<24);
   __asm ("nop");
   __asm ("nop");
	*/
	sys_clk_enable (dma_hclk_enable);
   
	request_irq(DMA_INT, PRIORITY_FIFTEEN, dma_IRQHandler);
	dma_channel_status_init();	
	rDMA_CFG_L |= (1<<0);//enable DW_ahb_dmac

	// 仅通道0, 1, 2, 3支持scatter DMA
	scatter_dma_memcpy_dma_channel = dma_request_dma_channel (0);
	assert (scatter_dma_memcpy_dma_channel == 0);
	OS_CREATERSEMA(&scatter_dma_memcpy_access_sema);	
	
#ifdef _DMA_MEMCPY_USE_LLI_DMA_
#ifdef STATIC_DMA_MEMCPY_CHANNEL	
	// 预先分配专用于DMA内存拷贝的DMA通道
	dma_memcpy_dma_channel = dma_request_dma_channel (DMA_MEMCPY_CHANNEL);
	OS_CREATERSEMA(&dma_memcpy_access_sema);
#endif
#endif
	
#ifdef DMA_PRINT_PARAMETER
	unsigned int DMA_COMP_PARAMS;
	const unsigned int MAX_MULT_SIZE[] = {4, 8, 16, 32, 64, 128, 256, 0};
	const unsigned int FIFO_DEPTH[]    = {8, 16, 32, 64, 128, 0, 0, 0};
	
	printf ("DMA Component Parameter\n");
	DMA_COMP_PARAMS = rDMA_COMP_PARAMS_1_L;
	printf ("rDMA_COMP_PARAMS_1_L=0x%08x\n", DMA_COMP_PARAMS);
	
	printf ("CH0 Component Parameter\n");
	DMA_COMP_PARAMS = rDMA_COMP_PARAMS_2_L;
	// 32, 64 (ARKN141实际值)
	printf ("\tMaximum value of burst transaction size    : %4d \n",  MAX_MULT_SIZE[(DMA_COMP_PARAMS >> 16) & 7]);
	printf ("\tFIFO depth in bytes                        : %4d \n",  FIFO_DEPTH[(DMA_COMP_PARAMS >> 28) & 7]);

	DMA_COMP_PARAMS = rDMA_COMP_PARAMS_3_H;
	printf ("CH1 Component Parameter\n");
	// 32, 64 (ARKN141实际值)
	printf ("\tMaximum value of burst transaction size    : %4d \n",  MAX_MULT_SIZE[(DMA_COMP_PARAMS >> 16) & 7]);
	printf ("\tFIFO depth in bytes                        : %4d \n",  FIFO_DEPTH[(DMA_COMP_PARAMS >> 28) & 7]);
	
	DMA_COMP_PARAMS = rDMA_COMP_PARAMS_3_L;
	printf ("CH2 Component Parameter\n");
	// 32, 64 (ARKN141实际值)
	printf ("\tMaximum value of burst transaction size    : %4d \n",  MAX_MULT_SIZE[(DMA_COMP_PARAMS >> 16) & 7]);
	printf ("\tFIFO depth in bytes                        : %4d \n",  FIFO_DEPTH[(DMA_COMP_PARAMS >> 28) & 7]);
	
	DMA_COMP_PARAMS = rDMA_COMP_PARAMS_4_H;
	printf ("CH3 Component Parameter\n");
	// 32, 64 (ARKN141实际值)
	printf ("\tMaximum value of burst transaction size    : %4d \n",  MAX_MULT_SIZE[(DMA_COMP_PARAMS >> 16) & 7]);
	printf ("\tFIFO depth in bytes                        : %4d \n",  FIFO_DEPTH[(DMA_COMP_PARAMS >> 28) & 7]);
	printf ("\tSRC GAT EN                                 : %s \n",  (DMA_COMP_PARAMS >> 9) & 1 ? "TRUE" : "FALSE"  );
	printf ("\tDST GAT EN                                 : %s \n",  (DMA_COMP_PARAMS >> 8) & 1 ? "TRUE" : "FALSE"  );

	DMA_COMP_PARAMS = rDMA_COMP_PARAMS_4_L;
	printf ("CH4 Component Parameter\n");
	// 32, 64 (ARKN141实际值)
	printf ("\tMaximum value of burst transaction size    : %4d \n",  MAX_MULT_SIZE[(DMA_COMP_PARAMS >> 16) & 7]);
	printf ("\tFIFO depth in bytes                        : %4d \n",  FIFO_DEPTH[(DMA_COMP_PARAMS >> 28) & 7]);
	printf ("\tSRC GAT EN                                 : %s \n",  (DMA_COMP_PARAMS >> 9) & 1 ? "TRUE" : "FALSE"  );
	printf ("\tDST GAT EN                                 : %s \n",  (DMA_COMP_PARAMS >> 8) & 1 ? "TRUE" : "FALSE"  );
	

	DMA_COMP_PARAMS = rDMA_COMP_PARAMS_5_H;
	printf ("CH5 Component Parameter\n");
	// 16, 32 (ARKN141实际值)
	printf ("\tMaximum value of burst transaction size    : %4d \n",  MAX_MULT_SIZE[(DMA_COMP_PARAMS >> 16) & 7]);
	printf ("\tFIFO depth in bytes                        : %4d \n",  FIFO_DEPTH[(DMA_COMP_PARAMS >> 28) & 7]);

	DMA_COMP_PARAMS = rDMA_COMP_PARAMS_5_L;
	printf ("CH6 Component Parameter\n");
	// 16, 32 (ARKN141实际值)
	printf ("\tMaximum value of burst transaction size    : %4d \n",  MAX_MULT_SIZE[(DMA_COMP_PARAMS >> 16) & 7]);
	printf ("\tFIFO depth in bytes                        : %4d \n",  FIFO_DEPTH[(DMA_COMP_PARAMS >> 28) & 7]);

	DMA_COMP_PARAMS = rDMA_COMP_PARAMS_6_H;
	printf ("CH7 Component Parameter\n");
	// 64, 128 (实际值)
	printf ("\tMaximum value of burst transaction size    : %4d \n",  MAX_MULT_SIZE[(DMA_COMP_PARAMS >> 16) & 7]);
	printf ("\tFIFO depth in bytes                        : %4d \n",  FIFO_DEPTH[(DMA_COMP_PARAMS >> 28) & 7]);
	
	printf ("\n\n");
	printf ("DMA Component Parameter END\n");
#endif
}

void dma_clr_int(UINT8 channel)
{
	dma_clr_trans(channel);
	dma_clr_block(channel);
	dma_clr_dst_trans(channel);
	dma_clr_src_trans(channel);
	dma_clr_err(channel);
}


void dma_detect_ch_disable(UINT8 ch)
{
	UINT32 count=0;
	
	while(rDMA_CHEN_L&(1<<ch))
	{
		count++;
		if(count>DMA_TIMEOUT)
		{	
			printk("Wait DMA Channel%d disable timeout\n",ch);
			break;
		}	
	}
}
void dma_int_clrBlk(UINT32 ch)
{
	rDMA_CLEAR_BLOCK_L = (1<<ch);
}

void dma_int_clrDstTran(UINT32 ch)
{
	rDMA_CLEAR_DST_TRAN_L = (1<<ch);
}

void dma_int_clrErr(UINT32 ch)
{
	rDMA_CLEAR_ERR_L = (1<<ch);
}

void dma_int_clrSrcTran(UINT32 ch)
{
	rDMA_CLEAR_SRC_TRAN_L = (1<<ch);
}

void dma_int_clrTfr(UINT32 ch)
{
	rDMA_CLEAR_TRF_L = (1<<ch);
}

void dma_int_clr(UINT32 ch)
{
	UINT32 count=0;

	dma_int_clrTfr(ch);
	dma_int_clrBlk(ch);
	dma_int_clrSrcTran(ch);
	dma_int_clrDstTran(ch);
	dma_int_clrErr(ch);
	
	while(rDMA_RAW_TRF_L>>ch || rDMA_RAW_BLOCK_L>>ch || rDMA_RAW_SRC_TRAN_L>>ch || 
		  rDMA_RAW_DST_TRAN_L>>ch || rDMA_RAW_ERR_L>>ch)
	{
		count++;
		if(count > DMA_TIMEOUT)
		{
			printk("Error!Clear dma interrupt timeout.\n");
			break;
		}
	}
}

INT32 dma_ch_request(UINT32 ch)
{
	INT32 i;
	INT32 val;

	if(ch < MAX_DMA_CH_NUM)
	{
		val = (rDMA_CHEN_L>>ch) & 1;
		if(val)
		{
			for(i=0;i<MAX_DMA_CH_NUM;i++)
			{
				val = (rDMA_CHEN_L>>i) & 1;
				if(val == 0)
					return i;
			}
		}
		else
			return ch;
	}
	else
		return -1;
	return -1;//removed waring
}

void dma_DescCtl_config(UINT32 *descCtl_H, UINT32 *descCtl_L, DMA_ctlReg_Para *para)
{
	*descCtl_H = para->blkTS;
	*descCtl_L = (para->llpSrcEn<<28) | (para->llpDstEn<<27) | (para->sms<<25) | (para->dms<<23)
				| (para->ttFC<<20) | (para->dstScatterEn<<18) | (para->srcGatherEn<<17) |
				(para->srcMSize<<14) | (para->dstMSize<<11) | (para->sinc<<9) | (para->dinc<<7) |
				(para->srcTrWidth<<4) | (para->dstTrWidth<<1) | (para->intEn);	
}

void dma_sarReg_config(UINT32 ch, UINT32 srcAddr)
{
	if(ch >= MAX_DMA_CH_NUM)
	{
		printk ("illegal dma channel (%d)\n", ch);
		return;
	}

	rDMA_SARx_L(ch) = srcAddr;
}

void dma_darReg_config(UINT32 ch, UINT32 dstAddr)
{
	if(ch >= MAX_DMA_CH_NUM)
	{
		printk ("illegal dma channel (%d)\n", ch);
		return;
	}

	rDMA_DARx_L(ch) = dstAddr;
}

void dma_ctlReg_config(UINT32 ch, DMA_ctlReg_Para *para)
{
	if(ch >= MAX_DMA_CH_NUM)
	{
		printk ("illegal dma channel (%d)\n", ch);
		return;
	}
		
			rDMA_CTLx_H(ch) = para->blkTS;
			rDMA_CTLx_L(ch) = (para->llpSrcEn<<28) | (para->llpDstEn<<27) | (para->sms<<25) | (para->dms<<23)
						| (para->ttFC<<20) | (para->dstScatterEn<<18) | (para->srcGatherEn<<17) |
						(para->srcMSize<<14) | (para->dstMSize<<11) | (para->sinc<<9) | (para->dinc<<7) |
						(para->srcTrWidth<<4) | (para->dstTrWidth<<1) | (para->intEn);
	
}

void dma_cfgReg_config(UINT32 ch, DMA_cfgReg_Para *para)
{
	if(ch >= MAX_DMA_CH_NUM)
	{
		printk ("illegal dma channel (%d)\n", ch);
		return;
	}

			rDMA_CFGx_H(ch) = (para->dstPer<<12) | (para->srcPer<<7) | (para->ssUpdEn<<6) | (para->dsUpdEn<<5) |
							(para->protCtl<<2) | (para->fifoMode<<1) | (para->fcMode);
			rDMA_CFGx_L(ch) = (para->reloadDst<<31) | (para->reloadSrc<<30) | (para->maxAbrst<<20) | (para->srcHsPol<<19) |
							(para->dstHsPol<<18) | (para->lockB<<17) | (para->lockCh<<16) | (para->lockBL<<14) | 
							(para->lockChL<<12) | (para->hsSelSrc<<11) | (para->hsSelDst<<10) | (para->chSusp<<8) |
							(para->chPrior<<5);	
}

void dma_dsrReg_config(UINT32 ch, UINT32 count, UINT32 interval)
{
	if(ch >= MAX_DMA_CH_NUM)
	{
		printk ("illegal dma channel (%d)\n", ch);
		return;
	}
	rDMA_DSRx_L(ch) = (count<<20) | interval;
}

void dma_sgrReg_config(UINT32 ch, UINT32 count, UINT32 interval)
{
	if(ch >= MAX_DMA_CH_NUM)
	{
		printk ("illegal dma channel (%d)\n", ch);
		return;
	}
	rDMA_SGRx_L(ch) = (count<<20) | interval;
}


void Dma_TfrOver_PostSem(UINT32 ch)
{
	if(ch >= MAX_DMA_CH_NUM)
	{
		printk ("illegal dma channel (%d)\n", ch);
		return;
	}
	OS_SignalCSema (&DmaChxTfrOver_Sem[ch]);
}


void Dma_TfrOver_PendSem(UINT32 ch)
{
	if(ch >= MAX_DMA_CH_NUM)
	{
		printk ("illegal dma channel (%d)\n", ch);
		return;
	}
	OS_WaitCSema (&DmaChxTfrOver_Sem[ch]);
}

int Dma_TfrOver_PendSemEx(UINT32 ch, unsigned int timeout_ms)
{
	if(ch >= MAX_DMA_CH_NUM)
	{
		printk ("illegal dma channel (%d)\n", ch);
		return -1;
	}
	return OS_WaitCSemaTimed (&DmaChxTfrOver_Sem[ch], timeout_ms);
}

void Dma_TfrOver_Int(UINT32 ulIRQFactors,UINT32 ch)
{
	if(rDMA_RAW_TRF_L & (1<<ch))
	{
		//printk("Release Sd Dma CH%d TfrOver Sem\n");	
		Dma_TfrOver_PostSem(ch);
		dma_detect_ch_disable(ch);
		dma_int_clr(ch);	
		dma_release_channel(ch);
	}			
}

void dma_suspend_channel (UINT32 ch)
{
	if (rDMA_CHEN_L & (1<<ch))	
	{
		// 1) Channel Suspend. Suspends all DMA data transfers from the source until this bit is cleared
		rDMA_CFGx_L(ch) |= (1 << 8);
		// 2) Software can now poll the CFGx.FIFO_EMPTY bit until it indicates that the channel FIFO is empty.
		while(!(rDMA_CFGx_L(ch) & (1 << 9)));
		// 3) The ChEnReg.CH_EN bit can then be cleared by software once the channel FIFO is empty.
		rDMA_CHEN_L = 1<<(8+ch);
		while(rDMA_CHEN_L & (1<<ch));
	}
}


#define M2M_COPY_DMA_CTL_L 	((0<<28)\
										|(0<<27)\
										|(0<<25)\
										|(0<<23)\
										|(M2M_DMAC<<20)\
										|(0<<18)\
										|(0<<17)\
										|(DMA_BURST_SIZE_8<<14)\
										|(DMA_BURST_SIZE_8<<11)\
										|(0<<9)\
										|(0<<7)\
										|(DMA_WIDTH_16<<4)\
										|(DMA_WIDTH_16<<1)\
										|(1<<0))
								
#define M2M_COPY_DMA_CTL_H 	(0<<12)

// 17 R/W 0x1
//	Bus Lock Bit. When active, the AHB bus master
//	signal hlock is asserted for the duration specified in
//	CFGx.LOCK_B_L.
// DMA访问SRAM时不能设置Bus Lock Bit, 否则会将总线挂死.
// DMA访问DRAM时设置Bus Lock Bit, 可以改善访问效率.
// 20170314 不能设置Bus Lock Bit, 否则会导致ISP 3D bus bandwidth abnormal
#define M2M_COPY_DMA_CFG_L		((0<<31)\
										|(0<<30)\
										|(0<<17)\
										|(0<<8))


#define M2M_COPY_DMA_CFG_H		((0<<12)\
										|(1<<6)\
										|(1<<5)\
										|(1<<2)\
										|(1<<1))


// 测试中发现, 启动MEMCPY DMA后, ISP会出现Bus Width Abnormal. 
// 分析后确认是ISP访问DDR端口的优先级低于DMA访问DDR的端口1的优先级. 
// 大块内存DMA拷贝导致ISP DDR写入阻塞, 从而出现Bus Width Abnormal. 
// DMA访问DDR的端口2的优先级低于ISP, 因此内存拷贝DMA的Source Master Select 及 Destination Master Select均选择第二个端口,
// 第二个端口的优先级低于ISP端口的优先级.
#define DMA_MEMCPY_CTL_L_LLI_16 	((0<<28)\
										|(0<<27)\
	/* 端口2 */						|(1<<25)\
	/* 端口2 */						|(1<<23)\
										|(M2M_DMAC<<20)\
										|(0<<18)\
										|(0<<17)\
										|(DMA_BURST_SIZE_16<<14)\
										|(DMA_BURST_SIZE_16<<11)\
										|(0<<9)\
										|(0<<7)\
										|(DMA_WIDTH_16<<4)\
										|(DMA_WIDTH_16<<1)\
										|(1<<0))

#define DMA_MEMCPY_CTL_L_LLI_32 	((0<<28)\
										|(0<<27)\
		/* 端口2 */					|(1<<25)\
		/* 端口2 */					|(1<<23)\
										|(M2M_DMAC<<20)\
										|(0<<18)\
										|(0<<17)\
										|(DMA_BURST_SIZE_16<<14)\
										|(DMA_BURST_SIZE_16<<11)\
										|(0<<9)\
										|(0<<7)\
										|(DMA_WIDTH_32<<4)\
										|(DMA_WIDTH_32<<1)\
										|(1<<0))

#define DMA_MEMCPY_DMA_CTL_H 	(0<<12)


#define DMA_MEMCPY_DMA_CFG_L	((0<<31)\
										|(0<<30)\
										|(0<<17)\
										|(0<<8))

#define DMA_MEMCPY_DMA_CFG_H_LLI	((0<<12)\
										|(0<<6) /*SS_UPD_EN */ \
										|(0<<5) /*DS_UPD_EN */\
										|(1<<2)\
										|(1<<1))

/********************************************************************************
int dma_m2m_copy (unsigned char ch, unsigned char *src_buf, unsigned char *dst_buf, unsigned int len)
*   len  byte数据长度
*   src_buf  源数据  
*   dst_buf  目标数据
*   每次传输数据最大 为4096 个 byte/word （与 DMA_WIDTH_32 配置有关 ）
********************************************************************************/

int dma_m2m_copy (unsigned char DmaCh, unsigned char *src_buf, unsigned char *dst_buf, unsigned int len)
{
	dma_cfg_channel(DmaCh, 
					(UINT32)src_buf,
					(UINT32)dst_buf,
					0,
					M2M_COPY_DMA_CTL_L,
					M2M_COPY_DMA_CTL_H|(len>>1),
					M2M_COPY_DMA_CFG_L|(DmaCh<<5), 
					(M2M_COPY_DMA_CFG_H),
					0,
					0);
	
	dma_start_channel(DmaCh);
	
	while(!dma_transfer_over(DmaCh))
	{
	}
	
	//printf ("end of dma_start_channel\n");

	//dma_clr_int (DmaCh);
	
	while(rDMA_CHEN_L & (1<<DmaCh))
	{
		
	}
	
	//printf ("wait for DMA stop\n");
	
	//dma_release_channel(DmaCh);

	return 0;	
}



#ifdef _DMA_MEMCPY_USE_LLI_DMA_

static void dma_memcpy_transfer_over_IRQHandler_lli (UINT32 ulIRQFactors, UINT32 channel, void *channel_private_data)
{
   int ret = 0;
	if(ulIRQFactors & ERR_INT_BIT)
	{
		// This interrupt is generated when an ERROR response is received from an AHB slave on the
		// HRESP bus during a DMA transfer. 
		// In addition, the DMA transfer is cancelled and the channel is disabled.
		// DMA 异常
		printf ("dma_memcpy ERR_INT_BIT\n");
		ret = 1;
	}
	else
	{
		//DEBUG_MSG ("dma_memcpy dma end\n");
	}
	dma_clr_int(channel);

	// DMA结束
	*(int *)channel_private_data = ret;
	// 发送DMA传输结束事件
	Dma_TfrOver_PostSem (channel);
}
#endif


void dma_memcpy (unsigned char *dst_buf, unsigned char *src_buf, unsigned int len)
{
#if 1
	// CPU的传输速率高于DMA，CPU 516MHz， AHB 156MHz, 使用CPU模式
	memcpy (dst_buf, src_buf, len);
#else
	unsigned char *d, *s;
	unsigned int size;
	if( (unsigned int)dst_buf & 1 || (unsigned int)src_buf & 1 || len & 1 || len < 4096)
	{
mem_copy:
		memcpy (dst_buf, src_buf, len);
	}
	else
	{
#ifdef STATIC_DMA_MEMCPY_CHANNEL		
		OS_Use (&dma_memcpy_access_sema);
		unsigned int dma_ch = dma_memcpy_dma_channel;	//dma_request_dma_channel (DMA_MEMCPY_CHANNEL);
#else
		unsigned int dma_ch = dma_request_dma_channel (DMA_MEMCPY_CHANNEL);
#endif
		int dma_ret = 0;
		int bit32 = 1;		// 32bit or 16bit
		if(dma_ch == ALL_CHANNEL_RUNNING)
			goto mem_copy;
		
		if( ((unsigned int)dst_buf & 3) || ((unsigned int)src_buf & 3) || (len & 3) )
			bit32 = 0;		// 16bit width
		
#ifdef _DMA_MEMCPY_USE_LLI_DMA_
		struct dw_lli *dma_lli = NULL;
		int lli_count = (len + DMA_MEMCPY_MAX_BATCH_TRANS_PAGE*512 - 1) / (DMA_MEMCPY_MAX_BATCH_TRANS_PAGE*512);
#ifdef STATIC_DMA_MEMCPY_CHANNEL		
		dma_lli = dma_memcpy_dma_lli;
#else
		dma_lli = (struct dw_lli *)kernel_malloc(sizeof(struct dw_lli) * lli_count);		
#endif
   	register_channel_IRQHandler (dma_ch, 
										  dma_memcpy_transfer_over_IRQHandler_lli, 
										  &dma_ret,
										  //(1<< BLOCK_INT_BIT)|(1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  (1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  );
	
		int i;
		UINT32 left = len;
		dma_addr_t dst_addr = (dma_addr_t)dst_buf;
		dma_addr_t sar_addr = (dma_addr_t)src_buf;
		struct dw_lli *	lli = dma_lli;
		// 创建DMA链表
		for (i = 0; i < lli_count; i++)
		{
			lli->sar = sar_addr;
			lli->dar = dst_addr;
			if(i != (lli_count - 1))
				lli->llp = (UINT32)&dma_lli[i + 1];
			else
				lli->llp = 0;
			
			if(bit32)
				lli->ctllo = DMA_MEMCPY_CTL_L_LLI_32 | (1 << 27);
			else
				lli->ctllo = DMA_MEMCPY_CTL_L_LLI_16 | (1 << 27);
			
			if(left >= DMA_MEMCPY_MAX_BATCH_TRANS_PAGE*512)
				lli->ctlhi = DMA_MEMCPY_DMA_CTL_H | (DMA_MEMCPY_MAX_BATCH_TRANS_PAGE*512 / (2 << bit32) );
			else
				lli->ctlhi = DMA_MEMCPY_DMA_CTL_H | (left / (2 << bit32) );
			lli->sstat = 0;
			lli->dstat = 0;
			dst_addr += DMA_MEMCPY_MAX_BATCH_TRANS_PAGE*512;
			sar_addr += DMA_MEMCPY_MAX_BATCH_TRANS_PAGE*512;
			left -= DMA_MEMCPY_MAX_BATCH_TRANS_PAGE*512;
			lli ++;
		}
			
		dma_clr_int(dma_ch);
		dma_cfg_channel(dma_ch, 
						dma_lli[0].sar,
						dma_lli[0].dar,
						(UINT32)(&dma_lli[0])&0xfffffffc,
						dma_lli[0].ctllo,
						dma_lli[0].ctlhi,
						DMA_MEMCPY_DMA_CFG_L|(dma_ch<<5), (DMA_MEMCPY_DMA_CFG_H_LLI),
						0,
						0);	
		dma_flush_range ((UINT32)&dma_lli[0], (UINT32)&dma_lli[lli_count]);
		dma_inv_range   ((unsigned int)dst_buf, len + (unsigned int)dst_buf);
		dma_flush_range ((unsigned int)src_buf, len + (unsigned int)src_buf);
		
		dma_start_channel(dma_ch);
		Dma_TfrOver_PendSemEx (dma_ch, 200);			
		dma_clr_int(dma_ch);
#ifdef STATIC_DMA_MEMCPY_CHANNEL		
#else
		kernel_free (dma_lli);
#endif
		dma_inv_range   ((unsigned int)dst_buf, len + (unsigned int)dst_buf);

#else
		d = dst_buf;
		s = src_buf;
		size = len;
		dma_inv_range ((unsigned int)dst_buf, len + (unsigned int)dst_buf);
		dma_flush_range ((unsigned int)src_buf, len + (unsigned int)src_buf);
		while(size > 0)
		{
			unsigned int to_trans = size;
			if(to_trans > DMA_MEMCPY_MAX_BATCH_TRANS_PAGE*512)
				to_trans = DMA_MEMCPY_MAX_BATCH_TRANS_PAGE*512;
			dma_m2m_copy (dma_ch, s, d, to_trans);
			s += to_trans;
			d += to_trans;
			size -= to_trans;
		}
		dma_inv_range ((unsigned int)dst_buf, len + (unsigned int)dst_buf);
#endif		
		
#ifdef STATIC_DMA_MEMCPY_CHANNEL		
		OS_Unuse (&dma_memcpy_access_sema);
#else
		// 动态分配
		dma_release_channel(dma_ch);
#endif
	}
#endif
}



#define DMA_SCATTER_COPY_DMA_CTL_L 	(  0\
	/* src Block chaining disable */	| (0<<28)\
	/* dst Block chaining disable */	| (0<<27)\
	/* 使用 AHB master 2 */				| (1<<25)\
	/* 使用 AHB master 2 */				| (1<<23)\
												| (M2M_DMAC<<20)\
	/* dst Scatter enabled */			| (1<<18)\
	/* src Scatter enabled */			| (1<<17)\
												| (DMA_BURST_SIZE_32   << 14) \
												| (DMA_BURST_SIZE_32   << 11) \
	/* SINC, Increment */				| (0<<9) \
	/* DINC, Increment */				| (0<<7) \
												| (DMA_WIDTH_32 << 4) \
												| (DMA_WIDTH_32 << 1) \
	/*关闭中断*/							| (0<<0))

#define DMA_SCATTER_COPY_DMA_CTL_H 	(0<<12)

#define DMA_SCATTER_COPY_DMA_CFG_L	((0<<31)\
												|(0<<30)\
												|(0<<17)\
												|(0<<8))

#define DMA_SCATTER_COPY_DMA_CFG_H	((0<<12)\
												|(0<<6) /*SS_UPD_EN */ \
												|(0<<5) /*DS_UPD_EN */\
												|(1<<2)\
												|(1<<1))

#include <math.h>

// burst_len为8， 16， 32， 64， 128， 256， 512， 1024
void xm_dma_scatter_copy (unsigned char *src_addr, 	// 源地址
								  unsigned char *dst_addr, 	// 目标地址
								  int src_stride, 				// 源地址每次burst传输后地址增量(可以为负值)，必须为4的倍数, 且不小于burst_len
								  int dst_stride,					// 目标地址每次burst传输后地址增量(可以为负值)，必须为4的倍数, 且不小于burst_len
								  int burst_len, 					// 每次Burst传输的字节长度, 为8， 16， 32， 64， 128， 256， 512， 1024
								  int total_len					// 传输的总字节长度
								)
{
	unsigned int ctrl_lo;
	//unsigned int ctrl_hi;
	unsigned int sgrx_lo, dsrx_lo;
	unsigned int src, dst;
	unsigned int beat_len;

	src = (unsigned int)src_addr;
	dst = (unsigned int)dst_addr;
	OS_Use (&scatter_dma_memcpy_access_sema);

	ctrl_lo = DMA_SCATTER_COPY_DMA_CTL_L;
	//ctrl_hi = DMA_SCATTER_COPY_DMA_CTL_H;
	if(src_stride < 0)
	{
		/* SINC, Decrement */	
		ctrl_lo |= (1 << 9);
	}
	if(dst_stride < 0)
	{
		/* DINC, Decrement */	
		ctrl_lo |= (1 << 7);
	}
	sgrx_lo = ((burst_len/4) << 20) | (abs(src_stride) - burst_len)/4;
	dsrx_lo = ((burst_len/4) << 20) | (abs(dst_stride) - burst_len)/4;
	rDMA_CTLx_L(0) = ctrl_lo;
	rDMA_CFGx_L(0) = DMA_SCATTER_COPY_DMA_CFG_L | (0<<5);
	rDMA_CFGx_H(0) = DMA_SCATTER_COPY_DMA_CFG_H;
	rDMA_SGRx_L(0) = sgrx_lo;
	rDMA_DSRx_L(0) = dsrx_lo;
	while(total_len > 0)
	{
		rDMA_SARx_L(0) = (dma_addr_t)src;
		rDMA_DARx_L(0) = (dma_addr_t)dst;
		//rDMA_CTLx_L(0) = ctrl_lo;
		if(total_len >= 2048)
		{
			beat_len = 2048;
			//rDMA_CTLx_H(0) = DMA_SCATTER_COPY_DMA_CTL_H | (2048 / 4);
			//total_len -= 2048;
		}
		else
		{
			beat_len = total_len;
			//rDMA_CTLx_H(0) = DMA_SCATTER_COPY_DMA_CTL_H | (total_len / 4);
			//total_len = 0;
		}
		rDMA_CTLx_H(0) = DMA_SCATTER_COPY_DMA_CTL_H | (beat_len / 4);
		//rDMA_CFGx_L(0) = DMA_SCATTER_COPY_DMA_CFG_L|(0<<5);
		//rDMA_CFGx_H(0) = DMA_SCATTER_COPY_DMA_CFG_H;
		//rDMA_SGRx_L(0) = sgrx_lo;
		//rDMA_DSRx_L(0) = dsrx_lo;
		rDMA_CHEN_L = ((1<<(8+0))|(1<<0));
		// 等待DMA传输完成
		while(!(rDMA_RAW_TRF_L & (1<<0)))
		{
		}
		total_len -= beat_len;
		src += src_stride * (2048/burst_len);
		dst += dst_stride * (2048/burst_len);
		// 等待DMA通道关闭，所有数据已从DMA FIFO刷新到DRAM/SRAM
		while(rDMA_CHEN_L & (1<<0))
		{
		}
		
	}
	
	OS_Unuse (&scatter_dma_memcpy_access_sema);
}

#if SCATTER_DMA_TEST
void load_block_param_vert_reverse (unsigned char *src, unsigned char *dst, unsigned int stride);

void scatter_dma_memcpy_test (void)
{
	unsigned int *test_data;
	unsigned int test_size = 16384;
	unsigned int *iram_data = (unsigned int *)0x300000;
	
	test_data = (unsigned int *)kernel_malloc (test_size);
	if(test_data)
	{
		for (int i = 0; i < test_size/4; i ++)
		{
			test_data[i] = i;
		}
		dma_flush_range ((unsigned int)test_data, test_size + (unsigned int)test_data);
		memset ((char *)iram_data, 0, 32768);
		dma_flush_range ((unsigned int)iram_data, test_size + (unsigned int)iram_data);
		XMINT64 t1 = XM_GetHighResolutionTickCount ();
		xm_dma_scatter_copy ((unsigned char *)test_data, 
								(unsigned char *)iram_data,
								128,
								256,
								128,
								128*128);
		t1 = XM_GetHighResolutionTickCount () - t1;
		XM_printf ("dma=%d\n", (unsigned int)t1);
		t1 = XM_GetHighResolutionTickCount ();			  
		load_block_param_vert_reverse ((unsigned char *)test_data, (unsigned char *)iram_data, 256);
		t1 = XM_GetHighResolutionTickCount () - t1;
		XM_printf ("cpu=%d\n", (unsigned int)t1);
		
		
		memset ((char *)iram_data, 0, 32768);
		dma_flush_range ((unsigned int)iram_data, test_size + (unsigned int)iram_data);
		xm_dma_scatter_copy ((unsigned char *)test_data, 
								(unsigned char *)iram_data,
								8,
								16,
								8,
								8*128);	
									
		kernel_free (test_data);
	}
	
	test_data = (unsigned int *)kernel_malloc (0x40000);
	if(test_data)
	{
		XMINT64 t1 = XM_GetHighResolutionTickCount ();
		dma_memcpy ((unsigned char *)test_data, (unsigned char *)0x80000000, 0x40000);
		t1 = XM_GetHighResolutionTickCount () - t1;
		XM_printf ("dma=%d\n", (unsigned int)t1);
		t1 = XM_GetHighResolutionTickCount ();
		memcpy ((unsigned char *)test_data, (unsigned char *)0x80000000, 0x40000);
		t1 = XM_GetHighResolutionTickCount () - t1;		
		XM_printf ("cpu=%d\n", (unsigned int)t1);
		kernel_free (test_data);
	}
}
#endif	// SCATTER_DMA_TEST
