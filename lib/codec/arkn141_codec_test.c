//****************************************************************************
//
//	Copyright (C) 2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: arkn141_codec.c
//
//	Revision history
//
//		2014.12.21	ZhuoYongHong Initial version
//
//		H264 encoder/decoder 及 JPEG encoder/decoder 的硬件资源存在共享，
//		执行过程中需要保护
//
//****************************************************************************
#include <stdio.h>
#include "ark1960.h"
#include "rtos.h"
#include "FS.h"
#include "arkn141_codec.h"

#ifdef DEBUG
#define	DEBUG_PRINT(fmt, args...)		printf(fmt, ## args)	
#else
#define	DEBUG_PRINT(fmt, args...)	
#endif

extern int arkn141_yuv2jpg_test_ex (char *volume_name, unsigned int repeat_test_count, int yuv_format, int slice_height);
extern int arkn141_jpg2yuv_test (char *volume_name, unsigned int repeat_test_count);
extern int on2_decode_test (void);
extern int jpeg_encode_main(int argc, char *argv[]);
extern int h264_decode (const char *h264_file_name, const char *yuv_file_name);
extern int jpeg_decode_main(int argc, char *argv[]);
extern void SetJpegQLevel(char level );



#define	ENC_FRAME_COUNT					"100"
#define	H264_ENC_MAX_QP					"36"							// H264最大QP值
//#define	H264_ENC_MAD_QP_ADJUSTMENT		"--mbQpAdjustment=-3"	//"--mbQpAdjustment=0"		// 	
#define	H264_ENC_MAD_QP_ADJUSTMENT		"--mbQpAdjustment=-0"	//"--mbQpAdjustment=0"		// 	

														// MAD based MB QP adjustment	
														// Valid value range: [-8, 7]
														// 0 = disabled	
														// Enables MAD thresholding for macroblock based QP adjustment. 
														// Use this value for adjusting QP of all macroblocks under 
														//	threshold. Adaptive threshold will select macroblocks with 
														//	least detail and decreasing their QP can reduce coding 
														//	artifacts and improve subjective visual quality. 
														//	Enabling this will disable MB RC.	
																					

#define	H264_PIXELMV	"0"					// Adaptive setting disables 1/4p MVs for > 720p.\n"
											//	0=OFF, 1=Adaptive, 2=ON [1]	


#define	ON2_H264_ENC_VIEW_MODE_DOUBLE						0
#define	ON2_H264_ENC_VIEW_MODE_SINGLE						1
#define	ON2_H264_ENC_VIEW_MODE ON2_H264_ENC_VIEW_MODE_SINGLE

#if ON2_H264_ENC_VIEW_MODE == ON2_H264_ENC_VIEW_MODE_DOUBLE
#define	H264_ENC_VIEW_MODE	"--viewMode=0"
#elif ON2_H264_ENC_VIEW_MODE == ON2_H264_ENC_VIEW_MODE_SINGLE
#define	H264_ENC_VIEW_MODE	"--viewMode=1"
#endif

#define	ON2_H264_ENC_INPUT_FORMAT_YUV420_planar		0
#define	ON2_H264_ENC_INPUT_FORMAT_YUV420_semiplanar	1
#define	ON2_H264_ENC_INPUT_FORMAT_YUV420					ON2_H264_ENC_INPUT_FORMAT_YUV420_semiplanar
//#define	ON2_H264_ENC_INPUT_FORMAT_YUV420					ON2_H264_ENC_INPUT_FORMAT_YUV420_planar

// H264编码输入源格式定义 0 - YUV420 planar 或者 1 - YUV420 semiplanar
#define	H264_ENC_INPUT_FORMAT_YUV420_planar				"--inputFormat=0"
#define	H264_ENC_INPUT_FORMAT_YUV420_semiplanar		"--inputFormat=1"

#if ON2_H264_ENC_INPUT_FORMAT_YUV420 == ON2_H264_ENC_INPUT_FORMAT_YUV420_planar
#define	H264_ENC_INPUT_FORMAT			H264_ENC_INPUT_FORMAT_YUV420_planar
#elif ON2_H264_ENC_INPUT_FORMAT_YUV420 == ON2_H264_ENC_INPUT_FORMAT_YUV420_semiplanar
#define	H264_ENC_INPUT_FORMAT			H264_ENC_INPUT_FORMAT_YUV420_semiplanar
#else
#error	please define ON2_H264_ENC_INPUT_FORMAT_YUV420
#endif

static  char *argv_1080P[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1088",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
	H264_ENC_INPUT_FORMAT,	// 输入格式选择 (YUV420 planar 或者YUV420 semiplanar)	
	"-b",
	ENC_FRAME_COUNT,	// 100帧
	"-i",
#if ON2_H264_ENC_INPUT_FORMAT_YUV420 == ON2_H264_ENC_INPUT_FORMAT_YUV420_planar
	"\\MINITEST\\RAW\\VIDEO\\YUV420\\19201088.YUV",
#elif ON2_H264_ENC_INPUT_FORMAT_YUV420 == ON2_H264_ENC_INPUT_FORMAT_YUV420_semiplanar
	"\\MINITEST\\RAW\\VIDEO\\Y_UV420\\19201088.YUV",
#endif
	"-q",
	"20",
	"-m",
	H264_ENC_MAX_QP,
	"-M",		// quarterPixelMv
	H264_PIXELMV,
	"-8",		// trans8x8
	"1",	
//	"-K",		//	"-enableCabac",
//	"1",
//	"-Z",		//	"-videoStab",
//	"1",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"1600000",
	"-o",
#if ON2_H264_ENC_INPUT_FORMAT_YUV420 == ON2_H264_ENC_INPUT_FORMAT_YUV420_planar
	"\\MINITEST\\ON2ENC\\YUV420\\1080.264",
#elif ON2_H264_ENC_INPUT_FORMAT_YUV420 == ON2_H264_ENC_INPUT_FORMAT_YUV420_semiplanar
	"\\MINITEST\\ON2ENC\\Y_UV420\\1080.264",
#endif

	H264_ENC_VIEW_MODE,
};

static  char *argv_1080P_slice[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1088",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
	H264_ENC_INPUT_FORMAT,	// 输入格式选择 (YUV420 planar 或者YUV420 semiplanar)	
	"-b",
	"20",	// 100帧
	"-i",
#if ON2_H264_ENC_INPUT_FORMAT_YUV420 == ON2_H264_ENC_INPUT_FORMAT_YUV420_planar
	"\\MINITEST\\RAW\\VIDEO\\YUV420\\19201088.YUV",
#elif ON2_H264_ENC_INPUT_FORMAT_YUV420 == ON2_H264_ENC_INPUT_FORMAT_YUV420_semiplanar
	"\\MINITEST\\RAW\\VIDEO\\Y_UV420\\19201088.YUV",
#endif
	"-q",
	"20",
	"-m",
	H264_ENC_MAX_QP,
	"-M",		// quarterPixelMv
	H264_PIXELMV,
	"-8",		// trans8x8
	"1",	
//	"-K",		//	"-enableCabac",
//	"1",
//	"-Z",		//	"-videoStab",
//	"1",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"1600000",
	"-o",
#if ON2_H264_ENC_INPUT_FORMAT_YUV420 == ON2_H264_ENC_INPUT_FORMAT_YUV420_planar
	"\\MINITEST\\ON2ENC\\YUV420\\1080_SL1.264",
#elif ON2_H264_ENC_INPUT_FORMAT_YUV420 == ON2_H264_ENC_INPUT_FORMAT_YUV420_semiplanar
	"\\MINITEST\\ON2ENC\\Y_UV420\\1080_SL1.264",
#endif

	H264_ENC_VIEW_MODE,
	
	// 设置slice大小(宏块行个数)
	"-V",
	"1",		
};

static  char *argv_720P[] = {
	"arkn141",
	"-w",
	"1280",
	"-h",
	"720",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
	"-b",
	ENC_FRAME_COUNT,	// 100帧
	H264_ENC_INPUT_FORMAT,	// 输入格式选择 (YUV420 planar 或者YUV420 semiplanar)	
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\Y_UV420\\12800720.YUV",
	"-q",
	"20",
	"-m",
	H264_ENC_MAX_QP,
	"-M",		// quarterPixelMv
	H264_PIXELMV,
	"-8",		// trans8x8
	"1",	
//	"-K",		//	"-enableCabac",
//	"1",
//	"-Z",		//	"-videoStab",
//	"1",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"800000",
	"-o",
	"\\MINITEST\\ON2ENC\\Y_UV420\\720P.264",
};

static  char *argv_1080P_Y_UV420[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1088",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
	"--inputFormat=1",	// 输入格式选择 (YUV420 semi-planar)	
	"-b",
	ENC_FRAME_COUNT,	// 100帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\Y_UV420\\19201088.YUV",
	"-q",
	"20",
	"-m",
	H264_ENC_MAX_QP,
	"-M",		// quarterPixelMv
	H264_PIXELMV,
	"-8",		// trans8x8
	"1",	
//	"-K",		//	"-enableCabac",
//	"1",
//	"-Z",		//	"-videoStab",
//	"1",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"1600000",
	"-o",
	"\\MINITEST\\ON2ENC\\Y_UV420\\1080.264",
	H264_ENC_VIEW_MODE,
};


static  char *argv_720P_YUV420[] = {
	"arkn141",
	"-w",
	"1280",
	"-h",
	"720",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
	"-b",
	ENC_FRAME_COUNT,	// 100帧
	H264_ENC_INPUT_FORMAT_YUV420_planar,	// 输入格式选择 (YUV420 planar 或者YUV420 semiplanar)	
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\YUV420\\12800720.YUV",
	"-q",
	"20",
	"-m",
	H264_ENC_MAX_QP,
	"-M",		// quarterPixelMv
	H264_PIXELMV,
	"-8",		// trans8x8
	"1",	
//	"-K",		//	"-enableCabac",
//	"1",
//	"-Z",		//	"-videoStab",
//	"1",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"800000",
	"-o",
	"\\MINITEST\\ON2ENC\\YUV420\\720P.264",
};

static  char *argv_VGA_Y_UV420[] = {
	"arkn141",
	"-w",
	"640",
	"-h",
	"480",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
	"-b",
	ENC_FRAME_COUNT,	// 100帧	
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\Y_UV420\\06400480.YUV",
   "--inputFormat=1",      // Y_UV420
	"-q",
	"20",
	"-m",
	H264_ENC_MAX_QP,	
	"-M",		// quarterPixelMv
	H264_PIXELMV,
	"-8",		// trans8x8
	"1",		
//	"-K",		//	"-enableCabac",
//	"1",
//	"-Z",		//	"-videoStab",
//	"1",
	"-D",		// disableDeblocking
	"0",
	"-o",
	"\\MINITEST\\ON2ENC\\Y_UV420\\06400480.264",
};

static  char *argv_CIF_Y_UV420[] = {
	"arkn141",
	"-b",
	"300",	// 300帧
	
	"-j",		// inputRateNumer
	"30",
	"-J",		// inputRateDenom
	"1",
	
	"-f",		// outputRateNumer
	"30",
	"-F",		// outputRateDenom
	"1",
	
	"-w",
	"352",
	"-h",
	"288"
	"-q",
	"20",
	"-m",
	"36",
	
//	"-2",		// MAD based MB QP adjustment
//	"-3",		
	
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	

	"-M",		// quarterPixelMv
	"1",
	"-8",		// trans8x8
	"1",	

	"-Z",		//	"-videoStab",
	"1",

	"-B",		//"-bitPerSecond",
	"80000",

   "--inputFormat=1",      // Y_UV420 (ON2_H264_ENC_INPUT_FORMAT_YUV420_semi_planar)

	"-i",
	"\\MINITEST\\RAW\\VIDEO\\Y_UV420\\03520288.YUV",
	"-K",		//	"-enableCabac",
	"1",
	"-o",
	"\\MINITEST\\ON2ENC\\Y_UV420\\03520288.264"	
};

static OS_STACKPTR int H264_ENC_Stack_1080P_SLICE[0x8000];          /* Task stacks */
static OS_TASK TCB_H264_ENC_1080P_SLICE;                        /* Task-control-blocks */
static OS_STACKPTR int H264_ENC_Stack_720P_YUV420[0x8000];          /* Task stacks */
static OS_TASK TCB_H264_ENC_720P_YUV420;     

static OS_STACKPTR int H264_ENC_Stack_1080P_Y_UV420[0x8000];          /* Task stacks */
static OS_TASK TCB_H264_ENC_1080P_Y_UV420;     

static OS_STACKPTR int H264_ENC_Stack_VGA_Y_UV420[0x8000];          /* Task stacks */
static OS_TASK TCB_H264_ENC_VGA_Y_UV420;     

static OS_STACKPTR int H264_ENC_Stack_CIF_Y_UV420[0x8000];          /* Task stacks */
static OS_TASK TCB_H264_ENC_CIF_Y_UV420;     

static OS_STACKPTR int H264_DEC_Stack_1080P[0x4000];          /* Task stacks */
static OS_TASK TCB_H264_DEC_1080P;     
static OS_STACKPTR int H264_DEC_Stack_720P[0x4000];          /* Task stacks */
static OS_TASK TCB_H264_DEC_720P;     
static OS_STACKPTR int H264_DEC_Stack_VGA[0x4000];          /* Task stacks */
static OS_TASK TCB_H264_DEC_VGA;     

static OS_STACKPTR int JPEG_DEC_Stack_1[0x4000];          /* Task stacks */
static OS_TASK TCB_JPEG_DEC_1;

static OS_STACKPTR int JPEG_ENC_Stack_1[0x4000];          /* Task stacks */
static OS_TASK TCB_JPEG_ENC_1;     


extern int arkn141_x264_encoder_test (int argc, char *argv[]);

 

static void h264_encode_1080P_SLICE (void)
{
    printf ("1080P H264 Encode slice\n");
	 arkn141_codec_open (ARKN141_CODEC_TYPE_H264_ENCODER);
    arkn141_x264_encoder_test (sizeof(argv_1080P_slice)/sizeof(char *), argv_1080P_slice);	
	 arkn141_codec_close (ARKN141_CODEC_TYPE_H264_ENCODER);
    printf ("1080P H264 Encode slice end\n");
    OS_Suspend (&TCB_H264_ENC_1080P_SLICE);
}

static void h264_encode_CIF_Y_UV420 (void)
{
    printf ("CIF Y_UV420 H264 Encode\n");
	 arkn141_codec_open (ARKN141_CODEC_TYPE_H264_ENCODER);
    arkn141_x264_encoder_test (sizeof(argv_CIF_Y_UV420)/sizeof(char *), argv_CIF_Y_UV420);	
	 arkn141_codec_close (ARKN141_CODEC_TYPE_H264_ENCODER);
    printf ("CIF H264 Y_UV420 Encode End\n");
    OS_Suspend (&TCB_H264_ENC_CIF_Y_UV420);
}

static void h264_encode_VGA_Y_UV420 (void)
{
    printf ("VGA Y_UV420 H264 Encode\n");
	 arkn141_codec_open (ARKN141_CODEC_TYPE_H264_ENCODER);
    arkn141_x264_encoder_test (sizeof(argv_VGA_Y_UV420)/sizeof(char *), argv_VGA_Y_UV420);	
	 arkn141_codec_close (ARKN141_CODEC_TYPE_H264_ENCODER);
    printf ("VGA H264 Y_UV420 Encode End\n");
    OS_Suspend (&TCB_H264_ENC_VGA_Y_UV420);
}

static void h264_encode_720P_YUV420 (void)
{
    printf ("720P YUV420 H264 Encode\n");
	 arkn141_codec_open (ARKN141_CODEC_TYPE_H264_ENCODER);
    arkn141_x264_encoder_test (sizeof(argv_720P_YUV420)/sizeof(char *), argv_720P_YUV420);	
	 arkn141_codec_close (ARKN141_CODEC_TYPE_H264_ENCODER);
    printf ("720P H264 YUV420 Encode End\n");
    OS_Suspend (&TCB_H264_ENC_720P_YUV420);
}

static void h264_encode_1080P_Y_UV420 (void)
{
    printf ("1080P Y_UV420 H264 Encode\n");
	 arkn141_codec_open (ARKN141_CODEC_TYPE_H264_ENCODER);
    arkn141_x264_encoder_test (sizeof(argv_1080P_Y_UV420)/sizeof(char *), argv_1080P_Y_UV420);	
	 arkn141_codec_close (ARKN141_CODEC_TYPE_H264_ENCODER);
    printf ("1080P H264 Y_UV420 Encode End\n");
    OS_Suspend (&TCB_H264_ENC_1080P_Y_UV420);
}


static void h264_decode_1080P (void)
{
    printf ("1080P H264 Decode\n");
	 arkn141_codec_open (ARKN141_CODEC_TYPE_H264_DECODER);
	 h264_decode ("\\MINITEST\\RAW\\VIDEO\\1080.264", 		"\\MINITEST\\ON2DEC\\H264\\Y_UV420\\1080P.YUV");
	 arkn141_codec_close (ARKN141_CODEC_TYPE_H264_DECODER);
    printf ("1080P H264 Decode End\n");
    OS_Suspend (&TCB_H264_DEC_1080P);
}

static void h264_decode_720P (void)
{
    printf ("720P H264 Decode\n");
	 arkn141_codec_open (ARKN141_CODEC_TYPE_H264_DECODER);
	 h264_decode ("\\MINITEST\\RAW\\VIDEO\\720P.264", 		"\\MINITEST\\ON2DEC\\H264\\Y_UV420\\720P.YUV");
	 arkn141_codec_close (ARKN141_CODEC_TYPE_H264_DECODER);
    printf ("720P H264 Decode End\n");
    OS_Suspend (&TCB_H264_DEC_720P);
}

static void h264_decode_VGA (void)
{
    printf ("VGA H264 Decode\n");
	 arkn141_codec_open (ARKN141_CODEC_TYPE_H264_DECODER);
	 h264_decode ("\\MINITEST\\RAW\\VIDEO\\06400480.264", 		"\\MINITEST\\ON2DEC\\H264\\Y_UV420\\06400480.YUV");
	 arkn141_codec_close (ARKN141_CODEC_TYPE_H264_DECODER);
    printf ("VGA H264 Decode End\n");
    OS_Suspend (&TCB_H264_DEC_VGA);
}


static  char *argv_640480_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\RAW\\JPEG\\06400480\\00000003.JPG",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\JPEG\\06400480\\00000003.YUV"
};

static void jpeg_decode_1 (void)
{
	 printf ("JPEG Decode\n");
	 arkn141_codec_open (ARKN141_CODEC_TYPE_JPEG_DECODER);
	 jpeg_decode_main(sizeof(argv_640480_jpeg_decode)/sizeof(char *), 	argv_640480_jpeg_decode );	
	 arkn141_codec_close (ARKN141_CODEC_TYPE_JPEG_DECODER);
    printf ("JPEG Decode End\n");
    OS_Suspend (&TCB_JPEG_DEC_1);
}

static  char *argv_jpeg_encode_640480[] = {
	"arkn141",
	"-w",	"640",
	"-h",	"480",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0
	"-i",	"\\MINITEST\\RAW\\YUV420\\06400480\\00000003.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\JPEG\\06400480\\00000003.JPG"
};

static  char *argv_jpeg_encode_12800720[] = {
	"arkn141",
	"-w",	"1280",
	"-h",	"720",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0
	"-i",	"\\MINITEST\\RAW\\YUV420\\12800720\\00000001.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\JPEG\\12800720\\00000001.JPG"
};

static  char *argv_jpeg_encode_19201080[] = {
	"arkn141",
	"-w",	"1920",
	"-h",	"1080",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0
	"-i",	"\\MINITEST\\RAW\\YUV420\\19201080\\00000002.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\JPEG\\19201080\\00000002.JPG"
};

static void jpeg_encode_1 (void)
{
	printf ("JPEG Encode 1\n");
	arkn141_codec_open (ARKN141_CODEC_TYPE_JPEG_ENCODER);
	SetJpegQLevel (9);
	jpeg_encode_main (sizeof(argv_jpeg_encode_640480)/sizeof(char *), argv_jpeg_encode_640480);	
	jpeg_encode_main (sizeof(argv_jpeg_encode_12800720)/sizeof(char *), argv_jpeg_encode_12800720);	
	jpeg_encode_main (sizeof(argv_jpeg_encode_19201080)/sizeof(char *), argv_jpeg_encode_19201080);	
	
	arkn141_codec_close (ARKN141_CODEC_TYPE_JPEG_ENCODER);
	printf ("JPEG Encode End\n");
	OS_Suspend (&TCB_JPEG_ENC_1);
}

// 多个编解码器并行工作测试
// 采用多线程同时测试 H264 encoder/decoder 以及 JPEG encoder/decoder
// 以下测试中仅使用3个并发线程, 
// 注意:
//		使用FPGA版本20141011_arkn141_en
void h264_multiple_encoder_test (void)
{	
//	OS_CREATETASK(&TCB_H264_ENC_1080P_SLICE,   "H264 Enc_1080P_SLICE",   h264_encode_1080P_SLICE,   250, H264_ENC_Stack_1080P_SLICE);
//	OS_CREATETASK(&TCB_H264_ENC_CIF_Y_UV420,   "H264 Enc CIF Y_UV420",   h264_encode_CIF_Y_UV420,   250, H264_ENC_Stack_CIF_Y_UV420);
//	OS_CREATETASK(&TCB_H264_ENC_VGA_Y_UV420,   "H264 Enc VGA Y_UV420",   h264_encode_VGA_Y_UV420,   250, H264_ENC_Stack_VGA_Y_UV420);
	//OS_CREATETASK(&TCB_H264_ENC_720P_YUV420,   "H264 Enc 720P YUV420",   h264_encode_720P_YUV420,   250, H264_ENC_Stack_720P_YUV420);
	OS_CREATETASK(&TCB_H264_ENC_1080P_Y_UV420, "H264 Enc 1080P Y_UV420", h264_encode_1080P_Y_UV420, 250, H264_ENC_Stack_1080P_Y_UV420);
	 
//	OS_CREATETASK(&TCB_JPEG_ENC_1, "JPEG Enc_1", jpeg_encode_1, 250, JPEG_ENC_Stack_1);
}


void h264_multiple_decoder_test (void)
{	
	 
	//OS_CREATETASK(&TCB_H264_DEC_1080P, "H264 Dec_1080P", h264_decode_1080P, 250, H264_DEC_Stack_1080P);
	OS_CREATETASK(&TCB_H264_DEC_720P,  "H264 Dec_720P",  h264_decode_720P,  250, H264_DEC_Stack_720P);
	OS_CREATETASK(&TCB_H264_DEC_VGA,   "H264 Dec_VGA",   h264_decode_VGA,   250, H264_DEC_Stack_VGA);
	
	OS_CREATETASK(&TCB_JPEG_DEC_1, "JPEG Dec_1", jpeg_decode_1, 250, JPEG_DEC_Stack_1);

}

extern int arkn141_yuv2jpg_test (char *volume_name);
extern int arkn141_yuv2jpg_slice_test (char *volume_name);
extern int on2_encode_test (void);

// 1) 测试FPGA版本
//		encoder
//			20150317_arkn141_encoder_sd_dma_OK
//			20150616_arkn141_encoder

//#define INCLUDE_H264_JPEG_MULTIPLE_ENCODER_RUN		// 2个H264/1个JPEG编码任务并行工作, DCACHE开/关状态下均已测试, 结果完全一致, OK
//#define	INCLUDE_JPEG_FORMAT_ENCODER_TEST		// Y_UV420, YUV420, YUYV 3种格式的YUV数据文件测试, 已测试, OK
//#define	INCLUDE_JPEG_SLICE_TEST				// JPEG宏块行测试 (slice)  DCACHE开/关状态下均已测试, 结果完全一致, OK
#define	INCLUDE_ON2_ENCODER_BASELINE_TEST

// *********************************************
// H264测试时, 需要将JTag的速度降低, 
//	J-Link/J-Trace JTag/SWD speed 设置为Auto, Initial 2000 khz
// 其他测试时, 速度设置为Fixed 10000 kHz
// *********************************************


// H264 + JPEG 编解码测试
int arkn141_h264_jpeg_encoder_test (void)
{
	printf ("arkn141_h264_jpeg_test start...\n");
	
	printf ("Please Insert volume(%s) to begin\n", "mmc:");
	while(1)
	{
		int volume_status = FS_GetVolumeStatus ("mmc:");
		if(volume_status == FS_MEDIA_IS_PRESENT)
		{
			printf ("Volume(%s) present\n", "mmc:");
			break;
		}
		OS_Delay (100);
	}
	
	
#ifdef INCLUDE_H264_JPEG_MULTIPLE_ENCODER_RUN
	h264_multiple_encoder_test ();
#endif
	
	
	// encoder测试
	
	// ahb_clk_en[31] : 1: enbale codec clk
   //                  0: close  codec clk
	rSYS_AHB_CLK_EN = rSYS_AHB_CLK_EN | (unsigned int)(1 << 31);
	//rSYS_AHB_CLK_EN &= ~(1 << 31);
	
	// 编码模式
	rSYS_DEVICE_CLK_CFG0 &=~(0x3<<28);
	rSYS_DEVICE_CLK_CFG0 |= (0x1<<28);
	
#ifdef INCLUDE_ON2_ENCODER_BASELINE_TEST
	on2_encode_test ();
#endif
	
#ifdef INCLUDE_JPEG_FORMAT_ENCODER_TEST	// Y_UV420, YUV420, YVUV
	arkn141_yuv2jpg_test ("mmc"); 
#endif
	
#ifdef INCLUDE_JPEG_SLICE_TEST	// slice test (宏块行分片测试)
	arkn141_yuv2jpg_slice_test ("mmc"); 
#endif
	
	
	
	printf ("arkn141_h264_jpeg_test finish\n");
	return 0;
}

// FPGA版本
// 1) 20150619_arkn141_decoder
// 测试时钟组合, 共4种
// 1) AXI/DDR/Codec 20/25/6 , OK
// 2) AXI/DDR/Codec 10/25/10, OK
// 3) AXI/DDR/Codec 15/12/18, OK
// 4) AXI/DDR/Codec 25/10/15, OK

//#define	INCLUDE_H264_DECODE_BASELINE_TEST		// 包含CIF/VGA/720P/1080P规格的H264码流解码, 已测试, OK
//#define	INCLUDE_JPEG_DECODE_BASELINE_TEST			// 已测试, OK
//#define	INCLUDE_H264_JPEG_MULTIPLE_DECODER_RUN		// 并发线程测试 (2个H264解码线程(1080P+720P), 1个JPEG解码线程), 已测试, OK
// H264 + JPEG 解码测试
int arkn141_h264_jpeg_decoder_test (void)
{
	printf ("arkn141_h264_jpeg_decoder_test start...\n");
	
	printf ("Please Insert volume(%s) to begin\n", "mmc:");
	while(1)
	{
		int volume_status = FS_GetVolumeStatus ("mmc:");
		if(volume_status == FS_MEDIA_IS_PRESENT)
		{
			printf ("Volume(%s) present\n", "mmc:");
			break;
		}
		OS_Delay (100);
	}
	
	
#ifdef INCLUDE_H264_JPEG_MULTIPLE_DECODER_RUN
	h264_multiple_decoder_test ();
#endif
	
	
	// decoder测试
	
	// ahb_clk_en[31] : 1: enbale codec clk
   //                  0: close  codec clk
	rSYS_AHB_CLK_EN |= (unsigned int)(1 << 31);
	//rSYS_AHB_CLK_EN &= ~(1 << 31);
	
	// 解码模式
	rSYS_DEVICE_CLK_CFG0 &=~(0x3<<28);
	rSYS_DEVICE_CLK_CFG0 |= (0x0<<28);
	
#ifdef INCLUDE_H264_DECODE_BASELINE_TEST
	on2_decode_test ();
#endif
	
#ifdef INCLUDE_JPEG_DECODE_BASELINE_TEST	// 基本JPEG解码功能验证, 多种规格尺寸的JPEG解码
	arkn141_jpg2yuv_test ("mmc", 1); 
#endif	
	
	
	printf ("arkn141_h264_jpeg_decoder_test finish\n");
	return 0;
}

int arkn141_h264_jpeg_test (void)
{
	
	printf ("Please Insert volume(%s) to begin\n", "mmc:");
	while(1)
	{
		int volume_status = FS_GetVolumeStatus ("mmc:");
		if(volume_status == FS_MEDIA_IS_PRESENT)
		{
			printf ("Volume(%s) present\n", "mmc:");
			break;
		}
		OS_Delay (100);
	}

	//while(1)
	{
	arkn141_h264_jpeg_encoder_test ();		// 编码测试
	}
	
	arkn141_h264_jpeg_decoder_test ();		// 解码测试
	
	FS_CACHE_Clean("");
	
	
#ifdef INCLUDE_H264_JPEG_MULTIPLE_ENCODER_RUN
	h264_multiple_encoder_test ();
#endif
#ifdef INCLUDE_H264_JPEG_MULTIPLE_DECODER_RUN
	h264_multiple_decoder_test ();
#endif
	
	return 0;
}