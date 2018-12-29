#ifndef _ARK1960_TESTCASE_H_
#define _ARK1960_TESTCASE_H_

#if defined (__cplusplus)
	extern "C"{
#endif
		
// SPI FLASH读、写、擦除测试（完整空间测试）		
extern int spi_test (void);

// SPI FLASH读、写、擦除、DMA读、DMA写、INTERRUPT测试
extern void TestSSI(void);

// ********************************************************************
//
//        SD卡功能验证模块
//
// ********************************************************************
// 1) 验证SD卡的DMA读、DMA写过程
// 2) 基于文件系统的文件写入验证SD卡(卡格式化、文件创建、写入、读出比较)
// 3) SD多卡文件系统测试(尚未测试)

// 返回  0  测试过程成功
// 返回 -1  测试过程失败
// sd_dev_index 表示测试SD卡卡设备序号(从0开始，当前支持两个SD设备0及1)
int fs_sd_function_test (unsigned int sd_dev_index);

// SD卡性能测试
// 1）将卡写满后，模拟文件替换、删除过程，测试卡写入的平均速度
int fs_sd_performance_test (int sd_dev_index);

// NFC 功能测试模块
// 返回  0  测试过程成功
// 返回 -1  测试过程失败
// dev_index 表示测试设备序号(从0开始，当前支持1个NFC设备)
int fs_nf_function_test (unsigned int dev_index);

// NFC性能测试
// 1）将卡写满后，模拟文件替换、删除过程，测试卡写入的平均速度
int fs_nf_performance_test (int dev_index);


// 遥控器测试
int remote_test (void);

// DMA测试

// 0  DMA test OK
// -1 DMA test NG
int dma_copy_test (void);

// DMA chain test
void dma_copy_test_chain (unsigned int dmachannel );

// 使用任务执行DMA复制测试
void dma_endless_test (void);


// USB Host测试，验证Host
int usb_host_test (void);

int fs_usb_function_test (unsigned int usb_dev_index);

//       文件系统设备驱动异常处理验证模块
// device_name  测试的设备名称，如mmc, nf, usb
// device_index 设备索引号，从0开始
// fs_test_file_count 测试写入文件个数
// fs_test_file_size 测试写入文件字节大小(512倍数)
// fs_test_loop_times 测试循环次数
// 返回  0  测试过程成功
// 返回 -1  测试过程失败
int fs_device_exception_test (const char *device_name, unsigned int device_index,
										int fs_test_file_count, 
										int fs_test_file_size,
										int fs_test_loop_times
											);

// 文件系统CACHE服务功能测试(读写、定位、比较)
int fs_device_cache_file_test (const char *device_name, unsigned int device_index);

// 文件系统Cache服务性能测试
int fs_device_cache_file_performance_test (const char *device_name, unsigned int device_index);


// 文件系统读测试
int fs_device_scan_read_test (const char *device_name, unsigned int device_index);

// LCDC OSD 测试
void lcdc_osd_test (void);


// UART 验证模块
int uart_test (void);
int uart1_test (void);

// deinterlace 验证模块
// 支持YUV420及YUV422
// 测试数据保存在SD卡上的根目录下，
// 其目录结构如下
// \\YUV420
// \\YUV420\\WWWXHHHR\\, 如 \\YUV420\\720X480R\\
// \\YUV420\\WWWXHHHD\\, 如 \\YUV420\\720X480D\\
//	
// \\YUV422
// \\YUV422\\WWWXHHHR\\, 如 \\YUV422\\720X480R\\
// \\YUV422\\WWWXHHHD\\, 如 \\YUV422\\720X480D\\
// 
// WWW(宽度), 支持720, 960
// HHH(高度), 支持480, 576	
// R 表示单场源数据，目录下的YUV源文件按偶、奇场交错存放，每个文件均是一个单场数据
// D 表示经过3D DEINTERLACE处理后帧目标数据文件，每个文件是一个包含偶、奇场的完整帧
//
// YUV420支持720X480, 720X576, 960X480, 960X576
// YUV422支持720X480, 720X576, 960X480, 960X576
//
int deinterlace_test (const char *dev);

// L2CC 测试模块
void l2cc_test (void);

// 8bit/12bit ARGB888转换到YUV444测试
// 扫描 \\RGB2YUV\\RGB\\目录下的子目录，目标YUV数据保存在\\RGB2YUV\\YUV\\目录下的子目录
// 子目录使用wh命名，w,h均使用4位10进制命名
//	如1920x1280, 目录名为19201280
// 如 640x 480, 目录名为06400480
// 12_00001.rgb 表示12bit的ARGB测试数据文件
// 08_00001.rgb 表示8bit的ARGB测试数据文件
int ark1960_rgb2yuv_test (char *volume_name);

// YUV420转换到8bit ARGB888测试(仅支持8bit)
// 扫描 \\YUV2RGB\\YUV\\目录下的子目录，目标RGB数据保存在\\YUV2RGB\\RGB\\目录下的子目录
int ark1960_yuv2rgb_test (char *volume_name);

// ***************************************************************************
//				YUV缩放测试
//
// 扫描 \\SCALE\\YUV\\目录下的子目录(子目录使用wh命名，w,h均使用4位10进制命名)，
// 遍历子目录下的YUV420文件，如 \\SCALE\\YUV\\RAW\\19201280\\1.YUV
// 对每个文件分别执行1）增量放大及2）缩量缩小操作,
//    生成的文件保存到相应的子目录下,格式保存为YUV420格式
//		增量放大路径  \\SCALE\\YUV\\NEW\\19201280\\1\\INC\\19201312.YUV
// 	缩量减小路径  \\SCALE\\YUV\\NEW\\19201280\\1\\DEC\\19201248.YUV
//
// 子目录使用wh命名，w, h均使用4位10进制命名
//	如1920x1280, 目录名为19201280
// 如 640x 480, 目录名为06400480
// 宽高必须为16的倍数
//
// ***************************************************************************
int ark1960_yuv_scale_test (char *volume_name);


// ***************************************************************************
//				RGB缩放测试
//
// 扫描 \\SCALE\\RGB\\目录下的子目录(子目录使用wh命名，w,h均使用4位10进制命名)，
// 遍历子目录下的RGB文件，如 \\SCALE\\RGB\\RAW\\19201280\\1.RGB
// 对每个文件分别执行1）增量放大及2）缩量缩小操作,
//    生成的文件保存到相应的子目录下,格式保存为32位BITMAP文件格式
//		增量放大路径  \\SCALE\\RGB\\NEW\\19201280\\1\\INC\\19201312.BMP
// 	缩量减小路径  \\SCALE\\RGB\\NEW\\19201280\\1\\DEC\\19201248.BMP
//
// 子目录使用wh命名，w, h均使用4位10进制命名
//	如1920x1280, 目录名为19201280
// 如 640x 480, 目录名为06400480
// 宽高必须为16的倍数
//
// ***************************************************************************
int ark1960_rgb_scale_test (char *volume_name);

// ARGB888 Alpha-Blending测试
int ark1960_alpha_blending_test (char *volume_name);


// ***************************************************************************
//				YUV420滤波测试
//
//
// ***************************************************************************
int ark1960_min_max_mid_filter_test (char *volume_name);


// YUV420转换
// YUV420 --> yuy2
// YUV420 --> uyvy
int ark1960_yuv2yuy2_yuv2uyvy_test (char *volume_name);

int ark1960_move_image_test (char *volume_name);

// 图像清晰度测试
int ark1960_af_dct_test (char *volume_name);

int convert_yuv420_to_argb888_save_into_bitmap_file (const char *yuv_file, 
																	  int w,
																	  int h,
																	  int stride,
																	  char *yuv_buffer
																	  );

int ark1960_rgb2yuv_minitest (char *volume_name);

int ark1960_yuv_scale_minitest (char *volume_name);
// YUV420 --> JPG 转换测试(最小集测试)
int ark1960_yuv2jpg_minitest (char *volume_name);

int ark1960_alpha_blending_minitest (char *volume_name);
int ark1960_min_max_mid_filter_minitest (char *volume_name);

// 二值化最小集测试
int ark1960_binary_minitest (char *volume_name);

// YUV420 --> ARGB888 最小集测试
int ark1960_yuv2rgb_minitest (char *volume_name);

// YUV420 --> YUY2
// YUV420 --> UYVY 最小集测试(快速测试)
int ark1960_yuv2yuy2_yuv2uyvy_minitest (char *volume_name);

// MOVE最小集测试 (MOVE操作比DMA快一倍)
int ark1960_move_image_minitest (char *volume_name);

// ZOOM IN最小集测试
int ark1960_zoom_in_minitest (char *volume_name);

// ZOOM IN测试
int ark1960_zoom_in_test (char *volume_name);

// ARGB888 -- > RGB565   ARGB888 -- > RGB454 最小集测试
int ark1960_argb888_to_rgb565_rgb454_minitest (char *volume_name);

// YUV420 --> h264 转换测试(最小集测试)
int ark1960_yuv2h264_minitest (char *volume_name);


int check_and_create_directory (char *path);

int on2_codec_test (void);

// ethernet mac 测试
int ethernet_mac_test (void);

//void testDMA( );

// ISP+Scalar测试
void isp_scalar_test (void);

// ISP+Scalar测试
int xm_isp_scalar_test (void);


// H264 + JPEG 编解码测试
int arkn141_h264_jpeg_test (void);

// ISP autotest
void isp_autotest (void );


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif

