#include "xm_proj_define.h"
#include "arkn141_isp_sensor_cfg.h"		// sensor�����ļ�

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_JX_H65
#include "arkn141_isp_cmos_sensor_io.h"
#include <string.h>
#include "xm_i2c.h"



#define	SEN_I2C_ADDR				(0x60 >> 1)
//#define	SEN_I2C_ADDR				(0x6c >> 1)
#define	JX_H65_I2C_TIMEOUT		100

#include "xm_i2c.h"


#define	SENSOR_TYPE_NAME		"JX_H65"


static xm_i2c_client_t sensor_device = NULL; 
void isp_i2c_init(void)
{
	int ret = -1;
	if(sensor_device)
	{
		XM_printf ("warning, isp_i2c_init called again\n");
		return;
	}
	sensor_device = xm_i2c_open (XM_I2C_DEV_0,
								 SEN_I2C_ADDR,		
								 SENSOR_TYPE_NAME,
								 XM_I2C_ADDRESSING_7BIT,
								 &ret);
	if(sensor_device == NULL)
	{
		//XM_printf("isp sensor(%s)'s i2c inititialize failure\n", SENSOR_TYPE_NAME);
	}
	else
	{
		
		//XM_printf("isp sensor(%s)'s i2c inititialize success\n", SENSOR_TYPE_NAME);
	}
	if(ret == 0)
		XM_printf("isp sensor(%s)'s i2c inititialize success\n", SENSOR_TYPE_NAME);
	else
		XM_printf("isp sensor(%s)'s i2c inititialize failure\n", SENSOR_TYPE_NAME);
	
}

void isp_i2c_exit(void)
{
	if(sensor_device)
	{
		if(xm_i2c_close (sensor_device))
		{
			XM_printf("isp sensor(%s)'s xm_i2c_close failure\n", SENSOR_TYPE_NAME);
		}
		else
		{
			XM_printf("isp sensor(%s)'s xm_i2c_close success\n", SENSOR_TYPE_NAME);
			sensor_device = NULL;
		}
	}
}

int i2c_reg8_write8 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int data)
{
	u8_t cmd[2];
	if(sensor_device == NULL)
		return (-1);
	
	cmd[0] = (u8_t)(ucDataOffset >> 0);
	cmd[1] = (u8_t)data;
	if(xm_i2c_write (sensor_device, cmd, 2, JX_H65_I2C_TIMEOUT) >= 0)
		return 0;
	else 
		return (-1);
}

int i2c_reg8_read8 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int *data)
{
	u8_t reg[1];
	u8_t dat[1];
	if(sensor_device == NULL)
		return (-1);
	
	reg[0] = (u8_t)(ucDataOffset >> 0);
	if(xm_i2c_write (sensor_device, (u8_t *)reg, 1, JX_H65_I2C_TIMEOUT) < 0)
		return -1;
	
	dat[0] = 0;
	if(xm_i2c_read  (sensor_device, (u8_t *)dat, 1, JX_H65_I2C_TIMEOUT) != 1)
		return -1;
	*data = dat[0];
	return 0;
}

// ��ʼ��CMOS sensor
// ����ֵ
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_init (void)
{
	// ��ʼ�� JX_H65
	return 0;
}

// ��λCMOS sensor
// ����ֵ
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_reset (void)
{
	// ��λ JX_H65
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
	return i2c_reg8_read8 (SEN_I2C_ADDR, addr, data);
}


// д���������ݵ�sensor�ļĴ���
// ����ֵ
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_write_register (u32_t addr, u32_t data)
{
	return i2c_reg8_write8 (SEN_I2C_ADDR, addr, data);
}

#endif	// ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_JX_H65