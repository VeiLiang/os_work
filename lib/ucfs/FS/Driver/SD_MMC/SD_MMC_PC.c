#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <SYS\STAT.H>
#include <errno.h>
#include "sdmmc.h"
#include "FS.h"
#include "fs_card.h"
#include "fs_port.h"
#include "printk.h"
#include <io.h>
#include <fcntl.h>
#include "xmprintf.h"
#include "fevent.h"

#define MAX_SDMMC_DEV		1
#define NUM_UNITS          1

static SDMMC_INFO sdmmc_info_chip0;
static int card_insert_state[MAX_SDMMC_DEV];
CRITICAL_SECTION  critical_section[MAX_SDMMC_DEV];
int fd_card[MAX_SDMMC_DEV];		// 模拟的PC文件句柄

U32 card_size[MAX_SDMMC_DEV] = {
	0				// 总扇区数
};

extern volatile FS_CARD SDCard;

SDMMC_INFO * sdmmc_chip0 = &sdmmc_info_chip0;

INT32 SD_ReadSector(SDMMC_INFO * sdmmc_info,UINT32 Unit,ULONG Sector,UINT8 *pBuffer, ULONG NumSectors)
{
	int dev;
	__int64 off;
	INT32 r = -1;
	dev = sdmmc_info->ChipSel - CHIP0;
	if(dev < 0 || dev >= MAX_SDMMC_DEV)
		return -1;

	if(Sector >= card_size[dev])
	{
		XM_printf ("Sector (%d) exceed maximum sector number (%d)\n", card_size[dev]);
		return -1;
	}
	if((Sector + NumSectors) >= card_size[dev])
	{
		XM_printf ("Sector (%d) + NumSectors(%d) exceed maximum sector number (%d)\n", Sector, NumSectors, card_size[dev]);
		return -1;
	}
	EnterCriticalSection (&critical_section[dev]);
	if(fd_card[dev] >= 0)
	{
		off = Sector;
		off *= 512;
		if(_lseeki64 (fd_card[dev], off, SEEK_SET) == off)
		{
			if(_read (fd_card[dev], pBuffer, NumSectors * 512) == (int)(NumSectors * 512))
				r = 0;
			else
				r = -1;
		}
		else
		{
			r = -1;
		}
	}
	else
	{
		r = -1;
	}
	
	LeaveCriticalSection (&critical_section[dev]);
	return r;
}

INT32 SD_WriteSector(SDMMC_INFO * sdmmc_info,UINT32 Unit,ULONG  Sector,UINT8 *pBuffer, ULONG  NumSectors, U8 RepeatSame)
{
	int dev;
	__int64 off;
	INT32 r = -1;
	dev = sdmmc_info->ChipSel - CHIP0;
	if(dev < 0 || dev >= MAX_SDMMC_DEV)
		return -1;

	if(Sector >= card_size[dev])
	{
		XM_printf ("Sector (%d) exceed maximum sector number (%d)\n", card_size[dev]);
		return -1;
	}
	if((Sector + NumSectors) >= card_size[dev])
	{
		XM_printf ("Sector (%d) + NumSectors(%d) exceed maximum sector number (%d)\n", Sector, NumSectors, card_size[dev]);
		return -1;
	}
	EnterCriticalSection (&critical_section[dev]);
	if(fd_card[dev] >= 0)
	{
		off = Sector;
		off *= 512;
		_lseeki64 (fd_card[dev], off, SEEK_SET);
		_write (fd_card[dev], pBuffer, NumSectors * 512);
		r = 0;
	}
	else
	{
		r = -1;
	}
	
	LeaveCriticalSection (&critical_section[dev]);
	return r;
}

INT32 SD_WriteMultiBuffer(SDMMC_INFO * sdmmc_info, ULONG  Sector, ULONG  NumSectors, ULONG NumBuffers, const UINT8 **pSectorBuffer, ULONG *pSectorCount)
{
	int dev;
	__int64 off;
	ULONG i, sector_count;
	INT32 r = -1;
	dev = sdmmc_info->ChipSel - CHIP0;
	if(dev < 0 || dev >= MAX_SDMMC_DEV)
		return -1;

	if(Sector >= card_size[dev])
	{
		XM_printf ("Sector (%d) exceed maximum sector number (%d)\n", card_size[dev]);
		return -1;
	}
	if((Sector + NumSectors) >= card_size[dev])
	{
		XM_printf ("Sector (%d) + NumSectors(%d) exceed maximum sector number (%d)\n", Sector, NumSectors, card_size[dev]);
		return -1;
	}

	sector_count = 0;
	for (i = 0; i < NumBuffers; i ++)
	{
		sector_count += pSectorCount[i];
	}
	if(sector_count != NumSectors)
	{
		XM_printf ("NumSectors (%d) mismatch buffers's sector number(%d)\n", NumSectors, sector_count);
		return -1;
	}

	EnterCriticalSection (&critical_section[dev]);
	if(fd_card[dev] >= 0)
	{
		off = Sector;
		off *= 512;
		_lseeki64 (fd_card[dev], off, SEEK_SET);
		for (i = 0; i < NumBuffers; i ++)
		{
			_write (fd_card[dev], pSectorBuffer[i], pSectorCount[i] * 512);
		}
		r = 0;
	}
	else
	{
		r = -1;
	}
	
	LeaveCriticalSection (&critical_section[dev]);
	return r;
}

INT32 SD_ReadMultiBuffer(SDMMC_INFO * sdmmc_info, ULONG  Sector, ULONG  NumSectors, ULONG NumBuffers, UINT8 **pSectorBuffer, ULONG *pSectorCount)
{
	int dev;
	__int64 off;
	ULONG i, sector_count;
	INT32 r = -1;
	dev = sdmmc_info->ChipSel - CHIP0;
	if(dev < 0 || dev >= MAX_SDMMC_DEV)
		return -1;

	if(Sector >= card_size[dev])
	{
		XM_printf ("Sector (%d) exceed maximum sector number (%d)\n", card_size[dev]);
		return -1;
	}
	if((Sector + NumSectors) >= card_size[dev])
	{
		XM_printf ("Sector (%d) + NumSectors(%d) exceed maximum sector number (%d)\n", Sector, NumSectors, card_size[dev]);
		return -1;
	}

	sector_count = 0;
	for (i = 0; i < NumBuffers; i ++)
	{
		sector_count += pSectorCount[i];
	}
	if(sector_count != NumSectors)
	{
		XM_printf ("NumSectors (%d) mismatch buffers's sector number(%d)\n", NumSectors, sector_count);
		return -1;
	}

	EnterCriticalSection (&critical_section[dev]);
	if(fd_card[dev] >= 0)
	{
		off = Sector;
		off *= 512;
		_lseeki64 (fd_card[dev], off, SEEK_SET);
		for (i = 0; i < NumBuffers; i ++)
		{
			_read (fd_card[dev], pSectorBuffer[i], pSectorCount[i] * 512);
		}
		r = 0;
	}
	else
	{
		r = -1;
	}
	
	LeaveCriticalSection (&critical_section[dev]);
	return r;
}



#define SDCARD_REMOVE		0
#define SDCARD_INSERT		1

static void PostCardEvent(UINT32 ulPlugIn, UINT32 ulCardID)
{
	
	fEvent curEvent;
	fCard_Event *pCardEvent=NULL;
	printk ("SDMMC %d %s\n", ulCardID, (ulPlugIn == SDCARD_INSERT) ? "INSERT" : "REMOVE");
	
	pCardEvent = (fCard_Event *)(&curEvent.uFEvent);
	pCardEvent->event_type = CARD_EVENT;
	pCardEvent->cardType = SD_DEVICE;
	pCardEvent->cardID = ulCardID;
	pCardEvent->plugIn = ulPlugIn;

//	libModeMng_PostEvent(&curEvent);	
	addEventQueue(&curEvent);
}

void SDCard_insert (int dev, const char *sdmmc_file_name, __int64 sdmmc_size)
{
	char temp[MAX_PATH];
	int fd;
	__int64 size;
	if(dev >= MAX_SDMMC_DEV)
	{
		XM_printf ("dev %d illegal\n", dev);
		return;
	}

	EnterCriticalSection (&critical_section[dev]);
	// 检查SD卡是否已插入(文件已存在)
	// 
	if(fd_card[dev] >= 0)
	{
		XM_printf ("Card %d inserted\n", dev);
		LeaveCriticalSection (&critical_section[dev]);
		return;
	}
	// 检查文件是否存在, 大小是否正确
	//sprintf (temp, "d:\\SD_%d.BIN", dev);
	strcpy (temp, sdmmc_file_name);
	fd = _open (temp, _O_RDONLY|_O_BINARY);
	if(fd >= 0)
	{
		__int64 sd_real_size = sdmmc_size;
		//sd_real_size *= 512;
		size = _lseeki64 (fd, 0, SEEK_END);
		_close (fd);
		if(size == sd_real_size)
		{
			fd = _open (temp, _O_BINARY|_O_RDWR, _S_IREAD | _S_IWRITE);
		}
		else
		{
			fd = _open (temp, _O_BINARY|_O_CREAT|_O_TRUNC|_O_RDWR, _S_IREAD | _S_IWRITE);
			if(fd >= 0)
			{
				char *buff = (char *)malloc (0x100000);
				__int64 loop = sdmmc_size / 0x100000;
				memset (buff, 0, sizeof(buff));
				while(loop > 0)
				{
					if(_write (fd, buff, 0x100000) != 0x100000)
						break;
					loop --;
				}

				_lseeki64 (fd, 0, SEEK_SET);
			}
			else
			{
				XM_printf("errno=%d\n", errno);
			}
		}
	}
	else
	{
		// 创建新的ROM文件
		fd = _open (temp, _O_BINARY|_O_CREAT|_O_RDWR, _S_IREAD | _S_IWRITE);
		if(fd >= 0)
		{
				char *buff = (char *)malloc (0x100000);
				__int64 loop = sdmmc_size / 0x100000;
				memset (buff, 0, sizeof(buff));
				while(loop > 0)
				{
					if(_write (fd, buff, 0x100000) != 0x100000)
						break;
					loop --;
				}

				_lseeki64 (fd, 0, SEEK_SET);
		}

	}

	if(fd < 0)
	{
		XM_printf ("Card %d insert failed, simulated file NG", dev);
		LeaveCriticalSection (&critical_section[dev]);
		return;
	}

	card_size[dev] = (unsigned int)(sdmmc_size / 512);
	SDCard.SecNum = card_size[dev];
	SDCard.Present = 1;
	SDCard.Changed = 0;

	fd_card[dev] = fd;

	LeaveCriticalSection (&critical_section[dev]);

	PostCardEvent(SDCARD_INSERT, dev);

	/*
	sprintf (temp, "mmc:%d:", dev);
	if(FS_Mount (temp) == 0)
		XM_printf ("mount (%s) success\n", temp);
	else
		XM_printf ("mount (%s) failure\n", temp);
	*/
}

void SDCard_remove (int dev)
{
	char temp[32];
	sprintf (temp, "mmc:%d:", dev);
	if(dev >= MAX_SDMMC_DEV)
		return;

	//FS_UnmountForced (temp);

	EnterCriticalSection (&critical_section[dev]);
	if(fd_card[dev] >= 0)
	{
		_close (fd_card[dev]);
		fd_card[dev] = -1;
	}
	SDCard.SecNum = 0;
	SDCard.Present = 0;
	SDCard.Changed = 1;
	LeaveCriticalSection (&critical_section[dev]);

	PostCardEvent(SDCARD_REMOVE, dev);
}

void SDCard_Module_CardCheck (void)
{
	__int64 sd_size = 2 * 1024 * 1024;
	sd_size *= 1024;

	SDCard_insert (0, "D:\\SDMMC_2GB.BIN", sd_size);

}

void SDCard_Module_Init(void)
{
	int i;
	sdmmc_chip0->ChipSel = CHIP0;
	for (i = 0; i < MAX_SDMMC_DEV; i++)
	{
		fd_card[i] = -1;
		card_insert_state[i] = 0;
		InitializeCriticalSection (&critical_section[i]);
	}
}

INT32 InitSDMMCCard(UINT32 ulCardID)
{
	return 0;
}

