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
--  Abstract : JPEG Decoder Testbench
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: dectestbench.c,v $
--  $Date: 2010/02/08 14:05:00 $
--  $Revision: 1.58 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "jpegdecapi.h"
#include "dwl.h"
#include "jpegdeccontainer.h"
#include  "DecGetOption.h"
#include "xm_printf.h"

#include "deccfg.h"
#include "tb_cfg.h"
#include "regdrv.h"

#include "fs.h"
#include "RTOS.H"
#include "xm_core.h"

#ifndef MAX_PATH_
#define MAX_PATH   256  /* maximum lenght of the file path */
#endif

#define JPEG_INPUT_BUFFER 0x5120
//#define DECODER_DEBUG
#ifdef DECODER_DEBUG
	#define DEBUG_DECODE XM_printf
#else
	//#define DEBUG_DECODE print_null
	#define DEBUG_DECODE
#endif

unsigned int vaddr_to_page_addr (unsigned int addr);
void PrintJpegRet(JpegDecRet * pJpegRet);
void dma_inv_range (unsigned int ulStart, unsigned int ulEnd);
void dma_clean_range(unsigned int ulStart, unsigned int ulEnd);
void dma_flush_range(unsigned int ulStart, unsigned int ulEnd);

static u32 output_picture_endian = DEC_X170_OUTPUT_PICTURE_ENDIAN;

#define ADD_SUPPORT_YUV422
unsigned int yuv_format = JPEGDEC_YCbCr420_SEMIPLANAR;
unsigned int get_yuv_format(void)
{
	if(yuv_format == JPEGDEC_YCbCr422_SEMIPLANAR)
		return 6;//FORMAT_Y_UV422;
	else
		return 5;//FORMAT_Y_UV420;
}

// only YUV420 support
int arkn141_jpeg_get_image_info (
	char *jpeg_stream,			// JPEG流缓存区指针
	int   jpeg_length,			// JPEG流字节长度
	int  *img_width,				// 解码的图片宽度(解码缓冲区分配)
	int  *img_height,				// 解码的图片高度
	int  *img_display_width,		// 图片的实际显示宽度
	int  *img_display_height		// 图片的实际显示高度
)
{
    JpegDecInst jpeg;
    JpegDecRet jpegRet;
    JpegDecImageInfo imageInfo;
    JpegDecInput jpegIn;
    int ret = -1;

    u32 clock_gating = DEC_X170_INTERNAL_CLOCK_GATING;
    u32 latency_comp = DEC_X170_LATENCY_COMPENSATION;
    u32 bus_burst_length = DEC_X170_BUS_BURST_LENGTH;
    u32 asic_service_priority = DEC_X170_ASIC_SERVICE_PRIORITY;
    u32 data_discard = DEC_X170_DATA_DISCARD_ENABLE;

    /* reset input */
    jpegIn.streamBuffer.pVirtualAddress = NULL;
    jpegIn.streamBuffer.busAddress = 0;
    jpegIn.streamLength = 0;
    jpegIn.pictureBufferY.pVirtualAddress = NULL;
    jpegIn.pictureBufferY.busAddress = 0;
    jpegIn.pictureBufferCbCr.pVirtualAddress = NULL;
    jpegIn.pictureBufferCbCr.busAddress = 0;

    /* reset imageInfo */
    imageInfo.displayWidth = 0;
    imageInfo.displayHeight = 0;
    imageInfo.outputWidth = 0;
    imageInfo.outputHeight = 0;
    imageInfo.version = 0;
    imageInfo.units = 0;
    imageInfo.xDensity = 0;
    imageInfo.yDensity = 0;
    imageInfo.outputFormat = 0;
    imageInfo.thumbnailType = 0;
    imageInfo.displayWidthThumb = 0;
    imageInfo.displayHeightThumb = 0;
    imageInfo.outputWidthThumb = 0;
    imageInfo.outputHeightThumb = 0;
    imageInfo.outputFormatThumb = 0;

    jpegIn.sliceMbSet = 0;
    jpegIn.bufferSize = 0;

	
	 jpegRet = JpegDecInit(&jpeg);
	 if(jpegRet != JPEGDEC_OK)
	 {
	 	return -1;
	 }
	 
    /* NOTE: The registers should not be used outside decoder SW for other
     * than compile time setting test purposes */
    SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs, HWIF_DEC_LATENCY,
                   latency_comp);
    SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs, HWIF_DEC_CLK_GATE_E,
                   clock_gating);
    SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs, HWIF_DEC_OUT_ENDIAN,
                   output_picture_endian);
    SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs, HWIF_DEC_MAX_BURST,
                   bus_burst_length);
    if ((DWLReadAsicID() >> 16) == 0x8170U)
    {
        SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs, HWIF_PRIORITY_MODE,
                       asic_service_priority);
        SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs, HWIF_DEC_DATA_DISC_E,
                       data_discard);
    }
	 
    /* initialize JpegDecDecode input structure */
    jpegIn.streamBuffer.pVirtualAddress = (u32 *) jpeg_stream;
    jpegIn.streamBuffer.busAddress = (u32)vaddr_to_page_addr((unsigned int)jpeg_stream);
    jpegIn.streamLength = jpeg_length;
    
    jpegRet = JpegDecGetImageInfo(jpeg, &jpegIn, &imageInfo);
    if(jpegRet != JPEGDEC_OK)
    {
    	 ret = -1;	 	
    }
    else
    {
    	//XM_printf( "\t\t-JPEG width: %d\n", imageInfo.outputWidth);
    	//XM_printf( "\t\t-JPEG height: %d\n", imageInfo.outputHeight);
    	//XM_printf( "\t\t-JPEG disp-width: %d\n", imageInfo.displayWidth);
    	//XM_printf( "\t\t-JPEG disp-height: %d\n", imageInfo.displayHeight);
		
		yuv_format = imageInfo.outputFormat;
#ifdef ADD_SUPPORT_YUV422
		if(imageInfo.outputFormat != JPEGDEC_YCbCr420_SEMIPLANAR && imageInfo.outputFormat != JPEGDEC_YCbCr422_SEMIPLANAR)
#else
		if(imageInfo.outputFormat != JPEGDEC_YCbCr420_SEMIPLANAR)
#endif
    	{
#ifdef ADD_SUPPORT_YUV422
    		DEBUG_DECODE ("only YUV420 and YUV422 semiplaner JPEG support\n");
#else
    		DEBUG_DECODE ("only YUV420 semiplaner JPEG support\n");
    		ret = -1;
#endif
    	}
    	else 
 		{
 			*img_width = imageInfo.outputWidth;
 			*img_height = imageInfo.outputHeight;
			*img_display_width = imageInfo.displayWidth;
			*img_display_height = imageInfo.displayHeight;
 			ret = 0;
 		}
    }
    
    JpegDecRelease(jpeg);
    
    return ret;
}


int arkn141_jpeg_decode (	
	char *jpeg_stream,			// JPEG流缓存区指针
	int   jpeg_length,			// JPEG流字节长度
	char*	image_y,
	char* image_cbcr
)
{
	 int ret = -1;
    u8 *inputBuffer = NULL;
    u32 len;
    u32 streamTotalLen = 0;
    u32 streamSeekLen = 0;

    u32 streamInFile = 0;
    u32 mcuSizeDivider = 0;

    i32 i, j = 0;
    u32 tmp = 0;
    u32 size = 0;
    u32 loop;
    u32 frameCounter = 0;
    u32 inputReadType = 0;
    u32 amountOfMCUs = 0;
    u32 mcuInRow = 0;
	 u32 tickscount;

    JpegDecInst jpeg;
    JpegDecRet jpegRet;
    JpegDecImageInfo imageInfo;
    JpegDecInput jpegIn;
    JpegDecOutput jpegOut;


    u32 clock_gating = DEC_X170_INTERNAL_CLOCK_GATING;
    u32 latency_comp = DEC_X170_LATENCY_COMPENSATION;
    u32 bus_burst_length = DEC_X170_BUS_BURST_LENGTH;
    u32 asic_service_priority = DEC_X170_ASIC_SERVICE_PRIORITY;
    u32 data_discard = DEC_X170_DATA_DISCARD_ENABLE;

	 
    /* reset input */
    jpegIn.streamBuffer.pVirtualAddress = NULL;
    jpegIn.streamBuffer.busAddress = 0;
    jpegIn.streamLength = 0;
    jpegIn.pictureBufferY.pVirtualAddress = (u32 *)image_y;
    jpegIn.pictureBufferY.busAddress = (u32)vaddr_to_page_addr((u32)image_y);
    jpegIn.pictureBufferCbCr.pVirtualAddress = (u32 *)image_cbcr;
    jpegIn.pictureBufferCbCr.busAddress = (u32)vaddr_to_page_addr((u32)image_cbcr);
	 jpegIn.decImageType = JPEGDEC_IMAGE;

    /* reset output */
    jpegOut.outputPictureY.pVirtualAddress = NULL;
    jpegOut.outputPictureY.busAddress = 0;
    jpegOut.outputPictureCbCr.pVirtualAddress = NULL;
    jpegOut.outputPictureCbCr.busAddress = 0;
    jpegOut.outputPictureCr.pVirtualAddress = NULL;
    jpegOut.outputPictureCr.busAddress = 0;

    /* reset imageInfo */
    imageInfo.displayWidth = 0;
    imageInfo.displayHeight = 0;
    imageInfo.outputWidth = 0;
    imageInfo.outputHeight = 0;
    imageInfo.version = 0;
    imageInfo.units = 0;
    imageInfo.xDensity = 0;
    imageInfo.yDensity = 0;
    imageInfo.outputFormat = 0;
    imageInfo.thumbnailType = 0;
    imageInfo.displayWidthThumb = 0;
    imageInfo.displayHeightThumb = 0;
    imageInfo.outputWidthThumb = 0;
    imageInfo.outputHeightThumb = 0;
    imageInfo.outputFormatThumb = 0;

    /* set default */
    jpegIn.sliceMbSet = 0;
    jpegIn.bufferSize = 0;

    /* Jpeg initialization */
    jpegRet = JpegDecInit(&jpeg);
    if(jpegRet != JPEGDEC_OK)
    {
        /* Handle here the error situation */
        PrintJpegRet(&jpegRet);
        goto end;
    }


    /* NOTE: The registers should not be used outside decoder SW for other
     * than compile time setting test purposes */
    SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs, HWIF_DEC_LATENCY,
                   latency_comp);
    SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs, HWIF_DEC_CLK_GATE_E,
                   clock_gating);
    SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs, HWIF_DEC_OUT_ENDIAN,
                   output_picture_endian);
    SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs, HWIF_DEC_MAX_BURST,
                   bus_burst_length);
    if ((DWLReadAsicID() >> 16) == 0x8170U)
    {
        SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs, HWIF_PRIORITY_MODE,
                       asic_service_priority);
        SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs, HWIF_DEC_DATA_DISC_E,
                       data_discard);
    }
 
	 dma_flush_range ((u32)jpeg_stream, jpeg_length + (unsigned int)jpeg_stream);
	 
    /* initialize JpegDecDecode input structure */
    jpegIn.streamBuffer.pVirtualAddress = (u32 *) jpeg_stream;
    jpegIn.streamBuffer.busAddress = (u32)vaddr_to_page_addr((u32)jpeg_stream);;
    jpegIn.streamLength = jpeg_length;
	 jpegIn.bufferSize = 0; /* input buffering (0 == not used) */

    jpegRet = JpegDecGetImageInfo(jpeg, &jpegIn, &imageInfo);
    if(jpegRet != JPEGDEC_OK)
    {
        goto end;
    }
    //if(imageInfo.outputFormat != JPEGDEC_YCbCr420_SEMIPLANAR)
    
#ifdef ADD_SUPPORT_YUV422
		if(imageInfo.outputFormat != JPEGDEC_YCbCr420_SEMIPLANAR && imageInfo.outputFormat != JPEGDEC_YCbCr422_SEMIPLANAR)
#else
		if(imageInfo.outputFormat != JPEGDEC_YCbCr420_SEMIPLANAR)
#endif
    {
    	XM_printf ("illegal format(%d), only YCbCr420 support\n", imageInfo.outputFormat);
        goto end;
    }

    if(imageInfo.outputFormat)
    {
        switch (imageInfo.outputFormat)
        {
        case JPEGDEC_YCbCr420_SEMIPLANAR:
            //XM_printf(
            //        "\t\t-JPEG: FULL RESOLUTION OUTPUT: JPEGDEC_YCbCr420_SEMIPLANAR\n");
            break;
        }
    }
    
    /* stream type */
    switch (imageInfo.codingMode)
    {
    case JPEGDEC_BASELINE:
        //XM_printf( "\t\t-JPEG: STREAM TYPE: JPEGDEC_BASELINE\n");
        break;
    case JPEGDEC_PROGRESSIVE:
        //XM_printf( "\t\t-JPEG: STREAM TYPE: JPEGDEC_PROGRESSIVE\n");
        break;
    case JPEGDEC_NONINTERLEAVED:
        //XM_printf( "\t\t-JPEG: STREAM TYPE: JPEGDEC_NONINTERLEAVED\n");
        break;
    }

    if(imageInfo.codingMode != JPEGDEC_BASELINE)
    {
    		XM_printf ("only BASELINE support\n");
    		goto end; 		
    }	
	 
	 dma_inv_range ((u32)image_y, imageInfo.outputWidth * imageInfo.outputHeight + (unsigned int)image_y);
	 dma_inv_range ((u32)image_cbcr, imageInfo.outputWidth * imageInfo.outputHeight /2 + (unsigned int)image_cbcr);

	jpegRet = JpegDecDecode(jpeg, &jpegIn, &jpegOut);
	if(jpegRet == JPEGDEC_FRAME_READY)
	{
		ret = 0;
	}
	else
	{
		ret = -1;
	}


end:
	if(jpeg)
		JpegDecRelease(jpeg);
	XM_printf( "Jpeg decode end, ret=%d\n", ret);

	return ret;
}

#include "fs.h"
#include "rtos.h"
#include "arkn141_codec.h"

void arkn141_jpeg_decode_test (void)
{
	XM_printf ("arkn141_jpeg_decode_test ...\n");
	FS_FILE *fp = NULL;
	unsigned int len;
	char *mem = NULL;
	char *yuv = NULL;
	int w, h;
	int display_w, display_h;
	while(1)
	{
		int volume_status = FS_GetVolumeStatus ("mmc:0:");
		if(volume_status == FS_MEDIA_IS_PRESENT)
		{
			XM_printf ("SDMMC (slot %d) present\n", 0);
			break;
		}
		OS_Delay (100);
	}
	
	do
	{
		fp = FS_FOpen ("\\19201080.JPG", "rb");
		if(fp == NULL)
		{
			XM_printf ("arkn141_jpeg_decode_test failed, open \\19201080.JPG NG\n");
			break;
		}
		len = FS_GetFileSize(fp);
		if(len == 0)
		{
			XM_printf ("arkn141_jpeg_decode_test failed, open \\19201080.JPG NG\n");
			break;		
		}
		mem = kernel_malloc (len);
		if(FS_FRead (mem, 1, len, fp) != len)
		{
			XM_printf ("arkn141_jpeg_decode_test failed, read \\19201080.JPG NG\n");
			break;		
		}
		FS_FClose (fp);
		fp = NULL;
		
		if(arkn141_jpeg_get_image_info (mem, len, &w, &h, &display_w, &display_h) < 0)
		{
			XM_printf ("arkn141_jpeg_decode_test failed, arkn141_jpeg_get_image_info \\19201080.JPG NG\n");
			break;
		}
		
		XM_printf ("w = %d, h = %d\n", w, h);
		
		yuv = kernel_malloc (w * h * 3/2);
		if(yuv == NULL)
		{
			XM_printf ("arkn141_jpeg_decode_test failed, malloc NG\n");
			break;
		}
		
		if(arkn141_codec_open (ARKN141_CODEC_TYPE_JPEG_DECODER) < 0)
		{
			XM_printf ("arkn141_jpeg_decode_test failed, arkn141_codec_open NG\n");
			break;
		}
		
		if(arkn141_jpeg_decode (mem, len, yuv, yuv + w * h) < 0)
		{
			XM_printf ("arkn141_jpeg_decode_test failed, arkn141_jpeg_decode NG\n");
			break;
		}
		arkn141_codec_close (ARKN141_CODEC_TYPE_JPEG_DECODER);
		fp = FS_FOpen ("\\19201080.YUV", "wb");
		if(fp)
		{
			FS_FWrite (yuv, w * h * 3/2, 1, fp);
			FS_FClose (fp);
			fp = NULL;
			
			FS_CACHE_Clean(""); 
		}
		
	} while(0);
	
	
	if(fp)
		FS_FClose (fp);
	if(mem)
		kernel_free (mem);
	if(yuv)
		kernel_free (yuv);
}
