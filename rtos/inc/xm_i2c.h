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

// I2C�豸ID����
enum {
	XM_I2C_DEV_0 = 0,
	XM_I2C_DEV_COUNT
};

typedef void *	xm_i2c_client_t;				// i2c client�豸���

#define	XM_I2C_ADDRESSING_7BIT		1		// 7bit slave ��ַ
#define	XM_I2C_ADDRESSING_10BIT		2		// 10bit slave ��ַ

// I2C�豸�����붨��
#define	XM_I2C_ERRCODE_OK								0
#define	XM_I2C_ERRCODE_ILLEGAL_PARA				(-1)		// ��Ч�Ĳ���
#define	XM_I2C_ERRCODE_DEV_OPENED					(-2)		// �豸�Ѵ�, (ͬһ�豸��ֹ��δ�)
#define	XM_I2C_ERRCODE_DEV_CLOSED					(-3)		// �豸�ѹر�
#define	XM_I2C_ERRCODE_TIMEOUT						(-4)		// ���ջ��Ͳ�����ʱ
#define	XM_I2C_ERRCODE_NOMEM							(-5)		// �ڴ治��, �豸��ʧ��
#define	XM_I2C_ERRCODE_ILLEGAL_HANDLE				(-6)		// �Ƿ����豸���
#define	XM_I2C_ERRCODE_DEVICE_BUSY					(-7)		// �豸æ
#define	XM_I2C_ERRCODE_ILLEGAL_CLIENT_HANDLE	(-8)		// �Ƿ���client�豸���


// ����һ��I2C slave�豸
// ����ֵ 
//		== 0		�豸��ʧ��, err_code���ش�����
//		!= 0  	i2c slave�豸���
//			  
xm_i2c_client_t  xm_i2c_open (u8_t  i2c_dev_id, 			// i2c�豸��
										u16_t slave_address, 		// slave�豸��ַ
										char *name,						// slave�豸��, ��Ϊ��
										u16_t i2c_addressing_bit, 	// 7bit����10bit��ַģʽ
										int *err_code					// ʧ��ʱ����������
										);

// �ر�I2C�豸
//		client		i2c slave�豸���
// = 0 �ɹ�
// < 0 ��ʾ������ 
int xm_i2c_close (xm_i2c_client_t client);

// ��I2C���豸д������(����)
// 	timeout ����Ϊ��λ�ĳ�ʱʱ��
// ����ֵ
// 	>= 0 �ɹ�д����ֽ���
// 	< 0  ��ʾ������
// �жϳɹ�д����ֽ��� �Ƿ���ڴ�������ֽ���, �ж��Ƿ�ʱ. �����ڱ�ʾд�볬ʱ
int xm_i2c_write (xm_i2c_client_t i2c_client, 	// client�豸���
						u8_t *data, 						// д������ݻ�ַ���ֽڳ���
						int size, 
						int timeout							// ��ʱ����
																// 	<  0, 	��ʾ���޵ȴ�
																//		== 0,		��ʾ���ȴ�
																//		>  0,		��ʾ���ĵȴ�ʱ��(���뵥λ)
						);

// ��i2c client�豸��ȡ����
// 	timeout ����Ϊ��λ�ĳ�ʱʱ��
// ����ֵ
// 	>= 0 �ɹ��������ֽ���, ��ȡ�������� != size ��ʾ�ѳ�ʱ
// 	< 0  ��ʾ������
int	xm_i2c_read  (xm_i2c_client_t i2c_client, 	// client�豸���
						  u8_t *data, 						// ���������ݻ�ַ���ֽڳ���
						  int size, 
						  int timeout						// ��ʱ����
																// 	<  0, 	��ʾ���޵ȴ�
																//		== 0,		��ʾ���ȴ�
																//		>  0,		��ʾ���ĵȴ�ʱ��(���뵥λ)
						);


// ����I2C�豸
int	xm_i2c_device_open (u8_t dev_id);
// �ر�I2C�豸
// dev_id I2C�豸ID
int	xm_i2c_device_close (u8_t dev_id);

// �ײ�ӿ�
void xm_i2c_init (void);


#endif	// #if _XMSYS_I2C_ == _XMSYS_I2C_HARDWARE_	// Ӳ��I2Cģʽ

#endif	// _XM_I2C_DEV_H_