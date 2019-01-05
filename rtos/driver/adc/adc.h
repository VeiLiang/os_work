/*
 * ADC.h
 *
 *  Created on: 2010-10-15
 *      Author: Administrator
 */

#ifndef ADC_H_
#define ADC_H_


//#define  TOUCHPAN

#define  BAT_CHANNEL           		1
#define  TOUCHPAN_CHANNEL      	2
#define  AUX0_CHANNEL          		3
#define  AUX1_CHANNEL          		4
#define  AUX2_CHANNEL          		5
#define  AUX3_CHANNEL          		6

#define  AUX0_START_INT       		(1<<0)
#define  AUX0_STOP_INT        		(1<<1)
#define  AUX0_VALUE_INT       		(1<<2)

#define  AUX1_START_INT       		(1<<3)
#define  AUX1_STOP_INT        		(1<<4)
#define  AUX1_VALUE_INT       		(1<<5)

#define  AUX2_START_INT       		(1<<6)
#define  AUX2_STOP_INT        		(1<<7)
#define  AUX2_VALUE_INT       		(1<<8)

#define  AUX3_START_INT       		(1<<9)
#define  AUX3_STOP_INT        		(1<<10)
#define  AUX3_VALUE_INT       		(1<<11)

#define	AUX_START_INT(ch)				(1 << ((ch)*3 + 0))
#define	AUX_STOP_INT(ch)       		(1 << ((ch)*3 + 1))
#define  AUX_VALUE_INT(ch)   			(1 << ((ch)*3 + 2))

#define  TOUCHPAN_START_INT   	(1<<12)
#define  TOUCHPAN_STOP_INT    	(1<<13)
#define  TOUCHPAN_VALUE_INT   	(1<<14)

#define  TOUCHPAN_BAT_INT     	(1<<15)


#define ADCKEY_NULL				0xfffff

typedef struct
{
	UINT16 adcX;
	UINT16 adcY;
} PAN_DATA;

enum {
	XM_ADC_AUX0 = 0,		// BAT
	XM_ADC_AUX1,			// KEY
	XM_ADC_COUNT
};


void ADC_init(void);

UINT32 GetADCKey(void);
void Enable_ADC_Channel(UINT32 channel);
void Disable_ADC_Channel(UINT32 channel);

void ADC_Test(void);

#ifdef TOUCHPAN
UINT32 GetADCTouch(PAN_DATA *pPan);
#endif


#endif /* ADC_H_ */
