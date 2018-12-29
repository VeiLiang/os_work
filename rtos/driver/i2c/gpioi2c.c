/***********************************************************************
*Copyright (c)2012  Arkmicro Technologies Inc. 
*All Rights Reserved 
*
*Filename:    Gpioi2c.c
*Version :    1.0 
*Date    :    2010.04.07
*Author  :    gongmingli
*Abstract:    Ark1630 soc i2c  
*History :     
* 
*Version :    2.0 
*Date    :    2012.02.27
*Author  :    ls 
*Abstract:    ark1660  Ver1.0 MPW driver remove waring
*History :    1.0

************************************************************************/
#include "hardware.h"
#include "gpio.h"
#include "printk.h"
#include "target_ucos_ii.h"
#include "gpioi2c.h"


#define	OS_ENTER_CRITICAL		OS_EnterRegion
#define	OS_EXIT_CRITICAL		OS_LeaveRegion

#define	printk	XM_printf
//#define	printk(...)	

extern void XM_lock (void);
extern void XM_unlock (void);

static void SCLK_DIR_IN(unsigned int SclSelectGpio)	//SCLK_DIR_IN	
{
	XM_lock();
	SetGPIOPadDirection(SclSelectGpio,euInputPad);
	XM_unlock();
}
			
 void SCLK_DIR_OUT(unsigned int SclSelectGpio)//SCLK_DIR_OUT
{
	XM_lock();
	SetGPIOPadDirection(SclSelectGpio,euOutputPad);
	XM_unlock();
}

				
 void SCLK_LOW(unsigned int SclSelectGpio)//SCLK_LOW
{
	XM_lock();
	SetGPIOPadData(SclSelectGpio,euDataLow);
	XM_unlock();
}


 void SCLK_HIGH(unsigned int SclSelectGpio)//SCLK_HIGH
{	
	XM_lock();
	SetGPIOPadData(SclSelectGpio,euDataHigh);
	XM_unlock();
}
			
static void SDA_DIR_IN(unsigned int SdaSelectGpio)//SDA_DIR_IN
{
	XM_lock();
	SetGPIOPadDirection(SdaSelectGpio,euInputPad);
	XM_unlock();
}
				
static void SDA_DIR_OUT(unsigned int SdaSelectGpio)//SDA_DIR_OUT
{
	XM_lock();
	SetGPIOPadDirection(SdaSelectGpio,euOutputPad);
	XM_unlock();
}

static void SDA_LOW(unsigned int SdaSelectGpio)//SDA_LOW
{
	XM_lock();
	SetGPIOPadData(SdaSelectGpio,euDataLow);
	XM_unlock();
}
			
static void SDA_HIGH(unsigned int SdaSelectGpio)//SDA_HIGH
{
	XM_lock();
	SetGPIOPadData(SdaSelectGpio,euDataHigh);
	XM_unlock();
}

static unsigned int READ_SDA(unsigned int SdaSelectGpio)//READ_SDA
{
	unsigned int GetSdaState;
	
	XM_lock();
	GetSdaState = GetGPIOPadData(SdaSelectGpio);
	//printk("GetSdaState: 0x%x\n",GetSdaState);
	XM_unlock();
	return  GetSdaState;
}

static OS_EVENT *lg_pSemGPIOI2CGroup1 = NULL;
static OS_EVENT *lg_pSemGPIOI2CGroup2 = NULL;
static OS_EVENT *lg_pSemGPIOI2CGroup3 = NULL;
static int gpioi2c_inited = 0;

int OpenGPIOI2CDevice(const GPIO_I2C  *I2cInfo)
{
	INT8U err;
	int ret = 0;
	
	switch(I2cInfo->I2cGroup)
	{
		case 1:
			OSSemPend(lg_pSemGPIOI2CGroup1, 0, &err);
			break;
		case 2:
			OSSemPend(lg_pSemGPIOI2CGroup2, 0, &err);
			break;
		case 3:
			OSSemPend(lg_pSemGPIOI2CGroup3, 0, &err);
			break;
		default:
			ret = -1;
			break;
	}

	return ret;
}

int CloseGPIOI2CDevice(const GPIO_I2C  *I2cInfo)
{
	int ret = 0;
	
	switch(I2cInfo->I2cGroup)
	{
		case 1:
			OSSemPost(lg_pSemGPIOI2CGroup1);
			break;
		case 2:
			OSSemPost(lg_pSemGPIOI2CGroup2);
			break;
		case 3:
			OSSemPost(lg_pSemGPIOI2CGroup3);
			break;
		default:
			ret = -1;
			break;
	}

	return ret;
}


 void GpioI2cStart(const GPIO_I2C  *I2cInfo)
{
	unsigned int SdaSelectGpio;
	unsigned int SclSelectGpio;

	switch(I2cInfo->I2cGroup)
	{
		case 1:
			SclSelectGpio = SCL_GPIO_GROUP1;
			SdaSelectGpio = SDA_GPIO_GROUP1;
			break;
			
		case 2:
			SclSelectGpio = SCL_GPIO_GROUP2;
			SdaSelectGpio = SDA_GPIO_GROUP2;
			break;
			
		case 3:
			SclSelectGpio = SCL_GPIO_GROUP3;
			SdaSelectGpio = SDA_GPIO_GROUP3;
			break;
			
		default:
			return;
	}
	
	SCLK_DIR_OUT(SclSelectGpio); //SCLK_DIR_OUT;
	SDA_DIR_OUT(SdaSelectGpio);//SDA_DIR_OUT;
	DELAY(DURATION_CONVERT);

	SCLK_HIGH(SclSelectGpio);//SCLK_HIGH;
	SDA_HIGH(SdaSelectGpio);//SDA_HIGH;

	DELAY(DURATION_START_1);
	SDA_LOW(SdaSelectGpio);//SDA_LOW; 
	DELAY(DURATION_START_2);
	SCLK_LOW(SclSelectGpio);//SCLK_LOW;
	DELAY(DURATION_START_3);
}

int GpioI2cSendAck(const GPIO_I2C  *I2cInfo)
{
	unsigned int SdaSelectGpio;
	unsigned int SclSelectGpio;

	switch(I2cInfo->I2cGroup)
	{
		case 1:
			SclSelectGpio = SCL_GPIO_GROUP1;
			SdaSelectGpio = SDA_GPIO_GROUP1;
			break;
			
		case 2:
			SclSelectGpio = SCL_GPIO_GROUP2;
			SdaSelectGpio = SDA_GPIO_GROUP2;
			break;
			
		case 3:
			SclSelectGpio = SCL_GPIO_GROUP3;
			SdaSelectGpio = SDA_GPIO_GROUP3;
			break;
			
		default:
			return 0;
	}
	
	SDA_DIR_OUT(SdaSelectGpio);//SDA_DIR_OUT;
	DELAY(DURATION_CONVERT);
	SDA_LOW(SdaSelectGpio);//SDA_LOW;

	DELAY(DURATION_LOW/2);
	SCLK_HIGH(SclSelectGpio);//SCLK_HIGH;
	DELAY(DURATION_HIGH);
	SCLK_LOW(SclSelectGpio);//SCLK_LOW;
	DELAY(DURATION_LOW/2);	
	
	return 0;
}

int GpioI2cSendNAck(const GPIO_I2C  *I2cInfo)
{
	unsigned int SdaSelectGpio;
	unsigned int SclSelectGpio;

	switch(I2cInfo->I2cGroup)
	{
		case 1:
			SclSelectGpio = SCL_GPIO_GROUP1;
			SdaSelectGpio = SDA_GPIO_GROUP1;
			break;
			
		case 2:
			SclSelectGpio = SCL_GPIO_GROUP2;
			SdaSelectGpio = SDA_GPIO_GROUP2;
			break;
			
		case 3:
			SclSelectGpio = SCL_GPIO_GROUP3;
			SdaSelectGpio = SDA_GPIO_GROUP3;
			break;
			
		default:
			return 0;
	}
	
	SDA_DIR_OUT(SdaSelectGpio);//SDA_DIR_OUT;
	DELAY(DURATION_CONVERT);
	SDA_HIGH(SdaSelectGpio);//SDA_HIGH;

	DELAY(DURATION_LOW/2);
	SCLK_HIGH(SclSelectGpio);//SCLK_HIGH;
	DELAY(DURATION_HIGH);
	SCLK_LOW(SclSelectGpio);//SCLK_LOW;
	DELAY(DURATION_LOW/2);	
	
	return 0;
}

int GpioI2cCheckAck(const GPIO_I2C  *I2cInfo)
{
	unsigned int SdaSelectGpio;
	unsigned int SclSelectGpio;
	unsigned int loop;

	switch(I2cInfo->I2cGroup)
	{
		case 1:
			SclSelectGpio = SCL_GPIO_GROUP1;
			SdaSelectGpio = SDA_GPIO_GROUP1;
			break;
			
		case 2:
			SclSelectGpio = SCL_GPIO_GROUP2;
			SdaSelectGpio = SDA_GPIO_GROUP2;
			break;
			
		case 3:
			SclSelectGpio = SCL_GPIO_GROUP3;
			SdaSelectGpio = SDA_GPIO_GROUP3;
			break;
			
		default:
			return -1;
	}
	
	SDA_DIR_IN(SdaSelectGpio);//SDA_DIR_IN;
	DELAY(DURATION_CONVERT);
	
	SCLK_HIGH(SclSelectGpio);//SCLK_HIGH;
	DELAY(DURATION_HIGH);
	//DELAY();

	loop = 5;
	while(loop > 0)
	{
		if(READ_SDA(SdaSelectGpio) == 0)
			break;
		DELAY(DURATION_CONVERT);
		loop --;
	}
	
	//if(READ_SDA(SdaSelectGpio) != 0)
	if(loop == 0)
	{
		SCLK_LOW(SclSelectGpio);//SCLK_LOW;
		DELAY(DURATION_LOW);
		printk("CheckAck ERROR! \n");
		return -1;
	}
	else
	{
		SCLK_LOW(SclSelectGpio);//SCLK_LOW;
		DELAY(DURATION_LOW);
		return 0;
	}
}

void GpioI2cStop(const GPIO_I2C  *I2cInfo)
{
	unsigned int SdaSelectGpio;
	unsigned int SclSelectGpio;

	switch(I2cInfo->I2cGroup)
	{
		case 1:
			SclSelectGpio = SCL_GPIO_GROUP1;
			SdaSelectGpio = SDA_GPIO_GROUP1;
			break;
			
		case 2:
			SclSelectGpio = SCL_GPIO_GROUP2;
			SdaSelectGpio = SDA_GPIO_GROUP2;
			break;
			
		case 3:
			SclSelectGpio = SCL_GPIO_GROUP3;
			SdaSelectGpio = SDA_GPIO_GROUP3;
			break;
			
		default:
			return;
	}
	
	SDA_DIR_OUT(SdaSelectGpio);//SDA_DIR_OUT;
	SCLK_DIR_OUT(SclSelectGpio); //SCLK_DIR_OUT;
	DELAY(DURATION_CONVERT);
	
	SDA_LOW(SdaSelectGpio);//SDA_LOW;
	
	DELAY(DURATION_STOP_1);
	SCLK_HIGH(SclSelectGpio);//SCLK_HIGH;
	DELAY(DURATION_STOP_2);
	SDA_HIGH(SdaSelectGpio);//SDA_HIGH;
	DELAY(DURATION_STOP_3);
}

void GpioI2cWriteByte(UINT8 data,const GPIO_I2C  *I2cInfo)
{
	INT32 i;

	unsigned int SdaSelectGpio;
	unsigned int SclSelectGpio;

	switch(I2cInfo->I2cGroup)
	{
		case 1:
			SclSelectGpio = SCL_GPIO_GROUP1;
			SdaSelectGpio = SDA_GPIO_GROUP1;
			break;
			
		case 2:
			SclSelectGpio = SCL_GPIO_GROUP2;
			SdaSelectGpio = SDA_GPIO_GROUP2;
			break;
			
		case 3:
			SclSelectGpio = SCL_GPIO_GROUP3;
			SdaSelectGpio = SDA_GPIO_GROUP3;
			break;
			
		default:
			return;
	}
	
	SDA_DIR_OUT(SdaSelectGpio);//SDA_DIR_OUT;	
	SCLK_DIR_OUT(SclSelectGpio); //SCLK_DIR_OUT;

	DELAY(DURATION_CONVERT);
	
	for(i = 7; i>=0; i--)
	{
		if((data >> i) & 0x01)
			SDA_HIGH(SdaSelectGpio);//SDA_HIGH;
		else
			SDA_LOW(SdaSelectGpio);//SDA_LOW;

		DELAY(DURATION_LOW/2);
		SCLK_HIGH(SclSelectGpio);//SCLK_HIGH;
		DELAY(DURATION_HIGH);
		SCLK_LOW(SclSelectGpio);//SCLK_LOW;
		DELAY(DURATION_LOW/2);
	}	
	SDA_HIGH(SdaSelectGpio);//SDA_HIGH;
}

void GpioI2cReadByte(UINT8 *data,const GPIO_I2C  *I2cInfo)
{
	INT32 i;
	unsigned int SdaSelectGpio;
	unsigned int SclSelectGpio;

	switch(I2cInfo->I2cGroup)
	{
		case 1:
			SclSelectGpio = SCL_GPIO_GROUP1;
			SdaSelectGpio = SDA_GPIO_GROUP1;
			break;
			
		case 2:
			SclSelectGpio = SCL_GPIO_GROUP2;
			SdaSelectGpio = SDA_GPIO_GROUP2;
			break;
			
		case 3:
			SclSelectGpio = SCL_GPIO_GROUP3;
			SdaSelectGpio = SDA_GPIO_GROUP3;
			break;
			
		default:
			return;
	}


	SDA_DIR_IN(SdaSelectGpio);//SDA_DIR_IN;
	SCLK_DIR_OUT(SclSelectGpio); //SCLK_DIR_OUT;

	DELAY(DURATION_CONVERT);
	
	for(i = 7; i>=0; i--)
	{
		DELAY(DURATION_LOW/2);
		SCLK_HIGH(SclSelectGpio);//SCLK_HIGH;
		DELAY(DURATION_HIGH);
		*data = (*data << 1) | ((UINT8)READ_SDA(SdaSelectGpio));
		SCLK_LOW(SclSelectGpio);//SCLK_LOW;
		DELAY(DURATION_LOW/2);
	}	
}

int GpioI2cDeviceWriteBytes (const GPIO_I2C  *I2cInfo, unsigned char *buff, unsigned char size)
{
	int error = 0;
	//INT32 cpu_sr;
	int i;
	
	error = OpenGPIOI2CDevice(I2cInfo);
	if(error)
		return error;
	
	GpioI2cStart(I2cInfo);
		
	/*Send Slave Address*/
	GpioI2cWriteByte(I2cInfo->DeviceAddr,I2cInfo);//DeviceAddr,

	/*Check Ack*/
	error = GpioI2cCheckAck(I2cInfo);
	if(error)
	{
		printk("w CheckAck error1\n");
		goto STOP;
	}
		
	/*Send SubAddress*/
	for (i = 0; i < I2cInfo->reg_size; i ++)
	{
		GpioI2cWriteByte(I2cInfo->reg[i],I2cInfo);//register

		/*Check Ack*/
		error = GpioI2cCheckAck(I2cInfo);
		if(error)
		{
			printk("w CheckAck error2\n");
			goto STOP;
		}
	}
		
	while(size > 0)
	{
		/*Send Data*/
		GpioI2cWriteByte(*buff, I2cInfo);//wrdata
		error = GpioI2cCheckAck(I2cInfo);
		if(error)
		{
			printk("w CheckAck error3\n");
			goto STOP;
		}
		buff ++;
		size = (unsigned char)(size - 1);
	}
	

STOP:
	GpioI2cStop(I2cInfo);	
	CloseGPIOI2CDevice(I2cInfo);
	
	return error;	
}

// 从i2c slave读取指定长度的数据
// 返回值
//		>=0	读出的字节数
//		-1		failure
int GpioI2cDeviceReadBytes (const GPIO_I2C  *I2cInfo, unsigned char *data, unsigned char size)
{
	int ret = -1;
	//INT32 cpu_sr;
	int error;
	int i;

	error = OpenGPIOI2CDevice(I2cInfo);
	if(error)
		return -1;

	//经过JLINK单步调试发现如果单步执行GpioI2cDeviceRead代码会导致i2c读失败
	//但是写不会，因此读仍添加关中断保护
	//OS_ENTER_CRITICAL();
	
	GpioI2cStart(I2cInfo);

	/*Send Slave Address*/
	GpioI2cWriteByte(I2cInfo->DeviceAddr,I2cInfo);//DeviceAddr

	/*Check Ack*/
	error = GpioI2cCheckAck(I2cInfo);
	if(error)
	{
		printk("R CheckAck error1\n");
		goto STOP;
	}
		

	for (i = 0; i < I2cInfo->reg_size; i ++)
	{
		/*Send SubAddress*/
		GpioI2cWriteByte(I2cInfo->reg[i],I2cInfo);//register
	
		/*Check Ack*/
		error = GpioI2cCheckAck(I2cInfo);
		if(error)
		{
			printk("R CheckAck error2\n");
			goto STOP;
		}
	}
		
	/*Restart*/
	DELAY(DURATION_LOW);
	GpioI2cStart(I2cInfo);

	/*Send Slave Address*/
	//I2cInfo->DeviceAddr |= 1;
	GpioI2cWriteByte((I2cInfo->DeviceAddr + 1),I2cInfo);
	/*Check Ack*/
	error = GpioI2cCheckAck(I2cInfo);
	if(error)
	{
		printk("R CheckAck error3\n");
		goto STOP;
	}
	
	ret = size;
	
	while(size > 1)
	{
		/*Receive Data*/
		GpioI2cReadByte(data,I2cInfo);
		GpioI2cSendAck(I2cInfo);
		data ++;
		size = (unsigned char)(size - 1);
	}
	/*Receive Data*/
	GpioI2cReadByte(data,I2cInfo);
	GpioI2cSendNAck(I2cInfo);
		
STOP:		
	GpioI2cStop(I2cInfo);

	//OS_EXIT_CRITICAL();

	CloseGPIOI2CDevice(I2cInfo);
		
	return ret;
}


static void GPIO_I2C_Module_Init(void)
{
	if(gpioi2c_inited)
		return;
	
	lg_pSemGPIOI2CGroup1 = OSSemCreate(1);
	//lg_pSemGPIOI2CGroup2 = OSSemCreate(1);
	lg_pSemGPIOI2CGroup3 = OSSemCreate(1);
	gpioi2c_inited = 1;
}


/**
 * iic gpio setting
 *
 * GPIO103,GPIO104配置成IO
 */
void group2_i2c_pad_init(void)
{
	unsigned int val;
	
	XM_lock();
		
	// 第三组GPIO I2C
	// 7116-SDA SD1_D2 ([9]	 sd1_d2	sd1_d2_pad	GPIO103	sd1_data2)
	// 7116-SCL SD1_D3 ([10] sd1_d3	sd1_d3_pad	GPIO104	sd1_data3)
	// pad_ctl9
	val = rSYS_PAD_CTRL0A;
	val &= ~( (0x03 << 0) | (0x03 << 2) );
	rSYS_PAD_CTRL0A= val;
		
	XM_unlock();
	
	lg_pSemGPIOI2CGroup2 = OSSemCreate(1);
}

void GPIO_I2C_Init(void)
{
#if 0
	unsigned int val;
	
	XM_lock();
		
#if HDMI_720P	
	// pad_ctl8
	// [25:24]	nd_rb	nd_rb_pad	GPIO92	nandflash_rb0
	// [27:26]	nd_cen	nd_cen_pad	GPIO93	nandflash_cen0
	val = rSYS_PAD_CTRL08 ;
	val &= ~ ( (0x3 << 24) | (0x3 << 26) );
	rSYS_PAD_CTRL08 = val;
#endif

#if TULV

	// 第一组GPIO I2C
	// SDA	UART3_RXD(GPIO114)
	// SCL	UART3_TXD(GPIO115)
	//val = rSYS_PAD_CTRL09;
	//val &= ~( (0x03 << 28) | (0x03 << 30) );
	//rSYS_PAD_CTRL09 = val;
	
	// 第二组GPIO I2C
	// SDA	CD5 (GPIO85)
	// SCL	CD6 (GPIO86), 
#if TULV_BB_TEST	
	val = rSYS_PAD_CTRL08;
	val &= ~( (0x3 << 10) | (0x3 << 12) );
	rSYS_PAD_CTRL08 = val;
#else

	// SDA	cd1_pad(GPIO81)
	// SCL	cd0_pad(GPIO80)
	//val = rSYS_PAD_CTRL09;
	//val &= ~( (1 << 9) | (1 << 10) );
	//rSYS_PAD_CTRL09= val;
#endif

	// 第三组GPIO I2C
	// 7116-SDA SD1_D2 ([9]	 sd1_d2	sd1_d2_pad	GPIO103	sd1_data2)
	// 7116-SCL SD1_D3 ([10] sd1_d3	sd1_d3_pad	GPIO104	sd1_data3)
	// pad_ctl9
	val = rSYS_PAD_CTRL0A;
	val &= ~( (0x03 << 0) | (0x03 << 2) );
	rSYS_PAD_CTRL0A= val;
	
#endif
	
#if HONGJING_CVBS
#if HONGJING_BB_TEST	
	// D0 --> SCL
	// D3 --> SDA
	// pad_ctl8
	// [1:0]	cd0	cd0_pad	GPIO80	nand_data[9]
	// [7:6]	cd3	cd3_pad	GPIO83	nand_data[3]
	val = rSYS_PAD_CTRL08;
	val &= ~( (0x3 << 0) | (0x3 << 6) );
	rSYS_PAD_CTRL08 = val;	
#else
	// SDA [17:16]	uartrxd0	uartrxd0_pad	GPIO108	uart0_UARTRXD
	// pad_ctl9
	// [17:16]	    uartrxd0	uartrxd0_pad	GPIO108	uart0_UARTRXD
	val = rSYS_PAD_CTRL09;
	val &= ~( (0x3 << 16) );
	rSYS_PAD_CTRL09 = val;
	
	//	SCL LCD_D23/CPU_RST/GPIO75
	// pad_ctl7
	// [11:9]	lcd_d23	lcd_d23_pad	itu_b_din10输入 /输出GPIO75
	val = rSYS_PAD_CTRL07;
	val &= ~( (0x7 << 9) );
	val |= ( (1 << 30) );		// 设置为1, GPIO61~GPIO69, GPIO74~GPIO79输出使能	
	rSYS_PAD_CTRL07 = val;	
#endif
#endif
	
	XM_unlock();
	
	GPIO_I2C_Module_Init();
#endif
}


// 软件I2C中为空函数
void i2c_master_init(int slaveaddr,unsigned int hcnt,unsigned int lcnt,unsigned char mode,unsigned int maskvalue)
{
	GPIO_I2C_Init ();	
}



int i2c_reg_read_bytes (unsigned int slvaddr, unsigned int ucDataOffset, unsigned char *data, unsigned char size)
{
	GPIO_I2C gpio_i2c;
	gpio_i2c.DeviceAddr = (unsigned char)(slvaddr << 1);
	gpio_i2c.I2cGroup = 1;	// 使用第一组IO脚，SDA-->GPIO71 SCL-->GPIO70
	gpio_i2c.reg[0] = (unsigned char)ucDataOffset;	// 写入I2C设备的内部偏移地址或寄存器地址
	gpio_i2c.reg_size = 1;
	
	GpioI2cDeviceReadBytes (&gpio_i2c, data, size);
	
	return 0;
}

int i2c_reg_write_bytes (unsigned int slvaddr, unsigned int ucDataOffset, unsigned char* data, unsigned char size)
{
	GPIO_I2C gpio_i2c;
	gpio_i2c.DeviceAddr = (unsigned char)(slvaddr << 1);
	gpio_i2c.I2cGroup = 1;	// 使用第一组IO脚，SDA-->GPIO71 SCL-->GPIO70
	gpio_i2c.reg[0] = (unsigned char)ucDataOffset;	// 写入I2C设备的内部偏移地址或寄存器地址
	gpio_i2c.reg_size = 1;
	GpioI2cDeviceWriteBytes (&gpio_i2c, data, size);
	return 0;
}

// slvaddr地址为7位地址，需在最低位补上读(1)/写位(0)
// 一字节地址模式
void i2c_reg_write (unsigned int slvaddr, unsigned int ucDataOffset, unsigned char data)
{
	unsigned char buff[1];
	buff[0] = data;
	i2c_reg_write_bytes (slvaddr, ucDataOffset, buff, 1);
}

// slvaddr地址为7位地址，需在最低位补上读(1)/写位(0)
// 一字节地址模式
unsigned char i2c_reg_read (unsigned int slvaddr, unsigned int ucDataOffset)
{
	unsigned char buff[1];
	buff[0] = 0;
	i2c_reg_read_bytes (slvaddr, ucDataOffset, buff, 1);
	return buff[0];
}


int rxchip_i2c_read_bytes(unsigned int slvaddr, unsigned char* addr, int size, unsigned char *rxdata, int length)
{
	GPIO_I2C gpio_i2c;
	gpio_i2c.DeviceAddr = (unsigned char)(slvaddr );
	gpio_i2c.I2cGroup = 2;	// 使用第二组IO脚
	memcpy (gpio_i2c.reg, addr, size);
	gpio_i2c.reg_size = size;
	return GpioI2cDeviceReadBytes (&gpio_i2c, (unsigned char *)rxdata, length);	
}

int rxchip_i2c_write_bytes(unsigned int slvaddr, unsigned char* addr, int size, unsigned char *txdata, int length)
{
	GPIO_I2C gpio_i2c;
	gpio_i2c.DeviceAddr = (unsigned char)(slvaddr );
	gpio_i2c.I2cGroup = 2;	// 使用第二组IO脚
	memcpy (gpio_i2c.reg, addr, size);
	gpio_i2c.reg_size = size;
	return GpioI2cDeviceWriteBytes (&gpio_i2c, (unsigned char *)txdata, length);		
}


void i2c_close(unsigned int i2c_addr )
{
	printf("i2c i2c_close finished\n");
}





