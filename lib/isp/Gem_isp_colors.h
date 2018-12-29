// =============================================================================
// File        : Gem_isp_colors.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------

#ifndef  GEM_ISP_COLORS_H
#define  GEM_ISP_COLORS_H
#define GEM_COLORS_BASE (0x12c)
#define GEM_LUT_BASE (0x1a0)
#define GEM_DEMOSAIC_BASE (0x1a8)
#include "Gem_isp_ae.h"
#include "Gem_isp_awb.h"

struct isp_color_matrix_t
{
  unsigned short enable;// 1 bit
  short matrixcoeff[3][3]; // 16 bit��
};

struct isp_gamma_lut_t
{
  unsigned int enable;// 1 bit
  unsigned short gamma_lut[65];// 16 bit ֻд
};

struct isp_rgb2yuv_matrix_t
{
  signed short rgbcoeff[4][3];// 10 bit 
};

// demosaic����
// ��������(����/�͹�), �ο�20160628�Ĳ��Ա���
// demosaic��ȱʡ������������, 
// 	1) ���нϺõĽ�����, 
//		2) �ǳ���΢����ɫ����������̸�����(imatestʵ��),  
//		3) iso12233����������, �Ϻõ�α�ʼ����̸�����
//		4) 1844FLAG��ֹ��־, �Ϻõ�����ʮ�ֱ��غڿ�	
// mode = 0
// coff_00_07 = 32(40Ҳ����)
// demk = 128
// coff_20_27 = 255
// horz_thread = 0
// demDhv_ofst = 0
struct isp_demosaic_t {
	unsigned char mode;						// bit31  demmethod
													//	2��demosaic�㷨, 
													//	0 old  1 new
	
	unsigned char coff_00_07;				// bit7-0  demk1
													//	0 ~  7  �����ȵ�����Ҫ�޸ĵ�8λ��ֵ
													//		�ڰ������£�demk1��Ӱ��ǳ�΢����
													//		demk1�ɹ̶�Ϊһ�����ʵ�ֵ���κγ����£�
	
	unsigned short demk;						// bit19-8 demk, 12bit
													//		demkӰ�������,
													//			demk����0ʱ, ����coff_00_07�ı仯����Ӱ�������
													//			demk��Ϊ0ʱ, ���Ӳ���coff_00_07��ֵ����߽�����
													//
													//		���������սϰ�ʱ��Ӧ��Сdemk������demk����������ʱ���������(�������޷�ʹ��2D��crosstalk�Ƚ��뼼���˳�)
													//		�ڰ������£����������׳����ڵƹ�Ĺ��α߽��ϡ�(����ֵ��128��С��16)
	
	unsigned char coff_20_27;				// 20 ~ 27  һ��̶�Ϊ16
	
	unsigned short horz_thread;			// bit11-0  demDofst
													// ˮƽ�������ֵ (12bit, 0 ~ 4095), 
													//	ƽʱһ������Ϊ�̶�ֵ, ��Ҫ���ڷֱ��ʲ���ʱ΢��, �ﵽ��Ҫ�ķֱ��� 
	
	unsigned short demDhv_ofst;			// bit23-12 demDhv_ofst, (12bit, 0 ~ 4095)
													//		(32~ 256)
													//		һ������Ϊ0. ֵ����ʱ�׳������̸�����.
};

typedef enum{
   HDTV_type_0255=0,
   HDTV_type_16235,
   SDTV_type_0255,
   SDTV_type_16235,
}rgb_ypbpr_enum;

typedef struct isp_colors_
{
  unsigned int rgb2ypbpr_type;
  struct isp_color_matrix_t colorm;
  struct isp_gamma_lut_t gamma;
  struct isp_rgb2yuv_matrix_t rgb2yuv;
  
  struct isp_demosaic_t	demosaic;
} isp_colors_t;

typedef struct isp_colors_ *isp_colors_ptr_t;

void isp_colors_init (isp_colors_ptr_t p_colors);

void isp_colors_init_io (isp_colors_ptr_t p_colors);

void isp_colors_run (isp_colors_ptr_t p_colors, isp_awb_ptr_t p_awb, isp_ae_ptr_t p_ae);

void isp_create_rgb2ycbcr_matrix (unsigned int rgb2ycbcr_type, struct isp_rgb2yuv_matrix_t *matrix);


#endif
