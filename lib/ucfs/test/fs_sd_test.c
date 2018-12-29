#include <stdio.h>
#include <stdlib.h>
#include "ark1960.h"
//#include "RTOS.h"		// OS头文件
#include "FS.h"
#include "xmtype.h"
#include "xmbase.h"
#include <assert.h>
#include <xmprintf.h>

#include "ark1960_testcase.h"

//#define	TEST_FILE_COUNT 128
#define	TEST_FILE_COUNT 16

#undef printf
#define	printf	XM_printf

// ********************************************************************
//
//        SD卡功能验证模块
//	F:\old\ywj\ywj\ark1960\bit_file\20131223_rtc_usb_nand_sdmmc_spi_\20131223_rtc_usb_nand_sdmmc_spi_.bit
//
// ********************************************************************
// 1) 验证SD卡的DMA读、DMA写过程
// 2) 基于文件系统的文件写入验证SD卡(卡格式化、文件创建、写入、读出比较)
// 3) SD多卡文件系统测试(尚未测试)

// 返回  0  测试过程成功
// 返回 -1  测试过程失败
// sd_dev_index 表示测试SD卡卡设备序号(从0开始，当前支持两个SD设备0及1)
int _fs_sd_function_test (unsigned int sd_dev_index)
{
	FS_FILE *fp;
	FS_DISK_INFO Info;
	int ret = 0;
	int i;
	char dev_name[32];
	FS_FORMAT_INFO FormatInfo;
	FS_DEV_INFO DevInfo;
	char filename[32];
	unsigned int *fs_buff;
	unsigned int *fs_temp;
	unsigned int r_start_ticket, r_end_ticket;
	unsigned int w_start_ticket, w_end_ticket;
	
	if(sd_dev_index >= SDMMC_DEV_COUNT)
	{
		printf ("sd_dev(%d) exceed maximum device number(%d) within ARK1960\n", 
				  sd_dev_index, (SDMMC_DEV_COUNT-1));
		return -1;	// test NG
	}

	printf ("\n\nSDMMC (slot %d) Testing\n", sd_dev_index);
	printf ("Please Insert sdmmc card into slot %d to begin\n", sd_dev_index);
	while(1){
		int volume_status;
		memset (dev_name, 0, sizeof(dev_name));
		sprintf (dev_name, "mmc:%d:", sd_dev_index);
		volume_status = FS_GetVolumeStatus (dev_name);
		//int volume_status = FS_GetVolumeStatus ("mmc:0:");
		if(volume_status == FS_MEDIA_IS_PRESENT)
		{
			printf ("SDMMC (slot %d) present\n", sd_dev_index);
			break;
		}
		XM_Sleep (100);
	}
	
	
	memset (dev_name, 0, sizeof(dev_name));
	sprintf (dev_name, "mmc:%d:", sd_dev_index);
	
	// SD卡对应的文件系统格式化
#if 1
	printf ("FS_Format %s\n", dev_name);
	FS_SDMMC_Driver.pfIoCtl (0, FS_CMD_GET_DEVINFO, 0, &DevInfo);
	FormatInfo.SectorsPerCluster = 64;
	FormatInfo.NumRootDirEntries = 512;
	FormatInfo.pDevInfo = &DevInfo;
	//if(FS_FormatSD (dev_name)!= 0)
	//if(FS_Format(dev_name, &FormatInfo) != 0)
	if(FS_Format(dev_name, NULL) != 0)
	{
		printf ("FS_TEST(%s) NG, Format %s failed\n", dev_name, dev_name);
		return -1;	// test NG
	}
	FS_CACHE_Clean ("");
#endif
	
	// 读取卡容量信息
	if (FS_GetVolumeInfo(dev_name, &Info) == -1) 
	{
		printf("FS_TEST(%s) NG, Failed to get volume(%s) information.\n", 
				 dev_name, dev_name);
		return -1;	// test NG
	} 
	else 
	{
		printf ("volume (%s) information\n", dev_name);
		printf("Number of total clusters = %d\n"
           "Number of free clusters  = %d\n"
           "Sectors per cluster      = %d\n"
           "Bytes per sector         = %d\n", 
           Info.NumTotalClusters, 
           Info.NumFreeClusters, 
           Info.SectorsPerCluster, 
           Info.BytesPerSector);
	}	
	
	#define FS_BUFF_SIZE	(256 * 1024 * 4)
	fs_buff = (unsigned int *)malloc (FS_BUFF_SIZE);
	fs_temp = (unsigned int *)malloc (FS_BUFF_SIZE);
	if (fs_buff == NULL || fs_temp == NULL)
	{
		printf("FS_TEST(%s) NG, memory allocation failed\n", dev_name);
		if(fs_buff)
			free (fs_buff);
		if(fs_temp)
			free (fs_temp);
		return -1;
	}
	i = 0;
	while (i < (FS_BUFF_SIZE/4))
	{
		fs_buff[i] = i;
		i ++;
	}
	i = 0;
	
	w_start_ticket = XM_GetTickCount ();
	while (i < TEST_FILE_COUNT)
	{
		sprintf (filename, "%s\\t_%04d.bin", dev_name, i);
		// printf ("--> filename %s\n", filename);
		fp = FS_FOpen (filename, "wb");
		if(fp)
		{
			//printf ("open %s OK\n", filename);
			
			FS_FWrite (fs_buff, 1, FS_BUFF_SIZE, fp);
			FS_FClose (fp);
		}
		else
		{
			ret = -1;	// 标记测试存在失败
			printf ("open %s NG\n", filename);
		}
		i ++;
	}
	
	// 刷新CACHE内容到SD卡
	FS_CACHE_Clean(dev_name);
	w_end_ticket = XM_GetTickCount ();
	
	r_start_ticket = w_end_ticket;
	i = 0;
	// 读取并检查文件
	while (i < TEST_FILE_COUNT)
	{
		memset (fs_temp, 0, FS_BUFF_SIZE);
		sprintf (filename, "%s\\t_%04d.bin", dev_name, i);
		fp = FS_FOpen (filename, "rb");
		if(fp)
		{
			//printf ("open %s OK\n", filename);
			
			FS_FRead (fs_temp, 1, FS_BUFF_SIZE, fp);
			
			FS_FClose (fp);
			
			if(memcmp (fs_temp, fs_buff, FS_BUFF_SIZE) == 0)
			{
				//printf ("w/r/c OK\n");
			}
			else
			{
				ret = -1;	// 标记测试存在失败
				printf ("w/r/c NG\n");
			}
		}
		else
		{
			ret = -1;	// 标记测试存在失败
			printf ("open %s NG\n", filename);
		}
		i ++;
	}
	r_end_ticket = XM_GetTickCount ();
	
	printf("\nFS_TEST(%s) OK\n", dev_name);
	printf ("w   %dM(%d file), time=%d\n", TEST_FILE_COUNT, TEST_FILE_COUNT, w_end_ticket - w_start_ticket);
	printf ("r/c %dM(%d file), time=%d\n", TEST_FILE_COUNT, TEST_FILE_COUNT, r_end_ticket - r_start_ticket);
	printf ("\n");
	
	free (fs_buff);
	free (fs_temp);
	
	return ret;
}

int fs_sd_function_test (unsigned int sd_dev_index)
{
	char dev_name[32];
		int volume_status;
		if(sd_dev_index >= SDMMC_DEV_COUNT)
		{
			printf ("sd_dev(%d) exceed maximum device number(%d) within ARK1960\n", 
					  sd_dev_index, (SDMMC_DEV_COUNT-1));
			return -1;	// test NG
		}
	
		printf ("\n\nSDMMC (slot %d) Testing\n", sd_dev_index);
		printf ("Please Insert sdmmc card into slot %d to begin\n", sd_dev_index);
		while(1){
			memset (dev_name, 0, sizeof(dev_name));
			sprintf (dev_name, "mmc:%d:", sd_dev_index);
			volume_status = FS_GetVolumeStatus (dev_name);
			//int volume_status = FS_GetVolumeStatus ("mmc:0:");
			if(volume_status == FS_MEDIA_IS_PRESENT)
			{
				printf ("SDMMC (slot %d) present\n", sd_dev_index);
				break;
			}
			XM_Sleep (100);
		}

	do
	{
		
		_fs_sd_function_test (sd_dev_index);
		
		
		while(1)
		{
			int volume_status = FS_GetVolumeStatus (dev_name);
			if(volume_status != FS_MEDIA_IS_PRESENT)
				break;
		}
		
		printf ("Please Insert volume(%s) to begin\n", dev_name);
		while(1)
		{
			int volume_status = FS_GetVolumeStatus (dev_name);
			if(volume_status == FS_MEDIA_IS_PRESENT)
			{
				printf ("Volume(%s) present\n", dev_name);
				break;
			}
			XM_Sleep (100);
		}
		
		
	} while(1);
	
}

