
/***********************************************************************
*Copyright (c)2012  Arkmicro Technologies Inc. 
*All Rights Reserved 
*
*Filename:    SSI.c
*Version :    1.0 
*Date    :    2010.10.07
*Author  :    gongmingli
*Abstract:     
 *	此版本为测试9月30号的FPGA1660SOF 上面的SPI 选折
 *    的管脚为select input->spirxd_1, spics1
 *    相应的管脚应在rAHB_SYS_REG25 中配置
*History :     
* 
*Version :    2.0 
*Date    :    2012.02.27
*Author  :     
*Abstract:    ark1960  Ver1.0 MPW driver remove waring
*History :    1.0

************************************************************************/
#include "hardware.h"  
#include "printk.h"    
#include "uart.h"      
#include "ssi.h"
#include "string.h" // add for remove waring
#include "cpuclkcounter.h"
#include "clock.h"
#include "xm_core.h"

#if 1
#define SPI_READ_FAST	1
#if  SPI_READ_FAST
#define   SPI_READ_FAST_DUAL   0   
#endif 
#endif

	
//#define SPI_WRITE_DMA  
//#define SPI_READ_DMA   
//#define SPI_WRITEZERO_DMA  //用于在读取数据时候，快速填充0，相当于给flash发时钟信号


/******************************************************************************
该宏定义 
仅作为 写 0 的操作使用 (满足读取时序的要求，即提供时钟 )
读取数据时候 ，需要提供时钟，以使得数据读取出来 
********************************************************************************/
#define SSI_WRITE_ZERO_DMA_CTL_L ((0<<28)\
									|(0<<27)\
									|(0<<25)\
									|(0<<23)\
									|(M2P_DMAC<<20)\
									|(0<<18)\
									|(0<<17)\
									|(DMA_BURST_SIZE_16<<14)\
									|(DMA_BURST_SIZE_16<<11)\
/*SINC 1x=No change*/		|(2<<9)\
/*DINC 1x=No change*/		|(2<<7)\
									|(DMA_WIDTH_32<<4)\
									|(DMA_WIDTH_32<<1)\
									|(1<<0))


#define SSI_WRITE_DMA_CTL_L 		((0<<28)\
									|(0<<27)\
									|(0<<25)\
									|(0<<23)\
									|(M2P_DMAC<<20)\
									|(0<<18)\
									|(0<<17)\
									|(DMA_BURST_SIZE_16<<14)\
									|(DMA_BURST_SIZE_16<<11)\
/*SINC 00=Increment*/		|(0<<9)\
/*DINC 1x=No change*/		|(2<<7)\
									|(DMA_WIDTH_32<<4)\
									|(DMA_WIDTH_32<<1)\
									|(1<<0))


#define SSI_WRITE_DMA_CTL_H 		(0<<12)

#define SSI_WRITE_DMA_CFG_L		((0<<31)\
									|(0<<30)\
									|(0<<17)\
									|(0<<8))

#define SSI_WRITE_DMA_CFG_H		((REQ_SSI_TX<<12)\
									   |(0<<7)\
									   |(1<<6)\
								   	|(1<<5)\
/*PROTCTL*/					     	|(1<<2)\
									   |(1<<1))

#define SSI_READ_DMA_CTL_L 		((0<<28)\
/*LLP_DST_EN*/			         |(0<<27)\
/*Source Master Select.*/     |(0<<25)\
/*Destination Master Select.*/|(0<<23)\
/*Transfer Type and Flow Control*/|(P2M_DMAC<<20)\
/*DST_SCATTER_EN*/				|(0<<18)\
/*SRC_GATHER_EN*/					|(0<<17)\
									   |(DMA_BURST_SIZE_16<<14)\
									   |(DMA_BURST_SIZE_16<<11)\
/*SINC 1x=No change*/			|(2<<9)\
/*DINC 00=Increment*/			|(0<<7)\
									   |(DMA_WIDTH_32<<4)\
									   |(DMA_WIDTH_32<<1)\
									   |(1<<0))
								
#define SSI_READ_DMA_CTL_H 		(0<<12)


#define SSI_READ_DMA_CFG_L			((0<<31)\
/*Automatic Source Reload.*/	|(0<<30)\
/*Bus Lock Bit.*/					|(0<<17)\
/*Channel Suspend.Bit*/			|(0<<8))


#define SSI_READ_DMA_CFG_H		((0<<12)\
/*SRC_PER*/							|(REQ_SSI_RX<<7)\
									   |(1<<6)\
									   |(1<<5)\
/*PROTCTL*/					     	|(1<<2)\
/*FIFO Mode Select*/				|(1<<1))

/*******************************************************************************
*   Spi-Flash Command
********************************************************************************/
#define SPI_WRITE_ENABLE        0x06
#define SPI_WRITE_DISABLE       0x04
#define SPI_READ_STATUS         0x05
#define SPI_WRITE_STATUS        0x01
#define SPI_READ_DATA           0x03
#define SPI_FAST_READ           0x0B
#define SPI_FAST_READ_DUAL      0x3B
#define SPI_PAGE_PROGRAM        0x02
#define SPI_SECTOR_ERASE        0x20
#define SPI_SECTOR_ERASE_1      0xD7
#define SPI_BLOCK_ERASE         0xD8
#define SPI_BLOCK_ERASE_1       0x52
#define SPI_CHIP_ERASE          0xC7
#define SPI_CHIP_ERASE_1        0x60
#define SPI_POWER_DOWN          0xB9
#define SPI_READ_JEDEC_ID       0x9F
#define SPI_READ_ID_1           0xAB
#define SPI_MF_DEVICE_ID        0x90
#define SPI_MF_DEVICE_ID_1      0x15
#define SPI_READ_ELECTRON_SIGN  0xAB

#define SPI_MF_WINBOND          0xEF
#define SPI_MF_EON              0x1C
#define SPI_MF_AMIC             0x37
#define SPI_MF_ATMEL            0x1F
#define SPI_MF_SST              0xBF
#define SPI_MF_MXIC             0xC2

/*******************************************************************************
*   INT-Mask   Or  Status check
********************************************************************************/
#define SPI_RXFIFO_ENABLE 	 (1<<8)  // not use in int-mask
#define SPI_TRANS_COMPLETE  (1<<7)  // int 1.
#define SPI_RXFIFO_OVER 	 (1<<6)  // int 2.
#define SPI_RXFIFO_FULL 	 (1<<5)  // int 3.  
#define SPI_RXFIFO_REQ 		 (1<<4)  // int 4.
#define SPI_RXFIFO_NOTEMPTY (1<<3)  // int 5.  x -- At least 1 word in RXFIFO 
#define SPI_TXFIFO_FULL 	 (1<<2)  // int 6.
#define SPI_TXFIFO_REQ 		 (1<<1)  // int 7.
#define SPI_TXFIFO_EMPTY 	 (1<<0)  // int 8.


#define SPIFLASH_BUSY    (1<<0)
#define SPIFLASH_WRITEENABLE (1<<1)

//
//  

#define WORDSPERPAGE		   64
#define BYTESPERPAGE 		256
#define PAGESPERSECTORS 	16
#define SECTORSPERBLOCK 	16
#define BLOCKSPERFLASH 		128
#define BYTESPERBLOCK		(BYTESPERPAGE*PAGESPERSECTORS*SECTORSPERBLOCK)
#define BYTESPERSECTOR		(BYTESPERPAGE*PAGESPERSECTORS)


#define SPI_RXFIFO_DEPTH	64
#define SPI_TXFIFO_DEPTH	64




const tSPI_Mf_ID Spi_Mf_ID[] =
{
	{SPI_MF_WINBOND, "Winbond"},
	{SPI_MF_EON, "Eon"},
	{SPI_MF_AMIC, "Amic"},
	{SPI_MF_ATMEL, "Atmel"},
	{SPI_MF_SST, "SST"},
	{SPI_MF_MXIC, "Macronix"},
	{0x0, "Unknown"}
};

const tSPI_DevInfo Spi_DevInfo[] =
{
//   Manufacturer   Type Capa DevID  Chip   Sector(Bot) Block(Top) Part
	{SPI_MF_WINBOND,0x30,0x13,0x12,0x080000,0x00001000,0x00010000, "W25X40"},
	{SPI_MF_WINBOND,0x30,0x14,0x13,0x100000,0x00001000,0x00010000, "W25X80"},
	{SPI_MF_WINBOND,0x30,0x15,0x14,0x200000,0x00001000,0x00010000, "W25X16"},
	{SPI_MF_WINBOND,0x30,0x16,0x15,0x400000,0x00001000,0x00010000, "W25X32"},
	{SPI_MF_WINBOND,0x40,0x16,0x15,0x400000,0x00001000,0x00010000, "W25X32"},
	{SPI_MF_WINBOND,0x40,0x17,0x16,0x800000,0x00001000,0x00010000, "W25X64"},
	{SPI_MF_EON,    0x31,0x14,0x13,0x100000,0x00001000,0x00010000, "EN25F80"},
	{SPI_MF_EON,    0x31,0x15,0x14,0x200000,0x00001000,0x00010000, "EN25F16"},
	{SPI_MF_EON,    0x31,0x16,0x15,0x400000,0x00001000,0x00010000, "EN25F32"},
	{SPI_MF_EON,    0x20,0x14,0x33,0x100000,0x80001000,0x80010000, "EN25B80"},
	{SPI_MF_EON,    0x20,0x15,0x34,0x200000,0x80001000,0x80010000, "EN25B16"},
	{SPI_MF_EON,    0x20,0x16,0x35,0x400000,0x80001000,0x80010000, "EN25B32"},
	{SPI_MF_EON,    0x51,0x14,0x13,0x100000,0x00001000,0x00010000, "EN25T80"},
	{SPI_MF_EON,    0x51,0x15,0x14,0x200000,0x00001000,0x00010000, "EN25T16"},
	{SPI_MF_MXIC,   0x20,0x14,0x13,0x100000,0x00001000,0x00010000, "KH25L80"},
	{SPI_MF_MXIC,   0x20,0x15,0x14,0x200000,0x00001000,0x00010000, "KH25L16"},
	{0x0,           0x00,0x00,0x00,0x400000,0x00001000,0x00010000, "XX25X80"},		// 缺省2MB
};

static unsigned int ulChip;
static tSPI_DevInfo const *pSpiDev;
static unsigned int nMinSectorSize;
static unsigned int nMaxSectorSize;

//static unsigned int nTopBlockAddr;  // remove waring

#include "rtos.h"
static OS_RSEMA SpiOpt_Sem;	// DMA互斥信号量

/*******************************************************************************
*正常 
*     SpiDmaRead  能正常读取
*
* 参数       *buf    数据缓存区域
*            burst_length         bit数
********************************************************************************/
void SpiDmaRead(UINT32 *buf , UINT32 burst_length )
{
	UINT8 SPIDmaCh = dma_request_dma_channel( SSI_DMA_READ_CHANNL );
	rSPI_DMAREG |= (1<<23) ;

	dma_cfg_channel(SPIDmaCh,
						 rSPI_RXFIFO,(UINT32)buf,
						 0,
						 SSI_READ_DMA_CTL_L,SSI_READ_DMA_CTL_H|( burst_length>>5 ),
						 SSI_READ_DMA_CFG_L|SPIDmaCh,SSI_READ_DMA_CFG_H,
						 0,0);
	
	rDMA_CHEN_L|= ((1<<(8+SPIDmaCh))|(1<<SPIDmaCh));
	
	while(!dma_transfer_over(SPIDmaCh));		
	dma_clr_int(SPIDmaCh);
	dma_detect_ch_disable(SPIDmaCh);
	dma_release_channel(SPIDmaCh);	
	rSPI_DMAREG &= ~(1<<23);
}

/*******************************************************************************
*正常
*     SpiDmaWrite  能正常写
*
* 参数       *buf    数据缓存区域
*            burst_length         数据bit数
********************************************************************************/
void SpiDmaWrite(UINT32 *buf , UINT32 burst_length )
{
	UINT8 SPIDmaCh = dma_request_dma_channel( SSI_DMA_WRITE_CHANNL );
	
	dma_cfg_channel(SPIDmaCh,
						 (UINT32)buf,rSPI_TXFIFO,
						 0,
						 SSI_WRITE_DMA_CTL_L,SSI_WRITE_DMA_CTL_H|(burst_length>>5),
						 SSI_WRITE_DMA_CFG_L|(SPIDmaCh),SSI_WRITE_DMA_CFG_H,
						 0,0);
	
	rDMA_CHEN_L|= ((1<<(8+SPIDmaCh))|(1<<SPIDmaCh));
	
	while(!dma_transfer_over(SPIDmaCh));		
	dma_clr_int(SPIDmaCh);
	dma_detect_ch_disable(SPIDmaCh);
	dma_release_channel(SPIDmaCh);	
}
/*******************************************************************************
*   为满足 外部时钟 ，使用DMA  传输 len个 0数据 
*         提供时钟以便能从spi-flash读取出数据 
********************************************************************************/
void SpiDmaWriteZero( UINT32 len )
{
	UINT32 buf[1]={0};	
	UINT8 SPIDmaCh = dma_request_dma_channel( SSI_DMA_WRITE_CHANNL );
	rSPI_DMAREG |= (1<<7) ;
	dma_cfg_channel( SPIDmaCh,
						 (UINT32)buf,rSPI_TXFIFO,
						 0,
						 SSI_WRITE_ZERO_DMA_CTL_L,SSI_WRITE_DMA_CTL_H|(len),/* 传输 20个值为0的 word数据 */
						 SSI_WRITE_DMA_CFG_L|(SPIDmaCh),SSI_WRITE_DMA_CFG_H,
						 0,0);
	
	rDMA_CHEN_L|= ((1<<(8+SPIDmaCh))|(1<<SPIDmaCh));
	
	while(!dma_transfer_over(SPIDmaCh));		
	dma_clr_int(SPIDmaCh);
	dma_detect_ch_disable(SPIDmaCh);
	dma_release_channel(SPIDmaCh);	
	rSPI_DMAREG &= ~(1<<7);
}

static void Spi_ControllerInit(UINT32 ulChip)
{
	if(ulChip)
		rSPI_CONTROLREG |= (1<<18);
	else
		rSPI_CONTROLREG &= ~(1<<18);

	rSPI_CONTROLREG |= (1<<5) | (1<<4) | 1; 
		
	rSPI_CONFIGREG  =  0;//(1<<20) | (1<<4) | 1;  //Mode3
	rSPI_INTREG     = 0x0;
	rSPI_PERIODREG  = 0x0;
	rSPI_TESTREG    = 1<<28;
	rSPI_DMAREG = (15<<16) | 32;
}
/*清空 FIFO 中的数据 */
static void SpiEmptyRxFIFO(void)
{
	INT32 data ;
	while(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY)
	{
		data = rSPI_RXDATA;
		(void) data;
	}
}
/* 写使能 */
 void SpiWriteEnable(void)
{
	UINT32 val;
	rSPI_TXDATA = (SPI_WRITE_ENABLE<<24);
	val  = rSPI_CONTROLREG;
	val &= ~(0xFFF<<20);	
	val |= (7<<20) |(1<<2);
	rSPI_CONTROLREG = val;
	while(!(rSPI_STATUSREG & SPI_TRANS_COMPLETE));
	while(!(SpiReadSta() & SPIFLASH_WRITEENABLE));
}

//读取 SPI-FLASH 状态 
 UINT8 SpiReadSta(void)
{
	UINT32 val;
	UINT32 status;

	SpiEmptyRxFIFO();	// 因为 需要 rSPI_TXDATA ，避免数据读取错误
	rSPI_STATUSREG |= SPI_RXFIFO_ENABLE;	
	
	rSPI_TXDATA = SPI_READ_STATUS<<16;
	val  = rSPI_CONTROLREG ;
	val &= ~(0xFFF<<20);
	val |= (15<<20) |(1<<2);
	rSPI_CONTROLREG = val;

	while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY));
	status = rSPI_RXDATA;
	rSPI_STATUSREG &= ~SPI_RXFIFO_ENABLE;

	return (status>>24);	
}

//写入  SPI-FLASH 状态 
static  void SpiWriteSta(UINT8 data )
{
	UINT32 val;
	UINT8 status;

	SpiWriteEnable();
	rSPI_STATUSREG |= SPI_RXFIFO_ENABLE;	
	
	rSPI_TXDATA = ((data<<24)|(SPI_WRITE_STATUS<<16));
	val  = rSPI_CONTROLREG ;
	val &= ~(0xFFF<<20);
	val |= (15<<20) |(1<<2);
	rSPI_CONTROLREG = val;
	while(!(rSPI_STATUSREG & SPI_TRANS_COMPLETE));
	while(SpiReadSta() & SPIFLASH_BUSY);
}




static void SpiWriteSingleByte(UINT32 addr, UINT8 data)
{
	UINT8 tmpaddr[3];
	UINT32 val;

	tmpaddr[0] = addr;
	tmpaddr[1] = addr>>8;
	tmpaddr[2] = addr>>16;
	
	SpiWriteEnable();
	rSPI_TXDATA = SPI_PAGE_PROGRAM<<24;
	rSPI_TXDATA = (data<<24) | (tmpaddr[0]<<16) | (tmpaddr[1]<<8) | tmpaddr[2];
	val   = rSPI_CONTROLREG;
	val  &= ~(0xFFF<<20);	
	val  |= (39<<20) | (1<<2);
	rSPI_CONTROLREG = val;
	while(!(rSPI_STATUSREG & SPI_TRANS_COMPLETE));
	while(SpiReadSta() & SPIFLASH_BUSY);
}
/*******************************************************************************
* static UINT8 SpiReadSingleByte(UINT32 addr )
* 在地址 addr 读取 单个字节数据 并返回这个数据数值 
*******************************************************************************/
static UINT8 SpiReadSingleByte(UINT32 addr )
{
	UINT32 data;
	UINT8 tmpaddr[3];
	UINT32 val;

	tmpaddr[0] = addr;
	tmpaddr[1] = addr>>8;
	tmpaddr[2] = addr>>16;
	
	SpiEmptyRxFIFO();	
	rSPI_STATUSREG |= SPI_RXFIFO_ENABLE;	

#if SPI_READ_FAST
	rSPI_TXDATA = (tmpaddr[2]<<24) | SPI_FAST_READ<<16;
	rSPI_TXDATA = (tmpaddr[0]<<8) | tmpaddr[1];
	val = rSPI_CONTROLREG;
	val &= ~(0xFFF<<20);	
	val |= (47<<20) | (1<<2);	// 48bit -1
	rSPI_CONTROLREG = val;
	while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY));
	data = rSPI_RXDATA;// 第一次 假数据	  未确定，可能不需要
#else
	rSPI_TXDATA = SPI_READ_DATA<<24;//03
	rSPI_TXDATA = (tmpaddr[0]<<16) | (tmpaddr[1]<<8) | tmpaddr[2];
	val  = rSPI_CONTROLREG ;
	val &= ~(0xFFF<<20);	
	val |= (39<<20) | (1<<2);	// 40bit -1    5byte * 8 bit = 40bit 
	rSPI_CONTROLREG = val;
#endif
	while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY));
	data = rSPI_RXDATA;// 第一次 假数据
	while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY));
	data = rSPI_RXDATA;// 这次是正确 数据 
	rSPI_STATUSREG &= ~SPI_RXFIFO_ENABLE;

	return (data>>24);
}

/*******************************************************************************
*static void Spi_CfgClk(void)
* 配置 时钟 
********************************************************************************/
static void Spi_CfgClk(void)
{
	unsigned int val;
	unsigned int clk_src;
	unsigned int div = 3;
	// 23-20	R/W	4	Ssp_clk_sel: IntSspClkSwitch
	// 					4'b0000: cpupll_clk
	// 					4'b0001: syspll_clk 
	// 					4'b0010: audpll_clk
	// 					4'b0100: clk_24m
	// 					Note: Other value is forbidden.
	clk_src = 0x04;
	// 19-16	R/W	0	IntSspClkSwitch_div 
	//						IntSspClkDiv = IntSspClkSwitch / (IntSspClkSwitch_div ? IntSspClkSwitch_div : 1)
	
	val = rSYS_DEVICE_CLK_CFG0;
	val &= ~(0xFF << 16);
	val |= (clk_src << 20) | (div << 16);
	rSYS_DEVICE_CLK_CFG0 = val;	
}

/*******************************************************************************
* static void Spi_SelectChip(UINT32 ulChip)
* 完成片选 
********************************************************************************/
static void Spi_SelectChip(UINT32 ulChip)
{
	UINT32 val;

	if(ulChip)
	{
		printk ("illegal spi chip(%d)\n", ulChip);
		return;
		/*
		val = rSYS_PAD_CTRL07;
		val &= ~((0x3<<4) | (0x3F<<8));
		val |= (2<<4) | (2<<8) | (2<<10) | (2<<12);
		rSYS_PAD_CTRL07 = val;
		rSYS_PAD_CTRL0A |= (1<<1);	
		rSPI_CONTROLREG |= (1<<18);*/
	}
	else
	{
		// pad_ctl8
		//                                 0           1              2
		// [17:16]	nd_cle	nd_cle_pad	GPIO88	nandflash_cle	ssp_csn0_out
		// [19:18]	nd_ale	nd_ale_pad	GPIO89	nandflash_ale	ssp_rxd
		// [21:20]	nd_ren	nd_ren_pad	GPIO90	nandflash_ren	ssp_txd
		// [23:22]	nd_wen	nd_wen_pad	GPIO91	nandflash_wen	ssp_clkout
		
		OS_IncDI();    // Initially disable interrupts
		val = rSYS_PAD_CTRL08;
		val &= ~0x00FF0000;
		val |=  0x00AA0000;
		rSYS_PAD_CTRL08 = val;
		OS_DecRI();		// enable interrupt
		
		//rSPI_CONTROLREG &= ~(1<<18);
		val  = rSPI_CONTROLREG ;
		val &= ~(0x3<<18);
		val |=  (0x0<<18);
		rSPI_CONTROLREG = val;
	}			
}



////////////////////////////////////////////////////////
static void Spi_Read_JEDEC_ID(UINT32 ulChip, UINT8 *mfid, UINT8 *memid, UINT8 *capid)
{
	UINT32 val;
	
	Spi_SelectChip(ulChip);

	SpiEmptyRxFIFO();
	rSPI_STATUSREG |= SPI_RXFIFO_ENABLE;  
	
	rSPI_TXDATA = SPI_READ_JEDEC_ID;
	val  = rSPI_CONTROLREG ;
	val &= ~(0xFFF<<20);
	val |= (31<<20) | (1<<2);  
	rSPI_CONTROLREG = val;
	
	while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY));	
	val = rSPI_RXDATA;
	rSPI_STATUSREG &= ~SPI_RXFIFO_ENABLE;
	
	*mfid  = val>>8;
	*memid = val>>16;
	*capid = val>>24;
}

////////////////////////////////////////////////////////
static void Spi_Read_Device_ID(UINT32 ulChip, UINT8 *mfid, UINT8 *devid)
{
	UINT32 val;
	
	Spi_SelectChip(ulChip);

	SpiEmptyRxFIFO();
	rSPI_STATUSREG |= SPI_RXFIFO_ENABLE;  
	
	rSPI_TXDATA = SPI_MF_DEVICE_ID<<16;
	rSPI_TXDATA = 0;
	val  = rSPI_CONTROLREG;
	val &= ~(0xFFF<<20);
	val |= (47<<20) | (1<<2);  
	rSPI_CONTROLREG = val;
	
	while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY));	
	val = rSPI_RXDATA;
	while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY));	
	val = rSPI_RXDATA;	
	rSPI_STATUSREG &= ~SPI_RXFIFO_ENABLE;
	
	*mfid  = val>>16;
	*devid = val>>24;

}

#define	SSI_DEBUG
int init_SPI(void)
{
	unsigned int i;  
	
	UINT8 mfid,devid,memid,capid;
	
	ulChip = 0; //select chip0
	//ulChip = 1; //select chip1
	
	// 创建互斥信号量
	OS_CREATERSEMA (&SpiOpt_Sem);
	
	// reset nand
	Nand_ClkDisable ();
	sys_soft_reset (softreset_nand);

	Spi_ClkDisable();
	// reset spi
	sys_soft_reset (softreset_ssp);

	Spi_ClkEnable();
	Spi_CfgClk();
	
	
	Spi_SelectChip(ulChip);
	Spi_ControllerInit(ulChip);
	
	Spi_Read_JEDEC_ID(ulChip, &mfid,&memid,&capid);
	Spi_Read_Device_ID(ulChip, &mfid,&devid);

	Spi_ClkDisable();
	
	for(i = 0; i < (sizeof(Spi_Mf_ID)/sizeof(tSPI_Mf_ID))-1; i++)
	{
		if(mfid==Spi_Mf_ID[i].Id)
			break;
	}
	
#ifdef SSI_DEBUG
	printk(Spi_Mf_ID[i].Mf_Name);
	printk("ManufacturerID: 0x%02x\n", mfid);
	printk("DeviceID: 0x%02x\n", devid);	
	printk("Memory Type ID: 0x%02x\n", memid);
	printk("Capacity ID: 0x%02x\n", capid);	
#endif

	pSpiDev = Spi_DevInfo;
	for(i = 0; i < (sizeof(Spi_DevInfo)/sizeof(tSPI_DevInfo))-1; i++)
	{
		if(mfid==pSpiDev->Mf_ID)
		{
			if((memid==pSpiDev->Type)
			 &&(capid==pSpiDev->Capacity))
			 //&&(devid==pSpiDev->DeviceID))
			 break;
		}
		pSpiDev++;
	}
	if(pSpiDev->SectorSize < 0x80000000)
	{
		nMinSectorSize = pSpiDev->SectorSize;
		nMaxSectorSize = nMinSectorSize;
	}
	else
	{
		nMinSectorSize = pSpiDev->SectorSize&0x1FFFF;
		nMaxSectorSize = pSpiDev->BlockSize&0x1FFFF;
		if(nMinSectorSize < nMaxSectorSize)
		{
			//nBottomBlockAddr = 0;
			//nTopBlockAddr = nMaxSectorSize; // remove waring
		}
		else
		{
			i = nMinSectorSize;
			nMinSectorSize = nMaxSectorSize;
			nMaxSectorSize = i;
			//nBottomBlockAddr = pSpiDev->ChipSize-nMaxSectorSize;
			//nTopBlockAddr = pSpiDev->ChipSize; // remove waring
		}
	}
	
	printk(pSpiDev->Part);
	printk(" Size = 0x%X\n", pSpiDev->ChipSize);
	//printk("Min = %d, Max = %d\n", nMinSectorSize, nMaxSectorSize);

	/*	
	ProtectStatus = spi_rdsr();
	printk("Protect: 0x%X\n", ProtectStatus);
	if(ProtectStatus&0xFC)
	{
		spi_wrsr(0x00);
		ProtectStatus = spi_rdsr();
		printk("Clear Protect: 0x%X\n", ProtectStatus);
	}
	*/
	return i;
}

void Spi_ChipErase(UINT32 ulChip)
{
	UINT32 val;
	printk("Start erase the whole spi flash,please wait...\n");	

	Spi_ClkEnable();
	Spi_SelectChip(ulChip);
	SpiWriteEnable();
	
	rSPI_TXDATA = (UINT32)(SPI_CHIP_ERASE<<24);
	val   = rSPI_CONTROLREG;
	val  &= ~(0xFFF<<20);	
	val  |= (7<<20) | (1<<2);
	rSPI_CONTROLREG = val;
	while(!(rSPI_STATUSREG & SPI_TRANS_COMPLETE));
	
	while(SpiReadSta() & SPIFLASH_BUSY ) ;
	Spi_ClkDisable();
	printk("The whole flash erased!\n");
}

//////////////////////////////////////////////////////////
void Spi_BlockErase(UINT32 ulChip, UINT32 blockNum)
{
	UINT32 addr;
	UINT8 tmpaddr[3];
	UINT32 val;
	
	addr = BYTESPERBLOCK*blockNum;
	tmpaddr[2] = ((addr & 0xFFFFFF) >> 16);
	tmpaddr[1] = ((addr & 0xFFFF) >> 8);
	tmpaddr[0] = (addr & 0xFF);

	Spi_ClkEnable();
	Spi_SelectChip(ulChip);
	SpiWriteEnable();
	rSPI_TXDATA = (tmpaddr[0]<<24) | (tmpaddr[1]<<16) | (tmpaddr[2]<<8) | SPI_BLOCK_ERASE;
	val  = rSPI_CONTROLREG ;
	val &= ~(0xFFF<<20);
	val |= (31<<20) | (1<<2);
	rSPI_CONTROLREG = val;
	while(!(rSPI_STATUSREG & SPI_TRANS_COMPLETE));
	while(SpiReadSta() & SPIFLASH_BUSY);
	Spi_ClkDisable();
	//printk("Block%d erased!\n",blockNum);
}
/*******************************************************************************
* static void Spi_SectorErase(UINT32 secNum)
*  擦除扇区号  secNum 
********************************************************************************/
 void Spi_SectorErase(UINT32 secNum)
{
	UINT32 addr;
	UINT8 tmpaddr[3];
	UINT32 val;

	addr = BYTESPERSECTOR*secNum;
	tmpaddr[0] = addr;
	tmpaddr[1] = addr>>8;
	tmpaddr[2] = addr>>16;
	
	SpiWriteEnable();	
	rSPI_TXDATA = (tmpaddr[0]<<24) | (tmpaddr[1]<<16) | (tmpaddr[2]<<8) | SPI_SECTOR_ERASE;
	val  = rSPI_CONTROLREG ;
	val &= ~(0xFFF<<20);
	val |= ((31<<20) |(1<<2));
	rSPI_CONTROLREG = val;
	while(!(rSPI_STATUSREG & SPI_TRANS_COMPLETE));
	while(SpiReadSta() & SPIFLASH_BUSY);
	
	//printk("Sector%d erased!\n",secNum);
}

unsigned int Spi_GetSize(void)
{
	if(pSpiDev==NULL)
		return 0;
	return pSpiDev->ChipSize;
}

INT8 *Spi_GetPart(void)
{
	if(pSpiDev==NULL)
		return 0;
	return (INT8 *)pSpiDev->Part;
}

//由于burst长度最大只能设置为2的12次方个比特位，而且需要40位来发送命令
//因而最大的传输字节数为(4096-40)/8
static void Spi_Read_FewBytes(UINT32 addr, UINT32 *buffer, UINT32 numBytes)
{
	UINT32 burst_length;
	UINT32 rundNumBytes;
	UINT32 i;
	UINT32 val;
	UINT8 *pData;
	UINT8 tmpaddr[3];

	tmpaddr[0] = addr;
	tmpaddr[1] = addr>>8;
	tmpaddr[2] = addr>>16;	
	//read for word align
	rundNumBytes = numBytes & 3;
	
	if(numBytes > 3)
	{
		SpiEmptyRxFIFO();
		val  = rSPI_STATUSREG ; 
		val |= SPI_RXFIFO_ENABLE;
		rSPI_STATUSREG  =  val; 
		
#if SPI_READ_FAST
#if  SPI_READ_FAST_DUAL
		//in (SPI_FAST_READ_DUAL) 
		burst_length = 40+(numBytes-rundNumBytes)*8;	

		rSPI_TXDATA = SPI_FAST_READ_DUAL<<24;
		rSPI_TXDATA = (tmpaddr[0]<<16) | (tmpaddr[1]<<8) | tmpaddr[2];
		
		val  = rSPI_CONTROLREG;
		val &= ~(0xFFF<<20);	
		val |= ((burst_length-1)<<20) | (1<<2);	
		rSPI_CONTROLREG = val;

		burst_length -= 40;

		while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY ) );
		val = rSPI_RXDATA;// 清除假数据 
#else
		//in (SPI_FAST_READ) 
		burst_length = 40+(numBytes-rundNumBytes)*8;	

		rSPI_TXDATA = SPI_FAST_READ<<24;
		rSPI_TXDATA = (tmpaddr[0]<<16) | (tmpaddr[1]<<8) | tmpaddr[2];
		
		val  = rSPI_CONTROLREG;
		val &= ~(0xFFF<<20);	
		val |= ((burst_length-1)<<20) | (1<<2);	
		rSPI_CONTROLREG = val;

		burst_length -= 40;

		while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY ) );
		val = rSPI_RXDATA;// 清除假数据 
#endif
#else
		//in (SPI_READ_DATA) 
		burst_length = 32+(numBytes-rundNumBytes)*8;	
		rSPI_TXDATA = (tmpaddr[0]<<24) | (tmpaddr[1]<<16) | (tmpaddr[2]<<8) | SPI_READ_DATA;
		
		val  = rSPI_CONTROLREG;
		val &= ~(0xFFF<<20);	
		val |= ((burst_length-1)<<20) | (1<<2);	
		rSPI_CONTROLREG = val;
		
		burst_length -= 32;
#endif
		while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY ) );
		val = rSPI_RXDATA;// 清除假数据 
		//send clk 
		while(rSPI_STATUSREG & SPI_TXFIFO_FULL);
#ifdef SPI_WRITEZERO_DMA  //
		SpiDmaWriteZero( (burst_length>>5) );
#else
		for(i=0;i<burst_length/32;i++)
		{
			rSPI_TXDATA = 0;	
		}
#endif
#ifdef SPI_READ_DMA		
		SpiDmaRead(buffer , burst_length );
		buffer += (burst_length/32);		
#else
#if  SPI_READ_FAST_DUAL
		for(i=0;i<burst_length/32;i+=2)
		{
			while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY));
			*buffer++ = rSPI_RXDATA;;
		}
#else
		for(i=0;i<burst_length/32;i++)
		{
			while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY));
			*buffer++ = rSPI_RXDATA;;
		}
#endif
#endif
		rSPI_STATUSREG &= ~SPI_RXFIFO_ENABLE; 
		while(SpiReadSta() & SPIFLASH_BUSY);
	}
	else
		burst_length = 0;
	pData = (UINT8 *)buffer;
	val= addr+(burst_length>>3) ;
	for(i=0;i<rundNumBytes;i++)
		*pData++ = SpiReadSingleByte(val+i);		
}

static void Spi_Read_Page(UINT32 pagenum, UINT32 *buf)
{
	UINT32 addr;
	INT32 i;
	UINT32 val;
	UINT8 tmpaddr[3];

	addr = pagenum*BYTESPERPAGE;
	tmpaddr[0] = addr;
	tmpaddr[1] = addr>>8;
	tmpaddr[2] = addr>>16;
	
	SpiEmptyRxFIFO();
	rSPI_STATUSREG |= SPI_RXFIFO_ENABLE; 
	rSPI_TXDATA = SPI_FAST_READ<<24;// SPI_FAST_READ  contains 2 words data
	rSPI_TXDATA = (tmpaddr[0]<<16) | (tmpaddr[1]<<8) | tmpaddr[2];
	val  = rSPI_CONTROLREG;
	val &= ~(0xFFF<<20);	
	val |= ((BYTESPERPAGE*8+40-1)<<20) | (1<<2);		
	rSPI_CONTROLREG = val;
	
	while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY ) );
	val = rSPI_RXDATA;// 清除假数据 	
	while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY ) );
	val = rSPI_RXDATA;// 清除假数据 
	
	//send clk
	while(!(rSPI_STATUSREG & SPI_TXFIFO_EMPTY));//判断的条件是 等待 rSPI_TXDATA 中 没有数据，如有数据，等待空为止
#ifdef SPI_WRITEZERO_DMA  //
	SpiDmaWriteZero( WORDSPERPAGE );
#else
	for(i=0;i<WORDSPERPAGE;i++)
		rSPI_TXDATA = 0;    
#endif
	
#ifdef SPI_READ_DMA
	SpiDmaRead(buf,(WORDSPERPAGE<<5));  // WORDSPERPAGE=64 // 64*32=2048
#else
#if SPI_READ_FAST_DUAL
	for(i=0;i<WORDSPERPAGE;i++)
	{
		while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY));// wait data
		*buf++ = rSPI_RXDATA;
	}
#else
	for(i=0;i<WORDSPERPAGE;i++)
	{
		while(!(rSPI_STATUSREG & SPI_RXFIFO_NOTEMPTY));// wait data
		*buf++ = rSPI_RXDATA;
	}
#endif
#endif		
	rSPI_STATUSREG &= ~SPI_RXFIFO_ENABLE; 
}

static void Spi_Read_Sector(UINT32 secnum, UINT32 *buf)
{
	INT32 i;
	UINT32 basepage;

	basepage = secnum*PAGESPERSECTORS;

	for(i=0;i<PAGESPERSECTORS;i++)
		Spi_Read_Page(basepage+i,buf+i*WORDSPERPAGE);	
}

int old_Spi_Read(UINT32 addr, UINT8 *buffer, UINT32 numBytes)
{
	UINT32 rundNumBytes;
	UINT32 i;
	UINT32 pageno,pagenum,frbytes;
	

	if(buffer == 0)
	{
		printk("buffer point is null\n");
		return 0;
	}
	if(addr+numBytes > pSpiDev->ChipSize)
	{
		printk("address over %08X\n", pSpiDev->ChipSize);
		return 0;
	}
	
#ifdef SPI_READ_DMA
	dma_inv_range ((UINT32)buffer, numBytes + (UINT32)buffer);
#endif

	
	OS_Use (&SpiOpt_Sem);

	Spi_ClkEnable();
	Spi_SelectChip(ulChip);
	//read for word align
	rundNumBytes = (UINT32)buffer & 3;
	if(rundNumBytes)//  如果非word对齐。先处理不足WORD的部分
	{
		//printk("read addr is not align.\n");
		rundNumBytes = 4-rundNumBytes;
		for(i=0; i<rundNumBytes; i++)
			*buffer++ = SpiReadSingleByte(addr+i);
		addr += rundNumBytes;
		numBytes -= rundNumBytes;
	}
	//read for PAGE align
	frbytes = BYTESPERPAGE-(addr%BYTESPERPAGE);
	if(frbytes > numBytes)
		frbytes = numBytes;
	Spi_Read_FewBytes(addr ,(UINT32*)buffer, frbytes);
	numBytes -= frbytes;
	addr += frbytes;
	buffer += frbytes;
	pageno = addr/BYTESPERPAGE;
	pagenum = numBytes/BYTESPERPAGE;
	for(i=0;i<pagenum;i++)
	{
		Spi_Read_Page(pageno+i,(UINT32*)buffer);
		buffer += BYTESPERPAGE;
		numBytes -= BYTESPERPAGE;
		addr += BYTESPERPAGE;
	}
	if(numBytes)
		Spi_Read_FewBytes(addr ,(UINT32*)buffer, numBytes);

	Spi_ClkDisable();
	OS_Unuse (&SpiOpt_Sem);
	
	return 1;
}

static void Spi_Write_Page(UINT32 pagenum, UINT32 *buf)
{
	UINT32 addr;
	INT32 i;
	UINT8 tmpaddr[3];

	addr = pagenum*BYTESPERPAGE;
	tmpaddr[0] = addr;
	tmpaddr[1] = addr>>8;
	tmpaddr[2] = addr>>16;
	
	SpiWriteEnable();
	
	rSPI_TXDATA = (tmpaddr[0]<<24) | (tmpaddr[1]<<16) | (tmpaddr[2]<<8) | SPI_PAGE_PROGRAM;
	rSPI_CONTROLREG |= ((BYTESPERPAGE*8+32-1)<<20) | (1<<2);
	
#ifdef SPI_WRITE_DMA 
	while(!(rSPI_STATUSREG & SPI_TXFIFO_EMPTY));
	rSPI_DMAREG |= (1<<7) ;
	SpiDmaWrite(buf, (WORDSPERPAGE<<5) ); //64*32= 2048 bits
#else
	while(!(rSPI_STATUSREG & SPI_TXFIFO_EMPTY));
	for(i=0;i<WORDSPERPAGE;i++)
		rSPI_TXDATA = *buf++;	
#endif
	
	while(!(rSPI_STATUSREG & SPI_TRANS_COMPLETE));
	while(SpiReadSta() & SPIFLASH_BUSY);
	
	#ifdef SPI_WRITE_DMA 
	rSPI_DMAREG &= ~(1<<7);
	#endif
}

static void Spi_Write_Sector(UINT32 secnum, UINT32 *buf)
{
	INT32 i;
	UINT32 basepage;

	Spi_SectorErase(secnum);

	basepage = secnum*PAGESPERSECTORS;
	for(i=0;i<PAGESPERSECTORS;i++)
		Spi_Write_Page(basepage+i,buf+i*WORDSPERPAGE);	
}

int Spi_Write(UINT32 addr, UINT8 *buffer, UINT32 numBytes)
{
	UINT32 i;
	UINT32 secno,secnum;
	UINT32 spos;
	UINT32 len;
	UINT8 *tmpbuf;

	if(addr==0)
	{
		//printk("Warning! SPI Addr 0 is Not Allowed To Write!\n");
		//return 0;
	}
	if(buffer==NULL)
		return 0;
	if(numBytes == 0)
		return 0;
	if(pSpiDev==NULL)
		return 0;
	
	
	OS_Use (&SpiOpt_Sem);

	secno = addr/BYTESPERSECTOR;
	spos  = addr%BYTESPERSECTOR;
	len   = BYTESPERSECTOR-spos;
	if(numBytes > len)
	{
		len = numBytes - len;
		if(len%BYTESPERSECTOR)
			secnum = len/BYTESPERSECTOR + 2;
		else
			secnum = len/BYTESPERSECTOR + 1;				
	}
	else
		secnum = 1;	
	
    tmpbuf = (UINT8 *)kernel_malloc(secnum*BYTESPERSECTOR);
	if(tmpbuf == NULL)
	{
		OS_Unuse (&SpiOpt_Sem);
		printk("Spi malloc error!\n");
		return 0;
	}
	
	Spi_ClkEnable();
	Spi_SelectChip(ulChip);	
	
#ifdef SPI_WRITE_DMA
	dma_inv_range ((UINT32)tmpbuf, secnum * BYTESPERSECTOR + (UINT32)tmpbuf);
#endif
	for(i=0;i<secnum;i++)
		Spi_Read_Sector(secno+i, (UINT32*)(tmpbuf+i*BYTESPERSECTOR));	
	memcpy((void *)(tmpbuf+spos),(void *)buffer,numBytes);
#ifdef SPI_WRITE_DMA
	dma_flush_range ((UINT32)tmpbuf, secnum * BYTESPERSECTOR + (UINT32)tmpbuf);
#endif
	
	for(i=0;i<secnum;i++)
		Spi_Write_Sector(secno+i, (UINT32*)(tmpbuf+i*BYTESPERSECTOR));
	Spi_ClkDisable();
	
	kernel_free(tmpbuf);

	OS_Unuse (&SpiOpt_Sem);
	return numBytes;
}





/*******************************************************************************
*  SpiSetMemoryProtection  设置内存保护级别 
* 参数，val= (TB BP2 BP1 BP0 )顺序 ，参数低四位数据有效，高四位忽略，见代码、
* 芯片类型：Winbound SPI-FLASH :W25X16-32-64
*  唐朝20140530
* 这个函数允许修改 芯片的保护范围，详情参考使用的芯片文档，并针对性的修改
 比如Winbound芯片可以通过设置保护范围，一半只读，一半读写都可以
********************************************************************************/
void SpiSetMemoryProtection(UINT8 val)
{
	UINT8 status;
	status = SpiReadSta ();
	printf("Old spi-flash status = 0x%02x\n",status);
	status &=~(0xf<<2);
	status |= ((val&0xf)<<2); // 保证只写进去 4 bit
	printf("New spi-flash status = 0x%02x\n",status);
	SpiWriteSta(status );
}

int Spi_Read(UINT32 addr, UINT8 *buffer, UINT32 numBytes)
{
	UINT32 rundNumBytes;
	UINT32 i;
	UINT32 pageno,pagenum,frbytes;
	unsigned char page_fifo[BYTESPERPAGE];
	unsigned int off, len;
	
#ifdef SPI_READ_DMA
	unsigned int start, end;
#endif
	

	if(buffer == 0)
	{
		printk("buffer point is null\n");
		return 0;
	}
	if(addr+numBytes > pSpiDev->ChipSize)
	{
		printk("address over %08X\n", pSpiDev->ChipSize);
		return 0;
	}
	
#ifdef SPI_READ_DMA
	start = (UINT32)buffer;
	end = numBytes + (UINT32)buffer;
	dma_inv_range (start, end);
#endif

	
	OS_Use (&SpiOpt_Sem);

	Spi_ClkEnable();
	Spi_SelectChip(ulChip);
	
	off = addr & (BYTESPERPAGE - 1);
	pageno = addr / BYTESPERPAGE;
	if(off)
	{
#ifdef SPI_READ_DMA
		dma_inv_range ((UINT32)page_fifo, BYTESPERPAGE + (UINT32)page_fifo);
#endif
		Spi_Read_Page (pageno, (UINT32*)page_fifo);
#ifdef SPI_READ_DMA
		dma_inv_range ((UINT32)page_fifo, BYTESPERPAGE + (UINT32)page_fifo);
#endif
		len = numBytes;
		if(len > (BYTESPERPAGE - off))
			len = (BYTESPERPAGE - off);
		memcpy (buffer, page_fifo + off, len);
#ifdef SPI_READ_DMA
		dma_flush_range ((unsigned int)buffer, len + (unsigned int)buffer);
#endif
		buffer += len;
		addr += len;
		numBytes -= len;
		pageno ++;
	}

	if(numBytes)
	{
		pagenum = numBytes / BYTESPERPAGE;
		for(i = 0; i < pagenum; i++)
		{
			Spi_Read_Page (pageno + i, (UINT32*)buffer);
			buffer += BYTESPERPAGE;
			numBytes -= BYTESPERPAGE;
			addr += BYTESPERPAGE;
		}
		pageno += pagenum;
	}
	
	if(numBytes)
	{
#ifdef SPI_READ_DMA
		dma_inv_range ((UINT32)page_fifo, BYTESPERPAGE + (UINT32)page_fifo);
#endif
		Spi_Read_Page (pageno, (UINT32*)page_fifo);
#ifdef SPI_READ_DMA
		dma_inv_range ((UINT32)page_fifo, BYTESPERPAGE + (UINT32)page_fifo);
#endif
		
		memcpy (buffer, page_fifo, numBytes);
#ifdef SPI_READ_DMA
		dma_flush_range ((unsigned int)buffer, numBytes + (unsigned int)buffer);
#endif
	}

	Spi_ClkDisable();
	OS_Unuse (&SpiOpt_Sem);
	
#ifdef SPI_READ_DMA
	dma_inv_range (start, end);
#endif
	
	return 1;
}