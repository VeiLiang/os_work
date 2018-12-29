/**
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       IOCfg.c
    @ingroup    mIPRJAPCommonIO

    @brief      IO config module
                This file is the IO config module

    @note       Nothing.

    @date       2005/12/24
*/

/** \addtogroup mIPRJAPCommonIO */
//@{

#include "Type.h"
#include "DrvExt.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
#include "debug.h"
#include "IOCfg.h"
#include "Utility.h"
#include "Pll.h"
#include "IOInit.h"
//#include "Timer.h"
#include "GSensor.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          DxDrv
#define __DBGLVL__          1 // 0=OFF, 1=ERROR, 2=TRACE
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "DebugModule.h"
///////////////////////////////////////////////////////////////////////////////

#if (USE_VIO == ENABLE)
UINT32 Virtual_IO[VIO_MAX_ID] = {0};
UINT32 vio_getpin(UINT32 id){if(id>=VIO_MAX_ID)return 0; return Virtual_IO[id];}
void vio_setpin(UINT32 id, UINT32 v){if(id>=VIO_MAX_ID)return; Virtual_IO[id] = v;}
#endif

#define GPIO_SET_NONE           0xffffff
#define GPIO_SET_OUTPUT_LOW     0x0
#define GPIO_SET_OUTPUT_HI      0x1

GPIO_INIT_OBJ uiGPIOMapInitTab[] = {
    //CARD
    {  GPIO_CARD_POWER,         GPIO_DIR_OUTPUT,    GPIO_SET_OUTPUT_HI,   GPIO_SET_NONE         },
    {  GPIO_CARD_DETECT,        GPIO_DIR_INPUT,     PAD_PULLUP,           PAD_CARD_DETECT       },
    {  GPIO_CARD_WP,            GPIO_DIR_INPUT,     PAD_PULLUP,           PAD_CARD_WP           },
    //LCD
    //{  GPIO_LCD_BLG_PCTL,       GPIO_DIR_OUTPUT,    GPIO_SET_OUTPUT_LOW,  PAD_LCD_BLG_PCTL      },
    //LED
    {  GPIO_GREEN_LED,          GPIO_DIR_OUTPUT,    GPIO_SET_OUTPUT_LOW,  GPIO_SET_NONE         },
    {  GPIO_RED_LED,              GPIO_DIR_OUTPUT,    GPIO_SET_OUTPUT_HI,  GPIO_SET_NONE         },    
    //{  GPIO_WIFI_LED,           GPIO_DIR_OUTPUT,    GPIO_SET_OUTPUT_HI,  GPIO_SET_NONE         },
    //{  GPIO_REC_LED,           GPIO_DIR_OUTPUT,    GPIO_SET_OUTPUT_HI,  GPIO_SET_NONE         },    
    //KEY
 //   {  GPIO_KEY_SHUTTER2,           GPIO_DIR_INPUT,     PAD_PULLUP,           PAD_KEY_MODE          },
 //   {  GPIO_KEY_LEFT,           GPIO_DIR_INPUT,     PAD_PULLUP,           PAD_KEY_MODE          },    
    //Sensor
#if (_SENSORLIB_ == _SENSORLIB_CMOS_IMX322LQJ_)
    //Sensor
    {  GPIO_SENSOR_PWM1,        GPIO_DIR_OUTPUT,    GPIO_SET_OUTPUT_HI,  PAD_PIN_NOT_EXIST     },
    {  GPIO_SENSOR_PWM2,        GPIO_DIR_OUTPUT,    GPIO_SET_OUTPUT_HI,  PAD_PIN_NOT_EXIST     },
    {  GPIO_SENSOR_PWM3,        GPIO_DIR_OUTPUT,    GPIO_SET_OUTPUT_HI,  PAD_PIN_NOT_EXIST     },
    {  GPIO_SENSOR_RESET,       GPIO_DIR_OUTPUT,    GPIO_SET_OUTPUT_LOW,   PAD_PIN_NOT_EXIST     },
#else
    //Sensor
    {  GPIO_SENSOR_PWM0,        GPIO_DIR_OUTPUT,    GPIO_SET_OUTPUT_LOW,  PAD_PIN_NOT_EXIST     },
    {  GPIO_SENSOR_PWM1,        GPIO_DIR_OUTPUT,    GPIO_SET_OUTPUT_LOW,  PAD_PIN_NOT_EXIST     },
    {  GPIO_SENSOR_RESET,       GPIO_DIR_OUTPUT,    GPIO_SET_OUTPUT_HI,   PAD_PIN_NOT_EXIST     },
#endif	
    {  GPIO_SENSOR_STANDBY,     GPIO_DIR_OUTPUT,    GPIO_SET_OUTPUT_HI,   PAD_PIN_NOT_EXIST     },
    //Wi-Fi power
    {  GPIO_WIFI_POWER_PWM5,    GPIO_DIR_OUTPUT,    GPIO_SET_OUTPUT_HI,   PAD_PIN_NOT_EXIST     },
    {  GPIO_KEY_ENTER,         GPIO_DIR_INPUT,     PAD_PULLUP,           PAD_KEY_ENTER       },    

    {  GPIO_PASSWORD_SCK,       GPIO_DIR_OUTPUT,    PAD_PULLUP,   PAD_PASSWORD_SCK     },
    {  GPIO_PASSWORD_SDA,       GPIO_DIR_INPUT,    PAD_PULLDOWN,   PAD_PASSWORD_SDA     },
#if 0
     {  GPIO_KEY_ZOOMOUT,       GPIO_DIR_INPUT,     PAD_PULLUP,           PAD_KEY_ZOOMOUT       },
     {  GPIO_KEY_ZOOMIN,        GPIO_DIR_INPUT,     PAD_PULLUP,           PAD_KEY_ZOOMIN       },
     {  GPIO_KEY_LEFT,          GPIO_DIR_INPUT,     PAD_PULLUP,           PAD_KEY_LEFT       },
     {  GPIO_KEY_ENTER,         GPIO_DIR_INPUT,     PAD_PULLUP,           PAD_KEY_ENTER       },
     {  GPIO_KEY_UP,            GPIO_DIR_INPUT,     PAD_PULLUP,           PAD_KEY_UP       },
     {  GPIO_KEY_RIGHT,         GPIO_DIR_INPUT,     PAD_PULLUP,           PAD_KEY_RIGHT       },
     {  GPIO_KEY_PLAYBACK,      GPIO_DIR_INPUT,     PAD_PULLUP,           PAD_KEY_PLAYBACK       },
     {  GPIO_KEY_DOWN,          GPIO_DIR_INPUT,     PAD_PULLUP,           PAD_KEY_DOWN       },
     {  GPIO_KEY_MENU,          GPIO_DIR_INPUT,     PAD_PULLUP,           PAD_KEY_MENU       },

     //#NT#2012/07/25#Isiah Chang -begin
     //#NT#Added GPIO map for Lens&motor driver.
     {  GPIO_LENS_ZOOM_PI,      GPIO_DIR_INPUT,     PAD_PULLDOWN,         PAD_LENS_ZOOM_PI       },
     {  GPIO_LENS_ZOOM_PR,      GPIO_DIR_INPUT,     PAD_PULLDOWN,         PAD_LENS_ZOOM_PR       },
     {  GPIO_LENS_FOCUS_PI,     GPIO_DIR_INPUT,     PAD_PULLDOWN,         PAD_LENS_FOCUS_PI      },

     {  GPIO_LENS_IN1A,         GPIO_DIR_OUTPUT,    PAD_NONE,             PAD_PIN_NOT_EXIST      },
     {  GPIO_LENS_IN1B,         GPIO_DIR_OUTPUT,    PAD_NONE,             PAD_PIN_NOT_EXIST      },
     {  GPIO_LENS_IN2A,         GPIO_DIR_OUTPUT,    PAD_NONE,             PAD_PIN_NOT_EXIST      },
     {  GPIO_LENS_IN2B,         GPIO_DIR_OUTPUT,    PAD_NONE,             PAD_PIN_NOT_EXIST      },
     {  GPIO_LENS_IN3A,         GPIO_DIR_OUTPUT,    PAD_NONE,             PAD_PIN_NOT_EXIST      },
     {  GPIO_LENS_IN3B,         GPIO_DIR_OUTPUT,    PAD_NONE,             PAD_PIN_NOT_EXIST      },
     {  GPIO_LENS_IN4A,         GPIO_DIR_OUTPUT,    PAD_NONE,             PAD_PIN_NOT_EXIST      },
     {  GPIO_LENS_IN4B,         GPIO_DIR_OUTPUT,    PAD_NONE,             PAD_PIN_NOT_EXIST      },
     //#NT#2012/07/25#Isiah Chang -end
#endif
};

UINT32 totalGpioInitCount = sizeof(uiGPIOMapInitTab)/sizeof((uiGPIOMapInitTab)[0]);

void IO_InitGensor(void);

#include "rtc.h"

//should be call after rtc_open()
void IO_GetPowerSrc(void)
{
    UINT32 pwrlost, pwsrc;
    pwrlost = rtc_isPowerLost();
    if(pwrlost)
    {
        DBG_DUMP("^GPowerOn Pwr Lost!\r\n"); //"firs time power-on" or "lost power of Gold capacitor"

        //should notify user to configure current date-time!
    }

    pwsrc = rtc_getPWROnSource();
    if(pwsrc == RTC_PWRON_SRC_PWR_SW)
    {
        debug_err(("^GPowerOn Src = PWR key\r\n"));
    }
    else if(pwsrc == RTC_PWRON_SRC_PWR_SW2)
    {
       debug_err(("^GPowerOn Src = PB Key\r\n"));
    }
    else if(pwsrc == RTC_PWRON_SRC_PWR_SW3)
    {
        debug_err(("^GPowerOn Src = USB plug\r\n"));
    }
    else if(pwsrc == RTC_PWRON_SRC_PWR_SW4)
    {
        debug_err(("^GPowerOn Src = DC plug\r\n"));
    }
    else if(pwsrc == RTC_PWRON_SRC_PWR_ALM)
    {
        debug_err(("^GPowerOn Src = PWR alarm\r\n"));
    }
}

/**
  Do GPIO initialization

  Initialize input/output pins, and pin status

  @param void
  @return void
*/
void IO_InitGPIO(void)
{
    UINT32 iSValue;

    DBG_IND("GPIO START\r\n");
    //--------------------------------------------------------------------
    // Open GPIO driver
    //--------------------------------------------------------------------
    #if 1 //_MIPS_TODO
    gpio_open();
    for(iSValue=0 ; iSValue<totalGpioInitCount ; iSValue++)
    {
        if (uiGPIOMapInitTab[iSValue].GpioDir == GPIO_DIR_INPUT)
        {
            gpio_setDir(uiGPIOMapInitTab[iSValue].GpioPin, GPIO_DIR_INPUT);
            pad_setPullUpDown(uiGPIOMapInitTab[iSValue].PadPin, uiGPIOMapInitTab[iSValue].PadDir);
        }
        else
        {
            gpio_setDir(uiGPIOMapInitTab[iSValue].GpioPin, GPIO_DIR_OUTPUT);
            if (uiGPIOMapInitTab[iSValue].PadDir == GPIO_SET_OUTPUT_HI)
            {
                gpio_setPin(uiGPIOMapInitTab[iSValue].GpioPin);
            }
            else
            {
                gpio_clearPin(uiGPIOMapInitTab[iSValue].GpioPin);
            }
        }
    }
/*
pad_setDrivingSink(PAD_DS_CGPIO22, PAD_DRIVINGSINK_15MA);	
pad_setDrivingSink(PAD_DS_CGPIO23, PAD_DRIVINGSINK_15MA);	
pad_setDrivingSink(PAD_DS_CGPIO24, PAD_DRIVINGSINK_15MA);	
pad_setDrivingSink(PAD_DS_CGPIO25, PAD_DRIVINGSINK_15MA);	
pad_setDrivingSink(PAD_DS_CGPIO26, PAD_DRIVINGSINK_15MA);	
pad_setDrivingSink(PAD_DS_CGPIO27, PAD_DRIVINGSINK_15MA);	*/
    #endif

    //--------------------------------------------------------------------
    // Use Non-Used PWM to be Delay Timer
    //--------------------------------------------------------------------
    #if defined(PWMID_TIMER)
    Delay_setPwmChannels(PWMID_TIMER);
    #endif

    DBG_IND("GPIO END\r\n");

}


#define GSENSOR_TYPE_NONE    0XFF
#define GSENSOR_TYPE_GMA301   0x0
#define GSENSOR_TYPE_DA380     0x1
#define GSENSOR_TYPE_GMA30x   0x2

#define GSENSOR_TPYE   GSENSOR_TYPE_DA380 

#if(GSENSOR_TPYE==GSENSOR_TYPE_DA380)

#define NSA_REG_SPI_I2C                 0x00
#define NSA_REG_WHO_AM_I                0x01
#define NSA_REG_ACC_X_LSB               0x02
#define NSA_REG_ACC_X_MSB               0x03
#define NSA_REG_ACC_Y_LSB               0x04
#define NSA_REG_ACC_Y_MSB               0x05
#define NSA_REG_ACC_Z_LSB               0x06
#define NSA_REG_ACC_Z_MSB               0x07 
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

#define   DA380_ID          0x13 /* WHO AM I register data */

static GSENSOR_INFO g_GsensorInfo;

void DA380_Mask_Write(unsigned char addr, unsigned char mask, unsigned char data)
{
    unsigned char      tmp_data;

    tmp_data = GSensor_I2C_ReadReg(addr);
    tmp_data &= ~mask;
    tmp_data |= data & mask;
    GSensor_I2C_WriteReg(addr, tmp_data);
}

BOOL CheckGsensor(UINT32 GSensitivity)
{
    UINT8 gs_data[6];
    INT16 xRawData,yRawData,zRawData;
    static INT16 pre_xRawData = 0,pre_yRawData = 0,pre_zRawData = 0;
    static UINT8 b_is_first = 0;

    gs_data[0] = GSensor_I2C_ReadReg(NSA_REG_ACC_X_LSB);

    gs_data[1] = GSensor_I2C_ReadReg(NSA_REG_ACC_X_MSB);

    gs_data[2] = GSensor_I2C_ReadReg(NSA_REG_ACC_X_LSB+1+1);

    gs_data[3] = GSensor_I2C_ReadReg(NSA_REG_ACC_X_LSB+1+1+1);

    gs_data[4] = GSensor_I2C_ReadReg(NSA_REG_ACC_X_LSB+1+1+1+1);

    gs_data[5] = GSensor_I2C_ReadReg(NSA_REG_ACC_X_LSB+1+1+1+1+1);

    xRawData= ((INT16)(gs_data[1] << 8 | gs_data[0]))>> 4;//4060
    yRawData = ((INT16)(gs_data[3] << 8 | gs_data[2]))>> 4;//4070
    zRawData = ((INT16)(gs_data[5] << 8 | gs_data[4]))>> 4;//3785
//debug_err((">>>>X = %d Y = %d Z = %d S = %d\r\n",xRawData,yRawData,zRawData,GSensitivity));
    if(b_is_first == 0)
    {
        pre_xRawData = xRawData;
        pre_yRawData = yRawData;
        pre_zRawData = zRawData;

        b_is_first = 1;

        return FALSE;
    }
/*
    if(0)//park_recorder ==1)
    {
        //if((abs(xRawData - pre_xRawData)> 280)||(abs(yRawData - pre_yRawData) > 280)||(abs(zRawData - pre_zRawData) > 280))
        //    return Gsensor_Mode_Crash;

    }
    else */
    //debug_err((">>>>X = %d Y = %d Z = %d S = %d\r\n",xRawData,yRawData,zRawData,GSensitivity));
    //if((abs(xRawData - pre_xRawData)> GSensitivity*300)||(abs(yRawData - pre_yRawData) > GSensitivity*300)||(abs(zRawData - pre_zRawData) > GSensitivity*300))
    if((abs(xRawData - pre_xRawData)> (4-GSensitivity)*200)||(abs(yRawData - pre_yRawData) > (4-GSensitivity)*200)||(abs(zRawData - pre_zRawData) > (4-GSensitivity)*200))
    {
        if(GSensitivity == 0)
            return FALSE;

        pre_xRawData = xRawData;
        pre_yRawData = yRawData;
        pre_zRawData = zRawData;
        return TRUE;
    }

    pre_xRawData = xRawData;
    pre_yRawData = yRawData;
    pre_zRawData = zRawData;

    return FALSE;
}

void DA380_open_parking_interrupt(int num)
{
	int   res = 0;

	GSensor_I2C_WriteReg(NSA_REG_INTERRUPT_SETTINGS1,0x03);
	GSensor_I2C_WriteReg(NSA_REG_ACTIVE_DURATION,0x03 );
	 GSensor_I2C_WriteReg(NSA_REG_ACTIVE_THRESHOLD,0x26);////DEBUG//38
			
	switch(num){

		case 0:
			GSensor_I2C_WriteReg(NSA_REG_INTERRUPT_MAPPING1,0x04 );
			break;

		case 1:
			GSensor_I2C_WriteReg(NSA_REG_INTERRUPT_MAPPING3,0x04 );
			break;
	}

//	return res;
}
/**
  Initialize Gsensor IIC bus

  Initialize Gsensor IIC bus

  @param void
  @return void
*/
void IO_InitGensor(void)
{
   GSENSOR_INFO GsensorInfo;
   UINT32 ulData;
    GsensorInfo.I2C_RegBytes = GSENSOR_I2C_REGISTER_1BYTE;
    GsensorInfo.I2C_PinMux = I2C_PINMUX_1ST;
    GsensorInfo.I2C_BusClock = I2C_BUS_CLOCK_400KHZ;
    // DMARD07 GSensor I2C slave addresses

    GsensorInfo.I2C_Slave_WAddr = 0x4E;
    GsensorInfo.I2C_Slave_RAddr = 0x4F;    

    if (GSensor_I2C_Init(GsensorInfo) == TRUE)
    {
        ////check MM3A310_DA380 chip ID
        ulData = GSensor_I2C_ReadReg(NSA_REG_WHO_AM_I);
        if(DA380_ID != ulData)
        {
            debug_err((">>>>>DA380 not match\r\n"));
        }
        else
        {
            debug_err((">>>>DA380\r\n"));
        }
        DA380_Mask_Write(NSA_REG_SPI_I2C, 0x24, 0x24);

        Delay_DelayMs(5);////5ms

        DA380_Mask_Write(NSA_REG_G_RANGE, 0x03, 0x02);
        DA380_Mask_Write(NSA_REG_POWERMODE_BW, 0xFF, 0x1E);
        DA380_Mask_Write(NSA_REG_ODR_AXIS_DISABLE, 0xFF, 0x07);

        DA380_Mask_Write(NSA_REG_INT_PIN_CONFIG, 0x0F, 0x05);//set int_pin level
        DA380_Mask_Write(NSA_REG_INT_LATCH, 0x8F, 0x81);//clear latch and set latch mode

        DA380_Mask_Write(NSA_REG_ENGINEERING_MODE, 0xFF, 0x83);
        DA380_Mask_Write(NSA_REG_ENGINEERING_MODE, 0xFF, 0x69);
        DA380_Mask_Write(NSA_REG_ENGINEERING_MODE, 0xFF, 0xBD);
        Delay_DelayMs(10);     
     
    } 
    else 
    {
        debug_msg("G Sensor Init failed !!\r\n");
    }
}
#endif

#if(GSENSOR_TPYE==GSENSOR_TYPE_GMA301)
GSENSOR_INFO ui_GsensorInfo;
static I2C_STS GsSensor_I2C_Receive(UINT32 *value, BOOL bNACK, BOOL bStop)
{
    I2C_DATA I2cData;
    I2C_STS ret;

    I2cData.VersionInfo = DRV_VER_96650;
    I2cData.ByteCount = I2C_BYTE_CNT_1;
    I2cData.Byte[0].Param = I2C_BYTE_PARAM_NONE;
    if ( bNACK == TRUE )
        I2cData.Byte[0].Param |= I2C_BYTE_PARAM_NACK;
    if ( bStop == TRUE )
        I2cData.Byte[0].Param |= I2C_BYTE_PARAM_STOP;

    ret = i2c_receive(&I2cData);
    if ( ret != I2C_STS_OK )
    {
        DBG_ERR("i2c ret = %02x!!\r\n", ret);
    }

    *value = I2cData.Byte[0].uiValue;

    return ret;
}

static I2C_STS GsSensor_I2C_Transmit(UINT32 value, BOOL bStart, BOOL bStop)
{
    I2C_DATA I2cData;
    I2C_STS ret;

    I2cData.VersionInfo = DRV_VER_96650;
    I2cData.ByteCount = I2C_BYTE_CNT_1;
    I2cData.Byte[0].uiValue = value & 0xff;
    I2cData.Byte[0].Param = I2C_BYTE_PARAM_NONE;
    if ( bStart == TRUE )
        I2cData.Byte[0].Param |= I2C_BYTE_PARAM_START;
    if ( bStop == TRUE )
        I2cData.Byte[0].Param |= I2C_BYTE_PARAM_STOP;
    ret = i2c_transmit(&I2cData);
    if ( ret != I2C_STS_OK )
    {
            DBG_ERR("i2c ret = %d!!\r\n", ret);
    }
    return ret;
}
UINT32 GSensor_I2C_ReadReg_2B(UINT32 uiAddr, UINT32 uidata)
{
  UINT32      ulWriteAddr, ulReadAddr, ulReg1;
  static UINT32 ulData1,ulData2;

    ulWriteAddr =0x30;
    ulReadAddr  =0x31;
    ulReg1      =(uiAddr&0x000000ff);
    ulData1     = 0;
    ulData2 = 0;
//debug_err((">>> read register\r\n"));
    if(i2c_lock(ui_GsensorInfo.I2C_Channel) != E_OK)
    {
        debug_err(("GSensor: readReg I2C Lock Fail\r\n"));
        return ulData1;
    }

    if (GsSensor_I2C_Transmit(ulWriteAddr, 1, 0) != I2C_STS_OK)
    {
        debug_err(("Error transmit data1!!\r\n"));
        i2c_unlock(ui_GsensorInfo.I2C_Channel);
        return  ulData1;
    }

    if (GsSensor_I2C_Transmit(ulReg1, 0, 0) != I2C_STS_OK)
    {
        debug_err(("Error transmit data2!!\r\n"));
        i2c_unlock(ui_GsensorInfo.I2C_Channel);
        return  ulData1;
    }

    if (GsSensor_I2C_Transmit(ulReadAddr, 1, 0) != I2C_STS_OK)
    {
        debug_err(("Error transmit data3!!\r\n"));
        i2c_unlock(ui_GsensorInfo.I2C_Channel);
        return  ulData1;
    }

    if (GsSensor_I2C_Receive(&ulData1, 0, 0) != I2C_STS_OK)
    {
        debug_err(("Error Receive data!!\r\n"));
        i2c_unlock(ui_GsensorInfo.I2C_Channel);
        return  ulData1;
    }

    if (GsSensor_I2C_Receive(&ulData2, 1, 1) != I2C_STS_OK)
    {
        debug_err(("Error Receive data!!\r\n"));
        i2c_unlock(ui_GsensorInfo.I2C_Channel);
        return  ulData1;
    }
    if(i2c_unlock(ui_GsensorInfo.I2C_Channel) != E_OK)
    {
        debug_err(("GSensor: readReg I2C UnLock Fail\r\n"));
        return  ulData1;
    }
   // debug_err((">>>>>uidata = %d %d\r\n",ulData1,ulData2));

   uidata =  (ulData1<<8 + ulData2);
   //debug_err((">>>uidata = %d\r\n",uidata));
    return  uidata;
}

BOOL CheckGsensor(UINT32 GSensitivity)
{
    UINT32 value;
        GSensor_I2C_ReadReg_2B(0x1C,value);
        debug_msg("reg 0x12:%x\r\n",GSensor_I2C_ReadReg(0x12));//*****2014_0819 added for GMA301 X, Y, Z data read.*****
        debug_msg("reg 0x14:%x\r\n",GSensor_I2C_ReadReg(0x14));
        debug_msg("reg 0x15:%x\r\n",GSensor_I2C_ReadReg(0x15));
        debug_msg("reg 0x16:%x\r\n",GSensor_I2C_ReadReg(0x16));
        debug_msg("reg 0x17:%x\r\n",GSensor_I2C_ReadReg(0x17));
        debug_msg("reg 0x18:%x\r\n",GSensor_I2C_ReadReg(0x18));   
        debug_msg("reg 0x19:%x\r\n",GSensor_I2C_ReadReg(0x19));//*****2014_0819 modified because it ReadReg(0x12) is incorrectly.*****

}

void IO_InitGensor(void)
{
  GSENSOR_INFO GsensorInfo;
  UINT32 value;

    GsensorInfo.I2C_RegBytes = GSENSOR_I2C_REGISTER_1BYTE;
    GsensorInfo.I2C_PinMux = I2C_PINMUX_1ST;
    GsensorInfo.I2C_BusClock = I2C_BUS_CLOCK_400KHZ;
    GsensorInfo.I2C_Slave_WAddr = 0x30;
    GsensorInfo.I2C_Slave_RAddr = 0x31;
    //debug_err((">>> GMA301\r\n"));
    if (GSensor_I2C_Init(GsensorInfo) == TRUE)
    {
		//init
		GSensor_I2C_WriteReg(0x21,0x52);//
		GSensor_I2C_WriteReg(0x00,0x02);//
		GSensor_I2C_WriteReg(0x00,0x12);//
		GSensor_I2C_WriteReg(0x00,0x02);//
		GSensor_I2C_WriteReg(0x00,0x82);//
		GSensor_I2C_WriteReg(0x00,0x02);//
		GSensor_I2C_WriteReg(0x1F,0x28);//
		GSensor_I2C_WriteReg(0x0C,0x8F);//
		GSensor_I2C_WriteReg(0x00,0x06);//
	
		//interrupt setup
		GSensor_I2C_WriteReg(0x11,0x04);//IIC 0X07 for no pullup //0x06 High active  0x04 low active
		//Gsensor_WriteReg(0x11,0x06);//IIC 0X06 for no pullup 
		//set Gsensor Level 
		//0x08-->0.5G 
		//0X10-->1G
		GSensor_I2C_WriteReg(0x38,0XFF);//
		GSensor_I2C_WriteReg(0x39,0X30);//10 1g 20 2g 30 3g 40 4g 50 5g 60 6g  (8 : 0.5g)
		
		GSensor_I2C_WriteReg(0x0F,0x00);//
		GSensor_I2C_WriteReg(0x0E,0x00);//0x1C//0x00 // 0x00:disable interrupt
		GSensor_I2C_WriteReg(0x1F,0x28);//To disable micro motion interrupt.
		//TimerDelayMs(10);
		//Latched reference data of micro motion.
		GSensor_I2C_ReadReg(0x12);
		GSensor_I2C_ReadReg(0x13);
		GSensor_I2C_ReadReg(0x14);
		GSensor_I2C_ReadReg(0x15);
		GSensor_I2C_ReadReg(0x16);
		GSensor_I2C_ReadReg(0x17);
		GSensor_I2C_ReadReg(0x18);
		GSensor_I2C_ReadReg(0x19);
		//GSensor_I2C_ReadReg(0x1A);
		//GSensor_I2C_ReadReg(0x1B);
        /*
        debug_msg("reg 0x14:%x\r\n",GSensor_I2C_ReadReg(0x14));//X-H
        debug_msg("reg 0x15:%x\r\n",GSensor_I2C_ReadReg(0x15));//X-L
        debug_msg("reg 0x16:%x\r\n",GSensor_I2C_ReadReg(0x16));//Y-H
        debug_msg("reg 0x17:%x\r\n",GSensor_I2C_ReadReg(0x17));//Y-L
        debug_msg("reg 0x18:%x\r\n",GSensor_I2C_ReadReg(0x18));//Z-H
	 debug_msg("reg 0x19:%x\r\n",GSensor_I2C_ReadReg(0x19));//Z-L
*/
		GSensor_I2C_WriteReg(0x1F,0x38);//To enable micro motion interrupt.
		Delay_DelayMs(1); //2014_0819 added 1ms delay for micro motion setup itself.

		GSensor_I2C_WriteReg(0x0E,0x00);//To enable interrupt.//parking monitor
		GSensor_I2C_ReadReg_2B(0x1C,value);
	
    }
    else {
        debug_msg("G Sensor Init failed !!\r\n");
    }
}

#endif

#if(GSENSOR_TPYE==GSENSOR_TYPE_GMA30x)
// GMA30x
GSENSOR_INFO ui_GsensorInfo;
static I2C_STS GsSensor_I2C_Receive(UINT32 *value, BOOL bNACK, BOOL bStop)
{
    I2C_DATA I2cData;
    I2C_STS ret;

    I2cData.VersionInfo = DRV_VER_96650;
    I2cData.ByteCount = I2C_BYTE_CNT_1;
    I2cData.Byte[0].Param = I2C_BYTE_PARAM_NONE;
    if ( bNACK == TRUE )
        I2cData.Byte[0].Param |= I2C_BYTE_PARAM_NACK;
    if ( bStop == TRUE )
        I2cData.Byte[0].Param |= I2C_BYTE_PARAM_STOP;

    ret = i2c_receive(&I2cData);
    if ( ret != I2C_STS_OK )
    {
        DBG_ERR("i2c ret = %02x!!\r\n", ret);
    }

    *value = I2cData.Byte[0].uiValue;

    return ret;
}

static I2C_STS GsSensor_I2C_Transmit(UINT32 value, BOOL bStart, BOOL bStop)
{
    I2C_DATA I2cData;
    I2C_STS ret;

    I2cData.VersionInfo = DRV_VER_96650;
    I2cData.ByteCount = I2C_BYTE_CNT_1;
    I2cData.Byte[0].uiValue = value & 0xff;
    I2cData.Byte[0].Param = I2C_BYTE_PARAM_NONE;
    if ( bStart == TRUE )
        I2cData.Byte[0].Param |= I2C_BYTE_PARAM_START;
    if ( bStop == TRUE )
        I2cData.Byte[0].Param |= I2C_BYTE_PARAM_STOP;
    ret = i2c_transmit(&I2cData);
    if ( ret != I2C_STS_OK )
    {
            DBG_ERR("i2c ret = %d!!\r\n", ret);
    }
    return ret;
}
UINT32 GSensor_I2C_ReadReg_2B(UINT32 uiAddr, UINT32 uidata)
{
  UINT32      ulWriteAddr, ulReadAddr, ulReg1;
  static UINT32 ulData1,ulData2;

    ulWriteAddr =0x30;
    ulReadAddr  =0x31;
    ulReg1      =(uiAddr&0x000000ff);
    ulData1     = 0;
    ulData2 = 0;
//debug_err((">>> read register\r\n"));
    if(i2c_lock(ui_GsensorInfo.I2C_Channel) != E_OK)
    {
        debug_err(("GSensor: readReg I2C Lock Fail\r\n"));
        return ulData1;
    }

    if (GsSensor_I2C_Transmit(ulWriteAddr, 1, 0) != I2C_STS_OK)
    {
        debug_err(("Error transmit data1!!\r\n"));
        i2c_unlock(ui_GsensorInfo.I2C_Channel);
        return  ulData1;
    }

    if (GsSensor_I2C_Transmit(ulReg1, 0, 0) != I2C_STS_OK)
    {
        debug_err(("Error transmit data2!!\r\n"));
        i2c_unlock(ui_GsensorInfo.I2C_Channel);
        return  ulData1;
    }

    if (GsSensor_I2C_Transmit(ulReadAddr, 1, 0) != I2C_STS_OK)
    {
        debug_err(("Error transmit data3!!\r\n"));
        i2c_unlock(ui_GsensorInfo.I2C_Channel);
        return  ulData1;
    }

    if (GsSensor_I2C_Receive(&ulData1, 0, 0) != I2C_STS_OK)
    {
        debug_err(("Error Receive data!!\r\n"));
        i2c_unlock(ui_GsensorInfo.I2C_Channel);
        return  ulData1;
    }

    if (GsSensor_I2C_Receive(&ulData2, 1, 1) != I2C_STS_OK)
    {
        debug_err(("Error Receive data!!\r\n"));
        i2c_unlock(ui_GsensorInfo.I2C_Channel);
        return  ulData1;
    }
    if(i2c_unlock(ui_GsensorInfo.I2C_Channel) != E_OK)
    {
        debug_err(("GSensor: readReg I2C UnLock Fail\r\n"));
        return  ulData1;
    }
   // debug_err((">>>>>uidata = %d %d\r\n",ulData1,ulData2));

   uidata =  (ulData1<<8 + ulData2);
   //debug_err((">>>uidata = %d\r\n",uidata));
    return  uidata;
}

static BOOL g_GSFirstFlag = TRUE;
BOOL CheckGsensor(UINT32 GSensitivity)
{
 INT16  abs_cal_x=0,abs_cal_y=0,abs_cal_z=0;
 INT16 Threshold = 0,P_INT_COUNT = 0;
 INT16 tmp_data[6];
 INT16 Xdata,Ydata,Zdata,tmp,GsXData,GsYData,GsZData;
 static INT16 OldGsXData,OldGsYData,OldGsZData,Xdatab=0,Ydatab=0,Zdatab=0;
 UINT16 value;

    if (GSensitivity == 3)
        Threshold = 280;
    else if (GSensitivity == 2)
	  Threshold = 460;
    else if (GSensitivity == 1)
	  Threshold = 800;
    else
	  Threshold = 1024;

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
    Zdata = (signed short)(((tmp_data[5] << 8)  | tmp_data[4])&0xfffe);
    if (g_GSFirstFlag)
    {
       OldGsXData = Xdata;
       OldGsYData = Ydata;
       OldGsZData = Zdata;
       g_GSFirstFlag = FALSE;
    }
    //debug_err((">>>>GS X %d Y %d Z %d\r\n",(OldGsXData-Xdata),(OldGsYData-Ydata),(OldGsZData-Zdata)));
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

void GMA302_open_parking_interrupt(int num)
{
  UINT16 value;

  //GSensor_I2C_WriteReg(0x01,0x02);// Powerdown reset
  GSensor_I2C_WriteReg(0x16,0x1B);

  GSensor_I2C_WriteReg(0x03,0x01);
  //門檻設置,從 0x01 ~ 0x1F . 0x01 = 0.25G. 0x04:1G   0x1F:7.75G

  
  GSensor_I2C_ReadReg_2B(0x04,&value);
  GSensor_I2C_ReadReg(0x05);
  GSensor_I2C_ReadReg(0x06);
  GSensor_I2C_ReadReg(0x07);
  GSensor_I2C_ReadReg(0x08);
  GSensor_I2C_ReadReg(0x09);
  GSensor_I2C_ReadReg(0x0A);
  GSensor_I2C_ReadReg(0x0B);
  GSensor_I2C_WriteReg(0x15,0x4C);//啟動中斷 沒上拉電組--高電平輸出0x5D,低電平0x55
  											//				有上拉電組--高電平輸出0x4C,低電平0x44
  //debug_err((">>>>GMA303 parking\r\n"));
}
void IO_InitGensor(void)
{
  GSENSOR_INFO GsensorInfo;
  UINT32 value;

    GsensorInfo.I2C_RegBytes = GSENSOR_I2C_REGISTER_1BYTE;
    GsensorInfo.I2C_PinMux = I2C_PINMUX_1ST;
    GsensorInfo.I2C_BusClock = I2C_BUS_CLOCK_400KHZ;
    GsensorInfo.I2C_Slave_WAddr = 0x30;
    GsensorInfo.I2C_Slave_RAddr = 0x31;
    //debug_err((">>> GMA302\r\n"));
    if (GSensor_I2C_Init(GsensorInfo) == TRUE)
    {
       debug_err(("GAM303 = 0x%x\r\n",GSensor_I2C_ReadReg(0x04)));//0x55    
	 //init
	 GSensor_I2C_WriteReg(0x01,0x02);// Powerdown reset
	 GSensor_I2C_WriteReg(0x03,0x1F);//
	 GSensor_I2C_WriteReg(0x38,0x9F);//
	 GSensor_I2C_WriteReg(0x15,0x00);//0x00 low active 0x08 high active
	 GSensor_I2C_WriteReg(0x16,0x00);//
	 GSensor_I2C_WriteReg(0x02,0x02);//
	 GSensor_I2C_WriteReg(0x02,0x00);//
	 GSensor_I2C_WriteReg(0x02,0x04);//
	 GSensor_I2C_WriteReg(0x02,0x00);//"	
	 //debug_err((" GMA init end%x\r\n",GSensor_I2C_ReadReg(0x15)));
    }
    else {
        debug_msg("G Sensor Init failed !!\r\n");
    }
}
#endif

/**
  Initialize voltage detection

  Initialize voltage detection for battery and flash

  @param void
  @return void
*/

void IO_InitADC(void)
{

    if (adc_open(ADC_CH_VOLDET_BATTERY) != E_OK)
    {
        DBG_ERR("Can't open ADC channel for battery voltage detection\r\n");
        return;
    }

    if (adc_open(ADC_CH_VOLDET_KEY1) != E_OK)
    {
        DBG_ERR("Can't open ADC channel for key key1 detection\r\n");
        return;
    }
    if (adc_open(ADC_CH_VOLDET_KEY2) != E_OK)
    {
        DBG_ERR("Can't open ADC channel for key key1 detection\r\n");
        return;
    }

    //650 Range is 250K Hz ~ 2M Hz
    adc_setConfig(ADC_CONFIG_ID_OCLK_FREQ, 250000); //250K Hz

    //battery voltage detection
    adc_setChConfig(ADC_CH_VOLDET_BATTERY, ADC_CH_CONFIG_ID_SAMPLE_FREQ, 10000); //10K Hz, sample once about 100 us for CONTINUOUS mode
    adc_setChConfig(ADC_CH_VOLDET_BATTERY, ADC_CH_CONFIG_ID_SAMPLE_MODE, (VOLDET_ADC_MODE) ? ADC_CH_SAMPLEMODE_CONTINUOUS : ADC_CH_SAMPLEMODE_ONESHOT);
    adc_setChConfig(ADC_CH_VOLDET_BATTERY, ADC_CH_CONFIG_ID_INTEN, FALSE);

    //key key1 detection
    adc_setChConfig(ADC_CH_VOLDET_KEY1, ADC_CH_CONFIG_ID_SAMPLE_FREQ, 10000); //10K Hz, sample once about 100 us for CONTINUOUS mode
    adc_setChConfig(ADC_CH_VOLDET_KEY1, ADC_CH_CONFIG_ID_SAMPLE_MODE, (VOLDET_ADC_MODE) ? ADC_CH_SAMPLEMODE_CONTINUOUS : ADC_CH_SAMPLEMODE_ONESHOT);
    adc_setChConfig(ADC_CH_VOLDET_KEY1, ADC_CH_CONFIG_ID_INTEN, FALSE);

    //key key2 detection
    adc_setChConfig(ADC_CH_VOLDET_KEY2, ADC_CH_CONFIG_ID_SAMPLE_FREQ, 10000); //10K Hz, sample once about 100 us for CONTINUOUS mode
    adc_setChConfig(ADC_CH_VOLDET_KEY2, ADC_CH_CONFIG_ID_SAMPLE_MODE, (VOLDET_ADC_MODE) ? ADC_CH_SAMPLEMODE_CONTINUOUS : ADC_CH_SAMPLEMODE_ONESHOT);
    adc_setChConfig(ADC_CH_VOLDET_KEY2, ADC_CH_CONFIG_ID_INTEN, FALSE);

    // Enable adc control logic
    adc_setEnable(TRUE);

    Delay_DelayMs(15); //wait ADC stable  //for pwr on speed up
}

#if _MIPS_TODO
static char ADCStr1[32];
static char ADCStr2[32];

char* VolDet_GetStatusString1(void)
{
    sprintf(ADCStr1, "A0=%ld,A1=%ld,A2=%ld,A3=%ld", adc_readData(0), adc_readData(1),adc_readData(2),adc_readData(3));
    return ADCStr1;
}

char* VolDet_GetStatusString2(void)
{
    sprintf(ADCStr2, "A4=%ld,A5=%ld,A6=%ld,A7=%ld", adc_readData(4), adc_readData(5),adc_readData(6),adc_readData(7));
    return ADCStr2;
}
#endif


void Dx_InitIO(void)  // Config IO for external device
{
    IO_InitPinmux(); //initial PINMUX config
    IO_InitGPIO(); //initial GPIO pin status
    IO_InitADC(); //initial ADC pin status
    #if 1 // Removed useless code.
    IO_InitGensor();
    #endif
#if (_HDMITYPE_ == _HDMI_ON_)
    //thse two pins are default pull-up for GPIO,but must turn off for HDMI DDC I2C
    pad_setPullUpDown(PAD_PIN_PGPIO28, PAD_NONE);
    pad_setPullUpDown(PAD_PIN_PGPIO29, PAD_NONE);
#endif
}

void Dx_UninitIO(void)  // Config IO for external device
{
    gpio_clearPin(GPIO_WIFI_POWER_PWM5);
    //gpio_setPin(GPIO_WIFI_LED);
    //gpio_setPin(GPIO_REC_LED);
    // Disable adc control logic
    adc_setEnable(FALSE);
    adc_close(ADC_CH_VOLDET_BATTERY);
    adc_close(ADC_CH_VOLDET_KEY1);
    adc_close(ADC_CH_VOLDET_KEY2);
}

UINT32 DxGetPWDICStatus()
{
    UINT32 status=0,status1=0,prestatus,nextstatus,i;
    for (i=0;i<10;i++)
    {
        prestatus = gpio_getPin(GPIO_PASSWORD_SDA);

        gpio_clearPin(GPIO_PASSWORD_SCK);
        Delay_DelayMs(1);  
        nextstatus = gpio_getPin(GPIO_PASSWORD_SDA);
        if (prestatus == nextstatus)
            status = 1;
        else
            status1++;
        gpio_setPin(GPIO_PASSWORD_SCK);
        Delay_DelayMs(1);  
    }
    //debug_err((">s = %d S1 = %d\r\n",status,status1));//s=1;S1=9;
    if ((status == 1)&&(status1 == 9))
    {
        gpio_setPin(GPIO_PASSWORD_SCK);
    }
    else
    {
        while(1)
        {
            SenFP_ShowOSDString("CHECK authorization ",20,52,2);
            Delay_DelayMs(100);
        };
    }

    return 1;
}



