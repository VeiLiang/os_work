// =============================================================================
// File        : Gem_isp_denoise.c
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------

#include "Gem_isp_denoise.h"
#include "Gem_isp_io.h"
#include <stdio.h>

void isp_denoise_init_io (isp_denoise_ptr_t p_denoise)
{
	int i, data0, data1, data2, data3, data4, data5, data6;
	
	// sensitiv 0 ~ 2 ����˲���0�˲�ǿ��, ֵԽ���ʾ����ĵ�Խ��
	//		0	 �˲��ر�
	//		7	 11��������˲�
	// sensitiv 3 ~ 5 ���ر����˲���1�˲�ǿ��
	// 
	data0 =  ((p_denoise->enable2d & 0x07) <<  0) 
			| ((p_denoise->enable3d & 0x07) <<  3) 
			| ((p_denoise->sel_3d_table & 0x03) << 8)	// 3D��˹��ѡ��
			| ((p_denoise->sensitiv0 & 0x07) << 10)	// 2D�˲���0(���)����������, 0 �˲��ر�	
			| ((p_denoise->sensitiv1 & 0x07) << 13)	// 2D�˲���1(����)����������, 0 �˲��ر�
			| ((p_denoise->sel_3d_matrix & 0x01) << 16)		// 3D����Ծ�������ѡ�� 0 �ڽ� 1 ���ĵ�
			;
	
	data1 =  ((p_denoise->y_thres0 & 0x3FF) <<  0) 
			| ((p_denoise->u_thres0 & 0x3FF) << 10) 
			| ((p_denoise->v_thres0 & 0x3FF) << 20);
	data2 =  ((p_denoise->y_thres1 & 0x3FF) <<  0) 
			| ((p_denoise->u_thres1 & 0x3FF) << 10) 
			| ((p_denoise->v_thres1 & 0x3FF) << 20);
	data3 =  ((p_denoise->y_thres2 & 0x3FF) <<  0) 
			| ((p_denoise->u_thres2 & 0x3FF) << 10) 
			| ((p_denoise->v_thres2 & 0x3FF) << 20);  
	
	// strength�ѷ���������ʹ��
	data4 = (p_denoise->y_strength0) | (p_denoise->u_strength0<<10) | (p_denoise->v_strength0<<20);
	data5 = (p_denoise->y_strength1) | (p_denoise->u_strength1<<10) | (p_denoise->v_strength1<<20);
	data6 = (p_denoise->y_strength2) | (p_denoise->u_strength2<<10) | (p_denoise->v_strength2<<20);
	
	Gem_write ((GEM_DENOISE_BASE+0x00), data0);
	Gem_write ((GEM_DENOISE_BASE+0x04), data1);
	Gem_write ((GEM_DENOISE_BASE+0x08), data2);
	Gem_write ((GEM_DENOISE_BASE+0x0c), data3); 
	Gem_write ((GEM_DENOISE_BASE+0x10), data4);
	Gem_write ((GEM_DENOISE_BASE+0x14), data5);
	Gem_write ((GEM_DENOISE_BASE+0x18), data6); 
	
	for (i = 0; i < 17; i++)
	{
		data0 = (0x06) | (i << 8) | ((p_denoise->noise0[i] & 0xFF) << 16);
		Gem_write ((GEM_LUT_BASE+0x00), data0);
	}
	
	for (i = 0; i < 17; i++)
	{
		data0 = (0x07) | (i << 8) | ((p_denoise->noise1[i] & 0xFF) <<16);
		Gem_write ((GEM_LUT_BASE+0x00), data0);
	}
}









