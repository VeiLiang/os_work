#include <xm_proj_define.h>
#include "common.h"
#include "descript.h"

/**********************************************************************************************************
Description: 																						
    Define the device descript.
	cfghs is the USB 2.0+USB audio 1300k sensor
	cfghs1 is the USB 2.0+USB audio 300k sensor
	cfghs2 is the USB 2.0+USB audio 100k sensor
	cfghs3 is the USB 2.0 1300k sensor
	cfghs4 is the USB 2.0 300k sensor
	cfghs5 is the USB 2.0 100k sensor
	cfgfs is the USB 1.1+USB audio
	cfgfs1 is the USB 1.1
	cfgos is the other speed descript
	cfgfs3 is the USB 1.1 100k sensor
	cfgfs2 is the USB 1.1+  USB audio 100k sensor
***********************************************************************************************************/

BYTE code gcTestPacket[53] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
		0xAA, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
		0xEE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xBF, 0xDF,
		0xEF, 0xF7, 0xFB, 0xFD, 0xFC, 0x7E, 0xBF, 0xDF,
		0xFE, 0xF7, 0xFB, 0xFD, 0x7E, };

//string
BYTE code gcLangIDString[4] =
{
	0x04,0x03,0x09,0x04,
};

//string 1
/*
BYTE code gcManufacturerString[18] =
{	
   	0x12,0x03,//length & type
	0x41,0x00,//A
	0x52,0x00,//R
	0x4B,0x00,//K
	0x4D,0x00,//M
	0x49,0x00,//I
	0x43,0x00,//C		
	0x52,0x00,//R
	0x4F,0x00, //O
};*/

BYTE code gcManufacturerString[24] =
{	
   0x18,0x03,//length & type
	0x45,0x00, //E
	0x58,0x00, //X
	0x43,0x00, //C
	0x45,0x00, //E
	0x45,0x00, //E
	0x44,0x00, //D		
	0x53,0x00, //S
	0x50,0x00, //P
	0x41,0x00, //A
	0x43,0x00, //C
	0x45,0x00, //E
};

//string 2
/*
BYTE code gcProduceString[34] =
{
	0x22,0x03,		 //length & type
	0x55,0x00,      // U
	0x53,0x00,      // S
	0x42,0x00,      // B
	0x32,0x00,      // 2
	0x2E,0x00,      // .
	0x30,0x00,      // 0
	0x20,0x00,      //
	0x50,0x00,      // P
	0x43,0x00,      // C
	0x20,0x00,      //
	0x43,0x00,      // C
	0x41,0x00,      // A
	0x4D,0x00,      // M
	0x45,0x00,      // E
	0x52,0x00,      // R
	0x41,0x00,      // A
};*/

BYTE code gcProduceString[34] =
{
	0x22,0x03,		 //length & type
	0x58,0x00,      // X
	0x53,0x00,      // S
	0x50,0x00,      // P
	0x41,0x00,      // A
	0x43,0x00,      // C
	0x45,0x00,      // E
	0x20,0x00,      //
	0x50,0x00,      // P
	0x43,0x00,      // C
	0x20,0x00,      //
	0x43,0x00,      // C
	0x41,0x00,      // A
	0x4D,0x00,      // M
	0x45,0x00,      // E
	0x52,0x00,      // R
	0x41,0x00,      // A
};

//string 4
BYTE code gcProduceString1[22] =
{
   	0x16,0x03,//length & type
	0x55,0x00,		// U
	0x53,0x00,		// S
	0x42,0x00,		// B
	0x32,0x00,		// 2
	0x2E,0x00,		// .
	0x30,0x00,		// 0
	0x20,0x00,		//
	0x4D,0x00,		// M
	0x49,0x00,		// I
	0x43,0x00,			// C
};

//string 3
BYTE code gcSerialnumString[18] =
{
    0x12,0x03,//length & type
    0x32,0x00,	// 2
    0x30,0x00,	// 0
    0x31,0x00,	//0x30,0x00,	// 0
    0x30,0x00,	//0x36,0x00,	// 6
    0x30,0x00,	// 0
    0x38,0x00,	//0x35,0x00,	// 5
    0x30,0x00,	//0x31,0x00,	// 1
    0x31,0x00,	//0x36,0x00, 	// 6
};


//string
BYTE code HQualifierData[10] =
{
	0x0a,		//bLength
	0x06,		//Device Qualifier Type, fixed value
	0x00,0x02,	//bcdUSB : 2.0
	0xEF,		//bDeviceClass
	0x02,		//bDeviceSubClass
	0x01,		//bDeviceProtocol
	0x40,		//bMaxPacketSize0
	0x01,		//bNumConfigurations
	0x00,		//bReserved
};

#if 1
//string
BYTE code FQualifierData[10] =
{
	0x0a,
	0x06,
	0x10,0x01,	//bcdUSB : 1.1
	0xEF,
	0x02,
	0x01,
	0x40,
	0x01,
	0x00,
};
#endif

STD_DEV_DSCR xdata gsStdDevDesc;
#if 0
STD_DEV_DSCR code gcStdDevDesc =
{
	0x12,		//bLength
	0x01,		//bDescriptorType
	0x0002,		//bcdUSB
	0xEF,		//bDeviceClass
	0x02,		//bDeviceSubClass
	0x01,		//bDeviceProtocol
	0x40,		//bMaxPacketSize0
	0xec18,		//idVendor
	0x9933,		//idProduct
	0x0001,		//bcdDevice
	0x01,		//iManufacturer
	0x02,		//iProduct
	0x00,		//iSerialNumber
	0x01,		//bNumConfigurations
};
#endif
////STD_DEVQUAL_DSCR xdata gsDevQualDesc;

#if 1	//hally
STD_CFG_DSCR code gcStd_cfg_desc =
{
	////Initialise the descriptors for configuration 1, high speed
	0x09,	//bLength		  		= sizeof(STD_CFG_DSCR);
	0x02,	//bDescriptorType	
	// 0xB502,	//(WORD)(sizeof(M_CFG_HS1)<<8) + (WORD)(sizeof(M_CFG_HS1)>>8),//0xB502	//wTotalLength
	0xB5, 0x02,
	0x04,	//bNumInterfaces		
	0x01,	//bConfigurationValue 	
	0x00,	//iConfiguration	  		
	0x80,	//bmAttributes	  		
#if MAKE_FOR_ASIC_OR_FPGA
	//0x80,	//bMaxPower, 256mA
	0x64,	//bMaxPower, 200mA
	//0x60,	//bMaxPower, 192mA
	//0x5a,	//bMaxPower, 180mA
	//0x32,	//bMaxPower, 100mA
#else
	0xa0,	//bMaxPower, 320mA
#endif
};

IF_ASS_DSCR code gcStd_IAD_desc =
{
	//IAD  descriptors	{0e,03,00}
    0x08,   // bLength
	0x0B,   // bDescriptorType	INTERFACE ASSOCIATION
	0x00,   // bFirstInterface
	0x02,   // bInterfaceCount
	0x0E,   // bFunctionClass
	0x03,   // bFunctionSubClass
	0x00,   // bFunctionProtocol
	0x00,	//0x02,	//0x00,   // iFunction		//此处为0x02可能会导致认证不过
};

STD_IF_DSCR code gcStd_VCIF_desc =
{
	// VideoControl Interface
	0x09,	//bLength		  		sizeof(STD_IF_DSCR);
	0x04,	//bDescriptorType		
	0x00,	//bInterfaceNumber		
	0x00,	//bAlternateSetting  		
	0x01,	//bNumEndpoints		
	0x0E,	//bInterfaceClass		 CC_VIDEO
	0x01,	//bInterfaceSubClass 	SC_VIDEOCONTROL
	0x00,	//bInterfaceProtocol 		
	0x00,	//0x02,	//0x00,	//iInterface	//??? 是不是需要一个string ???
};

CS_VCHEAD_DSCR code gcCS_VCIF_desc =
{
// VIDEO CONTROL CS HEADER
    0x0D,       	//bLength
	0x24,     	//bDescriptorType 	CS_INTERFACE
	0x01,     	//bDescriptorSubType 	VC_HEADER
	// 0x0001,  	//bcdUVC   Must be 1.0
	0x00, 0x01,
	// 0x3300,  	//wTotalLength		总长度为51 Bytes
	0x33, 0x00,  	//wTotalLength		总长度为51 Bytes
   // 0x006CDC02,  // dwClockFrequency (48MHz)
   0x00, 0x6C, 0xDC, 0x02,  // dwClockFrequency (48MHz)
	//0x00366E01, //dwClockFrequency	(24MHz)
	0x01,     	// bInCollection		: total num of VS interfaces
	0x01,    	// baInterfaceNr	: interface num of first VS interface
};

CAM_TERM_DSCR code gcCS_Camera_IT_desc =
{
	// CAMERA TERMINAL
    0x12,      	// bLength
	0x24,    	// bDescriptorType	CS_INTERFACE
	0x02,	// bDescriptorSubType	VC_INPUT_TERMINAL
	0x01,      	// bTerminalID
	// 0x0102,  	// wTerminalType 	ITT_CAMERA
	0x01, 02,
	0x00,     	// bAssocTerminal
	0x00,     	// iTerminal
	// 0x0000,   	// wObjectiveFocalLengthMin
	0x00, 0x00,
	// 0x0000,   	// wObjectiveFocalLengthMax
	0x00, 0x00,
   // 0x0000,   	// wOcularFocalLength
   0x00, 0x00,
	0x03,     	// bControlSize	 :  有3  Bytes bmControls 
	// 注意: bmControls1 及 bmControls2 由外部变量 bmCamAttrControls1 及 bmCamAttrControls2 控制
	//       参考 init_descriptor
    0x00,	//0x0a,	//0x00,//hally 0x0A,     	// bmControls1		D1:AE Mode	D3:ExpTime(Abs)
    0x00,	//0x20,	//0x00,//hally 0x20,     	// bmControls2		D13:Roll(Abs)
    0x00,     	// bmControls3
//bmControls:	
//D0:Scanning Mode	D1:AE Mode	D2:AE Priority	D3:ExpTime(Abs)	
//D4:ExpTime(Rel)		D5:Focus(Abs)	D6:Focus(Rel)	D7:Iris(Abs)	
//D8:Iris(Rel)	D9:Zoom(Abs)	D10:Zoom(Rel)	D11:PanTilt(Abs)	
//D12:PanTilt(Rel)	D13:Roll(Abs)	D14:Roll(Rel)	D15:Reserved	
//D16:Reserved	D17:Focus,Auto	D18:Privacy	
};

PRO_UNIT_DSCR code gcCS_PU_desc =
{
	// PROCESSING UNIT
    0x0B,		//bLength
    0x24,		//bDescriptorType	CS_INTERFACE
    0x05,		//bDescriptorSubType	VC_PROCESSING_UNIT
    0x02,		//bUnitID
    0x01,		//bSourceID
    // 0x0000,	//0x0040,	//0x0000,		//wMaxMultiplier
    0x00, 0x00,
    0x02,		//bControlSize	:  有2  Bytes bmControls
    // 0x3f06,	//0xbf17,	//0x3f07,	//0x3F06,	//0x3F07,	//don't need backlightcompensation
    0x3f, 0x06,
//    0x3F,		//bmControls1	D0:Brightness	D1:Contrast	D2:Hue	D3:Saturation	D4:Sharpness	D5:Gamma
//    0x07,		//bmControls2	D8:BacklightCompensation	D9:Gain	D10:PowerLineFrequence
    0x00,		//iProcessing
    //缺1  Byte  的bmVideoStandards
//bmControls:	
//D0:Brightness	D1:Contrast	D2:Hue	D3:Saturation	
//D4:Sharpness	D5:Gamma	D6:WhiteBalanceTemperature	D7:WhiteBalanceComponent	
//D8:BacklightCompensation	D9:Gain	D10:PowerLineFrequence	D11:Hue,Auto	
//D12:WhiteBalanceTemperature,Auto	D13:WhiteBalanceComponent,Auto	D14:DigitalMultiplier	D15:DigitalMultiplierLimit	
//D16:AnalogVideoStandard	D17:AnalogVideoLockStatus   	
};



OUT_TERM_DSCR code gcCS_OT_desc =
{
	//OUTPUT TERMIAL
    0x09,      	//bLength
	0x24,   	//bDescriptorType 	CS_INTERFACE
	0x03,    	//bDescriptorSubType 	VC_OUTPUT_TERMINAL
	0x03,     	//bTerminalID		//被Class-specific VS Interface Input Header Descriptor  所绑定,  从而与ep3in  联系在一起
	// 0x0101,  	//wTerminalType 	TT_STREAMING
	0x01, 0x01,
	0x00,     	//bAssocTerminal
	0x02,	//0x03,	//0x02,     	//bSourceID	:  本Terminal  与上面的Processing Unit  相联
	0x00,     	//iTerminal
};

STD_EP_DSCR code gcSTD_ep_desc =
{
  	// STD INT EP : ep1in,  包大小为16 Bytes
    0x07,		//bLength
	0x05,	//bDescriptorType
	0x81,	//bEndpointAddress
	0x03,	//bmAttributes		:	Interrupt, No Sync (固定值)
	// 0x1000,	//wMaxPacketSize
	0x10, 0x00,
	0x0A,	//bInterval
};

CS_VCEP_DSCR code gcCS_ep_desc =
{
	// Class-specific VC Interrupt Endpoint
	0x05,	//bLength
	0x25,	//bDescriptorType
	0x03,	//bDescriptorSubType
	// 0x1000,	//wMaxTransferSize
	0x10, 0x00
};

STD_IF_DSCR code gcSTD_VSIF_desc =
{
	// VideoStream Interface 1 Alternate Seting 0
    0x09,			//bLength
	0x04,           //bDescriptorType
	0x01,           //bInterfaceNumber
	0x00,           //bAlternateSetting
	0x00,           //bNumEndpoints
	0x0E,           //bInterfaceClass
	0x02,           //bInterfaceSubClass
	0x00,           //bInterfaceProtocol
	0x00,			//iInterface
};

CS_VSINHEAD_DSCR code gcCS_VS_header_desc =
{
	// VS Input Header
	0x0E,           // bLength
	0x24,           // bDescriptorType		CS_INTERFACE
	0x01,           // bDescriptorSubType		VS_INPUT_HEADER
	0x01,           // bNumFormats		注意：设为0x02是枚举不成功
	// 0x4105,//hally 0x1C01,	    // wTotalLength		总长度为284  Bytes，注意此长度需随FORMAT数及FRAME数重新计算
	0x41, 0x05,
	VIDEO_EP_IN,           // bEndpointAddress	: 指出对应的Video Data Endpoint  为85
	0x00,           // bmInfo (D0 : Dynamic format change supported   0:No; 1:Yes)
	0x03,           // bTerminalLink : 指向上面的Video Output Terminal
	0x02,           // bStillCaptureMethod (still image capture method 2)
	0x01,           // bTriggerSupport (Hardware trigger supported for still image capture) 0 : Not supported; 1 : Supported
	0x00,           // bTriggerUsage		0 : Initiate still image capture; 1 : General purpose button event
	0x01,           // bControlSize
	0x00, //video playload format 1 - null          // bmaControls
	//0x04, //video playload format 1 - compression quality		//0x00,           // bmaControls
//D0:wKeyFrameRate 	D1:wPFrameRate	D2:wCompQuality	D3:wCompWindowSize
//D4:Generate Key Frame	D5:Updata Frame Segment
};

CS_VSINHEAD_DSCR1 code gcCS_VS_header_desc1 =
{
	// VS Input Header
	0x0F,           // bLength
	0x24,           // bDescriptorType		CS_INTERFACE
	0x01,           // bDescriptorSubType		VS_INPUT_HEADER
	0x02,	//0x01,           // bNumFormats		注意：设为0x02是枚举不成功
	//0x4105,//hally 0x1C01,	    // wTotalLength		总长度为284  Bytes，注意此长度需随FORMAT数及FRAME数重新计算
	0x41, 0x05,
	VIDEO_EP_IN,           // bEndpointAddress	: 指出对应的Video Data Endpoint  为85
	0x00,           // bmInfo (D0 : Dynamic format change supported   0:No; 1:Yes)
	0x03,           // bTerminalLink : 指向上面的Video Output Terminal
	0x02,           // bStillCaptureMethod (still image capture method 2)
	0x01,           // bTriggerSupport (Hardware trigger supported for still image capture) 0 : Not supported; 1 : Supported
	0x00,           // bTriggerUsage		0 : Initiate still image capture; 1 : General purpose button event
	0x01,           // bControlSize
	0x00, //video playload format 1 - null          // bmaControls
	0x04, //video playload format 1 - compression quality		//0x00,           // bmaControls
//D0:wKeyFrameRate 	D1:wPFrameRate	D2:wCompQuality	D3:wCompWindowSize
//D4:Generate Key Frame	D5:Updata Frame Segment
};

//#define	YUV422
VS_YUV_FORMAT_DSCR code gcCS_VS_yuvformat_desc =
{
	// VS FORMAT 1  YUV
	0x1B,           // bLength
	0x24,           // bDescriptorType
	0x04,           // bDescriptorSubType		//VS_FORMAT_UNCOMPRESSED
	0x01,           // bFormatIndex		注意：在此项目中固定为1
	0x05,           // bNumFrameDescriptors 注意：总帧数需随SENSOR变化，相应的会影响header内的wTotalLength
	
	// 0x5955,		//guidFormat0
	0x59, 0x55,
	// 0x5932,		//guidFormat1
	0x59, 0x32,
	// 0x0000,		//guidFormat2
	0x00, 0x00,
	// 0x1000,		//guidFormat3
	0x10, 0x00,
	// 0x8000,		//guidFormat4
	0x80, 0x00,
	// 0x00AA,		//guidFormat5
	0x00, 0xAA,
	// 0x0038,		//guidFormat6
	0x00, 0x38,
	// 0x9B71,		//guidFormat7
	0x9B, 0x71,
	0x10,           // bBitsPerPixel		//解码后
	0x01,           // bDefaultFrameIndex		:  指出哪种frame  是默认的
	0x00,           // bAspectRatioX
	0x00,           // bAspectRatioY
	0x00,           // bmInterlaceFlags
	0x00,           // bCopyProtect
};

//#define	YUV422
//default yuv frame
VS_MJPEG_FRAME_DSCR code gcCS_yuvframe_desc =
{
	// VS YUV FRAME 1	
	0x2A,             // bLength
	0x24,             // bDescriptorType
	0x05,             // bDescriptorSubType	//VS_FRAME_UNCOMPRESSED
	0x01,             // bFrameIndex
	0x00,             // bmCapabilities (bit0 set to 1 in method 1 still image capture ,other is set to 0) (bit1 Fixed Frame Rate)
	// 0x8002,           // wWidth   640
	0x80, 0x02,
	// 0xE001,           // wHeight  480
	0xE0, 0x01,
	// 0x00007701,	//0x00B80B00,       // dwMinBitRate
	0x00, 0x00, 0x77, 0x01,
	// 0x0000ca08,	//0x0000B80B,       // dwMaxBitRate
	0x00, 0x00, 0xca, 0x08,
	// 0x00983A00,       // dwMaxVideoFrameBufSize	deprecated //???应为00 60 09 00 吧???
	0x00, 0x98, 0x3A, 0x00,
	// 0x15160500,       // dwDefaultFrameInterval
	0x15, 0x16, 0x05, 0x00,
	0x04,	//0x04,             // bFrameIntervalType
	// 0x15160500,	//dwFrameInterval0
	0x15, 0x16, 0x05, 0x00,
	// 0x2A2C0A00,	//dwFrameInterval1
	0x2A, 0x2C, 0x0A, 0x00,
	// 0x40420F00,	//dwFrameInterval2
	0x40, 0x42, 0x0F, 0x00,
	// 0x80841E00,	//dwFrameInterval3
	0x80, 0x84, 0x1E, 0x00,
};

// MJPEG Video Format Descriptor
//mjpeg format
VS_MJPEG_FORMAT_DSCR code gcCS_VS_mjpegformat_desc =
{
	// VS FORMAT1    MJPEG
	0x0B,           //	Size of this Descriptor, in bytes: 11
	0x24,           //	Video Class-Specific Descriptor, CS_INTERFACE
	0x06,           // VS_FORMAT_MJPEG Descriptor subtype
	0x02,           // bFormatIndex
	0x05,           // bNumFrameDescriptors
	0x01,           // bmFlags (Uses fixed size samples.)
					 // 	D0: FixedSizeSamples. 1 = Yes
	0x01,           // bDefaultFrameIndex
	0x00,           // bAspectRatioX
	0x00,           // bAspectRatioY
	0x00,           // bmInterlaceFlags
	0x00,           // bCopyProtect
};

//default mjpeg frame Descriptor
VS_MJPEG_FRAME_DSCR code gcCS_mjpegframe_desc	=
{
	// VS MJPEG FRAME 1	
	0x2A,             //
	0x24,             //
	0x07,             //
	0x01,             // bFrameIndex
	0x00,             // bmCapabilities (bit0 set to 1 in method 1 still image capture ,other is set to 0)
	// 0x8002,           // wWidth   640(0x280)
	0x80, 0x02,
	// 0xE001,           // wHeight  480(0x1E0)
	0xE0, 0x01,
	// 0x00f40100,	//yuanxiang //0x00007701,	//0x00600900,       // 0x0A860100,       // dwMinBitRate
	0x00, 0xf4, 0x01, 00,			// 128000 bps
	// 0x00c0a800,	//yuanxiang //0x0000ca08,	//0x00009411,	//0x00401901,       // dwMaxBitRate
	0x00, 0xc0, 0xa8, 0x00,		// 11059200 bps
	// 0x00080700,	//yuanxiang //0x00600900,       // dwMaxVideoFrameBufSize
	0x00, 0x08, 0x07, 0x00,		// 460800 (0x70800)
	// 0x15160500,	//yuanxiang //0x0A8B0200,	//0x15160500,       // dwDefaultFrameInterval
	0x15, 0x16, 0x05, 0x00,	// 30帧	 1000000000ns/30帧=333333=0x51615
	0x04,	//0x04,             // bFrameIntervalType,
								 //	 支持的独立频率个数 4个 The number of discrete frame intervals supported
	// 0x0A8B0200,		//60fps	
	0x0A, 0x8B, 0x02, 0x00,	// 1s=1000000000ns/60帧=166666=0x28B0A
	// 0x15160500,		//30fps
	0x15, 0x16, 0x05, 0x00,	// 1s=1000000000ns/30帧=333333=0x51615
	// 0x2A2C0A00,	 	//15fps
	0x2A, 0x2C, 0x0A, 0x00,	// 1s=1000000000ns/15帧=666666=0xA2C2A
	//0x40420F00,		//10fps
	// 0x80841E00,		//5fps
	0x80, 0x84, 0x1E, 0x00,	// 1s=1000000000ns/5帧=2000000=0x1E8480
};

//STIL_IMA_FRAME_DSCR code gcCS_still_image_frame_desc =
STIL_IMA_YUVFRAME_DSCR code gcCS_still_image_yuvframe_desc =
{
	// Still image
	0x1B,	//0x12,//0x0E,	//bLength
	0x24,	//bDescriptorType
	0x03,	//bDescriptorSubType	VS_STILL_IMAGE_FRAME
	0x00,	//bEndpointAddress
	0x05,	//bNumImageSizePatterns		//??? 为何与上面的Frames  不对应???
	// 0x8002,	//wWidth1		640
	0x80, 0x02,
	// 0xE001,	//wHeight1		480
	0xE0, 0x01,
	// 0xA000,	//wWidth2		160
	0xA0, 0x00,
	// 0x7800,	//wHeight2		120
	0x78, 0x00,
	// 0xB000,	//wWidth3		176
	0xB0, 0x00,
	// 0x9000,	//wHeight3		144
	0x90, 0x00,
	// 0x4001,	//wWidth4		320
	0x40, 0x01,
	// 0xF000,	//wHeight4		240
	0xF0, 0x00,
	// 0x6001,	//wWidth5		352
	0x60, 0x01,
	// 0x2001,	//wHeight5		288
	0x20, 0x01,
	0x01,	//bNumCompressionPattern
	0x01,
};


STIL_IMA_MJPEGFRAME_DSCR code gcCS_still_image_mjpegframe_desc =
{
	// Still image
	0x1E,	//0x12,//0x0E,	//bLength
	0x24,	//bDescriptorType
	0x03,	//bDescriptorSubType	VS_STILL_IMAGE_FRAME
	0x00,	//bEndpointAddress
	0x05,	//bNumImageSizePatterns		//??? 为何与上面的Frames  不对应???
	// 0x8002,	//wWidth1		640
	0x80, 0x02,
	// 0xE001,	//wHeight1		480
	0xE0, 0x01,
	// 0xA000,	//wWidth2		160
	0xA0, 0x00,
	// 0x7800,	//wHeight2		120
	0x78, 0x00,
	// 0xB000,	//wWidth3		176
	0xB0, 0x00,
	// 0x9000,	//wHeight3		144
	0x90, 0x00,
	// 0x4001,	//wWidth4		320
	0x40, 0x01,
	// 0xF000,	//wHeight4		240
	0xF0, 0x00,
	// 0x6001,	//wWidth5		352
	0x60, 0x01,
	// 0x2001,	//wHeight5		288
	0x20, 0x01,
	0x04,	//bNumCompressionPattern
	0x01,
	0x02,
	0x05,
	0x0e,
};

	
COL_MATCH_DSCR code gcCS_color_match_desc =
{
	// COLOR DSCR
    0x06,		//bLength
    0x24,		//bDescriptorType
    0x0D,		//bDescriptorSubType	VS_COLORFORMAT
    0x00,	//0x01, 	//0x00,		//bColorPrimaries		//要不要改为1, 1, 4
    0x00,	//0x01,	//0x00,		//bTransferCharacteristics
    0x00,	//0x04,	//0x00		//bMatrixCoefficients
};


// A VideoStreaming interface containing a bulk endpoint for streaming shall support only alternate
//	setting zero. Additional alternate settings containing bulk endpoints are not permitted in a device
//	that is compliant with the Video Class specification
STD_IF_DSCR code gcSTD_VS_IF1_HS_bulk =
{
	//IF1_alt1	ep5in (BULK)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x00,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass， CC_VIDEO
	0x02,	//bInterfaceSubClass， SC_VIDEOSTREAMING
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_HS_bulk =
{
	// STD BULK EP : ep5in,  包大小为512 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	(BYTE)(0x80 | VIDEO_EP_INDEX),	//bEndpointAddress
	0x02,	//bmAttributes		: 	Bulk
	0x00, 0x02,
	0x01,	//bInterval , Never NAKs
			// 	For high-speed bulk/control OUT endpoints, 
			//	the bInterval must specify the maximum NAK rate of the endpoint. 
			//	A value of 0 indicates the endpoint never NAKs. 
			//	Other values indicate at most 1 NAK each	 
			//		bInterval number of microframes. This value must be	 
			//		in the range from 0 to 255.	 
};


#if 1
STD_IF_DSCR code gcSTD_VS_IF1_HS_desc1 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x01,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass， CC_VIDEO
	0x02,	//bInterfaceSubClass， SC_VIDEOSTREAMING
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_HS_desc1 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x05,	//bmAttributes		: 	iso, asynchronization (固定的)
	// 0xc000,	//wMaxPacketSize, here is 192  , total bandwidth = 1536000 , 160x120@40fps
	0xc0, 0x00,
	0x01,	//bInterval
};

STD_IF_DSCR code gcSTD_VS_IF1_HS_desc2 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 2
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x02,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_HS_desc2 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x05,	//bmAttributes		: 	iso, asynchronization (固定的)
	// 0x8001,	//wMaxPacketSize , here is 384  , total bandwidth = 3072000 , 160x120@80fps
	0x80, 0x01,
	0x01,	//bInterval
};

STD_IF_DSCR code gcSTD_VS_IF1_HS_desc3 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 3
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x03,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_HS_desc3 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x05,	//bmAttributes		: 	iso, asynchronization (固定的)
	// 0x0002,	//wMaxPacketSize  , here is 512  , total bandwidth = 4096000 , 160x120@106fps
	0x00, 0x02,
	0x01,	//bInterval
};

STD_IF_DSCR code gcSTD_VS_IF1_HS_desc4 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x04,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_HS_desc4 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x05,	//bmAttributes		: 	iso, asynchronization (固定的)
	// 0x8002,	//wMaxPacketSize	, here is 640  , total bandwidth = 5120000 , 160x120@133fps
	0x80, 0x02,
	0x01,	//bInterval
};

STD_IF_DSCR code gcSTD_VS_IF1_HS_desc5 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x05,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_HS_desc5 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x05,	//bmAttributes		: 	iso, asynchronization (固定的)
	// 0x2003,	//wMaxPacketSize   , here is 800  , total bandwidth = 6400000 , 160x120@166fps
	0x20, 0x03,
	0x01,	//bInterval
};

STD_IF_DSCR code gcSTD_VS_IF1_HS_desc6 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x06,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_HS_desc6 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x05,	//bmAttributes		: 	iso, asynchronization (固定的)
	// 0xc003,	//0xb003, //wMaxPacketSize	  , here is 960  , total bandwidth = 7680000 , 160x120@200fps
	0xc0, 0x03,					// 0x3C0 = 960
	0x01,	//bInterval
};

STD_IF_DSCR code gcSTD_VS_IF1_HS_desc7 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x07,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_HS_desc7 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x05,	//bmAttributes		: 	iso, asynchronization (固定的)
	// 0x800a,	//wMaxPacketSize  , here is 1280  , total bandwidth = 10240000 , 160x120@266fps
	0x80, 0x0a,					// 0x280 = 640, 01B = 1 additional (2 per microframe)
	0x01,	//bInterval
};

STD_IF_DSCR code gcSTD_VS_IF1_HS_desc8 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x08,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_HS_desc8 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x05,	//bmAttributes		: 	iso, asynchronization (固定的)
	// 0x200b,	//wMaxPacketSize   , here is 1600  , total bandwidth = 12800000 , 160x120@333fps
	0x20, 0x0b,					// 0x320 = 800, 01B = 1 additional (2 per microframe)
	0x01,	//bInterval
};

STD_IF_DSCR code gcSTD_VS_IF1_HS_desc9 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x09,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_HS_desc9 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x05,	//bmAttributes		: 	iso, asynchronization (固定的)
	// 0xE00b,	//wMaxPacketSize  , here is 1984  , total bandwidth = 15872000 , 640x480@25fps
	0xE0, 0x0b,					// 0x3E0=992, 01 = 1 additional (2 per microframe)
	0x01,	//bInterval
};

STD_IF_DSCR code gcSTD_VS_IF1_HS_desc10 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x0a,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_HS_desc10 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x05,	//bmAttributes		: 	iso, asynchronization (固定的)
	//0x8013,	//wMaxPacketSize   , here is 2688  , total bandwidth = 21504000 , 160x120@560fps
	// 0x8013,	//wMaxPacketSize   , here is 2688  , total bandwidth = 21504000 , 640x480@35fps
	0x80, 0x13,					// 0x380=896, 10 = 2 additional (3 per microframe)
	0x01,	//bInterval
};
#endif

STD_IF_DSCR code gcSTD_VS_IF1_HS_desc11 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x0b,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_HS_desc11 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x05,	//bmAttributes		: 	iso, asynchronization (固定的)
	//0xe813,	//0xfc13,	//wMaxPacketSize  , here is 3060  , total bandwidth = 24000000 , 640x480@40fps
	// 0xe813,	//0xfc13,	//wMaxPacketSize  , here is 3060  , total bandwidth = 24000000 , 640x480@40fps
	0xe8, 0x13,				// 0x3E8=1000, 10 = 2 additional (3 per microframe)
	0x01,	//bInterval
};

#if 1
STD_IF_DSCR code gcSTD_VS_IF1_FS_desc1 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x01,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_FS_desc1 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x01,	//bmAttributes		: 	iso, asynchronization (固定的)
	// 0xe001,	//wMaxPacketSize //1.1HUB下同插2个设备，可支持2个320x240以下窗口，只支持一个录音（带宽不够）
	0xe0, 0x01,
	0x01,	//bInterval
};

STD_IF_DSCR code gcSTD_VS_IF1_FS_desc2 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x02,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_FS_desc2 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x01,	//bmAttributes		: 	iso, asynchronization (固定的)
	// 0x0002,	//0x0003,	//wMaxPacketSize
	0x00, 0x02,
	0x01,	//bInterval
};

STD_IF_DSCR code gcSTD_VS_IF1_FS_desc3 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x03,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_FS_desc3 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x01,	//bmAttributes		: 	iso, asynchronization (固定的)
	// 0x2002,	//wMaxPacketSize //1.1HUB下同时插2个，可同时支持2个320x240以下窗口，但不能录音（带宽不够）
	0x20, 0x02,
	0x01,	//bInterval
};

STD_IF_DSCR code gcSTD_VS_IF1_FS_desc4 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x04,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_FS_desc4 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x01,	//bmAttributes		: 	iso, asynchronization (固定的)
	// 0x4002,	//0x6003,	//wMaxPacketSize
	0x40, 0x02,
	0x01,	//bInterval
};

STD_IF_DSCR code gcSTD_VS_IF1_FS_desc5 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x05,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_FS_desc5 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x01,	//bmAttributes		: 	iso, asynchronization (固定的)
	// 0x2003,	//0x8003,	//wMaxPacketSize
	0x20, 0x03,
	0x01,	//bInterval
};

STD_IF_DSCR code gcSTD_VS_IF1_FS_desc6 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x06,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_FS_desc6 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x01,	//bmAttributes		: 	iso, asynchronization (固定的)
	//0x8003,	//wMaxPacketSize 	//会闪屏的HUB在1.1下打开视频时不能录音（不能分配带宽）
	// 0x3803,	//wMaxPacketSize 	//会闪屏的HUB在1.1下打开视频时可录音
	0x38, 0x03,
	//0x9002,	//wMaxPacketSize	//会闪屏的HUB在1.1下打开视频时可录音，但切窗口会闪屏
	0x01,	//bInterval
};

STD_IF_DSCR code gcSTD_VS_IF1_FS_desc7 =
{
	//IF1_alt1	ep5in (ISO)
	// VideoStream Interface 1 Alternate Seting 1
	0x09,	//bLength
	0x04,	//bDescriptorType
	0x01,	//bInterfaceNumber
	0x07,	//bAlternateSetting
	0x01,	//bNumEndpoints
	0x0E,	//bInterfaceClass
	0x02,	//bInterfaceSubClass
	0x00,	//bInterfaceProtocol
	0x00,	//iInterface
};

STD_EP_DSCR code gcSTD_VS_ep_FS_desc7 =
{
	// STD ISO EP : ep5in,  包大小为5096 Bytes
	0x07,	//bLength
	0x05,	//bDescriptorType
	0x85,	//bEndpointAddress
	0x01,	//bmAttributes		: 	iso, asynchronization (固定的)
	// 0xa003,	//0xe803,	//0xff03,	//wMaxPacketSize
	0xa0, 0x03,
	0x01,	//bInterval
};
#endif


//audio
AUDIO_IAD code gcAudio_IAD_desc =
{
	// AUDIO IAD {01, 01, 00},  也许应为{01, 00, 00}
    0x08,		//bLength
    0x0B,		//bDescriptorType
    0x02,		//bFirstInterface
    0x02,		//bInterfaceCount
    0x01,		//bFunctionClass : AUDIO_FUNCTION
    0x01,		//bFunctionSubClass : AUDIOSTREAMING		??? FUNCTION_SUBCLASS_UNDEFINED : 0x00 ???
    0x00,		//bFunctionProtocol	??? AF_VERSION_02_00 : 0x20 ???
    0x04		//iFunction
};

STD_IF_DSCR code gcSTD_ACIF_desc =
{
	// AUDIO Control Interface : bInterfaceProtocol  可能有问题
    0x09,		//bLength
    0x04,		//bDescriptorType
    0x02,		//bInterfaceNumber
    0x00,		//bAlternateSetting
    0x00,		//bNumEndpoints
    0x01,		//bInterfaceClass		AUDIO
    0x01,		//bInterfaceSubClass		AUDIOCONTROL
    0x00,		//bInterfaceProtocol	: ??? IP_VERSION_02_00 : 0x20 ???
    0x04		//iInterface
};

CS_ACHEAD_DSCR code gcCS_AcHead_desc =
{
	// AUDIO CONTROL CS HEADER
    0x09,		//bLength
    0x24,		//bDescriptorType : CS_INTERFACE
    0x01,		//bDescriptorSubtype : HEADER
    // 0x0001,	//bcdADC	: 原来是参照ADC 1.0 的啊!!!
	 0x00, 0x01,
    // 0x2800,	//wTotalLength		总长度为40 Bytes
	 0x28, 0x00,
    0x01,		//bInCollection		: total number of AS interfaces
    0x03,		//baInterfaceNr		: interface num of first AS interface	
};

AIN_TERM_DSCR code gcAudio_IT_desc =
{
	// Input Terminal
    0x0C,		//bLength
    0x24,		//bDescriptorType
    0x02,		//bDescriptorSubtype	INPUT_TERMINAL
    0x01,		//bTerminalID
    // 0x0102,	//wTerminalType	:  Microphone
	 0x01, 0x02,
    0x00,		//bAssocTerminal
    0x02,		//bNrChannels		//Embedded audio channel cluster descriptor	//应为1  吧
    // 0x0300,	//wChannelConfig	//Embedded audio channel cluster descriptor	//Left Front & Right Front
	 0x03, 0x00,
    0x00,		//iChannelNames	//Embedded audio channel cluster descriptor
    0x00,		//iTerminal
};

FEA_UNIT_DSCR code gcFeater_desc =
{
	// Feature Unit
    0x0A,		//bLength
    0x24,		//bDescriptorType
    0x06,		//bDescriptorSubtype
    0x02,		//bUnitID
    0x01,		//bSourceID
    0x01,		//bControlSize	: 每个bmaControls  占用1  Byte
    0x00,		//bmaControls0		: Virtual Channel	//D0:Mute, D1:Volume, D2:Bass, D3:Mid,     D4:Treble, D5:Graphic Equalizer, D6:Automatic Gain, D7:Delay,     D8:Bass Boost, D9:Loundness
    0x03,		//bmaControls1		: Left Front Channel
    0x03,		//bmaControls2		: Right Front Channel
    0x00,		//iFeature
};

AOUT_TERM_DSCR code gcAudio_OT_desc =
{
	// Output Terminal
    0x09,		//bLength
    0x24,		//bDescriptorType		CS_INTERFACE
    0x03,		//bDescriptorSubtype	OUTPUT_TERMINAL
    0x03,		//bTerminalID			//被下面的IF3_alt1  的CS AS GENERAL  所绑定,  从而与ep2in  联系在一起
    // 0x0101,	//wTerminalType		USB Streaming
	 0x01, 0x01,
    0x00,		//bAssocTerminal
    0x02,		//bSourceID
    0x00,		//iTerminal
};

//IF3_alt0
STD_IF_DSCR code gcSTD_as0_desc =
{
	// AS Interface Alternate Seting  0
    0x09,		//bLength
    0x04,		//bDescriptorType		M_DST_INTERFACE
    0x03,		//bInterfaceNumber
    0x00,		//bAlternateSetting
    0x00,		//bNumEndpoints
    0x01,		//bInterfaceClass		AUDIO
    0x02,		//bInterfaceSubClass		AUDIOSTREAMING
    0x00,		//bInterfaceProtocol
    0x04,		//iInterface
};

//IF3_alt1		ep2in  (ISO)
STD_IF_DSCR code gcSTD_as1_desc =
{
    // AS Interface Alternate Seting  1
    0x09,		//bLength
    0x04,		//bDescriptorType		M_DST_INTERFACE
    0x03,		//bInterfaceNumber
    0x01,		//bAlternateSetting
    0x01,		//bNumEndpoints
    0x01,		//bInterfaceClass		AUDIO
    0x02,		//bInterfaceSubClass		AUDIOSTREAMING
    0x00,		//bInterfaceProtocol
    0x04,		//iInterface
};

CS_AS_GENE code gcCS_as_general_desc =
{
	// CS AS GENERAL
    0x07,		//bLength
    0x24,		//bDescriptorType		CS_INTERFACE
    0x01,		//bDescriptorSubtype	AS_GENERAL
    0x03,		//bTerminalLink : 指向上面的Audio Output Terminal
    0x01,		//bDelay
    // 0x0100,	//wFormatTag		PCM		决定接下来的FORMAT TYPE
	 0x01, 0x00
	//wFormatTag
	//0x0001		PCM
	//0x0002		PCM8
	//0x0003		IEEE_FLOAS
	//0x0004		ALAW
	//0x0005		MULAW
};

AS_FORMAT_DSCR code gcCS_as_format_desc =
{
	// FORMAT TYPE
    0x0B,		//bLength
    0x24,		//bDescriptorType		CS_INTERFACE
    0x02,		//bDescriptorSubtype	FORMAT_TYPE
    0x01,		//bFormatyType	FORMAT_TYPE_I
    0x01,		//bNrChannels
    0x02,		//bSubFrameSize
    0x10,		//bBitResolution
    0x01,		//bSamFreqType	有1  种frequency (每种freq  占用3  个Bytes)
    0x80,		//tSamFreq1_B1        	48K Hz
    0xBB,		//tSamFreq1_B2
    0x00,		//tSamFreq1_B3
};

AUD_EP_DSCR code gcSTD_as_ep_desc =
{
	// STD ISO EP : ep2in
    0x09,		//bLength
    0x05,		//bDescriptorType
    0x82,		//bEndpointAddress
    0x05,	//0x01,		//bmAttributes		:	iso, no synchronization	//??? 应为0x05  吧???
    // 0x6000,	//0x8000,	//wMaxPacketSize
	 0x60, 0x00,
    // 0x0400,	//wInterval	//???  bInterval + bRefresh,  且取固定值0x0100 ???
	 0x04, 0x00,
    0x00,		//bSyncAddress
};

CS_EP_DSCR code gcCS_as_ep_desc =
{
	// CS EP
    0x07,		//bLength
    0x25,		//bDescriptorType		CS_ENDPOINT
    0x01,		//bDescriptorSubtype	EP_GENERAL
    0x01,		//bmAttributes		//D0:Sampling Frequency Control	D1:Pitch Control		D7(flag):MaxPacketsOnly
    0x00,		//bLockDelayUnits
    // 0x0000,	//wLockDelay
	 0x00, 0x00
};
#endif

