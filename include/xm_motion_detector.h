//****************************************************************************
//
//	Copyright (C) 2014-2015 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_motion_detector.h
//	  �˶����
//
//	Revision history
//
//		2014.08.01	ZhuoYongHong add motion detector
//
//****************************************************************************
#ifndef _XM_MOTION_DETECTOR_H_
#define _XM_MOTION_DETECTOR_H_

#if defined (__cplusplus)
	extern "C"{
#endif

enum 
{
	XM_MOTION_DETECTOR_QUIET = 0,		// ��ֹ
	XM_MOTION_DETECTOR_MOVING			// �˶�
};


// ��ʼ�����˳�
void xm_motion_detector_init(void);
void xm_motion_detector_exit(void);

// ���������˶���������ͼ��ߴ磬
// �������ͼ��ߴ�Ϊ320*240
// ����ֵ
//		-1		ʧ��
//		0		�ɹ�
int xm_motion_detector_config(unsigned int image_width, unsigned int image_height,unsigned int image_stride);


// ����/��ȡ�����ж������Ƿ��˶�����ֵ
// ����ֵ 8 ~ 1023��ȡֵ��ΧΪ0 ~ 65535
int xm_motion_detector_set_pixel_threshold(unsigned int threshold);
int xm_motion_detector_get_pixel_threshold(void);

// ����/��ȡ�����ж������Ƿ�Ϊ�˶�״̬���˶�����������ֵ
// ���ж�Ϊ�˶�״̬�����ص����������ڸ���ֵʱ�������ж�Ϊ�˶�״̬��
// ����ֵ 100
int xm_motion_detector_set_motion_threshold(unsigned int motion_threshold);
int xm_motion_detector_get_motion_threshold(void);

// ��һ֡��ͼ����뵽�ƶ������н��м��
// ����ֵ
//   (-1)	δִ�г�ʼ���� �޷����м��
//   XM_MOTION_DETECTOR_QUIET     ��ǰΪ��ֹ״̬
//   XM_MOTION_DETECTOR_MOVING    ��ǰΪ�˶�״̬
int xm_motion_detector_monitor_image(unsigned char *image_y);// ����ͼ���Y����
												 

unsigned int xm_motion_detector_match_inttime_gain (unsigned int inttime_gain);

// ��ȡ��ǰ�˶�������Ϣ
//		binary_image_buffer����Ϊ�գ���ʾ��ץȡ��ֵͼ
// ����ֵ
//  0   �ɹ�
// -1   ʧ��
int xm_motion_detector_get_binary_image (	unsigned char *binary_image_buffer,	// �����ֵͼ�Ļ�����
											unsigned int   binary_image_length,	// �����ֵͼ�Ļ��������ֽڳ���
											unsigned int *pixel_threshold,		// �����ж�Ϊ�˶�״̬����ֵ
											unsigned int *motion_count,			// �ж�Ϊ�˶�״̬�����ص�����,
											unsigned int *image_threshold,		// �����ж�Ϊ�˶�״̬����ֵ
											unsigned int *motion_detect_result	// �˶����Ľ��, ��ֹ���˶�
										);

// ��Ҫ����
//	1) ��ʼ��	 xm_motion_detector_init
//	2) ��������Դͼ��ߴ� xm_motion_detector_config ��320�� 240�� 320��
// 3) �����˶��ж���ֵ xm_motion_detector_set_pixel_threshold (72)
// 4) ѭ���� ����xm_motion_detector_monitor_image �ж�ÿ������ͼ���Ƿ�Ϊ�˶�����
// 5) ������ xm_motion_detector_exit

// 

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_MOTION_DETECTOR_H_													