/*
**********************************************************************
Copyright (c)2012  Arkmicro Technologies Inc.  All Rights Reserved
Filename: sdmmc.c
Version : 1.2
Date    : 2011.12.20
Author  : ls
Abstract: ark1660 soc sd driver
History :
***********************************************************************
*/
#include "hardware.h"
#include "sdmmc.h"
#include "fs.h"
#include "fs_port.h"
#include "fs_card.h"
#include <assert.h>
#include <xm_printf.h>
#include "xm_dev.h"

#include "fevent.h"
#include <stdlib.h>

#include "timer.h"
#include "cpuclkcounter.h"
#include "xm_base.h"
#include "xm_kernel_err_code.h"
#include "xm_core.h"


#if SD_DEBUG
	#define DEBUG_MSG(fmt, args...)	XM_printf(fmt, ##args)
	#define SD_ERROR_MSG(fmt, args...)	XM_printf(fmt, ##args)
#else
	#define DEBUG_MSG(fmt, args...)	//
	#define SD_ERROR_MSG(fmt, args...)	XM_printf(fmt, ##args)
//DEBUG_MSG
#endif

static void PostCardEvent(UINT32 ulPlugIn, UINT32 ulCardID);


//#define	_XMSYS_SDMMC_EVENT_DRIVEN_				// 事件驱动

// 定义SDMMC控制器的FIFO字节大小
#define FIFO_SIZE		32		// 

#define MAX_BATCH_TRANS_SECTOR		16

SDMMC_INFO sdmmc_info_chip0;

#if SDMMC_DEV_COUNT > 1
SDMMC_INFO sdmmc_info_chip1;
#endif


SDMMC_REG_INFO sdmmc_reg_info_chip0;
#if SDMMC_DEV_COUNT > 1
SDMMC_REG_INFO sdmmc_reg_info_chip1;
#endif

SDMMC_INFO * sdmmc_chip0 = &sdmmc_info_chip0;
#if SDMMC_DEV_COUNT > 1
SDMMC_INFO * sdmmc_chip1 = &sdmmc_info_chip1;
#endif

static UINT32 SDHC_Controls[SDMMC_DEV_COUNT] = {	SDHC0_BASE
#if SDMMC_DEV_COUNT > 1
																  ,SDHC1_BASE
#endif
};

#define CARD_DEDIGHERING_STATUS_IDLE			0
#define CARD_DEDIGHERING_STATUS_RUNNING		1
#define CARD_DEDIGHERING_STATUS_PASS			2

static HardwareTimer *lg_pSDMMC0_DetheringTimer = NULL;
#if SDMMC_DEV_COUNT > 1
static HardwareTimer *lg_pSDMMC1_DetheringTimer = NULL;
#endif

static volatile UINT32 lg_ulCard0DeDitheringStatus = CARD_DEDIGHERING_STATUS_IDLE;
#if SDMMC_DEV_COUNT > 1
static volatile UINT32 lg_ulCard1DeDitheringStatus = CARD_DEDIGHERING_STATUS_IDLE;
#endif


#define	SDMMC_EVENT_NONE							(0)
#define	SDMMC_EVENT_WRITE_SUCCESS				(1)		// DMA传输完成
#define	SDMMC_EVENT_WRITE_DMA_ERROR			(-1)
#define	SDMMC_EVENT_WRITE_CARD_REMOVE			(-2)		// 卡拔出导致传输结束
#define	SDMMC_EVENT_WRITE_NO_CRC_STATUS		(-3)		// the CRC status start bit is not received two clocks
																		//		after the end bit of the data block is sent out, the data path does the following:
																		//		■ Signals no CRC status error to the BIU
																		//		■ Terminates further data transfer
																		//		■ Signals data transfer done to the BIU	
#define	SDMMC_EVENT_WRITE_NEGATIVE_CRC		(-4)		// If the CRC status received after the write data block is negative (that is, not 010),
																		//		a data CRC error is signaled to the BIU and further data transfer is continued.		

//static  UINT32 sd_ocr; 
//static  UINT32 sd_cid[4];
//static  UINT32 sd_csd[4];
//static  UINT32 sd_rca;
static  UINT16 block_len;
//static  UINT16 BLOCKSIZE;

static char *sdmmc_dma_lli_base[SDMMC_DEV_COUNT];	// 预先分配的DMA LLI基址
static struct dw_lli *sdmmc_dma_lli[SDMMC_DEV_COUNT];	// 预先分配的DMA LLI, cache line 对齐
static UINT sdmmc_dma_channel[SDMMC_DEV_COUNT];		// 预先分配的DMA通道
static SDMMC_INFO *sdmmc_chip[SDMMC_DEV_COUNT];

static OS_EVENT sdmmc_event[SDMMC_DEV_COUNT];		// 异步事件
static OS_EVENT sdmmc_busy_ticket_event[SDMMC_DEV_COUNT];	// Busy 定时器事件
static volatile int sdmmc_cause[SDMMC_DEV_COUNT];		

static INT8 *sd_spec[3]={"1.0 & 1.01", "1.1", "2.0"};
static INT8 *sd_capacity[2]={"Standard", "High"};
static INT8 *mmc_spec[5]={"1.0-1.2", "1.4", "2.0-2.2","3.1-3.2-3.31","4.x"};
static INT8 *mmc_capacity[2]={"Standard", "High"};

UINT32  SDHC_REG_READ32(SDMMC_INFO * sdmmc_info,unsigned int Register)
{
	UINT32 readValue;
	UINT32 SDHC_Control = sdmmc_info->sdmmc_reg_info->SDHC_Control;
	
	readValue =  *((volatile UINT32 *)(SDHC_Control + Register));

	return readValue;
	   
}

void  SDHC_REG_WRITE32(SDMMC_INFO * sdmmc_info,unsigned int Register,unsigned int rdata)
{
	UINT32 SDHC_Control = sdmmc_info->sdmmc_reg_info->SDHC_Control;
	
	  *((volatile UINT32 *)(SDHC_Control + Register)) = rdata;
}

// 检测卡的插入状态
// 卡一旦拔出, 直到卡驱动开始重新初始化卡, 这段时间内均认为卡已无效(即使卡已快速重新插入),
// 卡无效时, 卡驱动会阻止所有卡IO操作, 防止卡操作异常或异常操作导致的卡损坏
static int is_card_insert (SDMMC_INFO * sdmmc_info)
{
	if(sdmmc_info == NULL)
		return 0;
	if(sdmmc_info->card_present)
	{
		// 确认物理卡是否已插入
		if(!(SDHC_REG_READ32(sdmmc_info, SDMMC_CDETECT) & 0x1))
			return 1;
	}
	
	return 0;
} 


INT32 IsCardExist(UINT32 ulCardID)
{
	INT32 CardExist = 0; 
	SDMMC_INFO * sdmmc_info;
	if(ulCardID >= SDMMC_DEV_COUNT)
		return CardExist;

	switch(ulCardID)
	{
		case 0:
			sdmmc_info = sdmmc_chip0;
			break;
#if SDMMC_DEV_COUNT > 1
		case 1:
			sdmmc_info = sdmmc_chip1;
			break;
#endif
		default:
			return CardExist;
	}
	
	// 检查卡的物理连接状态
	//CardExist = !(SDHC_REG_READ32(sdmmc_info, SDMMC_CDETECT) & 0x1);
	//return  CardExist; 
	return is_card_insert (sdmmc_info);
}  

static void fifo_dma_reset (SDMMC_INFO * sdmmc_info)
{
	unsigned int ticket_timeout = XM_GetTickCount () + 100;		// timeout 100ms
	UINT32 val;
	
	//XM_printf ("FIFO&DMA reset\n");
	
	// Generic DMA mode – Simultaneously sets controller_reset, fifo_reset, and dma_reset;
	// FIFO reset
	val = SDHC_REG_READ32 (sdmmc_info, SDMMC_CTRL);
	val |= 0x07;
	SDHC_REG_WRITE32 (sdmmc_info, SDMMC_CTRL, val);
	while(SDHC_REG_READ32 (sdmmc_info, SDMMC_CTRL) & 0x07)
	{
		if(XM_GetTickCount() >= ticket_timeout)
		{
			XM_printf ("FIFO&DMA reset timeout\n");
			break;
		}
	}
}

#undef CONFIG_ARKN141_ASIC
/*----------------------------------
DivFreq must 0 2 4 6 8 10..
Source = CPU_PLL / DivFreq
Source = CLK_24M / 2 = 12M

----------------------------------*/
static void Select_Card_SrcClk(SDMMC_INFO * sdmmc_info, unsigned int Source,unsigned int DivFreq)
{
#ifdef CONFIG_ARKN141_ASIC	
	unsigned int IntSdmmcClkSwitch_sel;
	unsigned int IntSdmmcClkSwitch_div;

	CHIPSEL ChipSel = sdmmc_info->ChipSel;
	
	if((DivFreq <= 0) ||((DivFreq%2)!=0))
	{
		DEBUG_MSG("config error!\n");
		return;
	}

	switch(Source)
	{
		case CLK_24M:
			sdmmc_info->sd_clk_source = CLK_24M;
			IntSdmmcClkSwitch_sel = 4;
			IntSdmmcClkSwitch_div = 0;
			break;
		case SYS_PLL:
			sdmmc_info->sd_clk_source = SYS_PLL;
			IntSdmmcClkSwitch_sel = 1;
			IntSdmmcClkSwitch_div = DivFreq/2 - 1;			
			break;
		case CLK_240M:
			sdmmc_info->sd_clk_source = CLK_240M;
			IntSdmmcClkSwitch_sel = 2;
			IntSdmmcClkSwitch_div = DivFreq/2 - 1;			
			break;
		case CPU_PLL:
			sdmmc_info->sd_clk_source = CPU_PLL;
			IntSdmmcClkSwitch_sel = 0;
			IntSdmmcClkSwitch_div = DivFreq/2 - 1;			
			break;
		default:
			break;
	}
	
	if(ChipSel == CHIP0)// SD0
	{
		rSYS_SD_CLK_CFG = (0 << 20) // drv  no delay
					|(0 << 13) // sample no delay
					|(0 << 12) // 0: IntSdmmc1ClkInv
					|(IntSdmmcClkSwitch_sel << 8)  // source :   CLK_24M / SYS_PLL / CLK_240M / CPU_PLL
					|(1 << 7)  // 1: IntSdmmcClkDiv   pll  div 
					|(0 << 6)  // sdmmc_cclk_in
					|(1 << 5)  // clocks enable
					|(IntSdmmcClkSwitch_div << 0); // IntSdmmcClkSwitch_div:   IntSdmmcClkDiv=  IntSdmmcClkSwitch /  ((IntSdmmcClkSwitch_div + 1) * 2)
					 // card  clk = 240/10 = 24M
		                             
	}
	else if(ChipSel == CHIP1)// SD1
	{
		 rSYS_SD1_CLK_CFG = (0 << 20) // drv  no delay
						|(0 << 13) // sample no delay
						|(0 << 12) // 0: IntSdmmcClkInv
						|(IntSdmmcClkSwitch_sel << 8)  // source :   CLK_24M / SYS_PLL / CLK_240M / CPU_PLL
						|(1 << 7)  // 1: IntSdmmcClkDiv   pll  div
						|(0 << 6)  // sdmmc_cclk_in
						|(1 << 5)  // clocks enable
					 	|(IntSdmmcClkSwitch_div << 0); // IntSdmmcClkSwitch_div:   IntSdmmcClkDiv=  IntSdmmcClkSwitch /  ((IntSdmmcClkSwitch_div + 1) * 2) -> card  clk = 240/8 = 30M
	}
	else if(ChipSel == CHIP2)// SD2
	{
		 rSYS_SD2_CLK_CFG = (0 << 20) // drv  no delay
						|(0 << 13) // sample no delay
						|(0 << 12) // 0: IntSdmmc1ClkInv
						|(IntSdmmcClkSwitch_sel << 8)  // source :   CLK_24M / SYS_PLL / CLK_240M / CPU_PLL
						|(1 << 7)  // 1: IntSdmmcClkDiv   pll  div
						|(0 << 6)  // sdmmc_cclk_in
						|(1 << 5)  // clocks enable
					 	|(IntSdmmcClkSwitch_div << 0); // IntSdmmcClkSwitch_div:   IntSdmmcClkDiv=  IntSdmmcClkSwitch /  ((IntSdmmcClkSwitch_div + 1) * 2) -> card  clk = 240/8 = 30M
	}
#endif
}

unsigned int Get_Card_SrcClk(SDMMC_INFO * sdmmc_info)
{
#ifdef CONFIG_ARKN141_ASIC	
	unsigned int rSD_CLK_REG;
	unsigned int IntSdmmcClkSwitch_sel;
	unsigned int IntSdmmcClkSwitch_div;
	unsigned int GetCradSrcClk;
	
	unsigned int div;
	CHIPSEL ChipSel = sdmmc_info->ChipSel;

	if(ChipSel == CHIP0)
	{
		rSD_CLK_REG = rSYS_SD_CLK_CFG;
	}
	else if(ChipSel == CHIP1)
	{
		rSD_CLK_REG = rSYS_SD1_CLK_CFG;
	}
	else if(ChipSel == CHIP2)
	{
		rSD_CLK_REG = rSYS_SD2_CLK_CFG;
	}
	else
		return 0;

	IntSdmmcClkSwitch_sel = (rSD_CLK_REG>>8)&0xf;
	IntSdmmcClkSwitch_div = rSD_CLK_REG&0x1f;
	XM_printf ("IntSdmmcClkSwitch_sel: %d\n",IntSdmmcClkSwitch_sel);
	XM_printf("IntSdmmcClkSwitch_div: %d\n",IntSdmmcClkSwitch_div);

	div = (IntSdmmcClkSwitch_div + 1)*2;
	XM_printf("div : %d\n",div);
	switch(IntSdmmcClkSwitch_sel)
	{
		case 0://cpupll_clk
			GetCradSrcClk = GetCPUPLLFrequency()/div;
			break;
		case 1://syspll_clk
			GetCradSrcClk = GetSYSPLLFrequency()/div;
			break;
		case 2://dds_clk
			GetCradSrcClk = GetCLK240MFrequency()/div;
			break;
		case 4://clk_24m
			GetCradSrcClk = GetEXT24ExcryptFrequency()/div;
			break;
		default:
			GetCradSrcClk = GetEXT24ExcryptFrequency()/div;
			break;
	}
	XM_printf("GetCradSrcClk : %d\n",GetCradSrcClk);

	return  GetCradSrcClk;
#else
	// FPGA
	return 24000000;
#endif
}


#define	SD1_PAD_GROUP_0	0	// 使用卡1的第0组管脚输出组合(即使用SD1的管脚配置输出)
#define	SD1_PAD_GROUP_1	1	// 使用卡1的第1组管脚输出组合(与LCD的lcd_out11 ~ lcd_out17复用)
static int sd1_pad_group = SD1_PAD_GROUP_0;
static void select_sd_pad(SDMMC_INFO * sdmmc_info)
{
	UINT32 val;
	CHIPSEL ChipSel = sdmmc_info->ChipSel;
	
	if(ChipSel == CHIP0)
	{
		OS_IncDI();	// disable interrupt
		rSYS_PAD_CTRL09 |= 0x7F;
		OS_DecRI();	// enable interrupt		
	}
	else if(ChipSel == CHIP1)
	{
		OS_IncDI();	// disable interrupt
		if(sd1_pad_group == SD1_PAD_GROUP_0)
		{
			rSYS_PAD_CTRL0B &= ~(1 << 2);	// 0 配置SD卡1使用卡1的第0组管脚输出组合(即使用SD1的管脚配置输出)
													//	1 配置SD卡1使用卡1的第1组管脚输出组合(与LCD的lcd_out11 ~ lcd_out17复用)
			rSYS_PAD_CTRL09 |= (0x7F << 7);
		}
		else
		{
			rSYS_PAD_CTRL0B |=  (1 << 2);	// 0 配置SD卡1使用卡1的第0组管脚输出组合(即使用SD1的管脚配置输出)
													//	1 配置SD卡1使用卡1的第1组管脚输出组合(与LCD的lcd_out11 ~ lcd_out17复用)
			val = rSYS_PAD_CTRL06;
			val &= ~( (0x7 << 3) | (0x7 << 6) | (0x7 << 9) | (0x7 << 12) | (0x7 << 15) | (0x7 << 18) | (0x7 << 21) );
			val |=  ( (0x2 << 3) | (0x2 << 6) | (0x2 << 9) | (0x2 << 12) | (0x2 << 15) | (0x2 << 18) | (0x2 << 21) );
			rSYS_PAD_CTRL06 = val;
		}
		OS_DecRI();	// enable interrupt
	}
	else 
	{
		XM_printf ("illegal SD chip (%d)\n", ChipSel);
	}
	busy_delay(10);
	//busy_delay(880);	// 2008.11.7,Carl added for the slow one --> SanDisk 64M

	

}


static void GetCardInfo(INT32 *CID, CARDTYPE ct)
{
	UINT16 Year;
	UINT8 Month;
	UINT8 CardName[7];
	UINT8 RevisionN;
	UINT8 RevisionM;
	UINT8 ManufacturerID;
	UINT16 OEMApplicationID;
	UINT32 ProductSerialNumber;
	
	if(ct== MMC)
	{
		Month = (UINT8)((CID[0]&0x0000f000)>>12);
		Year = (unsigned long)((CID[0]&0x00000f00)>>8)+1997;
		RevisionN = (UINT8)((CID[1]&0x00f00000)>>20);
		RevisionM = (UINT8)((CID[1]&0x000f0000)>>16);
		CardName[0] = (UINT8)(CID[3]&0x000000ff);
		CardName[1] = (UINT8)((CID[2]&0xff000000)>>24);
		CardName[2] = (UINT8)((CID[2]&0x00ff0000)>>16);
		CardName[3] = (UINT8)((CID[2]&0x0000ff00)>>8);
		CardName[4] = (UINT8)(CID[2]&0x000000ff);
		CardName[5] = (UINT8)((CID[1]&0xff000000)>>24);
		CardName[6] = '\0';
		ManufacturerID = (UINT8)((CID[3]&0xff000000)>>24);
		OEMApplicationID =(UINT16)((CID[3]&0x00ffff00)>>8);
		ProductSerialNumber = (UINT32)(((CID[1]&0x00ffffff)<<8) + ((CID[0]&0xff000000)>>24));
		DEBUG_MSG("Product Name : %s\n", CardName);
		DEBUG_MSG("Product revision : %d.%d\n", RevisionN, RevisionM);
		DEBUG_MSG("Manufacturing date %d-%d\n", Month, Year);
		DEBUG_MSG("ManufacturerID :%x\n",ManufacturerID);
		DEBUG_MSG("OEMApplicationID :%x\n",OEMApplicationID);
		DEBUG_MSG("ProductSerialNumber :%x\n",ProductSerialNumber);
	}
	else
	{
		Month = (UINT8)((CID[0]&0x00000f00)>>8);
		Year = (unsigned long)((CID[0]&0x000ff000)>>12)+2000;
		RevisionN = (UINT8)((CID[1]&0xf0000000)>>28);
		RevisionM = (UINT8)((CID[1]&0x0f00000)>>24);
		CardName[0] = (UINT8)(CID[3]&0x000000ff);
		CardName[1] = (UINT8)((CID[2]&0xff000000)>>24);
		CardName[2] = (UINT8)((CID[2]&0x00ff0000)>>16);
		CardName[3] = (UINT8)((CID[2]&0x0000ff00)>>8);
		CardName[4] = (UINT8)(CID[2]&0x000000ff);
		CardName[5] = '\0';
		ManufacturerID = (UINT8)((CID[3]&0xff000000)>>24);
		OEMApplicationID =(UINT16)((CID[3]&0x00ffff00)>>8);
		ProductSerialNumber = (UINT32)(((CID[1]&0x00ffffff)<<8) + ((CID[0]&0xff000000)>>24));
		DEBUG_MSG("Product Name : %s\n", CardName);
		DEBUG_MSG("Product revision : %d.%d\n", RevisionN, RevisionM);
		DEBUG_MSG("Manufacturing date %d-%d\n", Month, Year);
		DEBUG_MSG("ManufacturerID :%x\n",ManufacturerID);
		DEBUG_MSG("OEMApplicationID :%x\n",OEMApplicationID);
		DEBUG_MSG("ProductSerialNumber :%x\n",ProductSerialNumber);
	}
}

static INT32  confi_fifo_unempty(SDMMC_INFO * sdmmc_info)
{
	UINT32 ticket = XM_GetTickCount();
	while ((SDHC_REG_READ32(sdmmc_info, SDMMC_STATUS) & 0x00000004))
	{
		if((XM_GetTickCount() - ticket) > 3000)	// 1000ms
		{
			DEBUG_MSG("confi_fifo_unempty timeout\n");		
			return 1;
		}
		if(!is_card_insert(sdmmc_info))
		{
			// card removed
			DEBUG_MSG("confi_fifo_unempty failed, card (%d) removed\n", sdmmc_info->ChipSel);	
			return 1;
		}
	}
	
	return 0;
}

static UINT8 GetSD_SPEC(UINT32 *scr)
{
	UINT8 ss;
	ss=(UINT8)(scr[0]&0x0000000f);
	
	return ss;
}

static UINT8 GetMMC_SPEC(SDMMC_INFO * sdmmc_info)
{
	UINT8 ss;
 	ss = (sdmmc_info->sdmmc_reg_info->sd_csd[3]>>26)&0x0000000f;
	
	return ss;
}

#define	USE_TICKET_EVENT

#ifdef USE_TICKET_EVENT

//static	OS_EVENT ticket_event;	// 100us ticket
static void timer_event_intr_handler (void *ticket_event)
{
	OS_EVENT_Set (ticket_event);
}

#endif


INT32 data_tran_over(SDMMC_INFO * sdmmc_info)
{
  	INT32 tt;
	UINT32 ticket = XM_GetTickCount();

	do
	{
		//if( (XM_GetTickCount() - ticket) > SDMMC_DATA_TIMEOUT_MS )
		if( (XM_GetTickCount() - ticket) > 2*SDMMC_DATA_TIMEOUT_MS )
		{
			DEBUG_MSG("data_tran_over is timeout and ChIP is %d\n", sdmmc_info->ChipSel);
			return 1;
		}
		
		if(!is_card_insert(sdmmc_info))
		{
			DEBUG_MSG("data_tran_over (%d) failed, card removed\n", sdmmc_info->ChipSel);
			return 1;
		}
		
		tt = SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS);
		//if(tt & ((1<<9)|(1<<7)|(1<<15)))		//timeout, end_bit err, data crc err
		{
			//DEBUG_MSG("ERROR SDMMC_RINTSTS  is %d\n", tt);		    
			//return 1;
		}
	}while (!(tt & 0x00000008));

	// 20181228 bit 10 –Data starvation-by-host timeout (HTO) (0x400) 会导致以下条件满足，导致返回错误。
	// 实际上HTO仅用来警告FIFO已被填满(读)或者FIFO已取空(写)的标志，不是异常
	//if( (tt&0xBFC2) != 0 )
	if( (tt&0xBBC2) != 0 )
	{
		DEBUG_MSG("This rSDMMC_RINTSTS = 0x%x and ChIP is %d\n", tt, sdmmc_info->ChipSel);

		SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);
		return 1;
	}
	//rSDMMC_RINTSTS = 0x0000ffff;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);
	 
	return 0;   // 0 means successful
}

static INT32 send_finish_command(SDMMC_INFO * sdmmc_info)
{
	UINT32 ticket = XM_GetTickCount();

	do 
	{
		if( (XM_GetTickCount() - ticket) > 5 )
		{
			DEBUG_MSG("send_finish_command failed, timeout\n");
			return	1;
		}
		
		if(!is_card_insert(sdmmc_info))
		{
			DEBUG_MSG("send_finish_command failed, card removed\n");
			return 1;
		}
		//31 R/W 0 start_cmd
		// It is auto-cleared. When bit is set, host should not attempt to write to any command registers. 
		// If write is attempted, hardware lock error is set in raw interrupt register
	}while (SDHC_REG_READ32(sdmmc_info, SDMMC_CMD)  & 0x80000000);

	return 0;
}

// 1) CMD2 and ACMD41
// The card response to the host command starts after NID clock cycles)
// 2) other command
// The minimum delay between the host command and card response is NCR clock cycles.
//
// NID min(5) max(5)  clock cycles
// NCR min(2) max(64) clock cycles
// 
// R1/R1b (48 bits), 
// R2 (136 bits),
// R3 (48 bits)
// R6 (48 bits)
// 
// 最大超时时间 = 5 (NID) + 136 (R2, CID) = 141 sd_clk cycle 
// 按低速150KHz时钟计算，141 * (1000/150) = 21.15us = 1ms
static INT32 sd_command_done(SDMMC_INFO * sdmmc_info)
{
	UINT32 ticket = XM_GetTickCount();
	INT32 val;
	int ret = 0;
	
	// If the DWC_mobile_storage is unable to load the command – that is, a command is already in
	// 	progress, a second command is in the buffer, and a third command is attempted – then it
	// 	generates an HLE (hardware-locked error).
	// 4. Check if there is an HLE.
	if( SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS) & RINTSTS_HLE )
	{
		DEBUG_MSG("sd_command_done failed, hardware-locked error\n");		
		return 1;
	}
	
#ifdef USE_TICKET_EVENT
	OS_EVENT *event;
	
	//OS_EVENT_Create (&ticket_event);
	event = &sdmmc_busy_ticket_event[sdmmc_info->ChipSel - CHIP0];
	timer_x_start (1, 10000, timer_event_intr_handler, event );
#endif
	
	
	// 5. Wait for command execution to complete. After receiving either a response from a card or response
	//		timeout, the DWC_mobile_storage sets the command_done bit in the RINTSTS register. Software can
	//		either poll for this bit or respond to a generated interrupt.	
  	do
	{
		// 1) SD卡写入操作时
		//    当卡快速拔出又立刻插入时, SD卡不再响应COMMAND, 无Command done (CD), 
		//    导致CMD12_STOP_STEARM发出后出现死循环
		//		加入下面的超时判断, 按照理论值, 最大超时时间 = 5 (NID) + 136 (R2, CID) = 141 sd_clk cycle 
		
		// When multiple block write is stopped by CMD12, the busy from the response of CMD12 is up to 500ms.		
		if( (XM_GetTickCount() - ticket) > 500 )
		{
			val = SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS);
			// bit 2 – Command done (CD)
			if(val & 0x00000004)
			{
				DEBUG_MSG("sd_command_done ok, but timeout\n");		
				break;
			}
			//DEBUG_MSG("sd_command_done failed, timeout\n");		
			ret =	1;
			break;
		}

		if(!is_card_insert(sdmmc_info))
		{
			//DEBUG_MSG("sd_command_done failed, card removed\n");		
			ret = 1;
			break;
		}
#ifdef USE_TICKET_EVENT
		OS_EVENT_WaitTimed (event, 1);
#endif		
		// bit 2 – Command done (CD)
		val = SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS);
	}while (!(val & 0x00000004));
	
#ifdef USE_TICKET_EVENT
	timer_x_stop (1);
#endif
	
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);
	// 检查response_timeout error, response_CRC error, or response error is set.
	if(val & (RINTSTS_RTO | RINTSTS_RCRC | RINTSTS_RE))
	{
		//DEBUG_MSG("sd_command_done failed, response error(SDMMC_RINTSTS = 0x%08x)\n", val & 0xFFFF);
		return 1;
	}
	return ret;
}

static INT32 sd_command_done_no_clear_RINTSYS (SDMMC_INFO * sdmmc_info)
{
	UINT32 ticket = XM_GetTickCount();
	INT32 val;
	int ret = 0;
	
	// If the DWC_mobile_storage is unable to load the command – that is, a command is already in
	// 	progress, a second command is in the buffer, and a third command is attempted – then it
	// 	generates an HLE (hardware-locked error).
	// 4. Check if there is an HLE.
	if( SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS) & RINTSTS_HLE )
	{
		DEBUG_MSG("sd_command_done failed, hardware-locked error\n");		
		return 1;
	}
	
#ifdef USE_TICKET_EVENT
	OS_EVENT *event;
	
	//OS_EVENT_Create (&ticket_event);
	event = &sdmmc_busy_ticket_event[sdmmc_info->ChipSel - CHIP0];
	timer_x_start (1, 10000, timer_event_intr_handler, event );
#endif
	
	
	// 5. Wait for command execution to complete. After receiving either a response from a card or response
	//		timeout, the DWC_mobile_storage sets the command_done bit in the RINTSTS register. Software can
	//		either poll for this bit or respond to a generated interrupt.	
  	do
	{
		// 1) SD卡写入操作时
		//    当卡快速拔出又立刻插入时, SD卡不再响应COMMAND, 无Command done (CD), 
		//    导致CMD12_STOP_STEARM发出后出现死循环
		//		加入下面的超时判断, 按照理论值, 最大超时时间 = 5 (NID) + 136 (R2, CID) = 141 sd_clk cycle 
		// When multiple block write is stopped by CMD12, the busy from the response of CMD12 is up to 500ms.		
		if( (XM_GetTickCount() - ticket) > 500 )
		{
			val = SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS);
			// bit 2 – Command done (CD)
			if(val & 0x00000004)
			{
				DEBUG_MSG("sd_command_done ok, but timeout\n");		
				break;
			}
			//DEBUG_MSG("sd_command_done failed, timeout\n");		
			ret =	1;
			break;
		}

		if(!is_card_insert(sdmmc_info))
		{
			//DEBUG_MSG("sd_command_done failed, card removed\n");		
			ret = 1;
			break;
		}
#ifdef USE_TICKET_EVENT
		OS_EVENT_WaitTimed (event, 1);
#endif				
		// bit 2 – Command done (CD)
		val = SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS);
	}while (!(val & 0x00000004));
	
#ifdef USE_TICKET_EVENT
	timer_x_stop (1);
#endif
	
	//SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);
	// 检查response_timeout error, response_CRC error, or response error is set.
	if(val & (RINTSTS_RTO | RINTSTS_RCRC | RINTSTS_RE))
	{
		// 错误时不清除SDMMC_RINTSTS, 外部调用继续后续处理
		DEBUG_MSG("sd_command_done failed, response error(SDMMC_RINTSTS = 0x%08x)\n", val & 0xFFFF);
		return 1;
	}
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);
	return ret;
}

static int get_card_status (SDMMC_INFO * sdmmc_info, unsigned int *card_status)
{
	int ret = -1;
	// [31:16] RCA
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, sdmmc_info->sdmmc_reg_info->sd_rca);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD13_STATUS_CARD);
	if(sd_command_done(sdmmc_info) == 0)
	{
		*card_status = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0);
		ret = 0;
	}

  	return ret;
}


static INT32  chang_clk(SDMMC_INFO * sdmmc_info, INT32 div, INT32 src, INT32 clken)
{
	// 1) Before disabling the clocks, ensure that the card is not busy due to any previous data command. To
	//		determine this, check for 0 in bit 9 of the STATUS register.	
	// 9 data_busy	
	//		0 – card data not busy
	//		1 – card data busy
	if(SDHC_REG_READ32(sdmmc_info, SDMMC_STATUS)  & (1 << 9))
	{
		DEBUG_MSG("change_clk failed, the card is busy\n");
		return 1;
	}
	
	// 2) Update the Clock Enable register to disable all clocks. To ensure completion of any previous
	//		command before this update, send a command to the CIU to update the clock registers by setting:
	//		. start_cmd bit
	//		. “update clock registers only” bit
	//		. “wait_previous data complete” bit
	//		Wait for the CIU to take the command by polling for 0 on the start_cmd bit.
	//rSDMMC_CLKENA = 0x00000000;         // stop clock
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CLKENA, 0x00000000);
	//rSDMMC_CMD = CMD_CHANG_CLK;            // bit 31,21,13 set, viz start_cmd,Update_clk_regs_only,
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD_CHANG_CLK);
	if(send_finish_command(sdmmc_info))
	{
		DEBUG_MSG("change_clk stop clock failed\n");
		return 1;
	}

	// 3) Set the start_cmd bit to update the Clock Divider and/or Clock Source registers, and send a
	//		command to the CIU in order to update the clock registers; wait for the CIU to take the command.
	//rSDMMC_CLKDIV = div;         // because div to high ,temp change it to a low value :4
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CLKDIV, div);
	//rSDMMC_CLKSRC = src;         // 16 card all use the clock source 0
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CLKSRC, src);
	//rSDMMC_CMD = CMD_CHANG_CLK;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD_CHANG_CLK);
	if(send_finish_command(sdmmc_info))
	{
		DEBUG_MSG("change_clk change failed\n");	
		return 1;
	}

	//	4) Set start_cmd to update the Clock Enable register in order to enable the required clocks and send a
	//		command to the CIU to update the clock registers; wait for the CIU to take the command.	
	//rSDMMC_CLKENA = clken;         // enable all 16 card clock
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CLKENA, clken);
	//rSDMMC_CMD = CMD_CHANG_CLK;//uart_print_word(rSDMMC_MINTSTS);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD_CHANG_CLK);
	if(send_finish_command(sdmmc_info))
	{
		DEBUG_MSG("change_clk enable clock failed\n");
		return 1;
	}
	else
	{
		DEBUG_MSG("change_clk enable clock successful\n");
		return 0;
	}
}

static INT32  initial_clk(SDMMC_INFO * sdmmc_info)
{
  	//rSDMMC_CMD = CMD_INITIAL_CLK;       // first initial card for 80 clock then send command;
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD_INITIAL_CLK);
  	if(sd_command_done(sdmmc_info))
  	{
		DEBUG_MSG("sd cmd fails, error\n");
		return 1;
	}

	return 0;
}

static INT32  conf_spec(SDMMC_INFO * sdmmc_info)
{
	int loop_times = REPEAT_COUNT_OF_FAILED_COMMAND;
	while(loop_times > 0)
	{
		//rSDMMC_CMDARG = 0x00000100|0x000000ff;
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, 0x00000100|0x000000ff);
		//rSDMMC_CMD = CMD8_SPEC;                 //CMD8
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD8_SPEC);

		if(sd_command_done(sdmmc_info) == 0)
			break;
		
		loop_times --;
	}
	
	if(loop_times == 0)
	{
		DEBUG_MSG ("conf_spec error\n");
		return 1;
	}
	
	//if((rSDMMC_RESP0&0x000000ff) == 0x000000ff)   //card will echos when it is compatible with 2.0
	if((SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0) & 0x000000ff) == 0x000000ff)   //card will echos when it is compatible with 2.0
	{
		sdmmc_info->CardSpec = Ver2;      				//protocol 2.x
		DEBUG_MSG ("protocol 2.x\n");
	}
	else
	{
		sdmmc_info->CardSpec = Ver1;	     				//protocol 1.x
		DEBUG_MSG ("protocol 1.x\n");
	}
	return 0;
}

#define MAX_MATCH_LOOP		30
#define MAX_INQUIRY_LOOP	10
static INT32 match_sd_vcc(SDMMC_INFO * sdmmc_info)
{
	INT32 tt = 0;
	INT32 hc;
	INT32 i;
	INT32 base_time = 10000;
	
	for (i = 0; i < MAX_INQUIRY_LOOP; i ++)
	{
		// inquiry CMD41 use for getting OCR
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, 0x00000000);
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD55_APP);
		if( sd_command_done(sdmmc_info))
		{
			DEBUG_MSG("match_sd_vcc failed, inquiry CMD41's CMD55 error\n");
			return 2;
		}
		
		// inquiry CMD41
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, 0x00000000);
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD41_MATCH_VCC);
		if(sd_command_done(sdmmc_info))
		{
			DEBUG_MSG("match_sd_vcc failed, inquiry CMD41's CMD41 error\n");
			return 2;
		}
		tt = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0);
		if(tt & ~0x80000000)
			break;
		OS_Delay (1);
	}
	if(i == MAX_INQUIRY_LOOP)
	{
		// 无应答，卡损坏
		DEBUG_MSG("match_sd_vcc failed, inquiry no response\n");
		return 0;
	}

	//XM_printf ("card's OCR = 0x%08x\n", tt);
	// 卡已应答它的电压范围。如果卡因为下面的原因(the card has not finished the power up routine)无法识别成功，
	//		提示用户“卡接触不良，重新插卡”
	
	// last 1s
	UINT32 start_ticket = XM_GetTickCount();
  	//for(i=0 ;i < MAX_MATCH_LOOP; i++ )
	while(1)
  	{
		//rSDMMC_CMDARG = 0x00000000;
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, 0x00000000);
		//rSDMMC_CMD = CMD55_APP;          // rSDMMC_CMD 55
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD55_APP);
		if( sd_command_done(sdmmc_info))
		{
			DEBUG_MSG("sd_command_done fails111\n");
			return 2;
		}
		
		if(sdmmc_info->CardSpec == Ver1)
			//rSDMMC_CMDARG = 0x00FF8000;		// normal voltage : 2.7-3.3 v
			SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, 0x00FF8000);
		else
			//rSDMMC_CMDARG = 0x40FF8000;            // normal voltage : 2.7-3.3 v
			SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, 0x40FF8000);
		
		//rSDMMC_CMD = CMD41_MATCH_VCC;          // CMD41
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD41_MATCH_VCC);
		if(sd_command_done(sdmmc_info))
		{
			DEBUG_MSG("sd_command_done22 error in match vcc\n");
			return 2;
		}
		
		OS_Delay (10);
		//busy_delay(base_time);
	     //   DEBUG_MSG("Response data is %x\n",rSDMMC_RESP0);
		//base_time += 10000;
		//tt = rSDMMC_RESP0&0x80000000;
		// bit31 card power up status bit(busy), this bit is set to LOW if the card has not finished the power up routine.
		// Bit 31 - Card power up status bit, this status bit is set if the card power up procedure has been finished.
		tt = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0)&0x80000000;
		if(tt == 0x80000000)
			break;
		
		if(!is_card_insert(sdmmc_info))
		{
			DEBUG_MSG("card removed in match vcc\n");
			return 2;
		}
		
		if( (XM_GetTickCount() - start_ticket) > 2000 )
		{
			DEBUG_MSG("sd match voltage failed! timeout\n");
			return 2;
		}
	}
	//Read Card, Must read card before return 0 or 1
  	//tt = rSDMMC_RESP0&0x00ff8000;
  	tt = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0)&0x00ff8000;
  	//hc = rSDMMC_RESP0&0x40000000;
  	hc = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0)&0x40000000;
	//if((i >= MAX_MATCH_LOOP) ||(tt == 0))
	if(tt == 0)
	{
		DEBUG_MSG("sd failure match voltage! timeout\n");
		return 0;
	}
	
	
	// Bit 30 - Card Capacity Status bit, 0 indicates that the card is SDSC. 1 indicates that the card is SDHC or
	// SDXC. The Card Capacity Status bit is valid after the card power up procedure is completed and the
	// card power up status bit is set to 1. The Host shall read this status bit to identify SDSC Card or
	// SDHC/SDXC Card.	
  	if(hc == 0x40000000)
	{
		sdmmc_info->Capacity = High;
	}
  	else
	{
		sdmmc_info->Capacity = Standard;
	}
	//  	sd_ocr = tt;    //Here get OCR
	
  	return 1;
}

static INT32 match_mmc_vcc(SDMMC_INFO * sdmmc_info)
{
	INT32 tt;
	INT32 i;
	INT32 base_time = 10000;

	for(i=0 ;i < MAX_MATCH_LOOP; i++ )
	{
		//rSDMMC_CMDARG = 0x00FF8000;       // normal voltage : 2.7-3.3 v
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, 0x00FF8000);
		//rSDMMC_CMD = CMD1_MATCH_VCC;          // CMD1
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD1_MATCH_VCC);
		if( sd_command_done(sdmmc_info))
			return 2;

		busy_delay(base_time);

		base_time += 10000;
		//tt = rSDMMC_RESP0&0x80000000;
		tt = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0)&0x80000000;
		if(tt == 0x80000000)
			break;
	}

	//Read Card, Must read card before return 0 or 1
	//tt = rSDMMC_RESP0&0x00ff8000;
	tt = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0)&0x00ff8000;
	if((i >= MAX_MATCH_LOOP) ||(tt == 0))
	{
		DEBUG_MSG("mmc failure match voltage! times =%d\n",i);
		return 0;
	}
	
//	sd_ocr = tt;    //Here get OCR

	return 1;
}

static INT32 get_RCA(SDMMC_INFO * sdmmc_info)
{
	int loop_times = REPEAT_COUNT_OF_FAILED_COMMAND;
	while (loop_times > 0)
	{
		//rSDMMC_CMD = CMD2_CID;          // CMD2   --- all send CID
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD2_CID);
		if(sd_command_done(sdmmc_info) == 0)
			break;
		loop_times --;
	}
	if(loop_times == 0)
	{
		DEBUG_MSG("get_RCA failed, CMD2_CID NG\n");
		return 1;
	}

//	sd_cid[0] = rSDMMC_RESP0;
//	sd_cid[1] = rSDMMC_RESP1;
//	sd_cid[2] = rSDMMC_RESP2;
//	sd_cid[3] = rSDMMC_RESP3;

	sdmmc_info->sdmmc_reg_info->sd_cid[0] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0);
	sdmmc_info->sdmmc_reg_info->sd_cid[1] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP1);
	sdmmc_info->sdmmc_reg_info->sd_cid[2] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP2);
	sdmmc_info->sdmmc_reg_info->sd_cid[3] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP3);
		
	loop_times = REPEAT_COUNT_OF_FAILED_COMMAND;
	while(loop_times > 0)
	{
		//rSDMMC_CMD = CMD3_RCA;          // CMD3    --- ask card publish new RCA     ///  = 9FFC
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD3_RCA);
		if(sd_command_done(sdmmc_info) == 0)
			break;
		loop_times --;
	}
	if(loop_times == 0)
	{
		DEBUG_MSG("get_RCA failed, CMD3_RCA NG\n");
		return 1;
	}
	   
	//sd_rca = rSDMMC_RESP0&0xffff0000;
	sdmmc_info->sdmmc_reg_info->sd_rca = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0)&0xffff0000;

	DEBUG_MSG("SD get card RCA:0x%x\n",sdmmc_info->sdmmc_reg_info->sd_rca);

	return 0;
}

static INT32 get_csd(SDMMC_INFO * sdmmc_info, UINT32 rca,UINT32 *csd)
{
	int loop_times = REPEAT_COUNT_OF_FAILED_COMMAND;
	while(loop_times > 0)
	{
		//rSDMMC_CMDARG = rca;
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, rca);
		//rSDMMC_CMD = CMD9_CSD;          // CMD9   --- addressed card send CSD
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD9_CSD);
		if(sd_command_done(sdmmc_info) == 0)
			break;
		loop_times --;
	}
	if(loop_times == 0)
	{
		DEBUG_MSG("get_csd failed, CMD9_CSD NG\n");
		return 1;
	}
	
//  	csd[0] = rSDMMC_RESP0;
//  	csd[1] = rSDMMC_RESP1;
//  	csd[2] = rSDMMC_RESP2;
//  	csd[3] = rSDMMC_RESP3;

  	csd[0] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0);
  	csd[1] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP1);
  	csd[2] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP2);
  	csd[3] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP3);
	
	return 0;
}

static INT32  get_cid(SDMMC_INFO * sdmmc_info, UINT32 rca, UINT32 *cid)
{
	int loop_times = REPEAT_COUNT_OF_FAILED_COMMAND;
	while (loop_times > 0)
	{
		//rSDMMC_CMDARG = rca;
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, rca);
		//rSDMMC_CMD = CMD10_CID;          // CMD10   --- all send CID
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD10_CID);
		if(sd_command_done(sdmmc_info) == 0)
			break;
		loop_times --;
	}
	if(loop_times == 0)
	{
		DEBUG_MSG("get_cid failed, CMD10_CID NG\n");
		return 1;
	}
	
  	cid[0] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0);
  	cid[1] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP1);
  	cid[2] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP2);
  	cid[3] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP3);

	if((SDCard.Present == 0) && (sdmmc_info->ChipSel == CHIP0))
	{
		SDCard.REG_CID[0] = cid[0];
		SDCard.REG_CID[1] = cid[1];
		SDCard.REG_CID[2] = cid[2];
		SDCard.REG_CID[3] = cid[3];
	}
#if SDMMC_DEV_COUNT > 1
	else if((SDCard1.Present == 0) && (sdmmc_info->ChipSel == CHIP1))
	{
		SDCard1.REG_CID[0] = cid[0];
		SDCard1.REG_CID[1] = cid[1];
		SDCard1.REG_CID[2] = cid[2];
		SDCard1.REG_CID[3] = cid[3];
	}
#endif
	return 0;
}

static INT32 get_blocklen(SDMMC_INFO * sdmmc_info)
{
	INT32 mid,i;
	INT32 val = 1;

	mid = (sdmmc_info->sdmmc_reg_info->sd_csd[2]>>16)&0x0000000f;
	for(i = 0; i < mid; i ++)
		val *=2;

	return val;
}

static UINT32 get_cap1(SDMMC_INFO * sdmmc_info)
{
	INT32 c_size,c_size0,c_size1;
	INT32 mult;
	INT32 blocknr;
	UINT32 memory_cap;
	INT32 mid,i;

	c_size0 = (sdmmc_info->sdmmc_reg_info->sd_csd[1] >> 30) & 0x00000003;		//get size value first party
	c_size1 = (sdmmc_info->sdmmc_reg_info->sd_csd[2] << 2) & 0x000000ffc;		//get size value second party
	c_size = c_size0 | c_size1;               			//get size value
	c_size += 1;

	mid = (sdmmc_info->sdmmc_reg_info->sd_csd[1] >> 15) & 0x00000007;    		//caculate mult

	mid += 2;
	mult = 1;
	for(i = 0; i < mid; i ++)
	{
		mult *= 2;                         				 	//get mut value
	}
	blocknr = c_size*mult;                 				//get mount block
	memory_cap = (blocknr*sdmmc_info->blocksize) >> 9;        	//get this card capacity, unit is sector

	return memory_cap;
}

static UINT32 get_cap2(SDMMC_INFO * sdmmc_info)
{
	INT32 c_size,c_size0,c_size1;
	UINT32 memory_cap;

	c_size0 = (sdmmc_info->sdmmc_reg_info->sd_csd[1] >> 16) & 0x0000FFFF;
	c_size1 = (sdmmc_info->sdmmc_reg_info->sd_csd[2] << 16) & 0x003F0000;

	c_size = c_size0 | c_size1;               				//get size value
	c_size = c_size + 1;

	memory_cap = c_size<<10; //unit is sector

	return memory_cap;
}

static UINT32 get_capacity(SDMMC_INFO * sdmmc_info)
{
	UINT32 ret;

	if(sdmmc_info->CardSpec == Ver2 && sdmmc_info->Capacity== High)
	{
		ret = get_cap2(sdmmc_info);
	}
	else
	{
		ret = get_cap1(sdmmc_info);
	}
	DEBUG_MSG("sdmmc capacity = %dsector\n",ret);
		
	return ret;
}

static INT32 get_TAAC(SDMMC_INFO * sdmmc_info)
{
	UINT32 time_unit[] = {1,10,100,1000,10000,100000,1000000,10000000};
	UINT32 time_value[] = {0,10,12,13,15,20,25,30,35,40,45,50,55,60,70,80};
	UINT32 TAAC_value;
	UINT32 i,j;

	
	i = (sdmmc_info->sdmmc_reg_info->sd_csd[3]>>16)&0x07;
	j = (sdmmc_info->sdmmc_reg_info->sd_csd[3]>>19)&0x1f;
	TAAC_value = time_unit[i]*time_value[j];

	return TAAC_value;

}

static INT32 get_NASC(SDMMC_INFO * sdmmc_info)
{
	return (sdmmc_info->sdmmc_reg_info->sd_csd[3]>>8)&0xFF;
}

static INT32 get_R2W_FACTOR(SDMMC_INFO * sdmmc_info)
{
	return (sdmmc_info->sdmmc_reg_info->sd_csd[0]>>26)&0x07;
}

// Part_1_Physical_Layer_Simplified_Specification_Ver_3.01_Final_100518.pdf
// 4.6.2 Read, Write and Erase Timeout Conditions
static void CalculateTimeOutParameter(SDMMC_INFO * sdmmc_info)
{
	UINT32 taac;
	UINT32 nasc;
	UINT32 r2w_factor;
	UINT32 read_max_timeout, write_max_timeout;

	if(sdmmc_info->CardSpec == Ver2 && sdmmc_info->Capacity== High)
	{
		// A High Capacity SD Memory Card and Extended Capacity SD Memory Card indicate TAAC and NSAC
		// as fixed values. The host should use 100 ms timeout (minimum) for single and multiple read operations
		// rather than using TAAC and NSAC.
		sdmmc_info->ReadTimeOutCycle = sdmmc_info->sd_clk/10;	//conver 100ms to sd_clk cycles
		
		// In case of High Capacity SD Memory Card, maximum length of busy is defined as 250ms for all write operation
		// It is strongly recommended for hosts to implement more than 500ms timeout value even if the card 	 
		//		indicates the 250ms maximum busy length. 	 

		//sdmmc_info->WriteTimeOutCycle = sdmmc_info->sd_clk/4;	//conver 250ms to sd_clk cycles
		sdmmc_info->WriteTimeOutCycle = sdmmc_info->sd_clk/2;	//conver 500ms to sd_clk cycles
	}
	else
	{
		UINT32 tmp, div,tmp_sd_clk;
		// For a Standard Capacity SD Memory Card, the times after which a timeout condition for read operations
		// occurs are (card independent) either 100 times longer than the typical access times for these
		// operations given below or 100 ms (the lower of the two). The read access time is defined as the sum
		// of the two times given by the CSD parameters TAAC and NSAC (see Chapter 5.3).
		
		// 4.6.2.2 Write
		// For a Standard Capacity SD Memory Card, the times after which a timeout condition for write
		// operations occurs are (card independent) either 100 times longer than the typical program times for
		// these operations given below or 250 ms (the lower of the two).		
		tmp = get_TAAC(sdmmc_info);

		//下述代码涉及大整数的乘除法运算，为了处理
		//简单，并减小误差，这里考虑了数据都是10的整倍数
		//因此，先使用10的约整法，缩小运算数据大小以
		//保证精度
		div = 1000000000;	// 1 s
		tmp_sd_clk = sdmmc_info->sd_clk;
		do
		{
			if((tmp/10)*10==tmp)
			{
				if(div == (div/10)*10)
				{
					tmp = tmp/10;
					div = div/10;
				}
				else
					break;
			}
			else
				break;
		}while(1);

		do
		{
			if((tmp_sd_clk/10)*10 == tmp_sd_clk)
			{
				if(div == (div/10)*10)
				{
					tmp_sd_clk = tmp_sd_clk/10;
					div = div/10;
				}
				else
					break;
			}
			else
				break;
		}while(1);
		taac = (tmp*tmp_sd_clk)/div;

		
		nasc = get_NASC(sdmmc_info) * 100;
		r2w_factor = get_R2W_FACTOR(sdmmc_info);
		r2w_factor = 1<<r2w_factor;

		read_max_timeout = sdmmc_info->sd_clk/10;	//conver 100ms to sd_clk cycles
		write_max_timeout = sdmmc_info->sd_clk/4;	//conver 250ms to sd_clk cycles

		sdmmc_info->ReadTimeOutCycle = (taac + nasc)*100;
		sdmmc_info->WriteTimeOutCycle = sdmmc_info->ReadTimeOutCycle * r2w_factor;
		if(sdmmc_info->ReadTimeOutCycle > read_max_timeout)
			sdmmc_info->ReadTimeOutCycle = read_max_timeout;
		if(sdmmc_info->WriteTimeOutCycle > write_max_timeout)
			sdmmc_info->WriteTimeOutCycle = write_max_timeout;		
	
	}
}




static INT32 select_sd(SDMMC_INFO * sdmmc_info, INT32 rca)
{
	int loop_times = REPEAT_COUNT_OF_FAILED_COMMAND;
	while(loop_times > 0)
	{
		//rSDMMC_CMDARG = rca;
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, rca);
		//rSDMMC_CMD = CMD7_SELECT_CARD;          // CMD7    --- select the card
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD7_SELECT_CARD);
		if(sd_command_done(sdmmc_info) == 0)
			break;
		loop_times --;
	}
	
	if(loop_times == 0)
	{
		DEBUG_MSG("select_sd failed, CMD7_SELECT_CARD NG\n");
		return 1;
	}
	else
		return 0;
}

static INT8 set_blocklen(SDMMC_INFO * sdmmc_info, UINT16 len)
{
	UINT16 length;

	int loop_times = REPEAT_COUNT_OF_FAILED_COMMAND;
	length = len;
	
	while(loop_times > 0)
	{
		//rSDMMC_CMDARG = length;
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, length);
		//rSDMMC_CMD = CMD16_SET_BLOCKLEN;          // CMD16    --- set block length = blk_len byte
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD16_SET_BLOCKLEN);
		if(sd_command_done(sdmmc_info) == 0)
			break;
		loop_times --;
	}
	if(loop_times == 0)
	{
		DEBUG_MSG("set_blocklen failed, CMD16_SET_BLOCKLEN NG\n");
		return 1;
	}

	// 29 BLOCK_LEN_ERROR
	// The transferred block length is not allowed for this card, or the number
	// of transferred bytes does not match the block length.
  	//while(rSDMMC_RESP0 & 0x20000000)
  	while(SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0) & 0x20000000)
  	{
		//rSDMMC_CMDARG = length >> 1;
		loop_times = REPEAT_COUNT_OF_FAILED_COMMAND;
		while(loop_times > 0)
		{
			SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, length >> 1);
			//rSDMMC_CMD = CMD16_SET_BLOCKLEN;          // CMD16    --- set block length = blk_len/2 byte
			SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD16_SET_BLOCKLEN);
	
			if(sd_command_done(sdmmc_info) == 0)
				break;
			
			loop_times --;
		}
		
		if(loop_times == 0)
		{
			DEBUG_MSG("set_blocklen failed, CMD16_SET_BLOCKLEN NG\n");
			return 1;
		}
  	}

  	return 0;
}

static int sd_bus_width(SDMMC_INFO * sdmmc_info, UINT32 width)
{
	int loop_times = REPEAT_COUNT_OF_FAILED_COMMAND;
	while(loop_times > 0)
	{
		if(width == 4)
			//rSDMMC_CTYPE= 1;             					// bus_width = 4 bit;
			SDHC_REG_WRITE32(sdmmc_info, SDMMC_CTYPE, 1);
		else
			//rSDMMC_CTYPE = 0;
			SDHC_REG_WRITE32(sdmmc_info, SDMMC_CTYPE, 0);
		//rSDMMC_CMDARG = sd_rca;
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, sdmmc_info->sdmmc_reg_info->sd_rca);
		//rSDMMC_CMD = CMD55_APP;              				//CMD55
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD55_APP);
		if(sd_command_done(sdmmc_info) == 0)
			break;
		loop_times --;
	}
	if(loop_times == 0)
	{
		DEBUG_MSG("sd_bus_width failed, CMD55_APP NG\n");
		return 1;
	}

	loop_times = REPEAT_COUNT_OF_FAILED_COMMAND;
	while(loop_times > 0)
	{
		switch(width)
		{
			case 4  :
				//rSDMMC_CMDARG = 0x00000002;
				SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG , 0x00000002);
				break;
			default :
				//rSDMMC_CMDARG = 0x00000000;
				SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG , 0x00000000);
		}
	
		//rSDMMC_CMD = ACMD6_WID;             				//CMD6
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, ACMD6_WID);
		if(sd_command_done(sdmmc_info) == 0)
			break;
		
		loop_times --;
	}
	
	if(loop_times == 0)
	{
		DEBUG_MSG("sd_bus_width failed, ACMD6_WID NG\n");
		return 1;
	}
	
	return 0;
}

XMINT64 		XM_GetHighResolutionTickCount (void);


#ifdef SDMMC_BUSY_DEBUG
static XMINT64 busy_data_delay = 0;
static int busy_delay_count = 0;
#endif

static INT32 busy_data(SDMMC_INFO * sdmmc_info)
{
	INT32 count_sd=0;
	UINT32 ticket = XM_GetTickCount();
	XMINT64 s_ticket = XM_GetHighResolutionTickCount();
	XMINT64 e_ticket;
	int s_diff;
	int ret = 0;
	
#ifdef USE_TICKET_EVENT
	OS_EVENT *event;
	
	//OS_EVENT_Create (&ticket_event);
	event = &sdmmc_busy_ticket_event[sdmmc_info->ChipSel - CHIP0];
	timer_x_start (1, 10000, timer_event_intr_handler, event );
#endif

#ifdef SDMMC_BUSY_DEBUG
	busy_delay_count ++;
#endif
   
	do
	{
		count_sd ++;
		if(count_sd == 1000)
		{
			count_sd = 0;
			DEBUG_MSG("busy_data is error and ChIP is %d\n", sdmmc_info->ChipSel);
		}
		
		// 按最大读写
		//if( (XM_GetTickCount() - ticket) > SDMMC_DATA_TIMEOUT_MS )
		if( (XM_GetTickCount() - ticket) > (5 * 1000) )
		{
			DEBUG_MSG("busy_data (%d) failed, exceed max timeout, SDMMC_STATUS=0x%08x\n", sdmmc_info->ChipSel, SDHC_REG_READ32(sdmmc_info, SDMMC_STATUS));
			ret = 1;
			break;
		}
		
		if(!is_card_insert(sdmmc_info))
		{
			DEBUG_MSG("busy_data (%d) failed, card removed\n", sdmmc_info->ChipSel);
			ret = 1;
			break;
		}

		
		if(SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS) & (1<<9))
		{
			DEBUG_MSG("busy_data, Data read timeout (DRTO)\n");
			ret = 1;
			break;
		}
		
		// ensure that the card is not busy due to any previous data command. To
		// determine this, check for 0 in bit 9 of the STATUS register.	
		if(!(SDHC_REG_READ32(sdmmc_info, SDMMC_STATUS) & 0x00000200))
			break;
		
#ifdef USE_TICKET_EVENT
		OS_EVENT_WaitTimed (event, 1);
#else
		OS_Delay (1);
#endif
	}while(1);

	
#ifdef SDMMC_BUSY_DEBUG
	e_ticket = XM_GetHighResolutionTickCount() - s_ticket;
#endif
	
#ifdef USE_TICKET_EVENT
	timer_x_stop (1);
	//OS_EVENT_Delete (&ticket_event);
#endif
	
#ifdef SDMMC_BUSY_DEBUG
	busy_data_delay += e_ticket;
	
	if((busy_delay_count % 512) == 0)
	{
		s_diff = (int)(busy_data_delay / busy_delay_count);
		XM_printf ("busy data delay = %d\n", (int)s_diff);
	}
#endif
	
	return ret;
}

// 0 	DMA通道正常disable
// -1	SD卡已拔出
// -2	DMA通道无法disable, 超时异常. (需要确认该异常处理的方法)
static int card_dma_detect_ch_disable(SDMMC_INFO * sdmmc_info, UINT8 channel)
{
	unsigned int timeout_ticket = XM_GetTickCount () + 1000;
	int ret = 0;
	while( rDMA_CHEN_L & (1 << channel) )
	{
		/*
		if((SDCard.Present == 0) && (sdmmc_info->ChipSel == CHIP0))
		{
			DEBUG_MSG("card0_dma_detect_ch_disable!!!!!!\n");
			ret = -1;
			break;
		}
#if SDMMC_DEV_COUNT > 1
		if((SDCard1.Present == 0) && (sdmmc_info->ChipSel == CHIP1))
		{
			DEBUG_MSG("card1_dma_detect_ch_disable!!!!!!\n");
			ret = -1;
			break;
		}
#endif
		*/
		if(!is_card_insert(sdmmc_info))
		{
			DEBUG_MSG("card%d_dma_detect_ch_disable! card plugout\n", sdmmc_info->ChipSel);
			ret = -1;
			break;
		}
		
		if(XM_GetTickCount() >= timeout_ticket)
		{
			XM_printf ("Fatal Error, card_dma_detect_ch_disable (%d) timeout\n", channel);
			ret = -2;
			break;
		}
		
	}
	
	return ret;
}

#define DMA_OPT
#ifdef DMA_OPT
// 预先分配DMA通道7, FIFO depth=128bytes, 可以满足 同时读取16 burst(64bytes),及写入16 burst(64bytes)
#define SDMMC_DMA_FAVORITE_CHANNEL		7
#else
// 预先分配DMA通道1, FIFO depth=64bytes
#define SDMMC_DMA_FAVORITE_CHANNEL		1
#endif

#ifndef _NEW_SDMMC_DMA_DRIVER_
static INT32 sd_write_dma(SDMMC_INFO * sdmmc_info, UINT32 *data_buf, UINT32  len)
{
	UINT32 SdDmaCh;
	UINT32 DmaRequestTime = 0;
//	UINT32 DmaTime = 0;
	INT32 rst = 0;
	
#if 0
	do{
		SdDmaCh = dma_request_dma_channel(SDMMC_DMA_FAVORITE_CHANNEL);
		if((DmaRequestTime++)>50000)
		{
			DEBUG_MSG("SD request dma channel%d failed!\n");
			return 1;
		}
	}while(SdDmaCh==ALL_CHANNEL_RUNNING);
#else
	SdDmaCh = sdmmc_dma_channel[sdmmc_info->ChipSel - CHIP0];
#endif
//CheckPADRegs();
	dma_clr_int(SdDmaCh);

	dma_cfg_channel(SdDmaCh, 
					(UINT32)data_buf, sdmmc_info->sdmmc_reg_info->SDHC_Control + SDMMC_FIFO,
					0,
					SD_TX_DMA_CTL_L,SD_TX_DMA_CTL_H|len/4,
					SD_TX_DMA_CFG_L|(SdDmaCh<<5),(SD_TX_DMA_CFG_H|(sdmmc_info->dma_req<<12)),
					0,0);
	dma_start_channel(SdDmaCh);

	while(!dma_transfer_over(SdDmaCh))
	{
		/*
		if((SDCard.Present == 0) && (sdmmc_info->ChipSel == CHIP0))
		{
			DEBUG_MSG("Card Removed!!!!!!!\n");
			rst = 1;
			break;
		}
#if SDMMC_DEV_COUNT > 1
		if((SDCard1.Present == 0) && (sdmmc_info->ChipSel == CHIP1))
		{
			DEBUG_MSG("Card Removed!!!!!!!\n");
			rst = 1;
			break;
		}
#endif*/
		if(!is_card_insert(sdmmc_info))
		{
			DEBUG_MSG("Card %d Removed!\n", sdmmc_info->ChipSel);	
			rst = 1;
			break;			
		}
		if(SDHC_REG_READ32(sdmmc_info,SDMMC_RINTSTS) & (1<<9))
		{
			DEBUG_MSG("Write DMA err!!!!!!!\n");
			rst = 1;
			break;
		}
	}

	dma_clr_int(SdDmaCh);
	if(card_dma_detect_ch_disable(sdmmc_info, SdDmaCh) != 0)
	{
		rst = 1;
	}
	dma_release_channel(SdDmaCh);

	return rst;	
	//busy_delay(1000000);
}
#endif

#ifdef _NEW_SDMMC_DMA_DRIVER_
/*
static int check_sdmmc_status (SDMMC_INFO * sdmmc_info)
{
	int ret = 0;
	unsigned int rintsts = SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS);
	if(sdmmc_info->is_read_mode)
	{
		// 检查SD卡读异常
					
		// Data CRC error (bit 7) – CRC error occurred during data reception
		if(rintsts & (1 << 7))
		{
			DEBUG_MSG("Read DMA err, Data CRC error!!!!!!!\n");
			ret = -1;
		}
		// Start bit error (bit 13) – Start bit was not received during data reception.
		// End bit error (bit 15) – End bit was not received during data reception or for a write
		//		operation; a CRC error is indicated by the card.
		else if(rintsts & ((1 << 13) | (1 << 15)))
		{
			DEBUG_MSG("Read DMA err, Start/End bit error!!!!!!!\n");
			ret = -1;
		}
	}
	else
	{
		// 检查SD卡写异常
				
		// End bit error (bit 15) – End bit was not received during data reception or for a write
		//		operation; a CRC error is indicated by the card.
		if(rintsts & (1 << 15))
		{
			DEBUG_MSG("Write DMA err, End bit error!!!!!!!\n");
			ret = -1;
		}
	}
	
	return ret;
}*/

//#define	_DMA_CONFIG_REGISTER_

static void sdmmc_dma_transmit_stop (UINT32 chip, int stop_cause)
{
	if(sdmmc_cause[chip - CHIP0] == SDMMC_EVENT_NONE)
	{
		sdmmc_cause[chip - CHIP0] = stop_cause;
	}
	OS_EVENT_Set (&sdmmc_event[chip - CHIP0]);
}

static void sdmmc_dma_transfer_over_IRQHandler (UINT32 ulIRQFactors, UINT32 channel, void *channel_private_data)
{
   int ret = 0;
   SDMMC_INFO * sdmmc_info = channel_private_data;
#ifdef DEBUG
   assert (sdmmc_info->SdDmaCh == channel);
#endif
	do 
	{
		//if(rDMA_STATUS_ERR_L & (1<<channel))
		if(ulIRQFactors & ERR_INT_BIT)
		{
			// This interrupt is generated when an ERROR response is received from an AHB slave on the
			// HRESP bus during a DMA transfer. 
			// In addition, the DMA transfer is cancelled and the channel is disabled.
			// DMA 异常
			DEBUG_MSG ("sdmmc ERR_INT_BIT\n");
			ret = 1;
		}
		dma_clr_int(channel);
		// 检查DMA是否异常或者传输结束
		if(ret != 0 || sdmmc_info->len == 0)
		//if(ret != 0)	
		{
			// DMA异常
			break;
		}
		/*
		else if(sdmmc_info->len == 0)
		{
			// DMA传输结束
			if(check_sdmmc_status (sdmmc_info))
			{
				ret = 1;
				break;
			}
		}*/
		else
		{
			// 检查SD卡是否存在
			/*
			if(sdmmc_info->ChipSel == CHIP0)
			{
				if(SDCard.Present == 0) 
				{
					DEBUG_MSG ("dma_transfer_over_IRQHandler, card 0 removed\n");
					ret = 1;
				}
			}
#if SDMMC_DEV_COUNT > 1
			else if(sdmmc_info->ChipSel == CHIP1)
			{
				if(SDCard1.Present == 0) 
				{
					DEBUG_MSG ("dma_transfer_over_IRQHandler, card 1 removed\n");
					ret = 1;
				}
			}
#endif
			*/
			if(!is_card_insert(sdmmc_info))
			{
				DEBUG_MSG ("dma_transfer_over_IRQHandler, card %d removed\n", sdmmc_info->ChipSel);
				ret = 1;
			}

			if(ret != 0)
			{
				// SD Card removed
				break;
			}
			else
			{
				// 继续DMA传输
				unsigned int rintsts = SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS);
				unsigned int to_transfer;
				to_transfer = sdmmc_info->len;
				//XM_printf ("len=%d\n", sdmmc_info->len);
				
				if(sdmmc_info->is_read_mode)
				{
					// read mode
					if(to_transfer > (MAX_BATCH_TRANS_SECTOR*512))
						to_transfer = (MAX_BATCH_TRANS_SECTOR*512);
					// 检查SD卡读写异常
					// Data CRC error (bit 7) – CRC error occurred during data reception
					if(rintsts & (1 << 7))
					{
						DEBUG_MSG("Read DMA err, Data CRC error!!!!!!!\n");
						ret = 1;
						break;			
					}
					// Start bit error (bit 13) – Start bit was not received during data reception.
					// End bit error (bit 15) – End bit was not received during data reception or for a write
					//		operation; a CRC error is indicated by the card.
					if(rintsts & ((1 << 13) | (1 << 15)))
					{
						DEBUG_MSG("Read DMA err, Start/End bit error!!!!!!!\n");
						ret = 1;
						break;						
					}
					
					// MMC读出
#ifdef _DMA_CONFIG_REGISTER_
					dma_cfg_channel(channel, 
							sdmmc_info->sdmmc_reg_info->SDHC_Control + SDMMC_FIFO,
							(UINT32)sdmmc_info->buf,
							0,
							SD_RX_DMA_CTL_L,
							SD_RX_DMA_CTL_H|(to_transfer/4),
							SD_RX_DMA_CFG_L|(channel<<5), (SD_RX_DMA_CFG_H|(sdmmc_info->dma_req<<7)),
							0,
							0);
#else			
					rDMA_DARx_L(channel) = (UINT32)sdmmc_info->buf;
					rDMA_CTLx_H(channel) = SD_RX_DMA_CTL_H|(to_transfer/4);
#endif
				}
				else
				{
					if(to_transfer > (1*512))
						to_transfer = (1*512);
					
					// DMA传输结束，需要判断DMA写入到FIFO中的内容是否已全部写入到SD卡(最多32个WORD，141配置)
					// 32*(32/4)*SD_CLK = (256/24000000) / 516000000 = 5504 指令周期(按最快516MHz CPU时钟 )
					// 等待FIFO为空
					unsigned int loop = 0;
					// 按照最块CPU时钟516MHz，最恶劣情况需要等待5504个CPU周期，以下2000个循环足够满足5504的机器周期
					// 一般情况下fifo_count读出时为0值(数据已全部写入到SD卡)
					while(loop < 2000)
					{
						unsigned int fifo_count = (SDHC_REG_READ32(sdmmc_info, SDMMC_STATUS) >> 17) & 0x1FFF;
						if(fifo_count == 0)
							break;
						//XM_printf ("fifo_count=%d\n", fifo_count);
						loop ++;
					}
					
					// 等待一个start bit周期(一个SD_CLK, 24MHz)
					delay ( (516/24)/3 );
						
					// FIFO为空后，检查DAT0状态
					// 检查DAT0是否busy。
					// 9 data_busy 1 or 0; depends on cdata_in Inverted version of raw selected card_data[0]
					//		0 – card data not busy
					//		1 – card data busy
					// ensure that the card is not busy due to any previous data command. To
					// determine this, check for 0 in bit 9 of the STATUS register.	
					if((SDHC_REG_READ32(sdmmc_info, SDMMC_STATUS) & 0x00000200))
					{
						XM_printf ("card write busy\n");
						ret = 2;		// 标记DMA传送暂停，终止当前DMA传输。DAT0 Busy
						break;
					}
					
					// 检查SD卡读写异常
					// End bit error (bit 15) – End bit was not received during data reception or for a write
					//		operation; a CRC error is indicated by the card.
					if(rintsts & (1 << 15))
					{
						DEBUG_MSG("Write DMA err, End bit error!!!!!!!\n");
						ret = 1;
						break;						
					}
					// MMC写入
#ifdef _DMA_CONFIG_REGISTER_
					dma_cfg_channel(channel, 
									(UINT32)sdmmc_info->buf, 
									sdmmc_info->sdmmc_reg_info->SDHC_Control + SDMMC_FIFO,
									0,
									SD_TX_DMA_CTL_L,SD_TX_DMA_CTL_H|(to_transfer/4),
									SD_TX_DMA_CFG_L|(channel<<5),(SD_TX_DMA_CFG_H|(sdmmc_info->dma_req<<12)),
									0,0);
#else
					rDMA_SARx_L(channel) = (UINT32)sdmmc_info->buf;
					rDMA_CTLx_H(channel) = SD_TX_DMA_CTL_H|(to_transfer/4);
#endif
				}
				dma_start_channel(channel);
				sdmmc_info->buf += to_transfer;
				sdmmc_info->len -= to_transfer;
	
				return;
			}
		}
	} while (0);

	// DMA结束
	sdmmc_info->ret = ret;
	// 发送DMA传输结束事件
	Dma_TfrOver_PostSem (channel);
	
}

#ifdef _SDMMC_USE_LLI_DMA_

static void sdmmc_dma_transfer_over_IRQHandler_lli (UINT32 ulIRQFactors, UINT32 channel, void *channel_private_data)
{
   int ret = 0;
   SDMMC_INFO * sdmmc_info = channel_private_data;
#ifdef DEBUG
   assert (sdmmc_info->SdDmaCh == channel);
#endif
	if(ulIRQFactors & ERR_INT_BIT)
	{
		// This interrupt is generated when an ERROR response is received from an AHB slave on the
		// HRESP bus during a DMA transfer. 
		// In addition, the DMA transfer is cancelled and the channel is disabled.
		// DMA 异常
		DEBUG_MSG ("sdmmc ERR_INT_BIT\n");
		ret = 1;
		sdmmc_info->debug_flag = -1;
	}
	else
	{
		//DEBUG_MSG ("sdmmc dma end\n");
		sdmmc_info->debug_flag = 2;
	}
	dma_clr_int(channel);

	// DMA结束
	sdmmc_info->ret = ret;
	// 发送DMA传输结束事件
	Dma_TfrOver_PostSem (channel);
}
#endif

// 使用LLI DMA来避免Rx FIFO Full导致的Stop-Clock
// 7.2.5.1 Single-Block or Multiple-Block Read
// If the Card Read Threshold feature is not enabled for such cards, then the Host system should ensure
//	that the Rx FIFO does not become full during a Read Data transfer by ensuring that the Rx FIFO is
//	drained out at a rate faster than that at which data is pushed into the FIFO
// 10.7.1 Stop-Clock
static INT32 sd_read_dma_new (SDMMC_INFO * sdmmc_info, UINT32 *data_buf,UINT32  len)
{
	UINT32 SdDmaCh;
	//UINT32 DmaRequestTime = 0;
	UINT32 DmaTime = 0;
	INT32 rst = 0;
	UINT32 to_transfer;
	unsigned int rintsts;
	unsigned int ticket_to_timeout;
	
	//XM_printf ("sd_read_dma len=%d\n", len);
      
#if 0
	sdmmc_info->SdDmaCh = -1;
	do{
		SdDmaCh = dma_request_dma_channel(SDMMC_DMA_FAVORITE_CHANNEL);
		if((DmaRequestTime++)>50000)
		{
			DEBUG_MSG("SD request dma channel%d failed!\n");
			return 1;
		}
	}while(SdDmaCh==ALL_CHANNEL_RUNNING);
   sdmmc_info->SdDmaCh = SdDmaCh;
#else
	SdDmaCh = sdmmc_dma_channel[sdmmc_info->ChipSel - CHIP0];
	sdmmc_info->SdDmaCh = SdDmaCh;
#endif
	
#ifdef _SDMMC_USE_LLI_DMA_
	int lli_count = (len + MAX_BATCH_TRANS_SECTOR*512 - 1) / (MAX_BATCH_TRANS_SECTOR*512);
	/* sdmmc_info->lli = kernel_malloc(sizeof(struct dw_lli) * lli_count);
	if(sdmmc_info->lli == NULL)
	{
		DEBUG_MSG("SD request dma lli failed!\n");
		return 1;
	}*/
	sdmmc_info->lli = sdmmc_dma_lli[sdmmc_info->ChipSel - CHIP0];
   register_channel_IRQHandler (SdDmaCh, 
										  sdmmc_dma_transfer_over_IRQHandler_lli, 
										  sdmmc_info,
										  //(1<< BLOCK_INT_BIT)|(1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  (1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  );
	int i;
	UINT32 left = len;
	dma_addr_t dst_addr = (dma_addr_t)data_buf;
	dma_addr_t	sar = sdmmc_info->sdmmc_reg_info->SDHC_Control + SDMMC_FIFO;
	struct dw_lli *	lli = &sdmmc_info->lli[0];
	// 创建DMA链表
	for (i = 0; i < lli_count; i++)
	{
		lli->sar = sar;
		lli->dar = dst_addr;
		if(i != (lli_count - 1))
			lli->llp = (UINT32)&sdmmc_info->lli[i + 1];
		else
			lli->llp = 0;
		lli->ctllo = SD_RX_DMA_CTL_L_LLI | (1 << 27);
		if(left >= MAX_BATCH_TRANS_SECTOR*512)
			lli->ctlhi = SD_RX_DMA_CTL_H | (MAX_BATCH_TRANS_SECTOR*512 / 4);
		else
			lli->ctlhi = SD_RX_DMA_CTL_H | (left / 4);
		lli->sstat = 0;
		lli->dstat = 0;
		dst_addr += MAX_BATCH_TRANS_SECTOR*512;
		left -= MAX_BATCH_TRANS_SECTOR*512;
		lli ++;
	}
		
	dma_clr_int(SdDmaCh);
	dma_cfg_channel(SdDmaCh, 
					sdmmc_info->lli[0].sar,
					sdmmc_info->lli[0].dar,
					(UINT32)(&sdmmc_info->lli[0])&0xfffffffc,
					sdmmc_info->lli[0].ctllo,
					sdmmc_info->lli[0].ctlhi,
					SD_RX_DMA_CFG_L|(SdDmaCh<<5), (SD_RX_DMA_CFG_H_LLI|(sdmmc_info->dma_req<<7)),
					0,
					0);	
	dma_flush_range ((UINT32)&sdmmc_info->lli[0], (UINT32)&sdmmc_info->lli[lli_count]);
	dma_inv_range ((UINT32)data_buf, ((UINT32)data_buf) + len);

#else
	
   register_channel_IRQHandler (SdDmaCh, 
										  sdmmc_dma_transfer_over_IRQHandler, 
										  sdmmc_info,
										  //(1<< BLOCK_INT_BIT)|(1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  (1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  );

//CheckPADRegs();
	to_transfer = len;
	if(to_transfer > (MAX_BATCH_TRANS_SECTOR*512))
		to_transfer = (MAX_BATCH_TRANS_SECTOR*512);
	sdmmc_info->buf = ((unsigned int)data_buf) + to_transfer;
	sdmmc_info->len = len - to_transfer;
	sdmmc_info->is_read_mode = 1;
	sdmmc_info->ret = 0;
	sdmmc_info->SdDmaCh = SdDmaCh;
	dma_clr_int(SdDmaCh);
	dma_cfg_channel(SdDmaCh, 
					sdmmc_info->sdmmc_reg_info->SDHC_Control + SDMMC_FIFO,
					(UINT32)data_buf,
					0,
					SD_RX_DMA_CTL_L,
					SD_RX_DMA_CTL_H|to_transfer/4,
					SD_RX_DMA_CFG_L|(SdDmaCh<<5), (SD_RX_DMA_CFG_H|(sdmmc_info->dma_req<<7)),
					0,
					0);
#endif
	
	sdmmc_info->debug_flag = 0;
	// 根据一个较慢的速度(160KB/sec)估算超时 
	DmaTime = 100 * (len / (MAX_BATCH_TRANS_SECTOR*512));
	if(DmaTime < 100)
		DmaTime = 100;
	dma_start_channel(SdDmaCh);
	
	// 超时
	ticket_to_timeout = XM_GetTickCount() + 5000;
	
	while(!rst)
	{
		int finished = Dma_TfrOver_PendSemEx (SdDmaCh, DmaTime);
		if(finished == 1)
		{
			// 检查DMA执行的结果
			if(sdmmc_info->ret)
				rst = 1;
			break;
		}
		else if(finished == 0)	// timeout
		{
			// 检查SD卡是否存在
			rintsts = SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS);
			//DEBUG_MSG ("sd_read_dma ch(%d) timeout, rintsts=0x%08x, time=%d, len=%d\n", sdmmc_info->ChipSel, rintsts, DmaTime, len);
			/*
			if(sdmmc_info->ChipSel == CHIP0)
			{
				if(SDCard.Present == 0) 
				{
					DEBUG_MSG("Card 0 Removed!!!!!!!\n");
					rst = 1;
					break;
				}
			}
#if SDMMC_DEV_COUNT > 1
			else if(sdmmc_info->ChipSel == CHIP1)
			{
				if(SDCard1.Present == 0) 
				{
					DEBUG_MSG("Card 1 Removed!!!!!!!\n");
					rst = 1;
					break;
				}
			}
#endif
			*/
			if(!is_card_insert(sdmmc_info))
			{
				DEBUG_MSG("Card %d Removed!!\n", sdmmc_info->ChipSel);
				rst = 1;
				break;
			}
			
			// 20170707 发现卡快速插拔时出现异常, DMA执行未能结束. 加入超时机制
			if(XM_GetTickCount() >= ticket_to_timeout)
			{
				DEBUG_MSG("SDMMC DMA timeout\n");
				rst = 1;
				break;
			}
			
			//DEBUG_MSG("SDMMC DMA timeout\n");
			//rst = 1;
			//break;
		}
		rintsts = SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS);
		// bit 9 – Data read timeout (DRTO)
		if(rintsts & (1<<9))
		{
			DEBUG_MSG("Read DMA err, RINTSTS=0x%08x Data read timeout!!!!!!!\n", rintsts);
			rst = 1;
			
			// 20170301 异常时stop dma, 避免随后的card_dma_detect_ch_disable挂死
			dma_stop_channel (SdDmaCh);
			
			break;
		}
		
		/*
		// Data CRC error (bit 7) – CRC error occurred during data reception
		if(rintsts & (1 << 7))
		{
			DEBUG_MSG("Read DMA err, Data CRC error!!!!!!!\n");
			rst = 1;
			break;			
		}
		// Start bit error (bit 13) – Start bit was not received during data reception.
		// End bit error (bit 15) – End bit was not received during data reception or for a write
		//		operation; a CRC error is indicated by the card.
		if(rintsts & ((1 << 13) | (1 << 15)))
		{
			DEBUG_MSG("Read DMA err, Start/End bit error!!!!!!!\n");
			rst = 1;
			break;						
		}
		*/
	}
	
	sdmmc_info->debug_flag = 0;
	
	dma_clr_int(SdDmaCh);
	if(card_dma_detect_ch_disable(sdmmc_info, SdDmaCh) != 0)
	{
		rst = 1;
	}
	dma_release_channel(SdDmaCh);

#ifdef _SDMMC_USE_LLI_DMA_
	dma_inv_range ((UINT32)data_buf, ((UINT32)data_buf) + len);
	//kernel_free (sdmmc_info->lli);
	sdmmc_info->lli = 0;
#endif
	
	rintsts = SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS);
	if(rintsts & ((1 << 13) | (1 << 15)))
	{
		DEBUG_MSG("Read DMA err, Start/End bit error!!!!!!!\n");
	}
	if(rintsts & (1<<9))
	{
		//DEBUG_MSG("Read DMA err, Data read timeout!!!!!!!\n");
	}
	
	// bit 7 – Data CRC error (DCRC)
	// Data CRC error – During a read-data-block transfer, if the CRC16 received does not match with
	// the internally generated CRC16, the data path signals a data CRC error to the BIU and continues
	// further data transfer.	
	if(rintsts & (1 << 7))
	{
		DEBUG_MSG("Read DMA err, Data CRC error\n");
		//rst = 1;
		sdmmc_info->crc_error = 1;	// 标记CRC错误
	}
	return rst;	
	//busy_delay(1000000);   
}

static INT32 sd_write_dma_new (SDMMC_INFO * sdmmc_info, UINT32 *data_buf,UINT32  len)
{
	UINT32 SdDmaCh;
	UINT32 DmaRequestTime = 0;
	UINT32 DmaTime = 0;
	INT32 rst = 0;
	UINT32 to_transfer;
	unsigned int intsts;
	unsigned int ticket_to_timeout;
	
	//XM_printf ("sd_read_dma len=%d\n", len);
      
#if 0
	sdmmc_info->SdDmaCh = -1;
	do{
		SdDmaCh = dma_request_dma_channel(SDMMC_DMA_FAVORITE_CHANNEL);
		if((DmaRequestTime++)>50000)
		{
			DEBUG_MSG("SD request dma channel%d failed!\n");
			return 1;
		}
	}while(SdDmaCh==ALL_CHANNEL_RUNNING);
   sdmmc_info->SdDmaCh = SdDmaCh;
#else
	SdDmaCh = sdmmc_dma_channel[sdmmc_info->ChipSel - CHIP0];
	sdmmc_info->SdDmaCh = SdDmaCh;
#endif
	
#ifdef _SDMMC_USE_LLI_DMA_
	int lli_count = (len + MAX_BATCH_TRANS_SECTOR*512 - 1) / (MAX_BATCH_TRANS_SECTOR*512);
	/* sdmmc_info->lli = kernel_malloc(sizeof(struct dw_lli) * lli_count);
	if(sdmmc_info->lli == NULL)
	{
		DEBUG_MSG("SD request dma lli failed!\n");
		return 1;
	}*/
	sdmmc_info->lli = sdmmc_dma_lli[sdmmc_info->ChipSel - CHIP0];
	
   register_channel_IRQHandler (SdDmaCh, 
										  sdmmc_dma_transfer_over_IRQHandler_lli, 
										  sdmmc_info,
										  //(1<< BLOCK_INT_BIT)|(1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  (1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  );
	
	int i;
	UINT32 left = len;
	dma_addr_t dst_addr = (dma_addr_t)data_buf;
	// 创建DMA链表
	dma_addr_t	dar = sdmmc_info->sdmmc_reg_info->SDHC_Control + SDMMC_FIFO;
	struct dw_lli *	lli = &sdmmc_info->lli[0];
	for (i = 0; i < lli_count; i++)
	{
		lli->sar = dst_addr;
		lli->dar = dar;
		if(i != (lli_count - 1))
			lli->llp = (UINT32)&sdmmc_info->lli[i + 1];
		else
			lli->llp = 0;
		lli->ctllo = SD_TX_DMA_CTL_L | (1 << 28);	// LLP_SRC_EN
		if(left >= MAX_BATCH_TRANS_SECTOR*512)
			lli->ctlhi = SD_TX_DMA_CTL_H | (MAX_BATCH_TRANS_SECTOR*512 / 4);
		else
			lli->ctlhi = SD_TX_DMA_CTL_H | (left / 4);
		lli->sstat = 0;
		lli->dstat = 0;
		dst_addr += MAX_BATCH_TRANS_SECTOR*512;
		left -= MAX_BATCH_TRANS_SECTOR*512;
		lli ++;
	}
	
	dma_clr_int(SdDmaCh);
	dma_cfg_channel(SdDmaCh, 
					sdmmc_info->lli[0].sar,
					sdmmc_info->lli[0].dar,
					(UINT32)(&sdmmc_info->lli[0])&0xfffffffc,
					sdmmc_info->lli[0].ctllo,
					sdmmc_info->lli[0].ctlhi,
					SD_TX_DMA_CFG_L | (SdDmaCh<<5), (SD_TX_DMA_CFG_H_LLI | (sdmmc_info->dma_req<<12)),
					0,
					0);	
	dma_flush_range ((UINT32)&sdmmc_info->lli[0], (UINT32)&sdmmc_info->lli[lli_count]);
#else	
   register_channel_IRQHandler (SdDmaCh, 
										  sdmmc_dma_transfer_over_IRQHandler, 
										  sdmmc_info,
										  //(1<< BLOCK_INT_BIT)|(1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  (1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  );

//CheckPADRegs();
	to_transfer = len;
	//if(to_transfer > (MAX_BATCH_TRANS_SECTOR*512))
	//	to_transfer = (MAX_BATCH_TRANS_SECTOR*512);
	if(to_transfer > (1*512))
		to_transfer = (1*512);
	sdmmc_info->buf = ((unsigned int)data_buf) + to_transfer;
	sdmmc_info->len = len - to_transfer;
	sdmmc_info->is_read_mode = 0;
	sdmmc_info->ret = 0;
	sdmmc_info->SdDmaCh = SdDmaCh;
	dma_clr_int(SdDmaCh);
	dma_cfg_channel(SdDmaCh, 
					(UINT32)data_buf, sdmmc_info->sdmmc_reg_info->SDHC_Control + SDMMC_FIFO,
					0,
					SD_TX_DMA_CTL_L,SD_TX_DMA_CTL_H|(to_transfer/4),
					SD_TX_DMA_CFG_L|(SdDmaCh<<5),(SD_TX_DMA_CFG_H|(sdmmc_info->dma_req<<12)),
					0,0);
#endif
	
	sdmmc_info->reinit = 0;
	sdmmc_info->debug_flag = 0;
	
	// 16*512字节传输最大超时1秒
	sdmmc_info->crc_error = 0;
	sdmmc_info->write_no_crc = 0;
	DmaTime = 2000 * (len / (MAX_BATCH_TRANS_SECTOR*512));
	if(DmaTime < 100)
		DmaTime = 100;
	else if(DmaTime >2000)
		DmaTime = 2000;
	//if(DmaTime > 500)
	//	DmaTime = 500;
	dma_start_channel(SdDmaCh);
	
	// 超时
	//ticket_to_timeout = XM_GetTickCount() + 2000;
	ticket_to_timeout = XM_GetTickCount() + 5000;
	
	while(!rst)
	{
		//unsigned int rintsts;
		int finished = Dma_TfrOver_PendSemEx (SdDmaCh, DmaTime);
		intsts = SDHC_REG_READ32(sdmmc_info,SDMMC_RINTSTS);
		if(finished == 1)
		{
			// 检查DMA执行的结果
			if(sdmmc_info->ret == 2 && sdmmc_info->len)
			{
				// DAT0 BUSY
				rst = 0;
				do 
				{
					// 等待卡DAT0空闲
					if(!(SDHC_REG_READ32(sdmmc_info, SDMMC_STATUS) & 0x00000200))
					{
						break;
					}
					if(XM_GetTickCount() >= ticket_to_timeout)
					{
						XM_printf ("DAT0 busy timeout\n");
						rst = 1;			// 标记超时
						break;
					}
				} while (1);
				if(rst == 0)
				{
					// 继续DMA传输
					XM_printf ("continue dma, left=%d\n", sdmmc_info->len);
					sdmmc_info->ret = 0;
					dma_start_channel(SdDmaCh);
					continue;
				}
			}
			else if(sdmmc_info->ret)
				rst = 1;
			else if(intsts & (1 << 15))	// write no CRC (EBE)
			{
				DEBUG_MSG("Write DMA err, write no CRC! RINTSTS=0x%08x\n", intsts);
				//rst = 1;
				sdmmc_info->write_no_crc = 1;
			}
			break;
		}
		else if(finished == 0)	// timeout
		{
			// 检查SD卡是否存在
			/*
			//DEBUG_MSG ("sd_write_dma ch(%d) timeout, time=%d, len=%d\n", sdmmc_info->ChipSel, DmaTime, len);
			if((SDCard.Present == 0) && (sdmmc_info->ChipSel == CHIP0))
			{
				DEBUG_MSG("Card 0 Removed!!!!!!!\n");
				rst = 1;
				break;
			}
#if SDMMC_DEV_COUNT > 1
			else if((SDCard1.Present == 0) && (sdmmc_info->ChipSel == CHIP1))
			{
				DEBUG_MSG("Card 1 Removed!!!!!!!\n");
				rst = 1;
				break;
			}
#endif
			*/
			if(!is_card_insert(sdmmc_info))
			{
				DEBUG_MSG("Card %d Removed!!\n", sdmmc_info->ChipSel);
				rst = 1;
				break;
			}
			else
			{
				if(intsts & (1 << 15))	// write no CRC (EBE)
				{
					DEBUG_MSG("Write DMA err, write no CRC! RINTSTS=0x%08x\n", intsts);
					sdmmc_info->write_no_crc = 1;
					//rst = 1;
					//break;
				}
				//DEBUG_MSG("Write DMA err, timeout! RINTSTS=0x%08x\n", intsts);
				//rst = 1;
				//break;
			}
			
			/*
			if(SDHC_REG_READ32(sdmmc_info,SDMMC_RINTSTS) & (1<<9))
			{
				// 其他异常
				DEBUG_MSG("Write DMA err!!!!!!!\n");
				rst = 1;
				break;
			}*/
			
			// 1) 20170707 发现卡快速插拔时出现异常, DMA执行未能结束. 加入超时机制
			// 2) 20170716 发现PNY卡经常出现卡写入一段时间后, DMA执行未能结束, 而且PNY卡不再应答其他命令, 
			//             此时对卡重新执行INIT后, 卡可以继续正常读写操作
			// 超时无应答后, 重新执行卡复位/卡初始化操作
			if(XM_GetTickCount() >= ticket_to_timeout)
			{
				DEBUG_MSG("SDMMC Write DMA timeout\n");
				sdmmc_info->reinit = 1;		// 标记卡需要复位
				rst = 1;			// 标记超时
				break;
			}
		}
		/*
		rintsts = SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS);
		// End bit error (bit 15) – End bit was not received during data reception or for a write
		//		operation; a CRC error is indicated by the card.
		if(rintsts & (1 << 15))
		{
			DEBUG_MSG("Write DMA err, End bit error!!!!!!!\n");
			rst = 1;
			break;						
		}
		*/
	}
	
	sdmmc_info->debug_flag = 0;
	
	dma_clr_int(SdDmaCh);
	//card_dma_detect_ch_disable(sdmmc_info, SdDmaCh);
	dma_release_channel(SdDmaCh);
#ifdef _SDMMC_USE_LLI_DMA_
	//kernel_free (sdmmc_info->lli);
	sdmmc_info->lli = 0;
#endif
	
	if(SDHC_REG_READ32(sdmmc_info,SDMMC_RINTSTS) & RINTSTS_DCRC)
	{
		sdmmc_info->crc_error = 1;
	}

	return rst;	
	//busy_delay(1000000);   
}
#endif


#ifndef _NEW_SDMMC_DMA_DRIVER_
static INT32 sd_read_dma(SDMMC_INFO * sdmmc_info, UINT32 *data_buf,UINT32  len)
{
	UINT32 SdDmaCh;
	UINT32 DmaRequestTime = 0;
//	UINT32 DmaTime = 0;
	INT32 rst = 0;
      
#if 0
	do{
		SdDmaCh = dma_request_dma_channel(SDMMC_DMA_FAVORITE_CHANNEL);
		if((DmaRequestTime++)>50000)
		{
			XM_printf ("kernel exception, SD read request dma failed!\n");
			return 1;
		}
	}while(SdDmaCh==ALL_CHANNEL_RUNNING);
#else
	SdDmaCh = sdmmc_dma_channel[sdmmc_info->ChipSel - CHIP0];
#endif

//CheckPADRegs();

	dma_clr_int(SdDmaCh);
	dma_cfg_channel(SdDmaCh, 
					sdmmc_info->sdmmc_reg_info->SDHC_Control + SDMMC_FIFO,
					(UINT32)data_buf,
					0,
					SD_RX_DMA_CTL_L,
					SD_RX_DMA_CTL_H|len/4,
					SD_RX_DMA_CFG_L|(SdDmaCh<<5), (SD_RX_DMA_CFG_H|(sdmmc_info->dma_req<<7)),
					0,
					0);
	dma_start_channel(SdDmaCh);
	
	while(!dma_transfer_over(SdDmaCh))
	{
		/*
		if((SDCard.Present == 0) && (sdmmc_info->ChipSel == CHIP0))
		{
			DEBUG_MSG("Card Removed!!!!!!!\n");
			rst = 1;
			break;
		}
#if SDMMC_DEV_COUNT > 1
		if((SDCard1.Present == 0) && (sdmmc_info->ChipSel == CHIP1))
		{
			DEBUG_MSG("Card Removed!!!!!!!\n");
			rst = 1;
			break;
		}
#endif
		*/
		if(!is_card_insert(sdmmc_info))
		{
			// card removed
			DEBUG_MSG("Card Removed!\n", sdmmc_info->ChipSel);	
			rst = 1;
			break;
		}
		
		if(SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS) & (1<<9))
		{
			DEBUG_MSG("Read DMA err!!!!!!!\n");
			rst = 1;
			break;
		}
	}

	dma_clr_int(SdDmaCh);
	if(card_dma_detect_ch_disable(sdmmc_info, SdDmaCh) != 0)
	{
		rst = 1;
	}
	dma_release_channel(SdDmaCh);

	return rst;	
	//busy_delay(1000000);
	
}
#endif


INT32 sd_mmc_write(SDMMC_INFO * sdmmc_info, UINT32 start_block,UINT32 blk_count,UINT32 *data_buf)

{
	INT32 i,j,k;
	UINT32  ret;
//	INT32 timeout = 0;
	INT32 result = SDMMC_RW_SUCCES;
	UINT32 val;
	UINT32 count_sd = 0;

	select_sd_pad(sdmmc_info);

	//val = rSDMMC_TMOUT ;
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_TMOUT);
	val &=~0xFFFFFF00;
	val |= sdmmc_info->WriteTimeOutCycle<<8;
	//rSDMMC_TMOUT = val;
//	SDHC_REG_WRITE32(sdmmc_info, SDMMC_TMOUT, val);
  	//rSDMMC_BLKSIZ = block_len;
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BLKSIZ, block_len);
  	//rSDMMC_BYTCNT = block_len*blk_count;
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BYTCNT, block_len*blk_count);

  	if(sdmmc_info->Capacity == Standard)
        start_block = start_block*block_len;

  	//rSDMMC_CMDARG = start_block;       // data address
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, start_block);
  	//rSDMMC_CMD = CMD25_WRITE_MUL;         // CMD25    --- write multipe block 0x80001759 CMD25_WRITE_MUL
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD25_WRITE_MUL);

	if(sd_command_done(sdmmc_info))
	{
		result = SDMMC_RW_CMDTIMEROUT;
		goto ERR_HANDLER;
	}


//CheckPADRegs();

	for(j = 0; j < blk_count; j ++)
	{
     		for (i = 0; i < block_len/128; i ++)
		{
        		for(k = 0; k < 32; k ++)
			{
                		//rSDMMC_DATA   = *data_buf++;
                		SDHC_REG_WRITE32(sdmmc_info, SDMMC_DATA, *data_buf);
				data_buf++;
			}
			//fifo_tran_over(sdmmc_info);
			do
			{
				count_sd ++;
				if(count_sd == 5000000)
				{
					count_sd = 0;
					DEBUG_MSG("SDMMC_STATUS is error and ChIP is %d\n", sdmmc_info->ChipSel);
				}
			
				if(SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS) & (1<<9))
				{
				    	DEBUG_MSG("SD card i/o read not finished.\n");
					result = SDMMC_RW_TIMEOUT;
					goto ERR_HANDLER;
				}
				/*
				if((SDCard.Present == 0) && (sdmmc_info->ChipSel == CHIP0))
				{
					DEBUG_MSG("Card(0) Removed!!!!!!!\n");
					result = SDMMC_RW_CARDREMOVED;
					goto ERR_HANDLER;
				}
#if SDMMC_DEV_COUNT > 1
				if((SDCard1.Present == 0) && (sdmmc_info->ChipSel == CHIP1))
				{
					DEBUG_MSG("Card(1) Removed!!!!!!!\n");
					result = SDMMC_RW_CARDREMOVED;
					goto ERR_HANDLER;
				}
#endif
				*/
				if(!is_card_insert(sdmmc_info))
				{
					DEBUG_MSG("Card(%d) Removed!!!!!!!\n", sdmmc_info->ChipSel);
					result = SDMMC_RW_CARDREMOVED;
					goto ERR_HANDLER;					
				}
				
			}
			while (!(SDHC_REG_READ32(sdmmc_info,SDMMC_STATUS) & 0x00000004));
		}
              busy_data(sdmmc_info);
    
  	}

  	ret = data_tran_over(sdmmc_info);

	if(ret)
	{
		DEBUG_MSG("SD%d card write pages not finished.\n", sdmmc_info->ChipSel);
		result = SDMMC_RW_TIMEOUT;
		goto ERR_HANDLER;
	}
	//rSDMMC_CMD = CMD12_STOP_STEARM;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD12_STOP_STEARM);
	if(sd_command_done(sdmmc_info))
	{
		result = SDMMC_RW_CMDTIMEROUT;
		goto ERR_HANDLER;
	}
	
  	if(busy_data(sdmmc_info))
  	{
		result = SDMMC_RW_TIMEOUT;
  	}

ERR_HANDLER:
	
  	return result;
}


INT32 sd_mmc_read(SDMMC_INFO * sdmmc_info, UINT32 start_block,UINT32 blk_count,UINT32 *data_buf)

{
 	INT32 i,j;
	UINT32 ret;
//	UINT32 timeout;
	INT32 result = SDMMC_RW_SUCCES;
	UINT32 val;
	UINT32 count_sd = 0;

	UINT32 start_addr = (UINT32)data_buf;
	UINT32 end_addr = start_addr + 512 * blk_count;
	
	//dma_inv_range (start_addr, end_addr);

	select_sd_pad(sdmmc_info);
	//val = rSDMMC_TMOUT ;
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_TMOUT);
	val &=~0xFFFFFF00;
	val |= sdmmc_info->ReadTimeOutCycle<<8;
	//rSDMMC_TMOUT = val;
//	SDHC_REG_WRITE32(sdmmc_info, SDMMC_TMOUT, val);
  	//rSDMMC_BLKSIZ = block_len;
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BLKSIZ, block_len);
  	//rSDMMC_BYTCNT = blk_count*block_len;//here 128 need adjust bases sd card information
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BYTCNT, blk_count*block_len);
    	if(sdmmc_info->Capacity == Standard)
	       	start_block = start_block*block_len;
  	//rSDMMC_CMDARG = start_block;   //start address
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, start_block);
  	//rSDMMC_CMD = CMD18_READ_MUL;      //CMD18   --- read multiple block CMD18_READ_MUL
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD18_READ_MUL);

	if(sd_command_done(sdmmc_info))
	{
		result = SDMMC_RW_CMDTIMEROUT;
		goto ERR_HANDLER;
	}

//CheckPADRegs();

	for(i = 0; i < (blk_count*(block_len/128)); i ++)
  	{
	    //confi_fifo_full();

		do
		{
			count_sd ++;
			if(count_sd == 5000000)
			{
				count_sd = 0;
				DEBUG_MSG("SDMMC_STATUS is error and ChIP is %d\n", sdmmc_info->ChipSel);
			}
		
			if(SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS) & (1<<9))
			{
			    	DEBUG_MSG("SD card i/o read not finished.\n");
				result = SDMMC_RW_TIMEOUT;
				goto ERR_HANDLER;
			}
			/*
			if((SDCard.Present == 0) && (sdmmc_info->ChipSel == CHIP0))
			{
				DEBUG_MSG("Card(0) Removed!!!!!!!\n");
				result = SDMMC_RW_CARDREMOVED;
				goto ERR_HANDLER;
			}
#if SDMMC_DEV_COUNT > 1
			if((SDCard1.Present == 0) && (sdmmc_info->ChipSel == CHIP1))
			{
				DEBUG_MSG("Card(1) Removed!!!!!!!\n");
				result = SDMMC_RW_CARDREMOVED;
				goto ERR_HANDLER;
			}	
#endif
			*/
			if(!is_card_insert(sdmmc_info))
			{
				DEBUG_MSG("Card(%d) Removed!!!!!!!\n", sdmmc_info->ChipSel);
				result = SDMMC_RW_CARDREMOVED;
				goto ERR_HANDLER;					
			}
			
		}while (!(SDHC_REG_READ32(sdmmc_info, SDMMC_STATUS)  & 0x00000008));

    	      	for (j = 0; j < 32; j ++)
        	{
	      	 	*data_buf ++ = SDHC_REG_READ32(sdmmc_info, SDMMC_DATA);
	  	}
	}
	
  	ret = data_tran_over(sdmmc_info);

  	if(ret)
	{
		DEBUG_MSG("SD card read pages not finished11. CHIP is %d\n", sdmmc_info->ChipSel);
		result =  SDMMC_RW_TIMEOUT;
		goto ERR_HANDLER;
  	}

  	if(busy_data(sdmmc_info))
	{
		DEBUG_MSG("SD card read pages not finished22.CHIP is %d\n", sdmmc_info->ChipSel);
		result =  SDMMC_RW_TIMEOUT;
  	}
	
ERR_HANDLER:
	//dma_flush_range (start_addr, end_addr);

	
	return result;
}

//#define MAX_BATCH_TRANS_SECTOR		16
INT32 sd_mmc_read_dma (SDMMC_INFO * sdmmc_info, UINT32 start_block,UINT32 blk_count,UINT32 *data_buf)

{	
#ifdef _NEW_SDMMC_DMA_DRIVER_
#else
	INT32 i,j,k;
#endif
	UINT32 ret;
//	UINT32 timeout;
	INT32 result = SDMMC_RW_SUCCES;
	UINT32 val;
	int crc_error = 0;
	
#ifdef _SDMMC_USE_LLI_DMA_
#else	
	unsigned int addr_begin, addr_end;
	//DEBUG_MSG("sd_mmc_read_dma. start_block=0x%08x, blk_count=0x%08x\n", start_block, blk_count);


	addr_begin = (unsigned int )data_buf;
	addr_end = ((unsigned int )data_buf) + blk_count*512;
	dma_inv_range (addr_begin, addr_end);
#endif
	
//	rSDMMC_CTRL |= (1<<5);
#ifdef _NEW_SDMMC_DMA_DRIVER_
#else
	i = blk_count/MAX_BATCH_TRANS_SECTOR;
	j = blk_count%MAX_BATCH_TRANS_SECTOR;
#endif
	//rSDMMC_FIFOTH |= (3<<28)|(15<<16)|16; //burst size 8, should be same as DWA_DMA 's SRC/DEST SIZE
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_FIFOTH);
	//val |= (3<<28)|(15<<16)|16;
	val = (3<<28)|(15<<16)|16;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_FIFOTH, val);

	
  	//dma_inv_range((unsigned int )data_buf, ((unsigned int )data_buf) + blk_count*512);
	//dma_flush_range ((unsigned int )data_buf, ((unsigned int )data_buf) + blk_count*512);

	select_sd_pad(sdmmc_info);
	
	//val = rSDMMC_TMOUT ;
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_TMOUT);
	val &=~0xFFFFFF00;
	val |= sdmmc_info->ReadTimeOutCycle<<8;
	//rSDMMC_TMOUT = val;
//	SDHC_REG_WRITE32(sdmmc_info, SDMMC_TMOUT, val);
	
  	//rSDMMC_BLKSIZ = block_len;
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BLKSIZ, block_len);
  	//rSDMMC_BYTCNT = blk_count*block_len;//here 128 need adjust bases sd card information
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BYTCNT, blk_count*block_len);
    	if(sdmmc_info->Capacity == Standard)
       	start_block = start_block*block_len;
  	//rSDMMC_CMDARG = start_block;   //start address
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, start_block);
  	//rSDMMC_CMD = CMD18_READ_MUL;      //CMD18   --- read multiple block CMD18_READ_MUL
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD18_READ_MUL);

	//if(sd_command_done(sdmmc_info))
	if(sd_command_done_no_clear_RINTSYS(sdmmc_info))
	{
		//DEBUG_MSG("SD (%d) card read dma error. send CMD18_READ_MUL NG\n", sdmmc_info->ChipSel);
		result = SDMMC_RW_CMDTIMEROUT;
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);
		return result;
		//goto ERR_HANDLER;
	}


#ifdef _NEW_SDMMC_DMA_DRIVER_
	sdmmc_info->crc_error = 0;
	ret = sd_read_dma_new (sdmmc_info, data_buf, blk_count * 512);
	crc_error = sdmmc_info->crc_error;
	sdmmc_info->crc_error = 0;
   if(ret)
	{
		DEBUG_MSG("SD card read dma not finished11. CHIP is %d, start_block=0x%08x, blk_count=0x%08x\n", sdmmc_info->ChipSel, start_block, blk_count);
		result = SDMMC_RW_DMAREAD_FAIL;
		goto ERR_HANDLER;		
	}
#else
   // Arkmicro 1960 version
	for(k = 0; k < i; k ++)
	{
		if(sd_read_dma(sdmmc_info,data_buf,block_len*MAX_BATCH_TRANS_SECTOR))
		{
			DEBUG_MSG("SD card read dma not finished11. CHIP is %d\n", sdmmc_info->ChipSel);
			result = SDMMC_RW_DMAREAD_FAIL;
			goto ERR_HANDLER;
		}
		
		data_buf += MAX_BATCH_TRANS_SECTOR*128;
	}

  	if(j != 0)
 	{
 		if(sd_read_dma(sdmmc_info,data_buf,block_len*j))
 		{
			DEBUG_MSG("SD card read dma not finished22.CHIP is %d\n", sdmmc_info->ChipSel);
			result = SDMMC_RW_DMAREAD_FAIL;
			goto ERR_HANDLER;
		}

 	}
#endif

  	ret = data_tran_over(sdmmc_info);
	
  	if(ret)
	{
		DEBUG_MSG("SD card read dma not finished11. CHIP is %d\n", sdmmc_info->ChipSel);
		result =  SDMMC_RW_TIMEOUT;
		goto ERR_HANDLER;
  	}

	// 20170818 修复金士顿64GB卡读写异常的缺陷
  	if(busy_data(sdmmc_info))
	{
		DEBUG_MSG("SD card read dma not finished22.CHIP is %d\n", sdmmc_info->ChipSel);
		result =  SDMMC_RW_TIMEOUT;
  	}
	
#ifdef _SDMMC_USE_LLI_DMA_
	
#else
	dma_inv_range (addr_begin, addr_end);
#endif
	
	if(result == SDMMC_RW_SUCCES && crc_error)
	{
		// 检查crc错误, 标记失败
		XM_printf ("DMA READ CRC ERROR\n");
		result = SDMMC_RW_DMAREAD_FAIL;
		
		// 清除错误
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);		
	}
	return result;
	
ERR_HANDLER:

	val = SDHC_REG_READ32(sdmmc_info, SDMMC_STATUS);
	if(val & ((0x7fff<<16)|(0x3<<9)|(0xf<<4)))
		DEBUG_MSG("SD read data status is error status is 0x%x.\n", val);
	
	// 20170511 fix bug
	val = SDHC_REG_READ32(sdmmc_info,SDMMC_RINTSTS);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);
	if(val & RINTSTS_DRTO)
	{
		// After the NACIO timeout, the application must abort the command by sending the CCSD and STOP commands, or the STOP command.
		// bit 9 – Data read timeout (DRTO)/Boot Data Start (BDS)
		//DEBUG_MSG("sd_mmc_read_dma. CMD12_STOP_STEARM send\n");
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD12_STOP_STEARM);
		if(sd_command_done(sdmmc_info))
		{
			DEBUG_MSG("sd_mmc_read_dma. CMD12_STOP_STEARM send NG\n");
		}		
	}
	if(val & RINTSTS_SBE)
	{
		// Data start bit error – During a 4-bit or 8-bit read-data transfer, if SBE occurs, the
		// application/driver must issue CMD12 for SD/MMC cards and CMD52 for SDIO card in order to
		// come out of the error condition. After CMD done is received, the application/driver must reset
		// IDMAC and CIU (if required) to clear the condition. FIFO reset must be done before issuing any
		// data transfer commands in general. Register CNTRL bits 2:0 control these resets:	
		//DEBUG_MSG("sd_mmc_read_dma. CMD12_STOP_STEARM send\n");
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD12_STOP_STEARM);
		if(sd_command_done(sdmmc_info))
		{
			DEBUG_MSG("sd_mmc_read_dma. CMD12_STOP_STEARM send NG\n");
		}		

		fifo_dma_reset (sdmmc_info);
	}
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);

	
#ifdef _SDMMC_USE_LLI_DMA_
	
#else
	dma_inv_range (addr_begin, addr_end);
#endif
	return result;
}

static INT32 sd_mmc_write_dma(SDMMC_INFO * sdmmc_info, UINT32 start_block,UINT32 blk_count,UINT32 *data_buf)
{
#ifdef _NEW_SDMMC_DMA_DRIVER_
	INT32 k;
#else
	INT32 i,j,k;
#endif
	UINT32  ret;
//	INT32 timeout = 0;
	INT32 result = SDMMC_RW_SUCCES;
	UINT32 val;
	int fatal_busy_error = 0;		//  1 means card busy forever
	
	
	// 此处加入1ms延时可以改善TF卡写入的兼容性. 
	// 1) PNY卡在格式化时报告DATA BUSY异常, 加入延时后, 不再报告DATA BUSY异常, 可以正常写入. 
	//   上述DATA BUSY异常出现时, 读取卡状态(CMD13, Card Status)及检查controller STATUS, 均未发现异常.
	// 2) 其他品牌的TF卡暂时未发现该类型错误
	// 3) 1ms延时基本不影响现在正常的卡写入过程, 一秒内卡写入的次数有限, 不超过20次.
	// 20170818 与busy_data处理相关, 此处无需延时
	// OS_Delay (1);

	//DEBUG_MSG("sd_mmc_write_dma. start_block=0x%08x, blk_count=0x%08x\n", start_block, blk_count);

//	rSDMMC_CTRL |= 0x01000020;
  	//rSDMMC_FIFOTH |= (2<<28)|8; //wrong config. burst size 8, should be same as DWA_DMA 's SRC/DEST SIZE
  	//rSDMMC_FIFOTH |= (3<<28)|16; //wrong config. burst size 8, should be same as DWA_DMA 's SRC/DEST SIZE
  	val = SDHC_REG_READ32(sdmmc_info, SDMMC_FIFOTH);
	// val |= (3<<28)|16;
	val = (3<<28)|16;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_FIFOTH, val);
  	//rSDMMC_FIFOTH |= (2<<28)|(7<<16)|8;
#ifdef _NEW_SDMMC_DMA_DRIVER_
#else
  	i = blk_count/MAX_BATCH_TRANS_SECTOR;
	j = blk_count%MAX_BATCH_TRANS_SECTOR;
#endif
	
	//dma_clean_range((UINT32)data_buf, (UINT32)data_buf+ blk_count*512);
	//dma_flush_range((UINT32)data_buf, (UINT32)data_buf+ blk_count*512);

	//select_sd_pad(sdmmc_info);

	//val = rSDMMC_TMOUT ;
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_TMOUT);
	val &=~0xFFFFFF00;
	val |= sdmmc_info->WriteTimeOutCycle<<8;
	//rSDMMC_TMOUT = val;
//	SDHC_REG_WRITE32(sdmmc_info, SDMMC_TMOUT, val);
	
  	//rSDMMC_BLKSIZ = block_len;
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BLKSIZ, block_len);
  	//rSDMMC_BYTCNT = block_len*blk_count;
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BYTCNT, block_len*blk_count);

  	if(sdmmc_info->Capacity == Standard)
        start_block = start_block*block_len;
	
  	//rSDMMC_CMDARG = start_block;       // data address
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, start_block);
  	//rSDMMC_CMD = CMD25_WRITE_MUL;         // CMD25    --- write multipe block 0x80001759 CMD25_WRITE_MUL
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD25_WRITE_MUL);

	//if(sd_command_done(sdmmc_info))
	if(sd_command_done_no_clear_RINTSYS(sdmmc_info))
	{
		//DEBUG_MSG("SD card write dma failed. send CMD25_WRITE_MUL NG\n");
		result = SDMMC_RW_CMDTIMEROUT;
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);
		return result;
		//goto ERR_HANDLER;
	}
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0);
	if(val & ((1 << 31)|(1<<30)|(1<<29)|(1<<23)|(1<<22)|(1<<21)|(1<<20)|(1<<19)))
	{
		XM_printf ("SD CMD25_WRITE_MUL RESP0=0x%08x\n", val);
	}

#ifdef _NEW_SDMMC_DMA_DRIVER_
	if(sd_write_dma_new (sdmmc_info, data_buf, blk_count * 512))
	{
		DEBUG_MSG("SD card write dma not finished11. CHIP is %d\n", sdmmc_info->ChipSel);
		result = SDMMC_RW_DMAREAD_FAIL;
		goto ERR_HANDLER;		
	}
	// 20170818 修复金士顿64GB卡读写异常的缺陷
	if(busy_data(sdmmc_info))
	{
		result = SDMMC_RW_TIMEOUT;
		DEBUG_MSG("sd_mmc_write_dma failed, busy_data NG\n");
		goto ERR_HANDLER;	
	}
#else
 	for(k = 0; k < i; k ++)
	{
//		timeout = 0;
		if(sd_write_dma(sdmmc_info,data_buf, block_len*MAX_BATCH_TRANS_SECTOR))
		{
			DEBUG_MSG("sdmmc write dma not finished11\n");
			result = SDMMC_RW_DMAWRITE_FAIL;
			goto ERR_HANDLER;
		}
		busy_data(sdmmc_info);
  		data_buf += MAX_BATCH_TRANS_SECTOR*128;
	}
	if(j != 0)
	{
//		timeout = 0;
		if(sd_write_dma(sdmmc_info,data_buf,block_len*j))
		{
			DEBUG_MSG("sdmmc write dma not finished22\n");
			result = SDMMC_RW_DMAWRITE_FAIL;
			goto ERR_HANDLER;
		}
		busy_delay(2000);
		busy_data(sdmmc_info);
	}
#endif
  	ret = data_tran_over(sdmmc_info);

	if(ret)
	{
		DEBUG_MSG("SD%d card write pages not finished.\n", sdmmc_info->ChipSel);
		result = SDMMC_RW_TIMEOUT;
		goto ERR_HANDLER;
	}
	
	// a) The last busy in any write operation up to 500ms including single and multiple block write.
	// b) When multiple block write is stopped by CMD12, the busy from the response of CMD12 is up to 500ms.
	//rSDMMC_CMD = CMD12_STOP_STEARM;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD12_STOP_STEARM);
	if(sd_command_done(sdmmc_info))
	{
		result = SDMMC_RW_CMDTIMEROUT;
		DEBUG_MSG("SD%d card write error. CMD12_STOP_STEARM send NG\n", sdmmc_info->ChipSel);
		goto ERR_HANDLER;
	}
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0);
	if(val & ((1 << 31)|(1<<30)|(1<<29)|(1<<23)|(1<<22)|(1<<21)|(1<<20)|(1<<19)))
	{
		XM_printf ("SD CMD12_STOP_STEARM RESP0=0x%08x, start_block=%d, blk_count=%d\n", val, start_block, blk_count);
	}
	
	// 检测Busy Complete Interrupt, 按照上面b)的说明，最大500ms
  	if(busy_data(sdmmc_info))
  	{
		result = SDMMC_RW_TIMEOUT;
		DEBUG_MSG("SD%d card write error. busy_data NG\n", sdmmc_info->ChipSel);
		fatal_busy_error = 1;
		goto ERR_HANDLER;
  	}
	
	if(sdmmc_info->crc_error)
	{
		sdmmc_info->crc_error = 0;		// 清除crc标志
		DEBUG_MSG("SD%d card write failed. data crc error\n", sdmmc_info->ChipSel);
		result = SDMMC_RW_TIMEOUT;
	}
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_STATUS);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);

	return result;
	
ERR_HANDLER:
	
	sdmmc_info->crc_error = 0;		// 清除crc标志
	
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_STATUS);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);

	
	if(val & ((0x7fff<<16)|(0x3<<9)|(0xf<<4)))
	{
		DEBUG_MSG("SD write data status is error status is 0x%x.\n", val);
	}
	
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD12_STOP_STEARM);
	if(sd_command_done(sdmmc_info))
	{
		DEBUG_MSG("sd_mmc_write_dma. CMD12_STOP_STEARM send NG\n");
	}
	else
	{
		DEBUG_MSG("sd_mmc_write_dma. CMD12_STOP_STEARM send OK\n");
	}
	
	if(sdmmc_info->reinit)
	{
		XM_printf ("sd_mmc_write_dma fatal error, re-init\n");
		fatal_busy_error = 1;
		sdmmc_info->reinit = 0;
	}
	fifo_dma_reset (sdmmc_info);
		
#if 0
	if(fatal_busy_error)
	{
		// 检查卡是否已拔出. 若是, 不发出REINIT事件
		if(is_card_insert(sdmmc_info))
		{
			// 需要卡重新识别(发送CMD0指令来复位SD卡)
			// 将卡状态置为不存在, 以便文件系统层快速退出, 重新执行REINIT指令
			//sdmmc_info->card_present = 0;
			
			// 检查卡是否正在执行REINIT, 若是, 阻止REINIT消息递归投递
			if(sdmmc_info->do_reinit == 0)
			{
				PostCardEvent (SDCARD_REINIT, sdmmc_info->ChipSel);
			}
		}
	}
#endif

	return result;
}

static void sdmmc_dma_transfer_over_intr_lli (UINT32 ulIRQFactors, UINT32 channel, void *channel_private_data)
{
   int ret = 0;
   SDMMC_INFO * sdmmc_info = channel_private_data;
#ifdef DEBUG
   assert (sdmmc_info->SdDmaCh == channel);
#endif
	if(ulIRQFactors & ERR_INT_BIT)
	{
		// This interrupt is generated when an ERROR response is received from an AHB slave on the
		// HRESP bus during a DMA transfer. 
		// In addition, the DMA transfer is cancelled and the channel is disabled.
		// DMA 异常
		DEBUG_MSG ("sdmmc ERR_INT_BIT\n");
		ret = 1;
	}
	dma_clr_int(channel);

	// DMA结束
	sdmmc_info->ret = ret;
	
#ifdef _XMSYS_SDMMC_EVENT_DRIVEN_
	// 发送DMA传输结束事件
	sdmmc_dma_transmit_stop (sdmmc_info->ChipSel, (ret == 0) ? SDMMC_EVENT_WRITE_SUCCESS : SDMMC_EVENT_WRITE_DMA_ERROR);
#endif
}

static int send_command_until_repeat_count_of_failed (SDMMC_INFO * sdmmc_info, UINT32 cmd, UINT32 arg)
{
	int loop_times = REPEAT_COUNT_OF_FAILED_COMMAND;
	while(loop_times > 0)
	{
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, arg);
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, cmd);
		if(sd_command_done(sdmmc_info) == 0)
		{
			return 0;
		}
		loop_times --;
	}
	return 1;
}

static INT32 sd_mmc_write_dma_event_driven (SDMMC_INFO * sdmmc_info, UINT32 start_block, UINT32 blk_count, UINT32 *data_buf)
{
	int loop_times;
	UINT32 val;
	INT32 result = SDMMC_RW_SUCCES;
	UINT32 SdDmaCh;
	UINT32  len;
	unsigned int curr_ticket;
	unsigned int busy_ticket;
	int lli_count;
	int err_cause;
	int i;
	UINT32 left;
	dma_addr_t dst_addr;
	dma_addr_t	sar;
	struct dw_lli *	lli;	
	//val = SDHC_REG_READ32(sdmmc_info, SDMMC_FIFOTH);
	val = (3<<28)|(15<<16)|16;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_FIFOTH, val);
	
	len = blk_count * 512;
	
	dma_clean_range ((UINT32)data_buf, (UINT32)data_buf + blk_count * 512);
	
	select_sd_pad(sdmmc_info);
	
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BLKSIZ, block_len);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BYTCNT, block_len*blk_count);
  	
	if(sdmmc_info->Capacity == Standard)
		start_block = start_block*block_len;
	
	if(send_command_until_repeat_count_of_failed(sdmmc_info, CMD25_WRITE_MUL, start_block))
	{
		XM_printf ("SD_WRITE Failed, send_command(CMD25_WRITE_MUL) NG\n");
		result = SDMMC_RW_CMDTIMEROUT;
		goto ERR_HANDLER;		
	}
	
	// 复位事件
	OS_EVENT_Reset (&sdmmc_event[sdmmc_info->ChipSel - CHIP0]);
	// 检查卡是否拔出。复位事件可能清除卡拔出事件
	if(SDHC_REG_READ32(sdmmc_info, SDMMC_CDETECT) & 0x1)
	{
		XM_printf ("SD_WRITE Failed, card removed\n");
		result = SDMMC_RW_CMDTIMEROUT;
		goto ERR_HANDLER;		
	}
	
	SdDmaCh = sdmmc_dma_channel[sdmmc_info->ChipSel - CHIP0];
	sdmmc_info->SdDmaCh = SdDmaCh;
	lli_count = (len + MAX_BATCH_TRANS_SECTOR*512 - 1) / (MAX_BATCH_TRANS_SECTOR*512);
	sdmmc_info->lli = sdmmc_dma_lli[sdmmc_info->ChipSel - CHIP0];
   register_channel_IRQHandler (SdDmaCh, 
										  sdmmc_dma_transfer_over_intr_lli, 
										  sdmmc_info,
										  (1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  );
	left = len;
	dst_addr = (dma_addr_t)data_buf;
	sar = sdmmc_info->sdmmc_reg_info->SDHC_Control + SDMMC_FIFO;
	lli = &sdmmc_info->lli[0];
	// 创建DMA链表
	for (i = 0; i < lli_count; i++)
	{
		lli->sar = sar;
		lli->dar = dst_addr;
		if(i != (lli_count - 1))
			lli->llp = (UINT32)&sdmmc_info->lli[i + 1];
		else
			lli->llp = 0;
		lli->ctllo = SD_RX_DMA_CTL_L_LLI | (1 << 27);
		if(left >= MAX_BATCH_TRANS_SECTOR*512)
			lli->ctlhi = SD_RX_DMA_CTL_H | (MAX_BATCH_TRANS_SECTOR*512 / 4);
		else
			lli->ctlhi = SD_RX_DMA_CTL_H | (left / 4);
		lli->sstat = 0;
		lli->dstat = 0;
		dst_addr += MAX_BATCH_TRANS_SECTOR*512;
		left -= MAX_BATCH_TRANS_SECTOR*512;
		lli ++;
	}
		
	dma_clr_int(SdDmaCh);
	lli = &sdmmc_info->lli[0];
	dma_cfg_channel(SdDmaCh, 
					lli->sar,
					lli->dar,
					(UINT32)(&sdmmc_info->lli[0])&0xfffffffc,
					lli->ctllo,
					lli->ctlhi,
					SD_RX_DMA_CFG_L|(SdDmaCh<<5), (SD_RX_DMA_CFG_H_LLI|(sdmmc_info->dma_req<<7)),
					0,
					0);	
	dma_flush_range ((UINT32)&sdmmc_info->lli[0], (UINT32)&sdmmc_info->lli[lli_count]);
	//dma_inv_range ((UINT32)data_buf, ((UINT32)data_buf) + len);	
	
	// 开启SDMMC WRITE异常中断
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_INTMASK);
	// val |= RINTSTS_EBE | RINTSTS_DCRC;		// write no CRC (EBE), Negative CRC
	val |= RINTSTS_EBE;		// write no CRC (EBE) 意味SD卡编程出现异常，不再应答。
									// Negative CRC数据传输中存在干扰，导致CRC错误。继续传输，结束后，返回失败要求重传。
	SDHC_REG_WRITE32 (sdmmc_info, SDMMC_INTMASK, val);
	
	dma_start_channel(SdDmaCh);
	// 等待SDMMC事件(DMA传输完毕或其他异常事件)
	OS_EVENT_Wait (&sdmmc_event[sdmmc_info->ChipSel - CHIP0]);

	// 关闭SDMMC WRITE异常中断
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_INTMASK);
	// val &= ~(RINTSTS_EBE | RINTSTS_DCRC);
	val &= ~(RINTSTS_EBE);
	SDHC_REG_WRITE32 (sdmmc_info, SDMMC_INTMASK, val);
	
	dma_clr_int(SdDmaCh);
	//card_dma_detect_ch_disable(sdmmc_info, SdDmaCh);
	dma_release_channel(SdDmaCh);
	sdmmc_info->lli = 0;
	
	err_cause = sdmmc_cause[sdmmc_info->ChipSel - CHIP0];
	switch(err_cause)
	{
		case SDMMC_EVENT_WRITE_SUCCESS:
			// 检查 Negative CRC是否已设置。若是，需要重传
			val = SDHC_REG_READ32(sdmmc_info, SDMMC_RINTSTS);
			if(val & RINTSTS_DCRC)
			{
				result = SDMMC_RW_DMAWRITE_FAIL;
				XM_printf ("SD_WRITE Failed, Negative CRC\n");
			}
			break;
			
		case SDMMC_EVENT_WRITE_DMA_ERROR:
			result = SDMMC_RW_DMAWRITE_FAIL;
			XM_printf ("SD_WRITE Failed, DMA Error\n");
			break;
			
		case SDMMC_EVENT_WRITE_CARD_REMOVE:
			result = SDMMC_RW_CARDREMOVED;
			XM_printf ("SD_WRITE Failed, last busy NG, card removed\n");
			goto ERR_HANDLER;
			
		case SDMMC_EVENT_WRITE_NO_CRC_STATUS:
			result = SDMMC_RW_DMAWRITE_FAIL;
			XM_printf ("SD_WRITE Failed, WRITE_NO_CRC\n");
			break;
			
		default:
			result = SDMMC_RW_DMAWRITE_FAIL;
			XM_printf ("SD_WRITE Failed, unknown Error\n");
			break;				
	}
		
	
	// a) The last busy in any write operation up to 500ms including single and multiple block write.
	// b) When multiple block write is stopped by CMD12, the busy from the response of CMD12 is up to 500ms.
	if(send_command_until_repeat_count_of_failed(sdmmc_info, CMD12_STOP_STEARM, 0))
	{
		result = SDMMC_RW_CMDTIMEROUT;
		XM_printf ("SD_WRITE Failed, send CMD12_STOP_STEARM NG\n");
		goto ERR_HANDLER;		
	}
	
	// 等待last busy，最大500ms
	curr_ticket = XM_GetTickCount();
	while( SDHC_REG_READ32(sdmmc_info, SDMMC_STATUS) & 0x00000200 )
	{
		if( SDHC_REG_READ32(sdmmc_info, SDMMC_CDETECT) & (1 << (sdmmc_info->ChipSel - CHIP0)) )
		{
			// card removed
			result = SDMMC_RW_CARDREMOVED;
			XM_printf ("SD_WRITE Failed, last busy NG, card removed\n");
			break;
		}
		busy_ticket = XM_GetTickCount() - curr_ticket;
		if(busy_ticket >= 500)		
		{
			result = SDMMC_RW_CMDTIMEROUT;
			XM_printf ("SD_WRITE Failed, last busy timeout\n");
			break;	// 超时
		}
		XM_Sleep (1);
	}
	
ERR_HANDLER:
	
	if(result != SDMMC_RW_SUCCES)
	{
			// FIFO reset
			val = SDHC_REG_READ32 (sdmmc_info, SDMMC_CTRL);
			val |= (1 << 1);
			SDHC_REG_WRITE32 (sdmmc_info, SDMMC_CTRL, val);
			while(SDHC_REG_READ32 (sdmmc_info, SDMMC_CTRL) & (1 << 1));
			// DMA reset
			val = SDHC_REG_READ32 (sdmmc_info, SDMMC_CTRL);
			val |= (1 << 2);
			SDHC_REG_WRITE32 (sdmmc_info, SDMMC_CTRL, val);
			while(SDHC_REG_READ32 (sdmmc_info, SDMMC_CTRL) & (1 << 2));
	}
	
	return result;
	
}


/*int get_status(int rca)
{
  	rSDMMC_CMDARG = rca;
  	rSDMMC_CMD = CMD13_STATUS_CARD;       //CMD13 --get rSDMMC_STATUS
  	sd_command_done();

  	return rSDMMC_RESP0;
}
*/
static INT32 get_scr(SDMMC_INFO * sdmmc_info, UINT32 *scr)
{
  	INT32 i;

  	//rSDMMC_BLKSIZ = 8;
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BLKSIZ, 8);
  	//rSDMMC_BYTCNT = 8;
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BYTCNT, 8);

  	//rSDMMC_CMDARG = sd_rca;
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, sdmmc_info->sdmmc_reg_info->sd_rca);
	//rSDMMC_CMD = CMD55_APP;              //CMD55
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD55_APP);
  	if(sd_command_done(sdmmc_info))
  	{
		return 1;
  	}
  	//rSDMMC_CMD = ACMD51_GET_CSR;              //ACMD51
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, ACMD51_GET_CSR);
  	if(sd_command_done(sdmmc_info))
  	{
		return 1;
  	}
	busy_delay(1000);
	
  	if(confi_fifo_unempty(sdmmc_info))
  	{
		// 如果卡检测到插入，应提示用户接触不良，重新插入
		//return 1;
		goto error;
	}
  	for(i = 0; i < 2; i ++)
  	{
		*scr++ = SDHC_REG_READ32(sdmmc_info, SDMMC_DATA);
  	}

	return 0;
	
error:
	// All data read commands can be aborted any time by the stop command (CMD12). The data transfer
	// will terminate and the card will return to the Transfer State. The read commands are: block read
	// (CMD17), multiple block read (CMD18), send write protect (CMD30), send scr (ACMD51) and
	// general command in read mode (CMD56).		
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD12_STOP_STEARM);
	if(sd_command_done(sdmmc_info))
	{
		
	}
	return 1;
}

static INT8 set_RCA(SDMMC_INFO * sdmmc_info, INT32 rca)
{
  	//rSDMMC_CMD = CMD2_CID;          // rSDMMC_CMD 2   --- all send CID
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD2_CID);
	if(sd_command_done(sdmmc_info))
		return 1;
	
	sdmmc_info->sdmmc_reg_info->sd_rca = rca;

//	sd_cid[0] = rSDMMC_RESP0;
//	sd_cid[1] = rSDMMC_RESP1;
//	sd_cid[2] = rSDMMC_RESP2;
//	sd_cid[3] = rSDMMC_RESP3;

	sdmmc_info->sdmmc_reg_info->sd_cid[0] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0);
	sdmmc_info->sdmmc_reg_info->sd_cid[1] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP1);
	sdmmc_info->sdmmc_reg_info->sd_cid[2] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP2);
	sdmmc_info->sdmmc_reg_info->sd_cid[3] = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP3);

  	//rSDMMC_CMDARG = sd_rca;
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, sdmmc_info->sdmmc_reg_info->sd_rca);
  	//rSDMMC_CMD = CMD3_RCA;          // CMD3    --- ask card publish new RCA
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD3_RCA);
	if(sd_command_done(sdmmc_info))
		return 1;
	
	return 0;
}

static UINT8 get_width(UINT32 *scr)
{
	if((scr[0]&0x00000f00) == 0x00000500)
		return 4;
	else
		return 1;
}

static INT32 mmc_bus_width(SDMMC_INFO * sdmmc_info, INT32 width)
{
//	INT32 ret;

	if(width == 4)
	{
      		//rSDMMC_CTYPE= 0x00000001;             				// bus_width = 4 bit;
      		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CTYPE, 0x00000001);
		//rSDMMC_CMDARG = 0x03B70100;
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, 0x03B70100);
	}
	else
	{
		//rSDMMC_CTYPE = 0x00000000;             				// bus_width = 1 bit;
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CTYPE, 0x00000000);
      		//rSDMMC_CMDARG = 0x03B70000;
      		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, 0x03B70000);
	}

	//rSDMMC_CMD = CMD6_SWITH;          					// CMD6    --- set width_bus
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD6_SWITH);
  	if(sd_command_done(sdmmc_info))
	{
		//rSDMMC_CTYPE = 0x00000000;
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CTYPE, 0x00000000);
		DEBUG_MSG("mmc_command_done error\n");
		return 1;
	}
  	if(busy_data(sdmmc_info))
	{
		//rSDMMC_CTYPE = 0x00000000;
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CTYPE, 0x00000000);
		DEBUG_MSG("busy_data error\n");
		return 1;
	}
  	if((SDHC_REG_READ32(sdmmc_info, SDMMC_STATUS)&0x0001f800) == 0x00003000)
  	{
  		DEBUG_MSG("Set 4-bit mode success\n");
		return 0;
  	}

	//rSDMMC_CTYPE = 0x00000000;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CTYPE, 0x00000000);
	
	return 1;
}




INT32 sd_mmc_card_identify(SDMMC_INFO * sdmmc_info)
{
	UINT8 VccSuccess;
	UINT8 val;
	INT32 spc;
	UINT32 divider;
//	UINT32 SrcClk;

	sdmmc_info->CardType = SD;
	sdmmc_info->Capacity = Standard; //need to re_initialize in case of MMC card

	//rSDMMC_CTRL |= 0x01000003;	//fifo reset, controller reset
#if  0
	val = SDHC_REG_READ32(ulChip, SDMMC_CTRL);
	val |= 0x01000003;
#ifdef USE_DMA_INTERFACE
	val |= 0x00000020;
#endif
	SDHC_REG_WRITE32(ulChip, SDMMC_CTRL, val);
	DEBUG_MSG("initial  enable  open drain!!\n");
#else
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_CTRL);
	//val &= ~(0x1<<24);
	val |= (0x1<<24);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CTRL, val);

	val = SDHC_REG_READ32(sdmmc_info, SDMMC_CTRL);
	val |= 0x00000003;
#ifdef USE_DMA_INTERFACE
	val |= 0x00000024;
#endif
	
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CTRL, val);
#endif
	//rSDMMC_RINTSTS = 0xFFFFFFFF;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0xFFFFFFFF);
	
//#ifdef SD_DMA_TRANSFER
//	rSDMMC_CTRL |= 0x01000020;
//#endif
	//rSDMMC_CTYPE = 0x00000000;	// set the controller 1bit mode
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CTYPE, 0x00000000);

	//divider = Get_Card_SrcClk(sdmmc_info)/INIT_CLOCK;//1200/15=80 
	//divider = divider>>1;
	divider = Get_Card_SrcClk(sdmmc_info) / (2 * INIT_CLOCK) + 1;
	if(chang_clk(sdmmc_info, divider, CLK_SRC, CLK_CLKEN)) // SD card frequeny is 16.5/0x6B/2MHz
	{
		DEBUG_MSG("sd_mmc_card_identify failed, change_clk error\n");
		return 1;
	}


  	//rSDMMC_PWREN = PWREN_ON;
  	SDHC_REG_WRITE32(sdmmc_info, SDMMC_PWREN, PWREN_ON);
	// Once power is turned on, firmware should wait for regulator/switch ramp-up time before trying to initialize card
	OS_Delay (1);	// 1ms
  	if(initial_clk(sdmmc_info))
  	{
  		DEBUG_MSG("sd_mmc_card_identify failed, initial_clk failed\n");
		return 1;
  	}

	//rSDMMC_CMDARG =00000000;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, 00000000);
	//rSDMMC_CMD = CMD0_GO_IDLE;       // first initial card for 80 clock then send command;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD0_GO_IDLE);
  	if(sd_command_done(sdmmc_info))
  	{
		DEBUG_MSG("sd_mmc_card_identify failed, CMD0_GO_IDLE error\n");
		return 1;
	}

	spc= conf_spec(sdmmc_info);

  	if(spc== 0)
  	{
  		//DEBUG_MSG("Ver2.00 or later SD Memory Card\n");
  	}
  		

	if(spc ==1)
	{
		DEBUG_MSG("cmd operate fails,quit\n");
		return 1;
	}

	sdmmc_info->sd_clk = Get_Card_SrcClk(sdmmc_info);
 	VccSuccess = match_sd_vcc(sdmmc_info);

	if(VccSuccess == 2)
	{
		DEBUG_MSG("match sd voltage fails cos of cmd not finish\n");
		return 1;
	}
  	else if (VccSuccess == 0)
  	{
  		VccSuccess = match_mmc_vcc(sdmmc_info);
  		if (VccSuccess == 0)
  			return 1;
		else if(VccSuccess == 2)
		{
			DEBUG_MSG("match sd voltage fails cos of cmd not finish\n");
			return 1;
		}
  		else
  		{
  			sdmmc_info->CardType = MMC;		

#define MAX_MMC_TRANS_CLK		20000000
			if(sdmmc_info->sd_clk > MAX_MMC_TRANS_CLK)
			{
				if(sdmmc_info->sd_clk_source == SYS_PLL)
				{
					if((GetSYSPLLFrequency() / MAX_MMC_TRANS_CLK) % 2)
						divider = (GetSYSPLLFrequency() / MAX_MMC_TRANS_CLK) + 1;
					else
						divider = (GetSYSPLLFrequency() / MAX_MMC_TRANS_CLK);
					
					Select_Card_SrcClk(sdmmc_info,  SYS_PLL, divider);
				}
				else if(sdmmc_info->sd_clk_source == CLK_240M)
				{
					if((GetSYSPLLFrequency() / MAX_MMC_TRANS_CLK) % 2)
						divider = (GetCLK240MFrequency() / MAX_MMC_TRANS_CLK) + 1;
					else
						divider = (GetCLK240MFrequency() / MAX_MMC_TRANS_CLK);
					
					Select_Card_SrcClk(sdmmc_info,  CLK_240M, divider);
				}
				else if(sdmmc_info->sd_clk_source == CPU_PLL)
				{
					if((GetSYSPLLFrequency() / MAX_MMC_TRANS_CLK) % 2)
						divider = (GetCPUPLLFrequency() / MAX_MMC_TRANS_CLK) + 1;
					else
						divider = (GetCPUPLLFrequency() / MAX_MMC_TRANS_CLK);

					Select_Card_SrcClk(sdmmc_info,  CPU_PLL, divider);
				}
			}
  		}
  	}
	else
	{
	#define MAX_SD_TRANS_CLK		24000000
		if(sdmmc_info->sd_clk > MAX_SD_TRANS_CLK)
		{
			if(sdmmc_info->sd_clk_source == SYS_PLL)
			{
				if((GetSYSPLLFrequency() / MAX_SD_TRANS_CLK) % 2)
					divider = (GetSYSPLLFrequency() / MAX_SD_TRANS_CLK) + 1;
				else
					divider = (GetSYSPLLFrequency() / MAX_SD_TRANS_CLK);
				
				Select_Card_SrcClk(sdmmc_info,  SYS_PLL, divider);
			}
			else if(sdmmc_info->sd_clk_source == CLK_240M)
			{
				if((GetSYSPLLFrequency() / MAX_SD_TRANS_CLK) % 2)
					divider = (GetCLK240MFrequency() / MAX_SD_TRANS_CLK) + 1;
				else
					divider = (GetCLK240MFrequency() / MAX_SD_TRANS_CLK);
				
				Select_Card_SrcClk(sdmmc_info,  CLK_240M, divider);
			}
			else if(sdmmc_info->sd_clk_source == CPU_PLL)
			{
				if((GetSYSPLLFrequency() / MAX_SD_TRANS_CLK) % 2)
					divider = (GetCPUPLLFrequency() / MAX_SD_TRANS_CLK) + 1;
				else
					divider = (GetCPUPLLFrequency() / MAX_SD_TRANS_CLK);

				Select_Card_SrcClk(sdmmc_info,  CPU_PLL, divider);
			}
		}
	}
	return 0;
}

static unsigned int  get_all_information(SDMMC_INFO * sdmmc_info)
{
	unsigned int scr[2];
//	DEBUG_MSG("----------------------------------\n");	
//	DEBUG_MSG("OCR (32bit) : %x\n\n",sd_ocr);

//	DEBUG_MSG("CID3  (128bit) : %x\n",sdmmc_info->sdmmc_reg_info->sd_cid[3]);
//	DEBUG_MSG("CID2  (128bit) : %x\n",sdmmc_info->sdmmc_reg_info->sd_cid[2]);
//	DEBUG_MSG("CID1  (128bit) : %x\n",sdmmc_info->sdmmc_reg_info->sd_cid[1]);
//	DEBUG_MSG("CID0  (128bit) : %x\n\n",sdmmc_info->sdmmc_reg_info->sd_cid[0]);
	
//	DEBUG_MSG("CSD3  (128bit) : %x\n",sdmmc_info->sdmmc_reg_info->sd_csd[3]);
//	DEBUG_MSG("CSD2  (128bit) : %x\n",sdmmc_info->sdmmc_reg_info->sd_csd[2]);
//	DEBUG_MSG("CSD1  (128bit) : %x\n",sdmmc_info->sdmmc_reg_info->sd_csd[1]);
//	DEBUG_MSG("CSD0  (128bit) : %x\n\n",sdmmc_info->sdmmc_reg_info->sd_csd[0]);
	
 	if(sdmmc_info->CardType == SD)
 	{
		if(get_scr(sdmmc_info, scr))
		{
			DEBUG_MSG("get_scr fails\n");
			return 1;
		}
		//DEBUG_MSG("scr1  (64bit) : %x\n",scr[1]);
		//DEBUG_MSG("scr0  (64bit) : %x\n\n",scr[0]);
 	}
	
	//DEBUG_MSG("RCA  (128bit) : %x\n", sdmmc_info->sdmmc_reg_info->sd_rca);

	if(sdmmc_info->CardType == SD)
	{
		GetCardInfo((INT32 *)sdmmc_info->sdmmc_reg_info->sd_cid,SD);
	}
	else
	{
		GetCardInfo((INT32 *)sdmmc_info->sdmmc_reg_info->sd_cid,MMC);
	}

	//DEBUG_MSG("----------------------------------\n");	

	return 0;

}

static INT32 init_sd_mmc(SDMMC_INFO * sdmmc_info)
{
	UINT32 scr[2];
//	UINT32 tran_speed;
	UINT8 width = 1;

#if 1//def CONFIG_ARKN141_ASIC
	UINT32 sd_clk_div = 0;// 0 : no div        1:    2 div
#else	
	// 20131223 ZhuoYongHong
	// FPGA的源时钟固定为35MHz，需分频
	//UINT32 sd_clk_div = 1;// 0 : no div        1:    2 div
	UINT32 sd_clk_div = 1;
#endif	

	if(sdmmc_info->CardType == SD)
		get_RCA(sdmmc_info);
	else
		set_RCA(sdmmc_info,0xabcd0000);

	if(get_csd(sdmmc_info,sdmmc_info->sdmmc_reg_info->sd_rca,sdmmc_info->sdmmc_reg_info->sd_csd))
	{
		DEBUG_MSG("SDMMC get_csd error\n");
		return 1;
	}

	
  	if(get_cid(sdmmc_info,sdmmc_info->sdmmc_reg_info->sd_rca,sdmmc_info->sdmmc_reg_info->sd_cid))
  	{
		DEBUG_MSG("SDMMC get_cid error\n");
		return 1;
  	}
		
  	sdmmc_info->blocksize = get_blocklen(sdmmc_info);

	block_len = 512;
	
	if(sdmmc_info->ChipSel == CHIP0)
  		SDCard.SecNum = get_capacity(sdmmc_info);
#if SDMMC_DEV_COUNT > 1
	else if(sdmmc_info->ChipSel == CHIP1)
		SDCard1.SecNum = get_capacity(sdmmc_info);
#endif
	
 	if((!SDCard.SecNum) && (sdmmc_info->ChipSel == CHIP0))
 	{
 		DEBUG_MSG("sd0 initial wrong\n");
 		return 1;
 	}
#if SDMMC_DEV_COUNT > 1
	if((!SDCard1.SecNum) && (sdmmc_info->ChipSel == CHIP1))
 	{
 		DEBUG_MSG("sd1 initial wrong\n");
 		return 1;
 	}
#endif
  	if(select_sd(sdmmc_info,sdmmc_info->sdmmc_reg_info->sd_rca))
  	{
		DEBUG_MSG("select_sd fails\n");
		return 1;
	}
		
//	XM_printf ("sd_clk_div = %d\n", sd_clk_div);

//  	tran_speed = get_speed_tran();
	/*如果SD_CLK工作频率很低，需检查CMD线的上拉电阻是否符合要求	*/
	if(chang_clk(sdmmc_info,sd_clk_div,CLK_DIV_NORMAL,CLK_CLKEN))
	{
       DEBUG_MSG("chang_clk fails\n");
       return 1;//note clk
   }

	if(sdmmc_info->CardType == SD)
	{
		UINT8 SD_SPEC;
		
		if(get_scr(sdmmc_info, scr))
		{
			DEBUG_MSG("get_scr fails\n");
			return 1;
		}
		width=get_width(scr);
		SD_SPEC = GetSD_SPEC(scr);
		DEBUG_MSG("Bus Width : %d\r\n", width);
		DEBUG_MSG("SD Memory Card - Spec. Version %s Capacity %s\n", sd_spec[SD_SPEC],sd_capacity[sdmmc_info->Capacity]);
		if(sd_bus_width(sdmmc_info,width))			// =4 ------- 4-bus; !=4 --------1-bus	
		{
			DEBUG_MSG("SD Card set 4bit width failure,and work at 1bit width.\n");
		}
	}
   else
   {
		UINT8 MMC_SPEC;
		
		MMC_SPEC = GetMMC_SPEC(sdmmc_info);
		//DEBUG_MSG("MMC_SPEC : %d\r\n", MMC_SPEC);
		DEBUG_MSG("MMC Card - Spec. Version %s Capacity %s\n", mmc_spec[MMC_SPEC],mmc_capacity[sdmmc_info->Capacity]);
		if(MMC_SPEC >= 4)
			width = 4;
		if(mmc_bus_width(sdmmc_info,width))
		{
			DEBUG_MSG("MMC Card set 4bit width failure,and work at 1bit width.\n");
		}
   }
	if(1 == get_all_information(sdmmc_info))
	{
		return 1;
	}
	
	if(sdmmc_info->CardSpec != Ver2)
		set_blocklen(sdmmc_info,block_len);

	CalculateTimeOutParameter(sdmmc_info);

	if(sdmmc_info->ChipSel == CHIP0)
	{
		SDCard.Present = 1;
		SDCard.Changed = 1; //card changed found
	}
#if SDMMC_DEV_COUNT > 1
	else if(sdmmc_info->ChipSel == CHIP1)
	{
		SDCard1.Present = 1;
		SDCard1.Changed = 1; //card changed found
	}
#endif
	return 0;
}

INT32 SDMMC_CardPlugOut(SDMMC_INFO * sdmmc_info)
{
	unsigned int val;

	sdmmc_info->init_cardflag_sd = 0x0;
	sdmmc_info->card_present = 0;		// 标记SD卡已拔出或者已执行过弹出操作
	DEBUG_MSG ("card_present=0\n");
		
	/* update the card Info */

	if(sdmmc_info->ChipSel == CHIP0)
	{
		SDCard.SecNum = 0;
		SDCard.Present =0; //card is not in socket
		SDCard.Changed = 1; //card changed found
	}
#if SDMMC_DEV_COUNT > 1
	else if(sdmmc_info->ChipSel == CHIP1)
	{
		SDCard1.SecNum = 0;
		SDCard1.Present =0; //card is not in socket
		SDCard1.Changed = 1; //card changed found
	}
#endif
	
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_CTRL);
	val &= ~(0x1<<24);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CTRL, val);

	val = SDHC_REG_READ32(sdmmc_info, SDMMC_CTRL);
	val |= 0x00000003;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CTRL, val);
  	DEBUG_MSG("SD card remove out of the new_out INT!!!\n");

	return 0;
}

static INT32 SD_Identify(SDMMC_INFO * sdmmc_info)
{
	INT32 ret,val;

	select_sd_pad(sdmmc_info);

	ret= sd_mmc_card_identify(sdmmc_info);

	val = SDHC_REG_READ32(sdmmc_info, SDMMC_CTRL);
	val &= ~(0x1<<24);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CTRL, val);
	
	if(ret)
		DEBUG_MSG("SD_Identify Chip%d fails\n", sdmmc_info->ChipSel);

	return ret;
}

static INT32 SD_Init(SDMMC_INFO * sdmmc_info)
{
	INT32 ret;

	
	ret= init_sd_mmc(sdmmc_info);
	
	
	return ret;
}


#define MAX_TRANS_SECTORS		64	
INT32 SD_ReadSector(SDMMC_INFO * sdmmc_info,UINT32 Unit,ULONG Sector,UINT8 *pBuffer, ULONG NumSectors)
{
	// 20181025 大文件读出时, NumSectors会超出SDMMC最大支持的Burst长度，导致异常。
	// 使用MAX_TRANS_SECTORS限制最大burst长度
#if 0//defined(_NEW_SDMMC_DMA_DRIVER_) && defined(USE_DMA_INTERFACE)
	return sd_mmc_read_dma(sdmmc_info, Sector, NumSectors, (UINT32 *)pBuffer);
#else
	INT32 ret, Counts, ReadCounts, LeftSectors;

	ReadCounts = NumSectors / MAX_TRANS_SECTORS;
	LeftSectors = NumSectors % MAX_TRANS_SECTORS;

	for(Counts=0; Counts < ReadCounts; Counts++)
	{
#ifdef USE_DMA_INTERFACE		
		ret= sd_mmc_read_dma(sdmmc_info,Sector + MAX_TRANS_SECTORS*Counts, MAX_TRANS_SECTORS, (UINT32 *)pBuffer);		
#else
		ret= sd_mmc_read(sdmmc_info,Sector + MAX_TRANS_SECTORS*Counts, MAX_TRANS_SECTORS, (UINT32 *)pBuffer);
#endif
		pBuffer += MAX_TRANS_SECTORS*block_len;

		if(ret < 0)
		{
			return ret;
		}
	}

	if(LeftSectors)
	{
#ifdef USE_DMA_INTERFACE
		ret= sd_mmc_read_dma(sdmmc_info,Sector + MAX_TRANS_SECTORS*Counts, LeftSectors, (UINT32 *)pBuffer);
#else
		ret= sd_mmc_read(sdmmc_info,Sector + MAX_TRANS_SECTORS*Counts, LeftSectors, (UINT32 *)pBuffer);
#endif
//		LeftSectors += 16*block_len;

		if(ret < 0)
		{		
			return ret;
		}
	}
	
	return ret;
#endif
}

INT32 InitSDMMCCard(UINT32 ulCardID)
{
	INT32 result = -1;
	SDMMC_INFO * sdmmc_info;

	switch(ulCardID)
	{
		case 0:
			sdmmc_info = sdmmc_chip0;
			break;
#if SDMMC_DEV_COUNT > 1
		case 1:
			sdmmc_info = sdmmc_chip1;
			break;
#endif

		default:
			return result;
	}	

	XM_lock ();
	// 检查卡物理状态
	if(SDHC_REG_READ32(sdmmc_info, SDMMC_CDETECT) & 0x1)
	{
		// 卡未插入
		XM_unlock ();
		return -1;
	}
	// 检查卡去抖动状态
	if(ulCardID == 0 && lg_ulCard0DeDitheringStatus != CARD_DEDIGHERING_STATUS_PASS)
	{
		// 卡未插入
		XM_unlock ();
		return -1;		
	}
#if SDMMC_DEV_COUNT > 1
	if(ulCardID == 1 && lg_ulCard1DeDitheringStatus != CARD_DEDIGHERING_STATUS_PASS)
	{
		// 卡未插入
		XM_unlock ();
		return -1;		
	}
#endif
	
	// 保护init_cardflag_sd与card_present的一致性
	//if(!sdmmc_info->init_cardflag_sd)
	{
		sdmmc_info->card_present = 1;		// 标记卡已有效, 允许卡IO操作
		XM_unlock ();
		XM_printf ("card_present=1\n");

		if ( !SD_Identify(sdmmc_info))
		{
			DEBUG_MSG("SDMMC card successfully\n");
			if (!SD_Init(sdmmc_info))
			{
				sdmmc_info->init_cardflag_sd = 0x1;	 // means init success		
				result = 0;
			}
		}
		else
		{
			DEBUG_MSG("SDMMC card identify failed\n");
		}
	}
	/*
	else
	{
		XM_unlock ();
		XM_printf ("SDMMC card already inited\n");
		result = 0;
	}
	*/

	return result;
}

INT32 StopSDMMCCard (UINT32 ulCardID)
{
	INT32 result = -1;
	SDMMC_INFO * sdmmc_info;

	switch(ulCardID)
	{
		case 0:
			sdmmc_info = sdmmc_chip0;
			break;
#if SDMMC_DEV_COUNT > 1
		case 1:
			sdmmc_info = sdmmc_chip1;
			break;
#endif

		default:
			return result;
	}	
	
	XM_lock ();
	sdmmc_info->init_cardflag_sd = 0;
	sdmmc_info->card_present = 0;
	XM_unlock ();	
	
	XM_printf ("card_present=0\n");
	
	return 0;
}




static void PostCardEvent(UINT32 ulPlugIn, UINT32 ulCardID)
{
	if(ulPlugIn == SDCARD_REINIT)
		DEBUG_MSG ("SDMMC %d %s\n", ulCardID, "REINIT");
	else
		DEBUG_MSG ("SDMMC %d %s\n", ulCardID, (ulPlugIn == SDCARD_INSERT) ? "INSERT" : "REMOVE");
	
	fEvent curEvent;
	fCard_Event *pCardEvent=NULL;
	
	pCardEvent = (fCard_Event *)(&curEvent.uFEvent);
	pCardEvent->event_type = CARD_EVENT;
	pCardEvent->cardType = SD_DEVICE;
	pCardEvent->cardID = ulCardID;
	pCardEvent->plugIn = ulPlugIn;
	
	if(ulPlugIn == SDCARD_REINIT)
	{
		int discard_op = 0;
		// 检查卡物理状态, 如果非正常操作状态CARD_DEDIGHERING_STATUS_PASS, 则放弃该操作
		XM_lock();
		if(ulCardID == 0 && lg_ulCard0DeDitheringStatus != CARD_DEDIGHERING_STATUS_PASS)
		{
			// 卡插拔抖动处理过程中
			discard_op = 1;
		}
#if SDMMC_DEV_COUNT > 1
		if(ulCardID == 1 && lg_ulCard1DeDitheringStatus != CARD_DEDIGHERING_STATUS_PASS)
		{
			// 卡插拔抖动处理过程中
			discard_op = 1;
		}
#endif		
		XM_unlock();
		if(discard_op)
		{
			DEBUG_MSG ("DitheringStatus, SDCARD_REINIT\n");
			return;
		}
		
		// 检查相同消息是否在系统消息队列中
		if(chkEventQueue (&curEvent))
		{
			// 消息存在于系统消息队列中, 不再发送
			DEBUG_MSG ("queued, SDCARD_REINIT\n");
			return;
		}
	}

//	libModeMng_PostEvent(&curEvent);	
	addEventQueue(&curEvent);
}

// SDMMC0_Dedithering_Handler是一个软中断, 不是真正的硬件中断
static void SDMMC0_Dedithering_Handler(void *pdata, UINT16 card)
{
	// DEBUG_MSG("time out\r\n");
	XM_lock();		// 保护, 不受到卡插拔中断的影响
	if(lg_ulCard0DeDitheringStatus == CARD_DEDIGHERING_STATUS_IDLE)
	{
		lg_ulCard0DeDitheringStatus = CARD_DEDIGHERING_STATUS_RUNNING;
	}
	else if(lg_ulCard0DeDitheringStatus == CARD_DEDIGHERING_STATUS_RUNNING)
	{
		lg_ulCard0DeDitheringStatus = CARD_DEDIGHERING_STATUS_PASS;
		StopHardwareTimer(lg_pSDMMC0_DetheringTimer);
		PostCardEvent(SDCARD_INSERT, 0);
	}	
	XM_unlock();
}

#if SDMMC_DEV_COUNT > 1
static void SDMMC1_Dedithering_Handler(void *pdata, UINT16 card)
{
//	DEBUG_MSG("time out\r\n");
	XM_lock();		// 保护, 不受到卡插拔中断的影响
	if(lg_ulCard1DeDitheringStatus == CARD_DEDIGHERING_STATUS_IDLE)
	{
		lg_ulCard1DeDitheringStatus = CARD_DEDIGHERING_STATUS_RUNNING;
	}
	else if(lg_ulCard1DeDitheringStatus == CARD_DEDIGHERING_STATUS_RUNNING)
	{
		StopHardwareTimer(lg_pSDMMC1_DetheringTimer);
	
		lg_ulCard1DeDitheringStatus = CARD_DEDIGHERING_STATUS_PASS;
	
		PostCardEvent(SDCARD_INSERT, 1);
	}
	XM_unlock();
}
#endif


// 消颤时间
#define	SDMMC_DETHER_TIMEOUT		50		// 50ms	

static void InitDeditheringTimer(void)
{
	lg_pSDMMC0_DetheringTimer = NewHardwareTimer(SDMMC_DETHER_TIMEOUT, 0, SDMMC0_Dedithering_Handler);
	if(lg_pSDMMC0_DetheringTimer)
		AddHardwareTimer(lg_pSDMMC0_DetheringTimer);

#if SDMMC_DEV_COUNT > 1
	lg_pSDMMC1_DetheringTimer = NewHardwareTimer(SDMMC_DETHER_TIMEOUT, 0, SDMMC1_Dedithering_Handler);
	if(lg_pSDMMC1_DetheringTimer)
		AddHardwareTimer(lg_pSDMMC1_DetheringTimer);
#endif
	
	lg_ulCard0DeDitheringStatus = CARD_DEDIGHERING_STATUS_IDLE;
#if SDMMC_DEV_COUNT > 1
	lg_ulCard1DeDitheringStatus = CARD_DEDIGHERING_STATUS_IDLE;
#endif
}



void SDCard_Module_CardCheck (void)
{
	XM_lock ();
	if(SDHC_REG_READ32(sdmmc_chip0, SDMMC_CDETECT) & 0x1)
	{
		//DEBUG_MSG ("card 0 remove\n");
		PostCardEvent(SDCARD_REMOVE, 0);
	}
	else
	{
		//DEBUG_MSG ("card 0 insert\n");
		//lg_ulCard0DeDitheringStatus = CARD_DEDIGHERING_STATUS_PASS;
		//PostCardEvent(SDCARD_INSERT, 0);
		if(lg_pSDMMC0_DetheringTimer)
		{
			StartHardwareTimer(lg_pSDMMC0_DetheringTimer);
			lg_ulCard0DeDitheringStatus = CARD_DEDIGHERING_STATUS_RUNNING;
		}
	}
	XM_unlock ();

#if SDMMC_DEV_COUNT > 1
	XM_lock ();
	if(SDHC_REG_READ32(sdmmc_chip1, SDMMC_CDETECT) & 0x1)
	{
		//DEBUG_MSG ("card 1 remove\n");
		PostCardEvent(SDCARD_REMOVE, 1);
	}
	else
	{
		//DEBUG_MSG ("card 1 insert\n");
		//lg_ulCard1DeDitheringStatus = CARD_DEDIGHERING_STATUS_PASS;
		//PostCardEvent(SDCARD_INSERT, 1);
		if(lg_pSDMMC1_DetheringTimer)
		{
			StartHardwareTimer(lg_pSDMMC1_DetheringTimer);
			lg_ulCard1DeDitheringStatus = CARD_DEDIGHERING_STATUS_RUNNING;
		}
	}
	XM_unlock ();
#endif	
}

void SDMMC_IRQ_Handler0()
{
	UINT32 value;

//CheckPADRegs();

	//value = rSDMMC_MINTSTS;
	value = SDHC_REG_READ32(sdmmc_chip0, SDMMC_MINTSTS);
	//clear interrupt factor
	//rSDMMC_RINTSTS = value;
	SDHC_REG_WRITE32(sdmmc_chip0, SDMMC_RINTSTS, value);

	// DEBUG_MSG("enter SDMMC_IRQ_Handler, MINTSTS=%08x\n", value);

	if(value & 0x01)//card detect int
	{	
		if(SDHC_REG_READ32(sdmmc_chip0, SDMMC_CDETECT) & 0x1)
		{
			//card plug out
			sdmmc_chip0->card_present = 0;
			
			// 检查WRITE DMA
#ifdef _XMSYS_SDMMC_EVENT_DRIVEN_
			sdmmc_dma_transmit_stop (CHIP0, SDMMC_EVENT_WRITE_CARD_REMOVE);
#endif
			
			DEBUG_MSG("---------------SDMMC0 card plug out--------------\n");
			switch(lg_ulCard0DeDitheringStatus)
			{
				case CARD_DEDIGHERING_STATUS_IDLE:
					//do nothing
					break;

				case CARD_DEDIGHERING_STATUS_RUNNING:
					if(lg_pSDMMC0_DetheringTimer)
						StopHardwareTimer(lg_pSDMMC0_DetheringTimer);
					lg_ulCard0DeDitheringStatus = CARD_DEDIGHERING_STATUS_IDLE;
					break;

				case CARD_DEDIGHERING_STATUS_PASS:
					SDMMC_CardPlugOut(sdmmc_chip0);
					PostCardEvent(SDCARD_REMOVE, 0);
					lg_ulCard0DeDitheringStatus = CARD_DEDIGHERING_STATUS_IDLE;
					DEBUG_MSG ("STATUS_IDLE\n");
					break;
			}
		}
		else
		{
			//card inserted
			//DEBUG_MSG("---------------SDMMC0 card in--------------\n");

			switch(lg_ulCard0DeDitheringStatus)
			{
				case CARD_DEDIGHERING_STATUS_IDLE:
					if(lg_pSDMMC0_DetheringTimer)
						StartHardwareTimer(lg_pSDMMC0_DetheringTimer);
					lg_ulCard0DeDitheringStatus = CARD_DEDIGHERING_STATUS_RUNNING;
					break;
					
				case CARD_DEDIGHERING_STATUS_RUNNING:
					if(lg_pSDMMC0_DetheringTimer)
						StartHardwareTimer(lg_pSDMMC0_DetheringTimer);
					break;
					
				case CARD_DEDIGHERING_STATUS_PASS:
					//do nothing, it must be a error
					break;
			}
		}
	}

	if(value & (1<<12))
		DEBUG_MSG("Hardware locked write error\n");
	
#ifdef _XMSYS_SDMMC_EVENT_DRIVEN_
	if(value & RINTSTS_EBE)
	{
		if(sdmmc_chip0->is_read_mode)
		{
			// 读
		}
		else
		{
			// 写
			// write no CRC (EBE)
			sdmmc_dma_transmit_stop (CHIP0, SDMMC_EVENT_WRITE_NO_CRC_STATUS);
		}
	}
	/*
	if(value & RINTSTS_DCRC)
	{
		if(sdmmc_chip0->is_read_mode)
		{
		}
		else
		{
			sdmmc_dma_transmit_stop (CHIP0, SDMMC_EVENT_WRITE_NEGATIVE_CRC);
		}
	}
	*/
#endif
}

#if SDMMC_DEV_COUNT > 1
void SDMMC_IRQ_Handler1()
{
	UINT32 value;

//CheckPADRegs();

	//value = rSDMMC_MINTSTS;
	value = SDHC_REG_READ32(sdmmc_chip1, SDMMC_MINTSTS);
	//clear interrupt factor
	//rSDMMC_RINTSTS = value;
	SDHC_REG_WRITE32(sdmmc_chip1, SDMMC_RINTSTS, value);
//	DEBUG_MSG("enter SDMMC_IRQ_Handler\n");

	if(value & 0x01)//card detect int
	{	
		if(SDHC_REG_READ32(sdmmc_chip1, SDMMC_CDETECT) & 0x1)
		{
			//card plug out
			sdmmc_chip1->card_present = 0;
			DEBUG_MSG("SDMMC1 card out\n");

			switch(lg_ulCard1DeDitheringStatus)
			{
				case CARD_DEDIGHERING_STATUS_IDLE:
					//do nothing
					break;

				case CARD_DEDIGHERING_STATUS_RUNNING:
					if(lg_pSDMMC1_DetheringTimer)
						StopHardwareTimer(lg_pSDMMC1_DetheringTimer);
					lg_ulCard1DeDitheringStatus = CARD_DEDIGHERING_STATUS_IDLE;
					break;

				case CARD_DEDIGHERING_STATUS_PASS:
					SDMMC_CardPlugOut(sdmmc_chip1);
					PostCardEvent(SDCARD_REMOVE, 1);
					lg_ulCard1DeDitheringStatus = CARD_DEDIGHERING_STATUS_IDLE;
					break;
			}		 
		}
		else
		{
			//card inserted
			DEBUG_MSG("---------------SDMMC1 card in--------------\n");
			switch(lg_ulCard1DeDitheringStatus)
			{
				case CARD_DEDIGHERING_STATUS_IDLE:
					if(lg_pSDMMC1_DetheringTimer)
						StartHardwareTimer(lg_pSDMMC1_DetheringTimer);
					lg_ulCard1DeDitheringStatus = CARD_DEDIGHERING_STATUS_RUNNING;
					break;
					
				case CARD_DEDIGHERING_STATUS_RUNNING:
					if(lg_pSDMMC1_DetheringTimer)
						StartHardwareTimer(lg_pSDMMC1_DetheringTimer);
					break;
					
				case CARD_DEDIGHERING_STATUS_PASS:
					//do nothing, it must be a error
					break;
			}									
		}
	}

	if(value & (1<<12))
		DEBUG_MSG("Hardware locked write error\n");

}
#endif



INT32 SD_WriteSector(SDMMC_INFO * sdmmc_info,UINT32 Unit,ULONG  Sector,UINT8 *pBuffer, ULONG  NumSectors, U8 RepeatSame)
{
	INT32 ret, Counts, WriteCounts, LeftSectors;
	// 以16个连续块为write block len可以消除SD卡DMA写入中出现的无应答现象, 但写入速度较慢
	// unsigned int trans_sectors = MAX_BATCH_TRANS_SECTOR;	//sdmmc_info->WRITE_BL_LEN / 512;
	//unsigned int trans_sectors = MAX_BATCH_TRANS_SECTOR * 4;	//sdmmc_info->WRITE_BL_LEN / 512;
	//unsigned int trans_sectors = MAX_BATCH_TRANS_SECTOR;
	unsigned int trans_sectors = MAX_BATCH_TRANS_SECTOR * 16;	
	
	WriteCounts = NumSectors / trans_sectors;
	LeftSectors = NumSectors % trans_sectors;
	
	dma_clean_range((UINT32)pBuffer, (UINT32)pBuffer+ NumSectors*512);
	select_sd_pad(sdmmc_info);

	for(Counts=0; Counts < WriteCounts; Counts++)
	{
#ifdef USE_DMA_INTERFACE		
		ret= sd_mmc_write_dma(sdmmc_info,Sector + trans_sectors*Counts, trans_sectors, (UINT32 *)pBuffer);
#else
		ret= sd_mmc_write(sdmmc_info,Sector + MAX_TRANS_SECTORS*Counts, trans_sectors, (UINT32 *)pBuffer);
#endif
		if(RepeatSame==0)//add by zxh
			pBuffer += trans_sectors*block_len;

		if(ret  < 0)
		{
			return ret;
		}		
	}

	if(LeftSectors > 0)
	{
#ifdef USE_DMA_INTERFACE
		ret= sd_mmc_write_dma(sdmmc_info,Sector + trans_sectors*Counts, LeftSectors, (UINT32 *)pBuffer);
#else
		ret= sd_mmc_write(sdmmc_info,Sector + trans_sectors*Counts, LeftSectors, (UINT32 *)pBuffer);
#endif
//		pBuffer += LeftSectors*block_len;
		
		if(ret < 0)
		{
			return ret;
		}		
	}
		
	return ret;
}

#if 0//def USE_DMA_INTERFACE 	

static INT32 sd_write_multi_buffer_through_dma (SDMMC_INFO * sdmmc_info, UINT32 NumBuffers, const UINT8 **pSectorBuffer, ULONG *pSectorCount)
{
	UINT32 SdDmaCh;
	UINT32 DmaTime = 0;
	INT32 rst = 0;
	unsigned int intsts;
	UINT32  len;
	unsigned int ticket_to_timeout;
	      	
	SdDmaCh = sdmmc_dma_channel[sdmmc_info->ChipSel - CHIP0];
	sdmmc_info->SdDmaCh = SdDmaCh;
	
	// 创建DMA链表
	sdmmc_info->lli = sdmmc_dma_lli[sdmmc_info->ChipSel - CHIP0];
	
	dma_addr_t	dar = sdmmc_info->sdmmc_reg_info->SDHC_Control + SDMMC_FIFO;
	struct dw_lli *	lli = &sdmmc_info->lli[0];
	while(NumBuffers > 0)
	{
		UINT32 left;
		int i;
		dma_addr_t dst_addr = (dma_addr_t)*pSectorBuffer;
		int lli_count;
		
		len = *pSectorCount * 512;
		left = len;
		lli_count = (len + MAX_BATCH_TRANS_SECTOR*512 - 1) / (MAX_BATCH_TRANS_SECTOR*512);
		
		for (i = 0; i < lli_count; i++)
		{
			UINT32  to_transfer;
			lli->sar = dst_addr;
			lli->dar = dar;
			//if(i != (lli_count - 1))
			//	lli->llp = (UINT32)&sdmmc_info->lli[i + 1];
			//else
			//	lli->llp = 0;
			lli->llp = (UINT32)(lli + 1);
			lli->ctllo = SD_TX_DMA_CTL_L | (1 << 28);	// LLP_SRC_EN
			if(left >= MAX_BATCH_TRANS_SECTOR*512)
				to_transfer = MAX_BATCH_TRANS_SECTOR*512;
			else
				to_transfer = left;
			lli->ctlhi = SD_TX_DMA_CTL_H | (to_transfer / 4);
			lli->sstat = 0;
			lli->dstat = 0;
			dst_addr += to_transfer;
			left -= to_transfer;
			lli ++;
		}		
		
		pSectorCount ++;
		pSectorBuffer ++;
		NumBuffers --;
	}
	
	(lli - 1)->llp = 0;
	
	dma_clr_int(SdDmaCh);
	dma_cfg_channel(SdDmaCh, 
					sdmmc_info->lli[0].sar,
					sdmmc_info->lli[0].dar,
					(UINT32)(&sdmmc_info->lli[0])&0xfffffffc,
					sdmmc_info->lli[0].ctllo,
					sdmmc_info->lli[0].ctlhi,
					SD_TX_DMA_CFG_L | (SdDmaCh<<5), (SD_TX_DMA_CFG_H_LLI | (sdmmc_info->dma_req<<12)),
					0,
					0);	
	dma_flush_range ((UINT32)&sdmmc_info->lli[0], (UINT32)lli);
	
   register_channel_IRQHandler (SdDmaCh, 
										  sdmmc_dma_transfer_over_IRQHandler_lli, 
										  sdmmc_info,
										  //(1<< BLOCK_INT_BIT)|(1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  (1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  );
	
	sdmmc_info->reinit = 0;
	sdmmc_info->crc_error = 0;
	DmaTime = 5;
	/*
	DmaTime = 2000 * (len / (MAX_BATCH_TRANS_SECTOR*512));
	if(DmaTime < 10)
		DmaTime = 10;
	else if(DmaTime > 500)
		DmaTime = 500;*/
	dma_start_channel(SdDmaCh);
	
	ticket_to_timeout = XM_GetTickCount() + 5000;
	
	while(!rst)
	{
		//unsigned int rintsts;
		int finished = Dma_TfrOver_PendSemEx (SdDmaCh, DmaTime);
		intsts = SDHC_REG_READ32(sdmmc_info,SDMMC_RINTSTS);
		if(finished == 1)
		{
			// 检查DMA执行的结果
			if(sdmmc_info->ret)
			{
				rst = 1;
			}
			else if(intsts & (1 << 15))	// write no CRC (EBE)
			{
				DEBUG_MSG("Write DMA err, write no CRC! RINTSTS=0x%08x\n", intsts);
				//rst = 1;
				sdmmc_info->write_no_crc = 1;
			}
			break;
		}
		else if(finished == 0)	// timeout
		{
			if(intsts & (1 << 15))	// write no CRC (EBE)
			{
				DEBUG_MSG("Write DMA err, write no CRC! RINTSTS=0x%08x\n", intsts);
				//rst = 1;
				//break;
				sdmmc_info->write_no_crc = 1;
			}
			// 检查SD卡是否存在
			/*
			//DEBUG_MSG ("sd_write_dma ch(%d) timeout, time=%d, len=%d\n", sdmmc_info->ChipSel, DmaTime, len);
			if((SDCard.Present == 0) && (sdmmc_info->ChipSel == CHIP0))
			{
				DEBUG_MSG("Card 0 Removed!!!!!!!\n");
				rst = 1;
				break;
			}
#if SDMMC_DEV_COUNT > 1
			else if((SDCard1.Present == 0) && (sdmmc_info->ChipSel == CHIP1))
			{
				DEBUG_MSG("Card 1 Removed!!!!!!!\n");
				rst = 1;
				break;
			}
#endif
			*/
			if(!is_card_insert(sdmmc_info))
			{
				DEBUG_MSG("Card %d Removed!\n", sdmmc_info->ChipSel);
				rst = 1;
				break;				
			}
			
			// 20170707 发现卡快速插拔时出现异常, DMA执行未能结束. 加入超时机制
			if(XM_GetTickCount() >= ticket_to_timeout)
			{
				DEBUG_MSG("sd_write_multi_buffer_through_dma failed, DMA timeout\n");
				sdmmc_info->reinit = 1;		// 标记卡需要复位
				rst = 1;
				break;
			}
			
		}
	}
	
	dma_clr_int(SdDmaCh);
	//card_dma_detect_ch_disable(sdmmc_info, SdDmaCh);
	dma_release_channel(SdDmaCh);
	sdmmc_info->lli = 0;
	
	if(SDHC_REG_READ32(sdmmc_info,SDMMC_RINTSTS) & RINTSTS_DCRC)
	{
		sdmmc_info->crc_error = 1;
	}

	return rst;	
}
#endif

INT32 SD_WriteMultiBuffer(SDMMC_INFO * sdmmc_info, ULONG  Sector, ULONG  NumSectors, ULONG NumBuffers, const UINT8 **pSectorBuffer, ULONG *pSectorCount)
{
#if 0//def USE_DMA_INTERFACE 	
	INT32 result = SDMMC_RW_SUCCES;
	UINT32 val;
	UINT32  ret;
	U32 i;
	U8 *data_buf;
	UINT32 blk_count;
	UINT32 start_block;
	int fatal_busy_error = 0;		//  1 means card busy forever
	
	//DEBUG_MSG("SD_WriteMultiBuffer, start_block=0x%08x, blk_count=0x%08x\n", Sector, NumSectors);

	
	val = (3<<28)|16;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_FIFOTH, val);
	
	for (i = 0; i < NumBuffers; i ++)
	{
		data_buf = (U8 *)pSectorBuffer[i];
		blk_count = pSectorCount[i];
		dma_clean_range((UINT32)data_buf, (UINT32)data_buf + blk_count*512);
	}
	
	select_sd_pad(sdmmc_info);
		
	start_block = Sector;
	blk_count = NumSectors;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BLKSIZ, block_len);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BYTCNT, block_len*blk_count);
  	if(sdmmc_info->Capacity == Standard)
		start_block = start_block*block_len;
	
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, start_block);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD25_WRITE_MUL);
	//if(sd_command_done(sdmmc_info))
	if(sd_command_done_no_clear_RINTSYS(sdmmc_info))
	{
		DEBUG_MSG("SD_WriteMultiBuffer failed. send CMD25_WRITE_MUL NG\n");
		result = SDMMC_RW_CMDTIMEROUT;
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);
		return result;
		//goto ERR_HANDLER;
	}
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0);
	if(val & ((1 << 31)|(1<<30)|(1<<29)|(1<<23)|(1<<22)|(1<<21)|(1<<20)|(1<<19)))
	{
		XM_printf ("SD_WriteMultiBuffer, SD CMD25_WRITE_MUL RESP0=0x%08x, Sector=%d, NumSectors=%d\n", val, Sector, NumSectors);
	}
	
	if(sd_write_multi_buffer_through_dma (sdmmc_info, NumBuffers, (const UINT8 **)pSectorBuffer, pSectorCount))
	{
		DEBUG_MSG("SD_WriteMultiBuffer failed. dma transfer NG\n");
		result = SDMMC_RW_DMAREAD_FAIL;
		goto ERR_HANDLER;		
	}
	// 20170818 修复金士顿64GB卡读写异常的缺陷
	if(busy_data(sdmmc_info))
	{
		result = SDMMC_RW_TIMEOUT;
		DEBUG_MSG("SD_WriteMultiBuffer failed, busy_data NG\n");
		goto ERR_HANDLER;	
	}
	ret = data_tran_over(sdmmc_info);
	if(ret)
	{
		DEBUG_MSG("SD_WriteMultiBuffer failed, write pages not finished.\n");
		result = SDMMC_RW_TIMEOUT;
		goto ERR_HANDLER;
	}
	
	// a) The last busy in any write operation up to 500ms including single and multiple block write.
	// b) When multiple block write is stopped by CMD12, the busy from the response of CMD12 is up to 500ms.
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD12_STOP_STEARM);
	if(sd_command_done(sdmmc_info))
	{
		result = SDMMC_RW_CMDTIMEROUT;
		DEBUG_MSG("SD_WriteMultiBuffer failed, CMD12_STOP_STEARM send NG\n");
		goto ERR_HANDLER;
	}
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0);
	if(val & ((1 << 31)|(1<<30)|(1<<29)|(1<<23)|(1<<22)|(1<<21)|(1<<20)|(1<<19)))
	{
		XM_printf ("SD CMD12_STOP_STEARM RESP0=0x%08x, Sector=%d, NumSectors=%d\n", val, Sector, NumSectors);
	}
	// 检测Busy Complete Interrupt, 按照上面b)的说明，最大500ms
  	if(busy_data(sdmmc_info))
  	{
		result = SDMMC_RW_TIMEOUT;
		DEBUG_MSG("SD_WriteMultiBuffer failed, busy_data NG\n");
		fatal_busy_error = 1;
		goto ERR_HANDLER;
  	}

	if(sdmmc_info->crc_error)
	{
		sdmmc_info->crc_error = 0;		// 清除crc标志
		DEBUG_MSG("SD_WriteMultiBuffer failed. data crc error\n");
		result = SDMMC_RW_TIMEOUT;
	}

	return result;
ERR_HANDLER:
	
	sdmmc_info->crc_error = 0;		// 清除crc标志
	
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_STATUS);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);
	
	if(val & ((0x7fff<<16)|(0x3<<9)|(0xf<<4)))
	{
		DEBUG_MSG("SD write data status is error status is 0x%x.\n", val);
	}
	
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD12_STOP_STEARM);
	if(sd_command_done(sdmmc_info))
	{
		DEBUG_MSG("SD_WriteMultiBuffer. CMD12_STOP_STEARM send NG\n");
	}
	
	if(sdmmc_info->reinit)
	{
		XM_printf ("SD_WriteMultiBuffer fatal error, re-init\n");
		fatal_busy_error = 1;
		sdmmc_info->reinit = 0;
	}
	fifo_dma_reset (sdmmc_info);

	if(fatal_busy_error)
	{
		// 检查卡是否已拔出. 若是, 不发出REINIT事件
		if(is_card_insert(sdmmc_info))
		{
			// 需要卡重新识别(发送CMD0指令来复位SD卡)
			// 将卡状态置为不存在, 以便文件系统层快速退出, 重新执行REINIT指令
			//sdmmc_info->card_present = 0;
			// 检查卡是否正在执行REINIT, 若是, 阻止REINIT消息递归投递
			if(sdmmc_info->do_reinit == 0)
			{
				PostCardEvent (SDCARD_REINIT, sdmmc_info->ChipSel);
			}
		}
	}
	
	return result;
	
#else
	
	ULONG i;
	int ret = 0;
	for (i = 0; i < NumBuffers; i ++)
	{
		if(SD_WriteSector (sdmmc_info, 0, Sector, (UINT8 *)pSectorBuffer[i], pSectorCount[i], 0) < 0)
		{
			ret = -1;
			break;
		}
		Sector += pSectorCount[i];
	}
	
	return ret;
#endif
}


#if 0//def USE_DMA_INTERFACE 	

static INT32 sd_read_multi_buffer_through_dma (SDMMC_INFO * sdmmc_info, UINT32 NumBuffers, const UINT8 **pSectorBuffer, ULONG *pSectorCount)
{
	UINT32 SdDmaCh;
	UINT32 DmaTime = 0;
	INT32 rst = 0;
	unsigned int intsts;
	UINT32  len;
	unsigned int ticket_to_timeout;
	      	
	SdDmaCh = sdmmc_dma_channel[sdmmc_info->ChipSel - CHIP0];
	sdmmc_info->SdDmaCh = SdDmaCh;
	
	// 创建DMA链表
	sdmmc_info->lli = sdmmc_dma_lli[sdmmc_info->ChipSel - CHIP0];
	
	dma_addr_t	sar = sdmmc_info->sdmmc_reg_info->SDHC_Control + SDMMC_FIFO;
	struct dw_lli *	lli = &sdmmc_info->lli[0];
	while(NumBuffers > 0)
	{
		UINT32 left;
		int i;
		dma_addr_t dst_addr = (dma_addr_t)*pSectorBuffer;
		int lli_count;
		
		len = *pSectorCount * 512;
		left = len;
		lli_count = (len + MAX_BATCH_TRANS_SECTOR*512 - 1) / (MAX_BATCH_TRANS_SECTOR*512);
		
		for (i = 0; i < lli_count; i++)
		{
			UINT32  to_transfer;
			lli->sar = sar;
			lli->dar = dst_addr;
			//if(i != (lli_count - 1))
			//	lli->llp = (UINT32)&sdmmc_info->lli[i + 1];
			//else
			//	lli->llp = 0;
			lli->llp = (UINT32)(lli + 1);
			lli->ctllo = SD_RX_DMA_CTL_L | (1 << 27);	// LLP_SRC_EN
			if(left >= MAX_BATCH_TRANS_SECTOR*512)
				to_transfer = MAX_BATCH_TRANS_SECTOR*512;
			else
				to_transfer = left;
			lli->ctlhi = SD_RX_DMA_CTL_H | (to_transfer / 4);
			lli->sstat = 0;
			lli->dstat = 0;
			dst_addr += to_transfer;
			left -= to_transfer;
			lli ++;
		}		
		
		pSectorCount ++;
		pSectorBuffer ++;
		NumBuffers --;
	}
	
	(lli - 1)->llp = 0;
	
	dma_clr_int(SdDmaCh);
	dma_cfg_channel(SdDmaCh, 
					sdmmc_info->lli[0].sar,
					sdmmc_info->lli[0].dar,
					(UINT32)(&sdmmc_info->lli[0])&0xfffffffc,
					sdmmc_info->lli[0].ctllo,
					sdmmc_info->lli[0].ctlhi,
					SD_RX_DMA_CFG_L | (SdDmaCh << 5), (SD_RX_DMA_CFG_H_LLI | (sdmmc_info->dma_req << 7)),
					0,
					0);	
	dma_flush_range ((UINT32)&sdmmc_info->lli[0], (UINT32)lli);
	
   register_channel_IRQHandler (SdDmaCh, 
										  sdmmc_dma_transfer_over_IRQHandler_lli, 
										  sdmmc_info,
										  //(1<< BLOCK_INT_BIT)|(1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  (1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  );
	
	sdmmc_info->crc_error = 0;
	DmaTime = 5;
	/*
	DmaTime = 2000 * (len / (MAX_BATCH_TRANS_SECTOR*512));
	if(DmaTime < 10)
		DmaTime = 10;
	else if(DmaTime > 500)
		DmaTime = 500;*/
	dma_start_channel(SdDmaCh);
	
	ticket_to_timeout = XM_GetTickCount() + 5000;
	
	while(!rst)
	{
		//unsigned int rintsts;
		int finished = Dma_TfrOver_PendSemEx (SdDmaCh, DmaTime);
		intsts = SDHC_REG_READ32(sdmmc_info,SDMMC_RINTSTS);
		if(finished == 1)
		{
			// 检查DMA执行的结果
			if(sdmmc_info->ret)
			{
				rst = 1;
			}
			break;
		}
		else if(finished == 0)	// timeout
		{
			if(intsts & (1 << 9))	// bit 9 – Data read timeout (DRTO)
			{
				DEBUG_MSG("Read DMA err, Data read timeout! RINTSTS=0x%08x\n", intsts);
				rst = 1;
				
				// 20170301 异常时stop dma, 避免随后的card_dma_detect_ch_disable挂死
				dma_stop_channel (SdDmaCh);
				
				break;
			}
			// 检查SD卡是否存在
			/*
			//DEBUG_MSG ("sd_write_dma ch(%d) timeout, time=%d, len=%d\n", sdmmc_info->ChipSel, DmaTime, len);
			if((SDCard.Present == 0) && (sdmmc_info->ChipSel == CHIP0))
			{
				DEBUG_MSG("Card 0 Removed!!!!!!!\n");
				rst = 1;
				break;
			}
#if SDMMC_DEV_COUNT > 1
			else if((SDCard1.Present == 0) && (sdmmc_info->ChipSel == CHIP1))
			{
				DEBUG_MSG("Card 1 Removed!!!!!!!\n");
				rst = 1;
				break;
			}
#endif
			*/
			if(!is_card_insert(sdmmc_info))
			{
				// card removed
				DEBUG_MSG("Card %d Removed!\n", sdmmc_info->ChipSel);	
				rst = 1;
				break;
			}
			
			// 20170707 发现卡快速插拔时出现异常, DMA执行未能结束. 加入超时机制
			if(XM_GetTickCount() >= ticket_to_timeout)
			{
				DEBUG_MSG("sd_read_multi_buffer_through_dma failed, DMA timeout\n");
				rst = 1;
				break;
			}
			
		}
	}
	
	dma_clr_int(SdDmaCh);
	//card_dma_detect_ch_disable(sdmmc_info, SdDmaCh);
	if(card_dma_detect_ch_disable(sdmmc_info, SdDmaCh) != 0)
	{
		rst = 1;
	}
	
	dma_release_channel(SdDmaCh);
	sdmmc_info->lli = 0;
	
	// bit 7 – Data CRC error (DCRC)
	// Data CRC error – During a read-data-block transfer, if the CRC16 received does not match with
	// the internally generated CRC16, the data path signals a data CRC error to the BIU and continues
	// further data transfer.	
	if(intsts & (1 << 7))
	{
		DEBUG_MSG("Read DMA err, Data CRC error\n");
		//rst = 1;
		sdmmc_info->crc_error = 1;	// 标记CRC错误
	}

	return rst;	
}
#endif

INT32 SD_ReadMultiBuffer(SDMMC_INFO * sdmmc_info, ULONG  Sector, ULONG  NumSectors, ULONG NumBuffers, const UINT8 **pSectorBuffer, ULONG *pSectorCount)
{
#if 0//def USE_DMA_INTERFACE 	
	INT32 result = SDMMC_RW_SUCCES;
	UINT32 val;
	UINT32  ret;
	U32 i;
	U8 *data_buf;
	UINT32 blk_count;
	UINT32 start_block;
	int crc_error = 0;
	
	//DEBUG_MSG("SD_ReadMultiBuffer, start_block=0x%08x, blk_count=0x%08x\n", Sector, NumSectors);
	
	// invalidate d-cache
	for (i = 0; i < NumBuffers; i ++)
	{
		data_buf = (U8 *)pSectorBuffer[i];
		blk_count = pSectorCount[i];
		dma_inv_range((UINT32)data_buf, (UINT32)data_buf + blk_count*512);
	}
	
	select_sd_pad(sdmmc_info);

	val = (3<<28) | ((FIFO_SIZE/2 - 1) << 16) | (FIFO_SIZE/2);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_FIFOTH, val);
	
	// timeout
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_TMOUT);
	val &=~0xFFFFFF00;
	val |= sdmmc_info->ReadTimeOutCycle<<8;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_TMOUT, val);
	
	start_block = Sector;
	blk_count = NumSectors;
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BLKSIZ, block_len);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_BYTCNT, block_len*blk_count);
  	if(sdmmc_info->Capacity == Standard)
		start_block = start_block*block_len;	
	
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMDARG, start_block);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD18_READ_MUL);
	
	//if(sd_command_done(sdmmc_info))
	if(sd_command_done_no_clear_RINTSYS(sdmmc_info))
	{
		DEBUG_MSG("SD_ReadMultiBuffer failed. send CMD18_READ_MUL NG\n");
		result = SDMMC_RW_CMDTIMEROUT;
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);
		return result;
		//goto ERR_HANDLER;
	}
	
	// 检查Card Status
	// The response format R1 contains a 32-bit field named card status.
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_RESP0);
	if(val & ((1 << 31)|(1<<30)|(1<<29)|(1<<23)|(1<<22)|(1<<21)|(1<<20)|(1<<19)))
	{
		XM_printf ("SD_ReadMultiBuffer, SD CMD18_READ_MUL RESP0=0x%08x\n", val);
	}
	
	sdmmc_info->crc_error = 0;
	ret = sd_read_multi_buffer_through_dma (sdmmc_info, NumBuffers, (const UINT8 **)pSectorBuffer, pSectorCount);
	crc_error = sdmmc_info->crc_error;
	sdmmc_info->crc_error = 0;
	if(ret)
	{
		DEBUG_MSG("SD_ReadMultiBuffer failed. dma transfer NG, start_block=0x%08x, blk_count=0x%08x\n", start_block, blk_count);
		result = SDMMC_RW_DMAREAD_FAIL;
		goto ERR_HANDLER;
	}
	
	// 20170818 修复金士顿64GB卡读写异常的缺陷
  	if(busy_data(sdmmc_info))
	{
		DEBUG_MSG("SD_ReadMultiBuffer failed. busy data timeout\n");
		result =  SDMMC_RW_TIMEOUT;
		goto ERR_HANDLER;
  	}
	
	// invalidate d-cache
	for (i = 0; i < NumBuffers; i ++)
	{
		data_buf = (U8 *)pSectorBuffer[i];
		blk_count = pSectorCount[i];
		dma_inv_range((UINT32)data_buf, (UINT32)data_buf + blk_count*512);
	}
	
	ret = data_tran_over(sdmmc_info);
  	if(ret)
	{
		DEBUG_MSG("SD_ReadMultiBuffer failed, data_tran_over ng, CHIP is %d\n", sdmmc_info->ChipSel);
		result =  SDMMC_RW_TIMEOUT;
		goto ERR_HANDLER;
  	}
	if(result == SDMMC_RW_SUCCES && crc_error)
	{
		// 检查crc错误, 标记失败
		DEBUG_MSG ("SD_ReadMultiBuffer failed, DMA READ CRC ERROR\n");
		result = SDMMC_RW_DMAREAD_FAIL;
		
		// 清除错误
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);		
	}
	
	return result;
	
ERR_HANDLER:
	val = SDHC_REG_READ32(sdmmc_info, SDMMC_STATUS);
	if(val & ((0x7fff<<16)|(0x3<<9)|(0xf<<4)))
		DEBUG_MSG("SD read data status is error status is 0x%x.\n", val);
	
	// 20170511 fix bug	
	val = SDHC_REG_READ32(sdmmc_info,SDMMC_RINTSTS);
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);
	if(val & RINTSTS_DRTO)
	{
		// After the NACIO timeout, the application must abort the command by sending the CCSD and STOP commands, or the STOP command.
		// bit 9 – Data read timeout (DRTO)/Boot Data Start (BDS)
		//DEBUG_MSG("sd_mmc_read_dma. CMD12_STOP_STEARM send\n");
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD12_STOP_STEARM);
		if(sd_command_done(sdmmc_info))
		{
			DEBUG_MSG("sd_mmc_read_dma. CMD12_STOP_STEARM send NG\n");
		}		
	}
	if(val & RINTSTS_SBE)
	{
		// Data start bit error – During a 4-bit or 8-bit read-data transfer, if SBE occurs, the
		// application/driver must issue CMD12 for SD/MMC cards and CMD52 for SDIO card in order to
		// come out of the error condition. After CMD done is received, the application/driver must reset
		// IDMAC and CIU (if required) to clear the condition. FIFO reset must be done before issuing any
		// data transfer commands in general. Register CNTRL bits 2:0 control these resets:	
		//DEBUG_MSG("sd_mmc_read_dma. CMD12_STOP_STEARM send\n");
		SDHC_REG_WRITE32(sdmmc_info, SDMMC_CMD, CMD12_STOP_STEARM);
		if(sd_command_done(sdmmc_info))
		{
			DEBUG_MSG("sd_mmc_read_dma. CMD12_STOP_STEARM send NG\n");
		}		
		
		fifo_dma_reset (sdmmc_info);		
	}
	SDHC_REG_WRITE32(sdmmc_info, SDMMC_RINTSTS, 0x0000ffff);
	
	return result;
#else

	ULONG i;
	int ret = 0;
	for (i = 0; i < NumBuffers; i ++)
	{
		if(SD_ReadSector (sdmmc_info, 0, Sector, (UINT8 *)pSectorBuffer[i], pSectorCount[i]) < 0)
		{
			ret = -1;
			break;
		}
		Sector += pSectorCount[i];
	}
	
	return ret;
	
#endif
}


#include "gpio.h"
static void sdcard_power_on_reset (void)
{
	// pad_ctl8
	// CD2
	// [5:4]	cd2	cd2_pad	GPIO82	nand_data[10]
	unsigned int val;
    #if 0
	val = rSYS_PAD_CTRL08;
	val &= ~(0x3 << 4);
	rSYS_PAD_CTRL08 = val;
	SetGPIOPadDirection (GPIO82, euOutputPad);
	SetGPIOPadData (GPIO82, euDataLow);
    #endif
	//OS_Delay (10);
	for (int i = 0; i < 0xa00000; i ++);//原来是0X200000有时候上电识别不了卡
	SetGPIOPadData (GPIO82, euDataHigh);
	for (int i = 0; i < 0xa00000; i ++);
	//OS_Delay (10);
}


static void sdcard_0_reinit (void)
{
	UINT32 val;
	XM_lock ();
	if(SDHC_REG_READ32(sdmmc_chip0, SDMMC_CDETECT) & 0x1)
	{
		// 卡拔出
		if(lg_pSDMMC0_DetheringTimer)
		{
			StopHardwareTimer(lg_pSDMMC0_DetheringTimer);
		}
		lg_ulCard0DeDitheringStatus = CARD_DEDIGHERING_STATUS_IDLE;
		DEBUG_MSG ("OUT STATUS_IDLE\n");
	}
	else
	{
		// 卡插入
		if(lg_pSDMMC0_DetheringTimer)
		{
			StopHardwareTimer(lg_pSDMMC0_DetheringTimer);
		}
		lg_ulCard0DeDitheringStatus = CARD_DEDIGHERING_STATUS_PASS;
		DEBUG_MSG ("IN STATUS_PASS\n");
	}
	//lg_ulCard0DeDitheringStatus = CARD_DEDIGHERING_STATUS_PASS;
	
	// 关闭SD卡中断
	request_irq(SDHC0_INT, PRIORITY_FIFTEEN, 0);
	XM_unlock ();
	
	OS_EnterRegion ();
	// 关闭卡时钟并复位
	sys_clk_disable (sdc_hclk_enable);
	sys_soft_reset (softreset_card);
	sys_clk_enable (sdc_hclk_enable);
	OS_LeaveRegion ();  // 20181112 add
	
	sdcard_power_on_reset(); // 20181112 放开对卡的电源的控制

	OS_EnterRegion (); // 20181112 add
	
	select_sd_pad(sdmmc_chip0);
	
	Select_Card_SrcClk(sdmmc_chip0,CPU_PLL,18);// 400M/18
	
	SDHC_REG_WRITE32(sdmmc_chip0, SDMMC_RINTSTS, 0xFFFFFFFF);
	
	//卡拔插去抖时间设置为30毫秒
	val = Get_Card_SrcClk(sdmmc_chip0);
	//DEBUG_MSG("Get_Card_SrcClk = %d\r\n", val);
	val = val / 1000;
	val = 30 * val;
	//DEBUG_MSG("val = 0x%x\r\n", val);
	SDHC_REG_WRITE32(sdmmc_chip0, SDMMC_DEBNCE, val);
	
	val = SDHC_REG_READ32(sdmmc_chip0, SDMMC_CTRL);
	val |= (1<<4);
	SDHC_REG_WRITE32(sdmmc_chip0, SDMMC_CTRL, val);
	
	val = SDHC_REG_READ32(sdmmc_chip0, SDMMC_INTMASK);
	val |= (1<<0);// enable  detect interrupt
	SDHC_REG_WRITE32(sdmmc_chip0, SDMMC_INTMASK, val);
	
	request_irq(SDHC0_INT, PRIORITY_FIFTEEN, SDMMC_IRQ_Handler0);
	
	OS_LeaveRegion ();
}

INT32 SetSDMMCCardDoReInit (UINT32 ulCardID, int set)
{
	SDMMC_INFO * sdmmc_info;

	switch(ulCardID)
	{
		case 0:
			sdmmc_info = sdmmc_chip0;
			break;
#if SDMMC_DEV_COUNT > 1
		case 1:
			sdmmc_info = sdmmc_chip1;
			break;
#endif

		default:
			return -1;
	}
	// 强制重新初始化
	XM_lock ();
	sdmmc_info->do_reinit = set;
	XM_unlock ();
	return 0;
}



INT32 ResetSDMMCCard (UINT32 ulCardID)
{
	SDMMC_INFO * sdmmc_info;

	switch(ulCardID)
	{
		case 0:
			sdmmc_info = sdmmc_chip0;
			break;
#if SDMMC_DEV_COUNT > 1
		case 1:
			sdmmc_info = sdmmc_chip1;
			break;
#endif

		default:
			return -1;
	}
	
	// 强制重新初始化
	XM_lock ();
	sdmmc_info->init_cardflag_sd = 0;
	sdmmc_info->card_present = 0;
	XM_unlock ();
	if(ulCardID == 0)
		sdcard_0_reinit ();
	
	return 0;
}

void SDCard_Module_Init(void)
{
	UINT32 val;
	int i;
	
	// 关闭SD卡中断
	request_irq(SDHC0_INT, PRIORITY_FIFTEEN, 0);
#if SDMMC_DEV_COUNT > 1
	request_irq(SDHC1_INT, PRIORITY_FIFTEEN, 0);
#endif
	
	// 关闭卡时钟并复位
	sys_clk_disable (sdc_hclk_enable);
	sys_soft_reset (softreset_card);
	sys_clk_enable (sdc_hclk_enable);

	sdcard_power_on_reset (); // 20181112 放开对卡的电源的控制

#if SDMMC_DEV_COUNT > 1
	sys_clk_disable (sdc1_hclk_enable);
	sys_soft_reset (softreset_card1);
	sys_clk_enable (sdc1_hclk_enable);
#endif
	
#ifndef _XMSYS_FS_SDMMC_SUPPORT_
	XM_printf ("SDMMC don't support\n");
	return;
#endif
		
	//XM_printf ("SDMMC_DEV_COUNT=%d\n", SDMMC_DEV_COUNT);
	
	sdmmc_chip0->ChipSel = CHIP0;
#if SDMMC_DEV_COUNT > 1
	sdmmc_chip1->ChipSel = CHIP1;
#endif
	
	sdmmc_chip0->sdmmc_reg_info = &sdmmc_reg_info_chip0;
#if SDMMC_DEV_COUNT > 1
	sdmmc_chip1->sdmmc_reg_info = &sdmmc_reg_info_chip1;
#endif
	
	sdmmc_chip0->sdmmc_reg_info->SDHC_Control = SDHC_Controls[0];
#if SDMMC_DEV_COUNT > 1
	sdmmc_chip1->sdmmc_reg_info->SDHC_Control = SDHC_Controls[1];
#endif

	sdmmc_chip0->dma_req = REQ_SDMMC0;
#if SDMMC_DEV_COUNT > 1
	sdmmc_chip1->dma_req = REQ_SDMMC1;
#endif
	
#if SDMMC_PRINT_PARAMETER
	// 仅在第一次复位的时候读出的FIFO_DEPTH参数值有效
	XM_printf ("\nSDMMC Controller Parameter\n");
	const int H_DATA_WIDTH[] = {16, 32, 64, 0, 0, 0, 0, 0};
	XM_printf ("\tVERID = 0x%08x\n", SDHC_REG_READ32(sdmmc_chip0, SDMMC_VERID));
	// FIFO_DEPTH parameter
	// For FIFO_DEPTH parameter, power-on value of RX_WMark value of FIFO Threshold Watermark Register represents FIFO_DEPTH - 1.
	// 27:16 RX_WMark
	// FIFO_DEPTH = 32 (ARKN141当前的值)
	unsigned int reg = SDHC_REG_READ32(sdmmc_chip0, SDMMC_FIFOTH);
	XM_printf ("\tFIFO_DEPTH      = %d\n", 1 + ((reg >> 16) & 0xFFF) );
	// bit [9:7] H_DATA_WIDTH
	// 32bit (ARKN141当前的值)
	reg = SDHC_REG_READ32(sdmmc_chip0, SDMMC_HCON);
	XM_printf ("\tH_DATA_WIDTH    = %d bits\n", H_DATA_WIDTH[(reg >> 7) & 0x07]);
	XM_printf ("\n\n");
#endif

	select_sd_pad(sdmmc_chip0);
	
#if SDMMC_DEV_COUNT > 1
	select_sd_pad(sdmmc_chip1);
#endif
	
	Select_Card_SrcClk(sdmmc_chip0,CPU_PLL,18);// 400M/18
	
#if SDMMC_DEV_COUNT > 1
	Select_Card_SrcClk(sdmmc_chip1,CPU_PLL,18);// 400M/18
#endif
	SDHC_REG_WRITE32(sdmmc_chip0, SDMMC_RINTSTS, 0xFFFFFFFF);
	
#if SDMMC_DEV_COUNT > 1
	SDHC_REG_WRITE32(sdmmc_chip1, SDMMC_RINTSTS, 0xFFFFFFFF);
#endif

	//卡拔插去抖时间设置为30毫秒
	val = Get_Card_SrcClk(sdmmc_chip0);
	//DEBUG_MSG("Get_Card_SrcClk = %d\r\n", val);
	val = val / 1000;
	val = 30 * val;
	//DEBUG_MSG("val = 0x%x\r\n", val);
	SDHC_REG_WRITE32(sdmmc_chip0, SDMMC_DEBNCE, val);

#if SDMMC_DEV_COUNT > 1
	val = Get_Card_SrcClk(sdmmc_chip1);
	//DEBUG_MSG("Get_Card_SrcClk = %d\r\n", val);
	val = val / 1000;
	val = 30 * val;
	//DEBUG_MSG("val = 0x%x\r\n", val);	
	SDHC_REG_WRITE32(sdmmc_chip1, SDMMC_DEBNCE, val);
#endif
 
	val = SDHC_REG_READ32(sdmmc_chip0, SDMMC_CTRL);
	val |= (1<<4);
	SDHC_REG_WRITE32(sdmmc_chip0, SDMMC_CTRL, val);

#if SDMMC_DEV_COUNT > 1
	val = SDHC_REG_READ32(sdmmc_chip1, SDMMC_CTRL);
	val |= (1<<4);
	SDHC_REG_WRITE32(sdmmc_chip1, SDMMC_CTRL, val);
#endif
	
	val = SDHC_REG_READ32(sdmmc_chip0, SDMMC_INTMASK);
	val |= (1<<0);// enable  detect interrupt
	SDHC_REG_WRITE32(sdmmc_chip0, SDMMC_INTMASK, val);

#if SDMMC_DEV_COUNT > 1
	val = SDHC_REG_READ32(sdmmc_chip1, SDMMC_INTMASK);
	val |= (1<<0);// enable  detect interrupt
	SDHC_REG_WRITE32(sdmmc_chip1, SDMMC_INTMASK, val);
#endif

	InitDeditheringTimer();
	
	// 申请DMA读写通道及分配必须的内存资源
	// 不再动态申请及分配以上资源，避免APP缺陷(导致heap资源耗尽或者DMA通道全部被占用)引入的风险
	//		1) 无法正确保存及关闭文件
	//		2) 系统无法关闭FS系统
	for (i = 0; i < SDMMC_DEV_COUNT; i ++)
	{
		// 按照最大的Cache块长度分配
#ifdef USE_DMA_INTERFACE		
		unsigned int lli_size = sizeof(struct dw_lli) * SDMMC_MAX_BLOCK_SIZE / (MAX_BATCH_TRANS_SECTOR*512);
		lli_size = (lli_size + 0x3F) & ~0x3F;	// 2 Cache Line aligned(32 bytes one cache line, cortex a5)
		sdmmc_dma_lli_base[i] = kernel_malloc (lli_size);
		if(sdmmc_dma_lli_base[i])
		{
			// 地址32字节对齐
			sdmmc_dma_lli[i] = (struct dw_lli *) ((((unsigned int)sdmmc_dma_lli_base[i]) + 0x1F) & (~0x1F));
		}
		else
		{
			sdmmc_dma_lli[i] = NULL;
			DEBUG_MSG ("Fatal Error, SD request dma lli failed!\n");
			XM_TraceKernelErrCode (XM_KERNEL_ERR_CODE_SD_DMA_LLI_MALLOC);
		}
		
		sdmmc_dma_channel[i] = dma_request_dma_channel(SDMMC_DMA_FAVORITE_CHANNEL);
		if(sdmmc_dma_channel[i] == ALL_CHANNEL_RUNNING)
		{
			DEBUG_MSG ("Fatal Error, SD request dma channel failed!\n");
			XM_TraceKernelErrCode (XM_KERNEL_ERR_CODE_SD_DMA_CHANNEL_REQUEST);
		}
#endif
		
		OS_EVENT_Create (&sdmmc_event[i]);
		
		OS_EVENT_Create (&sdmmc_busy_ticket_event[i]);
	}
	
	request_irq(SDHC0_INT, PRIORITY_FIFTEEN, SDMMC_IRQ_Handler0);
#if SDMMC_DEV_COUNT > 1
	request_irq(SDHC1_INT, PRIORITY_FIFTEEN, SDMMC_IRQ_Handler1);
#endif
	
}

