//****************************************************************************
//
//	Copyright (C) 2012~2016 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_uvc_video.h
//	  usb uvc device driver
//
//	Revision history
//
//		2012.01.13	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_UVC_VIDEO_H_
#define _XM_UVC_VIDEO_H_

#include <xm_type.h>

#if defined (__cplusplus)
	extern "C"{
#endif

/* Values for bmHeaderInfo (Video and Still Image Payload Headers, 2.4.3.3) */
#define UVC_STREAM_EOH	(1 << 7)
#define UVC_STREAM_ERR	(1 << 6)
#define UVC_STREAM_STI	(1 << 5)
#define UVC_STREAM_RES	(1 << 4)
#define UVC_STREAM_SCR	(1 << 3)
#define UVC_STREAM_PTS	(1 << 2)
#define UVC_STREAM_EOF	(1 << 1)
#define UVC_STREAM_FID	(1 << 0)
		
struct UVCVIDEOSAMPLE {		
	DWORD		dwPayloadDataSize;		// Palyload Data Size (Payload Header + Payload Data)
	BYTE		bHeadLength;		// hearder length of packet header
	BYTE *		lpVideoSample;		// 待传Video Sample
	DWORD		cbVideoSample;
	BYTE *		lpPayloadData;		// 待传Payload缓冲区指针
	DWORD		cbPayloadData;		// 待传Payload缓冲区字节长度
	DWORD		cbPayloadSize;		// Payload字节长度
	
	volatile UINT		dma_timeout;		// DMA超时
	volatile BYTE		dma_transfer_start;
	
	BYTE		FID;					// This bit toggles at each frame start boundary and
										//		stays constant for the rest of the frame.
	
	volatile int		index;
};

struct UVCVIDEOSAMPLE * UVCVideoCreate (void);

// stop_or_break
//	1 --> STOP
// 2 --> BREAK
void UVCVideoDelete (struct UVCVIDEOSAMPLE *lpVideoSample, int stop_or_break);

void UVCVideoSetupPayload (BYTE *lpPayload, DWORD cbPayload);
int UVCVideoTransferPayload (void);

extern void UVCVideoTransfer (void);

int UVC_VideoTransferPayload (struct UVCVIDEOSAMPLE *VideoSample);

// 准备并开始UVC Sample传输
void UVC_VideoSetupVideoSample (BYTE *lpVideoSample, DWORD cbVideoSample);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */
		
#endif	// _XM_UVC_VIDEO_H_

