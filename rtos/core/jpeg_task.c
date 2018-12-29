#include <stdio.h>
#include "hardware.h"
#include "RTOS.h"		// OSͷ�ļ�
#include "FS.h"
#include <xm_type.h>
#include "xm_core.h"
#include <xm_dev.h>
#include <xm_queue.h>
#include <assert.h>
#include <string.h>
#include <xm_printf.h>
#include <xm_core_linear_memory.h>
#include "arkn141_codec.h"
#include "xm_isp_scalar.h"
#include "xm_file.h"
#include "arkn141_isp.h"

#define	JPEG_PRINT	XM_printf
#define	JPEG_FATAL	XM_printf	

static int jpeg_dummy_mode_init (void);
static int jpeg_dummy_mode_exit (void);

static void jpeg_isp_scalar_frame_free_user_callback (void *private_data);

extern int XMSYS_JpegFrameInsertExtendMessage (unsigned char uvc_channel, char *lpCdrBuffer, int cbCdrBuffer);
extern void XMSYS_CdrFilePrefetch (void);


// �������DVR���ݵ�section
// ͬʱ��Ҫ--image_input�ж���_DVR_SECTION_
#pragma section="_DVR_SECTION_"	

static char *dvr_buffer;
static DWORD  dvr_length;
extern U32 _DVR_BIN_;
void XM_DvrInit (void)
{
	dvr_buffer = (char *)&_DVR_BIN_;
	dvr_length = __section_size("_DVR_SECTION_");
	//printf ("dvr_buffer=%08x, dvr_length=%d\n", dvr_buffer, dvr_length);
}



static OS_TIMER JpegTimer;		// ��ʱ��

static OS_TASK TCB_JpegTask;
__no_init static OS_STACKPTR int StackJpegTask[XMSYS_JPEG_TASK_STACK_SIZE/4];          /* Task stacks */

static unsigned int jpeg_mode;

static int Quantization_level = 7;		// JPEG������������,Ӱ������


#define	JPEG_ISP_SCALAR_FRAME_COUNT		4
#define	JPEG_ISP_SCALAR_FRAME_ID			0x4653494a	// JISF

// ��ISP scalarģʽ��صı���
static int jpeg_isp_scalar_object;			// ISP scalar�����ʶ��
static queue_s jpeg_isp_scalar_data;		// ������׼���õ�ISP scalar֡����
static XMSYSSENSORPACKET jpeg_isp_scalar_fifo[JPEG_ISP_SCALAR_FRAME_COUNT];		// 
static void jpeg_isp_scalar_data_ready_user_callback (void *private_data);

// ��IMAGEģʽ��صı���
static char *image_file_data;					// ����IMAGEģʽ��ͼƬ�ļ�����
static int image_file_size;

#define	JPEG_FRAME_COUNT		2
#define	JPEG_FRAME_ID			0x4745504A

typedef struct tagXMJPEGFRAME {
	void *prev;
	void *next;
	unsigned int 	id;
	unsigned char *base;				// JPEG֡��������ַ
	unsigned int	size;				// JPEG֡�������ֽڴ�С
	unsigned char *fifo;				// JPEG֡��������ǰд��ָ��
	
	xm_core_linear_memory_t mem;

} XMJPEGFRAME;

static void XMSYS_JpegFrameDelete (void *lpJpegFrame, int ErrCode);

static XMJPEGFRAME jpeg_frame[JPEG_FRAME_COUNT];
static queue_s jpeg_free_link;
static OS_RSEMA	JpegSema;	

// UVC ���ڶ���
static unsigned int uvc_src_window_x, uvc_src_window_y, uvc_src_window_w, uvc_src_window_h;


// һ����˵, JPG�����ĸ�ʽ��YUV��ʽԴ���ݴ�С��1/6����.
// �˴�����1/4ԭʼ��ʽ��С����JPG������
#define	MAX_JPEG_FRAME_SIZE	(1920 * 1080 * 3/(2*3))

void XMSYS_JpegFrameInit (void)
{
	int i;
	memset (&jpeg_frame, 0, sizeof(jpeg_frame));
	queue_initialize (&jpeg_free_link);
	for (i = 0; i < JPEG_FRAME_COUNT; i ++)
	{
		if(xm_core_allocate_linear_memory (MAX_JPEG_FRAME_SIZE, 0x1000, &jpeg_frame[i].mem) < 0)
		{
			printf ("allocae jpeg frame %d failed\n", i);
		}
		else
		{
			jpeg_frame[i].base = (unsigned char *)jpeg_frame[i].mem.bus_address;
			jpeg_frame[i].size = MAX_JPEG_FRAME_SIZE;
			jpeg_frame[i].fifo = jpeg_frame[i].base;
			queue_insert ((queue_s *)&jpeg_frame[i], &jpeg_free_link);
		}
	}
	
	jpeg_isp_scalar_object = -1;
	// jpeg_mode = XMSYS_JPEG_MODE_ISP;		// �ᵼ��jpeg_task����
	jpeg_mode = XMSYS_JPEG_MODE_ISP_SCALAR;		// 
	queue_initialize (&jpeg_isp_scalar_data);
	memset (jpeg_isp_scalar_fifo, 0, sizeof(jpeg_isp_scalar_fifo));
	for (i = 0; i < JPEG_ISP_SCALAR_FRAME_COUNT; i++)
	{
		jpeg_isp_scalar_fifo[i].id = JPEG_ISP_SCALAR_FRAME_ID;
	}
	
	XM_DvrInit ();
		
	OS_CREATERSEMA (&JpegSema); /* Creates resource semaphore */
}

// ����һ���µ�JPEG֡
void * XMSYS_JpegFrameCreate (void)
{
	XMJPEGFRAME *frame = NULL;
	OS_Use (&JpegSema);
	if(queue_empty (&jpeg_free_link))
	{
		OS_Unuse (&JpegSema);
		return NULL;
	}
	
	frame = (XMJPEGFRAME *)queue_delete_next (&jpeg_free_link);
	frame->id = JPEG_FRAME_ID;
	frame->fifo = frame->base;	
	OS_Unuse (&JpegSema);
	
	return frame;	
}

void XMSYS_JpegFrameRetransfer (void *lpJpegFrame)
{
	printf ("XMSYS_JpegFrameRetransfer %x\n", lpJpegFrame);
	
}

// JPEG֡������׼����ϣ��ύ�ȴ�USB����
void XMSYS_JpegFrameSubmit (void *lpJpegFrame)
{
	XMJPEGFRAME *frame = lpJpegFrame;
	int err_code;
	if(frame == NULL)
		return;
	
	//printf ("base=%x,data=%x, size=%x\n", frame->base, frame->data,  frame->fifo - frame->base);
	err_code = XMSYS_CameraNewDataFrame (	(int *)frame->base, frame->fifo - frame->base, frame, XMSYS_JpegFrameDelete,
					XMSYS_JpegFrameRetransfer);
	if(err_code != XMSYS_CAMERA_SUCCESS)
	{
		// ֡��������ʧ��
		// �ͷ�����Դ
		XMSYS_JpegFrameDelete (frame, err_code);
	}
	else
	{
		// ֡�Ѵ���, ִ���ϴ��ļ�Ԥȡ
		XMSYS_CdrFilePrefetch ();
	}
}

// USB��������ص�����, ��Camera�������
// JPEG�Ѵ�����ϣ��ͷ�����Դ
static void XMSYS_JpegFrameDelete (void *lpJpegFrame, int ErrCode)
{
	XMJPEGFRAME *frame = lpJpegFrame;
	// printf ("XMSYS_JpegFrameDelete %x, code=%d\n", lpJpegFrame, ErrCode);
	if(frame == NULL)
		return;
	if(frame->id != JPEG_FRAME_ID)
	{
		JPEG_FATAL ("illegal jpeg frame %x\n", frame);
		return;
	}
	frame->id = 0;
	OS_Use (&JpegSema);
	queue_insert ((queue_s *)frame, &jpeg_free_link);
	OS_Unuse (&JpegSema);
}
#define	HYBRID_H264_ID				0x00E0FFFF

int arkn141_jpeg_encode (	
	char *imgY_in, 			// ����ͼ���Y/Cb/Cr
	char *imgCb_in, 
	char *imgCr_in ,
	unsigned int frame_type,
	char qLevel,						// Quantization level (0 - 9)
	int width,							// ����ͼ��Ŀ��/�߶�
	int height,
	unsigned char *jpeg_stream, 	// ����JPEG�������Ļ�����
	int *jpeg_len						// in,  ��ʾJPEG�������Ļ��������ֽڳ���
											// Out, ��ʾ����ɹ���JPEG�������ֽڳ���
);

int  arkn141_scalar_process (
					  unsigned char *yuv, // ��������
                 unsigned int inwindow_enable,unsigned int owindow_enable,
					  // ��������(x,y)          ������С �����߶�
					  int window_x,int window_y,int window_width,int window_height,
  
					  int iwidth, // ����ͼ���С  �����߶�
					  int iheight ,
					  
					  int owidth, 	// ���ź��ͼ������߶�
					  int oheight,  
					  int ostride,	// ���ͼ���ÿ�е���
					  
					  int source_format, // Video layer source format
					  unsigned char *yout,// �������
					  unsigned char *uout,
					  unsigned char *vout,
					  
					  unsigned int middle_finish_line_count,		// 0     ��ʾ�ر�middle finish�ж�
					  															// ��0ֵ ��ʾ����middle finish�ж�
					  void (*middle_finish_callback) (void *private_data, unsigned int current_line_count),	
					                                             // middle finish�жϻص�����
																				//        �˻ص����������ж��������б�����
					  void * middle_finish_private_data          // middle finish�жϻص�������˽������
					   
					);

// scalar���ſ������ǿ����
int  arkn141_isp_scalar_anti_aliasing_enhance (
					  unsigned char *isp_scalar_bugfix_1st_scalar_buffer, // ��������
					  int owidth, // ���ͼ���С  �����߶�
					  int oheight ,    
					  int rot_w,
					  
					  int source_format, // Video layer source format
					  unsigned char *yout// �������
					);

#define	JPEG_DEBUG		// JPEG���ʿ���

#ifdef JPEG_DEBUG
static unsigned int eval_jpeg_count = 0;		// ��¼֡��
static unsigned int eval_jpeg_start_ticket = 0;	// ͳ�ƿ�ʼ��ʱ��
static unsigned int eval_jpeg_encode_ticket;		// �ܱ���ʱ��
static unsigned int eval_jpeg_encode_bytes;		// �ܱ����ֽڴ�С
#endif

extern int jpeg_get_quantization_level (void);


// ��һ��YUV��Ƶ֡���뵽JPEG֡��
// ����ֵ
//	0		�ɹ�
// -1		ʧ��
int XMSYS_JpegFrameInsertVideoStream (void *lpJpegFrame, void *lpVideoFrame, int nVideoCx, int nVideoCy)
{
	XMSIZE size;
	int *ScaleFrame;
	int jpeg_len, yuv_len;
	XMJPEGFRAME *frame = lpJpegFrame;
	unsigned char *y, *cb, *cr;
	unsigned char *scale_fifo_addr = NULL;
	unsigned char *scale_fifo;
	int jpeg_ret;
#ifdef JPEG_DEBUG
	XMINT64	eval_jpeg_current_ticket;
#endif
	
	if(frame == NULL)
		return -1;
	if(frame->id != JPEG_FRAME_ID)
	{
		JPEG_FATAL ("illegal jpeg frame %x\n", frame);
		return -1;
	}
	
#ifdef JPEG_DEBUG
	eval_jpeg_current_ticket = XM_GetHighResolutionTickCount();
	if(eval_jpeg_count == 0)
		eval_jpeg_start_ticket = XM_GetTickCount();
#endif
	
	XMSYS_CameraGetWindowSize (&size);
	
	// JPEG����
	if(nVideoCx == size.cx && nVideoCy == size.cy)
	{
		// ��ISP��ͬ�ߴ�
		y = (unsigned char *)lpVideoFrame;
		cb = y + nVideoCx * nVideoCy;
		cr = y + nVideoCx * nVideoCy * 5/4;
		jpeg_len = frame->size;
	}
	else
	{
		// ��ISP��ͬ�ߴ�, ִ��scale down
		#define  FORMAT_Y_UV420             5
		unsigned int scale_file_size_aligned;
		
		if(jpeg_mode == XMSYS_JPEG_MODE_ISP)
		{
			// ISP ֡ --> Scalar ֡ --> JPG ֡
			// ����scalar�м仺��
			scale_file_size_aligned = size.cx * size.cy * 3/2;
			scale_file_size_aligned = (scale_file_size_aligned + 1023) & (~(1023));
			scale_fifo_addr = kernel_malloc (scale_file_size_aligned + 2048);
			if(scale_fifo_addr == NULL)
			{
				JPEG_FATAL ("jpeg failed, can't allocate scalar memory\n");
				return -1;
			}
			scale_fifo = (unsigned char *)( ((unsigned int)(scale_fifo_addr + 1023)) & ~(1023));
			
			dma_inv_range ((unsigned int)scale_fifo, scale_file_size_aligned + (unsigned int)scale_fifo);
		
			// scale down
			y = scale_fifo;
			cb = y + size.cx * size.cy;
			cr = y + size.cx * size.cy * 5/4;
			if(uvc_src_window_w && uvc_src_window_h)
			{
				arkn141_scalar_process (lpVideoFrame, 1, 0,
										uvc_src_window_x, uvc_src_window_y, uvc_src_window_w, uvc_src_window_h,
										nVideoCx, nVideoCy,
										size.cx, size.cy, size.cx,
										FORMAT_Y_UV420,
										y, cb, cr,
										0, 0, 0);
				
			}
			else
			{
				arkn141_scalar_process (lpVideoFrame, 0, 0,
										0, 0, nVideoCx, nVideoCy,
										nVideoCx, nVideoCy,
										size.cx, size.cy, size.cx,
										FORMAT_Y_UV420,
										y, cb, cr,
										0, 0, 0);
			}
			jpeg_len = frame->size;
		}
		else if(jpeg_mode == XMSYS_JPEG_MODE_ISP_SCALAR)
		{
			// ISP Scalar(�м�֡) --> ת�ü���ת�� --> JPG֡
			scale_file_size_aligned = size.cx * size.cy * 3/2;
			scale_file_size_aligned = (scale_file_size_aligned + 1023) & (~(1023));
			scale_fifo_addr = kernel_malloc (scale_file_size_aligned + 2048);
			if(scale_fifo_addr == NULL)
			{
				JPEG_FATAL ("jpeg failed, can't allocate scalar memory\n");
				return -1;
			}
			scale_fifo = (unsigned char *)( ((unsigned int)(scale_fifo_addr + 1023)) & ~(1023));
			
			dma_inv_range ((unsigned int)scale_fifo, scale_file_size_aligned + (unsigned int)scale_fifo);
		
			// scale down
			y = scale_fifo;
			cb = y + size.cx * size.cy;
			cr = y + size.cx * size.cy * 5/4;
			// scalar���ſ������ǿ����
			arkn141_isp_scalar_anti_aliasing_enhance (lpVideoFrame, 
										size.cx, size.cy,
										nVideoCy,
										FORMAT_Y_UV420,
										y);
			jpeg_len = frame->size;			
		}
		else
		{
			printf ("illegal jpeg_mode(%d)\n", jpeg_mode);
			return -1;
		}
		
	}

	yuv_len = jpeg_len;
	// ��ʱ, ����ԭʼYUV֡�������ź��֡���ݾ�����Ӳ������, ������Ч֡�������� 
	//dma_inv_range ((unsigned int)frame->fifo, jpeg_len + (unsigned int)frame->fifo);

	// JPEG code
	if(arkn141_codec_open (1) < 0)
	{
		if(scale_fifo_addr)
			kernel_free (scale_fifo_addr);
		return -1;
	}
		
	jpeg_ret = arkn141_jpeg_encode ((char *)y, (char *)cb, (char *)cr, 
								1,
#if HIGH_VIDEO_QUALITY
								7,		// Quantization level
#else
								//7,		// Quantization level,	182310 bytes /֡(1024*600)
								//6,		// Quantization level,  141959 bytes /֡(1024*600)
								//5,		// Quantization level,  116820 bytes /֡(1024*600)
								//4,		// Quantization level,  116820 bytes /֡(1024*600)
								Quantization_level,
#endif
								size.cx, size.cy,
								frame->fifo, &jpeg_len);
	
	arkn141_codec_close (1);
	
	if(scale_fifo_addr)
		kernel_free (scale_fifo_addr);
	
	
	// ���jpeg�����Ƿ��쳣
	if(jpeg_ret != 0 || jpeg_len == 0)
		return -1;
	
#ifdef JPEG_DEBUG
	// ʹ��ƽ����������JPEG����������, ���ʸ�ʱ����;���ʵ�ʱ����;
	eval_jpeg_count ++;
	unsigned int ticket = (unsigned int)(XM_GetHighResolutionTickCount() - eval_jpeg_current_ticket);
	eval_jpeg_encode_ticket += ticket;		// �ۼ��ܱ���ʱ��
	eval_jpeg_encode_bytes += jpeg_len;
	if(eval_jpeg_count == 50)		// ÿ50֡����
	{
		unsigned int e_ticket = XM_GetTickCount() - eval_jpeg_start_ticket;
		unsigned int avg_stream = eval_jpeg_encode_bytes / eval_jpeg_count;
		printf ("jpeg fps=%d, ticket=%d, avg_size=%d\n", eval_jpeg_count * 10000 / (XM_GetTickCount() - eval_jpeg_start_ticket) , 
				  (unsigned int)(eval_jpeg_encode_ticket / eval_jpeg_count), 
				  avg_stream);
		if(avg_stream >= 130000)
			Quantization_level --;
		else if(avg_stream < 90000)
			Quantization_level ++;
		if(Quantization_level < 3)
			Quantization_level = 3;
		else if(Quantization_level > 8)
			Quantization_level = 8;
		
		printf ("q_level=%d\n", Quantization_level);
		
		eval_jpeg_start_ticket = 0;
		eval_jpeg_count = 0;
		eval_jpeg_encode_ticket = 0;
		eval_jpeg_encode_bytes = 0;
		
	}
#endif	
	
	
	unsigned int aligned_jpeg_len = (jpeg_len + 0x1F) & (~0x1F);
	if(aligned_jpeg_len > yuv_len)
	{
		printf ("XMSYS_JpegFrameInsertVideoStream error, aligned_jpeg_len(%d) > yuv_len(%d)\n", aligned_jpeg_len, yuv_len);
	}
	
	// frame->fifo�ĵ�ַ��non-cache�ռ�ĵ�ַ, ������Ч����
	// dma_inv_range ((unsigned int)frame->fifo, jpeg_len + (unsigned int)frame->fifo);
	// dma_inv_range ((unsigned int)frame->fifo, aligned_jpeg_len + (unsigned int)frame->fifo);
	frame->fifo += jpeg_len;	
	
	return 0;
}

static void JpegTicketCallback (void)
{
	//printf ("JpegTicketCallback\n");
	OS_SignalEvent(XMSYS_JPEG_EVENT_TICKET, &TCB_JpegTask); /* ֪ͨ�¼� */
	OS_RetriggerTimer (&JpegTimer);
}

// ���ڶ��ֲ�ͬԴͼ���ȡģʽ
// 1) ISP YUV Image
// 2) ISP Scalar YUV Image
// 3) 601 In YUV Image
// 4) 601 In Scalar YUV Image 
static void JpegCameraFrameTransfer (void)
{
	XMSYSSENSORPACKET *sensorPacket = NULL;
	
	XMJPEGFRAME *jpegFrame = NULL;
	int ret = -1;		// ��ʼ���Ϊʧ��
	
	OS_Use(&JpegSema);
	
	do 
	{
		// ����һ��UVC֡
		jpegFrame = XMSYS_JpegFrameCreate ();
		if(jpegFrame == NULL)
			break;
		
		// ����ģʽ, Ƕ�벻ͬ��JPEGԴ��UVC������ȥ
		if(jpeg_mode == XMSYS_JPEG_MODE_ISP)
		{
			// Դ��������ISP
			
			// ���sensor֡
			sensorPacket = XMSYS_SensorCreatePacket (0);
			if(sensorPacket == NULL)	// û�п��õ�sensor֡
				break;
			
			//printf ("XMSYS_JpegFrameInsertVideoStream = %x, camera=%x, w=%d, h=%d\n", jpegFrame, sensorPacket->buffer,
			//		  	sensorPacket->width, sensorPacket->height);
			ret = XMSYS_JpegFrameInsertVideoStream (jpegFrame, sensorPacket->buffer, sensorPacket->width, sensorPacket->height);
			//printf ("XMSYS_SensorDeletePacket %x\n", sensorPacket);
			XMSYS_SensorDeletePacket (0, sensorPacket);
		}	// if(jpeg_mode == XMSYS_JPEG_MODE_ISP)
		
		else if(jpeg_mode == XMSYS_JPEG_MODE_ISP_SCALAR)
		{
			// Դ��������ISP Scalar
			sensorPacket = NULL;
			
			// ��ready����ȡ��һ��֡����
			XM_lock ();		// ��ֹISP scalar�жϲ�������
			if(!queue_empty(&jpeg_isp_scalar_data))
			{
				// ȡ�������еĵ�һ����Ԫ���������жϿ�
				sensorPacket = (XMSYSSENSORPACKET *)queue_delete_next (&jpeg_isp_scalar_data);
			}
			XM_unlock ();
			if(sensorPacket == NULL)
				break;
			
			// jpeg����
			ret = XMSYS_JpegFrameInsertVideoStream (jpegFrame, sensorPacket->data[0], sensorPacket->width, sensorPacket->height);
			
			// ��isp scalar֡����ѹ�뵽Ӳ��FIFO
			xm_isp_scalar_register_user_buffer (jpeg_isp_scalar_object,
																(u32_t)sensorPacket->data[0], (u32_t)sensorPacket->data[1],
																jpeg_isp_scalar_data_ready_user_callback, 
																jpeg_isp_scalar_frame_free_user_callback,
																sensorPacket
															);
		}	// else if(jpeg_mode == XMSYS_JPEG_MODE_ISP_SCALAR)
		
		else if(jpeg_mode == XMSYS_JPEG_MODE_VIDEO)
		{
			// UVC��������һ����Ƶ�ļ�, H264����-->Scale Down-->JPEG����-->UVC����
			
			memcpy (jpegFrame->fifo, dvr_buffer, dvr_length);
			jpegFrame->fifo += dvr_length;
			ret = 0;
			
			//ret = -1;
		}	// else if(jpeg_mode == XMSYS_JPEG_MODE_VIDEO)
		
		else if(jpeg_mode == XMSYS_JPEG_MODE_IMAGE)
		{
			// UVC��������һ��JPEGͼƬ, JPEG�ļ�-->UVC����	
			if(image_file_data && image_file_size < jpegFrame->size )
			{
				// ��IMAGEͼƬ(��ʽΪJPEG)Ƕ�뵽UVC����
				memcpy (jpegFrame->fifo, image_file_data, image_file_size);
				jpegFrame->fifo += image_file_size;
				ret = 0;
			}
			else
			{
				XM_printf ("JpegCameraFrameTransfer failed, miss image file data\n");
			}
			
		}	// else if(jpeg_mode == XMSYS_JPEG_MODE_IMAGE)
		
		else if(jpeg_mode == XMSYS_JPEG_MODE_DUMMY)
		{
			// Ƕ��һ��������С��ȫ��ͼƬ��UVC������, �������ַ�ʽ���и�����Ƶ�ļ����ص�
			memcpy (jpegFrame->fifo, dvr_buffer, dvr_length);
			jpegFrame->fifo += dvr_length;
			ret = 0;
		}	// else if(jpeg_mode == XMSYS_JPEG_MODE_NULL)
		
	} while (0);
	
	if(ret == 0 && jpegFrame)
	{
		// Ƕ��UVC�첽��Ϣ��
		int asyc_size;
		asyc_size = XMSYS_JpegFrameInsertExtendMessage (0, (char *)jpegFrame->fifo, jpegFrame->size - (jpegFrame->fifo - jpegFrame->base));
		if(asyc_size > 0)
		{
			jpegFrame->fifo += asyc_size;
		}
		else
		{
			XM_printf ("JpegCameraFrameTransfer warning, InsertExtendMessage error\n");
		}
		// �ύUVC������ʼ����
		XMSYS_JpegFrameSubmit (jpegFrame);
	}
	else if(jpegFrame)
	{
		// ɾ��UVC��
		XMSYS_JpegFrameDelete (jpegFrame, 0);
	}
	
	OS_Unuse(&JpegSema);
}

void XMSYS_JpegTask (void)
{
	OS_U8 jpeg_event;
	unsigned long ticket;
	
	//printf ("XMSYS_JpegTask\n");
	
	// ����һ��33ms��ʱ�� (30Hz)
	OS_CREATETIMER (&JpegTimer, JpegTicketCallback, 33);
	
	while(1)
	{
		// �ȴ��ⲿ�¼�
		jpeg_event = OS_WaitEvent(		XMSYS_JPEG_EVENT_ENCODE
										  | 	XMSYS_JPEG_EVENT_TICKET
											);
		
		if(jpeg_event & XMSYS_JPEG_EVENT_ENCODE)
		{							
		}
		
		//OS_Delay (2);

		// ��ʱ��30֡/s
		if(jpeg_event & XMSYS_JPEG_EVENT_TICKET)
		{
			if(XMSYS_CameraIsReady())
			{
				// ����Camera�豸׼���õ������ִ��
				JpegCameraFrameTransfer ();
			}
		}
		
	}
}

// JPEG�����ʼ��
void XMSYS_JpegInit (void)
{
	//arkn141_scale_init ();
	XMSYS_JpegFrameInit ();
		
	OS_CREATETASK(&TCB_JpegTask, "JpegTask", XMSYS_JpegTask, XMSYS_JPEG_TASK_PRIORITY, StackJpegTask);
}

// JPEG�������
void XMSYS_JpegExit (void)
{
	
}

static void jpeg_isp_scalar_data_ready_user_callback (void *private_data)
{
	XM_lock ();
	
#if 1
	if(jpeg_isp_scalar_object >= 0 && !queue_empty(&jpeg_isp_scalar_data))
	{
		XMSYSSENSORPACKET *scalar_framepacket = (XMSYSSENSORPACKET *)queue_delete_next (&jpeg_isp_scalar_data);
		// ������֡����ѹ�뵽Ӳ�����н�������ISP-Scalar��ʵʱ����
		//if(jpeg_isp_scalar_object >= 0)
		{
			xm_isp_scalar_register_user_buffer (jpeg_isp_scalar_object,
									(u32_t)scalar_framepacket->data[0], (u32_t)scalar_framepacket->data[1],
									jpeg_isp_scalar_data_ready_user_callback, 
									jpeg_isp_scalar_frame_free_user_callback, 
									scalar_framepacket
								);
		}
	}	
#endif
	queue_insert ((queue_s *)private_data, &jpeg_isp_scalar_data);
	
	XM_unlock ();
	
}

static void jpeg_isp_scalar_frame_free_user_callback (void *private_data)
{
	XM_lock ();
	queue_insert ((queue_s *)private_data, &jpeg_isp_scalar_data);
	XM_unlock ();
}

static void jpeg_isp_scalar_mode_exit (void)
{
	int i;
	if(jpeg_isp_scalar_object >= 0)
	{
		xm_isp_scalar_delete (jpeg_isp_scalar_object);
		jpeg_isp_scalar_object = -1;
	}
	for (i = 0; i < JPEG_ISP_SCALAR_FRAME_COUNT; i ++)
	{
		if(jpeg_isp_scalar_fifo[i].buffer)
		{
#ifdef ISP_SCALAR_USE_STATIC_FIFO
#else			
			kernel_free (jpeg_isp_scalar_fifo[i].buffer);
#endif
			jpeg_isp_scalar_fifo[i].buffer = NULL;
		}
	}	
}

#define	MIDDLE_ROTATE_SIZE		352		// �м�ת�þ���ĳߴ�
static int jpeg_isp_scalar_mode_init (void)
{
	int ret = -1;
	int i;
	XMSIZE size;
	int height;
	// ����Camera�ߴ紴��һ��scalar����
	assert (jpeg_isp_scalar_object == -1);
	do
	{
		queue_initialize (&jpeg_isp_scalar_data);

		XMSYS_CameraGetWindowSize (&size);
		if(jpeg_mode == XMSYS_JPEG_MODE_ISP_SCALAR)
		{
			// ISP Scalar
			if(size.cx >= 640 && size.cy >= 480)	// VGA����
			{
				// 640  x 480
				// 1280 x 720
				// �����м䴦��
				height = size.cy;
			}
			else
			{
				// 480 x 272
				// 320 x 240
				// ִ���м�����
				// ���ŵ� 480 x 352 ���� 320 x 352
				height = MIDDLE_ROTATE_SIZE;	
			}
		}
		else
		{
			// Scalar
			height = size.cy;
		}
		ret = xm_isp_scalar_create (size.cx, height );
		if(ret < 0)
		{
			XM_printf ("jpeg_isp_scalar_mode_init failed, can't create scalar object\n");
			return -1;
		}
		jpeg_isp_scalar_object = ret;
		// ����֡�ڴ�
		for (i = 0; i < JPEG_ISP_SCALAR_FRAME_COUNT; i ++)
		{
			u32_t image_size = size.cx * height * 3/2 + 2048;	
			jpeg_isp_scalar_fifo[i].width = size.cx;
			jpeg_isp_scalar_fifo[i].height = height;
			//jpeg_isp_scalar_fifo[i].buffer = isp_scalar_get_frame_buffer (i, image_size);//yong dakai
			if(jpeg_isp_scalar_fifo[i].buffer == NULL)
			{
				printf ("jpeg_isp_scalar_mode_init failed, allocate memory NG\n");
				goto err_exit;
			}
/*
#ifdef ISP_SCALAR_USE_STATIC_FIFO
			// ʹ�� i + 1 ȷ����Ԥ��һ֡�ռ����ڱ���λ�ھ�̬������(isp_scalar_static_fifo)֮��ı���, ����scalar bug����ʱ�����������������д��.
			// ������4֡scalar FIFO֡, ʵ�ʱ���������5֡�ռ�, ���һ֡�����쳣����.
			if( image_size * (i + 1) > sizeof(isp_scalar_static_fifo))
			{
				printf ("jpeg_isp_scalar_mode_init failed, scalar_static_fifo too small to allocate fifo\n");
				jpeg_isp_scalar_fifo[i].buffer = 0;
				goto err_exit;
			}
			jpeg_isp_scalar_fifo[i].buffer = isp_scalar_static_fifo + i * image_size;
#else 
			jpeg_isp_scalar_fifo[i].buffer = kernel_malloc (image_size);
			if(jpeg_isp_scalar_fifo[i].buffer == NULL)
			{
				printf ("jpeg_isp_scalar_mode_init failed, allocate memory NG\n");
				goto err_exit;
			}
#endif
*/
			//dma_inv_range ((unsigned int)(jpeg_isp_scalar_fifo[i].buffer), (unsigned int)(jpeg_isp_scalar_fifo[i].buffer) + size.cx * size.cy * 3/2 + 2048);
			jpeg_isp_scalar_fifo[i].data[0] = (char *)(((unsigned int)(jpeg_isp_scalar_fifo[i].buffer) + 1023) & ~1023);
			jpeg_isp_scalar_fifo[i].data[1] = jpeg_isp_scalar_fifo[i].data[0] + size.cx * height;
			
			dma_inv_range ((unsigned int)(jpeg_isp_scalar_fifo[i].data[0]), (unsigned int)(jpeg_isp_scalar_fifo[i].data[0]) + size.cx * height * 3/2);
		}
		
		// �������֡����ע�ᵽISP scalar����
		for (i = 0; i < JPEG_ISP_SCALAR_FRAME_COUNT; i ++)
			xm_isp_scalar_register_user_buffer (jpeg_isp_scalar_object,
																	(u32_t)jpeg_isp_scalar_fifo[i].data[0], (u32_t)jpeg_isp_scalar_fifo[i].data[1],
																	jpeg_isp_scalar_data_ready_user_callback, 
																	jpeg_isp_scalar_frame_free_user_callback, 
																	&jpeg_isp_scalar_fifo[i]
																);		
	} while(0);
	
	
	return 0;
	
err_exit:
	jpeg_isp_scalar_mode_exit ();
	return -1;
}

// ��ģʽ��JPEGͼƬǶ�뵽UVC����
// jpeg_image_file_name_to_embed Ƕ�뵽UVC�����е�JPEG�ļ�ȫ·����
static int jpeg_image_mode_init (char *jpeg_image_file_name_to_embed)
{
	void *fp = NULL;
	int ret = -1;
	unsigned int size;
	// ��JPEG�ļ����������浽image_file_data
	do 
	{
		fp = XM_fopen (jpeg_image_file_name_to_embed, "rb");
		if(fp == NULL)
		{
			XM_printf ("jpeg_image_mode_init failed, can't open image(%s)\n", jpeg_image_file_name_to_embed);
			break;
		}
		
		size = XM_filesize (fp);
		if(size <= 16384 || size > MAX_JPEG_FRAME_SIZE)	// ����16k��Ϊ����ЧͼƬ����
		{
			XM_printf ("jpeg_image_mode_init failed, image (%s)'s size(%d) invalid\n", jpeg_image_file_name_to_embed, size);
			break;			
		}
		
		image_file_data = kernel_malloc (size);
		if(image_file_data == NULL)
		{
			XM_printf ("jpeg_image_mode_init failed, memory busy, size = %d\n", size);
			break;
		}
			
		// ��JPEG�ļ�������JPEG֡
		if(XM_fread (image_file_data, 1, size, fp) <= 16384)
		{
			XM_printf ("jpeg_image_mode_init failed, file io error\n");
			
			kernel_free (image_file_data);
			image_file_data = NULL;
			break;			
		}
		// ����Ϊ32�ֽ�(cache line aligned)
		image_file_size = (size + 0x1F) & (~0x1F);	
		ret = 0;
	} while(0);
	
	if(fp)
		XM_fclose (fp);
		
	return ret; 
}

static int jpeg_image_mode_exit (void)
{
	if(image_file_data)
	{
		kernel_free (image_file_data);
		image_file_data = NULL;
	}
	image_file_size = 0;
	return 0;
}

static int jpeg_dummy_mode_init (void)
{
	return 0;
}

static int jpeg_dummy_mode_exit (void)
{
	return 0;
}

// mode						UVC������Դ
// mode_private_data		��UVCԴ������ص�˽������ 
int XMSYS_JpegSetupMode (unsigned int mode, void *mode_private_data)
{
	int ret = 0;
	//if(mode != XMSYS_JPEG_MODE_ISP_SCALAR && mode != XMSYS_JPEG_MODE_ISP)
	//{
	//	XM_printf ("XMSYS_JpegSetupMode failed, illegal mode (%d)\n", mode);
	//	return -1;
	//}
	
	OS_Use(&JpegSema);
	
	printf ("Close Old Mode (%d)\n", jpeg_mode);
	
	// ����Ƿ���Ҫ�ͷ���Դ
	//if(jpeg_mode != mode)
	{
		if(jpeg_mode == XMSYS_JPEG_MODE_ISP_SCALAR)
		{
			// ��Ҫ�ͷŴ�����scalar����	
			jpeg_isp_scalar_mode_exit ();
		}
		else if(jpeg_mode == XMSYS_JPEG_MODE_IMAGE)
		{
			jpeg_image_mode_exit ();
		}
		else if(jpeg_mode == XMSYS_JPEG_MODE_DUMMY)
		{
			jpeg_dummy_mode_exit ();
		}
	}
	jpeg_mode = mode;
	if(jpeg_mode == XMSYS_JPEG_MODE_ISP_SCALAR)
	{
		;//ret = jpeg_isp_scalar_mode_init ();
	}
	else if(jpeg_mode == XMSYS_JPEG_MODE_IMAGE)
	{
		if(mode_private_data)
			ret = jpeg_image_mode_init ((char *)mode_private_data);
		else
			ret = jpeg_dummy_mode_init ();
	}
	else if(jpeg_mode == XMSYS_JPEG_MODE_DUMMY)
	{
		jpeg_dummy_mode_init ();
	}
	
	printf ("open new mode(%d), ret=%d\n", jpeg_mode, ret);
	
	OS_Unuse(&JpegSema);
	return ret;
}

int arkn141_jpeg_get_image_info (
	char *jpeg_stream,			// JPEG��������ָ��
	int   jpeg_length,			// JPEG���ֽڳ���
	int  *img_width,				// �����ͼƬ���
	int  *img_height,				// �����ͼƬ�߶�
	int  *img_display_width,	// ͼƬ��ʵ����ʾ���
	int  *img_display_height	// ͼƬ��ʵ����ʾ�߶�
);

int arkn141_jpeg_decode (	
	char *jpeg_stream,			// JPEG��������ָ��
	int   jpeg_length,			// JPEG���ֽڳ���
	char*	image_y,
	char* image_cbcr
);

// ��֧��NV12��ʽ�������
int xm_jpeg_decode (const char *jpeg_data, size_t jpeg_size, 
						unsigned char *yuv,		// Y_UV420�����ַ(NV12��ʽ)
						unsigned int width,		// ������������ؿ��
						unsigned int height		// ������������ظ߶�
						)
{
	int w, h;
	int display_w, display_h;
	char *raw_yuv = NULL;
	int ret = -1;

	if(arkn141_codec_open (ARKN141_CODEC_TYPE_JPEG_DECODER) < 0)
	{
		printf ("xm_jpeg_decode failed, arkn141_codec_open NG\n");
		return ret;
	}
	
	do
	{
		if(arkn141_jpeg_get_image_info ((char *)jpeg_data, jpeg_size, &w, &h, &display_w, &display_h) < 0)
		{
			printf ("xm_jpeg_decode failed, arkn141_jpeg_get_image_info NG\n");
			break;
		}
		//printf ("w=%d, h=%d, lcd_w=%d, lwd_h=%d\n", w, h, width, height);
		raw_yuv = kernel_malloc (w * h * 3/2);
		if(raw_yuv == NULL)
		{
			printf ("xm_jpeg_decode failed, memory busy\n");
			break;
		}
		
		if(arkn141_jpeg_decode ((char *)jpeg_data, jpeg_size, raw_yuv, raw_yuv + w * h) < 0)
		{
			printf ("xm_jpeg_decode failed, arkn141_jpeg_decode NG\n");
			break;
		}
		
		// do scale
		if(w == 1920 && h == 1088)
		{
			if(arkn141_scalar_process ((unsigned char *)raw_yuv,
											1, 0,
											0, 0, display_w, display_h,
											w, h,
											width, height, width,
											FORMAT_Y_UV420,
											yuv,
											yuv + width * height,
											yuv + width * height,
											0, 0, 0) < 0)
				break;
		}
		else
		{
			if(arkn141_scalar_process ((unsigned char *)raw_yuv,
											0, 0,
											0, 0, w, h,
											w, h,
											width, height, width,
											FORMAT_Y_UV420,
											yuv,
											yuv + width * height,
											yuv + width * height,
											0, 0, 0) < 0)
				break;
		}

		ret = 0;		
	} while (0);
	
	if(raw_yuv)
		kernel_free (raw_yuv);
	
	arkn141_codec_close (ARKN141_CODEC_TYPE_JPEG_DECODER);
	
	return ret;
}

int xm_jpeg_file_decode (const char *jpeg_data, size_t jpeg_size, 
						unsigned char *yuv,		// Y_UV420�����ַ(NV12��ʽ)
						unsigned int width,		// ������������ؿ��
						unsigned int height		// ������������ظ߶�
						)
{
	int w, h;
	int display_w, display_h;
	char *raw_yuv = NULL;
	int ret = -1;

	if(arkn141_codec_open (ARKN141_CODEC_TYPE_JPEG_DECODER) < 0)
	{
		printf ("xm_jpeg_decode failed, arkn141_codec_open NG\n");
		return ret;
	}
	
	do
	{
		if(arkn141_jpeg_get_image_info ((char *)jpeg_data, jpeg_size, &w, &h, &display_w, &display_h) < 0)
		{
			printf ("xm_jpeg_decode failed, arkn141_jpeg_get_image_info NG\n");
			break;
		}
		//printf ("w=%d, h=%d, lcd_w=%d, lwd_h=%d\n", w, h, width, height);
		raw_yuv = kernel_malloc (w * h * 3/2);
		if(raw_yuv == NULL)
		{
			printf ("xm_jpeg_decode failed, memory busy\n");
			break;
		}
		
		if(arkn141_jpeg_decode ((char *)jpeg_data, jpeg_size, raw_yuv, raw_yuv + w * h) < 0)
		{
			printf ("xm_jpeg_decode failed, arkn141_jpeg_decode NG\n");
			break;
		}
		
		// do scale
		if(w == 1920 && h == 1088)
		{
			if(arkn141_scalar_process ((unsigned char *)raw_yuv,
											1, 0,
											0, (1080-480)/2, 1920, 480,	// ��?3?���?��?????����?
											//0, 0, display_w, display_h,
											w, h,
											width, height, width,
											FORMAT_Y_UV420,
											yuv,
											yuv + width * height,
											yuv + width * height,
											0, 0, 0) < 0)
				break;
		}
		else
		{
			if(arkn141_scalar_process ((unsigned char *)raw_yuv,
											0, 0,
											0, 200, 1280, 320,		// ��?3?���?��?????����?
											//0, 0, w, h,
											w, h,
											width, height, width,
											FORMAT_Y_UV420,
											yuv,
											yuv + width * height,
											yuv + width * height,
											0, 0, 0) < 0)
				break;
		}

		ret = 0;		
	} while (0);
	
	if(raw_yuv)
		kernel_free (raw_yuv);
	
	arkn141_codec_close (ARKN141_CODEC_TYPE_JPEG_DECODER);
	
	return ret;
}


int zoom_decode_jpeg_file (const char *file_name,unsigned char *zoombuffer,unsigned int w,unsigned int h)
{
	unsigned int size; 
	void *fp;
	char *jpg = NULL;
	int ret = -1;

	do
	{
		fp = XM_fopen (file_name, "rb");
		if(fp == NULL)
		{
			XM_printf ("AlbumView failed, open file(%s) NG\n", file_name);
			break;
		}
		size = XM_filesize (fp);
		if(size == 0 || size > 0x200000)
		{
			XM_printf ("AlbumView failed, file(%s)'s size (%d) illegal\n", file_name, size);
			break;
		}

		jpg = kernel_malloc (size);
		if(jpg == NULL)
		{
			XM_printf ("AlbumView failed, malloc (%d) NG\n", size);
			break;
		}

		if(XM_fread (jpg, 1, size, fp) != size)
		{
			XM_printf ("AlbumView failed, XM_fread (%d) NG\n", size);
			break;
		}

		XM_fclose (fp);
		fp = NULL;

		// ��JPEG�������뵽�´�����֡����
		ret = xm_jpeg_file_decode ((const char *)jpg, size, (unsigned char *)zoombuffer, w, h);
		kernel_free (jpg);
		jpg = NULL;
		break;
	} while (jpg);
}


int xm_uvc_set_window (unsigned int src_x, unsigned int src_y, unsigned int src_w, unsigned int src_h)
{
	int ret;
	src_x &= ~7;
	src_w &= ~7;
	src_y &= ~1;
	src_h &= ~1;
	if( (src_x + src_w) > isp_get_video_width() )
		return -1;
	if( (src_y + src_h) > isp_get_video_height() )
		return -1;
	
	OS_Use (&JpegSema);
	uvc_src_window_x = src_x;
	uvc_src_window_y = src_y;
	uvc_src_window_w = src_w;
	uvc_src_window_h = src_h;
	OS_Unuse (&JpegSema);
	return 0;
}


//////////////////////////////////////////////////////////////////////////////////
#include "arkn141_scale.h"
#include "Xm_osd_layer.h"

extern unsigned int get_yuv_format(void);

#if 0
static void  yuy2_to_nv12(unsigned char * pyuv422 ,unsigned char * pyuv420 ,unsigned int width ,unsigned int height)
{
	int i,j;
	unsigned char * pyuv420uv=pyuv420+width*height;
	unsigned char * pyuv422uv=pyuv422+width*height;

	
	memcpy(pyuv420,pyuv422,width*height);
	
	for(i=0,j=0; i<height; i=i+2,j++)
	{
		memcpy(pyuv420uv+j*width,pyuv422uv+i*width,width);
	}	
}


//Support YUY2 && NV12 format
int xm_uvc_jpeg_decode (const char *jpeg_data,	size_t jpeg_size, 
								unsigned char *yuv_422, 		// Y_UV422δ���������ַ(YUY2��ʽ)
								unsigned char *yuv_420/*,			// Y_UV420δ���������ַ(NV12��ʽ)
								unsigned char *yuv_scale		// Y_UV420���ź������ַ(NV12��ʽ)*/
							  )
{
	int w, h;
	int display_w, display_h;
//	char *raw_yuv = NULL;
	int ret = -1;
	
	unsigned int yuv_format;
	//unsigned int width = XM_lcdc_osd_get_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	//unsigned int height = XM_lcdc_osd_get_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);

	if(arkn141_codec_open (ARKN141_CODEC_TYPE_JPEG_DECODER) < 0)
	{
		JPEG_FATAL ("xm_jpeg_decode failed, arkn141_codec_open NG\n");
		return ret;
	}
	
	do
	{
		if(arkn141_jpeg_get_image_info ((char *)jpeg_data, jpeg_size, &w, &h, &display_w, &display_h) < 0)
		{
			JPEG_FATAL ("xm_jpeg_decode failed, arkn141_jpeg_get_image_info NG\n");
			break;
		}
		//JPEG_FATAL ("w=%d, h=%d, lcd_w=%d, lwd_h=%d\n", w, h, width, height);
		
		yuv_format = get_yuv_format();
		
		if(yuv_format == FORMAT_Y_UV422)
		{
			//���뻺��			
//			raw_yuv = kernel_malloc (w * h * 2);
//			if(raw_yuv == NULL)
//			{
//				JPEG_FATAL ("xm_jpeg_decode failed, raw_yuv memory busy\n");
//				break;
//			}
						
//			if(arkn141_jpeg_decode ((char *)jpeg_data, jpeg_size, raw_yuv, raw_yuv + w * h) < 0)
			if(arkn141_jpeg_decode ((char *)jpeg_data, jpeg_size, (char *)yuv_422, (char *)yuv_422 + w * h) < 0)
			{
				JPEG_FATAL ("xm_jpeg_decode failed, arkn141_jpeg_decode NG\n");
				break;
			}

			
//			yuy2_to_nv12((unsigned char *)raw_yuv, yuv_420, w, h);
			yuy2_to_nv12((unsigned char *)yuv_422, yuv_420, w, h);

			#if 0
			if(yuv_scale == NULL)
			{
				ret = 0;
				break;
			}

			// do scale
			if(w == 1920 && h == 1088)
			{
				if(arkn141_scalar_process ((unsigned char *)yuv_420,
												1, 0,
												0, 0, display_w, display_h,
												w, h,
												width, height, width,
												FORMAT_Y_UV420,
												yuv_scale,
												yuv_scale + width * height,
												yuv_scale + width * height,
												0, 0, 0) < 0)
					break;
			}
			else
			{
				if(arkn141_scalar_process ((unsigned char *)yuv_420,
												0, 0,
												0, 0, w, h,
												w, h,
												width, height, width,
												FORMAT_Y_UV420,
												yuv_scale,
												yuv_scale + width * height,
												yuv_scale + width * height,
												0, 0, 0) < 0)
					break;
			}
			#endif
		}
		else //default FORMAT_Y_UV420
		{
			if(arkn141_jpeg_decode ((char *)jpeg_data, jpeg_size, (char *)yuv_420, (char *)yuv_420 + w * h) < 0)
			{
				JPEG_FATAL ("xm_jpeg_decode failed, arkn141_jpeg_decode NG\n");
				break;
			}
			#if 0
			if(yuv_scale == NULL)
			{
				ret = 0;
				break;
			}
			
			// do scale
			if(w == 1920 && h == 1088)
			{
				if(arkn141_scalar_process ((unsigned char *)yuv_420,
												1, 0,
												0, 0, display_w, display_h,
												w, h,
												width, height, width,
												FORMAT_Y_UV420,
												yuv_scale,
												yuv_scale + width * height,
												yuv_scale + width * height,
												0, 0, 0) < 0)
					break;
			}
			else
			{
				if(arkn141_scalar_process ((unsigned char *)yuv_420,
												0, 0,
												0, 0, w, h,
												w, h,
												width, height, width,
												FORMAT_Y_UV420,
												yuv_scale,
												yuv_scale + width * height,
												yuv_scale + width * height,
												0, 0, 0) < 0)
					break;
			}
			#endif
		}
		
		ret = 0;		
	} while (0);
	
//	if(raw_yuv)
//		kernel_free (raw_yuv);
	
	arkn141_codec_close (ARKN141_CODEC_TYPE_JPEG_DECODER);
	
	return ret;
}
#endif
static void  yuy2_to_nv12_del_h(unsigned char * pyuv420 ,unsigned int width ,unsigned int height, unsigned int del_h)
{
	int i,j;
	unsigned char *uv_source = pyuv420+width*height;
	unsigned char *uv_dest = pyuv420+width*(height-del_h);

	//copy UV
	for(i=0,j=0; i<(height-del_h); i=i+2,j++)
	{
		memcpy(uv_dest+j*width, uv_source+i*width, width);
	}	
}
//yuv422����ֱ�ӽ�ѹ��yuv420 buffer�У�yuy2תnv12ʱֻ����uv���ݣ���ʡ2/3 ����ʱ��
int xm_uvc_jpeg_decode (const char *jpeg_data,	size_t jpeg_size, 
								//unsigned char *yuv_422, 		// Y_UV422δ���������ַ(YUY2��ʽ)
								unsigned char *yuv_420			// Y_UV420δ���������ַ(NV12��ʽ)
								//unsigned char *yuv_scale		// Y_UV420���ź������ַ(NV12��ʽ)
							  )
{
	int w, h;
	int display_w, display_h;
	int ret = -1;
	
	unsigned int yuv_format;

	if((jpeg_data == NULL) || (jpeg_size == 0))
	{
		printf ("jpeg data error.\n");
		return -1;
	}

	if(arkn141_codec_open (ARKN141_CODEC_TYPE_JPEG_DECODER) < 0)
	{
		JPEG_FATAL ("xm_jpeg_decode failed, arkn141_codec_open NG\n");
		return ret;
	}
	
	do
	{
		if(arkn141_jpeg_get_image_info ((char *)jpeg_data, jpeg_size, &w, &h, &display_w, &display_h) < 0)
		{
			JPEG_FATAL ("xm_jpeg_decode failed, arkn141_jpeg_get_image_info NG\n");
			break;
		}
		
		yuv_format = get_yuv_format();
		
		if(yuv_format == FORMAT_Y_UV422)
		{

			if(yuv_420 == NULL)
			{
				ret = 0;
				break;
			}

			if(arkn141_jpeg_decode ((char *)jpeg_data, jpeg_size, (char *)yuv_420, (char *)yuv_420 + w * h) < 0)
			{
				printf("11111111111111 jpeg_data: %x jpeg_size: %x \n",jpeg_data,jpeg_size);
				JPEG_FATAL ("1111 xm_jpeg_decode failed, arkn141_jpeg_decode NG\n");
				break;
			}

						
			//yuy2_to_nv12((unsigned char *)raw_yuv, yuv_420, w, h);
			if(h == 1088)
			{
				//h264���벻�ܱ���1920x1088��ֻ�ܱ���1920x1080������ɾ��8 ��(����ɾ�����8��)
				yuy2_to_nv12_del_h(yuv_420, w, h, 8);
				h = 1080;
			}
			else
			{
				yuy2_to_nv12_del_h(yuv_420, w, h, 0);
			}

			#if 0
			if(yuv_scale == NULL)
			{
				ret = 0;
				break;
			}

			// do scale
			if(w == 1920 && h == 1088)
			{
				if(arkn141_scalar_process ((unsigned char *)yuv_420,
												1, 0,
												0, 0, display_w, display_h,
												w, h,
												width, height, width,
												FORMAT_Y_UV420,
												yuv_scale,
												yuv_scale + width * height,
												yuv_scale + width * height,
												0, 0, 0) < 0)
					break;
			}
			else
			{
				if(arkn141_scalar_process ((unsigned char *)yuv_420,
												0, 0,
												0, 0, w, h,
												w, h,
												width, height, width,
												FORMAT_Y_UV420,
												yuv_scale,
												yuv_scale + width * height,
												yuv_scale + width * height,
												0, 0, 0) < 0)
					break;
			}
			#endif
		}
		else //default FORMAT_Y_UV420
		{
			if(arkn141_jpeg_decode ((char *)jpeg_data, jpeg_size, (char *)yuv_420, (char *)yuv_420 + w * h) < 0)
			{
				JPEG_FATAL ("xm_jpeg_decode failed, arkn141_jpeg_decode NG\n");
				break;
			}
			
			if(h == 1088)
			{
				//h264���벻�ܱ���1920x1088��ֻ�ܱ���1920x1080������ɾ��8 ��(����ɾ�����8��)
				memcpy(yuv_420+w*(h-8), yuv_420+w*h, w*(h-8)/2);
				h = 1080;
			}

			#if 0
			if(yuv_scale == NULL)
			{
				ret = 0;
				break;
			}
			
			// do scale
			if(w == 1920 && h == 1088)
			{
				if(arkn141_scalar_process ((unsigned char *)yuv_420,
												1, 0,
												0, 0, display_w, display_h,
												w, h,
												width, height, width,
												FORMAT_Y_UV420,
												yuv_scale,
												yuv_scale + width * height,
												yuv_scale + width * height,
												0, 0, 0) < 0)
					break;
			}
			else
			{
				if(arkn141_scalar_process ((unsigned char *)yuv_420,
												0, 0,
												0, 0, w, h,
												w, h,
												width, height, width,
												FORMAT_Y_UV420,
												yuv_scale,
												yuv_scale + width * height,
												yuv_scale + width * height,
												0, 0, 0) < 0)
					break;
			}
			#endif
		}
		
		ret = 0;		
	} while (0);
	
	arkn141_codec_close (ARKN141_CODEC_TYPE_JPEG_DECODER);
	
	return ret;
}




