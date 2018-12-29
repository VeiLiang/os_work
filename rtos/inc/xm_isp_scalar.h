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


// �������֧�ֵ�ISP scalar�ֱ���
// 1) ��ISP scalar������LCD��ʾ���ʱ, �ɽ�ISP scalar�ķֱ�������Ϊ��ʾ������ƵOSD�ߴ�
// 2) ��ISP scalar����LCD��ʾ������������ʱ, ��ISP scalar�ķֱ�������Ϊ���ߴ�
#define	MAX_SCALAR_WIDTH		480
#define	MAX_SCALAR_HEIGHT		272

#define	XM_ISP_SCALAR_ERRCODE_OK						(0)
#define	XM_ISP_SCALAR_ERRCODE_ILLEGAL_PARA			(-1)		// ��Ч�Ĳ���
#define	XM_ISP_SCALAR_ERRCODE_DEV_NO_INIT			(-2)		// �豸δ��ʼ��
#define	XM_ISP_SCALAR_ERRCODE_OBJ_RE_CREATE			(-3)		// scalarΪ�������,���ҽ���һ��������Դ���
#define	XM_ISP_SCALAR_ERRCODE_OBJ_INVALID			(-4)		// ��Ч����

enum {
	XM_ISP_SCALAR_FORMAT_YUV420 = 0,		// YUV420 planar (Y\U\V�����Ƿֿ����)
	XM_ISP_SCALAR_FORMAT_Y_UV420,			// NV12, YUV420 Semi-Planar (U��V�ǽ�����)
	XM_ISP_SCALAR_FORMAT_YUV422,			// YUV422 Planar (Y\U\V�����Ƿֿ����)
	XM_ISP_SCALAR_FORMAT_Y_UV422			// YUV422 Semi-Planar (U��V�ǽ�����)
};

// Դͼ��ͨ������
enum {
	XM_ISP_SCALAR_SRC_CHANNEL_ISP = 0,		// ISP���ͨ��
	XM_ISP_SCALAR_SRC_CHANNEL_ITU601			// ITU601���ͨ��
};

// ��Ƶͼ�����ͬ��(hsync)/��ͬ��(vsync)�źż��Զ���
enum {
	XM_ISP_SCALAR_SRC_SYNC_PLOARITY_HIGH_LEVEL = 0,	// �ߵ�ƽ��Ч(ISP VIDEO���ȱʡΪ�ߵ�ƽ)
	XM_ISP_SCALAR_SRC_SYNC_PLOARITY_LOW_LEVEL = 1,	// �͵�ƽ��Ч
	
};

enum {
	XM_ISP_SCALAR_SRC_YCBCR_UYVY = 0,
	XM_ISP_SCALAR_SRC_YCBCR_YUYV = 1
};

#define	XM_ISP_SCALAR_CALLBACK_COMMAND_READY		0x00000001		// ISP-Scalar֡����ȷ�������, �ȴ������ߴ���
#define	XM_ISP_SCALAR_CALLBACK_COMMAND_RECYCLE		0x00000002		// ISP-Scalar֡��Ч, ��������Ҫ����֡����
#define	XM_ISP_SCALAR_CALLBACK_COMMAND_RESET		0x00000003		// ISP-Scalar�쳣, ��������Ҫ��λisp-scalar��������ִ��



// ISP Scalar ���ò�����
// ��������ν�Դ��Ƶͼ��Ķ�������(Դ����, ����ָ����λ��/���ڴ�С )������ͼ���Ƶ�Ŀ����Ƶͼ��Ķ�������(Ŀ�괰��, ��������λ��/���ڴ�С)
typedef struct _xm_isp_scalar_configuration_parameters {
	u16_t		src_channel;			// Դͼ�������ͨ��ѡ�� (�ڲ�isp��� ���� �ⲿitu601����)
											//		�ڲ�isp���һ��ΪY_UV420(NV12)��ʽ
											//		�ⲿitu601����һ��ΪYUV422, YUV420�ȸ�ʽ
	u8_t		src_hsync_polarity;	// Դͼ�����ͬ���źż���ѡ��
	u8_t		src_vsync_polarity;	// Դͼ�����ͬ���źż���ѡ��
	

	// Դͼ��Ķ���(cmos sensor����601 in)
	u16_t 	src_format;				// YUV422/Y_UV422/YUV420/Y_UV420
	u16_t		src_ycbcr_sequence;	// y/cbcr��˳��, 0:uyvy 1:yuyv
	u16_t		src_width;				// Դͼ��Ŀ��
	u16_t		src_height;				// Դͼ��ĸ߶�
	u16_t		src_stride;				// Դͼ����п��
	// Դ���ڶ���(��������, ���϶���)
	u16_t		src_window_x;			// Դ���������Դͼ��ԭ���X����ƫ��
	u16_t		src_window_y;			// Դ���������Դͼ��ԭ���Y����ƫ��
	u16_t		src_window_width;		// Դ���ڵĿ��
	u16_t		src_window_height;	// Դ���ڵĸ߶�
	
	// Ŀ��ͼ����
	u16_t		dst_format;				// ��֧��Y_UV420(NV12)
	u16_t		dst_width;				// Ŀ��ͼ��Ŀ��
	u16_t		dst_height;				// Ŀ��ͼ��ĸ߶�
	u16_t		dst_stride;				// Ŀ��ͼ����п��, ����Ϊ16�ı���
	// Ŀ�괰�ڶ���(��������, ���϶���)
	u16_t		dst_window_x;			// Ŀ�괰�������Ŀ��ͼ��ԭ���X����ƫ��
	u16_t		dst_window_y;			// Ŀ�괰�������Ŀ��ͼ��ԭ���Y����ƫ��
	u16_t		dst_window_width;		// Ŀ�괰�ڵĿ��
	u16_t		dst_window_height;	// Ŀ�괰�ڵĸ߶�
	
	// ���ִ���������ж�
	u16_t 	mid_line; 								// mid_line����ֵ(����Ϊ��λ)
	void *	mid_line_user_data;					// �û�˽������
	xm_mid_line_callback mid_line_user_callback;	// �û��ص�����

} xm_isp_scalar_configuration_parameters;

void xm_isp_scalar_init (void);

void xm_isp_scalar_exit (void);


int xm_isp_scalar_run (xm_isp_scalar_configuration_parameters  *scalar_parameters );

// ����ISP scalar��Դ/Ŀ�괰��
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

// ����һ��ָ������ߴ��ISP��scalar����, 
// ����ֵ 
// <  0  ����ʧ��
// >= 0  �����ʶ��
int xm_isp_scalar_create (unsigned int video_w, unsigned int video_h);

// ɾ��ISP scalar����
int xm_isp_scalar_delete (int scalar_object);

int xm_isp_scalar_create_ex (
									  unsigned int src_channel,			// Դͼ�������ͨ��ѡ�� (�ڲ�isp��� ���� �ⲿitu601����)
									  unsigned int	src_hsync_polarity,	// Դͼ�����ͬ���źż���ѡ��
									  unsigned int	src_vsync_polarity,	// Դͼ�����ͬ���źż���ѡ��
									  unsigned int	src_format,				// YUV422/Y_UV422/YUV420/Y_UV420
									  unsigned int src_ycbcr_sequence,	// y/cbcr��˳��, 0:uyvy 1:yuyv
									  
									  unsigned int	src_width,				// Դͼ��Ŀ��
									  unsigned int	src_height,				// Դͼ��ĸ߶�
									  unsigned int	src_stride,				// Դͼ����п��
									  
									  // Դ���ڶ���(��������, ���϶���)
									  unsigned int	src_window_x,			// Դ���������Դͼ��ԭ���X����ƫ��
									  unsigned int	src_window_y,			// Դ���������Դͼ��ԭ���Y����ƫ��
									  unsigned int	src_window_width,		// Դ���ڵĿ��
									  unsigned int	src_window_height,	// Դ���ڵĸ߶�
									  
									  // ���ͼ����
									  unsigned int dst_width,				// ���ͼ��Ŀ��
									  unsigned int dst_height,				// ���ͼ��ĸ߶�
									  unsigned int dst_stride,				// ���ͼ����п��
									  
									  unsigned int mid_line_counter,
									  void (*mid_line_callback)(void *),
									  void * mid_line_user_data									  
									  );


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_ISP_SCALAR_H_
