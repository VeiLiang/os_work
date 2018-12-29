#ifndef _COMMON_H
#define _COMMON_H

#include <xm_type.h>

#define YUV422

#define xdata	
#define idata
#define code
typedef unsigned char bit;
extern void disable_usb_int(void);
extern void enable_usb_int(void);

extern unsigned char reg_brightness_r(void);
extern unsigned char reg_contrast_r (void);
extern unsigned char reg_hue_r (void);
extern unsigned char reg_saturation_r (void);
extern void reg_brightness_w(unsigned char);
extern void reg_contrast_w(unsigned char);
extern void reg_hue_w(unsigned char);
extern void reg_saturation_w(unsigned char);

#define MAKE_FOR_ASIC_OR_FPGA	1	// 0 - for fpga; 1 - for asic


#define FUNC_NO_USE			1

#define MASK_VER_2				1

#define LED_DEBUG		0

#if MAKE_FOR_ASIC_OR_FPGA
#define AUDIO_PARA_NUM		44
#else
#define AUDIO_PARA_NUM		4
#endif

#define TRUE		1
#define FALSE	0

#ifndef NULL
#define NULL		0
#endif

typedef unsigned char BOOL;
//typedef unsigned char BYTE;
//typedef unsigned short WORD;
//typedef unsigned long DWORD;

#define bmBIT0	0x01
#define bmBIT1	0x02
#define bmBIT2	0x04
#define bmBIT3	0x08
#define bmBIT4	0x10
#define bmBIT5	0x20
#define bmBIT6	0x40
#define bmBIT7	0x80

#define LSB(wValueA)	((BYTE)(wValueA))
#define MSB(wValueA)	((BYTE)(wValueA >> 8))

#define I2C_100K 0x00
#define I2C_400K 0x01

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

typedef struct _MIC_VOL_MAP_TBL
{
	WORD dbVal;
	BYTE regVal;
}MIC_VOL_MAP_TBL;

/******************* Device Status *******************/
#define DEVSTATE_DEFAULT	0x00
#define DEVSTATE_ADDRESS 	0x01
#define DEVSTATE_CONFIG		0x02

#define DEFLICK_MODE_OUTDOOR	0x00
#define DEFLICK_MODE_50Hz		0x01
#define DEFLICK_MODE_60Hz		0x02

typedef struct _DeviceStatus
{
	//for usb
	BOOL bUsb_HighSpeed;
	BYTE bUsb_DevState;		//DEVSTATE_DEFAULT
	
	//BYTE bKeySel;				// 1 - GPIO2 for effort & capture; 0 - GPIO2 for capture
	BYTE bDeflickMode;		//DEFLICK_MODE_50Hz
	BYTE bUsbHsOrFs;			// 1 - USB2.0; 0 - USB1.0
	BYTE bMicSel;				// 1 - with MIC; 0 - no MIC
	BYTE bYuvOrRgb;			// 1 - yuv; 0 - rgb
	BYTE bFormatSel;			// 1 - YUV; 0 - JPEG
	BYTE bDualFormatSel;		// 1 - 1 format(YUV or JPEG); 0 - 2 formats(YUV + JPEG)
	BYTE bDualSensorSel;		// 1 - 1 sensor; 0 - 2 sensors;	
} DeviceStatus;

extern DeviceStatus xdata gsDeviceStatus;

/******************* EP0 *******************/
// Endpoint 0 states
#define EP0_IDLE	0x00
#define EP0_RX	0x01
#define EP0_TX	0x02
#define EP0_CMD	0x03

#define EP0_MAXP 64

//usb2.0
//#define USB20_MAX_VIDEO_ISO_PACKET_SIZE			0x0014
//#define USB20_MAX_VIDEO_PAYLOAD_SIZE			0x000C0000
//#define USB20_INMAX_L_VIDEO_VAL					0x00
//#define USB20_INMAX_H_VIDEO_VAL					0x14
//#define USB20_VIDEO_DEPTH_L_VAL					0x00
//#define USB20_VIDEO_DEPTH_H_VAL					0x0C


//#define USB20_MAX_VIDEO_ISO_PACKET_SIZE			0xE813
//#define USB20_MAX_VIDEO_PAYLOAD_SIZE			0xB80B0000
//#define USB20_INMAX_L_VIDEO_VAL					0xE8
//#define USB20_INMAX_H_VIDEO_VAL					0x13
//#define USB20_VIDEO_DEPTH_L_VAL					0xB8
//#define USB20_VIDEO_DEPTH_H_VAL					0x0B

extern WORD xdata USB20_MAX_VIDEO_ISO_PACKET_SIZE;
extern DWORD xdata USB20_MAX_VIDEO_PAYLOAD_SIZE;
extern BYTE xdata USB20_INMAX_L_VIDEO_VAL;
extern BYTE xdata USB20_INMAX_H_VIDEO_VAL;
extern BYTE xdata USB20_VIDEO_DEPTH_L_VAL;
extern BYTE xdata USB20_VIDEO_DEPTH_H_VAL;


//#define USB20_MAX_VIDEO_ISO_PACKET_SIZE			0x8413
//#define USB20_MAX_VIDEO_PAYLOAD_SIZE			0x8C0A0000
//#define USB20_INMAX_L_VIDEO_VAL					0x84
//#define USB20_INMAX_H_VIDEO_VAL					0x13
//#define USB20_VIDEO_DEPTH_L_VAL					0x8C
//#define USB20_VIDEO_DEPTH_H_VAL					0x0A

//#define USB20_MAX_VIDEO_ISO_PACKET_SIZE			0x4013
//#define USB20_MAX_VIDEO_PAYLOAD_SIZE			0xc0090000
//#define USB20_INMAX_L_VIDEO_VAL					0x40
//#define USB20_INMAX_H_VIDEO_VAL					0x13
//#define USB20_VIDEO_DEPTH_L_VAL					0xc0
//#define USB20_VIDEO_DEPTH_H_VAL					0x09

//usb1.1
//此参数带不带HUB都不能录音，带宽不够，只能做无MIC操作
//#define USB11_MAX_VIDEO_ISO_PACKET_SIZE			0xFF03
//#define USB11_MAX_VIDEO_PAYLOAD_SIZE			0xFF030000
//#define USB11_INMAX_L_VIDEO_VAL					0xFF
//#define USB11_INMAX_H_VIDEO_VAL					0x03
//#define USB11_VIDEO_DEPTH_L_VAL					0xFF
//#define USB11_VIDEO_DEPTH_H_VAL					0x03

//此参数带HUB不能录音，带宽不够（但不带HUB可以）
//#define USB11_MAX_VIDEO_ISO_PACKET_SIZE			0xe803
//#define USB11_MAX_VIDEO_PAYLOAD_SIZE			0xe8030000
//#define USB11_INMAX_L_VIDEO_VAL					0xe8
//#define USB11_INMAX_H_VIDEO_VAL					0x03
//#define USB11_VIDEO_DEPTH_L_VAL					0xe8
//#define USB11_VIDEO_DEPTH_H_VAL					0x03

//此参数带HUB不能录音，带宽不够（但不带HUB可以）
//#define USB11_MAX_VIDEO_ISO_PACKET_SIZE			0xa803
//#define USB11_MAX_VIDEO_PAYLOAD_SIZE			0xa8030000
//#define USB11_INMAX_L_VIDEO_VAL					0xa8
//#define USB11_INMAX_H_VIDEO_VAL					0x03
//#define USB11_VIDEO_DEPTH_L_VAL					0xa8
//#define USB11_VIDEO_DEPTH_H_VAL					0x03

//#define USB11_MAX_VIDEO_ISO_PACKET_SIZE			0xe803
//#define USB11_MAX_VIDEO_PAYLOAD_SIZE			0xe8030000
//#define USB11_INMAX_L_VIDEO_VAL					0xe8
//#define USB11_INMAX_H_VIDEO_VAL					0x03
//#define USB11_VIDEO_DEPTH_L_VAL				    0xe8
//#define USB11_VIDEO_DEPTH_H_VAL					0x03


//此参数带不带HUB都可以录音，带宽足够
//#define USB11_MAX_VIDEO_ISO_PACKET_SIZE			0xa003
//#define USB11_MAX_VIDEO_PAYLOAD_SIZE			0xa0030000
//#define USB11_INMAX_L_VIDEO_VAL					0xa0		//改为全局变量以外挂可调
//#define USB11_INMAX_H_VIDEO_VAL					0x03		//改为全局变量以外挂可调
//#define USB11_VIDEO_DEPTH_L_VAL					0xa0		//改为全局变量以外挂可调
//#define USB11_VIDEO_DEPTH_H_VAL					0x03		//改为全局变量以外挂可调

//#define USB11_MAX_VIDEO_ISO_PACKET_SIZE			0x8003
//#define USB11_MAX_VIDEO_PAYLOAD_SIZE			0x80030000
//#define USB11_INMAX_L_VIDEO_VAL					0x80
//#define USB11_INMAX_H_VIDEO_VAL					0x03
//#define USB11_VIDEO_DEPTH_L_VAL					0x80
//#define USB11_VIDEO_DEPTH_H_VAL					0x03

//#define USB11_MAX_VIDEO_ISO_PACKET_SIZE			0x6003
//#define USB11_MAX_VIDEO_PAYLOAD_SIZE			0x60030000
//#define USB11_INMAX_L_VIDEO_VAL					0x60
//#define USB11_INMAX_H_VIDEO_VAL					0x03
//#define USB11_VIDEO_DEPTH_L_VAL					0x60
//#define USB11_VIDEO_DEPTH_H_VAL					0x03

//此参数带不带HUB都可以录音，带宽足够
//#define USB11_MAX_VIDEO_ISO_PACKET_SIZE			0x4803
//#define USB11_MAX_VIDEO_PAYLOAD_SIZE			0x48030000
//#define USB11_INMAX_L_VIDEO_VAL					0x48
//#define USB11_INMAX_H_VIDEO_VAL					0x03
//#define USB11_VIDEO_DEPTH_L_VAL					0x48
//#define USB11_VIDEO_DEPTH_H_VAL					0x03

//#define USB11_MAX_VIDEO_ISO_PACKET_SIZE			0x2003
//#define USB11_MAX_VIDEO_PAYLOAD_SIZE			0x20030000
//#define USB11_INMAX_L_VIDEO_VAL					0x20
//#define USB11_INMAX_H_VIDEO_VAL					0x03
//#define USB11_VIDEO_DEPTH_L_VAL					0x20
//#define USB11_VIDEO_DEPTH_H_VAL					0x03

//#define USB11_MAX_VIDEO_ISO_PACKET_SIZE			0x0003
//#define USB11_MAX_VIDEO_PAYLOAD_SIZE			0x00030000
//#define USB11_INMAX_L_VIDEO_VAL					0x00
//#define USB11_INMAX_H_VIDEO_VAL					0x03
//#define USB11_VIDEO_DEPTH_L_VAL					0x00
//#define USB11_VIDEO_DEPTH_H_VAL					0x03

extern WORD xdata USB11_MAX_VIDEO_ISO_PACKET_SIZE;
extern BYTE idata USB11_VIDEO_DEPTH_L_VAL;
extern BYTE idata USB11_VIDEO_DEPTH_H_VAL;
extern BYTE xdata USB11_INMAX_L_VIDEO_VAL;
extern BYTE xdata USB11_INMAX_H_VIDEO_VAL;
extern DWORD xdata USB11_MAX_VIDEO_PAYLOAD_SIZE;

extern BYTE xdata AUDIO_SIZE_1_FRAME;
extern BYTE xdata USB_INMAX_L_AUDIO_VAL;
extern BYTE xdata USB_INMAX_H_AUDIO_VAL;
extern WORD xdata MAX_MIC_VOLUME;
extern WORD xdata MAX_AUDIO_PAYLOAD_SIZE;
extern WORD xdata AUDIO_INTERVAL_HS;
extern WORD xdata AUDIO_INTERVAL_FS;

typedef struct _Ep0Status {
	BYTE bState; 		/* IDLE/RX/TX */

	BYTE bTxLock;
//	BYTE bRxLock;
	
	WORD wBytesLeft; 	/* Number of bytes left to send in TX mode */
	BYTE *pbData;	/* Pointer to data to transmit/receive */
//	WORD wBytesRecv; 	// Number of bytes received in RX mode
	
	BYTE bTestMode; 		/* Selected Test Mode */
	BYTE bFAddr; 		/* New function address */
} Ep0Status;

extern Ep0Status xdata gsEp0Status;

#define	B2L_32(i)	( (((i)&0xFF)<<24) | (((i)&0x0000FF00)<<8) | (((i)&0x00FF0000)>>8) | (((i)&0xFF000000)>>24) )
#define	B2L_16(i)	( (((i)&0xFF00)>>8) | (((i)&0x00FF)<<8) )

/******************* USB Request *******************/
/*
0. 由 bmRequestType 区分 usb, class, vendor
1. 由 bRequest 区分set_cur, get_cur, get_min 等
2. 由 LSB(wIndex) 区分 vc,vs, ac, as
3. 由 MSB(wIndex) 区分 各unit
4. 由 MSB(wValue) 区分 各control
*/
/***************************************************/
#pragma pack(1)

typedef struct _UsbRequest
{
	BYTE bmRequestType;
	BYTE bRequest;
	WORD wValue;
	WORD wIndex;
	WORD wLength;	
} UsbRequest;

#pragma pack()

extern UsbRequest xdata gsUsbRequest;

//共26  Bytes	???
#pragma pack(1)

typedef struct _VideoProbeCtrl
{
	WORD  bmHint;
	BYTE  bFormatIndex;
	BYTE  bFrameIndex;
	DWORD dwFrameInterval;
	WORD  wKeyFrameRate;
	WORD  wPFrameRate;
	WORD  wCompQuality;
	WORD  wCompWindowSize;
	WORD  wDelay;
	DWORD dwMaxVideoFrameSize;
	DWORD dwMaxPayloadTransferSize;
} VideoProbeCtrl;
#pragma pack()

extern VideoProbeCtrl xdata gsVideoProbeCtrl;

//共11  Bytes
#pragma pack(1)

typedef struct _StillProbeCtrl
{
	BYTE  bFormatIndex;	 				//video format index from a format descriptor.
	BYTE  bFrameIndex;					//video frame index from a frame descriptor.
	BYTE  bCompressionIndex;			//compression index from a frame descriptor.
	DWORD dwMaxVideoFrameSize;
	DWORD dwMaxPayloadTransferSize;	
} StillProbeCtrl;
#pragma pack()


extern StillProbeCtrl xdata gsStillProbeCtrl;

//for VideoStatus.bmAdjust
#define Adj_AEMode				0x0001
#define Adj_ExposureTime		0x0002
#define Adj_Roll				0x0004
#define Adj_BacklightComp		0x0008
#define Adj_Brightness			0x0010
#define Adj_Contrast			0x0020
#define Adj_Gain				0x0040
#define Adj_PowerLineFreq		0x0080
#define Adj_Hue					0x0100
#define Adj_Saturation			0x0200
#define Adj_Sharpness			0x0400
#define Adj_Gamma				0x0800
#define Adj_WBC					0x1000

#define AE_MODE_AUTO				0x02
#define AE_MODE_SHUTTER_PRIORITY	0x04

typedef struct _VideoStatus
{
	BOOL bStartVideo;
	
	BOOL bCaptureHeadFig;
	BOOL bCapture;
//	BOOL bCaptureFig;		//按键拍照标记

	BOOL bEffectFig;		//特效按键标记
	BYTE bEffectIdx;		//特效值
	BYTE bEffectCount;		//特效计数
	
	WORD bmAdjust;
	//BYTE bVendorMode;

	//attribute
	BYTE bAEMode;			//AE_MODE_AUTO
	WORD wExposureTime;
	BYTE bRoll;
	BYTE bBacklightComp;		//在gamma2curve[]中选择一组gamma
	BYTE bBrightness;
	BYTE bContrast;
	WORD wGain;
	BYTE bPowerLineFreq;
	BYTE bFlickerFlag;
	WORD wHue;
	BYTE bSaturation;
	BYTE bSharpness;
	BYTE bGamma;	
	WORD wWBC;
	BOOL bWBAuto;
} VideoStatus;

extern VideoStatus xdata gsVideoStatus;

//for AudioStatus.bmAdjust
#define Adj_Mute		0x01
#define Adj_Volume	0x02
#define Adj_Freq		0x04

typedef struct _AudioStatus
{
	BOOL bStartAudio;
	
//	BYTE bmAdjust;

	BYTE bMute;
	BYTE bVolume;
	BYTE bFreq;
} AudioStatus;

extern AudioStatus xdata gsAudioStatus;

typedef struct _VideoLimit
{
	WORD wMaxExposureTime;	//0x03E9
//	BYTE bMinRoll;				// 0
//	BYTE bMaxRoll;				// 180
	BYTE bMinBacklightComp;		// 16
	BYTE bMaxBacklightComp;		// 16
	BYTE bMaxPowerLineFreq;		// 2
	BYTE bMaxSharpness;			// 64
	BYTE bMinGamma;			// 1
	BYTE bMaxGamma;			// 16
} VideoLimit;

extern VideoLimit xdata gsVideoLimit;

typedef struct _AudioLimit
{
	WORD wMaxVolume;	//0x1FFF
} AudioLimit;

extern AudioLimit xdata gsAudioLimit;

typedef struct _VideoDefault
{
	BYTE bAEMode;
	WORD wExposureTime;
	BYTE bRoll;
	BYTE bBacklightComp;		//在gamma2curve[]中选择一组gamma
	BYTE bBrightness;
	BYTE bContrast;
	WORD wGain;
	BYTE bPowerLineFreq;
	BYTE bFlickerFlag;
	WORD wHue;
	//BYTE wHue;
	BYTE bSaturation;
	BYTE bSharpness;
	BYTE bGamma;
	WORD wWBC;
	BOOL bWBAuto;
} VideoDefault;

extern VideoDefault xdata gsVideoDefault;


/******************* Global Functions *************/
#define STATUS_SUCCESS TRUE
#define STATUS_FAILURE FALSE


#define SENSOR_WINSEL_WINQQVGA				0x01
#define SENSOR_WINSEL_WINQCIF				0x02
#define SENSOR_WINSEL_WINQVGA				0x03
#define SENSOR_WINSEL_WINCIF				0x04
#define SENSOR_WINSEL_WIN640x360			0x05
#define SENSOR_WINSEL_WIN640x400			0x06
#define SENSOR_WINSEL_WIN640x480			0x07
#define SENSOR_WINSEL_WIN640x512			0x08
#define SENSOR_WINSEL_WIN800x600			0x09
#define SENSOR_WINSEL_WIN1024x768			0x0A
#define SENSOR_WINSEL_WIN1280x720			0x0B
#define SENSOR_WINSEL_WIN1280x800			0x0C
#define SENSOR_WINSEL_WIN1280x960			0x0D
#define SENSOR_WINSEL_WIN1280x1024			0x0E
#define SENSOR_WINSEL_WIN1600x1200			0x0F

#define SPECIAL_EFFECT_NO						0x00
#define SPECIAL_EFFECT_RED						0x10
#define SPECIAL_EFFECT_CYAN						0x20
#define SPECIAL_EFFECT_BLACK_WHITE				0x30
#define SPECIAL_EFFECT_BLACK_WHITE_REVERSE		0x40
#define SPECIAL_EFFECT_CHROMA_REVERSE			0x50
#define SPECIAL_EFFECT_LUMININANCE_REVERSE		0x60
#define SPECIAL_EFFECT_ALL_REVERSE				0x70
#define SPECIAL_EFFECT_OFFSET_COLOR				0x80


//globalfunc.c :
void Delay(WORD a);
void dummy_sensor_function();
void dummy_sensor_function2(BYTE bValue);
void initSetFlicker();
//void pll_config(BYTE bVCO_cfg, BYTE bPLLA_cfg);
void pll_config(BYTE bPLLA_cfg);
void device_init();
#if (!MASK_VER_2)
void writeVideoWinIndex();
#endif
void readVideoWinIndex();
void readStillWinIndex();
void update_video_cur_attribute();
void proc_video_adjustment();
void update_audio_cur_attribute();

void usb_start_video();


#include "musbdefs.h"

/****************  Extern Data Declares **************/



//descript.c
extern BYTE code gcTestPacket[53];
//end descript.c

extern BYTE xdata bDualFormatOrder ;	// 1-yuv first; 0-jpeg first

extern volatile BYTE xdata bDebugFlg ; 	

extern BYTE xdata gbError;
extern BYTE xdata gbBuffer[64];
extern WORD xdata gwVID;
extern WORD xdata gwPID;

extern code MIC_VOL_MAP_TBL mic_vol_mapTbl[];

#if MASK_VER_2
extern BYTE xdata min_vol ;
#endif

extern BYTE xdata default_vol;


extern WORD xdata bmAttrAdjust;

//end globaldata.c

//VideoFunction.c
extern BYTE xdata gbVideoError;
extern BYTE xdata gbVideoErrorLast;
extern BYTE xdata bFlg_needFormatChange;
//extern BYTE xdata bManuGainFig ;
//end VideoFunction.c

//AudioFunction.c
extern BYTE xdata gbAudioError;
//end AudioFunction.c



//interrupt.c

void usb_endpoints_reset();
void usb_reset();
void usb_resume();
void usb_suspend();
void usb_interrupt_rom();
extern bit bSuspendFlg ;
extern bit bResetFlg ;
extern bit bResumeFlg ;
extern BYTE xdata gbReset;
//end interrupt.c

//sensorlib.c
extern DWORD xdata exposeTime;
extern WORD xdata exposeGain;
extern BYTE idata bRollDegree;
extern WORD idata AWB_Degree ;
//end sensorlib.c


//UsbRequest.c
extern bit bOtherSpeedDescFlg ;	// 0-use other-speed desc; 1-use fs desc
extern BYTE idata bCurAltSet;

void usb_proc_cmd();
void usb_ep0_transfer();
void RamBufferCtrlForOpenWindow();
void UsbSetInterface();
//end UsbRequest.c

//UvcRequest.c


//VenderRequest.c
extern bit gainFig;
extern BYTE xdata regAdd;
extern BYTE xdata regVal;
//end VenderRequest.c


//descript_def.c
extern WORD xdata bmVideoAttrContrls ;
extern BYTE xdata bmCamAttrControls1 ;
extern BYTE xdata bmCamAttrControls2 ;
extern BYTE xdata bJpegIndex_FS_1 ;
extern BYTE xdata bJpegIndex_FS_2 ;
extern BYTE xdata bJpegIndex_FS_3 ;
extern BYTE xdata bJpegIndex_FS_4 ;
extern BYTE xdata bJpegIndex_HS_1 ;
extern BYTE xdata bJpegIndex_HS_2 ;
extern bit bFlg_mjpeg;	//0 - no mjpeg, 1 - mjpeg

//引入数组yuv_winFrame_order作为YUV格式帧INDEX顺序映射表，通过修改此表可调整各分辩率相对应帧索引
//每个SENSOR初始化时应初始化yuv_winFrame_order
extern BYTE xdata yuv_winFrame_order[15];
//引入数组mjpeg_winFrame_order作为YUV格式帧INDEX顺序映射表，通过修改此表可调整各分辩率相对应帧索引
//每个SENSOR初始化时应初始化正确的mjpeg_winFrame_order
extern BYTE xdata mjpeg_winFrame_order[15];
//引入数组yuv_winFrame_BufferSize
//每个SENSOR初始化时应初始化正确的yuv_winFrame_BufferSize
extern DWORD xdata yuv_winFrame_BufferSize[15];
//引入数组mjpeg_winFrame_BufferSize
//每个SENSOR初始化时应初始化正确的mjpeg_winFrame_BufferSize
extern DWORD xdata mjpeg_winFrame_BufferSize[15];
extern WORD code Frame_Width[15];
extern WORD code Frame_Height[15];
extern WORD xdata yuv_wFrameSize_width[15];
extern WORD xdata yuv_wFrameSize_height[15];
extern WORD xdata mjpeg_wFrameSize_width[15];
extern WORD xdata mjpeg_wFrameSize_height[15];
extern BYTE xdata UpdateBuff[1760];
extern BYTE xdata *pDesc;
extern WORD xdata nDescLength;
//end descript_def.c

//sensorcommon.c

#if MASK_VER_2
extern BYTE code Fs_AltSet[12];
extern BYTE code * xdata pFs_AltSet_addr;

extern BYTE code Hs_AltSet[8];
extern BYTE code * xdata pHs_AltSet_addr;
#endif

#endif
