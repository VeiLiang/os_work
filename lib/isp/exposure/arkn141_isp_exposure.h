//****************************************************************************
//
//	Copyright (C) 2015 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: arkn141_isp_exposure.h
//	  constant��macro & basic typedef definition of ISP exposure
//
//	Revision history
//
//		2015.08.30	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _ARKN141_ISP_EXPOSURE_H_
#define _ARKN141_ISP_EXPOSURE_H_

#include "xm_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif


#define HISTOGRAM_BANDS 5

typedef struct tag_histogram_band
{
	u32_t value;
	u32_t error;
	i32_t balance;

	i32_t target;
	i32_t cent;
} histogram_band_t, *histogram_band_ptr_t;

typedef struct tag_histogram
{
	i32_t hist_balance;
	i32_t hist_dark;
	i32_t hist_dark_shift;
	i32_t hist_error;			// < 0, ��Ҫ��С�ع���
									// > 0, ��Ҫ�����ع���	
	histogram_band_t bands[HISTOGRAM_BANDS];
	
	u8_t	lum_hist[4];		// �������ж�
	
} histogram_t, *histogram_ptr_t;


#define	STEADY_CONTROL_AE_ERR_THREAD		5		// ��̬�����ع������ֵ
#define	STEADY_CONTROL_AE_LUM_THREAD		2		// ��̬�����ع�������ֵ

// ��̬����
typedef struct tag_steady_control {
	u8_t		enable;			// ��̬���ƿ�����־
	u8_t		count;			// ���ȴ���

	//	��¼��̬״̬�µ��ع����
	u32_t		exposure;

	// ������̬״̬�µ��ع�����. һ���ع����ⳬ����ֵ��Χ, ��̬���ƽ��
	i32_t		steady_error;	// ��̬�������Ĵ���ֵ
	u8_t		steady_lum;		// ��̬������������ֵ

} steady_control_t;

typedef struct auto_exposure {
	u32_t		increment;				// �ع���������
	u32_t		increment_max;
	u32_t		increment_min;
	u32_t		increment_offset;		// �ع�������Χ
	u16_t		increment_step;
	u16_t		increment_ratio;
	u32_t		increment_damping;	// ��������

											// ��sensor�����عⷭתʱ, ��ʱAE�Ὺ����̬����.
	steady_control_t steady_control;
	histogram_t histogram;
	u32_t			exposure_target;
	u32_t			exposure_quant;		// ϵͳ������ع������ο���
	i8_t			exposure_steps;
	u8_t			exposure_factor;

	u8_t		ae_compensation;	
	u8_t		ae_black_target;
	u8_t		ae_bright_target;

	u8_t		metering_hist_thresh_0_1;
	u8_t		metering_hist_thresh_1_2;
	u8_t		metering_hist_thresh_3_4;
	u8_t		metering_hist_thresh_4_5;

	u8_t		window_weight[3][3];		// 9����Ȩ������


} auto_exposure_t, *auto_exposure_ptr_t;

// ��ʼ��һ���Զ��ع��ʵ��
void isp_auto_exposure_initialize (auto_exposure_ptr_t p_ae_block);

void isp_auto_exposure_compensation (auto_exposure_ptr_t ae, histogram_band_t bands[5]);

void isp_histogram_bands_read (histogram_band_t bands[5]);

void isp_histogram_thresh_write (auto_exposure_ptr_t ae, u8_t histoBand[4]);

void isp_histogram_bands_write (u8_t histoBand[4]);

void isp_ae_window_weight_write (auto_exposure_ptr_t ae, u8_t win_weight[3][3]);

u32_t isp_ae_lum_read (void);

void isp_system_ae_black_target_write (auto_exposure_ptr_t ae, u8_t data);
u8_t isp_system_ae_black_target_read  (auto_exposure_ptr_t ae);
void isp_system_ae_bright_target_write (auto_exposure_ptr_t ae, u8_t data);
u8_t isp_system_ae_bright_target_read  (auto_exposure_ptr_t ae);

void isp_system_ae_compensation_write (auto_exposure_ptr_t ae, u8_t data);
u8_t isp_system_ae_compensation_read  (auto_exposure_ptr_t ae);

#define ISP_SYSTEM_MIN_INTEGRATION_TIME_DEFAULT (0x0002)
#define ISP_SYSTEM_MAX_INTEGRATION_TIME_DEFAULT (0xFFFF)

#if DVR_H264
#define	AE_COMPENSATION_DEFAULT		52
#define	AE_BRIGHT_TARGET_DEFAULT	12			

#else

#if 	ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_BG0806
// 20170803����·��, ��·���ϵĽ����ﶥ���Ƚ�0330Ч��ƫ��, ��΢����
#define	AE_COMPENSATION_DEFAULT		52
#define	AE_BRIGHT_TARGET_DEFAULT	12		
//#define	AE_COMPENSATION_DEFAULT		46		// ��������ֵ
//#define	AE_BRIGHT_TARGET_DEFAULT	10			
#else
#define	AE_COMPENSATION_DEFAULT		56//52//46
#define	AE_BRIGHT_TARGET_DEFAULT	16//12//10			
#endif

#endif


void auto_exposure_lut_load (auto_exposure_ptr_t p_ae_block, u8_t lut);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif	// _ARKN141_ISP_EXPOSURE_H_