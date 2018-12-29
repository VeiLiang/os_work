#ifndef _XM_ARKN141_ISP_AE_H_
#define _XM_ARKN141_ISP_AE_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

// arkn141_isp_set_exposure_compensation
// 	ISP�����عⲹ��
// ev	
//		�عⲹ��ֵ, -3 ~ +3
//
//	����ֵ
//		0	�ɹ�
//		-1	ʧ��
int xm_arkn141_isp_set_exposure_compensation (int ev);

// xm_arkn141_isp_set_sharpening_value
// 	�����񻯳̶�
//	sharpening_value
// 	0 ��΢
// 	1 ���
//		2 ǿ��
// 
//	����ֵ
//		0	�ɹ�
//		-1	ʧ��
int xm_arkn141_isp_set_sharpening_value (int sharpening_value);

// ���ù�ԴƵ��
// flicker_freq	
//			0			��ֹ��Դ
//			50			50hz��Դ
//			60			60hz��Դ
//			����ֵ	��Ч
//	����ֵ
//		0	�ɹ�
//		-1	ʧ��
int xm_arkn141_isp_set_flicker_freq  (int flicker_freq);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif	// _ARKN141_ISP_EXPOSURE_CMOS_H_