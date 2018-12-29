#ifndef _ARK1960_TESTCASE_H_
#define _ARK1960_TESTCASE_H_

#if defined (__cplusplus)
	extern "C"{
#endif
		
// SPI FLASH����д���������ԣ������ռ���ԣ�		
extern int spi_test (void);

// SPI FLASH����д��������DMA����DMAд��INTERRUPT����
extern void TestSSI(void);

// ********************************************************************
//
//        SD��������֤ģ��
//
// ********************************************************************
// 1) ��֤SD����DMA����DMAд����
// 2) �����ļ�ϵͳ���ļ�д����֤SD��(����ʽ�����ļ�������д�롢�����Ƚ�)
// 3) SD�࿨�ļ�ϵͳ����(��δ����)

// ����  0  ���Թ��̳ɹ�
// ���� -1  ���Թ���ʧ��
// sd_dev_index ��ʾ����SD�����豸���(��0��ʼ����ǰ֧������SD�豸0��1)
int fs_sd_function_test (unsigned int sd_dev_index);

// SD�����ܲ���
// 1������д����ģ���ļ��滻��ɾ�����̣����Կ�д���ƽ���ٶ�
int fs_sd_performance_test (int sd_dev_index);

// NFC ���ܲ���ģ��
// ����  0  ���Թ��̳ɹ�
// ���� -1  ���Թ���ʧ��
// dev_index ��ʾ�����豸���(��0��ʼ����ǰ֧��1��NFC�豸)
int fs_nf_function_test (unsigned int dev_index);

// NFC���ܲ���
// 1������д����ģ���ļ��滻��ɾ�����̣����Կ�д���ƽ���ٶ�
int fs_nf_performance_test (int dev_index);


// ң��������
int remote_test (void);

// DMA����

// 0  DMA test OK
// -1 DMA test NG
int dma_copy_test (void);

// DMA chain test
void dma_copy_test_chain (unsigned int dmachannel );

// ʹ������ִ��DMA���Ʋ���
void dma_endless_test (void);


// USB Host���ԣ���֤Host
int usb_host_test (void);

int fs_usb_function_test (unsigned int usb_dev_index);

//       �ļ�ϵͳ�豸�����쳣������֤ģ��
// device_name  ���Ե��豸���ƣ���mmc, nf, usb
// device_index �豸�����ţ���0��ʼ
// fs_test_file_count ����д���ļ�����
// fs_test_file_size ����д���ļ��ֽڴ�С(512����)
// fs_test_loop_times ����ѭ������
// ����  0  ���Թ��̳ɹ�
// ���� -1  ���Թ���ʧ��
int fs_device_exception_test (const char *device_name, unsigned int device_index,
										int fs_test_file_count, 
										int fs_test_file_size,
										int fs_test_loop_times
											);

// �ļ�ϵͳCACHE�����ܲ���(��д����λ���Ƚ�)
int fs_device_cache_file_test (const char *device_name, unsigned int device_index);

// �ļ�ϵͳCache�������ܲ���
int fs_device_cache_file_performance_test (const char *device_name, unsigned int device_index);


// �ļ�ϵͳ������
int fs_device_scan_read_test (const char *device_name, unsigned int device_index);

// LCDC OSD ����
void lcdc_osd_test (void);


// UART ��֤ģ��
int uart_test (void);
int uart1_test (void);

// deinterlace ��֤ģ��
// ֧��YUV420��YUV422
// �������ݱ�����SD���ϵĸ�Ŀ¼�£�
// ��Ŀ¼�ṹ����
// \\YUV420
// \\YUV420\\WWWXHHHR\\, �� \\YUV420\\720X480R\\
// \\YUV420\\WWWXHHHD\\, �� \\YUV420\\720X480D\\
//	
// \\YUV422
// \\YUV422\\WWWXHHHR\\, �� \\YUV422\\720X480R\\
// \\YUV422\\WWWXHHHD\\, �� \\YUV422\\720X480D\\
// 
// WWW(���), ֧��720, 960
// HHH(�߶�), ֧��480, 576	
// R ��ʾ����Դ���ݣ�Ŀ¼�µ�YUVԴ�ļ���ż���泡�����ţ�ÿ���ļ�����һ����������
// D ��ʾ����3D DEINTERLACE�����֡Ŀ�������ļ���ÿ���ļ���һ������ż���泡������֡
//
// YUV420֧��720X480, 720X576, 960X480, 960X576
// YUV422֧��720X480, 720X576, 960X480, 960X576
//
int deinterlace_test (const char *dev);

// L2CC ����ģ��
void l2cc_test (void);

// 8bit/12bit ARGB888ת����YUV444����
// ɨ�� \\RGB2YUV\\RGB\\Ŀ¼�µ���Ŀ¼��Ŀ��YUV���ݱ�����\\RGB2YUV\\YUV\\Ŀ¼�µ���Ŀ¼
// ��Ŀ¼ʹ��wh������w,h��ʹ��4λ10��������
//	��1920x1280, Ŀ¼��Ϊ19201280
// �� 640x 480, Ŀ¼��Ϊ06400480
// 12_00001.rgb ��ʾ12bit��ARGB���������ļ�
// 08_00001.rgb ��ʾ8bit��ARGB���������ļ�
int ark1960_rgb2yuv_test (char *volume_name);

// YUV420ת����8bit ARGB888����(��֧��8bit)
// ɨ�� \\YUV2RGB\\YUV\\Ŀ¼�µ���Ŀ¼��Ŀ��RGB���ݱ�����\\YUV2RGB\\RGB\\Ŀ¼�µ���Ŀ¼
int ark1960_yuv2rgb_test (char *volume_name);

// ***************************************************************************
//				YUV���Ų���
//
// ɨ�� \\SCALE\\YUV\\Ŀ¼�µ���Ŀ¼(��Ŀ¼ʹ��wh������w,h��ʹ��4λ10��������)��
// ������Ŀ¼�µ�YUV420�ļ����� \\SCALE\\YUV\\RAW\\19201280\\1.YUV
// ��ÿ���ļ��ֱ�ִ��1�������Ŵ�2��������С����,
//    ���ɵ��ļ����浽��Ӧ����Ŀ¼��,��ʽ����ΪYUV420��ʽ
//		�����Ŵ�·��  \\SCALE\\YUV\\NEW\\19201280\\1\\INC\\19201312.YUV
// 	������С·��  \\SCALE\\YUV\\NEW\\19201280\\1\\DEC\\19201248.YUV
//
// ��Ŀ¼ʹ��wh������w, h��ʹ��4λ10��������
//	��1920x1280, Ŀ¼��Ϊ19201280
// �� 640x 480, Ŀ¼��Ϊ06400480
// ��߱���Ϊ16�ı���
//
// ***************************************************************************
int ark1960_yuv_scale_test (char *volume_name);


// ***************************************************************************
//				RGB���Ų���
//
// ɨ�� \\SCALE\\RGB\\Ŀ¼�µ���Ŀ¼(��Ŀ¼ʹ��wh������w,h��ʹ��4λ10��������)��
// ������Ŀ¼�µ�RGB�ļ����� \\SCALE\\RGB\\RAW\\19201280\\1.RGB
// ��ÿ���ļ��ֱ�ִ��1�������Ŵ�2��������С����,
//    ���ɵ��ļ����浽��Ӧ����Ŀ¼��,��ʽ����Ϊ32λBITMAP�ļ���ʽ
//		�����Ŵ�·��  \\SCALE\\RGB\\NEW\\19201280\\1\\INC\\19201312.BMP
// 	������С·��  \\SCALE\\RGB\\NEW\\19201280\\1\\DEC\\19201248.BMP
//
// ��Ŀ¼ʹ��wh������w, h��ʹ��4λ10��������
//	��1920x1280, Ŀ¼��Ϊ19201280
// �� 640x 480, Ŀ¼��Ϊ06400480
// ��߱���Ϊ16�ı���
//
// ***************************************************************************
int ark1960_rgb_scale_test (char *volume_name);

// ARGB888 Alpha-Blending����
int ark1960_alpha_blending_test (char *volume_name);


// ***************************************************************************
//				YUV420�˲�����
//
//
// ***************************************************************************
int ark1960_min_max_mid_filter_test (char *volume_name);


// YUV420ת��
// YUV420 --> yuy2
// YUV420 --> uyvy
int ark1960_yuv2yuy2_yuv2uyvy_test (char *volume_name);

int ark1960_move_image_test (char *volume_name);

// ͼ�������Ȳ���
int ark1960_af_dct_test (char *volume_name);

int convert_yuv420_to_argb888_save_into_bitmap_file (const char *yuv_file, 
																	  int w,
																	  int h,
																	  int stride,
																	  char *yuv_buffer
																	  );

int ark1960_rgb2yuv_minitest (char *volume_name);

int ark1960_yuv_scale_minitest (char *volume_name);
// YUV420 --> JPG ת������(��С������)
int ark1960_yuv2jpg_minitest (char *volume_name);

int ark1960_alpha_blending_minitest (char *volume_name);
int ark1960_min_max_mid_filter_minitest (char *volume_name);

// ��ֵ����С������
int ark1960_binary_minitest (char *volume_name);

// YUV420 --> ARGB888 ��С������
int ark1960_yuv2rgb_minitest (char *volume_name);

// YUV420 --> YUY2
// YUV420 --> UYVY ��С������(���ٲ���)
int ark1960_yuv2yuy2_yuv2uyvy_minitest (char *volume_name);

// MOVE��С������ (MOVE������DMA��һ��)
int ark1960_move_image_minitest (char *volume_name);

// ZOOM IN��С������
int ark1960_zoom_in_minitest (char *volume_name);

// ZOOM IN����
int ark1960_zoom_in_test (char *volume_name);

// ARGB888 -- > RGB565   ARGB888 -- > RGB454 ��С������
int ark1960_argb888_to_rgb565_rgb454_minitest (char *volume_name);

// YUV420 --> h264 ת������(��С������)
int ark1960_yuv2h264_minitest (char *volume_name);


int check_and_create_directory (char *path);

int on2_codec_test (void);

// ethernet mac ����
int ethernet_mac_test (void);

//void testDMA( );

// ISP+Scalar����
void isp_scalar_test (void);

// ISP+Scalar����
int xm_isp_scalar_test (void);


// H264 + JPEG ��������
int arkn141_h264_jpeg_test (void);

// ISP autotest
void isp_autotest (void );


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif

