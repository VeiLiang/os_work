#include "mir3da.h"

#define abs(x) (((x) < 0) ? -(x) : (x))
#define IDC_SADDR_G_Sensor_W 0x4e
#define IDC_SADDR_G_Sensor_R 0x4f
#define IDC_STYPE_G_Sensor   IDC2

/*return value: 0: is ok    other: is failed*/
int     i2c_read_byte_data( unsigned char addr, unsigned char *data){
	
		int ret = 0;
		short pdata = 0;

		 idc_read (IDC_STYPE_G_Sensor,IDC_SADDR_G_Sensor_R, addr, &pdata, 1);
		
		*data = (unsigned char)pdata;
		
		return ret;
}

/*return value: 0: is ok    other: is failed*/
int     i2c_write_byte_data( unsigned char addr, unsigned char data){
		
		int ret = 0;
		
		idc_write (IDC_STYPE_G_Sensor,IDC_SADDR_G_Sensor_W,addr ,&data,1);
	
		return 0;
}

/*return value: 0: is count    other: is failed*/
int     i2c_read_block_data( unsigned char base_addr, unsigned char count, unsigned char *data){
	int i = 0;
		
	for(i = 0; i < count;i++)
	{
				if(i2c_read_byte_data(base_addr+i,(data+i)))
				{
					return -1;		
				}
	}	
		
	return count;
}

int mir3da_register_read( unsigned char addr, unsigned char *data){
    int     res = 0;

    res = i2c_read_byte_data(addr, data);
    if(res != 0) {
          return res;
    }	

    return res;
}

int mir3da_register_write( unsigned char addr, unsigned char data){
    int     res = 0;

    res = i2c_write_byte_data(addr, data);
    if(res != 0) {
         return res;
    }

    return res;
}

int mir3da_register_read_continuously( unsigned char addr, unsigned char count, unsigned char *data)
{
    int     res = 0;

    res = (count==i2c_read_block_data(addr, count, data)) ? 0 : 1;
    if(res != 0) {
         return res;
    }

    return res;
}

int mir3da_register_mask_write(unsigned char addr, unsigned char mask, unsigned char data){
    int     res = 0;
    unsigned char      tmp_data;

    res = mir3da_register_read(addr, &tmp_data);
    if(res) {
        return res;
    }

    tmp_data &= ~mask; 
    tmp_data |= data & mask;
    res = mir3da_register_write(addr, tmp_data);

    return res;
}

/*return value: 0: is ok    other: is failed*/
int mir3da_init(void){
	int             res = 0;
	unsigned char data=0;

	mir3da_register_read(NSA_REG_WHO_AM_I,&data);
	if(data != 0x13){
	       printk("------mir3da read chip id  error= %x-----\r\n",data); 
		return -1;
	}

	printk("------mir3da chip id = %x-----\r\n",data); 

	res =  mir3da_register_mask_write(NSA_REG_SPI_I2C, 0x24, 0x24);

	dly_tsk(5);

	res |= mir3da_register_mask_write(NSA_REG_G_RANGE, 0x03, 0x02);
	res |= mir3da_register_mask_write(NSA_REG_POWERMODE_BW, 0xFF, 0x5E);
	res |= mir3da_register_mask_write(NSA_REG_ODR_AXIS_DISABLE, 0xFF, 0x06);
	
	res |= mir3da_register_mask_write(NSA_REG_INT_PIN_CONFIG, 0x0F, 0x05);//set int_pin level
	res |= mir3da_register_mask_write(NSA_REG_INT_LATCH, 0x8F, 0x86);//clear latch and set latch mode
	
	res |= mir3da_register_mask_write(NSA_REG_ENGINEERING_MODE, 0xFF, 0x83);
	res |= mir3da_register_mask_write(NSA_REG_ENGINEERING_MODE, 0xFF, 0x69);
	res |= mir3da_register_mask_write(NSA_REG_ENGINEERING_MODE, 0xFF, 0xBD);
	res |= mir3da_register_mask_write(NSA_REG_SWAP_POLARITY, 0xFF, 0x00);

	return res;	    	
}

int mir3da_set_enable(char enable)
{
		int res = 0;
		if(enable)
		res = mir3da_register_mask_write(NSA_REG_POWERMODE_BW,0xC0,0x40);
		else	
		res = mir3da_register_mask_write(NSA_REG_POWERMODE_BW,0xC0,0x80);
	
	return res;	
}

int mir3da_open_interrupt(int num){
	int   res = 0;

	res = mir3da_register_write(NSA_REG_INTERRUPT_SETTINGS1,0x03);
	res = mir3da_register_write(NSA_REG_ACTIVE_DURATION,0x03 );
	res = mir3da_register_write(NSA_REG_ACTIVE_THRESHOLD,0x1B );
			
	switch(num){

		case 0:
			res = mir3da_register_write(NSA_REG_INTERRUPT_MAPPING1,0x04 );
			break;

		case 1:
			res = mir3da_register_write(NSA_REG_INTERRUPT_MAPPING3,0x04 );
			break;
	}

	return res;
}

int mir3da_close_interrupt(int num){
	int   res = 0;

	res = mir3da_register_write(NSA_REG_INTERRUPT_SETTINGS1,0x00 );
			
	switch(num){

		case 0:
			res = mir3da_register_write(NSA_REG_INTERRUPT_MAPPING1,0x00 );
			break;

		case 1:
			res = mir3da_register_write(NSA_REG_INTERRUPT_MAPPING3,0x00 );
			break;
	}

	return res;
}

/*return value: 0: is ok    other: is failed*/
int mir3da_read_data(short *x, short *y, short *z)
{
    unsigned char    tmp_data[6] = {0};

    if (mir3da_register_read_continuously(NSA_REG_ACC_X_LSB, 6, tmp_data) != 0) {
        return -1;
    }
    
    *x = ((short)(tmp_data[1] << 8 | tmp_data[0]))>> 4;
    *y = ((short)(tmp_data[3] << 8 | tmp_data[2]))>> 4;
    *z = ((short)(tmp_data[5] << 8 | tmp_data[4]))>> 4;

     printk("oringnal x y z %d %d %d\n",*x,*y,*z); 	

    return 0;
}

int mir3da_read_int_status(void)
{
	char data = 0;

	mir3da_register_read(NSA_REG_MOTION_FLAG,&data);
	if(data&0x04)
		return 1;

	return 0;
}

static short threhold[4] = {300,600,1200,10000};//value 10000 means turn off collision check
char mir3da_check_collision(int level)
{
		static short prev_x = 0,prev_y = 0,prev_z = 0;
		short x = 0, y = 0, z = 0;
		static char b_is_first = 0;
		char is_collision = 0;
		
		if(mir3da_read_data(&x,&y,&z))
			return -1;
			
		if(b_is_first == 0)
		{
				prev_x = x;
				prev_y = y;
				prev_z = z;
				
				b_is_first = 1;
				
				return 0;					
		}	
		
		if((abs(x - prev_x) > threhold[level])||(abs(y - prev_y) > threhold[level])||(abs(z - prev_z) > threhold[level]))
				is_collision = 1;
				
		prev_x = x;
		prev_y = y;
		prev_z = z;
		
		return is_collision;
}


