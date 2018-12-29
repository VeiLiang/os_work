#ifndef GPIOI2C_H__
#define  GPIOI2C_H__

#include "types.h"
#include"Gpio.h"

#ifdef __cplusplus
extern "C" {
#endif


#define GPIO_I2C_READMODE            0
#define GPIO_I2C_WRITEMODE           1

#define  SDA_GPIO_GROUP1	GPIO115	   //G_SENSRO-SDA UART3_TXD(GPIO115) 
#define  SCL_GPIO_GROUP1	GPIO114	  // G_SENSOR-SCL UART3_RXD(GPIO114)

#if TULV
	
// TP-IC icn85xx
#if TULV_BB_TEST	
#define  SDA_GPIO_GROUP2	GPIO85		// SDA	CD5 (GPIO85)
#define  SCL_GPIO_GROUP2	GPIO86		// SCL	CD6 (GPIO86), 
#else
#define  SDA_GPIO_GROUP2	GPIO103	    //LT8918-SDA
#define  SCL_GPIO_GROUP2	GPIO104      //LT8918-SCL
#endif

#define  SDA_GPIO_GROUP3	GPIO117  // 7116-SDA SD1_D2 ([9]  sd1_d2	sd1_d2_pad	GPIO103	sd1_data2)
#define  SCL_GPIO_GROUP3	GPIO116	 // 7116-SCL SD1_D3 ([10] sd1_d3	sd1_d3_pad	GPIO104	sd1_data3)

#elif HONGJING_CVBS
	
#if HONGJING_BB_TEST	

#define  SDA_GPIO_GROUP2	GPIO83	// SDA	cd3	cd3_pad	GPIO83
#define  SCL_GPIO_GROUP2	GPIO80	//	SCL	cd0	cd0_pad	GPIO80
	
#define  SDA_GPIO_GROUP3	GPIO71
#define  SCL_GPIO_GROUP3	GPIO70
	
#else
	
#define	 SDA_GPIO_GROUP2	GPIO103		// SDA [17:16]	uartrxd0	uartrxd0_pad	GPIO108	uart0_UARTRXD
#define  SCL_GPIO_GROUP2	GPIO104		//	SCL LCD_D23/CPU_RST/GPIO75

#define  SDA_GPIO_GROUP3	GPIO71
#define  SCL_GPIO_GROUP3	GPIO70

#endif
	
#else
	
#define  SDA_GPIO_GROUP2	GPIO_S_2
#define  SCL_GPIO_GROUP2	GPIO_S_3

#define  SDA_GPIO_GROUP3	GPIO71
#define  SCL_GPIO_GROUP3	GPIO70
	
#endif


#define DURATION_CONVERT  	(20)

#define DURATION_START_1		(60)
#define DURATION_START_2		(60)
#define DURATION_START_3		(60)

#define DURATION_STOP_1  		(80)
#define DURATION_STOP_2		(60)
#define DURATION_STOP_3		(130)

#define DURATION_HIGH			(90)    
#define DURATION_LOW			(180)  

#define DELAY(DURATION)		{volatile unsigned short i; for(i = 1; i <= DURATION; i++){__asm("nop");}}


typedef struct
{
	unsigned char DeviceAddr;
	unsigned char wrdata;
	unsigned char I2cGroup; //i2c channel select 
	
	unsigned char reg_size;	
	unsigned char reg[4];

}GPIO_I2C;



int Write_GPIO_I2C(unsigned char slvAddr,unsigned char *pi2cData,unsigned int length);
int Read_GPIO_I2C(unsigned char slvAddr,unsigned char *pi2cData,unsigned int length);
void GPIO_I2C_Init(void);
void GpioI2cWriteByte(UINT8 data,const GPIO_I2C  *I2cInfo);
void GpioI2cReadByte(UINT8 *data,const GPIO_I2C  *I2cInfo);
int GpioI2cSendAck(const GPIO_I2C  *I2cInfo);
int GpioI2cSendNAck(const GPIO_I2C  *I2cInfo);
int GpioI2cCheckAck(const GPIO_I2C  *I2cInfo);
void GpioI2cStart(const GPIO_I2C  *I2cInfo);
void GpioI2cStop(const GPIO_I2C  *I2cInfo);

int rxchip_i2c_read_bytes  (unsigned int slvaddr, unsigned char* addr, int size, unsigned char *rxdata, int length);
int rxchip_i2c_write_bytes (unsigned int slvaddr, unsigned char* addr, int size, unsigned char *txdata, int length);
void group2_i2c_pad_init(void);

#ifdef __cplusplus
}
#endif

#endif

