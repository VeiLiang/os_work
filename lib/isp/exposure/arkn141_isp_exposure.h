//****************************************************************************
//
//	Copyright (C) 2015 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: arkn141_isp_exposure.h
//	  constant，macro & basic typedef definition of ISP exposure
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
	i32_t hist_error;			// < 0, 需要减小曝光量
									// > 0, 需要增加曝光量	
	histogram_band_t bands[HISTOGRAM_BANDS];
	
	u8_t	lum_hist[4];		// 暗场景判断
	
} histogram_t, *histogram_ptr_t;


#define	STEADY_CONTROL_AE_ERR_THREAD		5		// 稳态控制曝光错误阈值
#define	STEADY_CONTROL_AE_LUM_THREAD		2		// 稳态控制曝光亮度阈值

// 稳态控制
typedef struct tag_steady_control {
	u8_t		enable;			// 稳态控制开启标志
	u8_t		count;			// 非稳次数

	//	记录稳态状态下的曝光参数
	u32_t		exposure;

	// 保存稳态状态下的曝光评测. 一旦曝光评测超出阈值范围, 稳态控制解除
	i32_t		steady_error;	// 稳态下锁定的错误值
	u8_t		steady_lum;		// 稳态下锁定的亮度值

} steady_control_t;

typedef struct auto_exposure {
	u32_t		increment;				// 曝光修正因子
	u32_t		increment_max;
	u32_t		increment_min;
	u32_t		increment_offset;		// 曝光修正范围
	u16_t		increment_step;
	u16_t		increment_ratio;
	u32_t		increment_damping;	// 增量阻尼

											// 当sensor持续曝光翻转时, 此时AE会开启稳态控制.
	steady_control_t steady_control;
	histogram_t histogram;
	u32_t			exposure_target;
	u32_t			exposure_quant;		// 系统定义的曝光修正参考量
	i8_t			exposure_steps;
	u8_t			exposure_factor;

	u8_t		ae_compensation;	
	u8_t		ae_black_target;
	u8_t		ae_bright_target;

	u8_t		metering_hist_thresh_0_1;
	u8_t		metering_hist_thresh_1_2;
	u8_t		metering_hist_thresh_3_4;
	u8_t		metering_hist_thresh_4_5;

	u8_t		window_weight[3][3];		// 9宫格权重因子


} auto_exposure_t, *auto_exposure_ptr_t;

// 初始化一个自动曝光的实例
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
// 20170803阴天路测, 马路边上的建筑物顶部比较0330效果偏白, 稍微过曝
#define	AE_COMPENSATION_DEFAULT		52
#define	AE_BRIGHT_TARGET_DEFAULT	12		
//#define	AE_COMPENSATION_DEFAULT		46		// 降低亮度值
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