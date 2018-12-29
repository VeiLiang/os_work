//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_video.c
//	  视频播放接口
//
//	Revision history
//
//		2012.09.06	ZhuoYongHong Initial version
//
//****************************************************************************
#include <string.h>
#include <xm_user.h>
#include <xm_base.h>
#include <xm_printf.h>
#include "app_video.h"
#include <xm_h264_codec.h>

// 顺序播放一个或多个(一组)语音
XMBOOL AP_VideoSendCommand (WORD wVideoCommand, DWORD dwVideoParam)
{	
	XMBOOL ret = 1;

	switch (wVideoCommand)
	{
		case AP_VIDEOCOMMAND_PLAY:
		{
			VIDEO_PLAY_PARAM *playParam = (VIDEO_PLAY_PARAM *)dwVideoParam;
			if(playParam == NULL)
				return 0;
			if(!playParam->pMainViewVideoFilePath)
				return 0;

#ifdef _DEBUG
			XM_printf ("VIDEOCOMMAND_PLAY %s %s\n", 
				playParam->pMainViewVideoFilePath, 
				playParam->pMainViewVideoFilePath2);
#endif
			break;
		}

		case AP_VIDEOCOMMAND_PAUSE:
			XM_printf ("VIDEOCOMMAND_CONTINUE_PAUSE\n");
			break;

		case AP_VIDEOCOMMAND_CONTINUE:
			XM_printf ("VIDEOCOMMAND_CONTINUE\n");
			break;

		case AP_VIDEOCOMMAND_SWITCH:
			if(dwVideoParam == AP_VIDEOSWITCHPARAM_MAINVICEVIEW)
			{
				XM_printf ("VIDEOCOMMAND_SWITCH 主/副画中面\n");
			}
			else if(dwVideoParam == AP_VIDEOSWITCHPARAM_MAINVIEW)
			{
				XM_printf ("VIDEOCOMMAND_SWITCH 主画面\n");
			}
			else if(dwVideoParam == AP_VIDEOSWITCHPARAM_VICEMAINVIEW)
			{
				XM_printf ("VIDEOCOMMAND_SWITCH 副/主画中面\n");
			}
			else if(dwVideoParam == AP_VIDEOSWITCHPARAM_VICEVIEW)
			{
				XM_printf ("VIDEOCOMMAND_SWITCH 副画面\n");
			}
			break;

		case AP_VIDEOCOMMAND_STOP:
			XMSYS_H264CodecPlayerStop ();
			XM_printf ("VIDEOCOMMAND_STOP\n");
			break;

		case AP_VIDEOCOMMAND_RECORD:
			XM_printf ("VIDEOCOMMAND_RECORD\n");
			break;

		default:
			break;
	}
	return ret;
}

