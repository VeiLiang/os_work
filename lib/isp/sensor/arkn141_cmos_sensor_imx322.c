#include "arkn141_isp_sensor_cfg.h"
#include "hardware.h"

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX322 || ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX323


#include "arkn141_isp_exposure_cmos.h"
#include "arkn141_isp_cmos_sensor_io.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "gem_isp_io.h"
#include "xm_h264_codec.h"

#ifdef WIN32
#include "xm_sensor_simulate.h"
#endif


#define	_DISABLE_GAMMA_UNDER_LOW_LIGHT_

#define	EXPOSURE_LINES_ADDR		(0x0202)	// Integration time adjustment (I2C) Designated in line units	
#define	AGAIN_ADDR					(0x301E)	// Gain setting

#define	STD_30_LINES	(1125+20)		
//#define	STD_30_LINES	2200		

#define	CMOS_STD_INTTIME	(1125+20-2)


extern int imx322_init_12bit  (unsigned int frame_lines);
extern int imx322_init_1080p_30fps_10bit  (unsigned int frame_lines);

extern int imx322_init_10bit  (unsigned int frame_lines);
extern int imx322_init_720p_10bit_30fps_mode (unsigned int frame_lines);


static cmos_inttime_t cmos_inttime;
static cmos_gain_t cmos_gain;

static u16_t sensor_frame_rate = 1;
//static u16_t FRM_LENGTH = 1125;	// 1080P
//static u16_t FRM_LENGTH = 2200;		// 720P
static u16_t FRM_LENGTH = STD_30_LINES;


static u32_t last_exp_time = 0xFFFFFFFF;
static u32_t last_again = 0xFFFFFFFF;
static u32_t last_dgain = 0xFFFFFFFF;

static void cmos_inttime_gain_reset (void)
{
	last_exp_time = 0xFFFFFFFF;
	last_again = 0xFFFFFFFF;
	last_dgain = 0xFFFFFFFF;
}

// PCLK = 74.25MHz
// Frame Size = 2200 * 1125
// fps = 74.25 * 1000000 / (2200 * 1125) = 30 帧/秒
static  cmos_inttime_ptr_t cmos_inttime_initialize(void)
{
	cmos_inttime.full_lines = FRM_LENGTH;
	cmos_inttime.full_lines_limit = 65535;
	cmos_inttime.max_lines_target = (u16_t)(FRM_LENGTH - 1);
	//cmos_inttime.min_lines_target = 1;
	cmos_inttime.min_lines_target = 2;	// 改善逆光下太暗的情况
	
	cmos_inttime.exposure_ashort = 0;
	if(last_exp_time != 0xFFFFFFFF)
		cmos_inttime.exposure_ashort = last_exp_time;

	return &cmos_inttime;
}

// The sensor's integration time is obtained by the following formula.
// Integration time = 1 frame period × (SVS + 1 - SPL) - (INTEG_TIME) × (1H period) - 0.3 [H] (However, SVS > SPL)
static void cmos_inttime_update (cmos_inttime_ptr_t p_inttime) 
{
	u16_t exp_time;
#ifdef WIN32
#else
	u16_t shutter_sweep_line_count;
#endif

	exp_time = (u16_t)p_inttime->exposure_ashort;

#ifdef WIN32
	win32_sensor_inttime_update (exp_time);
#else
	shutter_sweep_line_count = FRM_LENGTH - exp_time;
	//XM_printf ("INTEG_TIME = %4d\n", shutter_sweep_line_count);
   arkn141_isp_cmos_sensor_write_register (EXPOSURE_LINES_ADDR + 0, (u8_t)(shutter_sweep_line_count >> 8) );	// INTEG_TIME [15:8]
	arkn141_isp_cmos_sensor_write_register (EXPOSURE_LINES_ADDR + 1, (u8_t)(shutter_sweep_line_count     ) );	// INTEG_TIME [7:0]
#endif
}

//#define	ONLY_ANALOG
// 不使用数字增益. 使用数字增益在场景光强急剧变化时会带来较大的噪声.
// 使用eris进行拉伸
// 20170106晚上的测试已验证, 开启部分或全部数字增益时需要使用较大的降噪因子, 而这会导致画面模糊(3D运动模糊)

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX323

#ifdef ONLY_ANALOG
// ANALOG gain
static const u16_t analog_gain_table[] = 
{
	  256, 	  265, 	  274, 	  284, 	  294, 	  304, 	  315, 	  326, 
	  337, 	  349, 	  362, 	  374, 	  387, 	  401, 	  415, 	  430, 
	  445, 	  461, 	  477, 	  493, 	  511, 	  529, 	  547, 	  567, 
	  586, 	  607, 	  628, 	  650, 	  673, 	  697, 	  722, 	  747, 
	  773, 	  800, 	  828, 	  858, 	  888, 	  919, 	  951, 	  985, 
	 1019, 	 1055, 	 1092, 	 1130, 	 1170, 	 1211, 	 1254, 	 1298, 
	 1344, 	 1391, 	 1440, 	 1490, 	 1543, 	 1597, 	 1653, 	 1711, 
	 1771, 	 1833, 	 1898, 	 1964, 	 2033, 	 2105, 	 2179, 	 2255, 
	 2335, 	 2417, 	 2502, 	 2590, 	 2681, 	 2775, 	 2872, 	 2973, 
	 3078,
	 

};
	 
#else
// ANALOG + digital gain
static const u16_t analog_gain_table[] = 
{
	// analog gain, 信噪比较好
	  256, 	  265, 	  274, 	  284, 	  294, 	  304, 	  315, 	  326, 
	  337, 	  349, 	  362, 	  374, 	  387, 	  401, 	  415, 	  430, 
	  445, 	  461, 	  477, 	  493, 	  511, 	  529, 	  547, 	  567, 
	  586, 	  607, 	  628, 	  650, 	  673, 	  697, 	  722, 	  747, 
	  773, 	  800, 	  828, 	  858, 	  888, 	  919, 	  951, 	  985, 
	 1019, 	 1055, 	 1092, 	 1130, 	 1170, 	 1211, 	 1254, 	 1298, 
	 1344, 	 1391, 	 1440, 	 1490, 	 1543, 	 1597, 	 1653, 	 1711, 
	 1771, 	 1833, 	 1898, 	 1964, 	 2033, 	 2105, 	 2179, 	 2255, 
	 2335, 	 2417, 	 2502, 	 2590, 	 2681, 	 2775, 	 2872, 	 2973, 
	 3078, 	 
	 
	 // digital gain, 信噪比较差
	 3186, 	 3298, 	 3414, 	 3534, 	 3658, 	 3787, 	 3920, 
	 4057, 	 4200, 	 4348, 	 4500, 	 4658, 	 4822, 	 4992, 	 5167, 
	 5349, 	 5537, 	 5731, 	 5933, 	 6141, 	 6357, 	 6580, 	 6811, 
	 7051, 	 7299, 	 7555, 	 7821, 	 8095, 	 8380, 	 8674, 	 8979, 
	 9295, 	 9621, 	 9960, 	10310, 	10672, 	11047, 	11435, 	11837, 
	12253, 	12684, 	13129, 	13591, 	14068, 	14563, 	15074, 	15604, 
	16153, 	16720, 	17308, 	17916, 	18546, 	19197, 	19872, 	20570, 
	21293, 	22041, 	22816, 	23618, 	24448, 	25307, 	26196, 	27117, 
	28070, 	29056, 	30077, 	31134, 	32228, 	33361, 	34533, 	35747, 
	37003, 	38304, 	39650, 	41043, 	42485, 	43978, 	45524
};
#endif

#elif ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX322


#ifdef ONLY_ANALOG
// ANALOG gain
static const u16_t analog_gain_table[] =
{
	  512, 	  530, 	  549, 	  568, 	  588, 	  609, 	  630, 	  652, 
	  675, 	  699, 	  723, 	  749, 	  775, 	  802, 	  830, 	  860, 
	  890, 	  921, 	  953, 	  987, 	 1022, 	 1057, 	 1095, 	 1133, 
	 1173, 	 1214, 	 1257, 	 1301, 	 1347, 	 1394, 	 1443, 	 1494, 
	 1546, 	 1601, 	 1657, 	 1715, 	 1775, 	 1838, 	 1902, 	 1969, 
	 2038, 	 2110, 	 2184, 	 2261, 	 2340, 	 2423, 	 2508, 	 2596, 
	 2687, 	 2781, 	 2879, 	 2980, 	 3085, 	 3194, 	 3306, 	 3422, 
	 3542, 	 3667, 	 3796, 	 3929, 	 4067, 	 4210, 	 4358, 	 4511, 
	 4669, 	 4834, 	 5003, 	 5179, 	 5361, 	 5550, 	 5745, 	 5947, 
	 6156, 	 6372, 	 6596, 	 6828, 	 7068, 	 7316, 	 7573, 	 7839, 
	 8115,
};

#else

// ANALOG + digital gain
static const u16_t analog_gain_table[] =
{
	// ANALOG gain
	  512, 	  530, 	  549, 	  568, 	  588, 	  609, 	  630, 	  652, 
	  675, 	  699, 	  723, 	  749, 	  775, 	  802, 	  830, 	  860, 
	  890, 	  921, 	  953, 	  987, 	 1022, 	 1057, 	 1095, 	 1133, 
	 1173, 	 1214, 	 1257, 	 1301, 	 1347, 	 1394, 	 1443, 	 1494, 
	 1546, 	 1601, 	 1657, 	 1715, 	 1775, 	 1838, 	 1902, 	 1969, 
	 2038, 	 2110, 	 2184, 	 2261, 	 2340, 	 2423, 	 2508, 	 2596, 
	 2687, 	 2781, 	 2879, 	 2980, 	 3085, 	 3194, 	 3306, 	 3422, 
	 3542, 	 3667, 	 3796, 	 3929, 	 4067, 	 4210, 	 4358, 	 4511, 
	 4669, 	 4834, 	 5003, 	 5179, 	 5361, 	 5550, 	 5745, 	 5947, 
	 6156, 	 6372, 	 6596, 	 6828, 	 7068, 	 7316, 	 7573, 	 7839, 
	 8115, 	
	
	// digital gain
	 8400, 	 8695, 	 9001, 	 9317, 	 9644, 	 9983, 	10334, 
	10697, 	11073, 	11462, 	11865, 	12282, 	12714, 	13160, 	13623, 
	14102, 	14597, 	15110, 	15641, 	16191, 	16760, 	17349, 	17958, 
	18590, 	19243, 	19919, 	20619, 	21344, 	22094, 	22870, 	23674, 
	24506, 	25367, 	26259, 	27181, 	28136, 	29125, 	30149, 	31208, 
	32305, 	33440, 	34615, 	35832, 	37091, 	38395, 	39744, 	41141, 
	42586, 	44083, 	45632, 	47236, 	48896, 	50614, 	52393, 	54234, 
	56140, 	58113, 	60155, 	62269, 	64457
};
#endif

#endif	// ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX323

 // 设置CMOS sensor允许使用的最大增益
static int imx322_cmos_max_gain_set (cmos_gain_ptr_t gain, unsigned int max_analog_gain,  unsigned int max_digital_gain);


// The Programmable Gain Control (PGC) of this device consists of the analog block and digital block.
// The total of analog gain and digital gain can be set up to 42 dB by the GAIN register (address 1Eh [7:0]) setting.
static cmos_gain_ptr_t cmos_gain_initialize(void)
{
#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX323
	
	cmos_gain.again_shift = 8;
#ifdef ONLY_ANALOG
	 cmos_gain.max_again_target = (u16_t)(3078);
#else
	cmos_gain.max_again_target = (u16_t)(45524);		// 178倍增益
	//cmos_gain.max_again_target = (u16_t)(32305);
	//cmos_gain.max_again_target = (u16_t)(16384); 	// 64倍增益
#endif
	
#elif ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX322
	
	cmos_gain.again_shift = 9;
#ifdef ONLY_ANALOG
	cmos_gain.max_again_target = (u16_t)(8115);
#else
	cmos_gain.max_again_target = (u16_t)(64457);
#endif
	
#endif
	cmos_gain.again_count = sizeof(analog_gain_table)/sizeof(analog_gain_table[0]);
	
	cmos_gain.dgain_shift = 0;
	cmos_gain.max_dgain_target = 1;		// 禁止
	cmos_gain.dgain_count = 0;
	
#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX323

	//imx322_cmos_max_gain_set (&cmos_gain, 	32305, 1);		// 20170119晚上测试, 1 ) 车库效果较佳, 稍微存在一点过曝. 
																			//                   2 ) 马路上路灯及广告灯箱等存在严重过曝
																			//		应该是数字增益(8979)太大
	//imx322_cmos_max_gain_set (&cmos_gain, 	16384, 1);		// 64倍增益
	imx322_cmos_max_gain_set (&cmos_gain, 	45524, 1);		// 177倍增益
	
	//imx322_cmos_max_gain_set (&cmos_gain, 	3920, 1);		// 稍微开启部分数字增益
	//imx322_cmos_max_gain_set (&cmos_gain, 	3078, 1);		// 20170223 仅开启模拟增益, 减小夜晚灯光的光晕现象
	
#endif
	
	if(last_again != 0xFFFFFFFF)
	{
		cmos_gain.aindex = last_again;
	}

	return &cmos_gain;
}

// 设置CMOS sensor允许使用的最大增益
int imx322_cmos_max_gain_set (cmos_gain_ptr_t gain, unsigned int max_analog_gain,  unsigned int max_digital_gain)
{
	int count, index;
	if(gain == NULL)
		return -1;
	if(max_analog_gain == 0)
	{
		max_analog_gain = 1;		// 禁止模拟增益
	}
		
	if(gain->max_again_target != 1)
	{
		// 在增益表中查找该最大增益值
		count = sizeof(analog_gain_table) / sizeof(analog_gain_table[0]);
		for (index = 0; index < count; index ++)
		{
			if(analog_gain_table[index] >= max_analog_gain)
			{
				break;
			}
		}
		if(index >= count)
			index = count - 1;	// 使用最后一个增益值
		
		// 修改最大可用的增益值
		gain->max_again_target = analog_gain_table[index];
		gain->again_count = index + 1;		// 修改可用的增益项数量
	}
	return 0;
}

// 设置CMOS sensor允许使用的最大增益
int imx322_cmos_max_gain_get (cmos_gain_ptr_t gain, unsigned int* max_analog_gain,  unsigned int* max_digital_gain)
{
	if(gain == NULL)
		return -1;
	*max_analog_gain = gain->max_again_target;
	*max_digital_gain = gain->max_dgain_target;
	return 0;
}

static void cmos_gain_update (cmos_gain_ptr_t gain)
{
#ifdef WIN32
	win32_sensor_gain_update ( ((double)gain->dgain) / (1 << gain->dgain_shift) ,
										((double)gain->again) / (1 << gain->again_shift) );
#else
//	XM_printf ("AGAIN_REG, 0x%04x\n", gain->aindex);
//	XM_printf ("DGAIN_REG, 0x%04x\n", gain->dindex);

	arkn141_isp_cmos_sensor_write_register (AGAIN_ADDR, (u16_t)gain->aindex);
#endif
}

// 根据曝光量计算模拟增益
static u32_t analog_gain_from_exposure_calculate (cmos_gain_ptr_t gain, u32_t exposure, u32_t exposure_max)
{
	// 二分法查找最接近的模拟增益
	// 计算精度非常重要,会导致曝光的抖动
	int l, h, m, match;
	i64_t exp;
	i64_t mid;
	u32_t again = 1 << gain->again_shift;
	match = 0;
	if(exposure <= exposure_max)
	{
		gain->again = analog_gain_table[0];
		gain->aindex = 0;
		return exposure;
	}
	if(gain->again_count == 0)
	{
		gain->again = analog_gain_table[0];
		gain->aindex = 0;
		return exposure;		
	}
	l = 0;
	h = gain->again_count - 1;
	
	if(h < 0)
		h = 0;
	
	//h = sizeof(analog_gain_table)/sizeof(analog_gain_table[0]) - 1;
	//exp = (i64_t)(1 << gain->again_shift);
	//exp = exp * (i64_t)exposure;
	exp = exposure;
	while(l <= h)
	{
		m = (l + h) >> 1;
		mid = analog_gain_table[m];
		mid = mid * (i64_t)exposure_max;
		mid = mid / again;
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
	gain->again = analog_gain_table[m];
	gain->aindex = (u16_t)m;
	//return (u32_t)(exp / analog_gain_table[m]);
	exp = exp * (i64_t)again;
	exp = exp / analog_gain_table[m];
	return (u32_t)exp;
}

static void cmos_inttime_gain_update (cmos_inttime_ptr_t p_inttime, cmos_gain_ptr_t gain) 
{
	u16_t exp_time;
	u16_t shutter_sweep_line_count;

	// Register Hold
	arkn141_isp_cmos_sensor_write_register (0x0104, 0x01);		// register setting hold
	if(p_inttime)
	{
		exp_time = (u16_t)p_inttime->exposure_ashort;
		last_exp_time = exp_time;
	
		shutter_sweep_line_count = FRM_LENGTH - exp_time;
		//XM_printf ("sweep_line=%d\n", shutter_sweep_line_count);
		arkn141_isp_cmos_sensor_write_register (EXPOSURE_LINES_ADDR + 0, (u8_t)(shutter_sweep_line_count >> 8) );	// INTEG_TIME [15:8]
		arkn141_isp_cmos_sensor_write_register (EXPOSURE_LINES_ADDR + 1, (u8_t)(shutter_sweep_line_count     ) );	// INTEG_TIME [7:0]
	}
	
	if(gain)
	{
		//XM_printf ("aindex=%d\n", gain->aindex);
		last_again = gain->aindex;
		arkn141_isp_cmos_sensor_write_register (AGAIN_ADDR, (u16_t)gain->aindex);
	}
	
	arkn141_isp_cmos_sensor_write_register (0x0104, 0x00);		// reflection is applied
}

static void cmos_inttime_gain_update_manual (cmos_inttime_ptr_t p_inttime, cmos_gain_ptr_t gain) 
{
	u16_t exp_time;
	u16_t shutter_sweep_line_count;
	int i, aindex, dindex;
	
	// 计算aindex, dindex
	for (i = 0; i < gain->again_count; i++)
	{
		if(analog_gain_table[i] >= gain->again)
			break;
	}
	if(i == gain->again_count)
		i = gain->again_count - 1;
	aindex = i;
	gain->aindex = aindex;

	// Register Hold
	arkn141_isp_cmos_sensor_write_register (0x0104, 0x01);		// register setting hold
	if(p_inttime)
	{
		exp_time = (u16_t)p_inttime->exposure_ashort;
	
		shutter_sweep_line_count = FRM_LENGTH - exp_time;
		//XM_printf ("sweep_line=%d\n", shutter_sweep_line_count);
		arkn141_isp_cmos_sensor_write_register (EXPOSURE_LINES_ADDR + 0, (u8_t)(shutter_sweep_line_count >> 8) );	// INTEG_TIME [15:8]
		arkn141_isp_cmos_sensor_write_register (EXPOSURE_LINES_ADDR + 1, (u8_t)(shutter_sweep_line_count     ) );	// INTEG_TIME [7:0]
	}
	
	if(gain)
	{
		arkn141_isp_cmos_sensor_write_register (AGAIN_ADDR, (u16_t)gain->aindex);
	}
	
	arkn141_isp_cmos_sensor_write_register (0x0104, 0x00);		// reflection is applied
}


static u32_t cmos_get_iso (cmos_gain_ptr_t gain)
{
	i64_t iso = gain->again;
	iso = (iso * 100) >> (gain->again_shift);
	
	gain->iso =  (u32_t)iso; 
	
	return gain->iso;
}

static void cmos_fps_set (cmos_inttime_ptr_t p_inttime, u8_t fps)
{
	switch (fps)
	{
		case 30:
		default:
			p_inttime->full_lines = FRM_LENGTH;	//STD_30_LINES;
			p_inttime->lines_per_500ms = FRM_LENGTH * 30 / 2;	//STD_30_LINES * 30 / 2;
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
	int val = 0;
	if(vert_reverse_direction)
		val |= 1 << 1;			// revert

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX323
	if(horz_reverse_direction)
		val |= 1 << 0;			// revert
#endif	
	
	ret = arkn141_isp_cmos_sensor_write_register (0x0101, val);	
	
	return ret;
}

static const char *imx322_cmos_sensor_get_sensor_name (void)
{
#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX322
	return "IMX322";	
#elif ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX323
	return "IMX323";	
#endif
}

#include "gpio.h"
#include "rtos.h"
extern int imx322_init_720p_12bit_30fps_mode (unsigned int frame_lines);
extern int imx322_init_720p_10bit_60fps_mode (unsigned int frame_lines);
extern void isp_sensor_set_reset_pin_low (void);
extern void isp_sensor_set_reset_pin_high (void);



static int imx322_isp_sensor_init(isp_sen_ptr_t p_sen)
{
	int ret = 0;
	int loop = 10;
	int video_format = XMSYS_H264CodecGetVideoFormat();
	if( video_format == ARKN141_VIDEO_FORMAT_1080P_30)
	{	
		while(loop > 0)
		{
			// tLOW >= 500ns
			isp_sensor_set_reset_pin_low ();
			OS_Delay (1);
			isp_sensor_set_reset_pin_high ();
			OS_Delay (1);
			// 1920x1080
			//FRM_LENGTH = 1125 + 0x20;		// 0x465 + 0x20
			//FRM_LENGTH = 1125 + 0x120;		// 0x465 + 0x20
			//FRM_LENGTH = 1125 + 90;		// scale 40M输入/30帧输出
			//FRM_LENGTH = 1125 + 900;		// scale 40M输入/30帧输出
			//FRM_LENGTH = 1125 + 20;		// scale不出现pop error的最小值
			//FRM_LENGTH = 1125 + 20;		// scale不出现pop error的最小值
			//FRM_LENGTH = 1125 + 120;
			//FRM_LENGTH = 1125 + 350;
			FRM_LENGTH = 1125 + 150;
			//FRM_LENGTH = 1125 + 450;
			//FRM_LENGTH = 1125 + 30;	
			//if(isp_get_sensor_bit () == ARKN141_ISP_SENSOR_BIT_12)
			{
				ret = imx322_init_12bit (FRM_LENGTH);
#if 0//TULV_USB
				// 镜头旋转180度
				if(ret == 0)
				{
					cmos_sensor_set_readout_direction (1, 1);
				}
#endif
			}
			//else
			{
				//ret = imx322_init_1080p_30fps_10bit  (FRM_LENGTH);

				//ret = imx322_init_10bit (FRM_LENGTH);
			}
			
			if(ret == 0)
			{
				break;
			}
			loop --;
		}
		
		if(loop == 0)
		{
			ret = -1;
			XM_printf ("imx322 init 1080p NG\n");
		}
		else
		{
			ret = 0;
			XM_printf ("imx322 init 1080p OK\n");
		}
	}
	else if(		video_format == ARKN141_VIDEO_FORMAT_720P_30 
#ifndef HONGJING_CVBS
			  || 	video_format == ARKN141_VIDEO_FORMAT_720P_60
#endif
				  )
	{
		// 1280x720
		FRM_LENGTH = 0x02EE + 28;		// 0x02EE = 750
		while(loop > 0)
		{
			// tLOW >= 500ns
			isp_sensor_set_reset_pin_low ();
			OS_Delay (1);
			isp_sensor_set_reset_pin_high ();
			OS_Delay (1);
			if(video_format == ARKN141_VIDEO_FORMAT_720P_30)
			{
				//if(isp_get_sensor_bit() == ARKN141_ISP_SENSOR_BIT_12)
				//	ret = imx322_init_720p_12bit_30fps_mode (FRM_LENGTH);
				//else
					ret = imx322_init_720p_10bit_30fps_mode (FRM_LENGTH);
			}
			else
				ret = imx322_init_720p_10bit_60fps_mode (FRM_LENGTH);
			//if(isp_get_sensor_bit() == ARKN141_ISP_SENSOR_BIT_12)
			//	ret = imx322_init_720p_12bit_30fps_mode (FRM_LENGTH);
			//else
			//	ret = imx322_init_720p_10bit_30fps_mode (FRM_LENGTH);
			//	ret = imx322_init_720p_10bit_60fps_mode (FRM_LENGTH);
				
			if(ret == 0)
				break;
			loop --;
		}
		if(loop == 0)
		{
			ret = -1;
			XM_printf ("imx322 init 720p NG\n");
		}
		else
		{
			ret = 0;
			XM_printf ("imx322 init 720p OK\n");
		}
	}
	else
	{
		XM_printf ("un-support video format (%d)\n", video_format );
		return -1;
	}
	return ret;
}

static void imx322_cmos_isp_awb_init (isp_awb_ptr_t p_awb)		// 白平衡初始参数
{
	//p_awb->enable = 0;
  p_awb->enable = 1;    
  p_awb->mode = 1; //0：无效;  1：算法1，统一估计;  2：算法2，基于参考光源
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
  //p_awb->black = 4;   
  p_awb->black = 16;     
  p_awb->white = 210; 
  p_awb->jitter = 13;
  p_awb->r2g_min = 256/4;
  p_awb->r2g_max = 256*4;
  p_awb->b2g_min = 256/4;
  p_awb->b2g_max = 256*4;
  
  // A		R_GAIN	0x127
  //			G_GAIN	0x113
  //			B_GAIN	0x386
  //
  // TL84	R_GAIN	0x1A8
  //			G_GAIN	0x113
  //			B_GAIN	0x2C9
  // D50		R_GAIN	0x1F2
  //			G_GAIN	0x113
  //			B_GAIN	0x22D
  // D65		R_GAIN	0x204
  //			G_GAIN	0x113
  //			B_GAIN	0x1CB
  // 10000K	R_GAIN	0x1F5
  //			G_GAIN	0x113
  //			B_GAIN	0x12C
  // G/R, G/B, 
  p_awb->r2g_light[0] = 136;
  p_awb->b2g_light[0] = 153;
  p_awb->r2g_light[1] = 141;
  p_awb->b2g_light[1] = 126;
  p_awb->r2g_light[2] = 141; 
  p_awb->b2g_light[2] = 235;
  p_awb->r2g_light[3] = 143;   
  p_awb->b2g_light[3] = 147;
  p_awb->r2g_light[4] = 166;
  p_awb->b2g_light[4] = 99;
  p_awb->r2g_light[5] = 239; 
  p_awb->b2g_light[5] = 78;
  p_awb->r2g_light[6] = 0;   
  p_awb->b2g_light[6] = 0;
  p_awb->r2g_light[7] = 0;   
  p_awb->b2g_light[7] = 0;
  
  p_awb->use_light[0] = 1;
  p_awb->use_light[1] = 1;
  p_awb->use_light[2] = 1;
  p_awb->use_light[3] = 1;
  p_awb->use_light[4] = 1;
  p_awb->use_light[5] = 1;
  p_awb->use_light[6] = 0;
  p_awb->use_light[7] = 0;  
  
  p_awb->gain_g2r = 500;//434;
  p_awb->gain_g2b = 410;//348;

  isp_awb_init_io (p_awb);	
}

const unsigned short gamma_LUT_short[] = {    
	0, 	    1023, 	2047, 	3071, 	4095, 	5119, 	6143,   7167, 	
	8191, 	9215, 	10239, 	11263, 	12287,  	13311, 	14335, 	15359, 
	16383, 	17407, 	18431, 	19455, 	20479, 	21503, 	22527, 	23551, 	
	24575, 	25599, 	26623, 	27647, 	28671, 	29695, 	30719, 	31743, 	
   32768,       34536,       36256,       37928,       39552,       41128,       42656,      44136, 
   45568,       46952,       48288,       49576,       50816,       52008,       53152,      54248, 
   55296,       56296,       57248,       58152,       59008,       59816,       60576,      61288, 
   61952,       62568,       63136,       63656,       64128,       64552,       64928,      65256, 
   65535
};

const unsigned short gamma_LUT0[] = {    0,         280,         608,         984,        1408,        1880,        2400,       2968, 
                   3584,        4248,        4960,        5720,        6528,        7384,        8288,       9240, 
                  10240,       11288,       12384,       13528,       14720,       15960,       17248,      18584, 
                  19968,       21400,       22880,       24408,       25984,       27608,       29280,      31000, 
                  32768,       34536,       36256,       37928,       39552,       41128,       42656,      44136, 
                  45568,       46952,       48288,       49576,       50816,       52008,       53152,      54248, 
                  55296,       56296,       57248,       58152,       59008,       59816,       60576,      61288, 
                  61952,       62568,       63136,       63656,       64128,       64552,       64928,      65256, 
                  65535};

const unsigned short gamma_LUT1_old[] = {     
	0,        140,        336,        588,        896,       1260,       1680,       2156,   
    2688,       3276,       3920,       4620,       5376,       6188,       7056,       7980,  
    8960,       9996,      11088,      12236,      13440,      14700,      16016,      17388,   
   18816,      20300,      21840,      23436,      25088,      26796,      28560,      30380,   
   32256,      34644,      36464,      38228,      39936,      41588,      43184,      44724,   
   46208,      47636,      49008,      50324,      51584,      52788,      53936,      55028,   
   56064,      57044,      57968,      58836,      59648,      60404,      61104,      61748,   
   62336,      62868,      63344,      63764,      64128,      64436,      64688,      64884,   
   65024 };

// 增强暗处的亮度, 关闭暗处的对比度拉伸
const unsigned short gamma_LUT1[] = {     
	/*
	0,        140,        336,        588,        896,       1260,       1680,       2156,   
    2688,       3276,       3920,       4620,       5376,       6188,       7056,       7980,  
    8960,       9996,      11088,      12236,      13440,      14700,      16016,      17388,   
   18816,      20300,      21840,      23436,      25088,      26796,      28560,      30380,   
	*/
	0, 	    1023, 	2047, 	3071, 	4095, 	5119, 	6143,   7167, 	
	8191, 	9215, 	10239, 	11263, 	12287,  	13311, 	14335, 	15359, 
	16383, 	17407, 	18431, 	19455, 	20479, 	21503, 	22527, 	23551, 	
	24575, 	25599, 	26623, 	27647, 	28671, 	29695, 	30719, 	31743, 	
	
   32256,      34644,      36464,      38228,      39936,      41588,      43184,      44724,   
   46208,      47636,      49008,      50324,      51584,      52788,      53936,      55028,   
   56064,      57044,      57968,      58836,      59648,      60404,      61104,      61748,   
   62336,      62868,      63344,      63764,      64128,      64436,      64688,      64884,   
   65024 };

// 增强暗处的亮度, 关闭暗处的对比度拉伸
const unsigned short gamma_LUT2[] = {     
	/*
		0,        140,        336,        588,        896,       1260,       1680,       2156,   
    2688,       3276,       3920,       4620,       5376,       6188,       7056,       7980,  
    8960,       9996,      11088,      12236,      13440,      14700,      16016,      17388,   
   18816,      20300,      21840,      23436,      25088,      26796,      28560,      30380,   
	*/
	0, 	    1023, 	2047, 	3071, 	4095, 	5119, 	6143,   7167, 	
	8191, 	9215, 	10239, 	11263, 	12287,  	13311, 	14335, 	15359, 
	16383, 	17407, 	18431, 	19455, 	20479, 	21503, 	22527, 	23551, 	
	24575, 	25599, 	26623, 	27647, 	28671, 	29695, 	30719, 	31743, 	
	
	
   32256,      33791, 		34815, 		35839, 		36863, 		37887, 		38911, 		39935, 	
	40959, 		41983, 		43007, 		44031, 		45055, 		46079, 		47103, 		48127, 	
	49151, 		50175, 		51199, 		52223, 		53247, 		54271, 		55295, 		56319, 	
	57343, 		58367, 		59391, 		60415, 		61439, 		62463, 		63487, 		64511, 	
	65535, 
};

const unsigned short gamma_linear_table[65] = {
	0, 	    1023, 	2047, 	3071, 	4095, 	5119, 	6143,   7167, 	
	8191, 	9215, 	10239, 	11263, 	12287,  	13311, 	14335, 	15359, 
	16383, 	17407, 	18431, 	19455, 	20479, 	21503, 	22527, 	23551, 	
	24575, 	25599, 	26623, 	27647, 	28671, 	29695, 	30719, 	31743, 	
	32767, 	33791, 	34815, 	35839, 	36863, 	37887, 	38911, 	39935, 	
	40959, 	41983, 	43007, 	44031, 	45055, 	46079, 	47103, 	48127, 	
	49151, 	50175, 	51199, 	52223, 	53247, 	54271, 	55295, 	56319, 	
	57343, 	58367, 	59391, 	60415, 	61439, 	62463, 	63487, 	64511, 	
	65535, 
};

const unsigned short gamma_linear_table_2[65] = {
	0, 	1016, 	2032, 	3048, 	4064, 	5080, 	6096, 
	7112, 	8128, 	9144, 	10160, 	11176, 	12192, 
	13208, 	14224, 	15240, 	16256, 	17272, 	18288, 
	19304, 	20320, 	21336, 	22352, 	23368, 	24384, 
	25400, 	26416, 	27432, 	28448, 	29464, 	30480, 
	31496, 	32512, 	33528, 	34544, 	35560, 	36576, 
	37592, 	38608, 	39624, 	40640, 	41656, 	42672, 
	43688, 	44704, 	45720, 	46736, 	47752, 	48768, 
	49784, 	50800, 	51816, 	52832, 	53848, 	54864, 
	55880, 	56896, 	57912, 	58928, 	59944, 	60960, 
	61976, 	62992, 	64008, 	65024, 
};


#define	_ADJUST_GAMMA_UNDER_LOW_LIGHT_

#ifdef _ADJUST_GAMMA_UNDER_LOW_LIGHT_
static unsigned short gamma_adjust_table[65];
static int gamma_adjust_stage;			// 0 过渡状态  1 S曲线  2 hs曲线 3 线性曲线 
static int do_gamma_adjust;
#endif

// gamma曲线修改在ISP"消隐期"(场完成中断)中处理, 避免在"非消隐期"修改造成的画面亮度明显不一致的情形.
//		即在某些场景下修改Gamma曲线, 画面的顶部与底部的亮度存在明显的差异.
//		改为ISP"消隐期"修改, 可避免上述画面的亮度差异.
void isp_gamma_adjust(void)
{
#ifdef _ADJUST_GAMMA_UNDER_LOW_LIGHT_
	// 检查gamma曲线是否需要修正
	if(do_gamma_adjust)
	{
		int i;
		// 写入新的曲线
		for (i = 0; i < 65; i++)
		{
		  unsigned int data0 = (0x04) | (i << 8) | (gamma_adjust_table[i]<<16);
		  Gem_write ((GEM_LUT_BASE+0x00), data0);
		}
		do_gamma_adjust = 0;
	}

#endif	
}

static void imx322_cmos_isp_colors_init (isp_colors_ptr_t p_colors)	// 色彩初始参数
{
  int i, j;
  int gamma_wdr[65];
  int k_L, k_H, pmax, t0, s0, t1, s1, t2, s2, k0, k2;
  int b, c, d, bb, cc, dd;
  int contrast_Ls, contrast_Hs;
  
  p_colors->colorm.enable = 0;// 色矩阵 
    
  p_colors->gamma.enable =  1;
  for (i = 0; i < 65; i++)
  {
		//p_colors->gamma.gamma_lut[i] = gamma_LUT0[i];
		p_colors->gamma.gamma_lut[i] = gamma_LUT1[i];
  } 
  
#ifdef _ADJUST_GAMMA_UNDER_LOW_LIGHT_
  gamma_adjust_stage = 1;	// s曲线, 对比度提升
  do_gamma_adjust = 0;
#endif  

#ifdef _HDTV_000255_
  // 20170118 地下车库对比测试, 16~235会丢失较多的细节, 而0~255显示效果较好
  // 使用0~255范围, 尽量保留所有的细节
  p_colors->rgb2ypbpr_type = HDTV_type_0255;
#else
  // 20170223 恢复为16235模式, 提高场景的对比度及通透度
  p_colors->rgb2ypbpr_type = HDTV_type_16235;
#endif
  
  isp_create_rgb2ycbcr_matrix (p_colors->rgb2ypbpr_type, &p_colors->rgb2yuv);


  // demosaic 参数
	p_colors->demosaic.mode = 0;
	p_colors->demosaic.coff_00_07 = 32;
	p_colors->demosaic.coff_20_27 = 255;	// 滤波
	p_colors->demosaic.horz_thread = 0;
	//p_colors->demosaic.demk = 128;
	p_colors->demosaic.demk = 512;		// 20161230 改善解析度(树叶,纹理)
	p_colors->demosaic.demDhv_ofst = 0;

  isp_colors_init_io(p_colors);	  
}

// 20170203 3D拖影现象是因为抓取RAW数据导致
// 20170120上午
//#define	N141_3D_1		1	//


//#define	N141_3D_2		1		// 降低低照度场景下的绿叶拖尾, 降低白天天空的鬼影

//#define	N141_3D_3		1

static const unsigned char noise0_0[17] = {
255, 255, 255, 255, 
255, 248, 240, 212, 
212, 212, 212, 212, 
212, 212, 212, 212,
212
};

static const unsigned char noise1_0[17] = {
255, 255, 255, 255, 
255, 248, 240, 212, 
192, 160, 144, 128, 
128, 128, 128, 128,
128
};

static void imx322_cmos_isp_denoise_init (isp_denoise_ptr_t p_denoise)	// 降噪初始设置
{
  int i, x0, y0, x1, y1, x2, y2, x3, y3;
  int a, b, c, d, e, f, delta;

  p_denoise->enable2d = 7;
  if(isp_get_work_mode() == ISP_WORK_MODE_NORMAL)
  {
#if ISP_3D_DENOISE_SUPPORT
  		p_denoise->enable3d = 7; 
#else
		p_denoise->enable3d = 0; 
#endif
  }
  else
  {
		p_denoise->enable3d = 0;   
  }
  
  //p_denoise->sensitiv0 = 4;    
  //p_denoise->sensitiv1 = 4;
  p_denoise->sensitiv0 = 3;    	// 降低降噪强度, 提升清晰度
  p_denoise->sensitiv1 = 3;
  
  p_denoise->sel_3d_table = 3;		// 3的降噪效果较好
  p_denoise->sel_3d_matrix = 1;

   
   
  p_denoise->y_thres0 = 6;    
  p_denoise->u_thres0 = 10;
  p_denoise->v_thres0 = 10;
  
  p_denoise->y_thres1 = 6;   
  p_denoise->u_thres1 = 10;
  p_denoise->v_thres1 = 10;  
  
  p_denoise->y_thres2 = 6; 
  p_denoise->u_thres2 = 11;  
  p_denoise->v_thres2 = 11; 
    
  for (i = 0; i <= 16; i ++)
  {
	  p_denoise->noise0[i] = noise0_0[i];
  }
  
  for (i = 0; i <= 16; i ++)
  {
	  p_denoise->noise1[i] = noise1_0[i];
  }
  
  
  isp_denoise_init_io (p_denoise);	
}


// 20170227 增加场景的解析度拉伸, 增加通透度, 增加低照度场景的解析度
static const unsigned char imx322_default_resolt_before_20171122[33] = {
 200,  210,  220,  225,  225,  225, 
 225,  225,  225,  225,  225,  225, 
 225,  225,  225,  225,  225,  230, 
 230,  230,  230,  230,  230,  230, 
 230,  230,  230,  230,  230,  225, 
 220,  215,  210,  	
};

static unsigned int imx322_default_resolt[33] = {
 200,  215,  220,  225,  230,  230, 
 235,  235,  235,  235,  235,  235, 
 235,  235,  235,  235,  235,  235, 
 235,  235,  235,  235,  235,  235, 
 235,  235,  235,  235,  235,  230, 
 230,  225,  220,  	
};


#define	ERIS_COLORT_1 1
#if ERIS_COLORT_1
static const unsigned int imx322_default_colort[33] = {
   64,     128,    192,    256,    320,    384,    511,    511, 
   511,    511,    511,    511,    511,    511,    511,    511, 
   511,    511,    511,    511,    511,    511,    511,    511, 
   511,    511,    511,    511,    448,    384,    256,    192, 
   128	
};
#else

/*
static unsigned int imx322_default_colort[33] = {
   1,    1,    1,    1,    1,    1,    1,    1, 
   16,    24,    32,    40,    64,   80,    96,    112, 
   128,    160,    212,    256,    256,    256,    256,    256, 
   256,    256,    256,    256,    256,    256,    256,    256, 
   128	
};*/

static const unsigned int imx322_default_colort_1[33] = {
 32,   38,   44,   50,   56,   62, 
  68,   74,   80,   86,   92,   98, 
 104,  110,  116,  122,  128,  152, 
 176,  200,  224,  248,  272,  296, 
 320,  343,  367,  391,  415,  439, 
 463,  487,  511, 
};

static const unsigned int imx322_default_colort[33] = {
  8,   8,   8,   8,   8,   8, 
  16, 16,  16,  16,  16,  16, 
 32,  32,  32,  32,  32,  48, 
 48,  48,  48,  64,  64,  96, 
 96,  64,  64,  48,  48,  48, 
 32,  32,  32, 
};
#endif

static void imx322_cmos_isp_eris_init(isp_eris_ptr_t p_eris)			// 宽动态初始设置
{
  int i, j, x0, y0, x1, y1, x2, y2;
  int a, b, c, d;

  p_eris->enable = 1;
  p_eris->manual = 0;
  p_eris->target = 128;
  p_eris->black = 0;
  // 10bit使用D2~D11, D0~D1固定为0
  if(isp_get_sensor_bit() == ARKN141_ISP_SENSOR_BIT_12)
  	  p_eris->white = 4095;		// 尽可能保留动态范围
  else
	  p_eris->white = 1023;		// 尽可能保留动态范围
  p_eris->gain_max = 256;	//256;
  //p_eris->gain_max = 128;	//256;
  p_eris->gain_min = 64;	//256;
  p_eris->gain_man = 256;
  p_eris->cont_max = 256;	// 2*64;
  p_eris->cont_min = 64;	//	6*64;
  p_eris->cont_man = 16;
  p_eris->dfsEris = 1;
  p_eris->varEris = 0;
  p_eris->resols = 0;
  p_eris->resoli = 0; 
  p_eris->spacev = 0;
  
  
#if 1
  for (i = 0; i < 33; i++)
  {
	  p_eris->resolt[i] = imx322_default_resolt[i];
  }
#else
  {
  	x0 = 0;   y0 = 128;//153;//0;
  	x1 = 4;   y1 = 240;//217;//204;
  	x2 = 32;  y2 = 240;//230;//218;
  	
    b = (x1*y0-y1*x0)/(-x0+x1);
    a = (y1-y0)/(-x0+x1);
    d = (-x2*y1+y2*x1)/(x1-x2);
    c = (-y2+y1)/(x1-x2);
    
    for (i = 0; i < 33; i++)
    {
    	j = (a*i+b)*(i<x1) + (c*i+d)*((i>=x1)&&(i<x2)) + y2*(i>=x2); 	
    	p_eris->resolt[i] = (j>255) ? 255 : j;
    	//g_resolt[i] = p_eris->resolt[i];
    }
  	
  }  
#endif
  
#if 1
    for (i = 0; i < 33; i++)
    {
		// p_eris->colort[i] = 512; 
    	 p_eris->colort[i] = imx322_default_colort[i];   	
    }
  
#else
  {
	  // 3点折线段
	  // (0, 96) (4,511) (32, 511)
    x0 = 0;   y0 = 96;                                                 
    x1 = 4;   y1 = 255;
    x2 = 32;  y2 = 255;
	 
	  // (0, 0) (16,256) (32, 0)
//    x0 = 0;  y0 = 0;                                                 
//    x1 = 16; y1 = 256;
//    x2 = 32; y2 = 0;
	 
    
    b = (x1*y0-y1*x0)/(-x0+x1);
    a = (y1-y0)/(-x0+x1);
    d = (-x2*y1+y2*x1)/(x1-x2);
    c = (-y2+y1)/(x1-x2);
    
    for (i = 0; i < 33; i++)
    {
    	j = y1*(i==16) + (a*i+b)*(i<16) + (c*i+d)*(i>16);
    	p_eris->colort[i] = (j>511) ? 511 : j;   	
		//p_eris->colort[i] = 0;
    }
  }
#endif
  
	// ERIS直方图初始设置 
	// u8_t hist_thresh[4] = {0x10, 0x40, 0x80, 0xc0};
  	p_eris->eris_hist_thresh[0] = 0x10;
	p_eris->eris_hist_thresh[1] = 0x40;
	p_eris->eris_hist_thresh[2] = 0x80;
	p_eris->eris_hist_thresh[3] = 0xC0;
  
	
  isp_eris_init_io(p_eris);	
}

#if 1
const unsigned short lenscoeff[]=
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
  4096,4315,4322,4317,4331,     4339,4355,4361,4406,4443,
  4467,4466,4485,4519,4552,     4591,4648,4702,4771,4847,
  4923,5001,5125,5242,5373,     5518,5694,5885,6047,6230,
  6416,6597,6821,7055,7305,     7481,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,     7799,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,     7799,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,
  // b 
   4096,4315,4322,4317,4331,     4339,4355,4361,4406,4443,
  4467,4466,4485,4519,4552,     4591,4648,4702,4771,4847,
  4923,5001,5125,5242,5373,     5518,5694,5885,6047,6230,
  6416,6597,6821,7055,7305,     7481,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,     7799,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,     7799,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,
};
#else
const unsigned short lenscoeff[]=
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
  4096,4320,4335,4340,4351,     4368,4406,4442,4459,4467,
  4480,4510,4555,4594,4642,     4692,4750,4812,4883,4954,
  5109,5281,5463,5681,5900,     6172,6449,6789,7057,7387,
  7728,8106,8602,9000,9362,     9471,9826,10479,10479,10479,
  10479,10479,10479,10479,10479,10479,10479,10479,10479,10479,
  10479,10479,10479,10479,10479,10479,10479,10479,10479,10479,
  10479,10479,10479,10479,10479,
  // b
  4096,4320,4335,4340,4351,     4368,4406,4442,4459,4467,
  4480,4510,4555,4594,4642,     4692,4750,4812,4883,4954,
  5109,5281,5463,5681,5900,     6172,6449,6789,7057,7387,
  7728,8106,8602,9000,9362,     9471,9826,10479,10479,10479,
  10479,10479,10479,10479,10479,10479,10479,10479,10479,10479,
  10479,10479,10479,10479,10479,10479,10479,10479,10479,10479,
  10479,10479,10479,10479,10479,
};	
#endif


#define  Ycenter_x   960 
#define  Ycenter_y   540
#define  CenterRx   Ycenter_x
#define  CenterRy   Ycenter_y
#define  CenterGx   Ycenter_x
#define  CenterGy   Ycenter_y
#define  CenterBx   Ycenter_x
#define  CenterBy   Ycenter_y

static void imx322_cmos_isp_fesp_init(isp_fesp_ptr_t p_fesp)	// 镜头校正, fix-pattern-correction, 坏点去除初始设置
{
	p_fesp->Lensshade.enable = 0;
	
  int i, j, k;
  int x0, y0, x1, y1, x2, y2, x3, y3;
  int a, b, c, d, e, f, delta;
 // unsigned short R_lenslut[65];
//  unsigned short G_lenslut[65];
//  unsigned short B_lenslut[65];
//  unsigned short Y_lenslut[65];
  
  p_fesp->Lensshade.enable = 0;
  p_fesp->Lensshade.scale = 1;
  p_fesp->Lensshade.lscofst = 50;
  p_fesp->Lensshade.rcenterRx = CenterRx;
  p_fesp->Lensshade.rcenterRy = CenterRy;
  p_fesp->Lensshade.rcenterGx = CenterRx;
  p_fesp->Lensshade.rcenterGy = CenterRy;
  p_fesp->Lensshade.rcenterBx = CenterRx;
  p_fesp->Lensshade.rcenterBy = CenterRy;
  
  for( i=0 ; i < 195 ;i++ )
  {
    p_fesp->Lensshade.coef[i] = lenscoeff[i];
  }

  /*
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
  }*/
  
  
  // fix pattern correction
	
  p_fesp->Fixpatt.enable = 1;
  p_fesp->Fixpatt.mode = 0;
  
  if( isp_get_sensor_bit () == ARKN141_ISP_SENSOR_BIT_12)
  {
  		p_fesp->Fixpatt.rBlacklevel = 240;
  		p_fesp->Fixpatt.grBlacklevel = 240;
  		p_fesp->Fixpatt.gbBlacklevel = 240;
  		p_fesp->Fixpatt.bBlacklevel = 240;
  }
  else
  {
  		p_fesp->Fixpatt.rBlacklevel = 60;
  		p_fesp->Fixpatt.grBlacklevel = 60;
  		p_fesp->Fixpatt.gbBlacklevel = 60;
  		p_fesp->Fixpatt.bBlacklevel = 60;	  
  }
  p_fesp->Fixpatt.profile[0] = 255;
  p_fesp->Fixpatt.profile[1] = 255;
  p_fesp->Fixpatt.profile[2] = 255;
  p_fesp->Fixpatt.profile[3] = 255;
  p_fesp->Fixpatt.profile[4] = 255;
  p_fesp->Fixpatt.profile[5] = 255;
  p_fesp->Fixpatt.profile[6] = 255;
  p_fesp->Fixpatt.profile[7] = 255;
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
  p_fesp->Badpix.mode = 0; 
  p_fesp->Badpix.thresh = 19;		// IMX322 实测最低坏点判断阈值
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
  // cross talk的阈值越大, 图像越模糊. 8是一个较合适的值. 
  // 使用3D降噪来去除噪声
  p_fesp->Crosstalk.enable = 1;
  p_fesp->Crosstalk.mode = 1;
  p_fesp->Crosstalk.thresh = 10;
  p_fesp->Crosstalk.snsCgf = 3;		//		值越大, 滤除奇异点的能力越大.
  p_fesp->Crosstalk.thres0cgf = 10;
  p_fesp->Crosstalk.thres1cgf = 10;
  p_fesp->Crosstalk.profile[0] = 255;
  p_fesp->Crosstalk.profile[1] = 255;
  p_fesp->Crosstalk.profile[2] = 255;
  p_fesp->Crosstalk.profile[3] = 243;
  p_fesp->Crosstalk.profile[4] = 243;
  p_fesp->Crosstalk.profile[5] = 243;
  p_fesp->Crosstalk.profile[6] = 243;
  p_fesp->Crosstalk.profile[7] = 243;
  p_fesp->Crosstalk.profile[8] = 232;
  p_fesp->Crosstalk.profile[9] = 232;
  p_fesp->Crosstalk.profile[10] = 232;
  p_fesp->Crosstalk.profile[11] = 232;
  p_fesp->Crosstalk.profile[12] = 232;
  p_fesp->Crosstalk.profile[13] = 212;
  p_fesp->Crosstalk.profile[14] = 212;
  p_fesp->Crosstalk.profile[15] = 212;
  p_fesp->Crosstalk.profile[16] = 212;
  
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

#define	SATUATION_OFFSET		64		// 饱和度补偿

static void imx322_cmos_isp_enhance_init (isp_enhance_ptr_t p_enhance)	// 图像增强初始设置
{
  p_enhance->sharp.enable = 1;
  p_enhance->sharp.mode = 0;
  p_enhance->sharp.coring = 0;// 0-7 
  //p_enhance->sharp.strength = 64;//64;//32;//128; 
  //p_enhance->sharp.strength = 32;
  //p_enhance->sharp.strength = 255;
  p_enhance->sharp.strength = 196;
  //p_enhance->sharp.gainmax = 256;
  //p_enhance->sharp.gainmax = 128;		// 20170223 修改为轻微的锐化
  //p_enhance->sharp.gainmax = 144;		// 20170305 微调, 增加一点, 
  //p_enhance->sharp.gainmax = 160;		// 20170803白天路测的视频车牌的识别比第一现场0330稍差, 增加锐化程度改善识别度
  p_enhance->sharp.gainmax = 255;		// 20170805根据20170804路测结果,车牌辨识度较0330稍差, 
													//   通过分析视频, 第一现场(0330)的锐化度较高, 提升锐化增益至256 
  p_enhance->bcst.enable = 1;
  //p_enhance->bcst.bright = -24; // -256~255
  p_enhance->bcst.bright = 0; // -256~255
  p_enhance->bcst.contrast = 1024;//1024; // 0~1.xxx
  p_enhance->bcst.satuation = 1024 + SATUATION_OFFSET; //0~1.xxx
  // p_enhance->bcst.hue = 0; // -128~127
  p_enhance->bcst.hue = 0;
  p_enhance->bcst.offset0 = 0; // 0~255    
  p_enhance->bcst.offset1 = 128; // 0~255  
  //p_enhance->bcst.offset1 = 116; // 0~255  
  
  isp_enhance_init_io (p_enhance);	
}

static void imx322_cmos_isp_ae_init (isp_ae_ptr_t p_ae)		// 自动曝光初始设置
{

}

static void imx322_cmos_isp_sys_init (isp_sys_ptr_t p_sys, isp_param_ptr_t p_isp)		// 系统初始设置 (sensor pixel位数, bayer mode)
{
	p_sys->ispenbale = 1;  
	p_sys->ckpolar = 0;    
	p_sys->vcpolar = 1;    
	p_sys->hcpolar = 1;     
	p_sys->vmskenable = 0;	// 自动丢帧. 保留,必须设置为0
	p_sys->frameratei = 0; 
	p_sys->framerateo = 0; 	// 保留,必须设置为0
	
#if 0
	p_sys->frameratei = 30;
	p_sys->framerateo = 0; 	// 保留,必须设置为0
	p_sys->vifrasel0 = 0xAAAAAAAA;
	p_sys->vifrasel1 = 0;
	
#else
	p_sys->frameratei = 0;
	p_sys->framerateo = 0; 
	p_sys->vifrasel0 = 0;
	p_sys->vifrasel1 = 0;
#endif
	
	// IN/OUT (60帧/55帧, ), 
#if 0
	p_sys->frameratei = 64;
	p_sys->framerateo = 64; 
	p_sys->vifrasel0 = 0xFFFFFFFF;	// 29
	// p_sys->vifrasel1 = 0x07FFEFFE;	// 25
   p_sys->vifrasel1 = 0xFFFFFFFF;	// 22
#endif
	// IN/OUT (1帧/1帧)
	//  p_sys->frameratei = 0;
	//  p_sys->vifrasel0 = 0x00000000;
	// p_sys->vifrasel1 = 0x00000000;
	
	
	//(0.表示8位 1:表示10位 2:表示12位  3:表示14位)
	//XM_printf("sensor bit: 0:8bit 1:10bit 2:12bit 3:14bit  \n");
	
  	if(isp_get_sensor_bit () == ARKN141_ISP_SENSOR_BIT_12)
		p_sys->sensorbit = ARKN141_ISP_SENSOR_BIT_12;  
	else
		p_sys->sensorbit = ARKN141_ISP_SENSOR_BIT_10;
	
	// 0:RGGB 1:GRBG 2:BGGR 3:GBRG 
	//XM_printf("bayer mode: 0:RGGB 1:GRBG 2:BGGR 3:GBRG  ov9712=2  pp1210 720P=1 1080P=0 \n");
	
	if(IMAGE_H_SZ == 1920 )	// 1080P
		p_sys->bayermode = ARKN141_ISP_RAW_IMAGE_BAYER_MODE_RGGB;
	else if(IMAGE_H_SZ == 1280 )	// 720P
	{
		// 10bit
		// 根据 zonestridey 的设置值选择 GBRG or RGGB
		//p_sys->bayermode = ARKN141_ISP_RAW_IMAGE_BAYER_MODE_GBRG;
		p_sys->bayermode = ARKN141_ISP_RAW_IMAGE_BAYER_MODE_RGGB;
	}
	else
		p_sys->bayermode = ARKN141_ISP_RAW_IMAGE_BAYER_MODE_RGGB;
	
	p_sys->imagewidth = p_isp->image_width;
	p_sys->imageheight = p_isp->image_height;
	
	// imagehblank, zonestridex, zonestridey 是以ISP Core的时钟为计数基准, 
	// 计算时首先按照Sensor Pixel Clock时序进行计算, 然后换算到ISP Core Clock,
	
	// FPGA测试时 ISP Core Clock == Sensor Pixel Clock
	double ISP_CORE_CLOCK = arkn141_get_clks (ARKN141_CLK_ISP);
	double SENSOR_PIXEL_CLOCK = arkn141_get_clks (ARKN141_CLK_SENSOR_MCLK) * 2;
	// double ratio = 1;//((double)ISP_CORE_CLOCK) / SENSOR_PIXEL_CLOCK;
	double ratio = ISP_CORE_CLOCK/SENSOR_PIXEL_CLOCK;
#if IMX323_DCK_SYNC_MODE_ENABLE
	p_sys->sonyif = 0;
#else  
	p_sys->sonyif = 1;
#endif
	if(p_sys->imagewidth == 1920 && p_sys->imageheight == 1080)
	{
		p_sys->imagehblank = (unsigned int)(1 * 96);		// ISP Core Clock
		// 48 = 16(OB side ignored area) + 24(Ignored area of effective pixel side) + 8(Effective margin for color processing)
		p_sys->zonestridex = (unsigned int)(1 * 48);			
		// 31 	= 6(Dummy for communication) + 1(Frame information line) + 4(OB side ignored area) 
		//		+ 8(Vertical direction effective OB) + 4(Ignored area of effective pixel side) + 8(Effective margin for color processing)
		// p_sys->zonestridey = (unsigned int)(1 * (95-58));
		p_sys->zonestridey = (unsigned int)(1 * 31);
		//p_sys->zonestridey = (unsigned int)(1 * (95-58));
		//  p_sys->zonestridey = (unsigned int)(ratio * (25));		// 忽略 8(Effective margin for color processing)
		
		// imagehblank = ISP消隐周期 = sensor消隐点个数(sensor行像素点个数 - 行有效点个数) * ISP_CORE_CLOCK / SENSOR_PIXEL_CLOCK - 20(ISP逻辑占用周期)
		// ISP处理时间延长
		//p_sys->c = (unsigned int)(1 * 200);		// ISP Core Clock
		// p_sys->imagehblank = (unsigned int)(1 * 160);		// ISP Core Clock
		//p_sys->imagehblank = (unsigned int)(1 * 240);	
		double blank = (1125 * 2 - 48 - 1920) * ratio - 30;
		// p_sys->imagehblank = (unsigned int)(1 * 280);	// ISP 110MHz
		//p_sys->imagehblank = (unsigned int)(1 * 240);	// ISP 110MHz
		//p_sys->imagehblank = (unsigned int)(1 * 240);	
		p_sys->imagehblank = (unsigned int)(1 * 96);	
		if(p_sys->imagehblank > (UINT32)blank)
			p_sys->imagehblank = (UINT32)blank;
		//XM_printf ("max_imagehblank = %d, imageblank = %d\n", (UINT32)blank, p_sys->imagehblank);
		
		p_sys->resizebit	= 0;
	}
	else
	{
		p_sys->imagehblank = 96;
		// 48 = 16(OB side ignored area) + 24(Ignored area of effective pixel side) + 8(Effective margin for color processing)
		p_sys->zonestridex = 48;
		// 23 	= 6(Dummy for communication) + 1(Frame information line) + 4(OB side ignored area) 
		//		+ 6(Vertical direction effective OB) + 2(Ignored area of effective pixel side) + 4(Effective margin for color processing)
		p_sys->zonestridey = 23;
		
		p_sys->resizebit	= 0;	// 裁剪低2位
	}
	
	if(p_sys->imagewidth == 1920 && p_sys->imageheight == 1080 && isp_get_sensor_bit () == ARKN141_ISP_SENSOR_BIT_12)
	{
		p_sys->sonysac1 = 0xfff;
		p_sys->sonysac2 = 0x000;
		p_sys->sonysac3 = 0x000;
		p_sys->sonysac4 = 0x800; 
	}
	else if(p_sys->imagewidth == 1920 && p_sys->imageheight == 1080 && isp_get_sensor_bit () == ARKN141_ISP_SENSOR_BIT_10)
	{
		p_sys->sonysac1 = 0xfff;
		p_sys->sonysac2 = 0x000;
		p_sys->sonysac3 = 0x000;
		p_sys->sonysac4 = 0x800; 
		
		p_sys->resizebit	= 2;	// 10bit, 从D2~D11输出. sensor 左对齐12bit输出, 裁剪D0~D1
	}
	else if(p_sys->imagewidth == 1280 && p_sys->imageheight == 720 && isp_get_sensor_bit () == ARKN141_ISP_SENSOR_BIT_12)
	{
		p_sys->sonysac1 = 0xfff;
		p_sys->sonysac2 = 0x000;
		p_sys->sonysac3 = 0x000;
		p_sys->sonysac4 = 0x800; 	  
		
		p_sys->resizebit	= 0;	// 12bit
	}
	else
	{
		// 720p 10bit 30帧/60帧模式, 选择左对齐输出, D2~D11, 将D0~D1裁剪
		p_sys->sonysac1 = 0x3ff;
		p_sys->sonysac2 = 0x000;
		p_sys->sonysac3 = 0x000;
		p_sys->sonysac4 = 0x200;
		
		//p_sys->resizebit	= 0;	// 不裁剪, sensor 右对齐10bit输出
		p_sys->resizebit	= 2;	// 裁剪低2位, sensor 左对齐12bit输出
	}
	
	// 使能 
	p_sys->vmanSftenable = 0; 
	p_sys->vchkIntenable = 1;// 帧开始
	p_sys->pabtIntenable = 1; //点异常
	p_sys->fendIntenable = 1; // 帧完成
	p_sys->fabtIntenable = 1; // 地址异常
	p_sys->babtIntenable = 1; //总线异常
	p_sys->ffiqIntenable = 0;  //快中断
	p_sys->pendIntenable = 1;  //中止 ，如果一帧未完成，会完成该帧
	
	p_sys->infoIntenable = 1;	// 使能ISP的曝光统计完成中断
	
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
	if(isp_get_work_mode() == ISP_WORK_MODE_NORMAL)
	{
		p_sys->debugmode  = 0;
		p_sys->testenable = 0; // 开启dram测试模式  
		p_sys->rawmenable = 0; // 1 允许RAW写出
		p_sys->yuvenable  = 1; // 0:关掉数据输出  1:打开
#if ISP_3D_DENOISE_SUPPORT
		p_sys->refenable  = 1; // 1;3D 参考帧开启 0:关闭 
#else
		p_sys->refenable  = 0; // 1;3D 参考帧开启 0:关闭 
#endif
	}
	else if(isp_get_work_mode() == ISP_WORK_MODE_RAW)
	{
		// RAW写出会占用3D参考帧通道
		p_sys->debugmode  = 1;
		p_sys->testenable = 0; // 开启dram测试模式  
		p_sys->rawmenable = 1; // 1 允许RAW写出
		p_sys->yuvenable  = 1; // 0:关掉数据输出  1:打开
		p_sys->refenable  = 0; // 1;3D 参考帧开启 0:关闭 
	}
	else if(isp_get_work_mode() == ISP_WORK_MODE_AUTOTEST)
	{
		p_sys->debugmode  = 1;
		p_sys->testenable = 1; // 开启dram测试模式  
		p_sys->rawmenable = 0; // 1 允许RAW写出
		p_sys->yuvenable  = 1; // 0:关掉数据输出  1:打开
		p_sys->refenable  = 1; // 1;3D 参考帧开启 0:关闭 
									  // 使能参考帧 (DRAM测试模式使用REFBUF指向的数据为RAW数据)
	}
	else
	{
		p_sys->debugmode  = 0;
		p_sys->testenable = 0; // 开启dram测试模式  
		p_sys->rawmenable = 0; // 1 允许RAW写出
		p_sys->yuvenable  = 1; // 0:关掉数据输出  1:打开
#if ISP_3D_DENOISE_SUPPORT
		p_sys->refenable  = 1; // 1;3D 参考帧开启 0:关闭 	
#else
		p_sys->refenable  = 0; // 1;3D 参考帧开启 0:关闭
#endif
	}
/*	
#if 1
	
	p_sys->debugmode  = 0;
	p_sys->testenable = 0; //开启dram测试模式  
	p_sys->rawmenable = 0; 
	p_sys->yuvenable  = 1;//0:关掉数据输出  1:打开
	p_sys->refenable  = 1;//1;  
#else // catch raw
	p_sys->debugmode  = 0;
	p_sys->testenable = 0; //开启dram测试模式  
	p_sys->rawmenable = 0; 
	p_sys->yuvenable  = 1;//0:关掉数据输出  
	p_sys->refenable  = 1;     
#endif
	*/
	p_sys->yuvformat = isp_get_video_format ();  //0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 
   //XM_printf("0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 \n");
#if _XM_PROJ_ == _XM_PROJ_1_SENSOR_1080P
#if DDR3
	p_sys->dmalock = 0;
#else
	//p_sys->dmalock = 2;   //总线锁使能 2:使能  其他数值为关闭
	p_sys->dmalock = 0;   //总线锁使能 2:使能  其他数值为关闭
#endif
#else
   p_sys->dmalock = 0;    	//	总线锁使能 2:使能  其他数值为关闭
#endif
	//		总线锁禁止会减少H264的编码时间
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
extern cmos_exposure_t isp_exposure;


typedef struct _isp_awb_polyline_tbl {
	int  inttime;

	int  black;
	int  jitter;
} isp_awb_polyline_tbl;

static isp_awb_polyline_tbl awb_polyline_tbl[] = {
	{     1,	  16,  	10		},
	{		5,		8,		13		},
	{	 800,		8,		13		},
	{  1125,   16,		13		}
};

static void awb_match_inttime (int inttime, isp_awb_polyline_tbl *awb_tbl)
{
	int i;
	int val;
	isp_awb_polyline_tbl *lo, *hi;
	int count = sizeof(awb_polyline_tbl)/sizeof(awb_polyline_tbl[0]);
	for (i = 0; i < count; i ++)
	{
		if(inttime <= awb_polyline_tbl[i].inttime)
			break;
	}
	
	// 匹配
	if(inttime == awb_polyline_tbl[i].inttime)
	{
		memcpy (awb_tbl, &awb_polyline_tbl[i], sizeof(isp_awb_polyline_tbl));
		return;
	}
	
	// 边界
	else if(inttime < awb_polyline_tbl[0].inttime)
	{
		memcpy (awb_tbl, &awb_polyline_tbl[0], sizeof(isp_awb_polyline_tbl));
		return;
	}
	else if(i == count)
	{
		memcpy (awb_tbl, &awb_polyline_tbl[count - 1], sizeof(isp_awb_polyline_tbl));
		return;
	}
	
	lo = &awb_polyline_tbl[i - 1];
	hi = &awb_polyline_tbl[i];
	val = (lo->black + (hi->black - lo->black) * (inttime - lo->inttime) / (hi->inttime - lo->inttime));
	if(val < 4)
		val = 4;
	else if(val > 48)
		val = 48;
	awb_tbl->black = val;
	
	val = (lo->jitter + (hi->jitter - lo->jitter) * (inttime - lo->inttime) / (hi->inttime - lo->inttime));
	if(val < 4)
		val = 4;
	else if(val > 48)
		val = 48;
	awb_tbl->jitter = val;
}

static void imx322_cmos_isp_awb_run (isp_awb_ptr_t p_awb)
{
	return;
#if 0
	//isp_awb_info_read (p_awb);	
	unsigned int data0;
	unsigned int inttime;
	isp_awb_polyline_tbl awb_tbl;
	inttime = isp_exposure.cmos_inttime.exposure_ashort;
	// 
	awb_match_inttime ((int)inttime, &awb_tbl);
	p_awb->black = (unsigned char)awb_tbl.black;
	//p_awb->jitter = (unsigned char)awb_tbl.jitter;
	/*
	data0 	= ((p_awb->enable  & 0x01) <<  0) 
				| ((p_awb->mode    & 0x03) <<  1) 	// bit1-bit2     mode 
				// 0: unite gray white average 
				//	1: unite color temperature average  
				//	2: zone color temperature Weight
				| ((p_awb->manual  & 0x01) <<  3) 	// bit3  manual, 0: auto awb  1: mannual awb
				| ((p_awb->black   & 0xFF) <<  8) 
				| ((p_awb->white   & 0xFF) << 16)
				;
	Gem_write ((GEM_AWB0_BASE+0x00), data0);
	*/
#endif
}

// 20170216 Demk对噪声的影响很大
typedef struct _isp_demosaic_polyline_tbl {
	int	inttime_gain;
	int	demk;		// demk影响解析度, 值越大, 解析度越高, 噪声也越大.
						// 低照度时, 降低demk可减少噪声
} isp_demosaic_polyline_tbl;

static const isp_demosaic_polyline_tbl demosaic_polyline_tbl[] = {
	//  gain    		demk
#if ISP_3D_DENOISE_SUPPORT
	
	// 20170215 将解析度增强因子适当降低, 适当减轻细节, 降低H264编码的码流
#if 1		// 20170215

	{	  1,           196 - 16  },
	{	  64,          384 - 64  },
	{	  100,         512 - 64  },
	{	  600,         512 - 64  },
	{	  1*CMOS_STD_INTTIME,	  	384 - 64  },		
	{	  5*CMOS_STD_INTTIME,	   72	 },		

#ifdef _DISABLE_GAMMA_UNDER_LOW_LIGHT_
	// 低照度关闭gamma时, 此时可适当增加demosaic因子, 增加低照度下场景的解析度.
	{	  10*CMOS_STD_INTTIME,	   64	 },		// 最低保持为64
	
#else
	{	  10*CMOS_STD_INTTIME,	   48	 },		// 仅开启模拟增益时, 此时使能较大的demk值, 保持较好的解析度, 同时噪声较低
	{	  15*CMOS_STD_INTTIME,	   22	 },		// 20170217 晚上测试, 噪声的幅度还是有点偏大, 降低到22
//	{	  15*CMOS_STD_INTTIME,	   24	 },		// 20170216 设置为24,可以较好的消除抖动噪声
//	{	  15*CMOS_STD_INTTIME,	   26	 },		//	20170217 设置为26,稍微提升解析度, 提高车库中文字的辨析能力		

	{    20*CMOS_STD_INTTIME,     22    },		//	20170217 晚上测试, 噪声的幅度还是有点偏大, 降低到22.
//	{    20*CMOS_STD_INTTIME,     24    },		// 参考 f:\路测视频\20170114晚上\RAW\20170114\212128\212128_21212841_ISP_DEMK_064.PNG
												//		兼顾解析度与噪声
//	{    20*CMOS_STD_INTTIME,     26    },		//	20170217 设置为26,稍微提升解析度, 提高车库中文字的辨析能力	
#endif							
	
#else
	{	  1,           196   },
	{	  64,          384   },
	{	  100,         512   },
	{	  600,         512   },
	{		1*CMOS_STD_INTTIME,	  	384	},		// 仅开启模拟增益时, 此时使能较大的demk值, 保持较好的解析度, 同时噪声较低
	{	  10*CMOS_STD_INTTIME,	   196	},		// 仅开启模拟增益时, 此时使能较大的demk值, 保持较好的解析度, 同时噪声较低
	{    20*CMOS_STD_INTTIME,     48    },		// 参考 f:\路测视频\20170114晚上\RAW\20170114\212128\212128_21212841_ISP_DEMK_064.PNG
									//		兼顾解析度与噪声
#endif

#else	
	// 3D 关闭
	{	  1,           128 - 8   },
	{	  64,          256 - 64  },
	{	  100,         512 - 256 },
	{	  600,         512 - 256 - 48},
	{		1*CMOS_STD_INTTIME,	  	256 - 112 },		// 仅开启模拟增益时, 此时使能较大的demk值, 保持较好的解析度, 同时噪声较低
	{	  8*CMOS_STD_INTTIME,	   80	},		// 仅开启模拟增益时, 此时使能较大的demk值, 保持较好的解析度, 同时噪声较低
	{    20*CMOS_STD_INTTIME,     32    },		// 参考 f:\路测视频\20170114晚上\RAW\20170114\212128\212128_21212841_ISP_DEMK_064.PNG
									//		兼顾解析度与噪声
	{    35*CMOS_STD_INTTIME,     16    },		// 较大数字增益
	{    64*CMOS_STD_INTTIME,     4    },		// 较大数字增益
	{    128*CMOS_STD_INTTIME,    2    },		// 较大数字增益
#endif
};

static void demosaic_match (int inttime_gain, isp_demosaic_polyline_tbl *demosaic_tbl)
{
	int i;
	int demk;
	const isp_demosaic_polyline_tbl *lo, *hi;
	int count = sizeof(demosaic_polyline_tbl)/sizeof(demosaic_polyline_tbl[0]);
	for (i = 0; i < count; i ++)
	{
		if(inttime_gain <= demosaic_polyline_tbl[i].inttime_gain)
			break;
	}
	// 匹配
	if(inttime_gain == demosaic_polyline_tbl[i].inttime_gain)
	{
		demk = demosaic_polyline_tbl[i].demk;
	}
	// 边界
	else if(inttime_gain < demosaic_polyline_tbl[0].inttime_gain)
	{
		demk =  demosaic_polyline_tbl[0].demk;
	}
	else if(i == count)
	{
		demk = demosaic_polyline_tbl[count - 1].demk;
	}
	else
	{
		lo = &demosaic_polyline_tbl[i - 1];
		hi = &demosaic_polyline_tbl[i];
		demk = (int)(lo->demk + (hi->demk - lo->demk) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	}
	
	if(demk > 512)
		demk = 512;
	else if(demk < 4)
		demk = 4;
	
	demosaic_tbl->demk = demk;
}



static void imx322_cmos_isp_colors_run (isp_colors_ptr_t p_colors, isp_awb_ptr_t p_awb, isp_ae_ptr_t p_ae)
{
	// 配置demosaic, demk影响图像的解析度
	isp_demosaic_polyline_tbl demosaic_tbl;
	int demk;
	unsigned int data0;
	int inttime_again = cmos_calc_inttime_gain (&isp_exposure);
	
	demosaic_match (inttime_again, &demosaic_tbl);
	demk = demosaic_tbl.demk;
	p_colors->demosaic.demk = demk;	
	
	// 新版本ISP增加demosaic功能, 支持2种demosaic算法
	//  0 ~  7  清晰度调整主要修改低8位的值, 最大值64
	// 20 ~ 27  一般固定为16
	// 31       固定为0值
	data0 = ((p_colors->demosaic.mode & 1) << 31)
			| ((p_colors->demosaic.coff_00_07 & 0xFF) << 0)
			| ((p_colors->demosaic.coff_20_27 & 0xFF) << 20)
			| ((p_colors->demosaic.demk & 0xFFF) << 8)		// demk	bit19-8, 12bit
			;
	Gem_write ((GEM_DEMOSAIC_BASE+0x00), data0);
	
	
#if 0//def _DISABLE_GAMMA_UNDER_LOW_LIGHT_
	// 20170226 新加功能, 需要评估
	// Gamma调整
	// 亮度较亮的场景使用S曲线进行对比度增强
	// 亮度较暗的场景关闭GAMMA拉伸, 保留暗场景的细节
	// 为防止频繁开启/关闭Gamma导致画面变化明显, gamma的开启关闭具有迟滞效应
	// 关闭Gamma时RGB-->YUV切换到016235模式, 提升场景对比度
	// 开启Gamma时RGB-->YUV恢复到000255模式, 提升场景动态范围
	int do_gamma = 0;
	if(inttime_again >= 3 * CMOS_STD_INTTIME)
	{
		p_colors->gamma.enable = 0;
		do_gamma = 1;
		//p_colors->rgb2ypbpr_type = HDTV_type_16235;
	}
	else if(inttime_again <=  CMOS_STD_INTTIME)
	{
		p_colors->gamma.enable = 1;
		do_gamma = 1;
		//p_colors->rgb2ypbpr_type = HDTV_type_0255;
	}
	
	if(do_gamma)
	{
		//unsigned int data0,data1,data2,data3,data4,data5; 
		//isp_create_rgb2ycbcr_matrix (p_colors->rgb2ypbpr_type, &p_colors->rgb2yuv);
		
  	 	data0 = p_colors->gamma.enable;
   	Gem_write ((GEM_COLORS_BASE+0x18), data0);

#if 0
		data0 = (p_colors->rgb2yuv.rgbcoeff[0][0]) | (p_colors->rgb2yuv.rgbcoeff[0][1]<<16); 
		data1 = (p_colors->rgb2yuv.rgbcoeff[0][2]) | (p_colors->rgb2yuv.rgbcoeff[1][0]<<16);
		data2 = (p_colors->rgb2yuv.rgbcoeff[1][1]) | (p_colors->rgb2yuv.rgbcoeff[1][2]<<16);
		data3 = (p_colors->rgb2yuv.rgbcoeff[2][0]) | (p_colors->rgb2yuv.rgbcoeff[2][1]<<16);
		data4 = (p_colors->rgb2yuv.rgbcoeff[2][2]) | (p_colors->rgb2yuv.rgbcoeff[3][0]<<16);
		data5 = (p_colors->rgb2yuv.rgbcoeff[3][1]) | (p_colors->rgb2yuv.rgbcoeff[3][2]<<16);
		Gem_write ((GEM_COLORS_BASE+0x20), data0); 
		Gem_write ((GEM_COLORS_BASE+0x24), data1);
		Gem_write ((GEM_COLORS_BASE+0x28), data2);
		Gem_write ((GEM_COLORS_BASE+0x2c), data3);
		Gem_write ((GEM_COLORS_BASE+0x30), data4);
		Gem_write ((GEM_COLORS_BASE+0x34), data5);
#endif
	}
	
	
#endif
	
#ifdef _ADJUST_GAMMA_UNDER_LOW_LIGHT_
#if 1
	isp_gamma_polyline_tbl gamma_tbl;
	if(do_gamma_adjust == 1)		// 等待上一次gamma修正写入到寄存器
		return;
	//memset (&gamma_tbl, 0, sizeof(gamma_tbl));
	isp_ae_gamma_match (inttime_again, &gamma_tbl);
	for (int i = 0; i < 65; i ++)
	{
		gamma_adjust_table[i] = gamma_tbl.gamma_lut[i];
		p_colors->gamma.gamma_lut[i] = gamma_adjust_table[i];
	}
	
	do_gamma_adjust = 1;		// 标记修改曲线表
	
#else
	#define	INTTIME_LOW		(CMOS_STD_INTTIME)
	#define	INTTINE_MEDIUM	(CMOS_STD_INTTIME*2)
	#define	INTTIME_HIGH	(CMOS_STD_INTTIME*4)
	#define	LUM_HIGH			(11)
	#define	LUM_LOW			(5)
	
	unsigned int cur_lum;
	
	if(do_gamma_adjust == 1)		// 等待上一次gamma修正写入到寄存器
		return;
	
	cur_lum = p_ae->lumCurr;	// isp_ae_lum_read();
	if(gamma_adjust_stage == 1 && inttime_again <= INTTIME_LOW)
		return;		// 继续s曲线
	else if(gamma_adjust_stage == 2 && inttime_again >= INTTINE_MEDIUM && inttime_again < INTTIME_HIGH)
		return;		// 继续hs曲线
	else if(gamma_adjust_stage == 2 && inttime_again >= INTTIME_HIGH && cur_lum >= LUM_HIGH)
		return;		// 继续hs曲线
	else if(gamma_adjust_stage == 3 && inttime_again >= INTTIME_HIGH && cur_lum <= LUM_LOW)
	{
		// 低照度, 继续线性曲线
		return;
	}
	
	//计算曲线
	if(inttime_again <= INTTIME_LOW)
	{
		//XM_printf ("gamma-->s\n");
		// s曲线(提高对比度, 改善场景的通透度)
		for (int i = 0; i < 65; i ++)
		{
			gamma_adjust_table[i] = gamma_LUT1[i];
		}
		gamma_adjust_stage = 1;
	}
	else if(inttime_again > INTTIME_LOW && inttime_again < INTTINE_MEDIUM)
	{
		// 过渡曲线1
		//XM_printf ("gamma-->t1\n");
		for (int i = 0; i < 65; i ++)
		{
			int value = ((int)gamma_LUT1[i]) + ((int)gamma_LUT2[i] - (int)gamma_LUT1[i]) * (inttime_again - INTTIME_LOW) / (INTTINE_MEDIUM - INTTIME_LOW);
			if(value < 0)
				value = 0;
			else if(value > 65535)
			{
				value = 65535;
				// 异常, 返回
				//XM_printf ("gamma error t1\n");
				return;
			}
			gamma_adjust_table[i] = (unsigned short)value; 
		}
		gamma_adjust_stage = 0;
	}
	else if(inttime_again >= INTTINE_MEDIUM && inttime_again < INTTIME_HIGH)
	{
		// half-s曲线
		//XM_printf ("gamma-->hs\n");
		for (int i = 0; i < 65; i ++)
		{
			gamma_adjust_table[i] = gamma_LUT2[i];
		}
		gamma_adjust_stage = 2;
	}
	else if(inttime_again >= INTTIME_HIGH)
	{
		if(cur_lum >= LUM_HIGH)
		{
			// half-s曲线
			//XM_printf ("gamma-->hs\n");
			for (int i = 0; i < 65; i ++)
			{
				gamma_adjust_table[i] = gamma_LUT2[i];
			}
			gamma_adjust_stage = 2;
		}
		else if(cur_lum <= LUM_LOW)
		{
			// 线性曲线(保留暗光照场景下的暗部细节, 同时改善强光照下车牌变白无法分辨的情况)
			//XM_printf ("gamma-->linear\n");
			for (int i = 0; i < 65; i ++)
			{
				gamma_adjust_table[i] = gamma_linear_table[i];
			}
			gamma_adjust_stage = 3;
		}
		else
		{
			// 过渡曲线2, 线性插值
			//XM_printf ("gamma-->t2\n");
			for (int i = 0; i < 65; i ++)
			{
				int value = ((int)gamma_linear_table[i]) + ((int)gamma_LUT2[i] - (int)gamma_linear_table[i]) * (cur_lum - LUM_LOW) / (LUM_HIGH - LUM_LOW);
				if(value < 0)
				{
					// 异常, 返回
					value = 0;
					//XM_printf ("gamma error t2\n");
					return;
				}
				else if(value > 65535)
				{
					// 异常, 返回
					value = 65535;
					//XM_printf ("gamma error t2\n");
					return;
				}
				gamma_adjust_table[i] = (unsigned short)value; 
			}
			gamma_adjust_stage = 0;
		}
	}
	for (int i = 0; i < 65; i ++)
	{
		p_colors->gamma.gamma_lut[i] = gamma_adjust_table[i];
	}
	
	do_gamma_adjust = 1;		// 标记修改曲线表
#endif
	
#endif
	
}


typedef struct _isp_denoise_inttime_polyline_tbl {
	int  inttime_gain;
	
	// 2D Filter 0
	int y_thres0;	
	int u_thres0;
	int v_thres0;
	
	// 2D Filter 1
	int y_thres1;	
	int u_thres1;
	int v_thres1;
	
	// 3D Filter
	int y_thres2;	
	int u_thres2;
	int v_thres2;
	
} isp_denoise_inttime_polyline_tbl;

#if ISP_3D_DENOISE_SUPPORT

// 3D开启时, 2D降噪强度可适当降低
static const isp_denoise_inttime_polyline_tbl denoise_inttime_polyline_tbl[] = {
// inttime_gain y_0  u_0  v_0   y_1  u_1  v_1   y_2   u_2  v_2
	
	// 20170227 综合评估后, 增加降噪强度, 从细节看, 基本无差异. 从PNG的码流来看, 有10%的压缩率. 
	// 20170226 恢复2D&3D降噪参数到20170217上午的参数. 
#if 1
	// 20170217 稍微增加一下降噪的强度(阈值加1). 20170217上午测试发现, 路边林荫道细节太多, 导致H264码流太大, 丢帧明显
	// 20170217 下午高速公路测试, 路边林荫道还是会存在停顿. 降噪强度增加后, 路边树叶的细节与原来(20170217上午的参数)比较,基本没有差异
	// 20170227 微调3D的降噪强度 + 1
	// 20170305 将2D的降噪强度减1(-1), 保留更多的细节
	// 20170305 周一2台机器对比测试 2D的降噪强度减1(-1)
#if NORMAL_VIDEO_QUALITY
	// 增加亮度的降噪, 降低码流 3 --> 4
	// 增加亮度的随机噪声抑制 3 --> 4
	{  
		//1,         3,  4,   4,     3,   4,   4,     3,   3,   3,
		1,         5,  3,   3,     5,   3,   3,     4,   3,   3,
	} ,
	
	// 20170305 将2D的降噪强度减1(-1), 保留更多的细节
	{  
		//12,        3,  5,   5,     3,   5,   5,     3,   3,   3,
		32,        5,  4,   4,     5,   4,   4,     4,   3,   3,
	} ,	
	{  
		//12,        3,  5,   5,     3,   5,   5,     3,   3,   3,
		128,       4,  4,   4,     4,   4,   4,     4,   3,   3,
	} ,	
	
	// e:\proj\ARKN141\test\ISP调试\20170202
	{
		1073,      4,   6,   6,    4,   6,   6,    4,   3,   3,
	},
	
#else	// 缺省高清效果
	{  
		//1,         3,  4,   4,     3,   4,   4,     3,   3,   3,
		1,         3,  3,   3,     3,   3,   3,     3,   3,   3,
	} ,
	
	// 20170305 将2D的降噪强度减1(-1), 保留更多的细节
	{  
		//12,        3,  5,   5,     3,   5,   5,     3,   3,   3,
		12,        3,  4,   4,     3,   4,   4,     3,   3,   3,
	} ,
	// e:\proj\ARKN141\test\ISP调试\20170202
	{
		1073,      3,   6,   6,    3,   6,   6,    4,   3,   3,
	},
	
#endif
	
	{
		CMOS_STD_INTTIME,      4,   6,   6,    4,   6,   6,    4,   4,   4,
	},

#else	
	// 20170217上午的参数
	// 20170227 微调3D的降噪强度 + 1
	{  
		// 1,         3,  3,   3,     3,   3,   3,     2,   3,   3,
		1,         3,  3,   3,     3,   3,   3,     3,   3,   3,
	} ,
	{  
		//12,        3,  4,   4,     3,   4,   4,     2,   3,   3,
		12,        3,  4,   4,     3,   4,   4,     3,   3,   3,
	} ,
	

	// e:\proj\ARKN141\test\ISP调试\20170202
	{
		1073,      3,   5,   5,     3,   5,   5,    3,   3,   3,
	},
	
	{
		CMOS_STD_INTTIME,      4,   5,   5,     4,   5,   5,    4,   4,   4,
	},
#endif
	
#if 1
	// 新的参数, 需要测试. 将3D阈值减3
	
#ifdef _DISABLE_GAMMA_UNDER_LOW_LIGHT_
	// 20170227
	// 低亮度场景关闭gamma(对比度拉伸), 减小光晕现象.
	// gamma关闭后, 噪声的电平(拉伸)也跟随降低. 此时可降低低亮度场景下的降噪程度
	{
		CMOS_STD_INTTIME*2,    5,   6,   6,     5,   6,   6,    5,   6,  6,
	},
	{
		CMOS_STD_INTTIME*4,    6,   8,   8,     6,   8,   8,    7,   8,  8,
	} ,
	{
		CMOS_STD_INTTIME*12,   10,  14,  14,    10,  14,  14,   15,  15, 15,
	} ,
	{
		CMOS_STD_INTTIME*14,   10,  16,  16,    10,  16,  16,   15,  16, 16,
	} ,
	{
		CMOS_STD_INTTIME*20,   14,  22,  22,    14,  22,  22,   15,  16, 16,
	},
	{
		CMOS_STD_INTTIME*40,   18,  30,  30,    18,  30,  30,   15,  16, 16,
	},
	
#else
	{
		CMOS_STD_INTTIME*2,    6,   8,   8,     6,   8,   8,    5,   5,  5,
	},
	{
		CMOS_STD_INTTIME*4,    10,  11,  11,    10,  11,  11,   5,   5,  5,
	} ,
	{
		CMOS_STD_INTTIME*12,   15,  15,  15,    15,  15,  15,   11,  11, 11,
	} ,
	{
		CMOS_STD_INTTIME*14,   18,  20,  20,    18,  20,  20,   13,  13, 13,
	} ,
	{
		CMOS_STD_INTTIME*20,   20,  24,  24,    20,  24,  24,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*40,   30,  45,  45,    30,  45,  45,   13,  13, 13,
	},
#endif

#else
 	// 20170217晚上之前的参数
	// 20170217晚上测试, 发现视频上下轻微抖动(基板抖动)时, 视频的文字会模糊, 白天不存在这种模糊现象.
	//	分析与3D降噪有关, 需要降低3D降噪的强度
	{
		CMOS_STD_INTTIME*2,    6,   8,   8,     6,   8,   8,    5,   7,  7,
	},
	{
		CMOS_STD_INTTIME*4,    10,  11,  11,    10,  11,  11,   6,   8,  8,
	} ,
	{
		CMOS_STD_INTTIME*12,   15,  15,  15,    15,  15,  15,   11,  11, 11,
	} ,
	{
		CMOS_STD_INTTIME*14,   18,  20,  20,    18,  20,  20,   13,  13, 13,
	} ,
	{
		CMOS_STD_INTTIME*20,   20,  24,  24,    20,  24,  24,   16,  16, 16,
	},
	{
		CMOS_STD_INTTIME*40,   30,  45,  45,    30,  45,  45,   16,  16, 16,
	},
#endif
	
};

#else

//  ******* 无3D降噪   ******

// 参考 ISP调试\2D降噪\14503843\, 强光照环境下, 曝光时间短. 为了尽可能保持画面的细节,需要减轻降噪的程度.
// 	inttime_gain = 12 配置3,4,4,3,4,4,可以较好的降噪(地面噪声)及保持树的细节
static const isp_denoise_inttime_polyline_tbl denoise_inttime_polyline_tbl[] = {
// inttime_gain y_0  u_0  v_0   y_1  u_1  v_1   y_2   u_2  v_2
	{  
		1,         3,  4,   4,     3,   4,   4,     3,   3,   3,
		//1,         5,  3,   3,     5,   3,   3,     4,   3,   3,
	} ,
	
	// 20170305 将2D的降噪强度减1(-1), 保留更多的细节
	{  
		12,        3,  5,   5,     3,   5,   5,     3,   3,   3,
		//32,        5,  4,   4,     5,   4,   4,     4,   3,   3,
	} ,	
	{  
		128,        3,  5,   5,     3,   5,   5,     3,   3,   3,
		//128,       4,  4,   4,     4,   4,   4,     4,   3,   3,
	} ,	
	
	// e:\proj\ARKN141\test\ISP调试\20170202
	{
		1073,      4,   6,   6,    4,   6,   6,    4,   3,   3,
	},
	
	// 20170227
	// 低亮度场景关闭gamma(对比度拉伸), 减小光晕现象.
	// gamma关闭后, 噪声的电平(拉伸)也跟随降低. 此时可降低低亮度场景下的降噪程度
	{
		CMOS_STD_INTTIME*2,    6,   6,   6,     6,   6,   6,    5,   6,  6,
	},
	{
		CMOS_STD_INTTIME*4,    8,   10,  10,     8,  10,   10,    7,   8,  8,
	} ,
	{
		CMOS_STD_INTTIME*12,   14,  16,  16,    14,  16,  16,   15,  15, 15,
	} ,
	{
		CMOS_STD_INTTIME*14,   16,  18,  18,    16,  18,  18,   15,  16, 16,
	} ,
	{
		CMOS_STD_INTTIME*20,   20,  22,  22,    20,  22,  22,   15,  16, 16,
	},
	{
		CMOS_STD_INTTIME*40,   48,  55,  55,    48,  55,  55,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*64,   90, 120, 120,      90, 120, 120,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*177,  220, 230, 230,   223, 230, 230,   13,  13, 13,
	},
	

};

#endif

static void denoise_match_inttime_gain (int inttime_gain, isp_denoise_inttime_polyline_tbl *denoise_tbl)
{
	int i;
	int val;
	const isp_denoise_inttime_polyline_tbl *lo, *hi;
	int count = sizeof(denoise_inttime_polyline_tbl)/sizeof(denoise_inttime_polyline_tbl[0]);
	for (i = 0; i < count; i ++)
	{
		if(inttime_gain <= denoise_inttime_polyline_tbl[i].inttime_gain)
			break;
	}
	
	// 匹配
	if(inttime_gain == denoise_inttime_polyline_tbl[i].inttime_gain)
	{
		memcpy (denoise_tbl, &denoise_inttime_polyline_tbl[i], sizeof(isp_denoise_inttime_polyline_tbl));
		return;
	}
	
	// 边界
	else if(inttime_gain < denoise_inttime_polyline_tbl[0].inttime_gain)
	{
		memcpy (denoise_tbl, &denoise_inttime_polyline_tbl[0], sizeof(isp_denoise_inttime_polyline_tbl));
		return;
	}
	else if(i == count)
	{
		memcpy (denoise_tbl, &denoise_inttime_polyline_tbl[count - 1], sizeof(isp_denoise_inttime_polyline_tbl));
		return;
	}
	
	lo = &denoise_inttime_polyline_tbl[i - 1];
	hi = &denoise_inttime_polyline_tbl[i];
	val = (lo->y_thres0 + (hi->y_thres0 - lo->y_thres0) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->y_thres0 = val;
	val = (lo->u_thres0 + (hi->u_thres0 - lo->u_thres0) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->u_thres0 = val;
	val = (lo->v_thres0 + (hi->v_thres0 - lo->v_thres0) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->v_thres0 = val;
	val = (lo->y_thres1 + (hi->y_thres1 - lo->y_thres1) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->y_thres1 = val;
	val = (lo->u_thres1 + (hi->u_thres1 - lo->u_thres1) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->u_thres1 = val;
	val = (lo->v_thres1 + (hi->v_thres1 - lo->v_thres1) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->v_thres1 = val;

	val = (lo->y_thres2 + (hi->y_thres2 - lo->y_thres2) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->y_thres2 = val;
	val = (lo->u_thres2 + (hi->u_thres2 - lo->u_thres2) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->u_thres2 = val;
	val = (lo->v_thres2 + (hi->v_thres2 - lo->v_thres2) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->v_thres2 = val;
}

static void imx322_cmos_isp_denoise_run (isp_denoise_ptr_t p_denoise, isp_ae_ptr_t p_ae)
{ 
	unsigned int inttime_gain;
	unsigned int data0, data1, data2, data3;
	isp_denoise_inttime_polyline_tbl denoise_inttime_tbl;
	unsigned int sensitiv0, sensitiv1;
	inttime_gain = cmos_calc_inttime_gain (&isp_exposure);

	denoise_match_inttime_gain ((int)inttime_gain, &denoise_inttime_tbl);
	
	// 关闭3D的UV降噪通道
	//denoise_inttime_tbl.u_thres2 = 0;
	//denoise_inttime_tbl.v_thres2 = 0;
		
	data1 = (denoise_inttime_tbl.y_thres0 <<  0)
			| (denoise_inttime_tbl.u_thres0 << 10) 
			| (denoise_inttime_tbl.v_thres0 << 20);
	data2 = (denoise_inttime_tbl.y_thres1 <<  0)
			| (denoise_inttime_tbl.u_thres1 << 10) 
			| (denoise_inttime_tbl.v_thres1 << 20);
	data3 = (denoise_inttime_tbl.y_thres2 <<  0)
			| (denoise_inttime_tbl.u_thres2 << 10) 
			| (denoise_inttime_tbl.v_thres2 << 20);

		
		
	
	// 更新参数表
	p_denoise->y_thres0 = (data1 >>  0) & 0x3ff;
	p_denoise->u_thres0 = (data1 >> 10) & 0x3ff;
	p_denoise->v_thres0 = (data1 >> 20) & 0x3ff;
	p_denoise->y_thres1 = (data2 >>  0) & 0x3ff;
	p_denoise->u_thres1 = (data2 >> 10) & 0x3ff;
	p_denoise->v_thres1 = (data2 >> 20) & 0x3ff;
	p_denoise->y_thres2 = (data3 >>  0) & 0x3ff;
	p_denoise->u_thres2 = (data3 >> 10) & 0x3ff;
	p_denoise->v_thres2 = (data3 >> 20) & 0x3ff;
	
  
	Gem_write ((GEM_DENOISE_BASE+0x04), data1);
	Gem_write ((GEM_DENOISE_BASE+0x08), data2);
	Gem_write ((GEM_DENOISE_BASE+0x0c), data3); 
	
	if(inttime_gain <= (CMOS_STD_INTTIME*2))
	{
		sensitiv0 = 3;
		sensitiv1 = 3;
	}
	else if(inttime_gain <= (CMOS_STD_INTTIME*5))
	{
		sensitiv0 = 4;
		sensitiv1 = 4;
	}
	else if(inttime_gain <= (CMOS_STD_INTTIME*12))
	{
		// 模拟增益
		sensitiv0 = 4;
		sensitiv1 = 4;		
	}
	
	else
	{
		// 数字增益开启
		sensitiv0 = 5;
		sensitiv1 = 5;
	}
	
	p_denoise->sensitiv0 = sensitiv0;
	p_denoise->sensitiv1 = sensitiv1;
	
	data0 =  ((p_denoise->enable2d & 0x07) <<  0) 
			| ((p_denoise->enable3d & 0x07) <<  3) 
			| ((p_denoise->sel_3d_table & 0x03) << 8)	// 3D高斯表选择
			| ((p_denoise->sensitiv0 & 0x07) << 10)	// 2D滤波器0(差分)灵敏度设置, 0 滤波关闭	
			| ((p_denoise->sensitiv1 & 0x07) << 13)	// 2D滤波器1(边沿)灵敏度设置, 0 滤波关闭
			| ((p_denoise->sel_3d_matrix & 0x01) << 16)		// 3D相干性矩阵区域选择， 0 邻近 1 中心点
			;
	Gem_write ((GEM_DENOISE_BASE+0x00), data0);
	
}

typedef struct _isp_eris_polyline_tbl {
	int  inttime_gain;
	int  gain_max;
	int  resolt_ratio;	
	int  colort_ratio;	
} isp_eris_polyline_tbl;

// 为了减少低照度场景的噪声, 最小化eris的拉伸强度

static const isp_eris_polyline_tbl eris_polyline_tbl[] = {
	
	// 20170305 增加短曝光场景的拉伸强度, 改善解析度及颜色
	{	1,		   112, (int)(0.95 * 512),  (int)(0.75 * 512) },
	{	5,		   144, (int)(0.96 * 512),  (int)(0.75 * 512) },
	{	32,	   196, (int)(0.97 * 512),  (int)(0.75 * 512) },
	
//	{	1,		   112,  (int)(0.8 * 512),  (int)(0.7 * 512) },
//	{	5,		   144, (int)(0.8 * 512),  (int)(0.7 * 512) },
//	{	32,	   196, (int)(0.85 * 512),  (int)(0.7 * 512) },
	// 20170215 短曝光场景(1, 5, 32)应稍微降低增益(128, 192, 256 --> 96, 144, 208), 减轻高层建筑顶端发白的情况
	
	{  80,	   240, (int)(0.99 * 512),  (int)(0.8 * 512) },
	{  400,	   240, (int)(0.99 * 512),  (int)(0.8 * 512) },
	{	700,   	240, (int)(0.99 * 512),  (int)(0.8 * 512) },
	{	820,	   160, (int)(0.98 * 512),  (int)(0.8 * 512) },
	{	900,	   112, (int)(0.98 * 512),  (int)(0.8 * 512) },
	{	1023,	   80,  (int)(0.98 * 512),  (int)(0.75 * 512) },

	// 20170217 地下车库测试发现, N141墙壁上指引标志的色彩偏暗, 稍微提高暗场景下色彩的比率因子(+0.1)
	{	CMOS_STD_INTTIME,	   64,  (int)(0.98 * 512),  (int)(0.75 * 512) },
	{	CMOS_STD_INTTIME*2,	48,  (int)(0.98 * 512),  (int)(0.75 * 512) },
	{	CMOS_STD_INTTIME*4,	32,  (int)(0.98 * 512),  (int)(0.6 * 512) },
	
	// 20170223 降低eris增益值, 减弱光晕现象 (256 --> 176)
	//{	CMOS_STD_INTTIME*10,	256,  (int)(0.4 * 512),  (int)(0.4 * 512) },
//	{	CMOS_STD_INTTIME*10,	176,  (int)(0.4 * 512),  (int)(0.4 * 512) },
	// 20170223 降低eris增益值, 减弱光晕现象 (256 --> 128)
	//{	CMOS_STD_INTTIME*10,	128,  (int)(0.4 * 512),  (int)(0.4 * 512) },
	// 20170226 降低eris增益值为8, 降低光晕现象
	{	CMOS_STD_INTTIME*10,	28,   (int)(0.90 * 512),  (int)(0.5 * 512) },
	
	{	CMOS_STD_INTTIME*12,	24,   (int)(0.80 * 512),  (int)(0.5 * 512) },
	// 20170217 
	// 夜晚的场景基本是最大物理增益开启, 实测时光晕较大, 前车的车牌因为增益过大, 车牌位置发白的情况较多.
	// 适当降低最大物理增益时的eris增益值(256 --> 176), 增加场景的对比度, 减弱光晕现象, 缓解车牌区域发白的情况.
	// 20170223 降低eris增益值, 减弱光晕现象 (176 --> 128)
	//{	CMOS_STD_INTTIME*15,	176,  (int)(0.4 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*15,	128,  (int)(0.4 * 512),  (int)(0.4 * 512) },
	// 20170226 降低eris增益值为8, 降低光晕现象
	{	CMOS_STD_INTTIME*15,	16,   (int)(0.80 * 512),  (int)(0.4 * 512) },
	
	{	CMOS_STD_INTTIME*35,		32,   (int)(0.60 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*60,		48,   (int)(0.50 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*128,	64,   (int)(0.40 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*177,	128,   (int)(0.20 * 512),  (int)(0.4 * 512) },
};

static void match_resolt_colort (int inttime_gain,
								  unsigned char resolt[33],
								  unsigned short colort[33],
								  int *gain_max_data)
{
	int i;
	int bright;
	const isp_eris_polyline_tbl *lo, *hi;
	int resolt_ratio, colort_ratio;
	int gain_max;
	int count = sizeof(eris_polyline_tbl)/sizeof(eris_polyline_tbl[0]);
	for (i = 0; i < count; i ++)
	{
		if(inttime_gain <= (float)eris_polyline_tbl[i].inttime_gain)
			break;
	}
	// 匹配
	if(inttime_gain == eris_polyline_tbl[i].inttime_gain)
	{
		resolt_ratio = eris_polyline_tbl[i].resolt_ratio;
		colort_ratio = eris_polyline_tbl[i].colort_ratio;
		gain_max = eris_polyline_tbl[i].gain_max;
	}
	// 边界
	else if(inttime_gain < eris_polyline_tbl[0].inttime_gain)
	{
		resolt_ratio = eris_polyline_tbl[0].resolt_ratio;
		colort_ratio = eris_polyline_tbl[0].colort_ratio;
		gain_max = eris_polyline_tbl[0].gain_max;
	}
	else if(i == count)
	{
		resolt_ratio = eris_polyline_tbl[count - 1].resolt_ratio;
		colort_ratio = eris_polyline_tbl[count - 1].colort_ratio;
		gain_max = eris_polyline_tbl[count - 1].gain_max;
	}
	else
	{
		lo = &eris_polyline_tbl[i - 1];
		hi = &eris_polyline_tbl[i];
		resolt_ratio = lo->resolt_ratio + (hi->resolt_ratio - lo->resolt_ratio) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain);
		colort_ratio = lo->colort_ratio + (hi->colort_ratio - lo->colort_ratio) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain);
		gain_max = lo->gain_max + (hi->gain_max - lo->gain_max) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain);
	}
	
	if(resolt_ratio < 0)
		resolt_ratio = 0;
	if(colort_ratio < 0)
		colort_ratio = 0;
	
	for (i = 0; i < 33; i ++)
	{
		unsigned int val = (imx322_default_resolt[i] * resolt_ratio) >> 9;
		if(val >= 230)
			val = 230;
		resolt[i] = (unsigned char)val;
	}
	
	for (i = 0; i < 33; i ++)
	{
		unsigned int val = (imx322_default_colort[i] * colort_ratio) >> 9;
		if(val >= 511)
			val = 511;
		colort[i] = (unsigned short)val;
	}
	
	if(gain_max < 4)
		gain_max = 4;
	else if(gain_max > 256)
		gain_max = 256;
	*gain_max_data = gain_max;
}

typedef struct _isp_eris_dimlight_polyline_tbl {
	int  inttime_gain;
	int  white;	
} isp_eris_dimlight_polyline_tbl;

static const isp_eris_dimlight_polyline_tbl eris_dimlight_polyline_tbl[] = {	
	{	1,		  							4095 },
	{	CMOS_STD_INTTIME * 1,		4095 },
	{	CMOS_STD_INTTIME * 10,		3900 },	
	//{	CMOS_STD_INTTIME * 24,		2048 },	
	//{	CMOS_STD_INTTIME * 32,		2048 },	
	//{	CMOS_STD_INTTIME * 48,		1024 },	
	//{	CMOS_STD_INTTIME * 64,		384 },	
};


static void match_dimlight (int inttime_gain,  int *dimlight_white)
{
	int i;
	int white;
	const isp_eris_dimlight_polyline_tbl *lo, *hi;
	int count = sizeof(eris_dimlight_polyline_tbl)/sizeof(eris_dimlight_polyline_tbl[0]);
	for (i = 0; i < count; i ++)
	{
		if(inttime_gain <= (float)eris_dimlight_polyline_tbl[i].inttime_gain)
			break;
	}
	// 匹配
	if(inttime_gain == eris_dimlight_polyline_tbl[i].inttime_gain)
	{
		white = eris_dimlight_polyline_tbl[i].white;
	}
	// 边界
	else if(inttime_gain < eris_dimlight_polyline_tbl[0].inttime_gain)
	{
		white = eris_dimlight_polyline_tbl[0].white;
	}
	else if(i == count)
	{
		white = eris_dimlight_polyline_tbl[count - 1].white;
	}
	else
	{
		lo = &eris_dimlight_polyline_tbl[i - 1];
		hi = &eris_dimlight_polyline_tbl[i];
		white = lo->white + (hi->white - lo->white) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain);
	}
	
	if(white < 256)
		white = 256;
	if(white > 4095)
		white = 4095;

	*dimlight_white = white;	
}

//#define	BACKLIGHT_COMP	1
// 高亮度场景下(大面积天空下的高速公路, 逆光), 
// 	此时路边的场景及车辆因为曝光时间短, 亮度偏暗, 很难看清楚前车的车牌.此时应适当提升gain的增益, 增加亮度, 这样可以看到更多的细节(如前车的车牌).
//		但是增加亮度会导致对比度的降低, 因此仅在判定当前场景为逆光高亮度场景时应用.
// 规则初步定义如下
// 	ratio = 9宫格最大亮度的平均值 / (3个底部格子(20+21+22)值的累加和) 为gain调整系数
//		ratio > 2时进入逆光模式, 按照逆光法则微调增益值, 调整亮度
//	
static void imx322_cmos_isp_eris_run (isp_eris_ptr_t p_eris ,isp_ae_ptr_t p_ae)
{
	int i;
	unsigned int data0, data2;
	unsigned int inttime_gain;
	
	int gain_max;
	
#if BACKLIGHT_COMP
	int backlight_ratio;		// 逆光系数
	int max_lum, ground_lum;
	unsigned short *yavg_s;
	unsigned int backlight_gain_ratio = 256;
	
	if(isp_exposure.cmos_inttime.exposure_ashort <= 80)
	{
		max_lum = 0;
		yavg_s = &p_ae->yavg_s[0][0];
		for (i = 0; i < 9; i ++)
		{
			if(max_lum < yavg_s[i])
				max_lum = yavg_s[i];
		}
		ground_lum = p_ae->yavg_s[2][0] + p_ae->yavg_s[2][1] + p_ae->yavg_s[2][2];
		ground_lum ++;		// 避免为0
		backlight_ratio = max_lum / ground_lum;
		if(backlight_ratio >= 4)
		{
			// 启动逆光增益补偿
			// 补光增益区间 1 ~ 2 
			backlight_gain_ratio = 256 * max_lum / (ground_lum * 4);
		}
	}
#endif
	
	inttime_gain = cmos_calc_inttime_gain (&isp_exposure);
	
	// 根据曝光值, 查找场景匹配表, 决定拉伸表与色彩表的设置
	match_resolt_colort (inttime_gain, p_eris->resolt, p_eris->colort, &gain_max);
	for (i = 0; i < 33; i++)
	{
		data0 = (0x02) | (i << 8) | (p_eris->resolt[i]<<16);
		Gem_write ((GEM_LUT_BASE+0x00), data0);
	}
	
	for (i = 0; i < 33; i++)
	{
		data0 = (0x03) | (i << 8) | (p_eris->colort[i]<<16);
		Gem_write ((GEM_LUT_BASE+0x00), data0);
	}
	
#if BACKLIGHT_COMP
	// 逆光补偿
	gain_max = gain_max * backlight_gain_ratio >> 8;
	if(gain_max >= 256)
		gain_max = 256;
#endif
	
	// 暂时关闭eris动态增益调整
	p_eris->gain_max = gain_max;
	p_eris->gain_min = gain_max;
	data2 = (p_eris->gain_max) | (p_eris->gain_min << 16);
	Gem_write ((GEM_ERIS_BASE+0x08), data2);
	
	if(cmos_exposure_get_eris_dimlight())
	{
		int white;
		unsigned int data1;
		match_dimlight (inttime_gain, &white);
		p_eris->white = white;
		data1 = (p_eris->black)    | (p_eris->white << 16);
		Gem_write ((GEM_ERIS_BASE+0x04), data1);
	}
	
}

typedef struct _isp_lsc_polyline_tbl {
	int	inttime_gain;
	int	lscoff;	//	同心圆半径, lsc仅校正同心圆之外的区域, 半径越大, 校正区域越小, 强度越弱.
} isp_lsc_polyline_tbl;

static const isp_lsc_polyline_tbl lsc_polyline_tbl[] = {
	//  inttime_gain    	lscoff
	{		800,		  		50	 },
	{		CMOS_STD_INTTIME,		  		150 },
	{		CMOS_STD_INTTIME * 4,	  	250 },
};

static void lsc_match (int inttime_gain, isp_lsc_polyline_tbl *lsc_tbl)
{
	int i;
	int lscoff;
	const isp_lsc_polyline_tbl *lo, *hi;
	int count = sizeof(lsc_polyline_tbl)/sizeof(lsc_polyline_tbl[0]);
	for (i = 0; i < count; i ++)
	{
		if(inttime_gain <= (float)lsc_polyline_tbl[i].inttime_gain)
			break;
	}
	// 匹配
	if(inttime_gain == lsc_polyline_tbl[i].inttime_gain)
	{
		lscoff = lsc_polyline_tbl[i].lscoff;
	}
	// 边界
	else if(inttime_gain < lsc_polyline_tbl[0].inttime_gain)
	{
		lscoff =  lsc_polyline_tbl[0].lscoff;
	}
	else if(i == count)
	{
		lscoff = lsc_polyline_tbl[count - 1].lscoff;
	}
	else
	{
		lo = &lsc_polyline_tbl[i - 1];
		hi = &lsc_polyline_tbl[i];
		lscoff = (int)(lo->lscoff + (hi->lscoff - lo->lscoff) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	}
	
	if(lscoff > 250)
		lscoff = 250;
	else if(lscoff < 50)
		lscoff = 50;
	
	lsc_tbl->lscoff = lscoff;
}


typedef struct _isp_crosstalk_polyline_tbl {
	int	inttime_gain;
	int	thres;	
} isp_crosstalk_polyline_tbl;

#if ISP_3D_DENOISE_SUPPORT

// 3D开启时的参数, 
static const isp_crosstalk_polyline_tbl crosstalk_polyline_tbl[] = {
	//  inttime_gain    	crosstalk
	// 20170217上午及之前使用的版本
	{		50,				2	 },
	{		100,				3	 },
	{		161,				3	 },
	{		242,				3	 },
	{		1024,		  		3	 },
	{		1125,		  		4	 },
	{		1125 * 2,	  	7   },
	{		1125 * 4,	  	10  },
	{		1125 * 10,	  	13  },
	{  	1125 * 30,     25  },
};

#else

// 3D关闭时的参数, 
static const isp_crosstalk_polyline_tbl crosstalk_polyline_tbl[] = {
	//  inttime_gain    	crosstalk
	// 20170217上午及之前使用的版本
	{		50,				2 + 1	 },
	{		100,				3 + 1	 },
	{		161,				3 + 1	 },
	{		242,				3 + 1	 },
	{		1024,		  		3 + 2	 },
	{		CMOS_STD_INTTIME,		  		4 + 2	 },
	{		CMOS_STD_INTTIME * 2,	  	7 + 3  },
	{		CMOS_STD_INTTIME * 4,	  	10+ 3  },
	{		CMOS_STD_INTTIME * 10,	  	13+ 4  },
	{  	CMOS_STD_INTTIME * 30,     35+ 4  },
	{  	CMOS_STD_INTTIME * 64,     120  },
	{  	CMOS_STD_INTTIME * 128,    160  },
	{  	CMOS_STD_INTTIME * 177,    260  },
};

#endif

static void crosstalk_match (int inttime_gain, isp_crosstalk_polyline_tbl *crosstalk_tbl)
{
	int i;
	int thres;
	const isp_crosstalk_polyline_tbl *lo, *hi;
	int count = sizeof(crosstalk_polyline_tbl)/sizeof(crosstalk_polyline_tbl[0]);
	for (i = 0; i < count; i ++)
	{
		if(inttime_gain <= (float)crosstalk_polyline_tbl[i].inttime_gain)
			break;
	}
	// 匹配
	if(inttime_gain == crosstalk_polyline_tbl[i].inttime_gain)
	{
		thres = crosstalk_polyline_tbl[i].thres;
	}
	// 边界
	else if(inttime_gain < crosstalk_polyline_tbl[0].inttime_gain)
	{
		thres =  crosstalk_polyline_tbl[0].thres;
	}
	else if(i == count)
	{
		thres = crosstalk_polyline_tbl[count - 1].thres;
	}
	else
	{
		lo = &crosstalk_polyline_tbl[i - 1];
		hi = &crosstalk_polyline_tbl[i];
		thres = (int)(lo->thres + (hi->thres - lo->thres) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	}
	
	if(thres > 256)
		thres = 256;
	else if(thres < 3)
		thres = 3;
	
	crosstalk_tbl->thres = thres;
}
static void imx322_cmos_isp_fesp_run (isp_fesp_ptr_t p_fesp, isp_ae_ptr_t p_ae)
{
	// CrossTalk
	int inttime_gain;
	unsigned int data0, data1;
	isp_crosstalk_polyline_tbl crosstalk_tbl;
	isp_lsc_polyline_tbl lsc_tbl;

	int thres;
	int lscoff;
	
	inttime_gain = cmos_calc_inttime_gain (&isp_exposure);
	
	crosstalk_match (inttime_gain, &crosstalk_tbl);
	thres = crosstalk_tbl.thres;
	
	p_fesp->Crosstalk.thresh = thres;
	p_fesp->Crosstalk.thres1cgf = thres;
	p_fesp->Crosstalk.thres0cgf = thres;
	data0 = ((p_fesp->Crosstalk.enable    & 0x0001) << 0 ) 	// bit0 crosstalk enable (1: enable 0:disable)
			| ((p_fesp->Crosstalk.mode      & 0x0003) << 1 )	// bit1-bit2 crosstalk mode  (00: unite filter thres=128 10: use reg thres x1: base on lut)
			| ((p_fesp->Crosstalk.snsCgf    & 0x0003) << 3 )	// bit3-bit4 snsCgf
			| ((p_fesp->Crosstalk.thres0cgf & 0xFFFF) << 16)	// bit16-bit31 thres0cgf
			;
	data1 = ((p_fesp->Crosstalk.thresh    & 0xFFFF) <<  0)		// bit0-bit15     Crosstalk_thresh       thres2cgf
			| ((p_fesp->Crosstalk.thres1cgf & 0xFFFF) << 16)		// bit16-bit31    Crosstalk_thresh       thres1cgf
			;
	
	Gem_write ((GEM_CROSS_BASE+0x00), data0);
	Gem_write ((GEM_CROSS_BASE+0x04), data1);   
	
	// Lense Shading correct
	// 不修正其曲线参数, 仅根据曝光值修正其校正原点的同心圆半径, 同心圆半径之外的区域为校正区域.
	// 即为了避免lsc导致噪声增加, 亮度低的场景其同心圆半径将增大, 减小校正的区域及强度, 减弱噪声 
	// 计算同心圆半径
	lsc_match (inttime_gain, &lsc_tbl);
	lscoff = lsc_tbl.lscoff;
	p_fesp->Lensshade.lscofst = lscoff;
	// bit15-0        lscofst
	data0 = (p_fesp->Lensshade.lscofst & 0xFFFF);
	Gem_write ((GEM_LENS_LSCOFST_BASE+0x00), data0);
	
}

typedef struct _isp_enhance_polyline_tbl {
	int	inttime;
	int	bright;	
} isp_enhance_polyline_tbl;

#pragma data_alignment=32

static const isp_enhance_polyline_tbl enhance_polyline_tbl[] = {
	//  inttime   bright
	//{		3,		   -6	   },
	//{		64,	   -8 	},
	// 保留短曝光低光处的细节
	{		3,		   -2	   },
	{		64,	   -2	 	},
	
	{   512,       -8    },
	{   800,       -8    },
#if HIGH_CONTRAST
	{	 1125,		-9	},		// 场景光照较弱时, 噪声增加, 相应增加黑电平
	{	 CMOS_STD_INTTIME * 2,	  	-11},
#else
	{	 1125,		-3	},		// 场景光照较弱时, 噪声增加, 相应增加黑电平
#endif
};

static void enhance_match (int inttime, isp_enhance_polyline_tbl *enhance_tbl)
{
	int i;
	int bright;
	const isp_enhance_polyline_tbl *lo, *hi;
	int count = sizeof(enhance_polyline_tbl)/sizeof(enhance_polyline_tbl[0]);
	for (i = 0; i < count; i ++)
	{
		if(inttime <= enhance_polyline_tbl[i].inttime)
			break;
	}
	// 匹配
	if(inttime == enhance_polyline_tbl[i].inttime)
	{
		bright = enhance_polyline_tbl[i].bright;
	}
	// 边界
	else if(inttime < enhance_polyline_tbl[0].inttime)
	{
		bright =  enhance_polyline_tbl[0].bright;
	}
	else if(i == count)
	{
		bright = enhance_polyline_tbl[count - 1].bright;
	}
	else
	{
		lo = &enhance_polyline_tbl[i - 1];
		hi = &enhance_polyline_tbl[i];
		bright = lo->bright + (hi->bright - lo->bright) * (inttime - lo->inttime) / (hi->inttime - lo->inttime);
	}
	
	if(bright > 8)
		bright = 8;
	else if(bright < -10)
		bright = -10;
	
	enhance_tbl->bright = bright;
}

typedef struct _isp_satuation_polyline_tbl {
	int inttime;
	int	satuation;	
} isp_satuation_polyline_tbl;

static const isp_satuation_polyline_tbl satuation_polyline_tbl[] = {
	//  inttime    satuation
	{		1 * CMOS_STD_INTTIME,		  1024+SATUATION_OFFSET	},
	{		10 * CMOS_STD_INTTIME,	  1024+SATUATION_OFFSET	},
	{  	12 * CMOS_STD_INTTIME,     1024   },
	{  	24 * CMOS_STD_INTTIME,     720   },
	{  	32 * CMOS_STD_INTTIME,     512    },
	{  	40 * CMOS_STD_INTTIME,     384    },
};

static void satuation_match (int inttime, isp_satuation_polyline_tbl *satuation_tbl)
{
	int i;
	int satuation;
	const isp_satuation_polyline_tbl *lo, *hi;
	int count = sizeof(satuation_polyline_tbl)/sizeof(satuation_polyline_tbl[0]);
	for (i = 0; i < count; i ++)
	{
		if(inttime <= satuation_polyline_tbl[i].inttime)
			break;
	}
	// 匹配
	if(inttime == satuation_polyline_tbl[i].inttime)
	{
		satuation = satuation_polyline_tbl[i].satuation;
	}
	// 边界
	else if(inttime < satuation_polyline_tbl[0].inttime)
	{
		satuation =  satuation_polyline_tbl[0].satuation;
	}
	else if(i == count)
	{
		satuation = satuation_polyline_tbl[count - 1].satuation;
	}
	else
	{
		lo = &satuation_polyline_tbl[i - 1];
		hi = &satuation_polyline_tbl[i];
		satuation = (lo->satuation + (hi->satuation - lo->satuation) * (inttime - lo->inttime) / (hi->inttime - lo->inttime));
	}
	
	if(satuation > 1280)
		satuation = 1280;
	else if(satuation < 256)
		satuation = 256;
		
	satuation_tbl->satuation = satuation;
}

#ifdef _HDTV_000255_
#define	_ENABLE_BRIGHT_RUN_	1
#endif

static void imx322_cmos_isp_enhance_run (isp_enhance_ptr_t p_enhance)
{
	// 曝光时间很短时(强光照场景)
	unsigned int data1, data2;
	int bright;
	unsigned int inttime;
	//float again;
	int satuation;
	isp_enhance_polyline_tbl enhance_polyline_tbl;
	isp_satuation_polyline_tbl satuation_tbl;
	// 
	inttime = cmos_calc_inttime_gain (&isp_exposure);
	
	// 20170223 关闭亮度的运行调整. 已采用16235区域
#if _ENABLE_BRIGHT_RUN_
	enhance_match (inttime, &enhance_polyline_tbl);
	p_enhance->bcst.bright = enhance_polyline_tbl.bright;
	data1 	= ((p_enhance->bcst.enable    & 0x001) << 31) 
	  		| ((p_enhance->bcst.bright    & 0x3FF) <<  0) 
			| ((p_enhance->bcst.offset0   & 0x3FF) << 10) 
			| ((p_enhance->bcst.offset1   & 0x3FF) << 20)
			;
	Gem_write ((GEM_ENHANCE_BASE+0x04), data1);
#endif
	
	satuation_match (inttime, &satuation_tbl);
	satuation = satuation_tbl.satuation;
	p_enhance->bcst.satuation = satuation;
 	data2 	= ((p_enhance->bcst.contrast  & 0x7FF) <<  0) 
	  		| ((p_enhance->bcst.satuation & 0x7FF) << 11) 
			| ((p_enhance->bcst.hue       & 0x0FF) << 24)
			;
	Gem_write ((GEM_ENHANCE_BASE+0x08), data2);
	

}

typedef struct _isp_ae_polyline_tbl {
	int	inttime;
	u8_t	compensation;
	u8_t	black_target;
	u8_t	window_weight[9];
} isp_ae_polyline_tbl;

// 增强白天曝光
// 20171122下午测试， 比较MSTAR及善领, 场景偏暗， 增加曝光
static const isp_ae_polyline_tbl ae_polyline_tbl[] = {
	{	16,							64,	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							64,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},	
	{	512,							56,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	878,							32,	128,	{4, 5, 4, 5, 10, 5, 6,  8,  6 }	},
	{	CMOS_STD_INTTIME,			24,	128,	{4, 5, 4, 5, 10, 5, 6,  8,  6 }	},
	{	CMOS_STD_INTTIME*5/2,	19,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*8,		18,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*11,		17,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},	
	//{	CMOS_STD_INTTIME*11,		18,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	//{	CMOS_STD_INTTIME*64,		17,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	
};

static const isp_ae_polyline_tbl ae_polyline_tbl_20171113_night[] = {
	{	16,							50,	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							46,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	512,							35,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	CMOS_STD_INTTIME,			24,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	CMOS_STD_INTTIME*5/2,	23,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	//{	CMOS_STD_INTTIME*8,		22,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	//{	CMOS_STD_INTTIME*11,		18,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},	
	{	CMOS_STD_INTTIME*16,		20,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	{	CMOS_STD_INTTIME*32,		16,	128,	{5, 5, 5, 5, 5,  5,  5, 5,  5}	},		
	//{	CMOS_STD_INTTIME*64,		16,	128,	{5, 5, 5, 5, 5,  5,  5, 5,  5}	},		
	//{	CMOS_STD_INTTIME*128,	16,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	//{	CMOS_STD_INTTIME*256,	16,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	}		
};

static const isp_ae_polyline_tbl ae_polyline_tbl_20171122[] = {
	{	16,							50,	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							46,	128,	{4, 5, 4, 5, 10, 5, 6,  8,  6 }	},
	{	512,							38,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	878,							28,	128,	{4, 5, 4, 5, 10, 5, 6,  8,  6 }	},
	{	CMOS_STD_INTTIME,			24,	128,	{4, 5, 4, 5, 10, 5, 6,  8,  6 }	},
	{	CMOS_STD_INTTIME*5/2,	19,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*8,		18,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*11,		17,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},	
};

static const isp_ae_polyline_tbl ae_polyline_tbl_[] = {
	{	16,							50,	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							46,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	CMOS_STD_INTTIME,			38,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	CMOS_STD_INTTIME*5/2,	28,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	{	CMOS_STD_INTTIME*8,		22,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	{	CMOS_STD_INTTIME*11,		18,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},	
	{	CMOS_STD_INTTIME*16,		16,	112,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},	
	{	CMOS_STD_INTTIME*48,		16,	104,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	{	CMOS_STD_INTTIME*64,		16,	96,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	{	CMOS_STD_INTTIME*128,	16,	80,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},	
	//{	CMOS_STD_INTTIME*178,	16,	64,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},	
	
	// 最大178

};

static void ae_match (int inttime, isp_ae_polyline_tbl *ae_tbl)
{
	int i;
	int black_target;
	const isp_ae_polyline_tbl *lo, *hi;
	int count = sizeof(ae_polyline_tbl)/sizeof(ae_polyline_tbl[0]);
	ae_tbl->inttime = inttime;
	for (i = 0; i < count; i ++)
	{
		if(inttime <= ae_polyline_tbl[i].inttime)
			break;
	}
	// 匹配
	if(inttime == ae_polyline_tbl[i].inttime)
	{
		memcpy (ae_tbl->window_weight, ae_polyline_tbl[i].window_weight, 9);
		ae_tbl->compensation = ae_polyline_tbl[i].compensation;
		ae_tbl->black_target = ae_polyline_tbl[i].black_target;
	}
	// 边界
	else if(inttime < ae_polyline_tbl[0].inttime)
	{
		memcpy (ae_tbl->window_weight, ae_polyline_tbl[0].window_weight, 9);
		ae_tbl->compensation = ae_polyline_tbl[0].compensation;
		ae_tbl->black_target = ae_polyline_tbl[0].black_target;
	}
	else if(i == count)
	{
		memcpy (ae_tbl->window_weight, ae_polyline_tbl[count - 1].window_weight, 9);
		ae_tbl->compensation = ae_polyline_tbl[count - 1].compensation;
		ae_tbl->black_target = ae_polyline_tbl[count - 1].black_target;
	}
	else
	{
		lo = &ae_polyline_tbl[i - 1];
		hi = &ae_polyline_tbl[i];
		for (int index = 0; index < 9; index ++)
		{
			int w = lo->window_weight[index] + (hi->window_weight[index] - lo->window_weight[index]) * (inttime - lo->inttime) / (hi->inttime - lo->inttime);
			if(w < 0)
				w = 0;
			else if(w > 15)
				w = 15;
			ae_tbl->window_weight[index] = w;	
		}
		
		int comp = lo->compensation + (hi->compensation - lo->compensation) * (inttime - lo->inttime) / (hi->inttime - lo->inttime);
		if(comp < 4)
			comp = 4;
		else if(comp > 128)
			comp = 128;
		ae_tbl->compensation = comp;
		
		black_target = lo->black_target + (hi->black_target - lo->black_target) * (inttime - lo->inttime) / (hi->inttime - lo->inttime);
		if(black_target < 48)
			black_target = 48;
		else if(black_target > 128)
			black_target = 128;
		ae_tbl->black_target = black_target;		
	}
	/*
	printf ("win=[%02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d]\n", 
			  ae_tbl->window_weight[0], ae_tbl->window_weight[1], ae_tbl->window_weight[2],
			  ae_tbl->window_weight[3], ae_tbl->window_weight[4], ae_tbl->window_weight[5],
			  ae_tbl->window_weight[6], ae_tbl->window_weight[7], ae_tbl->window_weight[8]);
	*/			  
}


// 逆光
static u8_t ae_win_weight_backlight[3][3] = 
	{3, 4, 3, 6, 15, 6, 12, 15, 12};		// 

// 正常
static u8_t ae_win_weight_normal[3][3] = 
	{4, 5, 4, 5, 10, 5, 7, 10, 7};			// 强化隧道模式, 兼顾夜晚车牌(底部)

// 夜晚
static u8_t ae_win_weight_night[3][3] = 
	{3, 3, 3, 5, 12, 5, 12, 13, 12};		// 夜晚车牌/招牌

static void imx322_cmos_isp_ae_run (isp_ae_ptr_t p_ae)
{
	unsigned int inttime = cmos_calc_inttime_gain (&isp_exposure);
	
#if 1
	
	isp_ae_polyline_tbl ae_tbl;
	
	ae_match (inttime, &ae_tbl);
	
	isp_ae_window_weight_write (&isp_exposure.cmos_ae, (u8_t (*)[3])(ae_tbl.window_weight));

	isp_system_ae_compensation_write (&isp_exposure.cmos_ae, (u8_t)ae_tbl.compensation);	
	isp_system_ae_black_target_write (&isp_exposure.cmos_ae, (u8_t)ae_tbl.black_target);		
	isp_auto_exposure_compensation (&isp_exposure.cmos_ae, isp_exposure.cmos_ae.histogram.bands);	
	
#else
		
	// 切换时出现闪烁现象
	switch (arkn141_isp_get_ae_window_mode())
	{
		case AE_WINDOW_MODE_NORMAL:
			if(inttime < 16)
			{
				// 切换到逆光模式
				arkn141_isp_set_ae_window_mode (AE_WINDOW_MODE_BACKLIGHT);
			}
			else if(inttime >= (CMOS_STD_INTTIME*5/2))
			{
				// 切换到夜晚模式
				arkn141_isp_set_ae_window_mode (AE_WINDOW_MODE_NIGHT);
			}
			break;
			
		case AE_WINDOW_MODE_BACKLIGHT:
			if(inttime > 48)
			{
				// 切换到正常模式
				arkn141_isp_set_ae_window_mode (AE_WINDOW_MODE_NORMAL);
			}
			break;
			
		case AE_WINDOW_MODE_NIGHT:
			if(inttime < CMOS_STD_INTTIME)
			{
				// 切换到正常模式
				arkn141_isp_set_ae_window_mode (AE_WINDOW_MODE_NORMAL);				
			}
			break;
	}
#endif
}

static void imx322_cmos_isp_set_day_night_mode (cmos_gain_ptr_t gain, int day_night)	// day_night = 1, 夜晚增强模式, day_night = 0, 普通模式
{
	return;
	if(day_night)
	{
		// 夜晚增强模式, 开启数字增益(信噪比较差, 噪点大)
		imx322_cmos_max_gain_set (&cmos_gain, 	15604, 1);
	}
	else
	{
		// 普通模式, 关闭数字增益
		imx322_cmos_max_gain_set (&cmos_gain, 	3078, 1);
	}
}


u32_t isp_init_cmos_sensor (cmos_sensor_t *cmos_sensor)
{
	memset (cmos_sensor, 0, sizeof(cmos_sensor_t));
	cmos_sensor->cmos_gain_initialize = cmos_gain_initialize;
	cmos_sensor->cmos_max_gain_set = imx322_cmos_max_gain_set;
	cmos_sensor->cmos_max_gain_get = imx322_cmos_max_gain_get;
	
	//cmos_sensor->cmos_gain_update = cmos_gain_update;
	cmos_sensor->cmos_inttime_initialize = cmos_inttime_initialize;
	//cmos_sensor->cmos_inttime_update = cmos_inttime_update;
	cmos_sensor->cmos_inttime_gain_update = cmos_inttime_gain_update;
	cmos_sensor->cmos_inttime_gain_update_manual = cmos_inttime_gain_update_manual;
	cmos_sensor->analog_gain_from_exposure_calculate = analog_gain_from_exposure_calculate;
	cmos_sensor->digital_gain_from_exposure_calculate = NULL;
	cmos_sensor->cmos_get_iso = cmos_get_iso;
	cmos_sensor->cmos_fps_set = cmos_fps_set;
	cmos_sensor->cmos_sensor_set_readout_direction = cmos_sensor_set_readout_direction;
	
	cmos_sensor->cmos_sensor_get_sensor_name = imx322_cmos_sensor_get_sensor_name;
	// sensor初始化
	cmos_sensor->cmos_isp_sensor_init = imx322_isp_sensor_init;
	
	cmos_sensor->cmos_isp_awb_init = imx322_cmos_isp_awb_init;
	cmos_sensor->cmos_isp_colors_init = imx322_cmos_isp_colors_init;
	cmos_sensor->cmos_isp_denoise_init = imx322_cmos_isp_denoise_init;
	cmos_sensor->cmos_isp_eris_init = imx322_cmos_isp_eris_init;
	cmos_sensor->cmos_isp_fesp_init = imx322_cmos_isp_fesp_init;
	cmos_sensor->cmos_isp_enhance_init = imx322_cmos_isp_enhance_init;
	cmos_sensor->cmos_isp_ae_init = imx322_cmos_isp_ae_init;
	cmos_sensor->cmos_isp_sys_init = imx322_cmos_isp_sys_init;

	cmos_sensor->cmos_isp_awb_run = imx322_cmos_isp_awb_run;
	cmos_sensor->cmos_isp_colors_run = imx322_cmos_isp_colors_run;
	cmos_sensor->cmos_isp_denoise_run = imx322_cmos_isp_denoise_run;
	cmos_sensor->cmos_isp_eris_run = imx322_cmos_isp_eris_run;
	cmos_sensor->cmos_isp_fesp_run = imx322_cmos_isp_fesp_run;
	cmos_sensor->cmos_isp_enhance_run = imx322_cmos_isp_enhance_run;
	cmos_sensor->cmos_isp_ae_run = imx322_cmos_isp_ae_run;
	
	cmos_sensor->cmos_isp_set_day_night_mode = imx322_cmos_isp_set_day_night_mode;
	
	return 0;
}

#endif	// ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX322