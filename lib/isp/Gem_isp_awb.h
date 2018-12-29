// =============================================================================
// File        : Gem_isp_awb.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#ifndef __GEM_ISP_AWB_HEADFILE__
#define __GEM_ISP_AWB_HEADFILE__

#define GEM_AWB0_BASE (0x090) 
#define GEM_AWB1_BASE (0x1bc + 0x0c + 0x0c)
#define GEM_AWB2_BASE (0x20c + 0x0c + 0x0c)



typedef struct isp_auto_awb_ 
{
  unsigned char enable;    // 1 bit
  
  unsigned char mode;     // 2 bit
  
  unsigned char manual;      	// 1 bit, 1 �ֶ�ģʽ, 0 �Զ�ģʽ
  
  unsigned char weight[3][3]; // 4 bit
  unsigned char black;     // 8 bit
  unsigned char white;     // 8 bit
  unsigned char jitter;
  unsigned short r2g_min;
  unsigned short r2g_max;
  unsigned short b2g_min;     
  unsigned short b2g_max;
  
  unsigned short r2g_light[8];	// Rɫ�±�, �����ֵ 4095
  											//		���Զ�ģʽ��ʹ��
  unsigned short b2g_light[8];	// Bɫ�±�, �����ֵ 4095
  											//		���Զ�ģʽ��ʹ��
  
  unsigned char use_light[8];	// ��ʾ��Ӧ�Ĺ�Դ�Ƿ�ʹ��, 1 ʹ�� 0 δʹ��
  											//		���Զ�ģʽ��ʹ��
  
  unsigned short gain_g2r;    // 16 bit, ��Ч��Χ 0 ~ 4095
  										//		�ֶ�ģʽ�����ú�ɫ������	
  										//		ֵԽ��, ��ɫ��������ɫԽŨ
  unsigned short gain_g2b; 	// 16 bit, ��Ч��Χ 0 ~ 4095
  										//		�ֶ�ģʽ��������ɫ������
  										//		ֵԽ��, ��ɫ��������ɫԽŨ
  
  
  unsigned short gain_r2g;		// 16 bit, ֻ����, ����R����ɫ�¹���ֵ
  unsigned short gain_b2g;		// 16 bit, ֻ����, ����B����ɫ�¹���ֵ
  
  unsigned int gray_num;
} isp_awb_t;  

typedef struct isp_auto_awb_ *isp_awb_ptr_t;

void isp_awb_init (isp_awb_ptr_t p_awb);

void isp_awb_init_io (isp_awb_ptr_t p_awb);

void isp_awb_info_read (isp_awb_ptr_t p_awb);

void isp_awb_run (isp_awb_ptr_t p_awb);

#endif