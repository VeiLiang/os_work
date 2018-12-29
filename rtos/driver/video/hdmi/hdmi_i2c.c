

#include "xm_proj_define.h"
#include <string.h>
//#include "i2c.h"
#include "xm_i2c.h"

//#define	SEN_I2C_ADDR	0x9A		// PCADR high
#define	SEN_I2C_ADDR	(0x98>>1)		// PCADR pulled low

#define	IT66121FN_I2C_TIMEOUT		100

#if 0
static xm_i2c_client_t hdmi_device = NULL; 
void hdmi_i2c_init(void)
{
	int ret;
	
	hdmi_device = xm_i2c_open (XM_I2C_DEV_0,
								 SEN_I2C_ADDR,		
								 "IT66121FN",
								 XM_I2C_ADDRESSING_7BIT,
								 &ret);
	if(hdmi_device == NULL)
	{
		printf("isp sensor(IT66121FN)'s i2c inititialize failure\n");
	}
	else
	{
		printf("isp sensor(IT66121FN)'s i2c inititialize success\n");
	}
}

void hdmi_i2c_exit (void)
{
	if(hdmi_device)
	{
		xm_i2c_close (hdmi_device);
		hdmi_device = NULL;
	}
}

/*
   函数名：i2c_reg_write
   参数 ：I2C 写入
*/
int i2c_reg8_write (unsigned int slvaddr, unsigned int ucDataOffset, unsigned char * data, unsigned char size)
{
	unsigned char cmd[512];
	if(hdmi_device == NULL)
		return (-1);
	
	cmd[0] = (u8_t)ucDataOffset;
	memcpy (cmd + 1, data, size);
	if(xm_i2c_write (hdmi_device, cmd, size + 1, IT66121FN_I2C_TIMEOUT) >= 0)
		return 0;
	return (-1);
}

/*
   函数名：i2c_reg_read
   参数 ：I2C 读出
*/
int i2c_reg8_read (unsigned int slvaddr, unsigned int ucDataOffset, unsigned char *data,unsigned char size)
{
	if(hdmi_device == NULL)
		return (-1);
	if(xm_i2c_write (hdmi_device, (u8_t *)&ucDataOffset, 1, IT66121FN_I2C_TIMEOUT) < 0)
		return (-1);
	if(xm_i2c_read  (hdmi_device, data, size, IT66121FN_I2C_TIMEOUT) < 0)
		return (-1);
	return 0;
}
#else

int i2c_reg_read_bytes  (unsigned int slvaddr, unsigned int ucDataOffset, unsigned char *data, unsigned char size);
int i2c_reg_write_bytes (unsigned int slvaddr, unsigned int ucDataOffset, unsigned char* data, unsigned char size);


void hdmi_i2c_init(void)
{
	// 避免Nand引导方式的影响, 将其复位
	sys_soft_reset (softreset_nand);
	GPIO_I2C_Init();
}

void hdmi_i2c_exit (void)
{
}

/*
   函数名：i2c_reg_write
   参数 ：I2C 写入
*/
int i2c_reg8_write (unsigned int slvaddr, unsigned int ucDataOffset, unsigned char * data, unsigned char size)
{
	i2c_reg_write_bytes (SEN_I2C_ADDR, ucDataOffset, data, size);
	return 0;
}

/*
   函数名：i2c_reg_read
   参数 ：I2C 读出
*/
int i2c_reg8_read (unsigned int slvaddr, unsigned int ucDataOffset, unsigned char *data,unsigned char size)
{
	
	i2c_reg_read_bytes (SEN_I2C_ADDR, ucDataOffset, data, size);
	return 0;
}

#endif
