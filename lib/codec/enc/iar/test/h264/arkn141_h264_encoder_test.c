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

/* For command line structure */
#include "arkn141_h264_encoder.h"

/* For parameter parsing */
#include "EncGetOption.h"

/* For SW/HW shared memory allocation */
#include "ewl.h"

/* For accessing the EWL instance inside the encoder */
#include "H264Instance.h"

#include "types.h"

/* For printing and file IO */
#include <stdio.h>

/* For dynamic memory allocation */
#include <stdlib.h>

/* For memset, strcpy and strlen */
#include <string.h>

#include "xm_core_linear_memory.h"
#include "fs.h"

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

/*------------------------------------------------------------------------------

    Help
        Print out some instructions about usage.
------------------------------------------------------------------------------*/
static void Help(void)
{
    printf("Usage:  %s [options] -i inputfile\n\n", "h264_testenc");
    printf("Default parameters are marked inside []. More detailed description of\n"
                    "H.264 API parameters can be found from H.264 API Manual.\n\n");

    printf(
            " Parameters affecting which input frames are encoded:\n"
            "  -i[s] --input             Read input video sequence from file. [input.yuv]\n"
            "  -9[s] --inputMvc          Read MVC second view input video sequence from file. []\n"
            "  -o[s] --output            Write output H.264 stream to file. [stream.h264]\n"
            "  -a[n] --firstPic          First picture of input file to encode. [0]\n"
            "  -b[n] --lastPic           Last picture of input file to encode. [100]\n"
            "  -f[n] --outputRateNumer   1..1048575 Output picture rate numerator. [30]\n"
            "  -F[n] --outputRateDenom   1..1048575 Output picture rate denominator. [1]\n"
            "                               Encoded frame rate will be\n"
            "                               outputRateNumer/outputRateDenom fps\n"
            "  -j[n] --inputRateNumer    1..1048575 Input picture rate numerator. [30]\n"
            "  -J[n] --inputRateDenom    1..1048575 Input picture rate denominator. [1]\n\n");

    printf(
            " Parameters affecting input frame and encoded frame resolutions and cropping:\n"
            "  -w[n] --lumWidthSrc       Width of source image. [176]\n"
            "  -h[n] --lumHeightSrc      Height of source image. [144]\n");
    printf(
            "  -x[n] --width             Width of encoded output image. [--lumWidthSrc]\n"
            "  -y[n] --height            Height of encoded output image. [--lumHeightSrc]\n"
            "  -X[n] --horOffsetSrc      Output image horizontal cropping offset. [0]\n"
            "  -Y[n] --verOffsetSrc      Output image vertical cropping offset. [0]\n\n");

    printf(
            " Parameters for pre-processing frames before encoding:\n"
            "  -l[n] --inputFormat       Input YUV format. [0]\n"
            "                                0 - YUV420 planar\n"
            "                                1 - YUV420 semiplanar\n"
            "                                2 - YUYV422 interleaved\n"
            "                                3 - UYVY422 interleaved\n"
            "                                4 - RGB565 16bpp\n"
            "                                5 - BGR565 16bpp\n"
            "                                6 - RGB555 16bpp\n"
            "                                7 - BGR555 16bpp\n"
            "                                8 - RGB444 16bpp\n"
            "                                9 - BGR444 16bpp\n"
            "                                10 - RGB888 32bpp\n"
            "                                11 - BGR888 32bpp\n"
            "                                12 - RGB101010 32bpp\n"
            "                                13 - BGR101010 32bpp\n"
            "  -O[n] --colorConversion   RGB to YCbCr color conversion type. [0]\n"
            "                                0 - BT.601\n"
            "                                1 - BT.709\n"
            "                                2 - User defined, coeffs defined in testbench.\n");

    printf(
            "  -r[n] --rotation          Rotate input image. [0]\n"
            "                                0 - disabled\n"
            "                                1 - 90 degrees right\n"
            "                                2 - 90 degrees left\n"
            "  -Z[n] --videoStab         Enable video stabilization or scene change detection. [0]\n"
            "                                Stabilization works by adjusting cropping offset for\n"
            "                                every frame. Scene change detection encodes scene\n"
            "                                changes as intra frames, it is enabled when\n"
            "                                input resolution == encoded resolution.\n\n");

    printf(
            " Parameters affecting the output stream and encoding tools:\n"
            "  -L[n] --level             10..51, H264 Level. 40=Level 4.0 [40]\n"
            "                                Each level has resolution and bitrate limitations:\n"
            "                                10 - QCIF  (176x144)   75kbits\n"
            "                                20 - CIF   (352x288)   2.4Mbits\n"
            "                                30 - SD    (720x576)   12Mbits\n"
            "                                40 - 1080p (1920x1080) 24Mbits\n"
            "  -R[n] --byteStream        Stream type. [1]\n"
            "                                0 - NAL units. Nal sizes returned in <nal_sizes.txt>\n"
            "                                1 - byte stream according to H.264 Standard Annex B.\n");
    printf(
            "  -S[n] --sei               Enable SEI messages (buffering period + picture timing) [0]\n"
            "                                Writes Supplemental Enhancement Information messages\n"
            "                                containing information about picture time stamps\n"
            "                                and picture buffering into stream.\n");
    printf(
            "  -8[n] --trans8x8          0=OFF, 1=Adaptive, 2=ON [0]\n"
            "                                Adaptive setting enables 8x8 transform for >= 720p.\n"
            "                                Enabling 8x8 transform makes the stream High Profile.\n"
            "  -M[n] --quarterPixelMv    0=OFF, 1=Adaptive, 2=ON [1]\n"
            "                                Adaptive setting disables 1/4p MVs for > 720p.\n"
            "  -K[n] --enableCabac       0=OFF (CAVLC), 1=ON (CABAC),\n"
            "                                2=ON for Inter frames (intra=CAVLC, inter=CABAC) [0]\n"
            "                                Enabling CABAC makes the stream Main Profile.\n"
            "  -p[n] --cabacInitIdc      0..2 Initialization value for CABAC. [0]\n"
            "  -k[n] --videoRange        0..1 Video signal sample range value in H.264 stream. [0]\n"
            "                                0 - Y range in [16..235] Cb,Cr in [16..240]\n"
            "                                1 - Y,Cb,Cr range in [0..255]\n");

    printf(
            "  -T[n] --constIntraPred    0=OFF, 1=ON Constrained intra prediction flag [0]\n"
            "                                Improves stream error recovery by not allowing\n"
            "                                intra MBs to predict from neighboring inter MBs.\n"
            "  -D[n] --disableDeblocking 0..2 Value of disable_deblocking_filter_idc [0]\n"
            "                                0 = Inloop deblocking filter enabled (best quality)\n"
            "                                1 = Inloop deblocking filter disabled\n"
            "                                2 = Inloop deblocking filter disabled on slice edges\n"
            "  -I[n] --intraPicRate      Intra picture rate in frames. [0]\n"
            "                                Forces every Nth frame to be encoded as intra frame.\n"
            "  -V[n] --mbRowPerSlice     Slice size in macroblock rows. [0]\n\n");

    printf(
            " Parameters affecting the rate control and output stream bitrate:\n"
            "  -B[n] --bitPerSecond      10000..levelMax, target bitrate for rate control [1000000]\n"
            "  -U[n] --picRc             0=OFF, 1=ON Picture rate control. [1]\n"
            "                                Calculates new target QP for every frame.\n"
            "  -u[n] --mbRc              0=OFF, 1=ON Macroblock rate control (Check point rc). [1]\n"
            "                                Updates target QP inside frame.\n"
            "  -C[n] --hrdConformance    0=OFF, 1=ON HRD conformance. [0]\n"
            "                                Uses standard defined model to limit bitrate variance.\n"
            "  -c[n] --cpbSize           HRD Coded Picture Buffer size in bits. [0]\n"
            "                                Buffer size used by the HRD model.\n"
            "  -g[n] --gopLength         Group Of Pictures length in frames, 1..300 [intraPicRate]\n"
            "                                Rate control allocates bits for one GOP and tries to\n"
            "                                match the target bitrate at the end of each GOP.\n"
            "                                Typically GOP begins with an intra frame, but this\n"
            "                                is not mandatory.\n");

    printf(
            "  -s[n] --picSkip           0=OFF, 1=ON Picture skip rate control. [0]\n"
            "                                Allows rate control to skip frames if needed.\n"
            "  -q[n] --qpHdr             -1..51, Initial target QP. [26]\n"
            "                                -1 = Encoder calculates initial QP. NOTE: use -q-1\n"
            "                                The initial QP used in the beginning of stream.\n"
            "  -n[n] --qpMin             0..51, Minimum frame header QP. [10]\n"
            "  -m[n] --qpMax             0..51, Maximum frame header QP. [51]\n"
            "  -A[n] --intraQpDelta      -12..12, Intra QP delta. [-3]\n"
            "                                QP difference between target QP and intra frame QP.\n"
            "  -G[n] --fixedIntraQp      0..51, Fixed Intra QP, 0 = disabled. [0]\n"
            "                                Use fixed QP value for every intra frame in stream.\n"
            "  -2[n] --mbQpAdjustment    -8..7, MAD based MB QP adjustment, 0 = disabled. [0]\n"
            "                                Improves quality of macroblocks based on MAD value.\n"
            "  -1[n]:[n] --bpsAdjust     Frame:bitrate for adjusting bitrate on the fly.\n"
            "                                Sets new target bitrate at specific frame.\n\n");

    printf(
            " Other parameters for coding and reporting:\n"
            "  -z[n] --userData          SEI User data file name. File is read and inserted\n"
            "                                as SEI message before first frame.\n"
            "  -d    --psnr              Enables PSNR calculation for each frame.\n"
            "  -3[n] --mvOutput          Enable MV writing in <mv.txt> [0]\n\n");

    printf(
            "        --cir               start:interval for Cyclic Intra Refresh.\n"
            "                                Forces macroblocks in intra mode. [0:0]\n"
            "        --intraSliceMap1    32b hex bitmap for forcing slices 0..31\n"
            "                                coding as intra. LSB=slice0 [0]\n"
            "        --intraSliceMap2    32b hex bitmap for forcing slices 32..63\n"
            "                                coding as intra. LSB=slice32 [0]\n"
            "        --intraSliceMap3    32b hex bitmap for forcing slices 64..95\n"
            "                                coding as intra. LSB=slice64 [0]\n"
            "        --intraArea         left:top:right:bottom macroblock coordinates\n"
            "                                specifying rectangular area of MBs to\n"
            "                                force encoding in intra mode.\n"
            "        --roi1Area          left:top:right:bottom macroblock coordinates\n"
            "        --roi2Area          left:top:right:bottom macroblock coordinates\n"
            "                                specifying rectangular area of MBs as\n"
            "                                Region Of Interest with lower QP.\n"
            "        --roi1DeltaQp       -15..0, QP delta value for ROI 1 MBs. [0]\n"
            "        --roi2DeltaQp       -15..0, QP delta value for ROI 2 MBs. [0]\n"
            "        --viewMode          Mode for encoder view prediction and buffering [0]\n"
            "                                0 = Base view stream with two frame buffers\n"
            "                                1 = Base view stream with one frame buffer\n"
            "                                2 = Multiview stream with one frame buffer\n"
            "                                3 = Multiview stream with two frame buffers\n");

    printf(
            "\nTesting parameters that are not supported for end-user:\n"
            "  -Q[n] --chromaQpOffset    -12..12 Chroma QP offset. [2]\n"
            "  -W[n] --filterOffsetA     -6..6 Deblocking filter offset A. [0]\n"
            "  -E[n] --filterOffsetB     -6..6 Deblocking filter offset B. [0]\n"
            "  -N[n] --burstSize          0..63 HW bus burst size. [16]\n"
            "  -t[n] --burstType          0=SINGLE, 1=INCR HW bus burst type. [0]\n"
            "  -e[n] --testId            Internal test ID. [0]\n"
            "  -P[n] --trigger           Logic Analyzer trigger at picture <n>. [-1]\n"
            "\n");
    ;
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static int ParseDelim(char *optArg, char delim)
{
    i32 i;

    for (i = 0; i < (i32)strlen(optArg); i++)
        if (optArg[i] == delim)
        {
            optArg[i] = 0;
            return i;
        }

    return -1;
}

/*------------------------------------------------------------------------------

    Parameter
        Process the testbench calling arguments.

    Params:
        argc    - number of arguments to the application
        argv    - argument list as provided to the application
        cml     - processed comand line options   
    Return:
        0   - for success
        -1  - error    

------------------------------------------------------------------------------*/
static int Parameter(i32 argc, char **argv, x264_arkn141_commandLine * cml)
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
#ifdef ASIC_WAVE_TRACE_TRIGGER
        case 'P':
            trigger_point = atoi(optArg);
            break;
#endif
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

static void h264_WriteStrm(FS_FILE * fout, u32 * strmbuf, u32 size, u32 endian)
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
	 int ret = FS_FWrite(strmbuf, 1, size, fout);
    //if( ret != size )
	 {
		 printf ("h264_WriteStrm, size = %d, ret = %d\n", size, ret);
	 }
}

static i32 h264_NextPic(i32 inputRateNumer, i32 inputRateDenom, i32 outputRateNumer,
            i32 outputRateDenom, i32 frameCnt, i32 firstPic)
{
    u32 sift;
    u32 skip;
    u32 numer;
    u32 denom;
    u32 next;

    numer = (u32) inputRateNumer *(u32) outputRateDenom;
    denom = (u32) inputRateDenom *(u32) outputRateNumer;

    if(numer >= denom)
    {
        sift = 9;
        do
        {
            sift--;
        }
        while(((numer << sift) >> sift) != numer);
    }
    else
    {
        sift = 17;
        do
        {
            sift--;
        }
        while(((numer << sift) >> sift) != numer);
    }
    skip = (numer << sift) / denom;
    next = (((u32) frameCnt * skip) >> sift) + (u32) firstPic;

    return (i32) next;
}

/*------------------------------------------------------------------------------

    ReadPic

    Read raw YUV 4:2:0 or 4:2:2 image data from file

    Params:
        image   - buffer where the image will be saved
        size    - amount of image data to be read
        nro     - picture number to be read
        name    - name of the file to read
        width   - image width in pixels
        height  - image height in pixels
        format  - image format (YUV 420/422/RGB16/RGB32)

    Returns:
        0 - for success
        non-zero error code 
------------------------------------------------------------------------------*/
int h264_ReadPic(u8 * image, i32 size, i32 nro, FS_FILE *readFile, u32 file_size, i32 width, i32 height,
            i32 format)
{
	int ret = 0;
    /* Stop if over last frame of the file */
    if((u32)size * (nro + 1) > file_size)
    {
        printf("\nCan't read frame, EOF\n");
        ret = -1;
        goto end;
    }

    if(FS_FSeek(readFile, (u32)size * nro, SEEK_SET) != 0)
    {
        printf("\nI can't seek frame no: %i from file: %x\n",
                nro, (u32)readFile);
        ret = -1;
        goto end;
    }

    if((width & 0x0f) == 0)
        FS_FRead(image, 1, size, readFile);
    else
    {
        i32 i;
        u8 *buf = image;
        i32 scan = (width + 15) & (~0x0f);

        /* Read the frame so that scan (=stride) is multiple of 16 pixels */

        if(format == 0) /* YUV 4:2:0 planar */
        {
            /* Y */
            for(i = 0; i < height; i++)
            {
                FS_FRead(buf, 1, width, readFile);
                buf += scan;
            }
            /* Cb */
            for(i = 0; i < (height / 2); i++)
            {
                FS_FRead(buf, 1, width / 2, readFile);
                buf += scan / 2;
            }
            /* Cr */
            for(i = 0; i < (height / 2); i++)
            {
                FS_FRead(buf, 1, width / 2, readFile);
                buf += scan / 2;
            }
        }
        else if(format == 1)    /* YUV 4:2:0 semiplanar */
        {
            /* Y */
            for(i = 0; i < height; i++)
            {
                FS_FRead(buf, 1, width, readFile);
                buf += scan;
            }
            /* CbCr */
            for(i = 0; i < (height / 2); i++)
            {
                FS_FRead(buf, 1, width, readFile);
                buf += scan;
            }
        }
        else if(format <= 9)   /* YUV 4:2:2 interleaved or 16-bit RGB */
        {
            for(i = 0; i < height; i++)
            {
                FS_FRead(buf, 1, width * 2, readFile);
                buf += scan * 2;
            }
        }
        else    /* 32-bit RGB */
        {
            for(i = 0; i < height; i++)
            {
                FS_FRead(buf, 1, width * 4, readFile);
                buf += scan * 4;
            }
        }

    }

  end:

    return ret;
}


int arkn141_x264_encoder_test (int argc, char *argv[])
{
	x264_arkn141_commandLine cmdl, *cml;
	x264_arkn141_H264EncInst arkn141_enc_inst;
	x264_arkn141_input input;
	x264_arkn141_output output;
	
	char user_data[32];
	
	//H264EncInst 	enc;
	int ret;
	FS_FILE *readFile = NULL;
	FS_FILE *yuvFile = NULL;
	u32 file_size;
	
	xm_core_linear_memory_t pictureMem;
	xm_core_linear_memory_t pictureStabMem;
	xm_core_linear_memory_t outbufMem;
	
	int src_img_size;
	u32 pictureSize;
	
	u32 frameCnt = 0;
	u32 next = 0;
	
	FS_FILE *fout;
	
	cml = &cmdl;
	
	if(argc < 2)
	{
		Help();
		exit(0);
	}
	
	x264_arkn141_param_default (&cmdl);
	
	/* Parse command line parameters */
	if(Parameter(argc, argv, &cmdl) != 0)
	{
		printf("Input parameter error\n");
		return -1;
	}

	if(x264_arkn141_open_encoder (&cmdl, &arkn141_enc_inst))
	{
		printf ("can't open h264 encoder\n");
		return -1;
	}
	
 
	if(cml->inputFormat <= 1)
	{
		/* Input picture in planar YUV 4:2:0 format */
		pictureSize =
            ((cml->lumWidthSrc + 15) & (~15)) * cml->lumHeightSrc * 3 / 2;
	}
	else if((cml->inputFormat <= 9))
	{
		/* Input picture in YUYV 4:2:2 or 16-bit RGB format */
		pictureSize =
            ((cml->lumWidthSrc + 15) & (~15)) * cml->lumHeightSrc * 2;
	}
	else
	{
		/* Input picture in 32-bit RGB format */
		pictureSize =
            ((cml->lumWidthSrc + 15) & (~15)) * cml->lumHeightSrc * 4;
	}
	printf("Input %dx%d encoding at %dx%d\n", cml->lumWidthSrc,
           cml->lumHeightSrc, cml->width, cml->height);
	 
	 
	// 分配输入帧
	outbufMem.virtual_address = NULL;
	pictureMem.virtual_address = NULL;
	pictureStabMem.virtual_address = NULL;
	
	ret = xm_core_allocate_linear_memory (1024*1024, 2048, &outbufMem);
	if (ret != 0)
	{
		printf ("Failed to allocate outbufMem!\n");
		outbufMem.virtual_address = NULL;
		x264_arkn141_close_encoder (arkn141_enc_inst);
		ret = -1;
		goto encode_exit;
	}
	
	ret = xm_core_allocate_linear_memory (pictureSize, 2048, &pictureMem);
	if (ret != 0)
	{
		printf ("Failed to allocate input picture!\n");
		pictureMem.virtual_address = NULL;
		x264_arkn141_close_encoder (arkn141_enc_inst);
		ret = -1;
		goto encode_exit;
	}
       
	if(cml->videoStab > 0)
	{
		ret = xm_core_allocate_linear_memory(pictureSize, 2048, &pictureStabMem);
		if (ret != 0)
		{
			printf ("Failed to allocate stab input picture!\n");
			pictureStabMem.virtual_address = NULL;
			x264_arkn141_close_encoder (arkn141_enc_inst);
			ret = -1;
			goto encode_exit;
		}
	}	
	 
	printf("Input buffer size:          %d bytes\n", pictureMem.size);
	printf("Input buffer bus address:   0x%08x\n", pictureMem.bus_address);
	printf("Input buffer user address:  0x%08x\n", (u32) pictureMem.virtual_address);
	
	
    /* Source Image Size */
    if(cml->inputFormat <= 1)
    {
        src_img_size = cml->lumWidthSrc * cml->lumHeightSrc +
            2 * (((cml->lumWidthSrc + 1) >> 1) *
                 ((cml->lumHeightSrc + 1) >> 1));
    }
    else if((cml->inputFormat <= 9))
    {
        /* 422 YUV or 16-bit RGB */
        src_img_size = cml->lumWidthSrc * cml->lumHeightSrc * 2;
    }
    else
    {
        /* 32-bit RGB */
        src_img_size = cml->lumWidthSrc * cml->lumHeightSrc * 4;
    }

	printf("Reading input from file <%s>, frame size %d bytes.\n",
				cml->input, src_img_size);

	if (cml->inputMvc)
		printf("Reading second view input from file <%s>, frame size %d bytes.\n",
				cml->inputMvc, src_img_size);	
	
	yuvFile = FS_FOpen(cml->input, "rb");
   if(yuvFile == NULL)
   {
		printf("\nUnable to open YUV file: %s\n", cml->input);
		pictureStabMem.virtual_address = NULL;
		x264_arkn141_close_encoder (arkn141_enc_inst);
		ret = -1;
		goto encode_exit;
   }
   FS_FSeek(yuvFile, 0, SEEK_END);
   file_size = FS_FTell(yuvFile);
	readFile = yuvFile;

	// 生成H264流Header信息
	input.pOutBuf = (u32	 *)outbufMem.bus_address;
	input.busOutBuf = outbufMem.bus_address;
	input.outBufSize = outbufMem.size;
	if(x264_arkn141_encoder_headers (cml, arkn141_enc_inst, &input, &output) < 0)
	{
		x264_arkn141_close_encoder (arkn141_enc_inst);
		printf ("encoder h264 header error\n");
		ret = -1;
		goto encode_exit;
	}
	
	fout = FS_FOpen(cml->output, "wb");
	if(fout == NULL)
	{
		printf("Failed to create the output file.\n");
		ret = -1;
		goto encode_exit;
	}
	printf ("open output file (%s) success\n", cml->output);
	
	h264_WriteStrm (fout, (u32 *)outbufMem.bus_address, output.streamSize, 0);
		
	while((next = h264_NextPic(cml->inputRateNumer, cml->inputRateDenom,
                          cml->outputRateNumer, cml->outputRateDenom,
                          (cml->inputMvc) ? frameCnt/2 : frameCnt,
                          cml->firstPic)) <= cml->lastPic)
	{
		/* Read next frame */
		if(h264_ReadPic((u8 *) pictureMem.bus_address,
                    src_img_size, next,
                    readFile, file_size,
                   cml->lumWidthSrc, cml->lumHeightSrc, cml->inputFormat) != 0)
			break;

		if(cml->videoStab > 0)
		{
			/* Stabilize the frame after current frame */
			i32 nextStab = h264_NextPic(cml->inputRateNumer, cml->inputRateDenom,
                          cml->outputRateNumer, cml->outputRateDenom, frameCnt+1,
                          cml->firstPic);

			if(h264_ReadPic((u8 *) pictureStabMem.bus_address,
                        src_img_size, nextStab, readFile, file_size,
                       cml->lumWidthSrc, cml->lumHeightSrc,
                       cml->inputFormat) != 0)
				break;
		}

		/* Setup encoder input */
		u32 w = (cml->lumWidthSrc + 15) & (~0x0f);
		memset (&input, 0, sizeof(input));
		input.busLuma = pictureMem.bus_address;
		input.busChromaU = input.busLuma + (w * cml->lumHeightSrc);
		input.busChromaV = input.busChromaU +
			 (((w + 1) >> 1) * ((cml->lumHeightSrc + 1) >> 1));
		
		input.busLumaStab = pictureStabMem.bus_address;
		input.pOutBuf = (u32	 *)outbufMem.bus_address;
		input.busOutBuf = outbufMem.bus_address;
		input.outBufSize = outbufMem.size;
		
		memset (user_data, 0, sizeof(user_data));
		sprintf (user_data, "__XSPACE_2016_%04d__", frameCnt);
		//input.user_data = (void *)user_data;
		//input.user_data_size = strlen(user_data);

		ret = x264_arkn141_encoder_encode (arkn141_enc_inst, &input, &output, cml);
		if(ret > 0)
		{
			h264_WriteStrm (fout, (u32 *)outbufMem.bus_address, output.streamSize, 0);
		}
		else
		{
			printf ("x264_arkn141_encoder_encode failed, err = %d\n, src = %s\n", ret, cml->input);
			break;
		}
 		frameCnt++;
	}   /* End of main encoding loop */
	
	ret = x264_arkn141_encoder_encode (arkn141_enc_inst, NULL, &output, cml);
	if(ret > 0)
	{
		h264_WriteStrm (fout, (u32 *)outbufMem.bus_address, output.streamSize, 0);
	}

	printf ("arkn141_x264_encoder_test finished, src = %s\n", cml->input);
	
	ret = 0;
	FS_FClose (fout);
	
encode_exit:	
	
	if(yuvFile)
		FS_FClose (yuvFile);
	
	x264_arkn141_close_encoder (arkn141_enc_inst);
	
	xm_core_free_linear_memory (&pictureMem);
	xm_core_free_linear_memory (&pictureStabMem);
	xm_core_free_linear_memory (&outbufMem); 
	return ret;
}



						