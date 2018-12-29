#include  "hardware.h"
#include  "lcd.h"
#include "rtos.h"
#include "gpio.h"
#include "xm_dev.h"


// ������
// 1) �ر� DEMO_BB
//	2) ��Ӧ��SRGBģ�鶨�� 

// ������
//	1) ���� DEMO_BB 
//	2) ��Ӧ��SRGBģ�鶨�� 


#define LCD_BL_GPIO     GPIO31

#define	SRGB_A035QN05		1		// A035QN05 V2
#define	SRGB_JYD027			2		// JYD027-9624-8961-A0

#define	SRGB_TYPE 		SRGB_JYD027
//#define	SRGB_TYPE 		SRGB_A035QN05

//  54M HZ
//  320*240
//  A035QN05 V2

void SRGB_A035QN05_LCD_Init()
{
#define      lcd_enable             1//	0// 1
#define      screen_width           320//320
#define      rgb_pad_mode           1		// 0 RGB / 5 BGR
#define      direct_enable          0
#define      mem_lcd_enable         0 
#define      range_coeff_y          0
#define      range_coeff_uv         0
#define      lcd_done_intr_enable   1   //ʹ���ж�=1 ��ֹ�ж�=0 lcd_done_intr
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
//#define      back_color             (137<<16)|(143<<8)|(105)	// �ޱ���ɫ
#define      back_color             (0<<16)|(0<<8)|(0)	// �ޱ���ɫ	
#define      cpu_screen_csn_bit     8//0
#define      cpu_screen_wen_bit     9
	
//#define      DEN_h_rise             27    // data enable ?��????��?��y��?????
//#define      DEN_h_fall             (27+320*1)  //320
	
#define      DEN_h_rise             1    // data enable ?��????��?��y��?????
#define      DEN_h_fall             (1+320*1)  //320
	
	
#if	0//def OLD
#define      CPL                    404 //(350)   // clock cycles per line  max 4096   
#define      LPS                    262 //(506)      // Lines per screen value  
#else
#define      CPL                    340// 480	//404	// 404 //(350)   // clock cycles per line  max 4096   
#define      LPS                    280//416	//(302)	// 262 //(506)      // Lines per screen value  
#endif
	
#define      DEN0_v_rise            3   // even field data enable ��?��y????
#define      DEN0_v_fall            (240 + DEN0_v_rise)	//251//500  
#define      DEN1_v_rise            3//336 
#define      DEN1_v_fall            (240 + DEN0_v_rise)	//251//500//624

#define      HSW_rise                  ( 10+324     )
#define      HSW_fall                  ( 10 +4 - 0 )

#define      VSW0_v_rise               ( 1      )   // Vertical sync width value    ;  from register   timing1 10
#define      VSW0_v_fall               ( 3       )   // Vertical sync width value     ;  from register   timing1 10

#define      VSW0_h_rise                1 
#define      VSW0_h_fall                (VSW0_h_rise+320-0) ///320

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
#define      osd_0_global_coeff             255//����͸���� 0��͸�� <~~~> 255 ȫ͸��
#define      osd_0_y_addr                   0//����Ϊȫ͸��ɫ֮��Ӱ������osd_0_mult_coef�����ã���
#define      osd_0_u_addr                   0
#define      osd_0_v_addr                   0
#define      osd_0_stride                   320//320//1280// Դ  ���
#define      osd_0_hoffset                  0
#define      osd_0_mult_coef                63

#define      osd_1_global_coeff_enable      0
#define      osd_1_enable                   0
#define      osd_1_binary_enable            0
#define      osd_1_format                   1//0 : YUV420, 1 : ARGB8888, 2: RGB565, 3: RGB454
#define      osd_1_width                    320
#define      osd_1_height                   240
#define      osd_1_vc1_range_map            0
#define      osd_1_chroma_disable           0
#define      osd_1_h_position               0
#define      osd_1_v_position               100
#define      osd_1_global_coeff             255 //����͸����
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
#define      osd_2_global_coeff             255 //����͸���� 0��͸�� <~~~> 64 ȫ͸��
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
					| (direct_enable  << 16)			// LCD����������λ���ж�ʹ�ܣ�����Ч��
					| (mem_lcd_enable << 17)
					| (range_coeff_y  << 18)
					| (range_coeff_uv << 22)
					| (lcd_done_intr_enable << 26)
					| (dac_for_video  << 27)			// �͸�DAC������ѡ��ΪYPbPr�������ΪYPbPrʱ����λ��1��
					| (dac_for_cvbs   << 28)			// �͸�DAC������ѡ��ΪCVBS�������ΪCVBSʱ����λ��1��
					| (1 << 30)								// ��ͬ���ж�ʹ�ܣ�1��Ч��
					| (stop_lcd << 31)  					// LCDֹͣ����lcd_doneʱ��disable lcd_enable�źš�
					;
	
	lcd_param1  = ( lcd_interlace_flag     <<  0 ) 
					| ( screen_type     			<<  1 ) 	// ���������ѡ�񣬵����ΪCVBS��YPbPr��VGAʱ��ѡ�������������͡�
																	//		010������Srgb����
					| ( VSW1_enable     			<< 13 ) 	//	��ż��ʱ��������ʾ����һ����ʹ�ܣ���ˣ����������ʱ����λ��1��
					| ( LcdVComp     				<<	14 ) 
					| ( itu_pal_ntsc     		<< 15 ) 
					| ( hsync_ivs    				<< 17 ) 	// �����ͬ���ź��Ƿ�ȡ��ѡ�񣬵�ΪCPU��ʱ��ʾ�Ƿ�cpu_screen_rdȡ��
																	//		1��ȡ����0����ȡ����
					| ( vsync_ivs     			<< 18 ) 	// �����ͬ���ź��Ƿ�ȡ��ѡ�񣬵�ΪCPU��ʱ��ʾ�Ƿ�cpu_screen_rsȡ��
																	//		1��ȡ����0����ȡ����
					| ( lcd_ac_ivs     			<< 19 ) 	// �����Ч�����ź�data_en�Ƿ�ȡ��ѡ�񣬵�ΪCPU��ʱ��ʾ�Ƿ�cpu_screen_csȡ��
																	//		1��ȡ����0����ȡ����
					| ( 1  							<< 21 ) 
					| ( 1 							<< 23 ) 
					| ( test_on_flag     		<< 31 )  
					;
	
	lcd_param2  = 	( back_color     		<< 0 ) 
					|	( cpu_screen_csn_bit << 24) 
					|	( cpu_screen_wen_bit << 28)
					;
   
	lcd_param3  =	( DEN_h_rise      <<	0  )  // ����ʹ���ź�data_enˮƽ��ʼλ��
					|	( DEN_h_fall      <<	12 )  // ����ʹ���ź�data_enˮƽ����λ��
					;
	
	lcd_param4  = 	( DEN0_v_rise     << 0  )  // ����ʹ���ź�data_en��ֱ��ʼλ��, 
															//		����ʱ��ʱ��Ϊÿ֡�Ĵ�ֱʱ�򣬸���ʱ��ʱ��Ϊ��һ����ʱ��
					|	( DEN0_v_fall     << 12 ) 	// ����ʹ���ź�data_en��ֱ����λ�ã�
															//		����ʱ��ʱ��Ϊÿ֡�Ĵ�ֱʱ�򣬸���ʱ��ʱ��Ϊ��һ����ʱ��
					;
	
	lcd_param5  =	( DEN1_v_rise     <<	0  )	// ����ʹ���ź�data_en��ֱ��ʼλ�ã�
															//		���������ʱ��VSW1_enable=1����Ч��������ǵڶ����Ĵ�ֱ��ʼλ�ã��������ʱ��Ч��
					|	( DEN1_v_fall     <<	12 ) 	// ����ʹ���ź�data_en��ֱ����λ�ã�
															//		���������ʱ��VSW1_enable=1����Ч��������ǵڶ����Ĵ�ֱ����λ�ã��������ʱ��Ч��
					;
	
	lcd_param6  = 	( CPL    <<	0  )               // clock cycles per line  max 4096, ÿ�е��ܵ��� 
					|	( LPS    <<	12 )               // Lines per screen value  ÿ֡��������������ʱ��ʱΪ��ż������������
					;	
	
	lcd_param7  =	( HSW_rise       <<	0  )		// ��ͬ���ź�hsyn������λ��
					|	( HSW_fall       <<	12 ) 		// ��ͬ���ź�hsyn�½���λ��
					;
	
	lcd_param8  =	( VSW0_v_rise    <<	0  )     // ��ͬ���ź�vsyn0������λ�ã�����ʱ��ʱΪ��ͬ��������ʱ��ʱΪ��һ���ĳ�ͬ����
					|	( VSW0_v_fall    <<	12 )     // ��ͬ���ź�vsyn0�½���λ�ã�����ʱ��ʱΪ��ͬ��������ʱ��ʱΪ��һ���ĳ�ͬ����
					;
	
	lcd_param9  =	( VSW0_h_rise   	<<	0  )		// ��ͬ���ź�vsyn0������λ�ã�
																//		����ʱ��ʱΪ��ͬ��������ʱ��ʱΪ��һ���ĳ�ͬ����
																//		ͨ��VSW0_h_rise  VSW0_h_fall���Ե��ڳ�ͬ���ľ���λ�ã�����ż��Ҳ���Գ����С�
					|	( VSW0_h_fall   	<< 12 ) 		// ��ͬ���ź�vsyn0�½���λ�ã�
																//		����ʱ��ʱΪ��ͬ��������ʱ��ʱΪ��һ���ĳ�ͬ����
																//		ͨ��VSW0_h_rise  VSW0_h_fall���Ե��ڳ�ͬ���ľ���λ�ã�����ż��Ҳ���Գ����С�
					;   
	
	lcd_param10 =	( VSW1_v_rise   	<<	0  )		// ��ͬ���ź�vsyn1������λ�ã�
																//		����ʱ��ʱ��Ч(VSW1_enable=0)��
																//		����ʱ��ʱΪ�ڶ����ĳ�ͬ����VSW1_enable=1����
					| 	( VSW1_v_fall   	<<	12 ) 		// ��ͬ���ź�vsyn1�½���λ�ã�
																//		����ʱ��ʱ��Ч(VSW1_enable=0)��
																//		����ʱ��ʱΪ�ڶ����ĳ�ͬ����VSW1_enable=1����
					;   
	
	lcd_param11 =	( VSW1_h_rise    	<<	0  )		// ��ͬ���ź�vsyn1������λ�ã�
																//		����ʱ��VSW1_enable=0��ʱ��Ч������ʱ��(VSW1_enable=1)ʱΪ��һ���ĳ�ͬ����
																//		ͨ��VSW1_h_rise  VSW1_h_fall���Ե��ڳ�ͬ���ľ���λ�ã�����ż��Ҳ���Գ����С�
					|	( VSW1_h_fall    	<<	12 ) 		// ��ͬ���ź�vsyn1�½���λ�ã�
																//		����ʱ��VSW1_enable=0��ʱ��Ч������ʱ��(VSW1_enable=1)ʱΪ��һ���ĳ�ͬ����
																//		ͨ��VSW1_h_rise  VSW1_h_fall���Ե��ڳ�ͬ���ľ���λ�ã�����ż��Ҳ���Գ����С�
					;   
	
	// RGB to YCbCr coef0������Yʱ R G B��ϵ����
	//	Y =�� R * RGB2YUV_coeff00 + G * RGB2YUV_coeff01 + B * RGB2YUV_coeff02��/256 + Rgb2yuv_mode_sel * 16;
	lcd_param12 = ( 66 	<< 0  )			// RGB��YUVת��ʱ��R��ϵ����
					| ( 129 	<< 9  ) 			// RGB��YUVת��ʱ��G��ϵ����
					| ( 25  	<< 18 )			// RGB��YUVת��ʱ��B��ϵ����
					;
	
	// RGB to YCbCr coef1������Cbʱ R G B��ϵ����
	//	Cb =�� - R * RGB2YUV_coeff10 - G * RGB2YUV_coeff11 + B * RGB2YUV_coeff12��/256 + 128
	lcd_param13 = (  38 << 0  )			// RGB��YUVת��ʱ��R��ϵ����
					| (  74 << 9  )			// RGB��YUVת��ʱ��G��ϵ����
					| ( 112 << 18 )			// RGB��YUVת��ʱ��B��ϵ����
					;
	
	lcd_param14  = 112 | 94<<9 | 18<<18; 
	
	lcd_param15  = 298 | 0<<9 | 409<<18;
	
	lcd_param16  = 299 |  100<<9 | 208<<18;
	
	lcd_param17  = 298 | 517<<9 | 0<<19; 
	
	lcd_param18 = (sav_pos    << 0  )   		// ITU656��Ч������ʼλ��
					;
	
	lcd_param19 = ( f1_vblk_start  <<  0 )		// ��һ����������ʼλ��
					| ( f1_vblk_end    << 12 ) 	// ��һ������������λ��
					;
	
	lcd_param20 = ( f2_vblk_start  <<  0 ) 	// �ڶ�����������ʼλ��
					| ( f2_vblk_end    << 12 )  	// �ڶ�������������λ��
					;
	
	lcd_param21 = ( f2_start   << 0  )			// ��ż����־��field=1����ʼλ��
					| ( f2_end   	<< 12 )			// ��ż����־��field=1������λ��
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
    #if 1 //RGB888 ����λ
	LCD_R_BIT_ORDER =(0x2<<0)|(0x3 << 3)|(0x04<< 6)|(0x05<<9)|(0x06<<12)|(0x07 <<15);
    LCD_G_BIT_ORDER =(0x2<<0)|(0x3 << 3)|(0x04<< 6)|(0x05<<9)|(0x06<<12)|(0x07 <<15);
    LCD_B_BIT_ORDER =(0x2<<0)|(0x3 << 3)|(0x04<< 6)|(0x05<<9)|(0x06<<12)|(0x07 <<15);
    #else
    LCD_R_BIT_ORDER =(0x3<<3)|(0x4 << 6)|(0x05<< 9)|(0x06<<12)|(0x07<<15);
    LCD_G_BIT_ORDER =(0x2<<0)|(0x3 << 3)|(0x04<< 6)|(0x05<<9)|(0x06<<12)|(0x07 <<15);
    LCD_B_BIT_ORDER =(0x3<<3)|(0x4 << 6)|(0x05<< 9)|(0x06<<12)|(0x07<<15);
    #endif
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

#if 0
	//           sda     csb     scl
	// gpio       59      58      60 
#define	CS_H			rGPIO_PB_RDATA |=  ((0x1) << (58-32))
#define	CS_L			rGPIO_PB_RDATA &= ~((0x1) << (58-32))
#define	SDA_H			rGPIO_PB_RDATA |=  ((0x1) << (60-32))
#define	SDA_L			rGPIO_PB_RDATA &= ~((0x1) << (60-32))
#define	SCL_H			rGPIO_PB_RDATA |=  ((0x1) << (59-32))
#define	SCL_L			rGPIO_PB_RDATA &= ~((0x1) << (59-32))


void hw_setCs(int cs)
{
	// ASIC ������
	//        reset    sda     csb     scl
	// gpio    70      73      72      71 
	if(cs)
		CS_H;
	else
		CS_L;
}


	//        reset    sda     csb     scl
	// gpio    22      23      24      25 
	// bit     22      23      24      25
	//val&=~((1<<22)|(1<<23)|(1<<24)|(1<<25));

	// ASIC ������
	//        reset    sda     csb     scl
	// gpio    70      73      72      71 
void SetDout(unsigned char Data)
{
	#define SPI_DELAY    0X05

	unsigned char i;
	unsigned char tmp;
	unsigned int val;
	tmp = Data;
	Delay(20);

	
	// ASIC ������
	//        reset    sda     csb     scl
	// gpio    70      73      72      71 
	
	for (i = 0; i < 8; i++)
	{
		//hw_setClk(0);  
		SCL_L;
		if (tmp & 0x80)	
		{
			SDA_H;
	   }
		else
		{
			SDA_L;
			
		}
		Delay(SPI_DELAY);
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

#endif

#define	LCD_SDA_GPIO		GPIO60
#define	LCD_SCL_GPIO		GPIO59
#define	LCD_CS_GPIO		GPIO58

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
static void SPI_WriteCom(unsigned int value)
{

    unsigned int  i;
	unsigned int  dat;
    Set_CS(0);

    #if 0
	dat = 0x01000000;
	for(i=0;i<8;i++)
	{
		Delay_us(2);
		Set_SCL(0);
		Delay_us(2);
		if(dat & 0x80)
			Set_SDI(1);
		else
			Set_SDI(0);

		Delay_us(10);
		Set_SCL(1);	
		Delay_us(2);
		dat <<= 1;
	}
    #endif
     Delay_us(2);
	Set_SCL(0);
	Delay_us(2);
    Set_SDI(0);
    Delay_us(2);
	Set_SCL(1);
	Delay_us(2);
	dat = value;
	for(i=0;i<8;i++)
	{
		Delay_us(2);
		Set_SCL(0);
		Delay_us(2);
		if(dat & 0x80)
			Set_SDI(1);
		else
			Set_SDI(0);

		Delay_us(2);
		Set_SCL(1);	
		Delay_us(2);
		dat <<= 1;
	}
}

static void SPI_WriteData(unsigned char value)
{
	unsigned int i;
	unsigned int dat;
	Set_CS(0);
    #if 0
	Delay_us(10);
	for(i=0;i<8;i++)
	{
		Delay_us(2);
		Set_SCL(0);
		Delay_us(2);
		if(dat & 0x80)
			Set_SDI(1);
		else
			Set_SDI(0);
		Delay_us(2);
		Set_SCL(1);
		Delay_us(2);
		dat <<= 1;
	}
    #endif
    Delay_us(2);
	Set_SCL(0);
	Delay_us(2);
    Set_SDI(1);
    Delay_us(2);
	Set_SCL(1);
	Delay_us(2);
	dat = value;
	for(i=0;i<8;i++)
	{
		Delay_us(2);
		Set_SCL(0);
		Delay_us(2);
		if(dat& 0x80)
			Set_SDI(1);
		else
			Set_SDI(0);
		Delay_us(2);
		Set_SCL(1);
		Delay_us(2);
		dat <<= 1;
	}
	Set_CS(1);
	Delay_us(2);
	
}

void Pannel_SPI_Write(void)
{  	
    SPI_WriteCom(0xc8);
    SPI_WriteData(0xFF);
    SPI_WriteData(0x93);
    SPI_WriteData(0x42);

    SPI_WriteCom(0x36);
    SPI_WriteData(0x08);//0xc8 for rotate
    //SPI_WriteCom(0x36);
    //SPI_WriteData(0xC8);//0xc8 for rotate

    SPI_WriteCom(0x3A);
    SPI_WriteData(0x66);//0x0

    SPI_WriteCom(0xc0);
    SPI_WriteData(0x0f);
    SPI_WriteData(0x0f);

    SPI_WriteCom(0xC1);
   // SPI_WriteData(0x01);
    SPI_WriteData(0x10);//liu

    SPI_WriteCom(0xC5);
    //SPI_WriteData(0xdb);
    SPI_WriteData(0xb2);//liu

    SPI_WriteCom(0xb4);
    SPI_WriteData(0x02);

    SPI_WriteCom(0xe0);
    SPI_WriteData(0x00);
    SPI_WriteData(0x01);
    //SPI_WriteData(0x05);//liu
   // SPI_WriteData(0x08);
    SPI_WriteData(0x02);
    SPI_WriteData(0x02);
    SPI_WriteData(0x1a);
    SPI_WriteData(0x0c);
    SPI_WriteData(0x44);//42
    SPI_WriteData(0x63);//0x7a);
    SPI_WriteData(0x57);//0x54);
    SPI_WriteData(0x02);//0x08);
    SPI_WriteData(0x0b);//0x0d);
    SPI_WriteData(0x0c);
    SPI_WriteData(0x27);//0x23);
    SPI_WriteData(0x26);//0x25);
    SPI_WriteData(0x0f);

    SPI_WriteCom(0xE1);
    SPI_WriteData(0x00);
    SPI_WriteData(0x2f);//0x29);
    SPI_WriteData(0x34);//0x2f);
    SPI_WriteData(0x01);//0x03);
    SPI_WriteData(0x0d);//0x0F);
    SPI_WriteData(0x02);//0x05);
    SPI_WriteData(0x47);//0x42);
    SPI_WriteData(0x22);//0x55);
    SPI_WriteData(0x59);//0x53);
    SPI_WriteData(0x04);//0x06);
    SPI_WriteData(0x0f);
    SPI_WriteData(0x0e);//0x0c);
    SPI_WriteData(0x3f);//0x38);
    SPI_WriteData(0x3f);//0x3a);
    SPI_WriteData(0x0f);

    SPI_WriteCom(0xb0);
    SPI_WriteData(0xe0);

    SPI_WriteCom(0xf6);
    SPI_WriteData(0x01);//0X01
    SPI_WriteData(0x00);
    SPI_WriteData(0x07);

    SPI_WriteCom(0x11); //Exit Sleep
    OS_Delay(12);

    SPI_WriteCom(0x29); //Display ON
    SPI_WriteCom(0x2C); //Display ON
    
}	

void  rSRGB_CFG_reg_Init()
{
#if SRGB_TYPE == SRGB_A035QN05
	rSRGB_CFG = (0<<11)|(0<<0);
#elif SRGB_TYPE == SRGB_JYD027
	// Line 1, 3, 5  RGB
	// Line 2, 4, 6  GBR
	rSRGB_CFG 	= (0 << 11)
					| (0 << 0)
					| (0 << 2)		// 010:BGR
					| (0 << 5)		// 101:GBR
					;		 
#endif
	//rCPU_SCR_CTRL_REG = 0x1;
}

void SRGB_Sel_SPI_Pad()
{
	unsigned int val;
	// DEMO ������
	//            sda     csb     scl
	// gpio        60      58      59
	val = rGPIO_PB_MOD;
	val = val & (~((0x01 << (58-32)) | (0x01 << (59-32)) | (0x01 << (60-32))) );
	rGPIO_PB_MOD = val;
	
	val = rSYS_PAD_CTRL05;
	val &= ~((0x07 << 18) | (0x07 << 21) | (0x07 << 24));
	rSYS_PAD_CTRL05 = val;
}

void  SRGB_Reset()
{
#if SRGB_TYPE == SRGB_JYD027
	
#elif SRGB_TYPE == SRGB_A035QN05
	// ASIC ������
	//        reset    sda     csb     scl
	// gpio    70      73      72      71 
	// Reset 
	rGPIO_PC_RDATA |=  (1 << (70-64)); // reset 
	Delayms(20);
	rGPIO_PC_RDATA &= ~(1 << (70-64)); // reset 
	Delayms(50);
	rGPIO_PC_RDATA |=  (1 << (70-64)); // reset 
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
	// new = 17142857 / (600 * 470) = 60.79		// ��ʡ0.4֡��Դ(USB 720P)

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
	//	IntTvClkSwitch_sel��IntTvClkSwitch
	// 	3'b000: clk_240m
	// 	3'b001: syspll_clk 
	// 	3'b010: audpll_clk
	// 	3'b100: clk_24m
	// 24000000 / 2 = 12000000
	// 12000000/(404*302)=
	
	rSYS_LCD_CLK_CFG = (4 << 0) | (1 << 3) | (1 << 8) |(1 << 9) | (3 << 15);
}


void srgb_SelPad(void)
{
   unsigned int val;

	XM_lock ();
	// D0 ~ D7
	val = rSYS_PAD_CTRL06;
	val &= 0xFF000000;
	val |= 1<<15|1<<12|1<<9|1<<6|1<<3|1<<0;
	rSYS_PAD_CTRL06 = val;

    
	// clk vsync hsync
	val = rSYS_PAD_CTRL07;
	val &= ~((0x7 << 15) | (0x7 << 18) | (0x7 << 21));
	val |= (1 << 21) | (1 << 18) | (1 << 15);
    val &= ~( (1 << 31) );
	rSYS_PAD_CTRL07 = val;	
	XM_unlock ();
}

void SRGB_A035QN05_Screen_Init()
{
	srgb_SelPad ();
	
	SRGB_clock_init ();
	
	rSRGB_CFG_reg_Init();
		
	SRGB_Sel_SPI_Pad();
	
	SRGB_Reset();
	
	Pannel_SPI_Write();
	
}



void HW_SRGB_Init (void)
{
	OS_EnterRegion ();	// ��ֹ�����л�
	Lcd_ClkDisable();
	sys_soft_reset (softreset_lcd);
	Lcd_ClkEnable ();	
	OS_LeaveRegion ();	
	
	SRGB_A035QN05_Screen_Init ();
	SRGB_A035QN05_LCD_Init ();
}

// SRGB���ر��˳�
void HW_SRGB_Exit(void)
{
	int i;
	// Lcd_enable
	//	LCDʹ���źţ�Ϊ1ʱLCDģ�鹤����Ϊ0ʱ��LCDģ�鲻������
	// OS_IncDI();    // Initially disable interrupts
	OS_EnterRegion ();	// ��ֹ�����л�
	
	// �ȴ�֡ˢ�½���
	LCD_INTR_CLR_reg = 0xFFFFFFFF;
	while(!(LCD_STATUS_reg & 0x01));		// bit1: lcd_done_intr(ÿ�������ж�) ,
	// ��ֹLCDC
	LCD_PARAM0_reg &= ~1;
	
	/*
	// ����ΪGPIO�������
	for (i = GPIO52; i <= GPIO79; i ++)
	{
		SetGPIOPadData (i, euDataLow);
		SetGPIOPadDirection (i, euOutputPad);
	}
	rSYS_PAD_CTRL05 = 0;
	rSYS_PAD_CTRL06 = 0;
	rSYS_PAD_CTRL07 = 0;
	*/
	
	Lcd_ClkDisable();
	
	//OS_DecRI();    // Initially disable interrupts
	OS_LeaveRegion ();	// ��ֹ�����л�
}

// �������� GPIO31
void HW_LCD_BackLightOn (void)
{
	// GPIO76
	// [14:12]	lcd_de	lcd_de_pad	itu_b_din11���� /���GPIO76
	unsigned int val;
	XM_lock ();
	val = rSYS_PAD_CTRL03;
	val &= ~(0x7 << 3);
	rSYS_PAD_CTRL03 = val;
	XM_unlock ();
		
    OS_EnterRegion ();	// ��ֹ�����л�
	SetGPIOPadDirection (LCD_BL_GPIO, euOutputPad);
	SetGPIOPadData (LCD_BL_GPIO, euDataHigh);
	OS_LeaveRegion ();	// ��ֹ�����л�
}

// �رձ���
void HW_LCD_BackLightOff (void)
{
	unsigned int val;
	
	XM_lock ();
	val = rSYS_PAD_CTRL03;
	val &= ~(0x7 << 3);
	rSYS_PAD_CTRL03 = val;
	XM_unlock ();
	
	OS_EnterRegion ();	// ��ֹ�����л�
	SetGPIOPadDirection (LCD_BL_GPIO, euOutputPad);
	SetGPIOPadData (LCD_BL_GPIO, euDataLow);
	OS_LeaveRegion ();	// ��ֹ�����л�
	
}
