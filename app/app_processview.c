//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	SongXing
//
//	File name: app_process.c
//	格式化SD 卡
//
//	Revision history
//
//		2012.09.01	SongXing Initial version
//
//****************************************************************************
/*
*/
#include "rtos.h"		// 任务相关函数
#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"
#include "systemapi.h"
#include "app_processview.h"
#include "xm_h264_codec.h"
#include <xm_gif.h>
#include <xm_semaphore.h>
#include "xm_autotest.h"
#include "fs.h"
#include "xm_proj_define.h"
#include "xm_flash_space_define.h"


#define	OSD_AUTO_HIDE_TIMEOUT						2000	


#define	XMSYS_PROCESS_TASK_PRIORITY					200	
#define	XMSYS_PROCESS_TASK_STACK_SIZE				0x1000


static OS_TASK TCB_ProcessTask;
static OS_STACKPTR int StackProcessTask[XMSYS_PROCESS_TASK_STACK_SIZE/4];          /* Task stacks */


#define READY_TO_PROCESS	0
#define PROCESSING			1
#define END_OF_PROCESS		2

typedef struct tagPROCESSVIEWDATA {
	AP_PROCESSINFO	ProcessInfo;		// 过程控制参数

	BYTE 			curitem;//确认,取消index
	BYTE			enablekey;

	void *			process_sync_semaphore;			// 同步信号量控制
	int				process_state;					// 状态
	int				result;							// 保存外部过程操作的返回值
} PROCESSVIEWDATA;

static int process_task_exit (PROCESSVIEWDATA *ProcessViewData);


// 视频任务主函数
void XMSYS_ProcessTask (PROCESSVIEWDATA *ProcessViewData) 
{	
	// Video任务初始化完毕
	//int ret;
	XM_printf(">>>>>>>>>>>>>>>>>XMSYS_ProcessTask start................\r\n");
	ProcessViewData->process_state = PROCESS_STATE_WORKING;
	XM_SignalSemaphore(ProcessViewData->process_sync_semaphore);
	
	// 终止编解码
	//ret = XMSYS_H264CodecStop ();
	//if(ret == 0)
	{
		if(ProcessViewData->ProcessInfo.type == AP_PROCESS_VIEW_FORMAT)
		{
			// 格式化
			//xm_close_volume_service ("mmc:0");
			OS_Delay (500);
			
			// 执行格式化操作
			ProcessViewData->result = (*ProcessViewData->ProcessInfo.fpStartProcess)(ProcessViewData->ProcessInfo.lpPrivateData);
			
			if(ProcessViewData->result == 0)
			{
				// 格式化成功
				FS_CACHE_Clean ("");
				if(FS_Mount ("mmc:0:") >= 0)
				{
					xm_open_volume_service("mmc:0:", 0);
				}
				else
				{
					XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_INVALID);
				}
			}
			else
			{
				XM_SetFmlDeviceCap (DEVCAP_SDCARDSTATE, DEVCAP_SDCARDSTATE_INVALID);
			}
		}
		else if(ProcessViewData->ProcessInfo.type == AP_PROCESS_VIEW_RESTORESETTING)
		{
			// 恢复出厂设置
			ProcessViewData->result = (*ProcessViewData->ProcessInfo.fpStartProcess)(ProcessViewData->ProcessInfo.lpPrivateData);
			// 开启背光
			AP_SetMenuItem(APPMENUITEM_LCD, AppMenuData.lcd);
				
			// 设置录像的分辨率/帧率
			XMSYS_H264CodecSetVideoFormat(AppMenuData.video_resolution);
		}
		else if(ProcessViewData->ProcessInfo.type == AP_PROCESS_VIEW_SYSTEMUPDATE)
		{// 系统升级
			ProcessViewData->result = (*ProcessViewData->ProcessInfo.fpStartProcess)(ProcessViewData->ProcessInfo.lpPrivateData);
		}
	}

	if(ProcessViewData->result == 0)
		ProcessViewData->process_state = PROCESS_STATE_SUCCESS;
	else
		ProcessViewData->process_state = PROCESS_STATE_FAILURE;
}



void  XMSYS_ProcessTask_rtos (void *user_data)
{
	PROCESSVIEWDATA * ProcessViewData = (PROCESSVIEWDATA *)user_data;
	
	XMSYS_ProcessTask((PROCESSVIEWDATA *)user_data);

	XM_printf(">>>>>>>>>>>>>>>>>>XMSYS_ProcessTask End..............\r\n");

	XM_printf(">>>>>>>>>>>>XMSYS_ProcessTask_rtos, ProcessViewData->process_state:%d\r\n", ProcessViewData->process_state);
	XM_InvalidateWindow ();
	XM_UpdateWindow ();	
	// 终止当前任务
	OS_Terminate (NULL);
}


static int process_task_init (PROCESSVIEWDATA *ProcessViewData)
{
	if(ProcessViewData == NULL)
		return -1;

	// 已启动或操作完成
	//if(ProcessViewData->process_state != PROCESS_STATE_INITIAL && ProcessViewData->process_state != PROCESS_STATE_CONFIRM)
	//	return 0;

    if(ProcessViewData->process_sync_semaphore != NULL)
		return -1;
	
	ProcessViewData->process_sync_semaphore = XM_CreateSemaphore ("process_sync_semaphore", 0);
	if(!ProcessViewData->process_sync_semaphore)
	{
		return -1;
	}

	// 标记卡重新检测并关闭视频播放
	//APPMarkCardChecking(1);
	
	OS_CREATETASK_EX(&TCB_ProcessTask, "ProcessTask", 
						XMSYS_ProcessTask_rtos, 
						XMSYS_PROCESS_TASK_PRIORITY, 
						StackProcessTask,
						ProcessViewData
					);

	// 等待任务启动
	if(XM_WaitSemaphore(ProcessViewData->process_sync_semaphore) == 0)
	{
		XM_printf(">>>>>>>>>>>>>>>.process_task_init NG, wait semaphore \"process_sync_semaphore\" failed\n");

		process_task_exit (ProcessViewData);
		return -1;
	}

	return 0;
}

static int process_task_exit (PROCESSVIEWDATA *ProcessViewData)
{
	if(ProcessViewData == NULL)
		return -1;

	if(ProcessViewData->process_sync_semaphore)
	{
		// 关闭信号灯
		XM_CloseSemaphore (ProcessViewData->process_sync_semaphore);
		// 删除信号灯
		XM_DeleteSemaphore (ProcessViewData->process_sync_semaphore);
		ProcessViewData->process_sync_semaphore = NULL;
	}

	return 0;
}


VOID ProcessViewOnEnter(XMMSG *msg)
{
	PROCESSVIEWDATA *ProcessViewData;
	AP_PROCESSINFO* lpProcessInfo;

	if(msg->wp == 0)
	{
		// 窗口未建立，第一次进入
		XMRECT rect;
		lpProcessInfo = (AP_PROCESSINFO *)msg->lp;

		XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>ProcessViewOnEnter, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

		if(lpProcessInfo->type==AP_PROCESS_VIEW_SYSTEMUPDATE)
		{
			// 标记卡重新检测并关闭视频播放
			//APPMarkCardChecking (1);
		}

		
		// 参数检查
		if(lpProcessInfo == NULL)
			return;
		if(lpProcessInfo->fpStartProcess == NULL)
			return;

		// 分配私有数据句柄
		ProcessViewData = XM_calloc (sizeof(PROCESSVIEWDATA));
		if(ProcessViewData == NULL)
		{
			XM_printf ("ProcessViewData XM_calloc failed\n");
			// 失败返回到调用者窗口
			XM_PullWindow (0);
			return;
		}

		ProcessViewData->process_state = PROCESS_STATE_INITIAL;
		ProcessViewData->curitem = 0;
		ProcessViewData->enablekey = 1;
		
		memcpy(&ProcessViewData->ProcessInfo, lpProcessInfo, sizeof(ProcessViewData->ProcessInfo));
		
		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData(XMHWND_HANDLE(ProcessView), ProcessViewData);
		// 设置视窗的alpha值
		XM_SetWindowAlpha(XMHWND_HANDLE(ProcessView), (unsigned char)(0));
		XM_SetWindowPos(XMHWND_HANDLE(ProcessView), 287, 169, 441, 258);
	}
	else
	{
		ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ProcessView));

		// 窗口已建立，当前窗口从栈中恢复
		XM_printf(">>>>>>>>>>>>>>ProcessView Pull......................\n");
	}
	
	// 创建定时器，用于菜单的隐藏
	// 创建x秒的定时器
	//XM_SetTimer(XMTIMER_PROCESSVIEW, OSD_AUTO_HIDE_TIMEOUT);
}


VOID ProcessViewOnLeave (XMMSG *msg)
{
	// 删除定时器
	XM_KillTimer (XMTIMER_PROCESSVIEW);
	
	PROCESSVIEWDATA *ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ProcessView));
	if (msg->wp == 0)
	{
		// 窗口退出，彻底摧毁。
		// 获取私有数据句柄
		// 释放所有分配的资源
		if(ProcessViewData)
		{
			// 释放私有数据句柄
			XM_free (ProcessViewData);
		}
		XM_printf ("ProcessView Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
		XM_printf ("ProcessView Push\n");
	}
}

static void draw_progress_icon (void)
{
	XMRECT rect;
	XMRECT rc_pic;
	int offset;
	PROCESSVIEWDATA *ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ProcessView));

	if(ProcessViewData == NULL)
		return;

	// 检查定时器是否已开启
	//if(!is_timer_running(ProcessViewData))
	//	return;

	if(ProcessViewData->process_state == PROCESS_STATE_INITIAL)
		return;

	XM_printf(">>>>>>>>>>>>>>draw_progress_icon, ProcessViewData->process_state:%d\r\n", ProcessViewData->process_state);
	if(	(ProcessViewData->process_state == PROCESS_STATE_SUCCESS) || (ProcessViewData->process_state == PROCESS_STATE_FAILURE) )
	{//执行完SD卡格式化后,释放数据
		XM_printf(">>>>>>>>>>>>>>All Process End.............\r\n");

		process_task_exit(ProcessViewData);//删除信号量

		// 检查是否存在"过程结束处理程序". 若有, 执行该"过程结束处理程序"
		if(ProcessViewData->ProcessInfo.fpEndProcess)
		{
			XM_printf(">>>>>>>>>>>>>>>>process end process.......\r\n");
			(*ProcessViewData->ProcessInfo.fpEndProcess) ((void *)ProcessViewData->result);
		}
		else
		{
			XM_printf(">>>>>>>>>>>>>>>>no end process.......\r\n");
		}
		//XM_InvalidateWindow ();
		//XM_UpdateWindow ();
		return;
	}
	else
	{
		
	}
}

#define MSG_BODY_WIDTH  400

VOID ProcessViewOnPaint (XMMSG *msg)
{
	char text[64];
	char String[32];
	XMSIZE size;
	XMRECT rc, rect;
	unsigned int old_alpha;
	HANDLE hwnd = XMHWND_HANDLE(ProcessView);

	PROCESSVIEWDATA *ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(ProcessViewData == NULL)
		return;

	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>ProcessViewOnPaint, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);
	
    XM_GetWindowRect(XMHWND_HANDLE(Desktop),&rc);
	XM_FillRect(hwnd, rc.left, rc.top, rc.right, rc.bottom, 0x00000000);
	
	// 获取视窗位置信息
	XM_GetWindowRect(XMHWND_HANDLE(ProcessView), &rc);
	XM_FillRect(XMHWND_HANDLE(ProcessView), rc.left, rc.top, rc.right, rc.bottom, 0xB0404040);

	old_alpha = XM_GetWindowAlpha(hwnd);
	XM_SetWindowAlpha (hwnd, 255);

	//背景图片
	rect = rc;
	AP_RomImageDrawByMenuID(AP_ID_ALERT_BG_PIC, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	rect = rc;
	rect.left = rect.left + 174; rect.top = rect.top + 2;
	AP_RomImageDrawByMenuID(ProcessViewData->ProcessInfo.Title, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	//根据状态变化,切换不同文字显示;
	rect = rc;
	if(ProcessViewData->process_state==PROCESS_STATE_INITIAL)
	{
		//rect.left = rect.left + 113;
		rect.left = rect.left + (rect.right - rect.left - MSG_BODY_WIDTH)/2;
		rect.top = rect.top + 84;
	}
	else if(ProcessViewData->process_state==PROCESS_STATE_WORKING)
	{
		//rect.left = rect.left + 142;
		rect.left = rect.left + (rect.right - rect.left - MSG_BODY_WIDTH)/2;
		rect.top = rect.top + 84;
	}
	else if(ProcessViewData->process_state==PROCESS_STATE_SUCCESS)
	{
		//rect.left = rect.left + 162;
		rect.left = rect.left + (rect.right - rect.left - MSG_BODY_WIDTH)/2;
		rect.top = rect.top + 84;
		rect.right = rect.left + MSG_BODY_WIDTH;
	}
	AP_RomImageDrawByMenuID(ProcessViewData->ProcessInfo.DispItem[ProcessViewData->process_state], hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	//按钮选择框
	rect = rc;
	if(ProcessViewData->curitem==0)
	{//取消
		rect.left = rect.left + 96; rect.top = rect.top + 193;
	}
	else
	{//确定
		rect.left = rect.left + 296; rect.top = rect.top + 193;
	}
	AP_RomImageDrawByMenuID(AP_ID_SD_BUTTON, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	//取消按钮
	rect = rc;
	rect.left = rect.left + 100; rect.top = rect.top + 194;
	AP_RomImageDrawByMenuID(AP_ID_SD_CANCEL, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	//确定按钮
	rect = rc;
	rect.left = rect.left + 300; rect.top = rect.top + 194;
	AP_RomImageDrawByMenuID(AP_ID_SD_OK, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);

	draw_progress_icon();

	if(ProcessViewData->ProcessInfo.type==AP_PROCESS_VIEW_SYSTEMUPDATE)
	{
		rect = rc;
		rect.left = rect.left + 20; rect.top = rect.top + 150;	

		int setp = (*ProcessViewData->ProcessInfo.fpQueryProgress)(ProcessViewData->ProcessInfo.lpPrivateData);
		XM_printf(">>>>>>>>>>>setp:%d\r\n", setp);
	    sprintf(String,"%d", setp);
	    AP_TextGetStringSize(String,sizeof(String),&size);
	    AP_TextOutDataTimeString(hwnd, rect.left, rect.top, String, strlen(String));
	}
	XM_SetWindowAlpha (hwnd, (unsigned char)old_alpha);
}

VOID ProcessViewOnKeyDown (XMMSG *msg)
{
	PROCESSVIEWDATA *ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ProcessView));
	if(ProcessViewData == NULL)
		return;

	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>ProcessViewOnKeyDown, msg->wp:%d, msg->lp:%x\r\n", msg->wp, msg->lp);

	//按键音
	XM_Beep(XM_BEEP_KEYBOARD);
	switch(msg->wp)
	{
	    case REMOTE_KEY_LEFT:
	    case REMOTE_KEY_RIGHT:
		case VK_AP_DOWN:
		case VK_AP_UP:
			if(ProcessViewData->enablekey==1)
			{
				if(ProcessViewData->curitem==0)
				{
					ProcessViewData->curitem = 1;
				}
				else
				{
					ProcessViewData->curitem = 0;
				}
				XM_InvalidateWindow ();
				XM_UpdateWindow ();	
			}
			break;
        case REMOTE_KEY_DOWN:
		case VK_AP_SWITCH:
			if(ProcessViewData->curitem==1)
			{
				ProcessViewData->enablekey = 0;
				//启动线程
				if(process_task_init(ProcessViewData) < 0)
				{
					ProcessViewData->process_state = PROCESS_STATE_WORKING;
				}

				XM_printf(">>>>>>>>>>>>>qqq ProcessViewData->process_state:%d\r\n", ProcessViewData->process_state);
				if(ProcessViewData->ProcessInfo.type==AP_PROCESS_VIEW_SYSTEMUPDATE)
				{
					XM_SetTimer(XMTIMER_PROCESSVIEW, 500);//需要刷进度条,加快时间
				}
				else
				{
					XM_SetTimer(XMTIMER_PROCESSVIEW, OSD_AUTO_HIDE_TIMEOUT);
				}
				XM_InvalidateWindow();
				XM_UpdateWindow();				
			}
			else if(ProcessViewData->curitem==0)
			{
				XM_PullWindow(0);
			}
			break;
			
		default:
			break;
	}

}

VOID ProcessViewOnKeyUp (XMMSG *msg)
{
	PROCESSVIEWDATA *ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ProcessView));
	if(ProcessViewData == NULL)
		return;
}

VOID ProcessViewOnTimer (XMMSG *msg)
{
	xm_osd_framebuffer_t framebuffer;
	XMRECT rect;
	PROCESSVIEWDATA *ProcessViewData;

	ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ProcessView));
	if(ProcessViewData == NULL)
		return;

	XM_printf(">>>>>>>>>>>>>>>>>ProcessViewOnTimer..............\r\n");
	if((ProcessViewData->process_state==PROCESS_STATE_SUCCESS) || ProcessViewData->process_state==PROCESS_STATE_FAILURE)
	{
		XM_PullWindow(0);
	}

	if(ProcessViewData->ProcessInfo.type==AP_PROCESS_VIEW_SYSTEMUPDATE)
	{
		XM_InvalidateWindow();
		XM_UpdateWindow();	
	}
}

// 系统事件处理
VOID ProcessViewOnSystemEvent (XMMSG *msg)
{
	PROCESSVIEWDATA *ProcessViewData = (PROCESSVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(ProcessView));
	if(ProcessViewData == NULL)
		return;
	if(ProcessViewData->ProcessInfo.fpSystemEventCb)
	{
		(*ProcessViewData->ProcessInfo.fpSystemEventCb)(msg, ProcessViewData->process_state);
	}
}

// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (ProcessView)
	XM_ON_MESSAGE (XM_PAINT, ProcessViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, ProcessViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, ProcessViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, ProcessViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, ProcessViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, ProcessViewOnTimer)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, ProcessViewOnSystemEvent)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, ProcessView, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_ALERT)

