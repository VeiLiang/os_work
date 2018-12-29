#include <stdio.h>
#include <stdlib.h>
#include "RTOS.h"
#include "FS.h"
#include <xmkey.h>
#include <xmuser.h>
#include <xmdev.h>
#include "xmcore.h"
#include "xmvideoitem.h"
#include "xm_h264_cache_file.h"
#include "sdmmc.h"
#include "xmprintf.h"

// ������
OS_STACKPTR int StackMain[XMSYS_MAIN_TASK_STACK_SIZE/4];          /* Task stacks */
OS_TASK TCBMAIN;                        /* Task-control-blocks */

static unsigned int _FileSystemCache[_XMSYS_FS_CACHE_SIZE_/4];

// ��ʼ��ϵͳ��������
static void InitSystemBasicService (void)
{
	printf ("InitSystemBasicService Init\n");
	
	// �ļ�ϵͳ��Cache
	printf ("FS_Init\n");
	FS_Init ();
	FS_AssignCache ("", _FileSystemCache, sizeof(_FileSystemCache), FS_CACHE_RW);
	//FS_CACHE_SetMode ("", FS_SECTOR_TYPE_MASK_DIR|FS_SECTOR_TYPE_MASK_MAN, FS_CACHE_MODE_WT);
	FS_CACHE_SetMode("", FS_SECTOR_TYPE_MASK_DIR|FS_SECTOR_TYPE_MASK_MAN, FS_CACHE_MODE_WB);

	SDCard_Module_CardCheck ();
		
	printf ("InitSystemBasicService Success\n");
}

OS_STACKPTR int StackVideoItemTest[0x10000/2];          /* Task stacks */
OS_TASK TCB_VideoItemTest;                        /* Task-control-blocks */
static const char* video_path[] = {
	"mmc:0:\\video_f\\"
};
void VideoItemTestTask (void)
{
	unsigned int i = 0;
	while(1)
	{
		i ++;
		// ÿ15��ɨ��SD������
		if((i % 3) == 0)
		{
			XM_VideoItemMonitorAndRecycleGarbage (0);
		}
		
		OS_Delay (5000);
	}
}

extern int h264codec_simulator (void);

static void scan_disk (void)
{
	char file_name[XM_MAX_FILEFIND_NAME];
	XMFILEFIND fileFind;
	do 
	{
		if(XM_FindFirstFile ((char *)"\\", &fileFind, file_name, XM_MAX_FILEFIND_NAME))
		{
			do {
				// ���Ŀ¼������
				DWORD dwFileAttributes = fileFind.dwFileAttributes;
				// ����Ŀ¼���Ƿ����ļ�����
				if((dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == 0x00 )
				{
					XM_printf ("FILE  %s\n", file_name);
				}
				else if((dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == ATTR_DIRECTORY)
				{
					XM_printf ("DIR   %s\n", file_name);
				}
				else
				{
					XM_printf ("SYS   %s\n", file_name);
				}

			} while (XM_FindNextFile (&fileFind));
			XM_FindClose (&fileFind);
		}
	} while(0);
}

static void MainTask(void)
{
	XM_VideoItemInit (1, video_path);
	XMSYS_H264FileInit ();
	// SD���ļ�ϵͳ��ʼ��
	InitSystemBasicService ();	
	//while(1)
	{
		OS_Delay (10);
	}

	//scan_disk (); while(1);

	OS_CREATETASK(&TCB_VideoItemTest, "VideoItemTestTask", VideoItemTestTask, XMSYS_APP_TASK_PRIORITY, StackVideoItemTest);
	h264codec_simulator ();
}


int os_main (void)
{
	// ����ϵͳ��Ϣ����
	XMSYS_MessageInit ();
	SDCard_Module_Init();
	/* You need to create at least one task before calling OS_Start() */
	OS_CREATETASK(&TCBMAIN, "Main", MainTask, 250, StackMain);

	return 0;
}