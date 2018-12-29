#ifndef _DESCRIPT_H
#define _DESCRIPT_H

/* Request Type Field */
#define M_CMD_TYPEMASK	0x60

#define M_CMD_STDREQ	0x00
#define M_CMD_CLASSREQ	0x20
#define M_CMD_VENDREQ	0x40

#define M_CMD_STDDEVIN	0x80
#define M_CMD_STDDEVOUT	0x00
#define M_CMD_STDIFIN	0x81
#define M_CMD_STDIFOUT	0x01
#define M_CMD_STDEPIN	0x82
#define M_CMD_STDEPOUT	0x02

/* Standard Request Codes */
#define GET_STATUS			0x00
#define CLEAR_FEATURE		0x01
#define SET_FEATURE			0x03
#define SET_ADDRESS			0x05
#define GET_DESCRIPTOR		0x06
#define SET_DESCRIPTOR		0x07
#define GET_CONFIGURATION	0x08
#define SET_CONFIGURATION	0x09
#define GET_INTERFACE		0x0A
#define SET_INTERFACE		0x0B
#define SYNCH_FRAME			0x0C

/* Standard Device Feature Selectors */
#define M_FTR_DEVREMWAKE 0x0001
#define M_FTR_TESTMODE	 0x0002

/*Class-Specific Request Codes*/
#define SET_CUR         0x01
#define GET_CUR         0x81 	//读取当前值
#define GET_MIN         0x82	//读取最小值
#define GET_MAX         0x83	//读取最大值
#define GET_RES         0x84	//读取调节步长
#define GET_LEN         0x85      //读取长度
#define GET_INFO        0x86	//读取属性
#define GET_DEF        	0x87	//读取默认值


/********************* VideoControl Interface *********************/

/*VideoControl Interface Control Selectors*/
#define VC_CONTROL_UNDEFINED			0x00
#define VC_VIDEO_POWER_MODE_CONTROL    	0x01
#define VC_REQUEST_ERROR_CODE_CONTROL  	0x02

/*Camera Temrminal Control Selectors*/
#define CT_CONTROL_UNDEFINED             	0x00
#define CT_SCANNING_MODE_CONTROL            0x01
#define CT_AE_MODE_CONTROL                  0x02
#define CT_AE_PRIORITY_CONTROL              0x03
#define CT_EXPOSURE_TIME_ABSOLUTE_CONTROL   0x04
#define CT_EXPOSURE_TIME_RELATIVE_CONTROL   0x05
#define CT_FOCUS_ABSOLUTE_CONTROL           0x06
#define CT_FOCUS_RELATIVE_CONTROL           0x07
#define CT_FOCUS_AUTO_CONTROL               0x08
#define CT_IRIS_ABSOLUTE_CONTROL            0x09
#define CT_IRIS_RELATIVE_CONTROL            0x0A
#define CT_ZOOM_ABSOLUTE_CONTROL            0x0B
#define CT_ZOOM_RELATIVE_CONTROL            0x0C
#define CT_PANTILT_ABSOLUTE_CONTROL         0x0D
#define CT_PANTILT_RELATIVE_CONTROL         0x0E
#define CT_ROLL_ABSOLUTE_CONTROL            0x0F
#define CT_ROLL_RELATIVE_CONTROL            0x10
#define CT_PRIVACY_CONTROL                  0x11

/*Processing Unit Control Selectors*/
#define PU_CONTROL_UNDEFINED                		0x00
#define PU_BACKLIGHT_COMPENSATION_CONTROL   		0x01
#define PU_BRIGHTNESS_CONTROL               		0x02
#define PU_CONTRAST_CONTROL                         0x03
#define PU_GAIN_CONTROL                             0x04
#define PU_POWER_LINE_FREQUENCY_CONTROL             0x05
#define PU_HUE_CONTROL                              0x06
#define PU_SATURATION_CONTROL                       0x07
#define PU_SHARPNESS_CONTROL                        0x08
#define PU_GAMMA_CONTROL                            0x09
#define PU_WHITE_BALANCE_TEMPERATURE_CONTROL        0x0A
#define PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL   0x0B
#define PU_WHITE_BALANCE_COMPONENT_CONTROL          0x0C
#define PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL     0x0D
#define PU_DIGITAL_MULTIPLIER_CONTROL               0x0E
#define PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL         0x0F
#define PU_HUE_AUTO_CONTROL                         0x10
#define PU_ANALOG_VIDEO_STANDARD_CONTROL            0x11
#define PU_ANALOG_LOCK_STATUS_CONTROL               0x12


/********************* VideoStreaming Interface *********************/

/*VideoStreaming Interface Control Selectors*/
#define VS_CONTROL_UNDEFINED                        0x00
#define VS_PROBE_CONTROL                            0x01
#define VS_COMMIT_CONTROL                           0x02
#define VS_STILL_PROBE_CONTROL                      0x03
#define VS_STILL_COMMIT_CONTROL                     0x04
#define VS_STILL_IMAGE_TRIGGER_CONTROL              0x05
#define VS_STREAM_ERROR_CODE_CONTROL                0x06
#define VS_GENERATE_KEY_FRAME_CONTROL               0x07
#define VS_UPDATE_FRAME_SEGMENT_CONTROL             0x08
#define VS_SYNCH_DELAY_CONTROL                      0x09


/********************* AudioControl Interface *********************/

/*Feature Unit Control Selectors*/
#define FU_CONTROL_UNDEFINED			0x00
#define MUTE_CONTROL					0x01
#define VOLUME_CONTROL					0x02
#define BASS_CONTROL					0x03
#define MID_CONTROL						0x04
#define TREBLE_CONTROL					0x05
#define GRAPHIC_EQUALIZER_CONTROL		0x06
#define AUTOMATIC_GAIN_CONTROL			0x07
#define DELAY_CONTROL					0x08
#define BASS_BOOST_CONTROL				0x09
#define LOUDNESS_CONTROL				0x0A


/********************* AudioStreaming Interface *********************/

/*Endpoint Control Selectors*/
#define EP_CONTROL_UNDEFINED		0x00
#define SAMPLING_FREQ_CONTROL		0x01
#define PITCH_CONTROL				0x02


/* Interface Number */
#define VIDEO_CONTROL		0x00
#define VIDEO_STREAMING		0x01
#define AUDIO_CONTROL		0x02
#define AUDIO_STREAMING		0x03

#define AUDIO_ENDPOINT		0x82

/* Video Entity ID */
#define VC_INTERFACE_CONTROL	0x00
#define VC_CAMERA_TERMINAL		0x01
#define VC_PROCESSING_UNIT		0x02
#define VC_OUTPUT_TERMINAL		0x03
#define VC_SELECTOR_UNIT		0x04

/* Audio Entity ID */
#define AC_FEATURE_UNIT		0x02

/****************** Descriptor Structs **********************/

/* Standard Device Descriptor */
#pragma pack(1)
typedef struct STD_DEV_DSCR {
  BYTE	bLength;
  BYTE	bDescriptorType;
//  WORD  bcdUSB;
  BYTE  bcdUSB[2];
  BYTE	bDeviceClass;
  BYTE	bDeviceSubClass;
  BYTE	bDeviceProtocol;
  BYTE	bMaxPacketSize0;
//  WORD  idVendor;
  BYTE  idVendor[2];
//  WORD  idProduct;
  BYTE  idProduct[2];
//  WORD  bcdDevice;
  BYTE  bcdDevice[2];
  BYTE	iManufacturer;
  BYTE	iProduct;
  BYTE	iSerialNumber;
  BYTE	bNumConfigurations;
  } STD_DEV_DSCR;
typedef STD_DEV_DSCR * PSTD_DEV_DSCR;

/* Standard Device_Qualifier Descriptor */
typedef struct STD_DEVQUAL_DSCR {
  BYTE	bLength;
  BYTE	bDescriptorType;
  // WORD  bcdUSB;
  BYTE  bcdUSB[2];
  BYTE	bDeviceClass;
  BYTE	bDeviceSubClass;
  BYTE	bDeviceProtocol;
  BYTE	bMaxPacketSize0;
  BYTE	bNumConfigurations;
  BYTE	bReserved;
  } STD_DEVQUAL_DSCR;
typedef STD_DEVQUAL_DSCR * PSTD_DEVQUAL_DSCR;

/* Standard Configuration Descriptor */
typedef struct STD_CFG_DSCR {
  BYTE	bLength;
  BYTE	bDescriptorType;
  // WORD   wTotalLength;
  BYTE   wTotalLength[2];
  BYTE	bNumInterfaces;
  BYTE	bConfigurationValue;
  BYTE	iConfiguration;
  BYTE	bmAttributes;
  BYTE	bMaxPower;
  } STD_CFG_DSCR;
typedef STD_CFG_DSCR * PSTD_CFG_DSCR;

/* Standard Interface Descriptor */
typedef struct STD_IF_DSCR {
  BYTE	bLength;
  BYTE	bDescriptorType;
  BYTE	bInterfaceNumber;
  BYTE	bAlternateSetting;
  BYTE	bNumEndpoints;
  BYTE	bInterfaceClass;
  BYTE	bInterfaceSubClass;
  BYTE	bInterfaceProtocol;
  BYTE	iInterface;
  } STD_IF_DSCR;
typedef STD_IF_DSCR * PSTD_IF_DSCR;

/* Standard Endpoint Descriptor */
typedef struct STD_EP_DSCR {
  BYTE	bLength;
  BYTE	bDescriptorType;
  BYTE	bEndpointAddress;
  BYTE	bmAttributes;
  // WORD	wMaxPacketSize;
  BYTE	wMaxPacketSize[2];
  BYTE	bInterval;
  } STD_EP_DSCR;
typedef STD_EP_DSCR * PSTD_EP_DSCR;

/*Interface Association Deccriptor*/
typedef struct _IF_ASS_DSCR {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bFirstInterface;
  BYTE  bInterfaceCount;
  BYTE  bFunctionClass;
  BYTE  bFunctionSubClass;
  BYTE  bFunctionProtocol;
  BYTE  iFunction;
} IF_ASS_DSCR;
typedef IF_ASS_DSCR * PIF_ASS_DSCR;

/*Class-specific VC Interface Header Descriptor*/
typedef struct CS_VCHEAD_DSCR {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  // WORD  bcdUVC;
  BYTE  bcdUVC[2];
  // WORD  wTotalLength;
  BYTE  wTotalLength[2];
  // DWORD dwClockFrequency;
  BYTE dwClockFrequency[4];
  BYTE  bInCollection;
  BYTE  baInterfaceNr;
  } CS_VCHEAD_DSCR;
typedef CS_VCHEAD_DSCR * PCS_VCHEAD_DSCR;

/*Output Terminal Descriptor*/
typedef struct OUT_TERM_DSCR {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  BYTE  bTerminalID;
  // WORD  wTerminalType;
  BYTE  wTerminalType[2];
  BYTE  bAssocTerminal;
  BYTE  bSourceID;
  BYTE  iTerminal;
  } OUT_TERM_DSCR;
typedef OUT_TERM_DSCR * POUT_TERM_DSCR;

/*Camera Terminal Descriptor*/
typedef struct CAM_TERM_DSCR {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  BYTE  bTerminalID;
  // WORD  wTerminalType;
  BYTE  wTerminalType[2];
  BYTE  bAssocTerminal;
  BYTE  iTerminal;
  // WORD  wObjectiveFocalLengthMin;
  BYTE  wObjectiveFocalLengthMin[2];
  // WORD  wObjectiveFocalLengthMax;
  BYTE  wObjectiveFocalLengthMax[2];
  // WORD  wOcularFocalLength;
  BYTE  wOcularFocalLength[2];
  BYTE  bControlSize;
  BYTE  bmControls1;
  BYTE  bmControls2;
  BYTE  bmControls3;
  } CAM_TERM_DSCR;
typedef CAM_TERM_DSCR * PCAM_TERM_DSCR;

/*Selector Unit Descriptor*/
typedef struct SELE_UNIT_DSCR {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  BYTE  bUnitID;
  BYTE  bNrInPins;
  BYTE  baSourceID;
  BYTE  iSelector;
  } SELE_UNIT_DSCR;
typedef SELE_UNIT_DSCR * PSELE_UNIT_DSCR;

 /*Processing Unit Descriptor*/
typedef struct PRO_UNIT_DSCR {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  BYTE  bUnitID;
  BYTE  bSourceID;
  // WORD  wMaxMultiplier;
  BYTE  wMaxMultiplier[2];
  BYTE  bControlSize;
  // WORD  bmControls;
  BYTE  bmControls[2];
  BYTE  iProcessing;
  } PRO_UNIT_DSCR;
typedef PRO_UNIT_DSCR * PPRO_UNIT_DSCR;


#if 0
/*Extension Unit Descriptor*/
typedef struct EXTENSION_UNIT_DSCR {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  BYTE  bUnitID;
  WORD  guidExtensionCode0;
  WORD  guidExtensionCode1;
  WORD  guidExtensionCode2;
  WORD  guidExtensionCode3;
  WORD  guidExtensionCode4;
  WORD  guidExtensionCode5;
  WORD  guidExtensionCode6;
  WORD  guidExtensionCode7;
  BYTE  bNumControls;
  BYTE  bNrInPins;
  BYTE  baSourceID1;
  //BYTE  baSourceID2;
  BYTE  bControlSize;
  BYTE  bmControl1;
  BYTE  bmControl2;
  BYTE  bmControl3;
  BYTE  bmControl4;
  BYTE  iExtension;
  } EXTENSION_UNIT_DSCR;
typedef EXTENSION_UNIT_DSCR * PEXTENSION_UNIT_DSCR;
#endif

/*Class-specific VC Interrupt Endpoint Descriptor*/
typedef struct CS_VCEP_DSCR {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  // WORD  wMaxTransferSize;
  BYTE  wMaxTransferSize[2];
  } CS_VCEP_DSCR;
typedef CS_VCEP_DSCR * PCS_VCEP_DSCR;

/*Class-specific VS Interface Input Header Descriptor*/
//1 format
typedef struct CS_VSINHEAD_DSCR {
	BYTE  bLength;
	BYTE  bDescriptorType;
	BYTE  bDescriptorSubType;
	BYTE  bNumFormats;
	// WORD  wTotalLength;
	BYTE  wTotalLength[2];
	BYTE  bEndpointAddress;
	BYTE  bmInfo;
	BYTE  bTerminalLink;
	BYTE  bStillCaptureMethod;
	BYTE  bTriggerSupport;
	BYTE  bTriggerUsage;
	BYTE  bControlSize;
	BYTE  bmaControls1;
	//BYTE  bmaControls2;
} CS_VSINHEAD_DSCR;
typedef CS_VSINHEAD_DSCR * PCS_VSINHEAD_DSCR;

//2 format
typedef struct CS_VSINHEAD_DSCR1 {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  BYTE  bNumFormats;
  // WORD  wTotalLength;
  BYTE  wTotalLength[2];
  BYTE  bEndpointAddress;
  BYTE  bmInfo;
  BYTE  bTerminalLink;
  BYTE  bStillCaptureMethod;
  BYTE  bTriggerSupport;
  BYTE  bTriggerUsage;
  BYTE  bControlSize;
  BYTE  bmaControls1;
  BYTE  bmaControls2;
  } CS_VSINHEAD_DSCR1;
typedef CS_VSINHEAD_DSCR1 * PCS_VSINHEAD_DSCR1;

/*Class-specific VS Interface Output Header Descriptor*/
typedef struct CS_VSOUTHEAD_DSCR {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  BYTE  bNumFormats;
  // WORD  wTotalLength;
  BYTE  wTotalLength[2];
  BYTE  bEndpointAddress;
  BYTE  bTerminalLink;
  BYTE  bControlSize;
  // WORD  bmaControls;
  BYTE  bmaControls[2];
  } CS_VSOUTHEAD_DSCR;
typedef CS_VSOUTHEAD_DSCR * PCS_VSOUTHEAD_DSCR;

// Motion-JPEG Video Format Descriptor
// A MJPEG Video Format Descriptor is followed by one or more MJPEG Video Frame Descriptor(s); 
// each Video Frame Descriptor conveys information specific to a frame size supported for the format.
// 		The bmFlags field holds information about the video data stream characteristics.
//		FixedSizeSamples indicates whether all video samples are the same size.
/*Video Streaming MJPEG Format Type Descriptor*/
typedef struct VS_MJPEG_FORMAT_DSCR {
  BYTE  bLength;				// Size of this Descriptor, in bytes: 11
  BYTE  bDescriptorType;		// CS_INTERFACE Descriptor type.
  BYTE  bDescriptorSubType;	// VS_FORMAT_MJPEG Descriptor subtype
  BYTE  bFormatIndex;			// Index of this Format Descriptor
  								//		contains the one-based index of this format Descriptor, 
  								//		and is used by requests from the host to set and get the current video format.
  BYTE  bNumFrameDescriptors;	// Number of Frame Descriptors following that correspond to this format
  BYTE  bmFlags;				// Specifies characteristics of this format
								//		D0: FixedSizeSamples. 1 = Yes
								//		All other bits are reserved for future use and shall be reset to zero.
  BYTE  bDefaultFrameIndex;	// Optimum Frame Index (used to select resolution) for this stream
  BYTE  bAspectRatioX;			// The X dimension of the picture aspect ratio.
  BYTE  bAspectRatioY;			// The Y dimension of the picture aspect ratio.
  BYTE  bmInterlaceFlags;		// Specifies interlace information. 
  BYTE  bCopyProtect;			// Specifies if duplication of the video stream should be restricted:
								//		0: No restrictions
								//		1: Restrict duplication
  } VS_MJPEG_FORMAT_DSCR;
typedef VS_MJPEG_FORMAT_DSCR * PVS_MJPEG_FORMAT_DSCR;

/*Video Streaming YUV Format Type Descriptor*/
typedef struct VS_YUV_FORMAT_DSCR {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  BYTE  bFormatIndex;
  BYTE  bNumFrameDescriptors;
  // WORD  guidFormat0;
  BYTE  guidFormat0[2];
  // WORD  guidFormat1;
  BYTE  guidFormat1[2];
  // WORD  guidFormat2;
  BYTE  guidFormat2[2];
  // WORD  guidFormat3;
  BYTE  guidFormat3[2];
  // WORD  guidFormat4;
  BYTE  guidFormat4[2];
  // WORD  guidFormat5;
  BYTE  guidFormat5[2];
  // WORD  guidFormat6;
  BYTE  guidFormat6[2];
  // WORD  guidFormat7;
  BYTE  guidFormat7[2];
  BYTE  bBitsPerPixel;
  BYTE  bDefaultFrameIndex;
  BYTE  bAspectRatioX;
  BYTE  bAspectRatioY;
  BYTE  bmInterlaceFlags;
  BYTE  bCopyProtect;
  } VS_YUV_FORMAT_DSCR;
typedef VS_YUV_FORMAT_DSCR * PVS_YUV_FORMAT_DSCR;

/*Video Streaming MJPEG Frame Type Descriptor*/
// MJPEG Video Frame Descriptors
// One or more Frame Descriptors follow the MJPEG Video Format Descriptor they correspond to. 
// The Frame Descriptor is also used to determine the range of frame intervals that are supported 
// for the specified frame size.
typedef struct VS_MJPEG_FRAME_DSCR {
  BYTE  bLength;
  BYTE  bDescriptorType;				// CS_INTERFACE Descriptor type
  BYTE  bDescriptorSubType;			// VS_FRAME_MJPEG Descriptor subtype
  BYTE  bFrameIndex;					// Index of this Frame Descriptor
  										//		contains the one-based index of this Frame Descriptor, 
  										//		and is used by requests from the host to set and 
  										//		get the current frame index for the format in use. 
  										//		This index is one-based for each corresponding Format Descriptor 
  										//		supported by the device.
  BYTE  bmCapabilities;				//	D0: Still image supported
										//		Specifies whether still images are supported at this frame setting. 
										//		This is only applicable for VS interfaces with an IN video endpoint 
										//		using Still Image Capture Method 1, and should be set to 0 in 
										//		all other cases.
										//	D1: Fixed frame-rate
										//		Specifies whether the device provides a fixed frame rate on a stream 
										//		associated with this frame descriptor. Set to 1 if fixed rate is enabled; 
										//		otherwise, set to 0.
										//	D7..2: Reserved, set to 0.
  BYTE  wWidth[2];						//	Width of decoded bitmap frame in pixels
  BYTE  wHeight[2];					//	Height of decoded bitmap frame in pixels
  BYTE  dwMinBitRate[4];				//	Specifies the minimum bit rate at default compression quality and 
  										//		longest frame interval in Units of bps at which the data can be transmitted.
  BYTE  dwMaxBitRate[4];				//	Specifies the maximum bit rate at default compression quality and 
  										//		shortest frame interval in Units of bps at which the data can be transmitted.
  BYTE  dwMaxVideoFrameBufferSize[4];	//	Use of this field has been deprecated.
										//		Specifies the maximum number of bytes for a video (or still image) frame 
										//			the compressor will produce.
										//
										//		The dwMaxVideoFrameSize field of the Video Probe and Commit control 
										//			replaces this descriptor field. A value for this field shall be chosen 
										//			for compatibility with host software that implements an earlier version 
										//			of this specification.
  BYTE  dwDefaultFrameInterval[4];	//	Specifies the frame interval the device would like to indicate for use as 
  										//		a default. This must be a valid frame interval described in the fields below.
  BYTE  bFrameIntervalType;			//	Indicates how the frame interval can be programmed:
  										//		0: Continuous frame interval
										//		1..255: The number of discrete frame intervals supported (n)
  // Discrete Frame Intervals
  BYTE  dwFrameInterval0[4];			//	Shortest frame interval supported (at highest frame rate), in 100ns units.
  BYTE  dwFrameInterval1[4];
  BYTE  dwFrameInterval2[4];
  BYTE  dwFrameInterval3[4];			//	Longest frame interval supported (at lowest frame rate), in 100ns units.
} VS_MJPEG_FRAME_DSCR;
typedef VS_MJPEG_FRAME_DSCR * PVS_MJPEG_FRAME_DSCR;

/*Still Image Frame Descriptor*/
#if 1
//typedef struct STIL_IMA_FRAME_DSCR {
typedef struct STIL_IMA_YUVFRAME_DSCR {
	BYTE  bLength;
	BYTE  bDescriptorType;
	BYTE  bDescriptorSubType;
	BYTE  bEndpointAddress;
	BYTE  bNumImageSizePatterns; 		//5种？？？
	// WORD  wWidth1;
	BYTE  wWidth1[2];
	// WORD  wHeight1;
	BYTE  wHeight1[2];
	// WORD  wWidth2;
	BYTE  wWidth2[2];
	// WORD  wHieght2;
	BYTE  wHieght2[2];
	// WORD  wWidth3;
	BYTE  wWidth3[2];
	// WORD  wHeight3;
	BYTE  wHeight3[2];
	// WORD  wWidth4;
	BYTE  wWidth4[2];
	// WORD  wHeight4;
	BYTE  wHeight4[2];
	// WORD  wWidth5;
	BYTE  wWidth5[2];
	// WORD  wHeight5;
	BYTE  wHeight5[2];
	BYTE  bNumCompressionPattern;		//4种???
	BYTE  bCompression1;
	//BYTE  bCompression2;
	//BYTE  bCompression3;
	//BYTE  bCompression4;
} STIL_IMA_YUVFRAME_DSCR;
typedef STIL_IMA_YUVFRAME_DSCR * PSTIL_IMA_YUVFRAME_DSCR;
#else
/*
//typedef struct STIL_IMA_FRAME_DSCR {
typedef struct STIL_IMA_YUVFRAME_DSCR {
	BYTE  bLength;
	BYTE  bDescriptorType;
	BYTE  bDescriptorSubType;
	BYTE  bEndpointAddress;
	BYTE  bNumImageSizePatterns; 		//5种？？？
	WORD  wWidth1;
	WORD  wHeight1;
	WORD  wWidth2;
	WORD  wHieght2;
	WORD  wWidth3;
	WORD  wHeight3;
	WORD  wWidth4;
	WORD  wHeight4;
	WORD  wWidth5;
	WORD  wHeight5;
	WORD  wWidth6;
	WORD  wHeight6;
	WORD  wWidth7;
	WORD  wHeight7;
	WORD  wWidth8;
	WORD  wHeight8;
	WORD  wWidth9;
	WORD  wHeight9;
	BYTE  bNumCompressionPattern;		//4种???
	BYTE  bCompression1;
	//BYTE  bCompression2;
	//BYTE  bCompression3;
	//BYTE  bCompression4;
} STIL_IMA_YUVFRAME_DSCR;
typedef STIL_IMA_YUVFRAME_DSCR * PSTIL_IMA_YUVFRAME_DSCR;
*/
#endif

#if 1
typedef struct STIL_IMA_MJPEGFRAME_DSCR {
	BYTE  bLength;
	BYTE  bDescriptorType;
	BYTE  bDescriptorSubType;
	BYTE  bEndpointAddress;
	BYTE  bNumImageSizePatterns; 		//5种？？？
	// WORD  wWidth1;
	BYTE  wWidth1[2];
	// WORD  wHeight1;
	BYTE  wHeight1[2];
	// WORD  wWidth2;
	BYTE  wWidth2[2];
	// WORD  wHieght2;
	BYTE  wHieght2[2];
	// WORD  wWidth3;
	BYTE  wWidth3[2];
	// WORD  wHeight3;
	BYTE  wHeight3[2];
	// WORD  wWidth4;
	BYTE  wWidth4[2];
	// WORD  wHeight4;
	BYTE  wHeight4[2];
	// WORD  wWidth5;
	BYTE  wWidth5[2];
	// WORD  wHeight5;
	BYTE  wHeight5[2];
	BYTE  bNumCompressionPattern;		//1种???
	BYTE  bCompression1;
	BYTE  bCompression2;
	BYTE  bCompression3;
	BYTE  bCompression4;
} STIL_IMA_MJPEGFRAME_DSCR;
typedef STIL_IMA_MJPEGFRAME_DSCR * PSTIL_IMA_MJPEGFRAME_DSCR;
#else
/*
typedef struct STIL_IMA_MJPEGFRAME_DSCR {
	BYTE  bLength;
	BYTE  bDescriptorType;
	BYTE  bDescriptorSubType;
	BYTE  bEndpointAddress;
	BYTE  bNumImageSizePatterns; 		//5种？？？
	WORD  wWidth1;
	WORD  wHeight1;
	WORD  wWidth2;
	WORD  wHieght2;
	WORD  wWidth3;
	WORD  wHeight3;
	WORD  wWidth4;
	WORD  wHeight4;
	WORD  wWidth5;
	WORD  wHeight5;
	WORD  wWidth6;
	WORD  wHeight6;
	WORD  wWidth7;
	WORD  wHieght7;
	WORD  wWidth8;
	WORD  wHeight8;
	WORD  wWidth9;
	WORD  wHeight9;
	BYTE  bNumCompressionPattern;		//1种???
	BYTE  bCompression1;
	BYTE  bCompression2;
	BYTE  bCompression3;
	BYTE  bCompression4;
} STIL_IMA_MJPEGFRAME_DSCR;
typedef STIL_IMA_MJPEGFRAME_DSCR * PSTIL_IMA_MJPEGFRAME_DSCR;
*/
#endif


/*Color Matching Descriptor*/
typedef struct COL_MATCH_DSCR {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  BYTE  bColorPrimaries;
  BYTE  bTransferCharacteristics;
  BYTE  bMatrixCoefficients ;
  } COL_MATCH_DSCR;
typedef COL_MATCH_DSCR * PCOL_MATCH_DSCR;

/*Standard VS Isochronous Video Data Endpoint Descriptor*/
typedef struct VS_EPISO_DSCR {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bEndpointAddress;
  BYTE  bmAttributes;
  BYTE  wMaxPacketSize;
  BYTE  bInterval ;
  } VS_EPISO_DSCR;
typedef VS_EPISO_DSCR * PVS_EPISO_DSCR;

/*Standard VS Bulk Video Data Endpoint Descriptor*/
typedef struct VS_EPBULK_DSCR {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bEndpointAddress;
  BYTE  bmAttributes;
  BYTE  wMaxPacketSize;
  BYTE  bInterval ;
  } VS_EPBULK_DSCR;
typedef VS_EPBULK_DSCR * PVS_EPBULK_DSCR;


/* Audio Association Desc */
typedef struct AUD_IAD_DESC {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  WORD  bcdUVC;
  WORD  wTotalLength;
  BYTE  bInCollection;
  BYTE  baInterfaceNr;
  } AUD_IAD_DESC;
typedef AUD_IAD_DESC * PAUD_IAD_DESC;

/* Class-Specific AC Interface Desc */
typedef struct CS_ACINTR_DESC {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  WORD  bcdUVC;
  WORD  wTotalLength;
  BYTE  bInCollection;
  BYTE  baInterfaceNr;
  } CS_ACINTR_DESC;
typedef CS_ACINTR_DESC * PCS_ACINTR_DESC;

/* Input Terminal Desc */
typedef struct IN_TERM_DESC {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  BYTE  bTerminalID;
  WORD  wTerminalType;
  BYTE  bAssocTerminal;
  BYTE  bNrChannels;
  WORD  wChannelConfig;
  BYTE  iChannelNames;
  BYTE  iTerminal;
  } IN_TERM_DESC;
typedef IN_TERM_DESC * PIN_TERM_DESC;

/* Feature Unit Desc */
typedef struct FEA_UNIT_DESC {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  BYTE  bUnitID;
  BYTE  bSourceID;
  BYTE  bControlSize;
  BYTE  bmaControls;
  BYTE  iFeature;
  } FEA_UNIT_DESC;
typedef FEA_UNIT_DESC * PFEA_UNIT_DESC;

/* Class-Specific AS General Interface Desc */
typedef struct AS_FORMAT_TYPE_DESC {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  BYTE  bFormatType;
  BYTE  bNrChannels;
  BYTE  bSubFrameSize;
  BYTE  bBitResolution;
  BYTE  bSamFreqType;
  BYTE  tSamFreq0;
  BYTE  tSamFreq1;
  BYTE  tSamFreq2;
  } AS_FORMAT_TYPE_DESC;
typedef AS_FORMAT_TYPE_DESC * PAS_FORMAT_TYPE_DESC;

/* Audio Standard Ep Desc */
typedef struct AUD_STD_EP_DESC {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  BYTE  bmAttributes;
  WORD  wMaxPacketSize;
  BYTE  bInterval;
  BYTE  bRefresh;
  BYTE  bSynchAddress;
  } AUD_STD_EP_DESC;
typedef AUD_STD_EP_DESC * PAUD_STD_EP_DESC;

/* Audio Class-Specific Iso Ep Desc */
typedef struct AUD_CS_EP_DESC {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  BYTE  bmAttributes;
  BYTE  bLockDelayUnits;
  WORD  wLockDelay;
  } AUD_CS_EP_DESC;
typedef AUD_CS_EP_DESC * PAUD_CS_EP_DESC;


typedef struct PROBE_COMMIT_CONTROL {
  // WORD  bmHint;
  BYTE  bmHint[2];
  BYTE  bFormatIndex;
  BYTE  bFrameIndex;
  DWORD  dwFrameInterval;
  WORD  wKeyFrameRate;
  WORD  wPFrameRate;
  WORD  wCompQuality;
  WORD  wCompWindowSize;
  WORD  wDelay;
  DWORD  dwMaxVideoFrameSize;
  DWORD  dwMaxPayloadTransferSize;
  DWORD  dwClockFrequency;
  BYTE  bmFramingInfo;
  BYTE  bPreferedVersion;
  BYTE  bMinVersion;
  BYTE  bMaxVersion;
  } PROBE_COMMIT_CONTROL;

typedef struct AUDIO_IAD {
  BYTE bLength;
  BYTE bDescriptorType;
  BYTE bDescriptorSubtype;
  BYTE a;
  BYTE b;
  BYTE c;
  BYTE d;
  BYTE e;
} AUDIO_IAD;

typedef struct CS_ACHEAD_DSCR {
  BYTE bLength;
  BYTE bDescriptorType;
  BYTE bDescriptorSubtype;
//  WORD bcdADC;
  BYTE bcdADC[2];
//  WORD wTotalLength;
  BYTE wTotalLength[2];
  BYTE bInCollection;
  BYTE baInterfaceNr;
} CS_ACHEAD_DSCR;

typedef struct AIN_TERM_DSCR {
  BYTE bLength;
  BYTE bDescriptorType;
  BYTE bDescriptorSubtype;
  BYTE bTerminalID;
  // WORD wTerminalType;
  BYTE wTerminalType[2];
  BYTE bAssocTerminal;
  BYTE bNrChannels;
  // WORD wChannelConfig;
  BYTE wChannelConfig[2];
  BYTE iChannelNames;
  BYTE iTerminal;
} AIN_TERM_DSCR;

typedef struct FEA_UNIT_DSCR {
  BYTE bLength;
  BYTE bDescriptorType;
  BYTE bDescriptorSubtype;
  BYTE bUnitID;
  BYTE bSourceID;
  BYTE bControlSize;
  BYTE bmaControls0;
  BYTE bmaControls1;
  BYTE bmaControls2;
  BYTE iFeature;
} FEA_UNIT_DSCR;

typedef struct AOUT_TERM_DSCR {
  BYTE bLength;
  BYTE bDescriptorType;
  BYTE bDescriptorSubtype;
  BYTE bTerminalID;
  // WORD wTerminalType;
  BYTE wTerminalType[2];
  BYTE bAssocTerminal;
  BYTE bSourceID;
  BYTE iTerminal;
} AOUT_TERM_DSCR;

typedef struct CS_AS_GENE {
  BYTE bLength;
  BYTE bDescriptorType;
  BYTE bDescriptorSubtype;
  BYTE bTerminalLink;
  BYTE bDelay;
  // WORD wFormatTag;
  BYTE wFormatTag[2];
} CS_AS_GENE;

typedef struct AS_FORMAT_DSCR {
  BYTE bLength;
  BYTE bDescriptorType;
  BYTE bDescriptorSubtype;
  BYTE bFormatyType;
  BYTE bNrChannels;
  BYTE bSubFrameSize;
  BYTE bBitResolution;
  BYTE bSamFreqType;
/*  BYTE tSamFreq1;
  BYTE tSamFreq2;
  BYTE tSamFreq3;
  BYTE tSamFreq4;
  BYTE tSamFreq5;
  BYTE tSamFreq6;
  BYTE tSamFreq7;
  BYTE tSamFreq8;
  BYTE tSamFreq9;
  BYTE tSamFreq10;
  BYTE tSamFreq11;
  BYTE tSamFreq12;
  BYTE tSamFreq13;
  BYTE tSamFreq14;
  BYTE tSamFreq15;
  BYTE tSamFreq16;
  BYTE tSamFreq17;
  BYTE tSamFreq18;*/
  BYTE tSamFreq19;
  BYTE tSamFreq20;
  BYTE tSamFreq21;
} AS_FORMAT_DSCR;

typedef struct AUD_EP_DSCR {
  BYTE bLength;
  BYTE bDescriptorType;
  BYTE bEndpointAddress;
  BYTE bmAttributes;
  // WORD wMaxPacketSize;
  BYTE wMaxPacketSize[2];
  // WORD wInterval;
  BYTE wInterval[2];
  BYTE bSyncAddress;
} AUD_EP_DSCR;

typedef struct CS_EP_DSCR {
  BYTE bLength;
  BYTE bDescriptorType;
  BYTE bDescriptorSubtype;
  BYTE bmAttributes;
  BYTE bLockDelayUnits;
  // WORD wLockDelay;
  BYTE wLockDelay[2];
} CS_EP_DSCR;

/************ HS Descriptors **********************/
#if 0
typedef struct M_CFG_HS1 {
  STD_CFG_DSCR	      stdcfg;		 /* Required Standard Configuration Descriptor */
  IF_ASS_DSCR         ifass;       /*Interface Association Deccriptor*/

//IF0 : VC
  STD_IF_DSCR	      stdvc;	     /* VideoControl Interface Descriptor */
  CS_VCHEAD_DSCR	  vchead;	/* Class-specific VC Interface Header Descriptor */
  CAM_TERM_DSCR	      camterm;	    /* Camera Terminal Descriptor */
  PRO_UNIT_DSCR       prounit;	    /* Processing Unit Descriptor */
  OUT_TERM_DSCR       outterm;	    /* Output Terminal Descriptor */
  STD_EP_DSCR	      stdep0;	        /* Endpoint Descriptor for VideoControl Interface */
  CS_VCEP_DSCR        csvcep;            /* Class-specific VC Interrupt Endpoint Descriptor */

//IF1 : VS alt0
  STD_IF_DSCR	      stdvs0;	      /* VideoStream Interface Alternate 0 Descriptor */

  CS_VSINHEAD_DSCR1     vsinhead;	     /*Class-specific VS Interface Input Header Descriptor*/
  VS_YUV_FORMAT_DSCR     yuvfmt;

  VS_MJPEG_FRAME_DSCR    yuvframe1;
  VS_MJPEG_FRAME_DSCR    yuvframe2;
  VS_MJPEG_FRAME_DSCR    yuvframe3;
  VS_MJPEG_FRAME_DSCR    yuvframe4;
  VS_MJPEG_FRAME_DSCR    yuvframe5;
  VS_MJPEG_FRAME_DSCR    yuvframe6;
  VS_MJPEG_FRAME_DSCR    yuvframe7;
  VS_MJPEG_FRAME_DSCR    yuvframe8;
  #if 1
  VS_MJPEG_FRAME_DSCR    yuvframe9;
  VS_MJPEG_FRAME_DSCR    yuvframe10;
  VS_MJPEG_FRAME_DSCR    yuvframe11;
  VS_MJPEG_FRAME_DSCR    yuvframe12;
  VS_MJPEG_FRAME_DSCR    yuvframe13;
  VS_MJPEG_FRAME_DSCR    yuvframe14;
  VS_MJPEG_FRAME_DSCR    yuvframe15;
  #endif

#if 1	//mjpeg
  VS_MJPEG_FORMAT_DSCR   mjpegfmt;

  VS_MJPEG_FRAME_DSCR    mjpegframe1;
  #if 1
  VS_MJPEG_FRAME_DSCR    mjpegframe2;
  VS_MJPEG_FRAME_DSCR    mjpegframe3;
  VS_MJPEG_FRAME_DSCR    mjpegframe4;
  VS_MJPEG_FRAME_DSCR    mjpegframe5;
  VS_MJPEG_FRAME_DSCR    mjpegframe6;
  VS_MJPEG_FRAME_DSCR    mjpegframe7;
  VS_MJPEG_FRAME_DSCR    mjpegframe8;
  VS_MJPEG_FRAME_DSCR    mjpegframe9;
  VS_MJPEG_FRAME_DSCR    mjpegframe10;
  VS_MJPEG_FRAME_DSCR    mjpegframe11;
  VS_MJPEG_FRAME_DSCR    mjpegframe12;
  VS_MJPEG_FRAME_DSCR    mjpegframe13;
  VS_MJPEG_FRAME_DSCR    mjpegframe14;
  VS_MJPEG_FRAME_DSCR    mjpegframe15;
  #endif
#endif

  STIL_IMA_FRAME_DSCR    stillimage;
  COL_MATCH_DSCR         colordscr;	             /*Color Matching Descriptor*/

//IF1 : VS alt1
  STD_IF_DSCR	      stdvs1;	        /* VideoStream Interface Alternate 1 Descriptor */
  STD_EP_DSCR	      stdisoep;

//**************** Audio ************//
  AUDIO_IAD           audio;		    /* AUDIO IAD */

//IF2 : AC
  STD_IF_DSCR         stdac;            /* AUDIO CONTROL INTERFACE */	
  CS_ACHEAD_DSCR      achead;           /* AUDIO CONTROL CS */
  AIN_TERM_DSCR       audiointerm;      /* INPUT TERMINAL */
  FEA_UNIT_DSCR       feaunit;          /* Feature Unit Desc */	
  AOUT_TERM_DSCR      aouttrerm;        /* OUTPUT TERMINAL */

//IF3 : AS alt0
  STD_IF_DSCR         stdas0;           /* AS INTERFACE SET 0 */

//IF3 : AS alt1
  STD_IF_DSCR         stdas1;           /* AS INTERFACE SET 1 */

  CS_AS_GENE          asgene;           /* CS AS GENERAL */	
  AS_FORMAT_DSCR      asformat;         /* FORMAT TYPE */
  AUD_EP_DSCR         stdasep;          /* STD EP */
  CS_EP_DSCR          asep;             /*CS_ENDPOINT */
  } M_CFG_HS1;

typedef M_CFG_HS1 * PM_CFG_HS1;
#endif

#if 0
/***************** FS Descriptors ********************/
/* Configuration , full-speed */
typedef struct M_CFG_FS1 {
  STD_CFG_DSCR	      stdcfg;		 /* Required Standard Configuration Descriptor */
  IF_ASS_DSCR         ifass;       /*Interface Association Deccriptor*/

  STD_IF_DSCR	      stdvc;	     /* VideoControl Interface Descriptor */
  CS_VCHEAD_DSCR	  vchead;	/* Class-specific VC Interface Header Descriptor */
  CAM_TERM_DSCR	      camterm;	    /* Camera Terminal Descriptor */
  PRO_UNIT_DSCR       prounit;	    /* Processing Unit Descriptor */
  OUT_TERM_DSCR       outterm;	    /* Output Terminal Descriptor */
  STD_EP_DSCR	      stdep0;	        /* Endpoint Descriptor for VideoControl Interface */
  CS_VCEP_DSCR        csvcep;            /* Class-specific VC Interrupt Endpoint Descriptor */

  STD_IF_DSCR	      stdvs0;	      /* VideoStream Interface Alternate 0 Descriptor */
  CS_VSINHEAD_DSCR1     vsinhead;	     /*Class-specific VS Interface Input Header Descriptor*/
  VS_MJPEG_FORMAT_DSCR   mjpegfmt;          /*Video Streaming MJPEG Format Type Descriptor*/
  VS_MJPEG_FRAME_DSCR    mjpegframe;         /*Video Streaming MJPEG Frame Type Descriptor*/
  VS_MJPEG_FRAME_DSCR    mjpegframe1;         /*Video Streaming MJPEG Frame Type Descriptor*/
  VS_MJPEG_FRAME_DSCR    mjpegframe2;
  VS_MJPEG_FRAME_DSCR    mjpegframe3;         /*Video Streaming MJPEG Frame Type Descriptor*/
  VS_MJPEG_FRAME_DSCR    mjpegframe4;         /*Video Streaming MJPEG Frame Type Descriptor*/
  STIL_IMA_FRAME_DSCR    stillimage;
  COL_MATCH_DSCR         colordscr;	             /*Color Matching Descriptor*/

  STD_IF_DSCR	      stdvs1;	        /* VideoStream Interface Alternate 1 Descriptor */
  STD_EP_DSCR	      stdisoep;
  } M_CFG_FS1;

typedef M_CFG_FS1 * PM_CFG_FS1;

#endif

/*Class-specific VS Interface Input Header Descriptor*/
/*
typedef struct CS_VSINHEAD_YUV_MJPEG_DSCR {
  BYTE  bLength;
  BYTE  bDescriptorType;
  BYTE  bDescriptorSubType;
  BYTE  bNumFormats;
  WORD  wTotalLength;
  BYTE  bEndpointAddress;
  BYTE  bmInfo;
  BYTE  bTerminalLink;
  BYTE  bStillCaptureMethod;
  BYTE  bTriggerSupport;
  BYTE  bTriggerUsage;
  BYTE  bControlSize;
  BYTE  bmaControls1;		//for video payload format 1
  BYTE  bmaControls2;		//for video payload format 2
  } CS_VSINHEAD_YUV_MJPEG_DSCR;
typedef CS_VSINHEAD_YUV_MJPEG_DSCR * PCS_VSINHEAD_YUV_MJPEG_DSCR;
*/

#pragma pack()

#endif
