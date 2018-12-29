/*
**********************************************************************
Copyright (c)2007 Arkmicro Technologies Inc.  All Rights Reserved 
Filename: sdmmc.h
Version : 1.1 
Date    : 2012.12.20
Author  : ls
Abstract: ark1660  soc sd driver
History :
***********************************************************************
*/
#ifndef SDMMC_H
#define SDMMC_H

#include "types.h"
#include "Fs_card.h"
#include "xm_proj_define.h"

#ifdef __cplusplus
extern "C" {
#endif



#define SD_DEBUG                    			1
//#define SD_DEBUG                    			0	

#ifdef _XMSYS_DMA_SUPPORT_
#define USE_DMA_INTERFACE  
#endif
	
#ifdef USE_DMA_INTERFACE
#define	_SDMMC_USE_LLI_DMA_
#endif

#define INIT_CLOCK						150000

#define TIMEOUT_SD						5000000
	
#define SDMMC_MAX_BLOCK_SIZE			0x400000			// SDMMC驱动支持的最大块长度

#define SDMMC_CARD_INSERTED			1
#define SDMMC_CARD_REMOVED			0

#define CLK_24M							0
#define SYS_PLL							1
#define CLK_240M							2
#define CPU_PLL							3

#define SDMMC_RW_SUCCES				0
#define SDMMC_RW_TIMEOUT				-1
#define SDMMC_RW_DMAREAD_FAIL		-2
#define SDMMC_RW_DMAWRITE_FAIL		-3
#define SDMMC_RW_CMDTIMEROUT			-4
#define SDMMC_RW_CARDREMOVED			-5
	
#define SDCARD_REMOVE		0
#define SDCARD_INSERT		1
#define SDCARD_REINIT		2			// 卡重新初始化
	

typedef enum {CHIP0 = 0, CHIP1, CHIP2}CHIPSEL;
typedef enum {MMC, SD}CARDTYPE;
//static CARDTYPE CardType = SD; // 1 sd card, 0 means MMC
typedef enum {Ver0, Ver1, Ver2}CARDSPEC;
//static CARDSPEC Spec = Ver1;
typedef enum {Standard, High}CAPACITY;
//static CAPACITY Capacity = Standard;

typedef struct
{
	UINT32 SDHC_Control;
	UINT32 sd_cid[4];
	UINT32 sd_csd[4];
	UINT32 sd_rca;
}SDMMC_REG_INFO;

typedef struct
{
	SDMMC_REG_INFO *	sdmmc_reg_info;
	CHIPSEL				ChipSel;
	CARDTYPE			CardType;
	CARDSPEC			CardSpec;
	CAPACITY			Capacity;
	volatile UINT16				card_present;
	UINT16				init_cardflag_sd;
	UINT16				blocksize;
	UINT16				dma_req; 
	UINT8				sd_clk_source;
	UINT32				sd_clk;
	UINT32				ReadTimeOutCycle;
	UINT32				WriteTimeOutCycle;
#ifdef _NEW_SDMMC_DMA_DRIVER_
   UINT32            SdDmaCh;    // DMA通道
	unsigned int 	   buf;				// 读写缓冲区地址	
	unsigned int	   len;				// 读写字节长度
	unsigned char	   is_read_mode;  // 1 从SD卡读 0写入到SD卡
	int				   ret;				// 返回值。0 成功 < 0 异常
	
	int					crc_error;
	
	int					write_no_crc;
	
	int					reinit;		// sd卡需要执行复位操作
	
	int					debug_flag;
	
	int					do_reinit;	// 1 标记正在重新执行卡初始化过程, 过程中禁止重新投递REINIT消息
	
	struct dw_lli *	lli;
	
#endif   
}SDMMC_INFO;



extern SDMMC_INFO * sdmmc_chip0;
extern SDMMC_INFO * sdmmc_chip1;
extern SDMMC_INFO * sdmmc_chip2;

extern volatile FS_CARD SDCard;
extern volatile FS_CARD SDCard1;
extern volatile FS_CARD SDCard2;

#define	REPEAT_COUNT_OF_FAILED_COMMAND		3		// 命令错误重发次数

#define	SDMMC_DATA_TIMEOUT_MS					1000//750//500	// 按最大写超时500ms
																	// It is strongly recommended for hosts to implement more than 500ms timeout value even if the card 	 
																	//		indicates the 250ms maximum busy length. 	 


// Raw Interrupt Status Register
#define RINTSTS_CD			(1 <<  0)			// bit 0 – Card detect (CD)
#define RINTSTS_RE			(1 <<  1)			// bit 1 – Response error (RE)
															// 	Response errors – Set when an error is received during response reception. In this case, the response
															//		that copied in the response registers is invalid. Software can retry the command.
															//		Error in received response set if one of following occurs:
															//		■ Transmission bit != 0
															//		■ Command index mismatch
															//		■ End-bit != 1
#define RINTSTS_CMDDONE		(1 <<  2)			// bit 2 – Command done (CD)
															//		Command sent to card and got response from card, even if Response Error
															//		or CRC error occurs. Also set when response timeout occurs or CCSD sent	to CE-ATA device.		
#define RINTSTS_DTO			(1 <<  3)			// bit 3 – Data transfer over (DTO)
#define RINTSTS_TXDR			(1 <<  4)			// bit 4 – Transmit FIFO data request (TXDR)
#define RINTSTS_RXDR			(1 <<  5)			// bit 5 – Receive FIFO data request (RXDR)
#define RINTSTS_RCRC			(1 <<  6)			// bit 6 – Response CRC error (RCRC)
															//		Response CRC does not match with locally-generated CRC in CIU.
#define RINTSTS_DCRC			(1 <<  7)			// bit 7 – Data CRC error (DCRC)
															// 	1) Data errors – Set when error in data reception are observed; 
															//			for example, data CRC, start bit not found, end bit not found, and so on. 
															//			These errors could be set for any block-first block, intermediate
															//			block, or last block. On receipt of an error, the software can issue a STOP or ABORT command and
															//			retry the command for either whole data or partial data.		
															//		2) If there is a CRC16 mismatch, the data path signals a data CRC error to the BIU
															//		3) multiple-block write-data transfer
#define RINTSTS_RTO			(1 <<  8)			// bit 8 – Response timeout (RTO)/Boot Ack Received (BAR)
															//		For response timeout, software can retry the command
															//		Response timeout occurred. Command Done (CD) also set if response
															//		timeout occurs. If command involves data transfer and when response
															//		times out, no data transfer is attempted by DWC_mobile_storage.	
#define RINTSTS_DRTO			(1 <<  9)			// bit 9 – Data read timeout (DRTO)/Boot Data Start (BDS)
															//		For data timeout, the DWC_mobile_storage has not received the data start bit – either for the first block or the
															//		intermediate block – within the timeout period, so software can either retry the whole data transfer
															//		again or retry from a specified block onwards. By reading the contents of the TCBCNT later, the
															//		software can decide how many bytes remain to be copied.
#define RINTSTS_HTO			(1 << 10)			// bit 10 – Data starvation-by-host timeout (HTO) /Volt_switch_int
#define RINTSTS_FRUN			(1 << 11)			// bit 11 – FIFO underrun/overrun error (FRUN)
#define RINTSTS_HLE			(1 << 12)			// bit 12 – Hardware locked write error (HLE)
															//		Hardware locked error – Set when the DWC_mobile_storage cannot load a command issued by
															//		software. When software sets the start_cmd bit in the CMD register, the DWC_mobile_storage tries to
															//		load the command. If the command buffer is already filled with a command, this error is raised. The
															//		software then has to reload the command
#define RINTSTS_SBE			(1 << 13)			// bit 13 – Start-bit error (SBE) /Busy Clear Interrupt (BCI)
#define RINTSTS_ACD			(1 << 14)			// bit 14 – Auto command done (ACD)
#define RINTSTS_EBE			(1 << 15)			// bit 15 – End-bit error (read)/write no CRC (EBE)
															//		1) multiple-block write-data transfer
															//		write no CRC (EBE)
															//			The card sends back the CRC check result as a CRC status token on the DAT0 line.
															//			Note that the CRC response output is always two clocks after the end of data.
															//			if the CRC status start bit is not received by two clocks after the end of a data block, 
															//			a CRC status start bit error is signaled to the BIU by setting the write-no-CRC bit 
															//			in the RINTSTS register; further data transfer is terminated.
															//		2) Block Data Read
															//			If the received end bit is not 1, the BIU receives an end-bit error.	

#define SDMMC_CTRL							0x00
#define SDMMC_PWREN						0x04
#define SDMMC_CLKDIV						0x08
#define SDMMC_CLKSRC						0x0C
#define SDMMC_CLKENA						0x10
#define SDMMC_TMOUT						0x14
#define SDMMC_CTYPE						0x18
#define SDMMC_BLKSIZ						0x1C
#define SDMMC_BYTCNT						0x20
#define SDMMC_INTMASK						0x24
#define SDMMC_CMDARG						0x28
#define SDMMC_CMD							0x2C
#define SDMMC_RESP0						0x30
#define SDMMC_RESP1						0x34
#define SDMMC_RESP2						0x38
#define SDMMC_RESP3						0x3C
#define SDMMC_MINTSTS						0x40
#define SDMMC_RINTSTS						0x44
#define SDMMC_STATUS						0x48
#define SDMMC_FIFOTH						0x4C
#define SDMMC_CDETECT						0x50
#define SDMMC_WRTPRT						0x54
#define SDMMC_GPIO							0x58
#define SDMMC_TCBCNT						0x5C
#define SDMMC_TBBCNT						0x60
#define SDMMC_DEBNCE						0x64
#define SDMMC_USRID						0x68
#define SDMMC_VERID						0x6C
#define SDMMC_HCON							0x70
#define SDMMC_DATA							0x100
#define SDMMC_FIFO							0x100
//#define SDMMC_FIFO							(SDHC_Controls[lg_ulChip] + 0x100)


#define fifo_tran_over(sdmmc_info) do{}while (!(SDHC_REG_READ32(sdmmc_info,SDMMC_STATUS) & 0x00000004))
#define confi_fifo_full(sdmmc_info) do{}while (!(SDHC_REG_READ32(sdmmc_info,SDMMC_STATUS) & 0x00000008))
//#define confi_fifo_unempty() do{}while ((rSTATUS & 0x00000004))

#define PWREN_ON						0x00000000     //turn on all card power
#define PWREN_OFF						0xFFFFFFFF     //turn off all card power
#define CLK_CLKEN						0x0000ffff     //
#define CLK_CLKDIS						0x00000000     // disable clk
#define CLK_DIV_INITIAL					0x00000010     //initial choose frequency
#define CLK_DIV_NORMAL					0x00000000     //data transfers clk frequency
#define CLK_SRC							0x00000000     //             
#define CMD_CHANG_CLK					0x80202000     //chang clk
#define CMD_INITIAL_CLK					0x80008000     //initial clk before work
#define CMD0_GO_IDLE					0x80000000     //go idle
#define CMD1_MATCH_VCC					0x80000041     //turn on for match vcc
#define CMD2_CID						0x800001c2     //initial card get CID
#define CMD3_RCA						0x80000143     //comfire RCA to card
#define CMD6_SWTICH					0x80000346     //switch function
#define CMD7_SELECT_CARD				0x80000147     //select card
#define CMD8_SPEC						0x80000048     //confire specfic
#define CMD9_CSD						0x800001c9     //get CSD
#define CMD10_CID						0x800001ca     //get CID at transfers
#define CMD12_STOP_STEARM				0x8000004c     //stop block transfers 
#define CMD13_STATUS_CARD				0x8000014D     //get card status
#define CMD15_INACTIVE					0x8000000F     //make card to inactive
#define CMD16_SET_BLOCKLEN				0x80000150     //set card block length
#define CMD17_READ_SINGLE				0x80000351     //single block read
#define CMD18_READ_MUL					0x80001352     //multipe block read
#define CMD23_PRE_ERASE				0x80000157     //pre erase for write
#define CMD24_WRITE_SINGLE			0x80000758     //single block write
#define CMD25_WRITE_MUL				0x80000759     //multipe block write
#define CMD27_PROG_CSD					0x8000065b     //programme csd
#define CMD28_SET_PROTECT				0x8000015c     //set protect
#define CMD29_CLR_PROTECT				0x8000015d     //clearn protect
#define CMD30_SEND_WRITE				0x8000025e     //get the status about protect
#define CMD32_ERASESD_START			0x80000160     //set start erase SD card address
#define CMD33_ERASESD_END				0x80000161     //set end erase SD card address
#define CMD38_ERASE					0x80000166     //confirm erase
#define CMD41_MATCH_VCC				0x80000069     //FOR SD, R3(CR register)'s crc is fixed "1111111" 
#define CMD55_APP						0x80000177     //APP CMD
#define ACMD6_WID						0x80000146     //set bus width
#define ACMD13_GET_STATUS				0x8000024d     //get status 512 bit
#define ACMD42_DISCON_DATA3			0x8000016a     //make data3 disconnect
#define ACMD51_GET_CSR					0x80000273     //get card csr

//=======================MMC set bus==========================
#define CMD6_SWITH						0x80000446     //set bus width
//================================================

/*sd dma tx*/

// AHB master 1的优先级高于H264 codec及ISP, 
//		对于H264, 存在较大风险阻塞H264 codec取数, 导致H264编码时间变长, 效率变差.
// 	对于ISP, AHB master 1可能阻塞ISP 3D的取数, 导致bus bandwidth abnormal
// 使用 AHB master 2, AHB master 2的优先级低于H264 Codec及ISP, 这样可以避免H264编码效率变差
#define SD_TX_DMA_CTL_L 				((0<<28)\
										|(0<<27)\
	/* 使用 AHB master 2 */		|(1<<25)\
	/* 使用 AHB master 2 */		|(1<<23)\
										|(M2P_DMAC<<20)\
										|(0<<18)\
										|(0<<17)\
										|(DMA_BURST_SIZE_16<<14)\
										|(DMA_BURST_SIZE_16<<11)\
										|(0<<9)\
										|(2<<7)\
										|(DMA_WIDTH_32<<4)\
										|(DMA_WIDTH_32<<1)\
										|(1<<0))

#define SD_TX_DMA_CTL_H 				(0<<12)


#define SD_TX_DMA_CFG_L					((0<<31)\
										|(0<<30)\
										|(0<<17)\
										|(0<<8))

#define SD_TX_DMA_CFG_H				((0<<7)\
										|(1<<6)\
										|(1<<5)\
										|(1<<1))

#define SD_TX_DMA_CFG_H_LLI				((0<<7)\
										|(0<<6)\
										|(0<<5)\
										|(1<<1))

/*sd dma rx*/

#define SD_RX_DMA_CTL_L 				((0<<28)\
										|(0<<27)\
	/* 使用 AHB master 2 */		|(1<<25)\
	/* 使用 AHB master 2 */		|(1<<23)\
										|(P2M_DMAC<<20)\
										|(0<<18)\
										|(0<<17)\
										|(DMA_BURST_SIZE_16<<14)\
										|(DMA_BURST_SIZE_16<<11)\
										|(3<<9)\
										|(0<<7)\
										|(DMA_WIDTH_32<<4)\
										|(DMA_WIDTH_32<<1)\
										|(1<<0))
								
#define SD_RX_DMA_CTL_L_LLI 				((0<<28)\
										|(0<<27)\
	/* 使用 AHB master 2 */		|(1<<25)\
	/* 使用 AHB master 2 */		|(1<<23)\
										|(P2M_DMAC<<20)\
										|(0<<18)\
										|(0<<17)\
										|(DMA_BURST_SIZE_4<<14)\
										|(DMA_BURST_SIZE_4<<11)\
										|(3<<9)\
										|(0<<7)\
										|(DMA_WIDTH_32<<4)\
										|(DMA_WIDTH_32<<1)\
										|(1<<0))


#define SD_RX_DMA_CTL_H 				(0<<12)


#define SD_RX_DMA_CFG_L				((0<<31)\
										|(0<<30)\
										|(0<<17)\
										|(0<<8))


#define SD_RX_DMA_CFG_H				((0<<12)\
										|(1<<6)\
										|(1<<5)\
										|(1<<2)\
										|(1<<1))


#define SD_RX_DMA_CFG_H_LLI				((0<<12)\
										|(0<<6) /*SS_UPD_EN */ \
										|(0<<5) /*DS_UPD_EN */\
										|(1<<2)\
										|(1<<1))



INT32 InitSDMMCCard(UINT32 ulCardID);

INT32 ResetSDMMCCard (UINT32 ulCardID);

INT32 StopSDMMCCard (UINT32 ulCardID);

INT32 SetSDMMCCardDoReInit (UINT32 ulCardID, int set);


INT32 SD_ReadSector(SDMMC_INFO * sdmmc_info,UINT32 Unit,ULONG Sector,UINT8 *pBuffer, ULONG NumSectors);

INT32 SD_WriteSector(SDMMC_INFO * sdmmc_info,UINT32 Unit,ULONG  Sector,UINT8 *pBuffer, ULONG  NumSectors, UINT8 RepeatSame);

INT32 SD_WriteMultiBuffer(SDMMC_INFO * sdmmc_info, ULONG  Sector, ULONG  NumSectors, ULONG NumBuffers, const UINT8 **pSectorBuffer, ULONG *pSectorCount);

INT32 SD_ReadMultiBuffer (SDMMC_INFO * sdmmc_info, ULONG  Sector, ULONG  NumSectors, ULONG NumBuffers, const UINT8 **pSectorBuffer, ULONG *pSectorCount);


void	SDCard_Module_Init(void);

INT32 IsCardExist(UINT32 ulCardID);

void SDCard_Module_CardCheck (void);

#ifdef __cplusplus
}
#endif

#endif

