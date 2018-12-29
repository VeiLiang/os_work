#include "xm_proj_define.h"
#include "arkn141_isp_sensor_cfg.h"		// sensor�����ļ�

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_AR0330
#include "arkn141_isp_cmos_sensor_io.h"
#include <string.h>
#include "i2c.h"

extern unsigned int i2c_reg16_write16 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int data);
extern unsigned int i2c_reg16_read16 (unsigned int slvaddr, unsigned int ucDataOffset);


#define	SEN_I2C_ADDR				(0x20 >> 1)

#ifdef _XMSYS_I2C_USE_OLD_METHOD_

#define	I2C_WRITE(addr,data)		i2c_reg_write(SEN_I2C_ADDR,addr,data)
#define	I2C_READ(addr)				i2c_reg_read(SEN_I2C_ADDR,addr)

#else 

#define	I2C_WRITE(addr,data)		i2c_reg16_write16(SEN_I2C_ADDR,(addr),(data))
#define	I2C_READ(addr)				i2c_reg16_read16(SEN_I2C_ADDR,(addr))

#endif

#include "xm_i2c.h"

#define	SENSOR_TYPE_NAME		"AR0330"

#ifndef _XMSYS_I2C_USE_OLD_METHOD_

static xm_i2c_client_t sensor_device = NULL; 
void isp_i2c_init(void)
{
   u32_t ch;
	int ret;
	
	sensor_device = xm_i2c_open (XM_I2C_DEV_0,
								 SEN_I2C_ADDR,		
								 SENSOR_TYPE_NAME,
								 XM_I2C_ADDRESSING_7BIT,
								 &ret);
	if(sensor_device == NULL)
	{
		printf("isp sensor(%s)'s i2c inititialize failure\n", SENSOR_TYPE_NAME);
	}
	else
	{
		printf("isp sensor(%s)'s i2c inititialize success\n", SENSOR_TYPE_NAME);
	}
	//ch = i2c_reg16_read16 (SEN_I2C_ADDR, 1);
	//printf ("ch=0x%x\n", ch);
	
}

#if 0
/*
   ��������i2c_reg_write
   ���� ��I2C д��
*/
unsigned int i2c_reg_write (unsigned int slvaddr, unsigned int ucDataOffset, unsigned char data)
{
	u8_t cmd[2];
	if(sensor_device == NULL)
		return 0;
	
	cmd[0] = (u8_t)ucDataOffset;
	cmd[1] = data;
	xm_i2c_write (sensor_device, 
					  cmd, 
					  2,
					  20);
	return 1;
}

/*
   ��������i2c_reg_read
   ���� ��I2C ����
*/
unsigned char i2c_reg_read (unsigned int slvaddr, unsigned int ucDataOffset)
{
	unsigned char data = 0;
	if(sensor_device == NULL)
		return 0;
	xm_i2c_write (sensor_device, 
					  (u8_t *)&ucDataOffset, 
					  1,
					  20);
	xm_i2c_read  (sensor_device,
					  &data,
					  1, 
					  20);
	return data;
}
#endif

/*
   ��������i2c_reg16_write16
   ���� ��I2C д��
*/
// ucDataOffset 16bit �Ĵ�����ַ
// data 16bit �Ĵ���ֵ
unsigned int i2c_reg16_write16 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int data)
{
	u8_t cmd[4];
	int loop = 4;
	if(sensor_device == NULL)
		return 0;
	
	cmd[0] = (u8_t)(ucDataOffset >> 8);
	cmd[1] = (u8_t)(ucDataOffset >> 0);
	cmd[2] = (u8_t)(data >> 8);
	cmd[3] = (u8_t)(data >> 0);
	
	loop = 1;
	while (loop > 0)
	{
		if( xm_i2c_write (sensor_device, 
					  cmd, 
					  4,
					  200) >= 0)
			break;
		
		printf ("addr=0x%x, 0x%x\n", ucDataOffset, data);
		
		//printf ("rewrite\n");
		loop --;
	}
	return 1;
}

/*
   ��������i2c_reg_read
   ���� ��I2C ����
*/
unsigned int i2c_reg16_read16 (unsigned int slvaddr, unsigned int ucDataOffset)
{
	u8_t reg[2];
	u8_t dat[2];
	if(sensor_device == NULL)
		return 0;
	reg[0] = (u8_t)(ucDataOffset >> 8);
	reg[1] = (u8_t)(ucDataOffset >> 0);
	xm_i2c_write (sensor_device, 
					  (u8_t *)reg, 
					  2,
					  200);
	dat[0] = 0;
	dat[1] = 0;
	xm_i2c_read  (sensor_device,
					  (u8_t *)dat,
					  2, 
					  200);
	return (dat[0] << 8) | (dat[1]);
}
#endif


// ��ʼ��CMOS sensor
// ����ֵ
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_init (void)
{
	// ��ʼ�� PS1210K
	return 0;
}

// ��λCMOS sensor
// ����ֵ
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_reset (void)
{
	// ��λ PS1210K
	return 0;
}

// ��ȡsensor�ļĴ�������
//		��������ֵ������dataָ��ָ��ĵ�ַ. 
//		����ֵ����32λ, ����λ��0���
// ����ֵ
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_read_register  (u32_t addr, u32_t *data)
{
	*data = I2C_READ (addr);
	return 0;
}


// д���������ݵ�sensor�ļĴ���
// ����ֵ
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_write_register (u32_t addr, u32_t data)
{
	I2C_WRITE (addr, data);
	return 0;
}

#endif	// ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_AR0330