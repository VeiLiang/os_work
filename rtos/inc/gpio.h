/*
*********************************************************************************************************
*                                           
*                           (c) Copyright 2010-2012,
*
*
*                                  gpio head file
*
* File    : gpio.h   ark1660 soc
* By      : ls
* Version : V1.0.0
* Date    : 2012.02.07
*********************************************************************************************************
*                                             History
*
*
*/
#ifndef _GPIO_H
#define _GPIO_H
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************************
 * 1.0 宏定义区
 */
//可复用GPIO 端口编号
//可复用GPIO 端口0-15 只能做输出用，不能做输入，若需要
//请使用专用GPIO 端口
#define GPIO0		0
#define GPIO1		1
#define GPIO2		2
#define GPIO3		3
#define GPIO4		4
#define GPIO5		5
#define GPIO6		6
#define GPIO7		7
#define GPIO8		8
#define GPIO9		9
#define GPIO10		10
#define GPIO11		11
#define GPIO12		12
#define GPIO13		13
#define GPIO14		14
#define GPIO15		15
#define GPIO16		16
#define GPIO17		17
#define GPIO18		18
#define GPIO19		19
#define GPIO20		20
#define GPIO21		21
#define GPIO22		22
#define GPIO23		23
#define GPIO24		24
#define GPIO25		25
#define GPIO26		26
#define GPIO27		27
#define GPIO28		28
#define GPIO29		29
#define GPIO30		30
#define GPIO31		31

#define GPIO32		32
#define GPIO33		33
#define GPIO34		34
#define GPIO35		35
#define GPIO36		36
#define GPIO37		37
#define GPIO38		38
#define GPIO39		39
#define GPIO40		40
#define GPIO41		41
#define GPIO42		42
#define GPIO43		43
#define GPIO44		44
#define GPIO45		45
#define GPIO46		46
#define GPIO47		47
#define GPIO48		48
#define GPIO49		49
#define GPIO50		50
#define GPIO51		51
#define GPIO52		52
#define GPIO53		53
#define GPIO54		54
#define GPIO55		55
#define GPIO56		56
#define GPIO57		57
#define GPIO58		58
#define GPIO59		59
#define GPIO60		60
#define GPIO61		61
#define GPIO62		62
#define GPIO63		63

#define GPIO64		64
#define GPIO65		65
#define GPIO66		66
#define GPIO67		67
#define GPIO68		68
#define GPIO69		69
#define GPIO70		70
#define GPIO71		71
#define GPIO72		72
#define GPIO73		73
#define GPIO74		74
#define GPIO75		75
#define GPIO76		76
#define GPIO77		77
#define GPIO78		78
#define GPIO79		79
#define GPIO80		80
#define GPIO81		81
#define GPIO82		82
#define GPIO83		83
#define GPIO84		84
#define GPIO85		85
#define GPIO86		86
#define GPIO87		87
#define GPIO88		88
#define GPIO89		89
#define GPIO90		90
#define GPIO91		91
#define GPIO92		92
#define GPIO93		93
#define GPIO94		94
#define GPIO95		95

#define GPIO96		96
#define GPIO97		97
#define GPIO98		98
#define GPIO99		99
#define GPIO100		100
#define GPIO101		101
#define GPIO102		102
#define GPIO103		103
#define GPIO104		104
#define GPIO105		105
#define GPIO106		106
#define GPIO107		107
#define GPIO108		108
#define GPIO109		109
#define GPIO110		110
#define GPIO111		111
#define GPIO112		112
#define GPIO113		113
#define GPIO114		114
#define GPIO115		115
#define GPIO116		116
#define GPIO117		117
#define GPIO118		118
#define GPIO119		119
#define GPIO120		120
#define GPIO121		121
#define GPIO122		122
#define GPIO123		123
#define GPIO124		124
#define GPIO125		125
#define GPIO126		126
#define GPIO127		127

//专用GPIO 端口编号
#define GPIO_S_0	128
#define GPIO_S_1	129
#define GPIO_S_2	130
#define GPIO_S_3	131
#define GPIO_S_4	132
#define GPIO_S_5	133
#define GPIO_S_6	134
#define GPIO_S_7	135
#define GPIO_S_8	136
#define GPIO_S_9	137
#define GPIO_S_10	138
#define GPIO_S_11	139
#define GPIO_S_12	140
#define GPIO_S_13	141
#define GPIO_S_14	142
#define GPIO_S_15	143

#define MAX_GPIO_NUM 143

#define GPIO_BANK_A_START		0
#define GPIO_BANK_A_END			31
#define GPIO_BANK_B_START		32
#define GPIO_BANK_B_END			63
#define GPIO_BANK_C_START		64
#define GPIO_BANK_C_END			95
#define GPIO_BANK_D_START		96
#define GPIO_BANK_D_END			127
#define GPIO_BANK_S_START		128
#define GPIO_BANK_S_END		143

#define MAX_GPIO_PAD			(GPIO_BANK_S_END+1)

/********************************************************************************
 * 2.0 数据结构定义区
 *
********************************************************************************/

typedef enum
{
	euDataLow = 0,		//低电平
	euDataHigh,			//高电平
	euDataNone,
}EU_GPIO_Data;

typedef enum
{
	euLowLevelTrig = 0,	//低电平触发中断
	euHightLevelTrig,	//高电平触发中断
}EU_TRIG_LEVEL;

typedef enum
{
	euInputPad = 0,	//输入脚
	euOutputPad,	//输出脚
	euNoneSettingPad,
}EU_GPIO_Direction;

/********************************************************************************
 * 3.0 函数声明区
 *
********************************************************************************/



/********************************************************************************
 函数功能：设置GPIO 端口去抖时间长度
 输入参数：   
 	ulGPIO_Pad: 		GPIO 端口号
 	debance_ms:		去抖时间长度，以毫秒为单位
 返回值:
 	0:		操作成功
 	-1:		去抖时间长度太长，寄存器值溢出
 	-2:		GPIO 端口不正确或者指定端口不具备去抖功能，只
 			有GPIO_S_0-GPIO_S_7 之间的八个GPIO Pad 脚有去抖功能

 函数使用注意事项:
	去抖时间计算如下:
		(1/32.768k) *  debance_count = debance_ms

	配置100ms debance举例:
		SetGpioDebounce(GPIO_S_xx, 100)
		(1/33)*debance_count = 100 => debance_count = 3200 

	配置125msdebance举例:
		SetGpioDebounce(GPIO_S_xx, 125)
		(1/33)*debance_count = 125 => debance_count = 4125 
 *******************************************************************************/
INT32   SetGpioDebounce(UINT32 ulGPIO_Pad,UINT32 debance_ms);

/********************************************************************************
 函数功能：初始化GPIO 模块
 输入参数：   
	无
 返回值: 
 	无
 *******************************************************************************/
 void InitGPIO(void);

/********************************************************************************
 函数功能：注册GPIO 端口中断处理函数
 输入参数：   
 	ulGPIO_Pad: 		GPIO 端口号
 	euLevel:			GPIO 中断触发电平
 	fn_irq_handler:	GPIO 端口中断响应函数
 返回值:
 	0:		操作成功
 	-1:		GPIO0-GPIO15不能作为输入脚，因此不会响应中断
 	-2:		该GPIO 端口号不存在
 	-3:		中断触发电平不正确
 	-4:		中断处理函数不能为空
 *******************************************************************************/
INT32 GPIO_IRQ_Request(UINT32 ulGPIO_Pad, EU_TRIG_LEVEL euLevel, void(*fn_irq_handler)(void));

/******************************************************************************** 
 函数功能：打开GPIO 脚响应中断
 输入参数：   
 	ulGPIO_Pad: 		GPIO 端口号
 返回值:
 	0:		操作成功
 	-1:		GPIO0-GPIO15不能作为输入脚，因此不会响应中断
 	-2:		该GPIO 端口号不存在
 *******************************************************************************/
INT32 EnableGPIOPadIRQ(UINT32 ulGPIO_Pad);

/********************************************************************************
 函数功能：禁止GPIO 脚响应中断
 参数：   
 	ulGPIO_Pad: 		GPIO 端口号
 返回值:
 	0:		操作成功
 	-1:		GPIO0-GPIO15不能作为输入脚，因此不会响应中断
 	-2:		该GPIO 端口号不存在
 *******************************************************************************/
INT32 DisableGPIOPadIRQ(UINT32 ulGPIO_Pad);

/********************************************************************************
 函数功能：设置GPIO 端口输入输出方向
 输入参数：   
 	ulGPIO_Pad:	GPIO 端口号
 	euDirection:	0输入，1输出
 返回值：
  	0: 设置成功
  	-1: 	GPIO0-GPIO15不能作为输入脚
 	-2: 	GPIO 端口号不存在
 	-3: 管脚输入输出方向参数不正确
*******************************************************************************/
INT32 SetGPIOPadDirection(UINT32 ulGPIO_Pad, EU_GPIO_Direction euDirection);

/********************************************************************************
 函数功能：用于设置管脚输出状态
 输入参数：    ulGPIO_Pad IO编号
 			euData 管脚输出电平高低
 返回值：
 	0: 设置成功
 	-2: GPIO 管脚编号不正确
 	-3: 管脚输出电平参数不正确
********************************************************************************/
INT32 SetGPIOPadData(UINT32 ulGPIO_Pad, EU_GPIO_Data euData);

/********************************************************************************
 函数功能：用于获取管脚输入状态
 输入参数： 
	ulGPIO_Pad:	GPIO 端口号
 返回值：
	若GPIO 端口号不正确返回euDataNone，否则返回正确的电平
 *******************************************************************************/
EU_GPIO_Data GetGPIOPadData(UINT32 ulGPIO_Pad);

/********************************************************************************
 函数功能：获取指定GPIO 端口输入输出方向
 参数:
 	ulGPIO_Pad: GPIO 端口编号
 返回值:
	若GPIO 端口号不正确返回euNoneSettingPad，否则返回输入输出
	方向
 ********************************************************************************/
EU_GPIO_Direction GetGPIODataDirection(UINT32 ulGPIO_Pad);


/********************************************************************************
 函数功能：设置指定GPIO 端口的复用脚
 参数:
 	ulGPIO_Pad: GPIO 端口编号
 返回值:
	无
 函数使用注意事项:
 	其中GPIO85 连接到bias_fb_p_pad脚，只能做输出
 	GPIO86连接到bias_ovp_p_pad脚，只能做输出
 	GPIO0-GPIO15只能做输出，不能做输入
 ********************************************************************************/
void Select_GPIO_Pad(UINT32 ulGPIO_Pad);

/********************************************************************************
 函数功能：设置指定GPIO 端口的输出电平
 参数:
 	ulGPIO_Pad: GPIO 端口编号
 	IOstate_high_Low: 输出电平
 返回值:
  	0: 设置成功
  	-1: 	GPIO0-GPIO15不能作为输入脚
 	-2: 	GPIO 端口号不存在
  	-3: 管脚输出电平参数不正确
 ********************************************************************************/
INT32 Set_GPIO_Output(UINT32  GpioNum, EU_GPIO_Data IOstate_high_Low);

/********************************************************************************
 函数功能：设置指定GPIO 端口为输入端口
 参数:
 	ulGPIO_Pad: GPIO 端口编号
 	IOstate_high_Low: 输出电平
 返回值:
  	0: 设置成功
  	-1: 	GPIO85, GPIO86, GPIO0-GPIO15不能作为输入脚
 	-2: 	GPIO 端口号不存在
  	-3: 管脚输出电平参数不正确

  函数使用注意事项:
  	其中GPIO85, GPIO86, GPIO0-GPIO15脚只能做输出用，因此调用	这个
  	函数设置这两个脚为输入脚将无效
 ********************************************************************************/
INT32 Set_GPIO_Input(UINT32  GpioNum);

#ifdef __cplusplus
}
#endif

#endif


