#include "hardware.h"
#include "gpio.h"
#include "gpioi2c.h"
#include "xm_core.h"
#include "xm_printf.h"
#include "xm_dev.h"
#include "..\..\app\app.h"
#include <xm_video_task.h>
#include "common.h"
#include "xm_dev.h"
#include "xm_i2c.h"
#include "pr1000.h"

#ifdef _XMSYS_PR1000_SUPPORT_

#define I2C_RETRY_NUM				3

static OS_TASK TCB_PR1000_Task;
__no_init static OS_STACKPTR int StackPR1000Task[XMSYS_PR1000_TASK_STACK_SIZE/4];          /* Task stacks */
	
//#define	AHD_HW_I2C

#ifdef AHD_HW_I2C

#undef ahd_i2c_read_bytes
#undef ahd_i2c_write_bytes

static xm_i2c_client_t ahd_device = NULL;
static unsigned int ahd_address = 0;

#define PR1000_I2C_SLVADDRS  (0Xb8>>1)

static int ahd_i2c_write_bytes (unsigned int slvaddr, unsigned char* addr, int size, const char *txdata, int length)
{
	int ret = -1;
	int io_ret = 0;
	do
	{
		// 检查ahd的i2c地址是否一致.
		// 若不一致(从正常模式 <--> 编程模式 之间切换, i2c地址变化), 关闭当前的i2c设备, 然后重新打开
		if(ahd_device && ahd_address != slvaddr)
		{
			xm_i2c_close (ahd_device);
			ahd_device = NULL;
			ahd_address = 0;
		}

		if(ahd_device == NULL)
		{
			ahd_device = xm_i2c_open (XM_I2C_DEV_0,
								 slvaddr,		
								 "PR1000",
								 XM_I2C_ADDRESSING_7BIT,
								 &io_ret);
			if(ahd_device == NULL)
				break;
			ahd_address = slvaddr;
		}
		 
		if(xm_i2c_write (ahd_device, (u8_t *)addr, size, 100) < 0)
			break;

		if(xm_i2c_write (ahd_device, (u8_t *)txdata, length, 100) < 0)
		{
			break;
		}
		ret = 0;
		
	} while (0);

	/*
	if(ahd_device)
		xm_i2c_close (ahd_device);
	*/
	return ret;
}

static int ahd_i2c_read_bytes (unsigned int slvaddr, unsigned char* addr, int size, char *rxdata, int length)
{
	int io_ret = 0;
	int ret = -1;
	
	do
	{
		// 检查ahd 的i2c地址是否一致.
		// 若不一致(从正常模式 <--> 编程模式 之间切换, i2c地址变化), 关闭当前的i2c设备, 然后重新打开
		if(ahd_device && ahd_address != slvaddr)
		{
			xm_i2c_close (ahd_device);
			ahd_device = NULL;
			ahd_address = 0;
		}
		
		if(ahd_device == NULL)
		{
			ahd_device = xm_i2c_open (XM_I2C_DEV_0,
								 slvaddr,		
								 "PR1000",
								 XM_I2C_ADDRESSING_7BIT,
								 &io_ret);
			if(ahd_device == NULL)
				break;
			ahd_address = slvaddr;
		}
		 
		if(xm_i2c_write (ahd_device, (u8_t *)addr, size, 100) < 0)
			break;
		
		if(xm_i2c_read  (ahd_device, (u8_t *)rxdata, length, 100) != length)
			break;

		ret = length;
		
	} while (0);

	/*
	if(ahd_device)
		xm_i2c_close (ahd_device);
	*/
	return ret;
}

#else
#define PR1000_I2C_SLVADDRS  (0Xb8)

static int ahd_i2c_read_bytes (unsigned int slvaddr, unsigned char* addr, int size, char *rxdata, int length)
{
	GPIO_I2C gpio_i2c;
	gpio_i2c.DeviceAddr = (unsigned char)(slvaddr );
	gpio_i2c.I2cGroup = 1;	// 使用第一组IO脚
	memcpy (gpio_i2c.reg, addr, size);
	gpio_i2c.reg_size = size;
	return GpioI2cDeviceReadBytes (&gpio_i2c, (unsigned char *)rxdata, length);	
}

static int ahd_i2c_write_bytes (unsigned int slvaddr, unsigned char* addr, int size, const char *txdata, int length)
{
	GPIO_I2C gpio_i2c;
	gpio_i2c.DeviceAddr = (unsigned char)(slvaddr );
	gpio_i2c.I2cGroup = 1;	// 使用第一组IO脚
	memcpy (gpio_i2c.reg, addr, size);
	gpio_i2c.reg_size = size;
	return GpioI2cDeviceWriteBytes (&gpio_i2c, (unsigned char *)txdata, length);		
}

#endif

static unsigned char pr1000_i2c_write_bytes(unsigned short addr, const char *txdata, int length)
{
	int ret = -1;
	int retries = 0;
	unsigned char reg ;

	reg = addr;
	while(retries < I2C_RETRY_NUM)
	{
		ret = ahd_i2c_write_bytes (PR1000_I2C_SLVADDRS, &reg, 1, txdata, length);
		if(ret == 0)
			break;

		retries++;
	}
	if(retries >= I2C_RETRY_NUM)
	{
		XM_printf("%s i2c write error: %d\n", __func__, ret);
		ret = -1;
	}	
	return ret;

}

static unsigned char pr1000_i2c_read_bytes(unsigned short addr, char *rxdata, int length)
{
	int ret = -1;
	int retries = 0;
	unsigned char reg ;

	reg = addr;
	while(retries < I2C_RETRY_NUM)
	{
		ret = ahd_i2c_read_bytes (PR1000_I2C_SLVADDRS, &reg, 1, rxdata, length);
		if(ret == length)
			break;

		retries++;
	}
	if(retries >= I2C_RETRY_NUM)
	{
		XM_printf("%s i2c read error: %d, rxdata_len = %d\n", __func__, ret, length);
		ret = -1;
	}	
	return ret;
}

int PR1000_I2C_DRV_Write(unsigned int I2cNum, char I2cDevAddr, unsigned int I2cRegAddr, unsigned int I2cRegAddrByteNum, unsigned int Data, unsigned int DataLen)
{

	int ret = 0;
	
   	ret = pr1000_i2c_write_bytes(I2cRegAddr, (const char *)&Data,DataLen);	
	return ret;
}

int PR1000_I2C_DRV_Read(unsigned int I2cNum, char I2cDevAddr, unsigned int I2cRegAddr, unsigned int I2cRegAddrByteNum, uint8_t *pData, unsigned int DataLen)
{
	int ret = 0;
	
   	ret = pr1000_i2c_read_bytes(I2cRegAddr,pData,DataLen);
	return ret;
}

int PR1000_I2C_DRV_WriteBurst(unsigned int I2cNum, char I2cDevAddr, unsigned int I2cRegAddr, unsigned int I2cRegAddrByteNum, const uint8_t *pData, unsigned int DataLen)
{
   	int ret = 0;
	
	ret = pr1000_i2c_write_bytes(I2cRegAddr, pData,DataLen);	
    return ret;
}

int PR1000_I2C_DRV_ReadBurst(unsigned int I2cNum, char I2cDevAddr, unsigned int I2cRegAddr, unsigned int I2cRegAddrByteNum, unsigned int DataLen, uint8_t *pRetData)
{
    int ret = 0;
	
	ret = pr1000_i2c_read_bytes(I2cRegAddr, pRetData,DataLen);	
    return ret;
}

int PR1000_PageWrite(const int fd, unsigned char slave, int page, unsigned char reg, unsigned char value)
{
   	int ret = 0;
	if(page >= 0) ret = PR1000_I2C_DRV_Write(1, slave, 0xFF, 1, page, 1);
	ret = PR1000_I2C_DRV_Write(1, slave, reg, 1, value, 1);
	return(ret);
}

int PR1000_PageRead(const int fd, unsigned char slave, int page, unsigned char reg, unsigned char *pRet)
{
    int ret = 0;
	if(page >= 0) ret = PR1000_I2C_DRV_Write(1, slave, 0xFF, 1, page, 1);
	ret =  PR1000_I2C_DRV_Read(1, slave, reg, 1, pRet, 1);
	return(ret);
}

int PR1000_WriteMaskBit(const int fd, unsigned char slave, int page, unsigned char reg, unsigned char regMaskBit, unsigned char value)
{
	int ret = 0;
	uint8_t data = 0;

	if(page >= 0) ret = PR1000_I2C_DRV_Write(1, slave, 0xFF, 1, page, 1);
	ret =  PR1000_I2C_DRV_Read(1, slave, reg, 1, &data, 1);
	data &= ~(regMaskBit);
	data |= (value & regMaskBit);
	ret = PR1000_I2C_DRV_Write(1, slave, reg, 1, data, 1);
	return(ret);
}

int PR1000_ReadMaskBit(const int fd, unsigned char slave, int page, unsigned char reg, unsigned char regMaskBit, unsigned char *pRet)
{
	int ret = 0;
	uint8_t data = 0;

	if(page >= 0) ret = PR1000_I2C_DRV_Write(1, slave, 0xFF, 1, page, 1);
	ret =  PR1000_I2C_DRV_Read(1, slave, reg, 1, &data, 1);
	*pRet = data & regMaskBit;
	return(ret);
}


int PR1000_PageReadBurst(const int fd, unsigned char slave, int page, unsigned char reg, unsigned short length, unsigned char *pRetData)
{
	int ret = 0;

	if(page >= 0) ret = PR1000_I2C_DRV_Write(1, slave, 0xFF, 1, page, 1);
	ret =  PR1000_I2C_DRV_ReadBurst(1, slave, reg, 1, length, pRetData);
	
	return(ret);
}

int PR1000_PageWriteBurst(const int fd, unsigned char slave, int page, unsigned char reg, unsigned short length, const unsigned char *pData)
{
	int ret = 0;

	if(page >= 0) ret = PR1000_I2C_DRV_Write(1, slave, 0xFF, 1, page, 1);
	ret = PR1000_I2C_DRV_WriteBurst(1, slave, reg, 1, pData, length);
	return(ret);
}


void PR1000_Reset()
{
	// ITUB_D10 GPIO28
	unsigned int val;
	XM_lock();
	val = rSYS_PAD_CTRL02;
	val &= ~(0x07 << 24);
	rSYS_PAD_CTRL02 = val;
	SetGPIOPadDirection (GPIO28, euOutputPad);
	XM_unlock();

	XM_lock ();	
	SetGPIOPadData (GPIO28, euDataHigh);
	XM_unlock();
	
	OS_Delay(200); 
	XM_lock ();	
	SetGPIOPadData (GPIO28, euDataLow);
	XM_unlock();	
	
	OS_Delay (200); 
	XM_lock ();	
	SetGPIOPadData (GPIO28, euDataHigh);
	XM_unlock();
	OS_Delay (200);
}

extern int SetOutChn(int chn);
static int  Pr1000_Probe(void)
{
	uint8_t i2cReg = 0;
	uint8_t i2cData = 0;
	uint8_t i2cMask = 0;
	int page;
	int ret = 0;
		
	PR1000_Reset();
	ret= pr1000_module_init();
	
	/*****************************输出ITU601配置***************************************/
	/*
	Select the Output Mode for GPIO[3:0]
	GPIO[3:0] for P_MPP[4:1]
	0 : GPIO Output
	1 : Reserved
	*/
	PR1000_PageWrite(0, PR1000_I2C_SLVADDRS, 0, 0xd8, 0x00);

	/*
	GPIO[3:0] for P_MPP[4:1]
	0:Output Mode
	1:Input Mode
	EN_MPP1&2
	*/
	PR1000_PageWrite(0, PR1000_I2C_SLVADDRS, 0, 0xd0, 0xcc);

	/*
	MPP1 601IN2_HS/IR
	VDCK->148.5/144/108MHz Clock
	*/
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xF2 + (0*3);
	i2cMask = 0xF0; i2cData = 0x00;
	PR1000_WriteMaskBit(0, PR1000_I2C_SLVADDRS, page, i2cReg, i2cMask, i2cData);
	
	/*
	MPP2 601IN2_VS
	VDCK->148.5/144/108MHz Clock
	*/
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xF2 + (1*3);
	i2cMask = 0xF0; i2cData = 0x10;
	PR1000_WriteMaskBit(0, PR1000_I2C_SLVADDRS, page, i2cReg, i2cMask, i2cData);

	/*
	Select the Channel on 1 st Data for VD1/2/3/4 Pin
	*/
	page = PR1000_REG_PAGE_COMMON;
	i2cReg = 0xF0 + (1*3);
	i2cMask = 0x07; i2cData = 0x04;
	PR1000_WriteMaskBit(0, PR1000_I2C_SLVADDRS, page, i2cReg, i2cMask, i2cData);
	/******************************************************************************/
	
	XM_printf("PR1000 Device Probe %s\n", (ret < 0) ? "FAIL" : "PASS");
	
	return ret;
}

static void pr1000_task (void)
{
	int probe_count = 3;
	uint8_t i2cData = 0;

   	XM_printf("PR1000 Probe ...\n");

	while(Pr1000_Probe () < 0)
	{
		if(gpDrvHost)
		{
			kernel_free (gpDrvHost);
			gpDrvHost = NULL;
		}
		OS_Delay (100);
		probe_count --;
		if(probe_count == 0)
		{
		    XM_printf("pr1000 Task Terminate\n");
			OS_Terminate (NULL);
		}
	}
	
	DRV7116_PlugIn();//发送消息给656 开启接受动作
	Notification_Itu601_Isp_Scaler_Init();
	
	while(1)
 	{
 		/*
		PR1000_PageRead(0, PR1000_I2C_SLVADDRS, 0, 0x20, &i2cData);
		XM_printf("page0 0x20 =%x\n",i2cData);

		PR1000_PageRead(0, PR1000_I2C_SLVADDRS, 0, 0x21, &i2cData);
		XM_printf("page0 0x21 =%x\n",i2cData);
		*/
		OS_Delay (200);
 	}
}


void XMSYS_Pr1000Init (void)
{
    XM_printf("XMSYS_Pr1000Init\n");
	GPIO_I2C_Init ();
	OS_CREATETASK(&TCB_PR1000_Task, "PR1000", pr1000_task, XMSYS_PR1000_TASK_PRIORITY, StackPR1000Task);
}


void XMSYS_Pr1000Exit (void)
{
	
}


#endif

