#ifndef _DEINTERLACE_H_
#define _DEINTERLACE_H_

#define	DEINTERLACE_SUCCESS			(0)	// 成功
#define	DEINTERLACE_PARA_ERROR		(-1)	// 参数错误
#define	DEINTERLACE_AXI_ERROR		(-2)	// AXI写入异常
#define	DEINTERLACE_TIMEOUT			(-3)	// 超时

enum {
	DEINTERLACE_LINE_SIZE_720H = 0,
	DEINTERLACE_LINE_SIZE_960H,
};

enum {
	DEINTERLACE_DATA_MODE_420 = 0,
	DEINTERLACE_DATA_MODE_422
};

enum {
	DEINTERLACE_TYPE_PAL = 0,		// 576
	DEINTERLACE_TYPE_NTSC			// 480
};

enum {
	DEINTERLACE_FIELD_ODD = 0,
	DEINTERLACE_FIELD_EVEN
};

void deinterlace_init (void);
void deinterlace_exit (void);

int  deinterlace_process (unsigned int deinterlace_size, 
							 unsigned int data_mode,
							 unsigned int deinterlace_type,
							 unsigned int deinterlace_field,
							 unsigned char *src_field_addr_0,
							 unsigned char *src_field_addr_1,
							 unsigned char *src_field_addr_2,
							 unsigned char *dst_y_addr,
							 unsigned char *dst_u_addr,
							 unsigned char *dst_v_addr
							);


#endif	// _DEINTERLACE_H_
