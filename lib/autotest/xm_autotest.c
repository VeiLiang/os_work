//****************************************************************************
//
//	Copyright (C) 2017 Shenzhen Exceedspace Digital Technology Co.,LTD
//
//	Author	ZhuoYongHong
//
//	File name: xm_autotest.c
//	  auto test interface
//
//	Revision history
//
//		2017.05.01	ZhuoYongHong 将自动测试功能加入到n141
//
//****************************************************************************
#include "xm_type.h"
#include "xm_base.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include "rtos.h"
#include "fs.h"
#include "xm_core.h"
#include "xm_file.h"
#include "xm_printf.h"
#include "xm_autotest.h"


static OS_TASK TCB_AutoTestTask;
__no_init static OS_STACKPTR int StackAutoTestTask[XMSYS_AUTOTEST_TASK_STACK_SIZE/4];          /* Task stacks */

void XMSYS_AutoTestTask (void)
{
	OS_Delay (1000);
	
	while(1)
	{
		// 检查文件系统是否存在
		while(1)
		{
			int volume_status = FS_GetVolumeStatus ("mmc:");
			if(volume_status == FS_MEDIA_IS_PRESENT)
				break;
			OS_Delay (50);
		}
		
		// 检查SD卡的状态
		unsigned int card_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
		if(card_state != DEVCAP_SDCARDSTATE_INSERT)
		{
			OS_Delay (50);
			continue;
		}
		
		// 检查按键定义文件是否存在
		void *fp = XM_fopen ("\\keymap.def", "rb");
		if(fp)
		{
			XM_fclose (fp);

			// 检查测试脚本是否存在
			fp = XM_fopen ("\\autotest.seq", "rb");
			if(fp)
			{
				XM_fclose (fp);
				
				XM_printf ("autotest start ...\n");
				// 脚本存在
				xm_autotest_run ("\\autotest.seq");
				
				XM_printf ("autotest end !\n");
				
				/*
				// 等待磁盘卷拔出
				XM_printf ("Please Plug out the SD card!\n");
				while(1)
				{
					int volume_status = FS_GetVolumeStatus ("mmc:");
					if(volume_status != FS_MEDIA_IS_PRESENT)
						break;
					OS_Delay (50);
				}
				
				XM_printf ("Please insert the SD card to restart autotest!\n");
				*/
			}
			else
			{
				XM_printf ("autotest stop, miss \\autotest.seq\n");
			}
		}
		else
		{
			XM_printf ("autotest stop, miss \\keymap.def\n");
		}
		
		// 等待磁盘卷拔出
		//XM_printf ("Please Plug out the SD card!\n");
		while(1)
		{
			int volume_status = FS_GetVolumeStatus ("mmc:");
			if(volume_status != FS_MEDIA_IS_PRESENT)
				break;
			OS_Delay (50);
		}
	
		OS_Delay (50);
	}
	
}

void XMSYS_AutoTestInit (void)
{
	xm_autotest_init ();
	OS_CREATETASK(&TCB_AutoTestTask, "AutoTestTask", XMSYS_AutoTestTask, XMSYS_AUTOTEST_TASK_PRIORITY, StackAutoTestTask);
}


