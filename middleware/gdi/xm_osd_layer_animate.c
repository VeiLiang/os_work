//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_osd_layer_animate.c
//
//	Revision history
//
//		2014.02.27	ZhuoYongHong Initial version
//
//****************************************************************************
#include <string.h>
#include <stdlib.h>
#include "xm_printf.h"
#include "xm_type.h"
#include "xm_base.h"
#include "xm_user.h"
#include <assert.h>

#include "hw_osd_layer.h"
#include "xm_osd_layer.h"
#include "xm_osd_layer_animate.h"

#define	DELAY_CYCLE		10//20		// 最小移动延迟时间(毫秒)
#define	STEP_COUNT			11

// 移动步长
typedef struct tagMOVESTEP {
	unsigned int	lcdc_channel;			// LCDC视频输出通道
	unsigned int	delay_time;				// 下一次移动延迟时间(毫秒)
	unsigned int	osd_count;				// 需要移动的OSD层个数

	unsigned int	update_viewable;		// 更新可显示区

	// OSD层定义(OSD层编号及位置)
	unsigned int	osd_layer[2];			// OSD层通道号
	int				osd_offset_x[2];		// X轴方向OSD层原点位置(相对LCDC视频输出原点，即LCDC的左上角)
	int				osd_offset_y[2];		// Y轴方向OSD层原点位置(相对LCDC视频输出原点，即LCDC的左上角)
	unsigned int	osd_brightness[2];	// 亮度因子 (0 ~ 64)

	int				osd_viewable_x[2];
	int				osd_viewable_y[2];
	int				osd_viewable_w[2];
	int				osd_viewable_h[2];

} MOVESTEP;

extern void _xm_paint_view (HANDLE hWnd, xm_osd_framebuffer_t framebuffer);


static MOVESTEP move_step[64];	// 移动步长参数
static int move_step_count;		// 移动步长参数个数	

// 计算视图切换步长、时间及位置，并将待移动的OSD层定位到初始位置
static void calc_move_step_and_setup_initial_osd_layer_position ( 
								  unsigned int lcdc_channel, 
								  unsigned int osd_layer_old_top_view,
								  unsigned int osd_layer_new_top_view,
								  unsigned int animating_mode, 
								  unsigned int moving_direction
								  )
{
	unsigned int lcdc_width, lcdc_height;
	unsigned int osd_width, osd_height;
	int osd_start_x, osd_start_y;
	int osd_end_x, osd_end_y;
	int i;
	int				osd_viewable_x[2];
	int				osd_viewable_y[2];
	int				osd_viewable_w[2];
	int				osd_viewable_h[2];

	osd_start_x = 0;
	osd_start_y = 0;
	osd_end_x = 0;
	osd_end_y = 0;

	move_step_count = 0;
	memset (move_step, 0, sizeof(move_step));

	if(lcdc_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcdc_channel);
		return;
	}
	if(osd_layer_old_top_view != (unsigned int)(-1) && osd_layer_old_top_view >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer_old_top_view);
		return;
	}
	if(osd_layer_new_top_view != (unsigned int)(-1) && osd_layer_new_top_view >= XM_OSD_LAYER_COUNT)
	{
		XM_printf ("illegal osd_layer (%d)\n", osd_layer_new_top_view);
		return;
	}

	// 获取LCDC视频输出通道的尺寸
	lcdc_width = HW_lcdc_get_xdots (lcdc_channel);
	lcdc_height = HW_lcdc_get_ydots (lcdc_channel);
	// 获取OSD的尺寸
	osd_width = XM_lcdc_osd_get_width   (lcdc_channel, osd_layer_old_top_view);
	osd_height = XM_lcdc_osd_get_height (lcdc_channel, osd_layer_old_top_view);

	// OSD居中显示

	if(animating_mode == XM_OSDLAYER_ANIMATE_MODE_VIEW_CREATE)
	{
		// 1) 自下而上 (仅需要OSD2层)
		if(moving_direction == XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP)
		{
			int update_viewable = 1;
			// 新视图VIEW顶部从LCD底部开始，逐步移动到LCD顶部
			osd_start_x = (lcdc_width - osd_width) / 2;
			osd_start_y = lcdc_height - (lcdc_height - osd_height) / 2;
			// OSD对齐约束
			osd_start_x = XM_lcdc_osd_horz_align (lcdc_channel, osd_start_x);
			osd_start_y = XM_lcdc_osd_vert_align (lcdc_channel, osd_start_y);

			osd_end_x = (lcdc_width - osd_width) / 2;
			osd_end_y = (lcdc_height - osd_height) / 2;
			// OSD对齐约束
			osd_end_x = XM_lcdc_osd_horz_align (lcdc_channel, osd_end_x);
			osd_end_y = XM_lcdc_osd_vert_align (lcdc_channel, osd_end_y);

			osd_viewable_x[0] = XM_lcdc_osd_get_viewable_x_offset (lcdc_channel, osd_layer_new_top_view);
			osd_viewable_y[0] = XM_lcdc_osd_get_viewable_y_offset (lcdc_channel, osd_layer_new_top_view);
			osd_viewable_w[0] = XM_lcdc_osd_get_viewable_width (lcdc_channel, osd_layer_new_top_view);
			osd_viewable_h[0] = XM_lcdc_osd_get_viewable_height (lcdc_channel, osd_layer_new_top_view);
			osd_viewable_x[1] = XM_lcdc_osd_get_viewable_x_offset (lcdc_channel, osd_layer_old_top_view);
			osd_viewable_y[1] = XM_lcdc_osd_get_viewable_y_offset (lcdc_channel, osd_layer_old_top_view);
			osd_viewable_w[1] = XM_lcdc_osd_get_viewable_width (lcdc_channel, osd_layer_old_top_view);
			osd_viewable_h[1] = XM_lcdc_osd_get_viewable_height (lcdc_channel, osd_layer_old_top_view);

			if(	osd_viewable_w[1] == 0 && osd_viewable_h[1] == 0
				||	osd_viewable_w[0] == 0 && osd_viewable_h[0] == 0)
				update_viewable = 0;

			// 此处简单实现, 1秒内从底部匀速移到顶部
			move_step_count = STEP_COUNT;
			for (i = 0; i < move_step_count; i ++)
			{
				int offset;
				move_step[i].delay_time = DELAY_CYCLE;		// 100毫秒
				move_step[i].lcdc_channel = lcdc_channel;
				move_step[i].update_viewable = update_viewable;

				// 顶部视图自底向上移动; 底部视图保持位置不动，可视区域自下向上压缩。
				move_step[i].osd_layer[0] = osd_layer_new_top_view;
				move_step[i].osd_offset_x[0] = XM_lcdc_osd_horz_align (lcdc_channel, osd_start_x);
				move_step[i].osd_offset_y[0] = XM_lcdc_osd_vert_align (lcdc_channel,  
								osd_start_y + (osd_end_y - osd_start_y) * i / (STEP_COUNT-1));

				offset = osd_start_y - move_step[i].osd_offset_y[0];

				move_step[i].osd_brightness[0] = 64;

				move_step[i].osd_layer[1] = osd_layer_old_top_view;
				move_step[i].osd_offset_x[1] = XM_lcdc_osd_get_x_offset (lcdc_channel, osd_layer_old_top_view);
				move_step[i].osd_offset_y[1] = XM_lcdc_osd_get_y_offset (lcdc_channel, osd_layer_old_top_view);
				move_step[i].osd_brightness[1] = 64;

				move_step[i].osd_viewable_x[0] = osd_viewable_x[0];
				move_step[i].osd_viewable_y[0] = osd_viewable_y[0];
				move_step[i].osd_viewable_w[0] = osd_viewable_w[0];
				move_step[i].osd_viewable_h[0] = osd_viewable_h[0];

				// 底部视图的水平方向保持不变
				move_step[i].osd_viewable_x[1] = osd_viewable_x[1];
				move_step[i].osd_viewable_w[1] = osd_viewable_w[1];
				// 底部视图的可视区域向上收缩(起始位置保持不变)
				move_step[i].osd_viewable_y[1] = osd_viewable_y[1];
				move_step[i].osd_viewable_h[1] = osd_viewable_h[1] - offset;

				// 可移动的OSD层为2
				move_step[i].osd_count = 2;
			}
		}

		// 2) 自右向左
		else if(moving_direction == XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT)
		{
			osd_start_x = XM_lcdc_osd_horz_align (lcdc_channel, lcdc_width - (lcdc_width - osd_width) / 2);
			osd_start_y = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - osd_height) / 2);
			osd_end_x = XM_lcdc_osd_horz_align (lcdc_channel, (lcdc_width - osd_width) / 2);
			osd_end_y = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - osd_height) / 2);

			// 此处简单实现, 1秒内从底部匀速移到顶部
			move_step_count = STEP_COUNT;
			for (i = 0; i < move_step_count; i ++)
			{
				move_step[i].delay_time = DELAY_CYCLE;		// 100毫秒
				move_step[i].lcdc_channel = lcdc_channel;
				// 两个VIEW同时移动
				move_step[i].osd_layer[0] = osd_layer_new_top_view;
				move_step[i].osd_offset_x[0] = XM_lcdc_osd_horz_align (lcdc_channel,
						osd_start_x + (osd_end_x - osd_start_x) * i / (STEP_COUNT-1));
				move_step[i].osd_offset_y[0] = XM_lcdc_osd_vert_align (lcdc_channel, osd_start_y);
				move_step[i].osd_brightness[0] = 64;
				move_step[i].osd_layer[1] = osd_layer_old_top_view;
				move_step[i].osd_offset_x[1] = XM_lcdc_osd_horz_align (lcdc_channel, move_step[i].osd_offset_x[0] - osd_width);
				move_step[i].osd_offset_y[1] = XM_lcdc_osd_vert_align (lcdc_channel, move_step[i].osd_offset_y[0]);
				move_step[i].osd_brightness[1] = 64;

				move_step[i].osd_count = 2;
			}
		}
		else
		{
			return;
		}
	}
	else if(animating_mode == XM_OSDLAYER_ANIMATE_MODE_VIEW_REMOVE)
	{
		// 1) 自上而下
		if(moving_direction == XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM)
		{
			int update_viewable = 1;
			// VIEW顶部从LCD顶部开始，逐步移动到LCD底部
			osd_start_x = XM_lcdc_osd_horz_align (lcdc_channel, (lcdc_width - osd_width) / 2);
			osd_start_y = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - osd_height) / 2);

			osd_end_x = XM_lcdc_osd_horz_align (lcdc_channel, (lcdc_width - osd_width) / 2);
			osd_end_y = XM_lcdc_osd_vert_align (lcdc_channel, lcdc_height - (lcdc_height - osd_height) / 2);

			osd_viewable_x[0] = XM_lcdc_osd_get_viewable_x_offset (lcdc_channel, osd_layer_new_top_view);
			osd_viewable_y[0] = XM_lcdc_osd_get_viewable_y_offset (lcdc_channel, osd_layer_new_top_view);
			osd_viewable_w[0] = XM_lcdc_osd_get_viewable_width (lcdc_channel, osd_layer_new_top_view);
			osd_viewable_h[0] = XM_lcdc_osd_get_viewable_height (lcdc_channel, osd_layer_new_top_view);
			osd_viewable_x[1] = XM_lcdc_osd_get_viewable_x_offset (lcdc_channel, osd_layer_old_top_view);
			osd_viewable_y[1] = XM_lcdc_osd_get_viewable_y_offset (lcdc_channel, osd_layer_old_top_view);
			osd_viewable_w[1] = XM_lcdc_osd_get_viewable_width (lcdc_channel, osd_layer_old_top_view);
			osd_viewable_h[1] = XM_lcdc_osd_get_viewable_height (lcdc_channel, osd_layer_old_top_view);

			if(	osd_viewable_w[1] == 0 && osd_viewable_h[1] == 0
				||	osd_viewable_w[0] == 0 && osd_viewable_h[0] == 0)
				update_viewable = 0;

			// 此处简单实现, 1秒内从底部匀速移到顶部
			move_step_count = STEP_COUNT;
			for (i = 0; i < move_step_count; i ++)
			{
				int offset;
				move_step[i].delay_time = DELAY_CYCLE;		// 100毫秒
				move_step[i].lcdc_channel = lcdc_channel;
				move_step[i].update_viewable = update_viewable;

				// 顶部视图自顶向下移出; 底部视图保持位置不动，可视区域自上向下增加。
				move_step[i].osd_layer[0] = osd_layer_new_top_view;
				move_step[i].osd_offset_x[0] = XM_lcdc_osd_horz_align (lcdc_channel, osd_start_x);
				move_step[i].osd_offset_y[0] = XM_lcdc_osd_vert_align (lcdc_channel, 
								osd_start_y + (osd_end_y - osd_start_y) * i / (STEP_COUNT-1));

				// 计算顶部视图的移出高度
				offset = move_step[i].osd_offset_y[0] - osd_start_y;

				move_step[i].osd_brightness[0] = 64;

				move_step[i].osd_layer[1] = osd_layer_old_top_view;
				move_step[i].osd_offset_x[1] = XM_lcdc_osd_get_x_offset (lcdc_channel, osd_layer_old_top_view);
				move_step[i].osd_offset_y[1] = XM_lcdc_osd_get_y_offset (lcdc_channel, osd_layer_old_top_view);
				move_step[i].osd_brightness[1] = 64;

				move_step[i].osd_viewable_x[0] = osd_viewable_x[0];
				move_step[i].osd_viewable_y[0] = osd_viewable_y[0];
				move_step[i].osd_viewable_w[0] = osd_viewable_w[0];
				move_step[i].osd_viewable_h[0] = osd_viewable_h[0];

				// 底部视图的水平方向保持不变
				move_step[i].osd_viewable_x[1] = osd_viewable_x[1];
				move_step[i].osd_viewable_w[1] = osd_viewable_w[1];
				// 底部视图的可视区域向下延伸(起始位置保持不变)
				move_step[i].osd_viewable_y[1] = osd_viewable_y[1];
				move_step[i].osd_viewable_h[1] = offset;

				move_step[i].osd_count = 2;
			}
		}

		// 2) 自左向右
		else if(moving_direction == XM_OSDLAYER_MOVING_FROM_LEFT_TO_RIGHT)
		{
			osd_start_x = XM_lcdc_osd_horz_align (lcdc_channel, (lcdc_width - osd_width) / 2);
			osd_start_y = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - osd_height) / 2);
			osd_end_x = XM_lcdc_osd_horz_align (lcdc_channel, lcdc_width - (lcdc_width - osd_width) / 2);
			osd_end_y = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - osd_height) / 2);
			// 此处简单实现, 0.6秒内从底部匀速移到顶部
			move_step_count = STEP_COUNT;
			for (i = 0; i < move_step_count; i ++)
			{
				move_step[i].delay_time = DELAY_CYCLE;		// 60毫秒
				move_step[i].lcdc_channel = lcdc_channel;
				move_step[i].osd_layer[0] = osd_layer_new_top_view;

				// 两个VIEW同时移动
				move_step[i].osd_offset_x[0] = XM_lcdc_osd_horz_align (lcdc_channel, 
									osd_start_x + (osd_end_x - osd_start_x) * i / (STEP_COUNT-1));
				move_step[i].osd_offset_y[0] = XM_lcdc_osd_vert_align (lcdc_channel, osd_end_y);
				move_step[i].osd_brightness[0] = 64;
				move_step[i].osd_layer[1] = osd_layer_old_top_view;
				move_step[i].osd_offset_x[1] = XM_lcdc_osd_horz_align (lcdc_channel, move_step[i].osd_offset_x[0] - osd_width);
				move_step[i].osd_offset_y[1] = XM_lcdc_osd_vert_align (lcdc_channel, move_step[i].osd_offset_y[0]);
				move_step[i].osd_brightness[1] = 64;

				move_step[i].osd_count = 2;
			}
		}
		else
		{
			return;
		}
	}
	else if(animating_mode == XM_OSDLAYER_ANIMATE_MODE_EVENT_CREATE)
	{
		// 1) 自上而下 (需要更新OSD1及OSD2层)
		//if(moving_direction == XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM)
		{
			// VIEW底部从LCD顶部开始下滑，逐步移动到LCD底部
			osd_start_x = XM_lcdc_osd_horz_align (lcdc_channel, (lcdc_width - osd_width) / 2);
			osd_start_y = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - osd_height) / 2 - osd_height);

			osd_end_x = XM_lcdc_osd_horz_align (lcdc_channel, (lcdc_width - osd_width) / 2);
			osd_end_y = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - osd_height) / 2);

			osd_viewable_x[0] = XM_lcdc_osd_get_viewable_x_offset (lcdc_channel, osd_layer_new_top_view);
			osd_viewable_y[0] = XM_lcdc_osd_get_viewable_y_offset (lcdc_channel, osd_layer_new_top_view);
			osd_viewable_w[0] = XM_lcdc_osd_get_viewable_width (lcdc_channel, osd_layer_new_top_view);
			osd_viewable_h[0] = XM_lcdc_osd_get_viewable_height (lcdc_channel, osd_layer_new_top_view);

			osd_viewable_x[1] = XM_lcdc_osd_get_viewable_x_offset (lcdc_channel, osd_layer_old_top_view);
			osd_viewable_y[1] = XM_lcdc_osd_get_viewable_y_offset (lcdc_channel, osd_layer_old_top_view);
			osd_viewable_w[1] = XM_lcdc_osd_get_viewable_width (lcdc_channel, osd_layer_old_top_view);
			osd_viewable_h[1] = XM_lcdc_osd_get_viewable_height (lcdc_channel, osd_layer_old_top_view);

			// 此处简单实现, 1秒内从底部匀速移到顶部
			move_step_count = STEP_COUNT;
			for (i = 0; i < move_step_count; i ++)
			{
				int offset;
				move_step[i].delay_time = DELAY_CYCLE;		// 100毫秒
				move_step[i].lcdc_channel = lcdc_channel;
				move_step[i].update_viewable = 1;

				// 顶部视图自顶向下移动; 底部视图保持位置不动，可视区域自上向下压缩。
				move_step[i].osd_layer[0] = osd_layer_new_top_view;
				move_step[i].osd_offset_x[0] = XM_lcdc_osd_horz_align (lcdc_channel, osd_start_x);
				move_step[i].osd_offset_y[0] = XM_lcdc_osd_vert_align (lcdc_channel, 
									osd_start_y + (osd_end_y - osd_start_y) * i / (STEP_COUNT-1));

				offset = move_step[i].osd_offset_y[0] - osd_start_y;

				move_step[i].osd_brightness[0] = 64;

				move_step[i].osd_layer[1] = osd_layer_old_top_view;
				move_step[i].osd_offset_x[1] = XM_lcdc_osd_get_x_offset (lcdc_channel, osd_layer_old_top_view);
				move_step[i].osd_offset_y[1] = XM_lcdc_osd_get_y_offset (lcdc_channel, osd_layer_old_top_view);
				move_step[i].osd_brightness[1] = 64;

				move_step[i].osd_viewable_x[0] = osd_viewable_x[0];
				move_step[i].osd_viewable_y[0] = osd_viewable_y[0];
				move_step[i].osd_viewable_w[0] = osd_viewable_w[0];
				move_step[i].osd_viewable_h[0] = osd_viewable_h[0];

				// 底部视图的水平方向保持不变
				move_step[i].osd_viewable_x[1] = osd_viewable_x[1];
				move_step[i].osd_viewable_w[1] = osd_viewable_w[1];
				// 底部视图的可视区域向下移动并压缩高度
				move_step[i].osd_viewable_y[1] = osd_viewable_y[1] + offset;
				move_step[i].osd_viewable_h[1] = osd_viewable_h[1] - offset;

				// 可移动的OSD层为2
				move_step[i].osd_count = 2;
			}
		}
	}
	else if(animating_mode == XM_OSDLAYER_ANIMATE_MODE_EVENT_REMOVE)
	{
		//if(moving_direction == XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP)
		{
			osd_start_x = XM_lcdc_osd_horz_align (lcdc_channel, (lcdc_width - osd_width) / 2);
			osd_start_y = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - osd_height) / 2);

			osd_end_x = XM_lcdc_osd_horz_align (lcdc_channel, (lcdc_width - osd_width) / 2);
			osd_end_y  = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - osd_height) / 2 - osd_height);

			osd_viewable_x[0] = XM_lcdc_osd_get_viewable_x_offset (lcdc_channel, osd_layer_new_top_view);
			osd_viewable_y[0] = XM_lcdc_osd_get_viewable_y_offset (lcdc_channel, osd_layer_new_top_view);
			osd_viewable_w[0] = XM_lcdc_osd_get_viewable_width (lcdc_channel, osd_layer_new_top_view);
			osd_viewable_h[0] = XM_lcdc_osd_get_viewable_height (lcdc_channel, osd_layer_new_top_view);

			osd_viewable_x[1] = XM_lcdc_osd_get_viewable_x_offset (lcdc_channel, osd_layer_old_top_view);
			osd_viewable_y[1] = XM_lcdc_osd_get_viewable_y_offset (lcdc_channel, osd_layer_old_top_view);
			osd_viewable_w[1] = XM_lcdc_osd_get_viewable_width (lcdc_channel, osd_layer_old_top_view);
			osd_viewable_h[1] = XM_lcdc_osd_get_viewable_height (lcdc_channel, osd_layer_old_top_view);

			// 此处简单实现, 1秒内从底部匀速移到顶部
			move_step_count = STEP_COUNT;
			for (i = 0; i < move_step_count; i ++)
			{
				int offset;
				move_step[i].delay_time = DELAY_CYCLE;		// 100毫秒
				move_step[i].lcdc_channel = lcdc_channel;
				move_step[i].update_viewable = 1;

				// 顶层视图自底向上移动; 底层视图位置不动, 可视区域从底部向上扩展。
				move_step[i].osd_layer[0] = osd_layer_new_top_view;
				move_step[i].osd_offset_x[0] = XM_lcdc_osd_horz_align (lcdc_channel, osd_start_x);
				move_step[i].osd_offset_y[0] = XM_lcdc_osd_vert_align (lcdc_channel, 
									osd_start_y + (osd_end_y - osd_start_y) * i / (STEP_COUNT-1));

				// 计算顶部视图的移动量
				offset = osd_start_y - move_step[i].osd_offset_y[0];

				move_step[i].osd_brightness[0] = 64;

				// 顶部视图的可视区保持不变
				move_step[i].osd_viewable_x[0] = osd_viewable_x[0];
				move_step[i].osd_viewable_y[0] = osd_viewable_y[0];
				move_step[i].osd_viewable_w[0] = osd_viewable_w[0];
				move_step[i].osd_viewable_h[0] = osd_viewable_h[0];

				move_step[i].osd_layer[1] = osd_layer_old_top_view;
				move_step[i].osd_offset_x[1] = XM_lcdc_osd_get_x_offset (lcdc_channel, osd_layer_old_top_view);
				move_step[i].osd_offset_y[1] = XM_lcdc_osd_get_y_offset (lcdc_channel, osd_layer_old_top_view);
				move_step[i].osd_brightness[1] = 64;

				// 底部视图的水平方向保持不变
				move_step[i].osd_viewable_x[1] = osd_viewable_x[1];
				move_step[i].osd_viewable_w[1] = osd_viewable_w[1];
				// 设置底部视图的可视区偏移
				move_step[i].osd_viewable_y[1] = osd_viewable_y[1] + osd_viewable_h[1] - offset;
				// 设置底部视图的可视区高度
				move_step[i].osd_viewable_h[1] = offset;


				// 可移动的OSD层为2
				move_step[i].osd_count = 2;
			}
		}
	}
	// 将OSD层定位到初始位置，准备开始animate操作
	XM_lcdc_osd_set_x_offset (
						move_step[0].lcdc_channel,
						move_step[0].osd_layer[0],
						move_step[0].osd_offset_x[0]);
	XM_lcdc_osd_set_y_offset (
						move_step[0].lcdc_channel,
						move_step[0].osd_layer[0],
						move_step[0].osd_offset_y[0]);
	XM_lcdc_osd_set_brightness_coeff (
						move_step[0].lcdc_channel,
						move_step[0].osd_layer[0],
						move_step[0].osd_brightness[0]);

	if(move_step[0].update_viewable)
	{
		XM_lcdc_osd_set_viewable_x_offset (
			move_step[0].lcdc_channel,
			move_step[0].osd_layer[0],
			move_step[0].osd_viewable_x[0]);
		XM_lcdc_osd_set_viewable_y_offset (
			move_step[0].lcdc_channel,
			move_step[0].osd_layer[0],
			move_step[0].osd_viewable_y[0]);
		XM_lcdc_osd_set_viewable_height (
			move_step[0].lcdc_channel,
			move_step[0].osd_layer[0],
			move_step[0].osd_viewable_h[0]);
		XM_lcdc_osd_set_viewable_width (
			move_step[0].lcdc_channel,
			move_step[0].osd_layer[0],
			move_step[0].osd_viewable_w[0]);
	}

	if(move_step[0].osd_count == 2)
	{
		// 两个OSD层同时移动
		XM_lcdc_osd_set_x_offset (
						move_step[0].lcdc_channel,
						move_step[0].osd_layer[1],
						move_step[0].osd_offset_x[1]);
		XM_lcdc_osd_set_y_offset (
						move_step[0].lcdc_channel,
						move_step[0].osd_layer[1],
						move_step[0].osd_offset_y[1]);
		XM_lcdc_osd_set_brightness_coeff (
						move_step[0].lcdc_channel,
						move_step[0].osd_layer[1],
						move_step[0].osd_brightness[1]);

		if(move_step[0].update_viewable )
		{
			XM_lcdc_osd_set_viewable_x_offset (
				move_step[0].lcdc_channel,
				move_step[0].osd_layer[1],
				move_step[0].osd_viewable_x[1]);
			XM_lcdc_osd_set_viewable_y_offset (
				move_step[0].lcdc_channel,
				move_step[0].osd_layer[1],
				move_step[0].osd_viewable_y[1]);
			XM_lcdc_osd_set_viewable_height (
				move_step[0].lcdc_channel,
				move_step[0].osd_layer[1],
				move_step[0].osd_viewable_h[1]);
			XM_lcdc_osd_set_viewable_width (
				move_step[0].lcdc_channel,
				move_step[0].osd_layer[1],
				move_step[0].osd_viewable_w[1]);
		}
	}
}

// 计算视图切换步长、时间及位置，并将待移动的OSD层定位到初始位置
static void calc_alert_view_move_step_and_setup_initial_osd_layer_position ( 
								  unsigned int lcdc_channel, 
								  unsigned int moving_osd_layer,
								  unsigned int view_offset,		// 视窗相对于OSD层的偏移
								  unsigned int view_height,		// 视窗的高度
								  unsigned int moving_direction
								  )
{
	unsigned int lcdc_width, lcdc_height;
	unsigned int osd_width, osd_height;
	int osd_start_x;
	int osd_end_x;
	int osd_start_y;
	int osd_end_y;
	int i;
	unsigned int bottom_layer;	// 底层OSD
	unsigned int bottom_osd_width, bottom_osd_height;

	osd_start_x = 0;
	osd_start_y = 0;
	osd_end_x = 0;
	osd_end_y = 0;

	move_step_count = 0;
	memset (move_step, 0, sizeof(move_step));

	if(lcdc_channel >= XM_LCDC_CHANNEL_COUNT)
	{
		XM_printf ("illegal lcd_channel (%d)\n", lcdc_channel);
		return;
	}

	// 获取LCDC视频输出通道的尺寸
	lcdc_width = HW_lcdc_get_xdots (lcdc_channel);
	lcdc_height = HW_lcdc_get_ydots (lcdc_channel);
	// 获取OSD的尺寸
	osd_width = XM_lcdc_osd_get_width   (lcdc_channel, moving_osd_layer);
	osd_height = XM_lcdc_osd_get_height (lcdc_channel, moving_osd_layer);

	// 检查底层的OSD层
	bottom_layer = XM_OSD_LAYER_1;	// 缺省为OSD 1
	if(XM_lcdc_osd_get_enable (lcdc_channel, bottom_layer) == 0)
	{
		bottom_layer = XM_OSD_LAYER_0;	// 底层为视频层	
	}
	bottom_osd_width = XM_lcdc_osd_get_width   (lcdc_channel, bottom_layer);
	bottom_osd_height = XM_lcdc_osd_get_height (lcdc_channel, bottom_layer);


	// OSD居中显示

	// 1) 自下而上 (仅需要OSD2层)
	if(moving_direction == XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP)
	{
		// VIEW顶部从LCD底部开始，逐步移动到LCD顶部
		osd_start_x = (lcdc_width - osd_width) / 2;
		osd_start_y = lcdc_height - (lcdc_height - osd_height) / 2;
		osd_end_x = (lcdc_width - osd_width) / 2;
		osd_end_y = (lcdc_height - osd_height) / 2;

		osd_start_y -= view_offset;
		osd_end_y = osd_start_y - view_height - (osd_height - (view_offset + view_height));

		// 此处简单实现, 1秒内从底部匀速移到顶部
		move_step_count = STEP_COUNT;
		for (i = 0; i < move_step_count; i ++)
		{
			move_step[i].delay_time = DELAY_CYCLE;		// 100毫秒
			move_step[i].lcdc_channel = lcdc_channel;
			
			// 仅新的视图移动，老的视图不动
			move_step[i].osd_layer[0] = moving_osd_layer;
			move_step[i].osd_offset_x[0] = XM_lcdc_osd_horz_align (lcdc_channel, osd_start_x);
			move_step[i].osd_offset_y[0] = XM_lcdc_osd_vert_align (lcdc_channel, osd_start_y + (osd_end_y - osd_start_y) * i / (STEP_COUNT-1));
			move_step[i].osd_brightness[0] = 64;

			// 底部视图的可视区保持不变，仅修改亮度值
			move_step[i].osd_layer[1] = bottom_layer;
			//move_step[i].osd_offset_x[1] = XM_lcdc_osd_horz_align (lcdc_channel, (lcdc_width - bottom_osd_width) / 2);
			//move_step[i].osd_offset_y[1] = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - bottom_osd_height) / 2);
			move_step[i].osd_offset_x[1] = XM_lcdc_osd_get_x_offset (lcdc_channel, bottom_layer);
			move_step[i].osd_offset_y[1] = XM_lcdc_osd_get_y_offset (lcdc_channel, bottom_layer);
			move_step[i].osd_brightness[1] = 64 - i*2;
			

			// 可移动的OSD层为1
			move_step[i].osd_count = 2;
		}
	}
	// 1) 自上而下
	else if(moving_direction == XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM)
	{
		int temp;
		// VIEW顶部从LCD顶部开始，逐步移动到LCD底部
		osd_start_x = (lcdc_width - osd_width) / 2;
		osd_start_y = lcdc_height - (lcdc_height - osd_height) / 2;
		osd_end_x = (lcdc_width - osd_width) / 2;
		osd_end_y = (lcdc_height - osd_height) / 2;

		osd_start_y = osd_start_y - view_offset;
		osd_end_y = osd_start_y - view_height - (osd_height - (view_offset + view_height));

		temp = osd_start_y;
		osd_start_y = osd_end_y;
		osd_end_y = temp;

		// 此处简单实现, 1秒内从底部匀速移到顶部
		move_step_count = STEP_COUNT;
		for (i = 0; i < move_step_count; i ++)
		{
			move_step[i].delay_time = DELAY_CYCLE;		// 100毫秒
			move_step[i].lcdc_channel = lcdc_channel;
			
			// 仅新的视图移动，老的视图不动
			move_step[i].osd_layer[0] = moving_osd_layer;
			move_step[i].osd_offset_x[0] = XM_lcdc_osd_horz_align (lcdc_channel, osd_start_x);
			move_step[i].osd_offset_y[0] = XM_lcdc_osd_vert_align (lcdc_channel, osd_start_y + (osd_end_y - osd_start_y) * i / (STEP_COUNT-1));
			move_step[i].osd_brightness[0] = 64;
			
			// 底部视图的可视区保持不变，仅修改亮度值
			move_step[i].osd_layer[1] = bottom_layer;
		//	move_step[i].osd_offset_x[1] = XM_lcdc_osd_horz_align (lcdc_channel, (lcdc_width - bottom_osd_width) / 2);
		//	move_step[i].osd_offset_y[1] = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - bottom_osd_height) / 2);
			move_step[i].osd_offset_x[1] = XM_lcdc_osd_get_x_offset (lcdc_channel, bottom_layer);
			move_step[i].osd_offset_y[1] = XM_lcdc_osd_get_y_offset (lcdc_channel, bottom_layer);
			move_step[i].osd_brightness[1] = 64 - (10 - i)*2;
			
			// 可移动的OSD层为1
			move_step[i].osd_count = 2;
		}
	}

}

static void start_move_animate (MOVESTEP *movestep, int step_count)
{
		// OSD层移动
		XM_lcdc_osd_set_x_offset (
			movestep->lcdc_channel,
			movestep->osd_layer[0],
			movestep->osd_offset_x[0]);
		XM_lcdc_osd_set_y_offset (
			movestep->lcdc_channel,
			movestep->osd_layer[0],
			movestep->osd_offset_y[0]);
		XM_lcdc_osd_set_brightness_coeff (
			movestep->lcdc_channel,
			movestep->osd_layer[0],
			movestep->osd_brightness[0]);

		if(movestep->update_viewable)
		{
			XM_lcdc_osd_set_viewable_x_offset (
				movestep->lcdc_channel,
				movestep->osd_layer[0],
				movestep->osd_viewable_x[0]);
			XM_lcdc_osd_set_viewable_y_offset (
				movestep->lcdc_channel,
				movestep->osd_layer[0],
				movestep->osd_viewable_y[0]);
			XM_lcdc_osd_set_viewable_height (
				movestep->lcdc_channel,
				movestep->osd_layer[0],
				movestep->osd_viewable_h[0]);
			XM_lcdc_osd_set_viewable_width (
				movestep->lcdc_channel,
				movestep->osd_layer[0],
				movestep->osd_viewable_w[0]);
		}

		// 检查是否存在第二个移动的OSD层
		if(movestep->osd_count == 2)
		{
			XM_lcdc_osd_set_x_offset (
				movestep->lcdc_channel,
				movestep->osd_layer[1],
				movestep->osd_offset_x[1]);
			XM_lcdc_osd_set_y_offset (
				movestep->lcdc_channel,
				movestep->osd_layer[1],
				movestep->osd_offset_y[1]);
			XM_lcdc_osd_set_brightness_coeff (
				movestep->lcdc_channel,
				movestep->osd_layer[1],
				movestep->osd_brightness[1]);

			if(movestep->update_viewable)
			{
				XM_lcdc_osd_set_viewable_x_offset (
					movestep->lcdc_channel,
					movestep->osd_layer[1],
					movestep->osd_viewable_x[1]);
				XM_lcdc_osd_set_viewable_y_offset (
					movestep->lcdc_channel,
					movestep->osd_layer[1],
					movestep->osd_viewable_y[1]);
				XM_lcdc_osd_set_viewable_height (
					movestep->lcdc_channel,
					movestep->osd_layer[1],
					movestep->osd_viewable_h[1]);
				XM_lcdc_osd_set_viewable_width (
					movestep->lcdc_channel,
					movestep->osd_layer[1],
					movestep->osd_viewable_w[1]);
			}

		}
}

static void do_move_animate (MOVESTEP *movestep, int step_count)
{
	unsigned int layer_mask;
	unsigned int lock_mask;
	unsigned int skip_layer_mask = 0;
	int skip_animate = 0;

	if(skip_animate)
	{
		if(movestep->osd_count == 2)
		{
			skip_layer_mask = (1 << movestep->osd_layer[0]) | (1 << movestep->osd_layer[1]);
		}
		else
		{
			skip_layer_mask = (1 << movestep->osd_layer[0]);
		}
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, skip_layer_mask, skip_layer_mask);
	}

	while(step_count)
	{
		// 延时
		if(!skip_animate)
			XM_Sleep (movestep->delay_time);

		if(movestep->osd_count == 2)
		{
			layer_mask = (1 << movestep->osd_layer[0]) | (1 << movestep->osd_layer[1]);
		}
		else
		{
			layer_mask = (1 << movestep->osd_layer[0]);
		}
		lock_mask = layer_mask;

		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, layer_mask, lock_mask);

		// OSD层移动
		XM_lcdc_osd_set_x_offset (
			movestep->lcdc_channel,
			movestep->osd_layer[0],
			movestep->osd_offset_x[0]);
		XM_lcdc_osd_set_y_offset (
			movestep->lcdc_channel,
			movestep->osd_layer[0],
			movestep->osd_offset_y[0]);
		XM_lcdc_osd_set_brightness_coeff (
			movestep->lcdc_channel,
			movestep->osd_layer[0],
			movestep->osd_brightness[0]);

		if(movestep->update_viewable)
		{
			XM_lcdc_osd_set_viewable_x_offset (
				movestep->lcdc_channel,
				movestep->osd_layer[0],
				movestep->osd_viewable_x[0]);
			XM_lcdc_osd_set_viewable_y_offset (
				movestep->lcdc_channel,
				movestep->osd_layer[0],
				movestep->osd_viewable_y[0]);
			XM_lcdc_osd_set_viewable_height (
				movestep->lcdc_channel,
				movestep->osd_layer[0],
				movestep->osd_viewable_h[0]);
			XM_lcdc_osd_set_viewable_width (
				movestep->lcdc_channel,
				movestep->osd_layer[0],
				movestep->osd_viewable_w[0]);
		}

		// 检查是否存在第二个移动的OSD层
		if(movestep->osd_count == 2)
		{
			XM_lcdc_osd_set_x_offset (
				movestep->lcdc_channel,
				movestep->osd_layer[1],
				movestep->osd_offset_x[1]);
			XM_lcdc_osd_set_y_offset (
				movestep->lcdc_channel,
				movestep->osd_layer[1],
				movestep->osd_offset_y[1]);
			XM_lcdc_osd_set_brightness_coeff (
				movestep->lcdc_channel,
				movestep->osd_layer[1],
				movestep->osd_brightness[1]);

			if(movestep->update_viewable)
			{
				XM_lcdc_osd_set_viewable_x_offset (
					movestep->lcdc_channel,
					movestep->osd_layer[1],
					movestep->osd_viewable_x[1]);
				XM_lcdc_osd_set_viewable_y_offset (
					movestep->lcdc_channel,
					movestep->osd_layer[1],
					movestep->osd_viewable_y[1]);
				XM_lcdc_osd_set_viewable_height (
					movestep->lcdc_channel,
					movestep->osd_layer[1],
					movestep->osd_viewable_h[1]);
				XM_lcdc_osd_set_viewable_width (
					movestep->lcdc_channel,
					movestep->osd_layer[1],
					movestep->osd_viewable_w[1]);
			}

		}

		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, layer_mask, 0);
					
		movestep ++;
		step_count --;
	}

	if(skip_animate)
	{
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, skip_layer_mask, 0);
	}
}

// A) XM_OSDLAYER_ANIMATE_MODE_VIEW_CREATE
//    单层View从OSD 2层周边开始移动，逐步覆盖整个OSD 2层。完整覆盖后，View显示从OSD 2层迁移到OSD 1层，OSD 2关闭。
//		
//		用于新视窗创建(视窗栈压入操作)
//		被覆盖视窗(TOPVIEW), 可以为空
//		新创建视窗(NEWVIEW),
//		1) 获取当前OSD 1层的framebuffer_osd_1_topview，允许为空(NULL) 
//		2) 创建一个OSD 2层的framebuffer_osd_2_newview, 根据移动策略设置OSD 2层相对于LCDC输出原点的坐标
//		3) 在framebuffer_osd_newview上格式化输出NEWVIEW的内容(XM_PAINT消息)
//		4) 应用OSD 2层并显示, XM_osd_framebuffer_close (framebuffer_osd_newview)
//		5) 计算步进次数、步长、时间点，启动定时器任务，等待移动结束信号事件
//			定时器任务
//			5.1) 按照步长、时间点，调整OSD 2层的offset
//			5.2) 循环直到步进次数为0
//			5.3) 触发移动结束信号量
//		6) 释放OSD 1层的framebuffer (OSD 1关闭)
//		7)	创建一个OSD 1层的 framebuffer_osd_1_newview
//		8) 将framebuffer_osd_2_newview的设置及缓冲区内容复制到framebuffer_osd_1_newview
//		9)	OSD 1层开启，OSD 2层关闭
//		10)animate过程结束


// B) XM_OSDLAYER_ANIMATE_MODE_VIEW_REMOVE
//    单层View覆盖整个OSD 2层，然后向周边开始移动，逐步移出整个OSD 2层。完整移出后，OSD 2关闭。
//		用于视窗关闭(视窗栈弹出操作)
//		弹出视窗(PULLVIEW), 被弹出的视窗 OSD2
//		新栈顶视窗(TOPVIEW), 可以为空, OSD1
//		1) 创建一个OSD 2层的 framebuffer_osd_2_pullview
//		2) 在framebuffer_osd_2_pullview上格式化PULLVIEW的内容(XM_PAINT消息)
//		3) 释放OSD 1层的framebuffer (OSD 1关闭)
//		4) 创建一个OSD 1层的 framebuffer_osd_1_topview (可为空，被覆盖的顶层视窗)
//		5) 在 framebuffer_osd_1_topview 上格式化输出TOPVIEW的内容(XM_PAINT消息)
//		6) 计算步进次数、步长、时间点，启动定时器任务，等待移动结束信号事件
//			定时器任务
//			6.1) 按照步长、时间点，调整OSD 2层的offset
//			6.2) 循环直到步进次数为0
//			6.3) 触发移动结束信号量
//		7) 释放OSD 2层的framebuffer (OSD 2关闭)
//		8) animate过程结束

// C) XM_OSDLAYER_ANIMATE_MODE_ALERT_CREATE
//    单层View从OSD 2层周边开始移动，逐步覆盖整个OSD 2层。
//		
//		用于对话框(Dialog/Alert等)创建

// D) 单层View从OSD 2层中间向周边开始移动，逐步将OSD 2层移出显示区域，最终OSD 2关闭
//		
//		用于对话框(Dialog/Alert等)摧毁


// OSD层视窗移动
void XM_osd_layer_animate (
									unsigned int animating_mode,		// OSD移动模式
									unsigned int moving_direction,	// 移动方向
									HANDLE old_top_view,					// 老的顶视图句柄
									HANDLE new_top_view					// 新的顶视图句柄
									)
{
	xm_osd_framebuffer_t framebuffer_osd_1, framebuffer_osd_2;
	XMRECT rc;

	unsigned int osd_viewable_x[2];
	unsigned int osd_viewable_y[2];
	unsigned int osd_viewable_w[2];
	unsigned int osd_viewable_h[2];

	// 保存老的可视区位置
	osd_viewable_x[0] = XM_lcdc_osd_get_viewable_x_offset (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1);
	osd_viewable_y[0] = XM_lcdc_osd_get_viewable_y_offset (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1);
	osd_viewable_w[0] = XM_lcdc_osd_get_viewable_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1);
	osd_viewable_h[0] = XM_lcdc_osd_get_viewable_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1);
	osd_viewable_x[1] = XM_lcdc_osd_get_viewable_x_offset (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);
	osd_viewable_y[1] = XM_lcdc_osd_get_viewable_y_offset (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);
	osd_viewable_w[1] = XM_lcdc_osd_get_viewable_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);
	osd_viewable_h[1] = XM_lcdc_osd_get_viewable_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);


	framebuffer_osd_1 = NULL;
	framebuffer_osd_2 = NULL;

	XM_GetDesktopRect (&rc);	// 根据桌面视图的大小定义OSD尺寸

	// 普通视图
	if(	animating_mode == XM_OSDLAYER_ANIMATE_MODE_VIEW_CREATE
		||	animating_mode == XM_OSDLAYER_ANIMATE_MODE_EVENT_CREATE
		)
	{

		// 锁定OSD，延迟修改。等待OSD参数全部初始化完毕后，重新解锁OSD。
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, 
				(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1),
				(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1) );

		// 创建一个OSD 2层的framebuffer_osd_2_newview
		framebuffer_osd_2 = XM_osd_framebuffer_create (
					XM_LCDC_CHANNEL_0,
					XM_OSD_LAYER_2,
					XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
					rc.right - rc.left + 1,
					rc.bottom - rc.top + 1,
					new_top_view,
					0, 0
					);
		if(framebuffer_osd_2 == NULL)
		{
			XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1), 0);
			XM_printf ("XM_osd_layer_animate NG, Can't create OSD 2's framebuffer\n");
			return;
		}
		//		3) 在framebuffer_osd_newview上格式化输出NEWVIEW的内容(XM_PAINT消息)
		_xm_paint_view (new_top_view, framebuffer_osd_2);


		// 计算moving步长(步进次数、步长、时间点)，并设置初始移动开始位置
		calc_move_step_and_setup_initial_osd_layer_position (
				XM_LCDC_CHANNEL_0, 
				XM_OSD_LAYER_1,
				XM_OSD_LAYER_2, 
				animating_mode,
				moving_direction);

		//	开启OSD 2层并显示, XM_osd_framebuffer_close (framebuffer_osd_newview)
		XM_osd_framebuffer_close (framebuffer_osd_2, 0);

		//		5) 计算步进次数、步长、时间点，启动定时器任务，等待移动结束信号事件
		//			定时器任务
		//			5.1) 按照步长、时间点，调整OSD 2层的offset
		//			5.2) 循环直到步进次数为0
		//			5.3) 触发移动结束信号量
		start_move_animate (move_step, move_step_count);

		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1), 0);

		// 执行view渲染过程
		do_move_animate (move_step, move_step_count);


		// 锁定OSD参数修改
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, 
			(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1), 
			(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1) );

		//		6) 释放OSD 1层的framebuffer (OSD 1关闭)
		XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1);

		//		7)	创建一个OSD 1层的 framebuffer_osd_1_newview
		framebuffer_osd_1 = XM_osd_framebuffer_create (
					XM_LCDC_CHANNEL_0,
					XM_OSD_LAYER_1,
					XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
					rc.right - rc.left + 1,
					rc.bottom - rc.top + 1,
					new_top_view,
					0, 0
					);

		//		8) 将framebuffer_osd_2_newview的设置及缓冲区内容复制到framebuffer_osd_1_newview
		if(framebuffer_osd_1)
		{
			int osd2_x, osd2_y;
			// 复制视频缓冲区数据 
			XM_osd_framebuffer_copy (framebuffer_osd_1, framebuffer_osd_2);
			// 获取OSD 2的位置并设置到OSD 1
			osd2_x = XM_lcdc_osd_get_x_offset (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);
			osd2_y = XM_lcdc_osd_get_y_offset (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);
			XM_lcdc_osd_set_x_offset (
				XM_LCDC_CHANNEL_0,
				XM_OSD_LAYER_1,
				osd2_x);
			XM_lcdc_osd_set_y_offset (
				XM_LCDC_CHANNEL_0,
				XM_OSD_LAYER_1,
				osd2_y);

			//		9)	OSD 1层开启，
			XM_osd_framebuffer_close (framebuffer_osd_1, 0);
		}
		
		// OSD 2层关闭
		XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);
		//		10)animate过程结束

		// 检查栈顶TOPVIEW是否是桌面
		if(new_top_view == XM_GetDesktop())
		{
			// 关闭桌面对应的OSD层, 节省刷新带宽。
			// (桌面是一个隐藏的系统控制窗口，负责事件的缺省处理，不支持实际显示)
			XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1);
		}

		// 恢复可视区域
		if(osd_viewable_w[0] && osd_viewable_h[0])
		{
			XM_lcdc_osd_set_viewable_x_offset (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1, osd_viewable_x[0]);
			XM_lcdc_osd_set_viewable_y_offset (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1, osd_viewable_y[0]);
			XM_lcdc_osd_set_viewable_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1, osd_viewable_w[0]);
			XM_lcdc_osd_set_viewable_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1, osd_viewable_h[0]);
		}
		if(osd_viewable_w[1] && osd_viewable_h[1])
		{
			XM_lcdc_osd_set_viewable_x_offset (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2, osd_viewable_x[1]);
			XM_lcdc_osd_set_viewable_y_offset (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2, osd_viewable_y[1]);
			XM_lcdc_osd_set_viewable_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2, osd_viewable_w[1]);
			XM_lcdc_osd_set_viewable_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2, osd_viewable_h[1]);
		}

		// 解锁OSD参数修改
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1), 0);

	}
	else if(	animating_mode == XM_OSDLAYER_ANIMATE_MODE_VIEW_REMOVE
			||	animating_mode == XM_OSDLAYER_ANIMATE_MODE_EVENT_REMOVE
		)
	{
		// B) XM_OSDLAYER_ANIMATE_MODE_VIEW_REMOVE
		//    单层View覆盖整个OSD 2层，然后向周边开始移动，逐步移出整个OSD 2层。完整移出后，OSD 2关闭。
		//		用于视窗关闭(视窗栈弹出操作)
		//		弹出视窗(PULLVIEW), 被弹出的视窗 OSD2
		//		新栈顶视窗(TOPVIEW), 可以为空, OSD1

		// 锁定OSD1/OSD2物理层的任何修改
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, 
				(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1),
				(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1) );

		// 若OSD 1存在，
		//		{ 将OSD 1的内容复制到OSD 2. OSD 2开启，OSD 1关闭 }
		//		1) 打开OSD 1层的framebuffer_osd_1_pullview
		framebuffer_osd_1 = XM_osd_framebuffer_open (
					XM_LCDC_CHANNEL_0,
					XM_OSD_LAYER_1,
					XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
					rc.right - rc.left + 1,
					rc.bottom - rc.top + 1
					);
		if(framebuffer_osd_1)
		{
			//		将OSD 1上的视频缓冲区内容复制到OSD 2
			framebuffer_osd_2 = XM_osd_framebuffer_create (
						XM_LCDC_CHANNEL_0,
						XM_OSD_LAYER_2,
						XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
						rc.right - rc.left + 1,
						rc.bottom - rc.top + 1,
						old_top_view,
						0, 0
						);
			if(framebuffer_osd_2)
			{
				// 将OSD1层的帧数据复制到OSD2层
				XM_osd_framebuffer_copy (framebuffer_osd_2, framebuffer_osd_1);
				//		OSD 2层修改结束
				XM_osd_framebuffer_close (framebuffer_osd_2, 0);
			}
			XM_osd_framebuffer_close (framebuffer_osd_1, 0);
			// 释放OSD1层的帧缓存，同时关闭OSD1层
			XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1);
		}
		
		//		2) 在framebuffer_osd_1上格式化TOPVIEW的内容(XM_PAINT消息)
		framebuffer_osd_1 = XM_osd_framebuffer_create (
						XM_LCDC_CHANNEL_0,
						XM_OSD_LAYER_1,
						XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
						rc.right - rc.left + 1,
						rc.bottom - rc.top + 1,
						new_top_view,
						0, 0
						);
		if(framebuffer_osd_1)
		{
			_xm_paint_view (new_top_view, framebuffer_osd_1);
	
			//			显示OSD1
			XM_osd_framebuffer_close (framebuffer_osd_1, 0);
		}

		// 移动OSD2
		//		6) 计算步进次数、步长、时间点，启动定时器任务，等待移动结束信号事件
		calc_move_step_and_setup_initial_osd_layer_position (
				XM_LCDC_CHANNEL_0, 
				XM_OSD_LAYER_1,
				XM_OSD_LAYER_2, 
				animating_mode,
				moving_direction);

		//			定时器任务
		//			6.1) 按照步长、时间点，调整OSD 2层的offset
		//			6.2) 循环直到步进次数为0
		//			6.3) 触发移动结束信号量
		start_move_animate (move_step, move_step_count);

		// OSD1/OSD2已准备好。解锁，允许OSD物理层更新，OSD层开启并显示。
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1), 0);

		// 执行view渲染过程
		do_move_animate (move_step, move_step_count);

		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, 
			(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1),
			(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1));
		//		6) 释放OSD 2层的framebuffer (OSD 2关闭)
		XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);
		//		7) animate过程结束

		// 检查栈顶TOPVIEW是否是桌面
		if(new_top_view == XM_GetDesktop())
		{
			// 关闭桌面对应的OSD层
			// (桌面是一个隐藏的系统控制窗口，负责事件的缺省处理，不支持实际显示)
			XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1);
		}

		// 恢复可视区域
		if(osd_viewable_w[0] && osd_viewable_h[0])
		{
			XM_lcdc_osd_set_viewable_x_offset (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1, osd_viewable_x[0]);
			XM_lcdc_osd_set_viewable_y_offset (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1, osd_viewable_y[0]);
			XM_lcdc_osd_set_viewable_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1, osd_viewable_w[0]);
			XM_lcdc_osd_set_viewable_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1, osd_viewable_h[0]);
		}
		if(osd_viewable_w[1] && osd_viewable_h[1])
		{
			XM_lcdc_osd_set_viewable_x_offset (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2, osd_viewable_x[1]);
			XM_lcdc_osd_set_viewable_y_offset (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2, osd_viewable_y[1]);
			XM_lcdc_osd_set_viewable_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2, osd_viewable_w[1]);
			XM_lcdc_osd_set_viewable_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2, osd_viewable_h[1]);
		}

		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1), 0);
	}
	// ALERT(对话框)
	else if(animating_mode == XM_OSDLAYER_ANIMATE_MODE_ALERT_CREATE)
	{
		XMRECT rcAlert;
		// 创建一个OSD 2层的framebuffer_osd_2_newview
		framebuffer_osd_2 = XM_osd_framebuffer_create (
					XM_LCDC_CHANNEL_0,
					XM_OSD_LAYER_2,
					XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
					rc.right - rc.left + 1,
					rc.bottom - rc.top + 1,
					new_top_view,
					0, 0
					);
		if(framebuffer_osd_2 == NULL)
		{
			XM_printf ("XM_osd_layer_animate NG, Can't create OSD 2's framebuffer\n");
			return;
		}
		//		3) 在framebuffer_osd_newview上格式化输出NEWVIEW的内容(XM_PAINT消息)
		_xm_paint_view (new_top_view, framebuffer_osd_2);
		
		// 计算moving步长(步进次数、步长、时间点)，并设置初始移动开始位置
		// 获取alert的位置
		XM_GetWindowRect (new_top_view, &rcAlert);

		// 锁定OSD，延迟修改。等待OSD参数全部初始化完毕后，重新解锁OSD。
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, 
			(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1),
			(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1));

		calc_alert_view_move_step_and_setup_initial_osd_layer_position (
					XM_LCDC_CHANNEL_0,
					XM_OSD_LAYER_2,
					rcAlert.top,
					rcAlert.bottom - rcAlert.top + 1,
					moving_direction
					);
		//	开启OSD 2层并显示, XM_osd_framebuffer_close (framebuffer_osd_newview)
		XM_osd_framebuffer_close (framebuffer_osd_2, 0);

		start_move_animate (move_step, move_step_count);

		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, 
			(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1), 0);

		// 执行view渲染过程
		do_move_animate (move_step, move_step_count);
	}
	else if(animating_mode == XM_OSDLAYER_ANIMATE_MODE_ALERT_REMOVE)
	{
		XMRECT rcAlert;
		// 打开一个OSD 2层的framebuffer_osd_2_newview
		framebuffer_osd_2 = XM_osd_framebuffer_open (
					XM_LCDC_CHANNEL_0,
					XM_OSD_LAYER_2,
					XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
					rc.right - rc.left + 1,
					rc.bottom - rc.top + 1
					);
		if(framebuffer_osd_2 == NULL)
		{
			XM_printf ("XM_osd_layer_animate NG, Can't create OSD 2's framebuffer\n");
			return;
		}
		
		// 计算moving步长(步进次数、步长、时间点)，并设置初始移动开始位置
		// 获取alert的位置
		XM_GetWindowRect (old_top_view, &rcAlert);

		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, 
			(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1),
			(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1));

		calc_alert_view_move_step_and_setup_initial_osd_layer_position (
					XM_LCDC_CHANNEL_0,
					XM_OSD_LAYER_2,
					rcAlert.top,
					rcAlert.bottom - rcAlert.top + 1,
					moving_direction
					);

		//	开启OSD 2层并显示, XM_osd_framebuffer_close (framebuffer_osd_newview)
		XM_osd_framebuffer_close (framebuffer_osd_2, 0);

		start_move_animate (move_step, move_step_count);

		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1), 0);

		// 执行view渲染过程
		do_move_animate (move_step, move_step_count);

		//		6) 释放OSD 2层的framebuffer (OSD 2关闭)
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2), (1 << XM_OSD_LAYER_2) );
		XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2), 0);
	}
}
