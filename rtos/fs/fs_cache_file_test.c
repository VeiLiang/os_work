#include <stdio.h>
#include <stdlib.h>
#include "ark1960.h"
#include "RTOS.h"		// OS头文件
#include "FS.h"
#include "xm_type.h"
#include "xm_base.h"
#include <assert.h>
#include <xm_printf.h>
#include <xm_file.h>

#include "ark1960_testcase.h"
#include "xm_h264_file.h"
#include "xm_h264_cache_file.h"
#include "xm_h264_cache_command_queue.h"

extern void XMSYS_H264FileInit (void);
extern void XMSYS_H264FileExit (void);


#define	TEST_FIFO_SIZE		0x100000
#define	TEST_DATA_SIZE		(TEST_FIFO_SIZE/4)

// Cache文件测试代码
static int test_data[TEST_FIFO_SIZE/4];
static int read_temp[TEST_FIFO_SIZE/32/4];
static int read_temp_2[TEST_FIFO_SIZE/32/4];
void H264CacheFileTest (char *volume_label)
{
	char *data;
	char filename_1[32];
	char filename_2[32];
	int i, num, loop;
	void *h246File_1;
	void *h246File_2;
	
	int fail_count_1 = 0;
	int fail_count_2 = 0;
	
	int file_count = 200;

	unsigned int s_ticket;
	unsigned int  e_ticket;
		
	printf ("H264CacheFileTest Start\n");
	
	XMSYS_H264FileInit ();
	for (i = 0; i < TEST_FIFO_SIZE/4; i++)
	{
		test_data[i] = i;
	}
	while(1)
	{
		printf ("H264 Cache File Write Test Begin\n");
		s_ticket = XM_GetTickCount ();
		
		for (num = 0; num < file_count; num ++)
		{
			unsigned long s_tick = XM_GetTickCount();
			unsigned long e_tick;
			printf ("num=%d\n", num);
			sprintf (filename_1, "%s\\File%d.txt", volume_label, num*2 );
			sprintf (filename_2, "%s\\File%d.txt", volume_label, num*2 + 1);
						
			// H264 Cache文件写测试
			h246File_1 = h264_fopen (filename_1, "wb");
			h246File_2 = h264_fopen (filename_2, "wb");
			if(h246File_1 && h246File_2)
			{
				int k = 200;
				int l;
				//while(k > 0)
				{
					size_t offset;
					data = (char *)test_data;
					for (loop = 0; loop < 32; loop ++)
					{

						h264_fwrite (data, 1, (TEST_DATA_SIZE * 4) / 32, h246File_1);
						
						
						// 获取当前文件指针
						offset = h264_fseek (h246File_1, 0, XM_SEEK_CUR);
						h264_fseek (h246File_1, -1024, XM_SEEK_CUR);
						h264_fwrite (data + (TEST_DATA_SIZE * 4) / 32  - 1024, 1, 1024, h246File_1);
						h264_fseek (h246File_1, offset, XM_SEEK_SET);

						h264_fseek (h246File_1, offset-10000, XM_SEEK_SET);
						h264_fwrite (data + (TEST_DATA_SIZE * 4) / 32  - 10000, 1, 10000, h246File_1);
					
						h264_fseek (h246File_1, offset-1000, XM_SEEK_SET);
						h264_fwrite (data + (TEST_DATA_SIZE * 4) / 32  - 1000, 1, 100, h246File_1);
						
						h264_fseek (h246File_1, offset, XM_SEEK_SET);

						h264_fwrite (data, 1, (TEST_DATA_SIZE * 4) / 32, h246File_2);
						data += (TEST_DATA_SIZE * 4) / 32;
					}


					// 定位到头部
					h264_fseek (h246File_1, 1, XM_SEEK_SET);
					h264_fwrite ( ((char *)test_data) + 1, 1, H264_CACHE_FILE_BLOCK_SIZE, h246File_1);

					l = 0;
					data = (char *)test_data;
					offset = 0x80000 - 0x70000;
					while(l < 0x1000)
					{
						h264_fseek (h246File_1, offset, XM_SEEK_SET);
						h264_fwrite ( data + offset, 1, l, h246File_1);
						offset = 0x80000 + (rand() % 0xC000);
						l ++;
					}

					k --;
				}
				h264_fclose (h246File_1);
				h264_fclose (h246File_2);
				//FS_FClose (h246File_1);
			}
			
			xm_h264_cache_command_queue_waitfor_cache_command_done ();
			
			
			//FS_CACHE_Clean("");
			
			// H264 文件读测试
			printf("  Read & Compare file %s\n", filename_1);
			h246File_1 = h264_fopen (filename_1, "rb");
			h246File_2 = h264_fopen (filename_2, "rb");
			//h246File_1 = FS_FOpen (filename_1, "r");
			if(h246File_1 && h246File_2)
			{
				int k = 200;
				//while(k > 0)
				{
					//printf ("k=%d\n", k);
					data = (char *)test_data;
					for (loop = 0; loop < 32; loop ++)
					{
						//printf ("%d ", loop);
						if(h264_fread (read_temp, 1, (TEST_DATA_SIZE * 4) / 32, h246File_1) != (TEST_DATA_SIZE * 4) / 32)
						{
							printf ("read err 1\n");
						}
						else if(memcmp (read_temp, data, (TEST_DATA_SIZE * 4) / 32))
						{
#if 0
							int index = 0, count;
							while(index < (TEST_DATA_SIZE * 4) / 32)
							{
								if(read_temp[index] != ((int *)data)[index])
									break;
								index ++;
							}
							
							// 打印出错后的16字节数据
							printf ("Compare 1 Failed，index=0x%08x\n", index);
							count = 0;
							while (index < (TEST_DATA_SIZE * 4) / 32)
							{
								printf ("index=0x%08x  %08x  %08x\n", index, read_temp[index], ((int *)data)[index]);
								count ++;
								if(count >= (1024/4))
									break;
								index ++;
							}
#else
							printf ("Compare 1 Failed\n");
#endif
							
							fail_count_1 ++;
							break;
						}
						if(h264_fread (read_temp_2, 1, (TEST_DATA_SIZE * 4) / 32, h246File_2) != ((TEST_DATA_SIZE * 4) / 32))
						{
							printf ("read err 2\n");
						}
						else if(memcmp (read_temp_2, data, (TEST_DATA_SIZE * 4) / 32))
						{
#if 0
							int index = 0, count;
							while(index < (TEST_DATA_SIZE * 4) / 32)
							{
								if(read_temp_2[index] != ((int *)data)[index])
									break;
								index ++;
							}
							
							// 打印出错后的16字节数据
							printf ("Compare 2 Failed，index=0x%08x\n", index);
							count = 0;
							while (index < (TEST_DATA_SIZE * 4) / 32)
							{
								printf ("index=0x%08x  %08x  %08x\n", index, read_temp_2[index], ((int *)data)[index]);
								count ++;
								if(count >= (1024/4))
									break;
								index ++;
							}							
#else
							printf ("Compare 2 Failed\n");
#endif
							
							fail_count_2 ++;
							break;
						}
						data += (TEST_DATA_SIZE * 4) / 32;
					}
					k --;
				}

				h264_fclose (h246File_1);
				h264_fclose (h246File_2);
				
				e_tick = XM_GetTickCount();
				printf ("ticket=%d\n", e_tick - s_tick);
			}
		}
		
		e_ticket = XM_GetTickCount ();
		
		printf ("Time=%d, fail_count_1=%d, fail_count_2=%d\n", e_ticket - s_ticket, fail_count_1, fail_count_2);
		
		if(fail_count_1 || fail_count_2)
		{
			printf ("Please checkout SD card\n");
			//while(1);
		}
		
		
		printf ("%d H264 File Write Test End\n", file_count);
		
		XM_Delay (10000);
		//break;
	}
}

int fs_device_cache_file_test (const char *device_name, unsigned int device_index)
{
	char vol_name[32];
	//sprintf (vol_name, "%s:%d:", device_name, device_index);
	if(device_index == 0)
		sprintf (vol_name, "%s:%d:", device_name, device_index);
	else
		sprintf (vol_name, "%s%d:0", device_name, device_index);
	
	printf ("Please Insert volume(%s) to begin\n", vol_name);
	while(1){
		int volume_status = FS_GetVolumeStatus (vol_name);
		if(volume_status == FS_MEDIA_IS_PRESENT)
		{
			printf ("Volume(%s) present\n", vol_name);
			break;
		}
		OS_Delay (100);
	}

	H264CacheFileTest (vol_name);
	
	return 0;
}

