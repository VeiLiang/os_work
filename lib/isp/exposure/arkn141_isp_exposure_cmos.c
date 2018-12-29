#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arkn141_isp_sensor_cfg.h"
#include "arkn141_isp_exposure_cmos.h"
#include "arkn141_isp_exposure.h"
#include "rtos.h"
#include "xm_printf.h"
#include "xm_app_menudata.h"

extern cmos_exposure_t isp_exposure;

static void boundary_min_check_u16 (u16_t * param, u32_t min)
{
	if((*param) < min)
	{
		*param = (u16_t)min;
	}
}

static void boundary_max_check_u16 (u16_t * param, u32_t max)
{
	if((*param) > max)
	{
		*param = (u16_t)max;
	}
}

static void boundary_min_check (u32_t * param, u32_t min)
{
	if((*param) < min)
	{
		*param = min;
	}
}

static void boundary_max_check (u32_t * param, u32_t max)
{
	if((*param) > max)
	{
		*param = max;
	}
}

static void boundaries_check (u32_t * param, u32_t min, u32_t max)
{
	boundary_min_check(param, min);
	boundary_max_check(param, max);
}

u32_t exposure_limit_calculate(
	u16_t exp_lines,
	u32_t max_again,
	u32_t max_dgain,
	u16_t  shift,
	u16_t	gain_shift
	)
{
	i64_t exp = exp_lines * shift;
	exp *= max_again * max_dgain;
	exp = exp >> gain_shift;
	return (u32_t)exp;
}

// function limits the exposure to the nearest multiple of flicker half-period.
static u32_t exposure_antiflicker_correct (
		u32_t exposure_lines,
		u32_t flicker_freq,
		u32_t lines_per_500ms,
		u16_t	sys_factor
		)
{
	int N;
	if (flicker_freq == 0)
	{
		return exposure_lines;
	}
	N = (exposure_lines * flicker_freq) / (lines_per_500ms);
	N = N < 1 ? 1 : N;
	return (N * lines_per_500ms) / flicker_freq;
}

// 计算下一次的曝光量(积分时间/增益)
int isp_exposure_cmos_calculate (
		cmos_exposure_ptr_t p_exp,
		u32_t exp_increment,
		u32_t exp_increment_max,
		u32_t exp_increment_min,
		u32_t exp_increment_offset
		)
{
	unsigned int max_lines_short;
//	unsigned int max_lines_long;
	u32_t exposure_1st, exposure_2nd;
	u32_t new_exposure;
	u32_t old_exposure_ashort;
	i64_t temp64;
	int same_exp = 0;		// 检查理论曝光值是否相同
	
	int exp_dir = 0;		// 1 曝光量增加 0 曝光量不变 -1 曝光量减小
	
	old_exposure_ashort = p_exp->cmos_inttime.exposure_ashort;
//	max_lines_long  = p_exp->cmos_inttime.max_lines_target;
	max_lines_short = p_exp->cmos_inttime.max_lines_target;
		
	// 根据直方图误差调整理论曝光量 exposure
	// new_exposure的计算会出现32位溢出
	// new_exposure = (p_exp->exposure * exp_increment + (exp_increment_offset >> 1)) / exp_increment_offset;
	temp64 = p_exp->exposure;
	temp64 *= exp_increment;
	//temp64 += (exp_increment_offset >> 1);		// 不做四舍五入处理, 避免曝光量误差过大导致画面亮度轻微抖动
	new_exposure = (u32_t)(temp64 / exp_increment_offset);
//	p_exp->last_exposure = p_exp->exposure;
	
	/*
	XM_printf ("\t\texposure = %10d, new_exposure = %10d, increment = %10d\n", 
			p_exp->exposure,
			new_exposure,
			exp_increment);
	*/		
	if(p_exp->exposure == new_exposure)
	{
		same_exp = 1;
		exp_dir = 0;
	}
	else if(p_exp->exposure < new_exposure)
	{
		// 曝光量增加
		exp_dir = 1;
	}
	else
	{
		// 曝光量减小
		exp_dir = -1;
	}
	
	p_exp->exposure = new_exposure;
	
	// 检查曝光量的范围
	boundaries_check (
		&p_exp->exposure,
		p_exp->exp_llimit,
		p_exp->exp_tlimit
		);
		
	exposure_1st = p_exp->exposure;
	exposure_2nd = p_exp->exposure;
	
	// the exposure/gain is allocated as following:
	// 1) analog gain
	// 2) digital gain
	// 3) exposure time
	// 4) antiflicker correction (if enabled) -> setting exposure times to finite s
	// 5) adjust again/dgain if necessary
	
	// 检查sensor是否具有模拟增益.
	// 若有, 计算sensor的模拟增益
	if(1 != p_exp->cmos_gain.max_again_target)
	{
		// 根据曝光量exposure_1st, 计算模拟增益again, 然后修正exposure_1st
		exposure_1st = (*p_exp->cmos_sensor.analog_gain_from_exposure_calculate) (
			&p_exp->cmos_gain,
			exposure_1st,
			p_exp->cmos_inttime.max_flicker_lines * p_exp->sys_factor
			);
	}
	
	// 检查sensor是否具有数字增益.
	// 若有, 计算sensor的数字增益
	if(1 != p_exp->cmos_gain.max_dgain_target)
	{
		// 根据曝光量exposure_1st, 计算数字增益again, 然后修正exposure_1st
		exposure_1st = (*p_exp->cmos_sensor.digital_gain_from_exposure_calculate) (
			&p_exp->cmos_gain,
			exposure_1st,		// 曝光量
			p_exp->cmos_inttime.max_flicker_lines * p_exp->sys_factor	// 最大积分行数量
			);
	}
	
	// 根据估计的增益值(模拟+数字), 计算所需的曝光积分行数
	//p_exp->cmos_inttime.exposure_ashort = ((exposure_1st + p_exp->sys_factor - 1) / p_exp->sys_factor);
	if(p_exp->cmos_inttime.flicker_freq)		// 频闪时, 计算顺序为 1) 曝光行, 2) 模拟增益 3) 数字增益
		p_exp->cmos_inttime.exposure_ashort = ((exposure_2nd ) / p_exp->sys_factor);
	else
		p_exp->cmos_inttime.exposure_ashort = ((exposure_1st ) / p_exp->sys_factor);		// 禁止四舍五入, 会导致exposure_ashort * sys_factor大于exposure_2nd,
																												//		这样第二次校正时不会进行模拟增益校正,从而导致曝光不稳,出现闪烁.
	//if(p_exp->cmos_inttime.exposure_ashort >= p_exp->cmos_inttime.max_flicker_lines)
	//	p_exp->cmos_inttime.exposure_ashort = p_exp->cmos_inttime.max_flicker_lines;
	if(p_exp->cmos_inttime.exposure_ashort >= p_exp->cmos_inttime.max_lines)
		p_exp->cmos_inttime.exposure_ashort = p_exp->cmos_inttime.max_lines;
	
	if(p_exp->cmos_inttime.exposure_ashort > max_lines_short) 
		p_exp->cmos_inttime.exposure_ashort = max_lines_short;
	
	// anti-flicker 校正
	if(p_exp->cmos_inttime.flicker_freq)
	{
		// 消除50/60hz频闪会导致室外白天场景(不需要消除频闪)下的曝光行计算不准确.
		// 由于无法判断50/60hz频闪场景, 又需要保证白天曝光的准确性, 
		// 通过判断总曝光量为最大行曝光量的2倍时, 开启50/60hz频闪消除算法
		
		// 20170806 由于无法准确判断频闪(缺乏硬件支持), 且频闪使能判定算法不可靠, 因此频闪使能时不再考虑曝光是否过曝等问题,
		//		由使用者决定开启(50/60Hz)或关闭频闪功能
		u32_t new_exposure_ashort = exposure_antiflicker_correct(
				p_exp->cmos_inttime.exposure_ashort,
				p_exp->cmos_inttime.flicker_freq,
				p_exp->cmos_inttime.lines_per_500ms,
				p_exp->sys_factor
				);
		// 以下算法为防止反复导致频闪
		// new_exposure    inttime    again
		// 184434	        364	      1.979245364	
		// 190197	        728	      1.020545373	   曝光剧烈变化点
		// 181495	        364	      1.947705615	   曝光反复点, 维持
		// 140651	        364	      1.509390024		曝光反复点
		if(new_exposure_ashort == old_exposure_ashort)
		{
			p_exp->cmos_inttime.exposure_ashort = new_exposure_ashort;
		}
		else if(exp_dir == (-1))
		{
			// 曝光量减小
			//double gain = ((double)exposure_2nd) / p_exp->sys_factor;
			//gain = gain / new_exposure_ashort;
			//if(gain >= 1.75 && gain < 2.0) 
			u32_t igain = exposure_2nd / new_exposure_ashort;
			if( igain >= (p_exp->sys_factor * 6/5) && igain < (p_exp->sys_factor * 2) )
			{
				// 1.75 ~ 2.00 维持,使用老的inttime
				p_exp->cmos_inttime.exposure_ashort = old_exposure_ashort;
			}
			else
			{
				// < 1.75 或者 >= 2.00, 使用新的inttime
				p_exp->cmos_inttime.exposure_ashort = new_exposure_ashort;
				printf ("dec old=%d, new=%d\n", old_exposure_ashort, new_exposure_ashort);
			}
		}
		else
		{
			// 曝光量增加
			// 新的曝光行计数必须大于老的曝光行计数
			if(new_exposure_ashort > old_exposure_ashort)
			{
				p_exp->cmos_inttime.exposure_ashort = new_exposure_ashort;
				printf ("inc old=%d, new=%d\n", old_exposure_ashort, new_exposure_ashort);
			}
			else
			{
				p_exp->cmos_inttime.exposure_ashort = old_exposure_ashort;
			}
		}
				  
		/*		  
		p_exp->cmos_inttime.exposure_ashort = exposure_antiflicker_correct(
				p_exp->cmos_inttime.exposure_ashort,
				p_exp->cmos_inttime.flicker_freq,
				p_exp->cmos_inttime.lines_per_500ms,
				p_exp->sys_factor
				);
		*/
	}
	
	
	// 根据确定的曝光行数第二遍校正模拟及数字增益
	if(1 != p_exp->cmos_gain.max_again_target)
	{
		exposure_2nd = (*p_exp->cmos_sensor.analog_gain_from_exposure_calculate) (
			&p_exp->cmos_gain,
			exposure_2nd,
			(p_exp->cmos_inttime.exposure_ashort * p_exp->sys_factor)
			);
	}
	
	if(1 != p_exp->cmos_gain.max_dgain_target)
	{
		exposure_2nd = (*p_exp->cmos_sensor.digital_gain_from_exposure_calculate) (
			&p_exp->cmos_gain,
			exposure_2nd,
			(p_exp->cmos_inttime.exposure_ashort * p_exp->sys_factor)
			);
	}
	
	return same_exp;
}

// 根据计算的曝光设置更新sensor设置
void isp_cmos_inttime_update (cmos_exposure_ptr_t p_exp)
{
	// 修改曝光行积分数量/模拟/数字增益
	p_exp->cmos_sensor.cmos_inttime_gain_update (&p_exp->cmos_inttime, &p_exp->cmos_gain);
}

void isp_cmos_inttime_update_manual (cmos_exposure_ptr_t p_exp)
{
	// 修改曝光行积分数量/模拟/数字增益
	p_exp->cmos_sensor.cmos_inttime_gain_update_manual (&p_exp->cmos_inttime, &p_exp->cmos_gain);
}


// 设置帧率
int  isp_cmos_set_framerate (cmos_exposure_ptr_t p_exp, u8_t fps)
{
	cmos_inttime_ptr_t inttime;
	if(!p_exp->cmos_sensor.cmos_fps_set)
	{
		XM_printf ("cmos_fps_set can't be empty\n");
		return -1;
	}
	(*p_exp->cmos_sensor.cmos_fps_set) (&p_exp->cmos_inttime, fps);
	p_exp->fps = fps;

	inttime = &p_exp->cmos_inttime;
	boundary_min_check_u16(&inttime->min_lines_target, 1);
	inttime->min_lines = inttime->min_lines_target;
	inttime->max_lines_target = (u16_t)(inttime->full_lines - 2);
	inttime->max_lines = inttime->max_lines_target;
	if(inttime->max_lines > inttime->full_lines_limit)
		inttime->max_lines = inttime->full_lines_limit;

	// 更新flicker设置
	inttime->max_flicker_lines = (u16_t)exposure_antiflicker_correct(
		inttime->max_lines,
		inttime->flicker_freq,
		inttime->lines_per_500ms,
		p_exp->sys_factor
		);
	// 最小曝光量不考虑周期性flicker现象
	inttime->min_flicker_lines = inttime->min_lines;

	return 0;
}

// 使能/禁止flicker
int isp_cmos_set_flicker_freq (cmos_exposure_ptr_t p_exp, u8_t flicker_freq)
{
#if HYBRID_H264_MJPG || ISP_RAW_ENABLE
	flicker_freq = 0;
#endif
	if(	flicker_freq != 0 
		&& flicker_freq != 50
		&& flicker_freq != 60)
	{
		XM_printf ("illegal flicker freqency (%d)\n", flicker_freq);
		return -1;
	}
	
	OS_EnterRegion ();

	p_exp->cmos_inttime.flicker_freq = flicker_freq;

	// 更新flicker设置
	// max_flicker_lines 小于或等于 p_exp->cmos_inttime.max_lines
	p_exp->cmos_inttime.max_flicker_lines = (u16_t)exposure_antiflicker_correct(
		p_exp->cmos_inttime.max_lines,
		p_exp->cmos_inttime.flicker_freq,
		p_exp->cmos_inttime.lines_per_500ms,
		p_exp->sys_factor
		);
	// 最小曝光量不考虑周期性flicker现象
	p_exp->cmos_inttime.min_flicker_lines = p_exp->cmos_inttime.min_lines;
	
	OS_LeaveRegion();
	
	return 0;
}

int isp_exposure_cmos_initialize (cmos_exposure_ptr_t p_exp)
{
	cmos_gain_ptr_t gain;
	cmos_inttime_ptr_t inttime;
	unsigned int light_freq;

	isp_init_cmos_sensor (&p_exp->cmos_sensor);
	if(!p_exp->cmos_sensor.cmos_gain_initialize || !p_exp->cmos_sensor.cmos_inttime_initialize)
	{
		XM_printf ("the sensor's cmos_gain_initialize or cmos_inttime_initialize can't be empty\n");
		return -1;
	}
	
	// gain 初始化
	gain = (*p_exp->cmos_sensor.cmos_gain_initialize) ();
	if(!gain)
	{
		XM_printf ("the gain instance can't be empty\n");
		return -1;
	}
	memcpy (&p_exp->cmos_gain, gain, sizeof(p_exp->cmos_gain));
	gain = &p_exp->cmos_gain;
	// 缺省增益设置为1
	gain->again = 1 << gain->again_shift;
	gain->dgain = 1 << gain->dgain_shift;
	gain->iso = 100;
	gain->max_again = gain->max_again_target;
	gain->max_dgain = gain->max_dgain_target;

	// 曝光积分行数初始化
	inttime = (*p_exp->cmos_sensor.cmos_inttime_initialize)();
	if(!inttime)
	{
		XM_printf ("the inttime instance can't be empty\n");
		return -1;
	}
	memcpy (&p_exp->cmos_inttime, inttime, sizeof(p_exp->cmos_inttime));
#if DVR_H264
	p_exp->cmos_inttime.flicker_freq = 50;		// 缺省禁止flicker
#else
	p_exp->cmos_inttime.flicker_freq = 0;		// 缺省禁止flicker
#endif

	/*	
#if ISP_STARTUP_TEST
	p_exp->cmos_inttime.exposure_ashort = 0;
#else
	p_exp->cmos_inttime.exposure_ashort = 32;
#endif
	*/

	// 检查inttime参数

	// 设置sensor的初始参数(积分时间及增益设置)
	isp_cmos_inttime_update (p_exp);

	// 缺省帧率 30帧
	isp_cmos_set_framerate (p_exp, 30);
	
	light_freq = AP_GetMenuItem (APPMENUITEM_LIGHT_FREQ);
	if(light_freq == AP_SETTING_LIGHT_FREQ_50HZ)
		isp_cmos_set_flicker_freq (p_exp, 50);
	else if(light_freq == AP_SETTING_LIGHT_FREQ_60HZ)
		isp_cmos_set_flicker_freq (p_exp, 60);
	else //if(light_freq == AP_SETTING_LIGHT_FREQ_OFF)
		isp_cmos_set_flicker_freq (p_exp, 0);
		
	// 估计一个初始曝光值
	p_exp->sys_factor = 256;//128;	//64;
	p_exp->exposure = p_exp->cmos_inttime.exposure_ashort * p_exp->sys_factor;

	// 最大曝光量估算
	p_exp->exp_tlimit = exposure_limit_calculate(
				p_exp->cmos_inttime.max_flicker_lines,
		   	p_exp->cmos_gain.max_again,
				p_exp->cmos_gain.max_dgain,
				(u16_t)p_exp->sys_factor,
				(u16_t)(p_exp->cmos_gain.again_shift + p_exp->cmos_gain.dgain_shift)
				);
		
	// 最小曝光量估算
	p_exp->exp_llimit = exposure_limit_calculate(
				p_exp->cmos_inttime.min_flicker_lines,
		   	1,
				1,
				(u16_t)p_exp->sys_factor,
				0
				);

	p_exp->physical_exposure = 0;

	return 0;
}


void isp_min_integration_time_set (cmos_exposure_ptr_t p_exp, u16_t value)
{
	p_exp->cmos_inttime.min_lines_target = value;
}

void isp_max_integration_time_set (cmos_exposure_ptr_t p_exp, u16_t value)
{
	p_exp->cmos_inttime.max_lines_target = value;
}


//#define	AE_WINDOW_454575676	1		// 4 5 4 5  7 5 6 7 6

#if DVR_H264 == 1
#define AE_WINDOW_111111111		1
#else
// 20170120上午
#define	AE_WINDOW_4545A5686	1		// 4 5 4 5 10 5 6 8 6
#endif


//#define	AE_WINDOW_4545F56D6	1		// 4, 5, 4, 5, 15, 5, 6, 13,6
//#define	AE_WINDOW_3434F45D5	1		// 3, 4, 3, 4, 15, 4, 5, 13, 5

//#define	AE_10		1

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_BG0806
#define	AE_2		1		// 增加曝光量, 同时降低eris增益
#else
#define	AE_9		1
#endif

// 20170120下午, 使用AE_8, 曝光稍微有点不够, 某些场景偏暗
//#define	AE_8		1

// 20170120上午, 使用AE_7, 曝光稍微有点过度(高层建筑顶部发白), 隧道基本与第一现场效果一致, 降低补偿
//#define	AE_7	1


//#define	AE_6	1

//#define	AE_2	1
//#define	AE_3	1
//#define	AE_4	1
//#define	AE_5	1
// 初始化cmos sensor的实例
int arkn141_isp_ae_initialize (cmos_exposure_ptr_t p_exp)
{
	u8_t hist_thresh[4] = {0x10, 0x40, 0x80, 0xc0};
#if AE_WINDOW_111111111
	u8_t win_weight[3][3] = {1, 1, 1, 1,  1, 1, 1, 1, 1};
#elif AE_WINDOW_222121111
	u8_t win_weight[3][3] = {2, 2, 2, 1,  2, 1, 1, 1, 1};
#elif AE_WINDOW_454575676
	u8_t win_weight[3][3] = {4, 5, 4, 5,  7, 5, 6, 7, 6};		// 兼顾夜晚车牌(底部)及隧道(中间)
#elif AE_WINDOW_4545A5686
	u8_t win_weight[3][3] = {4, 5, 4, 5, 10, 5, 6, 8, 6};		// 强化隧道模式, 兼顾夜晚车牌(底部)
																				//		比较 AE_WINDOW_454575676, 更快进入及退出隧道
#elif AE_WINDOW_4545F56D6
	u8_t win_weight[3][3] = {4, 5, 4, 5, 15, 5, 6, 13,6};		// 蜕化隧道模式, 兼顾夜晚车牌(底部)
#elif AE_WINDOW_3434F45D5
	u8_t win_weight[3][3] = {3, 4, 3, 4, 15, 4, 5, 13,5};		// 蜕化隧道模式, 兼顾夜晚车牌(底部)		
																				//		问题, 会导致周边区域存在过曝的现象
#else
#error	please define AE_WINDOW_XXXXXXXXX
#endif

	isp_auto_exposure_initialize (&(p_exp->cmos_ae));

	isp_exposure_cmos_initialize (p_exp);

	isp_min_integration_time_set (p_exp, ISP_SYSTEM_MIN_INTEGRATION_TIME_DEFAULT);
	isp_max_integration_time_set (p_exp, ISP_SYSTEM_MAX_INTEGRATION_TIME_DEFAULT);

	isp_histogram_thresh_write (&p_exp->cmos_ae, hist_thresh);
	//isp_ae_window_weight_write (&p_exp->cmos_ae, win_weight);
	arkn141_isp_set_ae_window_mode (AE_WINDOW_MODE_NORMAL);

	isp_histogram_thread_update (p_exp);

	isp_system_ae_bright_target_write (&p_exp->cmos_ae, AE_BRIGHT_TARGET_DEFAULT);
	isp_system_ae_black_target_write  (&p_exp->cmos_ae, 128);	
	isp_system_ae_compensation_write (&p_exp->cmos_ae, AE_COMPENSATION_DEFAULT);

	isp_auto_exposure_compensation (&p_exp->cmos_ae, p_exp->cmos_ae.histogram.bands);

	return 0;
}

int arkn141_isp_ae_run (cmos_exposure_ptr_t p_exp)
{
	int ret = isp_auto_exposure_run (&p_exp->cmos_ae, p_exp, 0);
	return ret;
}

int arkn141_isp_set_sensor_readout_direction (cmos_exposure_ptr_t p_exp, 
															 unsigned int horz_reverse_direction, unsigned int vert_reverse_direction)
{
	if(p_exp && p_exp->cmos_sensor.cmos_sensor_set_readout_direction)
	{
		return (*p_exp->cmos_sensor.cmos_sensor_set_readout_direction) (horz_reverse_direction, vert_reverse_direction);
	}
	else
		return (-1);
}

unsigned int cmos_calc_inttime_gain (cmos_exposure_t *p_isp_exposure)
{
	XMINT64 inttime_gain;
	inttime_gain = p_isp_exposure->cmos_inttime.exposure_ashort;
	inttime_gain *= p_isp_exposure->cmos_gain.again;
	inttime_gain >>= p_isp_exposure->cmos_gain.again_shift;
	inttime_gain *= p_isp_exposure->cmos_gain.dgain;
	inttime_gain >>= p_isp_exposure->cmos_gain.dgain_shift;
	return (unsigned int)inttime_gain;
}


// 逆光
static u8_t ae_win_weight_backlight[3][3] = 
	{3, 4, 3, 6, 15, 6, 12, 15, 12};		// 

// 正常
static u8_t ae_win_weight_normal[3][3] = 
	{4, 5, 4, 5, 10, 5, 6, 8, 6};			// 强化隧道模式, 兼顾夜晚车牌(底部)

// 夜晚
static u8_t ae_win_weight_night[3][3] = 
	{3, 3, 3, 5, 12, 5, 12, 13, 12};		// 夜晚车牌/招牌

static unsigned int ae_window_mode = AE_WINDOW_MODE_NORMAL;

void arkn141_isp_set_ae_window_mode (unsigned int window_mode)
{
	if(window_mode == AE_WINDOW_MODE_BACKLIGHT)
	{
		// 逆光处理
		ae_window_mode = window_mode;
		isp_ae_window_weight_write (&isp_exposure.cmos_ae, ae_win_weight_backlight);
		XM_printf ("AW_BACKLIGHT\n");
	}		
	else if(window_mode == AE_WINDOW_MODE_NORMAL)
	{
		// 正常模式
		ae_window_mode = window_mode;
		isp_ae_window_weight_write (&isp_exposure.cmos_ae, ae_win_weight_normal);	
		XM_printf ("AW_NORMAL\n");
	}
	else if(window_mode == AE_WINDOW_MODE_NIGHT)
	{
		// 夜晚, 侧重车牌/指示牌
		ae_window_mode = window_mode;
		isp_ae_window_weight_write (&isp_exposure.cmos_ae, ae_win_weight_night);	
		XM_printf ("AW_NIGHT\n");
	}
}

unsigned int arkn141_isp_get_ae_window_mode (void)
{
	return ae_window_mode;
}

// 逆光, 保留暗处细节
const unsigned short gamma_LUT_0[] = {    
	0, 	    	1023, 		2047, 		3071, 		4095, 		5119, 		6143,   		7167, 	
	8191, 		9215, 		10239, 		11263, 		12287,  		13311, 		14335, 		15359, 
	16383, 		17407, 		18431, 		19455, 		20479, 		21503, 		22527, 		23551, 	
	24575, 		25599, 		26623, 		27647, 		28671, 		29695, 		30719, 		31743, 	
   32256,      34644,      36464,      38228,      39936,      41588,      43184,      44724,   
   46208,      47636,      49008,      50324,      51584,      52788,      53936,      55028,   
   56064,      57044,      57968,      58836,      59648,      60404,      61104,      61748,   
   62336,      62868,      63344,      63764,      64128,      64436,      64688,      64884,   
   65024 
};

// 普通场景, 高对比度
const unsigned short gamma_LUT_1[] = {     
	0,        	140,        336,        588,        896,       1260,       	1680,       2156,   
   2688,       3276,       3920,       4620,       5376,       6188,       7056,       7980,  
   8960,       9996,      	11088,      12236,      13440,      14700,      16016,      17388,   
   18816,      20300,      21840,      23436,      25088,      26796,      28560,      30380,   
   32256,      34644,      36464,      38228,      39936,      41588,      43184,      44724,   
   46208,      47636,      49008,      50324,      51584,      52788,      53936,      55028,   
   56064,      57044,      57968,      58836,      59648,      60404,      61104,      61748,   
   62336,      62868,      63344,      63764,      64128,      64436,      64688,      64884,   
   65024 
};

// 夜晚场景, 保留低处细节, 抑制高光溢出
const unsigned short gamma_LUT_2[65] = {
	0, 	   1023, 	2047, 	3071, 	4095, 	5119, 	6143,   7167, 	
	8191, 	9215, 	10239, 	11263, 	12287,  	13311, 	14335, 	15359, 
	16383, 	17407, 	18431, 	19455, 	20479, 	21503, 	22527, 	23551, 	
	24575, 	25599, 	26623, 	27647, 	28671, 	29695, 	30719, 	31743, 	
	32767, 	33791, 	34815, 	35839, 	36863, 	37887, 	38911, 	39935, 	
	40959, 	41983, 	43007, 	44031, 	45055, 	46079, 	47103, 	48127, 	
	49151, 	50175, 	51199, 	52223, 	53247, 	54271, 	55295, 	56319, 	
	57343, 	58367, 	59391, 	60415, 	61439, 	62463, 	63487, 	64511, 	
	65535, 
};

static isp_gamma_polyline_tbl gamma_polyline_tbl[] = {
	// 提升场景的对比度
#if 0
	{	32,					
		{    
			0, 	    	1023, 		2047, 		3071, 		4095, 		5119, 		6143,   		7167, 	
			8191, 		9215, 		10239, 		11263, 		12287,  		13311, 		14335, 		15359, 
			16383, 		17407, 		18431, 		19455, 		20479, 		21503, 		22527, 		23551, 	
			24575, 		25599, 		26623, 		27647, 		28671, 		29695, 		30719, 		31743, 	
			32256,      34644,      36464,      38228,      39936,      41588,      43184,      44724,   
			46208,      47636,      49008,      50324,      51584,      52788,      53936,      55028,   
			56064,      57044,      57968,      58836,      59648,      60404,      61104,      61748,   
			62336,      62868,      63344,      63764,      64128,      64436,      64688,      64884,   
			65024 
		}
	},
#endif
	{	128,					
		{
			//0,        	140,        336,        588,        896,       1260,       	1680,       2156,  
			0,        	256,        336,        588,        896,       1260,       	1680,       2156,   
			2688,       3276,       3920,       4620,       5376,       6188,       7056,       7980,  
			8960,       9996,      	11088,      12236,      13440,      14700,      16016,      17388,   
			18816,      20300,      21840,      23436,      25088,      26796,      28560,      30380,   
			32256,      34644,      36464,      38228,      39936,      41588,      43184,      44724,   
			46208,      47636,      49008,      50324,      51584,      52788,      53936,      55028,   
			56064,      57044,      57968,      58836,      59648,      60404,      61104,      61748,   
			62336,      62868,      63344,      63764,      64128,      64436,      64688,      64884,   
			65024 
		}
	},
	{	800,					
		{
			0,        	256,        336,        588,        896,       1260,       	1680,       2156,   
			2688,       3276,       3920,       4620,       5376,       6188,       7056,       7980,  
			8960,       9996,      	11088,      12236,      13440,      14700,      16016,      17388,   
			18816,      20300,      21840,      23436,      25088,      26796,      28560,      30380,   
			32256,      34644,      36464,      38228,      39936,      41588,      43184,      44724,   
			46208,      47636,      49008,      50324,      51584,      52788,      53936,      55028,   
			56064,      57044,      57968,      58836,      59648,      60404,      61104,      61748,   
			62336,      62868,      63344,      63764,      64128,      64436,      64688,      64884,   
			65024 
				
				
		}
	},
	{	1212,					
		{
			0,        	256,        336,        588,        896,       1260,       	1680,       2156,   
			2688,       3276,       3920,       4620,       5376,       6188,       7056,       7980,  
			8960,       9996,      	11088,      12236,      13440,      14700,      16016,      17388,   
			18816,      20300,      21840,      23436,      25088,      26796,      28560,      30380,   
			//32256,      34644,      36464,      38228,      39936,      41588,      43184,      44724,   
			//46208,      47636,      49008,      50324,      51584,      52788,      53936,      55028,   
			//56064,      57044,      57968,      58836,      59648,      60404,      61104,      61748,   
			//62336,      62868,      63344,      63764,      64128,      64436,      64688,      64884,   
			//65024 
			32767, 	33791, 	34815, 	35839, 	36863, 	37887, 	38911, 	39935, 	
			40959, 	41983, 	43007, 	44031, 	45055, 	46079, 	47103, 	48127, 	
			49151, 	50175, 	51199, 	52223, 	53247, 	54271, 	55295, 	56319, 	
			57343, 	58367, 	59391, 	60415, 	61439, 	62463, 	63487, 	64511, 	
			65535, 
				
		}
	},
	{	1212 * 3,			
		{
			0, 	   1023, 	2047, 	3071, 	4095, 	5119, 	6143,   7167, 	
			8191, 	9215, 	10239, 	11263, 	12287,  	13311, 	14335, 	15359, 
			16383, 	17407, 	18431, 	19455, 	20479, 	21503, 	22527, 	23551, 	
			24575, 	25599, 	26623, 	27647, 	28671, 	29695, 	30719, 	31743, 	
			32767, 	33791, 	34815, 	35839, 	36863, 	37887, 	38911, 	39935, 	
			40959, 	41983, 	43007, 	44031, 	45055, 	46079, 	47103, 	48127, 	
			49151, 	50175, 	51199, 	52223, 	53247, 	54271, 	55295, 	56319, 	
			57343, 	58367, 	59391, 	60415, 	61439, 	62463, 	63487, 	64511, 	
			65535, 
		}
	}
};


void isp_ae_gamma_match (int inttime, isp_gamma_polyline_tbl *gamma_tbl)
{
	int i;
	const isp_gamma_polyline_tbl *lo, *hi;
	int count = sizeof(gamma_polyline_tbl)/sizeof(gamma_polyline_tbl[0]);
	gamma_tbl->inttime = inttime;
	for (i = 0; i < count; i ++)
	{
		if(inttime <= gamma_polyline_tbl[i].inttime)
			break;
	}
	// 匹配
	if(inttime == gamma_polyline_tbl[i].inttime)
	{
		memcpy (gamma_tbl->gamma_lut, gamma_polyline_tbl[i].gamma_lut, sizeof(int) * 65);
	}
	// 边界
	else if(inttime < gamma_polyline_tbl[0].inttime)
	{
		memcpy (gamma_tbl->gamma_lut, gamma_polyline_tbl[0].gamma_lut, sizeof(int) * 65);
	}
	else if(i == count)
	{
		memcpy (gamma_tbl->gamma_lut, gamma_polyline_tbl[count - 1].gamma_lut, sizeof(int) * 65);
	}
	else
	{
		lo = &gamma_polyline_tbl[i - 1];
		hi = &gamma_polyline_tbl[i];
		for (i = 0; i < 65; i ++)
		{
			int w = lo->gamma_lut[i] + (hi->gamma_lut[i] - lo->gamma_lut[i]) * (inttime - lo->inttime) / (hi->inttime - lo->inttime);
			if(w < 0)
				w = 0;
			else if(w > 65535)
			{
				w = 65535;
			}
			gamma_tbl->gamma_lut[i] = w;	
		}
	}	
}