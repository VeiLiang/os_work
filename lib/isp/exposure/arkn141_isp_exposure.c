#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "arkn141_isp_exposure.h"
#include "arkn141_isp_exposure_cmos.h"
#ifdef WIN32
#include "xm_ispexp_simulate.h"
#endif
#include <xm_printf.h>

static void histogram_bands_initialize (histogram_band_t bands[4])
{
	bands[0].error   = 0;
	bands[1].error   = 0;
	bands[2].error   = 0;
	bands[3].error   = 0;
	bands[0].balance = 0;
	bands[1].balance = 0;
	bands[2].balance = 0;
	bands[3].balance = 0;
}

static void histogram_initialize (histogram_ptr_t p_hist)
{
	p_hist->hist_balance		= 0;
	p_hist->hist_dark			= 0;
	p_hist->hist_dark_shift	= 0x100;
	p_hist->hist_error   	= 0;

	histogram_bands_initialize (p_hist->bands);
}


void isp_system_ae_black_target_write (auto_exposure_ptr_t ae, u8_t data)
{
	ae->ae_black_target = data;
}

u8_t isp_system_ae_black_target_read (auto_exposure_ptr_t ae)
{
	return ae->ae_black_target;
}

void isp_system_ae_bright_target_write (auto_exposure_ptr_t ae, u8_t data)
{
	ae->ae_bright_target = data;
}

u8_t isp_system_ae_bright_target_read (auto_exposure_ptr_t ae)
{
	return ae->ae_bright_target;
}


void isp_histogram_thresh_write (auto_exposure_ptr_t ae, u8_t histoBand[4])
{
	ae->metering_hist_thresh_0_1 = histoBand[0];
	ae->metering_hist_thresh_1_2 = histoBand[1];
	ae->metering_hist_thresh_3_4 = histoBand[2];
	ae->metering_hist_thresh_4_5 = histoBand[3];

	isp_histogram_bands_write (histoBand);
}

void isp_system_ae_compensation_write (auto_exposure_ptr_t ae, u8_t data) 
{
   ae->ae_compensation = data;
}

u8_t isp_system_ae_compensation_read (auto_exposure_ptr_t ae) 
{
	return ae->ae_compensation;
}

void isp_histogram_thread_update (cmos_exposure_ptr_t p_exp)
{
	histogram_band_t *bands = p_exp->cmos_ae.histogram.bands;
	auto_exposure_ptr_t ae = &p_exp->cmos_ae;
	bands[0].cent    = (ae->metering_hist_thresh_0_1 + 0) / 2;
	bands[1].cent    = (ae->metering_hist_thresh_0_1 + ae->metering_hist_thresh_1_2) / 2;
	bands[2].cent    = (ae->metering_hist_thresh_1_2 + ae->metering_hist_thresh_3_4) / 2;
	bands[3].cent    = (ae->metering_hist_thresh_3_4 + ae->metering_hist_thresh_4_5) / 2;
	bands[4].cent    = (ae->metering_hist_thresh_4_5 + 255) / 2;

}

void isp_auto_exposure_compensation (auto_exposure_ptr_t ae, histogram_band_t bands[5])
{
	u8_t ae_comp;
	
	ae_comp = ae->ae_compensation;
	
	bands[0].target  = (ae->ae_black_target << 8) / 2;
	bands[1].target  = (ae->ae_black_target << 8);
	bands[2].target  = 0x2000;
	bands[3].target  = (ae->ae_bright_target << 8) * 2;
	bands[4].target  = (ae->ae_bright_target << 8);
	
	if(ae_comp > 64)
	{
		bands[3].target  = bands[3].target * ae_comp / 64;
		bands[4].target  = bands[4].target * ae_comp / 64;
	}
	else
	{
		bands[0].target  = bands[0].target * 64 / ae_comp;
		bands[1].target  = bands[1].target * 64 / ae_comp;
	}
	
	bands[0].cent    = (ae->metering_hist_thresh_0_1 + 0) / 2;
	bands[1].cent    = (ae->metering_hist_thresh_0_1 + ae->metering_hist_thresh_1_2) / 2;
	bands[2].cent    = (ae->metering_hist_thresh_1_2 + ae->metering_hist_thresh_3_4) / 2;
	bands[3].cent    = (ae->metering_hist_thresh_3_4 + ae->metering_hist_thresh_4_5) / 2;
	bands[4].cent    = (ae->metering_hist_thresh_4_5 + 255) / 2;
}

static void histogram_data_read (histogram_ptr_t p_hist)
{
	int _i;
	u32_t hacc = 0;

	isp_histogram_bands_read(p_hist->bands);

	for(_i = 0; _i < HISTOGRAM_BANDS; ++_i)
	{
		hacc += p_hist->bands[_i].value;
	}
	if(hacc >= 0xFFFF)
		p_hist->bands[2].value = 0;
	else
		p_hist->bands[2].value = (u16_t)(0xFFFF - hacc);

}

static void histogram_error_calculate (auto_exposure_ptr_t ae, histogram_ptr_t p_hist)
{
	u8_t ae_comp;
	u8_t lum = isp_ae_lum_read ();

	ae_comp = ae->ae_compensation;

	p_hist->hist_error = ae_comp - p_hist->hist_balance;
	
#if 1
	p_hist->lum_hist[0] = p_hist->lum_hist[1];
	p_hist->lum_hist[1] = p_hist->lum_hist[2];
	p_hist->lum_hist[2] = p_hist->lum_hist[3];
	p_hist->lum_hist[3] = lum;
	if(p_hist->hist_error > 0)
	{
		// 曝光不够场景
		int total_lum = 0;
		int i;
		for (i = 0; i < 4; i ++)
		{
			if(p_hist->lum_hist[i] >= 3)
				break;
			total_lum += p_hist->lum_hist[i];
		}
		if(i == 4)
		{
			// 亮度偏暗场景下, 增加曝光
			if(total_lum == 0)
				p_hist->hist_error *= 3;
			else
				p_hist->hist_error = p_hist->hist_error * 3 * 4 / total_lum;	
		}
	}
#else
	// 亮度偏暗场景下, 增加曝光
	if(lum < 3 && p_hist->hist_error > 0)
	{
		if(lum == 0)
			p_hist->hist_error *= 3;
		else
			p_hist->hist_error = p_hist->hist_error * 3 / lum;
	}
#endif
}

static void histogram_balance_calculate (histogram_ptr_t p_hist)
{
	int _i;
	float hbal;
	int center;
	float temp;

	hbal = p_hist->bands[2].cent;

	for(_i = 0; _i < HISTOGRAM_BANDS; ++_i)
	{
		if(_i == 0)                    
			center = 0x00;
		else if(_i == HISTOGRAM_BANDS) 
			center = 0xff;
		else                         
			center = p_hist->bands[_i].cent;

		temp = (p_hist->bands[_i].value + p_hist->bands[_i].target/4.0);
		temp *= (center - p_hist->bands[2].cent);
		temp /= p_hist->bands[_i].target;	// target越大,其对误差的计算影响越小
		hbal += temp;
	}
	p_hist->hist_balance =  (int)hbal;
}

static void histogram_dark_calculate (histogram_ptr_t p_hist)
{
	i32_t h_dark = 0;

	h_dark = (p_hist->hist_dark_shift*(p_hist->bands[0].value*2 + p_hist->bands[1].value) + 0x2000) /
				(p_hist->bands[2].value + 0x2000);

	p_hist->hist_dark = h_dark;
}



static void histogram_update (auto_exposure_ptr_t ae, histogram_ptr_t p_hist)
{
	histogram_data_read (p_hist);

	histogram_balance_calculate (p_hist);

	histogram_error_calculate (ae, p_hist);

	//histogram_dark_calculate (p_hist);
}

void isp_auto_exposure_initialize (auto_exposure_ptr_t p_ae_block)
{
	p_ae_block->exposure_quant = 0x0080;
	p_ae_block->increment_offset = (1 << 8) * 16;
	p_ae_block->increment = p_ae_block->increment_offset;
	p_ae_block->increment_damping = 0;		// 反相阻尼关闭
	p_ae_block->exposure_target = 0;

	p_ae_block->exposure_steps = 0;
	p_ae_block->exposure_factor = 1;

	p_ae_block->steady_control.enable = 0;
	p_ae_block->steady_control.count = 0;

	histogram_initialize (&(p_ae_block->histogram));
}

// 根据当前的直方图误差绝对值计算曝光增量的步长
// 误差绝对值越大,步长越大
static u32_t auto_exposure_increment_get (auto_exposure_ptr_t p_ae_block)
{
	u32_t _step = p_ae_block->exposure_quant;

	if(abs(p_ae_block->histogram.hist_error) <= 0x4)
	{
		_step = p_ae_block->exposure_quant / 64;
	}
	else if(abs(p_ae_block->histogram.hist_error) <= 0x8)
	{
		_step = p_ae_block->exposure_quant / 32;
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0x10)
	{
		//_step = p_ae_block->exposure_quant / 16;
		_step = p_ae_block->exposure_quant / 12;
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0x18)
	{
		//_step = p_ae_block->exposure_quant / 10;
		_step = p_ae_block->exposure_quant / 5;
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0x20)
	{
		//_step = p_ae_block->exposure_quant / 5;
		_step = p_ae_block->exposure_quant * 2 / 5;		// 0.4
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0x30)
	{
		//_step = p_ae_block->exposure_quant / 3;
		_step = p_ae_block->exposure_quant * 3 / 4;	// 0.75
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0x40)
	{
		_step = p_ae_block->exposure_quant * 4 / 5 ;
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0x50)
	{
		_step = p_ae_block->exposure_quant * 1;
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0x60)
	{
		_step = p_ae_block->exposure_quant * 5 / 4;
	}

	// 定义太大的step可能导致曝光调整过大, 导致直方图错误位翻转(从正到负 或 从负到正), 导致曝光闪烁现象发生
	// 比如 下面实例允许大的步幅控制 (> 0x200)
	// hist_error --> -2357,  step = 128(步幅过大), 减少曝光, 步幅调整过大, 新的曝光值 inttime=   61, again=1.000000, dgain=1.000000
	// hist_error -->   115,  step = 16, 因为步幅调整过大,需要逐步增加曝光值, 新的曝光值 inttime=   64, again=1.000000, dgain=1.007813
	// ...
	// hist_error -->      5, step = 1, 曝光稳定, inttime=   91, again=1.000000, dgain=1.007813
	/*
	else if(abs(p_ae_block->histogram.hist_error) > 0x200)
	{
		_step = p_ae_block->exposure_quant * 8;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 0x100)
	{
		_step = p_ae_block->exposure_quant * 4;
	}*/
	
	else if(abs(p_ae_block->histogram.hist_error) < 0x70)
	{
		//_step = p_ae_block->exposure_quant * 2;
		_step = p_ae_block->exposure_quant * 4 / 3;
	}
	// 考虑下面的步幅过大,导致画面亮暗明显
	/*else if(abs(p_ae_block->histogram.hist_error) < 0x90)
	{
		// _step = p_ae_block->exposure_quant * 2;
		_step = p_ae_block->exposure_quant * 2;
	}*/
	
	else if(abs(p_ae_block->histogram.hist_error) < 0xA0)
	{
		_step = p_ae_block->exposure_quant * 3/2;
		//_step = p_ae_block->exposure_quant * 5 / 2;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 3000)
	{
		// _step = p_ae_block->exposure_quant * 2;
		_step = p_ae_block->exposure_quant * 20;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 2000)
	{
		// _step = p_ae_block->exposure_quant * 2;
		_step = p_ae_block->exposure_quant * 15;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 1000)
	{
		// _step = p_ae_block->exposure_quant * 2;
		_step = p_ae_block->exposure_quant * 11;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 900)
	{
		// _step = p_ae_block->exposure_quant * 2;
		_step = p_ae_block->exposure_quant * 9;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 700)
	{
		// _step = p_ae_block->exposure_quant * 2;
		_step = p_ae_block->exposure_quant * 8;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 600)
	{
		_step = p_ae_block->exposure_quant * 6;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 500)
	{
		_step = p_ae_block->exposure_quant * 5;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 400)
	{
		_step = p_ae_block->exposure_quant * 4;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 300)
	{
		_step = p_ae_block->exposure_quant * 3;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 0x100)
	{
		_step = p_ae_block->exposure_quant * 2;
	}
	else
	{
		// 0xA0 <= x >= 0x100
		// 2 ~ 2.5
		//XM_printf ("error=%d, _step=%d\n", p_ae_block->histogram.hist_error, _step);
		_step = p_ae_block->exposure_quant * 2;
	}
	
	// 以下步幅降低一半会导致隧道进出适应缓慢,重新恢复为原来的步幅.
	//_step /= 2;

	return _step;
}

#undef XM_printf
#define	XM_printf(...)
void auto_exposure_step_calculate (auto_exposure_ptr_t p_ae_block, cmos_exposure_ptr_t exp_cmos_block)
{
	u32_t _step;
	u32_t _step_1st;
	u32_t ratio = 1;

	u8_t	old_steady_enable = p_ae_block->steady_control.enable;

	_step = auto_exposure_increment_get (p_ae_block);
	
	_step_1st = _step;

	// 极端曝光条件修正
	// 1) 该修正在某些光照条件下容易触发振荡, 需要较长时间才能平稳.
	//		增加修正条件的阈值 16 --> 32
	//	2) 减小阶梯值	
	if(p_ae_block->histogram.hist_error < (-32))
	{
		// 曝光严重过度, 能量主要集中在最右侧高光区 (0xC0 ~ 0xFF)
		if(p_ae_block->histogram.bands[4].value >= 0xF800)
		{
			// 将曝光量衰减75%, 32*0.75 = 24
			XM_printf ("SC-OV-0 0.75\r\n");
			_step = p_ae_block->exposure_quant * 8 * 3;
		}
		// 能量主要集中在高光区 (0x80 ~ 0xFF)
		else if( (p_ae_block->histogram.bands[3].value + p_ae_block->histogram.bands[4].value) >= 0xF800)
		{
			// 将曝光量衰减50%, 16/32 = 0.5
			XM_printf ("SC-OV-1 0.5\r\n");
			_step = p_ae_block->exposure_quant * 8 * 2 ;
		}
		// 能量主体集中在高光区 (0x80 ~ 0xFF)
		else if( (p_ae_block->histogram.bands[3].value + p_ae_block->histogram.bands[4].value) >= 0xE000)
		{
			// 将曝光量衰减43.75%, 14/32 = 0.4375
			XM_printf ("SC-OV-2 0.4375\r\n");
			_step = p_ae_block->exposure_quant * 7 * 2 ;
		}
		// 能量主体集中在高光区 (0x80 ~ 0xFF)
		else if( (p_ae_block->histogram.bands[3].value + p_ae_block->histogram.bands[4].value) >= 0xD000)
		{
			// 将曝光量衰减31.25%, 10/32 = 0.3125
			XM_printf ("SC-OV-2 0.3125\r\n");
			_step = p_ae_block->exposure_quant * 5 * 2 ;
		}
		// 能量主体集中在高光区 (0x80 ~ 0xFF)
		else if( (p_ae_block->histogram.bands[3].value + p_ae_block->histogram.bands[4].value) >= 0xC000)
		{
			// 将曝光量衰减25%, 8/32 = 0.25
			XM_printf ("SC-OV-2 0.25\r\n");
			_step = p_ae_block->exposure_quant * 4 * 2 ;
		}
		// 能量主体集中在高光区 (0x80 ~ 0xFF)
		else if( (p_ae_block->histogram.bands[3].value + p_ae_block->histogram.bands[4].value) >= 0xB000)
		{
			// 将曝光量衰减12.5%, 4/32 = 0.125
			XM_printf ("SC-OV-2 0.125\r\n");
			_step = p_ae_block->exposure_quant * 2 * 2 ;
		}
		// 能量主体集中在高光区 (0x80 ~ 0xFF)
		else if( (p_ae_block->histogram.bands[3].value + p_ae_block->histogram.bands[4].value) >= 0xA000)
		{
			// 将曝光量衰减9.375%, 3/32 = 0.09375
			XM_printf ("SC-OV-2 0.09375\r\n");
			_step = p_ae_block->exposure_quant * 3 ;
		}
		// 能量主体集中在高光区 (0x80 ~ 0xFF)
		else if( (p_ae_block->histogram.bands[3].value + p_ae_block->histogram.bands[4].value) >= 0x8000)
		{
			// 将曝光量衰减0.0625, 2/32 = 0.0625
			XM_printf ("SC-OV-2 0.0625\r\n");
			_step = p_ae_block->exposure_quant * 2 * 1 ;
		}
		// 低光区(0x00 ~ 0x3F)能量基本不存在, 低于2/256
		else if( (p_ae_block->histogram.bands[0].value + p_ae_block->histogram.bands[1].value) <= 0x0200 )
		{
			// 低光区(0x00 ~ 0x3F)能量基本不存在
			// 将整体曝光量衰减25%, 8 / 32 = 0.25
			XM_printf ("SC-OV-2 0.25\r\n");
			_step = p_ae_block->exposure_quant * 8 ;
		}
		// 低光区能量(0x00 ~ 0x0F)对曝光衰减速度的影响很低
		// 低光区(0x00 ~ 0x0F)能量基本不存在, 低于2/256
		// 这一条规则没有考虑低光区不存在的场景
		else if( (p_ae_block->histogram.bands[0].value ) <= 0x0200 )
		{
			// 低光区(0x00 ~ 0x0F)能量基本不存在
			// 将整体曝光量衰减1/32, 
			XM_printf ("SC-OV-3 0.03125\r\n");
			_step = p_ae_block->exposure_quant * 1 ;
		}
	}
	else if(p_ae_block->histogram.hist_error > 16)
	{

		// 曝光严重不足, 能量主要集中在低光区(0x00 ~ 0x3F)
		if( (p_ae_block->histogram.bands[0].value + p_ae_block->histogram.bands[1].value) >= 0xFF80 )
		{
			// 曝光量增加2倍 (*3), 即使溢出, 溢出的比例较低
			if(p_ae_block->histogram.bands[1].value <= 0x4000)
			{
				// 0x10 ~ 0x3F区域的能量较小
				//ratio = 8;		// 1024 * 8
				_step = p_ae_block->exposure_quant * 8;
				XM_printf ("SC-UE-0 2.00\r\n");
			}
			else
			{
				//ratio = 4;		// 1024 * 4
				_step = p_ae_block->exposure_quant * 4;
				XM_printf ("SC-UE-0 1.00\r\n");
			}
		}
		// 高光区 (0x80 ~ 0xFF)的比例很低
		// 将 中色调的区域(0x40~0x7F)映射到(0x80~0xC0)
		else if( (p_ae_block->histogram.bands[4].value + p_ae_block->histogram.bands[3].value) < 0x00C0 )
		{
			// 曝光量增加0.5倍. (*1.5)
			// 根据 中色调/低光 的比例决定放大倍数
			if(p_ae_block->histogram.bands[2].value <= 0x1000)
			{
				// 0.375
				//ratio = 3;		// 2048
				_step = p_ae_block->exposure_quant * 8;		// 1024
				XM_printf ("SC-UE-1 0.375\r\n");
			}
			else if(p_ae_block->histogram.bands[2].value <= 0x2000)
			{
				// 0.25
				//ratio = 1;		// 2048
				_step = p_ae_block->exposure_quant * 8;		// 1024
				XM_printf ("SC-UE-1 0.250\r\n");
			}
			else if(p_ae_block->histogram.bands[2].value <= 0x4000)
			{
				// 0.125
				//ratio = 1;		// 2048
				_step = p_ae_block->exposure_quant * 4;		// 512
				XM_printf ("SC-UE-1 0.125\r\n");
			}
			else if(p_ae_block->histogram.bands[2].value <= 0x8000)
			{
				// 0.0625
				//ratio = 1;		// 2048
				_step = p_ae_block->exposure_quant * 2;		// 256
				XM_printf ("SC-UE-1 0.0625\r\n");
			}
			else
			{
			}
		}
		
		// 最右侧高光区 (0xC0 ~ 0xFF)的比例很低
		// 高光区 (0xC0 ~ 0xFF)对曝光增强速度的影响非常有限
		else if( p_ae_block->histogram.bands[4].value < 0x00C0)
		{
			if(p_ae_block->increment < p_ae_block->increment_offset)
			{
				// 
			}
			else
			{
				// 曝光量增加1/64
				//ratio = 1;		// 2048
				_step = p_ae_block->exposure_quant/2;		// 64
				XM_printf ("SC-UE-2 0.016\r\n");
			}
		}
		
	}
	
	if(_step < _step_1st)
		_step = _step_1st;

	/*
	// 曝光逆转过程增量衰减控制
	if(p_ae_block->increment > p_ae_block->increment_offset && p_ae_block->histogram.hist_error < 0)
	{
		// 曝光增加过程中,检测到曝光过度. (即曝光过程逆转, 曝光反相衰减中)
		// 将步长缩至计算增量的1/4
		_step = _step/4;
		// 进一步检查上面修正的步长是否大于最近的步长, 因为最近的步长导致曝光过程逆转, 
		// 那么新的曝光反相衰减过程中, 步长应不大于最近的步长
		if(_step > (u32_t)p_ae_block->increment_step/2)
			_step = p_ae_block->increment_step/2;

		// 使能增量阻尼
		p_ae_block->increment_damping = 1;

		if(p_ae_block->steady_control.enable == 0)
		{
			if(p_ae_block->steady_control.count == 0)
			{
				// 保留当前的曝光参考
				p_ae_block->steady_control.steady_error = p_ae_block->histogram.hist_error;
				p_ae_block->steady_control.exposure = exp_cmos_block->last_exposure;
			}
			else if(abs(p_ae_block->steady_control.steady_error) > abs(p_ae_block->histogram.hist_error))
			{
				// 保留绝对值下的曝光参考
				p_ae_block->steady_control.steady_error = p_ae_block->histogram.hist_error;
				p_ae_block->steady_control.exposure = exp_cmos_block->last_exposure;
			}
			p_ae_block->steady_control.count ++;

			if(p_ae_block->steady_control.count >= 3)
			{
				p_ae_block->steady_control.count = 0;
				p_ae_block->steady_control.enable = 1;

				XM_printf ("\t\t Steady Enabel 0, error=%d, exposure=%d\r\n", 
					p_ae_block->steady_control.steady_error,
					p_ae_block->steady_control.exposure);
			}
		}
	}
	else if(p_ae_block->increment < p_ae_block->increment_offset && p_ae_block->histogram.hist_error > 0)
	{
		// 曝光减少过程中,检测到曝光不够. (即曝光过程逆转, 曝光反相增加中)
		// 将步长缩至计算增量的1/4
		_step = _step/4;
		// 进一步检查上面修正的步长是否大于最近的步长, 因为最近的步长导致曝光过程逆转, 
		// 那么新的曝光反相增长过程中, 步长应不大于最近的步长
		if(_step > (u32_t)p_ae_block->increment_step/2)
			_step = p_ae_block->increment_step/2;

		// 使能增量阻尼
		p_ae_block->increment_damping = 1;

		if(p_ae_block->steady_control.enable == 0)
		{
			if(p_ae_block->steady_control.count == 0)
			{
				// 保留当前的曝光参考
				p_ae_block->steady_control.steady_error = p_ae_block->histogram.hist_error;
				p_ae_block->steady_control.exposure = exp_cmos_block->last_exposure;
			}
			else if(abs(p_ae_block->steady_control.steady_error) > abs(p_ae_block->histogram.hist_error))
			{
				// 保留绝对值下的曝光参考
				p_ae_block->steady_control.steady_error = p_ae_block->histogram.hist_error;
				p_ae_block->steady_control.exposure = exp_cmos_block->last_exposure;
			}
			p_ae_block->steady_control.count ++;
			if(p_ae_block->steady_control.count >= 3)
			{
				p_ae_block->steady_control.count = 0;
				p_ae_block->steady_control.enable = 1;
			
				XM_printf ("\t\t Steady Enabel 1, error=%d, exposure=%d\r\n", 
					p_ae_block->steady_control.steady_error,
					p_ae_block->steady_control.exposure);
			}
		}
	}
	else if(p_ae_block->increment_damping)
	{
		// 关闭增量阻尼
		_step = _step/2;
		p_ae_block->increment_damping = 0;

		if(p_ae_block->steady_control.enable == 0)
			p_ae_block->steady_control.count = 0;
	}
	else
	{
		if(p_ae_block->steady_control.enable == 0)
			p_ae_block->steady_control.count = 0;

	}
	*/

#if 0
	_step = _step / 16;

	if(_step < 1) 
		_step = 1;
#else
	//_step = (_step + 8) / 16;
//	_step = (_step + 4) / 8;

	if(_step < 1) 
		_step = 1;
#endif

	p_ae_block->increment_step = (u16_t)_step;
	p_ae_block->increment_ratio = (u16_t)ratio;

	p_ae_block->increment_min = p_ae_block->increment_offset - _step;
	p_ae_block->increment_max = p_ae_block->increment_offset + _step * ratio;


	if(p_ae_block->histogram.hist_error > 8)
	//if(p_ae_block->histogram.hist_error > 4)
	//if(p_ae_block->histogram.hist_error > 2)
	{
		// 直方图整体偏左, 增加曝光
		p_ae_block->increment = p_ae_block->increment_max;
	}
	else if(p_ae_block->histogram.hist_error < -8)
	//else if(p_ae_block->histogram.hist_error < -4)
	//else if(p_ae_block->histogram.hist_error < -2)
	{
		// 直方图整体偏右, 减少曝光
		p_ae_block->increment = p_ae_block->increment_min;
	}
	else
	{
		// 直方图整体平衡, 保持当前曝光值
		p_ae_block->increment = p_ae_block->increment_offset;
	}


	/*
	// 稳态控制过程下的解除稳态控制检查
	if(p_ae_block->steady_control.enable)
	{
		// 检查是否是进入稳态控制过程后的第一次曝光
		if(old_steady_enable == 1)
		{
			// 进入稳态控制过程后的非第一次曝光

			// 检查稳态控制曝光错误的边界阈值是否超出
			if(	abs(p_ae_block->steady_control.steady_error - p_ae_block->histogram.hist_error) >= STEADY_CONTROL_AE_ERR_THREAD )
			{
				// 曝光错误误差已超出稳态控制的范围, 解除稳态控制
				// 关闭稳态控制
				p_ae_block->steady_control.enable = 0;
				p_ae_block->steady_control.count = 0;

				XM_printf ("\t\t Steady disable\r\n");
			}
		}
	}

	// 稳态控制过程下的曝光值及增量设定
	if(p_ae_block->steady_control.enable)
	{
		// 进入稳态状态过程后, 锁定稳态过程的每次曝光的曝光值为同一个曝光值设定, 这样避免曝光值非稳态导致的闪烁
		exp_cmos_block->exposure = p_ae_block->steady_control.exposure;
		// 锁定曝光增量为1
		p_ae_block->increment = p_ae_block->increment_offset;

	//	XM_printf ("\t\t Steady Control, exposure=%d, inc=%d\r\n", exp_cmos_block->exposure, 	p_ae_block->increment);
	}
	*/

	/*
	XM_printf ("\t\tstep = %4d, offset = %10d, max = %10d, min = %10d\n", 
		_step, 
		p_ae_block->increment_offset, 
		p_ae_block->increment_max,
		p_ae_block->increment_min);
		*/
}

static int cmos_exposure_get_error (cmos_exposure_ptr_t exp_cmos_block, int index)
{
	if(exp_cmos_block->stat_count < AE_STAT_COUNT)
		return 0;
	index = exp_cmos_block->stat_index - 1 - index;
	if(index < 0)
		index += AE_STAT_COUNT;
	return exp_cmos_block->stat_error[index];
}

#define min(a,b) ((a < b) ? a : b)
#define max(a,b) ((a > b) ? a : b)

static int steady_ae_enable = 1;		// AE稳态控制使能, 消除曝光振荡现象

void cmos_exposure_set_steady_ae (int enable)
{
	steady_ae_enable = enable;
}

static int auto_exposure_increment_calculate (auto_exposure_ptr_t p_ae_block, cmos_exposure_ptr_t exp_cmos_block)
{
	histogram_update(p_ae_block, &(p_ae_block->histogram));

	
	//isp_auto_exposure_compensation(p_ae_block, p_ae_block->histogram.bands);
	
	if(steady_ae_enable)
	{
		if(exp_cmos_block->stat_count < AE_STAT_COUNT)
		{
			exp_cmos_block->stat_error[exp_cmos_block->stat_count] = p_ae_block->histogram.hist_error;
			exp_cmos_block->stat_lum[exp_cmos_block->stat_count] = isp_ae_lum_read ();
			exp_cmos_block->stat_count ++;
		}
		else
		{
			unsigned int index = exp_cmos_block->stat_index;
			int error_0, error_1, error_2;
			int diff, min_error, max_error;
			exp_cmos_block->stat_error[index] = p_ae_block->histogram.hist_error;
			exp_cmos_block->stat_lum[index] = isp_ae_lum_read ();
			index ++;
			if(index >= AE_STAT_COUNT)
				index = 0;
			exp_cmos_block->stat_index = index;
			
			// 读取最近的AE错误值
			error_0 = cmos_exposure_get_error(exp_cmos_block, 0);
			error_1 = cmos_exposure_get_error(exp_cmos_block, 1);
			error_2 = cmos_exposure_get_error(exp_cmos_block, 2);
			
			// 检查稳态控制是否已应用
			if(exp_cmos_block->locked)
			{
				// 振荡保护中
				if(exp_cmos_block->locked_threshhold > 0)
				{
					if(error_0 > 0 && error_0 <= exp_cmos_block->locked_threshhold)
					{
						// 小于阈值, 被阻止
						XM_printf ("error = %d, locked_threshhold = %d, blocking\n", error_0, exp_cmos_block->locked_threshhold);
						return 1;
					}
					else
					{
						// 阻止解除
						XM_printf ("error = %d, locked_threshhold = %d, blocking leave\n", error_0, exp_cmos_block->locked_threshhold);
						exp_cmos_block->locked = 0;
						exp_cmos_block->locked_threshhold = 0;
					}
				}
				else 
				{
					if(error_0 < 0 && error_0 >= exp_cmos_block->locked_threshhold)
					{
						// 大于阈值, 被阻止
						XM_printf ("error = %d, locked_threshhold = %d, blocking\n", error_0, exp_cmos_block->locked_threshhold);
						return 1;
					}
					else
					{
						// 阻止解除
						XM_printf ("error = %d, locked_threshhold = %d, blocking leave\n", error_0, exp_cmos_block->locked_threshhold);
						exp_cmos_block->locked = 0;
						exp_cmos_block->locked_threshhold = 0;
					}
				}
			}
			else
			{
				// 判断是否存在振荡
				if(error_0 > 0 && error_1 < 0  && error_2 > 0 )
				{
					// 振荡, 11, -29, 10
					exp_cmos_block->locked = 1;
					diff = abs(error_0 - error_2);
					max_error = max(error_0, error_2);
					exp_cmos_block->locked_threshhold = max_error + max(3, diff);
					XM_printf ("error = %d, locked_threshhold = %d, blocking enter\n", error_0, exp_cmos_block->locked_threshhold);
					return 1;
				}
				else if(error_0 < 0 && error_1 > 0  && error_2 < 0 )
				{
					// 振荡, -9, 11, -11
					exp_cmos_block->locked = 1;
					diff = abs(error_0 - error_2);
					min_error = min(error_0, error_2);
					exp_cmos_block->locked_threshhold = min_error - max(3, diff);
					XM_printf ("error = %d, locked_threshhold = %d, blocking enter\n", error_0, exp_cmos_block->locked_threshhold);
					return 1;
				}
			}
				
		}
	}

	auto_exposure_step_calculate(p_ae_block, exp_cmos_block);
	
	return 0;
}

int isp_auto_exposure_run (auto_exposure_ptr_t ae_block, cmos_exposure_ptr_t exp_cmos_block, int man_exp)
{
	i64_t i64_temp;
	u32_t new_physical_exposure;		// 新的绝对(物理)曝光量
	int ae_stable = 1;
	if(man_exp == 1)
	{
		exp_cmos_block->exposure = ae_block->exposure_target;
	}
	
	exp_cmos_block->last_exposure = exp_cmos_block->exposure;


	// 估计曝光量
	int blocking = auto_exposure_increment_calculate(ae_block, exp_cmos_block);
	if(blocking)
	{
		return ae_stable;
	}
	
	if(man_exp == 1)
	{
		ae_block->increment = ae_block->increment_offset;
	}

	// 根据曝光量, 重新计算sensor的曝光参数
	isp_exposure_cmos_calculate (
				exp_cmos_block,
				ae_block->increment,
				ae_block->increment_max,
				ae_block->increment_min,
				ae_block->increment_offset
				);

	i64_temp = exp_cmos_block->cmos_inttime.exposure_ashort * exp_cmos_block->sys_factor;
	i64_temp *= exp_cmos_block->cmos_gain.again * exp_cmos_block->cmos_gain.dgain;
	i64_temp >>= (exp_cmos_block->cmos_gain.again_shift + exp_cmos_block->cmos_gain.dgain_shift);
	new_physical_exposure = (u32_t)i64_temp;

	// 20170102 zhuoyonghong
	// 为避免出现曝光不稳的现象, 不再执行逻辑曝光加速过程(循环执行逻辑曝光过程直至物理曝光出现差异).
	// 逻辑曝光过程的理论曝光值不等于实际的物理曝光量, 会引入误差(物理曝光量-理论曝光量). 
	// 连续多次逻辑曝光过程产生的物理曝光量误差(物理曝光量-理论曝光量)会更大.
	// 因此取消该逻辑曝光加速过程, 每次逐步执行 
	// 	1)逻辑曝光量计算-->2)物理曝光量计算-->3)物理曝光 -->4曝光反馈
	// 的过程, 避免较大曝光量误差的引入. 较大曝光量误差会导致画面亮度抖动
#if 0
	if(man_exp == 0)
	{
		// 自动曝光模式下
		//int loop = 16;
		int loop = 1;
		int same_exp = 0;
		// 循环直到物理曝光量存在变化
		while (loop > 0 && new_physical_exposure == exp_cmos_block->physical_exposure)
		{
			if(new_physical_exposure == exp_cmos_block->exp_llimit)		// 最小曝光量
				break;
			// 使用与进入时同样的曝光量增量参数, 计算下一个理论的曝光量
			// auto_exposure_step_calculate (ae_block, exp_cmos_block);
			// 根据计算的理论曝光量, 重新计算sensor的曝光参数
			same_exp = isp_exposure_cmos_calculate (
						exp_cmos_block,
						ae_block->increment,
						ae_block->increment_max,
						ae_block->increment_min,
						ae_block->increment_offset
						);
			if(same_exp)
				break;
			// 
			i64_temp = exp_cmos_block->cmos_inttime.exposure_ashort * exp_cmos_block->sys_factor;
			i64_temp *= exp_cmos_block->cmos_gain.again * exp_cmos_block->cmos_gain.dgain;
			i64_temp >>= (exp_cmos_block->cmos_gain.again_shift + exp_cmos_block->cmos_gain.dgain_shift);
			new_physical_exposure = (u32_t)i64_temp;
			loop --;
		}

		if(new_physical_exposure == exp_cmos_block->physical_exposure)
		{
			// 连续的绝对曝光量相同
			if(	(ae_block->increment == ae_block->increment_offset)
				|| (new_physical_exposure == exp_cmos_block->exp_llimit)
				|| (exp_cmos_block->exposure == exp_cmos_block->exp_tlimit)		
				|| (same_exp == 1)
				)
				ae_stable = 1;
			else
				ae_stable = 0;
		}
		else
			ae_stable = 0;
	}
#else
	// 比较新的物理曝光值与最近的值比较. 相同则认为曝光稳定
	if(exp_cmos_block->physical_exposure == new_physical_exposure)
		ae_stable = 1;
	else
		ae_stable = 0;
#endif

	exp_cmos_block->physical_exposure = new_physical_exposure;
	
	/*
	XM_printf ("\t\tlog_exposure=%10d, inttime=%5d, again=%f, dgain=%f\n", 
		(u32_t)new_physical_exposure,
		exp_cmos_block->cmos_inttime.exposure_ashort,
		((float)exp_cmos_block->cmos_gain.again) / (1 << exp_cmos_block->cmos_gain.again_shift),
		((float)exp_cmos_block->cmos_gain.dgain) / (1 << exp_cmos_block->cmos_gain.dgain_shift));
		*/

	// 调整sensor的曝光参数
	isp_cmos_inttime_update (exp_cmos_block);

	return ae_stable;
}

#define MAX_EXPOSURE_LUTS 3
static u16_t exposure_luts_defaults[MAX_EXPOSURE_LUTS][4] =
{ 
	{0xA000, 0x1400, 0x0180, 0x0110 },
	{0x8000, 0x4000, 0x0800, 0x0100 },
	{0x2000, 0x3000, 0x0700, 0x0220 } 
};

void auto_exposure_lut_load (auto_exposure_ptr_t p_ae_block, u8_t lut)
{
	int i;
	if(MAX_EXPOSURE_LUTS <= lut)
	{
		return;
	}

	for(i=0;i<HISTOGRAM_BANDS;++i)
	{
		p_ae_block->histogram.bands[i].target  = exposure_luts_defaults[lut][i];
	}
}