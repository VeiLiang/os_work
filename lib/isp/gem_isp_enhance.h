// =============================================================================
// File        : Gem_isp_enhance.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#ifndef _GEM_ISP_ENHANCE_H_
#define _GEM_ISP_ENHANCE_H_

#define GEM_ENHANCE_BASE  (0x190)

struct sharp_t
{
	unsigned char enable;	// 1 bit, �񻯹���ʹ��
	
	unsigned char mode;		// 1 bit, ��ģʽ�� 0: ǿ�� 1: ���
									//		"ǿ��"��"���"��Ч�������С
	
	unsigned char coring;	// 3 bit, ��Чֵ 0 ~ 7, ������ѡ��
									// 	ֵԽ��, ͼ��Խģ��, ͼ���񻯳̶Ƚ���.
	
	unsigned char strength;	// 8 bit, ��Чֵ 0 ~ 255
									//		ֵԽ��, ͼ��Խģ��, ͼ���񻯳̶Ƚ���.
	
	unsigned short gainmax;	// 10 bit, ��Чֵ 0 ~ 255, ����255����255��Ч��
									//		0��Ч������255��Ч��
									//		��1��255, ֵԽ��, ��Ч��Խǿ
};

struct bcst_t
{
	unsigned short enable;		// 1 bit
	
	short bright;					// 10 bit, ��Чֵ -255 ~ 255 
										//		���ӻ��Сÿ�����ص������ֵ
	
	unsigned short contrast;	// 11 bit, ��Чֵ 0 ~ 2047
										//		ͨ��������̬��Χ���ı�Աȶ�,
										//		ֵԽ��, ֱ��ͼԽ�����ƶ�, ���ĵ͹⴦ϸ���𽥶�ʧ, �ұߵĸ߹⴦�����ڱ���.
										//		ֵԽС, ֱ��ͼԽ�����ƶ�, �Ҳ�ĸ߹⴦ϸ���𽥶�ʧ, ���ĵ͹⴦�����ڱ���.
	
	unsigned short satuation;	// 11 bit, ��Чֵ 0 ~ 2047 
										//		ɫ�ʱ��Ͷȵ���
										//		ֵԽ��, ɫ��Խ����. ֵԽС, Խ�����ڻ�ɫ.
										//		����0ʱ, Ϊ�ڰ׻Ҷ�Ч��
	
	short hue;						// 8 bit,  ��Чֵ -128 ~ 127
										//		ɫ��ǵ���
	
	unsigned short offset0;		// 10 bit, ��Чֵ 0 ~ 255.  
										//		�Աȶ�Ϊ1024ʱ,������ֵ����Ч
	
	unsigned short offset1;		// 10 bit, ��Чֵ 0 ~ 255.   
										//		saturation offset (0,255) (default 128)
										//		hueΪ0ʱ, ������ֵ����Ч
};

typedef struct isp_enhance_
{
  struct sharp_t sharp;
  struct bcst_t bcst;
} isp_enhance_t;

typedef struct isp_enhance_ *isp_enhance_ptr_t;

void isp_enhance_init (isp_enhance_ptr_t p_enhance);

void isp_enhance_init_io (isp_enhance_ptr_t p_enhance);





#endif





