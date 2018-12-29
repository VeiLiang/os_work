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
#ifndef _XM_UART_DEV_H_
#define _XM_UART_DEV_H_

#define	XM_UART_ERRCODE_OK					0
#define	XM_UART_ERRCODE_ILLEGAL_PARA		(-1)		// ��Ч�Ĳ���
#define	XM_UART_ERRCODE_DEV_OPENED			(-2)		// �豸�Ѵ�, (ͬһ�豸��ֹ��δ�)
#define	XM_UART_ERRCODE_DEV_CLOSED			(-3)		// �豸�ѹر�
#define	XM_UART_ERRCODE_BREAK_CONDITION	(-4)		// ��⵽break condition, ����xm_uart_read������. �ô��󷵻غ��Զ����
																	// a break condition was detected, indicating that the received data input was held LOW for 
																	//		longer than a full-word transmission time (defined as start, data, parity and stop bits).
#define	XM_UART_ERRCODE_RX_FIFO_OVERFLOW	(-5)		// ���ջ������, ���ݴ��ڶ�ʧ
#define	XM_UART_ERRCODE_TIMEOUT				(-6)		// ���ջ��Ͳ�����ʱ
#define	XM_UART_ERRCODE_NOMEM				(-7)		// �ڴ治��, �豸��ʧ��
#define	XM_UART_ERRCODE_ILLEGAL_HANDLE	(-8)		// �Ƿ����豸���
#define	XM_UART_ERRCODE_DEVICE_BUSY		(-9)		// �豸æ
#define	XM_UART_ERRCODE_MODE_UNSUPPORT	(-10)		// ��֧�ֵĶ�ģʽ����дģʽ
#define	XM_UART_ERRCODE_DMA_RESOURCE		(-11)		// DMA��Դ����ʧ��
#define	XM_UART_ERRCODE_HW_FAULT			(-12)		// UARTӲ������

// �ܹ�6��UART�豸
// dev_id����
enum {
	XM_UART_DEV_0 = 0,
	XM_UART_DEV_1,
	XM_UART_DEV_2,
	XM_UART_DEV_3,
	XM_UART_DEV_4,
	XM_UART_DEV_5,
	XM_UART_DEV_6,
	XM_UART_DEV_7,
	XM_UART_DEV_COUNT
};

// �豸��дģʽ
#define	XM_UART_MODE_TX			0			// ����ģʽ
#define	XM_UART_MODE_RX			1			// ����ģʽ
#define	XM_UART_MODE_TR			2			// �շ�ģʽ

// ��żУ������
#define	XM_UART_PARITY_DISABLE	0			// ��У��
#define	XM_UART_PARITY_EVEN		1			// żУ��
#define	XM_UART_PARITY_ODD		2			// ��У��

// ֹͣλ����
#define	XM_UART_STOPBIT_1			1
#define	XM_UART_STOPBIT_2			2		

// RTS/CTSӲ��ͬ������������ʱ��֧��

// flush mode����
#define	XM_UART_FLUSH_MODE_TX_FIFO		0			// ������ͻ���
#define	XM_UART_FLUSH_MODE_RX_FIFO		1			// ������ջ���
#define	XM_UART_FLUSH_MODE_TR_FIFO		2			// ����շ�����


typedef void *	xm_uart_dev_t;

// ����һ�������豸
xm_uart_dev_t xm_uart_open (u8_t dev_id, 		// �豸��
									 u8_t stop_bit,	// ֹͣλ����(1����2)
									 u8_t parity,		// ��żУ������
									 u8_t mode,			// �շ�ģʽ(������/������/���շ�)
									 u32_t baud_rate, // ������
									 u32_t fifo_size,	// �����ֽڴ�С. ������շ�ģʽ, �����Ļ�����ָ��fifo_size��2��
									 int *err_code		// ʧ��ʱ����������
									);

// �򴮿�д������(����)
// timeout ����Ϊ��λ�ĳ�ʱʱ��
// timeout == (-1) ��ʾ���޵ȴ�
// >= 0 �ɹ�д����ֽ���
// < 0  ��ʾ������
int	xm_uart_write (xm_uart_dev_t uart_dev, u8_t *data, int size, int timeout);

// �Ӵ��ڶ�ȡ����(����)
// timeout ����Ϊ��λ�ĳ�ʱʱ��
// timeout == (-1) ��ʾ���޵ȴ�
// >= 0 �ɹ��������ֽ���, ��ȡ�������� != size ��ʾ�ѳ�ʱ
// < 0  ��ʾ������
int	xm_uart_read  (xm_uart_dev_t uart_dev, u8_t *data, int size, int timeout);

// �������е�����ȫ�����(���ջ�����ͻ�����շ�����)
// = 0 �ɹ�
// < 0 ��ʾ������
int	xm_uart_flush (xm_uart_dev_t uart_dev, u8_t flush_mode);

// ��TX����˷��� break condition (a low-level is continually output, ����2������֡����)
int	xm_uart_break (xm_uart_dev_t uart_dev);

// �ر��豸
// = 0 �ɹ�
// < 0 ��ʾ������
int	xm_uart_close (xm_uart_dev_t uart_dev);


void xm_uart_init (void);
void xm_uart_exit (void);



#endif	// _XM_UART_DEV_H_