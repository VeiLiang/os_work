#include "xm_proj_define.h"
#include "arkn141_isp_sensor_cfg.h"		// sensor配置文件

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_BG0806
#include "arkn141_isp_cmos_sensor_io.h"
#include <string.h>
#include "xm_i2c.h"
#include "xm_printf.h"

// BG0806 通过I2C 总线对外通信，对应的端口为SDA 和SCL。
// 从机读地址为0x65，从机写地址为0x64。
// 总线采用16 位的地址，8 位数据的组织方式。

#define	SEN_I2C_ADDR				(0x64 >> 1)
#define	BG0806_I2C_TIMEOUT		100

#include "xm_i2c.h"


#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_BG0806
#define	SENSOR_TYPE_NAME		"BG0806"
#endif


static xm_i2c_client_t sensor_device = NULL; 
void isp_i2c_init(void)
{
	int ret;
	
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
		XM_printf("isp sensor(%s)'s i2c inititialize success\n", SENSOR_TYPE_NAME);
	}	
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


int i2c_reg16_write8 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int data)
{
	u8_t cmd[3];
	if(sensor_device == NULL)
		return (-1);
	
	cmd[0] = (u8_t)(ucDataOffset >> 8);
	cmd[1] = (u8_t)(ucDataOffset >> 0);
	cmd[2] = (u8_t)data;
	if(xm_i2c_write (sensor_device, cmd, 3, BG0806_I2C_TIMEOUT) >= 0)
	{
	       //   XM_printf("i2c_write8 ok ,reg is %x,data is %x\n",ucDataOffset,data);
	   
		return 0;
	}
	else 
	{
	     // XM_printf("i2c_write8 error!!! ,reg is %x,data is %x\n",ucDataOffset,data);
		    return (-1);
	}
}

int i2c_reg16_read8 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int *data)
{
	u8_t reg[2];
	u8_t dat[1];
	if(sensor_device == NULL)
		return (-1);
	
	reg[0] = (u8_t)(ucDataOffset >> 8);
	reg[1] = (u8_t)(ucDataOffset >> 0);
	if(xm_i2c_write (sensor_device, (u8_t *)reg, 2, BG0806_I2C_TIMEOUT) < 0)
		return -1;
	
	dat[0] = 0;
	if(xm_i2c_read  (sensor_device, (u8_t *)dat, 1, BG0806_I2C_TIMEOUT) != 1)
		return -1;
	*data = dat[0];
	return 0;
}

// 初始化CMOS sensor
// 返回值
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_init (void)
{
	// 初始化 BG0806
	return 0;
}

// 复位CMOS sensor
// 返回值
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_reset (void)
{
	// 复位 BG0806
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
	return i2c_reg16_read8 (SEN_I2C_ADDR, addr, data);
}


// 写入数据内容到sensor的寄存器
// 返回值
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_write_register (u32_t addr, u32_t data)
{
	return i2c_reg16_write8 (SEN_I2C_ADDR, addr, data);
}

#endif	// ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_BG0806