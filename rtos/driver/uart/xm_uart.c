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
#define	_UART_TX_DMA_			// DMA��PIO���Ѳ���,OK
#endif

//#define	_UART_RX_DMA_			// NG, 20160308 DMA���Է�������, �޷�����1�ֽڵİ�. ��ȷ��
											//		��ȷ�ϣ� ark��bug��


#define	XM_UART_ID_FLAG		0x54524155		// "UART"

#undef UART_CLK
#define UART_CLK 24000000 //24M

#define UART_OE	(1 << 10)	// Overflow
#define UART_BE	(1 << 9)		// bread condition
#define UART_PE	(1 << 8)		// Parity Error
#define UART_FE	(1 << 7)		// Frame Error

typedef struct _tag_xm_uart_dev {
	u32_t		uart_id;				// UART�豸�ı�־�ֶ�

	u8_t		dev_id;				// �豸��
	u8_t		stop_bit;
	u8_t		parity;
	u8_t		mode;
	u32_t		baud_rate;			// ���ڲ�����
	
	// ��������ݿ�����tx_fifo, �ȴ�����
	u8_t *	tx_fifo;				// tx_buff == NULL �� tx_size == 0��ʾ�رմ��书��
	u32_t		tx_size;
	volatile u32_t		tx_header;	// ���׶���λ��
	volatile u32_t		tx_tailer;	// ��βд��λ��
	volatile u32_t		tx_count;
	OS_CSEMA	tx_csema;			// ���е�Ԫ���ֽڼ����ź���
	
	
	
	u8_t *	rx_fifo;
	u32_t		rx_size;
	volatile u32_t		rx_header;
	volatile u32_t		rx_tailer;
	OS_CSEMA	rx_csema;			// �ѽ��յ��ֽڼ����ź���
	
	u32_t		tx_byte_count;		// �ܹ�������ֽ���
	u32_t		tx_byte_discard;	// �����쳣�������ֽ���
	
	// RXͳ����Ϣ	
	u32_t		rx_byte_count;		// �ܹ����յ��ֽ���
	u32_t		rx_byte_count_overflow_discard;	// ����FIFO����������ֽ���
	u32_t		rx_byte_count_break_discard;		// Break Condition�������ֽ���
	u32_t		rx_byte_count_parity_discard;		// PE (parity error)
	u32_t		rx_byte_count_stopbit_discard;	// FE (stop bit error)
	
	
	int		err_code;			// �豸ͨ���쳣ʱ�Ĵ�����
	u32_t		comm_state;			// ��¼ͨ��״̬, Break Condition
	
	u16_t		intr;					// �жϺ�
	u8_t		tx_dma_req;				// TX DMA�����
	u8_t		rx_dma_req;				// RX DMA�����
	u32_t		uart_dma_control;		// UART DMA Control Register
	u32_t		uart_data_register;		// DMA��д�Ĵ�����ַ
	
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

// �жϼ�����������
static void uart_tx ( xm_uart_dev *uart_dev )
{
	u32_t base = uart_base[uart_dev->dev_id];
	u32_t next_pos;
	irq_disable (uart_dev->intr);		// ��ֹ��ǰuart���ж�
	
	// ��鷢��FIFO�Ƿ�����. ����(Full), ����ֹѭ��
	while ( !(rUART_FR(base) & (1 << 5)) )	// TXFF (tx fifo full)
	{
		// TX-FIFOδ��, ������FIFOд������
		
		// ��鴫�仺���Ƿ�Ϊ��
		int tx_count = uart_dev->tx_count;	//uart_dev->tx_size - OS_GetCSemaValue(&uart_dev->tx_csema);
		if(tx_count == 0)		// û����Ҫ���������
			break;
		
		rUART_DR(base) = uart_dev->tx_fifo[uart_dev->tx_header];
		// �ƶ�����ָ��, ָ����һ��λ��
		next_pos = uart_dev->tx_header + 1;
		if(next_pos >= uart_dev->tx_size)
			next_pos = 0;
		uart_dev->tx_header = next_pos;
		// �����е�Ԫ���ֽڼ����ź��� + 1
		OS_SignalCSema (&uart_dev->tx_csema);
		
		uart_dev->tx_count --;
	}
	
	irq_enable (uart_dev->intr);		// ����ǰuart���ж�
}
#endif

static void uart_rx ( xm_uart_dev *uart_dev )		// �жϵ���
{
	u32_t base = uart_base[uart_dev->dev_id];
	u32_t next_pos;
	int count;
	u32_t data;
	//irq_disable (uart_dev->intr);		// ��ֹ��ǰuart���ж�
	
	// ѭ��ֱ��RX FIFOΪ��
	while ( !(rUART_FR(base) & (1 << 4)) )	// RXFE (rx fifo empty)
	{
		// ��RX FIFO��ȡһ������
		data = rUART_DR(base);
		//printf (" 0x%4x\n", data);
		uart_dev->rx_byte_count ++;
		// ������ݵ���ȷ��
		if(data & (1 << 10))	// BE (break error)
		{
			uart_dev->comm_state |= UART_BE;
			uart_dev->rx_byte_count_break_discard ++;		
			continue;
		}
		else if(data & (1 << 11))	// OE (receive fifo full)
		{
			uart_dev->rx_byte_count_overflow_discard ++;
			// ����ǰ�������ֽ�д�뵽�������� ����ᶪʧ����
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
		// ����û�д���
		// ���RX�����Ƿ�����
		count = OS_GetCSemaValue (&uart_dev->rx_csema);
		if(count >= uart_dev->rx_size)
		{
			// ���, ��������
			uart_dev->rx_byte_count_overflow_discard ++;
			printf ("rx overflow, discard %d\n", uart_dev->rx_byte_count_overflow_discard);
		}
		else
		{
			// ��ӵ�RX������е�β��
			uart_dev->rx_fifo[uart_dev->rx_tailer] = (u8_t)data;
			next_pos = uart_dev->rx_tailer + 1;
			if(next_pos >= uart_dev->rx_size)
				next_pos = 0;
			uart_dev->rx_tailer = next_pos;
			
			// ���������ݵ�Ԫ���ֽڼ����ź��� + 1, ���ѵȴ����߳�
			OS_SignalCSema (&uart_dev->rx_csema);
		}
#endif
	}
	
	//irq_enable (uart_dev->intr);		// ����ǰuart���ж�
}

// dev_id �����жϵ�UART�豸��
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
		// ����ж�
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
	
	// PAD����
	if(uart_dev->dev_id == XM_UART_DEV_0)
	{
		XM_lock ();		// ��ֹ�������жϷ�������
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
		XM_lock ();		// ��ֹ�������жϷ�������
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
		XM_lock ();		// ��ֹ�������жϷ�������
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
		XM_lock ();		// ��ֹ�������жϷ�������
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
 	
	// ���ò�����
	// min-baudrate = 24000000 / 4*1024*1024 = 5.72bps
	unsigned int Baud_Rate_Divisor = ((UART_CLK << 3) + uart_dev->baud_rate) / (uart_dev->baud_rate << 1);
	rUART_IBRD(base) = (Baud_Rate_Divisor >> 6) & 0xFFFF;		// 16bit
	rUART_FBRD(base) = Baud_Rate_Divisor & 0x3f;		// 6bit
	
	// Line Control Register
	value = 0x70;		// fifo enable, 8 bits.
	//value = 0x60;		// fifo disable, 8 bits.
	if(uart_dev->stop_bit == 2)	// set two stop bit
		value |= (1 << 3);
	if(uart_dev->parity == XM_UART_PARITY_EVEN)	// ��У��
		value |= (1 << 1) | (1 << 2);
	else if(uart_dev->parity == XM_UART_PARITY_ODD)	// żУ��
		value |= (1 << 1);
	rUART_LCR_H(base) = value;
	
	value = 0;
	if(uart_dev->mode == XM_UART_MODE_TX || uart_dev->mode == XM_UART_MODE_TR)
	{
		// ����
		value |= (1 << 8);	// TXE;
	}
	if(uart_dev->mode == XM_UART_MODE_RX || uart_dev->mode == XM_UART_MODE_TR)
	{
		// ����
		value |= (1 << 9);	// RXE
	}
	rUART_CR(base) = value;
	
	// UART Interrupt FIFO Level Select Register
	//rUART_IFLS(base) = 0x12;	// RX/TX 1/2 full triggered
	rUART_IFLS(base) = (2 << 0) | (0 << 3);	// Transmit FIFO becomes <= 1/2 full, Receive FIFO becomes >= 1/8 full

	// �ж�����
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
	
	// ��װ�ж�����, �����ж�ʹ��
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
	
	// ��ֹ�ж�
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

// ����һ�������豸
xm_uart_dev_t xm_uart_open (u8_t dev_id, 		// �豸��
									 u8_t stop_bit,	// ֹͣλ����(1����2)
									 u8_t parity,		// ��żУ������
									 u8_t mode,			// �շ�ģʽ(������/������/���շ�)
									 u32_t baud_rate, // ������
									 u32_t fifo_size,	// �����ֽڴ�С. ������շ�ģʽ, �����Ļ�����ָ��fifo_size��2��
									 int *err_code		// ʧ��ʱ����������
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
	
	// �����������
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
		
		// ����豸�Ƿ��ѿ���
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
		// �쳣
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
	
	// ����������
	OS_Unuse (&uart_access_sema[dev_id]);
	
	return dev;
}

// �ر��豸
// = 0 �ɹ�
// < 0 ��ʾ������
int	xm_uart_close (xm_uart_dev_t uart_dev)
{
	xm_uart_dev *dev = (xm_uart_dev *)uart_dev;
	u8_t dev_id;
	int ret = XM_UART_ERRCODE_OK;
	
	// ����豸����Ƿ�Ϸ�
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
	
	// ��Դ������ʱ���
	OS_Use (&uart_access_sema[dev_id]);
		
	// ֹͣUART�豸
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
		
	dev->uart_id = 0;		// �����Ч
		
	kernel_free (dev);
		
	// ��Դ������ʽ��
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
										|(0<<9)		/* SINC, 00 = Increment, ָ����Դ���� */ \
										|(2<<7)		/* DINC, 1x = No change, ָ�� UART Data Register */ \
										|(DMA_WIDTH_8<<4)		/* SRC_TR_WIDTH, ����Դ���尴�ֽڶ��� */ \
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
										|(2<<9)		/* SINC, 1x = No change, ָ�� UART Data Register */ \
										|(0<<7)		/* DINC, 00 = Increment, ָ����Դ���� */ \
										|(DMA_WIDTH_8<<4)		/* SRC_TR_WIDTH, ����Դ���尴�ֽڶ��� */ \
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
		// DMA �쳣
		printf ("ERR_INT_BIT\n");
		ret = XM_UART_ERRCODE_HW_FAULT;
	}
	dma_clr_int (channel);

	// DMA����
	if(dev)
		dev->err_code = ret;
	// ����DMA��������¼�
	Dma_TfrOver_PostSem (channel);
	
}
#endif

// �򴮿�д������(����)
// timeout == (-1) ��ʾ���޵ȴ�
// timeout == 0 ��ʾ���ȴ�
// >= 0 �ɹ�д����ֽ���
// < 0  ��ʾ������
// �жϳɹ�д����ֽ��� �Ƿ���ڴ�������ֽ���, �ж��Ƿ�ʱ. �����ڱ�ʾд�볬ʱ
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
	if(timeout >= 0)	// ���㳬ʱ��ʱ���
		timeout_ticket = XM_GetTickCount() + timeout;
#endif
	
	// ��ֹ�����̷߳����豸
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
								UART_TX_DMA_CFG_L | (dma_ch << 5),		// DMAͨ�����ȼ�
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
				// ���޵ȴ�,ֱ����ɻ��쳣
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
		// �ȴ�����һ���ֽڵĿ��ÿռ�
		if(timeout < 0)	// ���޵ȴ�����ģʽ
		{
			OS_WaitCSema (&dev->tx_csema);
		}
		else if(timeout == 0)
		{
			ret = OS_WaitCSemaTimed (&dev->tx_csema, 0);
			if(ret == 0)	// ��ʱ
				break;			
		}
		else	// ��ʱ����ģʽ
		{
			// ����ʣ��ɴ���ʱ��
			int os_timeout = timeout_ticket - XM_GetTickCount();
			if(os_timeout <= 0)		// �ѳ�ʱ
				break;		
			ret = OS_WaitCSemaTimed (&dev->tx_csema, os_timeout);
			if(ret == 0)	// ��ʱ
				break;
		}
		
		// ����һ���ֽڿ��õĿ��е�Ԫ
		// ����һ���ֽڵ����ݵ����ͻ���
		dev->tx_fifo[dev->tx_tailer] = *data;
		
		// ������һ������д���λ��
		next_pos = dev->tx_tailer + 1;
		if(next_pos >= dev->tx_size)
			next_pos = 0;
		
		irq_disable (dev->intr);		// ��ֹ��ǰuart���ж�
		dev->tx_tailer = next_pos;
		dev->tx_count ++;
		irq_enable (dev->intr);		// ����ǰuart���ж�
		
		// ÿ�ξ������������
		uart_tx (dev);
		
		data ++;
		size --;
		
		byte_writed ++;
	}
#endif
			
	// ���������̷߳����豸
	OS_Unuse (&uart_access_sema[dev->dev_id]);
		
	return byte_writed;
}

// �Ӵ��ڶ�ȡ����(����)
// timeout == (-1) ��ʾ���޵ȴ�
// >= 0 �ɹ��������ֽ���, ��ȡ�������� != size ��ʾ�ѳ�ʱ
// < 0  ��ʾ������
int	xm_uart_read  (xm_uart_dev_t uart_dev, u8_t *data, int size, int timeout)
{
	u32_t timeout_ticket;
	int ret = 0;
	int byte_readed = 0;
	u32_t next_pos;
	xm_uart_dev *dev = (xm_uart_dev *)uart_dev;

	// ����豸����ĺϷ���
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

	if(timeout >= 0)	// ���㳬ʱ��ʱ���
		timeout_ticket = XM_GetTickCount() + timeout;
	
	// ��ֹ�����̷߳����豸
	OS_Use (&uart_access_sema[dev->dev_id]);
	
	irq_disable (dev->intr);
	if(dev->comm_state & UART_BE)
	{
		dev->comm_state &= ~UART_BE;		// ���Break Condition
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
								UART_RX_DMA_CFG_L | (dma_ch << 5),		// DMAͨ�����ȼ�
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
				// ���޵ȴ�,ֱ����ɻ��쳣
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
		// �ȴ�����һ���ֽڵĿ�������
		if(timeout < 0)	// ���޵ȴ�����ģʽ
		{
			OS_WaitCSema (&dev->rx_csema);
		}
		else if(timeout == 0)
		{
			ret = OS_WaitCSemaTimed (&dev->rx_csema, 0);
			if(ret == 0)	// ��ʱ
				break;			
		}
		else	// ��ʱ�ȴ�����ģʽ
		{
			// ����ʣ��ɽ���ʱ��
			int os_timeout = timeout_ticket - XM_GetTickCount();
			if(os_timeout <= 0)		// �ѳ�ʱ
				break;		
			ret = OS_WaitCSemaTimed (&dev->rx_csema, os_timeout);
			if(ret == 0)	// ��ʱ
				break;
		}		
		
		// ����һ���ֽڵĿ������ݵ�Ԫ��RX����
		// ����һ���ֽڵ����ݵ���ȡ����
		*data = dev->rx_fifo[dev->rx_header];
		
		// ������һ���������ݵ�Ԫ��ȡ��λ��
		next_pos = dev->rx_header + 1;
		if(next_pos >= dev->rx_size)
			next_pos = 0;
		dev->rx_header = next_pos;

		data ++;
		size --;		
		
		byte_readed ++;
	}
#endif	// _UART_RX_DMA_
	
	// ���������̷߳����豸
	OS_Unuse (&uart_access_sema[dev->dev_id]);
		
	return byte_readed;
}

// �������е�����ȫ�����(���ջ�����ͻ�����շ�����)
// = 0 �ɹ�
// < 0 ��ʾ������
int	xm_uart_flush (xm_uart_dev_t uart_dev, u8_t flush_mode)
{
	xm_uart_dev *dev = (xm_uart_dev *)uart_dev;
	u32_t base;
	u32_t timeout;
	int ret = XM_UART_ERRCODE_OK;
	
	// ����豸����ĺϷ���
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
	
	// ��ֹ�����̷߳����豸
	OS_Use (&uart_access_sema[dev->dev_id]);
	
	base = uart_base[dev->dev_id];
			
	timeout = XM_GetTickCount() + 200;
	
	do
	{
		if(flush_mode == XM_UART_FLUSH_MODE_TX_FIFO || flush_mode == XM_UART_FLUSH_MODE_TR_FIFO)
		{
			// ������仺��
			irq_disable (dev->intr);		// ��ֹ��ǰuart�豸���ж�
			// ����TX���п�����
			OS_SetCSemaValue (&dev->tx_csema, dev->tx_size);
			dev->tx_header = 0;
			dev->tx_tailer = 0;
			dev->tx_count = 0;
			irq_enable (dev->intr);			// ����ǰuart�豸���ж�
			
			// �ȴ��豸TX_FIFO����Ϊ��
			// ���TXFE (tx fifo empty)
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
					
			// �ȴ�UART�豸�������
			// ���BUSY 
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
			
			// ��ʱUART�豸���ж��Ѵ������������, TX FIFOΪ��, ����ֹͣ
			// ��λTX�߼�����
			dev->tx_header = 0;
			dev->tx_tailer = 0;
			dev->tx_count = 0;
			OS_SetCSemaValue (&dev->tx_csema, dev->tx_size);		// ���е�Ԫ���ź���
		}
		
		if(flush_mode == XM_UART_FLUSH_MODE_RX_FIFO || flush_mode == XM_UART_FLUSH_MODE_TR_FIFO)
		{
			// ���UART�豸��RX FIFO. ���������
			irq_disable (dev->intr);		// ��ֹ��ǰuart�豸���ж�
			
			// �ȴ�RX FIFOΪ��
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
			irq_enable (dev->intr);			// ����ǰuart�豸���ж�
		
			if(ret != XM_UART_ERRCODE_OK)
				break;
			
			// ������ջ���
			irq_disable (dev->intr);		// ��ֹ��ǰuart�豸���ж�
			// ����RX���п�����
			dev->rx_header = 0;
			dev->rx_tailer = 0;
			OS_SetCSemaValue (&dev->rx_csema, 0);
			irq_enable (dev->intr);			// ����ǰuart�豸���ж�
		}
		
	} while(0);
	
	// ���������̷߳����豸
	OS_Unuse (&uart_access_sema[dev->dev_id]);
	
	return ret;
}

// ���� break condition
// ��TX����˷��� break condition (a low-level is continually output, ����2������֡����)
int	xm_uart_break (xm_uart_dev_t uart_dev)
{
	xm_uart_dev *dev = (xm_uart_dev *)uart_dev;
	u32_t base; 
	u32_t cycle_time;
	u32_t timeout;
	int ret = XM_UART_ERRCODE_OK;
	// ����豸����ĺϷ���
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
	// ��ֹ�����̷߳����豸
	OS_Use (&uart_access_sema[dev->dev_id]);
	
	timeout = XM_GetTickCount () + 5000;
	while (XM_GetTickCount () < timeout)
	{
		// �ȴ�TX����Ϊ��
		//int tx_count = dev->tx_size - OS_GetCSemaValue(&dev->tx_csema);
		int tx_count = dev->tx_count;
		if(tx_count)
			continue;
		
		base = uart_base[dev->dev_id];
		// �ȴ�TX-FIFOΪ��
		if( !(rUART_FR(base) & (1 << 7)) )	// TX fifo non-empty
			continue;

		// ��鴫���Ƿ�æ
		if(rUART_FR(base) & (1 << 3))		// the UART is busy transmitting data
			continue;		
				
	} while(0);
	
	if(XM_GetTickCount () >= timeout)
	{
		// �豸æ, ��ʱ
		ret = XM_UART_ERRCODE_DEVICE_BUSY;
	}
	else
	{
		// ���ݲ����ʼ��㱣��2�������ź����ڵ�ʱ����
		cycle_time = 1000 * 13 * 2 / dev->baud_rate;
		if(cycle_time == 0)
			cycle_time = 1;		// ����1ms
			
		rUART_LCR_H(base) |=  (1 << 0);		// BRK
		XM_Sleep (cycle_time);
		rUART_LCR_H(base) &= ~(1 << 0);
	}
	
	// ���������̷߳����豸
	OS_Unuse (&uart_access_sema[dev->dev_id]);
	
	return ret;
}

