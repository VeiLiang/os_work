//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_video.h
//	  ��Ƶ���Žӿ�
//
//	Revision history
//
//		2012.09.06	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _XM_APP_VIDEO_H_
#define _XM_APP_VIDEO_H_

#if defined (__cplusplus)
	extern "C"{
#endif



// ���������
#define	AP_VIDEOCOMMAND_PLAY				0x0000
													// ��������	
													// dwVideoParam ָ��VIDEO_PLAY_PARAM�ṹ
													//		����ͼ����ͼ�ɶ���ΪNULL

typedef struct _VIDEO_PLAY_PARAM {
	char *		pMainViewVideoFilePath;		// ��Ƶ�����ļ���
	char *		pMainViewVideoFilePath2;	// ��Ƶ�����ļ���
} VIDEO_PLAY_PARAM;

#define	AP_VIDEOCOMMAND_PAUSE			0x0001
													// ��ͣ��ǰ��Ƶ����(��������Ƶ����)
													//	dwVideoParam ����Ϊ0

#define	AP_VIDEOCOMMAND_CONTINUE		0x0002
													// ������ͣ����Ƶ����
													//	dwVideoParam ����Ϊ0			


#define	AP_VIDEOCOMMAND_SWITCH			0x0003
													// �л���ǰ���ŵ���������ͼ(��������Ƶ���ż���¼����)
													// ���ڲ��Ż��¼�����з��͸����������/�������л�

// AP_VIDEOCOMMAND_SWITCH�Ĳ���dwVideoParam����
#define	AP_VIDEOSWITCHPARAM_MAINVICEVIEW			0		// ǰ(��)��(��)���л�
#define	AP_VIDEOSWITCHPARAM_MAINVIEW				1		// ����(ǰ)		
#define	AP_VIDEOSWITCHPARAM_VICEMAINVIEW			2		// ��(��)ǰ(��)���л�		
#define	AP_VIDEOSWITCHPARAM_VICEVIEW				3		// ����(��)		

#define	AP_VIDEOCOMMAND_STOP				0x0004
													// ֹͣ��ǰ��Ƶ���Ż���Ƶ��¼
													//	dwVideoParam ����Ϊ0

#define	AP_VIDEOCOMMAND_RECORD			0x0005
													// ��ʼ��Ƶ��¼(ֹͣʹ��AP_VIDEOCOMMAND_STOP)
													//	dwVideoParam ����Ϊ0





// ˳�򲥷�һ������(һ��)����
XMBOOL AP_VideoSendCommand (WORD wVideoCommand, DWORD dwVideoParam);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif	// #ifndef _XM_APP_VOICE_H_
