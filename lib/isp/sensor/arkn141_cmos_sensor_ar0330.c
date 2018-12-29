#include "arkn141_isp_sensor_cfg.h"

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_AR0330
#include "arkn141_isp_exposure_cmos.h"
#include <string.h>
#include <stdio.h>
#include "gem_isp_io.h"

#ifdef WIN32
#include "xm_sensor_simulate.h"
#endif

#define	SENSOR_GAIN_L			1
#define	SENSOR_GAIN_M			2
#define	SENSOR_GAIN_H			3

#define	SENSOR_GAIN				SENSOR_GAIN_L

#define	EXPOSURE_LINES_ADDR		(0x3012)	// coarse_integration_time	
#define	AGAIN_ADDR					(0x3060)	// analog_gain
#define	DGAIN_ADDR					(0x305E)	// global_gain

extern void ap0330_2304x1296_30fps_12bit_Parallel_98MPixel (void);


// The sensor frame-time will increase if the coarse_integration_time is set to a
// value equal to or greater than the frame_length_lines
// 使用 1080p + EIS的16:9标准配置 Active Readout Window 2304 x 1296, Sensor Output Resolution 2304 x 1296
// frame_length_lines = 1308, line_length_pck = 1248
#define	STD_30_LINES	1308			

static cmos_inttime_t cmos_inttime;
static cmos_gain_t cmos_gain;

static u16_t sensor_frame_rate = 1;

// PCLK = 74.25MHz
// Frame Size = 2200 * 1125
// fps = 74.25 * 1000000 / (2200 * 1125) = 30 帧/秒
static  cmos_inttime_ptr_t cmos_inttime_initialize(void)
{
	cmos_inttime.full_lines = STD_30_LINES;
	cmos_inttime.full_lines_limit = 65535;
	cmos_inttime.max_lines_target = STD_30_LINES - 2;
	cmos_inttime.min_lines_target = 2;

	cmos_inttime.exposure_ashort = 0;

	return &cmos_inttime;
}


typedef struct tag_AR0330_AGAIN {
	u16_t		coarse_fine;		// R0x3060[5:4] COARSE_GAIN	
										// R0x3060[3:0] FINE_GAIN
	u16_t		total_gain;
} AR0330_AGAIN;

static const AR0330_AGAIN analog_gain_table[] =
{
	{ 0x00,	(u16_t)(1.00 * 1024) },
	{ 0x01,	(u16_t)(1.03 * 1024) },
	{ 0x02,	(u16_t)(1.07 * 1024) },
	{ 0x03,	(u16_t)(1.10 * 1024) },
	{ 0x04,	(u16_t)(1.14 * 1024) },
	{ 0x05,	(u16_t)(1.19 * 1024) },
	{ 0x06,	(u16_t)(1.23 * 1024) },
	{ 0x07,	(u16_t)(1.28 * 1024) },
	{ 0x08,	(u16_t)(1.33 * 1024) },
	{ 0x09,	(u16_t)(1.39 * 1024) },
	{ 0x0A,	(u16_t)(1.45 * 1024) },
	{ 0x0B,	(u16_t)(1.52 * 1024) },
	{ 0x0C,	(u16_t)(1.60 * 1024) },
	{ 0x0D,	(u16_t)(1.68 * 1024) },
	{ 0x0E,	(u16_t)(1.78 * 1024) },
	{ 0x0F,	(u16_t)(1.88 * 1024) },
	{ 0x10,	(u16_t)(2.00 * 1024) },
#if SENSOR_GAIN == SENSOR_GAIN_L

#elif SENSOR_GAIN == SENSOR_GAIN_H
	{ 0x12,	(u16_t)(2.13 * 1024) },
	{ 0x14,	(u16_t)(2.29 * 1024) },
	{ 0x16,	(u16_t)(2.46 * 1024) },
	{ 0x18,	(u16_t)(2.67 * 1024) },
	{ 0x1A,	(u16_t)(2.91 * 1024) },
	{ 0x1C,	(u16_t)(3.20 * 1024) },
	{ 0x1E,	(u16_t)(3.56 * 1024) },
	{ 0x20,	(u16_t)(4.00 * 1024) },
	{ 0x24,	(u16_t)(4.57 * 1024) },
	{ 0x28,	(u16_t)(5.33 * 1024) },
	{ 0x2C,	(u16_t)(6.40 * 1024) },
	{ 0x30,	(u16_t)(8.00 * 1024) }
#endif
};

static cmos_gain_ptr_t cmos_gain_initialize(void)
{
#if SENSOR_GAIN == SENSOR_GAIN_L
	cmos_gain.again_shift = 0;
	cmos_gain.dgain_shift = 7;
	cmos_gain.max_again_target = 1;
	cmos_gain.max_dgain_target = 0x80;
#else
	
	cmos_gain.again_shift = 10;
#if SENSOR_GAIN == SENSOR_GAIN_L
	cmos_gain.max_again_target = (u16_t)(2.00 * 1024);
#elif SENSOR_GAIN == SENSOR_GAIN_H
	cmos_gain.max_again_target = (u16_t)(8.00 * 1024);
#endif
	cmos_gain.dgain_shift = 7;
	// Each digital gain can be configured from a gain of 0 to 15.875.
#if SENSOR_GAIN == SENSOR_GAIN_L
	cmos_gain.max_dgain_target = 0x80;		// 1 * 128 = 0x80
#elif SENSOR_GAIN == SENSOR_GAIN_M
	cmos_gain.max_dgain_target = 0x100;		// 2 * 128 = 0x100
#elif SENSOR_GAIN == SENSOR_GAIN_H
	cmos_gain.max_dgain_target = 0x7F0;		// 15.875 * 128 = 0x7F0
#endif
	
#endif
	
	return &cmos_gain;
}



// 根据曝光量计算模拟增益
static u32_t analog_gain_from_exposure_calculate (cmos_gain_ptr_t gain, u32_t exposure, u32_t exposure_max)
{
	// 二分法查找最接近的模拟增益
	int l, h, m, match;
	i64_t exp;
	i64_t mid;
	match = 0;
	if(exposure <= exposure_max)
	{
		gain->again = analog_gain_table[0].total_gain;
		gain->aindex = 0;
		return exposure;
	}
	l = 0;
	h = sizeof(analog_gain_table)/sizeof(analog_gain_table[0]) - 1;
	exp = exposure << gain->again_shift;
	while(l <= h)
	{
		m = (l + h) >> 1;
		mid = analog_gain_table[m].total_gain;
		mid = mid * exposure_max;
		// 寻找满足 mid <= exp的最大m
		if(mid < exp)
		{
			if(m > match)
				match = m;
			// 需增大模拟增益
			l = m + 1;
		}
		else if(mid > exp)
		{
			// 需减小模拟增益
			h = m - 1;
		}
		else
		{
			// mid == exp
			match = m;
			break;
		}
	}
	m = match;
	gain->again = analog_gain_table[m].total_gain;
	gain->aindex = (u16_t)analog_gain_table[m].coarse_fine;
	return (u32_t)(exp / analog_gain_table[m].total_gain);
}

// 根据曝光量计算数字增益
// 数字增益
// Each digital gain can be configured from a gain of 0 to 15.875
static u32_t digital_gain_from_exposure_calculate (cmos_gain_ptr_t gain, u32_t exposure, u32_t exposure_max)
{
	u32_t dgain;

	if(exposure <= exposure_max)
	{
		dgain = 1 << gain->dgain_shift;		// gain = 1
	}
	else
	{
		dgain = (exposure << gain->dgain_shift) / exposure_max;
		if(dgain > gain->max_dgain)
			dgain = gain->max_dgain;
	}
	gain->dgain = dgain;
	gain->dindex = (u16_t)dgain;
	return (exposure << gain->dgain_shift) / dgain; 
}

static void cmos_inttime_gain_update (cmos_inttime_ptr_t p_inttime, cmos_gain_ptr_t gain) 
{
	u16_t exp_time;

	if(p_inttime)
	{
		exp_time = (u16_t)p_inttime->exposure_ashort;
		arkn141_isp_cmos_sensor_write_register (EXPOSURE_LINES_ADDR, exp_time);
	}
	if(gain)
	{
		arkn141_isp_cmos_sensor_write_register (AGAIN_ADDR, (u16_t)gain->aindex);
		arkn141_isp_cmos_sensor_write_register (DGAIN_ADDR, (u16_t)gain->dindex);
	}
}

static u32_t cmos_get_iso (cmos_gain_ptr_t gain)
{
	i64_t iso = gain->again;
	iso = iso * gain->dgain;
	iso = (iso * 100) >> (gain->again_shift + gain->dgain_shift);
	
	gain->iso =  (u32_t)iso; 
	
	return gain->iso;
}

static void cmos_fps_set (cmos_inttime_ptr_t p_inttime, u8_t fps)
{
	switch (fps)
	{
		case 30:
		default:
			p_inttime->full_lines = STD_30_LINES;
			p_inttime->lines_per_500ms = STD_30_LINES * 30 / 2;
			break;
	}
}

	// 设置sensor readout drection
	// horz_reverse_direction --> 1  horz reverse direction 垂直反向
	//                        --> 0  horz normal direction
	// vert_reverse_direction --> 1  vert reverse direction 水平反向
	//                        --> 0  vert normal direction
static int cmos_sensor_set_readout_direction (u8_t horz_reverse_direction, u8_t vert_reverse_direction)
{
	int ret;
	unsigned int reg0x3040 = 0;
	arkn141_isp_cmos_sensor_read_register (0x3040, &reg0x3040);
	if(horz_reverse_direction)
		reg0x3040 |= (1 << 14);
	else
		reg0x3040 &= ~(1 << 14);
	if(vert_reverse_direction)
		reg0x3040 |= (1 << 15);
	else
		reg0x3040 &= ~(1 << 15);
	return arkn141_isp_cmos_sensor_write_register (0x3040, reg0x3040);			// 关闭/开启水平翻转/垂直翻转
}

static const char *ar0330_cmos_sensor_get_sensor_name (void)
{
	return "AR0330";	
}

int ar0330_isp_sensor_init(isp_sen_ptr_t p_sen)
{
	//ap0330_2304x1296_30fps_12bit_Parallel_98MPixel ();
	
	return 0;
}

static void ar0330_cmos_isp_awb_init (isp_awb_ptr_t p_awb)		// 白平衡初始参数
{
  p_awb->enable = 1;    
  p_awb->mode = 1; //0 调试 1 第一种算法 2 第二种算法    
  p_awb->manual = 0;//0=自动白平衡  1=手动白平衡
  p_awb->weight[0][0] = 1;
  p_awb->weight[0][1] = 2;
  p_awb->weight[0][2] = 1;
  p_awb->weight[1][0] = 2;
  p_awb->weight[1][1] = 4;
  p_awb->weight[1][2] = 2;
  p_awb->weight[2][0] = 1;
  p_awb->weight[2][1] = 2;
  p_awb->weight[2][2] = 1;
  p_awb->black = 4;     
  p_awb->white = 210; 
  p_awb->jitter = 13;
  p_awb->r2g_min = 256/4;
  p_awb->r2g_max = 256*4;
  p_awb->b2g_min = 256/4;
  p_awb->b2g_max = 256*4;
  p_awb->r2g_light[0] = 138;//154; N 
  p_awb->b2g_light[0] = 177;//195;//192; N
  p_awb->r2g_light[1] = 179;//189; DAY 
  p_awb->b2g_light[1] = 192;//205; DAY
  p_awb->r2g_light[2] = 195;//197; C 
  p_awb->b2g_light[2] = 169;//136; C
  p_awb->r2g_light[3] = 261;//305; U 
  p_awb->b2g_light[3] = 87;//131; U
  p_awb->r2g_light[4] = 294;// A   
  p_awb->b2g_light[4] = 87;// A
  p_awb->r2g_light[5] = 0;   
  p_awb->b2g_light[5] = 0;
  p_awb->r2g_light[6] = 0;   
  p_awb->b2g_light[6] = 0;
  p_awb->r2g_light[7] = 0;   
  p_awb->b2g_light[7] = 0;
  p_awb->use_light[0] = 1;
  p_awb->use_light[1] = 1;
  p_awb->use_light[2] = 1;
  p_awb->use_light[3] = 1;
  p_awb->use_light[4] = 1;
  p_awb->use_light[5] = 0;
  p_awb->use_light[6] = 0;
  p_awb->use_light[7] = 0;
  p_awb->gain_g2r = 426;//434;
  p_awb->gain_g2b = 280;//348;

  isp_awb_init_io (p_awb);	
}

unsigned short gamma_LUT0[] = {    0,         280,         608,         984,        1408,        1880,        2400,       2968, 
                   3584,        4248,        4960,        5720,        6528,        7384,        8288,       9240, 
                  10240,       11288,       12384,       13528,       14720,       15960,       17248,      18584, 
                  19968,       21400,       22880,       24408,       25984,       27608,       29280,      31000, 
                  32768,       34536,       36256,       37928,       39552,       41128,       42656,      44136, 
                  45568,       46952,       48288,       49576,       50816,       52008,       53152,      54248, 
                  55296,       56296,       57248,       58152,       59008,       59816,       60576,      61288, 
                  61952,       62568,       63136,       63656,       64128,       64552,       64928,      65256, 
                  65535};

static void ar0330_cmos_isp_colors_init (isp_colors_ptr_t p_colors)	// 色彩初始参数
{
  int i, j;
  int gamma_wdr[65];
  int k_L, k_H, pmax, t0, s0, t1, s1, t2, s2, k0, k2;
  int b, c, d, bb, cc, dd;
//  int low_lig_mode = 1;// 1=关闭gamma 0=打开gamma
  //int sm00, sm01, sm02, sm10, sm11, sm12, sm20, sm21, sm22;
  int contrast_Ls, contrast_Hs;
  int ccm_S[3][3],ccm_N[3][3], ccm_set[3][3];
  int satu, s_comp, r_comp, g_comp, b_comp;
  
  ccm_N[0][0] = 279;
  ccm_N[0][1] = -33;
  ccm_N[0][2] = 16;
  ccm_N[1][0] = 0;
  ccm_N[1][1] = 261;
  ccm_N[1][2] = 5;
  ccm_N[2][0] = 34;
  ccm_N[2][1] = -88;
  ccm_N[2][2] = 329;
     
  satu = 256+128;
  s_comp = 256 - satu;
  r_comp = (77 * s_comp)/256;
  g_comp = (150 * s_comp)/256;
  b_comp = (29 * s_comp)/256;
  ccm_S[0][0] = r_comp + satu;
  ccm_S[0][1] = g_comp;
  ccm_S[0][2] = b_comp;
  ccm_S[1][0] = r_comp;
  ccm_S[1][1] = g_comp + satu;
  ccm_S[1][2] = b_comp;
  ccm_S[2][0] = r_comp;
  ccm_S[2][1] = g_comp;
  ccm_S[2][2] = b_comp + satu;
  
/*  
  sm[0][0] = (299*(256-satu)+(satu*1000))/1000;
  sm[0][1] = (587*(256-satu))/1000;
  sm[0][2] = (114*(256-satu))/1000; 
  sm[1][0] = (299*(256-satu))/1000;
  sm[1][1] = (587*(256-satu)+(satu*1000))/1000;
  sm[1][2] = (114*(256-satu))/1000;
  sm[2][0] = (299*(256-satu))/1000;
  sm[2][1] = (587*(256-satu))/1000;
  sm[2][2] = (114*(256-satu)+(satu*1000))/1000;
*/
  
  
  ccm_set[0][0] = ccm_N[0][0];
  ccm_set[0][1] = ccm_N[0][1];
  ccm_set[0][2] = ccm_N[0][2];
  ccm_set[1][0] = ccm_N[1][0];
  ccm_set[1][1] = ccm_N[1][1];
  ccm_set[1][2] = ccm_N[1][2];
  ccm_set[2][0] = ccm_N[2][0];
  ccm_set[2][1] = ccm_N[2][1];
  ccm_set[2][2] = ccm_N[2][2];
  
  p_colors->colorm.enable = 0;// 色矩阵 
  p_colors->colorm.matrixcoeff[0][0] = ccm_set[0][0];//256;
  p_colors->colorm.matrixcoeff[0][1] = ccm_set[0][1];//0;
  p_colors->colorm.matrixcoeff[0][2] = ccm_set[0][2];//0;  
  p_colors->colorm.matrixcoeff[1][0] = ccm_set[1][0];//0;
  p_colors->colorm.matrixcoeff[1][1] = ccm_set[1][1];//256;
  p_colors->colorm.matrixcoeff[1][2] = ccm_set[1][2];//0;
  p_colors->colorm.matrixcoeff[2][0] = ccm_set[2][0];//0;
  p_colors->colorm.matrixcoeff[2][1] = ccm_set[2][1];//0;
  p_colors->colorm.matrixcoeff[2][2] = ccm_set[2][2];//256;
    
  p_colors->gamma.enable =  0;
  
  for (i = 0; i < 65; i++)
  {
		p_colors->gamma.gamma_lut[i] = gamma_LUT0[i];
  } 

  p_colors->rgb2ypbpr_type = HDTV_type_16235;
  // SDTV:16-235 
  // Y601 = 0.299R′ + 0.587G′ + 0.114B′
  // Cb = –0.172R′ – 0.339G′ + 0.511B′ + 128
  // Cr = 0.511R′ – 0.428G′ – 0.083B′ + 128
  if( p_colors->rgb2ypbpr_type == SDTV_type_16235)
  {
  p_colors->rgb2yuv.rgbcoeff[0][0] =  306;
  p_colors->rgb2yuv.rgbcoeff[0][1] =  601;
  p_colors->rgb2yuv.rgbcoeff[0][2] =  117;
  p_colors->rgb2yuv.rgbcoeff[1][0] =  176;
  p_colors->rgb2yuv.rgbcoeff[1][1] =  347;
  p_colors->rgb2yuv.rgbcoeff[1][2] =  523;
  p_colors->rgb2yuv.rgbcoeff[2][0] =  523;
  p_colors->rgb2yuv.rgbcoeff[2][1] =  438;
  p_colors->rgb2yuv.rgbcoeff[2][2] =  85;
  p_colors->rgb2yuv.rgbcoeff[3][0] = 0;
  p_colors->rgb2yuv.rgbcoeff[3][1] =  128;
  p_colors->rgb2yuv.rgbcoeff[3][2] =  128;
  }
  // SDTV:0-255
  // Y601 = 0.257R′ + 0.504G′ + 0.098B′ + 16
  // Cb = –0.148R′ – 0.291G′ + 0.439B′ + 128
  // Cr = 0.439R′ – 0.368G′ – 0.071B′ + 128
  if( p_colors->rgb2ypbpr_type == SDTV_type_0255)
  {
  p_colors->rgb2yuv.rgbcoeff[0][0] =  263;
  p_colors->rgb2yuv.rgbcoeff[0][1] =  516;
  p_colors->rgb2yuv.rgbcoeff[0][2] =  100;
  p_colors->rgb2yuv.rgbcoeff[1][0] =  152;
  p_colors->rgb2yuv.rgbcoeff[1][1] =  298;
  p_colors->rgb2yuv.rgbcoeff[1][2] =  450;
  p_colors->rgb2yuv.rgbcoeff[2][0] =  450;
  p_colors->rgb2yuv.rgbcoeff[2][1] =  377;
  p_colors->rgb2yuv.rgbcoeff[2][2] =  73;
  p_colors->rgb2yuv.rgbcoeff[3][0] = 16;
  p_colors->rgb2yuv.rgbcoeff[3][1] =  128;
  p_colors->rgb2yuv.rgbcoeff[3][2] =  128;
  }
  
  // HDTV:16-235
  // Y709 = 0.213R′ + 0.715G′ + 0.072B′
  // Cb = –0.117R′ – 0.394G′ + 0.511B′ + 128
  // Cr = 0.511R′ – 0.464G′ – 0.047B′ + 128
  // Y = (218*R + 732*G + 74*B)/1024
  // Cb = (-120*R - 403*G + 524*B)/1024 + 128
  // Cr = (523*R - 475*G - 48*B)/1024 + 128
  if( p_colors->rgb2ypbpr_type == HDTV_type_16235)
  {
  p_colors->rgb2yuv.rgbcoeff[0][0] = 218;
  p_colors->rgb2yuv.rgbcoeff[0][1] = 732;
  p_colors->rgb2yuv.rgbcoeff[0][2] = 74;
  p_colors->rgb2yuv.rgbcoeff[1][0] = 120;
  p_colors->rgb2yuv.rgbcoeff[1][1] = 403;
  p_colors->rgb2yuv.rgbcoeff[1][2] = 524;
  p_colors->rgb2yuv.rgbcoeff[2][0] = 523;
  p_colors->rgb2yuv.rgbcoeff[2][1] = 475;
  p_colors->rgb2yuv.rgbcoeff[2][2] = 48;
  p_colors->rgb2yuv.rgbcoeff[3][0] = 0;
  p_colors->rgb2yuv.rgbcoeff[3][1] = 128;
  p_colors->rgb2yuv.rgbcoeff[3][2] = 128;
  }
  
  // HDTV:0-255
  // Y709 = 0.183R′ + 0.614G′ + 0.062B′ + 16
  // Cb = –0.101R′ – 0.338G′ + 0.439B′ + 128
  // Cr = 0.439R′ – 0.399G′ – 0.040B′ + 128
  if( p_colors->rgb2ypbpr_type == HDTV_type_0255 )
  {
  // HDTV with offset
  p_colors->rgb2yuv.rgbcoeff[0][0] = 187;
  p_colors->rgb2yuv.rgbcoeff[0][1] = 629;
  p_colors->rgb2yuv.rgbcoeff[0][2] = 63;
  p_colors->rgb2yuv.rgbcoeff[1][0] = 103;
  p_colors->rgb2yuv.rgbcoeff[1][1] = 346;
  p_colors->rgb2yuv.rgbcoeff[1][2] = 450;
  p_colors->rgb2yuv.rgbcoeff[2][0] = 450;
  p_colors->rgb2yuv.rgbcoeff[2][1] = 409;
  p_colors->rgb2yuv.rgbcoeff[2][2] = 41;
  p_colors->rgb2yuv.rgbcoeff[3][0] = 16;
  p_colors->rgb2yuv.rgbcoeff[3][1] = 128;
  p_colors->rgb2yuv.rgbcoeff[3][2] = 128;
  }
  
	// demosaic 参数
	p_colors->demosaic.mode = 0;
	p_colors->demosaic.coff_00_07 = 32;
	p_colors->demosaic.coff_20_27 = 255;	// 滤波
	p_colors->demosaic.horz_thread = 0;
	p_colors->demosaic.demk = 128;
	p_colors->demosaic.demDhv_ofst = 0;
  
  isp_colors_init_io(p_colors);
}

static void ar0330_cmos_isp_denoise_init (isp_denoise_ptr_t p_denoise)	// 降噪初始设置
{
  int i, x0, y0, x1, y1, x2, y2, x3, y3;
  int a, b, c, d, e, f, delta;

  p_denoise->enable2d = 0;	//7;
  p_denoise->enable3d = 0;	//7;  

  p_denoise->sensitiv0 = 3;    
  p_denoise->sensitiv1 = 0;    
  p_denoise->sel_3d_table = 1;

   
  p_denoise->y_thres2 = 4; 
  p_denoise->u_thres2 = 16;  
  p_denoise->v_thres2 = 16; 
   
  p_denoise->y_thres0 = 14;    
  p_denoise->u_thres0 = 16;
  p_denoise->v_thres0 = 16;
  
  p_denoise->y_thres1 = 7;   
  p_denoise->u_thres1 = 8;
  p_denoise->v_thres1 = 8; 
    
  p_denoise->noise0[0]  = 255;//8;
  p_denoise->noise0[1]  = 255;//16;
  p_denoise->noise0[2]  = 255;//16;
  p_denoise->noise0[3]  = 255;//16;
  p_denoise->noise0[4]  = 255-0;//16;
  p_denoise->noise0[5]  = 255-4;//16;
  p_denoise->noise0[6]  = 255-8;//16;
  p_denoise->noise0[7]  = 255-16;//16;
  p_denoise->noise0[8]  = 255-32;//16;
  p_denoise->noise0[9]  = 255-32;//16;
  p_denoise->noise0[10] = 255-32;//16;
  p_denoise->noise0[11] = 255-32;//16;
  p_denoise->noise0[12] = 255-32;//16;
  p_denoise->noise0[13] = 255-32;//16;
  p_denoise->noise0[14] = 255-32;//16;
  p_denoise->noise0[15] = 255-32;//16;
  p_denoise->noise0[16] = 255-32;//16;

  p_denoise->noise1[0]  = 255;//8;
  p_denoise->noise1[1]  = 255;//16;
  p_denoise->noise1[2]  = 255;//16;
  p_denoise->noise1[3]  = 255;//16;
  p_denoise->noise1[4]  = 255-0;//16;
  p_denoise->noise1[5]  = 255-4;//16;
  p_denoise->noise1[6]  = 255-8;//16;
  p_denoise->noise1[7]  = 255-16;//16;
  p_denoise->noise1[8]  = 255-32;//16;
  p_denoise->noise1[9]  = 255-32;//16;
  p_denoise->noise1[10] = 255-32;//16;
  p_denoise->noise1[11] = 255-32;//16;
  p_denoise->noise1[12] = 255-32;//16;
  p_denoise->noise1[13] = 255-32;//16;
  p_denoise->noise1[14] = 255-32;//16;
  p_denoise->noise1[15] = 255-32;//16;
  p_denoise->noise1[16] = 255-32;//16;

 
  isp_denoise_init_io (p_denoise);
}

static void ar0330_cmos_isp_eris_init(isp_eris_ptr_t p_eris)			// 宽动态初始设置
{
  int i, j, x0, y0, x1, y1, x2, y2;
  int a, b, c, d;

  p_eris->enable = 0;	//1;
  p_eris->manual = 0;
  p_eris->resols = 0;
  p_eris->resoli = 144;
  p_eris->spacev = 0;
  p_eris->target = 128;
  p_eris->black = 0;
  p_eris->white = 4095;
  p_eris->gain_max = 256;
  p_eris->gain_min = 256;
  p_eris->cont_max = 12;
  p_eris->cont_min = 4;
  p_eris->gain_man = 195;
  p_eris->cont_man = 16;


  
  {
    for (i = 0; i < 33; i++)
    {
    	p_eris->resolt[i] = 144;
    }
  	
  }  
  
  {
    for (i = 0; i < 33; i++)
    {
    	p_eris->colort[i] = 0;   	
    }
  }
 
	// ERIS直方图初始设置 
	// u8_t hist_thresh[4] = {0x10, 0x40, 0x80, 0xc0};
  	p_eris->eris_hist_thresh[0] = 0x10;
	p_eris->eris_hist_thresh[1] = 0x40;
	p_eris->eris_hist_thresh[2] = 0x80;
	p_eris->eris_hist_thresh[3] = 0xC0;
  
  
  isp_eris_init_io(p_eris);	
}


#if 0
#define CenterRx		1137
#define CenterRy		344
#define CenterGx		1136
#define CenterGy		384
#define CenterBx		1136
#define CenterBy		385

unsigned  int lenscoeff[65*3]={
//R Coeff:number:43
//center x:1137 y:344 maxlength:1353
  //  1642  广角1华科光电
4096,4112,4129,4146,4180,
4224,4268,4332,4408,4496,
4609,4727,4864,5020,5201,
5395,5636,5899,6206,6527,
6905,7331,7812,8430,8992,
9114,9367,9870,10538,11304,
12116,12970,13954,14988,16450,
17906,19088,20234,21993,23804,
25941,27343,28906,
//continue:
28906,28906,
28906,28906,28906,28906,28906,
28906,28906,28906,28906,28906,
28906,28906,28906,28906,28906,
28906,28906,28906,28906,28906,


//G Coeff:number:42
//center x:1136 y:384 maxlength:1332
4096,4108,4120,4144,4169,
4220,4258,4324,4393,4477,
4581,4720,4869,5027,5235,
5440,5707,5976,6301,6663,
7069,7487,8096,8704,9222,
9346,9671,10091,10712,11415,
12216,13015,13926,15303,16579,
18086,19614,21100,22830,24432,
26276,27852,
//continue:
27852,27852,27852,
27852,27852,27852,27852,27852,
27852,27852,27852,27852,27852,
27852,27852,27852,27852,27852,
27852,27852,27852,27852,27852,


//B Coeff:number:42
//center x:1136 y:385 maxlength:1332
4096,4096,4121,4147,4173,
4208,4253,4318,4385,4474,
4577,4707,4857,5029,5227,
5442,5692,5983,6306,6644,
7069,7496,8075,8712,9239,
9370,9643,10084,10680,11417,
12187,13069,13989,15281,16835,
18394,19668,21360,23099,25146,
26845,28379,
//continue:
28379,28379,28379,
28379,28379,28379,28379,28379,
28379,28379,28379,28379,28379,
28379,28379,28379,28379,28379,
28379,28379,28379,28379,28379
};
#endif

#if   1
//  1642  广角1华科光电
// from zhl
#define  Ycenter_x   982 
#define  Ycenter_y   513
#define  CenterRx   Ycenter_x
#define  CenterRy   Ycenter_y
#define  CenterGx   Ycenter_x
#define  CenterGy   Ycenter_y
#define  CenterBx   Ycenter_x
#define  CenterBy   Ycenter_y
#endif

#if 0
//  1756  大镜头 蓝色圈圈
// from zhl
#define  CenterRx   963
#define  CenterRy   391
#define  CenterGx   874
#define  CenterGy   526
#define  CenterBx   932
#define  CenterBy   536
unsigned short lenscoeff[]=
{
  // r
  4096,4315,4322,4317,4331,     4339,4355,4361,4406,4443,
  4467,4466,4485,4519,4552,     4591,4648,4702,4771,4847,
  4923,5001,5125,5242,5373,     5518,5694,5885,6047,6230,
  6416,6597,6821,7055,7305,     7481,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,     7799,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,     7799,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,
  // g
  4096,4233,4244,4250,4286,     4340,4346,4332,4327,4349,
  4375,4408,4447,4482,4530,     4582,4644,4714,4788,4873,
  4970,5070,5187,5310,5433,     5563,5723,5890,5965,6079,
  6178,6278,6384,6567,6831,     7108,7413,7413,7413,7413,
  7413,7413,7413,7413,7413,     7413,7413,7413,7413,7413,
  7413,7413,7413,7413,7413,     7413,7413,7413,7413,7413,
  7413,7413,7413,7413,7413,
  // b 
  4096,4265,4273,4290,4353,     4365,4352,4330,4353,4377,
  4406,4436,4480,4524,4571,     4625,4681,4747,4823,4903,
  4992,5086,5176,5292,5409,     5543,5678,5830,5995,6190,
  6311,6477,6670,6852,7000,     7442,7442,7442,7442,7442,
  7442,7442,7442,7442,7442,     7442,7442,7442,7442,7442,
  7442,7442,7442,7442,7442,     7442,7442,7442,7442,7442,
  7442,7442,7442,7442,7442
};
#endif


#if 0
//  1756  大镜头 cctv
// from zhl
#define  CenterRx   884
#define  CenterRy   464
#define  CenterGx   895
#define  CenterGy   609
#define  CenterBx   1020
#define  CenterBy   564
unsigned short lenscoeff[]=
{
  // r
  4096,4320,4335,4340,4351,     4368,4406,4442,4459,4467,
  4480,4510,4555,4594,4642,     4692,4750,4812,4883,4954,
  5109,5281,5463,5681,5900,     6172,6449,6789,7057,7387,
  7728,8106,8602,9000,9362,     9471,9826,10479,10479,10479,
  10479,10479,10479,10479,10479,10479,10479,10479,10479,10479,
  10479,10479,10479,10479,10479,10479,10479,10479,10479,10479,
  10479,10479,10479,10479,10479,
  // g
  4096,4279,4335,4343,4317,     4285,4290,4303,4331,4359,
  4388,4417,4451,4500,4540,     4593,4654,4728,4811,4911,
  5051,5207,5387,5597,5832,     6071,6351,6676,6992,7272,
  7610,7993,8373,8860,9408,     10063,10641,11291,11291,11291,
  11291,11291,11291,11291,11291,11291,11291,11291,11291,11291,
  11291,11291,11291,11291,11291,11291,11291,11291,11291,11291,
  11291,11291,11291,11291,11291,
  // b 
  4096,4259,4269,4312,4343,     4356,4352,4337,4359,4381,
  4412,4445,4478,4524,4579,     4632,4689,4794,4901,5027,
  5161,5310,5487,5678,5873,     6105,6401,6697,6962,7074,
  7280,7471,7679,7837,8334,     8783,9373,9373,9373,9373,
  9373,9373,9373,9373,9373,     9373,9373,9373,9373,9373,
  9373,9373,9373,9373,9373,     9373,9373,9373,9373,9373,
  9373,9373,9373,9373,9373
};
#endif

#if   0
//  2226 use大镜头 黑
#define CenterRx		964
#define CenterRy		494
#define CenterGx		983
#define CenterGy		478
#define CenterBx		997
#define CenterBy		477

unsigned  int lenscoeff[65*3]={
//R Coeff:number:36
//center x:964 y:494 maxlength:1128
4096,4118,4118,4118,4140,
4162,4208,4231,4279,4352,
4402,4505,4586,4699,4817,
4941,5106,5246,5432,5673,
5891,6177,6436,6778,7092,
7509,7896,8417,8906,9456,
9947,10638,11264,11968,12765,
13206,
//continue:
13206,13206,13206,13206,
13206,13206,13206,13206,13206,
13206,13206,13206,13206,13206,
13206,13206,13206,13206,13206,
13206,13206,13206,13206,13206,
13206,13206,13206,13206,13206,


//G Coeff:number:36
//center x:983 y:478 maxlength:1151
4096,4096,4108,4121,4148,
4174,4215,4256,4313,4370,
4445,4522,4618,4719,4842,
4971,5107,5273,5449,5663,
5920,6173,6448,6748,7078,
7443,7846,8296,8741,9237,
9793,10337,10945,11629,12288,
12896,
//continue:
12896,12896,12896,12896,
12896,12896,12896,12896,12896,
12896,12896,12896,12896,12896,
12896,12896,12896,12896,12896,
12896,12896,12896,12896,12896,
12896,12896,12896,12896,12896,


//B Coeff:number:37
//center x:997 y:477 maxlength:1164
4096,4112,4112,4128,4145,
4178,4212,4264,4317,4372,
4447,4545,4626,4753,4865,
4982,5130,5314,5483,5693,
5921,6168,6436,6729,7049,
7402,7791,8224,8635,9090,
9595,10159,10683,11264,11911,
12637,13117,
//continue:
13117,13117,13117,
13117,13117,13117,13117,13117,
13117,13117,13117,13117,13117,
13117,13117,13117,13117,13117,
13117,13117,13117,13117,13117,
13117,13117,13117,13117,13117
};
#endif 

#if 0
//2058 use广角
#define CenterRx		966
#define CenterRy		544
#define CenterGx		979
#define CenterGy		544
#define CenterBx		993
#define CenterBy		557

unsigned  int lenscoeff[65*3]={
//R Coeff:number:35
//center x:966 y:544 maxlength:1108
4096,4096,4129,4146,4180,
4214,4250,4304,4378,4455,
4555,4681,4837,5028,5236,
5490,5771,6193,6682,7204,
7874,8536,9319,10260,11286,
12540,13915,15391,17217,19166,
21162,23086,25395,28216,29876,

//continue:
29876,29876,29876,29876,29876,
29876,29876,29876,29876,29876,
29876,29876,29876,29876,29876,
29876,29876,29876,29876,29876,
29876,29876,29876,29876,29876,
29876,29876,29876,29876,29876,


//G Coeff:number:35
//center x:979 y:544 maxlength:1119
4096,4105,4125,4145,4176,
4217,4259,4313,4391,4472,
4592,4720,4882,5071,5291,
5532,5835,6218,6656,7189,
7745,8393,9160,9964,10922,
12084,13312,14689,16384,18126,
19585,21568,23999,26214,27482,

//continue:
27482,27482,27482,27482,27482,
27482,27482,27482,27482,27482,
27482,27482,27482,27482,27482,
27482,27482,27482,27482,27482,
27482,27482,27482,27482,27482,
27482,27482,27482,27482,27482,


//B Coeff:number:36
//center x:993 y:557 maxlength:1137
4096,4096,4108,4134,4159,
4199,4252,4307,4392,4480,
4588,4718,4873,5057,5277,
5540,5830,6181,6577,7027,
7544,8142,8784,9537,10432,
11316,12479,13625,15173,16485,
17803,19352,21537,23022,24278,
25678,
//continue:
25678,25678,25678,25678,
25678,25678,25678,25678,25678,
25678,25678,25678,25678,25678,
25678,25678,25678,25678,25678,
25678,25678,25678,25678,25678,
25678,25678,25678,25678,25678
};
#endif

#if 0
//  from zhl use大镜头 黑
#define CenterRx		979//964
#define CenterRy		464//494
#define CenterGx		945//983
#define CenterGy		510//478
#define CenterBx		990//997
#define CenterBy		505//477

#endif
static void ar0330_cmos_isp_fesp_init(isp_fesp_ptr_t p_fesp)	// 镜头校正, fix-pattern-correction, 坏点去除初始设置
{
  int i, j, k;
  int x0, y0, x1, y1, x2, y2, x3, y3;
  int a, b, c, d, e, f, delta;
    unsigned short R_lenslut[65];
  unsigned short G_lenslut[65];
  unsigned short B_lenslut[65];
  unsigned short Y_lenslut[65];
  
  p_fesp->Lensshade.enable = 0;
  p_fesp->Lensshade.scale = 1;
  p_fesp->Lensshade.rcenterRx = CenterRx;//661;//0x27a;//1280/2;
  p_fesp->Lensshade.rcenterRy = CenterRy;//393;//0x168;//720/2;
  p_fesp->Lensshade.rcenterGx = CenterRx;//655;//0x276;//1280/2;
  p_fesp->Lensshade.rcenterGy = CenterRy;//408;//0x16f;//720/2;
  p_fesp->Lensshade.rcenterBx = CenterRx;//656;//0x27a;//1280/2;
  p_fesp->Lensshade.rcenterBy = CenterRy;//394;//0x16c;//720/2;
  
  /*for( i=0 ; i < 195 ;i++ )
  {
    p_fesp->Lensshade.coef[i] = lenscoeff[i];
  }*/
#if 1
//  from zhl use大镜头 黑
Y_lenslut[   0] = 4096;
Y_lenslut[   1] = 4108;
Y_lenslut[   2] = 4123;
Y_lenslut[   3] = 4143;
Y_lenslut[   4] = 4169;
Y_lenslut[   5] = 4204;
Y_lenslut[   6] = 4247;
Y_lenslut[   7] = 4303;
Y_lenslut[   8] = 4371;
Y_lenslut[   9] = 4450;
Y_lenslut[  10] = 4545;
Y_lenslut[  11] = 4668;
Y_lenslut[  12] = 4817;
Y_lenslut[  13] = 4987;
Y_lenslut[  14] = 5194;
Y_lenslut[  15] = 5430;
Y_lenslut[  16] = 5674;
Y_lenslut[  17] = 5964;
Y_lenslut[  18] = 6408;
Y_lenslut[  19] = 6900;
Y_lenslut[  20] = 7451;
Y_lenslut[  21] = 8068;
Y_lenslut[  22] = 8763;
Y_lenslut[  23] = 9565;
Y_lenslut[  24] = 10462;
Y_lenslut[  25] = 11498;
Y_lenslut[  26] = 12681;
Y_lenslut[  27] = 14012;
Y_lenslut[  28] = 15557;
Y_lenslut[  29] = 17133;
Y_lenslut[  30] = 18458;
Y_lenslut[  31] = 20280;
Y_lenslut[  32] = 22327;
Y_lenslut[  33] = 24377;
Y_lenslut[  34] = 26874;
Y_lenslut[  35] = 28100;
Y_lenslut[  36] = 28100;
Y_lenslut[  37] = 28100;
Y_lenslut[  38] = 28100;
Y_lenslut[  39] = 28100;
Y_lenslut[  40] = 28100;
Y_lenslut[  41] = 28100;
Y_lenslut[  42] = 28100;
Y_lenslut[  43] = 28100;
Y_lenslut[  44] = 28100;
Y_lenslut[  45] = 28100;
Y_lenslut[  46] = 28100;
Y_lenslut[  47] = 28100;
Y_lenslut[  48] = 28100;
Y_lenslut[  49] = 28100;
Y_lenslut[  50] = 28100;
Y_lenslut[  51] = 28100;
Y_lenslut[  52] = 28100;
Y_lenslut[  53] = 28100;
Y_lenslut[  54] = 28100;
Y_lenslut[  55] = 28100;
Y_lenslut[  56] = 28100;
Y_lenslut[  57] = 28100;
Y_lenslut[  58] = 28100;
Y_lenslut[  59] = 28100;
Y_lenslut[  60] = 28100;
Y_lenslut[  61] = 28100;
Y_lenslut[  62] = 28100;
Y_lenslut[  63] = 28100;
Y_lenslut[  64] = 28100;
  for( i=0 ; i < 65 ;i++ )
  {
    p_fesp->Lensshade.coef[i] = Y_lenslut[i];
  }
  for( i=0 ; i < 65 ;i++ )
  {
    p_fesp->Lensshade.coef[i+65] = Y_lenslut[i];
  }
   for( i=0 ; i < 65 ;i++ )
  {
    p_fesp->Lensshade.coef[i+65+65] = Y_lenslut[i];
  }
#endif
  // fix pattern correction
  p_fesp->Fixpatt.enable = 1;
  p_fesp->Fixpatt.mode = 0;
  p_fesp->Fixpatt.rBlacklevel = 2;
  p_fesp->Fixpatt.grBlacklevel = 2;
  p_fesp->Fixpatt.gbBlacklevel = 2;
  p_fesp->Fixpatt.bBlacklevel = 2;
  p_fesp->Fixpatt.profile[0] = 128;
  p_fesp->Fixpatt.profile[1] = 128;
  p_fesp->Fixpatt.profile[2] = 128;
  p_fesp->Fixpatt.profile[3] = 128;
  p_fesp->Fixpatt.profile[4] = 128;
  p_fesp->Fixpatt.profile[5] = 128;
  p_fesp->Fixpatt.profile[6] = 128;
  p_fesp->Fixpatt.profile[7] = 128;
  p_fesp->Fixpatt.profile[8] = 255;
  p_fesp->Fixpatt.profile[9] = 255;
  p_fesp->Fixpatt.profile[10] = 255;
  p_fesp->Fixpatt.profile[11] = 255;
  p_fesp->Fixpatt.profile[12] = 255;
  p_fesp->Fixpatt.profile[13] = 255;
  p_fesp->Fixpatt.profile[14] = 255;
  p_fesp->Fixpatt.profile[15] = 255;
  p_fesp->Fixpatt.profile[16] = 255;
  
  // bad pixel correction
  p_fesp->Badpix.enable = 1;
  p_fesp->Badpix.mode = 1; 
  p_fesp->Badpix.thresh = 128;
  p_fesp->Badpix.profile[0] = 255;
  p_fesp->Badpix.profile[1] = 255;
  p_fesp->Badpix.profile[2] = 255;
  p_fesp->Badpix.profile[3] = 255;
  p_fesp->Badpix.profile[4] = 255;
  p_fesp->Badpix.profile[5] = 255;
  p_fesp->Badpix.profile[6] = 255;
  p_fesp->Badpix.profile[7] = 255;
  p_fesp->Badpix.profile[8] = 255;
  p_fesp->Badpix.profile[9] = 255;
  p_fesp->Badpix.profile[10] = 255;
  p_fesp->Badpix.profile[11] = 255;
  p_fesp->Badpix.profile[12] = 255;
  p_fesp->Badpix.profile[13] = 255;
  p_fesp->Badpix.profile[14] = 255;
  p_fesp->Badpix.profile[15] = 255;
  
  // cross talk correction
  p_fesp->Crosstalk.enable = 1;
  p_fesp->Crosstalk.mode = 0;
  p_fesp->Crosstalk.thresh = 0;
  p_fesp->Crosstalk.profile[0] = 255;
  p_fesp->Crosstalk.profile[1] = 255;
  p_fesp->Crosstalk.profile[2] = 255;
  p_fesp->Crosstalk.profile[3] = 255;
  p_fesp->Crosstalk.profile[4] = 255;
  p_fesp->Crosstalk.profile[5] = 255;
  p_fesp->Crosstalk.profile[6] = 255;
  p_fesp->Crosstalk.profile[7] = 255;
  p_fesp->Crosstalk.profile[8] = 255;
  p_fesp->Crosstalk.profile[9] = 255;
  p_fesp->Crosstalk.profile[10] = 255;
  p_fesp->Crosstalk.profile[11] = 255;
  p_fesp->Crosstalk.profile[12] = 255;
  p_fesp->Crosstalk.profile[13] = 255;
  p_fesp->Crosstalk.profile[14] = 255;
  p_fesp->Crosstalk.profile[15] = 255;
  p_fesp->Crosstalk.profile[16] = 255;
  
/* 
  {
    delta = 16;
    x0 = 16; y0 = 64;
    x1 = 8;  y1 = 64+delta;
    x2 = 4;  y2 = 96+delta;
    x3 = 0;  y3 = 255+delta;
    
    f = -(-x2*y3+y2*x3)/(-x3+x2);
    e = (y2-y3)/(-x3+x2);
    d = (-x2*y1+y2*x1)/(x1-x2);
    c = (-y2+y1)/(x1-x2);
    b = (x1*y0-y1*x0)/(-x0+x1);
    a = (y1-y0)/(-x0+x1);
    
    for (i = 0; i < 17; i++)
    {
      if (i<=4 )
      {
         p_fesp->Crosstalk.noise[i] = e*i+f;
      }
      else if ((i>4) && (i<=8))
      {
         p_fesp->Crosstalk.noise[i] = c*i+d;
      }
      else
      {
         p_fesp->Crosstalk.noise[i] = a*i+b;
      } 
    }
  } 
*/  
  isp_fesp_init_io (p_fesp);

}	

static void ar0330_cmos_isp_enhance_init (isp_enhance_ptr_t p_enhance)	// 图像增强初始设置
{
  p_enhance->sharp.enable = 0;	//1;
  p_enhance->sharp.mode = 0;
  p_enhance->sharp.coring = 0;//3;// 增强 低频 暗的细节 取值范围： 0-7 
  p_enhance->sharp.strength = 64;//64;//32;//128; 
  p_enhance->sharp.gainmax = 256;
  
  p_enhance->bcst.enable = 0;	//1;
  p_enhance->bcst.bright = 0; // -256~255
  //                  设置为1324 导致白色变黑色
  p_enhance->bcst.contrast = 1024;//1024; // 0~1.xxx
  p_enhance->bcst.satuation = 1024; //0~1.xxx
  p_enhance->bcst.hue = (0) ; // -128~127
  p_enhance->bcst.offset0 = 0; // 0~255    // Y上的直流偏移
  p_enhance->bcst.offset1 = 128; // 0~255  // UV上的直流偏移
  
  isp_enhance_init_io (p_enhance);
}

static void ar0330_cmos_isp_ae_init (isp_ae_ptr_t p_ae)		// 自动曝光初始设置
{

}
	
static void ar0330_cmos_isp_sys_init (isp_sys_ptr_t p_sys, isp_param_ptr_t p_isp)		// 系统初始设置 (sensor pixel位数, bayer mode)
{
  p_sys->ispenbale = 1;  
  p_sys->ckpolar = 0;    
  p_sys->vcpolar = 0;    
  p_sys->hcpolar = 0;     
  p_sys->vmskenable = 1;// 自动丢帧  
  //p_sys->vmskenable = 0;
  p_sys->frameratei = 0; 
  p_sys->framerateo = 0; 
  //(0.表示8位 1:表示10位 2:表示12位  3:表示14位)
  printf("sensor bit: 0:8bit 1:10bit 2:12bit 3:14bit  \n");
  p_sys->sensorbit = 2;  
  // 0:RGGB 1:GRBG 2:BGGR 3:GBRG 
  printf("bayer mode: 0:RGGB 1:GRBG 2:BGGR 3:GBRG  ov9712=2  pp1210 720P=1 1080P=0 \n");
/*  if(IMAGE_H_SZ == 1920 )
     p_sys->bayermdoe = 0;//for pixplus //ov9712=2  //pp1210=0  
  else if(IMAGE_H_SZ == 1280)
     p_sys->bayermdoe = 2;//for pixplus //ov9712=2  //pp1210=1  
  */
  p_sys->bayermode = 1;//_ISP_BAYER_MODE_;
  p_sys->imagewidth = p_isp->image_width;
  p_sys->imageheight = p_isp->image_height;
  p_sys->imagehblank = 200;
  p_sys->zonestridex = 0;
  p_sys->zonestridey = 0;
  p_sys->sonyif = 0;
  p_sys->sonysac1 = 0xfff;
  p_sys->sonysac2 = 0x000;
  p_sys->sonysac3 = 0x000;
  p_sys->sonysac4 = 0xab0;  

  // 使能 
  p_sys->vmanSftenable = 0; 
  p_sys->vchkIntenable = 1;// 帧开始
  p_sys->pabtIntenable = 1; //点异常
  p_sys->fendIntenable = 1; // 帧完成
  p_sys->fabtIntenable = 1; // 地址异常
  p_sys->babtIntenable = 1; //总线异常
  p_sys->ffiqIntenable = 0;  //快中断
  p_sys->pendIntenable = 1;  //中止 ，如果一帧未完成，会完成该帧

  // 设置 场 
  p_sys->vmanSftset = 0;      
  p_sys->vchkIntclr = 1;
  p_sys->pabtIntclr = 1;
  p_sys->fendIntset = 1;
  p_sys->fendIntclr = 1;
  p_sys->fabtIntclr = 1;
  p_sys->babtIntclr = 1;
  p_sys->ffiqIntclr = 1;
  p_sys->pendIntclr = 1;
  p_sys->infoStaclr = 1;    

  p_sys->vchkIntraw = 0;
  p_sys->pabtIntraw = 0;
  p_sys->fendIntraw = 0;
  p_sys->fabtIntraw = 0;
  p_sys->babtIntraw = 0;
  p_sys->ffiqIntraw = 0;
  p_sys->pendIntraw = 0;

  p_sys->vchkIntmsk = 0;
  p_sys->pabtIntmsk = 0;
  p_sys->fendIntmsk = 0;
  p_sys->fabtIntmsk = 0;
  p_sys->babtIntmsk = 0;
  p_sys->ffiqIntmsk = 0;
  p_sys->pendIntmsk = 0;

  p_sys->fendIntid[0] = 0;
  p_sys->fendIntid[1] = 1;
  p_sys->fendIntid[2] = 2;
  p_sys->fendIntid[3] = 3;   
  p_sys->ffiqIntdelay = 4;// 快中断 
  p_sys->fendStaid = 0;
  p_sys->infoStadone = 0;
#if 0
	p_sys->debugmode  = 0;
	p_sys->testenable = 0; //开启dram测试模式  
	p_sys->rawmenable = 0; 
	p_sys->yuvenable  = 1;//0:关掉数据输出  1:打开
	p_sys->refenable  = 1;  
#else // catch raw
	p_sys->debugmode  = 1;
	p_sys->testenable = 0; //开启dram测试模式  
	p_sys->rawmenable = 1; 
	p_sys->yuvenable  = 1;//0:关掉数据输出  
	p_sys->refenable  = 0;     
#endif
	p_sys->yuvformat = 0;  //0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 
   printf("0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 \n");
	p_sys->dmalock = 2;    //总线锁使能 2:使能  其他数值为关闭
   //p_sys->dmalock = 0;    //总线锁使能 2:使能  其他数值为关闭
	p_sys->hstride = p_isp->image_stride; //图像跨度 16字节倍数
	p_sys->refaddr = p_isp->ref_addr;    //参考帧地址
	p_sys->rawaddr0 = p_isp->raw_addr[0];   
	p_sys->rawaddr1 = p_isp->raw_addr[1];   
	p_sys->rawaddr2 = p_isp->raw_addr[2];   
	p_sys->rawaddr3 = p_isp->raw_addr[3];   
	p_sys->yaddr0 = p_isp->y_addr[0]; 
	p_sys->uaddr0 = p_isp->u_addr[0];
	p_sys->vaddr0 = p_isp->v_addr[0];

	p_sys->yaddr1 = p_isp->y_addr[1];    
	p_sys->uaddr1 = p_isp->u_addr[1];
	p_sys->vaddr1 = p_isp->v_addr[1];

	p_sys->yaddr2 = p_isp->y_addr[2]; 
	p_sys->uaddr2 = p_isp->u_addr[2]; 
	p_sys->vaddr2 = p_isp->v_addr[2];

	p_sys->yaddr3 = p_isp->y_addr[3];              
	p_sys->uaddr3 = p_isp->u_addr[3];     
	p_sys->vaddr3 = p_isp->v_addr[3];	
}

// ISP运行设置
static void ar0330_cmos_isp_awb_run (isp_awb_ptr_t p_awb)
{
	int r2g, b2g;
    
	isp_awb_info_read (p_awb);

	r2g = p_awb->gain_r2g;
	b2g = p_awb->gain_b2g;
}

static void ar0330_cmos_isp_colors_run (isp_colors_ptr_t p_colors, isp_awb_ptr_t p_awb, isp_ae_ptr_t p_ae)
{
	
}

static void ar0330_cmos_isp_denoise_run (isp_denoise_ptr_t p_denoise, isp_ae_ptr_t p_ae)
{
  int data1, data2, data3;
  int y_thres2, y_thres0, y_thres1;
  int u_thres2, u_thres0, u_thres1;
  int v_thres2, v_thres0, v_thres1;
  
  int againLut[] = {1024,1088,1152,1216,1280,1344,1408,1472,
                       1536,1600,1664,1728,1792,1856,1920,1984,   
                       2048,2176,2304,2432,2560,2688,2816,2944,
                       3072,3200,3328,3456,3584,3712,3840,3968,
                       4096,4352,4608,4864,5120,5376,5632,5888,
                       6144,6400,6656,6912,7168,7424,7680,7936,
                       8192,8704,9216,9728,10240,10752,11264,11776,
                       12288,12800,13312,13824,14336,14848,15360,15872,
                       16384,17408,18432,19456,20480,21504,22528,23552,
                       24576,25600,26624,27648,28672,29696,30720,31744, 
                       32768,34816,36864,38912,40960,43008,45056,47104, 
                       49152,51200,53248,55296,57344,59392,61440,63488,
                       67584,71680,75776,79872,83968,88064,92160,96256,
                       100352,104448,108544,112640,116736,120832,124928,129024};
  int noiseLut[] = { 0, 4, 6, 8, 9,10,11,12,13,13,14,14,15,15,16,16, 
                    16,17,17,17,18,18,18,18,19,19,19,19,19,20,20,20,  
                    20,20,21,21,21,21,21,21,21,22,22,22,22,22,22,22,  
                    22,23,23,23,23,23,23,23,23,23,24,24,24,24,24,24,  
                    24,24,24,24,24,25,25,25,25,25,25,25,25,25,25,25,  
                    25,25,26,26,26,26,26,26,26,26,26,26,26,26,26,26,  
                    26,26,27,27,27,27,27,27,27,27,27,27,27,27,27,27,  
                    27,27,27,27,27,28,28,28,28,28,28,28,28,28,28,28,  
                    28,28,28,28,28,28,28,28,28,28,28,29,29,29,29,29,  
                    29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,  
                    29,29,29,29,29,30,30,30,30,30,30,30,30,30,30,30,  
                    30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,  
                    30,30,30,30,30,31,31,31,31,31,31,31,31,31,31,31,  
                    31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,  
                    31,31,31,31,31,31,31,31,31,31,32,32,32,32,32,32,  
                    32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32};
  int noiseLig[] = {0,0,0,0,1,2,3,4,5,6,7,8,9,10,11,12};                     
  int iso_gain = p_ae->dgainUpd * againLut[p_ae->againUpd]/(64*1024);
  int nosie_thresh = noiseLut[iso_gain-1]*3/2; 
  int index_lowlig = ((p_ae->etimeUpd>>8) > 255) ? 15 : ((p_ae->etimeUpd>>12) & 0xf);
  //int Is_higlig = ((p_ae->etimeUpd>>8) == 0) ? 1 : 0;
  int noise_lowlig = noiseLig[index_lowlig]/2;
  
  int nosie_thresh1 = 8;
  //if ((p_ae->againComp ==1) | (p_ae->dgainComp == 1))
  {
     y_thres2 = p_denoise->y_thres2 + nosie_thresh; //+ noise_lowlig; 
     u_thres2 = p_denoise->u_thres2 + nosie_thresh; //+ noise_lowlig;  
     v_thres2 = p_denoise->u_thres2 + nosie_thresh; //+ noise_lowlig;   
   
     y_thres0 = p_denoise->y_thres0 + nosie_thresh;    
     u_thres0 = p_denoise->u_thres0 + nosie_thresh;
     v_thres0 = p_denoise->v_thres0 + nosie_thresh;
  
     y_thres1 = p_denoise->y_thres1 + nosie_thresh/2;   
     u_thres1 = p_denoise->u_thres1 + nosie_thresh/2;
     v_thres1 = p_denoise->v_thres1 + nosie_thresh/2; 
  }
     data1 = (y_thres0) | (u_thres0<<10) | (v_thres0<<20);
     data2 = (y_thres1) | (u_thres1<<10) | (v_thres1<<20);
     data3 = (y_thres2) | (u_thres2<<10) | (v_thres2<<20);  
  
     Gem_write ((GEM_DENOISE_BASE+0x04), data1);
     Gem_write ((GEM_DENOISE_BASE+0x08), data2);
     Gem_write ((GEM_DENOISE_BASE+0x0c), data3); 
     printf ("y_thres2 = 0x%08x\n", y_thres2);	
     printf ("iso_gain = 0x%08x\n", iso_gain);	
}

static void ar0330_cmos_isp_eris_run (isp_eris_ptr_t p_eris ,isp_ae_ptr_t p_ae)
{
	
}

static void ar0330_cmos_isp_fesp_run (isp_fesp_ptr_t p_fesp, isp_ae_ptr_t p_ae)
{
    int i, data0, data1;
    
	if (p_ae->etimeUpd >= p_ae->etimeMax)
	{
	   p_fesp->Badpix.thresh = 255;
       p_fesp->Badpix.profile[0] = 0;
       p_fesp->Badpix.profile[1] = 0;//255;
       p_fesp->Badpix.profile[2] = 0;//255;
       p_fesp->Badpix.profile[3] = 0;//255;
       p_fesp->Badpix.profile[4] = 0;//255;
       p_fesp->Badpix.profile[5] = 0;//255;
       p_fesp->Badpix.profile[6] = 0;//255;
       p_fesp->Badpix.profile[7] = 255;
       p_fesp->Badpix.profile[8] = 255;
       p_fesp->Badpix.profile[9] = 255;
       p_fesp->Badpix.profile[10] = 255;
       p_fesp->Badpix.profile[11] = 255;
       p_fesp->Badpix.profile[12] = 255;
       p_fesp->Badpix.profile[13] = 255;
       p_fesp->Badpix.profile[14] = 255;
       p_fesp->Badpix.profile[15] = 255;
       
       Gem_write ((GEM_BADPIX_BASE+0x04), data1);
       for (i = 0; i < 16; i++)
       {
           data0 = (0x0a) | (i << 8) | (p_fesp->Badpix.profile[i]<<16);
           Gem_write ((GEM_LUT_BASE+0x00), data0);
       }
	}
	
}

static void ar0330_cmos_isp_ae_run (isp_ae_ptr_t p_ae)
{
	
}


u32_t isp_init_cmos_sensor (cmos_sensor_t *cmos_sensor)
{
	memset (cmos_sensor, 0, sizeof(cmos_sensor_t));
	cmos_sensor->cmos_gain_initialize = cmos_gain_initialize;
	cmos_sensor->cmos_inttime_initialize = cmos_inttime_initialize;
	cmos_sensor->cmos_inttime_gain_update = cmos_inttime_gain_update;
	cmos_sensor->analog_gain_from_exposure_calculate = analog_gain_from_exposure_calculate;
	cmos_sensor->digital_gain_from_exposure_calculate = digital_gain_from_exposure_calculate;
	cmos_sensor->cmos_get_iso = cmos_get_iso;
	cmos_sensor->cmos_fps_set = cmos_fps_set;
	cmos_sensor->cmos_sensor_set_readout_direction = cmos_sensor_set_readout_direction;
	
	cmos_sensor->cmos_sensor_get_sensor_name = ar0330_cmos_sensor_get_sensor_name;
	// sensor初始化
	cmos_sensor->cmos_isp_sensor_init = ar0330_isp_sensor_init;
	
	cmos_sensor->cmos_isp_awb_init = ar0330_cmos_isp_awb_init;
	cmos_sensor->cmos_isp_colors_init = ar0330_cmos_isp_colors_init;
	cmos_sensor->cmos_isp_denoise_init = ar0330_cmos_isp_denoise_init;
	cmos_sensor->cmos_isp_eris_init = ar0330_cmos_isp_eris_init;
	cmos_sensor->cmos_isp_fesp_init = ar0330_cmos_isp_fesp_init;
	cmos_sensor->cmos_isp_enhance_init = ar0330_cmos_isp_enhance_init;
	cmos_sensor->cmos_isp_ae_init = ar0330_cmos_isp_ae_init;
	cmos_sensor->cmos_isp_sys_init = ar0330_cmos_isp_sys_init;

	cmos_sensor->cmos_isp_awb_run = ar0330_cmos_isp_awb_run;
	cmos_sensor->cmos_isp_colors_run = ar0330_cmos_isp_colors_run;
	cmos_sensor->cmos_isp_denoise_run = ar0330_cmos_isp_denoise_run;
	cmos_sensor->cmos_isp_eris_run = ar0330_cmos_isp_eris_run;
	cmos_sensor->cmos_isp_fesp_run = ar0330_cmos_isp_fesp_run;
	cmos_sensor->cmos_isp_ae_run = ar0330_cmos_isp_ae_run;
	
		// 初始化sensor
	//(*cmos_sensor->cmos_isp_sensor_init) (NULL);

	
	return 0;
}

#endif	// ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_AR0330