// ITU601/ITU656输入源为YUV422
// 输出格式固定为Y_UV420(UV交替存放)
//
#ifndef _XM_ITU656_IN_H_
#define _XM_ITU656_IN_H_

#ifdef  __cplusplus
extern "C" {
#endif  // __cplusplus
	
#define	XMSYS_ITU656_IN_TASK_STACK_SIZE				(0x2000)
#define	XMSYS_ITU656_IN_TASK_PRIORITY				120

	
typedef struct _itu656_window_s {

   unsigned int Left_cut;
   unsigned int right_cut;
   unsigned int up_cut;
   unsigned int down_cut;
   unsigned int width;
   unsigned int height;
   unsigned int real_w;
   unsigned int real_h;   
} itu656_window_s;


// 获取ITU656 IN视频的输出尺寸定义
u32_t itu656_in_get_video_width  (void);
u32_t itu656_in_get_video_height (void);

// 获取ITU656 IN的输出图像字节大小 (Y_UV420格式)
u32_t itu656_in_get_video_image_size (void);

// 获取ITU656 IN的帧缓存地址
char *itu656_in_get_yuv_buffer (unsigned int frame_id);

// 将ITU656 IN的空闲帧(frame_id)压入到硬件队列准备接收数据
void itu656_in_set_frame_ready (unsigned int frame_id);

int itu656_in_get_ready_frame_id (void);

void itu656_in_set_frame_ready1 (unsigned int frame_id);

void xm_itu656_in_start (void);
void xm_itu656_in_stop  (void);

void xm_itu656_in_init (void);


#ifdef __cplusplus
}
#endif          // __cplusplus


#endif	// _XM_ITU656_IN_H_
