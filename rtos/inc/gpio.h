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
 * 1.0 �궨����
 */
//�ɸ���GPIO �˿ڱ��
//�ɸ���GPIO �˿�0-15 ֻ��������ã����������룬����Ҫ
//��ʹ��ר��GPIO �˿�
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

//ר��GPIO �˿ڱ��
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
 * 2.0 ���ݽṹ������
 *
********************************************************************************/

typedef enum
{
	euDataLow = 0,		//�͵�ƽ
	euDataHigh,			//�ߵ�ƽ
	euDataNone,
}EU_GPIO_Data;

typedef enum
{
	euLowLevelTrig = 0,	//�͵�ƽ�����ж�
	euHightLevelTrig,	//�ߵ�ƽ�����ж�
}EU_TRIG_LEVEL;

typedef enum
{
	euInputPad = 0,	//�����
	euOutputPad,	//�����
	euNoneSettingPad,
}EU_GPIO_Direction;

/********************************************************************************
 * 3.0 ����������
 *
********************************************************************************/



/********************************************************************************
 �������ܣ�����GPIO �˿�ȥ��ʱ�䳤��
 ���������   
 	ulGPIO_Pad: 		GPIO �˿ں�
 	debance_ms:		ȥ��ʱ�䳤�ȣ��Ժ���Ϊ��λ
 ����ֵ:
 	0:		�����ɹ�
 	-1:		ȥ��ʱ�䳤��̫�����Ĵ���ֵ���
 	-2:		GPIO �˿ڲ���ȷ����ָ���˿ڲ��߱�ȥ�����ܣ�ֻ
 			��GPIO_S_0-GPIO_S_7 ֮��İ˸�GPIO Pad ����ȥ������

 ����ʹ��ע������:
	ȥ��ʱ���������:
		(1/32.768k) *  debance_count = debance_ms

	����100ms debance����:
		SetGpioDebounce(GPIO_S_xx, 100)
		(1/33)*debance_count = 100 => debance_count = 3200 

	����125msdebance����:
		SetGpioDebounce(GPIO_S_xx, 125)
		(1/33)*debance_count = 125 => debance_count = 4125 
 *******************************************************************************/
INT32   SetGpioDebounce(UINT32 ulGPIO_Pad,UINT32 debance_ms);

/********************************************************************************
 �������ܣ���ʼ��GPIO ģ��
 ���������   
	��
 ����ֵ: 
 	��
 *******************************************************************************/
 void InitGPIO(void);

/********************************************************************************
 �������ܣ�ע��GPIO �˿��жϴ�����
 ���������   
 	ulGPIO_Pad: 		GPIO �˿ں�
 	euLevel:			GPIO �жϴ�����ƽ
 	fn_irq_handler:	GPIO �˿��ж���Ӧ����
 ����ֵ:
 	0:		�����ɹ�
 	-1:		GPIO0-GPIO15������Ϊ����ţ���˲�����Ӧ�ж�
 	-2:		��GPIO �˿ںŲ�����
 	-3:		�жϴ�����ƽ����ȷ
 	-4:		�жϴ���������Ϊ��
 *******************************************************************************/
INT32 GPIO_IRQ_Request(UINT32 ulGPIO_Pad, EU_TRIG_LEVEL euLevel, void(*fn_irq_handler)(void));

/******************************************************************************** 
 �������ܣ���GPIO ����Ӧ�ж�
 ���������   
 	ulGPIO_Pad: 		GPIO �˿ں�
 ����ֵ:
 	0:		�����ɹ�
 	-1:		GPIO0-GPIO15������Ϊ����ţ���˲�����Ӧ�ж�
 	-2:		��GPIO �˿ںŲ�����
 *******************************************************************************/
INT32 EnableGPIOPadIRQ(UINT32 ulGPIO_Pad);

/********************************************************************************
 �������ܣ���ֹGPIO ����Ӧ�ж�
 ������   
 	ulGPIO_Pad: 		GPIO �˿ں�
 ����ֵ:
 	0:		�����ɹ�
 	-1:		GPIO0-GPIO15������Ϊ����ţ���˲�����Ӧ�ж�
 	-2:		��GPIO �˿ںŲ�����
 *******************************************************************************/
INT32 DisableGPIOPadIRQ(UINT32 ulGPIO_Pad);

/********************************************************************************
 �������ܣ�����GPIO �˿������������
 ���������   
 	ulGPIO_Pad:	GPIO �˿ں�
 	euDirection:	0���룬1���
 ����ֵ��
  	0: ���óɹ�
  	-1: 	GPIO0-GPIO15������Ϊ�����
 	-2: 	GPIO �˿ںŲ�����
 	-3: �ܽ�������������������ȷ
*******************************************************************************/
INT32 SetGPIOPadDirection(UINT32 ulGPIO_Pad, EU_GPIO_Direction euDirection);

/********************************************************************************
 �������ܣ��������ùܽ����״̬
 ���������    ulGPIO_Pad IO���
 			euData �ܽ������ƽ�ߵ�
 ����ֵ��
 	0: ���óɹ�
 	-2: GPIO �ܽű�Ų���ȷ
 	-3: �ܽ������ƽ��������ȷ
********************************************************************************/
INT32 SetGPIOPadData(UINT32 ulGPIO_Pad, EU_GPIO_Data euData);

/********************************************************************************
 �������ܣ����ڻ�ȡ�ܽ�����״̬
 ��������� 
	ulGPIO_Pad:	GPIO �˿ں�
 ����ֵ��
	��GPIO �˿ںŲ���ȷ����euDataNone�����򷵻���ȷ�ĵ�ƽ
 *******************************************************************************/
EU_GPIO_Data GetGPIOPadData(UINT32 ulGPIO_Pad);

/********************************************************************************
 �������ܣ���ȡָ��GPIO �˿������������
 ����:
 	ulGPIO_Pad: GPIO �˿ڱ��
 ����ֵ:
	��GPIO �˿ںŲ���ȷ����euNoneSettingPad�����򷵻��������
	����
 ********************************************************************************/
EU_GPIO_Direction GetGPIODataDirection(UINT32 ulGPIO_Pad);


/********************************************************************************
 �������ܣ�����ָ��GPIO �˿ڵĸ��ý�
 ����:
 	ulGPIO_Pad: GPIO �˿ڱ��
 ����ֵ:
	��
 ����ʹ��ע������:
 	����GPIO85 ���ӵ�bias_fb_p_pad�ţ�ֻ�������
 	GPIO86���ӵ�bias_ovp_p_pad�ţ�ֻ�������
 	GPIO0-GPIO15ֻ�������������������
 ********************************************************************************/
void Select_GPIO_Pad(UINT32 ulGPIO_Pad);

/********************************************************************************
 �������ܣ�����ָ��GPIO �˿ڵ������ƽ
 ����:
 	ulGPIO_Pad: GPIO �˿ڱ��
 	IOstate_high_Low: �����ƽ
 ����ֵ:
  	0: ���óɹ�
  	-1: 	GPIO0-GPIO15������Ϊ�����
 	-2: 	GPIO �˿ںŲ�����
  	-3: �ܽ������ƽ��������ȷ
 ********************************************************************************/
INT32 Set_GPIO_Output(UINT32  GpioNum, EU_GPIO_Data IOstate_high_Low);

/********************************************************************************
 �������ܣ�����ָ��GPIO �˿�Ϊ����˿�
 ����:
 	ulGPIO_Pad: GPIO �˿ڱ��
 	IOstate_high_Low: �����ƽ
 ����ֵ:
  	0: ���óɹ�
  	-1: 	GPIO85, GPIO86, GPIO0-GPIO15������Ϊ�����
 	-2: 	GPIO �˿ںŲ�����
  	-3: �ܽ������ƽ��������ȷ

  ����ʹ��ע������:
  	����GPIO85, GPIO86, GPIO0-GPIO15��ֻ��������ã���˵���	���
  	����������������Ϊ����Ž���Ч
 ********************************************************************************/
INT32 Set_GPIO_Input(UINT32  GpioNum);

#ifdef __cplusplus
}
#endif

#endif


