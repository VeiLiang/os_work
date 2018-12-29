//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_video_task.h
//	  视频任务
//
//	Revision history
//
//		2012.09.06	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_VIDEO_TASK_H_
#define _XM_VIDEO_TASK_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// 视频画面拼接模式
enum {
	XMSYS_ASSEMBLY_MODE_FRONT_ONLY = 0,			// 前置全屏 (前置摄像头全屏)
	XMSYS_ASSEMBLY_MODE_REAL_ONLY,				// 后置全屏 (前置摄像头全屏)
	XMSYS_ASSEMBLY_MODE_FRONT_REAL,				// 前主后辅, 画中画(前置摄像头为主, 后置摄像头为辅)
	XMSYS_ASSEMBLY_MODE_REAL_FRONT,				// 前辅后主, 画中画(前置摄像头为辅, 后置摄像头为主)
	XMSYS_ASSEMBLY_MODE_ISP601_ONLY,
	XMSYS_ASSEMBLY_MODE_ITU656_ONLY,
	XMSYS_ASSEMBLY_MODE_USB_ONLY,
	XMSYS_ASSEMBLY_MODE_ISP601_ITU656,
	XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_Left,
	XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_Right,
	XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_UP,
	XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_Vert,
	XMSYS_ASSEMBLY_MODE_COUNT
};

// 视频系统任务初始化
int XMSYS_VideoInit (void);

// 视频系统任务退出
int XMSYS_VideoExit (void);

// 设置视频画面拼接模式
// 返回值
//	0		成功
// -1		失败
int XMSYS_VideoSetImageAssemblyMode (unsigned int image_assembly_mode);

// 返回当前视频图像拼接模式
unsigned int XMSYS_VideoGetImageAssemblyMode (void);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_VIDEO_TASK_H_