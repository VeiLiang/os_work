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

// ������һ�ε��ع���(����ʱ��/����)
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
	int same_exp = 0;		// ��������ع�ֵ�Ƿ���ͬ
	
	int exp_dir = 0;		// 1 �ع������� 0 �ع������� -1 �ع�����С
	
	old_exposure_ashort = p_exp->cmos_inttime.exposure_ashort;
//	max_lines_long  = p_exp->cmos_inttime.max_lines_target;
	max_lines_short = p_exp->cmos_inttime.max_lines_target;
		
	// ����ֱ��ͼ�����������ع��� exposure
	// new_exposure�ļ�������32λ���
	// new_exposure = (p_exp->exposure * exp_increment + (exp_increment_offset >> 1)) / exp_increment_offset;
	temp64 = p_exp->exposure;
	temp64 *= exp_increment;
	//temp64 += (exp_increment_offset >> 1);		// �����������봦��, �����ع����������»���������΢����
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
		// �ع�������
		exp_dir = 1;
	}
	else
	{
		// �ع�����С
		exp_dir = -1;
	}
	
	p_exp->exposure = new_exposure;
	
	// ����ع����ķ�Χ
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
	
	// ���sensor�Ƿ����ģ������.
	// ����, ����sensor��ģ������
	if(1 != p_exp->cmos_gain.max_again_target)
	{
		// �����ع���exposure_1st, ����ģ������again, Ȼ������exposure_1st
		exposure_1st = (*p_exp->cmos_sensor.analog_gain_from_exposure_calculate) (
			&p_exp->cmos_gain,
			exposure_1st,
			p_exp->cmos_inttime.max_flicker_lines * p_exp->sys_factor
			);
	}
	
	// ���sensor�Ƿ������������.
	// ����, ����sensor����������
	if(1 != p_exp->cmos_gain.max_dgain_target)
	{
		// �����ع���exposure_1st, ������������again, Ȼ������exposure_1st
		exposure_1st = (*p_exp->cmos_sensor.digital_gain_from_exposure_calculate) (
			&p_exp->cmos_gain,
			exposure_1st,		// �ع���
			p_exp->cmos_inttime.max_flicker_lines * p_exp->sys_factor	// ������������
			);
	}
	
	// ���ݹ��Ƶ�����ֵ(ģ��+����), ����������ع��������
	//p_exp->cmos_inttime.exposure_ashort = ((exposure_1st + p_exp->sys_factor - 1) / p_exp->sys_factor);
	if(p_exp->cmos_inttime.flicker_freq)		// Ƶ��ʱ, ����˳��Ϊ 1) �ع���, 2) ģ������ 3) ��������
		p_exp->cmos_inttime.exposure_ashort = ((exposure_2nd ) / p_exp->sys_factor);
	else
		p_exp->cmos_inttime.exposure_ashort = ((exposure_1st ) / p_exp->sys_factor);		// ��ֹ��������, �ᵼ��exposure_ashort * sys_factor����exposure_2nd,
																												//		�����ڶ���У��ʱ�������ģ������У��,�Ӷ������عⲻ��,������˸.
	//if(p_exp->cmos_inttime.exposure_ashort >= p_exp->cmos_inttime.max_flicker_lines)
	//	p_exp->cmos_inttime.exposure_ashort = p_exp->cmos_inttime.max_flicker_lines;
	if(p_exp->cmos_inttime.exposure_ashort >= p_exp->cmos_inttime.max_lines)
		p_exp->cmos_inttime.exposure_ashort = p_exp->cmos_inttime.max_lines;
	
	if(p_exp->cmos_inttime.exposure_ashort > max_lines_short) 
		p_exp->cmos_inttime.exposure_ashort = max_lines_short;
	
	// anti-flicker У��
	if(p_exp->cmos_inttime.flicker_freq)
	{
		// ����50/60hzƵ���ᵼ��������쳡��(����Ҫ����Ƶ��)�µ��ع��м��㲻׼ȷ.
		// �����޷��ж�50/60hzƵ������, ����Ҫ��֤�����ع��׼ȷ��, 
		// ͨ���ж����ع���Ϊ������ع�����2��ʱ, ����50/60hzƵ�������㷨
		
		// 20170806 �����޷�׼ȷ�ж�Ƶ��(ȱ��Ӳ��֧��), ��Ƶ��ʹ���ж��㷨���ɿ�, ���Ƶ��ʹ��ʱ���ٿ����ع��Ƿ���ص�����,
		//		��ʹ���߾�������(50/60Hz)��ر�Ƶ������
		u32_t new_exposure_ashort = exposure_antiflicker_correct(
				p_exp->cmos_inttime.exposure_ashort,
				p_exp->cmos_inttime.flicker_freq,
				p_exp->cmos_inttime.lines_per_500ms,
				p_exp->sys_factor
				);
		// �����㷨Ϊ��ֹ��������Ƶ��
		// new_exposure    inttime    again
		// 184434	        364	      1.979245364	
		// 190197	        728	      1.020545373	   �ع���ұ仯��
		// 181495	        364	      1.947705615	   �عⷴ����, ά��
		// 140651	        364	      1.509390024		�عⷴ����
		if(new_exposure_ashort == old_exposure_ashort)
		{
			p_exp->cmos_inttime.exposure_ashort = new_exposure_ashort;
		}
		else if(exp_dir == (-1))
		{
			// �ع�����С
			//double gain = ((double)exposure_2nd) / p_exp->sys_factor;
			//gain = gain / new_exposure_ashort;
			//if(gain >= 1.75 && gain < 2.0) 
			u32_t igain = exposure_2nd / new_exposure_ashort;
			if( igain >= (p_exp->sys_factor * 6/5) && igain < (p_exp->sys_factor * 2) )
			{
				// 1.75 ~ 2.00 ά��,ʹ���ϵ�inttime
				p_exp->cmos_inttime.exposure_ashort = old_exposure_ashort;
			}
			else
			{
				// < 1.75 ���� >= 2.00, ʹ���µ�inttime
				p_exp->cmos_inttime.exposure_ashort = new_exposure_ashort;
				printf ("dec old=%d, new=%d\n", old_exposure_ashort, new_exposure_ashort);
			}
		}
		else
		{
			// �ع�������
			// �µ��ع��м�����������ϵ��ع��м���
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
	
	
	// ����ȷ�����ع������ڶ���У��ģ�⼰��������
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

// ���ݼ�����ع����ø���sensor����
void isp_cmos_inttime_update (cmos_exposure_ptr_t p_exp)
{
	// �޸��ع��л�������/ģ��/��������
	p_exp->cmos_sensor.cmos_inttime_gain_update (&p_exp->cmos_inttime, &p_exp->cmos_gain);
}

void isp_cmos_inttime_update_manual (cmos_exposure_ptr_t p_exp)
{
	// �޸��ع��л�������/ģ��/��������
	p_exp->cmos_sensor.cmos_inttime_gain_update_manual (&p_exp->cmos_inttime, &p_exp->cmos_gain);
}


// ����֡��
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

	// ����flicker����
	inttime->max_flicker_lines = (u16_t)exposure_antiflicker_correct(
		inttime->max_lines,
		inttime->flicker_freq,
		inttime->lines_per_500ms,
		p_exp->sys_factor
		);
	// ��С�ع���������������flicker����
	inttime->min_flicker_lines = inttime->min_lines;

	return 0;
}

// ʹ��/��ֹflicker
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

	// ����flicker����
	// max_flicker_lines С�ڻ���� p_exp->cmos_inttime.max_lines
	p_exp->cmos_inttime.max_flicker_lines = (u16_t)exposure_antiflicker_correct(
		p_exp->cmos_inttime.max_lines,
		p_exp->cmos_inttime.flicker_freq,
		p_exp->cmos_inttime.lines_per_500ms,
		p_exp->sys_factor
		);
	// ��С�ع���������������flicker����
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
	
	// gain ��ʼ��
	gain = (*p_exp->cmos_sensor.cmos_gain_initialize) ();
	if(!gain)
	{
		XM_printf ("the gain instance can't be empty\n");
		return -1;
	}
	memcpy (&p_exp->cmos_gain, gain, sizeof(p_exp->cmos_gain));
	gain = &p_exp->cmos_gain;
	// ȱʡ��������Ϊ1
	gain->again = 1 << gain->again_shift;
	gain->dgain = 1 << gain->dgain_shift;
	gain->iso = 100;
	gain->max_again = gain->max_again_target;
	gain->max_dgain = gain->max_dgain_target;

	// �ع����������ʼ��
	inttime = (*p_exp->cmos_sensor.cmos_inttime_initialize)();
	if(!inttime)
	{
		XM_printf ("the inttime instance can't be empty\n");
		return -1;
	}
	memcpy (&p_exp->cmos_inttime, inttime, sizeof(p_exp->cmos_inttime));
#if DVR_H264
	p_exp->cmos_inttime.flicker_freq = 50;		// ȱʡ��ֹflicker
#else
	p_exp->cmos_inttime.flicker_freq = 0;		// ȱʡ��ֹflicker
#endif

	/*	
#if ISP_STARTUP_TEST
	p_exp->cmos_inttime.exposure_ashort = 0;
#else
	p_exp->cmos_inttime.exposure_ashort = 32;
#endif
	*/

	// ���inttime����

	// ����sensor�ĳ�ʼ����(����ʱ�估��������)
	isp_cmos_inttime_update (p_exp);

	// ȱʡ֡�� 30֡
	isp_cmos_set_framerate (p_exp, 30);
	
	light_freq = AP_GetMenuItem (APPMENUITEM_LIGHT_FREQ);
	if(light_freq == AP_SETTING_LIGHT_FREQ_50HZ)
		isp_cmos_set_flicker_freq (p_exp, 50);
	else if(light_freq == AP_SETTING_LIGHT_FREQ_60HZ)
		isp_cmos_set_flicker_freq (p_exp, 60);
	else //if(light_freq == AP_SETTING_LIGHT_FREQ_OFF)
		isp_cmos_set_flicker_freq (p_exp, 0);
		
	// ����һ����ʼ�ع�ֵ
	p_exp->sys_factor = 256;//128;	//64;
	p_exp->exposure = p_exp->cmos_inttime.exposure_ashort * p_exp->sys_factor;

	// ����ع�������
	p_exp->exp_tlimit = exposure_limit_calculate(
				p_exp->cmos_inttime.max_flicker_lines,
		   	p_exp->cmos_gain.max_again,
				p_exp->cmos_gain.max_dgain,
				(u16_t)p_exp->sys_factor,
				(u16_t)(p_exp->cmos_gain.again_shift + p_exp->cmos_gain.dgain_shift)
				);
		
	// ��С�ع�������
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
// 20170120����
#define	AE_WINDOW_4545A5686	1		// 4 5 4 5 10 5 6 8 6
#endif


//#define	AE_WINDOW_4545F56D6	1		// 4, 5, 4, 5, 15, 5, 6, 13,6
//#define	AE_WINDOW_3434F45D5	1		// 3, 4, 3, 4, 15, 4, 5, 13, 5

//#define	AE_10		1

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_BG0806
#define	AE_2		1		// �����ع���, ͬʱ����eris����
#else
#define	AE_9		1
#endif

// 20170120����, ʹ��AE_8, �ع���΢�е㲻��, ĳЩ����ƫ��
//#define	AE_8		1

// 20170120����, ʹ��AE_7, �ع���΢�е����(�߲㽨����������), ����������һ�ֳ�Ч��һ��, ���Ͳ���
//#define	AE_7	1


//#define	AE_6	1

//#define	AE_2	1
//#define	AE_3	1
//#define	AE_4	1
//#define	AE_5	1
// ��ʼ��cmos sensor��ʵ��
int arkn141_isp_ae_initialize (cmos_exposure_ptr_t p_exp)
{
	u8_t hist_thresh[4] = {0x10, 0x40, 0x80, 0xc0};
#if AE_WINDOW_111111111
	u8_t win_weight[3][3] = {1, 1, 1, 1,  1, 1, 1, 1, 1};
#elif AE_WINDOW_222121111
	u8_t win_weight[3][3] = {2, 2, 2, 1,  2, 1, 1, 1, 1};
#elif AE_WINDOW_454575676
	u8_t win_weight[3][3] = {4, 5, 4, 5,  7, 5, 6, 7, 6};		// ���ҹ����(�ײ�)�����(�м�)
#elif AE_WINDOW_4545A5686
	u8_t win_weight[3][3] = {4, 5, 4, 5, 10, 5, 6, 8, 6};		// ǿ�����ģʽ, ���ҹ����(�ײ�)
																				//		�Ƚ� AE_WINDOW_454575676, ������뼰�˳����
#elif AE_WINDOW_4545F56D6
	u8_t win_weight[3][3] = {4, 5, 4, 5, 15, 5, 6, 13,6};		// �ɻ����ģʽ, ���ҹ����(�ײ�)
#elif AE_WINDOW_3434F45D5
	u8_t win_weight[3][3] = {3, 4, 3, 4, 15, 4, 5, 13,5};		// �ɻ����ģʽ, ���ҹ����(�ײ�)		
																				//		����, �ᵼ���ܱ�������ڹ��ص�����
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


// ���
static u8_t ae_win_weight_backlight[3][3] = 
	{3, 4, 3, 6, 15, 6, 12, 15, 12};		// 

// ����
static u8_t ae_win_weight_normal[3][3] = 
	{4, 5, 4, 5, 10, 5, 6, 8, 6};			// ǿ�����ģʽ, ���ҹ����(�ײ�)

// ҹ��
static u8_t ae_win_weight_night[3][3] = 
	{3, 3, 3, 5, 12, 5, 12, 13, 12};		// ҹ����/����

static unsigned int ae_window_mode = AE_WINDOW_MODE_NORMAL;

void arkn141_isp_set_ae_window_mode (unsigned int window_mode)
{
	if(window_mode == AE_WINDOW_MODE_BACKLIGHT)
	{
		// ��⴦��
		ae_window_mode = window_mode;
		isp_ae_window_weight_write (&isp_exposure.cmos_ae, ae_win_weight_backlight);
		XM_printf ("AW_BACKLIGHT\n");
	}		
	else if(window_mode == AE_WINDOW_MODE_NORMAL)
	{
		// ����ģʽ
		ae_window_mode = window_mode;
		isp_ae_window_weight_write (&isp_exposure.cmos_ae, ae_win_weight_normal);	
		XM_printf ("AW_NORMAL\n");
	}
	else if(window_mode == AE_WINDOW_MODE_NIGHT)
	{
		// ҹ��, ���س���/ָʾ��
		ae_window_mode = window_mode;
		isp_ae_window_weight_write (&isp_exposure.cmos_ae, ae_win_weight_night);	
		XM_printf ("AW_NIGHT\n");
	}
}

unsigned int arkn141_isp_get_ae_window_mode (void)
{
	return ae_window_mode;
}

// ���, ��������ϸ��
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

// ��ͨ����, �߶Աȶ�
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

// ҹ����, �����ʹ�ϸ��, ���Ƹ߹����
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
	// ���������ĶԱȶ�
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
	// ƥ��
	if(inttime == gamma_polyline_tbl[i].inttime)
	{
		memcpy (gamma_tbl->gamma_lut, gamma_polyline_tbl[i].gamma_lut, sizeof(int) * 65);
	}
	// �߽�
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