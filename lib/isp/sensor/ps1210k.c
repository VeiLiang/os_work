#include "xm_proj_define.h"
#include "arkn141_isp_sensor_cfg.h"		// sensor配置文件

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_PS1210K
#include "arkn141_isp_cmos_sensor_io.h"
#include <string.h>
#include "i2c.h"
#include "xm_i2c.h"
#include "gem_isp.h"


#define	PS1210K_I2C_TIMEOUT		100


#define	SEN_I2C_ADDR				(0xEE >> 1)

#define	i2c_reg_write		i2c_reg8_write8
#define	i2c_reg_read		i2c_reg8_read8


#define PP1210K_ID            0x1210 


#ifdef CONFIG_ARKN141_ASIC
int isp_sensor_config_PP1210K_1080P (void)
{  
	unsigned int ppclk_div;
	unsigned int mclk_div;

	unsigned int val;
	
	i2c_reg_write(SEN_I2C_ADDR,0x03,0x00);//W0300// select GROUP A
	i2c_reg_write(SEN_I2C_ADDR,0x24,0x01);//W0300// soft reset
	OS_Delay(1);
	//#################### start up ####################
	i2c_reg_write(SEN_I2C_ADDR,0x03,0x00);//W0300
	i2c_reg_write(SEN_I2C_ADDR,0x29,0x98);//W2998	# output Hi-z release
  
	i2c_reg_write(SEN_I2C_ADDR,0x03,0x00);//W0300
	i2c_reg_write(SEN_I2C_ADDR,0x05,0x00);//W0503	# mirror/flip
	//i2c_reg_write(SEN_I2C_ADDR,0x05,0x02 );//W0503	# mirror/flip
	
	// framerate = 74.25M / (2200 * 1125) = 30fps
	// framewidth = 0x897 + 1 = 2200
	i2c_reg_write(SEN_I2C_ADDR,0x06,0x08);//W0608	# framewidth_h        (08)
	i2c_reg_write(SEN_I2C_ADDR,0x07,0x97);//W0797	# framewidth_l        (97)
	// frameheight = 0x464 + 1 = 1125
	i2c_reg_write(SEN_I2C_ADDR,0x08,0x04);	// fheight_a_h
	//i2c_reg_write(SEN_I2C_ADDR,0x09,0x64); // fheight_a_l
	//i2c_reg_write(SEN_I2C_ADDR,0x09,0x74);	// fheight_a_l, 值=0x74时FPGA ISP Scalar会出现Pop Error
	i2c_reg_write(SEN_I2C_ADDR,0x09,0x84);	// fheight_a_l, 值=0x84时FPGA ISP Scalar不再出现Pop Error

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

	//i2c_reg_write(SEN_I2C_ADDR,0x25,0x13);//W2510 # CLK DIV1
	ppclk_div = 0;	// ppclkdivider 000b : 1/1 001b : 2/3 010b : 1/2  011b : 1/3  100b
	mclk_div = 0;	// clkdivider 00b : 1/1 01b : 1/2 10b : 1/3 11b : 1/4
	val = ppclk_div | (mclk_div << 3);
	i2c_reg_write(SEN_I2C_ADDR,0x25,(unsigned char)val);

	i2c_reg_write(SEN_I2C_ADDR,0x33,0x01);//W3301 # pixelbias
	i2c_reg_write(SEN_I2C_ADDR,0x34,0x02);//W3402 # compbias

	i2c_reg_write(SEN_I2C_ADDR,0x36,0xc8);//W36C8 # TX_Bias; DCDC 4.96 V, LDO 4.37 V
	i2c_reg_write(SEN_I2C_ADDR,0x38,0x48);//W3848 # black_bias, range_sel 0.4 V
	i2c_reg_write(SEN_I2C_ADDR,0x3a,0x22);//W3A22 # main regulator output

	//i2c_reg_write(SEN_I2C_ADDR,0x41,0x06);//W4121 # pll_m_cnt (21)
	// vco = 30000000 * 5 / 2 = 75000000
	//i2c_reg_write(SEN_I2C_ADDR,0x41,0x04);
	//i2c_reg_write(SEN_I2C_ADDR,0x41,0x05);
	i2c_reg_write(SEN_I2C_ADDR,0x41,0x05);
	i2c_reg_write(SEN_I2C_ADDR,0x42,0x02);
	//i2c_reg_write(SEN_I2C_ADDR,0x41,0x08);//0x41:08=3.66f/s//W4121 # pll_m_cnt (21)
	//i2c_reg_write(SEN_I2C_ADDR,0x42,0x01);//W4208 # pll_r_cnt (04)

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
#else
int isp_sensor_config_PP1210K_1080P (void)
{  
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
	i2c_reg_write(SEN_I2C_ADDR,0x08,0x04);	// fheight_a_h
	//i2c_reg_write(SEN_I2C_ADDR,0x09,0x74);	// fheight_a_l, 值=0x74时FPGA ISP Scalar会出现Pop Error
	i2c_reg_write(SEN_I2C_ADDR,0x09,0x84);	// fheight_a_l, 值=0x84时FPGA ISP Scalar不再出现Pop Error

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

int isp_sensor_config_PP1210K_720P (void)
{
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
	i2c_reg_write(SEN_I2C_ADDR,0x07,0x47);//W0797	# framewidth_l        (97)
//  i2c_reg_write(SEN_I2C_ADDR,0x07,0x97);
	i2c_reg_write(SEN_I2C_ADDR,0x08,0x04);
	i2c_reg_write(SEN_I2C_ADDR,0x09,0x74);


	i2c_reg_write(SEN_I2C_ADDR,0x0c,0x01);//W0C01	# windowx1_h					(00)
	i2c_reg_write(SEN_I2C_ADDR,0x0d,0xcc);//W0D4E	# windowx1_l	x1:0x1cc=460(461)	14e=334 (335)	::dif=126		(01)
	i2c_reg_write(SEN_I2C_ADDR,0x0e,0x00);//W0E00	# windowy1_h					(00)
	i2c_reg_write(SEN_I2C_ADDR,0x0f,0xc2);//W0FC2   # windowy1_l	y1:0xc2=194				(02)
	i2c_reg_write(SEN_I2C_ADDR,0x10,0x06);//W1006	# windowx2_h					(07)
	i2c_reg_write(SEN_I2C_ADDR,0x11,0xcb);//W114D	# windowx2_l	x2:0x64d=1613				(8C)
	i2c_reg_write(SEN_I2C_ADDR,0x12,0x03);//W1203	# windowy2_h					(04)
	i2c_reg_write(SEN_I2C_ADDR,0x13,0x91);//W1391	# windowy2_l	y2:0x391=913-1=912				(45)

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
	
	i2c_reg_write(SEN_I2C_ADDR,0x41,0x08);//W4121 # pll_m_cnt (21)
	i2c_reg_write(SEN_I2C_ADDR,0x42,0x01);//W4208 # pll_r_cnt (04)
	
	i2c_reg_write(SEN_I2C_ADDR,0x40,0x10);//W4010 # pll_control
	//for(count=0;count<100000;count++);      //$0500	# Delay ------- 500ms here
	OS_Delay(50);
                                      
	i2c_reg_write(SEN_I2C_ADDR,0x40,0x00);//W4000 # pll_control on
	
	i2c_reg_write(SEN_I2C_ADDR,0x03,0x01);//W0301
	i2c_reg_write(SEN_I2C_ADDR,0x26,0x03);//W2603 # blacksun_th_h
	
	i2c_reg_write(SEN_I2C_ADDR,0x03,0x01);//W0301
	i2c_reg_write(SEN_I2C_ADDR,0xc0,0x01);//WC004 # inttime_h  
	i2c_reg_write(SEN_I2C_ADDR,0xc1,0x50);//WC15F # inttime_m  
	i2c_reg_write(SEN_I2C_ADDR,0xc2,0x00);//WC200 # inttime_l  
	i2c_reg_write(SEN_I2C_ADDR,0xc3,0x00);//WC300 # globalgain 
	i2c_reg_write(SEN_I2C_ADDR,0xc4,0x40);//WC440 # digitalgain
	//for(count=0;count<500000;count++);  
	OS_Delay(20);
	return 0;	
}

int isp_sensor_config_PP1210K (void)
{
	unsigned int id, data0, data1;
	id = 0;
	if(i2c_reg_read (SEN_I2C_ADDR, 0, &data0) < 0)
	{
		printf("i2c read failed, Found Sensor PixPlus1210K NG ...\n");
		return -1;
	}
	if(i2c_reg_read (SEN_I2C_ADDR, 1, &data1) < 0)
	{
		printf("i2c read failed, Found Sensor PixPlus1210K NG ...\n");
		return -1;
	}
	id = (data0 << 8) | data1;
	if (id == PP1210K_ID)
	{
		printf("Found Sensor PixPlus1210K OK ...\n"); 	
	}
	else
	{
		printf("Found Sensor PixPlus1210K NG ...\n");
		return -1;
	}
	
	if(isp_get_video_width() == 1920 && isp_get_video_height() == 1080)
		return isp_sensor_config_PP1210K_1080P ();
	else if(isp_get_video_width() == 1280 && isp_get_video_height() == 720)
		return isp_sensor_config_PP1210K_720P ();
	else
		return -1;
}


#endif	// ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_PS1210K