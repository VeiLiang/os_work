#include <stdio.h>
#include <stdlib.h>
#include "ark1960.h"
#include "RTOS.h"		// OS头文件
#include "FS.h"
#include "xmtype.h"
#include "xmbase.h"
#include "xmcore.h"
#include <assert.h>
#include <xmprintf.h>
#include "xmvideoitem.h"
#include "xm_h264_file.h"

#include "ark1960_testcase.h"


#define	FRAME_SIZE	(51*1024)

#include <xmvideoitem.h>
int _h264codec_simulator (void)
{
	char h264name[32];
	char h264name_f[32];
	char h264name_r[32];
	
	char file_name[64];	// 流文件全路径名
	int file_save, file_temp;

	HANDLE hVideoItem;
	XMVIDEOITEM *pVideoItem;
	XMSYSTEMTIME CurrTime;
	
	char *frame_data;
	int i;
		
	frame_data = OS_malloc (FRAME_SIZE);
	if(frame_data == NULL)
	{
		XM_printf ("OS_malloc frame_data failed\n");
		return -1;
	}
	for (i = 0; i < FRAME_SIZE; i ++)
		frame_data[i] = (char)(i & 0xFF);
	
	// 循环录像，直到录像停止
	while(1)
	{
		unsigned int channel;
		memset (h264name_f, 0, sizeof(h264name_f));
		memset (h264name_r, 0, sizeof(h264name_r));
		memset (h264name, 0, sizeof(h264name));
		
		memset (file_name, 0, sizeof(file_name));
		file_save = 0;
		file_temp = 0;
		
		
		hVideoItem = NULL;
		XM_GetLocalTime(&CurrTime);
		channel = 0;
		hVideoItem = XM_VideoItemCreateVideoItemHandle (channel, &CurrTime);
		if(hVideoItem == NULL)
		{
			create_video_item_failed:
			// 创建临时视频文件，用于记录视频。
			// 	当存储卡第一次插入时，某些情况下获取卡文件系统容量会持续较长时间(10秒~30秒)，导致视频项数据库暂时无法使用，
			// 	而导致无法记录视频。
			// 	为解决上述问题，创建一个临时视频记录文件，用于保存实时视频。
			if(XM_VideoItemCreateVideoTempFile (channel, h264name_f, sizeof(h264name_f)))
			{
				// 创建一个临时视频文件成功
				strcpy (file_name, h264name_f);
				file_save = 1;
				file_temp = 1;	// 标记是临时视频项文件
			}
			else
			{
				// 创建一个逻辑视频文件(无法写入)
				sprintf (h264name_f, "\\TEST.AVI");
				strcpy (file_name, h264name_f);
				file_save = 0;
			}
		}
		else
		{
			pVideoItem = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
			if(pVideoItem == NULL)
			{
				hVideoItem = NULL;
				goto create_video_item_failed;
			}
			if(XM_VideoItemGetVideoFilePath (pVideoItem, channel, h264name_f, sizeof(h264name_f)))
			{
				// 保存文件名
				strcpy (file_name, h264name_f);
				file_save = 1;
				file_temp = 0;
			}
			else
				goto create_video_item_failed;
		}
		
		// 模拟H264编码，模拟每秒写入12Mb(1.5MB)，每个文件8分钟，
		{
			unsigned int ticket = XM_GetTickCount();
			void *fp = h264_fopen (file_name, "wb");
			int ret = 0;
			if(file_save)
				XM_printf ("open ticket=%03d, %s\n", XM_GetTickCount() - ticket, file_name);
			if(fp)
			{
				// 按30帧/秒写入，每次写入51KB
				int second_count = 2 * 60;
				ret = 0;
				while(second_count && ret >= 0)
				{
					unsigned int ticket = OS_GetTime ();
					for (i = 0; i < 30; i++)
					{
						ret = h264_fwrite (frame_data, 1, FRAME_SIZE, fp);
						if(ret < 0)
						{
							// 检查卡状态
							if(!FS_IsVolumeMounted("mmc:0:"))
								break;
							
							if(ret == (-2))
							{
								XM_printf ("disk full\n");
								break;
							}
						}
						// 延时25ms
						OS_DelayUntil (ticket + i * 25);
					}
					second_count --;
				}
				
				h264_fclose (fp);
				
				if(ret == (-2))
				{
					XM_printf ("delay to waiting for delete\n");
					XM_VideoItemMonitorAndRecycleGarbage (0);
					OS_Delay (30000);
				}
			}
			
			{
			unsigned int spend_ticket = XM_GetTickCount() - ticket;
			if(ret >= 0 && spend_ticket > 0)
				XM_printf ("average write speed = %d\n", 120 * 30 * FRAME_SIZE / spend_ticket);
			}
			OS_Delay (1);
		}
		
	}
	
	OS_free (frame_data);
	
	return 0;
}

int h264codec_simulator (void)
{
	XMSYSTEMTIME SystemTime;
	SystemTime.wYear = 2016;
	SystemTime.bMonth = 1;
	SystemTime.bDay = 1;
	SystemTime.bHour = 0;
	SystemTime.bMinute = 0;
	SystemTime.bSecond = 0;
	SystemTime.wMilliSecond = 0;
	XM_SetLocalTime (&SystemTime);
	do 
	{
		// 检查视频项服务的基本服务是否已开启
		while(!XM_VideoItemIsBasicServiceOpened())
		{
			// 未开启
			OS_Delay (1);
		}
		// 标记编码器已开始工作
		XM_CodecStartEventSet ();
		
		_h264codec_simulator ();
		
	} while (1);
}

