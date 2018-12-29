#include "hardware.h"
#include  "lcd.h"

// PAL 制 
// 数字分辨率 720 * 576 
// 1= 54M  3=27M

#ifdef    _SUPPORT_TV_PAL_

static void tv_pal_osd_init (void)
{
#define      lcd_enable             1
#define      screen_width           720//*2//720
#define      rgb_pad_mode           0
#define      direct_enable          0
#define      mem_lcd_enable         0 
#define      range_coeff_y          0
#define      range_coeff_uv         0
#define      dac_for_video          0
#define      dac_for_cvbs           1 


#define      lcd_interlace_flag     1
#define      screen_type            1 //pRGB_i   0  pRGB_p   1   sRGB_p   2  ITU656   3
#define      VSW1_enable            1
#define      LcdVComp               0
#define      itu_pal_ntsc           0
#define      field_ivs              1
#define      hsync_ivs              1
#define      vsync_ivs              1
#define      lcd_ac_ivs             0
#define      rgb2yuv_en             1
#define      test_on_flag           0

#define      back_color             (137<<16)|(143<<8)|(105) //(200<<16)|(100<<8)|(16)
#define      cpu_screen_csn_bit     0
#define      cpu_screen_wen_bit     9
#define      DEN_h_rise             144         // pal 144   ntsc 138 // data enable ?ú????é?éyμ?????
#define      DEN_h_fall             (144+720*1) // pal 144   ntsc 138  
//note:  DEN0_v_fall - DEN0_v_rise  == DEN1_v_fall - DEN1_v_rise 
#define      DEN0_v_rise            22          // pal 22    ntsc 18  //even field data enable é?éy????
#define      DEN0_v_fall            310         // pal 310   ntsc 263
#define      DEN1_v_rise            334         // pal 334   ntsc 285
#define      DEN1_v_fall            622         // pal 622   ntsc 524
#define      CPL                    (864)       // pal 864   ntsc 858    // clock cycles per line  max 4096     ;  from register   timing2 16
#define      LPS                    (625)       // pal 625   ntsc 525    Lines per screen value  ;  from register   timing1 0
                                      
#define      HSW_rise                  (24-0)  //左右移动显示位置
#define      HSW_fall                  (24+126-0)
                                       
#define      VSW0_v_rise               (1)   //pal 1     ntsc 4    // Vertical sync width value    ;  from register   timing1 10
#define      VSW0_v_fall               (3)   //pal 3     ntsc 6    // Vertical sync width value     ;  from register   timing1 10
                                      
#define      VSW0_h_rise                1 
#define      VSW0_h_fall                (1)
                                      
#define      VSW1_v_rise                313          //pal 313   ntsc 266      // Vertical sync width value ;  from register   timing1 10
#define      VSW1_v_fall                316          //pal 316   ntsc 269     // Vertical sync width value  ;  from register   timing1 10
                                           
#define      VSW1_h_rise                864          //pal 864   ntsc 858
#define      VSW1_h_fall                864          //pal 864   ntsc 858

#define      sav_pos                   288
#define      f1_vblk_start             624
#define      f1_vblk_end               23
#define      f2_vblk_start             315           //pal 315   ntsc 265
#define      f2_vblk_end               336
#define      f2_start                  313
#define      f2_end                    625
#define      binary_thres              128
#define      binary_big                240
#define      binary_small              16

#define      osd_0_global_coeff_enable      0
#define      osd_0_enable                   0
#define      osd_0_binary_enable            0
#define      osd_0_format                   0  //0 yuv420  1 rgb8888 2 rgb565 3 rgb454
#define      osd_0_width                    0
#define      osd_0_height                   0
#define      osd_0_vc1_range_map            0
#define      osd_0_chroma_disable           0
#define      osd_0_h_position               0
#define      osd_0_v_position               0
#define      osd_0_global_coeff             0
#define      osd_0_y_addr                   0
#define      osd_0_u_addr                  0//(0x80119440+800*180)
#define      osd_0_v_addr                  0//(0x80119440+800*180+400*90)
#define      osd_0_stride                  0
#define      osd_0_hoffset                 0
#define      osd_0_mult_coef               63


#define      osd_1_global_coeff_enable      0
#define      osd_1_enable                   0
#define      osd_1_binary_enable            0
#define      osd_1_format                   0
#define      osd_1_width                    0
#define      osd_1_height                   0
#define      osd_1_vc1_range_map            0
#define      osd_1_chroma_disable           0
#define      osd_1_h_position               0
#define      osd_1_v_position               0
#define      osd_1_global_coeff             0
#define      osd_1_y_addr                   0
#define      osd_1_u_addr                  (0)
#define      osd_1_v_addr                  (0)
#define      osd_1_stride                  0
#define      osd_1_hoffset                 0
#define      osd_1_mult_coef               64

	          
#define      osd_2_global_coeff_enable      0
#define      osd_2_enable                   0
#define      osd_2_binary_enable            0
#define      osd_2_format                   0 //0 yuv420  1 rgb8888 2 rgb565 3 rgb454
#define      osd_2_width                    0
#define      osd_2_height                   0
#define      osd_2_vc1_range_map            0
#define      osd_2_chroma_disable           0
#define      osd_2_h_position               0
#define      osd_2_v_position               0
#define      osd_2_global_coeff             0
#define      osd_2_y_addr                   0
#define      osd_2_u_addr                  0
#define      osd_2_v_addr                  0
#define      osd_2_stride                  0
#define      osd_2_hoffset                 0
#define      osd_2_mult_coef               64
	unsigned int  lcd_param0, lcd_param1, lcd_param2, lcd_param3 ;
	unsigned int  lcd_param4, lcd_param5, lcd_param6, lcd_param7 ;
	unsigned int  lcd_param8, lcd_param9, lcd_param10, lcd_param11 ;
	unsigned int  lcd_param12, lcd_param13, lcd_param14, lcd_param15 ;
	unsigned int  lcd_param16, lcd_param17, lcd_param18, lcd_param19 ;
	unsigned int  lcd_param20, lcd_param21, lcd_param22  ;

	unsigned int  osd_0_param0, osd_0_param1, osd_0_param2, osd_0_param3 , osd_0_param4 ,osd_0_param5;
	unsigned int  osd_1_param0, osd_1_param1, osd_1_param2, osd_1_param3 , osd_1_param4 ,osd_1_param5;
	unsigned int  osd_2_param0, osd_2_param1, osd_2_param2, osd_2_param3 , osd_2_param4 ,osd_2_param5;
	unsigned int  osd_3_param0, osd_3_param1, osd_3_param2, osd_3_param3 , osd_3_param4 ,osd_3_param5;
	unsigned int  osd_4_param0, osd_4_param1, osd_4_param2, osd_4_param3 , osd_4_param4 ,osd_4_param5;
	unsigned int  osd_5_param0, osd_5_param1, osd_5_param2, osd_5_param3 , osd_5_param4 ,osd_5_param5;
	char  stop_lcd = 0 ;
	
	LCD_PARAM0_reg  = (1<<17);
	LCD_PARAM0_reg  = 0;
	LCD_PARAM0_reg  = (1<<17);
	LCD_PARAM0_reg  = 0;
	LCD_TVOUT_PARAM3_reg  = (1<<11)|(3<<12) ;
  lcd_param0  = 
   lcd_enable   |
   (screen_width          <<1)| 
   (rgb_pad_mode        <<13)| 
   (direct_enable          <<16)|  
   (mem_lcd_enable   <<17     )|
   (range_coeff_y  <<18    )|
   (range_coeff_uv          <<22    )|
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
	( field_ivs     <<16) |
   ( hsync_ivs     <<17) |
   ( vsync_ivs     <<18) |
   ( lcd_ac_ivs     <<19) |
	//( rgb2yuv_en     <<20) |
	//( 1     <<21) |
     ( 0     <<21) |
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
  
  lcd_param12  =
    (  65    <<0  )|   
    (  129    <<9 ) | (25<<18) ; 
  
  lcd_param13  =
    (  38    <<0  )|   
    (  74    <<9 ) | (112<<18) ; 
  
  lcd_param14  =
    (  112    <<0  )|   
    (  94     <<9 ) | (18<<18) ; 

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
    ( f2_end   <<12) ;
  lcd_param22  =
    ( binary_thres   <<16 )|
    ( binary_big   <<8 )|
    ( binary_small   <<0) ;

  osd_0_param0  =
    ( osd_0_global_coeff_enable       <<0  )|
    ( osd_0_enable       <<1  )|
    ( osd_0_binary_enable       <<2  )|
    ( osd_0_format       <<3  )|
    ( osd_0_width       <<6  )|
    ( osd_0_height       <<18  )|
    ( osd_0_vc1_range_map       <<30  )|
    ( osd_0_chroma_disable       <<31  ) ;

  osd_0_param1  =
    ( osd_0_h_position       <<0  )|
    ( osd_0_v_position       <<12  )|
    ( osd_0_global_coeff       <<24  ) ;

  osd_0_param2  =osd_0_y_addr ; 
  osd_0_param3  =osd_0_u_addr ; 
  osd_0_param4  =osd_0_v_addr ; 
  osd_0_param5  =osd_0_stride<<0 | osd_0_hoffset<<12 |  osd_0_mult_coef<<24 ;

  osd_1_param0  =
    ( osd_1_global_coeff_enable       <<0  )|
    ( osd_1_enable       <<1  )|
    ( osd_1_binary_enable       <<2  )|
    ( osd_1_format       <<3  )|
    ( osd_1_width       <<6  )|
    ( osd_1_height       <<18  )|
    ( osd_1_vc1_range_map       <<30  )|
    ( osd_1_chroma_disable       <<31  ) ;

  osd_1_param1  =
    ( osd_1_h_position       <<0  )|
    ( osd_1_v_position       <<12  )|
    ( osd_1_global_coeff       <<24  ) ;

  osd_1_param2  =osd_1_y_addr ; 
  osd_1_param3  =osd_1_u_addr ; 
  osd_1_param4  =osd_1_v_addr ; 
  osd_1_param5  =osd_1_stride<<0 | osd_1_hoffset<<12 |  osd_1_mult_coef<<24;


  osd_2_param0  =
    ( osd_2_global_coeff_enable       <<0  )|
    ( osd_2_enable       <<1  )|
    ( osd_2_binary_enable       <<2  )|
    ( osd_2_format       <<3  )|
    ( osd_2_width       <<6  )|
    ( osd_2_height       <<18  )|
    ( osd_2_vc1_range_map       <<30  )|
    ( osd_2_chroma_disable       <<31  ) ;

  osd_2_param1  =
    ( osd_2_h_position       <<0  )|
    ( osd_2_v_position       <<12  )|
    ( osd_2_global_coeff       <<24  ) ;

  osd_2_param2  =osd_2_y_addr ; 
  osd_2_param3  =osd_2_u_addr ; 
  osd_2_param4  =osd_2_v_addr ; 
  osd_2_param5  =osd_2_stride<<0 | osd_2_hoffset<<12 |  osd_2_mult_coef<<24 ;


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

  LCD_PARAM18_reg    = lcd_param18 ;
  LCD_PARAM19_reg    = lcd_param19 ;
  LCD_PARAM20_reg    = lcd_param20 ;
  LCD_PARAM21_reg    = lcd_param21 ;
  LCD_PARAM22_reg    = lcd_param22 ;
  //lcd_param1 = LCD_PARAM1_reg      ;

  LCD_OSD_0_PARAM0_reg     = osd_0_param0  ;
  LCD_OSD_0_PARAM1_reg     = osd_0_param1  ;
  LCD_OSD_0_PARAM2_reg     = osd_0_param2  ;
  LCD_OSD_0_PARAM3_reg     = osd_0_param3  ;
  LCD_OSD_0_PARAM4_reg     = osd_0_param4  ;
  LCD_OSD_0_PARAM5_reg     = osd_0_param5  ;

  LCD_OSD_1_PARAM0_reg     = osd_1_param0  ;
  LCD_OSD_1_PARAM1_reg     = osd_1_param1  ;
  LCD_OSD_1_PARAM2_reg     = osd_1_param2  ;
  LCD_OSD_1_PARAM3_reg     = osd_1_param3  ;
  LCD_OSD_1_PARAM4_reg     = osd_1_param4  ;
  LCD_OSD_1_PARAM5_reg     = osd_1_param5  ;
                                  
  LCD_OSD_2_PARAM0_reg     = osd_2_param0  ;
  LCD_OSD_2_PARAM1_reg     = osd_2_param1  ;
  LCD_OSD_2_PARAM2_reg     = osd_2_param2  ;
  LCD_OSD_2_PARAM3_reg     = osd_2_param3  ;
  LCD_OSD_2_PARAM4_reg     = osd_2_param4  ;
  LCD_OSD_2_PARAM5_reg     = osd_2_param5  ;
                                  
  LCD_PARAM0_reg     = lcd_param0  ;
   
}

static void tvencoder_init_pal(void)
{
//TV encoder setting
#define      chroma_freq_palbg              0x2a098acb  //pal
#define      chroma_freq_palm               0x21e6efa4  //palm
#define      chroma_freq_palnc              0x21f69446  //palnc
#define      chroma_freq_ntsc               0x21f07c1f  //ntsc
#define      chroma_phase                   0x2a
#define      clrbar_sel                     0
#define      clrbar_mode                    0
#define      bypass_yclamp                  0
#define      yc_delay                       4
#define      cvbs_enable                    1
#define      chroma_bw_1                    0 // bw_1,bw_0 : 00: narrow band; 01: wide band; 10: extra wide; 11: ultra wide.
#define      chroma_bw_0                    1
#define      comp_yuv                       0
#define      compchgain                     0
#define      hsync_width                    0x3f //0x7e*2
#define      burst_width                    0x3e   //pal 0x3e     ntsc 0x44
#define      back_porch                     0x45   //pal 0x45     ntsc 0x3b
#define      cb_burst_amp                   0x20
#define      cr_burst_amp                   0x20   //pal 0x20     ntsc 0x00
#define      slave_mode                     0x1
#define      black_level                    0xf2
#define      blank_level                    0xf0
#define      n1                             0x17
#define      n3                             0x21
#define      n8                             0x1b
#define      n9                             0x1b
#define      n10                            0x24
#define      num_lines                      625   // pal: 625;   ntsc: 525.
#define      n0                             0x3e
#define      n13                            0x0f
#define      n14                            0x0f
#define      n15                            0x60
#define      n5                             0x05
#define      white_level                    0x320
#define      cb_gain                        0x89
#define      n20                            0x04
#define      cr_gain                        0x89
#define      n16                            0x1
#define      n7                             0x2
#define      tint                           0
#define      n17                            0x0a
#define      n19                            0x05
#define      n18                            0x00
#define      breeze_way                     0x16
#define      n21                            0x3ff
#define      front_porch                    0x0c    //pal 0x0c    ntsc 0x10 
#define      n11                            0x7ce
#define      n12                            0x000
#define      activeline                     1440
#define      firstvideoline                0x05//0x0e    
#define      uv_order                       0
#define      pal_mode                       1       //pal 0x1    ntsc 0x0 
#define      invert_top                     0
#define      sys625_50                      0
#define      cphase_rst                     3
#define      vsync5                         1
#define      sync_level                     0x48
#define      n22                            0
#define      agc_pulse_level                0xa3
#define      bp_pulse_level                 0xc8
#define      n4                             0x15
#define      n6                             0x05
#define      n2                             0x15
#define      vbi_blank_level                0x128
#define      soft_rst                       0
#define      row63                          0
#define      row64                          0x07
#define      wss_clock                      0x2f7
#define      wss_dataf1                     0
#define      wss_dataf0                     0
#define      wss_linef1                     0
#define      wss_linef0                     0
#define      wss_level                      0x3ff
#define      venc_en                        1
#define      uv_first                       0
#define      uv_flter_en                    1
#define      notch_en                       0
#define      notch_wide                     0
#define      notch_freq                     0
#define      row78                          0
#define      row79                          0
#define      row80                          0

	LCD_TV_PARAM0_reg = chroma_freq_palbg;//chroma_freq_ntsc ;//此处定义 N制 P制
	LCD_TV_PARAM1_reg = chroma_bw_1<<27 | comp_yuv<<26|compchgain<<24|yc_delay<<17|cvbs_enable<<16|clrbar_sel<<10|clrbar_mode<<9|
	                    bypass_yclamp<<8 | chroma_phase ;

    LCD_TV_PARAM2_reg = cb_burst_amp<<24 | back_porch<<16 | burst_width<<8 | hsync_width;
    LCD_TV_PARAM3_reg = black_level<< 16 | slave_mode<<8 | cr_burst_amp ;
    LCD_TV_PARAM4_reg = n3<<24  | n1<<16 | blank_level ;
    LCD_TV_PARAM5_reg = n10<<24 | n9<<16 | n8 ;
    LCD_TV_PARAM6_reg = num_lines ;
    LCD_TV_PARAM7_reg = n15<<24 | n14<<16| n13<<8 | n0 ;
    LCD_TV_PARAM8_reg = cb_gain<<24 | white_level<<8 | n5 ;
    LCD_TV_PARAM9_reg = n7<<24      | n16 <<16 | cr_gain<<8 | n20 ;
    LCD_TV_PARAM10_reg= n18<<24     | n19 <<16 | n17<<8     | tint ;
    LCD_TV_PARAM11_reg= front_porch<<24     | n21<<8       | breeze_way ;
    LCD_TV_PARAM12_reg= n12 <<16     | n11 ;
    LCD_TV_PARAM13_reg= activeline ;
    LCD_TV_PARAM14_reg= n22<<24     | sync_level <<16 | uv_order<<15|pal_mode<<14|chroma_bw_0<<13|invert_top<<12|sys625_50<<11|
                        cphase_rst<<9|vsync5<<8     | firstvideoline ;
    LCD_TV_PARAM15_reg= n6<<24      | n4 <<16  | bp_pulse_level<<8     | agc_pulse_level ;
    LCD_TV_PARAM16_reg= soft_rst<<24| vbi_blank_level<<8    | n2 ;
    LCD_TV_PARAM17_reg= row64 <<16  | wss_clock ;
    LCD_TV_PARAM18_reg= wss_dataf1 ;
    LCD_TV_PARAM19_reg= wss_dataf0 ;
    LCD_TV_PARAM20_reg= wss_level <<16 | wss_linef0<<8     | wss_linef1 ;
    LCD_TV_PARAM21_reg= row80<<24 | row79<<16 | row78<<8 | venc_en<<7     | uv_first <<6 | uv_flter_en<<5  |notch_en<<4 |  notch_wide<<3 | notch_freq ;
}

static void  tv_pal_screen_init(void)
{
	unsigned int val;
	
	OS_EnterRegion ();	// 禁止任务切换
	Lcd_ClkDisable();
	sys_clk_disable (Tv_out_lcd_pixel_clk_enable);
	sys_soft_reset (softreset_lcd);
	Lcd_ClkEnable ();	
	sys_clk_enable (Tv_out_lcd_pixel_clk_enable);
	OS_LeaveRegion ();	
	
	// 297/11 = 27MHz
	rSYS_LCD_CLK_CFG = (1 << 0) | (11 << 3) | (2 << 15) | (1 << 9);
	
	rSYS_ANALOG_REG1 |= (1 << 5);
	LCD_TV_PARAM16_reg |= (1 << 24);
	delay (10000);
	LCD_TV_PARAM16_reg &= ~(1 << 24);
	delay (10000);
	tvencoder_init_pal();
}

void  HW_tv_pal_init(void)
{
	tv_pal_osd_init ();
	tv_pal_screen_init ();
}

void HW_tv_pal_exit (void)
{
	
}

#endif //#ifdef    _SUPPORT_TV_PAL_
