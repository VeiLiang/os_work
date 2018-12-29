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

#define	DELAY_CYCLE		10//20		// ��С�ƶ��ӳ�ʱ��(����)
#define	STEP_COUNT			11

// �ƶ�����
typedef struct tagMOVESTEP {
	unsigned int	lcdc_channel;			// LCDC��Ƶ���ͨ��
	unsigned int	delay_time;				// ��һ���ƶ��ӳ�ʱ��(����)
	unsigned int	osd_count;				// ��Ҫ�ƶ���OSD�����

	unsigned int	update_viewable;		// ���¿���ʾ��

	// OSD�㶨��(OSD���ż�λ��)
	unsigned int	osd_layer[2];			// OSD��ͨ����
	int				osd_offset_x[2];		// X�᷽��OSD��ԭ��λ��(���LCDC��Ƶ���ԭ�㣬��LCDC�����Ͻ�)
	int				osd_offset_y[2];		// Y�᷽��OSD��ԭ��λ��(���LCDC��Ƶ���ԭ�㣬��LCDC�����Ͻ�)
	unsigned int	osd_brightness[2];	// �������� (0 ~ 64)

	int				osd_viewable_x[2];
	int				osd_viewable_y[2];
	int				osd_viewable_w[2];
	int				osd_viewable_h[2];

} MOVESTEP;

extern void _xm_paint_view (HANDLE hWnd, xm_osd_framebuffer_t framebuffer);


static MOVESTEP move_step[64];	// �ƶ���������
static int move_step_count;		// �ƶ�������������	

// ������ͼ�л�������ʱ�估λ�ã��������ƶ���OSD�㶨λ����ʼλ��
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

	// ��ȡLCDC��Ƶ���ͨ���ĳߴ�
	lcdc_width = HW_lcdc_get_xdots (lcdc_channel);
	lcdc_height = HW_lcdc_get_ydots (lcdc_channel);
	// ��ȡOSD�ĳߴ�
	osd_width = XM_lcdc_osd_get_width   (lcdc_channel, osd_layer_old_top_view);
	osd_height = XM_lcdc_osd_get_height (lcdc_channel, osd_layer_old_top_view);

	// OSD������ʾ

	if(animating_mode == XM_OSDLAYER_ANIMATE_MODE_VIEW_CREATE)
	{
		// 1) ���¶��� (����ҪOSD2��)
		if(moving_direction == XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP)
		{
			int update_viewable = 1;
			// ����ͼVIEW������LCD�ײ���ʼ�����ƶ���LCD����
			osd_start_x = (lcdc_width - osd_width) / 2;
			osd_start_y = lcdc_height - (lcdc_height - osd_height) / 2;
			// OSD����Լ��
			osd_start_x = XM_lcdc_osd_horz_align (lcdc_channel, osd_start_x);
			osd_start_y = XM_lcdc_osd_vert_align (lcdc_channel, osd_start_y);

			osd_end_x = (lcdc_width - osd_width) / 2;
			osd_end_y = (lcdc_height - osd_height) / 2;
			// OSD����Լ��
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

			// �˴���ʵ��, 1���ڴӵײ������Ƶ�����
			move_step_count = STEP_COUNT;
			for (i = 0; i < move_step_count; i ++)
			{
				int offset;
				move_step[i].delay_time = DELAY_CYCLE;		// 100����
				move_step[i].lcdc_channel = lcdc_channel;
				move_step[i].update_viewable = update_viewable;

				// ������ͼ�Ե������ƶ�; �ײ���ͼ����λ�ò���������������������ѹ����
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

				// �ײ���ͼ��ˮƽ���򱣳ֲ���
				move_step[i].osd_viewable_x[1] = osd_viewable_x[1];
				move_step[i].osd_viewable_w[1] = osd_viewable_w[1];
				// �ײ���ͼ�Ŀ���������������(��ʼλ�ñ��ֲ���)
				move_step[i].osd_viewable_y[1] = osd_viewable_y[1];
				move_step[i].osd_viewable_h[1] = osd_viewable_h[1] - offset;

				// ���ƶ���OSD��Ϊ2
				move_step[i].osd_count = 2;
			}
		}

		// 2) ��������
		else if(moving_direction == XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT)
		{
			osd_start_x = XM_lcdc_osd_horz_align (lcdc_channel, lcdc_width - (lcdc_width - osd_width) / 2);
			osd_start_y = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - osd_height) / 2);
			osd_end_x = XM_lcdc_osd_horz_align (lcdc_channel, (lcdc_width - osd_width) / 2);
			osd_end_y = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - osd_height) / 2);

			// �˴���ʵ��, 1���ڴӵײ������Ƶ�����
			move_step_count = STEP_COUNT;
			for (i = 0; i < move_step_count; i ++)
			{
				move_step[i].delay_time = DELAY_CYCLE;		// 100����
				move_step[i].lcdc_channel = lcdc_channel;
				// ����VIEWͬʱ�ƶ�
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
		// 1) ���϶���
		if(moving_direction == XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM)
		{
			int update_viewable = 1;
			// VIEW������LCD������ʼ�����ƶ���LCD�ײ�
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

			// �˴���ʵ��, 1���ڴӵײ������Ƶ�����
			move_step_count = STEP_COUNT;
			for (i = 0; i < move_step_count; i ++)
			{
				int offset;
				move_step[i].delay_time = DELAY_CYCLE;		// 100����
				move_step[i].lcdc_channel = lcdc_channel;
				move_step[i].update_viewable = update_viewable;

				// ������ͼ�Զ������Ƴ�; �ײ���ͼ����λ�ò������������������������ӡ�
				move_step[i].osd_layer[0] = osd_layer_new_top_view;
				move_step[i].osd_offset_x[0] = XM_lcdc_osd_horz_align (lcdc_channel, osd_start_x);
				move_step[i].osd_offset_y[0] = XM_lcdc_osd_vert_align (lcdc_channel, 
								osd_start_y + (osd_end_y - osd_start_y) * i / (STEP_COUNT-1));

				// ���㶥����ͼ���Ƴ��߶�
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

				// �ײ���ͼ��ˮƽ���򱣳ֲ���
				move_step[i].osd_viewable_x[1] = osd_viewable_x[1];
				move_step[i].osd_viewable_w[1] = osd_viewable_w[1];
				// �ײ���ͼ�Ŀ���������������(��ʼλ�ñ��ֲ���)
				move_step[i].osd_viewable_y[1] = osd_viewable_y[1];
				move_step[i].osd_viewable_h[1] = offset;

				move_step[i].osd_count = 2;
			}
		}

		// 2) ��������
		else if(moving_direction == XM_OSDLAYER_MOVING_FROM_LEFT_TO_RIGHT)
		{
			osd_start_x = XM_lcdc_osd_horz_align (lcdc_channel, (lcdc_width - osd_width) / 2);
			osd_start_y = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - osd_height) / 2);
			osd_end_x = XM_lcdc_osd_horz_align (lcdc_channel, lcdc_width - (lcdc_width - osd_width) / 2);
			osd_end_y = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - osd_height) / 2);
			// �˴���ʵ��, 0.6���ڴӵײ������Ƶ�����
			move_step_count = STEP_COUNT;
			for (i = 0; i < move_step_count; i ++)
			{
				move_step[i].delay_time = DELAY_CYCLE;		// 60����
				move_step[i].lcdc_channel = lcdc_channel;
				move_step[i].osd_layer[0] = osd_layer_new_top_view;

				// ����VIEWͬʱ�ƶ�
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
		// 1) ���϶��� (��Ҫ����OSD1��OSD2��)
		//if(moving_direction == XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM)
		{
			// VIEW�ײ���LCD������ʼ�»������ƶ���LCD�ײ�
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

			// �˴���ʵ��, 1���ڴӵײ������Ƶ�����
			move_step_count = STEP_COUNT;
			for (i = 0; i < move_step_count; i ++)
			{
				int offset;
				move_step[i].delay_time = DELAY_CYCLE;		// 100����
				move_step[i].lcdc_channel = lcdc_channel;
				move_step[i].update_viewable = 1;

				// ������ͼ�Զ������ƶ�; �ײ���ͼ����λ�ò���������������������ѹ����
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

				// �ײ���ͼ��ˮƽ���򱣳ֲ���
				move_step[i].osd_viewable_x[1] = osd_viewable_x[1];
				move_step[i].osd_viewable_w[1] = osd_viewable_w[1];
				// �ײ���ͼ�Ŀ������������ƶ���ѹ���߶�
				move_step[i].osd_viewable_y[1] = osd_viewable_y[1] + offset;
				move_step[i].osd_viewable_h[1] = osd_viewable_h[1] - offset;

				// ���ƶ���OSD��Ϊ2
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

			// �˴���ʵ��, 1���ڴӵײ������Ƶ�����
			move_step_count = STEP_COUNT;
			for (i = 0; i < move_step_count; i ++)
			{
				int offset;
				move_step[i].delay_time = DELAY_CYCLE;		// 100����
				move_step[i].lcdc_channel = lcdc_channel;
				move_step[i].update_viewable = 1;

				// ������ͼ�Ե������ƶ�; �ײ���ͼλ�ò���, ��������ӵײ�������չ��
				move_step[i].osd_layer[0] = osd_layer_new_top_view;
				move_step[i].osd_offset_x[0] = XM_lcdc_osd_horz_align (lcdc_channel, osd_start_x);
				move_step[i].osd_offset_y[0] = XM_lcdc_osd_vert_align (lcdc_channel, 
									osd_start_y + (osd_end_y - osd_start_y) * i / (STEP_COUNT-1));

				// ���㶥����ͼ���ƶ���
				offset = osd_start_y - move_step[i].osd_offset_y[0];

				move_step[i].osd_brightness[0] = 64;

				// ������ͼ�Ŀ��������ֲ���
				move_step[i].osd_viewable_x[0] = osd_viewable_x[0];
				move_step[i].osd_viewable_y[0] = osd_viewable_y[0];
				move_step[i].osd_viewable_w[0] = osd_viewable_w[0];
				move_step[i].osd_viewable_h[0] = osd_viewable_h[0];

				move_step[i].osd_layer[1] = osd_layer_old_top_view;
				move_step[i].osd_offset_x[1] = XM_lcdc_osd_get_x_offset (lcdc_channel, osd_layer_old_top_view);
				move_step[i].osd_offset_y[1] = XM_lcdc_osd_get_y_offset (lcdc_channel, osd_layer_old_top_view);
				move_step[i].osd_brightness[1] = 64;

				// �ײ���ͼ��ˮƽ���򱣳ֲ���
				move_step[i].osd_viewable_x[1] = osd_viewable_x[1];
				move_step[i].osd_viewable_w[1] = osd_viewable_w[1];
				// ���õײ���ͼ�Ŀ�����ƫ��
				move_step[i].osd_viewable_y[1] = osd_viewable_y[1] + osd_viewable_h[1] - offset;
				// ���õײ���ͼ�Ŀ������߶�
				move_step[i].osd_viewable_h[1] = offset;


				// ���ƶ���OSD��Ϊ2
				move_step[i].osd_count = 2;
			}
		}
	}
	// ��OSD�㶨λ����ʼλ�ã�׼����ʼanimate����
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
		// ����OSD��ͬʱ�ƶ�
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

// ������ͼ�л�������ʱ�估λ�ã��������ƶ���OSD�㶨λ����ʼλ��
static void calc_alert_view_move_step_and_setup_initial_osd_layer_position ( 
								  unsigned int lcdc_channel, 
								  unsigned int moving_osd_layer,
								  unsigned int view_offset,		// �Ӵ������OSD���ƫ��
								  unsigned int view_height,		// �Ӵ��ĸ߶�
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
	unsigned int bottom_layer;	// �ײ�OSD
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

	// ��ȡLCDC��Ƶ���ͨ���ĳߴ�
	lcdc_width = HW_lcdc_get_xdots (lcdc_channel);
	lcdc_height = HW_lcdc_get_ydots (lcdc_channel);
	// ��ȡOSD�ĳߴ�
	osd_width = XM_lcdc_osd_get_width   (lcdc_channel, moving_osd_layer);
	osd_height = XM_lcdc_osd_get_height (lcdc_channel, moving_osd_layer);

	// ���ײ��OSD��
	bottom_layer = XM_OSD_LAYER_1;	// ȱʡΪOSD 1
	if(XM_lcdc_osd_get_enable (lcdc_channel, bottom_layer) == 0)
	{
		bottom_layer = XM_OSD_LAYER_0;	// �ײ�Ϊ��Ƶ��	
	}
	bottom_osd_width = XM_lcdc_osd_get_width   (lcdc_channel, bottom_layer);
	bottom_osd_height = XM_lcdc_osd_get_height (lcdc_channel, bottom_layer);


	// OSD������ʾ

	// 1) ���¶��� (����ҪOSD2��)
	if(moving_direction == XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP)
	{
		// VIEW������LCD�ײ���ʼ�����ƶ���LCD����
		osd_start_x = (lcdc_width - osd_width) / 2;
		osd_start_y = lcdc_height - (lcdc_height - osd_height) / 2;
		osd_end_x = (lcdc_width - osd_width) / 2;
		osd_end_y = (lcdc_height - osd_height) / 2;

		osd_start_y -= view_offset;
		osd_end_y = osd_start_y - view_height - (osd_height - (view_offset + view_height));

		// �˴���ʵ��, 1���ڴӵײ������Ƶ�����
		move_step_count = STEP_COUNT;
		for (i = 0; i < move_step_count; i ++)
		{
			move_step[i].delay_time = DELAY_CYCLE;		// 100����
			move_step[i].lcdc_channel = lcdc_channel;
			
			// ���µ���ͼ�ƶ����ϵ���ͼ����
			move_step[i].osd_layer[0] = moving_osd_layer;
			move_step[i].osd_offset_x[0] = XM_lcdc_osd_horz_align (lcdc_channel, osd_start_x);
			move_step[i].osd_offset_y[0] = XM_lcdc_osd_vert_align (lcdc_channel, osd_start_y + (osd_end_y - osd_start_y) * i / (STEP_COUNT-1));
			move_step[i].osd_brightness[0] = 64;

			// �ײ���ͼ�Ŀ��������ֲ��䣬���޸�����ֵ
			move_step[i].osd_layer[1] = bottom_layer;
			//move_step[i].osd_offset_x[1] = XM_lcdc_osd_horz_align (lcdc_channel, (lcdc_width - bottom_osd_width) / 2);
			//move_step[i].osd_offset_y[1] = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - bottom_osd_height) / 2);
			move_step[i].osd_offset_x[1] = XM_lcdc_osd_get_x_offset (lcdc_channel, bottom_layer);
			move_step[i].osd_offset_y[1] = XM_lcdc_osd_get_y_offset (lcdc_channel, bottom_layer);
			move_step[i].osd_brightness[1] = 64 - i*2;
			

			// ���ƶ���OSD��Ϊ1
			move_step[i].osd_count = 2;
		}
	}
	// 1) ���϶���
	else if(moving_direction == XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM)
	{
		int temp;
		// VIEW������LCD������ʼ�����ƶ���LCD�ײ�
		osd_start_x = (lcdc_width - osd_width) / 2;
		osd_start_y = lcdc_height - (lcdc_height - osd_height) / 2;
		osd_end_x = (lcdc_width - osd_width) / 2;
		osd_end_y = (lcdc_height - osd_height) / 2;

		osd_start_y = osd_start_y - view_offset;
		osd_end_y = osd_start_y - view_height - (osd_height - (view_offset + view_height));

		temp = osd_start_y;
		osd_start_y = osd_end_y;
		osd_end_y = temp;

		// �˴���ʵ��, 1���ڴӵײ������Ƶ�����
		move_step_count = STEP_COUNT;
		for (i = 0; i < move_step_count; i ++)
		{
			move_step[i].delay_time = DELAY_CYCLE;		// 100����
			move_step[i].lcdc_channel = lcdc_channel;
			
			// ���µ���ͼ�ƶ����ϵ���ͼ����
			move_step[i].osd_layer[0] = moving_osd_layer;
			move_step[i].osd_offset_x[0] = XM_lcdc_osd_horz_align (lcdc_channel, osd_start_x);
			move_step[i].osd_offset_y[0] = XM_lcdc_osd_vert_align (lcdc_channel, osd_start_y + (osd_end_y - osd_start_y) * i / (STEP_COUNT-1));
			move_step[i].osd_brightness[0] = 64;
			
			// �ײ���ͼ�Ŀ��������ֲ��䣬���޸�����ֵ
			move_step[i].osd_layer[1] = bottom_layer;
		//	move_step[i].osd_offset_x[1] = XM_lcdc_osd_horz_align (lcdc_channel, (lcdc_width - bottom_osd_width) / 2);
		//	move_step[i].osd_offset_y[1] = XM_lcdc_osd_vert_align (lcdc_channel, (lcdc_height - bottom_osd_height) / 2);
			move_step[i].osd_offset_x[1] = XM_lcdc_osd_get_x_offset (lcdc_channel, bottom_layer);
			move_step[i].osd_offset_y[1] = XM_lcdc_osd_get_y_offset (lcdc_channel, bottom_layer);
			move_step[i].osd_brightness[1] = 64 - (10 - i)*2;
			
			// ���ƶ���OSD��Ϊ1
			move_step[i].osd_count = 2;
		}
	}

}

static void start_move_animate (MOVESTEP *movestep, int step_count)
{
		// OSD���ƶ�
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

		// ����Ƿ���ڵڶ����ƶ���OSD��
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
		// ��ʱ
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

		// OSD���ƶ�
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

		// ����Ƿ���ڵڶ����ƶ���OSD��
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
//    ����View��OSD 2���ܱ߿�ʼ�ƶ����𲽸�������OSD 2�㡣�������Ǻ�View��ʾ��OSD 2��Ǩ�Ƶ�OSD 1�㣬OSD 2�رա�
//		
//		�������Ӵ�����(�Ӵ�ջѹ�����)
//		�������Ӵ�(TOPVIEW), ����Ϊ��
//		�´����Ӵ�(NEWVIEW),
//		1) ��ȡ��ǰOSD 1���framebuffer_osd_1_topview������Ϊ��(NULL) 
//		2) ����һ��OSD 2���framebuffer_osd_2_newview, �����ƶ���������OSD 2�������LCDC���ԭ�������
//		3) ��framebuffer_osd_newview�ϸ�ʽ�����NEWVIEW������(XM_PAINT��Ϣ)
//		4) Ӧ��OSD 2�㲢��ʾ, XM_osd_framebuffer_close (framebuffer_osd_newview)
//		5) ���㲽��������������ʱ��㣬������ʱ�����񣬵ȴ��ƶ������ź��¼�
//			��ʱ������
//			5.1) ���ղ�����ʱ��㣬����OSD 2���offset
//			5.2) ѭ��ֱ����������Ϊ0
//			5.3) �����ƶ������ź���
//		6) �ͷ�OSD 1���framebuffer (OSD 1�ر�)
//		7)	����һ��OSD 1��� framebuffer_osd_1_newview
//		8) ��framebuffer_osd_2_newview�����ü����������ݸ��Ƶ�framebuffer_osd_1_newview
//		9)	OSD 1�㿪����OSD 2��ر�
//		10)animate���̽���


// B) XM_OSDLAYER_ANIMATE_MODE_VIEW_REMOVE
//    ����View��������OSD 2�㣬Ȼ�����ܱ߿�ʼ�ƶ������Ƴ�����OSD 2�㡣�����Ƴ���OSD 2�رա�
//		�����Ӵ��ر�(�Ӵ�ջ��������)
//		�����Ӵ�(PULLVIEW), ���������Ӵ� OSD2
//		��ջ���Ӵ�(TOPVIEW), ����Ϊ��, OSD1
//		1) ����һ��OSD 2��� framebuffer_osd_2_pullview
//		2) ��framebuffer_osd_2_pullview�ϸ�ʽ��PULLVIEW������(XM_PAINT��Ϣ)
//		3) �ͷ�OSD 1���framebuffer (OSD 1�ر�)
//		4) ����һ��OSD 1��� framebuffer_osd_1_topview (��Ϊ�գ������ǵĶ����Ӵ�)
//		5) �� framebuffer_osd_1_topview �ϸ�ʽ�����TOPVIEW������(XM_PAINT��Ϣ)
//		6) ���㲽��������������ʱ��㣬������ʱ�����񣬵ȴ��ƶ������ź��¼�
//			��ʱ������
//			6.1) ���ղ�����ʱ��㣬����OSD 2���offset
//			6.2) ѭ��ֱ����������Ϊ0
//			6.3) �����ƶ������ź���
//		7) �ͷ�OSD 2���framebuffer (OSD 2�ر�)
//		8) animate���̽���

// C) XM_OSDLAYER_ANIMATE_MODE_ALERT_CREATE
//    ����View��OSD 2���ܱ߿�ʼ�ƶ����𲽸�������OSD 2�㡣
//		
//		���ڶԻ���(Dialog/Alert��)����

// D) ����View��OSD 2���м����ܱ߿�ʼ�ƶ����𲽽�OSD 2���Ƴ���ʾ��������OSD 2�ر�
//		
//		���ڶԻ���(Dialog/Alert��)�ݻ�


// OSD���Ӵ��ƶ�
void XM_osd_layer_animate (
									unsigned int animating_mode,		// OSD�ƶ�ģʽ
									unsigned int moving_direction,	// �ƶ�����
									HANDLE old_top_view,					// �ϵĶ���ͼ���
									HANDLE new_top_view					// �µĶ���ͼ���
									)
{
	xm_osd_framebuffer_t framebuffer_osd_1, framebuffer_osd_2;
	XMRECT rc;

	unsigned int osd_viewable_x[2];
	unsigned int osd_viewable_y[2];
	unsigned int osd_viewable_w[2];
	unsigned int osd_viewable_h[2];

	// �����ϵĿ�����λ��
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

	XM_GetDesktopRect (&rc);	// ����������ͼ�Ĵ�С����OSD�ߴ�

	// ��ͨ��ͼ
	if(	animating_mode == XM_OSDLAYER_ANIMATE_MODE_VIEW_CREATE
		||	animating_mode == XM_OSDLAYER_ANIMATE_MODE_EVENT_CREATE
		)
	{

		// ����OSD���ӳ��޸ġ��ȴ�OSD����ȫ����ʼ����Ϻ����½���OSD��
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, 
				(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1),
				(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1) );

		// ����һ��OSD 2���framebuffer_osd_2_newview
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
		//		3) ��framebuffer_osd_newview�ϸ�ʽ�����NEWVIEW������(XM_PAINT��Ϣ)
		_xm_paint_view (new_top_view, framebuffer_osd_2);


		// ����moving����(����������������ʱ���)�������ó�ʼ�ƶ���ʼλ��
		calc_move_step_and_setup_initial_osd_layer_position (
				XM_LCDC_CHANNEL_0, 
				XM_OSD_LAYER_1,
				XM_OSD_LAYER_2, 
				animating_mode,
				moving_direction);

		//	����OSD 2�㲢��ʾ, XM_osd_framebuffer_close (framebuffer_osd_newview)
		XM_osd_framebuffer_close (framebuffer_osd_2, 0);

		//		5) ���㲽��������������ʱ��㣬������ʱ�����񣬵ȴ��ƶ������ź��¼�
		//			��ʱ������
		//			5.1) ���ղ�����ʱ��㣬����OSD 2���offset
		//			5.2) ѭ��ֱ����������Ϊ0
		//			5.3) �����ƶ������ź���
		start_move_animate (move_step, move_step_count);

		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1), 0);

		// ִ��view��Ⱦ����
		do_move_animate (move_step, move_step_count);


		// ����OSD�����޸�
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, 
			(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1), 
			(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1) );

		//		6) �ͷ�OSD 1���framebuffer (OSD 1�ر�)
		XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1);

		//		7)	����һ��OSD 1��� framebuffer_osd_1_newview
		framebuffer_osd_1 = XM_osd_framebuffer_create (
					XM_LCDC_CHANNEL_0,
					XM_OSD_LAYER_1,
					XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
					rc.right - rc.left + 1,
					rc.bottom - rc.top + 1,
					new_top_view,
					0, 0
					);

		//		8) ��framebuffer_osd_2_newview�����ü����������ݸ��Ƶ�framebuffer_osd_1_newview
		if(framebuffer_osd_1)
		{
			int osd2_x, osd2_y;
			// ������Ƶ���������� 
			XM_osd_framebuffer_copy (framebuffer_osd_1, framebuffer_osd_2);
			// ��ȡOSD 2��λ�ò����õ�OSD 1
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

			//		9)	OSD 1�㿪����
			XM_osd_framebuffer_close (framebuffer_osd_1, 0);
		}
		
		// OSD 2��ر�
		XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);
		//		10)animate���̽���

		// ���ջ��TOPVIEW�Ƿ�������
		if(new_top_view == XM_GetDesktop())
		{
			// �ر������Ӧ��OSD��, ��ʡˢ�´���
			// (������һ�����ص�ϵͳ���ƴ��ڣ������¼���ȱʡ������֧��ʵ����ʾ)
			XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1);
		}

		// �ָ���������
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

		// ����OSD�����޸�
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1), 0);

	}
	else if(	animating_mode == XM_OSDLAYER_ANIMATE_MODE_VIEW_REMOVE
			||	animating_mode == XM_OSDLAYER_ANIMATE_MODE_EVENT_REMOVE
		)
	{
		// B) XM_OSDLAYER_ANIMATE_MODE_VIEW_REMOVE
		//    ����View��������OSD 2�㣬Ȼ�����ܱ߿�ʼ�ƶ������Ƴ�����OSD 2�㡣�����Ƴ���OSD 2�رա�
		//		�����Ӵ��ر�(�Ӵ�ջ��������)
		//		�����Ӵ�(PULLVIEW), ���������Ӵ� OSD2
		//		��ջ���Ӵ�(TOPVIEW), ����Ϊ��, OSD1

		// ����OSD1/OSD2�������κ��޸�
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, 
				(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1),
				(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1) );

		// ��OSD 1���ڣ�
		//		{ ��OSD 1�����ݸ��Ƶ�OSD 2. OSD 2������OSD 1�ر� }
		//		1) ��OSD 1���framebuffer_osd_1_pullview
		framebuffer_osd_1 = XM_osd_framebuffer_open (
					XM_LCDC_CHANNEL_0,
					XM_OSD_LAYER_1,
					XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
					rc.right - rc.left + 1,
					rc.bottom - rc.top + 1
					);
		if(framebuffer_osd_1)
		{
			//		��OSD 1�ϵ���Ƶ���������ݸ��Ƶ�OSD 2
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
				// ��OSD1���֡���ݸ��Ƶ�OSD2��
				XM_osd_framebuffer_copy (framebuffer_osd_2, framebuffer_osd_1);
				//		OSD 2���޸Ľ���
				XM_osd_framebuffer_close (framebuffer_osd_2, 0);
			}
			XM_osd_framebuffer_close (framebuffer_osd_1, 0);
			// �ͷ�OSD1���֡���棬ͬʱ�ر�OSD1��
			XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1);
		}
		
		//		2) ��framebuffer_osd_1�ϸ�ʽ��TOPVIEW������(XM_PAINT��Ϣ)
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
	
			//			��ʾOSD1
			XM_osd_framebuffer_close (framebuffer_osd_1, 0);
		}

		// �ƶ�OSD2
		//		6) ���㲽��������������ʱ��㣬������ʱ�����񣬵ȴ��ƶ������ź��¼�
		calc_move_step_and_setup_initial_osd_layer_position (
				XM_LCDC_CHANNEL_0, 
				XM_OSD_LAYER_1,
				XM_OSD_LAYER_2, 
				animating_mode,
				moving_direction);

		//			��ʱ������
		//			6.1) ���ղ�����ʱ��㣬����OSD 2���offset
		//			6.2) ѭ��ֱ����������Ϊ0
		//			6.3) �����ƶ������ź���
		start_move_animate (move_step, move_step_count);

		// OSD1/OSD2��׼���á�����������OSD�������£�OSD�㿪������ʾ��
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1), 0);

		// ִ��view��Ⱦ����
		do_move_animate (move_step, move_step_count);

		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, 
			(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1),
			(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1));
		//		6) �ͷ�OSD 2���framebuffer (OSD 2�ر�)
		XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);
		//		7) animate���̽���

		// ���ջ��TOPVIEW�Ƿ�������
		if(new_top_view == XM_GetDesktop())
		{
			// �ر������Ӧ��OSD��
			// (������һ�����ص�ϵͳ���ƴ��ڣ������¼���ȱʡ������֧��ʵ����ʾ)
			XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_1);
		}

		// �ָ���������
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
	// ALERT(�Ի���)
	else if(animating_mode == XM_OSDLAYER_ANIMATE_MODE_ALERT_CREATE)
	{
		XMRECT rcAlert;
		// ����һ��OSD 2���framebuffer_osd_2_newview
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
		//		3) ��framebuffer_osd_newview�ϸ�ʽ�����NEWVIEW������(XM_PAINT��Ϣ)
		_xm_paint_view (new_top_view, framebuffer_osd_2);
		
		// ����moving����(����������������ʱ���)�������ó�ʼ�ƶ���ʼλ��
		// ��ȡalert��λ��
		XM_GetWindowRect (new_top_view, &rcAlert);

		// ����OSD���ӳ��޸ġ��ȴ�OSD����ȫ����ʼ����Ϻ����½���OSD��
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
		//	����OSD 2�㲢��ʾ, XM_osd_framebuffer_close (framebuffer_osd_newview)
		XM_osd_framebuffer_close (framebuffer_osd_2, 0);

		start_move_animate (move_step, move_step_count);

		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, 
			(1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1), 0);

		// ִ��view��Ⱦ����
		do_move_animate (move_step, move_step_count);
	}
	else if(animating_mode == XM_OSDLAYER_ANIMATE_MODE_ALERT_REMOVE)
	{
		XMRECT rcAlert;
		// ��һ��OSD 2���framebuffer_osd_2_newview
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
		
		// ����moving����(����������������ʱ���)�������ó�ʼ�ƶ���ʼλ��
		// ��ȡalert��λ��
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

		//	����OSD 2�㲢��ʾ, XM_osd_framebuffer_close (framebuffer_osd_newview)
		XM_osd_framebuffer_close (framebuffer_osd_2, 0);

		start_move_animate (move_step, move_step_count);

		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2) | (1 << XM_OSD_LAYER_1), 0);

		// ִ��view��Ⱦ����
		do_move_animate (move_step, move_step_count);

		//		6) �ͷ�OSD 2���framebuffer (OSD 2�ر�)
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2), (1 << XM_OSD_LAYER_2) );
		XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);
		XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2), 0);
	}
}
