// =============================================================================
// File        : arkn141_isp.h
// Author      : ZhuoYongHong
// Date        : 2015.12.1
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------

#ifndef  _ARKN141_ISP_H_
#define  _ARKN141_ISP_H_

#include "xm_type.h"

#if defined (__cplusplus)
	extern "C"{
#endif

// ���֧�� 2048 * 1280
#if 0
#define IMAGE_H_SZ_MAX  2048	//1920//2048//1280
#define IMAGE_V_SZ_MAX  1280 
#else

#define IMAGE_H_SZ_MAX   	1920	
#define IMAGE_V_SZ_MAX	1080
//#define IMAGE_H_SZ_MAX   	1280	
//#define IMAGE_V_SZ_MAX	 	720

#endif

#define IMAGE_H_SZ		isp_get_video_width()
#define IMAGE_V_SZ		isp_get_video_height()



// 0: RGGB 1: GRBG 2: BGGR 3: GBRG
enum {
	ARKN141_ISP_RAW_IMAGE_BAYER_MODE_RGGB = 0,		// 
	ARKN141_ISP_RAW_IMAGE_BAYER_MODE_GRBG,
	ARKN141_ISP_RAW_IMAGE_BAYER_MODE_BGGR,
	ARKN141_ISP_RAW_IMAGE_BAYER_MODE_GBRG
};

// YUV���ݴ洢��ʽ
// 0��y_uv420 1:y_uv422 2:yuv420 3:yuv422 
enum {
	ARKN141_ISP_YUV_FORMAT_Y_UV420 = 0,
	ARKN141_ISP_YUV_FORMAT_Y_UV422,
	ARKN141_ISP_YUV_FORMAT_YUV420,
	ARKN141_ISP_YUV_FORMAT_YUV422
};


// 0: 8λ 1: 10λ 2: 12λ 3: 14λ
enum {
	ARKN141_ISP_SENSOR_BIT_8 = 0,
	ARKN141_ISP_SENSOR_BIT_10,
	ARKN141_ISP_SENSOR_BIT_12,
	ARKN141_ISP_SENSOR_BIT_14
};
	
unsigned int isp_get_sensor_bit (void);
void isp_set_sensor_bit (unsigned int sensor_bit);

void XMSYS_H264CodecSetFrameRate (unsigned char fps);

// ��ȡISP��Ƶ������ߴ綨��(1080P����720P)
unsigned int isp_get_video_width  (void);
unsigned int isp_get_video_height (void);
unsigned int isp_get_video_image_size (void);

// ����ISP����Ƶ�����ʽ
unsigned int isp_get_video_format (void);
void isp_set_video_format (unsigned int video_format);

// ����ISP��Ƶ������ߴ� (1080P����720P)
void isp_set_video_width  (unsigned int width);
void isp_set_video_height (unsigned int height);


// ����/��ȡISP RAWͼ���BAYER ģʽ
unsigned int isp_get_raw_image_bayer_mode (void);
void isp_set_raw_image_bayer_mode (unsigned int bayer_mode);

#define	_ISP_BAYER_MODE_		isp_get_raw_image_bayer_mode()




unsigned int *isp_get_yuv_buffer (unsigned int frame_id);

void isp_sys_set_frame_ready (unsigned int frame_id);


// auto-run��Ŀ����
enum {
	ISP_AUTO_RUN_AE = 0,
	ISP_AUTO_RUN_AWB,
	ISP_AUTO_RUN_COLOR,
	ISP_AUTO_RUN_ERIS,
	ISP_AUTO_RUN_DENOISE,
	ISP_AUTO_RUN_ENHANCE,
	ISP_AUTO_RUN_COUNT
};

// ����ISP auto-run��Ŀ��״̬
// state --> 1  ʹ��auto run
// state --> 0  ��ֹauto run
void isp_set_auto_run_state (unsigned int item, unsigned int state);
unsigned int isp_get_auto_run_state  (unsigned int item);


// ��ȡISP���ò������İ汾
unsigned int isp_get_isp_data_version (void);

// ��ȡISP���ò��������ֽڴ�С
unsigned short isp_get_isp_data_size (void);
// ��ȡEXP���Ʋ��������ֽڴ�С
unsigned short isp_get_exp_data_size (void);

// ��ȡISP���Ʋ�����������
int isp_get_isp_data (char *isp_data, int isp_size);
// ��ȡEXP���Ʋ�����������
int isp_get_exp_data (char *exp_data, int exp_size);

void xm_isp_init (void);

void xm_isp_exit (void);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _ARKN141_ISP_H_
