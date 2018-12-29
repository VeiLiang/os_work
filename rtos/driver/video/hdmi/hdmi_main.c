///*****************************************
//  Copyright (C) 2009-2014
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <Main.c>
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2014/01/07
//   @fileversion: ITE_HDMITX_SAMPLE_3.17
//******************************************/

// ver 1.0.0: release demo code,Mingchih Lung
// 2007/11/19: Partition Spliter.c to Cat6023.c,Cat6613.c,EDID.c,HDCP.c,IO.c,Utility.c and Spliter.c
// 2007/11/20: Modify default EDID to match CEA specification
// 2008/01/29: add HDCP_ROM_SEL for two type of 6613 PKG
// 2008/02/13: Increase a DDC abort command     for BENQ monitor
// 2008/02/14: fix A2 version HDCP bugs. add RX singal port define
// 2008/02/29: remove HDCP_ROM_SEL,new 6613 ROM detect.Fix audio muilty channel bug
#include "typedef.h"
#include "Mcu.h"
#include "IO.h"
#include "version.h"
#include "hdmitx_sys.h"
#include <stdio.h>

void CHANGE_INPUT();

#define IDM_HDMI_640x480p60  0
#define IDM_HDMI_480p60      1
#define IDM_HDMI_480p60_16x9 2
#define IDM_HDMI_720p60      3
#define IDM_HDMI_1080i60     4
#define IDM_HDMI_480i60      5
#define IDM_HDMI_480i60_16x9 6
#define IDM_HDMI_1080p60     7
#define IDM_HDMI_576p50      8
#define IDM_HDMI_576p50_16x9 9
#define IDM_HDMI_720p50      10
#define IDM_HDMI_1080i50     11
#define IDM_HDMI_576i50      12
#define IDM_HDMI_576i50_16x9 13
#define IDM_HDMI_1080p50     14
#define IDM_HDMI_1080p24     15
#define IDM_HDMI_1080p25     16
#define IDM_HDMI_1080p30     17

void delay1ms (unsigned int ms)
{
	OS_Delay (ms);
}

void CHANGE_INPUT()
{
    static int input_opt = 0 ;
 //   if( P1_6 == 1 ) return ;
//    while(P1_6 == 0 )
    {
        printf("Wait for input change\\\r") ;
        printf("Wait for input change|\r") ;
        printf("Wait for input change/\r") ;
        printf("Wait for input change-\r") ;
    }
    input_opt %= 18 ;
    switch(input_opt)
    {
        HDMITX_ChangeDisplayOption(HDMI_640x480p60,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_640x480p60,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_480p60 :
        HDMITX_ChangeDisplayOption(HDMI_480p60,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_480p60,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_480p60_16x9 :
        HDMITX_ChangeDisplayOption(HDMI_480p60_16x9,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_480p60_16x9,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_720p60 :
        HDMITX_ChangeDisplayOption(HDMI_720p60,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_720p60,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_1080i60 :
        HDMITX_ChangeDisplayOption(HDMI_1080i60,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_1080i60,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_480i60 :
        HDMITX_ChangeDisplayOption(HDMI_480i60,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_480i60,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_480i60_16x9 :
        HDMITX_ChangeDisplayOption(HDMI_480i60_16x9,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_480i60_16x9,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_1080p60 :
        HDMITX_ChangeDisplayOption(HDMI_1080p60,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_1080p60,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_576p50 :
        HDMITX_ChangeDisplayOption(HDMI_576p50,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_576p50,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_576p50_16x9 :
        HDMITX_ChangeDisplayOption(HDMI_576p50_16x9,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_576p50_16x9,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_720p50 :
        HDMITX_ChangeDisplayOption(HDMI_720p50,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_720p50,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_1080i50 :
        HDMITX_ChangeDisplayOption(HDMI_1080i50,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_1080i50,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_576i50 :
        HDMITX_ChangeDisplayOption(HDMI_576i50,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_576i50,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_576i50_16x9 :
        HDMITX_ChangeDisplayOption(HDMI_576i50_16x9,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_576i50_16x9,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_1080p50 :
        HDMITX_ChangeDisplayOption(HDMI_1080p50,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_1080p50,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_1080p24 :
        HDMITX_ChangeDisplayOption(HDMI_1080p24,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_1080p24,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_1080p25 :
        HDMITX_ChangeDisplayOption(HDMI_1080p25,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_1080p25,HDMI_RGB444) ;\n") ;
        break ;
    case IDM_HDMI_1080p30 :
        HDMITX_ChangeDisplayOption(HDMI_1080p30,HDMI_RGB444) ;
        printf("HDMITX_ChangeDisplayOption(HDMI_1080p30,HDMI_RGB444) ;\n") ;
        break ;
    }
    input_opt++ ;
    printf("\n") ;
}


extern void hdmi_i2c_init (void);
extern void hdmi_i2c_exit (void);
#define GPIO_INT								24
#define NANDFLASH_INT						8

void hdmi_main( void )
{
    BYTE count=0;
	 
	 printf ("enter hdmi setup\n");
	 
    printf( "\n" VERSION_STRING "\n");
	// init_test_video_input() ;
	 hdmi_i2c_init();

//////////////////////////////////////////////////
    InitHDMITX_Variable();
    InitHDMITX();
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HDMITX_ChangeDisplayOption(HDMI_720p60,HDMI_RGB444) ;
    while(1)
    {
    #ifdef SUPPORT_UART_CMD
        UartCommand();
    #endif
        //HoldSystem() ;
        //CHANGE_INPUT() ;

		  OS_Delay (100);
        //if(IsTimeOut(100))
        {
            HDMITX_DevLoopProc();
            count ++ ;
            printf("count = %d\n",(int)count) ;
            if( count > 50 )
            {
                DumpHDMITXReg() ;
                count = 0 ;
					 break;
            }
        }
    }
	 
	 hdmi_i2c_exit ();
	 
	 printf ("leave hdmi setup\n");
}

