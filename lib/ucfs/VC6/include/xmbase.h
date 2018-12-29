//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xmbase.h
//	  constant��macro & basic typedef definition of kernel service
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_BASE_H_
#define _XM_BASE_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// macro definition

// ϵͳ�������Ͷ���
#define	SDS_REBOOT				0		// re-booting ϵͳ��������
#define	SDS_POWEROFF			1		// system power off ϵͳ�ػ�,���Ѻ���������
#define	SDS_RESUME				2		// system power off ϵͳ�ػ�,���Ѻ�ָ��ϴε��ֳ�

// ���״̬�¼�
#define	DEVCAP_VOLTAGE_WORST		0
#define	DEVCAP_VOLTAGE_BAD		1
#define	DEVCAP_VOLTAGE_NORMAL	2
#define	DEVCAP_VOLTAGE_GOOD		3


// bDevCapIndex����
#define	DEVCAP_AUTOPOWEROFFTIME			0x01
			// ���û��ȡ�Զ��ػ�ʱ��
// wValue = 0��ʾ��ֹ�Զ��ػ�����0ֵ��ʾ���� (1 ~ 120)

#define	DEVCAP_VIDEOCONTRASTLEVEL		0x02
			// ���û��ȡ�Աȶ� (0 ~ 15��)

#define	DEVCAP_KEYBOARDTICKON			0x03
			// ���ð��������Ƿ��� ��0 �رգ� 1 ������

#define	DEVCAP_MAINBATTERYVOLTAGE		0x04
			// ��ȡ��ѹ״̬
			// 0  Worst (�ܲ�)
			// 1  bad	(�ϲ�)
			// 2	normal(����)
			// 3  best	(����)

#define	DEVCAP_USB							0x05
			// ��ȡUSB��״̬
	#define	DEVCAP_USB_DISCONNECT			0		// USB���ӶϿ�
	#define	DEVCAP_USB_CONNECT_CHARGE		1		// USB�Ѳ��룬��Ϊ�����ʹ��
	// USB�豸����
	#define	DEVCAP_USB_CONNECT_UDISK		2		// USB�Ѳ��룬������Ϊ�����豸��U��ʹ��
	#define	DEVCAP_USB_CONNECT_CAMERA		3		// USB�Ѳ��룬������Ϊ�����豸������ͷ

#define	DEVCAP_BACKLIGHT					0x06
			// ������رձ���
			// 1  ��������
			// 0	�رձ���

#define	DEVCAP_SDCARDSTATE				0x07	
			// SD��״̬���
			// 0  SD��׼����(���ڼ��SD��)
			//	1	SD��δ����
			// 2	SD��д����
			//	3	SD�����룬�Ҷ�д����
			//	4	SD�����룬���ļ�ϵͳ�Ѱ�װ (�޷�����������Ƶ��Ŀ¼)
			// 5	SD�����룬�Ҷ�д�����ļ�У����ʧ��(�����ǻ���)
			// 6	SD�����룬���ݿ�����ʶ�������
			// 7	SD�����룬�Ҷ�д�������ݿ���д��
			// 8	SD�����룬�޷�֧�ֵ��ļ�ϵͳ exFAT����NTFS
			//		"���ļ�ϵͳ�쳣"��"���ļ�У��ʧ��"��ʾ�û�ִ�и�ʽ������
	#define	DEVCAP_SDCARDSTATE_DETECT						0	// SD��׼����(���ڼ��SD��)
	#define	DEVCAP_SDCARDSTATE_UNPLUG						1	// ���ݿ��γ�
	#define	DEVCAP_SDCARDSTATE_WRITEPROTECT				2	// ���ݿ�д����
	#define	DEVCAP_SDCARDSTATE_INSERT						3	// ���ݿ���дģʽ(����ģʽ)
	#define	DEVCAP_SDCARDSTATE_FSERROR						4	// ���ݿ���ʶ��(��дģʽ)�����ļ�ϵͳ�Ѱ�װ
																			//		�޷������ݿ��ϴ���������Ƶ��¼��Ŀ¼�� 
																			//		������Ŀ¼ͬ�����ļ�ʱ������������(�Զ��޸�)
																			//		�޷��Զ��޸�ʱ��Ҫ��ʾ"��ʽ�����߻���"
															
	#define	DEVCAP_SDCARDSTATE_VERIFYFAILED				5	// �ļ���дУ��ʧ�� (���ݿ�����ʧЧ�Ļ������������ǻ���)
	#define	DEVCAP_SDCARDSTATE_INVALID						6	//	���ݿ�����ʶ�������(FS_MOUNTʧ��)
	#define	DEVCAP_SDCARDSTATE_DISKFULL					7	// ���ݿ���д��
	#define	DEVCAP_SDCARDSTATE_FSUNSUPPORT				8	// �޷�֧�ֵ��ļ�ϵͳ (exFAT����NTFS)

#define	DEVCAP_BACKUPBATTERYVOLTAGE	0x08
			// ���ݵ�ص�ѹ״̬
	#define	DEVCAP_BATTERYVOLTAGE_WORST		0
	#define	DEVCAP_BATTERYVOLTAGE_BAD			1
	#define	DEVCAP_BATTERYVOLTAGE_NORMAL		2
	#define	DEVCAP_BATTERYVOLTAGE_BEST			3
			// 0  Worst (�ܲ�)
			// 1  bad	(�ϲ�)
			// 2	normal(����)
			// 3  best	(����)

#define	DEVCAP_MIC							0x09
			// MIC״̬��ȡ������
			//	0	�ر�
			// 1	����
	#define	DEVCAP_MIC_OFF		0
	#define	DEVCAP_MIC_ON		1

#define	DEVCAP_TIMESETTING				0x0A
			// ��ȡϵͳʱ������״̬
			// 0	ϵͳʱ����δ����
			// 1	ϵͳʱ��������
	#define	DEVCAP_TIMESETTING_NG	0
	#define	DEVCAP_TIMESETTING_OK	1	

#define	DEVCAP_AVOUT						0x0B
			// ��ȡϵͳAVOUT�Ĳ���״̬
			// 0	AVOUT�γ�
			// 1	AVOUT����
	#define	DEVCAP_AVOUT_PLUGOUT		0	
	#define	DEVCAP_AVOUT_PLUGIN		1

#define	DEVCAP_AVOUT_TYPE					0x0C
			// ��ȡϵͳAVOUT�����Ͷ��� (�ɲ�Ʒ����)
			// ȡֵ�ο� hw_osd_layer.h �е�LCDC���ͨ�����Ͷ����AVOUT����

#define	DEVCAP_HDMI							0x0D
			// ��ȡϵͳHDMI�Ĳ���״̬
			// 0	HDMI�γ�
			// 1	HDMI����
	#define	DEVCAP_HDMI_PLUGOUT		0	
	#define	DEVCAP_HDMI_PLUGIN		1

#define	DEVCAP_HDMI_TYPE					0x0E
			// ��ȡϵͳ�����HDMI������ (�ɲ�Ʒ����)
			// ȡֵ�ο� hw_osd_layer.h �е�LCDC���ͨ�����Ͷ����HDMI����


#define	DEVCAP_LCD							0x0F	
			// ��ȡLCD��������״̬(������Ʒ��������Ʒ)
			// 0	������Ʒ
			// 1	������Ʒ
	#define	DEVCAP_LCD_DISCONNECT	0	
	#define	DEVCAP_LCD_CONNECT		1	

#define	DEVCAP_SCREEN						0x10
			// ��ȡ��ʾ��������״̬
			// 0	��ʾ��δ����(LCD��AVOUT��HDMI����ʾ�����δ����)
			// 1	��ʾ��������(����LCD��AVOUT��HDMI����һ����ʾ����������)
	#define	DEVCAP_SCREEN_DISCONNECT	0
	#define	DEVCAP_SCREEN_CONNECT		1

#define	DEVCAP_GPSBD					0x11
			// ��ȡGPSBD������״̬ 
			// 0	GPSBD�ѶϿ�(δ����)
			//	1	GPSBD������(����δ����)
			// 2	GPSBD������(���߶�·)
			// 3	GPSBD������(����������)
			// 4	GPSBD������(��λ�ɹ�)
	#define	DEVCAP_GPSBD_DISCONNECT						0
	#define	DEVCAP_GPSBD_CONNECT_ANTENNA_OPEN		1	
	#define	DEVCAP_GPSBD_CONNECT_ANTENNA_SHORT		2	
	#define	DEVCAP_GPSBD_CONNECT_ANTENNA_OK			3	
	#define	DEVCAP_GPSBD_CONNECT_LOCATE_OK			4	

#define	DEVCAP_VIDEO_REC				0x12		// ¼��״̬(ֹͣ��¼����)
	#define	DEVCAP_VIDEO_REC_STOP						0
	#define	DEVCAP_VIDEO_REC_START						1


#define	DEVCAP_VIDEOITEM				0x13		// ��Ƶ��״̬
															//	0 ��ʾ��Ƶ�������������޴���
	#define	DEVCAP_VIDEOITEM_NO_DATA					0x00000001		// ����Ƶ�����ݿ�
																						//		�������ڡ���д����������
	#define	DEVCAP_VIDEOITEM_TOO_MANY_ITEM			0x00000002		// ��Ƶ��̫�࣬����ϵͳ����
																						//		��Ҫ��ʽ�����Զ�ɾ��
	#define	DEVCAP_VIDEOITEM_LOWEST_RECYCLE_SPACE	0x00000004		// ��ѭ��ʹ�ÿռ�̫�ͣ��޷�¼��һ����������Ƶ
																						//		��Ҫ��ʽ�������Զ�ɾ��
	
	

#define	DEVCAP_SENSOR						0x20
#define	DEVCAP_SENSOR_1					0x21
#define	DEVCAP_SENSOR_2					0x22
#define	DEVCAP_SENSOR_3					0x23
#define	DEVCAP_SENSOR_4					0x24
#define	DEVCAP_SENSOR_5					0x25
#define	DEVCAP_SENSOR_6					0x26
#define	DEVCAP_SENSOR_7					0x27
			// ��ȡ����ͷ��״̬
			// 0	����ͷ�޷�ʶ��δ����
			// 1	����ͷ��ʶ�𣬳ɹ�����
	#define	DEVCAP_SENSOR_DISCONNECT	0
	#define	DEVCAP_SENSOR_CONNECT		1


#define	DEVCAP_DAYNIGHT					0x28		// ����/����״̬
	#define	DEVCAP_NIGHT					0			// ����
	#define	DEVCAP_DAY						1			// ����

#ifndef _XMBOOL_DEFINED_
#define _XMBOOL_DEFINED_
typedef unsigned char		XMBOOL;		// BOOL����
#endif

// ϵͳ��������
XMBOOL	XM_SetFmlDeviceCap  (BYTE bDevCapIndex, DWORD wValue);
DWORD  	XM_GetFmlDeviceCap  (BYTE bDevCapIndex);

// typedef definition

// structure definition

// ϵͳʱ��
typedef struct tagXMSYSTEMTIME {
	WORD		wYear;			
	BYTE		bDay;				// 1 ~ 31
	BYTE		bMonth;			// 1 ~ 12
	BYTE		bDayOfWeek;		// 0 ~ 6
	BYTE		bHour;			// 0 ~ 23
	BYTE		bMinute;			// 0 ~ 59
	BYTE		bSecond;			// 0 ~ 59
	WORD		wMilliSecond;	// 0 ~ 999
} XMSYSTEMTIME, *PXMSYSTEMTIME;

// ����
typedef struct tagXMSYSTEMALARM {
	UCHAR		AlarmHour;		// 0 ~ 23
	UCHAR		AlarmMinute;	// 0 ~ 59
	WORD		AlarmDay;		// since 2010.1.1						
} XMSYSTEMALARM, *PXMSYSTEMALARM;

// function protocol type
VOID XM_InitRTC (VOID);

// ʱ������/��ȡ����
//		����1��ʾϵͳʱ��������
//		����0��ʾϵͳʱ��δ���ã�ϵͳ����ȱʡ����ʱ��
XMBOOL	XM_GetLocalTime	(XMSYSTEMTIME* pSystemTime);
VOID		XM_SetLocalTime	(const XMSYSTEMTIME *pSystemTime);

// ��ȡ����������ĵδ���� ��1msΪһ�δ������λ��
DWORD		XM_GetTickCount	(VOID);

// ��ȡ����������߾��ȵĵδ��������΢��Ϊ������λ
XMINT64 		XM_GetHighResolutionTickCount (void);


VOID		XM_Sleep (DWORD dwMilliseconds);		// ��ʱ�����뵥λ

// ��������
XMBOOL	XM_SetAlarm		(const XMSYSTEMALARM *pSystemAlarm);
XMBOOL	XM_KillAlarm	(VOID);

// ϵͳ����,�Ƿ�������ʽ��ͬ�� SDS_REBOOT
VOID		XM_ShutDownSystem  (BYTE bShutDownType);


XMBOOL	XM_LoadUSBModule		(VOID);
XMBOOL	XM_ExitUSBModule		(VOID);

XMBOOL	XM_UsbDetect			(VOID);

VOID		XM_EnableUsb			(XMBOOL bEnableUsb);

short int XM_GetKeyState		(int vKey);

// ����������ʼ��
void 		XM_KeyInit				(void);

#define	XM_BEEP_KEYBOARD			1

// Beep����(�簴����)
XMBOOL XM_Beep (DWORD dwBeepID);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_BASE_H_
