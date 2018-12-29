//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_video.h
//	  视频播放接口
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



// 播放命令定义
#define	AP_VIDEOCOMMAND_PLAY				0x0000
													// 播放命令	
													// dwVideoParam 指向VIDEO_PLAY_PARAM结构
													//		主视图或副视图可定义为NULL

typedef struct _VIDEO_PLAY_PARAM {
	char *		pMainViewVideoFilePath;		// 视频播放文件名
	char *		pMainViewVideoFilePath2;	// 视频播放文件名
} VIDEO_PLAY_PARAM;

#define	AP_VIDEOCOMMAND_PAUSE			0x0001
													// 暂停当前视频播放(仅用于视频播放)
													//	dwVideoParam 保留为0

#define	AP_VIDEOCOMMAND_CONTINUE		0x0002
													// 继续暂停的视频播放
													//	dwVideoParam 保留为0			


#define	AP_VIDEOCOMMAND_SWITCH			0x0003
													// 切换当前播放的主、副视图(可用于视频播放及记录播放)
													// 可在播放或记录进行中发送该命令进行主/副画面切换

// AP_VIDEOCOMMAND_SWITCH的参数dwVideoParam定义
#define	AP_VIDEOSWITCHPARAM_MAINVICEVIEW			0		// 前(主)后(副)画中画
#define	AP_VIDEOSWITCHPARAM_MAINVIEW				1		// 单显(前)		
#define	AP_VIDEOSWITCHPARAM_VICEMAINVIEW			2		// 后(主)前(副)画中画		
#define	AP_VIDEOSWITCHPARAM_VICEVIEW				3		// 单显(后)		

#define	AP_VIDEOCOMMAND_STOP				0x0004
													// 停止当前视频播放或视频记录
													//	dwVideoParam 保留为0

#define	AP_VIDEOCOMMAND_RECORD			0x0005
													// 开始视频记录(停止使用AP_VIDEOCOMMAND_STOP)
													//	dwVideoParam 保留为0





// 顺序播放一个或多个(一组)语音
XMBOOL AP_VideoSendCommand (WORD wVideoCommand, DWORD dwVideoParam);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif	// #ifndef _XM_APP_VOICE_H_
