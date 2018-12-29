#include  "hardware.h"
#include  "lcd.h"
#include "rtos.h"
#include "gpio.h"
#include "xm_dev.h"

//#if DEMO_BB || DEV_BB_176
#define	_SUPPORT_SRGB_
//#endif

// 开发板
// 1) 关闭 DEMO_BB
//	2) 对应的SRGB模组定义 

// 样机板
//	1) 定义 DEMO_BB 
//	2) 对应的SRGB模组定义 

#define	SRGB_GCX133ALC		1		// GCX133ALC V2
#define	SRGB_JYD027			2		// JYD027-9624-8961-A0

//#define	SRGB_TYPE 		SRGB_JYD027
#define	SRGB_TYPE 		SRGB_GCX133ALC
#define	LCD_BL_GPIO			GPIO31


#ifdef _SUPPORT_SRGB_



//  54M HZ
//  320*240
//  GCX133ALC V2

void SRGB_GCX133ALC_LCD_Init()
{
#define      lcd_enable             1//	0// 1
#define      screen_width           320//320
#define      rgb_pad_mode           3		// 0 RGB / 5 BGR
#define      direct_enable          0
#define      mem_lcd_enable         0 
#define      range_coeff_y          0
#define      range_coeff_uv         0
#define      lcd_done_intr_enable   1   //使能中断=1 禁止中断=0 lcd_done_intr
#define      dac_for_video          0
#define      dac_for_cvbs           0 

          

#define      lcd_interlace_flag     0
#define      screen_type            2 	// 
#define      VSW1_enable            0 
#define      LcdVComp               0
#define      itu_pal_ntsc           0
#define      hsync_ivs              0
#define      vsync_ivs              0
#define      lcd_ac_ivs             0
#define      test_on_flag           0

//#define      back_color             (200<<16)|(100<<8)|(16)
//#define      back_color             (137<<16)|(143<<8)|(105)	// 无背景色
#define      back_color             (0<<16)|(0<<8)|(0)	// 无背景色	
#define      cpu_screen_csn_bit     8//0
#define      cpu_screen_wen_bit     9
	
//#define      DEN_h_rise             27    // data enable ?ú????é?éyμ?????
//#define      DEN_h_fall             (27+320*1)  //320
	
#define      DEN_h_rise             01    // data enable ?ú????é?éyμ?????
#define      DEN_h_fall             (01+320*1)  //320
	
	
#if	0//def OLD
#define      CPL                    404 //(350)   // clock cycles per line  max 4096   
#define      LPS                    262 //(506)      // Lines per screen value  
#else
#define      CPL                    425// 480	//404	// 404 //(350)   // clock cycles per line  max 4096   
#define      LPS                    314//416	//(302)	// 262 //(506)      // Lines per screen value  
#endif
	
#define      DEN0_v_rise            11   // even field data enable é?éy????
#define      DEN0_v_fall            (LPS - DEN0_v_rise)	//251//500  
#define      DEN1_v_rise            11//336 
#define      DEN1_v_fall            (LPS - DEN0_v_rise)	//251//500//624

#define      HSW_rise                  ( 4+324     )
#define      HSW_fall                  ( 4 +4 - 0 )

#define      VSW0_v_rise               ( 1      )   // Vertical sync width value    ;  from register   timing1 10
#define      VSW0_v_fall               ( 3       )   // Vertical sync width value     ;  from register   timing1 10

#define      VSW0_h_rise                1 
#define      VSW0_h_fall                (20+300-0) ///320

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

#define      osd_0_global_coeff_enable      1
#define      osd_0_enable                   0
#define      osd_0_binary_enable            0
#define      osd_0_format                   1//0 : YUV420, 1 : ARGB8888, 2: RGB565, 3: RGB454
#define      osd_0_width                    320///320
#define      osd_0_height                   240//480
#define      osd_0_vc1_range_map            0
#define      osd_0_chroma_disable           0
#define      osd_0_h_position               0
#define      osd_0_v_position               0
#define      osd_0_global_coeff             255//设置透明度 0不透明 <~~~> 255 全透明
#define      osd_0_y_addr                   0//设置为全透明色之后，影响因子osd_0_mult_coef起作用？？
#define      osd_0_u_addr                   0
#define      osd_0_v_addr                   0
#define      osd_0_stride                   320//320//1280// 源  宽度
#define      osd_0_hoffset                  0
#define      osd_0_mult_coef                63

#define      osd_1_global_coeff_enable      0
#define      osd_1_enable                   0
#define      osd_1_binary_enable            0
#define      osd_1_format                   0//0 : YUV420, 1 : ARGB8888, 2: RGB565, 3: RGB454
#define      osd_1_width                    320
#define      osd_1_height                   240
#define      osd_1_vc1_range_map            0
#define      osd_1_chroma_disable           0
#define      osd_1_h_position               0
#define      osd_1_v_position               100
#define      osd_1_global_coeff             255 //设置透明度
#define      osd_1_y_addr                   0
#define      osd_1_u_addr                  0
#define      osd_1_v_addr                  0
#define      osd_1_stride                  320
#define      osd_1_hoffset                 0
#define      osd_1_mult_coef               63

#define      osd_2_global_coeff_enable      0
#define      osd_2_enable                   0
#define      osd_2_binary_enable            0
#define      osd_2_format                   0
#define      osd_2_width                    320
#define      osd_2_height                   480
#define      osd_2_vc1_range_map            0
#define      osd_2_chroma_disable           0
#define      osd_2_h_position               0
#define      osd_2_v_position               0
#define      osd_2_global_coeff             255 //设置透明度 0不透明 <~~~> 64 全透明
#define      osd_2_y_addr                   0
#define      osd_2_u_addr                  0
#define      osd_2_v_addr                  0
#define      osd_2_stride                  320
#define      osd_2_hoffset                 0
#define      osd_2_mult_coef               63
	
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
	
	LCD_PARAM0_reg  = (1<<17);
	LCD_PARAM0_reg  = 0;
	LCD_PARAM0_reg  = (1<<17);
	LCD_PARAM0_reg  = 0;//bit 26 enable lcd_done_intr 
	LCD_PARAM0_reg  = (0<<26);
	
	LCD_TVOUT_PARAM3_reg  = (1<<11)|(3<<12) ;
	lcd_param0  = lcd_enable   
					| (screen_width << 1)
					| (rgb_pad_mode << 13)
					| (direct_enable  << 16)			// LCD场方向任意位置中断使能，高有效。
					| (mem_lcd_enable << 17)
					| (range_coeff_y  << 18)
					| (range_coeff_uv << 22)
					| (lcd_done_intr_enable << 26)
					| (dac_for_video  << 27)			// 送给DAC的数据选择为YPbPr，当输出为YPbPr时，该位置1。
					| (dac_for_cvbs   << 28)			// 送给DAC的数据选择为CVBS，当输出为CVBS时，该位置1。
					| (1 << 30)								// 场同步中断使能，1有效。
					| (stop_lcd << 31)  					// LCD停止，在lcd_done时，disable lcd_enable信号。
					;
	
	lcd_param1  = ( lcd_interlace_flag     <<  0 ) 
					| ( screen_type     			<<  1 ) 	// 输出屏类型选择，当输出为CVBS，YPbPr，VGA时，选择并行数据屏类型。
																	//		010：串行Srgb屏；
					| ( VSW1_enable     			<< 13 ) 	//	奇偶场时，用来表示另外一场的使能，因此，当隔行输出时，该位置1。
					| ( LcdVComp     				<<	14 ) 
					| ( itu_pal_ntsc     		<< 15 ) 
					| ( hsync_ivs    				<< 17 ) 	// 输出行同步信号是否取反选择，当为CPU屏时表示是否将cpu_screen_rd取反
																	//		1：取反；0：不取反。
					| ( vsync_ivs     			<< 18 ) 	// 输出场同步信号是否取反选择，当为CPU屏时表示是否将cpu_screen_rs取反
																	//		1：取反；0：不取反。
					| ( lcd_ac_ivs     			<< 19 ) 	// 输出有效数据信号data_en是否取反选择，当为CPU屏时表示是否将cpu_screen_cs取反
																	//		1：取反；0：不取反。
					| ( 1  							<< 21 ) 
					| ( 1 							<< 23 ) 
					| ( test_on_flag     		<< 31 )  
					;
	
	lcd_param2  = 	( back_color     		<< 0 ) 
					|	( cpu_screen_csn_bit << 24) 
					|	( cpu_screen_wen_bit << 28)
					;
   
	lcd_param3  =	( DEN_h_rise      <<	0  )  // 数据使能信号data_en水平起始位置
					|	( DEN_h_fall      <<	12 )  // 数据使能信号data_en水平结束位置
					;
	
	lcd_param4  = 	( DEN0_v_rise     << 0  )  // 数据使能信号data_en垂直起始位置, 
															//		逐行时序时，为每帧的垂直时序，隔行时序时，为第一场的时序。
					|	( DEN0_v_fall     << 12 ) 	// 数据使能信号data_en垂直结束位置，
															//		逐行时序时，为每帧的垂直时序，隔行时序时，为第一场的时序。
					;
	
	lcd_param5  =	( DEN1_v_rise     <<	0  )	// 数据使能信号data_en垂直起始位置，
															//		当隔行输出时（VSW1_enable=1）有效，代表的是第二场的垂直起始位置，逐行输出时无效。
					|	( DEN1_v_fall     <<	12 ) 	// 数据使能信号data_en垂直结束位置，
															//		当隔行输出时（VSW1_enable=1）有效，代表的是第二场的垂直结束位置，逐行输出时无效。
					;
	
	lcd_param6  = 	( CPL    <<	0  )               // clock cycles per line  max 4096, 每行的总点数 
					|	( LPS    <<	12 )               // Lines per screen value  每帧的总行数，隔行时序时为奇偶场的总行数。
					;	
	
	lcd_param7  =	( HSW_rise       <<	0  )		// 行同步信号hsyn上升沿位置
					|	( HSW_fall       <<	12 ) 		// 行同步信号hsyn下降沿位置
					;
	
	lcd_param8  =	( VSW0_v_rise    <<	0  )     // 场同步信号vsyn0上升沿位置，逐行时序时为场同步，隔行时序时为第一场的场同步。
					|	( VSW0_v_fall    <<	12 )     // 场同步信号vsyn0下降沿位置，逐行时序时为场同步，隔行时序时为第一场的场同步。
					;
	
	lcd_param9  =	( VSW0_h_rise   	<<	0  )		// 场同步信号vsyn0上升沿位置，
																//		逐行时序时为场同步，隔行时序时为第一场的场同步。
																//		通过VSW0_h_rise  VSW0_h_fall可以调节场同步的具体位置，且奇偶场也可以出半行。
					|	( VSW0_h_fall   	<< 12 ) 		// 场同步信号vsyn0下降沿位置，
																//		逐行时序时为场同步，隔行时序时为第一场的场同步。
																//		通过VSW0_h_rise  VSW0_h_fall可以调节场同步的具体位置，且奇偶场也可以出半行。
					;   
	
	lcd_param10 =	( VSW1_v_rise   	<<	0  )		// 场同步信号vsyn1上升沿位置，
																//		逐行时序时无效(VSW1_enable=0)，
																//		隔行时序时为第二场的场同步（VSW1_enable=1）。
					| 	( VSW1_v_fall   	<<	12 ) 		// 场同步信号vsyn1下降沿位置，
																//		逐行时序时无效(VSW1_enable=0)，
																//		隔行时序时为第二场的场同步（VSW1_enable=1）。
					;   
	
	lcd_param11 =	( VSW1_h_rise    	<<	0  )		// 场同步信号vsyn1上升沿位置，
																//		逐行时序（VSW1_enable=0）时无效，隔行时序(VSW1_enable=1)时为第一场的场同步。
																//		通过VSW1_h_rise  VSW1_h_fall可以调节场同步的具体位置，且奇偶场也可以出半行。
					|	( VSW1_h_fall    	<<	12 ) 		// 场同步信号vsyn1下降沿位置，
																//		逐行时序（VSW1_enable=0）时无效，隔行时序(VSW1_enable=1)时为第一场的场同步。
																//		通过VSW1_h_rise  VSW1_h_fall可以调节场同步的具体位置，且奇偶场也可以出半行。
					;   
	
	// RGB to YCbCr coef0，计算Y时 R G B的系数。
	//	Y =（ R * RGB2YUV_coeff00 + G * RGB2YUV_coeff01 + B * RGB2YUV_coeff02）/256 + Rgb2yuv_mode_sel * 16;
	lcd_param12 = ( 66 	<< 0  )			// RGB到YUV转换时，R的系数。
					| ( 129 	<< 9  ) 			// RGB到YUV转换时，G的系数。
					| ( 25  	<< 18 )			// RGB到YUV转换时，B的系数。
					;
	
	// RGB to YCbCr coef1，计算Cb时 R G B的系数。
	//	Cb =（ - R * RGB2YUV_coeff10 - G * RGB2YUV_coeff11 + B * RGB2YUV_coeff12）/256 + 128
	lcd_param13 = (  38 << 0  )			// RGB到YUV转换时，R的系数。
					| (  74 << 9  )			// RGB到YUV转换时，G的系数。
					| ( 112 << 18 )			// RGB到YUV转换时，B的系数。
					;
	
	lcd_param14  = 112 | 94<<9 | 18<<18; 
	
	lcd_param15  = 298 | 0<<9 | 409<<18;
	
	lcd_param16  = 299 |  100<<9 | 208<<18;
	
	lcd_param17  = 298 | 517<<9 | 0<<19; 
	
	lcd_param18 = (sav_pos    << 0  )   		// ITU656有效数据起始位置
					;
	
	lcd_param19 = ( f1_vblk_start  <<  0 )		// 第一场场消隐开始位置
					| ( f1_vblk_end    << 12 ) 	// 第一场场消隐结束位置
					;
	
	lcd_param20 = ( f2_vblk_start  <<  0 ) 	// 第二场场消隐开始位置
					| ( f2_vblk_end    << 12 )  	// 第二场场消隐结束位置
					;
	
	lcd_param21 = ( f2_start   << 0  )			// 奇偶场标志（field=1）起始位置
					| ( f2_end   	<< 12 )			// 奇偶场标志（field=1）结束位置
					;
	
	lcd_param22 = ( binary_thres  << 16 )
					| ( binary_big   	<<  8 )
					| ( binary_small  <<  0 ) 
					;
	
	
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
   osd_0_param2  = osd_0_y_addr;
   osd_0_param3  = 0;
   osd_0_param4  = 0;
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
	osd_1_param2  =0;
	osd_1_param3  =0; 
   osd_1_param4  =0; 
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
	osd_2_param2  =0 ;
	osd_2_param3  =0 ; 
	osd_2_param4  =0 ; 
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
	LCD_PARAM15_reg    = lcd_param15 ;
	LCD_PARAM16_reg    = lcd_param16 ;
	LCD_PARAM17_reg    = lcd_param17 ;
	
	LCD_PARAM18_reg    = lcd_param18 ;
	LCD_PARAM19_reg    = lcd_param19 ;
	LCD_PARAM20_reg    = lcd_param20 ;
	LCD_PARAM21_reg    = lcd_param21 ;
	LCD_PARAM22_reg    = lcd_param22 ;
	//  lcd_param1 = LCD_PARAM1_reg      ;
	
	
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
	
	//LCD_UPDATA_reg     = 0x1; //updata reg
}	// #ifdef _SUPPORT_SRGB_


//35 000 000 = 35 M
static void Delay(int n){
	while(n--);
}
//35 000 000 = 35 M
static void Delay1ms(void){
	int val=35000>>2;
	while(val--);
}

static void Delayms(int n){
	OS_Delay (n);
	//int val=n;
	//while(val--)
	//	Delay1ms();
}


#define	LCD_RESET_GPIO		GPIO93
#define	LCD_SDA_GPIO		GPIO81
#define	LCD_SCL_GPIO		GPIO80
#define	LCD_CS_GPIO			GPIO92

#if	SRGB_TYPE == SRGB_GCX133ALC	//DEV_BB_176

	//        reset    sda     csb     scl
	// gpio    93      80      92      81 
#define	CS_H			rGPIO_PC_RDATA |=  ((0x1) << (92-64))
#define	CS_L			rGPIO_PC_RDATA &= ~((0x1) << (92-64))
#define	RESET_H		rGPIO_PC_RDATA |=  ((0x1) << (93-64))
#define	RESET_L		rGPIO_PC_RDATA &= ~((0x1) << (93-64))
#define	SDA_H			rGPIO_PC_RDATA |=  ((0x1) << (81-64))
#define	SDA_L			rGPIO_PC_RDATA &= ~((0x1) << (81-64))
#define	SCL_H			rGPIO_PC_RDATA |=  ((0x1) << (80-64))
#define	SCL_L			rGPIO_PC_RDATA &= ~((0x1) << (80-64))

#endif

void hw_setCs(int cs)
{
	// ASIC 评估板
	//        reset    sda     csb     scl
	// gpio    93   	81      92      90 
	if(cs)
		CS_H;
	else
		CS_L;
}


	//        reset    sda     csb     scl
	// gpio    22      23      24      25 
	// bit     22      23      24      25
	//val&=~((1<<22)|(1<<23)|(1<<24)|(1<<25));

	// ASIC 评估板
	//        reset    sda     csb     scl
	// gpio    93   	81      92      90 
void SetDout(unsigned char Data)
{
	#define SPI_DELAY    0X05

	unsigned char i;
	unsigned char tmp;
	unsigned int val;
	tmp = Data;
	Delay(20);

	
	// ASIC 评估板
	//        reset    sda     csb     scl
	// gpio    93   	81      92      90 
	
	for (i = 0; i < 8; i++)
	{
		//hw_setClk(0);  
		SCL_L;
		if (tmp & 0x80)	
		{
			SDA_H;
		  //hw_setDout(1); 
			//      reset              sda              csb              scl
		  // val = (1 << (70-64)) | (1 << (73-64)) | (0 << (72-64)) | (0 << (71-64));
	   }
		else
		{
			SDA_L;
			//val = (1 << (70-64)) | (0 << (73-64)) | (0 << (72-64)) | (0 << (71-64));
		  //hw_setDout(0);
		}
		//rGPIO_PC_RDATA = val;
		Delay(SPI_DELAY);
		//val |= (1 << (71-64));
		//rGPIO_PC_RDATA = val;
		//hw_setClk(1);
		SCL_H;
		tmp <<= 1;
		Delay(SPI_DELAY);
	}
		
	Delay(20);	
}

#define LCDCtrlRegWrite(date)           hw_setCs(0);\
                                        SetDout((unsigned char)(date>>8));\
                                        SetDout((unsigned char)(date&0x00ff));\
                                        hw_setCs(1);


#define	SRGB_WORD(addr,data)		( (((addr) & 0xFF) << 8) | ((data) & 0xFF) )


void Pannel_SPI_Write(void)
{  	
#if SRGB_TYPE == SRGB_GCX133ALC
		LCDCtrlRegWrite(0x1080);
		LCDCtrlRegWrite(0x1100);
		LCDCtrlRegWrite(0x1210);
		LCDCtrlRegWrite(0x1324);
		LCDCtrlRegWrite(0x141c);
		LCDCtrlRegWrite(0x1580);
		LCDCtrlRegWrite(0x1620);
		LCDCtrlRegWrite(0x1720);
		LCDCtrlRegWrite(0x1820);
		LCDCtrlRegWrite(0x30f4);
		Delayms(10);
		LCDCtrlRegWrite(0x3144);
		LCDCtrlRegWrite(0x3244);
		LCDCtrlRegWrite(0x3320);
		LCDCtrlRegWrite(0x342f);
		LCDCtrlRegWrite(0x3528);
		LCDCtrlRegWrite(0x3615);
		LCDCtrlRegWrite(0x3708);
		LCDCtrlRegWrite(0x3812);
		Delayms(100);
		LCDCtrlRegWrite(0x3920);
		Delayms(100);
		LCDCtrlRegWrite(0x3af4);

#endif		
}	

void  rSRGB_CFG_reg_Init()
{
#if SRGB_TYPE == SRGB_GCX133ALC
	// Line 1, 3, 5  RGB
	// Line 2, 4, 6  GBR
	rSRGB_CFG 	= (0 << 11)
					| (0 << 0)
					| (2 << 2)		// 010:BGR
					| (5 << 5)		// 101:GBR
					;		 
#endif
	//rCPU_SCR_CTRL_REG = 0x1;
}

void SRGB_Sel_SPI_Pad()
{
	unsigned int val;
	//        reset    sda     csb     scl
	// gpio    93   	81      92      80 
#if SRGB_TYPE == SRGB_GCX133ALC
	val = rGPIO_PC_MOD;
	val = val & (~((0x01 << (80-64)) | (0x01 << (81-64)) | (0x01 << (92-64))|(0x01 << (93-64))) );
	rGPIO_PC_MOD = val;
	
	// ASIC 评估板
	//        reset    sda     csb     scl
	// gpio    70      73      72      71 
	val = rSYS_PAD_CTRL08;
	// GPIO80 GPIO81
	val &= ~((0x03) | (0x03 << 2));
	val |=  ((0x00) | (0x00 << 2));
	rSYS_PAD_CTRL08 = val;
	
	val = rSYS_PAD_CTRL08;
	// GPIO92 GPIO93
	val &= ~((0x03 << 24) | (0x03 << 26));
	val |=  ((0x00 << 24) | (0x00 << 26));
	rSYS_PAD_CTRL08 = val;
#endif
}

void  SRGB_Reset()
{
#if SRGB_TYPE == SRGB_JYD027
	
#elif SRGB_TYPE == SRGB_GCX133ALC
	// ASIC 评估板
	//        reset    sda     csb     scl
	// gpio    93   	81      92      90 
	// Reset 
	rGPIO_PC_RDATA |=  (1 << (93-64)); // reset 
	Delayms(20);
	rGPIO_PC_RDATA &= ~(1 << (93-64)); // reset 
	Delayms(50);
	rGPIO_PC_RDATA |=  (1 << (93-64)); // reset 
	Delayms(20);
#endif
}


void SRGB_clock_init (void)
{
	// clock
	// b001: syspll_clk
	// Tvout_lcd _clk_wire_div 4
	// Tvout_lcd_pixel_clk_wire_div 1
	//rSYS_LCD_CLK_CFG = (0 << 0) | (14 << 3) | (1 << 8) |(1 << 9) | (3 << 15);
	// 240000000 / 14 = 17142857
	// old = 17142857 / (404 * 262) = 162
	// new = 17142857 / (600 * 470) = 60.79		// 节省0.4帧资源(USB 720P)

	// 24-18	R/W	0	
	// TVOUT_LCD_OUT DLY	
	
	// 17-15	R/W	0x4	
	// Tvout_lcd_pixel_clk_wire_div 
	// Tvout_lcd_pixel_clk_wire = tvout_lcd_clk / (Tvout_lcd_pixel_clk_wire_div ? Tvout_lcd_pixel_clk_wire_div : 1);
	
	// 9	R/W	0x0	
	//	Tvout_lcd_pixel_clk 0 normal  1 inv	
	
	// 8	R/W	0x0	
	//	Tvout_lcd_clk  0 normal  1 inv
	
	// 7-3	R/W	0x40	
	// Tvout_lcd _clk_wire_div
	//		Tvout_lcd_clk  = IntTvClkSwitch / (Tvout_lcd_pixel_clk _div ? Tvout_lcd_pixel_clk_div : 1);
	
	// 2-0	R/W	0x4	
	//	IntTvClkSwitch_sel：IntTvClkSwitch
	// 	3'b000: clk_240m
	// 	3'b001: syspll_clk 
	// 	3'b010: audpll_clk
	// 	3'b100: clk_24m
	// 24000000 / 2 = 12000000
	// 12000000/(404*302)=
	
	rSYS_LCD_CLK_CFG = (4 << 0) | (2 << 3) | (1 << 8) |(1 << 9) | (3 << 15);
}


void srgb_SelPad(void)
{
   unsigned int val;

	XM_lock ();
	// D0 ~ D7
	val = rSYS_PAD_CTRL06;
	val &= 0xFF000000;
	val |= 1<<21|1<<18|1<<15|1<<12|1<<9|1<<6|1<<3|1<<0;
	rSYS_PAD_CTRL06 = val;
	
	// clk vsync hsync
	val = rSYS_PAD_CTRL07;
	val &= ~((0x7 << 15) | (0x7 << 18) | (0x7 << 21));
	val |= (1 << 21) | (1 << 18) | (1 << 15);
	rSYS_PAD_CTRL07 = val;	
	
	XM_unlock ();
	
}

void SRGB_GCX133ALC_Screen_Init()
{
	srgb_SelPad ();
	SRGB_clock_init ();
	rSRGB_CFG_reg_Init();
	SRGB_Sel_SPI_Pad();
	SRGB_Reset();
	Pannel_SPI_Write();
}



void HW_LCD_Init (void)
{
    printf("SRGB_320X240 \n");
	OS_EnterRegion ();	// 禁止任务切换
	Lcd_ClkDisable();
	sys_soft_reset (softreset_lcd);
	Lcd_ClkEnable ();	
	OS_LeaveRegion ();	
	SRGB_GCX133ALC_Screen_Init ();
	SRGB_GCX133ALC_LCD_Init ();
}

// SRGB屏关闭退出
void HW_SRGB_Exit(void)
{
	/*
	int i;
	// Lcd_enable
	//	LCD使能信号，为1时LCD模块工作，为0时，LCD模块不工作。
	// OS_IncDI();    // Initially disable interrupts
	OS_EnterRegion ();	// 禁止任务切换
	// 等待帧刷新结束
	LCD_INTR_CLR_reg = 0xFFFFFFFF;
	while(!(LCD_STATUS_reg & 0x01));		// bit1: lcd_done_intr(每场结束中断) ,
	// 禁止LCDC
	LCD_PARAM0_reg &= ~1;
	Lcd_ClkDisable();
	//OS_DecRI();    // Initially disable interrupts
	OS_LeaveRegion ();	// 禁止任务切换
	*/
}

// 开启背光
void HW_LCD_BackLightOn (void)
{
	// GPIO76
	// [14:12]	lcd_de	lcd_de_pad	itu_b_din11输入 /输出GPIO76
	unsigned int val;
	
		
	XM_lock ();
	
	val = rSYS_PAD_CTRL03;
	val &= ~(0x3 << 3);
	//val |= (1 << 30);		// 设置该位使能输出
	rSYS_PAD_CTRL03 = val;
	XM_unlock ();
		
	OS_EnterRegion ();	// 禁止任务切换
	SetGPIOPadDirection (LCD_BL_GPIO, euOutputPad);
	//if(LCD_get_enable())
	SetGPIOPadData (LCD_BL_GPIO, euDataHigh);
	//else
	//	SetGPIOPadData (LCD_BL_GPIO, euDataLow);
	OS_LeaveRegion ();	// 禁止任务切换
}

// 关闭背光
void HW_LCD_BackLightOff (void)
{
	unsigned int val;
	
	XM_lock ();
		val = rSYS_PAD_CTRL03;
	val &= ~(0x3 << 3);
	//val |= (1 << 30);		// 设置该位使能输出
	rSYS_PAD_CTRL03 = val;
	
	XM_unlock ();
	
	OS_EnterRegion ();	// 禁止任务切换
	SetGPIOPadDirection (LCD_BL_GPIO, euOutputPad);
	SetGPIOPadData (LCD_BL_GPIO, euDataLow);
	OS_LeaveRegion ();	// 禁止任务切换
}

void HW_LCD_ROTATE (int Select)
{
    if(Select){
        LCDCtrlRegWrite(0x1103);
    }else {
       LCDCtrlRegWrite(0x1100);
    }
}

u32_t Get_HW_LCD_VP(void) {
    return VP_ADJUSTMent_reg;
    
}

void  Set_HW_LCD_VP(int bright,int saturation,int contrast) {

    VP_ADJUSTMent_reg = bright | (contrast << 8) |(saturation <<16);
}

#endif       // #ifdef _SUPPORT_SRGB_
