//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_proj_define.h
//	  constant��macro & basic typedef definition of X-Mini System
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

#define	_XMSYS_VIDEOITEM_SUPPORT_			// ��Ƶ��֧��

// ����ң����֧��
#define	_XMSYS_REMOTE_CONTROL_SUPPORT_		
		
//#define	_XMSYS_I2C_USE_OLD_METHOD_		1
		
		
// UARTͨ�Ŷ˿ڶ��� 
#define	_XMSYS_UART_SOCKET_OLD_			1
#define	_XMSYS_UART_SOCKET_NEW_			2
		
#define	_XMSYS_UART_SOCKET_				_XMSYS_UART_SOCKET_NEW_
//#define	_XMSYS_UART_SOCKET_				_XMSYS_UART_SOCKET_OLD_		

// ����I2Cģʽ
#define	_XMSYS_I2C_SOFTWARE_				1	// ���I2Cģʽ
#define	_XMSYS_I2C_HARDWARE_				2	// Ӳ��I2Cģʽ
		
//#define	_XMSYS_I2C_		_XMSYS_I2C_SOFTWARE_	// ʹ�����I2C���� GPIO_I2C
#define	_XMSYS_I2C_		_XMSYS_I2C_HARDWARE_	// ʹ��Ӳ��I2C����
		
#define	_XMSYS_USB_USAGE_DEVICE_		1
#define	_XMSYS_USB_USAGE_HOST_			2
		
//#define	_XMSYS_USB_USAGE_					_XMSYS_USB_USAGE_HOST_			
#define	_XMSYS_USB_USAGE_					_XMSYS_USB_USAGE_DEVICE_					
		
		
#define	_NEW_SDMMC_DMA_DRIVER_			// ֧���µ�SDMMC DMA����
		
//#define	_XMSYS_SDMMC_EVENT_DRIVEN_				// �¼�����

// ****** �ļ�ϵͳ֧�ֺ궨�� *******		
		
#define	_XMSYS_FS_CACHE_SIZE_				0x100000		// 4MB�ļ�ϵͳCache		
		
#define	_XMSYS_FS_SDMMC_TYPE_POWERPAC_	1		// SDMMCʹ��PowerPac����
#define	_XMSYS_FS_SDMMC_TYPE_ARKMICRO_	2		// SDMMCʹ��ArkMicro����

// ѡ��ʹ��ArkMicro��SDMMC����		
#define	_XMSYS_FS_SDMMC_TYPE_				_XMSYS_FS_SDMMC_TYPE_ARKMICRO_

//#define	_XMSYS_FS_NANDFLASH_SUPPORT_				// �ļ�ϵͳ֧��NandFlash
#define	_XMSYS_FS_SDMMC_SUPPORT_					// �ļ�ϵͳ֧��SDMMC

#if _XMSYS_USB_USAGE_ == _XMSYS_USB_USAGE_HOST_		
#define	_XMSYS_FS_UDISK_SUPPORT_					// �ļ�ϵͳ֧��U��
#endif
		
#define	_XMSYS_DMA_SUPPORT_			// DMA֧��ʹ��
												//		ͬʱʹ��SDMMC/UART DMA֧��	
		
// *********************************************************************************		

#define	_XMSYS_VIDEO_SUPPORT_
// *********************************************************************************		
//                     ��ʾϵͳ�궨�� 
#ifdef _XMSYS_VIDEO_SUPPORT_
#define	_XMSYS_VIDEO_LCD_RGB_SUPPORT_			// ֧��LCDC RGB����Ƶ���		
#define	_XMSYS_VIDEO_CVBS_NTSC_SUPPORT_			// ֧��CVBS NTSC��Ƶ���		
#define	_XMSYS_VIDEO_CVBS_PAL_SUPPORT_			// ֧��CVBS PAL��Ƶ���		
#define	_XMSYS_VIDEO_YPBPR_480I_SUPPORT			// ֧��YPbPr 480i(����)��Ƶ���		
#define	_XMSYS_VIDEO_YPBPR_480P_SUPPORT			// ֧��YPbPr 480p(����)��Ƶ���
#define	_XMSYS_VIDEO_YPBPR_576I_SUPPORT			// ֧��YPbPr 576i(����)��Ƶ���		
#define	_XMSYS_VIDEO_YPBPR_576P_SUPPORT			// ֧��YPbPr 576p(����)��Ƶ���
#define	_XMSYS_VIDEO_VGA_640X480_SUPPORT_		// ֧��VGA 640X480��Ƶ���		
#define	_XMSYS_VIDEO_VGA_800X600_SUPPORT_		// ֧��VGA 800X600��Ƶ���		
#endif	

// *********************************************************************************		
		
		
// ****** UVC Camera ֧�ֺ궨�� *******			
#define	_XMSYS_UVC_CAMERA_TYPE_ISO_				1		// ISOģʽUVC Cameraʵ��
#define	_XMSYS_UVC_CAMERA_TYPE_BULK_				2		// BulkģʽUVC Cameraʵ�� 

#define	_XMSYS_UVC_CAMERA_TYPE_						_XMSYS_UVC_CAMERA_TYPE_BULK_		
// *********************************************************************************		
		
		
// IIS ADC 0 ���䷽ʽ����
// ʹ��DMAģʽ����ADC����		
#define	_XMSYS_IISADC_0_TYPE_DMA_			0	
// ʹ���ж�ģʽ����ADC����
#define	_XMSYS_IISADC_0_TYPE_INT_			1
#define	_XMSYS_IISADC_0_TYPE_				_XMSYS_IISADC_0_TYPE_INT_
//#define	_XMSYS_IISADC_0_TYPE_				_XMSYS_IISADC_0_TYPE_DMA_		
		
#define	_XMSYS_RTC_HARDWARE_					0								// ʹ��Ӳ��RTCʱ����ΪRTCʱ�� 
#define	_XMSYS_RTC_SOFTWARE_					1								// ʹ�������ʱ��ģ��RTCʱ��
//#define	_XMSYS_RTC_								_XMSYS_RTC_SOFTWARE_		// ϵͳʹ�����RTC
#define	_XMSYS_RTC_								_XMSYS_RTC_HARDWARE_		// ϵͳʹ��Ӳ��RTC
		

		
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
		
#define	_XM_PROJ_1_SENSOR_1080P				1			// 1·1080P
#define	_XM_PROJ_2_SENSOR_1080P_CVBS		2			// 2·1080P+CVBS
		
		
#define	_XM_FFMPEG_CODEC_AVI_		1
#define	_XM_FFMPEG_CODEC_MKV_		2
#define	_XM_FFMPEG_CODEC_MOV_		3

#define	_XM_FFMPEG_CODEC_				_XM_FFMPEG_CODEC_AVI_		
//#define	_XM_FFMPEG_CODEC_				_XM_FFMPEG_CODEC_MKV_		
//#define	_XM_FFMPEG_CODEC_				_XM_FFMPEG_CODEC_MOV_				
		

#ifndef _XM_PROJ_
#define	_XM_PROJ_				_XM_PROJ_2_SENSOR_1080P_CVBS			// ȱʡ1·1080P
#endif

	
// 4��D1ͼ�����źϳɿ���		
#define	SCALE_COMPOSE_1280X720		1
#define	SCALE_COMPOSE_1280X960		2
		
#define	SCALE_COMPOSE_SIZE			SCALE_COMPOSE_1280X720
		
#define	D1_SENSOR_TYPE_PAL			1
#define	D1_SENSOR_TYPE_NTSC			2

// D1_SENSOR_TYPE�����ڶ���D1����ͷ�ĳ�ʼ��ʽ��PAL����NTSC		
#define	D1_SENSOR_TYPE					D1_SENSOR_TYPE_NTSC		// DVR��4D1�г���¼����Ŀ NTSC����
//#define	D1_SENSOR_TYPE					D1_SENSOR_TYPE_PAL		// DVR��4D1�г���¼����Ŀ PAL����
		
#if _XM_PROJ_ == _XM_PROJ_1_SENSOR_1080P
		
// **************** 1������ͷ 1080P *******************
#define	XMSYS_SENSOR_COUNT							1

// ͨ��0����ͷ֡�ֽڴ�С����
#define	XMSYS_CH0_SENSOR_FRAME_WIDTH					1920
#define	XMSYS_CH0_SENSOR_FRAME_HEIGHT				1080

#define	XMSYS_ITU656_IN_SUPPORT					    0

#elif _XM_PROJ_ == _XM_PROJ_2_SENSOR_1080P_CVBS

// **************** 2������ͷ 1080P/VGA *******************
#define	XMSYS_SENSOR_COUNT							2

// ����ͷ֡�ֽڴ�С����		
// ͨ��0 ��1080P��
#define	XMSYS_CH0_SENSOR_FRAME_WIDTH					640//1920
#define	XMSYS_CH0_SENSOR_FRAME_HEIGHT				480//1080

// ͨ��1���ڶ�������ͷD1 NTSC��ʽ)
#define	XMSYS_CH1_SENSOR_FRAME_WIDTH					720
#define	XMSYS_CH1_SENSOR_FRAME_HEIGHT				480

#define	XMSYS_ITU656_IN_SUPPORT						 0

#else

#error please config sensor count & its frame size definition
		
#endif


// ͨ��0����ͷ֡�ֽڴ�С����
#define	XMSYS_CH0_SENSOR_FRAME_SIZE				(XMSYS_CH0_SENSOR_FRAME_WIDTH*(XMSYS_CH0_SENSOR_FRAME_HEIGHT + 0)*3/2)
// ͨ��1����ͷ֡�ֽڴ�С����
#define	XMSYS_CH1_SENSOR_FRAME_SIZE				(XMSYS_CH1_SENSOR_FRAME_WIDTH*(XMSYS_CH1_SENSOR_FRAME_HEIGHT + 0)*3/2)

#if defined(_WINDOWS)
#define	__no_init
#endif
		
#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_PROJ_DEFINE_H_
