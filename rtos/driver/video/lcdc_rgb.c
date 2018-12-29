#include "hardware.h"
#include "lcd.h"
#include <stdio.h>
#include <math.h>
#include "hw_osd_layer.h"
#include <gpio.h>
#include "pwm.h"

#include "rtos.h"
#include "xm_app_menudata.h"
#include "board_config.h"

extern unsigned int AP_GetVCOM_PWM();
extern void AP_SetVCOM_PWM(u8_t Data_PWM);
extern void XM_lock (void);
extern void XM_unlock (void);
extern unsigned int AP_GetLCD_Rotate(void);

static unsigned int Bright_toggle_index;//VCOM 高电平的次数
static unsigned int Bright_index = 0;

#define VS_START        (0)
#define HS_START		(0)

#define HD				(1024)
#define VD				(600)

#define VBP		 		(20)
#define VFP	 	 		(12)

#define VPW		 		(3)
#define TV				(VS_START+VPW+VBP+VD+VFP)

#define HBP				(140)//80
#define HFP		 		(160) //80
#define HPW	       		(20)
#define TH				(HS_START+HPW+HBP+HD+HFP)

static void LCD_Init (void)
{
#define      lcd_enable             1// 1
#define      screen_width           1024//800
#define      rgb_pad_mode           5//5
#define      direct_enable          0
#define      mem_lcd_enable         0
#define      range_coeff_y          0
#define      range_coeff_uv         0
#define      lcd_done_intr_enable   1   //使能中断=1 禁止中断=0 lcd_done_intr
#define      dac_for_video          0
#define      dac_for_cvbs           0

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
#define      back_color             (0x22<<16)|(0x44<<8)|(0x88 )	// 无背景色
#define      cpu_screen_csn_bit     0
#define      cpu_screen_wen_bit     9

#define      DEN_h_rise             (HFP+HPW+HBP)    // 数据使能信号data_en水平起始位置
#define      DEN_h_fall             (HFP+HPW+HBP+HD)//(20+800*1)
#define      DEN0_v_rise            (VFP+VPW+VBP)   // even field data enable é?éy????
#define      DEN0_v_fall            (VFP+VPW+VBP+VD)
#define      DEN1_v_rise            (VFP+VPW+VBP)
#define      DEN1_v_fall            (VFP+VPW+VBP+VD)


// 帧频 = LCD时钟/(CPL * LPS)
// 每行时钟周期个数

//#define      CPL                    (1000)   // clock cycles per line  max 4096     ;  from register   timing2 16
// 每屏行数量
//#define      LPS                    (525)      // Lines per screen value  ;  from register   timing1 0

//#define      CPL                    (2048)   // clock cycles per line  max 4096     ;  from register   timing2 16
//#define      CPL                    (1600)   // clock cycles per line  max 4096     ;  from register   timing2 16

#define      CPL                    (TH)   // clock cycles per line  max 4096     ;  from register   timing2 16
#define      LPS                    (TV)    //(625)      // Lines per screen value  ;  from register   timing1 0

#define      HSW_rise                (HFP)//( 24 - 0)
#define      HSW_fall                (HFP+HPW)//( 24+126 - 0)

#define      VSW0_v_rise             (VFP)//(1)   // Vertical sync width value    ;  from register   timing1 10
#define      VSW0_v_fall             (VFP+VFP)//(3)   // Vertical sync width value     ;  from register   timing1 10

#define      VSW0_h_rise             (HFP)
#define      VSW0_h_fall             (HFP+HPW)

#define      VSW1_v_rise             (VFP)          // Vertical sync width value ;  from register   timing1 10
#define      VSW1_v_fall             (VFP+VPW)            // Vertical sync width value  ;  from register   timing1 10

#define      VSW1_h_rise             (HFP)
#define      VSW1_h_fall             (HFP+HPW)

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
	 VP_RGB2YCBCR_COEF0_reg = 0x10650242;    // 复位缺省值
     VP_RGB2YCBCR_COEF1_reg = 0x01c09426;    // 复位缺省值
     VP_RGB2YCBCR_COEF2_reg = 0x0048bc70;    // 复位缺省值
     VP_CONTROL_reg =  0xF | (64 << 8);  // VDE

VP_ADJUSTMent_reg=0x606090;
LCD_GAMMA_REG_0=3;
LCD_GAMMA_REG_1=0x0E0A0603;
LCD_GAMMA_REG_2=0x1F1A1611;
LCD_GAMMA_REG_3=0x35302A25;
LCD_GAMMA_REG_4=0x4C46403B;
LCD_GAMMA_REG_5=0x635D5752;
LCD_GAMMA_REG_6=0x7A756F69;
LCD_GAMMA_REG_7=0x8F8A857F;
LCD_GAMMA_REG_8=0xA19D9894;
LCD_GAMMA_REG_9=0xB2AEAAA6;
LCD_GAMMA_REG_10=0xC1BEBAB6;
LCD_GAMMA_REG_11=0xCFCCC8C5;
LCD_GAMMA_REG_12=0xDAD7D5D2;
LCD_GAMMA_REG_13=0xE3E1DFDC;
LCD_GAMMA_REG_14=0xEAE9E7E5;
LCD_GAMMA_REG_15=0xF0EFEEEC;
LCD_GAMMA_REG_16=0xF5F4F3F2;
LCD_GAMMA_REG_17=0x0E0A0603;
LCD_GAMMA_REG_18=0x1F1A1611;
LCD_GAMMA_REG_19=0x35302A25;
LCD_GAMMA_REG_20=0x4C46403B;
LCD_GAMMA_REG_21=0x635D5752;
LCD_GAMMA_REG_22=0x7A756F69;
LCD_GAMMA_REG_23=0x8F8A857F;
LCD_GAMMA_REG_24=0xA19D9894;
LCD_GAMMA_REG_25=0xB2AEAAA6;
LCD_GAMMA_REG_26=0xC1BEBAB6;
LCD_GAMMA_REG_27=0xCFCCC8C5;
LCD_GAMMA_REG_28=0xDAD7D5D2;
LCD_GAMMA_REG_29=0xE3E1DFDC;
LCD_GAMMA_REG_30=0xEAE9E7E5;
LCD_GAMMA_REG_31=0xF0EFEEEC;
LCD_GAMMA_REG_32=0xF5F4F3F2;
LCD_GAMMA_REG_33=0x0E0A0603;
LCD_GAMMA_REG_34=0x1F1A1611;
LCD_GAMMA_REG_35=0x35302A25;
LCD_GAMMA_REG_36=0x4C46403B;
LCD_GAMMA_REG_37=0x635D5752;
LCD_GAMMA_REG_38=0x7A756F69;
LCD_GAMMA_REG_39=0x8F8A857F;
LCD_GAMMA_REG_40=0xA19D9894;
LCD_GAMMA_REG_41=0xB2AEAAA6;
LCD_GAMMA_REG_42=0xC1BEBAB6;
LCD_GAMMA_REG_43=0xCFCCC8C5;
LCD_GAMMA_REG_44=0xDAD7D5D2;
LCD_GAMMA_REG_45=0xE3E1DFDC;
LCD_GAMMA_REG_46=0xEAE9E7E5;
LCD_GAMMA_REG_47=0xF0EFEEEC;
LCD_GAMMA_REG_48=0xF5F4F3F2;
#if 0
VP_ADJUSTMent_reg=0x113A78A9;
LCD_GAMMA_REG_0=3;
LCD_GAMMA_REG_1=0x01000000;
LCD_GAMMA_REG_2=0x03020201;
LCD_GAMMA_REG_3=0x08070504;
LCD_GAMMA_REG_4=0x0D0B0A09;
LCD_GAMMA_REG_5=0x13110F0E;
LCD_GAMMA_REG_6=0x1E1B1815;
LCD_GAMMA_REG_7=0x312C2722;
LCD_GAMMA_REG_8=0x49433C37;
LCD_GAMMA_REG_9=0x655E5750;
LCD_GAMMA_REG_10=0x837C746D;
LCD_GAMMA_REG_11=0xAB9F948B;
LCD_GAMMA_REG_12=0xCFC9C1B7;
LCD_GAMMA_REG_13=0xE1DDD9D5;
LCD_GAMMA_REG_14=0xEDEAE8E5;
LCD_GAMMA_REG_15=0xF5F3F1EF;
LCD_GAMMA_REG_16=0xFAF9F7F6;
LCD_GAMMA_REG_17=0x01000000;
LCD_GAMMA_REG_18=0x03020201;
LCD_GAMMA_REG_19=0x08070504;
LCD_GAMMA_REG_20=0x0D0B0A09;
LCD_GAMMA_REG_21=0x13110F0E;
LCD_GAMMA_REG_22=0x1E1B1815;
LCD_GAMMA_REG_23=0x312C2722;
LCD_GAMMA_REG_24=0x49433C37;
LCD_GAMMA_REG_25=0x655E5750;
LCD_GAMMA_REG_26=0x837C746D;
LCD_GAMMA_REG_27=0xAB9F948B;
LCD_GAMMA_REG_28=0xCFC9C1B7;
LCD_GAMMA_REG_29=0xE1DDD9D5;
LCD_GAMMA_REG_30=0xEDEAE8E5;
LCD_GAMMA_REG_31=0xF5F3F1EF;
LCD_GAMMA_REG_32=0xFAF9F7F6;
LCD_GAMMA_REG_33=0x01000000;
LCD_GAMMA_REG_34=0x03020201;
LCD_GAMMA_REG_35=0x08070504;
LCD_GAMMA_REG_36=0x0D0B0A09;
LCD_GAMMA_REG_37=0x13110F0E;
LCD_GAMMA_REG_38=0x1E1B1815;
LCD_GAMMA_REG_39=0x312C2722;
LCD_GAMMA_REG_40=0x49433C37;
LCD_GAMMA_REG_41=0x655E5750;
LCD_GAMMA_REG_42=0x837C746D;
LCD_GAMMA_REG_43=0xAB9F948B;
LCD_GAMMA_REG_44=0xCFC9C1B7;
LCD_GAMMA_REG_45=0xE1DDD9D5;
LCD_GAMMA_REG_46=0xEDEAE8E5;
LCD_GAMMA_REG_47=0xF5F3F1EF;
LCD_GAMMA_REG_48=0xFAF9F7F6;
#endif
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


static void LCD_ClockInit (void)
{
#if 0
	unsigned int IntTvClkSwitch_sel;
	unsigned int Tvout_lcd_clk_wire_div;
	unsigned int Tvout_lcd_clk;
	unsigned int Tvout_lcd_pixel_clk;
	unsigned int Tvout_lcd_pixel_clk_wire_div;

	// 2-0	R/W	0x4	IntTvClkSwitch_sel：IntTvClkSwitch
	//							3'b000: clk_240m
	//							3'b001: syspll_clk
	//							3'b010: audpll_clk
	//							3'b100: clk_24m
	IntTvClkSwitch_sel = 0x0;

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

	// 24MHz
	rSYS_LCD_CLK_CFG =	(Tvout_lcd_pixel_clk_wire_div << 15)
							|	(Tvout_lcd_pixel_clk << 9)
							|	(Tvout_lcd_clk << 8)
							|	(Tvout_lcd_clk_wire_div << 3)
							|	(IntTvClkSwitch_sel << 0)
							;

	Lcd_ClkEnable ();
#else
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


	Tvout_lcd_clk_wire_div = 5;		// 48MHz

	// 8	R/W	0x0	Tvout_lcd_clk  0 normal  1 inv
	Tvout_lcd_clk = 0;

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
#endif
}

static void LCD_ClockStop (void)
{
	Lcd_ClkDisable ();
}

BOOL Close_Brighness = TRUE;

// 开启背光
void HW_LCD_BackLightOn (void)
{
	set_lcd_bl_pin_value(1);
	
    Close_Brighness = FALSE;
    Bright_toggle_index = 1;
	XM_printf(">>>>HW_LCD_BackLightOn.........\r\n");
}

// 关闭背光
void HW_LCD_BackLightOff (void)
{
	set_lcd_bl_pin_value(0);
	Close_Brighness = TRUE;

	XM_printf(">>>>HW_LCD_BackLightOff.........\r\n");
}

// RGB LCD初始化
void HW_LCD_Init (void)
{
	LCD_SelPad();
	LCD_ClockInit ();
	LCD_Init();
	lcd_vcom_init();//初始化vcomdc
	lcd_ctrl_pin_init();//控制脚初始化,bias,bl,u/d,l/r
	set_lcd_bias_pin_value(1);
	HW_LCD_ROTATE(AP_GetLCD_Rotate());
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


//获取LCD亮度,饱和度,对比度
u32_t Get_HW_LCD_VP(void) 
{
    return VP_ADJUSTMent_reg;
}


//设置LCD亮度,饱和度,对比度
void Set_HW_LCD_VP(int bright,int saturation,int contrast) 
{
    VP_ADJUSTMent_reg = bright | (contrast << 8) |(saturation <<16);
}

