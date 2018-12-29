#ifndef __HARDWARE_H
#define __HARDWARE_H

#include <xm_proj_define.h>

/* 
	rTIMER_PRS1 = 150 - 1;
	clk = t18xx_get_clks(TIMER1_CLK);
	rTIMER_MOD1 = (clk/150)/HZ;
*/
//#define HZ			1000  // 478*2 =956  HZ/s
//#define HZ			1010  //=480*2 =960 HZ/s
//#define HZ        1060  //=504*2 =1008 HZ/s
//#define HZ        1050  //=499.94*2 =999.88 HZ/s
//#define HZ        1051  //=499.94*2 =999.88 HZ/s
//#define HZ        1053  //=501.1*2 = 1002.2 HZ/s
//#define HZ        1052  //=501.1*2 = 1002.2 HZ/s
//#define HZ        1100  //=523.1*2 = 1046.2 HZ/s
//#define HZ        1200  //=567.8*2 = 1135.6 HZ/s
//#define HZ        1300  //=612.9*2 = 1225.8 HZ/s
//#define HZ        1400  //=657.4*2 = 1314.8 HZ/s
//#define HZ        1500  //=698.5*2 = 1397 HZ/s
//#define HZ        1600  //=740.1*2 = 1480.2 HZ/s
//#define HZ        1700  //=786.8*2 = 1573.6 HZ/s
//#define HZ        1800  //=825.3*2 = 1650.6 HZ/s
//#define HZ        1900  //=868.3*2 = 1736.6 HZ/s
//#define HZ        2000  //=907.7*2 = 1815.4 HZ/s
/*
1736.6
1650.6	86
1573.6	77
1480.2	93.4
1397	   83.2
1314.8	82.2
1225.8	89
1135.6	90.2
1046.2	89.4
956	   90.2
*/
/* 
	rTIMER_PRS1 = 128 - 1;
	clk = t18xx_get_clks(TIMER1_CLK);
	rTIMER_MOD1 = (clk/128)/HZ;
*/
#define HZ			1000  //  =476*2 =952  HZ/s
//#define HZ			1010  //= *2 =960 HZ/s
//#define HZ        1060  //= *2 =1008 HZ/s
//#define HZ        1050  //=  *2 =999.88 HZ/s
//#define HZ        1051  //=*2 =999.88 HZ/s
//#define HZ        1053  //= *2 = 1002.2 HZ/s
//#define HZ        1052  //= *2 = 1002.2 HZ/s
//#define HZ        1100  //=*2 = 1046.2 HZ/s
//#define HZ        1200  //=*2 = 1135.6 HZ/s
//#define HZ        1300  //=*2 = 1225.8 HZ/s
//#define HZ        1400  //=*2 = 1314.8 HZ/s
//#define HZ        1500  //=696.8*2 = 1393.6 HZ/s
//#define HZ        1600  //=*2 = 1480.2 HZ/s
//#define HZ        1700  //=*2 = 1573.6 HZ/s
//#define HZ        1800  //=*2 = 1650.6 HZ/s
//#define HZ        1900  //=*2 = 1736.6 HZ/s
//#define HZ        2000  //=*2 = 1814. HZ/s


#include "rtos.h"
#include "ark1960.h"
#include "types.h"
#include <xmtype.h>
#include "printk.h"
#include "dma.h"
#include "irqs.h"
#include "mmu.h"

void busy_delay(INT32U time);
void delay(unsigned int time);


#endif	// __HARDWARE_H


