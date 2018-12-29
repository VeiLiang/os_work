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
#ifndef _ARKN141_H264_ENCODER_H_
#define _ARKN141_H264_ENCODER_H_

#include "basetype.h"
#include "h264encapi.h"

typedef long long int	i64;

typedef const void *x264_arkn141_H264EncInst;

typedef int (*x264_arkn141_frame_lock)  (void *user_data);
typedef int (*x264_arkn141_frame_unlock)(void *user_data);

/*--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define DEFAULT -100
#define MAX_BPS_ADJUST 20

/* Structure for command line options */
typedef struct
{
    char *input;
    char *inputMvc;
    char *output;
    char *userData;
    i32 firstPic;
    i32 lastPic;
    i32 width;
    i32 height;
    i32 lumWidthSrc;
    i32 lumHeightSrc;
    i32 horOffsetSrc;
    i32 verOffsetSrc;
    i32 outputRateNumer;
    i32 outputRateDenom;
    i32 inputRateNumer;
    i32 inputRateDenom;
    i32 level;
    i32 hrdConformance;
    i32 cpbSize;
    i32 intraPicRate;
    i32 constIntraPred;
    i32 disableDeblocking;
    i32 mbRowPerSlice;
    i32 qpHdr;
    i32 qpMin;
    i32 qpMax;
    i32 bitPerSecond;
    i32 picRc;
    i32 mbRc;
    i32 picSkip;
    i32 rotation;
    i32 inputFormat;
    i32 colorConversion;
    i32 videoBufferSize;
    i32 videoRange;
    i32 chromaQpOffset;
    i32 filterOffsetA;
    i32 filterOffsetB;
    i32 trans8x8;
    i32 enableCabac;
    i32 cabacInitIdc;
    i32 testId;
    i32 burst;
    i32 bursttype;
    i32 quarterPixelMv;
    i32 sei;
    i32 byteStream;
    i32 videoStab;
    i32 gopLength;
    i32 intraQpDelta;
    i32 fixedIntraQp;    
    i32 mbQpAdjustment;
    i32 testParam;
    i32 mvOutput;
    i32 mvPredictor;
    i32 cirStart;
    i32 cirInterval;
    i32 intraSliceMap1;
    i32 intraSliceMap2;
    i32 intraSliceMap3;
    i32 intraAreaEnable;
    i32 intraAreaTop;
    i32 intraAreaLeft;
    i32 intraAreaBottom;
    i32 intraAreaRight;
    i32 roi1AreaEnable;
    i32 roi2AreaEnable;
    i32 roi1AreaTop;
    i32 roi1AreaLeft;
    i32 roi1AreaBottom;
    i32 roi1AreaRight;
    i32 roi2AreaTop;
    i32 roi2AreaLeft;
    i32 roi2AreaBottom;
    i32 roi2AreaRight;
    i32 roi1DeltaQp;
    i32 roi2DeltaQp;
    i32 viewMode;
    i32 psnr;
    i32 bpsAdjustFrame[MAX_BPS_ADJUST];
    i32 bpsAdjustBitrate[MAX_BPS_ADJUST];
} x264_arkn141_commandLine;

typedef struct {
	u32 		busLuma;         /* Bus address for input picture
                              * planar format: luminance component
                              * semiplanar format: luminance component 
                              * interleaved format: whole picture
                              */
	u32 		busChromaU;      /* Bus address for input chrominance
                              * planar format: cb component
                              * semiplanar format: both chrominance
                              * interleaved format: not used
                              */
	u32 		busChromaV;      /* Bus address for input chrominance
                              * planar format: cr component
                              * semiplanar format: not used
                              * interleaved format: not used
                              */
	i64 		i_pts;
	
	u32		force_i_frame;		// 1 ǿ��I֡ 0 �ɱ���������֡����
	
	void *	pOutBuf;
 	u32		busOutBuf;       /* Bus address of output stream buffer */
	u32		outBufSize;      /* Size of output stream buffer in bytes */
	
	u32		busLumaStab;     /* bus address of next picture to stabilize (luminance) */
	
	// H264�����������Ҫ������֡��������ֹ����֡��Դ�ͷ�
	void *							user_data;	// ������֡�������û�private data
	u32								user_data_size;		// �û�˽�������ֽڳ���
	//x264_arkn141_frame_lock 	lock;			//	��user_data��ʾ����Դ����
	//x264_arkn141_frame_unlock	unlock;		//	��user_data��ʾ����Դ����
	
} x264_arkn141_input;

typedef struct {
	u32  		b_keyframe;			// ����֡�Ƿ��ǹؼ�֡
	u32		streamSize;      /* Size of output stream in bytes */
	i8	   	*motionVectors;   /* One pixel motion vector x and y and corresponding
                                SAD value for every macroblock.
                                Format: mb0x mb0y mb0sadMsb mb0sadLsb mb1x .. */
	u32		*pNaluSizeBuf;   /* Output buffer for NAL unit sizes
                              * pNaluSizeBuf[0] = NALU 0 size in bytes
                              * pNaluSizeBuf[1] = NALU 1 size in bytes
                              * etc
                              * Zero value is written after last NALU.
                              */
	u32 		numNalus;        /* Amount of NAL units */
	
} x264_arkn141_output;

void x264_arkn141_param_default (x264_arkn141_commandLine *cml);

int x264_arkn141_open_encoder (x264_arkn141_commandLine *cml, x264_arkn141_H264EncInst *pEnc);

int x264_arkn141_close_encoder (x264_arkn141_H264EncInst pEnc);

int x264_arkn141_parse_param (x264_arkn141_commandLine *cml, char **x264_argv, int x264_argc);

int x264_arkn141_encoder_headers (x264_arkn141_commandLine *cml, 
											 x264_arkn141_H264EncInst enc, 
											 x264_arkn141_input *input, 
											 x264_arkn141_output *output
											 );
// input == NULL ��ʾ���������
// ����ֵ����
// 	-1			��������
//		-2			���뻺�������
//		-3			�����쳣 (Ӳ���쳣�����������ʱ��)
//		> 0		����������ֽڳ���
int x264_arkn141_encoder_encode (x264_arkn141_H264EncInst enc, 
											x264_arkn141_input *input,
											x264_arkn141_output *output,
											x264_arkn141_commandLine *cml
											);

#endif  // _ARKN141_H264_ENCODER_H_

