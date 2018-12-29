/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract : H264 Encoder testbench for linux
--
------------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
int on2_integration_main(int argc, char *argv[]);
void ewl_init (void);
void ewl_exit (void);
int  hx280enc_init(void);
int  hx170dec_init(void);
int h264_decode (const char *h264_file_name, const char *yuv_file_name);

//extern int h264_codec_main(int argc, char *argv[]);

static  char *argv[] = {
	"arkn141",
	"-w",
	//"352",
	"1920",
	"-h",
	//"288"
	"1088",
	"-i",
	//"\\MINITEST\\RAW\\VIDEO\\03520288.YUV",
	"\\MINITEST\\RAW\\VIDEO\\19201088.YUV",
};

static  char *argv_03520288_mono[] = {
	"arkn141",
	"-w",
	"352",
	"-h",
	"288"
	"-q",
	"20",
	"-m",
	"36",
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\test.YUV",
	"-K",		//	"-enableCabac",
	"1",
	"-o",
	"\\MINITEST\\ON2ENC\\mono.264"	
};

static  char *argv_03520288_YUV420[] = {
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

   "--inputFormat=0",      // YUV420 (ON2_H264_ENC_INPUT_FORMAT_YUV420_planar)

	"-i",
	"\\MINITEST\\RAW\\VIDEO\\YUV420\\03520288.YUV",
	"-K",		//	"-enableCabac",
	"1",
	"-o",
	"\\MINITEST\\ON2ENC\\YUV420\\03520288.264"	
};

static  char *argv_03520288_Y_UV420[] = {
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

   "--inputFormat=1",      // Y_UV420(H264_ENC_INPUT_FORMAT_YUV420_semiplanar)

	"-i",
	"\\MINITEST\\RAW\\VIDEO\\Y_UV420\\03520288.YUV",
	"-K",		//	"-enableCabac",
	"1",
	"-o",
	"\\MINITEST\\ON2ENC\\Y_UV420\\03520288.264"	
};


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

static  char *argv_1080P_CAVS[] = {
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
				//	Controls the usage of 1/4 pixel motion estimation precision. 
				//	When disabled the motion estimation will be done using 1/4 pixel precision. 
				//	1/4 pixel precision improves compression efficiency but takes more time.
	H264_PIXELMV,		//	Value 1 enables the adaptive usage based on resolution
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"1",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"1600000",
	"-o",
#if ON2_H264_ENC_INPUT_FORMAT_YUV420 == ON2_H264_ENC_INPUT_FORMAT_YUV420_planar
	"\\MINITEST\\ON2ENC\\YUV420\\1080CAVS.264",
#elif ON2_H264_ENC_INPUT_FORMAT_YUV420 == ON2_H264_ENC_INPUT_FORMAT_YUV420_semiplanar
	"\\MINITEST\\ON2ENC\\Y_UV420\\1080CAVS.264",
#endif
	H264_ENC_VIEW_MODE,
};

static  char *argv_1080P_YUV420[] = {
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
	"--inputFormat=0",	// 输入格式选择 (YUV420 planar)	
	"-b",
	ENC_FRAME_COUNT,	// 100帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\YUV420\\19201088.YUV",
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
	"\\MINITEST\\ON2ENC\\YUV420\\1080.264",
	H264_ENC_VIEW_MODE,
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

static  char *argv_1080P_CA[] = {
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
	"-K",		//	"-enableCabac",
	"1",
//	"-Z",		//	"-videoStab",
//	"1",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"1600000",
	"-o",
#if ON2_H264_ENC_INPUT_FORMAT_YUV420 == ON2_H264_ENC_INPUT_FORMAT_YUV420_planar
	"\\MINITEST\\ON2ENC\\YUV420\\1080CA.264",
#elif ON2_H264_ENC_INPUT_FORMAT_YUV420 == ON2_H264_ENC_INPUT_FORMAT_YUV420_semiplanar
	"\\MINITEST\\ON2ENC\\Y_UV420\\1080CA.264",
#endif
	H264_ENC_VIEW_MODE,
};

static  char *argv_1080P_CA_CAR1080P[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"400",	// 100帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\CAR1080P.YUV",
	"-q",
	"20",
	"-m",
	"30",
	"-M",		// quarterPixelMv
	H264_PIXELMV,
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
//	"-Z",		//	"-videoStab",
//	"1",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"10000000",
	"-o",
	"\\MINITEST\\ON2ENC\\CAR1080P.264",
};

static  char *argv_1080P_slice_YUV420[] = {
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
	"--inputFormat=0",	// 输入格式选择 (YUV420 planar)
	"-b",
	"20",	// 100帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\YUV420\\19201088.YUV",
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
	"\\MINITEST\\ON2ENC\\YUV420\\1080_SL1.264",

	H264_ENC_VIEW_MODE,
	
	// 设置slice大小(宏块行个数)
	"-V",
	"1",		
};

static  char *argv_1080P_slice_Y_UV420[] = {
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
	"--inputFormat=1",	// 输入格式选择 (YUV420 planar)
	"-b",
	"20",	// 100帧
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
	"\\MINITEST\\ON2ENC\\Y_UV420\\1080_SL1.264",

	H264_ENC_VIEW_MODE,
	
	// 设置slice大小(宏块行个数)
	"-V",
	"1",		
};

static  char *argv_1080P_CAVS_CAR1080P[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"400",	// 100帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\CAR1080P.YUV",
	"-q",
	"20",
	"-m",
	"30",
	"-M",		// quarterPixelMv
	H264_PIXELMV,
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"1",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"10000000",
	"-o",
	"\\MINITEST\\ON2ENC\\CAR_CAVS.264",
};

static  char *argv_720P_CAVS[] = {
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
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\YUV420\\12800720.YUV",
   "--inputFormat=0",      // YUV420
	"-q",
	"20",
	"-m",
	H264_ENC_MAX_QP,
	"-M",		// quarterPixelMv
	H264_PIXELMV,
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"1",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"800000",
	"-o",
	"\\MINITEST\\ON2ENC\\YUV420\\720PCAVS.264",
};

static  char *argv_720P_CA[] = {
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
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\YUV420\\12800720.YUV",
   "--inputFormat=0",      // YUV420
	"-q",
	"20",
	"-m",
	H264_ENC_MAX_QP,
	"-M",		// quarterPixelMv
	H264_PIXELMV,
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
//	"-Z",		//	"-videoStab",
//	"1",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"800000",
	"-o",
	"\\MINITEST\\ON2ENC\\YUV420\\720PCA.264",
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
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\YUV420\\12800720.YUV",
   "--inputFormat=0",      // YUV420
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

static  char *argv_720P_Y_UV420[] = {
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
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\Y_UV420\\12800720.YUV",
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
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"800000",
	"-o",
	"\\MINITEST\\ON2ENC\\Y_UV420\\720P.264",
};

/******************************************************************************/
static  char *argv_jpeg_encode_160160[] = {
	"arkn141",
	"-w",	"160",
	"-h",	"160",
	"-m","0",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\01600160\\01600160.YUV",
};
static  char *argv_jpeg_encode_496304[] = {
	"arkn141",
	"-w",	"496",
	"-h",	"304",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\04960304\\04960304.YUV"
};
static  char *argv_jpeg_encode_512512[] = {
	"arkn141",
	"-w",	"512",
	"-h",	"512",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\05120512\\05120512.YUV"

};
static  char *argv_jpeg_encode_640480[] = {
	"arkn141",
	"-w",	"640",
	"-h",	"480",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\06400480\\06400480.YUV"

};
static  char *argv_jpeg_encode_720576[] = {
	"arkn141",
	"-w",	"720",
	"-h",	"576",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\07200576\\07200576.YUV"
};
static  char *argv_jpeg_encode_800480[] = {
	"arkn141",
	"-w",	"800",
	"-h",	"480",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\08000480\\08000480.YUV"
};

static  char *argv_jpeg_encode_800608[] = {
	"arkn141",
	"-w",	"800",
	"-h",	"608",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\08000608\\08000608.YUV"
};
static  char *argv_jpeg_encode_1280720[] = {
	"arkn141",
	"-w",	"1280",
	"-h",	"720",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\12800720\\12800720.YUV"
};
static  char *argv_jpeg_encode_1440912[] = {
	"arkn141",
	"-w",	"1440",
	"-h",	"912",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\14400912\\14400912.YUV"
};
static  char *argv_jpeg_encode_19201088[] = {
	"arkn141",
	"-w",	"1920",
	"-h",	"1088",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\19201088\\19201088.YUV"
};
static  char *argv_jpeg_encode_81760512[] = {
	"arkn141",
	"-w",	"8176",
	"-h",	"512",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\81760512\\81760512.YUV"
};
static  char *argv_jpeg_encode_05128176[] = {
	"arkn141",
	"-w",	"512",
	"-h",	"8176",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\05128176\\05128176.YUV"
};
static  char *argv_jpeg_encode_10244096[] = {
	"arkn141",
	"-w",	"1024",
	"-h",	"4096",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\10244096\\10244096.YUV"
};
static  char *argv_jpeg_encode_10244096_2[] = {
	"arkn141",
	"-w",	"1024",
	"-h",	"4096",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	
	"-i",	"\\MINITEST\\ON2DEC\\jpeg\\10244096\\RESULT.YUV"
};
static  char *argv_jpeg_encode_40961024[] = {
	"arkn141",
	"-w",	"4096",
	"-h",	"1024",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\40961024\\40961024.YUV"
};
static  char *argv_jpeg_encode_40961024_2[] = {
	"arkn141",
	"-w",	"4096",
	"-h",	"1024",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	  -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2DEC\\jpeg\\40961024\\RESULT.YUV"
};
static  char *argv_jpeg_encode_81760512_2[] = {
	"arkn141",
	"-w",	"8176",
	"-h",	"512",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	
	"-i",	"\\MINITEST\\ON2DEC\\jpeg\\81760512\\RESULT.YUV"
};
static  char *argv_jpeg_encode_05128176_2[] = {
	"arkn141",
	"-w",	"512",
	"-h",	"8176",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	
	"-i",	"\\MINITEST\\ON2DEC\\jpeg\\05128176\\RESULT.YUV"
};
static  char *argv_jpeg_encode_46720800_2[] = {
	"arkn141",
	"-w",	"4672",
	"-h",	"800",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	
	"-i",	"\\MINITEST\\ON2DEC\\jpeg\\46720800\\RESULT.YUV"
};
static  char *argv_jpeg_encode_35041024_2[] = {
	"arkn141",
	"-w",	"3504",
	"-h",	"1024",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	
	"-i",	"\\MINITEST\\ON2DEC\\jpeg\\35041024\\RESULT.YUV"
};
static  char *argv_jpeg_encode_81760512_yuv422_2[] = {
	"arkn141",
	"-w",	"8176",
	"-h",	"512",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	
	"-i",	"\\MINITEST\\ON2DEC\\jpyuv422\\81760512\\RESULT.YUV"
};
/*******************************************************************************/
static  char *argv_jpeg_encode_81760512_slice[] = {
	"arkn141",
	"-w",	"8176",
	"-h",	"512",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	
	"-p","1",   //表示需要分割成Sclice
	"-R","2",  //应该表示分割成2*16行的小片		
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\81760512\\81760512.YUV"
};

static  char *argv_jpeg_encode_19201088_slice[] = {
	"arkn141",
	"-w",	"1920",
	"-h",	"1088",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	
	"-p","1",   //表示需要分割成Sclice
	"-R","2",  //应该表示分割成2*16行的小片	
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\19201088\\19201088.YUV"
};
static  char *argv_jpeg_encode_1280720_slice[] = {
	"arkn141",
	"-w",	"1280",
	"-h",	"720",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0
	"-p","1",   //表示需要分割成Sclice
	"-R","2",  //应该表示分割成2*16行的小片	
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\12800720\\12800720.YUV"
};
static  char *argv_jpeg_encode_1440912_slice[] = {
	"arkn141",
	"-w",	"1440",
	"-h",	"912",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	
	"-p","1",   //表示需要分割成Sclice
	"-R","2",  //应该表示分割成2*16行的小片
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\14400912\\14400912.YUV"
};
static  char *argv_jpeg_encode_640480_slice[] = {
	"arkn141",
	"-w",	"640",
	"-h",	"480",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0
	"-p","1",   //表示需要分割成Sclice
	"-R","2",  //应该表示分割成2*16行的小片	
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\06400480\\06400480.YUV"
};

static  char *argv_jpeg_encode_12800720[] = {
	"arkn141",
	"-w",	"1280",
	"-h",	"720",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","0",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\264.YUV",
	"-o",	"\\264.jpg",
};

/*******************************************************************************/
//ON2 Jpeg : start
//VGA: start
static  char *argv_jpeg_encode_vga_yuv420_0[] = {
	"arkn141",
	"-w",	"640",
	"-h",	"480",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","0",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\YUV420\\06400480\\00000000.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\YUV420\\06400480\\Result00.jpg",
};
static  char *argv_jpeg_encode_vga_yuv420_1[] = {
	"arkn141",
	"-w",	"640",
	"-h",	"480",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","1",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\YUV420\\06400480\\00000000.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\YUV420\\06400480\\Result01.jpg",
};
static  char *argv_jpeg_encode_vga_y_uv420_0[] = {
	"arkn141",
	"-w",	"640",
	"-h",	"480",
	"-b","0",//  one frame	
	"-g","1",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","0",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\Y_UV420\\06400480\\00000000.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\Y_UV420\\06400480\\Result00.jpg",
};
static  char *argv_jpeg_encode_vga_y_uv420_1[] = {
	"arkn141",
	"-w",	"640",
	"-h",	"480",
	"-b","0",//  one frame	
	"-g","1",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","1",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\Y_UV420\\06400480\\00000000.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\Y_UV420\\06400480\\Result01.jpg",
};
static  char *argv_jpeg_encode_vga_yuyv_0[] = {
	"arkn141",
	"-w",	"640",
	"-h",	"480",
	"-b","0",//  one frame	
	"-g","2",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","0",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\YUYV\\06400480\\00000000.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\YUYV\\06400480\\Result00.jpg",
};
static  char *argv_jpeg_encode_vga_yuyv_1[] = {
	"arkn141",
	"-w",	"640",
	"-h",	"480",
	"-b","0",//  one frame	
	"-g","2",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","1",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\YUYV\\06400480\\00000000.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\YUYV\\06400480\\Result01.jpg",
};
//----VGA Finish
//----720p start
static  char *argv_jpeg_encode_720p_yuv420_0[] = {
	"arkn141",
	"-w",	"1280",
	"-h",	"720",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","0",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\YUV420\\12800720\\00000001.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\YUV420\\12800720\\Result00.jpg",
};
static  char *argv_jpeg_encode_720p_yuv420_YUV420_2[] = {
	"arkn141",
	"-w",	"1280",
	"-h",	"720",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","0",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\YUV420\\12800720\\00000015.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\YUV420\\12800720\\00000015.jpg",
};
static  char *argv_jpeg_encode_720p_yuv420_1[] = {
	"arkn141",
	"-w",	"1280",
	"-h",	"720",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","1",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\YUV420\\12800720\\00000001.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\YUV420\\12800720\\Result01.jpg",
};

static  char *argv_jpeg_encode_720p_y_uv420_0[] = {
	"arkn141",
	"-w",	"1280",
	"-h",	"720",
	"-b","0",//  one frame	
	"-g","1",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","0",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\Y_UV420\\12800720\\00000001.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\Y_UV420\\12800720\\Result00.jpg",
};
static  char *argv_jpeg_encode_720p_y_uv420_1[] = {
	"arkn141",
	"-w",	"1280",
	"-h",	"720",
	"-b","0",//  one frame	
	"-g","1",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","1",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\Y_UV420\\12800720\\00000001.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\Y_UV420\\12800720\\Result01.jpg",
};
static  char *argv_jpeg_encode_720p_yuyv_0[] = {
	"arkn141",
	"-w",	"1280",
	"-h",	"720",
	"-b","0",//  one frame	
	"-g","2",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","0",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\YUYV\\12800720\\00000001.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\YUYV\\12800720\\Result00.jpg",
};
static  char *argv_jpeg_encode_720p_yuyv_1[] = {
	"arkn141",
	"-w",	"1280",
	"-h",	"720",
	"-b","0",//  one frame	
	"-g","2",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","1",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\YUYV\\12800720\\00000001.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\YUYV\\12800720\\Result01.jpg",
};
//720p end 

//1080p start
static  char *argv_jpeg_encode_1080p_yuv420_0[] = {
	"arkn141",
	"-w",	"1920",
	"-h",	"1080",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","0",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\YUV420\\19201080\\00000002.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\YUV420\\19201080\\Result00.jpg",
};
static  char *argv_jpeg_encode_1080p_yuv420_1[] = {
	"arkn141",
	"-w",	"1920",
	"-h",	"1080",
	"-b","0",//  one frame	
	"-g","0",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","1",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\YUV420\\19201080\\00000002.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\YUV420\\19201080\\Result01.jpg",
};
static  char *argv_jpeg_encode_1080p_y_uv420_0[] = {
	"arkn141",
	"-w",	"1920",
	"-h",	"1080",
	"-b","0",//  one frame	
	"-g","1",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","0",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\Y_UV420\\19201080\\00000002.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\Y_UV420\\19201080\\Result00.jpg",
};
static  char *argv_jpeg_encode_1080p_y_uv420_1[] = {
	"arkn141",
	"-w",	"1920",
	"-h",	"1080",
	"-b","0",//  one frame	
	"-g","1",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","1",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\Y_UV420\\19201080\\00000002.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\Y_UV420\\19201080\\Result01.jpg",
};
static  char *argv_jpeg_encode_1080p_yuyv_0[] = {
	"arkn141",
	"-w",	"1920",
	"-h",	"1080",
	"-b","0",//  one frame	
	"-g","2",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","0",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\YUYV\\19201080\\00000002.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\YUYV\\19201080\\Result00.jpg",
};
static  char *argv_jpeg_encode_1080p_yuyv_1[] = {
	"arkn141",
	"-w",	"1920",
	"-h",	"1080",
	"-b","0",//  one frame	
	"-g","2",// YUV420 = 0	  Y_UV420 = 1  YUYV = 2
	"-m","1",// -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
	"-i",	"\\MINITEST\\ON2ENC\\jpeg\\YUYV\\19201080\\00000002.YUV",
	"-o",	"\\MINITEST\\ON2ENC\\jpeg\\YUYV\\19201080\\Result01.jpg",
};

//10800p end

//ON2 Jpeg : end
/*******************************************************************************/
static  char *argv_720PSTAB[] = {
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
	"-b",
	"200",	// 200帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\720PSTAB.YUV",
	"-q",
	"20",
	"-m",
	"36",
	"--mbRc=1",
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
	"-M",		// quarterPixelMv
	H264_PIXELMV,
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"1",
	"-B",		//"-bitPerSecond",
	"800000",
	"-o",
	"\\MINITEST\\ON2ENC\\720PSTAB.264",
};

static  char *argv_VGA_YUV420[] = {
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
	"\\MINITEST\\RAW\\VIDEO\\YUV420\\06400480.YUV",
   "--inputFormat=0",      // YUV420
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
	"\\MINITEST\\ON2ENC\\YUV420\\06400480.264",
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

static  char *argv_VGA_CA[] = {
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
	"\\MINITEST\\RAW\\VIDEO\\06400480.YUV",
	"-q",
	"20",
	"-m",
	H264_ENC_MAX_QP,	
	"-M",		// quarterPixelMv
	H264_PIXELMV,
	"-8",		// trans8x8
	"1",		
	"-K",		//	"-enableCabac",
	"1",
//	"-Z",		//	"-videoStab",
//	"1",
	"-D",		// disableDeblocking
	"0",
	"-o",
	"\\MINITEST\\ON2ENC\\VGACA.264",
};

static  char *argv_VGA_CAVS[] = {
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
	"\\MINITEST\\RAW\\VIDEO\\06400480.YUV",
	"-q",
	"20",
	"-m",
	H264_ENC_MAX_QP,	
	"-M",		// quarterPixelMv
	H264_PIXELMV,
	"-8",		// trans8x8
	"1",		
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"1",
	"-D",		// disableDeblocking
	"0",
	"-o",
	"\\MINITEST\\ON2ENC\\VGACAVS.264",
};


static  char *argv_1080P_DUCK_20_36[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\1080DUCK.YUV",
	"--inputFormat=0",      // YUV420
	"-q",
	"20",
	"-m",
	"36",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"18000000",
	"-o",
	"\\MINITEST\\ON2ENC\\DUCK1080\\20_36.264",
};

static  char *argv_1080P_DUCK_20_30[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\1080DUCK.YUV",
	"--inputFormat=0",      // YUV420
	"-q",
	"20",
	"-m",
	"30",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"18000000",
	"-o",
	"\\MINITEST\\ON2ENC\\DUCK1080\\20_30.264",
};

static  char *argv_1080P_DUCK_20_40[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\1080DUCK.YUV",
	"--inputFormat=0",      // YUV420
	"-q",
	"20",
	"-m",
	"40",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"18000000",
	"-o",
	"\\MINITEST\\ON2ENC\\DUCK1080\\20_40.264",
};

static  char *argv_1080P_TOSHIBA_1_20_30[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\TB1080_1.YUV",
	"-q",
	"20",
	"-m",
	"30",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"18000000",
	"-o",
	"\\MINITEST\\ON2ENC\\TOSHIBA\\1_20_30.264",
};

static  char *argv_1080P_BIRD0_20_30[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\18BIRD0.YUV",
	"--inputFormat=0",      // YUV420
	"-q",
	"20",
	"-m",
	"30",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"12000000",
	"-o",
	"\\MINITEST\\ON2ENC\\BIRD1080\\0_20_30.264",
};

static  char *argv_1080P_BIRD0_20_36[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\18BIRD0.YUV",
	"--inputFormat=0",      // YUV420
	"-q",
	"20",
	"-m",
	"36",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"12000000",
	"-o",
	"\\MINITEST\\ON2ENC\\BIRD1080\\0_20_36.264",
};

static  char *argv_1080P_BIRD0_20_40[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\18BIRD0.YUV",
	"--inputFormat=0",      // YUV420
	"-q",
	"20",
	"-m",
	"40",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"12000000",
	"-o",
	"\\MINITEST\\ON2ENC\\BIRD1080\\0_20_40.264",
};

static  char *argv_1080P_BIRD1_20_30[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\18BIRD1.YUV",
	"--inputFormat=0",      // YUV420
	"-q",
	"20",
	"-m",
	"30",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"12000000",
	"-o",
	"\\MINITEST\\ON2ENC\\BIRD1080\\1_20_30.264",
};

static  char *argv_1080P_BIRD1_20_36[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\18BIRD1.YUV",
	"--inputFormat=0",      // YUV420
	"-q",
	"20",
	"-m",
	"36",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"12000000",
	"-o",
	"\\MINITEST\\ON2ENC\\BIRD1080\\1_20_36.264",
};

static  char *argv_1080P_BIRD1_20_40[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\18BIRD1.YUV",
	"--inputFormat=0",      // YUV420
	"-q",
	"20",
	"-m",
	"40",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"12000000",
	"-o",
	"\\MINITEST\\ON2ENC\\BIRD1080\\1_20_40.264",
};

static  char *argv_1080P_BIRD2_20_30[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\18BIRD2.YUV",
	"--inputFormat=0",      // YUV420
	"-q",
	"20",
	"-m",
	"30",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"12000000",
	"-o",
	"\\MINITEST\\ON2ENC\\BIRD1080\\2_20_30.264",
};

static  char *argv_1080P_BIRD3_20_30[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\18BIRD3.YUV",
	"--inputFormat=0",      // YUV420
	"-q",
	"20",
	"-m",
	"30",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"12000000",
	"-o",
	"\\MINITEST\\ON2ENC\\BIRD1080\\3_20_30.264",
};

static  char *argv_1080P_TOSHIBA_1_20_36[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\TB1080_1.YUV",
	"-q",
	"20",
	"-m",
	"36",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"18000000",
	"-o",
	"\\MINITEST\\ON2ENC\\TOSHIBA\\1_20_36.264",
};

static  char *argv_1080P_TOSHIBA_2_20_30[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\TB1080_2.YUV",
	"-q",
	"20",
	"-m",
	"30",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"18000000",
	"-o",
	"\\MINITEST\\ON2ENC\\TOSHIBA\\2_20_30.264",
};

static  char *argv_1080P_TOSHIBA_0_20_30[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\TB1080_0.YUV",
	"-q",
	"20",
	"-m",
	"30",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"18000000",
	"-o",
	"\\MINITEST\\ON2ENC\\TOSHIBA\\0_20_30.264",
};

static  char *argv_1080P_TOSHIBA_3_20_30[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\TB1080_3.YUV",
	"-q",
	"20",
	"-m",
	"30",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"18000000",
	"-o",
	"\\MINITEST\\ON2ENC\\TOSHIBA\\3_20_30.264",
};

static  char *argv_1080P_TOSHIBA_2_20_36[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	"--gopLength=150",
	"--intraPicRate=60",		// I帧编码的间隔帧数
	"--intraQpDelta=-4",		// The intra frames in H.264 video can sometimes introduce noticeable 
									//		flickering because of different prediction method. 
									//		This problem can be overcome by adjusting the quantization of 
									//		the intra frames compared to the surrounding inter frames.
	"--mbQpAdjustment=-3",		// MAD based MB QP adjustment
	"-b",
	"1000",	// 1000帧
	"-i",
	"\\MINITEST\\RAW\\VIDEO\\TB1080_2.YUV",
	"-q",
	"20",
	"-m",
	"36",
	"-M",		// quarterPixelMv
	"0",
	"-8",		// trans8x8
	"1",	
	"-K",		//	"-enableCabac",
	"1",
	"-Z",		//	"-videoStab",
	"0",
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
	"-B",		//"-bitPerSecond",
	"18000000",
	"-o",
	"\\MINITEST\\ON2ENC\\TOSHIBA\\2_20_36.264",
};

/*   Jpeg Decode                 **********************************************/
static  char *argv_512512_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\05120512\\05120512.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\05120512\\05120512.yuv"
};
static  char *argv_4848_jpeg_decode[] = {
	"arkn141",
	/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\00480048\\00480048.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\00480048\\00480048.yuv"
};
static  char *argv_6464_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\00640064\\00640064.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\00640064\\00640064.yuv"
};
static  char *argv_160160_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\01600160\\01600160.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\01600160\\01600160.yuv"
};
static  char *argv_496304_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\04960304\\04960304.jpg",  /*input file  norway.jpg*/
	"-i","\\MINITEST\\ON2DEC\\jpeg\\04960304\\04960304.yuv"
};
static  char *argv_640480_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\06400480\\06400480.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\06400480\\06400480.yuv"
};
static  char *argv_720576_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\07200576\\07200576.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\07200576\\07200576.yuv"
};
static  char *argv_800480_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\08000480\\08000480.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\08000480\\08000480.yuv"
};
static  char *argv_800608_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\08000608\\08000608.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\08000608\\08000608.yuv"
};
static  char *argv_720P_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\12800720\\12800720.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\12800720\\12800720.yuv"
};
static  char *argv_1440912_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\14400912\\14400912.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\14400912\\14400912.yuv"
};
static  char *argv_1080P_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\19201088\\19201088.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\19201088\\19201088.yuv"
};
static  char *argv_46720800_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\46720800\\46720800.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\46720800\\46720800.yuv"
};
static  char *argv_35041024_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\35041024\\35041024.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\35041024\\35041024.yuv"
};

static  char *argv_10244096_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\10244096\\10244096.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\10244096\\10244096.yuv"
};
static  char *argv_40961024_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\40961024\\40961024.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\40961024\\40961024.yuv"
};
static  char *argv_81760512_jpeg_decode_yuv422[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpyuv422\\81760512\\81760512.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpyuv422\\81760512\\81760512.yuv"
};
static  char *argv_05128176_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\05128176\\05128176.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\05128176\\05128176.yuv"
};
static  char *argv_81760512_jpeg_decode[] = {
	"arkn141",/* 81768176 */
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2DEC\\jpeg\\81760512\\81760512.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2DEC\\jpeg\\81760512\\81760512.yuv"
};

/*******************************************************************************/
static  char *argv_640480_jpeg_decode_slice[] = {
	"arkn141",
	"-i","\\MINITEST\\ON2ENC\\jpeg\\06400480\\06400480.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2ENC\\jpeg\\06400480\\06400480.YUV"
};
static  char *argv_720P_jpeg_decode_slice[] = {
	"arkn141",
	"-i","\\MINITEST\\ON2ENC\\jpeg\\12800720\\12800720.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2ENC\\jpeg\\12800720\\12800720.YUV"
};
static  char *argv_1080P_jpeg_decode_slice[] = {
	"arkn141",
	"-i","\\MINITEST\\ON2ENC\\jpeg\\19201088\\19201088.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2ENC\\jpeg\\19201088\\19201088.YUV"
};
static  char *argv_81760512_jpeg_decode_slice[] = {
	"arkn141",
	 /* From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder. */
	"-i","\\MINITEST\\ON2ENC\\jpeg\\81760512\\81760512.jpg",  /*input file  norway.jpg*/
	"-o","\\MINITEST\\ON2ENC\\jpeg\\81760512\\81760512.yuv"
};
/*******************************************************************************/
void on2_encode_test_jpeg_slice()
{
	char level=9;/* 0---9 */
	//int slicerownum=2;// 32
	//int slicerownum=8;//128
	int slicerownum=1;//=slicerownum * 16 lines
	printf("ON2_JPEG_TEST.  Encode \r\n");
	printf("Min:160160 ，   Max:05128176 or 81760512.\r\n");
	printf("编码宽度高度不能超过 8176 (详见代码：JpegEncApi.c line:=697 \r\n");
	SetJpegQLevel(level) ;
	// slicerownum 不要超过480/16 = 30 。
	//for(slicerownum = 2; slicerownum <=30 ; slicerownum = slicerownum*2)
	{
		// 是否使用独立的分开的Slice YUV源数据
		SetSliceRowNum(slicerownum);
		for( level=0 ; level <= 9 ; level++ )
		{// encode
			SetJpegQLevel(level) ;
#if 1
		//	SetSliceRowNum(15);//16  240/16=15
			jpeg_encode_main (sizeof(argv_jpeg_encode_640480_slice)/sizeof(char *), argv_jpeg_encode_640480_slice);	
		//	SetSliceRowNum(21);//16 320/16
			jpeg_encode_main (sizeof(argv_jpeg_encode_1280720_slice)/sizeof(char *), argv_jpeg_encode_1280720_slice);	
		//	SetSliceRowNum(34);//16 1088/16/2=34
			jpeg_encode_main (sizeof(argv_jpeg_encode_19201088_slice)/sizeof(char *), argv_jpeg_encode_19201088_slice);
#endif
			//	jpeg_encode_main (sizeof(argv_jpeg_encode_05128176)/sizeof(char *), argv_jpeg_encode_05128176);
			// 最大尺寸 8192*8192*3/2 = 100663296( 100M )
			// 编码宽度高度不能超过 8176 (详见代码：JpegEncApi.c line:=697 )
	//		jpeg_encode_main (sizeof(argv_jpeg_encode_81760512_slice)/sizeof(char *), argv_jpeg_encode_81760512_slice);
		}
	}
}

void on2_encode_test_jpeg_noslice()
{
	//编码 大小宽度不能小于96 pixels
	char level=9;/*取值范围 0---9 */
	printf("ON2_JPEG_TEST.  Encode \r\n");
	printf("Min:160160 ，   Max:05128176 or 81760512.\r\n");
	printf("编码宽度高度不能超过 8176 (详见代码：JpegEncApi.c line:=697 \r\n");
	SetJpegQLevel(level) ;
	//jpeg_encode_main (sizeof(argv_jpeg_encode_12800720)/sizeof(char *), argv_jpeg_encode_12800720);	
	
	jpeg_encode_main (sizeof(argv_jpeg_encode_720p_yuv420_0)/sizeof(char *), argv_jpeg_encode_720p_yuv420_0);
	//jpeg_encode_main (sizeof(argv_jpeg_encode_720p_yuv420_YUV420_2)/sizeof(char *), argv_jpeg_encode_720p_yuv420_YUV420_2);
	printf("ON2_JPEG_TEST.  Encode finish \r\n");

	while(1);
	return;

	//return 0;// add by tangchao  can deleted 
#if 1
	//注意后面的注释 YUV420 只能编码成YUV420.jpg 同时YUV422 只能转成YUV422.jpg
	jpeg_encode_main (sizeof(argv_jpeg_encode_vga_yuv420_0)/sizeof(char *), argv_jpeg_encode_vga_yuv420_0);	
//	jpeg_encode_main (sizeof(argv_jpeg_encode_vga_yuv420_1)/sizeof(char *), argv_jpeg_encode_vga_yuv420_1);	
	jpeg_encode_main (sizeof(argv_jpeg_encode_vga_y_uv420_0)/sizeof(char *), argv_jpeg_encode_vga_y_uv420_0);	
//	jpeg_encode_main (sizeof(argv_jpeg_encode_vga_y_uv420_1)/sizeof(char *), argv_jpeg_encode_vga_y_uv420_1);	
//	jpeg_encode_main (sizeof(argv_jpeg_encode_vga_yuyv_0)/sizeof(char *), argv_jpeg_encode_vga_yuyv_0);	
	jpeg_encode_main (sizeof(argv_jpeg_encode_vga_yuyv_1)/sizeof(char *), argv_jpeg_encode_vga_yuyv_1);	
	jpeg_encode_main (sizeof(argv_jpeg_encode_720p_yuv420_0)/sizeof(char *), argv_jpeg_encode_720p_yuv420_0);	
//	jpeg_encode_main (sizeof(argv_jpeg_encode_720p_yuv420_1)/sizeof(char *), argv_jpeg_encode_720p_yuv420_1);	
	jpeg_encode_main (sizeof(argv_jpeg_encode_720p_y_uv420_0)/sizeof(char *), argv_jpeg_encode_720p_y_uv420_0);	
//	jpeg_encode_main (sizeof(argv_jpeg_encode_720p_y_uv420_1)/sizeof(char *), argv_jpeg_encode_720p_y_uv420_1);	
//	jpeg_encode_main (sizeof(argv_jpeg_encode_720p_yuyv_0)/sizeof(char *), argv_jpeg_encode_720p_yuyv_0);	
	jpeg_encode_main (sizeof(argv_jpeg_encode_720p_yuyv_1)/sizeof(char *), argv_jpeg_encode_720p_yuyv_1);	
	jpeg_encode_main (sizeof(argv_jpeg_encode_1080p_yuv420_0)/sizeof(char *), argv_jpeg_encode_1080p_yuv420_0);	
//	jpeg_encode_main (sizeof(argv_jpeg_encode_1080p_yuv420_1)/sizeof(char *), argv_jpeg_encode_1080p_yuv420_1);	
	jpeg_encode_main (sizeof(argv_jpeg_encode_1080p_y_uv420_0)/sizeof(char *), argv_jpeg_encode_1080p_y_uv420_0);	
//	jpeg_encode_main (sizeof(argv_jpeg_encode_1080p_y_uv420_1)/sizeof(char *), argv_jpeg_encode_1080p_y_uv420_1);	
//	jpeg_encode_main (sizeof(argv_jpeg_encode_1080p_yuyv_0)/sizeof(char *), argv_jpeg_encode_1080p_yuyv_0);	
	jpeg_encode_main (sizeof(argv_jpeg_encode_1080p_yuyv_1)/sizeof(char *), argv_jpeg_encode_1080p_yuyv_1);	


#else
	for( level=0 ; level <= 9 ; level++ )
	{// encode
		SetJpegqLevel(level) ;
		jpeg_encode_main (sizeof(argv_jpeg_encode_160160)/sizeof(char *), argv_jpeg_encode_160160);	
		jpeg_encode_main (sizeof(argv_jpeg_encode_496304)/sizeof(char *), argv_jpeg_encode_496304);	
		jpeg_encode_main (sizeof(argv_jpeg_encode_512512)/sizeof(char *), argv_jpeg_encode_512512);	
		jpeg_encode_main (sizeof(argv_jpeg_encode_640480)/sizeof(char *), argv_jpeg_encode_640480);	
		jpeg_encode_main (sizeof(argv_jpeg_encode_720576)/sizeof(char *), argv_jpeg_encode_720576);	
		jpeg_encode_main (sizeof(argv_jpeg_encode_800480)/sizeof(char *), argv_jpeg_encode_800480);	
		jpeg_encode_main (sizeof(argv_jpeg_encode_800608)/sizeof(char *), argv_jpeg_encode_800608);	
		jpeg_encode_main (sizeof(argv_jpeg_encode_1280720)/sizeof(char *), argv_jpeg_encode_1280720);	
		jpeg_encode_main (sizeof(argv_jpeg_encode_1440912)/sizeof(char *), argv_jpeg_encode_1440912);	
		
		jpeg_encode_main (sizeof(argv_jpeg_encode_10244096)/sizeof(char *), argv_jpeg_encode_10244096);
		jpeg_encode_main (sizeof(argv_jpeg_encode_40961024)/sizeof(char *), argv_jpeg_encode_40961024);

		jpeg_encode_main (sizeof(argv_jpeg_encode_19201088)/sizeof(char *), argv_jpeg_encode_19201088);
		jpeg_encode_main (sizeof(argv_jpeg_encode_05128176)/sizeof(char *), argv_jpeg_encode_05128176);
		// 最大尺寸 8192*8192*3/2 = 100663296( 100M )
		// 编码宽度高度不能超过 8176 (详见代码：JpegEncApi.c line:=697 )
		jpeg_encode_main (sizeof(argv_jpeg_encode_81760512)/sizeof(char *), argv_jpeg_encode_81760512);
	}
#endif // 
}

// H264/JPEG 编码 (DCache开启/关闭)
// FPGA版本 20150616_arkn141_encoder
// 测试时钟组合, 共4种
// 1) AXI/DDR/Codec 20/25/6
// 2) AXI/DDR/Codec 10/25/10
// 3) AXI/DDR/Codec 15/12/18
// 4) AXI/DDR/Codec 25/10/15

//#define	H264_ENCODE_BASIC_SET		// H264编码基本测试集合 (包含YUV420/Y_UV420), DCACHE使能/禁止, 已测试,OK
//#define	H264_1080P_TEST
//#define	ENCODE_1080P_SLICE			// SLICE编码(SLICE中断)测试(包含YUV420/Y_UV420), DCACHE使能/禁止, 已测试,OK 

//#define	H264_ENCODE_DUCK_TAKEOFF_TEST	// QP测试, 已测试3种, OK 
//#define  H264_ENCODE_BIRD_TEST		// QP质量评估

//#define	H264_ENCODE_TOSHIBA_TEST
//#define	H264_720P_TEST
//#define	H264_VGA_TEST
#define	ON2_JPEG_ENCODE_TEST
//#define	ON2_JPEG_ENCODE_SLICE_TEST //表示进行分片测试
extern int arkn141_x264_encoder_test (int argc, char *argv[]);

//#define	H264_FFMPEG_TEST		// FFMPEG API测试

//#define  ENCODE_1080P


// ASIC已验证 AXI 300M DDR 400M, 
//		Int_mfc_enc_clk_sel = 2; (audpll_clk)
//		mfc_enc_clk_div = 1; (即H264 ENC 400M)

//int on2_codec_test (void)
int on2_encode_test (void)
{
	unsigned int ticket = XM_GetTickCount();
	hx280enc_init ();
	ewl_init ();
	//on2_integration_main (0, NULL);	
	//h264_codec_main (sizeof(argv)/sizeof(char *), argv);	
	//h264_codec_main (sizeof(argv_03520288_mono)/sizeof(char *), argv_03520288_mono);	
#ifdef H264_ENCODE_BASIC_SET
	// H264的基本验证测试
	h264_codec_main (sizeof(argv_03520288_YUV420)/sizeof(char *), argv_03520288_YUV420);	
	h264_codec_main (sizeof(argv_03520288_Y_UV420)/sizeof(char *), argv_03520288_Y_UV420);	
	h264_codec_main (sizeof(argv_VGA_YUV420)/sizeof(char *), argv_VGA_YUV420);	
	h264_codec_main (sizeof(argv_VGA_Y_UV420)/sizeof(char *), argv_VGA_Y_UV420);	
	h264_codec_main (sizeof(argv_720P_YUV420)/sizeof(char *), argv_720P_YUV420);	
	h264_codec_main (sizeof(argv_720P_Y_UV420)/sizeof(char *), argv_720P_Y_UV420);	
	h264_codec_main (sizeof(argv_1080P_YUV420)/sizeof(char *), argv_1080P_YUV420);
	h264_codec_main (sizeof(argv_1080P_Y_UV420)/sizeof(char *), argv_1080P_Y_UV420);
	
#endif
	// 720P的视频,有3个不同的场景，用于videoStab算法确认
	//h264_codec_main (sizeof(argv_720PSTAB)/sizeof(char *), argv_720PSTAB);	

#ifdef H264_1080P_TEST
	// 1080P 三种测试	

#ifdef ENCODE_1080P	
	printf ("\n\n   -- Encode 1080P --  \n\n");
	ticket = XM_GetTickCount();
	
#ifdef H264_FFMPEG_TEST
	arkn141_x264_encoder_test (sizeof(argv_1080P_YUV420)/sizeof(char *), argv_1080P_YUV420);	
#else
	h264_codec_main (sizeof(argv_1080P_YUV420)/sizeof(char *), argv_1080P_YUV420);	
#endif
	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
#endif	// ENCODE_1080P
	
#ifdef ENCODE_1080P_CA	
	printf ("\n\n  -- Encode 1080P_CA --    \n\n");
	ticket = XM_GetTickCount();
#ifdef H264_FFMPEG_TEST
	arkn141_x264_encoder_test (sizeof(argv_1080P_CA)/sizeof(char *), argv_1080P_CA);	
#else
	h264_codec_main (sizeof(argv_1080P_CA)/sizeof(char *), argv_1080P_CA);	
#endif
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
#endif	// ENCODE_1080P_CA

#ifdef ENCODE_1080P_CAVS	
	printf ("\n\n  -- Encode 1080P_CAVS --   \n\n");
	ticket = XM_GetTickCount();
#ifdef H264_FFMPEG_TEST
	arkn141_x264_encoder_test (sizeof(argv_1080P_CAVS)/sizeof(char *), argv_1080P_CAVS);	
#else
	h264_codec_main (sizeof(argv_1080P_CAVS)/sizeof(char *), argv_1080P_CAVS);	
#endif
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
#endif	// ENCODE_1080P_CAVS
	
	
	// H264 SLICE测试
#ifdef ENCODE_1080P_SLICE	
	printf ("\n\n  -- Encode 1080P_SLICE --   \n\n");
	ticket = XM_GetTickCount();
#ifdef H264_FFMPEG_TEST
	arkn141_x264_encoder_test (sizeof(argv_1080P_slice_YUV420)/sizeof(char *), argv_1080P_slice_YUV420);	
	arkn141_x264_encoder_test (sizeof(argv_1080P_slice_Y_UV420)/sizeof(char *), argv_1080P_slice_Y_UV420);
#else
	h264_codec_main (sizeof(argv_1080P_slice_YUV420)/sizeof(char *), argv_1080P_slice_YUV420);	
	h264_codec_main (sizeof(argv_1080P_slice_Y_UV420)/sizeof(char *), argv_1080P_slice_Y_UV420);	
#endif
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
#endif	// ENCODE_1080P_SLICE
	
	
#endif	// H264_1080P_TEST	
	
#ifdef H264_720P_TEST
	printf ("\n\n   -- Encode 720P --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_720P_YUV420)/sizeof(char *), argv_720P_YUV420);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
	printf ("\n\n  -- Encode 720P_CA --    \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_720P_CA)/sizeof(char *), argv_720P_CA);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
	printf ("\n\n  -- Encode 720P_CAVS --   \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_720P_CAVS)/sizeof(char *), argv_720P_CAVS);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
#endif	// H264_720P_TEST
	
#ifdef H264_VGA_TEST
	printf ("\n\n   -- Encode VGA --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_VGA_YUV420)/sizeof(char *), argv_VGA_YUV420);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
	printf ("\n\n  -- Encode VGA_CA --    \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_VGA_CA)/sizeof(char *), argv_VGA_CA);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
	printf ("\n\n  -- Encode VGA_CAVS --   \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_VGA_CAVS)/sizeof(char *), argv_VGA_CAVS);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
#endif	// H264_VGA_TEST
	
#if 0
	printf ("\n\n   -- Encode CARD 1080P --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_CA_CAR1080P)/sizeof(char *), argv_1080P_CA_CAR1080P);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
#endif
	
#if 0
	printf ("\n\n   -- Encode CARD 1080P CAVS --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_CAVS_CAR1080P)/sizeof(char *), argv_1080P_CAVS_CAR1080P);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
#endif
	
	// 编码质量评估	
#ifdef H264_ENCODE_DUCK_TAKEOFF_TEST
	// *********** Duck Take Off ******************
	printf ("\n\n   -- Encode 1080DUCK QP 20 ~ 30 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_DUCK_20_30)/sizeof(char *), argv_1080P_DUCK_20_30);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
	printf ("\n\n   -- Encode 1080DUCK QP 20 ~ 36 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_DUCK_20_36)/sizeof(char *), argv_1080P_DUCK_20_36);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
	printf ("\n\n   -- Encode 1080DUCK QP 20 ~ 40 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_DUCK_20_40)/sizeof(char *), argv_1080P_DUCK_20_40);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
#endif
	
#ifdef H264_ENCODE_BIRD_TEST
	
#if 0
	// *********** Bird ******************
	printf ("\n\n   -- Encode BIRD0 QP 20 ~ 30 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_BIRD0_20_30)/sizeof(char *), argv_1080P_BIRD0_20_30);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
	printf ("\n\n   -- Encode BIRD0 QP 20 ~ 36 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_BIRD0_20_36)/sizeof(char *), argv_1080P_BIRD0_20_36);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
	printf ("\n\n   -- Encode BIRD0 QP 20 ~ 40 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_BIRD0_20_40)/sizeof(char *), argv_1080P_BIRD0_20_40);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);

	printf ("\n\n   -- Encode BIRD1 QP 20 ~ 30 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_BIRD1_20_30)/sizeof(char *), argv_1080P_BIRD1_20_30);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
	printf ("\n\n   -- Encode BIRD1 QP 20 ~ 36 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_BIRD1_20_36)/sizeof(char *), argv_1080P_BIRD1_20_36);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
	printf ("\n\n   -- Encode BIRD1 QP 20 ~ 40 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_BIRD1_20_40)/sizeof(char *), argv_1080P_BIRD1_20_40);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);

	printf ("\n\n   -- Encode BIRD2 QP 20 ~ 30 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_BIRD2_20_30)/sizeof(char *), argv_1080P_BIRD2_20_30);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
	printf ("\n\n   -- Encode BIRD3 QP 20 ~ 30 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_BIRD3_20_30)/sizeof(char *), argv_1080P_BIRD3_20_30);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
#endif
	
#endif
	
	
#ifdef H264_ENCODE_TOSHIBA_TEST
	/*
	printf ("\n\n   -- Encode TB1080_1 QP 20 ~ 30 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_TOSHIBA_1_20_30)/sizeof(char *), argv_1080P_TOSHIBA_1_20_30);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
	printf ("\n\n   -- Encode TB1080_1 QP 20 ~ 36 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_TOSHIBA_1_20_36)/sizeof(char *), argv_1080P_TOSHIBA_1_20_36);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
	printf ("\n\n   -- Encode TB1080_2 QP 20 ~ 30 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_TOSHIBA_2_20_30)/sizeof(char *), argv_1080P_TOSHIBA_2_20_30);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
	printf ("\n\n   -- Encode TB1080_2 QP 20 ~ 36 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_TOSHIBA_2_20_36)/sizeof(char *), argv_1080P_TOSHIBA_2_20_36);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	*/
	
	/*
	printf ("\n\n   -- Encode TB1080_0 QP 20 ~ 30 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_TOSHIBA_0_20_30)/sizeof(char *), argv_1080P_TOSHIBA_0_20_30);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	*/
	
	printf ("\n\n   -- Encode TB1080_3 QP 20 ~ 30 --  \n\n");
	ticket = XM_GetTickCount();
	h264_codec_main (sizeof(argv_1080P_TOSHIBA_3_20_30)/sizeof(char *), argv_1080P_TOSHIBA_3_20_30);	
	printf ("enc time=%d\n", XM_GetTickCount() - ticket);
	
#endif
	
#ifdef  ON2_JPEG_ENCODE_TEST
	on2_encode_test_jpeg_noslice();
#endif
	
#ifdef ON2_JPEG_ENCODE_SLICE_TEST  //表示进行分片测试
	//编码 大小宽度不能小于96 pixels
	on2_encode_test_jpeg_slice();
#endif

	ewl_exit ();
	
	return 0;
}

void on2_decode_test_jpeg_noslice()
{
	jpeg_decode_main(sizeof(argv_4848_jpeg_decode)/sizeof(char *), 	argv_4848_jpeg_decode );
	jpeg_decode_main(sizeof(argv_6464_jpeg_decode)/sizeof(char *), 	argv_6464_jpeg_decode );	
	jpeg_decode_main(sizeof(argv_160160_jpeg_decode)/sizeof(char *), 	argv_160160_jpeg_decode );	
//	jpeg_decode_main(sizeof(argv_496304_jpeg_decode)/sizeof(char *), 	argv_496304_jpeg_decode );	
	jpeg_decode_main(sizeof(argv_800480_jpeg_decode)/sizeof(char *), 	argv_800480_jpeg_decode );		
	jpeg_decode_main(sizeof(argv_512512_jpeg_decode)/sizeof(char *), 	argv_512512_jpeg_decode );	
	jpeg_decode_main(sizeof(argv_640480_jpeg_decode)/sizeof(char *), 	argv_640480_jpeg_decode );	
	jpeg_decode_main(sizeof(argv_720576_jpeg_decode)/sizeof(char *), 	argv_720576_jpeg_decode );	
	jpeg_decode_main(sizeof(argv_800608_jpeg_decode)/sizeof(char *), 	argv_800608_jpeg_decode );		
	jpeg_decode_main(sizeof(argv_720P_jpeg_decode)/sizeof(char *), 	argv_720P_jpeg_decode );	
	jpeg_decode_main(sizeof(argv_1440912_jpeg_decode)/sizeof(char *), 	argv_1440912_jpeg_decode );		
	jpeg_decode_main(sizeof(argv_1080P_jpeg_decode)/sizeof(char *), 	argv_1080P_jpeg_decode );	

	// 8170 解码器支持的最大尺寸图像 4672*3504=24556032 ( 24m )
	jpeg_decode_main(sizeof(argv_46720800_jpeg_decode)/sizeof(char *), 	argv_46720800_jpeg_decode );	
	jpeg_decode_main(sizeof(argv_35041024_jpeg_decode)/sizeof(char *), 	argv_35041024_jpeg_decode );	
	// 如果 不是8170 解码器 ，则支持最大81768176尺寸的图像
	jpeg_decode_main(sizeof(argv_05128176_jpeg_decode)/sizeof(char *), 	argv_05128176_jpeg_decode );	
	jpeg_decode_main(sizeof(argv_81760512_jpeg_decode)/sizeof(char *), 	argv_81760512_jpeg_decode );	
	jpeg_decode_main(sizeof(argv_10244096_jpeg_decode)/sizeof(char *), 	argv_10244096_jpeg_decode );	
	jpeg_decode_main(sizeof(argv_40961024_jpeg_decode)/sizeof(char *), 	argv_40961024_jpeg_decode );	
}

void on2_decode_test_jpeg_slice()
{
	int QOrder=0;
	SetSlice(1);
	//QOrder = 3;
	for( QOrder = 0 ; QOrder <= 9 ; QOrder++ )
	{
		SetSliceQOrder(QOrder);
#if 1
		jpeg_decode_main(sizeof(argv_640480_jpeg_decode_slice)/sizeof(char *), 	argv_640480_jpeg_decode_slice );	
		jpeg_decode_main(sizeof(argv_720P_jpeg_decode_slice)/sizeof(char *), 	argv_720P_jpeg_decode_slice );	
		jpeg_decode_main(sizeof(argv_1080P_jpeg_decode_slice)/sizeof(char *), 	argv_1080P_jpeg_decode_slice );	
#endif
		//jpeg_decode_main(sizeof(argv_81760512_jpeg_decode_slice)/sizeof(char *), 	argv_81760512_jpeg_decode_slice );	
	}
	SetSlice(1);
}

#include "fs.h"
// 比较文件 (空文件总是比较失败)
// 0  --> 文件完全相同
// -1 --> 文件比较失败(内容存在差异)
// -2 --> 文件比较失败(文件大小不同、文件不存在或文件IO读取异常等其他原因导致)
int xm_file_compare (const char *file_1, const char *file_2)
{
	FS_FILE *fp_1, *fp_2;
	void *buf_1 = NULL, *buf_2 = NULL;
	char *abuf_1, *abuf_2;
	int ret = -2;
	U32 size_1, size_2;
	U32 len;
	U32 off = 0;
	//int cmp_loop;
	#define	FIFOSIZE		0x80000
	fp_1 = FS_FOpen (file_1, "rb");
	fp_2 = FS_FOpen (file_2, "rb");	
	buf_1 = malloc (FIFOSIZE + 2048);
	buf_2 = malloc (FIFOSIZE + 2048);
	do
	{
		if(fp_1 == NULL || fp_2 == NULL)
			break;
		if(buf_1 == NULL || buf_2 == NULL)
			break;
		
		abuf_1 = (char *)(((U32)buf_1 + 1023) & (~1023));
		abuf_2 = (char *)(((U32)buf_2 + 1023) & (~1023));
		
		dma_inv_range ((U32)abuf_1, (U32)abuf_1 + FIFOSIZE);
		dma_inv_range ((U32)abuf_2, (U32)abuf_2 + FIFOSIZE);
		
		abuf_1 = (char *)vaddr_to_page_addr (abuf_1);
		abuf_2 = (char *)vaddr_to_page_addr (abuf_2);
		
		size_1 = FS_GetFileSize(fp_1);
		if(size_1 == (U32)(0xFFFFFFFF))
		{
			// IO异常
			break;
		}
		size_2 = FS_GetFileSize(fp_2);
		if(size_2 == (U32)(0xFFFFFFFF))
		{
			// IO异常
			break;
		}
		if(size_1 == 0 || size_2 == 0)	// 空文件总是比较失败
			break;
		if(size_1 != size_2)
			break;

		// 文件为非空文件，且文件大小一致
		ret = -1;
		off = 0;
		while(size_1 > 0)
		{
			len = size_1;
			if(len > FIFOSIZE)
				len = FIFOSIZE;
			
			//dma_flush_range ((U32)abuf_1, (U32)abuf_1 + len);
			//dma_flush_range ((U32)abuf_2, (U32)abuf_2 + len);
			dma_inv_range ((U32)abuf_1, (U32)abuf_1 + len);
			dma_inv_range ((U32)abuf_2, (U32)abuf_2 + len);
			if(FS_Read (fp_1, abuf_1, len) != len)
				break;
			if(FS_Read (fp_2, abuf_2, len) != len)
				break;
			if(memcmp (abuf_1, abuf_2, len))
			{
				printf ("memcmp NG, offset=0x%08x\n", off);
				break;
			}
			
			size_1 -= len;
			off += len;
		}
		
		if(size_1 == 0)
			ret = 0;
		
	} while(0);
	if(fp_1)
		FS_FClose (fp_1);
	if(fp_2)
		FS_FClose (fp_2);
	if(buf_1)
		free (buf_1);
	if(buf_2)
		free (buf_2);
	
	return ret;
}

int xm_copy_file (const char *src_file, const char *dst_file)
{
	FS_FILE *fp_1, *fp_2;
	void *buf_1 = NULL;
	char *abuf_1;
	int ret = -2;
	U32 size_1;
	U32 len;
	U32 off = 0;
	#define	FIFOSIZE		0x80000
	fp_1 = FS_FOpen (src_file, "rb");
	fp_2 = FS_FOpen (dst_file, "wb");	
	buf_1 = malloc (FIFOSIZE + 2048);
	do
	{
		if(fp_1 == NULL || fp_2 == NULL)
			break;
		if(buf_1 == NULL)
			break;
		
		abuf_1 = (char *)(((U32)buf_1 + 1023) & (~1023));
		
		dma_inv_range ((U32)abuf_1, (U32)abuf_1 + FIFOSIZE);
		
		abuf_1 = (char *)vaddr_to_page_addr (abuf_1);
		
		size_1 = FS_GetFileSize(fp_1);
		if(size_1 == (U32)(0xFFFFFFFF))
		{
			// IO异常
			break;
		}
		if(size_1 == 0)	// 空文件总是比较失败
			break;

		// 文件为非空文件，且文件大小一致
		ret = -1;
		off = 0;
		while(size_1 > 0)
		{
			len = size_1;
			if(len > FIFOSIZE)
				len = FIFOSIZE;
			
			//dma_flush_range ((U32)abuf_1, (U32)abuf_1 + len);
			//dma_flush_range ((U32)abuf_2, (U32)abuf_2 + len);
			dma_inv_range ((U32)abuf_1, (U32)abuf_1 + len);
			if(FS_Read (fp_1, abuf_1, len) != len)
				break;
			if(FS_Write (fp_2, abuf_1, len) != len)
				break;
			
			size_1 -= len;
			off += len;
		}
		
		if(size_1 == 0)
			ret = 0;
		
	} while(0);
	if(fp_1)
		FS_FClose (fp_1);
	if(fp_2)
		FS_FClose (fp_2);
	if(buf_1)
		free (buf_1);
	
	return ret;	
}


#include "rtos.h"


#define	INCLUDE_H264_DECODE_BASELINE			// 已测试, OK
#define	INCLUDE_H264_DECODE_TAKEOFF			// 鸭群飞出水面, 	已测试, OK
//#define	_1080P_DECODE_
//#define	_720P_DECODE_
//#define	_VGA_DECODE_
//#define   _JPEG_DECODE_
int on2_decode_test (void)
{
	char dec_file[128];
	hx170dec_init ();
	
	dwl_init ();
	
#ifdef _1080P_DECODE_
	printf ("\n\n --- 1080 dec ---\n\n");	
	h264_decode ("\\MINITEST\\RAW\\VIDEO\\1080.264", "\\MINITEST\\ON2DEC\\1080.YUV");	
	
	printf ("\n\n --- 1080CA dec ---\n\n");
	h264_decode ("\\MINITEST\\RAW\\VIDEO\\1080CA.264", "\\MINITEST\\ON2DEC\\1080CA.YUV");	
	
	printf ("\n\n --- 1080CAVS dec ---\n\n");
	h264_decode ("\\MINITEST\\RAW\\VIDEO\\1080CAVS.264", "\\MINITEST\\ON2DEC\\1080CAVS.YUV");	
#endif
	
#ifdef _720P_DECODE_
	printf ("\n\n --- 720P dec ---\n\n");
	h264_decode ("\\MINITEST\\RAW\\VIDEO\\720P.264", "\\MINITEST\\ON2DEC\\720P.YUV");	
	//printf ("\n\n --- 720PCA dec ---\n\n");
	//h264_decode ("\\MINITEST\\RAW\\VIDEO\\720PCA.264", "\\MINITEST\\ON2DEC\\720PCA.YUV");	
	//printf ("\n\n --- 720PCAVS dec ---\n\n");
	//h264_decode ("\\MINITEST\\RAW\\VIDEO\\720PCAVS.264", "\\MINITEST\\ON2DEC\\720PCAVS.YUV");	
#endif
	
#ifdef _VGA_DECODE_   
   h264_decode ("\\MINITEST\\ON2ENC\\YUV420\\06400480.264", 		"\\MINITEST\\ON2DEC\\YUV420\\06400480.YUV");	
#endif

	
#ifdef INCLUDE_H264_DECODE_BASELINE
   printf ("\n\n --- 03520288 dec ---\n\n");
	h264_decode ("\\MINITEST\\RAW\\VIDEO\\03520288.264", 	"\\MINITEST\\ON2DEC\\03520288.YUV");	
   printf ("\n\n --- 06400480 dec ---\n\n");
   h264_decode ("\\MINITEST\\RAW\\VIDEO\\06400480.264", 	"\\MINITEST\\ON2DEC\\06400480.YUV");	
   printf ("\n\n --- 720P dec ---\n\n");
   h264_decode ("\\MINITEST\\RAW\\VIDEO\\720P.264", 		"\\MINITEST\\ON2DEC\\720P.YUV");	
   printf ("\n\n --- 1080P dec ---\n\n");
   h264_decode ("\\MINITEST\\RAW\\VIDEO\\1080.264", 		"\\MINITEST\\ON2DEC\\1080.YUV");	
	//h264_decode ("\\MINITEST\\RAW\\VIDEO\\SVA_C.264", 	"\\MINITEST\\ON2DEC\\SVA_C.YUV");	
	//h264_decode ("\\MINITEST\\RAW\\VIDEO\\2REF_WP.264", 	"\\MINITEST\\ON2DEC\\2REF_WP.YUV");	
#endif
	
#ifdef INCLUDE_H264_DECODE_TAKEOFF
   printf ("\n\n --- TakeOff 1080P dec ---\n\n");
   h264_decode ("\\MINITEST\\RAW\\VIDEO\\TAKEOFF.264", 		"\\MINITEST\\ON2DEC\\TAKEOFF.YUV");	
#endif
	
#ifdef _JPEG_DECODE_
	// From 48x48 pixels to 4672x3504 or (3504x4672) pixels in the 8170 decoder
	// From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder.
	//printf("From 48x48 pixels to 4672x3504 or (3504x4672) pixels in the 8170 decoder.\r\n");
	//目前解码支持最大的宽高 为8176*8176 像素点
	//使用画图转换为*.bmp文件，然后用2345看图转化为*.jpg
	printf("From 48x48 pixels to 8176x8176 pixels in the 8190/9170/9190 decoder.\r\n");
	printf("Please don't use YUV422-format Jpeg file, cann't make sure the decode correct.\r\n");

#define  _JPEG_DECODE_SLICE_
	
#ifdef _JPEG_DECODE_NO_SLICE_
	on2_decode_test_jpeg_noslice();
#endif
	
#ifdef _JPEG_DECODE_SLICE_
	on2_decode_test_jpeg_slice();
#endif
	
	//jpeg_decode_main(sizeof(argv_81760512_jpeg_decode_yuv422)/sizeof(char *), 	argv_81760512_jpeg_decode_yuv422 );	
#endif
	dwl_exit ();
	return 0;
}


int on2_jpeg_test_reencode (void)
{
#ifdef  _JPEG_DECODE_
	unsigned int ticket = XM_GetTickCount();
	printf("将已经解码的文件重新编码为*.jpg文件。必须为YUV420编码数据\r\n");
	hx280enc_init ();
	ewl_init ();
	SetJpegqLevel(9) ;
#if 0
	jpeg_encode_main (sizeof(argv_jpeg_encode_40961024_2)/sizeof(char *), argv_jpeg_encode_40961024_2);
	jpeg_encode_main (sizeof(argv_jpeg_encode_10244096_2)/sizeof(char *), argv_jpeg_encode_10244096_2);
	jpeg_encode_main (sizeof(argv_jpeg_encode_81760512_2)/sizeof(char *), argv_jpeg_encode_81760512_2);
	jpeg_encode_main (sizeof(argv_jpeg_encode_05128176_2)/sizeof(char *), argv_jpeg_encode_05128176_2);
	jpeg_encode_main (sizeof(argv_jpeg_encode_46720800_2)/sizeof(char *), argv_jpeg_encode_46720800_2);
	jpeg_encode_main (sizeof(argv_jpeg_encode_35041024_2)/sizeof(char *), argv_jpeg_encode_35041024_2);
#endif	
	jpeg_encode_main (sizeof(argv_jpeg_encode_81760512_yuv422_2)/sizeof(char *), argv_jpeg_encode_81760512_yuv422_2);
	
	ewl_exit ();

#endif
	return 0;
}