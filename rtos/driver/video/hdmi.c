#include "hardware.h"
#include  "lcd.h"
#include  <stdio.h>
#include <math.h>
#include "hw_osd_layer.h"
#include <gpio.h>
#include "pwm.h"

#include "rtos.h"

#define	W	1280
#define	H	720

#define	HDMI_720P_60HZ		1
//#define	HDMI_720P_30HZ		1

static void HDMI_Init (void)
{
#define      lcd_enable             1// 1
#define      screen_width           W
//#define      rgb_pad_mode           5// 101：BGR
#define      rgb_pad_mode           0	// 000：RGB
#define      direct_enable          0
#define      mem_lcd_enable         0 
#define      range_coeff_y          0
#define      range_coeff_uv         0
#define      lcd_done_intr_enable   1   //使能中断=1 禁止中断=0 lcd_done_intr
#define      dac_for_video          0
#define      dac_for_cvbs           0 

          

#define      lcd_interlace_flag     0
#define      screen_type            1  // 001：并行数据屏；
													// 010：串行Srgb屏
													//	011：ITU656输出；
													// 101：并行数据屏；
													// 110：串行CPU屏；		
#define      VSW1_enable            0 
#define      LcdVComp               0
#define      itu_pal_ntsc           0
#define      hsync_ivs              1
#define      vsync_ivs              1
#define      lcd_ac_ivs             0
#define      test_on_flag           0

#define      back_color             (0x22<<16)|(0x44<<8)|(0x88 )	// 无背景色
#define      cpu_screen_csn_bit     0
#define      cpu_screen_wen_bit     9
	
// 720p 60hz (  { 4,0, 1280, 720, 1650, 750, 74250000L, 0x2E, 110,  40, 220, 5, 5, 20, PROG,Vpos,Hpos}, //1280x720@60Hz)
//              {62,0, 1280, 720, 3300, 750, 74250000L, 0x2E, 1760, 40, 220, 5, 5, 20, PROG,Vpos,Hpos}, //1280x720@30Hz

#if HDMI_720P_60HZ
#define      DEN_h_rise             (110+40+220)    	// 数据使能信号data_en水平起始位置。
#define      DEN0_v_rise            (5+5+20)   			// 数据使能信号data_en垂直起始位置，逐行时序时，为每帧的垂直时序，隔行时序时，为第一场的时序。
#elif HDMI_720P_30HZ
#define      DEN_h_rise             (1760+40+220)		// 数据使能信号data_en水平起始位置。
#define      DEN0_v_rise            (5+5+20)    		// 数据使能信号data_en垂直起始位置，逐行时序时，为每帧的垂直时序，隔行时序时，为第一场的时序。
	
#endif
	
#define      DEN_h_fall             (DEN_h_rise + W)  // 数据使能信号data_en水平结束位置。
#define      DEN0_v_fall            (DEN0_v_rise + H)	// 数据使能信号data_en垂直结束位置，逐行时序时，为每帧的垂直时序，隔行时序时，为第一场的时序。  
#define      DEN1_v_rise            10						// 数据使能信号data_en垂直起始位置，
																		//		当隔行输出时（VSW1_enable=1）有效，代表的是第二场的垂直起始位置，逐行输出时无效。
#define      DEN1_v_fall            (DEN1_v_rise + H) // 数据使能信号data_en垂直结束位置，
																		//		当隔行输出时（VSW1_enable=1）有效，代表的是第二场的垂直结束位置，逐行输出时无效。
	


// 帧频 = LCD时钟/(CPL * LPS)	
// 每行时钟周期个数	

#define      CPL                    ( W + DEN_h_rise )       // clock cycles per line  max 4096     ;  from register   timing2 16
#define      LPS                    ( H + DEN0_v_rise )       // Lines per screen value  ;  from register   timing1 0	
	
#if HDMI_720P_60HZ
	
#define      HSW_rise               ( 110      )	// 行同步信号hsyn上升沿位置
#define      HSW_fall               ( 110 + 40 )	// 行同步信号hsyn下降沿位置

#define      VSW0_v_rise            ( 5      )   // 场同步信号vsyn0上升沿位置，逐行时序时为场同步，隔行时序时为第一场的场同步。
#define      VSW0_v_fall            ( 5 + 5  )   // 场同步信号vsyn0下降沿位置，逐行时序时为场同步，隔行时序时为第一场的场同步。

#define      VSW0_h_rise             5        // 场同步信号vsyn0上升沿位置，逐行时序时为场同步，隔行时序时为第一场的场同步。
#define      VSW0_h_fall				(5 + 5)	 // 场同步信号vsyn0下降沿位置，逐行时序时为场同步，隔行时序时为第一场的场同步。

#elif HDMI_720P_30HZ
	
#define      HSW_rise               ( 1760      )	// 行同步信号hsyn上升沿位置
#define      HSW_fall               ( 1760 + 40 )	// 行同步信号hsyn下降沿位置

#define      VSW0_v_rise            ( 5      )   // 场同步信号vsyn0上升沿位置，逐行时序时为场同步，隔行时序时为第一场的场同步。
#define      VSW0_v_fall            ( 5 + 5  )   // 场同步信号vsyn0下降沿位置，逐行时序时为场同步，隔行时序时为第一场的场同步。

#define      VSW0_h_rise             5        // 场同步信号vsyn0上升沿位置，逐行时序时为场同步，隔行时序时为第一场的场同步。
#define      VSW0_h_fall			   (5 + 5)	 // 场同步信号vsyn0下降沿位置，逐行时序时为场同步，隔行时序时为第一场的场同步。
	
#endif
	
#define      VSW1_v_rise                313                // Vertical sync width value ;  from register   timing1 10
#define      VSW1_v_fall                316               // Vertical sync width value  ;  from register   timing1 10

#define      VSW1_h_rise                864 
#define      VSW1_h_fall                864

#define      sav_pos                   288
#define      f1_vblk_start             624
#define      f1_vblk_end               23
#define      f2_vblk_start             311
#define      f2_vblk_end               336
#define      f2_start                  313
#define      f2_end                    625
#define      binary_thres              128
#define      binary_big                240
#define      binary_small              16

//#define      osd_0_global_coeff_enable      1
//#define      osd_0_enable                   1
#define      osd_0_global_coeff_enable      0
#define      osd_0_enable                   0

#define      osd_0_binary_enable            0
#define      osd_0_format                   0//0 : YUV420, 1 : ARGB8888, 2: RGB565, 3: RGB454
#define      osd_0_width                    0
#define      osd_0_height                   0
#define      osd_0_vc1_range_map            0
#define      osd_0_chroma_disable           0
#define      osd_0_h_position               0
#define      osd_0_v_position               0
#define      osd_0_global_coeff             0//设置透明度 0不透明 <~~~> 255 全透明
#define      osd_0_y_addr                   0x80000000//设置为全透明色之后，影响因子osd_0_mult_coef起作用？？
#define      osd_0_u_addr                   0x80000000
#define      osd_0_v_addr                   0x80000000
#define      osd_0_stride                   1280// 源  宽度
#define      osd_0_hoffset                  0
#define      osd_0_mult_coef                0

#define      osd_1_global_coeff_enable      0
#define      osd_1_enable                   0
#define      osd_1_binary_enable            0
#define      osd_1_format                   0//0 : YUV420, 1 : ARGB8888, 2: RGB565, 3: RGB454
#define      osd_1_width                    0
#define      osd_1_height                   0
#define      osd_1_vc1_range_map            0
#define      osd_1_chroma_disable           0
#define      osd_1_h_position               0
#define      osd_1_v_position               0
#define      osd_1_global_coeff             0 //设置透明度
#define      osd_1_y_addr                   0
#define      osd_1_u_addr                  0
#define      osd_1_v_addr                  0
#define      osd_1_stride                  0
#define      osd_1_hoffset                 0
#define      osd_1_mult_coef               0

#define      osd_2_global_coeff_enable      0
#define      osd_2_enable                   0
#define      osd_2_binary_enable            0
#define      osd_2_format                   0
#define      osd_2_width                    0
#define      osd_2_height                   0
#define      osd_2_vc1_range_map            0
#define      osd_2_chroma_disable           0
#define      osd_2_h_position               0
#define      osd_2_v_position               0
#define      osd_2_global_coeff             0 //设置透明度 0不透明 <~~~> 64 全透明
#define      osd_2_y_addr                   0
#define      osd_2_u_addr                  0
#define      osd_2_v_addr                  0
#define      osd_2_stride                  0
#define      osd_2_hoffset                 0
#define      osd_2_mult_coef               0
	unsigned int  lcd_param0, lcd_param1, lcd_param2, lcd_param3 ;
	unsigned int  lcd_param4, lcd_param5, lcd_param6, lcd_param7 ;
	unsigned int  lcd_param8, lcd_param9, lcd_param10, lcd_param11 ;
	unsigned int  lcd_param12, lcd_param13, lcd_param14, lcd_param15 ;
	unsigned int  lcd_param16, lcd_param17, lcd_param18, lcd_param19 ;
	unsigned int  lcd_param20, lcd_param21, lcd_param22  ;
	
	unsigned int  osd_0_param0, osd_0_param1, osd_0_param2, osd_0_param3 , osd_0_param4, osd_0_param5 ;
	unsigned int  osd_1_param0, osd_1_param1, osd_1_param2, osd_1_param3 , osd_1_param4, osd_1_param5 ;
	unsigned int  osd_2_param0, osd_2_param1, osd_2_param2, osd_2_param3 , osd_2_param4, osd_2_param5 ;
	unsigned int  osd_3_param0, osd_3_param1, osd_3_param2, osd_3_param3 , osd_3_param4, osd_3_param5 ;
	unsigned int  osd_4_param0, osd_4_param1, osd_4_param2, osd_4_param3 , osd_4_param4, osd_4_param5 ;
	unsigned int  osd_5_param0, osd_5_param1, osd_5_param2, osd_5_param3 , osd_5_param4, osd_5_param5 ;
	char  stop_lcd = 0 ;
   
   unsigned int addr,regno;
	
	LCD_PARAM0_reg  = (1<<17);
	LCD_PARAM0_reg  = 0;
	LCD_PARAM0_reg  = (1<<17);
	LCD_PARAM0_reg  = 0;//bit 26 enable lcd_done_intr 
	LCD_PARAM0_reg  = (0<<26);
	
	LCD_TVOUT_PARAM3_reg  = (1<<11)|(3<<12) ;
	lcd_param0  = 
		lcd_enable   |
		(screen_width          <<1)| 
		(rgb_pad_mode        <<13)| 
		(direct_enable          <<16)|  
		(mem_lcd_enable   <<17     )|
		(range_coeff_y  <<18    )|
		(range_coeff_uv          <<22    )|
		(lcd_done_intr_enable   <<26     )|
		(dac_for_video   <<27     )|
		(dac_for_cvbs   <<28     )|
		(1<<30)|
		(stop_lcd<<31)  ;
	
	lcd_param1  =
		( lcd_interlace_flag     <<0 ) |
		( screen_type     <<1) |
		( VSW1_enable     <<13) |
		( LcdVComp     <<14) |
		( itu_pal_ntsc     <<15) |
		( hsync_ivs     <<17) |
		( vsync_ivs     <<18) |
		( lcd_ac_ivs     <<19) |
		( 1  <<21 ) |
		( 1 <<23  ) |
		( test_on_flag     <<31)   ;
	
	lcd_param2  =
		( back_color     <<0 ) |
		( cpu_screen_csn_bit     <<24) |
		( cpu_screen_wen_bit     <<28);
   
	lcd_param3  =
		( DEN_h_rise      <<0  ) |  // data enable ?ú????é?éyμ?????
		( DEN_h_fall      <<12 )  ;
	
	lcd_param4  =
		( DEN0_v_rise     <<0  )|  // even field data enable é?éy????
		( DEN0_v_fall     <<12 ) ;
	
	lcd_param5  =
		( DEN1_v_rise     <<0  )|
		( DEN1_v_fall     <<12 ) ;
	
	lcd_param6  =
		(  CPL    <<0  )|               // clock cycles per line  max 4096                                 ;  from register   timing2 16
		(  LPS    <<12 ) ;              // Lines per screen value                                          ;  from register   timing1 0
	
	lcd_param7  =
		(  HSW_rise       <<0  )|
		(  HSW_fall       <<12 ) ;
	
	lcd_param8  =
		(  VSW0_v_rise    <<0  )|            // Vertical sync width value                                       ;  from register   timing1 10
		(  VSW0_v_fall    <<12 ) ;              // Vertical sync width value                                       ;  from register   timing1 10
	
	lcd_param9  =
		(  VSW0_h_rise   <<0  )|   
		(  VSW0_h_fall   <<12 ) ;   
	
	lcd_param10  =
		(  VSW1_v_rise   <<0  )|   
		(  VSW1_v_fall   <<12 ) ;   
	
	lcd_param11  =
		(  VSW1_h_rise    <<0  )|   
		(  VSW1_h_fall    <<12 ) ;   
	
	lcd_param12  = 66 | (129<<9) | (25<<18);
	
	lcd_param13  = 38 |  (74<<9) | (112<<18);
	
	lcd_param14  = 112 | (94<<9) | (18<<18); 
	
	lcd_param15  = 256 | (0<<9) | (394<<18);
	
	lcd_param16  = 256 |  (47<<9) | (118<<18);
	
	lcd_param17  = 256 | (465<<9) | (0<<19);
	
	//  lcd_param15  = 298 | (0<<9) | (409<<18);  
	
	//  lcd_param16  = 299 |  (100<<9) | (208<<18);
	
	//  lcd_param17  = 298 | (517<<9) | (0<<19); 
	
	lcd_param18  =
		(sav_pos    <<0     )   ;
	
	lcd_param19  =
		( f1_vblk_start    <<0  )|
		( f1_vblk_end    <<12 ) ;
	
	lcd_param20  =
		( f2_vblk_start        <<0  ) |
		( f2_vblk_end       <<12 )  ;
	lcd_param21  =
		( f2_start   <<0 )| 
		( f2_end   <<12);
	lcd_param22  =
		( binary_thres   <<16 )|
		( binary_big   <<8 )|
		( binary_small   <<0) ;
	
	LCD_PARAM1_reg     = lcd_param1  ;
	LCD_PARAM2_reg     = lcd_param2  ;
	LCD_PARAM3_reg     = lcd_param3  ;
	LCD_PARAM4_reg     = lcd_param4  ;
	LCD_PARAM5_reg     = lcd_param5  ;
	LCD_PARAM6_reg     = lcd_param6  ;
	LCD_PARAM7_reg     = lcd_param7  ;
	LCD_PARAM8_reg     = lcd_param8  ;
	LCD_PARAM9_reg     = lcd_param9  ;
	LCD_PARAM10_reg    = lcd_param10 ;
	LCD_PARAM11_reg    = lcd_param11 ;
	LCD_PARAM12_reg    = lcd_param12 ;
	LCD_PARAM13_reg    = lcd_param13 ;
	LCD_PARAM14_reg    = lcd_param14 ;
	LCD_PARAM15_reg    = lcd_param15 ;
	LCD_PARAM16_reg    = lcd_param16 ;
	LCD_PARAM17_reg    = lcd_param17 ;
	
	LCD_PARAM18_reg    = lcd_param18 ;
	LCD_PARAM19_reg    = lcd_param19 ;
	LCD_PARAM20_reg    = lcd_param20 ;
	LCD_PARAM21_reg    = lcd_param21 ;
	LCD_PARAM22_reg    = lcd_param22 ;
	//  lcd_param1 = LCD_PARAM1_reg      ;
	
   // addr,regno
   addr = LCD_OSD_BASE;
   for(regno = 0 ;regno<18 ; regno++ )
   {
      *(volatile unsigned int *)(addr)= 0;
      addr += 4;
   }
   
	LCD_PARAM0_reg     = lcd_param0  ;
	//offset :0x70190000 +0x214
	LCD_dithing_reg    = (1<<7) | (1<<15) | 5<<24;// 
}




static void HDMI_SelPad(void)
{
	unsigned int val;
	OS_IncDI();    // Initially disable interrupts
	
	rSYS_PAD_CTRL05 = 1<<27|1<<24|1<<21|1<<18|1<<15|1<<12|1<<9|1<<6|1<<3|1<<0;
	rSYS_PAD_CTRL06 = 1<<27|1<<24|1<<21|1<<18|1<<15|1<<12|1<<9|1<<6|1<<3|1<<0;
	
	val = rSYS_PAD_CTRL07;
	// lcd_dout[20] lcd_dout[21] lcd_dout[22] lcd_dout[23]
	val &= 0xFF000000;
	val |= 1<<21|1<<18|1<<15|1<<12|1<<9|1<<6|1<<3|1<<0;
	
	// 	[31]	lcd_d22, 0->lcd panel,输出port；1->cpu panel, 输入,cpu_scr_sel=1					
	val &= ~( (1 << 31) );
	
	rSYS_PAD_CTRL07 = val;
	
	rSYS_SOFT_RSTNA &= ~(1<<17);
	__asm ("nop");
   __asm ("nop");
	rSYS_SOFT_RSTNA |= (1<<17);
   __asm ("nop");
   __asm ("nop");
   
	OS_DecRI();		// enable interrupt
}


static void HDMI_ClockInit (void)
{
	unsigned int IntTvClkSwitch_sel;
	unsigned int Tvout_lcd_clk_wire_div;
	unsigned int Tvout_lcd_clk;
	unsigned int Tvout_lcd_pixel_clk;
	unsigned int Tvout_lcd_pixel_clk_wire_div;
	
	// 74.25MHz

	// 2-0	R/W	0x4	IntTvClkSwitch_sel：IntTvClkSwitch
	//							3'b000: clk_240m
	//							3'b001: syspll_clk 
	//							3'b010: audpll_clk
	//							3'b100: clk_24m
	IntTvClkSwitch_sel = 0x01;

	//	7-3	R/W	0x40	Tvout_lcd_clk_wire_div
	//							Tvout_lcd_clk  = IntTvClkSwitch / ( Tvout_lcd_pixel_clk _div ? Tvout_lcd_pixel_clk_div : 1 );
	Tvout_lcd_clk_wire_div = 4;

	// 8	R/W	0x0	Tvout_lcd_clk  0 normal  1 inv
	Tvout_lcd_clk = 0;

	// 9	R/W	0x0	Tvout_lcd_pixel_clk 0 normal  1 inv
	Tvout_lcd_pixel_clk = 0;

	// 17-15	R/W	0x4	Tvout_lcd_pixel_clk_wire_div
	//							Tvout_lcd_pixel_clk_wire = tvout_lcd_clk / ( Tvout_lcd_pixel_clk_wire_div ? Tvout_lcd_pixel_clk_wire_div : 1 );
	Tvout_lcd_pixel_clk_wire_div = 1;

	//rSYS_LCD_CLK_CFG = (4 << 0) | (1 << 3) | (1 << 15);
	//rSYS_LCD_CLK_CFG = (4 << 0) | (2 << 3) | (1 << 15);

	// 74.25MHz = 297MHz / 4
	rSYS_LCD_CLK_CFG =	(Tvout_lcd_pixel_clk_wire_div << 15)
							|	(Tvout_lcd_pixel_clk << 9)
							|	(Tvout_lcd_clk << 8)
							|	(Tvout_lcd_clk_wire_div << 3)
							|	(IntTvClkSwitch_sel << 0)
							;

	Lcd_ClkEnable ();
}

static void HDMI_ClockStop (void)
{
	Lcd_ClkDisable ();
}



// HDMI初始化
void HW_HDMI_Init (void)
{
	HDMI_SelPad();
	HDMI_ClockInit ();
	HDMI_Init();
}

// RGB LCD关闭退出
void HW_HDMI_Exit(void)
{
	int i;
	// Lcd_enable
	//	LCD使能信号，为1时LCD模块工作，为0时，LCD模块不工作。
	OS_EnterRegion ();	// 禁止任务切换
	
	// 等待帧刷新结束
	LCD_INTR_CLR_reg = 0xFFFFFFFF;
	while(!(LCD_STATUS_reg & 0x01));		// bit1: lcd_done_intr(每场结束中断) ,
	// 禁止LCDC
	LCD_PARAM0_reg &= ~1;
	
	/*
	// 设置为GPIO，输出低
	for (i = GPIO52; i <= GPIO79; i ++)
	{
		SetGPIOPadData (i, euDataLow);
		SetGPIOPadDirection (i, euOutputPad);
	}
	rSYS_PAD_CTRL05 = 0;
	rSYS_PAD_CTRL06 = 0;
	rSYS_PAD_CTRL07 = 0;
	*/
	
	OS_LeaveRegion ();	// 禁止任务切换
}