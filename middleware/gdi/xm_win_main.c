//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_win_main.c
//	  gdi������
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
// �����Ӵ�����
XMHWND_DECLARE(Desktop)
XMHWND_DECLARE(FactoryView)

#define	MAX_ENVIRONMENT_SIZE		(XM_USER_MEM_SIZE * 2)	// 2���û�RAM�ֽڴ�С��ʱ����
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
		// 2 �ֿ⡢��ʾ����

		// 3 �û��ڴ����ģ���ʼ��
		// winmem_init ();

		// ��Ϣϵͳ��ʼ��
		winuser_init ();

		// ��ʾϵͳ��ʼ��
		wingdi_init ();

		// 2 ���̡���ʱ����MCI��������, ��ʼ�����ⲿ�¼�
		XM_TimerDriverOpen ();
		XM_KeyDriverOpen ();
		XM_TouchDriverOpen ();
#ifndef WIN32
		// ң����ģ���ʼ��
		//RemoteKeyInit ();	
		
		// UART������ʼ��
		XM_UartDriverOpen ();
#endif

		// ��������
		if(dwExitCode == XMEXIT_CHANGE_RESOLUTION && environment)
		{
			// �ֱ����޸�ģʽ�£��ָ�ϵͳ���л���
			xm_restore_environment (environment, MAX_ENVIRONMENT_SIZE);
			// XM_heap_free (environment);
			kernel_free (environment);
			environment = NULL;

			// ��������Ƿ����
			if(XM_GetWindow() == NULL)
			{
				XM_PushWindow (XMHWND_HANDLE(Desktop));
			}
			// ���½�����Ϣ
			XM_PostMessage (XM_ENTER, 1, 1);		// lp bit0 = 1��ʾ���½����ԭ���Ƿֱ����޸�
			XM_InvalidateWindow ();
		}
		else
		{
			XM_PushWindow (XMHWND_HANDLE(Desktop));
		}


		// ����Ϣѭ��
		dwExitCode = XM_MainLoop ();

		XM_printf ("XM_MainLoop Exit\n");

		if(dwExitCode == XMEXIT_CHANGE_RESOLUTION)
		{
			// �ֱ����޸�ģʽ�£�����ϵͳ���л���
			//environment = XM_heap_malloc (MAX_ENVIRONMENT_SIZE);		
			environment = kernel_malloc (MAX_ENVIRONMENT_SIZE);	
			if(environment)
			{
				xm_save_environment (environment, MAX_ENVIRONMENT_SIZE);
			}
		}

		// ֹͣ���Ż�¼���
		XMSYS_H264CodecStop ();


#ifndef WIN32
		// UART�����ر�
		XM_UartDriverClose ();

		// ң����ģ��ر�
		//RemoteKeyExit ();		
#endif
		// ���̡���ʱ����MCI�����ر�
		XM_TouchDriverClose ();
		XM_KeyDriverClose ();
		XM_TimerDriverClose ();

		// ��ʾϵͳ�ر�
		wingdi_exit ();

		// ��Ϣϵͳ�ر�
		winuser_exit ();

		// GUI�ڴ�ϵͳ�ر�
		//winmem_exit ();

	} while (dwExitCode == XMEXIT_CHANGE_RESOLUTION);
	return dwExitCode;
}



