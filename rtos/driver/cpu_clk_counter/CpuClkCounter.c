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
#include "sys_soft_reset.h"
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
	
	nf = (val & 0xFF);
	
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
	
	nf = (val & 0xFF);
	
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

UINT32 GetISPFrequency (void)
{
	UINT32 val;
	UINT32 Int_ISP_clk_sel;
	UINT32 ISP_clk_div;
	UINT32 isp_clk;
	// 2-0	R/W	4	Int_ISP_clk_sel
	// 					4'b0000: itu_clk_a
	// 					4'b0001: Syspll_clk
	// 					4'b0010: audpll_clk
	// 					4'b0100: Clk_24m
	// 5-3	R/W	0	ISP clk div
	//						ISP clk = int_ISP_clk/div
	val = rSYS_DEVICE_CLK_CFG3;
	Int_ISP_clk_sel = val & 0x07;
	ISP_clk_div = (val >> 3) & 0x07;
	if(ISP_clk_div == 0)
		ISP_clk_div = 1;
	switch (Int_ISP_clk_sel)
	{
		case 0x01:		// Syspll_clk
			Int_ISP_clk_sel = GetSYSPLLFrequency();
			break;
			
		case 0x02:		// audpll_clk
			Int_ISP_clk_sel = GetAUDPLLFrequency();
			break;
			
		case 0x04:		// Clk_24m
			Int_ISP_clk_sel = GetEXT24ExcryptFrequency();
			break;
			
		default:
			Int_ISP_clk_sel = 0;
			break;
	}
	
	isp_clk = Int_ISP_clk_sel / ISP_clk_div;
	return isp_clk;
}

UINT32 GetSensorMCLK (void)
{
	UINT32 val;
	UINT32 sen_clk_switch_sel;
	UINT32 sen_clk_div;
	UINT32 sen_clk;
	// 22-19	R/W	0	sen_clk_div
	// 					sen_clk_int = sen_clk_switch/(sen_clk_div? sen_clk_div:1)
	// 18-15	R/W	4	sen_clk_switch_sel: mfc_clk_switch
	// 					4'b0000: Clk_240m
	//						4'b0001: Syspll_clk
	//						4'b0010: audpll_clk
	//						4'b0100: Clk_24m
	val = rSYS_DEVICE_CLK_CFG1;
	sen_clk_div = (val >> 19) & 0x0F;
	if(sen_clk_div == 0)
		sen_clk_div = 1;
	sen_clk_switch_sel = (val >> 15) & 0x0F;
	switch (sen_clk_switch_sel)
	{
		case 0x00:
			sen_clk_switch_sel = GetCLK240MFrequency();
			break;
		case 0x01:
			sen_clk_switch_sel = GetSYSPLLFrequency();
			break;
		case 0x02:
			sen_clk_switch_sel = GetAUDPLLFrequency();
			break;
		case 0x04:
			sen_clk_switch_sel = GetEXT24ExcryptFrequency();
			break;
		default:
			sen_clk_switch_sel = 0;
			break;
	}
	sen_clk = sen_clk_switch_sel / sen_clk_div;
	return sen_clk;
}

UINT32 GetScalarFrequency (void)
{
	UINT32 val;
	UINT32 Int_scale_clk_sel;
	UINT32 Scale_clk_div;
	UINT32 Scale_clk;
	// SYS_DEVICE_CLK_CFG2
	// 13-11	R/W	0x0	Scale clk div
	//                   Scale clk = int_scale_clk/div
	// 10-8	R/W	0x4	Int_scale_clk_sel
	//                   3'b0000: Clk_240m
	//                   3'b0001: Syspll_clk
	//                   3'b0010: audpll_clk
	//                   3'b0100: Clk_24m
	val = rSYS_DEVICE_CLK_CFG2;
	Int_scale_clk_sel = (val >> 8) & 0x07;
	Scale_clk_div = (val >> 11) & 0x07;
	if(Scale_clk_div == 0)
		Scale_clk_div = 1;
	switch (Int_scale_clk_sel)
	{
		case 0x00:		// Clk_240m
			Int_scale_clk_sel = GetCLK240MFrequency();
			break;
			
		case 0x01:		// Syspll_clk
			Int_scale_clk_sel = GetSYSPLLFrequency();
			break;
			
		case 0x02:		// audpll_clk
			Int_scale_clk_sel = GetAUDPLLFrequency();
			break;

		case 0x04:		// Clk_24m
			Int_scale_clk_sel = GetEXT24ExcryptFrequency();
			break;
			
		default:
			Int_scale_clk_sel = 0;
			break;
	}
	
	Scale_clk = Int_scale_clk_sel / Scale_clk_div;
	return Scale_clk;
}

UINT32 GetIspScalarFrequency (void)
{
	UINT32 val;
	UINT32 Int_scale_clk_sel;
	UINT32 Scale_clk_div;
	UINT32 Scale_clk;
	
	// SYS_device_clk_cfg3
	// 13-11	R/W	0x0	Isp_Scale clk div
	// Isp_Scale clk = int_isp_scale_clk/div
	
	// 10-8	R/W	0x4	Int_isp_scale_clk_sel
	// 4'b0000: Clk_240m
	// 4'b0001: Syspll_clk
	// 4'b0010: audpll_clk
	// 4'b0100: Clk_24m
	
	
	val = rSYS_DEVICE_CLK_CFG3;
	Int_scale_clk_sel = (val >> 8) & 0x07;
	Scale_clk_div = (val >> 11) & 0x07;
	if(Scale_clk_div == 0)
		Scale_clk_div = 1;
	switch (Int_scale_clk_sel)
	{
		case 0x00:		// Clk_240m
			Int_scale_clk_sel = GetCLK240MFrequency();
			break;
			
		case 0x01:		// Syspll_clk
			Int_scale_clk_sel = GetSYSPLLFrequency();
			break;
			
		case 0x02:		// audpll_clk
			Int_scale_clk_sel = GetAUDPLLFrequency();
			break;

		case 0x04:		// Clk_24m
			Int_scale_clk_sel = GetEXT24ExcryptFrequency();
			break;
			
		default:
			Int_scale_clk_sel = 0;
			break;
	}
	
	Scale_clk = Int_scale_clk_sel / Scale_clk_div;
	return Scale_clk;
}

UINT32 GetH264EncoderFrequency (void)
{
	UINT32 val;
	UINT32 Int_mfc_enc_clk_sel;
	UINT32 mfc_enc_clk_div;
	UINT32 mfc_enc_clk;
				
	// 25-23	R/W	0x0	mfc enc clk div
	// 						mfc enc clk = int_mfc_enc_clk/div
				
	// 22-20	R/W	0x4	Int_mfc_enc_clk_sel
	// 						4'b0000: Clk_240m
	//							4'b0001: Syspll_clk
	//							4'b0010: audpll_clk
	//							4'b0100: Cpupll_clk
	
	val = rSYS_DEVICE_CLK_CFG2;
	Int_mfc_enc_clk_sel = (val >> 20) & 0x07;
	mfc_enc_clk_div = (val >> 23) & 0x07;
	if(mfc_enc_clk_div == 0)
		mfc_enc_clk_div = 1;
	switch (Int_mfc_enc_clk_sel)
	{
		case 0x00:		// Clk_240m
			Int_mfc_enc_clk_sel = GetCLK240MFrequency();
			break;
			
		case 0x01:		// Syspll_clk
			Int_mfc_enc_clk_sel = GetSYSPLLFrequency();
			break;
			
		case 0x02:		// audpll_clk
			Int_mfc_enc_clk_sel = GetAUDPLLFrequency();
			break;

		default:
			Int_mfc_enc_clk_sel = 0;
			break;
	}
	
	mfc_enc_clk = Int_mfc_enc_clk_sel / mfc_enc_clk_div;
	return mfc_enc_clk;
}

UINT32 GetH264DecoderFrequency (void)
{
	UINT32 val;
	UINT32 Int_mfc_dec_clk_sel;
	UINT32 mfc_dec_clk_div;
	UINT32 mfc_dec_clk;
				
	// 19-17	R/W	0x0	Mfc_dec_clk div
	// Mfc_dec clk = int_mfc_dec_clk/div
				
	// 16-14	R/W	0x4	Int_mfc_dec_clk_sel
	// 						4'b0000: Clk_240m
	//							4'b0001: Syspll_clk
	//							4'b0010: audpll_clk
	//							4'b0100: Cpupll_clk
	
	val = rSYS_DEVICE_CLK_CFG2;
	Int_mfc_dec_clk_sel = (val >> 14) & 0x07;
	mfc_dec_clk_div = (val >> 17) & 0x07;
	if(mfc_dec_clk_div == 0)
		mfc_dec_clk_div = 1;
	switch (Int_mfc_dec_clk_sel)
	{
		case 0x00:		// Clk_240m
			Int_mfc_dec_clk_sel = GetCLK240MFrequency();
			break;
			
		case 0x01:		// Syspll_clk
			Int_mfc_dec_clk_sel = GetSYSPLLFrequency();
			break;
			
		case 0x02:		// audpll_clk
			Int_mfc_dec_clk_sel = GetAUDPLLFrequency();
			break;

		default:
			Int_mfc_dec_clk_sel = 0;
			break;
	}
	
	mfc_dec_clk = Int_mfc_dec_clk_sel / mfc_dec_clk_div;
	return mfc_dec_clk;
}

UINT32 Get_LCD_CLK_Frequency (void)
{
	unsigned int IntTvClkSwitch_sel;
	unsigned int Tvout_lcd_clk_wire_div;
	unsigned int Tvout_lcd_clk;
	unsigned int Tvout_lcd_pixel_clk;
	unsigned int Tvout_lcd_pixel_clk_wire_div;
	unsigned int TVOUT_LCD_OUT_DLY;
	
	//rSYS_LCD_CLK_CFG =	(TVOUT_LCD_OUT_DLY << 18)
	//						|	(Tvout_lcd_pixel_clk_wire_div << 15)
	//						|	(Tvout_lcd_pixel_clk << 9)
	//						|	(Tvout_lcd_clk << 8)
	//						|	(Tvout_lcd_clk_wire_div << 3)
	//						|	(IntTvClkSwitch_sel << 0)
	//						;
	
	// 2-0	R/W	0x4	IntTvClkSwitch_sel：IntTvClkSwitch
	//							3'b000: clk_240m
	//							3'b001: syspll_clk 
	//							3'b010: audpll_clk
	//							3'b100: clk_24m
	IntTvClkSwitch_sel = rSYS_LCD_CLK_CFG & 0x7;

	// 17-15	R/W	0x4	Tvout_lcd_pixel_clk_wire_div
	//							Tvout_lcd_pixel_clk_wire = tvout_lcd_clk / ( Tvout_lcd_pixel_clk_wire_div ? Tvout_lcd_pixel_clk_wire_div : 1 );
	Tvout_lcd_pixel_clk_wire_div = (rSYS_LCD_CLK_CFG >> 15) & 0x7;
	
	//	7-3	R/W	0x40	Tvout_lcd_clk_wire_div
	//	Tvout_lcd_clk  = IntTvClkSwitch / ( Tvout_lcd_pixel_clk _div ? Tvout_lcd_pixel_clk_div : 1 );
	Tvout_lcd_clk_wire_div = (rSYS_LCD_CLK_CFG >> 3) & 0x1F;
	
	switch(IntTvClkSwitch_sel)
	{
		case 0x00:
			IntTvClkSwitch_sel = GetCLK240MFrequency();
			break;
		case 0x01:
			IntTvClkSwitch_sel = GetSYSPLLFrequency();
			break;
		case 0x02:
			IntTvClkSwitch_sel = GetAUDPLLFrequency();
			break;
		case 0x04:
			IntTvClkSwitch_sel = GetEXT24ExcryptFrequency();
			break;
		default:
			IntTvClkSwitch_sel = 0;
			break;
	}
	
	Tvout_lcd_pixel_clk = IntTvClkSwitch_sel / (Tvout_lcd_pixel_clk_wire_div ? Tvout_lcd_pixel_clk_wire_div : 1);
	Tvout_lcd_clk = Tvout_lcd_pixel_clk / (Tvout_lcd_clk_wire_div ? Tvout_lcd_clk_wire_div : 1);
	return Tvout_lcd_clk;
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
		case ARKN141_CLK_ISP:		return GetISPFrequency();
		case ARKN141_CLK_ISP_SCALAR:	return GetIspScalarFrequency();
		case ARKN141_CLK_SENSOR_MCLK:	return GetSensorMCLK();
		case ARKN141_CLK_SCALAR:	return GetScalarFrequency();
		case ARKN141_CLK_H264_ENCODER:	return GetH264EncoderFrequency();
		case ARKN141_CLK_H264_DECODER:	return GetH264DecoderFrequency();
		case ARKN141_CLK_LCD_CLK:	return Get_LCD_CLK_Frequency();
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

//#define rSYS_SOFT_RSTNA							*((volatile unsigned int *)(SYS_BASE+0x74))
//#define rSYS_SOFT_RSTNB							*((volatile unsigned int *)(SYS_BASE+0x78))
void sys_soft_reset (int reset_dev)
{
	unsigned int mask;
	volatile unsigned int* reg;
	switch (reset_dev)
	{
		case softreset_card1:		
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 31;
			break;
		
		case softreset_ddr_ahb_reset:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 30;
			break;
		
		case softreset_zctrl:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 29;
			break;
			
		case softreset_h2xusb:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 27;
			break;
			
		case softreset_h2xdma:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 26;
			break;
			
		case softreset_usbphy:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 25;
			break;
			
		case softreset_dma:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 24;
			break;
			
		case softreset_usb:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 23;
			break;
			
		case softreset_deinter:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 22;
			break;
			
		case softreset_mae2_core:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 21;
			break;
			
		case softreset_nand:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 20;
			break;
			
		case softreset_itu:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 19;
			break;
			
		case softreset_adc:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 18;
			break;
			
		case softreset_lcd:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 17;
			break;

		case softreset_ddr:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 16;
			break;
			
		case softreset_i2s:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 15;
			break;
			
		case softreset_ssp:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 14;
			break;
			
		case softreset_mac:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 13;
			break;
			
		case softreset_card:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 12;
			break;
			
		case softreset_i2c:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 11;
			break;
			
		case softreset_ddr_cfg:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 9;
			break;
			
		case softreset_ddrphy:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 8;
			break;
			
		case softreset_ddr1:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 7;
			break;
			
		case softreset_mae2_axi:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 6;
			break;
			
		case softreset_rcrt:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 5;
			break;
			
		case softreset_rtc:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 4;
			break;
			
		case softreset_icu:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 3;
			break;
				
		case softreset_dac:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 2;
			break;
			
		case softreset_saradc:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 1;
			break;
			
		case softreset_imc:
			reg = ((volatile unsigned int *)(SYS_BASE+0x74));
			mask = 0;
			break;
			
		case softreset_isp_scale:
			reg = ((volatile unsigned int *)(SYS_BASE+0x78));
			mask = 12;
			break;
			
		case softreset_isp:
			reg = ((volatile unsigned int *)(SYS_BASE+0x78));
			mask = 11;
			break;
			
		case softreset_scale:
			reg = ((volatile unsigned int *)(SYS_BASE+0x78));
			mask = 10;
			break;
			
		case softreset_i2s2:
			reg = ((volatile unsigned int *)(SYS_BASE+0x78));
			mask = 9;
			break;
			
		case softreset_i2s1:
			reg = ((volatile unsigned int *)(SYS_BASE+0x78));
			mask = 8;
			break;
			
		case softreset_uart0:
			reg = ((volatile unsigned int *)(SYS_BASE+0x78));
			mask = 6;
			break;
			
		case softreset_uart1:
			reg = ((volatile unsigned int *)(SYS_BASE+0x78));
			mask = 5;
			break;
			
		case softreset_uart2:
			reg = ((volatile unsigned int *)(SYS_BASE+0x78));
			mask = 4;
			break;
			
		case softreset_uart3:
			reg = ((volatile unsigned int *)(SYS_BASE+0x78));
			mask = 3;
			break;
			
		case softreset_gpio:
			reg = ((volatile unsigned int *)(SYS_BASE+0x78));
			mask = 2;
			break;
			
		case softreset_timer:
			reg = ((volatile unsigned int *)(SYS_BASE+0x78));
			mask = 1;
			break;
			
		case softreset_pwm:
			reg = ((volatile unsigned int *)(SYS_BASE+0x78));
			mask = 0;
			break;
			
		default:
			printf ("illegal reset device (%d)\n", reset_dev);
			return;
	}
	
	OS_EnterRegion();
	*reg &= ~(1 << mask);
	delay (1000);
	*reg |=  (1 << mask);
	OS_LeaveRegion();
}

#define	SYS_AHB_CLK_EN		((volatile unsigned int *)(SYS_BASE+0x44))
#define	SYS_APB_CLK_EN		((volatile unsigned int *)(SYS_BASE+0x48))
#define	SYS_PER_CLK_EN		((volatile unsigned int *)(SYS_BASE+0x50))
void sys_clk_enable (unsigned int clk)
{
	unsigned int mask;
	volatile unsigned int* reg;
	switch (clk)
	{
		
	// SYS_AHB_CLK_EN
	case zctrl_clk_enable:		reg = SYS_AHB_CLK_EN;	mask = 30;	break;
	case Mae2_enc_enable:		reg = SYS_AHB_CLK_EN;	mask = 29;	break;
	case Mae2_dec_enable:		reg = SYS_AHB_CLK_EN;	mask = 28;	break;
	case Scale_clk_enable:		reg = SYS_AHB_CLK_EN;	mask = 27;	break;
	case Isp_clk_enable:			reg = SYS_AHB_CLK_EN;	mask = 26;	break;
	case Isp_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 25;	break;
	case Scale_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 24;	break;
	case h2xdma_xclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 23;	break;
	case h2xusb_xclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 22;	break;
	case imc_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 21;	break;
	case ddr_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 20;	break;
	case lcd_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 19;	break;
	case itu656_xclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 18;	break;
	case deinterlace_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 17;	break;
	case mae_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 16;	break;
	
	case Isp_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 12;	break;
	case ddrctl_hclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 11;	break;
	case MFC_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 10;	break;
	case h2xdma_hclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 9;	break;
	case h2xusb_hclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 8;	break;
	case Isp_Scale_clk_enable:	reg = SYS_AHB_CLK_EN;	mask = 7;	break;
	case mac_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 6;	break;
	
	case sdc1_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 4;	break;
	case sdc_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 3;	break;
	case nand_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 2;	break;
	case dma_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 1;	break;
	case usb_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 0;	break;
	
	
	// SYS_APB_CLK_EN
	case i2s2_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 16;	break;
	case i2s1_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 15;	break;
	case icu_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 14;	break;
	case pwm_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 13;	break;
	case rcrt_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 12;	break;
	case timer_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 11;	break;
	case uart3_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 10;	break;
	case uart2_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 9;	break;
	case uart1_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 8;	break;
	case uart0_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 7;	break;
	case rtc_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 6;	break;
	case gpio_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 5;	break;
	case ssp_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 4;	break;
	case adc_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 3;	break;
	case i2c_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 2;	break;
	case i2s_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 1;	break;
	case wdt_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 0;	break;
	
	
	// SYS_PER_CLK_EN
	case Tv_out_lcd_pixel_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 31;	break;
	case lcd_out_clk_enable:					reg = SYS_PER_CLK_EN;	mask = 30;	break;
	case Sensor_mclk_out_enable:				reg = SYS_PER_CLK_EN;	mask = 29;	break;
	case Mac_rx_clk_enable:						reg = SYS_PER_CLK_EN;	mask = 28;	break;
	case Mac_tx_clk_enable:						reg = SYS_PER_CLK_EN;	mask = 27;	break;
	
	case Itu_b_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 24;	break;
	case Itu_a_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 23;	break;
	case Uart3_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 22;	break;
	case Uart2_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 21;	break;
	case Uart1_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 20;	break;
	case Uart0_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 19;	break;
	case Gpio_debounce_clk_enable:	reg = SYS_PER_CLK_EN;	mask = 18;	break;
	case rcrt_clk_out_enable:	reg = SYS_PER_CLK_EN;	mask = 17;	break;
	case adc_clk_enable:			reg = SYS_PER_CLK_EN;	mask = 16;	break;
	case i2c_clk_enable:			reg = SYS_PER_CLK_EN;	mask = 15;	break;
	case pwm_clk_enable:			reg = SYS_PER_CLK_EN;	mask = 14;	break;
	case i2s2_mclk_enable:		reg = SYS_PER_CLK_EN;	mask = 13;	break;
	case i2s2_bitclk_enable:	reg = SYS_PER_CLK_EN;	mask = 12;	break;
	case i2s1_mclk_enable:		reg = SYS_PER_CLK_EN;	mask = 11;	break;
	case i2s1_bitclk_enable:	reg = SYS_PER_CLK_EN;	mask = 10;	break;
	case sadc_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 9;	break;
	case i2s_mclk_enable:		reg = SYS_PER_CLK_EN;	mask = 8;	break;
	case i2s_bitclk_enable:		reg = SYS_PER_CLK_EN;	mask = 7;	break;
	case spi_clk_enable:			reg = SYS_PER_CLK_EN;	mask = 6;	break;
	case Spi1_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 5;	break;
	
	case USB_PHY_clk_enable:	reg = SYS_PER_CLK_EN;	mask = 3;	break;
	case usb_12m_enable:			reg = SYS_PER_CLK_EN;	mask = 2;	break;
	case lcd_clk_enable:			reg = SYS_PER_CLK_EN;	mask = 1;	break;
	case ddr_phyclk_enable:		reg = SYS_PER_CLK_EN;	mask = 0;	break;
	
	// 异常
	default:							return;
	}
	
	OS_EnterRegion();
	*reg |= (1 << mask);
	OS_LeaveRegion();
	
}

void sys_clk_disable (unsigned int clk)
{
	unsigned int mask;
	volatile unsigned int* reg;
	switch (clk)
	{
		
	// SYS_AHB_CLK_EN
	case zctrl_clk_enable:		reg = SYS_AHB_CLK_EN;	mask = 30;	break;
	case Mae2_enc_enable:		reg = SYS_AHB_CLK_EN;	mask = 29;	break;
	case Mae2_dec_enable:		reg = SYS_AHB_CLK_EN;	mask = 28;	break;
	case Scale_clk_enable:		reg = SYS_AHB_CLK_EN;	mask = 27;	break;
	case Isp_clk_enable:			reg = SYS_AHB_CLK_EN;	mask = 26;	break;
	case Isp_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 25;	break;
	case Scale_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 24;	break;
	case h2xdma_xclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 23;	break;
	case h2xusb_xclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 22;	break;
	case imc_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 21;	break;
	case ddr_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 20;	break;
	case lcd_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 19;	break;
	case itu656_xclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 18;	break;
	case deinterlace_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 17;	break;
	case mae_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 16;	break;
	
	case Isp_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 12;	break;
	case ddrctl_hclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 11;	break;
	case MFC_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 10;	break;
	case h2xdma_hclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 9;	break;
	case h2xusb_hclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 8;	break;
	case Isp_Scale_clk_enable:	reg = SYS_AHB_CLK_EN;	mask = 7;	break;
	case mac_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 6;	break;
	case sdc1_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 4;	break;
	case sdc_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 3;	break;
	case nand_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 2;	break;
	case dma_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 1;	break;
	case usb_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 0;	break;
	
	
	// SYS_APB_CLK_EN
	case i2s2_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 16;	break;
	case i2s1_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 15;	break;
	case icu_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 14;	break;
	case pwm_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 13;	break;
	case rcrt_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 12;	break;
	case timer_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 11;	break;
	case uart3_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 10;	break;
	case uart2_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 9;	break;
	case uart1_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 8;	break;
	case uart0_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 7;	break;
	case rtc_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 6;	break;
	case gpio_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 5;	break;
	case ssp_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 4;	break;
	case adc_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 3;	break;
	case i2c_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 2;	break;
	case i2s_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 1;	break;
	case wdt_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 0;	break;
	
	
	// SYS_PER_CLK_EN
	case Tv_out_lcd_pixel_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 31;	break;
	case lcd_out_clk_enable:					reg = SYS_PER_CLK_EN;	mask = 30;	break;
	case Sensor_mclk_out_enable:				reg = SYS_PER_CLK_EN;	mask = 29;	break;
	case Mac_rx_clk_enable:						reg = SYS_PER_CLK_EN;	mask = 28;	break;
	case Mac_tx_clk_enable:						reg = SYS_PER_CLK_EN;	mask = 27;	break;
	
	case Itu_b_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 24;	break;
	case Itu_a_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 23;	break;
	case Uart3_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 22;	break;
	case Uart2_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 21;	break;
	case Uart1_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 20;	break;
	case Uart0_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 19;	break;
	case Gpio_debounce_clk_enable:	reg = SYS_PER_CLK_EN;	mask = 18;	break;
	case rcrt_clk_out_enable:	reg = SYS_PER_CLK_EN;	mask = 17;	break;
	case adc_clk_enable:			reg = SYS_PER_CLK_EN;	mask = 16;	break;
	case i2c_clk_enable:			reg = SYS_PER_CLK_EN;	mask = 15;	break;
	case pwm_clk_enable:			reg = SYS_PER_CLK_EN;	mask = 14;	break;
	case i2s2_mclk_enable:		reg = SYS_PER_CLK_EN;	mask = 13;	break;
	case i2s2_bitclk_enable:	reg = SYS_PER_CLK_EN;	mask = 12;	break;
	case i2s1_mclk_enable:		reg = SYS_PER_CLK_EN;	mask = 11;	break;
	case i2s1_bitclk_enable:	reg = SYS_PER_CLK_EN;	mask = 10;	break;
	case sadc_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 9;	break;
	case i2s_mclk_enable:		reg = SYS_PER_CLK_EN;	mask = 8;	break;
	case i2s_bitclk_enable:		reg = SYS_PER_CLK_EN;	mask = 7;	break;
	case spi_clk_enable:			reg = SYS_PER_CLK_EN;	mask = 6;	break;
	case Spi1_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 5;	break;
	
	case USB_PHY_clk_enable:	reg = SYS_PER_CLK_EN;	mask = 3;	break;
	case usb_12m_enable:			reg = SYS_PER_CLK_EN;	mask = 2;	break;
	case lcd_clk_enable:			reg = SYS_PER_CLK_EN;	mask = 1;	break;
	case ddr_phyclk_enable:		reg = SYS_PER_CLK_EN;	mask = 0;	break;
	
	// 异常
	default:							return;
	}
	
	OS_EnterRegion();
	*reg &= ~(1 << mask);
	OS_LeaveRegion();
	
}

// 检查时钟是否已启动
int sys_clk_check (unsigned int clk)
{
	unsigned int mask;
	volatile unsigned int* reg;
	int running = 0;
	switch (clk)
	{
		
	// SYS_AHB_CLK_EN
	case zctrl_clk_enable:		reg = SYS_AHB_CLK_EN;	mask = 30;	break;
	case Mae2_enc_enable:		reg = SYS_AHB_CLK_EN;	mask = 29;	break;
	case Mae2_dec_enable:		reg = SYS_AHB_CLK_EN;	mask = 28;	break;
	case Scale_clk_enable:		reg = SYS_AHB_CLK_EN;	mask = 27;	break;
	case Isp_clk_enable:			reg = SYS_AHB_CLK_EN;	mask = 26;	break;
	case Isp_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 25;	break;
	case Scale_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 24;	break;
	case h2xdma_xclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 23;	break;
	case h2xusb_xclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 22;	break;
	case imc_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 21;	break;
	case ddr_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 20;	break;
	case lcd_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 19;	break;
	case itu656_xclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 18;	break;
	case deinterlace_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 17;	break;
	case mae_xclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 16;	break;
	
	case Isp_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 12;	break;
	case ddrctl_hclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 11;	break;
	case MFC_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 10;	break;
	case h2xdma_hclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 9;	break;
	case h2xusb_hclk_enable:	reg = SYS_AHB_CLK_EN;	mask = 8;	break;
	case Isp_Scale_clk_enable:	reg = SYS_AHB_CLK_EN;	mask = 7;	break;
	case mac_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 6;	break;
	
	case sdc1_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 4;	break;
	case sdc_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 3;	break;
	case nand_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 2;	break;
	case dma_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 1;	break;
	case usb_hclk_enable:		reg = SYS_AHB_CLK_EN;	mask = 0;	break;
	
	
	// SYS_APB_CLK_EN
	case i2s2_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 16;	break;
	case i2s1_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 15;	break;
	case icu_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 14;	break;
	case pwm_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 13;	break;
	case rcrt_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 12;	break;
	case timer_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 11;	break;
	case uart3_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 10;	break;
	case uart2_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 9;	break;
	case uart1_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 8;	break;
	case uart0_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 7;	break;
	case rtc_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 6;	break;
	case gpio_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 5;	break;
	case ssp_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 4;	break;
	case adc_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 3;	break;
	case i2c_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 2;	break;
	case i2s_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 1;	break;
	case wdt_pclk_enable:		reg = SYS_APB_CLK_EN;	mask = 0;	break;
	
	
	// SYS_PER_CLK_EN
	case Tv_out_lcd_pixel_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 31;	break;
	case lcd_out_clk_enable:					reg = SYS_PER_CLK_EN;	mask = 30;	break;
	case Sensor_mclk_out_enable:				reg = SYS_PER_CLK_EN;	mask = 29;	break;
	case Mac_rx_clk_enable:						reg = SYS_PER_CLK_EN;	mask = 28;	break;
	case Mac_tx_clk_enable:						reg = SYS_PER_CLK_EN;	mask = 27;	break;
	
	case Itu_b_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 24;	break;
	case Itu_a_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 23;	break;
	case Uart3_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 22;	break;
	case Uart2_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 21;	break;
	case Uart1_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 20;	break;
	case Uart0_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 19;	break;
	case Gpio_debounce_clk_enable:	reg = SYS_PER_CLK_EN;	mask = 18;	break;
	case rcrt_clk_out_enable:	reg = SYS_PER_CLK_EN;	mask = 17;	break;
	case adc_clk_enable:			reg = SYS_PER_CLK_EN;	mask = 16;	break;
	case i2c_clk_enable:			reg = SYS_PER_CLK_EN;	mask = 15;	break;
	case pwm_clk_enable:			reg = SYS_PER_CLK_EN;	mask = 14;	break;
	case i2s2_mclk_enable:		reg = SYS_PER_CLK_EN;	mask = 13;	break;
	case i2s2_bitclk_enable:	reg = SYS_PER_CLK_EN;	mask = 12;	break;
	case i2s1_mclk_enable:		reg = SYS_PER_CLK_EN;	mask = 11;	break;
	case i2s1_bitclk_enable:	reg = SYS_PER_CLK_EN;	mask = 10;	break;
	case sadc_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 9;	break;
	case i2s_mclk_enable:		reg = SYS_PER_CLK_EN;	mask = 8;	break;
	case i2s_bitclk_enable:		reg = SYS_PER_CLK_EN;	mask = 7;	break;
	case spi_clk_enable:			reg = SYS_PER_CLK_EN;	mask = 6;	break;
	case Spi1_clk_enable:		reg = SYS_PER_CLK_EN;	mask = 5;	break;
	
	case USB_PHY_clk_enable:	reg = SYS_PER_CLK_EN;	mask = 3;	break;
	case usb_12m_enable:			reg = SYS_PER_CLK_EN;	mask = 2;	break;
	case lcd_clk_enable:			reg = SYS_PER_CLK_EN;	mask = 1;	break;
	case ddr_phyclk_enable:		reg = SYS_PER_CLK_EN;	mask = 0;	break;
	
	// 异常
	default:							return running;
	}
	
	if(*reg & (1 << mask))
		running = 1;
	return running;
}