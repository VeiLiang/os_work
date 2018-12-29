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

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
/* For printing and file IO */
#include <stdio.h>
#include "hardware.h"

/* For dynamic memory allocation */
#include <stdlib.h>

/* For memset, strcpy and strlen */
#include <string.h>

/* For accessing the EWL instance inside the encoder */
#include "H264Instance.h"

/* For Hantro H.264 encoder */
#include "h264encapi.h"

/* For parameter parsing */
#include "EncGetOption.h"


#include "arkn141_h264_encoder.h"
#include "xm_core.h"
#include <assert.h>

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------
              
NO_OUTPUT_WRITE: Output stream is not written to file. This should be used
                 when running performance simulations.
NO_INPUT_READ:   Input frames are not read from file. This should be used
                 when running performance simulations.
PSNR:            Enable PSNR calculation with --psnr option, only works with
                 system model

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define MAX_GOP_LEN 300
/* The maximum amount of frames for bitrate moving average calculation */
#define MOVING_AVERAGE_FRAMES    30

#define ARKN141_H264_ENCODER_ID	0x31343158	// "X141"

/* Global variables */


static const char *streamType[2] = { "BYTE_STREAM", "NAL_UNITS" };
static const char *viewMode[4] = { "H264_DOUBLE_BUFFER", "H264_SINGLE_BUFFER",
                             "MVC_INTER_VIEW_PRED", "MVC_INTER_PRED"};



typedef struct {
    i32 frame[MOVING_AVERAGE_FRAMES];
    i32 length;
    i32 count;
    i32 pos;
    i32 frameRateNumer;
    i32 frameRateDenom;
} ma_s;

typedef struct {
	u32  id;	
	/* SW/HW shared memories for output buffers */
	//EWLLinearMem_t outbufMem;
	
	// 外部提供的输入/输出资源
	H264EncIn 	encIn;
	H264EncOut 	encOut;
	
	H264EncRateCtrl	rc;
	int intraPeriodCnt;
	int codedFrameCnt;
	u32 frameCnt;
	int next;
	u32 streamSize;
	u32 bitrate;
	ma_s ma;	
	
	// 保存NAL SIZE信息
	int numNalus;		
	u32 NaluSizeBuf[2048/16 + 4*2];	// asic->sizeTblSize = (sizeof(u32) * (height+4) + 7) & (~7);
											// 最大满足2048像素高度
	
	H264EncInst 	encInst;
	
	i32 qpMin;		// 初始的qp设置值
	i32 qpMax;
	i32 bitPerSecond;
	
} arkn141_H264Encoder;

#ifdef H264_DEBUG
#define	H264_DEBUG_PRINT(fmt, args...)		printf(fmt, ## args)	
#else
#define	H264_DEBUG_PRINT(fmt, args...)		
#endif


/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void PrintNalSizes(const u32 *pNaluSizeBuf, const u8 *pOutBuf, 
        u32 strmSize, i32 byteStream);

static void PrintErrorValue(const char *errorDesc, u32 retVal);
static void MaAddFrame(ma_s *ma, i32 frameSizeBits);
static i32 Ma(ma_s *ma);


void x264_arkn141_H264SliceReady(H264EncSliceReady *slice);

/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/
void PrintNalSizes(const u32 *pNaluSizeBuf, const u8 *pOutBuf, u32 strmSize, 
        i32 byteStream)
{
    u32 nalu = 0, naluSum = 0;

    /* Step through the NALU size buffer */
    while(pNaluSizeBuf && pNaluSizeBuf[nalu] != 0) 
    {
#ifdef INTERNAL_TEST
        /* In byte stream format each NAL must start with 
         * start code: any number of leading zeros + 0x000001 */
        if (byteStream) {
            int zero_count = 0;
            const u8 *pTmp = pOutBuf + naluSum;

            /* count zeros, shall be at least 2 */
            while(*pTmp++ == 0x00)
                zero_count++;

            if(zero_count < 2 || pTmp[-1] != 0x01)
                H264_DEBUG_PRINT
                    ("Error: NAL unit %d at %d doesn't start with '00 00 01'  ",
                     nalu, naluSum);
        }
#endif

        naluSum += pNaluSizeBuf[nalu];
        H264_DEBUG_PRINT ("%d  ", pNaluSizeBuf[nalu]);
		  nalu++;
    }

#ifdef INTERNAL_TEST
    /* NAL sum must be the same as the whole frame stream size */
    if (naluSum && naluSum != strmSize)
        H264_DEBUG_PRINT("Error: NAL size sum (%d) does not match stream size",
                naluSum);
#endif
}


/*------------------------------------------------------------------------------
    PrintErrorValue
        Print return error value
------------------------------------------------------------------------------*/
void PrintErrorValue(const char *errorDesc, u32 retVal)
{
    const char * str;

    switch ((int)retVal)
    {
        case H264ENC_ERROR: str = "H264ENC_ERROR"; break;
        case H264ENC_NULL_ARGUMENT: str = "H264ENC_NULL_ARGUMENT"; break;
        case H264ENC_INVALID_ARGUMENT: str = "H264ENC_INVALID_ARGUMENT"; break;
        case H264ENC_MEMORY_ERROR: str = "H264ENC_MEMORY_ERROR"; break;
        case H264ENC_EWL_ERROR: str = "H264ENC_EWL_ERROR"; break;
        case H264ENC_EWL_MEMORY_ERROR: str = "H264ENC_EWL_MEMORY_ERROR"; break;
        case H264ENC_INVALID_STATUS: str = "H264ENC_INVALID_STATUS"; break;
        case H264ENC_OUTPUT_BUFFER_OVERFLOW: str = "H264ENC_OUTPUT_BUFFER_OVERFLOW"; break;
        case H264ENC_HW_BUS_ERROR: str = "H264ENC_HW_BUS_ERROR"; break;
        case H264ENC_HW_DATA_ERROR: str = "H264ENC_HW_DATA_ERROR"; break;
        case H264ENC_HW_TIMEOUT: str = "H264ENC_HW_TIMEOUT"; break;
        case H264ENC_HW_RESERVED: str = "H264ENC_HW_RESERVED"; break;
        case H264ENC_SYSTEM_ERROR: str = "H264ENC_SYSTEM_ERROR"; break;
        case H264ENC_INSTANCE_ERROR: str = "H264ENC_INSTANCE_ERROR"; break;
        case H264ENC_HRD_ERROR: str = "H264ENC_HRD_ERROR"; break;
        case H264ENC_HW_RESET: str = "H264ENC_HW_RESET"; break;
        default: str = "UNDEFINED";
    }

    //H264_DEBUG_PRINT("%s Return value: %s\n", errorDesc, str);
	 printf ("%s Return value: %s\n", errorDesc, str);
}



/*------------------------------------------------------------------------------
    Add new frame bits for moving average bitrate calculation
------------------------------------------------------------------------------*/
void MaAddFrame(ma_s *ma, i32 frameSizeBits)
{
    ma->frame[ma->pos++] = frameSizeBits;

    if (ma->pos == ma->length)
        ma->pos = 0;

    if (ma->count < ma->length)
        ma->count++;
}

/*------------------------------------------------------------------------------
    Calculate average bitrate of moving window
------------------------------------------------------------------------------*/
i32 Ma(ma_s *ma)
{
    i32 i;
    unsigned long long sum = 0;     /* Using 64-bits to avoid overflow */

    for (i = 0; i < ma->count; i++)
        sum += ma->frame[i];

    if (!ma->frameRateDenom)
        return 0;

    sum = sum / ma->length;

    return sum * ma->frameRateNumer / ma->frameRateDenom;
}

/*------------------------------------------------------------------------------
    Callback function called by the encoder SW after "slice ready"
    interrupt from HW. Note that this function is not necessarily called
    after every slice i.e. it is possible that two or more slices are
    completed between callbacks. The callback is not called for the last slice.
------------------------------------------------------------------------------*/
void x264_arkn141_H264SliceReady(H264EncSliceReady *slice)
{
    /* Here is possible to implement low-latency streaming by
     * sending the complete slices before the whole frame is completed. */
#if 0
    /* This is an example how to access the slices, but it will mess up prints */
    {
        i32 i;
        u8 *strmPtr;

        if (slice->slicesReadyPrev == 0)    /* New frame */
            strmPtr = (u8*)slice->pOutBuf;  /* Pointer to beginning of frame */
        else
            strmPtr = (u8*)slice->pAppData; /* Here we store the slice pointer */

        for (i = slice->slicesReadyPrev; i < slice->slicesReady; i++)
        {
            H264_DEBUG_PRINT("#%d:%p:%d\n", i, strmPtr, slice->sliceSizes[i]);
            strmPtr += slice->sliceSizes[i];
        }

        /* Store the slice pointer for next callback */
        slice->pAppData = strmPtr;
    }
#endif
}


void x264_arkn141_param_default (x264_arkn141_commandLine *cml)
{
    memset(cml, 0, sizeof(x264_arkn141_commandLine));

    cml->width = DEFAULT;
    cml->height = DEFAULT;
    cml->outputRateNumer = DEFAULT;
    cml->outputRateDenom = DEFAULT;
    cml->qpHdr = DEFAULT;
    cml->qpMin = DEFAULT;
    cml->qpMax = DEFAULT;
    cml->picRc = DEFAULT;
    cml->mbRc = DEFAULT;
    cml->cpbSize = DEFAULT;
    cml->constIntraPred = DEFAULT;
    cml->horOffsetSrc = DEFAULT;
    cml->verOffsetSrc = DEFAULT;
    cml->videoStab = DEFAULT;
    cml->gopLength = DEFAULT;

    cml->input = NULL;
    cml->output = NULL;
    cml->firstPic = 0;
    cml->lastPic = 100;
	 //cml->lastPic = 10;
    cml->inputRateNumer = 30;
    cml->inputRateDenom = 1;

    cml->lumWidthSrc = DEFAULT;
    cml->lumHeightSrc = DEFAULT;
    cml->rotation = 0;
    cml->inputFormat = 0;

    cml->bitPerSecond = 1000000;
    cml->intraQpDelta = DEFAULT;
    cml->fixedIntraQp=0;

    cml->chromaQpOffset = 2;
    cml->disableDeblocking = 0;
    cml->filterOffsetA = 0;
    cml->filterOffsetB = 0;
    cml->enableCabac = DEFAULT;
    cml->cabacInitIdc = DEFAULT;
    cml->trans8x8 = 0;
    cml->burst = 16;
    cml->bursttype = 0;
    cml->quarterPixelMv = DEFAULT;
    cml->testId = 0;
    cml->viewMode = DEFAULT;

    cml->sei = 0;
    cml->byteStream = 1;
    cml->hrdConformance = 0;
    cml->videoRange = 0;
    cml->picSkip = 0;
    cml->psnr = 0;
    cml->testParam = 0;	
}

static int arkn141_open_encoder (x264_arkn141_commandLine *cml, H264EncInst *pEnc)
{
    H264EncApiVersion apiVer;
    H264EncBuild encBuild;
	 
    H264EncRet ret;
    H264EncConfig cfg;
    H264EncCodingCtrl codingCfg;
    H264EncRateCtrl rcCfg;
    H264EncPreProcessingCfg preProcCfg;

    H264EncInst encoder;
	 
    apiVer = H264EncGetApiVersion();
    encBuild = H264EncGetBuild();

    //printf ("H.264 Encoder API version %d.%d\n", apiVer.major,
    //        apiVer.minor);
    //printf ("HW ID: 0x%08x\t SW Build: %u.%u.%u\n\n",
    //        encBuild.hwBuild, encBuild.swBuild / 1000000,
    //        (encBuild.swBuild / 1000) % 1000, encBuild.swBuild % 1000);
	 
    /* Default resolution, try parsing input file name */
    if(cml->lumWidthSrc == DEFAULT || cml->lumHeightSrc == DEFAULT)
    {
        /* No dimensions found in filename, using default QCIF */
        cml->lumWidthSrc  = 176;
        cml->lumHeightSrc = 144;
    }
	 
    /* Encoder initialization */
    if(cml->width == DEFAULT)
        cml->width = cml->lumWidthSrc;

    if(cml->height == DEFAULT)
        cml->height = cml->lumHeightSrc;
	
    /* outputRateNumer */
    if(cml->outputRateNumer == DEFAULT)
    {
        cml->outputRateNumer = cml->inputRateNumer;
    }

    /* outputRateDenom */
    if(cml->outputRateDenom == DEFAULT)
    {
        cml->outputRateDenom = cml->inputRateDenom;
    }

    if(cml->rotation)
    {
        cfg.width = cml->height;
        cfg.height = cml->width;
    }
    else
    {
        cfg.width = cml->width;
        cfg.height = cml->height;
    }

    cfg.frameRateDenom = cml->outputRateDenom;
    cfg.frameRateNum = cml->outputRateNumer;
    cfg.viewMode = H264ENC_BASE_VIEW_DOUBLE_BUFFER; /* Two buffers by default */
    if(cml->viewMode != DEFAULT)
        cfg.viewMode = (H264EncViewMode)cml->viewMode;
    if(cml->byteStream)
        cfg.streamType = H264ENC_BYTE_STREAM;
    else
        cfg.streamType = H264ENC_NAL_UNIT_STREAM;

    cfg.level = H264ENC_LEVEL_4;

    if(cml->level != DEFAULT && cml->level != 0)
        cfg.level = (H264EncLevel)cml->level;

    H264_DEBUG_PRINT
        ("Init config: size %dx%d   %d/%d fps  %s  %s  P&L %d\n",
         cfg.width, cfg.height, cfg.frameRateNum,
         cfg.frameRateDenom, streamType[cfg.streamType],
         viewMode[cfg.viewMode], cfg.level);

    if((ret = H264EncInit(&cfg, pEnc)) != H264ENC_OK)
    {
        PrintErrorValue("H264EncInit() failed.", ret);
        return -1;
    }

    encoder = *pEnc;

    /* Encoder setup: rate control */
    if((ret = H264EncGetRateCtrl(encoder, &rcCfg)) != H264ENC_OK)
    {
        PrintErrorValue("H264EncGetRateCtrl() failed.", ret);
        return -1;
    }
    else
    {
        H264_DEBUG_PRINT("Get rate control: qp %2d [%2d, %2d]  %8d bps  "
               "pic %d mb %d skip %d  hrd %d\n  cpbSize %d gopLen %d "
               "intraQpDelta %2d\n",
               rcCfg.qpHdr, rcCfg.qpMin, rcCfg.qpMax, rcCfg.bitPerSecond,
               rcCfg.pictureRc, rcCfg.mbRc, rcCfg.pictureSkip, rcCfg.hrd,
               rcCfg.hrdCpbSize, rcCfg.gopLen, rcCfg.intraQpDelta);

        if(cml->qpHdr != DEFAULT)
            rcCfg.qpHdr = cml->qpHdr;
        if(cml->qpMin != DEFAULT)
            rcCfg.qpMin = cml->qpMin;
        if(cml->qpMax != DEFAULT)
            rcCfg.qpMax = cml->qpMax;
        if(cml->picSkip != DEFAULT)
            rcCfg.pictureSkip = cml->picSkip;
        if(cml->picRc != DEFAULT)
            rcCfg.pictureRc = cml->picRc;
        if(cml->mbRc != DEFAULT)
            rcCfg.mbRc = cml->mbRc != 0 ? 1 : 0;
        if(cml->bitPerSecond != DEFAULT)
            rcCfg.bitPerSecond = cml->bitPerSecond;

        if(cml->hrdConformance != DEFAULT)
            rcCfg.hrd = cml->hrdConformance;

        if(cml->cpbSize != DEFAULT)
            rcCfg.hrdCpbSize = cml->cpbSize;

        if(cml->intraPicRate != 0)
            rcCfg.gopLen = MIN(cml->intraPicRate, MAX_GOP_LEN);

        if(cml->gopLength != DEFAULT)
            rcCfg.gopLen = cml->gopLength;
        
        if(cml->intraQpDelta != DEFAULT)
            rcCfg.intraQpDelta = cml->intraQpDelta;

        rcCfg.fixedIntraQp = cml->fixedIntraQp;
        rcCfg.mbQpAdjustment = cml->mbQpAdjustment;
                
        H264_DEBUG_PRINT("Set rate control: qp %2d [%2d, %2d] %8d bps  "
               "pic %d mb %d skip %d  hrd %d\n"
               "  cpbSize %d gopLen %d intraQpDelta %2d "
               "fixedIntraQp %2d mbQpAdjustment %d\n",
               rcCfg.qpHdr, rcCfg.qpMin, rcCfg.qpMax, rcCfg.bitPerSecond,
               rcCfg.pictureRc, rcCfg.mbRc, rcCfg.pictureSkip, rcCfg.hrd,
               rcCfg.hrdCpbSize, rcCfg.gopLen, rcCfg.intraQpDelta,
               rcCfg.fixedIntraQp, rcCfg.mbQpAdjustment);

        if((ret = H264EncSetRateCtrl(encoder, &rcCfg)) != H264ENC_OK)
        {
            PrintErrorValue("H264EncSetRateCtrl() failed.", ret);
            return -1;
        }
    }

    /* Encoder setup: coding control */
    if((ret = H264EncGetCodingCtrl(encoder, &codingCfg)) != H264ENC_OK)
    {
        PrintErrorValue("H264EncGetCodingCtrl() failed.", ret);
        return -1;
    }
    else
    {
        if(cml->mbRowPerSlice != DEFAULT)
        {
            codingCfg.sliceSize = cml->mbRowPerSlice;
        }

        if(cml->constIntraPred != DEFAULT)
        {
            if(cml->constIntraPred != 0)
                codingCfg.constrainedIntraPrediction = 1;
            else
                codingCfg.constrainedIntraPrediction = 0;
        }

        if(cml->disableDeblocking != 0)
            codingCfg.disableDeblockingFilter = 1;
        else
            codingCfg.disableDeblockingFilter = 0;

        if((cml->disableDeblocking != 1) &&
           ((cml->filterOffsetA != 0) || (cml->filterOffsetB != 0)))
            codingCfg.disableDeblockingFilter = 1;

        if(cml->enableCabac != DEFAULT)
        {
            codingCfg.enableCabac = cml->enableCabac;
            if (cml->cabacInitIdc != DEFAULT)
                codingCfg.cabacInitIdc = cml->cabacInitIdc;
        }

        codingCfg.transform8x8Mode = cml->trans8x8;

        if(cml->quarterPixelMv != DEFAULT)
            codingCfg.quarterPixelMv = cml->quarterPixelMv;

        if(cml->videoRange != DEFAULT)
        {
            if(cml->videoRange != 0)
                codingCfg.videoFullRange = 1;
            else
                codingCfg.videoFullRange = 0;
        }

        if(cml->sei)
            codingCfg.seiMessages = 1;
        else
            codingCfg.seiMessages = 0;

        codingCfg.cirStart = cml->cirStart;
        codingCfg.cirInterval = cml->cirInterval;
        codingCfg.intraSliceMap1 = cml->intraSliceMap1;
        codingCfg.intraSliceMap2 = cml->intraSliceMap2;
        codingCfg.intraSliceMap3 = cml->intraSliceMap3;
        codingCfg.intraArea.enable = cml->intraAreaEnable;
        codingCfg.intraArea.top = cml->intraAreaTop;
        codingCfg.intraArea.left = cml->intraAreaLeft;
        codingCfg.intraArea.bottom = cml->intraAreaBottom;
        codingCfg.intraArea.right = cml->intraAreaRight;
        codingCfg.roi1Area.enable = cml->roi1AreaEnable;
        codingCfg.roi1Area.top = cml->roi1AreaTop;
        codingCfg.roi1Area.left = cml->roi1AreaLeft;
        codingCfg.roi1Area.bottom = cml->roi1AreaBottom;
        codingCfg.roi1Area.right = cml->roi1AreaRight;
        codingCfg.roi2Area.enable = cml->roi2AreaEnable;
        codingCfg.roi2Area.top = cml->roi2AreaTop;
        codingCfg.roi2Area.left = cml->roi2AreaLeft;
        codingCfg.roi2Area.bottom = cml->roi2AreaBottom;
        codingCfg.roi2Area.right = cml->roi2AreaRight;
        codingCfg.roi1DeltaQp = cml->roi1DeltaQp;
        codingCfg.roi2DeltaQp = cml->roi2DeltaQp;

        H264_DEBUG_PRINT
            ("Set coding control: SEI %d Slice %5d   deblocking %d "
             "constrained intra %d video range %d\n"
             "  cabac %d  cabac initial idc %d  Adaptive 8x8 transform %d"
             "  quarter-pixel MV %d\n",
             codingCfg.seiMessages, codingCfg.sliceSize,
             codingCfg.disableDeblockingFilter,
             codingCfg.constrainedIntraPrediction, codingCfg.videoFullRange,
             codingCfg.enableCabac,
             codingCfg.cabacInitIdc, codingCfg.transform8x8Mode,
             codingCfg.quarterPixelMv );

        if (codingCfg.cirInterval)
            H264_DEBUG_PRINT("  CIR: %d %d\n",
                codingCfg.cirStart, codingCfg.cirInterval);

        if (codingCfg.intraSliceMap1 || codingCfg.intraSliceMap2 ||
            codingCfg.intraSliceMap3)
            H264_DEBUG_PRINT("  IntraSliceMap: 0x%0x 0x%0x 0x%0x\n",
                codingCfg.intraSliceMap1, codingCfg.intraSliceMap2,
                codingCfg.intraSliceMap3);

        if (codingCfg.intraArea.enable)
            H264_DEBUG_PRINT("  IntraArea: %dx%d-%dx%d\n",
                codingCfg.intraArea.left, codingCfg.intraArea.top,
                codingCfg.intraArea.right, codingCfg.intraArea.bottom);

        if (codingCfg.roi1Area.enable)
            H264_DEBUG_PRINT("  ROI 1: %d  %dx%d-%dx%d\n", codingCfg.roi1DeltaQp,
                codingCfg.roi1Area.left, codingCfg.roi1Area.top,
                codingCfg.roi1Area.right, codingCfg.roi1Area.bottom);

        if (codingCfg.roi2Area.enable)
            H264_DEBUG_PRINT("  ROI 2: %d  %dx%d-%dx%d\n", codingCfg.roi2DeltaQp,
                codingCfg.roi2Area.left, codingCfg.roi2Area.top,
                codingCfg.roi2Area.right, codingCfg.roi2Area.bottom);

        if((ret = H264EncSetCodingCtrl(encoder, &codingCfg)) != H264ENC_OK)
        {
            PrintErrorValue("H264EncSetCodingCtrl() failed.", ret);
            return -1;
        }
    }

    /* PreP setup */
    if((ret = H264EncGetPreProcessing(encoder, &preProcCfg)) != H264ENC_OK)
    {
        PrintErrorValue("H264EncGetPreProcessing() failed.", ret);
        return -1;
    }
    H264_DEBUG_PRINT
        ("Get PreP: input %4dx%d : offset %4dx%d : format %d : rotation %d "
           ": stab %d : cc %d\n",
         preProcCfg.origWidth, preProcCfg.origHeight, preProcCfg.xOffset,
         preProcCfg.yOffset, preProcCfg.inputType, preProcCfg.rotation,
         preProcCfg.videoStabilization, preProcCfg.colorConversion.type);

    preProcCfg.inputType = (H264EncPictureType)cml->inputFormat;
    preProcCfg.rotation = (H264EncPictureRotation)cml->rotation;

    preProcCfg.origWidth =
        cml->lumWidthSrc /*(cml->lumWidthSrc + 15) & (~0x0F) */ ;
    preProcCfg.origHeight = cml->lumHeightSrc;

    if(cml->horOffsetSrc != DEFAULT)
        preProcCfg.xOffset = cml->horOffsetSrc;
    if(cml->verOffsetSrc != DEFAULT)
        preProcCfg.yOffset = cml->verOffsetSrc;
    if(cml->videoStab != DEFAULT)
        preProcCfg.videoStabilization = cml->videoStab;
    if(cml->colorConversion != DEFAULT)
        preProcCfg.colorConversion.type =
                        (H264EncColorConversionType)cml->colorConversion;
    if(preProcCfg.colorConversion.type == H264ENC_RGBTOYUV_USER_DEFINED)
    {
        preProcCfg.colorConversion.coeffA = 20000;
        preProcCfg.colorConversion.coeffB = 44000;
        preProcCfg.colorConversion.coeffC = 5000;
        preProcCfg.colorConversion.coeffE = 35000;
        preProcCfg.colorConversion.coeffF = 38000;
    }

    H264_DEBUG_PRINT
        ("Set PreP: input %4dx%d : offset %4dx%d : format %d : rotation %d "
           ": stab %d : cc %d\n",
         preProcCfg.origWidth, preProcCfg.origHeight, preProcCfg.xOffset,
         preProcCfg.yOffset, preProcCfg.inputType, preProcCfg.rotation,
         preProcCfg.videoStabilization, preProcCfg.colorConversion.type);

    if((ret = H264EncSetPreProcessing(encoder, &preProcCfg)) != H264ENC_OK)
    {
        PrintErrorValue("H264EncSetPreProcessing() failed.", ret);
        return -1;
    }
    return 0;	 
}

int x264_arkn141_close_encoder (x264_arkn141_H264EncInst pEnc)
{
	arkn141_H264Encoder *arkn141_encoder;
	H264EncRet ret;
	
	arkn141_encoder = (arkn141_H264Encoder *)pEnc;
	if(arkn141_encoder == 0 || arkn141_encoder->id != ARKN141_H264_ENCODER_ID)
	{
		H264_DEBUG_PRINT ("invalid arkn141 x264 encoder\n");
		return -1;
	}
	
	//arkn141_FreeRes(arkn141_encoder);
	
	if(arkn141_encoder->encInst)
	{
		if((ret = H264EncRelease(arkn141_encoder->encInst)) != H264ENC_OK)
		{
			PrintErrorValue("H264EncRelease() failed.", ret);
		}
	}
	
	arkn141_encoder->id = 0;
	
	kernel_free ((void *)pEnc);
	
	return ret;
}

int x264_arkn141_open_encoder (x264_arkn141_commandLine *cml, x264_arkn141_H264EncInst *pEnc)
{
	arkn141_H264Encoder *arkn141_encoder;
	
	if(cml == NULL || pEnc == NULL)
	{
		H264_DEBUG_PRINT ("x264_arkn141_open_encoder failed, invalid parameter\n");
		return -1;
	}
		
	arkn141_encoder = (arkn141_H264Encoder *)kernel_calloc (sizeof(arkn141_H264Encoder), 1);
	if(arkn141_encoder == NULL)
	{
		H264_DEBUG_PRINT ("x264_arkn141_open_encoder allocate handle failed\n");
		return -1;
	}
	
	if(arkn141_open_encoder (cml, &arkn141_encoder->encInst) < 0)
	{
		x264_arkn141_close_encoder(arkn141_encoder);
		return -1;
	}
	
	// 保存初始的QP设置
	arkn141_encoder->qpMin = cml->qpMin;
	arkn141_encoder->qpMax = cml->qpMax;
	arkn141_encoder->bitPerSecond = cml->bitPerSecond;
	
	arkn141_encoder->intraPeriodCnt = 0;
	arkn141_encoder->codedFrameCnt = 0;
	arkn141_encoder->streamSize = 0;
	arkn141_encoder->next = 0;
	
	/* Set the window length for bitrate moving average calculation */
	arkn141_encoder->ma.pos = arkn141_encoder->ma.count = 0;
	arkn141_encoder->ma.frameRateNumer = cml->outputRateNumer;
	arkn141_encoder->ma.frameRateDenom = cml->outputRateDenom;
	if (cml->outputRateDenom)
		arkn141_encoder->ma.length = MAX(1, MIN(cml->outputRateNumer / cml->outputRateDenom,
                                MOVING_AVERAGE_FRAMES));
	else
		arkn141_encoder->ma.length = MOVING_AVERAGE_FRAMES;	
	
	arkn141_encoder->id = ARKN141_H264_ENCODER_ID;
	
	*pEnc = arkn141_encoder;
	
	return 0;
}

int x264_arkn141_encoder_headers (x264_arkn141_commandLine *cml, 
											 x264_arkn141_H264EncInst enc, 
											 x264_arkn141_input *input, 
											 x264_arkn141_output *output
											 )
{
	H264EncRet ret;	
	
	arkn141_H264Encoder *arkn141_encoder;
	H264EncInst encoder;
	
	arkn141_encoder = (arkn141_H264Encoder *)enc;
	if(arkn141_encoder == NULL || arkn141_encoder->encInst == NULL)
	{
		H264_DEBUG_PRINT ("invalid arkn141 x264 encoder\n");
		return -1;
	}
	if(input == NULL || output == NULL)
	{
		H264_DEBUG_PRINT ("invalid input/output buffer\n");
		return -1;
	}
	
	//if(arkn141_encoder->b_header_open)
	//	return 0;
	
		
	encoder = arkn141_encoder->encInst;
	
	arkn141_encoder->encIn.pOutBuf = input->pOutBuf;
	arkn141_encoder->encIn.busOutBuf = input->busOutBuf;
	arkn141_encoder->encIn.outBufSize = input->outBufSize;
	
	arkn141_encoder->numNalus = 0;

	/* Start stream */
	ret = H264EncStrmStart(encoder, &arkn141_encoder->encIn, &arkn141_encoder->encOut);
	if(ret != H264ENC_OK)
	{
		PrintErrorValue("H264EncStrmStart() failed.", ret);
		return -1;
	}	
	
	output->streamSize = arkn141_encoder->encOut.streamSize;
	
	//output->pNaluSizeBuf = arkn141_encoder->encOut.pNaluSizeBuf;
	//output->numNalus = arkn141_encoder->encOut.numNalus;
	memcpy (&arkn141_encoder->NaluSizeBuf[arkn141_encoder->numNalus], 
			  arkn141_encoder->encOut.pNaluSizeBuf,
			  arkn141_encoder->encOut.numNalus * sizeof(u32));
	arkn141_encoder->numNalus += arkn141_encoder->encOut.numNalus;
	output->pNaluSizeBuf = arkn141_encoder->NaluSizeBuf;
	output->numNalus = arkn141_encoder->numNalus;
	
		
	arkn141_encoder->streamSize += arkn141_encoder->encOut.streamSize;
	
	H264EncGetRateCtrl(encoder, &arkn141_encoder->rc);
	
	H264_DEBUG_PRINT("\nInput | Pic | QP | Type |   BR avg    MA(%3d) | ByteCnt (inst) |", arkn141_encoder->ma.length);
	H264_DEBUG_PRINT(" NALU sizes\n");
  	H264_DEBUG_PRINT("--------------------------------------------------------------------------------\n");
	H264_DEBUG_PRINT("      |     | %2d | HDR  |                     | %7i %6i | ", 
			 arkn141_encoder->rc.qpHdr, 
			 arkn141_encoder->streamSize, 
			 arkn141_encoder->encOut.streamSize);

	PrintNalSizes (arkn141_encoder->encOut.pNaluSizeBuf, 
						(u8 *) arkn141_encoder->encIn.pOutBuf, 
						arkn141_encoder->encOut.streamSize, cml->byteStream);
	H264_DEBUG_PRINT("\n");
	
	/* First frame is always intra with time increment = 0 */
	arkn141_encoder->encIn.codingType = H264ENC_INTRA_FRAME;
	arkn141_encoder->encIn.timeIncrement = 0;
	
	arkn141_encoder->intraPeriodCnt = cml->intraPicRate;
	
	//arkn141_encoder->b_header_open = 1;
	
	// global header信息
	if(arkn141_encoder->numNalus > 8 || arkn141_encoder->encOut.streamSize > 2048)
	{
		H264_DEBUG_PRINT ("H264EncStrmStart() global header failed.");
		return -1;
	}
	
	return arkn141_encoder->encOut.streamSize;
}

#define	_ENABLE_CACHELOAD_ADJUST_QP_
extern void XMSYS_H264CodecForceIFrame (void);


// 返回值定义
// 	-1			参数错误
//		-2			编码缓冲区溢出
//		-3			其他异常 (硬件异常，如编码器超时等)
//		> 0		编码的码流字节长度
int x264_arkn141_encoder_encode (x264_arkn141_H264EncInst enc, 
											x264_arkn141_input *input,
											x264_arkn141_output *output,
											x264_arkn141_commandLine *cml
											)
{
	arkn141_H264Encoder *arkn141_encoder;
	H264EncIn *encIn;
	H264EncOut *encOut;
	H264EncInst encoder;
	H264EncRateCtrl *rc;
	u32 bitrate = 0;
	int ret = -1;
	int enc_ret = -1;
	i32 i;
	
	arkn141_encoder = (arkn141_H264Encoder *)enc;
	if(arkn141_encoder == NULL || output == NULL || cml == NULL)
		return -1;
	encoder = arkn141_encoder->encInst;
	if(encoder == NULL)
		return -1;
	
	encIn = &arkn141_encoder->encIn;
	encOut = &arkn141_encoder->encOut;
	rc = &arkn141_encoder->rc;
	
	arkn141_encoder->numNalus = 0;
	
	if(input)
	{
			encIn->busLuma = input->busLuma;
			encIn->busChromaU = input->busChromaU;
			encIn->busChromaV = input->busChromaV;
			encIn->busLumaStab = input->busLumaStab;
			//encIn->pOutBuf = (u32 *)input->busOutBuf;
			encIn->pOutBuf = (u32 *)input->pOutBuf;
			encIn->busOutBuf = input->busOutBuf;
			encIn->outBufSize = input->outBufSize;
			
			//dma_inv_range ((unsigned int)encIn->pOutBuf, ((unsigned int)encIn->pOutBuf) + encIn->outBufSize);		
			
			// 判断是否强制I帧。
			if(input->force_i_frame)
				encIn->codingType = H264ENC_INTRA_FRAME;
			else
			{
				// 由编码器决定I帧或P帧
				/* Select frame type */
				if((cml->intraPicRate != 0) && (arkn141_encoder->intraPeriodCnt >= cml->intraPicRate))
					encIn->codingType = H264ENC_INTRA_FRAME;
				else
					encIn->codingType = H264ENC_PREDICTED_FRAME;
			}
			
			// 码率控制设置
			for (i = 0; i < MAX_BPS_ADJUST; i++)
				if (cml->bpsAdjustFrame[i] && (arkn141_encoder->codedFrameCnt == cml->bpsAdjustFrame[i]))
				{
					rc->bitPerSecond = cml->bpsAdjustBitrate[i];
					H264_DEBUG_PRINT("Adjusting bitrate target: %d\n", rc->bitPerSecond);
					if((ret = H264EncSetRateCtrl(encoder, rc)) != H264ENC_OK)
					{
						PrintErrorValue("H264EncSetRateCtrl() failed.", ret);
					}
				}
			
#ifdef _ENABLE_CACHELOAD_ADJUST_QP_
		// 20170225 根据Cache系统的负载微调qp值(一般调整max_qp)	
		// 负载较小时(0 ~ 2)恢复最大qp值, 提高视频质量
		// 负载很大时(3 ~ 9)增加最大qp值, 降低视频码流
		extern unsigned int XMSYS_GetCacheSystemBusyLevel (void);
		int do_rate_control = 1;
		unsigned int busy_level = XMSYS_GetCacheSystemBusyLevel ();
		if(busy_level <= 2)
		{
			// 恢复设定的码率
			if(rc->bitPerSecond != arkn141_encoder->bitPerSecond)
			{
				rc->bitPerSecond = arkn141_encoder->bitPerSecond;
				rc->qpMax = arkn141_encoder->qpMax;
				if(rc->qpHdr > rc->qpMax)
					rc->qpHdr = rc->qpMax;
				do_rate_control = 0;
				if((ret = H264EncSetRateCtrl(encoder, rc)) != H264ENC_OK)
				{
					//printf ("ADJUST_QP error, 2, rc->qpMax=%d, arkn141_encoder->qpMin=%d, qpMax=%d\n", rc->qpMax,
					//		  arkn141_encoder->qpMin, arkn141_encoder->qpMax);
					PrintErrorValue("H264EncSetRateCtrl() failed.", ret);
				}
				else
				{
					//XM_printf ("QP <-- restore\n");
				}
			}
		}
		//else if(busy_level >= 3)
		else if(busy_level <= 7)
		{
			// 负载较大, 减小码流
			// 3 --> 50% 
			// 4 --> 25% 
			//	5 --> 16.7%
			// 6 --> 12.5%
			// 7 --> 10%
			unsigned int new_bps = arkn141_encoder->bitPerSecond / (2 * (busy_level - 2));
			if(rc->bitPerSecond != new_bps)
			{
				rc->bitPerSecond = new_bps;
				rc->qpMax = arkn141_encoder->qpMax + busy_level * 2;
				if(rc->qpMax > 51)
					rc->qpMax = 51;
				if(rc->qpHdr > rc->qpMax)
					rc->qpHdr = rc->qpMax;
				do_rate_control = 0;
				if((ret = H264EncSetRateCtrl(encoder, rc)) != H264ENC_OK)
				{
					//printf ("ADJUST_QP error, 5, rc->qpMax=%d, arkn141_encoder->qpMax\n", rc->qpMax, arkn141_encoder->qpMax);
					//PrintErrorValue("H264EncSetRateCtrl() failed.", ret);
				}				
				else
				{
					//XM_printf ("QP --> increase\n");
				}
			}
		}
		else	// busy_level (8, 9)
		{
			// 负载很大, 减小码流至最低码流(2% ~ 1%))
			// 8 --> 2%
			// 9 --> 1%
			assert (busy_level == 8 || busy_level == 9);
			unsigned int new_bps = arkn141_encoder->bitPerSecond / (50 *(busy_level - 8 + 1));
			if(rc->bitPerSecond != new_bps)
			{
				rc->bitPerSecond = new_bps;
				rc->qpMax = 51;
				if(rc->qpHdr > rc->qpMax)
					rc->qpHdr = rc->qpMax;
				do_rate_control = 0;
				if((ret = H264EncSetRateCtrl(encoder, rc)) != H264ENC_OK)
				{
					//printf ("ADJUST_QP error, 5, rc->qpMax=%d, arkn141_encoder->qpMax\n", rc->qpMax, arkn141_encoder->qpMax);
					//PrintErrorValue("H264EncSetRateCtrl() failed.", ret);
				}				
				else
				{
					XM_printf ("bl (%d) --> bps (%d)\n", busy_level, new_bps);
				}
			}
		}
#endif
		
		
		if (input->user_data && input->user_data_size)
		{
			H264EncSetSeiUserData(encoder, input->user_data, input->user_data_size);
		}
		
		
		ret = H264EncStrmEncode(encoder, encIn, encOut, &x264_arkn141_H264SliceReady, NULL);
		
		H264EncGetRateCtrl(encoder, rc);
		
		// 累加总的码流长度
		arkn141_encoder->streamSize += encOut->streamSize;
		MaAddFrame(&arkn141_encoder->ma, encOut->streamSize*8);
		
		// 计算码率
		if((arkn141_encoder->frameCnt+1) && cml->outputRateDenom)
		{
			/* Using 64-bits to avoid overflow */
			unsigned long long tmp = arkn141_encoder->streamSize / (arkn141_encoder->frameCnt+1);
			tmp *= (u32) cml->outputRateNumer;

			bitrate = (u32) (8 * (tmp / (u32) cml->outputRateDenom));
		}
	
		switch (ret)
		{
			case H264ENC_FRAME_READY:
			/*
            printf ("%5i | %3i | %2i | %s | %9u %9u | %7i %6i | ",
                arkn141_encoder->next, arkn141_encoder->frameCnt, rc->qpHdr, 
                encOut->codingType == H264ENC_INTRA_FRAME ? " I  " :
                encOut->codingType == H264ENC_PREDICTED_FRAME ? " P  " : "skip",
                bitrate, Ma(&arkn141_encoder->ma), arkn141_encoder->streamSize, encOut->streamSize);
				PrintNalSizes(encOut->pNaluSizeBuf, (u8 *) encIn->pOutBuf,
                encOut->streamSize, cml->byteStream);
				printf("\n");
			*/
				
				{
					//extern unsigned int XM_GetTickCount (void);
					static unsigned int start_ticket = 0;
					
					if(start_ticket == 0)
						start_ticket = XM_GetTickCount();
					if(encOut->codingType == H264ENC_NOTCODED_FRAME)
					{
						static int skip_count = 0;
						skip_count ++;
						printf ("skip %d, avg = %d\n", skip_count, skip_count * 1000 * 10 / (XM_GetTickCount() - start_ticket) );
					}
				}

				/*
				WriteStrm(fout, outbufMem.virtualAddress, encOut.streamSize, 0);

				if(cml->byteStream == 0)
					WriteNalSizesToFile(nal_sizes_file, encOut.pNaluSizeBuf, encOut.numNalus);

				WriteMotionVectors(fmv, encOut.motionVectors, frameCnt,
                                cml->width, cml->height);
				*/

            if (input->user_data && input->user_data_size)
            {
                /* We want the user data to be written only once so
                 * we disable the user data and free the memory after
                 * first frame has been encoded. */
                H264EncSetSeiUserData(encoder, NULL, 0);
              //  free(pUserData);
              //  pUserData = NULL;
            }

				encIn->timeIncrement = cml->outputRateDenom;
		
				arkn141_encoder->frameCnt++;
		
				if (encOut->codingType == H264ENC_INTRA_FRAME)
					arkn141_encoder->intraPeriodCnt = 0;
		
				if (encOut->codingType != H264ENC_NOTCODED_FRAME) 
				{
					arkn141_encoder->intraPeriodCnt++; 
					arkn141_encoder->codedFrameCnt++;
				}
				
				enc_ret = encOut->streamSize;
            break;

			case H264ENC_OUTPUT_BUFFER_OVERFLOW:
            H264_DEBUG_PRINT("%5i | %3i | %2i | %s | %9u %9u | %7i %6i | \n",
                arkn141_encoder->next, arkn141_encoder->frameCnt, rc->qpHdr, "lost",
                bitrate, Ma(&arkn141_encoder->ma), arkn141_encoder->streamSize, encOut->streamSize);
				enc_ret = -2;
				// 继续, 下帧强制I帧编码
				XMSYS_H264CodecForceIFrame ();		
				enc_ret = 0;
            break;

			default:
				PrintErrorValue("H264EncStrmEncode() failed.", ret);
				/* For debugging, can be removed */
				//WriteStrm(fout, outbufMem.virtualAddress, encOut.streamSize, 0);
				/* We try to continue encoding the next frame */
				enc_ret = -3;
				break;
 		}

	}
	else
	{
		// 结束编码
		/* End stream */
		ret = H264EncStrmEnd (encoder, encIn, encOut);
		if(ret != H264ENC_OK)
		{
			PrintErrorValue("H264EncStrmEnd() failed.", ret);
			enc_ret = -3;
		}
		else
		{
			arkn141_encoder->streamSize += encOut->streamSize;
			H264_DEBUG_PRINT("      |     |    | END  |                     | %7i %6i | ",
					arkn141_encoder->streamSize, 
					encOut->streamSize);
			PrintNalSizes(encOut->pNaluSizeBuf, (u8 *) encIn->pOutBuf,
					encOut->streamSize, cml->byteStream);
			H264_DEBUG_PRINT("\n");

			/*
			WriteStrm(fout, outbufMem.virtualAddress, encOut.streamSize, 0);

			if(cml->byteStream == 0)
			{
				WriteNalSizesToFile(nal_sizes_file, encOut.pNaluSizeBuf,
							encOut.numNalus);
			}
			*/
				
			
			enc_ret = encOut->streamSize;
		}

		H264_DEBUG_PRINT("\nBitrate target %d bps, actual %d bps (%d%%).\n",
				rc->bitPerSecond, bitrate,
				(rc->bitPerSecond) ? bitrate*100/rc->bitPerSecond : 0);
		H264_DEBUG_PRINT("Total of %d frames processed, %d frames encoded, %d bytes.\n",
				arkn141_encoder->frameCnt, arkn141_encoder->codedFrameCnt, arkn141_encoder->streamSize);	
	}
	
	if(enc_ret > 0 && output)
	{
		output->b_keyframe = (encOut->codingType == H264ENC_INTRA_FRAME) ? 1 : 0;
		output->streamSize = encOut->streamSize;
		// output->pNaluSizeBuf = encOut->pNaluSizeBuf;
		// output->numNalus = encOut->numNalus;
		memcpy (&arkn141_encoder->NaluSizeBuf[arkn141_encoder->numNalus], 
			  encOut->pNaluSizeBuf,
			  encOut->numNalus * sizeof(u32));
		arkn141_encoder->numNalus += encOut->numNalus;
		output->pNaluSizeBuf = arkn141_encoder->NaluSizeBuf;
		output->numNalus = arkn141_encoder->numNalus;	
	}
	
	return enc_ret;
}
									
static const option_s option[] = {
    {"help", 'H'},
    {"firstPic", 'a', 1},
    {"lastPic", 'b', 1},
    {"width", 'x', 1},
    {"height", 'y', 1},
    {"lumWidthSrc", 'w', 1},
    {"lumHeightSrc", 'h', 1},
    {"horOffsetSrc", 'X', 1},
    {"verOffsetSrc", 'Y', 1},
    {"outputRateNumer", 'f', 1},
    {"outputRateDenom", 'F', 1},
    {"inputRateNumer", 'j', 1},
    {"inputRateDenom", 'J', 1},
    {"inputFormat", 'l', 1},        /* Input image format */
    {"colorConversion", 'O', 1},    /* RGB to YCbCr conversion type */
    {"inputMvc", '9', 1},
    {"input", 'i', 1},              /* "input" must be after "inputFormat" */
    {"output", 'o', 1},
    {"videoRange", 'k', 1},
    {"rotation", 'r', 1},           /* Input image rotation */
    {"intraPicRate", 'I', 1},
    {"constIntraPred", 'T', 1},
    {"disableDeblocking", 'D', 1},
    {"filterOffsetA", 'W', 1},
    {"filterOffsetB", 'E', 1},

    {"trans8x8", '8', 1},           /* adaptive 4x4/8x8 transform */
    {"enableCabac", 'K', 1},
    {"cabacInitIdc", 'p', 1},
    {"mbRowPerSlice", 'V', 1},

    {"bitPerSecond", 'B', 1},
    {"picRc", 'U', 1},
    {"mbRc", 'u', 1},
    {"picSkip", 's', 1},            /* Frame skipping */
    {"gopLength", 'g', 1},          /* group of pictures length */
    {"qpMin", 'n', 1},              /* Minimum frame header qp */
    {"qpMax", 'm', 1},              /* Maximum frame header qp */
    {"qpHdr", 'q', 1},              /* Defaul qp */
    {"chromaQpOffset", 'Q', 1},     /* Chroma qp index offset */
    {"hrdConformance", 'C', 1},     /* HDR Conformance (ANNEX C) */
    {"cpbSize", 'c', 1},            /* Coded Picture Buffer Size */
    {"intraQpDelta", 'A', 1},       /* QP adjustment for intra frames */
    {"fixedIntraQp", 'G', 1},       /* Fixed QP for all intra frames */
    
    {"userData", 'z', 1},           /* SEI User data file */
    {"level", 'L', 1},              /* Level * 10  (ANNEX A) */
    {"byteStream", 'R', 1},         /* Byte stream format (ANNEX B) */
    {"sei", 'S', 1},                /* SEI messages */
    {"videoStab", 'Z', 1},          /* video stabilization */
    {"bpsAdjust", '1', 1},          /* Setting bitrate on the fly */
    {"mbQpAdjustment", '2', 1},     /* MAD based MB QP adjustment */
    {"mvOutput", '3', 1},           /* MV output in mv.txt */

    {"testId", 'e', 1},
    {"burstSize", 'N', 1},
    {"burstType", 't', 1},
    {"quarterPixelMv", 'M', 1},
    {"trigger", 'P', 1},
    {"psnr", 'd', 0},
    {"testParam", 'v', 1},

    /* Only long option can be used for all the following parameters because
     * we have no more letters to use. All shortOpt=0 will be identified by
     * long option. */
    {"cir", '0', 1},
    {"intraSliceMap1", '0', 1},
    {"intraSliceMap2", '0', 1},
    {"intraSliceMap3", '0', 1},
    {"intraArea", '0', 1},
    {"roi1Area", '0', 1},
    {"roi2Area", '0', 1},
    {"roi1DeltaQp", '0', 1},
    {"roi2DeltaQp", '0', 1},
    {"viewMode", '0', 1},

    {0, 0, 0}                       /* End of options */
};

extern int ParseDelim(char *optArg, char delim);

int x264_arkn141_parse_param (x264_arkn141_commandLine *cml, char **argv, int argc)
{
	i32 ret, i;
	char *optArg;
	argument_s argument;
	i32 status = 0;
	i32 bpsAdjustCount = 0;
	
	memset(cml, 0, sizeof(x264_arkn141_commandLine));
	
	cml->width = DEFAULT;
	cml->height = DEFAULT;
	cml->outputRateNumer = DEFAULT;
	cml->outputRateDenom = DEFAULT;
	cml->qpHdr = DEFAULT;
	cml->qpMin = DEFAULT;
	cml->qpMax = DEFAULT;
	cml->picRc = DEFAULT;
	cml->mbRc = DEFAULT;
	cml->cpbSize = DEFAULT;
	cml->constIntraPred = DEFAULT;
	cml->horOffsetSrc = DEFAULT;
	cml->verOffsetSrc = DEFAULT;
	cml->videoStab = DEFAULT;
	cml->gopLength = DEFAULT;
	
	cml->input = NULL;
	cml->output = NULL;
	cml->firstPic = 0;
	cml->lastPic = 100;
	//cml->lastPic = 10;
	cml->inputRateNumer = 30;
	cml->inputRateDenom = 1;
	
	cml->lumWidthSrc = DEFAULT;
	cml->lumHeightSrc = DEFAULT;
	cml->rotation = 0;
	cml->inputFormat = 0;
	
	cml->bitPerSecond = 1000000;
	cml->intraQpDelta = DEFAULT;
	cml->fixedIntraQp=0;
	
	cml->chromaQpOffset = 2;
	cml->disableDeblocking = 0;
	cml->filterOffsetA = 0;
	cml->filterOffsetB = 0;
	cml->enableCabac = DEFAULT;
	cml->cabacInitIdc = DEFAULT;
	cml->trans8x8 = 0;
	cml->burst = 16;
	cml->bursttype = 0;
	cml->quarterPixelMv = DEFAULT;
	cml->testId = 0;
	cml->viewMode = DEFAULT;
	
	cml->sei = 0;
	cml->byteStream = 1;
	cml->hrdConformance = 0;
	cml->videoRange = 0;
	cml->picSkip = 0;
	cml->psnr = 0;
	cml->testParam = 0;
	
	argument.optCnt = 1;
	while((ret = EncGetOption(argc, argv, (option_s *)option, &argument)) != -1)
	{
		if(ret == -2)
		{
			status = 1;
		}
		optArg = argument.optArg;
		switch (argument.shortOpt)
		{
		case 'i':
			cml->input = optArg;
			break;
		case '9':
			cml->inputMvc = optArg;
			break;
		case 'o':
			cml->output = optArg;
			break;
		case 'z':
			cml->userData = optArg;
			break;
		case 'a':
			cml->firstPic = atoi(optArg);
			break;
		case 'b':
			cml->lastPic = atoi(optArg);
			break;
		case 'x':
			cml->width = atoi(optArg);
			break;
		case 'y':
			cml->height = atoi(optArg);
			break;
		case 'w':
			cml->lumWidthSrc = atoi(optArg);
			break;
		case 'h':
			cml->lumHeightSrc = atoi(optArg);
			break;
		case 'X':
			cml->horOffsetSrc = atoi(optArg);
			break;
		case 'Y':
			cml->verOffsetSrc = atoi(optArg);
			break;
		case 'f':
			cml->outputRateNumer = atoi(optArg);
			break;
		case 'F':
			cml->outputRateDenom = atoi(optArg);
			break;
		case 'j':
			cml->inputRateNumer = atoi(optArg);
			break;
		case 'J':
			cml->inputRateDenom = atoi(optArg);
			break;
		case 'T':
			cml->constIntraPred = atoi(optArg);
			break;
		case 'D':
			cml->disableDeblocking = atoi(optArg);
			break;
		case 'W':
			cml->filterOffsetA = atoi(optArg);
			break;
		case 'E':
			cml->filterOffsetB = atoi(optArg);
			break;
		case '8':
			cml->trans8x8 = atoi(optArg);
			break;
		case 'K':
			cml->enableCabac = atoi(optArg);
			break;
		case 'p':
			cml->cabacInitIdc = atoi(optArg);
			break;
		case 'I':
			cml->intraPicRate = atoi(optArg);
			break;
		case 'N':
			cml->burst = atoi(optArg);
			break;
		case 't':
			cml->bursttype = atoi(optArg);
			break;
		case 'q':
			cml->qpHdr = atoi(optArg);
			break;
		case 'n':
			cml->qpMin = atoi(optArg);
			break;
		case 'm':
			cml->qpMax = atoi(optArg);
			break;
		case 'B':
			cml->bitPerSecond = atoi(optArg);
			break;
		case 'U':
			cml->picRc = atoi(optArg);
			break;
		case 'u':
			cml->mbRc = atoi(optArg);
			break;
		case 's':
			cml->picSkip = atoi(optArg);
			break;
		case 'r':
			cml->rotation = atoi(optArg);
			break;
		case 'l':
			cml->inputFormat = atoi(optArg);
			break;
		case 'O':
			cml->colorConversion = atoi(optArg);
			break;
		case 'Q':
			cml->chromaQpOffset = atoi(optArg);
			break;
		case 'V':
			cml->mbRowPerSlice = atoi(optArg);
			break;
		case 'k':
			cml->videoRange = atoi(optArg);
			break;
		case 'L':
			cml->level = atoi(optArg);
			break;
		case 'C':
			cml->hrdConformance = atoi(optArg);
			break;
		case 'c':
			cml->cpbSize = atoi(optArg);
			break;
		case 'e':
			cml->testId = atoi(optArg);
			break;
		case 'S':
			cml->sei = atoi(optArg);
			break;
		case 'M':
			cml->quarterPixelMv = atoi(optArg);
			break;
		case 'R':
			cml->byteStream = atoi(optArg);
			break;
		case 'Z':
			cml->videoStab = atoi(optArg);
			break;
		case 'g':
			cml->gopLength = atoi(optArg);
			break;
		case 'A':
			cml->intraQpDelta = atoi(optArg);
			break;
		case 'G':
			cml->fixedIntraQp = atoi(optArg);
			break;
		case '1':
			if (bpsAdjustCount == MAX_BPS_ADJUST)
				break;
			/* Argument must be "xx:yy", replace ':' with 0 */
			if ((i = ParseDelim(optArg, ':')) == -1) break;
			/* xx is frame number */
			cml->bpsAdjustFrame[bpsAdjustCount] = atoi(optArg);
			/* yy is new target bitrate */
			cml->bpsAdjustBitrate[bpsAdjustCount] = atoi(optArg+i+1);
			bpsAdjustCount++;
			break;
		case '2':
			cml->mbQpAdjustment = atoi(optArg);
			break;
		case 'v':
			cml->testParam = atoi(optArg);
			break;
		case 'd':
			cml->psnr = 1;
			break;
		case '3':
			cml->mvOutput = atoi(optArg);
			break;
		case '0':
			/* Check long option */
			if (strcmp(argument.longOpt, "intraSliceMap1") == 0)
				cml->intraSliceMap1 = strtoul(optArg, NULL, 16);
			
			if (strcmp(argument.longOpt, "intraSliceMap2") == 0)
				cml->intraSliceMap2 = strtoul(optArg, NULL, 16);
			
			if (strcmp(argument.longOpt, "intraSliceMap3") == 0)
				cml->intraSliceMap3 = strtoul(optArg, NULL, 16);
			
			if (strcmp(argument.longOpt, "roi1DeltaQp") == 0)
				cml->roi1AreaEnable = cml->roi1DeltaQp = atoi(optArg);
			
			if (strcmp(argument.longOpt, "roi2DeltaQp") == 0)
				cml->roi2AreaEnable = cml->roi2DeltaQp = atoi(optArg);
			
			if (strcmp(argument.longOpt, "viewMode") == 0)
				cml->viewMode = atoi(optArg);
			
			if (strcmp(argument.longOpt, "cir") == 0)
			{
				/* Argument must be "xx:yy", replace ':' with 0 */
				if ((i = ParseDelim(optArg, ':')) == -1) break;
				/* xx is cir start */
				cml->cirStart = atoi(optArg);
				/* yy is cir interval */
				cml->cirInterval = atoi(optArg+i+1);
			}
			
			if (strcmp(argument.longOpt, "intraArea") == 0)
			{
			/* Argument must be "xx:yy:XX:YY".
				* xx is left coordinate, replace first ':' with 0 */
				if ((i = ParseDelim(optArg, ':')) == -1) break;
				cml->intraAreaLeft = atoi(optArg);
				/* yy is top coordinate */
				optArg += i+1;
				if ((i = ParseDelim(optArg, ':')) == -1) break;
				cml->intraAreaTop = atoi(optArg);
				/* XX is right coordinate */
				optArg += i+1;
				if ((i = ParseDelim(optArg, ':')) == -1) break;
				cml->intraAreaRight = atoi(optArg);
				/* YY is bottom coordinate */
				optArg += i+1;
				cml->intraAreaBottom = atoi(optArg);
				cml->intraAreaEnable = 1;
			}
			
			if (strcmp(argument.longOpt, "roi1Area") == 0)
			{
			/* Argument must be "xx:yy:XX:YY".
				* xx is left coordinate, replace first ':' with 0 */
				if ((i = ParseDelim(optArg, ':')) == -1) break;
				cml->roi1AreaLeft = atoi(optArg);
				/* yy is top coordinate */
				optArg += i+1;
				if ((i = ParseDelim(optArg, ':')) == -1) break;
				cml->roi1AreaTop = atoi(optArg);
				/* XX is right coordinate */
				optArg += i+1;
				if ((i = ParseDelim(optArg, ':')) == -1) break;
				cml->roi1AreaRight = atoi(optArg);
				/* YY is bottom coordinate */
				optArg += i+1;
				cml->roi1AreaBottom = atoi(optArg);
				cml->roi1AreaEnable = 1;
			}
			
			if (strcmp(argument.longOpt, "roi2Area") == 0)
			{
			/* Argument must be "xx:yy:XX:YY".
				* xx is left coordinate, replace first ':' with 0 */
				if ((i = ParseDelim(optArg, ':')) == -1) break;
				cml->roi2AreaLeft = atoi(optArg);
				/* yy is top coordinate */
				optArg += i+1;
				if ((i = ParseDelim(optArg, ':')) == -1) break;
				cml->roi2AreaTop = atoi(optArg);
				/* XX is right coordinate */
				optArg += i+1;
				if ((i = ParseDelim(optArg, ':')) == -1) break;
				cml->roi2AreaRight = atoi(optArg);
				/* YY is bottom coordinate */
				optArg += i+1;
				cml->roi2AreaBottom = atoi(optArg);
				cml->roi2AreaEnable = 1;
			}
			
			break;
		default:
			break;
        }
    }
	 
    return status;		
}
