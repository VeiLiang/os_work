#ifndef _XM_ROTATE_H_
#define _XM_ROTATE_H_

#if defined (__cplusplus)
	extern "C"{
#endif
						 
// Y_UV420格式
// 支持2种规格
// 1600*400	(1920x1080 --> 1600x400)
// 1280*320 (1280x720  --> 1280x320)
int xm_rotate_90 (
							unsigned char *y_src, 	// 源图像Y分量
							unsigned char *uv_src,	// 源图像UV分量
							int src_width,				// 源图像宽度(像素点单位)
							int src_height, 			// 源图像高度(像素点单位)
							int src_stride,			// 源图像行宽度(像素点单位)
							// 源开窗设置(必须为8的倍数)
							int src_window_x,			// 相对于源图像左上角(0,0)的x偏移, >= 0
							int src_window_y,			// 相对于源图像左上角(0,0)的y偏移, >= 0
							int src_window_width,	//	开窗的像素宽度
							int src_window_height,	//	开窗的像素高度
							
							unsigned char *y_dst,	//	目标图像Y分量	
							unsigned char *uv_dst,	//	目标图像UV分量	
							int dst_width,				// 目标图像宽度(像素点单位) == src_window_width
							int dst_height				// 目标图像高度(像素点单位) == src_window_height
						 );

// Y_UV420格式
// 支持2种规格
// 1600*400	(1920x1080 --> 1600x400)
// 1280*320 (1280x720  --> 1280x320)
int xm_rotate_90_by_zoom (
							unsigned char *y_src, 	// 源图像Y分量
							unsigned char *uv_src,	// 源图像UV分量
							int src_width,				// 源图像宽度(像素点单位)
							int src_height, 			// 源图像高度(像素点单位)
							int src_stride,			// 源图像行宽度(像素点单位)
							// 源开窗设置(必须为8的倍数)
							int src_window_x,			// 相对于源图像左上角(0,0)的x偏移
							int src_window_y,			// 相对于源图像左上角(0,0)的y偏移
							int src_window_width,	//	开窗的像素宽度
							int src_window_height,	//	开窗的像素高度
							
							unsigned char *y_dst,	//	目标图像Y分量	
							unsigned char *uv_dst,	//	目标图像UV分量	
							int dst_width,				// 目标图像宽度(像素点单位)
							int dst_height				// 目标图像高度(像素点单位)
						 );


// Y_UV420格式
// 支持2种规格
// 1600*400	(1920x1080 --> 1600x400)
// 1280*320 (1280x720  --> 1280x320)
int xm_rotate_90_by_zoom_opt (
							unsigned char *y_src, 	// 源图像Y分量
							unsigned char *uv_src,	// 源图像UV分量
							int src_width,				// 源图像宽度(像素点单位)
							int src_height, 			// 源图像高度(像素点单位)
							int src_stride,			// 源图像行宽度(像素点单位)
							// 源开窗设置(必须为8的倍数)
							int src_window_x,			// 相对于源图像左上角(0,0)的x偏移
							int src_window_y,			// 相对于源图像左上角(0,0)的y偏移
							int src_window_width,	//	开窗的像素宽度
							int src_window_height,	//	开窗的像素高度
							
							unsigned char *y_dst,	//	目标图像Y分量	
							unsigned char *uv_dst,	//	目标图像UV分量	
							int dst_width,				// 目标图像宽度(像素点单位)
							int dst_height,			// 目标图像高度(像素点单位)
							
							int horz_reverse,			// 水平反向
							int vert_reverse			// 垂直反向
						 );

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_ROTATE_H_
