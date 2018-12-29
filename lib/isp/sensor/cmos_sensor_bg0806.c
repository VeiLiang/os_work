#define SW_VREF
//#ifdef SW_VREF

struct stVrefh{
  unsigned short index;
  unsigned short  th_low;
  unsigned short  th_high;
  unsigned char vrefh;
};

const struct stVrefh stVrefhTable[] =
{
	{5, 263, 267, 0x22},
	{4, 216, 220, 0x1c},
	{3, 176, 180, 0x17},
	{2, 130, 134, 0x12},
	{1, 113, 117, 0x10}
};

static unsigned char pre_index = 0;
#define	SEN_I2C_ADDR				(0x64 >> 1)

static unsigned int I2C_READ (unsigned int addr)
{
	unsigned int data = 0;
	i2c_reg16_read8 (SEN_I2C_ADDR,(addr), &data);
	return data;
}

static void cmos_inttime_gain_update (cmos_inttime_ptr_t p_inttime, cmos_gain_ptr_t gain) 
{
	u16_t exp_time;
	u16_t shutter_sweep_line_count;

	u8_t reg_0x2b,reg_0x67,reg_0x68,reg_0x4f;
	u8_t reg_0x30,reg_0x34,reg_0x4d,reg_0x61;
	u8_t u8DgainHigh, u8DgainLow;
	int register_update = 0;
	
	unsigned int index;
	
	//printf ("ashort=%d\n", p_inttime->exposure_ashort);

	// Register Hold
	if(p_inttime)
	{
		exp_time = (u16_t)p_inttime->exposure_ashort;
	
		arkn141_isp_cmos_sensor_write_register (TEXP_ADDR + 0, (u8_t)(exp_time >> 8) );	
		arkn141_isp_cmos_sensor_write_register (TEXP_ADDR + 1, (u8_t)(exp_time     ) );	
		
		register_update = 1;
		//arkn141_isp_cmos_sensor_write_register (0x001d, 0x02 );	
	}
	
#ifdef SW_VREF
	unsigned long	dark_ave;
	unsigned char  RampGain;
	static unsigned int VrefhPre;
	unsigned char i;
	static unsigned int vrefh_min_tlb = 0x0c;

	RampGain = 128/(VrefhPre+1);
	dark_ave = ((I2C_READ(0x012b)&0xFF)<<8)|(I2C_READ(0x012c)&0xFF);
	
//	printf("dark_ave:%d,vrefh_min_tlb:%x\n",dark_ave,vrefh_min_tlb);
	
	if(dark_ave > 0xfff) 
		dark_ave = 0;
	dark_ave = dark_ave /RampGain;

	if(dark_ave < stVrefhTable[4].th_low)
	{
		pre_index = 0;
		vrefh_min_tlb = 0x0c;
	}
	else
	{	
		for(i=0;i<5;i++)
		{
			if(pre_index < stVrefhTable[i].index && dark_ave > stVrefhTable[i].th_high)
			{
				pre_index = stVrefhTable[i].index;
				vrefh_min_tlb = stVrefhTable[i].vrefh;
				break;
			}
			else if(pre_index >  stVrefhTable[i].index && dark_ave < stVrefhTable[i].th_low)
			{
				pre_index = stVrefhTable[i].index;
				vrefh_min_tlb = stVrefhTable[i].vrefh;	
				break;
			}
		}	
	}	
#endif	
	
	
	if(gain)
	{
		// 128/0x0c = 10.67
		// 128/13 = 9.846
		index = gain->aindex; // (0 ~ 115) <--> (0x0C ~ 0x7F)
		index = 115 - index + 0x0C;
		if (index > 0x7f){index = 0x7f;}		//max 8x dgain
		if (index < 0x0c){index = 0x0c;}
		
		if(index == vrefh_min_tlb){
			reg_0x2b = 0x10;reg_0x30 = 0x01;
			reg_0x34 = 0x01;reg_0x4d = 0x03;
			reg_0x4f = 0x0c;reg_0x61 = 0x02;
			reg_0x67 = 0x00;reg_0x68 = 0x80;
		}
		else{
			reg_0x2b = 0x30;reg_0x30 = 0x00;
			reg_0x34 = 0x00;reg_0x4d = 0x00;
			reg_0x4f = 0x09;reg_0x61 = 0x04;
			reg_0x67 = 0x01;reg_0x68 = 0x90;
		}
		
    	u8DgainHigh = (gain->dindex >> 8) & 0x0f;    
   
	 	u8DgainLow = (gain->dindex & 0xff);
 		
		
		arkn141_isp_cmos_sensor_write_register(0x002b,reg_0x2b);
		arkn141_isp_cmos_sensor_write_register(0x0030,reg_0x30);
		arkn141_isp_cmos_sensor_write_register(0x0034,reg_0x34);
		arkn141_isp_cmos_sensor_write_register(0x004d,reg_0x4d);
		arkn141_isp_cmos_sensor_write_register(0x004f,reg_0x4f);
		arkn141_isp_cmos_sensor_write_register(0x0061,reg_0x61);
		arkn141_isp_cmos_sensor_write_register(0x0067,reg_0x67);
		arkn141_isp_cmos_sensor_write_register(0x0068,reg_0x68);
		
		arkn141_isp_cmos_sensor_write_register(0x00bc,u8DgainHigh);
		arkn141_isp_cmos_sensor_write_register(0x00bd,u8DgainLow);

		arkn141_isp_cmos_sensor_write_register(0x00b1,index);
		//arkn141_isp_cmos_sensor_write_register(0x001d,0x02);
		register_update = 1;
		
#ifdef SW_VREF
		VrefhPre = index;
#endif
	}
	
	if(register_update)
	{
		arkn141_isp_cmos_sensor_write_register(0x001d,0x02);
	}
	return;
}