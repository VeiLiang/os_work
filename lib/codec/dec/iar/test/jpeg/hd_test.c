#include "regdrv.h"
#include "deccfg.h"

#include "H264decapi.h"
#include "Rvdecapi.h"
#include "on2rvdecapi.h"
#include "jpegdecapi.h"
#include "jpegdeccontainer.h"
#include "mpeg2decapi.h"
#include "mp4decapi.h" 
#include "vc1decapi.h" 
#include "vp8decapi.h" 
#include "ppapi.h" 


void hd264_test()
{
	
   u8 *stream_virtual_address = NULL;
   u8 *byteStrmStart;
   u32 stream_bus_address = 0;
   u32 streamLen, bufferLen;
   u32 picDecodeNumber = 0; /* decoded picture ID */
   
   H264DecInst decoder; 
   H264DecRet ret;
   H264DecRet infoRet;
   H264DecInput decIn;
   H264DecOutput decOut;
   H264DecPicture decPic;
   H264DecInfo decInfo;
	
	/* The implementation of the following is system dependent */
	//allocInputBuffer(&stream_virtual_address, &stream_bus_address, &streamLen);
	//updateStreamBuffer(&stream_virtual_address, &stream_bus_address,&bufferLen);
	
	/* Configure the decoder input structure for the first decoding unit */
	stream_virtual_address = (u8*) 0x6000000 ;
	byteStrmStart =(u8*)stream_virtual_address  ;
	decIn.dataLen =0x400000; 
	decIn.pStream = byteStrmStart ;//stream_virtual_address;
	stream_bus_address = (u32) stream_virtual_address ;
	decIn.streamBusAddress =(u32) stream_bus_address ; //stream_bus_address;
	
	*((volatile unsigned int *)(0xe0300000)) = 0x20; 
	infoRet = H264DecInit(&decoder,0,0,0);
	*((volatile unsigned int *)(0xe0300000)) = 0x21;   
	if(infoRet !=H264DEC_OK)
  	{ goto end;}
	do
  	{
		/* Picture ID is the picture number in decoding order */
		decIn.picId = picDecodeNumber;
		/* decode the stream data */
		*((volatile unsigned int *)(0xe0300000)) = 0x22;   
		ret = H264DecDecode(decoder, &decIn, &decOut);
		*((volatile unsigned int *)(0xe0300000)) = 0x23;   
		*((volatile unsigned int *)(0xE0000000 +0x398)) = 0xc0;
		*((volatile unsigned int *)(0xE0000000 +0x398)) = ret; 
		*((volatile unsigned int *)(0xe0300000)) = ret;   	
		switch(ret)
		{
		case H264DEC_HDRS_RDY:
			/* read stream info */
			infoRet = H264DecGetInfo(decoder, &decInfo);
			break;
		case H264DEC_ADVANCED_TOOLS:
			/* Arbitrary Slice Order/multiple slice groups noticed */
			/* in the stream. The decoder has to reallocate resources */
			break;
		case H264DEC_PIC_DECODED: /* a picture was decoded */
			/* The decoder output is ready, now obtain pictures in */
			/* display order, if any available */
			while(H264DecNextPicture(decoder, &decPic, 0) == H264DEC_PIC_RDY )
			{
				//displayOnScreen( decPic.pOutputPicture, decPic.picWidth,decPic.picHeight);
				
			}
			/* Increment decoding number after every decoded picture */
			picDecodeNumber++;
			break;
		case H264DEC_STRM_PROCESSED:
			/* Input stream was processed but no picture is ready */
			break;
		default:
			/* all other cases are errors where decoding cannot continue */
			goto end;
		}
		if(decOut.dataLeft == 0)
		{
			/* new buffer needed */
			//updateStreamBuffer(&stream_virtual_address, &stream_bus_address,&bufferLen);
		}
		else
		{
			/* data left undecoded */
			decIn.dataLen = decOut.dataLeft;
			decIn.pStream = decOut.pStrmCurrPos;
			decIn.streamBusAddress = decOut.strmCurrBusAddress;
		}
		/* keep decoding until all data from input stream buffer consumed */
	}
	while(decIn.dataLen > 0);
	/* stream ended, obtain remaining pictures in display order, if any
	available */
	while ( H264DecNextPicture(decoder, &decPic, 1) == H264DEC_PIC_RDY )
	{
		//displayOnScreen(decPic.pOutputPicture, decPic.picWidth,decPic.picHeight);
	}
end:
	H264DecRelease(decoder);
	
} 

void hdrv_test()
{
   u8 *stream_virtual_address = NULL;
   u8 *byteStrmStart;
   u32 stream_bus_address = 0;
   u32 streamLen, bufferLen;
   u32 picDecodeNumber = 0; /* decoded picture ID */
   On2RvDecRet infoRet;
   //H264DecInst decoder; 
   //H264DecRet ret;
   //H264DecRet infoRet;
   //H264DecInput decIn;
   //H264DecOutput decOut;
   //H264DecPicture decPic;
   //H264DecInfo decInfo;
   RvDecInst       decoder;
   On2DecoderOutParams  decOut ;
   On2DecoderInParams   decIn  ;
   On2DecoderInit       decinit; 
   On2RvMsgSetDecoderRprSizes msg_id;
   codecSegmentInfo slice_info[16];
	/* The implementation of the following is system dependent */
	//allocInputBuffer(&stream_virtual_address, &stream_bus_address, &streamLen);
	//updateStreamBuffer(&stream_virtual_address, &stream_bus_address,&bufferLen);
	
	/* Configure the decoder input structure for the first decoding unit */
#if 0
	//rv8
	stream_virtual_address = (u8 *) 0x6000040;
	byteStrmStart =(u8*)stream_virtual_address  ;
	
	decIn.dataLength =0x5a; 
	decIn.numDataSegments = 0x1;
	decIn.timestamp=0x1;
	decIn.flags = 0x2;
	decIn.pDataSegments[0].bIsValid = 0x1;
	decIn.pDataSegments[0].ulSegmentOffset = 0x0;
	
	stream_bus_address = (u32) stream_virtual_address ;
	decIn.streamBusAddr =(u32) stream_bus_address ; //stream_bus_address;
	
	msg_id.message_id = 0x1234;
	msg_id.num_sizes = 0x2;
	msg_id.sizes[0]=176;
	msg_id.sizes[1]=144;
	
	decinit.pels  = 176;
	decinit.lines = 144;
	decinit.nPadHeight = 0x0;
	decinit.nPadWidth  = 0x0;
	decinit.ulStreamVersion = 0x30203002;
#else
#if 0
	// bare
	stream_virtual_address = (u8 *) 0x6000080;
	byteStrmStart =(u8*)stream_virtual_address  ;
	
	decIn.dataLength =0x2650; 
	decIn.numDataSegments = 0x9;
	decIn.timestamp=0x0;
	decIn.flags = 0x2;
	slice_info[0].bIsValid = 0x1;
	slice_info[0].ulSegmentOffset = 0;
	slice_info[1].bIsValid = 0x1;
	slice_info[1].ulSegmentOffset = 0x0533;
	slice_info[2].bIsValid = 0x1;
	slice_info[2].ulSegmentOffset = 0x0A4C;
	slice_info[3].bIsValid = 0x1;
	slice_info[3].ulSegmentOffset = 0x0F32;
	slice_info[4].bIsValid = 0x1;
	slice_info[4].ulSegmentOffset = 0x145A;
	slice_info[5].bIsValid = 0x1;
	slice_info[5].ulSegmentOffset = 0x14AB;
	slice_info[6].bIsValid = 0x1;
	slice_info[6].ulSegmentOffset = 0x19CF;
	slice_info[7].bIsValid = 0x1;
	slice_info[7].ulSegmentOffset = 0x1EC1;
	slice_info[8].bIsValid = 0x1;
	slice_info[8].ulSegmentOffset = 0x2367;
	decIn.pDataSegments = &slice_info[0];
	stream_bus_address = (u32) stream_virtual_address ;
	decIn.streamBusAddr =(u32) stream_bus_address ; //stream_bus_address;
	
	msg_id.message_id = 0x1234;
	msg_id.num_sizes = 0x1;
	msg_id.sizes[0]=352;
	msg_id.sizes[1]=288;
	
	decinit.pels  = 352;
	decinit.lines = 288;
	decinit.nPadHeight = 0x0;
	decinit.nPadWidth  = 0x0;
	decinit.ulStreamVersion = 0x40006000;
#endif
#if 1
	stream_virtual_address = (u8 *) 0x6000046;
	byteStrmStart =(u8*)stream_virtual_address  ;
	
	decIn.dataLength =0x0104; 
	decIn.numDataSegments = 0x2;
	decIn.timestamp=0x0;
	decIn.flags = 0x2;
	
	slice_info[0].bIsValid = 0x1;
	slice_info[0].ulSegmentOffset = 0x0;
	slice_info[1].bIsValid = 0x1;
	slice_info[1].ulSegmentOffset = 0x07f;
	decIn.pDataSegments = &slice_info[0];
	
	stream_bus_address = (u32) stream_virtual_address ;
	decIn.streamBusAddr =(u32) stream_bus_address ; //stream_bus_address;
	
	msg_id.message_id = 0x1234;
	msg_id.num_sizes = 0x0;
	msg_id.sizes[0]=352;
	msg_id.sizes[1]=240;
	
	decinit.pels  = 352;
	decinit.lines = 240;
	decinit.nPadHeight = 0x0;
	decinit.nPadWidth  = 0x0;
	decinit.ulStreamVersion = 0x40004000;
#endif
#if 0
	//zsf
	stream_virtual_address = (u8 *) 0x6000048;
	byteStrmStart =(u8*)stream_virtual_address  ;
	
	decIn.dataLength =0x08e4; 
	decIn.numDataSegments = 0x2;
	decIn.timestamp=0x0;
	decIn.flags = 0x2;
	
	slice_info[0].bIsValid = 0x1;
	slice_info[0].ulSegmentOffset = 0x0;
	slice_info[1].bIsValid = 0x1;
	slice_info[1].ulSegmentOffset = 0x0459;
	decIn.pDataSegments = &slice_info[0];
	
	stream_bus_address = (u32) stream_virtual_address ;
	decIn.streamBusAddr =(u32) stream_bus_address ; //stream_bus_address;
	
	msg_id.message_id = 0x1234;
	msg_id.num_sizes = 0x0;
	msg_id.sizes[0]=1280;
	msg_id.sizes[1]=720;
	
	decinit.pels  = 1280;
	decinit.lines = 720;
	decinit.nPadHeight = 0x0;
	decinit.nPadWidth  = 0x0;
	decinit.ulStreamVersion = 0x40008000;
#endif
	
	
#endif
	
	infoRet = On2RvDecInit(&decinit,&decoder);
	if(infoRet !=ON2RVDEC_OK)
	{ goto end;}
	
	infoRet = On2RvDecCustomMessage(&msg_id, decoder);
	if(infoRet !=ON2RVDEC_OK)
	{ goto end;}
	
	
	On2RvDecDecode( stream_virtual_address,
						0 , /* unused */
						&decIn,
						&decOut,
						decoder);
	
	
	/* function to obtain last decoded picture out from the decoder */
	//On2RvDecRet On2RvDecPeek(void *pOutputParams, void *decInst);
	
end:
	On2RvDecFree(decoder);
	
	
}

void  hdjpeg_test()
{
	u32 *stream_virtual_address = NULL;
	
	JpegDecInst  decoder;
	JpegDecRet  infoRet;
	JpegDecInput DecIn; 
	JpegDecImageInfo DecImgInf;
	JpegDecOutput DecOut;
	
	DecIn.bufferSize = 0x1800;
	DecIn.streamLength = 0x2278;
	//DecIn.streamLength = 0x17a30a;
	DecIn.streamBuffer.busAddress=0x6000000;
	DecIn.streamBuffer.pVirtualAddress = (u32 * ) 0x6000000;
	DecIn.decImageType = JPEGDEC_IMAGE;
	DecIn.sliceMbSet = 0;
	DecIn.pictureBufferY.busAddress=0;
	DecIn.pictureBufferY.pVirtualAddress = stream_virtual_address;
	DecIn.pictureBufferCbCr.busAddress = 0;
	DecIn.pictureBufferCbCr.pVirtualAddress = stream_virtual_address;
	DecIn.pictureBufferCr.busAddress = 0;
	DecIn.pictureBufferCr.pVirtualAddress = stream_virtual_address;
	
	/* Initialization */
	infoRet = JpegDecInit(&decoder);
	if(infoRet !=JPEGDEC_OK)
	{ goto end;}
	
	
	/* Get image information of the JFIF */
	infoRet = JpegDecGetImageInfo(decoder,
											&DecIn,
											&DecImgInf);
	if(infoRet !=JPEGDEC_OK)
	{ goto end;}
	
	/* Decode JFIF */
	infoRet = JpegDecDecode(decoder,
									&DecIn, &DecOut);
	if(infoRet !=JPEGDEC_OK)
	{ goto end;}
	
end:
	JpegDecRelease( decoder);
}

void hdmpeg2_test(void)
{
	
	u8 *stream_virtual_address = NULL; 
	u32 stream_bus_address = 0; 
	u32 streamLen, bufferLen; 
	u32 picDecodeNumber = 0; /* decoded picture ID */ 
	
	Mpeg2DecInst      decoder; 
	Mpeg2DecRet       ret; 
	Mpeg2DecRet       infoRet; 
	Mpeg2DecInput     decIn; 
	Mpeg2DecOutput    decOut; 
	Mpeg2DecPicture   decPic; 
	Mpeg2DecInfo      decInfo;
	
	stream_bus_address = 0x6000000;
	stream_virtual_address =  (u8 * ) 0x6000000;
	streamLen = 0x100000;
	bufferLen   = 0x100000;
	picDecodeNumber = 0x0;
	
	
	/* The implementation of the following is system dependent */ 
	//allocInputBuffer(&stream_virtual_address, &stream_bus_address, &streamLen); 
	//updateStreamBuffer(&stream_virtual_address, &stream_bus_address, 
	//                  &bufferLen); 
	
	/* Configure the decoder input structure for the first decoding unit */ 
	decIn.dataLen = bufferLen; 
	decIn.pStream = stream_virtual_address; 
	decIn.streamBusAddress = stream_bus_address; 
	
	/* Decoder initialization, output reordering enabled  */ 
	/* check for any initialization errors must be done */ 
	
	infoRet = Mpeg2DecInit(&decoder, 0,picDecodeNumber); 
	
	if(infoRet != MPEG2DEC_OK) 
	{ 
		goto end; 
	} 
	
	do 
	{ 
		/* Before continuing decoding, check availability of display pictures */ 
		while( Mpeg2DecNextPicture(decoder, &decPic, 0) == MPEG2DEC_PIC_RDY ) 
		{ 
			//   displayOnScreen( decPic.pOutputPicture, decPic.frameWidth, 
			//                     decPic.frameHeight); 
		} 
		
		/* Picture ID is the picture number in decoding order */ 
		decIn.picId = picDecodeNumber; 
		
		/* decode the stream data */ 
		ret = Mpeg2DecDecode(decoder,  &decIn,  &decOut); 
		switch(ret) 
		{ 
		case MPEG2DEC_HDRS_RDY: 
			/* read stream info */ 
			infoRet = Mpeg2DecGetInfo(decoder, &decInfo); 
			break; 
			
			/* a picture was decoded */ 
		case MPEG2DEC_PIC_DECODED: 
			
			/* Increment decoding number after every decoded picture */ 
			picDecodeNumber++; 
			break; 
			
		case MPEG2DEC_STRM_PROCESSED: 
		case MPEG2DEC_STRM_ERROR: 
			/* Input stream was processed but no picture is ready */ 
			break; 
			
		default: 
			/* all other cases are errors where decoding cannot continue */ 
			goto end; 
		} 
		
		if(decOut.dataLeft == 0) 
		{ 
			/* new buffer needed */ 
			//    updateStreamBuffer(&stream_virtual_address, &stream_bus_address, 
			//                       &bufferLen); 
		} 
		
		else 
		{ 
			/* data left undecoded */ 
			decIn.dataLen =  decOut.dataLeft; 
			decIn.pStream = decOut.pStrmCurrPos; 
			decIn.streamBusAddress = decOut.strmCurrBusAddress; 
		} 
		
		/* keep decoding until all data from input stream buffer consumed */ 
	} 
	while(decIn.dataLen > 0); 
	
	
	/* stream ended, obtain remaining pictures in display order, if any 
	available */ 
	while (Mpeg2DecNextPicture(decoder, &decPic, 1) == MPEG2DEC_PIC_RDY ) 
	{ 
		//  displayOnScreen(decPic.pOutputPicture, decPic.frameWidth, 
		//                  decPic.frameHeight); 
	} 
	
end: 
	Mpeg2DecRelease(decoder); 
	
}

void  hdmpeg4_test()
{
	
	u8 *stream_virtual_address = NULL; 
	u32 stream_bus_address = 0; 
	u32 streamLen, bufferLen; 
	u32 picDecodeNumber = 0; /* decoded picture ID */ 
	
	MP4DecInst      decoder; 
	MP4DecRet       ret; 
	MP4DecRet       infoRet; 
	MP4DecInput     decIn; 
	MP4DecOutput    decOut; 
	MP4DecPicture   decPic; 
	MP4DecInfo      decInfo; 
	
	
	stream_bus_address = 0x6000000;
	stream_virtual_address =  (u8 * ) 0x6000000;
	bufferLen = 0x100000;
	/* Configure the decoder input structure for the first decoding unit */ 
	
	decIn.dataLen = bufferLen; 
	decIn.pStream = stream_virtual_address; 
	decIn.streamBusAddress = stream_bus_address; 
	
	/* Decoder initialization, output reordering enabled  */ 
	/* check for any initialization errors must be done */ 
	
	infoRet = MP4DecInit(&decoder, 0, 0,0); 
	
	if(infoRet != MP4DEC_OK) 
	{ 
		goto end; 
	} 
	
	do 
	{ 
		/* Before continuing decoding, check availability of display pictures */ 
		while( MP4DecNextPicture(decoder, &decPic, 0) == MP4DEC_PIC_RDY ) 
		{ 
			//   displayOnScreen( decPic.pOutputPicture, decPic.frameWidth, 
			//                   decPic.frameHeight); 
		} 
		
		/* Picture ID is the picture number in decoding order */ 
		decIn.picId = picDecodeNumber; 
		
		/* decode the stream data */ 
		ret = MP4DecDecode(decoder,  &decIn,  &decOut); 
		
		switch(ret) 
		{ 
		case MP4DEC_HDRS_RDY: 
			/* read stream info */ 
			infoRet = MP4DecGetInfo(decoder, &decInfo); 
			break; 
			
		case MP4DEC_DP_HDRS_RDY: 
			/* Data partitioning used in the stream. 
         * The decoder has to reallocate resources */ 
			break; 
			
			/* a picture was decoded */ 
		case MP4DEC_PIC_DECODED: 
			
			/* Increment decoding number after every decoded picture */ 
			picDecodeNumber++; 
			
			break; 
			
		case MP4DEC_STRM_PROCESSED: 
		case MP4DEC_STRM_ERROR: 
			/* Input stream was processed but no picture is ready */ 
			break; 
			
		default: 
			/* all other cases are errors where decoding cannot continue */ 
			goto end; 
		} 
		if(decOut.dataLeft == 0) 
		{ 
			/* new buffer needed */ 
			//updateStreamBuffer(&stream_virtual_address, &stream_bus_address, 
			//                   &bufferLen); 
		} 
		else 
		{ 
			/* data left undecoded */ 
			decIn.dataLen =  decOut.dataLeft; 
			decIn.pStream = decOut.pStrmCurrPos; 
			decIn.streamBusAddress = decOut.strmCurrBusAddress; 
		} 
		
		/* keep decoding until all data from input stream buffer consumed */ 
	} 
	while(decIn.dataLen > 0); 
	
	
	/* stream ended, obtain remaining pictures in display order, if any 
	available */ 
	while ( MP4DecNextPicture(decoder, &decPic, 1) == MP4DEC_PIC_RDY ) 
	{ 
		//displayOnScreen(decPic.pOutputPicture, decPic.frameWidth, 
		//                decPic.frameHeight); 
	} 
	
end: 
	MP4DecRelease(decoder);
	
}

void  hdvc1_test(void )
{
	
	u8 *stream_virtual_address = NULL; 
	u32 stream_bus_address = 0; 
	u32 streamLen, bufferLen; 
	u32 picDecodeNumber = 0; /* decoded picture ID */ 
	
	
	VC1DecInst      decoder; 
	VC1DecRet       ret; 
	VC1DecRet       infoRet; 
	VC1DecInput     decIn; 
	VC1DecOutput    decOut; 
	VC1DecPicture   decPic; 
	VC1DecInfo      decInfo; 
	VC1DecMetaData  pMetaData;
	
	stream_bus_address = 0x6000000;
	stream_virtual_address =  (u8 * ) 0x6000000;
	bufferLen = 0x100000;
	/* Configure the decoder input structure for the first decoding unit */ 
	
	decIn.streamSize = bufferLen; 
	decIn.pStream = stream_virtual_address; 
	decIn.streamBusAddress = stream_bus_address; 
	pMetaData.profile = 8;
	
	
	/* Decoder initialization, output reordering enabled  */ 
	/* check for any initialization errors must be done */ 
	
	infoRet = VC1DecInit(&decoder, &pMetaData, 0,1); 
	
	if(infoRet != VC1DEC_OK) 
	{ 
		goto end; 
	} 
	
	do 
	{ 
		/* Before continuing decoding, check availability of display pictures */ 
		while( VC1DecNextPicture(decoder, &decPic, 0) == VC1DEC_PIC_RDY ) 
		{ 
			//   displayOnScreen( decPic.pOutputPicture, decPic.frameWidth, 
			//                   decPic.frameHeight); 
		} 
		
		/* Picture ID is the picture number in decoding order */ 
		decIn.picId = picDecodeNumber; 
		
		/* decode the stream data */ 
		ret = VC1DecDecode(decoder,  &decIn,  &decOut); 
		
		switch(ret) 
		{ 
		case VC1DEC_HDRS_RDY: 
			/* read stream info */ 
			infoRet = VC1DecGetInfo(decoder, &decInfo); 
			break; 
			
			//  case VC1DEC_DP_HDRS_RDY: 
			/* Data partitioning used in the stream. 
         * The decoder has to reallocate resources */ 
			//    break; 
			
			/* a picture was decoded */ 
		case VC1DEC_PIC_DECODED: 
			
			/* Increment decoding number after every decoded picture */ 
			picDecodeNumber++; 
			
			break; 
			
		case VC1DEC_STRM_PROCESSED: 
		case VC1DEC_STRM_ERROR: 
			/* Input stream was processed but no picture is ready */ 
			break; 
			
		default: 
			/* all other cases are errors where decoding cannot continue */ 
			goto end; 
		} 
		if(decOut.dataLeft == 0) 
		{ 
			/* new buffer needed */ 
			//updateStreamBuffer(&stream_virtual_address, &stream_bus_address, 
			//                   &bufferLen); 
		} 
		else 
		{ 
			/* data left undecoded */ 
			decIn.streamSize =  decOut.dataLeft; 
			decIn.pStream = decOut.pStreamCurrPos; 
			decIn.streamBusAddress = decOut.strmCurrBusAddress; 
		} 
		
		/* keep decoding until all data from input stream buffer consumed */ 
	} 
	while(decIn.streamSize > 0); 
	
	
	/* stream ended, obtain remaining pictures in display order, if any 
	available */ 
	while ( VC1DecNextPicture(decoder, &decPic, 1) == VC1DEC_PIC_RDY ) 
	{ 
		//displayOnScreen(decPic.pOutputPicture, decPic.frameWidth, 
		//                decPic.frameHeight); 
	} 
	
end: 
	VC1DecRelease(decoder);
	
	
}

void hdvp8_test(void)
{
	u8 *stream_virtual_address = NULL; 
	u32 stream_bus_address = 0; 
	u32 streamLen, bufferLen; 
	u32 picDecodeNumber = 0; 
	
	VP8DecInst      decoder; 
	VP8DecRet       ret; 
	VP8DecRet       infoRet; 
	VP8DecInput     decIn; 
	VP8DecOutput    decOut; 
	VP8DecPicture   decPic; 
	VP8DecInfo      decInfo; 
	VP8DecFormat    decFormat; 
	
	/* The implementation of the following is system dependent */ 
	//allocInputBuffer(&stream_virtual_address, &stream_bus_address, &streamLen); 
	//updateStreamBuffer(&stream_virtual_address, &stream_bus_address, 
	//                   &bufferLen); 
	
	
	stream_bus_address = 0x6000030;
	stream_virtual_address =  (u8 * ) 0x6000030;
	//tream_bus_address = 0x60e18cb;
	//stream_virtual_address =  (u8 * ) 0x60e18cb;
	//stream_bus_address = 0x61918D0;
	//stream_virtual_address =  (u8 * ) 0x61918D0;
	
	bufferLen = 0x100000;
	
	/* Configure the decoder input structure for the first decoding unit */ 
	decIn.dataLen = bufferLen; 
	decIn.pStream = stream_virtual_address; 
	decIn.streamBusAddress = stream_bus_address; 
	//decIn.sliceHeight = sliceHeight; 
	decFormat = VP8DEC_VP8;
	//decFormat = VP8DEC_WEBP;
	
	/* Configure the decoder for WebP decoding */ 
	//decFormat = VP8DEC_WEBP; 
	/* Decoder initialization, */ 
	/* check for any initialization errors must be done */ 
	infoRet = VP8DecInit(&decoder, decFormat, 
								0, 0, DEC_REF_FRM_RASTER_SCAN); 
	
	if(infoRet != VP8DEC_OK) 
	{ 
		goto end; 
	} 
	
	do 
	{ 
		
		/* Before continuing decoding, check availability of display pictures */ 
		while( VP8DecNextPicture(decoder, &decPic, 0) == VP8DEC_PIC_RDY ) 
		{ 
			//   displayOnScreen( decPic.pOutputFrame, decPic.frameWidth, 
			//                    decPic.frameHeight); 
		} 
		
		/* Picture ID is the picture number in decoding order */ 
		//decIn.picId = 0; 
		
		/* decode the stream data */ 
		ret = VP8DecDecode(decoder,  &decIn,  &decOut); 
		
		switch(ret) 
		{ 
		case VP8DEC_HDRS_RDY: 
			/* read stream info */ 
			infoRet = VP8DecGetInfo(decoder, &decInfo); 
			break; 
			
			/* a picture was decoded */ 
		case VP8DEC_PIC_DECODED: 
			
			/* Increment decoding number after every decoded picture */ 
			picDecodeNumber++; 
			break; 
			
		case VP8DEC_STRM_ERROR: 
			/* Input stream was processed but no picture is ready */ 
			break; 
			
		default: 
			/* all other cases are errors where decoding cannot continue */ 
			goto end; 
		} 
		
	} while(decIn.dataLen > 0); 
	/* stream ended, obtain remaining pictures in display order, if any 
	available */ 
	while (VP8DecNextPicture(decoder, &decPic, 1) == VP8DEC_PIC_RDY ) 
	{ 
		//  displayOnScreen(decPic.pOutputFrame, decPic.frameWidth, 
		//                   decPic.frameHeight); 
	} 
	
end: 
	VP8DecRelease(decoder); 
	
} 

void hdpp_test(void)
{
	u32 input_bus_address = 0; 
	u32 pp_out_bus_address = 0; 
	
	PPInst pp = NULL; 
	PPResult ppRet; 
	PPConfig pPpConf; 
	
	/* these function implementations are system dependent, not included in this 
	example */ 
	//AllocInputBuffer(&input_bus_address); 
	//AllocOutputBuffer(&pp_out_bus_address); 
	input_bus_address   = 0x6000000;
	pp_out_bus_address = 0x6200000;
	/* Init PP to work in stand-alone mode */ 
	ppRet = PPInit(&pp); 
	
	if(ppRet != PP_OK){ 
		/* Handle errors here */ 
		goto end; 
	} 
	
	/* First get the default PP settings */ 
	ppRet = PPGetConfig(pp, &pPpConf); 
	if(ppRet != PP_OK){ 
		/* Handle errors here */ 
		goto end; 
	} 
	/* setup PP */ 
	
	/* Set input width to VGA */ 
	pPpConf.ppInImg.width = 240; 
	
	/* Set input height to VGA */ 
	pPpConf.ppInImg.height = 320; 
	/* Set video range to 0 */ 
	pPpConf.ppInImg.videoRange = 0; 
	
	/* Set the luminance input picture base address */ 
	pPpConf.ppInImg.bufferBusAddr = input_bus_address; 
	
	/* Set the chrominance base address offset in a linear YCbCr 4:2:0 buffer. 
	Chrominance plane will be located directly after the luminance plane */ 
	pPpConf.ppInImg.bufferCbBusAddr = 
		input_bus_address + pPpConf.ppInImg.width * pPpConf.ppInImg.height; 
	
	/* Set the input format to be YCbCr 4:2:0 Semi-planar */ 
	//pPpConf.ppInImg.pixFormat = PP_PIX_FMT_YCBCR_4_2_2_SEMIPLANAR; 
	pPpConf.ppInImg.pixFormat =  PP_PIX_FMT_CBYCRY_4_2_2_INTERLEAVED;
	/* Set output width to NTSC */ 
	pPpConf.ppOutImg.width = 720; 
	
	/* Set input height to NTSC */ 
	pPpConf.ppOutImg.height = 480; 
	
	/* Set output picture base address */ 
	pPpConf.ppOutImg.bufferBusAddr = pp_out_bus_address; 
	
	/* Set output picture to RGB 32-bit format */ 
	pPpConf.ppOutImg.pixFormat = PP_PIX_FMT_RGB32; 
	
	/* Now use the PP API to write in the new setup */ 
	
	
	ppRet = PPSetConfig(pp, &pPpConf); 
	if(ppRet != PP_OK){ 
		/* Handle errors here */ 
		goto end; 
	} 
	
	/* Loop for video */ 
	do{ 
		
		/* * * * * * * * * * * * * * * * * * * * * 
		Update new frame for the input buffer here 
		* * * * * * * * * * * * * * * * * * * * */ 
		
		ppRet = PPGetResult(pp); 
		if(ppRet != PP_OK){ 
			/* handle errors here */ 
			goto end; 
		} 
		
		/* The PP output is ready now */ 
		//   displayOnScreen( &pp_virtual_address ); 
		
		/* Here it is possible to update the PP setup for the next picture.  
		This is optional, as the earlier set configuration will not be reset between 
		pictures */ 
		
		/* Read the current configuration */ 
		ppRet = PPGetConfig(pp, &pPpConf); 
		if(ppRet != PP_OK){ 
			/* Handle errors here */ 
			goto end; 
		} 
		
		/* * * * * * * * * * * * * * * * * * * * * * 
		Write new values for the PP parameters here 
		* * * * * * * * * * * * * * * * * * * * * * */ 
		
		/* Write new configuration if necessary */ 
		ppRet = PPSetConfig(pp, &pPpConf); 
		if(ppRet != PP_OK){ 
			/* Handle errors here */ 
			goto end; 
		} 
	} 
	while( 1 ); 
	
end: 
	
	/* Release the post-processor instance */ 
	PPRelease(pp); 
	
	
   
}

void hd_test()
{
	//hd264_test();
	// hdrv_test();
	// hdjpeg_test();
	// hdmpeg2_test();
   hdmpeg4_test();
	// hdvc1_test();
	//   hdvp8_test();
	//  hdpp_test();
}

