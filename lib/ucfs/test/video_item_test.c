#include <stdio.h>
#include <stdlib.h>
#include "ark1960.h"
#include "RTOS.h"		// OSͷ�ļ�
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
	
	char file_name[64];	// ���ļ�ȫ·����
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
	
	// ѭ��¼��ֱ��¼��ֹͣ
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
			// ������ʱ��Ƶ�ļ������ڼ�¼��Ƶ��
			// 	���洢����һ�β���ʱ��ĳЩ����»�ȡ���ļ�ϵͳ����������ϳ�ʱ��(10��~30��)��������Ƶ�����ݿ���ʱ�޷�ʹ�ã�
			// 	�������޷���¼��Ƶ��
			// 	Ϊ����������⣬����һ����ʱ��Ƶ��¼�ļ������ڱ���ʵʱ��Ƶ��
			if(XM_VideoItemCreateVideoTempFile (channel, h264name_f, sizeof(h264name_f)))
			{
				// ����һ����ʱ��Ƶ�ļ��ɹ�
				strcpy (file_name, h264name_f);
				file_save = 1;
				file_temp = 1;	// �������ʱ��Ƶ���ļ�
			}
			else
			{
				// ����һ���߼���Ƶ�ļ�(�޷�д��)
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
				// �����ļ���
				strcpy (file_name, h264name_f);
				file_save = 1;
				file_temp = 0;
			}
			else
				goto create_video_item_failed;
		}
		
		// ģ��H264���룬ģ��ÿ��д��12Mb(1.5MB)��ÿ���ļ�8���ӣ�
		{
			unsigned int ticket = XM_GetTickCount();
			void *fp = h264_fopen (file_name, "wb");
			int ret = 0;
			if(file_save)
				XM_printf ("open ticket=%03d, %s\n", XM_GetTickCount() - ticket, file_name);
			if(fp)
			{
				// ��30֡/��д�룬ÿ��д��51KB
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
							// ��鿨״̬
							if(!FS_IsVolumeMounted("mmc:0:"))
								break;
							
							if(ret == (-2))
							{
								XM_printf ("disk full\n");
								break;
							}
						}
						// ��ʱ25ms
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
		// �����Ƶ�����Ļ��������Ƿ��ѿ���
		while(!XM_VideoItemIsBasicServiceOpened())
		{
			// δ����
			OS_Delay (1);
		}
		// ��Ǳ������ѿ�ʼ����
		XM_CodecStartEventSet ();
		
		_h264codec_simulator ();
		
	} while (1);
}

