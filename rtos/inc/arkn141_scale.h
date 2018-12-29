/*
	scale.h
*/
#ifndef _SCALE_ARKN141_HEADFILE_
#define _SCALE_ARKN141_HEADFILE_

/*                                         */
/*          NOTE !!!!!!!!                  */
/*                                         */

/* 因为FPGA 版本 输出的YUV格式不一样 , 所以定义此宏定义开关
 * 某些版本 只支持 YUV420
 * 某些版本 只支持 Y_UV420
 *     0 = Y_UV420      使用最新Scale的版本
				=>使用版本：包含20150324_arkn141_scale_Y_UV420 之后的版本
 *     1 = YUV420 
				=>使用版本：包含20150321_arkn141_scale_YUV420 之前的版本
**/
//      add : 新版本之后，该宏定义一般是不需要修改的，因为新版本只支持 0=Y_UV420
#define  SCALE_FORMAT_YUV420        0



/*******************************************************************************
	以下不要轻易改动，或者仔细阅读之后再修改
********************************************************************************/
#define  FORMAT_YUV422              0
#define  FORMAT_YUV420              1
#define  FORMAT_YUYV                2
#define  FORMAT_YUV                 3
#define  FORMAT_YVYU                4
#define  FORMAT_Y_UV420             5
#define  FORMAT_Y_UV422             6
#define  FORMAT_YUV444              7 // 没有支持这种格式数据
#define  FORMAT_UYVY                8
#define  FORMAT_VYUY                9
#define  FORMAT_SCALE_ERROR         10

#define  YUV420_UVFORMAT_UVUV                    (1<<13)
#define  YUV420_UVFORMAT_VUVU                    (0<<13)
#define  YUV420_UVINTERWEAVE                     (1<<12)
#define  YUV420_UVSEPARATE                       (0<<12)

/* 
	rAXI_SCALE_INT_STATUS 寄存器状态用宏定义
*/
#define  WB_MIDNUM_FINISH_FLAG     (1<<6)
#define  WB_MIDNUM_FINISH_INT      (1<<5)
#define  WB_FRAME_FINISH_FLAG     (1<<1)
#define  WB_FRAME_FINISH_INT      (1<<0)

 
typedef enum
{
    SCALE_RET_OK = 0,//
    SCALE_RET_TIMEOUT = 1,
	 SCALE_RET_INV_PARA = 2,// 无效参数
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

    // 下面5项 是开窗参数
    unsigned short inwindow_enable;//输入 开窗
    unsigned short owindow_enable;// 输出 开窗
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

// 读取当前的middle finish计数
unsigned int arkn141_scalar_get_current_middle_line_count (void);

// 执行scalar处理，直到图像处理完毕或者异常结束(超时、回写bresp error)
int  arkn141_scalar_process (
					  unsigned char *yuv, // 输入数据
                 unsigned int inwindow_enable,unsigned int owindow_enable,
					  // 开窗坐标(x,y)          开窗大小 宽度与高度
					  int window_x,int window_y,int window_width,int window_height,
  
					  int iwidth, // 输入图像大小  宽度与高度
					  int iheight ,
					  int owidth, // 输出图像大小  宽度与高度
					  int oheight,
					  int ostride,
					  
					  int source_format, // Video layer source format
					  unsigned char *yout,// 输出数据
					  unsigned char *uout,
					  unsigned char *vout,
					  
					  unsigned int middle_finish_line_count,		// 0     表示关闭middle finish中断
					  															// 非0值 表示开启middle finish中断
					  void (*middle_finish_callback) (void *private_data, unsigned int current_line_count),	
					                                             // middle finish中断回调函数
					  void * middle_finish_private_data          // middle finish中断回调函数的私有数据
					   
					);

void  arkn141_scale_init (void);


// scalar缩放抗锯齿增强处理
int  arkn141_isp_scalar_anti_aliasing_enhance (
					  unsigned char *isp_scalar_1st_scalar_buffer, // 输入数据, 初级缩放缓冲区
					  int owidth, 		// 输出图像大小  宽度与高度
					  int oheight , 
					  int rot_w,		// 转置矩阵大小
					  
					  int source_format, // Video layer source format
					  unsigned char *yout// 输出数据
					);


#endif