//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_syscheckingview.c
//	  上电自检
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

// 系统自检阶段定义
enum {
	POWERONCHECK_STEP_IDLE = 0,			// 空闲状态
	POWERONCHECK_STEP_TIMESETTING,		// 时间设置检查，检查时间是否已设置
	POWERONCHECK_STEP_CARD,					// 卡硬件检测(卡是否存在、卡写保护)
	POWERONCHECK_STEP_FILESYSTEM,			// 文件系统检查(包括可记录时间检查)
	POWERONCHECK_STEP_BACKUPBATTERY,		// 备份电池(CR2312)检查
	POWERONCHECK_LAST_STEP
};

// 自检窗口私有消息定义
#define	XM_USER_SYSCHECKING		(XM_USER+0)

// 执行自检过程。
// 返回值	1 表示自检阶段已全部完成。
//				  不表示每个阶段检查PASS，可能是用户放弃该项检查。如用户放弃系统时间设置
//								
//				0 表示自检阶段尚未完成
//
static int systemChecking (void)
{

	int checkingResult = APP_SYSTEMCHECK_SUCCESS;

	// 获取当前自检的阶段
	DWORD step = (DWORD)XM_GetWindowPrivateData (XMHWND_HANDLE(SysCheckingView));
	while(step < POWERONCHECK_LAST_STEP)
	{
		if(step == POWERONCHECK_STEP_CARD)
		{
			// 卡自检

			// 检查是否无卡操作模式
			if(AppGetCardOperationMode() == APP_DEMO_OPERATION_MODE)
			{
				// 已进入“无卡操作模式”
				step ++;
				continue;
			}

			checkingResult = APSYS_CardChecking();
			if(checkingResult == APP_SYSTEMCHECK_SUCCESS)
			{
				// 卡自检PASS
			} 
			else if(checkingResult == APP_SYSTEMCHECK_CARD_NOCARD)
			{
				// 检测到“卡未插入”
				// 检查系统设置中“无卡无时间设置演示模式”是否使能
				if(AppMenuData.demo_mode == AP_SETTING_DEMOMODE_ON)
				{
					// 已使能“无卡演示”
					// 自动设置“无卡操作模式”
					AppSetCardOperationMode (APP_DEMO_OPERATION_MODE);
					step ++;
					continue;
				}
				else
				{
					// 卡自检异常，终止检查过程。刷新显示，以反映当前的状态。等待用户输入，决定下一步处理。
					break;
				}
			}
			else
			{
				// 卡自检异常，终止检查过程。刷新显示，以反映当前的状态。等待用户输入，决定下一步处理。
				break;
			}
		}
		else if(step == POWERONCHECK_STEP_FILESYSTEM)
		{
			// 检查是否无卡操作模式
			if(AppGetCardOperationMode() == APP_DEMO_OPERATION_MODE)
			{
				// 已进入“无卡操作模式”
				step ++;
				continue;
			}

			// 文件系统自检 (包括目录检查、文件写入检查及读写比较检查、剩余可录空间检查)
			checkingResult = APSYS_FileSystemChecking();
			if(checkingResult == APP_SYSTEMCHECK_SUCCESS)
			{
				// 文件系统自检PASS
			}
			else
			{
				// 文件系统检查异常
				break;
			}
		}
		else if(step == POWERONCHECK_STEP_BACKUPBATTERY)
		{
			// 备份电池(CR2312)检查
			checkingResult = APSYS_BackupBatteryChecking();
			if(checkingResult == APP_SYSTEMCHECK_SUCCESS)
			{
				// 备份电池自检PASS
			}
			else
			{
				// 备份电池检查异常
				break;
			}
		}
		else if(step == POWERONCHECK_STEP_TIMESETTING)
		{
			// 系统时间是否已设置检查
			checkingResult = APSYS_TimeSettingChecking();
			if(checkingResult == APP_SYSTEMCHECK_SUCCESS)
			{
				// 系统时间自检PASS
			}
			else
			{
				// 检查系统设置中“无卡无时间设置演示模式”是否使能
				if(AppMenuData.demo_mode == AP_SETTING_DEMOMODE_ON)
				{
					// 演示模式允许系统时间不设置
				}
				// 时间水印关闭时，开机时不检查系统时间是否已设置。
				else if(AppMenuData.time_stamp)
				{
					// 时间水印开启
					// 系统时间检查异常
					break;
				}
				// 忽略时间设置。内核自动将系统复位为缺省系统时间
				checkingResult = APP_SYSTEMCHECK_SUCCESS;
			}
		}
		
		step ++;
	}
	// 更新窗口当前系统检查阶段值
	XM_SetWindowPrivateData (XMHWND_HANDLE(SysCheckingView), (VOID *)step);
	return checkingResult;
}

VOID SysCheckingViewOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
		// 开始系统上电自检
		DWORD step = POWERONCHECK_STEP_IDLE;
		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(SysCheckingView), (void *)step);
		
		// 给自检窗口投递自检消息
		XM_PostMessage (XM_USER_SYSCHECKING, 0, 0);
	}
	else
	{
		// 从其他子窗口返回

		// 给自检窗口投递自检消息, 继续自检过程
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

	// 根据自检阶段显示不同的信息
	// if(step == POWERONCHECK_STEP_IDLE)
	{
		// 显示“系统自检中”信息
		AppRes = AP_AppRes2RomRes (AP_ID_SYSCHECKING_INFO_SYSCHECKING);
		XM_DrawImageEx (XM_RomAddress(AppRes->rom_offset), AppRes->res_length, 
			XMHWND_HANDLE(SysCheckingView), &rc, XMGIF_DRAW_POS_CENTER);
	}
}


VOID SysCheckingViewOnKeyDown (XMMSG *msg)
{
	switch(msg->wp)
	{
		case VK_AP_MENU:		// 菜单键
			break;

		case VK_AP_MODE:		// 切换到录像回放模式
			break;

		case VK_AP_SWITCH:	// 画面切换键
			break;

		case VK_AP_UP:		// 紧急录像键
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

	// 开始下一阶段的自检过程
re_check:
	checkingResult = systemChecking();
	if(checkingResult == APP_SYSTEMCHECK_SUCCESS)
	{
		// 系统自检全部通过
		// 检查“语音提示”功能是否开启。
		if(AppMenuData.voice_prompts)
		{
			// 若开启，语音后台播放自检结果。

			// 区分“卡操作模式”及“无卡/只读演示模式”
			if(AppGetCardOperationMode() == APP_CARD_OPERATION_MODE)
			{
				// “卡操作模式”

				// 提示录音状态
				if(AppMenuData.mic)
					voice_id = XM_VOICE_ID_MIC_ON;
				else
					voice_id = XM_VOICE_ID_MIC_OFF;
                if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				    XM_voice_prompts_insert_voice (voice_id);

				// 提示水印
				if(AppMenuData.time_stamp)
					voice_id = XM_VOICE_ID_TIMESTAMP_ON;
				else
					voice_id = XM_VOICE_ID_TIMESTAMP_OFF;
                if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				    XM_voice_prompts_insert_voice (voice_id);

				// 提示循环录制时间
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
				// “无卡/只读演示模式”
				voice_id = XM_VOICE_ID_NOCARD_DEMO_OPERATIONMODE;
                if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				    XM_voice_prompts_insert_voice (voice_id);

				// 语音提示“MIC录音状况”
				if(AppMenuData.mic)
					voice_id = XM_VOICE_ID_MIC_ON;
				else
					voice_id = XM_VOICE_ID_MIC_OFF;
                if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				    XM_voice_prompts_insert_voice (voice_id);

				// 提示水印
				if(AppMenuData.time_stamp)
					voice_id = XM_VOICE_ID_TIMESTAMP_ON;
				else
					voice_id = XM_VOICE_ID_TIMESTAMP_OFF;
                if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				    XM_voice_prompts_insert_voice (voice_id);
			}
		}

		// 返回到桌面，开始行车记录
		XM_PullWindow (0);
	}
	else
	{
		// 自检未完成
		// 根据自检阶段，进入相应的子窗口完成检查设置流程
		if(checkingResult == APP_SYSTEMCHECK_CARD_NOCARD)
		{
			// 语音提示“卡未插入”
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
			    XM_voice_prompts_insert_voice (XM_VOICE_ID_CARD_NOCARD);
			XM_PushWindowEx (XMHWND_HANDLE(CardView), APP_CARDVIEW_CUSTOM_NOCARD);
		}
		else if(checkingResult == APP_SYSTEMCHECK_CARD_WRITEPROTECT)
		{
			// 语音提示“卡写保护”
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
			    XM_voice_prompts_insert_voice (XM_VOICE_ID_CARD_WRITEPROTECT);
			XM_PushWindowEx (XMHWND_HANDLE(CardView), APP_CARDVIEW_CUSTOM_WRITEPROTECT);
		}
		else if(checkingResult == APP_SYSTEMCHECK_FILESYSTEM_FSERROR)
		{
			// 语音提示“数据卡文件系统异常”“需要格式化”
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS)) {
    			XM_voice_prompts_insert_voice (XM_VOICE_ID_CARD_FILESYSTEMERROR);
    			XM_voice_prompts_insert_voice (XM_VOICE_ID_CARD_FORMAT);
            }

			XM_PushWindowEx (XMHWND_HANDLE(CardView), APP_CARDVIEW_CUSTOM_FSERROR);
		}
		else if(checkingResult == APP_SYSTEMCHECK_FILESYSTEM_LOWSPACE)
		{
			// 语音提示
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
			// 备份电池恶劣
			
			// 语音提示“备份电池电力耗尽”
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
			    XM_voice_prompts_insert_voice (XM_VOICE_ID_BACKUPBATTERY);

			// 备份电池仅提示语音，不显示界面
			step = (DWORD)XM_GetWindowPrivateData (XMHWND_HANDLE(SysCheckingView));
			XM_SetWindowPrivateData (XMHWND_HANDLE(SysCheckingView), (VOID *)step);

			// 继续系统自检
			goto re_check;
		}
		else if(checkingResult == APP_SYSTEMCHECK_TIMESETTING_INVALID)
		{
			// 系统时间未设置
			// 语音提示 “请设置系统时间”
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
			    XM_voice_prompts_insert_voice (XM_VOICE_ID_TIMESETTING);
			XM_PushWindowEx (XMHWND_HANDLE(DateTimeSetting), APP_DATETIMESETTING_CUSTOM_FORCED);
		}
	}
}




// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (SysCheckingView)
	XM_ON_MESSAGE (XM_PAINT, SysCheckingViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, SysCheckingViewOnKeyDown)
	XM_ON_MESSAGE (XM_ENTER, SysCheckingViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, SysCheckingViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, SysCheckingViewOnTimer)
	XM_ON_MESSAGE (XM_USER_SYSCHECKING, SysCheckingViewOnSysChecking)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, SysCheckingView, 0, 0, 0)


