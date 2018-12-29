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
#include "trace.h"

/* NOTE! This is needed when user allocated memory used */
#ifdef LINUX
#include <fcntl.h>
#include <sys/mman.h>
#endif /* #ifdef LINUX */

#include "jpegdecapi.h"
#include "dwl.h"
#include "jpegdeccontainer.h"
#include  "JpegDecTestBench.h"
#include  "DecGetOption.h"

#ifdef PP_PIPELINE_ENABLED
#include "ppapi.h"
#include "pptestbench.h"
#endif

#ifdef ASIC_TRACE_SUPPORT
#include "trace.h"
#endif

#include "deccfg.h"
#include "tb_cfg.h"
#include "regdrv.h"
#include "tb_sw_performance.h"
#include "fs.h"
#include "RTOS.H"

#ifndef MAX_PATH_
#define MAX_PATH   256  /* maximum lenght of the file path */
#endif

#define JPEG_INPUT_BUFFER 0x5120

/* SW/SW testing, read stream trace file */
FS_FILE *fStreamTrace = NULL;

static u32 memAllocation = 0;

/* memory parameters */
static u32 out_pic_size_luma;
static u32 out_pic_size_chroma;
//static fd_mem;
static JpegDecLinearMem outputAddressY;
static JpegDecLinearMem outputAddressCbCr;
static u32 frameReady = 0;
static u32 slicedOutputUsed = 0;
static u32 mode = 0;
static u32 writeOutput = 1;
static u32 sizeLuma = 0;/*计算出的亮度因子大小*/
static u32 sizeChroma = 0;/*计算的CbCr数据大小*/
static u32 sliceToUser = 0;
static u32 sliceSize = 0;
static u32 nonInterleaved = 0;
static i32 fullSliceCounter = -1;
static u32 output_picture_endian = DEC_X170_OUTPUT_PICTURE_ENDIAN;

static u8 outputfilename[MAX_PATH] = "\\JPEG_DEC.YUV";
static u8 outputfilename_slice[MAX_PATH];

/* stream start address */
u8 *byteStrmStart;

/* user allocated output */
DWLLinearMem_t userAllocLuma;
DWLLinearMem_t userAllocChroma;
DWLLinearMem_t userAllocCr;

/* progressive parameters */
static u32 scanCounter = 0;
static u32 progressive = 0;
static u32 scanReady = 0;
static u32 is8170HW = 0;

/* prototypes */
u32 allocMemory(JpegDecInst decInst, JpegDecImageInfo * imageInfo,
                JpegDecInput * jpegIn);
void calcSize(JpegDecImageInfo * imageInfo, u32 picMode);
void WriteOutput(u8 * dataLuma, u32 picSizeLuma, u8 * dataChroma,
                 u32 picSizeChroma, u32 picMode, JpegDecImageInfo *imageInfo,
					  char *output_filename);
void WriteOutputLuma(u8 * dataLuma, u32 picSizeLuma, u32 picMode);
void WriteOutputChroma(u8 * dataChroma, u32 picSizeChroma, u32 picMode);
void WriteFullOutput(u32 picMode);

void handleSlicedOutput(JpegDecImageInfo * imageInfo, JpegDecInput * jpegIn,
                        JpegDecOutput * jpegOut,
								char *output_filename);

void WriteCroppedOutput(JpegDecImageInfo * info, u8 * dataLuma, u8 * dataCb,
                        u8 * dataCr);

void WriteProgressiveOutput(u32 sizeLuma, u32 sizeChroma, u32 mode,
                            u8 * dataLuma, u8 * dataCb, u8 * dataCr,
									 char *output_filename);
void printJpegVersion(void);

void decsw_performance(void)
{
}

void cleanfilename_decode(char *filename)
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

void *JpegDecMalloc(unsigned int size);
void *JpegDecMemset(void *ptr, int c, unsigned int size);
void JpegDecFree(void *ptr);

void PrintJpegRet(JpegDecRet * pJpegRet);
u32 FindImageInfoEnd(u8 * pStream, u32 streamLength, u32 * pOffset);

u32 planarOutput = 0;
u32 bFrames = 0;

#ifdef ASIC_TRACE_SUPPORT
u32 picNumber;
#endif

//TBCfg tbCfg;

#ifdef ASIC_TRACE_SUPPORT
extern u32 useJpegIdct;
extern u32 gHwVer;
#endif

#ifdef JPEG_EVALUATION
extern u32 gHwVer;
#endif

static option_s decoptions[] = {
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

void JpegDecHelp(void)
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
            "  -X[n] --horOffsetSrc      not to write output picture. [0]\n"
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


int JpegDecParameter(i32 argc, char **argv, deccommandLine_s * cml)
{
    i32 ret;
    char *optarg;
    argument_s argument;
    int status = 0;

    memset(cml, DEFAULT, sizeof(deccommandLine_s));
//    strcpy(cml->input, "input.yuv");
    strcpy(cml->inputThumb, "thumbnail.jpg");
    strcpy(cml->com, "com.txt");
    strcpy(cml->output, "stream.jpg");
    cml->firstPic = 0;
  //  cml->lastPic = 10;
	 cml->lastPic = 10;
    cml->lumWidthSrc = DEFAULT;
    cml->lumHeightSrc = DEFAULT;
    cml->width = DEFAULT;
    cml->height = DEFAULT;
    cml->horOffsetSrc = 0;
    cml->verOffsetSrc = 0;
//    cml->qLevel = 9;
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

    argument.optCnt = 1;
    while((ret = DecGetOption(argc, argv, decoptions, &argument)) != -1)
    {
        if(ret == -2)
        {
            status = -1;
        }
        optarg = argument.optArg;
        switch (argument.shortOpt)
        {
        case 'H':
            JpegDecHelp();
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
        default:
            break;
        }
    }

    return status;
}

static int SliceStatus = 0;
void 	SetSlice(int Status)
{
	SliceStatus = Status;
}
static int SliceQOrder = 0;
void SetSliceQOrder(int QOrder)
{
	SliceQOrder = QOrder;
}


u32 crop = 0;
int jpeg_decode_main(int argc, char *argv[])
{
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
    i32 bufferSize = 0;
    u32 amountOfMCUs = 0;
    u32 mcuInRow = 0;
	 u32 tickscount;

    JpegDecInst jpeg;
    JpegDecRet jpegRet;
    JpegDecImageInfo imageInfo;
    JpegDecInput jpegIn;
    JpegDecOutput jpegOut;
    JpegDecApiVersion decVer;
    JpegDecBuild decBuild;

    DWLLinearMem_t streamMem;
	 deccommandLine_s cmdl;
    u8 *pImage = NULL;

    FS_FILE *fout = NULL;
    FS_FILE *fIn = NULL;
	 u8 filename[256];

    u32 clock_gating = DEC_X170_INTERNAL_CLOCK_GATING;
    u32 latency_comp = DEC_X170_LATENCY_COMPENSATION;
    u32 bus_burst_length = DEC_X170_BUS_BURST_LENGTH;
    u32 asic_service_priority = DEC_X170_ASIC_SERVICE_PRIORITY;
    u32 data_discard = DEC_X170_DATA_DISCARD_ENABLE;

#ifdef PP_PIPELINE_ENABLED
    PPApiVersion ppVer;
    PPBuild ppBuild;
#endif

    u32 streamHeaderCorrupt = 0;
    u32 seedRnd = 0;
    u32 streamBitSwap = 0;
    u32 streamTruncate = 0;
    FS_FILE *fTBCfg;
    u32 imageInfoLength = 0;

#ifdef ASIC_TRACE_SUPPORT
    gHwVer = 8190;   /* default to 8190 mode */
#endif

#ifdef JPEG_EVALUATION_8170
    gHwVer = 8170;
#elif JPEG_EVALUATION_8190
    gHwVer = 8190;
#elif JPEG_EVALUATION_9170
    gHwVer = 9170;
#elif JPEG_EVALUATION_9190
    gHwVer = 9190;
#elif JPEG_EVALUATION_G1
    gHwVer = 10000;
#endif
	 sliceSize = 0;
	 
    INIT_SW_PERFORMANCE;

    printf( "\n* * * * * * * * * * * * * * * * \n\n\n");
    printf( "      ");
    printf( "X170 JPEG TESTBENCH Decoder \n");
    printf( "\n\n* * * * * * * * * * * * * * * * \n");

    /* reset input */
    jpegIn.streamBuffer.pVirtualAddress = NULL;
    jpegIn.streamBuffer.busAddress = 0;
    jpegIn.streamLength = 0;
    jpegIn.pictureBufferY.pVirtualAddress = NULL;
    jpegIn.pictureBufferY.busAddress = 0;
    jpegIn.pictureBufferCbCr.pVirtualAddress = NULL;
    jpegIn.pictureBufferCbCr.busAddress = 0;

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
    bufferSize = 0;
    jpegIn.sliceMbSet = 0;
    jpegIn.bufferSize = 0;

#ifdef PP_PIPELINE_ENABLED
/*    if(argc < 2)
    {
#ifndef ASIC_TRACE_SUPPORT
        printf( "USAGE:\n%s [-X] [-S] [-P] stream.jpg\n", argv[0]);
        printf( "\t-X to not to write output picture\n");
        printf( "\t-S file.hex stream control trace file\n");
        printf( "\t-P write planar output\n");
		printJpegVersion();
#else
        printf( "USAGE:\n%s [-X] [-S] [-P] [-R] [-F] stream.jpg\n",
                argv[0]);
        printf( "\t-X to not to write output picture\n");
        printf( "\t-S file.hex stream control trace file\n");
        printf( "\t-P write planar output\n");
        printf( "\t-R use reference idct (implies cropping)\n");
        printf( "\t-F Force 8170 mode to HW model\n");
#endif
        exit(100);
    }*/

    /* read cmdl parameters */
/*    for(i = 1; i < argc - 1; i++)
    {
        if(strncmp(argv[i], "-X", 2) == 0)
        {
            writeOutput = 0;
        }
        else if(strncmp(argv[i], "-S", 2) == 0)
        {
            fStreamTrace = FS_FOpen((argv[i] + 2), "r");
        }
        else if(strncmp(argv[i], "-P", 2) == 0)
        {
            planarOutput = 1;
        }
#ifdef ASIC_TRACE_SUPPORT
        else if(strncmp(argv[i], "-R", 2) == 0)
        {
            useJpegIdct = 1;
            crop = 1;
        }
        else if(strcmp(argv[i], "-F") == 0)
        {
            gHwVer = 8170;
            is8170HW = 1;
            printf("\n\nForce 8170 mode to HW model!!!\n\n");
        }
#endif
        else
        {
            printf( "UNKNOWN PARAMETER: %s\n", argv[i]);
//            return 1;
        }
    }
*/

#else
/*    if(argc < 3)
    {
        printf( "USAGE:\n%s [-X] stream.jpg pp.cfg\n", argv[0]);
        printf( "\t-X to not to write output picture\n");
        printf( "\t-F Force 8170 mode to HW model\n");
        exit(100);
    }*/

    /* read cmdl parameters */
/*    for(i = 1; i < argc - 2; i++)
    {
        if(strncmp(argv[i], "-X", 2) == 0)
        {
            writeOutput = 0;
        }
        else if(strcmp(argv[i], "-F") == 0)
        {
            is8170HW = 1;
            printf("\n\nForce 8170 mode to HW model!!!\n\n");
        }
        else
        {
            printf( "UNKNOWN PARAMETER: %s\n", argv[i]);
            return 1;
        }
    }*/
#endif
    /* Print API and build version numbers */
    decVer = JpegGetAPIVersion();
    decBuild = JpegDecGetBuild();

    /* Version */
    printf(
            "\nX170 JPEG Decoder API v%d.%d - SW build: %d - HW build: %x\n",
            decVer.major, decVer.minor, decBuild.swBuild, decBuild.hwBuild);
		 
	 if(JpegDecParameter(argc, argv, &cmdl) != 0)
    {
        printf( "Input JpegDecParameter error\n");
        return -1;
    }
	 
	 
    /* check if 8170 HW */
    is8170HW = (decBuild.hwBuild >> 16) == 0x8170U ? 1 : 0;

    /******** PHASE 1 ********/
    printf( "\nPHASE 1: INIT JPEG DECODER\n");

    /* Jpeg initialization */
    START_SW_PERFORMANCE;
    decsw_performance();
    jpegRet = JpegDecInit(&jpeg);
    END_SW_PERFORMANCE;
    decsw_performance();
    if(jpegRet != JPEGDEC_OK)
    {
        /* Handle here the error situation */
        PrintJpegRet(&jpegRet);
        goto end;
    }

#ifdef PP_PIPELINE_ENABLED
    /* Initialize the post processer. If unsuccessful -> exit */
    if(pp_startup
       (cmdl.input, jpeg, PP_PIPELINED_DEC_TYPE_JPEG, &tbCfg) != 0)
    {
        printf( "PP INITIALIZATION FAILED\n");
        goto end;
    }

    if(pp_update_config
       (jpeg, PP_PIPELINED_DEC_TYPE_JPEG, &tbCfg) == CFG_UPDATE_FAIL)

    {
        printf( "PP CONFIG LOAD FAILED\n");
        goto end;
    }
#endif

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

    printf( "PHASE 1: INIT JPEG DECODER successful\n");//初始化

    /******** PHASE 2 ********/
    printf( "\nPHASE 2: OPEN/READ FILE \n");

#ifdef ASIC_TRACE_SUPPORT
    tmp = openTraceFiles();
    if(!tmp)
    {
        printf( "Unable to open trace file(s)\n");
    }
#endif

reallocate_input_buffer:

	 if(SliceStatus)
	 {//修改inputfilename and outputfilename outputfilename[MAX_PATH]
	 		memset(filename, 0 , MAX_PATH);
			memset(outputfilename, 0 , MAX_PATH);
			strncpy(filename , cmdl.input ,strlen(cmdl.input)-12);
			strncpy(outputfilename , cmdl.input ,strlen(cmdl.input)-12);
			sprintf(filename+strlen(cmdl.input)-12,"RESULT%02d.jpg", SliceQOrder );
			sprintf(outputfilename+strlen(cmdl.input)-12,"RESULT%02d.YUV", SliceQOrder );
			memset(cmdl.input, 0 , strlen(cmdl.input) );
			strcpy(cmdl.input, filename);
		//	jpegIn.sliceMbSet = 15;//240line 决定了slice的数量  *16
			jpegIn.sliceMbSet = 1;//16line 决定了slice的数量  *16
			
			memset( outputfilename_slice , 0 , MAX_PATH);
			strncpy(outputfilename_slice , outputfilename ,strlen(outputfilename)-12);
			sprintf(outputfilename_slice+strlen(outputfilename)-12,"%02d.YUV", SliceQOrder );
	 }
    /* Reading input file */ /*打开 输入文件 */

    fIn = FS_FOpen(cmdl.input, "rb");
	 printf("file name:%s\n",cmdl.input );
    if(fIn == NULL)
    {
        printf( "Unable to open input file %s \n", cmdl.input );
        return -1;
    }


    /* file i/o pointer to full */
    FS_FSeek(fIn, 0L, SEEK_END);
    len = FS_FTell(fIn);
 //   rewind(fIn);
	 FS_FSeek(fIn, 0L, SEEK_SET);//重定位到文件开始位置 Reposition to begin of file 

    /* Handle input buffer load *//*如果jpegIn.bufferSize为0，则 */
    if(jpegIn.bufferSize)
    {
        if(len > jpegIn.bufferSize)
        {
            streamTotalLen = len;
            len = jpegIn.bufferSize;
        }
        else
        {
            streamTotalLen = len;
            len = streamTotalLen;
            jpegIn.bufferSize = 0;
        }
    }
    else
    {
        jpegIn.bufferSize = 0;
        streamTotalLen = len;
    }

    streamInFile = streamTotalLen;

    /* NOTE: The DWL should not be used outside decoder SW
     * here we call it because it is the easiest way to get
     * dynamically allocated linear memory
     * */
    /* allocate memory for stream buffer. if unsuccessful -> exit */
    streamMem.virtualAddress = NULL;
    streamMem.busAddress = 0;

    /* allocate memory for stream buffer. if unsuccessful -> exit */
    if(DWLMallocLinear
       (((JpegDecContainer *) jpeg)->dwl, len, &streamMem) != DWL_OK)
    {
        printf( "UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n");
        goto end;
    }

    printf( "\t-Input: Allocated buffer: virt: 0x%08x bus: 0x%08x\n",
            streamMem.virtualAddress, streamMem.busAddress);

    /* memset input */
    (void) DWLmemset(streamMem.virtualAddress, 0, len);

    byteStrmStart = (u8 *) streamMem.virtualAddress;
    if(byteStrmStart == NULL)
    {
        printf( "UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n");
        goto end;
    }

    /* read input stream from file to buffer and close input file */
    FS_FRead(byteStrmStart, sizeof(u8), len, fIn);

    FS_FClose(fIn);

    /* initialize JpegDecDecode input structure */
    jpegIn.streamBuffer.pVirtualAddress = (u32 *) byteStrmStart;
    jpegIn.streamBuffer.busAddress = streamMem.busAddress;
    jpegIn.streamLength = streamTotalLen;

    if(writeOutput)
        printf( "\t-File: Write output: YES: %d\n", writeOutput);
    else
        printf( "\t-File: Write output: NO: %d\n", writeOutput);
    printf( "\t-File: MbRows/slice: %d\n", jpegIn.sliceMbSet);
    printf( "\t-File: Buffer size: %d\n", jpegIn.bufferSize);
    printf( "\t-File: Stream size: %d\n", jpegIn.streamLength);

    if(memAllocation)
        printf( "\t-File: Output allocated by USER: %d\n",
                memAllocation);
    else
        printf( "\t-File: Output allocated by DECODER: %d\n",
                memAllocation);

    printf( "\nPHASE 2: OPEN/READ FILE successful\n");

    /******** PHASE 3 ********/
    printf( "\nPHASE 3: GET IMAGE INFO\n");// 获取图片文件信息 
    /* Get image information of the JFIF and decode JFIF header */
    START_SW_PERFORMANCE;
    decsw_performance();
    jpegRet = JpegDecGetImageInfo(jpeg, &jpegIn, &imageInfo);
    END_SW_PERFORMANCE;
    decsw_performance();
    if(jpegRet != JPEGDEC_OK)
    {
        /* Handle here the error situation */
        PrintJpegRet(&jpegRet);
        if(JPEGDEC_INCREASE_INPUT_BUFFER == jpegRet)
        {
            DWLFreeLinear(((JpegDecContainer *) jpeg)->dwl, &streamMem);
            jpegIn.bufferSize += 256;
            goto reallocate_input_buffer;
        }
        else
        {
            goto end;
        }
    }

    /*  ******************** THUMBNAIL **************************** */
    /* Select if Thumbnail or full resolution image will be decoded */
    if(imageInfo.thumbnailType == JPEGDEC_THUMBNAIL_JPEG)
    {
        /* decode thumbnail */
        printf( "\t-JPEG THUMBNAIL IN STREAM\n");
        printf( "\t-JPEG THUMBNAIL INFO\n");
        printf( "\t\t-JPEG thumbnail width: %d\n",
                imageInfo.outputWidthThumb);
        printf( "\t\t-JPEG thumbnail height: %d\n",
                imageInfo.outputHeightThumb);

        /* stream type */
        switch (imageInfo.codingModeThumb)
        {
        case JPEGDEC_BASELINE:
            printf( "\t\t-JPEG: STREAM TYPE: JPEGDEC_BASELINE\n");
            break;
        case JPEGDEC_PROGRESSIVE:
            printf( "\t\t-JPEG: STREAM TYPE: JPEGDEC_PROGRESSIVE\n");
            break;
        case JPEGDEC_NONINTERLEAVED:
            printf( "\t\t-JPEG: STREAM TYPE: JPEGDEC_NONINTERLEAVED\n");
            break;
        }

        if(imageInfo.outputFormatThumb)
        {
            switch (imageInfo.outputFormatThumb)
            {
            case JPEGDEC_YCbCr400:
                printf(
                        "\t\t-JPEG: THUMBNAIL OUTPUT: JPEGDEC_YCbCr400\n");
                break;
            case JPEGDEC_YCbCr420_SEMIPLANAR:
                printf(
                        "\t\t-JPEG: THUMBNAIL OUTPUT: JPEGDEC_YCbCr420_SEMIPLANAR\n");
                break;
            case JPEGDEC_YCbCr422_SEMIPLANAR:
                printf(
                        "\t\t-JPEG: THUMBNAIL OUTPUT: JPEGDEC_YCbCr422_SEMIPLANAR\n");
                break;
            case JPEGDEC_YCbCr440:
                printf(
                        "\t\t-JPEG: THUMBNAIL OUTPUT: JPEGDEC_YCbCr440\n");
                break;
            case JPEGDEC_YCbCr411_SEMIPLANAR:
                printf(
                        "\t\t-JPEG: THUMBNAIL OUTPUT: JPEGDEC_YCbCr411_SEMIPLANAR\n");
                break;
            case JPEGDEC_YCbCr444_SEMIPLANAR:
                printf(
                        "\t\t-JPEG: THUMBNAIL OUTPUT: JPEGDEC_YCbCr444_SEMIPLANAR\n");
                break;
            }
        }
        jpegIn.decImageType = JPEGDEC_THUMBNAIL;
    }
    else if(imageInfo.thumbnailType == JPEGDEC_NO_THUMBNAIL)
    {
        /* decode full image */
        printf(
                "\t-NO THUMBNAIL IN STREAM ==> Decode full resolution image\n");
        jpegIn.decImageType = JPEGDEC_IMAGE;
    }
    else if(imageInfo.thumbnailType == JPEGDEC_THUMBNAIL_NOT_SUPPORTED_FORMAT)
    {
        /* decode full image */
        printf(
                "\tNOT SUPPORTED THUMBNAIL IN STREAM ==> Decode full resolution image\n");
        jpegIn.decImageType = JPEGDEC_IMAGE;
    }

    printf( "\t-JPEG FULL RESOLUTION INFO\n");
    printf( "\t\t-JPEG width: %d\n", imageInfo.outputWidth);
    printf( "\t\t-JPEG height: %d\n", imageInfo.outputHeight);
    if(imageInfo.outputFormat)
    {
        switch (imageInfo.outputFormat)
        {
        case JPEGDEC_YCbCr420_SEMIPLANAR:
            printf(
                    "\t\t-JPEG: FULL RESOLUTION OUTPUT: JPEGDEC_YCbCr420_SEMIPLANAR\n");
#ifdef ASIC_TRACE_SUPPORT
            trace_jpegDecodingTools.sampling_4_2_0 = 1;
#endif
            break;
        case JPEGDEC_YCbCr400:
            printf(
                    "\t\t-JPEG: FULL RESOLUTION OUTPUT: JPEGDEC_YCbCr400\n");
#ifdef ASIC_TRACE_SUPPORT
            trace_jpegDecodingTools.sampling_4_0_0 = 1;
#endif
            break;
        case JPEGDEC_YCbCr422_SEMIPLANAR:
            printf(
                    "\t\t-JPEG: FULL RESOLUTION OUTPUT: JPEGDEC_YCbCr422_SEMIPLANAR\n");
#ifdef ASIC_TRACE_SUPPORT
            trace_jpegDecodingTools.sampling_4_2_2 = 1;
#endif
            break;
        case JPEGDEC_YCbCr440:
            printf(
                    "\t\t-JPEG: FULL RESOLUTION OUTPUT: JPEGDEC_YCbCr440\n");
#ifdef ASIC_TRACE_SUPPORT
            trace_jpegDecodingTools.sampling_4_4_0 = 1;
#endif
            break;
        case JPEGDEC_YCbCr411_SEMIPLANAR:
            printf(
                    "\t\t-JPEG: FULL RESOLUTION OUTPUT: JPEGDEC_YCbCr411_SEMIPLANAR\n");
#ifdef ASIC_TRACE_SUPPORT
            trace_jpegDecodingTools.sampling_4_1_1 = 1;
#endif
            break;
        case JPEGDEC_YCbCr444_SEMIPLANAR:
            printf(
                    "\t\t-JPEG: FULL RESOLUTION OUTPUT: JPEGDEC_YCbCr444_SEMIPLANAR\n");
#ifdef ASIC_TRACE_SUPPORT
            trace_jpegDecodingTools.sampling_4_4_4 = 1;
#endif
            break;
        }
    }

    /* stream type */
    switch (imageInfo.codingMode)
    {
    case JPEGDEC_BASELINE:
        printf( "\t\t-JPEG: STREAM TYPE: JPEGDEC_BASELINE\n");
        break;
    case JPEGDEC_PROGRESSIVE:
        printf( "\t\t-JPEG: STREAM TYPE: JPEGDEC_PROGRESSIVE\n");
#ifdef ASIC_TRACE_SUPPORT
        trace_jpegDecodingTools.progressive = 1;
#endif
        break;
    case JPEGDEC_NONINTERLEAVED:
        printf( "\t\t-JPEG: STREAM TYPE: JPEGDEC_NONINTERLEAVED\n");
        break;
    }

    if(imageInfo.thumbnailType == JPEGDEC_THUMBNAIL_JPEG)
    {
        printf( "\t-JPEG ThumbnailType: JPEG\n");
#ifdef ASIC_TRACE_SUPPORT
        trace_jpegDecodingTools.thumbnail = 1;
#endif
    }
    else if(imageInfo.thumbnailType == JPEGDEC_NO_THUMBNAIL)
        printf( "\t-JPEG ThumbnailType: NO THUMBNAIL\n");
    else if(imageInfo.thumbnailType == JPEGDEC_THUMBNAIL_NOT_SUPPORTED_FORMAT)
        printf( "\t-JPEG ThumbnailType: NOT SUPPORTED THUMBNAIL\n");

    printf( "PHASE 3: GET IMAGE INFO successful\n");

    /* TB SPECIFIC == LOOP IF THUMBNAIL IN JFIF */
    /* Decode JFIF *//* 决定是分片处理 还是整一帧处理 */
	 /* 此处应该为决定是否有预览图*/
    if(jpegIn.decImageType == JPEGDEC_THUMBNAIL)
        frameCounter = 2;
    else
        frameCounter = 1;

#ifdef ASIC_TRACE_SUPPORT
    /* Handle incorrect slice size for HW testing */
    if(jpegIn.sliceMbSet > (imageInfo.outputHeight >> 4))
    {
        jpegIn.sliceMbSet = (imageInfo.outputHeight >> 4);
        printf("FIXED Decoder Slice MB Set %d\n", jpegIn.sliceMbSet);
    }
#endif

    /* no slice mode supported in progressive || non-interleaved ==> force to full mode */
    if((jpegIn.decImageType == JPEGDEC_THUMBNAIL &&
        imageInfo.codingModeThumb == JPEGDEC_PROGRESSIVE) ||
       (jpegIn.decImageType == JPEGDEC_IMAGE &&
        imageInfo.codingMode == JPEGDEC_PROGRESSIVE))
        jpegIn.sliceMbSet = 0;

    /******** PHASE 4 ********/
    /* Decode Image */
    for(loop = 0; loop < frameCounter; loop++)/* frameCounter 表示预览图or整帧处理 */
    {
        /* Tn */
        if(loop == 0 && frameCounter > 1)
        {
            printf( "\nPHASE 4: DECODE FRAME: THUMBNAIL\n");
            /* thumbnail mode */
            mode = 1;
        }
        else if(loop == 0 && frameCounter == 1)
        {
            printf( "\nPHASE 4: DECODE FRAME: FULL RESOLUTION\n");
            /* full mode */
            mode = 0;
        }
        else
        {
            printf( "\nPHASE 4: DECODE FRAME: FULL RESOLUTION\n");
            /* full mode */
            mode = 0;
        }

        /* if input (only full, not tn) > 4096 MCU      */
        /* ==> force to slice mode                                      */
        if(mode == 0)
        {
            /* calculate MCU's */
            if(imageInfo.outputFormat == JPEGDEC_YCbCr400 ||
               imageInfo.outputFormat == JPEGDEC_YCbCr444_SEMIPLANAR)
            {
                amountOfMCUs =
                    ((imageInfo.outputWidth * imageInfo.outputHeight) / 64);
                mcuInRow = (imageInfo.outputWidth / 8);
            }
            else if(imageInfo.outputFormat == JPEGDEC_YCbCr420_SEMIPLANAR)
            {
                /* 265 is the amount of luma samples in MB for 4:2:0 */
                amountOfMCUs =
                    ((imageInfo.outputWidth * imageInfo.outputHeight) / 256);
                mcuInRow = (imageInfo.outputWidth / 16);
            }
            else if(imageInfo.outputFormat == JPEGDEC_YCbCr422_SEMIPLANAR)
            {
                /* 128 is the amount of luma samples in MB for 4:2:2 */
                amountOfMCUs =
                    ((imageInfo.outputWidth * imageInfo.outputHeight) / 128);
                mcuInRow = (imageInfo.outputWidth / 16);
            }
            else if(imageInfo.outputFormat == JPEGDEC_YCbCr440)
            {
                /* 128 is the amount of luma samples in MB for 4:4:0 */
                amountOfMCUs =
                    ((imageInfo.outputWidth * imageInfo.outputHeight) / 128);
                mcuInRow = (imageInfo.outputWidth / 8);
            }
            else if(imageInfo.outputFormat == JPEGDEC_YCbCr411_SEMIPLANAR)
            {
                amountOfMCUs =
                    ((imageInfo.outputWidth * imageInfo.outputHeight) / 256);
                mcuInRow = (imageInfo.outputWidth / 32);
            }

            /* set mcuSizeDivider for slice size count */
            if(imageInfo.outputFormat == JPEGDEC_YCbCr400 ||
               imageInfo.outputFormat == JPEGDEC_YCbCr440 ||
               imageInfo.outputFormat == JPEGDEC_YCbCr444_SEMIPLANAR)
                mcuSizeDivider = 2;
            else
                mcuSizeDivider = 1;

#ifdef ASIC_TRACE_SUPPORT
            if(is8170HW)
            {
                /* over max MCU ==> force to slice mode */
                if((jpegIn.sliceMbSet == 0) &&
                   (amountOfMCUs > JPEGDEC_MAX_SLICE_SIZE))
                {
                    do
                    {
                        jpegIn.sliceMbSet++;
                    }
                    while(((jpegIn.sliceMbSet * (mcuInRow / mcuSizeDivider)) +
                           (mcuInRow / mcuSizeDivider)) <
                          JPEGDEC_MAX_SLICE_SIZE);
                    printf("Force to slice mode ==> Decoder Slice MB Set %d\n",
                           jpegIn.sliceMbSet);
                }
            }
            else
            {
                /* 8190 and over 16M ==> force to slice mode */
                if((jpegIn.sliceMbSet == 0) &&
                   ((imageInfo.outputWidth * imageInfo.outputHeight) >
                    JPEGDEC_MAX_PIXEL_AMOUNT))
                {
                    do
                    {
                        jpegIn.sliceMbSet++;
                    }
                    while(((jpegIn.sliceMbSet * (mcuInRow / mcuSizeDivider)) +
                           (mcuInRow / mcuSizeDivider)) <
                          JPEGDEC_MAX_SLICE_SIZE_8190);
                    printf
                        ("Force to slice mode (over 16M) ==> Decoder Slice MB Set %d\n",
                         jpegIn.sliceMbSet);
                }
            }
#else
            if(is8170HW)
            {
                /* over max MCU ==> force to slice mode */
                if((jpegIn.sliceMbSet == 0) &&
                   (amountOfMCUs > JPEGDEC_MAX_SLICE_SIZE))
                {
                    do
                    {
                        jpegIn.sliceMbSet++;
                    }
                    while(((jpegIn.sliceMbSet * (mcuInRow / mcuSizeDivider)) +
                           (mcuInRow / mcuSizeDivider)) <
                          JPEGDEC_MAX_SLICE_SIZE);
                    printf("Force to slice mode ==> Decoder Slice MB Set %d\n",
                           jpegIn.sliceMbSet);
                }
            }
            else
            {
                /* 8190 and over 16M ==> force to slice mode */
                if((jpegIn.sliceMbSet == 0) &&
                   ((imageInfo.outputWidth * imageInfo.outputHeight) >
                    JPEGDEC_MAX_PIXEL_AMOUNT))
                {
                    do
                    {
                        jpegIn.sliceMbSet++;
                    }
                    while(((jpegIn.sliceMbSet * (mcuInRow / mcuSizeDivider)) +
                           (mcuInRow / mcuSizeDivider)) <
                          JPEGDEC_MAX_SLICE_SIZE_8190);
                    printf
                        ("Force to slice mode (over 16M) ==> Decoder Slice MB Set %d\n",
                         jpegIn.sliceMbSet);
                }
            }
#endif
        }

        /* if user allocated memory */
        if(memAllocation)
        {
            printf( "\n\t-JPEG: USER ALLOCATED MEMORY\n");
            jpegRet = allocMemory(jpeg, &imageInfo, &jpegIn);
            if(jpegRet != JPEGDEC_OK)
            {
                /* Handle here the error situation */
                PrintJpegRet(&jpegRet);
                goto end;
            }
            printf( "\t-JPEG: USER ALLOCATED MEMORY successful\n\n");
        }

        /* decode 解码*/
        do
        {
            START_SW_PERFORMANCE;
   //         decsw_performance();
            tickscount = XM_GetTickCount();
            jpegRet = JpegDecDecode(jpeg, &jpegIn, &jpegOut);
            tickscount = XM_GetTickCount() - tickscount ;
            printf("JpegDecDecode one frame cost %d ms\n", tickscount);
            END_SW_PERFORMANCE;
   //         decsw_performance();

            if(jpegRet == JPEGDEC_FRAME_READY)
            {
                printf( "\t-JPEG: JPEGDEC_FRAME_READY\n");

                /* check if progressive ==> planar output */
                if((imageInfo.codingMode == JPEGDEC_PROGRESSIVE && mode == 0) ||
                   (imageInfo.codingModeThumb == JPEGDEC_PROGRESSIVE &&
                    mode == 1))
                {
                    progressive = 1;
                }

                if((imageInfo.codingMode == JPEGDEC_NONINTERLEAVED && mode == 0)
                   || (imageInfo.codingModeThumb == JPEGDEC_NONINTERLEAVED &&
                       mode == 1))
                    nonInterleaved = 1;
                else
                    nonInterleaved = 0;

                if(jpegIn.sliceMbSet && fullSliceCounter == -1)
                    slicedOutputUsed = 1;

                /* info to handleSlicedOutput */
                frameReady = 1;
            }
            else if(jpegRet == JPEGDEC_SCAN_PROCESSED)
            {
                /* TODO! Progressive scan ready... */
                printf( "\t-JPEG: JPEGDEC_SCAN_PROCESSED\n");

                /* progressive ==> planar output */
                if(imageInfo.codingMode == JPEGDEC_PROGRESSIVE)
                    progressive = 1;

                /* info to handleSlicedOutput */
                printf("SCAN %d READY\n", scanCounter);

                if(imageInfo.codingMode == JPEGDEC_PROGRESSIVE)
                {
                    /* calculate size for output */
                    calcSize(&imageInfo, mode);

                    printf("sizeLuma %d and sizeChroma %d\n", sizeLuma,
                           sizeChroma);

                    WriteProgressiveOutput(sizeLuma, sizeChroma, mode,
                                           (u8*)jpegOut.outputPictureY.
                                           pVirtualAddress,
                                           (u8*)jpegOut.outputPictureCbCr.
                                           pVirtualAddress,
                                           (u8*)jpegOut.outputPictureCr.
                                           pVirtualAddress,
														 cmdl.output);

                    scanCounter++;
                }

                /* update/reset */
                progressive = 0;
                scanReady = 0;

            }
            else if(jpegRet == JPEGDEC_SLICE_READY)
            {
                printf( "\t-JPEG: JPEGDEC_SLICE_READY\n");

                slicedOutputUsed = 1;
                if((imageInfo.codingMode == JPEGDEC_NONINTERLEAVED && mode == 0)
                   || (imageInfo.codingModeThumb == JPEGDEC_NONINTERLEAVED &&
                       mode == 1))
                    nonInterleaved = 1;
                else
                    nonInterleaved = 0;
                /* calculate/write output of slice 
                 * and update output budder in case of 
                 * user allocated memory */
					 
					 printf("-JPEG width: %d", imageInfo.outputWidth );
					 printf("-JPEG height: %d", imageInfo.outputHeight );
					 printf("-JPEG Slice height: %d", jpegIn.sliceMbSet*16  );
                if(jpegOut.outputPictureY.pVirtualAddress != NULL)
                    handleSlicedOutput(&imageInfo, &jpegIn, &jpegOut, cmdl.output);
					 
                scanCounter++;
            }
            else if(jpegRet == JPEGDEC_STRM_PROCESSED)
            {
                printf(
                        "\t-JPEG: JPEGDEC_STRM_PROCESSED ==> Load input buffer\n");

                /* update seek value */
                streamInFile -= len;
                streamSeekLen += len;

                if(streamInFile <= 0)
                {
                    printf( "\t\t==> Unable to load input buffer\n");
                    printf(
                            "\t\t\t==> TRUNCATED INPUT ==> JPEGDEC_STRM_ERROR\n");
                    jpegRet = JPEGDEC_STRM_ERROR;
                    goto strm_error;
                }

                if(streamInFile < len)
                {
                    len = streamInFile;
                }
				
                /* update the buffer size in case last buffer 
                   doesn't have the same amount of data as defined 更新缓存大小，避免*/
                if(len < jpegIn.bufferSize)
                {
                    jpegIn.bufferSize = len;
                }


                /* Reading input file */
                fIn = FS_FOpen(cmdl.input, "rb");
                if(fIn == NULL)
                {
                    printf( "Unable to open input file\n");
                    exit(-1);
                }



                /* file i/o pointer to full */
                FS_FSeek(fIn, streamSeekLen, SEEK_SET);
                /* read input stream from file to buffer and close input file */
                FS_FRead(byteStrmStart, sizeof(u8), len, fIn);
                FS_FClose(fIn);

                /* update */
                jpegIn.streamBuffer.pVirtualAddress = (u32 *) byteStrmStart;
                jpegIn.streamBuffer.busAddress = streamMem.busAddress;
            }
            else if(jpegRet == JPEGDEC_STRM_ERROR)
            {
              strm_error:

              if(jpegIn.sliceMbSet && fullSliceCounter == -1)
                  slicedOutputUsed = 1;    

                /* calculate/write output of slice 
                 * and update output budder in case of 
                 * user allocated memory */
                if(slicedOutputUsed &&
                   jpegOut.outputPictureY.pVirtualAddress != NULL)
                    handleSlicedOutput(&imageInfo, &jpegIn, &jpegOut, cmdl.output);

                /* info to handleSlicedOutput */
                frameReady = 1;
                slicedOutputUsed = 0;

                /* Handle here the error situation */
                PrintJpegRet(&jpegRet);
					 printf("JPEGDEC_STRM_ERROR line:%d\n",__LINE__);
                if(mode == 1)
                    break;
                else
                    goto error;
            }
            else
            {
                /* Handle here the error situation */
                PrintJpegRet(&jpegRet);
                goto end;
            }
        }
        while(jpegRet != JPEGDEC_FRAME_READY);

        /* new config *//*完成一帧解码后，继续下面的代码逻辑*/
        if(frameCounter > 1 && loop == 0)/*分片 */
        {
            /* calculate/write output of slice */
            if(slicedOutputUsed &&
               jpegOut.outputPictureY.pVirtualAddress != NULL)
            {
                handleSlicedOutput(&imageInfo, &jpegIn, &jpegOut, cmdl.output);
                slicedOutputUsed = 0;
            }

            /* thumbnail mode */
            mode = 1;

            /* calculate size for output */
            calcSize(&imageInfo, mode);

            printf( "\t-JPEG: ++++++++++ THUMBNAIL ++++++++++\n");
            printf( "\t-JPEG: Instance %x\n",
                    (JpegDecContainer *) jpeg);
            printf( "\t-JPEG: Luma output: 0x%08x size: %d\n",
                    jpegOut.outputPictureY.pVirtualAddress, sizeLuma);
            printf( "\t-JPEG: Chroma output: 0x%08x size: %d\n",
                    jpegOut.outputPictureCbCr.pVirtualAddress, sizeChroma);
            printf( "\t-JPEG: Luma output bus: 0x%08x\n",
                    (u8 *) jpegOut.outputPictureY.busAddress);
            printf( "\t-JPEG: Chroma output bus: 0x%08x\n",
                    (u8 *) jpegOut.outputPictureCbCr.busAddress);

            printf( "PHASE 4: DECODE FRAME: THUMBNAIL successful\n");

            /* if output write not disabled by TB */
            if(writeOutput)
            {
                /******** PHASE 5 ********/
                printf( "\nPHASE 5: WRITE OUTPUT\n");

                if(imageInfo.outputFormat)
                {
                    switch (imageInfo.outputFormat)
                    {
                    case JPEGDEC_YCbCr400:
                        printf(
                                "\t-JPEG: DECODER OUTPUT: JPEGDEC_YCbCr400\n");
                        break;
                    case JPEGDEC_YCbCr420_SEMIPLANAR:
                        printf(
                                "\t-JPEG: DECODER OUTPUT: JPEGDEC_YCbCr420_SEMIPLANAR\n");
                        break;
                    case JPEGDEC_YCbCr422_SEMIPLANAR:
                        printf(
                                "\t-JPEG: DECODER OUTPUT: JPEGDEC_YCbCr422_SEMIPLANAR\n");
                        break;
                    case JPEGDEC_YCbCr440:
                        printf(
                                "\t-JPEG: DECODER OUTPUT: JPEGDEC_YCbCr440\n");
                        break;
                    case JPEGDEC_YCbCr411_SEMIPLANAR:
                        printf(
                                "\t-JPEG: DECODER OUTPUT: JPEGDEC_YCbCr411_SEMIPLANAR\n");
                        break;
                    case JPEGDEC_YCbCr444_SEMIPLANAR:
                        printf(
                                "\t-JPEG: DECODER OUTPUT: JPEGDEC_YCbCr444_SEMIPLANAR\n");
                        break;
                    }
                }
#ifndef PP_PIPELINE_ENABLED
                /* write output */
                if(jpegIn.sliceMbSet)
                {
                    if(imageInfo.outputFormat != JPEGDEC_YCbCr400)
                        WriteFullOutput(mode);
                }
                else
                {
                    WriteOutput(((u8 *) jpegOut.outputPictureY.pVirtualAddress),
                                sizeLuma,
                                ((u8 *) jpegOut.outputPictureCbCr.
                                 pVirtualAddress), sizeChroma, mode,&imageInfo,
										  cmdl.output);
                }
#else
                /* PP test bench will do the operations only if enabled */
                /*pp_set_rotation(); */

                printf("PP_OUTPUT_WRITE\n");
                pp_write_output(0, 0, 0);
                pp_check_combined_status();
#endif

                printf( "PHASE 5: WRITE OUTPUT successful\n");
            }
            else
            {
                printf( "\nPHASE 5: WRITE OUTPUT DISABLED\n");
            }

            /******** PHASE 6 ********/
            printf( "\nPHASE 6: RELEASE JPEG DECODER\n");

            if(streamMem.virtualAddress != NULL)
                DWLFreeLinear(((JpegDecContainer *) jpeg)->dwl, &streamMem);

            if(userAllocLuma.virtualAddress != NULL)
                DWLFreeRefFrm(((JpegDecContainer *) jpeg)->dwl, &userAllocLuma);

            if(userAllocChroma.virtualAddress != NULL)
                DWLFreeRefFrm(((JpegDecContainer *) jpeg)->dwl,
                              &userAllocChroma);

            if(userAllocCr.virtualAddress != NULL)
                DWLFreeRefFrm(((JpegDecContainer *) jpeg)->dwl, &userAllocCr);

            /* release decoder instance */
            START_SW_PERFORMANCE;
            decsw_performance();
            JpegDecRelease(jpeg);
            END_SW_PERFORMANCE;
            decsw_performance();

            printf( "PHASE 6: RELEASE JPEG DECODER successful\n\n");

            if(inputReadType)
            {
                if(fIn)
                {
                    FS_FClose(fIn);
                }
            }

            if(fout)
            {
                FS_FClose(fout);
            }

            printf( "Decode: ...released\n");

            /* thumbnail done ==> set mode to full */
            mode = 0;
#if 0
            /* Print API and build version numbers */
            decVer = JpegGetAPIVersion();
            decBuild = JpegDecGetBuild();

            /* Version */
            printf(
                    "\nX170 JPEG Decoder API v%d.%d - SW build: %d - HW build: %x\n",
                    decVer.major, decVer.minor, decBuild.swBuild,
                    decBuild.hwBuild);
#endif
#ifdef ASIC_TRACE_SUPPORT
            /* update picture counter for tracing */
            picNumber++;
#endif

            /******** PHASE 1 ********/
            printf( "\nPHASE 1: INIT JPEG DECODER\n");

            /* Jpeg initialization */
            START_SW_PERFORMANCE;
            decsw_performance();
            jpegRet = JpegDecInit(&jpeg);
            END_SW_PERFORMANCE;
            decsw_performance();
            if(jpegRet != JPEGDEC_OK)
            {
                /* Handle here the error situation */
                PrintJpegRet(&jpegRet);
                goto end;
            }

            /* NOTE: The registers should not be used outside decoder SW for other
             * than compile time setting test purposes */
            printf("---Clock Gating %d---\n", clock_gating);
            printf("---Latency Compensation %d---\n", latency_comp);
            printf("---Output Picture Endian %d---\n", output_picture_endian);
            printf("---Bus Burst Length %d---\n", bus_burst_length);
            printf("---Asic Service Priority %d---\n", asic_service_priority);
            printf("---Data Discard  %d---\n", data_discard);
            FS_CACHE_Clean("");//fflush(stdout);

            SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs,
                           HWIF_DEC_LATENCY, latency_comp);
            SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs,
                           HWIF_DEC_CLK_GATE_E, clock_gating);
            SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs,
                           HWIF_DEC_OUT_ENDIAN, output_picture_endian);
            SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs,
                           HWIF_DEC_MAX_BURST, bus_burst_length);
            if ((DWLReadAsicID() >> 16) == 0x8170U)
            {   
                SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs,
                               HWIF_PRIORITY_MODE, asic_service_priority);
            }
            SetDecRegister(((JpegDecContainer *) jpeg)->jpegRegs,
                           HWIF_DEC_DATA_DISC_E, data_discard);

            printf( "PHASE 1: INIT JPEG DECODER successful\n");

            /******** PHASE 2 ********/
            printf( "\nPHASE 2: SET FILE IO\n");

            /******** PHASE 2 ********/
            printf( "\nPHASE 2: OPEN/READ FILE \n");

            /* Reading input file */
            fIn = FS_FOpen(cmdl.input, "rb");
            if(fIn == NULL)
            {
                printf( "Unable to open input file\n");
                exit(-1);
            }

            /* file i/o pointer to full */
            FS_FSeek(fIn, 0L, SEEK_END);
            len = FS_FTell(fIn);
            //rewind(fIn);
				FS_FSeek(fIn, 0L, SEEK_SET);// Reposition to begin of file 

            streamTotalLen = 0;
            streamSeekLen = 0;
            streamInFile = 0;

            /* Handle input buffer load */
            if(jpegIn.bufferSize)
            {
                if(len > jpegIn.bufferSize)
                {
                    streamTotalLen = len;
                    len = jpegIn.bufferSize;
                }
                else
                {
                    streamTotalLen = len;
                    len = streamTotalLen;
                    jpegIn.bufferSize = 0;
                }
            }
            else
            {
                jpegIn.bufferSize = 0;
                streamTotalLen = len;
            }

            streamInFile = streamTotalLen;

            /* NOTE: The DWL should not be used outside decoder SW
             * here we call it because it is the easiest way to get
             * dynamically allocated linear memory
             * */

            /* allocate memory for stream buffer. if unsuccessful -> exit */
            streamMem.virtualAddress = NULL;
            streamMem.busAddress = 0;

            /* allocate memory for stream buffer. if unsuccessful -> exit */
            if(DWLMallocLinear
               (((JpegDecContainer *) jpeg)->dwl, len, &streamMem) != DWL_OK)
            {
                printf( "UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n");
                goto end;
            }

            byteStrmStart = (u8 *) streamMem.virtualAddress;

            if(byteStrmStart == NULL)
            {
                printf( "UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n");
                goto end;
            }

            /* read input stream from file to buffer and close input file */
            FS_FRead(byteStrmStart, sizeof(u8), len, fIn);
            FS_FClose(fIn);

            /* initialize JpegDecDecode input structure */
            jpegIn.streamBuffer.pVirtualAddress = (u32 *) byteStrmStart;
            jpegIn.streamBuffer.busAddress = (u32) streamMem.busAddress;
            jpegIn.streamLength = streamTotalLen;

            if(writeOutput)
                printf( "\t-File IO: Write output: YES: %d\n",
                        writeOutput);
            else
                printf( "\t-File IO: Write output: NO: %d\n",
                        writeOutput);
            printf( "\t-File IO: MbRows/slice: %d\n",
                    jpegIn.sliceMbSet);

            printf( "PHASE 2: SET FILE IO successful\n");

            /******** PHASE 3 ********/
            printf( "\nPHASE 3: GET IMAGE INFO\n");

            /* Get image information of the JFIF and decode JFIF header */
            START_SW_PERFORMANCE;
            decsw_performance();
            jpegRet = JpegDecGetImageInfo(jpeg, &jpegIn, &imageInfo);/*获取图像信息 */
            END_SW_PERFORMANCE;
            decsw_performance();

            if(jpegRet != JPEGDEC_OK)
            {
                PrintJpegRet(&jpegRet);
                /* Handle here the error situation */
                goto end;
            }
            /* Decode Full Image */
            jpegIn.decImageType = JPEGDEC_IMAGE;

            /* no slice mode supported in progressive || non-interleaved ==> force to full mode */
            if(imageInfo.codingMode == JPEGDEC_PROGRESSIVE ||
               imageInfo.codingMode == JPEGDEC_NONINTERLEAVED)
                jpegIn.sliceMbSet = 0;

            printf( "PHASE 3: GET IMAGE INFO successful\n");
        }/*end if(frameCounter > 1 && loop == 0)*/
    }

error:

    /* calculate/write output of slice */
    if(slicedOutputUsed && jpegOut.outputPictureY.pVirtualAddress != NULL)
    {
        handleSlicedOutput(&imageInfo, &jpegIn, &jpegOut, cmdl.output);
        slicedOutputUsed = 0;
    }

    /* full resolution mode */
    mode = 0;

    if(jpegOut.outputPictureY.pVirtualAddress != NULL)
    {
        /* calculate size for output */
        calcSize(&imageInfo, mode);

        printf( "\n\t-JPEG: ++++++++++ FULL RESOLUTION ++++++++++\n");
        printf( "\t-JPEG: Instance %x\n", (JpegDecContainer *) jpeg);
        printf( "\t-JPEG: Luma output: 0x%08x size: %d\n",
                jpegOut.outputPictureY.pVirtualAddress, sizeLuma);
        printf( "\t-JPEG: Chroma output: 0x%08x size: %d\n",
                jpegOut.outputPictureCbCr.pVirtualAddress, sizeChroma);
        printf( "\t-JPEG: Luma output bus: 0x%08x\n",
                (u8 *) jpegOut.outputPictureY.busAddress);
        printf( "\t-JPEG: Chroma output bus: 0x%08x\n",
                (u8 *) jpegOut.outputPictureCbCr.busAddress);
    }

    printf( "PHASE 4: DECODE FRAME successful\n");/*编码成功*/

    /* if output write not disabled by TB */
    if(writeOutput)
    {
        /******** PHASE 5 ********/
        printf( "\nPHASE 5: WRITE OUTPUT\n");

#ifndef PP_PIPELINE_ENABLED
        if(imageInfo.outputFormat)
        {
            switch (imageInfo.outputFormat)
            {
            case JPEGDEC_YCbCr420_SEMIPLANAR:
                printf( "\t-JPEG: DECODER OUTPUT: JPEGDEC_YCbCr420_SEMIPLANAR\n");
                break;					
            case JPEGDEC_YCbCr400:
                printf( "\t-JPEG: DECODER OUTPUT: JPEGDEC_YCbCr400\n");
                break;
            case JPEGDEC_YCbCr422_SEMIPLANAR:
                printf( "\t-JPEG: DECODER OUTPUT: JPEGDEC_YCbCr422_SEMIPLANAR\n");
                break;
            case JPEGDEC_YCbCr440:
                printf( "\t-JPEG: DECODER OUTPUT: JPEGDEC_YCbCr440\n");
                break;
            case JPEGDEC_YCbCr411_SEMIPLANAR:
                printf( "\t-JPEG: DECODER OUTPUT: JPEGDEC_YCbCr411_SEMIPLANAR\n");
                break;
            case JPEGDEC_YCbCr444_SEMIPLANAR:
                printf( "\t-JPEG: DECODER OUTPUT: JPEGDEC_YCbCr444_SEMIPLANAR\n");
                break;
            }
        }

        if(imageInfo.codingMode == JPEGDEC_PROGRESSIVE)
            progressive = 1;

        /* write output */
        if(jpegIn.sliceMbSet)
        {
            if(imageInfo.outputFormat != JPEGDEC_YCbCr400)
                WriteFullOutput(mode);
        }
        else
        {
            if(imageInfo.codingMode != JPEGDEC_PROGRESSIVE)
            {
                WriteOutput(((u8 *) jpegOut.outputPictureY.pVirtualAddress),
                            sizeLuma,
                            ((u8 *) jpegOut.outputPictureCbCr.
                             pVirtualAddress), sizeChroma, mode,&imageInfo,
									 cmdl.output);
            }
            else
            {
                /* calculate size for output */
                calcSize(&imageInfo, mode);

                printf("sizeLuma %d and sizeChroma %d\n", sizeLuma, sizeChroma);

                WriteProgressiveOutput(sizeLuma, sizeChroma, mode,
                                       (u8*)jpegOut.outputPictureY.pVirtualAddress,
                                       (u8*)jpegOut.outputPictureCbCr.
                                       pVirtualAddress,
                                       (u8*)jpegOut.outputPictureCr.pVirtualAddress,
													cmdl.output);
            }

        }

        if(crop)
            WriteCroppedOutput(&imageInfo,
                               (u8*)jpegOut.outputPictureY.pVirtualAddress,
                               (u8*)jpegOut.outputPictureCbCr.pVirtualAddress,
                               (u8*)jpegOut.outputPictureCr.pVirtualAddress);

        progressive = 0;
#else
        /* PP test bench will do the operations only if enabled */
        /*pp_set_rotation(); */

        printf( "\t-JPEG: PP_OUTPUT_WRITE\n");
        pp_write_output(0, 0, 0);
        pp_check_combined_status();
#endif
        printf( "PHASE 5: WRITE OUTPUT successful\n");
    }
    else
    {
        printf( "\nPHASE 5: WRITE OUTPUT DISABLED\n");
    }

end:
    /******** PHASE 6 ********/
    printf( "\nPHASE 6: RELEASE JPEG DECODER\n");

    /* reset output write option */
    progressive = 0;

#ifdef PP_PIPELINE_ENABLED
    pp_close();
#endif

    if(streamMem.virtualAddress != NULL)
        DWLFreeLinear(((JpegDecContainer *) jpeg)->dwl, &streamMem);

    if(userAllocLuma.virtualAddress != NULL)
        DWLFreeRefFrm(((JpegDecContainer *) jpeg)->dwl, &userAllocLuma);

    if(userAllocChroma.virtualAddress != NULL)
        DWLFreeRefFrm(((JpegDecContainer *) jpeg)->dwl, &userAllocChroma);

    if(userAllocCr.virtualAddress != NULL)
        DWLFreeRefFrm(((JpegDecContainer *) jpeg)->dwl, &userAllocCr);

    /* release decoder instance */
    START_SW_PERFORMANCE;
    decsw_performance();
    JpegDecRelease(jpeg);
    END_SW_PERFORMANCE;
    decsw_performance();

    printf( "PHASE 6: RELEASE JPEG DECODER successful\n\n");

    if(inputReadType)
    {
        if(fIn)
        {
            FS_FClose(fIn);
        }
    }

    if(fout)
    {
        FS_FClose(fout);
        if(fStreamTrace)
            FS_FClose(fStreamTrace);
    }

#ifdef ASIC_TRACE_SUPPORT
    trace_SequenceCtrl(picNumber + 1, bFrames);
    trace_JpegDecodingTools();
    closeTraceFiles();
#endif

    /* Leave properly */
    JpegDecFree(pImage);

    FINALIZE_SW_PERFORMANCE;

    printf( "Jpeg: ...released\n");

    return 0;
}

/*------------------------------------------------------------------------------

    Function name:  WriteStrmDec

    Purpose:
        Write picture pointed by data to file. Size of the
        picture in pixels is indicated by picSize.

------------------------------------------------------------------------------*/
void WriteStrmDec(FS_FILE * fout, u32 * strmbuf, u32 size, u32 endian)
{
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

    }
    /* Write the stream to file */
    FS_FWrite(strmbuf, 1, size, fout);
}

/* 只写UV 分量 数据 */
static int savePictureIntoFileJpeg (FS_FILE *file, const u32 *picture, u32 picWidth, u32 picHeight, u32 picoutputFormat )
{
	unsigned char *uv = (unsigned char *)malloc(picWidth*picHeight*3/2);
	char *src_u, *src_v, *dst_u, *dst_v;
	u32 i, j;
	if( picoutputFormat == JPEGDEC_YCbCr420_SEMIPLANAR )
	{
		// Y_UV420数据
		// UV 只有UV分量的数据 
		//memcpy (uv, (unsigned char *)picture + picWidth * picHeight,  picWidth * picHeight / 2);
		dst_u = (char *)uv ;
		dst_v = (char *)uv + picWidth * picHeight /4;
		src_u = (char *)picture ;
		src_v = src_u + 1;
		for (j = 0; j < picHeight/2; j ++)
		{
			for (i = 0; i < picWidth/2; i ++)
			{
				*dst_u = *src_u;
				*dst_v = *src_v;
				src_u += 2;
				dst_u ++;
				src_v += 2;
				dst_v ++;
			}
		}
		
		if(file == NULL)
			return -1;
		
		if(FS_FWrite (uv, 1, picWidth * picHeight /2, file) != picWidth * picHeight /2)
			return -1;
	}
	else if( picoutputFormat == JPEGDEC_YCbCr422_SEMIPLANAR )
	{
		// Y_UV422数据
		// UV 只有UV分量的数据 
		// Y_UV422的 数据转化为 Y_UV420 并写入文件
		//memcpy (uv, (unsigned char *)picture + picWidth * picHeight,  picWidth * picHeight / 2);
		dst_u = (char *)uv ;
		dst_v = (char *)uv + picWidth * picHeight /4;
		src_u = (char *)picture ;
		src_v = src_u + 1;
		for (j = 0; j < picHeight/2; j ++)
		{
			for (i = 0; i < picWidth/2; i ++)
			{
				*dst_u = *src_u;
				*dst_v = *src_v;
				src_u += 2;//  UVUV UVUV 
				dst_u ++;
				src_v += 2;
				dst_v ++;
			}
			for (i = 0; i < picWidth/2; i ++)//第二行数据重复
			{
				src_u += 2;
				src_v += 2;				
			}			
		}
		
		if(file == NULL)
			return -1;
		
		if(FS_FWrite (uv, 1, picWidth * picHeight/2 , file) != picWidth * picHeight/2 )
			return -1;
	}
	else
	{
		printf("Unknow picoutputFormat= 0x%08x\r\n", picoutputFormat );
		return -1;
	}
	if(uv)
		free(uv);
	return 0;
}

/*------------------------------------------------------------------------------

    Function name:  WriteOutput

    Purpose:
        Write picture pointed by data to file. Size of the
        picture in pixels is indicated by picSize.

------------------------------------------------------------------------------*/

void
WriteOutput(u8 * dataLuma, u32 picSizeLuma, u8 * dataChroma,
            u32 picSizeChroma, u32 picMode, JpegDecImageInfo *imageInfo,
				char *output_filename)
{
    u32 i;
    FS_FILE *foutput = NULL;
    u8 *pYuvOut = NULL;
    u8 file[MAX_PATH];

    if(!sliceToUser)
    {
        /* foutput is global file pointer */
        if(foutput == NULL)
        {
            if(picMode == 0)
                foutput = FS_FOpen( output_filename , "wb");
            else
                foutput = FS_FOpen( output_filename , "wb");
            if(foutput == NULL)
            {
                printf( "UNABLE TO OPEN OUTPUT FILE (%s)\n", outputfilename);
                return;
            }
        }
    }
    else
    {
        /* foutput is global file pointer */
        if(foutput == NULL)
        {
            if(picMode == 0)
            {
					 memset( file , 0 , MAX_PATH ) ;
					 strncpy(file , outputfilename_slice, strlen(outputfilename_slice ) -4 );
                sprintf(file+strlen(outputfilename_slice ) -4, "%03d.yuv", fullSliceCounter);
					// 	 strncpy(outputfilename,argv[argc - 1],strlen(argv[argc - 1])-4);
                foutput = FS_FOpen(file, "wb");
            }
            else
            {
                //sprintf(file, "\\MINITEST\\JPG2YUV\\19201080\\tn_%d.yuv", fullSliceCounter);
                foutput = FS_FOpen(output_filename, "wb");
            }

            if(foutput == NULL)
            {
                printf( "UNABLE TO OPEN OUTPUT FILE\n");
                return;
            }
        }
    }

    if(foutput && dataLuma)
    {
        if(1)
        {
            printf( "\t-JPEG: Luminance\n");
            /* write decoder output to file */
            pYuvOut = dataLuma;
				// y
				WriteStrmDec(foutput, (u32*)pYuvOut, picSizeLuma, DEC_X170_BIG_ENDIAN );
        }
    }

    if(!nonInterleaved) /* 交错数据 */
    {
        /* progressive ==> planar */
        if(!progressive)
        {
            if(foutput && dataChroma)
            {
                printf( "\t-JPEG: Chrominance\n");
                /* write decoder output to file */
                pYuvOut = dataChroma;
                if(!planarOutput)
                {
					//	WriteStrmDec(foutput, (u32*)pYuvOut, picSizeChroma, DEC_X170_BIG_ENDIAN );
						 if(sliceSize)
						 	savePictureIntoFileJpeg(foutput, (u32*)pYuvOut, imageInfo->outputWidth ,           sliceSize   , imageInfo->outputFormat );// 1920,1088 );
						 else
							savePictureIntoFileJpeg(foutput, (u32*)pYuvOut, imageInfo->outputWidth ,  imageInfo->outputHeight, imageInfo->outputFormat );// 1920,1088 );
 
                }
                else
                {
                    printf("BASELINE PLANAR\n");
                    for(i = 0; i < picSizeChroma / 2; i++)
                        FS_FWrite(pYuvOut + 2 * i, sizeof(u8), 1, foutput);
                    for(i = 0; i < picSizeChroma / 2; i++)
                        FS_FWrite(pYuvOut + 2 * i + 1, sizeof(u8), 1, foutput);
                }
            }
        }
        else
        {
            if(foutput && dataChroma)
            {
                printf( "\t-JPEG: Chrominance\n");
                /* write decoder output to file */
                pYuvOut = dataChroma;
                if(!planarOutput)
                {// Y_UV420
						 WriteStrmDec(foutput, (u32*)pYuvOut, picSizeChroma, DEC_X170_BIG_ENDIAN );
                }
                else
                {
                    printf("PROGRESSIVE PLANAR OUTPUT\n");
                    for(i = 0; i < picSizeChroma; i++)
                        FS_FWrite(pYuvOut + (1 * i), sizeof(u8), 1, foutput);
                }
            }
        }
    }
    else
    {
        if(foutput && dataChroma)
        {
            printf( "\t-JPEG: Chrominance\n");
            /* write decoder output to file */
            pYuvOut = dataChroma;

            printf("NONINTERLEAVED: PLANAR OUTPUT\n");
            for(i = 0; i < picSizeChroma; i++)
                FS_FWrite(pYuvOut + (1 * i), sizeof(u8), 1, foutput);
        }
    }
    FS_FClose(foutput);
}

/*------------------------------------------------------------------------------

    Function name:  WriteOutputLuma

    Purpose:
        Write picture pointed by data to file. Size of the
        picture in pixels is indicated by picSize.

------------------------------------------------------------------------------*/
void WriteOutputLuma(u8 * dataLuma, u32 picSizeLuma, u32 picMode)
{
    u32 i;
    FS_FILE *foutput = NULL;
    u8 *pYuvOut = NULL;

    /* foutput is global file pointer */
    if(foutput == NULL)
    {
        if(picMode == 0)
        {
            foutput = FS_FOpen("out.yuv", "ab");
        }
        else
        {
            foutput = FS_FOpen("out_tn.yuv", "ab");
        }

        if(foutput == NULL)
        {
            printf( "UNABLE TO OPEN OUTPUT FILE\n");
            return;
        }
    }

    if(foutput && dataLuma)
    {
        if(1)
        {
            printf( "\t-JPEG: Luminance\n");
            /* write decoder output to file */
            pYuvOut = dataLuma;
            for(i = 0; i < (picSizeLuma >> 2); i++)
            {
#ifndef ASIC_TRACE_SUPPORT
                if(DEC_X170_BIG_ENDIAN == output_picture_endian)
                {
                    FS_FWrite(pYuvOut + (4 * i) + 3, sizeof(u8), 1, foutput);
                    FS_FWrite(pYuvOut + (4 * i) + 2, sizeof(u8), 1, foutput);
                    FS_FWrite(pYuvOut + (4 * i) + 1, sizeof(u8), 1, foutput);
                    FS_FWrite(pYuvOut + (4 * i) + 0, sizeof(u8), 1, foutput);
                }
                else
                {
#endif
                    FS_FWrite(pYuvOut + (4 * i) + 0, sizeof(u8), 1, foutput);
                    FS_FWrite(pYuvOut + (4 * i) + 1, sizeof(u8), 1, foutput);
                    FS_FWrite(pYuvOut + (4 * i) + 2, sizeof(u8), 1, foutput);
                    FS_FWrite(pYuvOut + (4 * i) + 3, sizeof(u8), 1, foutput);
#ifndef ASIC_TRACE_SUPPORT
                }
#endif
            }
        }
    }

    FS_FClose(foutput);
}

/*------------------------------------------------------------------------------

    Function name:  WriteOutput

    Purpose:
        Write picture pointed by data to file. Size of the
        picture in pixels is indicated by picSize.

------------------------------------------------------------------------------*/
void WriteOutputChroma(u8 * dataChroma, u32 picSizeChroma, u32 picMode)
{
    u32 i;
    FS_FILE *foutputChroma = NULL;
    u8 *pYuvOut = NULL;

    /* file pointer */
    if(foutputChroma == NULL)
    {
        if(picMode == 0)
        {
            if(!progressive)
            {
                if(fullSliceCounter == 0)
                    foutputChroma = FS_FOpen("outChroma.yuv", "wb");
                else
                    foutputChroma = FS_FOpen("outChroma.yuv", "ab");
            }
            else
            {
                if(!slicedOutputUsed)
                {
                    foutputChroma = FS_FOpen("outChroma.yuv", "wb");
                }
                else
                {
                    if(scanCounter == 0 || fullSliceCounter == 0)
                        foutputChroma = FS_FOpen("outChroma.yuv", "wb");
                    else
                        foutputChroma = FS_FOpen("outChroma.yuv", "ab");
                }
            }
        }
        else
        {
            if(!progressive)
            {
                if(fullSliceCounter == 0)
                    foutputChroma = FS_FOpen("outChroma_tn.yuv", "wb");
                else
                    foutputChroma = FS_FOpen("outChroma_tn.yuv", "ab");
            }
            else
            {
                if(!slicedOutputUsed)
                {
                    foutputChroma = FS_FOpen("outChroma_tn.yuv", "wb");
                }
                else
                {
                    if(scanCounter == 0 || fullSliceCounter == 0)
                        foutputChroma = FS_FOpen("outChroma_tn.yuv", "wb");
                    else
                        foutputChroma = FS_FOpen("outChroma_tn.yuv", "ab");
                }
            }
        }

        if(foutputChroma == NULL)
        {
            printf( "UNABLE TO OPEN OUTPUT FILE\n");
            return;
        }
    }

    if(foutputChroma && dataChroma)
    {
        printf( "\t-JPEG: Chrominance\n");
        /* write decoder output to file */
        pYuvOut = dataChroma;

        if(!progressive)
        {
            for(i = 0; i < (picSizeChroma >> 2); i++)
            {
#ifndef ASIC_TRACE_SUPPORT
                if(DEC_X170_BIG_ENDIAN == output_picture_endian)
                {
                    FS_FWrite(pYuvOut + (4 * i) + 3, sizeof(u8), 1, foutputChroma);
                    FS_FWrite(pYuvOut + (4 * i) + 2, sizeof(u8), 1, foutputChroma);
                    FS_FWrite(pYuvOut + (4 * i) + 1, sizeof(u8), 1, foutputChroma);
                    FS_FWrite(pYuvOut + (4 * i) + 0, sizeof(u8), 1, foutputChroma);
                }
                else
                {
#endif
                    FS_FWrite(pYuvOut + (4 * i) + 0, sizeof(u8), 1, foutputChroma);
                    FS_FWrite(pYuvOut + (4 * i) + 1, sizeof(u8), 1, foutputChroma);
                    FS_FWrite(pYuvOut + (4 * i) + 2, sizeof(u8), 1, foutputChroma);
                    FS_FWrite(pYuvOut + (4 * i) + 3, sizeof(u8), 1, foutputChroma);
#ifndef ASIC_TRACE_SUPPORT
                }
#endif
            }
        }
        else
        {
            printf("PROGRESSIVE PLANAR OUTPUT CHROMA\n");
            for(i = 0; i < picSizeChroma; i++)
                FS_FWrite(pYuvOut + (1 * i), sizeof(u8), 1, foutputChroma);
        }
    }
    FS_FClose(foutputChroma);
}

/*------------------------------------------------------------------------------

    Function name:  WriteFullOutput

    Purpose:
        Write picture pointed by data to file. 

------------------------------------------------------------------------------*/
void WriteFullOutput(u32 picMode)
{
    u32 i;
    FS_FILE *foutput = NULL;
    u8 *pYuvOutChroma = NULL;
    FS_FILE *fInputChroma = NULL;
    u32 length = 0;
    u32 chromaLen = 0;

	printf( "\t-JPEG: WriteFullOutput\n");

	/* if semi-planar output */
	if(!planarOutput)
	{
		/* Reading chroma file */
		if(picMode == 0)
			system("cat outChroma.yuv >> out.yuv");
		else
			system("cat outChroma_tn.yuv >> out_tn.yuv");
	}
	else
	{
		/* Reading chroma file */
		if(picMode == 0)
			fInputChroma = FS_FOpen("outChroma.yuv", "rb");
		else
			fInputChroma = FS_FOpen("outChroma_tn.yuv", "rb");

		if(fInputChroma == NULL)
		{
			printf( "Unable to open chroma output tmp file\n");
			exit(-1);
		}

		/* file i/o pointer to full */
		FS_FSeek(fInputChroma, 0L, SEEK_END);// SEEK_SET
		length = FS_FTell(fInputChroma);
		//rewind(fInputChroma);
		FS_FSeek(fInputChroma, 0L, SEEK_SET);// Reposition to begin of file 

		/* check length */
		chromaLen = length;

		pYuvOutChroma = JpegDecMalloc(sizeof(u8) * (chromaLen));

		/* read output stream from file to buffer and close input file */
		FS_FRead(pYuvOutChroma, sizeof(u8), chromaLen, fInputChroma);

		FS_FClose(fInputChroma);

		/* foutput is global file pointer */
		if(picMode == 0)
			foutput = FS_FOpen("out.yuv", "ab");
		else
			foutput = FS_FOpen("out_tn.yuv", "ab");

		if(foutput == NULL)
		{
			printf( "UNABLE TO OPEN OUTPUT FILE\n");
			return;
		}

		if(foutput && pYuvOutChroma)
		{
			printf( "\t-JPEG: Chrominance\n");
			if(!progressive)
			{
				if(!planarOutput)
				{
					/* write decoder output to file */
					for(i = 0; i < (chromaLen >> 2); i++)
					{
						FS_FWrite(pYuvOutChroma + (4 * i) + 0, sizeof(u8), 1, foutput);
						FS_FWrite(pYuvOutChroma + (4 * i) + 1, sizeof(u8), 1, foutput);
						FS_FWrite(pYuvOutChroma + (4 * i) + 2, sizeof(u8), 1, foutput);
						FS_FWrite(pYuvOutChroma + (4 * i) + 3, sizeof(u8), 1, foutput);
					}
				}
				else
				{
					for(i = 0; i < chromaLen / 2; i++)
						FS_FWrite(pYuvOutChroma + 2 * i, sizeof(u8), 1, foutput);
					for(i = 0; i < chromaLen / 2; i++)
						FS_FWrite(pYuvOutChroma + 2 * i + 1, sizeof(u8), 1, foutput);
				}
			}
			else
			{
				if(!planarOutput)
				{
					/* write decoder output to file */
					for(i = 0; i < (chromaLen >> 2); i++)
					{
						FS_FWrite(pYuvOutChroma + (4 * i) + 0, sizeof(u8), 1, foutput);
						FS_FWrite(pYuvOutChroma + (4 * i) + 1, sizeof(u8), 1, foutput);
						FS_FWrite(pYuvOutChroma + (4 * i) + 2, sizeof(u8), 1, foutput);
						FS_FWrite(pYuvOutChroma + (4 * i) + 3, sizeof(u8), 1, foutput);
					}
				}
				else
				{
					printf("PROGRESSIVE FULL CHROMA %d\n", chromaLen);
					for(i = 0; i < chromaLen; i++)
						FS_FWrite(pYuvOutChroma + i, sizeof(u8), 1, foutput);
				}
			}
		}
		FS_FClose(foutput);

		/* Leave properly */
		JpegDecFree(pYuvOutChroma);
	}
}

/*------------------------------------------------------------------------------

    Function name:  handleSlicedOutput

    Purpose:
        Calculates size for slice and writes sliced output

------------------------------------------------------------------------------*/
void
handleSlicedOutput(JpegDecImageInfo * imageInfo,
                   JpegDecInput * jpegIn, JpegDecOutput * jpegOut,
						 char *output_filename
						 )
{
    /* for output name */
    fullSliceCounter++;

    /******** PHASE X ********/
    if(jpegIn->sliceMbSet)
    {
		 printf( "\nPHASE SLICE: HANDLE SLICE %d\n", fullSliceCounter);
	 }

    /* save start pointers for whole output */
    if(fullSliceCounter == 0)
    {
        /* virtual address */
        outputAddressY.pVirtualAddress =
            jpegOut->outputPictureY.pVirtualAddress;
        outputAddressCbCr.pVirtualAddress =
            jpegOut->outputPictureCbCr.pVirtualAddress;

        /* bus address */
        outputAddressY.busAddress = jpegOut->outputPictureY.busAddress;
        outputAddressCbCr.busAddress = jpegOut->outputPictureCbCr.busAddress;
    }

    /* if output write not disabled by TB */
    if(writeOutput)
    {
        /******** PHASE 5 ********/
        printf( "\nPHASE 5: WRITE OUTPUT\n");

        if(imageInfo->outputFormat)
        {
            if(!frameReady)
            {
                sliceSize = jpegIn->sliceMbSet * 16;
            }
            else
            {
                if(mode == 0)
                    sliceSize =
                        (imageInfo->outputHeight -
                         ((fullSliceCounter) * (sliceSize)));
                else
                    sliceSize =
                        (imageInfo->outputHeightThumb -
                         ((fullSliceCounter) * (sliceSize)));
            }
        }

        /* slice interrupt from decoder */
        sliceToUser = 1;

        /* calculate size for output */
        calcSize(imageInfo, mode);

        /* test printf */
        printf( "\t-JPEG: ++++++++++ SLICE INFORMATION ++++++++++\n");
        printf( "\t-JPEG: Luma output: 0x%08x size: %d\n",
                jpegOut->outputPictureY.pVirtualAddress, sizeLuma);
        printf( "\t-JPEG: Chroma output: 0x%08x size: %d\n",
                jpegOut->outputPictureCbCr.pVirtualAddress, sizeChroma);
        printf( "\t-JPEG: Luma output bus: 0x%08x\n",
                (u8 *) jpegOut->outputPictureY.busAddress);
        printf( "\t-JPEG: Chroma output bus: 0x%08x\n",
                (u8 *) jpegOut->outputPictureCbCr.busAddress);

        /* write slice output */
        WriteOutput(((u8 *) jpegOut->outputPictureY.pVirtualAddress),
                    sizeLuma,
                    ((u8 *) jpegOut->outputPictureCbCr.pVirtualAddress),
                    sizeChroma, mode, imageInfo,
						  output_filename);

        /* write luma to final output file */
  /*      WriteOutputLuma(((u8 *) jpegOut->outputPictureY.pVirtualAddress),
                        sizeLuma, mode);*/

        if(imageInfo->outputFormat != JPEGDEC_YCbCr400)
        {
            /* write chroam to tmp file */
            WriteOutputChroma(((u8 *) jpegOut->outputPictureCbCr.
                               pVirtualAddress), sizeChroma, mode);
        }

        printf( "PHASE 5: WRITE OUTPUT successful\n");
    }
    else
    {
        printf( "\nPHASE 5: WRITE OUTPUT DISABLED\n");
    }

    if(frameReady)
    {
        /* give start pointers for whole output write */

        /* virtual address */
        jpegOut->outputPictureY.pVirtualAddress =
            outputAddressY.pVirtualAddress;
        jpegOut->outputPictureCbCr.pVirtualAddress =
            outputAddressCbCr.pVirtualAddress;

        /* bus address */
        jpegOut->outputPictureY.busAddress = outputAddressY.busAddress;
        jpegOut->outputPictureCbCr.busAddress = outputAddressCbCr.busAddress;
    }

    if(frameReady)
    {
        frameReady = 0;
        sliceToUser = 0;

        /******** PHASE X ********/
        if(jpegIn->sliceMbSet)
            printf( "\nPHASE SLICE: HANDLE SLICE %d successful\n",
                    fullSliceCounter);

        fullSliceCounter = -1;
    }
    else
    {
        /******** PHASE X ********/
        if(jpegIn->sliceMbSet)
            printf( "\nPHASE SLICE: HANDLE SLICE %d successful\n",
                    fullSliceCounter);
    }

}

/*------------------------------------------------------------------------------

    Function name:  calcSize

    Purpose:
        Calculate size

------------------------------------------------------------------------------*/
void calcSize(JpegDecImageInfo * imageInfo, u32 picMode)
{

    u32 format;

    sizeLuma = 0;
    sizeChroma = 0;

    format = picMode == 0 ?
        imageInfo->outputFormat : imageInfo->outputFormatThumb;

    /* if slice interrupt not given to user */
    if(!sliceToUser || scanReady)
    {
        if(picMode == 0)    /* full */
        {
            sizeLuma = (imageInfo->outputWidth * imageInfo->outputHeight);
        }
        else    /* thumbnail */
        {
            sizeLuma =
                (imageInfo->outputWidthThumb * imageInfo->outputHeightThumb);
        }
    }
    else
    {
        if(picMode == 0)    /* full */
        {
            sizeLuma = (imageInfo->outputWidth * sliceSize);
        }
        else    /* thumbnail */
        {
            sizeLuma = (imageInfo->outputWidthThumb * sliceSize);
        }
    }

    if(format != JPEGDEC_YCbCr400)
    {
        if(format == JPEGDEC_YCbCr420_SEMIPLANAR ||
           format == JPEGDEC_YCbCr411_SEMIPLANAR)
        {
            sizeChroma = (sizeLuma / 2);
        }
        else if(format == JPEGDEC_YCbCr444_SEMIPLANAR)
        {
            sizeChroma = sizeLuma * 2;
        }
        else
        {
            sizeChroma = sizeLuma;
        }
    }
}

/*------------------------------------------------------------------------------

    Function name:  allocMemory

    Purpose:
        Allocates user specific memory for output.

------------------------------------------------------------------------------*/
u32
allocMemory(JpegDecInst decInst, JpegDecImageInfo * imageInfo,
            JpegDecInput * jpegIn)
{
    u32 separateChroma = 0;
    u32 rotation = 0;

    out_pic_size_luma = 0;
    out_pic_size_chroma = 0;
    jpegIn->pictureBufferY.pVirtualAddress = NULL;
    jpegIn->pictureBufferY.busAddress = 0;
    jpegIn->pictureBufferCbCr.pVirtualAddress = NULL;
    jpegIn->pictureBufferCbCr.busAddress = 0;
    jpegIn->pictureBufferCr.pVirtualAddress = NULL;
    jpegIn->pictureBufferCr.busAddress = 0;

#ifdef PP_PIPELINE_ENABLED
    /* check if rotation used */
    rotation = pp_rotation_used();

    if(rotation)
        printf(
                "\t-JPEG: IN CASE ROTATION ==> USER NEEDS TO ALLOCATE FULL OUTPUT MEMORY\n");
#endif

    /* calculate sizes */
    if(jpegIn->decImageType == 0)
    {
        /* luma size */
        if(jpegIn->sliceMbSet && !rotation)
            out_pic_size_luma =
                (imageInfo->outputWidth * (jpegIn->sliceMbSet * 16));
        else
            out_pic_size_luma =
                (imageInfo->outputWidth * imageInfo->outputHeight);

        /* chroma size ==> semiplanar output */
        if(imageInfo->outputFormat == JPEGDEC_YCbCr420_SEMIPLANAR ||
           imageInfo->outputFormat == JPEGDEC_YCbCr411_SEMIPLANAR)
            out_pic_size_chroma = out_pic_size_luma / 2;
        else if(imageInfo->outputFormat == JPEGDEC_YCbCr422_SEMIPLANAR ||
                imageInfo->outputFormat == JPEGDEC_YCbCr440)
            out_pic_size_chroma = out_pic_size_luma;
        else if(imageInfo->outputFormat == JPEGDEC_YCbCr444_SEMIPLANAR)
            out_pic_size_chroma = out_pic_size_luma * 2;

        if(imageInfo->codingMode != JPEGDEC_BASELINE)
            separateChroma = 1;
    }
    else
    {
        /* luma size */
        if(jpegIn->sliceMbSet && !rotation)
            out_pic_size_luma =
                (imageInfo->outputWidthThumb * (jpegIn->sliceMbSet * 16));
        else
            out_pic_size_luma =
                (imageInfo->outputWidthThumb * imageInfo->outputHeightThumb);

        /* chroma size ==> semiplanar output */
        if(imageInfo->outputFormatThumb == JPEGDEC_YCbCr420_SEMIPLANAR ||
           imageInfo->outputFormatThumb == JPEGDEC_YCbCr411_SEMIPLANAR)
            out_pic_size_chroma = out_pic_size_luma / 2;
        else if(imageInfo->outputFormatThumb == JPEGDEC_YCbCr422_SEMIPLANAR ||
                imageInfo->outputFormatThumb == JPEGDEC_YCbCr440)
            out_pic_size_chroma = out_pic_size_luma;
        else if(imageInfo->outputFormatThumb == JPEGDEC_YCbCr444_SEMIPLANAR)
            out_pic_size_chroma = out_pic_size_luma * 2;

        if(imageInfo->codingModeThumb != JPEGDEC_BASELINE)
            separateChroma = 1;
    }

#ifdef LINUX
    {
        printf( "\t\t-JPEG: USER OUTPUT MEMORY ALLOCATION\n");

        jpegIn->pictureBufferY.pVirtualAddress = NULL;
        jpegIn->pictureBufferCbCr.pVirtualAddress = NULL;
        jpegIn->pictureBufferCr.pVirtualAddress = NULL;

        /**** memory area ****/

        /* allocate memory for stream buffer. if unsuccessful -> exit */
        userAllocLuma.virtualAddress = NULL;
        userAllocLuma.busAddress = 0;

        /* allocate memory for stream buffer. if unsuccessful -> exit */
        if(DWLMallocRefFrm
           (((JpegDecContainer *) decInst)->dwl, out_pic_size_luma,
            &userAllocLuma) != DWL_OK)
        {
            printf( "UNABLE TO ALLOCATE USER LUMA OUTPUT MEMORY\n");
            return JPEGDEC_MEMFAIL;
        }

        /* Luma Bus */
        jpegIn->pictureBufferY.pVirtualAddress = userAllocLuma.virtualAddress;
        jpegIn->pictureBufferY.busAddress = userAllocLuma.busAddress;

        /* memset output to gray */
        (void) DWLmemset(jpegIn->pictureBufferY.pVirtualAddress, 128,
                         out_pic_size_luma);

        /* allocate chroma */
        if(out_pic_size_chroma)
        {
            /* Baseline ==> semiplanar */
            if(separateChroma == 0)
            {
                /* allocate memory for stream buffer. if unsuccessful -> exit */
                if(DWLMallocRefFrm
                   (((JpegDecContainer *) decInst)->dwl, out_pic_size_chroma,
                    &userAllocChroma) != DWL_OK)
                {
                    printf(
                            "UNABLE TO ALLOCATE USER CHROMA OUTPUT MEMORY\n");
                    return JPEGDEC_MEMFAIL;
                }

                /* Chroma Bus */
                jpegIn->pictureBufferCbCr.pVirtualAddress =
                    userAllocChroma.virtualAddress;
                jpegIn->pictureBufferCbCr.busAddress =
                    userAllocChroma.busAddress;

                /* memset output to gray */
                (void) DWLmemset(jpegIn->pictureBufferCbCr.pVirtualAddress, 128,
                                 out_pic_size_chroma);
            }
            else    /* Progressive or non-interleaved ==> planar */
            {
                /* allocate memory for stream buffer. if unsuccessful -> exit */
                /* Cb */
                if(DWLMallocRefFrm
                   (((JpegDecContainer *) decInst)->dwl,
                    (out_pic_size_chroma / 2), &userAllocChroma) != DWL_OK)
                {
                    printf(
                            "UNABLE TO ALLOCATE USER CHROMA OUTPUT MEMORY\n");
                    return JPEGDEC_MEMFAIL;
                }

                /* Chroma Bus */
                jpegIn->pictureBufferCbCr.pVirtualAddress =
                    userAllocChroma.virtualAddress;
                jpegIn->pictureBufferCbCr.busAddress =
                    userAllocChroma.busAddress;

                /* Cr */
                if(DWLMallocRefFrm
                   (((JpegDecContainer *) decInst)->dwl,
                    (out_pic_size_chroma / 2), &userAllocCr) != DWL_OK)
                {
                    printf(
                            "UNABLE TO ALLOCATE USER CHROMA OUTPUT MEMORY\n");
                    return JPEGDEC_MEMFAIL;
                }

                /* Chroma Bus */
                jpegIn->pictureBufferCr.pVirtualAddress =
                    userAllocCr.virtualAddress;
                jpegIn->pictureBufferCr.busAddress = userAllocCr.busAddress;

                /* memset output to gray */
                /* Cb */
                (void) DWLmemset(jpegIn->pictureBufferCbCr.pVirtualAddress, 128,
                                 (out_pic_size_chroma / 2));

                /* Cr */
                (void) DWLmemset(jpegIn->pictureBufferCr.pVirtualAddress, 128,
                                 (out_pic_size_chroma / 2));
            }
        }
    }
#endif /* #ifdef LINUX */

#ifndef LINUX
    {
        printf( "\t\t-JPEG: MALLOC\n");

        /* allocate luma */
        jpegIn->pictureBufferY.pVirtualAddress =
            (u32 *) JpegDecMalloc(sizeof(u8) * out_pic_size_luma);

        JpegDecMemset(jpegIn->pictureBufferY.pVirtualAddress, 128,
                      out_pic_size_luma);

        /* allocate chroma */
        if(out_pic_size_chroma)
        {
            jpegIn->pictureBufferCbCr.pVirtualAddress =
                (u32 *) JpegDecMalloc(sizeof(u8) * out_pic_size_chroma);

            JpegDecMemset(jpegIn->pictureBufferCbCr.pVirtualAddress, 128,
                          out_pic_size_chroma);
        }
    }
#endif /* #ifndef LINUX */

    printf( "\t\t-JPEG: Allocate: Luma virtual %x bus %x size %d\n",
            (u32) jpegIn->pictureBufferY.pVirtualAddress,
            jpegIn->pictureBufferY.busAddress, out_pic_size_luma);

    if(separateChroma == 0)
    {
        printf(
                "\t\t-JPEG: Allocate: Chroma virtual %x bus %x size %d\n",
                (u32) jpegIn->pictureBufferCbCr.pVirtualAddress,
                jpegIn->pictureBufferCbCr.busAddress, out_pic_size_chroma);
    }
    else
    {
        printf(
                "\t\t-JPEG: Allocate: Cb virtual %x bus %x size %d\n",
                (u32) jpegIn->pictureBufferCbCr.pVirtualAddress,
                jpegIn->pictureBufferCbCr.busAddress,
                (out_pic_size_chroma / 2));

        printf(
                "\t\t-JPEG: Allocate: Cr virtual %x bus %x size %d\n",
                (u32) jpegIn->pictureBufferCr.pVirtualAddress,
                jpegIn->pictureBufferCr.busAddress, (out_pic_size_chroma / 2));
    }

    return JPEGDEC_OK;
}

/*-----------------------------------------------------------------------------

Print JPEG api return value

-----------------------------------------------------------------------------*/
void PrintJpegRet(JpegDecRet * pJpegRet)
{

    assert(pJpegRet);

    switch (*pJpegRet)
    {
    case JPEGDEC_FRAME_READY:
        printf( "TB: jpeg API returned : JPEGDEC_FRAME_READY\n");
        break;
    case JPEGDEC_OK:
        printf( "TB: jpeg API returned : JPEGDEC_OK\n");
        break;
    case JPEGDEC_ERROR:
        printf( "TB: jpeg API returned : JPEGDEC_ERROR\n");
        break;
    case JPEGDEC_DWL_HW_TIMEOUT:
        printf( "TB: jpeg API returned : JPEGDEC_HW_TIMEOUT\n");
        break;
    case JPEGDEC_UNSUPPORTED:
        printf( "TB: jpeg API returned : JPEGDEC_UNSUPPORTED\n");
        break;
    case JPEGDEC_PARAM_ERROR:
        printf( "TB: jpeg API returned : JPEGDEC_PARAM_ERROR\n");
        break;
    case JPEGDEC_MEMFAIL:
        printf( "TB: jpeg API returned : JPEGDEC_MEMFAIL\n");
        break;
    case JPEGDEC_INITFAIL:
        printf( "TB: jpeg API returned : JPEGDEC_INITFAIL\n");
        break;
    case JPEGDEC_HW_BUS_ERROR:
        printf( "TB: jpeg API returned : JPEGDEC_HW_BUS_ERROR\n");
        break;
    case JPEGDEC_SYSTEM_ERROR:
        printf( "TB: jpeg API returned : JPEGDEC_SYSTEM_ERROR\n");
        break;
    case JPEGDEC_DWL_ERROR:
        printf( "TB: jpeg API returned : JPEGDEC_DWL_ERROR\n");
        break;
    case JPEGDEC_INVALID_STREAM_LENGTH:
        printf(
                "TB: jpeg API returned : JPEGDEC_INVALID_STREAM_LENGTH\n");
        break;
    case JPEGDEC_STRM_ERROR:
        printf( "TB: jpeg API returned : JPEGDEC_STRM_ERROR\n");
        break;
    case JPEGDEC_INVALID_INPUT_BUFFER_SIZE:
        printf(
                "TB: jpeg API returned : JPEGDEC_INVALID_INPUT_BUFFER_SIZE\n");
        break;
    case JPEGDEC_INCREASE_INPUT_BUFFER:
        printf(
                "TB: jpeg API returned : JPEGDEC_INCREASE_INPUT_BUFFER\n");
        break;
    case JPEGDEC_SLICE_MODE_UNSUPPORTED:
        printf(
                "TB: jpeg API returned : JPEGDEC_SLICE_MODE_UNSUPPORTED\n");
        break;
    default:
        printf( "TB: jpeg API returned unknown status\n");
        break;
    }
}

/*------------------------------------------------------------------------------

    Function name:  JpegDecMalloc

------------------------------------------------------------------------------*/
void *JpegDecMalloc(unsigned int size)
{
    void *memPtr = (char *) malloc(size);

    return memPtr;
}

/*------------------------------------------------------------------------------

    Function name:  JpegDecMemset

------------------------------------------------------------------------------*/
void *JpegDecMemset(void *ptr, int c, unsigned int size)
{
    void *rv = NULL;

    if(ptr != NULL)
    {
        rv = memset(ptr, c, size);
    }
    return rv;
}

/*------------------------------------------------------------------------------

    Function name:  JpegDecFree

------------------------------------------------------------------------------*/
void JpegDecFree(void *ptr)
{
    if(ptr != NULL)
    {
        free(ptr);
    }
}

/*------------------------------------------------------------------------------

    Function name:  JpegDecTrace

    Purpose:
        Example implementation of JpegDecTrace function. Prototype of this
        function is given in jpegdecapi.h. This implementation appends
        trace messages to file named 'dec_api.trc'.

------------------------------------------------------------------------------*/
void JpegDecTrace(const char *string)
{
    FS_FILE *fp;

    fp = FS_FOpen("dec_api.trc", "at");

    if(!fp)
        return;

    FS_FWrite(string, 1, strlen(string), fp);
    FS_FWrite("\n", 1, 1, fp);

    FS_FClose(fp);
}

/*-----------------------------------------------------------------------------

    Function name:  FindImageInfoEnd

    Purpose:
        Finds 0xFFC4 from the stream and pOffset includes number of bytes to
        this marker. In case of an error returns != 0
        (i.e., the marker not found).

-----------------------------------------------------------------------------*/
u32 FindImageInfoEnd(u8 * pStream, u32 streamLength, u32 * pOffset)
{
    u32 i;

    for(i = 0; i < streamLength; ++i)
    {
        if(0xFF == pStream[i])
        {
            if(((i + 1) < streamLength) && 0xC4 == pStream[i + 1])
            {
                *pOffset = i;
                return 0;
            }
        }
    }
    return -1;
}

void WriteCroppedOutput(JpegDecImageInfo * info, u8 * dataLuma, u8 * dataCb,
                        u8 * dataCr)
{
    u32 i, j;
    FS_FILE *foutput = NULL;
    u8 *pYuvOut = NULL;
    u32 lumaW, lumaH, chromaW, chromaH, chromaOutputWidth, chromaOutputHeight;

    printf( "TB: WriteCroppedOut, displayW %d, displayH %d\n",
            info->displayWidth, info->displayHeight);

    foutput = FS_FOpen("cropped.yuv", "wb");
    if(foutput == NULL)
    {
        printf( "UNABLE TO OPEN OUTPUT FILE\n");
        return;
    }

    if(info->outputFormat == JPEGDEC_YCbCr420_SEMIPLANAR)
    {
        lumaW = (info->displayWidth + 1) & ~0x1;
        lumaH = (info->displayHeight + 1) & ~0x1;
        chromaW = lumaW / 2;
        chromaH = lumaH / 2;
        chromaOutputWidth = info->outputWidth / 2;
        chromaOutputHeight = info->outputHeight / 2;
    }
    else if(info->outputFormat == JPEGDEC_YCbCr422_SEMIPLANAR)
    {
        lumaW = (info->displayWidth + 1) & ~0x1;
        lumaH = info->displayHeight;
        chromaW = lumaW / 2;
        chromaH = lumaH;
        chromaOutputWidth = info->outputWidth / 2;
        chromaOutputHeight = info->outputHeight;
    }
    else if(info->outputFormat == JPEGDEC_YCbCr440)
    {
        lumaW = info->displayWidth;
        lumaH = (info->displayHeight + 1) & ~0x1;
        chromaW = lumaW;
        chromaH = lumaH / 2;
        chromaOutputWidth = info->outputWidth;
        chromaOutputHeight = info->outputHeight / 2;
    }
    else if(info->outputFormat == JPEGDEC_YCbCr411_SEMIPLANAR)
    {
        lumaW = (info->displayWidth + 3) & ~0x3;
        lumaH = info->displayHeight;
        chromaW = lumaW / 4;
        chromaH = lumaH;
        chromaOutputWidth = info->outputWidth / 4;
        chromaOutputHeight = info->outputHeight;
    }
    else if(info->outputFormat == JPEGDEC_YCbCr444_SEMIPLANAR)
    {
        lumaW = info->displayWidth;
        lumaH = info->displayHeight;
        chromaW = lumaW;
        chromaH = lumaH;
        chromaOutputWidth = info->outputWidth;
        chromaOutputHeight = info->outputHeight;
    }
    else
    {
        lumaW = info->displayWidth;
        lumaH = info->displayHeight;
        chromaW = 0;
        chromaH = 0;
        chromaOutputHeight = 0;
        chromaOutputHeight = 0;

    }

    /* write decoder output to file */
    pYuvOut = dataLuma;
    for(i = 0; i < lumaH; i++)
    {
        FS_FWrite(pYuvOut, sizeof(u8), lumaW, foutput);
        pYuvOut += info->outputWidth;
    }

    pYuvOut += (info->outputHeight - lumaH) * info->outputWidth;

    /* baseline -> output in semiplanar format */
    if(info->codingMode != JPEGDEC_PROGRESSIVE)
    {
        for(i = 0; i < chromaH; i++)
            for(j = 0; j < chromaW; j++)
                FS_FWrite(pYuvOut + i * chromaOutputWidth * 2 + j * 2,
                       sizeof(u8), 1, foutput);
        for(i = 0; i < chromaH; i++)
            for(j = 0; j < chromaW; j++)
                FS_FWrite(pYuvOut + i * chromaOutputWidth * 2 + j * 2 + 1,
                       sizeof(u8), 1, foutput);
    }
    else
    {
        pYuvOut = dataCb;
        for(i = 0; i < chromaH; i++)
        {
            FS_FWrite(pYuvOut, sizeof(u8), chromaW, foutput);
            pYuvOut += chromaOutputWidth;
        }
        /*pYuvOut += (chromaOutputHeight-chromaH)*chromaOutputWidth; */
        pYuvOut = dataCr;
        for(i = 0; i < chromaH; i++)
        {
            FS_FWrite(pYuvOut, sizeof(u8), chromaW, foutput);
            pYuvOut += chromaOutputWidth;
        }
    }

    FS_FClose(foutput);
}

void WriteProgressiveOutput(u32 sizeLuma, u32 sizeChroma, u32 mode,
                            u8 * dataLuma, u8 * dataCb, u8 * dataCr,
									 char *output)
{
    u32 i;
    FS_FILE *foutput = NULL;

    printf( "TB: WriteProgressiveOutput\n");

    //foutput = FS_FOpen("out.yuv", "ab");
	 foutput = FS_FOpen(output, "ab");
    if(foutput == NULL)
    {
        printf( "UNABLE TO OPEN OUTPUT FILE\n");
        return;
    }

    /* write decoder output to file */
    FS_FWrite(dataLuma, sizeof(u8), sizeLuma, foutput);
    FS_FWrite(dataCb, sizeof(u8), sizeChroma / 2, foutput);
    FS_FWrite(dataCr, sizeof(u8), sizeChroma / 2, foutput);

    FS_FClose(foutput);
}
/*------------------------------------------------------------------------------

    Function name: printJpegVersion

    Functional description: Print version info

    Inputs:

    Outputs:    NONE

    Returns:    NONE

------------------------------------------------------------------------------*/
void printJpegVersion(void)
{

    JpegDecApiVersion decVer;
    JpegDecBuild decBuild;

    /*
     * Get decoder version info
     */

    decVer = JpegGetAPIVersion();
    printf("\nAPI version:  %d.%d, ", decVer.major, decVer.minor);

    decBuild = JpegDecGetBuild();
    printf("sw build nbr: %d, hw build nbr: %x\n\n",
           decBuild.swBuild, decBuild.hwBuild);

}
