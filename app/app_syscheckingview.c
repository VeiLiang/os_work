//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_syscheckingview.c
//	  �ϵ��Լ�
//
//	Revision history
//
//		2012.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menudata.h"
#include "app_menuid.h"
#include "system_check.h"
#include "app_voice.h"

// ϵͳ�Լ�׶ζ���
enum {
	POWERONCHECK_STEP_IDLE = 0,			// ����״̬
	POWERONCHECK_STEP_TIMESETTING,		// ʱ�����ü�飬���ʱ���Ƿ�������
	POWERONCHECK_STEP_CARD,					// ��Ӳ�����(���Ƿ���ڡ���д����)
	POWERONCHECK_STEP_FILESYSTEM,			// �ļ�ϵͳ���(�����ɼ�¼ʱ����)
	POWERONCHECK_STEP_BACKUPBATTERY,		// ���ݵ��(CR2312)���
	POWERONCHECK_LAST_STEP
};

// �Լ촰��˽����Ϣ����
#define	XM_USER_SYSCHECKING		(XM_USER+0)

// ִ���Լ���̡�
// ����ֵ	1 ��ʾ�Լ�׶���ȫ����ɡ�
//				  ����ʾÿ���׶μ��PASS���������û����������顣���û�����ϵͳʱ������
//								
//				0 ��ʾ�Լ�׶���δ���
//
static int systemChecking (void)
{

	int checkingResult = APP_SYSTEMCHECK_SUCCESS;

	// ��ȡ��ǰ�Լ�Ľ׶�
	DWORD step = (DWORD)XM_GetWindowPrivateData (XMHWND_HANDLE(SysCheckingView));
	while(step < POWERONCHECK_LAST_STEP)
	{
		if(step == POWERONCHECK_STEP_CARD)
		{
			// ���Լ�

			// ����Ƿ��޿�����ģʽ
			if(AppGetCardOperationMode() == APP_DEMO_OPERATION_MODE)
			{
				// �ѽ��롰�޿�����ģʽ��
				step ++;
				continue;
			}

			checkingResult = APSYS_CardChecking();
			if(checkingResult == APP_SYSTEMCHECK_SUCCESS)
			{
				// ���Լ�PASS
			} 
			else if(checkingResult == APP_SYSTEMCHECK_CARD_NOCARD)
			{
				// ��⵽����δ���롱
				// ���ϵͳ�����С��޿���ʱ��������ʾģʽ���Ƿ�ʹ��
				if(AppMenuData.demo_mode == AP_SETTING_DEMOMODE_ON)
				{
					// ��ʹ�ܡ��޿���ʾ��
					// �Զ����á��޿�����ģʽ��
					AppSetCardOperationMode (APP_DEMO_OPERATION_MODE);
					step ++;
					continue;
				}
				else
				{
					// ���Լ��쳣����ֹ�����̡�ˢ����ʾ���Է�ӳ��ǰ��״̬���ȴ��û����룬������һ������
					break;
				}
			}
			else
			{
				// ���Լ��쳣����ֹ�����̡�ˢ����ʾ���Է�ӳ��ǰ��״̬���ȴ��û����룬������һ������
				break;
			}
		}
		else if(step == POWERONCHECK_STEP_FILESYSTEM)
		{
			// ����Ƿ��޿�����ģʽ
			if(AppGetCardOperationMode() == APP_DEMO_OPERATION_MODE)
			{
				// �ѽ��롰�޿�����ģʽ��
				step ++;
				continue;
			}

			// �ļ�ϵͳ�Լ� (����Ŀ¼��顢�ļ�д���鼰��д�Ƚϼ�顢ʣ���¼�ռ���)
			checkingResult = APSYS_FileSystemChecking();
			if(checkingResult == APP_SYSTEMCHECK_SUCCESS)
			{
				// �ļ�ϵͳ�Լ�PASS
			}
			else
			{
				// �ļ�ϵͳ����쳣
				break;
			}
		}
		else if(step == POWERONCHECK_STEP_BACKUPBATTERY)
		{
			// ���ݵ��(CR2312)���
			checkingResult = APSYS_BackupBatteryChecking();
			if(checkingResult == APP_SYSTEMCHECK_SUCCESS)
			{
				// ���ݵ���Լ�PASS
			}
			else
			{
				// ���ݵ�ؼ���쳣
				break;
			}
		}
		else if(step == POWERONCHECK_STEP_TIMESETTING)
		{
			// ϵͳʱ���Ƿ������ü��
			checkingResult = APSYS_TimeSettingChecking();
			if(checkingResult == APP_SYSTEMCHECK_SUCCESS)
			{
				// ϵͳʱ���Լ�PASS
			}
			else
			{
				// ���ϵͳ�����С��޿���ʱ��������ʾģʽ���Ƿ�ʹ��
				if(AppMenuData.demo_mode == AP_SETTING_DEMOMODE_ON)
				{
					// ��ʾģʽ����ϵͳʱ�䲻����
				}
				// ʱ��ˮӡ�ر�ʱ������ʱ�����ϵͳʱ���Ƿ������á�
				else if(AppMenuData.time_stamp)
				{
					// ʱ��ˮӡ����
					// ϵͳʱ�����쳣
					break;
				}
				// ����ʱ�����á��ں��Զ���ϵͳ��λΪȱʡϵͳʱ��
				checkingResult = APP_SYSTEMCHECK_SUCCESS;
			}
		}
		
		step ++;
	}
	// ���´��ڵ�ǰϵͳ���׶�ֵ
	XM_SetWindowPrivateData (XMHWND_HANDLE(SysCheckingView), (VOID *)step);
	return checkingResult;
}

VOID SysCheckingViewOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
		// ��ʼϵͳ�ϵ��Լ�
		DWORD step = POWERONCHECK_STEP_IDLE;
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(SysCheckingView), (void *)step);
		
		// ���Լ촰��Ͷ���Լ���Ϣ
		XM_PostMessage (XM_USER_SYSCHECKING, 0, 0);
	}
	else
	{
		// �������Ӵ��ڷ���

		// ���Լ촰��Ͷ���Լ���Ϣ, �����Լ����
		XM_PostMessage (XM_USER_SYSCHECKING, 0, 0);

		XM_printf ("SysCheckingView Pull\n");
	}
}

VOID SysCheckingViewOnLeave (XMMSG *msg)
{
	if (msg->wp == 0)
	{
		XM_printf ("SysCheckingView Exit\n");
	}
	else
	{
		XM_printf ("SysCheckingView Push\n");
	}
}



VOID SysCheckingViewOnPaint (XMMSG *msg)
{
	XMRECT rc;
	APPROMRES *AppRes;
	
//	DWORD step = (DWORD)XM_GetWindowPrivateData (XMHWND_HANDLE(SysCheckingView));

	XM_GetDesktopRect (&rc);
	XM_FillRect (XMHWND_HANDLE(SysCheckingView), rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_WINDOW));

	// �����Լ�׶���ʾ��ͬ����Ϣ
	// if(step == POWERONCHECK_STEP_IDLE)
	{
		// ��ʾ��ϵͳ�Լ��С���Ϣ
		AppRes = AP_AppRes2RomRes (AP_ID_SYSCHECKING_INFO_SYSCHECKING);
		XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, 
			XMHWND_HANDLE(SysCheckingView), &rc, XMGIF_DRAW_POS_CENTER);
	}
}


VOID SysCheckingViewOnKeyDown (XMMSG *msg)
{
	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
			break;

		case VK_AP_MODE:		// �л���¼��ط�ģʽ
			break;

		case VK_AP_SWITCH:	// �����л���
			break;

		case VK_AP_UP:		// ����¼���
			break;
	}
}

VOID SysCheckingViewOnTimer (XMMSG *msg)
{
}

VOID SysCheckingViewOnSysChecking (XMMSG *msg)
{
	int checkingResult;
	unsigned int voice_id;
	DWORD dwForecastTime;

	// ��ʼ��һ�׶ε��Լ����
re_check:
	checkingResult = systemChecking();
	if(checkingResult == APP_SYSTEMCHECK_SUCCESS)
	{
		// ϵͳ�Լ�ȫ��ͨ��
		// ��顰������ʾ�������Ƿ�����
		if(AppMenuData.voice_prompts)
		{
			// ��������������̨�����Լ�����

			// ���֡�������ģʽ�������޿�/ֻ����ʾģʽ��
			if(AppGetCardOperationMode() == APP_CARD_OPERATION_MODE)
			{
				// ��������ģʽ��

				// ��ʾ¼��״̬
				if(AppMenuData.mic)
					voice_id = XM_VOICE_ID_MIC_ON;
				else
					voice_id = XM_VOICE_ID_MIC_OFF;
                if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				    XM_voice_prompts_insert_voice (voice_id);

				// ��ʾˮӡ
				if(AppMenuData.time_stamp)
					voice_id = XM_VOICE_ID_TIMESTAMP_ON;
				else
					voice_id = XM_VOICE_ID_TIMESTAMP_OFF;
                if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				    XM_voice_prompts_insert_voice (voice_id);

				// ��ʾѭ��¼��ʱ��
				voice_id = XM_VOICE_ID_FORCASTRECORDTIME_TITLE;
                if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				    XM_voice_prompts_insert_voice (voice_id);
				dwForecastTime = AppGetForecastUsageTime();
				if(dwForecastTime < 20)
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_WORST;
				else if(dwForecastTime >= (12 * 60))
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_12H;
				else if(dwForecastTime >= (11 * 60))
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_11H;
				else if(dwForecastTime >= (10 * 60))
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_10H;
				else if(dwForecastTime >= (9 * 60))
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_9H;
				else if(dwForecastTime >= (8 * 60))
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_8H;
				else if(dwForecastTime >= (7 * 60))
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_7H;
				else if(dwForecastTime >= (6 * 60))
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_6H;
				else if(dwForecastTime >= (5 * 60))
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_5H;
				else if(dwForecastTime >= (4 * 60))
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_4H;
				else if(dwForecastTime >= (3 * 60))
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_3H;
				else if(dwForecastTime >= (2 * 60))
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_2H;
				else if(dwForecastTime >= (1 * 60))
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_1H;
				else if(dwForecastTime >= 50)
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_50M;
				else if(dwForecastTime >= 40)
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_40M;
				else if(dwForecastTime >= 30)
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_30M;
				else //if(dwForecastTime >= 20)
					voice_id = XM_VOICE_ID_FORCASTRECORDTIME_20M;
                if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				    XM_voice_prompts_insert_voice (voice_id);
			}
			else
			{
				// ���޿�/ֻ����ʾģʽ��
				voice_id = XM_VOICE_ID_NOCARD_DEMO_OPERATIONMODE;
                if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				    XM_voice_prompts_insert_voice (voice_id);

				// ������ʾ��MIC¼��״����
				if(AppMenuData.mic)
					voice_id = XM_VOICE_ID_MIC_ON;
				else
					voice_id = XM_VOICE_ID_MIC_OFF;
                if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				    XM_voice_prompts_insert_voice (voice_id);

				// ��ʾˮӡ
				if(AppMenuData.time_stamp)
					voice_id = XM_VOICE_ID_TIMESTAMP_ON;
				else
					voice_id = XM_VOICE_ID_TIMESTAMP_OFF;
                if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				    XM_voice_prompts_insert_voice (voice_id);
			}
		}

		// ���ص����棬��ʼ�г���¼
		XM_PullWindow (0);
	}
	else
	{
		// �Լ�δ���
		// �����Լ�׶Σ�������Ӧ���Ӵ�����ɼ����������
		if(checkingResult == APP_SYSTEMCHECK_CARD_NOCARD)
		{
			// ������ʾ����δ���롱
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
			    XM_voice_prompts_insert_voice (XM_VOICE_ID_CARD_NOCARD);
			XM_PushWindowEx (XMHWND_HANDLE(CardView), APP_CARDVIEW_CUSTOM_NOCARD);
		}
		else if(checkingResult == APP_SYSTEMCHECK_CARD_WRITEPROTECT)
		{
			// ������ʾ����д������
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
			    XM_voice_prompts_insert_voice (XM_VOICE_ID_CARD_WRITEPROTECT);
			XM_PushWindowEx (XMHWND_HANDLE(CardView), APP_CARDVIEW_CUSTOM_WRITEPROTECT);
		}
		else if(checkingResult == APP_SYSTEMCHECK_FILESYSTEM_FSERROR)
		{
			// ������ʾ�����ݿ��ļ�ϵͳ�쳣������Ҫ��ʽ����
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS)) {
    			XM_voice_prompts_insert_voice (XM_VOICE_ID_CARD_FILESYSTEMERROR);
    			XM_voice_prompts_insert_voice (XM_VOICE_ID_CARD_FORMAT);
            }

			XM_PushWindowEx (XMHWND_HANDLE(CardView), APP_CARDVIEW_CUSTOM_FSERROR);
		}
		else if(checkingResult == APP_SYSTEMCHECK_FILESYSTEM_LOWSPACE)
		{
			// ������ʾ
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS)) {
    			XM_voice_prompts_insert_voice (XM_VOICE_ID_FORCASTRECORDTIME_TITLE);
    			XM_voice_prompts_insert_voice (XM_VOICE_ID_FORCASTRECORDTIME_WORST);
    			XM_voice_prompts_insert_voice (XM_VOICE_ID_CARD_FORMAT);
            }
			XM_PushWindowEx (XMHWND_HANDLE(CardView), APP_CARDVIEW_CUSTOM_LOWSPACE);
		}
		else if(checkingResult == APP_SYSTEMCHECK_BACKUPBATTERY_LOWVOLTAGE)
		{
			DWORD step;
			// ���ݵ�ض���
			
			// ������ʾ�����ݵ�ص����ľ���
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
			    XM_voice_prompts_insert_voice (XM_VOICE_ID_BACKUPBATTERY);

			// ���ݵ�ؽ���ʾ����������ʾ����
			step = (DWORD)XM_GetWindowPrivateData (XMHWND_HANDLE(SysCheckingView));
			XM_SetWindowPrivateData (XMHWND_HANDLE(SysCheckingView), (VOID *)step);

			// ����ϵͳ�Լ�
			goto re_check;
		}
		else if(checkingResult == APP_SYSTEMCHECK_TIMESETTING_INVALID)
		{
			// ϵͳʱ��δ����
			// ������ʾ ��������ϵͳʱ�䡱
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
			    XM_voice_prompts_insert_voice (XM_VOICE_ID_TIMESETTING);
			XM_PushWindowEx (XMHWND_HANDLE(DateTimeSetting), APP_DATETIMESETTING_CUSTOM_FORCED);
		}
	}
}




// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (SysCheckingView)
	XM_ON_MESSAGE (XM_PAINT, SysCheckingViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, SysCheckingViewOnKeyDown)
	XM_ON_MESSAGE (XM_ENTER, SysCheckingViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, SysCheckingViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, SysCheckingViewOnTimer)
	XM_ON_MESSAGE (XM_USER_SYSCHECKING, SysCheckingViewOnSysChecking)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, SysCheckingView, 0, 0, 0)


