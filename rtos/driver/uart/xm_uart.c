//****************************************************************************
//
//	Copyright (C) 2012~2016 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_uart.h
//	  uart device driver
//
//	Revision history
//
//		2012.10.02	ZhuoYongHong Initial version
//
//****************************************************************************
#include "hardware.h"
#include "xm_uart.h"
#include "xm_base.h"
#include "xm_dev.h"
#include "uart.h"
#include "irqs.h"
#include "rtos.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "xm_core.h"


#ifdef _XMSYS_DMA_SUPPORT_
#define	_UART_TX_DMA_			// DMA及PIO均已测试,OK
#endif

//#define	_UART_RX_DMA_			// NG, 20160308 DMA测试发现问题, 无法接收1字节的包. 待确认
											//		已确认， ark的bug。


#define	XM_UART_ID_FLAG		0x54524155		// "UART"

#undef UART_CLK
#define UART_CLK 24000000 //24M

#define UART_OE	(1 << 10)	// Overflow
#define UART_BE	(1 << 9)		// bread condition
#define UART_PE	(1 << 8)		// Parity Error
#define UART_FE	(1 << 7)		// Frame Error

typedef struct _tag_xm_uart_dev {
	u32_t		uart_id;				// UART设备的标志字段

	u8_t		dev_id;				// 设备号
	u8_t		stop_bit;
	u8_t		parity;
	u8_t		mode;
	u32_t		baud_rate;			// 串口波特率
	
	// 传输的数据拷贝到tx_fifo, 等待传输
	u8_t *	tx_fifo;				// tx_buff == NULL 或 tx_size == 0表示关闭传输功能
	u32_t		tx_size;
	volatile u32_t		tx_header;	// 队首读出位置
	volatile u32_t		tx_tailer;	// 队尾写入位置
	volatile u32_t		tx_count;
	OS_CSEMA	tx_csema;			// 空闲单元的字节计数信号量
	
	
	
	u8_t *	rx_fifo;
	u32_t		rx_size;
	volatile u32_t		rx_header;
	volatile u32_t		rx_tailer;
	OS_CSEMA	rx_csema;			// 已接收的字节计数信号量
	
	u32_t		tx_byte_count;		// 总共传输的字节数
	u32_t		tx_byte_discard;	// 传输异常丢弃的字节数
	
	// RX统计信息	
	u32_t		rx_byte_count;		// 总共接收的字节数
	u32_t		rx_byte_count_overflow_discard;	// 接收FIFO溢出丢弃的字节数
	u32_t		rx_byte_count_break_discard;		// Break Condition丢弃的字节数
	u32_t		rx_byte_count_parity_discard;		// PE (parity error)
	u32_t		rx_byte_count_stopbit_discard;	// FE (stop bit error)
	
	
	int		err_code;			// 设备通信异常时的错误码
	u32_t		comm_state;			// 记录通信状态, Break Condition
	
	u16_t		intr;					// 中断号
	u8_t		tx_dma_req;				// TX DMA请求号
	u8_t		rx_dma_req;				// RX DMA请求号
	u32_t		uart_dma_control;		// UART DMA Control Register
	u32_t		uart_data_register;		// DMA读写寄存器地址
	
} xm_uart_dev;

static const u32_t uart_base[XM_UART_DEV_COUNT] = {
	UART0_BASE,
	UART1_BASE,
	UART2_BASE,
	UART3_BASE,
};

static xm_uart_dev * uart_device[XM_UART_DEV_COUNT];
static OS_RSEMA uart_access_sema[XM_UART_DEV_COUNT];		// 

#ifndef _UART_TX_DMA_		

// 中断及驱动均调用
static void uart_tx ( xm_uart_dev *uart_dev )
{
	u32_t base = uart_base[uart_dev->dev_id];
	u32_t next_pos;
	irq_disable (uart_dev->intr);		// 禁止当前uart的中断
	
	// 检查发送FIFO是否已满. 若满(Full), 则终止循环
	while ( !(rUART_FR(base) & (1 << 5)) )	// TXFF (tx fifo full)
	{
		// TX-FIFO未满, 继续向FIFO写入数据
		
		// 检查传输缓存是否为空
		int tx_count = uart_dev->tx_count;	//uart_dev->tx_size - OS_GetCSemaValue(&uart_dev->tx_csema);
		if(tx_count == 0)		// 没有需要传输的数据
			break;
		
		rUART_DR(base) = uart_dev->tx_fifo[uart_dev->tx_header];
		// 移动队首指针, 指向下一个位置
		next_pos = uart_dev->tx_header + 1;
		if(next_pos >= uart_dev->tx_size)
			next_pos = 0;
		uart_dev->tx_header = next_pos;
		// 将空闲单元的字节计数信号量 + 1
		OS_SignalCSema (&uart_dev->tx_csema);
		
		uart_dev->tx_count --;
	}
	
	irq_enable (uart_dev->intr);		// 允许当前uart的中断
}
#endif

static void uart_rx ( xm_uart_dev *uart_dev )		// 中断调用
{
	u32_t base = uart_base[uart_dev->dev_id];
	u32_t next_pos;
	int count;
	u32_t data;
	//irq_disable (uart_dev->intr);		// 禁止当前uart的中断
	
	// 循环直到RX FIFO为空
	while ( !(rUART_FR(base) & (1 << 4)) )	// RXFE (rx fifo empty)
	{
		// 从RX FIFO读取一个数据
		data = rUART_DR(base);
		//printf (" 0x%4x\n", data);
		uart_dev->rx_byte_count ++;
		// 检查数据的正确性
		if(data & (1 << 10))	// BE (break error)
		{
			uart_dev->comm_state |= UART_BE;
			uart_dev->rx_byte_count_break_discard ++;		
			continue;
		}
		else if(data & (1 << 11))	// OE (receive fifo full)
		{
			uart_dev->rx_byte_count_overflow_discard ++;
			// 允许当前读出的字节写入到缓冲区， 否则会丢失数据
			// 
			//printf ("OE %02x\n", data & 0xFF);
			//continue;
		}
		else if(data & (1 << 9))
		{
			uart_dev->rx_byte_count_parity_discard ++;
			//continue;
		}
		else if(data & (1 << 8))
		{
			uart_dev->rx_byte_count_stopbit_discard ++;
			//continue;
		}
		
#ifndef _UART_RX_DMA_
		// 数据没有错误
		// 检查RX缓存是否已满
		count = OS_GetCSemaValue (&uart_dev->rx_csema);
		if(count >= uart_dev->rx_size)
		{
			// 溢出, 丢弃处理
			uart_dev->rx_byte_count_overflow_discard ++;
			printf ("rx overflow, discard %d\n", uart_dev->rx_byte_count_overflow_discard);
		}
		else
		{
			// 添加到RX缓存队列的尾部
			uart_dev->rx_fifo[uart_dev->rx_tailer] = (u8_t)data;
			next_pos = uart_dev->rx_tailer + 1;
			if(next_pos >= uart_dev->rx_size)
				next_pos = 0;
			uart_dev->rx_tailer = next_pos;
			
			// 将可用数据单元的字节计数信号量 + 1, 唤醒等待的线程
			OS_SignalCSema (&uart_dev->rx_csema);
		}
#endif
	}
	
	//irq_enable (uart_dev->intr);		// 允许当前uart的中断
}

// dev_id 产生中断的UART设备号
static void uart_isr (u8_t dev_id, u32_t intr_status)
{
	u32_t base = uart_base[dev_id];
	u32_t uart_rsr = 	rUART_RSR(base);
	if(intr_status)
	{
		xm_uart_dev *uart_dev = uart_device[dev_id];
		
		// RX / RX-Timeout
		if( intr_status & ((1 << 4) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10)) )
		{
			//printf ("rx %x\n", intr_status);
			// Receive or Receive Timeout
			if(uart_dev)
				uart_rx (uart_dev);
		}
		
#ifndef _UART_TX_DMA_		
		// TX
		if(intr_status & (1 << 5))
		{
			// Transfer
			if(uart_dev)
			{
				uart_dev->tx_byte_count ++;
				uart_tx (uart_dev);
			}
		}
#endif
		
		// UART Receive Status/Error Clear Register
		// A write to UARTECR clears the framing, parity, break, and overrun errors.
		// Note: A write to this register (D[7:0]) clears the framing, parity, break, and overrun errors. The
		// data value is not important.
		
		if(uart_rsr)
		{
			rUART_RSR(base) = uart_rsr;
		}
		// 清除中断
		rUART_ICR(base) = intr_status;
	}
}

static void uart0_isr (void)
{
	if(rUART0_MIS)
		uart_isr (0, rUART0_MIS);
}

static void uart123_isr (void)
{
	if(rUART1_MIS)
		uart_isr (1, rUART1_MIS);
	if(rUART2_MIS)
		uart_isr (2, rUART2_MIS);
	if(rUART3_MIS)
		uart_isr (3, rUART3_MIS);
}


static void open_uart ( xm_uart_dev *uart_dev )
{
	u32_t value, reg;
	u32_t base = uart_base[uart_dev->dev_id];
	
	// PAD配置
	if(uart_dev->dev_id == XM_UART_DEV_0)
	{
		XM_lock ();		// 阻止并发或中断访问设置
		reg = rSYS_PAD_CTRL09;
		if(uart_dev->mode == XM_UART_MODE_TX || uart_dev->mode == XM_UART_MODE_TR)	// TX
		{
			// uarttxd0
			reg &= ~(0x03 << 18);
			reg |=  (0x01 << 18);
		}
		if(uart_dev->mode == XM_UART_MODE_RX || uart_dev->mode == XM_UART_MODE_TR)	// RX
		{
			// uartrxd0
			reg &= ~(0x03 << 16);
			reg |=  (0x01 << 16);
		}		
		rSYS_PAD_CTRL09 = reg;	
		XM_unlock ();
	
		// IP Reset (6	R/W	1	softreset_uart0)
		XM_lock ();	
		rSYS_SOFT_RSTNB &= ~(1<<6);
		__asm ("nop");
		rSYS_SOFT_RSTNB |=  (1<<6);
		XM_unlock ();
	}
	else if(uart_dev->dev_id == XM_UART_DEV_1)
	{
		XM_lock ();		// 阻止并发或中断访问设置
		reg = rSYS_PAD_CTRL09;
		if(uart_dev->mode == XM_UART_MODE_TX || uart_dev->mode == XM_UART_MODE_TR)	// TX
		{
			// uarttxd1
			reg &= ~(0x03 << 22);
			reg |=  (0x01 << 22);
		}
		if(uart_dev->mode == XM_UART_MODE_RX || uart_dev->mode == XM_UART_MODE_TR)	// RX
		{
			// uartrxd1
			reg &= ~(0x03 << 20);
			reg |=  (0x01 << 20);
		}		
		rSYS_PAD_CTRL09 = reg;	
		XM_unlock ();
	
		sys_clk_disable (uart1_pclk_enable);
		sys_clk_disable (Uart1_clk_enable);
		
		// IP Reset
		XM_lock ();	
		rSYS_SOFT_RSTNB &= ~(1<<5);
		__asm ("nop");
		__asm ("nop");
		rSYS_SOFT_RSTNB |=  (1<<5);		
		XM_unlock ();
		
		sys_clk_enable (uart1_pclk_enable);
		sys_clk_enable (Uart1_clk_enable);
	}
	else if(uart_dev->dev_id == XM_UART_DEV_2)
	{
		XM_lock ();		// 阻止并发或中断访问设置
		reg = rSYS_PAD_CTRL09;
		if(uart_dev->mode == XM_UART_MODE_TX || uart_dev->mode == XM_UART_MODE_TR)	// TX
		{
			// uarttxd2
			reg &= ~(0x03 << 26);
			reg |=  (0x01 << 26);
		}
		if(uart_dev->mode == XM_UART_MODE_RX || uart_dev->mode == XM_UART_MODE_TR)	// RX
		{
			// uartrxd2
			reg &= ~(0x03 << 24);
			reg |=  (0x01 << 24);
		}		
		rSYS_PAD_CTRL09 = reg;	
		XM_unlock ();
	
		sys_clk_disable (uart2_pclk_enable);
		sys_clk_disable (Uart2_clk_enable);
		
		// IP Reset (4	R/W	1	softreset_uart2)
		XM_lock ();	
		rSYS_SOFT_RSTNB &= ~(1<<4);
		__asm ("nop");
		rSYS_SOFT_RSTNB |=  (1<<4);
		XM_unlock ();
		
		sys_clk_enable (uart2_pclk_enable);
		sys_clk_enable (Uart2_clk_enable);
	}
	else if(uart_dev->dev_id == XM_UART_DEV_3)
	{
		XM_lock ();		// 阻止并发或中断访问设置
		reg = rSYS_PAD_CTRL09;
		if(uart_dev->mode == XM_UART_MODE_TX || uart_dev->mode == XM_UART_MODE_TR)	// TX
		{
			// uarttxd3
			reg &= ~(0x03 << 30);
			reg |=  (0x01 << 30);
		}
		if(uart_dev->mode == XM_UART_MODE_RX || uart_dev->mode == XM_UART_MODE_TR)	// RX
		{
			// uartrxd3
			reg &= ~(0x03 << 28);
			reg |=  (0x01 << 28);
		}		
		rSYS_PAD_CTRL09 = reg;	
		XM_unlock ();
	
		sys_clk_disable (uart3_pclk_enable);
		sys_clk_disable (Uart3_clk_enable);
		
		// IP Reset (3	R/W	1	softreset_uart3)
		XM_lock ();	
		rSYS_SOFT_RSTNB &= ~(1<<3);
		__asm ("nop");
		rSYS_SOFT_RSTNB |=  (1<<3);
		XM_unlock ();
		
		sys_clk_enable (uart3_pclk_enable);
		sys_clk_enable (Uart3_clk_enable);
	}
 	
	// 设置波特率
	// min-baudrate = 24000000 / 4*1024*1024 = 5.72bps
	unsigned int Baud_Rate_Divisor = ((UART_CLK << 3) + uart_dev->baud_rate) / (uart_dev->baud_rate << 1);
	rUART_IBRD(base) = (Baud_Rate_Divisor >> 6) & 0xFFFF;		// 16bit
	rUART_FBRD(base) = Baud_Rate_Divisor & 0x3f;		// 6bit
	
	// Line Control Register
	value = 0x70;		// fifo enable, 8 bits.
	//value = 0x60;		// fifo disable, 8 bits.
	if(uart_dev->stop_bit == 2)	// set two stop bit
		value |= (1 << 3);
	if(uart_dev->parity == XM_UART_PARITY_EVEN)	// 奇校验
		value |= (1 << 1) | (1 << 2);
	else if(uart_dev->parity == XM_UART_PARITY_ODD)	// 偶校验
		value |= (1 << 1);
	rUART_LCR_H(base) = value;
	
	value = 0;
	if(uart_dev->mode == XM_UART_MODE_TX || uart_dev->mode == XM_UART_MODE_TR)
	{
		// 发送
		value |= (1 << 8);	// TXE;
	}
	if(uart_dev->mode == XM_UART_MODE_RX || uart_dev->mode == XM_UART_MODE_TR)
	{
		// 接收
		value |= (1 << 9);	// RXE
	}
	rUART_CR(base) = value;
	
	// UART Interrupt FIFO Level Select Register
	//rUART_IFLS(base) = 0x12;	// RX/TX 1/2 full triggered
	rUART_IFLS(base) = (2 << 0) | (0 << 3);	// Transmit FIFO becomes <= 1/2 full, Receive FIFO becomes >= 1/8 full

	// 中断设置
	// Transfer
	value = 0;
	
#ifndef _UART_TX_DMA_
	if(uart_dev->mode == XM_UART_MODE_TX || uart_dev->mode == XM_UART_MODE_TR)
		value |= (1 << 5);			// Transfer
#endif
	// Receive / Receive Timeout / stop bit error / parity error /  break condition error / receive fifo full
	if(uart_dev->mode == XM_UART_MODE_RX || uart_dev->mode == XM_UART_MODE_TR)
#ifdef _UART_RX_DMA_
		//value |=            (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10);
		value |=            (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10);
#else
		value |= (1 << 4) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10);
#endif
	rUART_IMSC(base) = value;

	// Clear Interrupt
	rUART_ICR(base) = 0x3FF;
	
	// 安装中断向量, 串口中断使能
	if(uart_dev->dev_id == 0)
	{
		uart_dev->intr = UART0_INT;
		uart_dev->tx_dma_req = UART0_UARTTXDMABREQ;
		uart_dev->rx_dma_req = UART0_UARTRXDMABREQ;
		request_irq (UART0_INT, PRIORITY_FIFTEEN, uart0_isr);
	}
	else if(uart_dev->dev_id == 1)
	{
		uart_dev->intr = UART123_INT;
		uart_dev->tx_dma_req = UART1_UARTTXDMABREQ;
		uart_dev->rx_dma_req = UART1_UARTRXDMABREQ;
		request_irq (UART123_INT, PRIORITY_FIFTEEN, uart123_isr);
	}
	else if(uart_dev->dev_id == 2)
	{
		uart_dev->intr = UART123_INT;
		uart_dev->tx_dma_req = UART2_UARTTXDMABREQ;
		uart_dev->rx_dma_req = UART2_UARTRXDMABREQ;
		request_irq (UART123_INT, PRIORITY_FIFTEEN, uart123_isr);
	}
	else if(uart_dev->dev_id == 3)
	{
		uart_dev->intr = UART123_INT;
		uart_dev->tx_dma_req = UART3_UARTTXDMABREQ;
		uart_dev->rx_dma_req = UART3_UARTRXDMABREQ;
		request_irq (UART123_INT, PRIORITY_FIFTEEN, uart123_isr);
	}

	uart_dev->uart_data_register = base;
	uart_dev->uart_dma_control = uart_dev->uart_data_register + 0x48;

#ifdef _UART_TX_DMA_
	if(uart_dev->mode == XM_UART_MODE_TX || uart_dev->mode == XM_UART_MODE_TR)
		*((volatile u32_t *)(uart_dev->uart_dma_control)) |= (1 << 1);	// TXDMAE. If this bit is set to 1, DMA for the transmit FIFO is enabled.
#endif
	
#ifdef _UART_RX_DMA_
	if(uart_dev->mode == XM_UART_MODE_RX || uart_dev->mode == XM_UART_MODE_TR)	
		*((volatile u32_t *)(uart_dev->uart_dma_control)) |= (1 << 0) 	// RXDMAE. If this bit is set to 1, DMA for the receive FIFO is enabled.
																		 	| (1 << 2);	// DMAONERR If this bit is set to 1, the DMA receive request outputs,
																							// UARTRXDMASREQ or UARTRXDMABREQ, are disabled when the UART error interrupt is asserted.
#endif
	// Enable UART
	rUART_CR(base) |= (1 << 0);	
}

static void close_uart ( xm_uart_dev *uart_dev )
{
	u32_t base = uart_base[uart_dev->dev_id];
	if(uart_dev->dev_id == 0)
		request_irq (UART0_INT, PRIORITY_FIFTEEN, NULL);
	else if(uart_dev->dev_id == 1 && uart_dev->dev_id <= 3)
	{
		// request_irq (UART123_INT, PRIORITY_FIFTEEN, NULL);
	}

	// Disable UART
	rUART_CR(base) = 0;
	
	// 禁止中断
	rUART_IMSC(base) = 0;	
}

static volatile unsigned char uart_inited = 0;
void xm_uart_init (void)
{
	int i;
	if(uart_inited)
		return;
	memset (uart_device, 0, sizeof(uart_device));
	for (i = 0; i < XM_UART_DEV_COUNT; i ++)
	{
		OS_CREATERSEMA (&uart_access_sema[i]); /* Creates resource semaphore */
	}
	uart_inited = 1;
}

void xm_uart_exit (void)
{
	int i;
	if(uart_inited == 0)
		return;
	
	for (i = 0; i < XM_UART_DEV_COUNT; i ++)
	{
		OS_DeleteRSema (&uart_access_sema[i]); /* Deletes resource semaphore */
	}
	uart_inited = 0;
}

// 创建一个串口设备
xm_uart_dev_t xm_uart_open (u8_t dev_id, 		// 设备号
									 u8_t stop_bit,	// 停止位长度(1或者2)
									 u8_t parity,		// 奇偶校验设置
									 u8_t mode,			// 收发模式(仅接收/仅发送/或收发)
									 u32_t baud_rate, // 波特率
									 u32_t fifo_size,	// 缓存字节大小. 如果是收发模式, 则分配的缓存是指定fifo_size的2倍
									 int *err_code		// 失败时保存错误代码
									)
{
	xm_uart_dev *dev = NULL;
	int ret = XM_UART_ERRCODE_OK;
	
	if(dev_id >= XM_UART_DEV_COUNT)
	{
		printf ("uart open failed, uart dev_id (%d) illegal\n", dev_id);
		if(err_code)
			*err_code = XM_UART_ERRCODE_ILLEGAL_PARA;
		return NULL;
	}
	
	// 保护互斥访问
	OS_Use (&uart_access_sema[dev_id]);
	
	do
	{
		
		if(stop_bit != XM_UART_STOPBIT_1 && stop_bit != XM_UART_STOPBIT_2)
		{
			printf ("uart open failed, uart stop_bit (%d) illegal\n", stop_bit);
			ret = XM_UART_ERRCODE_ILLEGAL_PARA;
			break;			
		}
		
		if(parity > XM_UART_PARITY_ODD)
		{
			printf ("uart open failed, uart parity (%d) illegal\n", parity);
			ret = XM_UART_ERRCODE_ILLEGAL_PARA;
			break;			
		}
		
		if(mode > XM_UART_MODE_TR)
		{
			printf ("uart open failed, uart mode (%d) illegal\n", mode);
			ret = XM_UART_ERRCODE_ILLEGAL_PARA;
			break;
		}
		
		if(baud_rate < 10 || baud_rate > 921600)
		{
			printf ("uart open failed, uart baud_rate (%d) illegal\n", baud_rate);
			ret = XM_UART_ERRCODE_ILLEGAL_PARA;
			break;
		}
		
		if(fifo_size == 0 || fifo_size > 0x100000)
		{
			printf ("uart open failed, invalid fifo size(%d)\n", fifo_size);
			ret = XM_UART_ERRCODE_ILLEGAL_PARA;
			break;
		}
		
		// 检查设备是否已开启
		if(uart_device[dev_id])
		{
			printf ("uart dev_id (%d) open failed, can't re-open\n", dev_id);
			ret = XM_UART_ERRCODE_DEV_OPENED;
			break;
		}
		
		dev = kernel_malloc (sizeof(xm_uart_dev));
		if(dev == NULL)
		{
			printf ("uart dev_id (%d) open failed, memory busy\n", dev_id);
			ret = XM_UART_ERRCODE_NOMEM;
			break;
		}
		memset (dev, 0, sizeof(xm_uart_dev));
				
		if(mode == XM_UART_MODE_TX || mode == XM_UART_MODE_TR)
		{
			dev->tx_size = fifo_size;
			dev->tx_fifo = kernel_malloc (dev->tx_size);
			if(dev->tx_fifo == NULL)
			{
				ret = XM_UART_ERRCODE_NOMEM;
				break;
			}
		}
		if(mode == XM_UART_MODE_RX || mode == XM_UART_MODE_TR)
		{
			dev->rx_size = fifo_size;
			dev->rx_fifo = kernel_malloc (dev->rx_size);
			if(dev->rx_fifo == NULL)
			{
				ret = XM_UART_ERRCODE_NOMEM;
				break;
			}
		}
		dev->dev_id = dev_id;
		dev->stop_bit = stop_bit;
		dev->parity = parity;
		dev->mode = mode;
		dev->baud_rate = baud_rate;
		
		dev->uart_id = XM_UART_ID_FLAG;
		
		if(mode == XM_UART_MODE_TX || mode == XM_UART_MODE_TR)
		{
			dev->tx_count = 0;
			OS_CreateCSema (&dev->tx_csema, dev->tx_size);
		}
		
		if(mode == XM_UART_MODE_RX || mode == XM_UART_MODE_TR)
		{
			OS_CreateCSema (&dev->rx_csema, 0);
		}
				
		uart_device[dev_id] = dev;
		
		open_uart (dev);
		
	} while (0);
	
	if(ret != XM_UART_ERRCODE_OK)
	{
		// 异常
		if(dev && dev->tx_fifo)
			kernel_free (dev->tx_fifo);
		if(dev && dev->rx_fifo)
			kernel_free (dev->rx_fifo);
		if(dev)
		{
			kernel_free (dev);
			dev = NULL;
		}
	}
	
	if(err_code)
		*err_code = ret;
	
	// 解除互斥访问
	OS_Unuse (&uart_access_sema[dev_id]);
	
	return dev;
}

// 关闭设备
// = 0 成功
// < 0 表示错误码
int	xm_uart_close (xm_uart_dev_t uart_dev)
{
	xm_uart_dev *dev = (xm_uart_dev *)uart_dev;
	u8_t dev_id;
	int ret = XM_UART_ERRCODE_OK;
	
	// 检查设备句柄是否合法
	if(dev == NULL)
	{
		printf ("uart close failed, device handle(0x%8x) illegal\n", dev); 
		return XM_UART_ERRCODE_ILLEGAL_PARA;
	}
	dev_id = dev->dev_id;
	if(dev_id >= XM_UART_DEV_COUNT)
	{
		printf ("uart close failed, device id(%d) illegal\n", dev_id); 
		return XM_UART_ERRCODE_ILLEGAL_HANDLE;
	}
	if(dev->uart_id != XM_UART_ID_FLAG)
	{
		printf ("uart close failed, device handle(0x%8x)'s uart id(0x%8x) illegal\n", dev, dev->uart_id); 
		return XM_UART_ERRCODE_ILLEGAL_HANDLE;
	}
	
	if(dev != uart_device[dev_id])
	{
		printf ("uart close failed, device handle(0x%8x) is illegal device\n", dev); 
		return XM_UART_ERRCODE_ILLEGAL_HANDLE;		
	}
	
	// 资源互斥访问保护
	OS_Use (&uart_access_sema[dev_id]);
		
	// 停止UART设备
	close_uart (dev);
		
	uart_device[dev_id] = 0;

	if(dev->mode == XM_UART_MODE_TX || dev->mode == XM_UART_MODE_TR)
	{
		kernel_free (dev->tx_fifo);
		OS_DeleteCSema (&dev->tx_csema);
	}
	if(dev->mode == XM_UART_MODE_RX || dev->mode == XM_UART_MODE_TR)
	{
		kernel_free (dev->rx_fifo);
		OS_DeleteCSema (&dev->rx_csema);
	}
		
	dev->uart_id = 0;		// 标记无效
		
	kernel_free (dev);
		
	// 资源互斥访问解除
	OS_Unuse (&uart_access_sema[dev_id]);
	return ret;
}


#define UART_TX_DMA_CTL_L 		((0<<28)		/* LLP_SRC_EN */ \
										|(0<<27)		/* LLP_DST_EN */ \
										|(0<<25)		/* SMS */ \
										|(0<<23)		/* DMS */ \
										|(M2P_DMAC<<20)	/* TT_FC */ \
										|(0<<18)		/* DST_SCATTER_EN */ \
										|(0<<17)		/* SRC_GATHER_EN */ \
										|(DMA_BURST_SIZE_1<<14)		/* SRC_MSIZE */ \
										|(DMA_BURST_SIZE_1<<11)		/* DEST_MSIZE */ \
										|(0<<9)		/* SINC, 00 = Increment, 指向发送源缓冲 */ \
										|(2<<7)		/* DINC, 1x = No change, 指向 UART Data Register */ \
										|(DMA_WIDTH_8<<4)		/* SRC_TR_WIDTH, 发送源缓冲按字节对其 */ \
										|(DMA_WIDTH_8<<1)	/* DST_TR_WIDTH */ \
										|(1<<0))		/* INT_EN */
								
#define UART_TX_DMA_CTL_H 		  (0<<12)		/* DONE */

#define UART_TX_DMA_CFG_L		( (0<<31)		/* RELOAD_DST */ \
										| (0<<30)		/* RELOAD_SRC */ \
										| (0<<20)		/* MAX_ABRST, A value of 0 indicates that software is not limiting the maximum */ \
										| (0<<19)		/* SRC_HS_POL */	\
										| (0<<18)		/* DST_HS_POL */	\
										| (0<<17)		/* LOCK_B, */ \
										| (0<<16)		/* LOCK_CH */	\
										| (0<<14)		/* LOCK_B_L, Bus Lock Level */	\
										| (0<<12)		/* LOCK_CH_L, Channel Lock Level. */	\
										| (0<<11)		/* HS_SEL_SRC, 0 = Source Hardware handshaking interface */ \
										| (0<<10)		/* HS_SEL_DST, 0 = Destination Hardware handshaking interface */	\
										| (0<<8)			/* CH_SUSP, 0 = Not suspended. */	\
										| (0<<5))		/* CH_PRIOR, 0 is the lowest priority */


#define UART_TX_DMA_CFG_H		( (0<<7)			/* SRC_PER, this field is ignored */ \
										| (0<<6)			/* SS_UPD_EN, Source Status Update disable*/ \
										| (0<<5)			/* DS_UPD_EN, Destination Status Update disable */ \
										| (0<<2)			/* PROTCTL, Protection Control */ \
										| (1<<1)			/* FIFO_MODE, */	\
										| (0<<0))		/* FCMODE, this field is ignored */

#define UART_RX_DMA_CTL_L 		((0<<28)		/* LLP_SRC_EN */ \
										|(0<<27)		/* LLP_DST_EN */ \
										|(0<<25)		/* SMS */ \
										|(0<<23)		/* DMS */ \
										|(P2M_DMAC<<20)	/* TT_FC */ \
										|(0<<18)		/* DST_SCATTER_EN */ \
										|(0<<17)		/* SRC_GATHER_EN */ \
										|(DMA_BURST_SIZE_1<<14)		/* SRC_MSIZE */ \
										|(DMA_BURST_SIZE_1<<11)		/* DEST_MSIZE */ \
										|(2<<9)		/* SINC, 1x = No change, 指向 UART Data Register */ \
										|(0<<7)		/* DINC, 00 = Increment, 指向发送源缓冲 */ \
										|(DMA_WIDTH_8<<4)		/* SRC_TR_WIDTH, 发送源缓冲按字节对其 */ \
										|(DMA_WIDTH_8<<1)	/* DST_TR_WIDTH */ \
										|(1<<0))		/* INT_EN */
								
#define UART_RX_DMA_CTL_H 		  (0<<12)		/* DONE */

#define UART_RX_DMA_CFG_L		( (0<<31)		/* RELOAD_DST */ \
										| (0<<30)		/* RELOAD_SRC */ \
										| (0<<20)		/* MAX_ABRST, A value of 0 indicates that software is not limiting the maximum */ \
										| (0<<19)		/* SRC_HS_POL */	\
										| (0<<18)		/* DST_HS_POL */	\
										| (0<<17)		/* LOCK_B, */ \
										| (0<<16)		/* LOCK_CH */	\
										| (0<<14)		/* LOCK_B_L, Bus Lock Level */	\
										| (0<<12)		/* LOCK_CH_L, Channel Lock Level. */	\
										| (0<<11)		/* HS_SEL_SRC, 0 = Source Hardware handshaking interface */ \
										| (0<<10)		/* HS_SEL_DST, 0 = Destination Hardware handshaking interface */	\
										| (0<<8)			/* CH_SUSP, 0 = Not suspended. */	\
										| (0<<5))		/* CH_PRIOR, 0 is the lowest priority */


#define UART_RX_DMA_CFG_H		( (0<<12)		/* DEST_PER, this field is ignored */ \
										| (0<<6)			/* SS_UPD_EN, Source Status Update disable*/ \
										| (0<<5)			/* DS_UPD_EN, Destination Status Update disable */ \
										| (0<<2)			/* PROTCTL, Protection Control */ \
										| (1<<1)			/* FIFO_MODE, */	\
										| (0<<0))		/* FCMODE, this field is ignored */

#ifdef _UART_TX_DMA_
static void uart_dma_transfer_over_IRQHandler (UINT32 ulIRQFactors, UINT32 channel, void *channel_private_data)
{
   int ret = 0;
   xm_uart_dev * dev = channel_private_data;
		
	if(ulIRQFactors & ERR_INT_BIT)
	{
		// This interrupt is generated when an ERROR response is received from an AHB slave on the
		// HRESP bus during a DMA transfer. 
		// In addition, the DMA transfer is cancelled and the channel is disabled.
		// DMA 异常
		printf ("ERR_INT_BIT\n");
		ret = XM_UART_ERRCODE_HW_FAULT;
	}
	dma_clr_int (channel);

	// DMA结束
	if(dev)
		dev->err_code = ret;
	// 发送DMA传输结束事件
	Dma_TfrOver_PostSem (channel);
	
}
#endif

// 向串口写入数据(发送)
// timeout == (-1) 表示无限等待
// timeout == 0 表示不等待
// >= 0 成功写入的字节数
// < 0  表示错误码
// 判断成功写入的字节数 是否等于待传输的字节数, 判断是否超时. 不等于表示写入超时
int	xm_uart_write (xm_uart_dev_t uart_dev, u8_t *data, int size, int timeout)
{
#ifndef _UART_TX_DMA_
	u32_t timeout_ticket;
	int ret;
	u32_t next_pos;
#endif
	int byte_writed = 0;
	xm_uart_dev *dev = (xm_uart_dev *)uart_dev;
	
	if(uart_dev == NULL)
	{
		printf ("uart write failed, device handle(0x%8x) illegal\n", dev);
		return XM_UART_ERRCODE_ILLEGAL_PARA;
	}
	if(dev->dev_id >= XM_UART_DEV_COUNT)
	{
		printf ("uart write failed, device id(%d) illegal\n", dev->dev_id); 
		return XM_UART_ERRCODE_ILLEGAL_HANDLE;
	}
	if(dev->uart_id != XM_UART_ID_FLAG)
	{
		printf ("uart write failed, device handle(0x%8x)'s uart id(%d) illegal\n", dev, dev->uart_id); 
		return XM_UART_ERRCODE_ILLEGAL_HANDLE;
	}
	if(dev != uart_device[dev->dev_id])
	{
		printf ("uart write failed, device handle(0x%8x) is illegal device\n", dev); 
		return XM_UART_ERRCODE_ILLEGAL_HANDLE;		
	}
	if(dev->mode == XM_UART_MODE_RX)
	{
		//printf ("uart read failed, device handle(0x%8x) is only rx device\n", dev); 
		return XM_UART_ERRCODE_MODE_UNSUPPORT;		
	}
	if(data == NULL || size == 0)
		return 0;
	
	if(size > 0xFFF)
	{
		printf ("uart write failed, the size(%d) is too large, can't exceed maximum 0xFFF\n", size);
		return XM_UART_ERRCODE_ILLEGAL_PARA;
	}
	
#ifndef _UART_TX_DMA_
	if(timeout >= 0)	// 计算超时的时间点
		timeout_ticket = XM_GetTickCount() + timeout;
#endif
	
	// 阻止其他线程访问设备
	OS_Use (&uart_access_sema[dev->dev_id]);
	
#ifdef _UART_TX_DMA_
	{
		u32_t dma_ch;
		dma_ch = dma_request_dma_channel(1);
		if(dma_ch == ALL_CHANNEL_RUNNING)
		{
			printf("UART (%d) request dma channel failed!\n", dev->dev_id);
			OS_Unuse (&uart_access_sema[dev->dev_id]);
			return XM_UART_ERRCODE_DMA_RESOURCE;
		}
		
   	register_channel_IRQHandler (	dma_ch, 
												uart_dma_transfer_over_IRQHandler, 
												dev,
												(1<<TFR_INT_BIT) | (1<<ERR_INT_BIT)
											);
		
		dma_flush_range ((UINT32)data, ((UINT32)data) + size);
		dma_clr_int (dma_ch);
		dma_cfg_channel (	dma_ch, 
								(UINT32)data, 
								dev->uart_data_register,
								0,
								UART_TX_DMA_CTL_L,
								UART_TX_DMA_CTL_H | size,
								UART_TX_DMA_CFG_L | (dma_ch << 5),		// DMA通道优先级
								(UART_TX_DMA_CFG_H | (dev->tx_dma_req << 12)),
								0,
								0
							);
		
		dev->err_code = 0;
		
		dma_start_channel (dma_ch);
		
		int finished = 0;
		do
		{
			if(timeout < 0)
			{
				// 无限等待,直到完成或异常
				finished = Dma_TfrOver_PendSemEx (dma_ch, 100);
				if(finished == 0)
				{
					if(dev->err_code)
						break;
				}
			}
			else if(timeout == 0)
			{
				finished = Dma_TfrOver_PendSemEx (dma_ch, 0);
				break;
			}
			else
			{
				finished = Dma_TfrOver_PendSemEx (dma_ch, timeout);
				break;
			}
		} while (!finished);
		
		//while(!dma_transfer_over (dma_ch));
		dma_clr_int (dma_ch);
		dma_detect_ch_disable (dma_ch);
		dma_release_channel (dma_ch);
		if(dev->err_code)
			byte_writed = dev->err_code;
		else if(!finished)
			byte_writed = XM_UART_ERRCODE_TIMEOUT;
		else
			byte_writed = size;
	}
#else
	// _UART_UART_PIO_
	while (size > 0)
	{
		// 等待至少一个字节的可用空间
		if(timeout < 0)	// 无限等待传输模式
		{
			OS_WaitCSema (&dev->tx_csema);
		}
		else if(timeout == 0)
		{
			ret = OS_WaitCSemaTimed (&dev->tx_csema, 0);
			if(ret == 0)	// 超时
				break;			
		}
		else	// 超时传输模式
		{
			// 计算剩余可传输时间
			int os_timeout = timeout_ticket - XM_GetTickCount();
			if(os_timeout <= 0)		// 已超时
				break;		
			ret = OS_WaitCSemaTimed (&dev->tx_csema, os_timeout);
			if(ret == 0)	// 超时
				break;
		}
		
		// 至少一个字节可用的空闲单元
		// 复制一个字节的数据到发送缓存
		dev->tx_fifo[dev->tx_tailer] = *data;
		
		// 计算下一个缓存写入的位置
		next_pos = dev->tx_tailer + 1;
		if(next_pos >= dev->tx_size)
			next_pos = 0;
		
		irq_disable (dev->intr);		// 禁止当前uart的中断
		dev->tx_tailer = next_pos;
		dev->tx_count ++;
		irq_enable (dev->intr);		// 允许当前uart的中断
		
		// 每次均触发传输操作
		uart_tx (dev);
		
		data ++;
		size --;
		
		byte_writed ++;
	}
#endif
			
	// 允许其他线程访问设备
	OS_Unuse (&uart_access_sema[dev->dev_id]);
		
	return byte_writed;
}

// 从串口读取数据(接收)
// timeout == (-1) 表示无限等待
// >= 0 成功读出的字节数, 读取的字数数 != size 表示已超时
// < 0  表示错误码
int	xm_uart_read  (xm_uart_dev_t uart_dev, u8_t *data, int size, int timeout)
{
	u32_t timeout_ticket;
	int ret = 0;
	int byte_readed = 0;
	u32_t next_pos;
	xm_uart_dev *dev = (xm_uart_dev *)uart_dev;

	// 检查设备句柄的合法性
	if(dev == NULL)
	{
		printf ("uart read failed, device handle(0x%8x) illegal\n", dev); 
		return XM_UART_ERRCODE_ILLEGAL_PARA;
	}
	if(dev->dev_id >= XM_UART_DEV_COUNT)
	{
		printf ("uart read failed, device id(%d) illegal\n", dev->dev_id); 
		return XM_UART_ERRCODE_ILLEGAL_HANDLE;
	}
	if(dev->uart_id != XM_UART_ID_FLAG)
	{
		printf ("uart read failed, device handle(0x%8x)'s uart id(%d) illegal\n", dev, dev->uart_id); 
		return XM_UART_ERRCODE_ILLEGAL_HANDLE;
	}
	if(dev != uart_device[dev->dev_id])
	{
		printf ("uart read failed, device handle(0x%8x) is illegal device\n", dev); 
		return XM_UART_ERRCODE_ILLEGAL_HANDLE;		
	}
	if(dev->mode == XM_UART_MODE_TX)
	{
		printf ("uart read failed, device handle(0x%8x) is only tx device\n", dev); 
		return XM_UART_ERRCODE_MODE_UNSUPPORT;		
	}
	
	if(data == NULL || size == 0)
		return 0;

	if(timeout >= 0)	// 计算超时的时间点
		timeout_ticket = XM_GetTickCount() + timeout;
	
	// 阻止其他线程访问设备
	OS_Use (&uart_access_sema[dev->dev_id]);
	
	irq_disable (dev->intr);
	if(dev->comm_state & UART_BE)
	{
		dev->comm_state &= ~UART_BE;		// 清除Break Condition
		ret = XM_UART_ERRCODE_BREAK_CONDITION;
	}
	irq_enable (dev->intr);
	if(ret)
	{
		OS_Unuse (&uart_access_sema[dev->dev_id]);
		return ret;
	}
	
#ifdef _UART_RX_DMA_
	{
		u32_t dma_ch;
		dma_ch = dma_request_dma_channel(1);
		if(dma_ch == ALL_CHANNEL_RUNNING)
		{
			printf("UART (%d) request dma channel failed!\n", dev->dev_id);
			OS_Unuse (&uart_access_sema[dev->dev_id]);
			return XM_UART_ERRCODE_DMA_RESOURCE;
		}
		
   	register_channel_IRQHandler (	dma_ch, 
												uart_dma_transfer_over_IRQHandler, 
												dev,
												(1<<TFR_INT_BIT) | (1<<ERR_INT_BIT)
											);
		
		dma_inv_range ((UINT32)data, ((UINT32)data) + size);
		
		dma_clr_int (dma_ch);
		dma_cfg_channel (	dma_ch, 
								dev->uart_data_register,
								(UINT32)data, 
								0,
								UART_RX_DMA_CTL_L,
								UART_RX_DMA_CTL_H | size,
								UART_RX_DMA_CFG_L | (dma_ch << 5),		// DMA通道优先级
								(UART_RX_DMA_CFG_H | (dev->rx_dma_req << 7)),
								0,
								0
							);
		
		dev->err_code = 0;
		
		dma_start_channel (dma_ch);
		
		int finished = 0;
		do
		{
			if(timeout < 0)
			{
				// 无限等待,直到完成或异常
				finished = Dma_TfrOver_PendSemEx (dma_ch, 100);
				if(finished == 0)
				{
					if(dev->err_code)
						break;
				}
			}
			else if(timeout == 0)
			{
				finished = Dma_TfrOver_PendSemEx (dma_ch, 0);
				break;
			}
			else
			{
				finished = Dma_TfrOver_PendSemEx (dma_ch, timeout);
				break;
			}
		} while (!finished);
		
		if(!dev->err_code && !finished)
		{
			// timeout
			dma_suspend_channel (dma_ch);
		}
		
		//while(!dma_transfer_over (dma_ch));
		dma_clr_int (dma_ch);
		dma_detect_ch_disable (dma_ch);
		dma_release_channel (dma_ch);
		
		dma_inv_range ((UINT32)data, ((UINT32)data) + size);
		
		if(dev->err_code)
			byte_readed = dev->err_code;
		else if(!finished)
			byte_readed = XM_UART_ERRCODE_TIMEOUT;
		else
			byte_readed = size;		
	}
	
#else			// _UART_RX_PIO_
	
	while (size > 0)
	{
		// 等待至少一个字节的可用数据
		if(timeout < 0)	// 无限等待接收模式
		{
			OS_WaitCSema (&dev->rx_csema);
		}
		else if(timeout == 0)
		{
			ret = OS_WaitCSemaTimed (&dev->rx_csema, 0);
			if(ret == 0)	// 超时
				break;			
		}
		else	// 超时等待接收模式
		{
			// 计算剩余可接收时间
			int os_timeout = timeout_ticket - XM_GetTickCount();
			if(os_timeout <= 0)		// 已超时
				break;		
			ret = OS_WaitCSemaTimed (&dev->rx_csema, os_timeout);
			if(ret == 0)	// 超时
				break;
		}		
		
		// 至少一个字节的可用数据单元在RX缓存
		// 复制一个字节的数据到读取缓存
		*data = dev->rx_fifo[dev->rx_header];
		
		// 计算下一个可用数据单元读取的位置
		next_pos = dev->rx_header + 1;
		if(next_pos >= dev->rx_size)
			next_pos = 0;
		dev->rx_header = next_pos;

		data ++;
		size --;		
		
		byte_readed ++;
	}
#endif	// _UART_RX_DMA_
	
	// 允许其他线程访问设备
	OS_Unuse (&uart_access_sema[dev->dev_id]);
		
	return byte_readed;
}

// 将缓存中的数据全部清除(接收缓存或发送缓存或收发缓存)
// = 0 成功
// < 0 表示错误码
int	xm_uart_flush (xm_uart_dev_t uart_dev, u8_t flush_mode)
{
	xm_uart_dev *dev = (xm_uart_dev *)uart_dev;
	u32_t base;
	u32_t timeout;
	int ret = XM_UART_ERRCODE_OK;
	
	// 检查设备句柄的合法性
	if(dev == NULL)
	{
		printf ("uart flush failed, device handle(0x%8x) illegal\n", dev); 
		return XM_UART_ERRCODE_ILLEGAL_PARA;
	}
	if(dev->dev_id >= XM_UART_DEV_COUNT)
	{
		printf ("uart flush failed, device id(%d) illegal\n", dev->dev_id); 
		return XM_UART_ERRCODE_ILLEGAL_HANDLE;
	}
	if(dev->uart_id != XM_UART_ID_FLAG)
	{
		printf ("uart flush failed, device handle(0x%8x)'s uart id(%d) illegal\n", dev, dev->uart_id); 
		return XM_UART_ERRCODE_ILLEGAL_HANDLE;
	}
	if(dev != uart_device[dev->dev_id])
	{
		printf ("uart flush failed, device handle(0x%8x) is illegal device\n", dev); 
		return XM_UART_ERRCODE_ILLEGAL_HANDLE;		
	}
	
	// 阻止其他线程访问设备
	OS_Use (&uart_access_sema[dev->dev_id]);
	
	base = uart_base[dev->dev_id];
			
	timeout = XM_GetTickCount() + 200;
	
	do
	{
		if(flush_mode == XM_UART_FLUSH_MODE_TX_FIFO || flush_mode == XM_UART_FLUSH_MODE_TR_FIFO)
		{
			// 清除传输缓存
			irq_disable (dev->intr);		// 禁止当前uart设备的中断
			// 设置TX队列空条件
			OS_SetCSemaValue (&dev->tx_csema, dev->tx_size);
			dev->tx_header = 0;
			dev->tx_tailer = 0;
			dev->tx_count = 0;
			irq_enable (dev->intr);			// 允许当前uart设备的中断
			
			// 等待设备TX_FIFO传输为空
			// 检查TXFE (tx fifo empty)
			//	If the FIFO is disabled, this bit is set when the transmit holding register is empty. 
			//	If the FIFO is enabled, the TXFE bit is set when the transmit FIFO is empty.
			while(!(rUART_FR(base) & (1 << 7)))
			{
				if(XM_GetTickCount() >= timeout)
				{
					ret = XM_UART_ERRCODE_DEVICE_BUSY;
					break;
				}
			}
			if(ret != XM_UART_ERRCODE_OK)
				break;
					
			// 等待UART设备传输空闲
			// 检查BUSY 
			//	(If this bit is set to 1, the UART is busy transmitting data. This bit remains set until the complete byte, 
			//	including all the stop bits, has been sent from the shift register.)
			while((rUART_FR(base) & (1 << 3)))
			{
				if(XM_GetTickCount() >= timeout)
				{
					ret = XM_UART_ERRCODE_DEVICE_BUSY;
					break;
				}				
			}
			if(ret != XM_UART_ERRCODE_OK)
				break;
			
			// 此时UART设备的中断已触发并处理完毕, TX FIFO为空, 传输停止
			// 复位TX逻辑控制
			dev->tx_header = 0;
			dev->tx_tailer = 0;
			dev->tx_count = 0;
			OS_SetCSemaValue (&dev->tx_csema, dev->tx_size);		// 空闲单元的信号量
		}
		
		if(flush_mode == XM_UART_FLUSH_MODE_RX_FIFO || flush_mode == XM_UART_FLUSH_MODE_TR_FIFO)
		{
			// 检查UART设备的RX FIFO. 读出并清空
			irq_disable (dev->intr);		// 禁止当前uart设备的中断
			
			// 等待RX FIFO为空
			// RXFE (rx fifo empty)
			// If the FIFO is disabled, this bit is set when the receive holding register is empty. 
			// If the FIFO is enabled, the RXFE bit is set when the receive FIFO is empty.
			while (!(rUART_FR(base) & (1 << 4)))
			{
				u8_t temp = (u8_t)rUART_DR(base);
				if(XM_GetTickCount() >= timeout)
				{
					ret = XM_UART_ERRCODE_DEVICE_BUSY;
					break;
				}				
			}
			irq_enable (dev->intr);			// 允许当前uart设备的中断
		
			if(ret != XM_UART_ERRCODE_OK)
				break;
			
			// 清除接收缓存
			irq_disable (dev->intr);		// 禁止当前uart设备的中断
			// 设置RX队列空条件
			dev->rx_header = 0;
			dev->rx_tailer = 0;
			OS_SetCSemaValue (&dev->rx_csema, 0);
			irq_enable (dev->intr);			// 允许当前uart设备的中断
		}
		
	} while(0);
	
	// 允许其他线程访问设备
	OS_Unuse (&uart_access_sema[dev->dev_id]);
	
	return ret;
}

// 发送 break condition
// 在TX输出端发送 break condition (a low-level is continually output, 至少2个完整帧周期)
int	xm_uart_break (xm_uart_dev_t uart_dev)
{
	xm_uart_dev *dev = (xm_uart_dev *)uart_dev;
	u32_t base; 
	u32_t cycle_time;
	u32_t timeout;
	int ret = XM_UART_ERRCODE_OK;
	// 检查设备句柄的合法性
	if(dev == NULL)
	{
		printf ("uart break failed, device handle(0x%8x) illegal\n", dev); 
		return XM_UART_ERRCODE_ILLEGAL_PARA;
	}
	if(dev->dev_id >= XM_UART_DEV_COUNT)
	{
		printf ("uart break failed, device id(%d) illegal\n", dev->dev_id); 
		return XM_UART_ERRCODE_ILLEGAL_HANDLE;
	}
	if(dev->uart_id != XM_UART_ID_FLAG)
	{
		printf ("uart break failed, device handle(0x%8x)'s uart id(%d) illegal\n", dev, dev->uart_id); 
		return XM_UART_ERRCODE_ILLEGAL_HANDLE;
	}
	if(dev != uart_device[dev->dev_id])
	{
		printf ("uart break failed, device handle(0x%8x) is illegal device\n", dev); 
		return XM_UART_ERRCODE_ILLEGAL_HANDLE;		
	}
	// 阻止其他线程访问设备
	OS_Use (&uart_access_sema[dev->dev_id]);
	
	timeout = XM_GetTickCount () + 5000;
	while (XM_GetTickCount () < timeout)
	{
		// 等待TX缓存为空
		//int tx_count = dev->tx_size - OS_GetCSemaValue(&dev->tx_csema);
		int tx_count = dev->tx_count;
		if(tx_count)
			continue;
		
		base = uart_base[dev->dev_id];
		// 等待TX-FIFO为空
		if( !(rUART_FR(base) & (1 << 7)) )	// TX fifo non-empty
			continue;

		// 检查传输是否忙
		if(rUART_FR(base) & (1 << 3))		// the UART is busy transmitting data
			continue;		
				
	} while(0);
	
	if(XM_GetTickCount () >= timeout)
	{
		// 设备忙, 超时
		ret = XM_UART_ERRCODE_DEVICE_BUSY;
	}
	else
	{
		// 根据波特率计算保持2个完整信号周期的时间间隔
		cycle_time = 1000 * 13 * 2 / dev->baud_rate;
		if(cycle_time == 0)
			cycle_time = 1;		// 至少1ms
			
		rUART_LCR_H(base) |=  (1 << 0);		// BRK
		XM_Sleep (cycle_time);
		rUART_LCR_H(base) &= ~(1 << 0);
	}
	
	// 允许其他线程访问设备
	OS_Unuse (&uart_access_sema[dev->dev_id]);
	
	return ret;
}

