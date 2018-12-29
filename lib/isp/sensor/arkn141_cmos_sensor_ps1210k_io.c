#include "xm_proj_define.h"
#include "arkn141_isp_sensor_cfg.h"		// sensor配置文件

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_PS1210K
#include "arkn141_isp_cmos_sensor_io.h"
#include <string.h>
#include "i2c.h"
#include "xm_i2c.h"


#define	PS1210K_I2C_TIMEOUT		100


#define	SEN_I2C_ADDR				(0xEE >> 1)

#define	i2c_reg_write		i2c_reg8_write8



void I2C_WRITE (unsigned int addr, unsigned int data)
{
	i2c_reg8_write8(SEN_I2C_ADDR,(addr),(data));	
}

unsigned int I2C_READ (unsigned int addr)
{
	unsigned int data = 0;
	i2c_reg8_read8 (SEN_I2C_ADDR,(addr), &data);
	return data;
}

#if 0
int isp_sensor_config_PP1210K (void)
{
//  int temp;
//  unsigned long count = 0;
/*
  if (isp_sen_id_check() != 0)
  {
    return -1;
  }*/
  
  i2c_reg_write(SEN_I2C_ADDR,0x03,0x00);//W0300// select GROUP A
  i2c_reg_write(SEN_I2C_ADDR,0x24,0x01);//W0300// soft reset
  OS_Delay(1);
  //#################### start up ####################
  i2c_reg_write(SEN_I2C_ADDR,0x03,0x00);//W0300
  i2c_reg_write(SEN_I2C_ADDR,0x29,0x98);//W2998	# output Hi-z release
  
  i2c_reg_write(SEN_I2C_ADDR,0x03,0x00);//W0300
  i2c_reg_write(SEN_I2C_ADDR,0x05,0x00);//W0503	# mirror/flip
  //i2c_reg_write(SEN_I2C_ADDR,0x05,0x02 );//W0503	# mirror/flip
  i2c_reg_write(SEN_I2C_ADDR,0x06,0x08);//W0608	# framewidth_h        (08)
  i2c_reg_write(SEN_I2C_ADDR,0x07,0x97);//W0797	# framewidth_l        (97)
	i2c_reg_write(SEN_I2C_ADDR,0x08,0x04);
	i2c_reg_write(SEN_I2C_ADDR,0x09,0x74);

i2c_reg_write(SEN_I2C_ADDR,0x0c,0x00);//W0C01	# windowx1_h					(00)
i2c_reg_write(SEN_I2C_ADDR,0x0d,0x0d);//W0D4E	# windowx1_l					(01)
i2c_reg_write(SEN_I2C_ADDR,0x0e,0x00);//W0E00	# windowy1_h					(00)
i2c_reg_write(SEN_I2C_ADDR,0x0f,0x0e);//W0FC2   # windowy1_l					(02)
i2c_reg_write(SEN_I2C_ADDR,0x10,0x07);//W1006	# windowx2_h					(07)
i2c_reg_write(SEN_I2C_ADDR,0x11,0x8c);//W114D	# windowx2_l					(8C)
i2c_reg_write(SEN_I2C_ADDR,0x12,0x04);//W1203	# windowy2_h					(04)
i2c_reg_write(SEN_I2C_ADDR,0x13,0x45);//W1391	# windowy2_l					(45)

i2c_reg_write(SEN_I2C_ADDR,0x14,0x00);//W1400	# vsyncstartrow_f0_h	(00)
i2c_reg_write(SEN_I2C_ADDR,0x15,0x19);//W1519	# vsyncstartrow_f0_l	(0D)
i2c_reg_write(SEN_I2C_ADDR,0x15,0x04);//W1604	# vsyncstoprow_f0_h		(04)
i2c_reg_write(SEN_I2C_ADDR,0x17,0x53);//W1753	# vsyncstoprow_f0_l		(53)

i2c_reg_write(SEN_I2C_ADDR,0x25,0x13);//W2510 # CLK DIV1

i2c_reg_write(SEN_I2C_ADDR,0x33,0x01);//W3301 # pixelbias
i2c_reg_write(SEN_I2C_ADDR,0x34,0x02);//W3402 # compbias

i2c_reg_write(SEN_I2C_ADDR,0x36,0xc8);//W36C8 # TX_Bias; DCDC 4.96 V, LDO 4.37 V
i2c_reg_write(SEN_I2C_ADDR,0x38,0x48);//W3848 # black_bias, range_sel 0.4 V
i2c_reg_write(SEN_I2C_ADDR,0x3a,0x22);//W3A22 # main regulator output

//i2c_reg_write(SEN_I2C_ADDR,0x41,0x06);//W4121 # pll_m_cnt (21)
i2c_reg_write(SEN_I2C_ADDR,0x41,0x08);//0x41:08=3.66f/s//W4121 # pll_m_cnt (21)
i2c_reg_write(SEN_I2C_ADDR,0x42,0x01);//W4208 # pll_r_cnt (04)

i2c_reg_write(SEN_I2C_ADDR,0x40,0x10);//W4010 # pll_control
//for(count=0;count<100000;count++);      //$0500	# Delay ------- 500ms here
 OS_Delay(20);                                     
i2c_reg_write(SEN_I2C_ADDR,0x40,0x00);//W4000 # pll_control on

i2c_reg_write(SEN_I2C_ADDR,0x03,0x01);//W0301
i2c_reg_write(SEN_I2C_ADDR,0x26,0x03);//W2603 # blacksun_th_h

i2c_reg_write(SEN_I2C_ADDR,0x03,0x01);//W0301
//i2c_reg_write(SEN_I2C_ADDR,0xc0,0x01);//WC004 # inttime_h  
//i2c_reg_write(SEN_I2C_ADDR,0xc1,0xb8);//WC15F # inttime_m  
//i2c_reg_write(SEN_I2C_ADDR,0xc2,0x00);//WC200 # inttime_l  
i2c_reg_write(SEN_I2C_ADDR,0xc0,0x00);//WC004 # inttime_h  
i2c_reg_write(SEN_I2C_ADDR,0xc1,0x03);//WC15F # inttime_m  
i2c_reg_write(SEN_I2C_ADDR,0xc2,0x60);//WC200 # inttime_l  

i2c_reg_write(SEN_I2C_ADDR,0xc3,0x00);//WC300 # globalgain 
i2c_reg_write(SEN_I2C_ADDR,0xc4,0x40);//WC440 # digitalgain
//for(count=0;count<500000;count++);  
 OS_Delay(20);   
return 0;
}
#endif

static xm_i2c_client_t sensor_device = NULL; 
void isp_i2c_init(void)
{
	int ret;
	
	sensor_device = xm_i2c_open (XM_I2C_DEV_0,
								 SEN_I2C_ADDR,		
								 "PS1210K",
								 XM_I2C_ADDRESSING_7BIT,
								 &ret);
	if(sensor_device == NULL)
	{
		printf("isp sensor(PS1210K)'s i2c inititialize failure\n");
	}
	else
	{
		printf("isp sensor(PS1210K)'s i2c inititialize success\n");
	}
	
}

/*
   函数名：i2c_reg_write
   参数 ：I2C 写入
*/
int i2c_reg8_write8 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int data)
{
	u8_t cmd[2];
	if(sensor_device == NULL)
		return (-1);
	
	cmd[0] = (u8_t)ucDataOffset;
	cmd[1] = data;
	if(xm_i2c_write (sensor_device, cmd, 2, PS1210K_I2C_TIMEOUT) >= 0)
		return 0;
	return (-1);
}

/*
   函数名：i2c_reg_read
   参数 ：I2C 读出
*/
int i2c_reg8_read8 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int *data)
{
	u8_t dat[1];
	if(sensor_device == NULL)
		return (-1);
	if(xm_i2c_write (sensor_device, (u8_t *)&ucDataOffset, 1, PS1210K_I2C_TIMEOUT) < 0)
		return (-1);
	if(xm_i2c_read  (sensor_device, dat, 1, PS1210K_I2C_TIMEOUT) < 0)
		return (-1);
	*data = dat[0];
	return 0;
}


// 初始化CMOS sensor
// 返回值
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_init (void)
{
	// 初始化 PS1210K
	return 0;
}

// 复位CMOS sensor
// 返回值
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_reset (void)
{
	// 复位 PS1210K
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
	return i2c_reg8_read8 (SEN_I2C_ADDR, addr, data);
}


// 写入数据内容到sensor的寄存器
// 返回值
//  0    success
// -1    failure 
int arkn141_isp_cmos_sensor_write_register (u32_t addr, u32_t data)
{
	return i2c_reg8_write8 (SEN_I2C_ADDR, addr, data);
}

#endif	// ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_PS1210K