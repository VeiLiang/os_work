//****************************************************************************
//
//	Copyright (C) 2015 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: arkn141_isp_cmos_sensor_io.h
//			io interface API to access the cmos sensor
//
//	Revision history
//
//		2015.08.25	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _ARKN141_ISP_CMOS_SENSOR_IO_H_
#define _ARKN141_ISP_CMOS_SENSOR_IO_H_

#include <xm_type.h>

#if defined (__cplusplus)
	extern "C"{
#endif

// ��ʼ��CMOS sensor
// ����ֵ
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_init (void);

// ��λCMOS sensor
// ����ֵ
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_reset (void);

// ��ȡsensor�ļĴ�������
//		��������ֵ������dataָ��ָ��ĵ�ַ. 
//		����ֵ����32λ, ����λ��0���
// ����ֵ
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_read_register  (u32_t addr, u32_t *data);


// д���������ݵ�sensor�ļĴ���
// ����ֵ
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_write_register (u32_t addr, u32_t data);

// 16λ��ַ, 8λ����ģʽ��i2c��д����
int i2c_reg16_write8 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int data);
int i2c_reg16_read8 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int *data);

// 16λ��ַ, 16λ����ģʽ��i2c��д����
int i2c_reg16_write16 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int data);
int i2c_reg16_read16 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int *data);

// 8λ��ַ, 8λ����ģʽ��i2c��д����
int i2c_reg8_write8 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int data);
int i2c_reg8_read8  (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int *data);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _ARKN141_ISP_CMOS_SENSOR_IO_H_
