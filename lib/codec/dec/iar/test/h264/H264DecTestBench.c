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
--  Abstract : H264 Decoder testbench for linux
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "ark1960.h"

/* For SW/HW shared memory allocation */
#include "dwl.h"

/* For Hantro H.264 decoder */
#include "h264decapi.h"

/* For printing and file IO */
#include <stdio.h>

/* For dynamic memory allocation */
#include <stdlib.h>

/* For memset, strcpy and strlen */
#include <string.h>

/* For sleep */
//#include <unistd.h>

#include "fs.h"

#include <assert.h>

#ifdef H264_DEBUG
#define	H264_DEBUG_PRINT(fmt, args...)		printf(fmt, ## args)	
#else
#define	H264_DEBUG_PRINT(fmt, args...)		
#endif

#define	off_t	unsigned long

#define	SAVE_H264_OFFSET

#ifdef SAVE_H264_OFFSET
static FS_FILE *fp_h264_offset;
#endif

//#define _CODEC_SOFT_RESET_		// 并发测试时关闭
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

#define H264ERR_OUTPUT stdout


/* Global variables */



/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static int savePictureIntoFile (FS_FILE *file, const u32 *picture, u32 picWidth, u32 picHeight)
{	
	if(file == NULL)
		return -1;
	
	if(FS_FWrite (picture, 1, picWidth * picHeight * 3/2, file) != picWidth * picHeight * 3/2)
		return -1;
	return 0;
}

// streamLen 	输入/输出参数，
//					输入时指定updateStreamBuffer
static int updateStreamBuffer (FS_FILE *h264File, u8 *updateStreamBuffer, u32 stream_bus_address, u32* streamLen)
{
	unsigned int size, to_read;
	if(h264File == NULL)
		return -1;
	size = *streamLen;
	if(size == 0)
		return 0;

#ifdef SAVE_H264_OFFSET
	if(fp_h264_offset)
	{
		I32 pos;		
		pos = FS_GetFilePos(h264File);
		FS_Write (fp_h264_offset, &pos, 4);
	}
#endif
	
	
	to_read = FS_FRead (updateStreamBuffer, 1, size, h264File);
	if(to_read == 0)
	{
		*streamLen = 0;
		return 0;
	}
	
	if( updateStreamBuffer[0] == 0
			  && updateStreamBuffer[1] == 0 
			  && updateStreamBuffer[2] == 0 
			  && updateStreamBuffer[3] == 1 )
	{
	}
	else
	{
		printf ("NAL stream error\n");
		return -1;
	}
	assert (	  updateStreamBuffer[0] == 0 
			  && updateStreamBuffer[1] == 0 
			  && updateStreamBuffer[2] == 0 
			  && updateStreamBuffer[3] == 1); 
			  
	
	if(to_read != size || to_read <= 4)
	{
		// 码流尾部
	}
	else 
	{
		// 向前扫描00 00 00 01
		u8 * last = updateStreamBuffer + to_read - 4;
		u8 * scan = last;
		while(scan > updateStreamBuffer)
		{
			if(scan[0] == 0x00 && scan[1] == 0x00 && scan[2] == 0x00 && scan[3] == 0x01)
				break;
			scan --;
		}
		assert(scan[0] == 0x00 && scan[1] == 0x00 && scan[2] == 0x00 && scan[3] == 0x01);
		assert (scan > updateStreamBuffer);
		FS_FSeek (h264File, scan - (updateStreamBuffer + to_read), FS_SEEK_CUR); 
		to_read = scan - updateStreamBuffer;
	}
	
	*streamLen = to_read;
	return 0;
}

void H264DecTrace (const char *str)
{
	H264_DEBUG_PRINT ("TRACE %s\n", str);
}

/*------------------------------------------------------------------------------

    main

------------------------------------------------------------------------------*/
#define	H264_DEC_BUFFER_LENGTH		0x200000

//#define	DBG_H264_WRITE

unsigned int h264decasic_time;
int h264_decode (const char *h264_file_name, const char *yuv_file_name)
{
	H264DecInst decoder;
	H264DecApiVersion apiVer;
	H264DecBuild decBuild;
	//i32 ret;
	FS_FILE *yuvFile;
	FS_FILE *h264File;
	DWLLinearMem_t info;
	DWLInitParam_t para;
	
#ifdef DBG_H264_WRITE	
	FS_FILE *dbgFile;		// 保存回写的H264文件
#endif
	
	H264DecRet ret;
	H264DecRet infoRet;
	H264DecInput decIn;
	H264DecOutput decOut;
	H264DecPicture decPic;
	H264DecInfo decInfo;	
	
	void const*x170_instance;
	 
	u8 *stream_virtual_address = NULL;
	u32 stream_bus_address = 0;
	u32 bufferLen;
	u32 picDecodeNumber = 0; /* decoded picture ID */	
	u32 picDisplayNumber = 0;

	apiVer = H264DecGetAPIVersion();
	decBuild = H264DecGetBuild();

	printf ("H.264 Decoder API version %d.%d\n", apiVer.major,
            apiVer.minor);
	printf ("HW ID: 0x%08x\t SW Build: %u.%u.%u\n\n",
            decBuild.hwBuild, decBuild.swBuild / 1000000,
            (decBuild.swBuild / 1000) % 1000, decBuild.swBuild % 1000);
	
	memset (&info, 0, sizeof(info));
	memset (&decOut, 0, sizeof(decOut));
	
	yuvFile = NULL;
	h264File = NULL;
	x170_instance = NULL;
	decoder = NULL;
	
#ifdef DBG_H264_WRITE	
	dbgFile = NULL;
#endif
	
	bufferLen = H264_DEC_BUFFER_LENGTH;
	
	picDecodeNumber = picDisplayNumber = 1;
	
#ifdef SAVE_H264_OFFSET
	fp_h264_offset = FS_FOpen ("\\H264_OFF.BIN", "wb");
#endif


	//ret = -1;
	do 
	{
		/* Check that input file exists */
		h264File = FS_FOpen(h264_file_name, "rb");
		if(h264File == NULL)
		{
			printf ("h264_decode failed, Unable to open input file: %s\n", h264_file_name);
			break;
    	}
		
		yuvFile = FS_FOpen (yuv_file_name, "wb");
		if(yuvFile == NULL)
		{
			printf ("h264_decode failed, Unable to open output file: %s\n", yuv_file_name);
			break;
		}
		
#ifdef DBG_H264_WRITE	
		{
		char dbg_file[64];
		strcpy (dbg_file, yuv_file_name);
		char *dot = strchr (dbg_file, '.');
		assert (dot);
		strcpy (dot + 1, "264");
		dbgFile = FS_FOpen (dbg_file, "wb");
		}
#endif
		
		para.clientType = DWL_CLIENT_TYPE_H264_DEC;
		x170_instance = DWLInit (&para);
		if(x170_instance == NULL)
		{
			printf ("h264_decode failed, Unable to init H264 instance of X170\n");
			break;			
		}
		
		// 分配输入缓存
		if(DWLMallocLinear (x170_instance, H264_DEC_BUFFER_LENGTH, &info) != DWL_OK)
		{
			printf ("h264_decode failed, Unable malloc input buffer\n");
			break;
		}
		
		stream_virtual_address = (u8 *)info.virtualAddress;
		stream_bus_address = info.busAddress;
		bufferLen = H264_DEC_BUFFER_LENGTH;
		
		// 读取流数据
		dma_flush_range (info.busAddress, info.busAddress + H264_DEC_BUFFER_LENGTH);
		updateStreamBuffer (h264File, stream_virtual_address, stream_bus_address, &bufferLen);
		dma_flush_range (info.busAddress, info.busAddress + H264_DEC_BUFFER_LENGTH);
		
		
		/* Configure the decoder input structure for the first decoding unit */
		decIn.dataLen = bufferLen;
		decIn.pStream = (u8 *)info.virtualAddress;	
		decIn.streamBusAddress = info.busAddress;
		
#ifdef DBG_H264_WRITE	
		if(dbgFile)
			FS_Write (dbgFile, decIn.pStream, decIn.dataLen);
#endif		
		
		/* Decoder initialization, output reordering enabled */
		/* check for any initialization errors must be done */
		infoRet = H264DecInit(&decoder, 0, 0, 0);
		if(infoRet != H264DEC_OK)
		{
			break;
		}
		
		h264decasic_time = 0;
		
		do
		{
#ifdef _CODEC_SOFT_RESET_
        rSYS_SOFT_RSTNA &= ~((1 << 6)|(1 << 21));
        delay (10);
        rSYS_SOFT_RSTNA |=  ((1 << 6)|(1 << 21));
#endif
         
         
			/* Picture ID is the picture number in decoding order */
			decIn.picId = picDecodeNumber;
			ret = H264DecDecode(decoder, &decIn, &decOut);		
			switch(ret)
			{
				case H264DEC_OK:
				/* nothing to do, just call again */
 				break;
				
				
				case H264DEC_STREAM_NOT_SUPPORTED:
				{
					 printf(("ERROR: UNSUPPORTED STREAM!\n"));
					 goto end;
				}
				
				case H264DEC_HDRS_RDY:
					/* read stream info */
					infoRet = H264DecGetInfo(decoder, &decInfo);
					if(infoRet == H264DEC_OK)
					{
						printf ("h264 dec, img width=%d, height=%d\n", decInfo.picWidth, decInfo.picHeight);
						H264_DEBUG_PRINT(("Cropping params: (%d, %d) %dx%d\n",
                             decInfo.cropParams.cropLeftOffset,
                             decInfo.cropParams.cropTopOffset,
                             decInfo.cropParams.cropOutWidth,
                             decInfo.cropParams.cropOutHeight));

						H264_DEBUG_PRINT(("MonoChrome = %d\n", decInfo.monoChrome));
						H264_DEBUG_PRINT(("Interlaced = %d\n", decInfo.interlacedSequence));                
						H264_DEBUG_PRINT(("Pictures in DPB = %d\n", decInfo.picBuffSize));
						H264_DEBUG_PRINT(("Pictures in Multibuffer PP = %d\n", decInfo.multiBuffPpSize));                
					}
					
					break;
					
				case H264DEC_ADVANCED_TOOLS:
                /* ASO/STREAM ERROR was noticed in the stream. The decoder has to
                 * reallocate resources */
                //assert(decOut.dataLeft); /* we should have some data left *//* Used to indicate that picture decoding needs to finalized prior to corrupting next picture */

                /* Used to indicate that picture decoding needs to finalized prior to corrupting next picture
                 * picRdy = 0; */
					break;
					
				case H264DEC_PIC_DECODED: /* a picture was processed by HW */
					/* The decoder output is ready now */
					/* output reordering was disabled -> last decoded picture is */
					/* always available */
					//printf ("\th264, frame = %03d\n", picDecodeNumber);
					/* use function H264DecNextPicture() to obtain next picture
					* in display order. Function is called until no more images
					* are ready for display */
					while (H264DecNextPicture(decoder, &decPic, 0) == H264DEC_PIC_RDY)
					{
						savePictureIntoFile (yuvFile, decPic.pOutputPicture, decPic.picWidth, decPic.picHeight);	
						/* Increment display number for every displayed picture */
						picDisplayNumber++;
						
					}
					
					/* Increment decoding number after every decoded picture */
					picDecodeNumber++;
					
					//if(picDecodeNumber > 200)
					//if(picDecodeNumber > 20)
					//	break;
					break;					
					
				case H264DEC_STRM_PROCESSED:
				case H264DEC_STRM_ERROR:
				{
					/* Used to indicate that picture decoding needs to finalized prior to corrupting next picture
					* picRdy = 0; */

					break;
				}
					
 				case H264DEC_HW_TIMEOUT:
					H264_DEBUG_PRINT(("Timeout\n"));
 					goto end;
					
				default:
					printf ("H264DecDecode error code=%d\n", ret);
					/* all other cases are errors where decoding cannot continue */
					goto end;				
			}
			
			if(decOut.dataLeft == 0)
			{
				/* new buffer needed */
				bufferLen = H264_DEC_BUFFER_LENGTH;
				dma_flush_range (info.busAddress, info.busAddress + H264_DEC_BUFFER_LENGTH);
				updateStreamBuffer (h264File, stream_virtual_address, stream_bus_address, &bufferLen);
				dma_flush_range (info.busAddress, info.busAddress + H264_DEC_BUFFER_LENGTH);
				decIn.pStream = (u8 *)info.virtualAddress;	
				decIn.streamBusAddress = info.busAddress;
				decIn.dataLen = bufferLen;
				
#ifdef DBG_H264_WRITE	
				if(dbgFile)
					FS_Write (dbgFile, decIn.pStream, decIn.dataLen);
#endif		
				
			}
			else
			{
				/* data left undecoded */
				decIn.dataLen = decOut.dataLeft;
				decIn.pStream = decOut.pStrmCurrPos;
				decIn.streamBusAddress = decOut.strmCurrBusAddress;
			}
			
			//if(picDecodeNumber > 10)
			//	break;
			
		} while(decIn.dataLen > 0); /* keep decoding until all data consumed */
		
		//ret = 0;
		/* if output in display order is preferred, the decoder shall be forced
		* to output pictures remaining in decoded picture buffer. Use function
		* H264DecNextPicture() to obtain next picture in display order. Function
		* is called until no more images are ready for display. Second parameter
		* for the function is set to '1' to indicate that this is end of the
		* stream and all pictures shall be output */
 		while(H264DecNextPicture(decoder, &decPic, 1) == H264DEC_PIC_RDY)
		{
			H264_DEBUG_PRINT(("PIC %d, type %s", picDisplayNumber,
					decPic.isIdrPicture ? "IDR" : "NON-IDR"));
			if(picDisplayNumber != decPic.picId)
				H264_DEBUG_PRINT((", decoded pic %d", decPic.picId));
			if(decPic.nbrOfErrMBs)
			{
				H264_DEBUG_PRINT((", concealed %d\n", decPic.nbrOfErrMBs));
			}
			if(decPic.interlaced)
			{
				H264_DEBUG_PRINT((", INTERLACED "));
				if(decPic.fieldPicture)
				{
					H264_DEBUG_PRINT(("FIELD %s", decPic.topField ? "TOP" : "BOTTOM"));
				}
				else
				{
					H264_DEBUG_PRINT(("FRAME"));
				}
			}

			H264_DEBUG_PRINT(("\n"));
			//fflush(stdout);

			//numErrors += decPic.nbrOfErrMBs;

			/* Write output picture to file */
			savePictureIntoFile (yuvFile, 
										decPic.pOutputPicture, 
										decPic.picWidth, 
										decPic.picHeight);



			/* Increment display number for every displayed picture */
			picDisplayNumber++;
		}		
		
		/* every picture has been displayed after it was decoded, so, no worries */
		/* about any picture left buffered inside the decoder */
end:
		H264DecRelease(decoder);
	}	while (0);
	
	if(picDecodeNumber)
	{
		printf ("asic_dec_times=%d, frame=%d, avg_asic_dec_time=%d\n", 
			  h264decasic_time, picDecodeNumber, h264decasic_time/picDecodeNumber);
	}

	if(yuvFile)
		FS_FClose (yuvFile);
	if(h264File)
		FS_FClose (h264File);
	
#ifdef DBG_H264_WRITE	
	if(dbgFile)
		FS_FClose (dbgFile);
#endif	
	
#ifdef SAVE_H264_OFFSET
	if(fp_h264_offset)
	{
		FS_FClose (fp_h264_offset);
		fp_h264_offset = NULL;
	}
#endif	
	
	FS_CACHE_Clean (""); 
	
	DWLFreeLinear (x170_instance, &info);
	if(x170_instance)
		DWLRelease (x170_instance);

	return 0;
}

// streamLen 	输入/输出参数，
//					输入时指定updateStreamBuffer
static int read_file_test_updateStreamBuffer (FS_FILE *h264File, u8 *updateStreamBuffer, u32 stream_bus_address, u32* streamLen)
{
	unsigned int size, to_read;
	if(h264File == NULL)
		return -1;
	size = *streamLen;
	if(size == 0)
		return 0;
	
	
	to_read = FS_FRead (updateStreamBuffer, 1, size, h264File);
	if(to_read == 0)
	{
		*streamLen = 0;
		return 0;
	}
	
	if( updateStreamBuffer[0] == 0
			  && updateStreamBuffer[1] == 0 
			  && updateStreamBuffer[2] == 0 
			  && updateStreamBuffer[3] == 1 )
	{
	}
	else
	{
		printf ("NAL stream error\n");
		return -1;
	}
	assert (	  updateStreamBuffer[0] == 0 
			  && updateStreamBuffer[1] == 0 
			  && updateStreamBuffer[2] == 0 
			  && updateStreamBuffer[3] == 1); 
			  
	
	if(to_read != size || to_read <= 4)
	{
		// 码流尾部
	}
	else 
	{
		// 向前扫描00 00 00 01
		u8 * last = updateStreamBuffer + to_read - 4;
		u8 * scan = last;
		while(scan > updateStreamBuffer)
		{
			if(scan[0] == 0x00 && scan[1] == 0x00 && scan[2] == 0x00 && scan[3] == 0x01)
				break;
			scan --;
		}
		assert(scan[0] == 0x00 && scan[1] == 0x00 && scan[2] == 0x00 && scan[3] == 0x01);
		assert (scan > updateStreamBuffer);
		FS_FSeek (h264File, scan - (updateStreamBuffer + to_read), FS_SEEK_CUR); 
		to_read = scan - updateStreamBuffer;
	}
	
	*streamLen = to_read;
	return 0;
}

int h264_decode_file_read_test (const char *h264_file_name, const char *yuv_file_name)
{
	H264DecInst decoder;
	H264DecApiVersion apiVer;
	H264DecBuild decBuild;
	//i32 ret;
	FS_FILE *yuvFile;
	FS_FILE *h264File;
	DWLLinearMem_t info;
	DWLInitParam_t para;
	
#ifdef DBG_H264_WRITE	
	FS_FILE *dbgFile;		// 保存回写的H264文件
#endif
	
	H264DecRet ret;
	H264DecRet infoRet;
	H264DecInput decIn;
	H264DecOutput decOut;
	H264DecPicture decPic;
	H264DecInfo decInfo;	
	
	void const*x170_instance;
	 
	u8 *stream_virtual_address = NULL;
	u32 stream_bus_address = 0;
	u32 bufferLen;
	u32 picDecodeNumber = 0; /* decoded picture ID */	
	u32 picDisplayNumber = 0;

	apiVer = H264DecGetAPIVersion();
	decBuild = H264DecGetBuild();

	printf ("H.264 Decoder API version %d.%d\n", apiVer.major,
            apiVer.minor);
	printf ("HW ID: 0x%08x\t SW Build: %u.%u.%u\n\n",
            decBuild.hwBuild, decBuild.swBuild / 1000000,
            (decBuild.swBuild / 1000) % 1000, decBuild.swBuild % 1000);
	
	memset (&info, 0, sizeof(info));
	memset (&decOut, 0, sizeof(decOut));
	
	yuvFile = NULL;
	h264File = NULL;
	x170_instance = NULL;
	decoder = NULL;
	
#ifdef DBG_H264_WRITE	
	dbgFile = NULL;
#endif
	
	bufferLen = H264_DEC_BUFFER_LENGTH;
	
	picDecodeNumber = picDisplayNumber = 1;
	

	//ret = -1;
	do 
	{
		/* Check that input file exists */
		h264File = FS_FOpen(h264_file_name, "rb");
		if(h264File == NULL)
		{
			printf ("h264_decode failed, Unable to open input file: %s\n", h264_file_name);
			break;
    	}
		
		yuvFile = FS_FOpen (yuv_file_name, "wb");
		if(yuvFile == NULL)
		{
			printf ("h264_decode failed, Unable to open output file: %s\n", yuv_file_name);
			break;
		}
		
#ifdef DBG_H264_WRITE	
		{
		char dbg_file[64];
		strcpy (dbg_file, yuv_file_name);
		char *dot = strchr (dbg_file, '.');
		assert (dot);
		strcpy (dot + 1, "264");
		dbgFile = FS_FOpen (dbg_file, "wb");
		}
#endif
		
		para.clientType = DWL_CLIENT_TYPE_H264_DEC;
		x170_instance = DWLInit (&para);
		if(x170_instance == NULL)
		{
			printf ("h264_decode failed, Unable to init H264 instance of X170\n");
			break;			
		}
		
		// 分配输入缓存
		if(DWLMallocLinear (x170_instance, H264_DEC_BUFFER_LENGTH, &info) != DWL_OK)
		{
			printf ("h264_decode failed, Unable malloc input buffer\n");
			break;
		}
		
		stream_virtual_address = (u8 *)info.virtualAddress;
		stream_bus_address = info.busAddress;
		bufferLen = H264_DEC_BUFFER_LENGTH;
		
		// 读取流数据
		dma_flush_range (info.busAddress, info.busAddress + H264_DEC_BUFFER_LENGTH);
		read_file_test_updateStreamBuffer (h264File, stream_virtual_address, stream_bus_address, &bufferLen);
		dma_flush_range (info.busAddress, info.busAddress + H264_DEC_BUFFER_LENGTH);
		
		
		/* Configure the decoder input structure for the first decoding unit */
		decIn.dataLen = bufferLen;
		decIn.pStream = (u8 *)info.virtualAddress;	
		decIn.streamBusAddress = info.busAddress;
		
#ifdef DBG_H264_WRITE	
		if(dbgFile)
			FS_Write (dbgFile, decIn.pStream, decIn.dataLen);
#endif		
		
		/* Decoder initialization, output reordering enabled */
		/* check for any initialization errors must be done */
		infoRet = H264DecInit(&decoder, 0, 0, 0);
		if(infoRet != H264DEC_OK)
		{
			break;
		}
		
		h264decasic_time = 0;
		
		do
		{
			/* Picture ID is the picture number in decoding order */
			decIn.picId = picDecodeNumber;
			
			decOut.dataLeft = 0;
			
			if(decOut.dataLeft == 0)
			{
				/* new buffer needed */
				bufferLen = H264_DEC_BUFFER_LENGTH;
				dma_flush_range (info.busAddress, info.busAddress + H264_DEC_BUFFER_LENGTH);
				read_file_test_updateStreamBuffer (h264File, stream_virtual_address, stream_bus_address, &bufferLen);
				dma_flush_range (info.busAddress, info.busAddress + H264_DEC_BUFFER_LENGTH);
				decIn.pStream = (u8 *)info.virtualAddress;	
				decIn.streamBusAddress = info.busAddress;
				decIn.dataLen = bufferLen;
				
#ifdef DBG_H264_WRITE	
				if(dbgFile)
					FS_Write (dbgFile, decIn.pStream, decIn.dataLen);
#endif		
				
			}
			else
			{
				/* data left undecoded */
				decIn.dataLen = decOut.dataLeft;
				decIn.pStream = decOut.pStrmCurrPos;
				decIn.streamBusAddress = decOut.strmCurrBusAddress;
			}
			
			//if(picDecodeNumber > 20)
			//	break;
			
		} while(decIn.dataLen > 0); /* keep decoding until all data consumed */
		

		H264DecRelease(decoder);
	}	while (0);
	

	if(yuvFile)
		FS_FClose (yuvFile);
	if(h264File)
		FS_FClose (h264File);
	
#ifdef DBG_H264_WRITE	
	if(dbgFile)
		FS_FClose (dbgFile);
#endif	
	
	
	FS_CACHE_Clean (""); 
	
	DWLFreeLinear (x170_instance, &info);
	if(x170_instance)
		DWLRelease (x170_instance);

	return 0;	
}

int cache_cmp;
char *cache_cmp_addr;
int cache_cmp_size;
unsigned int cache_cmp_off;

#include "rtos.h"

unsigned int sd_read_addr;
#define	_READ_PAGE_
int cache_test (const char *h264_file_name)
{
	FS_FILE *fp;
	int ret = -1;
	unsigned int size, file_size;
	unsigned int off;
	unsigned int to_read, readed;
	char *comp_addr, *aligned_comp_addr;
	char *data_addr, *aligned_data_addr;
	char *sram_base = (char *)0x300000;
	
	printf ("cache_test\n");
	
	fp = NULL;
	comp_addr = NULL;
	data_addr = NULL;
	
	do
	{
		fp = FS_FOpen(h264_file_name, "rb");
		if(fp == NULL)
		{
			printf ("can't open (%s)\n", h264_file_name);
			break;
		}
		comp_addr = malloc (0x480000);
		if(comp_addr == NULL)
		{
			FS_FClose (fp);
			printf ("malloc 0x480000 failed\n");
			break;
		}
		memset (comp_addr, 0, 0x480000);
		aligned_comp_addr = (char *)((((unsigned int)comp_addr) + 0xFFFF) & ~0xFFFF);
		file_size = FS_Read (fp, aligned_comp_addr, 0x480000);
		if(file_size == 0)
		{
			FS_FClose (fp);
			printf ("read (%s) NG\n", h264_file_name);
			break;
		}
		
		FS_FClose (fp);
		
		
		data_addr = malloc (0x480000);
		if(data_addr == NULL)
		{
			printf ("malloc 0x280000 failed\n");
			break;
		}
		
		//aligned_data_addr = data_addr;
		aligned_data_addr = (char *)((((unsigned int)data_addr) + 0xFFFF) & ~0xFFFF);
		
		int i;
		for (i = 0; i < 100; i ++)
		{
			memset (data_addr, 0, 0x480000);
			dma_flush_range (data_addr, data_addr + 0x480000);
			fp = FS_FOpen(h264_file_name, "rb");

			off = 0;
#ifdef _READ_PAGE_
			to_read = 32;
#else			
			to_read = 1;
#endif
			
			//OS_EnterRegion ();
			
			do
			{
				
				dma_inv_range (aligned_data_addr + off, aligned_data_addr + off + to_read);
				//printf ("read buff=0x%08x\n", aligned_data_addr + off);
				//char *addr = aligned_data_addr + off;
				char *addr = sram_base;
				//readed = FS_Read (fp, aligned_data_addr + off, to_read);
				
				if((off & 511) == 0)
				{
					cache_cmp = 1;
					cache_cmp_addr = aligned_comp_addr + off;
					cache_cmp_size = 32;
					cache_cmp_off = off;
				}
				readed = FS_Read (fp, addr, to_read);
				if(readed)
				{
					if(memcmp (addr, aligned_comp_addr + off, readed))
					{
						#define GPIO_BASE					0x40409000			// apb_gpio		32'h4040_9000,4K

						#define rGPIO_PA_MOD							(*(volatile unsigned int *)(GPIO_BASE + 0x00))

						rGPIO_PA_MOD = 0x01; 
						printf ("FS cmp ng, dma_addr=0x%08x, buf_addr=0x%08x, off=0x%08x\n", 
								  sd_read_addr,
								  aligned_comp_addr + off,
								  off);
						
						printf ("sram 0x300000\n");
						int j;
						for (j = 0; j < 16; j++)
						{
							printf ("%02x ", *(addr + j));
						}
						printf ("\n");
						for (j = 16; j < 24; j++)
						{
							printf ("%02x ", *(addr + j));
						}
						printf ("\n");
						printf ("dram %08x\n", aligned_comp_addr + off);
						//int j;
						for (j = 0; j < 16; j++)
						{
							printf ("%02x ", *(aligned_comp_addr + off + j));
						}
						printf ("\n");
						for (j = 16; j < 24; j++)
						{
							printf ("%02x ", *(aligned_comp_addr + off + j));
						}
						printf ("\n");
					}
					memcpy (aligned_data_addr + off, addr, readed);
				}
				else
				{
					break;
				}
				
#ifdef _READ_PAGE_
#else
				to_read ++;
#endif
				off += readed;
				
			} while (1);
			
			//OS_LeaveRegion ();
			
			FS_FClose (fp);
			
			char filename[64];
			sprintf (filename, "\\%02d.BIN", i);
			FS_FILE *fp_bin = FS_FOpen (filename, "wb");;
			if(fp_bin)
			{
				FS_Write (fp_bin, aligned_data_addr, file_size);
				FS_FClose (fp_bin);
			}
			
			FS_CACHE_Clean (""); 
			
			printf ("file compare ...\n");
			if(xm_file_compare (h264_file_name, filename) == 0)
			{
				printf ("cmp OK\n");
			}
			else
			{
				printf ("cmp NG\n");
			}
		
			
		}
		
	} while(1);
	
	if(comp_addr)
		free (comp_addr);
	if(data_addr)
		free (data_addr);
	
	return 0;
}