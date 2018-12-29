#ifndef _XM_ROTATE_H_
#define _XM_ROTATE_H_

#if defined (__cplusplus)
	extern "C"{
#endif
						 
// Y_UV420��ʽ
// ֧��2�ֹ��
// 1600*400	(1920x1080 --> 1600x400)
// 1280*320 (1280x720  --> 1280x320)
int xm_rotate_90 (
							unsigned char *y_src, 	// Դͼ��Y����
							unsigned char *uv_src,	// Դͼ��UV����
							int src_width,				// Դͼ����(���ص㵥λ)
							int src_height, 			// Դͼ��߶�(���ص㵥λ)
							int src_stride,			// Դͼ���п��(���ص㵥λ)
							// Դ��������(����Ϊ8�ı���)
							int src_window_x,			// �����Դͼ�����Ͻ�(0,0)��xƫ��, >= 0
							int src_window_y,			// �����Դͼ�����Ͻ�(0,0)��yƫ��, >= 0
							int src_window_width,	//	���������ؿ��
							int src_window_height,	//	���������ظ߶�
							
							unsigned char *y_dst,	//	Ŀ��ͼ��Y����	
							unsigned char *uv_dst,	//	Ŀ��ͼ��UV����	
							int dst_width,				// Ŀ��ͼ����(���ص㵥λ) == src_window_width
							int dst_height				// Ŀ��ͼ��߶�(���ص㵥λ) == src_window_height
						 );

// Y_UV420��ʽ
// ֧��2�ֹ��
// 1600*400	(1920x1080 --> 1600x400)
// 1280*320 (1280x720  --> 1280x320)
int xm_rotate_90_by_zoom (
							unsigned char *y_src, 	// Դͼ��Y����
							unsigned char *uv_src,	// Դͼ��UV����
							int src_width,				// Դͼ����(���ص㵥λ)
							int src_height, 			// Դͼ��߶�(���ص㵥λ)
							int src_stride,			// Դͼ���п��(���ص㵥λ)
							// Դ��������(����Ϊ8�ı���)
							int src_window_x,			// �����Դͼ�����Ͻ�(0,0)��xƫ��
							int src_window_y,			// �����Դͼ�����Ͻ�(0,0)��yƫ��
							int src_window_width,	//	���������ؿ��
							int src_window_height,	//	���������ظ߶�
							
							unsigned char *y_dst,	//	Ŀ��ͼ��Y����	
							unsigned char *uv_dst,	//	Ŀ��ͼ��UV����	
							int dst_width,				// Ŀ��ͼ����(���ص㵥λ)
							int dst_height				// Ŀ��ͼ��߶�(���ص㵥λ)
						 );


// Y_UV420��ʽ
// ֧��2�ֹ��
// 1600*400	(1920x1080 --> 1600x400)
// 1280*320 (1280x720  --> 1280x320)
int xm_rotate_90_by_zoom_opt (
							unsigned char *y_src, 	// Դͼ��Y����
							unsigned char *uv_src,	// Դͼ��UV����
							int src_width,				// Դͼ����(���ص㵥λ)
							int src_height, 			// Դͼ��߶�(���ص㵥λ)
							int src_stride,			// Դͼ���п��(���ص㵥λ)
							// Դ��������(����Ϊ8�ı���)
							int src_window_x,			// �����Դͼ�����Ͻ�(0,0)��xƫ��
							int src_window_y,			// �����Դͼ�����Ͻ�(0,0)��yƫ��
							int src_window_width,	//	���������ؿ��
							int src_window_height,	//	���������ظ߶�
							
							unsigned char *y_dst,	//	Ŀ��ͼ��Y����	
							unsigned char *uv_dst,	//	Ŀ��ͼ��UV����	
							int dst_width,				// Ŀ��ͼ����(���ص㵥λ)
							int dst_height,			// Ŀ��ͼ��߶�(���ص㵥λ)
							
							int horz_reverse,			// ˮƽ����
							int vert_reverse			// ��ֱ����
						 );

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_ROTATE_H_
