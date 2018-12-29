// =============================================================================
// File        : Gem_isp_denoise.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#ifndef __GEM_ISP_DENOISE_HEADFILE__
#define __GEM_ISP_DENOISE_HEADFILE__

#include "Gem_isp_ae.h"

#define GEM_DENOISE_BASE  (0x164)
#define GEM_LUT_BASE  (0x1a0)

typedef struct isp_denoise_
{
  unsigned short enable2d;		// 3 bit 	space 2d denoise 
  										//		bit 0 	1 enable space 2d denoise, 0 disable 
  										// 	bit 1		�������ȷ���(Y)�˲�ǿ��ϵ����ѡ�񷽷�
  										//					0	�������ȷ�����Ԫ��ʹ����ͬ���˲�ǿ��ϵ��y_thres(������ֵ), �˲�ǿ�Ƚ���������ֵӰ��.
  										//					1	ʹ�ò��ұ�(��������ֵ)��������ֵ(y_thres)��ϵķ��������˲�ǿ��ϵ��,
  										//						ÿ�����ȷ�����Ԫ�ص��˲�����������ֵӰ��, Ҳ�ܸ÷���ֵ��С��Ӱ��
  										//							(ʹ�÷���ֵ��С����17�����Բ��ұ�õ���Ӧ�ķ���Ӱ������, ����ǿ������Խ��, �˲�ǿ��Խ��;
  										//							 ������Ӱ�����Ӿ�Ϊ���ֵ255ʱ, ��ʱ�˲���ͬ�ڽ������˲���ֵ�ķ���).
  										//	
  										//					1 ʹ�� filter0/1�� y_thre0/1* noise0_lutdata
  										//						������noise0_lutdata�ı���ֵ��Ϊ255ʱ, ��ʱ��ͬ�ڲ�ʹ�ñ��Ч��
  										//					0 ��ʹ��filter0/1�� y_thre0/1	
  										//		bit 2		����ɫ�ȷ���(CbCr)�˲�ǿ��ϵ����ѡ�񷽷�	
  										//					1 ʹ�� filter0/1�� u_thre0/1* noise0_lutdata �� v_thre0/1* noise0_lutdata
  										//					0 ��ʹ��filter0/1�� u_thre0/1 �� v_thre0/1 	
  										//
  										//		ͨ���Ƚ�, ����������Ϣ���˲�Ч���ǳ�����. ͨ��ɫ����Ϣ���˲�Ч���ǳ���΢.
  
  unsigned short enable3d;		// 3 bit 	temporty 3d denoise
  
  unsigned short sensitiv;		// 0724 �汾 3 bit  
  
  unsigned short sensitiv0;	// �°汾ISP���˲���0(���)������������, 3bit
  										//		ֵԽ��, ��ʾ���ڲ����˲��ĵ������Խ��, �ڽ���Ե�ǰ���Ӱ��Խ��, �˲�����ƽ��. 	
  										//		0		�˲����ر�
  										//		1		3�����ڵ�����˲�
  										//		5		��ʾ11����ȫ�������˲�.
  										//		6,7	��5��ͬ
  										//				0, 3, 5, 7, 9, 11��6�����ڵ�ѡ�񷽷�
  
  unsigned short sensitiv1;	// �°汾ISP���˲���1(����)������������, 3bit
  										//		ֵԽ��, ��ʾ���ڲ����˲��ĵ�����Խ��, �˲���ǿ��Խ��. 	
  										//		0		�˲����ر�
  										//		5		��ʾ11����ȫ�������˲�.
  										//		6,7	��5��ͬ
   
  
  unsigned short sel_3d_table;	// 3D ��˹��ѡ��(temp0-temp3)
  
  unsigned short sel_3d_matrix;	// 3D ѡ�����������
  											//		0		�ڽ����������
  											//		1		���ĵ��������
  
  // �˲���0��������ֵ���� (2D���)
  unsigned short y_thres0;		// ���ȷ���Y��ֵ,  10 bit ����Чֵ 0 ~ 1023
  										//		��������ʱ, �����(snr)��С.  ����Ƚϲ�(�ź�ǿ�Ȳ���, ����ǿ������, snr�½�)�����ص�����; 
  										//		�����˲���������ֵ, �����ڸ�������ֵ�ĸ���ĵ���������ص�����˲�����, ��������ͼ��������.	
  										//		��ֵԽ��, �����˲������ص�Խ��, �˲�Խƽ��, ͼ����������ڽ���(ͼ������ģ��)
  										
  unsigned short u_thres0;		// ɫ�ȷ���Cb��ֵ, 10 bit ����Чֵ 0 ~ 1023
  unsigned short v_thres0;		// ɫ�ȷ���Cr��ֵ, 10 bit ����Чֵ 0 ~ 1023

  // �˲���1��������ֵ���� (2D����), �����˲��ᱣ����Ե��ϸ�ڡ�
  unsigned short y_thres1;		// 10 bit ����Чֵ 0 ~ 1023
  unsigned short u_thres1;		// 10 bit ����Чֵ 0 ~ 1023
  unsigned short v_thres1;		// 10 bit ����Чֵ 0 ~ 1023

  // �˲���2��������ֵ���� (3D)
  unsigned short y_thres2;		// 10 bit ��
  unsigned short u_thres2;		// 10 bit ��
  unsigned short v_thres2;		// 10 bit ��

  // ǿ��(�ѷ���)
  unsigned short y_strength0;//10bit
  unsigned short y_strength1;//10bit
  unsigned short y_strength2;//10bit

  unsigned short u_strength0;//10bit
  unsigned short u_strength1;//10bit
  unsigned short u_strength2;//10bit

  unsigned short v_strength0;//10bit
  unsigned short v_strength1;//10bit
  unsigned short v_strength2;//10bit
  
  
  // 2D���ұ�
  unsigned char noise0[17];  	// 8bit, 0 ~ 255
  										//		2d space noise0_lutdata (����Y/CbCr��ǿ��ֵ��0~255��16�ȷֲ�ֵ���ұ�)	
  										//			���Ը������ȷ���Y(����CbCr����)��ǿ��ֵ��Ʋ�ͬ���˲�ǿ��.
  										//			���µ�ǿ�ȱ�
  										//			����(����ǿ��)  �˲�ǿ��ϵ��   ˵��
  										//			0   (0  ),        255          (��С�������ߵ͹⴦��snr�������С, ���������˲�ǿ��)
  										//			1   (16 ),        255
  										//       ...
  										//			16  (255),        223          (�����������������snr��������, ������С���˲�ǿ��) 
  										// 	ֵԽ��, �˲�ǿ��Խ��
  
  
  // 3D���ұ�
  unsigned char noise1[17];  	// 8bit, 0 ~ 255
  										//		3d temporty lutdata
} isp_denoise_t; 

typedef struct isp_denoise_  *isp_denoise_ptr_t;

void isp_denoise_init (isp_denoise_ptr_t p_denoise);

void isp_denoise_init_io (isp_denoise_ptr_t p_denoise);

void isp_denoise_run (isp_denoise_ptr_t p_denoise, isp_ae_ptr_t p_ae);

#endif	// #ifndef __GEM_ISP_DENOISE_HEADFILE__
