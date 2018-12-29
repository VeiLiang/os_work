#include "hardware.h"
#include "gpio.h"
#include "gpioi2c.h"
#include "xm_dev.h"
#include "xm_base.h"
#include "xm_power.h"
#include "xm_app_menudata.h"

//#define G_SENSOR_ENABLE

//#define GMA303KU
#define DS_DA380

#define	COLLISION_RECORDING_INTERVAL		2000		// 最小碰撞记录间隔时间(毫秒)

#ifdef G_SENSOR_ENABLE

#define	XMSYS_GSENSOR_TASK_STACK_SIZE	0x400
#define	XMSYS_GSENSOR_TASK_PRIORITY		230

#define	G_INT_GPIO							GPIO110

#define	XMSYS_GSENSOR_EVENT				0x01

#define	debug_msg	XM_printf

#define	Delay_DelayMs	OS_Delay

static void CollisionEventSelfClearCallback (void);

static OS_TASK TCB_GSensorTask;
__no_init static OS_STACKPTR int StackGSensorTask[XMSYS_GSENSOR_TASK_STACK_SIZE/4];          /* Task stacks */

static unsigned int collision_event_ticket;			// 碰撞事件发生的时间
static unsigned int collision_event_triggering;		// 碰撞事件触发中
static OS_TIMER CollisionEventSelfClearTimer;		// Self-Clearing定时器

static unsigned int parking_collision_startup;		// 停车监控模式下启动, 3分钟录像后停止

#ifdef GMA303KU
#define	GMA303KU_SLAVE_ADDRESS			0x19		// SA0 = VDDIO

static void gsensor_gma303ku_intr (void)
{
	//XM_printf ("G_INT\n");
	OS_SignalEvent(XMSYS_GSENSOR_EVENT, &TCB_GSensorTask); /* 通知事件 */
}

int GSensor_I2C_WriteReg (unsigned char addr, unsigned char data)
{
	int loop = 3;
	int ret;
	unsigned char buff[1];
	buff[0] = data;
	while (loop > 0)
	{
		ret = gma303ku_i2c_write_bytes (GMA303KU_SLAVE_ADDRESS, &addr, 1, (char *)buff, 1);
		if(ret == 0)
			return ret;
		loop --;
	}
	return -1;
}

unsigned char GSensor_I2C_ReadReg (unsigned char addr)
{
	int loop = 3;
	unsigned char buff[1];	
	buff[0] = 0;
	
	while(loop > 0)
	{
		if(gma303ku_i2c_read_bytes (GMA303KU_SLAVE_ADDRESS, &addr, 1, (char *)buff, 1) == 1)
		{
			return buff[0];
		}
		loop --;
	}
	return 0;
}

int GSensor_I2C_ReadReg_2B (unsigned char addr, UINT16 *value)
{
	int ret;
	int loop = 3;
	unsigned char buff[2];	
	buff[0] = 0;
	buff[1] = 0;
	
	while (loop > 0)
	{
		ret = gma303ku_i2c_read_bytes (GMA303KU_SLAVE_ADDRESS, &addr, 1, (char *)buff, 2);
		if(ret == 2)
		{
			*value = (UINT16)( (buff[0] << 8) | (buff[1]) );
			return 0;
		}
		loop --;
	}
		
	return -1;
}

static int gma303ku_identify (void)
{
	unsigned int val;
	int loop = 5;
	while(loop > 0)
	{
		val = GSensor_I2C_ReadReg(0x00);
		if(val == 0xA3)
		{
			break;
		}
		loop --;
	}
	if(loop == 0)
	{
		XM_printf ("GMA303KU NG\n");
		return -1;
	}
	else
	{
		XM_printf ("GMA303KU OK\n");
		return  0;	
	}
}


// 停车监控
// parking_monitor_enable 是否使能停车监控
void GMA303ku_open_parking_interrupt(int parking_monitor_enable)
{
	UINT16 value;
	unsigned char reg_15h;

	GSensor_I2C_WriteReg(0x01,0x02);// Powerdown reset
  
	// HP_NCM, MVE_NCM, HP_CM, MVE_CM
	//GSensor_I2C_WriteReg(0x16,0x1B);
	GSensor_I2C_WriteReg(0x16,0x12);

	// GSensor_I2C_WriteReg(0x03,0x01);	// 0.25G, G_INT的脉冲宽度不能满足导通时序
	GSensor_I2C_WriteReg(0x03,0x04);	// 1.00G, 从0x01--> 0x04可以改善G_INT的脉冲宽度, 
												//		现在应用0x04设置, 其中断触发的脉冲宽度可以达到50ms左右, 
												//		满足DC-DC导通, N141 Power-On-Reset, BootRom启动并输出Power_ENABLE电平锁定ACC的时序
	//門檻設置,從 0x01 ~ 0x1F . 0x01 = 0.25G. 0x04:1G   0x1F:7.75G

	GSensor_I2C_ReadReg_2B(0x04,&value);
	GSensor_I2C_ReadReg(0x05);
	GSensor_I2C_ReadReg(0x06);
	GSensor_I2C_ReadReg(0x07);
	GSensor_I2C_ReadReg(0x08);
	GSensor_I2C_ReadReg(0x09);
	GSensor_I2C_ReadReg(0x0A);
	GSensor_I2C_ReadReg(0x0B);
	if(parking_monitor_enable)
		reg_15h = 0x4c;
	else
		reg_15h = 0x08;		// 关闭motion detection
	// 非连续模式motion detection, INT输出高, 连续模式motion detection, 沒上拉電組
	GSensor_I2C_WriteReg(0x15, reg_15h);	//啟動中斷 	有上拉電組--高電平輸出0x5D,低電平0x55
  												//				   沒上拉電組--高電平輸出0x4C,低電平0x44
  
	// IMEN_NCM(interrupt enable for motion detection), ITYPE_NCM 
	// 非连续模式motion detection, INT输出高, 连续模式motion detection, 有上拉電組
	//GSensor_I2C_WriteReg(0x15,0x5D);
	//debug_err((">>>>GMA303 parking\r\n"));
}

static BOOL g_GSFirstFlag = TRUE;
BOOL CheckGsensor (UINT32 GSensitivity)
{
	INT16  abs_cal_x=0,abs_cal_y=0,abs_cal_z=0;
	INT16 Threshold = 0,P_INT_COUNT = 0;
	INT16 tmp_data[6];
	INT16 Xdata,Ydata,Zdata,tmp,GsXData,GsYData,GsZData;
	static INT16 OldGsXData,OldGsYData,OldGsZData,Xdatab=0,Ydatab=0,Zdatab=0;
	UINT16 value;

	/*
	if (GSensitivity == 3)
		Threshold = 280;
	else 
	*/	
	if (GSensitivity == 2)
		Threshold = 280;	//460;
	else if (GSensitivity == 1)
		Threshold = 600;
	else
		Threshold = 1024;
	 
	//printf ("thread=%d\n", Threshold);

    GSensor_I2C_ReadReg_2B(0x04,&value);
    GSensor_I2C_ReadReg(0x05);
    tmp_data[0] = GSensor_I2C_ReadReg(0x06);
    tmp_data[1] = GSensor_I2C_ReadReg(0x07);
    tmp_data[2] = GSensor_I2C_ReadReg(0x08);
    tmp_data[3] = GSensor_I2C_ReadReg(0x09);
    tmp_data[4] = GSensor_I2C_ReadReg(0x0A);
    tmp_data[5] = GSensor_I2C_ReadReg(0x0B);

    Xdata = (signed short)(((tmp_data[1] << 8) | tmp_data[0])&0xfffe)/2;
    Ydata = (signed short)(((tmp_data[3] << 8) | tmp_data[2])&0xfffe)/2;
    Zdata = (signed short)(((tmp_data[5] << 8)  | tmp_data[4])&0xfffe)/2;
    if (g_GSFirstFlag)
    {
       OldGsXData = Xdata;
       OldGsYData = Ydata;
       OldGsZData = Zdata;
       g_GSFirstFlag = FALSE;
    }
    //debug_err((">>>>GS X %d Y %d Z %d\r\n",(OldGsXData-Xdata),(OldGsYData-Ydata),(OldGsZData-Zdata)));
	 
	 // printf ("old (%d, %d, %d), new (%d, %d, %d)\n", OldGsXData, OldGsYData, OldGsZData, Xdata, Ydata, Zdata);
	 // printf ("dif (%d, %d, %d)\n", abs(OldGsXData-Xdata), abs(OldGsYData-Ydata),  abs(OldGsZData-Zdata));
	 
    if(1)//Xdatab!=0 & Ydatab!=0 & Zdatab !=0)
    {
        if((abs(OldGsXData-Xdata)>Threshold)||(abs(OldGsYData-Ydata)>Threshold)||(abs(OldGsZData-Zdata)>Threshold))
        {
            OldGsXData = Xdata;
            OldGsYData = Ydata;
            OldGsZData = Zdata;
            return TRUE;
        }
        else
        {
            OldGsXData = Xdata;
            OldGsYData = Ydata;
            OldGsZData = Zdata;
            return FALSE;
        }
    }
    else
    {
        OldGsXData = Xdata;
        OldGsYData = Ydata;
        OldGsZData = Zdata;
        return FALSE;
    }
}

static void g_sensor_SelPad (void)
{
	unsigned int val;
	
	// GPIO0 -->G_INT
	// pad_ctla
	// [5:4]	gpio0	gpio0_pad	GPIO0	sen_clk_out
	XM_lock ();
	val = rSYS_PAD_CTRL0A;
	val &= ~( (3 << 4) );
	rSYS_PAD_CTRL0A = val;
	XM_unlock ();	
}

static void g_sensor_init_G_INT (void)
{
	XM_lock();
	SetGPIOPadDirection (G_INT_GPIO, euInputPad);
	GPIO_IRQ_Request (G_INT_GPIO, euHightLevelTrig, gsensor_gma303ku_intr);		// 高电平触发
	EnableGPIOPadIRQ (G_INT_GPIO);
	XM_unlock();		
}

// G_sensor初始化
int xm_g_sensor_init (void)
{
	unsigned int parking_monitor_enable;
	
	g_sensor_SelPad ();
	
	if(gma303ku_identify () < 0)
		return -1;
	
	// 检查是否停车监控
	{
		// 停车监控启动
		// 录像后自动关闭
		
		// 如何区分ACC点火启动还是G-Sensor触发启动?
		// G-Sensor在ACC点火后触发, 如何区分?
		// 需要确认ACC是否已点火
		if(xm_power_check_acc_on())
			parking_collision_startup = 0;
		else
		{
			parking_collision_startup = 1;
			collision_event_triggering = 1;		// 标记录像需要加锁
		}
	}
	
	XM_printf ("startup %s\n", parking_collision_startup ? "G-Sensor" : "ACC");
	
	parking_monitor_enable = (AP_GetMenuItem (APPMENUITEM_PARKMONITOR) == AP_SETTING_PARK_MONITOR_ON) ? 1 : 0;
	XM_printf ("parking_monitor %s\n", parking_monitor_enable ? "enable" : "disable");
	GMA303ku_open_parking_interrupt (parking_monitor_enable);
	
	g_sensor_init_G_INT ();
	
	return 0;
}
#endif


#ifdef DS_DA380
#define	DA380_SLAVE_ADDRESS			0x4e		

#define NSA_REG_SPI_I2C                 0x00
#define NSA_REG_WHO_AM_I                0x01
#define NSA_REG_ACC_X_LSB               0x02
#define NSA_REG_ACC_X_MSB               0x03
#define NSA_REG_ACC_Y_LSB               0x04
#define NSA_REG_ACC_Y_MSB               0x05
#define NSA_REG_ACC_Z_LSB               0x06
#define NSA_REG_ACC_Z_MSB               0x07
#define NSA_REG_MOTION_FLAG				0x09
#define NSA_REG_G_RANGE                 0x0f
#define NSA_REG_ODR_AXIS_DISABLE        0x10
#define NSA_REG_POWERMODE_BW            0x11
#define NSA_REG_SWAP_POLARITY           0x12
#define NSA_REG_FIFO_CTRL               0x14
#define NSA_REG_INTERRUPT_SETTINGS1     0x16
#define NSA_REG_INTERRUPT_SETTINGS2     0x17
#define NSA_REG_INTERRUPT_MAPPING1      0x19
#define NSA_REG_INTERRUPT_MAPPING2      0x1a
#define NSA_REG_INTERRUPT_MAPPING3      0x1b
#define NSA_REG_INT_PIN_CONFIG          0x20
#define NSA_REG_INT_LATCH               0x21
#define NSA_REG_ACTIVE_DURATION         0x27
#define NSA_REG_ACTIVE_THRESHOLD        0x28
#define NSA_REG_TAP_DURATION            0x2A
#define NSA_REG_TAP_THRESHOLD           0x2B
#define NSA_REG_CUSTOM_OFFSET_X         0x38
#define NSA_REG_CUSTOM_OFFSET_Y         0x39
#define NSA_REG_CUSTOM_OFFSET_Z         0x3a
#define NSA_REG_ENGINEERING_MODE        0x7f
#define NSA_REG_SENSITIVITY_TRIM_X      0x80
#define NSA_REG_SENSITIVITY_TRIM_Y      0x81
#define NSA_REG_SENSITIVITY_TRIM_Z      0x82
#define NSA_REG_COARSE_OFFSET_TRIM_X    0x83
#define NSA_REG_COARSE_OFFSET_TRIM_Y    0x84
#define NSA_REG_COARSE_OFFSET_TRIM_Z    0x85
#define NSA_REG_FINE_OFFSET_TRIM_X      0x86
#define NSA_REG_FINE_OFFSET_TRIM_Y      0x87
#define NSA_REG_FINE_OFFSET_TRIM_Z      0x88
#define NSA_REG_SENS_COMP               0x8c
#define NSA_REG_SENS_COARSE_TRIM        0xd1

#define abs(x) (((x) < 0) ? -(x) : (x))

static short threhold[4] = {300,600,1200,10000};//value 10000 means turn off collision check

static void gsensor_da380_intr (void)
{
	XM_printf ("G_INT\n");
	OS_SignalEvent(XMSYS_GSENSOR_EVENT, &TCB_GSensorTask); /* 通知事件 */
}

int mir3da_register_write (unsigned char addr, unsigned char data)
{
	int ret = -1;
	int retries = 0;

	while(retries < 3)
	{
		ret = da380_i2c_write_bytes (DA380_SLAVE_ADDRESS, &addr, 1, &data, 1);
		if(ret == 0)
			break;
		
		retries++;
	}
	
	if(retries >= 3)
	{
		XM_printf("%s i2c write dev:%x addr:%x error\n",__func__,DA380_SLAVE_ADDRESS,addr);
		ret = -1;
	}
	return ret;
}

unsigned char mir3da_register_read (unsigned char addr,char *rxdata)
{
	int ret = -1;
	int retries = 0;

	while(retries < 3)
	{
		if(da380_i2c_read_bytes (DA380_SLAVE_ADDRESS, &addr, 1, rxdata, 1) == 1)
		{
			break;
		}
		retries++;
	}

	if(retries >= 3)
	{
		XM_printf("%s i2c read dev:%x addr:%x error\n",__func__,DA380_SLAVE_ADDRESS,addr);
		ret = -1;
	}	
	return ret;
}

int mir3da_read_byte_data( unsigned char addr, unsigned char *data)
{
	int ret = 0;
	char pdata = 0;
	
	da380_i2c_read_bytes (DA380_SLAVE_ADDRESS, &addr, 1, &pdata, 1);
	*data = (unsigned char)pdata;
	return ret;
}

/*return value: 0: is count    other: is failed*/
int i2c_read_block_data( unsigned char base_addr, unsigned char count, unsigned char *data)
{
	int i = 0;
		
	for(i = 0; i < count;i++)
	{
		if(mir3da_read_byte_data(base_addr+i,(data+i)))
		{
			return -1;		
		}
	}	
	return count;
}


int mir3da_register_mask_write(unsigned char addr, unsigned char mask, unsigned char data)
{
    int  res = 0;
    unsigned char tmp_data;

    mir3da_register_read(addr, &tmp_data);

    tmp_data &= ~mask; 
    tmp_data |= data & mask;
    res = mir3da_register_write(addr, tmp_data);

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

int mir3da_open_interrupt(int num)
{
	int   res = 0;

	res = mir3da_register_write(NSA_REG_INTERRUPT_SETTINGS1,0x03);
	res = mir3da_register_write(NSA_REG_ACTIVE_DURATION,0x03 );
	//res = mir3da_register_write(NSA_REG_ACTIVE_THRESHOLD,0x1B );
	res = mir3da_register_write(NSA_REG_ACTIVE_THRESHOLD,0x26);
	
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

int mir3da_register_read_continuously( unsigned char addr, unsigned char count, unsigned char *data)
{
    int     res = 0;
    res = (count==i2c_read_block_data(addr, count, data)) ? 0 : 1;
    if(res != 0) 
	{
         return res;
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

     debug_msg("oringnal x y z %d %d %d\n",*x,*y,*z); 	

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

	  	XM_printf ("old (%d, %d, %d), new (%d, %d, %d)\n", prev_x, prev_y, prev_z, x, y, z);
	  	XM_printf ("dif (%d, %d, %d)\n", abs(x - prev_x), abs(y - prev_y),  abs(z - prev_z));
	 	
		if((abs(x - prev_x) > threhold[level])||(abs(y - prev_y) > threhold[level])||(abs(z - prev_z) > threhold[level]))
				is_collision = 1;
				
		prev_x = x;
		prev_y = y;
		prev_z = z;
		
		return is_collision;
}

static int da380_identify (void)
{
	unsigned char val;
	int loop = 5;
	while(loop > 0)
	{
		mir3da_register_read(NSA_REG_WHO_AM_I,&val);
		if(val == 0x13)
		{
			break;
		}
		loop --;
	}
	if(loop == 0)
	{
		XM_printf ("DA380 NG\n");
		return -1;
	}
	else
	{
		XM_printf ("DA380 OK\n");
		return  0;	
	}
}


// 停车监控
// parking_monitor_enable 是否使能停车监控
void da380_open_parking_interrupt(int parking_monitor_enable)
{	
	mir3da_register_mask_write(NSA_REG_SPI_I2C, 0x24, 0x24);

	Delay_DelayMs(5);

	mir3da_register_mask_write(NSA_REG_G_RANGE, 0x03, 0x02);
	//mir3da_register_mask_write(NSA_REG_POWERMODE_BW, 0xFF, 0x5E);
	mir3da_register_mask_write(NSA_REG_POWERMODE_BW, 0xFF, 0x1E);
	//mir3da_register_mask_write(NSA_REG_ODR_AXIS_DISABLE, 0xFF, 0x06);
	mir3da_register_mask_write(NSA_REG_ODR_AXIS_DISABLE, 0xFF, 0x07);
	
	mir3da_register_mask_write(NSA_REG_INT_PIN_CONFIG, 0x0F, 0x05);//set int_pin level
	//mir3da_register_mask_write(NSA_REG_INT_LATCH, 0x8F, 0x86);//clear latch and set latch mode
	mir3da_register_mask_write(NSA_REG_INT_LATCH, 0x8F, 0x81);//clear latch and set latch mode

	mir3da_register_mask_write(NSA_REG_ENGINEERING_MODE, 0xFF, 0x83);
	mir3da_register_mask_write(NSA_REG_ENGINEERING_MODE, 0xFF, 0x69);
	mir3da_register_mask_write(NSA_REG_ENGINEERING_MODE, 0xFF, 0xBD);
	mir3da_register_mask_write(NSA_REG_SWAP_POLARITY, 0xFF, 0x00);
	Delay_DelayMs(10);
	
	mir3da_open_interrupt(0);
	
	if(parking_monitor_enable)
	{
		mir3da_set_enable(1);
	}
	else
	{
		mir3da_set_enable(0);
	}
}

BOOL CheckGsensor (UINT32 GSensitivity)
{
   return mir3da_check_collision(GSensitivity);
}

static void g_sensor_SelPad (void)
{
	unsigned int val;

	// UART1_RXD/GPIO110 -->G_INT
	// pad_ctl09
	XM_lock ();
	val = rSYS_PAD_CTRL09;
	val &= ~( (3 << 20) );
	rSYS_PAD_CTRL09 = val;
	XM_unlock ();	
}

static void g_sensor_init_G_INT (void)
{
	XM_lock();
	SetGPIOPadDirection (G_INT_GPIO, euInputPad);
	GPIO_IRQ_Request (G_INT_GPIO, euHightLevelTrig, gsensor_da380_intr);		// 高电平触发
	EnableGPIOPadIRQ (G_INT_GPIO);
	XM_unlock();		
}

// G_sensor初始化
int xm_g_sensor_init (void)
{
	unsigned int parking_monitor_enable;
	
	g_sensor_SelPad ();
	
	if(da380_identify () < 0)
		return -1;
	
	// 检查是否停车监控
	{
		// 停车监控启动
		// 录像后自动关闭
		
		// 如何区分ACC点火启动还是G-Sensor触发启动?
		// G-Sensor在ACC点火后触发, 如何区分?
		// 需要确认ACC是否已点火
		if(xm_power_check_acc_on())
			parking_collision_startup = 0;
		else
		{
			parking_collision_startup = 1;
			collision_event_triggering = 1;		// 标记录像需要加锁
		}
	}
	
	XM_printf ("startup %s\n", parking_collision_startup ? "G-Sensor" : "ACC");
	
	parking_monitor_enable = (AP_GetMenuItem (APPMENUITEM_PARKMONITOR) == AP_SETTING_PARK_MONITOR_ON) ? 1 : 0;
	XM_printf ("parking_monitor %s\n", parking_monitor_enable ? "enable" : "disable");
	da380_open_parking_interrupt (parking_monitor_enable);
	
	g_sensor_init_G_INT ();
	
	return 0;
}
#endif


static void gsensor_task (void)
{
	// 创建0.5秒定时器
	OS_CREATETIMER (&CollisionEventSelfClearTimer, CollisionEventSelfClearCallback, 500);
	
	xm_g_sensor_init ();
	
	while(1)
	{
		OS_U8 g_event;
		
		g_event = OS_WaitEvent (XMSYS_GSENSOR_EVENT);
		
		if(g_event & (XMSYS_GSENSOR_EVENT) )
		{
			XM_printf ("XMSYS_GSENSOR_EVENT\n");
			UINT32 GSensitivity;
			unsigned int AP_SETTING_COLLISION = AP_GetMenuItem (APPMENUITEM_COLLISION_SENSITIVITY);
			if(AP_SETTING_COLLISION == AP_SETTING_COLLISION_1G)	// AP_ID_SETTING_CLOSE;
			{
				// 关闭
				continue;
			}
			else if(AP_SETTING_COLLISION == AP_SETTING_COLLISION_2G)
			{
				// AP_ID_VIDEOSETTING_COLLISIONSENSITIVITY_LOW
				GSensitivity = 0;
			}
			else if(AP_SETTING_COLLISION == AP_SETTING_COLLISION_3G)
			{
				// AP_ID_VIDEOSETTING_COLLISIONSENSITIVITY_MEDIUM
				GSensitivity = 1;
			}
			else
			{
				// AP_ID_VIDEOSETTING_COLLISIONSENSITIVITY_HIGH
				GSensitivity = 2;
			}
			
			if(CheckGsensor (GSensitivity))
			{
				XM_printf ("G-Event\n");
				XM_lock();
				// 记录事件
				collision_event_triggering = 1;
				collision_event_ticket = XM_GetTickCount();		
				XM_unlock ();
			}
		}
	}
}

void XMSYS_GSensorInit (void)
{
	// GPIO I2C
	GPIO_I2C_Init ();
	OS_CREATETASK(&TCB_GSensorTask, "GSensor", gsensor_task, XMSYS_GSENSOR_TASK_PRIORITY, StackGSensorTask);
}


void XMSYS_GSensorExit (void)
{
	
}

// 碰撞事件自清除定时器
// 非录像状态下清除碰撞事件
static void CollisionEventSelfClearCallback (void)
{
	XM_lock ();
	if(collision_event_triggering)
	{
		// 事件已触发
		// 检查事件是否已超时未被读取
		if( XM_GetTickCount()  >= (collision_event_ticket + COLLISION_RECORDING_INTERVAL) )
		{
			// 超时未被读取, 清除
			collision_event_triggering = 0;
			printf ("timeout to clear collision_event\n");
		}
	}
	XM_unlock ();
	
	OS_RetriggerTimer (&CollisionEventSelfClearTimer);

}

// 检查并清除碰撞事件
// 返回值
//		1		碰撞已发生
//		0		碰撞未发生
int XMSYS_GSensorCheckAndClearCollisionEvent (void)
{
	int ret = 0;
	XM_lock ();
	if(collision_event_triggering)
	{
		// 清除事件
		collision_event_triggering = 0;
		ret = 1;
	}
	XM_unlock ();
	return ret;
}

// 检查是否是停车碰撞启动
//	返回值
//		1		停车碰撞启动
//		0		非停车碰撞启动
int XMSYS_GSensorCheckParkingCollisionStartup (void)
{
	int ret;
	// 检查ACC是否已on
	ret = parking_collision_startup;
	//parking_collision_startup = 0;
	if(xm_power_check_acc_on())
	{
		ret = 0;
	}
	return ret;
}

#else
void XMSYS_GSensorInit (void)
{
}
void XMSYS_GSensorExit (void)
{
	
}

int XMSYS_GSensorCheckAndClearCollisionEvent (void)
{
	return 0;
}

int XMSYS_GSensorCheckParkingCollisionStartup (void)
{
	return 0;
}
#endif

