//****************************************************************************
//
//	Copyright (C) 2010 Shenzhen Exceedspace Digital Technology Co.,LTD
//
//	Author	ZhuoYongHong
//
//	File name: xm_tester.c
//	  tester interface
//
//	Revision history
//
//		2006.04.25	ZhuoYongHong begin
//		2009.08.01	ZhuoYongHong 加入笔事件支持
//
//****************************************************************************
#include "xm_type.h"
#include "xm_base.h"
#include "xm_tester.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "wintest.h"
#include <stdio.h>
#include <assert.h>
#include "xm_dev.h"
#include "xm_printf.h"
#include "xm_user.h"
#include "xm_file.h"
#include "rtos.h"

#define	MAX_PATH	127

static volatile int at_stop = 0;
static volatile int at_running = 0;		// 1 表示自动测试已启动 0 表示自动测试未启动
static OS_RSEMA				at_sema;	


int OnTesterStart (const char *script_file) 
{
	// TODO: Add your command handler code here
	XMTESTERSTART StartParam;
	char message[256];
	int result;

	memset (&StartParam, 0, sizeof(StartParam));


	StartParam.uTestMode = TESTER_MODE_SCRIPT;

	
	StartParam.lpKeyMapFile = "keymap.def";

	StartParam.fpCaptureBitmap = 0;
	StartParam.lpScriptFile = script_file;

	result = XM_TesterSendCommand (TESTER_CMD_START, &StartParam);

	if(result == 0)
		return 0;

	switch(result)
	{
		case TESTERSYSERR_BUSY:
			sprintf (message, "脚本服务忙，请关闭已经打开的自动测试脚本");
			break;

		case TESTERSYSERR_INVALIDSCRIPTFILE:
			sprintf (message, "无效自动测试脚本文件(%s)", StartParam.lpScriptFile);
			break;

		case TESTERSYSERR_INVALIDSCRIPTLINE:
			sprintf (message, "脚本文件(%s)存在语法错误, 请检查记录文件(%s.log)获取详细信息", 
				StartParam.lpScriptFile, StartParam.lpScriptFile);
			break;

		case TESTERSYSERR_INVALIDKEYMAPFILE:
			sprintf (message, "无效键盘映射文件(%s)", StartParam.lpKeyMapFile);
			break;

		case TESTERSYSERR_FAILTTOCREATELOG:
			sprintf (message, "不能打开LOG文件(%s.log)", StartParam.lpScriptFile);
			break;

		case TESTERSYSERR_FAILTOCREATEBITMAP:
			sprintf (message, "不能打开位图文件(%s)", (char *)(void *)StartParam.fpCaptureBitmap);
			break;

		default:
			sprintf (message, "未知错误类型(%d)", result);
			break;
	}
	
	XM_TesterSendCommand (TESTER_CMD_STOP, 0);

	XM_printf ("脚本执行异常, %s", message);	
	return -1;
}

void xm_autotest_init (void)
{
	at_stop = 0;
	at_running = 0;
	OS_CREATERSEMA(&at_sema); /* Creates resource semaphore */
}

int xm_autotest_stop (void)
{
	unsigned int timeout_ticket;
	int waiting_for_stop = 0;
	int ret = 0;
	OS_Use(&at_sema); 	
	if(at_running)
		waiting_for_stop = 1;	// 等待自动测试结束
	at_stop = 1;
	OS_Unuse(&at_sema); 	
	
	if(waiting_for_stop)
	{
		timeout_ticket = XM_GetTickCount() + 4000;
		while(at_running)
		{
			if(XM_GetTickCount() >= timeout_ticket)
			{
				ret = -1;
				XM_printf ("xm_autotest_stop failed, at busy\n");
				break;
			}
			OS_Delay (20);
		}
	}
	
	return ret;
}

// 
int xm_autotest_run (const char *autotest_script_file)
{
	XMTESTERMESSAGE Message;
#ifdef _WINDOWS
	char log_filename[MAX_PATH];
	char *ext;
	void *fp_log;
#endif
	int ret;
	char event_message[32];
	
	if(autotest_script_file == NULL || *autotest_script_file == 0)
	{
		XM_printf ("xm_autotest_run failed, illegal script file\n");
		return -1;
	}

	OS_Use(&at_sema);	
	if(at_stop)
	{
		at_stop = 0;
		OS_Unuse(&at_sema); 	
		XM_printf ("xm_autotest_run failed, stop\n");
		return -1;
	}
	if(OnTesterStart (autotest_script_file) < 0)
	{
		XM_printf ("xm_autotest_run failed, load script NG\n");
		OS_Unuse(&at_sema); 	
		return -1;
	}
	at_stop = 0;
	at_running = 1;
	OS_Unuse(&at_sema); 	

#ifdef _WINDOWS
	memset (log_filename, 0, sizeof(log_filename));
	strcpy (log_filename, autotest_script_file);
	ext = strrchr (log_filename, '.');
	if(ext)
	{
		ext[1] = 'L';
		ext[2] = 'O';
		ext[3] = 'G';
	}
	else
	{
		sprintf (log_filename, "%s.LOG", autotest_script_file);
	}

	fp_log = XM_fopen (log_filename, "wb");
#endif


	do
	{
		if(at_stop == 1)
			break;
		
		// 检查自动测试消息
		if(XM_TesterSendCommand (TESTER_CMD_MESSAGE, &Message))
		{
			const char *key_name;
			ret = 0;
			if(Message.message == XM_KEYDOWN)
			{
				ret = XM_KeyEventProc (Message.wParam, XMKEY_PRESSED | XMKEY_STROKE);
				XM_Delay (20);

			}
			else if(Message.message == XM_KEYUP)
			{
				ret = XM_KeyEventProc (Message.wParam, 0 );
				XM_Delay (20);
			}
			else if(Message.message == 0)
			{
				continue;
			}
			else
			{
				break;
			}

			key_name = TesterGetKeyName (Message.wParam);
			if(ret == 1)
			{
				if(Message.message == XM_KEYDOWN)
					sprintf (event_message, "%s\n",  key_name);
			//	else
			//		sprintf (event_message, "%s : XM_KEYUP\n",  key_name);


#ifdef _WINDOWS
				if(fp_log)
				{
					XM_fwrite (event_message, 1, strlen(event_message), fp_log);
					XM_fflush (fp_log);
				}
#endif
			}	
			else
			{

			}
		}
		else
		{
			break;
		}
	} while (at_stop == 0);

	XM_KeyEventProc (0xFFFF, 0);

#ifdef _WINDOWS
	if(fp_log)
		XM_fclose (fp_log);
#endif

	OS_Use(&at_sema);	
	at_stop = 0;
	at_running = 0;
	OS_Unuse(&at_sema);
	
	return 0;
}