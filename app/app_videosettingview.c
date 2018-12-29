//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_videosetting.c
//	  ��Ƶ����
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"
#include "xm_h264_codec.h"

//#define	_XM_RECORDRESOLUTION_ENABLE_
#define _XM_TIMEWATERMARK_ENABLE_
#define _XM_G_SENSOR_ENABLE_
//#define _XM_URGENTRECORDTIME_ENABLE_
#define _XM_PARKMODE_ENABLE_
//#define _XM_LIGHTFREQ_ENABLE_
//#define _XM_EXPOSEURECOMPENSATION_ENABLE

#define	OSD_AUTO_HIDE_TIMEOUT			10000	

// ¼�����õ����ò˵��˳����, 0��ʾ��һ����Ŀ(���)
static enum  {
	VIDEO_MENU_VIDEOSEGMENTSIZE = 0,				// ¼��ֶ�ʱ��
	
#ifdef _XM_RCDDELAY_ENABLE_	
	VIDEO_MENU_RCDDELAY,								// ��¼��ʱ
#endif

#ifdef _XM_RECORDRESOLUTION_ENABLE_		// ʹ��¼��ֱ���������
	VIDEO_MENU_RECORDRESOLUTION,		// ¼��ֱ���
#endif

#ifdef _XM_G_SENSOR_ENABLE_				// ʹ��G-Sensor������
	VIDEO_MENU_COLLISIONSENSITIVITY,				// ��ײ������
#endif

#ifdef _XM_URGENTRECORDTIME_ENABLE_				// ����¼��ʱ��
	VIDEO_MENU_URGENTRECORDTIME,					// ����¼��ʱ��
#endif

#ifdef _XM_EXPOSEURECOMPENSATION_ENABLE
     VIDEO_MENU_EXPOSEURECOMPENSATION,
#endif
     
#ifdef _XM_LIGHTFREQ_ENABLE_	
     VIDEO_MENU_LIGHTFREQ,
#endif

#ifdef _XM_PARKMODE_ENABLE_
      VIDEO_MENU_PARKMODE,
#endif
	  
#ifdef _XM_TIMEWATERMARK_ENABLE_			// ˮӡ������
	VIDEO_MENU_TIMEWATERMARK,						// ˮӡ����
 #endif
 
	VIDEO_MENU_COUNT									// ������Ŀ��
} VIDEOSETTING_MENU_DEFINE;


// ˽�������
#define	VIDEOSETTINGVIEW_COMMAND_MODIFY		0
#define	VIDEOSETTINGVIEW_COMMAND_SYSTEM		1
#define	VIDEOSETTINGVIEW_COMMAND_RETURN		2

// �����á����ڰ�ť�ؼ�����
#define	VIDEOSETTINGVIEWBTNCOUNT	2
static const XMBUTTONINFO videoRecordBtn[VIDEOSETTINGVIEWBTNCOUNT] = {
	 /*
	 {	
		VK_AP_MENU,		VIDEOSETTINGVIEW_COMMAND_MODIFY,	AP_ID_COMMON_MENU,	AP_ID_BUTTON_MODIFY
	 },
	 */
	{	
		VK_AP_MENU,	VIDEOSETTINGVIEW_COMMAND_SYSTEM,	AP_ID_COMMON_MODE,	AP_ID_VIDEOSETTING_BUTTON_SYSTEM
	},
	{	
		VK_AP_MODE,		VIDEOSETTINGVIEW_COMMAND_RETURN,	AP_ID_COMMON_OK,	AP_ID_BUTTON_RETURN
	},
};

typedef struct tagVIDEOSETTINGVIEWDATA {
	int				nTopItem;					// ��һ�����ӵĲ˵���
	int				nCurItem;					// ��ǰѡ��Ĳ˵���
	int				nItemCount;				// �˵������
	int				nTouchItem;				// ��ǰ������, -1 ��ʾû��
	
	#ifdef _XM_PARKMODE_ENABLE_
    HANDLE			hMoniorSwitchControl;		//ͣ�����
    #endif
	
	APPMENUOPTION	menuOption;				// �˵�ѡ��
	XMBUTTONCONTROL	btnControl;

} VIDEOSETTINGVIEWDATA;

// ���ؿؼ��Ļص�����
static void Videosetting_switch_callback (void *private_data, unsigned int state)
{
	unsigned int type = (unsigned int)private_data;
	if(type == APPMENUITEM_PARKMONITOR)
	{
		if(state == 1)
		{
			AP_SetMenuItem (type, state);
			// ����˵����õ�����洢�豸
			AP_SaveMenuData (&AppMenuData);
		}
		else
		{
			AP_SetMenuItem (type, state);
			// ����˵����õ�����洢�豸
			AP_SaveMenuData (&AppMenuData);
		}
	}
	
}
VOID VideoSettingViewOnEnter (XMMSG *msg)
{
     XMPOINT pt;
	if(msg->wp == 0)
	{
		// ����δ��������һ�ν���
		VIDEOSETTINGVIEWDATA *settingViewData;

		// ����˽�����ݾ��
		settingViewData = XM_calloc (sizeof(VIDEOSETTINGVIEWDATA));
		if(settingViewData == NULL)
		{
			XM_printf ("settingViewData XM_calloc failed\n");
			// ʧ�ܷ��ص������ߴ���
			XM_PullWindow (0);
			return;
		}
		
		// ��ʼ��˽������
		settingViewData->nCurItem = 0;
		settingViewData->nTopItem = 0;

		settingViewData->nItemCount = VIDEO_MENU_COUNT;
		settingViewData->nTouchItem = -1;
		#ifdef _XM_PARKMODE_ENABLE_
        // Switch���ؿؼ���ʼ�� (ͣ�����)
		settingViewData->hMoniorSwitchControl = AP_SwitchButtonControlInit (
			XMHWND_HANDLE(VideoSettingView),
			&pt,
			AppMenuData.parkmonitor,
			Videosetting_switch_callback,
			(void *)APPMENUITEM_PARKMONITOR);
		#endif
		
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(VideoSettingView), settingViewData);

		// ��ť�ؼ���ʼ��
		AP_ButtonControlInit (&settingViewData->btnControl, VIDEOSETTINGVIEWBTNCOUNT, 
			XMHWND_HANDLE(VideoSettingView), videoRecordBtn);

	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("VideoSettingView Pull\n");
	}
	
	// ������ʱ�������ڲ˵�������
	// ����x��Ķ�ʱ��
	XM_SetTimer (XMTIMER_VIDEOSETTING, OSD_AUTO_HIDE_TIMEOUT);
}

VOID VideoSettingViewOnLeave (XMMSG *msg)
{
	// ɾ����ʱ��
	XM_KillTimer (XMTIMER_VIDEOSETTING);
	
	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		VIDEOSETTINGVIEWDATA *settingViewData = (VIDEOSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoSettingView));
		// �ͷ����з������Դ
		if(settingViewData)
		{
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&settingViewData->btnControl);

			#ifdef _XM_PARKMODE_ENABLE_
			if(settingViewData->hMoniorSwitchControl)
				AP_SwitchButtonControlExit (settingViewData->hMoniorSwitchControl);
			#endif
			
			// �ͷ�˽�����ݾ��
			XM_free (settingViewData);
		}
		// �����Ӵ���˽�����ݾ��Ϊ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(VideoSettingView), NULL);
		XM_printf ("VideoSettingView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("VideoSettingView Push\n");
	}
}



VOID VideoSettingViewOnPaint (XMMSG *msg)
{
	int nItem, nLastItem;
	XMRECT rc, rect, rcArrow;
	int i;
	int nVisualCount;
	float scale_factor;		// ˮƽ��������
	HANDLE hwnd = XMHWND_HANDLE(VideoSettingView);

	XMCOORD menu_name_x, menu_data_x, menu_flag_x;	// �˵�����⡢��ֵ����ʶ��x����
	unsigned int old_alpha;
	DWORD dwMenuID;

	VIDEOSETTINGVIEWDATA *settingViewData = (VIDEOSETTINGVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(settingViewData == NULL)
		return;

	XM_GetDesktopRect (&rc);
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;

	// ����ˮƽ��������(UI��320X240������)
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);

	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	menu_name_x = (XMCOORD)(APP_POS_ITEM5_MENUNAME_X * scale_factor);
	menu_data_x = (XMCOORD)(APP_POS_ITEM5_MENUDATA_X * scale_factor);
	menu_flag_x = (XMCOORD)(APP_POS_ITEM5_MENUFLAG_X * scale_factor);

	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);
	
	// --------------------------------------
	//
	// ********* 1 ��ʾ����������Ϣ *********
	//
	// --------------------------------------
	AP_DrawTitlebarControl (hwnd, AP_NULLID, AP_ID_VIDEOSETTING_TITLE);

	// ��ʾ�˵���ˮƽ�ָ���
	rect = rc;
	for (i = 0; i < nVisualCount; i++)
	{
		rect.left = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_X * scale_factor);
		rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + (i+1) * APP_POS_ITEM5_LINEHEIGHT);
		AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMSPLITLINE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	}

	// ��䵱ǰѡ�����
	rect = rc;
	rect.left = 0;
	rect.top = (XMCOORD)(APP_POS_ITEM5_SPLITLINE_Y + 1 + (settingViewData->nCurItem - settingViewData->nTopItem)  * APP_POS_ITEM5_LINEHEIGHT);
	AP_RomImageDrawByMenuID (AP_ID_COMMON_MENUITEMBACKGROUND, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	// --------------------------------------
	//
	// ********* 2 ��ʾ�˵����� *************
	//
	// --------------------------------------
	rect = rc;
	rect.top = APP_POS_ITEM5_MENUNAME_Y;
	nLastItem = settingViewData->nItemCount;
	if(nLastItem > (settingViewData->nTopItem + nVisualCount))
		nLastItem = (settingViewData->nTopItem + nVisualCount);
	for (nItem = settingViewData->nTopItem; nItem < nLastItem; nItem ++)
	{
		rect.left = menu_name_x;
		rcArrow = rect;
		switch (nItem)
		{
#ifdef _XM_RECORDRESOLUTION_ENABLE_		// ʹ��¼��ֱ���������
			case VIDEO_MENU_RECORDRESOLUTION:	// ��Ƶ�ֱ���
				AP_RomImageDrawByMenuID (AP_ID_VIDEOSETTING_RECORDRESOLUTION, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
			    //��ʾѡ��
				rect.left = menu_data_x;
				if(AppMenuData.video_resolution == 0)
					dwMenuID = AP_ID_VIDEOSETTING_RECORDRESOLUTION_1080P_30;
				else if (AppMenuData.video_resolution == 1)
					dwMenuID = AP_ID_VIDEOSETTING_RECORDRESOLUTION_720P_30;
				else
					dwMenuID = AP_ID_VIDEOSETTING_RECORDRESOLUTION_720P_60;

				AP_RomImageDrawByMenuID (dwMenuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
                //��ʾ���ҵı��
				rcArrow = rect;
				rcArrow.left = menu_flag_x;
				AP_RomImageDrawByMenuID (AP_ID_COMMON_RIGHT_ARROW, hwnd, &rcArrow, XMGIF_DRAW_POS_LEFTTOP);
				break;
#endif	// #ifdef _XM_RECORDRESOLUTION_ENABLE_		// ʹ��¼��ֱ���������

			case VIDEO_MENU_VIDEOSEGMENTSIZE:	// "¼��ֶ�ʱ��"
				AP_RomImageDrawByMenuID (AP_ID_VIDEOSETTING_VIDEOSEGMENTSIZE, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				rect.left = menu_data_x;
				if(AppMenuData.video_time_size== 0)
					dwMenuID = AP_ID_VIDEOSETTING_VIDEOSEGMENTSIZE_2_MINUTE;
				else if (AppMenuData.video_time_size == 1)
					dwMenuID = AP_ID_VIDEOSETTING_VIDEOSEGMENTSIZE_3_MINUTE;
				else //if (AppMenuData.video_time_size == 2)
					dwMenuID = AP_ID_VIDEOSETTING_VIDEOSEGMENTSIZE_5_MINUTE;
				AP_RomImageDrawByMenuID (dwMenuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
                // ��ʾ���ҵı��
				rcArrow = rect;
				rcArrow.left = menu_flag_x;
				AP_RomImageDrawByMenuID (AP_ID_COMMON_RIGHT_ARROW, hwnd, &rcArrow, XMGIF_DRAW_POS_LEFTTOP);
				break;
				
#ifdef _XM_G_SENSOR_ENABLE_
			case VIDEO_MENU_COLLISIONSENSITIVITY:	// ����ײ�����ȡ�
				AP_RomImageDrawByMenuID (AP_ID_VIDEOSETTING_COLLISIONSENSITIVITY, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				rect.left = menu_data_x;
				if(AppMenuData.collision_sensitivity== 0)
					dwMenuID = AP_ID_SETTING_CLOSE;
				else if (AppMenuData.collision_sensitivity == 1)
					dwMenuID = AP_ID_VIDEOSETTING_COLLISIONSENSITIVITY_LOW;
				else if (AppMenuData.collision_sensitivity == 2)
					dwMenuID = AP_ID_VIDEOSETTING_COLLISIONSENSITIVITY_MEDIUM;
				else //if (AppMenuData.collision_sensitivity == 3)
					dwMenuID = AP_ID_VIDEOSETTING_COLLISIONSENSITIVITY_HIGH;
				AP_RomImageDrawByMenuID (dwMenuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
                // ��ʾ���ҵı��
				break;
#endif

		#ifdef _XM_RCDDELAY_ENABLE_		
			case VIDEO_MENU_RCDDELAY://  ����¼��ʱ��
				AP_RomImageDrawByMenuID (AP_ID_VIDEOSETTING_RCDDELAY, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				rect.left = menu_data_x;
                    
				if(AppMenuData.record_delay== 0)
					dwMenuID = AP_ID_VIDEOSETTING_RCDDELAY_S0;
				else if (AppMenuData.record_delay == 1)
					dwMenuID = AP_ID_VIDEOSETTING_RCDDELAY_S3;
				else if (AppMenuData.record_delay == 2)
					dwMenuID = AP_ID_VIDEOSETTING_RCDDELAY_S5;
				else if (AppMenuData.record_delay == 3)
					dwMenuID = AP_ID_VIDEOSETTING_RCDDELAY_S10;
				else //if (AppMenuData.record_delay == 4)
					dwMenuID = AP_ID_VIDEOSETTING_RCDDELAY_S20;
				AP_RomImageDrawByMenuID (dwMenuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
                // ��ʾ���ҵı��
				break;
                #endif
				
#ifdef _XM_TIMEWATERMARK_ENABLE_			// ˮӡ������
			case VIDEO_MENU_TIMEWATERMARK:	// "ʱ��ˮӡ"
			AP_RomImageDrawByMenuID (AP_ID_VIDEOSETTING_TIMEWATERMARK, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				rect.left = menu_data_x;
				if(AppMenuData.time_stamp== 1)
					dwMenuID = AP_ID_SETTING_OPEN;
				else
					dwMenuID = AP_ID_SETTING_CLOSE;

				AP_RomImageDrawByMenuID (dwMenuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				break;
#endif	// #ifdef _XM_TIMEWATERMARK_ENABLE_			// ˮӡ������

#ifdef _XM_URGENTRECORDTIME_ENABLE_				// ����¼��ʱ��				
			case VIDEO_MENU_URGENTRECORDTIME:	// "����¼��ʱ��"
			//	AppRes = AP_AppRes2RomRes (AP_ID_VIDEOSETTING_URGENTRECORDTIME);
			//	XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
			//	AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, 
			//			hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				AP_RomImageDrawByMenuID (AP_ID_VIDEOSETTING_URGENTRECORDTIME, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				rect.left = menu_data_x;

				if(AppMenuData.urgent_record_time == 0)
				//	AppRes = AP_AppRes2RomRes (AP_ID_VIDEOSETTING_RECORDTIMEALARM_MIN5);
					dwMenuID = AP_ID_VIDEOSETTING_RECORDTIMEALARM_MIN5;
				else if (AppMenuData.urgent_record_time == 1)
					//AppRes = AP_AppRes2RomRes (AP_ID_VIDEOSETTING_RECORDTIMEALARM_MIN10);
					dwMenuID = AP_ID_VIDEOSETTING_RECORDTIMEALARM_MIN10;
				else //if (AppMenuData.urgent_record_time == 2)
					//AppRes = AP_AppRes2RomRes (AP_ID_VIDEOSETTING_RECORDTIMEALARM_MIN15);
					dwMenuID = AP_ID_VIDEOSETTING_RECORDTIMEALARM_MIN15;

		//		XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
		//		AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, 
		//				hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				AP_RomImageDrawByMenuID (dwMenuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
			    			
				break;
#endif
        #ifdef _XM_PARKMODE_ENABLE_
		         case VIDEO_MENU_PARKMODE:
				 AP_RomImageDrawByMenuID (AP_ID_VIDEOSETTING_MONITOR, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				rect.left = menu_data_x;
				
				if(AppMenuData.parkmonitor == AP_SETTING_PARK_MONITOR_OFF)
					dwMenuID = AP_ID_SETTING_CLOSE;
				else
					dwMenuID = AP_ID_SETTING_OPEN;
				AP_RomImageDrawByMenuID (dwMenuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				
				//AP_SwitchButtonControlMove (settingViewData->hMoniorSwitchControl, menu_flag_x - 51, rect.top - 5);
				//AP_SwitchButtonControlMessageHandler (settingViewData->hMoniorSwitchControl, msg);
				     break;
		#endif

		#ifdef _XM_EXPOSEURECOMPENSATION_ENABLE
			   case VIDEO_MENU_EXPOSEURECOMPENSATION:
				 AP_RomImageDrawByMenuID (AP_ID_VIDEOSETTING_EXPOSURECOMPENSATION, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				rect.left = menu_data_x;
				
				if(AppMenuData.exposure_compensation == AP_SETTING_EV_0)
					dwMenuID = AP_ID_VIDEOSETTING_EXPOSURECOMPENSATION_POSI_0;
				else if(AppMenuData.exposure_compensation == AP_SETTING_EV_MINUS_2)
					dwMenuID = AP_ID_VIDEOSETTING_EXPOSURECOMPENSATION_NEGA_2;
				else if(AppMenuData.exposure_compensation == AP_SETTING_EV_MINUS_1)
					dwMenuID = AP_ID_VIDEOSETTING_EXPOSURECOMPENSATION_NEGA_1;
				else if(AppMenuData.exposure_compensation == AP_SETTING_EV_1)
					dwMenuID = AP_ID_VIDEOSETTING_EXPOSURECOMPENSATION_POSI_1;
				else 
					dwMenuID = AP_ID_VIDEOSETTING_EXPOSURECOMPENSATION_POSI_2;
				AP_RomImageDrawByMenuID (dwMenuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
                break;
		#endif
		
#ifdef _XM_LIGHTFREQ_ENABLE_	
				 case VIDEO_MENU_LIGHTFREQ:
				 AP_RomImageDrawByMenuID (AP_ID_VIDEOSETTING_LIGHTFREP, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				// ��ʾѡ��
				rect.left = menu_data_x;
				
				if(AppMenuData.light_freq == AP_SETTING_LIGHTFREQ_CLOSE)
					dwMenuID = AP_ID_VIDEOSETTING_LIGHTFREQ_CLOSE;
				else if(AppMenuData.light_freq == AP_SETTING_LIGHT_FREQ_50HZ)
					dwMenuID = AP_ID_VIDEOSETTING_LIGHTFREQ_50HZ;
		
				else 
					dwMenuID = AP_ID_VIDEOSETTING_LIGHTFREQ_60HZ;
				AP_RomImageDrawByMenuID (dwMenuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
				
				AP_RomImageDrawByMenuID (dwMenuID, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
                          break;	
   #endif
			default:

				break;
		}
		// ��ʾ���ҵı��
		rcArrow = rect;
		rcArrow.left = menu_flag_x;
	//	AppRes = AP_AppRes2RomRes (AP_ID_COMMON_RIGHT_ARROW);
	//	XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, hwnd, &rcArrow, XMGIF_DRAW_POS_LEFTTOP);
	//	AP_RomImageDraw (AppRes->rom_offset, AppRes->res_length, 
	//					hwnd, &rcArrow, XMGIF_DRAW_POS_LEFTTOP);
	  
		  AP_RomImageDrawByMenuID (AP_ID_COMMON_RIGHT_ARROW, hwnd, &rcArrow, XMGIF_DRAW_POS_LEFTTOP);
		  rect.top += APP_POS_ITEM5_LINEHEIGHT;
	}
	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
	AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);

	XM_SetWindowAlpha (hwnd, (unsigned char)old_alpha);
}


VOID VideoSettingViewOnKeyDown (XMMSG *msg)
{
	VIDEOSETTINGVIEWDATA *settingViewData = (VIDEOSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoSettingView));
	int nVisualCount;
	XMRECT rc;
	if(settingViewData == NULL)
		return;

	XM_GetWindowRect (XMHWND_HANDLE(VideoSettingView), &rc);
	nVisualCount = rc.bottom - rc.top + 1 - APP_POS_MENU_Y - (240 - APP_POS_BUTTON_Y);
	nVisualCount /= APP_POS_ITEM5_LINEHEIGHT;
	// ������
	XM_Beep (XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
		case VK_AP_SWITCH:	// �����л���
			// ��"¼�����"�����У�MENU��Power��Switch��MODE��������Ϊ��ť������
			// �˴������ĸ����¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			XM_SetTimer (XMTIMER_VIDEOSETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
			
        #if 1
		case VK_AP_UP:		// ����¼���
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			settingViewData->nCurItem --;
			if(settingViewData->nCurItem < 0)
			{
				// �۽������һ��
				settingViewData->nCurItem = (WORD)(settingViewData->nItemCount - 1);
				settingViewData->nTopItem = 0;
				while ( (settingViewData->nCurItem - settingViewData->nTopItem) >= nVisualCount )
				{
					settingViewData->nTopItem ++;
				}
			}
			else
			{
				if(settingViewData->nTopItem > settingViewData->nCurItem)
					settingViewData->nTopItem = settingViewData->nCurItem;
			}

			/*
			if (settingViewData->nCurItem)
			{
				settingViewData->nCurItem--;
			}
			else if (settingViewData->nTopItem)
			{
				settingViewData->nTopItem--;
			}*/

			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_VIDEOSETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;

		case VK_AP_DOWN:	// 
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);

			settingViewData->nCurItem ++;	
			if(settingViewData->nCurItem >= settingViewData->nItemCount)
			{
				// �۽�����һ��
				settingViewData->nTopItem = 0;
				settingViewData->nCurItem = 0;
			}
			else
			{
				while ( (settingViewData->nCurItem - settingViewData->nTopItem) >= nVisualCount )
				{
					settingViewData->nTopItem ++;
				}
			}
			
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_VIDEOSETTING, OSD_AUTO_HIDE_TIMEOUT);
			/*
			if ((settingViewData->nCurItem + settingViewData->nTopItem) < (settingViewData->nItemCount- 1))
			{
				if (settingViewData->nCurItem < (APP_MENUOPTIONVIEW_ITEM_COUNT-1))
				{
					settingViewData->nCurItem ++;
				}
				else
				{
					settingViewData->nTopItem++;
				}

				// ˢ��
				XM_InvalidateWindow ();
				XM_UpdateWindow ();
			}
			*/
			break;
			#endif
	}

}

VOID VideoSettingViewOnKeyUp (XMMSG *msg)
{
	VIDEOSETTINGVIEWDATA *settingViewData;
	settingViewData = (VIDEOSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoSettingView));
	if(settingViewData == NULL)
		return;

	switch(msg->wp)
	{
	    case VK_AP_SWITCH:
		   	XM_PostMessage (XM_COMMAND,VIDEOSETTINGVIEW_COMMAND_MODIFY , settingViewData->nCurItem);// settingViewData->btnControl.pBtnInfo[0].wCommand
		   	break;
		
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
			// ��"ʱ������"�����У�MENU��MODE��������Ϊ��ť������
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg);
			break;
	}
}

// MIC�����ر�
static const DWORD dwOnOffMenuOption[2] = {
	AP_ID_SETTING_CLOSE,
	AP_ID_SETTING_OPEN
};


// "¼��ֶ�ʱ��"
static const DWORD dwVideoSizeMenuOption[] = {
	AP_ID_VIDEOSETTING_VIDEOSEGMENTSIZE_2_MINUTE,
	AP_ID_VIDEOSETTING_VIDEOSEGMENTSIZE_3_MINUTE,
	AP_ID_VIDEOSETTING_VIDEOSEGMENTSIZE_5_MINUTE
};

static void VideoSizeMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	AppMenuData.video_time_size = (BYTE)dMenuOptionSelect;
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
}

#ifdef _XM_G_SENSOR_ENABLE_
// "��ײ������"
static const DWORD dwCollisionMenuOption[] = {
	AP_ID_SETTING_CLOSE,
	AP_ID_VIDEOSETTING_COLLISIONSENSITIVITY_LOW,
	AP_ID_VIDEOSETTING_COLLISIONSENSITIVITY_MEDIUM,
	AP_ID_VIDEOSETTING_COLLISIONSENSITIVITY_HIGH,
};

static void CollisionMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	AppMenuData.collision_sensitivity = (BYTE)dMenuOptionSelect;
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
}
#endif	// _XM_G_SENSOR_ENABLE_


// "��¼��ʱ"
static const DWORD dwRcdRelayMenuOption[] = {
	AP_ID_VIDEOSETTING_RCDDELAY_S0,
	AP_ID_VIDEOSETTING_RCDDELAY_S3,
	AP_ID_VIDEOSETTING_RCDDELAY_S5,
	AP_ID_VIDEOSETTING_RCDDELAY_S10,
	AP_ID_VIDEOSETTING_RCDDELAY_S20, 
};

static void RcdDelayMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	//if(dMenuOptionSelect < 0)
		//dMenuOptionSelect = 0;
	//else if(dMenuOptionSelect >= sizeof(dwRcdRelayMenuOption)/sizeof(dwRcdRelayMenuOption[0]))
		//dMenuOptionSelect = sizeof(dwRcdRelayMenuOption)/sizeof(dwRcdRelayMenuOption[0]) - 1;
      
	AppMenuData.record_delay= (BYTE)dMenuOptionSelect;
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
}

// ����¼��ʱ��
static const DWORD dwUrgentRecordTimeMenuOption[] = {
	AP_ID_VIDEOSETTING_RECORDTIMEALARM_MIN5,
	AP_ID_VIDEOSETTING_RECORDTIMEALARM_MIN10,
	AP_ID_VIDEOSETTING_RECORDTIMEALARM_MIN15,
};

static void UrgentRecordTimeMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	AppMenuData.urgent_record_time = (BYTE)dMenuOptionSelect;
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
}

// ¼��ֱ���
static const DWORD dwRecordResolution[] = {
	AP_ID_VIDEOSETTING_RECORDRESOLUTION_1080P_30,
	AP_ID_VIDEOSETTING_RECORDRESOLUTION_720P_30,
	AP_ID_VIDEOSETTING_RECORDRESOLUTION_720P_60
};

static void RecordResolutionMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	if(dMenuOptionSelect < 0)
		dMenuOptionSelect = 0;
	else if(dMenuOptionSelect >= sizeof(dwRecordResolution)/sizeof(dwRecordResolution[0]))
		dMenuOptionSelect = sizeof(dwRecordResolution)/sizeof(dwRecordResolution[0]) - 1;
	
	if(AP_GetMenuItem(APPMENUITEM_VIDEO_RESOLUTION) == (BYTE)dMenuOptionSelect)
		return;
	
	if(AppMenuData.video_resolution == (BYTE)dMenuOptionSelect)
		return;
	
	AP_SetMenuItem (APPMENUITEM_VIDEO_RESOLUTION, (BYTE)dMenuOptionSelect);
	
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
	
	// ��������������, Ӧ���µķֱ���/֡������
	if(XMSYS_H264CodecStop() == 0)
	{		
		XMSYS_H264CodecSetVideoFormat (dMenuOptionSelect);
        printf("1111111111111111111111111111111 RecordResolutionMenuOptionCB\n");
		XMSYS_H264CodecRecorderStart ();
	}	
}
// �عⲹ��ѡ��
static const DWORD dwExposure_compensationMenuOption[] = 
{
	AP_ID_VIDEOSETTING_EXPOSURECOMPENSATION_NEGA_2,		// -2
	AP_ID_VIDEOSETTING_EXPOSURECOMPENSATION_NEGA_1,		// -1
	AP_ID_VIDEOSETTING_EXPOSURECOMPENSATION_POSI_0,		// 0
	AP_ID_VIDEOSETTING_EXPOSURECOMPENSATION_POSI_1,		// +1
	AP_ID_VIDEOSETTING_EXPOSURECOMPENSATION_POSI_2,		// +2
};

static void Exposure_compensationMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	AppMenuData.exposure_compensation = (BYTE)dMenuOptionSelect;
	AP_SetMenuItem (APPMENUITEM_EV, (BYTE)dMenuOptionSelect);
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
}
// ��ԴƵ��
static const DWORD dwLightFreqMenuOption[] = 
{
	AP_ID_VIDEOSETTING_LIGHTFREQ_CLOSE,
	AP_ID_VIDEOSETTING_LIGHTFREQ_50HZ,
	AP_ID_VIDEOSETTING_LIGHTFREQ_60HZ,
};
static void LightFreqMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	AppMenuData.light_freq = (BYTE)dMenuOptionSelect;
	AP_SetMenuItem (APPMENUITEM_LIGHT_FREQ, (BYTE)dMenuOptionSelect);
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
}
static const DWORD dwParkMonitorMenuOption[] = {
	AP_ID_SETTING_CLOSE,
	AP_ID_SETTING_OPEN,

};
static void ParkMonitorMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	AP_SetMenuItem (APPMENUITEM_PARKMONITOR, (BYTE)dMenuOptionSelect);
	//AppMenuData.lcd = (BYTE)dMenuOptionSelect;
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
}
static const DWORD dwTimeStampMenuOption[] = {
	AP_ID_SETTING_CLOSE,
	AP_ID_SETTING_OPEN,

};
static void TimeStampMenuOptionCB (VOID *lpUserData, int dMenuOptionSelect)
{
	AP_SetMenuItem (APPMENUITEM_TIME_STAMP, (BYTE)dMenuOptionSelect);
	//AppMenuData.lcd = (BYTE)dMenuOptionSelect;
	// ����˵����õ�����洢�豸
	AP_SaveMenuData (&AppMenuData);
}
VOID VideoSettingViewOnCommand (XMMSG *msg)
{
	VIDEOSETTINGVIEWDATA *settingViewData;
	int	SelectedItem;
	 
	settingViewData = (VIDEOSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoSettingView));
	if(settingViewData == NULL)
		return;

	SelectedItem = settingViewData->nCurItem;// + settingViewData->nTopItem;

	switch(msg->wp)
	{
		case VIDEOSETTINGVIEW_COMMAND_MODIFY:
			
			if(SelectedItem == VIDEO_MENU_VIDEOSEGMENTSIZE)
			{    XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				// "¼��ֶ�ʱ��"
				settingViewData->menuOption.dwWindowIconId = 0xFFFFFFFF;
				settingViewData->menuOption.dwWindowTextId = AP_ID_VIDEOSETTING_VIDEOSEGMENTSIZE_TITLE;
				settingViewData->menuOption.wMenuOptionCount = sizeof(dwVideoSizeMenuOption)/sizeof(dwVideoSizeMenuOption[0]);
				settingViewData->menuOption.wMenuOptionSelect = AppMenuData.video_time_size;
				settingViewData->menuOption.lpdwMenuOptionId = (DWORD*)&dwVideoSizeMenuOption;
				settingViewData->menuOption.fpMenuOptionCB = VideoSizeMenuOptionCB;
				settingViewData->menuOption.lpUserData = NULL;

				//settingViewData->nCurItem = -1;
				// �����˵�ѡ���б���
				AP_OpenMenuOptionView (&settingViewData->menuOption);
			}

#ifdef _XM_RECORDRESOLUTION_ENABLE_		// ʹ��¼��ֱ���������
			else if(SelectedItem == VIDEO_MENU_RECORDRESOLUTION)
			{
			    XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				// ¼��ֱ���
				settingViewData->menuOption.dwWindowIconId = 0xFFFFFFFF;
				settingViewData->menuOption.dwWindowTextId = AP_ID_VIDEOSETTING_RECORDRESOLUTION_TITLE;
				settingViewData->menuOption.wMenuOptionCount = sizeof(dwRecordResolution)/sizeof(dwRecordResolution[0]);
				settingViewData->menuOption.wMenuOptionSelect = AppMenuData.video_resolution;
				settingViewData->menuOption.lpdwMenuOptionId = (DWORD*)&dwRecordResolution;
				settingViewData->menuOption.fpMenuOptionCB = RecordResolutionMenuOptionCB;
				settingViewData->menuOption.lpUserData = NULL;

				//settingViewData->nCurItem = -1;
				// �����˵�ѡ���б���
				AP_OpenMenuOptionView (&settingViewData->menuOption);
			}
#endif	// #ifdef _XM_RECORDRESOLUTION_ENABLE_		// ʹ��¼��ֱ���������


#ifdef _XM_G_SENSOR_ENABLE_
			else if(SelectedItem == VIDEO_MENU_COLLISIONSENSITIVITY)
			{
			      XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				// "��ײ������"
				settingViewData->menuOption.dwWindowIconId = 0xFFFFFFFF;
				settingViewData->menuOption.dwWindowTextId = AP_ID_VIDEOSETTING_COLLISIONSENSITIVITY_TITLE;
				settingViewData->menuOption.wMenuOptionCount = sizeof(dwCollisionMenuOption)/sizeof(dwCollisionMenuOption[0]);
				settingViewData->menuOption.wMenuOptionSelect = AppMenuData.collision_sensitivity;
				settingViewData->menuOption.lpdwMenuOptionId = (DWORD*)&dwCollisionMenuOption;
				settingViewData->menuOption.fpMenuOptionCB = CollisionMenuOptionCB;
				settingViewData->menuOption.lpUserData = NULL;
				//settingViewData->nCurItem = -1;
				// �����˵�ѡ���б���
				AP_OpenMenuOptionView (&settingViewData->menuOption);
			}
#endif	// _XM_G_SENSOR_ENABLE_

#ifdef _XM_RCDDELAY_ENABLE_
			else if(SelectedItem == VIDEO_MENU_RCDDELAY)
			{
			      XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				// ����ʱ��¼ʱ�䡱
				settingViewData->menuOption.dwWindowIconId = 0xFFFFFFFF;
				settingViewData->menuOption.dwWindowTextId = AP_ID_VIDEOSETTING_RCDDELAY_TITLE;
				settingViewData->menuOption.wMenuOptionCount = sizeof(dwRcdRelayMenuOption)/sizeof(dwRcdRelayMenuOption[0]);
				settingViewData->menuOption.wMenuOptionSelect = AppMenuData.record_delay;
				settingViewData->menuOption.lpdwMenuOptionId = (DWORD*)&dwRcdRelayMenuOption;
				settingViewData->menuOption.fpMenuOptionCB = RcdDelayMenuOptionCB;
				settingViewData->menuOption.lpUserData = NULL;

				//settingViewData->nCurItem = -1;
				// �����˵�ѡ���б���
				AP_OpenMenuOptionView (&settingViewData->menuOption);
			}
#endif

#ifdef _XM_URGENTRECORDTIME_ENABLE_				// ����¼��ʱ��			
			else if(SelectedItem == VIDEO_MENU_URGENTRECORDTIME)
			{
			      XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				// ����¼��ʱ��
				settingViewData->menuOption.dwWindowIconId = 0xFFFFFFFF;
				settingViewData->menuOption.dwWindowTextId = AP_ID_VIDEOSETTING_URGENTRECORD_TITLE;
				settingViewData->menuOption.wMenuOptionCount = sizeof(dwUrgentRecordTimeMenuOption)/sizeof(dwUrgentRecordTimeMenuOption[0]);
				settingViewData->menuOption.wMenuOptionSelect = AppMenuData.urgent_record_time;
				settingViewData->menuOption.lpdwMenuOptionId = (DWORD*)&dwUrgentRecordTimeMenuOption;
				settingViewData->menuOption.fpMenuOptionCB = UrgentRecordTimeMenuOptionCB;
				settingViewData->menuOption.lpUserData = NULL;
				//settingViewData->nCurItem = -1;
				// �����˵�ѡ���б���
				AP_OpenMenuOptionView (&settingViewData->menuOption);
			}
#endif

#ifdef _XM_TIMEWATERMARK_ENABLE_			// ˮӡ������
			else if(SelectedItem == VIDEO_MENU_TIMEWATERMARK)
			{
			
				// "ʱ��ˮӡ"
				/*
				settingViewData->menuOption.dwWindowIconId = 0xFFFFFFFF;
				settingViewData->menuOption.dwWindowTextId = AP_ID_VIDEOSETTING_TIMEWATERMARK_TITLE;
				settingViewData->menuOption.wMenuOptionCount = 2;
				settingViewData->menuOption.wMenuOptionSelect = AppMenuData.time_stamp;
				settingViewData->menuOption.lpdwMenuOptionId = (DWORD*)&dwOnOffMenuOption;
				settingViewData->menuOption.fpMenuOptionCB = TimeStampMenuOptionCB;
				settingViewData->menuOption.lpUserData = NULL;
				// �����˵�ѡ���б���
				AP_OpenMenuOptionView (&settingViewData->menuOption);
				*/

				//XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				//XM_PushWindowEx (XMHWND_HANDLE(WaterMarkSettingView), 0);

				XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				settingViewData->menuOption.dwWindowIconId = 0xFFFFFFFF;
				settingViewData->menuOption.dwWindowTextId = AP_ID_VIDEOSETTING_TIMEWATERMARK_TITLE;
				settingViewData->menuOption.wMenuOptionCount = sizeof(dwTimeStampMenuOption)/sizeof(DWORD);
				settingViewData->menuOption.wMenuOptionSelect = AppMenuData.time_stamp;
				settingViewData->menuOption.lpdwMenuOptionId = (DWORD*)&dwTimeStampMenuOption;
				settingViewData->menuOption.fpMenuOptionCB = TimeStampMenuOptionCB;
				settingViewData->menuOption.lpUserData = NULL;
				// �����˵�ѡ���б���
				AP_OpenMenuOptionView (&settingViewData->menuOption);
			}
#endif	// #ifdef _XM_TIMEWATERMARK_ENABLE_			// ˮӡ������

 #ifdef _XM_PARKMODE_ENABLE_
			else if (SelectedItem == VIDEO_MENU_PARKMODE)
			{
			    #if 0
                // MIC�����ر�
				XMMSG key_msg;
				key_msg.message = XM_KEYDOWN;
				key_msg.wp = VK_AP_MENU;
				key_msg.lp = 0;
                            
				AP_SwitchButtonControlMessageHandler (settingViewData->hMoniorSwitchControl, &key_msg);
                #endif
				
				XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				settingViewData->menuOption.dwWindowIconId = 0xFFFFFFFF;
				settingViewData->menuOption.dwWindowTextId = AP_ID_VIDEOSETTING_PARKMONITOR_TITLE;
				settingViewData->menuOption.wMenuOptionCount = sizeof(dwParkMonitorMenuOption)/sizeof(DWORD);
				settingViewData->menuOption.wMenuOptionSelect = AppMenuData.parkmonitor;
				settingViewData->menuOption.lpdwMenuOptionId = (DWORD*)&dwParkMonitorMenuOption;
				settingViewData->menuOption.fpMenuOptionCB = ParkMonitorMenuOptionCB;
				settingViewData->menuOption.lpUserData = NULL;
				// �����˵�ѡ���б���
				AP_OpenMenuOptionView (&settingViewData->menuOption);
			}
#endif

#ifdef _XM_EXPOSEURECOMPENSATION_ENABLE
            else if(SelectedItem == VIDEO_MENU_EXPOSEURECOMPENSATION)
			{
			      XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				// �عⲹ��
				settingViewData->menuOption.dwWindowIconId = 0xFFFFFFFF;
				settingViewData->menuOption.dwWindowTextId = AP_ID_VIDEOSETTING_EXPOSURECOMPENSATION_TITLE;
				settingViewData->menuOption.wMenuOptionCount = sizeof(dwExposure_compensationMenuOption)/sizeof(dwExposure_compensationMenuOption[0]);
				settingViewData->menuOption.wMenuOptionSelect = AppMenuData.exposure_compensation;
				settingViewData->menuOption.lpdwMenuOptionId = (DWORD*)&dwExposure_compensationMenuOption;
				settingViewData->menuOption.fpMenuOptionCB = Exposure_compensationMenuOptionCB;
				settingViewData->menuOption.lpUserData = NULL;
				//settingViewData->nCurItem = -1;
				// �����˵�ѡ���б���
				AP_OpenMenuOptionView (&settingViewData->menuOption);
			}
#endif
		
#ifdef _XM_LIGHTFREQ_ENABLE_	
			else if(SelectedItem == VIDEO_MENU_LIGHTFREQ)
			{
			    XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
				// ��ԴƵ��
				settingViewData->menuOption.dwWindowIconId = 0xFFFFFFFF;
				settingViewData->menuOption.dwWindowTextId = AP_ID_VIDEOSETTING_LIGHTFREQ_TITLE;
				settingViewData->menuOption.wMenuOptionCount = sizeof(dwLightFreqMenuOption)/sizeof(dwLightFreqMenuOption[0]);
				settingViewData->menuOption.wMenuOptionSelect = AppMenuData.light_freq;
				settingViewData->menuOption.lpdwMenuOptionId = (DWORD*)&dwLightFreqMenuOption;
				settingViewData->menuOption.fpMenuOptionCB = LightFreqMenuOptionCB;
				settingViewData->menuOption.lpUserData = NULL;
				//settingViewData->nCurItem = -1;
				// �����˵�ѡ���б���
				AP_OpenMenuOptionView (&settingViewData->menuOption);
			}	
			break;
#endif			

		case VIDEOSETTINGVIEW_COMMAND_RETURN:
			// ���ص�����
			XM_PullWindow (0);
			break;

		case VIDEOSETTINGVIEW_COMMAND_SYSTEM:
			XM_SetViewSwitchAnimatingDirection (XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT);
			// �л�����ϵͳ������
			XM_JumpWindowEx (XMHWND_HANDLE(SystemSettingView), 0, XM_JUMP_POPDEFAULT);
			// �л�����ˮӡ���á�����
			//XM_JumpWindowEx (XMHWND_HANDLE(WaterMarkSettingView), 0, 0);
			break;

	}
}

static VOID VideoSettingViewOnTimer (XMMSG *msg)
{
	VIDEOSETTINGVIEWDATA *settingViewData;
	
	settingViewData = (VIDEOSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoSettingView));
	if(settingViewData == NULL)
		return;

	// ���ص�����
	XM_PullWindow(XMHWND_HANDLE(Desktop));
}


VOID VideoSettingViewOnTouchDown (XMMSG *msg)
{
	int index;
	VIDEOSETTINGVIEWDATA *settingViewData = (VIDEOSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoSettingView));
	if(settingViewData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg))
		return;

	index = AppLocateItem (XMHWND_HANDLE(VideoSettingView), settingViewData->nItemCount, APP_POS_ITEM5_LINEHEIGHT, settingViewData->nTopItem, LOWORD(msg->lp), HIWORD(msg->lp));
	if(index < 0)
		return;
	//XM_printf("video index is %d\n",index);
	settingViewData->nTouchItem = index;
	//if(settingViewData->nCurItem != index)
	{
		settingViewData->nCurItem = index;
		XM_InvalidateWindow ();
		XM_UpdateWindow ();
		XM_SetTimer (XMTIMER_VIDEOSETTING, OSD_AUTO_HIDE_TIMEOUT);
	}
}

VOID VideoSettingViewOnTouchUp (XMMSG *msg)
{
	VIDEOSETTINGVIEWDATA *settingViewData;
	settingViewData = (VIDEOSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoSettingView));
	if(settingViewData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg))
		return;
	if(settingViewData->nTouchItem == -1)
		return;
	settingViewData->nTouchItem = -1;
	
	XM_PostMessage (XM_COMMAND,VIDEOSETTINGVIEW_COMMAND_MODIFY , settingViewData->nCurItem);// settingViewData->btnControl.pBtnInfo[0].wCommand
      
}

VOID VideoSettingViewOnTouchMove (XMMSG *msg)
{
	VIDEOSETTINGVIEWDATA *settingViewData;
	settingViewData = (VIDEOSETTINGVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(VideoSettingView));
	if(settingViewData == NULL)
		return;

	if(AP_ButtonControlMessageHandler (&settingViewData->btnControl, msg))
		return;

}

// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (VideoSettingView)
	XM_ON_MESSAGE (XM_PAINT, VideoSettingViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, VideoSettingViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, VideoSettingViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, VideoSettingViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, VideoSettingViewOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, VideoSettingViewOnCommand)
	XM_ON_MESSAGE (XM_TIMER, VideoSettingViewOnTimer)
	XM_ON_MESSAGE (XM_TOUCHDOWN, VideoSettingViewOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, VideoSettingViewOnTouchUp)
	XM_ON_MESSAGE (XM_TOUCHMOVE, VideoSettingViewOnTouchMove)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, VideoSettingView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)
