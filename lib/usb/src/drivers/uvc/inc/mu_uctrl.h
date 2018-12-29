/******************************************************************
*                                                                *
*        Copyright Mentor Graphics Corporation 2005              *
*                                                                *
*                All Rights Reserved.                            *
*                                                                *
*    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION *
*  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS   *
*  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                    *
*                                                                *
******************************************************************/

/*
 * MUSBStack-S UVC Protocol/Command-Set Interface
 * $Revision: 1.2 $
 */

#ifndef __MUSB_UVC_CTRL_H__
#define __MUSB_UVC_CTRL_H__

#include "mu_cdi.h"
#include "mu_uapi.h"

#define ISO_WIDTH					800
#define ISO_HEIGH					480
#define ISO_FRAME_SIZE				ISO_WIDTH*ISO_HEIGH*2
#define PACKET_LEN					3072
#define PACKET_COUNT_PRE_FRAME		(ISO_FRAME_SIZE/PACKET_LEN+10)		//¶àÔ¤Áô10  ¸ö·ÀÖ¹Òç³ö


#define REGISTER_USER_VIDEO_FUNC			0x00
#define REGISTER_USER_AUDIO_FUNC		0x01
#define REGISTER_USER_VIDEO_ARG			0x02
#define REGISTER_USER_AUDIO_ARG			0x03
#define REQUEST_GETPROBE_CONTROL		0x04
#define REQUEST_SETPROBE_CONTROL		0x05
#define REQUEST_GETCOMMIT_CONTROL		0x06
#define REQUEST_SETCOMMIT_CONTROL		0x07
#define REQUEST_OPEN_UVC_VIDEO			0x08
#define REQUEST_STOP_UVC_VIDEO			0x09
#define REQUEST_OPEN_UVC_AUDIO			0x0A
#define REQUEST_STOP_UVC_AUDIO			0x0B
#define REQUEST_UVC_VIDEO_RECEIVE		0x0C
#define REQUEST_UVC_AUDIO_RECEIVE		0x0D
#define REQUEST_UVC_RECEIVE_FINISH		0x0E
#define REQUEST_UVC_VIDEO_DISCONNECT	0x0F
#define REQUEST_GETBACKLIGHT_CONTROL	0x10


#define UVC_STREAM_EOH	(1 << 7)
#define UVC_STREAM_ERR	(1 << 6)
#define UVC_STREAM_STI	(1 << 5)
#define UVC_STREAM_RES	(1 << 4)
#define UVC_STREAM_SCR	(1 << 3)
#define UVC_STREAM_PTS	(1 << 2)
#define UVC_STREAM_EOF	(1 << 1)
#define UVC_STREAM_FID	(1 << 0)

//uvc callback data type
#define  UVC_VIDEO_FRAME_EVENT		0x00
#define  UVC_AUDIO_FRAME_EVENT		0x01
#define  UVC_FRAME_ERROR_EVENT		0x02
#define  UVC_VIDEO_GET_FREE_BUF_EVENT	0x03
#define  UVC_VIDEO_RECAPTURE_EVENT		0x04
#define  UVC_VIDEO_NEW_CAPTURE_EVENT	0x05
#define  UVC_VIDEO_BACKLIGHT_EVENT		0x06
#define  UVC_VIDEO_RESET_EVENT			0x07


#define UVC_BACKLIGHT_MODE
enum {
	UVC_OK = 0x0,
	UVC_NO_DEVICE = 0xFE,
	UVC_UNDEF = 0xFFFFFFFF,
};



typedef struct _MGC_UVC_PROBE_PARAM_ {
  uint16_t		bmHint;
  uint8_t		bFormatIndex;
  uint8_t		bFrameIndex;
  uint32_t		dwFrameInterval;
  uint16_t		wKeyFrameRate;
  uint16_t		wPFrameRate;
  uint16_t		wCompQuality;
  uint16_t		wCompWindowSize;
  uint16_t		wDelay;
  uint32_t		dwMaxVideoFrameSize;
  uint32_t		dwMaxPayloadTransferSize;
}MGC_UVC_PROBE_PARAM;

uint32_t MUSB_UVC_Control(uint16_t Command, void *Arg);
#endif	/* multiple inclusion protection */

