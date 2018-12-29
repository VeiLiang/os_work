//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_win_main.c
//	  gdi主函数
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#include <string.h>
#include <xm_user.h>
#include <xm_base.h>
#include <xm_heap_malloc.h>
#include <xm_dev.h>
#include <xm_rom.h>
#include <xm_dev.h>
#include <xm_gdi.h>
#include "xm_gdi_device.h"
#include "xm_device.h"
#include <xm_printf.h>
#include <xm_videoitem.h>
#include "xm_voice_prompts.h"
#include <stdlib.h>
#include <xm_malloc.h>
#include "xm_h264_codec.h"
#include "xm_core.h"
// 桌面视窗声明
XMHWND_DECLARE(Desktop)
XMHWND_DECLARE(FactoryView)

#define	MAX_ENVIRONMENT_SIZE		(XM_USER_MEM_SIZE * 2)	// 2倍用户RAM字节大小临时缓存
extern void xm_save_environment (unsigned char *buff, unsigned int size);
extern void xm_restore_environment (unsigned char *buff, unsigned int size);



int XM_Main (void)
{	
	unsigned char *environment = NULL;
	DWORD dwExitCode = 0;

	XM_printf ("XM_Main Entry\n");

	dwExitCode = 0;
	do
	{
		// 2 字库、显示开启

		// 3 用户内存分配模块初始化
		// winmem_init ();

		// 消息系统初始化
		winuser_init ();

		// 显示系统初始化
		wingdi_init ();

		// 2 键盘、定时器、MCI驱动开启, 开始接受外部事件
		XM_TimerDriverOpen ();
		XM_KeyDriverOpen ();
		XM_TouchDriverOpen ();
#ifndef WIN32
		// 遥控器模块初始化
		//RemoteKeyInit ();	
		
		// UART驱动初始化
		XM_UartDriverOpen ();
#endif

		// 启动桌面
		if(dwExitCode == XMEXIT_CHANGE_RESOLUTION && environment)
		{
			// 分辨率修改模式下，恢复系统运行环境
			xm_restore_environment (environment, MAX_ENVIRONMENT_SIZE);
			// XM_heap_free (environment);
			kernel_free (environment);
			environment = NULL;

			// 检查桌面是否存在
			if(XM_GetWindow() == NULL)
			{
				XM_PushWindow (XMHWND_HANDLE(Desktop));
			}
			// 重新进入消息
			XM_PostMessage (XM_ENTER, 1, 1);		// lp bit0 = 1表示重新进入的原因是分辨率修改
			XM_InvalidateWindow ();
		}
		else
		{
			XM_PushWindow (XMHWND_HANDLE(Desktop));
		}


		// 主消息循环
		dwExitCode = XM_MainLoop ();

		XM_printf ("XM_MainLoop Exit\n");

		if(dwExitCode == XMEXIT_CHANGE_RESOLUTION)
		{
			// 分辨率修改模式下，保存系统运行环境
			//environment = XM_heap_malloc (MAX_ENVIRONMENT_SIZE);		
			environment = kernel_malloc (MAX_ENVIRONMENT_SIZE);	
			if(environment)
			{
				xm_save_environment (environment, MAX_ENVIRONMENT_SIZE);
			}
		}

		// 停止播放或录像过
		XMSYS_H264CodecStop ();


#ifndef WIN32
		// UART驱动关闭
		XM_UartDriverClose ();

		// 遥控器模块关闭
		//RemoteKeyExit ();		
#endif
		// 键盘、定时器、MCI驱动关闭
		XM_TouchDriverClose ();
		XM_KeyDriverClose ();
		XM_TimerDriverClose ();

		// 显示系统关闭
		wingdi_exit ();

		// 消息系统关闭
		winuser_exit ();

		// GUI内存系统关闭
		//winmem_exit ();

	} while (dwExitCode == XMEXIT_CHANGE_RESOLUTION);
	return dwExitCode;
}



