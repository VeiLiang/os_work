// =============================================================================
// File        : Gem_isp_colors.c
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------

#include "Gem_isp_colors.h"
#include "Gem_isp_io.h"
#include "stdio.h"


void isp_colors_init_io (isp_colors_ptr_t p_colors)
{
	unsigned int i, data0, data1, data2, data3, data4, data5;
  
   data0 = p_colors->colorm.enable;
   Gem_write ((GEM_COLORS_BASE+0x00), data0);
   
   data0 = (p_colors->colorm.matrixcoeff[0][0]) | (p_colors->colorm.matrixcoeff[0][1]<<16);
   data1 = (p_colors->colorm.matrixcoeff[0][2]) | (p_colors->colorm.matrixcoeff[1][0]<<16);
   data2 = (p_colors->colorm.matrixcoeff[1][1]) | (p_colors->colorm.matrixcoeff[1][2]<<16);
   data3 = (p_colors->colorm.matrixcoeff[2][0]) | (p_colors->colorm.matrixcoeff[2][1]<<16); 
   data4 = (p_colors->colorm.matrixcoeff[2][2]);
   Gem_write ((GEM_COLORS_BASE+0x04), data0);
   Gem_write ((GEM_COLORS_BASE+0x08), data1);
   Gem_write ((GEM_COLORS_BASE+0x0c), data2);
   Gem_write ((GEM_COLORS_BASE+0x10), data3);
   Gem_write ((GEM_COLORS_BASE+0x14), data4);   

   data0 = p_colors->gamma.enable;
   Gem_write ((GEM_COLORS_BASE+0x18), data0);
   
   for (i = 0; i < 65; i++)
   {
     data0 = (0x04) | (i << 8) | (p_colors->gamma.gamma_lut[i]<<16);
     Gem_write ((GEM_LUT_BASE+0x00), data0);
   }

   data0 = (p_colors->rgb2yuv.rgbcoeff[0][0]) | (p_colors->rgb2yuv.rgbcoeff[0][1]<<16); 
   data1 = (p_colors->rgb2yuv.rgbcoeff[0][2]) | (p_colors->rgb2yuv.rgbcoeff[1][0]<<16);
   data2 = (p_colors->rgb2yuv.rgbcoeff[1][1]) | (p_colors->rgb2yuv.rgbcoeff[1][2]<<16);
   data3 = (p_colors->rgb2yuv.rgbcoeff[2][0]) | (p_colors->rgb2yuv.rgbcoeff[2][1]<<16);
   data4 = (p_colors->rgb2yuv.rgbcoeff[2][2]) | (p_colors->rgb2yuv.rgbcoeff[3][0]<<16);
   data5 = (p_colors->rgb2yuv.rgbcoeff[3][1]) | (p_colors->rgb2yuv.rgbcoeff[3][2]<<16);
   Gem_write ((GEM_COLORS_BASE+0x20), data0); 
   Gem_write ((GEM_COLORS_BASE+0x24), data1);
   Gem_write ((GEM_COLORS_BASE+0x28), data2);
   Gem_write ((GEM_COLORS_BASE+0x2c), data3);
   Gem_write ((GEM_COLORS_BASE+0x30), data4);
   Gem_write ((GEM_COLORS_BASE+0x34), data5);
	
	// 新版本ISP增加demosaic功能, 支持2种demosaic算法
	//  0 ~  7  清晰度调整主要修改低8位的值, 最大值64
	// 20 ~ 27  一般固定为16
	// 31       固定为0值
	data0 = ((p_colors->demosaic.mode & 1) << 31)
			| ((p_colors->demosaic.coff_00_07 & 0xFF) << 0)
			| ((p_colors->demosaic.coff_20_27 & 0xFF) << 20)
			| ((p_colors->demosaic.demk & 0xFFF) << 8)		// demk	bit19-8, 12bit
			;
	
	// 水平方向的阈值 (0 ~ 4095), 
	//	平时一般设置为固定值, 主要用于分辨率测试时微调, 达到需要的分辨率 
	data1 = p_colors->demosaic.horz_thread & 0xFFF;
	
	// 垂直方向的阈值 (32 ~ 256)
	data1 |= (p_colors->demosaic.demDhv_ofst & 0xFFF) << 12;

	Gem_write ((GEM_DEMOSAIC_BASE+0x00), data0);
	Gem_write ((GEM_DEMOSAIC_BASE+0x04), data1);
}



void isp_create_rgb2ycbcr_matrix (unsigned int rgb2ycbcr_type, struct isp_rgb2yuv_matrix_t *matrix)
{
	if(rgb2ycbcr_type == SDTV_type_16235)
	{
		matrix->rgbcoeff[0][0] =  306;
		matrix->rgbcoeff[0][1] =  601;
	  	matrix->rgbcoeff[0][2] =  117;
		matrix->rgbcoeff[1][0] =  176;
		matrix->rgbcoeff[1][1] =  347;
		matrix->rgbcoeff[1][2] =  523;
		matrix->rgbcoeff[2][0] =  523;
		matrix->rgbcoeff[2][1] =  438;
		matrix->rgbcoeff[2][2] =  85;
		matrix->rgbcoeff[3][0] =  0;
		matrix->rgbcoeff[3][1] =  128;
		matrix->rgbcoeff[3][2] =  128;
	}
	// SDTV:0-255
	// Y601 =   0.257R′+  0.504G′ + 0.098B′ + 16
	// Cb   = –0.148R′– 0.291G′ + 0.439B′ + 128
	// Cr   =   0.439R′– 0.368G′ –0.071B′ + 128
	else if( rgb2ycbcr_type == SDTV_type_0255)
	{
		matrix->rgbcoeff[0][0] =  263;
		matrix->rgbcoeff[0][1] =  516;
		matrix->rgbcoeff[0][2] =  100;
		matrix->rgbcoeff[1][0] =  152;
		matrix->rgbcoeff[1][1] =  298;
		matrix->rgbcoeff[1][2] =  450;
		matrix->rgbcoeff[2][0] =  450;
		matrix->rgbcoeff[2][1] =  377;
		matrix->rgbcoeff[2][2] =  73;
		matrix->rgbcoeff[3][0] =  16;
		matrix->rgbcoeff[3][1] =  128;
		matrix->rgbcoeff[3][2] =  128;
	}
	// HDTV:16-235
	// Y709 = 0.213R′ + 0.715G′ + 0.072B′
	// Cb = –0.117R′ – 0.394G′ + 0.511B′ + 128
	// Cr = 0.511R′ – 0.464G′ – 0.047B′ + 128
	// Y = (218*R + 732*G + 74*B)/1024
	// Cb = (-120*R - 403*G + 524*B)/1024 + 128
	// Cr = (523*R - 475*G - 48*B)/1024 + 128
	else if( rgb2ycbcr_type == HDTV_type_16235)
	{
		matrix->rgbcoeff[0][0] = 218;
		matrix->rgbcoeff[0][1] = 732;
		matrix->rgbcoeff[0][2] = 74;
		matrix->rgbcoeff[1][0] = 120;
		matrix->rgbcoeff[1][1] = 403;
		matrix->rgbcoeff[1][2] = 524;
		matrix->rgbcoeff[2][0] = 523;
		matrix->rgbcoeff[2][1] = 475;
		matrix->rgbcoeff[2][2] = 48;
		matrix->rgbcoeff[3][0] = 0;
		matrix->rgbcoeff[3][1] = 128;
		matrix->rgbcoeff[3][2] = 128;
	}
	// HDTV:0-255
	// Y709 = 0.183R′ + 0.614G′ + 0.062B′ + 16
	// Cb = –0.101R′ – 0.338G′ + 0.439B′ + 128
	// Cr = 0.439R′ – 0.399G′ – 0.040B′ + 128
	else //if( rgb2ycbcr_type == HDTV_type_0255 )
	{
		// HDTV with offset
		matrix->rgbcoeff[0][0] = 187;
		matrix->rgbcoeff[0][1] = 629;
		matrix->rgbcoeff[0][2] = 63;
		matrix->rgbcoeff[1][0] = 103;
		matrix->rgbcoeff[1][1] = 346;
		matrix->rgbcoeff[1][2] = 450;
		matrix->rgbcoeff[2][0] = 450;
		matrix->rgbcoeff[2][1] = 409;
		matrix->rgbcoeff[2][2] = 41;
		matrix->rgbcoeff[3][0] = 16;
		matrix->rgbcoeff[3][1] = 128;
		matrix->rgbcoeff[3][2] = 128;
	}
}

void isp_set_color_coeff(unsigned int choice )
{
	unsigned int data0,data1,data2,data3,data4,data5; 
	struct isp_rgb2yuv_matrix_t rgb2ycbcr_matrix;
	
	//配置参数 
	isp_create_rgb2ycbcr_matrix (choice, &rgb2ycbcr_matrix);
	
	//写入寄存器 
   data0 = (rgb2ycbcr_matrix.rgbcoeff[0][0]) | (rgb2ycbcr_matrix.rgbcoeff[0][1]<<16); 
   data1 = (rgb2ycbcr_matrix.rgbcoeff[0][2]) | (rgb2ycbcr_matrix.rgbcoeff[1][0]<<16);
   data2 = (rgb2ycbcr_matrix.rgbcoeff[1][1]) | (rgb2ycbcr_matrix.rgbcoeff[1][2]<<16);
   data3 = (rgb2ycbcr_matrix.rgbcoeff[2][0]) | (rgb2ycbcr_matrix.rgbcoeff[2][1]<<16);
   data4 = (rgb2ycbcr_matrix.rgbcoeff[2][2]) | (rgb2ycbcr_matrix.rgbcoeff[3][0]<<16);
   data5 = (rgb2ycbcr_matrix.rgbcoeff[3][1]) | (rgb2ycbcr_matrix.rgbcoeff[3][2]<<16);
   Gem_write ((GEM_COLORS_BASE+0x20), data0); 
   Gem_write ((GEM_COLORS_BASE+0x24), data1);
   Gem_write ((GEM_COLORS_BASE+0x28), data2);
   Gem_write ((GEM_COLORS_BASE+0x2c), data3);
   Gem_write ((GEM_COLORS_BASE+0x30), data4);
   Gem_write ((GEM_COLORS_BASE+0x34), data5);

}
