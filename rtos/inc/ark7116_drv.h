#ifndef _ARK7116_DRV_H_
#define _ARK7116_DRV_H_

#if __cplusplus
    extern "C"
    {
#endif

//#include <windows.h>
//#include "ARK7116_API_lib.h"
//Device address define

typedef enum _InputSourceIDType
{
	INPUT_AV1,
	INPUT_AV2,
	INPUT_TV,
	INPUT_CAMERA_DoorBell,
	INPUT_CAMERA_Car,
	INPUT_SVIDEO,
	INPUT_ITU656,
	INPUT_FM,	
	INPUT_YPBPR,
	INPUT_VGA,
	MAX_VIDEO_CHANNEL ,
	ALL_INPUT_SOURCE = 0XFF,
}InputSourceType;


typedef enum _ConfigDisplayMode
{
     DISP_16_9= 0 ,
     DISP_4_3,
}ConfigDisplayMode;



typedef enum _ColorSysType
{
    PAL = 0,
	PAL_N,
	PAL_M,
    NTSC,
    SECAM,   
    PAL60,
    AUTO,
	NULL_SYS = -1,
}ColorSysType;



//MCU CFG Addr
#define MCU_CFG_ADDR 				0xC6

/************************Global ***********************/
#define RSTN                           	0XFD00
#define ENH_PLL                        0XFD0E


//BUS Addr
#define BUS_STATUS_ADDR         	0xAF


typedef struct _PanlstaticPara
{
    unsigned int addr;
    unsigned char dat;
}PanlstaticPara;

typedef struct _PanlPosDynPara
{
    unsigned int addr;
    unsigned char dat_posDyn[6];
}PanlPosDynPara;

typedef struct _PanlSysDynPara
{
    unsigned int addr;
    unsigned char dat_sysDyn[8];
}PanlSysDynPara;


typedef enum _VdeOutputType
{
	VDE_CLOSE = 0,             
	VDE_RED,          
	VDE_GREEN,          
	VDE_BLUE ,                    
	VDE_GRAY,     
	VDE_WHITE,     
	VDE_BLACK,
	MAX_VDECOLOR = VDE_BLACK,
} VdeOutputTyp;

/*************************************VP CONTROL REG*********************************/
#define BRIGHT_REG               		0XFFD4
#define CONTRAST_REG            		0XFFD3 
#define SATURATION_REG          		0XFFD6
#define TINT_REG                        0XFFD5
#define VDE_REG                    		0XFFD2
 

/*==============start===============*/
/*AV1
[VideoChannel]
AV1
[VideoType]
CVBS
[VideoPI]
VIDEO_P
[VideoPicSys]
PAL
[VideoData]
13500000
 690
 280
 864
 312

Update date:Monday, November 24, 2014
Update time:11:05:45
*/

/*屏参参数相关的结构体*/
typedef struct _PannelPara
{
   PanlstaticPara  *pVideoStaicPara;
   PanlPosDynPara *pVideoPosDynPara;
   PanlSysDynPara  *pVideoSysDynPara;
}PannelPara;
typedef struct _VideoChannel
{
   unsigned char INPUT_ID;
   PannelPara    VideoPara;
}VideoChannel;

#define STATIC_NUM (467)

//#define STATIC_NUM (467-43 + 12 + 10 + 10 + 11)
#define POS_DYN_NUM 14
#define SYS_DYN_NUM 4
#define PAD_MUX_NUM 43
//#define PAD_MUX_NUM 0
#define VCOM_AC_Def 0X00
#define VCOM_DC_Def 0X00
#define PWMA_VAL 0X0000
#define PWMB_VAL 0X0000
#define PAL_PLL_CLK 0X0090
#define NTSC_PLL_CLK 0X0090
#define PWM0_CYCLE_VAL 0X00FF
#define PWM1_CYCLE_VAL 0X00FF
#define PWM2_CYCLE_VAL 0X00FF
#define PWM3_CYCLE_VAL 0X00FF
#define PWM0_DUTY_VAL 0X0080
#define PWM1_DUTY_VAL 0X0080
#define PWM2_DUTY_VAL 0X0080
#define PWM3_DUTY_VAL 0X0080

#define I2C_ACCESS_LOOP_TIME   		20


typedef struct
{
	unsigned char regAddr;
	unsigned char regValue;
}ARK7116REG;

typedef struct
{
	unsigned char regAddr;
	unsigned char regPalValue;
	unsigned char regNtscValue;
}ARK7116DYREG;

typedef struct {
	DWORD dwOpenCount;
} T_ARK7116DRIVERINIT_STRUCTURE, *PT_ARK7116DRIVERINIT_STRUCTURE;

typedef struct {
	T_ARK7116DRIVERINIT_STRUCTURE *pDeviceContext;
} T_ARK7116DRIVEROPEN_STRUCTURE, *PT_ARK7116DRIVEROPEN_STRUCTURE;

DWORD I2C_ARK7116_Read(UINT8 devaddr, UINT8 wordaddr, UINT8 *pOutBuf);
DWORD I2C_ARK7116_Write(UINT8 devaddr, UINT8 *pi2cData, UINT32 length);

#if __cplusplus
       }
#endif

#endif

