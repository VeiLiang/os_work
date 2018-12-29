//****************************************************************************
//
//	Copyright (C) 2012~2016 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_i2c.c
//	  i2c master device driver
//
//	Revision history
//
//		2015.12.02	ZhuoYongHong Initial version
//	
//	PIO及DMA模式均已使用PS1210K验证中断/DMA OK
// 	测试FPGA版本  20150511_arkn141_isp_sd_dma_uart1_timing_ok
//
//****************************************************************************
#include "hardware.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xm_i2c.h"
#include "rtos.h"
#include "xm_queue.h"
#include "xm_dev.h"
#include "xm_core.h"
#include "dma.h"


#if _XMSYS_I2C_ == _XMSYS_I2C_HARDWARE_

#define	I2C_WRITE_METHOD_PIO		1
#define	I2C_WRITE_METHOD_DMA		2
//#define	I2C_WRITE_METHOD			I2C_WRITE_METHOD_DMA		// 已使用PS1210K测试, OK
#define	I2C_WRITE_METHOD			I2C_WRITE_METHOD_PIO

#define	I2C_READ_METHOD_PIO		1
#define	I2C_READ_METHOD_DMA		2
//#define	I2C_READ_METHOD			I2C_READ_METHOD_DMA		// 已使用PS1210K测试, OK
#define	I2C_READ_METHOD			I2C_READ_METHOD_PIO

/*
 * Registers offset
 */
#define DW_IC_CON					0x0
#define DW_IC_TAR					0x4
#define DW_IC_DATA_CMD			0x10
#define DW_IC_SS_SCL_HCNT		0x14
#define DW_IC_SS_SCL_LCNT		0x18
#define DW_IC_FS_SCL_HCNT		0x1c
#define DW_IC_FS_SCL_LCNT		0x20
#define DW_IC_INTR_STAT			0x2c
#define DW_IC_INTR_MASK			0x30
#define DW_IC_RAW_INTR_STAT	0x34
#define DW_IC_RX_TL				0x38
#define DW_IC_TX_TL				0x3c
#define DW_IC_CLR_INTR			0x40
#define DW_IC_CLR_RX_UNDER		0x44
#define DW_IC_CLR_RX_OVER		0x48
#define DW_IC_CLR_TX_OVER		0x4c
#define DW_IC_CLR_RD_REQ		0x50
#define DW_IC_CLR_TX_ABRT		0x54
#define DW_IC_CLR_RX_DONE		0x58
#define DW_IC_CLR_ACTIVITY		0x5c
#define DW_IC_CLR_STOP_DET		0x60
#define DW_IC_CLR_START_DET	0x64
#define DW_IC_CLR_GEN_CALL		0x68
#define DW_IC_ENABLE				0x6c
#define DW_IC_STATUS				0x70
#define DW_IC_TXFLR				0x74
#define DW_IC_RXFLR				0x78
#define DW_IC_TX_ABRT_SOURCE	0x80
#define DW_IC_DMA_CR				0x88
#define DW_IC_DMA_TDLR			0x8c
#define DW_IC_DMA_RDLR			0x90
#define DW_IC_DENOISE			0xa0
#define DW_IC_COMP_PARAM_1		0xf4

#define DW_IC_CON_MASTER				0x1
#define DW_IC_CON_SPEED_STD			0x2
#define DW_IC_CON_SPEED_FAST			0x4
#define DW_IC_CON_10BITADDR_MASTER	0x10
#define DW_IC_CON_RESTART_EN			0x20
#define DW_IC_CON_SLAVE_DISABLE		0x40

#define DW_IC_INTR_RX_UNDER	0x001
#define DW_IC_INTR_RX_OVER		0x002
#define DW_IC_INTR_RX_FULL		0x004
#define DW_IC_INTR_TX_OVER		0x008
#define DW_IC_INTR_TX_EMPTY	0x010
#define DW_IC_INTR_RD_REQ		0x020
#define DW_IC_INTR_TX_ABRT		0x040
#define DW_IC_INTR_RX_DONE		0x080
#define DW_IC_INTR_ACTIVITY	0x100
#define DW_IC_INTR_STOP_DET	0x200
#define DW_IC_INTR_START_DET	0x400
#define DW_IC_INTR_GEN_CALL	0x800

#define DW_IC_INTR_DEFAULT_MASK		(DW_IC_INTR_RX_FULL | \
						DW_IC_INTR_TX_EMPTY | \
						DW_IC_INTR_TX_ABRT | \
						DW_IC_INTR_STOP_DET)

#define DW_IC_STATUS_ACTIVITY		0x1

#define DW_IC_ERR_TX_ABRT			0x1

/*I2C dma tx*/

#define I2C_WRITE_DMA_CTL_L 		((0<<28)\
									|(0<<27)\
									|(0<<25)\
									|(0<<23)\
									|(M2P_DMAC<<20)\
									|(0<<18)\
									|(0<<17)\
									|(DMA_BURST_SIZE_4<<14)\
									|(DMA_BURST_SIZE_4<<11)\
									|(0<<9)\
									|(2<<7)\
									|(DMA_WIDTH_8<<4)\
									|(DMA_WIDTH_8<<1)\
									|(1<<0))

#define I2C_WRITE_DMA_CTL_H 		(0<<12)

#define I2C_WRITE_DMA_CFG_L		((0<<31)\
									|(0<<30)\
									|(0<<17)\
									|(0<<8))

#define I2C_WRITE_DMA_CFG_H		((REQ_I2C_TX<<12)\
									|(0<<7)\
									|(1<<6)\
									|(1<<5)\
									|(1<<1))

#define I2C_READ_DMA_CTL_L 			((0<<28)\
									|(0<<27)\
									|(0<<25)\
									|(0<<23)\
									|(P2M_DMAC<<20)\
									|(0<<18)\
									|(0<<17)\
									|(DMA_BURST_SIZE_1<<14) /* SRC_MSIZE */ \
									|(DMA_BURST_SIZE_1<<11) /* DEST_MSIZE */ \
									|(2<<9)\
									|(0<<7)\
									|(DMA_WIDTH_8<<4) /* SRC_TR_WIDTH */ \
									|(DMA_WIDTH_8<<1) /* DST_TR_WIDTH */ \
									|(1<<0))

#define I2C_READ_DMA_CTL_H 		(0<<12)


#define I2C_READ_DMA_CFG_L		((0<<31)\
								|(0<<30)\
								|(0<<17)\
								|(0<<8))


#define I2C_READ_DMA_CFG_H		((0<<12)\
								|(REQ_I2C_RX<<7)\
								|(1<<6)\
								|(1<<5)\
								|(1<<1))
/*
 * status codes
 */
#define STATUS_IDLE					0x0
#define STATUS_WRITE_IN_PROGRESS	0x1
#define STATUS_READ_IN_PROGRESS	0x2

#define TIMEOUT			20 /* ms */

/*
 * hardware abort codes from the DW_IC_TX_ABRT_SOURCE register
 *
 * only expected abort codes are listed here
 * refer to the datasheet for the full list
 */
#define ABRT_7B_ADDR_NOACK		0
#define ABRT_10ADDR1_NOACK		1
#define ABRT_10ADDR2_NOACK		2
#define ABRT_TXDATA_NOACK		3
#define ABRT_GCALL_NOACK		4
#define ABRT_GCALL_READ			5
#define ABRT_HS_ACKDET			6
#define ABRT_SBYTE_ACKDET		7
#define ABRT_HS_NORSTRT			8
#define ABRT_SBYTE_NORSTRT		9
#define ABRT_MASTER_DIS			11
#define ARB_LOST					12		// master lost arbitration

#define DW_IC_TX_ABRT_7B_ADDR_NOACK		(1UL << ABRT_7B_ADDR_NOACK)
#define DW_IC_TX_ABRT_10ADDR1_NOACK		(1UL << ABRT_10ADDR1_NOACK)
#define DW_IC_TX_ABRT_10ADDR2_NOACK		(1UL << ABRT_10ADDR2_NOACK)
#define DW_IC_TX_ABRT_TXDATA_NOACK		(1UL << ABRT_TXDATA_NOACK)
#define DW_IC_TX_ABRT_GCALL_NOACK		(1UL << ABRT_GCALL_NOACK)
#define DW_IC_TX_ABRT_GCALL_READ			(1UL << ABRT_GCALL_READ)
#define DW_IC_TX_ABRT_SBYTE_ACKDET		(1UL << ABRT_SBYTE_ACKDET)
#define DW_IC_TX_ABRT_SBYTE_NORSTRT		(1UL << ABRT_SBYTE_NORSTRT)
#define DW_IC_TX_ABRT_10B_RD_NORSTRT	(1UL << ABRT_10B_RD_NORSTRT)
#define DW_IC_TX_ABRT_MASTER_DIS			(1UL << ABRT_MASTER_DIS)
#define DW_IC_TX_ARB_LOST					(1UL << ARB_LOST)

#define DW_IC_TX_ABRT_NOACK		(DW_IC_TX_ABRT_7B_ADDR_NOACK | \
					 DW_IC_TX_ABRT_10ADDR1_NOACK | \
					 DW_IC_TX_ABRT_10ADDR2_NOACK | \
					 DW_IC_TX_ABRT_TXDATA_NOACK | \
					 DW_IC_TX_ABRT_GCALL_NOACK)

#define	writel(data,addr)		*((volatile u32 *)(addr)) = (data)
#define	readl(addr)				*((volatile u32 *)(addr))

#define	mutex_lock(lock)				OS_Use(lock)
#define	mutex_unlock(lock)			OS_Unuse(lock)

#define	likely(exp)						(!!(exp))
#define	unlikely(exp)					(!!(exp))

#define	INIT_COMPLETION(event_complete)	OS_EVENT_Reset(&event_complete)
#define	complete(event_complete)			OS_EVENT_Set(event_complete)
#define	wait_for_completion_interruptible_timeout(event_complete,timeout)		!OS_EVENT_WaitTimed(event_complete,timeout) 

#define	XMI2C_ID_FLAG				0x20433249		// "I2C "
#define	XMI2C_SLAVE_ID_FLAG		0x53433249		// "I2CS"

#define	I2C_CLK 				30000000 		//	30M


#define	EIO		 	5	/* I/O error */

#define	EAGAIN		11	/* Try again */
#define	EINVAL		22	/* Invalid argument */

#define	ETIMEDOUT	110	/* Connection timed out */

#define	EREMOTEIO	121	/* Remote I/O error */

#if 0
#define	dev_dbg		XM_printf
#define	dev_err		XM_printf
#define	dev_warn		XM_printf
#else
#define	dev_dbg(...)		
#define	dev_err(...)
#define	dev_warn(...)
#endif

static char *abort_sources[] = {
	/* 0 */	"Master is in 7-bit addressing mode and the address sent was not acknowledged by any slave",
	/* 1 */	"Master is in 10-bit address mode and the first 10-bit address byte was not acknowledged by any slave.",
	/* 2 */	"Master is in 10-bit address mode and the second address byte of the 10-bit address was not acknowledged by any slave.",
	/* 3 */	"Master has received an acknowledgement for the address, but when it sent data byte(s) following the address, it did not receive an acknowledge from the remote slave(s).",
	/* 4 */	"in master mode sent a General Call and no slave on the bus acknowledged the General Call.",
	/* 5 */	"in master mode sent a General Call but the user programmed the byte following the General Call tobe a read from the bus",
	/* 6 */	"Master is in High Speed mode and the High Speed Master code was acknowledged (wrong behavior).",
	/* 7 */	"Master has sent a START Byte and the START Byte was acknowledged (wrong behavior).",
	/* 8 */	"The restart is disabled (IC_RESTART_EN bit (IC_CON[5]) = 0) and the user is trying to use the master to transfer data in High Speed mode",
	/* 9 */	"The restart is disabled (IC_RESTART_EN bit (IC_CON[5]) = 0) and the user is trying to send a START Byte.",
	/*10 */	"",
	/*11 */	"User tries to initiate a Master operation with the Master mode disabled.",
	/*12 */	"Master has lost arbitration"
};

struct i2c_msg {
	__u16 addr;	/* slave address			*/
	__u16 flags;
#define I2C_M_TEN		0x0010	/* this is a ten bit chip address */
#define I2C_M_RD		0x0001	/* read data, from slave to master */
#define I2C_M_NOSTART		0x4000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_REV_DIR_ADDR	0x2000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_IGNORE_NAK	0x1000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NO_RD_ACK		0x0800	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_RECV_LEN		0x0400	/* length will be first received byte */
	__u16 len;		/* msg length				*/
	__u8 *buf;		/* pointer to msg data			*/
};

/**
 * struct dw_i2c_dev - private i2c-designware data
 * @dev: driver model device node
 * @base: IO registers pointer
 * @cmd_complete: tx completion indicator
 * @lock: protect this struct and IO registers
 * @clk: input reference clock
 * @cmd_err: run time hadware error code
 * @msgs: points to an array of messages currently being transfered
 * @msgs_num: the number of elements in msgs
 * @msg_write_idx: the element index of the current tx message in the msgs
 *	array
 * @tx_buf_len: the length of the current tx buffer
 * @tx_buf: the current tx buffer
 * @msg_read_idx: the element index of the current rx message in the msgs
 *	array
 * @rx_buf_len: the length of the current rx buffer
 * @rx_buf: the current rx buffer
 * @msg_err: error status of the current transfer
 * @status: i2c master status, one of STATUS_*
 * @abort_source: copy of the TX_ABRT_SOURCE register
 * @irq: interrupt number for the i2c master
 * @adapter: i2c subsystem adapter node
 * @tx_fifo_depth: depth of the hardware tx fifo
 * @rx_fifo_depth: depth of the hardware rx fifo
 */
typedef struct dw_i2c_dev {
	
	u32_t					i2c_id;				// I2C设备的标志字段
	u8_t					dev_id;				// 设备号
	int					irq;					// 中断号
	
	int					ref_count;			// i2c设备引用的次数, 即该i2c设备上挂接的client设备个数

//	struct device		*dev;
	volatile char		*base;
	struct OS_EVENT	cmd_complete;
	struct OS_RSEMA	lock;					// 设备互斥访问保护
//	struct clk			*clk;
	int					cmd_err;
	struct i2c_msg		*msgs;
	int					msgs_num;
	int					msg_write_idx;
	u32					tx_buf_len;
	u8						*tx_buf;
	int					msg_read_idx;
	u32					rx_buf_len;
	u8						*rx_buf;
	int					msg_err;
	unsigned int		status;
	u32					abort_source;
	//struct i2c_adapter	adapter;
	unsigned int		tx_fifo_depth;
	unsigned int		rx_fifo_depth;
} xm_i2c_dev;

#define	MAX_I2C_NAME	32
// I2C从设备定义
typedef struct _tag_i2c_client {
	queue_s		link;					// 从设备的双向链表
	u32_t			i2c_slave_id;		// I2C从设备的标志字段
	
	OS_RSEMA		access_sema;		// 互斥访问保护
	
	u16_t			addr;					// slave address
	u16_t			flags;				// addressing bit
	char			name[MAX_I2C_NAME];			// slave's name
	
	u16_t			dev_id;				// i2c设备ID
	xm_i2c_dev *driver;				// I2c设备句柄	
} xm_i2c_client;

// I2C设备基址定义
static const u32_t i2c_base[XM_I2C_DEV_COUNT] = {
	IIC_BASE
};

static const u32_t i2c_irq[XM_I2C_DEV_COUNT] = {
	I2C_INT
};


static xm_i2c_dev * i2c_device[XM_I2C_DEV_COUNT];		// 设备句柄数组
static OS_RSEMA i2c_access_sema[XM_I2C_DEV_COUNT];		// 并发访问保护信号量

static queue_s	 i2c_client_head;					// 从设备双向链表队首结构
static OS_RSEMA i2c_client_access_sema;		// 从设备访问信号量

static void i2c_delay (UINT32 count)
{
	while(count--);
}

// 将I2C设备对应的PAD脚配置为I2C功能脚
static void select_i2c_pad (u8_t i2c_dev)
{
	UINT32 val;
	if(i2c_dev == XM_I2C_DEV_0)
	{
		// 2014-12-09 之后的新pad
		XM_lock ();
		val = rSYS_PAD_CTRL0A ;
		val &= 0xfffffff0;
		//    GPIO116  GPIO117
		//       SCK   SDA
		val |= (1<<0)|(1<<2);
		rSYS_PAD_CTRL0A = val ;
		XM_unlock ();
	}
}

// 将I2C设备对应的PAD脚配置为输入上拉的GPIO
static void unselect_i2c_pad (u8_t i2c_dev)
{
	UINT32 val;
	if(i2c_dev == XM_I2C_DEV_0)
	{
		// 2014-12-09 之后的新pad
		val = rSYS_PAD_CTRL0A ;
		val &= 0xfffffff0;
		rSYS_PAD_CTRL0A = val ;
	}
}

// i2c IP reset
static void i2c_reset (u8_t i2c_dev)
{
	if(i2c_dev == XM_I2C_DEV_0)
	{
		rSYS_SOFT_RSTNA &= ~(1<<11);
		i2c_delay (100);
		rSYS_SOFT_RSTNA |=  (1<<11);
	}
}

static void i2c_clock_enable (u8_t i2c_dev)
{
	
}

static void i2c_clock_disable (u8_t i2c_dev)
{
	
}

static u32 i2c_dw_scl_hcnt(u32 ic_clk, u32 tSYMBOL, u32 tf, int cond, int offset)
{
	/*
	 * DesignWare I2C core doesn't seem to have solid strategy to meet
	 * the tHD;STA timing spec.  Configuring _HCNT based on tHIGH spec
	 * will result in violation of the tHD;STA spec.
	 */
	if (cond)
		/*
		 * Conditional expression:
		 *
		 *   IC_[FS]S_SCL_HCNT + (1+4+3) >= IC_CLK * tHIGH
		 *
		 * This is based on the DW manuals, and represents an ideal
		 * configuration.  The resulting I2C bus speed will be
		 * faster than any of the others.
		 *
		 * If your hardware is free from tHD;STA issue, try this one.
		 */
		return (ic_clk * tSYMBOL + 5000) / 10000 - 8 + offset;
	else
		/*
		 * Conditional expression:
		 *
		 *   IC_[FS]S_SCL_HCNT + 3 >= IC_CLK * (tHD;STA + tf)
		 *
		 * This is just experimental rule; the tHD;STA period turned
		 * out to be proportinal to (_HCNT + 3).  With this setting,
		 * we could meet both tHIGH and tHD;STA timing specs.
		 *
		 * If unsure, you'd better to take this alternative.
		 *
		 * The reason why we need to take into account "tf" here,
		 * is the same as described in i2c_dw_scl_lcnt().
		 */
		return (ic_clk * (tSYMBOL + tf) + 5000) / 10000 - 3 + offset;
}

static u32 i2c_dw_scl_lcnt(u32 ic_clk, u32 tLOW, u32 tf, int offset)
{
	/*
	 * Conditional expression:
	 *
	 *   IC_[FS]S_SCL_LCNT + 1 >= IC_CLK * (tLOW + tf)
	 *
	 * DW I2C core starts counting the SCL CNTs for the LOW period
	 * of the SCL clock (tLOW) as soon as it pulls the SCL line.
	 * In order to meet the tLOW timing spec, we need to take into
	 * account the fall time of SCL signal (tf).  Default tf value
	 * should be 0.3 us, for safety.
	 */
	return ((ic_clk * (tLOW + tf) + 5000) / 10000) - 1 + offset;
}

/**
 * i2c_dw_init() - initialize the designware i2c master hardware
 * @dev: device private data
 *
 * This functions configures and enables the I2C master.
 * This function is called during I2C init function, and in case of timeout at
 * run time.
 */
static void i2c_dw_init(struct dw_i2c_dev *dev)
{
	u32 input_clock_khz = I2C_CLK / 1000;
	u32 ic_con, hcnt, lcnt;

#ifdef CONFIG_ARKN141_ASIC
	input_clock_khz = GetAPB_CLK ()/1000;
#endif
	/* Disable the adapter */
	writel(0, dev->base + DW_IC_ENABLE);

	/* set standard and fast speed deviders for high/low periods */

	/* Standard-mode */
	hcnt = i2c_dw_scl_hcnt(input_clock_khz,
				40,	/* tHD;STA = tHIGH = 4.0 us */
				3,	/* tf = 0.3 us */
				0,	/* 0: DW default, 1: Ideal */
				0);	/* No offset */
	lcnt = i2c_dw_scl_lcnt(input_clock_khz,
				47,	/* tLOW = 4.7 us */
				3,	/* tf = 0.3 us */
				0);	/* No offset */
	
	//hcnt = 0x00000190;
	//lcnt = 0x000001d6;
			
	writel(hcnt, dev->base + DW_IC_SS_SCL_HCNT);
	writel(lcnt, dev->base + DW_IC_SS_SCL_LCNT);
	dev_dbg("Standard-mode HCNT:LCNT = %d:%d\n", hcnt, lcnt);

	/* Fast-mode */
	hcnt = i2c_dw_scl_hcnt(input_clock_khz,
				6,	/* tHD;STA = tHIGH = 0.6 us */
				3,	/* tf = 0.3 us */
				0,	/* 0: DW default, 1: Ideal */
				0);	/* No offset */
	lcnt = i2c_dw_scl_lcnt(input_clock_khz,
				13,	/* tLOW = 1.3 us */
				3,	/* tf = 0.3 us */
				0);	/* No offset */
	writel(hcnt, dev->base + DW_IC_FS_SCL_HCNT);
	writel(lcnt, dev->base + DW_IC_FS_SCL_LCNT);
	dev_dbg("Fast-mode HCNT:LCNT = %d:%d\n", hcnt, lcnt);

	/* Configure Tx/Rx FIFO threshold levels */
	writel(dev->tx_fifo_depth - 1, dev->base + DW_IC_TX_TL);
	writel(0, dev->base + DW_IC_RX_TL);
	
	// I2C Receive Data Level Register
	writel(0, dev->base + DW_IC_DMA_RDLR);
	// DMA Transmit Data Level Register
	writel(4, dev->base + DW_IC_DMA_TDLR);

	// de-noise设置
	//writel((32-15) | (1 << 7), dev->base + DW_IC_DENOISE);
	writel(10 | (1 << 7), dev->base + DW_IC_DENOISE);
	
	/* configure the i2c master */
	//ic_con = DW_IC_CON_MASTER | DW_IC_CON_SLAVE_DISABLE |
	//	DW_IC_CON_RESTART_EN | DW_IC_CON_SPEED_FAST;
	ic_con = DW_IC_CON_MASTER | DW_IC_CON_SLAVE_DISABLE |
		DW_IC_CON_RESTART_EN | DW_IC_CON_SPEED_STD;
	writel(ic_con, dev->base + DW_IC_CON);
}

/*
 * Waiting for bus not busy
 */
static int i2c_dw_wait_bus_not_busy(struct dw_i2c_dev *dev)
{
	int timeout = TIMEOUT;

	while (readl(dev->base + DW_IC_STATUS) & DW_IC_STATUS_ACTIVITY) {
		if (timeout <= 0) {
			dev_warn("timeout waiting for bus ready\n");
			return -ETIMEDOUT;
		}
		timeout--;
		OS_Delay(1);
	}

	return 0;
}

static void i2c_dw_xfer_init(struct dw_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	u32 ic_con;

	/* Disable the adapter */
	writel(0, dev->base + DW_IC_ENABLE);

	/* set the slave (target) address */
	writel(msgs[dev->msg_write_idx].addr, dev->base + DW_IC_TAR);

	/* if the slave address is ten bit address, enable 10BITADDR */
	ic_con = readl(dev->base + DW_IC_CON);
	if (msgs[dev->msg_write_idx].flags & I2C_M_TEN)
		ic_con |= DW_IC_CON_10BITADDR_MASTER;
	else
		ic_con &= ~DW_IC_CON_10BITADDR_MASTER;
	writel(ic_con, dev->base + DW_IC_CON);

	/* Enable the adapter */
	writel(1, dev->base + DW_IC_ENABLE);

	/* Enable interrupts */
	writel(DW_IC_INTR_DEFAULT_MASK, dev->base + DW_IC_INTR_MASK);
}

/*
 * Initiate (and continue) low level master read/write transaction.
 * This function is only called from i2c_dw_isr, and pumping i2c_msg
 * messages into the tx buffer.  Even if the size of i2c_msg data is
 * longer than the size of the tx buffer, it handles everything.
 */
static void
i2c_dw_xfer_msg(struct dw_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	u32 intr_mask;
	int tx_limit, rx_limit;
	u32 addr = msgs[dev->msg_write_idx].addr;
	u32 buf_len = dev->tx_buf_len;
	u8 *buf = dev->tx_buf;;

	intr_mask = DW_IC_INTR_DEFAULT_MASK;

	for (; dev->msg_write_idx < dev->msgs_num; dev->msg_write_idx++) {
		/*
		 * if target address has changed, we need to
		 * reprogram the target address in the i2c
		 * adapter when we are done with this transfer
		 */
		if (msgs[dev->msg_write_idx].addr != addr) {
			dev_err("%s: invalid target address\n", __func__);
			dev->msg_err = -EINVAL;
			break;
		}

		if (msgs[dev->msg_write_idx].len == 0) {
			dev_err("%s: invalid message length\n", __func__);
			dev->msg_err = -EINVAL;
			break;
		}

		if (!(dev->status & STATUS_WRITE_IN_PROGRESS)) {
			/* new i2c_msg */
			buf = msgs[dev->msg_write_idx].buf;
			buf_len = msgs[dev->msg_write_idx].len;
		}

		tx_limit = dev->tx_fifo_depth - readl(dev->base + DW_IC_TXFLR);
		rx_limit = dev->rx_fifo_depth - readl(dev->base + DW_IC_RXFLR);
		while (buf_len > 0 && tx_limit > 0 && rx_limit > 0) {
			if (msgs[dev->msg_write_idx].flags & I2C_M_RD) {
				writel(0x100, dev->base + DW_IC_DATA_CMD);
				rx_limit--;
			} else
				writel(*buf++, dev->base + DW_IC_DATA_CMD);
			tx_limit--; buf_len--;
		}

		dev->tx_buf = buf;
		dev->tx_buf_len = buf_len;

		if (buf_len > 0) {
			/* more bytes to be written */
			dev->status |= STATUS_WRITE_IN_PROGRESS;
			break;
		} else
			dev->status &= ~STATUS_WRITE_IN_PROGRESS;
	}

	/*
	 * If i2c_msg index search is completed, we don't need TX_EMPTY
	 * interrupt any more.
	 */
	if (dev->msg_write_idx == dev->msgs_num)
		intr_mask &= ~DW_IC_INTR_TX_EMPTY;

	if (dev->msg_err)
		intr_mask = 0;

	writel(intr_mask, dev->base + DW_IC_INTR_MASK);
}

static int 
i2c_dw_read(struct dw_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	int rx_valid;

	for (; dev->msg_read_idx < dev->msgs_num; dev->msg_read_idx++) {
		u32 len;
		u8 *buf;

		if (!(msgs[dev->msg_read_idx].flags & I2C_M_RD))
			continue;

		if (!(dev->status & STATUS_READ_IN_PROGRESS)) {
			len = msgs[dev->msg_read_idx].len;
			buf = msgs[dev->msg_read_idx].buf;
		} else {
			len = dev->rx_buf_len;
			buf = dev->rx_buf;
		}

		rx_valid = readl(dev->base + DW_IC_RXFLR);
		if(rx_valid == 0)
		{
			// 20170801 i2c异常, 未知原因触发.
			// 2路硬件I2C同时访问时(Sensor & TP), 在关闭其中1路I2C(Sensor)时, 某种条件下会触发以下异常
			// (I2C报告FULL中断, 但是读出的FIFO有效字节长度为0)
			// 此后会一直触发FULL中断, 且无法清除该中断, 导致CPU资源完全被I2C中断占用.
			// 加入异常处理机制可以解决此问题.
			return -1;
		}

		for (; len > 0 && rx_valid > 0; len--, rx_valid--)
			*buf++ = readl(dev->base + DW_IC_DATA_CMD);

		if (len > 0) {
			dev->status |= STATUS_READ_IN_PROGRESS;
			dev->rx_buf_len = len;
			dev->rx_buf = buf;
			//return;
			break;
		} else
			dev->status &= ~STATUS_READ_IN_PROGRESS;
	}
	
	return 0;
}

static int i2c_dw_handle_tx_abort(struct dw_i2c_dev *dev)
{
	unsigned long abort_source = dev->abort_source;
	int i;
	
	//XM_printf ("abort=0x%x\n", abort_source);
	
	if (abort_source & DW_IC_TX_ABRT_NOACK) {
		for (i = 0; i < sizeof(abort_sources)/sizeof(abort_sources[0]); i ++)
		{
			if(abort_source & (1 << i))
				dev_dbg("%s: %s\n", __func__, abort_sources[i]);
		}
		//for_each_set_bit(i, &abort_source, ARRAY_SIZE(abort_sources))
		//	dev_dbg("%s: %s\n", __func__, abort_sources[i]);
		return -EREMOTEIO;
	}

	//for_each_set_bit(i, &abort_source, ARRAY_SIZE(abort_sources))
	//	dev_err(dev->dev, "%s: %s\n", __func__, abort_sources[i]);
	for (i = 0; i < sizeof(abort_sources)/sizeof(abort_sources[0]); i ++)
	{
		if(abort_source & (1 << i))
			dev_err("%s: %s\n", __func__, abort_sources[i]);
	}

	if (abort_source & DW_IC_TX_ARB_LOST)
		return -EAGAIN;
	else if (abort_source & DW_IC_TX_ABRT_GCALL_READ)
		return -EINVAL; /* wrong msgs[] data */
	else
		return -EIO;
}


/*
 * Prepare controller for a transaction and call i2c_dw_xfer_msg
 */
static int i2c_dw_xfer(struct dw_i2c_dev *dev, struct i2c_msg msgs[], int num)
{
	int ret;

	dev_dbg("%s: msgs: %d\n", __func__, num);

	mutex_lock(&dev->lock);

	INIT_COMPLETION(dev->cmd_complete);
	dev->msgs = msgs;
	dev->msgs_num = num;
	dev->cmd_err = 0;
	dev->msg_write_idx = 0;
	dev->msg_read_idx = 0;
	dev->msg_err = 0;
	dev->status = STATUS_IDLE;
	dev->abort_source = 0;

	ret = i2c_dw_wait_bus_not_busy(dev);
	if (ret < 0)
		goto done;

	/* start the transfers */
	i2c_dw_xfer_init(dev);

	/* wait for tx to complete */
	ret = wait_for_completion_interruptible_timeout(&dev->cmd_complete, HZ);
	if (ret == 0) {
		//dev_err("controller timed out\n");
		i2c_dw_init(dev);
		ret = -ETIMEDOUT;
		goto done;
	} else if (ret < 0)
		goto done;

	if (dev->msg_err) {
		ret = dev->msg_err;
		goto done;
	}

	/* no error */
	if (likely(!dev->cmd_err)) {
		/* Disable the adapter */
		//writel(0, dev->base + DW_IC_ENABLE);
		ret = num;
		goto done;
	}

	/* We have an error */
	if (dev->cmd_err == DW_IC_ERR_TX_ABRT) {
		ret = i2c_dw_handle_tx_abort(dev);
		goto done;
	}
	ret = -EIO;

done:
	// A value of 0 for MST_ACTIVITY indicates that the I2C master FSM is in the IDLE state. This
	// is a pre-condition for safely shutting down DW_apb_i2c.	
	i2c_dw_wait_bus_not_busy(dev);
	
	//i2c_delay(80000);
	
	/* Disable the adapter */
	writel(0, dev->base + DW_IC_ENABLE);
	
	mutex_unlock(&dev->lock);

	return ret;
}



static u32 i2c_dw_read_clear_intrbits(struct dw_i2c_dev *dev)
{
	u32 stat;

	/*
	 * The IC_INTR_STAT register just indicates "enabled" interrupts.
	 * Ths unmasked raw version of interrupt status bits are available
	 * in the IC_RAW_INTR_STAT register.
	 *
	 * That is,
	 *   stat = readl(IC_INTR_STAT);
	 * equals to,
	 *   stat = readl(IC_RAW_INTR_STAT) & readl(IC_INTR_MASK);
	 *
	 * The raw version might be useful for debugging purposes.
	 */
	stat = readl(dev->base + DW_IC_INTR_STAT);

	/*
	 * Do not use the IC_CLR_INTR register to clear interrupts, or
	 * you'll miss some interrupts, triggered during the period from
	 * readl(IC_INTR_STAT) to readl(IC_CLR_INTR).
	 *
	 * Instead, use the separately-prepared IC_CLR_* registers.
	 */
	if (stat & DW_IC_INTR_RX_UNDER)
		readl(dev->base + DW_IC_CLR_RX_UNDER);
	if (stat & DW_IC_INTR_RX_OVER)
		readl(dev->base + DW_IC_CLR_RX_OVER);
	if (stat & DW_IC_INTR_TX_OVER)
		readl(dev->base + DW_IC_CLR_TX_OVER);
	if (stat & DW_IC_INTR_RD_REQ)
		readl(dev->base + DW_IC_CLR_RD_REQ);
	if (stat & DW_IC_INTR_TX_ABRT) {
		/*
		 * The IC_TX_ABRT_SOURCE register is cleared whenever
		 * the IC_CLR_TX_ABRT is read.  Preserve it beforehand.
		 */
		dev->abort_source = readl(dev->base + DW_IC_TX_ABRT_SOURCE);
		readl(dev->base + DW_IC_CLR_TX_ABRT);
	}
	if (stat & DW_IC_INTR_RX_DONE)
		readl(dev->base + DW_IC_CLR_RX_DONE);
	if (stat & DW_IC_INTR_ACTIVITY)
		readl(dev->base + DW_IC_CLR_ACTIVITY);
	if (stat & DW_IC_INTR_STOP_DET)
		readl(dev->base + DW_IC_CLR_STOP_DET);
	if (stat & DW_IC_INTR_START_DET)
		readl(dev->base + DW_IC_CLR_START_DET);
	if (stat & DW_IC_INTR_GEN_CALL)
		readl(dev->base + DW_IC_CLR_GEN_CALL);

	return stat;
}

/*
 * Interrupt service routine. This gets called whenever an I2C interrupt
 * occurs.
 */
static void i2c_dw_isr(void *dev_id)
{
	struct dw_i2c_dev *dev = dev_id;
	u32 stat;

	stat = i2c_dw_read_clear_intrbits(dev);
	dev_dbg("%s: stat=0x%x\n", __func__, stat);

	if (stat & DW_IC_INTR_TX_ABRT) {
		dev->cmd_err |= DW_IC_ERR_TX_ABRT;
		dev->status = STATUS_IDLE;

		/*
		 * Anytime TX_ABRT is set, the contents of the tx/rx
		 * buffers are flushed.  Make sure to skip them.
		 */
		writel(0, dev->base + DW_IC_INTR_MASK);
		goto tx_aborted;
	}

	if (stat & DW_IC_INTR_RX_FULL)
	{
		if(i2c_dw_read(dev) < 0)
		{
			// 20170801 i2c异常, 未知原因触发.
			// 2路硬件I2C同时访问时(Sensor & TP), 在关闭其中1路I2C(Sensor)时, 某种条件下会触发以下异常
			// (I2C报告FULL中断, 但是读出的FIFO有效字节长度为0)
			// 此后会一直触发FULL中断, 且无法清除该中断, 导致CPU资源完全被I2C中断占用.
			// 加入异常处理机制可以解决此问题.
			XM_printf ("i2c read FIFO error\n");
			dev->cmd_err |= DW_IC_ERR_TX_ABRT;
			dev->status = STATUS_IDLE;

			writel(0, dev->base + DW_IC_INTR_MASK);
			goto tx_aborted;
			
		}
	}
	
	if (stat & DW_IC_INTR_TX_EMPTY)
	{
		i2c_dw_xfer_msg(dev);
	}

	/*
	 * No need to modify or disable the interrupt mask here.
	 * i2c_dw_xfer_msg() will take care of it according to
	 * the current transmit status.
	 */

tx_aborted:
	if ((stat & (DW_IC_INTR_TX_ABRT | DW_IC_INTR_STOP_DET)) || dev->msg_err)
	{
		complete(&dev->cmd_complete);
	}

}

static void i2c_0_isr(void)
{
	xm_i2c_dev *dev = i2c_device[0];
	if(dev && dev->i2c_id == XMI2C_ID_FLAG)
		i2c_dw_isr (dev);
	else
	{
		XM_printf ("i2c 0 isr error\n");
	}
}

void xm_i2c_init (void)
{
	int i;
	memset (i2c_device, 0, sizeof(i2c_device));
	for (i = 0; i < XM_I2C_DEV_COUNT; i ++)
	{
		OS_CREATERSEMA (&i2c_access_sema[i]); /* Creates resource semaphore */
	}
	
	// 设置"从设备"队列为空
	queue_initialize (&i2c_client_head);
	OS_CREATERSEMA (&i2c_client_access_sema);
}

// 创建一个I2C slave设备
xm_i2c_client_t  xm_i2c_open (u8_t  dev_id, 			// i2c设备号
										u16_t slave_address, 		// slave设备地址
										char *name,						// slave设备名
										u16_t i2c_addressing_bit, 	// 7bit或者10bit地址模式
										int *err_code					// 失败时保存错误代码
										)
{
	xm_i2c_client*	client = NULL;
	int ret = XM_I2C_ERRCODE_OK;
		
	if(dev_id >= XM_I2C_DEV_COUNT)
	{
		XM_printf ("i2c client open failed, dev_id (%d) illegal\n", dev_id);
		if(err_code)
			*err_code = XM_I2C_ERRCODE_ILLEGAL_PARA;
		return NULL;
	}
	if(i2c_addressing_bit != XM_I2C_ADDRESSING_7BIT && i2c_addressing_bit != XM_I2C_ADDRESSING_10BIT)
	{
		XM_printf ("i2c client open failed, addressing bit (%d) illegal\n", i2c_addressing_bit);
		if(err_code)
			*err_code = XM_I2C_ERRCODE_ILLEGAL_PARA;
		return NULL;
	}
	if(i2c_addressing_bit == XM_I2C_ADDRESSING_7BIT && slave_address >= 128)
	{
		XM_printf ("i2c client open failed, slave address (%d) illegal, great than 127(7bit addressing)\n", slave_address);
		if(err_code)
			*err_code = XM_I2C_ERRCODE_ILLEGAL_PARA;
		return NULL;
	}
	else if(i2c_addressing_bit == XM_I2C_ADDRESSING_10BIT && slave_address >= 1024)
	{
		XM_printf ("i2c client open failed, slave address (%d) illegal, great than 1023(10bit addressing)\n", slave_address);
		if(err_code)
			*err_code = XM_I2C_ERRCODE_ILLEGAL_PARA;
		return NULL;
	}
	
	// 检查i2c设备是否已开启
	//XM_printf ("i2c client(%s) open, dev_id(%d), slave address(0x%x)\n", name, dev_id, slave_address);
	
	// 扫描从设备是否已打开
	// 保护互斥访问
	OS_Use (&i2c_client_access_sema);
	if(!queue_empty(&i2c_client_head))
	{
		// 非空链表
		xm_i2c_client *link = (xm_i2c_client *)queue_next (&i2c_client_head);
		while(link && (queue_s *)link != &i2c_client_head)
		{
			if(link->addr == slave_address && link->dev_id == dev_id)
			{
				XM_printf ("i2c client open failed, i2c dev(%d) slave address(%d) can't re-open\n", dev_id, slave_address);
				ret = XM_I2C_ERRCODE_DEV_OPENED;
				break;
			}
			link = (xm_i2c_client *)queue_next ((queue_s *)link);
		}	
	}

	do
	{
		if(ret != XM_I2C_ERRCODE_OK)
			break;
		client = kernel_malloc (sizeof(xm_i2c_client));
		if(client == NULL)
		{
			XM_printf ("i2c dev_id (%d) open failed, memory busy\n", dev_id);
			ret = XM_I2C_ERRCODE_NOMEM;
			break;
		}
		memset (client, 0, sizeof(xm_i2c_client));
		OS_CREATERSEMA (&client->access_sema);
		client->i2c_slave_id = XMI2C_SLAVE_ID_FLAG;
		client->addr = slave_address;
		client->dev_id = dev_id;
		if(i2c_addressing_bit == XM_I2C_ADDRESSING_10BIT)
			client->flags |= I2C_M_TEN;
		if(name)
		{
			int len = strlen(name);
			if(len >= MAX_I2C_NAME)
				len = MAX_I2C_NAME - 1;
			memcpy (client->name, name, len);
		}
		
		// 打开i2c设备
		ret = xm_i2c_device_open(dev_id);
		if(ret != XM_I2C_ERRCODE_OK)
		{
			kernel_free (client);
			client = NULL;
			break;
		}
		
		client->driver = i2c_device[dev_id];		
		
	} while (0);
	
	OS_Unuse (&i2c_client_access_sema);
	
	if(err_code)
		*err_code = ret;
	return client;
}

// 关闭I2C设备
// = 0 成功
// < 0 表示错误码 
int xm_i2c_close (xm_i2c_client_t i2c_device)
{
	int ret = XM_I2C_ERRCODE_OK;
	xm_i2c_client*	client = (xm_i2c_client*)i2c_device;
	
	// 保护互斥访问
	OS_Use (&i2c_client_access_sema);
	if(client == NULL || client->i2c_slave_id != XMI2C_SLAVE_ID_FLAG)
	{
		// 无效的slave句柄
		OS_Unuse (&i2c_client_access_sema);
		XM_printf ("i2c client(0x%x) close failed, illegal handle\n", client); 
		return XM_I2C_ERRCODE_ILLEGAL_CLIENT_HANDLE;
	}
	
	//XM_printf ("i2c client(%s) close, dev_id(%d), slave address(0x%x)\n", client->name, client->dev_id, client->addr);
	
	// 锁定slave设备句柄, 阻止其他对slave设备句柄的访问
	OS_Use (&client->access_sema);
	
	// 关闭i2c设备
	ret = xm_i2c_device_close (client->dev_id);
	
	// 删除slave设备标记
	client->i2c_slave_id = 0;
	
	OS_Unuse (&client->access_sema);
	
	OS_DeleteRSema (&client->access_sema);
	
	// 从设备链表中删去该slave设备
	queue_delete ((queue_s *)client);
	kernel_free (client);
	
	OS_Unuse (&i2c_client_access_sema);
	
	return ret;
}

// 向I2C从设备写入数据(发送)
// 	timeout 毫秒为单位的超时时间
// 返回值
// 	>= 0 成功写入的字节数
// 	< 0  表示错误码
// 判断成功写入的字节数 是否等于待传输的字节数, 判断是否超时. 不等于表示写入超时
int xm_i2c_write (xm_i2c_client_t i2c_client, 	// client设备句柄
						u8_t *data, 						// 写入的数据基址及字节长度
						int size, 
						int timeout							// 超时设置
																// 	<  0, 	表示无限等待
																//		== 0,		表示不等待
																//		>  0,		表示最大的等待时间(毫秒单位)
						)
{
	int ret = XM_I2C_ERRCODE_OK;
#if I2C_WRITE_METHOD == I2C_WRITE_METHOD_PIO
	struct i2c_msg msg;
#endif
	xm_i2c_client * client = (xm_i2c_client *)i2c_client;
	if(size > 8192)
		size = 8192;
	
	OS_Use (&i2c_client_access_sema);
	if(client == NULL || client->i2c_slave_id != XMI2C_SLAVE_ID_FLAG)
	{
		// 无效的slave句柄
		XM_printf ("i2c write failed, illegal slave handle(0x%x)\n", client);
		OS_Unuse (&i2c_client_access_sema);
		return XM_I2C_ERRCODE_ILLEGAL_CLIENT_HANDLE;
	}
	
	// 锁定slave设备句柄, 阻止其他对slave设备句柄的访问. 如阻止关闭句柄
	OS_Use (&client->access_sema);
	// 解锁slave设备链访问
	OS_Unuse (&i2c_client_access_sema);
	
#if I2C_WRITE_METHOD == I2C_WRITE_METHOD_PIO
	msg.addr = client->addr;
	
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = size;
	msg.buf = data;
	
	
	// 锁定与该slave设备关联的i2c设备, 阻止并发访问
	OS_Use (&i2c_access_sema[client->dev_id]);
	
	ret = i2c_dw_xfer (client->driver, &msg, 1);
	
	// 解锁与该slave设备关联的i2c设备
	OS_Unuse (&i2c_access_sema[client->dev_id]);
	
#elif I2C_WRITE_METHOD == I2C_WRITE_METHOD_DMA

	// 仅用作DMA功能的验证, 未考虑任何效率
	struct dw_i2c_dev *dev = client->driver;
	if(i2c_dw_wait_bus_not_busy(dev) >= 0)
	{
		/* start the transfers */
		/* Disable the adapter */
		writel(0, dev->base + DW_IC_ENABLE);

		/* set the slave (target) address */
		writel(client->addr, dev->base + DW_IC_TAR);
		
		/* if the slave address is ten bit address, enable 10BITADDR */
		unsigned int ic_con = readl(dev->base + DW_IC_CON);
		if (client->flags & I2C_M_TEN)
			ic_con |= DW_IC_CON_10BITADDR_MASTER;
		else
			ic_con &= ~DW_IC_CON_10BITADDR_MASTER;
		writel(ic_con, dev->base + DW_IC_CON);
		
		/* Enable the adapter */
		writel(1, dev->base + DW_IC_ENABLE);
		
		// enable TX DMA
		unsigned int dma_cr = readl(dev->base + DW_IC_DMA_CR);
		//dma_cr |= (1 << 1);		// TDMAE
		dma_cr = (1 << 1);		// TDMAE
		writel(dma_cr, dev->base + DW_IC_DMA_CR);
		
		/* Enable interrupts */
		//writel(DW_IC_INTR_DEFAULT_MASK, dev->base + DW_IC_INTR_MASK);
		UINT32 I2CDmaCh;
		UINT32 DmaRequestTime=0;
		do{
			I2CDmaCh = dma_request_dma_channel(0);
			if((DmaRequestTime++)>50000)
			{
				XM_printf("I2C request dma channel failed!\n");
				ret = XM_I2C_ERRCODE_DEVICE_BUSY;
				goto end_of_tx;
			}	
		}while(I2CDmaCh == ALL_CHANNEL_RUNNING);
		
		dma_clean_range((UINT32)data, (UINT32)data + size);
		
		dma_cfg_channel (I2CDmaCh,
					 (UINT32)data,(UINT32)(dev->base + DW_IC_DATA_CMD) ,
					 0,
					 I2C_WRITE_DMA_CTL_L, I2C_WRITE_DMA_CTL_H|size,
					 I2C_WRITE_DMA_CFG_L|I2CDmaCh, I2C_WRITE_DMA_CFG_H,
					 0, 0);
		rDMA_CHEN_L|= ((1<<(8+I2CDmaCh))|(1<<I2CDmaCh));
		while(!dma_transfer_over(I2CDmaCh));
		dma_detect_ch_disable(I2CDmaCh);
		dma_release_channel(I2CDmaCh);
		
		// A value of 0 for MST_ACTIVITY indicates that the I2C master FSM is in the IDLE state. This
		// is a pre-condition for safely shutting down DW_apb_i2c.	
		i2c_dw_wait_bus_not_busy (dev);
		
		/* Disable the adapter */
		writel(0, dev->base + DW_IC_ENABLE);
		
	}
	
end_of_tx:
#endif	
	
	// 解锁slave设备句柄
	OS_Unuse (&client->access_sema);
	
	return ret;
}

// 从i2c client设备读取数据
// 	timeout 毫秒为单位的超时时间
// 返回值
// 	>= 0 成功读出的字节数, 读取的字数数 != size 表示已超时
// 	< 0  表示错误码
int	xm_i2c_read  (xm_i2c_client_t i2c_client, 	// client设备句柄
						  u8_t *data, 						// 读出的数据基址及字节长度
						  int size, 
						  int timeout						// 超时设置
																// 	<  0, 	表示无限等待
																//		== 0,		表示不等待
																//		>  0,		表示最大的等待时间(毫秒单位)
						)
{
	int ret = XM_I2C_ERRCODE_OK;
	struct i2c_msg msg;
	xm_i2c_client * client = (xm_i2c_client *)i2c_client;
	if(size > 8192)
		size = 8192;
	
	OS_Use (&i2c_client_access_sema);
	if(client == NULL || client->i2c_slave_id != XMI2C_SLAVE_ID_FLAG)
	{
		// 无效的slave句柄
		XM_printf ("i2c read failed, illegal slave handle(0x%x)\n", client);
		OS_Unuse (&i2c_client_access_sema);
		return XM_I2C_ERRCODE_ILLEGAL_CLIENT_HANDLE;
	}
	
	// 锁定slave设备句柄, 阻止其他对slave设备句柄的访问. 如阻止关闭句柄
	OS_Use (&client->access_sema);
	// 解锁slave设备链访问
	OS_Unuse (&i2c_client_access_sema);
	
#if I2C_READ_METHOD == I2C_READ_METHOD_PIO
	msg.addr = client->addr;
	
	msg.flags = client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = size;
	msg.buf = data;
	
	
	// 锁定与该slave设备关联的i2c设备, 阻止并发访问
	OS_Use (&i2c_access_sema[client->dev_id]);
	
	ret = i2c_dw_xfer (client->driver, &msg, 1);
	if(ret == 1)
	{
		ret = msg.len;
	}
	
	// 解锁与该slave设备关联的i2c设备
	OS_Unuse (&i2c_access_sema[client->dev_id]);
#elif I2C_READ_METHOD == I2C_READ_METHOD_DMA
	// 仅用作DMA功能的验证, 未考虑任何效率
	struct dw_i2c_dev *dev = client->driver;
	if(i2c_dw_wait_bus_not_busy(dev) >= 0)
	{
		/* start the transfers */
		/* Disable the adapter */
		writel(0, dev->base + DW_IC_ENABLE);

		/* set the slave (target) address */
		writel(client->addr, dev->base + DW_IC_TAR);
		
		/* if the slave address is ten bit address, enable 10BITADDR */
		unsigned int ic_con = readl(dev->base + DW_IC_CON);
		if (client->flags & I2C_M_TEN)
			ic_con |= DW_IC_CON_10BITADDR_MASTER;
		else
			ic_con &= ~DW_IC_CON_10BITADDR_MASTER;
		writel(ic_con, dev->base + DW_IC_CON);
		
		/* Enable the adapter */
		writel(1, dev->base + DW_IC_ENABLE);
	
		// enable RX DMA
		unsigned int dma_cr = readl(dev->base + DW_IC_DMA_CR);
		//dma_cr |= (1 << 0);		// RDMAE
		dma_cr = (1 << 0);		// RDMAE
		writel(dma_cr, dev->base + DW_IC_DMA_CR);
		
		/* Enable interrupts */
		//writel(DW_IC_INTR_DEFAULT_MASK, dev->base + DW_IC_INTR_MASK);
		UINT32 I2CDmaCh;
		UINT32 DmaRequestTime=0;
		do{
			I2CDmaCh = dma_request_dma_channel(0);
			if((DmaRequestTime++)>50000)
			{
				XM_printf("I2C request dma channel failed!\n");
				ret = XM_I2C_ERRCODE_DEVICE_BUSY;
				goto end_of_rx;
			}	
		}while(I2CDmaCh == ALL_CHANNEL_RUNNING);
		
		dma_inv_range ((UINT32)data, ((UINT32)data) + size);
		
		dma_cfg_channel (I2CDmaCh,
					 (UINT32)(dev->base + DW_IC_DATA_CMD) ,
					 (UINT32)data,
					 0,
					 I2C_READ_DMA_CTL_L, I2C_READ_DMA_CTL_H|size,
					 I2C_READ_DMA_CFG_L|I2CDmaCh, I2C_READ_DMA_CFG_H,
					 0, 0);
		rDMA_CHEN_L|= ((1<<(8+I2CDmaCh))|(1<<I2CDmaCh));
	
		int tmp = size;
		while(tmp--)
		{
			writel (0x0100, dev->base + DW_IC_DATA_CMD); /*read data*/
		}
		
		while(!dma_transfer_over(I2CDmaCh));
		dma_detect_ch_disable(I2CDmaCh);
		dma_release_channel(I2CDmaCh);
		
		dma_inv_range ((UINT32)data, ((UINT32)data) + size);

		// A value of 0 for MST_ACTIVITY indicates that the I2C master FSM is in the IDLE state. This
		// is a pre-condition for safely shutting down DW_apb_i2c.	
		i2c_dw_wait_bus_not_busy (dev);
		/* Disable the adapter */
		writel(0, dev->base + DW_IC_ENABLE);
		
	}
	
end_of_rx:	
#endif
	
	// 解锁slave设备句柄
	OS_Unuse (&client->access_sema);
	
	return ret;	
}

int	xm_i2c_device_open (u8_t dev_id)
{
	int ret = XM_I2C_ERRCODE_OK;
	xm_i2c_dev *dev = NULL;
	if(dev_id >= XM_I2C_DEV_COUNT)
		return XM_I2C_ERRCODE_ILLEGAL_PARA;
	
	// 保护互斥访问
	OS_Use (&i2c_access_sema[dev_id]);
	
	dev = i2c_device[dev_id];
	do 
	{
		// 检查设备是否已创建
		if(dev)
		{
			// 检查是否合法的设备
			if(dev->i2c_id != XMI2C_ID_FLAG)
			{
				XM_printf ("i2c device(%d) open failed, illegal I2C id (%d)\n", dev_id, dev->i2c_id);
				ret = XM_I2C_ERRCODE_ILLEGAL_HANDLE;
				break;
			}
			dev->ref_count ++;
			break;
		}
		
		// 设备未创建
		dev = kernel_malloc (sizeof(xm_i2c_dev));
		if(dev == NULL)
		{
			ret = XM_I2C_ERRCODE_NOMEM;
			break;
		}
		memset (dev, 0, sizeof(xm_i2c_dev));
		dev->i2c_id = XMI2C_ID_FLAG;
		dev->dev_id = dev_id;
		dev->irq = i2c_irq[dev_id];
		dev->base = (char *)i2c_base[dev_id];
		dev->ref_count = 1;
		
		OS_CREATERSEMA (&dev->lock);
		OS_EVENT_Create (&dev->cmd_complete);
		
		// clock enable
		i2c_clock_enable (dev_id);
		
		// IP reset
		i2c_reset (dev_id);
		
		// PAD配置为I2C function
		select_i2c_pad (dev_id);
		
		OS_Delay (40);
		
		{
			u32 param1 = readl(dev->base + DW_IC_COMP_PARAM_1);
	
			dev->tx_fifo_depth = ((param1 >> 16) & 0xff) + 1;
			dev->rx_fifo_depth = ((param1 >> 8)  & 0xff) + 1;
			
			// HAS_DMA
			if(param1 & (1 << 6))
				dev_dbg ("i2c dev(%d) support DMA\n", dev_id);
			else
				dev_dbg ("i2c dev(%d) don't support DMA\n", dev_id);
			// MAX_SPEED_MODE
			switch( (param1 >> 2) & 3 )
			{
				case 0:	dev_dbg ("Max Speed Mode reserved\n");		break;
				case 1:	dev_dbg ("Max Speed Mode (Standard)\n");	break;	
				case 2:	dev_dbg ("Max Speed Mode (Fast)\n");		break;	
				case 3:	dev_dbg ("Max Speed Mode (High)\n");		break;	
			}
			// APB_DATA_WIDTH
			switch( (param1 >> 0) & 3 )
			{
				case 0:	dev_dbg ("APB DATA WIDTH (8bit)\n");		break;
				case 1:	dev_dbg ("APB DATA WIDTH (16bit)\n");		break;
				case 2:	dev_dbg ("APB DATA WIDTH (32bit)\n");		break;		
			}
		}
		
		i2c_device[dev_id] = dev;
		
		i2c_dw_init(dev);
		
		writel(0, dev->base + DW_IC_INTR_MASK); /* disable IRQ */
		
		request_irq (dev->irq, PRIORITY_FIFTEEN, i2c_0_isr);
		
	} while (0);
	
	// 解除互斥访问保护
	OS_Unuse (&i2c_access_sema[dev_id]);
	
	return ret;
}

// 关闭I2C设备
// dev_id I2C设备ID
int	xm_i2c_device_close (u8_t dev_id)
{
	int ret = XM_I2C_ERRCODE_OK;
	xm_i2c_dev *dev = NULL;
	if(dev_id >= XM_I2C_DEV_COUNT)
		return XM_I2C_ERRCODE_ILLEGAL_PARA;

	// 保护互斥访问
	OS_Use (&i2c_access_sema[dev_id]);
	dev = i2c_device[dev_id];
	do 
	{
		if(dev == NULL)
		{
			XM_printf ("i2c device (%d) close failed, device closed before\n", dev_id);
			ret = XM_I2C_ERRCODE_DEV_CLOSED;
			break;
		}
		if(dev->i2c_id != XMI2C_ID_FLAG)
		{
			XM_printf ("i2c device(%d) close failed, illegal I2C id (%d)\n", dev_id, dev->i2c_id);
			ret = XM_I2C_ERRCODE_ILLEGAL_HANDLE;
			break;
		}
		if(dev->ref_count == 0)
		{
			XM_printf ("i2c device (%d) close failed, illegal device ref_count\n", dev_id);
			ret = XM_I2C_ERRCODE_ILLEGAL_HANDLE;
			break;
		}
		
		dev->ref_count --;
		// 判断设备是否继续使用 (多个I2C slave 设备连接到同一个I2C master)
		if(dev->ref_count)	
			break;
		
		// I2C设备不再使用
		
		// PAD配置为GPIO输入上拉
		unselect_i2c_pad (dev_id);
		
		// close clock
		i2c_clock_disable (dev_id);
		
		// disable device
		writel(0, dev->base + DW_IC_ENABLE);
		
		// 关闭中断
		request_irq (dev->irq, PRIORITY_FIFTEEN, NULL);
		
		i2c_device[dev_id] = NULL;
		
		OS_DeleteRSema (&dev->lock);
		
		OS_EVENT_Delete (&dev->cmd_complete);
		
		dev->i2c_id = 0;
		
		kernel_free (dev);
		
	} while (0);
	// 解除互斥访问保护
	OS_Unuse (&i2c_access_sema[dev_id]);
	
	return ret;
}

#endif	// #if _XMSYS_I2C_ == _XMSYS_I2C_HARDWARE_	// 硬件I2C模式
