#include <xm_proj_define.h>
#include "common.h"

volatile BYTE xdata bDebugFlg	; 	//bDebugFlg的位定义参考reg_module_enable


BYTE xdata gbError;
BYTE xdata gbBuffer[64];

WORD xdata gwVID = 0x18EC;			
WORD xdata gwPID = 0x1189;			


//usb
DeviceStatus xdata gsDeviceStatus;		
UsbRequest xdata gsUsbRequest;	 
Ep0Status xdata gsEp0Status;

//descript_def
BYTE xdata bDualFormatOrder = 1;	//1-yuv first; 0-jpeg first

//video
WORD xdata bmAttrAdjust;
VideoProbeCtrl xdata gsVideoProbeCtrl;
StillProbeCtrl xdata gsStillProbeCtrl;
VideoStatus xdata gsVideoStatus;

VideoLimit xdata gsVideoLimit;
VideoDefault xdata gsVideoDefault;

//audio
AudioStatus xdata gsAudioStatus;
AudioLimit xdata gsAudioLimit;

#if _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_ISO_
WORD xdata USB20_MAX_VIDEO_ISO_PACKET_SIZE = B2L_16(0xe813);
DWORD xdata USB20_MAX_VIDEO_PAYLOAD_SIZE = B2L_32(0xB80B0000);

// Maximum Packet Size of ISO Endpoint
BYTE xdata USB20_INMAX_L_VIDEO_VAL = 0xe8;
BYTE xdata USB20_INMAX_H_VIDEO_VAL = 0x13;

BYTE xdata USB20_VIDEO_DEPTH_L_VAL = 0xb8;
BYTE xdata USB20_VIDEO_DEPTH_H_VAL = 0x0b;

#elif _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_BULK_

WORD xdata USB20_MAX_VIDEO_ISO_PACKET_SIZE = B2L_16(0x0004);
DWORD xdata USB20_MAX_VIDEO_PAYLOAD_SIZE = B2L_32(0x00000200);

// Maximum Packet Size of Bulk Endpoint
BYTE xdata USB20_INMAX_L_VIDEO_VAL = 0x00;	// 512
BYTE xdata USB20_INMAX_H_VIDEO_VAL = 0x02;

// Payload size定义
#if _VIDEO_EP_ == _VIDEO_EP_4_
BYTE xdata USB20_VIDEO_DEPTH_L_VAL = 0x00;	// 512
BYTE xdata USB20_VIDEO_DEPTH_H_VAL = 0x02;
#elif _VIDEO_EP_ == _VIDEO_EP_5_
BYTE xdata USB20_VIDEO_DEPTH_L_VAL = 0x00;	// 1024
BYTE xdata USB20_VIDEO_DEPTH_H_VAL = 0x04;
#endif

#endif


WORD xdata USB11_MAX_VIDEO_ISO_PACKET_SIZE = B2L_16(0xe803);
BYTE idata USB11_VIDEO_DEPTH_L_VAL = 0xe8;
BYTE idata USB11_VIDEO_DEPTH_H_VAL = 0x03;
BYTE xdata USB11_INMAX_L_VIDEO_VAL = 0xe8;
BYTE xdata USB11_INMAX_H_VIDEO_VAL = 0x03;
DWORD xdata USB11_MAX_VIDEO_PAYLOAD_SIZE = B2L_32(0xe8030000);

BYTE xdata AUDIO_SIZE_1_FRAME = 0x60;
BYTE xdata USB_INMAX_L_AUDIO_VAL = 0x60;
BYTE xdata USB_INMAX_H_AUDIO_VAL = 0x00;

WORD xdata MAX_AUDIO_PAYLOAD_SIZE = B2L_16(0x6000);
WORD xdata AUDIO_INTERVAL_HS = B2L_16(0x0400);
WORD xdata AUDIO_INTERVAL_FS = B2L_16(0x0400);

//音量控制
#if MAKE_FOR_ASIC_OR_FPGA	//asic
WORD xdata MAX_MIC_VOLUME = 0x4fff;
code MIC_VOL_MAP_TBL mic_vol_mapTbl[AUDIO_PARA_NUM] =
{
	//pc    //reg
	0x0000, 0x00,		//0.0dB
	0x0180, 0x01,		//1.5dB
	0x0300, 0x02,		//3.0dB
	0x0480, 0x03,		//4.5dB
	0x0600, 0x04,		//6.0dB
	0x0780, 0x05,		//7.5dB
	0x0900, 0x06,		//9.0dB
	0x0a80, 0x07,		//10.5dB
	0x0c00, 0x08,		//12.0dB
	0x0d80, 0x09,		//13.5dB
	0x0f00, 0x0a,		//15.0dB
	0x1080, 0x0b,		//16.5dB
	0x1200, 0x0c,		//18.0dB
	0x1380, 0x0d,		//19.5dB
	0x1500, 0x0e,		//21.0dB
	0x1680, 0x0f,		//22.5dB
	0x1800, 0x10,		//24.0dB
	0x1980, 0x11,		//25.5dB
	0x1b00, 0x12,		//27.0dB
	0x1c80, 0x13,		//28.5dB
	0x1e00, 0x14,		//30.0dB
	0x1f80, 0x15,		//31.5dB
	0x2100, 0x16,		//33.0dB
	0x2280, 0x17,		//34.5dB
	0x2400, 0x18,		//36.0dB
	0x2580, 0x19,		//37.5dB
	0x2700, 0x1a,		//39.0dB
	0x2880, 0x1b,		//40.5dB
	0x2a00, 0x1c,		//42.0dB
	0x2b80, 0x1d,		//43.5dB
	0x2d00, 0x1e,		//45.0dB
	0x2e80, 0x1f,		//46.5d	

//正式3399需添加
	0x3000, 0x74,		//48.0dB
	0x3180, 0x75,		//49.5dB
	0x3300, 0x76,		//51.0dB
	0x3480, 0x77,		//52.5dB
	0x3600, 0x78,		//54.0dB
	0x3780, 0x79,		//55.5dB
	0x3900, 0x7a,		//57.0dB
	0x3A80, 0x7b,		//58.5dB
	0x3C00, 0x7c,		//60.0dB
	0x3D80, 0x7d,		//61.5dB	//实验很容易溢出，且噪声很大
	0x3F00, 0x7e,		//63.0dB	//实验很容易溢出，且噪声很大
	0x4080, 0x7f,		//64.5dB	//实验很容易溢出，且噪声很大
};

#if MASK_VER_2
BYTE xdata min_vol = 0x14;
BYTE xdata default_vol = 0x1d;
#else
BYTE xdata default_vol = 0x14;
#endif



#else	//fpga
//fpga, 0x00 - div1; 0x01 - div2; 0x02 - div4; 0x03 - div8

WORD xdata MAX_MIC_VOLUME = 0x2fff;
code MIC_VOL_MAP_TBL mic_vol_mapTbl[AUDIO_PARA_NUM] =
{
	//pc    //reg
	0x0000, 0x00,		//0.125
	0x0c00, 0x01,		//0.25
	0x1800, 0x02,		//0.5
	0x2400, 0x03,		//1
};

BYTE xdata default_vol = 0x03;
#endif

