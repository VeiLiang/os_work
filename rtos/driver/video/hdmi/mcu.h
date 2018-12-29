///*****************************************
//  Copyright (C) 2009-2014
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <mcu.h>
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2014/01/07
//   @fileversion: ITE_HDMITX_SAMPLE_3.17
//******************************************/

#ifndef _MCU_H_
#define _MCU_H_

#include <stdio.h>
#include <stdlib.h>


/***************************************/
/* DEBUG INFO define                   */
/**************************************/
//#define Build_LIB
//#define MODE_RS232



/*************************************/
/*Port Using Define                  */
/*************************************/

#define _1PORT_



/************************************/
/* Function DEfine                  */
/***********************************/


#define       _HBR_I2S_


///////////////////////////////////////////////////////////////////////////////
// Include file
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
//#include "Reg_c51.h"
///////////////////////////////////////////////////////////////////////////////
// Type Definition
///////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
#define FALSE 0
#define TRUE 1

#define SUCCESS 0
#define FAIL -1

#define ON 1
#define OFF 0

#define LO_ACTIVE TRUE
#define HI_ACTIVE FALSE

#ifndef NULL
#define NULL 0
#endif
///////////////////////////////////////////////////////////////////////////////
// Constant Definition
///////////////////////////////////////////////////////////////////////////////
#define TX0DEV            0
    #define TX0ADR    0x98
#define TX0CECADR     0x9C

#define RXADR            0x90

#define EDID_ADR        0xA0    // alex 070321

#define DELAY_TIME        1        // unit=1 us;
#define IDLE_TIME        100        // unit=1 ms;

#define HIGH            1
#define LOW                0

#ifdef _HPDMOS_
    #define HPDON        0
    #define HPDOFF        1
#else
    #define HPDON        1
    #define HPDOFF        0
#endif


///////////////////////////////////////////////////////////////////////////////
// 8051 Definition
///////////////////////////////////////////////////////////////////////////////

#ifdef _ENCEC_
    #define CECDEV    0
#endif




#endif    // _MCU_H_
