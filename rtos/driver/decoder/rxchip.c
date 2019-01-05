#include "hardware.h"
#include "gpio.h"
#include "gpioi2c.h"
#include "ark7116_drv.h"
#include "xm_core.h"
#include "xm_printf.h"
#include "xm_dev.h"
#include "..\..\app\app.h"
#include <xm_video_task.h>
#include "rxchip.h"
#include "app_head.h"
#include "board_config.h"
#include "rxchip.h"

//这个定义在IAR软件工程中:option-C/C++ compiler---preprocessor中
#ifdef _XMSYS_RXCHIP_SUPPORT_

typedef struct _RXCCHIPstaticPara
{
    unsigned char addr;
    unsigned char dat;
}RXCHIPstaticPara;

void XM_RxchipDisplayInit();


struct rxchip_detect_info{
	u8  en_rxchip_check;
	u8 ch1_signalokcnt;
	u8 ch1_signalnookcnt;
	u8 ch1_signal_state;
    u8 ch1_signal_flag;
	u8 ch1_work_mode;
	u8 ch1_pre_work_mode;
    u8 ch1_nosignal_flag;
    u8 ch1_signalok_flag;

	u8 ch2_signalokcnt;
	u8 ch2_signalnookcnt;
	u8 ch2_signal_state;
    u8 ch2_signal_flag;
	u8 ch2_work_mode;
	u8 ch2_pre_work_mode;
    u8 ch2_nosignal_flag;
    u8 ch2_signalok_flag;

	u8 checktimecnt ;
    u8 parkingon_flag;
    u8 parkingoff_flag;
    u8 signal_check_flag;

	u8 ch1_nosignal_status;
	u8 ch2_nosignal_status;

	u8 complete_check_times;//整个流程检测次数
	u8 complete_check_flag;//整个流程检测标志
	u8 first_check_flag;
};

enum {
    RXCHIP_WORK_MODE_720P_PAL = 0x00,
    RXCHIP_WORK_MODE_720P_NTSC = 0x01,
    RXCHIP_WORK_MODE_CVBS_PAL = 0x02,
    RXCHIP_WORK_MODE_CVBS_NTSC = 0x03,
    RXCHIP_WORK_MODE_NO_SIGNAL=0x04,
    RXCHIP_FORBIDDEN = 0xff,
};


//UTC 命令,最后一个数据计算校验和
u8 utc_head[4] = {0x00, 0x00, 0x00, 0x00};
u8 utc_mirror_cmd[4] = {0x12, 0x23, 0x34, 0x00};//镜像
u8 utc_normal_cmd[4] = {0x23, 0x45, 0x67, 0x00};//正常


//rxchip iic
#define RXCHIP_SEN0_SLAVE_ADDR		0x5A
#define RXCHIP_SEN1_SLAVE_ADDR		0x58	

#define RXCHIP_DETECT_TIME     	50
#define RXCHIP_CHECK_MAX_TIME	25
#define RXCHIP_CHECK_MIN_TIME	10

#define CAM1		0x01
#define CAM2		0x02
#define CAM1_CAM2	0x03

#define false 0
#define true 1


#define I2C_RETRY_NUM				3
#define HDA							0x00
#define HDT							0x01
#define _720P_25FPS					0x02
#define _720P_30FPS					0x03
#define _720P_50FPS					0x04
#define _720P_60FPS					0x05
#define _1080P_25FPS				0x06
#define _1080P_30FPS				0x07
#define FORMAT_OFFSET				0X00	

#define RXCHIP_CHECK_EVENT    (1<<0)

static struct rxchip_detect_info rxchip_detect;
static OS_TIMER Rxchip_TImer;
static OS_TASK TCB_Rxchip_Task;
__no_init static OS_STACKPTR int StackRxchipTask[XMSYS_RXCHIP_TASK_STACK_SIZE/4];          /* Task stacks */

static OS_RSEMA rxchip_sema;			// 互斥保护
static OS_MAILBOX rxchip_mailbox;							// 一字节邮箱。
static u8 rxchip_mailbuf[sizeof(RXCHIPCMD)*10];				// 一个字节的邮箱

static unsigned char Camera_Standard =HDA;
static unsigned char Camera_Video_Format=_720P_25FPS;
#ifdef EN_RN6752M1080N_CHIP
static unsigned int  Camera_Video_Width=960;
static unsigned int  Camera_Video_Height=1080;
#else
static unsigned int  Camera_Video_Width=1280;
static unsigned int  Camera_Video_Height=720;
#endif
static unsigned char Camera_Pre_Video_Format=_720P_25FPS;
u8_t ACC_Det_Channel = 0;
BOOL Camera_Data_signed = FALSE;

#if 1//add lmg
int Sensor1_In_Flag = 0;//根据标志判断录像录几通道，默认录两通道
int Sensor2_In_Flag = 0;
uchar sensor2reverse = 0;//根触发相关后续优化
uchar sensor1reverse = 0;
u8_t Delay_rxchip_Data = 0;
BOOL rxchip_ready_flag = FALSE;//rx初始化完成标志
#endif

extern unsigned int SensorData_CH0_Format;
extern unsigned int SensorData_CH1_Format;
extern unsigned int SensorData_CH2_Format;

extern OS_TASK get_isp_itu601_task(void);
extern void Notification_Itu601_Isp_Scaler_Init(void);
extern void rxchip_itu656_PlugIn(void);

BOOL AHD_Guide_Start(void)
{
	return TRUE;
}


/**
 *
 * rxchip 返回rx 初始化完成标志
 *
 */
BOOL get_rxchip_ready_state(void)
{
	return rxchip_ready_flag;
}

#ifdef EN_RN6752_CHIP
const RXCHIPstaticPara rxchip_rn6752_720p_pal_staticPara[]=
{
	//RN6752-601-720P
    {0x01, 0x10}, // brightness (default: 00h)
	{0x02, 0x80}, // contrast (default: 80h)
	{0x03, 0x80}, // saturation (default: 80h)
	{0x04, 0x80}, // hue (default: 80h)

	{0x05, 0x88}, // sharpness (default: 0x08h)
	{0x09, 0x08}, // EQ
	{0x34, 0x00}, // darkref￡?OB
	{0x35, 0x80}, // darkrefcnt

	{0x57, 0x2A}, // B/W stretch,black stretch, M = 32

	{0x81, 0x01},// turn on video decoder
	{0xA3, 0x04},// enable 72MHz sampling
	{0xDB, 0x8F},// internal use*
	{0xFF, 0x00},// switch to ch0 (default; optional)
	{0x2C, 0x30},// select sync slice points
	{0x50, 0x02},// 720p resolution select for BT.601
	{0x56, 0x04},// disable SAV & EAV for BT601; 0x00 enable SAV & EAV for BT656
	{0x63, 0xBD},// filter control
	{0x59, 0x00},// extended register access
	{0x5A, 0x02},// data for extended register
	{0x58, 0x01},// enable extended register write
	{0x07, 0x23},// 720p format
	{0x2F, 0x04},// internal use*
	{0x5E, 0x0B},// enable H-scaling control
	{0x51, 0x44},// scale factor1
	{0x52, 0x86},// scale factor2,0X78-0X80
	{0x53, 0x22},// scale factor3
	{0x3A, 0x04},// no channel information insertion; invert VBLK for frame valid
	{0x3E, 0x32},// AVID & VBLK out for BT.601
	{0x40, 0x04},// no channel information insertion; invert VBLK for frame valid
	{0x46, 0x23},// AVID & VBLK out for BT.601

	{0x49, 0x84},

	{0x28, 0x92},
	{0x00, 0x20}, // internal use*
	{0x0D, 0x20}, // cagc initial value
	{0x2D, 0xF2}, // cagc adjust
	{0x37, 0X33},
	{0x61, 0X6C},
	{0xDF, 0xFE}, // enable 720p format

	{0x8E, 0x00},// single channel output for VP
	{0x8F, 0x80},// 720p mode for VP
	{0x8D, 0x31},// enable VP out
	{0x89, 0x09},// select 72MHz for SCLK
	{0x88, 0xC1},// enable SCLK out
	{0x81, 0x01},// turn on video decoder
	
	{0x96, 0x00},
	{0x97, 0x0b},
	{0x98, 0x00},
	{0x9a, 0x40},
	{0x9b, 0xe1},
	{0x9c, 0x00},

	//{0x00, 0xC0},//test bar color
}
#endif


#ifdef EN_RN6752M_CHIP
const RXCHIPstaticPara rxchip_rn6752m_720p_pal_staticPara[]=
{
	//RN6752M-601-720P(包括同轴控制)
	// 720P@25 BT601
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	{0x81, 0x01}, // turn on video decoder
	{0xA3, 0x04},
	{0xDF, 0xFE}, // enable HD format
	{0x88, 0x40}, // disable SCLK0B out
	{0xF6, 0x40}, // disable SCLK3A out

	// ch0
	{0xFF, 0x00}, // switch to ch0 (default; optional)
	{0x00, 0x20}, // internal use*
	{0x06, 0x08}, // internal use*
	{0x07, 0x63}, // HD format
	{0x2A, 0x01}, // filter control
	{0x3A, 0x00}, // No Insert Channel ID in SAV/EAV code
	{0x3F, 0x10}, // channel ID
	{0x4C, 0x37}, // equalizer
	{0x4F, 0x03}, // sync control
	{0x50, 0x02}, // 720p resolution
	{0x56, 0x05}, // BT 72M mode and BT601 mode
	{0x5F, 0x40}, // blank level
	{0x63, 0xF5}, // filter control
	{0x59, 0x00}, // extended register access
	{0x5A, 0x42}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x59, 0x33}, // extended register access
	{0x5A, 0x23}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x51, 0xE1}, // scale factor1
	{0x52, 0x88}, // scale factor2
	{0x53, 0x12}, // scale factor3
	{0x5B, 0x07}, // H-scaling control
	{0x5E, 0x0B}, // enable H-scaling control
	{0x6A, 0x82}, // H-scaling control
	{0x28, 0x92}, // cropping
	{0x03, 0x80}, // saturation
	{0x04, 0x80}, // hue
	{0x05, 0x00}, // sharpness
	{0x57, 0x23}, // black/white stretch
	{0x68, 0x32}, // coring

	//{0x3A, 0x04},
	//{0x3E, 0x32},
	{0x3A, 0x02},//P-MOS
	{0x3E, 0xf6},//同轴映射到AVID脚
	    
	{0x40, 0x04},
	{0x46, 0x23},
	
	{0x49, 0x84},
	{0x6d, 0x00},
	
	{0x8E, 0x00}, // single channel output for VP
	{0x8F, 0x80}, // 720p mode for VP
	{0x8D, 0x31}, // enable VP out
	{0x89, 0x09}, // select 72MHz for SCLK
	{0x88, 0x41}, // enable SCLK out

	{0x96, 0x00}, // select AVID & VBLK as status indicator
	{0x97, 0x0B}, // enable status indicator out on AVID,VBLK & VSYNC 
	{0x98, 0x00}, // video timing pin status
	{0x9A, 0x40}, // select AVID & VBLK as status indicator 
	{0x9B, 0xE1}, // enable status indicator out on HSYNC
	{0x9C, 0x00}, // video timing pin status
};
#endif


#ifdef EN_RN6752M1080N_CHIP
const RXCHIPstaticPara rxchip_1080n_pal_staticPara[]=
{
	//RN6752M-601-1080N
	// 1080N@25 BT656
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	{0x81, 0x01}, // turn on video decoder
	{0xA3, 0x04}, //
	{0xDF, 0xFE}, // enable HD format
	{0xF0, 0x00},

	{0x88, 0x40}, // disable SCLK0B out
	{0xF6, 0x40}, // disable SCLK3A out

	// ch0
	{0xFF, 0x00}, // switch to ch0 (default; optional)
	{0x00, 0x20}, // internal use*
	{0x06, 0x08}, // internal use*
	{0x07, 0x63}, // HD format
	{0x2A, 0x01}, // filter control
	{0x3A, 0x00}, // No Insert Channel ID in SAV/EAV code
	{0x3F, 0x10}, // channel ID
	{0x4C, 0x37}, // equalizer
	{0x4F, 0x03}, // sync control
	{0x50, 0x13}, // 1080N resolution
	{0x56, 0x05}, // 72M and BT656 mode
	{0x5F, 0x44}, // blank level
	{0x63, 0xF8}, // filter control
	{0x59, 0x00}, // extended register access
	{0x5A, 0x48}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x59, 0x33}, // extended register access
	{0x5A, 0x23}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x51, 0xF4}, // scale factor1
	{0x52, 0x29}, // scale factor2
	{0x53, 0x15}, // scale factor3
	{0x5B, 0x07}, // H-scaling control
	{0x5E, 0x08}, // enable H-scaling control
	{0x6A, 0x83}, // H-scaling control
	{0x28, 0x92}, // cropping
	{0x03, 0x80}, // saturation
	{0x04, 0x80}, // hue
	{0x05, 0x04}, // sharpness
	{0x57, 0x23}, // black/white stretch
	{0x68, 0x00}, // coring

	{0x47, 0x88},
	//{0x3A, 0x04},
	//{0x3E, 0x32},
	{0x3A, 0x02},//P-MOS
	{0x3E, 0xf6},//同轴映射到AVID脚
	{0x49, 0x84},
	
		//{0x42, 0x20},
		//{0x20, 0x84},
		//{0x23, 0x2a-0x2a},
		//{0x24, 0x62-0x2a},
		//{0x25, 0x2a+20},
		//{0x26, 0x62+20},
		//{0x28, 0xa2},
	
	{0x8E, 0x00}, // single channel output for VP
	{0x8F, 0x80}, // 1080p mode for VP
	{0x8D, 0x31}, // enable VP out
	{0x89, 0x09}, // select 72MHz for SCLK
	{0x88, 0x41}, // enable SCLK out
	
	{0x96, 0x00}, // select AVID & VBLK as status indicator
	{0x97, 0x0B}, // enable status indicator out on AVID,VBLK & VSYNC 
	{0x98, 0x00}, // video timing pin status
	{0x9A, 0x40}, // select AVID & VBLK as status indicator 
	{0x9B, 0xE1}, // enable status indicator out on HSYNC
	{0x9C, 0x00}, // video timing pin status

	//{0x00, 0xC0},//test bar color
};
#endif


/**
 *
 * rxchip iic io 初始化
 *
 */
void rxchip_I2C_Init(void)
{
	group2_i2c_pad_init();
}


/**
 * iic read fun
 *
 * @param devaddress 设备地址
 * @param address 寄存器地址
 *
 * @return the error code or 寄存器地址数据
 */
static unsigned char gpio_i2c_read(unsigned char devaddress, unsigned char address)
{
	 int ret = -1;
	 int retries = 0;
	 unsigned char dat=0;

	 while(retries < I2C_RETRY_NUM)
	 {
	 	ret = rxchip_i2c_read_bytes(devaddress, (unsigned char *)&address, 1, (unsigned char *)&dat, 1);
		if(ret == 1)
			break;

		retries++;
	 }
	 if(retries >= I2C_RETRY_NUM)
	 {
		XM_printf("rxchip i2c read dev:%x addr:%x error\n",devaddress,address);
		ret = -1;
	 }	
	 	
	 if(ret < 0)
	 {
		 return 0xFF;
	 }
	 else
	 {
		 return dat;
	 }
}


/**
 * iic write fun
 *
 * @param devaddress 设备地址
 * @param address 寄存器地址
 * @param data 写入寄存器数据
 * @return the error code, RT_EOK on initialization successfully.
 */
static unsigned char gpio_i2c_write(unsigned int devaddress, unsigned char address, unsigned char data)
{
	int ret = -1;
	int retries = 0;

	while(retries < I2C_RETRY_NUM)
	{
		ret = rxchip_i2c_write_bytes(devaddress, (unsigned char *)&address, 1, (unsigned char *)&data, 1);
		if(ret == 0)
			break;
		
		retries++;
	}
	
	if(retries >= I2C_RETRY_NUM)
	{
		XM_printf("rxchip i2c write dev:%x addr:%x error\n",devaddress,address);
		ret = -1;
	}
	
	if(ret < 0)
	{
		return 0xFF;
	}
	else
	{
		return 0;
	}
}


/**
 *
 * rxchip 设置打开无信号蓝屏
 *
 */
void rxchip_open_panel(u8 ch)
{
	if(ch==CAM1)
	{
		//sensor 0
		//XM_lock();
		gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x3A, 0x09);
		//XM_unlock();
	}

	if(ch==CAM2)
	{
		//sensor 1
		//XM_lock();
		gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x3A, 0x09);
		//XM_unlock();
	}
}


/**
 *
 * rxchip 设置关闭无信号蓝屏
 *
 */
void rxchip_close_panel(u8 ch)
{
	if(ch==CAM1)
	{
		//sensor 0
		//XM_lock();
		gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x3A, 0x08);
		//XM_unlock();
	}

	if(ch==CAM2)
	{
		//sensor 1
		//XM_lock();
		gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x3A, 0x08);
		//XM_unlock();
	}
}


/**
 *
 * rxchip 设置无信号蓝屏
 *
 */
void rxchip_set_blue_panel(void)
{
	//sensor 0
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x3B, 0x13);
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x3C, 0xdd);
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x3D, 0x72);
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x3A, 0x09);

	//sensor 1
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x3B, 0x13);
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x3C, 0xdd);
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x3D, 0x72);
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x3A, 0x09);
}


/**
 *
 * rxchip 设置无信号黑屏
 *
 */
void rxchip_set_black_panel(void)
{
	//sensor 0
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x3B, 0x00);
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x3C, 0x80);
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x3D, 0x80);
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x3A, 0x09);

	//sensor 1
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x3B, 0x00);
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x3C, 0x80);
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x3D, 0x80);
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x3A, 0x09);
}


/**
 *
 * rxchip 进度条0-100，转换成视频参数值
 *
 */
u8 rxchip_video_parameter_adj(u8 best_val, u8 val)
{
	u8 temp;

	if(val>=50)
	{
		temp = val - 50 ;
		temp += best_val ;
	}
	else
	{
		temp = 50 - val ;
		temp = best_val - temp ;
	}

	return temp;
}

/**
 *
 * rxchip 设置亮度,val范围0-100
 *
 */
static void rxchip_set_brightness(u8 val)
{	
	u8_t temp;
	
	if((val<=100))
	{
		temp = rxchip_video_parameter_adj(BEST_BRIGHTNESS, val);

		//XM_lock();
		gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x01, temp);
		//XM_unlock();

		//XM_lock();
		gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x01, temp);
		//XM_unlock();
	}
}

/**
 *
 * rxchip 设置对比度
 *
 */
static void rxchip_set_contrast(u8 val)
{	
	u8_t temp;

	if((val<=100))
	{
		temp = rxchip_video_parameter_adj(BEST_CONTRAST, val);

		//XM_lock();
		gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x02, temp);
		//XM_unlock();

		//XM_lock();
		gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x02, temp);
		//XM_unlock();
	}
}


/**
 *
 * rxchip 设置饱和度
 *
 */
static void rxchip_set_saturation(u8 val)
{	
	u8_t temp;

	if((val<=100))
	{
		temp = rxchip_video_parameter_adj(BEST_SATURATION, val);

		//XM_lock();
		gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x03, temp);
		//XM_unlock();

		//XM_lock();
		gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x03, temp);
		//XM_unlock();
	}
}


/**
 *
 * rxchip 设置色调
 *
 */
static void rxchip_set_hue(u8 val)
{	
	u8_t temp;

	if((val<=100))
	{
		temp = rxchip_video_parameter_adj(BEST_HUE, val);

		//XM_lock();
		gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x04, temp);
		//XM_unlock();

		//XM_lock();
		gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x04, temp);
		//XM_unlock();
	}
}

/**
 *
 * rxchip 设置清晰度
 *
 */
static void rxchip_set_sharpness(u8 val)
{	
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x05, val);
	//XM_unlock();
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x05, val);
	//XM_unlock();
}


/**
 *
 * rxchip 设置OB
 *
 */
static void rxchip_set_ob(u8 val)
{	
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x34, val);
	//XM_unlock();
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x34, val);
	//XM_unlock();
}

/**
 *
 * rxchip 设置BW
 *
 */
static void rxchip_set_bw(u8 val)
{	
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x57, val);
	//XM_unlock();
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x57, val);
	//XM_unlock();
}
/**
 *
 * rxchip 设置PAL制式
 *
 */
void rxchip_sensor0_setting_ahd_pal(void)
{
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x59, 0x00);// extended register access
	//XM_unlock();
	
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x5A, 0x02);// data for extended register
	//XM_unlock();
	
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x58, 0x01);// enable extended register write
	//XM_unlock();
}


/**
 *
 * rxchip 设置PAL制式
 *
 */
void rxchip_sensor1_setting_ahd_pal(void)
{
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x59, 0x00);// extended register access
	//XM_unlock();
	
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x5A, 0x02);// data for extended register
	//XM_unlock();
	
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x58, 0x01);// enable extended register write
	//XM_unlock();
}

/**
 *
 * rxchip 设置NTSC制式
 *
 */
void rxchip_sensor0_setting_ahd_ntsc(void)
{
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x59, 0x00);// extended register access
	//XM_unlock();
	
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x5A, 0x04);// data for extended register
	//XM_unlock();
	
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, 0x58, 0x01);// enable extended register write
	//XM_unlock();
}

/**
 *
 * rxchip 设置NTSC制式
 *
 */
void rxchip_sensor1_setting_ahd_ntsc(void)
{
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x59, 0x00);// extended register access
	//XM_unlock();
	
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x5A, 0x04);// data for extended register
	//XM_unlock();
	
	//XM_lock();
	gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, 0x58, 0x01);// enable extended register write
	//XM_unlock();
}


/**
 *
 * rxchip 读状态寄存器
 *
 */
u8 rxchip_read_sensnor0_status(void)
{
	u8 regDat;
	
	//XM_lock();
	regDat = gpio_i2c_read(RXCHIP_SEN0_SLAVE_ADDR, 0x00);
	//XM_unlock();
	
	return regDat;
}

/**
 *
 * rxchip 读状态寄存器
 *
 */
u8 rxchip_read_sensnor1_status(void)
{
	u8 regDat;
	
	//XM_lock();
	regDat = gpio_i2c_read(RXCHIP_SEN1_SLAVE_ADDR, 0x00);
	//XM_unlock();
	
	return regDat;
}

/**
 *
 * rxchip 设置rxchip参数,比如亮度，对比度等
 *
 */
void rxchip_setting(RXCHIPCMD cmd)
{
	OS_PutMail(&rxchip_mailbox, &cmd);
}


/**
 *
 * rxchip 复位函数
 *
 */
static void rxchip_reset(void)
{

	set_sensor0_reset_pin_value(1);
	set_sensor1_reset_pin_value(1);
	OS_Delay(200); 
	set_sensor0_reset_pin_value(0);
	set_sensor1_reset_pin_value(0);
	OS_Delay(200); 
	set_sensor0_reset_pin_value(1);
	set_sensor1_reset_pin_value(1);
	OS_Delay(200);
}


/**
 * check id
 *
 * @return 返回错误代码，bit0=1代表sensor 0,id错误，bit1=1代表sensor 1,id错误，
 * 
 */
int rxchip_check_id(void)
{
	int ret, error;

	error = 0;
	
	ret = gpio_i2c_read(RXCHIP_SEN0_SLAVE_ADDR, 0xfe);
	ret = (ret<<8)|gpio_i2c_read(RXCHIP_SEN0_SLAVE_ADDR, 0xfd);

	#ifdef EN_RN6752_CHIP
	if(ret!=0x0401)
	#endif
	#ifdef EN_RN6752M_CHIP
	if(ret!=0x0501)
	#endif
	#ifdef EN_RN6752M1080N_CHIP
	if(ret!=0x0501)
	#endif
	{
		error |= 0x01;
		XM_printf(">>>sensor 0, id error\r\n");
	}
	XM_printf(">>>sensor 0,id:%x\r\n", ret);	
	
	ret = gpio_i2c_read(RXCHIP_SEN1_SLAVE_ADDR, 0xfe);
	ret = (ret<<8)|gpio_i2c_read(RXCHIP_SEN1_SLAVE_ADDR, 0xfd);

	#ifdef EN_RN6752_CHIP
	if(ret!=0x0401)
	#endif
	#ifdef EN_RN6752M_CHIP
	if(ret!=0x0501)
	#endif	
	#ifdef EN_RN6752M1080N_CHIP
	if(ret!=0x0501)
	#endif
	{
		error |= 0x02;
		XM_printf(">>>sensor 1, id error\r\n");
	}
	XM_printf(">>>sensor 1,id:%x\r\n", ret);


	return error;
}


u32_t camera_get_video_height_2 (void)
{
	return Camera_Video_Height-FORMAT_OFFSET;
}


u32_t camera_get_video_width_2 (void)
{
	return Camera_Video_Width-FORMAT_OFFSET;
}


u32_t camera_get_video_format_2(void)
{
    return Camera_Video_Format;
}

//先高低4位调换，再次高低4位各自高低位颠倒
//参考蝶式交换法
u8_t utc_change_value(u8_t val)
{
	val=(val<<4)|(val>>4);  
	val=((val<<2)&0xcc)|((val>>2)&0x33);  
	val=((val<<1)&0xaa)|((val>>1)&0x55);  

	return val;
}


#ifdef EN_UTC_FUN
//sen:      0,sensor0  1,sensor1
//cmd_index 0:正常     1:镜像
//res:      0,720P     1:1080P
void rxchip_coaxitron_control(u8_t sen, u8_t res, u8_t cmd_index)
{
    u8_t address;

	if(sen==0)
	{
		address = RXCHIP_SEN0_SLAVE_ADDR;
	}
	else
	{
		address = RXCHIP_SEN1_SLAVE_ADDR;
	}
	
#if 0//以下配置已包含在初始化列表中,可以不用再次配置
	#ifdef RXCHIP_INTERFACE_BT601
    //gpio_i2c_write(address, 0x96, 0x00);//0x00
    //gpio_i2c_write(address, 0x97, 0x0b);//0x0b  
    //gpio_i2c_write(address, 0x98, 0x00);//0x00
	#else
	gpio_i2c_write(address, 0X9A, 0X40);
	gpio_i2c_write(address, 0X9B, 0XE1);
	gpio_i2c_write(address, 0X9C, 0X00);
	#endif
	
	//gpio_i2c_write(address, 0XFF, 0x00);

	#ifdef RXCHIP_INTERFACE_BT601
    //gpio_i2c_write(address, 0X3A, 0x02);//P-MOS
    //gpio_i2c_write(address, 0X3E, 0xf6);//同轴映射到AVID脚
	#else
	gpio_i2c_write(address, 0X40, 0x08);//P-MOS
	gpio_i2c_write(address, 0X46, 0x60);//同轴映射到行同步脚
	#endif
#endif	

    gpio_i2c_write(address, 0X3A, temp);

    gpio_i2c_write(address, 0X73, 0x44);

	if(res==0)
	{
		gpio_i2c_write(address, 0X75, 0x06);//720P
	}
	else
	{
		gpio_i2c_write(address, 0X75, 0x16);//1080P
	}

	//发送命令
	switch(cmd_index)
	{
		case 0://正常
			gpio_i2c_write(address, 0X74, utc_head[0]);
			gpio_i2c_write(address, 0X74, utc_head[1]);
			gpio_i2c_write(address, 0X74, utc_head[2]);
		    gpio_i2c_write(address, 0X74, utc_head[3]);
		    gpio_i2c_write(address, 0X73, 0x44);
			
			gpio_i2c_write(address, 0X74, utc_normal_cmd[0]);
			gpio_i2c_write(address, 0X74, utc_normal_cmd[1]);
			gpio_i2c_write(address, 0X74, utc_normal_cmd[2]);
		    gpio_i2c_write(address, 0X74, utc_normal_cmd[3]);
		    gpio_i2c_write(address, 0X73, 0x44);	
			break;

		case 1://镜像
			gpio_i2c_write(address, 0X74, utc_head[0]);
			gpio_i2c_write(address, 0X74, utc_head[1]);
			gpio_i2c_write(address, 0X74, utc_head[2]);
		    gpio_i2c_write(address, 0X74, utc_head[3]);
		    gpio_i2c_write(address, 0X73, 0x44);
			
			gpio_i2c_write(address, 0X74, utc_mirror_cmd[0]);
			gpio_i2c_write(address, 0X74, utc_mirror_cmd[1]);
			gpio_i2c_write(address, 0X74, utc_mirror_cmd[2]);
		    gpio_i2c_write(address, 0X74, utc_mirror_cmd[3]);
		    gpio_i2c_write(address, 0X73, 0x44);	
			break;

		default:
			break;
	}
}
#endif


// pre_initial_start
void rxchipm_Pre_initial(unsigned char sen) 
{
	u8_t rom_byte1, rom_byte2, rom_byte3, rom_byte4, rom_byte5, rom_byte6;

	gpio_i2c_write(sen, 0xE1, 0x80);
	gpio_i2c_write(sen, 0xFA, 0x81);

	rom_byte1 = gpio_i2c_read(sen, 0xfe);
	rom_byte2 = gpio_i2c_read(sen, 0xfe);
	rom_byte3 = gpio_i2c_read(sen, 0xfe);
	rom_byte4 = gpio_i2c_read(sen, 0xfe);
	rom_byte5 = gpio_i2c_read(sen, 0xfe);
	rom_byte6 = gpio_i2c_read(sen, 0xfe);

	rom_byte1 = rom_byte1;
	rom_byte2 = rom_byte2;
	rom_byte3 = rom_byte3;
	rom_byte4 = rom_byte4;//解决compile warning
	
	//config. decoder accroding to rom_byte5 and rom_byte6
	if ((rom_byte6 == 0x00) && (rom_byte5 == 0x00)) {
		gpio_i2c_write(sen, 0xEF, 0xAA);  
		gpio_i2c_write(sen, 0xE7, 0xFF);
		gpio_i2c_write(sen, 0xFF, 0x09);
		gpio_i2c_write(sen, 0x03, 0x0C);
		gpio_i2c_write(sen, 0xFF, 0x0B);
		gpio_i2c_write(sen, 0x03, 0x0C);
	}
	else if (((rom_byte6 == 0x34) && (rom_byte5 == 0xA9)) ||
         ((rom_byte6 == 0x2C) && (rom_byte5 == 0xA8))) {
		gpio_i2c_write(sen, 0xEF, 0xAA);  
		gpio_i2c_write(sen, 0xE7, 0xFF);
		gpio_i2c_write(sen, 0xFC, 0x60);
		gpio_i2c_write(sen, 0xFF, 0x09);
		gpio_i2c_write(sen, 0x03, 0x18);
		gpio_i2c_write(sen, 0xFF, 0x0B);
		gpio_i2c_write(sen, 0x03, 0x18);
	}
	else {
		gpio_i2c_write(sen, 0xEF, 0xAA);  
		gpio_i2c_write(sen, 0xFC, 0x60);
		gpio_i2c_write(sen, 0xFF, 0x09);
		gpio_i2c_write(sen, 0x03, 0x18);
		gpio_i2c_write(sen, 0xFF, 0x0B);
		gpio_i2c_write(sen, 0x03, 0x18);	
	}
}


/**
 * rxchip_sensor0_init
 *
 * sensor 0初始化
 * 
 */
static void rxchip_sensor0_init(void)
{
	u8 i;

	XM_printf(">>>>>rxchip_sensor0_init\r\n");

	#ifdef EN_RN6752_CHIP
	for (i = 0; i < sizeof(rxchip_rn6752_720p_pal_staticPara) / sizeof(RXCHIPstaticPara); i++) 
	{
		gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, rxchip_rn6752_720p_pal_staticPara[i].addr, rxchip_rn6752_720p_pal_staticPara[i].dat);
	}
	#endif
	
	#ifdef EN_RN6752M_CHIP
	rxchipm_Pre_initial(RXCHIP_SEN0_SLAVE_ADDR);
	
	for (i = 0; i < sizeof(rxchip_rn6752m_720p_pal_staticPara) / sizeof(RXCHIPstaticPara); i++) 
	{
		gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, rxchip_rn6752m_720p_pal_staticPara[i].addr, rxchip_rn6752m_720p_pal_staticPara[i].dat);
	}
	#endif
	
	#ifdef EN_RN6752M1080N_CHIP
	rxchipm_Pre_initial(RXCHIP_SEN0_SLAVE_ADDR);
	for (i = 0; i < sizeof(rxchip_1080n_pal_staticPara) / sizeof(RXCHIPstaticPara); i++) 
	{
		gpio_i2c_write(RXCHIP_SEN0_SLAVE_ADDR, rxchip_1080n_pal_staticPara[i].addr, rxchip_1080n_pal_staticPara[i].dat);
	}
	#endif
	XM_printf(">>>>>>rxchip sensor0 init ok..............\r\n");
}


/**
 * rxchip_sensor1_init
 *
 * sensor 1初始化
 * 
 */
static void rxchip_sensor1_init(void)
{
	u8 i;

	XM_printf(">>>>>rxchip_sensor1_init\r\n");
	
	#ifdef EN_RN6752_CHIP
	for (i = 0; i < sizeof(rxchip_rn6752_720p_pal_staticPara) / sizeof(RXCHIPstaticPara); i++) 
	{
		gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, rxchip_rn6752_720p_pal_staticPara[i].addr, rxchip_rn6752_720p_pal_staticPara[i].dat);
	}
	#endif
	
	#ifdef EN_RN6752M_CHIP
	rxchipm_Pre_initial(RXCHIP_SEN1_SLAVE_ADDR);
	
	for (i = 0; i < sizeof(rxchip_rn6752m_720p_pal_staticPara) / sizeof(RXCHIPstaticPara); i++) 
	{
		gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, rxchip_rn6752m_720p_pal_staticPara[i].addr, rxchip_rn6752m_720p_pal_staticPara[i].dat);
	}
	#endif
	
	#ifdef EN_RN6752M1080N_CHIP
	rxchipm_Pre_initial(RXCHIP_SEN1_SLAVE_ADDR);
	for (i = 0; i < sizeof(rxchip_1080n_pal_staticPara) / sizeof(RXCHIPstaticPara); i++) 
	{
		gpio_i2c_write(RXCHIP_SEN1_SLAVE_ADDR, rxchip_1080n_pal_staticPara[i].addr, rxchip_1080n_pal_staticPara[i].dat);
	}
	#endif
	XM_printf(">>>>>>rxchip sensor1 init ok..............\r\n");
}


static void rxchip_task (void)
{
	OS_TASK temp_task;
	u8 error, cur_ch, regDat;
	u8 signal_check_vaild_flag = false;
	static u8 backup_ch1_signal_state;
	static u8 backup_ch2_signal_state;
	RXCHIPCMD rxchip_cmd;
	
	#if 0
	 //OS_EnterRegion();	// 禁止任务切换
	 enter_region_to_protect_7116_setup();
     pr2000_reset();
	 XM_printf("Init PR2000\n");
	 XM_printf("CHIP ID =%x\n",check_id());
     //get_camera_infor();
     pr2000_device_init();
	 //OS_LeaveRegion();
     adjust_c_phase();
	 leave_region_to_protect_7116_setup();
	 //ITU656_in = 1;
     //ISP_SCALAR_RUN;
	 //isp_scalar_run();
	 
	 while(1)
	 {
		//camera_format_detect();
		#ifdef JLINK_DISABLE	
		Reversing_Detect();
		#else
		#endif
		Pr2000_signal();
		OS_Delay (200);
	 }
	#else

	enter_region_to_protect_7116_setup();

	rxchip_reset();
	error = rxchip_check_id();
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>error code:%x\r\n",error);
	if((error&0x01) == 0)
	{//sensor 0 init
		rxchip_sensor0_init();
		OS_Delay (100);
	}
	
	if((error&0x02) == 0)
	{//sensor 1 init
		rxchip_sensor1_init();
		OS_Delay (100);
	}
	XM_RxchipDisplayInit();
	rxchip_set_blue_panel();
	leave_region_to_protect_7116_setup();

	rxchip_ready_flag = TRUE;

	temp_task = get_isp_itu601_task();
	OS_SignalEvent(0x22, &temp_task);
	
	Notification_Itu601_Isp_Scaler_Init();//add lmg
	rxchip_itu656_PlugIn();
	
	rxchip_detect.en_rxchip_check = 1;
	cur_ch = CAM1_CAM2;
	Delay_rxchip_Data = 0;
	
	while(1)
	{
		char myevent = OS_WaitEvent(RXCHIP_CHECK_EVENT);
		//XM_printf(">>>>>rxchip myevent:%x\r\n", myevent);

		if(OS_GetMailTimed(&rxchip_mailbox, &rxchip_cmd, 2) == 0)
		{
			XM_printf(">>>>rxchip, rxchip_cmd.cmd:%d, rxchip_cmd.value:%d\r\n", rxchip_cmd.cmd, rxchip_cmd.dat);
			switch(rxchip_cmd.cmd)
			{
				case CMD_BRIGHTNESS:
					rxchip_set_brightness(rxchip_cmd.dat);
					break;

				case CMD_SATURATION:
					rxchip_set_saturation(rxchip_cmd.dat);
					break;

				case CMD_CONTRAST:
					rxchip_set_contrast(rxchip_cmd.dat);
					break;

				case CMD_HUE:
					rxchip_set_hue(rxchip_cmd.dat);
					break;
					
				case CMD_SHARPNESS:
					rxchip_set_sharpness(rxchip_cmd.dat);
					break;

				case CMD_OB:
					rxchip_set_ob(rxchip_cmd.dat);
					break;

				case CMD_BW:
					rxchip_set_bw(rxchip_cmd.dat);
					break;
					
				default:
					XM_printf(">>>>>undefin cmd..................\r\n");
					break;
			}
		}

	    if( rxchip_detect.en_rxchip_check==1 )
		{
			rxchip_detect.checktimecnt++;
			switch(rxchip_detect.checktimecnt)
			{
				case RXCHIP_CHECK_MAX_TIME:
					if( (cur_ch==(CAM1_CAM2)) || (cur_ch == CAM1) )
					{
						//ch1-sensor0
						backup_ch1_signal_state =  rxchip_detect.ch1_signal_state;
						if( (rxchip_detect.ch1_signalokcnt>(RXCHIP_CHECK_MAX_TIME-RXCHIP_CHECK_MIN_TIME) ) && (rxchip_detect.ch1_signalnookcnt<RXCHIP_CHECK_MIN_TIME) )
						{//have signal
							rxchip_detect.ch1_signal_state <<= 1;
							rxchip_detect.ch1_signal_state &= (~0x01);
						}
						else if( (rxchip_detect.ch1_signalokcnt<RXCHIP_CHECK_MIN_TIME) && (rxchip_detect.ch1_signalnookcnt>(RXCHIP_CHECK_MAX_TIME-RXCHIP_CHECK_MIN_TIME) ) )
						{//no signal
							rxchip_detect.ch1_signal_state <<= 1;
							rxchip_detect.ch1_signal_state |= (0x01);
						}
						rxchip_detect.ch1_signal_state &= (~0xFC);
					}

					if( (cur_ch==(CAM1_CAM2)) || (cur_ch == CAM2) )
					{
						//ch3-sensor1
						backup_ch2_signal_state =  rxchip_detect.ch2_signal_state;
						if( (rxchip_detect.ch2_signalokcnt>(RXCHIP_CHECK_MAX_TIME-RXCHIP_CHECK_MIN_TIME) ) && (rxchip_detect.ch2_signalnookcnt<RXCHIP_CHECK_MIN_TIME) )
						{//have signal
							rxchip_detect.ch2_signal_state <<= 1;
							rxchip_detect.ch2_signal_state &= (~0x01);
						}
						else if( (rxchip_detect.ch2_signalokcnt<RXCHIP_CHECK_MIN_TIME) && (rxchip_detect.ch2_signalnookcnt>(RXCHIP_CHECK_MAX_TIME-RXCHIP_CHECK_MIN_TIME) ) )
						{//no signal
							rxchip_detect.ch2_signal_state <<= 1;
							rxchip_detect.ch2_signal_state |= (0x01);
						}
						rxchip_detect.ch2_signal_state &= (~0xFC);
					}

					if( cur_ch==(CAM1_CAM2) )
					{
						if( ( (rxchip_detect.ch2_signalokcnt+rxchip_detect.ch2_signalnookcnt)<(RXCHIP_CHECK_MAX_TIME-1) ) || ( (rxchip_detect.ch1_signalokcnt+rxchip_detect.ch1_signalnookcnt)<(RXCHIP_CHECK_MAX_TIME-1) ) )
						{
							signal_check_vaild_flag = true;
						}
					}
					else if( cur_ch==CAM1 )
					{
						//printf(">>>__this->ch0_ch3_signalokcnt:%d\r\n", __this->ch0_ch3_signalokcnt);
						//printf(">>>__this->ch0_ch3_signalnookcnt:%d\r\n", __this->ch0_ch3_signalnookcnt);
						if( ( ((rxchip_detect.ch1_signalokcnt)+(rxchip_detect.ch1_signalnookcnt))<(RXCHIP_CHECK_MAX_TIME-1) ) )
						{
							signal_check_vaild_flag = true;
						}
					}
					else if( cur_ch==CAM2 )
					{
						if( ((rxchip_detect.ch2_signalokcnt)+(rxchip_detect.ch2_signalnookcnt))<(RXCHIP_CHECK_MAX_TIME-1)  )
						{
							signal_check_vaild_flag = true;
						}
					}
					if( signal_check_vaild_flag == true )
					{
						XM_printf(">>>cnt error ...................\r\n");
						signal_check_vaild_flag = false;

						rxchip_detect.checktimecnt = 0;

						rxchip_detect.ch2_signalokcnt = 0;
						rxchip_detect.ch2_signalnookcnt = 0;
						rxchip_detect.ch2_signal_state = backup_ch2_signal_state;

						rxchip_detect.ch1_signalokcnt = 0;
						rxchip_detect.ch1_signalnookcnt = 0;
						rxchip_detect.ch1_signal_state = backup_ch1_signal_state;
					}
					break;


				case (RXCHIP_CHECK_MAX_TIME+1):
					if( (cur_ch==(CAM1_CAM2)) || (cur_ch == CAM1) )
					{
						if( (((rxchip_detect.ch1_signal_state)&0x03)==0x01) ){//有信号-->无信号
							rxchip_detect.ch1_signal_flag = false;
						}
						else if( (((rxchip_detect.ch1_signal_state)&0x03)==0x02) ){//无信号--->有信号
							rxchip_detect.ch1_signal_flag = true;
						}
					}

					if( (cur_ch==(CAM1_CAM2)) || (cur_ch == CAM2) )
					{
						if( (((rxchip_detect.ch2_signal_state)&0x03)==0x01) ){//有信号-->无信号
							rxchip_detect.ch2_signal_flag = false;
						}
						else if( (((rxchip_detect.ch2_signal_state)&0x03)==0x02) ){//无信号--->有信号
							rxchip_detect.ch2_signal_flag = true;
						}
					}
					break;


				case (RXCHIP_CHECK_MAX_TIME+2):
					if( (cur_ch==(CAM1_CAM2)) || (cur_ch == CAM1) )
					{
						if(rxchip_detect.ch1_signal_flag==true)
						{
							regDat = rxchip_read_sensnor0_status();
							if((regDat&0x31)==0x20)
							{//pal
								rxchip_detect.ch1_work_mode = RXCHIP_WORK_MODE_720P_PAL;
							}
							else if((regDat&0x31)==0x21)
							{//ntsc
								rxchip_detect.ch1_work_mode = RXCHIP_WORK_MODE_720P_NTSC;
							}

							if(rxchip_detect.ch1_work_mode != rxchip_detect.ch1_pre_work_mode)
							{
								if(rxchip_detect.ch1_work_mode==RXCHIP_WORK_MODE_720P_PAL)
								{
									XM_printf(">>>>ch1 pal.....\r\n");
									rxchip_sensor0_setting_ahd_pal();
									SensorData_CH0_Format = AP_DSTRESOURCE_720P_25;
								}
								else if(rxchip_detect.ch1_work_mode==RXCHIP_WORK_MODE_720P_NTSC)
								{
									XM_printf(">>>>ch1 ntsc.....\r\n");
									rxchip_sensor0_setting_ahd_ntsc();
									SensorData_CH0_Format = AP_DSTRESOURCE_720P_25;
								}
								rxchip_detect.ch1_pre_work_mode = rxchip_detect.ch1_work_mode;
							}
						}
					}

					if( (cur_ch==(CAM1_CAM2)) || (cur_ch == CAM2) )
					{
						if(rxchip_detect.ch2_signal_flag==true)
						{
							regDat = rxchip_read_sensnor1_status();
							if((regDat&0x31)==0x20)
							{//pal
								rxchip_detect.ch2_work_mode = RXCHIP_WORK_MODE_720P_PAL;
							}
							else if((regDat&0x31)==0x21)
							{//ntsc
								rxchip_detect.ch2_work_mode = RXCHIP_WORK_MODE_720P_NTSC;
							}

							if(rxchip_detect.ch2_work_mode != rxchip_detect.ch2_pre_work_mode)
							{
								if(rxchip_detect.ch2_work_mode==RXCHIP_WORK_MODE_720P_PAL)
								{
									XM_printf(">>>>ch2 pal.....\r\n");
									rxchip_sensor1_setting_ahd_pal();
									SensorData_CH1_Format = AP_DSTRESOURCE_720P_25;
								}
								else if(rxchip_detect.ch2_work_mode==RXCHIP_WORK_MODE_720P_NTSC)
								{
									XM_printf(">>>>ch2 ntsc.....\r\n");
									rxchip_sensor1_setting_ahd_ntsc();
									SensorData_CH1_Format = AP_DSTRESOURCE_720P_30;
								}
								rxchip_detect.ch2_pre_work_mode = rxchip_detect.ch2_work_mode;
							}
						}
					}
					break;

				case (RXCHIP_CHECK_MAX_TIME+3):
					if( (cur_ch==(CAM1_CAM2)) || (cur_ch == CAM1) )
					{
						//ch1
						if(((rxchip_detect.ch1_signal_state)&0x03)==0x01){//有信号-->无信号
							XM_printf(">>>>ch1 signal----->no signal\r\n") ;
							rxchip_open_panel(CAM1);
							rxchip_detect.ch1_nosignal_status = 1;
						}
						else if(((rxchip_detect.ch1_signal_state)&0x03)==0x02){//无信号--->有信号
							XM_printf(">>>>ch1 no signal------>signal\r\n") ;
							rxchip_close_panel(CAM1);
							rxchip_detect.ch1_nosignal_status = 0;
						}
						else if(((rxchip_detect.ch1_signal_state)&0x03)==0x03){//无信号--->无信号
							if((rxchip_detect.ch1_nosignal_flag) == false)
							{
								XM_printf(">>>>ch1 no signal----->no signal\r\n") ;
								rxchip_detect.ch1_nosignal_flag = true;
								rxchip_open_panel(CAM1);
								rxchip_detect.ch1_nosignal_status = 1;
							}
						}
						else if(((rxchip_detect.ch1_signal_state)&0x03)==0x00){//有信号--->有信号
							if((rxchip_detect.ch1_signalok_flag) == false)
							{
								XM_printf(">>>>ch1 signal----->signal\r\n") ;
								rxchip_detect.ch1_signalok_flag = true;
								rxchip_close_panel(CAM1);
								rxchip_detect.ch1_nosignal_status = 0;
							}
						}
					}

					if( (cur_ch==(CAM1_CAM2)) || (cur_ch == CAM2) )
					{
						//CH2
						if(((rxchip_detect.ch2_signal_state)&0x03)==0x01){//有信号-->无信号
							XM_printf(">>>>ch2 signal----->no signal\r\n") ;
							rxchip_open_panel(CAM2);
							rxchip_detect.ch2_nosignal_status = 1;
						}
						else if(((rxchip_detect.ch2_signal_state)&0x03)==0x02){//无信号--->有信号
							XM_printf(">>>>ch2 no signal------>signal\r\n") ;
							rxchip_close_panel(CAM2);
							rxchip_detect.ch2_nosignal_status = 0;
						}
						else if(((rxchip_detect.ch2_signal_state)&0x03)==0x03){//无信号--->无信号
							if((rxchip_detect.ch2_nosignal_flag) == false)
							{
								XM_printf(">>>>ch2 no signal----->no signal\r\n") ;
								rxchip_detect.ch2_nosignal_flag = true;
								rxchip_open_panel(CAM2);
								rxchip_detect.ch2_nosignal_status = 1;
							}
						}
						else if(((rxchip_detect.ch2_signal_state)&0x03)==0x00){//有信号--->有信号
							if((rxchip_detect.ch2_signalok_flag) == false)
							{
								XM_printf(">>>>ch2 signal----->signal\r\n") ;
								rxchip_detect.ch2_signalok_flag = true;
								rxchip_close_panel(CAM2);
								rxchip_detect.ch2_nosignal_status = 0;
							}
						}
					}
					rxchip_clear_mailbox();
					rxchip_detect.checktimecnt = 0;
					rxchip_detect.ch1_signalokcnt = 0;
					rxchip_detect.ch1_signalnookcnt = 0;
					rxchip_detect.ch2_signalokcnt = 0;
					rxchip_detect.ch2_signalnookcnt = 0;
					rxchip_detect.complete_check_times++;
					if( (rxchip_detect.complete_check_times>=3) && (rxchip_detect.complete_check_flag==FALSE) && (rxchip_detect.first_check_flag==FALSE))
					{
						rxchip_detect.complete_check_flag = TRUE;
						rxchip_detect.first_check_flag = TRUE;
						rxchip_detect.complete_check_times = 0;
						AP_PostSystemEvent(SYSTEM_EVENT_POWERON_START_REC);
					}
					//XM_printf(">>>>>>>>>>>rxchip_detect.complete_check_times:%d\r\n", rxchip_detect.complete_check_times);
					break;

				default:
					if( (cur_ch==(CAM1_CAM2)) || (cur_ch == CAM1) )
					{
						regDat = rxchip_read_sensnor0_status();
						//XM_printf(">>>>ch1 regDat:%x\r\n", regDat);

						//CH1-sensor0
						if(!(regDat&0x10))
						{//signal ok
							rxchip_detect.ch1_signalokcnt++;
						}
						else
						{//signal no ok
							rxchip_detect.ch1_signalnookcnt++;
						}
					}

					if( (cur_ch==(CAM1_CAM2)) || (cur_ch == CAM2) )
					{
						regDat = rxchip_read_sensnor1_status();
						//XM_printf(">>>>ch2 regDat:%x\r\n", regDat);

						//CH2-sensor1
						if(!(regDat&0x10))
						{//signal ok
							rxchip_detect.ch2_signalokcnt++;
						}
						else
						{//signal no ok
							rxchip_detect.ch2_signalnookcnt++;
						}
					}
					break;
			}
		}
	}
#endif
}

/**
 * Rxchip_TicketCallback
 *
 * rxchip 定时触发检测信号
 * 
 */
void Rxchip_TicketCallback(void)
{
    OS_SignalEvent(RXCHIP_CHECK_EVENT, &TCB_Rxchip_Task); /* 通知事件 */
	OS_RetriggerTimer (&Rxchip_TImer);
}


/**
 * rxchip_check_ctrl
 *
 * rxchip 控制rxchip检测信号
 * 
 */
void rxchip_check_ctrl(u8 val)
{
	rxchip_detect.en_rxchip_check = val;
}


/**
 * rxchip_check_ctrl
 *
 * rxchip 复位重新检测相关参数
 * 
 */
void reset_check_time(void)
{
	rxchip_detect.checktimecnt = 0;
	rxchip_detect.ch1_signalokcnt = 0;
	rxchip_detect.ch1_signalnookcnt = 0;
	rxchip_detect.ch2_signalokcnt = 0;
	rxchip_detect.ch2_signalnookcnt = 0;
   	rxchip_detect.ch1_nosignal_flag = 0;
    rxchip_detect.ch1_signalok_flag = 0;
    rxchip_detect.ch2_nosignal_flag = 0;
    rxchip_detect.ch2_signalok_flag = 0;
	rxchip_detect.parkingon_flag = false;
	rxchip_detect.parkingoff_flag = false;
}


/**
 * rxchip_check_ctrl
 *
 * rxchip 控制参数清零
 * 
 */
void rxchip_ctrl_clear(void)
{
	rxchip_detect.en_rxchip_check =0;
	rxchip_detect.ch1_signalokcnt =0;
	rxchip_detect.ch1_signalnookcnt =0;
	rxchip_detect.ch1_signal_state =0x03;
    rxchip_detect.ch1_signal_flag =false;
	rxchip_detect.ch1_work_mode =RXCHIP_WORK_MODE_NO_SIGNAL;
	rxchip_detect.ch1_pre_work_mode =RXCHIP_WORK_MODE_NO_SIGNAL;
    rxchip_detect.ch1_nosignal_flag =false;
    rxchip_detect.ch1_signalok_flag =false;

	rxchip_detect.ch2_signalokcnt =0;
	rxchip_detect.ch2_signalnookcnt =0;
	rxchip_detect.ch2_signal_state =0x03;
    rxchip_detect.ch2_signal_flag =false;
	rxchip_detect.ch2_work_mode =RXCHIP_WORK_MODE_NO_SIGNAL;
	rxchip_detect.ch2_pre_work_mode =RXCHIP_WORK_MODE_NO_SIGNAL;
    rxchip_detect.ch2_nosignal_flag =false;
    rxchip_detect.ch2_signalok_flag =false;

	rxchip_detect.checktimecnt =0;
    rxchip_detect.parkingon_flag =0;
    rxchip_detect.parkingoff_flag =0;
    rxchip_detect.signal_check_flag =0;

	rxchip_detect.ch1_nosignal_status = 0;//
	rxchip_detect.ch2_nosignal_status = 0;

	rxchip_detect.complete_check_flag = FALSE;
	rxchip_detect.first_check_flag = FALSE;
}

// 清除缓存的应答消息
static void rxchip_clear_mailbox(void)
{
	OS_Use(&rxchip_sema);
	OS_ClearMB(&rxchip_mailbox);
	OS_Unuse(&rxchip_sema);
}


/**
 * rxchip_get_ch1_signal_status
 *
 * rxchip 返回通道1 无信号状态
 * 
 */
u8 rxchip_get_ch1_signal_status(void)
{
	return rxchip_detect.ch1_nosignal_status;
}


/**
 * rxchip_get_ch2_signal_status
 *
 * rxchip 返回通道2 无信号状态
 * 
 */
u8 rxchip_get_ch2_signal_status(void)
{
	return rxchip_detect.ch2_nosignal_status;
}


/**
 * rxchip_get_check_flag
 *
 * rxchip 返回检测完成标志
 * 
 */
u8 rxchip_get_check_flag(void)
{
	return rxchip_detect.complete_check_flag;
}

void rxchip_set_check_flag(u8 val)
{
	rxchip_detect.complete_check_flag = val;
}


/**
 * rxchip_set_delay_data
 *
 * rxchip 延迟数据到屏幕显示
 * 
 */
void rxchip_set_delay_data(u8_t val)
{
	Delay_rxchip_Data = val;
}


/**
 * rxchip_utc_cmd_update
 *
 * rxchip UTC命令更新校验和
 * 
 */
void rxchip_utc_cmd_update(void)
{
	utc_head[3] = utc_head[0]^utc_head[1]^utc_head[2];
	utc_mirror_cmd[3] = utc_mirror_cmd[0]^utc_mirror_cmd[1]^utc_mirror_cmd[2];
	utc_normal_cmd[3] = utc_normal_cmd[0]^utc_normal_cmd[1]^utc_normal_cmd[2];

	//utc_head[3] = utc_change_value(utc_head[3]);
	//utc_mirror_cmd[3] = utc_change_value(utc_mirror_cmd[3]);
	//utc_normal_cmd[3] = utc_change_value(utc_normal_cmd[3]);
	
	XM_printf(">>>>utc_head[3]:%d\r\n", utc_head[3]);
	XM_printf(">>>>utc_mirror_cmd[3]:%d\r\n", utc_mirror_cmd[3]);
	XM_printf(">>>>utc_normal_cmd[3]:%d\r\n", utc_normal_cmd[3]);
}



/**
 * XMSYS_RxchipInit
 *
 * rxchip 任务初始化
 * 
 */
void XMSYS_RxchipInit(void)
{
    XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>XMSYS_RxchipInit\r\n");

	rxchip_ctrl_clear();//控制参数清0
	rxchip_I2C_Init();//rxchip iic初始化
	rxchip_reset_pin_init();//reset pin初始化

	#ifdef EN_UTC_FUN
	rxchip_utc_cmd_update();
	#endif
	
	OS_CREATERSEMA(&rxchip_sema);
	OS_CREATEMB(&rxchip_mailbox, sizeof(RXCHIPCMD), 10, &rxchip_mailbuf);	
	OS_CREATETIMER(&Rxchip_TImer, Rxchip_TicketCallback, 50);//50ms，定时发送事件
	OS_CREATETASK(&TCB_Rxchip_Task, "RXCHIP", rxchip_task, XMSYS_RXCHIP_TASK_PRIORITY, StackRxchipTask);
}

void XM_RxchipDisplayInit()
{
    RXCHIPCMD cmd;
    cmd.cmd = CMD_BRIGHTNESS;
    cmd.dat = AP_GetColor_Brightness();
    rxchip_setting(cmd);
    
    cmd.cmd = CMD_CONTRAST;
    cmd.dat = AP_GetColor_Contrast();
    rxchip_setting(cmd);
    
    cmd.cmd = CMD_SATURATION;
    cmd.dat = AP_GetColor_Saturation();
    rxchip_setting(cmd);
    
    cmd.cmd = CMD_HUE;
    cmd.dat = AP_GetColor_Tone();
    rxchip_setting(cmd);
}

void XMSYS_RxchipExit(void)
{
	
}

#endif

