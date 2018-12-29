//****************************************************************************
//
//	Copyright (C) 2012~2016 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_isp_scalar.h
//	  ARKN141 ISP scalar device driver
//
//	Revision history
//
//		2016.04.02	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_ISP_SCALAR_H_
#define _XM_ISP_SCALAR_H_

#include "xm_type.h"


#if defined (__cplusplus)
	extern "C"{
#endif

typedef void (*xm_mid_line_callback) (void *xm_mid_line_user_data);


// 设置最大支持的ISP scalar分辨率
// 1) 当ISP scalar仅用于LCD显示输出时, 可将ISP scalar的分辨率设置为显示屏的视频OSD尺寸
// 2) 当ISP scalar用于LCD显示输出及其他输出时, 将ISP scalar的分辨率设置为最大尺寸
#define	MAX_SCALAR_WIDTH		480
#define	MAX_SCALAR_HEIGHT		272

#define	XM_ISP_SCALAR_ERRCODE_OK						(0)
#define	XM_ISP_SCALAR_ERRCODE_ILLEGAL_PARA			(-1)		// 无效的参数
#define	XM_ISP_SCALAR_ERRCODE_DEV_NO_INIT			(-2)		// 设备未初始化
#define	XM_ISP_SCALAR_ERRCODE_OBJ_RE_CREATE			(-3)		// scalar为互斥对象,有且仅有一个对象可以创建
#define	XM_ISP_SCALAR_ERRCODE_OBJ_INVALID			(-4)		// 无效对象

enum {
	XM_ISP_SCALAR_FORMAT_YUV420 = 0,		// YUV420 planar (Y\U\V数据是分开存放)
	XM_ISP_SCALAR_FORMAT_Y_UV420,			// NV12, YUV420 Semi-Planar (U、V是交叉存放)
	XM_ISP_SCALAR_FORMAT_YUV422,			// YUV422 Planar (Y\U\V数据是分开存放)
	XM_ISP_SCALAR_FORMAT_Y_UV422			// YUV422 Semi-Planar (U、V是交叉存放)
};

// 源图像通道定义
enum {
	XM_ISP_SCALAR_SRC_CHANNEL_ISP = 0,		// ISP输出通道
	XM_ISP_SCALAR_SRC_CHANNEL_ITU601			// ITU601输出通道
};

// 视频图像的行同步(hsync)/列同步(vsync)信号极性定义
enum {
	XM_ISP_SCALAR_SRC_SYNC_PLOARITY_HIGH_LEVEL = 0,	// 高电平有效(ISP VIDEO输出缺省为高电平)
	XM_ISP_SCALAR_SRC_SYNC_PLOARITY_LOW_LEVEL = 1,	// 低电平有效
	
};

enum {
	XM_ISP_SCALAR_SRC_YCBCR_UYVY = 0,
	XM_ISP_SCALAR_SRC_YCBCR_YUYV = 1
};

#define	XM_ISP_SCALAR_CALLBACK_COMMAND_READY		0x00000001		// ISP-Scalar帧已正确接收完毕, 等待调用者处理
#define	XM_ISP_SCALAR_CALLBACK_COMMAND_RECYCLE		0x00000002		// ISP-Scalar帧无效, 调用者需要将该帧回收
#define	XM_ISP_SCALAR_CALLBACK_COMMAND_RESET		0x00000003		// ISP-Scalar异常, 调用者需要复位isp-scalar对象并重新执行



// ISP Scalar 配置参数表
// 定义了如何将源视频图像的定义区域(源窗口, 包含指窗口位置/窗口大小 )或整个图像复制到目标视频图像的定义区域(目标窗口, 包含窗口位置/窗口大小)
typedef struct _xm_isp_scalar_configuration_parameters {
	u16_t		src_channel;			// 源图像的输入通道选择 (内部isp输出 或者 外部itu601输入)
											//		内部isp输出一般为Y_UV420(NV12)格式
											//		外部itu601输入一般为YUV422, YUV420等格式
	u8_t		src_hsync_polarity;	// 源图像的行同步信号极性选择
	u8_t		src_vsync_polarity;	// 源图像的列同步信号极性选择
	

	// 源图像的定义(cmos sensor或者601 in)
	u16_t 	src_format;				// YUV422/Y_UV422/YUV420/Y_UV420
	u16_t		src_ycbcr_sequence;	// y/cbcr的顺序, 0:uyvy 1:yuyv
	u16_t		src_width;				// 源图像的宽度
	u16_t		src_height;				// 源图像的高度
	u16_t		src_stride;				// 源图像的行宽度
	// 源窗口定义(自左向右, 自上而下)
	u16_t		src_window_x;			// 源窗口相对于源图像原点的X方向偏移
	u16_t		src_window_y;			// 源窗口相对于源图像原点的Y方向偏移
	u16_t		src_window_width;		// 源窗口的宽度
	u16_t		src_window_height;	// 源窗口的高度
	
	// 目标图像定义
	u16_t		dst_format;				// 仅支持Y_UV420(NV12)
	u16_t		dst_width;				// 目标图像的宽度
	u16_t		dst_height;				// 目标图像的高度
	u16_t		dst_stride;				// 目标图像的行宽度, 必须为16的倍数
	// 目标窗口定义(自左向右, 自上而下)
	u16_t		dst_window_x;			// 目标窗口相对于目标图像原点的X方向偏移
	u16_t		dst_window_y;			// 目标窗口相对于目标图像原点的Y方向偏移
	u16_t		dst_window_width;		// 目标窗口的宽度
	u16_t		dst_window_height;	// 目标窗口的高度
	
	// 部分处理完成行中断
	u16_t 	mid_line; 								// mid_line计数值(以行为单位)
	void *	mid_line_user_data;					// 用户私有数据
	xm_mid_line_callback mid_line_user_callback;	// 用户回调函数

} xm_isp_scalar_configuration_parameters;

void xm_isp_scalar_init (void);

void xm_isp_scalar_exit (void);


int xm_isp_scalar_run (xm_isp_scalar_configuration_parameters  *scalar_parameters );

// 配置ISP scalar的源/目标窗口
int xm_isp_scalar_window_config (u16_t src_x, u16_t src_y,
											u16_t src_w, u16_t src_h,
											u16_t dst_x, u16_t dst_y,
											u16_t dst_w, u16_t dst_h
											);


int xm_isp_scalar_register_user_buffer (	int  scalar_object,
													 	u32_t y_addr, 
														u32_t uv_addr,
														void	(*data_ready_user_callback) (void *user_private_data),
														void	(*frame_free_user_callback) (void *user_private_data),
														void *user_private_data
													);

// 创建一个指定输出尺寸的ISP的scalar对象, 
// 返回值 
// <  0  创建失败
// >= 0  对象标识符
int xm_isp_scalar_create (unsigned int video_w, unsigned int video_h);

// 删除ISP scalar对象
int xm_isp_scalar_delete (int scalar_object);

int xm_isp_scalar_create_ex (
									  unsigned int src_channel,			// 源图像的输入通道选择 (内部isp输出 或者 外部itu601输入)
									  unsigned int	src_hsync_polarity,	// 源图像的行同步信号极性选择
									  unsigned int	src_vsync_polarity,	// 源图像的列同步信号极性选择
									  unsigned int	src_format,				// YUV422/Y_UV422/YUV420/Y_UV420
									  unsigned int src_ycbcr_sequence,	// y/cbcr的顺序, 0:uyvy 1:yuyv
									  
									  unsigned int	src_width,				// 源图像的宽度
									  unsigned int	src_height,				// 源图像的高度
									  unsigned int	src_stride,				// 源图像的行宽度
									  
									  // 源窗口定义(自左向右, 自上而下)
									  unsigned int	src_window_x,			// 源窗口相对于源图像原点的X方向偏移
									  unsigned int	src_window_y,			// 源窗口相对于源图像原点的Y方向偏移
									  unsigned int	src_window_width,		// 源窗口的宽度
									  unsigned int	src_window_height,	// 源窗口的高度
									  
									  // 输出图像定义
									  unsigned int dst_width,				// 输出图像的宽度
									  unsigned int dst_height,				// 输出图像的高度
									  unsigned int dst_stride,				// 输出图像的行宽度
									  
									  unsigned int mid_line_counter,
									  void (*mid_line_callback)(void *),
									  void * mid_line_user_data									  
									  );


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_ISP_SCALAR_H_
