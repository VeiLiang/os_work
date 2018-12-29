//****************************************************************************
//
//	Copyright (C) 2015 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: arkn141_exposure_cmos.h
//	  constant，macro & basic typedef definition of ISP exposure
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


// CMOS sensor 增益控制结构定义
typedef struct tag_cmos_gain {
	u32_t	again;			// sensor当前设置的模拟增益
	u32_t	dgain;			// sensor当前设置的数字增益
	u32_t iso;				// sensor当前应用的ISO值, ISO = (100 * again * dgain) >> (again_shift + dgain_shift)
								//		基准ISO = 100, 此时无数字/模拟增益使能

	// 可配置增益, 手动曝光时可指定模拟/数字曝光值
	// 可配置的最大模拟增益, max_again_target <= max_again
	u32_t	max_dgain;		
	// 可配置的最大数字增益, max_dgain_target <= max_dgain
	u32_t	max_again;		// sensor的最大模拟增益

	u32_t max_again_target;		// sensor的最大数字增益
	u32_t max_dgain_target;		// sensor的最大模拟增益
	
	u32_t	again_count;			// 模拟增益可用的项目数
	u32_t	dgain_count;			// 数字增益可用的项目数


	u16_t	again_shift;	// 模拟增益放大倍数, dgain_max = (d) << dgain_shift;
	u16_t	dgain_shift;	// 数字增益放大倍数, dgain_max = (a) << dgain_shift;
	

	u16_t aindex;			// 快速索引, again与模拟增益寄存器之间的映射
	u16_t dindex;			// 快速索引, dgain与数字增益寄存器之间的映射
	
	u16_t again_lookup;	// 模拟增益表查找值
	
	float log_again;		// 模拟增益
	float	log_dgain;		// 数字增益
} cmos_gain_t, *cmos_gain_ptr_t;

// CMOS sensor积分时间结构定义
typedef struct tag_cmos_inttime {
	// 画面闪烁校正参数
	u16_t		flicker_freq;
	u16_t		lines_per_500ms;

	u16_t		max_flicker_lines;
	u16_t		min_flicker_lines;


	u16_t		full_lines;				// 每帧行数, 包括 所有有效数据行 + 所有消隐行
	u16_t		full_lines_limit;		// 允许的最大曝光积分行数, 可允许1帧或多帧曝光积分行设置
	// 可配置的最小/最大曝光积分行数设置
	u16_t		min_lines_target;		// 可配置的最小曝光积分行数设置	
	u16_t		max_lines_target;		// 可配置的最大曝光积分行数设置	

	// 当前应用的最小/最大曝光行数量设置
	u16_t		min_lines;
	u16_t		max_lines;
	
	u32_t		exposure_ashort;		// 积分行数

} cmos_inttime_t, * cmos_inttime_ptr_t;



typedef struct tag_cmos_sensor {
	// 初始化CMOS sensor的曝光积分时间实例
	cmos_inttime_ptr_t (*cmos_inttime_initialize) (void);

	// 初始化CMOS sensor的增益实例
	cmos_gain_ptr_t (*cmos_gain_initialize)(void);
	
	// 设置CMOS sensor允许使用的最大增益
	int (*cmos_max_gain_set) (cmos_gain_ptr_t gain, unsigned int max_analog_gain, unsigned int max_digital_gain);
	// 获取CMOS sensor允许使用的最大增益
	int (*cmos_max_gain_get) (cmos_gain_ptr_t gain, unsigned int *max_analog_gain, unsigned int *max_digital_gain);
	
	// 修改曝光积分时间及增益
	void (*cmos_inttime_gain_update)(cmos_inttime_ptr_t p_inttime, cmos_gain_ptr_t gain);
	
	// 手工修改曝光积分时间及增益
	void (*cmos_inttime_gain_update_manual)(cmos_inttime_ptr_t p_inttime, cmos_gain_ptr_t gain);

	// 根据曝光量exposure计算模拟增益
	u32_t (*analog_gain_from_exposure_calculate)	 (cmos_gain_ptr_t gain, u32_t exposure, u32_t exposure_max);
	// 根据曝光量exposure计算数字增益
   u32_t (*digital_gain_from_exposure_calculate) (cmos_gain_ptr_t gain, u32_t exposure, u32_t exposure_max);
	// 获取当前曝光的ISO
	u32_t (*cmos_get_iso)(cmos_gain_ptr_t gain);

	// 设置帧率
	void (*cmos_fps_set) (cmos_inttime_ptr_t p_inttime, u8_t fps);
	
	// 设置sensor readout drection
	// horz_reverse_direction --> 1  horz reverse direction 垂直反向
	//                        --> 0  horz normal direction
	// vert_reverse_direction --> 1  vert reverse direction 水平反向
	//                        --> 0  vert normal direction
	int (*cmos_sensor_set_readout_direction) (u8_t horz_reverse_direction, u8_t vert_reverse_direction);
	
	// sensor的名称
	const char * (*cmos_sensor_get_sensor_name) (void);
	
	// sensor初始化
	int  (*cmos_isp_sensor_init) (isp_sen_ptr_t p_sen);

	
	// ISP初始设置
	void (*cmos_isp_awb_init) (isp_awb_ptr_t p_awb);					// 白平衡初始参数
	void (*cmos_isp_colors_init) (isp_colors_ptr_t p_colors);		// 色彩初始参数
	void (*cmos_isp_denoise_init) (isp_denoise_ptr_t p_denoise);	// 降噪初始设置
	void (*cmos_isp_eris_init)(isp_eris_ptr_t p_eris);					// 宽动态初始设置
	void (*cmos_isp_fesp_init) (isp_fesp_ptr_t p_fesp);				// 镜头校正, fix-pattern-correction, 坏点去除初始设置
	void (*cmos_isp_enhance_init) (isp_enhance_ptr_t p_enhance);	// 图像增强初始设置
	void (*cmos_isp_ae_init) (isp_ae_ptr_t p_ae);						// 自动曝光初始设置
	void (*cmos_isp_sys_init) (isp_sys_ptr_t p_sys, isp_param_ptr_t p_isp);		// 系统初始设置 (sensor pixel位数, bayer mode)

	// ISP运行设置
	void (*cmos_isp_awb_run) (isp_awb_ptr_t p_awb);			// 白平衡动态参数调整
	void (*cmos_isp_colors_run) (isp_colors_ptr_t p_colors, isp_awb_ptr_t p_awb, isp_ae_ptr_t p_ae);	// 色彩表动态参数调整
	void (*cmos_isp_denoise_run) (isp_denoise_ptr_t p_denoise, isp_ae_ptr_t p_ae);	// 降噪动态调整
	void (*cmos_isp_eris_run) (isp_eris_ptr_t p_eris ,isp_ae_ptr_t p_ae);		// 宽动态动态调整
	void (*cmos_isp_fesp_run) (isp_fesp_ptr_t p_fesp, isp_ae_ptr_t p_ae);		// fesp动态调整
	void (*cmos_isp_enhance_run) (isp_enhance_ptr_t p_enhance);
	void (*cmos_isp_ae_run) (isp_ae_ptr_t p_ae);			// AE动态调整
	
	// 20170824 增加微光增强模式
	void (*cmos_isp_set_day_night_mode) (cmos_gain_ptr_t gain, int day_night);		// day_night = 1, 夜晚增强模式, day_night = 0, 普通模式
	
} cmos_sensor_t, *cmos_sensor_ptr_t;

// ISP AE系统信息
typedef struct tag_isp_ae_sys_record {
	u16_t					version;						// 版本号
	u16_t					size;							// 结构字节大小

	u32_t					ev;							// 曝光补偿设置 0.01 ~ 10.0
	u32_t					threshold[4];				// 阈值
	u32_t					black;
	u32_t					white;
	u32_t					comp;
	
	// sensor
	u32_t					sensor_ashift;
	u32_t					sensor_dshift;
	u32_t					max_again_target;
	u32_t					max_dgain_target;

	u32_t					sensor_type;
	u32_t					record_count;		// 记录个数
} isp_ae_sys_record;

#define	ARKN141_ISP_AE_RECORD_V_0001	0x0001
typedef struct tag_isp_ae_frame_record {
	u16_t					version;						// 版本号
	u16_t					size;							// 结构字节大小

	u16_t					index;
	u16_t					lum;
	u32_t					hist[5];
	u32_t					balance;
	i32_t					error;
	
	u8_t					ae_compensation;			// 值越大, 图像的亮度值越大. 值越小, 图像的亮度值越小.
															//		
	u8_t					ae_black_target;
	u8_t					ae_bright_target;
	
	// 曝光量增量(加/减)控制参数
	u32_t					increment_step;			// 曝光量增量阶梯控制
	u32_t					increment_offset;			// 曝光量增量基准
	u32_t					increment_max;				// 允许最大曝光量
	u32_t					increment_min;				// 允许最小曝光量
	u32_t					increment;					// 当前应用的增量值
	// 曝光量
	u32_t					last_exposure;				// 最近一次的曝光量
	u32_t					exposure;					// 理论曝光值
	u32_t					physical_exposure;		// 实际应用的曝光量
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
	u16_t					sys_factor;		// 曝光值放大系数
												//		当曝光量较小时, 为改善曝光量计算的准确性, 将曝光量乘以1个放大系数, 减少计算误差 	
	u32_t					exposure;		// 当前理论曝光值
	u32_t					last_exposure;

	u32_t					physical_exposure;	// 最近的物理曝光值

	cmos_gain_t			cmos_gain;
	cmos_inttime_t		cmos_inttime;
	cmos_sensor_t		cmos_sensor;

	auto_exposure_t	cmos_ae;

	u32_t		exp_tlimit;
	u32_t		exp_llimit;
	u8_t		fps;

	// 记录最近的统计值
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

// 根据计算的曝光设置更新sensor设置
void isp_cmos_inttime_update (cmos_exposure_ptr_t p_exp);

// 初始化cmos sensor的实例
int isp_cmos_sensor_initialize (cmos_exposure_ptr_t p_exp);

u32_t isp_init_cmos_sensor (cmos_sensor_t *cmos_sensor);

int isp_auto_exposure_run (auto_exposure_ptr_t ae_block, cmos_exposure_ptr_t exp_cmos_block, int man_exp);

// 初始化cmos sensor的ISP实例
int arkn141_isp_ae_initialize (cmos_exposure_ptr_t p_exp);

int arkn141_isp_ae_run (cmos_exposure_ptr_t p_exp);

void isp_histogram_thread_update (cmos_exposure_ptr_t p_exp);

void auto_exposure_step_calculate (auto_exposure_ptr_t p_ae_block, cmos_exposure_ptr_t exp_cmos_block);

// 设置sensor的读出方向(水平及垂直方向)
int arkn141_isp_set_sensor_readout_direction (cmos_exposure_ptr_t p_exp, 
															 unsigned int horz_reverse_direction, unsigned int vert_reverse_direction);

void isp_cmos_inttime_update_manual (cmos_exposure_ptr_t p_exp);

unsigned int cmos_calc_inttime_gain (cmos_exposure_t *p_isp_exposure);

// 使能/禁止flicker
int isp_cmos_set_flicker_freq (cmos_exposure_ptr_t p_exp, u8_t flicker_freq);

enum {
	AE_WINDOW_MODE_BACKLIGHT = 0,
	AE_WINDOW_MODE_NORMAL,
	AE_WINDOW_MODE_NIGHT,
	AE_WINDOW_MODE_COUNT
};

// 设置AE窗口模式
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