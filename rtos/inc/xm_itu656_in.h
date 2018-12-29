// ITU601/ITU656����ԴΪYUV422
// �����ʽ�̶�ΪY_UV420(UV������)
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


// ��ȡITU656 IN��Ƶ������ߴ綨��
u32_t itu656_in_get_video_width  (void);
u32_t itu656_in_get_video_height (void);

// ��ȡITU656 IN�����ͼ���ֽڴ�С (Y_UV420��ʽ)
u32_t itu656_in_get_video_image_size (void);

// ��ȡITU656 IN��֡�����ַ
char *itu656_in_get_yuv_buffer (unsigned int frame_id);

// ��ITU656 IN�Ŀ���֡(frame_id)ѹ�뵽Ӳ������׼����������
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
