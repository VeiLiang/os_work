
#include "hardware.h"
#include "gpio.h"
#include "gpioi2c.h"
#include "ark7116_drv.h"
#include "xm_core.h"
#include "xm_printf.h"
#include "xm_dev.h"
#include <xm_video_task.h>


#ifdef _XMSYS_ARK7116_SUPPORT_

#define ARK7116_FUNCTION    0
#define ARK7116_ERROR       0

#define ARK7116_CONFIG_EVENT		0x01
extern int ITU656_in;
//static HANDLE lg_hI2C;   // I2C Bus Driver

static OS_TASK TCB_ARK7116_Task;
__no_init static OS_STACKPTR int StackARK7116Task[XMSYS_ARK7116_TASK_STACK_SIZE/4];          /* Task stacks */



//#define	RETAILMSG(...)
#define	RETAILMSG	XM_printf
#define	TEXT

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

Update date:Friday, September 23, 2016
Update time:17:44:22
*/

PanlstaticPara AV1_staticPara[]=
{
//GLOBAL
    {0XFD01,0X01}, 
    {0XFD02,0X00}, 
    {0XFD0A,0X90}, 
    {0XFD0B,0X1D}, 
    {0XFD0C,0X33}, 
    {0XFD0D,0X20}, 
    {0XFD0E,0X2C}, 
    {0XFD0F,0X09}, 
    {0XFD10,0X01}, 
    {0XFD11,0XFF}, 
    {0XFD12,0XFF}, 
    {0XFD13,0XFF}, 
    {0XFD14,0X02}, 
    {0XFD15,0X02}, 
    {0XFD16,0X0A}, 
    {0XFD1A,0X40}, 
    {0XFD1B,0XFF}, 
    {0XFD1C,0XFF}, 
    {0XFD1D,0X00}, 
    {0XFD5B,0X04}, 
    {0XFD19,0X4A}, 


#if 0
    {0XFD30,0X22},	 
    {0XFD31,0X00},
    {0XFD32,0X11},
    {0XFD33,0X11},
    {0XFD34,0X55},
    {0XFD35,0X55},
    {0XFD36,0X55},
    {0XFD37,0X55},
    {0XFD38,0X55},
    {0XFD39,0X55},
    {0XFD3A,0X11},
    {0XFD3B,0X11},

    {0XFD3C,0X00},
    {0XFD3D,0X00},
    {0XFD3E,0X00},
    {0XFD3F,0X00},
    {0XFD40,0X00},
    {0XFD41,0X00},
    {0XFD42,0X00},
    {0XFD43,0X00},
    {0XFD44,0X01},
    {0XFD45,0x00},

    {0XFD46,0X00},	 
    {0XFD47,0X00},
    {0XFD48,0X00},
    {0XFD49,0X00},
    {0XFD4A,0X00},
    {0XFD4B,0X00},
    {0XFD4C,0X12},
    {0XFD4D,0X4F},
    {0XFD4E,0X02},
    {0XFD4F,0X27},

    {0XFD50,0X0B},
    {0XFD51,0X00},
    {0XFD52,0X00},
    {0XFD53,0X00},
    {0XFD54,0X00},
    {0XFD55,0X00},
    {0XFD56,0X00},
    {0XFD57,0X00},
    {0XFD58,0X00},
    {0XFD59,0X00},
    {0XFD5A,0X00},


#else
// PAD MUX
    {0XFD30,0X22},	 
    {0XFD31,0X00},
    {0XFD32,0X11},
    {0XFD33,0X11},
    {0XFD34,0X55},
    {0XFD35,0X55},
    {0XFD36,0X55},
    {0XFD37,0X55},
    {0XFD38,0X55},
    {0XFD39,0X55},
    {0XFD3A,0X11},
    {0XFD3B,0X11},
    {0XFD3C,0X00},
    {0XFD3D,0X00},
    {0XFD3E,0X00},
    {0XFD3F,0X00},
    {0XFD40,0X00},
    {0XFD41,0X00},
    {0XFD42,0X00},
    {0XFD43,0X00},
    {0XFD44,0X01},
    {0XFD45,0x00},
    {0XFD46,0X00},	 
    {0XFD47,0X00},
    {0XFD48,0X00},
    {0XFD49,0X00},
    {0XFD4A,0X00},
    {0XFD4B,0X00},
    {0XFD4C,0X12},
    {0XFD4D,0X4F},
    {0XFD4E,0X02},
    {0XFD4F,0X27},
    {0XFD50,0X0B},
    {0XFD51,0X00},
    {0XFD52,0X00},
    {0XFD53,0X00},
    {0XFD54,0X00},
    {0XFD55,0X00},
    {0XFD56,0X00},
    {0XFD57,0X00},
    {0XFD58,0X00},
    {0XFD59,0X00},
    {0XFD5A,0X00},
#endif




	 

//PWM
//DECODER
       {0XFE00,0X80}, 
    {0XFE01,0X06}, 
    {0XFE02,0X00}, 
    {0XFE03,0X80}, 
    {0XFE04,0X80}, 
    {0XFE05,0X30}, 
    {0XFE06,0X02}, 
    {0XFE07,0X80}, 
    {0XFE08,0X00}, 
    {0XFE09,0X00}, 
    {0XFE0A,0X2F}, 
    {0XFE0B,0X40}, 
    {0XFE0C,0X10}, 
    {0XFE0D,0X03}, 
    {0XFE0E,0X72}, 
    {0XFE0F,0X07}, 
    {0XFE10,0X14}, 
    {0XFE11,0X09}, 
    {0XFE12,0X00}, 
    {0XFE13,0X16}, 
    {0XFE14,0X22}, 
    {0XFE15,0X05}, 
    {0XFE26,0X0E}, 
    {0XFE27,0X08}, 
    {0XFE28,0X05}, 
    {0XFE2A,0X10}, 
    {0XFE35,0XAA}, 
    {0XFE36,0XAA}, 
    {0XFE37,0X60}, 
    {0XFE38,0X0F}, 
    {0XFE39,0X08}, 
    {0XFE48,0X07}, 
    {0XFE54,0X00}, 
    {0XFE55,0X0A}, 
    {0XFE5F,0XC0}, 
    {0XFE60,0X03}, 
    {0XFE61,0X2D}, 
    {0XFE62,0X41}, 
    {0XFE63,0Xc0}, 
    {0XFE83,0XDB}, 
    {0XFEA0,0X02}, 
    {0XFEAA,0X06}, 
    {0XFEAB,0X12}, 
    {0XFEAC,0XA9}, 
    {0XFEB1,0X02}, 
    {0XFEB2,0X04}, 
    {0XFEB5,0X67}, 
    {0XFEC9,0X00}, 
    {0XFECA,0X4F}, 
    {0XFECD,0X32}, 
    {0XFED0,0X51}, 

    {0XFED1,0X19}, 
    {0XFED2,0X00}, 
    {0XFED3,0X00}, 
    {0XFED4,0X00}, 
    {0XFED5,0XB5}, 
    {0XFED6,0X08}, 
    {0XFED7,0XFF}, 
    {0XFED8,0XE3}, 
    {0XFED9,0X40}, 
    {0XFEDA,0X29}, 
    {0XFEDB,0X00}, 
    {0XFEDC,0X00}, 
    {0XFEDD,0X51}, 
    {0XFEDE,0X59}, 
    {0XFEDF,0X53}, 
    {0XFEE0,0X29},
    {0XFEE1,0X96},
    {0XFEE2,0X02},
    {0XFE44,0X20},
    {0XFE45,0X80},
    {0XFE43,0X80},
    {0XFECB,0X06},
    {0XFE56,0X07},
    {0XFE46,0X00},

#if 0
//VP
    {0XFFB0 ,0X67},
    {0XFFB1 ,0X0F},
    {0XFFB2 ,0X10},
    {0XFFB3 ,0X10},
    {0XFFB4 ,0X10},
    {0XFFB5 ,0X60},
    {0XFFB6 ,0X10},
    {0XFFB7 ,0X10},
    {0XFFB8 ,0X10},
    {0XFFB9 ,0X22},
    {0XFFBA ,0X20},
    {0XFFBB ,0X22},
    {0XFFBC ,0X20},
    {0XFFBD ,0X20},
    {0XFFBE ,0X20},
    {0XFFBF ,0X20},
    {0XFFC0 ,0XE0},
    {0XFFC1 ,0X20},
    {0XFFC2 ,0XB5},
    {0XFFC3 ,0XB5},
    {0XFFC4 ,0XFF},
    {0XFFC5 ,0XFF},
    {0XFFC6 ,0X99},
    {0XFFC7 ,0X31},
    {0XFFC8 ,0X10},
    {0XFFC9 ,0X30},
    {0XFFCA ,0X00},
    {0XFFCB ,0X80},
    {0XFFCC ,0X80},
    {0XFFCD ,0X2D},
    {0XFFCE ,0X13},
    {0XFFCF ,0XDD},
    {0XFFD0 ,0X72},
    {0XFFD1 ,0X40},
    {0XFFD2 ,0X4F},
    {0XFFD3 ,0X88},
    {0XFFD4 ,0X78},
    {0XFFD7 ,0X10},
    {0XFFD8 ,0X80},
    {0XFFD9 ,0X80},
    {0XFFDA ,0X5C},
    {0XFFDD ,0XFF},
    {0XFFDE ,0X0E},
    {0XFFDF ,0X0E},
    {0XFFE0 ,0X0E},
    {0XFFE1 ,0X0E},
    {0XFFE2 ,0X0E},
    {0XFFE3 ,0X0E},
    {0XFFE4 ,0X0E},
    {0XFFE5 ,0X0E},
    {0XFFE6 ,0X0E},
    {0XFFE7 ,0X50},
    {0XFFE8 ,0X10},
    {0XFFE9 ,0X22},
    {0XFFEA ,0X20},
    {0XFFF0 ,0X3A},
    {0XFFF1 ,0XCB},
    {0XFFF2 ,0XF3},
    {0XFFF3 ,0XD1},
    {0XFFF4 ,0XFD},
    {0XFFF5 ,0X36},
    {0XFFF6 ,0XFC},
    {0XFFF7 ,0XDD},
    {0XFFF8 ,0XED},
    {0XFFF9 ,0XFD},
    {0XFFFA ,0X55},
    {0XFFFB ,0x81},
    {0XFFD5 ,0x00},
    {0XFFD6 ,0x15},
    #endif
	//VP
    
    {0XFFB0,0X67}, 
    {0XFFB1,0X0F}, 
    {0XFFB2,0X10}, 
    {0XFFB3,0X10}, 
    {0XFFB4,0X10}, 
    {0XFFC8,0X05}, 
    {0XFFC9,0X00}, 
    {0XFFCE ,0X13},
    {0XFFCF ,0XDD},
    {0XFFD0 ,0X72},
    {0XFFD7 ,0X10},
    {0XFFE8,0X10}, 
    {0XFFF0,0X3A}, 
    {0XFFF1,0XCB}, 
    {0XFFF2,0XF3}, 
    {0XFFF3,0XD1}, 
    {0XFFF4,0XFD}, 
    {0XFFF5,0X36}, 
    {0XFFF6,0XFC}, 
    {0XFFF7,0XDD}, 
    {0XFFF8,0XED}, 
    {0XFFF9,0XFD}, 
    {0XFFFA,0X55}, 
    {0XFFFB,0X01}, 



//TCON
    {0XFC00 ,0X05},
    {0XFC01 ,0X00},
    {0XFC02 ,0X00},
    {0XFC03 ,0X00},
    {0XFC04 ,0X00},
    {0XFC05 ,0X00},
    {0XFC06 ,0X00},
    {0XFC07 ,0X00},
    {0XFC08 ,0X00},
    {0XFC09 ,0X0E},
    {0XFC0A ,0X33},
    {0XFC0B ,0X00},
    {0XFC0C ,0X00},
    {0XFC0D ,0X00},
    {0XFC0E ,0X00},
    {0XFC0F ,0X00},
    {0XFC10 ,0X00},
    {0XFC11 ,0XFF},
    {0XFC12 ,0X10},
    {0XFC13 ,0X00},
    {0XFC14 ,0X03},
    {0XFC15 ,0X00},
    {0XFC16 ,0X03},
    {0XFC17 ,0X00},
    {0XFC18 ,0X10},
    {0XFC19 ,0X00},
    {0XFC1A ,0X14},
    {0XFC1B ,0X00},
    {0XFC1C ,0X06},
    {0XFC1D ,0X00},
    {0XFC1E ,0X0E},
    {0XFC1F ,0X00},
    {0XFC20 ,0X1B},
    {0XFC21 ,0X00},
    {0XFC22 ,0X1F},
    {0XFC23 ,0X00},
    {0XFC24 ,0X1B},
    {0XFC25 ,0X00},
    {0XFC26 ,0X1F},
    {0XFC27 ,0X00},
    {0XFC28 ,0X10},
    {0XFC29 ,0X00},
    {0XFC2A ,0X12},
    {0XFC2B ,0X00},
    {0XFC2C ,0X04},
    {0XFC2D ,0X00},
    {0XFC2E ,0X04},
    {0XFC2F ,0X00},
    {0XFC30 ,0X01},
    {0XFC31 ,0X00},
    {0XFC32 ,0X01},
    {0XFC33 ,0X00},
    {0XFC34 ,0X08},
    {0XFC35 ,0X09},
    {0XFC36 ,0X09},
    {0XFC37 ,0X0D},
    {0XFC38 ,0X80},
    {0XFC39 ,0X80},
    {0XFC3A ,0X80},
    {0XFC3B ,0X80},
    {0XFC3C ,0X80},
    {0XFC3D ,0X80},
    {0XFC3E ,0XA0},
    {0XFC3F ,0XA0},
    {0XFC40 ,0XA0},
    {0XFC41 ,0X3F},
    {0XFC42 ,0XBD},
    {0XFC43 ,0X04},
    {0XFC44 ,0X00},
    {0XFC45 ,0X04},
    {0XFC46 ,0X39},


#if 0
//SCALE
    {0XFC90 ,0X02},
    {0XFC91 ,0X00},
    {0XFC92 ,0X00},
    {0XFC93 ,0X0C},
    {0XFC94 ,0X00},
    {0XFC95 ,0X00},
    {0XFC96 ,0X55},
    {0XFC97 ,0X04},
    {0XFC98 ,0XF6},
    {0XFC99 ,0X01},
    {0XFC9A ,0X46},
    {0XFC9B ,0X03},
    {0XFC9C ,0X02},
    {0XFC9D ,0X00},
    {0XFC9E ,0X06},
    {0XFC9F ,0X00},
    {0XFCA0 ,0X3D},
    {0XFCA1 ,0X00},
    {0XFCA2 ,0XC5},
    {0XFCA3 ,0X02},
    {0XFCA4 ,0X0C},
    {0XFCA5 ,0X00},
    {0XFCA6 ,0X16},
    {0XFCA7 ,0X00},
    {0XFCA8 ,0X19},
    {0XFCA9 ,0X00},
    {0XFCAA ,0X1B},
    {0XFCAB ,0X02},
    {0XFCAC ,0X13},
    {0XFCAD ,0X00},
    {0XFCAE ,0X00},
    {0XFCAF ,0X00},
    {0XFCB0 ,0X00},
    {0XFCB1 ,0X14},
    {0XFCB2 ,0X00},
    {0XFCB3 ,0X00},
    {0XFCB4 ,0X00},
    {0XFCB5 ,0X00},
    {0XFCB7 ,0X1F},
    {0XFCB8 ,0X02},
    {0XFCBB ,0X2C},
    {0XFCBC ,0X02},
    {0XFCBD ,0X00},
    {0XFCBE ,0X00},
    {0XFCBF ,0X0C},
    {0XFCC0 ,0X00},
    {0XFCC1 ,0X00},
    {0XFCC2 ,0X60},
    {0XFCC3 ,0X04},
    {0XFCC4 ,0X4D},
    {0XFCC5 ,0X02},
    {0XFCC6 ,0XE0},
    {0XFCC7 ,0X03},
    {0XFCC8 ,0X02},
    {0XFCC9 ,0X00},
    {0XFCCA ,0X06},
    {0XFCCB ,0X00},
    {0XFCCC ,0X3D},
    {0XFCCD ,0X00},
    {0XFCCE ,0XC5},
    {0XFCCF ,0X02},
    {0XFCD1 ,0X00},
    {0XFCD2 ,0X15},
    {0XFCD3 ,0X00},
    {0XFCD4 ,0X24},
    {0XFCD5 ,0X00},
    {0XFCD6 ,0X23},
    {0XFCD7 ,0X02},
    {0XFCD8 ,0X06},
    {0XFCD9 ,0X00},
    {0XFCDA ,0X00},
    {0XFCDB ,0X00},
    {0XFCDC ,0X00},
    {0XFCDD ,0X14},
    {0XFCDE ,0X00},
    {0XFCDF ,0X00},
    {0XFCE0 ,0X00},
    {0XFCE1 ,0X02},
    {0XFCE3 ,0X01},
    {0XFCE4 ,0X02},
    {0XFCE5 ,0XE0},
    {0XFCE6 ,0X03},
    {0XFCE7 ,0X00},
    {0XFCE8 ,0X04},
    {0XFCD0 ,0X0A},
    {0XFCE9 ,0X40},
    {0XFCE2 ,0X00},
    {0XFCB6 ,0X00},
    {0XFB35 ,0x00},
    {0XFB89 ,0x00},
    {0xFCEA ,0xFF},
    #endif
	//SCALE
	#if 1
    {0XFC90,0X02}, 
    {0XFC91,0X00}, 
    {0XFC92,0X00}, 
    {0XFC93,0X0C}, 
    {0XFC94,0X00}, 
    {0XFC95,0X00}, 
    {0XFC98,0XF6}, 
    {0XFC99,0X01}, 
    {0XFC9A,0X46}, 
    {0XFC9B,0X03}, 
    {0XFC9C,0X02}, 
    {0XFC9D,0X00}, 
    {0XFC9E,0X06}, 
    {0XFC9F,0X00}, 
    {0XFCA0,0X3D}, 
    {0XFCA1,0X00}, 
    {0XFCA2,0XC5}, 
    {0XFCA3,0X02}, 
    {0XFCA4,0X0C}, 
    {0XFCA5,0X00}, 
    {0XFCA6,0X16}, 
    {0XFCA7,0X00}, 
    {0XFCA8,0X19}, 
    {0XFCA9,0X00}, 
    {0XFCAA,0X1B}, 
    {0XFCAB,0X02}, 
    {0XFCB1,0X14}, 
    {0XFCB2,0X00}, 
    {0XFCB3,0X00}, 
    {0XFCB4,0X00}, 
    {0XFCB5,0X00}, 
    {0XFCB7,0X1F}, 
    {0XFCB8,0X02}, 
    {0XFCBB,0X2C}, 
    {0XFCBC,0X02}, 
    {0XFCBD,0X00}, 
    {0XFCBE,0X00}, 
    {0XFCBF,0X0C}, 
    {0XFCC0,0X00}, 
    {0XFCC1,0X00}, 
    {0XFCC4,0X4D}, 
    {0XFCC5,0X02}, 
    {0XFCC6,0XE0}, 
    {0XFCC7,0X03}, 
    {0XFCC8,0X02}, 
    {0XFCC9,0X00}, 
    {0XFCCA,0X06}, 
    {0XFCCB,0X00}, 
    {0XFCCC,0X3D}, 
    {0XFCCD,0X00}, 
    {0XFCCE,0XC5}, 
    {0XFCCF,0X02}, 
    {0XFCD1,0X00}, 
    {0XFCD2,0X15}, 
    {0XFCD3,0X00}, 
    {0XFCD4,0X24}, 
    {0XFCD5,0X00}, 
    {0XFCD6,0X23}, 
    {0XFCD7,0X02}, 
    {0XFCD9,0X02},
    {0XFCDD,0X14}, 
    {0XFCDE,0X00}, 
    {0XFCDF,0X00}, 
    {0XFCE0,0X00}, 
    {0XFCE1,0X02}, 
    {0XFCD0,0X0A}, 
    {0XFCE2,0X00}, 
    {0XFCB6,0X00}, 
    {0XFB35,0X00}, 
    {0XFB89,0X00}, 
     #endif


	//GAMMA
    {0XFF00,0X03}, 
    {0XFF01,0X0B}, 
    {0XFF02,0X16}, 
    {0XFF03,0X22},
    {0XFF04,0X2E}, 
    {0XFF05,0X3A}, 
    {0XFF06,0X46}, 
    {0XFF07,0X53}, 
    {0XFF08,0X5F}, 
    {0XFF09,0X6A}, 
    {0XFF0A,0X75}, 
    {0XFF0B,0X80}, 
    {0XFF0C,0X8A}, 
    {0XFF0D,0X94}, 
    {0XFF0E,0X9D}, 
    {0XFF0F,0XA6}, 
    {0XFF10,0XAE}, 
    {0XFF11,0XB6}, 
    {0XFF12,0XBD}, 
    {0XFF13,0XC4}, 
    {0XFF14,0XCA}, 
    {0XFF15,0XD0}, 
    {0XFF16,0XD5}, 
    {0XFF17,0XDB}, 
    {0XFF18,0XDF}, 
    {0XFF19,0XE4}, 
    {0XFF1A,0XE8}, 
    {0XFF1B,0XEC}, 
    {0XFF1C,0XF0}, 
    {0XFF1D,0XF4}, 
    {0XFF1E,0XF8}, 
    {0XFF1F,0XFB}, 
    {0XFF20,0X0B}, 
    {0XFF21,0X16}, 
    {0XFF22,0X22}, 
    {0XFF23,0X2E}, 
    {0XFF24,0X3A}, 
    {0XFF25,0X46}, 
    {0XFF26,0X53}, 
    {0XFF27,0X5F}, 
    {0XFF28,0X6A}, 
    {0XFF29,0X75}, 
    {0XFF2A,0X80}, 
    {0XFF2B,0X8A}, 
    {0XFF2C,0X94}, 
    {0XFF2D,0X9D}, 
    {0XFF2E,0XA6}, 
    {0XFF2F,0XAE}, 
    {0XFF30,0XB6}, 
    {0XFF31,0XBD}, 
    {0XFF32,0XC4}, 
    {0XFF33,0XCA}, 
    {0XFF34,0XD0}, 
    {0XFF35,0XD5}, 
    {0XFF36,0XDB}, 
    {0XFF37,0XDF}, 
    {0XFF38,0XE4}, 
    {0XFF39,0XE8}, 
    {0XFF3A,0XEC}, 
    {0XFF3B,0XF0}, 
    {0XFF3C,0XF4}, 
    {0XFF3D,0XF8}, 
    {0XFF3E,0XFB}, 
    {0XFF3F,0X0B}, 
    {0XFF40,0X16}, 
    {0XFF41,0X22}, 
    {0XFF42,0X2E}, 
    {0XFF43,0X3A}, 
    {0XFF44,0X46}, 
    {0XFF45,0X53}, 
    {0XFF46,0X5F}, 
    {0XFF47,0X6A}, 
    {0XFF48,0X75}, 
    {0XFF49,0X80}, 
    {0XFF4A,0X8A}, 
    {0XFF4B,0X94}, 
    {0XFF4C,0X9D}, 
    {0XFF4D,0XA6}, 
    {0XFF4E,0XAE}, 
    {0XFF4F,0XB6}, 
    {0XFF50,0XBD}, 
    {0XFF51,0XC4}, 
    {0XFF52,0XCA}, 
    {0XFF53,0XD0}, 
    {0XFF54,0XD5}, 
    {0XFF55,0XDB}, 
    {0XFF56,0XDF}, 
    {0XFF57,0XE4}, 
    {0XFF58,0XE8}, 
    {0XFF59,0XEC}, 
    {0XFF5A,0XF0}, 
    {0XFF5B,0XF4}, 
    {0XFF5C,0XF8}, 
    {0XFF5D,0XFB}, 
    {0XFF5E,0XFF}, 
    {0XFF5F,0XFF}, 
    {0XFF60,0XFF}, 
};

PanlPosDynPara  AV1_posDynPara[]=
{
//dispmode:  16:9  4:3  DM_EX0  DM_EX1  DM_EX2  DM_EX3
//GLOBAL
//PAD MUX
//PWM
//GPIO
//DECODER
//VP
//TCON
//SCALE
    {0XFC96,{0X55,0X70,0X70,0X70,0X70,0X70}},
    {0XFC97,{0X04,0X03,0X03,0X03,0X03,0X03}},
    {0XFCAC,{0X13,0X1D,0X1D,0X1D,0X1D,0X1D}},
    {0XFCAD,{0X00,0X00,0X00,0X00,0X00,0X00}},
    {0XFCAE,{0X00,0X00,0X00,0X00,0X00,0X00}},
    {0XFCAF,{0X00,0X00,0X00,0X00,0X00,0X00}},
    {0XFCB0,{0X00,0X00,0X00,0X00,0X00,0X00}},
    {0XFCC2,{0X60,0X7A,0X7A,0X7A,0X7A,0X7A}},
    {0XFCC3,{0X04,0X03,0X03,0X03,0X03,0X03}},
    {0XFCD8,{0X06,0X08,0X08,0X08,0X08,0X08}},
    //{0XFCD9,{0X00,0X02,0X02,0X02,0X02,0X02}},
    {0XFCDA,{0X00,0X00,0X00,0X00,0X00,0X00}},
    {0XFCDB,{0X00,0X00,0X00,0X00,0X00,0X00}},
    {0XFCDC,{0X00,0X00,0X00,0X00,0X00,0X00}},
//GAMMA
};
PanlSysDynPara  AV1_sysDynPara[]=
{
//picSys:   PAL  PAL-N  PAL-M  NTSC SECAM PAL-60 NTSC-J NTSC-4.43
//GLOBAL
//PAD MUX
//PWM
//GPIO
//DECODER
//VP
 
    {0XFFD3,{0X80, 0X7B, 0X7B, 0X80, 0X7B, 0X7B, 0X7B, 0X7B}},
    {0XFFD4,{0X80, 0X7A, 0X7A, 0X80, 0X7A, 0X7A, 0X7A, 0X7A}},
    {0XFFD5,{0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}},
    {0XFFD6,{0X35, 0X36, 0X36, 0X35, 0X36, 0X36, 0X36, 0X36}},
//TCON
//SCALE
//GAMMA
};
//

/*点屏 PAD MUX 参数*/
PanlstaticPara  AMT_PadMuxStaticPara[]=
{
//PAD MUX
    {0XFD30,0X22},	 
    {0XFD31,0X00},
    {0XFD32,0X11},
    {0XFD33,0X11},
    {0XFD34,0X55},
    {0XFD35,0X55},
    {0XFD36,0X55},
    {0XFD37,0X55},
    {0XFD38,0X55},
    {0XFD39,0X55},
    {0XFD3A,0X11},
    {0XFD3B,0X11},
    {0XFD3C,0X55},
    {0XFD3D,0X55},
    {0XFD3E,0X55},
    {0XFD3F,0X55},
    {0XFD40,0X55},
    {0XFD41,0X55},
    {0XFD42,0X00},
    {0XFD43,0X00},
    {0XFD44,0X01},
    {0XFD45,0x00},
    {0XFD46,0X00},	 
    {0XFD47,0X00},
    {0XFD48,0X00},
    {0XFD49,0X00},
    {0XFD4A,0X00},
    {0XFD4B,0X00},
    {0XFD4C,0X12},
    {0XFD4D,0X4F},
    {0XFD4E,0X02},
    {0XFD4F,0X27},
    {0XFD50,0X0B},
    {0XFD51,0X00},
    {0XFD52,0X00},
    {0XFD53,0X00},
    {0XFD54,0X00},
    {0XFD55,0X00},
    {0XFD56,0X00},
    {0XFD57,0X00},
    {0XFD58,0X00},
    {0XFD59,0X00},
    {0XFD5A,0X00},
 

};

/*不同通道屏参的定义、初始化*/
VideoChannel VideoChannelPara[]=
{
	{ INPUT_AV1,{AV1_staticPara,AV1_posDynPara,AV1_sysDynPara},},
};


DWORD I2C_ARK7116_Write(UINT8 devaddr, UINT8 *pi2cData, UINT32 length)
{
	int ret;
	if(length < 2)
		return 0xFFFFFFFF;
	
	ret = ark7116_i2c_write_bytes (devaddr, pi2cData, 1, (char *)(pi2cData + 1), (int)(length - 1));
	if(ret < 0)
		return 0xFFFFFFFF;
	else
		return 0;
}

DWORD I2C_ARK7116_Read(UINT8 devaddr, UINT8 wordaddr, UINT8 *pBufOut)
{
	 //int ark7116_i2c_read_bytes (unsigned int slvaddr, unsigned char* addr, int size, char *rxdata, int length)
	 int ret = ark7116_i2c_read_bytes (devaddr, &wordaddr, 1, (char *)pBufOut, 1);
	 if(ret < 0)
		 return 0xFFFFFFFF;
	 else
		 return 0;
}

/*
static void busy_delay(UINT32 time)
{
	volatile unsigned int loops = time;

	while(loops--);
}
*/

/****************************************************************************
*name:   AMT_WriteReg(UINT RegAddr,UCHAR RegVal)
*input:  RegAddr, RegVal             
*output: non  

*description:写AMT Slave模式下寄存器。
*history:   yshuizhou   2013/08/08    1.0    build   this  function
*****************************************************************************/
int  AMT_WriteReg(UINT16 RegAddr, UINT8 RegVal)
{
	int ucLoop;
	UINT8 ucDeviceAddr;
	UINT8 uctmpDeviceAddr;
	UINT8 ucSubAddr;

	unsigned char readRegval;

	ucLoop = I2C_ACCESS_LOOP_TIME;
	uctmpDeviceAddr = (unsigned char)((RegAddr>>8)&0XFF);
	ucSubAddr = (UINT8)(RegAddr&0XFF);

	switch(uctmpDeviceAddr)
	{
	    case 0XF9:
	    case 0XFD:
			 ucDeviceAddr= 0XB0;
			 break;

		case 0XFA:
			 ucDeviceAddr= 0XBE;
			 break;
			 	
		case 0XFB:
			 ucDeviceAddr= 0XB6;
			 break;

	    case 0XFC:
			 ucDeviceAddr= 0XB8;
			 break;

		case 0XFE:
			 ucDeviceAddr= 0XB2;
			 break;

		case 0XFF:
			 ucDeviceAddr= 0XB4;
			 break;

		case 0X00:
			ucDeviceAddr = 0XBE;
			break;
			
		default:
			 ucDeviceAddr= 0XB0;
			 break;			
	}
	while(ucLoop--)
	{
		if(ark7116_i2c_write_bytes (ucDeviceAddr, &ucSubAddr, 1, (char *)&RegVal, 1) == 0)
			break;
	}
	if(ucLoop == 0)
		return -1;
	else
		return 0;
}

/****************************************************************************
*name:   ConfigSlaveMode(void)
*input:  CtrlMode              
*output: void  

*description:
*****************************************************************************/
void ConfigSlaveMode(void)
{   
    unsigned char AddrBuff[6] = {0xa1,0xa2,0xa3,0xa4,0xa5,0xa6};
    unsigned char DataBuff[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned char i;
	
	RETAILMSG((TEXT("+++ConfigSlaveMode! \r\n")));
	
    DataBuff[0] = 0X55;
    DataBuff[1] = 0xAA;
    DataBuff[2] = 0X03;
    DataBuff[3] = 0X50;  //slave mode
    DataBuff[4] = 0;     // crc val
    DataBuff[5] = DataBuff[2]^DataBuff[3]^DataBuff[4];
	 
    AMT_WriteReg(MCU_CFG_ADDR,0X40);
    AMT_WriteReg(RSTN,0X00);
	AMT_WriteReg(RSTN,0X5A);	 

	AMT_WriteReg(BUS_STATUS_ADDR, 0x00);  //I2c Write Start
	
	for(i =0;i < 6;i++)
	{
	   AMT_WriteReg(AddrBuff[i], DataBuff[i]);
	}
	AMT_WriteReg(BUS_STATUS_ADDR, 0x11);  //I2c Write End
	//delay(1400000);
	OS_Delay (200);

	//AMT_WriteReg(0xFAC6, 0x20);

	RETAILMSG((TEXT("---ConfigSlaveMode!\r\n")));
}

/****************************************************************************
*name:   AMT_WriteStaticPara(PanlstaticPara * dataPt,UINT num)
*input:              
*output:   

*description:配置显示模式参数。
*****************************************************************************/
void AMT_WriteStaticPara(PanlstaticPara * dataPt,UINT num)
{
	int ucLoop;
	UINT8 ucDeviceAddr;
	UINT8 uctmpDeviceAddr;
	UINT8 ucSubAddr;
	UINT8 ucRegVal;
		
	 while(num--)
	 {

     ucLoop = I2C_ACCESS_LOOP_TIME;
	 uctmpDeviceAddr = (unsigned char)(((*dataPt).addr>>8)&0XFF);
	 ucSubAddr = (unsigned char)(((*dataPt).addr)&0XFF);
	 ucRegVal = (*dataPt).dat;

		 	
		 switch(uctmpDeviceAddr)
		 {
		    case 0XF9:
		    case 0XFD:
				 ucDeviceAddr= 0XB0;
				 break;

			case 0XFA:
				 ucDeviceAddr= 0XBE;
				 break;
				 	
			case 0XFB:
				 ucDeviceAddr= 0XB6;
				 break;

		    case 0XFC:
				 ucDeviceAddr= 0XB8;
				 break;

			case 0XFE:
				 ucDeviceAddr= 0XB2;
				 break;

			case 0XFF:
				 ucDeviceAddr= 0XB4;
				 break;

		    case 0X00:
			     ucDeviceAddr = 0XBE;
			     break;
			
			default:
				 ucDeviceAddr= 0XB0;
				 break;			
		 }
		 while(ucLoop--)
		 {
			if(ark7116_i2c_write_bytes (ucDeviceAddr, &ucSubAddr, 1, (char *)&ucRegVal, 1) == 0)
				break;			 
		 }
		 dataPt++;
	 }
	
}
/****************************************************************************
*name:   AMT_WriteDispZoomDynPara(PanlSysDynPara * dataPt,UINT num,UCHAR currentSys)
*input:              
*output:   

*description:配置显示模式参数。
*****************************************************************************/
void AMT_WriteDispZoomDynPara(PanlPosDynPara * dataPt,UINT num,UCHAR currentmode)
{

     int ucLoop;
	 UINT8 uctmpDeviceAddr;
     UINT8 ucDeviceAddr;	 
     UINT8 ucSubAddr;
     UINT8 ucRegVal;

	 while(num--)
	 {


        ucLoop = I2C_ACCESS_LOOP_TIME;
		 uctmpDeviceAddr = (unsigned char)((((*dataPt).addr)>>8)&0XFF);
		 ucSubAddr = (unsigned char)(((*dataPt).addr)&0XFF);
		 ucRegVal =	(*dataPt).dat_posDyn[currentmode];
		 switch(uctmpDeviceAddr)
		 {
		    case 0XF9:
		    case 0XFD:
				 ucDeviceAddr= 0XB0;
				 break;

			case 0XFA:
				 ucDeviceAddr= 0XBE;
				 break;
				 	
			case 0XFB:
				 ucDeviceAddr= 0XB6;
				 break;

		    case 0XFC:
				 ucDeviceAddr= 0XB8;
				 break;

			case 0XFE:
				 ucDeviceAddr= 0XB2;
				 break;

			case 0XFF:
				 ucDeviceAddr= 0XB4;
				 break;
				 
            case 0X00:
			     ucDeviceAddr = 0XBE;
			     break;
				 
			default:
				 ucDeviceAddr= 0XB0;
				 break;			
		 }
		 
		 while(ucLoop--)
	 	 {
			if(ark7116_i2c_write_bytes (ucDeviceAddr, &ucSubAddr, 1, (char *)&ucRegVal, 1) == 0)
				break;
	 	 }
		 dataPt++;
	 }
}
	 
/****************************************************************************
*name:   AMT_WriteColorSysDynPara(PanlSysDynPara * dataPt,UINT num,UCHAR currentSys)
*input:              
*output:   

*description:配置图像制式参数
*****************************************************************************/
void AMT_WriteColorSysDynPara(PanlSysDynPara * dataPt,UINT num,UCHAR currentSys)
{
     int ucLoop;
     UINT8 ucDeviceAddr;
	 UINT8 uctmpDeviceAddr;
     UINT8 ucSubAddr;
     UINT8 ucRegVal;

	 while(num--)
	 {
       ucLoop = I2C_ACCESS_LOOP_TIME;
		 uctmpDeviceAddr = (unsigned char)((((*dataPt).addr)>>8)&0XFF);
		 ucSubAddr = (unsigned char)(((*dataPt).addr)&0XFF);
		 ucRegVal = (*dataPt).dat_sysDyn[currentSys];
//	RETAILMSG((TEXT("\r\n ColorSysDynPara Info.wrdata =0x%x,Info.reg =0x%x !\r\n"),Ark7116Info.wrdata,Ark7116Info.reg));
		
		 	
		 switch(uctmpDeviceAddr)
		 {
		    case 0XF9:
		    case 0XFD:
				 ucDeviceAddr= 0XB0;
				 break;

			case 0XFA:
				 ucDeviceAddr= 0XBE;
				 break;
				 	
			case 0XFB:
				 ucDeviceAddr= 0XB6;
				 break;

		    case 0XFC:
				 ucDeviceAddr= 0XB8;
				 break;

			case 0XFE:
				 ucDeviceAddr= 0XB2;
				 break;

			case 0XFF:
				 ucDeviceAddr= 0XB4;
				 break;

			case 0X00:
			     ucDeviceAddr = 0XBE;
			     break;
				 
			default:
				 ucDeviceAddr= 0XB0;
				 break;			
		 }
		 
		 while(ucLoop--)
		 {
			if(ark7116_i2c_write_bytes (ucDeviceAddr, &ucSubAddr, 1, (char *)&ucRegVal, 1) == 0)
				break;
	
		 }
		 dataPt++;
	 }
}


/***********************************************************
*name:    ConfigStaticPara() 
*input:     CurretSource
*output:    non
*Update:    2011-11-18
*state:     allright

*description:  
         这个函数是配置不同通道的静态参数。      

************************************************************/
void ConfigStaticPara(UCHAR CurretSource)
{    
	 RETAILMSG((TEXT("ConfigStaticPara! \r\n")));
	 
 //    CurretSource = CurretSource;	
	 AMT_WriteStaticPara(VideoChannelPara[INPUT_AV1].VideoPara.pVideoStaicPara,STATIC_NUM);
}


/***********************************************************
*name:    ConfigPadMuxPara() 
*input:   non
*output:  non
*Update:  2011-11-18
*state:   allright

*description:  
         这个函数是配置不同通道的静态参数。      

************************************************************/
void ConfigPadMuxPara(void)
{   
	 RETAILMSG((TEXT("ConfigPadMuxPara! \r\n")));
	 
	AMT_WriteStaticPara(AMT_PadMuxStaticPara,PAD_MUX_NUM);
}


/***********************************************************
*name:     	ConfigColorSysDynPara(UCHAR currentSys) 
*input:     currentSys
*output:    non
*update:    2011-11-18
*state:     allright

*description:  
         这个函数是实时配置不同图像制式参数。      

************************************************************/
void ConfigColorSysDynPara(UCHAR currentSys) 
{    
	 RETAILMSG((TEXT("ConfigColorSysDynPara! \r\n")));
	 
	 AMT_WriteColorSysDynPara(VideoChannelPara[INPUT_AV1].VideoPara.pVideoSysDynPara,SYS_DYN_NUM,currentSys);
//   g_ucbrightness = AMT_ReadReg(BRIGHT_REG);
//	 g_ucContrast = AMT_ReadReg(CONTRAST_REG);
//	 g_ucSaturation = AMT_ReadReg(SATURATION_REG);
}

/***********************************************************
*name:     	ConfigDispZoomDynPara(UCHAR currentmode) 
*input:     currentmode
*output:    non
*update:    2011-11-18
*state:     allright

*description:  
         这个函数是配置不同通道16:9/4:3显示的参数。      

************************************************************/
void ConfigDispZoomDynPara(UCHAR currentmode) 
{    
	 RETAILMSG((TEXT("ConfigDispZoomDynPara! \r\n")));
	  
	 AMT_WriteDispZoomDynPara(VideoChannelPara[INPUT_AV1].VideoPara.pVideoPosDynPara,POS_DYN_NUM,currentmode);
}


/****************************************************************************
*name:   InitiaGlobalClk(void)
*input:  void              
*output: void  

*description:
      ARK 初始化模块时钟。

*****************************************************************************/
void InitGlobalPara()
{	
	RETAILMSG((TEXT("InitGlobalPara! \r\n")));

	AMT_WriteReg(ENH_PLL,0X20);
	ConfigStaticPara(INPUT_AV1); 
	ConfigDispZoomDynPara(DISP_16_9);
	ConfigColorSysDynPara(PAL);
	ConfigPadMuxPara(); 
	AMT_WriteReg(ENH_PLL,0X2C);
}

static void Ark7116Reset()
{
	// GPIO31/PWM1
	// pad_ctl3
	// [5:3]	itu_c_vsync_in	itu_c_vsync_in_pad	GPIO31	itu_c_vsync_in	mac_mdio	pwm_out[1]
	unsigned int val;
	XM_lock();
	val = rSYS_PAD_CTRL03;
	val &= ~(0x07 << 3);
	rSYS_PAD_CTRL03 = val;
	SetGPIOPadDirection (GPIO31, euOutputPad);
	XM_unlock();
	
	XM_lock ();	
	SetGPIOPadData (GPIO31, euDataLow);
	XM_unlock();	
	OS_Delay (10);
	XM_lock ();	
	SetGPIOPadData (GPIO31, euDataHigh);
	XM_unlock();	
	OS_Delay (10);
	XM_lock ();	
	SetGPIOPadData (GPIO31, euDataLow);
	XM_unlock();	
	
	/*
	//Reset ARK7116
	API_GPIO_init();
	API_GPIO_SetOutput(GPIO_S_0, 1);
	busy_delay(100000);
	API_GPIO_SetOutput(GPIO_S_0, 0);
	busy_delay(100000);
	API_GPIO_exit();
	*/
}

void InitArk7116(void)
{     
	RETAILMSG((TEXT("++InitChip! \r\n")));
	Ark7116Reset();
	ConfigSlaveMode();	
	InitGlobalPara();
	RETAILMSG((TEXT("---InitChip! \r\n")));	
}

static BOOL ARK7116_Device_Init(void)
{
    InitArk7116();
    RETAILMSG((TEXT("Ark7116Init--\r\n")));
	
    return TRUE;
}
static void ARK7116_signal(void)
{
    u8_t add = 0x26;
	u8_t dat;
	static u8_t presignal;
	ark7116_i2c_read_bytes (0xB2, &add, 1, (char *)&dat, 1);
       ITU656_in=((dat&0x02)>>1);
	if(presignal != ((dat&0x02)>>1))
	{
       presignal=((dat&0x02)>>1);
	   if(presignal==1)
	   {
	        XMSYS_VideoSetImageAssemblyMode (XMSYS_ASSEMBLY_MODE_FRONT_REAL);
	   }
	   else
	   {
            XMSYS_VideoSetImageAssemblyMode (XMSYS_ASSEMBLY_MODE_FRONT_ONLY);
	   }
	}
	
}
static void ARK7116_task (void)
{
	OS_EnterRegion();
	ARK7116_Device_Init ();
	OS_LeaveRegion();
	
	u8_t add = 0x0a;
	u8_t dat;
	ark7116_i2c_read_bytes (0xB0, &add, 1, (char *)&dat, 1);
	printf ("dat=0x%x\n", dat);
	add = 0x0d;
	ark7116_i2c_read_bytes (0xB0, &add, 1, (char *)&dat, 1);
	printf ("dat=0x%x\n", dat);
	
	while(1)
	{
	     #if 0
		OS_U8 ark7116_event;
		
		ark7116_event = OS_WaitEvent (ARK7116_CONFIG_EVENT);
		
		if(ark7116_event & (ARK7116_CONFIG_EVENT) )
		{
			
		}
		#endif
		ARK7116_signal();
		OS_Delay (200);
	}	
}

void XMSYS_Ark7116Init (void)
{
	GPIO_I2C_Init ();
	OS_CREATETASK(&TCB_ARK7116_Task, "ARK7116", ARK7116_task, XMSYS_ARK7116_TASK_PRIORITY, StackARK7116Task);
}

void XMSYS_Ark7116Exit (void)
{
}


#else

void ark7116_init (void)
{
	
}
void ark7116_exit (void)
{
}

#endif
