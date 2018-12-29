#include "hardware.h"
#include  "lcd.h"
#include  <stdio.h>
#include <math.h>
#include "hw_osd_layer.h"
#include <gpio.h>
#include "pwm.h"
#include "xm_dev.h"

#include "rtos.h"

#ifdef _SUPPORT_LCD_RGB_RM68172_

#define	LCD_BL_GPIO			GPIO28

//#define	PWM_BL

#if TULV_BB_TEST
// 开发板评估
#define	LCD_RESET_GPIO		GPIO87
#define	LCD_SDA_GPIO		GPIO84
#define	LCD_SCL_GPIO		GPIO83
#define	LCD_CS_GPIO		GPIO80
#else
// 产品板
#define	LCD_RESET_GPIO		GPIO93
#define	LCD_SDA_GPIO		GPIO81
#define	LCD_SCL_GPIO		GPIO80
#define	LCD_CS_GPIO		GPIO92
#endif

#define	SCL_RISING

#define	W	480
#define	H	854

//#define	H1		1	// 高温75度挂机
//#define	H2		1	// 
#define	H3		1

static void lcd_rm68172_osd_init (void)
{
#define      lcd_enable             1// 1
#define      screen_width           W
#define      rgb_pad_mode           5//5
#define      direct_enable          0
#define      mem_lcd_enable         0 
#define      range_coeff_y          0
#define      range_coeff_uv         0
#define      lcd_done_intr_enable   1   //使能中断=1 禁止中断=0 lcd_done_intr
#define      dac_for_video          0
#define      dac_for_cvbs           0 

//=============== RGB Timing =============== 
//H Active = 480
//H Back Porch (Sync Width Not Included) = 16
//H Front Porch = 24
//H Pulse Width = 8
//V Active = 854
//V Back Porch (Sync Width Not Included) = 16
//V Front Porch = 18
//V Pulse Width = 2
//=============== RGB Timing =============== 
          

#define      lcd_interlace_flag     0
#define      screen_type            1 //pRGB_i   0  pRGB_p   1   sRGB_p   2  ITU656   3
#define      VSW1_enable            0 
#define      LcdVComp               0
#define      itu_pal_ntsc           0
#define      hsync_ivs              1
#define      vsync_ivs              1
#define      lcd_ac_ivs             0
#define      test_on_flag           0

//#define      back_color             (200<<16)|(100<<8)|(16)
#define      back_color             (0xff<<16)|(0x00<<8)|(0x00 )	// 无背景色
#define      cpu_screen_csn_bit     0
#define      cpu_screen_wen_bit     9
	
#if H1
#define		THFP			150		// Horizontal front porch
#define		THS			12		// Horizontal low pulse width
#define		THBP			150		// Horizontal back porch
	
#define		TVFP			9//8		// Vertical front porch
#define		TVS			2		// Vertical low pulse width
#define		TVBP			9//8		// Vertical back porch
	
#elif H2
#define		THFP			110		// Horizontal front porch
#define		THS			8		// Horizontal low pulse width
#define		THBP			110		// Horizontal back porch
	
#define		TVFP			9//8		// Vertical front porch
#define		TVS			2		// Vertical low pulse width
#define		TVBP			190//8		// Vertical back porch

#elif H3
#define		THFP			120		// Horizontal front porch
#define		THS			8		// Horizontal low pulse width
#define		THBP			120		// Horizontal back porch
	
#define		TVFP			9//8		// Vertical front porch
#define		TVS			2		// Vertical low pulse width
#define		TVBP			210//8		// Vertical back porch
	
#else
#define		THFP			24		// Horizontal front porch
#define		THS			8		// Horizontal low pulse width
#define		THBP			16		// Horizontal back porch
	
#define		TVFP			9//8		// Vertical front porch
#define		TVS			2		// Vertical low pulse width
#define		TVBP			9//8		// Vertical back porch
#endif

#define      DEN_h_rise             (THFP+THS+THBP)    // 数据使能信号data_en水平起始位置。
#define      DEN_h_fall             (DEN_h_rise+W)  	// 数据使能信号data_en水平结束位置。
#define      DEN0_v_rise            (TVFP+TVS+TVBP)  	 // 数据使能信号data_en垂直起始位置，逐行时序时，为每帧的垂直时序，隔行时序时，为第一场的时序。
#define      DEN0_v_fall            (DEN0_v_rise+H)  // 数据使能信号data_en垂直结束位置，逐行时序时，为每帧的垂直时序，隔行时序时，为第一场的时序。
#define      DEN1_v_rise            20
#define      DEN1_v_fall            (20+H) 
	

// 帧频 = LCD时钟/(CPL * LPS)	
// 每行时钟周期个数	

// 每屏行数量	
#define      CPL                    (W + DEN_h_rise)   		// clock cycles per line  max 4096     ;  from register   timing2 16
																				// 480 + 24 + 8 + 16 = 528
#define      LPS                    (H + DEN0_v_rise)      	// Lines per screen value  ;  from register   timing1 0	
																				//	854 + 9 + 2 + 9 = 874
	
#define      HSW_rise                  ( THFP   )		// 行同步信号hsyn上升沿位置
#define      HSW_fall                  ( THFP+THS )	// 行同步信号hsyn下降沿位置

#define      VSW0_v_rise               ( TVFP    )    // 场同步信号vsyn0上升沿位置，逐行时序时为场同步，隔行时序时为第一场的场同步。
#define      VSW0_v_fall               ( TVFP+TVS )   // 场同步信号vsyn0下降沿位置，逐行时序时为场同步，隔行时序时为第一场的场同步。

#define      VSW0_h_rise               ( TVFP    ) 		// 场同步信号vsyn0上升沿位置，逐行时序时为场同步
#define      VSW0_h_fall					( TVFP+TVS  ) 		// 场同步信号vsyn0下降沿位置，逐行时序时为场同步

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
#define      osd_0_stride                   0// 源  宽度
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
	
//#define	OLD_SETTING	
	// YUV(Full-Range, 0 ~ 255)
	// YCbCr to RGB coef0，计算R时 Y Cb Cr的系数。
	// R = ((Y - ysub_sel*16)* YUV2RGB_coeff00  + (Cr-128) * YUV2RGB_coeff02)/256 
	// R′ = 1.164(Y709 – 16) + 1.793(Cr – 128)
	
#ifdef OLD_SETTING
	lcd_param15  = 256 | (0<<9) | (394<<18);
#else
	lcd_param15  = (((unsigned int)(1.164 * 256 + 0.5)) <<  0)		// YUV2 RGB_coeff00, YUV到RGB转换时，Y的系数。
					 | (0 << 9)
					 |	(((unsigned int)(1.793 * 256 + 0.5)) << 18)		// YUV2 RGB _coeff02, YUV到RGB转换时，Cr的系数。
					 ;
#endif
	
	// YCbCr to RGB coef1，计算G时 Y Cb Cr的系数。
	// G =((Y - ysub_sel*16) * YUV2RGB_coeff00 - (Cb - 128) * YUV2RGB_coeff11 - (Cr - 128) * YUV2RGB_coeff12)/256 
	// G′ = 1.164(Y709 – 16) –0.213(Cb – 128) – 0.534(Cr – 128) 
#ifdef OLD_SETTING	
	lcd_param16  = 256 |  (47<<9) | (118<<18);
#else
	lcd_param16 = (1 << 0)		// Ysub_sel, 计算R G B时Y上是否减16选择。
										//		1：-16
										//		0：-0
					| (0x80 << 1)
					| (((unsigned int)(0.213 * 256 + 0.5)) <<  9)		// YUV2 RGB_coeff11 YUV到RGB转换时，Cb的系数。
					| (((unsigned int)(0.534 * 256 + 0.5)) << 18)		// YUV2 RGB _coeff12 YUV到RGB转换时，Cr的系数。
					;
#endif
	
	// YCbCr to RGB coef2，计算B时 Y Cb Cr的系数。
   // B =((Y - ysub_sel*16) * YUV2RGB_coeff00 +(Cb - 128) * YUV2RGB_coeff21)/256 
	// B′ = 1.164(Y709 – 16) + 2.115(Cb – 128)
#ifdef OLD_SETTING	
	lcd_param17  = 256 | (465<<9) | (0<<19);
#else
	lcd_param17 = (0x100 << 0)
					| (((unsigned int)(2.115 * 256 + 0.5)) <<  9)		// YUV2 RGB _coeff21 YUV到RGB转换时，Cb的系数。
					| (0 << 19)
					;
#endif
	
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




static void LCD_SelPad(void)
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


// Pixel clock cycle fPCLKSYS 8 30 MHz
static void LCD_ClockInit (void)
{
	unsigned int IntTvClkSwitch_sel;
	unsigned int Tvout_lcd_clk_wire_div;
	unsigned int Tvout_lcd_clk;
	unsigned int Tvout_lcd_pixel_clk;
	unsigned int Tvout_lcd_pixel_clk_wire_div;
	unsigned int TVOUT_LCD_OUT_DLY;
	
	// fPCLKSYS 8 30
	// PCLK = 240/9 = 26.7MHz
	// PCLK = 240/15 = 16MHz
	// fps = 16 / (528 * 874) = 34.7
	// fps = 12 / (528 * 874) = 26.0
	// fps = 10 / (528 * 874) = 21.7

	// 2-0	R/W	0x4	IntTvClkSwitch_sel：IntTvClkSwitch
	//							3'b000: clk_240m
	//							3'b001: syspll_clk 
	//							3'b010: audpll_clk
	//							3'b100: clk_24m
	IntTvClkSwitch_sel = 0;

	//	7-3	R/W	0x40	Tvout_lcd_clk_wire_div
	//							Tvout_lcd_clk  = IntTvClkSwitch / ( Tvout_lcd_pixel_clk _div ? Tvout_lcd_pixel_clk_div : 1 );
	
#if H1
	Tvout_lcd_clk_wire_div = 16;		// 15MHz
#elif  H2
	Tvout_lcd_clk_wire_div = 16;		// 15MHz
#elif  H3
	Tvout_lcd_clk_wire_div = 15;		// 16MHz
#else
	// Tvout_lcd_clk_wire_div = 9;
	//Tvout_lcd_clk_wire_div = 15;
	//Tvout_lcd_clk_wire_div = 16;
	//Tvout_lcd_clk_wire_div = 20;		// 12MHz
	Tvout_lcd_clk_wire_div = 24;		// 10MHz
#endif
	

	// 8	R/W	0x0	Tvout_lcd_clk  0 normal  1 inv
	Tvout_lcd_clk = 1;

	// 9	R/W	0x0	Tvout_lcd_pixel_clk 0 normal  1 inv
	Tvout_lcd_pixel_clk = 0;

	// 17-15	R/W	0x4	Tvout_lcd_pixel_clk_wire_div
	//							Tvout_lcd_pixel_clk_wire = tvout_lcd_clk / ( Tvout_lcd_pixel_clk_wire_div ? Tvout_lcd_pixel_clk_wire_div : 1 );
	Tvout_lcd_pixel_clk_wire_div = 1;
	
	// 24-18	R/W	0	TVOUT_LCD_OUT DLY
	TVOUT_LCD_OUT_DLY = 0;

	// 26.7MHz
	rSYS_LCD_CLK_CFG =	(TVOUT_LCD_OUT_DLY << 18)
							|	(Tvout_lcd_pixel_clk_wire_div << 15)
							|	(Tvout_lcd_pixel_clk << 9)
							|	(Tvout_lcd_clk << 8)
							|	(Tvout_lcd_clk_wire_div << 3)
							|	(IntTvClkSwitch_sel << 0)
							;

	Lcd_ClkEnable ();
}

static void LCD_ClockStop (void)
{
	Lcd_ClkDisable ();
}

#ifdef PWM_BL

#else
// 设置背光级别 (0.0 ~ 1.0)
// 1-1, 1-2, 1-3, 1-4, 0
static const unsigned int bl_level_count[5] = {2, 3, 4, 5, 0};
static unsigned int bl_toggle_count;
static unsigned int bl_toggle_index;

static OS_TICK_HOOK bl_tick_hook;
static void bl_tick_func (void)
{
	// 使用GPIO模拟PWM
	if(bl_toggle_count == 0)
	{
		rGPIO_PA_RDATA &= ~(1<<28);
	}
	else
	{
		if(bl_toggle_index == 0)
		{
			rGPIO_PA_RDATA |= (1<<28);		// 开
		}
		else
		{
			rGPIO_PA_RDATA &= ~(1<<28);	// 关
		}
		bl_toggle_index ++;
		if(bl_toggle_index >= bl_toggle_count)
			bl_toggle_index = 0;
	}
}
#endif

// 背光初始化
// 使用GPIO模拟PWM
static void HW_LCD_BackLightInit (void)
{
#ifdef PWM_BL
	// pad_ctl3
	//                                    0          1              2          3            
	// [2:0]	ituclk_in_c	ituclk_in_c_pad	GPIO30	ituclk_in_c	mac_rx_dv	pwm_out[0]
	unsigned int val;
	unsigned int bl_freq;	// 背光PWM频率
	
	XM_lock ();	
	
	val = rSYS_PAD_CTRL03;
	val &= ~(0x07 << 0);
	val |=  (0x03 << 0);
	rSYS_PAD_CTRL03 = val;
		
   rSYS_SOFT_RSTNB &= ~(1<<0);
   __asm ("nop");
   __asm ("nop");
   rSYS_SOFT_RSTNB |= (1<<0);
   __asm ("nop");
   __asm ("nop");
	
	// 13	R/W	1	pwm_pclk enable
	//				1: enable
	//				0: disable
	rSYS_APB_CLK_EN |= (1 << 13);
	
	// 14	R/W	1	pwm_clk enable
	//					1: enable
	//					0: disable
	rSYS_PER_CLK_EN |= (1 << 14);
	
	bl_freq = 1000;
	int div = arkn141_get_clks(ARKN141_CLK_APB) / bl_freq;
	int duty = div / 50;	// 50% duty
	int cycle = 0x7FFFFFFF;
	int pwm_ena = 1;
	int polarity = 1; 
	int intr_en = 0;		// 不产生中断
	int pwm_activate = 1;
	int cmd = 	( pwm_ena)
				|	( pwm_activate << 2 )
				|	( intr_en << 3) 
				|	( polarity << 4)
				;
	PWM_CFG (0, cmd, div, duty, cycle);
	PWM_OnOff (0, 1);
	
	XM_unlock();
	
#else
	// pad_ctl2
	//                                           0           1           2          3
	// [26:24]	itu_b_din10	itu_b_din10_pad	GPIO28	itu_b_din[10]	mac_rxd[3]	pwm_out[7]
	// 配置为GPIO输出
	unsigned int val;
	XM_lock();
	val = rSYS_PAD_CTRL02;
	val &= ~(0x07 << 24);
	rSYS_PAD_CTRL02 = val;
	XM_unlock();
		
	XM_lock ();	
	SetGPIOPadDirection (LCD_BL_GPIO, euOutputPad);
	SetGPIOPadData (LCD_BL_GPIO, euDataLow);
	XM_unlock();	
	
	bl_toggle_count = 0;
	bl_toggle_index = 0;	
	OS_TICK_AddHook (&bl_tick_hook, bl_tick_func);	
#endif
}

// 开启背光
void HW_LCD_BackLightOn (void)
{
	// 根据背光值设置
	float level = HW_lcdc_get_backlight_on (0);
	HW_LCD_BackLightSetLevel (level);
}

// 关闭背光
void HW_LCD_BackLightOff (void)
{
#ifdef PWM_BL
	
#else
	XM_lock ();	
	rGPIO_PA_RDATA &= ~(1<<28);
	bl_toggle_index = 0;
	bl_toggle_count = 0;
	XM_unlock();
#endif
}

// 背光ON/OFF状态切换
void HW_LCD_BackLightToggleOnOff (void)
{
#ifdef PWM_BL
	
#else
	if(bl_toggle_count)	// off状态
	{
		HW_LCD_BackLightOff ();
	}
	else
	{
		HW_LCD_BackLightOn ();
	}
#endif
}

// 设置背光亮度级别 (0.0 ~ 1.0)
void HW_LCD_BackLightSetLevel (float level)
{
#ifdef PWM_BL
#else
	unsigned int val;
	XM_lock();
	val = rSYS_PAD_CTRL02;
	val &= ~(0x07 << 24);
	rSYS_PAD_CTRL02 = val;
	XM_unlock();
	
	int index = (int)((1.0 - level) * 4);
	if(index < 0)
		index = 0;
	else if(index > 4)
		index = 4;
	
	//index = 3;
	
	XM_lock();
	bl_toggle_count = bl_level_count[index];
	bl_toggle_index = 0;
	XM_unlock();
#endif
}

void	CS_H (void)
{
	XM_lock ();
	SetGPIOPadData (LCD_CS_GPIO, euDataHigh);
	XM_unlock ();
}

void	CS_L (void)
{
	XM_lock ();
	SetGPIOPadData (LCD_CS_GPIO, euDataLow);
	XM_unlock ();
}

void RESET_H (void)
{
	XM_lock ();
	SetGPIOPadData (LCD_RESET_GPIO, euDataHigh);
	XM_unlock ();
}

void RESET_L (void)
{
	XM_lock ();
	SetGPIOPadData (LCD_RESET_GPIO, euDataLow);
	XM_unlock ();
}

void SDA_H (void)
{
	XM_lock ();
	SetGPIOPadData (LCD_SDA_GPIO, euDataHigh);
	XM_unlock ();
}

void SDA_L (void)
{
	XM_lock ();
	SetGPIOPadData (LCD_SDA_GPIO, euDataLow);
	XM_unlock ();
}

void SCL_H (void)
{
	XM_lock ();
	SetGPIOPadData (LCD_SCL_GPIO, euDataHigh);
	XM_unlock ();
}

void SCL_L (void)
{
	XM_lock ();
	SetGPIOPadData (LCD_SCL_GPIO, euDataLow);
	XM_unlock ();
}




void Delay_us (int n)
{
	// 250,000,000	
	XMINT64 ticket = XM_GetHighResolutionTickCount() + 1;
	while (1)
	{
		//for (int i = 0; i < 250/4; i ++);
		//n --;
		if(XM_GetHighResolutionTickCount() >= ticket)
			break;
	}
}

//35 000 000 = 35 M
static void Delay1ms(void)
{
	//Delay_us (1000);
	OS_Delay(1);
}


static void Delayms(int n){
	//OS_Delay (n);
	int val=n;
	while(val--)
		Delay1ms();
}

static void DelayMS (int n)
{
	//OS_Delay (n);
	Delayms (n);
}


static void Set_RST(unsigned long index)
{
	if(index) RESET_H();
	else      RESET_L();
}

static void Set_CS(unsigned long index)
{
	if(index) CS_H();
	else      CS_L();	
}

static void Set_SCL(unsigned long index)
{
	if(index)  SCL_H();
	else       SCL_L();
}

static void Set_SDI(unsigned long index)
{
	if(index) SDA_H();
	else      SDA_L();	
}

static void SPI_3W_wrByte(unsigned char cmd)
{
	unsigned long kk;	


	Set_SCL(0);
	for(kk=0;kk<8;kk++)
	{
		if(cmd&0x80) Set_SDI(1);
		else         Set_SDI(0);
    	cmd<<=1;
		Set_SCL(0);	Delay_us(5);
	
		Set_SCL(1);	Delay_us(5);
	//	cmd = cmd<<1;	
	}
}


static void SPI_WriteData(unsigned char value)
{
	unsigned int i;
	unsigned int dat;
	dat =0x40;
	Set_CS(0);
	Delay_us(20);
	for(i=0;i<8;i++)
	{
		Delay_us(100);
		Set_SCL(0);
		Delay_us(100);
		if(dat & 0x80)
			Set_SDI(1);
		else
			Set_SDI(0);
		Delay_us(100);
		Set_SCL(1);
		Delay_us(100);
		dat <<= 1;
	}
	dat = value;
	for(i=0;i<8;i++)
	{
		Delay_us(100);
		Set_SCL(0);
		Delay_us(100);
		if(dat& 0x80)
			Set_SDI(1);
		else
			Set_SDI(0);
		Delay_us(100);
		Set_SCL(1);
		Delay_us(100);
		dat <<= 1;
	}
	Set_CS(1);
	Delay_us(200);
	
}
	
static void SPI_WriteCom(unsigned int value)
{
	unsigned int  i;
	unsigned int  dat;
	Set_CS(0);
	dat = 0x20;
	for(i=0;i<8;i++)
	{
		Delay_us(100);
		Set_SCL(0);
		Delay_us(100);
		if(dat & 0x80)
			Set_SDI(1);
		else
			Set_SDI(0);

		Delay_us(100);
		Set_SCL(1);	
		Delay_us(100);
		dat <<= 1;
	}

	dat = value >> 8;
	for(i=0;i<8;i++)
	{
		Delay_us(100);
		Set_SCL(0);
		Delay_us(100);
		if(dat & 0x80)
			Set_SDI(1);
		else
			Set_SDI(0);

		Delay_us(100);
		Set_SCL(1);	
		Delay_us(100);
		dat <<= 1;
	}
	
	dat = 0x00;
	for(i=0;i<8;i++)
	{
		Delay_us(100);
		Set_SCL(0);
		Delay_us(100);
		if(dat & 0x80)
			Set_SDI(1);
		else
			Set_SDI(0);

		Delay_us(100);
		Set_SCL(1);	
		Delay_us(100);
		dat <<= 1;
	}

	dat = value;
	for(i=0;i<8;i++)
	{
		Delay_us(100);
		Set_SCL(0);
		Delay_us(100);
		if(dat & 0x80)
			Set_SDI(1);
		else
			Set_SDI(0);

		Delay_us(100);
		Set_SCL(1);	
		Delay_us(100);
		dat <<= 1;
	}
	Set_CS(1);
	Delay_us(200);
}

//#define	NEW
static void pannel_spi_write (void)
{
	
#ifdef NEW
	
SPI_WriteCom(0xF000); SPI_WriteData(0x55);
SPI_WriteCom(0xF001); SPI_WriteData(0xAA);
SPI_WriteCom(0xF002); SPI_WriteData(0x52);
SPI_WriteCom(0xF003); SPI_WriteData(0x08);
SPI_WriteCom(0xF004); SPI_WriteData(0x02);

SPI_WriteCom(0xF600); SPI_WriteData(0x60);
SPI_WriteCom(0xF601); SPI_WriteData(0x40);

SPI_WriteCom(0xFE00); SPI_WriteData(0x01);
SPI_WriteCom(0xFE01); SPI_WriteData(0x80);
SPI_WriteCom(0xFE02); SPI_WriteData(0x09);
SPI_WriteCom(0xFE03); SPI_WriteData(0x09);

SPI_WriteCom(0xF000); SPI_WriteData(0x55);
SPI_WriteCom(0xF001); SPI_WriteData(0xAA);
SPI_WriteCom(0xF002); SPI_WriteData(0x52);
SPI_WriteCom(0xF003); SPI_WriteData(0x08);
SPI_WriteCom(0xF004); SPI_WriteData(0x01);

SPI_WriteCom(0xB000); SPI_WriteData(0x07);

SPI_WriteCom(0xB100); SPI_WriteData(0x07);

SPI_WriteCom(0xB500); SPI_WriteData(0x08);

SPI_WriteCom(0xB600); SPI_WriteData(0x44);

SPI_WriteCom(0xB700); SPI_WriteData(0x34);

SPI_WriteCom(0xB800); SPI_WriteData(0x24);

SPI_WriteCom(0xB900); SPI_WriteData(0x34);

SPI_WriteCom(0xBA00); SPI_WriteData(0x14);

SPI_WriteCom(0xBC00); SPI_WriteData(0x00);
SPI_WriteCom(0xBC01); SPI_WriteData(0x78);
SPI_WriteCom(0xBC02); SPI_WriteData(0x13);

SPI_WriteCom(0xBD00); SPI_WriteData(0x00);
SPI_WriteCom(0xBD01); SPI_WriteData(0x78);
SPI_WriteCom(0xBD02); SPI_WriteData(0x13);

SPI_WriteCom(0xBE00); SPI_WriteData(0x00);
SPI_WriteCom(0xBE01); SPI_WriteData(0x15);

SPI_WriteCom(0xD100); SPI_WriteData(0x00);
SPI_WriteCom(0xD101); SPI_WriteData(0x00);
SPI_WriteCom(0xD102); SPI_WriteData(0x00);
SPI_WriteCom(0xD103); SPI_WriteData(0x17);
SPI_WriteCom(0xD104); SPI_WriteData(0x00);
SPI_WriteCom(0xD105); SPI_WriteData(0x3F);
SPI_WriteCom(0xD106); SPI_WriteData(0x00);
SPI_WriteCom(0xD107); SPI_WriteData(0x5E);
SPI_WriteCom(0xD108); SPI_WriteData(0x00);
SPI_WriteCom(0xD109); SPI_WriteData(0x7A);
SPI_WriteCom(0xD10A); SPI_WriteData(0x00);
SPI_WriteCom(0xD10B); SPI_WriteData(0xAA);
SPI_WriteCom(0xD10C); SPI_WriteData(0x00);
SPI_WriteCom(0xD10D); SPI_WriteData(0xD1);
SPI_WriteCom(0xD10E); SPI_WriteData(0x01);
SPI_WriteCom(0xD10F); SPI_WriteData(0x10);
SPI_WriteCom(0xD110); SPI_WriteData(0x01);
SPI_WriteCom(0xD111); SPI_WriteData(0x41);
SPI_WriteCom(0xD112); SPI_WriteData(0x01);
SPI_WriteCom(0xD113); SPI_WriteData(0x8A);
SPI_WriteCom(0xD114); SPI_WriteData(0x01);
SPI_WriteCom(0xD115); SPI_WriteData(0xC2);
SPI_WriteCom(0xD116); SPI_WriteData(0x02);
SPI_WriteCom(0xD117); SPI_WriteData(0x13);
SPI_WriteCom(0xD118); SPI_WriteData(0x02);
SPI_WriteCom(0xD119); SPI_WriteData(0x54);
SPI_WriteCom(0xD11A); SPI_WriteData(0x02);
SPI_WriteCom(0xD11B); SPI_WriteData(0x56);
SPI_WriteCom(0xD11C); SPI_WriteData(0x02);
SPI_WriteCom(0xD11D); SPI_WriteData(0x92);
SPI_WriteCom(0xD11E); SPI_WriteData(0x02);
SPI_WriteCom(0xD11F); SPI_WriteData(0xD8);
SPI_WriteCom(0xD120); SPI_WriteData(0x03);
SPI_WriteCom(0xD121); SPI_WriteData(0x07);
SPI_WriteCom(0xD122); SPI_WriteData(0x03);
SPI_WriteCom(0xD123); SPI_WriteData(0x46);
SPI_WriteCom(0xD124); SPI_WriteData(0x03);
SPI_WriteCom(0xD125); SPI_WriteData(0x70);
SPI_WriteCom(0xD126); SPI_WriteData(0x03);
SPI_WriteCom(0xD127); SPI_WriteData(0xA4);
SPI_WriteCom(0xD128); SPI_WriteData(0x03);
SPI_WriteCom(0xD129); SPI_WriteData(0xC1);
SPI_WriteCom(0xD12A); SPI_WriteData(0x03);
SPI_WriteCom(0xD12B); SPI_WriteData(0xE0);
SPI_WriteCom(0xD12C); SPI_WriteData(0x03);
SPI_WriteCom(0xD12D); SPI_WriteData(0xEC);
SPI_WriteCom(0xD12E); SPI_WriteData(0x03);
SPI_WriteCom(0xD12F); SPI_WriteData(0xF5);
SPI_WriteCom(0xD130); SPI_WriteData(0x03);
SPI_WriteCom(0xD131); SPI_WriteData(0xFA);
SPI_WriteCom(0xD132); SPI_WriteData(0x03);
SPI_WriteCom(0xD133); SPI_WriteData(0xFF);

SPI_WriteCom(0xD200); SPI_WriteData(0x00);
SPI_WriteCom(0xD201); SPI_WriteData(0x00);
SPI_WriteCom(0xD202); SPI_WriteData(0x00);
SPI_WriteCom(0xD203); SPI_WriteData(0x17);
SPI_WriteCom(0xD204); SPI_WriteData(0x00);
SPI_WriteCom(0xD205); SPI_WriteData(0x3F);
SPI_WriteCom(0xD206); SPI_WriteData(0x00);
SPI_WriteCom(0xD207); SPI_WriteData(0x5E);
SPI_WriteCom(0xD208); SPI_WriteData(0x00);
SPI_WriteCom(0xD209); SPI_WriteData(0x7A);
SPI_WriteCom(0xD20A); SPI_WriteData(0x00);
SPI_WriteCom(0xD20B); SPI_WriteData(0xAA);
SPI_WriteCom(0xD20C); SPI_WriteData(0x00);
SPI_WriteCom(0xD20D); SPI_WriteData(0xD1);
SPI_WriteCom(0xD20E); SPI_WriteData(0x01);
SPI_WriteCom(0xD20F); SPI_WriteData(0x10);
SPI_WriteCom(0xD210); SPI_WriteData(0x01);
SPI_WriteCom(0xD211); SPI_WriteData(0x41);
SPI_WriteCom(0xD212); SPI_WriteData(0x01);
SPI_WriteCom(0xD213); SPI_WriteData(0x8A);
SPI_WriteCom(0xD214); SPI_WriteData(0x01);
SPI_WriteCom(0xD215); SPI_WriteData(0xC2);
SPI_WriteCom(0xD216); SPI_WriteData(0x02);
SPI_WriteCom(0xD217); SPI_WriteData(0x13);
SPI_WriteCom(0xD218); SPI_WriteData(0x02);
SPI_WriteCom(0xD219); SPI_WriteData(0x54);
SPI_WriteCom(0xD21A); SPI_WriteData(0x02);
SPI_WriteCom(0xD21B); SPI_WriteData(0x56);
SPI_WriteCom(0xD21C); SPI_WriteData(0x02);
SPI_WriteCom(0xD21D); SPI_WriteData(0x92);
SPI_WriteCom(0xD21E); SPI_WriteData(0x02);
SPI_WriteCom(0xD21F); SPI_WriteData(0xD8);
SPI_WriteCom(0xD220); SPI_WriteData(0x03);
SPI_WriteCom(0xD221); SPI_WriteData(0x07);
SPI_WriteCom(0xD222); SPI_WriteData(0x03);
SPI_WriteCom(0xD223); SPI_WriteData(0x46);
SPI_WriteCom(0xD224); SPI_WriteData(0x03);
SPI_WriteCom(0xD225); SPI_WriteData(0x70);
SPI_WriteCom(0xD226); SPI_WriteData(0x03);
SPI_WriteCom(0xD227); SPI_WriteData(0xA4);
SPI_WriteCom(0xD228); SPI_WriteData(0x03);
SPI_WriteCom(0xD229); SPI_WriteData(0xC1);
SPI_WriteCom(0xD22A); SPI_WriteData(0x03);
SPI_WriteCom(0xD22B); SPI_WriteData(0xE0);
SPI_WriteCom(0xD22C); SPI_WriteData(0x03);
SPI_WriteCom(0xD22D); SPI_WriteData(0xEC);
SPI_WriteCom(0xD22E); SPI_WriteData(0x03);
SPI_WriteCom(0xD22F); SPI_WriteData(0xF5);
SPI_WriteCom(0xD230); SPI_WriteData(0x03);
SPI_WriteCom(0xD231); SPI_WriteData(0xFA);
SPI_WriteCom(0xD232); SPI_WriteData(0x03);
SPI_WriteCom(0xD233); SPI_WriteData(0xFF);

SPI_WriteCom(0xD300); SPI_WriteData(0x00);
SPI_WriteCom(0xD301); SPI_WriteData(0x00);
SPI_WriteCom(0xD302); SPI_WriteData(0x00);
SPI_WriteCom(0xD303); SPI_WriteData(0x17);
SPI_WriteCom(0xD304); SPI_WriteData(0x00);
SPI_WriteCom(0xD305); SPI_WriteData(0x3F);
SPI_WriteCom(0xD306); SPI_WriteData(0x00);
SPI_WriteCom(0xD307); SPI_WriteData(0x5E);
SPI_WriteCom(0xD308); SPI_WriteData(0x00);
SPI_WriteCom(0xD309); SPI_WriteData(0x7A);
SPI_WriteCom(0xD30A); SPI_WriteData(0x00);
SPI_WriteCom(0xD30B); SPI_WriteData(0xAA);
SPI_WriteCom(0xD30C); SPI_WriteData(0x00);
SPI_WriteCom(0xD30D); SPI_WriteData(0xD1);
SPI_WriteCom(0xD30E); SPI_WriteData(0x01);
SPI_WriteCom(0xD30F); SPI_WriteData(0x10);
SPI_WriteCom(0xD310); SPI_WriteData(0x01);
SPI_WriteCom(0xD311); SPI_WriteData(0x41);
SPI_WriteCom(0xD312); SPI_WriteData(0x01);
SPI_WriteCom(0xD313); SPI_WriteData(0x8A);
SPI_WriteCom(0xD314); SPI_WriteData(0x01);
SPI_WriteCom(0xD315); SPI_WriteData(0xC2);
SPI_WriteCom(0xD316); SPI_WriteData(0x02);
SPI_WriteCom(0xD317); SPI_WriteData(0x13);
SPI_WriteCom(0xD318); SPI_WriteData(0x02);
SPI_WriteCom(0xD319); SPI_WriteData(0x54);
SPI_WriteCom(0xD31A); SPI_WriteData(0x02);
SPI_WriteCom(0xD31B); SPI_WriteData(0x56);
SPI_WriteCom(0xD31C); SPI_WriteData(0x02);
SPI_WriteCom(0xD31D); SPI_WriteData(0x92);
SPI_WriteCom(0xD31E); SPI_WriteData(0x02);
SPI_WriteCom(0xD31F); SPI_WriteData(0xD8);
SPI_WriteCom(0xD320); SPI_WriteData(0x03);
SPI_WriteCom(0xD321); SPI_WriteData(0x07);
SPI_WriteCom(0xD322); SPI_WriteData(0x03);
SPI_WriteCom(0xD323); SPI_WriteData(0x46);
SPI_WriteCom(0xD324); SPI_WriteData(0x03);
SPI_WriteCom(0xD325); SPI_WriteData(0x70);
SPI_WriteCom(0xD326); SPI_WriteData(0x03);
SPI_WriteCom(0xD327); SPI_WriteData(0xA4);
SPI_WriteCom(0xD328); SPI_WriteData(0x03);
SPI_WriteCom(0xD329); SPI_WriteData(0xC1);
SPI_WriteCom(0xD32A); SPI_WriteData(0x03);
SPI_WriteCom(0xD32B); SPI_WriteData(0xE0);
SPI_WriteCom(0xD32C); SPI_WriteData(0x03);
SPI_WriteCom(0xD32D); SPI_WriteData(0xEC);
SPI_WriteCom(0xD32E); SPI_WriteData(0x03);
SPI_WriteCom(0xD32F); SPI_WriteData(0xF5);
SPI_WriteCom(0xD330); SPI_WriteData(0x03);
SPI_WriteCom(0xD331); SPI_WriteData(0xFA);
SPI_WriteCom(0xD332); SPI_WriteData(0x03);
SPI_WriteCom(0xD333); SPI_WriteData(0xFF);

SPI_WriteCom(0xD400); SPI_WriteData(0x00);
SPI_WriteCom(0xD401); SPI_WriteData(0x00);
SPI_WriteCom(0xD402); SPI_WriteData(0x00);
SPI_WriteCom(0xD403); SPI_WriteData(0x17);
SPI_WriteCom(0xD404); SPI_WriteData(0x00);
SPI_WriteCom(0xD405); SPI_WriteData(0x3F);
SPI_WriteCom(0xD406); SPI_WriteData(0x00);
SPI_WriteCom(0xD407); SPI_WriteData(0x5E);
SPI_WriteCom(0xD408); SPI_WriteData(0x00);
SPI_WriteCom(0xD409); SPI_WriteData(0x7A);
SPI_WriteCom(0xD40A); SPI_WriteData(0x00);
SPI_WriteCom(0xD40B); SPI_WriteData(0xAA);
SPI_WriteCom(0xD40C); SPI_WriteData(0x00);
SPI_WriteCom(0xD40D); SPI_WriteData(0xD1);
SPI_WriteCom(0xD40E); SPI_WriteData(0x01);
SPI_WriteCom(0xD40F); SPI_WriteData(0x10);
SPI_WriteCom(0xD410); SPI_WriteData(0x01);
SPI_WriteCom(0xD411); SPI_WriteData(0x41);
SPI_WriteCom(0xD412); SPI_WriteData(0x01);
SPI_WriteCom(0xD413); SPI_WriteData(0x8A);
SPI_WriteCom(0xD414); SPI_WriteData(0x01);
SPI_WriteCom(0xD415); SPI_WriteData(0xC2);
SPI_WriteCom(0xD416); SPI_WriteData(0x02);
SPI_WriteCom(0xD417); SPI_WriteData(0x13);
SPI_WriteCom(0xD418); SPI_WriteData(0x02);
SPI_WriteCom(0xD419); SPI_WriteData(0x54);
SPI_WriteCom(0xD41A); SPI_WriteData(0x02);
SPI_WriteCom(0xD41B); SPI_WriteData(0x56);
SPI_WriteCom(0xD41C); SPI_WriteData(0x02);
SPI_WriteCom(0xD41D); SPI_WriteData(0x92);
SPI_WriteCom(0xD41E); SPI_WriteData(0x02);
SPI_WriteCom(0xD41F); SPI_WriteData(0xD8);
SPI_WriteCom(0xD420); SPI_WriteData(0x03);
SPI_WriteCom(0xD421); SPI_WriteData(0x07);
SPI_WriteCom(0xD422); SPI_WriteData(0x03);
SPI_WriteCom(0xD423); SPI_WriteData(0x46);
SPI_WriteCom(0xD424); SPI_WriteData(0x03);
SPI_WriteCom(0xD425); SPI_WriteData(0x70);
SPI_WriteCom(0xD426); SPI_WriteData(0x03);
SPI_WriteCom(0xD427); SPI_WriteData(0xA4);
SPI_WriteCom(0xD428); SPI_WriteData(0x03);
SPI_WriteCom(0xD429); SPI_WriteData(0xC1);
SPI_WriteCom(0xD42A); SPI_WriteData(0x03);
SPI_WriteCom(0xD42B); SPI_WriteData(0xE0);
SPI_WriteCom(0xD42C); SPI_WriteData(0x03);
SPI_WriteCom(0xD42D); SPI_WriteData(0xEC);
SPI_WriteCom(0xD42E); SPI_WriteData(0x03);
SPI_WriteCom(0xD42F); SPI_WriteData(0xF5);
SPI_WriteCom(0xD430); SPI_WriteData(0x03);
SPI_WriteCom(0xD431); SPI_WriteData(0xFA);
SPI_WriteCom(0xD432); SPI_WriteData(0x03);
SPI_WriteCom(0xD433); SPI_WriteData(0xFF);

SPI_WriteCom(0xD500); SPI_WriteData(0x00);
SPI_WriteCom(0xD501); SPI_WriteData(0x00);
SPI_WriteCom(0xD502); SPI_WriteData(0x00);
SPI_WriteCom(0xD503); SPI_WriteData(0x17);
SPI_WriteCom(0xD504); SPI_WriteData(0x00);
SPI_WriteCom(0xD505); SPI_WriteData(0x3F);
SPI_WriteCom(0xD506); SPI_WriteData(0x00);
SPI_WriteCom(0xD507); SPI_WriteData(0x5E);
SPI_WriteCom(0xD508); SPI_WriteData(0x00);
SPI_WriteCom(0xD509); SPI_WriteData(0x7A);
SPI_WriteCom(0xD50A); SPI_WriteData(0x00);
SPI_WriteCom(0xD50B); SPI_WriteData(0xAA);
SPI_WriteCom(0xD50C); SPI_WriteData(0x00);
SPI_WriteCom(0xD50D); SPI_WriteData(0xD1);
SPI_WriteCom(0xD50E); SPI_WriteData(0x01);
SPI_WriteCom(0xD50F); SPI_WriteData(0x10);
SPI_WriteCom(0xD510); SPI_WriteData(0x01);
SPI_WriteCom(0xD511); SPI_WriteData(0x41);
SPI_WriteCom(0xD512); SPI_WriteData(0x01);
SPI_WriteCom(0xD513); SPI_WriteData(0x8A);
SPI_WriteCom(0xD514); SPI_WriteData(0x01);
SPI_WriteCom(0xD515); SPI_WriteData(0xC2);
SPI_WriteCom(0xD516); SPI_WriteData(0x02);
SPI_WriteCom(0xD517); SPI_WriteData(0x13);
SPI_WriteCom(0xD518); SPI_WriteData(0x02);
SPI_WriteCom(0xD519); SPI_WriteData(0x54);
SPI_WriteCom(0xD51A); SPI_WriteData(0x02);
SPI_WriteCom(0xD51B); SPI_WriteData(0x56);
SPI_WriteCom(0xD51C); SPI_WriteData(0x02);
SPI_WriteCom(0xD51D); SPI_WriteData(0x92);
SPI_WriteCom(0xD51E); SPI_WriteData(0x02);
SPI_WriteCom(0xD51F); SPI_WriteData(0xD8);
SPI_WriteCom(0xD520); SPI_WriteData(0x03);
SPI_WriteCom(0xD521); SPI_WriteData(0x07);
SPI_WriteCom(0xD522); SPI_WriteData(0x03);
SPI_WriteCom(0xD523); SPI_WriteData(0x46);
SPI_WriteCom(0xD524); SPI_WriteData(0x03);
SPI_WriteCom(0xD525); SPI_WriteData(0x70);
SPI_WriteCom(0xD526); SPI_WriteData(0x03);
SPI_WriteCom(0xD527); SPI_WriteData(0xA4);
SPI_WriteCom(0xD528); SPI_WriteData(0x03);
SPI_WriteCom(0xD529); SPI_WriteData(0xC1);
SPI_WriteCom(0xD52A); SPI_WriteData(0x03);
SPI_WriteCom(0xD52B); SPI_WriteData(0xE0);
SPI_WriteCom(0xD52C); SPI_WriteData(0x03);
SPI_WriteCom(0xD52D); SPI_WriteData(0xEC);
SPI_WriteCom(0xD52E); SPI_WriteData(0x03);
SPI_WriteCom(0xD52F); SPI_WriteData(0xF5);
SPI_WriteCom(0xD530); SPI_WriteData(0x03);
SPI_WriteCom(0xD531); SPI_WriteData(0xFA);
SPI_WriteCom(0xD532); SPI_WriteData(0x03);
SPI_WriteCom(0xD533); SPI_WriteData(0xFF);

SPI_WriteCom(0xD600); SPI_WriteData(0x00);
SPI_WriteCom(0xD601); SPI_WriteData(0x00);
SPI_WriteCom(0xD602); SPI_WriteData(0x00);
SPI_WriteCom(0xD603); SPI_WriteData(0x17);
SPI_WriteCom(0xD604); SPI_WriteData(0x00);
SPI_WriteCom(0xD605); SPI_WriteData(0x3F);
SPI_WriteCom(0xD606); SPI_WriteData(0x00);
SPI_WriteCom(0xD607); SPI_WriteData(0x5E);
SPI_WriteCom(0xD608); SPI_WriteData(0x00);
SPI_WriteCom(0xD609); SPI_WriteData(0x7A);
SPI_WriteCom(0xD60A); SPI_WriteData(0x00);
SPI_WriteCom(0xD60B); SPI_WriteData(0xAA);
SPI_WriteCom(0xD60C); SPI_WriteData(0x00);
SPI_WriteCom(0xD60D); SPI_WriteData(0xD1);
SPI_WriteCom(0xD60E); SPI_WriteData(0x01);
SPI_WriteCom(0xD60F); SPI_WriteData(0x10);
SPI_WriteCom(0xD610); SPI_WriteData(0x01);
SPI_WriteCom(0xD611); SPI_WriteData(0x41);
SPI_WriteCom(0xD612); SPI_WriteData(0x01);
SPI_WriteCom(0xD613); SPI_WriteData(0x8A);
SPI_WriteCom(0xD614); SPI_WriteData(0x01);
SPI_WriteCom(0xD615); SPI_WriteData(0xC2);
SPI_WriteCom(0xD616); SPI_WriteData(0x02);
SPI_WriteCom(0xD617); SPI_WriteData(0x13);
SPI_WriteCom(0xD618); SPI_WriteData(0x02);
SPI_WriteCom(0xD619); SPI_WriteData(0x54);
SPI_WriteCom(0xD61A); SPI_WriteData(0x02);
SPI_WriteCom(0xD61B); SPI_WriteData(0x56);
SPI_WriteCom(0xD61C); SPI_WriteData(0x02);
SPI_WriteCom(0xD61D); SPI_WriteData(0x92);
SPI_WriteCom(0xD61E); SPI_WriteData(0x02);
SPI_WriteCom(0xD61F); SPI_WriteData(0xD8);
SPI_WriteCom(0xD620); SPI_WriteData(0x03);
SPI_WriteCom(0xD621); SPI_WriteData(0x07);
SPI_WriteCom(0xD622); SPI_WriteData(0x03);
SPI_WriteCom(0xD623); SPI_WriteData(0x46);
SPI_WriteCom(0xD624); SPI_WriteData(0x03);
SPI_WriteCom(0xD625); SPI_WriteData(0x70);
SPI_WriteCom(0xD626); SPI_WriteData(0x03);
SPI_WriteCom(0xD627); SPI_WriteData(0xA4);
SPI_WriteCom(0xD628); SPI_WriteData(0x03);
SPI_WriteCom(0xD629); SPI_WriteData(0xC1);
SPI_WriteCom(0xD62A); SPI_WriteData(0x03);
SPI_WriteCom(0xD62B); SPI_WriteData(0xE0);
SPI_WriteCom(0xD62C); SPI_WriteData(0x03);
SPI_WriteCom(0xD62D); SPI_WriteData(0xEC);
SPI_WriteCom(0xD62E); SPI_WriteData(0x03);
SPI_WriteCom(0xD62F); SPI_WriteData(0xF5);
SPI_WriteCom(0xD630); SPI_WriteData(0x03);
SPI_WriteCom(0xD631); SPI_WriteData(0xFA);
SPI_WriteCom(0xD632); SPI_WriteData(0x03);
SPI_WriteCom(0xD633); SPI_WriteData(0xFF);

SPI_WriteCom(0xF000); SPI_WriteData(0x55);
SPI_WriteCom(0xF001); SPI_WriteData(0xAA);
SPI_WriteCom(0xF002); SPI_WriteData(0x52);
SPI_WriteCom(0xF003); SPI_WriteData(0x08);
SPI_WriteCom(0xF004); SPI_WriteData(0x03);

SPI_WriteCom(0xB000); SPI_WriteData(0x05);
SPI_WriteCom(0xB001); SPI_WriteData(0x17);
SPI_WriteCom(0xB002); SPI_WriteData(0xF9);
SPI_WriteCom(0xB003); SPI_WriteData(0x53);
SPI_WriteCom(0xB004); SPI_WriteData(0x53);
SPI_WriteCom(0xB005); SPI_WriteData(0x00);
SPI_WriteCom(0xB006); SPI_WriteData(0x30);

SPI_WriteCom(0xB100); SPI_WriteData(0x05);
SPI_WriteCom(0xB101); SPI_WriteData(0x17);
SPI_WriteCom(0xB102); SPI_WriteData(0xFB);
SPI_WriteCom(0xB103); SPI_WriteData(0x55);
SPI_WriteCom(0xB104); SPI_WriteData(0x53);
SPI_WriteCom(0xB105); SPI_WriteData(0x00);
SPI_WriteCom(0xB106); SPI_WriteData(0x30);

SPI_WriteCom(0xB200); SPI_WriteData(0xFB);
SPI_WriteCom(0xB201); SPI_WriteData(0xFC);
SPI_WriteCom(0xB202); SPI_WriteData(0xFD);
SPI_WriteCom(0xB203); SPI_WriteData(0xFE);
SPI_WriteCom(0xB204); SPI_WriteData(0xF0);
SPI_WriteCom(0xB205); SPI_WriteData(0x53);
SPI_WriteCom(0xB206); SPI_WriteData(0x00);
SPI_WriteCom(0xB207); SPI_WriteData(0xC5);
SPI_WriteCom(0xB208); SPI_WriteData(0x08);

SPI_WriteCom(0xB300); SPI_WriteData(0x5B);
SPI_WriteCom(0xB301); SPI_WriteData(0x00);
SPI_WriteCom(0xB302); SPI_WriteData(0xFB);
SPI_WriteCom(0xB303); SPI_WriteData(0x5A);
SPI_WriteCom(0xB304); SPI_WriteData(0x5A);
SPI_WriteCom(0xB305); SPI_WriteData(0x0C);

SPI_WriteCom(0xB400); SPI_WriteData(0xFF);
SPI_WriteCom(0xB401); SPI_WriteData(0x00);
SPI_WriteCom(0xB402); SPI_WriteData(0x01);
SPI_WriteCom(0xB403); SPI_WriteData(0x02);
SPI_WriteCom(0xB404); SPI_WriteData(0xC0);
SPI_WriteCom(0xB405); SPI_WriteData(0x40);
SPI_WriteCom(0xB406); SPI_WriteData(0x05);
SPI_WriteCom(0xB407); SPI_WriteData(0x08);
SPI_WriteCom(0xB408); SPI_WriteData(0x53);

SPI_WriteCom(0xB500); SPI_WriteData(0x00);
SPI_WriteCom(0xB501); SPI_WriteData(0x00);
SPI_WriteCom(0xB502); SPI_WriteData(0xFF);
SPI_WriteCom(0xB503); SPI_WriteData(0x83);
SPI_WriteCom(0xB504); SPI_WriteData(0x5F);
SPI_WriteCom(0xB505); SPI_WriteData(0x5E);
SPI_WriteCom(0xB506); SPI_WriteData(0x50);
SPI_WriteCom(0xB507); SPI_WriteData(0x50);
SPI_WriteCom(0xB508); SPI_WriteData(0x33);
SPI_WriteCom(0xB509); SPI_WriteData(0x33);
SPI_WriteCom(0xB50A); SPI_WriteData(0x55);

SPI_WriteCom(0xB600); SPI_WriteData(0xBC);
SPI_WriteCom(0xB601); SPI_WriteData(0x00);
SPI_WriteCom(0xB602); SPI_WriteData(0x00);
SPI_WriteCom(0xB603); SPI_WriteData(0x00);
SPI_WriteCom(0xB604); SPI_WriteData(0x2A);
SPI_WriteCom(0xB605); SPI_WriteData(0x80);
SPI_WriteCom(0xB606); SPI_WriteData(0x00);

SPI_WriteCom(0xB700); SPI_WriteData(0x00);
SPI_WriteCom(0xB701); SPI_WriteData(0x00);
SPI_WriteCom(0xB702); SPI_WriteData(0x00);
SPI_WriteCom(0xB703); SPI_WriteData(0x00);
SPI_WriteCom(0xB704); SPI_WriteData(0x00);
SPI_WriteCom(0xB705); SPI_WriteData(0x00);
SPI_WriteCom(0xB706); SPI_WriteData(0x00);
SPI_WriteCom(0xB707); SPI_WriteData(0x00);

SPI_WriteCom(0xB800); SPI_WriteData(0x11);
SPI_WriteCom(0xB801); SPI_WriteData(0x60);
SPI_WriteCom(0xB802); SPI_WriteData(0x00);

SPI_WriteCom(0xB900); SPI_WriteData(0x90);

SPI_WriteCom(0xBA00); SPI_WriteData(0x44);
SPI_WriteCom(0xBA01); SPI_WriteData(0x44);
SPI_WriteCom(0xBA02); SPI_WriteData(0x08);
SPI_WriteCom(0xBA03); SPI_WriteData(0xAC);
SPI_WriteCom(0xBA04); SPI_WriteData(0xE2);
SPI_WriteCom(0xBA05); SPI_WriteData(0x64);
SPI_WriteCom(0xBA06); SPI_WriteData(0x44);
SPI_WriteCom(0xBA07); SPI_WriteData(0x44);
SPI_WriteCom(0xBA08); SPI_WriteData(0x44);
SPI_WriteCom(0xBA09); SPI_WriteData(0x44);
SPI_WriteCom(0xBA0A); SPI_WriteData(0x47);
SPI_WriteCom(0xBA0B); SPI_WriteData(0x3F);
SPI_WriteCom(0xBA0C); SPI_WriteData(0xDB);
SPI_WriteCom(0xBA0D); SPI_WriteData(0x91);
SPI_WriteCom(0xBA0E); SPI_WriteData(0x54);
SPI_WriteCom(0xBA0F); SPI_WriteData(0x44);

SPI_WriteCom(0xBB00); SPI_WriteData(0x44);
SPI_WriteCom(0xBB01); SPI_WriteData(0x43);
SPI_WriteCom(0xBB02); SPI_WriteData(0x79);
SPI_WriteCom(0xBB03); SPI_WriteData(0xFD);
SPI_WriteCom(0xBB04); SPI_WriteData(0xB5);
SPI_WriteCom(0xBB05); SPI_WriteData(0x14);
SPI_WriteCom(0xBB06); SPI_WriteData(0x44);
SPI_WriteCom(0xBB07); SPI_WriteData(0x44);
SPI_WriteCom(0xBB08); SPI_WriteData(0x44);
SPI_WriteCom(0xBB09); SPI_WriteData(0x44);
SPI_WriteCom(0xBB0A); SPI_WriteData(0x40);
SPI_WriteCom(0xBB0B); SPI_WriteData(0x4A);
SPI_WriteCom(0xBB0C); SPI_WriteData(0xCE);
SPI_WriteCom(0xBB0D); SPI_WriteData(0x86);
SPI_WriteCom(0xBB0E); SPI_WriteData(0x24);
SPI_WriteCom(0xBB0F); SPI_WriteData(0x44);

SPI_WriteCom(0xBC00); SPI_WriteData(0xE0);
SPI_WriteCom(0xBC01); SPI_WriteData(0x1F);
SPI_WriteCom(0xBC02); SPI_WriteData(0xF8);
SPI_WriteCom(0xBC03); SPI_WriteData(0x07);

SPI_WriteCom(0xBD00); SPI_WriteData(0xE0);
SPI_WriteCom(0xBD01); SPI_WriteData(0x1F);
SPI_WriteCom(0xBD02); SPI_WriteData(0xF8);
SPI_WriteCom(0xBD03); SPI_WriteData(0x07);

SPI_WriteCom(0xF000); SPI_WriteData(0x55);
SPI_WriteCom(0xF001); SPI_WriteData(0xAA);
SPI_WriteCom(0xF002); SPI_WriteData(0x52);
SPI_WriteCom(0xF003); SPI_WriteData(0x08);
SPI_WriteCom(0xF004); SPI_WriteData(0x00);

SPI_WriteCom(0xB000); SPI_WriteData(0x00);
SPI_WriteCom(0xB001); SPI_WriteData(0x10);

SPI_WriteCom(0xB400); SPI_WriteData(0x10);

SPI_WriteCom(0xB500); SPI_WriteData(0x6B);

SPI_WriteCom(0xBC00); SPI_WriteData(0x00);

SPI_WriteCom(0x3600); SPI_WriteData(0x01);		// Vertical Flip


SPI_WriteCom(0x3500); SPI_WriteData(0x00);

SPI_WriteCom(0x1100);

DelayMS(120);

SPI_WriteCom(0x2900);

DelayMS(100);
	
	
#else
    /**************************************************
    IC Name: RM68172GA1
    Panel Maker/Size: IVO498
    Panel Product No.: BF050WVQ-100
    Version: CODE
    Date: V1.0_141222
    **************************************************/
   // SPI_writecom(0x20F0); SPI_writecom(0x0000);SPI_writedat(0x4055);
    
SPI_WriteCom(0xF000); SPI_WriteData(0x55);
SPI_WriteCom(0xF001); SPI_WriteData(0xAA);
SPI_WriteCom(0xF002); SPI_WriteData(0x52);
SPI_WriteCom(0xF003); SPI_WriteData(0x08);
SPI_WriteCom(0xF004); SPI_WriteData(0x02);

SPI_WriteCom(0xF600); SPI_WriteData(0x60);
SPI_WriteCom(0xF601); SPI_WriteData(0x40);

SPI_WriteCom(0xFE00); SPI_WriteData(0x01);
SPI_WriteCom(0xFE01); SPI_WriteData(0x80);
SPI_WriteCom(0xFE02); SPI_WriteData(0x09);
SPI_WriteCom(0xFE03); SPI_WriteData(0x09);


SPI_WriteCom(0xF000); SPI_WriteData(0x55);
SPI_WriteCom(0xF001); SPI_WriteData(0xAA);
SPI_WriteCom(0xF002); SPI_WriteData(0x52);
SPI_WriteCom(0xF003); SPI_WriteData(0x08);
SPI_WriteCom(0xF004); SPI_WriteData(0x01);

SPI_WriteCom(0xB000); SPI_WriteData(0x0a);

SPI_WriteCom(0xB100); SPI_WriteData(0x0a);

SPI_WriteCom(0xB500); SPI_WriteData(0x08);

SPI_WriteCom(0xB600); SPI_WriteData(0x54);

SPI_WriteCom(0xB700); SPI_WriteData(0x44);

SPI_WriteCom(0xB800); SPI_WriteData(0x24);

SPI_WriteCom(0xB900); SPI_WriteData(0x34);

SPI_WriteCom(0xBA00); SPI_WriteData(0x14);

SPI_WriteCom(0xBC00); SPI_WriteData(0x00);
SPI_WriteCom(0xBC01); SPI_WriteData(0xA8);
SPI_WriteCom(0xBC02); SPI_WriteData(0x13);

SPI_WriteCom(0xBD00); SPI_WriteData(0x00);
SPI_WriteCom(0xBD01); SPI_WriteData(0xA0);
SPI_WriteCom(0xBD02); SPI_WriteData(0x1a);

SPI_WriteCom(0xBE00); SPI_WriteData(0x00);
SPI_WriteCom(0xBE01); SPI_WriteData(0x1a);

 SPI_WriteCom(0xD100); SPI_WriteData(0x00);
   SPI_WriteCom(0xD101); SPI_WriteData(0x00);
   SPI_WriteCom(0xD102); SPI_WriteData(0x00);
   SPI_WriteCom(0xD103); SPI_WriteData(0x17);
   SPI_WriteCom(0xD104); SPI_WriteData(0x00);
   SPI_WriteCom(0xD105); SPI_WriteData(0x3E);
   SPI_WriteCom(0xD106); SPI_WriteData(0x00);
   SPI_WriteCom(0xD107); SPI_WriteData(0x5E);
   SPI_WriteCom(0xD108); SPI_WriteData(0x00);
   SPI_WriteCom(0xD109); SPI_WriteData(0x7B);
   SPI_WriteCom(0xD10A); SPI_WriteData(0x00);
   SPI_WriteCom(0xD10B); SPI_WriteData(0xA9);
   SPI_WriteCom(0xD10C); SPI_WriteData(0x00);
   SPI_WriteCom(0xD10D); SPI_WriteData(0xCE);
   SPI_WriteCom(0xD10E); SPI_WriteData(0x01);
   SPI_WriteCom(0xD10F); SPI_WriteData(0x0A);
   SPI_WriteCom(0xD110); SPI_WriteData(0x01);
   SPI_WriteCom(0xD111); SPI_WriteData(0x37);
   SPI_WriteCom(0xD112); SPI_WriteData(0x01);
   SPI_WriteCom(0xD113); SPI_WriteData(0x7C);
   SPI_WriteCom(0xD114); SPI_WriteData(0x01);
   SPI_WriteCom(0xD115); SPI_WriteData(0xB0);
   SPI_WriteCom(0xD116); SPI_WriteData(0x01);
   SPI_WriteCom(0xD117); SPI_WriteData(0xFF);
   SPI_WriteCom(0xD118); SPI_WriteData(0x02);
   SPI_WriteCom(0xD119); SPI_WriteData(0x3D);
   SPI_WriteCom(0xD11A); SPI_WriteData(0x02);
   SPI_WriteCom(0xD11B); SPI_WriteData(0x3F);
   SPI_WriteCom(0xD11C); SPI_WriteData(0x02);
   SPI_WriteCom(0xD11D); SPI_WriteData(0x7C);
   SPI_WriteCom(0xD11E); SPI_WriteData(0x02);
   SPI_WriteCom(0xD11F); SPI_WriteData(0xC4);
   SPI_WriteCom(0xD120); SPI_WriteData(0x02);
   SPI_WriteCom(0xD121); SPI_WriteData(0xF6);
   SPI_WriteCom(0xD122); SPI_WriteData(0x03);
   SPI_WriteCom(0xD123); SPI_WriteData(0x3A);
   SPI_WriteCom(0xD124); SPI_WriteData(0x03);
   SPI_WriteCom(0xD125); SPI_WriteData(0x68);
   SPI_WriteCom(0xD126); SPI_WriteData(0x03);
   SPI_WriteCom(0xD127); SPI_WriteData(0xA0);
   SPI_WriteCom(0xD128); SPI_WriteData(0x03);
   SPI_WriteCom(0xD129); SPI_WriteData(0xBF);
   SPI_WriteCom(0xD12A); SPI_WriteData(0x03);
   SPI_WriteCom(0xD12B); SPI_WriteData(0xE0);
   SPI_WriteCom(0xD12C); SPI_WriteData(0x03);
   SPI_WriteCom(0xD12D); SPI_WriteData(0xEC);
   SPI_WriteCom(0xD12E); SPI_WriteData(0x03);
   SPI_WriteCom(0xD12F); SPI_WriteData(0xF5);
   SPI_WriteCom(0xD130); SPI_WriteData(0x03);
   SPI_WriteCom(0xD131); SPI_WriteData(0xFA);
   SPI_WriteCom(0xD132); SPI_WriteData(0x03);
   SPI_WriteCom(0xD133); SPI_WriteData(0xFF);

       SPI_WriteCom(0xD200); SPI_WriteData(0x00);
   SPI_WriteCom(0xD201); SPI_WriteData(0x00);
   SPI_WriteCom(0xD202); SPI_WriteData(0x00);
   SPI_WriteCom(0xD203); SPI_WriteData(0x17);
   SPI_WriteCom(0xD204); SPI_WriteData(0x00);
   SPI_WriteCom(0xD205); SPI_WriteData(0x3E);
   SPI_WriteCom(0xD206); SPI_WriteData(0x00);
   SPI_WriteCom(0xD207); SPI_WriteData(0x5E);
   SPI_WriteCom(0xD208); SPI_WriteData(0x00);
   SPI_WriteCom(0xD209); SPI_WriteData(0x7B);
   SPI_WriteCom(0xD20A); SPI_WriteData(0x00);
   SPI_WriteCom(0xD20B); SPI_WriteData(0xA9);
   SPI_WriteCom(0xD20C); SPI_WriteData(0x00);
   SPI_WriteCom(0xD20D); SPI_WriteData(0xCE);
   SPI_WriteCom(0xD20E); SPI_WriteData(0x01);
   SPI_WriteCom(0xD20F); SPI_WriteData(0x0A);
   SPI_WriteCom(0xD210); SPI_WriteData(0x01);
   SPI_WriteCom(0xD211); SPI_WriteData(0x37);
   SPI_WriteCom(0xD212); SPI_WriteData(0x01);
   SPI_WriteCom(0xD213); SPI_WriteData(0x7C);
   SPI_WriteCom(0xD214); SPI_WriteData(0x01);
   SPI_WriteCom(0xD215); SPI_WriteData(0xB0);
   SPI_WriteCom(0xD216); SPI_WriteData(0x01);
   SPI_WriteCom(0xD217); SPI_WriteData(0xFF);
   SPI_WriteCom(0xD218); SPI_WriteData(0x02);
   SPI_WriteCom(0xD219); SPI_WriteData(0x3D);
   SPI_WriteCom(0xD21A); SPI_WriteData(0x02);
   SPI_WriteCom(0xD21B); SPI_WriteData(0x3F);
   SPI_WriteCom(0xD21C); SPI_WriteData(0x02);
   SPI_WriteCom(0xD21D); SPI_WriteData(0x7C);
   SPI_WriteCom(0xD21E); SPI_WriteData(0x02);
   SPI_WriteCom(0xD21F); SPI_WriteData(0xC4);
   SPI_WriteCom(0xD220); SPI_WriteData(0x02);
   SPI_WriteCom(0xD221); SPI_WriteData(0xF6);
   SPI_WriteCom(0xD222); SPI_WriteData(0x03);
   SPI_WriteCom(0xD223); SPI_WriteData(0x3A);
   SPI_WriteCom(0xD224); SPI_WriteData(0x03);
   SPI_WriteCom(0xD225); SPI_WriteData(0x68);
   SPI_WriteCom(0xD226); SPI_WriteData(0x03);
   SPI_WriteCom(0xD227); SPI_WriteData(0xA0);
   SPI_WriteCom(0xD228); SPI_WriteData(0x03);
   SPI_WriteCom(0xD229); SPI_WriteData(0xBF);
   SPI_WriteCom(0xD22A); SPI_WriteData(0x03);
   SPI_WriteCom(0xD22B); SPI_WriteData(0xE0);
   SPI_WriteCom(0xD22C); SPI_WriteData(0x03);
   SPI_WriteCom(0xD22D); SPI_WriteData(0xEC);
   SPI_WriteCom(0xD22E); SPI_WriteData(0x03);
   SPI_WriteCom(0xD22F); SPI_WriteData(0xF5);
   SPI_WriteCom(0xD230); SPI_WriteData(0x03);
   SPI_WriteCom(0xD231); SPI_WriteData(0xFA);
   SPI_WriteCom(0xD232); SPI_WriteData(0x03);
   SPI_WriteCom(0xD233); SPI_WriteData(0xFF);

      SPI_WriteCom(0xD300); SPI_WriteData(0x00);
   SPI_WriteCom(0xD301); SPI_WriteData(0x00);
   SPI_WriteCom(0xD302); SPI_WriteData(0x00);
   SPI_WriteCom(0xD303); SPI_WriteData(0x17);
   SPI_WriteCom(0xD304); SPI_WriteData(0x00);
   SPI_WriteCom(0xD305); SPI_WriteData(0x3E);
   SPI_WriteCom(0xD306); SPI_WriteData(0x00);
   SPI_WriteCom(0xD307); SPI_WriteData(0x5E);
   SPI_WriteCom(0xD308); SPI_WriteData(0x00);
   SPI_WriteCom(0xD309); SPI_WriteData(0x7B);
   SPI_WriteCom(0xD30A); SPI_WriteData(0x00);
   SPI_WriteCom(0xD30B); SPI_WriteData(0xA9);
   SPI_WriteCom(0xD30C); SPI_WriteData(0x00);
   SPI_WriteCom(0xD30D); SPI_WriteData(0xCE);
   SPI_WriteCom(0xD30E); SPI_WriteData(0x01);
   SPI_WriteCom(0xD30F); SPI_WriteData(0x0A);
   SPI_WriteCom(0xD310); SPI_WriteData(0x01);
   SPI_WriteCom(0xD311); SPI_WriteData(0x37);
   SPI_WriteCom(0xD312); SPI_WriteData(0x01);
   SPI_WriteCom(0xD313); SPI_WriteData(0x7C);
   SPI_WriteCom(0xD314); SPI_WriteData(0x01);
   SPI_WriteCom(0xD315); SPI_WriteData(0xB0);
   SPI_WriteCom(0xD316); SPI_WriteData(0x01);
   SPI_WriteCom(0xD317); SPI_WriteData(0xFF);
   SPI_WriteCom(0xD318); SPI_WriteData(0x02);
   SPI_WriteCom(0xD319); SPI_WriteData(0x3D);
   SPI_WriteCom(0xD31A); SPI_WriteData(0x02);
   SPI_WriteCom(0xD31B); SPI_WriteData(0x3F);
   SPI_WriteCom(0xD31C); SPI_WriteData(0x02);
   SPI_WriteCom(0xD31D); SPI_WriteData(0x7C);
   SPI_WriteCom(0xD31E); SPI_WriteData(0x02);
   SPI_WriteCom(0xD31F); SPI_WriteData(0xC4);
   SPI_WriteCom(0xD320); SPI_WriteData(0x02);
   SPI_WriteCom(0xD321); SPI_WriteData(0xF6);
   SPI_WriteCom(0xD322); SPI_WriteData(0x03);
   SPI_WriteCom(0xD323); SPI_WriteData(0x3A);
   SPI_WriteCom(0xD324); SPI_WriteData(0x03);
   SPI_WriteCom(0xD325); SPI_WriteData(0x68);
   SPI_WriteCom(0xD326); SPI_WriteData(0x03);
   SPI_WriteCom(0xD327); SPI_WriteData(0xA0);
   SPI_WriteCom(0xD328); SPI_WriteData(0x03);
   SPI_WriteCom(0xD329); SPI_WriteData(0xBF);
   SPI_WriteCom(0xD32A); SPI_WriteData(0x03);
   SPI_WriteCom(0xD32B); SPI_WriteData(0xE0);
   SPI_WriteCom(0xD32C); SPI_WriteData(0x03);
   SPI_WriteCom(0xD32D); SPI_WriteData(0xEC);
   SPI_WriteCom(0xD32E); SPI_WriteData(0x03);
   SPI_WriteCom(0xD32F); SPI_WriteData(0xF5);
   SPI_WriteCom(0xD330); SPI_WriteData(0x03);
   SPI_WriteCom(0xD331); SPI_WriteData(0xFA);
   SPI_WriteCom(0xD332); SPI_WriteData(0x03);
   SPI_WriteCom(0xD333); SPI_WriteData(0xFF);

#if 1
        SPI_WriteCom(0xD400); SPI_WriteData(0x00);
   SPI_WriteCom(0xD401); SPI_WriteData(0x00);
   SPI_WriteCom(0xD402); SPI_WriteData(0x00);
   SPI_WriteCom(0xD403); SPI_WriteData(0x17);
   SPI_WriteCom(0xD404); SPI_WriteData(0x00);
   SPI_WriteCom(0xD405); SPI_WriteData(0x3E);
   SPI_WriteCom(0xD406); SPI_WriteData(0x00);
   SPI_WriteCom(0xD407); SPI_WriteData(0x5E);
   SPI_WriteCom(0xD408); SPI_WriteData(0x00);
   SPI_WriteCom(0xD409); SPI_WriteData(0x7B);
   SPI_WriteCom(0xD40A); SPI_WriteData(0x00);
   SPI_WriteCom(0xD40B); SPI_WriteData(0xA9);
   SPI_WriteCom(0xD40C); SPI_WriteData(0x00);
   SPI_WriteCom(0xD40D); SPI_WriteData(0xCE);
   SPI_WriteCom(0xD40E); SPI_WriteData(0x01);
   SPI_WriteCom(0xD40F); SPI_WriteData(0x0A);
   SPI_WriteCom(0xD410); SPI_WriteData(0x01);
   SPI_WriteCom(0xD411); SPI_WriteData(0x37);
   SPI_WriteCom(0xD412); SPI_WriteData(0x01);
   SPI_WriteCom(0xD413); SPI_WriteData(0x7C);
   SPI_WriteCom(0xD414); SPI_WriteData(0x01);
   SPI_WriteCom(0xD415); SPI_WriteData(0xB0);
   SPI_WriteCom(0xD416); SPI_WriteData(0x01);
   SPI_WriteCom(0xD417); SPI_WriteData(0xFF);
   SPI_WriteCom(0xD418); SPI_WriteData(0x02);
   SPI_WriteCom(0xD419); SPI_WriteData(0x3D);
   SPI_WriteCom(0xD41A); SPI_WriteData(0x02);
   SPI_WriteCom(0xD41B); SPI_WriteData(0x3F);
   SPI_WriteCom(0xD41C); SPI_WriteData(0x02);
   SPI_WriteCom(0xD41D); SPI_WriteData(0x7C);
   SPI_WriteCom(0xD41E); SPI_WriteData(0x02);
   SPI_WriteCom(0xD41F); SPI_WriteData(0xC4);
   SPI_WriteCom(0xD420); SPI_WriteData(0x02);
   SPI_WriteCom(0xD421); SPI_WriteData(0xF6);
   SPI_WriteCom(0xD422); SPI_WriteData(0x03);
   SPI_WriteCom(0xD423); SPI_WriteData(0x3A);
   SPI_WriteCom(0xD424); SPI_WriteData(0x03);
   SPI_WriteCom(0xD425); SPI_WriteData(0x68);
   SPI_WriteCom(0xD426); SPI_WriteData(0x03);
   SPI_WriteCom(0xD427); SPI_WriteData(0xA0);
   SPI_WriteCom(0xD428); SPI_WriteData(0x03);
   SPI_WriteCom(0xD429); SPI_WriteData(0xBF);
   SPI_WriteCom(0xD42A); SPI_WriteData(0x03);
   SPI_WriteCom(0xD42B); SPI_WriteData(0xE0);
   SPI_WriteCom(0xD42C); SPI_WriteData(0x03);
   SPI_WriteCom(0xD42D); SPI_WriteData(0xEC);
   SPI_WriteCom(0xD42E); SPI_WriteData(0x03);
   SPI_WriteCom(0xD42F); SPI_WriteData(0xF5);
   SPI_WriteCom(0xD430); SPI_WriteData(0x03);
   SPI_WriteCom(0xD431); SPI_WriteData(0xFA);
   SPI_WriteCom(0xD432); SPI_WriteData(0x03);
   SPI_WriteCom(0xD433); SPI_WriteData(0xFF);
   SPI_WriteCom(0xD500); SPI_WriteData(0x00);
   SPI_WriteCom(0xD501); SPI_WriteData(0x00);
   SPI_WriteCom(0xD502); SPI_WriteData(0x00);
   SPI_WriteCom(0xD503); SPI_WriteData(0x17);
   SPI_WriteCom(0xD504); SPI_WriteData(0x00);
   SPI_WriteCom(0xD505); SPI_WriteData(0x3E);
   SPI_WriteCom(0xD506); SPI_WriteData(0x00);
   SPI_WriteCom(0xD507); SPI_WriteData(0x5E);
   SPI_WriteCom(0xD508); SPI_WriteData(0x00);
   SPI_WriteCom(0xD509); SPI_WriteData(0x7B);
   SPI_WriteCom(0xD50A); SPI_WriteData(0x00);
   SPI_WriteCom(0xD50B); SPI_WriteData(0xA9);
   SPI_WriteCom(0xD50C); SPI_WriteData(0x00);
   SPI_WriteCom(0xD50D); SPI_WriteData(0xCE);
   SPI_WriteCom(0xD50E); SPI_WriteData(0x01);
   SPI_WriteCom(0xD50F); SPI_WriteData(0x0A);
   SPI_WriteCom(0xD510); SPI_WriteData(0x01);
   SPI_WriteCom(0xD511); SPI_WriteData(0x37);
   SPI_WriteCom(0xD512); SPI_WriteData(0x01);
   SPI_WriteCom(0xD513); SPI_WriteData(0x7C);
   SPI_WriteCom(0xD514); SPI_WriteData(0x01);
   SPI_WriteCom(0xD515); SPI_WriteData(0xB0);
   SPI_WriteCom(0xD516); SPI_WriteData(0x01);
   SPI_WriteCom(0xD517); SPI_WriteData(0xFF);
   SPI_WriteCom(0xD518); SPI_WriteData(0x02);
   SPI_WriteCom(0xD519); SPI_WriteData(0x3D);
   SPI_WriteCom(0xD51A); SPI_WriteData(0x02);
   SPI_WriteCom(0xD51B); SPI_WriteData(0x3F);
   SPI_WriteCom(0xD51C); SPI_WriteData(0x02);
   SPI_WriteCom(0xD51D); SPI_WriteData(0x7C);
   SPI_WriteCom(0xD51E); SPI_WriteData(0x02);
   SPI_WriteCom(0xD51F); SPI_WriteData(0xC4);
   SPI_WriteCom(0xD520); SPI_WriteData(0x02);
   SPI_WriteCom(0xD521); SPI_WriteData(0xF6);
   SPI_WriteCom(0xD522); SPI_WriteData(0x03);
   SPI_WriteCom(0xD523); SPI_WriteData(0x3A);
   SPI_WriteCom(0xD524); SPI_WriteData(0x03);
   SPI_WriteCom(0xD525); SPI_WriteData(0x68);
   SPI_WriteCom(0xD526); SPI_WriteData(0x03);
   SPI_WriteCom(0xD527); SPI_WriteData(0xA0);
   SPI_WriteCom(0xD528); SPI_WriteData(0x03);
   SPI_WriteCom(0xD529); SPI_WriteData(0xBF);
   SPI_WriteCom(0xD52A); SPI_WriteData(0x03);
   SPI_WriteCom(0xD52B); SPI_WriteData(0xE0);
   SPI_WriteCom(0xD52C); SPI_WriteData(0x03);
   SPI_WriteCom(0xD52D); SPI_WriteData(0xEC);
   SPI_WriteCom(0xD52E); SPI_WriteData(0x03);
   SPI_WriteCom(0xD52F); SPI_WriteData(0xF5);
   SPI_WriteCom(0xD530); SPI_WriteData(0x03);
   SPI_WriteCom(0xD531); SPI_WriteData(0xFA);
   SPI_WriteCom(0xD532); SPI_WriteData(0x03);
   SPI_WriteCom(0xD533); SPI_WriteData(0xFF);
#endif
	
        SPI_WriteCom(0xD600); SPI_WriteData(0x00);
   SPI_WriteCom(0xD601); SPI_WriteData(0x00);
   SPI_WriteCom(0xD602); SPI_WriteData(0x00);
   SPI_WriteCom(0xD603); SPI_WriteData(0x17);
   SPI_WriteCom(0xD604); SPI_WriteData(0x00);
   SPI_WriteCom(0xD605); SPI_WriteData(0x3E);
   SPI_WriteCom(0xD606); SPI_WriteData(0x00);
   SPI_WriteCom(0xD607); SPI_WriteData(0x5E);
   SPI_WriteCom(0xD608); SPI_WriteData(0x00);
   SPI_WriteCom(0xD609); SPI_WriteData(0x7B);
   SPI_WriteCom(0xD60A); SPI_WriteData(0x00);
   SPI_WriteCom(0xD60B); SPI_WriteData(0xA9);
   SPI_WriteCom(0xD60C); SPI_WriteData(0x00);
   SPI_WriteCom(0xD60D); SPI_WriteData(0xCE);
   SPI_WriteCom(0xD60E); SPI_WriteData(0x01);
   SPI_WriteCom(0xD60F); SPI_WriteData(0x0A);
   SPI_WriteCom(0xD610); SPI_WriteData(0x01);
   SPI_WriteCom(0xD611); SPI_WriteData(0x37);
   SPI_WriteCom(0xD612); SPI_WriteData(0x01);
   SPI_WriteCom(0xD613); SPI_WriteData(0x7C);
   SPI_WriteCom(0xD614); SPI_WriteData(0x01);
   SPI_WriteCom(0xD615); SPI_WriteData(0xB0);
   SPI_WriteCom(0xD616); SPI_WriteData(0x01);
   SPI_WriteCom(0xD617); SPI_WriteData(0xFF);
   SPI_WriteCom(0xD618); SPI_WriteData(0x02);
   SPI_WriteCom(0xD619); SPI_WriteData(0x3D);
   SPI_WriteCom(0xD61A); SPI_WriteData(0x02);
   SPI_WriteCom(0xD61B); SPI_WriteData(0x3F);
   SPI_WriteCom(0xD61C); SPI_WriteData(0x02);
   SPI_WriteCom(0xD61D); SPI_WriteData(0x7C);
   SPI_WriteCom(0xD61E); SPI_WriteData(0x02);
   SPI_WriteCom(0xD61F); SPI_WriteData(0xC4);
   SPI_WriteCom(0xD620); SPI_WriteData(0x02);
   SPI_WriteCom(0xD621); SPI_WriteData(0xF6);
   SPI_WriteCom(0xD622); SPI_WriteData(0x03);
   SPI_WriteCom(0xD623); SPI_WriteData(0x3A);
   SPI_WriteCom(0xD624); SPI_WriteData(0x03);
   SPI_WriteCom(0xD625); SPI_WriteData(0x68);
   SPI_WriteCom(0xD626); SPI_WriteData(0x03);
   SPI_WriteCom(0xD627); SPI_WriteData(0xA0);
   SPI_WriteCom(0xD628); SPI_WriteData(0x03);
   SPI_WriteCom(0xD629); SPI_WriteData(0xBF);
   SPI_WriteCom(0xD62A); SPI_WriteData(0x03);
   SPI_WriteCom(0xD62B); SPI_WriteData(0xE0);
   SPI_WriteCom(0xD62C); SPI_WriteData(0x03);
   SPI_WriteCom(0xD62D); SPI_WriteData(0xEC);
   SPI_WriteCom(0xD62E); SPI_WriteData(0x03);
   SPI_WriteCom(0xD62F); SPI_WriteData(0xF5);
   SPI_WriteCom(0xD630); SPI_WriteData(0x03);
   SPI_WriteCom(0xD631); SPI_WriteData(0xFA);
   SPI_WriteCom(0xD632); SPI_WriteData(0x03);
   SPI_WriteCom(0xD633); SPI_WriteData(0xFF);

SPI_WriteCom(0xF000); SPI_WriteData(0x55);
SPI_WriteCom(0xF001); SPI_WriteData(0xAA);
SPI_WriteCom(0xF002); SPI_WriteData(0x52);
SPI_WriteCom(0xF003); SPI_WriteData(0x08);
SPI_WriteCom(0xF004); SPI_WriteData(0x03);

SPI_WriteCom(0xB000); SPI_WriteData(0x05);
SPI_WriteCom(0xB001); SPI_WriteData(0x17);
SPI_WriteCom(0xB002); SPI_WriteData(0xF9);
SPI_WriteCom(0xB003); SPI_WriteData(0x53);
SPI_WriteCom(0xB004); SPI_WriteData(0x53);
SPI_WriteCom(0xB005); SPI_WriteData(0x00);
SPI_WriteCom(0xB006); SPI_WriteData(0x30);

SPI_WriteCom(0xB100); SPI_WriteData(0x05);
SPI_WriteCom(0xB101); SPI_WriteData(0x17);
SPI_WriteCom(0xB102); SPI_WriteData(0xFB);
SPI_WriteCom(0xB103); SPI_WriteData(0x55);
SPI_WriteCom(0xB104); SPI_WriteData(0x53);
SPI_WriteCom(0xB105); SPI_WriteData(0x00);
SPI_WriteCom(0xB106); SPI_WriteData(0x30);

SPI_WriteCom(0xB200); SPI_WriteData(0xFB);
SPI_WriteCom(0xB201); SPI_WriteData(0xFC);
SPI_WriteCom(0xB202); SPI_WriteData(0xFD);
SPI_WriteCom(0xB203); SPI_WriteData(0xFE);
SPI_WriteCom(0xB204); SPI_WriteData(0xF0);
SPI_WriteCom(0xB205); SPI_WriteData(0x53);
SPI_WriteCom(0xB206); SPI_WriteData(0x00);
SPI_WriteCom(0xB207); SPI_WriteData(0xC5);
SPI_WriteCom(0xB208); SPI_WriteData(0x08);

SPI_WriteCom(0xB300); SPI_WriteData(0x5B);
SPI_WriteCom(0xB301); SPI_WriteData(0x00);
SPI_WriteCom(0xB302); SPI_WriteData(0xFB);
SPI_WriteCom(0xB303); SPI_WriteData(0x5A);
SPI_WriteCom(0xB304); SPI_WriteData(0x5A);
SPI_WriteCom(0xB305); SPI_WriteData(0x0C);

SPI_WriteCom(0xB400); SPI_WriteData(0xFF);
SPI_WriteCom(0xB401); SPI_WriteData(0x00);
SPI_WriteCom(0xB402); SPI_WriteData(0x01);
SPI_WriteCom(0xB403); SPI_WriteData(0x02);
SPI_WriteCom(0xB404); SPI_WriteData(0xC0);
SPI_WriteCom(0xB405); SPI_WriteData(0x40);
SPI_WriteCom(0xB406); SPI_WriteData(0x05);
SPI_WriteCom(0xB407); SPI_WriteData(0x08);
SPI_WriteCom(0xB408); SPI_WriteData(0x53);

SPI_WriteCom(0xB500); SPI_WriteData(0x00);
SPI_WriteCom(0xB501); SPI_WriteData(0x00);
SPI_WriteCom(0xB502); SPI_WriteData(0xFF);
SPI_WriteCom(0xB503); SPI_WriteData(0x83);
SPI_WriteCom(0xB504); SPI_WriteData(0x5F);
SPI_WriteCom(0xB505); SPI_WriteData(0x5E);
SPI_WriteCom(0xB506); SPI_WriteData(0x50);
SPI_WriteCom(0xB507); SPI_WriteData(0x50);
SPI_WriteCom(0xB508); SPI_WriteData(0x33);
SPI_WriteCom(0xB509); SPI_WriteData(0x33);
SPI_WriteCom(0xB50A); SPI_WriteData(0x55);

SPI_WriteCom(0xB600); SPI_WriteData(0xBC);
SPI_WriteCom(0xB601); SPI_WriteData(0x00);
SPI_WriteCom(0xB602); SPI_WriteData(0x00);
SPI_WriteCom(0xB603); SPI_WriteData(0x00);
SPI_WriteCom(0xB604); SPI_WriteData(0x2A);
SPI_WriteCom(0xB605); SPI_WriteData(0x80);
SPI_WriteCom(0xB606); SPI_WriteData(0x00);

SPI_WriteCom(0xB700); SPI_WriteData(0x00);
SPI_WriteCom(0xB701); SPI_WriteData(0x00);
SPI_WriteCom(0xB702); SPI_WriteData(0x00);
SPI_WriteCom(0xB703); SPI_WriteData(0x00);
SPI_WriteCom(0xB704); SPI_WriteData(0x00);
SPI_WriteCom(0xB705); SPI_WriteData(0x00);
SPI_WriteCom(0xB706); SPI_WriteData(0x00);
SPI_WriteCom(0xB707); SPI_WriteData(0x00);

SPI_WriteCom(0xB800); SPI_WriteData(0x11);
SPI_WriteCom(0xB801); SPI_WriteData(0x60);
SPI_WriteCom(0xB802); SPI_WriteData(0x00);

SPI_WriteCom(0xB900); SPI_WriteData(0x90);

SPI_WriteCom(0xBA00); SPI_WriteData(0x44);
SPI_WriteCom(0xBA01); SPI_WriteData(0x44);
SPI_WriteCom(0xBA02); SPI_WriteData(0x08);
SPI_WriteCom(0xBA03); SPI_WriteData(0xAC);
SPI_WriteCom(0xBA04); SPI_WriteData(0xE2);
SPI_WriteCom(0xBA05); SPI_WriteData(0x64);
SPI_WriteCom(0xBA06); SPI_WriteData(0x44);
SPI_WriteCom(0xBA07); SPI_WriteData(0x44);
SPI_WriteCom(0xBA08); SPI_WriteData(0x44);
SPI_WriteCom(0xBA09); SPI_WriteData(0x44);
SPI_WriteCom(0xBA0A); SPI_WriteData(0x47);
SPI_WriteCom(0xBA0B); SPI_WriteData(0x3F);
SPI_WriteCom(0xBA0C); SPI_WriteData(0xDB);
SPI_WriteCom(0xBA0D); SPI_WriteData(0x91);
SPI_WriteCom(0xBA0E); SPI_WriteData(0x54);
SPI_WriteCom(0xBA0F); SPI_WriteData(0x44);

SPI_WriteCom(0xBB00); SPI_WriteData(0x44);
SPI_WriteCom(0xBB01); SPI_WriteData(0x43);
SPI_WriteCom(0xBB02); SPI_WriteData(0x79);
SPI_WriteCom(0xBB03); SPI_WriteData(0xFD);
SPI_WriteCom(0xBB04); SPI_WriteData(0xB5);
SPI_WriteCom(0xBB05); SPI_WriteData(0x14);
SPI_WriteCom(0xBB06); SPI_WriteData(0x44);
SPI_WriteCom(0xBB07); SPI_WriteData(0x44);
SPI_WriteCom(0xBB08); SPI_WriteData(0x44);
SPI_WriteCom(0xBB09); SPI_WriteData(0x44);
SPI_WriteCom(0xBB0A); SPI_WriteData(0x40);
SPI_WriteCom(0xBB0B); SPI_WriteData(0x4A);
SPI_WriteCom(0xBB0C); SPI_WriteData(0xCE);
SPI_WriteCom(0xBB0D); SPI_WriteData(0x86);
SPI_WriteCom(0xBB0E); SPI_WriteData(0x24);
SPI_WriteCom(0xBB0F); SPI_WriteData(0x44);

SPI_WriteCom(0xBC00); SPI_WriteData(0xE0);
SPI_WriteCom(0xBC01); SPI_WriteData(0x1F);
SPI_WriteCom(0xBC02); SPI_WriteData(0xF8);
SPI_WriteCom(0xBC03); SPI_WriteData(0x07);

SPI_WriteCom(0xBD00); SPI_WriteData(0xE0);
SPI_WriteCom(0xBD01); SPI_WriteData(0x1F);
SPI_WriteCom(0xBD02); SPI_WriteData(0xF8);
SPI_WriteCom(0xBD03); SPI_WriteData(0x07);

SPI_WriteCom(0xF000); SPI_WriteData(0x55);
SPI_WriteCom(0xF001); SPI_WriteData(0xAA);
SPI_WriteCom(0xF002); SPI_WriteData(0x52);
SPI_WriteCom(0xF003); SPI_WriteData(0x08);
SPI_WriteCom(0xF004); SPI_WriteData(0x00);

SPI_WriteCom(0xB000); SPI_WriteData(0x00);
SPI_WriteCom(0xB001); SPI_WriteData(0x10);

SPI_WriteCom(0xB400); SPI_WriteData(0x10);

SPI_WriteCom(0xB500); SPI_WriteData(0x6B);		// 854
//SPI_WriteCom(0xB500); SPI_WriteData(0x50);		// 800

SPI_WriteCom(0xBC00); SPI_WriteData(0x00);

SPI_WriteCom(0x3600); SPI_WriteData(0x01);		// Vertical Flip


SPI_WriteCom(0x3500); SPI_WriteData(0x00);
SPI_WriteCom(0x3A00); SPI_WriteData(0x77);
SPI_WriteCom(0x1100);

Delayms(120);

SPI_WriteCom(0x2900);

Delayms(100);
#endif
}

void rm68172_screen_poweroff (void)
{
	Delayms(20);
	SPI_WriteCom(0x2800);
	Delayms(120);
	SPI_WriteCom(0x1000);
	Delayms(120);
	Set_RST(0);
	Delayms(20);
}


void spi_pad_init (void)
{
	unsigned int val;
	
	
#if TULV_BB_TEST
	// pad_ctl8
	// RESET --> cd7_pad	GPIO87	nand_data[6]
	// SDA   --> cd4_pad	GPIO84	nand_data[12]
	// SCL   --> cd3_pad GPIO83	nand_data[3]
	// CS    --> cd0_pad GPIO80	nand_data[9]
 
	XM_lock ();
	val = rSYS_PAD_CTRL08;
	val &= ~( (3 << 0) | (3 << 6) | (3 << 8) | (3 << 14) );
	rSYS_PAD_CTRL08 = val;
	XM_unlock ();
	
#else
	// pad_ctl8
	// LCD_RESET --> ND_CEN	[27:26]	nd_cen	nd_cen_pad	GPIO93	nandflash_cen0
	// LCD_CS    --> ND_RB	[25:24]	nd_rb		nd_rb_pad	GPIO92	nandflash_rb0
	// LCD_SDI	 --> CD1		[3:2]		cd1		cd1_pad		GPIO81	nand_data[2]
	// LCD_SCL	 --> CD0		[1:0]		cd0		cd0_pad		GPIO80	nand_data[9]

	XM_lock ();
	val = rSYS_PAD_CTRL08;
	val &= ~( (3 << 0) | (3 << 2) | (3 << 24) | (3 << 26) );
	rSYS_PAD_CTRL08 = val;
	XM_unlock ();
	
#endif

	XM_lock ();	
	SetGPIOPadDirection (LCD_CS_GPIO, euOutputPad);
	SetGPIOPadDirection (LCD_SCL_GPIO, euOutputPad);
	SetGPIOPadDirection (LCD_SDA_GPIO, euOutputPad);
	SetGPIOPadDirection (LCD_RESET_GPIO, euOutputPad);
	XM_unlock ();	
	
	
	Set_RST(0);
	Delayms(100);
	Set_RST(1);

	Set_CS(1);
	//Set_CSX(1);
	Set_SCL(1);	
	Set_SDI(1);
	
}

static void rm68172_screen_init(void)
{
	LCD_SelPad();
	LCD_ClockInit ();
	spi_pad_init ();
	pannel_spi_write ();	
}



// RGB LCD初始化
void HW_LCD_Init (void)
{
	OS_EnterRegion ();	// 禁止任务切换
	Lcd_ClkDisable();
	sys_soft_reset (softreset_lcd);
	Lcd_ClkEnable ();	
	OS_LeaveRegion ();	
	
	rm68172_screen_init ();
	lcd_rm68172_osd_init();

	HW_LCD_BackLightInit (); 
}

// RGB LCD关闭退出
void HW_LCD_Exit(void)
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
	
	OS_LeaveRegion ();	// 禁止任务切换
	
	rm68172_screen_poweroff ();
}

#endif