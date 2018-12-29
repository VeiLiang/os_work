//****************************************************************************
//
//	Copyright (C) 2012~2017 ShenZhen ExceedSpace
//
//
//	Author	ZhuoYongHong
//
//	File name: avi_codec_video_setting.h
//	  AVI流编解码参数设置
//
//	Revision history
//
//		2017.06.18	ZhuoYongHong Initial version
//
//****************************************************************************


#define	ENC_FRAME_COUNT					"100"
#define	H264_ENC_MAX_QP					"30"							// H264最大QP值
#define	H264_ENC_MAD_QP_ADJUSTMENT		"--mbQpAdjustment=-3"	//"--mbQpAdjustment=0"		// 	
//#define	H264_ENC_MAD_QP_ADJUSTMENT		"--mbQpAdjustment=-0"	//"--mbQpAdjustment=0"		// 	

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

#define	H264_ENC_INPUT_FORMAT			H264_ENC_INPUT_FORMAT_YUV420_semiplanar


const char *argv_720P[] = {
	"arkn141",
	"-w",
	"1280",
	"-h",
	"720",
	//"--gopLength=240",			// 8秒码流缓存
	"--gopLength=60",			// 2秒码流缓存
	"--videoRange=1",				// 0..1 Video signal sample range value in H.264 stream.
										//		0 - Y range in [16..235] Cb,Cr in [16..240]
										//		1 - Y,Cb,Cr range in [0..255]
	
	"--intraPicRate=60",		// I帧编码的间隔帧数, 2秒一帧

	"--hrdConformance=0",		
	//"--hrdConformance=1",		//	HDR Conformance (ANNEX C) 
										//		0=OFF, 1=ON HRD conformance.
										//	Uses standard defined model to limit bitrate variance.	

	"--picRc=1",					//    0=OFF, 1=ON Picture rate control. [1]
										//		Calculates new target QP for every frame.


	"--intraQpDelta=-5",		// The intra frames in H.264 video can sometimes introduce noticeable 
		//		flickering because of different prediction method. 
		//		This problem can be overcome by adjusting the quantization of 
		//		the intra frames compared to the surrounding inter frames.
		
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
	
	//H26x4_ENC_INPUT_FORMAT_YUV420_planar,	// 输入格式选择 (YUV420 planar 或者YUV420 semiplanar)	
	H264_ENC_INPUT_FORMAT_YUV420_semiplanar,
	
	"-q",			/* QP for next encoded picture, [-1..51]
                              * -1 = Let rate control calculate initial QP
                              * This QP is used for all pictures if 
                              * HRD and pictureRc and mbRc are disabled
                              * If HRD is enabled it may override this QP
                              */
#if HIGH_VIDEO_QUALITY
	"22",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	"25",			// 一般图像质量
#else
	"23",			// 较高图像质量
#endif
	//"25",	
	
	"-n",
	
#if HIGH_VIDEO_QUALITY
	"20",		// Minimum QP for any picture, [0..51] 
#elif NORMAL_VIDEO_QUALITY
	//"23",
	"24",	
#else
	"21",
#endif
	//"21",		// Minimum QP for any picture, [0..51] 
	//"23",		// Minimum QP for any picture, [0..51] 
	
	"-m",
	//"36",		//
	//"45",
	//"36",
	//"40",
	//"35",		// 20170224 去机场测试(20~35), 阴天, 效果不好
	//"30",		// 20170227下午测试, 存在码流溢出,编码停顿的现象. 
	//"31",			// 	将最大QP值改为31
#if HIGH_VIDEO_QUALITY
	"29",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	//"32",			// 一般图像质量
	"37",	
#else
	"30",			// 较高图像质量 // 20170228 阴天上午的效果不佳, 将最大QP值重新调整为改为30
#endif

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
#if HIGH_VIDEO_QUALITY
	"6400000",
#elif NORMAL_VIDEO_QUALITY
	//"5600000",
	//"5120000",
	"5000000",
#else
	"5800000",			// 与其他方案码率一致
#endif
};

const char *argv_1080n[] = {
	"arkn141",
	"-w",
	"960",
	"-h",
	"1080",
	//"--gopLength=240",			// 8秒码流缓存
	"--gopLength=60",			// 2秒码流缓存
	"--videoRange=1",				// 0..1 Video signal sample range value in H.264 stream.
										//		0 - Y range in [16..235] Cb,Cr in [16..240]
										//		1 - Y,Cb,Cr range in [0..255]
	
	"--intraPicRate=60",		// I帧编码的间隔帧数, 2秒一帧

	"--hrdConformance=0",		
	//"--hrdConformance=1",		//	HDR Conformance (ANNEX C) 
										//		0=OFF, 1=ON HRD conformance.
										//	Uses standard defined model to limit bitrate variance.	

	"--picRc=1",					//    0=OFF, 1=ON Picture rate control. [1]
										//		Calculates new target QP for every frame.


	"--intraQpDelta=-5",		// The intra frames in H.264 video can sometimes introduce noticeable 
		//		flickering because of different prediction method. 
		//		This problem can be overcome by adjusting the quantization of 
		//		the intra frames compared to the surrounding inter frames.
		
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
	
	//H26x4_ENC_INPUT_FORMAT_YUV420_planar,	// 输入格式选择 (YUV420 planar 或者YUV420 semiplanar)	
	H264_ENC_INPUT_FORMAT_YUV420_semiplanar,
	
	"-q",			/* QP for next encoded picture, [-1..51]
                              * -1 = Let rate control calculate initial QP
                              * This QP is used for all pictures if 
                              * HRD and pictureRc and mbRc are disabled
                              * If HRD is enabled it may override this QP
                              */
#if HIGH_VIDEO_QUALITY
	"22",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	"25",			// 一般图像质量
#else
	"23",			// 较高图像质量
#endif
	//"25",	
	
	"-n",
	
#if HIGH_VIDEO_QUALITY
	"20",		// Minimum QP for any picture, [0..51] 
#elif NORMAL_VIDEO_QUALITY
	//"23",
	"24",	
#else
	"21",
#endif
	//"21",		// Minimum QP for any picture, [0..51] 
	//"23",		// Minimum QP for any picture, [0..51] 
	
	"-m",
	//"36",		//
	//"45",
	//"36",
	//"40",
	//"35",		// 20170224 去机场测试(20~35), 阴天, 效果不好
	//"30",		// 20170227下午测试, 存在码流溢出,编码停顿的现象. 
	//"31",			// 	将最大QP值改为31
#if HIGH_VIDEO_QUALITY
	"29",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	//"32",			// 一般图像质量
	"37",	
#else
	"30",			// 较高图像质量 // 20170228 阴天上午的效果不佳, 将最大QP值重新调整为改为30
#endif

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
#if HIGH_VIDEO_QUALITY
	"6400000",
#elif NORMAL_VIDEO_QUALITY
	//"5600000",
	//"5120000",
	"5000000",
#else
	"5800000",			// 与其他方案码率一致
#endif
};

const char *argv_1080P[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	
	//"--gopLength=30",
	//"--gopLength=240",			// 8秒码流缓存		
	"--gopLength=50",			// 2秒码流缓存	
	//"--gopLength=8",
	
#ifdef _HDTV_000255_
	"--videoRange=1",				// 0..1 Video signal sample range value in H.264 stream.
										//		0 - Y range in [16..235] Cb,Cr in [16..240]
										//		1 - Y,Cb,Cr range in [0..255]
#else
	"--videoRange=0",				// 0..1 Video signal sample range value in H.264 stream.
										//		0 - Y range in [16..235] Cb,Cr in [16..240]
										//		1 - Y,Cb,Cr range in [0..255]

#endif
										
	"--intraPicRate=60",		// I帧编码的间隔帧数, 2秒一帧
	
	"--hrdConformance=0",		
	//"--hrdConformance=1",		//	HDR Conformance (ANNEX C) 
										//		0=OFF, 1=ON HRD conformance.
										//	Uses standard defined model to limit bitrate variance.	
	//"--cpbSize=6000000",		// Coded Picture Buffer Size
	//"--cpbSize=14800000",		// 20170717,  增加CPB, 改善瞬时丢帧现象
										// 	Buffer size used by the HRD model.
										//		Size in bits of the coded picture buffer (CPB) used by the HRD model. 
										//		When HRD is enabled an encoded frame can’t be bigger than CPB. 
										//		By default the encoder will use the maximum allowed size for the initialized encoder level
									// 较小的cpbSize会导致帧的skip.
	
	"--picRc=1",					//    0=OFF, 1=ON Picture rate control. [1]
										//		Calculates new target QP for every frame.
	
	//"--fixedIntraQp=20",
	"--intraQpDelta=-5",		// The intra frames in H.264 video can sometimes introduce noticeable 
		//		flickering because of different prediction method. 
		//		This problem can be overcome by adjusting the quantization of 
		//		the intra frames compared to the surrounding inter frames.
		
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
		
	//H26x4_ENC_INPUT_FORMAT_YUV420_planar,	// 输入格式选择 (YUV420 planar 或者YUV420 semiplanar)	
	H264_ENC_INPUT_FORMAT_YUV420_semiplanar,
		
	"-q",			/* QP for next encoded picture, [-1..51]
                              * -1 = Let rate control calculate initial QP
                              * This QP is used for all pictures if 
                              * HRD and pictureRc and mbRc are disabled
                              * If HRD is enabled it may override this QP
                              */
#if HIGH_VIDEO_QUALITY
	"22",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	"25",			// 一般图像质量
#else
	"23",			// 较高图像质量
#endif
	//"25",
	
	"-n",
	
#if HIGH_VIDEO_QUALITY
	"20",		// Minimum QP for any picture, [0..51] 
#elif NORMAL_VIDEO_QUALITY
	//"23",
	"24",
#else
	"21",
#endif
	//"21",		// Minimum QP for any picture, [0..51] 
	//"23",		// Minimum QP for any picture, [0..51] 
	
	"-m",
	//"36",		//
	//"45",
	//"36",
	//"40",
	//"35",		// 20170224 去机场测试(20~35), 阴天, 效果不好
	//"30",		// 20170227下午测试, 存在码流溢出,编码停顿的现象. 
	//"31",			// 	将最大QP值改为31
#if HIGH_VIDEO_QUALITY
	"29",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	//"32",			// 一般图像质量
	//"35",			// 20171221 zhuoyonghong 降低平均码流 一般图像质量
	"37",			// 降低平均码流， 避免2路编码码流过大，无法写入而丢帧。
#else
	"30",			// 较高图像质量 // 20170228 阴天上午的效果不佳, 将最大QP值重新调整为改为30
#endif
		
	"-M",		// quarterPixelMv
	H264_PIXELMV,
		
	"-8",		// trans8x8
	"1",	
		
	"-K",		//	"-enableCabac",
	"1",
		
	//"-Z",		//	"-videoStab",
	//	"1",
		
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
	//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
		
	"-B",		//"-bitPerSecond",
#if HIGH_VIDEO_QUALITY
	"14000000",
#elif NORMAL_VIDEO_QUALITY
	//"12800000",
	"10400000",
#else
	"13200000",			// 与其他方案码率一致
#endif
	//"14800000",
	//"15200000",
	//"18000000",		// 20170217 提高码率,避免丢帧
};

// 普通质量视频编码
const char *argv_1080P_normal_video_quality[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	
	//"--gopLength=30",
	"--gopLength=240",			// 8秒码流缓存		
	//"--gopLength=8",
	
#ifdef _HDTV_000255_
	"--videoRange=1",				// 0..1 Video signal sample range value in H.264 stream.
										//		0 - Y range in [16..235] Cb,Cr in [16..240]
										//		1 - Y,Cb,Cr range in [0..255]
#else
	"--videoRange=0",				// 0..1 Video signal sample range value in H.264 stream.
										//		0 - Y range in [16..235] Cb,Cr in [16..240]
										//		1 - Y,Cb,Cr range in [0..255]

#endif
										
	"--intraPicRate=60",		// I帧编码的间隔帧数, 2秒一帧
	
	"--hrdConformance=0",		
	//"--hrdConformance=1",		//	HDR Conformance (ANNEX C) 
										//		0=OFF, 1=ON HRD conformance.
										//	Uses standard defined model to limit bitrate variance.	
	//"--cpbSize=6000000",		// Coded Picture Buffer Size
	//"--cpbSize=14800000",		// 20170717,  增加CPB, 改善瞬时丢帧现象
										// 	Buffer size used by the HRD model.
										//		Size in bits of the coded picture buffer (CPB) used by the HRD model. 
										//		When HRD is enabled an encoded frame can’t be bigger than CPB. 
										//		By default the encoder will use the maximum allowed size for the initialized encoder level
									// 较小的cpbSize会导致帧的skip.
	
	"--picRc=1",					//    0=OFF, 1=ON Picture rate control. [1]
										//		Calculates new target QP for every frame.
	
	//"--fixedIntraQp=20",
	"--intraQpDelta=-5",		// The intra frames in H.264 video can sometimes introduce noticeable 
		//		flickering because of different prediction method. 
		//		This problem can be overcome by adjusting the quantization of 
		//		the intra frames compared to the surrounding inter frames.
		
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
		
	//H26x4_ENC_INPUT_FORMAT_YUV420_planar,	// 输入格式选择 (YUV420 planar 或者YUV420 semiplanar)	
	H264_ENC_INPUT_FORMAT_YUV420_semiplanar,
		
	"-q",			/* QP for next encoded picture, [-1..51]
                              * -1 = Let rate control calculate initial QP
                              * This QP is used for all pictures if 
                              * HRD and pictureRc and mbRc are disabled
                              * If HRD is enabled it may override this QP
                              */
	"25",			// 一般图像质量	
	"-n",
	"23",	
	"-m",
	"32",			// 一般图像质量
		
	"-M",		// quarterPixelMv
	H264_PIXELMV,
		
	"-8",		// trans8x8
	"1",	
		
	"-K",		//	"-enableCabac",
	"1",
		
	//"-Z",		//	"-videoStab",
	//	"1",
		
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
	//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
		
	"-B",		//"-bitPerSecond",
	"12800000",
};

// 较高质量视频编码
const char *argv_1080P_good_video_quality[] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	
	//"--gopLength=30",
	"--gopLength=240",			// 8秒码流缓存		
	//"--gopLength=8",
	
#ifdef _HDTV_000255_
	"--videoRange=1",				// 0..1 Video signal sample range value in H.264 stream.
										//		0 - Y range in [16..235] Cb,Cr in [16..240]
										//		1 - Y,Cb,Cr range in [0..255]
#else
	"--videoRange=0",				// 0..1 Video signal sample range value in H.264 stream.
										//		0 - Y range in [16..235] Cb,Cr in [16..240]
										//		1 - Y,Cb,Cr range in [0..255]

#endif
										
	"--intraPicRate=60",		// I帧编码的间隔帧数, 2秒一帧
	
	"--hrdConformance=0",		
	//"--hrdConformance=1",		//	HDR Conformance (ANNEX C) 
										//		0=OFF, 1=ON HRD conformance.
										//	Uses standard defined model to limit bitrate variance.	
	//"--cpbSize=6000000",		// Coded Picture Buffer Size
	//"--cpbSize=14800000",		// 20170717,  增加CPB, 改善瞬时丢帧现象
										// 	Buffer size used by the HRD model.
										//		Size in bits of the coded picture buffer (CPB) used by the HRD model. 
										//		When HRD is enabled an encoded frame can’t be bigger than CPB. 
										//		By default the encoder will use the maximum allowed size for the initialized encoder level
									// 较小的cpbSize会导致帧的skip.
	
	"--picRc=1",					//    0=OFF, 1=ON Picture rate control. [1]
										//		Calculates new target QP for every frame.
	
	//"--fixedIntraQp=20",
	"--intraQpDelta=-5",		// The intra frames in H.264 video can sometimes introduce noticeable 
		//		flickering because of different prediction method. 
		//		This problem can be overcome by adjusting the quantization of 
		//		the intra frames compared to the surrounding inter frames.
		
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
		
	//H26x4_ENC_INPUT_FORMAT_YUV420_planar,	// 输入格式选择 (YUV420 planar 或者YUV420 semiplanar)	
	H264_ENC_INPUT_FORMAT_YUV420_semiplanar,
		
	"-q",			/* QP for next encoded picture, [-1..51]
                              * -1 = Let rate control calculate initial QP
                              * This QP is used for all pictures if 
                              * HRD and pictureRc and mbRc are disabled
                              * If HRD is enabled it may override this QP
                              */
	"23",			// 较高图像质量
	
	"-n",	
	"21",
	
	"-m",
	"30",			// 较高图像质量 // 20170228 阴天上午的效果不佳, 将最大QP值重新调整为改为30
		
	"-M",		// quarterPixelMv
	H264_PIXELMV,
		
	"-8",		// trans8x8
	"1",	
		
	"-K",		//	"-enableCabac",
	"1",
		
	//"-Z",		//	"-videoStab",
	//	"1",
		
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
	//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
		
	"-B",		//"-bitPerSecond",
	"13200000",			// 与其他方案码率一致
};

// 很高质量视频编码
const char *argv_1080P_high_video_quality [] = {
	"arkn141",
	"-w",
	"1920",
	"-h",
	"1080",
	
	//"--gopLength=30",
	"--gopLength=240",			// 8秒码流缓存		
	//"--gopLength=8",
	
#ifdef _HDTV_000255_
	"--videoRange=1",				// 0..1 Video signal sample range value in H.264 stream.
										//		0 - Y range in [16..235] Cb,Cr in [16..240]
										//		1 - Y,Cb,Cr range in [0..255]
#else
	"--videoRange=0",				// 0..1 Video signal sample range value in H.264 stream.
										//		0 - Y range in [16..235] Cb,Cr in [16..240]
										//		1 - Y,Cb,Cr range in [0..255]

#endif
										
	"--intraPicRate=60",		// I帧编码的间隔帧数, 2秒一帧
	
	"--hrdConformance=0",		
	//"--hrdConformance=1",		//	HDR Conformance (ANNEX C) 
										//		0=OFF, 1=ON HRD conformance.
										//	Uses standard defined model to limit bitrate variance.	
	//"--cpbSize=6000000",		// Coded Picture Buffer Size
	//"--cpbSize=14800000",		// 20170717,  增加CPB, 改善瞬时丢帧现象
										// 	Buffer size used by the HRD model.
										//		Size in bits of the coded picture buffer (CPB) used by the HRD model. 
										//		When HRD is enabled an encoded frame can’t be bigger than CPB. 
										//		By default the encoder will use the maximum allowed size for the initialized encoder level
									// 较小的cpbSize会导致帧的skip.
	
	"--picRc=1",					//    0=OFF, 1=ON Picture rate control. [1]
										//		Calculates new target QP for every frame.
	
	//"--fixedIntraQp=20",
	"--intraQpDelta=-5",		// The intra frames in H.264 video can sometimes introduce noticeable 
		//		flickering because of different prediction method. 
		//		This problem can be overcome by adjusting the quantization of 
		//		the intra frames compared to the surrounding inter frames.
		
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
		
	//H26x4_ENC_INPUT_FORMAT_YUV420_planar,	// 输入格式选择 (YUV420 planar 或者YUV420 semiplanar)	
	H264_ENC_INPUT_FORMAT_YUV420_semiplanar,
		
	"-q",			/* QP for next encoded picture, [-1..51]
                              * -1 = Let rate control calculate initial QP
                              * This QP is used for all pictures if 
                              * HRD and pictureRc and mbRc are disabled
                              * If HRD is enabled it may override this QP
                              */
	"22",			// 高清图像质量
	
	"-n",
	"20",		// Minimum QP for any picture, [0..51] 
	
	"-m",
	"29",			// 高清图像质量
		
	"-M",		// quarterPixelMv
	H264_PIXELMV,
		
	"-8",		// trans8x8
	"1",	
		
	"-K",		//	"-enableCabac",
	"1",
		
	//"-Z",		//	"-videoStab",
	//	"1",
		
	"-D",		// disableDeblocking
	"0",		//		0 = Inloop deblocking filter enabled (best quality)	
	//	"1",		// 	1 = Value 1 disables entirely the deblocking filter
		
	"-B",		//"-bitPerSecond",
	"14000000",
};

const char *argv_cvbs_ntsc[] = {
	"arkn141",
	"-w",
	"720",
	"-h",
	"240",
	"--gopLength=240",			// 8秒码流缓存
	"--videoRange=1",				// 0..1 Video signal sample range value in H.264 stream.
										//		0 - Y range in [16..235] Cb,Cr in [16..240]
										//		1 - Y,Cb,Cr range in [0..255]
	
	"--intraPicRate=60",		// I帧编码的间隔帧数, 2秒一帧

	"--hrdConformance=0",		
	//"--hrdConformance=1",		//	HDR Conformance (ANNEX C) 
										//		0=OFF, 1=ON HRD conformance.
										//	Uses standard defined model to limit bitrate variance.	

	"--picRc=1",					//    0=OFF, 1=ON Picture rate control. [1]
										//		Calculates new target QP for every frame.


	"--intraQpDelta=-5",		// The intra frames in H.264 video can sometimes introduce noticeable 
		//		flickering because of different prediction method. 
		//		This problem can be overcome by adjusting the quantization of 
		//		the intra frames compared to the surrounding inter frames.
		
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
	
	//H26x4_ENC_INPUT_FORMAT_YUV420_planar,	// 输入格式选择 (YUV420 planar 或者YUV420 semiplanar)	
	H264_ENC_INPUT_FORMAT_YUV420_semiplanar,
	
	"-q",			/* QP for next encoded picture, [-1..51]
                              * -1 = Let rate control calculate initial QP
                              * This QP is used for all pictures if 
                              * HRD and pictureRc and mbRc are disabled
                              * If HRD is enabled it may override this QP
                              */
#if HIGH_VIDEO_QUALITY
	"22",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	"25",			// 一般图像质量
#else
	"23",			// 较高图像质量
#endif
	//"25",	
	
	"-n",
	
#if HIGH_VIDEO_QUALITY
	"20",		// Minimum QP for any picture, [0..51] 
#elif NORMAL_VIDEO_QUALITY
	"23",
#else
	"21",
#endif
	//"21",		// Minimum QP for any picture, [0..51] 
	//"23",		// Minimum QP for any picture, [0..51] 
	
	"-m",
	//"36",		//
	//"45",
	//"36",
	//"40",
	//"35",		// 20170224 去机场测试(20~35), 阴天, 效果不好
	//"30",		// 20170227下午测试, 存在码流溢出,编码停顿的现象. 
	//"31",			// 	将最大QP值改为31
#if HIGH_VIDEO_QUALITY
	"29",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	"32",			// 一般图像质量
#else
	"30",			// 较高图像质量 // 20170228 阴天上午的效果不佳, 将最大QP值重新调整为改为30
#endif

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
#if HIGH_VIDEO_QUALITY
	"3200000",
#elif NORMAL_VIDEO_QUALITY
	"2800000",
#else
	"2900000",			// 与其他方案码率一致
#endif
};

const char *argv_cvbs_pal[] = {
	"arkn141",
	"-w",
	"720",
	"-h",
	"288",
	"--gopLength=240",			// 8秒码流缓存
	"--videoRange=1",				// 0..1 Video signal sample range value in H.264 stream.
										//		0 - Y range in [16..235] Cb,Cr in [16..240]
										//		1 - Y,Cb,Cr range in [0..255]
	
	"--intraPicRate=60",		// I帧编码的间隔帧数, 2秒一帧

	"--hrdConformance=0",		
	//"--hrdConformance=1",		//	HDR Conformance (ANNEX C) 
										//		0=OFF, 1=ON HRD conformance.
										//	Uses standard defined model to limit bitrate variance.	

	"--picRc=1",					//    0=OFF, 1=ON Picture rate control. [1]
										//		Calculates new target QP for every frame.


	"--intraQpDelta=-5",		// The intra frames in H.264 video can sometimes introduce noticeable 
		//		flickering because of different prediction method. 
		//		This problem can be overcome by adjusting the quantization of 
		//		the intra frames compared to the surrounding inter frames.
		
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
	
	//H26x4_ENC_INPUT_FORMAT_YUV420_planar,	// 输入格式选择 (YUV420 planar 或者YUV420 semiplanar)	
	H264_ENC_INPUT_FORMAT_YUV420_semiplanar,
	
	"-q",			/* QP for next encoded picture, [-1..51]
                              * -1 = Let rate control calculate initial QP
                              * This QP is used for all pictures if 
                              * HRD and pictureRc and mbRc are disabled
                              * If HRD is enabled it may override this QP
                              */
#if HIGH_VIDEO_QUALITY
	"22",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	"25",			// 一般图像质量
#else
	"23",			// 较高图像质量
#endif
	//"25",	
	
	"-n",
	
#if HIGH_VIDEO_QUALITY
	"20",		// Minimum QP for any picture, [0..51] 
#elif NORMAL_VIDEO_QUALITY
	"23",
#else
	"21",
#endif
	//"21",		// Minimum QP for any picture, [0..51] 
	//"23",		// Minimum QP for any picture, [0..51] 
	
	"-m",
	//"36",		//
	//"45",
	//"36",
	//"40",
	//"35",		// 20170224 去机场测试(20~35), 阴天, 效果不好
	//"30",		// 20170227下午测试, 存在码流溢出,编码停顿的现象. 
	//"31",			// 	将最大QP值改为31
#if HIGH_VIDEO_QUALITY
	"29",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	"32",			// 一般图像质量
#else
	"30",			// 较高图像质量 // 20170228 阴天上午的效果不佳, 将最大QP值重新调整为改为30
#endif

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
#if HIGH_VIDEO_QUALITY
	"3200000",
#elif NORMAL_VIDEO_QUALITY
	"2800000",
#else
	"2900000",			// 与其他方案码率一致
#endif
};

const char *argv_720X480P[] = {
	"arkn141",
	"-w",
	"720",
	"-h",
	"480",
	"--gopLength=240",			// 8秒码流缓存
	"--videoRange=1",				// 0..1 Video signal sample range value in H.264 stream.
										//		0 - Y range in [16..235] Cb,Cr in [16..240]
										//		1 - Y,Cb,Cr range in [0..255]
	
	"--intraPicRate=60",		// I帧编码的间隔帧数, 2秒一帧

	"--hrdConformance=0",		
	//"--hrdConformance=1",		//	HDR Conformance (ANNEX C) 
										//		0=OFF, 1=ON HRD conformance.
										//	Uses standard defined model to limit bitrate variance.	

	"--picRc=1",					//    0=OFF, 1=ON Picture rate control. [1]
										//		Calculates new target QP for every frame.


	"--intraQpDelta=-5",		// The intra frames in H.264 video can sometimes introduce noticeable 
		//		flickering because of different prediction method. 
		//		This problem can be overcome by adjusting the quantization of 
		//		the intra frames compared to the surrounding inter frames.
		
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
	
	//H26x4_ENC_INPUT_FORMAT_YUV420_planar,	// 输入格式选择 (YUV420 planar 或者YUV420 semiplanar)	
	H264_ENC_INPUT_FORMAT_YUV420_semiplanar,
	
	"-q",			/* QP for next encoded picture, [-1..51]
                              * -1 = Let rate control calculate initial QP
                              * This QP is used for all pictures if 
                              * HRD and pictureRc and mbRc are disabled
                              * If HRD is enabled it may override this QP
                              */
#if HIGH_VIDEO_QUALITY
	"22",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	"25",			// 一般图像质量
#else
	"23",			// 较高图像质量
#endif
	//"25",	
	
	"-n",
	
#if HIGH_VIDEO_QUALITY
	"20",		// Minimum QP for any picture, [0..51] 
#elif NORMAL_VIDEO_QUALITY
	"23",
#else
	"21",
#endif
	//"21",		// Minimum QP for any picture, [0..51] 
	//"23",		// Minimum QP for any picture, [0..51] 
	
	"-m",
	//"36",		//
	//"45",
	//"36",
	//"40",
	//"35",		// 20170224 去机场测试(20~35), 阴天, 效果不好
	//"30",		// 20170227下午测试, 存在码流溢出,编码停顿的现象. 
	//"31",			// 	将最大QP值改为31
#if HIGH_VIDEO_QUALITY
	"29",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	"32",			// 一般图像质量
#else
	"30",			// 较高图像质量 // 20170228 阴天上午的效果不佳, 将最大QP值重新调整为改为30
#endif

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
#if HIGH_VIDEO_QUALITY
	"3000000",
#elif NORMAL_VIDEO_QUALITY
	"2000000",
#else
	"2500000",			// 与其他方案码率一致
#endif
};

const char *argv_640X480P[] = {
	"arkn141",
	"-w",
	"640",
	"-h",
	"480",
	"--gopLength=240",			// 8秒码流缓存
	"--videoRange=1",				// 0..1 Video signal sample range value in H.264 stream.
										//		0 - Y range in [16..235] Cb,Cr in [16..240]
										//		1 - Y,Cb,Cr range in [0..255]
	
	"--intraPicRate=60",		// I帧编码的间隔帧数, 2秒一帧

	"--hrdConformance=0",		
	//"--hrdConformance=1",		//	HDR Conformance (ANNEX C) 
										//		0=OFF, 1=ON HRD conformance.
										//	Uses standard defined model to limit bitrate variance.	

	"--picRc=1",					//    0=OFF, 1=ON Picture rate control. [1]
										//		Calculates new target QP for every frame.


	"--intraQpDelta=-5",		// The intra frames in H.264 video can sometimes introduce noticeable 
		//		flickering because of different prediction method. 
		//		This problem can be overcome by adjusting the quantization of 
		//		the intra frames compared to the surrounding inter frames.
		
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
	
	//H26x4_ENC_INPUT_FORMAT_YUV420_planar,	// 输入格式选择 (YUV420 planar 或者YUV420 semiplanar)	
	H264_ENC_INPUT_FORMAT_YUV420_semiplanar,
	
	"-q",			/* QP for next encoded picture, [-1..51]
                              * -1 = Let rate control calculate initial QP
                              * This QP is used for all pictures if 
                              * HRD and pictureRc and mbRc are disabled
                              * If HRD is enabled it may override this QP
                              */
#if HIGH_VIDEO_QUALITY
	"22",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	"25",			// 一般图像质量
#else
	"23",			// 较高图像质量
#endif
	//"25",	
	
	"-n",
	
#if HIGH_VIDEO_QUALITY
	"20",		// Minimum QP for any picture, [0..51] 
#elif NORMAL_VIDEO_QUALITY
	"23",
#else
	"21",
#endif
	//"21",		// Minimum QP for any picture, [0..51] 
	//"23",		// Minimum QP for any picture, [0..51] 
	
	"-m",
	//"36",		//
	//"45",
	//"36",
	//"40",
	//"35",		// 20170224 去机场测试(20~35), 阴天, 效果不好
	//"30",		// 20170227下午测试, 存在码流溢出,编码停顿的现象. 
	//"31",			// 	将最大QP值改为31
#if HIGH_VIDEO_QUALITY
	"29",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	"32",			// 一般图像质量
#else
	"30",			// 较高图像质量 // 20170228 阴天上午的效果不佳, 将最大QP值重新调整为改为30
#endif

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
#if HIGH_VIDEO_QUALITY
	"2400000",
#elif NORMAL_VIDEO_QUALITY
	"1600000",
#else
	"2000000",			// 与其他方案码率一致
#endif
};

const char *argv_320X240P[] = {
	"arkn141",
	"-w",
	"320",
	"-h",
	"240",
	"--gopLength=240",			// 8秒码流缓存
	"--videoRange=1",				// 0..1 Video signal sample range value in H.264 stream.
										//		0 - Y range in [16..235] Cb,Cr in [16..240]
										//		1 - Y,Cb,Cr range in [0..255]
	
	"--intraPicRate=60",		// I帧编码的间隔帧数, 2秒一帧

	"--hrdConformance=0",		
	//"--hrdConformance=1",		//	HDR Conformance (ANNEX C) 
										//		0=OFF, 1=ON HRD conformance.
										//	Uses standard defined model to limit bitrate variance.	

	"--picRc=1",					//    0=OFF, 1=ON Picture rate control. [1]
										//		Calculates new target QP for every frame.


	"--intraQpDelta=-5",		// The intra frames in H.264 video can sometimes introduce noticeable 
		//		flickering because of different prediction method. 
		//		This problem can be overcome by adjusting the quantization of 
		//		the intra frames compared to the surrounding inter frames.
		
	H264_ENC_MAD_QP_ADJUSTMENT,		// MAD based MB QP adjustment
	
	//H26x4_ENC_INPUT_FORMAT_YUV420_planar,	// 输入格式选择 (YUV420 planar 或者YUV420 semiplanar)	
	H264_ENC_INPUT_FORMAT_YUV420_semiplanar,
	
	"-q",			/* QP for next encoded picture, [-1..51]
                              * -1 = Let rate control calculate initial QP
                              * This QP is used for all pictures if 
                              * HRD and pictureRc and mbRc are disabled
                              * If HRD is enabled it may override this QP
                              */
#if HIGH_VIDEO_QUALITY
	"22",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	"25",			// 一般图像质量
#else
	"23",			// 较高图像质量
#endif
	//"25",	
	
	"-n",
	
#if HIGH_VIDEO_QUALITY
	"20",		// Minimum QP for any picture, [0..51] 
#elif NORMAL_VIDEO_QUALITY
	"23",
#else
	"21",
#endif
	//"21",		// Minimum QP for any picture, [0..51] 
	//"23",		// Minimum QP for any picture, [0..51] 
	
	"-m",
	//"36",		//
	//"45",
	//"36",
	//"40",
	//"35",		// 20170224 去机场测试(20~35), 阴天, 效果不好
	//"30",		// 20170227下午测试, 存在码流溢出,编码停顿的现象. 
	//"31",			// 	将最大QP值改为31
#if HIGH_VIDEO_QUALITY
	"29",			// 高清图像质量
#elif NORMAL_VIDEO_QUALITY
	"32",			// 一般图像质量
#else
	"30",			// 较高图像质量 // 20170228 阴天上午的效果不佳, 将最大QP值重新调整为改为30
#endif

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
#if HIGH_VIDEO_QUALITY
	"600000",
#elif NORMAL_VIDEO_QUALITY
	"400000",
#else
	"500000",			// 与其他方案码率一致
#endif
};