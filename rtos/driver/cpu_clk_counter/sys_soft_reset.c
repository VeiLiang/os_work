/***********************************************************************
*Copyright (c)2012  Arkmicro Technologies Inc. 
*All Rights Reserved 
*
*Filename:    CpuClkCount.c
*Version :    1.0 
*Date    :    2011.12.05
*Author  :    donier 
*Abstract:    ark1660  Ver1.0 MPW driver
*History :     
* 
*Version :    2.0 
*Date    :    2012.02.27
*Author  :    donier
*Abstract:    ark1660  Ver1.0 MPW driver remove waring
*History :    1.0

************************************************************************/
 

#include "hardware.h"
#include "cpuClkCounter.h"
#include "printk.h"

typedef enum
{
	eu_24M,
	eu_cpu_pll,
	eu_sys_pll,
	eu_vid_pll,
}EU_PLL_SOURCE;




UINT32 GetCPUPLLFrequency()
{
	UINT32 val;
	UINT32 ref_clk;
	UINT32 no,nf,enable;
	UINT32 speed;
	
	speed = 0;
	val = rSYS_PLLRFCK_CTL;
	ref_clk = 12000000 * ((val & 0x1));
	
	val = rSYS_CPUPLL_CFG;
	
	no = (val >> 12) & 0x03;
	if(no == 0)
		no = 1;
	else if(no == 1)
		no = 2;
	else if(no == 2)
		no = 4;
	else
		no = 8;
	
	nf = (val & 0xFF)+1;
	
	enable = (val >> 14) & 0x1;

	if(enable)
		speed = ref_clk * nf /no;
	
	//printf ("CPUPLL=%d\n", speed);
	return speed;
}

UINT32 GetSYSPLLFrequency()
{
	UINT32 val;
	UINT32 ref_clk;
	UINT32 no,nf,enable;
	UINT32 speed;
	
	speed = 0;
	val = rSYS_PLLRFCK_CTL;
	ref_clk = 12000000 * (((val>>3) & 0x1));
	
	val = rSYS_SYSPLL_CFG;
	
	no = (val >> 12) & 0x03;
	if(no == 0)
		no = 1;
	else if(no == 1)
		no = 2;
	else if(no == 2)
		no = 4;
	else
		no = 8;
	
	nf = (val & 0xFF)+1;
	
	enable = (val >> 14) & 0x1;

	if(enable)
		speed = ref_clk * nf /no;
	
	//printf ("SYSPLL=%d\n", speed);
	return speed;
}

UINT32 GetAUDPLLFrequency()
{
	UINT32 val;
	UINT32 ref_clk;
	UINT32 no,nf,enable;
	UINT32 speed;
	
	speed = 0;
	val = rSYS_PLLRFCK_CTL;
	ref_clk = 12000000 * (((val>>6)&0x1));
	
	
	val = rSYS_AUDPLL_CFG;
	
	no = (val >> 12) & 0x03;
	if(no == 0)
		no = 1;
	else if(no == 1)
		no = 2;
	else if(no == 2)
		no = 4;
	else
		no = 8;
	
	nf = val & 0xFF;
	
	enable = (val >> 14) & 0x1;

	if(enable)
		speed = ref_clk * nf /no;
	
	//printf ("AUDPLL=%d\n", speed);
	return speed;
}

UINT32 GetEXT24ExcryptFrequency()
{
	return 24000000;
}

UINT32 GetEXTRTCFrequency()
{
	return 32768;
}

UINT32 GetCLK240MFrequency()
{
	return 240000000;
}

UINT32 GetAHB_CLK(void)
{
	UINT32 val;
	UINT32 main_hclk_source;
	UINT32 hclk_div;
	UINT32 main_hclk;
	UINT32 speed;
	
	val = rSYS_CLK_SEL;
	
	// main_hclk_clk_sel
	// SYS_CLK_SEL [17-15]
	// 3b000 : cpupll_clk
	// 3b001 : syspll_clk
	// 3b010 : audpll_clk
	// 3b100 : clk_24m
	main_hclk_source = (val >> 15) & 0x07;
	
	// hclk_div
	// SYS_CLK_SEL [21-19]
	// hclk = main_hclk / (hclk_div + 1)
	hclk_div = (val >> 19) & 0x07;
	hclk_div = hclk_div + 1;
	
	switch(main_hclk_source)
	{
		case 0:
			main_hclk = GetCPUPLLFrequency();
			break;
		case 1:
			main_hclk = GetSYSPLLFrequency();
			break;
		case 2:
			main_hclk = GetAUDPLLFrequency();
			break;
		case 4:
			main_hclk = GetEXT24ExcryptFrequency();
			break;
		default :
			main_hclk = 0;
			break;
	}
	speed = main_hclk / hclk_div;
	
	return speed;
}

UINT32 GetAPB_CLK(void)
{
	UINT32 val;
	UINT32 hclk;
	UINT32 pclk_div;
	UINT32 pclk;
	
	val = rSYS_CLK_SEL;
	// pclk_div
	// SYS_CLK_SEL [24-23]
	// 2'b00: pclk = hclk
	// 2'b01: pclk = hclk/2
	// 2'b10: pclk = hclk/4

	pclk_div = (val >> 23) & 0x3;
	hclk = GetAHB_CLK();
	pclk = hclk / ( 1<< pclk_div);
	
	return pclk;
}

UINT32 GetCPU_CLK(void)
{
	UINT32 val;
	UINT32 main_clk_source;
	UINT32 cpu_clk_div;
	UINT32 main_clk;
	UINT32 cpu_clk;
	
	val = rSYS_CLK_SEL;
	
	// main_cpu_clk_sel
	// SYS_CLK_SEL [27-25]
	// 3'b000: cpupll_clk
	// 3'b001: rtc_clk
	// 3'b010: syspll_clk
	// 3'b100: clk_24m
	main_clk_source = (val >> 25) & 0x07;
	if(main_clk_source == 0)
		main_clk = GetCPUPLLFrequency();
	else if(main_clk_source == 1)
		main_clk = GetEXTRTCFrequency();
	else if(main_clk_source == 2)
		main_clk = GetSYSPLLFrequency();
	else if(main_clk_source == 4)
		main_clk = GetEXT24ExcryptFrequency();
	
	// cpu_clk_div
	// SYS_CLK_SEL [30-28]
	// cpu_clk = main_clk / (cpu_clk_div + 1)
	cpu_clk_div = (val >> 28) & 0x07;
	cpu_clk_div = cpu_clk_div + 1;
	
	cpu_clk = main_clk / cpu_clk_div;
	return cpu_clk;
}

UINT32 GetXCLK()
{
	UINT32 val;
	UINT32 xclk_div;
	UINT32 main_xclk_sel;
	UINT32 main_xclk;
	
	// 6-4	R/W	0	Xclk div
	// 					Xclk = main_Xclk/(Xclk_div+1)
	//
	// 2-0	R/W	4	main_xclk_sel
	// 				3'b000: cpupll_clk
	// 				3'b001: syspll_clk
	// 				3'b010: audpll_clk
	// 				3'b100: clk_24m

	val = rSYS_CLK_SEL;
	xclk_div = (val >> 4) & 0x07;
	xclk_div = xclk_div + 1;
	
	main_xclk_sel = val & 0x07;
	if(main_xclk_sel == 0)
		main_xclk = GetCPUPLLFrequency();
	else if(main_xclk_sel == 1)
		main_xclk = GetSYSPLLFrequency();
	else if(main_xclk_sel == 2)
		main_xclk = GetAUDPLLFrequency();
	else if(main_xclk_sel == 4)
		main_xclk = GetEXT24ExcryptFrequency();
	else
		main_xclk = 0;
	
	return main_xclk / xclk_div;
}

UINT32 GetDDRIICLK()
{
	UINT32 val;
	UINT32 ddr2_mclk_sel;
	UINT32 ddr2clk_div;
	UINT32 ddr2_clk;
	
	// 14:12	R/W	0	ddr2clk_div
	// 					ddr2_phyclk = main_ddr2_clk / (ddr2clk_div ? ddr2clk_div : 1)
	//
	// 11-8	R/W	4	main_ddr2_clk_sel
	// 					4b0000 : cpupll_clk
	// 					4b0001 : syspll_clk
	// 					4b0010 : audpll_clk
	// 					4b0100 : clk_24m

	val = rSYS_CLK_SEL;
	
	ddr2_mclk_sel  = (val >> 8) & 0x0F;
	ddr2clk_div = (val >> 12) & 0x07;
	
	if(ddr2clk_div == 0)
	{
		ddr2clk_div = 1;
	}
		
	switch(ddr2_mclk_sel)
	{
		case 0:
			ddr2_mclk_sel = GetCPUPLLFrequency(); 
			break;
		case 1:
			ddr2_mclk_sel = GetSYSPLLFrequency();
			break;
		case 2:
			ddr2_mclk_sel = GetAUDPLLFrequency();
			break;
		case 4:
			ddr2_mclk_sel = GetEXT24ExcryptFrequency();
			break;
		default :
			ddr2_mclk_sel = 0;
			break;
	}
	
	ddr2_clk  = ddr2_mclk_sel / ddr2clk_div;
	return  ddr2_clk;
}


UINT32 arkn141_get_clks (UINT32 clk_id)
{
	switch (clk_id)
	{
		case ARKN141_CLK_CPUPLL:	return GetCPUPLLFrequency();
		case ARKN141_CLK_SYSPLL:	return GetSYSPLLFrequency();
		case ARKN141_CLK_AUDPLL:	return GetAUDPLLFrequency();
		case ARKN141_CLK_24M:		return 24000000;
		case ARKN141_CLK_240M:		return 240000000;
		case ARKN141_CLK_CPU:		return GetCPU_CLK();
		case ARKN141_CLK_AXI:		return GetXCLK();
		case ARKN141_CLK_AHB:		return GetAHB_CLK();
		case ARKN141_CLK_APB:		return GetAPB_CLK();
		case ARKN141_CLK_DDR:		return GetDDRIICLK();
		default:							return 0;
	}
}

void ShowHWFreqInfo(void)
{
	UINT32 clk;
	
	clk = GetCPUPLLFrequency();
	printk("CPU PLL: %d\n", clk);

	clk = GetSYSPLLFrequency();
	printk("SYS PLL: %d\n", clk);

	clk = GetAUDPLLFrequency();
	printk("AUD PLL: %d\n",clk);

	clk = GetCPU_CLK();
	printk("CPU CLK: %d\n", clk);

	clk = GetDDRIICLK();
	printk("DDR CLK: %d\n", clk);

	clk = GetXCLK();
	printk("AXI CLK: %d\n", clk);

	clk = GetAHB_CLK();
	printk("AHB CLK: %d\n", clk);

	clk = GetAPB_CLK();
	printk("APB CLK: %d\n", clk);

}