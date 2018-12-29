// =============================================================================
// File        : Gem_isp_ae.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#ifndef _GEM_ISP_AE_HEADFILE_
#define _GEM_ISP_AE_HEADFILE_

#define GEM_AE0_BASE (0x070)


#define GEM_AE1_BASE (0x1b0 + 0x0c + 0x0c)	// ����3���Ĵ���
#define GEM_AE2_BASE (0x1c4 + 0x0c + 0x0c)


typedef struct isp_auto_exposure_ 
{
  unsigned char histoBand[4];
  unsigned char winWeight[3][3];
  unsigned short histoGram[5];
  
  unsigned short yavg_s[3][3]; 
  unsigned short histogram_s[3][3][5];
  
  unsigned char lumTarget;
  unsigned char lumCurr;
  
  unsigned int eris_hist_thresh;				// ERIS����ֱ��ͼ��4����ֵ(ÿ����ֵ��ȡֵ��ΧΪ0~255)
  														//	bit0~7��Ӧ01����, bit8~15��Ӧ12����, bit16~23��Ӧ23����, bit24~31��Ӧ34����			
  
  unsigned int  eris_hist_statics[5];		// (ֻ����Ϣ) 5��ERIS����ֱ��ͼ��ͳ�Ƶ����ظ���,
  unsigned int  eris_yavg;						// (ֻ����Ϣ) ERIS����ͳ��ֵ, ����������ص���, �õ�ƽ������ֵ
  

} isp_ae_t; 

typedef struct isp_auto_exposure_ *isp_ae_ptr_t;

void isp_ae_init (isp_ae_ptr_t p_ae); 

void isp_ae_init_io (isp_ae_ptr_t ae); 

void isp_ae_info_read (isp_ae_ptr_t p_ae);

void isp_ae_sts2_read (isp_ae_ptr_t p_ae);




void isp_ae_run (isp_ae_ptr_t p_ae);

#endif
