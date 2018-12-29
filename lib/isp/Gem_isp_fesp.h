// =============================================================================
// File        : Gem_isp_fesp.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#ifndef  _GEM_ISP_FESP_H_
#define  _GEM_ISP_FESP_H_

#define GEM_LENS_BASE     (0x0e0)
#define GEM_FIXPATT_BASE  (0x0f0)
#define GEM_BADPIX_BASE   (0x100)
#define GEM_CROSS_BASE    (0x108)
#define GEM_LUT_BASE      (0x1a0)

#define GEM_LENS_LSCOFST_BASE	(0x6c * 4)

#include "Gem_isp_ae.h"

struct Lensshade_t 
{
  unsigned char enable; 		// 1 bit
  
  unsigned char scale; 			// 2 bit
  										// ��ͷ��Ӱ�����ֵ����߶�
										//	0: 16�� 1: 32�� 2: 64�� 3: 128��		
  
  unsigned short coef[195]; 		// 16 bit  
  										// coef[0]~coef[64]    R������Ӱ�����ֵϵ��
  										// coef[65]~coef[129]  G������Ӱ�����ֵϵ��
  										//	coef[130]~coef[194] B������Ӱ�����ֵϵ��
  unsigned int rcenterRx;		// 16 bit ��ͷ��ӰR����x��������
  unsigned int rcenterRy;		// 16 bit ��ͷ��ӰR����y��������
  unsigned int rcenterGx;		// 16 bit ��ͷ��ӰG����x��������
  unsigned int rcenterGy;		// 16 bit ��ͷ��ӰG����y��������
  unsigned int rcenterBx;		// 16 bit ��ͷ��ӰB����x��������
  unsigned int rcenterBy;		// 16 bit ��ͷ��ӰB����y��������
  
  unsigned int lscofst;			// 16bit
};

struct Fixpatt_t
{
  unsigned short enable;
  unsigned short mode;
  unsigned short rBlacklevel;		// 16 bit, ��Чλ8bit (0 ~ 255).
  unsigned short grBlacklevel;	// 16 bit, ��Чλ8bit (0 ~ 255).
  unsigned short gbBlacklevel;	// 16 bit, ��Чλ8bit (0 ~ 255).
  unsigned short bBlacklevel;		// 16 bit, ��Чλ8bit (0 ~ 255).
  unsigned char profile[17];		// (0 ~ 255)

};

// BadPixel������ȥ������4���ٽ��߽��ϵ�����㣨��demosaic�㷨���룿��
struct Badpix_t
{
  unsigned char enable;				// 0 ��ֹ 1 ʹ��
  unsigned char mode; 				// 0 ��ͨģʽ 1 ���ģʽ
  unsigned short thresh;			// ��ͨģʽ�µ���ֵ
  											//		����Χ������ƫ����ڸ���ֵ�ĵ㽫�����˲�
  unsigned char profile[16]; 		// ��16���Ҷ����ȶ�Ӧ����ֵ
};

struct Crosstalk_t
{
	unsigned short enable;		// crosstalk enable, 1bit, (1: enable 0:disable)
	
	unsigned short mode; 		// crosstalk mode, 2bit �˲�ģʽ
										//			00: unite filter thres=128 
										//			10: use reg thres
										//				��ֵ�Ĵ���ģʽ
										//					����5x5��4��������ƽ��ֵavg, ��thres0cgf�Ƚ�; 
										//					���ݱȽϽ��, (avg > thres0cgf)ѡ��thres1cgf����(avg <= thres0cgf)ѡ��thres2cgf�����˲�������ֵ.
										//					��thres1cgf��thres2cgf��ͬʱ, ��ʱ��ֵ����ѡ��ͬһ��ֵ 
										//			x1: base on lut
										//				��ģʽ10�Ƚ�, �˲���ǿ�Ȳ�������ֵ�й�, ͬʱ�����ص�����ȴ�С���.
										//				ͨ�����ұ�profile[17], �ɶ���������ֵ��ص��˲�ǿ������.
										//				����, ����ֵ�ϵ͵�����(�����С), �˲�ǿ�����ӿ����ýϴ�, ��������˳�����; 
										//				����ֵ�ϴ������(����ȴ�), �˲�ǿ�����ӿ����ýϵ�, ���������˳����µĻ���ģ��.
										//			ģʽ11 �� ģʽ01 ���ǻ��ڲ��ұ�, �����ڲ���. 
										//			ģʽ01���˲�ǿ�ȸ���ģʽ11.
	
	// ������ֵ
	// ��ֵԽ��, �˲���ǿ��Խ��. 
	//		����thresh(thres2cgf)�ı仯���˲�Ч����Ӱ�����thres1cgf�ı仯��Ӱ��.
	//		���һ�����thresh(thres2cgf)
	unsigned short thresh;			// ������ֵ2, thres2cgf, 16bit, ��Ч��Χ 0 ~ 1023
											//		
	unsigned short thres1cgf;		// ������ֵ1, 16bit, ��Ч��Χ 0 ~ 1023
	
	unsigned short snsCgf;			//	2bit, 0 ~ 3
											//		ֵԽ��, �˳�����������Խ��. 
											//		��ֵ�ı仯������Ӱ�����.
											//		ȱʡֵ��������Ϊ3
	
	unsigned short thres0cgf;		// ��������ֵ�ȽϼĴ���, 16bit
	
	unsigned char profile[17];	// �˲���ǿ�����ò��ұ�, 8bit, ��Ч��Χ 0 ~ 255
											//		�������ȷ�������"���ұ�"ѡ���˲�ǿ��
											//		���ұ����ֵԽ��, ����ǿ��Խ��
											//			0   	�˲�ǿ����С
											//			255	�˲�ǿ�����
											//
											//	1) ��������ǿ�Ⱥܴ�ʱ(�����չ�ֱ��)���ع�ʱ�䳬�̣���������ϸ�����ع�ʱ�䲻�����������ܴ�
											//	   ͨ�����ұ�ͬ���������ã���ߵ�������(0 ~ 15, 16 ~ 31)���˲�ǿ����������ǿ���������˲���
											//		��С�������������˲�ǿ��ֵ�����ͷ����������˲�ǿ��, �Ӷ�������������������������������ϸ�ڡ�
};

typedef struct isp_fesp_
{ 
  struct Lensshade_t Lensshade;
  struct Fixpatt_t Fixpatt;
  struct Badpix_t Badpix;
  struct Crosstalk_t Crosstalk;
} isp_fesp_t;

typedef struct isp_fesp_ *isp_fesp_ptr_t;

void isp_fesp_init (isp_fesp_ptr_t p_fesp);

void isp_fesp_init_io (isp_fesp_ptr_t p_fesp);

void isp_fesp_run (isp_fesp_ptr_t p_fesp, isp_ae_ptr_t p_ae);

#endif