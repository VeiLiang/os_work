//****************************************************************************
//
//	Copyright (C) 2015 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: arkn141_exposure_cmos.h
//	  constant��macro & basic typedef definition of ISP exposure
//
//	Revision history
//
//		2015.08.30	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _ARKN141_ISP_EXPOSURE_CMOS_H_
#define _ARKN141_ISP_EXPOSURE_CMOS_H_

#include "xm_type.h"
#include "arkn141_isp_exposure.h"
#include "Gem_isp_ae.h"
#include "gem_isp_awb.h"
#include "gem_isp_colors.h"
#include "gem_isp_denoise.h"
#include "gem_isp_enhance.h"
#include "gem_isp_eris.h"
#include "gem_isp_fesp.h"
#include "gem_isp_sys.h"
#include "gem_isp_sensor.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif


// CMOS sensor ������ƽṹ����
typedef struct tag_cmos_gain {
	u32_t	again;			// sensor��ǰ���õ�ģ������
	u32_t	dgain;			// sensor��ǰ���õ���������
	u32_t iso;				// sensor��ǰӦ�õ�ISOֵ, ISO = (100 * again * dgain) >> (again_shift + dgain_shift)
								//		��׼ISO = 100, ��ʱ������/ģ������ʹ��

	// ����������, �ֶ��ع�ʱ��ָ��ģ��/�����ع�ֵ
	// �����õ����ģ������, max_again_target <= max_again
	u32_t	max_dgain;		
	// �����õ������������, max_dgain_target <= max_dgain
	u32_t	max_again;		// sensor�����ģ������

	u32_t max_again_target;		// sensor�������������
	u32_t max_dgain_target;		// sensor�����ģ������
	
	u32_t	again_count;			// ģ��������õ���Ŀ��
	u32_t	dgain_count;			// ����������õ���Ŀ��


	u16_t	again_shift;	// ģ������Ŵ���, dgain_max = (d) << dgain_shift;
	u16_t	dgain_shift;	// ��������Ŵ���, dgain_max = (a) << dgain_shift;
	

	u16_t aindex;			// ��������, again��ģ������Ĵ���֮���ӳ��
	u16_t dindex;			// ��������, dgain����������Ĵ���֮���ӳ��
	
	u16_t again_lookup;	// ģ����������ֵ
	
	float log_again;		// ģ������
	float	log_dgain;		// ��������
} cmos_gain_t, *cmos_gain_ptr_t;

// CMOS sensor����ʱ��ṹ����
typedef struct tag_cmos_inttime {
	// ������˸У������
	u16_t		flicker_freq;
	u16_t		lines_per_500ms;

	u16_t		max_flicker_lines;
	u16_t		min_flicker_lines;


	u16_t		full_lines;				// ÿ֡����, ���� ������Ч������ + ����������
	u16_t		full_lines_limit;		// ���������ع��������, ������1֡���֡�ع����������
	// �����õ���С/����ع������������
	u16_t		min_lines_target;		// �����õ���С�ع������������	
	u16_t		max_lines_target;		// �����õ�����ع������������	

	// ��ǰӦ�õ���С/����ع�����������
	u16_t		min_lines;
	u16_t		max_lines;
	
	u32_t		exposure_ashort;		// ��������

} cmos_inttime_t, * cmos_inttime_ptr_t;



typedef struct tag_cmos_sensor {
	// ��ʼ��CMOS sensor���ع����ʱ��ʵ��
	cmos_inttime_ptr_t (*cmos_inttime_initialize) (void);

	// ��ʼ��CMOS sensor������ʵ��
	cmos_gain_ptr_t (*cmos_gain_initialize)(void);
	
	// ����CMOS sensor����ʹ�õ��������
	int (*cmos_max_gain_set) (cmos_gain_ptr_t gain, unsigned int max_analog_gain, unsigned int max_digital_gain);
	// ��ȡCMOS sensor����ʹ�õ��������
	int (*cmos_max_gain_get) (cmos_gain_ptr_t gain, unsigned int *max_analog_gain, unsigned int *max_digital_gain);
	
	// �޸��ع����ʱ�估����
	void (*cmos_inttime_gain_update)(cmos_inttime_ptr_t p_inttime, cmos_gain_ptr_t gain);
	
	// �ֹ��޸��ع����ʱ�估����
	void (*cmos_inttime_gain_update_manual)(cmos_inttime_ptr_t p_inttime, cmos_gain_ptr_t gain);

	// �����ع���exposure����ģ������
	u32_t (*analog_gain_from_exposure_calculate)	 (cmos_gain_ptr_t gain, u32_t exposure, u32_t exposure_max);
	// �����ع���exposure������������
   u32_t (*digital_gain_from_exposure_calculate) (cmos_gain_ptr_t gain, u32_t exposure, u32_t exposure_max);
	// ��ȡ��ǰ�ع��ISO
	u32_t (*cmos_get_iso)(cmos_gain_ptr_t gain);

	// ����֡��
	void (*cmos_fps_set) (cmos_inttime_ptr_t p_inttime, u8_t fps);
	
	// ����sensor readout drection
	// horz_reverse_direction --> 1  horz reverse direction ��ֱ����
	//                        --> 0  horz normal direction
	// vert_reverse_direction --> 1  vert reverse direction ˮƽ����
	//                        --> 0  vert normal direction
	int (*cmos_sensor_set_readout_direction) (u8_t horz_reverse_direction, u8_t vert_reverse_direction);
	
	// sensor������
	const char * (*cmos_sensor_get_sensor_name) (void);
	
	// sensor��ʼ��
	int  (*cmos_isp_sensor_init) (isp_sen_ptr_t p_sen);

	
	// ISP��ʼ����
	void (*cmos_isp_awb_init) (isp_awb_ptr_t p_awb);					// ��ƽ���ʼ����
	void (*cmos_isp_colors_init) (isp_colors_ptr_t p_colors);		// ɫ�ʳ�ʼ����
	void (*cmos_isp_denoise_init) (isp_denoise_ptr_t p_denoise);	// �����ʼ����
	void (*cmos_isp_eris_init)(isp_eris_ptr_t p_eris);					// ��̬��ʼ����
	void (*cmos_isp_fesp_init) (isp_fesp_ptr_t p_fesp);				// ��ͷУ��, fix-pattern-correction, ����ȥ����ʼ����
	void (*cmos_isp_enhance_init) (isp_enhance_ptr_t p_enhance);	// ͼ����ǿ��ʼ����
	void (*cmos_isp_ae_init) (isp_ae_ptr_t p_ae);						// �Զ��ع��ʼ����
	void (*cmos_isp_sys_init) (isp_sys_ptr_t p_sys, isp_param_ptr_t p_isp);		// ϵͳ��ʼ���� (sensor pixelλ��, bayer mode)

	// ISP��������
	void (*cmos_isp_awb_run) (isp_awb_ptr_t p_awb);			// ��ƽ�⶯̬��������
	void (*cmos_isp_colors_run) (isp_colors_ptr_t p_colors, isp_awb_ptr_t p_awb, isp_ae_ptr_t p_ae);	// ɫ�ʱ�̬��������
	void (*cmos_isp_denoise_run) (isp_denoise_ptr_t p_denoise, isp_ae_ptr_t p_ae);	// ���붯̬����
	void (*cmos_isp_eris_run) (isp_eris_ptr_t p_eris ,isp_ae_ptr_t p_ae);		// ��̬��̬����
	void (*cmos_isp_fesp_run) (isp_fesp_ptr_t p_fesp, isp_ae_ptr_t p_ae);		// fesp��̬����
	void (*cmos_isp_enhance_run) (isp_enhance_ptr_t p_enhance);
	void (*cmos_isp_ae_run) (isp_ae_ptr_t p_ae);			// AE��̬����
	
	// 20170824 ����΢����ǿģʽ
	void (*cmos_isp_set_day_night_mode) (cmos_gain_ptr_t gain, int day_night);		// day_night = 1, ҹ����ǿģʽ, day_night = 0, ��ͨģʽ
	
} cmos_sensor_t, *cmos_sensor_ptr_t;

// ISP AEϵͳ��Ϣ
typedef struct tag_isp_ae_sys_record {
	u16_t					version;						// �汾��
	u16_t					size;							// �ṹ�ֽڴ�С

	u32_t					ev;							// �عⲹ������ 0.01 ~ 10.0
	u32_t					threshold[4];				// ��ֵ
	u32_t					black;
	u32_t					white;
	u32_t					comp;
	
	// sensor
	u32_t					sensor_ashift;
	u32_t					sensor_dshift;
	u32_t					max_again_target;
	u32_t					max_dgain_target;

	u32_t					sensor_type;
	u32_t					record_count;		// ��¼����
} isp_ae_sys_record;

#define	ARKN141_ISP_AE_RECORD_V_0001	0x0001
typedef struct tag_isp_ae_frame_record {
	u16_t					version;						// �汾��
	u16_t					size;							// �ṹ�ֽڴ�С

	u16_t					index;
	u16_t					lum;
	u32_t					hist[5];
	u32_t					balance;
	i32_t					error;
	
	u8_t					ae_compensation;			// ֵԽ��, ͼ�������ֵԽ��. ֵԽС, ͼ�������ֵԽС.
															//		
	u8_t					ae_black_target;
	u8_t					ae_bright_target;
	
	// �ع�������(��/��)���Ʋ���
	u32_t					increment_step;			// �ع����������ݿ���
	u32_t					increment_offset;			// �ع���������׼
	u32_t					increment_max;				// ��������ع���
	u32_t					increment_min;				// ������С�ع���
	u32_t					increment;					// ��ǰӦ�õ�����ֵ
	// �ع���
	u32_t					last_exposure;				// ���һ�ε��ع���
	u32_t					exposure;					// �����ع�ֵ
	u32_t					physical_exposure;		// ʵ��Ӧ�õ��ع���
	// sensor
	u32_t					sensor_inttime;
	u32_t					sensor_again;
	u32_t					sensor_dgain;
	u16_t					sensor_aindex;
	u16_t					sensor_dindex;
	u16_t					sensor_again_shift;
	u16_t					sensor_dgain_shift;

} isp_ae_frame_record_t, *isp_ae_frame_record_ptr_t;

#define	AE_STAT_COUNT		4
typedef struct tag_cmos_exposure
{
	u16_t					sys_factor;		// �ع�ֵ�Ŵ�ϵ��
												//		���ع�����Сʱ, Ϊ�����ع��������׼ȷ��, ���ع�������1���Ŵ�ϵ��, ���ټ������ 	
	u32_t					exposure;		// ��ǰ�����ع�ֵ
	u32_t					last_exposure;

	u32_t					physical_exposure;	// ����������ع�ֵ

	cmos_gain_t			cmos_gain;
	cmos_inttime_t		cmos_inttime;
	cmos_sensor_t		cmos_sensor;

	auto_exposure_t	cmos_ae;

	u32_t		exp_tlimit;
	u32_t		exp_llimit;
	u8_t		fps;

	// ��¼�����ͳ��ֵ
	i32_t					stat_error[AE_STAT_COUNT];
	u16_t					stat_lum[AE_STAT_COUNT];
	int					stat_count;
	int					stat_index;
	int					locked;
	int					locked_threshhold;
	// 
	isp_ae_frame_record_t	ae_record;
} cmos_exposure_t, *cmos_exposure_ptr_t;



/*
 * This function calculates applies increment/decrement to
 * current exposure and translates it into next values for
 * gains and exposures.
 */
int isp_exposure_cmos_calculate (
		cmos_exposure_ptr_t p_exp,
		u32_t exp_increment,
		u32_t exp_increment_max,
		u32_t exp_increment_min,
		u32_t exp_increment_offset
		);

/* Min Integration Time in lines */
void isp_min_integration_time_set (cmos_exposure_ptr_t p_exp, u16_t value);

/* Max Integration Time in lines */
void isp_max_integration_time_set (cmos_exposure_ptr_t p_exp, u16_t value);

// ���ݼ�����ع����ø���sensor����
void isp_cmos_inttime_update (cmos_exposure_ptr_t p_exp);

// ��ʼ��cmos sensor��ʵ��
int isp_cmos_sensor_initialize (cmos_exposure_ptr_t p_exp);

u32_t isp_init_cmos_sensor (cmos_sensor_t *cmos_sensor);

int isp_auto_exposure_run (auto_exposure_ptr_t ae_block, cmos_exposure_ptr_t exp_cmos_block, int man_exp);

// ��ʼ��cmos sensor��ISPʵ��
int arkn141_isp_ae_initialize (cmos_exposure_ptr_t p_exp);

int arkn141_isp_ae_run (cmos_exposure_ptr_t p_exp);

void isp_histogram_thread_update (cmos_exposure_ptr_t p_exp);

void auto_exposure_step_calculate (auto_exposure_ptr_t p_ae_block, cmos_exposure_ptr_t exp_cmos_block);

// ����sensor�Ķ�������(ˮƽ����ֱ����)
int arkn141_isp_set_sensor_readout_direction (cmos_exposure_ptr_t p_exp, 
															 unsigned int horz_reverse_direction, unsigned int vert_reverse_direction);

void isp_cmos_inttime_update_manual (cmos_exposure_ptr_t p_exp);

unsigned int cmos_calc_inttime_gain (cmos_exposure_t *p_isp_exposure);

// ʹ��/��ֹflicker
int isp_cmos_set_flicker_freq (cmos_exposure_ptr_t p_exp, u8_t flicker_freq);

enum {
	AE_WINDOW_MODE_BACKLIGHT = 0,
	AE_WINDOW_MODE_NORMAL,
	AE_WINDOW_MODE_NIGHT,
	AE_WINDOW_MODE_COUNT
};

// ����AE����ģʽ
unsigned int arkn141_isp_get_ae_window_mode (void);
void arkn141_isp_set_ae_window_mode (unsigned int window_mode);

typedef struct _isp_gamma_polyline_tbl {
	int 	inttime;
	int	gamma_lut[65];
} isp_gamma_polyline_tbl;

void isp_ae_gamma_match (int inttime_gain, isp_gamma_polyline_tbl *gamma_tbl);

unsigned int cmos_exposure_get_eris_dimlight (void);
void cmos_exposure_set_eris_dimlight (unsigned int enable);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif	// _ARKN141_ISP_EXPOSURE_CMOS_H_