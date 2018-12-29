/*
	scale.h
*/
#ifndef _ISP_SCALE_ARKN141_HEADFILE_
#define _ISP_SCALE_ARKN141_HEADFILE_

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
#define  FORMAT_SCALE_ERROR         16

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

#define  ISP_SCALE_BUF_MAX                    4

#define  OVERFLOW_CHECK_VALUE                 0x96

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

typedef struct _ispscaledata_s {
    struct _ispscaledata_s			*prev;
    struct _ispscaledata_s			*next;
    unsigned int   Ydata;
    unsigned int   Pbprdata;
} ispscaledata_s;


typedef struct _ispscale_s {
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
} ispscale_s;


ispscaledata_s *getscalebufferdata();

void ISPScaleIntHander();

void ISPScale_WB_Address_Reg_Init(  );
void ISPScale_WB_Address_Reg_fill(unsigned int count );
unsigned int return_ispscalecount();

void Reset_ispscalecount();

#endif