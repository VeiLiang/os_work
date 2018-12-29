#ifndef __DMA_H
#define __DMA_H

#include "types.h"
#include "common.h"
#include "ark1960.h"

/* LLI == Linked List Item; a.k.a. DMA block descriptor */
struct dw_lli {
	/* values that are not changed by hardware */
	dma_addr_t	sar;
//	u32_t			rev1;
	dma_addr_t	dar;
//	u32_t			rev2;
	dma_addr_t	llp;		/* chain to next lli */
//	u32_t			rev3;
	u32		ctllo;
	/* values that may get written back: */
	u32		ctlhi;
	/* sstat and dstat can snapshot peripheral register state.
	 * silicon config may discard either or both...
	 */
	u32		sstat;
//	u32_t		rev4;
	u32		dstat;
//	u32_t		rev5;
	u32		rev;
};

#define REQ_I2S_RX					I2S_RX_FIFO_DMACBREQ
#define REQ_I2S_TX					I2S_TX_FIFO_DMACBREQ


#define CHANNEL_STATE_IDLE			0
#define CHANNEL_STATE_RUNNING		1
#define ALL_CHANNEL_RUNNING		0xFFFFFFFF

#define BLOCK_INT_BIT				4
#define DST_INT_BIT					3
#define ERR_INT_BIT					2
#define SRC_INT_BIT					1
#define TFR_INT_BIT					0

#define DMA_WIDTH_8				0
#define DMA_WIDTH_16				1
#define DMA_WIDTH_32				2
#define DMA_WIDTH_64				3
#define DMA_WIDTH_128				4
#define DMA_WIDTH_256				5

#define DMA_BURST_SIZE_1			0
#define DMA_BURST_SIZE_4			1
#define DMA_BURST_SIZE_8			2
#define DMA_BURST_SIZE_16			3
#define DMA_BURST_SIZE_32			4
#define DMA_BURST_SIZE_64			5
#define DMA_BURST_SIZE_128			6
#define DMA_BURST_SIZE_256			7

#define M2M_DMAC 				 	0
#define M2P_DMAC 					1
#define P2M_DMAC					2
#define P2P_DMAC					3
#define P2M_PERI					4
#define P2P_SRC_PERI				5
#define M2P_PERI 					6
#define P2P_DST_PERI				7

typedef struct 
{
	UINT32 srcAddr;
	UINT32 desAddr;
	UINT32 nextDesc;
	UINT32 control_l;
	UINT32 control_h;
	UINT32 src_status;
	UINT32 dst_status;
}DmaDescriptor;


					

void dma_clr_trans(UINT8 channel);

void dma_clr_block(UINT8 channel);

void dma_clr_dst_trans(UINT8 channel);

void dma_clr_src_trans(UINT8 channel);

void dma_clr_err(UINT8 channel);

#define dma_clr_trans(channel) 				\
{														\
	rDMA_CLEAR_TRF_L = (1<<channel);			\
}

#define dma_clr_block(channel)				\
{														\
	rDMA_CLEAR_BLOCK_L = (1<<channel);		\
}

#define dma_clr_dst_trans(channel)			\
{														\
	rDMA_CLEAR_DST_TRAN_L = (1<<channel);	\
}

#define dma_clr_src_trans(channel)			\
{														\
	rDMA_CLEAR_SRC_TRAN_L = (1<<channel);	\
}

#define dma_clr_err(channel)					\
{														\
	rDMA_CLEAR_ERR_L = (1<<channel);			\
}


void dma_channel_status_init(void);

void dma_clr_int(UINT8 channel);

void dma_detect_ch_disable(UINT8 channel);

UINT32 dma_request_dma_channel(UINT32 nFavoriteChannel);

void dma_release_channel(UINT32 nChannel);

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
	);

void dma_init(void);

//void register_changle_IRQHandler(UINT32 channel, void (*pfnIRQHandler)(UINT32 ulIRQFactors), UINT32 irq_mask);

void register_changle_IRQHandler(UINT32 channel, 
	void (*pfnIRQHandler)(UINT32 ulIRQFactors, UINT32 channel, void *channel_private_data), 
   void *channel_private_data,
	UINT32 irq_mask);

void dma_stop_channel(UINT32 nChannel);

void dma_start_channel(UINT32 nChannel);
#define dma_start_channel(nChannel)								\
{																			\
	if(nChannel < MAX_DMA_CH_NUM)									\
	{																		\
		rDMA_CHEN_L = ((1<<(8+nChannel))|(1<<nChannel));	\
	}																		\
}



void dma_int_clr(UINT32 ch);// removed waring

// 专用于SDMMC的DMA通道号
#define	SD_DMA_CHANNEL			0
// 专用于语音播放的DMA通道号	
#define	VOICE_DMA_CHANNEL		1
// 专用于MIC播放的DMA通道号	
#define	ADC_DMA_CHANNEL		2
// 专用于USB的DMA通道号	
#define	USB_DMA_CHANNEL		3
// MIC/LineIn ADC通道2 4
#define	ADC1_DMA_CHANNEL		3
//

#define SSI_DMA_READ_CHANNL  		1
#define SSI_DMA_WRITE_CHANNL  		0


#define	DMA_CANNEL_COUNT		8
#define MAX_DMA_CH_NUM   			DMA_CANNEL_COUNT


#define DMA_BLOCK_SIZE 		512//UNIT is byte!!

/*single block transfer*/
#define DMA_M2M_CTL_L_SINGLE 	((0<<28)\
								|(0<<27)\
								|(0<<25)\
								|(0<<23)\
								|(0<<20)\
								|(0<<18)\
								|(0<<17)\
								|(DMA_BURST_SIZE_16<<14)\
								|(DMA_BURST_SIZE_16<<11)\
								|(0<<9)\
								|(0<<7)\
								|(DMA_WIDTH_8<<4)\
								|(DMA_WIDTH_8<<1)\
								|(1<<0))

#define DMA_M2M_CFG_L_SINGLE		((0<<31)\
									|(0<<30)\
									|(1<<17)\
									|(0<<8))

#define DMA_M2M_CFG_H_SINGLE	((1<<6)\
								|(1<<5)\
								|(1<<1))	


/*MUITL block transfer USE LLI*/

#define DMA_M2M_CTL_L_MULTI	((1<<28)\
								|(1<<27)\
								|(0<<25)\
								|(0<<23)\
								|(M2M_DMAC<<20)\
								|(0<<18)\
								|(0<<17)\
								|(DMA_BURST_SIZE_16<<14)\
								|(DMA_BURST_SIZE_16<<11)\
								|(0<<9)\
								|(0<<7)\
								|(DMA_WIDTH_8<<4)\
								|(DMA_WIDTH_8<<1)\
								|(1<<0))

#define DMA_M2M_CTL_L_MULTI_LAST	((0<<28)\
								|(0<<27)\
								|(0<<25)\
								|(0<<23)\
								|(0<<20)\
								|(0<<18)\
								|(0<<17)\
								|(DMA_BURST_SIZE_16<<14)\
								|(DMA_BURST_SIZE_16<<11)\
								|(0<<9)\
								|(0<<7)\
								|(DMA_WIDTH_32<<4)\
								|(DMA_WIDTH_32<<1)\
								|(1<<0))


#define DMA_M2M_CFG_L_MULTI 	((0<<31)\
								|(0<<30)\
								|(0<<17)\
								|(0<<8))


#define DMA_M2M_CFG_H_MULTI	((1<<6)\
								|(1<<5)\
								|(1<<1))	

						
#define DMA_M2M_CTL_H ((0<<12)\
						|(DMA_BLOCK_SIZE<<0))


#define DMA_M2M_CFG_L	((0<<31)\
						|(0<<30)\
						|(1<<17)\
						|(0<<8))


#define DMA_M2M_CFG_H	((1<<6)\
						|(1<<5)\
						|(1<<1))	


UINT32 dma_transfer_over(UINT8 channel);

void Dma_TfrOver_PostSem(UINT32 ch);
void Dma_TfrOver_PendSem(UINT32 ch);
int Dma_TfrOver_PendSemEx(UINT32 ch, unsigned int timeout_ms);


#define DMA_TIMEOUT 10000000

#define DMA_MEMORY			0
#define DMA_PERIPHERAL		1

#define DMA_NUM_MASTER_INT	2
#define DMA_NUM_CHANNELS		4
#define DMA_NUM_HS_INT			16
#define DMA_MAX_BLK_SIZE		4088         //real max blk size is 4095,choose 4088 for allign


#define DMA_MSIZE1		0
#define DMA_MSIZE4		1
#define DMA_MSIZE8		2
#define DMA_MSIZE16		3
#define DMA_MSIZE32		4
#define DMA_MSIZE64		5
#define DMA_MSIZE128	6
#define DMA_MSIZE256	7

#define DMA_TR_WIDTH8		0
#define DMA_TR_WIDTH16		1
#define DMA_TR_WIDTH32		2
#define DMA_TR_WIDTH64		3

#define DMA_TTFC_M2M_DW				0
#define DMA_TTFC_M2P_DW				1
#define DMA_TTFC_P2M_DW				2
#define DMA_TTFC_P2P_DW				3
#define DMA_TTFC_P2M_PERI				4
#define DMA_TTFC_P2P_SRCPERI			5
#define DMA_TTFC_M2P_PERI				6
#define DMA_TTFC_P2P_DSTPERI			7

typedef	struct{
	UINT32 blkTS;
	UINT8 llpSrcEn;
	UINT8 llpDstEn;
	UINT8 sms;
	UINT8 dms;
	UINT8 ttFC;
	UINT8 dstScatterEn;
	UINT8 srcGatherEn;
	UINT8 srcMSize;
	UINT8 dstMSize;
	UINT8 sinc;
	UINT8 dinc;
	UINT8 srcTrWidth;
	UINT8 dstTrWidth;
	UINT8 intEn;

}DMA_ctlReg_Para;

typedef	struct{
	UINT8 dstPer;
	UINT8 srcPer;
	UINT8 ssUpdEn;
	UINT8 dsUpdEn;
	UINT8 protCtl;
	UINT8 fifoMode;
	UINT8 fcMode;
	UINT8 reloadDst;
	UINT8 reloadSrc;
	UINT8 maxAbrst;
	UINT8 srcHsPol;
	UINT8 dstHsPol;
	UINT8 lockB;
	UINT8 lockCh;
	UINT8 lockBL;
	UINT8 lockChL;
	UINT8 hsSelSrc;
	UINT8 hsSelDst;
	UINT8 chSusp;
	UINT8 chPrior;
}DMA_cfgReg_Para;

typedef	struct{
	UINT32 enable;
	UINT32 count;
	UINT32 interval;	
}DMA_Gather_Para;

typedef	struct{
	UINT32 enable;
	UINT32 count;
	UINT32 interval;
}DMA_Scatter_Para;

typedef struct
{
	UINT32 addr;
	UINT32 count;
	UINT32 interval;	
	UINT8 devType;
	UINT8 trWidth;
	UINT8 mSize;
	UINT8 inc;
	UINT8 per;
}dmaTfrPortInfo;

int sddma_p2m_multi_block_lli (unsigned char  channel,
										unsigned int address, unsigned int length);

int mmcsd_write_data_dma (unsigned char Unit, char *buf, unsigned int len);

int mmcsd_read_data_dma  (unsigned char Unit, char *buf, unsigned int len);


void mmcsd_dma_init (void);
void mmcsd_dma_exit (void);


void VOICE_DMA_INT_handler (void);
void ADC_DMA_INT_handler (void);
void ADC1_DMA_INT_handler (void);

INT32 M2MDmaInit(void);

// 使用DMA，将源地址(src_addr)的内容拷贝到目标地址(dst_addr), 复制的字节长度为bytes_to_copy
// dma_channel		指定DMA通道号
// src_addr			源地址，4字节对齐
// dst_addr			目标地址，4字节对齐
// bytes_to_copy	复制的字节长度, 4的倍数
int dma_m2m_copy (unsigned char dma_channel, 
						unsigned char *src_addr, unsigned char *dst_addr, unsigned int bytes_to_copy);

INT32 M2MDmaMemCpy(UINT32 dstaddr, UINT32 srcaddr, UINT32 numbytes);

#endif
