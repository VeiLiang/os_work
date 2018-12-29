//****************************************************************************
//
//	Copyright (C) 2012~2016 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_i2c.h
//	  i2c master device driver
//
//	Revision history
//
//		2015.12.02	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_I2C_DEV_H_
#define _XM_I2C_DEV_H_

#include "hardware.h"

#if _XMSYS_I2C_ == _XMSYS_I2C_HARDWARE_

// I2C设备ID定义
enum {
	XM_I2C_DEV_0 = 0,
	XM_I2C_DEV_COUNT
};

typedef void *	xm_i2c_client_t;				// i2c client设备句柄

#define	XM_I2C_ADDRESSING_7BIT		1		// 7bit slave 地址
#define	XM_I2C_ADDRESSING_10BIT		2		// 10bit slave 地址

// I2C设备错误码定义
#define	XM_I2C_ERRCODE_OK								0
#define	XM_I2C_ERRCODE_ILLEGAL_PARA				(-1)		// 无效的参数
#define	XM_I2C_ERRCODE_DEV_OPENED					(-2)		// 设备已打开, (同一设备禁止多次打开)
#define	XM_I2C_ERRCODE_DEV_CLOSED					(-3)		// 设备已关闭
#define	XM_I2C_ERRCODE_TIMEOUT						(-4)		// 接收或发送操作超时
#define	XM_I2C_ERRCODE_NOMEM							(-5)		// 内存不够, 设备打开失败
#define	XM_I2C_ERRCODE_ILLEGAL_HANDLE				(-6)		// 非法的设备句柄
#define	XM_I2C_ERRCODE_DEVICE_BUSY					(-7)		// 设备忙
#define	XM_I2C_ERRCODE_ILLEGAL_CLIENT_HANDLE	(-8)		// 非法的client设备句柄


// 创建一个I2C slave设备
// 返回值 
//		== 0		设备打开失败, err_code返回错误码
//		!= 0  	i2c slave设备句柄
//			  
xm_i2c_client_t  xm_i2c_open (u8_t  i2c_dev_id, 			// i2c设备号
										u16_t slave_address, 		// slave设备地址
										char *name,						// slave设备名, 可为空
										u16_t i2c_addressing_bit, 	// 7bit或者10bit地址模式
										int *err_code					// 失败时保存错误代码
										);

// 关闭I2C设备
//		client		i2c slave设备句柄
// = 0 成功
// < 0 表示错误码 
int xm_i2c_close (xm_i2c_client_t client);

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
						);

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
						);


// 开启I2C设备
int	xm_i2c_device_open (u8_t dev_id);
// 关闭I2C设备
// dev_id I2C设备ID
int	xm_i2c_device_close (u8_t dev_id);

// 底层接口
void xm_i2c_init (void);


#endif	// #if _XMSYS_I2C_ == _XMSYS_I2C_HARDWARE_	// 硬件I2C模式

#endif	// _XM_I2C_DEV_H_