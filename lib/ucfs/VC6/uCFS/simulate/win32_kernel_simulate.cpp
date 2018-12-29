//****************************************************************************
//
//	Copyright (C) 2010~2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: win32_kernel_simulate.c
//	  Win32 ϵͳ�ں˻�������ӿ�ģ��
//
//	Revision history
//
//		2010.08.18	ZhuoYongHong Initial version
//
//****************************************************************************#include <io.h>
#include <stdio.h>
#include <direct.h>
#include <afx.h>
#include <assert.h>
#include <winbase.h>

#pragma warning(disable:4100) // ����ʾ4100�ž�����Ϣ (unreferenced formal parameter)
#pragma warning(disable : 4996)	// warning C4996: 'fopen': This function or variable may be unsafe.


typedef unsigned char		XMBOOL;		// BOOL����

#include "..\..\include\xm_proj_define.h"
#include "..\..\include\xmtype.h"
#include "..\..\include\xmdev.h"
#include "..\..\include\xmlang.h"
#include "..\..\include\xmbase.h"
#include "..\..\include\xmuser.h"
#include "..\..\include\xmkey.h"

#include "..\..\include\hw_osd_layer.h"



extern "C" {

typedef unsigned char		XMBOOL;		// BOOL����

volatile unsigned int hw_bSDCardState = SYSTEM_EVENT_CARD_INSERT;			// SD���Ѳ���
volatile static BYTE bVideoContrastLevel = 7;
volatile static BYTE bBackLight = 0;

volatile unsigned int hw_bSensorConnect;						// Sensor 0(����ͷ0)ʶ��(����)״̬												

volatile unsigned int hw_bLcdPlugIn;							// LCD������(������Ʒ)��δ��(������Ʒ)
volatile unsigned int hw_bAvoutPlugIn;							// AVOUT����/�γ�ģ��
volatile unsigned int hw_uAvoutType;							// AVOUT�������
volatile unsigned int hw_bHdmiPlugIn;							// HDMI����/�γ�״̬
volatile unsigned int hw_uHdmiType;								// HDMI����(��Ʒ����)
volatile unsigned int hw_bTimeSetting = 0;					// ϵͳʱ��δ����
volatile unsigned int hw_bBackupBatteryVoltageLevel;		// ���ݵ��״̬
volatile unsigned int hw_bMainBatteryVoltageLevel;			// �����״̬
volatile unsigned int hw_bUsbConnectState;					// USB����״̬
volatile unsigned int hw_bGpsbdConnectState;					// GPSBD����״̬
volatile unsigned int hw_bVideoRecState;						// ¼��״̬

volatile unsigned int hw_bDayNightState = 0;					//	0 ǿ�ƺ�ҹģ��
																// 1 ǿ�ư���ģ��
																//	2 �Զ�ѡ�� (����ʱ���Զ�ѡ����졢����ģʽ)	


static WORD wOemCodePage	= FML_LANG_CHINESE_SIMPLIFIED;	// ϵͳ�������ԣ��ļ�ϵͳʹ��
static WORD wResCodePage	= FML_LANG_CHINESE_SIMPLIFIED;	// UI(��Դ)�������ԣ�������ʾʹ��


// ��INI�ļ�����ģ������ϵͳ��ʼ״̬
void LoadInitialSetting (void)
{
	// ��״̬
	char str[128];
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"WITHDRAW",
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "DETECT") == 0)
		hw_bSDCardState = SYSTEM_EVENT_CARD_DETECT;						// �������
	else if(strcmp (str, "UNPLUG") == 0)
		hw_bSDCardState = SYSTEM_EVENT_CARD_UNPLUG;					// ���γ�
	else if(strcmp (str, "WRITE_PROTECT") == 0)
		hw_bSDCardState = SYSTEM_EVENT_CARD_INSERT_WRITE_PROTECT;	// ��д����
	else if(strcmp (str, "INSERT") == 0)
		hw_bSDCardState = SYSTEM_EVENT_CARD_INSERT;						// SD������(��д����)
	else if(strcmp (str, "FSERROR") == 0)
		hw_bSDCardState = SYSTEM_EVENT_CARD_FS_ERROR;						// SD������(�ļ�ϵͳ�����쳣, �޷�ʶ��Ŀ���)
	else if(strcmp (str, "VERIFY_ERROR") == 0)
		hw_bSDCardState = SYSTEM_EVENT_CARD_VERIFY_ERROR;				// SD����дУ����ʧ�ܣ�SD������
	else if(strcmp (str, "INVALID") == 0)
		hw_bSDCardState = SYSTEM_EVENT_CARD_INVALID;						// SD���޷�ʶ��
	else
		hw_bSDCardState = SYSTEM_EVENT_CARD_INSERT;						// SD������(��д����)

	// RTCʱ������
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"RTC",
			"ʱ��δ����",
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "ʱ��δ����") == 0)
		hw_bTimeSetting = 0;					// ʱ��δ����
	else if(strcmp (str, "ʱ��������") == 0)
		hw_bTimeSetting = 1;					// ʱ��������

	// ���ݵ��
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"BACKUPBATTERY",
			"2",			// ���״������
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "0") == 0)
		hw_bBackupBatteryVoltageLevel = 0;					// �ϲ�
	else if(strcmp (str, "1") == 0)
		hw_bBackupBatteryVoltageLevel = 1;					// ��
	else if(strcmp (str, "2") == 0)
		hw_bBackupBatteryVoltageLevel = 2;					// ����
	else
		hw_bBackupBatteryVoltageLevel = 3;					// ��

	// �����
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"MAINBATTERY",
			"2",			// ���״������
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "0") == 0)
		hw_bMainBatteryVoltageLevel = 0;					// �ϲ�
	else if(strcmp (str, "1") == 0)
		hw_bMainBatteryVoltageLevel = 1;					// ��
	else if(strcmp (str, "2") == 0)
		hw_bMainBatteryVoltageLevel = 2;					// ����
	else
		hw_bMainBatteryVoltageLevel = 3;					// ��

	// AVOUT �����γ�
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT",
			"PLUGIN",			// ����
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "PLUGIN") == 0)
		hw_bAvoutPlugIn = 1;					// AVOUT����
	else if(strcmp (str, "PLUGOUT") == 0)
		hw_bAvoutPlugIn = 0;					// AVOUT�γ�
	else 
		hw_bAvoutPlugIn = 0;					// AVOUT�γ�

	// AVOUT ����
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT_TYPE",
			"CVBS_NTSC",			
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "CVBS_NTSC") == 0)
		hw_uAvoutType = XM_LCDC_TYPE_CVBS_NTSC;
	else if(strcmp (str, "CVBS_PAL") == 0)
		hw_uAvoutType = XM_LCDC_TYPE_CVBS_PAL;
	else if(strcmp (str, "YPbPr_480i") == 0)
		hw_uAvoutType = XM_LCDC_TYPE_YPbPr_480i;
	else if(strcmp (str, "YPbPr_480p") == 0)
		hw_uAvoutType = XM_LCDC_TYPE_YPbPr_480p;
	else if(strcmp (str, "YPbPr_576i") == 0)
		hw_uAvoutType = XM_LCDC_TYPE_YPbPr_576i;
	else if(strcmp (str, "YPbPr_576p") == 0)
		hw_uAvoutType = XM_LCDC_TYPE_YPbPr_576p;
	else 
		hw_bAvoutPlugIn = 0;					// AVOUT�γ�


	// HDMI �����γ�
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI",
			"PLUGIN",			// ����
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "PLUGIN") == 0)
		hw_bHdmiPlugIn = 1;					// HDMI ����
	else if(strcmp (str, "PLUGOUT") == 0)
		hw_bHdmiPlugIn = 0;					// HDMI �γ�
	else 
		hw_bHdmiPlugIn = 0;					// HDMI �γ�

	// HDMI ����
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI_TYPE",
			"HDMI_1080p",			
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "HDMI_1080p") == 0)
		hw_uHdmiType = XM_LCDC_TYPE_HDMI_1080p;
	else if(strcmp (str, "HDMI_1080i") == 0)
		hw_uHdmiType = XM_LCDC_TYPE_HDMI_1080i;
	else if(strcmp (str, "HDMI_720p") == 0)
		hw_uHdmiType = XM_LCDC_TYPE_HDMI_720p;
	else if(strcmp (str, "HDMI_720i") == 0)
		hw_uHdmiType = XM_LCDC_TYPE_HDMI_720i;
	else
		hw_uHdmiType = XM_LCDC_TYPE_HDMI_720p;

	// LCD��ʾ�� ���ӻ�δ�� (ģ������/����)
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"LCD",
			"CONNECT",			// ����
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "CONNECT") == 0)
		hw_bLcdPlugIn = 1;					// LCD������
	else if(strcmp (str, "DISCONNECT") == 0)
		hw_bLcdPlugIn = 0;					// ������Ʒ
	else 
		hw_bLcdPlugIn = 0;					// ������Ʒ

	// USB����״̬ 
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"USB",
			"DISCONNECT",			// δ����
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "CHARGE") == 0)
		hw_bUsbConnectState = DEVCAP_USB_CONNECT_CHARGE;			// USB�Ѳ��룬��Ϊ�����ʹ��
	else if(strcmp (str, "UDISK") == 0)
		hw_bUsbConnectState = DEVCAP_USB_CONNECT_UDISK;				// USB�Ѳ��룬������Ϊ�����豸��U��ʹ��
	else if(strcmp (str, "CAMERA") == 0)
		hw_bUsbConnectState = DEVCAP_USB_CONNECT_CAMERA;			// USB�Ѳ��룬������Ϊ�����豸������ͷ
	else if(strcmp (str, "DISCONNECT") == 0)
		hw_bUsbConnectState = DEVCAP_USB_DISCONNECT;					// USB�ѶϿ�
	else
		hw_bUsbConnectState = DEVCAP_USB_DISCONNECT;					// USB�ѶϿ�

	// Sensor 0 ���ӻ��޷�ʶ��
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SENSOR",
			"CONNECT",			// ����
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "CONNECT") == 0)
		hw_bSensorConnect = DEVCAP_SENSOR_CONNECT;					// Sensor��ʶ�𣬳ɹ�����
	else if(strcmp (str, "DISCONNECT") == 0)
		hw_bSensorConnect = DEVCAP_SENSOR_DISCONNECT;					// Sensor�޷�ʶ��δ������
	else 
		hw_bSensorConnect = DEVCAP_SENSOR_DISCONNECT;					// Sensor�޷�ʶ��δ������

	// GPSBD ���ӡ��Ͽ�
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"GPSBD",
			"DISCONNECT",			// GPSBD�Ͽ�
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "DISCONNECT") == 0)
		hw_bGpsbdConnectState = DEVCAP_GPSBD_DISCONNECT;			// GPSBD�ѶϿ�
	else if(strcmp (str, "ANTENNA_OPEN") == 0)
		hw_bGpsbdConnectState = DEVCAP_GPSBD_CONNECT_ANTENNA_OPEN;			// GPSBD����δ����
	else if(strcmp (str, "ANTENNA_SHORT") == 0)
		hw_bGpsbdConnectState = DEVCAP_GPSBD_CONNECT_ANTENNA_SHORT;		// GPSBD���߶�·
	else if(strcmp (str, "ANTENNA_OK") == 0)
		hw_bGpsbdConnectState = DEVCAP_GPSBD_CONNECT_ANTENNA_OK;			// GPSBD��������
	else if(strcmp (str, "LOCATE_OK") == 0)
		hw_bGpsbdConnectState = DEVCAP_GPSBD_CONNECT_LOCATE_OK;				// GPSBD�Ѷ�λ
	else
		hw_bGpsbdConnectState = DEVCAP_GPSBD_DISCONNECT;			// GPSBD�ѶϿ�

	// ���졢����ģʽѡ��
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"DAYTIME",
			"AUTO",			// ȱʡ�Զ�ѡ��
			str,
			sizeof(str),
			".\\ark1960.ini"
			);
	if(strcmp (str, "SUN") == 0)
		hw_bDayNightState = 1;			// ǿ�ư���ģʽ
	else if(strcmp (str, "MOON") == 0)
		hw_bDayNightState = 0;			// ǿ������ģʽ
	else //if(strcmp (str, "AUTO") == 0)
		hw_bDayNightState = 2;			// �Զ�ѡ��  (����ʱ���Զ�ѡ����졢����ģʽ)	
}

// ����ģ������ϵͳ��ʼ״̬��INI�ļ�
void SaveInitialSetting (void)
{
	if(hw_bSDCardState == SYSTEM_EVENT_CARD_DETECT)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"DETECT",
			".\\ark1960.ini"
			);
	else if(hw_bSDCardState == SYSTEM_EVENT_CARD_UNPLUG)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"UNPLUG",
			".\\ark1960.ini"
			);
	else if(hw_bSDCardState == SYSTEM_EVENT_CARD_INSERT_WRITE_PROTECT)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"WRITE_PROTECT",
			".\\ark1960.ini"
			);
	else if(hw_bSDCardState == SYSTEM_EVENT_CARD_INSERT)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"INSERT",
			".\\ark1960.ini"
			);
	else if(hw_bSDCardState == SYSTEM_EVENT_CARD_FS_ERROR)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"FSERROR",
			".\\ark1960.ini"
			);
	else if(hw_bSDCardState == SYSTEM_EVENT_CARD_VERIFY_ERROR)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"VERIFY_ERROR",
			".\\ark1960.ini"
			);
	else if(hw_bSDCardState == SYSTEM_EVENT_CARD_INVALID)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"INVALID",
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SDCARD",
			"�ɶ�д",
			".\\ark1960.ini"
			);

	if(hw_bTimeSetting == 0)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"RTC",
			"ʱ��δ����",
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"RTC",
			"ʱ��������",
			".\\ark1960.ini"
			);

	// ���ݵ��
	if(hw_bBackupBatteryVoltageLevel == 0)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"BACKUPBATTERY",
			"0",			// ���״������
			".\\ark1960.ini"
			);
	else if(hw_bBackupBatteryVoltageLevel == 1)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"BACKUPBATTERY",
			"1",			// ���״������
			".\\ark1960.ini"
			);
	else if(hw_bBackupBatteryVoltageLevel == 2)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"BACKUPBATTERY",
			"2",			// ���״������
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"BACKUPBATTERY",
			"3",			// ���״������
			".\\ark1960.ini"
			);

	// �����
	if(hw_bMainBatteryVoltageLevel == 0)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"MAINBATTERY",
			"0",			// ���״������
			".\\ark1960.ini"
			);
	else if(hw_bMainBatteryVoltageLevel == 1)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"MAINBATTERY",
			"1",			// ���״������
			".\\ark1960.ini"
			);
	else if(hw_bMainBatteryVoltageLevel == 2)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"MAINBATTERY",
			"2",			// ���״������
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"MAINBATTERY",
			"3",			// ���״������
			".\\ark1960.ini"
			);


	// AVOUT����
	if(hw_uAvoutType == XM_LCDC_TYPE_CVBS_NTSC)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT_TYPE",
			"CVBS_NTSC",			// CVBS_NTSC
			".\\ark1960.ini"
			);
	else if(hw_uAvoutType == XM_LCDC_TYPE_CVBS_PAL)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT_TYPE",
			"CVBS_PAL",			// CVBS_PAL
			".\\ark1960.ini"
			);
	else if(hw_uAvoutType == XM_LCDC_TYPE_YPbPr_480i)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT_TYPE",
			"YPbPr_480i",			// YPbPr_480i
			".\\ark1960.ini"
			);
	else if(hw_uAvoutType == XM_LCDC_TYPE_YPbPr_480p)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT_TYPE",
			"YPbPr_480p",			// YPbPr_480p
			".\\ark1960.ini"
			);
	else if(hw_uAvoutType == XM_LCDC_TYPE_YPbPr_576i)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT_TYPE",
			"YPbPr_576i",			// YPbPr_576i
			".\\ark1960.ini"
			);
	else if(hw_uAvoutType == XM_LCDC_TYPE_YPbPr_576p)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT_TYPE",
			"YPbPr_576p",			// YPbPr_576p
			".\\ark1960.ini"
			);

	if(hw_bAvoutPlugIn == 1)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT",
			"PLUGIN",			// AVOUT����
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"AVOUT",
			"PLUGOUT",			// AVOUT�γ�
			".\\ark1960.ini"
			);


	// HDMI���״̬
	if(hw_bHdmiPlugIn == 1)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI",
			"PLUGIN",			// HDMI ����
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI",
			"PLUGOUT",			// HDMI �γ�
			".\\ark1960.ini"
			);

	// HDMI�ӿ�����
	if(hw_uHdmiType == XM_LCDC_TYPE_HDMI_1080p)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI_TYPE",
			"HDMI_1080p",			// HDMI_1080p
			".\\ark1960.ini"
			);
	else if(hw_uHdmiType == XM_LCDC_TYPE_HDMI_1080i)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI_TYPE",
			"HDMI_1080i",			// HDMI_1080i
			".\\ark1960.ini"
			);
	else if(hw_uHdmiType == XM_LCDC_TYPE_HDMI_720p)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI_TYPE",
			"HDMI_720p",			// HDMI_720p
			".\\ark1960.ini"
			);
	else if(hw_uHdmiType == XM_LCDC_TYPE_HDMI_720i)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI_TYPE",
			"HDMI_720i",			// HDMI_720i
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"HDMI_TYPE",
			"HDMI_720p",			// HDMI_720p
			".\\ark1960.ini"
			);


	// LCD��״̬
	if(hw_bLcdPlugIn == 1)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"LCD",
			"CONNECT",			// ��LCD��
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"LCD",
			"DISCONNECT",		// ��LCD��
			".\\ark1960.ini"
			);

	// USB����״̬ 
	if(hw_bUsbConnectState == DEVCAP_USB_CONNECT_CHARGE)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"USB",
			"CHARGE",			// USB�Ѳ��룬��Ϊ�����ʹ��
			".\\ark1960.ini"
			);
	else if(hw_bUsbConnectState == DEVCAP_USB_CONNECT_UDISK)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"USB",
			"UDISK",			// USB�Ѳ��룬������Ϊ�����豸��U��ʹ��
			".\\ark1960.ini"
			);
	else if(hw_bUsbConnectState == DEVCAP_USB_CONNECT_CAMERA)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"USB",
			"CAMERA",			// USB�Ѳ��룬������Ϊ�����豸������ͷ
			".\\ark1960.ini"
			);
	else if(hw_bUsbConnectState == DEVCAP_USB_DISCONNECT)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"USB",
			"DISCONNECT",			// USB�ѶϿ�
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"USB",
			"DISCONNECT",			// USB�ѶϿ�
			".\\ark1960.ini"
			);



	// Sensor 0(����ͷ0)״̬
	if(hw_bSensorConnect == DEVCAP_SENSOR_CONNECT)
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SENSOR",
			"CONNECT",			// Sensor��ʶ�𣬳ɹ�����
			".\\ark1960.ini"
			);
	else
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"SENSOR",
			"DISCONNECT",		// Sensor�޷�ʶ��δ������
			".\\ark1960.ini"
			);

	if(hw_bGpsbdConnectState == DEVCAP_GPSBD_DISCONNECT)
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"GPSBD",
			"DISCONNECT",		
			".\\ark1960.ini"
			);
	}
	else if(hw_bGpsbdConnectState == DEVCAP_GPSBD_CONNECT_ANTENNA_OPEN)
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"GPSBD",
			"ANTENNA_OPEN",		
			".\\ark1960.ini"
			);
	}
	else if(hw_bGpsbdConnectState == DEVCAP_GPSBD_CONNECT_ANTENNA_SHORT)
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"GPSBD",
			"ANTENNA_SHORT",		
			".\\ark1960.ini"
			);
	}
	else if(hw_bGpsbdConnectState == DEVCAP_GPSBD_CONNECT_ANTENNA_OK)
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"GPSBD",
			"ANTENNA_OK",		
			".\\ark1960.ini"
			);
	}
	else if(hw_bGpsbdConnectState == DEVCAP_GPSBD_CONNECT_LOCATE_OK)
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"GPSBD",
			"LOCATE_OK",		
			".\\ark1960.ini"
			);
	}
	else
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"GPSBD",
			"DISCONNECT",		
			".\\ark1960.ini"
			);
	}

	// ���졢����ģʽ
	if(hw_bDayNightState == 0)		// 0 ǿ�ƺ�ҹģ��
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"DAYTIME",
			"MOON",		
			".\\ark1960.ini"
			);
	}
	else if(hw_bDayNightState == 1)		// 1 ǿ�ư���ģ��
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"DAYTIME",
			"SUN",		
			".\\ark1960.ini"
			);
	}
	else
	{
		WritePrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"DAYTIME",
			"AUTO",		
			".\\ark1960.ini"
			);

	}
}

// ��ȡϵͳ���Դ���ҳ
WORD	XM_GetSystemCodePage		(void)
{
	return wOemCodePage;
}

// ��ȡ��Դ���Դ���ҳ
WORD	XM_GetResourceCodePage	(void)
{
	return wResCodePage;
}

// ������Դ���Դ���ҳ������ֵΪԭ���Ĵ���ҳ
WORD	XM_SetResourceCodePage	(WORD wCodePage)
{
	WORD wCode = wResCodePage;
	wResCodePage = wCodePage;
	return wCode;
}


XMBOOL	XM_GetLocalTime	(XMSYSTEMTIME* pSystemTime)
{
	SYSTEMTIME time;
	GetLocalTime (&time);

	pSystemTime->wYear = time.wYear;
	pSystemTime->bDay = (BYTE)time.wDay;
	pSystemTime->bMonth = (BYTE)time.wMonth;
	pSystemTime->bDayOfWeek = (BYTE)time.wDayOfWeek;
	pSystemTime->bHour = (BYTE)time.wHour;
	pSystemTime->bMinute = (BYTE)time.wMinute;
	pSystemTime->bSecond = (BYTE)time.wSecond;
	return (XMBOOL)hw_bTimeSetting;
}

VOID	XM_SetLocalTime	(const XMSYSTEMTIME *pSystemTime)
{
	hw_bTimeSetting = 1;
}


// ����ϵͳ����
XMBOOL	XM_SetFmlDeviceCap  (BYTE bDevCapIndex, DWORD dwValue)
{
	switch (bDevCapIndex)
	{

		case DEVCAP_VIDEOCONTRASTLEVEL:
			// ���û��ȡ�Աȶ� (0 ~ 15��)
			if(dwValue >= 15)
				return 0;
			bVideoContrastLevel = (BYTE)dwValue;
			break;

		case DEVCAP_USB:
			return 0;

		case DEVCAP_MAINBATTERYVOLTAGE:
			// ���������õ�ѹ��ֻ�����ȡ
			return 0;

		case DEVCAP_BACKLIGHT:
			bBackLight = (BYTE)dwValue;
			break;

		case DEVCAP_AVOUT:
			hw_bAvoutPlugIn = (BYTE)dwValue;
			break;

		case DEVCAP_HDMI:
			hw_bHdmiPlugIn = (BYTE)dwValue;
			break;

		case DEVCAP_VIDEO_REC:
			if(dwValue > DEVCAP_VIDEO_REC_START)
				return 0;
			hw_bVideoRecState = (BYTE)dwValue;
			break;

		default:
			return 0;
	}
	return 1;
}

// ��ȡϵͳ����
DWORD  	XM_GetFmlDeviceCap  (BYTE bDevCapIndex)
{
	switch (bDevCapIndex)
	{

		case DEVCAP_VIDEOCONTRASTLEVEL:
			return (WORD)bVideoContrastLevel;

		case DEVCAP_MAINBATTERYVOLTAGE:
			return hw_bMainBatteryVoltageLevel;

		case DEVCAP_USB:
			return hw_bUsbConnectState;

		case DEVCAP_BACKLIGHT:
			return bBackLight;

		case DEVCAP_SDCARDSTATE:	// SD��״̬���
			return hw_bSDCardState;

		case DEVCAP_TIMESETTING:	// ϵͳʱ���Ƿ�������
			return hw_bTimeSetting;

		case DEVCAP_BACKUPBATTERYVOLTAGE:	// ���ݵ��
			return hw_bBackupBatteryVoltageLevel;

		case DEVCAP_AVOUT:			// AVOUT���״̬
			return hw_bAvoutPlugIn;			// 1 ���� 0 �γ�

		case DEVCAP_AVOUT_TYPE:			// AVOUT����
			return hw_uAvoutType;			

		case DEVCAP_HDMI:				// HDMI���״̬
			return hw_bHdmiPlugIn;			// 1 ���� 0 �γ�

		case DEVCAP_HDMI_TYPE:			// AVOUT����
			return hw_uHdmiType;			

		case DEVCAP_LCD:					// LCD������״̬
			return hw_bLcdPlugIn;

		case DEVCAP_SCREEN:				// ������ʾ��״̬
			if(hw_bAvoutPlugIn || hw_bLcdPlugIn || hw_bHdmiPlugIn)
				return 1;
			else
				return 0;

		case DEVCAP_SENSOR:				// ����ͷ״̬
			return hw_bSensorConnect;

		case DEVCAP_GPSBD:
			return hw_bGpsbdConnectState;

		case DEVCAP_VIDEO_REC:
			return hw_bVideoRecState;

		case DEVCAP_DAYNIGHT:
			if(hw_bDayNightState == 0)		// ����ģʽ
				return DEVCAP_NIGHT;
			else if(hw_bDayNightState == 1)		// ����ģʽ
				return DEVCAP_DAY;
			else if(hw_bDayNightState == 2)		// �Զ�ģʽ
			{
				XMSYSTEMTIME time;	
				XM_GetLocalTime (&time);
				// 7 ~ 19��
				if(time.bHour >= 7 && time.bHour <= 19)
					return DEVCAP_DAY;
				else
					return DEVCAP_NIGHT;
			}
			else
				return DEVCAP_DAY;

		default:
			return 0;
	}
}



extern void XM_heap_init (void);
extern void XM_heap_exit (void);


CRITICAL_SECTION CriticalSection;
CRITICAL_SECTION CodecCriticalSection;


void XM_SimulatorInit (void)
{
	XM_heap_init ();
	LoadInitialSetting ();
	InitializeCriticalSection (&CriticalSection);

	InitializeCriticalSection (&CodecCriticalSection);
}

void XM_SimulatorExit (void)
{
	XM_heap_exit ();
	SaveInitialSetting ();
	DeleteCriticalSection (&CriticalSection);
}

// ���ж�
void XM_lock (void)
{
	EnterCriticalSection (&CriticalSection);
}

// ���ж�
void XM_unlock (void)
{
	LeaveCriticalSection (&CriticalSection);
}

VOID XM_idle (VOID)
{

}

VOID XM_wakeup (VOID)
{
}

short int XM_GetKeyState (int vKey)
{
	return GetKeyState (vKey);
}


#define ANSI            /* Comment out for UNIX version     */
#include <stdarg.h>

void __cdecl XM_printf (const char *fmt, ...)
{
	char xm_info[2048];

	va_list ap;
	va_start(ap, fmt); 
	vsprintf (xm_info, fmt, ap);
	// vsprintf_s (xm_info, sizeof(xm_info), fmt, ap);
	::AfxTrace ("%s", xm_info);
	va_end(ap); 
}


int		XM_LoadMenuData (void *lpMenuData, int cbMenuData)
{
	FILE *fp = fopen ("menudata.bin", "rb");
	if(fp == 0)
		return -1;
	if( (int)fread (lpMenuData, 1, cbMenuData, fp) != cbMenuData)
	{
		fclose (fp);
		return -1;
	}
	fclose (fp);
	return 0;

}

int		XM_SaveMenuData (void *lpMenuData, int cbMenuData)
{
	FILE* fp = fopen ("menudata.bin", "wb");
	if(fp == 0)
		return -1;
	if( (int)fwrite (lpMenuData, 1, cbMenuData, fp) != cbMenuData)
	{
		fclose (fp);
		return -1;
	}
	fclose (fp);
	return 0;
}



void* OS_malloc(unsigned int n)
{
	return malloc (n);
}

void  OS_free  (void* pMemBlock)
{
	free (pMemBlock);
}

void* OS_calloc (unsigned int bytes,unsigned int size)
{
	return calloc (bytes, size);
}

void* OS_realloc  (void* pMemBlock, unsigned NewSize)
{
	return realloc (pMemBlock, NewSize);
}

}