//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_systemupdate.c
//	  系统升级
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"
#include "app_processview.h"
#include "systemapi.h"

#ifdef _WINDOWS
static const char update_file[] = "update.bin";				// 当前运行目录
#else
static const char update_file[] = "mmc:0:\\update.bin";	// SD卡0根目录下
#endif

static AP_PROCESSINFO SystemUpdateProcessInfo;

// 系统升级过程视窗的系统事件处理回调函数
static void system_update_view_system_event_handler (XMMSG *msg, int process_state)
{
	switch(msg->wp)
	{
		case SYSTEM_EVENT_CARD_UNPLUG:					// SD卡拔出事件
		case SYSTEM_EVENT_CARD_INVALID:					// 卡无效
		case SYSTEM_EVENT_CARD_INSERT:					// 卡插入
		case SYSTEM_EVENT_CARD_FS_ERROR:				// 卡文件系统错误
			XM_printf(">>>>>>>>>>system_update_view_system_event_handler, 1...\r\n");
			if(	process_state == PROCESS_STATE_INITIAL||process_state == PROCESS_STATE_CONFIRM)
			{
				// 第一次确认及第二次确认过程中, 以上卡事件导致"系统升级过程"直接退出
				XM_BreakSystemEventDefaultProcess (msg);		// 终止该系统事件的继续处理
				XM_PullWindow (0);		// 弹出"系统升级"视窗

				// 将系统事件重新压入, 等待系统处理
				XM_lock ();
				XM_KeyEventProc ((unsigned short)0xF0, (unsigned short)(msg->wp));
				XM_unlock ();
			}
			else if(process_state == PROCESS_STATE_WORKING || process_state == PROCESS_STATE_SUCCESS)
			{
				XM_BreakSystemEventDefaultProcess (msg);		// 终止该系统事件的继续处理, 即过滤该事件
			}
			break;

		case SYSTEM_EVENT_CARD_VERIFY_ERROR:
		case SYSTEM_EVENT_CARD_DISK_FULL:	
		case SYSTEM_EVENT_VIDEOITEM_RECYCLE_CONSUMED:
		case SYSTEM_EVENT_VIDEOITEM_LOW_SPACE:
			// 提示，继续等待用户操作
			XM_BreakSystemEventDefaultProcess (msg);
			break;

		case SYSTEM_EVENT_MAIN_BATTERY:	// 主电池电压变化事件
		{
			XM_BreakSystemEventDefaultProcess (msg);
			// 读取主电池的当前状态
			if(	process_state == PROCESS_STATE_INITIAL
				||	process_state == PROCESS_STATE_CONFIRM)
			{
				if(XM_GetFmlDeviceCap (DEVCAP_MAINBATTERYVOLTAGE) == DEVCAP_BATTERYVOLTAGE_WORST)
				{
					XM_PullWindow (0);

					XM_lock ();
					XM_KeyEventProc ((unsigned short)VK_AP_SYSTEM_EVENT, (unsigned short)(msg->wp));
					XM_unlock ();
				}
			}
			break;
		}
		
		case SYSTEM_EVENT_SYSTEM_UPDATE_FILE_CHECKED:
		{
			// 重复收到的系统升级消息, 简单丢弃
			XM_BreakSystemEventDefaultProcess (msg);		// 终止该系统事件的继续处理
			break;
		}

		default:
			break;
	}
}

// 按钮命令的回调函数
static void menu_cb (UINT menu)
{
	// 升级成功后强制重启
	XM_ShutDownSystem(SDS_REBOOT);
}


// 系统升级结束回调函数
static int end_update_cb (void * state)
{
	if((int)state == 0)
	{
		// 升级成功
		#if 0
		DWORD dwNormalButton[1] = {AP_ID_CARD_INFO_SYSUPDATE_REBOOT};

		XM_OpenOkCancelView (AP_ID_CARD_INFO_SYSUPDATE_INFO_FINISH,	// 信息文本资源ID
									0,												// 图片信息资源ID, 非0值，指定显示图片信息的资源ID
									1,												// 按钮个数，1或者2
									dwNormalButton,							//	按钮资源ID的数组
									NULL,											//	按钮按下资源ID, 可以为空.
									0x80404040,									// 背景色, 0x80404040(ARGB32) 表示半透明(Alpga=0x80)的浅灰色(RGB=0x404040)
									5.0f,		// 自动关闭时间
									1.0f,			// Alpha透明度, 1.0 全覆盖    0.0 全透明
													//		Alpha透明度 * 	背景色的Alpha值 表示 最终的Alpha因子
									menu_cb,		// 按钮命令的回调函数，参数为 XM_COMMAND_OK 或者 XM_COMMAND_CANCEL
									XM_VIEW_ALIGN_CENTRE,
									XM_OKCANCEL_OPTION_ENABLE_POPTOPVIEW|XM_OKCANCEL_OPTION_SYSTEM_MODEL
									);
		#endif
		//升级成功后强制重启
		XM_ShutDownSystem(SDS_REBOOT);
	}
	else
	{
		//升级失败
		//AP_OpenMessageViewEx(AP_ID_SYSTEMSETTING_UPDATE_TITLE, AP_ID_CARD_INFO_SYSUPDATE_FAILURE, (DWORD)-1, 1);
	}
	return 0;
}

void app_start_update (int push_view)
{
	DWORD UpdateFileStatus;
	// 检查系统升级文件是否存在

	// 检查系统升级文件是否为合法的升级文件(文件标识及CRC检查)
	
	// 检查系统升级文件是否是当前机器使用 (检查厂家ID)

	// 检查系统版本
	//UpdateFileStatus = XM_GetUpdateFileStatus();
	
	//if (UpdateFileStatus == 0)
	//{
	//	AP_OpenMessageView (AP_ID_SYSTEMSETTING_UPDATE_FILE_ABNORMAL);
	//}
	//else
	{
		memset (&SystemUpdateProcessInfo, 0, sizeof(SystemUpdateProcessInfo));
		SystemUpdateProcessInfo.type = AP_PROCESS_VIEW_SYSTEMUPDATE;
		SystemUpdateProcessInfo.Title = AP_ID_SYSTEMSETTING_UPDATE_TITLE;
		SystemUpdateProcessInfo.DispItem[0] = AP_ID_CARD_INFO_SYSUPDATE_INITIAL;
		SystemUpdateProcessInfo.DispItem[1] = AP_ID_CARD_INFO_SYSUPDATE_CONFIRM;
		SystemUpdateProcessInfo.DispItem[2] = AP_ID_CARD_INFO_SYSUPDATE_WORKING;
		SystemUpdateProcessInfo.DispItem[3] = AP_ID_CARD_INFO_SYSUPDATE_SUCCESS;
		SystemUpdateProcessInfo.DispItem[4] = AP_ID_CARD_INFO_SYSUPDATE_FAILURE;
		SystemUpdateProcessInfo.nDispItemNum = 5;
		SystemUpdateProcessInfo.nMaxProgress = 10;
		SystemUpdateProcessInfo.fpStartProcess = StartUpdateProgress;
		SystemUpdateProcessInfo.fpEndProcess = end_update_cb;
		SystemUpdateProcessInfo.fpQueryProgress = QueryUpdateProgress;
		SystemUpdateProcessInfo.fpSystemEventCb = system_update_view_system_event_handler;
		
		// 压入或将栈顶视窗(文件校验提示视窗)弹栈后压入
		if(push_view)
			XM_PushWindowEx (XMHWND_HANDLE(ProcessView), (DWORD)(&SystemUpdateProcessInfo));
		else
			XM_JumpWindowEx (XMHWND_HANDLE(ProcessView), (DWORD)(&SystemUpdateProcessInfo), XM_JUMP_POPTOPVIEW);
	}
}

// 文件系统读写校验错误回调函数
static void fs_verify_error_alert_cb (void *lpUserData, unsigned int uKeyPressed)
{
	app_start_update (0);
}

// 打开系统升级视窗
// push_view_or_pull_view	
//		1	push view, 将系统升级视窗压入到当前视窗栈的栈顶
//		0	pull view, 将当前视窗栈顶部的视窗弹出, 然后再将系统升级视窗压入到视窗栈的栈顶
VOID AP_OpenSystemUpdateView (unsigned int push_view_or_pull_view)
{
	
	// 检查卡状态 (卡拔出、卡写保护禁止执行操作)
	DWORD CardStatus = XM_GetFmlDeviceCap (DEVCAP_SDCARDSTATE);

	if (CardStatus == DEVCAP_SDCARDSTATE_UNPLUG)
	{
		// 卡已拔出
		AP_PostSystemEvent (SYSTEM_EVENT_CARD_UNPLUG);
		return;
	}
	else if (CardStatus == DEVCAP_SDCARDSTATE_INVALID)
	{
		// 卡无效事件
		AP_PostSystemEvent (SYSTEM_EVENT_CARD_INVALID);
		return;
	}
	else if(CardStatus == DEVCAP_SDCARDSTATE_FS_ERROR)
	{
		// 卡文件系统错误
		XM_OpenAlertView (	
			AP_ID_CARD_MESSAGE_CARD_FSERROR,	// 信息文本资源ID
			0,			// 图片信息资源ID
			0,
			0,												// 按钮文字资源ID
			0,												// 按钮按下文字资源ID
			APP_ALERT_BKGCOLOR,
			10.0,											// 5秒钟自动关闭
			APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
			NULL,											// 按键回调函数
			NULL,
			XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
			//XM_VIEW_ALIGN_CENTRE,
			0												// ALERTVIEW视图的控制选项
			);
		return;
	}

	// 检查主电池电压
	#if 0
	if(XM_GetFmlDeviceCap (DEVCAP_MAINBATTERYVOLTAGE) == DEVCAP_BATTERYVOLTAGE_WORST)
	{
		// 电池电量极低
		AP_PostSystemEvent (SYSTEM_EVENT_MAIN_BATTERY);
		return;
	}
    #endif
	
	app_start_update (push_view_or_pull_view);

}