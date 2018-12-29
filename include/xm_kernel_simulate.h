//****************************************************************************
//
//	Copyright (C) 2010~2014 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_kernel_simulate.h
//	  Ӳ��ģ��ӿ�
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_KERNEL_SIMULATE_H_
#define _XM_KERNEL_SIMULATE_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// Win32 �ں�ģ��ӿڣ�����MFCģ��Ӳ���޸��ں˵�״̬

// -------------------------------------------------------
//
// ********** !!! ����ȫ�ֱ�������Win32��Ӧ�� ************
//
// -------------------------------------------------------
#if WIN32 && _WINDOWS

extern unsigned int hw_bSDCardState;					// SD������״̬ģ��
extern unsigned int hw_bTimeSetting;					// ϵͳʱ������״̬ 0 δ���� 1 ������
extern unsigned int hw_bBackupBatteryVoltageLevel;	// ���ݵ��״̬
extern unsigned int hw_bMainBatteryVoltageLevel;	// �����״̬	
extern unsigned int hw_bLcdPlugIn;						// LCD������(������Ʒ)��δ��(������Ʒ)
extern unsigned int hw_bAvoutPlugIn;					// AVOUT����/�γ�ģ��
extern unsigned int hw_uAvoutType;						// AVOUT�������
extern unsigned int hw_bHdmiPlugIn;						// HDMI����/�γ�״̬
extern unsigned int hw_uHdmiType;						// HDMI����(��Ʒ����)
extern unsigned int hw_bSensorConnect;					// Sensor 0(����ͷ0)ʶ��(����)״̬												
extern unsigned int hw_bUsbConnectState;				// USB����״̬
extern unsigned int hw_bGpsbdConnectState;			// GPSBD����״̬
extern unsigned int hw_bDayNightState;					// ���졢��ҹģ��						

// ******** ��Ƶģ��ӿ� ********

// ��Win32 HDC�豸��ָ��λ����ʾlcdc�����ͨ��
void FML_VideoUpdate (HDC hdc, int x, int y, unsigned int lcdc_channel);
// ����win32λͼclipboard
void FML_CreateBitmapClipboard (HWND hWnd, unsigned int lcd_channel);

// WIN32��ģ�ⴥ��ƣ�ͼ�ʻ�¼�����
void Win32_TriggerLongTimeDrivingEvent (unsigned int event);

void FML_IspIsr (void);

#endif

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_KERNEL_SIMULATE_H_