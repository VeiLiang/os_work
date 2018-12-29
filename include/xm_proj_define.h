//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_proj_define.h
//	  constant，macro & basic typedef definition of X-Mini System
//
//	Revision history
//
//		2010.10.18	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_PROJ_DEFINE_H_
#define _XM_PROJ_DEFINE_H_

#if defined (__cplusplus)
	extern "C"{
#endif
		
		
#define	_XMSYS_APP_SUPPORT_

#define	CONFIG_ARKN141_ASIC
//#define	ASIC_LCD_480X272		1
		
#define	SYS_IAR_RTOS
		
#define	_XMSYS_CODEC_SUPPORT_		// H264/JPEG encoder/decoder support

#define	_XMSYS_VIDEOITEM_SUPPORT_			// 视频项支持

// 定义遥控器支持
#define	_XMSYS_REMOTE_CONTROL_SUPPORT_		
		
//#define	_XMSYS_I2C_USE_OLD_METHOD_		1
		
		
// UART通信端口定义 
#define	_XMSYS_UART_SOCKET_OLD_			1
#define	_XMSYS_UART_SOCKET_NEW_			2
		
#define	_XMSYS_UART_SOCKET_				_XMSYS_UART_SOCKET_NEW_
//#define	_XMSYS_UART_SOCKET_				_XMSYS_UART_SOCKET_OLD_		

// 定义I2C模式
#define	_XMSYS_I2C_SOFTWARE_				1	// 软件I2C模式
#define	_XMSYS_I2C_HARDWARE_				2	// 硬件I2C模式
		
//#define	_XMSYS_I2C_		_XMSYS_I2C_SOFTWARE_	// 使用软件I2C访问 GPIO_I2C
#define	_XMSYS_I2C_		_XMSYS_I2C_HARDWARE_	// 使用硬件I2C访问
		
#define	_XMSYS_USB_USAGE_DEVICE_		1
#define	_XMSYS_USB_USAGE_HOST_			2
		
//#define	_XMSYS_USB_USAGE_					_XMSYS_USB_USAGE_HOST_			
#define	_XMSYS_USB_USAGE_					_XMSYS_USB_USAGE_DEVICE_					
		
		
#define	_NEW_SDMMC_DMA_DRIVER_			// 支持新的SDMMC DMA驱动
		
//#define	_XMSYS_SDMMC_EVENT_DRIVEN_				// 事件驱动

// ****** 文件系统支持宏定义 *******		
		
#define	_XMSYS_FS_CACHE_SIZE_				0x100000		// 4MB文件系统Cache		
		
#define	_XMSYS_FS_SDMMC_TYPE_POWERPAC_	1		// SDMMC使用PowerPac驱动
#define	_XMSYS_FS_SDMMC_TYPE_ARKMICRO_	2		// SDMMC使用ArkMicro驱动

// 选择使用ArkMicro的SDMMC驱动		
#define	_XMSYS_FS_SDMMC_TYPE_				_XMSYS_FS_SDMMC_TYPE_ARKMICRO_

//#define	_XMSYS_FS_NANDFLASH_SUPPORT_				// 文件系统支持NandFlash
#define	_XMSYS_FS_SDMMC_SUPPORT_					// 文件系统支持SDMMC

#if _XMSYS_USB_USAGE_ == _XMSYS_USB_USAGE_HOST_		
#define	_XMSYS_FS_UDISK_SUPPORT_					// 文件系统支持U盘
#endif
		
#define	_XMSYS_DMA_SUPPORT_			// DMA支持使能
												//		同时使能SDMMC/UART DMA支持	
		
// *********************************************************************************		

#define	_XMSYS_VIDEO_SUPPORT_
// *********************************************************************************		
//                     显示系统宏定义 
#ifdef _XMSYS_VIDEO_SUPPORT_
#define	_XMSYS_VIDEO_LCD_RGB_SUPPORT_			// 支持LCDC RGB屏视频输出		
#define	_XMSYS_VIDEO_CVBS_NTSC_SUPPORT_			// 支持CVBS NTSC视频输出		
#define	_XMSYS_VIDEO_CVBS_PAL_SUPPORT_			// 支持CVBS PAL视频输出		
#define	_XMSYS_VIDEO_YPBPR_480I_SUPPORT			// 支持YPbPr 480i(隔行)视频输出		
#define	_XMSYS_VIDEO_YPBPR_480P_SUPPORT			// 支持YPbPr 480p(逐行)视频输出
#define	_XMSYS_VIDEO_YPBPR_576I_SUPPORT			// 支持YPbPr 576i(隔行)视频输出		
#define	_XMSYS_VIDEO_YPBPR_576P_SUPPORT			// 支持YPbPr 576p(逐行)视频输出
#define	_XMSYS_VIDEO_VGA_640X480_SUPPORT_		// 支持VGA 640X480视频输出		
#define	_XMSYS_VIDEO_VGA_800X600_SUPPORT_		// 支持VGA 800X600视频输出		
#endif	

// *********************************************************************************		
		
		
// ****** UVC Camera 支持宏定义 *******			
#define	_XMSYS_UVC_CAMERA_TYPE_ISO_				1		// ISO模式UVC Camera实现
#define	_XMSYS_UVC_CAMERA_TYPE_BULK_				2		// Bulk模式UVC Camera实现 

#define	_XMSYS_UVC_CAMERA_TYPE_						_XMSYS_UVC_CAMERA_TYPE_BULK_		
// *********************************************************************************		
		
		
// IIS ADC 0 传输方式定义
// 使用DMA模式传输ADC数据		
#define	_XMSYS_IISADC_0_TYPE_DMA_			0	
// 使用中断模式传输ADC数据
#define	_XMSYS_IISADC_0_TYPE_INT_			1
#define	_XMSYS_IISADC_0_TYPE_				_XMSYS_IISADC_0_TYPE_INT_
//#define	_XMSYS_IISADC_0_TYPE_				_XMSYS_IISADC_0_TYPE_DMA_		
		
#define	_XMSYS_RTC_HARDWARE_					0								// 使用硬件RTC时钟作为RTC时钟 
#define	_XMSYS_RTC_SOFTWARE_					1								// 使用软件定时器模拟RTC时钟
//#define	_XMSYS_RTC_								_XMSYS_RTC_SOFTWARE_		// 系统使用软件RTC
#define	_XMSYS_RTC_								_XMSYS_RTC_HARDWARE_		// 系统使用硬件RTC
		

		
#define	_USB_DEV_0_						1
#define	_USB_DEV_1_						2
		

#define	_USB_DEV_		_USB_DEV_0_

		
#define	_PAYLOAD_PIPE_ISO_			1
#define	_PAYLOAD_PIPE_BULK_			2
		
#define	_PAYLOAD_PIPE_					_PAYLOAD_PIPE_BULK_
//#define	_PAYLOAD_PIPE_					_PAYLOAD_PIPE_ISO_		
	
#define	_VIDEO_EP_4_						1		
#define	_VIDEO_EP_5_						2
		
#define	_VIDEO_EP_						_VIDEO_EP_5_
//#define	_VIDEO_EP_						_VIDEO_EP_4_		
		
#if _VIDEO_EP_ == _VIDEO_EP_5_
#define	VIDEO_EP_INDEX					5
#define	VIDEO_EP_IN						(0x80 | VIDEO_EP_INDEX)
#elif _VIDEO_EP_ == _VIDEO_EP_4_
#define	VIDEO_EP_INDEX					4
#define	VIDEO_EP_IN						(0x80 | VIDEO_EP_INDEX)
#else
#error	please define VIDEO_EP_IN
#endif
		
		
		
//#define	_XM_VIDEO_OSD_SUPPORT_
		
#define	_XM_PROJ_1_SENSOR_1080P				1			// 1路1080P
#define	_XM_PROJ_2_SENSOR_1080P_CVBS		2			// 2路1080P+CVBS
		
		
#define	_XM_FFMPEG_CODEC_AVI_		1
#define	_XM_FFMPEG_CODEC_MKV_		2
#define	_XM_FFMPEG_CODEC_MOV_		3

#define	_XM_FFMPEG_CODEC_				_XM_FFMPEG_CODEC_AVI_		
//#define	_XM_FFMPEG_CODEC_				_XM_FFMPEG_CODEC_MKV_		
//#define	_XM_FFMPEG_CODEC_				_XM_FFMPEG_CODEC_MOV_				
		

#ifndef _XM_PROJ_
#define	_XM_PROJ_				_XM_PROJ_2_SENSOR_1080P_CVBS			// 缺省1路1080P
#endif

	
// 4个D1图像缩放合成控制		
#define	SCALE_COMPOSE_1280X720		1
#define	SCALE_COMPOSE_1280X960		2
		
#define	SCALE_COMPOSE_SIZE			SCALE_COMPOSE_1280X720
		
#define	D1_SENSOR_TYPE_PAL			1
#define	D1_SENSOR_TYPE_NTSC			2

// D1_SENSOR_TYPE仅用于定义D1摄像头的初始制式是PAL还是NTSC		
#define	D1_SENSOR_TYPE					D1_SENSOR_TYPE_NTSC		// DVR或4D1行车记录仪项目 NTSC测试
//#define	D1_SENSOR_TYPE					D1_SENSOR_TYPE_PAL		// DVR或4D1行车记录仪项目 PAL测试
		
#if _XM_PROJ_ == _XM_PROJ_1_SENSOR_1080P
		
// **************** 1个摄像头 1080P *******************
#define	XMSYS_SENSOR_COUNT							1

// 通道0摄像头帧字节大小定义
#define	XMSYS_CH0_SENSOR_FRAME_WIDTH					1920
#define	XMSYS_CH0_SENSOR_FRAME_HEIGHT				1080

#define	XMSYS_ITU656_IN_SUPPORT					    0

#elif _XM_PROJ_ == _XM_PROJ_2_SENSOR_1080P_CVBS

// **************** 2个摄像头 1080P/VGA *******************
#define	XMSYS_SENSOR_COUNT							2

// 摄像头帧字节大小定义		
// 通道0 （1080P）
#define	XMSYS_CH0_SENSOR_FRAME_WIDTH					640//1920
#define	XMSYS_CH0_SENSOR_FRAME_HEIGHT				480//1080

// 通道1（第二个摄像头D1 NTSC制式)
#define	XMSYS_CH1_SENSOR_FRAME_WIDTH					720
#define	XMSYS_CH1_SENSOR_FRAME_HEIGHT				480

#define	XMSYS_ITU656_IN_SUPPORT						 0

#else

#error please config sensor count & its frame size definition
		
#endif


// 通道0摄像头帧字节大小定义
#define	XMSYS_CH0_SENSOR_FRAME_SIZE				(XMSYS_CH0_SENSOR_FRAME_WIDTH*(XMSYS_CH0_SENSOR_FRAME_HEIGHT + 0)*3/2)
// 通道1摄像头帧字节大小定义
#define	XMSYS_CH1_SENSOR_FRAME_SIZE				(XMSYS_CH1_SENSOR_FRAME_WIDTH*(XMSYS_CH1_SENSOR_FRAME_HEIGHT + 0)*3/2)

#if defined(_WINDOWS)
#define	__no_init
#endif
		
#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_PROJ_DEFINE_H_
