#ifdef __LINUX_SYSTEM__
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <asm/io.h>
#include <linux/proc_fs.h>
#else
#endif

#include <stdint.h>
#include "pr1000_table.h"
#include "pr1000_user_config.h"


/* PR1000 Total Chip. 1~4 */
const uint8_t PR1000_CHIP_COUNT=1; 

/* I2C Slave address of PR1000 Chip. */
const uint8_t PR1000_I2C_SLVADDRS[4] = {
	(0x5C<<1),  	//Master prChn:0~3
	(0x5D<<1),  	//Slave1 prChn:4~7
	(0x5E<<1),  	//Slave2 prChn:8~11
	(0x5F<<1)	//Slave3 prChn:12~15
};

const uint8_t DEFAULT_INIT_FORMAT = pr1000_format_HDA;
const uint8_t DEFAULT_INIT_RESOLUTION = pr1000_outresol_1280x720p25;

/* Select interrupt event mininum msec time. */
const uint32_t PR1000_INT_SYNC_PERIOD=300; //mininum 100msec.

/* Select video out port mux format. */
const uint8_t PR1000_VIDOUTF_MUX_CH=0;  //0:mux_1ch, 1:mux_2ch, 2:mux_4ch
const uint8_t PR1000_VIDOUTF_16BIT=0;  //0:8bit, 1:16bit
const uint8_t PR1000_VIDOUTF_BT656=1;  //0:16bit bt1120, 1:8bit bt656
const uint8_t PR1000_VIDOUTF_RATE=1;  //0:148.5M/297M, 1:148.5M, 2:144M, 3:108M
const uint8_t PR1000_VIDOUTF_RESOL=1;  //0:1080p(720p60), 1:720p, 2:sd960, 3:sd720

/* Select audio & alink port en/disable. */
const uint8_t PR1000_AUDIO_ALINK_EN=1;  //0:disable, 1:enable

//////////////////////////////////////////////////////////////////////////////////////////////
/*
        VD  Output Y/C define value.
	    16bit,  8bit
	0 = VIN1_Y, VIN1
	1 = VIN1_C,
	2 = VIN2_Y, VIN2
	3 = VIN2_C,
	4 = VIN3_Y, VIN3
	5 = VIN3_C,
	6 = VIN4_Y, VIN4
	7 = VIN4_C,
       -1 = Not used port
*/
/* MUX 1CH table. 1port have 1ch*/ /* 8/16BIT-CHIP-VD-PORT-(VIN,CH) */
const int PR1000_VID_INOUT_MUX1CH_CHDEF[2][4][4][4][2] = 
{
	//8bit output(BT656)
	{ /*{{{*/
		{ /* Chip 0 */ //--> Master chip, i2cslave = PR1000_I2C_SLVADDRS[0]
		/* VD out port:    {VINx,Chn},{x,x},{x,x},{x,x} */
		/*         VD1: */ {{ 0, 0},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 2, 1},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD3: */ {{ 4, 2},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{ 6, 3},{-1,-1},{-1,-1},{-1,-1}},
		},
		{ /* Chip 1 */ //--> Slave1 chip, i2cslave = PR1000_I2C_SLVADDRS[1]
		/* VD out port:    {VINx,Chn},{x,x},{x,x},{x,x} */
		/*         VD1: */ {{ 0, 4},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 2, 5},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD3: */ {{ 4, 6},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{ 6, 7},{-1,-1},{-1,-1},{-1,-1}},
		},
		{ /* Chip 2 */ //--> Slave2 chip, i2cslave = PR1000_I2C_SLVADDRS[2]
		/* VD out port:    {VINx,Chn},{x,x},{x,x},{x,x} */
		/*         VD1: */ {{ 0, 8},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 2, 9},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD3: */ {{ 4,10},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{ 6,11},{-1,-1},{-1,-1},{-1,-1}},
		},
		{ /* Chip 3 */ //--> Slave3 chip, i2cslave = PR1000_I2C_SLVADDRS[3]
		/* VD out port:    {VINx,Chn},{x,x},{x,x},{x,x} */
		/*         VD1: */ {{ 0,12},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 2,13},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD3: */ {{ 4,14},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{ 6,15},{-1,-1},{-1,-1},{-1,-1}},
		},
	},/*}}}*/
	//16bit output(BT1120)
	{ /*{{{*/
		{ /* Chip 0 */ //--> Master chip, i2cslave = PR1000_I2C_SLVADDRS[0]
		/* VD out port:    {VINx,Chn},{x,x},{x,x},{x,x} */
		/*         VD1: */ {{ 1,-1},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 0, 0},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD3: */ {{ 5,-1},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{ 4, 1},{-1,-1},{-1,-1},{-1,-1}},
		},
		{ /* Chip 1 */ //--> Slave1 chip, i2cslave = PR1000_I2C_SLVADDRS[1]
		/* VD out port:    {VINx,Chn},{x,x},{x,x},{x,x} */
		/*         VD1: */ {{ 1,-1},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 0, 2},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD3: */ {{ 5,-1},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{ 4, 3},{-1,-1},{-1,-1},{-1,-1}},
		},
		{ /* Chip 2 */ //--> Slave2 chip, i2cslave = PR1000_I2C_SLVADDRS[2]
		/* VD out port:    {VINx,Chn},{x,x},{x,x},{x,x} */
		/*         VD1: */ {{ 1,-1},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 0, 4},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD3: */ {{ 5,-1},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{ 4, 5},{-1,-1},{-1,-1},{-1,-1}},
		},
		{ /* Chip 3 */ //--> Slave3 chip, i2cslave = PR1000_I2C_SLVADDRS[3]
		/* VD out port:    {VINx,Chn},{x,x},{x,x},{x,x} */
		/*         VD1: */ {{ 1,-1},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 0, 6},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD3: */ {{ 5,-1},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{ 4, 7},{-1,-1},{-1,-1},{-1,-1}},
		},
	}/*}}}*/
};

/* MUX 2CH table. 1port have 2ch*/ /* 8/16BIT-CHIP-VD-PORT-(VIN,CH) */
const int PR1000_VID_INOUT_MUX2CH_CHDEF[2][4][4][4][2] = 
{
	//8bit output(BT656)
	{ /*{{{*/
		{ /* Chip 0 */ //--> Master chip, i2cslave = PR1000_I2C_SLVADDRS[0]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{x,x},{x,x} */
		/*         VD1: */ {{ 6, 0},{ 4, 1},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 2, 2},{ 0, 3},{-1,-1},{-1,-1}},
		/*         VD3: */ {{-1,-1},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{-1,-1},{-1,-1},{-1,-1},{-1,-1}},
		},
		{ /* Chip 1 */ //--> Slave1 chip, i2cslave = PR1000_I2C_SLVADDRS[1]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{x,x},{x,x} */
		/*         VD1: */ {{ 6, 4},{ 4, 5},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 2, 6},{ 0, 7},{-1,-1},{-1,-1}},
		/*         VD3: */ {{-1,-1},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{-1,-1},{-1,-1},{-1,-1},{-1,-1}},
		},
		{ /* Chip 2 */ //--> Slave2 chip, i2cslave = PR1000_I2C_SLVADDRS[2]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{x,x},{x,x} */
		/*         VD1: */ {{ 6, 8},{ 4, 9},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 2,10},{ 0,11},{-1,-1},{-1,-1}},
		/*         VD3: */ {{-1,-1},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{-1,-1},{-1,-1},{-1,-1},{-1,-1}},
		},
		{ /* Chip 3 */ //--> Slave3 chip, i2cslave = PR1000_I2C_SLVADDRS[3]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{x,x},{x,x} */
		/*         VD1: */ {{ 6,12},{ 4,13},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 2,14},{ 0,15},{-1,-1},{-1,-1}},
		/*         VD3: */ {{-1,-1},{-1,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{-1,-1},{-1,-1},{-1,-1},{-1,-1}},
		},
	},/*}}}*/
	//16bit output(BT1120)
	{ /*{{{*/
		{ /* Chip 0 */ //--> Master chip, i2cslave = PR1000_I2C_SLVADDRS[0]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{x,x},{x,x} */
		/*         VD1: */ {{ 1,-1},{ 3,-1},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 0, 0},{ 2, 1},{-1,-1},{-1,-1}},
		/*         VD3: */ {{ 5,-1},{ 7,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{ 4, 2},{ 6, 3},{-1,-1},{-1,-1}},
		},
		{ /* Chip 1 */ //--> Slave1 chip, i2cslave = PR1000_I2C_SLVADDRS[1]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{x,x},{x,x} */
		/*         VD1: */ {{ 1,-1},{ 3,-1},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 0, 4},{ 2, 5},{-1,-1},{-1,-1}},
		/*         VD3: */ {{ 5,-1},{ 7,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{ 4, 6},{ 6, 7},{-1,-1},{-1,-1}},
		},
		{ /* Chip 2 */ //--> Slave2 chip, i2cslave = PR1000_I2C_SLVADDRS[2]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{x,x},{x,x} */
		/*         VD1: */ {{ 1,-1},{ 3,-1},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 0, 8},{ 2, 9},{-1,-1},{-1,-1}},
		/*         VD3: */ {{ 5,-1},{ 7,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{ 4,10},{ 6,11},{-1,-1},{-1,-1}},
		},
		{ /* Chip 3 */ //--> Slave3 chip, i2cslave = PR1000_I2C_SLVADDRS[3]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{x,x},{x,x} */
		/*         VD1: */ {{ 1,-1},{ 3,-1},{-1,-1},{-1,-1}},
		/*         VD2: */ {{ 0,12},{ 2,13},{-1,-1},{-1,-1}},
		/*         VD3: */ {{ 5,-1},{ 7,-1},{-1,-1},{-1,-1}},
		/*         VD4: */ {{ 4,14},{ 6,15},{-1,-1},{-1,-1}},
		},
	}/*}}}*/
};


/* MUX 4CH table. 1port have 4ch*/ /* 8/16BIT-CHIP-VD-PORT-(VIN,CH) */
/* ==> Don't support 1080p_4mux. 297Mhz, 4channel 720p. 1port have 4ch. */
const int PR1000_VID_INOUT_MUX4CH_CHDEF[2][4][4][4][2] = 
{
	//8bit output(BT656)
	{ /*{{{*/
		{ /* Chip 0 */ //--> Master chip, i2cslave = PR1000_I2C_SLVADDRS[0]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{VINx,Chn},{VINx,Chn} */
		/*         VD1: */ {{ 0,  0},{ 2,  1},{ 4,  2},{ 6,  3}},
		/*         VD2: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		/*         VD3: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		/*         VD4: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		},
		{ /* Chip 1 */ //--> Slave1 chip, i2cslave = PR1000_I2C_SLVADDRS[1]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{VINx,Chn},{VINx,Chn} */
		/*         VD1: */ {{ 0,  4},{ 2,  5},{ 4,  6},{ 6,  7}},
		/*         VD2: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		/*         VD3: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		/*         VD4: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		},
		{ /* Chip 2 */ //--> Slave2 chip, i2cslave = PR1000_I2C_SLVADDRS[2]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{VINx,Chn},{VINx,Chn} */
		/*         VD1: */ {{ 0,  8},{ 2,  9},{ 4, 10},{ 6, 11}},
		/*         VD2: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		/*         VD3: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		/*         VD4: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		},
		{ /* Chip 3 */ //--> Slave3 chip, i2cslave = PR1000_I2C_SLVADDRS[3]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{VINx,Chn},{VINx,Chn} */
		/*         VD1: */ {{ 0, 12},{ 2, 13},{ 4, 14},{ 6, 15}},
		/*         VD2: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		/*         VD3: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		/*         VD4: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		},
	},/*}}}*/
	//16bit output(BT1120)
	{ /*{{{*/
		{ /* Chip 0 */ //--> Master chip, i2cslave = PR1000_I2C_SLVADDRS[0]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{VINx,Chn},{VINx,Chn} */
		/*         VD1: */ {{ 1, -1},{ 3, -1},{ 5, -1},{ 7, -1}},
		/*         VD2: */ {{ 0,  0},{ 2,  1},{ 4,  2},{ 6,  3}},
		/*         VD3: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		/*         VD4: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		},
		{ /* Chip 1 */ //--> Slave1 chip, i2cslave = PR1000_I2C_SLVADDRS[1]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{VINx,Chn},{VINx,Chn} */
		/*         VD1: */ {{ 1, -1},{ 3, -1},{ 5, -1},{ 7, -1}},
		/*         VD2: */ {{ 0,  4},{ 2,  5},{ 4,  6},{ 6,  7}},
		/*         VD3: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		/*         VD4: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		},
		{ /* Chip 2 */ //--> Slave2 chip, i2cslave = PR1000_I2C_SLVADDRS[2]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{VINx,Chn},{VINx,Chn} */
		/*         VD1: */ {{ 1, -1},{ 3, -1},{ 5, -1},{ 7, -1}},
		/*         VD2: */ {{ 0,  8},{ 2,  9},{ 4, 10},{ 6, 11}},
		/*         VD3: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		/*         VD4: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		},
		{ /* Chip 3 */ //--> Slave3 chip, i2cslave = PR1000_I2C_SLVADDRS[3]
		/* VD out port:    {VINx,Chn},{VINx,Chn},{VINx,Chn},{VINx,Chn} */
		/*         VD1: */ {{ 1, -1},{ 3, -1},{ 5, -1},{ 7, -1}},
		/*         VD2: */ {{ 0, 12},{ 2, 13},{ 4, 14},{ 6, 15}},
		/*         VD3: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		/*         VD4: */ {{-1, -1},{-1, -1},{-1, -1},{-1, -1}},
		},
	}/*}}}*/
};
//////////////////////////////////////////////////////////////////////////////////////////////

/* VD outport clkphase control. Value[0x8~0xF]. reference register 0x0xE8,0x0xE9 */
const int PR1000_VIDOUTF_CLKPHASE[4][4] = 
{
	/* VD1, VD2, VD3, VD4 */
	{  0x9, 0x9, 0x9, 0x9 }, /* Chip 0 */ //--> Master chip, i2cslave = PR1000_I2C_SLVADDRS[0]
	{  0x9, 0x9, 0x9, 0x9 }, /* Chip 1 */ //--> Slave1 chip, i2cslave = PR1000_I2C_SLVADDRS[1]
	{  0x9, 0x9, 0x9, 0x9 }, /* Chip 2 */ //--> Slave2 chip, i2cslave = PR1000_I2C_SLVADDRS[2]
	{  0x9, 0x9, 0x9, 0x9 }, /* Chip 3 */ //--> Slave3 chip, i2cslave = PR1000_I2C_SLVADDRS[3]
};

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*** PR1000 register map - VIDEO Out format control. ***/
// 0xFF:not support
const _PR1000_REG_TABLE_VIDOUT_FORMAT pr1000_reg_table_vidout_format[MAX_PR1000_VID_OUTF_MUX_TYPE][2][MAX_PR1000_VID_OUTF_RESOL_TYPE] = 
{/*{{{*/
	//PR1000_VID_OUTF_MUX_1CH,
	{
		//8bit,
		{/*{{{*/
			//HD1080P: vdck_out_phase,
			{                     0x0, },
			// HD720P: vdck_out_phase,
			{                     0xC, },
			// SD960H: vdck_out_phase,
			{                     0x8, },
			// SD720H: vdck_out_phase,
			{                     0x8, },
		},/*}}}*/
		//16bit,
		{/*{{{*/
			//HD1080P: vdck_out_phase,
			{                     0xC, },
			// HD720P: vdck_out_phase,
			{                     0x8, },
			// SD960H: vdck_out_phase,
			{                     0x8, }, // forcely 36Mhz
			// SD720H: vdck_out_phase,
			{                     0x8, }, // forcely 27Mhz
		},/*}}}*/
	},
	//PR1000_VID_OUTF_MUX_2CH,
	{
		//8bit,
		{/*{{{*/
			//HD1080P: vdck_out_phase,
			{                     0x0, },
			// HD720P: vdck_out_phase,
			{                     0x0, },
			// SD960H: vdck_out_phase,
			{                     0xC, },
			// SD720H: vdck_out_phase,
			{                     0xC, },
		},/*}}}*/
		//16bit,
		{/*{{{*/
			//HD1080P: vdck_out_phase,
			{                     0x0, },
			// HD720P: vdck_out_phase,
			{                     0xC, },
			// SD960H: vdck_out_phase,
			{                     0x8, },
			// SD720H: vdck_out_phase,
			{                     0x8, },
		},/*}}}*/
	},
	//PR1000_VID_OUTF_MUX_4CH,
	{
		//8bit,
		{/*{{{*/
			//HD1080P: vdck_out_phase,
			{                    0xFF, }, // not support
			// HD720P: vdck_out_phase,
			{                     0x0, },
			// SD960H: vdck_out_phase,
			{                     0x0, },
			// SD720H: vdck_out_phase,
			{                     0x0, },
		},/*}}}*/
		//16bit,
		{/*{{{*/
			//HD1080P: vdck_out_phase,
			{                     0x0, },
			// HD720P: vdck_out_phase,
			{                     0x0, },
			// SD960H: vdck_out_phase,
			{                     0xC, },
			// SD720H: vdck_out_phase,
			{                     0xC, },
		},/*}}}*/
	},
};/*}}}*/

//////////////////////////////////////////////////////////////////////////////////////////////////////
//#define PR1000_IRQ_CPU_EXTERNAL  //If use cpu external irq, define
//#define PR1000_IRQ_CASCADE	//If pr1000 irq cascade, define.

int SetCPUExternalInterrupt(void)
{
#ifdef PR1000_IRQ_CPU_EXTERNAL  //If use cpu external irq, define
	/* gpio15_02 -> set external interrupt */
#define MUXCTL_PHY_ADDR        		IO_ADDRESS(0x120F0000)           
#define MUXCTL_REG			(MUXCTL_PHY_ADDR + 0x02FC) // 0x2FC[muxctrl_reg191]: sata_led_n1/gpio15_2
#define GPIO_PHY_ADDR            	IO_ADDRESS(0x12240000)
#define GPIOREG				(15) //gpio15_0
#define GPIONUM				(2)  //gpio15_2
#define GPIO_INT_NUM 			(92)	//gpio15 irq number. refer document.

#define GPIO_DATA                     	(GPIO_PHY_ADDR + 0x03FC) 
#define GPIO_DIR                      	(GPIO_PHY_ADDR + 0x0400)
#define GPIO_IS                       	(GPIO_PHY_ADDR + 0x0404)
#define GPIO_IBE                      	(GPIO_PHY_ADDR + 0x0408)
#define GPIO_IEV                      	(GPIO_PHY_ADDR + 0x040C)
#define GPIO_IE                       	(GPIO_PHY_ADDR + 0x0410)
#define GPIO_RIS                      	(GPIO_PHY_ADDR + 0x0414)
#define GPIO_MIS                      	(GPIO_PHY_ADDR + 0x0418)
#define GPIO_IC                       	(GPIO_PHY_ADDR + 0x041C)

	/* Use cpu gpio interrupt handler. */

	__u32 reg = 0;
	int intNum = GPIO_INT_NUM;

	/* gpio mode */
#ifdef MUXCTL_REG
	reg = 0;
	writeb( reg, (volatile void __iomem *)MUXCTL_REG);     
#endif // MUXCTL_REG

	printk("Init: set cpu gpio%d_%d external interrupt. \n", GPIOREG, GPIONUM); //gpio15_2 --> Chip0(Master)
	{/*{{{*/
		/* input mode */
		reg = readb((volatile void __iomem *)GPIO_DIR);
		reg &= (__u32)~(0x1<<(GPIONUM)); /* input mode */
		writeb( reg, (volatile void __iomem *)GPIO_DIR);  /* set input mode */

		/* level sensitive mode */
		reg = readb((volatile void __iomem *)GPIO_IS);
		reg |= (__u32)(0x1<<(GPIONUM)); /* level sensitive mode */
		writeb( reg, (volatile void __iomem *)GPIO_IS);  /* set level sensitive mode */

		/* low-level sensitive mode */
		reg = readb((volatile void __iomem *)GPIO_IEV);
		reg &= (__u32)~(0x1<<(GPIONUM)); /* low-level sensitive mode */
		writeb( reg, (volatile void __iomem *)GPIO_IEV);  /* set low-level sensitive mode */

		/* clear interrupt */
		reg = readb((volatile void __iomem *)GPIO_IC);
		reg |= (__u32)(0x1<<(GPIONUM)); 
		writeb( reg, (volatile void __iomem *)GPIO_IC);  

		/* enable interrupt */
		reg = readb((volatile void __iomem *)GPIO_IE);
		reg |= (__u32)(0x1<<(GPIONUM)); 
		writeb( reg, (volatile void __iomem *)GPIO_IE);  
	}/*}}}*/

	return(intNum);
#else
	/* Don't use cpu gpio interrupt handler. thread polling check. */
	return(0);
#endif
}

int GetCPUExternalIrqChipState(uint8_t *pstIrqChipState)
{
	int ret = 0;

#ifdef PR1000_IRQ_CPU_EXTERNAL  //If use cpu external irq, define
	uint8_t stIrqChipState = 0;
	__u32 reg = 0;
	__u32 clrReg = 0;

	/* ris mode */
	reg = readb((volatile void __iomem *)GPIO_RIS);
	if(reg == 0) 
	{
		return(0);
	}
	else
	{
		/* clear cpu interrupt */
		{
			if(reg & (1<<GPIONUM) )//gpio15_2 --> Chip0(Master)
			{
				clrReg |= (1<<GPIONUM);
			}
			writeb( clrReg, (volatile void __iomem *)GPIO_IC);  /* clear interrupt*/
		}

#ifdef PR1000_IRQ_CASCADE
		if(reg & (1<<GPIONUM) )//gpio15_2 --> Chip0(Master)
		{
			stIrqChipState |= (1<<3);
			ret = 0; //cascade return 0.
		}
#else
		if(reg & (1<<GPIONUM) )//gpio15_2 --> Chip0(Master)
		{
			stIrqChipState |= (1<<3);
			ret++;
		}
#endif // PR1000_IRQ_CASCADE
	}
	*pstIrqChipState = stIrqChipState;
#else
	/* Don't use cpu gpio interrupt handler. thread polling check. */
	return(0);
#endif // PR1000_IRQ_CPU_EXTERNAL  //If use cpu external irq, define

	return(ret);
}


int SetOutChn(int chn)
{
	int ret = 0;
	//int portA=-1, portB=-1, portC=-1, portD=-1;
	int portA=4, portB=-1, portC=1, portD=-1;
	_stOutChn stOutChn;

	memset(&stOutChn, 0, sizeof(_stOutChn));
	stOutChn.prChip = 0;
	stOutChn.prChn = chn;
	stOutChn.portChSel[0] = portA;
	stOutChn.portChSel[1] = portB;
	stOutChn.portChSel[2] = portC;
	stOutChn.portChSel[3] = portD;

	ret= PR1000_VID_SetOutChn(0, stOutChn.prChip, &stOutChn);
	return(ret);
}


