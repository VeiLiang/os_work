//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xmmci.h
//	  constant��macro & basic typedef definition of MCI
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
#define	XMMCIERRORCODE_DEVICEBUSY			((LONG)(-1))		//	�豸æ��
																				// ����/��¼�豸��ֻ��һ��ʵ��		
#define	XMMCIERRORCODE_INVALIDFORMAT		((LONG)(-2))		// ��ЧWAVEFORMAT��ʽ��Ϣ
#define	XMMCIERRORCODE_INVALIDPARAMETER	((LONG)(-3))		// ��Ч����
#define	XMMCIERRORCODE_LOWVOLTAGE		((LONG)(-4))		// ����ʧ�ܣ�ϵͳ��ѹ��
#define	XMMCIERRORCODE_DEVICECLOSED		((LONG)(-5))		// ����ʧ�ܣ��豸�ѹر�		

#define	XMMCIDEVICE_PLAY			0x00000001		// �����豸
#define	XMMCIDEVICE_RECORD		0x00000002		// ��¼�豸0
#define	XMMCIDEVICE_RECORD1		0x00000004		// ��¼�豸1
#define	XMMCIDEVICE_PLAY			0x00000001		// �����豸
		
// MCI�豸����
enum {
	XMMCI_DEV_0 = 0,		// I2S0
	XMMCI_DEV_1,			// I2S1
	XMMCI_DEV_COUNT
};


typedef struct tagXMWAVEFORMAT { 
	WORD		wChannels; 							// ͨ����	
	WORD		wBitsPerSample;					// ָ��������bit����8bit��16bit��
	DWORD		dwSamplesPerSec;					// 8K��11025��16K��22050��24K��32K, 44100, 48000��
} XMWAVEFORMAT; 

// WAVE�豸������ʼ��(�����ں˵���)
void XM_WaveInit (void);
// WAVE�豸�����˳�(�����ں˵���)
void XM_WaveExit (void);


// �򿪲���/��¼�豸
// dwDevice ָ���豸��������XMMCIDEVICE_PLAY��XMMCIDEVICE_RECORD�������XMMCIDEVICE_PLAY|XMMCIDEVICE_RECORD
LONG XM_WaveOpen (DWORD dwDevice, XMWAVEFORMAT *lpWaveFormat);

// �رղ���/��¼�豸
// dwDevice ָ���豸��������XMMCIDEVICE_PLAY��XMMCIDEVICE_RECORD�������XMMCIDEVICE_PLAY|XMMCIDEVICE_RECORD
LONG XM_WaveClose (DWORD dwDevice);

// �򲥷��豸д��PCM��������
LONG XM_WaveWrite	(DWORD dwDevice, WORD *lpPcmSample, LONG cbPcmSample);

// ��¼���豸��ȡPCM��������
LONG XM_WaveRead	(DWORD dwDevice, WORD *lpPcmSample, LONG cbPcmSample);



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_MCI_H_