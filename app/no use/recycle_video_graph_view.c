//****************************************************************************
//
//	Copyright (C) 2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: recycle_video_graph_view.c
//	  可循环视频饼图视图
//
//	Revision history
//
//		2014.05.17	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_voice.h"
#include "xm_app_menudata.h"

#include <math.h>
#include <assert.h>

#define	pai		3.1415926f

#define	MAX_REGION		2		// 允许一个点可落入的区域的最大个数

#define	CYCLE_SIZE	120
#define	IMAGE_SIZE	300

#ifdef MAKE_TABLE
// 数组每个元素表示该坐标(X,Y)对应的点属于的等分区域(0 ~ 299). 0xFFFF表示该点不属于等分区域
static float region_diff_12[MAX_REGION][CYCLE_SIZE*CYCLE_SIZE];

static unsigned short region_12[MAX_REGION][CYCLE_SIZE*CYCLE_SIZE];

static unsigned short region[CYCLE_SIZE*CYCLE_SIZE];
static int create_region = 1;
static unsigned short region_diff_12_fixed[MAX_REGION][CYCLE_SIZE*CYCLE_SIZE];
#else
#include "recycle_video_graph_math.c"
#endif

unsigned int rcolor[IMAGE_SIZE];


void recycle_video_graph_setup (float recycle_percent, float locked_percent, float others_percent)
{
	int i;
	int recycle_ratio, locked_ratio, others_ratio;

	recycle_ratio = (int)(recycle_percent * 300.0 + 0.5);	
	locked_ratio = (int)( (recycle_percent + locked_percent) * 300.0 + 0.5);
	others_ratio = (int)( (recycle_percent + locked_percent + others_percent) * 300.0 + 0.5);

	for (i = 0; i < IMAGE_SIZE; i ++)
	{
		if(i < recycle_ratio)
			rcolor[i] = 0x00008000;
		else if(i < locked_ratio)
			rcolor[i] = 0x00800000;
		else
			rcolor[i] = 0x00000080;
	}
}

#ifdef MAKE_TABLE

static int init = 0;
void recycle_video_graph_init (void)
{
	if(init == 1)
		return;


	create_region = 1;
	
	memset (region_diff_12, 0, sizeof(region_diff_12));
	memset (region_12, 0xFF, sizeof(region_12));
	memset (region, 0xFF, sizeof(region));

	init = 1;
}

static void calc_cycle_region (double org_x, double org_y, double r)
{
	int x, y;
	double dx, dy;
	float angle;	
	// 角度 90° ~ 0° ~ 270° ~ 180° ~ 90°顺时针伸缩
	unsigned short id;
	int region_index;
	float distance;
	float start = 90.0f;
	angle = 0.0f;
	while (angle < 360.0f)
	{
		//id = (unsigned short)(angle / 300.0 + 0.5);
		float f_id;
		start -= angle;
		if(start < 0.0f)
			start += 360.0f;

		f_id = (angle * 300.0f) / 360.0f;
		id = (unsigned short)(f_id + 0.5f);
		if(id >= 300)
			id = 300 - 1;
		//dx = org_x + r * cos (start * 2 * pai / 360.0f);
		//dy = org_y - r * sin (start * 2 * pai / 360.0f);
		dx = org_x + r * cos ((angle + 270.0) * 2 * pai / 360.0f);
		dy = org_y + r * sin ((angle + 270.0) * 2 * pai / 360.0f);
		angle += 0.1f;
		x = (int)(dx + 0.5f);
		y = (int)(dy + 0.5f);
		if(x < 0)
			x = 0;
		else if(x >= CYCLE_SIZE)
			x = CYCLE_SIZE - 1;
		if(y < 0)
			y = 0;
		else if(y >= CYCLE_SIZE)
			y = CYCLE_SIZE - 1;

		//distance = 0;
		// 计算距离
		distance = (float)sqrt((x - dx) * (x - dx) + (y - dy) * (y - dy));
		// 计算能量
		//distance = (float)((x - dx) * (x - dx) + (y - dy) * (y - dy));
		for (region_index = 0; region_index < MAX_REGION; region_index ++)
		{
			// 检查相同色码点是否已存在

			if(region_12[region_index][y*CYCLE_SIZE + x] == id)
			{
				if(region_diff_12[region_index][y*CYCLE_SIZE + x] > distance)
					region_diff_12[region_index][y*CYCLE_SIZE + x] = distance;
				break;
			}
			else if(region_12[region_index][y*CYCLE_SIZE + x] == 0xFFFF)
			{
				region_12[region_index][y*CYCLE_SIZE + x] = id;
				// 计算偏差
				// region_diff_12[region_index][y*CYCLE_SIZE + x] = fabs((float)(((double)id) - f_id));

				// 计算距离
				region_diff_12[region_index][y*CYCLE_SIZE + x] = distance;
				break;
			}
		}

		if(region_index == MAX_REGION)
		{
			// 比较距离
			double max_distance = 0;
			int max_idx = 0;
			for (region_index = 0; region_index < MAX_REGION; region_index ++)
			{
				if(region_diff_12[region_index][y*CYCLE_SIZE + x] > max_distance)
				{
					max_idx = region_index;
					max_distance = region_diff_12[region_index][y*CYCLE_SIZE + x];
				}
			}
			// 替换最大的项目
			if(distance < max_distance)
			{
				region_12[max_idx][y*CYCLE_SIZE + x] = id;
				region_diff_12[max_idx][y*CYCLE_SIZE + x] = distance;
			}
		}

	}
}
#endif

void recycle_video_graph_draw (u8_t *framebuf, unsigned int linelength, 
										 float recycle_percent,
										 float locked_percent,
										 float others_percent)
{
#ifdef MAKE_TABLE
	int x, y;
#endif

	int i, j;

#ifdef MAKE_TABLE
	recycle_video_graph_init ();
#endif

	recycle_video_graph_setup (recycle_percent, locked_percent, others_percent);


#ifdef MAKE_TABLE
	// 第三步 按300等分，将内圈与外圆之间的点分为300个1.2°的扇形
	if(create_region)
	{
		double r1, r2;
		int graph_x, graph_y;
		int graph_r;		// 半径

		graph_x = (int)(CYCLE_SIZE / 2.0f);
		graph_y = (int)(CYCLE_SIZE / 2.0f);
		graph_r = (int)(CYCLE_SIZE / 2.0f);
		memset (region, 0xFF, sizeof(region));

		//r1 = graph_r - 30; 
		r1 = 0;
		r2 = graph_r;
		while(r1 <= r2)
		{
			calc_cycle_region (graph_x, graph_y, r1);
			r1 += 0.1;
		}

		// 扫描region_1及region_2，计算region
		for (y = 0; y < CYCLE_SIZE; y ++)
		{
			for (x = 0; x < CYCLE_SIZE; x ++)
			{

				if(	region_12[0][y * CYCLE_SIZE + x] == region_12[1][y * CYCLE_SIZE + x]
					&& region_12[0][y * CYCLE_SIZE + x] != 0xFFFF)
				{
					// 相同颜色
					region[y*CYCLE_SIZE + x] = region_12[0][y * CYCLE_SIZE + x];
				}
				else if(region_12[0][y * CYCLE_SIZE + x] == 0xFFFF && region_12[1][y * CYCLE_SIZE + x] == 0xFFFF)
				{
					// 没有填充
				}
				else if(region_12[0][y * CYCLE_SIZE + x] != 0xFFFF && region_12[1][y * CYCLE_SIZE + x] != 0xFFFF)
				{
					// 两个区均占据改点
					// 需要融合处理
#ifdef SIMPLE
					// 取最小值
					if(region_diff_12[0][y * CYCLE_SIZE + x] < region_diff_12[1][y * CYCLE_SIZE + x])
						region[y * CYCLE_SIZE + x] = region_12[0][y * CYCLE_SIZE + x];
					else
						region[y * CYCLE_SIZE + x] = region_12[1][y * CYCLE_SIZE + x];
#else
					region[y * CYCLE_SIZE + x] = 0xFFFE;		// 标记该点需要融合处理
#endif
				}
				else if(region_12[0][y * CYCLE_SIZE + x] != 0xFFFF)
				{
					// 仅一个区占据该点
					region[y * CYCLE_SIZE + x] = region_12[0][y * CYCLE_SIZE + x];
				}
				else
				{
					assert (0);
				}

			}
		}

		// 将浮点优化为定点
		for (j = 0; j < CYCLE_SIZE; j ++)
		{
			for (i = 0; i < CYCLE_SIZE; i ++)
			{
				int index = j * CYCLE_SIZE + i;
				int diff;
				diff = (unsigned int)(region_diff_12[0][index] * 8192.0 + 0.5);
				if(diff > 32767)
				{
					XM_printf ("diff %d NG\n", diff);
				}
				region_diff_12_fixed[0][index] = (unsigned short)diff;
				diff = (unsigned int)(region_diff_12[1][index] * 8192.0 + 0.5);
				if(diff > 32767)
				{
					XM_printf ("diff %d NG\n", diff);
				}
				region_diff_12_fixed[1][index] = (unsigned short)diff;;
				if(region[index] == 0xFFFE)
				{
					if(region_diff_12_fixed[0][index] == 0 && region_diff_12_fixed[1][index] == 0)
					{
						XM_printf ("illegal fixed, i=%d, j=%d\n", i, j);
					}
				}
			}
		}

#ifdef _WINDOWS
		{
			FILE *fp;
			int i;
			fp = fopen ("recycle_video_graph_math.c", "w");
			if(fp)
			{
				fprintf (fp, "static unsigned short region[CYCLE_SIZE*CYCLE_SIZE] = \n{\n");
				
				for (i = 0; i < CYCLE_SIZE*CYCLE_SIZE/8; i ++)
				{
					fprintf (fp, "\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\n",
						region[i*8], region[i*8 + 1], region[i*8 + 2], region[i*8 + 3],
						region[i*8 + 4], region[i*8 + 5], region[i*8 + 6], region[i*8 + 7]);

				}
				
				fprintf (fp, "};\n");


				fprintf (fp, "static unsigned short region_12[MAX_REGION][CYCLE_SIZE*CYCLE_SIZE] = \n{\n");
				
				fprintf (fp, "\t{\n");
				for (i = 0; i < CYCLE_SIZE*CYCLE_SIZE/8; i ++)
				{
					fprintf (fp, "\t\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\n",
						region_12[0][i*8], region_12[0][i*8 + 1], region_12[0][i*8 + 2], region_12[0][i*8 + 3],
						region_12[0][i*8 + 4], region_12[0][i*8 + 5], region_12[0][i*8 + 6], region_12[0][i*8 + 7]);

				}
				fprintf (fp, "\t},\n");
				
				fprintf (fp, "\t{\n");
				for (i = 0; i < CYCLE_SIZE*CYCLE_SIZE/8; i ++)
				{
					fprintf (fp, "\t\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\n",
						region_12[1][i*8], region_12[1][i*8 + 1], region_12[1][i*8 + 2], region_12[1][i*8 + 3],
						region_12[1][i*8 + 4], region_12[1][i*8 + 5], region_12[1][i*8 + 6], region_12[1][i*8 + 7]);

				}
				fprintf (fp, "\t},\n");
				fprintf (fp, "};\n");



				fprintf (fp, "\nstatic unsigned int region_diff_12_fixed[MAX_REGION][CYCLE_SIZE*CYCLE_SIZE] = \n{\n");
				fprintf (fp, "\t{\n");
				for (i = 0; i < CYCLE_SIZE*CYCLE_SIZE/8; i ++)
				{
					fprintf (fp, "\t\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\n",
						region_diff_12_fixed[0][i*8], region_diff_12_fixed[0][i*8 + 1], region_diff_12_fixed[0][i*8 + 2], region_diff_12_fixed[0][i*8 + 3],
						region_diff_12_fixed[0][i*8 + 4], region_diff_12_fixed[0][i*8 + 5], region_diff_12_fixed[0][i*8 + 6], region_diff_12_fixed[0][i*8 + 7]);

				}
				fprintf (fp, "\t},\n");
				
				fprintf (fp, "\t{\n");
				for (i = 0; i < CYCLE_SIZE*CYCLE_SIZE/8; i ++)
				{
					fprintf (fp, "\t\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\n",
						region_diff_12_fixed[1][i*8], region_diff_12_fixed[1][i*8 + 1], region_diff_12_fixed[1][i*8 + 2], region_diff_12_fixed[1][i*8 + 3],
						region_diff_12_fixed[1][i*8 + 4], region_diff_12_fixed[1][i*8 + 5], region_diff_12_fixed[1][i*8 + 6], region_diff_12_fixed[1][i*8 + 7]);

				}
				fprintf (fp, "\t},\n");


				fprintf (fp, "};\n");

				fclose (fp);
			}
		}
#endif

		create_region = 0;
	}
#endif
	
	for (j = 0; j < CYCLE_SIZE; j ++)
	{
		for (i = 0; i < CYCLE_SIZE; i ++)
		{
			unsigned int cc;
			int index = j * CYCLE_SIZE + i;
			if(region[index] == 0xFFFF)
			{
			}
			else if(region[index] == 0xFFFE)
			{
				
				// 需要融合的点
				unsigned int cc_1 = rcolor[region_12[0][index]];
				unsigned int cc_2 = rcolor[region_12[1][index]];
				if(cc_1 != cc_2)
				{
					unsigned int ratio1 = region_diff_12_fixed[0][index];
					unsigned int ratio2 = region_diff_12_fixed[1][index];
					unsigned int r1, g1, b1, r2, g2, b2;
					unsigned int r, g, b;
					
					r1 = (cc_1 >> 16) & 0xFF;
					g1 = (cc_1 >>  8) & 0xFF;
					b1 = (cc_1 >>  0) & 0xFF;
					r2 = (cc_2 >> 16) & 0xFF;
					g2 = (cc_2 >>  8) & 0xFF;
					b2 = (cc_2 >>  0) & 0xFF;
					r = (unsigned int)((ratio2 * r1 + ratio1 * r2 + (ratio1 + ratio2)/2 ) / (ratio1 + ratio2));
					g = (unsigned int)((ratio2 * g1 + ratio1 * g2 + (ratio1 + ratio2)/2) / (ratio1 + ratio2));
					b = (unsigned int)((ratio2 * b1 + ratio1 * b2 + (ratio1 + ratio2)/2) / (ratio1 + ratio2));
					if(r > 255)
						r = 255;
					if(g > 255)
						g = 255;
					if(b > 255)
						b = 255;
					cc = XM_RGB(r,g,b);
				}
				else
				{
					cc = cc_1;
				}
				*(unsigned int *)(framebuf + j * linelength + i * 4) = 0xF0000000 | cc;
			}
			else
			{
				unsigned int color_index = region[index];
				cc = rcolor[color_index];
				*(unsigned int *)(framebuf + j * linelength + i * 4) = 0xF0000000 | cc;
			}			
		}
	}

}