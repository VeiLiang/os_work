//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xmmci.h
//	  constant，macro & basic typedef definition of MCI
//
//	Revision history
//
//		2011.06.08	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_MCI_H_
#define _XM_MCI_H_

#include <xm_proj_define.h>

#if defined (__cplusplus)
	extern "C"{
#endif

#define	XMMCIERRORCODE_SUCCESS				((LONG)(0))		
#define	XMMCIERRORCODE_DEVICEBUSY			((LONG)(-1))		//	设备忙。
																				// 播放/记录设备均只有一个实例		
#define	XMMCIERRORCODE_INVALIDFORMAT		((LONG)(-2))		// 无效WAVEFORMAT格式信息
#define	XMMCIERRORCODE_INVALIDPARAMETER	((LONG)(-3))		// 无效参数
#define	XMMCIERRORCODE_LOWVOLTAGE		((LONG)(-4))		// 操作失败，系统电压低
#define	XMMCIERRORCODE_DEVICECLOSED		((LONG)(-5))		// 操作失败，设备已关闭		

#define	XMMCIDEVICE_PLAY			0x00000001		// 播放设备
#define	XMMCIDEVICE_RECORD		0x00000002		// 记录设备0
#define	XMMCIDEVICE_RECORD1		0x00000004		// 记录设备1
#define	XMMCIDEVICE_PLAY			0x00000001		// 播放设备
		
// MCI设备定义
enum {
	XMMCI_DEV_0 = 0,		// I2S0
	XMMCI_DEV_1,			// I2S1
	XMMCI_DEV_COUNT
};


typedef struct tagXMWAVEFORMAT { 
	WORD		wChannels; 							// 通道数	
	WORD		wBitsPerSample;					// 指定采样点bit数（8bit或16bit）
	DWORD		dwSamplesPerSec;					// 8K，11025，16K，22050，24K，32K, 44100, 48000等
} XMWAVEFORMAT; 

// WAVE设备驱动初始化(仅由内核调用)
void XM_WaveInit (void);
// WAVE设备驱动退出(仅由内核调用)
void XM_WaveExit (void);


// 打开播放/记录设备
// dwDevice 指定设备，可以是XMMCIDEVICE_PLAY、XMMCIDEVICE_RECORD或者组合XMMCIDEVICE_PLAY|XMMCIDEVICE_RECORD
LONG XM_WaveOpen (DWORD dwDevice, XMWAVEFORMAT *lpWaveFormat);

// 关闭播放/记录设备
// dwDevice 指定设备，可以是XMMCIDEVICE_PLAY、XMMCIDEVICE_RECORD或者组合XMMCIDEVICE_PLAY|XMMCIDEVICE_RECORD
LONG XM_WaveClose (DWORD dwDevice);

// 向播放设备写入PCM采样数据
LONG XM_WaveWrite	(DWORD dwDevice, WORD *lpPcmSample, LONG cbPcmSample);

// 从录音设备读取PCM采样数据
LONG XM_WaveRead	(DWORD dwDevice, WORD *lpPcmSample, LONG cbPcmSample);



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_MCI_H_