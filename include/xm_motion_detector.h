//****************************************************************************
//
//	Copyright (C) 2014-2015 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_motion_detector.h
//	  运动检测
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
	XM_MOTION_DETECTOR_QUIET = 0,		// 静止
	XM_MOTION_DETECTOR_MOVING			// 运动
};


// 初始化及退出
void xm_motion_detector_init(void);
void xm_motion_detector_exit(void);

// 配置用于运动检测的输入图像尺寸，
// 最大输入图像尺寸为320*240
// 返回值
//		-1		失败
//		0		成功
int xm_motion_detector_config(unsigned int image_width, unsigned int image_height,unsigned int image_stride);


// 设置/获取用于判定像素是否运动的阈值
// 典型值 8 ~ 1023，取值范围为0 ~ 65535
int xm_motion_detector_set_pixel_threshold(unsigned int threshold);
int xm_motion_detector_get_pixel_threshold(void);

// 设置/获取用于判定画面是否为运动状态的运动像素数量阈值
// 即判定为运动状态的像素的总数量大于该阈值时，画面判定为运动状态。
// 典型值 100
int xm_motion_detector_set_motion_threshold(unsigned int motion_threshold);
int xm_motion_detector_get_motion_threshold(void);

// 将一帧新图像加入到移动侦测队列进行监控
// 返回值
//   (-1)	未执行初始化， 无法进行监控
//   XM_MOTION_DETECTOR_QUIET     当前为静止状态
//   XM_MOTION_DETECTOR_MOVING    当前为运动状态
int xm_motion_detector_monitor_image(unsigned char *image_y);// 输入图像的Y分量
												 

unsigned int xm_motion_detector_match_inttime_gain (unsigned int inttime_gain);

// 获取当前运动检测的信息
//		binary_image_buffer可以为空，表示不抓取二值图
// 返回值
//  0   成功
// -1   失败
int xm_motion_detector_get_binary_image (	unsigned char *binary_image_buffer,	// 保存二值图的缓冲区
											unsigned int   binary_image_length,	// 保存二值图的缓冲区的字节长度
											unsigned int *pixel_threshold,		// 像素判定为运动状态的阈值
											unsigned int *motion_count,			// 判定为运动状态的像素点总数,
											unsigned int *image_threshold,		// 画面判定为运动状态的阈值
											unsigned int *motion_detect_result	// 运动检测的结果, 静止或运动
										);

// 主要流程
//	1) 初始化	 xm_motion_detector_init
//	2) 设置输入源图像尺寸 xm_motion_detector_config （320， 240， 320）
// 3) 设置运动判定阈值 xm_motion_detector_set_pixel_threshold (72)
// 4) 循环， 调用xm_motion_detector_monitor_image 判定每个输入图像是否为运动画面
// 5) 结束， xm_motion_detector_exit

// 

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_MOTION_DETECTOR_H_													