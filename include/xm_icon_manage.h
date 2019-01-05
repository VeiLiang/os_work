//****************************************************************************
//
//	Copyright (C) 2010~2014 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_icon_manage.h
//	  ״̬ICON�Ĺ�����ʾ������
//
//	Revision history
//
//		2012.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
/*
*/
#ifndef _XM_ICON_MANAGE_H_
#define _XM_ICON_MANAGE_H_

#include "xm_osd_framebuffer.h"
#include "xm_app_menudata.h"

#if defined (__cplusplus)
	extern "C"{
#endif


enum {	
	AP_ICON_REC_OFF	=	0,		// ¼��ر�
	AP_ICON_REC_ON,				// ¼����
	AP_ICON_REC_COUNT
};

enum
{
	AP_ICON_DISPLAY_OFF = 0,
	AP_ICON_DISPLAY_ON,
};


// ICON���Ͷ���
enum {
	AP_ICON_MIC,						// MIC��־(�������ر�)
											//		AP_SETTING_MIC_OFF
											//		AP_SETTING_MIC_ON
	
	AP_ICON_VOICE,						// �������ֿ��ر�־(�������ر�)
											//		AP_SETTING_VOICE_PROMPTS_OFF
											//		AP_SETTING_VOICE_PROMPTS_ON
	
	AP_ICON_REC_RED_DOT,				//¼����
											
											
	
	AP_ICON_VIDEORESOLUTION,		// ¼������(1080P 60��1080P 30��720P 60��720P 30��)
											//		AP_SETTING_VIDEORESOLUTION_1080P_30
											//		AP_SETTING_VIDEORESOLUTION_1080P_60
											//		AP_SETTING_VIDEORESOLUTION_720P_30
											//		AP_SETTING_VIDEORESOLUTION_720P_60
	
	AP_ICON_CARD,						// ��״̬ (���γ������𻵡���д���������޷�ʶ�𡢿�������д)
											//		DEVCAP_SDCARDSTATE_UNPLUG
											//		DEVCAP_SDCARDSTATE_WRITEPROTECT
											//		DEVCAP_SDCARDSTATE_INSERT
											//		DEVCAP_SDCARDSTATE_VERIFYFAILED
	
	AP_ICON_USB,						// USB���ӶϿ���USB��硢U��
											//		DEVCAP_USB_DISCONNECT
											//		DEVCAP_USB_CONNECT_CHARGE
											//		DEVCAP_USB_CONNECT_UDISK
											//		DEVCAP_USB_CONNECT_CAMERA

	AP_ICON_MAIN_BATTERY,			// �����״̬
	AP_ICON_BACKUP_BATTERY,			// ���ݵ��״̬
											//		DEVCAP_BATTERYVOLTAGE_WORST
											//		DEVCAP_BATTERYVOLTAGE_BAD
											//		DEVCAP_BATTERYVOLTAGE_NORMAL
											//		DEVCAP_BATTERYVOLTAGE_BEST
	

	AP_ICON_GPS,						//  GPS״̬
										// δ����
										// ������
										// �Ѷ�λ
    AP_ICON_PHOTO,	
    AP_ICON_HOME ,
    AP_ICON_LOCK ,
    AP_ICON_DAY_NIGHT_MODE,
    AP_ICON_SYS_SETTING,
    AP_ICON_VIDEO_SWITCH,
    AP_ICON_PHOTOGRAPH,
    AP_ICON_RECORD_SWITCH,
    AP_ICON_RECORD_LOCK,
    AP_ICON_RECORD_LIST,
    AP_ICON_V_LINE,
    AP_ICON_H_LINE,
    AP_ICON_BIAOZHI,
    AP_ICON_CH_AHD1,
    AP_ICON_CH_AHD2,
    AP_ICON_NO_SIGNED,
    AP_ICON_MODE,
	AP_ICON_TAKEPHOTO,
	
	AP_ICON_COUNT
};

// ICON�����ʼ��
void XM_IconInit (void);

// ICON����ر�
void XM_IconExit (void);

void XM_VideoIconInit (void);

void XM_VideoIconExit (void);




// ����ICON��״̬
void XM_IconSetState (unsigned int dwIcon, unsigned int dwIconState);

// ��ȡICON��״̬
unsigned int XM_IconGetState (unsigned int dwIcon);

// �����ӵ�ICON��ʾ����Ƶ��֡������
void XM_IconDisplay (xm_osd_framebuffer_t framebuffer);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif	// #ifndef _XM_ICON_MANAGE_H_




