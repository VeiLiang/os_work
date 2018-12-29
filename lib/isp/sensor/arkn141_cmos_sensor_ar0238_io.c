#include "xm_proj_define.h"
#include "arkn141_isp_sensor_cfg.h"		// sensor配置文件

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_AR0238 || ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_AR0230
#include "arkn141_isp_cmos_sensor_io.h"
#include <string.h>
#include "xm_i2c.h"



// Two-Wire Serial address select. 0: 0x20. 1: 0x30
#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_AR0238
#define	SEN_I2C_ADDR				(0x20 >> 1)
#elif ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_AR0230
#define	SEN_I2C_ADDR				(0x30 >> 1)
#endif

#define	AR0238_I2C_TIMEOUT		100

#include "xm_i2c.h"


#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_AR0238
#define	SENSOR_TYPE_NAME		"AR0238"
#elif ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_AR0230
#define	SENSOR_TYPE_NAME		"AR0230"
#endif

static int I2C_WRITE (unsigned int addr, unsigned int data)
{
	return i2c_reg16_write16(SEN_I2C_ADDR,(addr),(data));	
}


static xm_i2c_client_t sensor_device = NULL; 
void isp_i2c_init(void)
{
	int ret;
	unsigned int data;
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
		XM_printf("isp sensor(%s)'s i2c inititialize failure\n", SENSOR_TYPE_NAME);
	}
	else
	{
		//XM_printf("isp sensor(%s)'s i2c inititialize success\n", SENSOR_TYPE_NAME);
	}
	
#if 0
	data = 0;
	i2c_reg16_read16 (SEN_I2C_ADDR, 0x3000, &data);
	XM_printf ("0x3000=0x%04x\n", data);
	data = 0;
	i2c_reg16_read16 (SEN_I2C_ADDR, 0x3002, &data);
	XM_printf ("0x3002=0x%04x\n", data);
	data = 0;
	i2c_reg16_read16 (SEN_I2C_ADDR, 0x3004, &data);
	XM_printf ("0x3004=0x%04x\n", data);
	data = 0;
	i2c_reg16_read16 (SEN_I2C_ADDR, 0x3006, &data);
	XM_printf ("0x3006=0x%04x\n", data);
	data = 0;
	i2c_reg16_read16 (SEN_I2C_ADDR, 0x3008, &data);
	XM_printf ("0x3008=0x%04x\n", data);
	
	
	while(1)
	{
		i2c_reg16_write16 (SEN_I2C_ADDR, 0x3002, 0x0020);
		data = 0;
		i2c_reg16_read16 (SEN_I2C_ADDR, 0x3002, &data);
		XM_printf ("0x3002=0x%04x\n", data);
		OS_Delay (100);
		i2c_reg16_write16 (SEN_I2C_ADDR, 0x3002, 0x0040);
		data = 0;
		i2c_reg16_read16 (SEN_I2C_ADDR, 0x3002, &data);
		XM_printf ("0x3002=0x%04x\n", data);
	}
	
	data = 0;
	i2c_reg16_read16 (SEN_I2C_ADDR, 0x301A, &data);
	XM_printf ("0x301A=0x%04x\n", data);
	i2c_reg16_write16 (SEN_I2C_ADDR, 0x301A, 0x0001);
	i2c_reg16_write16 (SEN_I2C_ADDR, 0x301A, 0x10D8);
	data = 0;
	i2c_reg16_read16 (SEN_I2C_ADDR, 0x301A, &data);
	XM_printf ("0x301A=0x%04x\n", data);
	
	if(I2C_WRITE (0x301A, 0x0001) < 0)
		return;
	
	if(I2C_WRITE (0x301A, 0x10D8) < 0)
		return;
#endif
	
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

int i2c_reg16_write16 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int data)
{
	u8_t cmd[4];
	if(sensor_device == NULL)
		return (-1);
	
	cmd[0] = (u8_t)(ucDataOffset >> 8);
	cmd[1] = (u8_t)(ucDataOffset >> 0);
	cmd[2] = (u8_t)(data >> 8);
	cmd[3] = (u8_t)(data >> 0);
	if(xm_i2c_write (sensor_device, cmd, 4, AR0238_I2C_TIMEOUT) >= 0)
		return 0;
	else 
		return (-1);
}

int i2c_reg16_read16 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int *data)
{
	u8_t reg[2];
	u8_t dat[2];
	if(sensor_device == NULL)
		return (-1);
	
	reg[0] = (u8_t)(ucDataOffset >> 8);
	reg[1] = (u8_t)(ucDataOffset >> 0);
	if(xm_i2c_write (sensor_device, (u8_t *)reg, 2, AR0238_I2C_TIMEOUT) < 0)
		return -1;
	
	dat[0] = 0;
	dat[1] = 0;
	if(xm_i2c_read  (sensor_device, (u8_t *)dat, 2, AR0238_I2C_TIMEOUT) != 2)
		return -1;
	*data = (dat[0] << 8) | (dat[1] << 0);
	return 0;
}

// 初始化CMOS sensor
// 返回值
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_init (void)
{
	// 初始化 AR0238
	return 0;
}

// 复位CMOS sensor
// 返回值
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_reset (void)
{
	// 复位 AR0238
	return 0;
}

// 读取sensor的寄存器内容
//		读出的数值保存在data指针指向的地址. 
//		若数值不够32位, 将高位用0填充
// 返回值
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_read_register  (u32_t addr, u32_t *data)
{
	return i2c_reg16_read16 (SEN_I2C_ADDR, addr, data);
}


// 写入数据内容到sensor的寄存器
// 返回值
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_write_register (u32_t addr, u32_t data)
{
	return i2c_reg16_write16 (SEN_I2C_ADDR, addr, data);
}

#endif	// ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_AR0238