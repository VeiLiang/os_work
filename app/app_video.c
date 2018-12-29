//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_video.c
//	  ��Ƶ���Žӿ�
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

// ˳�򲥷�һ������(һ��)����
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
				XM_printf ("VIDEOCOMMAND_SWITCH ��/��������\n");
			}
			else if(dwVideoParam == AP_VIDEOSWITCHPARAM_MAINVIEW)
			{
				XM_printf ("VIDEOCOMMAND_SWITCH ������\n");
			}
			else if(dwVideoParam == AP_VIDEOSWITCHPARAM_VICEMAINVIEW)
			{
				XM_printf ("VIDEOCOMMAND_SWITCH ��/��������\n");
			}
			else if(dwVideoParam == AP_VIDEOSWITCHPARAM_VICEVIEW)
			{
				XM_printf ("VIDEOCOMMAND_SWITCH ������\n");
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

