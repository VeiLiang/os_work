/*
	scale.h
*/
#ifndef _SCALE_ARKN141_HEADFILE_
#define _SCALE_ARKN141_HEADFILE_

/*                                         */
/*          NOTE !!!!!!!!                  */
/*                                         */

/* ��ΪFPGA �汾 �����YUV��ʽ��һ�� , ���Զ���˺궨�忪��
 * ĳЩ�汾 ֻ֧�� YUV420
 * ĳЩ�汾 ֻ֧�� Y_UV420
 *     0 = Y_UV420      ʹ������Scale�İ汾
				=>ʹ�ð汾������20150324_arkn141_scale_Y_UV420 ֮��İ汾
 *     1 = YUV420 
				=>ʹ�ð汾������20150321_arkn141_scale_YUV420 ֮ǰ�İ汾
**/
//      add : �°汾֮�󣬸ú궨��һ���ǲ���Ҫ�޸ĵģ���Ϊ�°汾ֻ֧�� 0=Y_UV420
#define  SCALE_FORMAT_YUV420        0



/*******************************************************************************
	���²�Ҫ���׸Ķ���������ϸ�Ķ�֮�����޸�
********************************************************************************/
#define  FORMAT_YUV422              0
#define  FORMAT_YUV420              1
#define  FORMAT_YUYV                2
#define  FORMAT_YUV                 3
#define  FORMAT_YVYU                4
#define  FORMAT_Y_UV420             5
#define  FORMAT_Y_UV422             6
#define  FORMAT_YUV444              7 // û��֧�����ָ�ʽ����
#define  FORMAT_UYVY                8
#define  FORMAT_VYUY                9
#define  FORMAT_SCALE_ERROR         10

#define  YUV420_UVFORMAT_UVUV                    (1<<13)
#define  YUV420_UVFORMAT_VUVU                    (0<<13)
#define  YUV420_UVINTERWEAVE                     (1<<12)
#define  YUV420_UVSEPARATE                       (0<<12)

/* 
	rAXI_SCALE_INT_STATUS �Ĵ���״̬�ú궨��
*/
#define  WB_MIDNUM_FINISH_FLAG     (1<<6)
#define  WB_MIDNUM_FINISH_INT      (1<<5)
#define  WB_FRAME_FINISH_FLAG     (1<<1)
#define  WB_FRAME_FINISH_INT      (1<<0)

 
typedef enum
{
    SCALE_RET_OK = 0,//
    SCALE_RET_TIMEOUT = 1,
	 SCALE_RET_INV_PARA = 2,// ��Ч����
    SCALE_RET_DATA_ERROR = 2,
    SCALE_RET_HW_ERROR = 4,
    SCALE_RET_SYSTEM_ERROR = 5,
    SCALE_RET_HW_RESET = 6,
	 SCALE_RET_FAILED = 7,
	 SCALE_RET_MALLOC_FAILED = 8,
	 SCALE_RET_COMPARE_NG = 9
} SCALE_RET;


typedef struct _scale_s {
    unsigned short inwidth;
    unsigned short inheight;
    unsigned short owidth;
    unsigned short oheight;
    unsigned short Midline; 
    unsigned short ostride;
    unsigned int   source_format;

    // ����5�� �ǿ�������
    unsigned short inwindow_enable;//���� ����
    unsigned short owindow_enable;// ��� ����
    unsigned short inwindow_x;
    unsigned short inwindow_y;
    unsigned short inwindow_width;
    unsigned short inwindow_height;
    unsigned char *yuv;
    unsigned char *yout;
    unsigned char *uout;
    unsigned char *vout;
} scale_s;

void scale_arkn141_set_writeback_addr(unsigned int Yaddr ,unsigned int Uaddr ,unsigned int Vaddr);


void scale_arkn141_writeback_start();


//#pragma inline
void  scale_arkn141_open();


//#pragma inline
void  scale_arkn141_close();



void ScaleIntHander();

void Scale_Int_init (int status );

// ��ȡ��ǰ��middle finish����
unsigned int arkn141_scalar_get_current_middle_line_count (void);

// ִ��scalar����ֱ��ͼ������ϻ����쳣����(��ʱ����дbresp error)
int  arkn141_scalar_process (
					  unsigned char *yuv, // ��������
                 unsigned int inwindow_enable,unsigned int owindow_enable,
					  // ��������(x,y)          ������С �����߶�
					  int window_x,int window_y,int window_width,int window_height,
  
					  int iwidth, // ����ͼ���С  �����߶�
					  int iheight ,
					  int owidth, // ���ͼ���С  �����߶�
					  int oheight,
					  int ostride,
					  
					  int source_format, // Video layer source format
					  unsigned char *yout,// �������
					  unsigned char *uout,
					  unsigned char *vout,
					  
					  unsigned int middle_finish_line_count,		// 0     ��ʾ�ر�middle finish�ж�
					  															// ��0ֵ ��ʾ����middle finish�ж�
					  void (*middle_finish_callback) (void *private_data, unsigned int current_line_count),	
					                                             // middle finish�жϻص�����
					  void * middle_finish_private_data          // middle finish�жϻص�������˽������
					   
					);

void  arkn141_scale_init (void);


// scalar���ſ������ǿ����
int  arkn141_isp_scalar_anti_aliasing_enhance (
					  unsigned char *isp_scalar_1st_scalar_buffer, // ��������, �������Ż�����
					  int owidth, 		// ���ͼ���С  �����߶�
					  int oheight , 
					  int rot_w,		// ת�þ����С
					  
					  int source_format, // Video layer source format
					  unsigned char *yout// �������
					);


#endif