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

#include "fs.h"
/*--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/* User selectable testbench configuration */

/* Define this if you want to save each frame of motion jpeg 
 * into frame%d.jpg */
/* #define SEPARATE_FRAME_OUTPUT */

/* Define this if yuv don't want to use debug printf */
/*#define ASIC_WAVE_TRACE_TRIGGER*/

/* Output stream is not written to file. This should be used
   when running performance simulations. */
/*#define NO_OUTPUT_WRITE */

/* Define these if you want to use testbench defined 
 * comment header */
/*#define TB_DEFINED_COMMENT */

#define USER_DEFINED_QTABLE 10

//static char input[] = "\\MINITEST\\RAW\\VIDEO\\TEST.YUV";	
//static char output[] = "\\stream.264";
//static char input[] = "\\MINITEST\\RAW\\VIDEO\\03520288.YUV";
//static char output[] = "\\test.jpg";
//static char output[] = "\\stream.jpg";

static char nal_sizes_file[] = "\\nal_size.txt";
static char *streamType[2] = { "BYTE_STREAM", "NAL_UNITS" };
static char *viewMode[4] = { "H264_DOUBLE_BUFFER", "H264_SINGLE_BUFFER",
                             "MVC_INTER_VIEW_PRED", "MVC_INTER_PRED"};

/* Global variables */

/* Command line options */

static option_s options[] = {
    {"help", 'H', 0},
    {"inputThumb", 'I', 1},
    {"thumbnail", 'T', 1},
    {"widthThumb", 'K', 1},
    {"heightThumb", 'L', 1},
    {"input", 'i', 1},
    {"output", 'o', 1},
    {"firstPic", 'a', 1},
    {"lastPic", 'b', 1},
    {"lumWidthSrc", 'w', 1},
    {"lumHeightSrc", 'h', 1},
    {"width", 'x', 1},
    {"height", 'y', 1},
    {"horOffsetSrc", 'X', 1},
    {"verOffsetSrc", 'Y', 1},
    {"restartInterval", 'R', 1},
    {"qLevel", 'q', 1},
    {"frameType", 'g', 1},
    {"colorConversion", 'v', 1},
    {"rotation", 'G', 1},
    {"codingType", 'p', 1},
    {"codingMode", 'm', 1},
    {"markerType", 't', 1},
    {"units", 'u', 1},
    {"xdensity", 'k', 1},
    {"ydensity", 'l', 1},
    {"write", 'W', 1},
    {"comLength", 'c', 1},
    {"comFile", 'C', 1},
    {"trigger", 'P', 1},
    {NULL, 0, 0}
};

/* SW/HW shared memories for input/output buffers */
EWLLinearMem_t pictureMem;
EWLLinearMem_t outbufMem;

/* Test bench definition of comment header */
#ifdef TB_DEFINED_COMMENT
    /* COM data */
static u32 comLen = 38;
static u8 comment[39] = "This is Hantro's test COM data header.";
#endif

static JpegEncCfg cfg;

static u32 writeOutput = 1;

/* Logic Analyzer trigger point */
static i32 trigger_point = -1;

static u32 thumbDataLength;
u8 * thumbData = NULL; /* thumbnail data buffer */


/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void JpegFreeRes(JpegEncInst enc);
static int  JpegAllocRes(commandLine_s * cmdl, JpegEncInst encoder);
static int  JpegOpenEncoder(commandLine_s * cml, JpegEncInst * encoder);
static void JpegCloseEncoder(JpegEncInst encoder);
static int  JpegReadPic(u8 * image, i32 width, i32 height, i32 sliceNum,
                   i32 sliceRows, i32 frameNum, char *name, u32 inputMode);
static int  JpegParameter(i32 argc, char **argv, commandLine_s * ep);
static void JpegHelp(void);
static void JpegWriteStrm(FS_FILE * fout, u32 * outbuf, u32 size, u32 endian);
static u32  JpegGetResolution(char *filename, i32 *pWidth, i32 *pHeight);


#ifdef SEPARATE_FRAME_OUTPUT
static void JpegWriteFrame(char *filename, u32 * strmbuf, u32 size);
#endif

static char qLevel=0;
static char GetJpegqLevel()
{
	return qLevel;
}

void SetJpegQLevel(char level )
{
	qLevel = level;
}
static int Slicerownum=1;
static int GetSliceRowNum()
{
	return Slicerownum;
}
void SetSliceRowNum(int rownum )
{
	Slicerownum = rownum;
}


void cleanfilename_encode(char *filename)
{
	int len = strlen(filename );
	int no = len;
	while(filename[no--] != '\\'   )
	{	
		if(no == 0 )
			break;
	}
	if(!no)
		return;
	no +=2 ;
	while( no < (len) )
	{
		filename[no] = 0;
		no ++ ;
	}
	return ;
}

/*------------------------------------------------------------------------------

    main

------------------------------------------------------------------------------*/
int jpeg_encode_main(int argc, char *argv[])
{
    JpegEncInst encoder;
    JpegEncRet ret;
    JpegEncIn encIn;
    JpegEncOut encOut;
    JpegEncApiVersion encVer;
    JpegEncBuild encBuild;
	 char filename[256];
	 unsigned char addno = 0;

    FS_FILE *fout = NULL;
    i32 picBytes = 0;

    commandLine_s cmdl;

  //  printf(
	    printf("\n* * * * * * * * * * * * * * * * * * * * *\n\n");
       printf("      HANTRO JPEG ENCODER TESTBENCH\n");
       printf("\n* * * * * * * * * * * * * * * * * * * * *\n\n");

    if(argc < 2)
    {
        JpegHelp();
        exit(0);
    }

    /* Print API and build version numbers */
    encVer = JpegEncGetApiVersion();
    encBuild = JpegEncGetBuild();

    /* Version */
    printf(
            "\nJPEG Encoder API v%d.%d - SW build: %d - HW build: %x\n\n",
            encVer.major, encVer.minor, encBuild.swBuild, encBuild.hwBuild);

    /* Parse command line JpegParameters */
    if(JpegParameter(argc, argv, &cmdl) != 0)
    {
        printf( "Input JpegParameter error\n");
        return -1;
    }
	 cmdl.qLevel=GetJpegqLevel();/*获取当前的qLevel */
	 printf("qLevel=%d\n",cmdl.qLevel );
	 //加入对slice的描述
	 cmdl.restartInterval = GetSliceRowNum();
	 printf("RowNum=%d\n",cmdl.restartInterval );
	 
	 printf(" input file:%s\n", cmdl.input );
    /* Encoder initialization */
    if(JpegOpenEncoder(&cmdl, &encoder) != 0)
    {
        return -1;
    }

    /* Allocate input and output buffers */
    if(JpegAllocRes(&cmdl, encoder) != 0)
    {
        printf( "Failed to allocate the external resources!\n");
        JpegFreeRes(encoder);
        JpegCloseEncoder(encoder);
        return 1;
    }

    /* Setup encoder input */
    encIn.pOutBuf = (u8 *)outbufMem.virtualAddress;

    printf("encIn.pOutBuf 0x%08x\n", (unsigned int) encIn.pOutBuf);
    encIn.busOutBuf = outbufMem.busAddress;
    encIn.outBufSize = outbufMem.size;
    encIn.frameHeader = 1;

    {// 分片           每片行数
        i32 slice = 0, sliceRows = 0;
        i32 next = 0, last = 0, picCnt = 0;
        i32 widthSrc, heightSrc;
        char *input;


        /* Set Full Resolution mode */
        ret = JpegEncSetPictureSize(encoder, &cfg);

	/* Handle error situation */
        if(ret != JPEGENC_OK)
        {
#ifndef ASIC_WAVE_TRACE_TRIGGER
            printf("FAILED. Error code: %i\n", ret);
#endif
            goto end;
        }

            /* If no slice mode, the slice equals whole frame */
				if(cmdl.partialCoding == 0)
     	     		sliceRows = cmdl.lumHeightSrc;
            else
           		sliceRows = cmdl.restartInterval * 16;

            widthSrc = cmdl.lumWidthSrc;
            heightSrc = cmdl.lumHeightSrc;

            input = cmdl.input;

            last = cmdl.lastPic;

			// 包含  YUV420 和 Y_UV420 
        if(cmdl.frameType <= JPEGENC_YUV420_SEMIPLANAR)// 
        {
            /* Bus addresses of input picture, used by hardware encoder */
            encIn.busLum = pictureMem.busAddress;
            encIn.busCb = encIn.busLum + (widthSrc * sliceRows);
            encIn.busCr = encIn.busCb +
                (((widthSrc + 1) / 2) * ((sliceRows + 1) / 2));

            /* Virtual addresses of input picture, used by software encoder */
            encIn.pLum = (u8 *)pictureMem.virtualAddress;
            encIn.pCb = encIn.pLum + (widthSrc * sliceRows);
            encIn.pCr = encIn.pCb +
                (((widthSrc + 1) / 2) * ((sliceRows + 1) / 2));
        }
        else
        {
            /* Bus addresses of input picture, used by hardware encoder */
            encIn.busLum = pictureMem.busAddress;
            encIn.busCb = encIn.busLum;
            encIn.busCr = encIn.busCb;

            /* Virtual addresses of input picture, used by software encoder */
            encIn.pLum = (u8 *)pictureMem.virtualAddress;
            encIn.pCb = encIn.pLum;
            encIn.pCr = encIn.pCb;
        }

		  printf("cmdl.input:%s\n",cmdl.input);
		  printf("cmdl.output:%s\n",cmdl.output);
		  memset(filename , 0 , 256) ;
		  
        if((fout = FS_FOpen(cmdl.output, "wb")) == NULL )
		  {
				//如果打不开输出文件，则创建一个新文件  	
		  		strcpy(filename , cmdl.input );
		  		cleanfilename_encode(filename);
				if( cmdl.codingMode )//codingMode :0=YUV 4:2:0, 1=YUV 4:2:2. 
			  {
					addno = 10*(cmdl.codingMode);
					sprintf(filename +strlen(filename) , "Result%02d.jpg", cmdl.qLevel+addno );
			  }
			  else
					sprintf(filename +strlen(filename) , "Result%02d.jpg", cmdl.qLevel );
	
			  fout = FS_FOpen( filename , "wb");

			  printf("filename:%s\n",filename); 
			  if(fout == NULL)
			  {
					printf( "Failed to create the output file.\n");
					JpegCloseEncoder(encoder);
					JpegFreeRes(encoder);
					return -1;
			  }
		  }
		  else
		  {
			  strcpy(filename , cmdl.output );
		  }
		  
        /* Main encoding loop */
        ret = JPEGENC_FRAME_READY;
        next = cmdl.firstPic;
        while(next <= last &&
              (ret == JPEGENC_FRAME_READY ||
               ret == JPEGENC_OUTPUT_BUFFER_OVERFLOW))
        {
#ifdef SEPARATE_FRAME_OUTPUT
            char framefile[50];
            sprintf(framefile, "frame%d%s.jpg", picCnt, mode == 1 ? "tn" : "");
            remove(framefile);
#endif

#ifndef ASIC_WAVE_TRACE_TRIGGER
            printf("Frame %3d started...\n", picCnt);
#endif
            FS_CACHE_Clean("");//fflush(stdout);

            /* Loop until one frame is encoded */
            do
            {
#ifndef NO_INPUT_YUV
					int tickscount;
                /* Read next slice */
					if(JpegReadPic
                   ((u8 *) pictureMem.virtualAddress,
                    widthSrc, heightSrc, slice,
                    sliceRows, next, input, cmdl.frameType) != 0)
                    break;


#endif

				    tickscount = XM_GetTickCount();
                ret = JpegEncEncode(encoder, &encIn, &encOut);
					 tickscount = XM_GetTickCount() - tickscount ;
           		 printf("Encode %d ms ", tickscount);
                switch (ret)
                {
                case JPEGENC_RESTART_INTERVAL:

#ifndef ASIC_WAVE_TRACE_TRIGGER
						 printf("Frame:%3d Slice:%3d restart interval! %6u bytes\n",
                           picCnt, slice,encOut.jfifSize);
                    FS_CACHE_Clean("");//fflush(stdout);
#endif
                    if(writeOutput)
                    {
							  dma_inv_range ((u32)outbufMem.virtualAddress, encOut.jfifSize + (u32)outbufMem.virtualAddress);
							  JpegWriteStrm(fout, outbufMem.virtualAddress, 
                                encOut.jfifSize, 0);
						  }
#ifdef SEPARATE_FRAME_OUTPUT
                    if(writeOutput)
                        JpegWriteFrame(framefile, outbufMem.virtualAddress, 
                                encOut.jfifSize);
#endif
                    picBytes += encOut.jfifSize;
                    slice++;    /* Encode next slice */
                    break;

                case JPEGENC_FRAME_READY:
#ifndef ASIC_WAVE_TRACE_TRIGGER
						 printf("Frame:%3d Slice:%3d ready! %6u bytes\n",
                           picCnt, slice,      encOut.jfifSize);
                    FS_CACHE_Clean("");//fflush(stdout);
#endif
                    if(writeOutput)
                    {
							  dma_inv_range ((u32)outbufMem.virtualAddress, encOut.jfifSize + (u32)outbufMem.virtualAddress);
							  JpegWriteStrm(fout, outbufMem.virtualAddress, 
                                encOut.jfifSize, 0);
						  }
#ifdef SEPARATE_FRAME_OUTPUT
                    if(writeOutput)
                        JpegWriteFrame(framefile, outbufMem.virtualAddress, 
                                encOut.jfifSize);
#endif
                    picBytes = 0;
                    slice = 0;
                    break;

                case JPEGENC_OUTPUT_BUFFER_OVERFLOW:

#ifndef ASIC_WAVE_TRACE_TRIGGER
                    printf("Frame %3d lost! Output buffer overflow.\n",
                           picCnt);
#endif

                    /* For debugging */
                    if(writeOutput)
                    {
							  dma_inv_range ((u32)outbufMem.virtualAddress, outbufMem.size + (u32)outbufMem.virtualAddress);
							  JpegWriteStrm(fout, outbufMem.virtualAddress, 
                                outbufMem.size, 0);
						  }
                    /* Rewind the file back this picture's bytes
                    FS_FSeek(fout, -picBytes, SEEK_CUR);*/
                    picBytes = 0;
                    slice = 0;
                    break;

                default:

#ifndef ASIC_WAVE_TRACE_TRIGGER
                    printf("FAILED. Error code: %i\n", ret);
#endif
                    /* For debugging */
                    if(writeOutput)
                    {
							  dma_inv_range ((u32)outbufMem.virtualAddress, encOut.jfifSize + (u32)outbufMem.virtualAddress);
							  JpegWriteStrm(fout, outbufMem.virtualAddress,
                                encOut.jfifSize, 0);
						  }
                    break;
                }
            }
            while(ret == JPEGENC_RESTART_INTERVAL);

            picCnt++;
            next = picCnt + cmdl.firstPic;
        }   /* End of main encoding loop */

    }   /* End of encoding modes */

end:

#ifndef ASIC_WAVE_TRACE_TRIGGER
    printf("Release encoder\n");
#endif

    /* Free all resources */
    JpegCloseEncoder(encoder);
    if(fout != NULL)
        FS_FClose(fout);
    JpegFreeRes(encoder);

    return 0;
}

/*------------------------------------------------------------------------------

    JpegAllocRes

    Allocation of the physical memories used by both SW and HW: 
    the input picture and the output stream buffer.

    NOTE! The implementation uses the EWL instance from the encoder
          for OS independence. This is not recommended in final environment 
          because the encoder will release the EWL instance in case of error.
          Instead, the memories should be allocated from the OS the same way
          as inside EWLMallocLinear().

------------------------------------------------------------------------------*/
int JpegAllocRes(commandLine_s * cmdl, JpegEncInst enc)
{
    i32 sliceRows = 0;
    u32 pictureSize;
    u32 outbufSize;
    i32 ret;

    /* Set slice size and output buffer size
     * For output buffer size, 1 byte/pixel is enough for most images.
     * Some extra is needed for testing purposes (noise input) */

    if(cmdl->partialCoding == 0)
    {
        sliceRows = cmdl->lumHeightSrc;
    }
    else
    {
        sliceRows = cmdl->restartInterval * 16;
    }
            
    outbufSize = cmdl->width * sliceRows * 3 + 100 * 1024;

    if(cmdl->thumbnail)
        outbufSize += cmdl->widthThumb * cmdl->heightThumb;

    /* calculate picture size */
    if(cmdl->frameType <= JPEGENC_YUV420_SEMIPLANAR)/*YUV420 & Y_UV420*/
    {
        /* Input picture in YUV 4:2:0 format */
        pictureSize = cmdl->lumWidthSrc * sliceRows * 3 / 2;
    }
    else if(cmdl->frameType <= JPEGENC_BGR444)
    {
        /* Input picture in YUYV 4:2:2 or 16-bit RGB format */
        pictureSize = cmdl->lumWidthSrc * sliceRows * 2;
    }
    else
    {
        /* Input picture in 32-bit RGB format */
        pictureSize = cmdl->lumWidthSrc * sliceRows * 4;
    }
    
    pictureMem.virtualAddress = NULL;
    outbufMem.virtualAddress = NULL;
    
     /* Here we use the EWL instance directly from the encoder 
     * because it is the easiest way to allocate the linear memories */
    ret = EWLMallocLinear(((jpegInstance_s *)enc)->asic.ewl, pictureSize, 
                &pictureMem);
    if (ret != EWL_OK)
    {
        printf( "Failed to allocate input picture!\n");
        pictureMem.virtualAddress = NULL;        
        return 1;
    }
    /* this is limitation for FPGA testing */
    outbufSize = outbufSize < (1024*1024*7) ? outbufSize : (1024*1024*7);

    ret = EWLMallocLinear(((jpegInstance_s *)enc)->asic.ewl, outbufSize, 
                &outbufMem);
    if (ret != EWL_OK)
    {
        printf( "Failed to allocate output buffer!\n");
        outbufMem.virtualAddress = NULL;
        return 1;
    }

#ifndef ASIC_WAVE_TRACE_TRIGGER
    printf("Input %dx%d + %dx%d encoding at %dx%d + %dx%d ",
               cmdl->lumWidthSrcThumb, cmdl->lumHeightSrcThumb,
               cmdl->lumWidthSrc, cmdl->lumHeightSrc,
               cmdl->widthThumb, cmdl->heightThumb, cmdl->width, cmdl->height);

    if(cmdl->partialCoding != 0)
        printf("in slices of %dx%d", cmdl->width, sliceRows);
    printf("\n");
#endif


#ifndef ASIC_WAVE_TRACE_TRIGGER
    printf("Input buffer size:          %d bytes\n", pictureMem.size);
    printf("Input buffer bus address:   0x%08x\n", pictureMem.busAddress);
    printf("Input buffer user address:  %10p\n", pictureMem.virtualAddress);
    printf("Output buffer size:         %d bytes\n", outbufMem.size);
    printf("Output buffer bus address:  0x%08x\n", outbufMem.busAddress);
    printf("Output buffer user address: %10p\n",  outbufMem.virtualAddress);
#endif

    return 0;
}

/*------------------------------------------------------------------------------

    JpegFreeRes

------------------------------------------------------------------------------*/
void JpegFreeRes(JpegEncInst enc)
{
    if(pictureMem.virtualAddress != NULL)
        EWLFreeLinear(((jpegInstance_s *)enc)->asic.ewl, &pictureMem); 
    if(outbufMem.virtualAddress != NULL)
        EWLFreeLinear(((jpegInstance_s *)enc)->asic.ewl, &outbufMem);
    if(thumbData != NULL)
        free(thumbData);
#ifndef TB_DEFINED_COMMENT   
    if(cfg.pCom != NULL)
	    free((void *)(cfg.pCom));
#endif
}

/*------------------------------------------------------------------------------

    JpegOpenEncoder

------------------------------------------------------------------------------*/
int JpegOpenEncoder(commandLine_s * cml, JpegEncInst * pEnc)
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

#ifndef TB_DEFINED_COMMENT
    FS_FILE *fileCom = NULL;
#endif

    /* Default resolution, try parsing input file name */
    if(cml->lumWidthSrc == DEFAULT || cml->lumHeightSrc == DEFAULT)
    {
        if (JpegGetResolution(cml->input, &cml->lumWidthSrc, &cml->lumHeightSrc))
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

    cfg.rotation = (JpegEncPictureRotation)cml->rotation;
    cfg.inputWidth = (cml->lumWidthSrc + 15) & (~15);   /* API limitation */
    if (cfg.inputWidth != (u32)cml->lumWidthSrc)
        printf( "Warning: Input width must be multiple of 16!\n");
    cfg.inputHeight = cml->lumHeightSrc;

    if(cfg.rotation)
    {
        /* full */
        cfg.xOffset = cml->verOffsetSrc;
        cfg.yOffset = cml->horOffsetSrc;

        cfg.codingWidth = cml->height;
        cfg.codingHeight = cml->width;
        cfg.xDensity = cml->ydensity;
        cfg.yDensity = cml->xdensity;	
    }
    else
    {
        /* full */
        cfg.xOffset = cml->horOffsetSrc;
        cfg.yOffset = cml->verOffsetSrc;

        cfg.codingWidth = cml->width;
        cfg.codingHeight = cml->height;
        cfg.xDensity = cml->xdensity;
        cfg.yDensity = cml->ydensity;
    }

    if (cml->qLevel == USER_DEFINED_QTABLE)
    {
        cfg.qTableLuma = qTable;
        cfg.qTableChroma = qTable;
    }
    else
        cfg.qLevel = cml->qLevel;

    cfg.restartInterval = cml->restartInterval;
    cfg.codingType = (JpegEncCodingType)cml->partialCoding;
    cfg.frameType = (JpegEncFrameType)cml->frameType;
    cfg.unitsType = (JpegEncAppUnitsType)cml->unitsType;
    cfg.markerType = (JpegEncTableMarkerType)cml->markerType;
    cfg.colorConversion.type = (JpegEncColorConversionType)cml->colorConversion;
    if (cfg.colorConversion.type == JPEGENC_RGBTOYUV_USER_DEFINED)
    {
        /* User defined RGB to YCbCr conversion coefficients, scaled by 16-bits */
        cfg.colorConversion.coeffA = 20000;
        cfg.colorConversion.coeffB = 44000;
        cfg.colorConversion.coeffC = 5000;
        cfg.colorConversion.coeffE = 35000;
        cfg.colorConversion.coeffF = 38000;
    }
    writeOutput = cml->write;
    cfg.codingMode = (JpegEncCodingMode)cml->codingMode;

#ifdef NO_OUTPUT_WRITE
    writeOutput = 0;
#endif

    if(cml->thumbnail < 0 || cml->thumbnail > 3)
    {
        printf( "\nNot valid thumbnail format!");
	    return -1;	
    }
    
	if(cml->thumbnail != 0)
	{
		FS_FILE *fThumb;
	
		fThumb = FS_FOpen(cml->inputThumb, "rb");
		if(fThumb == NULL)
		{
			 printf( "\nUnable to open Thumbnail file: %s\n", cml->inputThumb);
			 return -1;
		}
		
		switch(cml->thumbnail)
		{
			 case 1:
			FS_FSeek(fThumb,0,SEEK_END);		
			thumbDataLength = FS_FTell(fThumb);
			FS_FSeek(fThumb,0,SEEK_SET);
			break;
			 case 2:
			thumbDataLength = 3*256 + cml->widthThumb * cml->heightThumb;
			break;
			 case 3:
			thumbDataLength = cml->widthThumb * cml->heightThumb * 3;		
			break;
			 default:
			assert(0);		
		}
	
		thumbData = (u8*)malloc(thumbDataLength);
		FS_FRead(thumbData,1,thumbDataLength, fThumb);
		FS_FClose(fThumb);
	}

/* use either "hard-coded"/testbench COM data or user specific */
#ifdef TB_DEFINED_COMMENT
	cfg.comLength = comLen;
	cfg.pCom = comment;
#else
	cfg.comLength = cml->comLength;

	if(cfg.comLength)
	{
        /* allocate mem for & read comment data */
		cfg.pCom = (u8 *) malloc(cfg.comLength);

		fileCom = FS_FOpen(cml->com, "rb");
		if(fileCom == NULL)
		{
			printf( "\nUnable to open COMMENT file: %s\n", cml->com);
			return -1;
		}

		FS_FRead((void*)(cfg.pCom), 1, cfg.comLength, fileCom);
		FS_FClose(fileCom);
	}

#endif

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

	 
	if((ret = JpegEncInit(&cfg, pEnc)) != JPEGENC_OK)
	{
		printf(
                "Failed to initialize the encoder. Error code: %8i\n", ret);
		return -1;
	}

	if(thumbData != NULL)
	{
		JpegEncThumb jpegThumb;
		jpegThumb.format = cml->thumbnail == 1 ? JPEGENC_THUMB_JPEG : cml->thumbnail == 3 ?
						  JPEGENC_THUMB_RGB24 : JPEGENC_THUMB_PALETTE_RGB8;
		jpegThumb.width = cml->widthThumb;
		jpegThumb.height = cml->heightThumb;
		jpegThumb.data = thumbData;
				jpegThumb.dataLength = thumbDataLength;
		
		ret = JpegEncSetThumbnail(*pEnc, &jpegThumb );
		if(ret != JPEGENC_OK )
		{
			 printf("Failed to set thumbnail. Error code: %8i\n", ret);
				 return -1;
		}
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
        printf(
                "Failed to release the encoder. Error code: %8i\n", ret);
    }
}

/*------------------------------------------------------------------------------

    JpegParameter

------------------------------------------------------------------------------*/
int JpegParameter(i32 argc, char **argv, commandLine_s * cml)
{
    i32 ret;
    char *optarg;
    argument_s argument;
    int status = 0;

    memset(cml, 0, sizeof(commandLine_s));
//    strcpy(cml->input, "input.yuv");
    strcpy(cml->inputThumb, "thumbnail.jpg");
    strcpy(cml->com, "com.txt");
//    strcpy(cml->output, "stream.jpg");
    cml->firstPic = 0;
  //  cml->lastPic = 10;
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
  //  cml->output = output;

    argument.optCnt = 1;
    while((ret = EncGetOption(argc, argv, options, &argument)) != -1)
    {
        if(ret == -2)
        {
            status = -1;
        }
        optarg = argument.optArg;
        switch (argument.shortOpt)
        {
        case 'H':
            JpegHelp();
            exit(0);
        case 'i':
            if(strlen(optarg) < MAX_PATH)
            {
                strcpy(cml->input, optarg);
            }
            else
            {
                status = -1;
            }
            break;
        case 'I':
            if(strlen(optarg) < MAX_PATH)
            {
                strcpy(cml->inputThumb, optarg);
            }
            else
            {
                status = -1;
            }
            break;
        case 'o':
            if(strlen(optarg) < MAX_PATH)
            {
                strcpy(cml->output, optarg);
            }
            else
            {
                status = -1;
            }
            break;
        case 'C':
            if(strlen(optarg) < MAX_PATH)
            {
                strcpy(cml->com, optarg);
            }
            else
            {
                status = -1;
            }
            break;
        case 'a':
            cml->firstPic = atoi(optarg);
            break;
        case 'b':
            cml->lastPic = atoi(optarg);
            break;
        case 'x':
            cml->width = atoi(optarg);
            break;
        case 'y':
            cml->height = atoi(optarg);
            break;
        case 'w':
            cml->lumWidthSrc = atoi(optarg);
            break;
        case 'h':
            cml->lumHeightSrc = atoi(optarg);
            break;
        case 'X':
            cml->horOffsetSrc = atoi(optarg);
            break;
        case 'Y':
            cml->verOffsetSrc = atoi(optarg);
            break;
        case 'R':
            cml->restartInterval = atoi(optarg);
            break;
        case 'q':
            cml->qLevel = atoi(optarg);
            break;
        case 'g':
            cml->frameType = atoi(optarg);
            break;
        case 'v':
            cml->colorConversion = atoi(optarg);
            break;
        case 'G':
            cml->rotation = atoi(optarg);
            break;
        case 'p':
            cml->partialCoding = atoi(optarg);
            break;
        case 'm':
            cml->codingMode = atoi(optarg);
            break;
        case 't':
            cml->markerType = atoi(optarg);
            break;
        case 'u':
            cml->unitsType = atoi(optarg);
            break;
        case 'k':
            cml->xdensity = atoi(optarg);
            break;
        case 'l':
            cml->ydensity = atoi(optarg);
            break;
        case 'T':
            cml->thumbnail = atoi(optarg);
            break;
        case 'K':
            cml->widthThumb = atoi(optarg);
            break;
        case 'L':
            cml->heightThumb = atoi(optarg);
            break;
        case 'W':
            cml->write = atoi(optarg);
            break;
        case 'c':
            cml->comLength = atoi(optarg);
            break;
        case 'P':
            trigger_point = atoi(optarg);
            break;
        default:
            break;
        }
    }

    return status;
}

/*------------------------------------------------------------------------------

    JpegReadPic

    Read raw YUV image data from file
    Image is divided into slices, each slice consists of equal amount of 
    image rows except for the bottom slice which may be smaller than the 
    others. sliceNum is the number of the slice to be read
    and sliceRows is the amount of rows in each slice (or 0 for all rows).

------------------------------------------------------------------------------*/
int JpegReadPic(u8 * image, i32 width, i32 height, i32 sliceNum, i32 sliceRows,
            i32 frameNum, char *name, u32 inputMode)
{
    FS_FILE *file = NULL;
    i32 frameSize;
    i32 frameOffset;
    i32 sliceLumOffset = 0;
    i32 sliceCbOffset = 0;
    i32 sliceCrOffset = 0;
    i32 sliceLumSize;        /* The size of one slice in bytes */
    i32 sliceCbSize;
    i32 sliceCrSize;
    i32 sliceLumSizeRead;    /* The size of the slice to be read */
    i32 sliceCbSizeRead;
    i32 sliceCrSizeRead;

    if(sliceRows == 0)
    {
        sliceRows = height;
    }

    if(inputMode == 0)
    {
        /* YUV 4:2:0 planar */
        frameSize = width * height + (width / 2 * height / 2) * 2;
        sliceLumSizeRead = sliceLumSize = width * sliceRows;
        sliceCbSizeRead = sliceCbSize = width / 2 * sliceRows / 2;
        sliceCrSizeRead = sliceCrSize = width / 2 * sliceRows / 2;
    }
    else if(inputMode == 1)
    {
        /* YUV 4:2:0 semiplanar */
        frameSize = width * height + (width / 2 * height / 2) * 2;
        sliceLumSizeRead = sliceLumSize = width * sliceRows;
        sliceCbSizeRead = sliceCbSize = width * sliceRows / 2;
        sliceCrSizeRead = sliceCrSize = 0;
    }
    else if(inputMode <= 9)
    {
        /* YUYV 4:2:2, this includes both luminance and chrominance */
        frameSize = width * height * 2;
        sliceLumSizeRead = sliceLumSize = width * sliceRows * 2;
        sliceCbSizeRead = sliceCbSize = 0;
        sliceCrSizeRead = sliceCrSize = 0;
    }
    else
    {
        /* 32-bit RGB */
        frameSize = width * height * 4;
        sliceLumSizeRead = sliceLumSize = width * sliceRows * 4;
        sliceCbSizeRead = sliceCbSize = 0;
        sliceCrSizeRead = sliceCrSize = 0;
    }

    /* The bottom slice may be smaller than the others */
    if(sliceRows * (sliceNum + 1) > height)
    {
        sliceRows = height - sliceRows * sliceNum;

        if(inputMode == 0) {
            sliceLumSizeRead = width * sliceRows;
            sliceCbSizeRead = width / 2 * sliceRows / 2;
            sliceCrSizeRead = width / 2 * sliceRows / 2;
        } else if(inputMode == 1) {
            sliceLumSizeRead = width * sliceRows;
            sliceCbSizeRead = width * sliceRows / 2;
        } else if(inputMode <= 9) {
            sliceLumSizeRead = width * sliceRows * 2;
        } else {
            sliceLumSizeRead = width * sliceRows * 4;
        }
    }

    /* Offset for frame start from start of file */
    frameOffset = frameSize * frameNum;
    /* Offset for slice luma start from start of frame */
    sliceLumOffset = sliceLumSize * sliceNum;
    /* Offset for slice cb start from start of frame */
    if(inputMode <= 1)
        sliceCbOffset = width * height + sliceCbSize * sliceNum;
    /* Offset for slice cr start from start of frame */
    if(inputMode == 0)
        sliceCrOffset = width * height + 
                        width/2 * height/2 + sliceCrSize * sliceNum;

    /* Read input from file frame by frame */
#ifndef ASIC_WAVE_TRACE_TRIGGER
//	 printf("slice %d  ",sliceNum     );
/*    printf("Reading frame %d slice %d (%d bytes) from %s... ",
           frameNum, sliceNum, 
           sliceLumSizeRead + sliceCbSizeRead + sliceCrSizeRead, name);*/
    FS_CACHE_Clean("");//fflush(stdout);
#endif

    file = FS_FOpen(name, "rb");
    if(file == NULL)
    {
        printf( "\nUnable to open VOP file: %s\n", name);
        return -1;
    }

    FS_FSeek(file, frameOffset + sliceLumOffset, SEEK_SET);
	 
    /* Stop if last VOP of the file */
    if(FS_FEof(file))
    {
        printf( "\nI can't read VOP no: %d ", frameNum);
        printf( "from file: %s\n", name);
        FS_FClose(file);
        return -1;
    }
	 
	 dma_inv_range ((u32)image, sliceLumSizeRead + (u32)image);
    FS_FRead(image, 1, sliceLumSizeRead, file);
	 dma_flush_range ((u32)image, sliceLumSizeRead + (u32)image);
    if (sliceCbSizeRead) 
    {
        FS_FSeek(file, frameOffset + sliceCbOffset, SEEK_SET);
		  dma_inv_range ((u32)(image + sliceLumSize), sliceCbSizeRead + (u32)(image + sliceLumSize));
        FS_FRead(image + sliceLumSize, 1, sliceCbSizeRead, file);
		  dma_flush_range ((u32)(image + sliceLumSize), sliceCbSizeRead + (u32)(image + sliceLumSize));
    }
    if (sliceCrSizeRead) 
    {
        FS_FSeek(file, frameOffset + sliceCrOffset, SEEK_SET);
		  dma_inv_range ((u32)(image + sliceLumSize + sliceCbSize), sliceCrSizeRead + (u32)(image + sliceLumSize + sliceCbSize));
        FS_FRead(image + sliceLumSize + sliceCbSize, 1, sliceCrSizeRead, file);
		  dma_flush_range ((u32)(image + sliceLumSize + sliceCbSize), sliceCrSizeRead + (u32)(image + sliceLumSize + sliceCbSize));
    }



#ifndef ASIC_WAVE_TRACE_TRIGGER
    printf("OK\n");
    FS_CACHE_Clean("");//fflush(stdout);
#endif

    FS_FClose(file);

    return 0;
}

/*------------------------------------------------------------------------------

    JpegHelp

------------------------------------------------------------------------------*/
void JpegHelp(void)
{
    printf( "Usage:  %s [options] -i inputfile\n", "jpeg_testenc");
    printf(
            "  -H    --help              Display this help.\n"
            "  -W[n] --write             0=NO, 1=YES write output. [1]\n"
            "  -i[s] --input             Read input from file. [input.yuv]\n"
            "  -I[s] --inputThumb        Read thumbnail input from file. [thumbnail.jpg]\n"
            "  -o[s] --output            Write output to file. [stream.jpg]\n"
            "  -a[n] --firstPic          First picture of input file. [0]\n"
            "  -b[n] --lastPic           Last picture of input file. [0]\n"
            "  -w[n] --lumWidthSrc       Width of source image. [176]\n"
            "  -h[n] --lumHeightSrc      Height of source image. [144]\n");
    printf(
            "  -x[n] --width             Width of output image. [--lumWidthSrc]\n"
            "  -y[n] --height            Height of output image. [--lumHeightSrc]\n"
            "  -X[n] --horOffsetSrc      Output image horizontal offset. [0]\n"
            "  -Y[n] --verOffsetSrc      Output image vertical offset. [0]\n");
    printf(
            "  -R[n] --restartInterval   Restart interval in MCU rows. [0]\n"
            "  -q[n] --qLevel            0..10, quantization scale. [1]\n"
            "                            10 = use testbench defined qtable\n"
            "  -g[n] --frameType         Input YUV format. [0]\n"
            "                               0 - YUV420\n"
            "                               1 - YUV420 semiplanar\n"
            "                               2 - YUYV422\n"
            "                               3 - UYVY422\n"
            "                               4 - RGB565\n"
            "                               5 - BRG565\n"
            "                               6 - RGB555\n"
            "                               7 - BRG555\n"
            "                               8 - RGB444\n"
            "                               9 - BGR444\n"
            "                               10 - RGB888\n"
            "                               11 - BGR888\n"
            "                               12 - RGB101010\n"
            "                               13 - BGR101010\n"
            "  -v[n] --colorConversion   RGB to YCbCr color conversion type. [0]\n"
            "                               0 - BT.601\n"
            "                               1 - BT.709\n"
            "                               2 - User defined\n"
            "  -G[n] --rotation          Rotate input image. [0]\n"
            "                               0 - disabled\n"
            "                               1 - 90 degrees right\n"
            "                               2 - 90 degrees right\n"
            "  -p[n] --codingType        0=whole frame, 1=partial frame encoding. [0]\n"
            "  -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
            "  -t[n] --markerType        Quantization/Huffman table markers. [0]\n"
            "                               0 = single marker\n"
            "                               1 = multi marker\n"
            "  -u[n] --units             Units type of x- and y-density. [0]\n"
            "                               0 = pixel aspect ratio\n"
            "                               1 = dots/inch\n"
            "                               2 = dots/cm\n"
            "  -k[n] --xdensity          Xdensity to APP0 header. [1]\n"
            "  -l[n] --ydensity          Ydensity to APP0 header. [1]\n");
    printf(
            "  -T[n] --thumbnail         0=NO, 1=JPEG, 2=RGB8, 3=RGB24 Thumbnail to stream. [0]\n"
            "  -K[n] --widthThumb        Width of thumbnail output image. [32]\n"
            "  -L[n] --heightThumb       Height of thumbnail output image. [32]\n");
#ifdef TB_DEFINED_COMMENT
    printf( 
            "\n   Using comment values defined in testbench!\n");
#else
    printf(
            "  -c[n] --comLength         Comment header data length. [0]\n"
            "  -C[s] --comFile           Comment header data file. [com.txt]\n");
#endif
    printf(
            "\nTesting JpegParameters that are not supported for end-user:\n"
            "  -P[n] --trigger           Logic Analyzer trigger at picture <n>. [-1]\n"
            "\n");
}

/*------------------------------------------------------------------------------

    Write encoded stream to file

------------------------------------------------------------------------------*/
void JpegWriteStrm(FS_FILE * fout, u32 * strmbuf, u32 size, u32 endian)
{
	dma_inv_range ((u32)strmbuf, size + (u32)strmbuf);
    /* Swap the stream endianess before writing to file if needed */
    if(endian == 1)
    {
        u32 i = 0, words = (size + 3) / 4;

        while(words)
        {
            u32 val = strmbuf[i];
            u32 tmp = 0;

            tmp |= (val & 0xFF) << 24;
            tmp |= (val & 0xFF00) << 8;
            tmp |= (val & 0xFF0000) >> 8;
            tmp |= (val & 0xFF000000) >> 24;
            strmbuf[i] = tmp;
            words--;
            i++;
        }
			
		  dma_flush_range ((u32)strmbuf, size + (u32)strmbuf);
    }

    /* Write the stream to file */

#ifndef ASIC_WAVE_TRACE_TRIGGER
    printf("Writing stream (%i bytes)... ", size);
    FS_CACHE_Clean("");//fflush(stdout);
#endif

    FS_FWrite(strmbuf, 1, size, fout);

#ifndef ASIC_WAVE_TRACE_TRIGGER
    printf("OK\n");
    FS_CACHE_Clean("");//fflush(stdout);
#endif

}

#ifdef SEPARATE_FRAME_OUTPUT
/*------------------------------------------------------------------------------

    Write encoded frame to file

------------------------------------------------------------------------------*/
void JpegWriteFrame(char *filename, u32 * strmbuf, u32 size)
{
    FILE *fp;

    fp = FS_FOpen(filename, "ab");

        if(fp)
        {
            FS_FWrite(strmbuf, 1, size, fp);
            FS_FClose(fp);
        }
}
#endif

/*------------------------------------------------------------------------------
    JpegGetResolution
        Parse image resolution from file name
------------------------------------------------------------------------------*/
u32 JpegGetResolution(char *filename, i32 *pWidth, i32 *pHeight)
{
    i32 i;
    u32 w, h;
    i32 len = strlen(filename);
    i32 filenameBegin = 0;

    /* Find last '/' in the file name, it marks the beginning of file name */
    for (i = len-1; i; --i)
        if (filename[i] == '/') {
            filenameBegin = i+1;
            break;
        }

    /* If '/' found, it separates trailing path from file name */
    for (i = filenameBegin; i <= len-3; ++i)
    {
        if ((strncmp(filename+i, "subqcif", 7) == 0) ||
            (strncmp(filename+i, "sqcif", 5) == 0))
        {
            *pWidth = 128;
            *pHeight = 96;
            printf("Detected resolution SubQCIF (128x96) from file name.\n");
            return 0;
        }
        if (strncmp(filename+i, "qcif", 4) == 0)
        {
            *pWidth = 176;
            *pHeight = 144;
            printf("Detected resolution QCIF (176x144) from file name.\n");
            return 0;
        }
        if (strncmp(filename+i, "4cif", 4) == 0)
        {
            *pWidth = 704;
            *pHeight = 576;
            printf("Detected resolution 4CIF (704x576) from file name.\n");
            return 0;
        }
        if (strncmp(filename+i, "cif", 3) == 0)
        {
            *pWidth = 352;
            *pHeight = 288;
            printf("Detected resolution CIF (352x288) from file name.\n");
            return 0;
        }
        if (strncmp(filename+i, "qqvga", 5) == 0)
        {
            *pWidth = 160;
            *pHeight = 120;
            printf("Detected resolution QQVGA (160x120) from file name.\n");
            return 0;
        }
        if (strncmp(filename+i, "qvga", 4) == 0)
        {
            *pWidth = 320;
            *pHeight = 240;
            printf("Detected resolution QVGA (320x240) from file name.\n");
            return 0;
        }
        if (strncmp(filename+i, "vga", 3) == 0)
        {
            *pWidth = 640;
            *pHeight = 480;
            printf("Detected resolution VGA (640x480) from file name.\n");
            return 0;
        }
        if (strncmp(filename+i, "720p", 4) == 0)
        {
            *pWidth = 1280;
            *pHeight = 720;
            printf("Detected resolution 720p (1280x720) from file name.\n");
            return 0;
        }
        if (strncmp(filename+i, "1080p", 5) == 0)
        {
            *pWidth = 1920;
            *pHeight = 1080;
            printf("Detected resolution 1080p (1920x1080) from file name.\n");
            return 0;
        }
        if (filename[i] == 'x')
        {
            if (sscanf(filename+i-4, "%ux%u", &w, &h) == 2)
            {
                *pWidth = w;
                *pHeight = h;
                printf("Detected resolution %dx%d from file name.\n", w, h);
                return 0;
            }
            else if (sscanf(filename+i-3, "%ux%u", &w, &h) == 2)
            {
                *pWidth = w;
                *pHeight = h;
                printf("Detected resolution %dx%d from file name.\n", w, h);
                return 0;
            }
            else if (sscanf(filename+i-2, "%ux%u", &w, &h) == 2)
            {
                *pWidth = w;
                *pHeight = h;
                printf("Detected resolution %dx%d from file name.\n", w, h);
                return 0;
            }
        }
        if (filename[i] == 'w')
        {
            if (sscanf(filename+i, "w%uh%u", &w, &h) == 2)
            {
                *pWidth = w;
                *pHeight = h;
                printf("Detected resolution %dx%d from file name.\n", w, h);
                return 0;
            }
        }
    }

    return 1;   /* Error - no resolution found */
}

/*------------------------------------------------------------------------------

    API tracing

------------------------------------------------------------------------------*/
void JpegEnc_Trace(const char *msg)
{
    static FS_FILE *fp = NULL;

    if(fp == NULL)
        fp = FS_FOpen("api.trc", "wt");

    if(fp)
        printf( "%s\n", msg);
}
