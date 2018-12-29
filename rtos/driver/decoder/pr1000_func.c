#ifdef __LINUX_SYSTEM__
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/freezer.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/list.h>
#include <asm/delay.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>

#include "pr1000.h"
#include "pr1000_user_config.h"
#include "pr1000_table.h"
#include "pr1000_ptz_table.h"
#include "pr1000_ptz_drvtable.h"
#include "pr1000_ptz_table_sd.h"
#ifndef DONT_SUPPORT_STD_PVI
#include "pr1000_ptz_table_pvi.h"
#endif // DONT_SUPPORT_STD_PVI
#include "pr1000_ptz_table_hda.h"
#include "pr1000_ptz_table_cvi.h"
#include "pr1000_ptz_table_hdt.h"
#include "pr1000_func.h"
#include "pr1000_ceqfunc.h"
#include "drv_cq.h"

#else //#ifdef __LINUX_SYSTEM__

#include "pr1000.h"

#include "pr1000_user_config.h"
#include "pr1000_table.h"
#include "pr1000_ptz_table.h"
#include "pr1000_ptz_drvtable.h"
#include "pr1000_ptz_table_sd.h"
#ifndef DONT_SUPPORT_STD_PVI
#include "pr1000_ptz_table_pvi.h"
#endif // DONT_SUPPORT_STD_PVI
#include "pr1000_ptz_table_hda.h"
#include "pr1000_ptz_table_cvi.h"
#include "pr1000_ptz_table_hdt.h"
#include "pr1000_func.h"
#include "pr1000_ceqfunc.h"
#include "drv_cq.h"

extern _drvHost *gpDrvHost;

#endif // __LINUX_SYSTEM__

#undef DEBUG_VERIFY_I2C_WRTABLE_PR1000
#undef DEBUG_PTZ_PR1000

#define MDelay 	OS_Delay

#define PR1000_INT_WAKE_PERIOD		(4000) //msec.
#define MAX_PR1000_PTZ_SIZE             (128)

///////////////////////////////////////////////////////////////////////////
#if 0
static uint8_t Swap8(uint8_t val)
{
#define ZZZZ(x,s,m) (((x) >>(s)) & (m)) | (((x) & (m))<<(s));
	val = ZZZZ(val,4,   0x0F );
	val = ZZZZ(val,2,   0x33 );
	val = ZZZZ(val,1,   0x55 );

	return val;
#undef ZZZZ
}

static uint64_t Swap64(uint64_t val)
{
#define ZZZZ(x,s,m) (((x) >>(s)) & (m)) | (((x) & (m))<<(s));
	val = ZZZZ(val,32,  0x00000000FFFFFFFFull );
	val = ZZZZ(val,16,  0x0000FFFF0000FFFFull );
	val = ZZZZ(val,8,   0x00FF00FF00FF00FFull );
	val = ZZZZ(val,4,   0x0F0F0F0F0F0F0F0Full );
	val = ZZZZ(val,2,   0x3333333333333333ull );
	val = ZZZZ(val,1,   0x5555555555555555ull );

	return val;
#undef ZZZZ
}
#endif
#ifdef __LINUX_SYSTEM__
inline static _stPortChSel *GetMapChInfo(const int prChip, const int prChn, const _drvHost *pHost)
{
	int i;
	for(i = 0; i < MAX_CHN; i++)
	{
		Dbg("i:%d prChip:%d-%d, prChn:%d-%d\n", i COMMA prChip COMMA pHost->sysHost.portChSel[i].prChip COMMA prChn COMMA pHost->sysHost.portChSel[i].prChn);
		if( (pHost->sysHost.portChSel[i].prChip == prChip) && (pHost->sysHost.portChSel[i].prChn == prChn) && ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr))
		{
			return((_stPortChSel *)&pHost->sysHost.portChSel[i]);
		}
		
	}
	return(NULL);
}
#else //#ifdef __LINUX_SYSTEM__
static _stPortChSel *GetMapChInfo(const int prChip, const int prChn, const _drvHost *pHost)
{
	int i;
	for(i = 0; i < MAX_CHN; i++)
	{
		Dbg("i:%d prChip:%d-%d, prChn:%d-%d\n", i COMMA prChip COMMA pHost->sysHost.portChSel[i].prChip COMMA prChn COMMA pHost->sysHost.portChSel[i].prChn);
		if( (pHost->sysHost.portChSel[i].prChip == prChip) && (pHost->sysHost.portChSel[i].prChn == prChn) && ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr))
		{
			return((_stPortChSel *)&pHost->sysHost.portChSel[i]);
		}
		
	}
	return(NULL);
}
#endif // __LINUX_SYSTEM__

///////////////////////////////////////////////////////////////////////////

void PR1000_ChipInfo(const int fd, _drvHost *pHost)
{
	uint16_t chipID = 0;
	uint8_t revID = 0;
	uint8_t i2cData = 0;
	uint8_t i2cSlaveAddr; 
	uint8_t chip; 

	DbgString("> PixelPlus PR1000\n");

	for(chip = PR1000_MASTER; chip < PR1000_CHIP_COUNT; chip++)
	{
		i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

		if(PR1000_PageRead(fd, i2cSlaveAddr, 0, 0xFC, &i2cData) < 0)
		{
			ErrorString("Read chipID.\n");
			return;
		}
		chipID = i2cData << 8;
		if(PR1000_PageRead(fd, i2cSlaveAddr, 0, 0xFD, &i2cData) < 0)
		{
			ErrorString("Read chipID.\n");
			return;
		}
		chipID |= i2cData;

		if(PR1000_PageRead(fd, i2cSlaveAddr, 0, 0xFE, &i2cData) < 0)
		{
			ErrorString("Read chipID.\n");
			return;
		}
		revID = i2cData;

		Print("\tPR1000 Chip%d [Slv:0x%02x, ChipId:0x%04X, RevID:0x%02X]\n", chip COMMA i2cSlaveAddr>>1 COMMA chipID COMMA revID);

 		pHost->sysHost.chipID_verify |= ((chipID == PR1000_CHIPID)?1:0)<<chip;
 		pHost->sysHost.revID[chip] = revID;
	}

	return;
}

int PR1000_TestInfeface(const int fd, _drvHost *pHost)
{
	uint8_t i2cSlaveAddr;
	uint8_t chip; 
        int page;
        int testLoop = 0;

	typedef union 
	{
		uint8_t reg;
		struct
		{    
			uint8_t b7:1;
			uint8_t b6:1;
			uint8_t b5:1;
			uint8_t b4:1;
			uint8_t b3:1;
			uint8_t b2:1;
			uint8_t b1:1;
			uint8_t b0:1;
		}b;                  
	}_Reg;

	DbgString("> PixelPlus PR1000\n");

	for(chip = PR1000_MASTER; chip < PR1000_CHIP_COUNT; chip++)
	{
		pHost->sysHost.i2cRW_verify &= (uint8_t)~(1<<chip);
		i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

#ifdef DEBUG_VERIFY_I2C_WRTABLE_PR1000
		testLoop = 100;
#else
		testLoop = 1;
#endif
		while(testLoop--)
		{
			uint8_t addr = 0x90;
			uint8_t wr = 0xff;
			uint8_t rd = 0xff;
			uint8_t i;

			Dbg("PR1000 i2c single rw test(addr:0x%x).\n", addr);

        		page = PR1000_REG_PAGE_COMMON;
			for(i = 0; i < 0x10; i++)
			{
				wr = 0x80 + i;
				PR1000_PageWrite(fd, i2cSlaveAddr, page, addr, wr);

				PR1000_PageRead(fd, i2cSlaveAddr, page, addr, &rd);

				if(wr != rd)
				{
					Error("Error PR1000 i2c single rw test(addr:0x%x (w:0x%x-r:0x%x).\n", addr COMMA wr COMMA rd);
					return(-1);
				}
			}
			if(i >= 0x10) Print("PR1000 chip%d i2c single rw test OK!.(loop:%d)\n", chip COMMA testLoop);
		}

		/* test union bit test */
		{
			uint8_t addr = 0x90;
			uint8_t wr = 0xAC;
			_Reg rd;

			DbgString("PR1000 Union bit test\n");

        		page = PR1000_REG_PAGE_COMMON;
			PR1000_PageWrite(fd, i2cSlaveAddr, page, addr, wr);

			PR1000_PageRead(fd, i2cSlaveAddr, page, addr, &rd.reg);

			Dbg("UnionBit Wr:0x%02x, Rd:0x%02x, B7:%d, B0:%d\n", wr COMMA rd.reg COMMA rd.b.b7 COMMA rd.b.b0);
			if((rd.b.b7 == 0) && (rd.b.b0 == 1)) 
			{
				PrintString("UnionBit Big endian.(Hixx, Stm32)\n");
			}
			else 
			{
				PrintString("UnionBit Little endian\n");
			}
		}

#ifdef DEBUG_VERIFY_I2C_WRTABLE_PR1000
		testLoop = 100;
#else
		testLoop = 1;
#endif
		while(testLoop--)
		{
			uint8_t addr = 0xA0;
			uint8_t wr[12] = {0x50, 0xf0, 0xa0, 0xf0, 0x51, 0xf1, 0xa1, 0xf1, 0x52, 0xf2, 0xa2, 0xf2};
			uint8_t rd[12] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
			uint8_t i, j, length = 12;
			uint8_t swr = 0xA5, srd = 0xA5;


			Dbg("PR1000 i2c burst rw test(addr:0x%x).\n", addr);

        		page = PR1000_REG_PAGE_COMMON;
			for(i = 0; i < 0x10; i++)
			{
				PR1000_PageWrite(fd, i2cSlaveAddr, page, addr+length, swr);

				PR1000_PageWriteBurst(fd, i2cSlaveAddr, page, addr, length, wr);
				PR1000_PageReadBurst(fd, i2cSlaveAddr, page, addr, length, rd);

				PR1000_PageRead(fd, i2cSlaveAddr, page, addr+length, &srd);

				if(swr != srd)
				{
					Error("Error PR1000 i2c burst rw area test(addr:0x%x (w:0x%x-r:0x%x).\n", addr+length COMMA swr COMMA srd);
					return(-1);
				}

				for(j = 0; j < length; j++)
				{
					if(wr[j] != rd[j])
					{
						Error("Error PR1000 i2c burst rw test(addr:0x%x (w:0x%x-r:0x%x).\n", addr COMMA wr[j] COMMA rd[j]);
						return(-1);
					}
					rd[j] = 0xff; //init
				}
				if(rd[j-1] != 0xff) break;
			}
			if(i >= 0x10) Print("PR1000 chip%d i2c burst rw test OK!.(loop:%d)\n", chip COMMA testLoop);
		}

#ifdef DEBUG_VERIFY_I2C_WRTABLE_PR1000
		testLoop = 100;
#else
		testLoop = 1;
#endif
		while(testLoop--)
		{
			uint8_t addr = 0x11;
			uint8_t wr[12] = {0x50, 0xf0, 0xa0, 0xf0, 0x51, 0xf1, 0xa1, 0xf1, 0x52, 0xf2, 0xa2, 0xf2};
			uint8_t rd[12] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
			uint8_t i, j, length = 12;
			uint8_t srd;


			Dbg("PR1000 i2c burst fifo rw test(addr:0x%x).\n", addr);

        		page = PR1000_REG_PAGE_PTZ;
			PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x00, 0x80); //rxpath en
			PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x20, 0x80); //txpath en
			PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x0D, 0x40); //addr hold en
			PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x10, 0x80); //init wr fifo
			PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x14, 0x80); //init rd fifo
			for(i = 0; i < 0x10; i++)
			{
				addr = 0x11;
				PR1000_PageWriteBurst(fd, i2cSlaveAddr, page, addr, length, wr);
				addr = 0x12;
				PR1000_PageRead(fd, i2cSlaveAddr, page, addr, &srd);
				Dbg("Wr Queue Size:%d\n", srd);
				if(srd % length)
				{
					Error("Error PR1000 i2c burst fifo rw test(Wr Queue size:%d).\n", srd);

					PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x00, 0x00); //rxpath en
					PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x20, 0x00); //txpath en
					PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x0D, 0x00); //addr hold en
					PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x10, 0x00); //init wr fifo
					PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x14, 0x00); //init rd fifo
					return(-1);
				}

				addr = 0x14;
				PR1000_PageWrite(fd, i2cSlaveAddr, page, addr, 0x10);
				addr = 0x16;
				PR1000_PageReadBurst(fd, i2cSlaveAddr, page, addr, length, rd);

				for(j = 0; j < length; j++)
				{
					if(wr[j] != rd[j])
					{
						Error("Error PR1000 i2c burst fifo rw test(addr:0x%x (w:0x%x-r:0x%x).\n", addr COMMA wr[j] COMMA rd[j]);

						PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x00, 0x00); //rxpath en
						PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x20, 0x00); //txpath en
						PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x0D, 0x00); //addr hold en
						PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x10, 0x00); //init wr fifo
						PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x14, 0x00); //init rd fifo
						return(-1);
					}
					rd[j] = 0xff; //init
				}
				if(rd[j-1] != 0xff) break;
			}
			if(i >= 0x10) Print("PR1000 chip%d i2c burst fifo rw test OK!.(loop:%d)\n", chip COMMA testLoop);

			PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x00, 0x00); //rxpath en
			PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x20, 0x00); //txpath en
			PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x0D, 0x00); //addr hold en
			PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x10, 0x00); //init wr fifo
			PR1000_PageWrite(fd, i2cSlaveAddr, page, 0x14, 0x00); //init rd fifo
		}
	}

	for(chip = PR1000_MASTER; chip < PR1000_CHIP_COUNT; chip++)
	{
		i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

		{
			uint8_t addr = 0x90;
			uint8_t wr = i2cSlaveAddr;

        		page = PR1000_REG_PAGE_COMMON;
			PR1000_PageWrite(fd, i2cSlaveAddr, page, addr, wr);
			Dbg("Wr chip:%d slave addr:%x data:%x\n", chip COMMA i2cSlaveAddr COMMA wr);
		}
	}
	for(chip = PR1000_MASTER; chip < PR1000_CHIP_COUNT; chip++)
	{
		i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

		{
			uint8_t addr = 0x90;
			uint8_t rd = 0xff;

        		page = PR1000_REG_PAGE_COMMON;
			PR1000_PageRead(fd, i2cSlaveAddr, page, addr, &rd);

			Dbg("Rd chip:%d slave addr:%x data:%x\n", chip COMMA i2cSlaveAddr COMMA rd);
			if(i2cSlaveAddr != rd)
			{
				Error("Error PR1000 i2c slave addr verify test(addr:0x%x (w:0x%x-r:0x%x).\n", addr COMMA i2cSlaveAddr COMMA rd);
				return(-1);
			}
			pHost->sysHost.i2cRW_verify |= (uint8_t)(1<<chip);
		}
	}


	return(0);
}

void InitPR1000(const int fd)
{
        uint8_t i2cReg = 0;
        uint16_t i2cData = 0;
        int page;
	uint8_t i2cSlaveAddr;
	uint8_t chip;
	_PR1000_REG_TABLE_COMMON *pTableCommon = NULL;

	for(chip = PR1000_MASTER; chip < PR1000_CHIP_COUNT; chip++)
	{
		Print("> PixelPlus PR1000 Init [chip:%d]\n", chip);
		#ifdef DEBUG_VERIFY_I2C_WRTABLE_PR1000
		Print("> Load common register and verify i2c write value [chip:%d]\n", chip);
		#endif //DEBUG_VERIFY_I2C_WRTABLE_PR1000

		i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

		pTableCommon = (_PR1000_REG_TABLE_COMMON *)pr1000_reg_table_common;

		while( (pTableCommon->addr != 0xFF) || (pTableCommon->pData != 0xFFFF) )
		{
			i2cReg = pTableCommon->addr;
			i2cData = pTableCommon->pData;
			page = (i2cData>>8);
			pTableCommon++;

			if(i2cData == 0xFF00) continue; //skip
			else if(i2cData == 0xFFFF) break; //end
			else
			{
				Dbg("Page:%d, Addr:%02x, Data:%04x\n", page COMMA i2cReg COMMA i2cData);
				if( PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, (uint8_t)i2cData) < 0)
				{
					ErrorString("Write reg.\n");
					return;
				}

				#ifdef DEBUG_VERIFY_I2C_WRTABLE_PR1000
				{
					uint8_t rData, wData;

					wData = (uint8_t)i2cData;
					if( PR1000_PageRead(fd, i2cSlaveAddr, page, (uint8_t)i2cReg, &rData) < 0)
					{
						ErrorString("Read reg.\n");
						return;
					}
					if(wData != rData) 
					{
						Error("Error [p%d, addr:0x%2x, w:0x%02x, r:0x%02x]\n", page COMMA i2cReg COMMA wData COMMA rData);
					}
				}
				#endif // DEBUG_VERIFY_I2C_WRTABLE_PR1000
			}
		}
	}
	return;
}

//only for make register table 
#if 0
#define PRINT_I2C_DATA //also define gpio_i2c.c 
#ifdef PRINT_I2C_DATA
extern int gPrintI2C; //if gPrintI2C=1, print i2c write register.


/* Mpp1-Hsync, Mpp2-Vsync */
/*
{0xff, 0x00}, 
{0xd0, 0xcc},
{0xd8, 0x00},
{0xf2, 0x0c},
{0xf5, 0x1c},
*/
int PrintI2C_PR1000_Init(const int fd, _drvHost *pHost, enum _pr1000_table_format format, enum _pr1000_table_outresol resol)
{/*{{{*/
	int mapChn;
	uint8_t chip;
#ifndef DONT_SUPPORT_AUD_ALINK
	_stAUDCascadeAttr stAUDCascadeAttr;
#endif // DONT_SUPPORT_AUD_ALINK
	int (*pPortChSel)[4][2];
	_stPortChSel *pStPortChSel = NULL;
	int vd = 0, outseq = 0, vinx, prChn;


	/* load board specific configuration */
	PrintString("#############################################################\n");
	Print(">>>>>> Set mode [%s, %s] <<<<<<<\n", _STR_PR1000_FORMAT[format] COMMA _STR_PR1000_OUTRESOL[resol]);
	PrintString("#############################################################\n");
	for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
	{
		pHost->sysHost.gPR1000RxType[mapChn] = format;
		pHost->sysHost.gPR1000RxResol[mapChn] = resol;
	}
	{/*{{{*/
		pHost->sysHost.stOutFormatAttr.chipCnt = PR1000_CHIP_COUNT; //get from pr1000_user_config.c
		pHost->sysHost.stOutFormatAttr.muxChCnt = (enum _pr1000_vid_outformat_mux_type)PR1000_VIDOUTF_MUX_CH; //get from pr1000_user_config.c
		pHost->sysHost.stOutFormatAttr.b16bit = PR1000_VIDOUTF_16BIT; //get from pr1000_user_config.c
		pHost->sysHost.stOutFormatAttr.datarate = PR1000_VIDOUTF_RATE; //0:default
		pHost->sysHost.stOutFormatAttr.resol = (enum _pr1000_vid_outformat_resol_type)PR1000_VIDOUTF_RESOL; //get from pr1000_user_config.c
		pHost->sysHost.stOutFormatAttr.outfmt_bt656 = PR1000_VIDOUTF_BT656; //get from pr1000_user_config.c

		for(chip = PR1000_MASTER; chip < PR1000_CHIP_COUNT; chip++)
		{
			if(pHost->sysHost.stOutFormatAttr.muxChCnt == PR1000_VID_OUTF_MUX_1CH)
			{
				pPortChSel = (int (*)[4][2])&PR1000_VID_INOUT_MUX1CH_CHDEF[pHost->sysHost.stOutFormatAttr.b16bit][chip];
				Dbg("chip:%d, PR1000_VID_OUTF_MUX_1CH\n", chip);
				Dbg("PR1000_VID_OUTF_MUX_1CH chn0 vinx:%d, mapChn:%d\n",
					pPortChSel[0][0][0] COMMA //VINx
					pPortChSel[0][0][1]); //chn.(mapping channel)
			}
#ifdef __LINUX_SYSTEM__
			else if(pHost->sysHost.stOutFormatAttr.muxChCnt == PR1000_VID_OUTF_MUX_2CH)
			{
				pPortChSel = (int (*)[4][2])&PR1000_VID_INOUT_MUX2CH_CHDEF[pHost->sysHost.stOutFormatAttr.b16bit][chip];
				Dbg("chip:%d, PR1000_VID_OUTF_MUX_2CH\n", chip);
				Dbg("PR1000_VID_OUTF_MUX_2CH chn0 vinx:%d, mapChn:%d\n",
					pPortChSel[0][0][0] COMMA //VINx
					pPortChSel[0][0][1]); //chn.(mapping channel)
			}
			else if(pHost->sysHost.stOutFormatAttr.muxChCnt == PR1000_VID_OUTF_MUX_4CH)
			{
				pPortChSel = (int (*)[4][2])&PR1000_VID_INOUT_MUX4CH_CHDEF[pHost->sysHost.stOutFormatAttr.b16bit][chip];
				Dbg("chip:%d, PR1000_VID_OUTF_MUX_4CH\n", chip);
				Dbg("PR1000_VID_OUTF_MUX_4CH chn0 vinx:%d, mapChn:%d\n",
					pPortChSel[0][0][0] COMMA //VINx
					pPortChSel[0][0][1]); //chn.(mapping channel)
			}
#endif // __LINUX_SYSTEM__
			else
			{
				ErrorString("Invalid muxch config\n");
				break;
			}
			for(vd = 0; vd < 4; vd++) //VD1~VD4
			{
				for(outseq = 0; outseq < 4; outseq++) //ABCD
				{
					vinx = pPortChSel[vd][outseq][0]; //VINx
					mapChn = pPortChSel[vd][outseq][1]; //chn.(mapping channel)
					if(mapChn >= MAX_CHN) 
					{
						Error("Invalid mapping channel(%d)\n", mapChn);
						mapChn = -1;
					}
					if( ((vinx%2) == 0) && (mapChn < 0) )
					{
						mapChn = vinx/2;
					}

					if( (vinx >= 0) && (mapChn >= 0) )
					{
						pStPortChSel = (_stPortChSel *)&pHost->sysHost.portChSel[mapChn];

						pStPortChSel->chn = mapChn;
						pStPortChSel->vdPort = vd & 0x3;
						pStPortChSel->prChip = chip & 0x3;
						pStPortChSel->prChn = (vinx/2) & 0x3;
						pStPortChSel->i2cSlvAddr = PR1000_I2C_SLVADDRS[chip];
						Print("mapChn:%d, vd:%d, prChip:%d, prChn:%d, vinx:%d, i2c:0x%2x\n",
								pStPortChSel->chn COMMA
								pStPortChSel->vdPort COMMA
								pStPortChSel->prChip COMMA
								pStPortChSel->prChn COMMA
								vinx COMMA
								pStPortChSel->i2cSlvAddr);
					}
				}
			}
		}

		memcpy(pHost->sysHost.stOutFormatAttr.portChClkPhase, &PR1000_VIDOUTF_CLKPHASE,
			sizeof(pHost->sysHost.stOutFormatAttr.portChClkPhase)); //get from pr1000_user_config.c

		Print("VID Format [muxChCnt:%d, b16bit:%d, resol:%d, datarate:%d]\n",
				pHost->sysHost.stOutFormatAttr.muxChCnt COMMA
				pHost->sysHost.stOutFormatAttr.b16bit COMMA
				pHost->sysHost.stOutFormatAttr.resol COMMA
				pHost->sysHost.stOutFormatAttr.datarate);

		for(chip = PR1000_MASTER; chip < PR1000_CHIP_COUNT; chip++)
		{
			Print("VID Format [chip:%d, chkphase(%x/%x/%x/%x)]\n", chip COMMA
					pHost->sysHost.stOutFormatAttr.portChClkPhase[chip][0] COMMA
					pHost->sysHost.stOutFormatAttr.portChClkPhase[chip][1] COMMA
					pHost->sysHost.stOutFormatAttr.portChClkPhase[chip][2] COMMA
					pHost->sysHost.stOutFormatAttr.portChClkPhase[chip][3]);
		}
	}/*}}}*/
	PrintString("#############################################################\n");
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	InitPR1000(fd);

	/* Load vid register table */
	{
		for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
		{
			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) continue;

			PR1000_VID_LoadTable(fd, &pHost->sysHost.portChSel[mapChn], TRUE, pHost->sysHost.gPR1000RxType[mapChn], pHost->sysHost.gPR1000RxResol[mapChn]);
		}
	}

	/* VADC port reset(power down->power on) */
	{
		uint8_t page;
		uint8_t i2cSlaveAddr = 0;
		uint8_t i2cReg = 0;
		uint8_t i2cData = 0;
		uint8_t i2cMask = 0;

		for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
		{
			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) continue;

			prChn = pHost->sysHost.portChSel[mapChn].prChn;
			i2cSlaveAddr = pHost->sysHost.portChSel[mapChn].i2cSlvAddr;

			page = PR1000_VDEC_PAGE(prChn);
			i2cReg = 0x68 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
			i2cMask = 0x10; i2cData = 0x10;
			Dbg("Write [p%d, 0x%02x-0x%02x(mask:%02x)]\n", page COMMA i2cReg COMMA i2cData COMMA i2cMask);
			PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData);
		}
#ifdef __LINUX_SYSTEM__
		mdelay(200);
#else //#ifdef __LINUX_SYSTEM__
		MDelay(200);
#endif // __LINUX_SYSTEM__

		for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
		{
			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) continue;

			prChn = pHost->sysHost.portChSel[mapChn].prChn;
			i2cSlaveAddr = pHost->sysHost.portChSel[mapChn].i2cSlvAddr;

			page = PR1000_VDEC_PAGE(prChn);
			i2cReg = 0x68 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
			i2cMask = 0x10; i2cData = 0x00;
			Dbg("Write [p%d, 0x%02x-0x%02x(mask:%02x)]\n", page COMMA i2cReg COMMA i2cData COMMA i2cMask);
			PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData);
		}
#ifdef __LINUX_SYSTEM__
		mdelay(100);
#else //#ifdef __LINUX_SYSTEM__
		MDelay(100);
#endif // __LINUX_SYSTEM__
	}

	/* Load ptz & veven register table. */
	{
#ifndef DONT_SUPPORT_EVENT_FUNC
		_stMaskCellAttr stMaskCellAttr;
		/* VEVENT attr */
		_stMdAttr stMdAttr;
		_stBdAttr stBdAttr;
		_stBdSpSens stBdSpSens;
		_stNdAttr stNdAttr;
		_stDdAttr stDdAttr;
		_stDfdAttr stDfdAttr;
#endif // DONT_SUPPORT_EVENT_FUNC

		for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
		{
			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) continue;

#ifndef DONT_SUPPORT_PTZ_FUNC
			PR1000_PTZ_LoadTable(fd, &pHost->sysHost.portChSel[mapChn], pHost->sysHost.gPR1000RxType[mapChn], pHost->sysHost.gPR1000RxResol[mapChn], pHost);
			PR1000_PTZ_Init(fd, &pHost->sysHost.portChSel[mapChn], pHost); //Do after ptz loadtable.
			PR1000_PTZ_SetPattern(fd, &pHost->sysHost.portChSel[mapChn], pHost->sysHost.gPR1000RxType[mapChn], pHost->sysHost.gPR1000RxResol[mapChn], pHost);
#endif // DONT_SUPPORT_PTZ_FUNC

			PR1000_VEVENT_LoadTable(fd, &pHost->sysHost.portChSel[mapChn], pHost->sysHost.gPR1000RxType[mapChn], pHost->sysHost.gPR1000RxResol[mapChn]);

#ifndef DONT_SUPPORT_EVENT_FUNC
			Dbg("VEVENT ClearMask & Write Maskformat Pz/Md/Bd [mapChn:%d]\n", mapChn);
			PR1000_VEVENT_ClearMask(fd, &pHost->sysHost.portChSel[mapChn]);
			PR1000_VEVENT_SetMaskAttr(fd, &pHost->sysHost.portChSel[mapChn], pHost->sysHost.gPR1000RxResol[mapChn], &pr1000_mask_attr_table_vevent[pHost->sysHost.gPR1000RxResol[mapChn]]);
			PR1000_VEVENT_GetMaskAttr(fd, &pHost->sysHost.portChSel[mapChn], pHost->sysHost.gPR1000RxResol[mapChn], &stMaskCellAttr);
			/* get default attribute */
			PR1000_VEVENT_GetMdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stMdAttr);
			PR1000_VEVENT_GetBdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stBdAttr);
			PR1000_VEVENT_GetNdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stNdAttr);
			PR1000_VEVENT_GetDdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDdAttr);
			PR1000_VEVENT_GetDfdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDfdAttr);

			Print("VEVENT Set attribute of Md/Bd/Nd/Dd/Dfd [mapChn:%d]\n", mapChn);
			PR1000_VEVENT_SetMdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stMdAttr);
			stBdSpSens.spsens = ((stMaskCellAttr.cellCntX * stMaskCellAttr.cellCntY)/100)*50; //Set 50% blind.
			PR1000_VEVENT_SetBdSpSens(fd, &pHost->sysHost.portChSel[mapChn], &stBdSpSens);
			PR1000_VEVENT_SetBdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stBdAttr);
			PR1000_VEVENT_SetNdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stNdAttr);
			PR1000_VEVENT_SetDdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDdAttr);
			PR1000_VEVENT_SetDfdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDfdAttr);

			PR1000_VEVENT_SetDisplayCellFormat(fd, &pHost->sysHost.portChSel[mapChn], &stDefVEVENTDisplayAttr);
#endif // DONT_SUPPORT_EVENT_FUNC
		}
	}

	/* set video output. */
	PR1000_VID_SetOutFormatAttr(fd, &pHost->sysHost.stOutFormatAttr);
	/////////////////////////////////////////////////////////////

#ifndef DONT_SUPPORT_AUD_ALINK
	for(chip = PR1000_MASTER; chip < PR1000_CHIP_COUNT; chip++)
	{
		Dbg("AUD Enable [chip:%d]\n", chip);
		PR1000_AUD_Enable(fd, chip, ENABLE);

		Dbg("AUD Set Rec attribute [chip:%d]\n", chip);
		PR1000_AUD_SetRecAttr(fd, chip, &stDefAUDRecAttr);

		Dbg("AUD Set Pb attribute [chip:%d]\n", chip);
		PR1000_AUD_SetPbAttr(fd, chip, &stDefAUDPbAttr);

		Dbg("AUD Disable Mute Rec/Mix/Voc/Dac [chip:%d]\n", chip);
		for(prChn = 0; prChn < DEF_PR1000_MAX_CHN; prChn++)
		{
			PR1000_AUD_SetRecMute(fd, chip, prChn, DISABLE);
		}
		PR1000_AUD_SetMixMute(fd, chip, DISABLE);
		PR1000_AUD_SetVocMute(fd, chip, DISABLE);
		PR1000_AUD_SetDacMute(fd, chip, DISABLE);

		Dbg("AUD Set dac [chip:%d, ch:%d]\n", chip COMMA 0);
		PR1000_AUD_SetDacChn(fd, chip, 0); //0~15:live, 16:Pb, 17:Voice, 18:Mix

		{
			_stAUDDetAttr stAUDDetAttr;

			Dbg("AUD Get detect attr. [prChn:%d]\n", chip);
			PR1000_AUD_GetDetAttr(fd, chip, &stAUDDetAttr);
			Dbg("AUD Set detect attr. [prChn:%d]\n", chip);
			PR1000_AUD_SetDetAttr(fd, chip, &stAUDDetAttr);
		}

		if(chip == PR1000_MASTER)
		{
			stAUDCascadeAttr.bCascadeMaster = 1;
			for(prChn = 0; prChn < DEF_PR1000_MAX_CHN*PR1000_CHIP_COUNT; prChn++)
			{
				PR1000_AUD_SetRecMute(fd, chip, prChn, DISABLE);
			}
		}
		else
		{
			stAUDCascadeAttr.bCascadeMaster = 0;
		}
		PR1000_AUD_SetCascadeAttr(fd, chip, &stAUDCascadeAttr);
	}
#endif // DONT_SUPPORT_AUD_ALINK

	PR1000_IRQ_Init(fd, pHost); //init all chip.
	
	return 0;
}/*}}}*/
#endif // PRINT_I2C_DATA
#endif

int PR1000_Init(const int fd, _drvHost *pHost)
{
	int mapChn;
	uint8_t chip;
#ifndef DONT_SUPPORT_AUD_ALINK
	_stAUDCascadeAttr stAUDCascadeAttr;
#endif // DONT_SUPPORT_AUD_ALINK
	int (*pPortChSel)[4][2];
	_stPortChSel *pStPortChSel = NULL;
	int vd = 0, outseq = 0, vinx, prChn;

//only for make register table 
#ifdef PRINT_I2C_DATA
	{/*{{{*/
		enum _pr1000_table_format format = DEFAULT_INIT_FORMAT;
		enum _pr1000_table_outresol resol = DEFAULT_INIT_RESOLUTION;

		gPrintI2C = 1;
		PrintString("print register ############################################################\n");

		if( ((DEFAULT_INIT_FORMAT & 0x80) == 0x80) && ((DEFAULT_INIT_RESOLUTION & 0x80) == 0x80) )
		{
			for(format = pr1000_format_SD720; format < max_pr1000_format; format++)
			{
				for(resol = pr1000_outresol_720x480i60; resol <= pr1000_outresol_1920x1080p25; resol++)
				{
					PrintI2C_PR1000_Init(fd, pHost, format, resol);
				}
			}
		}
		else if( ((DEFAULT_INIT_FORMAT & 0x80) == 0x80) && ((DEFAULT_INIT_RESOLUTION & 0x80) != 0x80) )
		{
			resol = DEFAULT_INIT_RESOLUTION & 0x7F;
			for(format = pr1000_format_SD720; format < max_pr1000_format; format++)
			{
				PrintI2C_PR1000_Init(fd, pHost, format, resol);
			}
		}
		else if( ((DEFAULT_INIT_FORMAT & 0x80) != 0x80) && ((DEFAULT_INIT_RESOLUTION & 0x80) == 0x80) )
		{
			format = DEFAULT_INIT_FORMAT & 0x7F;
			for(resol = pr1000_outresol_720x480i60; resol <= pr1000_outresol_1920x1080p25; resol++)
			{
				PrintI2C_PR1000_Init(fd, pHost, format, resol);
			}
		}
		else
		{
			resol = DEFAULT_INIT_RESOLUTION & 0x7F;
			format = DEFAULT_INIT_FORMAT & 0x7F;
			PrintI2C_PR1000_Init(fd, pHost, format, resol);
		}

		gPrintI2C = 0;
		PrintString("############################################################\n");
		return(-1);
	}/*}}}*/
#endif // PRINT_I2C_DATA

	/* load board specific configuration */
	Print("Set default mode [%s, %s]\n", _STR_PR1000_FORMAT[DEFAULT_INIT_FORMAT] COMMA _STR_PR1000_OUTRESOL[DEFAULT_INIT_RESOLUTION]);
	for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
	{
		pHost->sysHost.gPR1000RxType[mapChn] = DEFAULT_INIT_FORMAT;
		pHost->sysHost.gPR1000RxResol[mapChn] = DEFAULT_INIT_RESOLUTION;
	}
	{/*{{{*/
		pHost->sysHost.stOutFormatAttr.chipCnt = PR1000_CHIP_COUNT; //get from pr1000_user_config.c
		pHost->sysHost.stOutFormatAttr.muxChCnt = (enum _pr1000_vid_outformat_mux_type)PR1000_VIDOUTF_MUX_CH; //get from pr1000_user_config.c
		pHost->sysHost.stOutFormatAttr.b16bit = PR1000_VIDOUTF_16BIT; //get from pr1000_user_config.c
		pHost->sysHost.stOutFormatAttr.datarate = PR1000_VIDOUTF_RATE; //0:default
		pHost->sysHost.stOutFormatAttr.resol = (enum _pr1000_vid_outformat_resol_type)PR1000_VIDOUTF_RESOL; //get from pr1000_user_config.c
		pHost->sysHost.stOutFormatAttr.outfmt_bt656 = PR1000_VIDOUTF_BT656; //get from pr1000_user_config.c

		for(chip = PR1000_MASTER; chip < PR1000_CHIP_COUNT; chip++)
		{
			if(pHost->sysHost.stOutFormatAttr.muxChCnt == PR1000_VID_OUTF_MUX_1CH)
			{
				pPortChSel = (int (*)[4][2])&PR1000_VID_INOUT_MUX1CH_CHDEF[pHost->sysHost.stOutFormatAttr.b16bit][chip];
				Dbg("chip:%d, PR1000_VID_OUTF_MUX_1CH\n", chip);
				Dbg("PR1000_VID_OUTF_MUX_1CH chn0 vinx:%d, mapChn:%d\n",
					pPortChSel[0][0][0] COMMA //VINx
					pPortChSel[0][0][1]); //chn.(mapping channel)
			}
#ifdef __LINUX_SYSTEM__
			else if(pHost->sysHost.stOutFormatAttr.muxChCnt == PR1000_VID_OUTF_MUX_2CH)
			{
				pPortChSel = (int (*)[4][2])&PR1000_VID_INOUT_MUX2CH_CHDEF[pHost->sysHost.stOutFormatAttr.b16bit][chip];
				Dbg("chip:%d, PR1000_VID_OUTF_MUX_2CH\n", chip);
				Dbg("PR1000_VID_OUTF_MUX_2CH chn0 vinx:%d, mapChn:%d\n",
					pPortChSel[0][0][0] COMMA //VINx
					pPortChSel[0][0][1]); //chn.(mapping channel)
			}
			else if(pHost->sysHost.stOutFormatAttr.muxChCnt == PR1000_VID_OUTF_MUX_4CH)
			{
				pPortChSel = (int (*)[4][2])&PR1000_VID_INOUT_MUX4CH_CHDEF[pHost->sysHost.stOutFormatAttr.b16bit][chip];
				Dbg("chip:%d, PR1000_VID_OUTF_MUX_4CH\n", chip);
				Dbg("PR1000_VID_OUTF_MUX_4CH chn0 vinx:%d, mapChn:%d\n",
					pPortChSel[0][0][0] COMMA //VINx
					pPortChSel[0][0][1]); //chn.(mapping channel)
			}
#endif // __LINUX_SYSTEM__
			else
			{
				ErrorString("Invalid muxch config\n");
				break;
			}
			for(vd = 0; vd < 4; vd++) //VD1~VD4
			{
				for(outseq = 0; outseq < 4; outseq++) //ABCD
				{
					vinx = pPortChSel[vd][outseq][0]; //VINx
					mapChn = pPortChSel[vd][outseq][1]; //chn.(mapping channel)
					if(mapChn >= MAX_CHN) 
					{
						Error("Invalid mapping channel(%d)\n", mapChn);
						mapChn = -1;
					}
					if( ((vinx%2) == 0) && (mapChn < 0) )
					{
						mapChn = vinx/2;
					}

					if( (vinx >= 0) && (mapChn >= 0) )
					{
						pStPortChSel = (_stPortChSel *)&pHost->sysHost.portChSel[mapChn];

						pStPortChSel->chn = mapChn;
						pStPortChSel->vdPort = vd & 0x3;
						pStPortChSel->prChip = chip & 0x3;
						pStPortChSel->prChn = (vinx/2) & 0x3;
						pStPortChSel->i2cSlvAddr = PR1000_I2C_SLVADDRS[chip];
						Print("mapChn:%d, vd:%d, prChip:%d, prChn:%d, vinx:%d, i2c:0x%2x\n",
								pStPortChSel->chn COMMA
								pStPortChSel->vdPort COMMA
								pStPortChSel->prChip COMMA
								pStPortChSel->prChn COMMA
								vinx COMMA
								pStPortChSel->i2cSlvAddr);
					}
				}
			}
		}

		memcpy(pHost->sysHost.stOutFormatAttr.portChClkPhase, &PR1000_VIDOUTF_CLKPHASE,
			sizeof(pHost->sysHost.stOutFormatAttr.portChClkPhase)); //get from pr1000_user_config.c

		Print("VID Format [muxChCnt:%d, b16bit:%d, resol:%d, datarate:%d]\n",
				pHost->sysHost.stOutFormatAttr.muxChCnt COMMA
				pHost->sysHost.stOutFormatAttr.b16bit COMMA
				pHost->sysHost.stOutFormatAttr.resol COMMA
				pHost->sysHost.stOutFormatAttr.datarate);

		for(chip = PR1000_MASTER; chip < PR1000_CHIP_COUNT; chip++)
		{
			Print("VID Format [chip:%d, chkphase(%x/%x/%x/%x)]\n", chip COMMA
					pHost->sysHost.stOutFormatAttr.portChClkPhase[chip][0] COMMA
					pHost->sysHost.stOutFormatAttr.portChClkPhase[chip][1] COMMA
					pHost->sysHost.stOutFormatAttr.portChClkPhase[chip][2] COMMA
					pHost->sysHost.stOutFormatAttr.portChClkPhase[chip][3]);
		}
	}/*}}}*/
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	InitPR1000(fd);

	/* Load vid register table */
	{
		for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
		{
			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) continue;

			PR1000_VID_LoadTable(fd, &pHost->sysHost.portChSel[mapChn], TRUE, pHost->sysHost.gPR1000RxType[mapChn], pHost->sysHost.gPR1000RxResol[mapChn]);
#ifdef SUPPORT_HIDE_EQING_DISPLAY 
			PrintString("Hide EQing display\n");
			PR1000_VID_HideEQingDisplay(fd, &pHost->sysHost.portChSel[mapChn]);
#endif // SUPPORT_HIDE_EQING_DISPLAY 
		}
	}

	/* VADC port reset(power down->power on) */
	{
		uint8_t page;
		uint8_t i2cSlaveAddr = 0;
		uint8_t i2cReg = 0;
		uint8_t i2cData = 0;
		uint8_t i2cMask = 0;

		for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
		{
			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) continue;

			prChn = pHost->sysHost.portChSel[mapChn].prChn;
			i2cSlaveAddr = pHost->sysHost.portChSel[mapChn].i2cSlvAddr;

			page = PR1000_VDEC_PAGE(prChn);
			i2cReg = 0x68 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
			i2cMask = 0x10; i2cData = 0x10;
			Dbg("Write [p%d, 0x%02x-0x%02x(mask:%02x)]\n", page COMMA i2cReg COMMA i2cData COMMA i2cMask);
			PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData);
		}
#ifdef __LINUX_SYSTEM__
		mdelay(200);
#else //#ifdef __LINUX_SYSTEM__
		MDelay(200);
#endif // __LINUX_SYSTEM__

		for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
		{
			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) continue;

			prChn = pHost->sysHost.portChSel[mapChn].prChn;
			i2cSlaveAddr = pHost->sysHost.portChSel[mapChn].i2cSlvAddr;

			page = PR1000_VDEC_PAGE(prChn);
			i2cReg = 0x68 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
			i2cMask = 0x10; i2cData = 0x00;
			Dbg("Write [p%d, 0x%02x-0x%02x(mask:%02x)]\n", page COMMA i2cReg COMMA i2cData COMMA i2cMask);
			PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData);
		}
#ifdef __LINUX_SYSTEM__
		mdelay(100);
#else //#ifdef __LINUX_SYSTEM__
		MDelay(100);
#endif // __LINUX_SYSTEM__
	}

	/* Load ptz & veven register table. */
	{
#ifndef DONT_SUPPORT_EVENT_FUNC
		_stMaskCellAttr stMaskCellAttr;
		/* VEVENT attr */
		_stMdAttr stMdAttr;
		_stBdAttr stBdAttr;
		_stBdSpSens stBdSpSens;
		_stNdAttr stNdAttr;
		_stDdAttr stDdAttr;
		_stDfdAttr stDfdAttr;
#endif // DONT_SUPPORT_EVENT_FUNC

		for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
		{
			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) continue;

#ifndef DONT_SUPPORT_PTZ_FUNC
			PR1000_PTZ_LoadTable(fd, &pHost->sysHost.portChSel[mapChn], pHost->sysHost.gPR1000RxType[mapChn], pHost->sysHost.gPR1000RxResol[mapChn], pHost);
			PR1000_PTZ_Init(fd, &pHost->sysHost.portChSel[mapChn], pHost); //Do after ptz loadtable.
			PR1000_PTZ_SetPattern(fd, &pHost->sysHost.portChSel[mapChn], pHost->sysHost.gPR1000RxType[mapChn], pHost->sysHost.gPR1000RxResol[mapChn], pHost);
#endif // DONT_SUPPORT_PTZ_FUNC

			PR1000_VEVENT_LoadTable(fd, &pHost->sysHost.portChSel[mapChn], pHost->sysHost.gPR1000RxType[mapChn], pHost->sysHost.gPR1000RxResol[mapChn]);

#ifndef DONT_SUPPORT_EVENT_FUNC
			Dbg("VEVENT ClearMask & Write Maskformat Pz/Md/Bd [mapChn:%d]\n", mapChn);
			PR1000_VEVENT_ClearMask(fd, &pHost->sysHost.portChSel[mapChn]);
			PR1000_VEVENT_SetMaskAttr(fd, &pHost->sysHost.portChSel[mapChn], pHost->sysHost.gPR1000RxResol[mapChn], &pr1000_mask_attr_table_vevent[pHost->sysHost.gPR1000RxResol[mapChn]]);
			PR1000_VEVENT_GetMaskAttr(fd, &pHost->sysHost.portChSel[mapChn], pHost->sysHost.gPR1000RxResol[mapChn], &stMaskCellAttr);
			/* get default attribute */
			PR1000_VEVENT_GetMdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stMdAttr);
			PR1000_VEVENT_GetBdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stBdAttr);
			PR1000_VEVENT_GetNdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stNdAttr);
			PR1000_VEVENT_GetDdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDdAttr);
			PR1000_VEVENT_GetDfdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDfdAttr);

			Print("VEVENT Set attribute of Md/Bd/Nd/Dd/Dfd [mapChn:%d]\n", mapChn);
			PR1000_VEVENT_SetMdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stMdAttr);
			stBdSpSens.spsens = ((stMaskCellAttr.cellCntX * stMaskCellAttr.cellCntY)/100)*50; //Set 50% blind.
			PR1000_VEVENT_SetBdSpSens(fd, &pHost->sysHost.portChSel[mapChn], &stBdSpSens);
			PR1000_VEVENT_SetBdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stBdAttr);
			PR1000_VEVENT_SetNdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stNdAttr);
			PR1000_VEVENT_SetDdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDdAttr);
			PR1000_VEVENT_SetDfdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDfdAttr);

			PR1000_VEVENT_SetDisplayCellFormat(fd, &pHost->sysHost.portChSel[mapChn], &stDefVEVENTDisplayAttr);
#endif // DONT_SUPPORT_EVENT_FUNC
		}
	}

	/* set video output. */
	PR1000_VID_SetOutFormatAttr(fd, &pHost->sysHost.stOutFormatAttr);
	/////////////////////////////////////////////////////////////

#ifndef DONT_SUPPORT_AUD_ALINK
	for(chip = PR1000_MASTER; chip < PR1000_CHIP_COUNT; chip++)
	{
		Dbg("AUD Enable [chip:%d]\n", chip);
		PR1000_AUD_Enable(fd, chip, ENABLE);

		Dbg("AUD Set Rec attribute [chip:%d]\n", chip);
		PR1000_AUD_SetRecAttr(fd, chip, &stDefAUDRecAttr);

		Dbg("AUD Set Pb attribute [chip:%d]\n", chip);
		PR1000_AUD_SetPbAttr(fd, chip, &stDefAUDPbAttr);

		Dbg("AUD Disable Mute Rec/Mix/Voc/Dac [chip:%d]\n", chip);
		for(prChn = 0; prChn < DEF_PR1000_MAX_CHN; prChn++)
		{
			PR1000_AUD_SetRecMute(fd, chip, prChn, DISABLE);
		}
		PR1000_AUD_SetMixMute(fd, chip, DISABLE);
		PR1000_AUD_SetVocMute(fd, chip, DISABLE);
		PR1000_AUD_SetDacMute(fd, chip, DISABLE);

		Dbg("AUD Set dac [chip:%d, ch:%d]\n", chip COMMA 0);
		PR1000_AUD_SetDacChn(fd, chip, 0); //0~15:live, 16:Pb, 17:Voice, 18:Mix

		{
			_stAUDDetAttr stAUDDetAttr;

			Dbg("AUD Get detect attr. [prChn:%d]\n", chip);
			PR1000_AUD_GetDetAttr(fd, chip, &stAUDDetAttr);
			Dbg("AUD Set detect attr. [prChn:%d]\n", chip);
			PR1000_AUD_SetDetAttr(fd, chip, &stAUDDetAttr);
		}

		if(chip == PR1000_MASTER)
		{
			stAUDCascadeAttr.bCascadeMaster = 1;
			for(prChn = 0; prChn < DEF_PR1000_MAX_CHN*PR1000_CHIP_COUNT; prChn++)
			{
				PR1000_AUD_SetRecMute(fd, chip, prChn, DISABLE);
			}
		}
		else
		{
			stAUDCascadeAttr.bCascadeMaster = 0;
		}
		PR1000_AUD_SetCascadeAttr(fd, chip, &stAUDCascadeAttr);
	}
#endif // DONT_SUPPORT_AUD_ALINK

	PR1000_IRQ_Init(fd, pHost); //init all chip.

#if 0
	//power down all
	{/*{{{*/
		uint8_t page;
		uint8_t i2cSlaveAddr = 0;
		uint8_t i2cReg = 0;
		uint8_t i2cData = 0;
		uint8_t i2cMask = 0;

		PrintString("########## power down all ##############\n");

		i2cSlaveAddr = PR1000_I2C_SLVADDRS[0];
		page = PR1000_REG_PAGE_COMMON;

		i2cReg = 0xE6;
		i2cData = 0x80;
		Print("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
		if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
		i2cReg = 0xE7;
		i2cData = 0x8B;
		Print("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
		if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
		i2cReg = 0xEA;
		i2cData = 0xA0;
		Print("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
		if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
		i2cReg = 0xEB;
		i2cData = 0xE9;
		Print("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
		if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
		i2cReg = 0xEC;
		i2cData = 0xA8;
		Print("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
		if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
		i2cReg = 0xED;
		i2cData = 0xA8;
		Print("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
		if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		page = PR1000_REG_PAGE_VDEC1;
		i2cReg = 0x68;
		i2cData = 0x30;
		Print("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
		if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
		i2cReg = 0xE8;
		i2cData = 0x30;
		Print("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
		if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		page = PR1000_REG_PAGE_VDEC2;
		i2cReg = 0x68;
		i2cData = 0x30;
		Print("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
		if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
		i2cReg = 0xE8;
		i2cData = 0x30;
		Print("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
		if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		page = PR1000_REG_PAGE_COMMON;
		i2cReg = 0xA4;
		i2cData = 0x00;
		Print("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
		if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

	}/*}}}*/
#endif

	return 0;
}

#ifndef DONT_SUPPORT_ETC_FUNC
int PR1000_SetPwDown(const int fd, const _stPrPwDown *pstPrPwDown)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	uint8_t prChn, i2cSlaveAddr; 

	if( (pstPrPwDown == NULL) )
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	if( pstPrPwDown->prChip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[pstPrPwDown->prChip];

	//uint8_t bVDEC_CLK_PD0; //prchn 0, page0 0xEC b'3
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xEC;
	i2cData = ((pstPrPwDown->bVDEC_CLK_PD0)&0x1) << 3;
	i2cMask = 0x08;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t bVDEC_CLK_PD1; //prchn 1, page0 0xEC b'7
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xEC;
	i2cData = ((pstPrPwDown->bVDEC_CLK_PD1)&0x1) << 7;
	i2cMask = 0x80;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t bVDEC_CLK_PD2; //prchn 2, page0 0xED b'3
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xED;
	i2cData = ((pstPrPwDown->bVDEC_CLK_PD2)&0x1) << 3;
	i2cMask = 0x08;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t bVDEC_CLK_PD3; //prchn 3, page0 0xED b'7
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xED;
	i2cData = ((pstPrPwDown->bVDEC_CLK_PD3)&0x1) << 7;
	i2cMask = 0x80;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t bAUDIO_ALINK_PD; //0xEA b'7
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xEA;
	i2cData = ((pstPrPwDown->bAUDIO_ALINK_PD)&0x1) << 7;
	i2cMask = 0x80;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xEB;
	if(pstPrPwDown->bAUDIO_ALINK_PD)
	{
		i2cData = 0xE0;
	}
	else
	{
		i2cData = 0x00;
	}
	i2cMask = 0xE0;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	//uint8_t bVADC_PD0; //prchn 0, page1 0x68 b'4
	prChn = 0;
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x68;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = (pstPrPwDown->bVADC_PD0&0x1)<<4;
	i2cMask = 0x10;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t bVADC_PD1; //prchn 1, page1 0xE8 b'4
	prChn = 1;
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x68;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = (pstPrPwDown->bVADC_PD1&0x1)<<4;
	i2cMask = 0x10;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t bVADC_PD2; //prchn 2, page2 0x68 b'4
	prChn = 2;
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x68;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = (pstPrPwDown->bVADC_PD2&0x1)<<4;
	i2cMask = 0x10;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t bVADC_PD3; //prchn 3, page2 0xE8 b'4
	prChn = 3;
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x68;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = (pstPrPwDown->bVADC_PD3&0x1)<<4;
	i2cMask = 0x10;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	//uint8_t bAADC_PD; //page0 0xEB b'[7:5]
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xEB;
	i2cData = (pstPrPwDown->AADC_PD&0x7)<<5;
	i2cMask = 0xE0;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	//uint8_t bPLL1_PD; //page0 0xE6 b'7 All digital/anglog
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xE6;
	i2cData = ((pstPrPwDown->bPLL1_PD)&0x1) << 7;
	i2cMask = 0x80;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t bPLL2_PD; //page0 0xE7 b'7 Only video output
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xE7;
	i2cData = ((pstPrPwDown->bPLL2_PD)&0x1) << 7;
	i2cMask = 0x80;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_GetPwDown(const int fd, _stPrPwDown *pstPrPwDown)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	uint8_t prChn, i2cSlaveAddr; 

	if( (pstPrPwDown == NULL) )
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	if( pstPrPwDown->prChip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[pstPrPwDown->prChip];

	//uint8_t bVDEC_CLK_PD0; //prchn 0, page0 0xEC b'3
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xEC;
	i2cMask = 0x08;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPrPwDown->bVDEC_CLK_PD0 = (i2cData>>3)&0x1;
	//uint8_t bVDEC_CLK_PD1; //prchn 1, page0 0xEC b'7
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xEC;
	i2cMask = 0x80;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPrPwDown->bVDEC_CLK_PD1 = (i2cData>>7)&0x1;
	//uint8_t bVDEC_CLK_PD2; //prchn 2, page0 0xED b'3
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xED;
	i2cMask = 0x08;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPrPwDown->bVDEC_CLK_PD2 = (i2cData>>3)&0x1;
	//uint8_t bVDEC_CLK_PD3; //prchn 3, page0 0xED b'7
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xED;
	i2cMask = 0x80;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPrPwDown->bVDEC_CLK_PD3 = (i2cData>>7)&0x1;
	//uint8_t bAUDIO_ALINK_PD; //0xEA b'7
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xEA;
	i2cMask = 0x80;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPrPwDown->bAUDIO_ALINK_PD = (i2cData>>7)&0x1;

	//uint8_t bVADC_PD0; //prchn 0, page1 0x68 b'4
	prChn = 0;
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x68;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x10;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPrPwDown->bVADC_PD0 = (i2cData>>4)&0x1;
	//uint8_t bVADC_PD1; //prchn 1, page1 0xE8 b'4
	prChn = 1;
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x68;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x10;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPrPwDown->bVADC_PD1 = (i2cData>>4)&0x1;
	//uint8_t bVADC_PD2; //prchn 2, page2 0x68 b'4
	prChn = 2;
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x68;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x10;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPrPwDown->bVADC_PD2 = (i2cData>>4)&0x1;
	//uint8_t bVADC_PD3; //prchn 3, page2 0xE8 b'4
	prChn = 3;
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x68;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x10;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPrPwDown->bVADC_PD3 = (i2cData>>4)&0x1;

	//uint8_t bAADC_PD; //page0 0xEB b'[7:5]
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xEB;
	i2cMask = 0xE0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPrPwDown->AADC_PD = (i2cData>>5)&0x7;

	//uint8_t bPLL1_PD; //page0 0xE6 b'7 All digital/anglog
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xE6;
	i2cMask = 0x80;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPrPwDown->bPLL1_PD = (i2cData>>7)&0x1;
	//uint8_t bPLL2_PD; //page0 0xE7 b'7 Only video output
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xE7;
	i2cMask = 0x80;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPrPwDown->bPLL2_PD = (i2cData>>7)&0x1;


	return(ret);
}
#endif // DONT_SUPPORT_ETC_FUNC


//////////////////////////////////////////////////////////////////////////////////////////////////////
/*** Interrupt Service Routine (ISR) control. ***/
//////////////////////////////////////////////////////////////////////////////////////////////////////
int PR1000_IRQ_Init(const int fd, _drvHost *pHost)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t i2cSlaveAddr; 
	uint8_t chip;
        int page;
	uint8_t mapChn; 
	uint8_t prChip, prChn; 

	int intSyncPeriod;
	_stPortChSel *pstPortChSel = NULL;
	unsigned long bitDonePrChipFlag = 0;

	PrintString("> PixelPlus PR1000 Init Irq\n");

	for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) continue;

		if( (pstPortChSel = (_stPortChSel *)&pHost->sysHost.portChSel[mapChn]) == NULL)
		{
			ErrorString("Invalid argu\n");
			return(-1);
		}

		prChip = pstPortChSel->prChip;
		prChn = pstPortChSel->prChn;
		i2cSlaveAddr = pstPortChSel->i2cSlvAddr;
		Dbg("IRQOut mapChn:%d prChip:%d,prChn:%d,i2cSlv:0x%02x\n", mapChn COMMA prChip COMMA prChn COMMA i2cSlaveAddr);

		if(!_TEST_BIT(prChip, &bitDonePrChipFlag))
		{
			Dbg("Set IRQ prChip:%d\n", prChip);
			_SET_BIT(prChip, &bitDonePrChipFlag);

			/* IRQ Out En */
			{/*{{{*/
				page = PR1000_REG_PAGE_COMMON;
				i2cReg = 0x80;
#ifdef __LINUX_SYSTEM__
				i2cData = 0x40 | 0x0F; //If CPU interrupt low-level mode.
#else // __LINUX_SYSTEM__
				i2cData = 0x40 | 0x10 | 0x0F; //If CPU interrupt edge mode.
#endif // __LINUX_SYSTEM__
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}/*}}}*/

			/* IRQ Sync En */
			{/*{{{*/
				Print("Chip%d Interrupt period sync:%dmsec, wake:%dmsec.\n", prChip COMMA PR1000_INT_SYNC_PERIOD COMMA PR1000_INT_WAKE_PERIOD);
				if(PR1000_INT_SYNC_PERIOD < 100) 
				{
					ErrorString("Invalid int sync period. mininum 100msec.\n");
					return(-1);
				}

				page = PR1000_REG_PAGE_COMMON;
				i2cReg = 0x81;
				//Set sync period. unit:20msec, 0=20msec, 1=40msec....
				intSyncPeriod = PR1000_INT_SYNC_PERIOD / 20;
				intSyncPeriod = (intSyncPeriod > 0) ? intSyncPeriod-1:0; 
				i2cData = intSyncPeriod; //Set sync period. unit:20msec, 0=20msec, 1=40msec....
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}

				page = PR1000_REG_PAGE_COMMON;
				i2cReg = 0x82;
				i2cData = PR1000_INT_WAKE_PERIOD / PR1000_INT_SYNC_PERIOD;
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}

#if 0 /* Don't use this.  */
				{
					DbgString("Interrupt not linked sync period.\n");
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0x83; //Set sync sel
					/*  DFD|DD|ND|BD|MD|VFD|NOVID|PTZ*/
					i2cData = (0<<7)|(0<<6)|(0<<5)|(0<<4)|(0<<3)|(0<<2)|(0<<1)|(0<<0); 
					if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0x84; //Set sync sel
					/*  GPIO5_1|GPIO0|AD */
					i2cData = (0<<2)|(0<<1)|(0<<0); 
					if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}
				}
#endif
				{
					Dbg("Interrupt linked sync period(%dmsec).\n", (intSyncPeriod+1)*20);
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0x83; //Set sync sel
					/*  DFD|DD|ND|BD|MD|VFD|NOVID|PTZ*/
					/*  sync:DFD|DD|ND|BD|MD|VFD|NOVID, nosync:PTZ*/
					i2cData = (1<<7)|(1<<6)|(1<<5)|(1<<4)|(1<<3)|(1<<2)|(1<<1)|(0<<0);
					if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0x84; //Set sync sel
					/*  GPIO5_1|GPIO0|AD */
					i2cData = (1<<2)|(1<<1)|(1<<0); 
					if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}
				}

			}/*}}}*/
		}
	}

	for(chip = PR1000_MASTER; chip < PR1000_CHIP_COUNT; chip++)
	{
		i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];
		page = PR1000_REG_PAGE_COMMON;

		/* IRQ ENA PTZ(ch0~ch3) */
		i2cReg = 0xA0; //ch0
		i2cData = 0x00; //Set ptz disable 
		if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		i2cReg = 0xA1; //ch1
		i2cData = 0x00; //Set ptz disable 
		if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		i2cReg = 0xA2; //ch2
		i2cData = 0x00; //Set ptz disable 
		if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		i2cReg = 0xA3; //ch3
		i2cData = 0x00; //Set ptz disable 
		if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}
	/* IRQ ENA PTZ/MD/ND/DFD/AD/WAKE/GPIO */
	for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) continue;

		if( (pstPortChSel = (_stPortChSel *)&pHost->sysHost.portChSel[mapChn]) == NULL)
		{
			ErrorString("Invalid argu\n");
			return(-1);
		}

		prChip = pstPortChSel->prChip;
		prChn = pstPortChSel->prChn;
		i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

		Dbg("PTZ mapChn:%d prChip:%d,prChn:%d,i2cSlv:0x%02x\n", mapChn COMMA prChip COMMA prChn COMMA i2cSlaveAddr);
		{/*{{{*/
			page = PR1000_REG_PAGE_COMMON;

			if(prChn == 0)
			{
				/* IRQ ENA PTZ(ch0~ch3) */
				i2cReg = 0xA0; //ch0
				i2cData = 0xBF; //Set ptz enable 
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}
			else if(prChn == 1)
			{
				i2cReg = 0xA1; //ch1
				i2cData = 0xBF; //Set ptz enable 
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}
			else if(prChn == 2)
			{
				i2cReg = 0xA2; //ch2
				i2cData = 0xBF; //Set ptz enable 
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}
			else //if(prChn == 3)
			{
				i2cReg = 0xA3; //ch3
				i2cData = 0xBF; //Set ptz enable 
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}
		}/*}}}*/
	}

	/* IRQ ENA NOVID/MD/ND/DFD/AD/GPIO */
	{
		uint8_t i2cDataNovid = 0;
		uint8_t i2cDataMd = 0;
		uint8_t i2cDataNd = 0;
		uint8_t i2cDataDfd = 0;
		uint8_t i2cDataAd = 0;
		uint8_t i2cDataGpio0 = 0;
		uint8_t i2cDataGpio1 = 0;
		uint8_t i2cDataGpio2 = 0;
		uint8_t i2cDataGpio3 = 0;
		uint8_t i2cDataGpio4 = 0;
		uint8_t i2cDataGpio5 = 0;


		for(chip = PR1000_MASTER; chip < PR1000_CHIP_COUNT; chip++)
		{/*{{{*/
			i2cDataNovid = 0;
			i2cDataMd = 0;
			i2cDataNd = 0;
			i2cDataDfd = 0;
			i2cDataAd = 0;
			i2cDataGpio0 = 0;
			i2cDataGpio1 = 0;
			i2cDataGpio2 = 0;
			i2cDataGpio3 = 0;
			i2cDataGpio4 = 0;
			i2cDataGpio5 = 0;

			for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
			{/*{{{*/
				if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) continue;

				if( (pstPortChSel = (_stPortChSel *)&pHost->sysHost.portChSel[mapChn]) == NULL)
				{
					ErrorString("Invalid argu\n");
					return(-1);
				}

				prChip = pstPortChSel->prChip;
				prChn = pstPortChSel->prChn;
				i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

				if(prChip != chip) continue;

				i2cDataNovid |= (1<<(prChn+4))|(1<<prChn); //VFD|NOVID
				i2cDataMd |= (1<<(prChn+4))|(1<<prChn); //BD|MD
				i2cDataNd |= (1<<(prChn+4))|(1<<prChn); //DD|ND
				i2cDataDfd |= (1<<(prChn+4))|(1<<prChn); //NOAUD|DFD
				i2cDataAd |= (1<<(prChn+4))|(1<<prChn); //AD_DIFF|AD_ABS
				i2cDataGpio0 |= (0<<(prChn+4))|(0<<prChn); //GPIO0
				i2cDataGpio1 |= (0<<(prChn+4))|(0<<prChn); //GPIO1
				i2cDataGpio2 |= (0<<(prChn+4))|(0<<prChn); //GPIO2
				i2cDataGpio3 |= (0<<(prChn+4))|(0<<prChn); //GPIO3
				i2cDataGpio4 |= (0<<(prChn+4))|(0<<prChn); //GPIO4
				i2cDataGpio5 |= (0<<(prChn+4))|(0<<prChn); //GPIO5
				if(chip == PR1000_MASTER) i2cDataGpio5 |= (1<<7); //only master chip, enable WAKE

				/* Initailize first connect camera */
				{
					/* set next interrupt level */
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0x90;
					i2cMask = 0x01<<prChn; 
					i2cData = (WAIT_NEXT_NOVIDEO_FALSE)<<prChn; //next interrupt video.
					Dbg("Novideo Level prChn:%d change(0x%x)\n", prChn COMMA i2cMask & i2cData);
					Dbg("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
					if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{     
						ErrorString("Write reg.\n");
					}             
				}

			}/*}}}*/

			Dbg("IRQEn chip:%d\n", chip);

			i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];
			page = PR1000_REG_PAGE_COMMON;

			i2cReg = 0xA4; //NOVID
			i2cData = i2cDataNovid;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xA5; //MD
			i2cData = i2cDataMd;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xA6; //ND
			i2cData = i2cDataNd;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xA7; //DFD
			i2cData = i2cDataDfd;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xA8; //AD
			i2cData = i2cDataAd;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xA9; //GPIO0
			i2cData = i2cDataGpio0;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xAA; //GPIO1
			i2cData = i2cDataGpio1;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xAB; //GPIO2
			i2cData = i2cDataGpio2;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xAC; //GPIO3
			i2cData = i2cDataGpio3;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xAD; //GPIO4
			i2cData = i2cDataGpio4;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xAE; //WAKE|GPIO5
			i2cData = i2cDataGpio5;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}/*}}}*/
	}

	for(chip = PR1000_MASTER; chip < PR1000_CHIP_COUNT; chip++)
	{
		i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

		/* IRQ Clr */
		{/*{{{*/
			/* IRQ Clr PTZ(ch0~ch3) */
			page = PR1000_REG_PAGE_COMMON;
			i2cReg = 0xB0; //ch0
			i2cData = 0xFF; //Clr
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xB1; //ch1
			i2cData = 0xFF; //Clr
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xB2; //ch2
			i2cData = 0xFF; //Clr
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xB3; //ch3
			i2cData = 0xFF; //Clr
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			/* IRQ Clr NOVID/MD/ND/DFD/AD/GPIO */
			page = PR1000_REG_PAGE_COMMON;
			i2cReg = 0xB4; //VFD|NOVID
			i2cData = 0xFF; //Clr
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xB5; //BD|MD
			i2cData = 0xFF; //Clr
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xB6; //DD|MD
			i2cData = 0xFF; //Clr
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xB7; //NOAUD|DFD
			i2cData = 0xFF; //Clr
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xB8; //AD_DIFF|AD_ABS
			i2cData = 0xFF; //Clr
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xB9; //GPIO0
			i2cData = 0xFF; //Clr
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xBA; //GPIO1
			i2cData = 0xFF; //Clr
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xBB; //GPIO2
			i2cData = 0xFF; //Clr
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xBC; //GPIO3
			i2cData = 0xFF; //Clr
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xBD; //GPIO4
			i2cData = 0xFF; //Clr
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			i2cReg = 0xBE; //WAKE|GPIO5
			i2cData = 0xFF; //Clr
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}/*}}}*/

		/* IRQ Init Vfd level */
		{/*{{{*/
			page = PR1000_REG_PAGE_COMMON;
			i2cReg = 0x90;
			i2cMask = 0xF0;
			i2cData = 0xF0;
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}/*}}}*/

	}

	return(ret);
}

int PR1000_Kthread(void *arg)
{
	const int fd = 0; // should do in Kthread(). fd = 0

	int ret = 0;

	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 
	_stPortChSel *pstPortChSel = NULL;
	uint32_t defChromaPhase = 0;
	uint32_t tunnChromaPhase;
	uint8_t format = 0;
	uint8_t resol = 0;
	_stCEQData *pstCEQData = NULL;
	_PR1000_REG_TABLE_VDEC_SD *pTableSD = NULL;
	_PR1000_REG_TABLE_VDEC_HD *pTableHD = NULL;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
	_PR1000_REG_TABLE_VDEC_HD_EXTEND *pTableHD_EXTEND = NULL;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
	uint16_t tblData = 0;
	uint8_t regs[4];
	_stCeqDet stCeqDet;
	_stCeqLock stCeqLock;

	_stCeqInfoReg stCeqInfoReg;

	uint8_t estResult[2];
	uint16_t estResultFact[2];
	uint8_t bWrAtten;
	uint8_t bWrComp;

	/* endless loop */
#ifdef __LINUX_SYSTEM__
        _drvHost *pHost = (_drvHost *)arg;
	uint8_t timeWake = 0;

	set_freezable();
	while(1)
	{
		msleep_interruptible(PR1000_THREAD_POLLING_PERIOD);

		if( try_to_freeze()) continue;

		if(kthread_should_stop()) break;

		/////////////////////////////////////////////////
		/* wait event */
		if(pHost->irq <= 0) //Don't irq, check polling.
		{
			PR1000_IRQ_Isr(fd, pHost);
		}
		/////////////////////////////////////////////////

		/* check chroma lock when loading table. */
		timeWake++;
		if(timeWake >= 2) //check every PR1000_THREAD_POLLING_PERIOD * 2 time.
		{/*{{{*/
			timeWake = 0;

			/* chroma lock check. */
			for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
			{
				if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) continue;

				//check indicate start flag
				if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock) == 0)  continue;

				Dbg("mapChn:%d, bit:%d\n", mapChn COMMA (_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock)==0?0:1));

				pstPortChSel = (_stPortChSel *)&pHost->sysHost.portChSel[mapChn];
				prChip = pstPortChSel->prChip;
				prChn = pstPortChSel->prChn;
				i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

				/* chroma lock check. */
				{/*{{{*/
					defChromaPhase = 0;
					tblData = 0;
					pstCEQData = (_stCEQData *)&pHost->sysHost.stCEQDataList[mapChn]; //Max 4chip * 4channel
					if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnLoadedRegTable))
					{
						format = pHost->sysHost.gPR1000RxType[mapChn];
						resol = pHost->sysHost.gPR1000RxResol[mapChn];
						pstCEQData->estStep = PR1000_CEQ_STEP_ESTSTART;
					}
					else
					{
						format = pstCEQData->format;
						resol = pstCEQData->camResol;
					}

					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0x00 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
					if( PR1000_PageReadBurst(fd, i2cSlaveAddr, page, i2cReg, 2, regs) < 0)
					{
						ErrorString("PageReadBurst.\n");
						break;
					}
					stCeqDet.reg = regs[0];
					stCeqLock.reg = regs[1];
					Dbg("Step:%d reg[%02x] lock[hpll:%d,hper:%d,std:%d]\n", pstCEQData->estStep COMMA
							stCeqLock.reg COMMA
							stCeqLock.b.lock_hpll COMMA
							stCeqLock.b.lock_hperiod COMMA
							stCeqLock.b.lock_std);

					if( (stCeqLock.b.lock_hpll == 1) && (stCeqLock.b.lock_hperiod == 1) && (stCeqLock.b.lock_std == 1) )
					{/*{{{*/
						Dbg("reg 0x%02x  std:%d,ref:%d,res:%d\n", stCeqDet.reg COMMA
								stCeqDet.b.det_ifmt_std COMMA
								stCeqDet.b.det_ifmt_ref COMMA
								stCeqDet.b.det_ifmt_res);

						Dbg("mapChn:%d format:%d, lockcnt:%d\n", mapChn COMMA format COMMA pHost->sysHost.cntChromaLockTunn[mapChn]);
						Print("mapChn:%d reg[%02x] det[signal:%d, chroma:%d], lock[chroma:%d, hpll:%d, std:%d]\n", mapChn COMMA
										stCeqLock.reg COMMA
										stCeqLock.b.det_signal COMMA
										stCeqLock.b.det_chroma COMMA
										stCeqLock.b.lock_chroma COMMA
										stCeqLock.b.lock_hpll COMMA
										stCeqLock.b.lock_std);

						if( (stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 1) ) 
						{
							Print("--> Lock chroma. [mapChn:%d, cnt:%d]\n", mapChn COMMA pHost->sysHost.cntChromaLockTunn[mapChn]);
							pHost->sysHost.lastChromaLockTunn[mapChn] = pHost->sysHost.cntChromaLockTunn[mapChn];

							pHost->sysHost.cntChromaLockTunn[mapChn] = 0; //init
							_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock);
							_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
							_SET_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);
							if(pstCEQData->monStep == PR1000_MON_STEP_NONE)
							{
								pstCEQData->eqProcFlag.C_LOCK_CNT = 1;
								pstCEQData->monStep = PR1000_MON_STEP_START;

								#if 0
								/* immediately start eq tunning. Do after load vid table. */
								if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) == 0 )
								{
									Print("immediately start eq tunning by ceqstep. mapChn:%d\n", mapChn);
									_SET_BIT(mapChn, &pHost->sysHost.bitChnWakeIsrImmediately);
								}
								#endif
							}
							else if(pstCEQData->monStep > PR1000_MON_STEP_NONE)
							{
								/* immediately start eq tunning. Do after load vid table. */
								if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) == 0 )
								{
									Print("immediately start eq tunning by monstep. mapChn:%d\n", mapChn);
									_SET_BIT(mapChn, &pHost->sysHost.bitChnWakeIsrImmediately);
								}
							}
							continue;
						}
						else if( (stCeqLock.b.det_chroma == 0) && (stCeqLock.b.lock_chroma == 0) ) 
						{
							Print("Don't detect chroma. Skip. [mapChn:%d]\n", mapChn);

							pHost->sysHost.cntChromaLockTunn[mapChn] = 0; //init
							_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock);
							_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
							_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);

							Print("CurDetect[%s, %s]\n", _STR_PR1000_FORMAT[pstCEQData->format] COMMA _STR_PR1000_INRESOL[pstCEQData->camResol]);
							continue;
						}
						else
						{
							if(pHost->sysHost.cntChromaLockTunn[mapChn] < MAX_CNT_CHROMALOCK)
							{
								/* check chroma detect ok and lock fail */
								if( (stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 0) ) 
								{/*{{{*/
									Print("Don't chroma lock => mapChn:%d reg[%02x] det[signal:%d, chroma:%d], lock[chroma:%d, hpll:%d, std:%d]\n", mapChn COMMA
											stCeqLock.reg COMMA
											stCeqLock.b.det_signal COMMA
											stCeqLock.b.det_chroma COMMA
											stCeqLock.b.lock_chroma COMMA
											stCeqLock.b.lock_hpll COMMA
											stCeqLock.b.lock_std);

									/* get default chromaphase value from table */
									{/*{{{*/
										switch(format)
										{/*{{{*/
											case pr1000_format_SD720:
											case pr1000_format_SD960:
												{
													pTableSD = (_PR1000_REG_TABLE_VDEC_SD *)pr1000_reg_table_vdec_SD;
												}
												break;
#ifndef DONT_SUPPORT_STD_PVI
											case pr1000_format_PVI:
												{
													pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_PVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
													pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_PVI_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
												}
												break;
#endif // DONT_SUPPORT_STD_PVI
											case pr1000_format_HDA:
												{
													pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_HDA;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
													pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_HDA_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
												}
												break;
											case pr1000_format_CVI:
												{
													pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_CVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
													pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_CVI_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
												}
												break;
											case pr1000_format_HDT:
												{
													pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_HDT;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
													pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_HDT_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
												}
												break;
											case pr1000_format_HDT_NEW:
												{
													pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_HDT_NEW;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
													pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_HDT_NEW_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
												}
												break;
											default: 
												{
													pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_CVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
													pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_CVI_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
												}
												break;
										}/*}}}*/

										defChromaPhase = 0;
										switch(format)
										{
											case pr1000_format_SD720:
											case pr1000_format_SD960:
												{/*{{{*/
													while(pTableSD->addr != 0xFF)
													{
														i2cReg = pTableSD->addr;
														tblData = pTableSD->pData[resol];
														pTableSD++;
														if((i2cReg != 0x46) && (i2cReg != 0x47) && (i2cReg != 0x48) ) continue;

														if(tblData == 0xFF00) continue; //skip
														else if(tblData == 0xFFFF) break; //end
														else
														{
															page = (tblData>>8);
															Dbg("page:%d, reg:%x\n", page COMMA i2cReg);
															if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x46) )
															{
																defChromaPhase |= (uint8_t)tblData << 16;
															}
															else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x47) )
															{
																defChromaPhase |= (uint8_t)tblData << 8;
															}
															else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x48) )
															{
																defChromaPhase |= (uint8_t)tblData;
																break;
															}
														}
													}

												}/*}}}*/
												break;
#ifndef DONT_SUPPORT_STD_PVI
											case pr1000_format_PVI:
#endif // DONT_SUPPORT_STD_PVI
											case pr1000_format_HDA:
											case pr1000_format_CVI:
											case pr1000_format_HDT:
											case pr1000_format_HDT_NEW:
												{/*{{{*/
													if(resol <= pr1000_outresol_1920x1080p25)
													{
														while(pTableHD->addr != 0xFF)
														{/*{{{*/
															i2cReg = pTableHD->addr;
															tblData = pTableHD->pData[resol - pr1000_outresol_1280x720p60];
															pTableHD++;
															if((i2cReg != 0x46) && (i2cReg != 0x47) && (i2cReg != 0x48) ) continue;

															if(tblData == 0xFF00) continue; //skip
															else if(tblData == 0xFFFF) break; //end
															else
															{
																page = (tblData>>8);
																Dbg("page:%d, reg:%x\n", page COMMA i2cReg);
																if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x46) )
																{
																	defChromaPhase |= (uint8_t)tblData << 16;
																}
																else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x47) )
																{
																	defChromaPhase |= (uint8_t)tblData << 8;
																}
																else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x48) )
																{
																	defChromaPhase |= (uint8_t)tblData;
																	break;
																}
															}
														}/*}}}*/
													}
													#ifndef DONT_SUPPORT_VDRESOL_EXTEND
													else
													{
														while(pTableHD_EXTEND->addr != 0xFF)
														{/*{{{*/
															i2cReg = pTableHD_EXTEND->addr;
															tblData = pTableHD_EXTEND->pData[resol - pr1000_outresol_1280x720p60c];
															pTableHD_EXTEND++;
															if((i2cReg != 0x46) && (i2cReg != 0x47) && (i2cReg != 0x48) ) continue;

															if(tblData == 0xFF00) continue; //skip
															else if(tblData == 0xFFFF) break; //end
															else
															{
																page = (tblData>>8);
																Dbg("page:%d, reg:%x\n", page COMMA i2cReg);
																if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x46) )
																{
																	defChromaPhase |= (uint8_t)tblData << 16;
																}
																else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x47) )
																{
																	defChromaPhase |= (uint8_t)tblData << 8;
																}
																else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x48) )
																{
																	defChromaPhase |= (uint8_t)tblData;
																	break;
																}
															}
														}/*}}}*/
													}
													#endif // DONT_SUPPORT_VDRESOL_EXTEND
												}/*}}}*/
												break;
											default: 
												{
													defChromaPhase = 0;
												}
												break;
										}
										Dbg("defChromaPhase:0x%08x\n", defChromaPhase);

									}/*}}}*/

									page = PR1000_VDEC_PAGE(prChn);
									i2cReg = 0x46 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
									if( PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &regs[0]) < 0)
									{
										ErrorString("PageRead.\n");
										break;
									}

									tunnChromaPhase = (pHost->sysHost.cntChromaLockTunn[mapChn] >> 1) + 1;
									tunnChromaPhase *= 40; //+- offset
									if(pHost->sysHost.cntChromaLockTunn[mapChn] & 0x1) // - tunning
									{
										tunnChromaPhase = defChromaPhase - tunnChromaPhase;
									}
									else // + tunning
									{
										tunnChromaPhase = defChromaPhase + tunnChromaPhase;
									}
									Print("mapChn:%d defChromaPhase:0x%08x, tunnPhase:0x%08x\n", mapChn COMMA defChromaPhase COMMA tunnChromaPhase);

									regs[0] &= 0xC0;
									regs[0] |= ((tunnChromaPhase>>16)&0x1F) | 0x20; //0x20:set manual
									regs[1] = ((tunnChromaPhase>>8)&0xFF);
									regs[2] = ((tunnChromaPhase)&0xFF);

									page = PR1000_VDEC_PAGE(prChn);
									i2cReg = 0x46 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
									if( PR1000_PageWriteBurst(fd, i2cSlaveAddr, page, i2cReg, 3, regs) < 0)
									{
										ErrorString("PageReadBurst.\n");
										break;
									}
									/* apply loading register. */
									{/*{{{*/
										page = PR1000_VDEC_PAGE(prChn);
										i2cReg = 0x61 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
										i2cData = 0x0E;
										Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA i2cData);
										if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
										{
											ErrorString("Write reg.\n");
											break;
										}
										page = PR1000_VDEC_PAGE(prChn);
										i2cReg = 0x61 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
										i2cData = 0x0F;
										Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA i2cData);
										if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
										{
											ErrorString("Write reg.\n");
											break;
										}
									}/*}}}*/

									pHost->sysHost.cntChromaLockTunn[mapChn]++;

								}/*}}}*/
							}
							else if(pHost->sysHost.cntChromaLockTunn[mapChn] == MAX_CNT_CHROMALOCK)
							{/*{{{*/
								Error("Over check time chroma lock. [mapChn:%d]\n", mapChn);

								/* get default chromaphase value from table */
								{/*{{{*/
									switch(format)
									{/*{{{*/
										case pr1000_format_SD720:
										case pr1000_format_SD960:
											{
												pTableSD = (_PR1000_REG_TABLE_VDEC_SD *)pr1000_reg_table_vdec_SD;
											}
											break;
#ifndef DONT_SUPPORT_STD_PVI
										case pr1000_format_PVI:
											{
												pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_PVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
												pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_PVI_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
											}
											break;
#endif // DONT_SUPPORT_STD_PVI
										case pr1000_format_HDA:
											{
												pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_HDA;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
												pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_HDA_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
											}
											break;
										case pr1000_format_CVI:
											{
												pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_CVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
												pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_CVI_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
											}
											break;
										case pr1000_format_HDT:
											{
												pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_HDT;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
												pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_HDT_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
											}
											break;
										case pr1000_format_HDT_NEW:
											{
												pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_HDT_NEW;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
												pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_HDT_NEW_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
											}
											break;
										default: 
											{
												pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_CVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
												pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_CVI_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
											}
											break;
									}/*}}}*/

									defChromaPhase = 0;
									switch(format)
									{
										case pr1000_format_SD720:
										case pr1000_format_SD960:
											{/*{{{*/
												while(pTableSD->addr != 0xFF)
												{
													i2cReg = pTableSD->addr;
													tblData = pTableSD->pData[resol];
													pTableSD++;
													if((i2cReg != 0x46) && (i2cReg != 0x47) && (i2cReg != 0x48) ) continue;

													if(tblData == 0xFF00) continue; //skip
													else if(tblData == 0xFFFF) break; //end
													else
													{
														page = (tblData>>8);
														Dbg("page:%d, reg:%x\n", page COMMA i2cReg);
														if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x46) )
														{
															defChromaPhase |= (uint8_t)tblData << 16;
														}
														else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x47) )
														{
															defChromaPhase |= (uint8_t)tblData << 8;
														}
														else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x48) )
														{
															defChromaPhase |= (uint8_t)tblData;
															break;
														}
													}
												}

											}/*}}}*/
											break;
#ifndef DONT_SUPPORT_STD_PVI
										case pr1000_format_PVI:
#endif // DONT_SUPPORT_STD_PVI
										case pr1000_format_HDA:
										case pr1000_format_CVI:
										case pr1000_format_HDT:
										case pr1000_format_HDT_NEW:
											{/*{{{*/
												if(resol <= pr1000_outresol_1920x1080p25)
												{
													while(pTableHD->addr != 0xFF)
													{/*{{{*/
														i2cReg = pTableHD->addr;
														tblData = pTableHD->pData[resol - pr1000_outresol_1280x720p60];
														pTableHD++;
														if((i2cReg != 0x46) && (i2cReg != 0x47) && (i2cReg != 0x48) ) continue;

														if(tblData == 0xFF00) continue; //skip
														else if(tblData == 0xFFFF) break; //end
														else
														{
															page = (tblData>>8);
															Dbg("page:%d, reg:%x\n", page COMMA i2cReg);
															if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x46) )
															{
																defChromaPhase |= (uint8_t)tblData << 16;
															}
															else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x47) )
															{
																defChromaPhase |= (uint8_t)tblData << 8;
															}
															else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x48) )
															{
																defChromaPhase |= (uint8_t)tblData;
																break;
															}
														}
													}/*}}}*/
												}
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
												else
												{
													while(pTableHD_EXTEND->addr != 0xFF)
													{/*{{{*/
														i2cReg = pTableHD_EXTEND->addr;
														tblData = pTableHD_EXTEND->pData[resol - pr1000_outresol_1280x720p60c];
														pTableHD_EXTEND++;
														if((i2cReg != 0x46) && (i2cReg != 0x47) && (i2cReg != 0x48) ) continue;

														if(tblData == 0xFF00) continue; //skip
														else if(tblData == 0xFFFF) break; //end
														else
														{
															page = (tblData>>8);
															Dbg("page:%d, reg:%x\n", page COMMA i2cReg);
															if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x46) )
															{
																defChromaPhase |= (uint8_t)tblData << 16;
															}
															else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x47) )
															{
																defChromaPhase |= (uint8_t)tblData << 8;
															}
															else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x48) )
															{
																defChromaPhase |= (uint8_t)tblData;
																break;
															}
														}
													}/*}}}*/
												}
#endif // DONT_SUPPORT_VDRESOL_EXTEND
											}/*}}}*/
											break;
										default: 
											{
												defChromaPhase = 0;
											}
											break;
									}
									Dbg("defChromaPhase:0x%08x\n", defChromaPhase);

								}/*}}}*/

								page = PR1000_VDEC_PAGE(prChn);
								i2cReg = 0x46 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
								if( PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &regs[0]) < 0)
								{
									ErrorString("PageRead.\n");
									break;
								}

								regs[0] &= 0xC0;
								regs[0] |= ((defChromaPhase>>16)&0x1F) | 0x00; //0x00:set auto
								regs[1] = ((defChromaPhase>>8)&0xFF);
								regs[2] = ((defChromaPhase)&0xFF);

								page = PR1000_VDEC_PAGE(prChn);
								i2cReg = 0x46 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
								if( PR1000_PageWriteBurst(fd, i2cSlaveAddr, page, i2cReg, 3, regs) < 0)
								{
									ErrorString("PageReadBurst.\n");
									break;
								}
								Print("mapChn:%d defChromaPhase:0x%08x\n", mapChn COMMA defChromaPhase);

								/* apply loading register. */
								{/*{{{*/
									page = PR1000_VDEC_PAGE(prChn);
									i2cReg = 0x61 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
									i2cData = 0x0E;
									if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
									{
										ErrorString("Write reg.\n");
										break;
									}
									page = PR1000_VDEC_PAGE(prChn);
									i2cReg = 0x61 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
									i2cData = 0x0F;
									if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
									{
										ErrorString("Write reg.\n");
										break;
									}
								}/*}}}*/

								pHost->sysHost.cntChromaLockTunn[mapChn] = 0; //init
								_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock);
								_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
								_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);
								continue;
							}/*}}}*/
						}
					}/*}}}*/
					else
					{
						pHost->sysHost.cntChromaLockTunn[mapChn] = 0; //init
						_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock);
						_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
						_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);
					}
				}/*}}}*/

			}

#ifdef SUPPORT_CABLE_EQ
			/* VadcGainSel check. */
			for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
			{
				if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) continue;

				//check indicate start flag
				if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) == 0)  continue;
				if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnLoadedRegTable) == 0) continue;

				Dbg("mapChn:%d, bit:%d\n", mapChn COMMA (_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel)==0?0:1));

				pstPortChSel = (_stPortChSel *)&pHost->sysHost.portChSel[mapChn];
				prChip = pstPortChSel->prChip;
				prChn = pstPortChSel->prChn;
				i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

				if(pHost->sysHost.cntVadcGainSelTunn[mapChn] > MAX_PR1000_CEQ_TBL_NUM)
				{
					_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel);
					_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
					_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
					_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
					continue;
				}

				/* VadcGainSel check. */
				{/*{{{*/
					pstCEQData = (_stCEQData *)&pHost->sysHost.stCEQDataList[mapChn]; //Max 4chip * 4channel
					format = pstCEQData->format;
					resol = pstCEQData->camResol;

					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0x00 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
					if( PR1000_PageReadBurst(fd, i2cSlaveAddr, page, i2cReg, 2, regs) < 0)
					{
						ErrorString("PageReadBurst.\n");
						break;
					}
					stCeqDet.reg = regs[0];
					stCeqLock.reg = regs[1];
					Dbg("Step:%d reg[%02x] lock[hpll:%d,hper:%d,std:%d]\n", pstCEQData->estStep COMMA
							stCeqLock.reg COMMA
							stCeqLock.b.lock_hpll COMMA
							stCeqLock.b.lock_hperiod COMMA
							stCeqLock.b.lock_std);

					if( (stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 1) ) 
					{
						if( PR1000_CEQ_GetInfoRegs(fd, pstPortChSel, &stCeqInfoReg) >= 0)
						{
							uint8_t bFlagChg = FALSE;
							uint16_t detAcGain = stCeqInfoReg.b.det_eq_acgain_h<<8|stCeqInfoReg.b.det_eq_acgain_l;
							int stepDir = 0;
							if( detAcGain < 0x0700 )
							{
								stepDir = -1; // --
								bFlagChg = TRUE;
							}
							else if( detAcGain >= 0x0900 )
							{
								stepDir = 1; // ++
								bFlagChg = TRUE;
							}
							else
							{
								Print("===>> Lock vadc gain sel mapChn:%d detAcGain:0x%04x, tunn:%d\n", mapChn COMMA detAcGain COMMA pHost->sysHost.cntVadcGainSelTunn[mapChn]);

								_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel);
								_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
								_SET_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
								_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
								continue;
							}
							Print("mapChn:%d detAcGain:0x%04x, dir:%d, flagChg:%d, tunn:%d\n", mapChn COMMA detAcGain COMMA stepDir COMMA bFlagChg COMMA pHost->sysHost.cntVadcGainSelTunn[mapChn]);

							if(bFlagChg)
							{/*{{{*/
								uint16_t u16CurFact[2];

								if(pHost->sysHost.cntVadcGainSelTunn[mapChn] == 0)
								{
									pHost->sysHost.dirTunnStep[mapChn] = stepDir; //+ or -
								}
								else
								{
									Print("mapChn:%d, old:%d, new:%d\n", mapChn COMMA pHost->sysHost.dirTunnStep[mapChn] COMMA stepDir); //+ or -
									if(pHost->sysHost.dirTunnStep[mapChn] != stepDir) //+ or -
									{
										pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0; //init
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel);
										_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
									}
								}

								pHost->sysHost.cntVadcGainSelTunn[mapChn]++;

								u16CurFact[0] = pstCEQData->attenFact; 
								u16CurFact[1] = pstCEQData->compFact; 

								Print("mapChn:%d, curdist:atten[%dm],comp[%dm]\n", mapChn COMMA u16CurFact[0] COMMA u16CurFact[1]);
								/* estResult[0]: 0x12,(From Atten value), estResult[1]: 0x13,(From Comp value) */
								/* estResultFact[0]: (From Atten dist value), estResultFact[1]: (From Comp dist value) */
								if( (ret = PR1000_CEQ_GetEQTunnEstFactor(fd, (enum _pr1000_table_format)format, (enum _pr1000_table_inresol)resol, &stCeqInfoReg, stepDir, u16CurFact, estResult, estResultFact)) >= 0)
								{
									// + Enable EQ. write 0x12~0x13 
									bWrAtten = TRUE; bWrComp = FALSE;
									if(bWrAtten)
									{
										pstCEQData->estResultAtten = estResult[0]&0x1F;	//0x12,(From Atten value)
										pstCEQData->attenFact = estResultFact[0];

										page = PR1000_REG_PAGE_COMMON;
										i2cReg = 0x12 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
										estResult[0] |= 0x20; //Enable EQ
										PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, (uint8_t)estResult[0]);
										Print("Set tunn STD EQ value mapChn:%d. (Atten:%dm,0x%x)\n", mapChn COMMA estResultFact[0] COMMA estResult[0]);
									}
									if(bWrComp)
									{
										pstCEQData->estResultComp = estResult[1]&0x3F;	//0x13,(From Comp value)
										pstCEQData->compFact = estResultFact[1];

										page = PR1000_REG_PAGE_COMMON;
										i2cReg = 0x13 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
										PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, (uint8_t)estResult[1]);
										Print("Set tunn STD EQ value mapChn:%d. (Comp:%dm,0x%x)\n", mapChn COMMA estResultFact[1] COMMA estResult[1]);
									}

									if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel))
									{
										/* do vadc gain. Must do after SetEQGain */
										PR1000_CEQ_SetVADCGain(fd, (enum _pr1000_table_format)format, (enum _pr1000_table_inresol)resol, pstPortChSel, pstCEQData);
									}

									if( ret == 0 ) // case step is Min or Max 
									{
										Print("mapChn:%d, case step is Min or Max\n", mapChn); 
										pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0; //init
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel);
										_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
									}
								}
							}/*}}}*/
						}
					}
					else
					{
						pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0; //init
						_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel);
						_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
						_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
						_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
					}
				}/*}}}*/
			}
#endif // SUPPORT_CABLE_EQ

		#if 0 //get direct format detect from driver test
		//////
		{
			_wqPollChnStatus wqPollChnStatus;
			int mapChn = 0;
			_stEventDetStd stEventDetStd;

			memcpy(&wqPollChnStatus, &pHost->wqPollChnStatus, sizeof(_wqPollChnStatus));
			memset(&pHost->wqPollChnStatus, 0, sizeof(_wqPollChnStatus));

			Print(">>>> ch:%d, detstd:%lx, novid:%lx\n", mapChn COMMA 
					wqPollChnStatus.bitWqPollDetStd COMMA wqPollChnStatus.bitWqPollNovid);
			//if( (wqPollChnStatus.bitWqPollDetStd & (1<<mapChn)) && ((wqPollChnStatus.bitWqPollNovid & (1<<mapChn)) == 0) )
			if( (wqPollChnStatus.bitWqPollDetStd & (1<<mapChn)) )
			{
				pHost->eventHost.stEventDetStd[mapChn].mapChn = mapChn;
				memcpy(&stEventDetStd, &pHost->eventHost.stEventDetStd[mapChn], sizeof(_stEventDetStd));

				Print("###### TODO Set ch:%d, format:%d, resol:%d\n", mapChn COMMA stEventDetStd.format COMMA stEventDetStd.resol);
				// Set_RX_MODE ---> //
			}
		}
		//////
		#endif

		}/*}}}*/
	}
	DbgString("stop kthread\n");

#else //#ifdef __LINUX_SYSTEM__

        _drvHost *pHost = (_drvHost *)gpDrvHost;
	static uint8_t timeWake = 0;

	{
		/////////////////////////////////////////////////
		/* wait event */
		if(pHost->irq <= 0) //Don't irq, check polling.
		{
			PR1000_IRQ_Isr(fd, pHost);
		}
		/////////////////////////////////////////////////

		/* check chroma lock when loading table. */
		timeWake++;
		if(timeWake >= 2) //check every PR1000_THREAD_POLLING_PERIOD * 2 time.
		{/*{{{*/
			timeWake = 0;

			/* chroma lock check. */
			for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
			{
				if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) continue;

				//check indicate start flag
				if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock) == 0)  continue;

				Dbg("mapChn:%d, bit:%d\n", mapChn COMMA (_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock)==0?0:1));

				pstPortChSel = (_stPortChSel *)&pHost->sysHost.portChSel[mapChn];
				prChip = pstPortChSel->prChip;
				prChn = pstPortChSel->prChn;
				i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

				/* chroma lock check. */
				{/*{{{*/
					defChromaPhase = 0;
					tblData = 0;
					pstCEQData = (_stCEQData *)&pHost->sysHost.stCEQDataList[mapChn]; //Max 4chip * 4channel
					if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnLoadedRegTable))
					{
						format = pHost->sysHost.gPR1000RxType[mapChn];
						resol = pHost->sysHost.gPR1000RxResol[mapChn];
						pstCEQData->estStep = PR1000_CEQ_STEP_ESTSTART;
					}
					else
					{
						format = pstCEQData->format;
						resol = pstCEQData->camResol;
					}

					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0x00 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
					if( PR1000_PageReadBurst(fd, i2cSlaveAddr, page, i2cReg, 2, regs) < 0)
					{
						ErrorString("PageReadBurst.\n");
						break;
					}
					stCeqDet.reg = regs[0];
					stCeqLock.reg = regs[1];
					Dbg("Step:%d reg[%02x] lock[hpll:%d,hper:%d,std:%d]\n", pstCEQData->estStep COMMA
							stCeqLock.reg COMMA
							stCeqLock.b.lock_hpll COMMA
							stCeqLock.b.lock_hperiod COMMA
							stCeqLock.b.lock_std);

					if( (stCeqLock.b.lock_hpll == 1) && (stCeqLock.b.lock_hperiod == 1) && (stCeqLock.b.lock_std == 1) )
					{/*{{{*/
						Dbg("reg 0x%02x  std:%d,ref:%d,res:%d\n", stCeqDet.reg COMMA
								stCeqDet.b.det_ifmt_std COMMA
								stCeqDet.b.det_ifmt_ref COMMA
								stCeqDet.b.det_ifmt_res);

						Dbg("mapChn:%d format:%d, lockcnt:%d\n", mapChn COMMA format COMMA pHost->sysHost.cntChromaLockTunn[mapChn]);
						Print("mapChn:%d reg[%02x] det[signal:%d, chroma:%d], lock[chroma:%d, hpll:%d, std:%d]\n", mapChn COMMA
										stCeqLock.reg COMMA
										stCeqLock.b.det_signal COMMA
										stCeqLock.b.det_chroma COMMA
										stCeqLock.b.lock_chroma COMMA
										stCeqLock.b.lock_hpll COMMA
										stCeqLock.b.lock_std);

						if( (stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 1) ) 
						{
							Print("--> Lock chroma. [mapChn:%d, cnt:%d]\n", mapChn COMMA pHost->sysHost.cntChromaLockTunn[mapChn]);
							pHost->sysHost.lastChromaLockTunn[mapChn] = pHost->sysHost.cntChromaLockTunn[mapChn];

							pHost->sysHost.cntChromaLockTunn[mapChn] = 0; //init
							_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock);
							_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
							_SET_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);
							if(pstCEQData->monStep == PR1000_MON_STEP_NONE)
							{
								pstCEQData->eqProcFlag.C_LOCK_CNT = 1;
								pstCEQData->monStep = PR1000_MON_STEP_START;

								#if 0
								/* immediately start eq tunning. Do after load vid table. */
								if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) == 0 )
								{
									Print("immediately start eq tunning by ceqstep. mapChn:%d\n", mapChn);
									_SET_BIT(mapChn, &pHost->sysHost.bitChnWakeIsrImmediately);
								}
								#endif
							}
							else if(pstCEQData->monStep > PR1000_MON_STEP_NONE)
							{
								/* immediately start eq tunning. Do after load vid table. */
								if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) == 0 )
								{
									Print("immediately start eq tunning by monstep. mapChn:%d\n", mapChn);
									_SET_BIT(mapChn, &pHost->sysHost.bitChnWakeIsrImmediately);
								}
							}
							continue;
						}
						else if( (stCeqLock.b.det_chroma == 0) && (stCeqLock.b.lock_chroma == 0) ) 
						{
							Print("Don't detect chroma. Skip. [mapChn:%d]\n", mapChn);

							pHost->sysHost.cntChromaLockTunn[mapChn] = 0; //init
							_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock);
							_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
							_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);

							Print("CurDetect[%s, %s]\n", _STR_PR1000_FORMAT[pstCEQData->format] COMMA _STR_PR1000_INRESOL[pstCEQData->camResol]);
							continue;
						}
						else
						{
							if(pHost->sysHost.cntChromaLockTunn[mapChn] < MAX_CNT_CHROMALOCK)
							{
								/* check chroma detect ok and lock fail */
								if( (stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 0) ) 
								{/*{{{*/
									Print("Don't chroma lock => mapChn:%d reg[%02x] det[signal:%d, chroma:%d], lock[chroma:%d, hpll:%d, std:%d]\n", mapChn COMMA
											stCeqLock.reg COMMA
											stCeqLock.b.det_signal COMMA
											stCeqLock.b.det_chroma COMMA
											stCeqLock.b.lock_chroma COMMA
											stCeqLock.b.lock_hpll COMMA
											stCeqLock.b.lock_std);

									/* get default chromaphase value from table */
									{/*{{{*/
										switch(format)
										{/*{{{*/
											case pr1000_format_SD720:
											case pr1000_format_SD960:
												{
													pTableSD = (_PR1000_REG_TABLE_VDEC_SD *)pr1000_reg_table_vdec_SD;
												}
												break;
#ifndef DONT_SUPPORT_STD_PVI
											case pr1000_format_PVI:
												{
													pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_PVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
													pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_PVI_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
												}
												break;
#endif // DONT_SUPPORT_STD_PVI
											case pr1000_format_HDA:
												{
													pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_HDA;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
													pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_HDA_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
												}
												break;
											case pr1000_format_CVI:
												{
													pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_CVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
													pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_CVI_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
												}
												break;
											case pr1000_format_HDT:
												{
													pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_HDT;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
													pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_HDT_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
												}
												break;
											case pr1000_format_HDT_NEW:
												{
													pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_HDT_NEW;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
													pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_HDT_NEW_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
												}
												break;
											default: 
												{
													pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_CVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
													pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_CVI_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
												}
												break;
										}/*}}}*/

										defChromaPhase = 0;
										switch(format)
										{
											case pr1000_format_SD720:
											case pr1000_format_SD960:
												{/*{{{*/
													while(pTableSD->addr != 0xFF)
													{
														i2cReg = pTableSD->addr;
														tblData = pTableSD->pData[resol];
														pTableSD++;
														if((i2cReg != 0x46) && (i2cReg != 0x47) && (i2cReg != 0x48) ) continue;

														if(tblData == 0xFF00) continue; //skip
														else if(tblData == 0xFFFF) break; //end
														else
														{
															page = (tblData>>8);
															Dbg("page:%d, reg:%x\n", page COMMA i2cReg);
															if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x46) )
															{
																defChromaPhase |= (uint32_t)((uint8_t)tblData) << 16;
															}
															else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x47) )
															{
																defChromaPhase |= (uint32_t)((uint8_t)tblData) << 8;
															}
															else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x48) )
															{
																defChromaPhase |= (uint8_t)tblData;
																break;
															}
														}
													}

												}/*}}}*/
												break;
#ifndef DONT_SUPPORT_STD_PVI
											case pr1000_format_PVI:
#endif // DONT_SUPPORT_STD_PVI
											case pr1000_format_HDA:
											case pr1000_format_CVI:
											case pr1000_format_HDT:
											case pr1000_format_HDT_NEW:
												{/*{{{*/
													if(resol <= pr1000_outresol_1920x1080p25)
													{
														while(pTableHD->addr != 0xFF)
														{/*{{{*/
															i2cReg = pTableHD->addr;
															tblData = pTableHD->pData[resol - pr1000_outresol_1280x720p60];
															pTableHD++;
															if((i2cReg != 0x46) && (i2cReg != 0x47) && (i2cReg != 0x48) ) continue;

															if(tblData == 0xFF00) continue; //skip
															else if(tblData == 0xFFFF) break; //end
															else
															{
																page = (tblData>>8);
																Dbg("page:%d, reg:%x\n", page COMMA i2cReg);
																if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x46) )
																{
																	defChromaPhase |= (uint32_t)((uint8_t)tblData) << 16;
																}
																else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x47) )
																{
																	defChromaPhase |= (uint32_t)((uint8_t)tblData) << 8;
																}
																else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x48) )
																{
																	defChromaPhase |= (uint8_t)tblData;
																	break;
																}
															}
														}/*}}}*/
													}
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
													else
													{
														while(pTableHD_EXTEND->addr != 0xFF)
														{/*{{{*/
															i2cReg = pTableHD_EXTEND->addr;
															tblData = pTableHD_EXTEND->pData[resol - pr1000_outresol_1280x720p60c];
															pTableHD_EXTEND++;
															if((i2cReg != 0x46) && (i2cReg != 0x47) && (i2cReg != 0x48) ) continue;

															if(tblData == 0xFF00) continue; //skip
															else if(tblData == 0xFFFF) break; //end
															else
															{
																page = (tblData>>8);
																Dbg("page:%d, reg:%x\n", page COMMA i2cReg);
																if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x46) )
																{
																	defChromaPhase |= (uint8_t)tblData << 16;
																}
																else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x47) )
																{
																	defChromaPhase |= (uint8_t)tblData << 8;
																}
																else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x48) )
																{
																	defChromaPhase |= (uint8_t)tblData;
																	break;
																}
															}
														}/*}}}*/
													}
#endif // DONT_SUPPORT_VDRESOL_EXTEND
												}/*}}}*/
												break;
											default: 
												{
													defChromaPhase = 0;
												}
												break;
										}
										Dbg("defChromaPhase:0x%08x\n", defChromaPhase);

									}/*}}}*/

									page = PR1000_VDEC_PAGE(prChn);
									i2cReg = 0x46 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
									if( PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &regs[0]) < 0)
									{
										ErrorString("PageRead.\n");
										break;
									}

									tunnChromaPhase = (pHost->sysHost.cntChromaLockTunn[mapChn] >> 1) + 1;
									tunnChromaPhase *= 40; //+- offset
									if(pHost->sysHost.cntChromaLockTunn[mapChn] & 0x1) // - tunning
									{
										tunnChromaPhase = defChromaPhase - tunnChromaPhase;
									}
									else // + tunning
									{
										tunnChromaPhase = defChromaPhase + tunnChromaPhase;
									}
									Print("mapChn:%d defChromaPhase:0x%08x, tunnPhase:0x%08x\n", mapChn COMMA defChromaPhase COMMA tunnChromaPhase);

									regs[0] &= 0xC0;
									regs[0] |= ((tunnChromaPhase>>16)&0x1F) | 0x20; //0x20:set manual
									regs[1] = ((tunnChromaPhase>>8)&0xFF);
									regs[2] = ((tunnChromaPhase)&0xFF);

									page = PR1000_VDEC_PAGE(prChn);
									i2cReg = 0x46 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
									if( PR1000_PageWriteBurst(fd, i2cSlaveAddr, page, i2cReg, 3, regs) < 0)
									{
										ErrorString("PageReadBurst.\n");
										break;
									}
									/* apply loading register. */
									{/*{{{*/
										page = PR1000_VDEC_PAGE(prChn);
										i2cReg = 0x61 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
										i2cData = 0x0E;
										if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
										{
											ErrorString("Write reg.\n");
											break;
										}
										page = PR1000_VDEC_PAGE(prChn);
										i2cReg = 0x61 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
										i2cData = 0x0F;
										if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
										{
											ErrorString("Write reg.\n");
											break;
										}
									}/*}}}*/

									pHost->sysHost.cntChromaLockTunn[mapChn]++;

								}/*}}}*/
							}
							else if(pHost->sysHost.cntChromaLockTunn[mapChn] == MAX_CNT_CHROMALOCK)
							{/*{{{*/
								Error("Over check time chroma lock. [mapChn:%d]\n", mapChn);

								/* get default chromaphase value from table */
								{/*{{{*/
									switch(format)
									{/*{{{*/
										case pr1000_format_SD720:
										case pr1000_format_SD960:
											{
												pTableSD = (_PR1000_REG_TABLE_VDEC_SD *)pr1000_reg_table_vdec_SD;
											}
											break;
#ifndef DONT_SUPPORT_STD_PVI
										case pr1000_format_PVI:
											{
												pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_PVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
												pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_PVI_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
											}
											break;
#endif // DONT_SUPPORT_STD_PVI
										case pr1000_format_HDA:
											{
												pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_HDA;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
												pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_HDA_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
											}
											break;
										case pr1000_format_CVI:
											{
												pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_CVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
												pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_CVI_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
											}
											break;
										case pr1000_format_HDT:
											{
												pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_HDT;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
												pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_HDT_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
											}
											break;
										case pr1000_format_HDT_NEW:
											{
												pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_HDT_NEW;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
												pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_HDT_NEW_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
											}
											break;
										default: 
											{
												pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_CVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
												pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_CVI_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
											}
											break;
									}/*}}}*/

									defChromaPhase = 0;
									switch(format)
									{
										case pr1000_format_SD720:
										case pr1000_format_SD960:
											{/*{{{*/
												while(pTableSD->addr != 0xFF)
												{
													i2cReg = pTableSD->addr;
													tblData = pTableSD->pData[resol];
													pTableSD++;
													if((i2cReg != 0x46) && (i2cReg != 0x47) && (i2cReg != 0x48) ) continue;

													if(tblData == 0xFF00) continue; //skip
													else if(tblData == 0xFFFF) break; //end
													else
													{
														page = (tblData>>8);
														Dbg("page:%d, reg:%x\n", page COMMA i2cReg);
														if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x46) )
														{
															defChromaPhase |= (uint32_t)((uint8_t)tblData) << 16;
														}
														else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x47) )
														{
															defChromaPhase |= (uint32_t)((uint8_t)tblData) << 8;
														}
														else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x48) )
														{
															defChromaPhase |= (uint8_t)tblData;
															break;
														}
													}
												}

											}/*}}}*/
											break;
#ifndef DONT_SUPPORT_STD_PVI
										case pr1000_format_PVI:
#endif // DONT_SUPPORT_STD_PVI
										case pr1000_format_HDA:
										case pr1000_format_CVI:
										case pr1000_format_HDT:
										case pr1000_format_HDT_NEW:
											{/*{{{*/
												if(resol <= pr1000_outresol_1920x1080p25)
												{
													while(pTableHD->addr != 0xFF)
													{/*{{{*/
														i2cReg = pTableHD->addr;
														tblData = pTableHD->pData[resol - pr1000_outresol_1280x720p60];
														pTableHD++;
														if((i2cReg != 0x46) && (i2cReg != 0x47) && (i2cReg != 0x48) ) continue;

														if(tblData == 0xFF00) continue; //skip
														else if(tblData == 0xFFFF) break; //end
														else
														{
															page = (tblData>>8);
															Dbg("page:%d, reg:%x\n", page COMMA i2cReg);
															if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x46) )
															{
																defChromaPhase |= (uint32_t)((uint8_t)tblData) << 16;
															}
															else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x47) )
															{
																defChromaPhase |= (uint32_t)((uint8_t)tblData) << 8;
															}
															else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x48) )
															{
																defChromaPhase |= (uint8_t)tblData;
																break;
															}
														}
													}/*}}}*/
												}
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
												else
												{
													while(pTableHD_EXTEND->addr != 0xFF)
													{/*{{{*/
														i2cReg = pTableHD_EXTEND->addr;
														tblData = pTableHD_EXTEND->pData[resol - pr1000_outresol_1280x720p60c];
														pTableHD_EXTEND++;
														if((i2cReg != 0x46) && (i2cReg != 0x47) && (i2cReg != 0x48) ) continue;

														if(tblData == 0xFF00) continue; //skip
														else if(tblData == 0xFFFF) break; //end
														else
														{
															page = (tblData>>8);
															Dbg("page:%d, reg:%x\n", page COMMA i2cReg);
															if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x46) )
															{
																defChromaPhase |= (uint8_t)tblData << 16;
															}
															else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x47) )
															{
																defChromaPhase |= (uint8_t)tblData << 8;
															}
															else if( (page == PR1000_REG_PAGE_VDEC1) && (i2cReg == 0x48) )
															{
																defChromaPhase |= (uint8_t)tblData;
																break;
															}
														}
													}/*}}}*/
												}
#endif // DONT_SUPPORT_VDRESOL_EXTEND
											}/*}}}*/
											break;
										default: 
											{
												defChromaPhase = 0;
											}
											break;
									}
									Dbg("defChromaPhase:0x%08x\n", defChromaPhase);

								}/*}}}*/

								page = PR1000_VDEC_PAGE(prChn);
								i2cReg = 0x46 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
								if( PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &regs[0]) < 0)
								{
									ErrorString("PageRead.\n");
									break;
								}

								regs[0] &= 0xC0;
								regs[0] |= ((defChromaPhase>>16)&0x1F) | 0x00; //0x00:set auto
								regs[1] = ((defChromaPhase>>8)&0xFF);
								regs[2] = ((defChromaPhase)&0xFF);

								page = PR1000_VDEC_PAGE(prChn);
								i2cReg = 0x46 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
								if( PR1000_PageWriteBurst(fd, i2cSlaveAddr, page, i2cReg, 3, regs) < 0)
								{
									ErrorString("PageReadBurst.\n");
									break;
								}
								Print("mapChn:%d defChromaPhase:0x%08x\n", mapChn COMMA defChromaPhase);

								/* apply loading register. */
								{/*{{{*/
									page = PR1000_VDEC_PAGE(prChn);
									i2cReg = 0x61 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
									i2cData = 0x0E;
									if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
									{
										ErrorString("Write reg.\n");
										break;
									}
									page = PR1000_VDEC_PAGE(prChn);
									i2cReg = 0x61 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
									i2cData = 0x0F;
									if( (PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
									{
										ErrorString("Write reg.\n");
										break;
									}
								}/*}}}*/

								pHost->sysHost.cntChromaLockTunn[mapChn] = 0; //init
								_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock);
								_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
								_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);
								continue;
							}/*}}}*/
						}
					}/*}}}*/
					else
					{
						pHost->sysHost.cntChromaLockTunn[mapChn] = 0; //init
						_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock);
						_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
						_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);
					}
				}/*}}}*/

			}

			/* VadcGainSel check. */
			for(mapChn = 0; mapChn < MAX_CHN; mapChn++)
			{
				if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) continue;

				//check indicate start flag
				if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) == 0)  continue;
				if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnLoadedRegTable) == 0) continue;

				Dbg("mapChn:%d, bit:%d\n", mapChn COMMA (_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel)==0?0:1));

				pstPortChSel = (_stPortChSel *)&pHost->sysHost.portChSel[mapChn];
				prChip = pstPortChSel->prChip;
				prChn = pstPortChSel->prChn;
				i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

				if(pHost->sysHost.cntVadcGainSelTunn[mapChn] > MAX_PR1000_CEQ_TBL_NUM)
				{
					_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel);
					_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
					_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
					_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
					continue;
				}

#ifdef SUPPORT_CABLE_EQ
				/* VadcGainSel check. */
				{/*{{{*/
					pstCEQData = (_stCEQData *)&pHost->sysHost.stCEQDataList[mapChn]; //Max 4chip * 4channel
					format = pstCEQData->format;
					resol = pstCEQData->camResol;

					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0x00 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
					if( PR1000_PageReadBurst(fd, i2cSlaveAddr, page, i2cReg, 2, regs) < 0)
					{
						ErrorString("PageReadBurst.\n");
						break;
					}
					stCeqDet.reg = regs[0];
					stCeqLock.reg = regs[1];
					Dbg("Step:%d reg[%02x] lock[hpll:%d,hper:%d,std:%d]\n", pstCEQData->estStep COMMA
							stCeqLock.reg COMMA
							stCeqLock.b.lock_hpll COMMA
							stCeqLock.b.lock_hperiod COMMA
							stCeqLock.b.lock_std);

					if( (stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 1) ) 
					{
						if( PR1000_CEQ_GetInfoRegs(fd, pstPortChSel, &stCeqInfoReg) >= 0)
						{
							uint8_t bFlagChg = FALSE;
							uint16_t detAcGain = stCeqInfoReg.b.det_eq_acgain_h<<8|stCeqInfoReg.b.det_eq_acgain_l;
							int stepDir = 0;
							if( detAcGain < 0x0700 )
							{
								stepDir = -1; // --
								bFlagChg = TRUE;
							}
							else if( detAcGain >= 0x0900 )
							{
								stepDir = 1; // ++
								bFlagChg = TRUE;
							}
							else
							{
								Print("===>> Lock vadc gain sel mapChn:%d detAcGain:0x%04x, tunn:%d\n", mapChn COMMA detAcGain COMMA pHost->sysHost.cntVadcGainSelTunn[mapChn]);

								_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel);
								_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
								_SET_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
								_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
								continue;
							}
							Print("mapChn:%d detAcGain:0x%04x, dir:%d, flagChg:%d, tunn:%d\n", mapChn COMMA detAcGain COMMA stepDir COMMA bFlagChg COMMA pHost->sysHost.cntVadcGainSelTunn[mapChn]);

							if(bFlagChg)
							{/*{{{*/
								uint16_t u16CurFact[2];

								if(pHost->sysHost.cntVadcGainSelTunn[mapChn] == 0)
								{
									pHost->sysHost.dirTunnStep[mapChn] = stepDir; //+ or -
								}
								else
								{
									Print("mapChn:%d, old:%d, new:%d\n", mapChn COMMA pHost->sysHost.dirTunnStep[mapChn] COMMA stepDir); //+ or -
									if(pHost->sysHost.dirTunnStep[mapChn] != stepDir) //+ or -
									{
										pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0; //init
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel);
										_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
									}
								}

								pHost->sysHost.cntVadcGainSelTunn[mapChn]++;

								u16CurFact[0] = pstCEQData->attenFact; 
								u16CurFact[1] = pstCEQData->compFact; 

								Print("mapChn:%d, curdist:atten[%dm],comp[%dm]\n", mapChn COMMA u16CurFact[0] COMMA u16CurFact[1]);
								/* estResult[0]: 0x12,(From Atten value), estResult[1]: 0x13,(From Comp value) */
								/* estResultFact[0]: (From Atten dist value), estResultFact[1]: (From Comp dist value) */
								if( (ret = PR1000_CEQ_GetEQTunnEstFactor(fd, (enum _pr1000_table_format)format, (enum _pr1000_table_inresol)resol, &stCeqInfoReg, stepDir, u16CurFact, estResult, estResultFact)) >= 0)
								{
									// + Enable EQ. write 0x12~0x13 
									bWrAtten = TRUE; bWrComp = FALSE;
									if(bWrAtten)
									{
										pstCEQData->estResultAtten = estResult[0]&0x1F;	//0x12,(From Atten value)
										pstCEQData->attenFact = estResultFact[0];

										page = PR1000_REG_PAGE_COMMON;
										i2cReg = 0x12 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
										estResult[0] |= 0x20; //Enable EQ
										PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, (uint8_t)estResult[0]);
										Print("Set tunn STD EQ value mapChn:%d. (Atten:%dm,0x%x)\n", mapChn COMMA estResultFact[0] COMMA estResult[0]);
									}
									if(bWrComp)
									{
										pstCEQData->estResultComp = estResult[1]&0x3F;	//0x13,(From Comp value)
										pstCEQData->compFact = estResultFact[1];

										page = PR1000_REG_PAGE_COMMON;
										i2cReg = 0x13 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
										PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, (uint8_t)estResult[1]);
										Print("Set tunn STD EQ value mapChn:%d. (Comp:%dm,0x%x)\n", mapChn COMMA estResultFact[1] COMMA estResult[1]);
									}

									if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel))
									{
										/* do vadc gain. Must do after SetEQGain */
										PR1000_CEQ_SetVADCGain(fd, (enum _pr1000_table_format)format, (enum _pr1000_table_inresol)resol, pstPortChSel, pstCEQData);
									}

									if( ret == 0 ) // case step is Min or Max 
									{
										Print("mapChn:%d, case step is Min or Max\n", mapChn); 
										pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0; //init
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel);
										_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
									}
								}
							}/*}}}*/
						}
					}
					else
					{
						pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0; //init
						_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel);
						_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
						_SET_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
						_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
					}
				}/*}}}*/
#endif // SUPPORT_CABLE_EQ
			}

			
		#if 0 //get direct format detect from driver test
		//////
		{
			_wqPollChnStatus wqPollChnStatus;
			int mapChn = 0;
			_stEventDetStd stEventDetStd;

			memcpy(&wqPollChnStatus, &pHost->wqPollChnStatus, sizeof(_wqPollChnStatus));
			memset(&pHost->wqPollChnStatus, 0, sizeof(_wqPollChnStatus));

			Print(">>>> ch:%d, detstd:%x, novid:%x\n", mapChn COMMA 
					wqPollChnStatus.bitWqPollDetStd COMMA wqPollChnStatus.bitWqPollNovid);
			if( (wqPollChnStatus.bitWqPollDetStd & (1<<mapChn)) && ((wqPollChnStatus.bitWqPollNovid & (1<<mapChn)) == 0) )
			{
				pHost->eventHost.stEventDetStd[mapChn].mapChn = mapChn;
				memcpy(&stEventDetStd, &pHost->eventHost.stEventDetStd[mapChn], sizeof(_stEventDetStd));

				Print("###### TODO Set ch:%d, format:%d, resol:%d\n", mapChn COMMA stEventDetStd.format COMMA stEventDetStd.resol);
				// Set_RX_MODE ---> //
			}
		}
		//////
		#endif

		}/*}}}*/
	}
	DbgString("stop kthread\n");

#endif // __LINUX_SYSTEM__

	return(0);
}

int GetIrqChipState(const int fd, uint8_t *pstIrqChipState)
{
	int ret = 0;

	uint8_t i2cReg = 0, i2cData;
	int page;

	_stIrqChipState stIrqChipState;

	uint8_t i2cSlaveAddr; 

	if(pstIrqChipState == NULL)
	{
		ErrorString("Invalid argu\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[PR1000_MASTER]; //read irq cascade state information
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0x85;
	if( PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &stIrqChipState.reg) < 0)
	{
		ErrorString("PageRead.\n");
		return(-1);
	}

	if( stIrqChipState.b.irqTimer ) //irqTimer
	{
		page = PR1000_REG_PAGE_COMMON;
		i2cReg = 0xBE; //CLR_WAKE
		i2cData = 0x80;
		PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData);
	}

	if( (stIrqChipState.reg & 0xF) == 0)
	{
		return(0); // no irq any chip.
	}
	else
	{
		*pstIrqChipState = stIrqChipState.reg;
		ret = 1;
	}

	return(ret);
}

int PR1000_IRQ_Isr(const int fd, _drvHost *pHost)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int page;

	_stIrqChipState stIrqChipState;
	uint8_t regIRQ_EVENT_STATE;

	_stIRQReg stIRQClr;
	_stIRQReg stIRQStatus;
	uint8_t prChip, prChn, i2cSlaveAddr; 

#ifdef __LINUX_SYSTEM__
	uint64_t  u64CurUsec;
	uint32_t  u32IntMsec = 0;
	struct timeval timeVal;

	pHost->sysHost.u32RxIntCnt++;
	do_gettimeofday(&timeVal);
	u64CurUsec = ((uint64_t)timeVal.tv_sec*1000000LL + (uint64_t)timeVal.tv_usec);
	DO_DIV(u64CurUsec, 1000LL);/* mod = do_div(x,y), x=x/y */
	u32IntMsec = (uint32_t)(u64CurUsec);
	if(pHost->sysHost.u32IntOldMsec==0) pHost->sysHost.u32IntOldMsec = u32IntMsec;
	pHost->sysHost.u32IntIntervalTimeMsec = u32IntMsec - pHost->sysHost.u32IntOldMsec;
	pHost->sysHost.u32IntOldMsec = u32IntMsec;
#endif // __LINUX_SYSTEM__

	DbgString("Isr\n");

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[PR1000_MASTER]; //read irq cascade state information

	if(pHost->sysHost.bitChnWakeIsrImmediately != 0)
	{
		PR1000_IRQ_WAKE_Isr(fd, pHost);
		pHost->sysHost.bitChnWakeIsrImmediately = 0;
		goto PR1000_IRQ_Isr_Error;
	}

	/*  Get interrupt chip. */
	ret = GetCPUExternalIrqChipState((uint8_t *)&stIrqChipState.reg);
	if( ret == 0 ) /* 0:nocpuexternal or cascade, >0:cpuexternal_nocascade */
	{
		DbgString("nocpuexternal or cascade\n");
		if(GetIrqChipState(fd, (uint8_t *)&stIrqChipState.reg) <= 0)
		{
			goto PR1000_IRQ_Isr_Error;
		}
	}
	else if( ret > 0 )
	{
		DbgString("cpuexternal_nocascade\n");
		if(stIrqChipState.b.irqMaster)
		{
			i2cSlaveAddr = PR1000_I2C_SLVADDRS[PR1000_MASTER]; //read irq cascade state information
			page = PR1000_REG_PAGE_COMMON;
			i2cReg = 0x85;
			if( PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData) < 0)
			{
				ErrorString("PageRead.\n");
				return(-1);
			}

			if( i2cData & 0x80 ) //irqTimer
			{
				page = PR1000_REG_PAGE_COMMON;
				i2cReg = 0xBE; //CLR_WAKE
				i2cData = 0x80;
				PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData);
				stIrqChipState.b.irqTimer = 1;
			}
		}
	}
	else if( ret < 0 )
	{
		DbgString(" no interrupt\n");
		goto PR1000_IRQ_Isr_Error;
	}
	Dbg("stIrqChipState.reg:0x%x\n", stIrqChipState.reg);

	for(prChip = PR1000_MASTER; prChip < PR1000_CHIP_COUNT; prChip++)
	{
		if( stIrqChipState.reg & (1<<(3-prChip)) )
		{
			i2cSlaveAddr = PR1000_I2C_SLVADDRS[prChip];
			page = PR1000_REG_PAGE_COMMON;
			i2cReg = 0x86;
			if( PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &regIRQ_EVENT_STATE) < 0)
			{
				ErrorString("PageRead.\n");
				continue;
			}
			Dbg("prChip:%d, regIRQ_EVENT_STATE:0x%x\n", prChip COMMA regIRQ_EVENT_STATE);
			if( (regIRQ_EVENT_STATE & 0xFF) == 0)
			{
				continue;
			}

			/* Read irq clr. Write irq clr except VFD,MD */
			page = PR1000_REG_PAGE_COMMON;
			i2cReg = 0xB0;
			if( PR1000_PageReadBurst(fd, i2cSlaveAddr, page, i2cReg, sizeof(stIRQClr), (uint8_t *)&stIRQClr) < 0)
			{
				ErrorString("PageReadBurst.\n");
				continue;
			}

			i2cData = stIRQClr.u8NOVID; //temporary save.
			stIRQClr.u8NOVID &= 0xF0;	//Don't clear novid. novid is cleard NOVID_Isr()
			if( PR1000_PageWriteBurst(fd, i2cSlaveAddr, page, i2cReg, sizeof(stIRQClr), (uint8_t *)&stIRQClr) < 0)
			{
				ErrorString("PageReadBurst.\n");
				continue;
			}
			stIRQClr.u8NOVID = i2cData; //recover data.
			Dbg("Int>Ptz:%02x/%02x/%02x/%02x,NOV:%02x,MD:%02x,ND:%02x,DFD:%02x,AD:%02x\n", stIRQClr.u8PTZ[0] COMMA stIRQClr.u8PTZ[1] COMMA stIRQClr.u8PTZ[2] COMMA stIRQClr.u8PTZ[3] COMMA stIRQClr.u8NOVID COMMA stIRQClr.u8MD COMMA stIRQClr.u8ND COMMA stIRQClr.u8DFD COMMA stIRQClr.u8AD);

			/* Read irq status. */
			page = PR1000_REG_PAGE_COMMON;
			i2cReg = 0xC0;
			if( PR1000_PageReadBurst(fd, i2cSlaveAddr, page, i2cReg, sizeof(stIRQStatus), (uint8_t *)&stIRQStatus) < 0)
			{
				ErrorString("PageReadBurst.\n");
				continue;
			}

			if(regIRQ_EVENT_STATE & (1<<0)) //PTZ
			{/*{{{*/
				for(prChn = 0; prChn < DEF_PR1000_MAX_CHN; prChn++)
				{
					if(stIRQClr.u8PTZ[prChn] != 0)
					{
						if(PR1000_IRQ_PTZ_Isr(fd, prChip, prChn, stIRQClr.u8PTZ[prChn], pHost) < 0)
						{
							ErrorString("PTZ Isr.\n");
						}
					}
				}
			}/*}}}*/
			if(regIRQ_EVENT_STATE & (1<<1)) //VFD|NOVID
			{/*{{{*/
				Dbg("prChip:%d, VFD|NOVID regIRQ_CLR:0x%x\n", prChip COMMA stIRQClr.u8NOVID);
				if(stIRQClr.u8NOVID != 0)
				{
					if( stIRQClr.u8NOVID & 0x0F) //NOVID
					{
						if(PR1000_IRQ_NOVID_Isr(fd, prChip, stIRQClr.u8NOVID&0x0F, stIRQStatus.u8NOVID&0x0F, pHost) < 0) 
						{
							ErrorString("NOVID Isr.\n");
						}
					}
					if( stIRQClr.u8NOVID & 0xF0) //VFD
					{
						/* check video on & vfd */
						i2cData = ((~stIRQClr.u8NOVID)&(stIRQClr.u8NOVID>>4)) & 0xF;
						if( i2cData != 0)
						{
							if(PR1000_IRQ_VFD_Isr(fd, prChip, i2cData, i2cData, pHost) < 0) 
							{
								ErrorString("VFD Isr.\n");
							}
						}
					}
				}
			}/*}}}*/
#ifndef DONT_SUPPORT_EVENT_FUNC
			if(regIRQ_EVENT_STATE & (1<<2)) //BD|MD
			{/*{{{*/
				Dbg("prChip:%d, BD|MD regIRQ_CLR:0x%x\n", prChip COMMA stIRQClr.u8MD);
				if(stIRQClr.u8MD != 0)
				{
					if( stIRQClr.u8MD & 0x0F) //MD
					{
						if(PR1000_IRQ_MD_Isr(fd, prChip, stIRQClr.u8MD&0x0F, stIRQStatus.u8MD&0x0F, pHost) < 0) 
						{
							ErrorString("MD Isr.\n");
						}
					}
					if( stIRQClr.u8MD & 0xF0) //BD
					{
						if(PR1000_IRQ_BD_Isr(fd, prChip, (stIRQClr.u8MD>>4)&0x0F, (stIRQStatus.u8MD>>4)&0x0F, pHost) < 0) 
						{
							ErrorString("BD Isr.\n");
						}
					}
				}
			}/*}}}*/
			if(regIRQ_EVENT_STATE & (1<<3)) //DD|ND
			{/*{{{*/
				Dbg("prChip:%d, DD|ND regIRQ_CLR:0x%x\n", prChip COMMA stIRQClr.u8ND);
				if(stIRQClr.u8ND != 0)
				{
					if( stIRQClr.u8ND & 0x0F) //ND
					{
						if(PR1000_IRQ_ND_Isr(fd, prChip, stIRQClr.u8ND&0x0F, stIRQStatus.u8ND&0x0F, pHost) < 0) 
						{
							ErrorString("ND Isr.\n");
						}
					}
					if( stIRQClr.u8ND & 0xF0) //DD
					{
						if(PR1000_IRQ_DD_Isr(fd, prChip, (stIRQClr.u8ND>>4)&0x0F, (stIRQStatus.u8ND>>4)&0x0F, pHost) < 0) 
						{
							ErrorString("DD Isr.\n");
						}
					}
				}
			}/*}}}*/
			if(regIRQ_EVENT_STATE & (1<<4)) //NOAUD|DFD
			{/*{{{*/
				Dbg("prChip:%d, NOAUD|DFD regIRQ_CLR:0x%x\n", prChip COMMA stIRQClr.u8DFD);
				if(stIRQClr.u8DFD != 0)
				{
					if( stIRQClr.u8DFD & 0x0F) //DFD
					{
						if(PR1000_IRQ_DFD_Isr(fd, prChip, stIRQClr.u8DFD&0x0F, stIRQStatus.u8DFD&0x0F, pHost) < 0) 
						{
							ErrorString("DFD Isr.\n");
						}
					}
					if( stIRQClr.u8DFD & 0xF0) //NOAUD
					{
						if(PR1000_IRQ_NOAUD_Isr(fd, prChip, (stIRQClr.u8DFD>>4)&0x0F, (stIRQStatus.u8DFD>>4)&0x0F, pHost) < 0) 
						{
							ErrorString("NOAUD Isr.\n");
						}
					}
				}
			}/*}}}*/
			if(regIRQ_EVENT_STATE & (1<<5)) //AD_DIFF|AD_ABS
			{/*{{{*/
				Dbg("prChip:%d, AD_DIFF|AD_ABS regIRQ_CLR:0x%x\n", prChip COMMA stIRQClr.u8AD);
				if(stIRQClr.u8AD != 0)
				{
					if( stIRQClr.u8AD & 0x0F) //AD_ABS
					{
						if(PR1000_IRQ_AD_ABS_Isr(fd, prChip, stIRQClr.u8AD&0x0F, stIRQStatus.u8AD&0x0F, pHost) < 0) 
						{
							ErrorString("AD_ABS Isr.\n");
						}
					}
					if( stIRQClr.u8AD & 0xF0) //AD_DIFF
					{
						if(PR1000_IRQ_AD_DIFF_Isr(fd, prChip, (stIRQClr.u8AD>>4)&0x0F, (stIRQStatus.u8AD>>4)&0x0F, pHost) < 0) 
						{
							ErrorString("AD_DIFF Isr.\n");
						}
					}
				}
			}/*}}}*/
#endif // DONT_SUPPORT_EVENT_FUNC
			if(regIRQ_EVENT_STATE & (1<<6)) //GPIO0
			{/*{{{*/
				Dbg("prChip:%d, GPIO0 regIRQ_CLR:0x%x\n", prChip COMMA stIRQClr.u8GPIO0);
				if(stIRQClr.u8GPIO0 != 0)
				{
					prChn = 0;
					if(PR1000_IRQ_GPIO_Isr(fd, prChip, prChn, stIRQClr.u8GPIO0, pHost) < 0)
					{
						ErrorString("GPIO Isr.\n");
					}
				}
			}/*}}}*/
			if(regIRQ_EVENT_STATE & (1<<7)) //GPIO5_1
			{/*{{{*/
				for(prChn = 0; prChn < 5; prChn++)
				{
					Dbg("prChip:%d, GPIO%d regIRQ_CLR:0x%x\n", prChip COMMA prChn COMMA stIRQClr.u8GPIO1_5[prChn]);
					if(stIRQClr.u8GPIO1_5[prChn] != 0)
					{
						if(PR1000_IRQ_GPIO_Isr(fd, prChip, prChn+1, stIRQClr.u8GPIO1_5[prChn], pHost) < 0)
						{
							ErrorString("GPIO Isr.\n");
						}
					}
				}
			}/*}}}*/
		}
	}

	//IRQ_WAKE
	Dbg("0x%02x(%d,%d)\n", stIrqChipState.reg COMMA stIrqChipState.b.irqTimer COMMA stIrqChipState.b.irqMaster);
	if(stIrqChipState.b.irqTimer)
	{
		DbgString("WAKE_Isr\n");
		PR1000_IRQ_WAKE_Isr(fd, pHost);

		/* wake up poll if set */
		if( (pHost->wqPollChnStatus.bitWqPollDetStd != 0) || (pHost->wqPollChnStatus.bitWqPollNovid != 0) ||
				(pHost->wqPollChnStatus.bitWqPollVfd != 0) || (pHost->wqPollChnStatus.bitWqPollMd != 0) ||
				(pHost->wqPollChnStatus.bitWqPollBd != 0) || (pHost->wqPollChnStatus.bitWqPollNd != 0) ||
				(pHost->wqPollChnStatus.bitWqPollDd != 0) || (pHost->wqPollChnStatus.bitWqPollDfd != 0) ||
				(pHost->wqPollChnStatus.bitWqPollAdMute != 0) || (pHost->wqPollChnStatus.bitWqPollAdAbs != 0) ||
				(pHost->wqPollChnStatus.bitWqPollAdDiff != 0) || (pHost->wqPollChnStatus.bitWqPollGpio != 0) ||
				(pHost->wqPollChnStatus.bitWqPollPtzRx != 0) )
		{
			Dbg("Wake up(DetStd:%lx, Novid:%lx, Vfd:%lx, Md:%lx, Bd:%lx, Nd:%lx, Dd:%lx, Dfd:%lx, AdMute:%lx, AdAbs:%lx, AdDiff:%lx, Gpio:%lx, PtzRx;%lx)\n",
					pHost->wqPollChnStatus.bitWqPollDetStd COMMA pHost->wqPollChnStatus.bitWqPollNovid COMMA
					pHost->wqPollChnStatus.bitWqPollVfd COMMA pHost->wqPollChnStatus.bitWqPollMd COMMA
					pHost->wqPollChnStatus.bitWqPollBd COMMA pHost->wqPollChnStatus.bitWqPollNd COMMA
					pHost->wqPollChnStatus.bitWqPollDd COMMA pHost->wqPollChnStatus.bitWqPollDfd COMMA
					pHost->wqPollChnStatus.bitWqPollAdMute COMMA pHost->wqPollChnStatus.bitWqPollAdAbs COMMA
					pHost->wqPollChnStatus.bitWqPollAdDiff COMMA pHost->wqPollChnStatus.bitWqPollGpio COMMA
					pHost->wqPollChnStatus.bitWqPollPtzRx);
			WAKE_UP_INTERRUPTIBLE(&pHost->wqPoll);
		}
	}


PR1000_IRQ_Isr_Error:
	return(ret);
}

int PR1000_IRQ_PTZ_Isr(const int fd, const uint8_t prChip, const uint8_t prChn, const uint8_t intReg, _drvHost *pHost)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask;

	uint8_t rxFifoSize = 0;
	uint8_t rxFifoData;

	int i;
	uint8_t pTempRecvBuffer[MAX_PR1000_PTZ_SIZE];
        FIFO *pPtzRcvQ = NULL;
        SPINLOCK_T *pLockPtzRcvQ = NULL;
        char *pPtzRcvData = NULL;
	int mapChn; uint8_t i2cSlaveAddr; 
	_stPortChSel *pstPortChSel = GetMapChInfo(prChip, prChn, pHost);
	_stCEQData *pstCEQData = NULL;

	if(pstPortChSel == NULL)
	{
		Error("Can't get port ch(%d) sel info.\n", prChn);
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	pstCEQData = (_stCEQData *)&pHost->sysHost.stCEQDataList[mapChn];
        pPtzRcvQ = (FIFO *)&pHost->ptzHost.ptzRecvQ[mapChn];
        pLockPtzRcvQ = (SPINLOCK_T *)&pHost->ptzHost.splockPtzRecvQ[mapChn];
	Dbg("Int> PTZ Isr(mapChn:%d) intReg:0x%x\n", mapChn COMMA intReg);

	if(intReg == 0)
	{
		return(0);
	}

	/* Check interrupt type */
	/* TX */
	if(intReg & 0x01) // Ptz Tx 
	{
		DbgString("Ptz Tx Done.\n");
		_SET_BIT(0, &pHost->ptzHost.bitPtzIsrStatus[mapChn]);
		ret = 0;
	}
	if(intReg & 0x02)
	{
		DbgString("Ptz Tx fifo empty. Init Tx Path & Fifo.\n");
		_SET_BIT(1, &pHost->ptzHost.bitPtzIsrStatus[mapChn]);

		/* reset tx fifo */
		/* Path Disable->Enable->WR_INIT(0xA0)(txdata only) */
		PR1000_PTZ_EnableTXPath(fd, i2cSlaveAddr, prChn, DISABLE);
		PR1000_PTZ_InitTXFifo(fd, i2cSlaveAddr, prChn);
		PR1000_PTZ_EnableTXPath(fd, i2cSlaveAddr, prChn, ENABLE);
		ret = 0;
	}
	if(intReg & 0x04)
	{
		DbgString("Ptz Tx fifo overflow. Init Tx Path & Fifo.\n");
		_SET_BIT(2, &pHost->ptzHost.bitPtzIsrStatus[mapChn]);

		/* reset tx fifo */
		/* Path Disable->Enable->WR_INIT(0xA0)(txdata only) */
		PR1000_PTZ_EnableTXPath(fd, i2cSlaveAddr, prChn, DISABLE);
		PR1000_PTZ_InitTXFifo(fd, i2cSlaveAddr, prChn);
		PR1000_PTZ_EnableTXPath(fd, i2cSlaveAddr, prChn, ENABLE);
	}
	//////////////////////////////////////////////////////////////////////////////////
	
	/* RX */
	if(intReg & 0x10)
	{
		DbgString("Ptz Rx fifo empty. Init Rx Path & Fifo.\n");
		_SET_BIT(4, &pHost->ptzHost.bitPtzIsrStatus[mapChn]);

		/* reset rx fifo */
		/* Path Disable->Enable->RD_INIT(0xA4)(rxdata only) */
		PR1000_PTZ_EnableRXPath(fd, i2cSlaveAddr, prChn, DISABLE);
		PR1000_PTZ_InitRXFifo(fd, i2cSlaveAddr, prChn);
		PR1000_PTZ_EnableRXPath(fd, i2cSlaveAddr, prChn, ENABLE);
	}
	if(intReg & 0x20)
	{
		DbgString("Ptz Rx fifo overflow. Init Rx Path & Fifo.\n");
		_SET_BIT(5, &pHost->ptzHost.bitPtzIsrStatus[mapChn]);

		/* reset rx fifo */
		/* Path Disable->Enable->RD_INIT(0xA4)(rxdata only) */
		PR1000_PTZ_EnableRXPath(fd, i2cSlaveAddr, prChn, DISABLE);
		PR1000_PTZ_InitRXFifo(fd, i2cSlaveAddr, prChn);
		PR1000_PTZ_EnableRXPath(fd, i2cSlaveAddr, prChn, ENABLE);
	}

	if( (intReg & 0x40) || (intReg & 0x80) )
	{
		if(intReg & 0x40)
		{
			DbgString("Ptz Rx sync.\n");
			_SET_BIT(6, &pHost->ptzHost.bitPtzIsrStatus[mapChn]);
		}
		if(intReg & 0x80)
		{
			DbgString("Ptz Rx data error. Init Rx Path & Fifo. Restart rx.\n");
			_SET_BIT(7, &pHost->ptzHost.bitPtzIsrStatus[mapChn]);

			/* path disable->enable, fifo clear, rx start. */
			PR1000_PTZ_StartRX(fd, &pHost->sysHost.portChSel[mapChn], STOP);
			PR1000_PTZ_EnableRXPath(fd, i2cSlaveAddr, prChn, DISABLE);
			PR1000_PTZ_EnableRXPath(fd, i2cSlaveAddr, prChn, ENABLE);
			PR1000_PTZ_InitRXFifo(fd, i2cSlaveAddr, prChn);
			PR1000_PTZ_StartRX(fd, &pHost->sysHost.portChSel[mapChn], START);
		}
	}
	else if(intReg & 0x08) // Ptz Rx
	{
		DbgString("Ptz Rx.\n");
		_SET_BIT(3, &pHost->ptzHost.bitPtzIsrStatus[mapChn]);

		/* Rx Sub */ 
		if( PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, PR1000_REG_ADDR_PTZ_FIFO_RD_SIZE + PR1000_OFFSETADDR_PTZ_CH(prChn), &rxFifoSize) < 0)
		{
			ErrorString("PageRead.\n");
			return(-1);
		}

		Dbg("Rx data Fifo:%d \n", rxFifoSize);
		if(rxFifoSize >= MAX_PR1000_PTZ_SIZE)
		{
			Dbg("Warning! Over Rx Size(%d/%d). Init RxFifo.\n", rxFifoSize COMMA MAX_PR1000_PTZ_SIZE);
			_SET_BIT(8, &pHost->ptzHost.bitPtzIsrStatus[mapChn]);
		}
		if(rxFifoSize > 0)
		{
			/* ready read fifo */
			i2cMask = 0x17; i2cData = 0x13; // 1'st set pre-loaddata bit before read fifo.
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_DATA + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if(pHost->sysHost.cntSTDFORMATCheck[mapChn] > 0) //check STDFORMAT check 
			{
				if(rxFifoSize > 10) rxFifoSize = 10;
				if( PR1000_PageReadBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, rxFifoSize, pTempRecvBuffer) < 0)
				{
					ErrorString("PageReadBurst.\n");
					return(-1);
				}
			}
			else
			{
				if(rxFifoSize >= 200)
				{
					rxFifoSize = MAX_PR1000_PTZ_SIZE;
					if( PR1000_PageReadBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, rxFifoSize, pTempRecvBuffer) < 0)
					{
						ErrorString("PageReadBurst.\n");
						return(-1);
					}

					/* clear rx fifo */
					PR1000_PTZ_EnableRXPath(fd, i2cSlaveAddr, prChn, DISABLE);
					PR1000_PTZ_InitRXFifo(fd, i2cSlaveAddr, prChn);
					PR1000_PTZ_EnableRXPath(fd, i2cSlaveAddr, prChn, ENABLE);
				}
				else
				{
					if( PR1000_PageReadBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, rxFifoSize, pTempRecvBuffer) < 0)
					{
						ErrorString("PageReadBurst.\n");
						return(-1);
					}
				}
			}

#ifdef DEBUG_PTZ_PR1000
			{
				uint8_t u8Data = 0;
#ifdef __LINUX_SYSTEM__
				printk("Recv %02d: ", rxFifoSize);
#else
				printf("Recv %02d: ", rxFifoSize);
#endif
				for(i = 0; i < rxFifoSize; i++)
				{
#ifdef __LINUX_SYSTEM__
					printk("0x%02x ", pTempRecvBuffer[i]);
#else
					printf("0x%02x ", pTempRecvBuffer[i]);
#endif
				}
				if( PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, PR1000_REG_ADDR_PTZ_FIFO_RD_SIZE + PR1000_OFFSETADDR_PTZ_CH(prChn), &u8Data) < 0)
				{
					ErrorString("PageRead.\n");
					return(-1);
				}
#ifdef __LINUX_SYSTEM__
				printk("(remain rfifo:%d).\n", u8Data);
#else
				printf("(remain rfifo:%d).\n\r", u8Data);
#endif

			}
			Dbg("mapCh:%d timeOver:%d, cntCheck:%d\n", mapChn COMMA pHost->sysHost.timeOverSTDFORMATCheck[mapChn] COMMA pHost->sysHost.cntSTDFORMATCheck[mapChn]);
#endif
	
			if(pHost->sysHost.cntSTDFORMATCheck[mapChn] > 0) //check STDFORMAT check 
			{/*{{{*/
				pHost->sysHost.cntSTDFORMATCheck[mapChn]--;

				if(pstCEQData->format == pr1000_format_HDT) /* case HDT/HDT_NEW format */
				{/*{{{*/
					if( (pstCEQData->camResol == pr1000_inresol_1280x720p25) || (pstCEQData->camResol == pr1000_inresol_1280x720p30) )
					{/*{{{*/
						for(i = 0, rxFifoData = 0; i < rxFifoSize; i++)
						{
							rxFifoData |= pTempRecvBuffer[i];
						}

						if( (rxFifoData == 0x00) || (rxFifoData == 0x80) ) // old format
						{
							// Old HDT
							pHost->ptzHost.testSTDFORMATData[mapChn][pHost->sysHost.cntSTDFORMATCheck[mapChn]] = FALSE;
						}
						else
						{
							// New HDT
							pHost->ptzHost.testSTDFORMATData[mapChn][pHost->sysHost.cntSTDFORMATCheck[mapChn]] = TRUE;
						}

						if(pHost->sysHost.cntSTDFORMATCheck[mapChn] <= 0)
						{
							/* check result is stable ? */
							for(i = 0, ret = TRUE; i < PTZ_STDFORMAT_CHECK_CNT; i++)
							{
								if( pHost->ptzHost.testSTDFORMATData[mapChn][0] != pHost->ptzHost.testSTDFORMATData[mapChn][i])
								{
									Error("Unstable ptz STDFORMAT. restart mapChn:%d\n", mapChn);
									pHost->sysHost.cntSTDFORMATCheck[mapChn] = PTZ_STDFORMAT_CHECK_CNT; //refresh.
									pHost->sysHost.timeOverSTDFORMATCheck[mapChn] = PTZ_STDFORMAT_CHECK_CNT; //refresh.
									ret = FALSE;
									break;
								}
							}

							if(ret) //stable case.
							{
								if( pHost->ptzHost.testSTDFORMATData[mapChn][0] == FALSE )
								{
									// Old HDT
									pHost->ptzHost.flagSTDFORMATResult[mapChn] = FALSE;
								}
								else
								{
									// New HDT
									pHost->ptzHost.flagSTDFORMATResult[mapChn] = TRUE;

								}
#ifdef DEBUG_PTZ_PR1000
								Print("mapChn:%d checking STDFORMAT(cnt:%d, result:%d(0[old]:ptz all zero, 1[new]:not zero))\n", mapChn COMMA pHost->sysHost.cntSTDFORMATCheck[mapChn] COMMA pHost->ptzHost.flagSTDFORMATResult[mapChn]);
#endif
							}
						}
					}/*}}}*/
				}/*}}}*/
				else if(pstCEQData->format == pr1000_format_HDA) /* case HDA format */
				{/*{{{*/
						int j;
						for(i = 0, rxFifoData = 0; i < rxFifoSize; i++)
						{
							for(j = 6; j > 0; j--)
							{
								if(pTempRecvBuffer[i] & (1<<j))
								{
									rxFifoData++;
								}
							}
						}

						if( (rxFifoData >= 3) )
						{
							// real HDA format
							pHost->ptzHost.testSTDFORMATData[mapChn][pHost->sysHost.cntSTDFORMATCheck[mapChn]] = TRUE;
						}
						else
						{
							// noEQ CVI format
							pHost->ptzHost.testSTDFORMATData[mapChn][pHost->sysHost.cntSTDFORMATCheck[mapChn]] = FALSE;
						}

						if(pHost->sysHost.cntSTDFORMATCheck[mapChn] <= 0)
						{
							/* check result is stable ? */
							for(i = 0, ret = TRUE; i < PTZ_STDFORMAT_CHECK_CNT; i++)
							{
								if( pHost->ptzHost.testSTDFORMATData[mapChn][0] != pHost->ptzHost.testSTDFORMATData[mapChn][i])
								{
									Error("Unstable ptz STDFORMAT. restart mapChn:%d\n", mapChn);
									pHost->sysHost.cntSTDFORMATCheck[mapChn] = PTZ_STDFORMAT_CHECK_CNT; //refresh.
									pHost->sysHost.timeOverSTDFORMATCheck[mapChn] = PTZ_STDFORMAT_CHECK_CNT; //refresh.
									ret = FALSE;
									break;
								}
							}

							if(ret) //stable case.
							{
								if( pHost->ptzHost.testSTDFORMATData[mapChn][0] == FALSE )
								{
									// noEQ CVI format
									pHost->ptzHost.flagSTDFORMATResult[mapChn] = FALSE;
								}
								else
								{
									// real HDA format
									pHost->ptzHost.flagSTDFORMATResult[mapChn] = TRUE;

								}
#ifdef DEBUG_PTZ_PR1000
								Print("mapChn:%d checking STDFORMAT(cnt:%d, result:%d(0[noEQ CVI]:ptz all zero, 1[readHDA]:not zero))\n", mapChn COMMA pHost->sysHost.cntSTDFORMATCheck[mapChn] COMMA pHost->ptzHost.flagSTDFORMATResult[mapChn]);
#endif
							}
						}
				}/*}}}*/

				/* clear rx fifo */
				PR1000_PTZ_EnableRXPath(fd, i2cSlaveAddr, prChn, DISABLE);
				PR1000_PTZ_InitRXFifo(fd, i2cSlaveAddr, prChn);
				PR1000_PTZ_EnableRXPath(fd, i2cSlaveAddr, prChn, ENABLE);

				if(pHost->sysHost.cntSTDFORMATCheck[mapChn] <= 0) //stop stdformat check.
				{
					PR1000_PTZ_StartRX(fd, &pHost->sysHost.portChSel[mapChn], FALSE);
				}
			}/*}}}*/
			else  // normal ptz receive data.
			{/*{{{*/
				if(pHost->ptzHost.bRxUsedSTDFORMATOnly[mapChn] == TRUE) //TRUE: PTZ Rx used check STDFORMAT only.
				{
					if( (pstCEQData->format == pr1000_format_HDT) &&
							((pstCEQData->camResol == pr1000_inresol_1280x720p25) || (pstCEQData->camResol == pr1000_inresol_1280x720p30)))
					{
						return(0);
					}
				}

				DbgString("put recv queue.\n");
				for(i = 0; i < rxFifoSize; i++)
				{
					if(!PR1000_CQ_is_full(pPtzRcvQ))
					{
#ifdef __LINUX_SYSTEM__
						/*** put stream dma info to CQ & wakeup dma thread ***/
						/* in isp, use GFP_ATOMIC */
						if( (pPtzRcvData = (char *)ZALLOC(sizeof(char), GFP_ATOMIC)) == NULL)
						{
							ErrorString("failed allocate ptzRecvData\n");
							break;
						}
						*pPtzRcvData = pTempRecvBuffer[i];

						/* put ptzRecvQ */
						DbgString("put ptzRecvQ\n");
						if( (ret = PR1000_CQ_put_locked(pPtzRcvQ, pPtzRcvData, pLockPtzRcvQ)) != sizeof(pPtzRcvData) )
						{
							ErrorString("pr1000_CQ_put\n");
							break;
						}
#else // __LINUX_SYSTEM__
						/*** put stream dma info to CQ & wakeup dma thread ***/

						/* put ptzRecvQ */
						DbgString("put ptzRecvQ\n");
						if( (ret = PR1000_CQ_put_locked(pPtzRcvQ, &pTempRecvBuffer[i], pLockPtzRcvQ)) != sizeof(pPtzRcvData) )
						{
							ErrorString("pr1000_CQ_put\n");
							break;
						}

#endif // __LINUX_SYSTEM__
					}
				}


#ifndef DONT_SUPPORT_PTZ_FUNC
#ifndef DONT_SUPPORT_PTZ_ETC_CMD
				/* find matching ptz command */
				if( (ret = PR1000_PTZ_ResolveRecvData(fd, pHost->sysHost.gPR1000RxType[mapChn], pHost->sysHost.gPR1000RxResol[mapChn], pTempRecvBuffer, rxFifoSize)) < 0)
				{
					ErrorString("Resolve receive ptz data.\n");
				}
				else
				{
					pHost->ptzHost.u32PtzRxCnt[mapChn]++;
				}
				_SET_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollPtzRx);
				WAKE_UP_INTERRUPTIBLE(&pHost->wqPoll);
				Dbg("mapChn:%d pollPtzRx:%lx\n", mapChn COMMA pHost->wqPollChnStatus.bitWqPollPtzRx);
#endif // DONT_SUPPORT_PTZ_ETC_CMD
#endif // DONT_SUPPORT_PTZ_FUNC
			}/*}}}*/

			ret = 0;
		}
		else
		{
			Dbg("rxFifoSize(%d).\n", rxFifoSize);
			ret = 0;
		}
	}
	////////////////////////////////////////////////////////////////////////////////////


	return(ret);
}

int PR1000_IRQ_NOVID_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask;
	int page;
	uint8_t bNovid;
	int mapChn; uint8_t prChn, i2cSlaveAddr; 
	_stPortChSel *pstPortChSel = NULL;

	Dbg("Int> NOVID Isr prChip(%d) intReg:0x%x, intStatus:0x%x\n", prChip COMMA intReg COMMA intStatus);

	if(intReg == 0)
	{
		return(ret);
	}

	if( prChip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid prChip.\n");
		return(-1);
	}

	for(prChn = 0; prChn < DEF_PR1000_MAX_CHN; prChn++)/* only support ch0, ch1, ch2, ch3 */
	{
		if(intReg & (1<<prChn))
		{
			Dbg("Int prChn:%d.\n", prChn);

			pstPortChSel = GetMapChInfo(prChip, prChn, pHost);

			if(pstPortChSel == NULL)
			{
				Dbg("Can't get port ch(%d) sel info.\n", prChn);
				continue;
			}

			mapChn = pstPortChSel->chn;
			i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

			page = PR1000_REG_PAGE_COMMON;
			i2cReg = 0x90;
			if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
			{     
				ErrorString("Read reg.\n");
			}             
			Dbg("Rd [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);

			bNovid = (i2cData>>prChn) & 1;

			if(bNovid == WAIT_NEXT_NOVIDEO_TRUE)
			{
				Print("CEQ Stop by Novid. Int prChip:%d, mapChn:%d.\n", prChip COMMA mapChn);
				PR1000_CEQ_Stop(fd, mapChn, (void *)pHost);
				memset(pHost->ptzHost.testSTDFORMATData[mapChn], 0, sizeof(uint8_t)*PTZ_STDFORMAT_CHECK_CNT);
				pHost->ptzHost.flagSTDFORMATResult[mapChn] = 0;

				pHost->sysHost.cntChromaLockTunn[mapChn] = 0;
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnLoadedRegTable);
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock);
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);

				pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0;
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel);
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);

				//clear event status bits.
				_CLEAR_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollDetStd); 
				//_CLEAR_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollNovid);
				_CLEAR_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollVfd);
				_CLEAR_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollMd);
				_CLEAR_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollBd);
				_CLEAR_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollNd);
				_CLEAR_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollDd);
				_CLEAR_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollDfd);
				_CLEAR_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollAdMute);
				_CLEAR_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollAdAbs);
				_CLEAR_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollAdDiff);
				_CLEAR_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollGpio);
				_CLEAR_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollPtzRx);

				_SET_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollNovid);
				WAKE_UP_INTERRUPTIBLE(&pHost->wqPoll);
				Dbg("mapChn:%d pollNovid:%lx\n", mapChn COMMA pHost->wqPollChnStatus.bitWqPollNovid);

				/* Stop GPIO0 for EQ interrupt */
				{/*{{{*/
					uint8_t i2cReg = 0;
					uint8_t i2cData = 0;
					uint8_t i2cMask = 0;

					Dbg("Stop GPIO0 for EQ interrupt prChn:%d\n", prChn);

					/* set IRQ_GPIO_EN */
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0xA9;
					i2cMask = (1<<prChn); i2cData = (0<<prChn); //disable interrupt.
					Dbg("Write [p%d, 0x%02x-0x%02x(mask:%02x)]\n", page COMMA i2cReg COMMA i2cData COMMA i2cMask);
					if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
					}

					/* set next novideo interrupt level */
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0x90;
					i2cMask = 0x01<<prChn; 
					i2cData = (WAIT_NEXT_NOVIDEO_FALSE)<<prChn; //next interrupt video.
					Dbg("Novideo Level prChn:%d change(0x%x)\n", prChn COMMA i2cMask & i2cData);
					Dbg("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
					if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{     
						ErrorString("Write reg.\n");
					}             
				}/*}}}*/
			}
			else
			{
				if(pHost->sysHost.stCEQDataList[mapChn].estStep <= PR1000_CEQ_STEP_STDCHECK)
				{
					_SET_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollNovid);
					WAKE_UP_INTERRUPTIBLE(&pHost->wqPoll);
					Dbg("mapChn:%d pollNovid:%lx\n", mapChn COMMA pHost->wqPollChnStatus.bitWqPollNovid);
				}

				/* stop VFD interrupt */
				page = PR1000_REG_PAGE_COMMON;
				i2cReg = 0xA4;
				i2cMask = 0x10<<prChn; 
				i2cData = (0)<<prChn; 
				Dbg("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
				if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
				{     
					ErrorString("Write reg.\n");
				}             

				/* set next interrupt level */
				page = PR1000_REG_PAGE_COMMON;
				i2cReg = 0x90;
				i2cMask = 0x01<<prChn; 
				i2cData = (WAIT_NEXT_NOVIDEO_TRUE)<<prChn; //next interrupt video.
				Dbg("Novideo Level prChn:%d change(0x%x)\n", prChn COMMA i2cMask & i2cData);
				Dbg("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
				if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
				{     
					ErrorString("Write reg.\n");
				}             

				/* Start GPIO0 for EQ interrupt */
				{/*{{{*/
					uint8_t i2cReg = 0;
					uint8_t i2cData = 0;
					uint8_t i2cMask = 0;

					Dbg("Start GPIO0 for EQ interrupt prChn:%d\n", prChn);

					/* set IRQ_GPIO_MD */
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0x89;
					i2cMask = (1<<prChn); i2cData = (1<<prChn); //level interrupt.
					Dbg("Write [p%d, 0x%02x-0x%02x(mask:%02x)]\n", page COMMA i2cReg COMMA i2cData COMMA i2cMask);
					if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
					}

					/* set IRQ_GPIO_LV */
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0x91;
					i2cMask = (1<<prChn); i2cData = (0<<prChn); //low level interrupt.
					Dbg("Write [p%d, 0x%02x-0x%02x(mask:%02x)]\n", page COMMA i2cReg COMMA i2cData COMMA i2cMask);
					if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
					}

					/* set IRQ_GPIO_BOTH */
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0x99;
					i2cMask = (1<<prChn); i2cData = (0<<prChn); //interrupt by GPIO_LV.
					Dbg("Write [p%d, 0x%02x-0x%02x(mask:%02x)]\n", page COMMA i2cReg COMMA i2cData COMMA i2cMask);
					if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
					}

					/* set IRQ_GPIO_IOB */
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0xD0;
					i2cMask = (1<<prChn); i2cData = (0<<prChn); //output mode.
					Dbg("Write [p%d, 0x%02x-0x%02x(mask:%02x)]\n", page COMMA i2cReg COMMA i2cData COMMA i2cMask);
					if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
					}

					/* set IRQ_MPP_MD */
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0xD8;
					i2cMask = (1<<prChn); i2cData = (1<<prChn); //GPIO output.
					Dbg("Write [p%d, 0x%02x-0x%02x(mask:%02x)]\n", page COMMA i2cReg COMMA i2cData COMMA i2cMask);
					if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
					}

					/* set IRQ_GPIO_EN */
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0xA9;
					i2cMask = (1<<prChn); i2cData = (1<<prChn); //enable interrupt.
					Dbg("Write [p%d, 0x%02x-0x%02x(mask:%02x)]\n", page COMMA i2cReg COMMA i2cData COMMA i2cMask);
					if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
					}

					/* Set GPIO_OUT0 */
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0xE0;
					i2cMask = (1<<prChn); i2cData = (0<<prChn); //0:interrupt, 1:nointerrupt.
					Dbg("Write [p%d, 0x%02x-0x%02x(mask:%02x)]\n", page COMMA i2cReg COMMA i2cData COMMA i2cMask);
					if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
					}
				}/*}}}*/
			}

			/* clear interrupt pending. */
			page = PR1000_REG_PAGE_COMMON;
			i2cReg = 0xB4;
			i2cMask = 0x01<<prChn; i2cData = 1<<prChn;
			Dbg("Wr [p%d, 0x%02x-0x%02x]\n", page COMMA i2cReg COMMA i2cData);
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
			}
		}
	}
	ret = intReg;

	return(ret);
}

int PR1000_IRQ_VFD_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost)
{
	int ret = -1;
	uint8_t bVfd;
	int mapChn; uint8_t prChn, i2cSlaveAddr; 
	_stPortChSel *pstPortChSel = NULL;

	Dbg("Int> VFD Isr prChip(%d) intReg:0x%x, intStatus:0x%x\n", prChip COMMA intReg COMMA intStatus);

	if(intReg == 0)
	{
		return(ret);
	}

	if( prChip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid prChip.\n");
		return(-1);
	}

	for(prChn = 0; prChn < DEF_PR1000_MAX_CHN; prChn++)/* only support ch0, ch1, ch2, ch3 */
	{
		if(intReg & (1<<prChn))
		{
			Dbg("Int prChn:%d.\n", prChn);

			pstPortChSel = GetMapChInfo(prChip, prChn, pHost);

			if(pstPortChSel == NULL)
			{
				Dbg("Can't get port ch(%d) sel info.\n", prChn);
				continue;
			}

			mapChn = pstPortChSel->chn;
			i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

			bVfd = (intStatus>>prChn) & 1;

			if(bVfd)
			{
				/* Restart from EQ to STD lock. Like to novideo->video sequence. */
				Print("CEQ Stop by Vfd. Int mapChn:%d.\n", mapChn);
				PR1000_CEQ_Stop(fd, mapChn, (void *)pHost);
				memset(pHost->ptzHost.testSTDFORMATData[mapChn], 0, sizeof(uint8_t)*PTZ_STDFORMAT_CHECK_CNT);
				pHost->ptzHost.flagSTDFORMATResult[mapChn] = 0;

				pHost->sysHost.cntChromaLockTunn[mapChn] = 0;
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnLoadedRegTable);
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock);
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);

				pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0;
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel);
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
				_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
			}
		}
	}
	ret = intReg;

	return(ret);
}

#ifndef DONT_SUPPORT_EVENT_FUNC
int PR1000_IRQ_MD_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost)
{
	int ret = -1;
	int mapChn; uint8_t prChn, i2cSlaveAddr; 
	_stPortChSel *pstPortChSel = NULL;

	Dbg("Int> MD Isr prChip(%d) intReg:0x%x, intStatus:0x%x\n", prChip COMMA intReg COMMA intStatus);

	if(intReg == 0)
	{
		return(ret);
	}

	if( prChip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid prChip.\n");
		return(-1);
	}

	for(prChn = 0; prChn < DEF_PR1000_MAX_CHN; prChn++)/* only support ch0, ch1, ch2, ch3 */
	{
		if(intReg & (1<<prChn))
		{
			Dbg("Int prChn:%d.\n", prChn);

			pstPortChSel = GetMapChInfo(prChip, prChn, pHost);

			if(pstPortChSel == NULL)
			{
				Dbg("Can't get port ch(%d) sel info.\n", prChn);
				continue;
			}

			mapChn = pstPortChSel->chn;
			i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

			/* Get det buffer and print. If don't use, comment. */
			#if 0 //debug
			{
				uint64_t detBuf[PR1000_MAX_DET_CELL_Y_NUM] = {0, };
				uint8_t resol = pHost->sysHost.gPR1000RxResol[mapChn];

				/* GetDetBuf */
				if( (ret = PR1000_VEVENT_GetDetData(fd, pstPortChSel, 0, PR1000_MAX_DET_CELL_Y_NUM, detBuf)) < 0)
				{
					ErrorString("GetDetData.\n");
					return(-1);
				}
				#if 0 //debug
				PR1000_VEVENT_PrintDetData(fd, resol, (uint64_t *)detBuf, PR1000_MAX_DET_CELL_Y_NUM);
				#endif
			}
			#endif
			ret = intReg;

			_SET_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollMd);
			WAKE_UP_INTERRUPTIBLE(&pHost->wqPoll);
			Dbg("mapChn:%d pollMd:%lx\n", mapChn COMMA pHost->wqPollChnStatus.bitWqPollMd);
		}
	}


	return(ret);
}

int PR1000_IRQ_BD_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost)
{
	int ret = -1;
	int mapChn; uint8_t prChn, i2cSlaveAddr; 
	_stPortChSel *pstPortChSel = NULL;

	Dbg("Int> BD Isr prChip(%d) intReg:0x%x, intStatus:0x%x\n", prChip COMMA intReg COMMA intStatus);

	if(intReg == 0)
	{
		return(ret);
	}

	if( prChip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid prChip.\n");
		return(-1);
	}

	for(prChn = 0; prChn < DEF_PR1000_MAX_CHN; prChn++)/* only support ch0, ch1, ch2, ch3 */
	{
		if(intReg & (1<<prChn))
		{
			Dbg("Int prChn:%d.\n", prChn);

			pstPortChSel = GetMapChInfo(prChip, prChn, pHost);

			if(pstPortChSel == NULL)
			{
				Dbg("Can't get port ch(%d) sel info.\n", prChn);
				continue;
			}

			mapChn = pstPortChSel->chn;
			i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

			_SET_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollBd);
			WAKE_UP_INTERRUPTIBLE(&pHost->wqPoll);
			Dbg("mapChn:%d pollBd:%lx\n", mapChn COMMA pHost->wqPollChnStatus.bitWqPollBd);
		}
	}
	ret = intReg;

	return(ret);
}

int PR1000_IRQ_ND_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost)
{
	int ret = -1;
	int mapChn; uint8_t prChn, i2cSlaveAddr; 
	_stPortChSel *pstPortChSel = NULL;

	Dbg("Int> ND Isr prChip(%d) intReg:0x%x, intStatus:0x%x\n", prChip COMMA intReg COMMA intStatus);

	if(intReg == 0)
	{
		return(ret);
	}

	if( prChip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid prChip.\n");
		return(-1);
	}

	for(prChn = 0; prChn < DEF_PR1000_MAX_CHN; prChn++)/* only support ch0, ch1, ch2, ch3 */
	{
		if(intReg & (1<<prChn))
		{
			Dbg("Int prChn:%d.\n", prChn);

			pstPortChSel = GetMapChInfo(prChip, prChn, pHost);

			if(pstPortChSel == NULL)
			{
				Dbg("Can't get port ch(%d) sel info.\n", prChn);
				continue;
			}

			mapChn = pstPortChSel->chn;
			i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

			_SET_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollNd);
			WAKE_UP_INTERRUPTIBLE(&pHost->wqPoll);
			Dbg("mapChn:%d pollNd:%lx\n", mapChn COMMA pHost->wqPollChnStatus.bitWqPollNd);
		}
	}
	ret = intReg;

	return(ret);
}

int PR1000_IRQ_DD_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost)
{
	int ret = -1;
	int mapChn; uint8_t prChn, i2cSlaveAddr; 
	_stPortChSel *pstPortChSel = NULL;

	Dbg("Int> DD Isr prChip(%d) intReg:0x%x, intStatus:0x%x\n", prChip COMMA intReg COMMA intStatus);

	if(intReg == 0)
	{
		return(ret);
	}

	if( prChip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid prChip.\n");
		return(-1);
	}

	for(prChn = 0; prChn < DEF_PR1000_MAX_CHN; prChn++)/* only support ch0, ch1, ch2, ch3 */
	{
		if(intReg & (1<<prChn))
		{
			Dbg("Int prChn:%d.\n", prChn);

			pstPortChSel = GetMapChInfo(prChip, prChn, pHost);

			if(pstPortChSel == NULL)
			{
				Dbg("Can't get port ch(%d) sel info.\n", prChn);
				continue;
			}

			mapChn = pstPortChSel->chn;
			i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

			_SET_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollDd);
			WAKE_UP_INTERRUPTIBLE(&pHost->wqPoll);
			Dbg("mapChn:%d pollDd:%lx\n", mapChn COMMA pHost->wqPollChnStatus.bitWqPollDd);
		}
	}
	ret = intReg;

	return(ret);
}

int PR1000_IRQ_DFD_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost)
{
	int ret = -1;
	int mapChn; uint8_t prChn, i2cSlaveAddr; 
	_stPortChSel *pstPortChSel = NULL;

	Dbg("Int> DFD Isr prChip(%d) intReg:0x%x, intStatus:0x%x\n", prChip COMMA intReg COMMA intStatus);

	if(intReg == 0)
	{
		return(ret);
	}

	if( prChip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid prChip.\n");
		return(-1);
	}

	for(prChn = 0; prChn < DEF_PR1000_MAX_CHN; prChn++)/* only support ch0, ch1, ch2, ch3 */
	{
		if(intReg & (1<<prChn))
		{
			Dbg("Int prChn:%d.\n", prChn);

			pstPortChSel = GetMapChInfo(prChip, prChn, pHost);

			if(pstPortChSel == NULL)
			{
				Dbg("Can't get port ch(%d) sel info.\n", prChn);
				continue;
			}

			mapChn = pstPortChSel->chn;
			i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

			_SET_BIT(mapChn, &pHost->wqPollChnStatus.bitWqPollDfd);
			WAKE_UP_INTERRUPTIBLE(&pHost->wqPoll);
			Dbg("mapChn:%d pollDfd:%lx\n", mapChn COMMA pHost->wqPollChnStatus.bitWqPollDfd);
		}
	}
	ret = intReg;

	return(ret);
}

int PR1000_IRQ_AD_DIFF_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost)
{
	int ret = -1;
	uint8_t i2cSlaveAddr; 
	uint8_t prChn;
	uint8_t i;

	Dbg("Int> AD_DIFF Isr prChip(%d) intReg:0x%x, intStatus:0x%x\n", prChip COMMA intReg COMMA intStatus);

	if(intReg == 0)
	{
		return(ret);
	}

	if( prChip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid prChip.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[prChip];
	for(i = 0; i < DEF_PR1000_MAX_CHN; i++)/* only support ch0, ch1, ch2, ch3 */
	{
		if(intReg & (1<<i))
		{
			prChn = prChip*DEF_PR1000_MAX_CHN + i;
			Dbg("Int prChn:%d.\n", prChn);

			_SET_BIT(prChn, &pHost->wqPollChnStatus.bitWqPollAdDiff);
			WAKE_UP_INTERRUPTIBLE(&pHost->wqPoll);
			Dbg("prChn:%d pollAdDiff:%lx\n", prChn COMMA pHost->wqPollChnStatus.bitWqPollAdDiff);
		}
	}
	ret = intReg;

	return(ret);
}

int PR1000_IRQ_NOAUD_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost)
{
	int ret = -1;
	uint8_t i2cSlaveAddr; 
	uint8_t prChn;
	uint8_t i;

	Dbg("Int> NOAUD Isr prChip(%d) intReg:0x%x, intStatus:0x%x\n", prChip COMMA intReg COMMA intStatus);

	if(intReg == 0)
	{
		return(ret);
	}

	if( prChip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid prChip.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[prChip];
	for(i = 0; i < DEF_PR1000_MAX_CHN; i++)/* only support ch0, ch1, ch2, ch3 */
	{
		if(intReg & (1<<i))
		{
			prChn = prChip*DEF_PR1000_MAX_CHN + i;
			Dbg("Int prChn:%d.\n", prChn);

			_SET_BIT(prChn, &pHost->wqPollChnStatus.bitWqPollAdMute);
			WAKE_UP_INTERRUPTIBLE(&pHost->wqPoll);
			Dbg("prChn:%d pollAdMute:%lx\n", prChn COMMA pHost->wqPollChnStatus.bitWqPollAdMute);
		}
	}
	ret = intReg;

	return(ret);
}

int PR1000_IRQ_AD_ABS_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost)
{
	int ret = -1;
	uint8_t i2cSlaveAddr; 
	uint8_t prChn;
	uint8_t i;

	Dbg("Int> AD_ABS Isr prChip(%d) intReg:0x%x, intStatus:0x%x\n", prChip COMMA intReg COMMA intStatus);

	if(intReg == 0)
	{
		return(ret);
	}

	if( prChip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid prChip.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[prChip];
	for(i = 0; i < DEF_PR1000_MAX_CHN; i++)/* only support ch0, ch1, ch2, ch3 */
	{
		if(intReg & (1<<i))
		{
			prChn = prChip*DEF_PR1000_MAX_CHN + i;
			Dbg("Int prChn:%d.\n", prChn);

			_SET_BIT(prChn, &pHost->wqPollChnStatus.bitWqPollAdAbs);
			WAKE_UP_INTERRUPTIBLE(&pHost->wqPoll);
			Dbg("prChn:%d pollAdAbs:%lx\n", prChn COMMA pHost->wqPollChnStatus.bitWqPollAdAbs);
		}
	}
	ret = intReg;


	return(ret);
}
#endif // DONT_SUPPORT_EVENT_FUNC

int PR1000_IRQ_GPIO_Isr(const int fd, const uint8_t prChip, const uint8_t prChn, const uint8_t intReg, _drvHost *pHost)
{
	int ret = -1;
	int mapChn; uint8_t i2cSlaveAddr; 
	int tmpPrChn;
	_stPortChSel *pstPortChSel = NULL;


	Dbg("Int> GPIO Isr prChn(%d) intReg:0x%x\n", prChn COMMA intReg);

	if(intReg == 0)
	{
		return(ret);
	}

	ret = intReg;

	/* dont' use */
	#if 0
	_SET_BIT(prChn, &pHost->wqPollChnStatus.bitWqPollGpio);
	WAKE_UP_INTERRUPTIBLE(&pHost->wqPoll);
	Dbg("prChn:%d pollGpio:%lx\n", mapChn COMMA pHost->wqPollChnStatus.bitWqPollGpio);
	#endif

	for(tmpPrChn = 0; tmpPrChn < DEF_PR1000_MAX_CHN; tmpPrChn++)
	{
		if((intReg & (1<<tmpPrChn)) == 0) continue;

		pstPortChSel = GetMapChInfo(prChip, tmpPrChn, pHost);
		if(pstPortChSel == NULL)
		{
			Dbg("Can't get port ch(%d) sel info.\n", tmpPrChn);
			return(ret);
		}

		mapChn = pstPortChSel->chn;
		i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

		Dbg("CEQ Start by Gpio. Int prChip:%d, mapChn:%d(%d)\n", prChip COMMA mapChn COMMA pHost->sysHost.portChSel[mapChn].chn);
		PR1000_CEQ_Start(fd, mapChn, (void *)pHost);
	}

	return(ret);
}

int PR1000_IRQ_WAKE_Isr(const int fd, _drvHost *pHost)
{
	int ret = -1;

	_stCeqDet stCeqDet;
	_stCeqLock stCeqLock;

	uint8_t i2cReg = 0;
	uint8_t regs[3];
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 
	_stPortChSel *pstPortChSel = NULL;
	_stCeqInfoReg stCeqInfoReg;
	uint8_t estResult[2];
	uint16_t estResultFact[2];
	uint8_t bWrAtten;
	uint8_t bWrComp;

	_stCEQData *pstCEQData = NULL;

	Dbg("Int> WAKE Isr [%dmsec]\n", PR1000_INT_WAKE_PERIOD);

	for(prChip = 0; prChip < (PR1000_CHIP_COUNT); prChip++)
	{
		for(prChn = 0; prChn < DEF_PR1000_MAX_CHN; prChn++)
		{
			if( (pstPortChSel = GetMapChInfo(prChip, prChn, pHost)) == NULL)
			{
				Dbg("Can't get port ch(%d) sel info.\n", prChn);
				continue;
			}

			mapChn = pstPortChSel->chn;
			i2cSlaveAddr = pstPortChSel->i2cSlvAddr;
			pstCEQData = (_stCEQData *)&pHost->sysHost.stCEQDataList[mapChn]; //Max 4chip * 4channel

			if(pstCEQData->monStep < PR1000_MON_STEP_START) // Not yet CEQ complete, Don't check
			{
				continue;
			}

			if( _TEST_BIT(mapChn, &pHost->sysHost.bitChnWaitCheckChromaLock) )
			{
				if( _TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock) )
				{
					Print("Wait check chroma lock, mapChn:%d\n", mapChn);
					continue;
				}
				else
				{
					Print("Done check chroma lock, mapChn:%d\n", mapChn);
					_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnWaitCheckChromaLock);
				}
			}

			if( (pstCEQData->bEnable) && (pstCEQData->bLock) )
			{

				i2cSlaveAddr = PR1000_I2C_SLVADDRS[prChip];

				i2cReg = 0x00 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
				if( PR1000_PageReadBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_COMMON, i2cReg, 2, regs) < 0)
				{
					ErrorString("Read reg.\n");
					return(-1);//1:change vf or next ch. 0:do nothing, -1:error
				}
				stCeqDet.reg = regs[0];
				stCeqLock.reg = regs[1];

				Dbg("mon_step:%d Det[%02x] lock[0x%02x]\n", pstCEQData->monStep COMMA stCeqDet.reg COMMA stCeqLock.reg);

				if(pHost->sysHost.bitChnWakeIsrImmediately == 0)
				{
					if( PR1000_MON_CheckChgVF(fd, pstPortChSel, pstCEQData, (void *)pHost) > 0)//1:change vf or next ch. 0:do nothing, -1:error
					{
						ret++;
						continue;
					}
				}

				Dbg("mapChn:%d C_LOCK_CNT:%d, EQ_CNT:%d, AC_GAIN_ADJ:%d, AC_GAIN_HOLD:%d\n", mapChn COMMA 
						pstCEQData->eqProcFlag.C_LOCK_CNT COMMA 
						pstCEQData->eqProcFlag.EQ_CNT COMMA
						pstCEQData->eqProcFlag.AC_GAIN_ADJ COMMA
						pstCEQData->eqProcFlag.AC_GAIN_HOLD);

				if(pstCEQData->eqProcFlag.C_LOCK_CNT == 1)
				{
					if(pstCEQData->eqProcFlag.EQ_CNT == 1)
					{/*{{{*/
						if( stCeqLock.b.det_chroma == 1 )
						{
							Dbg("mapChn:%d mon_step:%d(%s)\n", mapChn COMMA pstCEQData->monStep COMMA _STR_PR1000_MON_STEP[pstCEQData->monStep]);
							pstCEQData->flagMonStepComplete[pstCEQData->monStep]++;
							switch(pstCEQData->monStep)
							{/*{{{*/
								case PR1000_MON_STEP_NONE:
									{
										/* do nothing */
									}
									break;
								case PR1000_MON_STEP_START:
									{
										pstCEQData->retryCnt = 0;
										if( ((stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 0)) ) 
										{/*{{{*/
											if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock) == 0)
											{
												Print("WAKE Isr Start check chroma lock fast. mapChn(%d)\n", mapChn);

												pHost->sysHost.cntChromaLockTunn[mapChn] = 0; //init
												_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
												_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);
												_SET_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock); // start check chroma lock on kthread.
												pstCEQData->monStep = PR1000_MON_STEP_CHROMALOCK1;
											}
										}/*}}}*/
										else if( ((stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 1)) ) 
										{/*{{{*/
#ifdef SUPPORT_CABLE_EQ
											if( PR1000_CEQ_GetInfoRegs(fd, pstPortChSel, &stCeqInfoReg) >= 0)
											{
												/* estResult[0]: 0x12,(From Atten value), estResult[1]: 0x13,(From Comp value) */
												/* estResultFact[0]: (From Atten dist value), estResultFact[1]: (From Comp dist value) */
												if( (PR1000_CEQ_GetSTDEst(fd, pstCEQData->format, pstCEQData->camResol, &stCeqInfoReg, estResult, estResultFact)) >= 0)
												{
													uint16_t detAcGain = stCeqInfoReg.b.det_eq_acgain_h<<8|stCeqInfoReg.b.det_eq_acgain_l;
													Print("mapChn:%d, Atten:0x%02x, detAcGain:0x%04x, dist:%d\n", mapChn COMMA pstCEQData->estResultAtten COMMA detAcGain COMMA estResultFact[1]);
													if( (pstCEQData->estResultAtten != 0) && (detAcGain < 0x0700) && (estResultFact[1] <= 1200) )
													{
														if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) == 0)
														{
															Print("WAKE Isr Start check vadc gain sel. mapChn(%d)\n", mapChn);
															pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0; //init
															_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
															_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
															_SET_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
															_SET_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel); // start check chroma lock on kthread.
															pstCEQData->monStep = PR1000_MON_STEP_VADCGAINSEL;
														}
													}
													else
													{/*{{{*/
														// + Enable EQ. write 0x12~0x13 
														bWrAtten = TRUE; bWrComp = TRUE;
														if(bWrAtten)
														{
															pstCEQData->estResultAtten = estResult[0]&0x1F;	//0x12,(From Atten value)
															pstCEQData->attenFact = estResultFact[0];

															i2cReg = 0x12 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
															estResult[0] |= 0x20; //Enable EQ
															PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_COMMON, i2cReg, (uint8_t)estResult[0]);
															Print("Set STD EQ value mapChn:%d. (Atten:%dm,0x%x)\n", mapChn COMMA estResultFact[0] COMMA estResult[0]);
														}
														if(bWrComp)
														{
															if(estResultFact[1] >= estResultFact[0])
															{
																pstCEQData->compFact = estResultFact[0];
																estResultFact[0] = pstCEQData->compFact; 
																estResultFact[1] = pstCEQData->attenFact; //get from atten distance.
																Print("mapChn:%d get from atten:%dm\n", mapChn COMMA pstCEQData->attenFact);
															}
															else
															{
																pstCEQData->compFact = estResultFact[1];
																estResultFact[1] = pstCEQData->compFact; //get from comp distance.
																estResultFact[0] = pstCEQData->attenFact;
																Print("mapChn:%d get from comp:%dm\n", mapChn COMMA pstCEQData->compFact);
															}

															if( (PR1000_CEQ_GetEQDistFactor(fd, pstCEQData->format, pstCEQData->camResol, estResultFact, estResult)) < 0)
															{
																Error("Invalid getstddist. Forcely bypass EQ. [mapChn:%d]\n", mapChn);
															}

															pstCEQData->estResultComp = estResult[1]&0x3F;	//0x13,(From Comp value)

															Print("mapChn:%d det_eq_dcgain:0x%04x\n", mapChn COMMA pstCEQData->saved_det_eq_dcgain);
															if(pstCEQData->saved_det_eq_dcgain < 0x2000)
															{
																i2cReg = 0x13 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
																PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_COMMON, i2cReg, 0);
																Print("Forcely Set STD EQ value mapChn:%d. (Comp:%dm,0x%x)\n", mapChn COMMA estResultFact[1] COMMA 0);
															}
															else
															{
																i2cReg = 0x13 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
																PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_COMMON, i2cReg, (uint8_t)estResult[1]);
																Print("Set STD EQ value mapChn:%d. (Comp:%dm,0x%x)\n", mapChn COMMA estResultFact[1] COMMA estResult[1]);
															}
														}
														PrintString("===>> Lock vadc gain sel");

														/* do vadc gain. Must do after SetEQGain */
														PR1000_CEQ_SetVADCGain(fd, pstCEQData->format, pstCEQData->camResol, pstPortChSel, pstCEQData);

														pstCEQData->monStep = PR1000_MON_STEP_START;
														pstCEQData->eqProcFlag.AC_GAIN_ADJ = 0;
														pstCEQData->eqProcFlag.EQ_CNT = 2;
													}/*}}}*/
												}
											}
#else
											if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) == 0)
											{
												Print("WAKE Isr Start check vadc gain sel. mapChn(%d)\n", mapChn);
												pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0; //init
												_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
												_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
												_SET_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
												_SET_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel); // start check chroma lock on kthread.
												pstCEQData->monStep = PR1000_MON_STEP_VADCGAINSEL;
											}
#endif // SUPPORT_CABLE_EQ

										}/*}}}*/
									}
									break;
								case PR1000_MON_STEP_CHROMALOCK1:
									{
										if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock) != 0)
										{
											break; //wait done.
										}
										if( ((stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 1)) ) 
										{/*{{{*/
#ifdef SUPPORT_CABLE_EQ
											if( PR1000_CEQ_GetInfoRegs(fd, pstPortChSel, &stCeqInfoReg) >= 0)
											{

												/* estResult[0]: 0x12,(From Atten value), estResult[1]: 0x13,(From Comp value) */
												/* estResultFact[0]: (From Atten dist value), estResultFact[1]: (From Comp dist value) */
												if( (PR1000_CEQ_GetSTDEst(fd, pstCEQData->format, pstCEQData->camResol, &stCeqInfoReg, estResult, estResultFact)) >= 0)
												{
													uint16_t detAcGain = stCeqInfoReg.b.det_eq_acgain_h<<8|stCeqInfoReg.b.det_eq_acgain_l;
													Print("mapChn:%d, Atten:0x%02x, detAcGain:0x%04x, dist:%d\n", mapChn COMMA pstCEQData->estResultAtten COMMA detAcGain COMMA estResultFact[1]);
													if( (pstCEQData->estResultAtten != 0) && (detAcGain < 0x0700) && (estResultFact[1] <= 1200) )
													{
														if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) == 0)
														{
															Print("WAKE Isr Start check vadc gain sel. mapChn(%d)\n", mapChn);
															pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0; //init
															_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
															_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
															_SET_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
															_SET_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel); // start check chroma lock on kthread.

															pstCEQData->monStep = PR1000_MON_STEP_VADCGAINSEL;
														}
													}
													else
													{/*{{{*/
														bWrAtten = TRUE; bWrComp = TRUE;
														if(bWrAtten)
														{
															pstCEQData->estResultAtten = estResult[0]&0x1F;	//0x12,(From Atten value)
															pstCEQData->attenFact = estResultFact[0];

															i2cReg = 0x12 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
															estResult[0] |= 0x20; //Enable EQ
															PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_COMMON, i2cReg, (uint8_t)estResult[0]);
															Print("Set STD EQ value mapChn:%d. (Atten:%dm,0x%x)\n", mapChn COMMA estResultFact[0] COMMA estResult[0]);
														}
														if(bWrComp)
														{
															if(estResultFact[1] >= estResultFact[0])
															{
																pstCEQData->compFact = estResultFact[0];
																estResultFact[0] = pstCEQData->compFact; 
																estResultFact[1] = pstCEQData->attenFact; //get from atten distance.
																Print("mapChn:%d get from atten:%dm\n", mapChn COMMA pstCEQData->attenFact);
															}
															else
															{
																pstCEQData->compFact = estResultFact[1];
																estResultFact[1] = pstCEQData->compFact; //get from comp distance.
																estResultFact[0] = pstCEQData->attenFact;
																Print("mapChn:%d get from comp:%dm\n", mapChn COMMA pstCEQData->compFact);
															}

															if( (PR1000_CEQ_GetEQDistFactor(fd, pstCEQData->format, pstCEQData->camResol, estResultFact, estResult)) < 0)
															{
																Error("Invalid getstddist. Forcely bypass EQ. [mapChn:%d]\n", mapChn);
															}

															pstCEQData->estResultComp = estResult[1]&0x3F;	//0x13,(From Comp value)

															Print("mapChn:%d det_eq_dcgain:0x%04x\n", mapChn COMMA pstCEQData->saved_det_eq_dcgain);
															if(pstCEQData->saved_det_eq_dcgain < 0x2000)
															{
																i2cReg = 0x13 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
																PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_COMMON, i2cReg, 0);
																Print("Forcely Set STD EQ value mapChn:%d. (Comp:%dm,0x%x)\n", mapChn COMMA estResultFact[1] COMMA 0);
															}
															else
															{
																i2cReg = 0x13 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
																PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_COMMON, i2cReg, (uint8_t)estResult[1]);
																Print("Set STD EQ value mapChn:%d. (Comp:%dm,0x%x)\n", mapChn COMMA estResultFact[1] COMMA estResult[1]);
															}
														}

														/* do vadc gain. Must do after SetEQGain */
														PR1000_CEQ_SetVADCGain(fd, pstCEQData->format, pstCEQData->camResol, pstPortChSel, pstCEQData);

														pstCEQData->monStep = PR1000_MON_STEP_START;
														pstCEQData->eqProcFlag.AC_GAIN_ADJ = 0;
														pstCEQData->eqProcFlag.EQ_CNT = 2;
													}/*}}}*/
												}
											}
#else
											if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) == 0)
											{
												Print("WAKE Isr Start check vadc gain sel. mapChn(%d)\n", mapChn);
												pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0; //init
												_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
												_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
												_SET_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
												_SET_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel); // start check chroma lock on kthread.

												pstCEQData->monStep = PR1000_MON_STEP_VADCGAINSEL;
											}
#endif // SUPPORT_CABLE_EQ
										}/*}}}*/
										else
										{/*{{{*/
											pstCEQData->monStep = PR1000_MON_STEP_START;
											if(pstCEQData->retryCnt++ >= 5)
											{
												pstCEQData->eqProcFlag.AC_GAIN_ADJ = 0;
												pstCEQData->eqProcFlag.EQ_CNT = 2;
											}
										}/*}}}*/
									}
									break;
								case PR1000_MON_STEP_VADCGAINSEL:
									{
										if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) != 0)
										{
											break; //wait done.
										}

										bWrAtten = FALSE; bWrComp = TRUE;
										if(PR1000_CEQ_SetEQGain(fd, bWrAtten, bWrComp, pstPortChSel, pstCEQData) < 0)
										{
											Error("Set EQ Gain. mapChn:%d\n", mapChn);
											break;
										}

										if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel))
										{
											pstCEQData->eqProcFlag.AC_GAIN_ADJ = 0;
											pstCEQData->eqProcFlag.EQ_CNT = 2;
											pstCEQData->eqProcFlag.AC_GAIN_HOLD = 0;
										}
										else
										{
											pstCEQData->eqProcFlag.AC_GAIN_ADJ = 0;
											pstCEQData->eqProcFlag.EQ_CNT = 2;
											pstCEQData->eqProcFlag.AC_GAIN_HOLD = 1;
										}
										Print("mapChn:%d, AC_GAIN_HOLD:%d\n", mapChn COMMA pstCEQData->eqProcFlag.AC_GAIN_HOLD);

										pstCEQData->monStep = PR1000_MON_STEP_START;
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
									}
									break;
								default:
									{
										Error("mapChn:%d\n", mapChn);
									}
									break;
							}/*}}}*/
						}
						else
						{
							/* read eq info and set eq gain */
							bWrAtten = TRUE; bWrComp = FALSE;
							if(PR1000_CEQ_SetEQGain(fd, bWrAtten, bWrComp, pstPortChSel, pstCEQData) < 0)
							{
								Error("Set EQ Gain. mapChn:%d\n", mapChn);
								break;
							}
							/* do vadc gain. Must do after SetEQGain */
							if(PR1000_CEQ_SetVADCGain(fd, pstCEQData->format, pstCEQData->camResol, pstPortChSel, pstCEQData) < 0)
							{
								Error("Set VADC Gain. mapChn:%d\n", mapChn);
								break;
							}
						}
					}/*}}}*/
					else //if(pstCEQData->eqProcFlag.EQ_CNT == 1)
					{
						if( (pstCEQData->eqProcFlag.AC_GAIN_ADJ == 1) && (pstCEQData->compFact <= 1200) )
						{/*{{{*/
							if( (pstCEQData->eqProcFlag.AC_GAIN_HOLD != 0) || (stCeqLock.b.det_chroma != 1) )
							{
								/* hold the previous value */
								Print("mapChn:%d, hold the previous value\n", mapChn);
								continue;
							}
							else
							{
								Dbg("mapChn:%d mon_step:%d(%s)\n", mapChn COMMA pstCEQData->monStep COMMA _STR_PR1000_MON_STEP[pstCEQData->monStep]);
								pstCEQData->flagMonStepComplete[pstCEQData->monStep]++;
								switch(pstCEQData->monStep)
								{/*{{{*/
									case PR1000_MON_STEP_NONE:
										{
											/* do nothing */
										}
										break;
									case PR1000_MON_STEP_START:
										{
											pstCEQData->retryCnt = 0;
											if( ((stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 0)) ) 
											{/*{{{*/
												if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock) == 0)
												{
													Print("WAKE Isr Start check chroma lock fast. mapChn(%d)\n", mapChn);

													pHost->sysHost.cntChromaLockTunn[mapChn] = 0; //init
													_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
													_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);
													_SET_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock); // start check chroma lock on kthread.
													pstCEQData->monStep = PR1000_MON_STEP_CHROMALOCK3;
												}
											}/*}}}*/
											else if( ((stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 1)) ) 
											{/*{{{*/
#ifdef SUPPORT_CABLE_EQ
												if( PR1000_CEQ_GetInfoRegs(fd, pstPortChSel, &stCeqInfoReg) >= 0)
												{
													/* estResult[0]: 0x12,(From Atten value), estResult[1]: 0x13,(From Comp value) */
													/* estResultFact[0]: (From Atten dist value), estResultFact[1]: (From Comp dist value) */
													if( (PR1000_CEQ_GetSTDEst(fd, pstCEQData->format, pstCEQData->camResol, &stCeqInfoReg, estResult, estResultFact)) >= 0)
													{
														uint16_t detAcGain = stCeqInfoReg.b.det_eq_acgain_h<<8|stCeqInfoReg.b.det_eq_acgain_l;
														Print("mapChn:%d, detAcGain:0x%04x\n", mapChn COMMA detAcGain);
														if( (detAcGain < 0x0700) || (detAcGain >= 0x0900) )
														{
															if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) == 0)
															{
																Print("WAKE Isr Start check vadc gain sel. mapChn(%d)\n", mapChn);
																pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0; //init
																_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
																_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
																_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
																_SET_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel); // start check chroma lock on kthread.
																pstCEQData->monStep = PR1000_MON_STEP_START;
															}
														}
													}
												}
#else
												if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) == 0)
												{
													Print("WAKE Isr Start check vadc gain sel. mapChn(%d)\n", mapChn);
													pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0; //init
													_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
													_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
													_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
													_SET_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel); // start check chroma lock on kthread.
													pstCEQData->monStep = PR1000_MON_STEP_START;
												}

#endif // SUPPORT_CABLE_EQ
											}/*}}}*/
										}
										break;
									case PR1000_MON_STEP_CHROMALOCK3:
										{
											if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock) != 0)
											{
												break; //wait done.
											}
											if( ((stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 1)) ) 
											{
#ifdef SUPPORT_CABLE_EQ
												if( PR1000_CEQ_GetInfoRegs(fd, pstPortChSel, &stCeqInfoReg) >= 0)
												{
													/* estResult[0]: 0x12,(From Atten value), estResult[1]: 0x13,(From Comp value) */
													/* estResultFact[0]: (From Atten dist value), estResultFact[1]: (From Comp dist value) */
													if( (PR1000_CEQ_GetSTDEst(fd, pstCEQData->format, pstCEQData->camResol, &stCeqInfoReg, estResult, estResultFact)) >= 0)
													{
														uint16_t detAcGain = stCeqInfoReg.b.det_eq_acgain_h<<8|stCeqInfoReg.b.det_eq_acgain_l;
														Print("mapChn:%d, detAcGain:0x%04x\n", mapChn COMMA detAcGain);
														if( (detAcGain < 0x0700) || (detAcGain >= 0x0900) )
														{
															if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) == 0)
															{
																Print("WAKE Isr Start check vadc gain sel. mapChn(%d)\n", mapChn);
																pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0; //init
																_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
																_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
																_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
																_SET_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel); // start check chroma lock on kthread.

																pstCEQData->monStep = PR1000_MON_STEP_START;
																break; //exit. this function in ISR.
															}
														}
													}
												}
#else
												if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) == 0)
												{
													Print("WAKE Isr Start check vadc gain sel. mapChn(%d)\n", mapChn);
													pHost->sysHost.cntVadcGainSelTunn[mapChn] = 0; //init
													_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneVadcGainSel);
													_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultVadcGainSel);
													_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnApplyVadcGainSel);
													_SET_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel); // start check chroma lock on kthread.

													pstCEQData->monStep = PR1000_MON_STEP_START;
													break; //exit. this function in ISR.
												}

#endif // SUPPORT_CABLE_EQ
											}
											else
											{
												pstCEQData->monStep = PR1000_MON_STEP_START;
												if(pstCEQData->retryCnt++ >= 5)
												{
													pstCEQData->eqProcFlag.AC_GAIN_HOLD = 1;
												}
											}
										}
										break;
									default:
									{
										Error("mapChn:%d\n", mapChn);
									}
									break;
								}/*}}}*/
							}
						}/*}}}*/
						else
						{
							Dbg("mapChn:%d mon_step:%d(%s)\n", mapChn COMMA pstCEQData->monStep COMMA _STR_PR1000_MON_STEP[pstCEQData->monStep]);
							pstCEQData->flagMonStepComplete[pstCEQData->monStep]++;
							switch(pstCEQData->monStep)
							{/*{{{*/
								case PR1000_MON_STEP_NONE:
									{
										/* do nothing */
									}
									break;
								case PR1000_MON_STEP_START:
									{
										pstCEQData->retryCnt = 0;
										if( ((stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 0)) ) 
										{/*{{{*/
											if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock) == 0)
											{
												Print("WAKE Isr Start check chroma lock fast. mapChn(%d)\n", mapChn);

												pHost->sysHost.cntChromaLockTunn[mapChn] = 0; //init
												_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
												_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);
												_SET_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock); // start check chroma lock on kthread.
												pstCEQData->monStep = PR1000_MON_STEP_CHROMALOCK2;
											}
										}/*}}}*/
										else if( ((stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 1)) ) 
										{
											/* CEQ compensate */
											bWrAtten = FALSE; bWrComp = TRUE;
											if( (ret = PR1000_CEQ_Compensate(fd, mapChn,
															(enum _pr1000_table_format)pHost->eventHost.stEventDetStd[mapChn].format, 
															(enum _pr1000_table_inresol)pHost->eventHost.stEventDetStd[mapChn].resol, 
															bWrAtten, bWrComp,
															(void *)pHost)) >= 0)
											{
												pstCEQData->flagMonStepComplete[PR1000_MON_STEP_COMPENSATE]++;
											}
										}

									}
									break;
								case PR1000_MON_STEP_CHROMALOCK2:
									{
										if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock) != 0)
										{
											break; //wait done.
										}
										if( ((stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 1)) ) 
										{
											/* CEQ compensate */
											bWrAtten = FALSE; bWrComp = TRUE;
											if( (ret = PR1000_CEQ_Compensate(fd, mapChn,
															(enum _pr1000_table_format)pHost->eventHost.stEventDetStd[mapChn].format, 
															(enum _pr1000_table_inresol)pHost->eventHost.stEventDetStd[mapChn].resol, 
															bWrAtten, bWrComp,
															(void *)pHost)) >= 0)
											{
												pstCEQData->flagMonStepComplete[PR1000_MON_STEP_COMPENSATE]++;
											}

										}
										else
										{
											pstCEQData->monStep = PR1000_MON_STEP_START;
											if(pstCEQData->retryCnt++ >= 5)
											{
												pstCEQData->eqProcFlag.AC_GAIN_HOLD = 1;
											}
										}
									}
									break;
								default:
									{
										Error("mapChn:%d\n", mapChn);
									}
									break;
							}/*}}}*/
						}
					}
				}
				else //if(pstCEQData->eqProcFlag.C_LOCK_CNT == 1)
				{
					Dbg("mapChn:%d mon_step:%d(%s)\n", mapChn COMMA pstCEQData->monStep COMMA _STR_PR1000_MON_STEP[pstCEQData->monStep]);
					pstCEQData->flagMonStepComplete[pstCEQData->monStep]++;
					switch(pstCEQData->monStep)
					{/*{{{*/
						case PR1000_MON_STEP_NONE:
							{
								/* do nothing */
							}
							break;
						case PR1000_MON_STEP_START:
							{
								enum _pr1000_table_format format;
								enum _pr1000_table_inresol camResol;

								pstCEQData->retryCnt = 0;

								PR1000_IfmtToStdResol(fd, stCeqDet.b.det_ifmt_std, stCeqDet.b.det_ifmt_ref, stCeqDet.b.det_ifmt_res, &format, &camResol);
								Dbg("mapChn:%d, [%s, %s]\n", mapChn COMMA _STR_PR1000_FORMAT[format] COMMA _STR_PR1000_INRESOL[camResol]);
								if( (format == pr1000_format_HDA) &&
									((camResol == pr1000_inresol_1280x720p60) ||
									(camResol == pr1000_inresol_1280x720p50) ||
									(camResol == pr1000_inresol_1280x720p30) ||
									(camResol == pr1000_inresol_1280x720p25)) )
								{
									if( stCeqLock.b.det_chroma == 1 ) 
									{/*{{{*/
										if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock) == 0)
										{
											Print("WAKE Isr Start check chroma lock fast. mapChn(%d)\n", mapChn);

											pHost->sysHost.cntChromaLockTunn[mapChn] = 0; //init
											_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
											_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);
											_SET_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock); // start check chroma lock on kthread.
										}
										pstCEQData->monStep = PR1000_MON_STEP_CHROMALOCK4;
									}/*}}}*/
									else
									{
										if(PR1000_CEQ_SetVIDHDA_CUNLOCK(fd, pstPortChSel, pstCEQData->camResol) >= 0)
										{
											pstCEQData->monStep = PR1000_MON_STEP_CHECKSTDHDA;
										}
									}
								}
								else
								{
									if( stCeqLock.b.det_chroma == 1 ) 
									{/*{{{*/
										if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock) == 0)
										{
											Print("WAKE Isr Start check chroma lock fast. mapChn(%d)\n", mapChn);

											pHost->sysHost.cntChromaLockTunn[mapChn] = 0; //init
											_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
											_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);
											_SET_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock); // start check chroma lock on kthread.
										}
										pstCEQData->monStep = PR1000_MON_STEP_CHROMALOCK6;
									}/*}}}*/
									else
									{
										pstCEQData->monStep = PR1000_MON_STEP_START;
									}

								}
							}
							break;
						case PR1000_MON_STEP_CHROMALOCK4:
							{
								if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock) != 0)
								{
									break; //wait done.
								}
								if( ((stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 1)) ) 
								{
									pstCEQData->eqProcFlag.C_LOCK_CNT = 1;
									pstCEQData->monStep = PR1000_MON_STEP_START;
								}
								else
								{
									if(PR1000_CEQ_SetVIDCVI_CUNLOCK(fd, pstPortChSel, pstCEQData->camResol) >= 0)
									{
										pstCEQData->monStep = PR1000_MON_STEP_START;
									}
								}
							}
							break;
						case PR1000_MON_STEP_CHECKSTDHDA:
							{
								if( stCeqLock.b.det_chroma == 1 ) 
								{/*{{{*/
									if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock) == 0)
									{
										Print("WAKE Isr Start check chroma lock fast. mapChn(%d)\n", mapChn);

										pHost->sysHost.cntChromaLockTunn[mapChn] = 0; //init
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
										_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);
										_SET_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock); // start check chroma lock on kthread.
									}
									pstCEQData->monStep = PR1000_MON_STEP_CHROMALOCK5;
								}/*}}}*/
								else
								{
									if(PR1000_CEQ_SetVIDCVI_CUNLOCK(fd, pstPortChSel, pstCEQData->camResol) >= 0)
									{
										pstCEQData->monStep = PR1000_MON_STEP_START;
									}
								}
							}
							break;
						case PR1000_MON_STEP_CHROMALOCK5:
							{
								if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock) != 0)
								{
									break; //wait done.
								}
								if( ((stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 1)) ) 
								{
									/* read eq info and set eq gain */
									bWrAtten = TRUE; bWrComp = FALSE;
									if(PR1000_CEQ_SetEQGain(fd, bWrAtten, bWrComp, pstPortChSel, pstCEQData) < 0)
									{
										Error("Set EQ Gain. mapChn:%d\n", mapChn);
										break;
									}
									/* do vadc gain. Must do after SetEQGain */
									if(PR1000_CEQ_SetVADCGain(fd, pstCEQData->format, pstCEQData->camResol, pstPortChSel, pstCEQData) < 0)
									{
										Error("Set VADC Gain. mapChn:%d\n", mapChn);
										break;
									}

									pstCEQData->bForceChgStd = FALSE;	// forcely change standard format.
									pstCEQData->format = pr1000_format_HDA;
									PR1000_CEQ_Done(fd, mapChn, (void *)pHost);

									pstCEQData->eqProcFlag.C_LOCK_CNT = 1;
									pstCEQData->monStep = PR1000_MON_STEP_START;
								}
								else
								{
									if(PR1000_CEQ_SetVIDCVI_CUNLOCK(fd, pstPortChSel, pstCEQData->camResol) >= 0)
									{
										pstCEQData->monStep = PR1000_MON_STEP_START;
									}
								}
							}
							break;
						case PR1000_MON_STEP_CHROMALOCK6:
							{
								if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock) != 0)
								{
									break; //wait done.
								}
								if( ((stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 1)) ) 
								{
									pstCEQData->eqProcFlag.C_LOCK_CNT = 1;
									pstCEQData->monStep = PR1000_MON_STEP_START;
								}
								else
								{
									pstCEQData->monStep = PR1000_MON_STEP_START;
								}
							}
							break;
						default:
							{
								Error("mapChn:%d\n", mapChn);
							}
							break;
					}/*}}}*/
				}
			}
		}
	}

	return(ret);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*** VID(video) control. ***/
//////////////////////////////////////////////////////////////////////////////////////////////////////
int PR1000_VID_LoadTable(const int fd, const _stPortChSel *pstPortChSel, const uint8_t bBootTime, const uint8_t format, const uint8_t resol)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint16_t tblData = 0;
	uint8_t i2cMask = 0;
	_PR1000_REG_TABLE_VDEC_SD *pTableSD = NULL;
	_PR1000_REG_TABLE_VDEC_HD *pTableHD = NULL;
	_PR1000_REG_TABLE_COMMON *pTableVDEC_common = NULL;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
	_PR1000_REG_TABLE_VDEC_HD_EXTEND *pTableHD_EXTEND = NULL;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 
	uint8_t page;

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	Print("> PixelPlus PR1000 Load table [mapChn:%d, prChip:%d, prChn:%d, %s, %s]\n", mapChn COMMA prChip COMMA prChn COMMA _STR_PR1000_FORMAT[format] COMMA _STR_PR1000_OUTRESOL[resol]);
	#ifdef DEBUG_VERIFY_I2C_WRTABLE_PR1000
	Print("> And verify i2c write table [mapChn:%d]\n", mapChn);
	#endif //DEBUG_VERIFY_I2C_WRTABLE_PR1000

	if( (format >= max_pr1000_format) || (resol >= max_pr1000_outresol) )
	{
		ErrorString("invalid type.\n");
		return(-1);
	}

	/* write vdec common */
	pTableVDEC_common = (_PR1000_REG_TABLE_COMMON *)pr1000_reg_table_vdec_common;
	while(pTableVDEC_common->addr != 0xFF)
	{/*{{{*/
		i2cReg = pTableVDEC_common->addr;
		tblData = pTableVDEC_common->pData;
		pTableVDEC_common++;

		if(tblData == 0xFF00) continue; //skip
		else if(tblData == 0xFFFF) break; //end
		else
		{
			page = PR1000_VDEC_PAGE(prChn);
			i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
			Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA tblData);
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, (uint8_t)tblData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}
	}/*}}}*/

	//If old chip, change setting(0x6A,0x6B).
	{
		uint8_t revID = 0;
		if(PR1000_PageRead(fd, i2cSlaveAddr, 0, 0xFE, &i2cData) < 0)
		{
			ErrorString("Read chipID.\n");
			return(-1);
		}
		revID = i2cData>>4;

		if(revID == 0) 
		{
			page = PR1000_VDEC_PAGE(prChn);
			i2cReg = 0x6A;
			i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
			i2cData = 0x11;
			Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA i2cData);
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			page = PR1000_VDEC_PAGE(prChn);
			i2cReg = 0x6B;
			i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
			i2cData = 0x00;
			Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA i2cData);
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}
	}

	/* loading channel table register */
	if( (format == pr1000_format_SD720) || (format == pr1000_format_SD960) ) // sd720, sd960 format
	{/*{{{*/
		if(resol > pr1000_outresol_960x576i50)
		{
			Error("Invalid sd resol(%d).\n", resol);
			return(-1);
		}

		pTableSD = (_PR1000_REG_TABLE_VDEC_SD *)pr1000_reg_table_vdec_SD;

		while(pTableSD->addr != 0xFF)
		{
			i2cReg = pTableSD->addr;
			tblData = pTableSD->pData[resol];
			pTableSD++;

			if(tblData == 0xFF00) continue; //skip
			else if(tblData == 0xFFFF) break; //end
			else
			{
				page = (tblData>>8);
				if(page == PR1000_REG_PAGE_COMMON) //0x10,0x30,0x50,0x70
				{
					if( (i2cReg == 0x11) && (bBootTime == FALSE) )
					{
						i2cReg += PR1000_OFFSETADDR_VDEC_0PAGE_CH(prChn);
						i2cMask = 0x0F; i2cData = (uint8_t)tblData;
						Dbg("Write [p%d, 0x%02x-0x%02x(mask:%02x)]\n", page COMMA i2cReg COMMA i2cData COMMA i2cMask);
						if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
						}
					}
					else
					{

						i2cReg += PR1000_OFFSETADDR_VDEC_0PAGE_CH(prChn);
						Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA tblData);
						if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, (uint8_t)tblData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
#ifdef DEBUG_VERIFY_I2C_WRTABLE_PR1000
						{
							uint8_t rData, wData;

							wData = (uint8_t)tblData;
							if( PR1000_PageRead(fd, i2cSlaveAddr, page, (uint8_t)i2cReg, &rData) < 0)
							{
								ErrorString("Read reg.\n");
								return(-1);
							}
							if(wData != rData) 
							{
								Error("Error [p%d, addr:0x%2x, w:0x%02x, r:0x%02x]\n", page COMMA i2cReg COMMA wData COMMA rData);
							}
						}
#endif // DEBUG_VERIFY_I2C_WRTABLE_PR1000
					}
				}
				else if(page == PR1000_REG_PAGE_VDEC1)
				{
					/* COMB_VCRL_VTH register write only to ch0 */
					if( ((prChn > 0 ) && (i2cReg >= 0x70)) ) continue;

					if(i2cReg == 0x4F)
					{
						Dbg("Enable CHIPID(mapChn:%d)\n", mapChn);
						page = PR1000_VDEC_PAGE(prChn);
						i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
						if( PR1000_VIDOUTF_MUX_CH == PR1000_VID_OUTF_MUX_1CH)i2cData = (uint8_t)tblData;
						else i2cData = (uint8_t)tblData | 0x0C;

						Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA tblData);
						if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
					}
					else
					{

						page = PR1000_VDEC_PAGE(prChn);
						i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
						Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA tblData);
						if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, (uint8_t)tblData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
					}
#ifdef DEBUG_VERIFY_I2C_WRTABLE_PR1000
					{
						uint8_t rData, wData;

						wData = (uint8_t)tblData;
						if( PR1000_PageRead(fd, i2cSlaveAddr, page, (uint8_t)i2cReg, &rData) < 0)
						{
							ErrorString("Read reg.\n");
							return(-1);
						}
						if(wData != rData) 
						{
							Error("Error [p%d, addr:0x%2x, w:0x%02x, r:0x%02x]\n", page COMMA i2cReg COMMA wData COMMA rData);
						}
					}
#endif // DEBUG_VERIFY_I2C_WRTABLE_PR1000
				}
			}
		}
	}/*}}}*/
	else if( 
#ifndef DONT_SUPPORT_STD_PVI
			(format == pr1000_format_PVI) || 
#endif // DONT_SUPPORT_STD_PVI
			(format == pr1000_format_HDA) || 
			(format == pr1000_format_CVI) || 
			(format == pr1000_format_HDT) || 
			(format == pr1000_format_HDT_NEW) 
	       ) // hd 720p,1080p format
	{/*{{{*/
		if( (resol < pr1000_outresol_1280x720p60) )
		{
			Error("Invalid hd resol(%d).\n", resol);
			return(-1);
		}

		switch(format)
		{
#ifndef DONT_SUPPORT_STD_PVI
			case pr1000_format_PVI:
				{
					pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_PVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
					pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_PVI_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
				}
				break;
#endif // DONT_SUPPORT_STD_PVI
			case pr1000_format_HDA:
				{
					pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_HDA;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
					pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_HDA_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
				}
				break;
			case pr1000_format_CVI:
				{
					pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_CVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
					pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_CVI_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
				}
				break;
			case pr1000_format_HDT:
				{
					pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_HDT;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
					pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_HDT_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
				}
				break;
			case pr1000_format_HDT_NEW:
				{
					pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_HDT_NEW;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
					pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_HDT_NEW_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
				}
				break;
			default: 
				{
					pTableHD = (_PR1000_REG_TABLE_VDEC_HD *)pr1000_reg_table_vdec_CVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
					pTableHD_EXTEND = (_PR1000_REG_TABLE_VDEC_HD_EXTEND *)pr1000_reg_table_vdec_CVI_extend;
#endif // DONT_SUPPORT_VDRESOL_EXTEND
				}
				break;
		}
		if(resol <= pr1000_outresol_1920x1080p25)
		{
			while(pTableHD->addr != 0xFF)
			{/*{{{*/
				i2cReg = pTableHD->addr;
				tblData = pTableHD->pData[resol - pr1000_outresol_1280x720p60];
				pTableHD++;

				if(tblData == 0xFF00) continue; //skip
				else if(tblData == 0xFFFF) break; //end
				else
				{
					page = (tblData>>8);
					if(page == PR1000_REG_PAGE_COMMON) //0x10,0x30,0x50,0x70
					{
						if( (i2cReg == 0x11) && (bBootTime == FALSE) )
						{
							i2cReg += PR1000_OFFSETADDR_VDEC_0PAGE_CH(prChn);
							i2cMask = 0x0F; i2cData = (uint8_t)tblData;
							Dbg("Write [p%d, 0x%02x-0x%02x(mask:%02x)]\n", page COMMA i2cReg COMMA i2cData COMMA i2cMask);
							if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
							{
								ErrorString("Write reg.\n");
							}
						}
						else
						{
							i2cReg += PR1000_OFFSETADDR_VDEC_0PAGE_CH(prChn);
							Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA tblData);
							if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, (uint8_t)tblData)) < 0)
							{
								ErrorString("Write reg.\n");
								return(-1);
							}
#ifdef DEBUG_VERIFY_I2C_WRTABLE_PR1000
							{
								uint8_t rData, wData;

								wData = (uint8_t)tblData;
								if( PR1000_PageRead(fd, i2cSlaveAddr, page, (uint8_t)i2cReg, &rData) < 0)
								{
									ErrorString("Read reg.\n");
									return(-1);
								}
								if(wData != rData) 
								{
									Error("Error [p%d, addr:0x%2x, w:0x%02x, r:0x%02x]\n", page COMMA i2cReg COMMA wData COMMA rData);
								}
							}
#endif // DEBUG_VERIFY_I2C_WRTABLE_PR1000
						}
					}
					else if(page == PR1000_REG_PAGE_VDEC1)
					{
						/* COMB_VCRL_VTH register write only to ch0 */
						if( ((prChn > 0 ) && (i2cReg >= 0x70)) ) continue;

						if(i2cReg == 0x4F)
						{
							Dbg("Enable CHIPID(mapChn:%d)\n", mapChn);
							page = PR1000_VDEC_PAGE(prChn);
							i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
							if( PR1000_VIDOUTF_MUX_CH == PR1000_VID_OUTF_MUX_1CH)i2cData = (uint8_t)tblData;
							else i2cData = (uint8_t)tblData | 0x0C;

							Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA tblData);
							if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
							{
								ErrorString("Write reg.\n");
								return(-1);
							}
						}
						else
						{
							page = PR1000_VDEC_PAGE(prChn);
							i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
							Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA tblData);
							if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, (uint8_t)tblData)) < 0)
							{
								ErrorString("Write reg.\n");
								return(-1);
							}
						}
#ifdef DEBUG_VERIFY_I2C_WRTABLE_PR1000
						{
							uint8_t rData, wData;

							wData = (uint8_t)tblData;
							if( PR1000_PageRead(fd, i2cSlaveAddr, page, (uint8_t)i2cReg, &rData) < 0)
							{
								ErrorString("Read reg.\n");
								return(-1);
							}
							if(wData != rData) 
							{
								Error("Error [p%d, addr:0x%2x, w:0x%02x, r:0x%02x]\n", page COMMA i2cReg COMMA wData COMMA rData);
							}
						}
#endif // DEBUG_VERIFY_I2C_WRTABLE_PR1000
					}
				}
			}/*}}}*/
		}
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
		else
		{
			while(pTableHD_EXTEND->addr != 0xFF)
			{/*{{{*/
				i2cReg = pTableHD_EXTEND->addr;
				tblData = pTableHD_EXTEND->pData[resol - pr1000_outresol_1280x720p60c];
				pTableHD_EXTEND++;

				if(tblData == 0xFF00) continue; //skip
				else if(tblData == 0xFFFF) break; //end
				else
				{
					page = (tblData>>8);
					if(page == PR1000_REG_PAGE_COMMON) //0x10,0x30,0x50,0x70
					{
						if( (i2cReg == 0x11) && (bBootTime == FALSE) )
						{
							i2cReg += PR1000_OFFSETADDR_VDEC_0PAGE_CH(prChn);
							i2cMask = 0x0F; i2cData = (uint8_t)tblData;
							Dbg("Write [p%d, 0x%02x-0x%02x(mask:%02x)]\n", page COMMA i2cReg COMMA i2cData COMMA i2cMask);
							if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
							{
								ErrorString("Write reg.\n");
							}
						}
						else
						{
							i2cReg += PR1000_OFFSETADDR_VDEC_0PAGE_CH(prChn);
							Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA tblData);
							if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, (uint8_t)tblData)) < 0)
							{
								ErrorString("Write reg.\n");
								return(-1);
							}
#ifdef DEBUG_VERIFY_I2C_WRTABLE_PR1000
							{
								uint8_t rData, wData;

								wData = (uint8_t)tblData;
								if( PR1000_PageRead(fd, i2cSlaveAddr, page, (uint8_t)i2cReg, &rData) < 0)
								{
									ErrorString("Read reg.\n");
									return(-1);
								}
								if(wData != rData) 
								{
									Error("Error [p%d, addr:0x%2x, w:0x%02x, r:0x%02x]\n", page COMMA i2cReg COMMA wData COMMA rData);
								}
							}
#endif // DEBUG_VERIFY_I2C_WRTABLE_PR1000
						}
					}
					else if(page == PR1000_REG_PAGE_VDEC1)
					{
						/* COMB_VCRL_VTH register write only to ch0 */
						if( ((prChn > 0 ) && (i2cReg >= 0x70)) ) continue;

						if(i2cReg == 0x4F)
						{
							Dbg("Enable CHIPID(mapChn:%d)\n", mapChn);
							page = PR1000_VDEC_PAGE(prChn);
							i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
							if( PR1000_VIDOUTF_MUX_CH == PR1000_VID_OUTF_MUX_1CH)i2cData = (uint8_t)tblData;
							else i2cData = (uint8_t)tblData | 0x0C;

							Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA tblData);
							if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
							{
								ErrorString("Write reg.\n");
								return(-1);
							}
						}
						else
						{

							page = PR1000_VDEC_PAGE(prChn);
							i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
							Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA tblData);
							if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, (uint8_t)tblData)) < 0)
							{
								ErrorString("Write reg.\n");
								return(-1);
							}
						}
#ifdef DEBUG_VERIFY_I2C_WRTABLE_PR1000
						{
							uint8_t rData, wData;

							wData = (uint8_t)tblData;
							if( PR1000_PageRead(fd, i2cSlaveAddr, page, (uint8_t)i2cReg, &rData) < 0)
							{
								ErrorString("Read reg.\n");
								return(-1);
							}
							if(wData != rData) 
							{
								Error("Error [p%d, addr:0x%2x, w:0x%02x, r:0x%02x]\n", page COMMA i2cReg COMMA wData COMMA rData);
							}
						}
#endif // DEBUG_VERIFY_I2C_WRTABLE_PR1000
					}
				}
			}/*}}}*/
		}
#endif // DONT_SUPPORT_VDRESOL_EXTEND


	}/*}}}*/
	else
	{
		Error("Invalid format(%d).\n", format);
		return(-1);
	}

	/* apply loading register. */
	{/*{{{*/
		page = PR1000_VDEC_PAGE(prChn);
		i2cReg = 0x61 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
		i2cData = 0x0E;
		Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA i2cData);
		if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
		page = PR1000_VDEC_PAGE(prChn);
		i2cReg = 0x61 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
		i2cData = 0x0F;
		Dbg("Write [p%d, 0x%02x-0x%04x]\n", page COMMA i2cReg COMMA i2cData);
		if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}/*}}}*/

#ifdef SUPPORT_CABLE_EQ
	Dbg("Enable Manual EQ(mapChn:%d)\n", mapChn);
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0x12 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
	i2cMask = 0x20; i2cData = 0x20;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
#else
	Dbg("Disble Manual EQ(mapChn:%d)\n", mapChn);
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0x12 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
	i2cMask = 0x20; i2cData = 0x00;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
#endif // SUPPORT_CABLE_EQ

	return(ret);
}

int PR1000_VID_SetChnAttr(const int fd, const _stPortChSel *pstPortChSel, const _stChnAttr *pstChnAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstChnAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	//uint16_t u16HActive; //b[12:0]
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x12; //u16HActive [12:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = (pstChnAttr->u16HActive >> 8)&0x1F;
	i2cMask = 0x1F;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x14; //u16HActive [12:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = pstChnAttr->u16HActive & 0xFF;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	//uint16_t u16HDelay; //b[12:0]
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x11; //u16HDelay [12:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = (pstChnAttr->u16HDelay >> 8)&0x1F;
	i2cMask = 0x1F;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x13; //u16HDelay [12:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = pstChnAttr->u16HDelay & 0xFF;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	//uint16_t u16VActive; //b[10:0]
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x12; //u16VActive [10:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = ((pstChnAttr->u16VActive >> 8)&0x07) << 5;
	i2cMask = 0xE0;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x16; //u16VActive [10:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = pstChnAttr->u16VActive & 0xFF;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	//uint16_t u16VDelay; //b[10:0]
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x11; //u16VDelay [10:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = ((pstChnAttr->u16VDelay >> 8)&0x07) << 5;
	i2cMask = 0xE0;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x15; //u16VDelay [10:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = pstChnAttr->u16VDelay & 0xFF;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	if(pstChnAttr->u16HSCLRatio != 0)
	{
		page = PR1000_VDEC_PAGE(prChn);
		i2cReg = 0x29; //u16HSCLRatio [15:8]
		i2cData = pstChnAttr->u16HSCLRatio>>8;
		if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		i2cReg = 0x2A; //u16HSCLRatio [7:0]
		i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
		i2cData = pstChnAttr->u16HSCLRatio & 0xFF;
		if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}

	return(ret);
}

int PR1000_VID_GetChnAttr(const int fd, const _stPortChSel *pstPortChSel, _stChnAttr *pstChnAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstChnAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	//uint16_t u16HActive; //b[12:0]
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x12; //u16HActive [12:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x1F;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstChnAttr->u16HActive = (i2cData&0x1F)<<8;

	i2cReg = 0x14; //u16HActive [12:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstChnAttr->u16HActive |= i2cData;

	//uint16_t u16HDelay; //b[12:0]
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x11; //u16HDelay [12:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x1F;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstChnAttr->u16HDelay = (i2cData&0x1F)<<8;

	i2cReg = 0x13; //u16HDelay [12:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstChnAttr->u16HDelay |= i2cData;

	//uint16_t u16VActive; //b[10:0]
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x12; //u16VActive [10:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0xE0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstChnAttr->u16VActive = (i2cData>>5)<<8;

	i2cReg = 0x16; //u16VActive [10:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstChnAttr->u16VActive |= i2cData;

	//uint16_t u16VDelay; //b[10:0]
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x11; //u16VDelay [10:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x1F;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstChnAttr->u16VDelay = (i2cData>>5)<<8;

	i2cReg = 0x15; //u16VDelay [10:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstChnAttr->u16VDelay |= i2cData;

	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x29; //u16HSCLRatio [15:8]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstChnAttr->u16HSCLRatio = i2cData<<8;

	i2cReg = 0x2A; //u16HSCLRatio [7:0]
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstChnAttr->u16HSCLRatio |= i2cData;

	return(ret);
}

#ifndef DONT_SUPPORT_VID_ENHANCEMENT
int PR1000_VID_SetCscAttr(const int fd, const _stPortChSel *pstPortChSel, const _stCscAttr *pstCscAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstCscAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	page = PR1000_VDEC_PAGE(prChn);

	i2cReg = 0x24; //u8CbGain;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = pstCscAttr->u8CbGain;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x25; //u8CrGain;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = pstCscAttr->u8CrGain;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x26; //u8CbOffset;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = pstCscAttr->u8CbOffset;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x27; //u8CrOffset;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = pstCscAttr->u8CrOffset;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VID_GetCscAttr(const int fd, const _stPortChSel *pstPortChSel, _stCscAttr *pstCscAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstCscAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	page = PR1000_VDEC_PAGE(prChn);

	i2cReg = 0x24; //u8CbGain;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstCscAttr->u8CbGain = i2cData;

	i2cReg = 0x25; //u8CrGain;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstCscAttr->u8CrGain = i2cData;

	i2cReg = 0x26; //u8CbOffset;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstCscAttr->u8CbOffset = i2cData;

	i2cReg = 0x27; //u8CrOffset;
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstCscAttr->u8CrOffset = i2cData;

	return(ret);
}

int PR1000_VID_SetContrast(const int fd, const _stPortChSel *pstPortChSel, const _stContrast *pstContrast)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstContrast == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	page = PR1000_VDEC_PAGE(prChn);

	i2cReg = 0x20; //Contrast
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = pstContrast->u8Contrast;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VID_GetContrast(const int fd, const _stPortChSel *pstPortChSel, _stContrast *pstContrast)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstContrast == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	page = PR1000_VDEC_PAGE(prChn);

	i2cReg = 0x20; //Contrast
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstContrast->u8Contrast = i2cData;

	return(ret);
}

int PR1000_VID_SetBright(const int fd, const _stPortChSel *pstPortChSel, const _stBright *pstBright)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstBright == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	page = PR1000_VDEC_PAGE(prChn);

	i2cReg = 0x21; //Bright
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = pstBright->u8Bright;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VID_GetBright(const int fd, const _stPortChSel *pstPortChSel, _stBright *pstBright)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstBright == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	page = PR1000_VDEC_PAGE(prChn);

	i2cReg = 0x21; //Bright
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstBright->u8Bright = i2cData;

	return(ret);
}

int PR1000_VID_SetSaturation(const int fd, const _stPortChSel *pstPortChSel, const _stSaturation *pstSaturation)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstSaturation == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	page = PR1000_VDEC_PAGE(prChn);

	i2cReg = 0x22; //Saturation
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = pstSaturation->u8Saturation;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VID_GetSaturation(const int fd, const _stPortChSel *pstPortChSel, _stSaturation *pstSaturation)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstSaturation == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	page = PR1000_VDEC_PAGE(prChn);

	i2cReg = 0x22; //Saturation
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstSaturation->u8Saturation = i2cData;

	return(ret);
}

int PR1000_VID_SetHue(const int fd, const _stPortChSel *pstPortChSel, const _stHue *pstHue)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstHue == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	page = PR1000_VDEC_PAGE(prChn);

	i2cReg = 0x23; //Hue
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = pstHue->u8Hue;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VID_GetHue(const int fd, const _stPortChSel *pstPortChSel, _stHue *pstHue)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstHue == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	page = PR1000_VDEC_PAGE(prChn);

	i2cReg = 0x23; //Hue
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstHue->u8Hue = i2cData;

	return(ret);
}

int PR1000_VID_SetSharpness(const int fd, const _stPortChSel *pstPortChSel, const _stSharpness *pstSharpness)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstSharpness == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	page = PR1000_VDEC_PAGE(prChn);

	i2cReg = 0x39; //Sharpness
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x0F;
	i2cData = pstSharpness->u8Sharpness & 0xF;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VID_GetSharpness(const int fd, const _stPortChSel *pstPortChSel, _stSharpness *pstSharpness)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstSharpness == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	page = PR1000_VDEC_PAGE(prChn);

	i2cReg = 0x39; //Sharpness
	i2cReg += PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x0F;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}

	pstSharpness->u8Sharpness = i2cData&0xF;

	return(ret);
}

int PR1000_VID_SetBlank(const int fd, const _stPortChSel *pstPortChSel, const int bEnable, const int blankColor)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 
	int page;

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	/* blank */
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x00 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x03; 
	i2cData = ((bEnable&1)<<1) | (blankColor&1); //0:normal video output, 1:manual background mode
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}


#endif // DONT_SUPPORT_VID_ENHANCEMENT

int PR1000_VID_SetOutFormatAttr(const int fd, const _stOutFormatAttr *pstOutFormatAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t i2cSlaveAddr; 
	uint8_t ch; 
	uint8_t page; 

	uint8_t chip = 0;
	uint8_t muxChCnt, b16bit, resol; 
	uint8_t datarate;
	uint8_t outfmt_bt656;
	_PR1000_REG_TABLE_VIDOUT_FORMAT *prTbl = NULL;
	int (*pPortChSel)[4][2];
	int *pPortChClkPhase = NULL;

	DbgString("Set Outformat attr.\n");

	if(pstOutFormatAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	muxChCnt = pstOutFormatAttr->muxChCnt&0x3;
	b16bit = pstOutFormatAttr->b16bit&0x1;
	resol =pstOutFormatAttr->resol&0x3;
	datarate =pstOutFormatAttr->datarate&0x3;
	outfmt_bt656 = pstOutFormatAttr->outfmt_bt656&0x1;

	prTbl = (_PR1000_REG_TABLE_VIDOUT_FORMAT *)&pr1000_reg_table_vidout_format[muxChCnt][b16bit][resol];
	if(prTbl->vdck_out_phase == 0xFF) // not support
	{
		ErrorString("Don't support type.\n");
		return(-1);
	}

	for(chip = 0; chip < PR1000_CHIP_COUNT; chip++)
	{
		i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

		page = PR1000_REG_PAGE_COMMON;
		i2cReg = 0xEF;
		i2cMask = 0x0C; i2cData = muxChCnt<<2;
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		page = PR1000_REG_PAGE_COMMON;
		i2cReg = 0xEF;
		i2cMask = 0x10; i2cData = b16bit<<4;
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		page = PR1000_REG_PAGE_COMMON;
		i2cReg = 0xEF;
		i2cMask = 0x03; i2cData = datarate;
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		if(datarate == 2) //144Mhz
		{
			page = PR1000_REG_PAGE_COMMON;
			i2cReg = 0xE7;
			i2cMask = 0x1F; i2cData = 0x10;
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}
		else if(datarate == 3) //108Mhz
		{
			page = PR1000_REG_PAGE_COMMON;
			i2cReg = 0xE7;
			i2cMask = 0x1F; i2cData = 0x1C;
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}

		page = PR1000_REG_PAGE_COMMON;
		i2cReg = 0xEF;
		i2cMask = 0x20; i2cData = outfmt_bt656<<5;
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		if( (muxChCnt == PR1000_VID_OUTF_MUX_2CH) && (b16bit == 0) && (datarate == 0) ) //mux_2ch & 8bit & rate 0 
		{
			page = PR1000_REG_PAGE_COMMON;
			i2cReg = 0xEF;
			i2cMask = 0x80; i2cData = 0x80;
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}
		else if( (muxChCnt == PR1000_VID_OUTF_MUX_4CH) && (b16bit == 0) && (datarate >= 1) ) //mux_4ch & 8bit & rate >= 1
		{
			page = PR1000_REG_PAGE_COMMON;
			i2cReg = 0xEF;
			i2cMask = 0x80; i2cData = 0x80;
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}

		for(ch = 0; ch < DEF_PR1000_MAX_CHN; ch++)
		{
			page = PR1000_REG_PAGE_COMMON;
			i2cReg = 0xF2 + (ch*3);
			i2cMask = 0x0F; i2cData = prTbl->vdck_out_phase&0xF;
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			page = PR1000_VDEC_PAGE(ch);
			i2cReg = 0x4F + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(ch);
			if( muxChCnt == PR1000_VID_OUTF_MUX_1CH)i2cData = 0x00;
			else i2cData = 0x0C;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}

		pPortChClkPhase = (int *)&pstOutFormatAttr->portChClkPhase[chip];
		for(ch = 0; ch < DEF_PR1000_MAX_CHN; ch++)
		{
			Print("CHSEL Chip:%d, Ch:%d, ClkPhase:0x%x\n",  chip COMMA ch COMMA pPortChClkPhase[ch]);

			page = PR1000_REG_PAGE_COMMON;
			i2cReg = 0xE8 + (ch/2);
			i2cMask = 0xF<<(4*(ch%2)); 
			i2cData = (pPortChClkPhase[ch]&0xF)<<(4*(ch%2)); 
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}

		if(muxChCnt == PR1000_VID_OUTF_MUX_1CH)
		{
			pPortChSel = (int (*)[4][2])&PR1000_VID_INOUT_MUX1CH_CHDEF[b16bit][chip];
		}
#ifdef __LINUX_SYSTEM__
		else if(muxChCnt == PR1000_VID_OUTF_MUX_2CH)
		{
			pPortChSel = (int (*)[4][2])&PR1000_VID_INOUT_MUX2CH_CHDEF[b16bit][chip];
		}
		else if(muxChCnt == PR1000_VID_OUTF_MUX_4CH)
		{
			pPortChSel = (int (*)[4][2])&PR1000_VID_INOUT_MUX4CH_CHDEF[b16bit][chip];
		}
#endif // __LINUX_SYSTEM__
		else
		{
			ErrorString("Invalid muxch config\n");
			break;
		}

		/* channel id */
		{/*{{{*/
			if(muxChCnt == PR1000_VID_OUTF_MUX_1CH) //mux_1ch
			{
				page = PR1000_REG_PAGE_COMMON;
				i2cReg = 0xEE;
				i2cData = 0x00;
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}
			else if(muxChCnt == PR1000_VID_OUTF_MUX_2CH) //mux_2ch
			{
				int chipNum0 = -1, chipNum1 = -1, chipNum2 = -1, chipNum3 = -1;
				page = PR1000_REG_PAGE_COMMON;
				i2cReg = 0xEE;
				for(ch = 0; ch < DEF_PR1000_MAX_CHN; ch++)
				{
					if( (pPortChSel[ch][0][0] != -1) && (pPortChSel[ch][1][0] != -1) ) //vinx
					{
						if(chipNum0 < 0) // chipNum0, chipNum1 pair.
						{
							chipNum0 = pPortChSel[ch][0][0]; //vinx
							chipNum1 = pPortChSel[ch][1][0]; //vinx
							if(chipNum0 <= chipNum1) { chipNum0 = 0; chipNum1 = 1;}
							else if(chipNum0 > chipNum1) { chipNum0 = 1; chipNum1 = 0;}
						}
						else
						{
							if(chipNum2 < 0)
							{
								chipNum2 = pPortChSel[ch][0][0]; //vinx
								chipNum3 = pPortChSel[ch][1][0]; //vinx
								if(chipNum2 <= chipNum3) { chipNum2 = 0; chipNum3 = 1;}
								else if(chipNum2 > chipNum3) { chipNum2 = 1; chipNum3 = 0;}
							}
							else
							{
								ErrorString("Invalid channel define.\n");
							}
						}
					}
				}
				/* if 1channel define. forcely define. */
				if( (chipNum0 == -1) && (chipNum1 == -1) ) { chipNum0 = 0; chipNum1 = 1; }
				if( (chipNum2 == -1) && (chipNum3 == -1) ) { chipNum2 = 0; chipNum3 = 1; }

				i2cData = ((chipNum3&0x3)<<6)|((chipNum2&0x3)<<4)|((chipNum1&0x3)<<2)|((chipNum0&0x3)<<0);
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}
			else if(muxChCnt == PR1000_VID_OUTF_MUX_4CH) //mux_4ch
			{
				int chipNum0 = -1, chipNum1 = -1, chipNum2 = -1, chipNum3 = -1;
				page = PR1000_REG_PAGE_COMMON;
				i2cReg = 0xEE;
				for(ch = 0; ch < DEF_PR1000_MAX_CHN; ch++)
				{
					if( (pPortChSel[ch][0][0] != -1) && (pPortChSel[ch][1][0] != -1) && (pPortChSel[ch][2][0] != -1) && (pPortChSel[ch][3][0] != -1) ) //vinx
					{
						if(chipNum0 < 0)
						{
							chipNum0 = pPortChSel[ch][0][0]; //vinx
							chipNum1 = pPortChSel[ch][1][0]; //vinx
							chipNum2 = pPortChSel[ch][2][0]; //vinx
							chipNum3 = pPortChSel[ch][3][0]; //vinx
							chipNum0 /=2; chipNum0 &=0x3;
							chipNum1 /=2; chipNum1 &=0x3;
							chipNum2 /=2; chipNum2 &=0x3;
							chipNum3 /=2; chipNum3 &=0x3;
							break;
						}
					}
				}
				/* if 1channel define. forcely define. */
				if( (chipNum0 == -1) && (chipNum1 == -1) && (chipNum2 == -1) && (chipNum3 == -1) ) 
				{
					chipNum0 = 0; chipNum1 = 1; chipNum2 = 2; chipNum3 = 3;
				}

				i2cData = ((chipNum3&0x3)<<6)|((chipNum2&0x3)<<4)|((chipNum1&0x3)<<2)|((chipNum0&0x3)<<0);
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}

			}
		}/*}}}*/

		/* vdout en/disable */
		{/*{{{*/
			for(ch = 0; ch < DEF_PR1000_MAX_CHN; ch++)
			{
				if(b16bit)
				{/*{{{*/
					if( (pPortChSel[ch][0][0] == -1) && (pPortChSel[ch][1][0] == -1) && (pPortChSel[ch][2][0] == -1) && (pPortChSel[ch][3][0] == -1) ) //vdout
					{
						Print("CHSEL Chip:%d, Vd:%d, (ABCD:(%d,%d,%d,%d))->[N/A]\n",  chip COMMA ch COMMA 
								pPortChSel[ch][0][1] COMMA pPortChSel[ch][1][1] COMMA pPortChSel[ch][2][1] COMMA pPortChSel[ch][3][1]);

						Print("Disable Vid out. Chip:%d, Vd:%d\n", chip COMMA ch);
						page = PR1000_REG_PAGE_COMMON;
						i2cReg = 0xD1 + (ch);
						i2cData = 0xFF;
						if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
						page = PR1000_REG_PAGE_COMMON;
						i2cReg = 0xD6;
						i2cMask = 0x1<<ch; i2cData = 0x1<<ch;
						if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
					}
					else
					{
						Print("Enable Vid out. Chip:%d, Vd:%d\n", chip COMMA ch);
						page = PR1000_REG_PAGE_COMMON;
						i2cReg = 0xD1 + (ch);
						i2cData = 0x00;
						if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
						page = PR1000_REG_PAGE_COMMON;
						i2cReg = 0xD6;
						i2cMask = 0x1<<ch; i2cData = 0x0<<ch;
						if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}

						page = PR1000_REG_PAGE_COMMON;
						i2cReg = 0xF0 + (ch*3);
						i2cMask = 0x77; i2cData = ((pPortChSel[ch][1][0]&0x7)<<4)|((pPortChSel[ch][0][0]&0x7)<<0); //BA
						if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
						page = PR1000_REG_PAGE_COMMON;
						i2cReg = 0xF0 + (ch*3)+1;
						i2cMask = 0x77; i2cData = ((pPortChSel[ch][3][0]&0x7)<<4)|((pPortChSel[ch][2][0]&0x7)<<0); //DC
						if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}

						Print("CHSEL Chip:%d, Vd:%d, (ABCD:(%d,%d,%d,%d))\n",  chip COMMA ch COMMA 
								pPortChSel[ch][0][0] COMMA pPortChSel[ch][1][0] COMMA pPortChSel[ch][2][0] COMMA pPortChSel[ch][3][0]);

					}
				}/*}}}*/
				else //8bit
				{/*{{{*/

					if( (pPortChSel[ch][0][1] == -1) && (pPortChSel[ch][1][1] == -1) && (pPortChSel[ch][2][1] == -1) && (pPortChSel[ch][3][1] == -1) ) //vdout
					{
						Print("CHSEL Chip:%d, Vd:%d, (ABCD:(%d,%d,%d,%d))->[N/A]\n",  chip COMMA ch COMMA 
								pPortChSel[ch][0][1] COMMA pPortChSel[ch][1][1] COMMA pPortChSel[ch][2][1] COMMA pPortChSel[ch][3][1]);

						Print("Disable Vid out. Chip:%d, Vd:%d\n", chip COMMA ch);
						page = PR1000_REG_PAGE_COMMON;
						i2cReg = 0xD1 + (ch);
						i2cData = 0xFF;
						if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
						page = PR1000_REG_PAGE_COMMON;
						i2cReg = 0xD6;
						i2cMask = 0x1<<ch; i2cData = 0x1<<ch;
						if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
					}
					else
					{
						Print("Enable Vid out. Chip:%d, Vd:%d\n", chip COMMA ch);
						page = PR1000_REG_PAGE_COMMON;
						i2cReg = 0xD1 + (ch);
						i2cData = 0x00;
						if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
						page = PR1000_REG_PAGE_COMMON;
						i2cReg = 0xD6;
						i2cMask = 0x1<<ch; i2cData = 0x0<<ch;
						if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}

						page = PR1000_REG_PAGE_COMMON;
						i2cReg = 0xF0 + (ch*3);
						i2cMask = 0x77; i2cData = ((pPortChSel[ch][1][0]&0x7)<<4)|((pPortChSel[ch][0][0]&0x7)<<0); //BA
						if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
						page = PR1000_REG_PAGE_COMMON;
						i2cReg = 0xF0 + (ch*3)+1;
						i2cMask = 0x77; i2cData = ((pPortChSel[ch][3][0]&0x7)<<4)|((pPortChSel[ch][2][0]&0x7)<<0); //DC
						if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}

						Print("CHSEL Chip:%d, Vd:%d, (ABCD:(%d,%d,%d,%d))\n",  chip COMMA ch COMMA 
								pPortChSel[ch][0][0] COMMA pPortChSel[ch][1][0] COMMA pPortChSel[ch][2][0] COMMA pPortChSel[ch][3][0]);
					}
				}/*}}}*/
			}
		}/*}}}*/

		/* vdec/ptz out en/disable */
		{/*{{{*/
			int i = 0, bFind = 0, vinCh = 0, vdecCh = 0;

			for(vinCh = 0; vinCh < 8; vinCh+=2)
			{
				bFind = 0;
				vdecCh = vinCh/2;
				for(ch = 0; ch < DEF_PR1000_MAX_CHN; ch++)
				{
					for(i = 0; i < DEF_PR1000_MAX_CHN; i++)
					{
						if(pPortChSel[ch][i][0] == vinCh) 
						{
							bFind = 1;
							break;
						}
					}
					if(bFind)
					{
						Dbg("Find vinCh:%d, portSet:%d\n", vinCh COMMA pPortChSel[ch][i][0]);
						break;
					}
				}
				if(bFind)
				{
					Dbg("Enable VDEC. Chip:%d, vdecCh:%d\n", chip COMMA vdecCh);
					page = PR1000_VDEC_PAGE(vdecCh);
					i2cReg = 0x68 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(vdecCh);
					i2cMask = 0x10; 
					i2cData = 0x00;
					if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}

					Dbg("Enable VDEC clk out. Chip:%d, vdecCh:%d\n", chip COMMA vdecCh);
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0xEC + (vdecCh/2);
					i2cMask = 0x8<<((vdecCh%2)*4); i2cData = 0x0<<((vdecCh%2)*4);
					if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}

					Dbg("Enable Ptz out. Chip:%d, vdecCh:%d\n", chip COMMA vdecCh);
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0xD0;
					i2cMask = 0x1<<(vdecCh+4); i2cData = 0x0<<(vdecCh+4);
					if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}

					Print("CHSEL Chip:%d, Vdec/pzt:%d, Enable\n",  chip COMMA vdecCh);
				}
				else
				{
					Print("CHSEL Chip:%d, Vdec/ptz:%d, Disable\n",  chip COMMA vdecCh);

					Dbg("Disable VDEC. Chip:%d, vdecCh:%d\n", chip COMMA vdecCh);
					page = PR1000_VDEC_PAGE(vdecCh);
					i2cReg = 0x68 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(vdecCh);
					i2cMask = 0x10; 
					i2cData = 0x10;
					if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}

					Dbg("Disable VDEC clk out. Chip:%d, vdecCh:%d\n", chip COMMA vdecCh);
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0xEC + (vdecCh/2);
					i2cMask = 0x8<<((vdecCh%2)*4); i2cData = 0x8<<((vdecCh%2)*4);
					if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}

					Dbg("Disable Ptz out. Chip:%d, vdecCh:%d\n", chip COMMA vdecCh);
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0xD0;
					i2cMask = 0x1<<(vdecCh+4); i2cData = 0x1<<(vdecCh+4);
					if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}
				}
			}
		}/*}}}*/

		/* audio & alink en/disable */
		{/*{{{*/

			if(PR1000_AUDIO_ALINK_EN == 0)
			{
				Print("CHSEL Chip:%d, Audio&Alink Disable\n",  chip);

				page = PR1000_REG_PAGE_COMMON;
				i2cReg = 0xEA;
				i2cData = 0x80;
				i2cMask = 0x80;
				if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
				page = PR1000_REG_PAGE_COMMON;
				i2cReg = 0xEB;
				i2cData = 0xE0;
				i2cMask = 0xE0;
				if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}
		}/*}}}*/
	}

	return(ret);
}

int PR1000_VID_SetOutChn(const int fd, const uint8_t chip, const _stOutChn *pstOutChn)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t i2cSlaveAddr; 
	uint8_t ch; 
	uint8_t page; 
	int portA, portB, portC, portD;

	DbgString("Set Outchn attr.\n");
	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	if(pstOutChn == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	ch = pstOutChn->prChn;
	if( (pstOutChn->portChSel[0] == -1) && (pstOutChn->portChSel[1] == -1) 
			&& (pstOutChn->portChSel[2] == -1) && (pstOutChn->portChSel[3] == -1) )
	{
		Print("CHSEL chip:%d, ch:%d, (ABCD:(%d%d%d%d))[N/A]\n", chip COMMA ch COMMA 
				pstOutChn->portChSel[0] COMMA pstOutChn->portChSel[1] COMMA pstOutChn->portChSel[2] COMMA pstOutChn->portChSel[3]);

		Print("Disable Vid out. Chip:%d, ch:%d\n", chip COMMA ch);
		page = PR1000_REG_PAGE_COMMON;
		i2cReg = 0xD1 + (ch);
		i2cData = 0xFF;
		if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
		page = PR1000_REG_PAGE_COMMON;
		i2cReg = 0xD6;
		i2cMask = 0x1<<ch; i2cData = 0x1<<ch;
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

	}
	else
	{
		portA = pstOutChn->portChSel[0];
		portB = pstOutChn->portChSel[1];
		portC = pstOutChn->portChSel[2];
		portD = pstOutChn->portChSel[3];
		portA = (portA < 0) ? 0: portA;
		portB = (portB < 0) ? 0: portB;
		portC = (portC < 0) ? 0: portC;
		portD = (portD < 0) ? 0: portD;

		Print("Enable Vid out. Chip:%d, ch:%d\n", chip COMMA ch);
		page = PR1000_REG_PAGE_COMMON;
		i2cReg = 0xD1 + (ch);
		i2cData = 0x00;
		if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
		page = PR1000_REG_PAGE_COMMON;
		i2cReg = 0xD6;
		i2cMask = 0x1<<ch; i2cData = 0x0<<ch;
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		page = PR1000_REG_PAGE_COMMON;
		i2cReg = 0xF0 + (ch*3);
		i2cMask = 0x77; i2cData = ((portB&0x7)<<4)|((portA&0x7)<<0); //BA
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
		page = PR1000_REG_PAGE_COMMON;
		i2cReg = 0xF0 + (ch*3)+1;
		i2cMask = 0x77; i2cData = ((portD&0x7)<<4)|((portC&0x7)<<0); //DC
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		Print("CHSEL Chip:%d, ch:%d, (ABCD:(%d%d%d%d))\n",  chip COMMA ch COMMA portA COMMA portB COMMA portC COMMA portD);
	}

	return(ret);
}

int PR1000_IfmtToStdResol(const int fd, const uint8_t std, const uint8_t ref, const uint8_t res, enum _pr1000_table_format *pFormat, enum _pr1000_table_inresol *pResol)
{
	int ret = -1;

	if( (pFormat == NULL) || (pResol == NULL) )
	{
		return(-1);
	}

	if(res == PR1000_DET_IFMT_RES_480i)
	{
		*pFormat =  pr1000_format_SD720; 	//Can select: SD720/SD960
		*pResol = pr1000_inresol_ntsc; 	
	}
	else if(res == PR1000_DET_IFMT_RES_576i) //SD576
	{
		*pFormat =  pr1000_format_SD720; 	//Can select: SD720/SD960
		*pResol = pr1000_inresol_pal; 	
	}
	else if(res == PR1000_DET_IFMT_RES_720p)
	{
		if(std == PR1000_DET_IFMT_STD_CVI) *pFormat = pr1000_format_CVI;
		else if(std == PR1000_DET_IFMT_STD_HDA) *pFormat = pr1000_format_HDA;
		else if(std == PR1000_DET_IFMT_STD_HDT) *pFormat = pr1000_format_HDT;
#ifndef DONT_SUPPORT_STD_PVI
		else if(std == PR1000_DET_IFMT_STD_PVI) *pFormat = pr1000_format_PVI;
#endif // DONT_SUPPORT_STD_PVI
		else { return(-1);}

		if(ref == PR1000_DET_IFMT_REF_25) *pResol = pr1000_inresol_1280x720p25;
		else if(ref == PR1000_DET_IFMT_REF_30) *pResol = pr1000_inresol_1280x720p30;
		else if(ref == PR1000_DET_IFMT_REF_50) *pResol = pr1000_inresol_1280x720p50;
		else if(ref == PR1000_DET_IFMT_REF_60) *pResol = pr1000_inresol_1280x720p60;
		else { return(-1);}
	}
	else if(res == PR1000_DET_IFMT_RES_1080p)
	{
		if(std == PR1000_DET_IFMT_STD_CVI) *pFormat = pr1000_format_CVI;
		else if(std == PR1000_DET_IFMT_STD_HDA) *pFormat = pr1000_format_HDA;
		else if(std == PR1000_DET_IFMT_STD_HDT) *pFormat = pr1000_format_HDT;
#ifndef DONT_SUPPORT_STD_PVI
		else if(std == PR1000_DET_IFMT_STD_PVI) *pFormat = pr1000_format_PVI;
#endif // DONT_SUPPORT_STD_PVI
		else { return(-1);}

		if(ref == PR1000_DET_IFMT_REF_25) *pResol = pr1000_inresol_1920x1080p25;
		else if(ref == PR1000_DET_IFMT_REF_30) *pResol = pr1000_inresol_1920x1080p30;
		else { return(-1);}
	}
	else
	{
		return(-1);
	}

	ret = 0;

	return(ret);
}

int PR1000_VID_HideEQingDisplay(const int fd, const _stPortChSel *pstPortChSel)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 
	int page;

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	Dbg("Set eqing display channel[mapCh:%d]\n", mapChn);

	{
		/* blank manually current blank color. */
		page = PR1000_VDEC_PAGE(prChn);
		i2cReg = 0x00 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
		i2cMask = 0x02; 
		i2cData = ((1)<<1); //0:normal video output, 1:manual background mode
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}

	return(ret);
}

#ifndef DONT_SUPPORT_AUD_ALINK
//////////////////////////////////////////////////////////////////////////////////////////////////////
/*** PR1000 - AUDIO control. ***/
//////////////////////////////////////////////////////////////////////////////////////////////////////
int PR1000_AUD_Enable(const int fd, const uint8_t chip, const int bEnable)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	DbgString("Audio Enable.\n");
	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	if(bEnable)
	{
		DbgString("Enable Audio.\n");
		i2cMask = 0x01; i2cData = 0x01;
	}
	else
	{
		DbgString("Disable Audio.\n");
		i2cMask = 0x01; i2cData = 0x00;
	}
	i2cReg = 0x36;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_AUD_SetAiGain(const int fd, const uint8_t chip, const _stAUDAiGain *pstAUDAiGain)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 
	uint8_t prChn; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	if( pstAUDAiGain == NULL )
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	prChn = pstAUDAiGain->prChn;
	if( prChn > 3 )
	{
		ErrorString("Invalid prChn.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	switch(prChn)
	{
		case 0:
		{
			i2cMask = 0xF0;
			i2cData = (pstAUDAiGain->u8Gain&0xF)<<4;
			i2cReg = 0x02;
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}
		break;
		case 1:
		{
			i2cMask = 0x0F;
			i2cData = (pstAUDAiGain->u8Gain&0xF);
			i2cReg = 0x03;
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}
		break;
		case 2:
		{
			i2cMask = 0xF0;
			i2cData = (pstAUDAiGain->u8Gain&0xF)<<4;
			i2cReg = 0x03;
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}
		break;
		case 3:
		{
			i2cMask = 0x0F;
			i2cData = (pstAUDAiGain->u8Gain&0xF);
			i2cReg = 0x04;
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}
		break;
		default:
		{
			return(0);
		}
		break;
	}

	return(ret);
}

int PR1000_AUD_GetAiGain(const int fd, const uint8_t chip, _stAUDAiGain *pstAUDAiGain)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 
	uint8_t prChn; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	if( pstAUDAiGain == NULL )
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	prChn = pstAUDAiGain->prChn;
	if( prChn > 3 )
	{
		ErrorString("Invalid prChn.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	switch(prChn)
	{
		case 0:
		{
			i2cMask = 0xF0;
			i2cReg = 0x02;
			if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, &i2cData)) < 0)
			{
				ErrorString("Read reg.\n");
				return(-1);
			}
			pstAUDAiGain->u8Gain = i2cData>>4;
		}
		break;
		case 1:
		{
			i2cMask = 0x0F;
			i2cReg = 0x03;
			if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, &i2cData)) < 0)
			{
				ErrorString("Read reg.\n");
				return(-1);
			}
			pstAUDAiGain->u8Gain = i2cData&0xF;
		}
		break;
		case 2:
		{
			i2cMask = 0xF0;
			i2cReg = 0x03;
			if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, &i2cData)) < 0)
			{
				ErrorString("Read reg.\n");
				return(-1);
			}
			pstAUDAiGain->u8Gain = i2cData>>4;
		}
		break;
		case 3:
		{
			i2cMask = 0x0F;
			i2cReg = 0x04;
			if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, &i2cData)) < 0)
			{
				ErrorString("Read reg.\n");
				return(-1);
			}
			pstAUDAiGain->u8Gain = i2cData&0xF;
		}
		break;
		default:
		{
			return(-1);
		}
		break;
	}

	return(ret);
}

int PR1000_AUD_SetDacGain(const int fd, const uint8_t chip, const _stAUDDacGain *pstAUDDacGain)
{/*{{{*/
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}
	if( pstAUDDacGain == NULL )
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cMask = 0xF;
	i2cData = pstAUDDacGain->u8Gain&0xF;
	i2cReg = 0x0A;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}/*}}}*/

int PR1000_AUD_GetDacGain(const int fd, const uint8_t chip, _stAUDDacGain *pstAUDDacGain)
{/*{{{*/
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}
	if( pstAUDDacGain == NULL )
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cMask = 0xF;
	i2cData = 0;
	i2cReg = 0x0A;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDDacGain->u8Gain = i2cData&0xF;

	return(ret);
}/*}}}*/

int PR1000_AUD_SetMixMode(const int fd, const uint8_t chip, const _stAUDMixMode *pstAUDMixMode)
{/*{{{*/
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}
	if( pstAUDMixMode == NULL )
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cMask = 0x1;
	i2cData = pstAUDMixMode->bMixMode;
	i2cReg = 0x11;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}/*}}}*/

int PR1000_AUD_GetMixMode(const int fd, const uint8_t chip, _stAUDMixMode *pstAUDMixMode)
{/*{{{*/
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}
	if( pstAUDMixMode == NULL )
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cMask = 0x1;
	i2cData = 0;
	i2cReg = 0x11;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDMixMode->bMixMode = i2cData&1;

	return(ret);
}/*}}}*/

int PR1000_AUD_SetRecAttr(const int fd, const uint8_t chip, const _stAUDRecAttr *pstAUDRecAttr)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cMask = 0xFF;
	i2cData = 0;
	i2cData |= (pstAUDRecAttr->bMaster & 1) << 7;
	i2cData |= (pstAUDRecAttr->bDSP & 1) << 6;
	i2cData |= (pstAUDRecAttr->bDSP_User & 1) << 5;
	i2cData |= (pstAUDRecAttr->bClkRise & 1) << 4;
	i2cData |= (pstAUDRecAttr->format & 0x3) << 2;
	i2cData |= (pstAUDRecAttr->sampleRate & 0x3) << 0;

	i2cReg = 0x0C;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cMask = 0xF0;
	i2cData = 0;
	i2cData |= (pstAUDRecAttr->b8bits & 1) << 7;
	i2cData |= (pstAUDRecAttr->bitRate & 0x7) << 4;

	i2cReg = 0x0D;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	/* set io port direction */
	if(pstAUDRecAttr->bMaster)
	{
		i2cMask = 0x07; //DATA/SYNC/CLK
		i2cData = 0x00; //0:output, 1:input
		i2cReg = 0xD5;
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_COMMON, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}
	else
	{
		i2cMask = 0x07; //DATA/SYNC/CLK
		i2cData = 0x03; //0:output, 1:input
		i2cReg = 0xD5;
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_COMMON, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}

	return(ret);
}

int PR1000_AUD_GetRecAttr(const int fd, const uint8_t chip, _stAUDRecAttr *pstAUDRecAttr)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}
	if( pstAUDRecAttr == NULL )
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cMask = 0xFF;
	i2cData = 0;
	i2cReg = 0x0C;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDRecAttr->bMaster = (i2cData>>7)&1;
	pstAUDRecAttr->bDSP = (i2cData>>6)&1;
	pstAUDRecAttr->bDSP_User = (i2cData>>5)&1;
	pstAUDRecAttr->bClkRise = (i2cData>>4)&1;
	pstAUDRecAttr->format = (i2cData>>2)&0x3;
	pstAUDRecAttr->sampleRate = (i2cData)&0x3;

	i2cMask = 0xF0;
	i2cData = 0;
	i2cReg = 0x0D;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDRecAttr->b8bits = (i2cData>>7)&1;
	pstAUDRecAttr->bitRate = (i2cData>>4)&0x7;

	return(ret);
}

int PR1000_AUD_SetRecMute(const int fd, const uint8_t chip, const uint8_t ch, const int bEnable)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 
	uint8_t chn; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	if(ch < 8)
	{
		chn = ch;
		i2cReg = 0x0F;
	}
	else
	{
		chn = ch - 8;
		i2cReg = 0x0E;
	}
	i2cMask = (1<<chn);
	if(bEnable) i2cData = (1<<chn);
	else i2cData = 0;

	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_AUD_SetPbAttr(const int fd, const uint8_t chip, const _stAUDPbAttr *pstAUDPbAttr)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cMask = 0x01F;
	i2cData = 0;
	i2cData |= (pstAUDPbAttr->b8bits & 1) << 4;
	i2cData |= (pstAUDPbAttr->bMaster & 1) << 3;
	i2cData |= (pstAUDPbAttr->bRightChn & 1) << 2;
	i2cData |= (pstAUDPbAttr->format & 0x3) << 0;

	i2cReg = 0x08;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cMask = 0xFF;
	i2cData = 0;
	i2cData |= (pstAUDPbAttr->b8bitLow & 1) << 7;
	i2cData |= (pstAUDPbAttr->bDSP & 1) << 6;
	i2cData |= (pstAUDPbAttr->bDSP_User & 1) << 5;
	i2cData |= (pstAUDPbAttr->bitRate & 0x7) << 2;
	i2cData |= (pstAUDPbAttr->sampleRate & 0x3) << 0;

	i2cReg = 0x09;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	/* set io port direction */
	if(pstAUDPbAttr->bMaster)
	{
		i2cMask = 0x38; //DATA/SYNC/CLK
		i2cData = 0x20; //0:output, 1:input
		i2cReg = 0xD5;
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_COMMON, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}
	else
	{
		i2cMask = 0x38; //DATA/SYNC/CLK
		i2cData = 0x38; //0:output, 1:input
		i2cReg = 0xD5;
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_COMMON, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}


	return(ret);
}

int PR1000_AUD_GetPbAttr(const int fd, const uint8_t chip, _stAUDPbAttr *pstAUDPbAttr)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	if( pstAUDPbAttr == NULL )
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cMask = 0x01F;
	i2cData = 0;
	i2cReg = 0x08;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDPbAttr->b8bits = (i2cData>>4)&1;
	pstAUDPbAttr->bMaster = (i2cData>>3)&1;
	pstAUDPbAttr->bRightChn = (i2cData>>2)&1;
	pstAUDPbAttr->format = i2cData&0x3;

	i2cMask = 0xFF;
	i2cData = 0;
	i2cReg = 0x09;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDPbAttr->b8bitLow = (i2cData>>7)&1;
	pstAUDPbAttr->bDSP = (i2cData>>6)&1;
	pstAUDPbAttr->bDSP_User = (i2cData>>5)&1;
	pstAUDPbAttr->bitRate = (i2cData>>2)&0x7;
	pstAUDPbAttr->sampleRate = i2cData&0x3;

	return(ret);
}

int PR1000_AUD_SetMixMute(const int fd, const uint8_t chip, const int bEnable)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cMask = 0x40;
	if(bEnable) i2cData = 0x40;
	else i2cData = 0;

	i2cReg = 0x0B;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_AUD_SetVocMute(const int fd, const uint8_t chip, const int bEnable)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cMask = 0x80;
	if(bEnable) i2cData = 0x80;
	else i2cData = 0;

	i2cReg = 0x0B;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_AUD_SetDacMute(const int fd, const uint8_t chip, const int bEnable)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cMask = 0x10;
	if(bEnable) i2cData = 0x10;
	else i2cData = 0;

	i2cReg = 0x0A;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_AUD_SetDacChn(const int fd, const uint8_t chip, const int ch)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	if( ch > 18 )
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cMask = 0x1F;
	i2cData = (ch & 0x1F);

	i2cReg = 0x0B;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_AUD_SetDetAttr(const int fd, const uint8_t chip, const _stAUDDetAttr *pstAUDDetAttr)
{
	int ret = -1;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cReg = 0x24;
	i2cData = pstAUDDetAttr->absThresholdCh0;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x25;
	i2cData = pstAUDDetAttr->absThresholdCh1;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x26;
	i2cData = pstAUDDetAttr->absThresholdCh2;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x27;
	i2cData = pstAUDDetAttr->absThresholdCh3;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x28;
	i2cData = pstAUDDetAttr->diffThresholdCh0;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x29;
	i2cData = pstAUDDetAttr->diffThresholdCh1;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x2A;
	i2cData = pstAUDDetAttr->diffThresholdCh2;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x2B;
	i2cData = pstAUDDetAttr->diffThresholdCh3;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x2C;
	i2cData = pstAUDDetAttr->noAudThresholdCh0;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x2D;
	i2cData = pstAUDDetAttr->noAudThresholdCh1;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x2E;
	i2cData = pstAUDDetAttr->noAudThresholdCh2;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x2F;
	i2cData = pstAUDDetAttr->noAudThresholdCh3;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x32;
	i2cData = pstAUDDetAttr->noAudMax; 	//on->noAud detect time. increase->fast
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x33;
	i2cData = pstAUDDetAttr->noAudMin;	//noAud->on detect time. decrease->fast
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_AUD_GetDetAttr(const int fd, const uint8_t chip, _stAUDDetAttr *pstAUDDetAttr)
{
	int ret = -1;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cReg = 0x24;
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDDetAttr->absThresholdCh0 = i2cData;

	i2cReg = 0x25;
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDDetAttr->absThresholdCh1 = i2cData;

	i2cReg = 0x26;
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDDetAttr->absThresholdCh2 = i2cData;

	i2cReg = 0x27;
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDDetAttr->absThresholdCh3 = i2cData;

	i2cReg = 0x28;
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDDetAttr->diffThresholdCh0 = i2cData;

	i2cReg = 0x29;
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDDetAttr->diffThresholdCh1 = i2cData;

	i2cReg = 0x2A;
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDDetAttr->diffThresholdCh2 = i2cData;

	i2cReg = 0x2B;
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDDetAttr->diffThresholdCh3 = i2cData;

	i2cReg = 0x2C;
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDDetAttr->noAudThresholdCh0 = i2cData;

	i2cReg = 0x2D;
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDDetAttr->noAudThresholdCh1 = i2cData;

	i2cReg = 0x2E;
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDDetAttr->noAudThresholdCh2 = i2cData;

	i2cReg = 0x2F;
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDDetAttr->noAudThresholdCh3 = i2cData;

	i2cReg = 0x32;
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDDetAttr->noAudMax = i2cData; 	//on->noAud detect time. increase->fast

	i2cReg = 0x33;
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDDetAttr->noAudMin = i2cData;	//noAud->on detect time. decrease->fast

	return(ret);
}

int PR1000_AUD_SetCascadeAttr(const int fd, const uint8_t chip, const _stAUDCascadeAttr *pstAUDCascadeAttr)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	if( pstAUDCascadeAttr == NULL )
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cMask = 0x10;
	i2cData = (pstAUDCascadeAttr->bCascadeMaster&1)<<4;
	i2cReg = 0x35;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_AUD_GetCascadeAttr(const int fd, const uint8_t chip, _stAUDCascadeAttr *pstAUDCascadeAttr)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	uint8_t i2cSlaveAddr; 

	if( chip > PR1000_CHIP_COUNT )
	{
		ErrorString("Invalid chip.\n");
		return(-1);
	}

	if( pstAUDCascadeAttr == NULL )
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	i2cMask = 0x10;
	i2cData = 0;
	i2cReg = 0x35;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_AUD, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstAUDCascadeAttr->bCascadeMaster = (i2cData>>4)&0x1;

	return(ret);
}
#endif // DONT_SUPPORT_AUD_ALINK

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*** PR1000 - PTZ control. ***/
//////////////////////////////////////////////////////////////////////////////////////////////////////
int PR1000_PTZ_LoadTable(const int fd, const _stPortChSel *pstPortChSel, const uint8_t format, const uint8_t resol, _drvHost *pHost)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint16_t tblData = 0;
	_PR1000_REG_TABLE_PTZ_SD *pTableSD = NULL;
	_PR1000_REG_TABLE_COMMON *pTableSD_common = NULL;
	_PR1000_REG_TABLE_PTZ_HD *pTableHD = NULL;
	_PR1000_REG_TABLE_COMMON *pTableHD_common = NULL;
	uint8_t ptzResol = resol; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if( (format >= max_pr1000_format) || (resol >= max_pr1000_outresol) )
	{
		ErrorString("invalid type.\n");
		return(-1);
	}

	/* convert cif,shrink resolution to camera resolution. */
	switch(resol)
	{
		case pr1000_outresol_1280x720p60c: ptzResol = pr1000_outresol_1280x720p60; break;
		case pr1000_outresol_1280x720p50c: ptzResol = pr1000_outresol_1280x720p50; break;
		case pr1000_outresol_1280x720p30c: ptzResol =  pr1000_outresol_1280x720p30; break;
		case pr1000_outresol_1280x720p25c: ptzResol =  pr1000_outresol_1280x720p25; break;
		case pr1000_outresol_1920x1080p30c: ptzResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25c: ptzResol =  pr1000_outresol_1920x1080p25; break;
		case pr1000_outresol_1920x1080p30s: ptzResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25s: ptzResol =  pr1000_outresol_1920x1080p25; break;
		default: break;
	}

	Print("> PixelPlus PR1000 PTZ Load table [mapChn:%d, prChn:%d, %s, %s]\n", mapChn COMMA prChn COMMA _STR_PR1000_FORMAT[format] COMMA _STR_PR1000_OUTRESOL[ptzResol]);

	/* loading ptz channel table register */
	if( (format == pr1000_format_SD720) || (format == pr1000_format_SD960) ) // sd720, sd960 format
	{
		if(ptzResol > pr1000_outresol_960x576i50)
		{
			Error("Invalid sd resol(%d).\n", ptzResol);
			return(-1);
		}

		pTableSD_common = (_PR1000_REG_TABLE_COMMON *)pr1000_reg_table_ptz_SD_common;
		while(pTableSD_common->addr != 0xFF)
		{
			i2cReg = pTableSD_common->addr + PR1000_OFFSETADDR_PTZ_CH(prChn); // ch base + offset
			tblData = pTableSD_common->pData;
			pTableSD_common++;

			if(tblData == 0xFF00) continue; //skip
			else if(tblData == 0xFFFF) break; //end
			else
			{
				Dbg("Write [p%d, 0x%02x-0x%04x]\n", (uint8_t)(tblData>>8) COMMA i2cReg COMMA tblData);
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, (uint8_t)(tblData>>8), i2cReg, (uint8_t)tblData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}

			}
		}

		pTableSD = (_PR1000_REG_TABLE_PTZ_SD *)pr1000_reg_table_ptz_SD;
		while(pTableSD->addr != 0xFF)
		{
			i2cReg = pTableSD->addr + PR1000_OFFSETADDR_PTZ_CH(prChn); // ch base + offset
			tblData = pTableSD->pData[ptzResol];
			pTableSD++;

			if(tblData == 0xFF00) continue; //skip
			else if(tblData == 0xFFFF) break; //end
			else
			{
				Dbg("Write [p%d, 0x%02x-0x%04x]\n", (uint8_t)(tblData>>8) COMMA i2cReg COMMA tblData);
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, (uint8_t)(tblData>>8), i2cReg, (uint8_t)tblData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}

			}
		}
	}
	else if( 
#ifndef DONT_SUPPORT_STD_PVI
			(format == pr1000_format_PVI) || 
#endif // DONT_SUPPORT_STD_PVI
			(format == pr1000_format_HDA) || 
			(format == pr1000_format_CVI) || 
			(format == pr1000_format_HDT) || 
			(format == pr1000_format_HDT_NEW) 
			) // hd 720p,1080p format
	{
		if( (ptzResol < pr1000_outresol_1280x720p60) )
		{
			Error("Invalid hd resol(%d).\n", ptzResol);
			return(-1);
		}

		switch(format)
		{
#ifndef DONT_SUPPORT_STD_PVI
			case pr1000_format_PVI:
				{
					pTableHD_common = (_PR1000_REG_TABLE_COMMON *)pr1000_reg_table_ptz_PVI_common;
					pTableHD = (_PR1000_REG_TABLE_PTZ_HD *)pr1000_reg_table_ptz_PVI;
				}
				break;
#endif // DONT_SUPPORT_STD_PVI
			case pr1000_format_HDA:
				{
					pTableHD_common = (_PR1000_REG_TABLE_COMMON *)pr1000_reg_table_ptz_HDA_common;
					pTableHD = (_PR1000_REG_TABLE_PTZ_HD *)pr1000_reg_table_ptz_HDA;
				}
				break;
			case pr1000_format_CVI:
				{
					pTableHD_common = (_PR1000_REG_TABLE_COMMON *)pr1000_reg_table_ptz_CVI_common;
					pTableHD = (_PR1000_REG_TABLE_PTZ_HD *)pr1000_reg_table_ptz_CVI;
				}
				break;
			case pr1000_format_HDT:
				{
					pTableHD_common = (_PR1000_REG_TABLE_COMMON *)pr1000_reg_table_ptz_HDT_common;
					pTableHD = (_PR1000_REG_TABLE_PTZ_HD *)pr1000_reg_table_ptz_HDT;
				}
				break;
			case pr1000_format_HDT_NEW:
				{
					pTableHD_common = (_PR1000_REG_TABLE_COMMON *)pr1000_reg_table_ptz_HDT_NEW_common;
					pTableHD = (_PR1000_REG_TABLE_PTZ_HD *)pr1000_reg_table_ptz_HDT_NEW;
				}
				break;
			default: 
				{
					pTableHD_common = (_PR1000_REG_TABLE_COMMON *)pr1000_reg_table_ptz_CVI_common;
					pTableHD = (_PR1000_REG_TABLE_PTZ_HD *)pr1000_reg_table_ptz_CVI;
				}
				break;
		}

		while(pTableHD_common->addr != 0xFF)
		{
			i2cReg = pTableHD_common->addr + PR1000_OFFSETADDR_PTZ_CH(prChn); // ch base + offset
			tblData = pTableHD_common->pData;
			pTableHD_common++;

			if(tblData == 0xFF00) continue; //skip
			else if(tblData == 0xFFFF) break; //end
			else
			{
				Dbg("Write [p%d, 0x%02x-0x%04x]\n", (uint8_t)(tblData>>8) COMMA i2cReg COMMA tblData);
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, (uint8_t)(tblData>>8), i2cReg, (uint8_t)tblData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}
		}

		while(pTableHD->addr != 0xFF)
		{
			i2cReg = pTableHD->addr + PR1000_OFFSETADDR_PTZ_CH(prChn); // ch base + offset
			tblData = pTableHD->pData[ptzResol-pr1000_outresol_1280x720p60];
			pTableHD++;

			if(tblData == 0xFF00) continue; //skip
			else if(tblData == 0xFFFF) break; //end
			else
			{
				Dbg("Write [p%d, 0x%02x-0x%04x]\n", (uint8_t)(tblData>>8) COMMA i2cReg COMMA tblData);
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, (uint8_t)(tblData>>8), i2cReg, (uint8_t)tblData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}

			}
		}
	}
	else
	{
		Error("Invalid format(%d).\n", format);
		return(-1);
	}

	/* saved config */
	pHost->ptzHost.ptzType[mapChn] = format;
	pHost->ptzHost.ptzResol[mapChn] = ptzResol;

	return(ret);
}

int PR1000_PTZ_EnableRXPath(const int fd, const uint8_t i2cSlaveAddr, const uint8_t prChn, const int bEnable)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;

	if(bEnable)
	{
		DbgString("Enable PTZ_RX_EN.\n");
		i2cMask = 0x80; i2cData = 0x80;
	}
	else
	{
		DbgString("Disable PTZ_RX_EN.\n");
		i2cMask = 0x80; i2cData = 0x00;
	}
	i2cReg = PR1000_REG_ADDR_PTZ_RX_EN + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_PTZ_InitRXFifo(const int fd, const uint8_t i2cSlaveAddr, const uint8_t prChn)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;

	i2cMask = 0x87; i2cData = 0x83;
	i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_PTZ_EnableTXPath(const int fd, const uint8_t i2cSlaveAddr, const uint8_t prChn, const int bEnable)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;

	if(bEnable)
	{
		DbgString("Enable PTZ_TX_EN.\n");
		i2cMask = 0x80; i2cData = 0x80;
	}
	else
	{
		DbgString("Disable PTZ_TX_EN.\n");
		i2cMask = 0x80; i2cData = 0x00;
	}
	i2cReg = PR1000_REG_ADDR_PTZ_TX_EN + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_PTZ_InitTXFifo(const int fd, const uint8_t i2cSlaveAddr, const uint8_t prChn)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;

	i2cMask = 0x87; i2cData = 0x80;
	i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_PTZ_StartRX(const int fd, const _stPortChSel *pstPortChSel, const int bStart)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(bStart)
	{
		Dbg("Start PTZ_RX mapChn(%d)\n", mapChn);
		i2cMask = 0xC0; i2cData = 0xC0;
		i2cReg = PR1000_REG_ADDR_PTZ_RX_EN + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		i2cMask = 0x97; i2cData = 0x83;
		i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}
	else
	{
		Dbg("Stop PTZ_RX mapChn(%d)\n", mapChn);
		i2cMask = 0x40; i2cData = 0x00;
		i2cReg = PR1000_REG_ADDR_PTZ_RX_EN + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}
	return(ret);
}

int PR1000_PTZ_StartTX(const int fd, const _stPortChSel *pstPortChSel, const int bStart)
{
	int ret = -1;
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(bStart)
	{
		Dbg("Start PTZ_TX mapChn(%d)\n", mapChn);
		i2cMask = 0x40; i2cData = 0x40;
	}
	else
	{
		Dbg("Stop PTZ_TX mapChn(%d)\n", mapChn);
		i2cMask = 0x40; i2cData = 0x00;
	}
	i2cReg = PR1000_REG_ADDR_PTZ_TX_EN + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg..\n");
		return(-1);
	}

	return(ret);
}

int PR1000_PTZ_Init(const int fd, const _stPortChSel *pstPortChSel, _drvHost *pHost)
{
	int ret = 0;

	uint8_t fifoSize = 0;

	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	Print("Init PTZ mapChn:%d prChn:%d.\n", mapChn COMMA prChn);

	if( PR1000_PTZ_EnableTXPath(fd, i2cSlaveAddr, prChn, DISABLE) < 0)
	{
		ErrorString("EnableTXPath.\n");
		return(-1);
	}
	if( PR1000_PTZ_StartTX(fd, pstPortChSel, STOP) < 0)
	{
		ErrorString("StartTX.\n");
		return(-1);
	}
	if( PR1000_PTZ_EnableTXPath(fd, i2cSlaveAddr, prChn, ENABLE) < 0)
	{
		ErrorString("EnableTXPath.\n");
		return(-1);
	}

	/* reset rx fifo, stop rx */
	if( PR1000_PTZ_EnableRXPath(fd, i2cSlaveAddr, prChn, DISABLE) < 0)
	{
		ErrorString("EnableRXPath.\n");
		return(-1);
	}
	if( PR1000_PTZ_StartRX(fd, pstPortChSel, STOP) < 0)
	{
		ErrorString("StartRX.\n");
		return(-1);
	}
	if( PR1000_PTZ_EnableRXPath(fd, i2cSlaveAddr, prChn, ENABLE) < 0)
	{
		ErrorString("EnableRXPath.\n");
		return(-1);
	}

	/* Init fifo, and verify */
	if( PR1000_PTZ_InitTXFifo(fd, i2cSlaveAddr, prChn) < 0)
	{
		ErrorString("InitTXFifo.\n");
		return(-1);
	}
	if( PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, PR1000_REG_ADDR_PTZ_FIFO_WR_SIZE + PR1000_OFFSETADDR_PTZ_CH(prChn), &fifoSize) < 0)
	{
		ErrorString("PageRead.\n");
		return(-1);
	}
	if(fifoSize != 0) 
	{
		Error("cur tx Fifo:%d.\n", fifoSize);
		return(-1);
	}


	if( PR1000_PTZ_InitRXFifo(fd, i2cSlaveAddr, prChn) < 0)
	{
		ErrorString("InitRxFifo.\n");
		return(-1);
	}
	if( PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, PR1000_REG_ADDR_PTZ_FIFO_RD_SIZE + PR1000_OFFSETADDR_PTZ_CH(prChn), &fifoSize) < 0)
	{
		ErrorString("PageRead.\n");
		return(-1);
	}
	if(fifoSize != 0) 
	{
		Error("cur rx Fifo:%d.\n", fifoSize);
		return(-1);
	}

	pHost->ptzHost.u32PtzTxCnt[mapChn] = 0;
	pHost->ptzHost.u32PtzRxCnt[mapChn] = 0;

	return(ret);
}

int PR1000_PTZ_SetPattern(const int fd, const _stPortChSel *pstPortChSel, const uint8_t format,  const uint8_t resol, _drvHost *pHost)
{
	int ret = 0;

	uint8_t outResol = resol; 
	uint8_t camResol = resol; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 
	uint8_t i2cReg, i2cData, i2cMask;

	DbgString("Set pattern PTZ.\n");

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if( (format >= max_pr1000_format) || (resol >= max_pr1000_outresol) )
	{
		ErrorString("invalid type.\n");
		return(-1);
	}

	/* saved config */
	pHost->ptzHost.initialized[mapChn] = FALSE;

	/* convert cif,shrink resolution to camera resolution. */
	switch(resol)
	{
		case pr1000_outresol_1280x720p60c: outResol = pr1000_outresol_1280x720p60; break;
		case pr1000_outresol_1280x720p50c: outResol = pr1000_outresol_1280x720p50; break;
		case pr1000_outresol_1280x720p30c: outResol =  pr1000_outresol_1280x720p30; break;
		case pr1000_outresol_1280x720p25c: outResol =  pr1000_outresol_1280x720p25; break;
		case pr1000_outresol_1920x1080p30c: outResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25c: outResol =  pr1000_outresol_1920x1080p25; break;
		case pr1000_outresol_1920x1080p30s: outResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25s: outResol =  pr1000_outresol_1920x1080p25; break;
		default: break;
	}

	Print("Set ptz tx & rx pattern mapChn:%d, %s.\n", mapChn COMMA _STR_PR1000_FORMAT[format]);
	switch(format)
	{
		case pr1000_format_SD720:
		case pr1000_format_SD960:
			{/*{{{*/
				if( (outResol == pr1000_outresol_720x480i60) || (outResol == pr1000_outresol_960x480i60) )
				{
#ifndef DONT_SUPPORT_PTZ_FUNC
					if( (ret = PR1000_PTZ_SetTxParam(fd, pstPortChSel, (const _stPTZTxParam *)&pr1000_ptz_txparam_sd_def[pr1000_outresol_720x480i60])) < 0)
					{
						ErrorString("Write ptz tx param.\n");
						return(-1);
					}
#endif // DONT_SUPPORT_PTZ_FUNC
					if( (ret = PR1000_PTZ_SetRxParam(fd, pstPortChSel, (const _stPTZRxParam *)&pr1000_ptz_rxparam_sd_def[pr1000_outresol_720x480i60])) < 0)
					{
						ErrorString("Write ptz rx param.\n");
						return(-1);
					}
				}
				else if( (outResol == pr1000_outresol_720x576i50) || (outResol == pr1000_outresol_960x576i50) )
				{
#ifndef DONT_SUPPORT_PTZ_FUNC
					if( (ret = PR1000_PTZ_SetTxParam(fd, pstPortChSel, (const _stPTZTxParam *)&pr1000_ptz_txparam_sd_def[pr1000_outresol_720x576i50])) < 0)
					{
						ErrorString("Write ptz tx param.\n");
						return(-1);
					}
#endif // DONT_SUPPORT_PTZ_FUNC
					if( (ret = PR1000_PTZ_SetRxParam(fd, pstPortChSel, (const _stPTZRxParam *)&pr1000_ptz_rxparam_sd_def[pr1000_outresol_720x576i50])) < 0)
					{
						ErrorString("Write ptz rx param.\n");
						return(-1);
					}
				}
				else
				{
					Error("Invalid resol. mapChn:%d\n", mapChn);
				}

#ifndef DONT_SUPPORT_PTZ_FUNC
				if(PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_sd_tx_pat_format, sizeof(pr1000_ptz_table_sd_tx_pat_format), pr1000_ptz_table_sd_tx_pat_data, sizeof(pr1000_ptz_table_sd_tx_pat_data)) < 0)
				{
					Error("Init pattern flag.(mapChn:%d)\n", mapChn);
					return(-1);
				}
#endif // DONT_SUPPORT_PTZ_FUNC

				if(PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_sd_rx_pat_format, sizeof(pr1000_ptz_table_sd_rx_pat_format), pr1000_ptz_table_sd_rx_pat_start_format, sizeof(pr1000_ptz_table_sd_rx_pat_start_format), pr1000_ptz_table_sd_rx_pat_start_data, sizeof(pr1000_ptz_table_sd_rx_pat_start_data)) < 0)
				{
					Error("Init pattern flag.(mapChn:%d)\n", mapChn);
					return(-1);
				}
				/* set rd data mode for receive data. */
				i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_SD_RX_DATA_BITSWAP&0x1)<<6);
				i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
				if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}/*}}}*/
			break;
#ifndef DONT_SUPPORT_STD_PVI
		case pr1000_format_PVI:
			{/*{{{*/
				if( (outResol >= pr1000_outresol_1280x720p60) && (outResol <= pr1000_outresol_1920x1080p25) )
				{
#ifndef DONT_SUPPORT_PTZ_FUNC
					if( (ret = PR1000_PTZ_SetTxParam(fd, pstPortChSel, (const _stPTZTxParam *)&pr1000_ptz_txparam_pvi_def[outResol-pr1000_outresol_1280x720p60])) < 0)
					{
						ErrorString("Write ptz tx param.\n");
						return(-1);
					}
#endif // DONT_SUPPORT_PTZ_FUNC

					if( (ret = PR1000_PTZ_SetRxParam(fd, pstPortChSel, (const _stPTZRxParam *)&pr1000_ptz_rxparam_pvi_def[outResol-pr1000_outresol_1280x720p60])) < 0)
					{
						ErrorString("Write ptz rx param.\n");
						return(-1);
					}
				}
				else
				{
					Error("Invalid resol. mapChn:%d\n", mapChn);
				}

#ifndef DONT_SUPPORT_PTZ_FUNC
				if(PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_pvi_tx_pat_format, sizeof(pr1000_ptz_table_pvi_tx_pat_format), pr1000_ptz_table_pvi_tx_pat_data, sizeof(pr1000_ptz_table_pvi_tx_pat_data)) < 0)
				{
					Error("Init pattern flag.(mapChn:%d)\n", mapChn);
					return(-1);
				}
#endif // DONT_SUPPORT_PTZ_FUNC

				if(PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_pvi_rx_pat_format, sizeof(pr1000_ptz_table_pvi_rx_pat_format), pr1000_ptz_table_pvi_rx_pat_start_format, sizeof(pr1000_ptz_table_pvi_rx_pat_start_format), pr1000_ptz_table_pvi_rx_pat_start_data, sizeof(pr1000_ptz_table_pvi_rx_pat_start_data)) < 0)
				{
					Error("Init pattern flag.(mapChn:%d)\n", mapChn);
					return(-1);
				}
				/* set rd data mode for receive data. */
				i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_PVI_RX_DATA_BITSWAP&0x1)<<6);
				i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
				if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}/*}}}*/
			break;
#endif // DONT_SUPPORT_STD_PVI
		case pr1000_format_HDA:
			{/*{{{*/
				if( (outResol >= pr1000_outresol_1280x720p60) && (outResol <= pr1000_outresol_1920x1080p25) )
				{
#ifndef DONT_SUPPORT_PTZ_FUNC
					if( (ret = PR1000_PTZ_SetTxParam(fd, pstPortChSel, (const _stPTZTxParam *)&pr1000_ptz_txparam_hda_def[outResol-pr1000_outresol_1280x720p60])) < 0)
					{
						ErrorString("Write ptz tx param.\n");
						return(-1);
					}
#endif // DONT_SUPPORT_PTZ_FUNC

					if( (ret = PR1000_PTZ_SetRxParam(fd, pstPortChSel, (const _stPTZRxParam *)&pr1000_ptz_rxparam_hda_def[outResol-pr1000_outresol_1280x720p60])) < 0)
					{
						ErrorString("Write ptz rx param.\n");
						return(-1);
					}
				}
				else
				{
					Error("Invalid resol. mapChn:%d\n", mapChn);
				}

				if( (outResol >= pr1000_outresol_1280x720p60) && (outResol <= pr1000_outresol_1280x720p25) )
				{
#ifndef DONT_SUPPORT_PTZ_FUNC
					if(PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_hda_tx_pat_format_720p, sizeof(pr1000_ptz_table_hda_tx_pat_format_720p), pr1000_ptz_table_hda_tx_pat_data_720p, sizeof(pr1000_ptz_table_hda_tx_pat_data_720p)) < 0)
					{
						Error("Init pattern flag.(mapChn:%d)\n", mapChn);
						return(-1);
					}
#endif // DONT_SUPPORT_PTZ_FUNC
					if(PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_hda_rx_pat_format_720p, sizeof(pr1000_ptz_table_hda_rx_pat_format_720p), pr1000_ptz_table_hda_rx_pat_start_format_720p, sizeof(pr1000_ptz_table_hda_rx_pat_start_format_720p), pr1000_ptz_table_hda_rx_pat_start_data_720p, sizeof(pr1000_ptz_table_hda_rx_pat_start_data_720p)) < 0)
					{
						Error("Init pattern flag.(mapChn:%d)\n", mapChn);
						return(-1);
					}
					/* set rd data mode for receive data. */
					i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_HDA_RX_DATA_BITSWAP&0x1)<<6);
					i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
					if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}
				}
				else
				{
#ifndef DONT_SUPPORT_PTZ_FUNC
					if(PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_hda_tx_pat_format_1080p, sizeof(pr1000_ptz_table_hda_tx_pat_format_1080p), pr1000_ptz_table_hda_tx_pat_data_1080p, sizeof(pr1000_ptz_table_hda_tx_pat_data_1080p)) < 0)
					{
						Error("Init pattern flag.(mapChn:%d)\n", mapChn);
						return(-1);
					}
#endif // DONT_SUPPORT_PTZ_FUNC
					if(PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_hda_rx_pat_format_1080p, sizeof(pr1000_ptz_table_hda_rx_pat_format_1080p), pr1000_ptz_table_hda_rx_pat_start_format_1080p, sizeof(pr1000_ptz_table_hda_rx_pat_start_format_1080p), pr1000_ptz_table_hda_rx_pat_start_data_1080p, sizeof(pr1000_ptz_table_hda_rx_pat_start_data_1080p)) < 0)
					{
						Error("Init pattern flag.(mapChn:%d)\n", mapChn);
						return(-1);
					}
					/* set rd data mode for receive data. */
					i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_HDA_RX_DATA_BITSWAP&0x1)<<6);
					i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
					if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}
				}
			}/*}}}*/
			break;
		case pr1000_format_CVI:
			{/*{{{*/
				if( (outResol >= pr1000_outresol_1280x720p60) && (outResol <= pr1000_outresol_1920x1080p25) )
				{
#ifndef DONT_SUPPORT_PTZ_FUNC
					if( (ret = PR1000_PTZ_SetTxParam(fd, pstPortChSel, (const _stPTZTxParam *)&pr1000_ptz_txparam_cvi_def[outResol-pr1000_outresol_1280x720p60])) < 0)
					{
						ErrorString("Write ptz tx param.\n");
						return(-1);
					}
#endif // DONT_SUPPORT_PTZ_FUNC

					if( (ret = PR1000_PTZ_SetRxParam(fd, pstPortChSel, (const _stPTZRxParam *)&pr1000_ptz_rxparam_cvi_def[outResol-pr1000_outresol_1280x720p60])) < 0)
					{
						ErrorString("Write ptz rx param.\n");
						return(-1);
					}
				}
				else
				{
					Error("Invalid resol. mapChn:%d\n", mapChn);
				}

#ifndef DONT_SUPPORT_PTZ_FUNC
				if(PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_cvi_tx_pat_format, sizeof(pr1000_ptz_table_cvi_tx_pat_format), pr1000_ptz_table_cvi_tx_pat_data, sizeof(pr1000_ptz_table_cvi_tx_pat_data)) < 0)
				{
					Error("Init pattern flag.(mapChn:%d)\n", mapChn);
					return(-1);
				}
#endif // DONT_SUPPORT_PTZ_FUNC

				if(PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_cvi_rx_pat_format, sizeof(pr1000_ptz_table_cvi_rx_pat_format), pr1000_ptz_table_cvi_rx_pat_start_format, sizeof(pr1000_ptz_table_cvi_rx_pat_start_format), pr1000_ptz_table_cvi_rx_pat_start_data, sizeof(pr1000_ptz_table_cvi_rx_pat_start_data)) < 0)
				{
					Error("Init pattern flag.(mapChn:%d)\n", mapChn);
					return(-1);
				}
				/* set rd data mode for receive data. */
				i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_CVI_RX_DATA_BITSWAP&0x1)<<6);
				i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
				if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}/*}}}*/
			break;
		case pr1000_format_HDT:
			{/*{{{*/
				if( (outResol >= pr1000_outresol_1280x720p60) && (outResol <= pr1000_outresol_1920x1080p25) )
				{
#ifndef DONT_SUPPORT_PTZ_FUNC
					if( (ret = PR1000_PTZ_SetTxParam(fd, pstPortChSel, (const _stPTZTxParam *)&pr1000_ptz_txparam_hdt_def[outResol-pr1000_outresol_1280x720p60])) < 0)
					{
						ErrorString("Write ptz tx param.\n");
						return(-1);
					}
#endif // DONT_SUPPORT_PTZ_FUNC

					if( (ret = PR1000_PTZ_SetRxParam(fd, pstPortChSel, (const _stPTZRxParam *)&pr1000_ptz_rxparam_hdt_def[outResol-pr1000_outresol_1280x720p60])) < 0)
					{
						ErrorString("Write ptz rx param.\n");
						return(-1);
					}
				}
				else
				{
					Error("Invalid resol. mapChn:%d\n", mapChn);
				}

#ifndef DONT_SUPPORT_PTZ_FUNC
				if(PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_hdt_tx_pat_format, sizeof(pr1000_ptz_table_hdt_tx_pat_format), pr1000_ptz_table_hdt_tx_pat_data, sizeof(pr1000_ptz_table_hdt_tx_pat_data)) < 0)
				{
					Error("Init pattern flag.(mapChn:%d)\n", mapChn);
					return(-1);
				}
#endif // DONT_SUPPORT_PTZ_FUNC

				if(PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_hdt_rx_pat_format, sizeof(pr1000_ptz_table_hdt_rx_pat_format), pr1000_ptz_table_hdt_rx_pat_start_format, sizeof(pr1000_ptz_table_hdt_rx_pat_start_format), pr1000_ptz_table_hdt_rx_pat_start_data, sizeof(pr1000_ptz_table_hdt_rx_pat_start_data)) < 0)
				{
					Error("Init pattern flag.(mapChn:%d)\n", mapChn);
					return(-1);
				}
				/* set rd data mode for receive data. */
				i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_HDT_RX_DATA_BITSWAP&0x1)<<6);
				i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
				if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}/*}}}*/
			break;
		case pr1000_format_HDT_NEW:
			{/*{{{*/
				if( (outResol >= pr1000_outresol_1280x720p60) && (outResol <= pr1000_outresol_1920x1080p25) )
				{
#ifndef DONT_SUPPORT_PTZ_FUNC
					if( (ret = PR1000_PTZ_SetTxParam(fd, pstPortChSel, (const _stPTZTxParam *)&pr1000_ptz_txparam_hdt_new_def[outResol-pr1000_outresol_1280x720p60])) < 0)
					{
						ErrorString("Write ptz tx param.\n");
						return(-1);
					}
#endif // DONT_SUPPORT_PTZ_FUNC

					if( (ret = PR1000_PTZ_SetRxParam(fd, pstPortChSel, (const _stPTZRxParam *)&pr1000_ptz_rxparam_hdt_new_def[outResol-pr1000_outresol_1280x720p60])) < 0)
					{
						ErrorString("Write ptz rx param.\n");
						return(-1);
					}
				}
				else
				{
					Error("Invalid resol. mapChn:%d\n", mapChn);
				}

#ifndef DONT_SUPPORT_PTZ_FUNC
				if(PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_hdt_new_tx_pat_format, sizeof(pr1000_ptz_table_hdt_new_tx_pat_format), pr1000_ptz_table_hdt_new_tx_pat_data, sizeof(pr1000_ptz_table_hdt_new_tx_pat_data)) < 0)
				{
					Error("Init pattern flag.(mapChn:%d)\n", mapChn);
					return(-1);
				}
#endif // DONT_SUPPORT_PTZ_FUNC

				if(PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_hdt_new_rx_pat_format, sizeof(pr1000_ptz_table_hdt_new_rx_pat_format), pr1000_ptz_table_hdt_new_rx_pat_start_format, sizeof(pr1000_ptz_table_hdt_new_rx_pat_start_format), pr1000_ptz_table_hdt_new_rx_pat_start_data, sizeof(pr1000_ptz_table_hdt_new_rx_pat_start_data)) < 0)
				{
					Error("Init pattern flag.(mapChn:%d)\n", mapChn);
					return(-1);
				}
				/* set rd data mode for receive data. */
				i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_HDT_NEW_RX_DATA_BITSWAP&0x1)<<6);
				i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
				if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}/*}}}*/
			break;
		default:
			{
			}
			break;
	}

	/* saved config */
	pHost->ptzHost.initialized[mapChn] = TRUE;
	pHost->sysHost.cntSTDFORMATCheck[mapChn] = 0;
	pHost->sysHost.timeOverSTDFORMATCheck[mapChn] = 0;

	if(pHost->ptzHost.bRxUsedSTDFORMATOnly[mapChn] == TRUE) //TRUE: PTZ Rx used check STDFORMAT only.
	{
		switch(resol)
		{
			case pr1000_outresol_720x480i60: camResol = pr1000_inresol_ntsc; break;
			case pr1000_outresol_720x576i50: camResol = pr1000_inresol_pal; break;
			case pr1000_outresol_960x480i60: camResol = pr1000_inresol_ntsc; break;
			case pr1000_outresol_960x576i50: camResol = pr1000_inresol_pal; break;

			case pr1000_outresol_1280x720p60: camResol = pr1000_inresol_1280x720p60; break;
			case pr1000_outresol_1280x720p50: camResol = pr1000_inresol_1280x720p50; break;
			case pr1000_outresol_1280x720p30: camResol = pr1000_inresol_1280x720p30; break;
			case pr1000_outresol_1280x720p25: camResol = pr1000_inresol_1280x720p25; break;
			case pr1000_outresol_1920x1080p30: camResol = pr1000_inresol_1920x1080p30; break;
			case pr1000_outresol_1920x1080p25: camResol = pr1000_inresol_1920x1080p25; break;

			case pr1000_outresol_1280x720p60c: camResol = pr1000_inresol_1280x720p60; break;
			case pr1000_outresol_1280x720p50c: camResol = pr1000_inresol_1280x720p50; break;
			case pr1000_outresol_1280x720p30c: camResol =  pr1000_inresol_1280x720p30; break;
			case pr1000_outresol_1280x720p25c: camResol =  pr1000_inresol_1280x720p25; break;
			case pr1000_outresol_1920x1080p30c: camResol =  pr1000_inresol_1920x1080p30; break;
			case pr1000_outresol_1920x1080p25c: camResol =  pr1000_inresol_1920x1080p25; break;
			case pr1000_outresol_1920x1080p30s: camResol =  pr1000_inresol_1920x1080p30; break;
			case pr1000_outresol_1920x1080p25s: camResol =  pr1000_inresol_1920x1080p25; break;
			default: break;
		}

		Print("Set ptz rx used check STDFORMAT only. mapChn:%d, %s.\n", mapChn COMMA _STR_PR1000_FORMAT[format]);
		PR1000_PTZ_STDFORMAT_SetPattern(fd, pstPortChSel, format, camResol, pHost);

		/* don't delete */
		pHost->sysHost.cntSTDFORMATCheck[mapChn] = PTZ_STDFORMAT_CHECK_CNT;
		pHost->sysHost.timeOverSTDFORMATCheck[mapChn] = PTZ_STDFORMAT_CHECK_CNT;
	}

	return(ret);
}

int PR1000_PTZ_STDFORMAT_Check(const int fd, const _stPortChSel *pstPortChSel, const enum _pr1000_table_format format, const enum _pr1000_table_inresol camResol, _drvHost *pHost)
{
	int ret = -1;

	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

        if( (format >= max_pr1000_format) || (camResol >= max_pr1000_inresol) )
        {     
                ErrorString("invalid type.\n");
                return(-1);
        }     

	Dbg("Setup mapChn:%d, %s, %s\n", mapChn COMMA _STR_PR1000_FORMAT[format] COMMA _STR_PR1000_INRESOL[camResol]);

	if(format == pr1000_format_HDT) /* If HDT, check Old or New camera format 720p25 or 720p30. */
	{
		if( (camResol == pr1000_inresol_1280x720p25) || (camResol == pr1000_inresol_1280x720p30) )
		{
			if(pHost->sysHost.cntSTDFORMATCheck[mapChn] <= 0)
			{
				Print("HDT: Old or New Start mapChn:%d, %s, %s\n", mapChn COMMA _STR_PR1000_FORMAT[format] COMMA _STR_PR1000_INRESOL[camResol]);
				/* init once */
				ret = PR1000_PTZ_STDFORMAT_SetPattern(fd, pstPortChSel, format,  camResol, pHost);

				PR1000_PTZ_StartRX(fd, pstPortChSel, TRUE);
			}
		}
	}
	else if(format == pr1000_format_HDA) /* If HDA, check this is real HDA or noEQ CVI. */
	{
		if(pHost->sysHost.cntSTDFORMATCheck[mapChn] <= 0)
		{
			Print("HDA: real HDA or noEQ CVI Start mapChn:%d, %s, %s\n", mapChn COMMA _STR_PR1000_FORMAT[format] COMMA _STR_PR1000_INRESOL[camResol]);
			/* init once */
			ret = PR1000_PTZ_STDFORMAT_SetPattern(fd, pstPortChSel, format,  camResol, pHost);

			PR1000_PTZ_StartRX(fd, pstPortChSel, TRUE);
		}
	}
	else
	{
		DbgString("Don't support.\n");
		return(0);
	}

	/* don't delete */
	pHost->sysHost.cntSTDFORMATCheck[mapChn] = PTZ_STDFORMAT_CHECK_CNT;
	pHost->sysHost.timeOverSTDFORMATCheck[mapChn] = PTZ_STDFORMAT_CHECK_CNT;

	return(ret);
}

int PR1000_PTZ_STDFORMAT_SetPattern(const int fd, const _stPortChSel *pstPortChSel, const uint8_t format,  const uint8_t camResol, _drvHost *pHost)
{
	int ret = 0;

	uint8_t i2cReg;
	uint8_t i2cData = 0; 
	uint8_t i2cMask = 0;
	uint16_t tblData = 0;
	_PR1000_REG_TABLE_PTZ_SD *pTableSD = NULL;
	_PR1000_REG_TABLE_PTZ_HD *pTableHD = NULL;
	_PR1000_REG_TABLE_COMMON *pTableHD_common = NULL;

	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	DbgString("Set pattern PTZ STDFORMAT.\n");

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if( (format >= max_pr1000_format) || (camResol >= max_pr1000_inresol) )
	{
		ErrorString("invalid type.\n");
		return(-1);
	}

	/* saved config */
	if(pHost->ptzHost.initialized[mapChn] == FALSE)
	{
		ErrorString("Not initialize ptz.\n");
		return(-1);
	}

	switch(format)
	{
		case pr1000_format_SD720:
			{ 
				pTableSD = NULL;
			}
			break;
		case pr1000_format_SD960:
			{ 
				pTableSD = NULL;
			}
			break;
#ifndef DONT_SUPPORT_STD_PVI
		case pr1000_format_PVI:
			{ 
				pTableSD = NULL;
			}
			break;
#endif // DONT_SUPPORT_STD_PVI
		case pr1000_format_HDA:
			{/*{{{*/
				Print("Set HDA: std format check pattern mapChn:%d, %s.\n", mapChn COMMA _STR_PR1000_FORMAT[format]);

				/* check only HD and full HD format */
				if(camResol < pr1000_inresol_1280x720p60) //SD
				{
					break;
				}

				pTableHD_common = (_PR1000_REG_TABLE_COMMON *)pr1000_reg_table_ptz_HDA_common;
				while(pTableHD_common->addr != 0xFF)
				{
					i2cReg = pTableHD_common->addr + PR1000_OFFSETADDR_PTZ_CH(prChn); // ch base + offset
					tblData = pTableHD_common->pData;
					pTableHD_common++;

					if(tblData == 0xFF00) continue; //skip
					else if(tblData == 0xFFFF) break; //end
					else
					{
						Dbg("Write [p%d, 0x%02x-0x%04x]\n", (uint8_t)(tblData>>8) COMMA i2cReg COMMA tblData);
						if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, (uint8_t)(tblData>>8), i2cReg, (uint8_t)tblData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}

					}
				}

				pTableHD = (_PR1000_REG_TABLE_PTZ_HD *)pr1000_reg_table_ptz_HDA;
				while(pTableHD->addr != 0xFF)
				{
					i2cReg = pTableHD->addr + PR1000_OFFSETADDR_PTZ_CH(prChn); // ch base + offset
					tblData = pTableHD->pData[camResol-pr1000_inresol_1280x720p60];
					pTableHD++;

					if(tblData == 0xFF00) continue; //skip
					else if(tblData == 0xFFFF) break; //end
					else
					{
						Dbg("Write [p%d, 0x%02x-0x%04x]\n", (uint8_t)(tblData>>8) COMMA i2cReg COMMA tblData);
						if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, (uint8_t)(tblData>>8), i2cReg, (uint8_t)tblData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}

					}
				}


				PR1000_PTZ_Init(fd, &pHost->sysHost.portChSel[mapChn], pHost); //Do after ptz loadtable.

				if( (camResol >= pr1000_inresol_1280x720p60) && (camResol <= pr1000_inresol_1920x1080p25) )
				{
#ifndef DONT_SUPPORT_PTZ_FUNC
					if( (ret = PR1000_PTZ_SetTxParam(fd, pstPortChSel, (const _stPTZTxParam *)&pr1000_ptz_txparam_hda_def[camResol-pr1000_inresol_1280x720p60])) < 0)
					{
						ErrorString("Write ptz tx param.\n");
						return(-1);
					}
#endif // DONT_SUPPORT_PTZ_FUNC
					if( (ret = PR1000_PTZ_SetRxParam(fd, pstPortChSel, (const _stPTZRxParam *)&pr1000_ptz_rxparam_hda_stdformat_def[camResol-pr1000_inresol_1280x720p60])) < 0)
					{
						ErrorString("Write ptz rx param.\n");
						return(-1);
					}
				}
				else
				{
					Error("Invalid resol. mapChn:%d\n", mapChn);
				}

#ifndef DONT_SUPPORT_PTZ_FUNC
				if( (camResol >= pr1000_inresol_1280x720p60) && (camResol <= pr1000_inresol_1280x720p25) )
				{
					if(PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_hda_tx_pat_format_720p, sizeof(pr1000_ptz_table_hda_tx_pat_format_720p), pr1000_ptz_table_hda_tx_pat_data_720p, sizeof(pr1000_ptz_table_hda_tx_pat_data_720p)) < 0)
					{
						Error("Init pattern flag.(mapChn:%d)\n", mapChn);
						return(-1);
					}
				}
				else if( (camResol >= pr1000_inresol_1920x1080p30) && (camResol <= pr1000_inresol_1920x1080p25) )
				{
					if(PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_hda_tx_pat_format_1080p, sizeof(pr1000_ptz_table_hda_tx_pat_format_1080p), pr1000_ptz_table_hda_tx_pat_data_1080p, sizeof(pr1000_ptz_table_hda_tx_pat_data_1080p)) < 0)
					{
						Error("Init pattern flag.(mapChn:%d)\n", mapChn);
						return(-1);
					}
				}
#endif // DONT_SUPPORT_PTZ_FUNC

				if(PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_hda_stdformat_rx_pat_format, sizeof(pr1000_ptz_table_hda_stdformat_rx_pat_format), pr1000_ptz_table_hda_stdformat_rx_pat_start_format, sizeof(pr1000_ptz_table_hda_stdformat_rx_pat_start_format), pr1000_ptz_table_hda_stdformat_rx_pat_start_data, sizeof(pr1000_ptz_table_hda_stdformat_rx_pat_start_data)) < 0)
				{
					Error("Init pattern flag.(mapChn:%d)\n", mapChn);
					return(-1);
				}
				/* set rd data mode for receive data. */
				i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_HDT_STDFORMAT_RX_DATA_BITSWAP&0x1)<<6);
				i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
				if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
				/* STDFORMAT Parameter */
				{/*{{{*/
					/* PTZ_RX_EN[3] IGNORE_FRM_EN */
					i2cMask = 0x08; i2cData = 0x00;
					i2cReg = 0x00 + PR1000_OFFSETADDR_PTZ_CH(prChn);
					if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}

				}/*}}}*/

				/* make ptz receiving environment.(PTZ_SLICE_LEVEL0) */
				i2cReg = 0x62 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
				i2cData = 0x30;
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_VDEC_PAGE(prChn), i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}/*}}}*/
			break;
		case pr1000_format_CVI:
			{ 
				pTableSD = NULL;
			}
			break;
		case pr1000_format_HDT:
		case pr1000_format_HDT_NEW:
			{/*{{{*/
				Print("Set HDT: std format check pattern mapChn:%d, %s.\n", mapChn COMMA _STR_PR1000_FORMAT[format]);

				/* check only 720p25 or 720p30 format */
				if( (camResol != pr1000_inresol_1280x720p25) && (camResol != pr1000_inresol_1280x720p30) )
					break;

				if(format == pr1000_format_HDT)
				{
					pTableHD_common = (_PR1000_REG_TABLE_COMMON *)pr1000_reg_table_ptz_HDT_common;
					pTableHD = (_PR1000_REG_TABLE_PTZ_HD *)pr1000_reg_table_ptz_HDT;
				}
				else
				{
					pTableHD_common = (_PR1000_REG_TABLE_COMMON *)pr1000_reg_table_ptz_HDT_NEW_common;
					pTableHD = (_PR1000_REG_TABLE_PTZ_HD *)pr1000_reg_table_ptz_HDT_NEW;
				}

				while(pTableHD_common->addr != 0xFF)
				{
					i2cReg = pTableHD_common->addr + PR1000_OFFSETADDR_PTZ_CH(prChn); // ch base + offset
					tblData = pTableHD_common->pData;
					pTableHD_common++;

					if(tblData == 0xFF00) continue; //skip
					else if(tblData == 0xFFFF) break; //end
					else
					{
						Dbg("Write [p%d, 0x%02x-0x%04x]\n", (uint8_t)(tblData>>8) COMMA i2cReg COMMA tblData);
						if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, (uint8_t)(tblData>>8), i2cReg, (uint8_t)tblData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}

					}
				}

				while(pTableHD->addr != 0xFF)
				{
					i2cReg = pTableHD->addr + PR1000_OFFSETADDR_PTZ_CH(prChn); // ch base + offset
					tblData = pTableHD->pData[camResol-pr1000_inresol_1280x720p60];
					pTableHD++;

					if(tblData == 0xFF00) continue; //skip
					else if(tblData == 0xFFFF) break; //end
					else
					{
						Dbg("Write [p%d, 0x%02x-0x%04x]\n", (uint8_t)(tblData>>8) COMMA i2cReg COMMA tblData);
						if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, (uint8_t)(tblData>>8), i2cReg, (uint8_t)tblData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}

					}
				}

				PR1000_PTZ_Init(fd, &pHost->sysHost.portChSel[mapChn], pHost); //Do after ptz loadtable.

				if( (camResol >= pr1000_inresol_1280x720p60) && (camResol <= pr1000_inresol_1920x1080p25) )
				{
					if(format == pr1000_format_HDT)
					{
#ifndef DONT_SUPPORT_PTZ_FUNC
						if( (ret = PR1000_PTZ_SetTxParam(fd, pstPortChSel, (const _stPTZTxParam *)&pr1000_ptz_txparam_hdt_def[camResol-pr1000_inresol_1280x720p60])) < 0)
						{
							ErrorString("Write ptz tx param.\n");
							return(-1);
						}
#endif // DONT_SUPPORT_PTZ_FUNC
						if( (ret = PR1000_PTZ_SetRxParam(fd, pstPortChSel, (const _stPTZRxParam *)&pr1000_ptz_rxparam_hdt_stdformat_def[camResol-pr1000_inresol_1280x720p60])) < 0)
						{
							ErrorString("Write ptz rx param.\n");
							return(-1);
						}
					}
					else
					{
#ifndef DONT_SUPPORT_PTZ_FUNC
						if( (ret = PR1000_PTZ_SetTxParam(fd, pstPortChSel, (const _stPTZTxParam *)&pr1000_ptz_txparam_hdt_new_def[camResol-pr1000_inresol_1280x720p60])) < 0)
						{
							ErrorString("Write ptz tx param.\n");
							return(-1);
						}
#endif // DONT_SUPPORT_PTZ_FUNC
						if( (ret = PR1000_PTZ_SetRxParam(fd, pstPortChSel, (const _stPTZRxParam *)&pr1000_ptz_rxparam_hdt_new_stdformat_def[camResol-pr1000_inresol_1280x720p60])) < 0)
						{
							ErrorString("Write ptz rx param.\n");
							return(-1);
						}
					}
				}
				else
				{
					Error("Invalid resol. mapChn:%d\n", mapChn);
				}

#ifndef DONT_SUPPORT_PTZ_FUNC
				if(PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_hdt_tx_pat_format, sizeof(pr1000_ptz_table_hdt_tx_pat_format), pr1000_ptz_table_hdt_tx_pat_data, sizeof(pr1000_ptz_table_hdt_tx_pat_data)) < 0)
				{
					Error("Init pattern flag.(mapChn:%d)\n", mapChn);
					return(-1);
				}
#endif // DONT_SUPPORT_PTZ_FUNC

				if(PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_hdt_stdformat_rx_pat_format, sizeof(pr1000_ptz_table_hdt_stdformat_rx_pat_format), pr1000_ptz_table_hdt_stdformat_rx_pat_start_format, sizeof(pr1000_ptz_table_hdt_stdformat_rx_pat_start_format), pr1000_ptz_table_hdt_stdformat_rx_pat_start_data, sizeof(pr1000_ptz_table_hdt_stdformat_rx_pat_start_data)) < 0)
				{
					Error("Init pattern flag.(mapChn:%d)\n", mapChn);
					return(-1);
				}
				/* set rd data mode for receive data. */
				i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_HDT_STDFORMAT_RX_DATA_BITSWAP&0x1)<<6);
				i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
				if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
				/* STDFORMAT Parameter */
				{/*{{{*/
					/* Set start position by RX_FREQ_FIRST */
					if(camResol == pr1000_inresol_1280x720p25)
					{
						/* RX_FREQ_FIRST[23:16] */
						i2cData = 0x00;
						i2cReg = 0x03 + PR1000_OFFSETADDR_PTZ_CH(prChn);
						if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
						/* RX_FREQ_FIRST[15:08] */
						i2cData = 0x0F;
						i2cReg = 0x04 + PR1000_OFFSETADDR_PTZ_CH(prChn);
						if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
						/* RX_FREQ_FIRST[07:00] */
						i2cData = 0xD0;
						i2cReg = 0x05 + PR1000_OFFSETADDR_PTZ_CH(prChn);
						if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
					}
					else if(camResol == pr1000_inresol_1280x720p30)
					{
						/* RX_FREQ_FIRST[23:16] */
						i2cData = 0x00;
						i2cReg = 0x03 + PR1000_OFFSETADDR_PTZ_CH(prChn);
						if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
						/* RX_FREQ_FIRST[15:08] */
						i2cData = 0x0E;
						i2cReg = 0x04 + PR1000_OFFSETADDR_PTZ_CH(prChn);
						if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
						/* RX_FREQ_FIRST[07:00] */
						i2cData = 0x20;
						i2cReg = 0x05 + PR1000_OFFSETADDR_PTZ_CH(prChn);
						if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(-1);
						}
					}


					/* RX_FREQ[23:16] */
					i2cData = 0x36;
					i2cReg = 0x06 + PR1000_OFFSETADDR_PTZ_CH(prChn);
					if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}
					/* RX_FREQ[15:08] */
					i2cData = 0x00;
					i2cReg = 0x07 + PR1000_OFFSETADDR_PTZ_CH(prChn);
					if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}
					/* RX_FREQ[07:00] */
					i2cData = 0x00;
					i2cReg = 0x08 + PR1000_OFFSETADDR_PTZ_CH(prChn);
					if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}

					/* RX_LPF_LEN[05:00] */
					i2cMask = 0x3F; i2cData = 0x00;
					i2cReg = 0x09 + PR1000_OFFSETADDR_PTZ_CH(prChn);
					if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}
					/* PTZ_RX_VSTART_OS[10:08] */
					i2cMask = 0x07; i2cData = 0x02;
					i2cReg = 0x1A + PR1000_OFFSETADDR_PTZ_CH(prChn);
					if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}
					/* PTZ_RX_VSTART_OS[07:00] */
					i2cData = 0xE4;
					i2cReg = 0x1B + PR1000_OFFSETADDR_PTZ_CH(prChn);
					if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}

					/* PTZ_RX_EN[3] IGNORE_FRM_EN */
					i2cMask = 0x08; i2cData = 0x00;
					i2cReg = 0x00 + PR1000_OFFSETADDR_PTZ_CH(prChn);
					if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}

				}/*}}}*/

				/* make ptz receiving environment.(PTZ_SLICE_LEVEL0) */
				i2cReg = 0x62 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
				i2cData = 0x30;
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_VDEC_PAGE(prChn), i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}/*}}}*/
			break;
		default:
			{
			}
			break;
	}

	// Enable Ptz interrupt.
	i2cReg = 0xA0+prChn;
	i2cData = 0xBF;
	if( PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_COMMON, i2cReg, i2cData) < 0)
	{
		ErrorString("PageWrite.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_PTZ_CheckSendTxStatus(const int fd, const _stPortChSel *pstPortChSel)
{
	int ret = -1;
	uint8_t i2cReg, i2cData;

	uint8_t txFifoSize = 0;

	uint8_t txBusy = 0;
	uint8_t txStart = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;
	Dbg("mapChn:%d, prChip:%d, prChn:%d\n", mapChn COMMA prChip COMMA prChn);

	/* check tx status */
	i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_SIZE + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	txFifoSize = i2cData;
	Dbg("tx Fifo:%d\n", txFifoSize);

	if((txFifoSize == 0)) 
	{
		/* check tx status */
		i2cReg = 0x20 + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
		{
			ErrorString("Read reg.\n");
			return(-1);
		}
		txStart = i2cData;

		i2cReg = 0xC0 + prChn;
		if( (PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_COMMON, i2cReg, &i2cData)) < 0)
		{
			ErrorString("Read reg.\n");
			return(-1);
		}
		txBusy = i2cData;
                if( (txBusy&0x1) || (txStart&0x40) ) return(-1);

		ret = 0;
	}
	else
	{
		Dbg("txFifoSize:%d.\n", txFifoSize);
		ret = -1;
	}

	return(ret);
}

#ifndef DONT_SUPPORT_PTZ_FUNC
#ifndef DONT_SUPPORT_PTZ_ETC_CMD
int PR1000_PTZ_ResolveRecvData(const int fd, const uint8_t format, const uint8_t resol, const uint8_t *pData, const uint8_t length)
{
	int ret = -1;

	int i = 0;
	uint8_t cmdType = 0;
	const uint8_t *pPtzCmd;
	uint8_t ptzCmdLength;
	uint8_t cmdData;
	char bFindCmd = FALSE;
	uint32_t cmdArgu;
	uint8_t datInx;
	static int bPr1000PtzDoneCmd = FALSE;
	uint8_t camResol = resol; 
	uint8_t txLineCnt = 0;

	if( (format >= max_pr1000_format) || (resol >= max_pr1000_outresol) )
	{
		ErrorString("invalid type.\n");
		return(-1);
	}

	/* convert cif,shrink resolution to camera resolution. */
	switch(resol)
	{
		case pr1000_outresol_1280x720p60c: camResol = pr1000_outresol_1280x720p60; break;
		case pr1000_outresol_1280x720p50c: camResol = pr1000_outresol_1280x720p50; break;
		case pr1000_outresol_1280x720p30c: camResol =  pr1000_outresol_1280x720p30; break;
		case pr1000_outresol_1280x720p25c: camResol =  pr1000_outresol_1280x720p25; break;
		case pr1000_outresol_1920x1080p30c: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25c: camResol =  pr1000_outresol_1920x1080p25; break;
		case pr1000_outresol_1920x1080p30s: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25s: camResol =  pr1000_outresol_1920x1080p25; break;
		default: break;
	}

	switch(format)
	{
		case pr1000_format_SD720:
		case pr1000_format_SD960:
			{/*{{{*/
				DbgString("ptz SD data.\n");
				/* Find valid ptz command from table */
				cmdArgu = 0;
				bFindCmd = FALSE;
				for(cmdType = pr1000_ptz_table_command_OSD_TOP; cmdType < max_pr1000_ptz_table_command; cmdType++)
				{
					pPtzCmd = pr1000_ptz_table_sdcmd[cmdType].pCmd;
					ptzCmdLength = pr1000_ptz_table_sdcmd[cmdType].length;
					if( (pPtzCmd == NULL) || (ptzCmdLength == 0) ) continue;
					if( (ptzCmdLength >= length) && (ptzCmdLength % length) ) continue;

					bFindCmd = FALSE;
					for(i = 0, datInx = 0; i < ptzCmdLength; i++)
					{
						if(pr1000_ptz_table_sd_CMD_VALID[i] == 0) continue; // only check valid byte.

						if(pData[datInx] != pPtzCmd[i])
						{
							bFindCmd = FALSE;
							Dbg("Not cmd[%d(%s) 0x%02x/0x%02x].\n", i COMMA _STR_PR1000_PTZ_COMMAND[cmdType] COMMA pData[i] COMMA pPtzCmd[i]);
							break;
						}
						else
						{
							bFindCmd = TRUE;
							datInx++;
						}
					}

					if(bFindCmd == TRUE) 
					{
						cmdArgu = 0;
						break;
					}
				}

				if(bFindCmd == TRUE)
				{
					/* command argu */
					if(cmdType < pr1000_ptz_table_command_RESERVED0)
					{
						Print("Recv!! [%s(argu:0x%04x)].\n", _STR_PR1000_PTZ_COMMAND[cmdType] COMMA cmdArgu);
					}
					ret = cmdType;
				}
				else
				{
					ErrorString("Unknown cmd data.\n");
#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
					printk("%02d: ", length);
					for(i = 0; i < length; i++)
					{
						printk("0x%02x ", pData[i]);
					}
					printk("----\n");
#else
					printf("%02d: ", length);
					for(i = 0; i < length; i++)
					{
						printf("0x%02x ", pData[i]);
					}
					printf("----\n\r");
#endif
#endif

				}
			}/*}}}*/
			break;
#ifndef DONT_SUPPORT_STD_PVI
		case pr1000_format_PVI:
			{/*{{{*/
				uint32_t cmdParity = 0;

				DbgString("ptz PVI data.\n");

				/* check sync code */
				if(pData[3] != 0xAA)
				{
					Dbg("Sync 0x%02x/0x%02x.\n", pData[3] COMMA 0xAA);
					break;
				}
				else 
				{
					bPr1000PtzDoneCmd = FALSE; // Should do. When sync code, set false.
				}

				if( bPr1000PtzDoneCmd == TRUE) // Should do. When sync code, set false.
				{
					/* Already done. Escape repeat command with out sync code. */
					break;
				}

				/* calculate parity(6byte) */
				for(i = 0; i < 6; i++)
				{
					if( i==0 ) { cmdParity = pData[i]; }
					else { cmdParity += pData[i]; }
				}
				cmdParity %= 256;
				Dbg("Parity(%d) 0x%02x|0x%02x.\n", i COMMA pData[i] COMMA (uint8_t)cmdParity);
				if(pData[i] != (uint8_t)cmdParity)
				{
					Error("Parity(%d) 0x%02x/0x%02x.\n", i COMMA pData[i] COMMA (uint8_t)cmdParity);
#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
					for(i = 0; i < length; i++)
					{
						printk("0x%02x ", pData[i]);
					}
					printk("----\n");
#else
					for(i = 0; i < length; i++)
					{
						printf("0x%02x ", pData[i]);
					}
					printf("----\n\r");
#endif
#endif
					break; /* ignore parity error return. */
				}

				/* Find valid ptz command from table */
				cmdArgu = 0;
				bFindCmd = FALSE;
				for(cmdType = pr1000_ptz_table_command_OSD_TOP; cmdType < max_pr1000_ptz_table_command; cmdType++)
				{
					pPtzCmd = pr1000_ptz_table_pvicmd[cmdType].pCmd;
					ptzCmdLength = pr1000_ptz_table_pvicmd[cmdType].length;
					if( (pPtzCmd == NULL) || (ptzCmdLength == 0) ) continue;
					if( (ptzCmdLength >= length) && (ptzCmdLength % length) ) continue;

					bFindCmd = FALSE;
					for(i = 0, datInx = 0; i < ptzCmdLength; i++, datInx++)
					{
						if(pr1000_ptz_table_pvi_CMD_VALID[i] == 0) continue; // only check valid byte.

						if(pData[datInx] != pPtzCmd[i])
						{
							bFindCmd = FALSE;
							Dbg("Not cmd[%d(%s) 0x%02x/0x%02x].\n", i COMMA _STR_PR1000_PTZ_COMMAND[cmdType] COMMA pData[i] COMMA pPtzCmd[i]);
							break;
						}
						else
						{
							bFindCmd = TRUE;
						}
					}

					if(bFindCmd == TRUE) 
					{
						cmdArgu = (pData[3]<<8) | (pData[4]);
						break;
					}
				}

				if(bFindCmd == TRUE)
				{
					/* command argu */
					if(cmdType < pr1000_ptz_table_command_RESERVED0)
					{
						bPr1000PtzDoneCmd = TRUE; // Should do. When sync code, set false.
						Print("Recv!! [%s(argu:0x%04x)].\n", _STR_PR1000_PTZ_COMMAND[cmdType] COMMA cmdArgu);
					}
					ret = cmdType;
				}
				else
				{
					ErrorString("Unknown cmd data.\n");
#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
					printk("%02d: ", length);
					for(i = 0; i < length; i++)
					{
						printk("0x%02x ", pData[i]);
					}
					printk("----\n");
#else
					printf("%02d: ", length);
					for(i = 0; i < length; i++)
					{
						printf("0x%02x ", pData[i]);
					}
					printf("----\n\r");
#endif
#endif
				}
			}/*}}}*/
			break;
#endif // DONT_SUPPORT_STD_PVI
		case pr1000_format_HDA:
			{/*{{{*/
				Dbg("ptz HDA data. resol(%d).\n", camResol);

				if( (camResol == pr1000_outresol_1920x1080p30) || (camResol == pr1000_outresol_1920x1080p25) ) /* 1080p30, 1080p25 */
				{/*{{{*/
					/* check hda special escape sync code */
					/* check 0x00,0x00,0x00,0x00 or 0xAA sync code */
					if( (pData[0] == pr1000_ptz_table_hda_ESCAPE_CODE0_2Byte_1080p[0]) && 
							(pData[1] == pr1000_ptz_table_hda_ESCAPE_CODE0_2Byte_1080p[1]) )
					{
						bPr1000PtzDoneCmd = FALSE; // Should do. When sync code, set false.
						DbgString("ESCAPE Code0.\n");
						cmdType = pr1000_ptz_table_command_RESERVED0;
						ret = 0;
						break;
					}
					else if( (pData[0] == pr1000_ptz_table_hda_ESCAPE_CODE1_1Byte_1080p[0]) )
					{
						DbgString("ESCAPE Code1.\n");
						cmdType = pr1000_ptz_table_command_RESERVED0;
						ret = 0;
						break;
					}

					if( bPr1000PtzDoneCmd == TRUE) // Should do. When sync code, set false.
					{
						/* Already done. Escape repeat command with out sync code. */
						break;
					}

					/* Find valid ptz command from table */
					cmdArgu = 0;
					bFindCmd = FALSE;
					for(cmdType = pr1000_ptz_table_command_OSD_TOP; cmdType < max_pr1000_ptz_table_command; cmdType++)
					{
						pPtzCmd = pr1000_ptz_table_hdacmd_1080p[cmdType].pCmd;
						ptzCmdLength = pr1000_ptz_table_hdacmd_1080p[cmdType].length;
						if( (pPtzCmd == NULL) || (ptzCmdLength == 0) ) continue;
						if( (ptzCmdLength >= length) && (ptzCmdLength % length) ) continue;

						bFindCmd = FALSE;
						for(i = 0, datInx = 0; i < ptzCmdLength; i++)
						{
							if(pr1000_ptz_table_hda_CMD_VALID_1080p[i] == 0) continue; // only check valid byte.

							if(pData[datInx] != pPtzCmd[i])
							{
								bFindCmd = FALSE;
								Dbg("Not cmd[%d(%s) 0x%02x/0x%02x].\n", i COMMA _STR_PR1000_PTZ_COMMAND[cmdType] COMMA pData[i] COMMA pPtzCmd[i]);
								break;
							}
							else
							{
								bFindCmd = TRUE;
								datInx++;
							}
						}

						if(bFindCmd == TRUE) 
						{
							if( length >= 4 ) 
							{
								/* 1080p case */
								cmdArgu = (pData[2]<<8) | (pData[3]);
							}
							else
							{
								/* 720p case */
								cmdArgu = 0;
							}
							break;
						}
					}
				}/*}}}*/
				else if( (camResol == pr1000_outresol_1280x720p60) || (camResol == pr1000_outresol_1280x720p50) || 
					(camResol == pr1000_outresol_1280x720p30) || (camResol == pr1000_outresol_1280x720p25) )
				{/*{{{*/
					/* check hda special escape sync code */
					/* check 0x00,0x00,0x00,0x00 */
					if( (pData[0] == pr1000_ptz_table_hda_ESCAPE_CODE0_4Byte_720p[0]) && 
							(pData[1] == pr1000_ptz_table_hda_ESCAPE_CODE0_4Byte_720p[1]) &&
							(pData[2] == pr1000_ptz_table_hda_ESCAPE_CODE0_4Byte_720p[2]) && 
							(pData[3] == pr1000_ptz_table_hda_ESCAPE_CODE0_4Byte_720p[3]) )
					{
						bPr1000PtzDoneCmd = FALSE; // Should do. When sync code, set false.
						DbgString("ESCAPE Code0.\n");
						cmdType = pr1000_ptz_table_command_RESERVED0;
						ret = 0;
						break;
					}
					else
					{
						bPr1000PtzDoneCmd = FALSE; // Should do. When sync code, set false.
					}

					if( bPr1000PtzDoneCmd == TRUE) // Should do. When sync code, set false.
					{
						/* Already done. Escape repeat command with out sync code. */
						break;
					}

					/* Find valid ptz command from table */
					cmdArgu = 0;
					bFindCmd = FALSE;
					for(cmdType = pr1000_ptz_table_command_OSD_TOP; cmdType < max_pr1000_ptz_table_command; cmdType++)
					{
						pPtzCmd = pr1000_ptz_table_hdacmd_720p[cmdType].pCmd;
						ptzCmdLength = pr1000_ptz_table_hdacmd_720p[cmdType].length;
						if( (pPtzCmd == NULL) || (ptzCmdLength == 0) ) continue;
						if( (ptzCmdLength >= length) && (ptzCmdLength % length) ) continue;

						bFindCmd = FALSE;
						for(i = 0, datInx = 0; i < ptzCmdLength; i++)
						{
							if(pr1000_ptz_table_hda_CMD_VALID_720p[i] == 0) continue; // only check valid byte.

							if(pData[datInx] != pPtzCmd[i])
							{
								bFindCmd = FALSE;
								Dbg("Not cmd[%d(%s) 0x%02x/0x%02x].\n", i COMMA _STR_PR1000_PTZ_COMMAND[cmdType] COMMA pData[i] COMMA pPtzCmd[i]);
								break;
							}
							else
							{
								bFindCmd = TRUE;
								datInx++;
							}
						}

						if(bFindCmd == TRUE) 
						{
							/* 720p case */
							cmdArgu = 0;
							break;
						}
					}
				}/*}}}*/

				if(bFindCmd == TRUE)
				{
					/* command argu */
					if(cmdType < pr1000_ptz_table_command_RESERVED0)
					{
						bPr1000PtzDoneCmd = TRUE; // Should do. When sync code, set false.
						Print("Recv!!! [%s(argu:0x%04x)].\n", _STR_PR1000_PTZ_COMMAND[cmdType] COMMA cmdArgu);
					}
					ret = cmdType;
				}
				else
				{
					ErrorString("Unknown cmd data.\n");
#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
					printk("%02d: ", length);
					for(i = 0; i < length; i++)
					{
						printk("0x%02x ", pData[i]);
					}
					printk("----\n");
#else
					printf("%02d: ", length);
					for(i = 0; i < length; i++)
					{
						printf("0x%02x ", pData[i]);
					}
					printf("----\n\r");
#endif
#endif
				}
			}/*}}}*/
			break;
		case pr1000_format_CVI:
			{/*{{{*/
				uint32_t cmdParity = 0;
				uint8_t cmdByte4, cmdByte5;
				uint8_t cmdSAddr;

				DbgString("ptz CVI data.\n");

				/* check sync code */
				if(pData[0] != 0xA5)
				{
					Dbg("Sync 0x%02x/0x%02x.\n", pData[0] COMMA 0xA5);
					break;
				}
				else 
				{
					bPr1000PtzDoneCmd = FALSE; // Should do. When sync code, set false.
				}

				if( bPr1000PtzDoneCmd == TRUE) // Should do. When sync code, set false.
				{
					/* Already done. Escape repeat command with out sync code. */
					break;
				}

				if( (camResol >= pr1000_outresol_1280x720p60) && (camResol <= pr1000_outresol_1920x1080p25) )
				{
					txLineCnt = ((const _stPTZTxParam *)&pr1000_ptz_txparam_cvi_def[camResol-pr1000_outresol_1280x720p60])->tx_line_cnt;
				}
				else
				{
					ErrorString("Invalid resol.\n");
				}


				/* calculate parity(6byte) */
				for(i = 0; i < txLineCnt; i++)
				{
					if( i==0 ) { cmdParity = pData[i]; }
					else { cmdParity += pData[i]; }
				}
				cmdParity %= 256;
				Dbg("Parity(%d) 0x%02x|0x%02x.\n", i COMMA pData[i] COMMA (uint8_t)cmdParity);
				if(pData[i] != (uint8_t)cmdParity)
				{
					Error("Parity(%d) 0x%02x/0x%02x.\n", i COMMA pData[i] COMMA (uint8_t)cmdParity);
#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
					for(i = 0; i < length; i++)
					{
						printk("0x%02x ", pData[i]);
					}
					printk("----\n");
#else
					for(i = 0; i < length; i++)
					{
						printf("0x%02x ", pData[i]);
					}
					printf("----\n\r");
#endif
#endif
					break; /* ignore parity error return. */
				}

				/* Find valid ptz command from table */
				cmdData = pData[2]; // command code byte. reference datasheet.
				cmdByte4 = pData[3]; // command code byte. reference datasheet.
				cmdByte5 = pData[4]; // command code byte. reference datasheet.
				cmdSAddr = pData[5]; // source address code byte. reference datasheet.
				cmdArgu = 0;
				bFindCmd = FALSE;
				Dbg("cmdData:0x%02x,data(0x%02x,0x%02x).\n", cmdData COMMA cmdByte4 COMMA cmdByte5);
				if(cmdData & 0x80) //other command
				{/*{{{*/
					for(cmdType = pr1000_ptz_cvi_table_command_OTHER_SET_PRESET; cmdType < max_pr1000_ptz_cvi_table_command_other; cmdType++)
					{
						if(cmdData == pr1000_ptz_table_cvi_OTHER_CMD[cmdType])
						{
							if(cmdData == pr1000_ptz_table_cvi_OTHER_CMD[pr1000_ptz_cvi_table_command_OTHER_SET_PRESET]) //0x81
							{/*{{{*/
								if( ((cmdByte4==0)) && ((1<=cmdByte5)&&(cmdByte5<=80)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_SET_PRESET;
									bFindCmd = TRUE;
									break;
								}
								else if( ((cmdByte4==0)) && ((81<=cmdByte5)&&(cmdByte5<=90)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_SET_SCAN_LR_LIMIT;
									bFindCmd = TRUE;
									break;
								}
								else if( ((3<=cmdByte4)&&(cmdByte4<=255)) && ((91<=cmdByte5)&&(cmdByte5<=95)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_SET_SCAN_RATE;
									bFindCmd = TRUE;
									break;
								}
								else if( ((3<=cmdByte4)&&(cmdByte4<=255)) && ((cmdByte5==96)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_SET_TOUR_STAY;
									bFindCmd = TRUE;
									break;
								}
								else if( ((cmdByte5==98)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_H360_CONT_RSPEED;
									bFindCmd = TRUE;
									break;
								}
								else if( ((cmdByte5==99)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_SET_TOUR_SPEED;
									bFindCmd = TRUE;
									break;
								}
								else
								{
									bFindCmd = FALSE;
									break;
								}
							}/*}}}*/
							else if(cmdData == pr1000_ptz_table_cvi_OTHER_CMD[pr1000_ptz_cvi_table_command_OTHER_CALL_PRESET]) //0x83
							{/*{{{*/
								if( ((cmdByte4==0)) && ((1<=cmdByte5)&&(cmdByte5<=80)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_CALL_PRESET;
									bFindCmd = TRUE;
									break;
								}
								else if( ((cmdByte4==0)) && ((cmdByte5==90)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_SCAN_TOUR_PAT_STOP;
									bFindCmd = TRUE;
									break;
								}
								else if( ((cmdByte4==0)) && ((91<=cmdByte5)&&(cmdByte5<=95)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_BEGIN_TO_SCAN;
									bFindCmd = TRUE;
									break;
								}
								else if( ((cmdByte4==0)||(cmdByte4==1)) && ((cmdByte5==98)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_H360_CONT_ROT;
									bFindCmd = TRUE;
									break;
								}
								else if( (cmdByte4<=7) && ((cmdByte5==99)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_BEGIN_TO_TOUR;
									bFindCmd = TRUE;
									break;
								}
								else
								{
									bFindCmd = FALSE;
									break;
								}
							}/*}}}*/
							else if(cmdData == pr1000_ptz_table_cvi_OTHER_CMD[pr1000_ptz_cvi_table_command_OTHER_DELETE_PRESET]) //0x82
							{/*{{{*/
								if( (cmdByte4<=7) && ((1<=cmdByte5)&&(cmdByte5<=80)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_DELETE_PRESET;
									bFindCmd = TRUE;
									break;
								}
								else if( (cmdByte4<=7) && ((cmdByte5==81)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_DELETE_ALL_PRESET;
									bFindCmd = TRUE;
									break;
								}
								else
								{
									bFindCmd = FALSE;
									break;
								}
							}/*}}}*/
							else if(cmdData == pr1000_ptz_table_cvi_OTHER_CMD[pr1000_ptz_cvi_table_command_OTHER_ADD_TOUR]) //0x84
							{/*{{{*/
								if( (cmdByte4<=7) && ((1<=cmdByte5)&&(cmdByte5<=80)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_ADD_TOUR;
									bFindCmd = TRUE;
									break;
								}
								else
								{
									bFindCmd = FALSE;
									break;
								}
							}/*}}}*/
							else if(cmdData == pr1000_ptz_table_cvi_OTHER_CMD[pr1000_ptz_cvi_table_command_OTHER_SET_PAT_TO_BEGIN]) //0x85
							{/*{{{*/
								if( (cmdByte4<=4) && ((cmdByte5==1)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_SET_PAT_TO_BEGIN;
									bFindCmd = TRUE;
									break;
								}
								else if( (cmdByte4<=4) && ((cmdByte5==2)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_SET_PAT_TO_STOP;
									bFindCmd = TRUE;
									break;
								}
								else if( (cmdByte4<=4) && ((cmdByte5==0)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_BEGIN_TO_PAT;
									bFindCmd = TRUE;
									break;
								}
								else
								{
									bFindCmd = FALSE;
									break;
								}
							}/*}}}*/
							else if(cmdData == pr1000_ptz_table_cvi_OTHER_CMD[pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP]) //0x89
							{/*{{{*/
								if( ((cmdByte4==0)) /*&& ((cmdByte5==0))*/ ) //don't care cmdByte5
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_CLOSE;
									bFindCmd = TRUE;
									break;
								}
								else if( ((cmdByte4==1)) /*&& ((cmdByte5==0))*/ ) //don't care cmdByte5
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_OPEN;
									bFindCmd = TRUE;
									break;
								}
								else if( ((cmdByte4==2)) /*&& ((cmdByte5==0))*/ ) //don't care cmdByte5
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_MVBK;
									bFindCmd = TRUE;
									break;
								}
								else if( ((cmdByte4==3)) /*&& ((cmdByte5==0))*/ ) //don't care cmdByte5
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_ENTERNEXT;
									bFindCmd = TRUE;
									break;
								}
								else if( ((cmdByte4==4)) /*&& ((cmdByte5==0))*/ ) //don't care cmdByte5
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_MVUP;
									bFindCmd = TRUE;
									break;
								}
								else if( ((cmdByte4==5)) /*&& ((cmdByte5==0))*/ ) //don't care cmdByte5
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_MVDW;
									bFindCmd = TRUE;
									break;
								}
								else if( ((cmdByte4==6)) /*&& ((cmdByte5==0))*/ ) //don't care cmdByte5
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_MVLF;
									bFindCmd = TRUE;
									break;
								}
								else if( ((cmdByte4==7)) /*&& ((cmdByte5==0))*/ ) //don't care cmdByte5
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_MVRG;
									bFindCmd = TRUE;
									break;
								}
								else if( ((cmdByte4==8)) /*&& ((cmdByte5==0))*/ ) //don't care cmdByte5
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_CONFIRM;
									bFindCmd = TRUE;
									break;
								}
								else
								{
									bFindCmd = FALSE;
									break;
								}
							}/*}}}*/
							else if(cmdData == pr1000_ptz_table_cvi_OTHER_CMD[pr1000_ptz_cvi_table_command_OTHER_RESET_SELF_CHK]) //0x8D
							{/*{{{*/
								if( ((cmdByte4==0)) && ((cmdByte5==1)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_RESET_SELF_CHK;
									bFindCmd = TRUE;
									break;
								}
								else if( ((cmdByte4==0)) && ((cmdByte5==0)) )
								{
									cmdType = pr1000_ptz_cvi_table_command_OTHER_FACTORY_DEFSET;
									bFindCmd = TRUE;
									break;
								}
								else
								{
									bFindCmd = FALSE;
									break;
								}
							}/*}}}*/
							else
							{/*{{{*/
								bFindCmd = FALSE;
								break;
							}/*}}}*/
						}
					}

					if(bFindCmd == TRUE)
					{
						bPr1000PtzDoneCmd = TRUE; // Should do. When sync code, set false.
						cmdArgu = (cmdByte4<<8) | cmdByte5;
						Print("Recv!! [%s(argu:0x%04x, SA:0x%02x)].\n", _STR_PR1000_CVI_PTZ_COMMAND_OTHER[cmdType] COMMA cmdArgu COMMA cmdSAddr);
					}
				}/*}}}*/
				else if(cmdData & 0x40) //lens command
				{/*{{{*/
					for(cmdType = pr1000_ptz_cvi_table_command_LENS_TELE; cmdType < max_pr1000_ptz_cvi_table_command_lens; cmdType++)
					{
						if(cmdData == pr1000_ptz_table_cvi_LENS_CMD[cmdType])
						{
							bFindCmd = TRUE;
							break;
						}
					}

					if(bFindCmd == TRUE)
					{
						if(cmdType < pr1000_ptz_cvi_table_command_LENS_RESERVED0)
						{
							bPr1000PtzDoneCmd = TRUE; // Should do. When sync code, set false.
							cmdArgu = (cmdByte4<<8) | cmdByte5;
							Print("Recv!! [%s(argu:0x%04x, SA:0x%02x)].\n", _STR_PR1000_CVI_PTZ_COMMAND_LENS[cmdType] COMMA cmdArgu COMMA cmdSAddr);
						}
					}
				}/*}}}*/
				else if( (cmdData & 0xF0) == 0) //ptz command
				{/*{{{*/
					for(cmdType = pr1000_ptz_cvi_table_command_PTZ_RIGHT; cmdType < max_pr1000_ptz_cvi_table_command_ptz; cmdType++)
					{
						if(cmdData == pr1000_ptz_table_cvi_PTZ_CMD[cmdType])
						{
							bFindCmd = TRUE;
							break;
						}
					}

					if(bFindCmd == TRUE)
					{
						if(cmdType < pr1000_ptz_cvi_table_command_PTZ_RESERVED0)
						{
							bPr1000PtzDoneCmd = TRUE; // Should do. When sync code, set false.
							cmdArgu = (cmdByte4<<8) | cmdByte5;
							Print("Recv!! [%s(argu:0x%04x, SA:0x%02x)].\n", _STR_PR1000_CVI_PTZ_COMMAND_PTZ[cmdType] COMMA cmdArgu COMMA cmdSAddr);
						}
					}
				}/*}}}*/
				else
				{
					Error("Unknown command(0x%02x).\n", cmdData);
					ret = cmdType;
				}


				if(bFindCmd == TRUE)
				{
					ret = cmdType;
				}
				else
				{
					ErrorString("Unknown cmd data.\n");
#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
					printk("%02d: ", length);
					for(i = 0; i < length; i++)
					{
						printk("0x%02x ", pData[i]);
					}
					printk("----\n");
#else
					printf("%02d: ", length);
					for(i = 0; i < length; i++)
					{
						printf("0x%02x ", pData[i]);
					}
					printf("----\n\r");
#endif
#endif
				}
			}/*}}}*/
			break;
		case pr1000_format_HDT:
			{/*{{{*/
				uint32_t cmdParity = 0;

				DbgString("ptz HDT data.\n");

				/* check sync code */
				if(pData[0] != 0xB5)
				{
					Dbg("Sync 0x%02x/0x%02x.\n", pData[0] COMMA 0xB5);
					break;
				}
				else
				{
					bPr1000PtzDoneCmd = FALSE; // Should do. When sync code, set false.
				}

				if( bPr1000PtzDoneCmd == TRUE) // Should do. When sync code, set false.
				{
					/* Already done. Escape repeat command with out sync code. */
					break;
				}

				/* calculate parity(7byte) */
				for(i = 0; i < 7; i++)
				{
					if( i==0 ) { cmdParity = pData[i]; }
					else { cmdParity += pData[i]; }
				}
				cmdParity %= 256;
				Dbg("Parity(%d) 0x%02x|0x%02x.\n", i COMMA pData[i] COMMA (uint8_t)cmdParity);
				if(pData[i] != (uint8_t)cmdParity)
				{
					Error("Parity(%d) 0x%02x/0x%02x.\n", i COMMA pData[i] COMMA (uint8_t)cmdParity);
#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
					for(i = 0; i < length; i++)
					{
						printk("0x%02x ", pData[i]);
					}
					printk("----\n");
#else
					for(i = 0; i < length; i++)
					{
						printf("0x%02x ", pData[i]);
					}
					printf("----\n\r");
#endif
#endif
					break; /* ignore parity error return. */
				}

				/* Find valid ptz command from table */
				cmdArgu = 0;
				bFindCmd = FALSE;
				for(cmdType = pr1000_ptz_table_command_OSD_TOP; cmdType < max_pr1000_ptz_table_command; cmdType++)
				{
					pPtzCmd = pr1000_ptz_table_hdtcmd[cmdType].pCmd;
					ptzCmdLength = pr1000_ptz_table_hdtcmd[cmdType].length;
					if( (pPtzCmd == NULL) || (ptzCmdLength == 0) ) continue;
					if( (ptzCmdLength >= length) && (ptzCmdLength % length) ) continue;

					bFindCmd = FALSE;
					for(i = 0, datInx = 0; i < ptzCmdLength; i++, datInx++)
					{
						if(pr1000_ptz_table_hdt_CMD_VALID[i] == 0) continue; // only check valid byte.

						if(pData[datInx] != pPtzCmd[i])
						{
							bFindCmd = FALSE;
							Dbg("Not cmd[%d(%s) 0x%02x/0x%02x].\n", i COMMA _STR_PR1000_PTZ_COMMAND[cmdType] COMMA pData[i] COMMA pPtzCmd[i]);
							break;
						}
						else
						{
							bFindCmd = TRUE;
						}
					}

					if(bFindCmd == TRUE) 
					{
						cmdArgu = (pData[3]<<8) | (pData[4]);
						break;
					}
				}

				if(bFindCmd == TRUE)
				{
					/* command argu */
					if(cmdType < pr1000_ptz_table_command_RESERVED0)
					{
						bPr1000PtzDoneCmd = TRUE; // Should do. When sync code, set false.
						Print("Recv!! [%s(argu:0x%04x)].\n", _STR_PR1000_PTZ_COMMAND[cmdType] COMMA cmdArgu);
					}
					ret = cmdType;
				}
				else
				{
					ErrorString("Unknown cmd data.\n");
#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
					printk("%02d: ", length);
					for(i = 0; i < length; i++)
					{
						printk("0x%02x ", pData[i]);
					}
					printk("----\n");
#else
					printf("%02d: ", length);
					for(i = 0; i < length; i++)
					{
						printf("0x%02x ", pData[i]);
					}
					printf("----\n\r");
#endif
#endif
				}
			}/*}}}*/
			break;
		case pr1000_format_HDT_NEW:
			{/*{{{*/
				uint32_t cmdParity = 0;

				DbgString("ptz HDT_NEW data.\n");

				/* check sync code */
				if(pData[0] != 0xB5)
				{
					Dbg("Sync 0x%02x/0x%02x.\n", pData[0] COMMA 0xB5);
					break;
				}
				else
				{
					bPr1000PtzDoneCmd = FALSE; // Should do. When sync code, set false.
				}

				if( bPr1000PtzDoneCmd == TRUE) // Should do. When sync code, set false.
				{
					/* Already done. Escape repeat command with out sync code. */
					break;
				}

				/* calculate parity(7byte) */
				for(i = 0; i < 7; i++)
				{
					if( i==0 ) { cmdParity = pData[i]; }
					else { cmdParity += pData[i]; }
				}
				cmdParity %= 256;
				Dbg("Parity(%d) 0x%02x|0x%02x.\n", i COMMA pData[i] COMMA (uint8_t)cmdParity);
				if(pData[i] != (uint8_t)cmdParity)
				{
					Error("Parity(%d) 0x%02x/0x%02x.\n", i COMMA pData[i] COMMA (uint8_t)cmdParity);
#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
					for(i = 0; i < length; i++)
					{
						printk("0x%02x ", pData[i]);
					}
					printk("----\n");
#else
					for(i = 0; i < length; i++)
					{
						printf("0x%02x ", pData[i]);
					}
					printf("----\n\r");
#endif
#endif
					break; /* ignore parity error return. */
				}

				/* Find valid ptz command from table */
				cmdArgu = 0;
				bFindCmd = FALSE;
				for(cmdType = pr1000_ptz_table_command_OSD_TOP; cmdType < max_pr1000_ptz_table_command; cmdType++)
				{
					pPtzCmd = pr1000_ptz_table_hdt_newcmd[cmdType].pCmd;
					ptzCmdLength = pr1000_ptz_table_hdt_newcmd[cmdType].length;
					if( (pPtzCmd == NULL) || (ptzCmdLength == 0) ) continue;
					if( (ptzCmdLength >= length) && (ptzCmdLength % length) ) continue;

					bFindCmd = FALSE;
					for(i = 0, datInx = 0; i < ptzCmdLength; i++, datInx++)
					{
						if(pr1000_ptz_table_hdt_new_CMD_VALID[i] == 0) continue; // only check valid byte.

						if(pData[datInx] != pPtzCmd[i])
						{
							bFindCmd = FALSE;
							Dbg("Not cmd[%d(%s) 0x%02x/0x%02x].\n", i COMMA _STR_PR1000_PTZ_COMMAND[cmdType] COMMA pData[i] COMMA pPtzCmd[i]);
							break;
						}
						else
						{
							bFindCmd = TRUE;
						}
					}

					if(bFindCmd == TRUE) 
					{
						cmdArgu = (pData[3]<<8) | (pData[4]);
						break;
					}
				}

				if(bFindCmd == TRUE)
				{
					/* command argu */
					if(cmdType < pr1000_ptz_table_command_RESERVED0)
					{
						bPr1000PtzDoneCmd = TRUE; // Should do. When sync code, set false.
						Print("Recv!! [%s(argu:0x%04x)].\n", _STR_PR1000_PTZ_COMMAND[cmdType] COMMA cmdArgu);
					}
					ret = cmdType;
				}
				else
				{
					ErrorString("Unknown cmd data.\n");
#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
					printk("%02d: ", length);
					for(i = 0; i < length; i++)
					{
						printk("0x%02x ", pData[i]);
					}
					printk("----\n");
#else
					printf("%02d: ", length);
					for(i = 0; i < length; i++)
					{
						printf("0x%02x ", pData[i]);
					}
					printf("----\n\r");
#endif
#endif
				}
			}/*}}}*/
			break;

		default:
			{
				cmdType = 0;
				Error("invalid ptz format(%d).\n", format);
				ret = -1;
			}
			break;
	}

	return(ret);
}
#endif // DONT_SUPPORT_PTZ_ETC_CMD

int PR1000_PTZ_GetRxAttr(const int fd, const _stPortChSel *pstPortChSel, _stPTZRxAttr *pstPTZRxAttr)
{
	int ret = -1;

	uint8_t i2cReg = 0;
	int page;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstPTZRxAttr == NULL)
	{
		ErrorString("Invalid argu\n");
		return(-1);
	}

	/* Read ceq information registers. */
	page = PR1000_REG_PAGE_PTZ;
	i2cReg = 0x00 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	Dbg("PTZ GetRxRegs [mapChn:%d, %dp reg:0x%02x]\n", mapChn COMMA page COMMA i2cReg);
	if( (ret = PR1000_PageReadBurst(fd, i2cSlaveAddr, page, i2cReg, sizeof(_stPTZRxAttr), (uint8_t *)pstPTZRxAttr)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_PTZ_GetTxAttr(const int fd, const _stPortChSel *pstPortChSel, _stPTZTxAttr *pstPTZTxAttr)
{
	int ret = -1;

	uint8_t i2cReg = 0;
	int page;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstPTZTxAttr == NULL)
	{
		ErrorString("Invalid argu\n");
		return(-1);
	}

	/* Read ceq information registers. */
	page = PR1000_REG_PAGE_PTZ;
	i2cReg = 0x20 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	Dbg("PTZ GetTxRegs [mapChn:%d, %dp reg:0x%02x]\n", mapChn COMMA page COMMA i2cReg);
	if( (ret = PR1000_PageReadBurst(fd, i2cSlaveAddr, page, i2cReg, sizeof(_stPTZTxAttr), (uint8_t *)pstPTZTxAttr)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_PTZ_GetHVStartAttr(const int fd, const _stPortChSel *pstPortChSel, _stPTZHVStartAttr *pstPTZHVStartAttr)
{
	int ret = -1;

	uint8_t i2cReg = 0;
	int page;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstPTZHVStartAttr == NULL)
	{
		ErrorString("Invalid argu\n");
		return(-1);
	}

	page = PR1000_REG_PAGE_PTZ;
	i2cReg = 0x18 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	Dbg("PTZ GetHVStartRegs [mapChn:%d, %dp reg:0x%02x]\n", mapChn COMMA page COMMA i2cReg);
	if( (ret = PR1000_PageReadBurst(fd, i2cSlaveAddr, page, i2cReg, sizeof(_stPTZHVStartAttr), (uint8_t *)pstPTZHVStartAttr)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_PTZ_SetTxParam(const int fd, const _stPortChSel *pstPortChSel, const _stPTZTxParam *pstPTZTxParam)
{
	int ret = -1;

	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstPTZTxParam == NULL)
	{
		ErrorString("Invalid argu\n");
		return(-1);
	}
#ifdef DEBUG_PTZ_PR1000
	Print("tx_field_type:%x\n", pstPTZTxParam->tx_field_type); //reg 4x20 b[1:0]
	Print("tx_line_cnt:%x\n", pstPTZTxParam->tx_line_cnt); //reg 4x21 b[7:3]
	Print("tx_hst_os:%x\n", pstPTZTxParam->tx_hst_os); //reg 4x21 b[2:0]
	Print("tx_hst:%x\n", pstPTZTxParam->tx_hst); //reg 4x22 b[6:0]
	Print("tx_freq_first:0x%04x\n", pstPTZTxParam->tx_freq_first); //reg 4x23,4x24,4x25
	Print("tx_freq:0x%04x\n", pstPTZTxParam->tx_freq); //reg 4x26,4x27,4x28
	Print("tx_hpst:0x%02x\n", pstPTZTxParam->tx_hpst); //reg 4x29,4x2A b[12:0]
	Print("tx_line_len:%x\n", pstPTZTxParam->tx_line_len); //reg 4x2B b[5:0]
	Print("tx_all_data_len:%x\n", pstPTZTxParam->tx_all_data_len); //reg 4x2C b[7:0]
#endif // DEBUG_PTZ_PR1000

	//uint8_t tx_field_type; //reg 4x20 b[1:0]
	i2cMask = 0x03; i2cData = pstPTZTxParam->tx_field_type;
	i2cReg = 0x20 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t tx_line_cnt; //reg 4x21 b[7:3]
	i2cMask = 0xF8; i2cData = pstPTZTxParam->tx_line_cnt<<3;
	i2cReg = 0x21 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t tx_hst_os; //reg 4x21 b[2:0]
	i2cMask = 0x07; i2cData = pstPTZTxParam->tx_hst_os;
	i2cReg = 0x21 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t tx_hst; //reg 4x22 b[6:0]
	i2cMask = 0x7F; i2cData = pstPTZTxParam->tx_hst;
	i2cReg = 0x22 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint32_t tx_freq_first; //reg 4x23,4x24,4x25
	i2cData = (pstPTZTxParam->tx_freq_first>>16)&0xFF;
	i2cReg = 0x23 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cData = (pstPTZTxParam->tx_freq_first>>8)&0xFF;
	i2cReg = 0x24 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cData = (pstPTZTxParam->tx_freq_first)&0xFF;
	i2cReg = 0x25 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint32_t tx_freq; //reg 4x26,4x27,4x28
	i2cData = (pstPTZTxParam->tx_freq>>16)&0xFF;
	i2cReg = 0x26 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cData = (pstPTZTxParam->tx_freq>>8)&0xFF;
	i2cReg = 0x27 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cData = (pstPTZTxParam->tx_freq)&0xFF;
	i2cReg = 0x28 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
	i2cData = (pstPTZTxParam->tx_hpst>>8)&0xFF;
	i2cReg = 0x29 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cData = (pstPTZTxParam->tx_hpst)&0xFF;
	i2cReg = 0x2A + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t tx_line_len; //reg 4x2B b[5:0]
	i2cMask = 0x3F; i2cData = pstPTZTxParam->tx_line_len;
	i2cReg = 0x2B + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	i2cData = pstPTZTxParam->tx_all_data_len;
	i2cReg = 0x2C + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_PTZ_GetTxParam(const int fd, const _stPortChSel *pstPortChSel, _stPTZTxParam *pstPTZTxParam)
{
	int ret = -1;

	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstPTZTxParam == NULL)
	{
		ErrorString("Invalid argu\n");
		return(-1);
	}

	//uint8_t tx_field_type; //reg 4x20 b[1:0]
	i2cMask = 0x03; 
	i2cReg = 0x20 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZTxParam->tx_field_type = i2cData;
	//uint8_t tx_line_cnt; //reg 4x21 b[7:3]
	i2cMask = 0xF8; 
	i2cReg = 0x21 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZTxParam->tx_line_cnt = i2cData >> 3;
	//uint8_t tx_hst_os; //reg 4x21 b[2:0]
	i2cMask = 0x07; 
	i2cReg = 0x21 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZTxParam->tx_hst_os = i2cData;
	//uint8_t tx_hst; //reg 4x22 b[6:0]
	i2cMask = 0x7F; 
	i2cReg = 0x22 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZTxParam->tx_hst = i2cData;
	//uint32_t tx_freq_first; //reg 4x23,4x24,4x25
	i2cReg = 0x23 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZTxParam->tx_freq_first = (uint32_t)i2cData << 16;
	i2cReg = 0x24 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZTxParam->tx_freq_first |= (uint32_t)i2cData << 8;
	i2cReg = 0x25 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZTxParam->tx_freq_first |= i2cData;
	//uint32_t tx_freq; //reg 4x26,4x27,4x28
	i2cReg = 0x26 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZTxParam->tx_freq = (uint32_t)i2cData << 16;
	i2cReg = 0x27 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZTxParam->tx_freq |= (uint32_t)i2cData << 8;
	i2cReg = 0x28 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZTxParam->tx_freq |= i2cData;
	//uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
	i2cReg = 0x29 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZTxParam->tx_hpst = i2cData << 8;
	i2cReg = 0x2A + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZTxParam->tx_hpst |= i2cData; 
	//uint8_t tx_line_len; //reg 4x2B b[5:0]
	i2cMask = 0x3F; 
	i2cReg = 0x2B + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZTxParam->tx_line_len = i2cData;
	//uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	i2cReg = 0x2C + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZTxParam->tx_all_data_len = i2cData;

#ifdef DEBUG_PTZ_PR1000
	Print("tx_field_type:%x\n", pstPTZTxParam->tx_field_type); //reg 4x20 b[1:0]
	Print("tx_line_cnt:%x\n", pstPTZTxParam->tx_line_cnt); //reg 4x21 b[7:3]
	Print("tx_hst_os:%x\n", pstPTZTxParam->tx_hst_os); //reg 4x21 b[2:0]
	Print("tx_hst:%x\n", pstPTZTxParam->tx_hst); //reg 4x22 b[6:0]
	Print("tx_freq_first:0x%04x\n", pstPTZTxParam->tx_freq_first); //reg 4x23,4x24,4x25
	Print("tx_freq:0x%04x\n", pstPTZTxParam->tx_freq); //reg 4x26,4x27,4x28
	Print("tx_hpst:0x%02x\n", pstPTZTxParam->tx_hpst); //reg 4x29,4x2A b[12:0]
	Print("tx_line_len:%x\n", pstPTZTxParam->tx_line_len); //reg 4x2B b[5:0]
	Print("tx_all_data_len:%x\n", pstPTZTxParam->tx_all_data_len); //reg 4x2C b[7:0]
#endif // DEBUG_PTZ_PR1000


	return(ret);
}
#endif // DONT_SUPPORT_PTZ_FUNC

int PR1000_PTZ_SetRxParam(const int fd, const _stPortChSel *pstPortChSel, const _stPTZRxParam *pstPTZRxParam)
{
	int ret = -1;

	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstPTZRxParam == NULL)
	{
		ErrorString("Invalid argu\n");
		return(-1);
	}
#ifdef DEBUG_PTZ_PR1000
	Print("rx_field_type:%x\n", pstPTZRxParam->rx_field_type); //reg 4x00 b[1:0]
	Print("rx_line_cnt:%x\n", pstPTZRxParam->rx_line_cnt); //reg 4x01 b[7:3]
	Print("rx_hst_os:%x\n", pstPTZRxParam->rx_hst_os); //reg 4x01 b[2:0]
	Print("rx_hst:%x\n", pstPTZRxParam->rx_hst); //reg 4x02 b[6:0]
	Print("rx_freq_first:0x%04x\n", pstPTZRxParam->rx_freq_first); //reg 4x03,4x04,4x05
	Print("rx_freq:0x%04x\n", pstPTZRxParam->rx_freq); //reg 4x06,4x07,4x08
	Print("rx_lpf_len:%x\n", pstPTZRxParam->rx_lpf_len); //reg 4x09 b[5:0]
	Print("rx_h_pix_offset:%x\n", pstPTZRxParam->rx_h_pix_offset); //reg 4x0A b[7:0]
	Print("rx_line_len:%x\n", pstPTZRxParam->rx_line_len); //reg 4x0B b[5:0]
	Print("rx_valid_cnt:%x\n", pstPTZRxParam->rx_valid_cnt); //reg 4x0C b[7:0]
#endif // DEBUG_PTZ_PR1000

	//uint8_t rx_field_type; //reg 4x00 b[1:0]
	i2cMask = 0x03; i2cData = pstPTZRxParam->rx_field_type;
	i2cReg = 0x00 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t rx_line_cnt; //reg 4x01 b[7:3]
	i2cMask = 0xF8; i2cData = pstPTZRxParam->rx_line_cnt<<3;
	i2cReg = 0x01 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t rx_hst_os; //reg 4x01 b[2:0]
	i2cMask = 0x07; i2cData = pstPTZRxParam->rx_hst_os;
	i2cReg = 0x01 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t rx_hst; //reg 4x02 b[6:0]
	i2cMask = 0x7F; i2cData = pstPTZRxParam->rx_hst;
	i2cReg = 0x02 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint32_t rx_freq_first; //reg 4x03,4x04,4x05
	i2cData = (pstPTZRxParam->rx_freq_first>>16)&0xFF;
	i2cReg = 0x03 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cData = (pstPTZRxParam->rx_freq_first>>8)&0xFF;
	i2cReg = 0x04 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cData = (pstPTZRxParam->rx_freq_first)&0xFF;
	i2cReg = 0x05 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint32_t rx_freq; //reg 4x06,4x07,4x08
	i2cData = (pstPTZRxParam->rx_freq>>16)&0xFF;
	i2cReg = 0x06 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cData = (pstPTZRxParam->rx_freq>>8)&0xFF;
	i2cReg = 0x07 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cData = (pstPTZRxParam->rx_freq)&0xFF;
	i2cReg = 0x08 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t rx_lpf_len; //reg 4x09 b[5:0]
	i2cMask = 0x3F; i2cData = pstPTZRxParam->rx_lpf_len;
	i2cReg = 0x09 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
	i2cData = pstPTZRxParam->rx_h_pix_offset;
	i2cReg = 0x0A + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t rx_line_len; //reg 4x0B b[5:0]
	i2cMask = 0x3F; i2cData = pstPTZRxParam->rx_line_len;
	i2cReg = 0x0B + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	//uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	i2cData = pstPTZRxParam->rx_valid_cnt;
	i2cReg = 0x0C + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_PTZ_GetRxParam(const int fd, const _stPortChSel *pstPortChSel, _stPTZRxParam *pstPTZRxParam)
{
	int ret = -1;

	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstPTZRxParam == NULL)
	{
		ErrorString("Invalid argu\n");
		return(-1);
	}

	//uint8_t rx_field_type; //reg 4x00 b[1:0]
	i2cMask = 0x03; 
	i2cReg = 0x00 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZRxParam->rx_field_type = i2cData;
	//uint8_t rx_line_cnt; //reg 4x01 b[7:3]
	i2cMask = 0xF8; 
	i2cReg = 0x01 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZRxParam->rx_line_cnt = i2cData>>3;
	//uint8_t rx_hst_os; //reg 4x01 b[2:0]
	i2cMask = 0x07; 
	i2cReg = 0x01 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZRxParam->rx_hst_os = i2cData;
	//uint8_t rx_hst; //reg 4x02 b[6:0]
	i2cMask = 0x7F; 
	i2cReg = 0x02 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZRxParam->rx_hst = i2cData;
	//uint32_t rx_freq_first; //reg 4x03,4x04,4x05
	i2cReg = 0x03 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZRxParam->rx_freq_first = (uint32_t)i2cData << 16;
	i2cReg = 0x04 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZRxParam->rx_freq_first |= (uint32_t)i2cData << 8;
	i2cReg = 0x05 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZRxParam->rx_freq_first |= i2cData;
	//uint32_t rx_freq; //reg 4x06,4x07,4x08
	i2cReg = 0x06 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZRxParam->rx_freq = (uint32_t)i2cData << 16;
	i2cReg = 0x07 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZRxParam->rx_freq |= (uint32_t)i2cData << 8;
	i2cReg = 0x08 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZRxParam->rx_freq |= i2cData;
	//uint8_t rx_lpf_len; //reg 4x09 b[5:0]
	i2cMask = 0x3F; 
	i2cReg = 0x09 + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZRxParam->rx_lpf_len = i2cData;
	//uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
	i2cReg = 0x0A + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZRxParam->rx_h_pix_offset = i2cData;
	//uint8_t rx_line_len; //reg 4x0B b[5:0]
	i2cMask = 0x3F; 
	i2cReg = 0x0B + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZRxParam->rx_line_len = i2cData;
	//uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	i2cReg = 0x0C + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstPTZRxParam->rx_valid_cnt = i2cData;

#ifdef DEBUG_PTZ_PR1000
	Print("rx_field_type:%x\n", pstPTZRxParam->rx_field_type); //reg 4x00 b[1:0]
	Print("rx_line_cnt:%x\n", pstPTZRxParam->rx_line_cnt); //reg 4x01 b[7:3]
	Print("rx_hst_os:%x\n", pstPTZRxParam->rx_hst_os); //reg 4x01 b[2:0]
	Print("rx_hst:%x\n", pstPTZRxParam->rx_hst); //reg 4x02 b[6:0]
	Print("rx_freq_first:0x%04x\n", pstPTZRxParam->rx_freq_first); //reg 4x03,4x04,4x05
	Print("rx_freq:0x%04x\n", pstPTZRxParam->rx_freq); //reg 4x06,4x07,4x08
	Print("rx_lpf_len:%x\n", pstPTZRxParam->rx_lpf_len); //reg 4x09 b[5:0]
	Print("rx_h_pix_offset:%x\n", pstPTZRxParam->rx_h_pix_offset); //reg 4x0A b[7:0]
	Print("rx_line_len:%x\n", pstPTZRxParam->rx_line_len); //reg 4x0B b[5:0]
	Print("rx_valid_cnt:%x\n", pstPTZRxParam->rx_valid_cnt); //reg 4x0C b[7:0]
#endif // DEBUG_PTZ_PR1000

	return(ret);
}

#ifndef DONT_SUPPORT_PTZ_FUNC
/* PTZ Tx pattern format, pattern data. */
int PR1000_PTZ_WriteTxPattern(const int fd, const uint8_t i2cSlaveAddr, const uint8_t prChn, const unsigned char *pTxPatFormat, const int sizeTxPatFormat, const unsigned char *pTxPatData, const int sizeTxPatData)
{
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;

	int length;
	int srcInx, dstSize;
	uint8_t reg;

	/* write ptz tx pattern flag format */
	{/*{{{*/
		if( (length = sizeTxPatFormat) <= 0)
		{
			return(-1);
		}

#ifdef DEBUG_PTZ_PR1000
		Print("format strlength :%d.\n", length);
#ifdef __LINUX_SYSTEM__
		for(srcInx = 0; srcInx < length; srcInx++)
		{
			printk("0x%02x,", pTxPatFormat[srcInx]);
		}
#else
		for(srcInx = 0; srcInx < length; srcInx++)
		{
			printf("0x%02x,", pTxPatFormat[srcInx]);
		}
#endif
#endif // DEBUG_PTZ_PR1000

		/* init pattern format reg */
		i2cMask = 0xC7; i2cData = 0x81; //No bitswap
		i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		DbgString("pattern flag.\n");
		for(srcInx = 0, dstSize = 0; srcInx < length; srcInx++)
		{
			i2cData = pTxPatFormat[srcInx];
			/* single write */
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_DATA + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			Dbg("0x%02x ", i2cData);
			dstSize++;
		}
		/* verify */
		i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_ADDR + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
		{
			ErrorString("Read reg.\n");
			return(-1);
		}
		reg = i2cData;
		Dbg("wrAddr:%d\n", reg);
		if(reg != dstSize-1)
		{
			Error("Invalid [wr:0x%x, rd:0x%x]\n", dstSize-1 COMMA reg);
			return(-1);
		}
		Dbg("Write Pattern format %dByte.\n", dstSize);
	}/*}}}*/

	/* write ptz tx pattern flag data */
	{/*{{{*/
		if( (length = sizeTxPatData) <= 0)
		{
			return(-1);
		}

#ifdef DEBUG_PTZ_PR1000
		Print("data strlength :%d.\n", length);
#ifdef __LINUX_SYSTEM__
		for(srcInx = 0; srcInx < length; srcInx++)
		{
			printk("0x%02x,", pTxPatData[srcInx]);
		}
#else
		for(srcInx = 0; srcInx < length; srcInx++)
		{
			printf("0x%02x,", pTxPatData[srcInx]);
		}
#endif
#endif // DEBUG_PTZ_PR1000

		/* init pattern data reg */
		i2cMask = 0xC7; i2cData = 0x82; //No bitswap
		i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		DbgString("pattern data.\n");
		for(srcInx = 0, dstSize = 0; srcInx < length; srcInx++)
		{
			i2cData = pTxPatData[srcInx];
			/* single write */
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_DATA + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			Dbg("0x%02x ", i2cData);
			dstSize++;
		}
		/* verify */
		i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_ADDR + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
		{
			ErrorString("Read reg.\n");
			return(-1);
		}
		reg = i2cData;
		Dbg("wrAddr:%d\n", reg);
		if(reg != dstSize-1)
		{
			Error("Invalid [wr:0x%x, rd:0x%x]\n", dstSize-1 COMMA reg);
			return(-1);
		}
		Dbg("Write Pattern data %dByte.\n", dstSize);
	}/*}}}*/

	return(dstSize);
}
#endif // DONT_SUPPORT_PTZ_FUNC

/* PTZ Rx pattern format, start pattern format, start pattern data. */
int PR1000_PTZ_WriteRxPattern(const int fd, const uint8_t i2cSlaveAddr, const uint8_t prChn, const unsigned char *pRxPatFormat, const int sizeRxPatFormat, const unsigned char *pRxPatStartFormat, const int sizeRxPatStartFormat, const unsigned char *pRxPatStartData, const int sizeRxPatStartData)
{
	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;

	int length;
	int srcInx, dstSize;
	uint8_t reg;

	/* write ptz rx pattern flag format */
	{/*{{{*/
		if( (length = sizeRxPatFormat) <= 0)
		{
			return(-1);
		}

#ifdef DEBUG_PTZ_PR1000
		Print("format strlength :%d.\n", length);
#ifdef __LINUX_SYSTEM__
		for(srcInx = 0; srcInx < length; srcInx++)
		{
			printk("0x%02x,", pRxPatFormat[srcInx]);
		}
#else
		for(srcInx = 0; srcInx < length; srcInx++)
		{
			printf("0x%02x,", pRxPatFormat[srcInx]);
		}
#endif
#endif // DEBUG_PTZ_PR1000

		/* init pattern format reg */
		i2cMask = 0xC7; i2cData = 0x84; //No bitswap
		i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		DbgString("pattern flag.\n");
		for(srcInx = 0, dstSize = 0; srcInx < length; srcInx++)
		{
			i2cData = pRxPatFormat[srcInx];
			/* single write */
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_DATA + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			Dbg("0x%02x ", i2cData);
			dstSize++;
		}
		/* verify */
		i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_ADDR + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
		{
			ErrorString("Read reg.\n");
			return(-1);
		}
		reg = i2cData;
		Dbg("wrAddr:%d\n", reg);
		if(reg != dstSize-1)
		{
			Error("Invalid [wr:0x%x, rd:0x%x]\n", dstSize-1 COMMA reg);
			return(-1);
		}
		Dbg("Write Pattern format %dByte.\n", dstSize);
	}/*}}}*/

	/* write ptz rx pattern start flag */
	{/*{{{*/
		if( (length = sizeRxPatStartFormat) <= 0)
		{
			return(-1);
		}

#ifdef DEBUG_PTZ_PR1000
		Print("startFormat strlength :%d.\n", length);
#ifdef __LINUX_SYSTEM__
		for(srcInx = 0; srcInx < length; srcInx++)
		{
			printk("0x%02x,", pRxPatStartFormat[srcInx]);
		}
#else
		for(srcInx = 0; srcInx < length; srcInx++)
		{
			printf("0x%02x,", pRxPatStartFormat[srcInx]);
		}
#endif
#endif // DEBUG_PTZ_PR1000

		/* init pattern data reg */
		i2cMask = 0xC7; i2cData = 0x85;//No bitswap
		i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		DbgString("pattern data.\n");
		for(srcInx = 0, dstSize = 0; srcInx < length; srcInx++)
		{
			i2cData = pRxPatStartFormat[srcInx];
			/* single write */
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_DATA + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			Dbg("0x%02x ", i2cData);
			dstSize++;
		}
		/* verify */
		i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_ADDR + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
		{
			ErrorString("Read reg.\n");
			return(-1);
		}
		reg = i2cData;
		Dbg("wrAddr:%d\n", reg);
		if(reg != dstSize-1)
		{
			Error("Invalid [wr:0x%x, rd:0x%x]\n", dstSize-1 COMMA reg);
			return(-1);
		}
		Dbg("Write Pattern start format %dByte.\n", dstSize);
	}/*}}}*/

	/* write ptz rx pattern start data */
	{/*{{{*/
		if( (length = sizeRxPatStartData) <= 0)
		{
			return(-1);
		}

#ifdef DEBUG_PTZ_PR1000
		Print("startData strlength :%d.\n", length);
#ifdef __LINUX_SYSTEM__
		for(srcInx = 0; srcInx < length; srcInx++)
		{
			printk("0x%02x,", pRxPatStartData[srcInx]);
		}
#else
		for(srcInx = 0; srcInx < length; srcInx++)
		{
			printf("0x%02x,", pRxPatStartData[srcInx]);
		}
#endif
#endif // DEBUG_PTZ_PR1000

		/* init pattern data reg */
		i2cMask = 0xC7; i2cData = 0x86; //No bitswap
		i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		DbgString("pattern data.\n");
		for(srcInx = 0, dstSize = 0; srcInx < length; srcInx++)
		{
			i2cData = pRxPatStartData[srcInx];
			/* single write */
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_DATA + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
			Dbg("0x%02x ", i2cData);
			dstSize++;
		}
		/* verify */
		i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_ADDR + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
		{
			ErrorString("Read reg.\n");
			return(-1);
		}
		reg = i2cData;
		Dbg("wrAddr:%d\n", reg);
		if(reg != dstSize-1)
		{
			Error("Invalid [wr:0x%x, rd:0x%x]\n", dstSize-1 COMMA reg);
			return(-1);
		}
		Dbg("Write Pattern start data %dByte.\n", dstSize);
	}/*}}}*/

	return(dstSize);
}

#ifndef DONT_SUPPORT_PTZ_FUNC
//////////////////////////////////// SD720/SD960 /////////////////////////////////////////////
int PR1000_PTZ_SD_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength)
{
	int ret = 0;

	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;

	int bWaitCnt;
	uint8_t ptzCmdTxGroupCnt;

	uint16_t ptzSendLength;
	uint16_t ptzSendInx;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if( (pPtzCmd == NULL) || (ptzCmdLength == 0) )
	{
		ErrorString("Can't support ptz command.\n");
		return(-1);
	}

	ptzSendLength = PR1000_PTZ_SD_TXCMD_BASE_BYTE_CNT;
	ptzCmdTxGroupCnt = (ptzCmdLength/PR1000_PTZ_SD_TXCMD_BASE_BYTE_CNT);
	if(ptzCmdLength%PR1000_PTZ_SD_TXCMD_BASE_BYTE_CNT)
	{
		ErrorString("Invalid ptz command length.\n");
		return(-1);
	}
	Dbg("Set tx group cnt:0x%x\n", ptzCmdTxGroupCnt);

#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
	{
		int i;
		printk("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printk("0x%02X ", pPtzCmd[i]);
		}
		printk("\n");
	}
#else
	{
		int i;
		printf("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printf("0x%02X ", pPtzCmd[i]);
		}
		printf("\n\r");
	}
#endif
#endif

	ptzSendInx = 0;
	while(ptzSendInx < ptzCmdLength)
	{
		Dbg("sendinx:%d/%d\n", ptzSendInx COMMA ptzCmdLength);

		/* wait send tx status. */
		bWaitCnt = 10;
		while( (PR1000_PTZ_CheckSendTxStatus(fd, pstPortChSel) < 0) && bWaitCnt-- )
		{
			DbgString("Wait send tx status.\n");
#ifdef __LINUX_SYSTEM__
			mdelay(100);
#else //#ifdef __LINUX_SYSTEM__
			MDelay(100);
#endif // __LINUX_SYSTEM__
		}
		if(bWaitCnt <= 0)
		{
			ErrorString("TimeOver send tx status.\n");
			ret = -1;
			break;
		}
		else
		{
			uint8_t txFifoSize = 0;

			/* set group count */
			i2cData = 0x81; //0x80:enable
			i2cReg = PR1000_REG_ADDR_PTZ_TX_GROUP_CNT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			/* init data reg */
			i2cMask = 0xC7; i2cData = 0x00|((PR1000_PTZ_SD_TX_DATA_BITSWAP&0x1)<<6);
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_DATA + PR1000_OFFSETADDR_PTZ_CH(prChn);
#if 0 /* Single write Tx queue data */
			for(i = 0; i < ptzSendLength; i++)
			{
				i2cData = pPtzCmd[i+ptzSendInx];
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}
#else /* addr hold en burst write, speed up */
			if( PR1000_PageWriteBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, ptzCmdLength, &pPtzCmd[ptzSendInx]) < 0)
			{
				ErrorString("PageWriteBurst.\n");
				return(-1);
			}
#endif

			/* verify */
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_SIZE + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
			{
				ErrorString("Read reg.\n");
				return(-1);
			}
			txFifoSize = i2cData;
			if(txFifoSize != ptzSendLength)
			{
				Error("Invalid [wr:0x%x, rd:0x%x]\n", ptzSendLength COMMA txFifoSize);
				return(-1);
			}
			Dbg("Write Tx data to Fifo %dByte.\n", ptzSendLength);

			/* start tx */
			DbgString("Start ptz tx.\n");
			PR1000_PTZ_StartTX(fd, pstPortChSel, START);

			ptzSendInx += ptzSendLength;
			ret = ptzCmdLength;
		}
	}

	return(ret);
}

#ifndef DONT_SUPPORT_STD_PVI
//////////////////////////////////// PVI /////////////////////////////////////////////
int PR1000_PTZ_PVI_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength)
{
	int ret = 0;

	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;

	int bWaitCnt;
	uint8_t ptzCmdTxGroupCnt;
	uint8_t camResol = resol;

	uint16_t ptzSendLength;
	uint16_t ptzSendInx;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if( (pPtzCmd == NULL) || (ptzCmdLength == 0) )
	{
		ErrorString("Can't support ptz command.\n");
		return(-1);
	}

	/* convert cif,shrink resolution to camera resolution. */
	switch(resol)
	{
		case pr1000_outresol_1280x720p60c: camResol = pr1000_outresol_1280x720p60; break;
		case pr1000_outresol_1280x720p50c: camResol = pr1000_outresol_1280x720p50; break;
		case pr1000_outresol_1280x720p30c: camResol =  pr1000_outresol_1280x720p30; break;
		case pr1000_outresol_1280x720p25c: camResol =  pr1000_outresol_1280x720p25; break;
		case pr1000_outresol_1920x1080p30c: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25c: camResol =  pr1000_outresol_1920x1080p25; break;
		case pr1000_outresol_1920x1080p30s: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25s: camResol =  pr1000_outresol_1920x1080p25; break;
		default: break;
	}

	ptzSendLength = PR1000_PTZ_PVI_TXCMD_BASE_BYTE_CNT;
	ptzCmdTxGroupCnt = (ptzCmdLength/PR1000_PTZ_PVI_TXCMD_BASE_BYTE_CNT);
	if(ptzCmdLength%PR1000_PTZ_PVI_TXCMD_BASE_BYTE_CNT)
	{
		ErrorString("Invalid ptz command length.\n");
		return(-1);
	}
	Dbg("Set tx group cnt:0x%x\n", ptzCmdTxGroupCnt);

#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
	{
		int i;
		printk("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printk("0x%02X ", pPtzCmd[i]);
		}
		printk("\n");
	}
#else
	{
		int i;
		printf("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printf("0x%02X ", pPtzCmd[i]);
		}
		printf("\n\r");
	}
#endif
#endif

	ptzSendInx = 0;
	while(ptzSendInx < ptzCmdLength)
	{
		Dbg("sendinx:%d/%d\n", ptzSendInx COMMA ptzCmdLength);

		/* wait send tx status. */
		bWaitCnt = 10;
		while( (PR1000_PTZ_CheckSendTxStatus(fd, pstPortChSel) < 0) && bWaitCnt-- )
		{
			DbgString("Wait send tx status.\n");
#ifdef __LINUX_SYSTEM__
			mdelay(100);
#else //#ifdef __LINUX_SYSTEM__
			MDelay(100);
#endif // __LINUX_SYSTEM__
		}
		if(bWaitCnt <= 0)
		{
			ErrorString("TimeOver send tx status.\n");
			ret = -1;
			break;
		}
		else
		{
			uint8_t txFifoSize = 0;

			/* set group count */
			i2cData = 0x81; //0x80:enable
			i2cReg = PR1000_REG_ADDR_PTZ_TX_GROUP_CNT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			/* init data reg */
			i2cMask = 0xC7; i2cData = 0x00|((PR1000_PTZ_PVI_TX_DATA_BITSWAP&0x1)<<6);
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_DATA + PR1000_OFFSETADDR_PTZ_CH(prChn);
#if 0 /* Single write Tx queue data */
			for(i = 0; i < ptzSendLength; i++)
			{
				i2cData = pPtzCmd[i+ptzSendInx];
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}
#else /* addr hold en burst write, speed up */
			if( PR1000_PageWriteBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, ptzCmdLength, &pPtzCmd[ptzSendInx]) < 0)
			{
				ErrorString("PageWriteBurst.\n");
				return(-1);
			}
#endif
			/* verify */
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_SIZE + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
			{
				ErrorString("Read reg.\n");
				return(-1);
			}
			txFifoSize = i2cData;
			if(txFifoSize != ptzSendLength)
			{
				Error("Invalid [wr:0x%x, rd:0x%x]\n", ptzSendLength COMMA txFifoSize);
				return(-1);
			}
			Dbg("Write Tx data to Fifo %dByte.\n", ptzSendLength);

			/* start tx */
			DbgString("Start ptz tx.\n");
			PR1000_PTZ_StartTX(fd, pstPortChSel, START);

			ptzSendInx += ptzSendLength;
			ret = ptzCmdLength;
		}
	}

	return(ret);
}
#endif // DONT_SUPPORT_STD_PVI

//////////////////////////////////// HDA /////////////////////////////////////////////
int PR1000_PTZ_HDA_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength)
{
	int ret = 0;

	uint8_t i2cMask = 0;
	uint8_t i2cReg, i2cData;

	int bWaitCnt;
	uint8_t ptzCmdTxGroupCnt;
	uint8_t camResol = resol;

	uint16_t ptzSendLength = 0;
	uint16_t ptzSendInx;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if( (pPtzCmd == NULL) || (ptzCmdLength == 0) )
	{
		ErrorString("Can't support ptz command.\n");
		return(-1);
	}

	/* convert cif,shrink resolution to camera resolution. */
	switch(resol)
	{
		case pr1000_outresol_1280x720p60c: camResol = pr1000_outresol_1280x720p60; break;
		case pr1000_outresol_1280x720p50c: camResol = pr1000_outresol_1280x720p50; break;
		case pr1000_outresol_1280x720p30c: camResol =  pr1000_outresol_1280x720p30; break;
		case pr1000_outresol_1280x720p25c: camResol =  pr1000_outresol_1280x720p25; break;
		case pr1000_outresol_1920x1080p30c: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25c: camResol =  pr1000_outresol_1920x1080p25; break;
		case pr1000_outresol_1920x1080p30s: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25s: camResol =  pr1000_outresol_1920x1080p25; break;
		default: break;
	}

	if( (camResol == pr1000_outresol_1920x1080p30) || (camResol == pr1000_outresol_1920x1080p25) ) /* 1080p30, 1080p25 */
	{
		ptzSendLength = PR1000_PTZ_HDA_TXCMD_BASE_BYTE_CNT_1080p;
		ptzCmdTxGroupCnt = (ptzCmdLength/PR1000_PTZ_HDA_TXCMD_BASE_BYTE_CNT_1080p);
		if(ptzCmdLength%PR1000_PTZ_HDA_TXCMD_BASE_BYTE_CNT_1080p)
		{
			ErrorString("Invalid ptz command length.\n");
			return(-1);
		}
		Dbg("Set tx group cnt:0x%x\n", ptzCmdTxGroupCnt);
	}
	else if( (camResol == pr1000_outresol_1280x720p60) || (camResol == pr1000_outresol_1280x720p50) || 
		(camResol == pr1000_outresol_1280x720p30) || (camResol == pr1000_outresol_1280x720p25) )
	{
		ptzSendLength = PR1000_PTZ_HDA_TXCMD_BASE_BYTE_CNT_720p;
		ptzCmdTxGroupCnt = (ptzCmdLength/PR1000_PTZ_HDA_TXCMD_BASE_BYTE_CNT_720p);
		if(ptzCmdLength%PR1000_PTZ_HDA_TXCMD_BASE_BYTE_CNT_720p)
		{
			ErrorString("Invalid ptz command length.\n");
			return(-1);
		}
		Dbg("Set tx group cnt:0x%x\n", ptzCmdTxGroupCnt);
	}

#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
	{
		int i;
		printk("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printk("0x%02X ", pPtzCmd[i]);
		}
		printk("\n");
	}
#else
	{
		int i;
		printf("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printf("0x%02X ", pPtzCmd[i]);
		}
		printf("\n\r");
	}
#endif
#endif

	ptzSendInx = 0;
	while(ptzSendInx < ptzCmdLength)
	{
		Dbg("sendinx:%d/%d\n", ptzSendInx COMMA ptzCmdLength);

		/* wait send tx status. */
		bWaitCnt = 10;
		while( (PR1000_PTZ_CheckSendTxStatus(fd, pstPortChSel) < 0) && bWaitCnt-- )
		{
			DbgString("Wait send tx status.\n");
#ifdef __LINUX_SYSTEM__
			mdelay(100);
#else //#ifdef __LINUX_SYSTEM__
			MDelay(100);
#endif // __LINUX_SYSTEM__
		}
		if(bWaitCnt <= 0)
		{
			ErrorString("TimeOver send tx status.\n");
			ret = -1;
			break;
		}
		else
		{
			uint8_t txFifoSize = 0;

			/* set group count */
			i2cData = 0x81; //0x80:enable
			i2cReg = PR1000_REG_ADDR_PTZ_TX_GROUP_CNT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			/* init data reg */
			if( (camResol == pr1000_outresol_1920x1080p30) || (camResol == pr1000_outresol_1920x1080p25) ) /* 1080p30, 1080p25 */
			{
				i2cMask = 0xC7; i2cData = 0x00|((PR1000_PTZ_HDA_TX_DATA_BITSWAP_1080P&0x1)<<6);
			}
			else if( (camResol == pr1000_outresol_1280x720p60) || (camResol == pr1000_outresol_1280x720p50) || 
					(camResol == pr1000_outresol_1280x720p30) || (camResol == pr1000_outresol_1280x720p25) )
			{
				i2cMask = 0xC7; i2cData = 0x00|((PR1000_PTZ_HDA_TX_DATA_BITSWAP_720P&0x1)<<6);
			}
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_DATA + PR1000_OFFSETADDR_PTZ_CH(prChn);
#if 0 /* Single write Tx queue data */
			for(i = 0; i < ptzSendLength; i++)
			{
				i2cData = pPtzCmd[i+ptzSendInx];
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}
#else /* addr hold en burst write, speed up */
			if( PR1000_PageWriteBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, ptzCmdLength, &pPtzCmd[ptzSendInx]) < 0)
			{
				ErrorString("PageWriteBurst.\n");
				return(-1);
			}
#endif

			/* verify */
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_SIZE + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
			{
				ErrorString("Read reg.\n");
				return(-1);
			}
			txFifoSize = i2cData;
			if(txFifoSize != ptzSendLength)
			{
				Error("Invalid [wr:0x%x, rd:0x%x]\n", ptzSendLength COMMA txFifoSize);
				return(-1);
			}
			Dbg("Write Tx data to Fifo %dByte.\n", ptzSendLength);

			/* start tx */
			DbgString("Start ptz tx.\n");
			PR1000_PTZ_StartTX(fd, pstPortChSel, START);

			ptzSendInx += ptzSendLength;
			ret = ptzCmdLength;
		}
	}

	return(ret);
}

int PR1000_PTZ_HDA_STDFORMAT_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength)
{
	int ret = 0;

	uint8_t i2cMask = 0;
	uint8_t i2cReg, i2cData;

	int bWaitCnt;
	uint8_t ptzCmdTxGroupCnt;
	uint8_t camResol = resol;

	uint16_t ptzSendLength = 0;
	uint16_t ptzSendInx;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if( (pPtzCmd == NULL) || (ptzCmdLength == 0) )
	{
		ErrorString("Can't support ptz command.\n");
		return(-1);
	}

	/* convert cif,shrink resolution to camera resolution. */
	switch(resol)
	{
		case pr1000_outresol_1280x720p60c: camResol = pr1000_outresol_1280x720p60; break;
		case pr1000_outresol_1280x720p50c: camResol = pr1000_outresol_1280x720p50; break;
		case pr1000_outresol_1280x720p30c: camResol =  pr1000_outresol_1280x720p30; break;
		case pr1000_outresol_1280x720p25c: camResol =  pr1000_outresol_1280x720p25; break;
		case pr1000_outresol_1920x1080p30c: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25c: camResol =  pr1000_outresol_1920x1080p25; break;
		case pr1000_outresol_1920x1080p30s: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25s: camResol =  pr1000_outresol_1920x1080p25; break;
		default: break;
	}

	if( (camResol == pr1000_outresol_1920x1080p30) || (camResol == pr1000_outresol_1920x1080p25) ) /* 1080p30, 1080p25 */
	{
		ptzSendLength = PR1000_PTZ_HDA_TXCMD_BASE_BYTE_CNT_1080p;
		ptzCmdTxGroupCnt = (ptzCmdLength/PR1000_PTZ_HDA_TXCMD_BASE_BYTE_CNT_1080p);
		if(ptzCmdLength%PR1000_PTZ_HDA_TXCMD_BASE_BYTE_CNT_1080p)
		{
			ErrorString("Invalid ptz command length.\n");
			return(-1);
		}
		Dbg("Set tx group cnt:0x%x\n", ptzCmdTxGroupCnt);
	}
	else if( (camResol == pr1000_outresol_1280x720p60) || (camResol == pr1000_outresol_1280x720p50) || 
		(camResol == pr1000_outresol_1280x720p30) || (camResol == pr1000_outresol_1280x720p25) )
	{
		ptzSendLength = PR1000_PTZ_HDA_TXCMD_BASE_BYTE_CNT_720p;
		ptzCmdTxGroupCnt = (ptzCmdLength/PR1000_PTZ_HDA_TXCMD_BASE_BYTE_CNT_720p);
		if(ptzCmdLength%PR1000_PTZ_HDA_TXCMD_BASE_BYTE_CNT_720p)
		{
			ErrorString("Invalid ptz command length.\n");
			return(-1);
		}
		Dbg("Set tx group cnt:0x%x\n", ptzCmdTxGroupCnt);
	}

#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
	{
		int i;
		printk("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printk("0x%02X ", pPtzCmd[i]);
		}
		printk("\n");
	}
#else
	{
		int i;
		printf("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printf("0x%02X ", pPtzCmd[i]);
		}
		printf("\n\r");
	}
#endif
#endif

	ptzSendInx = 0;
	while(ptzSendInx < ptzCmdLength)
	{
		Dbg("sendinx:%d/%d\n", ptzSendInx COMMA ptzCmdLength);

		/* wait send tx status. */
		bWaitCnt = 10;
		while( (PR1000_PTZ_CheckSendTxStatus(fd, pstPortChSel) < 0) && bWaitCnt-- )
		{
			DbgString("Wait send tx status.\n");
#ifdef __LINUX_SYSTEM__
			mdelay(100);
#else //#ifdef __LINUX_SYSTEM__
			MDelay(100);
#endif // __LINUX_SYSTEM__
		}
		if(bWaitCnt <= 0)
		{
			ErrorString("TimeOver send tx status.\n");
			ret = -1;
			break;
		}
		else
		{
			uint8_t txFifoSize = 0;

			/* set group count */
			i2cData = 0x81; //0x80:enable
			i2cReg = PR1000_REG_ADDR_PTZ_TX_GROUP_CNT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			/* init data reg */
			if( (camResol == pr1000_outresol_1920x1080p30) || (camResol == pr1000_outresol_1920x1080p25) ) /* 1080p30, 1080p25 */
			{
				i2cMask = 0xC7; i2cData = 0x00|((PR1000_PTZ_HDA_TX_DATA_BITSWAP_1080P&0x1)<<6);
			}
			else if( (camResol == pr1000_outresol_1280x720p60) || (camResol == pr1000_outresol_1280x720p50) || 
					(camResol == pr1000_outresol_1280x720p30) || (camResol == pr1000_outresol_1280x720p25) )
			{
				i2cMask = 0xC7; i2cData = 0x00|((PR1000_PTZ_HDA_TX_DATA_BITSWAP_720P&0x1)<<6);
			}
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_DATA + PR1000_OFFSETADDR_PTZ_CH(prChn);
#if 0 /* Single write Tx queue data */
			for(i = 0; i < ptzSendLength; i++)
			{
				i2cData = pPtzCmd[i+ptzSendInx];
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}
#else /* addr hold en burst write, speed up */
			if( PR1000_PageWriteBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, ptzCmdLength, &pPtzCmd[ptzSendInx]) < 0)
			{
				ErrorString("PageWriteBurst.\n");
				return(-1);
			}
#endif

			/* verify */
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_SIZE + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
			{
				ErrorString("Read reg.\n");
				return(-1);
			}
			txFifoSize = i2cData;
			if(txFifoSize != ptzSendLength)
			{
				Error("Invalid [wr:0x%x, rd:0x%x]\n", ptzSendLength COMMA txFifoSize);
				return(-1);
			}
			Dbg("Write Tx data to Fifo %dByte.\n", ptzSendLength);

			/* start tx */
			DbgString("Start ptz tx.\n");
			PR1000_PTZ_StartTX(fd, pstPortChSel, START);

			ptzSendInx += ptzSendLength;
			ret = ptzCmdLength;
		}
	}

	return(ret);
}



//////////////////////////////////// CVI /////////////////////////////////////////////
int PR1000_PTZ_CVI_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength)
{
	int ret = 0;

	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;

	int i = 0;

	int bWaitCnt;
	uint8_t ptzCmdTxGroupCnt;
	uint8_t camResol = resol;

	uint16_t ptzSendLength;
	uint16_t ptzSendInx;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 
	uint8_t txLineCnt;

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if( (pPtzCmd == NULL) || (ptzCmdLength == 0) )
	{
		ErrorString("Can't support ptz command.\n");
		return(-1);
	}

	/* convert cif,shrink resolution to camera resolution. */
	switch(resol)
	{
		case pr1000_outresol_1280x720p60c: camResol = pr1000_outresol_1280x720p60; break;
		case pr1000_outresol_1280x720p50c: camResol = pr1000_outresol_1280x720p50; break;
		case pr1000_outresol_1280x720p30c: camResol =  pr1000_outresol_1280x720p30; break;
		case pr1000_outresol_1280x720p25c: camResol =  pr1000_outresol_1280x720p25; break;
		case pr1000_outresol_1920x1080p30c: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25c: camResol =  pr1000_outresol_1920x1080p25; break;
		case pr1000_outresol_1920x1080p30s: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25s: camResol =  pr1000_outresol_1920x1080p25; break;
		default: break;
	}

	if( (camResol >= pr1000_outresol_1280x720p60) && (camResol <= pr1000_outresol_1920x1080p25) )
	{
		txLineCnt = ((const _stPTZTxParam *)&pr1000_ptz_txparam_cvi_def[camResol-pr1000_outresol_1280x720p60])->tx_line_cnt;
	}
	else
	{
		Error("Invalid resol. mapChn:%d\n", mapChn);
		return(-1);
	}


	ptzSendLength = PR1000_PTZ_CVI_TXCMD_BASE_BYTE_CNT;
	ptzCmdTxGroupCnt = (ptzCmdLength/PR1000_PTZ_CVI_TXCMD_BASE_BYTE_CNT);
	if(ptzCmdLength%PR1000_PTZ_CVI_TXCMD_BASE_BYTE_CNT)
	{
		ErrorString("Invalid ptz command length.\n");
		return(-1);
	}
	Dbg("Set tx group cnt:0x%x\n", ptzCmdTxGroupCnt);

#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
	{
		int i;
		printk("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printk("0x%02X ", pPtzCmd[i]);
		}
		printk("\n");
	}
#else
	{
		int i;
		printf("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printf("0x%02X ", pPtzCmd[i]);
		}
		printf("\n\r");
	}
#endif
#endif

	ptzSendInx = 0;
	while(ptzSendInx < ptzCmdLength)
	{
		Dbg("sendinx:%d/%d\n", ptzSendInx COMMA ptzCmdLength);

		/* wait send tx status. */
		bWaitCnt = 10;
		while( (PR1000_PTZ_CheckSendTxStatus(fd, pstPortChSel) < 0) && bWaitCnt-- )
		{
			DbgString("Wait send tx status.\n");
#ifdef __LINUX_SYSTEM__
			mdelay(100);
#else //#ifdef __LINUX_SYSTEM__
			MDelay(100);
#endif // __LINUX_SYSTEM__
		}
		if(bWaitCnt <= 0)
		{
			ErrorString("TimeOver send tx status.\n");
			ret = -1;
			break;
		}
		else
		{
			uint8_t txFifoSize = 0;
			uint8_t parityInx = 0;
			uint32_t cmdParity = 0;
			uint8_t bitCnt = 0;
			uint8_t evenParity = 0;

			/* set group count */
			i2cData = 0x81; //0x80:enable
			i2cReg = PR1000_REG_ADDR_PTZ_TX_GROUP_CNT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			/* init data reg */
			i2cMask = 0xC7; i2cData = 0x00|((PR1000_PTZ_CVI_TX_DATA_BITSWAP&0x1)<<6);
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_DATA + PR1000_OFFSETADDR_PTZ_CH(prChn);
#if 0 /* Single write Tx queue data */
			for(i = 0; i < ptzSendLength; i++)
			{
				/* calculate parity(6byte) */
				if( parityInx == txLineCnt )
				{
					cmdParity %= 256;

					i2cData = cmdParity;
					if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}

					/* append parity data */
					for(evenParity = 0, bitCnt = 0; bitCnt < 8; bitCnt++)
					{
						if(i2cData & (1<<bitCnt)) evenParity++;
					}
					i2cData = ((evenParity&0x1))|0xFE;
					if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}

					cmdParity = 0;
					parityInx = 0;
				}
				else
				{
					cmdParity += pPtzCmd[i+ptzSendInx];

					i2cData = pPtzCmd[i+ptzSendInx];
					if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}

					/* append parity data */
					for(evenParity = 0, bitCnt = 0; bitCnt < 8; bitCnt++)
					{
						if(i2cData & (1<<bitCnt)) evenParity++;
					}
					i2cData = ((evenParity&0x1))|0xFE;
					if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
					{
						ErrorString("Write reg.\n");
						return(-1);
					}

					parityInx++;
				}
			}
#else /* addr hold en burst write, speed up */
			{
				uint8_t inxTxBuf = 0;
#ifdef __LINUX_SYSTEM__
				uint8_t *pTxBuf = NULL;
				if( (pTxBuf = (uint8_t *)ZALLOC(sizeof(uint8_t)*ptzSendLength*2, GFP_KERNEL)) == NULL)
				{
					ErrorString("malloc failed.\n");
					return(-1);
				}
#else //#ifdef __LINUX_SYSTEM__
				uint8_t pTxBuf[128];
				if(ptzSendLength*2 >= 128) 
				{
					ErrorString("malloc failed.\n");
					return(-1);
				}
#endif // __LINUX_SYSTEM__

				for(i = 0; i < ptzSendLength; i++)
				{
					/* calculate parity(6byte) */
					if( parityInx == txLineCnt )
					{
						cmdParity %= 256;

						i2cData = cmdParity;
						pTxBuf[inxTxBuf++] = i2cData;

						/* append parity data */
						for(evenParity = 0, bitCnt = 0; bitCnt < 8; bitCnt++)
						{
							if(i2cData & (1<<bitCnt)) evenParity++;
						}
						i2cData = ((evenParity&0x1))|0xFE;
						pTxBuf[inxTxBuf++] = i2cData;

						cmdParity = 0;
						parityInx = 0;
					}
					else
					{
						cmdParity += pPtzCmd[i+ptzSendInx];

						i2cData = pPtzCmd[i+ptzSendInx];
						pTxBuf[inxTxBuf++] = i2cData;

						/* append parity data */
						for(evenParity = 0, bitCnt = 0; bitCnt < 8; bitCnt++)
						{
							if(i2cData & (1<<bitCnt)) evenParity++;
						}
						i2cData = ((evenParity&0x1))|0xFE;
						pTxBuf[inxTxBuf++] = i2cData;

						parityInx++;
					}
				}
				if( PR1000_PageWriteBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, inxTxBuf, pTxBuf) < 0)
				{
					ErrorString("PageWriteBurst.\n");
					return(-1);
				}
#ifdef __LINUX_SYSTEM__
				FREE(pTxBuf);
#endif // __LINUX_SYSTEM__
			}
#endif
			/* verify */
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_SIZE + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
			{
				ErrorString("Read reg.\n");
				return(-1);
			}
			txFifoSize = i2cData/2;
			if(txFifoSize != ptzSendLength)
			{
				Error("Invalid [wr:0x%x, rd:0x%x]\n", ptzSendLength COMMA txFifoSize);
				return(-1);
			}
			Dbg("Write Tx data to Fifo %dByte.\n", ptzSendLength);

			/* start tx */
			DbgString("Start ptz tx.\n");
			PR1000_PTZ_StartTX(fd, pstPortChSel, START);

			ptzSendInx += ptzSendLength;
			ret = ptzCmdLength;
		}
	}


	return(ret);
}

//////////////////////////////////// HDT /////////////////////////////////////////////
int PR1000_PTZ_HDT_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength)
{
	int ret = 0;

	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;


	int bWaitCnt;
	uint8_t ptzCmdTxGroupCnt;
	uint8_t camResol = resol;

	uint16_t ptzSendLength;
	uint16_t ptzSendInx;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if( (pPtzCmd == NULL) || (ptzCmdLength == 0) )
	{
		ErrorString("Can't support ptz command.\n");
		return(-1);
	}

	/* convert cif,shrink resolution to camera resolution. */
	switch(resol)
	{
		case pr1000_outresol_1280x720p60c: camResol = pr1000_outresol_1280x720p60; break;
		case pr1000_outresol_1280x720p50c: camResol = pr1000_outresol_1280x720p50; break;
		case pr1000_outresol_1280x720p30c: camResol =  pr1000_outresol_1280x720p30; break;
		case pr1000_outresol_1280x720p25c: camResol =  pr1000_outresol_1280x720p25; break;
		case pr1000_outresol_1920x1080p30c: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25c: camResol =  pr1000_outresol_1920x1080p25; break;
		case pr1000_outresol_1920x1080p30s: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25s: camResol =  pr1000_outresol_1920x1080p25; break;
		default: break;
	}

	ptzSendLength = PR1000_PTZ_HDT_TXCMD_BASE_BYTE_CNT;
	ptzCmdTxGroupCnt = (ptzCmdLength/PR1000_PTZ_HDT_TXCMD_BASE_BYTE_CNT);
	if(ptzCmdLength%PR1000_PTZ_HDT_TXCMD_BASE_BYTE_CNT)
	{
		ErrorString("Invalid ptz command length.\n");
		return(-1);
	}
	Dbg("Set tx group cnt:0x%x\n", ptzCmdTxGroupCnt);

#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
	{
		int i;
		printk("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printk("0x%02X ", pPtzCmd[i]);
		}
		printk("\n");
	}
#else
	{
		int i;
		printf("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printf("0x%02X ", pPtzCmd[i]);
		}
		printf("\n\r");
	}
#endif
#endif

	ptzSendInx = 0;
	while(ptzSendInx < ptzCmdLength)
	{
		Dbg("sendinx:%d/%d\n", ptzSendInx COMMA ptzCmdLength);

		/* wait send tx status. */
		bWaitCnt = 10;
		while( (PR1000_PTZ_CheckSendTxStatus(fd, pstPortChSel) < 0) && bWaitCnt-- )
		{
			DbgString("Wait send tx status.\n");
#ifdef __LINUX_SYSTEM__
			mdelay(100);
#else //#ifdef __LINUX_SYSTEM__
			MDelay(100);
#endif // __LINUX_SYSTEM__
		}
		if(bWaitCnt <= 0)
		{
			ErrorString("TimeOver send tx status.\n");
			ret = -1;
			break;
		}
		else
		{
			uint8_t txFifoSize = 0;

			/* set group count */
			i2cData = 0x81; //0x80:enable
			i2cReg = PR1000_REG_ADDR_PTZ_TX_GROUP_CNT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			/* init data reg */
			i2cMask = 0xC7; i2cData = 0x00|((PR1000_PTZ_HDT_TX_DATA_BITSWAP&0x1)<<6);
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_DATA + PR1000_OFFSETADDR_PTZ_CH(prChn);
#if 0 /* Single write Tx queue data */
			for(i = 0; i < ptzSendLength; i++)
			{
				i2cData = pPtzCmd[i+ptzSendInx];
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}
#else /* addr hold en burst write, speed up */
			if( PR1000_PageWriteBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, ptzCmdLength, &pPtzCmd[ptzSendInx]) < 0)
			{
				ErrorString("PageWriteBurst.\n");
				return(-1);
			}
#endif
			/* verify */
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_SIZE + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
			{
				ErrorString("Read reg.\n");
				return(-1);
			}
			txFifoSize = i2cData;
			if(txFifoSize != ptzSendLength)
			{
				Error("Invalid [wr:0x%x, rd:0x%x]\n", ptzSendLength COMMA txFifoSize);
				return(-1);
			}
			Dbg("Write Tx data to Fifo %dByte.\n", ptzSendLength);

			/* start tx */
			DbgString("Start ptz tx.\n");
			PR1000_PTZ_StartTX(fd, pstPortChSel, START);

			ptzSendInx += ptzSendLength;
			ret = ptzCmdLength;
		}
	}

	return(ret);
}

//////////////////////////////////// HDT_NEW /////////////////////////////////////////////
int PR1000_PTZ_HDT_NEW_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength)
{
	int ret = 0;

	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;


	int bWaitCnt;
	uint8_t ptzCmdTxGroupCnt;
	uint8_t camResol = resol;

	uint16_t ptzSendLength;
	uint16_t ptzSendInx;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if( (pPtzCmd == NULL) || (ptzCmdLength == 0) )
	{
		ErrorString("Can't support ptz command.\n");
		return(-1);
	}

	/* convert cif,shrink resolution to camera resolution. */
	switch(resol)
	{
		case pr1000_outresol_1280x720p60c: camResol = pr1000_outresol_1280x720p60; break;
		case pr1000_outresol_1280x720p50c: camResol = pr1000_outresol_1280x720p50; break;
		case pr1000_outresol_1280x720p30c: camResol =  pr1000_outresol_1280x720p30; break;
		case pr1000_outresol_1280x720p25c: camResol =  pr1000_outresol_1280x720p25; break;
		case pr1000_outresol_1920x1080p30c: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25c: camResol =  pr1000_outresol_1920x1080p25; break;
		case pr1000_outresol_1920x1080p30s: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25s: camResol =  pr1000_outresol_1920x1080p25; break;
		default: break;
	}

	ptzSendLength = PR1000_PTZ_HDT_NEW_TXCMD_BASE_BYTE_CNT;
	ptzCmdTxGroupCnt = (ptzCmdLength/PR1000_PTZ_HDT_NEW_TXCMD_BASE_BYTE_CNT);
	if(ptzCmdLength%PR1000_PTZ_HDT_NEW_TXCMD_BASE_BYTE_CNT)
	{
		ErrorString("Invalid ptz command length.\n");
		return(-1);
	}
	Dbg("Set tx group cnt:0x%x\n", ptzCmdTxGroupCnt);

#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
	{
		int i;
		printk("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printk("0x%02X ", pPtzCmd[i]);
		}
		printk("\n");
	}
#else
	{
		int i;
		printf("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printf("0x%02X ", pPtzCmd[i]);
		}
		printf("\n\r");
	}
#endif
#endif

	ptzSendInx = 0;
	while(ptzSendInx < ptzCmdLength)
	{
		Dbg("sendinx:%d/%d\n", ptzSendInx COMMA ptzCmdLength);

		/* wait send tx status. */
		bWaitCnt = 10;
		while( (PR1000_PTZ_CheckSendTxStatus(fd, pstPortChSel) < 0) && bWaitCnt-- )
		{
			DbgString("Wait send tx status.\n");
#ifdef __LINUX_SYSTEM__
			mdelay(100);
#else //#ifdef __LINUX_SYSTEM__
			MDelay(100);
#endif // __LINUX_SYSTEM__
		}
		if(bWaitCnt <= 0)
		{
			ErrorString("TimeOver send tx status.\n");
			ret = -1;
			break;
		}
		else
		{
			uint8_t txFifoSize = 0;

			/* set group count */
			i2cData = 0x81; //0x80:enable
			i2cReg = PR1000_REG_ADDR_PTZ_TX_GROUP_CNT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			/* init data reg */
			i2cMask = 0xC7; i2cData = 0x00|((PR1000_PTZ_HDT_NEW_TX_DATA_BITSWAP&0x1)<<6);
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_DATA + PR1000_OFFSETADDR_PTZ_CH(prChn);
#if 0 /* Single write Tx queue data */
			for(i = 0; i < ptzSendLength; i++)
			{
				i2cData = pPtzCmd[i+ptzSendInx];
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}
#else /* addr hold en burst write, speed up */
			if( PR1000_PageWriteBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, ptzCmdLength, &pPtzCmd[ptzSendInx]) < 0)
			{
				ErrorString("PageWriteBurst.\n");
				return(-1);
			}
#endif

			/* verify */
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_SIZE + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
			{
				ErrorString("Read reg.\n");
				return(-1);
			}
			txFifoSize = i2cData;
			if(txFifoSize != ptzSendLength)
			{
				Error("Invalid [wr:0x%x, rd:0x%x]\n", ptzSendLength COMMA txFifoSize);
				return(-1);
			}
			Dbg("Write Tx data to Fifo %dByte.\n", ptzSendLength);

			/* start tx */
			DbgString("Start ptz tx.\n");
			PR1000_PTZ_StartTX(fd, pstPortChSel, START);

			ptzSendInx += ptzSendLength;
			ret = ptzCmdLength;
		}
	}

	return(ret);
}

int PR1000_PTZ_HDT_STDFORMAT_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength)
{
	int ret = 0;

	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;


	int bWaitCnt;
	uint8_t ptzCmdTxGroupCnt;
	uint8_t camResol = resol;

	uint16_t ptzSendLength;
	uint16_t ptzSendInx;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if( (pPtzCmd == NULL) || (ptzCmdLength == 0) )
	{
		ErrorString("Can't support ptz command.\n");
		return(-1);
	}

	/* convert cif,shrink resolution to camera resolution. */
	switch(resol)
	{
		case pr1000_outresol_1280x720p60c: camResol = pr1000_outresol_1280x720p60; break;
		case pr1000_outresol_1280x720p50c: camResol = pr1000_outresol_1280x720p50; break;
		case pr1000_outresol_1280x720p30c: camResol =  pr1000_outresol_1280x720p30; break;
		case pr1000_outresol_1280x720p25c: camResol =  pr1000_outresol_1280x720p25; break;
		case pr1000_outresol_1920x1080p30c: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25c: camResol =  pr1000_outresol_1920x1080p25; break;
		case pr1000_outresol_1920x1080p30s: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25s: camResol =  pr1000_outresol_1920x1080p25; break;
		default: break;
	}

	ptzSendLength = PR1000_PTZ_HDT_TXCMD_BASE_BYTE_CNT;
	ptzCmdTxGroupCnt = (ptzCmdLength/PR1000_PTZ_HDT_TXCMD_BASE_BYTE_CNT);
	if(ptzCmdLength%PR1000_PTZ_HDT_TXCMD_BASE_BYTE_CNT)
	{
		ErrorString("Invalid ptz command length.\n");
		return(-1);
	}
	Dbg("Set tx group cnt:0x%x\n", ptzCmdTxGroupCnt);

#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
	{
		int i;
		printk("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printk("0x%02X ", pPtzCmd[i]);
		}
		printk("\n");
	}
#else
	{
		int i;
		printf("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printf("0x%02X ", pPtzCmd[i]);
		}
		printf("\n\r");
	}
#endif
#endif

	ptzSendInx = 0;
	while(ptzSendInx < ptzCmdLength)
	{
		Dbg("sendinx:%d/%d\n", ptzSendInx COMMA ptzCmdLength);

		/* wait send tx status. */
		bWaitCnt = 10;
		while( (PR1000_PTZ_CheckSendTxStatus(fd, pstPortChSel) < 0) && bWaitCnt-- )
		{
			DbgString("Wait send tx status.\n");
#ifdef __LINUX_SYSTEM__
			mdelay(100);
#else //#ifdef __LINUX_SYSTEM__
			MDelay(100);
#endif // __LINUX_SYSTEM__
		}
		if(bWaitCnt <= 0)
		{
			ErrorString("TimeOver send tx status.\n");
			ret = -1;
			break;
		}
		else
		{
			uint8_t txFifoSize = 0;

			/* set group count */
			i2cData = 0x81; //0x80:enable
			i2cReg = PR1000_REG_ADDR_PTZ_TX_GROUP_CNT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			/* init data reg */
			i2cMask = 0xC7; i2cData = 0x00|((PR1000_PTZ_HDT_STDFORMAT_TX_DATA_BITSWAP&0x1)<<6);
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_DATA + PR1000_OFFSETADDR_PTZ_CH(prChn);
#if 0 /* Single write Tx queue data */
			for(i = 0; i < ptzSendLength; i++)
			{
				i2cData = pPtzCmd[i+ptzSendInx];
				if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}
#else /* addr hold en burst write, speed up */
			if( PR1000_PageWriteBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, ptzCmdLength, &pPtzCmd[ptzSendInx]) < 0)
			{
				ErrorString("PageWriteBurst.\n");
				return(-1);
			}
#endif

			/* verify */
			i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_SIZE + PR1000_OFFSETADDR_PTZ_CH(prChn);
			if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
			{
				ErrorString("Read reg.\n");
				return(-1);
			}
			txFifoSize = i2cData;
			if(txFifoSize != ptzSendLength)
			{
				Error("Invalid [wr:0x%x, rd:0x%x]\n", ptzSendLength COMMA txFifoSize);
				return(-1);
			}
			Dbg("Write Tx data to Fifo %dByte.\n", ptzSendLength);

			/* start tx */
			DbgString("Start ptz tx.\n");
			PR1000_PTZ_StartTX(fd, pstPortChSel, START);

			ptzSendInx += ptzSendLength;
			ret = ptzCmdLength;
		}
	}

	return(ret);
}

int PR1000_PTZ_HDT_CHGOLD_SendTxCmd(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol)
{
	int ret = 0;

	uint8_t i2cMask;
	uint8_t i2cReg, i2cData;


	int bWaitCnt;
	uint8_t ptzCmdTxGroupCnt;
	uint8_t camResol = resol;

	const uint8_t pr1000_ptz_table_hdt_chgold[]       = 	{0x40,0x21,0xC0,0x00,/**/0x00,0x40,0x0F,0x40,};

	const uint8_t *pPtzCmd = pr1000_ptz_table_hdt_chgold;
	uint16_t ptzCmdLength = sizeof(pr1000_ptz_table_hdt_chgold);
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	/* convert cif,shrink resolution to camera resolution. */
	switch(resol)
	{
		case pr1000_outresol_1280x720p60c: camResol = pr1000_outresol_1280x720p60; break;
		case pr1000_outresol_1280x720p50c: camResol = pr1000_outresol_1280x720p50; break;
		case pr1000_outresol_1280x720p30c: camResol =  pr1000_outresol_1280x720p30; break;
		case pr1000_outresol_1280x720p25c: camResol =  pr1000_outresol_1280x720p25; break;
		case pr1000_outresol_1920x1080p30c: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25c: camResol =  pr1000_outresol_1920x1080p25; break;
		case pr1000_outresol_1920x1080p30s: camResol =  pr1000_outresol_1920x1080p30; break;
		case pr1000_outresol_1920x1080p25s: camResol =  pr1000_outresol_1920x1080p25; break;
		default: break;
	}

	ptzCmdTxGroupCnt = (ptzCmdLength/PR1000_PTZ_HDT_TXCMD_BASE_BYTE_CNT) | 0x80; // 0x80:enable
	Dbg("Set tx group cnt:0x%x.\n", ptzCmdTxGroupCnt);
	i2cData = ptzCmdTxGroupCnt;
	i2cReg = PR1000_REG_ADDR_PTZ_TX_GROUP_CNT + PR1000_OFFSETADDR_PTZ_CH(prChn);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	if( (pPtzCmd == NULL) || (ptzCmdLength == 0) )
	{
		ErrorString("Can't support ptz command.\n");
		return(-1);
	}

#ifdef DEBUG_PTZ_PR1000
#ifdef __LINUX_SYSTEM__
	{
		int i;
		printk("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printk("0x%02X ", pPtzCmd[i]);
		}
		printk("\n");
	}
#else
	{
		int i;
		printf("Cmd Send(%d):", ptzCmdLength);
		for(i = 0; i < ptzCmdLength; i++)
		{
			printf("0x%02X ", pPtzCmd[i]);
		}
		printf("\n\r");
	}
#endif
#endif

	/* wait send tx status. */
	bWaitCnt = 10;
	while( (PR1000_PTZ_CheckSendTxStatus(fd, pstPortChSel) < 0) && bWaitCnt-- )
	{
		DbgString("Wait send tx status.\n");
#ifdef __LINUX_SYSTEM__
		mdelay(30);
#else //#ifdef __LINUX_SYSTEM__
		MDelay(30);
#endif // __LINUX_SYSTEM__
	}
	if(bWaitCnt <= 0)
	{
		ErrorString("TimeOver send tx status.\n");
		ret = -1;
	}
	else
	{
		uint8_t txFifoSize = 0;

		/* init data reg */
		i2cMask = 0xC7; i2cData = 0x00|((PR1000_PTZ_HDT_CHGOLD_TX_DATA_BITSWAP&0x1)<<6);
		i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_DATA + PR1000_OFFSETADDR_PTZ_CH(prChn);
#if 0 /* Single write Tx queue data */
		for(i = 0; i < ptzCmdLength; i++)
		{
			i2cData = pPtzCmd[i];
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}
		}
#else /* addr hold en burst write, speed up */
		if( PR1000_PageWriteBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, ptzCmdLength, pPtzCmd) < 0)
		{
			ErrorString("PageWriteBurst.\n");
			return(-1);
		}
#endif

		/* verify */
		i2cReg = PR1000_REG_ADDR_PTZ_FIFO_WR_SIZE + PR1000_OFFSETADDR_PTZ_CH(prChn);
		if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, &i2cData)) < 0)
		{
			ErrorString("Read reg.\n");
			return(-1);
		}
		txFifoSize = i2cData;
		if(txFifoSize != ptzCmdLength)
		{
			Error("Invalid mapChn:%d [wr:0x%x, rd:0x%x]\n", mapChn COMMA ptzCmdLength COMMA txFifoSize);
			return(-1);
		}
		Dbg("Write Tx data to Fifo %dByte.\n", ptzCmdLength);

		/* start tx */
		DbgString("Start ptz tx.\n");
		PR1000_PTZ_StartTX(fd, pstPortChSel, START);

		ret = ptzCmdLength;
	}

	return(ret);
}
#endif // DONT_SUPPORT_PTZ_FUNC


//////////////////////////////////////////////////////////////////////////////////////////////////////
/*** PR1000 - VEVENT control. ***/
//////////////////////////////////////////////////////////////////////////////////////////////////////
int PR1000_VEVENT_LoadTable(const int fd, const _stPortChSel *pstPortChSel, const uint8_t format, const uint8_t resol)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint16_t i2cData = 0;
	_PR1000_REG_TABLE_VEVENT *pTable;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}
	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	Print("> PixelPlus PR1000 VEVENT Load table [mapChn:%d, %s, %s]\n", mapChn COMMA _STR_PR1000_FORMAT[format] COMMA _STR_PR1000_OUTRESOL[resol]);

	/* loading vevent channel table register */
	if(resol >= max_pr1000_outresol)
	{
		Error("Invalid resol(%d).\n", resol);
		return(-1);
	}

	pTable = (_PR1000_REG_TABLE_VEVENT *)pr1000_reg_table_vevent;
	while(pTable->addr != 0xFF)
	{
		i2cReg = pTable->addr + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
		i2cData = pTable->pData[resol];
		pTable++;

		if(i2cData == 0xFF00) continue; //skip
		else if(i2cData == 0xFFFF) break; //end
		else
		{
			Dbg("Write [p%d, 0x%02x-0x%04x]\n", (uint8_t)(i2cData>>8) COMMA i2cReg COMMA i2cData);
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, (uint8_t)(i2cData>>8), i2cReg, (uint8_t)i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

		}
	}

	return(ret);
}

#ifndef DONT_SUPPORT_EVENT_FUNC
int PR1000_VEVENT_SetMaskAttr(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const _stMaskCellAttr *pstMaskCellAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstMaskCellAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

 	if( (pstMaskCellAttr->cellCntX == 0) || (pstMaskCellAttr->cellCntY == 0) || (pstMaskCellAttr->cellWidth == 0) || (pstMaskCellAttr->cellHeight == 0) )
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set MaskCell mapChn:%d Resol:%s size:%dx%d width:%dx%d off:%dx%d\n", mapChn COMMA 
		_STR_PR1000_OUTRESOL[resol] COMMA
		pstMaskCellAttr->cellCntX COMMA pstMaskCellAttr->cellCntY COMMA 
		pstMaskCellAttr->cellWidth COMMA pstMaskCellAttr->cellHeight COMMA
		pstMaskCellAttr->cellHStartOffset COMMA pstMaskCellAttr->cellVStartOffset);

	i2cReg = 0x15 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x3F; i2cData = (pstMaskCellAttr->cellCntX-1) & 0x3F;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x14 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x3F; i2cData = (pstMaskCellAttr->cellCntY-1) & 0x3F;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x13 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x7F; i2cData = (pstMaskCellAttr->cellWidth-1) & 0x7F;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x12 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x7F; i2cData = (pstMaskCellAttr->cellHeight-1) & 0x7F;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x16 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cData = (pstMaskCellAttr->cellVStartOffset) & 0xFF;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x17 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cData = (((pstMaskCellAttr->cellHStartOffset>>8) & 0x3)<<4) | ((pstMaskCellAttr->cellVStartOffset>>8) & 0x3);
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x18 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cData = (pstMaskCellAttr->cellHStartOffset) & 0xFF;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x5A + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x3F; i2cData = (pstMaskCellAttr->cellCntX-1) & 0x3F;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x5B + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x3F; i2cData = (pstMaskCellAttr->cellCntY-1) & 0x3F;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x58 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x7F; i2cData = (pstMaskCellAttr->cellWidth-1) & 0x7F;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x59 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x7F; i2cData = (pstMaskCellAttr->cellHeight-1) & 0x7F;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x55 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x0F; 
	i2cData = (((pstMaskCellAttr->cellVStartOffset>>8)&0x3)<<2) || (((pstMaskCellAttr->cellHStartOffset>>8)&0x3));
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x56 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = (pstMaskCellAttr->cellHStartOffset) & 0xFF;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x57 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cData = (pstMaskCellAttr->cellVStartOffset) & 0xFF;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetMaskAttr(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, _stMaskCellAttr *pstMaskCellAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstMaskCellAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	i2cReg = 0x15 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x3F; 
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMaskCellAttr->cellCntX = (i2cData+1) & 0x3F;

	i2cReg = 0x14 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x3F; 
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMaskCellAttr->cellCntY = (i2cData+1) & 0x3F;

	i2cReg = 0x13 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x7F; 
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMaskCellAttr->cellWidth = (i2cData+1) & 0x7F;

	i2cReg = 0x12 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x7F; 
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMaskCellAttr->cellHeight = (i2cData+1) & 0x7F;

	i2cReg = 0x16 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMaskCellAttr->cellVStartOffset = i2cData;

	i2cReg = 0x18 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMaskCellAttr->cellHStartOffset = i2cData;

	i2cReg = 0x17 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMaskCellAttr->cellHStartOffset |= ((i2cData>>4)&0x7)<<8;
	pstMaskCellAttr->cellVStartOffset |= (i2cData&0x7)<<8;

	#if 0
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x5A + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x3F; 
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMaskCellAttr->cellCntX = (i2cData+1) & 0x3F;

	i2cReg = 0x5B + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x3F; 
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMaskCellAttr->cellCntY = (i2cData+1) & 0x3F;

	i2cReg = 0x58 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x7F; 
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMaskCellAttr->cellWidth = (i2cData+1) & 0x7F;

	i2cReg = 0x59 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x7F; 
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMaskCellAttr->cellHeight = (i2cData+1) & 0x7F;

	i2cReg = 0x55 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMaskCellAttr->cellVStartOffset = ((i2cData>>2)&0x3)<<8;
	pstMaskCellAttr->cellHStartOffset = (i2cData&0x3)<<8;

	i2cReg = 0x56 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMaskCellAttr->cellHStartOffset |= i2cData;

	i2cReg = 0x57 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, page, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMaskCellAttr->cellVStartOffset |= i2cData;
	#endif

	Dbg("Get MaskCell mapChn:%d Resol:%s size:%dx%d width:%dx%d off:%dx%d\n", mapChn COMMA 
		_STR_PR1000_OUTRESOL[resol] COMMA
		pstMaskCellAttr->cellCntX COMMA pstMaskCellAttr->cellCntY COMMA 
		pstMaskCellAttr->cellWidth COMMA pstMaskCellAttr->cellHeight COMMA
		pstMaskCellAttr->cellHStartOffset COMMA pstMaskCellAttr->cellVStartOffset);

	return(ret);
}


int PR1000_VEVENT_SetMaskData(const int fd, const _stPortChSel *pstPortChSel, const enum _pr1000_vevent_mem_type maskType, const uint8_t startLine, const uint8_t lineCnt, const uint32_t *pu32MaskLineData)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int i, datInx;

	uint32_t maskData;
	uint8_t arrayData[4]; //32bit
	uint8_t wrLoc;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if( (maskType != PR1000_VMASK_MD) && (maskType != PR1000_VMASK_BD) && (maskType != PR1000_VMASK_PZ) )
	{
		ErrorString("Invalid maskType.\n");
		return(-1);
	}

	wrLoc = maskType;
	for(wrLoc += startLine, datInx = 0; wrLoc < (maskType + lineCnt); wrLoc++, datInx++)
	{
		/* write data */
		i2cReg = 0x00 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
		maskData = (uint32_t)*(pu32MaskLineData+datInx);
		for(i = 0; i < 4; i++)
		{
			arrayData[i] = (maskData >> (i*8)) & 0xFF;
		}
		if( (ret = PR1000_PageWriteBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, 4, arrayData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		/* write addr */
		i2cReg = 0x04 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
		i2cData = wrLoc;
		if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		/* enable write */
		i2cReg = 0x05 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
		i2cData = 0x40;
		if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}
	ret = datInx;

	/* disable write */
	i2cReg = 0x05 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cData = 0x00;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetMaskData(const int fd, const _stPortChSel *pstPortChSel, const enum _pr1000_vevent_mem_type maskType, const uint8_t startLine, const uint8_t lineCnt, uint32_t *pu32RetLineData)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int i, datInx;

	uint32_t maskData;
	uint8_t arrayData[4]; //32bit
	uint8_t rdLoc;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if( (maskType != PR1000_VMASK_MD) && (maskType != PR1000_VMASK_BD) && (maskType != PR1000_VMASK_PZ) )
	{
		ErrorString("Invalid maskType.\n");
		return(-1);
	}

	if(pu32RetLineData == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	/* read data */
	rdLoc = maskType;
	for(rdLoc += startLine, datInx = 0; rdLoc < (maskType + lineCnt); rdLoc++, datInx++)
	{
		/* read addr */
		i2cReg = 0x04 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
		i2cData = rdLoc;
		if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		/* enable read */
		i2cReg = 0x05 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
		i2cData = 0x20;
		if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		/* read data */
		i2cReg = 0x08 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
		if( (ret = PR1000_PageReadBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, 4, arrayData)) < 0)
		{
			ErrorString("Read reg.\n");
			return(-1);
		}

		for(maskData = 0, i = 0; i < 4; i++)
		{
			maskData |= arrayData[i]<<(i*8);
		}

		*(pu32RetLineData+datInx) = maskData;
	}
	ret = datInx;

	/* disable read */
	i2cReg = 0x05 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cData = 0x00;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_ClearMask(const int fd, const _stPortChSel *pstPortChSel)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	/* set clear bit */
	i2cReg = 0x05 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cData = 0x80;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	return(ret);
}

int PR1000_VEVENT_GetDetData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t startLine, const uint8_t lineCnt, uint64_t *pu64RetLineData)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int i, datInx;

	uint32_t maskData;
	uint8_t arrayData[4]; //32bit
	uint8_t rdLoc;
	uint64_t u64Data;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pu64RetLineData == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	rdLoc = PR1000_VMD_DET;
	for(rdLoc += startLine, datInx = 0; rdLoc < (PR1000_VMD_DET + lineCnt)*2; rdLoc+=2, datInx++)
	{
		/* read left 32bit */
		{/*{{{*/
			/* read addr */
			i2cReg = 0x04 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
			i2cData = rdLoc + 0;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			/* enable read */
			i2cReg = 0x05 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
			i2cData = 0x20;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			/* read data */
			i2cReg = 0x08 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
			if( (ret = PR1000_PageReadBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, 4, arrayData)) < 0)
			{
				ErrorString("Read reg.\n");
				return(-1);
			}

			for(maskData = 0, i = 0; i < 4; i++)
			{
				maskData |= arrayData[i]<<(i*8);
			}
		}/*}}}*/
		u64Data = maskData;

		/* read right 32bit */
		{/*{{{*/
			/* read addr */
			i2cReg = 0x04 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
			i2cData = rdLoc + 1;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			/* enable read */
			i2cReg = 0x05 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
			i2cData = 0x20;
			if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
			{
				ErrorString("Write reg.\n");
				return(-1);
			}

			/* read data */
			i2cReg = 0x08 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
			if( (ret = PR1000_PageReadBurst(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, 4, arrayData)) < 0)
			{
				ErrorString("Read reg.\n");
				return(-1);
			}

			for(maskData = 0, i = 0; i < 4; i++)
			{
				maskData |= arrayData[i]<<(i*8);
			}
		}/*}}}*/
		u64Data |= (((uint64_t)maskData)<<32);


		*(pu64RetLineData+datInx) = u64Data;
		Dbg("Read [p%d, 0x%02x-%08x%08x]\n", PR1000_REG_PAGE_VEVENT COMMA i2cReg COMMA (uint32_t)(u64Data>>32) COMMA (uint32_t)u64Data);
	}
	ret = datInx;

	return(ret);
}

int PR1000_VEVENT_SetDisplayCellFormat(const int fd, const _stPortChSel *pstPortChSel, const _stVEVENTDisplayAttr *pstVeventDisplayAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int page;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstVeventDisplayAttr->cellFormat < PR1000_VEVENT_DISP_NONE)
	{
		Print("Set display mask cell mapCh:%d, type:%d(%s).\n", mapChn COMMA pstVeventDisplayAttr->cellFormat COMMA (pstVeventDisplayAttr->cellFormat==PR1000_VEVENT_DISP_PZ)?"PZ":((pstVeventDisplayAttr->cellFormat==PR1000_VEVENT_DISP_BD)?"BD":"MD"));
		page = PR1000_VDEC_PAGE(prChn);
		i2cReg = 0x5A + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
		i2cMask = 0xC0;
		i2cData = (pstVeventDisplayAttr->cellFormat&0x3)<<6;
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}

	{
		page = PR1000_VDEC_PAGE(prChn);
		i2cReg = 0x53 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
		i2cMask = 0x1F;
		i2cData = (pstVeventDisplayAttr->maskBlendLevel & 0x1F);
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}

	{
		page = PR1000_VDEC_PAGE(prChn);
		i2cReg = 0x54 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
		i2cMask = 0xC0;
		i2cData = (pstVeventDisplayAttr->detBndrLevel & 0x3) << 6;
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}

	{
		page = PR1000_VDEC_PAGE(prChn);
		i2cReg = 0x54 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
		i2cMask = 0x30;
		i2cData = (pstVeventDisplayAttr->detBndrWidth & 0x3) << 4;
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}

	{
		page = PR1000_VDEC_PAGE(prChn);
		i2cReg = 0x54 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
		i2cMask = 0x0C;
		i2cData = (pstVeventDisplayAttr->maskBndrLevel & 0x3) << 2;
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}

	{
		page = PR1000_VDEC_PAGE(prChn);
		i2cReg = 0x54 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
		i2cMask = 0x03;
		i2cData = (pstVeventDisplayAttr->maskBndrWidth & 0x3);
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}

	{
		page = PR1000_VDEC_PAGE(prChn);
		i2cReg = 0x55 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
		i2cMask = 0x70;
		i2cData = (pstVeventDisplayAttr->maskColor & 0x7) << 4;
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}

	if(pstVeventDisplayAttr->cellFormat == PR1000_VEVENT_DISP_BD)
	{
		i2cReg = 0x19 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
		i2cMask = 0x20;
		i2cData = 0x20;
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}
	else if(pstVeventDisplayAttr->cellFormat == PR1000_VEVENT_DISP_MD)
	{
		i2cReg = 0x19 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
		i2cMask = 0x20;
		i2cData = 0x00;
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}
	else if(pstVeventDisplayAttr->cellFormat == PR1000_VEVENT_DISP_NONE)
	{
		page = PR1000_VDEC_PAGE(prChn);
		i2cReg = 0x54 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn); //maks bndr
		i2cMask = 0x0F;
		i2cData = 0x00;
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}

		i2cReg = 0x19 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
		i2cMask = 0x20;
		i2cData = 0x00;
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}

	{
		page = PR1000_VDEC_PAGE(prChn);
		i2cReg = 0x53 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
		i2cMask = 0x80;
		i2cData = (pstVeventDisplayAttr->bEnMaskPln & 0x01) << 7;
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}

	{
		page = PR1000_VDEC_PAGE(prChn);
		i2cReg = 0x53 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
		i2cMask = 0x40;
		i2cData = (pstVeventDisplayAttr->bEnDetPln & 0x01) << 6;
		if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
		{
			ErrorString("Write reg.\n");
			return(-1);
		}
	}


	ret = 0;

	return(ret);
}

int PR1000_VEVENT_WriteMaskFormat(const int fd, const _stPortChSel *pstPortChSel, const enum _pr1000_vevent_mem_type maskType, const uint32_t *pFormat)
{
	int ret = -1;

	uint32_t maskBuf[PR1000_MAX_MASK_CELL_Y_NUM] = { 0,  };

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid.\n");
		return(-1);
	}

	if( (maskType != PR1000_VMASK_MD) && (maskType != PR1000_VMASK_BD) && (maskType != PR1000_VMASK_PZ) )
	{
		ErrorString("Invalid maskType.\n");
		return(-1);
	}

	memcpy(maskBuf, pFormat, sizeof(uint32_t)*PR1000_MAX_MASK_CELL_Y_NUM);

	if( (ret = PR1000_VEVENT_SetMaskData(fd, pstPortChSel, maskType, 0, PR1000_MAX_MASK_CELL_Y_NUM, maskBuf)) < 0 ) 
	{
		ErrorString("Set mask data.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_ReadMaskFormat(const int fd, const _stPortChSel *pstPortChSel, const enum _pr1000_vevent_mem_type maskType, uint32_t *pFormat)
{
	int ret = -1;

	uint32_t maskBuf[PR1000_MAX_MASK_CELL_Y_NUM] = { 0,  };

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid.\n");
		return(-1);
	}

	if( (maskType != PR1000_VMASK_MD) && (maskType != PR1000_VMASK_BD) && (maskType != PR1000_VMASK_PZ) )
	{
		ErrorString("Invalid maskType.\n");
		return(-1);
	}

	if( (ret = PR1000_VEVENT_GetMaskData(fd, pstPortChSel, maskType, 0, PR1000_MAX_MASK_CELL_Y_NUM, maskBuf)) < 0 ) 
	{
		ErrorString("Set mask data.\n");
		return(-1);
	}

	memcpy(pFormat, maskBuf, sizeof(uint32_t)*PR1000_MAX_MASK_CELL_Y_NUM);

	return(ret);
}


int PR1000_VEVENT_PrintDetData(const int fd, const uint8_t resol, const uint64_t *pData, const int lineCnt)
{
	int ret = -1;

	int srcInx, dstBitInx, dstSize;
	uint64_t u64Byte;
	_stMaskCellAttr *pstCell = (_stMaskCellAttr *)&pr1000_mask_attr_table_vevent[resol];
	int endLine;

	if(lineCnt  <= 0)
	{
		Error("Invalid lineCnt :%d.\n", lineCnt);
		return(-1);
	}

#ifdef __LINUX_SYSTEM__
	/* clear screen, move cursor to 0,0 */
	printk("%c[2J", 0x1B); // clear screen.
	printk("%c[0;0H", 0x1B); // move cursor to 0,0

	/* print data */
	dstSize = 0;
	endLine = (pstCell->cellCntY > lineCnt) ? lineCnt:pstCell->cellCntY;
	printk("Det Resol:%dx%d end:%d.\n", pstCell->cellCntX, pstCell->cellCntY, endLine);

#if 1
	/* print 1,0 converting */
	printk("\t  :");
	for(dstBitInx = 0; dstBitInx < pstCell->cellCntX; dstBitInx++)
	{
		printk("%d", dstBitInx%10);
	}
	printk("\n");
	for(srcInx = 0; srcInx < endLine; srcInx++)
	{
		u64Byte = (uint64_t)*(pData + srcInx);

		printk("\t%02d:", srcInx);
		for(dstBitInx = 0; dstBitInx < pstCell->cellCntX; dstBitInx++)
		{
			if( u64Byte & ((uint64_t)(1<<dstBitInx)) ) 
				printk("1");
			else 
				printk("0");
		}
		printk("\n");
	}
#else
	/* print raw data */
	for(srcInx = 0; srcInx < endLine; srcInx++)
	{
		u64Byte = (uint64_t)*(pData + srcInx);
		u64Byte = Swap64(u64Byte);

		printk("\t%02d:%08x%08x\n", srcInx, (uint32_t)(u64Byte>>32), (uint32_t)u64Byte);
	}

#endif

#else //#ifdef __LINUX_SYSTEM__

	/* clear screen, move cursor to 0,0 */
	printf("%c[2J", 0x1B); // clear screen.
	printf("%c[0;0H", 0x1B); // move cursor to 0,0

	/* print data */
	dstSize = 0;
	endLine = (pstCell->cellCntY > lineCnt) ? lineCnt:pstCell->cellCntY;
	printf("Det Resol:%dx%d end:%d.\n\r", pstCell->cellCntX, pstCell->cellCntY, endLine);

#if 1
	/* print 1,0 converting */
	printf("\t  :");
	for(dstBitInx = 0; dstBitInx < pstCell->cellCntX; dstBitInx++)
	{
		printf("%d", dstBitInx%10);
	}
	printf("\n\r");
	for(srcInx = 0; srcInx < endLine; srcInx++)
	{
		u64Byte = (uint64_t)*(pData + srcInx);

		printf("\t%02d:", srcInx);
		for(dstBitInx = 0; dstBitInx < pstCell->cellCntX; dstBitInx++)
		{
			if( u64Byte & ((uint64_t)(1<<dstBitInx)) ) 
				printf("1");
			else 
				printf("0");
		}
		printf("\n\r");
	}
#else
	/* print raw data */
	for(srcInx = 0; srcInx < endLine; srcInx++)
	{
		u64Byte = (uint64_t)*(pData + srcInx);
		u64Byte = Swap64(u64Byte);

		printf("\t%02d:%08x%08x\n\r", srcInx, (uint32_t)(u64Byte>>32), (uint32_t)u64Byte);
	}

#endif

#endif // __LINUX_SYSTEM__

	ret = srcInx*dstBitInx;

	return(ret);
}
#endif // DONT_SUPPORT_EVENT_FUNC

int PR1000_VEVENT_SetNovidAttr(const int fd, const _stPortChSel *pstPortChSel, const _stNovidAttr *pstNovidAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 
	int page;

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstNovidAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Novid attribute [mapCh:%d]\n", mapChn);

	/* blankColor */
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x00 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x01; 
	i2cData = pstNovidAttr->blankColor & 1;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetNovidAttr(const int fd, const _stPortChSel *pstPortChSel, _stNovidAttr *pstNovidAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 
	int page;

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstNovidAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Novid attribute [mapCh:%d]\n", mapChn);

	/* blankColor */
	page = PR1000_VDEC_PAGE(prChn);
	i2cReg = 0x00 + PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
	i2cMask = 0x01; 
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstNovidAttr->blankColor = i2cData & 1;

	return(ret);
}

#ifndef DONT_SUPPORT_EVENT_FUNC
int PR1000_VEVENT_SetMdAttr(const int fd, const _stPortChSel *pstPortChSel, const _stMdAttr *pstMdAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstMdAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Md attribute [mapCh:%d]\n", mapChn);

	/* bMaskEn */
	i2cReg = 0x19 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x40;
	i2cData = (pstMdAttr->bMaskEn & 0x01)<<6;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	/* bEn */
	i2cReg = 0x19 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x80;
	i2cData = (pstMdAttr->bEn & 0x01)<<7;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetMdAttr(const int fd, const _stPortChSel *pstPortChSel, _stMdAttr *pstMdAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstMdAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Md attribute [mapCh:%d]\n", mapChn);

	/* bMaskEn */
	i2cReg = 0x19 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x40;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMdAttr->bMaskEn = (i2cData & 0x40)>>6;

	/* bEn */
	i2cReg = 0x19 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x80;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMdAttr->bEn = (i2cData & 0x80)>>7;

	return(ret);
}

int PR1000_VEVENT_SetMdLvSens(const int fd, const _stPortChSel *pstPortChSel, const _stMdLvSens *pstMdLvSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstMdLvSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Md attribute [mapCh:%d]\n", mapChn);

	/* lvsens */
	i2cReg = 0x1A + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	i2cData = pstMdLvSens->lvsens & 0xFF;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x1B + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x0F;
	i2cData = (pstMdLvSens->lvsens >> 8) & 0x0F;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetMdLvSens(const int fd, const _stPortChSel *pstPortChSel, _stMdLvSens *pstMdLvSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstMdLvSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Md attribute [mapCh:%d]\n", mapChn);

	/* lvsens */
	i2cReg = 0x1A + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMdLvSens->lvsens = i2cData & 0xFF;

	i2cReg = 0x1B + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x0F;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMdLvSens->lvsens |= (i2cData & 0x0F)<<8;

	return(ret);
}

int PR1000_VEVENT_SetMdSpSens(const int fd, const _stPortChSel *pstPortChSel, const _stMdSpSens *pstMdSpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstMdSpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Md attribute [mapCh:%d]\n", mapChn);

	/* spsens */
	i2cReg = 0x1C + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	i2cData = pstMdSpSens->spsens & 0xFF;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	i2cReg = 0x1B + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xF0;
	i2cData = ((pstMdSpSens->spsens >> 8) & 0x0F)<<4;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetMdSpSens(const int fd, const _stPortChSel *pstPortChSel, _stMdSpSens *pstMdSpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstMdSpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Md attribute [mapCh:%d]\n", mapChn);

	/* spsens */
	i2cReg = 0x1C + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMdSpSens->spsens = i2cData & 0xFF;

	i2cReg = 0x1B + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xF0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMdSpSens->spsens |= ((i2cData & 0xF0)>>4)<<8;

	return(ret);
}

int PR1000_VEVENT_SetMdTmpSens(const int fd, const _stPortChSel *pstPortChSel, const _stMdTmpSens *pstMdTmpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstMdTmpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Md attribute [mapCh:%d]\n", mapChn);

	/* tmpsens */
	i2cReg = 0x1D + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cData = pstMdTmpSens->tmpsens & 0xFF;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetMdTmpSens(const int fd, const _stPortChSel *pstPortChSel, _stMdTmpSens *pstMdTmpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstMdTmpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Md attribute [mapCh:%d]\n", mapChn);

	/* tmpsens */
	i2cReg = 0x1D + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstMdTmpSens->tmpsens = i2cData & 0xFF;

	return(ret);
}

int PR1000_VEVENT_SetMdVelocity(const int fd, const _stPortChSel *pstPortChSel, const _stMdVelocity *pstMdVelocity)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstMdVelocity == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Md attribute [mapCh:%d]\n", mapChn);

	/* velocity */
	i2cReg = 0x11 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cData = pstMdVelocity->velocity;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetMdVelocity(const int fd, const _stPortChSel *pstPortChSel, _stMdVelocity *pstMdVelocity)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstMdVelocity == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Md attribute [mapCh:%d]\n", mapChn);

	/* velocity */
	i2cReg = 0x11 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	pstMdVelocity->velocity = i2cData;

	return(ret);
}

int PR1000_VEVENT_SetBdAttr(const int fd, const _stPortChSel *pstPortChSel, const _stBdAttr *pstBdAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstBdAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Bd attribute [mapCh:%d]\n", mapChn);

	/* bMaskEn */
	/* If bMaskEn, calculate spsens 50% of unmask cell num on buffer. */
	i2cReg = 0x1E + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x40;
	i2cData = (pstBdAttr->bMaskEn & 0x01)<<6;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	/* bEn */
	i2cReg = 0x1E + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x80;
	i2cData = (pstBdAttr->bEn & 0x01)<<7;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}


	return(ret);
}

int PR1000_VEVENT_GetBdAttr(const int fd, const _stPortChSel *pstPortChSel, _stBdAttr *pstBdAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstBdAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Bd attribute [mapCh:%d]\n", mapChn);

	/* bMaskEn */
	/* If bMaskEn, calculate spsens 50% of unmask cell num on buffer. */
	i2cReg = 0x1E + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x40;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstBdAttr->bMaskEn  = (i2cData & 0x40)>>6;

	/* bEn */
	i2cReg = 0x1E + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x80;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstBdAttr->bEn = (i2cData & 0x80)>>7;

	return(ret);
}

int PR1000_VEVENT_SetBdSpSens(const int fd, const _stPortChSel *pstPortChSel, const _stBdSpSens *pstBdSpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstBdSpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Bd attribute [mapCh:%d]\n", mapChn);

	/* spsens */
	i2cReg = 0x21 + PR1000_OFFSETADDR_VEVENT_CH(mapChn); // ch base + offset
	i2cMask = 0xFF;
	i2cData = pstBdSpSens->spsens & 0xFF;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x20 + PR1000_OFFSETADDR_VEVENT_CH(mapChn); // ch base + offset
	i2cMask = 0xF0;
	i2cData = ((pstBdSpSens->spsens >> 8) & 0x0F)<<4;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetBdSpSens(const int fd, const _stPortChSel *pstPortChSel, _stBdSpSens *pstBdSpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstBdSpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Bd attribute [mapCh:%d]\n", mapChn);

	/* spsens */
	i2cReg = 0x21 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstBdSpSens->spsens = i2cData & 0xFF;

	i2cReg = 0x20 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xF0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstBdSpSens->spsens |= ((i2cData & 0xF0)>>4)<<8;

	return(ret);
}

int PR1000_VEVENT_SetBdLvSens(const int fd, const _stPortChSel *pstPortChSel, const _stBdLvSens *pstBdLvSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstBdLvSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Bd attribute [mapCh:%d]\n", mapChn);

	/* lvsens */
	i2cReg = 0x1F + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	i2cData = pstBdLvSens->lvsens & 0xFF;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x20 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x0F;
	i2cData = (pstBdLvSens->lvsens >> 8) & 0x0F;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetBdLvSens(const int fd, const _stPortChSel *pstPortChSel, _stBdLvSens *pstBdLvSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstBdLvSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Bd attribute [mapCh:%d]\n", mapChn);

	/* lvsens */
	i2cReg = 0x1F + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstBdLvSens->lvsens = i2cData & 0xFF;

	i2cReg = 0x20 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x0F;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstBdLvSens->lvsens |= (i2cData & 0x0F)<<8;

	return(ret);
}

int PR1000_VEVENT_SetBdTmpSens(const int fd, const _stPortChSel *pstPortChSel, const _stBdTmpSens *pstBdTmpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstBdTmpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Bd attribute [mapCh:%d]\n", mapChn);

	/* tmpsens */
	i2cReg = 0x22 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cData = pstBdTmpSens->tmpsens & 0xFF;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetBdTmpSens(const int fd, const _stPortChSel *pstPortChSel, _stBdTmpSens *pstBdTmpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstBdTmpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Bd attribute [mapCh:%d]\n", mapChn);

	/* tmpsens */
	i2cReg = 0x22 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstBdTmpSens->tmpsens  = i2cData & 0xFF;

	return(ret);
}

int PR1000_VEVENT_SetNdAttr(const int fd, const _stPortChSel *pstPortChSel, const _stNdAttr *pstNdAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstNdAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Nd attribute [mapCh:%d]\n", mapChn);

	/* bEn */
	i2cReg = 0x23 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x80;
	i2cData = (pstNdAttr->bEn & 0x01)<<7;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}


	return(ret);
}

int PR1000_VEVENT_GetNdAttr(const int fd, const _stPortChSel *pstPortChSel, _stNdAttr *pstNdAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstNdAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Nd attribute [mapCh:%d]\n", mapChn);

	/* bEn */
	i2cReg = 0x23 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x80;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstNdAttr->bEn = (i2cData & 0x80)>>7;

	return(ret);
}

int PR1000_VEVENT_SetNdLvSens(const int fd, const _stPortChSel *pstPortChSel, const _stNdLvSens *pstNdLvSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstNdLvSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Nd attribute [mapCh:%d]\n", mapChn);

	/* lvsens_low */
	i2cReg = 0x24 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	i2cData = pstNdLvSens->lvsens_low & 0xFF;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x25 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x0F;
	i2cData = (pstNdLvSens->lvsens_low >> 8) & 0x0F;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	/* lvsens_high */
	i2cReg = 0x26 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	i2cData = pstNdLvSens->lvsens_high & 0xFF;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x25 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xF0;
	i2cData = ((pstNdLvSens->lvsens_high >> 8) & 0x0F)<<4;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetNdLvSens(const int fd, const _stPortChSel *pstPortChSel, _stNdLvSens *pstNdLvSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstNdLvSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Nd attribute [mapCh:%d]\n", mapChn);

	/* lvsens_low */
	i2cReg = 0x24 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstNdLvSens->lvsens_low  = i2cData & 0xFF;

	i2cReg = 0x25 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x0F;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstNdLvSens->lvsens_low |= (i2cData & 0x0F)<<8;

	/* lvsens_high */
	i2cReg = 0x26 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstNdLvSens->lvsens_high = i2cData & 0xFF;

	i2cReg = 0x25 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xF0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstNdLvSens->lvsens_high |= ((i2cData & 0xF0)>>4)<<8;

	return(ret);
}

int PR1000_VEVENT_SetNdTmpSens(const int fd, const _stPortChSel *pstPortChSel, const _stNdTmpSens *pstNdTmpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstNdTmpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Nd attribute [mapCh:%d]\n", mapChn);

	/* tmpsens */
	i2cReg = 0x27 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cData = pstNdTmpSens->tmpsens & 0xFF;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetNdTmpSens(const int fd, const _stPortChSel *pstPortChSel, _stNdTmpSens *pstNdTmpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstNdTmpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Nd attribute [mapCh:%d]\n", mapChn);

	/* tmpsens */
	i2cReg = 0x27 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstNdTmpSens->tmpsens = i2cData & 0xFF;

	return(ret);
}

int PR1000_VEVENT_SetDdAttr(const int fd, const _stPortChSel *pstPortChSel, const _stDdAttr *pstDdAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstDdAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Dd attribute [mapCh:%d]\n", mapChn);

	/* bEn */
	i2cReg = 0x28 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x80;
	i2cData = (pstDdAttr->bEn & 0x01)<<7;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetDdAttr(const int fd, const _stPortChSel *pstPortChSel, _stDdAttr *pstDdAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstDdAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Dd attribute [mapCh:%d]\n", mapChn);

	/* bEn */
	i2cReg = 0x28 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x80;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDdAttr->bEn = (i2cData & 0x80)>>7;

	return(ret);
}

int PR1000_VEVENT_SetDdLvSens(const int fd, const _stPortChSel *pstPortChSel, const _stDdLvSens *pstDdLvSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstDdLvSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Dd attribute [mapCh:%d]\n", mapChn);

	/* lvsens_low */
	i2cReg = 0x29 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	i2cData = pstDdLvSens->lvsens_low & 0xFF;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x2A + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x0F;
	i2cData = (pstDdLvSens->lvsens_low >> 8) & 0x0F;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	/* lvsens_high */
	i2cReg = 0x2B + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	i2cData = pstDdLvSens->lvsens_high & 0xFF;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x2A + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xF0;
	i2cData = ((pstDdLvSens->lvsens_high >> 8) & 0x0F)<<4;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetDdLvSens(const int fd, const _stPortChSel *pstPortChSel, _stDdLvSens *pstDdLvSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstDdLvSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Dd attribute [mapCh:%d]\n", mapChn);

	/* lvsens_low */
	i2cReg = 0x29 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDdLvSens->lvsens_low = i2cData & 0xFF;

	i2cReg = 0x2A + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x0F;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDdLvSens->lvsens_low |= (i2cData & 0x0F)<<8;

	/* lvsens_high */
	i2cReg = 0x2B + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDdLvSens->lvsens_high = i2cData & 0xFF;

	i2cReg = 0x2A + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xF0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDdLvSens->lvsens_high |= ((i2cData & 0xF0)>>4)<<8;

	return(ret);
}

int PR1000_VEVENT_SetDdTmpSens(const int fd, const _stPortChSel *pstPortChSel, const _stDdTmpSens *pstDdTmpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstDdTmpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Dd attribute [mapCh:%d]\n", mapChn);

	/* tmpsens */
	i2cReg = 0x2C + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cData = pstDdTmpSens->tmpsens & 0xFF;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetDdTmpSens(const int fd, const _stPortChSel *pstPortChSel, _stDdTmpSens *pstDdTmpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstDdTmpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Dd attribute [mapCh:%d]\n", mapChn);

	/* tmpsens */
	i2cReg = 0x2C + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDdTmpSens->tmpsens = i2cData & 0xFF;

	return(ret);
}

int PR1000_VEVENT_SetDfdAttr(const int fd, const _stPortChSel *pstPortChSel, const _stDfdAttr *pstDfdAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstDfdAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Dfd attribute [mapCh:%d]\n", mapChn);

	/* vstart */
	i2cReg = 0x30 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	i2cData = pstDfdAttr->vstart & 0xFF;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x31 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x07;
	i2cData = (pstDfdAttr->vstart >> 8) & 0x07;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	/* hstart */
	i2cReg = 0x32 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	i2cData = pstDfdAttr->hstart & 0xFF;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x31 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x70;
	i2cData = ((pstDfdAttr->hstart >> 8) & 0x07)<<4;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	/* vsize */
	i2cReg = 0x33 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	i2cData = pstDfdAttr->vsize & 0xFF;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x34 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x07;
	i2cData = (pstDfdAttr->vsize >> 8) & 0x07;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	/* hsize */
	i2cReg = 0x35 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	i2cData = pstDfdAttr->hsize & 0xFF;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}
	i2cReg = 0x34 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x70;
	i2cData = ((pstDfdAttr->hsize >> 8) & 0x07)<<4;
	if( (ret = PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetDfdAttr(const int fd, const _stPortChSel *pstPortChSel, _stDfdAttr *pstDfdAttr)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstDfdAttr == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Dfd attribute [mapCh:%d]\n", mapChn);

	/* vstart */
	i2cReg = 0x30 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDfdAttr->vstart = i2cData & 0xFF;

	i2cReg = 0x31 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x07;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDfdAttr->vstart |= (i2cData & 0x07)<<8;

	/* hstart */
	i2cReg = 0x32 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDfdAttr->hstart = i2cData & 0xFF;

	i2cReg = 0x31 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x70;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDfdAttr->hstart |= ((i2cData & 0x70)>>4)<<8;

	/* vsize */
	i2cReg = 0x33 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDfdAttr->vsize = i2cData & 0xFF;

	i2cReg = 0x34 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x07;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDfdAttr->vsize |= (i2cData & 0x07)<<8;

	/* hsize */
	i2cReg = 0x35 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0xFF;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDfdAttr->hsize = i2cData & 0xFF;

	i2cReg = 0x34 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cMask = 0x70;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDfdAttr->hsize |= ((i2cData & 0x70)>>4)<<8;

	return(ret);
}

int PR1000_VEVENT_SetDfdLvSens(const int fd, const _stPortChSel *pstPortChSel, const _stDfdLvSens *pstDfdLvSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstDfdLvSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Dfd attribute [mapCh:%d]\n", mapChn);

	/* lvsens */
	i2cReg = 0x36 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cData = pstDfdLvSens->lvsens & 0xFF;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetDfdLvSens(const int fd, const _stPortChSel *pstPortChSel, _stDfdLvSens *pstDfdLvSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstDfdLvSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Dfd attribute [mapCh:%d]\n", mapChn);

	/* lvsens */
	i2cReg = 0x36 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDfdLvSens->lvsens = i2cData & 0xFF;

	return(ret);
}

int PR1000_VEVENT_SetDfdSpSens(const int fd, const _stPortChSel *pstPortChSel, const _stDfdSpSens *pstDfdSpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstDfdSpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Dfd attribute [mapCh:%d]\n", mapChn);

	/* spsens */
	i2cReg = 0x37 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cData = pstDfdSpSens->spsens & 0xFF;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetDfdSpSens(const int fd, const _stPortChSel *pstPortChSel, _stDfdSpSens *pstDfdSpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstDfdSpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Dfd attribute [mapCh:%d]\n", mapChn);

	/* spsens */
	i2cReg = 0x37 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDfdSpSens->spsens = i2cData & 0xFF;

	return(ret);
}

int PR1000_VEVENT_SetDfdTmpSens(const int fd, const _stPortChSel *pstPortChSel, const _stDfdTmpSens *pstDfdTmpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstDfdTmpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Set Dfd attribute [mapCh:%d]\n", mapChn);

	/* tmpsens */
	i2cReg = 0x38 + PR1000_OFFSETADDR_VEVENT_CH(prChn); // ch base + offset
	i2cData = pstDfdTmpSens->tmpsens & 0xFF;
	if( (ret = PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, i2cData)) < 0)
	{
		ErrorString("Write reg.\n");
		return(-1);
	}

	return(ret);
}

int PR1000_VEVENT_GetDfdTmpSens(const int fd, const _stPortChSel *pstPortChSel, _stDfdTmpSens *pstDfdTmpSens)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pstDfdTmpSens == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	Dbg("Get Dfd attribute [mapCh:%d]\n", mapChn);

	/* tmpsens */
	i2cReg = 0x38 + PR1000_OFFSETADDR_VEVENT_CH(mapChn); // ch base + offset
	if( (ret = PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_VEVENT, i2cReg, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	pstDfdTmpSens->tmpsens = i2cData & 0xFF;

	return(ret);
}
#endif // DONT_SUPPORT_EVENT_FUNC

int PR1000_VEVENT_GetNovidStatus(const int fd, const _stPortChSel *pstPortChSel, uint8_t *pStatus)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pStatus == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	/* IRQ Status */
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xC4; //VFD|NOVID
	i2cMask = 0x0F;
	i2cData = 0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	*pStatus = (i2cData>>prChn)&1;

	return(ret);
}

int PR1000_VEVENT_GetVfdStatus(const int fd, const _stPortChSel *pstPortChSel, uint8_t *pStatus)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pStatus == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	/* IRQ Status */
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xC4; //VFD|NOVID
	i2cMask = 0xF0;
	i2cData = 0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	*pStatus = ((i2cData>>4)>>prChn)&1;

	return(ret);
}

#ifndef DONT_SUPPORT_EVENT_FUNC
int PR1000_VEVENT_GetMdStatus(const int fd, const _stPortChSel *pstPortChSel, uint8_t *pStatus)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pStatus == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xC5; //BD|MD
	i2cMask = 0x0F;
	i2cData = 0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	*pStatus = (i2cData>>prChn)&1;

	return(ret);
}

int PR1000_VEVENT_GetBdStatus(const int fd, const _stPortChSel *pstPortChSel, uint8_t *pStatus)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pStatus == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xC5; //BD|MD
	i2cMask = 0xF0;
	i2cData = 0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	*pStatus = ((i2cData>>4)>>prChn)&1;

	return(ret);
}

int PR1000_VEVENT_GetNdStatus(const int fd, const _stPortChSel *pstPortChSel, uint8_t *pStatus)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pStatus == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	/* IRQ Status */
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xC6; //DD|ND
	i2cMask = 0x0F;
	i2cData = 0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	*pStatus = (i2cData>>prChn)&1;

	return(ret);
}

int PR1000_VEVENT_GetDdStatus(const int fd, const _stPortChSel *pstPortChSel, uint8_t *pStatus)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pStatus == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	/* IRQ Status */
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xC6; //DD|ND
	i2cMask = 0xF0;
	i2cData = 0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	*pStatus = ((i2cData>>4)>>prChn)&1;

	return(ret);
}

int PR1000_VEVENT_GetDfdStatus(const int fd, const _stPortChSel *pstPortChSel, uint8_t *pStatus)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	int mapChn; uint8_t prChip, prChn, i2cSlaveAddr; 

	if( (pstPortChSel == NULL) || !ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr))
	{
		ErrorString("Invalid ch.\n");
		return(-1);
	}

	mapChn = pstPortChSel->chn;
	prChip = pstPortChSel->prChip;
	prChn = pstPortChSel->prChn;
	i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

	if(pStatus == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	/* IRQ Status */
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xC7; //NOAUD|DFD
	i2cMask = 0x0F;
	i2cData = 0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	*pStatus = (i2cData>>prChn)&1;

	return(ret);
}

int PR1000_VEVENT_GetAdMuteStatus(const int fd, const uint8_t chip, const uint8_t prChn, uint8_t *pStatus)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	uint8_t i2cSlaveAddr; 

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	if(pStatus == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	/* IRQ Status */
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xC7; //NOAUD|DFD
	i2cMask = 0xF0;
	i2cData = 0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	*pStatus = ((i2cData>>4)>>prChn)&1;

	return(ret);
}

int PR1000_VEVENT_GetAdAbsStatus(const int fd, const uint8_t chip, const uint8_t prChn, uint8_t *pStatus)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	uint8_t i2cSlaveAddr; 

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	if(pStatus == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	/* IRQ Status */
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xC8; //AD_DIFF|AD_ABS
	i2cMask = 0x0F;
	i2cData = 0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	*pStatus = (i2cData>>prChn)&1;

	return(ret);
}

int PR1000_VEVENT_GetAdDiffStatus(const int fd, const uint8_t chip, const uint8_t prChn, uint8_t *pStatus)
{
	int ret = -1;
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	uint8_t page; 
	uint8_t i2cSlaveAddr; 

	i2cSlaveAddr = PR1000_I2C_SLVADDRS[chip];

	if(pStatus == NULL)
	{
		ErrorString("Invalid argu.\n");
		return(-1);
	}

	/* IRQ Status */
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xC8; //AD_DIFF|AD_ABS
	i2cMask = 0xF0;
	i2cData = 0;
	if( (ret = PR1000_ReadMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, &i2cData)) < 0)
	{
		ErrorString("Read reg.\n");
		return(-1);
	}
	*pStatus = ((i2cData>>4)>>prChn)&1;

	return(ret);
}
#endif // DONT_SUPPORT_EVENT_FUNC




//////////////////////////////////////////////////////////////////////////////////////////////////////
