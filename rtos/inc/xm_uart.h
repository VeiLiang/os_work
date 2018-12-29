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
#define	XM_UART_ERRCODE_ILLEGAL_PARA		(-1)		// 无效的参数
#define	XM_UART_ERRCODE_DEV_OPENED			(-2)		// 设备已打开, (同一设备禁止多次打开)
#define	XM_UART_ERRCODE_DEV_CLOSED			(-3)		// 设备已关闭
#define	XM_UART_ERRCODE_BREAK_CONDITION	(-4)		// 检测到break condition, 仅在xm_uart_read中设置. 该错误返回后自动清除
																	// a break condition was detected, indicating that the received data input was held LOW for 
																	//		longer than a full-word transmission time (defined as start, data, parity and stop bits).
#define	XM_UART_ERRCODE_RX_FIFO_OVERFLOW	(-5)		// 接收缓存溢出, 数据存在丢失
#define	XM_UART_ERRCODE_TIMEOUT				(-6)		// 接收或发送操作超时
#define	XM_UART_ERRCODE_NOMEM				(-7)		// 内存不够, 设备打开失败
#define	XM_UART_ERRCODE_ILLEGAL_HANDLE	(-8)		// 非法的设备句柄
#define	XM_UART_ERRCODE_DEVICE_BUSY		(-9)		// 设备忙
#define	XM_UART_ERRCODE_MODE_UNSUPPORT	(-10)		// 不支持的读模式或者写模式
#define	XM_UART_ERRCODE_DMA_RESOURCE		(-11)		// DMA资源请求失败
#define	XM_UART_ERRCODE_HW_FAULT			(-12)		// UART硬件错误

// 总共6个UART设备
// dev_id定义
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

// 设备读写模式
#define	XM_UART_MODE_TX			0			// 发送模式
#define	XM_UART_MODE_RX			1			// 接收模式
#define	XM_UART_MODE_TR			2			// 收发模式

// 奇偶校验设置
#define	XM_UART_PARITY_DISABLE	0			// 无校验
#define	XM_UART_PARITY_EVEN		1			// 偶校验
#define	XM_UART_PARITY_ODD		2			// 奇校验

// 停止位设置
#define	XM_UART_STOPBIT_1			1
#define	XM_UART_STOPBIT_2			2		

// RTS/CTS硬件同步机制驱动暂时不支持

// flush mode定义
#define	XM_UART_FLUSH_MODE_TX_FIFO		0			// 清除发送缓存
#define	XM_UART_FLUSH_MODE_RX_FIFO		1			// 清除接收缓存
#define	XM_UART_FLUSH_MODE_TR_FIFO		2			// 清除收发缓存


typedef void *	xm_uart_dev_t;

// 创建一个串口设备
xm_uart_dev_t xm_uart_open (u8_t dev_id, 		// 设备号
									 u8_t stop_bit,	// 停止位长度(1或者2)
									 u8_t parity,		// 奇偶校验设置
									 u8_t mode,			// 收发模式(仅接收/仅发送/或收发)
									 u32_t baud_rate, // 波特率
									 u32_t fifo_size,	// 缓存字节大小. 如果是收发模式, 则分配的缓存是指定fifo_size的2倍
									 int *err_code		// 失败时保存错误代码
									);

// 向串口写入数据(发送)
// timeout 毫秒为单位的超时时间
// timeout == (-1) 表示无限等待
// >= 0 成功写入的字节数
// < 0  表示错误码
int	xm_uart_write (xm_uart_dev_t uart_dev, u8_t *data, int size, int timeout);

// 从串口读取数据(接收)
// timeout 毫秒为单位的超时时间
// timeout == (-1) 表示无限等待
// >= 0 成功读出的字节数, 读取的字数数 != size 表示已超时
// < 0  表示错误码
int	xm_uart_read  (xm_uart_dev_t uart_dev, u8_t *data, int size, int timeout);

// 将缓存中的数据全部清除(接收缓存或发送缓存或收发缓存)
// = 0 成功
// < 0 表示错误码
int	xm_uart_flush (xm_uart_dev_t uart_dev, u8_t flush_mode);

// 在TX输出端发送 break condition (a low-level is continually output, 至少2个完整帧周期)
int	xm_uart_break (xm_uart_dev_t uart_dev);

// 关闭设备
// = 0 成功
// < 0 表示错误码
int	xm_uart_close (xm_uart_dev_t uart_dev);


void xm_uart_init (void);
void xm_uart_exit (void);



#endif	// _XM_UART_DEV_H_