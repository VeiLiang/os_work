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
--  Abstract : Jpeg Encoder testbench
--
------------------------------------------------------------------------------*/

/* For JpegParameter parsing */
#include "EncGetOption.h"
#include "JpegEncTestBench.h"

/* For SW/HW shared memory allocation */
#include "ewl.h"

/* For accessing the EWL instance inside the encoder */
#include "EncJpegInstance.h"

/* For compiler flags, test data, debug and tracing */
#include "enccommon.h"

/* For Hantro Jpeg encoder */
#include "jpegencapi.h"

/* For printing and file IO */
#include <stdio.h>

/* For dynamic memory allocation */
#include <stdlib.h>

/* For memset, strcpy and strlen */
#include <string.h>

#include <assert.h>
#include <xm_type.h>

#include "arkn141_jpeg_codec.h"
#include "xm_core.h"

void dma_inv_range (UINT32 ulStart, UINT32 ulEnd);
void dma_flush_range(UINT32 ulStart, UINT32 ulEnd);
unsigned int vaddr_to_page_addr (unsigned int addr);



#define USER_DEFINED_QTABLE 10
static u32 comLen = 7;
static u8 comment[8] = "ARKN141";

/*--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

void jpeg_arkn141_param_default (commandLine_s *cml)
{
    memset(cml, 0, sizeof(commandLine_s));
    cml->firstPic = 0;
	 cml->lastPic = 10;
    cml->lumWidthSrc = DEFAULT;
    cml->lumHeightSrc = DEFAULT;
    cml->width = DEFAULT;
    cml->height = DEFAULT;
    cml->horOffsetSrc = 0;
    cml->verOffsetSrc = 0;
    cml->qLevel = 9;
    cml->restartInterval = 0;
    cml->thumbnail = 0;
    cml->widthThumb = 32;
    cml->heightThumb = 32;
    cml->frameType = 0;
    cml->colorConversion = 0;
    cml->rotation = 0;
    cml->partialCoding = 0;
    cml->codingMode = 0;
    cml->markerType = 0;
    cml->unitsType = 0;
    cml->xdensity = 1;
    cml->ydensity = 1;
    cml->write = 1;
    cml->comLength = 0;	 
}

typedef struct {
	JpegEncInst encoder;
	JpegEncIn encIn;
	JpegEncOut encOut;
	JpegEncCfg cfg;
	 
	i32 slice;
	i32 sliceRows;
	i32 widthSrc;
	i32 heightSrc;
	 
	EWLLinearMem_t pictureMem;
} arkn141_jpeg_encoder;




/*------------------------------------------------------------------------------

    _JpegOpenEncoder

------------------------------------------------------------------------------*/
static int _JpegOpenEncoder(commandLine_s * cml, JpegEncInst * pEnc, JpegEncCfg *cfg)
{
    JpegEncRet ret;

    /* An example of user defined quantization table */
    const u8 qTable[64] = {1, 1, 1, 1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1, 1, 1, 1};


    /* Default resolution, try parsing input file name */
    if(cml->lumWidthSrc == DEFAULT || cml->lumHeightSrc == DEFAULT)
    {
        {
            /* No dimensions found in filename, using default QCIF */
            cml->lumWidthSrc = 176;
            cml->lumHeightSrc = 144;
        }
    }

    /* Encoder initialization */
    if(cml->width == DEFAULT)
        cml->width = cml->lumWidthSrc;

    if(cml->height == DEFAULT)
        cml->height = cml->lumHeightSrc;

    cfg->rotation = (JpegEncPictureRotation)cml->rotation;
    cfg->inputWidth = (cml->lumWidthSrc + 15) & (~15);   /* API limitation */
    if (cfg->inputWidth != (u32)cml->lumWidthSrc)
        printf( "Warning: Input width must be multiple of 16!\n");
    cfg->inputHeight = cml->lumHeightSrc;

    if(cfg->rotation)
    {
        /* full */
        cfg->xOffset = cml->verOffsetSrc;
        cfg->yOffset = cml->horOffsetSrc;

        cfg->codingWidth = cml->height;
        cfg->codingHeight = cml->width;
        cfg->xDensity = cml->ydensity;
        cfg->yDensity = cml->xdensity;	
    }
    else
    {
        /* full */
        cfg->xOffset = cml->horOffsetSrc;
        cfg->yOffset = cml->verOffsetSrc;

        cfg->codingWidth = cml->width;
        cfg->codingHeight = cml->height;
        cfg->xDensity = cml->xdensity;
        cfg->yDensity = cml->ydensity;
    }

    if (cml->qLevel == USER_DEFINED_QTABLE)
    {
        cfg->qTableLuma = qTable;
        cfg->qTableChroma = qTable;
    }
    else
        cfg->qLevel = cml->qLevel;

    cfg->restartInterval = cml->restartInterval;
    cfg->codingType = (JpegEncCodingType)cml->partialCoding;
    cfg->frameType = (JpegEncFrameType)cml->frameType;
    cfg->unitsType = (JpegEncAppUnitsType)cml->unitsType;
    cfg->markerType = (JpegEncTableMarkerType)cml->markerType;
    cfg->colorConversion.type = (JpegEncColorConversionType)cml->colorConversion;
    if (cfg->colorConversion.type == JPEGENC_RGBTOYUV_USER_DEFINED)
    {
        /* User defined RGB to YCbCr conversion coefficients, scaled by 16-bits */
        cfg->colorConversion.coeffA = 20000;
        cfg->colorConversion.coeffB = 44000;
        cfg->colorConversion.coeffC = 5000;
        cfg->colorConversion.coeffE = 35000;
        cfg->colorConversion.coeffF = 38000;
    }

    cfg->codingMode = (JpegEncCodingMode)cml->codingMode;

    if(cml->thumbnail < 0 || cml->thumbnail > 3)
    {
        printf( "\nNot valid thumbnail format!");
	    return -1;	
    }
    

	/* use either "hard-coded"/testbench COM data or user specific */
	cfg->comLength = 7;
	cfg->pCom = comment;

//#ifndef ASIC_WAVE_TRACE_TRIGGER
#if 0	 
    printf( "Init config: %dx%d @ x%dy%d => %dx%d   \n",
            cfg.inputWidth, cfg.inputHeight, cfg.xOffset, cfg.yOffset,
            cfg.codingWidth, cfg.codingHeight);

    printf(
            "\n\t**********************************************************\n");
    printf( "\n\t-JPEG: ENCODER CONFIGURATION\n");
    if (cml->qLevel == USER_DEFINED_QTABLE)
    {
        i32 i;
        printf( "JPEG: qTableLuma \t:");
        for (i = 0; i < 64; i++)
            printf( " %d", cfg.qTableLuma[i]);
        printf( "\n");
        printf( "JPEG: qTableChroma \t:");
        for (i = 0; i < 64; i++)
            printf( " %d", cfg.qTableChroma[i]);
        printf( "\n");
    }
    else
        printf( "\t-JPEG: qp \t\t:%d\n", cfg.qLevel);

    printf( "\t-JPEG: inX \t\t:%d\n", cfg.inputWidth);
    printf( "\t-JPEG: inY \t\t:%d\n", cfg.inputHeight);
    printf( "\t-JPEG: outX \t\t:%d\n", cfg.codingWidth);
    printf( "\t-JPEG: outY \t\t:%d\n", cfg.codingHeight);
    printf( "\t-JPEG: rst \t\t:%d\n", cfg.restartInterval);
    printf( "\t-JPEG: xOff \t\t:%d\n", cfg.xOffset);
    printf( "\t-JPEG: yOff \t\t:%d\n", cfg.yOffset);
    printf( "\t-JPEG: frameType \t:%d\n", cfg.frameType);
    printf( "\t-JPEG: colorConversionType :%d\n", cfg.colorConversion.type);
    printf( "\t-JPEG: colorConversionA    :%d\n", cfg.colorConversion.coeffA);
    printf( "\t-JPEG: colorConversionB    :%d\n", cfg.colorConversion.coeffB);
    printf( "\t-JPEG: colorConversionC    :%d\n", cfg.colorConversion.coeffC);
    printf( "\t-JPEG: colorConversionE    :%d\n", cfg.colorConversion.coeffE);
    printf( "\t-JPEG: colorConversionF    :%d\n", cfg.colorConversion.coeffF);
    printf( "\t-JPEG: rotation \t:%d\n", cfg.rotation);
    printf( "\t-JPEG: codingType \t:%d\n", cfg.codingType);
    printf( "\t-JPEG: codingMode \t:%d\n", cfg.codingMode);
    printf( "\t-JPEG: markerType \t:%d\n", cfg.markerType);
    printf( "\t-JPEG: units \t\t:%d\n", cfg.unitsType);
    printf( "\t-JPEG: xDen \t\t:%d\n", cfg.xDensity);
    printf( "\t-JPEG: yDen \t\t:%d\n", cfg.yDensity);


    printf( "\t-JPEG: thumbnail format\t:%d\n", cml->thumbnail);
    printf( "\t-JPEG: Xthumbnail\t:%d\n", cml->widthThumb);
    printf( "\t-JPEG: Ythumbnail\t:%d\n", cml->heightThumb);
    
    printf( "\t-JPEG: First picture\t:%d\n", cml->firstPic);
    printf( "\t-JPEG: Last picture\t\t:%d\n", cml->lastPic);
#ifdef TB_DEFINED_COMMENT
    printf( "\n\tNOTE! Using comment values defined in testbench!\n");
#else
    printf( "\t-JPEG: comlen \t\t:%d\n", cfg.comLength);
    printf( "\t-JPEG: COM \t\t:%s\n", cfg.pCom);
#endif
    printf(
            "\n\t**********************************************************\n\n");
#endif

	 
	if((ret = JpegEncInit(cfg, pEnc)) != JPEGENC_OK)
	{
		printf(
                "Failed to initialize the encoder. Error code: %8i\n", ret);
		return -1;
	}


	return 0;
}


/*------------------------------------------------------------------------------

  JpegCloseEncoder

------------------------------------------------------------------------------*/
void JpegCloseEncoder(JpegEncInst encoder)
{
	JpegEncRet ret;
		 
	if((ret = JpegEncRelease(encoder)) != JPEGENC_OK)
	{
		printf( "Failed to release the encoder. Error code: %8i\n", ret);
	}
}

int arkn141_jpeg_encode (	
	char *imgY_in, 			// 输入图像的Y/Cb/Cr
	char *imgCb_in, 
	char *imgCr_in ,
	unsigned int frame_type,
	char qLevel,						// Quantization level (0 - 9)
	int width,							// 输入图像的宽度/高度
	int height,
	unsigned char *jpeg_stream, 	// 保存JPEG编码流的缓冲区
	int *jpeg_len						// in,  表示JPEG编码流的缓冲区的字节长度
											// Out, 表示编码成功后JPEG码流的字节长度
) 
{
	JpegEncInst encoder;
	JpegEncRet ret;
	JpegEncIn encIn;
	JpegEncOut encOut;
	JpegEncApiVersion encVer;
	JpegEncBuild encBuild;
	JpegEncCfg cfg;
	commandLine_s cmdl;
	i32 slice = 0, sliceRows = 0;
	i32 widthSrc, heightSrc;
	
	memset (&cmdl, 0, sizeof(cmdl));
	memset (&cfg, 0, sizeof(cfg));

	if(frame_type != ARKN141_YUV_FRAME_YUV420 && frame_type != ARKN141_YUV_FRAME_Y_UV420)
	{
		printf ("arkn141_jpeg_encode failed, illegal frame type (%d)\n", frame_type);
		return -1;
	}
	
	
	if(imgY_in == NULL || imgCb_in == NULL)
	{
		printf ("arkn141_jpeg_encode failed, illegal img buffer\n");
		return -1;
	}
	if(frame_type == ARKN141_YUV_FRAME_YUV420 && imgCr_in == NULL)
	{
		printf ("arkn141_jpeg_encode failed, illegal img buffer\n");
		return -1;
	}
	
	if(jpeg_stream == NULL || jpeg_len == NULL)
	{
		printf ("arkn141_jpeg_encode failed, null jpeg_stream or jpeg_len\n");
		return -1;
	}
	
	if(width > 2048 || height > 2048)
	{
		printf ("arkn141_jpeg_encode failed, width (%d) or height (%d) exceed 2048\n", width, height);
		return -1;
	}
	
	/* Print API and build version numbers */
	encVer = JpegEncGetApiVersion();
	encBuild = JpegEncGetBuild();
	
	/* Version */
	//printf(
	//	"\nJPEG Encoder API v%d.%d - SW build: %d - HW build: %x\n\n",
	//	encVer.major, encVer.minor, encBuild.swBuild, encBuild.hwBuild);
	
	
	jpeg_arkn141_param_default (&cmdl);
	cmdl.lumWidthSrc = width;
	cmdl.lumHeightSrc = height;
	
	cmdl.qLevel = qLevel;
	if(frame_type == ARKN141_YUV_FRAME_YUV420)
		cmdl.frameType = JPEGENC_YUV420_PLANAR;
	else if(frame_type == ARKN141_YUV_FRAME_Y_UV420)
		cmdl.frameType = JPEGENC_YUV420_SEMIPLANAR;
	
	if(_JpegOpenEncoder (&cmdl, &encoder, &cfg) < 0)
	{
		printf ("arkn141_jpeg_encode failed, _JpegOpenEncoder NG\n");
		return -1;
	}
	
   /* Setup encoder input */
	// dma_inv_range ((unsigned int)jpeg_stream, (*jpeg_len) + (unsigned int)jpeg_stream);
	encIn.pOutBuf = (u8 *)vaddr_to_page_addr((unsigned int)jpeg_stream);	
	encIn.busOutBuf = (u32)vaddr_to_page_addr ((unsigned int)jpeg_stream);
	encIn.outBufSize = *jpeg_len;
	encIn.frameHeader = 1;
       
	/* Set Full Resolution mode */
   ret = JpegEncSetPictureSize(encoder, &cfg);
	/* Handle error situation */
	if(ret != JPEGENC_OK)
	{
		goto end;
	}

	/* If no slice mode, the slice equals whole frame */
	if(cmdl.partialCoding == 0)
		sliceRows = cmdl.lumHeightSrc;
	else
		sliceRows = cmdl.restartInterval * 16;

	widthSrc = cmdl.lumWidthSrc;
	heightSrc = cmdl.lumHeightSrc;

	/* Bus addresses of input picture, used by hardware encoder */
	encIn.busLum = (u32)imgY_in;
	encIn.busCb = (u32)imgCb_in;
	encIn.busCr = (u32)imgCr_in;
	/* Virtual addresses of input picture, used by software encoder */
	encIn.pLum = (const u8 *)vaddr_to_page_addr((unsigned int)imgY_in);
	encIn.pCb = (const u8 *)vaddr_to_page_addr((unsigned int)imgCb_in);
	encIn.pCr = (const u8 *)vaddr_to_page_addr((unsigned int)imgCr_in);

	ret = JpegEncEncode(encoder, &encIn, &encOut);
	switch (ret)
	{
		case JPEGENC_RESTART_INTERVAL:
			*jpeg_len = encOut.jfifSize;
			break;
			
		case JPEGENC_FRAME_READY:
			*jpeg_len = encOut.jfifSize;
			break;
			
		case JPEGENC_OUTPUT_BUFFER_OVERFLOW:
			*jpeg_len = 0;
			break;
			
		default:
			*jpeg_len = 0;
			printf("FAILED. Error code: %i\n", ret);
			/* For debugging */
			// 除非CPU复位, 否则无法从HW_BUS_ERROR中恢复
			assert (ret != JPEGENC_HW_BUS_ERROR);
			break;
	}
	
end:
	//printf("Release encoder\n");
	/* Free all resources */
	JpegCloseEncoder(encoder);
	return 0;
}

#include "fs.h"
#include "rtos.h"
#include "arkn141_codec.h"

void arkn141_jpeg_encode_test (void)
{
	char *yuv_buf;
	char *jpg_buf;
	int jpg_len;
	printf ("arkn141_jpeg_encode_test ...\n");
	OS_Delay (2000);
	FS_FILE *fp = FS_FOpen ("\\19201080.YUV", "rb");
	if(fp == NULL)
	{
		printf ("arkn141_jpeg_encode_test failed, can't open file(%s)\n", "\\1.YUV");
		return;
	}
	yuv_buf = kernel_malloc (1920 * 1080 * 3 / 2);
	jpg_buf = kernel_malloc (1920 * 1080 / 2);
	jpg_len = 1920 * 1080 / 2;
	if(yuv_buf == NULL || jpg_buf == NULL)
	{
		printf ("arkn141_jpeg_encode_test failed, memory allocate NG\n");
		FS_FClose (fp);
		return;
	}
	FS_FRead (yuv_buf, 1920 * 1080 * 3/2, 1, fp);
	FS_FClose (fp);
	
	dma_flush_range ((unsigned int)yuv_buf, 1920 * 1080 * 3/2 + (unsigned int)yuv_buf);
	dma_inv_range ((unsigned int)jpg_buf, jpg_len + (unsigned int)jpg_buf);
	if(arkn141_codec_open (ARKN141_CODEC_TYPE_JPEG_ENCODER) < 0)
	{
		goto end;
	}
	arkn141_jpeg_encode (yuv_buf, yuv_buf + 1920 * 1080, yuv_buf + 1920 * 1080 * 5/4, ARKN141_YUV_FRAME_Y_UV420,
								7, 1920, 1080, 
								jpg_buf, 
								&jpg_len);
	arkn141_codec_close (ARKN141_CODEC_TYPE_JPEG_ENCODER);
	printf ("jpg_len = %d\n", jpg_len);
	if(jpg_len)
	{
		fp = FS_FOpen ("\\19201080.JPG", "wb");
		if(fp)
		{
			FS_FWrite (jpg_buf, jpg_len, 1, fp);
			FS_FClose (fp);
			FS_CACHE_Clean("");
		}
	}
	
	printf ("arkn141_jpeg_encode_test end\n");
end:
	if(yuv_buf)
		kernel_free (yuv_buf);
	if(jpg_buf)
		kernel_free (jpg_buf);
	
}