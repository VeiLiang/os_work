#ifndef _XM_ARKN141_ISP_AE_H_
#define _XM_ARKN141_ISP_AE_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

// arkn141_isp_set_exposure_compensation
// 	ISP设置曝光补偿
// ev	
//		曝光补偿值, -3 ~ +3
//
//	返回值
//		0	成功
//		-1	失败
int xm_arkn141_isp_set_exposure_compensation (int ev);

// xm_arkn141_isp_set_sharpening_value
// 	设置锐化程度
//	sharpening_value
// 	0 轻微
// 	1 柔和
//		2 强烈
// 
//	返回值
//		0	成功
//		-1	失败
int xm_arkn141_isp_set_sharpening_value (int sharpening_value);

// 设置光源频率
// flicker_freq	
//			0			禁止光源
//			50			50hz光源
//			60			60hz光源
//			其他值	无效
//	返回值
//		0	成功
//		-1	失败
int xm_arkn141_isp_set_flicker_freq  (int flicker_freq);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif	// _ARKN141_ISP_EXPOSURE_CMOS_H_